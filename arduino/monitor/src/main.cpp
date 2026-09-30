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
    if (*value == '"' || *value == '\\') Serial.print('\\');
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
  if (millis() - lastReport < REPORT_INTERVAL_MS) return;
  lastReport = millis();

  Serial.print("{\"temperatures\":[");
  bool first = true;
  for (size_t index = 0; index < MAX_SENSORS; ++index)
  {
    if (!sensors[index].enabled || sensors[index].type != SENSOR_TEMPERATURE) continue;
    const float value = readTemperatureC(sensors[index].pin);
    if (isnan(value) || isinf(value)) continue;
    if (!first) Serial.print(',');
    first = false;
    Serial.print("{\"name\":");
    printJsonString(sensors[index].name);
    Serial.print(",\"value\":");
    Serial.print(value, 2);
    Serial.print('}');
  }

  Serial.print("],\"currents\":[");
  first = true;
  for (size_t index = 0; index < MAX_SENSORS; ++index)
  {
    if (!sensors[index].enabled || sensors[index].type != SENSOR_CURRENT) continue;
    const float value = readCurrentA(sensors[index]);
    if (isnan(value) || isinf(value)) continue;
    if (!first) Serial.print(',');
    first = false;
    Serial.print("{\"name\":");
    printJsonString(sensors[index].name);
    Serial.print(",\"value\":");
    Serial.print(value, 3);
    Serial.print('}');
  }
  Serial.println("]}");
}
