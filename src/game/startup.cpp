#include "game/startup.hpp"

#include <algorithm>
#include <cstring>

#include "game/crt.hpp"
#include "game/dgroup.hpp"
#include "game/globals.hpp"
#include "game/sys.hpp"
#include "game/ui.hpp"
#include "platform/assets.hpp"

namespace conflict::startup {

using namespace sys;

namespace {
uint16_t malloc_copy(const std::vector<uint8_t>& src, uint16_t size, uint16_t copy) {
    uint16_t p = ds.malloc(size);
    std::copy_n(src.begin(), std::min<size_t>(copy, src.size()), ds.ptr(p));
    return p;
}
}  // namespace

// 1000:07fc — the original mallocs the font buffers in DGROUP and fills them with getc(); the
// bytes come from Assets (same text-mode reading) and are placed at the same offsets.
void load_fonts() {
    const Assets& a = sys::assets();
    // Small fonts: malloc(0x465) but the fill loop writes up to 0x465 inclusive; allocate 0x466.
    g::font_etech = malloc_copy(a.font_etech, 0x466, 0x466);
    g::font_extra = malloc_copy(a.font_extra, 0x466, 0x466);
    g::font_xini = malloc_copy(a.font_xini, 0x3f0, 0x3f0);
    g::font_xguard = malloc_copy(a.font_xguard, 0x3f0, 0x3f0);
    g::icons = malloc_copy(a.icons, 0x900, 0x800);
    g::icons_cur = g::icons;
    g::font_small = g::font_etech;
    g::font_large = uint16_t(g::font_xini - 0x900);
}

// 1000:2058 — red screen with one of four intro texts, chosen at random.
void intro() {
    gfx_init();
    load_fonts();
    g::fg_colour = 4;
    gfx_fill_rect(0, 0x27f, 0, 0x15d);
    g::bg_colour = 15;
    g::fg_colour = 0;
    char* title = ds.str(g::kWindowTitle);
    char* text = ds.str(g::kWindowText);
    char* lines = ds.str(g::kWindowLines);
    std::strcpy(text, ds.str(0x14b4));  // ""
    // Each poem is a title (1st string) and '|'-separated lines appended to the lines buffer.
    // The first poem starts the buffer with strcpy, the others strcat onto it (it is still empty
    // at start-up because BSS is zeroed).
    struct Poem {
        uint16_t title;
        int x0, y0, x1, y1;
        bool copy_first;
        std::initializer_list<uint16_t> lines;
    };
    static const Poem kPoems[] = {
        {0x14b5, 0xa5, 0x28, 0x1db, 0xa0, true,
         {0x14c9, 0x14e7, 0x14ff, 0x151f, 0x1534, 0x154d, 0x156b, 0x157e}},  // Sweet Bird of Truth
        {0x1598, 0x78, 0x28, 0x208, 0xa0, false,
         {0x15a6, 0x15ce, 0x15f4, 0x1621, 0x164a, 0x1674, 0x169e, 0x16c6}},  // Early Warning
        {0x16f2, 0x66, 0x46, 0x21a, 0x84, false, {0x16fe, 0x172c, 0x175b}},  // CONFLICT IS
        {0x178e, 0x78, 0x32, 0x208, 0x98, false,
         {0x179a, 0x17bd, 0x17e0, 0x180c, 0x1834, 0x185d}},  // Revelation
    };
    int which = crt::random(4);
    if (which < 1 || which > 4) return;
    const Poem& p = kPoems[which - 1];
    std::strcpy(title, ds.str(p.title));
    bool first = true;
    for (uint16_t l : p.lines) {
        if (first && p.copy_first) std::strcpy(lines, ds.str(l));
        else std::strcat(lines, ds.str(l));
        first = false;
    }
    ui::text_window(p.x0, p.y0, p.x1, p.y1, title, text, lines);
}

// 1000:0739 — the original timed empty loops against the DOS clock to scale later delays by
// CPU speed. Our delays are real-time already, so store the nominal value 1.0.
void calibrate_delay() { g::delay_calibration = 1.0f; }

// 1000:2337 — "C O N F L I C T" panel, Get Ready / Load Game menu, copyright line.
void title_screen() {
    char copyright[100];
    std::strcpy(copyright, ds.str(0x1881));  // "COPYRIGHT VIRGIN MASTERTRONIC 1990"
    g::loaded_game_flag = 0;
    ui::draw_panel(0x94, 0x32, 0x1e0, 0x50);
    gfx_moveto(0xb9, 0x62);
    g::font_large = uint16_t(g::font_xini - 0x900);
    gfx_text_large(ds.str(0x18a4));  // "C O N F L I C T"
    gfx_moveto(0xaa, 0x7c);
    gfx_text_small(ds.str(0x18b4));  // "The Middle East Political Simulator"
    if (ui::menu_popup(0xfa, 0x66, 0x17c, 0x93, ds.str(g::menu_texts[6]), 0) == 2)
        g::loaded_game_flag = 1;
    ui::draw_panel(0xe, 0xa0, 0x265, 0xb4);
    int w = gfx_text_width(copyright);
    gfx_moveto((0x254 - w) / 2 + 0x11, 0x126);
    gfx_text_small(copyright);
    wait_or_key(20000);
}

}  // namespace conflict::startup
