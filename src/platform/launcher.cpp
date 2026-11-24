#include "platform/launcher.hpp"

#include <algorithm>
#include <span>
#include <string_view>
#include <vector>

#include "platform/assets.hpp"
#include "platform/ega.hpp"
#include "platform/host.hpp"
#include "platform/redrawn_maps.hpp"

namespace conflict {

namespace {

constexpr uint16_t kKeyEnter = 0x1c0d, kKeyEsc = 0x011b, kKeyUp = 0x4800, kKeyDown = 0x5000,
                   kKeyHome = 0x4700, kKeyEnd = 0x4f00;
constexpr int kWhite = 15, kBlack = 0, kGrey = 7, kDarkBlue = 1;

// The launcher draws with private copies of the game's fonts: small fonts at 0x0000 (etech) and
// 0x1000 (extra), the large font so that glyph c is at large_font + c * 36 (the game biases its
// pointer by -0x900 the same way).
struct Fonts {
    std::vector<uint8_t> mem = std::vector<uint8_t>(0x10000, 0);
    static constexpr uint16_t kEtech = 0x0000, kExtra = 0x1000, kLarge = 0x3000 - 0x900;
    explicit Fonts(const Assets& a) {
        std::copy(a.font_etech.begin(), a.font_etech.end(), mem.begin() + kEtech);
        std::copy(a.font_extra.begin(), a.font_extra.end(), mem.begin() + kExtra);
        std::copy(a.font_xini.begin(), a.font_xini.end(), mem.begin() + 0x3000);
    }
};

// .MP decoder into a 640x350 array (same rules as Ega::draw_map).
std::vector<uint8_t> decode_map(std::span<const uint8_t> map) {
    constexpr int W = ega::kWidth, H = ega::kHeight;
    std::vector<uint8_t> px(size_t(W * H), 0);
    auto span = [&](int x0, int x1, int y, uint8_t c) {
        int row = H - 1 - y;
        for (int x = std::max(x0, 0); x <= std::min(x1, W - 1); ++x) px[size_t(row * W + x)] = c;
    };
    size_t si = 0;
    int cy = 0, cx = 0;
    auto u16 = [&](size_t at) { return int(map[at] | (map[at + 1] << 8)); };
    while (si + 5 <= map.size()) {
        uint8_t c = map[si];
        int xe = u16(si + 1), y = u16(si + 3);
        si += 5;
        while (cy <= H - 1 && cy != y) { span(cx, W - 1, cy, c); ++cy; cx = 0; }
        if (cy > H - 1) break;
        span(cx, xe - 1, cy, c);
        cx = xe;
        if (y == H - 1 && xe == W - 1) break;
    }
    return px;
}

class Screen {
public:
    Screen(ega::Ega& e, const Fonts& f) : e_(e) {
        e_.font_mem = f.mem.data();
        e_.large_font = Fonts::kLarge;
        e_.fix_text_width = true;
        small(Fonts::kEtech);
    }
    void small(uint16_t font) { e_.small_font = font; }
    void fill(int x0, int x1, int y0, int y1, int colour) {
        e_.fg = uint8_t(colour);
        e_.fill_rect(x0, x1, y0, y1);
    }
    // White panel with a black frame and a 2-pixel shadow, like the game's windows (1000:00b3).
    void panel(int x0, int y0, int x1, int y1) {
        fill(x0, x1, y0, y1, kWhite);
        fill(x0 - 1, x1 + 1, y0 - 1, y0 - 1, kBlack);
        fill(x0 - 1, x1 + 1, y1 + 1, y1 + 1, kBlack);
        fill(x0 - 1, x0 - 1, y0 - 1, y1 + 1, kBlack);
        fill(x1 + 1, x1 + 1, y0 - 1, y1 + 1, kBlack);
        fill(x0 + 1, x1 + 3, y1 + 2, y1 + 3, kBlack);
        fill(x1 + 2, x1 + 3, y0 + 1, y1 + 3, kBlack);
    }
    int width(std::string_view s) const { return e_.text_width(s); }
    int width_large(std::string_view s) const {
        int w = 0;
        for (char c : s) w += e_.font_mem[uint16_t(e_.large_font + ega::Ega::map_char(uint8_t(c)) * 36 + 32)] >> 3;
        return w;
    }
    void text(int x, int y, std::string_view s, int colour) {
        e_.fg = uint8_t(colour);
        e_.moveto(x, y);
        e_.text_small(s);
    }
    void text_centre(int cx, int y, std::string_view s, int colour) { text(cx - width(s) / 2, y, s, colour); }
    void text_large_centre(int cx, int y, std::string_view s, int colour) {
        e_.fg = uint8_t(colour);
        e_.moveto(cx - width_large(s) / 2, y);
        e_.text_large(s);
    }
    // Map picture scaled to half size: source rectangle (sx0, sy0)-(sx1, sy1) drawn at (x, y).
    void map_half(const std::vector<uint8_t>& px, int sx0, int sy0, int sx1, int sy1, int x, int y) {
        for (int sy = sy0; sy <= sy1; sy += 2) {
            int row = y + (sy - sy0) / 2;
            int run = sx0;
            while (run <= sx1) {
                uint8_t c = px[size_t(sy * ega::kWidth + run)];
                int end = run;
                while (end + 2 <= sx1 && px[size_t(sy * ega::kWidth + end + 2)] == c) end += 2;
                fill(x + (run - sx0) / 2, x + (end - sx0) / 2, row, row, c);
                run = end + 2;
            }
        }
    }

private:
    ega::Ega& e_;
};

}  // namespace

std::optional<MapStyle> run_launcher(Host& host, ega::Ega& screen, const Assets& assets, MapStyle initial) {
    Fonts fonts(assets);
    Screen s(screen, fonts);
    std::vector<uint8_t> original = assets.maps.empty() ? std::vector<uint8_t>(ega::kWidth * ega::kHeight, kGrey)
                                                        : decode_map(assets.maps[0]);
    std::vector<uint8_t> redrawn = decode_map(redrawn_maps::map(0));
    int choice = initial == MapStyle::Redrawn ? 1 : 0;

    screen.set_mode();
    s.fill(0, ega::kWidth - 1, 0, ega::kHeight - 1, kGrey);
    // Title
    s.panel(40, 12, 599, 62);
    s.text_large_centre(320, 18, "CONFLICT", kBlack);
    s.text_centre(320, 42, "The Middle East Political Simulator", kBlack);
    // Choice
    s.panel(40, 92, 259, 250);
    s.text_centre(150, 100, "Maps:", kBlack);
    s.fill(150 - s.width("Maps:") / 2, 150 + s.width("Maps:") / 2, 109, 109, kBlack);
    // Help line
    s.panel(40, 290, 599, 318);
    s.text_centre(320, 300, "Arrow keys: choose      Enter: start      Esc: quit", kBlack);
    // Preview frame
    s.panel(290, 92, 599, 250);

    for (;;) {
        const char* names[2] = {"Original", "Redrawn"};
        s.fill(48, 252, 118, 240, kWhite);
        for (int i = 0; i < 2; ++i) {
            int y = 124 + i * 28;
            s.text(70, y, names[i], kBlack);
            s.small(Fonts::kExtra);  // checkbox glyphs, as in the game's menus
            s.text(220, y, i == choice ? "23" : "01", kBlack);
            s.small(Fonts::kEtech);
        }
        const char* about[2][3] = {{"The maps as drawn", "in 1990.", ""},
                                   {"New maps with real", "coastlines, borders", "and city names."}};
        for (int l = 0; l < 3; ++l) s.text(60, 190 + l * 14, about[choice][l], kDarkBlue);
        // preview: the main map, half size, centred in its panel
        const auto& px = choice == 1 ? redrawn : original;
        s.map_half(px, 8, 7, 509, 307, 320, 96);
        s.fill(296, 593, 249, 250, kWhite);

        uint16_t key = host.get_key();
        if (key == kKeyEnter) return choice == 1 ? MapStyle::Redrawn : MapStyle::Original;
        if (key == kKeyEsc) return std::nullopt;
        if (key == kKeyUp || key == kKeyHome) choice = 0;
        if (key == kKeyDown || key == kKeyEnd) choice = 1;
    }
}

}  // namespace conflict
