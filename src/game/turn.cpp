#include "game/turn.hpp"

#include <cstdio>
#include <cstring>

#include "game/ai.hpp"
#include "game/war.hpp"
#include "game/border.hpp"
#include "game/crt.hpp"
#include "game/diplomacy.hpp"
#include "game/dgroup.hpp"
#include "game/globals.hpp"
#include "game/monthly.hpp"
#include "game/month.hpp"
#include "game/reports.hpp"
#include "game/setup.hpp"
#include "game/sys.hpp"
#include "game/ui.hpp"

namespace conflict::turn {


void briefing() { reports::briefing(); }
void diplomacy(int n) { diplomacy::diplomacy(n); }
void intelligence(int n) { diplomacy::intelligence(n); }
void new_game() { setup::new_game(); }
void nation_turn(int n) { ai::nation_turn(n); }
void end_of_month() { monthly::end_of_month(); }
void f3036() { monthly::build_digest(); }
void f9fd9() { monthly::deliver_arms(); }
void f4675() { month::player_month(); }
void strategic_action() { border::strategic_action(); }
void f5a52() { monthly::garrison_borders(); }
void f74e1() { monthly::covert_operations(); }
void f1b92_0bd1() { war::fight_wars(); }
void f1b92_0007() { war::air_strikes(); }

}  // namespace conflict::turn
