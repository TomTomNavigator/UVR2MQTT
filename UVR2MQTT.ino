const byte dataPin = 2; 
const byte interrupt = 2;
unsigned long upload_interval = 60000;
const int additionalBits = 0;
unsigned long beginn = millis();
char SensorValue[7][10];
bool Ausgang[7];

#include <PubSubClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h> 
#include "receive.h"
#include "process.h"
#include "dump.h"
#include "MQTT.h"
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include "config.h"
#include <WiFiClient.h>

#define EEPROM_SIZE 512

Config config;
ESP8266WebServer server(80);

WiFiClient wifiClient;
WiFiClientSecure wifiClientSecure;

PubSubClient* mqtt_client = nullptr;

void saveConfig() {
  Config tempConfig;
  EEPROM.get(0, tempConfig);
  
  if (memcmp(&tempConfig, &config, sizeof(Config)) != 0) {
    EEPROM.put(0, config);
    EEPROM.commit();
  }
}

void setDefaultConfig() {
  memset(&config, 0, sizeof(Config));
  strcpy(config.magic, "UVR2MQTT");
  strcpy(config.ssid, "");
  strcpy(config.password, "");
  strcpy(config.mqtt_server, "abc123.s1.eu.hivemq.cloud");
  strcpy(config.mqtt_user, "user");
  strcpy(config.mqtt_pass, "passw0rd");
  strcpy(config.mqtt_topic, "heating/UVR610K/");
  config.mqtt_port = 8883;
  config.mqtt_tls = true;
}

void resetToDefaults() {
  setDefaultConfig();
  EEPROM.put(0, config);
  EEPROM.commit();
}

void stopAllProcessing() {
  // Disconnect MQTT client
  if (mqtt_client && mqtt_client->connected()) {
    mqtt_client->disconnect();
  }
  
  // Disable WiFi auto-reconnect
  WiFi.setAutoReconnect(false);
  
  // Give time for cleanup
  delay(100);
}

void loadConfig() {
  EEPROM.get(0, config);
  
  // If no valid config found, use defaults
  if (strcmp(config.magic, "UVR2MQTT") != 0) {
    setDefaultConfig();
    EEPROM.put(0, config);
    EEPROM.commit();
  }
}

void handleWebInterface() {
  // Redirect to config page since we don't have a homepage
  server.sendHeader("Location", "/config");
  server.send(302, "text/plain", "");
}

// Minimal HTML templates to save flash space
const char CONFIG_HTML[] PROGMEM = R"(<!DOCTYPE html>
<html>
<head>
    <title>UVR2MQTT Config</title>
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h2 { color: #333; margin-bottom: 20px; }
        label { display: block; margin-top: 15px; margin-bottom: 5px; font-weight: bold; color: #555; }
        input[type="text"], input[type="password"], input[type="number"] { 
            width: 100%; padding: 10px; margin-bottom: 10px; border: 1px solid #ddd; 
            border-radius: 4px; box-sizing: border-box; font-size: 14px; 
        }
        input[type="checkbox"] { margin-right: 8px; transform: scale(1.2); }
        .checkbox-label { display: flex; align-items: center; margin: 15px 0; }
        input[type="submit"] { 
            background: #007cba; color: white; border: none; padding: 12px 20px; 
            cursor: pointer; border-radius: 4px; font-size: 16px; margin-top: 20px;
            width: 100%; 
        }
        input[type="submit"]:hover { background: #005a87; }
        .danger { background: #d9534f; }
        .danger:hover { background: #c9302c; }
        .warning { background: #ff6b35; }
        .warning:hover { background: #e55a2b; }
        .form-group { margin-bottom: 15px; }
        .button-group { margin-top: 30px; display: flex; gap: 10px; }
        .button-group input { margin: 0; flex: 1; }
    </style>
</head>
<body>
    <div class="container">
        <h2>UVR2MQTT Configuration</h2>
        <form method="POST" action="/save">)";

const char SAVE_HTML[] PROGMEM = R"(<!DOCTYPE html>
<html>
<head>
    <title>Settings Saved</title>
    <meta name="viewport" content="width=device-width,initial-scale=1">
    <style>
        body { font-family: Arial, sans-serif; margin: 20px; background: #f5f5f5; text-align: center; }
        .container { max-width: 400px; margin: 50px auto; background: white; padding: 30px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        h2 { color: #28a745; }
    </style>
</head>
<body>
    <div class="container">
        <h2>Settings Saved!</h2>
        <p>Device is restarting with new configuration...</p>
    </div>
</body>
</html>)";

void handleConfig() {
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  
  String html = FPSTR(CONFIG_HTML);
  
  // Build form fields
  html += F("<div class='form-group'><label for='ssid'>WiFi SSID</label>");
  html += F("<input type='text' id='ssid' name='ssid' value='");
  html += String(config.ssid);
  html += F("' required></div>");
  
  html += F("<div class='form-group'><label for='password'>WiFi Password</label>");
  html += F("<input type='text' id='password' name='password' value='");
  html += String(config.password);
  html += F("'></div>");
  
  html += F("<div class='form-group'><label for='mqtt_server'>MQTT Server</label>");
  html += F("<input type='text' id='mqtt_server' name='mqtt_server' value='");
  html += String(config.mqtt_server);
  html += F("' required></div>");
  
  html += F("<div class='form-group'><label for='mqtt_port'>MQTT Port</label>");
  html += F("<input type='number' id='mqtt_port' name='mqtt_port' value='");
  html += String(config.mqtt_port);
  html += F("' min='1' max='65535' required></div>");
  
  html += F("<div class='form-group'><label for='mqtt_user'>MQTT Username</label>");
  html += F("<input type='text' id='mqtt_user' name='mqtt_user' value='");
  html += String(config.mqtt_user);
  html += F("'></div>");
  
  html += F("<div class='form-group'><label for='mqtt_pass'>MQTT Password</label>");
  html += F("<input type='text' id='mqtt_pass' name='mqtt_pass' value='");
  html += String(config.mqtt_pass);
  html += F("'></div>");
  
  html += F("<div class='form-group'><label for='mqtt_topic'>MQTT Topic Prefix</label>");
  html += F("<input type='text' id='mqtt_topic' name='mqtt_topic' value='");
  html += String(config.mqtt_topic);
  html += F("' required></div>");
  
  html += F("<div class='form-group'><div class='checkbox-label'>");
  html += F("<input type='checkbox' id='mqtt_tls' name='mqtt_tls' value='1'");
  if (config.mqtt_tls) html += F(" checked");
  html += F("><label for='mqtt_tls'>Use TLS/SSL Encryption</label></div></div>");
  
  html += F("<input type='submit' value='Save Configuration & Restart'>");
  html += F("</form>");
  
  html += F("<form method='POST' action='/reset' style='margin-top: 20px;'>");
  html += F("<input type='submit' value='Reset to Factory Defaults' class='danger' ");
  html += F("onclick='return confirm(\"This will reset all settings to defaults. Continue?\");'>");
  html += F("</form>");
  
  html += F("</div></body></html>");
  
  server.send(200, "text/html", html);
}

void handleRestart() {
  server.send(200, "text/html", 
    F("<!DOCTYPE html><html><head><title>Restart Device</title><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
      "<meta http-equiv=\"refresh\" content=\"3;url=/config\">"
      "<style>body{font-family:Arial;margin:20px;background:#f5f5f5;text-align:center;}"
      ".container{max-width:400px;margin:50px auto;background:white;padding:30px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}"
      "h2{color:#ff6b35;} .btn{background:#007cba;color:white;border:none;padding:10px 20px;border-radius:4px;cursor:pointer;text-decoration:none;display:inline-block;margin:10px;}"
      ".btn:hover{background:#005a87;} .danger{background:#d9534f;} .danger:hover{background:#c9302c;}</style></head><body><div class=\"container\">"
      "<h2>Restart Device</h2><p>Are you sure you want to restart the device?</p>"
      "<form method='POST' action='/restart' style='display:inline;'><input type='submit' value='Yes, Restart Now' class='btn danger'></form>"
      "<a href='/config' class='btn'>Cancel</a></div></body></html>"));
}

void handleRestartConfirm() {
  server.send(200, "text/html", 
    F("<!DOCTYPE html><html><head><title>Restarting</title><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
      "<meta http-equiv=\"refresh\" content=\"5;url=/config\">"
      "<style>body{font-family:Arial;margin:20px;background:#f5f5f5;text-align:center;}"
      ".container{max-width:400px;margin:50px auto;background:white;padding:30px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}"
      "h2{color:#ff6b35;}</style></head><body><div class=\"container\">"
      "<h2>Restarting Device...</h2><p>The device is restarting now.</p><p>Please wait a moment and refresh the page.</p>"
      "</div></body></html>"));
  delay(3000);
  ESP.restart();
}

void handleSave() {
  if (server.hasArg("ssid")) {
    strlcpy(config.ssid, server.arg("ssid").c_str(), sizeof(config.ssid));
  }
  if (server.hasArg("password")) {
    strlcpy(config.password, server.arg("password").c_str(), sizeof(config.password));
  }
  if (server.hasArg("mqtt_server")) {
    strlcpy(config.mqtt_server, server.arg("mqtt_server").c_str(), sizeof(config.mqtt_server));
  }
  if (server.hasArg("mqtt_user")) {
    strlcpy(config.mqtt_user, server.arg("mqtt_user").c_str(), sizeof(config.mqtt_user));
  }
  if (server.hasArg("mqtt_pass")) {
    strlcpy(config.mqtt_pass, server.arg("mqtt_pass").c_str(), sizeof(config.mqtt_pass));
  }
  if (server.hasArg("mqtt_port")) {
    int port = server.arg("mqtt_port").toInt();
    if (port > 0 && port <= 65535) config.mqtt_port = port;
  }
  if (server.hasArg("mqtt_topic")) {
    strlcpy(config.mqtt_topic, server.arg("mqtt_topic").c_str(), sizeof(config.mqtt_topic));
  }
  config.mqtt_tls = server.hasArg("mqtt_tls");
  
  saveConfig();
  server.send_P(200, "text/html", SAVE_HTML);
  delay(5000);
  ESP.restart();
}

void setupWebInterface() {
  server.on("/", handleWebInterface);
  server.on("/config", handleConfig);
  server.on("/save", HTTP_POST, handleSave);
  server.on("/restart", HTTP_GET, handleRestart);  // Add GET handler for restart page
  server.on("/restart", HTTP_POST, handleRestartConfirm);  // POST handler for actual restart
  server.on("/reset", HTTP_POST, []() {
    resetToDefaults();
    server.send(200, "text/html", 
      F("<!DOCTYPE html><html><head><title>Reset Complete</title><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
        "<style>body{font-family:Arial;margin:20px;background:#f5f5f5;text-align:center;}"
        ".container{max-width:400px;margin:50px auto;background:white;padding:30px;border-radius:8px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}"
        "h2{color:#d9534f;}</style></head><body><div class=\"container\">"
        "<h2>Reset to Factory Defaults</h2><p>All settings have been reset.</p><p>Device restarting in 3 seconds...</p>"
        "</div></body></html>"));
    delay(3000);
    ESP.restart();
  });
  server.begin();
}

void setupMQTTClient() {
  // Safer memory management - disconnect and set to nullptr instead of delete
  if (mqtt_client) {
    if (mqtt_client->connected()) {
      mqtt_client->disconnect();
    }
    // Don't delete, just reset to avoid destructor warning
    mqtt_client = nullptr;
  }
  
  if (config.mqtt_tls) {
    wifiClientSecure.setInsecure();
    mqtt_client = new PubSubClient(wifiClientSecure);
  } else {
    mqtt_client = new PubSubClient(wifiClient);
  }
  mqtt_client->setServer(config.mqtt_server, config.mqtt_port);
  mqtt_client->setKeepAlive(90); // Longer keepalive = fewer packets
  mqtt_client->setSocketTimeout(15); // Faster timeout for failed connections
}

void startSetupAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("UVR2MQTT-Setup");
  setupWebInterface();
  
  unsigned long setupStart = millis();
  const unsigned long setupTimeout = 600000; // 10 minutes in milliseconds
  
  while (millis() - setupStart < setupTimeout) {
    server.handleClient();
    delay(10);
  }
  
  ESP.restart();
}

void setup() {
  EEPROM.begin(EEPROM_SIZE);
  loadConfig();

  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  WiFi.setAutoReconnect(false);
  WiFi.persistent(false); // Reduce flash writes
  WiFi.setOutputPower(17); // Reduce WiFi output power (0-20.5dBm)
  
  // Check if WiFi credentials are configured (empty or whitespace-only)
  if (strlen(config.ssid) == 0 || strlen(config.password) == 0 || 
      strcmp(config.ssid, "") == 0 || strcmp(config.password, "") == 0) {
    startSetupAP(); 
  }
  
  WiFi.begin(config.ssid, config.password);
  unsigned long startAttempt = millis();
  
  while (WiFi.status() != WL_CONNECTED && millis() - startAttempt < 300000) {
    // Allow other tasks to run, like the web server if it's active (e.g., in AP mode)
    // or just yield to the OS.
    delay(100); // Small delay to prevent watchdog timer reset
    yield();
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    startSetupAP(); // This now has a 10-minute timeout and will restart
    // Function will not return - device restarts after 10 minutes
  }

  setupMQTTClient();
  
  // Start web interface for configuration and monitoring
  setupWebInterface();

  // Remove redundant WiFi connection attempts - already connected above
  Receive::start();
}

// Reconnection logic
unsigned long lastWifiReconnectAttempt = 0;
unsigned long wifiReconnectInterval = 10000; // Start with 10 seconds
const unsigned long maxWifiReconnectInterval = 300000; // Max 5 minutes
unsigned long wifiDisconnectedSince = 0; // Track when WiFi first disconnected
const unsigned long wifiRestartTimeout = 43200000; // 12 hours in milliseconds

unsigned long lastMqttReconnectAttempt = 0;
unsigned long mqttReconnectInterval = 10000; // Start with 10 seconds
const unsigned long maxMqttReconnectInterval = 300000; // Max 5 minutes

void manageConnections() {
  // Manage WiFi connection
  if (WiFi.status() != WL_CONNECTED) {
    // Track when WiFi first disconnected
    if (wifiDisconnectedSince == 0) {
      wifiDisconnectedSince = millis();
    }
    
    // If WiFi has been disconnected for 12 hours, restart device
    if (millis() - wifiDisconnectedSince > wifiRestartTimeout) {
      ESP.restart();
    }
    
    // If WiFi is disconnected, ensure MQTT is also seen as disconnected
    if (mqtt_client && mqtt_client->connected()) {
      mqtt_client->disconnect();
    }
    
    if (millis() - lastWifiReconnectAttempt > wifiReconnectInterval) {
      lastWifiReconnectAttempt = millis();
      
      // Try to reconnect
      WiFi.begin(config.ssid, config.password);
      
      // Increase backoff interval for next time
      wifiReconnectInterval *= 2;
      if (wifiReconnectInterval > maxWifiReconnectInterval) {
        wifiReconnectInterval = maxWifiReconnectInterval;
      }
    }
  } else {
    // If WiFi is connected, reset the WiFi backoff interval and disconnect timer
    wifiReconnectInterval = 10000;
    wifiDisconnectedSince = 0; // Reset disconnect timer 

    // Manage MQTT connection
    if (mqtt_client && !mqtt_client->connected()) {
      if (millis() - lastMqttReconnectAttempt > mqttReconnectInterval) {
        lastMqttReconnectAttempt = millis();
        
        if (mqtt_connect()) {
          // If MQTT connects, reset the MQTT backoff interval
          mqttReconnectInterval = 10000;
        } else {
          // If MQTT fails to connect, increase the backoff interval
          mqttReconnectInterval *= 2;
          if (mqttReconnectInterval > maxMqttReconnectInterval) {
            mqttReconnectInterval = maxMqttReconnectInterval;
          }
        }
      }
    }
  }
}

void loop() {
  static unsigned long lastDataProcess = 0;
  
  server.handleClient();

  manageConnections();

  if (mqtt_client && mqtt_client->connected()) {
    mqtt_client->loop();
  }

  // Process data less frequently to save CPU
  if (!Receive::receiving && (millis() - lastDataProcess > 1000)) {
    Process::start();
    Receive::start();
    lastDataProcess = millis();
  }

  // Send data only when MQTT is connected
  if (millis() - beginn > upload_interval && mqtt_client && mqtt_client->connected()) {
    mqtt_daten_senden();
    beginn = millis();
  }
  
  // Small delay to prevent excessive CPU usage
  delay(10);
}
