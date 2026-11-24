#include "game/border.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "game/affairs.hpp"
#include "game/ai.hpp"
#include "game/crt.hpp"
#include "game/fixes.hpp"
#include "game/globals.hpp"
#include "game/sys.hpp"
#include "game/ui.hpp"

#pragma GCC diagnostic ignored "-Wrestrict"  // distinct strings inside one DGROUP buffer

namespace conflict::border {

using namespace sys;
using namespace g;

namespace {
const char* menu(uint16_t ptr_addr) { return ds.str(word_at(ptr_addr)); }
const char* icon(int index) { return ds.str(uint16_t(index * 0x80 + icons_cur)); }
// Option names: peacetime list at DS:32c6, wartime list at DS:339e (1-based option numbers).
const char* order_name(int n, int option) {
    return ds.str(word_at(uint16_t((at_war(Israel, n) == 0 ? 0x32c6 : 0x339e) + 2 * (option - 1))));
}
// Not saved: this month's order per border, the month it belongs to, front positions shown.
inline Var<int16_t> border_order(int n) { return Var<int16_t>(uint16_t(0xb2e8 + 2 * n)); }
const Var<int16_t> orders_month(0x9292);
inline Var<int16_t> front_shown(int n) { return Var<int16_t>(uint16_t(0x9282 + 2 * n)); }
constexpr uint16_t kReport = 0x91b4;     // char[200]
constexpr uint16_t kNumber = 0x927c;     // itoa scratch
constexpr uint16_t kFrontBar = 0x33ae;   // EXTRA-font bar string, edited in place
int nation_for_choice(int choice) {
    switch (choice) {
        case 1: return Egypt;
        case 2: return Jordan;
        case 3: return Lebanon;
        case 4: return Syria;
    }
    return -1;
}
}  // namespace

// 1000:7e9c
void strategic_action() {
    int reserves = army_reserves[Israel];
    int choice = 0;
    int wars = 0;
    if (at_war(Israel, Egypt) != 0 && nation_status[Egypt] != 10) wars = 1;
    if (at_war(Israel, Jordan) != 0 && nation_status[Jordan] != 10) ++wars;
    if (at_war(Israel, Lebanon) != 0 && nation_status[Lebanon] != 10) ++wars;
    if (at_war(Israel, Syria) != 0 && nation_status[Syria] != 10) ++wars;
    char* title = ds.str(kWindowTitle);
    char* text = ds.str(kWindowText);
    char* lines = ds.str(kWindowLines);
    std::strcpy(lines, ds.str(0x3641));  // ""
    gfx_draw_map(1);
    affairs::overlay_israel();
    while (choice < 5) {
        std::strcpy(title, ds.str(0x3642));  // "National Command Headquarters"
        if (wars == 0 && strike_damage == 7) {
            int r = crt::random(6);
            if (r > 3) std::strcpy(text, ds.str(0x3660));  // "The generals are itching for action."
            if (r == 1) std::strcpy(text, ds.str(0x3685));  // "Our troops want to do some marching..."
            if (r == 2) std::strcpy(text, ds.str(0x36ac));  // "The generals are getting bored."
            if (r == 3) std::strcpy(text, ds.str(0x36cc));  // "Quo vadis?"
        }
        if (wars == 1) std::strcpy(text, ds.str(0x36d7));  // "We are currently at war."
        if (wars == 2) {
            std::strcpy(text, ds.str(0x36f0));  // "At the present we have conflict on two fronts."
            std::strcat(text, ds.str(0x371f));  // "This could over stretch our defences..."
        }
        if (wars > 2) std::strcpy(text, ds.str(0x3747));  // "We are now surounded by enemy attack.This coul…"
        if (strike_damage != 7) {  // damage from enemy strikes: 100s digit = kind, rest = nation
            if (strike_damage < 100) {
                std::strcpy(text, ds.str(0x3785));  // "We received light military damage from "
                std::strcat(text, ds.str(nation_adjective[strike_damage]));
                std::strcat(text, ds.str(0x37ad));  // " strike attacks."
            }
            if (strike_damage > 99 && strike_damage < 200) {
                std::strcpy(text, ds.str(0x37be));  // "We received heavy equipment loss from "
                std::strcat(text, ds.str(nation_adjective[strike_damage - 100]));
                std::strcat(text, ds.str(0x37e5));  // " strike attacks."
            }
            if (strike_damage > 199 && strike_damage < 300) {
                std::strcpy(text, ds.str(0x37f6));  // "Some industrial damage was done by recent "
                std::strcat(text, ds.str(nation_adjective[strike_damage - 200]));
                std::strcat(text, ds.str(0x3821));  // " strike bombings."
            }
            if (strike_damage > 299) {
                std::strcpy(text, ds.str(0x3833));  // "Many innocent civilians have been hit by the r…"
                std::strcat(text, ds.str(nation_adjective[strike_damage - 300]));
                std::strcat(text, ds.str(0x3868));  // " bombings."
            }
            if (nation_status[Israel] > 2) std::strcat(text, ds.str(0x3873));  // "We demand a counter strike!"
        }
        if (strike_damage == 7) {
            for (int n = 0; n < 7; ++n) {
                if (relations(Israel, n) == 2 && nation_status[n] < 10 && border_brigades[n] == 0) {
                    std::strcpy(lines, ds.str(0x388f));  // "|We should prepare a reception|party for any "
                    std::strcat(lines, ds.str(nation_adjective[n]));
                    std::strcat(lines, ds.str(0x38bd));  // " visitors|"
                }
                if (relations(Israel, n) == 1 && forces(Israel, n, Troops) != 0 && at_war(Israel, n) == 0) {
                    std::strcpy(lines, ds.str(0x38c8));  // "|We are ready to charge into|"
                    std::strcat(lines, ds.str(nation_name[n]));
                    std::strcat(lines, ds.str(0x38e6));  // " with all guns blazing!!|"
                }
                if (forces(Israel, Israel, EarlyWarning) != 0 && rival[n] == Israel &&
                    relations(Israel, n) == 3 && nation_status[n] != 10) {
                    std::strcpy(lines, ds.str(0x3900));  // "|Surveillance warning:Enemy "
                    std::strcat(lines, ds.str(0x391d));  // "|mobilisation on "
                    std::strcat(lines, ds.str(nation_adjective[n]));
                    std::strcat(lines, ds.str(0x392f));  // " border|"
                }
                if (forces(n, Israel, Troops) != 0 && border_brigades[n] == 0) {
                    std::strcpy(lines, ds.str(0x3938));  // "|Warning!! Enemy troop activity "
                    std::strcat(lines, ds.str(0x3959));  // "|on undefended "
                    std::strcat(lines, ds.str(nation_adjective[n]));
                    std::strcat(lines, ds.str(0x3969));  // " border|"
                }
            }
        }
        if (reserves < 1000) {
            std::strcpy(text, ds.str(0x3972));  // "Our total losses have eaten away at our reserv…"
            std::strcat(text, ds.str(0x39aa));  // "We now have less than 100,000 people to call up."
        }
        gfx_restore_rect(10, 0x276, 8, 0x155);
        affairs::overlay_israel();
        if (std::strlen(text) < 0x47) ui::text_window(0x140, 0x84, 0x276, 0xc3, title, text, lines);
        else ui::text_window(0xe6, 0x6e, 0x276, 0xc3, title, text, lines);
        menu_context = 4;
        choice = ui::menu_popup(0x1ae, 5, 0x276, 100, menu(0x0cce), 0);
        menu_context = 0;
        gfx_restore_rect(0xe6, 0x276, 0xc0, 0x155);
        gfx_restore_rect(0x1ae, 0x276, 8, 0xaf);
        affairs::overlay_israel();
        if (choice < 5) {
            border_command(choice);
            int n = nation_for_choice(choice);
            std::strcpy(lines, ds.str(0x39db));  // ""
            if (strike_order[n] != 0) std::strcpy(lines, ds.str(0x39dc));  // commend your strength
            if (strike_order[n] == 5) std::strcpy(lines, ds.str(0x3a16));  // commend your courage
        }
    }
}

// 1000:8462
void border_command(int choice) {
    int n = nation_for_choice(choice);
    if (n < 0) {
        std::fputs(ds.str(0x3a4c), stderr);  // "Illegal border country"
        if (fixes::fix(13)) return;  // bug #13: no border to command
        n = 0;
    }
    if (orders_month != turn_count) {
        for (int i = 0; i < 8; ++i) border_order(i) = 0;
        orders_month = turn_count.get();
    }
    int rel = relations(Israel, n);
    if (nation_status[n] == 10) occupied_border(n);
    if (nation_status[n] == 10) return;
    if (border_order(n) != 0) orders_given(n);
    if (border_order(n) != 0) return;
    int options[12];
    int count = 0;
    if (at_war(Israel, n) == 0) {
        if (rel > 1 && border_brigades[n] < 5 && border_brigades[Israel] != 0) {
            count = 1;
            options[0] = 1;
        }
        if ((rel < 4 || border_brigades[n] != 0) && border_brigades[n] < 5 &&
            border_brigades[n] + border_brigades[Israel] > 4)
            options[count++] = 2;
        if (rel < 4 && border_brigades[n] > 0) options[count++] = 3;
        if (rel < 5 && rel > 1 && forces(Israel, Israel, Bombers) != 0) {
            if (rel == 4) options[count++] = 4;
            else if (nuclear[n] < 7) options[count++] = 5;
            options[count++] = 6;
        }
        if (nuclear[n] > 6 && forces(Israel, Israel, Bombers) != 0 && rel > 1) options[count++] = 7;
        if (border_brigades[n] > 0) options[count++] = 8;
        if (rel == 1 && border_brigades[n] == 0) options[count++] = 9;
        if (border_brigades[n] > 4) options[count++] = 0xb;
        options[count++] = 10;
    }
    if (at_war(Israel, n) != 0) {
        if (border_brigades[Israel] != 0) options[0] = 1;
        count = border_brigades[Israel] != 0 ? 1 : 0;
        if (forces(Israel, Israel, MainBattleTanks) > 0x32) options[count++] = 2;
        if (forces(Israel, Israel, Sams) > 0) options[count++] = 3;
        if (forces(Israel, Israel, Fighters) > 0 || forces(Israel, Israel, AttackHelicopters) > 0 ||
            forces(Israel, Israel, Gunships) > 0)
            options[count++] = 4;
        if (count != 0 || forces(Israel, Israel, LightTanks) != 0) options[count++] = 5;
        if (at_war(Israel, n) > 1) options[count++] = 6;
        if (nuclear[Israel] == 10) options[count++] = 7;
        options[count++] = 8;
    }
    char orders[402];
    std::strcpy(orders, ds.str(0x3a63));  // "          front?"
    for (int i = 0; i < count; ++i) {
        std::strcat(orders, order_name(n, options[i]));
        std::strcat(orders, ds.str(0x3a74));  // "?"
    }
    char* title = ds.str(kWindowTitle);
    char* text = ds.str(kWindowText);
    char* lines = ds.str(kWindowLines);
    std::strcpy(title, ds.str(nation_adjective[n]));
    std::strcat(title, ds.str(0x3a76));  // " border command report"
    std::strcpy(text, ds.str(0x3a8e));  // "<"
    if (at_war(Israel, n) != 0) std::strcat(text, ds.str(war_status_text[n]));  // war status
    if (at_war(Israel, n) == 0) affairs::overlay_israel();
    std::strcat(text, ds.str(border_report(n)));
    std::strcat(text, ds.str(0x3a90));  // ">"
    std::strcpy(lines, ds.str(0x3a92));  // "|"
    int pick;
    int len = int(std::strlen(orders));
    if (at_war(Israel, n) == 0) {
        ui::text_window(300, 0x91, 0x276, 0xc3, title, text, lines);
        if (len < 0x79) {
            if (len < 0x51) pick = ui::menu_popup(0x1a4, 5, 0x276, 0x50, orders, n);
            else pick = ui::menu_popup(400, 5, 0x276, 100, orders, n);
        } else {
            pick = ui::menu_popup(200, 5, 0x276, 0x8f, orders, n);
        }
    } else {
        ui::text_window(300, 0x8f, 0x276, 0xc3, title, text, lines);
        if (count == 8) pick = ui::menu_popup(0x154, 5, 0x276, 0x93, orders, n);
        else if (len < 0x32) pick = ui::menu_popup(0x1a4, 5, 0x276, 0x50, orders, n);
        else if (len < 0x4c) pick = ui::menu_popup(0x1a4, 5, 0x276, 0x8c, orders, n);
        else pick = ui::menu_popup(0x154, 5, 0x276, 0x8c, orders, n);
    }
    gfx_restore_rect(200, 0x276, 8, 0x155);
    if (at_war(Israel, n) != 0) {
        gfx_restore_rect(0x3c, 0x10e, 0x15, 0x11f);
        gfx_restore_rect(5, 0x118, 0x126, 0x155);
    }
    int order = options[pick - 1];
    border_order(n) = int16_t(order);
    if (at_war(Israel, n) == 0) {
        switch (order) {
            case 1: post_brigade(n); break;
            case 2:
                mass_on_border(n);
                us_disapproval += 5;
                break;
            case 3: {  // provoke: relations broken off
                front_line[n] += int16_t(relations(Israel, n) + 1);
                relations(Israel, n) = 1;
                if (rel != 1) ai::relation_news(Israel, n);
                int r = relations(Israel, n);
                us_disapproval += int16_t(r * 5 + 10);
                acts_of_violence += int16_t(r + 5);
                break;
            }
            case 4: case 5: case 6: case 7:  // strikes (strike_order = 1..4)
                strike_order[n] = int16_t(order - 3);
                acts_of_violence += order - 3 == 4 ? 4 : 2;
                break;
            case 8: pull_back(n); break;
            case 9: match_enemy(n); break;
            case 10: border_order(n) = 0; break;
            case 0xb: commit_all(n); break;
        }
    }
    if (at_war(Israel, n) != 0) {
        switch (order) {
            case 1:
                ++border_brigades[n];
                --border_brigades[Israel];
                break;
            case 2: reinforce_armour(n); break;
            case 3: sam_cover(n); break;
            case 4: air_support(n); break;
            case 5: commit_all(n); break;
            case 6: withdraw_brigade(n); break;
            case 7:  // nuclear strike, confirmed
                strike_order[n] = 5;
                if (ui::menu_popup(0xdc, 0x32, 0x1a4, 0x82, menu(0x0cee), 0) == 2) strike_order[n] = 0;
                else acts_of_violence += 10;
                gfx_restore_rect(0xdc, 0x1a4, 0x57, 0xe3);
                break;
            case 8: border_order(n) = 0; break;
            default:
                std::fputs(ds.str(0x3a94), stderr);  // "illegal case"
                std::exit(1);
        }
    }
}

// 1000:76f4
uint16_t border_report(int n) {
    char* report = ds.str(kReport);
    char* number = ds.str(kNumber);
    char* bar = ds.str(kFrontBar);
    int y = 0x19;
    if (turn_count == 1) std::memset(report, 0, 200);
    int war = at_war(Israel, n);
    if (war != 0) {
        if (war == 1) {
            front_shown(n) = 0;
            std::strcpy(bar, ds.str(0x33b9));  // "====><===="
        }
        int troops = forces(n, Israel, Troops) - forces(Israel, n, Troops);
        int tanks = forces(n, Israel, MainBattleTanks) - forces(Israel, n, MainBattleTanks);
        int air = forces(n, Israel, Fighters) - forces(Israel, n, Fighters);
        int sams = forces(n, Israel, Sams);
        std::strcpy(report, ds.str(0x33c4));  // ""
        if (air > 10 && air < 0x14 && at_war(Israel, n) < 3) std::strcpy(report, ds.str(0x33c5));  // "The opposition has air superiority;"
        if (air > 10 && air < 0x14 && at_war(Israel, n) > 2) std::strcpy(report, ds.str(0x33e9));  // "The opposition still has air superiority;"
        if (air > 0x13 && air < 0x1e) std::strcpy(report, ds.str(0x3413));  // "We badly need more air cover;"
        if (air > 0x1d) std::strcpy(report, ds.str(0x3431));  // "Our pilots are demanding more air cover;"
        if (air > 10 && forces(Israel, n, Fighters) < air) std::strcpy(report, ds.str(0x345a));  // "The skies are full of enemy jets!"
        if (troops > 100 && tanks < 200) std::strcat(report, ds.str(0x347c));  // "We need more troops on the front."
        if (tanks > 200 && troops < 100) std::strcat(report, ds.str(0x349e));  // "We need more tanks."
        if (sams > 0x32 && air < 0) std::strcat(report, ds.str(0x34b2));  // "Our fighters have come against heavy SAM attack."
        if (troops > 100 && tanks > 200) std::strcat(report, ds.str(0x34e3));  // "We are outnumbered in ground forces."
        // Two force panels: ours on this front, and theirs (rounded to tens).
        ui::draw_titled_panel(0x3c, 0xc, 0x8e, 0xa4, ds.str(0x3508));  // "ISRAEL"
        for (int k = 0; k < 8; ++k) {
            if (k == 2 || (k == 6 && forces(Israel, n, Gunships) == 0)) continue;
            double d = ega_rows(y);
            gfx_moveto(0x40, int(d));
            ui::draw_icon(icon(force_icon[k]));
            gfx_moveto(0x68, int(d + 6.0));
            crt::itoa(forces(Israel, n, k), number);
            if (forces(Israel, n, k) == 0) std::strcpy(number, ds.str(0x350f));  // "NONE"
            gfx_text_small(number);
            y += 0x14;
        }
        y = 0x19;
        ui::draw_titled_panel(0xbe, 0xc, 0x110, 0xa4, ds.str(nation_name[n]));
        for (int k = 0; k < 8; ++k) {
            if (k == 2 || (k == 6 && forces(n, Israel, Gunships) == 0)) continue;
            double d = ega_rows(y);
            gfx_moveto(0xc1, int(d));
            ui::draw_icon(icon(force_icon[k]));
            gfx_moveto(0xea, int(d + 6.0));
            int v = forces(n, Israel, k) / 10 * 10;
            crt::itoa(v, number);
            if (v == 0) std::strcpy(number, ds.str(0x3514));  // "NONE"
            gfx_text_small(number);
            y += 0x14;
        }
        // Front line: flash the old position if it moved, then draw the new one.
        ui::draw_titled_panel(0x32, 0xa8, 0x118, 0xc3, ds.str(0x3519));  // "FRONT LINE"
        gfx_moveto(0x36, 0x142);
        gfx_text_small(ds.str(0x3524));  // "Israel"
        gfx_moveto(0xd4, 0x142);
        gfx_text_small(ds.str(nation_name[n]));
        // Bug #23: the markers go at front + 4 / + 5 of a 10-character bar, so a front at +-5
        // writes outside it. Bug #15: the bar lives in static data and survives loads.
        auto place = [&](int front) {
            if (fixes::fix(23)) front = front < -4 ? -4 : (front > 4 ? 4 : front);
            bar[front + 4] = '>';
            bar[front + 5] = '<';
        };
        if (fixes::fix(15) && war != 1) {  // rebuild the bar at the position last shown
            std::strcpy(bar, ds.str(0x352b));  // "=========="
            place(front_shown(n));
        }
        gfx_moveto(0x6c, 0x142);
        font_small = font_extra;
        gfx_text_small(bar);
        uint16_t fg = fg_colour;
        if (front_shown(n) != front_line[n]) {
            for (int i = 0; i < 8; ++i) {
                fg_colour = 15;
                for (int r = 0; r < 7; ++r) {
                    int last = front_shown(n);
                    if (fixes::fix(23)) last = last < -4 ? -4 : (last > 4 ? 4 : last);
                    gfx_fill_rect_row((last + 4) * 10 + 0x6c, (last + 6) * 10 + 0x6c, int(double(r) + 322.0), 1);
                }
                delay_loop(9000);
                gfx_moveto(0x6c, 0x142);
                fg_colour = 0;
                gfx_text_small(bar);
                delay_loop(9000);
            }
        }
        fg_colour = 15;
        for (int r = 0; r < 7; ++r) gfx_fill_rect_row(0x6c, 0xd2, int(double(r) + 322.0), 1);
        fg_colour = fg;
        std::strcpy(bar, ds.str(0x352b));  // "=========="
        place(front_line[n]);
        gfx_moveto(0x6c, 0x142);
        gfx_text_small(bar);
        font_small = font_etech;
        front_shown(n) = front_line[n].get();
    }
    if (at_war(Israel, n) == 0) {
        std::strcpy(report, ds.str(0x3536));  // "The "
        std::strcat(report, ds.str(nation_adjective[n]));
        std::strcat(report, ds.str(0x353b));  // " front"
        int theirs = forces(n, Israel, Troops);
        if (theirs == 0) std::strcat(report, ds.str(0x3542));  // " is peaceful."
        if (theirs > 0 && theirs < 500) std::strcat(report, ds.str(0x3550));  // " has some troop activity."
        if (theirs > 499 && theirs < 1000) std::strcat(report, ds.str(0x356a));  // " has large scale enemy activity."
        if (theirs > 999) std::strcat(report, ds.str(0x358b));  // " has full scale deployment."
        int ours = forces(Israel, n, Troops);
        if (ours == 0) std::strcat(report, ds.str(0x35a7));  // "Our presence is merely provisional."
        if (ours > 200 && ours < 1000) {
            crt::itoa(border_brigades[n], number);
            std::strcat(report, ds.str(0x35cb));  // " We have "
            std::strcat(report, number);
            std::strcat(report, ds.str(0x35d4));  // " troop brigades stationed here."
        }
        if (ours > 4 && ours < 0xc9) std::strcat(report, ds.str(0x35f4));  // "We have a single brigade stationed here."
        if (ours > 999) std::strcat(report, ds.str(0x361d));  // "We are at full deployment strength."
    }
    return kReport;
}

void occupied_border(int n) {
    affairs::overlay_israel();
    ui::menu_popup(400, 5, 0x276, 0x3c, menu(0x0cf0), n);
    gfx_restore_rect(400, 0x276, 8, 0x69);
}

void orders_given(int n) {
    char* title = ds.str(kWindowTitle);
    char* text = ds.str(kWindowText);
    char* lines = ds.str(kWindowLines);
    std::strcpy(lines, ds.str(0x3ad0));  // "|"
    std::strcpy(title, ds.str(0x3ad2));  // "Border command report"
    std::strcpy(text, ds.str(0x3ae9));   // "Orders for this month: "
    std::strcat(text, order_name(n, border_order(n)));
    ui::text_window(0x140, 0x87, 0x276, 0xc3, title, text, lines);
    ui::menu_popup(0x17c, 5, 0x276, 0x50, menu(0x0cd0), n);
    gfx_restore_rect(0x17c, 0x276, 8, 0x8c);
    gfx_restore_rect(0x140, 0x276, 0xec, 0x155);
}

void post_brigade(int n) {
    ++border_brigades[n];
    --border_brigades[Israel];
    if (forces(Israel, Israel, MainBattleTanks) > 0x95) {
        forces(Israel, Israel, MainBattleTanks) -= 0x96;
        forces(Israel, n, MainBattleTanks) += 0x96;
    }
    if (forces(Israel, Israel, LightTanks) > 199) {
        forces(Israel, Israel, LightTanks) -= 200;
        forces(Israel, n, LightTanks) += 200;
    }
    if (forces(Israel, Israel, Sams) > 9) {
        forces(Israel, Israel, Sams) -= 10;
        if (fixes::fix(2)) forces(Israel, n, Sams) += 10;  // bug #2: they are SAMs
        else forces(Israel, n, LightTanks) += 10;          // the original adds them to light tanks
    }
}

void mass_on_border(int n) {
    int need = (1000 - forces(Israel, n, Troops)) / 200;
    if (border_brigades[Israel] < need) {
        border_brigades[n] += border_brigades[Israel].get();
        border_brigades[Israel] = 0;
    } else {
        border_brigades[n] = 5;
        border_brigades[Israel] -= int16_t(need);
    }
    for (int k = 1; k < 9; ++k) {
        if (k == EarlyWarning || k == Bombers) continue;
        int t = (forces(Israel, Israel, k) << 1) / 3;
        forces(Israel, Israel, k) -= int16_t(t);
        forces(Israel, n, k) += int16_t(t);
    }
}

void pull_back(int n) {
    border_brigades[Israel] += border_brigades[n].get();
    border_brigades[n] = 0;
    for (int k = 1; k < 9; ++k) {
        forces(Israel, Israel, k) += forces(Israel, n, k).get();
        forces(Israel, n, k) = 0;
    }
    ++relations(Israel, n);
}

void match_enemy(int n) {
    int enemy = forces(n, n, Troops) + forces(n, Israel, Troops);
    int ours = border_brigades[n] * 200;
    if (ours <= enemy) {
        border_brigades[n] += border_brigades[Israel].get();
        border_brigades[Israel] = 0;
    } else {
        int c = border_brigades[Israel];
        while (c * 200 > enemy) --c;
        border_brigades[n] += int16_t(c);
        border_brigades[Israel] -= int16_t(c);
    }
    for (int k = 1; k < 9; ++k) {
        if (k == Bombers || k == EarlyWarning) continue;
        int need = forces(Israel, n, k) - (forces(n, n, k) + forces(n, Israel, k));
        int avail = forces(Israel, Israel, k);
        if (need < avail) {
            forces(Israel, n, k) += int16_t(avail);
            forces(Israel, Israel, k) = 0;
        } else {
            forces(Israel, Israel, k) -= int16_t(need);
            forces(Israel, n, k) += int16_t(need);
        }
    }
}

void commit_all(int n) {
    border_brigades[n] += border_brigades[Israel].get();
    border_brigades[Israel] = 0;
    for (int k = 1; k < 9; ++k) {
        if (k == EarlyWarning || k == Bombers) continue;
        forces(Israel, n, k) += forces(Israel, Israel, k).get();
        forces(Israel, Israel, k) = 0;
    }
}

void reinforce_armour(int n) {
    int need = (forces(n, Israel, MainBattleTanks) - forces(Israel, n, MainBattleTanks)) + 0x32;
    if (need < 0) need = 0x32;
    if (forces(Israel, Israel, MainBattleTanks) < need) {
        forces(Israel, n, MainBattleTanks) += forces(Israel, Israel, MainBattleTanks).get();
        forces(Israel, Israel, MainBattleTanks) = 0;
    } else {
        forces(Israel, n, MainBattleTanks) += int16_t(need);
        forces(Israel, Israel, MainBattleTanks) -= int16_t(need);
    }
    if (forces(Israel, Israel, LightTanks) < 0x32) {
        forces(Israel, n, LightTanks) += forces(Israel, Israel, LightTanks).get();
        forces(Israel, Israel, LightTanks) = 0;
    } else {
        forces(Israel, n, LightTanks) += 0x32;
        forces(Israel, Israel, LightTanks) -= 0x32;
    }
}

void sam_cover(int n) {
    int want = border_brigades[n] * 10;
    if (want <= forces(Israel, Israel, Sams)) {
        forces(Israel, n, Sams) += int16_t(want);
        forces(Israel, Israel, Sams) -= int16_t(want);
    } else {
        forces(Israel, n, Sams) += forces(Israel, Israel, Sams).get();
        forces(Israel, Israel, Sams) = 0;
    }
}

void air_support(int n) {
    int fighters = (forces(n, Israel, Fighters) - forces(Israel, n, Fighters)) + 0x14;
    int helicopters = (forces(n, Israel, AttackHelicopters) - forces(Israel, n, AttackHelicopters)) + 10;
    if (forces(Israel, Israel, Fighters) < fighters) {
        forces(Israel, n, Fighters) += forces(Israel, Israel, Fighters).get();
        forces(Israel, Israel, Fighters) = 0;
    } else {
        forces(Israel, n, Fighters) += int16_t(fighters);
        forces(Israel, Israel, Fighters) -= int16_t(fighters);
    }
    if (forces(Israel, Israel, AttackHelicopters) < helicopters) {
        forces(Israel, n, AttackHelicopters) += forces(Israel, Israel, AttackHelicopters).get();
        forces(Israel, Israel, AttackHelicopters) = 0;
    } else {
        forces(Israel, n, AttackHelicopters) += int16_t(helicopters);
        forces(Israel, Israel, AttackHelicopters) -= int16_t(helicopters);
    }
    if (forces(Israel, Israel, Gunships) != 0) {
        int half = forces(Israel, Israel, Gunships) / 2;
        forces(Israel, Israel, Gunships) -= int16_t(half);
        forces(Israel, n, Gunships) += int16_t(half);
    }
}

void withdraw_brigade(int n) {
    if (border_brigades[n] != 0) {
        --border_brigades[n];
        ++border_brigades[Israel];
    }
}

}  // namespace conflict::border
