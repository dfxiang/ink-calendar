#include "weather.h"

#include <Arduino.h>
#include <HTTPClient.h>

#include "config.h"
#include "settings.h"

namespace {

// Pulls the JSON number that follows `"key":` in `body`.
bool jsonInt(const String &body, const char *key, int *out) {
  String needle = String("\"") + key + "\":";
  int pos = -1;
  while (true) {
    pos = body.indexOf(needle, pos + 1);
    if (pos < 0) {
      return false;
    }
    int i = pos + needle.length();
    while (i < static_cast<int>(body.length()) && (body[i] == ' ' || body[i] == '\t')) {
      ++i;
    }
    bool negative = false;
    if (i < static_cast<int>(body.length()) && body[i] == '-') {
      negative = true;
      ++i;
    }
    int64_t value = 0;
    bool any = false;
    while (i < static_cast<int>(body.length()) && body[i] >= '0' && body[i] <= '9') {
      value = value * 10 + (body[i] - '0');
      any = true;
      ++i;
    }
    if (!any) {
      continue;  // units entry like "\"temperature_2m\": \"°C\"" - keep looking
    }
    if (negative) {
      value = -value;
    }
    *out = static_cast<int>(value);
    return true;
  }
}

}  // namespace

const char *weatherText(int wmoCode) {
  switch (wmoCode) {
    case 0:
    case 1:
      return "晴";
    case 2:
      return "多云";
    case 3:
      return "阴";
    case 45:
    case 48:
      return "雾";
    case 51:
    case 53:
    case 55:
    case 61:
      return "小雨";
    case 63:
    case 80:
    case 81:
      return "阵雨";
    case 65:
      return "大雨";
    case 56:
    case 57:
    case 66:
    case 67:
      return "冻雨";
    case 71:
    case 85:
      return "小雪";
    case 73:
      return "中雪";
    case 75:
    case 77:
    case 86:
      return "大雪";
    case 82:
      return "强阵雨";
    case 95:
    case 96:
    case 99:
      return "雷阵雨";
    default:
      return "晴";
  }
}

int weatherChangeKey(const WeatherInfo &w) {
  if (!w.valid) {
    return -1;
  }
  return ((w.code & 0xFF) << 24) | ((w.temperature & 0xFF) << 16) |
         ((w.tempMax & 0xFF) << 8) | (w.tempMin & 0xFF);
}

bool fetchWeather(WeatherInfo *out) {
  if (out == nullptr) {
    return false;
  }
  out->valid = false;

  HTTPClient http;
  String url = "http://api.open-meteo.com/v1/forecast?latitude=";
  url += String(settingsLatitude(), 4);
  url += "&longitude=";
  url += String(settingsLongitude(), 4);
  url += "&current=temperature_2m,relative_humidity_2m,weather_code";
  url += "&daily=temperature_2m_max,temperature_2m_min";
  url += "&timezone=Asia%2FShanghai&forecast_days=1";

  http.setConnectTimeout(4000);
  http.setTimeout(6000);
  http.setReuse(false);

  if (!http.begin(url)) {
    Serial.println(F("[weather] begin failed"));
    return false;
  }
  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    Serial.printf("[weather] HTTP %d\n", status);
    http.end();
    return false;
  }
  const String body = http.getString();
  http.end();

  WeatherInfo w;
  if (!jsonInt(body, "temperature_2m", &w.temperature) ||
      !jsonInt(body, "relative_humidity_2m", &w.humidity) ||
      !jsonInt(body, "weather_code", &w.code)) {
    Serial.println(F("[weather] parse failed"));
    return false;
  }
  // Note: jsonInt stops at the first non-digit; "36.7" parses as 36.
  if (!jsonInt(body, "temperature_2m_max", &w.tempMax)) {
    w.tempMax = w.temperature;
  }
  if (!jsonInt(body, "temperature_2m_min", &w.tempMin)) {
    w.tempMin = w.temperature;
  }
  w.valid = true;
  *out = w;
  Serial.printf("[weather] code=%d %dC hum=%d%% max=%d min=%d\n", w.code,
                w.temperature, w.humidity, w.tempMax, w.tempMin);
  return true;
}
