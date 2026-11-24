// Start-up screen of the port (not in the original game): choose the original maps or the redrawn
// ones, then start. Drawn with the game's own fonts and window style, but entirely outside the
// game's memory, so the game itself starts exactly as before.
#pragma once

#include <optional>

namespace conflict {

namespace ega { class Ega; }
class Host;
struct Assets;

enum class MapStyle { Original, Redrawn };

// Shows the launcher until Enter (returns the choice) or Esc (returns nothing: quit).
std::optional<MapStyle> run_launcher(Host& host, ega::Ega& screen, const Assets& assets,
                                     MapStyle initial = MapStyle::Original);

}  // namespace conflict
