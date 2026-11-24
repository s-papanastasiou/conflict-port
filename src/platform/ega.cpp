#include "platform/ega.hpp"

#include <algorithm>
#include <cstdio>

namespace conflict::ega {

const std::array<uint32_t, 16> kPalette = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA, 0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF, 0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
};

namespace {
// Driver tables in DGROUP.
constexpr uint8_t kLeftMask[8] = {0xff, 0x7f, 0x3f, 0x1f, 0x0f, 0x07, 0x03, 0x01};   // DS:837c
constexpr uint8_t kRightMask[8] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};  // DS:8384

// Row table DS:83a2 holds y*80 for y in 0..349. Rows outside that range read past the table in
// the original; we extend it linearly instead.
inline uint16_t row_offset(int y) { return uint16_t(y * kRowBytes); }

void warn_once(const char* what) {
    static bool warned = false;
    if (!warned) {
        std::fprintf(stderr, "ega: %s (original behaviour would be undefined here)\n", what);
        warned = true;
    }
}
}  // namespace

Ega::Ega() : vram_(65536 * 8, 0) {}

void Ega::set_mode() { std::fill(vram_.begin(), vram_.end(), 0); }

void Ega::write_masked(uint16_t off, uint8_t mask, uint8_t colour) {
    uint8_t* p = &vram_[size_t(off) * 8];
    for (int k = 0; k < 8; ++k)
        if (mask & (0x80 >> k)) p[k] = colour & 0x0f;
}

uint8_t Ega::compare_mask(uint16_t off, uint8_t colour) const {
    const uint8_t* p = &vram_[size_t(off) * 8];
    uint8_t m = 0;
    for (int k = 0; k < 8; ++k)
        if (p[k] == (colour & 0x0f)) m |= 0x80 >> k;
    return m;
}

void Ega::copy_front(std::span<uint8_t, kWidth * kHeight> out) const {
    std::copy_n(vram_.begin() + size_t(kFrontBase) * 8, out.size(), out.begin());
}

// 1f39:0302 — position the pen. Note the driver swaps the arguments: first is x, second is y.
void Ega::moveto(int x, int y) {
    pen_off = uint16_t(row_offset(y) + (uint16_t(x) >> 3));
    pen_bit = uint8_t(x & 7);
}

// 1f39:032c — set one pixel to fg (write mode 2 with a single-bit mask).
void Ega::plot(int x, int y) {
    moveto(x, y);
    write_masked(pen_off, uint8_t(0x80 >> pen_bit), fg);
}

// Shared span core of 1f39:035e (front page) and 1f39:03d7 (back buffer): fill x0..x1 inclusive
// on one row with set/reset colour.
void Ega::span_raw(uint16_t base, int x0, int x1, int y, uint8_t colour) {
    uint16_t di = uint16_t(base + row_offset(y) + (uint16_t(x0) >> 3));
    uint8_t lm = kLeftMask[x0 & 7];
    uint8_t rm = kRightMask[x1 & 7];
    int count = int(uint16_t(x1) >> 3) - int(uint16_t(x0) >> 3);
    if (count < 0) {
        warn_once("span with x1 < x0 across a byte boundary");
        return;
    }
    if (!(lm & 0x80)) {
        if (count == 0) {
            write_masked(di, lm & rm, colour);
            return;
        }
        write_masked(di++, lm, colour);
        --count;
    }
    for (; count > 0; --count) write_masked(di++, 0xff, colour);
    write_masked(di, rm, colour);
}

// 1f39:0827 -> 035e
void Ega::hspan(int x0, int x1, int y, int colour_flag) {
    moveto(x0, y);
    span_raw(kFrontBase, x0, x1, y, colour_flag ? fg : 0);
}

// 1f39:0858 -> 044f
void Ega::vline(int y0, int y1, int x, int colour_flag) {
    int count = y1 - y0 + 1;
    moveto(x, y0);
    uint8_t mask = uint8_t(0x80 >> pen_bit);
    uint8_t colour = colour_flag ? fg : 0;
    uint16_t di = pen_off;
    for (uint16_t n = uint16_t(count); n != 0; --n) {  // `loop` with cx = count
        write_masked(di, mask, colour);
        di = uint16_t(di + kRowBytes);
    }
}

// 1f39:091e -> 05e0: do { span(x0, x1, y) } while (++y != y1 + 1)
void Ega::fill_rect(int x0, int x1, int y0, int y1) {
    if (y0 > y1) {
        warn_once("fill_rect with y0 > y1");
        return;
    }
    for (int y = y0; y <= y1; ++y) hspan(x0, x1, y, 1);
}

// 1f39:06dd — copy a byte-aligned rectangle from the back buffer to the screen (write mode 1).
// The driver widens the rectangle by 1 pixel left/top and 3 pixels right/bottom.
void Ega::restore_rect(int x0, int x1, int y0, int y1) {
    x0 -= 1;
    x1 += 3;
    y0 -= 1;
    y1 += 3;
    moveto(x0, y0);
    uint16_t off = pen_off;
    int rows = y1 - y0 + 1;
    int extra = (x0 & 7) ? 1 : 0;
    int bytes = (int(uint16_t(x1 - x0 - 1)) >> 3) + 1 + extra;
    for (int r = 0; r < rows; ++r) {
        for (int b = 0; b < bytes; ++b) {
            uint16_t o = uint16_t(off + b);
            std::copy_n(&vram_[size_t(uint16_t(kBackBase + o)) * 8], 8, &vram_[size_t(o) * 8]);
        }
        off = uint16_t(off + kRowBytes);
    }
}

// 1f39:0889 — in a byte-aligned rectangle, repaint pixels of colour `from` with colour `to`.
void Ega::recolour(int x, int y, int w, int h, int from, int to) {
    moveto(x, y);
    int bytes = (w >> 3);
    uint16_t row = pen_off;
    for (int r = 0; r < h; ++r) {
        for (int b = 0; b < bytes; ++b) {
            uint16_t o = uint16_t(row + b);
            write_masked(o, compare_mask(o, uint8_t(from)), uint8_t(to));
        }
        row = uint16_t(row + kRowBytes);
    }
}

// 1f39:0659 — characters below '0' are remapped to spare glyph slots.
uint8_t Ega::map_char(uint8_t c) {
    if (c >= 0x30) return c;
    switch (c) {
        case ' ': return 0x40;
        case '.': return 0x60;
        case '-': return 0x5c;
        case '!': return 0x5e;
        case '%': return 0x5b;
        case '$': return 0x5d;
        default: return 0x5f;
    }
}

// 1f39:048e (1 byte per row) / 1f39:04fc (2 bytes per row): transparent glyph in fg at the pen,
// shifted right by the pen's bit offset.
void Ega::draw_glyph(uint16_t src, int nrows, int bytes_per_row) {
    int shift = pen_bit;
    uint16_t di = pen_off;
    for (int r = 0; r < nrows; ++r) {
        uint32_t bits = 0;
        for (int b = 0; b < bytes_per_row; ++b)
            bits = (bits << 8) | font_byte(uint16_t(src + r * bytes_per_row + b));
        int out_bytes = bytes_per_row + (shift ? 1 : 0);
        bits = (bits << 8) >> shift;  // align so the top output byte is the first screen byte
        for (int b = 0; b < out_bytes; ++b) {
            uint8_t m = uint8_t(bits >> (8 * (bytes_per_row - b)));
            write_masked(uint16_t(di + b), m, fg);
        }
        di = uint16_t(di + kRowBytes);
    }
}

// 1f39:08e4 -> 0584: 9-byte glyphs (8 rows + advance width).
void Ega::text_small(std::string_view s) {
    for (char ch : s) {
        if (ch == 0) break;
        uint16_t g = uint16_t(small_font + map_char(uint8_t(ch)) * 9);
        draw_glyph(g, 8, 1);
        int adv = font_byte(uint16_t(g + 8)) + pen_bit;
        pen_off = uint16_t(pen_off + (uint8_t(adv) >> 3));
        pen_bit = uint8_t(adv & 7);
    }
}

// 1f39:08f8 -> 05f2: 36-byte glyphs (16 rows x 2 bytes + advance*8 + 3 unused).
void Ega::text_large(std::string_view s) {
    for (char ch : s) {
        if (ch == 0) break;
        uint16_t g = uint16_t(large_font + map_char(uint8_t(ch)) * 36);
        draw_glyph(g, 16, 2);
        int adv = (font_byte(uint16_t(g + 32)) >> 3) + pen_bit;
        pen_off = uint16_t(pen_off + (uint8_t(adv) >> 3));
        pen_bit = uint8_t(adv & 7);
    }
}

// 1f39:09e9 — sum of advance widths of all characters but the last, plus 8 if the last glyph has
// any pixel in its rightmost column, else 0. (The original's column scan re-tests the same bit
// each iteration, so this is the actual behaviour; keep it.)
int Ega::text_width(std::string_view s) const {
    int width = 0;
    size_t i = 0;
    for (;;) {
        uint8_t c = map_char(i < s.size() ? uint8_t(s[i]) : 0);
        ++i;
        bool last = i >= s.size() || s[i] == 0;
        uint16_t g = uint16_t(small_font + c * 9);
        if (!last) {
            width += font_byte(uint16_t(g + 8));
            continue;
        }
        uint8_t ored = 0;
        for (int r = 0; r < 8; ++r) ored |= font_byte(uint16_t(g + r));
        if (fix_text_width) {  // bug #6: width up to the rightmost used column
            int empty = 0;
            while (empty < 8 && !(ored & (1 << empty))) ++empty;
            return width + (8 - empty);
        }
        return width + ((ored & 1) ? 8 : 0);
    }
}

// 1f39:07df -> 028e: 32x32 opaque icon, 4 bytes per row, at the pen's byte (bit ignored).
void Ega::draw_icon(const uint8_t* src) {
    uint16_t di = pen_off;
    for (int r = 0; r < 32; ++r) {
        for (int b = 0; b < 4; ++b) {
            uint8_t m = *src++;
            write_masked(uint16_t(di + b), m, fg);
            write_masked(uint16_t(di + b), uint8_t(~m), bg);
        }
        di = uint16_t(di + kRowBytes);
    }
}

// 1f39:0746 -> 0256: 160x200 1bpp picture written to all planes at the pen's byte:
// set bits become colour 15, clear bits colour 0.
void Ega::draw_picture(std::span<const uint8_t> pic) {
    if (pic.empty()) return;  // segment table entry 0: picture not loaded
    uint16_t di = pen_off;
    size_t si = 0;
    for (int r = 0; r < 200; ++r) {
        for (int b = 0; b < 20; ++b) {
            uint8_t v = si < pic.size() ? pic[si] : 0;
            ++si;
            write_masked(uint16_t(di + b), v, 0x0f);
            write_masked(uint16_t(di + b), uint8_t(~v), 0x00);
        }
        di = uint16_t(di + kRowBytes);
    }
}

// 1f39:0937 — decode a .MP map into the back buffer, then copy the whole page to the screen.
// Records: [colour u8][x_end u16][y u16]. Rows are stored bottom-up (03d7 draws at 0x15d - y).
void Ega::draw_map(std::span<const uint8_t> map) {
    size_t si = 0;
    int cur_y = 0, cur_x = 0;  // DS:839c, DS:839a
    auto u16 = [&](size_t at) { return int(map[at] | (map[at + 1] << 8)); };
    auto back_span = [&](int x0, int x1, int y, uint8_t col) {
        span_raw(kBackBase, x0, x1, 0x15d - y, col);
    };
    while (si + 5 <= map.size()) {
        uint8_t colour = map[si];
        int x_end = u16(si + 1), y = u16(si + 3);
        si += 5;
        bool done = false;
        for (;;) {
            if (cur_y > 0x15d) { done = true; break; }
            if (cur_y != y) {
                back_span(cur_x, 0x27f, cur_y, colour);
                ++cur_y;
                cur_x = 0;
                continue;
            }
            back_span(cur_x, x_end - 1, cur_y, colour);
            cur_x = x_end;
            if (y == 0x15d && x_end == 0x27f) done = true;
            break;
        }
        if (done) break;
    }
    // rep movsb of 0x6d60 bytes in write mode 1: back buffer -> visible page
    std::copy_n(&vram_[size_t(kBackBase) * 8], size_t(kBackBase) * 8, &vram_[size_t(kFrontBase) * 8]);
}

}  // namespace conflict::ega
