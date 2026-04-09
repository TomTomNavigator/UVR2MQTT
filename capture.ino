#include "capture.h"
#include "process.h"
#include <Arduino.h>

namespace Capture {

  namespace {
    void printHexByte(byte value) {
      if (value < 0x10) {
        Serial.print('0');
      }
      Serial.print(value, HEX);
    }
  }

  void begin() {
    if (!enabled) {
      return;
    }

    Serial.begin(115200);
    delay(50);
    Serial.println();
    Serial.println(F("[capture] serial capture enabled"));
    Serial.println(F("[capture] format: frame=<n> valid=<0|1> start_bit=<n> bytes=<64 hex bytes>"));
  }

  void logFrame(bool validFrame, int startBit) {
    if (!enabled) {
      return;
    }

    if (printOnlyValidFrames && !validFrame) {
      return;
    }

    static unsigned long frameNumber = 0;
    frameNumber++;

    Serial.print(F("[capture] frame="));
    Serial.print(frameNumber);
    Serial.print(F(" valid="));
    Serial.print(validFrame ? 1 : 0);
    Serial.print(F(" start_bit="));
    Serial.print(startBit);
    Serial.print(F(" bytes="));

    for (int i = 0; i < 64; i++) {
      printHexByte(Process::data_bits[i]);
      if (i < 63) {
        Serial.print(' ');
      }
    }
    Serial.println();
  }
}
