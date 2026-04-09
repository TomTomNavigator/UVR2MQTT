#ifndef CAPTURE_H
#define CAPTURE_H

namespace Capture {
  constexpr bool enabled = true;
  constexpr bool printOnlyValidFrames = false;
  constexpr bool includeSensorSummary = true;

  void begin();
  void logFrame(bool validFrame, int startBit);
}

#endif
