// Defence Procurement Office: buying arms from four suppliers (code segment 1000).
#pragma once

namespace conflict::arms {

void procurement();                                // 1000:9376
int list_offers(int supplier, int* offers);        // 1000:9b48
void negotiate(const int* offers, int choice, int supplier);  // 1000:999e
int choose_quantity(int weapon);                   // 1000:9d3e

}  // namespace conflict::arms
