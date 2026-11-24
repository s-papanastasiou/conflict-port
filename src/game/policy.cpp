#include "game/policy.hpp"

#include <cstring>

#include "game/ai.hpp"
#include "game/crt.hpp"
#include "game/globals.hpp"
#include "game/reports.hpp"
#include "game/sys.hpp"
#include "game/ui.hpp"

#pragma GCC diagnostic ignored "-Wrestrict"  // distinct strings inside one DGROUP buffer

namespace conflict::policy {

using namespace sys;
using namespace g;

namespace {
const char* menu(uint16_t ptr_addr) { return ds.str(word_at(ptr_addr)); }
// Not saved: month each screen was last used this turn, and the programme level shown then.
const Var<int16_t> west_bank_month(0x92a0), nuclear_month(0x92a2), nuclear_level_shown(0x92a4);
}  // namespace

void nuclear_program() {
    int threat = 0;
    if (nation_status[Iran] == 10 || nation_status[Iraq] == 10) reports::black_out_collapsed();
    int level = nuclear[Israel];
    if (nuclear_month == turn_count) level = nuclear_level_shown;  // already decided this month
    int setback = crt::random(10);
    char* title = ds.str(kWindowTitle);
    char* text = ds.str(kWindowText);
    char* lines = ds.str(kWindowLines);
    std::strcpy(title, ds.str(0x4ae2));  // "Israeli Nuclear Program"
    std::strcpy(lines, ds.str(0x4afc));  // "Most dangerous threat: "
    int worst = 0;
    for (int n = 0; n < 7; ++n) {
        int danger = nuclear[n] * 5;
        if (border(Israel, n) == 1 && nation_status[n] != 10) danger += 0x14;
        danger -= relations(Israel, n);
        if (worst < danger) {
            threat = n;
            worst = danger;
        }
    }
    std::strcat(lines, ds.str(nation_name[threat]));
    std::strcat(lines, ds.str(0x4b15));
    if (level == 6) {
        std::strcpy(text, ds.str(0x4b18));
        std::strcat(text, ds.str(0x4b55));
    }
    if (level == 7) {
        std::strcpy(text, ds.str(0x4b89));
        std::strcat(text, ds.str(0x4bca));
    }
    if (level == 8) {
        std::strcpy(text, ds.str(0x4bfc));
        std::strcat(text, ds.str(0x4c3b));
    }
    if (level == 9) std::strcpy(text, ds.str(0x4c5a));  // "Israel is now a nuclear power..."
    if (nuclear_month == turn_count) {
        ui::text_window(0x140, 0x73, 0x276, 0xc3, title, text, lines);
        ui::menu_popup(0x1ae, 5, 0x276, 0x32, menu(0x0ccc), 0);
    } else if (nuclear[Israel] == 9) {
        ui::text_window(0x140, 0x73, 0x276, 0xc3, title, text, lines);
        ui::menu_popup(0x1ae, 5, 0x276, 0x32, menu(0x0ce8), 0);
        nuclear[Israel] = 10;
    } else {
        ui::text_window(0x140, 0x73, 0x276, 0xc3, title, text, lines);
        int choice = ui::menu_popup(0x1ae, 5, 0x276, 0x3c, menu(0x0cca), 0);
        if (choice == 1 && treasury[Israel] > 0) {  // continue funding (20 M)
            if (treasury[Israel] < 0xf) ++setback;
            if (treasury[Israel] < 6) ++setback;
            if (treasury[Israel] < 0x15) treasury[Israel] = 0;
            else treasury[Israel] -= 0x14;
            if (nuclear[Israel] < 10 && setback < 4) ++nuclear[Israel];
            if (nuclear[Israel] == 10) ai::event_news(4);
        }
    }
    nuclear_month = turn_count.get();
    nuclear_level_shown = int16_t(level);
}

void west_bank() {
    char* title = ds.str(kWindowTitle);
    char* text = ds.str(kWindowText);
    char* lines = ds.str(kWindowLines);
    std::strcpy(title, ds.str(0x4a76));  // "West Bank Policy"
    std::strcpy(text, ds.str(0x4a87));   // "Arab unrest: "
    std::strcat(text, ds.str(word_at(uint16_t(0x4a60 + 2 * nation_status[Israel]))));
    std::strcat(text, ds.str(0x4a95));
    std::strcpy(lines, ds.str(0x4a97));  // "Israeli presence: "
    std::strcat(lines, border_brigades[8] == 0 ? ds.str(0x4aba) : ds.str(0x4aab));  // low / full
    std::strcat(lines, ds.str(0x4ac7));  // "Tactics: "
    std::strcat(lines, sb266 == 3 ? ds.str(0x4ad2) : ds.str(0x4ada));
    gfx_draw_map(1);
    ui::text_window(0x140, 0x78, 0x276, 0xc3, title, text, lines);
    if (west_bank_month == turn_count) {  // policy already set this month
        ui::menu_popup(0x1a4, 5, 0x276, 0x46, menu(0x0cc2), 0);
        return;
    }
    if (border_brigades[8] == 0 && border_brigades[Israel] != 0) {
        if (ui::menu_popup(400, 5, 0x276, 100, menu(0x0cac), 0) == 1) {  // post a brigade
            --border_brigades[Israel];
            ++border_brigades[8];
        }
        west_bank_month = turn_count.get();
        return;
    }
    if (border_brigades[8] == 0 && border_brigades[Israel] == 0) {  // no brigades available
        ui::menu_popup(400, 5, 0x276, 0x46, menu(0x0cd6), 1);
        west_bank_month = turn_count.get();
        return;
    }
    if (border_brigades[8] != 0 && sb266 == 3) {
        int choice = ui::menu_popup(400, 5, 0x276, 0x6e, menu(0x0cd2), 0);
        if (choice == 2) {  // allow troops to engage protesters
            sb266 = 6;
            sb264 += 0xf;
            sb276 += 3;
            ai::event_news(5);
            west_bank_month = turn_count.get();
            return;
        }
        if (choice == 1) {  // reduce presence
            --border_brigades[8];
            ++border_brigades[Israel];
            west_bank_month = turn_count.get();
            return;
        }
    }
    if (border_brigades[8] != 0 && sb266 == 6) {
        int choice = ui::menu_popup(400, 5, 0x276, 100, menu(0x0cd4), 0);
        if (choice == 2) sb266 = 3;  // soft tactics
        if (choice == 1) {
            --border_brigades[8];
            ++border_brigades[Israel];
        }
        west_bank_month = turn_count.get();
    }
}

}  // namespace conflict::policy
