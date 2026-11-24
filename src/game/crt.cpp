#include "game/crt.hpp"

#include <chrono>
#include <ctime>

#include "game/dgroup.hpp"

namespace conflict::crt {

namespace {
constexpr uint16_t kSeed = 0x7ff0;
}

void srand(uint32_t seed) { ds.put<uint32_t>(kSeed, seed); }

int rand() {
    uint32_t s = ds.get<uint32_t>(kSeed) * 0x41c64e6du + 0x3039u;
    ds.put<uint32_t>(kSeed, s);
    return int((s >> 16) & 0x7fff);
}

int random(int n) {
    if (n < 0) return 0;
    double r = double(rand());
    double v = (double(n) * r) / 32768.0 + 1.0;
    return int(v);
}

uint32_t dos_time_hsec() {
    using namespace std::chrono;
    auto now = system_clock::now();
    std::time_t t = system_clock::to_time_t(now);
    std::tm lt{};
    localtime_r(&t, &lt);
    auto cs = duration_cast<milliseconds>(now.time_since_epoch()).count() % 1000 / 10;
    // 20cd:000c: ((h*60 + m)*60 + s)*100 + hundredths, computed in 16-bit registers -> DX:AX.
    uint32_t v = ((uint32_t(lt.tm_hour) * 60 + uint32_t(lt.tm_min)) * 60 + uint32_t(lt.tm_sec)) * 100 +
                 uint32_t(cs);
    return v;
}

void longjmp_restart(int value) { throw Restart{value}; }

void itoa(int value, char* out) {
    int16_t v = int16_t(value);
    int16_t n = v < 0 ? int16_t(-v) : v;
    int len = 0;
    do {
        out[len++] = char(n % 10 + '0');
        n = int16_t(n / 10);
    } while (n > 0);
    if (v < 0) out[len++] = '-';
    out[len] = '\0';
    for (int i = 0, j = len - 1; i < j; ++i, --j) {
        char t = out[i];
        out[i] = out[j];
        out[j] = t;
    }
}

void sprintf(char* out, const char* fmt, std::initializer_list<FmtArg> args) {
    auto arg = args.begin();
    char num[8];
    for (; *fmt; ++fmt) {
        if (fmt[0] == '%' && (fmt[1] == 'd' || fmt[1] == 's') && arg != args.end()) {
            const char* text = arg->s;
            if (fmt[1] == 'd') {
                itoa(arg->i, num);
                text = num;
            }
            while (*text) *out++ = *text++;
            ++arg;
            ++fmt;
        } else {
            *out++ = *fmt;
        }
    }
    *out = '\0';
}

}  // namespace conflict::crt
