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

    void printSensorSummary() {
      if (!includeSensorSummary) {
        return;
      }

      Serial.print(F("[capture] sensors="));
      for (int i = 1; i <= 6; i++) {
        if (i > 1) {
          Serial.print(F(", "));
        }
        Serial.print(i);
        Serial.print('=');
        if (SensorValue[i][0] == '\0') {
          Serial.print('-');
        } else {
          Serial.print(SensorValue[i]);
        }
      }
      Serial.print(F(" outputs="));
      for (int i = 1; i <= 6; i++) {
        if (i > 1) {
          Serial.print(',');
        }
        Serial.print(Ausgang[i] ? '1' : '0');
      }
      Serial.println();
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
    printSensorSummary();
  }
}
