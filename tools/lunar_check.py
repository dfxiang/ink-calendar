"""Validate the 1900-2100 lunar calendar table + algorithm.

The table was rebuilt from the authoritative lunar-python library (the
classic circulating "lunarInfo" table has known typos for 1933/1978/1996,
fixed here). The same table and conversion steps are ported to src/lunar.cpp.

Run with:
    python tools/lunar_check.py
"""

from datetime import date

LUNAR_INFO = [
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
    0x092e0, 0x0d2e3, 0x0c960, 0x0d557, 0x0d4a0, 0x0da50, 0x05d55, 0x056a0, 0x0a6d0, 0x055d4,
    0x052d0, 0x0a9b8, 0x0a950, 0x0b4a0, 0x0b6a6, 0x0ad50, 0x055a0, 0x0aba4, 0x0a5b0, 0x052b0,
    0x0b273, 0x06930, 0x07337, 0x06aa0, 0x0ad50, 0x14b55, 0x04b60, 0x0a570, 0x054e4, 0x0d160,
    0x0e968, 0x0d520, 0x0daa0, 0x16aa6, 0x056d0, 0x04ae0, 0x0a9d4, 0x0a2d0, 0x0d150, 0x0f252,
    0x0d520,
]


def leap_month(y):
    return LUNAR_INFO[y - 1900] & 0xF


def leap_days(y):
    if leap_month(y):
        return 30 if LUNAR_INFO[y - 1900] & 0x10000 else 29
    return 0


def month_days(y, m):
    return 30 if LUNAR_INFO[y - 1900] & (0x10000 >> m) else 29


def lunar_year_days(y):
    s = 348
    for k in range(12):
        s += 1 if LUNAR_INFO[y - 1900] & (0x8000 >> k) else 0
    return s + leap_days(y)


MONTHS = ["正月", "二月", "三月", "四月", "五月", "六月",
          "七月", "八月", "九月", "十月", "冬月", "腊月"]


def solar_to_lunar(year, month, day):
    """Return (lunar_year, lunar_month, lunar_day, is_leap_month).

    Mirrors src/lunar.cpp exactly: counts days since 1900-01-31
    (lunar 1900-01-01) and walks the lunar table.
    """
    offset = (date(year, month, day) - date(1900, 1, 31)).days

    # Find the lunar year.
    y = 1900
    while True:
        length = lunar_year_days(y)
        if offset >= length:
            offset -= length
            y += 1
        else:
            break

    # Find the lunar month inside that year.
    leap = leap_month(y)
    is_leap = False
    m = 1
    while True:
        if leap > 0 and m == leap + 1 and not is_leap:
            m -= 1
            is_leap = True
            length = leap_days(y)
        else:
            length = month_days(y, m)
        if is_leap and m == leap + 1:
            is_leap = False
        if offset >= length:
            offset -= length
            m += 1
        else:
            break

    return y, m, offset + 1, is_leap


def fmt(y, m, d, leap):
    name = ("闰" if leap else "") + MONTHS[m - 1]
    return f"{name}{day_name(d)}"


def day_name(d):
    ten = "初十廿"
    if d == 10:
        return "初十"
    if d == 20:
        return "二十"
    if d == 30:
        return "三十"
    if d < 10:
        return "初" + "一二三四五六七八九"[d - 1]
    if d < 20:
        return "十" + "一二三四五六七八九"[d - 11]
    return "廿" + "一二三四五六七八九"[d - 21]


def stem_branch(year):
    stems = "甲乙丙丁戊己庚辛壬癸"
    branches = "子丑寅卯辰巳午未申酉戌亥"
    yc = year - 1864
    return stems[yc % 10] + branches[yc % 12]


def check(solar, expected):
    y, m, d, leap = solar_to_lunar(*solar)
    got = fmt(y, m, d, leap)
    gz = stem_branch(y) + "年"
    ok = got == expected["lunar"]
    print(f"{solar} -> {gz} {got}  expected {expected['lunar']}  {'OK' if ok else 'FAIL'}")
    if not ok:
        raise SystemExit(1)


if __name__ == "__main__":
    print(f"table entries: {len(LUNAR_INFO)} (expect 201)")
    check((2024, 2, 10), {"lunar": "正月初一"})
    check((2025, 1, 29), {"lunar": "正月初一"})
    check((2026, 2, 17), {"lunar": "正月初一"})
    check((2023, 1, 22), {"lunar": "正月初一"})
    check((2022, 2, 1), {"lunar": "正月初一"})
    check((2024, 2, 24), {"lunar": "正月十五"})
    check((2024, 6, 10), {"lunar": "五月初五"})
    check((2024, 9, 17), {"lunar": "八月十五"})
    check((2025, 10, 6), {"lunar": "八月十五"})
    check((2024, 12, 31), {"lunar": "腊月初一"})
    check((2023, 3, 22), {"lunar": "闰二月初一"})   # 2023 has leap month 2
    check((2025, 7, 25), {"lunar": "闰六月初一"})   # 2025 has leap month 6
    check((1900, 1, 31), {"lunar": "正月初一"})
    check((1900, 12, 22), {"lunar": "冬月初一"})
    print("stem/branch spot checks:", stem_branch(1900), stem_branch(2026))
    y, m, d, leap = solar_to_lunar(2026, 8, 3)
    print("2026-08-03 ->", stem_branch(y) + "年", fmt(y, m, d, leap))
    print("ALL PASSED")
