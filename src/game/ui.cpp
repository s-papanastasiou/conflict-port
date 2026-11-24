#include "game/ui.hpp"

#include <cstring>

#include "game/crt.hpp"
#include "game/globals.hpp"
#include "game/save.hpp"
#include "game/sys.hpp"
#include "game/reports.hpp"

namespace conflict::ui {

using namespace sys;

// 1000:00b3 — panel in bg colour with a black frame and a 2-pixel drop shadow (right, bottom).
void draw_panel(int x0, int y0, int x1, int y1) {
    uint16_t saved = g::fg_colour;
    g::fg_colour = g::bg_colour;
    double bottom = ega_rows(y1);
    double top = ega_rows(y0);
    int top_row = int(top);
    gfx_fill_rect(x0, x1, top_row, int(bottom));
    top = top - 1.0;
    int frame_top = int(top);
    int right = x1 + 1;
    int left = x0 - 1;
    g::fg_colour = saved;
    gfx_fill_rect_row(left, right, frame_top, 0);
    int frame_bottom = int(bottom + 1.0);
    gfx_fill_rect_row(left, right, frame_bottom, 0);
    int shadow_right = x1 + 2;
    gfx_fill_rect_row(x0, shadow_right, int(bottom + 2.0), 0);
    int shadow_bottom = int(bottom + 3.0);
    gfx_fill_rect_row(x0, shadow_right, shadow_bottom, 0);
    gfx_vline(frame_top, frame_bottom, left, 0);
    gfx_vline(frame_top, frame_bottom, right, 0);
    gfx_vline(frame_top, frame_bottom, shadow_right, 0);
    gfx_vline(top_row, shadow_bottom, x1 + 3, 0);
}

// 1000:0286 — panel with an underlined title centred on its top line.
void draw_titled_panel(int x0, int y0, int x1, int y1, const char* title) {
    draw_panel(x0, y0, x1, y1);
    int x = x0;
    int w = gfx_text_width(title);
    if (w < x1 - x0) x += (x1 - x0 - w) >> 1;
    gfx_fill_rect_row(x - 1, x + w + 1, int(ega_rows(y0) + 9.0), 0);
    gfx_moveto(x, int(ega_rows(y0) + 1.0));
    gfx_text_small(title);
}

namespace {
// Word wrap shared by text_window and menu_add_item: takes the longest prefix of *p that fits
// `width` pixels, breaking at a space, copies it to buf and advances *p past it.
void wrap_line(const char*& p, char* buf, int width) {
    int n = width / 7 + 10;
    while (int(std::strlen(p)) < n) --n;
    std::strncpy(buf, p, size_t(n));
    buf[n] = '\0';
    p += int(int8_t(n)) - 1;
    bool cut = false;
    while (!(gfx_text_width(buf) <= width && cut)) {
        while (*--p != ' ') --n;
        buf[n - 1] = '\0';
        cut = true;
        --n;
    }
    ++p;
}
}  // namespace

// 1000:0a7c — titled panel holding `text` word-wrapped to the panel width, followed by
// `lines`, a list of '|'-terminated lines printed as-is (the '|' is printed too; it is blank in
// the font).
void text_window(int x0, int y0, int x1, int y1, const char* title, const char* text,
                 const char* lines) {
    int line_y = 0;
    bool panel_drawn = false;
    const char* p = text;
    int width = (x1 - x0) - 15;
    char buf[80];
    auto at_line = [&] { gfx_moveto(x0 + 15, int(ega_rows(line_y) + (ega_rows(y0) + 16.0))); };
    while ((*p == '\0' ? 0 : gfx_text_width(p)) > width) {
        wrap_line(p, buf, width);
        if (!panel_drawn) {
            draw_titled_panel(x0, y0, x1, y1, title);
            panel_drawn = true;
        }
        at_line();
        gfx_text_small(buf);
        line_y += 10;
    }
    if (!panel_drawn) draw_titled_panel(x0, y0, x1, y1, title);
    at_line();
    gfx_text_small(p);
    const char* start = lines;
    for (;;) {
        line_y += 10;
        if (*lines == '\0') break;
        int n = 1;
        const char* next;
        // PORT FIX: the original scans on (past the string's end if need be) for the next '|'
        // and strncpy()s everything up to it into its 80-byte stack buffer. Text that does not
        // end in '|' (e.g. the summit's war-duration line) can overflow it and crash the
        // original. We keep the scan, but stop at the end of DGROUP and never copy more than
        // the buffer holds.
        while (next = lines + 1, *lines != '|') {
            ++n;
            lines = next;
            if (n >= int(sizeof buf) - 1 || (ds.contains(start) && !ds.contains(next))) break;
        }
        if (n > int(sizeof buf) - 1) n = int(sizeof buf) - 1;
        std::strncpy(buf, start, size_t(n));
        start = next;
        buf[n] = '\0';
        at_line();
        gfx_text_small(buf);
        lines = next;
    }
}

// 1000:0e13 — one menu item (word-wrapped), followed by an empty checkbox. Records the item's
// last text line in menu_item_line[++menu_item_count].
void menu_add_item(int x, int y, int width, const char* text) {
    const char* p = text;
    char buf[80];
    if (g::menu_line == 1) g::menu_item_count = 0;
    while (gfx_text_width(p) > width) {
        wrap_line(p, buf, width);
        double base = ega_rows(y) + 13.0;
        gfx_moveto(x + 10, int(ega_rows(g::menu_line << 3) + base));
        ++g::menu_line;
        gfx_text_small(buf);
    }
    double base = ega_rows(y) + 13.0;
    gfx_moveto(x + 10, int(ega_rows(g::menu_line << 3) + base));
    ++g::menu_item_count;
    g::menu_item_line[g::menu_item_count] = g::menu_line;
    ++g::menu_line;
    gfx_text_small(p);
    gfx_moveto(x + width + 5, int(ega_rows((g::menu_line - 1) * 8) + base));
    g::font_small = g::font_extra;
    gfx_text_small(ds.str(0x11ed));  // "01": empty checkbox
    g::font_small = g::font_etech;
}

// 1000:0d24 — menu from '?'-separated text: title, then items. A title starting with ' ' gets
// a name from nation_names[arg] written over its start. Returns the chosen item (1-based).
int menu_popup(int x0, int y0, int x1, int y1, const char* text, int arg) {
    g::menu_line = 0;
    const char* item = text;
    int width = (x1 - x0) - 25;
    char buf[80];
    for (;;) {
        int n = 0;
        const char* q = item;
        while (*q != '?') {
            ++n;
            ++q;
        }
        std::strncpy(buf, item, size_t(n));
        buf[n] = '\0';
        if (g::menu_line == 0 && buf[0] == ' ') {
            str_overlay(buf, ds.str(g::nation_adjective[arg]), 1);
            squeeze_spaces(buf);
        }
        if (g::menu_line == 0) draw_titled_panel(x0, y0, x1, y1, buf);
        else menu_add_item(x0, y0, width, buf);
        if (q[1] == '\0') break;
        ++g::menu_line;
        item += int(int8_t(n)) + 1;
    }
    return menu_select(x1, y0);
}

// 1000:1044 — move the selection with Up/Down/Home/End, confirm with Enter. Shift-E restarts
// (the "I quit" menu), Shift-S saves where menu_context == 5, Shift-B shows the briefing where
// menu_context == 1. The chosen checkbox gets an animated tick.
int menu_select(int x, int y) {
    int item = 1;
    if (g::menu_context != 0) item = g::menu_context_pos[g::menu_context - 1];
    g::font_small = g::font_extra;
    gfx_moveto(x - 20, int(double(g::menu_item_line[item]) * 14.0 + (ega_rows(y) + 13.0)));
    int line = g::menu_item_line[item];
    gfx_text_small(ds.str(0x120a));  // "23": selected checkbox
    for (;;) {
        line <<= 3;
        uint16_t key = get_key();
        uint16_t fg = g::fg_colour;
        if (key == key::kEnter) {  // clear the box interior, then draw the tick
            g::fg_colour = g::bg_colour;
            for (int r = int(ega_rows(line) + (ega_rows(y) + 15.0));
                 !(ega_rows(line) + (ega_rows(y) + 19.0) <= double(r)); ++r)
                gfx_fill_rect_row(x - 16, x - 8, r, 0xff);
            g::font_small = g::font_etech;
            g::fg_colour = fg;
            if (g::menu_context != 0) {
                g::menu_context_pos[g::menu_context - 1] = int16_t(item);
                if (g::menu_line - 1 == g::menu_item_line[item])
                    g::menu_context_pos[g::menu_context - 1] = 1;
            }
            for (int c = x - 16; c < x - 8; ++c) {
                double dy = double(g::menu_tick_dy[c - x + 16]);
                gfx_plot(c, int(ega_rows(y) + ega_rows(line) + dy));
                delay_loop(3000);
            }
            return item;
        }
        if (key == key::kShiftE) crt::longjmp_restart(999);  // "I quit"
        if (key == key::kShiftS && g::menu_context == 5) break;  // save
        if (key == key::kShiftB && g::menu_context == 1) reports::briefing();
        fg = g::fg_colour;
        if (key == key::kEnd)
            while (g::menu_item_line[item] < g::menu_line - 1) ++item;
        if (key == key::kHome) item = 1;
        if (key == key::kUp && item > 1) --item;
        if (key == key::kDown && g::menu_line - 1 != g::menu_item_line[item]) ++item;
        // Erase the old selection, draw the new one.
        g::fg_colour = g::bg_colour;
        for (int r = int(ega_rows(line) + (ega_rows(y) + 15.0));
             !(ega_rows(line) + (ega_rows(y) + 19.0) <= double(r)); ++r)
            gfx_fill_rect_row(x - 16, x - 8, r, 1);
        gfx_moveto(x - 20, int(double(g::menu_item_line[item]) * 14.0 + (ega_rows(y) + 13.0)));
        g::fg_colour = fg;
        gfx_text_small(ds.str(0x120d));  // "23"
        line = g::menu_item_line[item];
    }
    save::save_game();
    return 1;
}

// 1000:03bd — copy src over dst starting at 1-based position pos (no terminator written).
void str_overlay(char* dst, const char* src, int pos) {
    char* d = dst + pos - 1;
    for (; *src; ++src) *d++ = *src;
}

// 1000:03e5 — collapse runs of spaces to a single space, in place. Also clears DS:874c[100]
// (a side effect of the original that we keep).
void squeeze_spaces(char* s) {
    std::memset(ds.ptr(g::kSqueezeScratch), 0, 100);
    int spaces = 0;
    char* out = s;
    for (; *s; ++s) {
        spaces = (*s == ' ') ? spaces + 1 : 0;
        if (spaces < 2) *out++ = *s;
    }
    *out = '\0';
}

// 1000:b8ab — 32x32 icon; colour depends on which icon it is.
void draw_icon(const char* icon) {
    uint16_t fg = g::fg_colour, bg = g::bg_colour;
    int8_t index = int8_t((ds.off(icon) - g::icons_cur) / 0x80);
    uint16_t colour = 0;
    if (index == 8 || index == 10 || index == 13) colour = 4;      // red
    else if (index == 9 || index == 11 || index == 12) colour = 1;  // blue
    g::fg_colour = colour;
    g::bg_colour = 15;
    gfx_draw_glyph32(icon);
    g::fg_colour = fg;
    g::bg_colour = bg;
}

// 1000:24c2 — the wall of icons behind the title and menus (15 x 8). icon == 99 picks random
// icons 7..12, never the same twice in a row.
void draw_icon_border(int icon) {
    int chosen[120];
    // The original compares the first pick with an uninitialised stack word; 0 never matches.
    int prev = 0;
    for (int i = 0; i < 120; ++i) {
        if (icon == 99) {
            int r;
            do r = crt::random(6) + 6;
            while (r == prev);
            chosen[i] = r;
            prev = r;
        } else {
            chosen[i] = icon;
        }
    }
    int k = 0;
    for (int y = 6; y < 190; y += 25)
        for (int col = 0; col < 15; ++col) {
            gfx_moveto(col * 40 + 20, int(ega_rows(y)));
            draw_icon(ds.str(uint16_t(chosen[k] * 0x80 + g::icons_cur)));
            ++k;
        }
}

// 1000:245c — "Thinking..." status box shown while the computer moves.
void draw_status_bar() {
    g::progress_dot_x = int16_t(gfx_text_width(ds.str(0x18d8)) + 0x68);  // "Thinking..."
    draw_panel(100, 0xaa, 0xe6, 0xb6);
    gfx_moveto(0x69, 0x12e);
    gfx_text_small(ds.str(0x18e4));  // "Thinking.. "
}

}  // namespace conflict::ui
