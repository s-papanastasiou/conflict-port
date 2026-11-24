#include "game/endgame.hpp"

#include <cstring>

#include "game/crt.hpp"
#include "game/globals.hpp"
#include "game/monthly.hpp"
#include "game/news.hpp"
#include "game/newspaper.hpp"
#include "game/sys.hpp"
#include "game/ui.hpp"

#pragma GCC diagnostic ignored "-Wrestrict"  // distinct strings inside one DGROUP buffer

namespace conflict::endgame {

using namespace sys;
using namespace g;

void game_over(int kind, int closing, int border_icon, int nation) {
    int r = crt::random(3);
    news::add_headline(ds.str(word_at(uint16_t(0x5144 + 2 * (r + kind * 3 - 1)))), nation, nation);
    monthly::build_digest();
    std::strcpy(ds.str(kWindowTitle), ds.str(0x5652));  // "The End Of The Game..."
    closing_newspaper(closing);
    ui::draw_icon_border(border_icon);
    std::strcpy(ds.str(kWindowText), ds.str(word_at(uint16_t(0x5362 + 2 * kind))));
    std::strcpy(ds.str(kWindowLines), ds.str(0x5669));  // "YOUR END"
    ui::text_window(0x5a, 0x82, 0x23a, 0xc3, ds.str(kWindowTitle), ds.str(kWindowText),
                    ds.str(kWindowLines));
    leadership_analysis();
    crt::longjmp_restart(10);
}

void closing_newspaper(int closing) {
    if (headline_count != 0) {
        newspaper::show();
        std::strcpy(ds.str(kWindowText), ds.str(word_at(uint16_t(0x4f00 + 2 * closing))));
        ui::menu_popup(0x1ce, 5, 0x276, 0x2a, ds.str(kWindowText), 0);
    }
    fg_colour = 0xe;
    gfx_fill_rect(0, 0x27f, 0, 0x15d);
    fg_colour = 0;
}

void leadership_analysis() {
    char* text = ds.str(kWindowText);
    char* lines = ds.str(kWindowLines);
    uint16_t bonus = 0;
    int held = 0;  // neighbours under Israeli control
    if (nation_status[Jordan] == 10 && nation_status[Syria] == 10 && nation_status[Egypt] == 10 &&
        nation_status[Lebanon] == 10)
        bonus = 100;
    if (bonus < 100 && (nation_status[Syria] == 10 || nation_status[Egypt] == 10)) bonus += 0x32;
    int score = int16_t(((bonus + nation_status[Israel] * -8) - sb264) + sa04e[Israel] * 0x10 +
                        turn_count * 4);
    // A collapsed nation's GNP slot holds its controlling power (7 = Israel).
    if (budget[Lebanon] != 7) score -= 5;
    if (budget[Jordan] != 7) score -= 10;
    if (budget[Syria] != 7) score -= 0x19;
    if (turn_count < 4) score -= 0x14;
    char digits[8];
    crt::itoa(score, digits);
    for (size_t i = 0; i < std::strlen(digits); ++i) digits[i] = char(digits[i] + '0');  // big digits
    std::strcpy(text, ds.str(0x53ba));   // "Your leadership was "
    std::strcpy(lines, ds.str(0x53cf));  // ""
    if (score < 0x1e) std::strcat(text, ds.str(0x53d0));       // a complete joke
    else if (score < 0x2d) std::strcat(text, ds.str(0x53e1));  // a total failure
    if (score > 0x2c && score < 0x41) std::strcat(text, ds.str(0x53f2));  // "quite inept."
    if (score > 0x40 && score < 0x50) std::strcat(text, ds.str(0x53ff));  // "not memorable."
    if (score > 0x4f && score < 0x96) std::strcat(text, ds.str(0x540e));  // "not impressive."
    if (score > 0x95 && score < 0xa5) std::strcat(text, ds.str(0x541e));  // "quite impressive."
    if (score > 0xa4 && score < 0x10e) std::strcat(text, ds.str(0x5430));  // "very impressive."
    if (score > 0x10d && score < 0x14a) std::strcat(text, ds.str(0x5441));  // "inspired !"
    if (score > 0x149 && score < 400) std::strcat(text, ds.str(0x544c));  // "superb !"
    if (score > 399) std::strcat(text, ds.str(0x5455));  // " amazing !!"
    if (nation_status[Syria] == 10 && budget[Syria] == 7) held = 1;
    if (nation_status[Lebanon] == 10 && budget[Lebanon] == 7) ++held;
    if (nation_status[Jordan] == 10 && budget[Jordan] == 7) ++held;
    if (nation_status[Egypt] == 10 && budget[Egypt] == 7) ++held;
    if (turn_count < 6) crt::sprintf(lines, ds.str(0x5461), {int(turn_count)});  // "|-You survived only %d months."
    if (turn_count > 5 && turn_count < 0xe) crt::sprintf(lines, ds.str(0x5480), {int(turn_count)});  // "|-You survived %d months in office."
    if (turn_count > 0xd && turn_count < 0x18) crt::sprintf(lines, ds.str(0x54a4), {turn_count % 12});  // "|-You survived one year and %d months."
    if (turn_count > 0x17) std::strcpy(lines, ds.str(0x54cb));  // "|-You hung on to power for a long time!"
    if (held == 0) std::strcat(lines, ds.str(0x5551));  // "|-There are no states"
    else {
        if (held == 1) std::strcat(lines, ds.str(0x54f3));  // "|-There is only one state"
        if (held == 2) std::strcat(lines, ds.str(0x550d));  // "|-There are two states"
        if (held == 3) std::strcat(lines, ds.str(0x5524));  // "|-There are three states"
        if (held == 4) std::strcat(lines, ds.str(0x553d));  // "|-All border states"
    }
    std::strcat(lines, ds.str(0x5567));  // " under Israeli control."
    std::strcat(lines, ds.str(0x557f));  // "You committed "
    if (sb276 < 1) std::strcat(lines, ds.str(0x5590));  // "no"
    if (sb276 > 0 && sb276 < 4) std::strcat(lines, ds.str(0x5593));  // "few"
    if (sb276 > 3 && sb276 < 10) std::strcat(lines, ds.str(0x5597));  // "numerous"
    if (sb276 > 10) std::strcat(lines, ds.str(0x55a0));  // "copious"
    std::strcat(lines, ds.str(0x55a8));  // " acts of violence."
    std::strcat(lines, ds.str(0x55bb));  // "Relations with the U.S. are "
    if (sb264 < 1) std::strcat(lines, ds.str(0x55da));  // "excellent."
    if (sb264 > 0 && sb264 < 5) std::strcat(lines, ds.str(0x55e5));  // "good."
    if (sb264 > 4 && sb264 < 0x1e) std::strcat(lines, ds.str(0x55eb));  // "poor."
    if (sb264 > 0x1d) std::strcat(lines, ds.str(0x55f1));  // "sour."
    std::strcat(lines, ds.str(0x55f7));  // "Our Prestige ended as "
    std::strcat(lines, ds.str(word_at(uint16_t(0x1098 + 2 * sa04e[Israel]))));
    std::strcat(lines, ds.str(0x5610));  // "."
    std::strcat(lines, ds.str(0x5612));  // "Leadership Style: "
    int style = 7;
    if (sb276 > 10) style = 1;
    if (sb276 > 0x14) style = 0;
    if (sb26a == 0 && sb26c == 0 && sb266 == 6 && sb276 > 8 && nation_status[Israel] > 4 &&
        sa03e[Israel] > 0x22 && sb264 > 0x28)
        style = 5;
    if (sb276 == 0 && sb264 < 10) style = 4;
    if (sb276 < 6 && sb264 < 0x1e && nation_status[Israel] < 4 && (sb26c != 0 || sb26a != 0) &&
        sa03e[Israel] < 0x24)
        style = 2;
    if (sb276 > 0 && sb276 < 8 && sb26a == 0 && sb26c == 0 && sa04e[Israel] > 4) style = 6;
    if (sb276 < 4 && sb26a != 0 && sb26c != 0 && sb264 < 0x1e) style = 3;
    std::strcat(lines, ds.str(word_at(uint16_t(0x53aa + 2 * style))));
    std::strcat(lines, ds.str(0x5627));  // ".|"
    std::strcat(text, ds.str(0x562a));  // "Analysis Score:"
    ui::text_window(0x34, 10, 0x24e, 0x78, ds.str(0x563e), text, lines);  // "LEADERSHIP ANALYSIS"
    font_small = font_extra;
    gfx_moveto(0x214, 0x21);
    gfx_text_small(digits);
    font_small = font_etech;
}

}  // namespace conflict::endgame
