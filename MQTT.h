#include <WiFiClientSecure.h>
#include "config.h"

extern PubSubClient* mqtt_client;
extern char SensorValue[][10];
extern bool Ausgang[];
extern Config config; // Access to config including mqtt_topic

boolean mqtt_connect(void) {
  // The new `manageConnections` function in the main .ino file ensures WiFi is
  // connected before this is called. We just need to attempt the connection.
  // The connect function is blocking, but will timeout after 15 seconds (as set in setup).
  char clientId[20];
  snprintf(clientId, sizeof(clientId), "UVR2MQTT-%06X", ESP.getChipId());
  return mqtt_client->connect(clientId, config.mqtt_user, config.mqtt_pass);
}

void mqtt_daten_senden() {
  // The main loop now ensures this is only called when the client is connected.
  char topic[160];
  bool publish_failed = false;

  for (int sv = 1; sv <= 6; sv++) {
    snprintf(topic, sizeof(topic), "%sSensor%d", config.mqtt_topic, sv);
    if (!mqtt_client->publish(topic, SensorValue[sv])) {
      publish_failed = true;
      break;
    }
  }

  if (!publish_failed) {
    for (int sv = 1; sv <= 6; sv++) {
      snprintf(topic, sizeof(topic), "%sAusgang%d", config.mqtt_topic, sv);
      if (!mqtt_client->publish(topic, Ausgang[sv] ? "1" : "0")) {
        publish_failed = true;
        break;
      }
    }
  }

  if (publish_failed) {
    mqtt_client->disconnect();
  }
}
