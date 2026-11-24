#include "game/fixes.hpp"

namespace conflict::fixes {

namespace {
bool g_enabled = true;
}

void set_enabled(bool on) { g_enabled = on; }
bool enabled() { return g_enabled; }

}  // namespace conflict::fixes
