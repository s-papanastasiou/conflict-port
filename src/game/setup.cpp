#include "game/setup.hpp"

#include "game/crt.hpp"
#include "game/globals.hpp"
#include "game/news.hpp"

namespace conflict::setup {

using namespace g;
using crt::random;

namespace {

// Picks a nation index 0..7 by `random(8) - 1` until it is acceptable (the original's nested
// do/while loops: the inner loop rejects `first_reject`, the outer loop the others).
template <class Reject>
int pick_partner(int first_reject, Reject reject) {
    int n;
    do {
        do n = random(8) - 1;
        while (n == first_reject);
    } while (reject(n));
    return n;
}

void home_forces(int n, int f0, int f1, int f4, int f2, int f3) {
    // Same assignment order as the original (0, 1, 4, 2, 3).
    forces(n, n, 0) = int16_t(f0);
    forces(n, n, 1) = int16_t(f1);
    forces(n, n, 4) = int16_t(f4);
    forces(n, n, 2) = int16_t(f2);
    forces(n, n, 3) = int16_t(f3);
}

void setup_egypt() {  // 1d93:1693
    gnp[Egypt] = 0x433;
    army_reserves[Egypt] = 5000;
    defence_share[Egypt] = 0x23;
    prestige[Egypt] = int16_t(random(3) + 1);
    nuclear[Egypt] = int16_t(random(5) + 2);
    morale[Egypt] = 2;
    border(Egypt, Libya) = 1;
    border(Egypt, Israel) = 1;
    border(Egypt, Jordan) = 2;
    home_forces(Egypt, 3000, 2000, 0x50, 0x19, 100);
    nation_status[Egypt] = 0;
    insurgency[Egypt] = 0;
    faction[Egypt] = int16_t(random(7) - 1);
    rival[Egypt] = int16_t(pick_partner(Iran, [](int n) { return n == 2 || n == 6 || n == 0 || n == 4; }));
}

void setup_iran() {  // 1d93:148b
    gnp[Iran] = 0x127a;
    army_reserves[Iran] = 5000;
    defence_share[Iran] = 0xd;
    prestige[Iran] = int16_t(random(3) + 1);
    nuclear[Iran] = int16_t(random(4) + 1);
    morale[Iran] = 2;
    border(Iran, Iraq) = 1;
    border(Iran, Syria) = 2;
    home_forces(Iran, 0x898, 0x5dc, 0xfa, 0x20, 100);
    nation_status[Iran] = 2;
    insurgency[Iran] = 1;
    faction[Iran] = int16_t(random(7) + 6);
    rival[Iran] = int16_t(pick_partner(Libya, [](int n) { return n == 7 || n == 1 || n == 0 || n == 4 || n == 3; }));
}

void setup_iraq() {  // 1d93:1537
    gnp[Iraq] = 0x49c;
    army_reserves[Iraq] = 0x9c4;
    defence_share[Iraq] = 0xb;
    prestige[Iraq] = int16_t(random(3) + 1);
    nuclear[Iraq] = int16_t(random(4) + 1);
    morale[Iraq] = 1;
    border(Iraq, Iran) = 1;
    border(Iraq, Syria) = 1;
    border(Iraq, Jordan) = 1;
    home_forces(Iraq, 0x640, 0x578, 100, 4, 100);
    nation_status[Iraq] = 1;
    insurgency[Iraq] = 1;
    faction[Iraq] = int16_t(random(7) - 1);
    rival[Iraq] = int16_t(pick_partner(Iraq, [](int n) { return n == 7 || n == 5 || n == 0 || n == 4; }));
}

void setup_jordan() {  // 1d93:1742
    gnp[Jordan] = 100;
    army_reserves[Jordan] = 300;
    defence_share[Jordan] = 0xf;
    prestige[Jordan] = int16_t(random(3) + 1);
    nuclear[Jordan] = 0;
    morale[Jordan] = 1;
    border(Jordan, Iraq) = 1;
    border(Jordan, Syria) = 1;
    border(Jordan, Israel) = 1;
    border(Jordan, Egypt) = 2;
    border(Jordan, Lebanon) = 2;
    home_forces(Jordan, 0x262, 200, 0x140, 3, 0x3c);
    nation_status[Jordan] = 0;
    insurgency[Jordan] = 0;
    faction[Jordan] = int16_t(random(7) - 1);
    rival[Jordan] = int16_t(pick_partner(Iran, [](int n) { return n == 5 || n == 3; }));
}

void setup_lebanon() {  // 1d93:17e5
    gnp[Lebanon] = 0x134;
    army_reserves[Lebanon] = 1000;
    defence_share[Lebanon] = 2;
    prestige[Lebanon] = 2;
    nuclear[Lebanon] = 0;
    morale[Lebanon] = 0;
    border(Lebanon, Syria) = 1;
    border(Lebanon, Israel) = 1;
    border(Lebanon, Jordan) = 2;
    home_forces(Lebanon, 0xaa, 0, 0x28, 3, 0xc);
    nation_status[Lebanon] = 3;
    insurgency[Lebanon] = 3;
    faction[Lebanon] = int16_t(random(0xd) - 1);
    rival[Lebanon] = int16_t(pick_partner(Libya, [](int n) { return n == 0 || n == 1 || n == 2 || n == 4 || n == 3; }));
}

void setup_libya() {  // 1d93:15e5
    gnp[Libya] = 1000;
    army_reserves[Libya] = 1000;
    defence_share[Libya] = 2;
    prestige[Libya] = int16_t(random(3) + 1);
    nuclear[Libya] = int16_t(random(4) + 2);
    morale[Libya] = 0;
    border(Libya, Egypt) = 1;
    border(Libya, Israel) = 2;
    home_forces(Libya, 0xdc, 0x4b0, 0, 0xc, 0x32);
    nation_status[Libya] = 2;
    insurgency[Libya] = 2;
    faction[Libya] = int16_t(random(5) + 6);
    rival[Libya] = int16_t(pick_partner(Libya, [](int n) { return n == 1 || n == 2 || n == 3 || n == 4 || n == 6; }));
}

void setup_syria() {  // 1d93:1878
    gnp[Syria] = 400;
    army_reserves[Syria] = 1000;
    defence_share[Syria] = 0x19;
    prestige[Syria] = int16_t(random(3) + 1);
    nuclear[Syria] = int16_t(random(5) + 2);
    morale[Syria] = 2;
    border(Syria, Iraq) = 1;
    border(Syria, Jordan) = 1;
    border(Syria, Lebanon) = 1;
    border(Syria, Israel) = 1;
    border(Syria, Iran) = 2;
    home_forces(Syria, 2000, 2000, 300, 8, 0xb4);
    insurgency[Syria] = 0;
    nation_status[Syria] = 1;
    faction[Syria] = int16_t(random(7) - 1);
    rival[Syria] = int16_t(pick_partner(Libya, [](int n) { return n == 1 || n == 6 || n == 0; }));
}

void setup_israel() {  // 1d93:192f
    gnp[Israel] = 0x41a;
    army_reserves[Israel] = 0x1194;
    defence_share[Israel] = 0x23;
    prestige[Israel] = 4;
    nuclear[Israel] = 6;
    morale[Israel] = 3;
    border(Israel, Egypt) = 1;
    border(Israel, Jordan) = 1;
    border(Israel, Syria) = 1;
    border(Israel, Lebanon) = 1;
    home_forces(Israel, 0x578, 2000, 0x32, 5, 200);
    nation_status[Israel] = 1;
    insurgency[Israel] = 2;
    faction[Israel] = 0xe;
    rival[Israel] = 7;
}

}  // namespace

void new_game() {
    int prev = 0;
    turn_count = 1;
    for (int i = 0; i < 25; ++i) {
        headline_ptr[i] = 0;
        news_ptr[i] = 0;
    }
    headline_count = 0;
    news_count = 0;
    news_text_used = 0;
    report_text_used = 0;
    land_for_peace = 0;
    menu_context = 0;
    west_bank_tactics = 3;
    middle_east_calm = 0x96;
    summit_agreement = 0;
    acts_of_violence = 0;
    strike_damage = 7;
    us_disapproval = 0;
    // "* premier assassinated" / "* premier dies in bomb blast", then "New * premier ..."
    const Arr<uint16_t, 4> premier_news{0x19f6};
    int r = random(2);
    news::add_headline(ds.str(premier_news[r - 1]), Israel, Israel);
    r = random(2);
    news::add_headline(ds.str(premier_news[r + 1]), Israel, Israel);
    for (int a = 1; a < 8; ++a)
        for (int b = 0; b < a; ++b) {
            int v;
            do v = random(4) + 3;
            while (v == prev);
            relations(a, b) = int16_t(v);
            at_war(a, b) = 0;
            prev = v;
        }
    for (int a = 0; a < 8; ++a)
        for (int b = 0; b < 8; ++b)
            for (int k = 0; k < 9; ++k) forces(a, b, k) = 0;
    setup_egypt();
    setup_iran();
    setup_iraq();
    setup_jordan();
    setup_lebanon();
    setup_libya();
    setup_syria();
    setup_israel();
    for (int n = 0; n < 8; ++n) {
        front_line[n] = 0;
        border_brigades[n] = 0;
        peace_offer[n] = 0;
        forces(n, n, 5) = 0x19;
        forces(n, n, 7) = 5;
        treasury[n] = 0;
        digest_part[n] = 0;
        war_status_text[n] = 0;
    }
    border_brigades[7] = 7;
    border_brigades[8] = 0;
}

}  // namespace conflict::setup
