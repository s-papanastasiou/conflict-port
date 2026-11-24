// Software model of CONFLICT's EGA driver (code segment 1f39 of Conflict.exe).
//
// The original draws in EGA mode 10h (640x350, 16 colours) through the graphics controller.
// We model video memory the way the driver addresses it: a 64 KB window of byte offsets, each
// byte covering 8 horizontal pixels. The visible page starts at offset 0 (segment A000), the
// back buffer at offset 28000 (segment A6D6), so drawing below row 349 spills into the back
// buffer exactly as on the real card.
//
// Every public routine names the driver entry point it reproduces.
#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace conflict::ega {

constexpr int kWidth = 640;
constexpr int kHeight = 350;
constexpr int kRowBytes = 80;
constexpr uint16_t kFrontBase = 0x0000;           // DS:836c = A000
constexpr uint16_t kBackBase = kRowBytes * kHeight;  // DS:836e = A6D6 -> offset 0x6d60

class Ega {
public:
    Ega();

    // Colours used by the drawing routines (low byte of the globals; only bits 0-3 reach VRAM).
    uint8_t fg = 0;  // DS:b2a2
    uint8_t bg = 0;  // DS:b2a0

    // Fonts live in the game's 64 KB data segment and are addressed by 16-bit offsets, exactly as
    // the driver does: glyph for char c is at font_mem[small_font + c * 9] (DS:b290) or
    // font_mem[large_font + c * 36] (DS:b292; the game biases that pointer by -0x900 itself).
    const uint8_t* font_mem = nullptr;  // 64 KB
    uint16_t small_font = 0;
    uint16_t large_font = 0;

    // Fix for original bug #6 (docs/ORIGINAL_BUGS.md): measure the last glyph's real width.
    bool fix_text_width = false;

    // Pen position (DS:8370 byte offset, DS:8372 bit within byte).
    uint16_t pen_off = 0;
    uint8_t pen_bit = 0;

    void set_mode();                                   // 1f39:0771 (int 10h AX=0010h clears VRAM)
    void moveto(int x, int y);                         // 1f39:0801
    void plot(int x, int y);                           // 1f39:0813
    void hspan(int x0, int x1, int y, int colour_flag);  // 1f39:0827 (flag 0 -> colour 0, else fg)
    void vline(int y0, int y1, int x, int colour_flag);  // 1f39:0858 (flag 0 -> colour 0, else fg)
    void fill_rect(int x0, int x1, int y0, int y1);      // 1f39:091e (rows y0..y1 inclusive, fg)
    void restore_rect(int x0, int x1, int y0, int y1);   // 1f39:06dd (back buffer -> screen)
    void recolour(int x, int y, int w, int h, int from, int to);  // 1f39:0889
    void text_small(std::string_view s);               // 1f39:08e4
    void text_large(std::string_view s);               // 1f39:08f8
    int text_width(std::string_view s) const;          // 1f39:09e9
    void draw_icon(const uint8_t* src);                // 1f39:07df (32x32, fg/bg, at pen byte)
    void draw_picture(std::span<const uint8_t> pic);   // 1f39:0746 (160x200 1bpp at pen byte)
    void draw_map(std::span<const uint8_t> map);       // 1f39:0937 (RLE map -> back, copy to front)

    // Colour index (0-15) of a visible pixel.
    uint8_t pixel(int x, int y) const { return vram_[(size_t(kFrontBase) + y * kRowBytes) * 8 + x]; }
    // Visible page as 640x350 colour indices.
    void copy_front(std::span<uint8_t, kWidth * kHeight> out) const;

private:
    // One entry per pixel: index = byte_offset * 8 + bit (bit 0 = leftmost pixel of the byte).
    std::vector<uint8_t> vram_;

    void write_masked(uint16_t off, uint8_t mask, uint8_t colour);
    uint8_t compare_mask(uint16_t off, uint8_t colour) const;  // read mode 1
    void span_raw(uint16_t base, int x0, int x1, int y, uint8_t colour);  // 1f39:035e / 03d7 core
    void draw_glyph(uint16_t src, int nrows, int bytes_per_row);           // 1f39:048e / 04fc
    uint8_t font_byte(uint16_t off) const { return font_mem[off]; }
    static uint8_t map_char(uint8_t c);                                    // 1f39:0659
};

// Default mode 10h palette as 0xRRGGBB.
extern const std::array<uint32_t, 16> kPalette;

}  // namespace conflict::ega
