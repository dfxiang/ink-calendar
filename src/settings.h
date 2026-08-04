#pragma once

// Persisted device configuration (NVS). WiFi credentials and the weather
// location are stored here so they can be changed from the phone without
// reflashing the firmware.

struct DeviceSettings {
  char ssid[33];        // "" when not configured
  char password[65];
  bool hasLatitude;
  bool hasLongitude;
  float latitude;
  float longitude;

  bool hasWifi() const { return ssid[0] != '\0'; }
};

// Reads the saved settings. Returns true when anything was saved before.
bool loadSettings(DeviceSettings *out);

void saveSettings(const DeviceSettings &settings);
void clearSettings();

// Effective weather coordinates: saved value if present, otherwise the
// compile-time defaults from config.h.
float settingsLatitude();
float settingsLongitude();
