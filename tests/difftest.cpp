// Runs one ported function on a known state, for comparison with the original (tools/difftest.py).
//   difftest GAME_DIR SAVE|- SEED FUNCTION [ARGS...] OUT_PREFIX
// Writes OUT_PREFIX.before / OUT_PREFIX.after (64 KB DGROUP images) and prints the original
// function's address.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <map>
#include <string>
#include <vector>

#include "game/ai.hpp"
#include "game/crt.hpp"
#include "game/dgroup.hpp"
#include "game/fixes.hpp"
#include "game/monthly.hpp"
#include "game/news.hpp"
#include "game/save.hpp"
#include "game/newspaper.hpp"
#include "game/arms.hpp"
#include "game/war.hpp"
#include "game/border.hpp"
#include "game/month.hpp"
#include "game/policy.hpp"
#include "game/reports.hpp"
#include "game/endgame.hpp"
#include "game/yearly.hpp"
#include "game/affairs.hpp"
#include "game/diplomacy.hpp"
#include "game/setup.hpp"
#include "game/sys.hpp"
#include "game/globals.hpp"
#include "platform/host.hpp"
#include "platform/assets.hpp"
#include "platform/ega.hpp"
#include "game/startup.hpp"
#include "platform/dosfile.hpp"

using namespace conflict;

namespace {
struct Entry {
    const char* address;
    int nargs;
    std::function<int(const std::vector<int>&)> run;
};

const std::map<std::string, Entry>& registry() {
    static const std::map<std::string, Entry> r = {
        {"random", {"1000:0010", 1, [](auto& a) { return crt::random(a[0]); }}},
        {"new_game", {"1000:2e39", 0, [](auto&) { setup::new_game(); return 0; }}},
        {"nation_turn", {"1000:3138", 1, [](auto& a) { ai::nation_turn(a[0]); return 0; }}},
        {"collapse", {"1000:58be", 1, [](auto& a) { ai::collapse(a[0]); return 0; }}},
        {"withdraw_forces", {"1000:595d", 1, [](auto& a) { ai::withdraw_forces(a[0]); return 0; }}},
        {"choose_rival", {"1000:6e8e", 1, [](auto& a) { ai::choose_rival(a[0]); return 0; }}},
        {"adjust_relation", {"1000:3897", 3, [](auto& a) { ai::adjust_relation(a[0], a[1], a[2]); return 0; }}},
        {"pressure", {"1000:3a23", 2, [](auto& a) { ai::pressure(a[0], a[1]); return 0; }}},
        {"relation_news", {"1d93:0007", 2, [](auto& a) { ai::relation_news(a[0], a[1]); return 0; }}},
        {"instability_news", {"1d93:037b", 1, [](auto& a) { ai::instability_news(a[0]); return 0; }}},
        {"event_news", {"1d93:0b2a", 1, [](auto& a) { ai::event_news(a[0]); return 0; }}},
        {"end_of_month", {"1000:56c7", 0, [](auto&) { monthly::end_of_month(); return 0; }}},
        {"drift_news", {"1d93:0468", 2, [](auto& a) { monthly::drift_news(a[0], a[1]); return 0; }}},
        {"build_digest", {"1000:3036", 0, [](auto&) { monthly::build_digest(); return 0; }}},
        {"deliver_arms", {"1000:9fd9", 0, [](auto&) { monthly::deliver_arms(); return 0; }}},
        {"garrison_borders", {"1000:5a52", 0, [](auto&) { monthly::garrison_borders(); return 0; }}},
        {"foreign_office_advice", {"1000:502c", 0, [](auto&) { affairs::foreign_office_advice(); return 0; }}},
        {"nation_affairs", {"1000:549b", 1, [](auto& a) { affairs::nation_affairs(a[0]); return 0; }}},
        {"diplomacy", {"1000:61b0", 1, [](auto& a) { diplomacy::diplomacy(a[0]); return 0; }}},
        {"intelligence", {"1000:6fa6", 1, [](auto& a) { diplomacy::intelligence(a[0]); return 0; }}},
        {"make_peace", {"1b92:141b", 2, [](auto& a) { diplomacy::make_peace(a[0], a[1]); return 0; }}},
        {"leadership_analysis", {"1000:aff1", 0, [](auto&) { endgame::leadership_analysis(); return 0; }}},
        {"closing_newspaper", {"1000:af8a", 1, [](auto& a) { endgame::closing_newspaper(a[0]); return 0; }}},
        {"budget_meeting", {"1d93:111f", 1, [](auto& a) { yearly::budget_meeting(a[0]); return 0; }}},
        {"summit", {"1d93:0c02", 0, [](auto&) { yearly::summit(); return 0; }}},
        {"procurement", {"1000:9376", 0, [](auto&) { arms::procurement(); return 0; }}},
        {"choose_quantity", {"1000:9d3e", 1, [](auto& a) { return arms::choose_quantity(a[0]); }}},
        {"review_forces", {"1000:a08f", 0, [](auto&) { reports::review_forces(); return 0; }}},
        {"nuclear_program", {"1000:aac8", 0, [](auto&) { policy::nuclear_program(); return 0; }}},
        {"west_bank", {"1000:a86d", 0, [](auto&) { policy::west_bank(); return 0; }}},
        {"player_month", {"1000:4675", 0, [](auto&) { month::player_month(); return 0; }}},
        {"strategic_action", {"1000:7e9c", 0, [](auto&) { border::strategic_action(); return 0; }}},
        {"border_command", {"1000:8462", 1, [](auto& a) { border::border_command(a[0]); return 0; }}},
        {"border_report", {"1000:76f4", 1, [](auto& a) { return int(border::border_report(a[0])); }}},
        {"fight_wars", {"1b92:0bd1", 0, [](auto&) { war::fight_wars(); return 0; }}},
        {"air_strikes", {"1b92:0007", 0, [](auto&) { war::air_strikes(); return 0; }}},
        {"israeli_strikes", {"1b92:0714", 0, [](auto&) { war::israeli_strikes(); return 0; }}},
        {"ground_battle", {"1b92:1c4d", 2, [](auto& a) { war::ground_battle(a[0], a[1]); return 0; }}},
        {"helicopter_battle", {"1b92:16c1", 2, [](auto& a) { war::helicopter_battle(a[0], a[1]); return 0; }}},
        {"air_battle", {"1b92:1993", 4, [](auto& a) { war::air_battle(a[0], a[1], a[2], a[3]); return 0; }}},
        {"mobilise", {"1000:5aa5", 2, [](auto& a) { war::mobilise(a[0], a[1]); return 0; }}},
        {"casualty_news", {"1d93:0599", 4, [](auto& a) { war::casualty_news(a[0], a[1], a[2], a[3]); return 0; }}},
        {"newspaper", {"1000:14bc", 0, [](auto&) { newspaper::show(); return 0; }}},
        {"covert_operations", {"1000:74e1", 0, [](auto&) { monthly::covert_operations(); return 0; }}},
    };
    return r;
}

void dump(const std::string& path) {
    std::ofstream f(path, std::ios::binary);
    f.write(reinterpret_cast<const char*>(ds.data()), Dgroup::kSize);
}
}  // namespace

int main(int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--list") {
        for (auto& [name, e] : registry()) std::printf("%s %s %d\n", name.c_str(), e.address, e.nargs);
        return 0;
    }
    if (argc < 6) {
        std::fprintf(stderr, "usage: difftest GAME_DIR SAVE|- SEED FUNCTION [ARGS...] OUT_PREFIX\n");
        return 2;
    }
    fixes::set_enabled(false);  // compare against the original, bugs and all
    dos::set_game_dir(argv[1]);
    std::string err;
    auto exe = dos::resolve("CONFLICT.EXE");
    if (!exe || !ds.load(*exe, err)) {
        std::fprintf(stderr, "cannot load CONFLICT.EXE: %s\n", err.c_str());
        return 1;
    }
    std::string save = argv[2];
    if (save != "-" && !save::read_file(save)) {
        std::fprintf(stderr, "cannot read %s\n", save.c_str());
        return 1;
    }
    // DIFFTEST_POKES="b2d6=1,b2d8=2": 16-bit values written into DGROUP before the run.
    if (const char* pokes = std::getenv("DIFFTEST_POKES")) {
        std::string p = pokes;
        size_t i = 0;
        while (i < p.size()) {
            size_t j = p.find(',', i);
            if (j == std::string::npos) j = p.size();
            std::string item = p.substr(i, j - i);
            size_t eq = item.find('=');
            if (eq != std::string::npos)
                ds.put<int16_t>(uint16_t(std::stoul(item.substr(0, eq), nullptr, 16)),
                                int16_t(std::stol(item.substr(eq + 1), nullptr, 0)));
            i = j + 1;
        }
    }
    // Pause loops in the original scale with this calibration; make them end at once.
    g::delay_calibration = 1e9f;
    if (const char* k = std::getenv("DIFFTEST_KEYS")) {
        std::vector<uint16_t> keys;
        std::string kerr;
        if (!parse_key_script(k, keys, kerr)) {
            std::fprintf(stderr, "%s\n", kerr.c_str());
            return 2;
        }
        sys::set_test_keys(keys);
    }
    crt::srand(uint32_t(std::strtoul(argv[3], nullptr, 0)));
    auto it = registry().find(argv[4]);
    if (it == registry().end()) {
        std::fprintf(stderr, "unknown function %s\n", argv[4]);
        return 2;
    }
    std::vector<int> args;
    for (int i = 5; i < argc - 1; ++i) args.push_back(std::atoi(argv[i]));
    if (int(args.size()) != it->second.nargs) {
        std::fprintf(stderr, "%s takes %d arguments\n", argv[4], it->second.nargs);
        return 2;
    }
    // DIFFTEST_SCREEN=file.bmp: draw for real (fonts, maps) and save the screen afterwards. This
    // changes text widths, so use it for looking, not for comparing.
    const char* screen_path = std::getenv("DIFFTEST_SCREEN");
    static Assets assets;
    static ega::Ega screen;
    if (screen_path) {
        if (!assets.load(err)) {
            std::fprintf(stderr, "%s\n", err.c_str());
            return 1;
        }
        sys::attach(screen, nullptr, assets);
        startup::load_fonts();
        g::bg_colour = 15;
    }
    std::string out = argv[argc - 1];
    dump(out + ".before");
    int ax = 0;
    try {
        ax = it->second.run(args);
    } catch (const crt::Restart& r) {
        ax = 0x10000 + r.value;  // longjmp(restart_jmpbuf, value): reported as "restart"
    }
    dump(out + ".after");
    if (screen_path) save_screenshot(screen, screen_path);
    if (ax >= 0x10000) std::printf("%s restart:%d\n", it->second.address, ax - 0x10000);
    else std::printf("%s %d\n", it->second.address, ax & 0xffff);
    return 0;
}
