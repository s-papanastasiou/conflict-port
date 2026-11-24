// SDL3 host: window, presentation of the EGA page, and BIOS-style keyboard input.
//
// The game logic is written in the original's blocking style (it waits inside get_key()), so the
// host runs on the same thread: every input call pumps SDL events and presents the screen.
#pragma once

#include <cstdint>
#include <deque>
#include <filesystem>
#include <string>
#include <vector>

#include "platform/ega.hpp"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

namespace conflict {

// Thrown out of input calls when the window is closed, so the game loop can unwind.
struct QuitRequested {};

class Host {
public:
    explicit Host(ega::Ega& ega) : ega_(ega) {}
    ~Host();
    Host(const Host&) = delete;
    Host& operator=(const Host&) = delete;

    bool open(const char* title, int scale);
    // F11 switches between window and fullscreen at any time.
    void toggle_fullscreen();
    void present();

    // int86(0x16) AH=0 — wait for a key, return scan << 8 | ascii (1000:0444 get_key).
    uint16_t get_key();
    // 1f39:0a58 — if no key is waiting return 0. An ASCII key is consumed and gives 0. An
    // extended key is consumed, then the driver blocks for a *second* key and returns its scan
    // code (original behaviour, kept as-is).
    int kbd_poll_ext();
    // Fix for original bug #7: return the first extended key's scan code without waiting.
    bool fix_key_poll = false;
    // Port addition: consume any pending key and report whether there was one (used to let any
    // key, including Enter, skip the intro and title pauses).
    bool take_any_key();
    // Pump events and present without waiting; for busy loops in the original.
    void idle();
    // Delay in milliseconds while staying responsive.
    void sleep_ms(int ms);

    // Testing: feed keys only when the game blocks in get_key(), save a screenshot at each such
    // wait, and end (QuitRequested) when the script runs out. Keys are BIOS codes; see
    // parse_key_script() for the names accepted.
    void set_script(std::vector<uint16_t> keys) { script_ = std::move(keys); scripted_ = true; }
    void set_shot_dir(std::filesystem::path dir) { shot_dir_ = std::move(dir); }
    // Headless: no window at all (requires a script).
    bool open_headless() { headless_ = true; return true; }
    bool headless() const { return headless_; }
    // Save a screenshot into the shot directory (scripted runs only).
    void snapshot();

private:
    void pump(int wait_ms);
    void push_key(uint16_t code) { keys_.push_back(code); }

    ega::Ega& ega_;
    SDL_Window* window_ = nullptr;
    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    std::deque<uint16_t> keys_;
    uint64_t last_present_ = 0;
    std::vector<uint16_t> script_;
    size_t script_pos_ = 0;
    bool scripted_ = false;
    bool headless_ = false;
    std::filesystem::path shot_dir_;
    int shot_count_ = 0;
};

// "enter,down,down,S-e,esc" -> BIOS codes. Names: enter esc space up down left right home end
// pgup pgdn f1..f10, single letters/digits, S-<letter> for shifted letters.
bool parse_key_script(const std::string& text, std::vector<uint16_t>& out, std::string& error);

// Write the visible EGA page to a BMP file (works without opening a window).
bool save_screenshot(const ega::Ega& ega, const std::filesystem::path& path);

// Shows an error in a message box (for a program started without a terminal, as on Windows).
void show_error_box(const std::string& message);

// Directory containing the running program.
std::filesystem::path program_dir();

}  // namespace conflict
