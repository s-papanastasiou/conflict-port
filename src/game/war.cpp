#include "game/war.hpp"

#include "game/ai.hpp"
#include "game/crt.hpp"
#include "game/endgame.hpp"
#include "game/fixes.hpp"
#include "game/globals.hpp"
#include "game/news.hpp"
#include "game/sys.hpp"

namespace conflict::war {

using namespace g;
using crt::random;

namespace {
void headline(uint16_t tmpl, int a, int b) { news::add_headline(ds.str(tmpl), a, b); }
void news_item(uint16_t tmpl, int a, int b) { news::add_news(ds.str(tmpl), a, b); }
int sign(int v) { return (v > 0) - (v < 0); }
// The original's int is 16 bits: `imul` then `cwd; idiv` keeps only the low word of a product.
int wrap16(int v) { return int16_t(v); }
// Zortech's software double division (2007:0390): a zero numerator gives 0 (even 0/0); a zero
// divisor gives +-infinity after a no-op error hook. IEEE matches except for 0/0 (NaN).
double soft_div(int num, int den) {
    if (num == 0) return 0.0;
    return double(float(double(num))) / double(float(double(den)));
}
// SAM defence rating: SAMs * 100 / (troops + 1) + troops / 50, with the original's 16-bit
// overflow (docs/ORIGINAL_BUGS.md #22: from 328 SAMs upwards the rating turns negative).
int sam_defence(int sams, int troops) {
    int product = fixes::fix(22) ? sams * 100 : wrap16(sams * 100);
    return product / (troops + 1) + troops / 0x32;
}
// The original checks for scan code 0x45 (Num Lock) during long battles to abort to the menu.
void check_abort() {
    if (sys::kbd_poll_ext() == 0x45) crt::longjmp_restart(999);
}
void worsen_relation_with_israel(int n) {
    if (relations(Israel, n) > 1) --relations(Israel, n);
}
void lose_bombers() {
    if (forces(Israel, Israel, Bombers) < 3) forces(Israel, Israel, Bombers) = 0;
    else forces(Israel, Israel, Bombers) -= 3;
}
const Var<int16_t> last_strike_story(0x92a6);
}  // namespace

// 1b92:0bd1
void fight_wars() {
    int fights = 0;  // never reset per pair (docs/ORIGINAL_BUGS.md #3)
    for (int a = 1; a <= 7; ++a) {
        for (int b = 0; b < a; ++b) {
            if (!(relations(a, b) == 1 && nation_status[a] < 10 && nation_status[b] < 10)) continue;
            if (fixes::fix(3)) fights = 0;  // bug #3: count this pair's battles only
            if (at_war(a, b) != 0 || a != Israel) mobilise(a, b);
            if (sa15e[b] == 2) peace_offer_check(b);
            progress_dot();
            int troops_a = forces(a, b, Troops), tanks_a = forces(a, b, MainBattleTanks);
            int troops_b = forces(b, a, Troops), tanks_b = forces(b, a, MainBattleTanks);
            if (forces(a, b, Fighters) > 5) forces(a, b, Fighters) -= int16_t(random(5));
            int air_a = forces(a, b, Fighters);
            air_a = forces(b, a, Fighters) < air_a ? air_a - forces(b, a, Fighters) : 0;
            if (forces(b, a, Fighters) > 5) forces(b, a, Fighters) -= int16_t(random(5));
            int air_b = forces(b, a, Fighters);
            air_b = forces(a, b, Fighters) < air_b ? air_b - forces(a, b, Fighters) : 0;
            check_abort();
            if (air_a != 0 || air_b != 0) {
                ++fights;
                air_battle(air_a, air_b, a, b);
            }
            if (forces(a, b, Gunships) != 0 || forces(b, a, Gunships) != 0 ||
                forces(a, b, AttackHelicopters) != 0 || forces(b, a, AttackHelicopters) != 0) {
                ++fights;
                helicopter_battle(a, b);
            }
            if (tanks_a != 0 || tanks_b != 0 || forces(a, b, LightTanks) != 0 ||
                forces(b, a, LightTanks) != 0 || troops_a != 0 || troops_b != 0) {
                ++fights;
                ground_battle(a, b);
            }
            if (fights != 0) {
                ++at_war(a, b);
                consider_nuclear(a, b);
            }
            int lost_troops_a = troops_a - forces(a, b, Troops);
            int lost_tanks_a = tanks_a - forces(a, b, MainBattleTanks);
            int lost_troops_b = troops_b - forces(b, a, Troops);
            int lost_tanks_b = tanks_b - forces(b, a, MainBattleTanks);
            // Report whichever side lost the larger share (troops, then tanks). Written as the
            // original branches (`!(r1 > r2)`), with Zortech's software division.
            bool report_a = troops_a == 0 ||
                            !(soft_div(lost_troops_b, troops_b) > soft_div(lost_troops_a, troops_a));
            if (report_a) {
                if (troops_b != 0) casualty_news(a, b, 0, lost_troops_a);
            } else {
                casualty_news(b, a, 0, lost_troops_b);
            }
            bool report_b = tanks_b == 0 ||
                            !(soft_div(lost_tanks_a, tanks_a) > soft_div(lost_tanks_b, tanks_b));
            if (report_b) {
                if (tanks_a != 0) casualty_news(b, a, 1, lost_tanks_b);
            } else {
                casualty_news(a, b, 1, lost_tanks_a);
            }
            if (forces(a, b, Troops) < lost_troops_a * 2 && nation_status[a] < 10) ++nation_status[a];
            if (forces(b, a, Troops) < lost_troops_b * 2 && nation_status[b] < 10) ++nation_status[b];
            // Israel's front line.
            if (a == Israel && fights != 0 && at_war(Israel, b) > 1) {
                if (forces(Israel, b, MainBattleTanks) < 1 || forces(b, Israel, MainBattleTanks) < 1) {
                    if (forces(Israel, b, MainBattleTanks) == 0) {
                        --s98a0[b];
                        casualty_news(Israel, b, 2, s98a0[b]);
                    }
                    if (forces(b, Israel, MainBattleTanks) == 0) {
                        ++s98a0[b];
                        casualty_news(b, Israel, 2, s98a0[b]);
                    }
                } else {
                    int s = sign(lost_troops_b - lost_troops_a) + sign(lost_tanks_b - lost_tanks_a);
                    s98a0[b] += int16_t(sign(s));
                }
            }
            if (at_war(a, b) == 6) {
                --s98c2[a];
                --s98c2[b];
            }
            if (a == Israel && troops_a < 1 && troops_b != 0) s98a0[b] = -5;
            conquest_check(8, 8, b, a, fights);
            if (lost_troops_a > 0 && lost_troops_a <= s9300[a]) {  // reserves refill the army
                forces(a, a, Troops) += int16_t(lost_troops_a);
                s9300[a] -= int16_t(lost_troops_a);
            }
            if (lost_troops_b > 0 && lost_troops_b <= s9300[b]) {
                forces(b, b, Troops) += int16_t(lost_troops_b);
                s9300[b] -= int16_t(lost_troops_b);
            }
        }
    }
}

// 1b92:12ed — n offers Israel peace if hard pressed (sa15e[n] = 1).
void peace_offer_check(int n) {
    int pressure = s98a0[n] > 2 ? 2 : 0;
    for (int i = 0; i < 7; ++i) {
        if (border(n, i) == 1 && nation_status[i] != 10) {
            if (i < n && (relations(n, i) == 1 || relations(n, i) == 2)) ++pressure;
            if (n < i && (relations(i, n) == 1 || relations(i, n) == 2)) ++pressure;
        }
    }
    if (nation_status[n] > 2 && nation_status[n] < 8) ++pressure;
    // The original reuses its loop index (now 7) here; docs/ORIGINAL_BUGS.md #4.
    if (double(forces(n, Israel, Troops)) * 1.5 < double(forces(Israel, n, Troops))) pressure += 2;
    if (pressure > 2) sa15e[n] = 1;
}

void progress_dot() {
    sys::gfx_moveto(status_bar_x, 0x12e);
    sys::gfx_text_small(ds.str(0x18f0));
    status_bar_x += 10;
}

// 1b92:1993 — each point of air superiority costs the other side tanks, light tanks and troops;
// the other side's SAMs may shoot fighters down.
void air_battle(int a_edge, int b_edge, int a, int b) {
    int troops_a = forces(a, b, Troops), tanks_a = forces(a, b, MainBattleTanks);
    int light_a = forces(a, b, LightTanks);
    int troops_b = forces(b, a, Troops), tanks_b = forces(b, a, MainBattleTanks);
    int light_b = forces(b, a, LightTanks);
    int sams_a = forces(a, b, Sams), sams_b = forces(b, a, Sams);
    int defence_a = sam_defence(sams_a, troops_a);
    int defence_b = sam_defence(sams_b, troops_b);
    for (int i = 0; i < b_edge; ++i) {
        int hit = random(5) + 2;
        if (tanks_a <= hit && hit < light_a) light_a -= hit;
        if (hit < tanks_a) tanks_a -= hit;
        int r = random(3);
        if (r + 1 < troops_a) troops_a -= r + 1;
        if (random(100) < defence_a) --forces(b, a, Fighters);
    }
    for (int i = 0; i < a_edge; ++i) {
        int hit = random(5) + 2;
        if (tanks_b <= hit && hit < light_b) light_b -= hit;
        if (hit < tanks_b) tanks_b -= hit;
        int r = random(3);
        if (r + 1 < troops_b) troops_b -= r + 1;
        if (random(100) < defence_b) --forces(a, b, Fighters);
    }
    forces(a, b, Troops) = int16_t(troops_a);
    forces(a, b, MainBattleTanks) = int16_t(tanks_a);
    if (forces(a, b, MainBattleTanks) < 1) forces(a, b, MainBattleTanks) = 0;
    if (forces(a, b, Troops) < 1) forces(a, b, Troops) = 0;
    forces(a, b, LightTanks) = int16_t(light_a);
    forces(b, a, Troops) = int16_t(troops_b);
    forces(b, a, MainBattleTanks) = int16_t(tanks_b);
    if (forces(b, a, MainBattleTanks) < 1) forces(b, a, MainBattleTanks) = 0;
    if (forces(b, a, Troops) < 1) forces(b, a, Troops) = 0;
    forces(b, a, LightTanks) = int16_t(light_b);
}

// 1b92:16c1 — attack helicopters hit tanks; SAMs bring helicopters down. Israel's gunships on
// b's border always take part (docs/ORIGINAL_BUGS.md #19).
void helicopter_battle(int a, int b) {
    int heli_b = forces(b, a, AttackHelicopters);
    int heli_a = forces(a, b, AttackHelicopters);
    // Bug #19: the original always takes Israel's gunships on b's border.
    int gunships = fixes::fix(19) ? forces(a, b, Gunships) : forces(Israel, b, Gunships);
    int troops_a = forces(a, b, Troops);
    int tanks_a = forces(a, b, MainBattleTanks);
    int tanks_b = forces(b, a, MainBattleTanks);
    int sams_a = forces(a, b, Sams);
    int defence_b = sam_defence(forces(b, a, Sams), forces(b, a, Troops));
    int defence_a = sam_defence(sams_a, troops_a);
    if (heli_a * 4 <= tanks_b) tanks_b -= heli_a * 4;
    if (heli_b * 4 <= tanks_a) tanks_a -= heli_b * 4;
    int heli_a_left = heli_a;
    for (int i = 0; i < heli_a; ++i)
        if (double(random(100)) < double(defence_b) * 1.5) --heli_a_left;
    int heli_b_left = heli_b;
    for (int i = 0; i < heli_b; ++i)
        if (double(random(100)) < double(defence_a) * 1.5) --heli_b_left;
    int gunships_left = gunships;
    for (int i = 0; i < gunships; ++i) {
        int r = random(100);
        if (r < 0x1e && forces(b, Israel, Sams) > 10) forces(b, Israel, Sams) -= 3;
        if (r > 0x1d && r < 0x3c && heli_b_left > 3) heli_b_left -= 3;
        if (r > 0x3b && tanks_b > 6) tanks_b -= 8;
        if (double(r) < double(defence_b) * 1.5) --gunships_left;
    }
    forces(a, b, MainBattleTanks) = int16_t(tanks_a);
    if (forces(a, b, MainBattleTanks) < 1) forces(a, b, MainBattleTanks) = 0;
    forces(b, a, MainBattleTanks) = int16_t(tanks_b);
    if (forces(b, a, MainBattleTanks) < 1) forces(b, a, MainBattleTanks) = 0;
    forces(a, b, AttackHelicopters) = int16_t(heli_a_left);
    forces(b, a, AttackHelicopters) = int16_t(heli_b_left);
    if (fixes::fix(19)) forces(a, b, Gunships) = int16_t(gunships_left);
    else forces(Israel, b, Gunships) = int16_t(gunships_left);
}

// 1b92:1c4d — three rounds of armour and infantry fighting, then breakdowns.
void ground_battle(int a, int b) {
    int troops_a = forces(a, b, Troops), tanks_a = forces(a, b, MainBattleTanks);
    int light_a = forces(a, b, LightTanks);
    int troops_b = forces(b, a, Troops), tanks_b = forces(b, a, MainBattleTanks);
    int light_b = forces(b, a, LightTanks);
    for (int round = 0; round < 3; ++round) {
        int hit_by_a = tanks_a == 0 ? 0 : random(tanks_a) / 7;
        int hit_by_b = tanks_b == 0 ? 0 : random(tanks_b) / 7;
        if (round == 0) {
            if (tanks_b < hit_by_a && hit_by_a <= light_b) light_b -= hit_by_a;
            tanks_b = tanks_b < hit_by_a ? 0 : tanks_b - hit_by_a;
            if (tanks_a < hit_by_b && hit_by_b <= light_a) light_a -= hit_by_b;
            tanks_a = tanks_a < hit_by_b ? 0 : tanks_a - hit_by_b;
        } else {
            if (tanks_a == 0) troops_a -= hit_by_b >> 3;
            troops_a -= hit_by_b >> 3;
            if (tanks_b == 0) troops_b -= hit_by_a >> 3;
            troops_b -= hit_by_a >> 3;
        }
        int light_hit_a = light_a == 0 ? 0 : random(light_a) >> 2;
        int light_hit_b = light_b == 0 ? 0 : random(light_b) >> 2;
        if (light_hit_b < tanks_a) tanks_a -= light_hit_b;
        if (light_hit_a < tanks_b) tanks_b -= light_hit_a;
        if (troops_a / 100 <= light_b) light_b -= troops_a / 100;
        if (troops_b / 100 <= light_a) light_a -= troops_b / 100;
        int d = nation_status[a] - s98c2[a];
        int strength_a = d < 2 ? random(troops_a) : random(troops_a / d);
        d = nation_status[b] - s98c2[b];
        int strength_b = d < 2 ? random(troops_b) : random(troops_b / d);
        int diff = (strength_b - strength_a) / 10;
        if (diff > 0) troops_a -= diff;
        if (diff < 0) troops_b += diff;
        if (troops_b < 1) troops_b = 0;
        if (troops_a < 1) troops_a = 0;
    }
    int breakdown_b = tanks_b, breakdown_a = tanks_a;
    for (int i = 0; i < breakdown_b; ++i) {
        if (random(0x14) < 4) --tanks_b;
        check_abort();
    }
    for (int i = 0; i < breakdown_a; ++i) {
        if (random(0x14) < 4) --tanks_a;
        check_abort();
    }
    forces(a, b, Troops) = int16_t(troops_a);
    forces(a, b, MainBattleTanks) = int16_t(tanks_a < 1 ? 0 : tanks_a);
    forces(a, b, LightTanks) = int16_t(light_a);
    forces(b, a, Troops) = int16_t(troops_b);
    forces(b, a, MainBattleTanks) = int16_t(tanks_b < 1 ? 0 : tanks_b);
    forces(b, a, LightTanks) = int16_t(light_b);
}

// 1000:5ec3 — Israel's ordered nuclear strike (sa12e == 5), or a nuclear power losing a war.
void consider_nuclear(int a, int b) {
    if (sa12e[b] == 5) nuclear_strike(Israel, b);
    if (nuclear[a] == 10 && a != Israel) {
        int war = at_war(a, b);
        if ((war > 6 && nation_status[a] > 5) || (sa04e[b] <= sa04e[a] && nuclear[b] < 8 && war > 2))
            nuclear_strike(a, b);
    }
    if (nuclear[b] == 10) {
        int war = at_war(a, b);
        if ((war > 6 && nation_status[b] > 5) || (a == Israel && nuclear[Israel] < 10) ||
            (sa04e[a] <= sa04e[b] && nuclear[a] < 8 && war > 2))
            nuclear_strike(b, a);
    }
}

// 1000:ad9b — wipes out the target's troops on the border; a nuclear exchange, or one in a
// very tense Middle East, ends the game.
void nuclear_strike(int a, int b) {
    int r = random(5);
    forces(b, a, Troops) = 0;
    if (a == Israel) sb264 += 100;
    headline(word_at(uint16_t(0x4d76 + 2 * (r - 1))), a, b);
    headline(word_at(uint16_t(0x4d76 + 2 * r)), a, b);
    news_item(word_at(0x4d76), a, b);
    if ((nuclear[a] == 10 && nuclear[b] == 10) || sb262 < 0x87) {
        if (a == Israel || b == Israel) endgame::game_over(3, 2, 2, 0);
        else endgame::game_over(4, 2, 2, 0);
    }
}

// 1d93:0599 — kind 0: troop losses, 1: tank losses, 2: front line moved. Also sets the war
// status text shown at the border (sa1d2) for Israel's wars.
void casualty_news(int a, int b, int kind, int amount) {
    uint16_t big = 0, small = 0;
    int war = b < a ? at_war(a, b) : at_war(b, a);
    if (kind == 0) {
        int roll = random(4);
        int k = roll - 1;
        int troops = forces(a, b, Troops);
        if (amount * 2 - troops != 0 && troops <= amount * 2) {
            big = word_at(uint16_t(0x662c + 2 * k));
            small = word_at(0x6dea);
        }
        if (war == 1) {
            big = word_at(uint16_t(0x6704 + 2 * (roll + 1)));
            small = word_at(0x6de2);
            if (a == Lebanon) big = word_at(0x6704);
            int wars = 0;
            for (int i = 0; i < 8; ++i) {
                if (at_war(i, a) != 0) ++wars;
                if (at_war(a, i) != 0) ++wars;
            }
            if (wars > 1) {
                big = word_at(0x6706);
                small = word_at(0x6de0);
            }
        }
        if (war > 1 && amount > 0x32) small = word_at(0x6de4);
        if (war > 1 && amount > 400) {
            big = word_at(uint16_t(0x682a + 2 * k));
            small = word_at(0x6de6);
        }
        if (war > 1 && amount > 0x2ee) {
            big = word_at(uint16_t(0x682a + 2 * (roll + 3)));
            small = word_at(0x6de8);
        }
        if (war == 6) big = word_at(uint16_t(0x68c6 + 2 * k));
    }
    if (kind == 2) big = word_at(uint16_t(0x6e84 + 2 * (random(5) - 1)));
    if (kind == 1) {
        int r = random(3);
        if (war == 2) {
            big = word_at(uint16_t(0x6ad8 + 2 * (r - 1)));
        } else {
            if (amount > 300) {
                big = word_at(uint16_t(0x6ad8 + 2 * (r + 2)));
                if (forces(a, b, Fighters) + 0x14 < forces(b, a, Fighters))
                    big = word_at(uint16_t(0x6ad8 + 2 * (r + 0xb)));
            }
            if (amount > 500) big = word_at(uint16_t(0x6ad8 + 2 * (r + 5)));
            if (amount > 700) big = word_at(uint16_t(0x6ad8 + 2 * (r + 8)));
        }
    }
    if ((a == Israel || b == Israel) && kind == 0) {
        if (a == Israel) {
            sa1d2[b] = word_at(0x6c3e);
            if (amount > 0x32) sa1d2[b] = word_at(0x6c40);
        }
        if (a == Israel && amount > 0x96) sa1d2[b] = word_at(0x6c42);
        if (a == Israel && amount > 0xfa) sa1d2[b] = word_at(0x6c44);
        if (a == Israel && amount > 0x1c2) sa1d2[b] = word_at(0x6c46);
        if (a == Israel && amount > 600) sa1d2[b] = word_at(0x6c48);
        if (b == Israel) {
            sa1d2[a] = word_at(0x6c4a);
            if (amount > 100) sa1d2[a] = word_at(0x6c4c);
        }
        if (b == Israel && amount > 300) sa1d2[a] = word_at(0x6c4e);
        if (b == Israel && amount > 500) sa1d2[a] = word_at(0x6c50);
    }
    if (big != 0) headline(big, a, b);
    if (small != 0) news_item(small, a, b);
}

// 1000:5fad — called as (8, 8, b, a, fights): decides whether a war has been won.
void conquest_check(int w, int l, int b, int a, int fights) {
    if ((forces(b, a, Troops) < 1 || nation_status[b] == 10) && fights != 0) {
        w = a;
        l = b;
    }
    if ((forces(a, b, Troops) < 1 || nation_status[a] == 10) && fights != 0) {
        w = b;
        l = a;
    }
    if (a == Israel && s98a0[b] > 4 && fights != 0) {
        w = Israel;
        l = b;
    }
    if (a == Israel && s98a0[b] < -4 && fights != 0) {
        w = b;
        l = Israel;
    }
    if (w == 8) return;
    if (l == Israel) {
        conquest_news(b, Israel);
        endgame::game_over(2, 0, 4, b);  // Israel overrun
    }
    nation_status[w] = 0;
    if (sa04e[w] < 10) ++sa04e[w];
    budget[l] = int16_t(w);  // a fallen nation's GNP slot records its conqueror
    conquest_news(w, l);
    ai::collapse(l);
    for (int k = 1; k < 9; ++k) {
        if (k != Fighters) forces(w, w, k) += int16_t(forces(l, w, k) + forces(w, l, k));
        forces(l, w, k) = 0;
        forces(w, l, k) = 0;
    }
    ai::choose_rival(w);
}

// 1000:ae6c
void conquest_news(int w, int l) {
    int wars = 0;
    for (int i = 0; i < 8; ++i) {
        if (i < l && at_war(l, i) != 0) ++wars;
        if (l < i && at_war(i, l) != 0) ++wars;
    }
    for (int i = 0; i < 8; ++i)
        if (i != w && nation_status[i] < 10 &&
            ((l < i && relations(i, l) < 3) || (i < l && relations(l, i) < 3)))
            news_item(word_at(0x4e74), i, l);
    uint16_t big = word_at(0x4e6a);
    if (wars > 1) big = word_at(0x4e68);
    if (l == Lebanon) big = word_at(0x4e72);
    if (sa12e[l] == 5) big = word_at(0x4e70);
    headline(big, w, l);
    news_item(word_at(0x4e6c), w, l);
    news_item(word_at(0x4e6e), w, l);
}

// 1000:5aa5 — both sides move their home forces to the front. The share computed for Egypt and
// Syria never limits the move (docs/ORIGINAL_BUGS.md #18).
void mobilise(int a, int b) {
    int share = 10;
    if (a == Israel) {
        int need = border_brigades[b] * 200 - forces(Israel, b, Troops);
        if (need <= forces(Israel, Israel, Troops)) {
            forces(Israel, b, Troops) += int16_t(need);
            forces(Israel, Israel, Troops) -= int16_t(need);
        }
        if (forces(Israel, Israel, Troops) < need) {
            forces(Israel, b, Troops) += forces(Israel, Israel, Troops).get();
            forces(Israel, Israel, Troops) = 0;
        }
    }
    for (int k = 0; k < 9; ++k) {
        if (k == Bombers) continue;
        if (a != Israel && a != Syria && nation_status[a] < 8) {
            forces(a, b, k) += forces(a, a, k).get();
            forces(a, a, k) = 0;
        }
        if (b != Egypt && b != Syria && nation_status[b] < 8) {
            forces(b, a, k) += forces(b, b, k).get();
            forces(b, b, k) = 0;
        }
    }
    if (b == Egypt && nation_status[Egypt] < 8) {
        share = (a == Israel && relations(Libya, Egypt) > 3 && nation_status[Libya] != 10) ? 10 : 8;
        if (a == Libya) share = 5;
    }
    if ((a == Syria || b == Syria) && nation_status[Syria] < 8) {
        int other = a != Syria ? a : b;
        if (other == Israel && relations(Syria, Jordan) > 3 && relations(Syria, Iraq) > 3) share = 9;
        if (other == Israel && relations(Syria, Iraq) < 4) share = 6;
        if (other == Israel && relations(Syria, Iraq) < 4) share = 8;
        if (other == Jordan) share = 3;
        if (other == Lebanon) share = 2;
        if (other == Iraq && relations(Israel, Syria) > 3) share = 9;
        if (other == Iraq && relations(Israel, Syria) < 4) share = 7;
    }
    for (int k = 0; k < 9; ++k) {
        if (k == Bombers) continue;
        if (a == Syria) {
            int home = forces(Syria, Syria, k);
            int move = (share * home) / 10;
            if (move < home && !fixes::fix(18)) {  // bug #18: the original moves everything
                forces(Syria, b, k) += forces(Syria, Syria, k).get();
                forces(Syria, Syria, k) = 0;
            } else {
                forces(Syria, b, k) += int16_t(move);
                forces(Syria, Syria, k) -= int16_t(move);
            }
        }
        if (b == Egypt || b == Syria) {
            int home = forces(b, b, k);
            int move = (share * home) / 10;
            if (move < home && !fixes::fix(18)) {  // bug #18
                forces(b, a, k) += forces(b, b, k).get();
                forces(b, b, k) = 0;
            } else {
                forces(b, a, k) += int16_t(move);
                forces(b, b, k) -= int16_t(move);
            }
        }
    }
}

// 1b92:0007 — in order of prestige, each neighbour may strike its rival from the air
// (roll 0-2: military targets, 3: industry, 4: terror bombing). Israel's orders resolve in turn.
void air_strikes() {
    int order[9];
    int count = 0;
    sb26e = 7;  // no damage to report
    for (int p = 10; p > 0; --p)
        for (int n = 0; n < 8; ++n)
            if (sa04e[n] == p) order[count++] = n;
    for (int i = 0; i < 8 && i < count; ++i) {
        int n = order[i];
        if (n == Israel) israeli_strikes();
        if (n == Israel || n < 0 || n > 7 || nation_status[n] == 10) continue;
        int roll = random(8);
        int target = rival[n];
        int rel = n < target ? relations(target, n) : relations(n, target);
        if (forces(n, n, Bombers) > 2 && (rel == 3 || rel == 2) && nation_status[n] != 10 &&
            nation_status[target] != 10 && border(n, target) != 0 && roll < 3) {
            int r = random(10);
            if (forces(Israel, Israel, EarlyWarning) != 0 && target == Israel) r -= 2;
            if (forces(target, target, Fighters) > 0 && r < 2) {
                forces(n, n, Bombers) -= 3;
                strike_news(1, 1, n, target);
            }
            if (r > 1 && r < 5) strike_news(1, 2, n, target);
            if (r > 4 && r < 9) {
                if ((r == 5 || r == 6) && forces(target, target, MainBattleTanks) > 6)
                    forces(target, target, MainBattleTanks) -= 6;
                if (r == 7 && forces(target, target, Fighters) > 2) forces(target, target, Fighters) -= 2;
                if (r == 8 && forces(target, target, Bombers) > 1) forces(target, target, Bombers) -= 1;
                strike_news(1, 3, n, target);
                if (target == Israel) sb26e = int16_t(n);
            }
            if (r > 8 && r < 11) {
                if (r == 9 && forces(target, target, MainBattleTanks) > 0)
                    forces(target, target, MainBattleTanks) = int16_t(forces(target, target, MainBattleTanks) / 2);
                // Bug #21: in the original r == 9 also loses 3 bombers.
                if (r == 10 && forces(target, target, Bombers) < 6) forces(target, target, Bombers) = 0;
                else if (!fixes::fix(21) || r == 10) forces(target, target, Bombers) -= 3;
                if (sa04e[n] < 0xb) ++sa04e[n];
                --rel;
                strike_news(1, 4, n, target);
                if (target == Israel) sb26e = int16_t(n + 100);
            }
        }
        if (border(n, target) != 0 && roll == 3 && forces(n, n, Bombers) > 2 && nation_status[n] != 10 &&
            nation_status[target] != 10 && (rel == 2 || rel == 3)) {
            int r = random(10);
            if (forces(Israel, Israel, EarlyWarning) != 0 && target == Israel) r -= 1;
            if (r < 4) {
                forces(n, n, Bombers) -= 3;
                strike_news(2, 1, n, target);
            } else {
                budget[target] -= int16_t(r * 0x32);
                if (r > 7 && (target == Iran || target == Iraq)) budget[target] -= 0x96;
                if ((target == Syria || target == Lebanon || target == Jordan) && budget[target] < 0x32)
                    budget[target] = 0x32;
                if ((target == Israel || target == Egypt || target == Iran || target == Iraq) &&
                    budget[target] < 500)
                    budget[target] = 500;
                --rel;
                strike_news(2, r < 8 ? 3 : 4, n, target);
                if (target == Israel) sb26e = int16_t(n + 200);
            }
        }
        if (border(n, target) != 0 && roll == 4 && forces(n, n, Bombers) > 2 && nation_status[n] != 10 &&
            nation_status[target] != 10 && (rel == 2 || rel == 3) && sa04e[n] < 4) {
            int r = random(10);
            if (forces(Israel, Israel, EarlyWarning) != 0 && target == Israel) r -= 2;
            if (r > 6 && nation_status[target] < 9) ++nation_status[target];
            if (r > 0 && r < 3) strike_news(3, 1, n, target);
            if (r > 2 && r < 7) strike_news(3, 2, n, target);
            if (r > 6 && r < 9) strike_news(3, 3, n, target);
            if (r == 10) strike_news(3, 4, n, target);
            if (r > 6) --rel;
            if (target == Israel && r > 6) sb26e = int16_t(n + 300);
        }
        if (rel < 1) rel = 1;
        if (n < target) relations(target, n) = int16_t(rel);
        else relations(n, target) = int16_t(rel);
    }
}

// 1b92:0714 — Israel's strike orders (sa12e: 1 air bases, 2 military, 3 civilian, 4 nuclear
// facilities); then the orders are cleared.
void israeli_strikes() {
    for (int n = 0; n < 7; ++n) {
        int r = random(10);
        if (sa12e[n] == 1 && forces(Israel, Israel, Bombers) != 0) {
            if (forces(Israel, Israel, Bombers) < 3) r -= 5;
            if (r < 2) {
                lose_bombers();
                strike_news(2, 1, Israel, n);
            }
            if (r > 1 && r < 11) {
                sb264 += 10;
                budget[n] -= int16_t(r);
                if ((n == Egypt || n == Lebanon || n == Jordan) && budget[n] < 0x1e) budget[n] = 0x1e;
                if (n == Syria && budget[Syria] < 0x32) budget[Syria] = 0x32;
                strike_news(2, r < 6 ? 3 : 4, Israel, n);
                worsen_relation_with_israel(n);
            }
        }
        if (sa12e[n] == 2 && forces(Israel, Israel, Bombers) != 0) {
            if (forces(Israel, Israel, Bombers) < 3) r -= 5;
            if (r < 3) {
                lose_bombers();
                strike_news(1, 1, Israel, n);
            }
            if (r > 2 && r < 6) {
                sb264 += 10;
                if (r == 4 && forces(n, n, Fighters) > 2) forces(n, n, Fighters) -= 2;
                if (r == 5 && forces(n, n, Bombers) > 1) forces(n, n, Bombers) -= 1;
                worsen_relation_with_israel(n);
                strike_news(1, 3, Israel, n);
            }
            if (r > 5 && r < 11) {
                sb264 += 0xc;
                if (r > 5 && r < 10 && forces(n, n, MainBattleTanks) > 0)
                    forces(n, n, MainBattleTanks) = int16_t(forces(n, n, MainBattleTanks) / 2);
                if (r == 10 && forces(n, n, Bombers) < 6) forces(n, n, Bombers) = 0;
                else if (!fixes::fix(21) || r == 10) forces(n, n, Bombers) -= 3;  // bug #21: not for 6..9
                if (sa04e[Israel] < 0xb) ++sa04e[Israel];
                worsen_relation_with_israel(n);
                strike_news(1, 4, Israel, n);
            }
        }
        if (sa12e[n] == 3 && forces(Israel, Israel, Bombers) != 0) {
            s9300[n] -= int16_t(r);
            if (nation_status[n] < 9) ++nation_status[n];
            if (r < 3) {
                lose_bombers();
                strike_news(3, 1, Israel, n);
            }
            if (r > 2) {
                worsen_relation_with_israel(n);
                sb264 += 0x1e;
                strike_news(3, r < 6 ? 3 : 4, Israel, n);
            }
        }
        if (sa12e[n] == 4 && forces(Israel, Israel, Bombers) != 0) {
            if (r < 3) {
                lose_bombers();
                strike_news(4, 1, Israel, n);
            } else {
                if (nuclear[n] < 10) nuclear[n] -= 3;
                if (fixes::fix(20) && nuclear[n] < 0) nuclear[n] = 0;  // bug #20: no negative levels
                if (sa04e[Israel] < 10) ++sa04e[Israel];
                strike_news(4, 4, Israel, n);
                worsen_relation_with_israel(n);
            }
        }
    }
    for (int n = 0; n < 7; ++n) sa12e[n] = 0;
}

// 1d93:0931 — kind: 1 military, 2 industrial/air base, 3 civilian, 4 nuclear facility;
// result: 1 failed (planes lost), 2 no effect, 3 damage, 4 heavy damage.
void strike_news(int kind, int result, int a, int b) {
    uint16_t big = 0, small = 0;
    int k;
    do k = random(6) - 1;
    while (k == last_strike_story);
    if (result == 1) big = word_at(uint16_t(0x6f6c + 2 * k));
    if (result == 2) big = word_at(uint16_t(0x706e + 2 * k));
    if (result == 3) {
        if (kind == 3) big = word_at(uint16_t(0x7150 + 2 * k));
        if (kind == 1) big = word_at(0x73f6);
        if (kind == 2) big = word_at(uint16_t(0x7336 + 2 * k));
        if (kind == 3) small = word_at(0x74b2);
        if (kind == 1 || kind == 2) small = word_at(0x74b4);
    }
    last_strike_story = int16_t(k);
    if (result == 4) {
        if (kind == 3) big = word_at(uint16_t(0x7240 + 2 * k));
        if (kind == 1) big = word_at(0x741c);
        if (kind == 4) big = word_at(0x74e0);
        if (kind == 2) {
            big = word_at(0x73cc);
            if (b == Lebanon) big = word_at(0x73ce);
            if (b == Iran) big = word_at(uint16_t(0x73cc + 2 * (random(2) + 1)));
        }
        if (kind == 3) small = word_at(0x74b2);
        if (kind == 1 || kind == 2) small = word_at(0x74b4);
        if (kind == 4) small = word_at(0x74b6);
    }
    if (relations(b, a) == 1 || relations(a, b) == 1) big = word_at(uint16_t(0x75aa + 2 * k));
    if (big != 0) headline(big, a, b);
    if (small != 0) news_item(small, a, b);
}

}  // namespace conflict::war
