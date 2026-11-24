// Core checks that need the original files: CONFLICT_GAME_DIR must point at them.
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

#include "game/crt.hpp"
#include "game/dgroup.hpp"
#include "game/save.hpp"
#include "platform/dosfile.hpp"

using namespace conflict;

static int failures = 0;
#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::fprintf(stderr, "%s:%d: CHECK failed: %s\n", __FILE__, __LINE__, #cond); \
            ++failures;                                                      \
        }                                                                    \
    } while (0)

static std::vector<char> slurp(const std::filesystem::path& p) {
    std::ifstream f(p, std::ios::binary);
    return {std::istreambuf_iterator<char>(f), {}};
}

int main() {
    const char* dir = std::getenv("CONFLICT_GAME_DIR");
    if (!dir) {
        std::fprintf(stderr, "set CONFLICT_GAME_DIR to the original game files\n");
        return 77;  // skipped
    }
    dos::set_game_dir(dir);
    std::string err;
    auto exe = dos::resolve("CONFLICT.EXE");
    CHECK(exe && ds.load(*exe, err));

    // rand(): ANSI C LCG; seed 1 gives the classic sequence.
    crt::srand(1);
    int expect[] = {16838, 5758, 10113, 17515, 31051};
    for (int e : expect) CHECK(crt::rand() == e);
    // random(n) stays in 1..n.
    crt::srand(12345);
    for (int i = 0; i < 10000; ++i) {
        int r = crt::random(7);
        CHECK(r >= 1 && r <= 7);
        if (r < 1 || r > 7) break;
    }

    // Save files round-trip byte for byte.
    auto tmp = std::filesystem::temp_directory_path() / "conflict_roundtrip.sav";
    int n = 0;
    for (const auto& e : std::filesystem::directory_iterator(dir)) {
        auto name = e.path().filename().string();
        if (name.size() < 4 || name.find(".SAV") == std::string::npos) continue;
        CHECK(save::read_file(e.path()));
        CHECK(save::write_file(tmp));
        CHECK(slurp(tmp) == slurp(e.path()));
        CHECK(std::filesystem::file_size(tmp) == save::kFileSize);
        ++n;
    }
    std::filesystem::remove(tmp);
    CHECK(n > 0);
    std::printf("save files checked: %d\n", n);

    if (failures) std::fprintf(stderr, "%d check(s) failed\n", failures);
    else std::printf("all checks passed\n");
    return failures ? 1 : 0;
}
