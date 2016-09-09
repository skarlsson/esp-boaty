#include <MQTTHelper.h>

static const char* to_string(MQTTHelper::connection_state_t s)
{ 
  switch (s)
  {
    case MQTTHelper::MQTT_OK:  return "CONNECTED";
  }
  return "DISCONNECTED";
}

MQTTHelper::MQTTHelper(PubSubClient* pubSubClient, const char* rootName, const char* mqtt_server):
_serial_log(true),
_mqtt_device_name(rootName),
_pubSubClient(pubSubClient),
_conn_state(DISCONNECTED),
_lastStateReportedAt(0),
_lastStateReported(MQTT_OK),
_lastMQTTReconnectAttempt(0)
{
  strcpy(_mqtt_server, mqtt_server);   
  
   for (int i = 0; i != NR_ERROR_TYPES; ++i)
    _error_counter[i] = 0;
}

bool MQTTHelper::init(){
  _pubSubClient->set_server(_mqtt_server, 1883);
  return true;
}
/*
bool MQTTHelper::connect() {
  if (_pubSubClient->connect(_mqtt_device_name)) {
    Serial.println("MQTT connect OK");
    return true;
  }
  Serial.println("MQTT connect failed");
  return false;
}
*/

bool MQTTHelper::connected() {
  return _pubSubClient->connected();
}

bool MQTTHelper::loop()
{
  long now = millis();
 
  // if we do not have wifi then we're not connected...
  if (WiFi.status() != WL_CONNECTED) {
    if (_conn_state != DISCONNECTED)
      _error_counter[MQTT_CONNECTION_LOST_ERROR]++;
    _conn_state = DISCONNECTED;
    _pubSubClient->disconnect();
    return false;
  }

   // everthing good?
  if (_pubSubClient->loop()) {
    if (_conn_state < MQTT_OK)  // can this ever happen???? if so we need to call callback here aswell...
	    _conn_state = MQTT_OK; 
    return true;
  }

  // not connected anymore
  if (_conn_state >= MQTT_OK)
  {
    _error_counter[MQTT_CONNECTION_LOST_ERROR]++;
    _conn_state = DISCONNECTED;
  }
    
  if (now - _lastMQTTReconnectAttempt > 5000) {
    _lastMQTTReconnectAttempt = now;
    // Attempt to reconnect
    if (_pubSubClient->connect(_mqtt_device_name)) {
      Serial.println("MQTT connect OK");
      _lastMQTTReconnectAttempt = 0;
      if (_on_connected)
        _on_connected(_pubSubClient);
    } else {
      return false;
    }
  }
    
  return _pubSubClient->loop();
}

void MQTTHelper::report_state()
{ 
  long now = millis();
  if (_lastStateReported != _conn_state)
  {
    _lastStateReported = _conn_state;
    _lastStateReportedAt = now;
    Serial.print("esp_mqtt::state:");
    Serial.println(to_string(_conn_state));
  }
  
  if (now - _lastStateReportedAt > 5000)
  {
    _lastStateReportedAt = now;
    Serial.print("esp_mqtt::state:");
    Serial.println(to_string(_conn_state));
   }
}

