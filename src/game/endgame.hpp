// End of the game (code segment 1000).
#pragma once

namespace conflict::endgame {

// 1000:b5b5 — epitaph headline, closing newspaper, "The End Of The Game...", analysis, then
// restart into the "The End" menu. kind 0: removed from office, 1: victory.
[[noreturn]] void game_over(int kind, int closing, int border_icon, int nation);
void closing_newspaper(int closing);  // 1000:af8a
void leadership_analysis();           // 1000:aff1

}  // namespace conflict::endgame
