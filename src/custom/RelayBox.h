#pragma once
#include <functional>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <csi/timer.h>

std::string device_address_to_string(const uint8_t*); 
bool device_address_from_string(const std::string& s, uint8_t*);


struct temp_data
{
  temp_data() : value(127.0), valid(false) {}
  DeviceAddress address;
  std::string   name;
  float         value;
  float         reported_value;
  bool          valid;
};

struct output_port_handler
{
  using transform =  std::function<bool (bool)>;

  output_port_handler(const char* logic_name, int port, bool initial_state=false);
  void attach(transform f) { _transform = f; }
  void init();
  void loop();
  
  const char*     _logic_name;
  int             _port;
  bool            _in_state;
  csi::timer      _off_timer;
  bool            _out_state;
  bool            _reported_state;
  transform       _transform;
};

class RelayBox
{
  public:
  enum connection_state_t { DISCONNECTED, WIFI_OK, MQTT_OK, NR_CONNECTION_STATES };
  enum error_types_t      { WIFI_DISCONNECTED, MQTT_CONNECTION_LOST_ERROR, MQTT_TX_ERROR, NR_ERROR_TYPES };
  enum control_type_t     { DIRECT, TIMED_ON };
  enum { TEMP_RESOLUTION_BITS  = 12 };
  enum { NR_OF_OUTPUT_PORTS  = 4 };
  using on_thermometers =  std::function<void (temp_data*, size_t terms)>;

  
  
  RelayBox(PubSubClient* mqtt_client, const char* rootName, const char** deviceNames, bool serial_logging = false);
  void init();
  bool map_temp_sensors(const char* mapping[][2], size_t items);
  //void setup(const char* ssid, const char* wifi_password, const char* mqtt_server);
  void on_connect();
  void on_publish(const MQTT::Publish& msg);
  void loop();
  void set_connection_state(connection_state_t s);
  void set_callback(on_thermometers f) {_on_thermometers = f; };
  void set_output_transform(const char* id, std::function<bool (bool)>);
  private:
  void mqtt_log(const char* s);

  private:
  const char*        _rootName;

  //state
  connection_state_t _conn_state2;
  long               _lastBlinkAt;
  bool 		           _blinkState;
  int                _blink_timers[NR_CONNECTION_STATES];
  int                _error_counter[NR_ERROR_TYPES];
  bool               _serial_log;
  // hardware
  OneWire            _oneWire;
  DallasTemperature  _DS18B20;

  output_port_handler* _outputs[NR_OF_OUTPUT_PORTS];
    
  int             _nr_of_temp_sensors;
  unsigned long   _next_temp_ready_at;
  char            _tempString[6];
  
  temp_data       _temp_data[16];
  int             _temp_delay_in_ms;
  
  PubSubClient*   _mqtt_client;
  char            _mqtt_out_buffer[1024];
  char            _command_topic[64];
  char            _output_topic[64];
  char            _log_topic[64];
  long            _last_mqtt_publish;
  
  on_thermometers _on_thermometers;
};