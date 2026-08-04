#pragma once

#include <stdint.h>

struct FontInfo {
  const uint8_t *bits;
  const uint32_t *codes;
  const uint16_t *offsets;
  uint16_t count;
  uint8_t size;  // square cell: size x size
};

#include "font_data.h"

// Binary search a code point in a font. Returns a pointer to the packed
// glyph bitmap (size rows, (size + 7) / 8 bytes per row) or nullptr.
inline const uint8_t *fontFindGlyph(const FontInfo &font, uint32_t code) {
  int lo = 0;
  int hi = static_cast<int>(font.count) - 1;
  while (lo <= hi) {
    const int mid = (lo + hi) / 2;
    if (font.codes[mid] == code) {
      return font.bits + font.offsets[mid];
    }
    if (font.codes[mid] < code) {
      lo = mid + 1;
    } else {
      hi = mid - 1;
    }
  }
  return nullptr;
}
