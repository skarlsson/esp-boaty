#include "WiFiHelper.h"

WiFiHelper::WiFiHelper(WiFiClient* espClient, const char* ssid, const char* wifi_password) :
_espClient(espClient),
_serial_log(true),
_connected(false),
_diconnection_counter(0)
{
  strcpy(_ssid, ssid);
  strcpy(_wifi_password, wifi_password);
}

bool WiFiHelper::init()
{
  delay(10);
  // We start by connecting to a WiFi network

  if (_serial_log) {
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(_ssid);
  }
  config_WiFi();
}

bool WiFiHelper::loop()
{
  return connected();
}

bool WiFiHelper::connected()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    if (_connected)
    {
      _connected = false;
      _diconnection_counter++;
    }
    return false;
  }
  _connected = true;
  return true;
}

void WiFiHelper::config_WiFi()
{
  WiFi.enableSTA(true);
  WiFi.mode(WIFI_STA);
  WiFi.setSleepMode(WIFI_NONE_SLEEP);
  //WiFi.setOutputPower(0);
  WiFi.begin(_ssid, _wifi_password);
  
  if (_serial_log)
  {
    Serial.print("ssid:");
    Serial.print(_ssid);
    Serial.print(", wifi_password:");
    Serial.println(_wifi_password); 
  }
}