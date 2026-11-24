// The original's 64 KB data segment (DGROUP), loaded from the user's Conflict.exe.
//
// CONFLICT was built in Zortech C's medium model: every global, string table, menu text and
// heap block sits in one 64 KB segment and is addressed by 16-bit offsets. Game state even
// stores such offsets (e.g. pointers into the news buffers are saved to disk). The port keeps
// that segment as real memory so behaviour, layout and save files stay identical, while the
// C++ code reaches it through the named, typed views below (see globals.hpp).
//
// Nothing from the original is compiled into the port: the initialised part of the segment is
// read from Conflict.exe at start-up.
#pragma once

#include <array>
#include <bit>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>

namespace conflict {

static_assert(std::endian::native == std::endian::little, "DGROUP views assume a little-endian host");

class Dgroup {
public:
    static constexpr uint32_t kSize = 0x10000;
    static constexpr uint16_t kInitBytes = 0x874b;  // initialised data in the EXE; BSS follows

    // Reads the initialised data from Conflict.exe. Returns false with a reason on failure.
    bool load(const std::filesystem::path& exe, std::string& error);

    uint8_t* ptr(uint16_t off) { return mem_.data() + off; }
    const uint8_t* ptr(uint16_t off) const { return mem_.data() + off; }
    char* str(uint16_t off) { return reinterpret_cast<char*>(mem_.data() + off); }
    const uint8_t* data() const { return mem_.data(); }

    // Offset of a pointer into this segment (the 16-bit "near pointer" the original stores).
    uint16_t off(const void* p) const;
    bool contains(const void* p) const;

    template <class T>
    T get(uint16_t off) const {
        T v;
        std::memcpy(&v, mem_.data() + off, sizeof v);
        return v;
    }
    template <class T>
    void put(uint16_t off, T v) {
        std::memcpy(mem_.data() + off, &v, sizeof v);
    }

    // Near heap (malloc in the original). Blocks come from the space above BSS.
    uint16_t malloc(uint16_t size);

private:
    // 16 bytes of slack so 2-byte accesses at 0xffff never leave the buffer.
    alignas(16) std::array<uint8_t, kSize + 16> mem_{};
    uint16_t heap_top_ = 0xb400;
};

extern Dgroup ds;

// A typed variable at a fixed DGROUP offset. Behaves like an lvalue of T.
template <class T>
struct Var {
    uint16_t off;

    constexpr explicit Var(uint16_t o) : off(o) {}
    constexpr Var(const Var&) = default;  // copies the view, not the value

    operator T() const { return ds.get<T>(off); }
    T get() const { return ds.get<T>(off); }
    const Var& operator=(T v) const { ds.put<T>(off, v); return *this; }
    const Var& operator=(const Var& o) const { return *this = o.get(); }
    const Var& operator+=(T v) const { return *this = T(get() + v); }
    const Var& operator-=(T v) const { return *this = T(get() - v); }
    const Var& operator++() const { return *this += T(1); }
    const Var& operator--() const { return *this -= T(1); }
    T operator++(int) const { T v = get(); *this = T(v + 1); return v; }
    T operator--(int) const { T v = get(); *this = T(v - 1); return v; }
};

// A fixed-size array of T at a DGROUP offset. Indexing returns a Var; out-of-range indices
// behave as in the original (they address whatever follows in the segment).
template <class T, int N>
struct Arr {
    uint16_t off;
    static constexpr int size = N;
    Var<T> operator[](int i) const { return Var<T>(uint16_t(off + i * int(sizeof(T)))); }
    uint16_t addr(int i = 0) const { return uint16_t(off + i * int(sizeof(T))); }
};

// A 16-bit near pointer to char stored in DGROUP.
struct NearStr {
    uint16_t off;
    uint16_t raw() const { return ds.get<uint16_t>(off); }
    char* get() const { return ds.str(raw()); }
    operator char*() const { return get(); }
    const NearStr& operator=(uint16_t target) const { ds.put<uint16_t>(off, target); return *this; }
};

}  // namespace conflict
