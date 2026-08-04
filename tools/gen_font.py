"""Generate the bitmap CJK/ASCII fonts used by the calendar firmware.

Glyphs are rendered from a TrueType font (default: Windows SimHei), binarized
and packed into C arrays written to src/font_data.h. Every non-ASCII
character that appears in src/*.{cpp,h} is included automatically, so the
generated font always covers whatever the firmware draws.

Usage:
    python tools/gen_font.py [path-to-ttf]
"""

import io
import re
import sys
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parent.parent
SRC = ROOT / "src"
OUT = SRC / "font_data.h"
DEFAULT_FONT = r"C:\Windows\Fonts\simhei.ttf"

ASCII_RANGE = set(range(0x20, 0x7F))  # printable ASCII (keeps ' ' as blank)


def collect_characters():
    """Return the sorted set of code points needed by the firmware."""
    chars = set(ASCII_RANGE)
    for path in SRC.glob("*.cpp"):
        for line in path.read_text(encoding="utf-8").splitlines():
            stripped = line.lstrip()
            if stripped.startswith("//") or stripped.startswith("*"):
                continue
            for ch in line:
                if ord(ch) > 0x7F:
                    chars.add(ord(ch))
    # Characters that only appear as runtime-composed strings.
    for ch in "农历星期天气温湿度最高最低°%℃":
        chars.add(ord(ch))
    return sorted(chars)


def is_fullwidth(code):
    """True for CJK / full-width code points that should fill most of a cell."""
    return (0x2E80 <= code <= 0x9FFF or 0xF900 <= code <= 0xFAFF or
            0xFF00 <= code <= 0xFFEF or 0x3000 <= code <= 0x303F or
            code in (0x2018, 0x2019, 0x201C, 0x201D, 0x2026, 0x00B7))


def target_ink_height(code, size):
    """Ink height (px) the glyph should occupy inside a `size`x`size` cell.

    Small punctuation ('.', ':', the degree sign, ...) must keep its
    proportional size; scaling its ink bounding box to fill the whole cell
    turns a period into a solid square and a degree sign into a full-size
    ring that reads as "O".
    """
    if is_fullwidth(code):
        return max(2, round(size * 0.90))
    if code in (0x2E, 0x2C, 0x3001):  # . , 、
        return max(2, round(size * 0.28))
    if code in (0x3A, 0x3B, 0x00B0, 0x2103, 0x2018, 0x2019, 0x201C, 0x201D):
        # : ; ° ℃ and small quotation marks
        return max(2, round(size * 0.52))
    return max(2, round(size * 0.72))


def vertical_offset(code, size, nh):
    """Where the scaled glyph should sit inside the cell.

    CJK glyphs are centered; ASCII text sits on a shared baseline; the
    degree sign is treated as a superscript near the top of the line.
    """
    if is_fullwidth(code):
        return (size - nh) // 2
    if code in (0x00B0, 0x2103):  # degree / ℃: superscript
        return max(0, round(size * 0.08))
    return max(0, size - nh - round(size * 0.10))


def render_glyph(font, code, size, scale):
    """Render one character into a `size`x`size` binary bitmap.

    Windows CJK fonts report narrow horizontal metrics through FreeType, so
    simply downscaling leaves the ink far too small. The glyph's ink bounding
    box is cropped and scaled to a class-dependent target height (CJK glyphs
    fill most of the cell, ASCII text about 70%, punctuation stays small), so
    proportions survive while the stroke weight matches classic 16x16-style
    bitmap fonts.
    """
    rs = size * scale
    img = Image.new("L", (rs, rs), 0)
    draw = ImageDraw.Draw(img)
    draw.text((0, 0), chr(code), font=font, fill=255)

    pixels = img.load()
    min_x, min_y, max_x, max_y = rs, rs, -1, -1
    for y in range(rs):
        for x in range(rs):
            if pixels[x, y] > 16:
                if x < min_x:
                    min_x = x
                if x > max_x:
                    max_x = x
                if y < min_y:
                    min_y = y
                if y > max_y:
                    max_y = y
    if max_x < min_x:  # blank glyph (space)
        return [0] * size

    crop = img.crop((min_x, min_y, max_x + 1, max_y + 1))
    cw = max_x - min_x + 1
    ch = max_y - min_y + 1
    scale_to = target_ink_height(code, size) / max(cw, ch)
    nw = max(1, round(cw * scale_to))
    nh = max(1, round(ch * scale_to))
    small = crop.resize((nw, nh), Image.LANCZOS)
    out = Image.new("L", (size, size), 0)
    out.paste(small, ((size - nw) // 2, vertical_offset(code, size, nh)))
    out_px = out.load()

    rows = []
    for y in range(size):
        row = 0
        for x in range(size):
            if out_px[x, y] > 127:
                row |= 1 << (size - 1 - x)
        rows.append(row)
    return rows


def pack_header(name, size, codes, rows_by_code, row_stride_bits):
    row_bytes = (row_stride_bits + 7) // 8
    lines = []
    bits = []
    offsets = []
    offset = 0
    low_bits = row_stride_bits - 8 if row_stride_bits > 8 else row_stride_bits
    for code in codes:
        offsets.append(offset)
        rows = rows_by_code[code]
        for row in rows:
            bits.append((row >> (row_stride_bits - 8)) & 0xFF if row_stride_bits >= 8 else row & 0xFF)
            if row_stride_bits > 8:
                # For a non-byte-aligned stride (e.g. 12 bits) the remainder
                # must be placed in the MSBs of the low byte; `row & 0xFF`
                # would reuse bits already stored in the high byte and double
                # every overlapping stroke on the target.
                bits.append(((row & ((1 << low_bits) - 1)) << (8 - low_bits)) & 0xFF)
        offset += len(rows) * row_bytes

    lines.append(f"static const uint8_t k{name}Bits[] = {{")
    for i in range(0, len(bits), 16):
        chunk = ", ".join("0x%02X" % b for b in bits[i : i + 16])
        lines.append("  " + chunk + ",")
    lines.append("};")
    lines.append("")
    lines.append(f"static const uint32_t k{name}Codes[] = {{")
    for i in range(0, len(codes), 12):
        chunk = ", ".join("0x%04X" % c for c in codes[i : i + 12])
        lines.append("  " + chunk + ",")
    lines.append("};")
    lines.append("")
    lines.append(f"static const uint16_t k{name}Offsets[] = {{")
    for i in range(0, len(offsets), 12):
        chunk = ", ".join("%d" % o for o in offsets[i : i + 12])
        lines.append("  " + chunk + ",")
    lines.append("};")
    lines.append("")
    return "\n".join(lines), len(codes)


def main():
    ttf = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_FONT
    if not Path(ttf).exists():
        sys.exit(f"font not found: {ttf}")

    codes = collect_characters()
    print(f"characters: {len(codes)}")

    sections = ["// Auto-generated by tools/gen_font.py - do not edit by hand.\n", "#pragma once", "",
                "#include <stdint.h>", ""]

    # The 12px font renders from an 8x source so thin strokes (degree sign,
    # periods, colons) survive the downscale without breaking apart.
    for size, scale in ((16, 4), (12, 8)):
        ttf_font = ImageFont.truetype(ttf, size * scale)
        rows_by_code = {c: render_glyph(ttf_font, c, size, scale) for c in codes}
        name = f"Font{size}"
        header, count = pack_header(name, size, codes, rows_by_code, size)
        sections.append(f"// {size}x{size} font, {count} glyphs")
        sections.append(header)
        sections.append(f"const FontInfo kFont{size} = {{ k{name}Bits, k{name}Codes, k{name}Offsets, {count}, {size} }};")
        sections.append("")

    OUT.write_text("\n".join(sections), encoding="utf-8")
    print(f"wrote {OUT} ({OUT.stat().st_size} bytes)")


if __name__ == "__main__":
    main()
