// File access with DOS semantics: case-insensitive names relative to the game directory, and
// Zortech text-mode reading where the original used it.
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace conflict::dos {

// Paths as UTF-8 text and back (on Windows path::string() uses the ANSI code page instead).
std::string path_utf8(const std::filesystem::path& p);
std::filesystem::path utf8_path(std::string_view text);

// Directory holding the original game files. Set once at startup.
void set_game_dir(std::filesystem::path dir);
const std::filesystem::path& game_dir();

// Resolve a DOS file name (any case) inside the game directory.
std::optional<std::filesystem::path> resolve(std::string_view name);

// Whole file in binary mode.
std::optional<std::vector<uint8_t>> read_binary(std::string_view name);

// Emulates fopen(name, "r") + repeated getc() (Zortech 21ae:0059): CR bytes are skipped and
// 0x1A ends the file. After end of file every read yields EOF (-1), which callers store as 0xFF.
class TextReader {
public:
    explicit TextReader(std::vector<uint8_t> data) : data_(std::move(data)) {}
    int getc();

private:
    std::vector<uint8_t> data_;
    size_t pos_ = 0;
    bool eof_ = false;
};
std::optional<TextReader> open_text(std::string_view name);

}  // namespace conflict::dos
