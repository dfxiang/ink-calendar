"""Render the calendar page to a PNG for visual inspection.

Uses the exact glyph bitmaps shipped in src/font_data.h and mirrors the
layout constants in src/render.cpp, so the preview matches the firmware.

Usage:
    python tools/preview.py [YYYY-MM-DD] [output.png]
"""

import re
import sys
from datetime import date
from pathlib import Path

from PIL import Image

ROOT = Path(__file__).resolve().parent.parent
FONT_DATA = ROOT / "src" / "font_data.h"

WHITE = (255, 255, 255)
BLACK = (0, 0, 0)
RED = (210, 20, 30)
YELLOW = (245, 215, 80)


def parse_fonts():
    """Parse the generated C header into {font_size: {code: [rows...]}}."""
    text = FONT_DATA.read_text(encoding="utf-8")
    fonts = {}
    for size in (16, 12):
        bits_match = re.search(rf"static const uint8_t kFont{size}Bits\[\] = \{{(.*?)\}};", text, re.S)
        codes_match = re.search(rf"static const uint32_t kFont{size}Codes\[\] = \{{(.*?)\}};", text, re.S)
        offs_match = re.search(rf"static const uint16_t kFont{size}Offsets\[\] = \{{(.*?)\}};", text, re.S)
        bits = [int(x, 16) for x in re.findall(r"0x[0-9A-Fa-f]+", bits_match.group(1))]
        codes = [int(x, 16) for x in re.findall(r"0x[0-9A-Fa-f]+", codes_match.group(1))]
        offs = [int(x) for x in re.findall(r"\d+", offs_match.group(1))]
        glyphs = {}
        row_bytes = (size + 7) // 8
        for code, off in zip(codes, offs):
            rows = []
            for r in range(size):
                row = 0
                for b in range(row_bytes):
                    row = (row << 8) | bits[off + r * row_bytes + b]
                rows.append(row)
            glyphs[code] = rows
        fonts[size] = glyphs
    return fonts


FONTS = parse_fonts()


class Preview:
    W = 400
    H = 300
    MARGIN = 8
    COL_W = 57
    ROW_H = 36
    GRID_Y0 = 80
    WEEKDAY_Y = 56

    def __init__(self):
        self.img = Image.new("RGB", (self.W, self.H), WHITE)
        self.px = self.img.load()

    def text(self, x, y, s, color, size):
        glyphs = FONTS[size]
        for ch in s:
            for row_idx, row in enumerate(glyphs.get(ord(ch), [])):
                for col in range(size):
                    # Packed rows are stored MSB-first in a 16-bit cell, so
                    # column 0 maps to bit 15 regardless of font size.
                    if row & (1 << (15 - col)):
                        self.px[x + col, y + row_idx] = color
            x += size
        return x

    def text_width(self, s, size):
        return len(s) * size

    def fill_rect(self, x, y, w, h, color):
        for yy in range(y, y + h):
            for xx in range(x, x + w):
                self.px[xx, yy] = color

    def rect(self, x, y, w, h, color, t=1):
        for i in range(t):
            for xx in range(x + i, x + w - i):
                self.px[xx, y + i] = color
                self.px[xx, y + h - 1 - i] = color
            for yy in range(y + i, y + h - i):
                self.px[x + i, yy] = color
                self.px[x + w - 1 - i, yy] = color

    def hline(self, x, y, w, color):
        for xx in range(x, x + w):
            self.px[xx, y] = color

    def vline(self, x, y, h, color):
        for yy in range(y, y + h):
            self.px[x, yy] = color

    def save(self, path):
        self.img.save(path)


def render(d, weather, preview):
    year, month, day = d.year, d.month, d.day
    weekday = (d.weekday() + 1) % 7  # tm_wday: 0 = Sunday
    from lunar_check import solar_to_lunar

    # Header line 1
    wd = "日一二三四五六"[weekday]
    line = f"{year}年{month}月{day}日 星期{wd}"
    preview.text(preview.MARGIN, 6, line, BLACK, 16)
    if weather[0]:
        line = f"{weather[0]} {weather[1]}°C"
    else:
        line = "--°C"
    preview.text(preview.W - preview.MARGIN - preview.text_width(line, 16), 6, line, RED, 16)

    # Header line 2
    ly, lm, ld, leap = solar_to_lunar(year, month, day)
    line = f"农历{ganzhi(ly)}年{month_name(lm, leap)}{day_name(ld)}"
    preview.text(preview.MARGIN, 28, line, BLACK, 16)
    line = f"湿度{weather[2]}% 高{weather[3]}° 低{weather[4]}°"
    preview.text(preview.W - preview.MARGIN - preview.text_width(line, 12), 32, line, RED, 12)

    # Separator + weekday labels
    preview.hline(0, 52, preview.W, BLACK)
    for col in range(7):
        label = "日一二三四五六"[col]
        color = RED if 1 <= col <= 5 else BLACK
        preview.text(col * preview.COL_W + (preview.COL_W - 16) // 2, preview.WEEKDAY_Y, label, color, 16)

    # Grid
    for row in range(7):
        preview.hline(0, preview.GRID_Y0 + row * preview.ROW_H, preview.W, BLACK)
    for col in range(8):
        preview.vline(col * preview.COL_W, preview.GRID_Y0, 6 * preview.ROW_H, BLACK)

    # Cells
    import calendar as cal
    total_days = cal.monthrange(year, month)[1]
    first_weekday = (weekday - (day - 1)) % 7
    for dnum in range(1, total_days + 1):
        idx = first_weekday + dnum - 1
        col, row = idx % 7, idx // 7
        cell_x = col * preview.COL_W
        cell_y = preview.GRID_Y0 + row * preview.ROW_H
        is_today = dnum == day
        if is_today:
            preview.fill_rect(cell_x + 2, cell_y + 2, preview.COL_W - 4, preview.ROW_H - 4, YELLOW)
        color = RED if 1 <= col <= 5 else BLACK
        num = str(dnum)
        preview.text(cell_x + (preview.COL_W - preview.text_width(num, 16)) // 2, cell_y + 3, num, color, 16)
        ly2, lm2, ld2, leap2 = solar_to_lunar(year, month, dnum)
        festival = festival_for(year, month, dnum, lm2, ld2, leap2)
        if festival:
            label = festival
        elif ld2 == 1:
            label = month_name(lm2, leap2)
        else:
            label = day_name(ld2)
        text_color = RED if festival else BLACK
        preview.text(cell_x + (preview.COL_W - preview.text_width(label, 16)) // 2, cell_y + 20, label, text_color, 16)
        # if is_today:
        #     preview.rect(cell_x + 2, cell_y + 2, preview.COL_W - 4, preview.ROW_H - 4, RED, 2)


def centered(preview, y, s, color, size):
    w = preview.text_width(s, size)
    preview.text((preview.W - w) // 2, y, s, color, size)


def render_config_screen(preview, ap_ssid="InkScreen-Calendar", ap_ip="192.168.4.1"):
    preview.img.paste(WHITE, (0, 0, preview.W, preview.H))
    centered(preview, 34, "WiFi 配置模式", RED, 16)
    preview.hline(20, 64, preview.W - 40, BLACK)
    centered(preview, 88, "1. 手机连接热点", BLACK, 16)
    centered(preview, 108, ap_ssid, BLACK, 16)
    centered(preview, 148, "2. 手机浏览器打开", BLACK, 16)
    centered(preview, 168, f"http://{ap_ip}", BLACK, 16)
    centered(preview, 220, "RST 快速按两次 = 清除配置", BLACK, 12)
    centered(preview, 240, "保存后自动重启并连接 WiFi", BLACK, 12)


def render_connecting_screen(preview, ssid="MyWiFi"):
    preview.img.paste(WHITE, (0, 0, preview.W, preview.H))
    centered(preview, 80, "正在连接 WiFi...", BLACK, 16)
    centered(preview, 120, f"SSID: {ssid}", BLACK, 16)
    centered(preview, 160, "正在同步时间与天气", BLACK, 12)


def ganzhi(y):
    stems = "甲乙丙丁戊己庚辛壬癸"
    branches = "子丑寅卯辰巳午未申酉戌亥"
    yc = y - 1864
    return stems[yc % 10] + branches[yc % 12]


def month_name(m, leap):
    names = ["正月", "二月", "三月", "四月", "五月", "六月",
             "七月", "八月", "九月", "十月", "冬月", "腊月"]
    return ("闰" if leap else "") + names[m - 1]


def day_name(d):
    names = ["初一", "初二", "初三", "初四", "初五", "初六", "初七", "初八", "初九", "初十",
             "十一", "十二", "十三", "十四", "十五", "十六", "十七", "十八", "十九", "二十",
             "廿一", "廿二", "廿三", "廿四", "廿五", "廿六", "廿七", "廿八", "廿九", "三十"]
    return names[d - 1]


def festival_for(year, month, day, lm, ld, leap):
    # Lunar festivals
    if (lm, ld) == (1, 1):
        return "春节"
    if (lm, ld) == (1, 15):
        return "元宵"
    if (lm, ld) == (5, 5):
        return "端午"
    if (lm, ld) == (7, 7):
        return "七夕"
    if (lm, ld) == (8, 15):
        return "中秋"
    if (lm, ld) == (9, 9):
        return "重阳"
    if (lm, ld) == (12, 8):
        return "腊八"
    if lm == 12:
        from lunar_check import solar_to_lunar
        import datetime as dt
        nxt = date(year, month, day) + dt.timedelta(days=1)
        ny, nm, nd, nleap = solar_to_lunar(nxt.year, nxt.month, nxt.day)
        if (nm, nd) == (1, 1):
            return "除夕"
    # Solar festivals
    solar_fest = {(1, 1): "元旦", (3, 8): "妇女节", (5, 1): "劳动节",
                  (6, 1): "儿童节", (8, 1): "建军节", (9, 10): "教师节", (10, 1): "国庆节"}
    return solar_fest.get((month, day))


def main():
    target = date(2026, 8, 3)
    out = ROOT / "preview.png"
    if len(sys.argv) > 1:
        target = date.fromisoformat(sys.argv[1])
    if len(sys.argv) > 2:
        out = Path(sys.argv[2])
    # (condition text, temp, humidity, max, min)
    weather = ("多云", 34, 44, 36, 27)
    p = Preview()
    render(target, weather, p)
    p.save(out)
    print(f"saved {out}")

    if len(sys.argv) > 3 and sys.argv[3] == "config":
        render_config_screen(p)
        p.save(ROOT / "preview_config.png")
        print("saved preview_config.png")


if __name__ == "__main__":
    main()
