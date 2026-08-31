#include "sensors.h"

#include <Preferences.h>
#include <math.h>

NtcSensor sensors[MAX_SENSORS] = {
    {"Coolant", 1, true},
    {"Ambient", 2, true},
    {"GPU Backplate", 3, true},
};

Preferences preferences;

bool isAdcPin(uint8_t pin)
{
  return pin >= 1 && pin <= 10;
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
      if (saved.length() == 0)
      {
        continue;
      }
      const int firstSeparator = saved.indexOf('|');
      const int secondSeparator = saved.indexOf('|', firstSeparator + 1);
      if (firstSeparator < 1 || secondSeparator < 0)
      {
        continue;
      }
      saved.substring(0, firstSeparator).toCharArray(sensors[index].name, sizeof(sensors[index].name));
      const int pin = saved.substring(firstSeparator + 1, secondSeparator).toInt();
      sensors[index].pin = static_cast<uint8_t>(pin);
      sensors[index].enabled = saved.substring(secondSeparator + 1).toInt() == 1 &&
                               isAdcPin(sensors[index].pin);
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
    const String value = String(sensors[index].name) + "|" + sensors[index].pin +
                         "|" + (sensors[index].enabled ? "1" : "0");
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
