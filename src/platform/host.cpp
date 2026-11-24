#include "platform/host.hpp"

#include <SDL3/SDL.h>

#include <array>
#include <cctype>
#include <cstdlib>
#include <cstdio>

namespace conflict {

namespace {

// IBM PC scan codes (set 1) for letters and digits.
constexpr std::array<uint8_t, 26> kLetterScan = {
    0x1e, 0x30, 0x2e, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, 0x24, 0x25, 0x26, 0x32,
    0x31, 0x18, 0x19, 0x10, 0x13, 0x1f, 0x14, 0x16, 0x2f, 0x11, 0x2d, 0x15, 0x2c};
constexpr std::array<uint8_t, 10> kDigitScan = {0x0b, 0x02, 0x03, 0x04, 0x05,
                                                0x06, 0x07, 0x08, 0x09, 0x0a};

// Translate an SDL key event into the 16-bit code BIOS int 16h would return (0 = ignore).
uint16_t bios_code(const SDL_KeyboardEvent& k) {
    bool shift = k.mod & SDL_KMOD_SHIFT;
    SDL_Keycode key = k.key;
    if (key >= SDLK_A && key <= SDLK_Z) {
        int i = int(key - SDLK_A);
        uint8_t ascii = uint8_t((shift ? 'A' : 'a') + i);
        return uint16_t(kLetterScan[i] << 8 | ascii);
    }
    if (key >= SDLK_0 && key <= SDLK_9 && !shift) {
        int i = int(key - SDLK_0);
        return uint16_t(kDigitScan[i] << 8 | ('0' + i));
    }
    switch (key) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER: return 0x1c0d;
        case SDLK_ESCAPE: return 0x011b;
        case SDLK_SPACE: return 0x3920;
        case SDLK_BACKSPACE: return 0x0e08;
        case SDLK_TAB: return 0x0f09;
        case SDLK_UP: case SDLK_KP_8: return 0x4800;
        case SDLK_DOWN: case SDLK_KP_2: return 0x5000;
        case SDLK_LEFT: case SDLK_KP_4: return 0x4b00;
        case SDLK_RIGHT: case SDLK_KP_6: return 0x4d00;
        case SDLK_HOME: case SDLK_KP_7: return 0x4700;
        case SDLK_END: case SDLK_KP_1: return 0x4f00;
        case SDLK_PAGEUP: case SDLK_KP_9: return 0x4900;
        case SDLK_PAGEDOWN: case SDLK_KP_3: return 0x5100;
        default: break;
    }
    if (key >= SDLK_F1 && key <= SDLK_F10) return uint16_t((0x3b + (key - SDLK_F1)) << 8);
    return 0;
}

void front_to_argb(const ega::Ega& ega, uint32_t* dst, int pitch_px) {
    static std::array<uint8_t, ega::kWidth * ega::kHeight> idx;
    ega.copy_front(idx);
    for (int y = 0; y < ega::kHeight; ++y)
        for (int x = 0; x < ega::kWidth; ++x)
            dst[y * pitch_px + x] = 0xff000000u | ega::kPalette[idx[y * ega::kWidth + x]];
}

}  // namespace

Host::~Host() {
    if (texture_) SDL_DestroyTexture(texture_);
    if (renderer_) SDL_DestroyRenderer(renderer_);
    if (window_) SDL_DestroyWindow(window_);
    if (window_) SDL_Quit();
}

bool Host::open(const char* title, int scale) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL_Init: %s\n", SDL_GetError());
        return false;
    }
    // EGA's 350 lines filled a 4:3 monitor: present 640x350 in a 640x480 logical area.
    if (!SDL_CreateWindowAndRenderer(title, 640 * scale, 480 * scale, SDL_WINDOW_RESIZABLE,
                                     &window_, &renderer_)) {
        std::fprintf(stderr, "SDL_CreateWindowAndRenderer: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetRenderVSync(renderer_, 1);
    SDL_SetRenderLogicalPresentation(renderer_, 640, 480, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                 ega::kWidth, ega::kHeight);
    if (!texture_) {
        std::fprintf(stderr, "SDL_CreateTexture: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetTextureScaleMode(texture_, SDL_SCALEMODE_PIXELART);
    return true;
}

void Host::toggle_fullscreen() {
    if (!window_) return;
    bool full = (SDL_GetWindowFlags(window_) & SDL_WINDOW_FULLSCREEN) != 0;
    // Borderless desktop fullscreen; the 4:3 letterboxing keeps the picture's proportions.
    if (!SDL_SetWindowFullscreen(window_, !full))
        std::fprintf(stderr, "SDL_SetWindowFullscreen: %s\n", SDL_GetError());
    present();
}

void Host::present() {
    if (headless_ || !renderer_) return;
    void* pixels;
    int pitch;
    if (SDL_LockTexture(texture_, nullptr, &pixels, &pitch)) {
        front_to_argb(ega_, static_cast<uint32_t*>(pixels), pitch / 4);
        SDL_UnlockTexture(texture_);
    }
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderTexture(renderer_, texture_, nullptr, nullptr);
    SDL_RenderPresent(renderer_);
    last_present_ = SDL_GetTicks();
}

void Host::pump(int wait_ms) {
    if (headless_) return;
    SDL_Event ev;
    bool got = wait_ms > 0 ? SDL_WaitEventTimeout(&ev, wait_ms) : SDL_PollEvent(&ev);
    while (got) {
        if (ev.type == SDL_EVENT_QUIT) throw QuitRequested{};
        if (ev.type == SDL_EVENT_KEY_DOWN) {
            if (ev.key.key == SDLK_F11) {  // F11: fullscreen on/off; not passed to the game
                if (!ev.key.repeat) toggle_fullscreen();
            } else if (uint16_t code = bios_code(ev.key)) {
                push_key(code);
            }
        }
        got = SDL_PollEvent(&ev);
    }
}

void Host::idle() {
    if (headless_) return;
    pump(0);
    if (SDL_GetTicks() - last_present_ >= 16) present();
}

void Host::sleep_ms(int ms) {
    if (headless_) return;
    uint64_t end = SDL_GetTicks() + uint64_t(ms);
    present();
    while (SDL_GetTicks() < end) pump(int(end - SDL_GetTicks()));
}

void Host::snapshot() {
    if (!scripted_ || shot_dir_.empty()) return;
    char name[32];
    std::snprintf(name, sizeof name, "wait_%03d.bmp", shot_count_++);
    save_screenshot(ega_, shot_dir_ / name);
}

uint16_t Host::get_key() {
    if (scripted_) {
        snapshot();
        if (!keys_.empty()) {
            uint16_t k = keys_.front();
            keys_.pop_front();
            return k;
        }
        if (script_pos_ >= script_.size()) throw QuitRequested{};
        present();
        return script_[script_pos_++];
    }
    present();
    while (keys_.empty()) {
        pump(100);
        present();
    }
    uint16_t k = keys_.front();
    keys_.pop_front();
    return k;
}

bool Host::take_any_key() {
    idle();
    if (keys_.empty()) return false;
    keys_.pop_front();
    return true;
}

int Host::kbd_poll_ext() {
    idle();
    if (keys_.empty()) return 0;
    uint16_t k = keys_.front();
    keys_.pop_front();
    if (k & 0xff) return 0;
    if (fix_key_poll) return k >> 8;  // bug #7
    return get_key() >> 8;
}

bool parse_key_script(const std::string& text, std::vector<uint16_t>& out, std::string& error) {
    size_t i = 0;
    while (i <= text.size()) {
        size_t j = text.find(',', i);
        if (j == std::string::npos) j = text.size();
        std::string k = text.substr(i, j - i);
        i = j + 1;
        if (k.empty()) continue;
        static const struct { const char* name; uint16_t code; } kNamed[] = {
            {"enter", 0x1c0d}, {"esc", 0x011b}, {"space", 0x3920}, {"up", 0x4800},
            {"down", 0x5000},  {"left", 0x4b00}, {"right", 0x4d00}, {"home", 0x4700},
            {"end", 0x4f00},   {"pgup", 0x4900}, {"pgdn", 0x5100}};
        bool found = false;
        for (auto& n : kNamed)
            if (k == n.name) { out.push_back(n.code); found = true; }
        if (found) continue;
        if (k.size() >= 2 && k[0] == 'f' && std::isdigit(uint8_t(k[1]))) {
            int n = std::atoi(k.c_str() + 1);
            if (n >= 1 && n <= 10) { out.push_back(uint16_t((0x3a + n) << 8)); continue; }
        }
        bool shift = k.size() == 3 && k[0] == 'S' && k[1] == '-';
        char c = shift ? k[2] : (k.size() == 1 ? k[0] : 0);
        if (c >= 'a' && c <= 'z') {
            out.push_back(uint16_t(kLetterScan[c - 'a'] << 8 | (shift ? c - 32 : c)));
            continue;
        }
        if (c >= '0' && c <= '9' && !shift) {
            out.push_back(uint16_t(kDigitScan[c - '0'] << 8 | c));
            continue;
        }
        error = "unknown key in script: " + k;
        return false;
    }
    return true;
}

bool save_screenshot(const ega::Ega& ega, const std::filesystem::path& path) {
    SDL_Surface* s = SDL_CreateSurface(ega::kWidth, ega::kHeight, SDL_PIXELFORMAT_ARGB8888);
    if (!s) return false;
    front_to_argb(ega, static_cast<uint32_t*>(s->pixels), s->pitch / 4);
    bool ok = SDL_SaveBMP(s, path.string().c_str());
    SDL_DestroySurface(s);
    return ok;
}

}  // namespace conflict
