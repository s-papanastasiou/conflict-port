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
#include "game/border.hpp"
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
        treasury[Israel] = int16_t(int(double(float(double(defence_share[Israel]))) / 100.0 * double(gnp[Israel]) +
                                       double(treasury[Israel])));
        int pressure = (insurgency[Israel] + nation_status[Israel]) - prestige[Israel];
        int policing = border_brigades[8] * west_bank_tactics;
        if (crt::random(10) < pressure - policing) {
            ++nation_status[Israel];
            if (nation_status[Israel] > 9) endgame::game_over(0, 0, 7, 0);  // voted out
            ai::instability_news(Israel);
        }
        if (turn_count == 1) {
            year = 0x7cd;  // 1997
            month_of_year = 0;
            warned_world_opinion = 0;
        }
        ++month_of_year;  // month of the year
        if (month_of_year == 13) {
            ++year;
            month_of_year = 1;
        }
        for (int n = 0; n < 8; ++n) {
            strike_order[n] = 0;
            relation_directive[n] = 0;
            insurgency_directive[n] = 0;
            covert_action[n] = 0;
        }
        // Middle East calm: sum of relations between surviving nations, minus war months.
        middle_east_calm = 0;
        for (int a = 1; a < 8; ++a)
            for (int b = 0; b < a; ++b) {
                if (nation_status[a] < 10 && nation_status[b] < 10) {
                    middle_east_calm += relations(a, b).get();
                    int war = at_war(a, b);
                    if (war != 0) middle_east_calm -= int16_t(war);
                } else {
                    middle_east_calm += 5;
                }
            }
        if (turn_count > 2 && month_of_year == 1) yearly::budget_meeting(year);
        if (month_of_year == 7 && middle_east_calm < 0x96) yearly::summit();
        if (nation_status[Syria] > 9 && nation_status[Egypt] > 9 && nation_status[Lebanon] > 9 &&
            nation_status[Jordan] > 9)
            endgame::game_over(1, 1, 99, 0);  // all border states fallen: victory
    }
    loaded_game_flag = 0;
    crt::itoa(year, year_text);
    if (headline_count != 0) {
        newspaper::show();
        std::strcpy(text, month_name(month_of_year));
        std::strcat(text, ds.str(0x1b5c));  // " "
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
        std::strcpy(title, month_name(month_of_year));
        std::strcat(title, ds.str(0x1b81));  // " "
        std::strcat(title, year_text);
        std::strcat(title, ds.str(0x1b83));  // " Foreign Office"
        affairs::foreign_office_advice();
        std::strcpy(lines, ds.str(0x1b93));  // "Our international prestige is "
        std::strcat(lines, ds.str(word_at(uint16_t(0x1098 + 2 * prestige[Israel]))));
        std::strcat(lines, ds.str(0x1bb3));  // ". Middle East tension is "
        if (middle_east_calm < 0x79) std::strcat(lines, ds.str(0x1bcd));  // "totally dire"
        else std::strcat(lines, ds.str(word_at(uint16_t(0x1032 + 2 * ((middle_east_calm - 0x78) / 5)))));
        std::strcat(lines, ds.str(0x1bda));  // ".|"
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
        int wanted = relation_directive[n];
        if (wanted != 0 && wanted != relations(Israel, n)) {
            relations(Israel, n) = int16_t(wanted);
            ai::relation_news(Israel, n);
        }
        if (insurgency_directive[n] != 0) insurgency[n] = insurgency_directive[n].get();
    }
    // National Security Office until "Next Turn".
    for (;;) {
        std::strcpy(text, ds.str(0x1bdd));  // "Always keep the security of Israel in mind."
        if (turn_count > 2 && (nuclear[Israel] < nuclear[Egypt] || nuclear[Israel] < nuclear[Syria]))
            security_advice(0x11);
        if (turn_count > 5 && us_disapproval < 10 && relations(Israel, Egypt) > 2 && relations(Israel, Syria) > 2 &&
            relations(Israel, Lebanon) > 2 && relations(Israel, Jordan) > 2)
            security_advice(0x14);
        if (month_of_year == 7 && summit_agreement == 1) security_advice(0x12);
        if (land_for_peace == 0) {
            if (nation_status[Israel] > 3) security_advice(0xc);
            if (nation_status[Israel] > 5) security_advice(3);
            if (nation_status[Israel] > 7) security_advice(4);
            if (nation_status[Israel] > 3 && month_of_year == 5) security_advice(0xe);
        }
        if (at_war(Israel, Egypt) == 3) security_advice(0x15);
        if (at_war(Israel, Syria) == 2) security_advice(0x16);
        if (land_for_peace == 1) {
            if (nation_status[Israel] == 6) security_advice(0xf);
            if (nation_status[Israel] > 6) security_advice(0x10);
        }
        if (warned_world_opinion == 1 && us_disapproval > 0x32) security_advice(0x13);
        if (strike_damage != 7) security_advice(2);
        for (int n = 0; n < 7; ++n)
            if (forces(n, Israel, Troops) != 0 && forces(Israel, n, Troops) == 0) security_advice(5);
        crt::itoa(treasury[Israel], budget_text);
        crt::itoa(year, year_text);
        std::strcpy(policy_menu, menu(0x0ca6));  // "Policy:?Purchase arms?Strategic Action?Review Forces?"
        std::strcpy(title, month_name(month_of_year));
        std::strcat(title, ds.str(0x1c09));  // " "
        std::strcat(title, year_text);
        std::strcat(title, ds.str(0x1c0b));  // " National Security Office"
        std::strcpy(lines, ds.str(0x1c3f));  // "Current Defense Budget $"
        std::strcat(lines, budget_text);
        std::strcat(lines, ds.str(0x1c59));  // " M|"
        ui::text_window(300, 0x87, 0x276, 0xbe, title, text, lines);
        // Extra items: Nuclear Club (until the bomb exists), Palestinian Problem (until settled),
        // Next Turn.
        if (nuclear[Israel] != 10) options[0] = 0;
        option_count = nuclear[Israel] != 10 ? 1 : 0;
        if (land_for_peace == 0) options[option_count++] = 1;
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
        if (pick == 2) border::strategic_action();
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
