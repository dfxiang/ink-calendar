#include "lunar.h"

#include <string.h>

namespace {

// Lunar year table for 1900..2100 (one entry per year).
// Bits 0..3: leap month (0 = none); bit 16: leap month has 30 days;
// bits 17..28: months 1..12 have 30 days (0 = 29 days).
// Rebuilt from the authoritative lunar-python data; the commonly
// circulating classic table has known typos for 1933/1978/1996 that are
// fixed here (verified against lunar-python / zhdate over 1900-2100).
const uint32_t kLunarInfo[] = {
    0x04bd8, 0x04ae0, 0x0a570, 0x054d5, 0x0d260, 0x0d950, 0x16554, 0x056a0, 0x09ad0, 0x055d2,
    0x04ae0, 0x0a5b6, 0x0a4d0, 0x0d250, 0x1d255, 0x0b540, 0x0d6a0, 0x0ada2, 0x095b0, 0x14977,
    0x04970, 0x0a4b0, 0x0b4b5, 0x06a50, 0x06d40, 0x1ab54, 0x02b60, 0x09570, 0x052f2, 0x04970,
    0x06566, 0x0d4a0, 0x0ea50, 0x16a95, 0x05ad0, 0x02b60, 0x186e3, 0x092e0, 0x1c8d7, 0x0c950,
    0x0d4a0, 0x1d8a6, 0x0b550, 0x056a0, 0x1a5b4, 0x025d0, 0x092d0, 0x0d2b2, 0x0a950, 0x0b557,
    0x06ca0, 0x0b550, 0x15355, 0x04da0, 0x0a5b0, 0x14573, 0x052b0, 0x0a9a8, 0x0e950, 0x06aa0,
    0x0aea6, 0x0ab50, 0x04b60, 0x0aae4, 0x0a570, 0x05260, 0x0f263, 0x0d950, 0x05b57, 0x056a0,
    0x096d0, 0x04dd5, 0x04ad0, 0x0a4d0, 0x0d4d4, 0x0d250, 0x0d558, 0x0b540, 0x0b6a0, 0x195a6,
    0x095b0, 0x049b0, 0x0a974, 0x0a4b0, 0x0b27a, 0x06a50, 0x06d40, 0x0af46, 0x0ab60, 0x09570,
    0x04af5, 0x04970, 0x064b0, 0x074a3, 0x0ea50, 0x06b58, 0x05ac0, 0x0ab60, 0x096d5, 0x092e0,
    0x0c960, 0x0d954, 0x0d4a0, 0x0da50, 0x07552, 0x056a0, 0x0abb7, 0x025d0, 0x092d0, 0x0cab5,
    0x0a950, 0x0b4a0, 0x0baa4, 0x0ad50, 0x055d9, 0x04ba0, 0x0a5b0, 0x15176, 0x052b0, 0x0a930,
    0x07954, 0x06aa0, 0x0ad50, 0x05b52, 0x04b60, 0x0a6e6, 0x0a4e0, 0x0d260, 0x0ea65, 0x0d530,
    0x05aa0, 0x076a3, 0x096d0, 0x04afb, 0x04ad0, 0x0a4d0, 0x1d0b6, 0x0d250, 0x0d520, 0x0dd45,
    0x0b5a0, 0x056d0, 0x055b2, 0x049b0, 0x0a577, 0x0a4b0, 0x0aa50, 0x1b255, 0x06d20, 0x0ada0,
    0x14b63, 0x09370, 0x049f8, 0x04970, 0x064b0, 0x168a6, 0x0ea50, 0x06b20, 0x1a6c4, 0x0aae0,
    0x0a2e0, 0x0d2e3, 0x0c960, 0x0d557, 0x0d4a0, 0x0da50, 0x05d55, 0x056a0, 0x0a6d0, 0x055d4,
    0x052d0, 0x0a9b8, 0x0a950, 0x0b4a0, 0x0b6a6, 0x0ad50, 0x055a0, 0x0aba4, 0x0a5b0, 0x052b0,
    0x0b273, 0x06930, 0x07337, 0x06aa0, 0x0ad50, 0x14b55, 0x04b60, 0x0a570, 0x054e4, 0x0d160,
    0x0e968, 0x0d520, 0x0daa0, 0x16aa6, 0x056d0, 0x04ae0, 0x0a9d4, 0x0a2d0, 0x0d150, 0x0f252,
    0x0d520,
};

constexpr size_t kLunarInfoCount = sizeof(kLunarInfo) / sizeof(kLunarInfo[0]);

int leapMonth(int year) {
  return kLunarInfo[year - 1900] & 0x0F;
}

int leapDays(int year) {
  if (leapMonth(year) != 0) {
    return (kLunarInfo[year - 1900] & 0x10000) ? 30 : 29;
  }
  return 0;
}

int monthDays(int year, int month) {
  return (kLunarInfo[year - 1900] & (0x10000 >> month)) ? 30 : 29;
}

int lunarYearDays(int year) {
  int sum = 348;
  for (int i = 0; i < 12; ++i) {
    if (kLunarInfo[year - 1900] & (0x8000 >> i)) {
      ++sum;
    }
  }
  return sum + leapDays(year);
}

// Days since 1970-01-01 for a Gregorian date (Howard Hinnant's algorithm).
int64_t daysFromCivil(int year, int month, int day) {
  const int64_t y = year - (month <= 2);
  const int era = static_cast<int>((y >= 0 ? y : y - 399) / 400);
  const uint64_t yoe = static_cast<uint64_t>(y - era * 400);
  const uint64_t doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
  const uint64_t doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  return era * 146097LL + static_cast<int64_t>(doe) - 719468LL;
}

const char *kStems = "甲乙丙丁戊己庚辛壬癸";
const char *kBranches = "子丑寅卯辰巳午未申酉戌亥";
const char *kMonthNames[] = {"正月", "二月", "三月", "四月", "五月", "六月",
                             "七月", "八月", "九月", "十月", "冬月", "腊月"};
const char *kDayNames[] = {"初一", "初二", "初三", "初四", "初五", "初六", "初七", "初八", "初九", "初十",
                           "十一", "十二", "十三", "十四", "十五", "十六", "十七", "十八", "十九", "二十",
                           "廿一", "廿二", "廿三", "廿四", "廿五", "廿六", "廿七", "廿八", "廿九", "三十"};

}  // namespace

bool solarToLunar(int solarYear, int solarMonth, int solarDay, LunarDate *out) {
  if (out == nullptr) {
    return false;
  }
  if (solarYear < 1900 || solarYear > 2100) {
    return false;
  }

  int64_t offset = daysFromCivil(solarYear, solarMonth, solarDay) - daysFromCivil(1900, 1, 31);
  if (offset < 0) {
    return false;
  }

  // Walk to the containing lunar year.
  int year = 1900;
  while (true) {
    const int length = lunarYearDays(year);
    if (offset >= length) {
      offset -= length;
      ++year;
      if (year - 1900 >= static_cast<int>(kLunarInfoCount)) {
        return false;
      }
    } else {
      break;
    }
  }

  // Walk to the containing lunar month.
  const int leap = leapMonth(year);
  bool isLeap = false;
  int month = 1;
  while (true) {
    int length;
    if (leap > 0 && month == leap + 1 && !isLeap) {
      --month;
      isLeap = true;
      length = leapDays(year);
    } else {
      length = monthDays(year, month);
    }
    if (isLeap && month == leap + 1) {
      isLeap = false;
    }
    if (offset >= length) {
      offset -= length;
      ++month;
    } else {
      break;
    }
  }

  out->year = year;
  out->month = month;
  out->day = static_cast<int>(offset) + 1;
  out->leap = isLeap;
  return true;
}

const char *ganZhiYear(int lunarYear) {
  const int yearCyl = lunarYear - 1864;  // 1864 = 甲子
  static char name[7];
  memcpy(name, kStems + 3 * (yearCyl % 10), 3);
  memcpy(name + 3, kBranches + 3 * (yearCyl % 12), 3);
  name[6] = '\0';
  return name;
}

void lunarMonthName(int month, bool leap, char *out) {
  if (out == nullptr) {
    return;
  }
  size_t len = 0;
  if (leap) {
    memcpy(out, "闰", 3);
    len = 3;
  }
  memcpy(out + len, kMonthNames[month - 1], 6);
  out[len + 6] = '\0';
}

void lunarDayName(int day, char *out) {
  if ((out == nullptr) || (day < 1) || (day > 30)) {
    return;
  }
  memcpy(out, kDayNames[day - 1], 6);
  out[6] = '\0';
}

const char *festivalFor(int solarYear, int solarMonth, int solarDay, const LunarDate &lunar) {
  // 农历节日(优先于普通农历文字)
  if (lunar.month == 1 && lunar.day == 1) {
    return "春节";
  }
  if (lunar.month == 1 && lunar.day == 15) {
    return "元宵";
  }
  if (lunar.month == 5 && lunar.day == 5) {
    return "端午";
  }
  if (lunar.month == 7 && lunar.day == 7) {
    return "七夕";
  }
  if (lunar.month == 8 && lunar.day == 15) {
    return "中秋";
  }
  if (lunar.month == 9 && lunar.day == 9) {
    return "重阳";
  }
  if (lunar.month == 12 && lunar.day == 8) {
    return "腊八";
  }

  // 除夕:腊月最后一天(廿九或三十)
  if (lunar.month == 12) {
    LunarDate next;
    int nextYear = solarYear;
    int nextMonth = solarMonth;
    int nextDay = solarDay + 1;
    static const int kMonthLen[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int limit = kMonthLen[nextMonth - 1];
    if (nextMonth == 2 && (nextYear % 4 == 0) && ((nextYear % 100 != 0) || (nextYear % 400 == 0))) {
      limit = 29;
    }
    if (nextDay > limit) {
      nextDay = 1;
      ++nextMonth;
      if (nextMonth > 12) {
        nextMonth = 1;
        ++nextYear;
      }
    }
    if (solarToLunar(nextYear, nextMonth, nextDay, &next) && next.month == 1 && next.day == 1) {
      return "除夕";
    }
  }

  // 公历节日
  if (solarMonth == 1 && solarDay == 1) {
    return "元旦";
  }
  if (solarMonth == 3 && solarDay == 8) {
    return "妇女节";
  }
  if (solarMonth == 5 && solarDay == 1) {
    return "劳动节";
  }
  if (solarMonth == 6 && solarDay == 1) {
    return "儿童节";
  }
  if (solarMonth == 8 && solarDay == 1) {
    return "建军节";
  }
  if (solarMonth == 9 && solarDay == 10) {
    return "教师节";
  }
  if (solarMonth == 10 && solarDay == 1) {
    return "国庆节";
  }
  return nullptr;
}
