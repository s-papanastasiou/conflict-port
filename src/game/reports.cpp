#include "game/reports.hpp"

#include <cstring>

#pragma GCC diagnostic ignored "-Wrestrict"  // distinct strings inside one DGROUP buffer

#include "game/crt.hpp"
#include "game/globals.hpp"
#include "game/sys.hpp"
#include "game/ui.hpp"

namespace conflict::reports {

using namespace sys;
using namespace g;

// 1000:0491 — two bar charts per neighbour, built from characters of the EXTRA font:
// left, internal stability (10 - instability, drawn leftwards from x=205, '>' if collapsed);
// right, relations with Israel (1.5 segments per point, 'X' if at war).
void briefing() {
    menu_context = 0;
    font_small = font_etech;
    std::strcpy(ds.str(kWindowTitle), ds.str(0x110e));  // "Internal stability   Diplomatic relations"
    ui::draw_titled_panel(0x32, 5, 0x1ea, 0x73, ds.str(kWindowTitle));
    font_small = font_extra;
    gfx_moveto(0x53, 0x16);
    gfx_text_small(ds.str(0x113d));  // FIRM ... WEAK
    gfx_moveto(0x131, 0x16);
    gfx_text_small(ds.str(0x114d));  // POOR ... GOOD
    char bar[20];
    for (int n = 0; n < 7; ++n) {
        font_small = font_etech;
        const char* name = ds.str(nation_name[n]);
        int w = gfx_text_width(name);
        gfx_moveto((100 - w) / 2 + 0xcd, int(ega_rows(n * 12) + 34.0));
        gfx_text_small(name);
        font_small = font_extra;
        int k = 0;
        while (!(double(relations(Israel, n)) * 1.5 <= double(k))) bar[k++] = '=';
        bar[k] = '>';
        if (at_war(Israel, n) != 0) {
            k = 0;
            bar[0] = 'X';
        }
        if (nation_status[n] == 10) {
            k = 0;
            bar[0] = '<';
        }
        bar[k + 1] = '\0';
        gfx_moveto(0x131, int(ega_rows(n * 12) + 34.0));
        gfx_text_small(bar);
        for (k = 0; k < 10 - nation_status[n]; ++k) bar[k + 1] = '=';
        bar[0] = '<';
        if (nation_status[n] == 10) bar[0] = '>';
        bar[k + 1] = '\0';
        int y = int(ega_rows(n * 12) + 34.0);
        gfx_moveto(0xcd - gfx_text_width(bar), y);
        gfx_text_small(bar);
    }
    font_small = font_extra;
    get_key();
    gfx_restore_rect(0x32, 0x1ea, 8, 0xc9);
    black_out_collapsed();
    menu_context = 1;
}

// 1000:4f69 — recolour each collapsed nation's map colour to black inside its area.
void black_out_collapsed() {
    if (nation_status[Egypt] == 10) gfx_blit_colour(0, 100, 0xa5, 0x103, 10, 0);
    if (nation_status[Lebanon] == 10) gfx_blit_colour(0x32, 0x19, 200, 0x50, 10, 0);
    if (nation_status[Iraq] == 10) gfx_blit_colour(0xfa, 0, 300, 200, 10, 0);
    if (nation_status[Jordan] == 10) gfx_blit_colour(0x65, 0x46, 0xfa, 0x96, 2, 0);
    if (nation_status[Syria] == 10) gfx_blit_colour(0x65, 0, 0xfa, 0x69, 0xe, 0);
}

// 1000:a08f
void review_forces() {
    char* title = ds.str(kWindowTitle);
    char* text = ds.str(kWindowText);
    char* lines = ds.str(kWindowLines);
    char num[8], qty[8], cost[8], row[82];
    int listed = 0;
    if (nation_status[Iran] == 10 || nation_status[Iraq] == 10) black_out_collapsed();
    std::strcpy(title, ds.str(0x4786));  // "Awaiting delivery of:"
    std::strcpy(text, ds.str(0x479d));   // "Description  Quantity  Cost"
    std::strcpy(lines, ds.str(0x47ce));  // ""
    std::strcpy(row, ds.str(0x47cf));  // "|"
    for (int w = 0; w < 19; ++w) {
        if (!(turn_count > 1 && arms_order(w, turn_count - 1) != 0 && listed < 7)) continue;
        int value = arms_order(w, turn_count - 1) * weapon_cost[w];
        std::strcat(row, ds.str(weapon_name[w]));
        while (gfx_text_width(ds.str(0x47d1)) > gfx_text_width(row)) std::strcat(row, ds.str(0x47f5));  // "Description                        ", " "
        crt::itoa(arms_order(w, turn_count - 1), qty);
        std::strcat(row, qty);
        crt::itoa(value, cost);
        while (gfx_text_width(ds.str(0x47f7)) > gfx_text_width(row)) std::strcat(row, ds.str(0x4825));  // "Description                         Quantity ", " "
        std::strcat(row, ds.str(0x4827));  // "$"
        std::strcat(row, cost);
        std::strcat(row, ds.str(0x4829));  // " M|"
        std::strcat(lines, row);
        std::strcpy(row, ds.str(0x482d));  // ""
        ++listed;
    }
    if (std::strlen(lines) < 0x3d) {
        std::strcpy(title, ds.str(0x482e));  // "Arms deals:"
        int deals = 0, total = 0;
        for (int w = 0; w < 19; ++w) {
            int q = arms_order(w, turn_count);
            if (q != 0) {
                total += q * weapon_cost[w];
                ++deals;
            }
        }
        crt::itoa(total, cost);
        if (total != 0) {
            if (deals < 2) crt::sprintf(text, ds.str(0x4877), {total});  // "One arms agreement signed with an expenditure …"
            else crt::sprintf(text, ds.str(0x483a), {deals, total});  // "%d arms agreements signed with a total expendi…"
            ui::text_window(100, 10, 400, 0x29, title, text, ds.str(0x48ae));  // "|"
        }
    } else {
        ui::text_window(0x28, 10, 0x1b8, 0x5a, title, text, lines);
    }
    std::strcpy(title, ds.str(0x48b0));  // "Battlefield Forces undeployed"
    ui::draw_titled_panel(0x28, 100, 0x276, 0xc1, title);
    crt::itoa(border_brigades[Israel], num);
    std::strcpy(lines, num);
    int troops = border_brigades[Israel] * 0x14;
    std::strcat(lines, troops < 0x15 ? ds.str(0x48df) : ds.str(0x48d2));  // " brigade (" / " brigades ("
    crt::itoa(troops, num);
    std::strcat(lines, num);
    std::strcat(lines, ds.str(0x48eb));  // " thousand troops) and "
    if (troops == 0) std::strcpy(lines, ds.str(0x4904));  // "NO troops and "
    crt::itoa(army_reserves[Israel] / 10, num);
    std::strcat(lines, num);
    std::strcat(lines, ds.str(0x4913));  // " thousand reserves"
    auto icon = [](int force) { return ds.str(uint16_t(force_icon[force] * 0x80 + icons_cur)); };
    gfx_moveto(0x32, 199);
    ui::draw_icon(icon(Troops));
    gfx_moveto(0x5a, 0xce);
    gfx_text_small(lines);
    // Counts with an icon; zero shows a word ("none") from the data instead.
    struct Slot { int force, ix, iy, tx, ty; uint16_t none; };
    static const Slot kSlots[] = {
        {MainBattleTanks, 0x32, 0xfc, 0x5a, 0x103, 0x4927}, {Fighters, 0x32, 0x130, 0x5a, 0x137, 0x492c},
        {Sams, 0x96, 0xfc, 0xbe, 0x103, 0x4931},            {LightTanks, 0x96, 0x130, 0xbe, 0x137, 0x4936},
        {AttackHelicopters, 0xfa, 0xfc, 0x122, 0x103, 0x493b}};
    for (const Slot& s : kSlots) {
        int count = forces(Israel, Israel, s.force);
        crt::itoa(count, num);
        std::strcpy(lines, num);
        if (count == 0) std::strcpy(lines, ds.str(s.none));
        gfx_moveto(s.ix, s.iy);
        ui::draw_icon(icon(s.force));
        gfx_moveto(s.tx, s.ty);
        gfx_text_small(lines);
    }
    if (forces(Israel, Israel, Gunships) != 0) {
        crt::itoa(forces(Israel, Israel, Gunships), num);
        std::strcpy(lines, num);
        gfx_moveto(0xfa, 0x130);
        ui::draw_icon(icon(Gunships));
        gfx_moveto(0x122, 0x137);
        gfx_text_small(lines);
    }
    if (forces(Israel, Israel, Bombers) != 0) {
        crt::itoa(forces(Israel, Israel, Bombers), num);
        std::strcpy(lines, num);
        gfx_moveto(0x1a4, 0xea);
        ui::draw_icon(icon(Bombers));
        gfx_moveto(0x1cc, 0xf1);
        std::strcat(lines, ds.str(0x4940));  // " bombers ready"
        gfx_text_small(lines);
    }
    if (forces(Israel, Israel, EarlyWarning) != 0) {
        gfx_moveto(0x1a4, 0x10f);
        ui::draw_icon(icon(EarlyWarning));
        gfx_moveto(0x1cc, 0x118);
        std::strcpy(lines, ds.str(0x494f));  // "Surveillance active"
        gfx_text_small(lines);
    }
    if (nuclear[Israel] == 10) {
        gfx_moveto(0x1a4, 0x132);
        ui::draw_icon(ds.str(uint16_t(icons_cur + 0x100)));
        gfx_moveto(0x1cc, 0x139);
        std::strcpy(lines, ds.str(0x4965));  // "Missiles ready"
        gfx_text_small(lines);
    }
    ui::menu_popup(0x1c2, 5, 0x276, 0x28, ds.str(word_at(0x0cc8)), 0);
}

}  // namespace conflict::reports
