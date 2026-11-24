#include "game/affairs.hpp"

#include <cstring>

#include "game/globals.hpp"
#include "game/news.hpp"
#include "game/sys.hpp"
#include "game/diplomacy.hpp"
#include "game/ui.hpp"

namespace conflict::affairs {

using namespace sys;
using namespace g;

namespace {
// Remembered state for the overlays (not saved).
const Var<int16_t> last_iraq_nuclear(0x918e), last_iran_nuclear(0x9190);
const Var<int16_t> last_libya_nuclear(0x9192), last_egypt_nuclear(0x9194);
const Var<int16_t> last_syria_nuclear(0x9196);
const Var<int16_t> month_iraq_iran(0x9198), month_egypt_libya(0x919a), month_syria(0x919c);
const Var<int16_t> month_israel(0x919e);
const Var<int16_t> warned_world_opinion(0xb2e6);  // 0, 1 (warned), 2 (warned twice)

const char* icon(int index) { return ds.str(uint16_t(icons_cur + index * 0x80)); }

void advice(int n) { std::strcpy(ds.str(kWindowText), ds.str(word_at(uint16_t(0x2190 + 2 * n)))); }
void advice_about(uint16_t tmpl, int a, int b) {
    char buf[102] = {};
    news::format(ds.str(tmpl), a, b, buf);
    std::strcpy(ds.str(kWindowText), buf);
}

// Wait n iterations of the original's empty delay loop (2000/15000 over the calibration).
void pause(double count) { delay_loop(long(count / double(float(delay_calibration)))); }
}  // namespace

// 1000:502c — the Foreign Office's comment; later rules override earlier ones.
void foreign_office_advice() {
    uint16_t tmpl = 0;
    std::strcpy(ds.str(kWindowText), ds.str(0x240e));  // "Good Morning." (skipping '#')
    if (turn_count == 1) advice(8);
    if (sb262 < 0x9c && sb262 > 0x96) advice(1);
    if (sb264 > 0x32) advice(0);
    if (sb262 < 0x97 && sb262 > 0x8c) advice(0xb);
    if (sb266 == 6 && sb26a == 0) advice(9);
    if (sb26e != 7) advice(6);
    for (int a = 1; a < 7; ++a)
        for (int b = 0; b < a; ++b) {
            if (nation_status[a] == 10 || nation_status[b] == 10) continue;
            if (relations(a, b) == 3 && a != Iran &&
                (relations(Israel, a) < 5 || relations(Israel, b) < 5))
                tmpl = word_at(0x23fa);
            if (relations(a, b) == 8 && border(Israel, a) == 1 && border(Israel, b) == 1)
                tmpl = word_at(0x23fc);
            if (relations(a, b) == 10 && (relations(Israel, a) == 3 || relations(Israel, b) == 3))
                tmpl = word_at(0x23fe);
            if (tmpl != 0) {
                advice_about(tmpl, a, b);
                tmpl = 0;
            }
        }
    for (int n = 0; n < 7; ++n) {
        if (nation_status[n] == 10) continue;
        if (nation_status[n] > 6 && sa11e[n] < 10 && border(Israel, n) == 1 && sa0ee[n] == 0)
            tmpl = word_at(0x2400);
        if (forces(Israel, n, 0) != 0 && relations(Israel, n) < 4 && relations(Israel, n) > 1)
            tmpl = word_at(0x2402);
        if (turn_count != 1 && relations(Israel, n) == 7 && sa0de[n] == 0 && sa0ee[n] == 0)
            tmpl = word_at(0x2404);
        if (sa0de[n] == 0 && relations(Israel, n) == 2) tmpl = word_at(0x2406);
        if (at_war(Israel, n) == 0 && sa0de[n] == 0 && relations(Israel, n) == 1)
            tmpl = word_at(0x2408);
        if (tmpl != 0) {
            advice_about(tmpl, n, Israel);
            tmpl = 0;
        }
    }
    if (sb264 > 0x22 && sb264 < 0x32 && warned_world_opinion == 0) {
        advice(10);
        warned_world_opinion = 1;
    }
    if (sb264 > 0x31 && warned_world_opinion != 2) {
        advice(0xd);
        warned_world_opinion = 2;
    }
    if (sb274 == 6 && sb262 < 0x96) advice(7);
    for (int n = 0; n < 7; ++n) {
        if (sa15e[n] == 1 && sa0de[n] == 0 && border(Israel, n) == 1) tmpl = word_at(0x240a);
        if (relations(Israel, n) < 4 && nuclear[Israel] < 10 && nuclear[n] == 10) tmpl = word_at(0x240c);
        if (tmpl != 0) {
            advice_about(tmpl, n, Israel);
            tmpl = 0;
        }
    }
}

// 1000:549b
void nation_affairs(int n) {
    bool done = false;
    gfx_draw_map(int16_t(word_at(uint16_t(0x10fe + 2 * n))));
    while (!done) {
        if (n == Iran || n == Iraq) overlay_iraq_iran();
        if (n == Egypt || n == Libya) overlay_egypt_libya();
        if (n == Jordan || n == Lebanon) overlay_israel();
        if (n == Syria) overlay_syria();
        char* title = ds.str(kWindowTitle);
        char* text = ds.str(kWindowText);
        char* lines = ds.str(kWindowLines);
        std::strcpy(title, ds.str(0x241c));  // ""
        std::strcat(title, ds.str(nation_adjective[n]));
        std::strcat(title, ds.str(0x241d));  // " Official News"
        std::strcpy(text, ds.str(0x242c));  // "|"
        std::strcat(text, ds.str(nation_adjective[n]));
        std::strcat(text, ds.str(0x242e));  // " prestige: "
        std::strcat(text, ds.str(word_at(uint16_t(0x1098 + 2 * sa04e[n]))));
        std::strcat(text, ds.str(0x243a));  // ".|"
        std::strcpy(lines, ds.str(sa1e2[n]));
        int len = int(std::strlen(lines));
        if (len > 200) std::strcpy(text, ds.str(0x243d));  // "|"
        if (len != 0 && len < 0x5c) ui::text_window(0x140, 0x82, 0x276, 0xc3, title, lines, text);
        if (len > 0x5b) ui::text_window(0x122, 100, 0x276, 0xc3, title, lines, text);
        menu_context = 2;
        int choice = ui::menu_popup(0x1a9, 5, 0x276, 0x4c, ds.str(word_at(0x0cae)), n);
        menu_context = 0;
        if (choice != 3) {
            gfx_restore_rect(0x122, 0x276, 0xaf, 0x155);
            gfx_restore_rect(0x1a9, 0x276, 8, 0x85);
        }
        if (choice == 1) diplomacy::diplomacy(n);
        if (choice == 2) diplomacy::intelligence(n);
        if (choice == 3) done = true;
    }
}

void overlay_iraq_iran() {
    if (turn_count == 1) {
        month_iraq_iran = turn_count.get();
        last_iraq_nuclear = nuclear[Iraq].get();
        last_iran_nuclear = nuclear[Iran].get();
    }
    mark_collapsed(Iraq, 0xe8, 0x51);
    mark_collapsed(Iran, 0x15e, 0xf);
    show_nuclear(Iran, last_iran_nuclear, month_iraq_iran, 0x32, 5, 0xfa, 0x3c, 0x111, 6);
    show_nuclear(Iraq, last_iraq_nuclear, month_iraq_iran, 0xf, 0x34, 0xbe, 0x58, 0x9c, 0x1f);
    mark_war(Syria, Iraq, month_iraq_iran, 0x53, 0x32);
    mark_war(Iraq, Iran, month_iraq_iran, 0x118, 0x2f);
    month_iraq_iran = turn_count.get();
    last_iraq_nuclear = nuclear[Iraq].get();
    last_iran_nuclear = nuclear[Iran].get();
}

void overlay_egypt_libya() {
    if (turn_count == 1) {
        month_egypt_libya = turn_count.get();
        last_libya_nuclear = nuclear[Libya].get();
        last_egypt_nuclear = nuclear[Egypt].get();
    }
    mark_collapsed(Egypt, 0x189, 0x3a);
    mark_collapsed(Libya, 0x3a, 0x30);
    show_nuclear(Libya, last_libya_nuclear, month_egypt_libya, 0x1e, 0x91, 0xe6, 0xbd, 0xd2, 0x79);
    mark_war(Libya, Egypt, month_egypt_libya, 0x11d, 0x52);
    show_nuclear(Egypt, last_egypt_nuclear, month_egypt_libya, 0xa5, 0x91, 0x16d, 0xbb, 0x17e, 0x89);
    month_egypt_libya = turn_count.get();
    last_libya_nuclear = nuclear[Libya].get();
    last_egypt_nuclear = nuclear[Egypt].get();
}

void overlay_syria() {
    if (turn_count == 1) {
        month_syria = turn_count.get();
        last_syria_nuclear = nuclear[Syria].get();
    }
    show_nuclear(Syria, last_syria_nuclear, month_syria, 0x50, 0x61, 0x118, 0x91, 0xcd, 0x43);
    mark_war(Syria, Iraq, month_syria, 0x151, 0x4d);
    mark_war(Israel, Syria, month_syria, 0x5a, 0x70);
    mark_war(Syria, Jordan, month_syria, 0x98, 0x76);
    mark_collapsed(Syria, 0xcd, 0x43);
    mark_collapsed(Lebanon, 0x4a, 0x4b);
    month_syria = turn_count.get();
    last_syria_nuclear = nuclear[Syria].get();
}

// 1000:3fc9 — Israel's neighbourhood: collapsed neighbours, brigades on borders, wars.
void overlay_israel() {
    mark_collapsed(Egypt, 0x3f, 0x8a);
    mark_collapsed(Syria, 0x14f, 0x19);
    mark_collapsed(Jordan, 0x116, 0x6b);
    mark_collapsed(Lebanon, 0xd0, 0x10);
    const int brigade_icon = 7;  // icons_cur + 0x380
    if (border_brigades[Egypt] != 0) { gfx_moveto(0x5b, 0xee); ui::draw_icon(icon(brigade_icon)); }
    if (border_brigades[Syria] != 0) { gfx_moveto(0xfb, 0x32); ui::draw_icon(icon(brigade_icon)); }
    if (border_brigades[Lebanon] != 0) { gfx_moveto(0xd0, 0x1c); ui::draw_icon(icon(brigade_icon)); }
    if (border_brigades[Jordan] != 0) { gfx_moveto(0xae, 0x93); ui::draw_icon(icon(brigade_icon)); }
    mark_war(Israel, Egypt, month_israel, 0x5b, 0x88);
    mark_war(Israel, Syria, month_israel, 0xfb, 0x1d);
    mark_war(Israel, Lebanon, month_israel, 0xd0, 0x10);
    mark_war(Israel, Jordan, month_israel, 0xae, 0x54);
    month_israel = turn_count.get();
}

void mark_collapsed(int n, int x, int y) {
    if (nation_status[n] == 10) {
        gfx_moveto(x, int(ega_rows(y)));
        ui::draw_icon(icon(4));  // icons_cur + 0x200
    }
}

void mark_war(int a, int b, int last_month, int x, int y) {
    if (at_war(a, b) != 0 && nation_status[a] < 10 && nation_status[b] < 10) {
        gfx_moveto(x, int(ega_rows(y)));
        if (turn_count != last_month) blink_war(x, y);
        ui::draw_icon(icon(3));  // icons_cur + 0x180
    }
}

// Flash an icon five times: inverted (white on black), then normal.
static void blink(int x, int y, int index) {
    gfx_moveto(x, int(ega_rows(y)));
    ui::draw_icon(icon(index));
    for (int i = 0; i < 5; ++i) {
        gfx_moveto(x, int(ega_rows(y)));
        fg_colour = 15;
        bg_colour = 0;
        gfx_draw_glyph32(icon(index));
        pause(2000.0);
        gfx_moveto(x, int(ega_rows(y)));
        fg_colour = 0;
        bg_colour = 15;
        ui::draw_icon(icon(index));
        pause(2000.0);
    }
}

void blink_war(int x, int y) { blink(x, y, 3); }

void blink_nuclear(int x, int y, int n) { blink(x, y, int16_t(word_at(uint16_t(0x1a2e + 2 * nuclear[n])))); }

// 1000:4126 — nuclear programme icon; a WARNING window the first time it is seen (level 6) or
// tested (level 10, after 9).
void show_nuclear(int n, int last_level, int last_month, int x0, int y0, int x1, int y1, int x, int y) {
    if (nuclear[n] <= 6) return;
    gfx_moveto(x, int(ega_rows(y)));
    ui::draw_icon(icon(int16_t(word_at(uint16_t(0x1a2e + 2 * nuclear[n])))));
    if (last_month != turn_count && (last_level == 6 || (nuclear[n] == 10 && last_level == 9))) {
        const char* text = last_level == 6 ? ds.str(0x1a44) : ds.str(0x1a76);  // "U.S. satellite has sighted a nuclear installat…", "Reports have confirmed a nuclear test explosion"
        ui::text_window(x0, y0, x1, y1, ds.str(0x1aa8), text, ds.str(0x1aa6));  // "WARNING", "|"
        blink_nuclear(x, y, n);
        wait_or_key(long(15000.0 / double(float(delay_calibration))));
        gfx_restore_rect(x0, x1, int(ega_rows(y0)), int(ega_rows(y1)));
    }
}

}  // namespace conflict::affairs
