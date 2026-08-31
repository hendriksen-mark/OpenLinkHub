#pragma once

#include <Arduino.h>
#include "config.h"

struct NtcSensor
{
  char name[25];
  uint8_t pin;
  bool enabled;
};

extern NtcSensor sensors[MAX_SENSORS];

bool isAdcPin(uint8_t pin);
void loadSensors();
void saveSensors();
float readTemperatureC(uint8_t pin);
