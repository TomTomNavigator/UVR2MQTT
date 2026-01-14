# UVR2MQTT

UVR2MQTT - read the DL Bus from Technische Alternative and publish to MQTT via an ESP8266. 
In my case a a UVR610, read more here: https://thomasheinz.net/enerboxx-uvr610-mittels-dl-bus-und-esp8266-auslesen-und-via-mqtt-daten-weitergeben/


--> Added a web config (configure via Wifi, 192.168.4.1) and also SSL/TLS MQTT option. 

I changed and adapted existing code to my use case, see resources for more. 

<img width="526" height="722" alt="image" src="https://github.com/user-attachments/assets/e50ed897-d3c9-4d3f-968a-3449aa54661c" />

## Installation
Just compile it and install it to your ESP8266. I am using Pin **GPIO2 = D4**.
Compiled *.bin ready to download in the Release section. 

Install with https://esphome.github.io/esp-web-tools/ (for example) 


When the ESP8266 starts it will create a Setup Access Point - "UVR2MQTT-Setup" - where you can configure your settings. 

## Resources: 
 * https://github.com/Buster01/UVR2MQTT
 * https://github.com/stoffelll/UVR2MQTT
 * https://www.mikrocontroller.net/topic/134862
 * https://github.com/martinkropf/UVR31_RF24
