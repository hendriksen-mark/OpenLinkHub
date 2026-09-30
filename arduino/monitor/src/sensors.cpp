#include "sensors.h"

#include <Preferences.h>
#include <math.h>

Sensor sensors[MAX_SENSORS] = {
    {"Coolant", 1, SENSOR_TEMPERATURE, 0, 0, true},
    {"12V Rail", 2, SENSOR_CURRENT, 1.65, 0.185, true},
    {"Ambient", 3, SENSOR_TEMPERATURE, 0, 0, false},
};

Preferences preferences;

bool isAdcPin(uint8_t pin)
{
  return pin >= 1 && pin <= MAX_SENSORS;
}

void loadSensors()
{
  preferences.begin("sensors", true);
  if (preferences.isKey("configured"))
  {
    for (size_t index = 0; index < MAX_SENSORS; ++index)
    {
      const String key = String("s") + index;
      const String saved = preferences.getString(key.c_str(), "");
      sensors[index].enabled = false;
      const int first = saved.indexOf('|');
      const int second = saved.indexOf('|', first + 1);
      const int third = saved.indexOf('|', second + 1);
      const int fourth = saved.indexOf('|', third + 1);
      if (first < 1 || second < 0 || third < 0 || fourth < 0)
      {
        continue;
      }
      saved.substring(0, first).toCharArray(sensors[index].name, sizeof(sensors[index].name));
      sensors[index].type = static_cast<SensorType>(saved.substring(first + 1, second).toInt());
      sensors[index].pin = static_cast<uint8_t>(saved.substring(second + 1, third).toInt());
      sensors[index].offsetVoltage = saved.substring(third + 1, fourth).toFloat();
      sensors[index].sensitivity = saved.substring(fourth + 1).toFloat();
      sensors[index].enabled = sensors[index].type != SENSOR_DISABLED && isAdcPin(sensors[index].pin) &&
                               (sensors[index].type == SENSOR_TEMPERATURE || sensors[index].sensitivity > 0);
    }
  }
  preferences.end();
}

void saveSensors()
{
  preferences.begin("sensors", false);
  for (size_t index = 0; index < MAX_SENSORS; ++index)
  {
    const String key = String("s") + index;
    const String value = String(sensors[index].name) + "|" + sensors[index].type + "|" + sensors[index].pin +
                         "|" + String(sensors[index].offsetVoltage, 4) + "|" + String(sensors[index].sensitivity, 4);
    preferences.putString(key.c_str(), value);
  }
  preferences.putBool("configured", true);
  preferences.end();
}

float readTemperatureC(uint8_t pin)
{
  const uint16_t raw = analogRead(pin);
  if (raw == 0 || raw >= ADC_MAXIMUM)
  {
    return NAN;
  }
  const float resistance = SERIES_RESISTOR * raw / (ADC_MAXIMUM - raw);
  const float steinhart = log(resistance / NTC_NOMINAL_RESISTANCE) / NTC_BETA +
                          1.0 / (NTC_NOMINAL_TEMPERATURE_C + 273.15);
  return 1.0 / steinhart - 273.15;
}

float readCurrentA(const Sensor &sensor)
{
  uint32_t total = 0;
  for (uint8_t sample = 0; sample < 16; ++sample)
  {
    total += analogRead(sensor.pin);
  }
  const float voltage = (static_cast<float>(total) / 16.0) * ADC_REFERENCE_VOLTAGE / ADC_MAXIMUM;
  const float current = (voltage - sensor.offsetVoltage) / sensor.sensitivity;
  return current > 0 ? current : 0;
}
