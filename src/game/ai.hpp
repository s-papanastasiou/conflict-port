// The neighbours' monthly turn (code segments 1000 and 1d93).
#pragma once

namespace conflict::ai {

void nation_turn(int n);                  // 1000:3138
void collapse(int n);                     // 1000:58be — government falls
void withdraw_forces(int n);              // 1000:595d — n leaves the game: forces go home/vanish
void choose_rival(int n);                  // 1000:6e8e
void adjust_relation(int a, int b, int r);  // 1000:3897
void pressure(int a, int b);              // 1000:3a23 — aid / pressure on a weaker rival
void relation_news(int a, int b);         // 1d93:0007
void instability_news(int n);             // 1d93:037b
void event_news(int kind);                // 1d93:0b2a

}  // namespace conflict::ai
