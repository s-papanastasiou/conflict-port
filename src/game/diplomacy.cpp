#include "game/diplomacy.hpp"

// GCC cannot tell that two offsets into the DGROUP buffer name distinct strings.
#pragma GCC diagnostic ignored "-Wrestrict"

#include <cstring>

#include "game/ai.hpp"
#include "game/crt.hpp"
#include "game/fixes.hpp"
#include "game/globals.hpp"
#include "game/news.hpp"
#include "game/sys.hpp"
#include "game/ui.hpp"

namespace conflict::diplomacy {

using namespace sys;
using namespace g;
using crt::random;

namespace {
const char* menu(uint16_t ptr_addr) { return ds.str(word_at(ptr_addr)); }
const char* str_at(uint16_t ptr_addr) { return ds.str(word_at(ptr_addr)); }

void expert_opinion(uint16_t title, uint16_t text_ptr, uint16_t lines, long pause) {
    ui::text_window(0x50, 0x1e, 0x15e, 0x5f, ds.str(title), str_at(text_ptr), ds.str(lines));
    wait_or_key(pause);
    gfx_restore_rect(0x50, 0x15e, 0x34, 0xa6);
}
}  // namespace

// 1000:61b0
void diplomacy(int n) {
    int wars = 0;
    char comment[110] = {};
    int rel_israel = relations(Israel, n);
    char* title = ds.str(kWindowTitle);
    char* lines = ds.str(kWindowLines);
    std::strcpy(title, ds.str(0x2820));
    std::strcat(title, ds.str(nation_adjective[n]));
    std::strcat(title, ds.str(0x2821));  // " diplomatic relations"
    if (rel_israel < 2) {
        std::strcpy(lines, ds.str(0x2860));  // "We have no formal relations with "
        std::strcat(lines, ds.str(nation_name[n]));
        std::strcat(lines, ds.str(0x2883));
    } else {
        std::strcpy(lines, ds.str(0x2837));  // "Our relations with "
        std::strcat(lines, ds.str(nation_name[n]));
        std::strcat(lines, ds.str(0x284c));  // " at present are "
        std::strcat(lines, ds.str(word_at(uint16_t(0xe08 + 2 * rel_israel))));
        std::strcat(lines, ds.str(0x285d));
    }
    int their_rival = rival[n];
    uint16_t tmpl = word_at(0x2806);
    if (their_rival == Israel && rel_israel > 4) tmpl = word_at(0x2808);
    if (rel_israel == 9 && sa04e[n] <= sa04e[Israel]) tmpl = word_at(0x280a);
    if (rel_israel == 9 && sa04e[Israel] < sa04e[n]) tmpl = word_at(0x280c);
    if (rel_israel > 6)
        for (int i = 0; i < 8; ++i)
            if ((i < n && at_war(n, i) != 0) || (n < i && at_war(i, n) != 0)) ++wars;
    if (wars != 0 && rel_israel < 10) tmpl = word_at(0x280e);
    if (rel_israel > 3 && forces(Israel, n, 0) > 0) tmpl = word_at(0x2812);
    if (sa15e[n] == 0 && at_war(Israel, n) == 1) tmpl = word_at(0x2814);
    if (sa15e[n] == 1) tmpl = word_at(0x2816);
    if (sa15e[n] == 2) tmpl = word_at(0x2818);
    if (nation_status[n] == 10) tmpl = word_at(0x281a);
    if (sa0de[n] != 0 && sa0de[n] < rel_israel) tmpl = word_at(0x281e);
    if (sa0de[n] != 0 && rel_israel < sa0de[n]) tmpl = word_at(0x281c);
    news::format(ds.str(tmpl), n, n, comment);
    if (relations(Israel, their_rival) > 6 && rel_israel > 6) {
        std::strcpy(comment, ds.str(0x289a));
        std::strcat(comment, ds.str(nation_adjective[n]));
        std::strcat(comment, ds.str(0x289f));  // " government is unhappy about our r..."
        std::strcat(comment, ds.str(nation_name[their_rival]));
        std::strcat(comment, ds.str(0x28d0));
    }
    ui::text_window(0x140, 0x78, 0x276, 0xbe, title, comment, lines);
    if (nation_status[n] == 10) {
        ui::menu_popup(0x1c2, 5, 0x276, 0x3c, menu(0x0cdc), n);
        gfx_restore_rect(0x1c2, 0x276, 8, 0x69);
        gfx_restore_rect(0x140, 0x276, 0xd2, 0x14c);
        return;
    }
    // Directive already given this month: change it, or keep it and leave.
    if (sa0de[n] != 0 && ui::menu_popup(0x1c2, 5, 0x276, 0x48, menu(0x0cb0), n) == 2) {
        gfx_restore_rect(0x1c2, 0x276, 8, 0x7e);
        gfx_restore_rect(0x140, 0x276, 0xd2, 0x14c);
        return;
    }
    if (rel_israel > 1 && rel_israel < 10) {
        long pause = long(22000.0 / double(float(delay_calibration)));
        int choice;
        if (rel_israel == 9 && relations(Israel, their_rival) < 7)
            choice = ui::menu_popup(0x1bc, 5, 0x276, 0x52, menu(0x0cba), n);
        else if (rel_israel < 7)
            choice = ui::menu_popup(400, 5, 0x276, 0x68, menu(0x0cb6), n);
        else
            choice = ui::menu_popup(400, 5, 0x276, 0x68, menu(0x0cb8), n);
        if (choice == 1 && relations(Israel, their_rival) > 6) expert_opinion(0x28d4, 0x2526, 0x28d2, pause);
        if (choice == 1 && forces(Israel, n, 0) != 0) expert_opinion(0x28e5, 0x24d8, 0x28e3, pause);
        if (choice == 1 && rel_israel < 4) expert_opinion(0x28f6, 0x2576, 0x28f4, pause);
        if (choice == 1 && rival[n] != Israel && rel_israel > 3) {
            if (rel_israel > 6 && treasury[Israel] > 0x96) {  // buy goodwill
                treasury[Israel] -= 0x96;
                treasury[n] += 0x96;
            }
            if (relations(Israel, their_rival) < 7 && forces(Israel, n, 0) == 0 &&
                rel_israel - random(4) > 2) {
                sa0de[n] = int16_t(rel_israel + 1);
                if (wars != 0 && rel_israel < 9) sa0de[n] = int16_t(rel_israel + 2);
            } else {
                sa0de[n] = int16_t(rel_israel);
            }
            if (sa0de[n] == 10 && sa04e[Israel] < 8 && turn_count < 10) ++sa04e[Israel];
        } else {
            sa0de[n] = int16_t(rel_israel);
        }
        if (choice == 2 && (rel_israel > 4 || (rel_israel < 5 && border(Israel, n) == 1)))
            sa0de[n] = int16_t(rel_israel - 1);
    }
    if (rel_israel > 9) {
        int choice = ui::menu_popup(0x1bd, 5, 0x276, 0x5f, menu(0x0cbc), n);
        if (choice == 1) sa0de[n] = int16_t(rel_israel);
        if (choice == 2) sa0de[n] = int16_t(rel_israel - 1);
    }
    if (rel_israel == 1 && at_war(Israel, n) == 0) {
        ui::menu_popup(0x1c2, 5, 0x276, 100, menu(0x0cea), n);
        sa0de[n] = 1;
    }
    if (rel_israel == 1 && at_war(Israel, n) != 0) {
        if (sa15e[n] == 1) {  // they offered peace
            if (ui::menu_popup(0x1ae, 5, 0x276, 0x4b, menu(0x0cda), n) == 1) {
                std::strcpy(title, ds.str(0x2905));  // "PEACE AGREEMENT"
                char* text = ds.str(kWindowText);
                std::strcpy(text, ds.str(0x2916));   // "Israel and "
                std::strcat(text, ds.str(nation_name[n]));
                std::strcat(text, ds.str(0x295c));   // " have accepted an immediate cease..."
                std::strcpy(lines, ds.str(0x29fb));
                ui::text_window(0x82, 100, 500, 0xc3, title, text, lines);
                if (ui::menu_popup(0x1ae, 5, 0x276, 0x55, menu(0x0cf2), 0) == 1) make_peace(Israel, n);
                gfx_restore_rect(0x82, 500, 0xaf, 0x155);
                gfx_restore_rect(0x1ae, 0x276, 8, 0x94);
            } else {
                peace_news(Israel, n, 2);
            }
            sa15e[n] = 0;
        } else {
            sa15e[n] = ui::menu_popup(0x1c2, 5, 0x276, 0x5a, menu(0x0cd8), n) == 1 ? 2 : 0;
            sa0de[n] = 1;
        }
    }
    gfx_restore_rect(400, 0x276, 8, 0xd2);
    gfx_restore_rect(0x140, 0x276, 0xd2, 0x14c);
}

// 1000:6fa6
void intelligence(int n) {
    int instability = nation_status[n];
    int insurgency = sa11e[n];
    char* title = ds.str(kWindowTitle);
    char* text = ds.str(kWindowText);
    char* lines = ds.str(kWindowLines);
    std::strcpy(title, ds.str(0x2e58));  // "Psychopolitical warfare ..."
    std::strcpy(lines, ds.str(0x2e83));  // "Internal stability: "
    std::strcat(lines, ds.str(word_at(uint16_t(0x2a94 + 2 * instability))));
    std::strcat(lines, ds.str(0x2e99));
    if (instability == 10) {
        int controller = budget[n];  // a collapsed nation's budget slot holds its new master
        if (controller < 8) {
            if (controller > -1) {
                std::strcat(lines, ds.str(0x2eef));  // "Controlling power: "
                std::strcat(lines, ds.str(nation_name[controller]));
                if (nation_status[controller] == 10) {
                    std::strcat(lines, ds.str(0x2f03));
                    std::strcat(lines, ds.str(nation_name[budget[controller]]));
                }
                std::strcat(lines, ds.str(0x2f09));
            }
        } else {
            std::strcat(lines, ds.str(0x2ed6));  // "Controlling faction: "
            std::strcat(lines, ds.str(word_at(uint16_t(0xfae + 2 * sa13e[n]))));
            std::strcat(lines, ds.str(0x2eec));
        }
    } else if (insurgency != 0) {
        if (insurgency == 1) {
            std::strcat(lines, ds.str(0x2e9c));  // "Supported pressure group: "
            std::strcat(lines, ds.str(word_at(uint16_t(0xfae + 2 * sa13e[n]))));
        } else {
            std::strcat(lines, ds.str(0x2eb7));  // "State of opposition group: "
            std::strcat(lines, ds.str(word_at(uint16_t(0x2e4c + 2 * (insurgency / 2)))));
        }
        std::strcat(lines, ds.str(0x2ed3));
    }
    int danger = ((insurgency + instability) - sa04e[n]) - 4;
    if (danger < -2) std::strcpy(text, str_at(0x2dd8));
    else if (danger < 1) std::strcpy(text, str_at(0x2dda));
    if (danger > 0 && danger < 10) std::strcpy(text, str_at(uint16_t(0x2dd8 + 2 * danger)));
    if (danger > 9 && danger < 12) std::strcpy(text, str_at(0x2dea));
    if (danger > 11) std::strcpy(text, str_at(0x2dec));
    if (danger > 2 && danger < 9 && at_war(Israel, n) != 0) std::strcpy(text, str_at(0x2df0));
    if (instability == 9) std::strcpy(text, str_at(0x2dee));
    if (instability == 10 && budget[n] > 7) std::strcpy(text, str_at(0x2df4));
    if (instability == 10 && budget[n] < 8) std::strcpy(text, str_at(0x2df2));
    ui::text_window(0xfa, 0x78, 0x276, 0xc3, title, text, lines);
    if (sa0ee[n] != 0 && ui::menu_popup(0x1b8, 5, 0x276, 0x50, menu(0x0cb2), n) == 2) {
        gfx_restore_rect(0x1b8, 0x276, 8, 0x8c);
        gfx_restore_rect(0xfa, 0x276, 0xd2, 0x155);
        return;
    }
    int choice;
    if (insurgency == 0)
        choice = ui::menu_popup(0x1a9, 5, 0x276, 0x5c, menu(0x0cc0), n);
    else if (nation_status[n] == 10 || (insurgency < 8 && instability < 6))
        choice = ui::menu_popup(0x154, 5, 0x276, 0x69, menu(0x0cbe), n);
    else
        choice = ui::menu_popup(0x140, 5, 0x276, 100, menu(0x0cfa), n);
    if (random(5) < 2) {
        sa0ee[n] = int16_t(insurgency);  // the operation fails to change anything
    } else {
        if (choice == 1 && instability < 9) {
            // Bug #16: the original charges even when the insurgency is already at maximum.
            if (!fixes::fix(16) || insurgency < 10) treasury[Israel] -= 2;
            if (insurgency < 10) sa0ee[n] = int16_t(insurgency + 1);
        }
        if (choice == 2 && insurgency != 0) {
            treasury[Israel] -= 2;
            sa0ee[n] = int16_t(insurgency - 1);
        }
    }
    if (instability < 10 && (insurgency > 7 || instability > 5) && choice == 3) {
        gfx_restore_rect(0xfa, 0x276, 0xc0, 0x155);
        gfx_restore_rect(0x140, 0x276, 8, 0xb7);
        int extreme = ui::menu_popup(400, 5, 0x276, 0x5a, menu(0x0cfc), n);
        if (extreme == 1) covert_action[n] = 1;  // assassinate leader
        if (extreme == 2) covert_action[n] = 2;  // start coup
        sa0ee[n] = int16_t(insurgency);
    }
    gfx_restore_rect(0xfa, 0x276, 0xc0, 0x155);
    gfx_restore_rect(0x140, 0x276, 8, 0xb7);
}

// 1b92:141b — end the war between a and b. With Israel, an aggressor gains prestige/stability
// effects; between others, the stronger side gains. Border forces go home.
void make_peace(int a, int b) {
    at_war(a, b) = 0;
    relations(a, b) = 5;
    if (a == Israel) {
        if (s98a0[b] > 0) {
            if (sa04e[Israel] < 10) ++sa04e[Israel];
            if (nation_status[b] < 9) ++nation_status[b];
        }
        if (s98a0[b] < 0) {
            if (sa04e[b] < 10) ++sa04e[b];
            if (nation_status[Israel] < 9) ++nation_status[Israel];
        }
        sa15e[Israel] = 0;
        ai::choose_rival(b);
        for (int k = 0; k < 9; ++k) {
            forces(Israel, Israel, k) += forces(Israel, b, k).get();
            forces(b, b, k) += forces(b, Israel, k).get();
            forces(Israel, b, k) = 0;
            forces(b, Israel, k) = 0;
        }
        border_brigades[Israel] += border_brigades[b].get();
        border_brigades[b] = 0;
    } else {
        if (forces(b, a, 0) < forces(a, b, 0)) {
            if (sa04e[a] < 10) ++sa04e[a];
            nation_status[a] = 0;
        }
        if (forces(a, b, 0) < forces(b, a, 0)) {
            if (sa04e[b] < 10) ++sa04e[b];
            nation_status[b] = 0;
        }
        ai::choose_rival(a);
        ai::choose_rival(b);
        for (int k = 0; k < 9; ++k) {
            forces(a, a, k) += forces(a, b, k).get();
            forces(b, b, k) += forces(b, a, k).get();
            forces(a, b, k) = 0;
            forces(b, a, k) = 0;
        }
    }
    peace_news(a, b, 1);
}

void peace_news(int a, int b, int kind) {
    uint16_t t = 0;
    if (kind == 1) t = word_at(0x75d0);
    if (kind == 2) t = word_at(0x75ea);
    if (t != 0) news::add_headline(ds.str(t), a, b);
}

}  // namespace conflict::diplomacy
