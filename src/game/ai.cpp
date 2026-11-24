#include "game/ai.hpp"

#include <cstdio>

#include "game/crt.hpp"
#include "game/fixes.hpp"
#include "game/globals.hpp"
#include "game/news.hpp"

namespace conflict::ai {

using namespace g;
using crt::random;

namespace {
// Stack slots that nation_turn reads before writing them. In the original they keep whatever the
// previous call (at the same stack depth) left there; we keep them between calls the same way.
int g_stale_relation = 0;  // [bp-0x2e]
int g_stale_roll = 0;      // [bp-0x2a]

void headline(uint16_t tmpl, int a, int b) { news::add_headline(ds.str(tmpl), a, b); }
void news_item(uint16_t tmpl, int a, int b) { news::add_news(ds.str(tmpl), a, b); }
int sign(int v) { return (v > 0) - (v < 0); }
}  // namespace

// 1000:3138
void nation_turn(int n) {
    int threat = 0;  // 1: a hostile (relation 1) nation exists, 2: an unfriendly (2) one
    if (nation_status[rival[n]] > 9) {
        withdraw_forces(rival[n]);
        choose_rival(n);
    }
    if (nation_status[n] > 9) {
        collapse(n);
        return;
    }
    int unrest = insurgency[n], instability = nation_status[n], control = prestige[n];
    if (random(10) < (unrest + instability) - control &&
        (at_war(Israel, n) == 0 || nation_status[n] != 9)) {
        ++nation_status[n];
        instability_news(n);
    }
    if (nation_status[n] == 10) {
        collapse(n);
        return;
    }
    treasury[n] = int16_t(int(double(float(double(defence_share[n]))) / 100.0 * double(gnp[n]) +
                             double(treasury[n])));
    for (int j = 0; j < 8; ++j) {
        if (fixes::fix(5) && j == n) continue;  // bug #5: no stale relation for itself
        if (n < j) g_stale_relation = relations(j, n);
        if (j < n) g_stale_relation = relations(n, j);
        if (g_stale_relation == 1 && nation_status[j] < 10) threat = 1;
        if (g_stale_relation == 2 && threat == 0 && nation_status[j] < 10) threat = 2;
    }
    if (at_war(Israel, n) == 0) {
        // Match Israel's forces on the common border, moving units between home and border.
        for (int k = 0; k < 9; ++k) {
            int diff = forces(Israel, n, k) - forces(n, Israel, k);
            if (relations(Israel, n) == 3 && threat != 1 && rival[n] == Israel)
                diff = forces(n, n, k) / 3 << 1;
            if ((relations(Israel, n) > 3 && diff < 0) || (diff > 0 && diff <= forces(n, n, k))) {
                forces(n, Israel, k) += int16_t(diff);
                forces(n, n, k) -= int16_t(diff);
            }
            if (diff > 0 && forces(n, n, k) < diff) {
                forces(n, Israel, k) += forces(n, n, k).get();
                forces(n, n, k) = 0;
            }
        }
        if (rival[n] == Israel && relations(Israel, n) < 3 && forces(n, Israel, 0) != 0 &&
            at_war(Israel, n) == 0) {
            front_line[n] -= 2;
            relations(Israel, n) = 1;
        }
    }
    for (int j = 0; j < 8; ++j) {
        if (fixes::fix(5) && threat == 1)  // bug #5: always use this pair's own relation
            g_stale_relation = j < n ? relations(n, j) : relations(j, n);
        if (threat != 1) {
            if (j < n) g_stale_relation = relations(n, j);
            else g_stale_relation = relations(j, n);
            if (j == rival[n] && j != n && nation_status[j] < 10) {
                if (double(forces(j, j, 0)) < double(forces(n, n, 0)) * 1.5 ||
                    double(forces(j, n, 0)) < double(forces(n, j, 0)) * 1.2000000000000002)
                    adjust_relation(n, j, g_stale_relation);
            }
        }
        if (j != Israel && j != rival[n] && n != rival[j] && nation_status[j] < 8 && border(n, j) != 0 &&
            g_stale_relation > 6 && n != j) {
            if (prestige[j] < prestige[n] || (prestige[n] == prestige[j] && gnp[j] < gnp[n]))
                pressure(n, j);
        }
    }
    switch (nuclear[n]) {
        case 0:
        case 10:
            break;
        case 1: case 2: case 3: case 4: case 5: case 6:
            if (treasury[n] > 50 && prestige[n] > 3) {
                treasury[n] -= 50;
                ++nuclear[n];
            }
            break;
        case 7: case 8: case 9:
            if (fixes::fix(5)) g_stale_roll = random(8);  // bug #5: a fresh roll, not last nation's
            if (treasury[n] > 20) {
                if (g_stale_roll < 4) ++nuclear[n];
                treasury[n] -= 20;
            }
            if (nuclear[n] == 10) event_news(4);
            break;
        default:
            // "illegal nuclar value" (bug #17: spelling)
            std::fputs(fixes::fix(17) ? "illegal nuclear value" : ds.str(0x1a03), stderr);  // "illegal nuclar value"
            std::fputc('\n', stderr);
    }
    // Spend the treasury on forces.
    if (threat == 2) {
        while (treasury[n] > 40) { ++forces(n, n, 3); treasury[n] -= 35; }
        while (treasury[n] > 5) { ++forces(n, n, 1); treasury[n] -= 1; }
    }
    if (threat == 1)
        while (treasury[n] > 1) { ++forces(n, n, 1); treasury[n] -= 1; }
    if (threat == 0) {
        g_stale_roll = random(8);
        while (forces(n, n, 2) < 3) { ++forces(n, n, 2); treasury[n] -= 25; }
        while (g_stale_roll == 2 && treasury[n] > 50) { ++forces(n, n, 3); treasury[n] -= 35; }
        while (g_stale_roll == 3 && treasury[n] > 5) { ++forces(n, n, 7); treasury[n] -= 5; }
        while (g_stale_roll == 4 && treasury[n] > 3) { ++forces(n, n, 5); treasury[n] -= 3; }
        if (g_stale_roll > 4 && treasury[n] > 120) {
            forces(n, n, 1) += 100;
            treasury[n] -= 100;
        }
    }
}

// 1000:58be
void collapse(int n) {
    uint16_t big = 0;
    if (strike_order[n] != 5) big = word_at(0x2484);
    nation_status[n] = 10;
    prestige[n] = 0;
    peace_offer[n] = 0;
    withdraw_forces(n);
    for (int i = 0; i < 8; ++i) {
        if (n < i) at_war(i, n) = 0;
        if (i < n) at_war(n, i) = 0;
    }
    if (big != 0) headline(big, n, n);
    news_item(word_at(0x2486), n, n);
}

// 1000:595d
void withdraw_forces(int n) {
    for (int e = 0; e < 8; ++e)
        for (int k = 0; k < 9; ++k) {
            if (e == Israel && k == 0) {
                border_brigades[7] += border_brigades[n].get();
                border_brigades[n] = 0;
            }
            forces(e, e, k) += forces(e, n, k).get();
            forces(e, n, k) = 0;
        }
    for (int e = 0; e < 8; ++e)
        for (int k = 0; k < 9; ++k) forces(n, e, k) = 0;
}

// 1000:6e8e
void choose_rival(int n) {
    int with_israel = relations(Israel, n);
    if (n == Egypt) {
        rival[Egypt] = Israel;
        if (with_israel > 7) {
            if (nation_status[Iran] != 10) rival[Egypt] = Iran;
            if (nation_status[Iraq] != 10) rival[Egypt] = Iraq;
        }
        if (nation_status[Libya] != 10 && relations(Libya, Egypt) < 7) rival[Egypt] = Libya;
    }
    if (n == Syria) {
        rival[Syria] = Israel;
        if (relations(Syria, Iraq) < with_israel && nation_status[Iraq] != 10) rival[Syria] = Iraq;
        if (relations(Syria, Jordan) < with_israel && nation_status[Jordan] != 10) rival[Syria] = Jordan;
        if (relations(Syria, Lebanon) < with_israel && nation_status[Lebanon] != 10) rival[Syria] = Lebanon;
    }
    if (n == Iran && nation_status[Iraq] != 10) rival[Iran] = Iraq;
    if (n == Iraq) {
        if (nation_status[Iran] != 10) rival[Iraq] = Iran;
        if (nation_status[Syria] != 10 && relations(Syria, Iraq) < 7) rival[Iraq] = Syria;
    }
    if (n == Jordan && relations(Syria, Jordan) < with_israel && nation_status[Syria] != 10)
        rival[Jordan] = Syria;
    if (nation_status[rival[n]] == 10) rival[n] = Israel;
}

// 1000:3897 — b is n's rival: n courts it and set the relation to r, eased by borders
// and relative strength.
void adjust_relation(int a, int b, int r) {
    if (insurgency[b] < 10) {
        treasury[a] -= 2;
        ++insurgency[b];
    }
    if (prestige[b] <= prestige[a] && border(a, b) == 1 && r > 1) --r;
    if (prestige[a] < prestige[b] && border(a, b) == 1 && r > 2) --r;
    if (prestige[b] <= prestige[a] && border(a, b) == 2 && r > 4) --r;
    if (prestige[a] < prestige[b] && border(a, b) == 0 && r > 5) --r;
    if (b < a && relations(a, b) != r) {
        relations(a, b) = int16_t(r);
        relation_news(a, b);
    }
    if (a < b && relations(b, a) != r) {
        relations(b, a) = int16_t(r);
        relation_news(a, b);
    }
}

// 1000:3a23
void pressure(int a, int b) {
    int r = b < a ? relations(a, b) : relations(b, a);
    if (r < 10 && treasury[a] > 100) {
        treasury[a] -= 100;
        treasury[b] += 100;
        if (r == 9 && prestige[a] < 10) ++prestige[a];
        if (r == 9) rival[b] = rival[a].get();
        if (b < a) ++relations(a, b);
        if (a < b) ++relations(b, a);
        relation_news(a, b);
    }
    if (r == 10) {
        if (border(b, rival[a]) == 1) {
            int t = int(double(treasury[a]) * 0.4);
            treasury[a] -= int16_t(t);
            treasury[b] += int16_t(t);
        }
        if (relations(rival[a], b) == 1 || relations(b, rival[a]) == 1) {
            if (forces(a, a, 0) > 500) {
                forces(a, a, 0) -= 200;
                forces(b, b, 0) += 200;
            }
            if (border(a, b) != 0)
                for (int k = 1; k < 9; ++k)
                    if (k != 2) {
                        int t = forces(a, a, k) / 3;
                        forces(a, a, k) -= int16_t(t);
                        forces(b, b, k) += int16_t(t);
                    }
            for (int j = 0; j < 8; ++j) {
                int rb = rel(j, b);
                int ra = rel(j, a);
                if ((rb < 4 && border(b, j) == 1 && rb > 1) || rb > 3) rb += sign(ra - rb);
                rel(j, b) = int16_t(rb);
            }
        }
    }
}

// 1d93:0007 — headline and news item for a changed relation between a and b.
void relation_news(int a, int b) {
    uint16_t big = 0, small = 0;
    int wars = 0;
    int roll = random(6);
    int k = roll - 1;
    int r = b < a ? relations(a, b) : relations(b, a);
    if (r == 4 && a == Israel && turn_count == 2) big = k < 4 ? word_at(0x580a) : word_at(0x580c);
    if (r == 3) {
        small = word_at(0x5c58);
        if (k < 2 || k > 5) {
            big = word_at(0x5810);
        } else {
            int i = k == 0 ? 1 : k;
            big = word_at(uint16_t(0x580a + 2 * i));
        }
        if (border(a, b) == 1) {
            big = word_at(uint16_t(0x580a + 2 * (roll + 1)));
            small = word_at(0x5c56);
        }
        if (b == Iran || b == Libya) big = word_at(0x581a);
    }
    if (r == 2) {
        big = word_at(uint16_t(0x5962 + 2 * (roll + 1)));
        if (a == Israel && forces(Israel, b, 0) != 0) big = word_at(0x5964);
        if (a == Israel && turn_count < 5) big = word_at(0x5962);
        if (a == Iran || a == Libya) big = k < 3 ? word_at(0x5972) : word_at(0x5974);
        small = word_at(0x5c5a);
        if (a == Israel && rival[b] != Israel) small = word_at(0x5c5c);
    }
    if (r == 1) {
        big = word_at(uint16_t(0x5ac6 + 2 * k));
        if (a == Israel && rival[b] != Israel) big = word_at(0x5ac6);
        if (b == Lebanon) big = word_at(0x5ad2);
        if (a == Iran || a == Libya) big = word_at(0x5ad4);
        for (int i = 0; i < 8; ++i) {
            if (b < i && at_war(i, b) != 0) ++wars;
            if (i < b && at_war(b, i) != 0) ++wars;
        }
        if (wars != 0) big = word_at(0x5ad6);
        small = word_at(0x5c5e);
    }
    if (r == 8 || r == 9) {
        k = random(3) - 1;
        small = word_at(uint16_t(0x5cec + 2 * k));
    }
    if (r == 10) {
        if (border(a, b) == 1 && k > 0 && k < 4) big = word_at(uint16_t(0x5b80 + 2 * k));
        if (rival[a] == rival[b] && border(a, b) != 0) {
            big = word_at(0x5b88);
            small = word_at(0x5d4c);
        }
        if (rival[a] == rival[b] && border(a, rival[a]) == 1 && border(b, rival[b]) == 1) {
            big = word_at(0x5b80);
            small = word_at(0x5d4e);
        }
    }
    if (big != 0) headline(big, a, b);
    if (small != 0) news_item(small, a, b);
}

// 1d93:037b — headline when a nation's instability rises.
void instability_news(int n) {
    uint16_t big = 0;
    if (n == Israel) {
        int r = random(2);
        if (nation_status[Israel] > 1) {
            if (land_for_peace == 0) big = word_at(uint16_t(0x6048 + 2 * (nation_status[Israel] * 2 + r - 5)));
            if (land_for_peace == 1) {
                big = word_at(uint16_t(0x6286 + 2 * (nation_status[Israel] - 1)));
                if (at_war(Israel, Egypt) != 0 && at_war(Israel, Egypt) < 4) big = word_at(0x6286);
            }
        }
    }
    if (faction[n] == 12 || faction[n] == 13) {
        int r = random(2);
        big = word_at(uint16_t(0x5d9c + 2 * (r - 1)));
    }
    if (n != Israel && faction[n] < 7 && nation_status[n] > 4) {
        int r = random(4);
        big = word_at(uint16_t(0x610c + 2 * (r - 1)));
    }
    if (big != 0) headline(big, n, n);
}

// 1d93:0b2a — special-event headlines about Israel (kind 1..6). DS:92a8 remembers which of
// kinds 1/2 was last reported (reset in the first month).
void event_news(int kind) {
    const Var<int16_t> last_event(0x92a8);
    uint16_t big = 0;
    int k = random(3) - 1;
    if (turn_count == 1) last_event = 0;
    if (kind == 1 && last_event == 0) {
        big = word_at(0x761c);
        last_event = 1;
    }
    if (kind == 2 && last_event != 2) {
        big = word_at(0x7642);
        last_event = 2;
    }
    if (kind == 3) {
        big = word_at(uint16_t(0x7732 + 2 * k));
        if (us_disapproval > 0x14) big = word_at(0x7738);
        if (us_disapproval > 0x32) big = word_at(0x773a);
    }
    if (kind == 4) {
        big = word_at(uint16_t(0x781c + 2 * k));
        if (nuclear[Libya] == 10) big = word_at(0x7824);
    }
    if (kind == 5) big = word_at(uint16_t(0x78be + 2 * k));
    if (kind == 6) big = word_at(0x78e6);
    if (big != 0) headline(big, Israel, Israel);
}

}  // namespace conflict::ai
