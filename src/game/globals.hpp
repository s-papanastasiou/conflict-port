// Named views of CONFLICT's globals in DGROUP. Offsets are DS offsets in the original; names
// marked "?" are provisional until the code using them has been ported.
#pragma once

#include "game/dgroup.hpp"

namespace conflict::g {

// forces() type slots (from the weapon tables at DS:40a8 and the unit names).
enum Force : int {
    Troops, MainBattleTanks, Bombers, Fighters, LightTanks, Sams, Gunships, AttackHelicopters,
    EarlyWarning, BorderFlag
};

// Nation indices used by every per-nation array.
enum Nation : int { Egypt, Iran, Iraq, Jordan, Lebanon, Libya, Syria, Israel, kNations };

// ---- drawing state (read by the EGA driver) ----
inline constexpr Var<uint16_t> fg_colour{0xb2a2};
inline constexpr Var<uint16_t> bg_colour{0xb2a0};
inline constexpr Var<uint16_t> font_small{0xb290};   // current small font (near pointer)
inline constexpr Var<uint16_t> font_large{0xb292};   // current large font, biased by -0x900
inline constexpr Var<uint16_t> font_etech{0xb294};
inline constexpr Var<uint16_t> font_extra{0xb296};
inline constexpr Var<uint16_t> font_xini{0xb298};
inline constexpr Var<uint16_t> font_xguard{0xb29a};
inline constexpr Var<uint16_t> icons{0xb29c};
inline constexpr Var<uint16_t> icons_cur{0xb29e};
inline constexpr Var<float> delay_calibration{0xb27a};

// ---- menus ----
inline constexpr Var<int16_t> menu_line{0x87b0};
inline constexpr Var<int16_t> menu_item_count{0x87c8};
inline constexpr Arr<int16_t, 12> menu_item_line{0x87b4};  // 1-based: [1..count]
inline constexpr Var<int16_t> menu_context{0xb270};        // 1: Shift-B allowed, 5: Shift-S allowed
inline constexpr Arr<int16_t, 5> menu_context_pos{0x1200};  // remembered item per context (1-based)
inline constexpr Arr<int16_t, 8> menu_tick_dy{0x11f0};      // tick-mark shape
inline constexpr Arr<uint16_t, 64> menu_texts{0x0cec};       // pointers to '?'-separated menus
// String tables (near pointers), indexed by nation.
inline constexpr Arr<uint16_t, 8> nation_adjective{0x0d80};  // "Egyptian"
inline constexpr Arr<uint16_t, 8> nation_name{0x0d30};       // "Egypt"
inline constexpr Arr<uint16_t, 8> nation_capital{0x10ec};    // "Cairo"
inline constexpr Var<int16_t> status_bar_x{0xb2c4};  // ?

// ---- text buffers ----
inline constexpr uint16_t kWindowTitle = 0xa1f2;   // char[200]
inline constexpr uint16_t kWindowText = 0xa2ba;    // char[250]
inline constexpr uint16_t kWindowLines = 0xa3b4;   // char[250]
inline constexpr uint16_t kNewsText = 0xa4ae;      // char[2500], saved, cleared every turn
inline constexpr uint16_t kReportText = 0xae72;    // char[1000], saved
inline constexpr uint16_t kSqueezeScratch = 0x874c;  // 100 bytes cleared by squeeze_spaces

// ---- game state: the 42 saved variables, in file order (see save.cpp) ----
inline constexpr Arr<int16_t, 8> s92f0{0x92f0};   // ?
inline constexpr Arr<int16_t, 8> s9300{0x9300};   // army reserves (x100 troops)
inline constexpr Arr<int16_t, 64> s9310{0x9310};  // at war [a][b] (briefing shows X)
inline constexpr Arr<int16_t, 8> s9390{0x9390};   // ?
inline constexpr Arr<int16_t, 640> s93a0{0x93a0}; // forces [owner][location][type 0..8], [..][9] border
inline constexpr Arr<int16_t, 8> s98a0{0x98a0};   // ?
inline constexpr Arr<int16_t, 9> s98b0{0x98b0};   // ?
inline constexpr Arr<int16_t, 8> s98c2{0x98c2};   // ?
inline constexpr Arr<int16_t, 950> s98d2{0x98d2}; // ?
inline constexpr Arr<int16_t, 8> sa03e{0xa03e};   // ?
inline constexpr Arr<int16_t, 8> sa04e{0xa04e};   // ?
inline constexpr Arr<int16_t, 64> sa05e{0xa05e};  // relations [a][b], a > b (briefing: POOR..GOOD)
inline constexpr Arr<int16_t, 8> sa0de{0xa0de};   // ?
inline constexpr Arr<int16_t, 8> sa0ee{0xa0ee};   // ?
inline constexpr Arr<int16_t, 8> sa0fe{0xa0fe};   // ?
inline constexpr Arr<int16_t, 8> nation_status{0xa10e};  // instability 0 (firm) .. 10 (collapsed)
inline constexpr Arr<int16_t, 8> sa11e{0xa11e};   // ?
inline constexpr Arr<int16_t, 8> sa12e{0xa12e};   // ?
inline constexpr Arr<int16_t, 8> sa13e{0xa13e};   // ?
inline constexpr Arr<int16_t, 8> sa14e{0xa14e};   // ?
inline constexpr Arr<int16_t, 8> sa15e{0xa15e};   // ?
inline constexpr Arr<uint16_t, 25> headline_ptr{0xa16e};  // near pointers into kReportText (6c39)
inline constexpr Arr<uint16_t, 25> news_ptr{0xa1a0};      // near pointers into kNewsText (6cb7)
inline constexpr Arr<int16_t, 8> sa1d2{0xa1d2};   // ?
inline constexpr Arr<uint16_t, 8> sa1e2{0xa1e2};  // near pointers into string tables ?
inline constexpr Var<int16_t> news_count{0xb25a};
inline constexpr Var<int16_t> headline_count{0xb25c};
inline constexpr Var<int16_t> report_text_used{0xb25e};
inline constexpr Var<int16_t> news_text_used{0xb260};
inline constexpr Var<int16_t> sb262{0xb262};
inline constexpr Var<int16_t> sb264{0xb264};
inline constexpr Var<int16_t> sb266{0xb266};
inline constexpr Var<int16_t> turn_count{0xb268};
inline constexpr Var<int16_t> sb26a{0xb26a};
inline constexpr Var<int16_t> sb26c{0xb26c};
inline constexpr Var<int16_t> sb26e{0xb26e};
inline constexpr Var<int16_t> sb270_menu_context{0xb270};  // same as menu_context; saved
inline constexpr Var<int16_t> year{0xb272};  // ?
inline constexpr Var<int16_t> sb274{0xb274};
inline constexpr Var<int16_t> sb276{0xb276};

inline constexpr Var<int16_t> loaded_game_flag{0xb278};

// ---- accessors for the 2-D / 3-D state arrays ----
inline Var<int16_t> forces(int owner, int location, int type) {
    return s93a0[(owner * 8 + location) * 10 + type];
}
inline Var<int16_t> border(int a, int b) { return forces(a, b, 9); }
inline Var<int16_t> relations(int a, int b) { return sa05e[a * 8 + b]; }
inline Var<int16_t> at_war(int a, int b) { return s9310[a * 8 + b]; }
// Relation between two nations, stored once at [larger][smaller]; rel(a, a) is the diagonal.
inline Var<int16_t> rel(int a, int b) { return a >= b ? relations(a, b) : relations(b, a); }
// Each nation's main rival / strike target (Israel by default; Israel's own entry is 7).
inline constexpr Arr<int16_t, 8> rival = sa14e;
inline constexpr Arr<int16_t, 8> nuclear = sa0fe;  // nuclear programme 0..10
inline constexpr Arr<int16_t, 8> treasury = s9390;
inline constexpr Arr<int16_t, 8> budget = s92f0;
// Arms on order: [weapon 0..18][month 0..49] units arriving that month (s98d2).
inline Var<int16_t> arms_order(int weapon, int month) { return s98d2[weapon * 50 + month]; }
inline constexpr Arr<int16_t, 9> border_brigades = s98b0;  // Israeli brigades per border; [7] pool?
inline constexpr Arr<int16_t, 19> weapon_force_type{0x40a8};  // weapon -> forces() type slot
// Not saved: this month's covert action per nation (1 assassinate, 2 coup) and attempt count.
inline constexpr Arr<int16_t, 8> covert_action{0xb2d6};
inline constexpr Arr<int16_t, 8> covert_attempts{0x91a6};
inline constexpr uint16_t kDigestText = 0x87ca;  // char[2500], per-nation news digest (not saved)
inline constexpr Arr<int16_t, 9> force_icon{0x3146};  // icon index per Force slot
// Arms market tables, indexed by weapon 0..18.
inline constexpr Arr<uint16_t, 19> weapon_name{0x3cbc};
inline constexpr Arr<uint16_t, 19> weapon_description{0x4082};
inline constexpr Arr<int16_t, 19> weapon_min_prestige{0x40ce};
inline constexpr Arr<int16_t, 19> weapon_min_loyalty{0x40f4};  // % of purchases from supplier
inline constexpr Arr<int16_t, 19> weapon_cost{0x411a};
inline constexpr Arr<int16_t, 19> weapon_icon{0x4140};
inline constexpr Arr<int16_t, 19> weapon_picture{0x4166};
inline constexpr Arr<uint16_t, 4> supplier_adjective{0x3b22};
// Not saved: purchase months per supplier (USA, Britain, France, private), total, last month.
inline constexpr Arr<int16_t, 4> supplier_deals{0x9294};
inline constexpr Var<int16_t> total_deals{0x929c};
inline constexpr Var<int16_t> last_deal_month{0x929e};
// A word in DGROUP (pointer tables of message templates are read this way).
inline uint16_t word_at(uint16_t off) { return ds.get<uint16_t>(off); }

}  // namespace conflict::g
