// Named views of CONFLICT's globals in DGROUP. Offsets are DS offsets in the original.
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
inline constexpr Var<int16_t> progress_dot_x{0xb2c4};  // next mark of the "Thinking..." progress line

// ---- text buffers ----
inline constexpr uint16_t kWindowTitle = 0xa1f2;   // char[200]
inline constexpr uint16_t kWindowText = 0xa2ba;    // char[250]
inline constexpr uint16_t kWindowLines = 0xa3b4;   // char[250]
inline constexpr uint16_t kNewsText = 0xa4ae;      // char[2500], saved, cleared every turn
inline constexpr uint16_t kReportText = 0xae72;    // char[1000], saved
inline constexpr uint16_t kSqueezeScratch = 0x874c;  // 100 bytes cleared by squeeze_spaces

// ---- game state: the 42 saved variables, in file order (see save.cpp) ----
// Per-nation arrays are indexed by Nation; values are as the original keeps them.
inline constexpr Arr<int16_t, 8> gnp{0x92f0};             // economy size; a fallen nation's slot holds its conqueror
inline constexpr Arr<int16_t, 8> army_reserves{0x9300};   // troops in reserve, refilling battle losses
inline constexpr Arr<int16_t, 64> war_table{0x9310};      // at_war(a, b): month of the war, 0 = peace
inline constexpr Arr<int16_t, 8> treasury{0x9390};        // defence funds ($M); grows by defence_share % of gnp a month
inline constexpr Arr<int16_t, 640> forces_table{0x93a0};  // forces(owner, location, type); type 9: border flag
inline constexpr Arr<int16_t, 8> front_line{0x98a0};      // Israel's front against each neighbour; + = Israel advancing, +-5 ends the war
inline constexpr Arr<int16_t, 9> border_brigades{0x98b0}; // Israeli brigades: [n] on the border with n, [7] at home, [8] policing the West Bank
inline constexpr Arr<int16_t, 8> morale{0x98c2};          // army morale; troops fight weaker when instability exceeds it
inline constexpr Arr<int16_t, 950> arms_order_table{0x98d2};  // arms_order(weapon, month)
inline constexpr Arr<int16_t, 8> defence_share{0xa03e};   // % of GNP spent on defence (yearly budget meeting)
inline constexpr Arr<int16_t, 8> prestige{0xa04e};        // international prestige 0..10 (Foreign Office text, weapon access)
inline constexpr Arr<int16_t, 64> relations_table{0xa05e};  // relations(a, b), stored at a > b: 1 (worst) .. 9 (best)
inline constexpr Arr<int16_t, 8> relation_directive{0xa0de};    // relation with Israel ordered by diplomacy, applied at month end (0 none)
inline constexpr Arr<int16_t, 8> insurgency_directive{0xa0ee};  // insurgency ordered by intelligence, applied at month end (0 none)
inline constexpr Arr<int16_t, 8> nuclear{0xa0fe};         // nuclear programme 0..10 (10 = weapons)
inline constexpr Arr<int16_t, 8> nation_status{0xa10e};   // instability 0 (firm) .. 10 (collapsed)
inline constexpr Arr<int16_t, 8> insurgency{0xa11e};      // insurgency 0..10
inline constexpr Arr<int16_t, 8> strike_order{0xa12e};    // Israel's order this month: 1 air bases, 2 military, 3 civilian, 4 nuclear sites, 5 nuclear strike
inline constexpr Arr<int16_t, 8> faction{0xa13e};         // controlling faction / pressure group (index into the faction names at DS:0fae)
inline constexpr Arr<int16_t, 8> rival{0xa14e};           // main rival / strike target (Israel by default; Israel's own entry is 7)
inline constexpr Arr<int16_t, 8> peace_offer{0xa15e};     // 1: offered Israel peace, 2: peace proposed (checked in the war)
inline constexpr Arr<uint16_t, 25> headline_ptr{0xa16e};  // near pointers into kReportText (6c39)
inline constexpr Arr<uint16_t, 25> news_ptr{0xa1a0};      // near pointers into kNewsText (6cb7)
inline constexpr Arr<uint16_t, 8> war_status_text{0xa1d2};  // text shown at the border during a war with Israel
inline constexpr Arr<uint16_t, 8> digest_part{0xa1e2};    // each nation's part of the news digest (kDigestText)
inline constexpr Var<int16_t> news_count{0xb25a};
inline constexpr Var<int16_t> headline_count{0xb25c};
inline constexpr Var<int16_t> report_text_used{0xb25e};
inline constexpr Var<int16_t> news_text_used{0xb260};
// Sum of all relations, less wars (150 at the start). The lower, the higher the "Middle East
// tension" the Foreign Office reports (below 121: "totally dire"); low values call a summit.
inline constexpr Var<int16_t> middle_east_calm{0xb262};
// US disapproval of Israel, 0 = relations excellent. Rises with attacks and strikes, falls with
// summits; from 50 the US stops aid and arms deliveries.
inline constexpr Var<int16_t> us_disapproval{0xb264};
inline constexpr Var<int16_t> west_bank_tactics{0xb266};  // policing of the West Bank: 3 soft, 6 hard
inline constexpr Var<int16_t> turn_count{0xb268};
inline constexpr Var<int16_t> land_for_peace{0xb26a};     // 1 once Israel has handed a border region to a neighbour
inline constexpr Var<int16_t> summit_agreement{0xb26c};   // 1 if Israel signed this year's summit agreement
// Damage to Israel from enemy strikes this month: 7 = none, else kind * 100 + attacking nation.
inline constexpr Var<int16_t> strike_damage{0xb26e};
inline constexpr Var<int16_t> sb270_menu_context{0xb270};  // same as menu_context; saved
inline constexpr Var<int16_t> year{0xb272};               // from 1997
inline constexpr Var<int16_t> month_of_year{0xb274};      // 1..12
inline constexpr Var<int16_t> acts_of_violence{0xb276};   // Israel's, for the end-of-game summary

inline constexpr Var<int16_t> loaded_game_flag{0xb278};

// ---- accessors for the 2-D / 3-D state arrays ----
inline Var<int16_t> forces(int owner, int location, int type) {
    return forces_table[(owner * 8 + location) * 10 + type];
}
inline Var<int16_t> border(int a, int b) { return forces(a, b, 9); }
inline Var<int16_t> relations(int a, int b) { return relations_table[a * 8 + b]; }
inline Var<int16_t> at_war(int a, int b) { return war_table[a * 8 + b]; }
// Relation between two nations, stored once at [larger][smaller]; rel(a, a) is the diagonal.
inline Var<int16_t> rel(int a, int b) { return a >= b ? relations(a, b) : relations(b, a); }
// Arms on order: [weapon 0..18][month 0..49] units arriving that month.
inline Var<int16_t> arms_order(int weapon, int month) { return arms_order_table[weapon * 50 + month]; }
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
