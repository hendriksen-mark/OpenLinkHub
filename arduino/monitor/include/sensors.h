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
  float offsetVoltage;
  float sensitivity;
  bool enabled;
};

extern Sensor sensors[MAX_SENSORS];

bool isAdcPin(uint8_t pin);
void loadSensors();
void saveSensors();
float readTemperatureC(uint8_t pin);
float readCurrentA(const Sensor &sensor);
