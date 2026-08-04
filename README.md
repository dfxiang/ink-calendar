# ESP32-C3 Four-Color E-Paper Desk Calendar

This project turns a Good Display GDEM042F86 4.2-inch four-color
(black/white/red/yellow) e-paper panel into a desktop calendar:

- Top area: Gregorian date, weekday, weather, lunar calendar year, humidity,
  and high/low temperature
- Middle area: monthly calendar view starting on Sunday, with both Gregorian
  and lunar dates in each cell
- Weekdays from Monday to Friday are rendered in red; Saturday and Sunday are
  rendered in black
- Today is highlighted with a red border and light yellow background
- Gregorian and lunar holidays are displayed automatically, including Spring
  Festival, Dragon Boat Festival, Mid-Autumn Festival, National Day, Lunar New
  Year's Eve, and others
- No hour/minute/second display; the device checks date and weather changes
  every 60 seconds and refreshes the e-paper panel only when needed

## Hardware

Same as the original project: AirM2M CORE ESP32C3 + GDEM042F86 4.2-inch
400x300 four-color e-paper display, connected over SPI.

Wiring remains unchanged:

| E-Paper | ESP32-C3 GPIO |
| --- | ---: |
| `VCC` | `3V3` |
| `GND` | `GND` |
| `SCK` | `GPIO4` |
| `SDA` | `GPIO6` |
| `RST` | `GPIO2` |
| `DC` | `GPIO3` |
| `CS1` | `GPIO7` |
| `BUSY` | `GPIO10` |

## Configure WiFi from a Phone

The device receives WiFi settings through a configuration portal and stores
them in the ESP32 NVS flash. When you change routers later, you only need to
configure WiFi again; reflashing the firmware is not required.

There are three ways to enter configuration mode:

1. **First boot / no saved configuration**: the device enters configuration
   mode immediately after startup.
2. **WiFi connection failure**: if connection attempts fail for 60 seconds, the
   device enters configuration mode automatically.
3. **Double-click the RST button**: press RST twice quickly within 3 seconds
   after startup to clear the saved configuration and enter configuration mode.
   This is equivalent to a factory reset.

In configuration mode, the e-paper panel displays setup instructions. On your
phone:

1. Connect to the `InkScreen-Calendar` hotspot. No password is required.
2. Your phone browser should open the configuration page automatically. If it
   does not, open `http://192.168.4.1` manually.
3. Select or enter the WiFi SSID and password. You can also change the weather
   latitude and longitude. The default location is Shanghai.
4. Tap "Save and Restart". The device will connect to the new WiFi network and
   begin displaying the calendar.

The page also includes a "Clear Configuration and Restart" button, which has
the same effect as double-clicking RST.

Configuration is stored in NVS under the `cfg` namespace. Weather latitude and
longitude can also be changed from the phone page, so code changes are no longer
needed. `WEATHER_LATITUDE` and `WEATHER_LONGITUDE` in
[src/config.h](src/config.h) are only used as defaults when no saved
configuration exists.

> Note: configuration uses the ESP32 built-in WiFi module with SoftAP and a web
> page. Any phone can configure it without installing an app. BLE provisioning
> requires Espressif's provisioning app on the phone and can be added later if
> needed.

### Optional: External Button for Configuration Mode

Change `CONFIG_BUTTON_PIN` in [src/config.h](src/config.h) to any free GPIO
pin. The default value is `-1`, which disables this feature. Connect one side
of the button to that GPIO and the other side to GND. After startup, hold the
button for 3 seconds to enter configuration mode.

> Warning: do not hold the onboard BOOT button (`GPIO9`) through reset, or the
> chip will enter download mode. Holding it after the firmware is already
> running is fine.

## Build and Flash

```powershell
pio run
pio run --target upload
```

After flashing, connect to the serial port to view logs:

```powershell
pio device monitor -b 115200
```

Example log:

```text
ESP32-C3 calendar starting...
[portal] AP SSID: InkScreen-Calendar      <- first boot, waiting for phone setup
[portal] AP IP: 192.168.4.1
...(after saving configuration and restarting)...
[wifi] connecting to MyHomeWiFi
[weather] code=2 34C hum=44% max=36 min=27
[panel] refresh done
```

## Refresh Strategy

- Every 60 seconds, the device checks whether the date, lunar date, or weekday
  has changed. If so, it performs a full-screen refresh.
- Every 60 seconds, the device fetches weather data. If the weather description,
  temperature, humidity, or high/low temperature changes, it performs a fast
  refresh. The panel does not support true partial refresh; the driver's fast
  mode is used to reduce flicker.
- Weather-triggered refreshes have a minimum interval of 2 minutes. This can be
  adjusted with `WEATHER_MIN_REFRESH_SEC` in `config.h` to avoid frequent
  e-paper refreshes and reduce panel wear.
- After each refresh, the panel automatically enters sleep mode for low power
  consumption.

## Troubleshooting

- **Forgot WiFi password / changed router**: double-click RST quickly and
  configure WiFi again.
- **Cannot connect after configuration**: wait 60 seconds. The device will
  return to configuration mode automatically so you can check the password.
- **Screen stays on the configuration page**: the device is currently in
  configuration mode. Follow the WiFi setup steps above.
- **Home WiFi does not appear in the scan list**: ESP32-C3 only supports
  2.4GHz WiFi. If your router uses a separate 5GHz SSID, it will not appear.
  Enable band steering / combined SSID on the router, or use the 2.4GHz SSID.
- **Phone cannot stay connected to the configuration hotspot**: some phones
  automatically switch back to the previous WiFi network. Open system WiFi
  settings and manually select `InkScreen-Calendar`.

## Weather and Fonts

- Weather API: `api.open-meteo.com` over HTTP. Only city latitude and longitude
  are required.
- Chinese font data is generated by [tools/gen_font.py](tools/gen_font.py) from
  the Windows built-in SimHei font as 16x16 and 12x12 bitmap fonts. The
  generated data is committed in [src/font_data.h](src/font_data.h). To
  regenerate it, run `python tools/gen_font.py`.
- Lunar calendar conversion uses a lookup-table implementation covering
  1900-2100. It has been compared against an authoritative lunar calendar
  library across the full range, with 0 differences across 73,384 days. The
  implementation is in [src/lunar.cpp](src/lunar.cpp).

## Development Helpers

- Layout preview: `python tools/preview.py 2026-08-03` generates a calendar
  preview.
- Add the `config` argument to generate the configuration-mode instruction
  screen: `python tools/preview.py 2026-08-03 preview.png config`.
- Lunar calendar validation: `python tools/lunar_check.py`.

## File Structure

```text
src/config.h        User configuration: SSID, coordinates, refresh interval
src/main.cpp        Main loop: WiFi, NTP, polling, display refresh
src/settings.cpp    Configuration storage in NVS: WiFi and weather coordinates
src/portal.cpp      Configuration portal: SoftAP, web setup, WiFi scanning
src/portal_page.h   Configuration web page
src/render.cpp      Calendar layout rendering
src/frame.cpp       400x300 2-bit framebuffer and bitmap text rendering
src/font.h/data.h   16x16 / 12x12 Chinese bitmap fonts
src/lunar.cpp       Gregorian-to-lunar conversion and holidays
src/weather.cpp     Open-Meteo weather fetching and parsing
src/gdem042f86.*    E-paper display driver, kept unchanged
tools/              Font generation, preview, and lunar validation scripts
```

## 中文版

中文版 [README-zh.md](README-zh.md) 在这里.
