# Development Notes

## Current state (2026-04-11)

All recent work has been optimisation and reliability hardening — no functional changes to the DL bus protocol handling or MQTT topic layout.

---

## Recent commit history (relevant)

| Commit | Summary |
|--------|---------|
| `a7d973a` | Speed up DL bus frame handling — replaced 1-second polling with `frame_complete` volatile flag; processing is now event-driven |
| `15ba419` | Publish MQTT state only when values change — added per-sensor/output cache; full republish every 15 minutes |
| `e8da4d7` | Clarify firmware compatibility for ESP8266 in README |

---

## Session optimisations (applied 2026-04-11, not yet committed)

Seven low-level optimisations were identified by static analysis and verified against the live code before being applied. None touch the DL bus interrupt path in a way that changes behaviour.

### 1 · `receive.ino` — single `micros()` call in ISR

`pin_changed()` was calling `micros()` twice: once to compute `time_diff` and once to update `last_bit_change`. A single call into `now` is captured first, eliminating a tiny but real timing skew inside the ISR.

```cpp
// before
unsigned long time_diff = micros() - last_bit_change;
last_bit_change = micros();

// after
unsigned long now = micros();
unsigned long time_diff = now - last_bit_change;
last_bit_change = now;
```

### 2 · `process.ino` — byte-level XOR in `invert()`

`invert()` was flipping bits one at a time (1 313 iterations for a 1 312-bit buffer). Replaced with a byte-level XOR over the 165-byte backing array — ~8× fewer iterations.

```cpp
// before
for (int i = 0; i <= bit_number; i++)
  write_bit(i, read_bit(i) ? 0 : 1);

// after
int byte_count = bit_number / 8 + 1;
for (int i = 0; i < byte_count; i++)
  data_bits[i] ^= 0xFF;
```

The 7 unused padding bits at the end of `data_bits[164]` are also flipped, but they are never read by `analyze()` or `trim()` (both iterate only up to `bit_number`), so behaviour is identical.

### 3 · `process.ino` — `trim()` offset counter

`trim()` was computing `offset = i - start_bit` on every iteration. A third loop variable incremented in the `for` header is equivalent and eliminates the subtraction.

```cpp
// before
for (int i = start_bit, bit = 0; i < bit_number; i++) {
  int offset = i - start_bit;

// after
for (int i = start_bit, bit = 0, offset = 0; i < bit_number; i++, offset++) {
```

### 4 · `process.ino` — bounds guard in `analyze()`

The inner `while (read_bit(i) == 1) i++;` had no upper bound. If a run of 1-bits reached the end of `data_bits[]`, the read would go out of bounds. Added `i < bit_number` guard.

```cpp
// before
while (read_bit(i) == 1)
  i++;

// after
while (i < bit_number && read_bit(i) == 1)
  i++;
```

This is a safety fix, not just a performance change.

### 5 · `dump.ino` — outputs word computed once

`Dump::outputs()` called `Process::fetch_output(i)` six times in a loop; each call recomputed `data_bits[41] * 256 + data_bits[40]`. The word is now read once before the loop and the bit test is inlined.

```cpp
// before
for (int i = 1; i <= 6; i++)
  Ausgang[i] = Process::fetch_output(i);

// after
int word = Process::data_bits[41] * 256 + Process::data_bits[40];
for (int i = 1; i <= 6; i++)
  Ausgang[i] = !!(word & (1 << (i - 1)));
```

### 6 · `process.h` — removed dead `timestamp_t` / `timestamp`

The `timestamp_t` struct and its `timestamp` variable were declared in `process.h` but never read or written anywhere in the codebase. Removed entirely — frees ~7 bytes of RAM and eliminates a confusing dead declaration.

### 7 · `UVR2MQTT.ino` — `yield()` instead of `delay(10)` in `loop()`

`loop()` ended with `delay(10)` "to prevent excessive CPU usage". On the ESP8266, `delay()` internally calls `yield()` plus a busy-wait. Using `yield()` directly services the WiFi/TCP stack without the artificial 10 ms stall on every iteration. The `delay(10)` inside `startSetupAP()` is intentional and was left untouched.

### 8 · `process.ino` — `prepare()` undefined behaviour when no sync found

If both `analyze()` calls return `-1` (no sync pattern — noisy or absent DL bus), `trim()` was called with `start_bit = -1`, feeding `read_bit(-1)` → negative bit shift → undefined behaviour. Added early `return false` guard.

### 9 · `receive.h/ino` — `ICACHE_RAM_ATTR` on `stop()` and `process_bit()`

Both are called from the ISR `pin_changed()` but were not marked `ICACHE_RAM_ATTR`. If the compiler didn't inline them, they would live in flash and crash the ESP8266 during concurrent flash operations (e.g. EEPROM commit from web config save).

### 10 · `UVR2MQTT.ino` — removed dead `stopAllProcessing()`

Defined but never called anywhere. Removed to free flash.

### 11 · `receive.h/ino` — removed dead `Receive::receiving` flag

After the `frame_complete` flag was added (commit `a7d973a`), nothing reads `receiving`. Removed the declaration and both writes (in `start()` and `stop()`), eliminating a volatile write from the ISR path.

### 12 · `UVR2MQTT.ino` — removed redundant `yield()` after `delay(50)` in setup

`delay()` on ESP8266 already calls `yield()` internally. The explicit `yield()` was a no-op.

### 13 · `receive.ino` — cached volatile read in `process_bit()`

`pulse_count` is volatile. The `BIT_COUNT` macro `(pulse_count / 2)` caused 5 separate volatile memory loads per bit-storing ISR call (the `% 2` check, two expansions for row/col, and one for the frame-complete check). Since interrupts don't nest on the ESP8266, the value can't change between reads. Caching `++pulse_count` into a local `int count` reduces it to 1 volatile read. Also replaced `% 2` with `& 1`. The ISR fires ~976 times/second.

### 14 · `process.ino/h` — removed dead `fetch_output()`

After optimisation #5 inlined the output word read into `Dump::outputs()`, `Process::fetch_output()` was no longer called. Removed the definition and declaration.

---

## Architecture snapshot

```
DL Bus (GPIO2 / D4)
  └─ Receive::pin_changed()  [ISR, ICACHE_RAM_ATTR]
       Manchester decode → Process::data_bits[]
       Sets Receive::frame_complete flag when done

loop()
  ├─ server.handleClient()       web config UI
  ├─ manageConnections()         WiFi + MQTT reconnect w/ exponential backoff
  ├─ mqtt_client->loop()         MQTT keepalive
  ├─ if (frame_complete)         atomic flag clear, then:
  │    Process::start()          sync detect → invert? → trim → check header
  │    Dump::start()             extract SensorValue[1-6], Ausgang[1-6]
  │    Receive::start()          arm ISR for next frame
  └─ every 60 s (MQTT up)
       mqtt_daten_senden()       publish changed values only
       every 15 min: force_all   republish everything

MQTT topics  (prefix configurable, default "heating/UVR610K/")
  {prefix}Sensor1 … Sensor6     float string, e.g. "23.40"
  {prefix}Ausgang1 … Ausgang6   "1" or "0"
```

## What has not been changed

- DL bus ISR logic, pulse timing constants, Manchester decoding
- `volatile` qualifiers on `frame_complete`, `pulse_count`, `last_bit_change`, `receiving`
- `noInterrupts()` / `interrupts()` atomic section in `loop()`
- Frame buffer sizing (`bit_number`, `data_bits[]` dimensions)
- 1-based sensor/output indexing (slots 1–6)
- MQTT state cache (`lastPublishedSensorValue`, `lastPublishedAusgang`)
- `delay(10)` inside `startSetupAP()` (intentional, setup-only loop)

---

## Possible future work (not yet investigated)

- `fetch_sensor()` and `fetch_output()` in `process.ino` are now called only from `Dump` — the `Process::sensor` global could be made local if that coupling is ever cleaned up
- `additionalBits` is hard-coded to `0` in `UVR2MQTT.ino`; its purpose should be confirmed or the constant removed
- MQTT publishing is on a fixed 60-second timer regardless of how recently data changed — could be tightened to publish immediately after each frame if desired
