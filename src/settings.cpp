#include "settings.h"

#include <Preferences.h>
#include <string.h>

#include "config.h"

namespace {

Preferences prefs;
constexpr char kNamespace[] = "cfg";
constexpr char kKeySsid[] = "ssid";
constexpr char kKeyPass[] = "pass";
constexpr char kKeyLat[] = "lat";
constexpr char kKeyLon[] = "lon";

}  // namespace

bool loadSettings(DeviceSettings *out) {
  if (out == nullptr) {
    return false;
  }
  memset(out, 0, sizeof(*out));

  prefs.begin(kNamespace, true);
  const String ssid = prefs.getString(kKeySsid, "");
  const String password = prefs.getString(kKeyPass, "");
  out->hasLatitude = prefs.isKey(kKeyLat);
  out->hasLongitude = prefs.isKey(kKeyLon);
  if (out->hasLatitude) {
    out->latitude = prefs.getFloat(kKeyLat, WEATHER_LATITUDE);
  }
  if (out->hasLongitude) {
    out->longitude = prefs.getFloat(kKeyLon, WEATHER_LONGITUDE);
  }
  prefs.end();

  strlcpy(out->ssid, ssid.c_str(), sizeof(out->ssid));
  strlcpy(out->password, password.c_str(), sizeof(out->password));
  return out->hasWifi() || out->hasLatitude || out->hasLongitude;
}

void saveSettings(const DeviceSettings &settings) {
  prefs.begin(kNamespace, false);
  prefs.putString(kKeySsid, settings.ssid);
  prefs.putString(kKeyPass, settings.password);
  if (settings.hasLatitude) {
    prefs.putFloat(kKeyLat, settings.latitude);
  }
  if (settings.hasLongitude) {
    prefs.putFloat(kKeyLon, settings.longitude);
  }
  prefs.end();
}

void clearSettings() {
  prefs.begin(kNamespace, false);
  prefs.clear();
  prefs.end();
}

float settingsLatitude() {
  DeviceSettings s;
  loadSettings(&s);
  return s.hasLatitude ? s.latitude : WEATHER_LATITUDE;
}

float settingsLongitude() {
  DeviceSettings s;
  loadSettings(&s);
  return s.hasLongitude ? s.longitude : WEATHER_LONGITUDE;
}
