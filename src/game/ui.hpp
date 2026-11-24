// Windows, menus and icons (code segment 1000 of the original).
// Coordinates: x in pixels (0-639); y in the game's 200-line units, scaled by 1.75 for EGA,
// unless a parameter is documented as a screen row.
#pragma once

namespace conflict::ui {

void draw_panel(int x0, int y0, int x1, int y1);                              // 1000:00b3
void draw_titled_panel(int x0, int y0, int x1, int y1, const char* title);    // 1000:0286
void text_window(int x0, int y0, int x1, int y1, const char* title, const char* text,
                 const char* lines);                                          // 1000:0a7c
void menu_add_item(int x, int y, int width, const char* text);                // 1000:0e13
int menu_popup(int x0, int y0, int x1, int y1, const char* text, int arg);    // 1000:0d24
int menu_select(int x, int y);                                                // 1000:1044
void draw_icon(const char* icon);                                             // 1000:b8ab
void draw_icon_border(int icon);                                              // 1000:24c2
void draw_status_bar();                                                       // 1000:245c

void str_overlay(char* dst, const char* src, int pos);                        // 1000:03bd
void squeeze_spaces(char* s);                                                 // 1000:03e5

}  // namespace conflict::ui
