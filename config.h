#ifndef CONFIG_H
#define CONFIG_H

struct Config {
  char magic[9]; // "UVR2MQTT" + '\0'
  char ssid[32];
  char password[32];
  char mqtt_server[64];
  char mqtt_user[32];
  char mqtt_pass[32];
  char mqtt_topic[128];
  uint16_t mqtt_port;
  bool mqtt_tls;
};

extern Config config;

#endif
