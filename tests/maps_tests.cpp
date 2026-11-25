// Checks the redrawn maps built into the program (no original files needed): five maps, within
// the 40000 bytes the game reads per map, ending with the (639, 349) record, and drawing a full
// screen through the EGA model with the expected colours in place.
#include <cstdio>
#include <set>

#include "platform/ega.hpp"
#include "platform/redrawn_maps.hpp"

using namespace conflict;

static int failures = 0;
#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                                \
        }                                                                              \
    } while (0)

int main() {
    // Interior point of each map (inside the frame) and its expected colour: sea / a country.
    struct Probe { int x, y, colour; };
    const Probe probes[5][2] = {
        {{40, 40, 11}, {300, 250, 14}},    // MDLEAST: Mediterranean, Saudi Arabia
        {{70, 40, 11}, {150, 280, 12}},    // ISRAEL: sea, Negev (Israel)
        {{250, 150, 10}, {440, 290, 11}},  // IRAQ: Iraq, Gulf
        {{120, 230, 14}, {380, 200, 10}},  // LIBYA: Libya, Egypt
        {{250, 120, 14}, {30, 150, 11}},   // SYRIA: Syria, sea
    };
    for (int i = 0; i < 5; ++i) {
        auto m = redrawn_maps::map(i);
        CHECK(!m.empty() && m.size() <= 40000);
        if (m.size() < 6) continue;
        // last record: [colour][x_end = 639][y = 349], then one trailing byte
        size_t last = m.size() - 6;
        CHECK((m[last + 1] | m[last + 2] << 8) == 639 && (m[last + 3] | m[last + 4] << 8) == 349);
        ega::Ega e;
        e.set_mode();
        e.draw_map(m);
        CHECK(e.pixel(0, 0) == 7 && e.pixel(600, 340) == 7);  // grey outside the map
        std::set<int> colours;
        for (int y = 0; y < ega::kHeight; ++y)
            for (int x = 0; x < ega::kWidth; ++x) colours.insert(e.pixel(x, y));
        CHECK(colours.size() >= 5);
        for (const auto& p : probes[i]) {
            if (e.pixel(p.x, p.y) != p.colour)
                std::fprintf(stderr, "map %d: pixel (%d,%d) is %d, expected %d\n", i, p.x, p.y,
                             e.pixel(p.x, p.y), p.colour);
            CHECK(e.pixel(p.x, p.y) == p.colour);
        }
    }
    CHECK(redrawn_maps::map(5).empty());
    if (failures) std::fprintf(stderr, "%d check(s) failed\n", failures);
    else std::printf("redrawn maps: all checks passed\n");
    return failures ? 1 : 0;
}
