// The newspaper that opens each month (code segment 1000).
#pragma once

namespace conflict::newspaper {

void show();                     // 1000:14bc — up to five recent headlines as overlapping papers
void briefly_box(int x, int top);  // 1000:1f6f
void filler_box(int x, int top);   // 1000:1d09 — random filler story or advert

}  // namespace conflict::newspaper
