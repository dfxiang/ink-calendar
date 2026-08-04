#pragma once

struct LunarDate {
  int year;   // lunar year (e.g. 2026)
  int month;  // 1..12
  int day;    // 1..30
  bool leap;  // true when `month` is the leap month of the year
};

// Solar (Gregorian) date -> lunar date. Valid for 1900-01-31 .. 2100-12-31.
// Returns false for dates outside that range.
bool solarToLunar(int solarYear, int solarMonth, int solarDay, LunarDate *out);

// Ganzhi (stem-branch) year name, e.g. "丙午". Static storage.
const char *ganZhiYear(int lunarYear);

// Lunar month name, e.g. "正月", "腊月", "闰二月". `out` needs >= 8 bytes.
void lunarMonthName(int month, bool leap, char *out);

// Lunar day name, e.g. "初一", "十五", "廿三", "三十". `out` needs >= 8 bytes.
void lunarDayName(int day, char *out);

// Festival name for a date, or nullptr if none. Solar festivals are checked
// first so they win over the regular lunar text; lunar festivals win over
// solar ones when both exist (not possible in practice).
const char *festivalFor(int solarYear, int solarMonth, int solarDay, const LunarDate &lunar);
