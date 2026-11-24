#include "game/dgroup.hpp"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

namespace conflict {

Dgroup ds;

namespace {
constexpr uint32_t kExpectedExeSize = 118379;
constexpr uint32_t kDgroupPara = 0x128e;  // DS paragraph relative to the load image
}  // namespace

bool Dgroup::load(const std::filesystem::path& exe, std::string& error) {
    std::ifstream f(exe, std::ios::binary);
    if (!f) {
        error = "cannot open " + exe.string();
        return false;
    }
    std::vector<uint8_t> d(std::istreambuf_iterator<char>(f), {});
    if (d.size() < 0x20 || d[0] != 'M' || d[1] != 'Z') {
        error = exe.string() + " is not a DOS executable";
        return false;
    }
    if (d.size() != kExpectedExeSize)
        std::fprintf(stderr, "warning: %s has %zu bytes, expected %u; this port was derived from "
                     "a different build\n", exe.string().c_str(), d.size(), kExpectedExeSize);
    uint32_t header = uint32_t(d[8] | d[9] << 8) * 16;
    uint32_t start = header + kDgroupPara * 16;
    if (start + kInitBytes > d.size()) {
        error = exe.string() + ": data segment not found";
        return false;
    }
    mem_.fill(0);
    std::memcpy(mem_.data(), d.data() + start, kInitBytes);
    static const char kSig[] = "Zortech C 4.00 library";
    if (std::memcmp(mem_.data() + 0x14, kSig, sizeof kSig - 1) != 0) {
        error = exe.string() + ": not the expected CONFLICT build (library signature missing)";
        return false;
    }
    heap_top_ = 0xb400;
    return true;
}

bool Dgroup::contains(const void* p) const {
    auto* b = static_cast<const uint8_t*>(p);
    return b >= mem_.data() && b < mem_.data() + kSize;
}

uint16_t Dgroup::off(const void* p) const {
    if (!contains(p)) {
        std::fprintf(stderr, "fatal: pointer outside DGROUP\n");
        std::abort();
    }
    return uint16_t(static_cast<const uint8_t*>(p) - mem_.data());
}

uint16_t Dgroup::malloc(uint16_t size) {
    uint16_t p = heap_top_;
    uint32_t top = uint32_t(heap_top_) + ((size + 1u) & ~1u);
    if (top > 0xf000) return 0;  // leave room for the stack the original kept here
    heap_top_ = uint16_t(top);
    std::memset(mem_.data() + p, 0, size);
    return p;
}

}  // namespace conflict
