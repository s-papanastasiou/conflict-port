// conflict-viewer: exercises the EGA layer and asset loaders on the original data files.
//   conflict-viewer [--game DIR] [--dump OUTDIR] [--scale N]
// Interactive: Left/Right switch scenes, Esc quits. With --dump, writes scene_N.bmp and exits.
#include <algorithm>
#include <cstdio>
#include <vector>
#include <cstring>
#include <filesystem>
#include <string>

#include "platform/assets.hpp"
#include "platform/dosfile.hpp"
#include "platform/ega.hpp"
#include "platform/host.hpp"

using namespace conflict;

namespace {

// The driver addresses fonts by 16-bit offsets into the data segment; lay them out the same way.
struct FontMem {
    std::vector<uint8_t> mem = std::vector<uint8_t>(0x10000, 0);
    uint16_t etech = 0x1000, extra = 0x1800, xini = 0x2000, xguard = 0x3000;
    explicit FontMem(const Assets& a) {
        std::copy(a.font_etech.begin(), a.font_etech.end(), mem.begin() + etech);
        std::copy(a.font_extra.begin(), a.font_extra.end(), mem.begin() + extra);
        std::copy(a.font_xini.begin(), a.font_xini.end(), mem.begin() + xini);
        std::copy(a.font_xguard.begin(), a.font_xguard.end(), mem.begin() + xguard);
    }
};
const FontMem* g_fonts = nullptr;

// A popup box in the style of the game: white panel, black frame, title in the large font.
void popup(ega::Ega& g, int x0, int y0, int x1, int y1, const char* title,
           const char* const* lines) {
    g.fg = 0;
    g.fill_rect(x0 + 4, x1 + 4, y0 + 4, y1 + 4);  // drop shadow
    g.fg = 15;
    g.fill_rect(x0, x1, y0, y1);
    g.fg = 0;
    g.hspan(x0, x1, y0, 1);
    g.hspan(x0, x1, y1, 1);
    g.vline(y0, y1, x0, 1);
    g.vline(y0, y1, x1, 1);
    g.small_font = g_fonts->etech;
    g.moveto((x0 + x1) / 2 - g.text_width(title) / 2, y0 + 4);
    g.text_small(title);
    int y = y0 + 20;
    for (; *lines; ++lines, y += 12) {
        g.moveto(x0 + 10, y);
        g.text_small(*lines);
    }
}

void draw_scene(ega::Ega& g, const Assets& a, int scene) {
    g.set_mode();
    g.small_font = g_fonts->etech;
    g.large_font = uint16_t(g_fonts->xini - 0x900);
    int nmaps = int(a.maps.size());
    if (scene < nmaps) {
        g.draw_map(a.maps[scene]);
        return;
    }
    scene -= nmaps;
    if (scene == 0) {  // map + picture + popup
        g.draw_map(a.maps[1]);
        g.moveto(440, 40);
        g.draw_picture(a.pictures[6]);
        static const char* lines[] = {"Our international prestige is MODERATE.",
                                      "Middle East tension is turbulent.",
                                      "Current Defence Budget $1050", nullptr};
        popup(g, 40, 220, 400, 300, "National Security Office", lines);
        g.restore_rect(60, 120, 60, 100);  // should show the map again (back buffer copy)
        return;
    }
    if (scene == 1) {  // icon wall like the game's border
        g.fg = 14;
        g.fill_rect(0, 639, 0, 349);
        for (int i = 0; i < 16; ++i) {
            g.fg = uint8_t(i < 8 ? 1 : 4);
            g.bg = 15;
            g.moveto(16 + (i % 8) * 48, 40 + (i / 8) * 48);
            g.draw_icon(a.icons.data() + i * 128);
        }
        g.fg = 1;
        g.large_font = uint16_t(g_fonts->xini - 0x900);
        g.moveto(16, 160);
        g.text_large("CONFLICT");
        g.large_font = uint16_t(g_fonts->xguard - 0x900);
        g.moveto(16, 190);
        g.text_large("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
        g.recolour(0, 280, 640, 60, 14, 2);  // yellow -> green strip
        return;
    }
    if (scene == 2) {  // font samples
        g.fg = 15;
        g.fill_rect(0, 639, 0, 349);
        g.fg = 0;
        const char* sample = "Middle East Political Simulator - 1997! 50% $1000 abcdefghijklmnopqrstuvwxyz";
        g.small_font = g_fonts->etech;
        g.moveto(8, 20);
        g.text_small(sample);
        g.small_font = g_fonts->extra;  // extra.fnt holds symbols, not a second alphabet
        g.moveto(8, 40);
        g.text_small(sample);
        for (int i = 0; i < 8; ++i) {  // sub-pixel offsets: glyph shifting
            g.small_font = g_fonts->etech;
            g.moveto(8 + i, 60 + i * 10);
            g.text_small("Shifted text 0123456789");
        }
        return;
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::filesystem::path game = "conflict", dump;
    int scale = 2;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--game") && i + 1 < argc) game = argv[++i];
        else if (!std::strcmp(argv[i], "--dump") && i + 1 < argc) dump = argv[++i];
        else if (!std::strcmp(argv[i], "--scale") && i + 1 < argc) scale = std::atoi(argv[++i]);
        else {
            std::fprintf(stderr, "usage: %s [--game DIR] [--dump OUTDIR] [--scale N]\n", argv[0]);
            return 2;
        }
    }
    dos::set_game_dir(game);
    Assets assets;
    std::string err;
    if (!assets.load(err)) {
        std::fprintf(stderr, "%s\n", err.c_str());
        return 1;
    }
    FontMem fonts(assets);
    g_fonts = &fonts;
    ega::Ega g;
    g.font_mem = fonts.mem.data();
    const int nscenes = int(assets.maps.size()) + 3;

    if (!dump.empty()) {
        std::filesystem::create_directories(dump);
        for (int s = 0; s < nscenes; ++s) {
            draw_scene(g, assets, s);
            auto p = dump / ("scene_" + std::to_string(s) + ".bmp");
            if (!save_screenshot(g, p)) return 1;
            std::printf("%s\n", p.string().c_str());
        }
        return 0;
    }

    Host host(g);
    if (!host.open("CONFLICT viewer", scale)) return 1;
    int scene = 0;
    try {
        for (;;) {
            draw_scene(g, assets, scene);
            uint16_t k = host.get_key();
            if (k == 0x011b) break;
            if (k == 0x4d00) scene = (scene + 1) % nscenes;
            if (k == 0x4b00) scene = (scene + nscenes - 1) % nscenes;
        }
    } catch (const QuitRequested&) {
    }
    return 0;
}
