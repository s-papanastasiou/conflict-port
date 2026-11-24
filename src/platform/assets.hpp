// Game data as the original holds it in memory after start-up.
#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace conflict {

struct Assets {
    // Small fonts (DS:b294 etech.fnt, DS:b296 extra.fnt): glyph for char c at [c * 9].
    // The original allocates 0x465 bytes and fills 0x1b0..0x465; we pad so any char is readable.
    std::vector<uint8_t> font_etech, font_extra;
    // Large fonts (DS:b298 xini.fnt, DS:b29a xguard.fnt): 0x3f0 bytes, glyph for c at [c*36-0x900].
    std::vector<uint8_t> font_xini, font_xguard;
    // 32x32 icons (DS:b29c): xnclr.fnt (0x380 bytes) followed by xtest.fnt (0x480 bytes).
    std::vector<uint8_t> icons;
    // Maps loaded by 1f39:078b, in this order (IRAQ1.MP is never loaded).
    std::vector<std::vector<uint8_t>> maps;
    // Weapon pictures loaded by 1f39:0783.
    std::vector<std::vector<uint8_t>> pictures;

    static constexpr std::array<const char*, 5> kMapFiles = {"MDLEAST.MP", "ISRAEL.MP", "IRAQ.MP",
                                                             "LIBYA.MP", "SYRIA.MP"};
    static constexpr std::array<const char*, 19> kPictureFiles = {
        "T16.DAT",   "T62.DAT",    "TORNADO.DAT", "APACHE.DAT", "AWACS.DAT",
        "F111.DAT",  "M1.DAT",     "M15.DAT",     "SAMCHAP.DAT", "500MD.DAT",
        "AMX13.DAT", "AMX30.DAT",  "CHIEFT.DAT",  "MI24.DAT",   "MIG27.DAT",
        "MIRAGE.DAT", "NIMROD.DAT", "SA8.DAT",    "SCORPION.DAT"};

    // Loads everything from dos::game_dir(). Returns false with a message in `error` if a file
    // the original treats as mandatory is missing.
    bool load(std::string& error);
};

}  // namespace conflict
