#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

#include "esp_attr.h"
#include "esp_system.h"
#include "soc/esp32c3/rtc.h"

#include "config.h"
#include "frame.h"
#include "gdem042f86.h"
#include "portal.h"
#include "render.h"
#include "settings.h"
#include "weather.h"

namespace {

constexpr int8_t kPinSck = 4;
constexpr int8_t kPinMiso = -1;
constexpr int8_t kPinMosi = 6;
constexpr int8_t kPinCs = 7;
constexpr int8_t kPinDc = 3;
constexpr int8_t kPinRst = 2;
constexpr int8_t kPinBusy = 10;

enum class Mode { kStation, kPortal };

Mode mode = Mode::kStation;
FrameBuffer frame;

bool renderedOnce = false;
bool wifiConnectedEver = false;
bool hadConfigBeforePortal = false;
uint32_t bootMs = 0;
uint32_t lastTickMs = 0;
uint32_t lastWifiLogMs = 0;
uint32_t lastWeatherRenderMs = 0;
uint32_t portalStartMs = 0;

int lastYear = 0;
int lastMonth = 0;
int lastDay = 0;
int lastWeatherKey = -1;

// ---- RST 双击检测 ----------------------------------------------------------
// RTC 内存跨重启保留(断电即清零):快速按两次 RST(默认 3 秒内)会清除已
// 保存的 WiFi 配置并进入配置模式。
RTC_NOINIT_ATTR uint32_t g_rstMagic;
RTC_NOINIT_ATTR uint32_t g_rstCount;
RTC_NOINIT_ATTR uint64_t g_rstLastUs;

bool detectDoubleReset() {
  constexpr uint32_t kMagic = 0xD05B1E77u;
  const esp_reset_reason_t reason = esp_reset_reason();
  const uint64_t nowUs = esp_rtc_get_time_us();

  if (g_rstMagic != kMagic) {  // 首次上电:RTC 内存未初始化
    g_rstMagic = kMagic;
    g_rstCount = 0;
    g_rstLastUs = nowUs;
    return false;
  }

  if (reason == ESP_RST_EXT) {  // 按下 RST 键导致的复位
    const bool withinWindow =
        (nowUs >= g_rstLastUs) && (nowUs - g_rstLastUs) <= RST_DOUBLE_PRESS_WINDOW_US;
    g_rstCount = withinWindow ? g_rstCount + 1 : 1;
  } else {  // 软件重启 / 上电等:计数清零
    g_rstCount = 0;
  }
  g_rstLastUs = nowUs;
  return g_rstCount >= 2;
}

// ---- 显示辅助 --------------------------------------------------------------

bool renderToPanel(bool fastUpdate) {
  const bool ok = GDEM042F86::displayFrame(frame.data(), GDEM042F86::kFrameBytes, fastUpdate);
  Serial.println(ok ? F("[panel] refresh done") : F("[panel] refresh FAILED"));
  return ok;
}

void showConfigScreen() {
  renderConfigScreen(frame, AP_SSID, WiFi.softAPIP().toString().c_str());
  renderToPanel(false);
}

void showConnectingScreen(const char *ssid) {
  renderConnectingScreen(frame, ssid);
  renderToPanel(false);
}

// ---- WiFi -----------------------------------------------------------------

void stationStart(const DeviceSettings &cfg) {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  WiFi.begin(cfg.ssid, cfg.password);
  Serial.printf("[wifi] connecting to %s\n", cfg.ssid);
}

void enterPortal() {
  if (mode == Mode::kPortal) {
    return;
  }
  mode = Mode::kPortal;
  portalStartMs = millis();
  hadConfigBeforePortal = true;  // 从正常模式进入:说明已保存过 WiFi 配置
  portalStart();
  showConfigScreen();
}

void checkConfigButton() {
#if CONFIG_BUTTON_PIN >= 0
  static bool pressed = false;
  static uint32_t pressStartMs = 0;
  if (digitalRead(CONFIG_BUTTON_PIN) == LOW) {
    if (!pressed) {
      pressed = true;
      pressStartMs = millis();
    } else if (millis() - pressStartMs >= 3000) {
      pressed = false;
      Serial.println(F("[btn] long press -> config portal"));
      enterPortal();
    }
  } else {
    pressed = false;
  }
#endif
}

// ---- 时间与天气 -------------------------------------------------------------

bool timeIsValid() {
  const time_t now = time(nullptr);
  return now > 1600000000;  // 约 2020-09,说明 NTP 已同步
}

// 每 60 秒的例行检查:日期变化整屏刷新;天气变化快速刷新
bool tick() {
  if (!timeIsValid()) {
    return false;
  }

  time_t now = time(nullptr);
  struct tm nowTm;
  localtime_r(&now, &nowTm);
  const int year = nowTm.tm_year + 1900;
  const int month = nowTm.tm_mon + 1;
  const int day = nowTm.tm_mday;

  WeatherInfo weather;
  const bool gotWeather = fetchWeather(&weather);
  const int weatherKey = gotWeather ? weatherChangeKey(weather) : -1;

  const bool dateChanged = !renderedOnce || year != lastYear || month != lastMonth || day != lastDay;
  const bool cooldownOk =
      static_cast<uint32_t>(millis() - lastWeatherRenderMs) >=
      static_cast<uint32_t>(WEATHER_MIN_REFRESH_SEC) * 1000UL;
  const bool weatherChanged = gotWeather && (weatherKey != lastWeatherKey) && cooldownOk;

  if (!dateChanged && !weatherChanged) {
    return false;
  }

  if (dateChanged) {
    Serial.printf("[tick] date change %04d-%02d-%02d\n", year, month, day);
  } else {
    Serial.println(F("[tick] weather change"));
  }

  renderCalendar(frame, year, month, day, nowTm.tm_wday, weather);
  const bool ok = renderToPanel(!dateChanged);

  renderedOnce = true;
  lastYear = year;
  lastMonth = month;
  lastDay = day;
  if (gotWeather) {
    lastWeatherKey = weatherKey;
    lastWeatherRenderMs = millis();
  }
  return ok;
}

// ---- 各模式主循环 -----------------------------------------------------------

void loopStation() {
  checkConfigButton();

  const uint32_t nowMs = millis();

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnectedEver = true;
  } else if (wifiConnectedEver) {
    // 曾经连上过,现在断开:每 30 秒主动重连一次(自动重连兜底)
    if (nowMs - lastWifiLogMs > 30000) {
      lastWifiLogMs = nowMs;
      Serial.printf("[wifi] disconnected (status=%d), reconnecting...\n", WiFi.status());
      WiFi.reconnect();
    }
  }

  // 首次连接超时:自动进入配置模式
  if (!wifiConnectedEver && (nowMs - bootMs >= WIFI_CONNECT_TIMEOUT_MS)) {
    Serial.println(F("[wifi] first connect timeout -> config portal"));
    enterPortal();
    return;
  }

  // WiFi 已连接但 NTP 长时间不同步:重启重试
  if (wifiConnectedEver && !timeIsValid() && (nowMs - bootMs > 5 * 60 * 1000UL)) {
    Serial.println(F("[ntp] sync timeout, restarting"));
    esp_restart();
  }

  if (!renderedOnce) {
    if (timeIsValid()) {
      if (tick()) {
        lastTickMs = nowMs;
      }
    }
    delay(100);
    return;
  }

  if (nowMs - lastTickMs >= static_cast<uint32_t>(TICK_INTERVAL_SEC) * 1000UL) {
    lastTickMs = nowMs;
    tick();
  }

  delay(10);
}

void loopPortal() {
  portalLoop();

  // 若保存过 WiFi 配置但仍进了配置模式(比如密码失效),超时后重启回正常模式
  if (hadConfigBeforePortal && (millis() - portalStartMs >= PORTAL_TIMEOUT_MS)) {
    Serial.println(F("[portal] idle timeout, restarting"));
    esp_restart();
  }
  delay(10);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(F("ESP32-C3 电子台历 starting..."));

  GDEM042F86::begin(kPinSck, kPinMiso, kPinMosi, kPinCs, kPinDc, kPinRst, kPinBusy);
  Serial.println(F("[panel] display bus initialized."));

#if CONFIG_BUTTON_PIN >= 0
  pinMode(CONFIG_BUTTON_PIN, INPUT_PULLUP);
#endif

  bootMs = millis();

  const bool doubleReset = detectDoubleReset();
  DeviceSettings cfg;
  loadSettings(&cfg);

  if (doubleReset) {
    Serial.println(F("[rst] double-press detected, clearing saved config"));
    clearSettings();
    cfg = DeviceSettings{};
  }

  if (cfg.hasWifi()) {
    mode = Mode::kStation;
    stationStart(cfg);
    configTime(TIMEZONE_OFFSET_SEC, TIMEZONE_DST_SEC, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
    Serial.println(F("[ntp] started."));
    showConnectingScreen(cfg.ssid);
  } else {
    mode = Mode::kPortal;
    portalStartMs = millis();
    hadConfigBeforePortal = false;
    portalStart();
    showConfigScreen();
  }
}

void loop() {
  if (mode == Mode::kStation) {
    loopStation();
  } else {
    loopPortal();
  }
}
