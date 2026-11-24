// Start-up sequence: graphics and fonts, intro poem, title screen (code segment 1000).
#pragma once

namespace conflict::startup {

void load_fonts();       // 1000:07fc
void intro();            // 1000:2058
void calibrate_delay();  // 1000:0739
void title_screen();     // 1000:2337

}  // namespace conflict::startup
