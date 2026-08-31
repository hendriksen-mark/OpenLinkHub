#pragma once

#include <Arduino.h>

constexpr float NTC_NOMINAL_RESISTANCE = 10000.0;
constexpr float NTC_NOMINAL_TEMPERATURE_C = 25.0;
constexpr float NTC_BETA = 3950.0;
constexpr float SERIES_RESISTOR = 10000.0;
constexpr uint16_t ADC_MAXIMUM = 4095;
constexpr unsigned long REPORT_INTERVAL_MS = 2000;
constexpr size_t MAX_SENSORS = 8;
constexpr char AP_SSID[] = "Temperature Monitor";
constexpr char AP_PASSWORD[] = "configureme";
