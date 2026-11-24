// Foreign Office: morning advice and the per-nation affairs screens (code segment 1000).
#pragma once

namespace conflict::affairs {

void foreign_office_advice();   // 1000:502c — sets the advice text (kWindowText)
void nation_affairs(int n);     // 1000:549b — map, official news, Diplomatic/Intelligence menu

// Map overlays per region; each remembers the month it was last shown (DS:918e..919e).
void overlay_iraq_iran();       // 1000:3d5e
void overlay_egypt_libya();     // 1000:3e40
void overlay_syria();           // 1000:3f03
void overlay_israel();          // 1000:3fc9
void mark_collapsed(int n, int x, int y);                   // 1000:42a5
void mark_war(int a, int b, int last_month, int x, int y);  // 1000:42f5
void blink_war(int x, int y);                               // 1000:4384
void show_nuclear(int n, int last_level, int last_month, int x0, int y0, int x1, int y1, int x,
                  int y);                                   // 1000:4126
void blink_nuclear(int x, int y, int n);                    // 1000:44d9

}  // namespace conflict::affairs
