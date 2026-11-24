#include "platform/dosfile.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>

namespace conflict::dos {

namespace {
std::filesystem::path g_dir = ".";

bool iequals(std::string_view a, std::string_view b) {
    return a.size() == b.size() &&
           std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
               return std::tolower(uint8_t(x)) == std::tolower(uint8_t(y));
           });
}
}  // namespace

std::string path_utf8(const std::filesystem::path& p) {
    auto u = p.u8string();
    return std::string(u.begin(), u.end());
}

std::filesystem::path utf8_path(std::string_view text) {
    return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

void set_game_dir(std::filesystem::path dir) { g_dir = std::move(dir); }
const std::filesystem::path& game_dir() { return g_dir; }

std::optional<std::filesystem::path> resolve(std::string_view name) {
    std::error_code ec;
    auto exact = g_dir / std::filesystem::path(name);
    if (std::filesystem::is_regular_file(exact, ec)) return exact;
    for (const auto& e : std::filesystem::directory_iterator(g_dir, ec)) {
        if (e.is_regular_file(ec) && iequals(path_utf8(e.path().filename()), name)) return e.path();
    }
    return std::nullopt;
}

std::optional<std::vector<uint8_t>> read_binary(std::string_view name) {
    auto p = resolve(name);
    if (!p) return std::nullopt;
    std::ifstream f(*p, std::ios::binary);
    if (!f) return std::nullopt;
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(f), {});
}

int TextReader::getc() {
    while (!eof_ && pos_ < data_.size()) {
        uint8_t c = data_[pos_++];
        if (c == '\r') continue;
        if (c == 0x1a) break;
        return c;
    }
    eof_ = true;
    return -1;
}

std::optional<TextReader> open_text(std::string_view name) {
    auto data = read_binary(name);
    if (!data) return std::nullopt;
    return TextReader(std::move(*data));
}

}  // namespace conflict::dos
