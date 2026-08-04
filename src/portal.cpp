#include "portal.h"

#include <Arduino.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <WiFi.h>

#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "portal_page.h"
#include "settings.h"

namespace {

constexpr byte kDnsPort = 53;
IPAddress apIp(192, 168, 4, 1);
IPAddress apGateway(192, 168, 4, 1);
IPAddress apSubnet(255, 255, 255, 0);

DNSServer dnsServer;
WebServer server(80);

bool started = false;
bool dirty = false;
uint32_t rebootAtMs = 0;

void addCommonHeaders() {
  server.sendHeader(F("Cache-Control"), F("no-store, no-cache, must-revalidate, max-age=0"));
  server.sendHeader(F("Pragma"), F("no-cache"));
  server.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
}

String htmlEscape(const String &input) {
  String out;
  out.reserve(input.length() + 16);
  for (size_t i = 0; i < input.length(); ++i) {
    const char c = input[i];
    switch (c) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      case '"':
        out += "&quot;";
        break;
      case '\'':
        out += "&#39;";
        break;
      default:
        out += c;
    }
  }
  return out;
}

String portalPage() {
  DeviceSettings s;
  loadSettings(&s);
  char latBuf[16];
  char lonBuf[16];
  dtostrf(s.hasLatitude ? s.latitude : WEATHER_LATITUDE, 0, 4, latBuf);
  dtostrf(s.hasLongitude ? s.longitude : WEATHER_LONGITUDE, 0, 4, lonBuf);

  String page = PORTAL_HTML;
  page.replace("@@SSID@@", htmlEscape(s.ssid));
  page.replace("@@PASS@@", htmlEscape(s.password));
  page.replace("@@LAT@@", latBuf);
  page.replace("@@LON@@", lonBuf);
  return page;
}

void sendPortalPage() {
  addCommonHeaders();
  server.send(200, F("text/html; charset=utf-8"), portalPage());
}

void redirectToPortal() {
  addCommonHeaders();
  server.sendHeader(F("Location"), F("http://192.168.4.1/"), true);
  server.send(302, F("text/plain"), F(""));
}

void sendNoContent() {
  addCommonHeaders();
  server.send(204, F("text/plain"), F(""));
}

void handleScan() {
  addCommonHeaders();
  String json = F("{\"networks\":[");
  const int n = WiFi.scanNetworks(false, true);  // synchronous, show hidden
  if (n < 0) {
    WiFi.scanDelete();
    server.send(200, F("application/json"), F("{\"networks\":[]}"));
    return;
  }
  bool first = true;
  for (int i = 0; i < n && i < 30; ++i) {
    String ssid = WiFi.SSID(i);
    if (ssid.length() == 0) {
      continue;
    }
    if (!first) {
      json += ",";
    }
    first = false;
    json += "{\"ssid\":\"";
    json += htmlEscape(ssid);
    json += "\",\"rssi\":";
    json += String(WiFi.RSSI(i));
    json += "}";
  }
  WiFi.scanDelete();
  json += "]}";
  server.send(200, F("application/json"), json);
}

void scheduleReboot() {
  rebootAtMs = millis() + 1500;
}

void handleSave() {
  addCommonHeaders();
  DeviceSettings s;
  loadSettings(&s);

  String ssid = server.arg(F("ssid"));
  String password = server.arg(F("pass"));
  String latStr = server.arg(F("lat"));
  String lonStr = server.arg(F("lon"));

  ssid.trim();
  if (ssid.length() == 0) {
    server.send(400, F("text/plain; charset=utf-8"), F("SSID 不能为空"));
    return;
  }

  strlcpy(s.ssid, ssid.c_str(), sizeof(s.ssid));
  strlcpy(s.password, password.c_str(), sizeof(s.password));

  const float lat = latStr.toFloat();
  const float lon = lonStr.toFloat();
  if (latStr.length() > 0 && lat >= -90.0f && lat <= 90.0f) {
    s.hasLatitude = true;
    s.latitude = lat;
  }
  if (lonStr.length() > 0 && lon >= -180.0f && lon <= 180.0f) {
    s.hasLongitude = true;
    s.longitude = lon;
  }

  saveSettings(s);
  dirty = true;
  Serial.printf("[portal] saved: ssid=%s lat=%.4f lon=%.4f\n", s.ssid, s.latitude, s.longitude);
  scheduleReboot();

  server.send(200, F("text/html; charset=utf-8"),
              F("<!DOCTYPE html><html lang=\"zh-CN\"><head><meta charset=\"utf-8\">"
                "<meta http-equiv=\"refresh\" content=\"1\"></head>"
                "<body style=\"font-family:sans-serif;text-align:center;padding-top:40px\">"
                "<h2>已保存,设备正在重启…</h2></body></html>"));
}

void handleClear() {
  addCommonHeaders();
  clearSettings();
  dirty = true;
  Serial.println(F("[portal] settings cleared, rebooting"));
  scheduleReboot();
  server.send(200, F("text/html; charset=utf-8"),
              F("<!DOCTYPE html><html lang=\"zh-CN\"><head><meta charset=\"utf-8\">"
                "<meta http-equiv=\"refresh\" content=\"1\"></head>"
                "<body style=\"font-family:sans-serif;text-align:center;padding-top:40px\">"
                "<h2>配置已清除,正在重启…</h2></body></html>"));
}

}  // namespace

void portalStart() {
  if (started) {
    return;
  }
  started = true;
  rebootAtMs = 0;

  WiFi.disconnect();
  WiFi.setAutoReconnect(false);
  // AP_STA 保留 STA 接口,手机端"扫描 WiFi"功能才能工作
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIp, apGateway, apSubnet);
  WiFi.softAP(AP_SSID, AP_PASSWORD);

  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(kDnsPort, "*", apIp);

  server.on(F("/"), HTTP_GET, sendPortalPage);
  server.on(F("/index.html"), HTTP_GET, sendPortalPage);
  server.on(F("/save"), HTTP_POST, handleSave);
  server.on(F("/clear"), HTTP_POST, handleClear);
  server.on(F("/scan"), HTTP_GET, handleScan);

  // 常见 captive portal 探测,引导手机弹出配置页
  server.on(F("/generate_204"), HTTP_GET, redirectToPortal);
  server.on(F("/gen_204"), HTTP_GET, redirectToPortal);
  server.on(F("/hotspot-detect.html"), HTTP_GET, sendPortalPage);
  server.on(F("/library/test/success.html"), HTTP_GET, sendPortalPage);
  server.on(F("/ncsi.txt"), HTTP_GET, redirectToPortal);
  server.on(F("/connecttest.txt"), HTTP_GET, redirectToPortal);
  server.on(F("/fwlink"), HTTP_GET, redirectToPortal);
  server.on(F("/favicon.ico"), HTTP_GET, sendNoContent);
  server.onNotFound(redirectToPortal);
  server.begin();

  Serial.print(F("[portal] AP SSID: "));
  Serial.println(AP_SSID);
  Serial.print(F("[portal] AP IP: "));
  Serial.println(WiFi.softAPIP());
}

void portalLoop() {
  if (!started) {
    return;
  }
  dnsServer.processNextRequest();
  server.handleClient();
  if (rebootAtMs != 0 && (millis() - rebootAtMs) < 0x80000000UL) {
    delay(100);
    esp_restart();
  }
}

bool portalDirty() {
  return dirty;
}
