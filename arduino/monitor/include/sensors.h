#pragma once

#include <Arduino.h>
#include "config.h"

enum SensorType : uint8_t
{
  SENSOR_DISABLED = 0,
  SENSOR_TEMPERATURE = 1,
  SENSOR_CURRENT = 2,
};

struct Sensor
{
  char name[25];
  uint8_t pin;
  SensorType type;
  bool enabled;
  // Only the member pair matching `type` is meaningful; the pairs share storage.
  union
  {
    struct
    {
      float ntcResistance;
      float ntcBeta;
    };
    struct
    {
      float offsetVoltage;
      float sensitivity;
    };
  };
};

Sensor makeTemperatureSensor(const char *name, uint8_t pin, bool enabled);
Sensor makeCurrentSensor(const char *name, uint8_t pin, bool enabled, float offsetVoltage, float sensitivity);

extern Sensor sensors[MAX_SENSORS];
extern float seriesResistor;

bool isAdcPin(uint8_t pin);
void loadSensors();
void saveSensors();
float readTemperatureC(const Sensor &sensor);
float readCurrentA(const Sensor &sensor);
void calibrateCurrentSensor(Sensor &sensor);
