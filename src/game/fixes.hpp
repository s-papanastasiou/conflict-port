// Fixes for bugs in the original game. Each guarded code path names its bug number from
// docs/ORIGINAL_BUGS.md, e.g. `if (fixes::fix(18)) { ...fixed... } else { ...original... }`.
// Fixes are on by default; `--original-bugs` (and the differential tests) turn them all off so
// the port behaves exactly like the 1990 game.
#pragma once

namespace conflict::fixes {

void set_enabled(bool on);
bool enabled();
// True if the fix for bug `number` is active (all fixes are switched together).
inline bool fix(int /*number*/) { return enabled(); }

}  // namespace conflict::fixes
