// Diplomatic and intelligence screens for one nation (code segments 1000, 1b92, 1d93).
#pragma once

namespace conflict::diplomacy {

void diplomacy(int n);                // 1000:61b0
void intelligence(int n);             // 1000:6fa6
void make_peace(int a, int b);        // 1b92:141b
void peace_news(int a, int b, int kind);  // 1d93:0ad0 (1: peace made, 2: offer ignored)

}  // namespace conflict::diplomacy
