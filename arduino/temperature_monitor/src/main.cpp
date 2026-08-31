#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "web_ui.h"

unsigned long lastReport = 0;

void printJsonString(const char *value)
{
  Serial.print('"');
  while (*value != '\0')
  {
    if (*value == '"' || *value == '\\')
    {
      Serial.print('\\');
    }
    Serial.print(*value++);
  }
  Serial.print('"');
}

void setup()
{
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  loadSensors();
  beginWebUi();
}

void loop()
{
  handleWebUi();
  if (millis() - lastReport < REPORT_INTERVAL_MS)
  {
    return;
  }
  lastReport = millis();

  Serial.print("{\"temperatures\":[");
  bool first = true;
  for (size_t index = 0; index < MAX_SENSORS; index++)
  {
    if (!sensors[index].enabled)
    {
      continue;
    }
    const float temperature = readTemperatureC(sensors[index].pin);
    if (isnan(temperature))
    {
      continue;
    }
    if (!first)
    {
      Serial.print(',');
    }
    first = false;
    Serial.print("{\"name\":");
    printJsonString(sensors[index].name);
    Serial.print(",\"value\":");
    Serial.print(temperature, 2);
    Serial.print('}');
  }
  Serial.println("]}");
}
