#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"
#include "sensors.h"
#include "web_ui.h"

unsigned long lastReport = 0;

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

  JsonDocument doc;
  JsonArray temperatures = doc["temperatures"].to<JsonArray>();
  JsonArray currents = doc["currents"].to<JsonArray>();

  for (size_t index = 0; index < MAX_SENSORS; ++index)
  {
    const Sensor &sensor = sensors[index];
    if (!sensor.enabled) continue;

    if (sensor.type == SENSOR_TEMPERATURE)
    {
      const float value = readTemperatureC(sensor);
      if (isnan(value) || isinf(value)) continue;
      JsonObject item = temperatures.add<JsonObject>();
      item["name"] = sensor.name;
      item["value"] = roundf(value * 100.0f) / 100.0f;
    }
    else if (sensor.type == SENSOR_CURRENT)
    {
      const float value = readCurrentA(sensor);
      if (isnan(value) || isinf(value)) continue;
      JsonObject item = currents.add<JsonObject>();
      item["name"] = sensor.name;
      item["value"] = roundf(value * 1000.0f) / 1000.0f;
    }
  }

  serializeJson(doc, Serial);
  Serial.println();
}
