// Checks that the bug fixes (docs/ORIGINAL_BUGS.md) change behaviour as intended. Each test runs
// the same scenario with fixes off (original) and on. Needs CONFLICT_GAME_DIR.
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <string>

#include "game/border.hpp"
#include "game/crt.hpp"
#include "game/dgroup.hpp"
#include "game/fixes.hpp"
#include "game/globals.hpp"
#include "game/news.hpp"
#include "game/save.hpp"
#include "game/setup.hpp"
#include "game/war.hpp"
#include "platform/dosfile.hpp"

using namespace conflict;
using namespace conflict::g;

static int failures = 0;
#define CHECK(bug, cond)                                                                   \
    do {                                                                                   \
        if (!(cond)) {                                                                     \
            std::fprintf(stderr, "bug #%d: CHECK failed: %s (line %d)\n", bug, #cond, __LINE__); \
            ++failures;                                                                    \
        }                                                                                  \
    } while (0)

static std::string exe;
static std::string game_dir;

// Fresh state: data segment from the EXE plus a new game with a fixed seed.
static void fresh(uint32_t seed = 7) {
    std::string err;
    ds.load(exe, err);
    crt::srand(seed);
    bool on = fixes::enabled();
    fixes::set_enabled(false);
    setup::new_game();
    fixes::set_enabled(on);
}

// Runs `scenario` with fixes off and on; returns {original, fixed} results of `measure`.
template <class Scenario, class Measure>
static std::pair<int, int> both(Scenario scenario, Measure measure) {
    int r[2];
    for (int on = 0; on < 2; ++on) {
        fixes::set_enabled(on != 0);
        try {
            scenario();
        } catch (const crt::Restart& r) {
            std::fprintf(stderr, "(scenario ended the game: restart %d, fixes %s)\n", r.value, on ? "on" : "off");
        }
        r[on] = measure();
    }
    fixes::set_enabled(true);
    return {r[0], r[1]};
}

int main() {
    const char* dir = std::getenv("CONFLICT_GAME_DIR");
    if (!dir) return 77;
    game_dir = dir;
    dos::set_game_dir(dir);
    exe = dos::resolve("CONFLICT.EXE")->string();

    {  // #2 posting a brigade moves SAMs into the SAM slot
        auto [orig, fixed] = both([] { fresh(); forces(Israel, Israel, Sams) = 40; border::post_brigade(Syria); },
                                  [] { return int(forces(Israel, Syria, Sams)); });
        CHECK(2, orig == 0 && fixed == 10);
    }
    {  // #3 a pair with no forces does not count as fighting after another pair fought
        auto [orig, fixed] = both(
            [] {
                fresh();
                relations(Israel, Egypt) = 1;   // Egypt (lower index pair first: 7,0)
                forces(Israel, Egypt, Troops) = 500;
                forces(Egypt, Israel, Troops) = 500;
                relations(Syria, Jordan) = 1;    // later pair 6,3? (a=6 > b=3 comes before a=7)
                relations(Israel, Lebanon) = 1;  // pair 7,4: nothing on that border
                for (int k = 0; k < 9; ++k) { forces(Israel, Lebanon, k) = 0; forces(Lebanon, Israel, k) = 0; }
                at_war(Israel, Lebanon) = 0;
                war::fight_wars();
            },
            [] { return int(at_war(Israel, Lebanon)); });
        CHECK(3, orig > fixed && fixed == 0);
    }
    {  // #14 overlapping 4-character search
        auto [orig, fixed] = both([] {}, [] { return news::str_find4("IIraq", "Iraq"); });
        CHECK(14, orig == 0 && fixed == 1);
    }
    {  // #18 Egypt sends only its share (80 %) against Israel
        auto [orig, fixed] = both(
            [] {
                fresh();
                relations(Libya, Egypt) = 1;  // Libya hostile to Egypt -> share 8
                forces(Egypt, Egypt, MainBattleTanks) = 1000;
                forces(Egypt, Israel, MainBattleTanks) = 0;
                war::mobilise(Israel, Egypt);
            },
            [] { return int(forces(Egypt, Israel, MainBattleTanks)); });
        CHECK(18, orig == 1000 && fixed == 800);
    }
    {  // #19 Israel's gunships stay out of a Syria-Iraq war
        auto [orig, fixed] = both(
            [] {
                fresh();
                forces(Israel, Iraq, Gunships) = 50;
                forces(Syria, Iraq, AttackHelicopters) = 5;
                forces(Iraq, Syria, Sams) = 100;  // Iraqi SAMs facing Syria
                forces(Iraq, Syria, Troops) = 100;
                war::helicopter_battle(Syria, Iraq);
            },
            [] { return int(forces(Israel, Iraq, Gunships)); });
        CHECK(19, orig < 50 && fixed == 50);
    }
    {  // #20 a strike on a small nuclear programme cannot make it negative
        int orig = 1, fixed = 1;
        for (uint32_t seed = 1; seed < 200 && (orig >= 0); ++seed) {
            auto r = both(
                [seed] {
                    fresh(seed);
                    nuclear[Syria] = 1;
                    forces(Israel, Israel, Bombers) = 20;
                    sa12e[Syria] = 4;
                    war::israeli_strikes();
                },
                [] { return int(nuclear[Syria]); });
            orig = r.first;
            fixed = r.second;
        }
        CHECK(20, orig < 0 && fixed == 0);
    }
    {  // #22 a large SAM force keeps shooting helicopters down
        auto [orig, fixed] = both(
            [] {
                fresh(3);
                forces(Israel, Egypt, Sams) = 425;
                forces(Israel, Egypt, Troops) = 1000;
                forces(Egypt, Israel, AttackHelicopters) = 100;
                forces(Israel, Egypt, Gunships) = 0;
                war::helicopter_battle(Israel, Egypt);
            },
            [] { return int(forces(Egypt, Israel, AttackHelicopters)); });
        CHECK(22, fixed < orig);
    }
    {  // #23 the front-line markers stay inside the 10-character bar
        auto [orig, fixed] = both(
            [] {
                fresh();
                at_war(Israel, Syria) = 4;
                s98a0[Syria] = 5;
                border::border_report(Syria);
            },
            [] { return int(std::strlen(ds.str(0x33ae))); });
        CHECK(23, orig != 10 && fixed == 10);
    }
    if (failures) std::fprintf(stderr, "%d fix check(s) failed\n", failures);
    else std::printf("all fix checks passed\n");
    return failures ? 1 : 0;
}
