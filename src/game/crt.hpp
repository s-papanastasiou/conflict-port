// Replacements for the Zortech C runtime calls the game makes, with identical behaviour where it
// matters (rand) and DOS-relative file access.
#pragma once

#include <cstdint>
#include <initializer_list>

namespace conflict::crt {

// rand()/srand() (20ca:0003 / 20ca:002a). The 32-bit seed lives at DS:7ff0 like the original.
void srand(uint32_t seed);
int rand();

// 1000:0010 — 1 + (int)(n * rand() / 32768.0), or 0 when n < 0. Returns 1..n.
int random(int n);

// 1d93:1996 — itoa with 16-bit arithmetic (digits, '-', then reversed in place).
void itoa(int value, char* out);

// sprintf (20d6:00b3) for the game's own format strings, which only use %d and %s.
struct FmtArg {
    FmtArg(int v) : is_str(false), i(v), s(nullptr) {}
    FmtArg(const char* v) : is_str(true), i(0), s(v) {}
    bool is_str;
    int i;
    const char* s;
};
void sprintf(char* out, const char* fmt, std::initializer_list<FmtArg> args);

// Hundredths of a second since midnight (int 21h AH=2Ch), used to seed srand.
uint32_t dos_time_hsec();

// setjmp/longjmp on the restart buffer (DS:b27e) become an exception caught in main().
struct Restart {
    int value;  // 999: "I quit" menu, 10: "The End" menu
};
[[noreturn]] void longjmp_restart(int value);

}  // namespace conflict::crt
