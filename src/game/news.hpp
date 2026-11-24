// Headline/news text formatting and the two text logs (code segment 1000).
#pragma once

namespace conflict::news {

// 1000:6d35 — expand a template into out. Placeholders: '*' adjective, '#' name, '+' capital of
// nation a; '@' adjective, '_' name, '&' capital of nation b; '%' "losses", '^' "ion",
// '~' "and", '}' "troops", '{' "between".
void format(const char* tmpl, int a, int b, char* out);

void add_headline(const char* tmpl, int a, int b);  // 1000:6c39 -> report text log
void add_news(const char* tmpl, int a, int b);      // 1000:6cb7 -> this month's news log

// 1000:0357 — 1 if the first 4 characters of pat occur in s. After a partial match the search
// resumes at the mismatching character (original behaviour).
int str_find4(const char* s, const char* pat);

}  // namespace conflict::news
