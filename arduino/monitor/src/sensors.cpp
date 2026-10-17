#include "sensors.h"

#include <Preferences.h>
#include <math.h>

Sensor makeTemperatureSensor(const char *name, uint8_t pin, bool enabled)
{
  Sensor sensor = {};
  strlcpy(sensor.name, name, sizeof(sensor.name));
  sensor.pin = pin;
  sensor.type = SENSOR_TEMPERATURE;
  sensor.enabled = enabled;
  sensor.ntcResistance = NTC_NOMINAL_RESISTANCE;
  sensor.ntcBeta = NTC_BETA;
  return sensor;
}

Sensor makeCurrentSensor(const char *name, uint8_t pin, bool enabled, float offsetVoltage, float sensitivity)
{
  Sensor sensor = {};
  strlcpy(sensor.name, name, sizeof(sensor.name));
  sensor.pin = pin;
  sensor.type = SENSOR_CURRENT;
  sensor.enabled = enabled;
  sensor.offsetVoltage = offsetVoltage;
  sensor.sensitivity = sensitivity;
  return sensor;
}

Sensor sensors[MAX_SENSORS] = {
    makeTemperatureSensor("Coolant", 1, true),
    makeCurrentSensor("12V Rail", 2, true, 1.65, 0.185),
    makeTemperatureSensor("Ambient", 3, false),
};

float seriesResistor = SERIES_RESISTOR;

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
    seriesResistor = preferences.getFloat("series", SERIES_RESISTOR);
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
      // The last two fields mean R25/beta for temperature sensors and offset/sensitivity for current sensors.
      const float firstValue = saved.substring(third + 1, fourth).toFloat();
      const int fifth = saved.indexOf('|', fourth + 1);
      const float secondValue = saved.substring(fourth + 1, fifth < 0 ? saved.length() : fifth).toFloat();
      if (sensors[index].type == SENSOR_TEMPERATURE)
      {
        sensors[index].ntcResistance = firstValue > 0 ? firstValue : NTC_NOMINAL_RESISTANCE;
        sensors[index].ntcBeta = secondValue > 0 ? secondValue : NTC_BETA;
      }
      else if (sensors[index].type == SENSOR_CURRENT)
      {
        sensors[index].offsetVoltage = firstValue;
        sensors[index].sensitivity = secondValue;
      }
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
    const bool temperature = sensors[index].type == SENSOR_TEMPERATURE;
    const String value = String(sensors[index].name) + "|" + sensors[index].type + "|" + sensors[index].pin +
                         "|" + String(temperature ? sensors[index].ntcResistance : sensors[index].offsetVoltage, 4) +
                         "|" + String(temperature ? sensors[index].ntcBeta : sensors[index].sensitivity, 4);
    preferences.putString(key.c_str(), value);
  }
  preferences.putFloat("series", seriesResistor);
  preferences.putBool("configured", true);
  preferences.end();
}

float readTemperatureC(const Sensor &sensor)
{
  const uint16_t raw = analogRead(sensor.pin);
  if (raw == 0 || raw >= ADC_MAXIMUM)
  {
    return NAN;
  }
  const float resistance = seriesResistor * raw / (ADC_MAXIMUM - raw);
  const float steinhart = log(resistance / sensor.ntcResistance) / sensor.ntcBeta +
                          1.0 / (NTC_NOMINAL_TEMPERATURE_C + 273.15);
  return 1.0 / steinhart - 273.15;
}

static float readCurrentVoltage(const Sensor &sensor, uint8_t sampleCount)
{
  uint32_t total = 0;
  for (uint8_t sample = 0; sample < sampleCount; ++sample)
  {
    total += analogRead(sensor.pin);
  }
  return (static_cast<float>(total) / sampleCount) * ADC_REFERENCE_VOLTAGE / ADC_MAXIMUM;
}

float readCurrentA(const Sensor &sensor)
{
  const float voltage = readCurrentVoltage(sensor, 16);
  const float current = (voltage - sensor.offsetVoltage) / sensor.sensitivity;
  return current > 0 ? current : 0;
}

void calibrateCurrentSensor(Sensor &sensor)
{
  sensor.offsetVoltage = readCurrentVoltage(sensor, 64);
}
