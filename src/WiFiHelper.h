#pragma once
#include <ESP8266WiFi.h>

class WiFiHelper
{ 
public:
  WiFiHelper(WiFiClient* espClient, const char* ssid, const char* wifi_password);
  bool init();
  bool loop();
  bool connected();
private:
  void config_WiFi();  
  bool               _connected;
  size_t             _diconnection_counter;
  bool               _serial_log;
  char               _ssid[32];
  char               _wifi_password[32];
  WiFiClient*        _espClient;
};