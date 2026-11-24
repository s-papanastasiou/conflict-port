// Driver and BIOS entry points as the game code calls them. Colours and font pointers are read
// from DGROUP (fg_colour, bg_colour, font_small, font_large) on every call, as the driver did.
#pragma once

#include <cstdint>
#include <vector>

namespace conflict {
namespace ega { class Ega; }
class Host;
struct Assets;
}

namespace conflict::sys {

void attach(ega::Ega& ega, Host* host, const Assets& assets);
// Testing: with no screen attached every driver call is a no-op (text width 0), like the stubbed
// driver in tools/oracle.py; get_key() then returns these keys in order, then Enter.
void set_test_keys(const std::vector<uint16_t>& keys);
ega::Ega& screen();
const Assets& assets();

void gfx_init();                                          // 1f39:0771
void gfx_moveto(int x, int y);                            // 1f39:0801
void gfx_plot(int x, int y);                              // 1f39:0813
void gfx_fill_rect_row(int x0, int x1, int y, int flag);  // 1f39:0827
void gfx_vline(int y0, int y1, int x, int flag);          // 1f39:0858
void gfx_fill_rect(int x0, int x1, int y0, int y1);       // 1f39:091e
void gfx_restore_rect(int x0, int x1, int y0, int y1);    // 1f39:06dd
void gfx_blit_colour(int x, int y, int w, int h, int from, int to);  // 1f39:0889
void gfx_text_small(const char* s);                       // 1f39:08e4
void gfx_text_large(const char* s);                       // 1f39:08f8
int gfx_text_width(const char* s);                        // 1f39:09e9
void gfx_draw_glyph32(const char* src);                   // 1f39:07df
void gfx_draw_map(int index);                             // 1f39:0937
void gfx_draw_picture(int index);                         // 1f39:0746

uint16_t get_key();   // 1000:0444 (BIOS int 16h AH=0)
int kbd_poll_ext();   // 1f39:0a58

// Busy-wait loops of the original, expressed in iterations of an empty `for` loop. The original
// relied on CPU speed; we map iterations to real time (see sys.cpp).
void delay_loop(long iterations);
// `for (long i = 0; i < n && !kbd_poll_ext(); i++)` — waits n iterations unless a key is hit.
// With fix #7 any key, including Enter, ends the wait (the original only reacted to extended
// keys, see kbd_poll_ext).
void wait_or_key(long iterations);

}  // namespace conflict::sys
