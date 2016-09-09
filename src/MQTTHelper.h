#pragma once
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <functional>

class MQTTHelper
{
  public:
  enum connection_state_t { DISCONNECTED, MQTT_OK, NR_CONNECTION_STATES };
  enum error_types_t      { MQTT_CONNECTION_LOST_ERROR, MQTT_TX_ERROR, NR_ERROR_TYPES };
  
  using on_connect_callback =  std::function<void(PubSubClient*)>;

  
  MQTTHelper(PubSubClient* pubSubClient, const char* rootName, const char* mqtt_server);
  bool init();
  bool loop();
  void report_state();
  //bool connect();
  bool connected();
  
  void set_on_connect_callback(on_connect_callback f) { _on_connected = f; }
	   
  private:
  const char*        _mqtt_device_name;
  connection_state_t _conn_state;
  int                _error_counter[NR_ERROR_TYPES];
   
  bool               _serial_log;
  PubSubClient*      _pubSubClient;
  char               _mqtt_server[32];
  char               _mqtt_out_buffer[1024];
  long               _lastMQTTReconnectAttempt;
  long               _lastStateReportedAt;
  connection_state_t  _lastStateReported;
  on_connect_callback _on_connected;
};