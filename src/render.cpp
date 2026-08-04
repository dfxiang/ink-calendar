#include "render.h"

#include <stdio.h>
#include <string.h>

#include "config.h"
#include "lunar.h"

namespace {

constexpr int kMargin = 8;
constexpr int kColWidth = 57;
constexpr int kRowHeight = 36;
constexpr int kGridX0 = 0;
constexpr int kGridY0 = 80;
constexpr int kWeekdayLabelY = 56;

const char kWeekdays[] = "日一二三四五六";

int daysInMonth(int year, int month) {
  static const int kDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
  int days = kDays[month - 1];
  if (month == 2 && (year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0))) {
    days = 29;
  }
  return days;
}

uint8_t dateColor(int weekday) {
  // 周一(1)到周五(5)红色,周六(6)/周日(0)黑色
  return (weekday >= 1 && weekday <= 5) ? kColorRed : kColorBlack;
}

// Centered 16px text inside a calendar cell.
void drawCellText(FrameBuffer &frame, int col, int row, int yOffset, const char *text,
                  uint8_t color) {
  const int cellX = kGridX0 + col * kColWidth;
  const int cellY = kGridY0 + row * kRowHeight;
  const int width = frame.textWidth(text, kFont16);
  frame.drawText(cellX + (kColWidth - width) / 2, cellY + yOffset, text, color, kFont16);
}

void drawDayCell(FrameBuffer &frame, int col, int row, int day, bool isToday,
                 const LunarDate &lunar, const char *festival) {
  const int cellX = kGridX0 + col * kColWidth;
  const int cellY = kGridY0 + row * kRowHeight;
  const int weekday = col;

  if (isToday && TODAY_YELLOW_BG) {
    frame.fillRect(cellX + 2, cellY + 2, kColWidth - 4, kRowHeight - 4, kColorYellow);
  }

  char buf[16];
  snprintf(buf, sizeof(buf), "%d", day);
  drawCellText(frame, col, row, 3, buf, dateColor(weekday));

  if (festival != nullptr) {
    drawCellText(frame, col, row, 20, festival, kColorRed);
  } else if (lunar.day == 1) {
    lunarMonthName(lunar.month, lunar.leap, buf);
    drawCellText(frame, col, row, 20, buf, kColorBlack);
  } else {
    lunarDayName(lunar.day, buf);
    drawCellText(frame, col, row, 20, buf, kColorBlack);
  }
  // 当天加红色方框
  // if (isToday) {
  //   frame.drawRect(cellX + 2, cellY + 2, kColWidth - 4, kRowHeight - 4, kColorRed, 2);
  // }
}

}  // namespace

void renderCalendar(FrameBuffer &frame, int year, int month, int day, int weekday,
                    const WeatherInfo &weather) {
  frame.clear(kColorWhite);

  // ---- Header line 1: date + weather -------------------------------------
  char line[64];
  const int wdayIndex = weekday;  // tm_wday: 0 = Sunday
  char wdayName[4];
  memcpy(wdayName, kWeekdays + 3 * wdayIndex, 3);
  wdayName[3] = '\0';
  snprintf(line, sizeof(line), "%d年%d月%d日 星期%s", year, month, day, wdayName);
  frame.drawText(kMargin, 6, line, kColorBlack, kFont16);

  if (weather.valid) {
    snprintf(line, sizeof(line), "%s %d°C", weatherText(weather.code), weather.temperature);
  } else {
    snprintf(line, sizeof(line), "--°C");
  }
  const int weatherWidth = frame.textWidth(line, kFont16);
  frame.drawText(FrameBuffer::kWidth - kMargin - weatherWidth, 6, line, kColorRed, kFont16);

  // ---- Header line 2: lunar date + humidity / high / low -----------------
  LunarDate lunar;
  if (solarToLunar(year, month, day, &lunar)) {
    char monthName[8];
    char dayName[8];
    lunarMonthName(lunar.month, lunar.leap, monthName);
    lunarDayName(lunar.day, dayName);
    snprintf(line, sizeof(line), "农历%s年%s%s", ganZhiYear(lunar.year), monthName, dayName);
  } else {
    snprintf(line, sizeof(line), "农历 --");
  }
  frame.drawText(kMargin, 28, line, kColorBlack, kFont16);

  snprintf(line, sizeof(line), "湿度%d%% 高%d° 低%d°", weather.valid ? weather.humidity : 0,
           weather.valid ? weather.tempMax : 0, weather.valid ? weather.tempMin : 0);
  if (!weather.valid) {
    snprintf(line, sizeof(line), "湿度-- 高-- 低--");
  }
  const int metaWidth = frame.textWidth(line, kFont12);
  frame.drawText(FrameBuffer::kWidth - kMargin - metaWidth, 32, line, kColorRed, kFont12);

  // ---- Separator + weekday labels ----------------------------------------
  frame.drawHLine(0, 52, FrameBuffer::kWidth, kColorBlack);
  for (int col = 0; col < 7; ++col) {
    char label[4];
    memcpy(label, kWeekdays + 3 * col, 3);
    label[3] = '\0';
    const int width = frame.textWidth(label, kFont16);
    const int x = kGridX0 + col * kColWidth + (kColWidth - width) / 2;
    frame.drawText(x, kWeekdayLabelY, label, dateColor(col), kFont16);
  }

  // ---- Grid lines ----------------------------------------------------------
  for (int row = 0; row <= 6; ++row) {
    frame.drawHLine(kGridX0, kGridY0 + row * kRowHeight, FrameBuffer::kWidth, kColorBlack);
  }
  for (int col = 0; col <= 7; ++col) {
    frame.drawVLine(kGridX0 + col * kColWidth, kGridY0, 6 * kRowHeight, kColorBlack);
  }

  // ---- Calendar cells -------------------------------------------------------
  const int firstWeekday = ((weekday - (day - 1)) % 7 + 7) % 7;
  const int totalDays = daysInMonth(year, month);
  for (int d = 1; d <= totalDays; ++d) {
    const int index = firstWeekday + d - 1;
    const int col = index % 7;
    const int row = index / 7;
    LunarDate lunar;
    if (!solarToLunar(year, month, d, &lunar)) {
      continue;
    }
    const char *festival = festivalFor(year, month, d, lunar);
    drawDayCell(frame, col, row, d, d == day, lunar, festival);
  }
}

namespace {

void centeredText(FrameBuffer &frame, int y, const char *text, uint8_t color,
                  const FontInfo &font) {
  const int width = frame.textWidth(text, font);
  frame.drawText((FrameBuffer::kWidth - width) / 2, y, text, color, font);
}

}  // namespace

void renderConfigScreen(FrameBuffer &frame, const char *apSsid, const char *apIp) {
  frame.clear(kColorWhite);
  centeredText(frame, 34, "WiFi 配置模式", kColorRed, kFont16);
  frame.drawHLine(20, 64, FrameBuffer::kWidth - 40, kColorBlack);

  centeredText(frame, 88, "1. 手机连接热点", kColorBlack, kFont16);
  centeredText(frame, 108, apSsid, kColorBlack, kFont16);
  centeredText(frame, 148, "2. 手机浏览器打开", kColorBlack, kFont16);
  char url[32];
  snprintf(url, sizeof(url), "http://%s", apIp);
  centeredText(frame, 168, url, kColorBlack, kFont16);

  centeredText(frame, 220, "RST 快速按两次 = 清除配置", kColorBlack, kFont12);
  centeredText(frame, 240, "保存后自动重启并连接 WiFi", kColorBlack, kFont12);
}

void renderConnectingScreen(FrameBuffer &frame, const char *ssid) {
  frame.clear(kColorWhite);
  centeredText(frame, 80, "正在连接 WiFi...", kColorBlack, kFont16);
  char line[48];
  snprintf(line, sizeof(line), "SSID: %s", ssid);
  centeredText(frame, 120, line, kColorBlack, kFont16);
  centeredText(frame, 160, "正在同步时间与天气", kColorBlack, kFont12);
}
