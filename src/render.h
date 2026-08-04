#pragma once

#include "frame.h"
#include "weather.h"

// Draws the full calendar page into `frame`:
//   - header: Gregorian date + weekday, weather (top), lunar date (second
//     line left), humidity / high / low (second line right)
//   - month grid: Sunday first, Mon-Fri dates in red, Sat/Sun in black,
//     today framed by a red box (optionally yellow background), every cell
//     carries its lunar date / festival name.
// `weekday` is tm_wday (0 = Sunday).
void renderCalendar(FrameBuffer &frame, int year, int month, int day, int weekday,
                    const WeatherInfo &weather);

// Full-screen setup-mode page shown while the config portal is active.
void renderConfigScreen(FrameBuffer &frame, const char *apSsid, const char *apIp);

// Full-screen page shown while waiting for Wi-Fi / NTP before first render.
void renderConnectingScreen(FrameBuffer &frame, const char *ssid);
