// Monthly bookkeeping between the player's decisions (code segments 1000 and 1d93).
#pragma once

namespace conflict::monthly {

void end_of_month();          // 1000:56c7 — events, random drift of two relations
void drift_news(int a, int b);  // 1d93:0468
void build_digest();          // 1000:3036 — per-nation digest of this month's news
void deliver_arms();          // 1000:9fd9
void garrison_borders();      // 1000:5a52
void covert_operations();     // 1000:74e1 — results of assassination / coup attempts

}  // namespace conflict::monthly
