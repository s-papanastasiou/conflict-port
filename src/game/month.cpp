#include "game/month.hpp"

#include <cstring>

#include "game/affairs.hpp"
#include "game/ai.hpp"
#include "game/arms.hpp"
#include "game/crt.hpp"
#include "game/endgame.hpp"
#include "game/globals.hpp"
#include "game/newspaper.hpp"
#include "game/policy.hpp"
#include "game/reports.hpp"
#include "game/sys.hpp"
#include "game/turn.hpp"
#include "game/ui.hpp"
#include "game/yearly.hpp"

#pragma GCC diagnostic ignored "-Wrestrict"  // distinct strings inside one DGROUP buffer

namespace conflict::month {

using namespace sys;
using namespace g;

namespace {
const char* menu(uint16_t ptr_addr) { return ds.str(word_at(ptr_addr)); }
const Var<int16_t> warned_world_opinion(0xb2e6);
}  // namespace

const char* month_name(int m) {
    if (m > 0 && m < 13) return ds.str(word_at(uint16_t(0x1b0e + 2 * m)));
    return ds.str(word_at(0x1b0e));
}

void security_advice(int n) { std::strcpy(ds.str(kWindowText), ds.str(word_at(uint16_t(0x2190 + 2 * n)))); }

void player_month() {
    char* title = ds.str(kWindowTitle);
    char* text = ds.str(kWindowText);
    char* lines = ds.str(kWindowLines);
    char year_text[8], budget_text[8], policy_menu[150];
    int options[3] = {0, 0, 0};
    int option_count = 0;
    if (loaded_game_flag == 0) {
        treasury[Israel] = int16_t(int(double(float(double(sa03e[Israel]))) / 100.0 * double(budget[Israel]) +
                                       double(treasury[Israel])));
        int pressure = (sa11e[Israel] + nation_status[Israel]) - sa04e[Israel];
        int policing = border_brigades[8] * sb266;
        if (crt::random(10) < pressure - policing) {
            ++nation_status[Israel];
            if (nation_status[Israel] > 9) endgame::game_over(0, 0, 7, 0);  // voted out
            ai::instability_news(Israel);
        }
        if (turn_count == 1) {
            year = 0x7cd;  // 1997
            sb274 = 0;
            warned_world_opinion = 0;
        }
        ++sb274;  // month of the year
        if (sb274 == 13) {
            ++year;
            sb274 = 1;
        }
        for (int n = 0; n < 8; ++n) {
            sa12e[n] = 0;
            sa0de[n] = 0;
            sa0ee[n] = 0;
            covert_action[n] = 0;
        }
        // Middle East calm: sum of relations between surviving nations, minus war months.
        sb262 = 0;
        for (int a = 1; a < 8; ++a)
            for (int b = 0; b < a; ++b) {
                if (nation_status[a] < 10 && nation_status[b] < 10) {
                    sb262 += relations(a, b).get();
                    int war = at_war(a, b);
                    if (war != 0) sb262 -= int16_t(war);
                } else {
                    sb262 += 5;
                }
            }
        if (turn_count > 2 && sb274 == 1) yearly::budget_meeting(year);
        if (sb274 == 7 && sb262 < 0x96) yearly::summit();
        if (nation_status[Syria] > 9 && nation_status[Egypt] > 9 && nation_status[Lebanon] > 9 &&
            nation_status[Jordan] > 9)
            endgame::game_over(1, 1, 99, 0);  // all border states fallen: victory
    }
    loaded_game_flag = 0;
    crt::itoa(year, year_text);
    if (headline_count != 0) {
        newspaper::show();
        std::strcpy(text, month_name(sb274));
        std::strcat(text, ds.str(0x1b5c));
        std::strcat(text, year_text);
        std::strcat(text, ds.str(0x1b5e));  // "?<Good morning, Prime Minister>?"
        menu_context = 5;
        ui::menu_popup(0x1d6, 5, 0x276, 0x2a, text, 0);
        menu_context = 0;
    }
    gfx_draw_map(0);
    reports::black_out_collapsed();
    std::memset(ds.ptr(kReportText), 0, 1000);
    headline_count = 0;
    report_text_used = 0;
    // Foreign Office: pick a neighbour to deal with until "Continue" (item 8).
    int choice = 0;  // uninitialised in the original
    while (choice != 8) {
        crt::itoa(treasury[Israel], budget_text);
        std::strcpy(title, month_name(sb274));
        std::strcat(title, ds.str(0x1b81));
        std::strcat(title, year_text);
        std::strcat(title, ds.str(0x1b83));  // " Foreign Office"
        affairs::foreign_office_advice();
        std::strcpy(lines, ds.str(0x1b93));  // "Our international prestige is "
        std::strcat(lines, ds.str(word_at(uint16_t(0x1098 + 2 * sa04e[Israel]))));
        std::strcat(lines, ds.str(0x1bb3));  // ". Middle East tension is "
        if (sb262 < 0x79) std::strcat(lines, ds.str(0x1bcd));  // "totally dire"
        else std::strcat(lines, ds.str(word_at(uint16_t(0x1032 + 2 * ((sb262 - 0x78) / 5)))));
        std::strcat(lines, ds.str(0x1bda));
        ui::text_window(0x9b, 0x7d, 0x203, 0xbe, title, text, lines);
        menu_context = 1;
        choice = ui::menu_popup(0x20d, 5, 0x276, 0xaa, menu(0x0ca4), 0);
        menu_context = 0;
        if (choice < 8) affairs::nation_affairs(choice - 1);
        gfx_draw_map(0);
        reports::black_out_collapsed();
    }
    // Directives given this month take effect.
    for (int n = 0; n < 8; ++n) {
        int wanted = sa0de[n];
        if (wanted != 0 && wanted != relations(Israel, n)) {
            relations(Israel, n) = int16_t(wanted);
            ai::relation_news(Israel, n);
        }
        if (sa0ee[n] != 0) sa11e[n] = sa0ee[n].get();
    }
    // National Security Office until "Next Turn".
    for (;;) {
        std::strcpy(text, ds.str(0x1bdd));  // "Always keep the security of Israel in mind."
        if (turn_count > 2 && (nuclear[Israel] < nuclear[Egypt] || nuclear[Israel] < nuclear[Syria]))
            security_advice(0x11);
        if (turn_count > 5 && sb264 < 10 && relations(Israel, Egypt) > 2 && relations(Israel, Syria) > 2 &&
            relations(Israel, Lebanon) > 2 && relations(Israel, Jordan) > 2)
            security_advice(0x14);
        if (sb274 == 7 && sb26c == 1) security_advice(0x12);
        if (sb26a == 0) {
            if (nation_status[Israel] > 3) security_advice(0xc);
            if (nation_status[Israel] > 5) security_advice(3);
            if (nation_status[Israel] > 7) security_advice(4);
            if (nation_status[Israel] > 3 && sb274 == 5) security_advice(0xe);
        }
        if (at_war(Israel, Egypt) == 3) security_advice(0x15);
        if (at_war(Israel, Syria) == 2) security_advice(0x16);
        if (sb26a == 1) {
            if (nation_status[Israel] == 6) security_advice(0xf);
            if (nation_status[Israel] > 6) security_advice(0x10);
        }
        if (warned_world_opinion == 1 && sb264 > 0x32) security_advice(0x13);
        if (sb26e != 7) security_advice(2);
        for (int n = 0; n < 7; ++n)
            if (forces(n, Israel, Troops) != 0 && forces(Israel, n, Troops) == 0) security_advice(5);
        crt::itoa(treasury[Israel], budget_text);
        crt::itoa(year, year_text);
        std::strcpy(policy_menu, menu(0x0ca6));  // "Policy:?Purchase arms?Strategic Action?Review Forces?"
        std::strcpy(title, month_name(sb274));
        std::strcat(title, ds.str(0x1c09));
        std::strcat(title, year_text);
        std::strcat(title, ds.str(0x1c0b));  // " National Security Office"
        std::strcpy(lines, ds.str(0x1c3f));  // "Current Defense Budget $"
        std::strcat(lines, budget_text);
        std::strcat(lines, ds.str(0x1c59));
        ui::text_window(300, 0x87, 0x276, 0xbe, title, text, lines);
        // Extra items: Nuclear Club (until the bomb exists), Palestinian Problem (until settled),
        // Next Turn.
        if (nuclear[Israel] != 10) options[0] = 0;
        option_count = nuclear[Israel] != 10 ? 1 : 0;
        if (sb26a == 0) options[option_count++] = 1;
        options[option_count++] = 2;
        for (int i = 0; i < option_count; ++i)
            std::strcat(policy_menu, ds.str(word_at(uint16_t(0x1b56 + 2 * options[i]))));
        menu_context = 3;
        int pick = ui::menu_popup(400, 8, 0x276, 0x7b, policy_menu, 0);
        menu_context = 0;
        gfx_restore_rect(300, 0x276, 0xec, 0x14c);
        gfx_restore_rect(400, 0x276, 0xe, 0xd7);
        if (nation_status[Iraq] == 10) reports::black_out_collapsed();
        if (pick == 1) arms::procurement();
        if (pick == 2) turn::strategic_action();
        if (pick == 3) reports::review_forces();
        if (pick - 4 >= 0) {
            int option = options[pick - 4];
            if (option == 0) policy::nuclear_program();
            if (option == 1) policy::west_bank();
            if (option == 2) return;
        }
        gfx_draw_map(0);
        reports::black_out_collapsed();
    }
}

}  // namespace conflict::month
