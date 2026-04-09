const byte dataPin = 2;
const byte interrupt = 2;
const int additionalBits = 0;
char SensorValue[7][10];
bool Ausgang[7];

#include "receive.h"
#include "process.h"
#include "dump.h"
#include "capture.h"

void setup() {
  Capture::begin();
  Receive::start();
}

void loop() {
  if (Receive::frame_complete) {
    noInterrupts();
    Receive::frame_complete = 0;
    interrupts();

    Process::start();
    Receive::start();
  }

  delay(1);
}
