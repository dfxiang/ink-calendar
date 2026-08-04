#pragma once

#include <stdint.h>

struct WeatherInfo {
  int temperature;  // current, whole degrees Celsius
  int humidity;     // relative humidity, %
  int tempMax;      // today's forecast max, whole degrees
  int tempMin;      // today's forecast min, whole degrees
  int code;         // WMO weather code
  bool valid;
};

// Fetches current + today's forecast from Open-Meteo (blocking, up to a few
// seconds). Returns true and fills `out` on success.
bool fetchWeather(WeatherInfo *out);

// Short Chinese description for a WMO weather code, e.g. "多云".
const char *weatherText(int wmoCode);

// Stable integer key describing the displayed weather, used to detect
// changes worth refreshing. -1 means "no data".
int weatherChangeKey(const WeatherInfo &w);
