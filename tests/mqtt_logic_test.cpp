#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "Arduino.h"
#include "../config.h"

namespace {
  struct PublishedMessage {
    std::string topic;
    std::string payload;
    bool retained;
  };
}

class PubSubClient {
 public:
  bool connect_result = true;
  bool connected_state = false;
  bool disconnected = false;
  int fail_on_attempt = -1;
  int publish_attempts = 0;
  std::vector<PublishedMessage> messages;

  boolean connect(const char*, const char*, const char*) {
    connected_state = connect_result;
    disconnected = false;
    return connect_result;
  }

  boolean publish(const char* topic, const char* payload, boolean retained) {
    publish_attempts++;
    if (publish_attempts == fail_on_attempt)
      return false;
    messages.push_back({topic, payload, retained});
    return true;
  }

  void disconnect() {
    connected_state = false;
    disconnected = true;
  }
};

struct EspStub {
  unsigned int getChipId() const { return 0x123456; }
} ESP;

std::size_t strlcpy(char* destination, const char* source, std::size_t destination_size) {
  const std::size_t source_length = std::strlen(source);
  if (destination_size > 0) {
    const std::size_t copy_length = source_length < destination_size - 1
                                      ? source_length
                                      : destination_size - 1;
    std::memcpy(destination, source, copy_length);
    destination[copy_length] = '\0';
  }
  return source_length;
}

PubSubClient client;
PubSubClient* mqtt_client = &client;
char SensorValue[7][10] = {{0}};
bool Ausgang[7] = {false};
Config config{};

#include "../MQTT.h"

namespace {
  void require(bool condition, const char* message) {
    if (!condition) {
      std::cerr << "FAIL: " << message << '\n';
      std::exit(1);
    }
  }

  void setState() {
    std::strcpy(config.mqtt_topic, "heating/UVR610K/");
    for (int i = 1; i <= 6; i++) {
      std::snprintf(SensorValue[i], sizeof(SensorValue[i]), "%d.00", 20 + i);
      Ausgang[i] = (i & 1) != 0;
    }
  }

  void testInitialAndChangedPublishing() {
    setState();
    require(mqtt_connect(), "MQTT connection should succeed");
    require(mqtt_daten_senden(), "initial MQTT state publish should succeed");
    require(client.messages.size() == 12, "initial publish should send all 12 state topics");
    for (const auto& message : client.messages)
      require(message.retained, "all state messages should be retained");

    require(mqtt_daten_senden(), "unchanged MQTT state check should succeed");
    require(client.messages.size() == 12, "unchanged state should not be republished");

    std::strcpy(SensorValue[1], "25.50");
    require(mqtt_daten_senden(), "changed MQTT state publish should succeed");
    require(client.messages.size() == 13, "only the changed state should be published");
    require(client.messages.back().topic == "heating/UVR610K/Sensor1",
            "changed sensor should use the existing topic layout");
  }

  void testFailureForcesReconnectAndFullRepublish() {
    client.fail_on_attempt = client.publish_attempts + 1;
    require(!mqtt_daten_senden(true), "publish failure should be reported");
    require(client.disconnected, "publish failure should disconnect MQTT");

    client.fail_on_attempt = -1;
    require(mqtt_connect(), "MQTT reconnect should succeed");
    const std::size_t message_count = client.messages.size();
    require(mqtt_daten_senden(), "state publish after reconnect should succeed");
    require(client.messages.size() == message_count + 12,
            "reconnect should reset the cache and republish all state");
  }
}

int main() {
  testInitialAndChangedPublishing();
  testFailureForcesReconnectAndFullRepublish();
  std::cout << "All MQTT logic tests passed.\n";
  return 0;
}
