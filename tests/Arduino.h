#ifndef UVR2MQTT_TEST_ARDUINO_H
#define UVR2MQTT_TEST_ARDUINO_H

#include <cstdint>

using byte = std::uint8_t;
using boolean = bool;

#define ICACHE_RAM_ATTR
#define IRAM_ATTR
#define CHANGE 1

inline void delay(unsigned long) {}
inline void attachInterrupt(byte, void (*)(), int) {}
inline void detachInterrupt(byte) {}
inline int digitalRead(byte) { return 0; }
inline unsigned long micros() {
  static unsigned long now = 0;
  now += 1024;
  return now;
}

#endif
