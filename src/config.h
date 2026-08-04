#pragma once

// ---------------------------------------------------------------------------
// 用户配置区 - 使用前请修改
// ---------------------------------------------------------------------------

// 天气位置(Open-Meteo,无需 API Key)。默认:上海
// 其他城市坐标可在地图或 https://open-meteo.com 查询
#define WEATHER_LATITUDE 31.2304
#define WEATHER_LONGITUDE 121.4737

// NTP 服务器(中国地区可用 ntp.aliyun.com / ntp.tencent.com)
#define NTP_SERVER1 "ntp.aliyun.com"
#define NTP_SERVER2 "ntp.tencent.com"
#define NTP_SERVER3 "pool.ntp.org"

// 时区偏移(秒)。中国标准时间 = UTC+8,无夏令时
#define TIMEZONE_OFFSET_SEC (8 * 3600)
#define TIMEZONE_DST_SEC 0

// 轮询周期:每隔多少秒检测一次日期变化并拉取天气
#define TICK_INTERVAL_SEC 60

// 天气变化触发刷屏的最小间隔(秒)。
// 墨水屏频繁刷新会缩短寿命,默认天气变化后至少间隔 2 分钟才再次刷屏
#define WEATHER_MIN_REFRESH_SEC 120

// 今天是否用黄色底 + 红色方框高亮(1=是)。设置为 0 则只用红色方框
#define TODAY_YELLOW_BG 1

// ---------------------------------------------------------------------------
// WiFi 配置门户(手机配置模式)
// ---------------------------------------------------------------------------

// 配置模式的 AP 热点名称与密码(密码留空 = 开放热点)
#define AP_SSID "InkScreen-Calendar"
#define AP_PASSWORD ""

// 首次连接超时(毫秒):超时后自动进入配置模式,等待手机配置
#define WIFI_CONNECT_TIMEOUT_MS 60000

// 配置模式最长停留时间(毫秒):超时自动重启回正常模式。
// 若从未保存过任何配置,则一直停留在配置模式等待
#define PORTAL_TIMEOUT_MS (15 * 60 * 1000UL)

// 可选:外接一个按钮接到该 GPIO,长按 3 秒进入配置模式。
// -1 = 不使用(默认靠 RST 双击 / 自动进入配置模式)
#define CONFIG_BUTTON_PIN -1

// RST 双击检测窗口(微秒):两次按 RST 间隔小于该值视为"重置配置"
#define RST_DOUBLE_PRESS_WINDOW_US (3 * 1000000ULL)
