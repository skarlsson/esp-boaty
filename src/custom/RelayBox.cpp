#include <math.h>
#include "RelayBox.h"
#include <Arduino.h>
#include <ArduinoJson.h>


#define D0 16
#define D1 5 // I2C Bus SCL (clock)
#define D2 4 // I2C Bus SDA (data)
#define D3 0
#define D4 2 // Same as "LED_BUILTIN", but inverted logic
#define D5 14 // SPI Bus SCK (clock)
#define D6 12 // SPI Bus MISO 
#define D7 13 // SPI Bus MOSI
#define D8 15 // SPI Bus SS (CS)
#define D9 3 // RX0 (Serial console)
#define D10 1 // TX0 (Serial console)

#define ONE_WIRE_BUS D3

//static RelayBox* _this = NULL;

/*
example of async tremp
https://github.com/milesburton/Arduino-Temperature-Control-Library/blob/master/examples/WaitForConversion2/WaitForConversion2.pde
*/

/*
static void callbackX(char* topic, uint8_t* payload, unsigned int length) {
  _this->callback(topic, payload, length);
}
*/

/*
static void callbackX(const MQTT::Publish& msg) {
  _this->callback(msg);
}
*/

output_port_handler::output_port_handler(const char* logic_name, int port, bool initial_state) : 
_logic_name(logic_name), 
_port(port), 
_in_state(initial_state), 
_out_state(initial_state), 
_reported_state(false) {}
  

void output_port_handler::init()
{
  pinMode(_port, OUTPUT);
  loop(); // force initial value
}

void
output_port_handler::loop()
{
  if (_off_timer.elapsed())
  {
    _off_timer.clear();
    _in_state = false;
  }
  _out_state = _transform ? _transform(_in_state) : _in_state;
  digitalWrite(_port, _out_state ? HIGH : LOW); // true == HIGH false == LOW
}

RelayBox::RelayBox(PubSubClient* mqtt_client, const char* rootName, const char** deviceNames, bool serial_logging) :
  _rootName(rootName),
  _oneWire(ONE_WIRE_BUS),
  _DS18B20(&_oneWire),
  _mqtt_client(mqtt_client),
  _serial_log(serial_logging),
  _next_temp_ready_at(-1),
  _conn_state2(DISCONNECTED),
  _last_mqtt_publish(0),
  _nr_of_temp_sensors(0),
  _lastBlinkAt(0),
  _blinkState(false)
  {
   _temp_delay_in_ms = 750 / (1 << (12 - TEMP_RESOLUTION_BITS)); 

  _blink_timers[0] = 100;
  _blink_timers[1] = 500;
  _blink_timers[2] = 2000;

  _outputs[0] = new output_port_handler(deviceNames[0], D1, false);
  _outputs[1] = new output_port_handler(deviceNames[1], D2, false);
  _outputs[2] = new output_port_handler(deviceNames[2], D6, false);
  _outputs[3] = new output_port_handler(deviceNames[3], D7, false);

  for (int i = 0; i != NR_ERROR_TYPES; ++i)
    _error_counter[i] = 0;

   //SETUP MQTT TOPICS
  strcpy(_output_topic, "devices/");
  strcat(_output_topic, _rootName);
  strcat(_output_topic, "/state");

  strcpy(_command_topic, "devices/");
  strcat(_command_topic, _rootName);
  strcat(_command_topic, "/command");

  strcpy(_log_topic, "log/");
  strcat(_log_topic, _rootName);

  //_this = this; // there can only be one... 
}

void RelayBox::init()
{
   // setup OneWire bus
  Serial.println("\n");
  _DS18B20.begin();

  if (_serial_log) {
   // locate devices on the bus
    Serial.print("Locating devices...");
  }

  _nr_of_temp_sensors = _DS18B20.getDeviceCount();

  if (_serial_log) {
  Serial.println("Dallas Temperature Control Library");
  Serial.print("Version: ");
  Serial.println(DALLASTEMPLIBVERSION);
  Serial.print("Found ");
    Serial.print(_nr_of_temp_sensors, DEC);
    Serial.println(" devices.");
    // report parasite power requirements
    Serial.print("Parasite power is: ");
    if (_DS18B20.isParasitePowerMode())
      Serial.println("ON");
    else
      Serial.println("OFF");
  }
  
  // TBD - MUST STORE TEMP ADRESSES 

  for (int i=0; i!=_nr_of_temp_sensors; ++i)
  {
    _DS18B20.getAddress(_temp_data[i].address, i);
    _DS18B20.setResolution(_temp_data[i].address, TEMP_RESOLUTION_BITS);
    _temp_data[i].name = device_address_to_string(_temp_data[i].address); // unless we override it later...
  }
 
  for (int i=0; i!=_nr_of_temp_sensors; ++i)
  {
    Serial.println(_temp_data[i].name.c_str());
  }
  
  _DS18B20.setWaitForConversion(false);
  _DS18B20.requestTemperatures(); 
  _next_temp_ready_at = millis() + _temp_delay_in_ms;
  
  
  pinMode(BUILTIN_LED, OUTPUT);     // Initialize the BUILTIN_LED pin as an output
  
  // configure and write initial state
  for (int i = 0; i != NR_OF_OUTPUT_PORTS; ++i) {
    _outputs[i]->init();
  }
  
  _mqtt_client->set_callback([this](const MQTT::Publish& msg)
  {
    on_publish(msg);
  });  
}


bool RelayBox::map_temp_sensors(const char* mapping[][2], size_t items)
{
  bool all_found = true;
  for (size_t i=0; i!=items; ++i)
  {
    const char* address = mapping[i][0];
    const char* id = mapping[i][1];
    bool found = false;
    for (size_t j=0; j!=_nr_of_temp_sensors; ++j)
    {
      if (_temp_data[j].name == address)
      {
        _temp_data[j].name = id;
        found = true;
        break;
      }
    }
    all_found = all_found & found;
  }
    
  Serial.println("Remapping temp sensors...");
  for (int i=0; i!=_nr_of_temp_sensors; ++i)
  {
    
    Serial.print(device_address_to_string(_temp_data[i].address).c_str());
    Serial.print(" -> ");
    Serial.println(_temp_data[i].name.c_str());
  }
    
  Serial.println(all_found ? "OK" : "ERROR: SENSOR missing");  
    
  return all_found;
}

void RelayBox::set_connection_state(connection_state_t s) 
{ 
 // are we degrading?
 if (_conn_state2 < s) 
    _error_counter[s]++;
  _conn_state2 = s; 
}


void RelayBox::mqtt_log(const char* s) {
  strcpy(_mqtt_out_buffer, "INFO");
  strcat(_mqtt_out_buffer, ": ");
  strcat(_mqtt_out_buffer, s);
  _mqtt_client->publish(_log_topic, (const uint8_t*) _mqtt_out_buffer, strlen(_mqtt_out_buffer));
  _mqtt_client->loop();
}

void RelayBox::on_connect()
{
  Serial.print("subscribing on: ");
  Serial.println(_command_topic);
 _mqtt_client->subscribe(_command_topic);
}

//void RelayBox::callback(char* topic, byte* payload, unsigned int length) {
void RelayBox::on_publish(const MQTT::Publish& msg) {
   //Serial.print("on_publish ");
   //Serial.println(msg.payload_len());
   
  // sanity
  if (msg.payload_len() < 10) {
    //Serial.print("callback < 10");
    return;
  }

  DynamicJsonBuffer jsonBuffer(1024);
  JsonObject& root = jsonBuffer.parseObject((char*) msg.payload());
  //root.prettyPrintTo(Serial);
  const char* device = root["device"];
  int state = root["state"];
  int timer = root["timer"];
  
  for (int i = 0; i != NR_OF_OUTPUT_PORTS; ++i) {
    if (strcmp(device, _outputs[i]->_logic_name) == 0) {
	  if (timer>0)
	  {
      _outputs[i]->_off_timer.reset(timer * 1000);
      _outputs[i]->_in_state = true;
    }
	  else
	  {
      _outputs[i]->_off_timer.clear();
      _outputs[i]->_in_state = state ? true : false;
	  }
    _outputs[i]->loop(); // forces the output
	  break;
    }
  }
}

void RelayBox::set_output_transform(const char* device, std::function<bool (bool)> f)
{
  for (int i = 0; i != NR_OF_OUTPUT_PORTS; ++i) {
    if (strcmp(device, _outputs[i]->_logic_name) == 0) {
      _outputs[i]->_transform = f;
    }
  }
}

void  RelayBox::loop() {
  long now = millis();
  if (now - _lastBlinkAt > _blink_timers[_conn_state2]) {
    _lastBlinkAt = now;
    digitalWrite(BUILTIN_LED, _blinkState ? HIGH : LOW);
    _blinkState = !_blinkState;
  }

  //update inputs
   if (_next_temp_ready_at < now)
  {
     for (int i=0; i!= _nr_of_temp_sensors; ++i)
     {
      _temp_data[i].value = roundf(_DS18B20.getTempCByIndex(i)*10)/10; // round this to one decimal
      _temp_data[i].valid = _temp_data[i].value < 126.0 ? true : false;
     }
     _DS18B20.requestTemperatures();
     _next_temp_ready_at = millis() + _temp_delay_in_ms;
     
     if (_on_thermometers) 
      _on_thermometers(_temp_data, _nr_of_temp_sensors);
    }
  
  
  
 //check state on outputs
  // we leave reporting to loop
  for (int i = 0; i != NR_OF_OUTPUT_PORTS; ++i)
  {
   _outputs[i]->loop();
  } 
 
  if (_conn_state2!=MQTT_OK)
    return;
    
    // it might have taken long time to get here...
  now     = millis();
  
  bool updated = false;
 
  // dont publish to often...
  if (now - _last_mqtt_publish > 1000) {
      // report relay change
    for (int i = 0; i != NR_OF_OUTPUT_PORTS; ++i) {
      if (_outputs[i]->_out_state != _outputs[i]->_reported_state) {
        updated = true;
      }
    }
    
   for (int i=0; i!= _nr_of_temp_sensors; ++i)
   {
     if (fabs(_temp_data[i].value - _temp_data[i].reported_value)>0.5)
     {
        Serial.print(_temp_data[i].value);
        Serial.print("!=");
        Serial.println(_temp_data[i].reported_value);
        
        updated = true;
     }
   }     
  }

  if (now - _last_mqtt_publish > 10000) {
    updated = true;
  }

  if (updated) {
    DynamicJsonBuffer jsonBuffer(JSON_OBJECT_SIZE(34));
    JsonObject& root = jsonBuffer.createObject();
    root["ts"] = now;

	JsonArray& data1 = root.createNestedArray("outputs");
  for (int i = 0; i != 4; ++i)
	{
		JsonObject& obj = data1.createNestedObject();
		obj["id"] = _outputs[i]->_logic_name;
		obj["state"] = _outputs[i]->_out_state;
		if (_outputs[i]->_off_timer.enabled())
		{
			obj["timer"] = _outputs[i]->_off_timer.remaining()/1000;
		}
	}
  
  JsonArray& temp_sensors = root.createNestedArray("temp_sensors");
  for (int i = 0; i != _nr_of_temp_sensors; ++i)
      {
        JsonObject& obj = temp_sensors.createNestedObject();
        obj["id"] = _temp_data[i].name.c_str();
        obj["value"] = _temp_data[i].value;
      }
	
	JsonArray& ec = root.createNestedArray("ec");
	for (int i=0; i!=NR_ERROR_TYPES; ++i)
		ec.add(_error_counter[i]);

	if (_serial_log) {
		root.prettyPrintTo(Serial);
	}
	
  int sz = root.printTo(_mqtt_out_buffer, sizeof(_mqtt_out_buffer));
  
	if (_serial_log) {
		Serial.print("publishing: ");
		Serial.print(sz);
		Serial.println(" bytes");
	}
    
  if (_mqtt_client->publish(_output_topic, (const uint8_t*) _mqtt_out_buffer, sz)) {
      for (int i = 0; i != NR_OF_OUTPUT_PORTS; ++i)
        _outputs[i]->_reported_state = _outputs[i]->_out_state;
       for (int i=0; i!= _nr_of_temp_sensors; ++i)
        _temp_data[i].reported_value = _temp_data[i].value;
   } else {
    if (_serial_log) {
			Serial.println("failed to publish");
    }
    //only disconnect after certain time in error state ????      
    _error_counter[MQTT_TX_ERROR]++;
    _mqtt_client->disconnect();
    _mqtt_client->loop();
  }
  _last_mqtt_publish = now;
  }
}

std::string device_address_to_string(const uint8_t* addr)
{
  char buf[32];
  sprintf(buf, "%02X%02X%02X%02X%02X%02X%02X%02X", addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], addr[6], addr[7]);
  return buf;
}

/**
 * hex2byte
 * take a hex string and convert it to a 8bit number (2 hex digits)
 */
uint8_t hex2byte(const char *hex) {
    uint8_t val = 0;
    for (int i=0; i!=2; ++i)
    {
        // get current character then increment
        uint8_t byte = *hex++; 
        // transform hex character to the 4bit equivalent number, using the ascii table indexes
        if (byte >= '0' && byte <= '9') byte = byte - '0';
        else if (byte >= 'a' && byte <='f') byte = byte - 'a' + 10;
        else if (byte >= 'A' && byte <='F') byte = byte - 'A' + 10;    
        // shift 4 to make space for new digit, and add the 4 bits of the new digit 
        val = (val << 4) | (byte & 0xF);
    }
    return val;
}

bool device_address_from_string(const std::string& s, uint8_t* addr){
  if (s.size()!=16)
    return false;  
  for(int i=0; i!=8; ++i)
  {
    addr[i] = hex2byte(&s.data()[i*2]);
  }
  return true;
}

