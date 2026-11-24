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
    s92f0[Egypt] = 0x433;
    s9300[Egypt] = 5000;
    sa03e[Egypt] = 0x23;
    sa04e[Egypt] = int16_t(random(3) + 1);
    sa0fe[Egypt] = int16_t(random(5) + 2);
    s98c2[Egypt] = 2;
    border(Egypt, Libya) = 1;
    border(Egypt, Israel) = 1;
    border(Egypt, Jordan) = 2;
    home_forces(Egypt, 3000, 2000, 0x50, 0x19, 100);
    nation_status[Egypt] = 0;
    sa11e[Egypt] = 0;
    sa13e[Egypt] = int16_t(random(7) - 1);
    sa14e[Egypt] = int16_t(pick_partner(Iran, [](int n) { return n == 2 || n == 6 || n == 0 || n == 4; }));
}

void setup_iran() {  // 1d93:148b
    s92f0[Iran] = 0x127a;
    s9300[Iran] = 5000;
    sa03e[Iran] = 0xd;
    sa04e[Iran] = int16_t(random(3) + 1);
    sa0fe[Iran] = int16_t(random(4) + 1);
    s98c2[Iran] = 2;
    border(Iran, Iraq) = 1;
    border(Iran, Syria) = 2;
    home_forces(Iran, 0x898, 0x5dc, 0xfa, 0x20, 100);
    nation_status[Iran] = 2;
    sa11e[Iran] = 1;
    sa13e[Iran] = int16_t(random(7) + 6);
    sa14e[Iran] = int16_t(pick_partner(Libya, [](int n) { return n == 7 || n == 1 || n == 0 || n == 4 || n == 3; }));
}

void setup_iraq() {  // 1d93:1537
    s92f0[Iraq] = 0x49c;
    s9300[Iraq] = 0x9c4;
    sa03e[Iraq] = 0xb;
    sa04e[Iraq] = int16_t(random(3) + 1);
    sa0fe[Iraq] = int16_t(random(4) + 1);
    s98c2[Iraq] = 1;
    border(Iraq, Iran) = 1;
    border(Iraq, Syria) = 1;
    border(Iraq, Jordan) = 1;
    home_forces(Iraq, 0x640, 0x578, 100, 4, 100);
    nation_status[Iraq] = 1;
    sa11e[Iraq] = 1;
    sa13e[Iraq] = int16_t(random(7) - 1);
    sa14e[Iraq] = int16_t(pick_partner(Iraq, [](int n) { return n == 7 || n == 5 || n == 0 || n == 4; }));
}

void setup_jordan() {  // 1d93:1742
    s92f0[Jordan] = 100;
    s9300[Jordan] = 300;
    sa03e[Jordan] = 0xf;
    sa04e[Jordan] = int16_t(random(3) + 1);
    sa0fe[Jordan] = 0;
    s98c2[Jordan] = 1;
    border(Jordan, Iraq) = 1;
    border(Jordan, Syria) = 1;
    border(Jordan, Israel) = 1;
    border(Jordan, Egypt) = 2;
    border(Jordan, Lebanon) = 2;
    home_forces(Jordan, 0x262, 200, 0x140, 3, 0x3c);
    nation_status[Jordan] = 0;
    sa11e[Jordan] = 0;
    sa13e[Jordan] = int16_t(random(7) - 1);
    sa14e[Jordan] = int16_t(pick_partner(Iran, [](int n) { return n == 5 || n == 3; }));
}

void setup_lebanon() {  // 1d93:17e5
    s92f0[Lebanon] = 0x134;
    s9300[Lebanon] = 1000;
    sa03e[Lebanon] = 2;
    sa04e[Lebanon] = 2;
    sa0fe[Lebanon] = 0;
    s98c2[Lebanon] = 0;
    border(Lebanon, Syria) = 1;
    border(Lebanon, Israel) = 1;
    border(Lebanon, Jordan) = 2;
    home_forces(Lebanon, 0xaa, 0, 0x28, 3, 0xc);
    nation_status[Lebanon] = 3;
    sa11e[Lebanon] = 3;
    sa13e[Lebanon] = int16_t(random(0xd) - 1);
    sa14e[Lebanon] = int16_t(pick_partner(Libya, [](int n) { return n == 0 || n == 1 || n == 2 || n == 4 || n == 3; }));
}

void setup_libya() {  // 1d93:15e5
    s92f0[Libya] = 1000;
    s9300[Libya] = 1000;
    sa03e[Libya] = 2;
    sa04e[Libya] = int16_t(random(3) + 1);
    sa0fe[Libya] = int16_t(random(4) + 2);
    s98c2[Libya] = 0;
    border(Libya, Egypt) = 1;
    border(Libya, Israel) = 2;
    home_forces(Libya, 0xdc, 0x4b0, 0, 0xc, 0x32);
    nation_status[Libya] = 2;
    sa11e[Libya] = 2;
    sa13e[Libya] = int16_t(random(5) + 6);
    sa14e[Libya] = int16_t(pick_partner(Libya, [](int n) { return n == 1 || n == 2 || n == 3 || n == 4 || n == 6; }));
}

void setup_syria() {  // 1d93:1878
    s92f0[Syria] = 400;
    s9300[Syria] = 1000;
    sa03e[Syria] = 0x19;
    sa04e[Syria] = int16_t(random(3) + 1);
    sa0fe[Syria] = int16_t(random(5) + 2);
    s98c2[Syria] = 2;
    border(Syria, Iraq) = 1;
    border(Syria, Jordan) = 1;
    border(Syria, Lebanon) = 1;
    border(Syria, Israel) = 1;
    border(Syria, Iran) = 2;
    home_forces(Syria, 2000, 2000, 300, 8, 0xb4);
    sa11e[Syria] = 0;
    nation_status[Syria] = 1;
    sa13e[Syria] = int16_t(random(7) - 1);
    sa14e[Syria] = int16_t(pick_partner(Libya, [](int n) { return n == 1 || n == 6 || n == 0; }));
}

void setup_israel() {  // 1d93:192f
    s92f0[Israel] = 0x41a;
    s9300[Israel] = 0x1194;
    sa03e[Israel] = 0x23;
    sa04e[Israel] = 4;
    sa0fe[Israel] = 6;
    s98c2[Israel] = 3;
    border(Israel, Egypt) = 1;
    border(Israel, Jordan) = 1;
    border(Israel, Syria) = 1;
    border(Israel, Lebanon) = 1;
    home_forces(Israel, 0x578, 2000, 0x32, 5, 200);
    nation_status[Israel] = 1;
    sa11e[Israel] = 2;
    sa13e[Israel] = 0xe;
    sa14e[Israel] = 7;
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
    sb26a = 0;
    menu_context = 0;
    sb266 = 3;
    sb262 = 0x96;
    sb26c = 0;
    sb276 = 0;
    sb26e = 7;
    sb264 = 0;
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
        s98a0[n] = 0;
        s98b0[n] = 0;
        sa15e[n] = 0;
        forces(n, n, 5) = 0x19;
        forces(n, n, 7) = 5;
        s9390[n] = 0;
        sa1e2[n] = 0;
        sa1d2[n] = 0;
    }
    s98b0[7] = 7;
    s98b0[8] = 0;
}

}  // namespace conflict::setup
