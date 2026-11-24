#include "game/save.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <vector>

#include "game/crt.hpp"
#include "game/dgroup.hpp"
#include "game/globals.hpp"
#include "game/ui.hpp"
#include "platform/dosfile.hpp"

namespace conflict::save {

namespace {
struct Region {
    uint16_t off;
    uint16_t bytes;
};
// fwrite(ptr, size, count) calls of 1000:25b3, in order.
constexpr std::array<Region, 42> kLayout = {{
    {0x92f0, 16},  {0x9300, 16},  {0x9310, 128}, {0x9390, 16},  {0x93a0, 1280}, {0x98a0, 16},
    {0x98b0, 18},  {0x98c2, 16},  {0x98d2, 1900}, {0xa03e, 16}, {0xa04e, 16},   {0xa05e, 128},
    {0xa0de, 16},  {0xa0ee, 16},  {0xa0fe, 16},  {0xa10e, 16},  {0xa11e, 16},   {0xa12e, 16},
    {0xa13e, 16},  {0xa14e, 16},  {0xa15e, 16},  {0xa16e, 50},  {0xa1a0, 50},   {0xa1d2, 16},
    {0xa1e2, 16},  {0xa4ae, 2500}, {0xae72, 1000}, {0xb25a, 2}, {0xb25c, 2},    {0xb25e, 2},
    {0xb260, 2},   {0xb262, 2},   {0xb264, 2},   {0xb266, 2},   {0xb268, 2},    {0xb26a, 2},
    {0xb26c, 2},   {0xb26e, 2},   {0xb270, 2},   {0xb272, 2},   {0xb274, 2},    {0xb276, 2},
}};

constexpr const char* kSlotNames[] = {"conf1.sav", "conf2.sav", "conf3.sav", "conf4.sav", "conf5.sav"};

std::filesystem::path slot_path(int slot) {
    const char* name = kSlotNames[slot - 1];
    if (auto p = dos::resolve(name)) return *p;
    return dos::game_dir() / name;
}
}  // namespace

bool write_file(const std::filesystem::path& path) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return false;
    for (const auto& r : kLayout) f.write(reinterpret_cast<const char*>(ds.ptr(r.off)), r.bytes);
    return bool(f);
}

bool read_file(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::vector<char> d(std::istreambuf_iterator<char>(f), {});
    // fread() of a short file fills what it can; the rest of each region is left unchanged.
    size_t pos = 0;
    for (const auto& r : kLayout) {
        size_t n = std::min<size_t>(r.bytes, d.size() - std::min(pos, d.size()));
        std::copy_n(d.data() + pos, n, reinterpret_cast<char*>(ds.ptr(r.off)));
        pos += n;
    }
    return true;
}

// 1000:25b3
void save_game() {
    g::menu_context = 0;
    g::font_small = g::font_etech;
    int slot = ui::menu_popup(100, 5, 0x15e, 0x82, ds.str(g::menu_texts[4]), 0);
    if (slot == 6) return;  // Cancel
    if (!write_file(slot_path(slot)))
        std::fprintf(stderr, "could not write %s\n", slot_path(slot).string().c_str());
}

// 1000:29ed
void load_game() {
    g::loaded_game_flag = 0;
    int slot = ui::menu_popup(0xb4, 0x1e, 0x1c2, 0xa0, ds.str(0x1927), 0);
    if (slot == 6) return;  // Cancel
    if (!read_file(slot_path(slot))) crt::longjmp_restart(999);
    g::loaded_game_flag = 1;
}

}  // namespace conflict::save
