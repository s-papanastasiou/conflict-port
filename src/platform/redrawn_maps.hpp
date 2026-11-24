// The redrawn maps (assets/maps/*.MP, made by tools/redraw_maps.py), built into the program.
#pragma once

#include <cstdint>
#include <span>

namespace conflict::redrawn_maps {

// Map `index` in Assets::kMapFiles order (MDLEAST, ISRAEL, IRAQ, LIBYA, SYRIA), in .MP format.
std::span<const uint8_t> map(int index);

}  // namespace conflict::redrawn_maps
