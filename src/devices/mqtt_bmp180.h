#pragma once
#include <SFE_BMP180.h>
#include <PubSubClient.h>
#include <csi/timer.h>

/*
	uses D1 & D2 (I2C bus)
*/

class weather_forecast
{
  public:
  enum forecast_t {  STABLE, STABLE_SUNNY, STABLE_RAINY, UNSTABLE, THUNDERSTORM, UNKNOWN };
  
  weather_forecast();
  void add_measurement(double pressure);
  forecast_t get() const { return _forecast;}
  private:
  csi::timer _slotTimer;
  int        _minuteCount;
  double     _pressureSamples[9][6];
	double     _pressureAvg[9];
  double     _dP_dt;
  forecast_t _forecast;
};

const char* to_string(weather_forecast::forecast_t);


class mqtt_bmp180
{
public:
	enum state_t { IDLE, WAIT_FOR_TEMP, WAIT_FOR_PRESSURE };
  mqtt_bmp180(PubSubClient* pubSubClient, const char* sensor_name);
	bool init();
	void set_altitude(int val);
	void loop();
	
private:
  PubSubClient*    _pubSubClient;
  state_t          _state;
	int              _altitude;
	double           _lastTemp;
  csi::timer       _nextStateTimer; 
	SFE_BMP180       _bmp180;
  char             _mqtt_output_topic[64];
  char             _mqtt_out_buffer[128];
  weather_forecast _forecast;
};
