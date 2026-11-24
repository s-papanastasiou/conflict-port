#include "game/newspaper.hpp"

#include "game/crt.hpp"
#include "game/globals.hpp"
#include "game/sys.hpp"

namespace conflict::newspaper {

using namespace sys;
using namespace g;
using crt::random;

// 1000:14bc — y values are in 200-line units (scaled by 1.75); each paper is offset 16 px right
// and 28 units down from the previous one; the newest gets filler text and a box.
void show() {
    fg_colour = 7;
    gfx_fill_rect(0, 0x27f, 0, 0x15d);
    fg_colour = 0;
    int first = headline_count < 6 ? 0 : headline_count - 5;
    for (int i = 0; i < headline_count - first; ++i) {
        int height = 0x69 - random(0xf);
        bool newest = (headline_count - 1) - first == i;
        if (newest) height = 0x5a;
        int paper = random(9) - 1;
        int top = i * 0x1c;
        int x = i * 0x10;
        double top_y = double(top) * 1.75;
        // Sheet: top edge, white body, masthead rule, side edges.
        gfx_fill_rect_row(x + 5, x + 0x1c2, int(top_y + 5.0), 0);
        fg_colour = 15;
        double bottom = double(top + height) * 1.75 - 1.0;
        if (348.0 <= bottom) gfx_fill_rect(x + 6, x + 0x1c1, int(double(top) * 1.75 + 6.0), 0x15c);
        else gfx_fill_rect(x + 6, x + 0x1c1, int(top_y + 6.0), int(bottom));
        fg_colour = 0;
        gfx_fill_rect_row(x + 0xf, x + 0x1b8, int(top_y + 30.0), 0);
        if (top + height < 0xc4) {
            int yb = int(double(top + height) * 1.75);
            gfx_fill_rect_row(x + 5, x + 0x15, yb, 0);
            int y5 = int(top_y + 5.0);
            gfx_vline(y5, yb, x + 5, 0);
            int y_next = int(double(top + 0x1c) * 1.75 + 5.0);
            gfx_vline(y5, y_next, x + 0x1c2, 0);
            gfx_vline(int(top_y + 6.0), y_next, x + 0x1c3, 0);
        } else {
            int y5 = int(top_y + 5.0);
            gfx_vline(y5, 0x15a, x + 5, 0);
            gfx_vline(y5, 0x15a, x + 0x1c2, 0);
            gfx_vline(int(top_y + 6.0), 0x15a, x + 0x1c3, 0);
        }
        if (newest) {
            if (i == 4) {
                gfx_vline(0xc9, 0x15a, 0x202, 0);
                gfx_vline(0xca, 0x15a, 0x203, 0);
            } else {
                int yb = int(double(top + height) * 1.75);
                gfx_fill_rect_row(x + 5, x + 0x1c2, yb, 0);
                gfx_vline(int(top_y + 5.0), yb, x + 0x1c2, 0);
                gfx_vline(int(top_y + 6.0), yb, x + 0x1c3, 0);
            }
            // Columns of greeked text drawn with EXTRA-font glyph strings.
            font_small = font_extra;
            int k = 0;
            for (int col = 0; col < 0x1a9; col += 0x55) {
                k = 0;
                while (k < 0x36 && double(top + k) * 1.75 + 53.0 < 339.5) {
                    double d = double(top + k) * 1.75;
                    gfx_moveto(x + 0x14 + col, int(d + 54.0));
                    gfx_text_small(ds.str(0x1498));
                    gfx_moveto(x + 0x14 + col, int(d + 63.0));
                    gfx_text_small(ds.str(0x14a2));
                    k += 0xb;
                }
            }
            font_small = font_etech;
            if (top + k + 0x2a < 0xc2) filler_box(x, top);
        }
        // Masthead: newspaper name in one of the two large fonts.
        gfx_moveto(x + (0x68 - int16_t(word_at(uint16_t(0x1486 + 2 * paper)))), int(double(top) * 1.75 + 10.0));
        uint16_t font = font_xini;
        if (paper != 0 && paper != 3) font = font_xguard;
        font_large = uint16_t(font - 0x900);
        gfx_text_large(ds.str(word_at(uint16_t(0x12a0 + 2 * paper))));
        // Headline, centred; the newest leaves room for the BRIEFLY box when short enough.
        int y = int(double(top) * 1.75 + 41.0);
        int idx = i + first;
        int w = gfx_text_width(ds.str(headline_ptr[idx]));
        gfx_moveto(x + ((0x1bd - w) >> 1) + 5, y);
        if (newest && gfx_text_width(ds.str(headline_ptr[idx])) < 0x154) {
            briefly_box(x, top);
            w = gfx_text_width(ds.str(headline_ptr[idx]));
            gfx_moveto(x + ((0x168 - w) >> 1) + 0x5a, y);
        }
        gfx_text_small(ds.str(headline_ptr[i + first]));
    }
}

// 1000:1f6f
void briefly_box(int x, int top) {
    int y = int(double(top + 0x16) * 1.75);
    gfx_fill_rect_row(x + 0xf, x + 100, y, 0);
    int y1 = int(double(top + 0x5a) * 1.75);
    gfx_vline(y, y1, x + 0xf, 0);
    gfx_vline(y, y1, x + 100, 0);
    gfx_moveto(x + 0x1c, int(double(top + 0x17) * 1.75));
    font_small = font_extra;
    gfx_text_small(ds.str(0x14ac));  // "BRIEFLY"
    font_small = font_etech;
}

// 1000:1d09 — a narrow (1 in 3) or wide white box with a random one-liner.
void filler_box(int x, int top) {
    int which = random(9);
    bool narrow = random(3) == 1;
    int left = narrow ? 0x113 : 0xbe;
    fg_colour = 15;
    gfx_fill_rect(x + left, x + 0x1bd, int(double(top + 0x34) * 1.75), int(double(top + 0x46) * 1.75));
    fg_colour = 0;
    int y = int(double(top + 0x37) * 1.75);
    gfx_fill_rect_row(x + left - 6, x + 0x1b9, y, 0);
    gfx_vline(y, int(double(top + 0x5a) * 1.75), x + left - 6, 0);
    gfx_moveto(x + left + 2, int(double(top + 0x3c) * 1.75));
    gfx_text_small(ds.str(word_at(uint16_t((narrow ? 0x1364 : 0x1474) + 2 * (which - 1)))));
}

}  // namespace conflict::newspaper
