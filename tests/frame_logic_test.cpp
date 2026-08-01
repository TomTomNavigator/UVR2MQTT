#include <array>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>

#include "Arduino.h"

const byte dataPin = 2;
const byte interrupt = 2;
const int additionalBits = 0;

namespace Dump {
  int start_calls = 0;
  void start();
}

#include "../process.h"
#include "../receive.h"
#include "../process.ino"
#include "../receive.ino"

namespace Dump {
  void start() {
    start_calls++;
  }
}

namespace {
  void require(bool condition, const char* message) {
    if (!condition) {
      std::cerr << "FAIL: " << message << '\n';
      std::exit(1);
    }
  }

  void clearState() {
    std::memset(Process::data_bits, 0, sizeof(Process::data_bits));
    Process::start_bit = -1;
    Dump::start_calls = 0;
  }

  void updateChecksum(std::array<byte, 64>& frame) {
    byte checksum = 0;
    for (int i = 0; i < 63; i++)
      checksum += frame[i];
    frame[63] = checksum;
  }

  std::array<byte, 64> makeFrame() {
    std::array<byte, 64> frame{};
    frame[0] = 0x80;
    frame[1] = 0x7f;
    updateChecksum(frame);
    return frame;
  }

  void encodeFrame(const std::array<byte, 64>& frame, bool inverted = false) {
    clearState();

    int bit_position = 0;
    for (int copy = 0; copy < 2; copy++) {
      for (int i = 0; i < 16; i++)
        Process::write_bit(bit_position++, 1);

      for (byte value : frame) {
        Process::write_bit(bit_position++, 0); // start bit
        for (int bit = 0; bit < 8; bit++)
          Process::write_bit(bit_position++, (value >> bit) & 0x01);
        Process::write_bit(bit_position++, 1); // stop bit
      }
    }

    if (inverted)
      Process::invert();
  }

  void testValidFrame() {
    encodeFrame(makeFrame());
    require(Process::start(), "valid frame should be accepted");
    require(Dump::start_calls == 1, "valid frame should update decoded state once");
  }

  void testInvertedFrame() {
    encodeFrame(makeFrame(), true);
    require(Process::start(), "inverted valid frame should be accepted");
  }

  void testChecksumRejection() {
    auto frame = makeFrame();
    frame[10] = 1;
    encodeFrame(frame);
    require(!Process::start(), "bad checksum should be rejected");
    require(Dump::start_calls == 0, "bad frame must not update decoded state");
  }

  void testIncompleteFrameRejection() {
    clearState();
    for (int i = Process::bit_number - 16; i < Process::bit_number; i++)
      Process::write_bit(i, 1);
    require(!Process::start(), "sync without a complete frame should be rejected");
  }

  void testNegativeRoomSensor() {
    auto frame = makeFrame();
    const int raw_room_value = 0x1f6; // -1.0 in a signed 9-bit field
    const int raw_sensor = 0x8000 | (ROOM << 12) | (NORMAL << 9) | raw_room_value;
    frame[8] = raw_sensor & 0xff;
    frame[9] = (raw_sensor >> 8) & 0xff;
    updateChecksum(frame);

    encodeFrame(frame);
    require(Process::start(), "frame containing a negative room value should be valid");
    Process::fetch_sensor(1);
    require(!Process::sensor.invalid, "negative room sensor should remain valid");
    require(Process::sensor.mode == NORMAL, "room mode should be decoded");
    require(std::fabs(Process::sensor.value - (-1.0f)) < 0.001f,
            "negative room value should be sign-extended from nine bits");
  }

  void testReceiveBufferBoundaries() {
    clearState();
    Receive::start();

    for (int bit = 0; bit < Process::bit_number; bit++) {
      const byte value = (bit == 0 || bit == Process::bit_number - 1) ? 1 : 0;
      Receive::process_bit(0);     // ignored first Manchester half-pulse
      Receive::process_bit(value); // stored decoded bit
    }

    require(Receive::frame_complete == 1, "receiver should complete after exactly bit_number bits");
    require(Process::read_bit(0) == 1, "receiver should write the first bit at index zero");
    require(Process::read_bit(Process::bit_number - 1) == 1,
            "receiver should write the final bit inside the buffer");
  }
}

int main() {
  testValidFrame();
  testInvertedFrame();
  testChecksumRejection();
  testIncompleteFrameRejection();
  testNegativeRoomSensor();
  testReceiveBufferBoundaries();
  std::cout << "All frame logic tests passed.\n";
  return 0;
}
