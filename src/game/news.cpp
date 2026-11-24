#include "game/news.hpp"

#include <cstring>

#include "game/dgroup.hpp"
#include "game/fixes.hpp"
#include "game/globals.hpp"

namespace conflict::news {

namespace {
// DS:91a0 (output length), DS:91a2 (output buffer), DS:91a4 (template cursor). The buffer is on
// the caller's stack in the original, so these are C++ state here (they are never saved).
int g_len;
char* g_out;
const char* g_in;

// 1000:6e57 — if the cursor is on `code`, append `word` and step over the code.
void substitute(const char* word, char code) {
    if (*g_in == code) {
        std::strcat(g_out, word);
        g_len += int(std::strlen(word));
        ++g_in;
    }
}

// Copy the formatted text into a log buffer: text, terminator, then the next free index.
void append(uint16_t buffer, Var<int16_t> used, Arr<uint16_t, 25> ptrs, Var<int16_t> count,
            const char* text) {
    ptrs[count] = uint16_t(used + buffer);
    char* log = ds.str(buffer);
    int k = 0;
    do {
        log[uint16_t(used.get())] = text[k];
        ++k;
        ++used;
    } while (text[k] != '\0');
    log[uint16_t(used.get())] = '\0';
    ++used;
    ++count;
}
}  // namespace

void format(const char* tmpl, int a, int b, char* out) {
    g_len = 0;
    g_out = out;
    g_in = tmpl;
    // Templates live in DGROUP; a template ending in a placeholder makes the original read on
    // into the following string. Using the DGROUP copy reproduces that.
    while (*g_in != '\0') {
        substitute(ds.str(g::nation_adjective[a]), '*');
        substitute(ds.str(g::nation_name[a]), '#');
        substitute(ds.str(g::nation_capital[a]), '+');
        substitute(ds.str(g::nation_adjective[b]), '@');
        substitute(ds.str(g::nation_name[b]), '_');
        substitute(ds.str(0x29fd), '%');  // "losses"
        substitute(ds.str(0x2a04), '^');  // "ion"
        substitute(ds.str(0x2a08), '~');  // "and"
        substitute(ds.str(0x2a0c), '}');  // "troops"
        substitute(ds.str(0x2a13), '{');  // "between"
        substitute(ds.str(g::nation_capital[b]), '&');
        g_out[g_len++] = *g_in++;
    }
    g_out[g_len] = '\0';
}

void add_headline(const char* tmpl, int a, int b) {
    char buf[100] = {};
    format(tmpl, a, b, buf);
    append(g::kReportText, g::report_text_used, g::headline_ptr, g::headline_count, buf);
}

void add_news(const char* tmpl, int a, int b) {
    char buf[100] = {};
    format(tmpl, a, b, buf);
    append(g::kNewsText, g::news_text_used, g::news_ptr, g::news_count, buf);
}

int str_find4(const char* s, const char* pat) {
    if (fixes::fix(14)) {  // bug #14: try every starting position
        for (; *s; ++s)
            if (s[0] == pat[0] && s[1] == pat[1] && s[2] == pat[2] && s[3] == pat[3]) return 1;
        return 0;
    }
    const char p0 = pat[0], p1 = pat[1], p2 = pat[2], p3 = pat[3];
    for (;;) {
        if (*s == '\0') return 0;
        if (*s++ != p0) continue;
        if (*s++ != p1) continue;
        if (*s++ != p2) continue;
        if (*s != p3) continue;
        return 1;
    }
}

}  // namespace conflict::news
