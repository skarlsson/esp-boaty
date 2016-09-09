#include "mqtt_bmp180.h"
#include <ArduinoJson.h>

#define SENSOR_SAMPLE_INTERVALL 100 
#define FORECAST_SAMPLE_INTERVALL 60000

static const char *weather[] = {
  "stable","stable_sunny","stable_rainy","unstable","thunderstorm","unknown"};

weather_forecast::weather_forecast() :
_minuteCount(0),
_forecast(UNKNOWN)
{
  _slotTimer.reset(FORECAST_SAMPLE_INTERVALL);
}

const char* to_string(weather_forecast::forecast_t t)
{
  return weather[t];
}  

// we save last measurement per minute even if we get several
void weather_forecast::add_measurement(double pressure)
{
   if (_slotTimer.elapsed())
   {
     _slotTimer.reset(FORECAST_SAMPLE_INTERVALL);
     _minuteCount++;
   }
   
  //From 0 to 5 min.
  if (_minuteCount <= 5){
    _pressureSamples[0][_minuteCount] = pressure;
  }
  //From 30 to 35 min.
  else if ((_minuteCount >= 30) && (_minuteCount <= 35)){
    _pressureSamples[1][_minuteCount - 30] = pressure;  
  }
  //From 60 to 65 min.
  else if ((_minuteCount >= 60) && (_minuteCount <= 65)){
    _pressureSamples[2][_minuteCount - 60] = pressure;  
  }  
  //From 90 to 95 min.
  else if ((_minuteCount >= 90) && (_minuteCount <= 95)){
    _pressureSamples[3][_minuteCount - 90] = pressure;  
  }
  //From 120 to 125 min.
  else if ((_minuteCount >= 120) && (_minuteCount <= 125)){
    _pressureSamples[4][_minuteCount - 120] = pressure;  
  }
  //From 150 to 155 min.
  else if ((_minuteCount >= 150) && (_minuteCount <= 155)){
    _pressureSamples[5][_minuteCount - 150] = pressure;  
  }
  //From 180 to 185 min.
  else if ((_minuteCount >= 180) && (_minuteCount <= 185)){
    _pressureSamples[6][_minuteCount - 180] = pressure;  
  }
  //From 210 to 215 min.
  else if ((_minuteCount >= 210) && (_minuteCount <= 215)){
    _pressureSamples[7][_minuteCount - 210] = pressure;  
  }
  //From 240 to 245 min.
  else if ((_minuteCount >= 240) && (_minuteCount <= 245)){
    _pressureSamples[8][_minuteCount - 240] = pressure;  
  }


  if (_minuteCount == 5) {
    // Avg pressure in first 5 min, value averaged from 0 to 5 min.
    _pressureAvg[0] = ((_pressureSamples[0][0] + _pressureSamples[0][1] 
      + _pressureSamples[0][2] + _pressureSamples[0][3]
      + _pressureSamples[0][4] + _pressureSamples[0][5]) / 6);
  } 
  else if (_minuteCount == 35) {
    // Avg pressure in 30 min, value averaged from 0 to 5 min.
    _pressureAvg[1] = ((_pressureSamples[1][0] + _pressureSamples[1][1] 
      + _pressureSamples[1][2] + _pressureSamples[1][3]
      + _pressureSamples[1][4] + _pressureSamples[1][5]) / 6);
    float change = (_pressureAvg[1] - _pressureAvg[0]);
    _dP_dt = change / 5; 
  } 
  else if (_minuteCount == 65) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    _pressureAvg[2] = ((_pressureSamples[2][0] + _pressureSamples[2][1] 
      + _pressureSamples[2][2] + _pressureSamples[2][3]
      + _pressureSamples[2][4] + _pressureSamples[2][5]) / 6);
    float change = (_pressureAvg[2] - _pressureAvg[0]);
    _dP_dt = change / 10; 
  } 
  else if (_minuteCount == 95) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    _pressureAvg[3] = ((_pressureSamples[3][0] + _pressureSamples[3][1] 
      + _pressureSamples[3][2] + _pressureSamples[3][3]
      + _pressureSamples[3][4] + _pressureSamples[3][5]) / 6);
    float change = (_pressureAvg[3] - _pressureAvg[0]);
    _dP_dt = change / 15; 
  } 
  else if (_minuteCount == 125) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    _pressureAvg[4] = ((_pressureSamples[4][0] + _pressureSamples[4][1] 
      + _pressureSamples[4][2] + _pressureSamples[4][3]
      + _pressureSamples[4][4] + _pressureSamples[4][5]) / 6);
    float change = (_pressureAvg[4] - _pressureAvg[0]);
    _dP_dt = change / 20; 
  } 
  else if (_minuteCount == 155) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    _pressureAvg[5] = ((_pressureSamples[5][0] + _pressureSamples[5][1] 
      + _pressureSamples[5][2] + _pressureSamples[5][3]
      + _pressureSamples[5][4] + _pressureSamples[5][5]) / 6);
    float change = (_pressureAvg[5] - _pressureAvg[0]);
    _dP_dt = change / 25; 
  } 
  else if (_minuteCount == 185) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    _pressureAvg[6] = ((_pressureSamples[6][0] + _pressureSamples[6][1] 
      + _pressureSamples[6][2] + _pressureSamples[6][3]
      + _pressureSamples[6][4] + _pressureSamples[6][5]) / 6);
    float change = (_pressureAvg[6] - _pressureAvg[0]);
    _dP_dt = change / 30; 
  }
  else if (_minuteCount == 215) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    _pressureAvg[7] = ((_pressureSamples[7][0] + _pressureSamples[7][1] 
      + _pressureSamples[7][2] + _pressureSamples[7][3]
      + _pressureSamples[7][4] + _pressureSamples[7][5]) / 6);
    float change = (_pressureAvg[7] - _pressureAvg[0]);
    _dP_dt = change / 35; 
  } 
  else if (_minuteCount == 245) {
    // Avg pressure at end of the hour, value averaged from 0 to 5 min.
    _pressureAvg[8] = ((_pressureSamples[8][0] + _pressureSamples[8][1] 
      + _pressureSamples[8][2] + _pressureSamples[8][3]
      + _pressureSamples[8][4] + _pressureSamples[8][5]) / 6);
    float change = (_pressureAvg[8] - _pressureAvg[0]);
    _dP_dt = change / 40; // note this is for t = 4 hour
        
    _minuteCount -= 30;
    _pressureAvg[0] = _pressureAvg[1];
    _pressureAvg[1] = _pressureAvg[2];
    _pressureAvg[2] = _pressureAvg[3];
    _pressureAvg[3] = _pressureAvg[4];
    _pressureAvg[4] = _pressureAvg[5];
    _pressureAvg[5] = _pressureAvg[6];
    _pressureAvg[6] = _pressureAvg[7];
    _pressureAvg[7] = _pressureAvg[8];
  } 

  if (_minuteCount < 36) //if time is less than 35 min 
    _forecast = UNKNOWN; // more time needed
  else if (_dP_dt < (-0.25))
    _forecast = THUNDERSTORM; // Quickly falling LP, Thunderstorm, not stable
  else if (_dP_dt > 0.25)
    _forecast = UNSTABLE; // Quickly rising HP, not stable weather
  else if ((_dP_dt > (-0.25)) && (_dP_dt < (-0.05)))
    _forecast =  STABLE_RAINY; // Slowly falling Low Pressure System, stable rainy weather
  else if ((_dP_dt > 0.05) && (_dP_dt < 0.25))
    _forecast =  STABLE_SUNNY; // Slowly rising HP stable good weather
  else if ((_dP_dt > (-0.05)) && (_dP_dt < 0.05))
    _forecast =  STABLE; // Stable weather
  else
    _forecast = UNKNOWN;
}  
  
mqtt_bmp180::mqtt_bmp180(PubSubClient* pubSubClient, const char* sensor_name):
_pubSubClient(pubSubClient),
_lastTemp(-100),
_altitude(0),
_state(IDLE)
{
    //SETUP MQTT TOPICS
  strcpy(_mqtt_output_topic, "devices/");
  strcat(_mqtt_output_topic, sensor_name);
  strcat(_mqtt_output_topic, "/state");
}

bool mqtt_bmp180::init()
{
	return _bmp180.begin();
}

void mqtt_bmp180::loop()
{
  // start a new series
  if (_state == IDLE && _nextStateTimer.elapsed())
  {
    char status = _bmp180.startTemperature();
    if (status != 0)
    {
      // Wait for the measurement to complete:
    _nextStateTimer.reset(status);     
    _state = WAIT_FOR_TEMP;
    } else {
      Serial.println("error starting temperature measurement");
      _nextStateTimer.reset(SENSOR_SAMPLE_INTERVALL); 
    }
  }
    
  // Retrieve the completed temperature me
  if (_state == WAIT_FOR_TEMP && _nextStateTimer.elapsed())
  {
    char status = _bmp180.getTemperature(_lastTemp);
    if (status != 0)
    {
      // Start a pressure measurement:
      // The parameter is the oversampling setting, from 0 to 3 (highest res, longest wait).
      // If request is successful, the number of ms to wait is returned.
      // If request is unsuccessful, 0 is returned.

      status = _bmp180.startPressure(3);
      if (status != 0) {
        _nextStateTimer.reset(status);     
        _state = WAIT_FOR_PRESSURE;
      } else {
         Serial.println("error starting pressure measurement");
        _state == IDLE ;
        _nextStateTimer.reset(SENSOR_SAMPLE_INTERVALL); 
      }
  } else {
    Serial.println("error retrieving temperature measurement");
    _state == IDLE ;
     _nextStateTimer.reset(SENSOR_SAMPLE_INTERVALL); 
  }
  }
  
  if (_state == WAIT_FOR_PRESSURE && _nextStateTimer.elapsed())
  {
      double P;
      char status = _bmp180.getPressure(P,_lastTemp);
      if (status != 0)
      {
          // The pressure sensor returns abolute pressure, which varies with altitude.
          // To remove the effects of altitude, use the sealevel function and your current altitude.
          // This number is commonly used in weather reports.
          // Parameters: P = absolute pressure in mb, ALTITUDE = current altitude in m.
          // Result: p0 = sea-level compensated pressure in mb

          double p0 = _bmp180.sealevel(P,_altitude); 
		  
          
          Serial.print("temperature: ");
          Serial.print(_lastTemp,2);
          Serial.print(" deg C, ");
	  
              Serial.print("relative (sea-level) pressure: ");
          Serial.print(p0,2);
          Serial.println(" mb, ");
          
          _forecast.add_measurement(p0);
          
          DynamicJsonBuffer jsonBuffer(JSON_OBJECT_SIZE(34));
          JsonObject& root = jsonBuffer.createObject();
          root["ts"] = millis();
          root["p"] = double_with_n_digits(p0, 6);
          root["t"] = _lastTemp; // should not be here since it's probably temp internally at sensor - not room or outside
          root["forecast"] = to_string(_forecast.get());
          
          //root.prettyPrintTo(Serial);
          
          int sz = root.printTo(_mqtt_out_buffer, sizeof(_mqtt_out_buffer));
          
          if (_pubSubClient->publish(_mqtt_output_topic, (const uint8_t*) _mqtt_out_buffer, sz, false)==false)
          {
             Serial.println("error publising to mqtt");
          }
          
          _state == IDLE ;
          _nextStateTimer.reset(SENSOR_SAMPLE_INTERVALL); // lets do this often for testing

        } else { 
          Serial.println("error retrieving pressure measurement");
          _state == IDLE ;
          _nextStateTimer.reset(SENSOR_SAMPLE_INTERVALL); 
        }
  }
}

