#include "game/yearly.hpp"

#include <cstring>

#include "game/ai.hpp"
#include "game/crt.hpp"
#include "game/diplomacy.hpp"
#include "game/globals.hpp"
#include "game/reports.hpp"
#include "game/sys.hpp"
#include "game/ui.hpp"

#pragma GCC diagnostic ignored "-Wrestrict"  // distinct strings inside one DGROUP buffer

namespace conflict::yearly {

using namespace sys;
using namespace g;

namespace {
const char* menu(uint16_t ptr_addr) { return ds.str(word_at(ptr_addr)); }
}

void budget_meeting(int year) {
    char year_text[8], aid_text[8], gnp_text[8];
    char* title = ds.str(kWindowTitle);
    char* text = ds.str(kWindowText);
    char* lines = ds.str(kWindowLines);
    crt::itoa(year, year_text);
    crt::itoa(sa03e[Israel], gnp_text);
    std::strcpy(title, year_text);
    std::strcat(title, ds.str(0x7cb5));  // " DEFENSE BUDGET MEETING"
    std::strcpy(aid_text, ds.str(0x7ccd));
    int aid = 0;
    if (sb264 < 5) aid = 3000;
    else if (sb264 < 0x1e) aid = 2000;
    if (sb264 > 0x1d && sb264 < 0x32) aid = 1000;
    if (sb264 > 0x31) aid = 0;
    treasury[Israel] += int16_t(aid);
    crt::itoa(aid, aid_text);
    std::strcpy(text, ds.str(0x7cce));
    if (aid == 0) {
        std::strcat(text, ds.str(0x7d41));  // "The U.S. have refused to give us ..."
    } else {
        std::strcat(text, ds.str(0x7d03));  // "The U.S. have given a financial a..."
        std::strcat(text, aid_text);
        std::strcat(text, ds.str(0x7d37));  // " million."
    }
    crt::itoa(treasury[Israel], aid_text);
    std::strcpy(lines, ds.str(0x7d75));  // "PRESENT DEFENSE BUDGET: $"
    std::strcat(lines, aid_text);
    std::strcat(lines, ds.str(0x7d90));
    ui::text_window(0x14, 10, 0x172, 0x50, title, text, lines);
    std::strcpy(title, ds.str(0x7d9b));  // "Spending increase"
    std::strcpy(text, ds.str(0x7dad));
    std::strcpy(lines, ds.str(0x7e07));  // "PERCENTAGE OF G.N.P. SPENT ON D..."
    std::strcat(lines, gnp_text);
    std::strcat(lines, ds.str(0x7e31));
    ui::text_window(0x172, 0x69, 0x276, 0xc3, title, text, lines);
    font_small = font_extra;
    gfx_moveto(0x1ff, 0x130);
    gfx_text_small(ds.str(0x7e33));
    gfx_moveto(0x1f2, 0xd8);
    gfx_text_small(ds.str(0x7e35));
    font_small = font_etech;
    int choice = ui::menu_popup(400, 5, 0x276, 0x55, menu(0x0ce4), 1);
    if (nation_status[Iraq] != 10) gfx_restore_rect(400, 0x276, 8, 0x96);
    gfx_restore_rect(0x172, 0x276, 0xb7, 0x155);
    if (choice == 1) {
        sa03e[Israel] += 2;
        if (nation_status[Israel] > 5) ++nation_status[Israel];
    }
    if (choice == 2) {
        sa03e[Israel] -= 2;
        if (nation_status[Israel] > 0 && nation_status[Israel] < 6) --nation_status[Israel];
        sb264 -= 5;
    }
    if (sb26c == 0) {
        std::strcpy(title, ds.str(0x7e37));  // "Army size"
        std::strcpy(text, ds.str(0x7e41));
        std::strcpy(lines, ds.str(0x7e90));
        ui::text_window(0x140, 0x78, 0x276, 0xc3, title, text, lines);
        choice = ui::menu_popup(400, 5, 0x276, 0x5a, menu(0x0ce6), 1);
        gfx_restore_rect(0x140, 0x276, 0xd2, 0x155);
        if (choice == 1) {
            border_brigades[Israel] += 2;
            forces(Israel, Israel, 0) += 400;
        }
    }
    sb26c = 0;
}

void summit() {
    int us_before = sb264;
    char* title = ds.str(kWindowTitle);
    char* text = ds.str(kWindowText);
    char* lines = ds.str(kWindowLines);
    char buf[320];
    std::strcpy(title, ds.str(0x78e8));  // " MIDDLE EASTERN SUMMIT "
    std::strcpy(text, ds.str(0x7900));
    ui::text_window(0x96, 0x37, 0x1e0, 100, title, text, ds.str(0x7963));
    if (ui::menu_popup(200, 0x6e, 0x1ae, 0xa0, menu(0x0cf6), 0) == 2) {
        sb264 += 5;  // Israel stayed away
        return;
    }
    gfx_draw_map(0);
    reports::black_out_collapsed();
    // Wars older than two months are settled.
    for (int a = 0; a < 8; ++a)
        for (int b = 0; b < a; ++b) {
            if (!(at_war(a, b) > 2 && nation_status[a] < 10 && nation_status[b] < 10 &&
                  (nation_status[a] > 2 || a == Israel) && nation_status[b] > 2))
                continue;
            std::strcpy(title, ds.str(0x7965));  // "SUMMIT PEACE AGREEMENT"
            char months[8];
            crt::itoa(at_war(a, b), months);
            crt::sprintf(lines, ds.str(0x797d), {static_cast<const char*>(months)});
            if (a == Israel) {
                std::strcpy(buf, ds.str(0x7999));  // "The U.N. have persuaded "
                std::strcat(buf, ds.str(nation_name[b]));
                std::strcat(buf, ds.str(0x79b2));
                ui::text_window(0x82, 100, 500, 0xc3, title, buf, lines);
                if (ui::menu_popup(0x1ae, 5, 0x276, 0x55, menu(0x0cf2), 0) == 1)
                    diplomacy::make_peace(Israel, b);
                gfx_restore_rect(0x82, 500, 0xaf, 0x155);
                gfx_restore_rect(0x1ae, 0x276, 8, 0x94);
            } else {
                std::strcpy(text, ds.str(0x7a51));  // "The "
                std::strcat(text, ds.str(nation_adjective[a]));
                std::strcat(text, ds.str(0x7a56));  // " - "
                std::strcat(text, ds.str(nation_adjective[b]));
                std::strcat(text, ds.str(0x7a5a));  // " war has been settled amicably by..."
                ui::text_window(0x96, 10, 0x1e0, 0x5a, title, text, lines);
                wait_or_key(long(80000.0 / double(float(delay_calibration))));  // 1d93:1a01
                diplomacy::make_peace(a, b);
                gfx_restore_rect(0x96, 0x1e0, 0x11, 0x9d);
            }
        }
    // A friendly, stable neighbour may offer to take the Palestinians.
    int best = Jordan;
    for (int n = 0; n < 8; ++n)
        if (border(Israel, n) == 1 && relations(Israel, n) > 3 && nation_status[n] < 8 &&
            sa04e[best] <= sa04e[n])
            best = n;
    if (sb26a == 0 && relations(Israel, best) > 3 && nation_status[best] < 8) {
        std::strcpy(buf, ds.str(0x7a88));  // "In order to settle the Palestinian ..."
        std::strcat(buf, ds.str(nation_name[best]));
        std::strcat(buf, ds.str(0x7af7));  // " has offered to take control of the "
        std::strcat(buf, ds.str(nation_adjective[best]));
        std::strcat(buf, ds.str(0x7b86));  // " border."
        std::strcpy(lines, ds.str(0x7b8f));
        ui::text_window(0x46, 100, 0x230, 0xc3, title, buf, lines);
        if (ui::menu_popup(0x1a4, 5, 0x276, 0x50, menu(0x0cde), 1) == 1) {
            sb264 -= 0x19;
            if (sa04e[Israel] < 10) ++sa04e[Israel];
            if (sa04e[best] < 10) ++sa04e[best];
            s98a0[best] = -2;
            sa11e[Israel] = 0;
            border_brigades[Israel] += border_brigades[8].get();
            border_brigades[8] = 0;
            sb26a = 1;
        }
    }
    gfx_draw_map(0);
    reports::black_out_collapsed();
    std::strcpy(title, ds.str(0x7be3));  // "SUMMIT PROPOSAL"
    std::strcpy(text, ds.str(0x7bf3));
    std::strcpy(lines, ds.str(0x7c87));
    ui::text_window(0x6e, 0x78, 0x208, 0xc3, title, text, lines);
    int choice = ui::menu_popup(0x1ae, 5, 0x276, 100, menu(0x0ce2), 1);
    gfx_restore_rect(0x6e, 0x208, 0xd2, 0x155);
    if (choice == 1) {
        sb264 -= 0xf;
        sb26c = 1;
    }
    if (us_before > 0x31 && sb264 < 0x32) ai::event_news(6);
}

}  // namespace conflict::yearly
