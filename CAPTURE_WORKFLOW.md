# Capture-first workflow

This branch is intentionally optimized for serial DL-Bus capture, not MQTT/web usage.

## What changed
- WiFi removed from runtime path
- MQTT removed from runtime path
- Web/AP config removed from runtime path
- Firmware starts serial logging immediately and listens on the DL bus

## Serial output
- Baud rate: `115200`
- Output includes:
  - frame number
  - validity flag
  - detected `start_bit`
  - 64 processed frame bytes in hex
  - simple decoded sensor/output summary

## Intended session workflow
1. Build firmware on the machine connected to the ESP.
2. Flash firmware to the ESP8266.
3. Open serial monitor at `115200` baud.
4. Observe idle traffic.
5. Change the real RAS+DL / Fernversteller state:
   - offset up/down
   - mode / slider switch
   - reconnect / restart if useful
6. Save serial logs for comparison.

## Build/flash examples
These depend on what is installed on the target machine.

### Arduino CLI
```bash
arduino-cli compile --fqbn esp8266:esp8266:d1_mini .
arduino-cli upload -p /dev/ttyUSB0 --fqbn esp8266:esp8266:d1_mini .
arduino-cli monitor -p /dev/ttyUSB0 -c baudrate=115200
```

### Alternative serial tools
```bash
screen /dev/ttyUSB0 115200
# or
picocom -b 115200 /dev/ttyUSB0
# or
minicom -D /dev/ttyUSB0 -b 115200
```

## Notes
- This is still only a first-pass capture build.
- It logs processed frames, not full oscilloscope-grade physical timing.
- If needed, next step is deeper raw timing capture / edge-duration logging.
