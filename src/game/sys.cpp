#include "game/sys.hpp"

#include <chrono>
#include <thread>

#include "game/fixes.hpp"
#include "game/globals.hpp"
#include "platform/assets.hpp"
#include "platform/ega.hpp"
#include "platform/host.hpp"

namespace conflict::sys {

namespace {
ega::Ega* g_ega = nullptr;
Host* g_host = nullptr;
const Assets* g_assets = nullptr;

// Nominal speed for busy loops: an empty Zortech `for` loop on a ~10 MHz AT is roughly 2 us per
// iteration; a loop calling kbd_poll_ext (an int 16h round trip) roughly 50 us.
constexpr double kUsPerEmptyIteration = 2.0;
constexpr double kUsPerPollIteration = 50.0;

std::vector<uint16_t> g_test_keys;
size_t g_test_key_pos = 0;

void sync() {
    g_ega->fg = uint8_t(g::fg_colour);
    g_ega->bg = uint8_t(g::bg_colour);
    g_ega->font_mem = ds.data();
    g_ega->small_font = g::font_small;
    g_ega->large_font = g::font_large;
    g_ega->fix_text_width = fixes::fix(6);
}
}  // namespace

void attach(ega::Ega& ega, Host* host, const Assets& assets) {
    g_ega = &ega;
    g_host = host;
    g_assets = &assets;
}
ega::Ega& screen() { return *g_ega; }
const Assets& assets() { return *g_assets; }

void gfx_init() { if (g_ega) g_ega->set_mode(); }
void gfx_moveto(int x, int y) { if (g_ega) g_ega->moveto(x, y); }
void gfx_plot(int x, int y) { if (!g_ega) return; sync(); g_ega->plot(x, y); }
void gfx_fill_rect_row(int x0, int x1, int y, int flag) { if (!g_ega) return; sync(); g_ega->hspan(x0, x1, y, flag); }
void gfx_vline(int y0, int y1, int x, int flag) { if (!g_ega) return; sync(); g_ega->vline(y0, y1, x, flag); }
void gfx_fill_rect(int x0, int x1, int y0, int y1) { if (!g_ega) return; sync(); g_ega->fill_rect(x0, x1, y0, y1); }
void gfx_restore_rect(int x0, int x1, int y0, int y1) { if (g_ega) g_ega->restore_rect(x0, x1, y0, y1); }
void gfx_blit_colour(int x, int y, int w, int h, int from, int to) {
    if (g_ega) g_ega->recolour(x, y, w, h, from, to);
}
void gfx_text_small(const char* s) { if (!g_ega) return; sync(); g_ega->text_small(s); }
void gfx_text_large(const char* s) { if (!g_ega) return; sync(); g_ega->text_large(s); }
int gfx_text_width(const char* s) { if (!g_ega) return 0; sync(); return g_ega->text_width(s); }
void gfx_draw_glyph32(const char* src) {
    if (!g_ega) return;
    sync();
    g_ega->draw_icon(reinterpret_cast<const uint8_t*>(src));
}
void gfx_draw_map(int index) {
    if (g_ega && g_assets && index >= 0 && index < int(g_assets->maps.size())) g_ega->draw_map(g_assets->maps[index]);
}
void gfx_draw_picture(int index) {
    if (g_ega && g_assets && index >= 0 && index < int(g_assets->pictures.size()))
        g_ega->draw_picture(g_assets->pictures[index]);
}

// 1000:0444 — the original reads the key through int86() with its register block at DS:b2a4,
// so the last key also stays there.
uint16_t get_key() {
    uint16_t key;
    if (g_host) key = g_host->get_key();
    else key = g_test_key_pos < g_test_keys.size() ? g_test_keys[g_test_key_pos++] : key::kEnter;
    ds.put<uint16_t>(0xb2a4, key);
    return key;
}
void set_test_keys(const std::vector<uint16_t>& keys) {
    g_test_keys = keys;
    g_test_key_pos = 0;
}
int kbd_poll_ext() {
    if (!g_host) return 0;
    g_host->fix_key_poll = fixes::fix(7);
    return g_host->kbd_poll_ext();
}

void delay_loop(long iterations) {
    if (!g_host) return;
    long us = long(iterations * kUsPerEmptyIteration);
    if (us >= 1000) g_host->sleep_ms(int(us / 1000));
    else g_host->idle();
}

void wait_or_key(long iterations) {
    if (!g_host) return;
    if (g_host->headless()) {
        g_host->snapshot();
        return;
    }
    using clock = std::chrono::steady_clock;
    auto end = clock::now() + std::chrono::microseconds(long(iterations * kUsPerPollIteration));
    while (clock::now() < end) {
        if (fixes::fix(7) ? g_host->take_any_key() : kbd_poll_ext() != 0) return;  // bug #7
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
}

}  // namespace conflict::sys
