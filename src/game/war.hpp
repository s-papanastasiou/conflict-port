// Wars, battles and air strikes, resolved at the end of each month (segments 1b92, 1d93, 1000).
#pragma once

namespace conflict::war {

void fight_wars();                          // 1b92:0bd1 — every hostile pair fights
void air_strikes();                         // 1b92:0007 — neighbours strike their rivals
void israeli_strikes();                     // 1b92:0714 — strikes ordered at the borders
void peace_offer_check(int n);              // 1b92:12ed
void progress_dot();                        // 1000:24a1 — "Thinking..." progress
void air_battle(int a_edge, int b_edge, int a, int b);  // 1b92:1993
void helicopter_battle(int a, int b);       // 1b92:16c1
void ground_battle(int a, int b);           // 1b92:1c4d
void consider_nuclear(int a, int b);        // 1000:5ec3
void nuclear_strike(int a, int b);          // 1000:ad9b
void casualty_news(int a, int b, int kind, int amount);  // 1d93:0599
void conquest_check(int w, int l, int b, int a, int fights);  // 1000:5fad
void conquest_news(int w, int l);           // 1000:ae6c
void mobilise(int a, int b);                // 1000:5aa5
void strike_news(int kind, int result, int a, int b);    // 1d93:0931

}  // namespace conflict::war
