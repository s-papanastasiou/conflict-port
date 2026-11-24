// CONFLICT: Middle East Political Simulator — Linux port. Entry point (1000:b681 main).
//   conflict [--game DIR] [--scale N] [--keys LIST [--shots DIR] [--headless]]
// DIR must contain the original game files, including CONFLICT.EXE. --keys feeds a scripted
// key sequence (see parse_key_script) for testing; --shots saves a screenshot at every key wait.
// --original-bugs turns off the fixes listed in docs/ORIGINAL_BUGS.md (exact 1990 behaviour).
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>

#include "game/crt.hpp"
#include "game/dgroup.hpp"
#include "game/fixes.hpp"
#include "game/globals.hpp"
#include "game/save.hpp"
#include "game/startup.hpp"
#include "game/sys.hpp"
#include "game/turn.hpp"
#include "game/ui.hpp"
#include "platform/assets.hpp"
#include "platform/dosfile.hpp"
#include "platform/ega.hpp"
#include "platform/host.hpp"

using namespace conflict;

namespace {

// Everything in main() after setjmp: title, new game or load, then the monthly loop.
void run_game() {
    using namespace sys;
    g::fg_colour = 14;
    gfx_fill_rect(0, 0x27f, 0, 0x15d);
    g::fg_colour = 0;
    ui::draw_icon_border(99);
    startup::title_screen();
    bool load = g::loaded_game_flag != 0;
    do {
        if (load) {
            save::load_game();
            if (g::loaded_game_flag != 0) break;
        }
        load = true;
        turn::new_game();
        for (int n = 0; n < 7; ++n) turn::nation_turn(n);
        turn::end_of_month();
    } while (g::loaded_game_flag != 0);
    for (;;) {
        turn::f3036();
        turn::f9fd9();
        turn::f4675();
        ui::draw_status_bar();
        turn::f5a52();
        std::memset(ds.ptr(g::kNewsText), 0, 0x9c4);
        g::news_count = 0;
        g::news_text_used = 0;
        turn::f74e1();
        for (int n = 0; n < 7; ++n)
            if (g::nation_status[n] != 10) turn::nation_turn(n);
        turn::f1b92_0bd1();
        turn::f1b92_0007();
        turn::end_of_month();
        ++g::turn_count;
    }
}

int game_main() {
    g::fg_colour = 0;
    g::bg_colour = 15;
    crt::srand(crt::dos_time_hsec());
    startup::intro();
    // gfx_load_maps(); chdir(".."); gfx_load_pictures(); — assets are loaded before start-up,
    // all from the game directory (see re/NOTES.md on the chdir).
    startup::calibrate_delay();
    sys::wait_or_key(long(50000.0 / double(float(g::delay_calibration))));
    int jump = 0;  // setjmp(restart_jmpbuf)
    for (;;) {
        try {
            if (jump != 0) {
                g::menu_context = 0;
                g::font_small = g::font_etech;
                int choice = jump == 10
                                 ? ui::menu_popup(0x1e0, 0x28, 0x276, 0x5a, ds.str(0x5690), 0)
                                 : ui::menu_popup(200, 0x46, 400, 0x78, ds.str(g::menu_texts[0]), 0);
                if (choice == 2) return 0;  // Exit
            }
            run_game();
        } catch (const crt::Restart& r) {
            jump = r.value;
        }
    }
}

}  // namespace

int main(int argc, char** argv) {
    std::filesystem::path game = ".";
    int scale = 2;
    std::string keys;
    std::filesystem::path shots;
    bool headless = false;
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--game") && i + 1 < argc) game = argv[++i];
        else if (!std::strcmp(argv[i], "--scale") && i + 1 < argc) scale = std::atoi(argv[++i]);
        else if (!std::strcmp(argv[i], "--keys") && i + 1 < argc) keys = argv[++i];
        else if (!std::strcmp(argv[i], "--shots") && i + 1 < argc) shots = argv[++i];
        else if (!std::strcmp(argv[i], "--headless")) headless = true;
        else if (!std::strcmp(argv[i], "--original-bugs")) fixes::set_enabled(false);
        else {
            std::fprintf(stderr, "usage: %s [--game DIR] [--scale N] [--original-bugs] [--keys LIST [--shots DIR] [--headless]]\n", argv[0]);
            return 2;
        }
    }
    dos::set_game_dir(game);
    std::string err;
    auto exe = dos::resolve("CONFLICT.EXE");
    if (!exe) {
        std::fprintf(stderr, "CONFLICT.EXE not found in %s (use --game DIR)\n", game.string().c_str());
        return 1;
    }
    if (!ds.load(*exe, err)) {
        std::fprintf(stderr, "%s\n", err.c_str());
        return 1;
    }
    Assets assets;
    if (!assets.load(err)) {
        std::fprintf(stderr, "%s\n", err.c_str());
        return 1;
    }
    ega::Ega screen;
    Host host(screen);
    if (!keys.empty()) {
        std::vector<uint16_t> script;
        if (!parse_key_script(keys, script, err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 2;
        }
        host.set_script(std::move(script));
        if (!shots.empty()) {
            std::filesystem::create_directories(shots);
            host.set_shot_dir(shots);
        }
    }
    if (headless ? !host.open_headless() : !host.open("CONFLICT", scale)) return 1;
    sys::attach(screen, &host, assets);
    try {
        return game_main();
    } catch (const QuitRequested&) {
        return 0;
    }
}
