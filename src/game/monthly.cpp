#include "game/monthly.hpp"

#include <cstring>

#include "game/ai.hpp"
#include "game/crt.hpp"
#include "game/globals.hpp"
#include "game/news.hpp"

namespace conflict::monthly {

using namespace g;
using crt::random;

namespace {
void headline(uint16_t tmpl, int a, int b) { news::add_headline(ds.str(tmpl), a, b); }
void news_item(uint16_t tmpl, int a, int b) { news::add_news(ds.str(tmpl), a, b); }
}  // namespace

// 1000:56c7
void end_of_month() {
    int deliveries = 0;
    if (sb264 > 0x32) ai::event_news(2);
    for (int i = 0; i < 7; ++i)
        if (turn_count > 2 && arms_order(i, turn_count - 1) != 0) ++deliveries;
    if (deliveries != 0 && sb264 < 0x32) ai::event_news(1);
    if (sb262 < 0x98 && ((turn_count - 5) % 12 == 0 || turn_count == 5)) ai::event_news(3);
    int count = sb262 < 0x9b ? 2 : 1;
    for (int k = 0; k < count; ++k) {
        int a, b;
        do {
            do {
                a = random(7);
                b = random(7) - 1;
            } while (nation_status[a] == 10);
        } while (nation_status[b] == 10 || a <= b);
        int old = relations(a, b);
        if (old == 2) relations(a, b) = 1;
        if (old == 5) relations(a, b) = (rival[a] == rival[b] || at_war(a, rival[b]) != 0) ? 6 : 4;
        if (nation_status[a] == 9) ai::collapse(a);
        if (nation_status[b] == 9) ai::collapse(b);
        if (nation_status[a] == 10 || nation_status[b] == 10) return;
        if (old == 6) relations(a, b) = 7;
        if (old != relations(a, b)) drift_news(a, b);
    }
}

// 1d93:0468
void drift_news(int a, int b) {
    int k = random(6) - 1;
    uint16_t big = 0;
    int r = b < a ? relations(a, b) : relations(b, a);
    if (r == 1) big = word_at(uint16_t(0x64ac + 2 * k));
    if (r == 4 && a != Israel) {
        big = word_at(uint16_t(0x63c4 + 2 * k));
        if (border(a, b) == 1 && rival[a] == b) big = word_at(0x63d0);
        if (border(a, b) == 1 && sa11e[b] + 2 < sa11e[a]) big = word_at(0x63d2);
    }
    if (r == 7 && border(a, b) == 1 && a != Israel) big = word_at(uint16_t(0x6588 + 2 * k));
    if (big != 0) headline(big, a, b);
}

// 1000:3036 — for each nation, collect this month's news items that mention it into the digest
// buffer; sa1e2[n] points at its part (or at an item matching DS:19fe, if any).
void build_digest() {
    char* digest = ds.str(kDigestText);
    if (news_count != 0) std::memset(digest, 0, 0x9c4);
    int pos = 0;
    for (int n = 0; n < 8; ++n) {
        sa1e2[n] = uint16_t(pos + kDigestText);
        for (int i = 0; i < news_count; ++i) {
            if (!news::str_find4(ds.str(news_ptr[i]), ds.str(nation_name[n]))) continue;
            const char* p = ds.str(news_ptr[i]);
            if (news::str_find4(p, ds.str(0x19fe))) sa1e2[n] = uint16_t(pos + kDigestText);  // "vict"
            while (*p != '\0') digest[uint16_t(pos++)] = *p++;
            digest[uint16_t(pos++)] = ' ';
        }
        digest[uint16_t(pos++)] = '\0';
    }
}

// 1000:9fd9 — ordered weapons arrive in Israel's home forces: weapons 7..13 one month after the
// order, weapons 0..6 too while sb264 < 30, and everything two months after.
void deliver_arms() {
    for (int w = 0; w < 19; ++w) {
        int type = weapon_force_type[w];
        if ((turn_count > 1 && w < 14 && w > 6) || (w < 7 && sb264 < 0x1e)) {
            forces(Israel, Israel, type) += arms_order(w, turn_count - 1).get();
            arms_order(w, turn_count - 1) = 0;
        }
        if (turn_count > 2) {
            forces(Israel, Israel, type) += arms_order(w, turn_count - 2).get();
            arms_order(w, turn_count - 2) = 0;
        }
    }
}

// 1000:5a52 — keep 200 tanks per brigade posted on each peaceful border.
void garrison_borders() {
    for (int n = 0; n < 7; ++n) {
        if (at_war(Israel, n) != 0) continue;
        int need = border_brigades[n] * 200 - forces(Israel, n, 0);
        if (need < forces(Israel, Israel, 0)) {
            forces(Israel, Israel, 0) -= int16_t(need);
            forces(Israel, n, 0) += int16_t(need);
        }
    }
}

// 1000:74e1
void covert_operations() {
    for (int n = 0; n < 7; ++n) {
        uint16_t big = 0, small = 0;
        if (covert_action[n] == 1) {  // assassination
            int v = random(5) + nation_status[n] + covert_attempts[n] * -5;
            if (v > 10) {
                nation_status[n] = 10;
                ai::collapse(n);
                big = word_at(0x2fe6);
                small = word_at(0x3138);
            }
            if (v > 6 && v < 11) {
                ++nation_status[n];
                if (nation_status[n] == 10) ai::collapse(n);
                big = word_at(0x2fe8);
                small = word_at(0x313a);
            }
            if (v < 7) {
                sb276 += 5;
                sb264 -= 0xf;
                big = word_at(0x2fea);
                small = word_at(0x313c);
            }
            ++covert_attempts[n];
            if (big != 0) headline(big, n, Israel);
            if (small != 0) news_item(small, n, Israel);
        }
        if (covert_action[n] == 2) {  // coup
            treasury[Israel] -= 0x32;
            int v = (sa11e[n] + nation_status[n] - 14) - forces(n, n, 0) / 200;
            if (v < 1) {
                if (v > -5) {
                    big = word_at(0x2fee);
                    small = word_at(0x3140);
                    sa11e[n] = int16_t(sa11e[n] / 2);
                }
            } else {
                big = word_at(0x2fec);
                small = word_at(0x313e);
                nation_status[n] = 10;
                ai::collapse(n);
            }
            if (v < -5) {
                big = word_at(0x2ff0);
                small = word_at(0x3142);
                sa11e[n] = 0;
            }
            if (big != 0) headline(big, n, Israel);
            if (small != 0) news_item(small, n, Israel);
        }
    }
}

}  // namespace conflict::monthly
