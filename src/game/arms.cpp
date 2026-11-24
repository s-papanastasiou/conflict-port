#include "game/arms.hpp"

#include <cstring>

#include "game/crt.hpp"
#include "game/fixes.hpp"
#include "game/globals.hpp"
#include "game/reports.hpp"
#include "game/sys.hpp"
#include "game/ui.hpp"

#pragma GCC diagnostic ignored "-Wrestrict"  // distinct strings inside one DGROUP buffer

namespace conflict::arms {

using namespace sys;
using namespace g;

namespace {
const char* menu(uint16_t ptr_addr) { return ds.str(word_at(ptr_addr)); }
bool lost_neighbour() {
    return nation_status[Iraq] == 10 || nation_status[Jordan] == 10 || nation_status[Syria] == 10;
}
void budget_line(char* line, uint16_t label, uint16_t tail) {
    char money[8];
    crt::itoa(treasury[Israel], money);
    std::strcpy(line, ds.str(label));
    std::strcat(line, money);
    std::strcat(line, ds.str(tail));
}
}  // namespace

void procurement() {
    char budget[26];
    char* title = ds.str(kWindowTitle);
    char* text = ds.str(kWindowText);
    char* lines = ds.str(kWindowLines);
    if (turn_count == 1) {
        total_deals = 0;
        for (int i = 0; i < 4; ++i) supplier_deals[i] = 0;
    }
    // The original sets the British spelling first, then overwrites it with the American one.
    std::strcpy(title, ds.str(0x452d));  // "Defense Procurement Office"
    budget_line(budget, 0x4548, 0x455a);
    if (at_war(Israel, Egypt) == 0 && at_war(Israel, Jordan) == 0 && at_war(Israel, Syria) == 0) {
        int r = crt::random(3) - 1;
        std::strcpy(text, ds.str(word_at(uint16_t(0x4206 + 2 * r))));
        int need = 3;
        if (forces(Israel, Israel, AttackHelicopters) < 0x32) need = 0;
        if (forces(Israel, Israel, Bombers) < 7) need = 1;
        if (forces(Israel, Israel, Fighters) < 200) need = 1;
        if (forces(Israel, Israel, Sams) < 0x19) need = 2;
        std::strcat(text, ds.str(word_at(uint16_t(0x42c2 + 2 * need))));
    } else {
        std::strcpy(text, ds.str(0x455e));  // "Due to our conflict situation,"
        std::strcat(text, ds.str(0x457d));  // "we must maintain high levels of battlefield eq…"
    }
    if (turn_count < 2 && forces(Israel, Israel, EarlyWarning) == 0) std::strcpy(text, ds.str(0x45b3));  // "We advise you to purchase an early warning sur…"
    if (nation_status[Iraq] == 10) reports::black_out_collapsed();
    ui::text_window(300, 0x7d, 0x276, 0xbe, title, text, budget);
    int choice = ui::menu_popup(0x1c2, 5, 0x276, 0x6c, menu(0x0ca8), 0);
    gfx_restore_rect(300, 0x276, 0xda, 0x14c);
    gfx_restore_rect(0x1c2, 0x276, 8, 0xbd);
    if (choice == 5) return;
    int supplier = choice - 1;
    int offers[9];
    int count = 0;  // uninitialised on the first pass in the original; only `count + 1 == 0` matters
    choice = 0;
    for (;;) {
        do {
            if (count + 1 == choice) return;  // "End talks"
            budget_line(budget, 0x4604, 0x4616);
            count = list_offers(supplier, offers);
            std::strcpy(title, ds.str(0x461a));  // "Meeting with "
            std::strcat(title, ds.str(supplier_adjective[supplier]));
            std::strcat(title, ds.str(0x4628));  // " arms dealer"
            if (count == 0) {
                if ((supplier == 0 || supplier == 3) && treasury[Israel] < 100) {
                    std::strcpy(text, ds.str(0x4635));  // "Due to your current lack of funds,"
                    std::strcat(text, ds.str(0x4658));  // "we feel unable to do business."
                }
                if ((supplier == 1 || supplier == 2) && treasury[Israel] < 0x32) {
                    std::strcpy(text, ds.str(0x4677));  // "Our government feels unable to trade"
                    std::strcat(text, ds.str(0x469c));  // " until your funds are larger."
                }
                if (sb264 > 0x31 && supplier != 3) {  // official arms embargo
                    std::strcpy(text, ds.str(0x46ba));  // "Due to the offical arms embargo on Israel,"
                    std::strcat(text, ds.str(0x46e5));  // "I have been instructed to make no sales."
                }
                ui::text_window(0x140, 0x82, 0x276, 0xc3, title, text, budget);
                ui::menu_popup(0x1c2, 5, 0x276, 0x32, menu(0x0cc4), 0);
                return;
            }
            std::strcpy(lines, ds.str(0x470e));  // "Equipment:?"
            for (int i = 0; i < count; ++i) {
                std::strcat(lines, ds.str(weapon_name[offers[i]]));
                std::strcat(lines, ds.str(0x471a));  // "?"
            }
            std::strcat(lines, ds.str(0x471c));  // "End talks?"
            if (supplier == 3) {
                std::strcpy(text, ds.str(word_at(0x44f0)));
            } else {
                int r = crt::random(3);
                std::strcpy(text, ds.str(word_at(uint16_t(0x44ea + 2 * (r - 1)))));
            }
            int mood = 6;
            if (sa04e[Israel] > 3 && treasury[Israel] > 0xfa) mood = 5;
            if (sa04e[Israel] > 3 && treasury[Israel] > 500) mood = 4;
            if (treasury[Israel] < 200) mood = 7;
            if (treasury[Israel] > 1000) mood = 10;
            if (total_deals > 5 && total_deals == supplier_deals[supplier]) mood = 9;
            if (count < 3) mood = 8;
            std::strcat(text, ds.str(word_at(uint16_t(0x44ea + 2 * mood))));
            ui::text_window(0x118, 0x86, 0x276, 0xc3, title, text, budget);
            if (count < 5) choice = ui::menu_popup(0x186, 5, 0x276, 0x78, lines, 0);
            else if (count < 7) choice = ui::menu_popup(0x16d, 5, 0x276, 0x84, lines, 0);
            else choice = ui::menu_popup(0x16d, 5, 0x276, 0x96, lines, 0);
            gfx_restore_rect(0x118, 0x276, 0xde, 0x155);
            gfx_restore_rect(0x16d, 0x276, 8, 0xea);
            if (lost_neighbour()) reports::black_out_collapsed();
        } while (count + 1 <= choice);
        negotiate(offers, choice, supplier);
        if ((supplier == 0 || supplier == 3) && treasury[Israel] < 100) return;
        if ((supplier == 1 || supplier == 2) && treasury[Israel] < 0x32) return;
        choice = ui::menu_popup(0x1a4, 5, 0x276, 0x4a, menu(0x0caa), 0);
        gfx_restore_rect(0x1a4, 0x276, 8, 0x81);
        if (lost_neighbour()) reports::black_out_collapsed();
        if (choice == 2) return;
    }
}

// Weapons a supplier will sell: prestige and loyalty high enough and the budget covers a unit.
// No sales from governments (USA/Britain/France) while US disapproval is 50 or more.
int list_offers(int supplier, int* offers) {
    int funds = treasury[Israel];
    int count = 0;
    struct Range { int first, last, min_funds; bool embargo; };
    static const Range kRanges[] = {{0, 7, 99, true}, {7, 11, 0x31, true}, {11, 14, 0x31, true}, {14, 19, 99, false}};
    if (supplier < 0 || supplier > 3) return 0;
    const Range& r = kRanges[supplier];
    if (r.embargo && !(sb264 < 0x32)) return 0;
    int loyalty = total_deals == 0 ? 100 : (supplier_deals[supplier] * 100) / total_deals;
    for (int w = r.first; w < r.last; ++w)
        if (weapon_min_prestige[w] <= sa04e[Israel] && weapon_min_loyalty[w] <= loyalty &&
            weapon_cost[w] < funds && funds > r.min_funds)
            offers[count++] = w;
    return count;
}

void negotiate(const int* offers, int choice, int supplier) {
    int w = offers[choice - 1];
    char line[50], money[8], cost[8];
    gfx_moveto(0xf0, 7);
    gfx_draw_picture(weapon_picture[w]);
    crt::itoa(treasury[Israel], money);
    crt::itoa(weapon_cost[w], cost);
    std::strcpy(line, ds.str(0x4727));  // "Unit cost: $"
    std::strcat(line, cost);
    std::strcat(line, ds.str(0x4735));  // " M|"
    std::strcat(line, ds.str(0x4739));  // "Funds available: $"
    std::strcat(line, money);
    std::strcat(line, ds.str(0x474c));  // " M|"
    std::strcpy(ds.str(kWindowTitle), ds.str(weapon_name[w]));
    std::strcpy(ds.str(kWindowText), ds.str(weapon_description[w]));
    ui::text_window(0xa5, 0x78, 0x1db, 0xc3, ds.str(kWindowTitle), ds.str(kWindowText), line);
    gfx_moveto(0x15e, 0x10d);
    ui::draw_icon(ds.str(uint16_t(weapon_icon[w] * 0x80 + icons_cur)));
    int quantity = choose_quantity(w);
    gfx_restore_rect(0xa5, 0x1db, 0xd2, 0x155);
    if (quantity != 0) {
        treasury[Israel] -= int16_t(quantity * weapon_cost[w]);
        arms_order(w, turn_count) += int16_t(quantity);
        if (last_deal_month != turn_count) {
            ++supplier_deals[supplier];
            ++total_deals;
            last_deal_month = turn_count.get();
        }
    }
}

// Offers: one step, about half the affordable amount, and the affordable maximum, each rounded
// down to a multiple of the step (5, 10 or 50 units for cheaper weapons, 1 for expensive ones).
int choose_quantity(int weapon) {
    int cost = weapon_cost[weapon];
    int most = treasury[Israel] / cost;
    int step = 0;  // the original leaves it unset when cost == 60; no weapon costs 60
    if (cost < 0x3c || (fixes::fix(11) && cost == 0x3c)) step = 5;  // bug #11: cost 60 had none
    if (cost < 10) step = 10;
    if (cost < 5) step = 0x32;
    if (cost > 0x3c) step = 1;
    do --most;
    while (most % step != 0);
    if (most <= step || step == 1) most = 0;
    int half = (step + most) / 2;
    do --half;
    while (half % step != 0);
    if (half <= step || step == 1) half = 0;
    char* lines = ds.str(kWindowLines);
    char num[8];
    std::strcpy(lines, ds.str(0x4750));  // "Purchase Options:?"
    crt::itoa(step, num);
    std::strcat(lines, num);
    std::strcat(lines, ds.str(0x4763));  // " units?"
    if (half != 0) {
        std::strcat(lines, ds.str(0x476a));  // "?"
        crt::itoa(half, num);
        std::strcat(lines, num);
        std::strcat(lines, ds.str(0x476c));  // " units"
    }
    if (most != 0) {
        std::strcat(lines, ds.str(0x4773));  // "?"
        crt::itoa(most, num);
        std::strcat(lines, num);
        std::strcat(lines, ds.str(0x4775));  // " units"
    }
    std::strcat(lines, ds.str(0x477c));  // "No deal?"
    int choice = ui::menu_popup(0x1c2, 7, 0x276, 100, lines, 1);
    gfx_restore_rect(0x1c2, 0x276, 0xc, 0xaf);
    gfx_restore_rect(0xf0, 0x19a, 8, 0x161);
    reports::black_out_collapsed();
    if (half != 0 && most != 0) {
        if (choice == 1) return step;
        if (choice == 2) return half;
        if (choice == 3) return most;
        if (choice == 4) return 0;
    }
    if (half == 0 && most != 0) {
        if (choice == 1) return step;
        if (choice == 2) return most;
        if (choice == 3) return 0;
    }
    if (half == 0 && most == 0) {
        if (choice == 1) return step;
        if (choice == 2) return 0;
    }
    return 0;
}

}  // namespace conflict::arms
