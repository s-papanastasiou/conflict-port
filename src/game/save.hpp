// Save and load (1000:25b3 / 1000:29ed). The file is the raw bytes of 42 DGROUP regions.
#pragma once

#include <cstdint>
#include <filesystem>

namespace conflict::save {

void save_game();  // 1000:25b3 — asks for a slot, writes confN.sav
void load_game();  // 1000:29ed — asks for a slot, reads confN.sav; sets loaded_game_flag

// Direct access for tools and tests (no menus).
bool write_file(const std::filesystem::path& path);
bool read_file(const std::filesystem::path& path);
constexpr uint32_t kFileSize = 7372;

}  // namespace conflict::save
