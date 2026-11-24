// National Command Headquarters: Israel's borders with Egypt, Jordan, Lebanon and Syria
// (code segment 1000).
#pragma once

#include <cstdint>

namespace conflict::border {

void strategic_action();         // 1000:7e9c
void border_command(int choice);  // 1000:8462 (1 Egypt, 2 Jordan, 3 Lebanon, 4 Syria)
uint16_t border_report(int n);   // 1000:76f4 — text at DS:91b4; in war also draws the front
void occupied_border(int n);     // 1000:8df1
void orders_given(int n);        // 1000:8ce6

// Peacetime orders
void post_brigade(int n);        // 1000:8e39
void mass_on_border(int n);      // 1000:8eb4
void pull_back(int n);           // 1000:8f4e
void match_enemy(int n);         // 1000:8fcc
void commit_all(int n);          // 1000:90e3 (also a wartime order)
// Wartime orders
void reinforce_armour(int n);    // 1000:9147
void sam_cover(int n);           // 1000:921d
void air_support(int n);         // 1000:9281
void withdraw_brigade(int n);    // 1000:8fb1

}  // namespace conflict::border
