// The monthly turn engine. Not ported yet: these entry points are named by their original
// addresses and currently show a notice.
#pragma once

namespace conflict::turn {

void briefing();              // 1000:0491 (Shift-B report)
void diplomacy(int n);        // 1000:61b0
void intelligence(int n);     // 1000:6fa6
void strategic_action();      // 1000:7e9c
void new_game();              // 1000:2e39
void nation_turn(int nation); // 1000:3138
void end_of_month();          // 1000:56c7
void f3036();                 // 1000:3036
void f9fd9();                 // 1000:9fd9
void f4675();                 // 1000:4675
void f5a52();                 // 1000:5a52
void f74e1();                 // 1000:74e1
void f1b92_0bd1();            // 1b92:0bd1
void f1b92_0007();            // 1b92:0007

}  // namespace conflict::turn
