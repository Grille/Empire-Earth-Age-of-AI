// THE SETTINGS, READ IN ONE PLACE: the rows (rows.h) become struct settings g_set and, for [ai_base], struct settings_ai
// g_set_ai[3] (a value per AI difficulty), filled by settings_read when the DLL loads - each row's text from the test
// ini's [autotest] (a test run), else from its section of aimod.ini (a row of [ai_base]: the difficulty's own section
// first), else its default - and checked by its kind; a mistake is logged (ini.c: file, section, key) and the row
// keeps its default. The parts read g_set / g_set_ai instead of the file: one reading of each key, one rule for the
// test ini's overrides, and the keys of every section of aimod.ini (rows.h SETTING_SECTIONS) known, so a key that is
// none of them is told. Included by eeaimod.c after base/ini.c, before every part that reads a setting.
#include "settings/rows.h"

#define SET_FIELD_FLAG(name, a) int name;
#define SET_FIELD_INT(name, a) int name;
#define SET_FIELD_NUM(name, a) int name;
#define SET_FIELD_PCT(name, a) int name;
#define SET_FIELD_SHARE(name, a) float name;
#define SET_FIELD_CHOICE(name, a) int name;
#define SET_FIELD_STR(name, a) char name[a];
#define SET_FIELD_STR_P(name, a) char name[16][a];
#define SET_FIELD_CHILD(name, a) int name;
#define SET_FIELD_X(kind, sec, key, def, a, b) SET_FIELD_##kind(key, a)
#define SET_FIELD_S(kind, sec, key, def, a, b) SET_FIELD_##kind(sec##_##key, a)
#define SET_NONE(...)
#define SET_FIELD_D(kind, key, def, a, b) SET_FIELD_##kind(key, a)
struct settings { SETTINGS(SET_FIELD_X, SET_FIELD_S, SET_NONE) };
struct settings_ai { SETTINGS(SET_NONE, SET_NONE, SET_FIELD_D) };
internal struct settings g_set;
internal struct settings_ai g_set_ai[3];   // 0 easy, 1 medium, 2 hard
internal const char *const k_set_diff[3] = { "easy", "medium", "hard" };   // the difficulties' names
internal const char *const k_set_diff_sec[3] = { "ai_easy", NULL, "ai_hard" };   // their sections (medium: [ai_base])

#define SET_ID_X(kind, sec, key, def, a, b) SETTING_##key,
#define SET_ID_S(kind, sec, key, def, a, b) SETTING_##sec##_##key,
#define SET_ID_D(kind, key, def, a, b) SETTING_##key,
enum { SETTINGS(SET_ID_X, SET_ID_S, SET_ID_D) SETTING_N };

enum { SETK_FLAG, SETK_INT, SETK_NUM, SETK_PCT, SETK_SHARE, SETK_CHOICE, SETK_STR, SETK_STR_P, SETK_CHILD };
// name: the field's, and the key that overrides it in the test ini; choices: a CHOICE's names, '|' between them;
// ai: a row of [ai_base] (its field in struct settings_ai, a value per difficulty); a: a CHILD's master (its row's id)
struct setting_row { const char *sec, *key, *name; int kind, ai; size_t off; int def; const char *def_s, *choices; int a, b; };
#define SET_ROW_FLAG(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_FLAG, ai, offsetof(st, name), def, NULL, NULL, a, b },
#define SET_ROW_INT(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_INT, ai, offsetof(st, name), def, NULL, NULL, a, b },
#define SET_ROW_NUM(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_NUM, ai, offsetof(st, name), def, NULL, NULL, a, b },
#define SET_ROW_PCT(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_PCT, ai, offsetof(st, name), def, NULL, NULL, a, b },
#define SET_ROW_SHARE(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_SHARE, ai, offsetof(st, name), def, NULL, NULL, a, b },
#define SET_ROW_CHOICE(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_CHOICE, ai, offsetof(st, name), 0, def, a, 0, b },
#define SET_ROW_STR(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_STR, ai, offsetof(st, name), 0, def, NULL, a, b },
#define SET_ROW_STR_P(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_STR_P, ai, offsetof(st, name), 0, def, NULL, a, b },
#define SET_ROW_CHILD(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_CHILD, ai, offsetof(st, name), def, NULL, NULL, SETTING_##a, b },
#define SET_ROW_X(kind, sec, key, def, a, b) SET_ROW_##kind(sec, key, key, struct settings, 0, def, a, b)
#define SET_ROW_S(kind, sec, key, def, a, b) SET_ROW_##kind(sec, key, sec##_##key, struct settings, 0, def, a, b)
#define SET_ROW_D(kind, key, def, a, b) SET_ROW_##kind(ai_base, key, key, struct settings_ai, 1, def, a, b)
internal const struct setting_row k_settings[SETTING_N] = { SETTINGS(SET_ROW_X, SET_ROW_S, SET_ROW_D) };

#define SET_UNDER(key, master) { SETTING_##key, SETTING_##master },
internal const struct { int row, master; } k_settings_under[] = { SETTING_UNDER(SET_UNDER) };
#define SETTING_UNDER_N ((int)(sizeof k_settings_under / sizeof *k_settings_under))

#define SET_GROUP(first, title) { SETTING_##first, title },
internal const struct { int first; const char *title; } k_setting_groups[] = { SETTING_GROUPS(SET_GROUP) };
#define SETTING_GROUP_N ((int)(sizeof k_setting_groups / sizeof *k_setting_groups))

#define SET_EASY(key, text) { #key, text },
internal const struct { const char *key, *text; } k_settings_easy[] = { SETTINGS_EASY(SET_EASY) };

enum { SECK_PLAIN, SECK_ALL, SECK_DIFF, SECK_TEST };
struct setting_section { const char *sec; int order, kind; const char *title; };
#define SET_SECTION(sec, order, kind, title) { #sec, order, SECK_##kind, title },
internal const struct setting_section k_setting_sections[] = { SETTING_SECTIONS(SET_SECTION) };
#define SETTING_SECTION_N ((int)(sizeof k_setting_sections / sizeof *k_setting_sections))

// where each row's value came from: 0 its default, 1 aimod.ini, 2 the test ini ([autotest]); a row of [ai_base]: the
// furthest of its difficulties
internal uint8_t g_set_from[SETTING_N];
internal const char *settings_section(int id) {   // the section a row's value was read from (not for a row of [ai_base])
    return g_set_from[id] == 2 ? "autotest" : k_settings[id].sec;
}

// the index of v among a CHOICE's names, -1: none of them
internal int setting_choice(const char *choices, const char *v) {
    int i = 0;
    for (const char *c = choices; ; i++) {
        const char *e = strchr(c, '|');
        size_t n = e ? (size_t)(e - c) : strlen(c);
        if (strlen(v) == n && !_strnicmp(c, v, n)) return i;
        if (!e) return -1;
        c = e + 1;
    }
}
internal void setting_choice_name(const char *choices, int i, char *out, int cap) {   // a CHOICE's name i
    const char *c = choices;
    for (; i > 0 && c; i--) { c = strchr(c, '|'); if (c) c++; }
    const char *e = c ? strchr(c, '|') : NULL;
    int n = !c ? 0 : e ? (int)(e - c) : (int)strlen(c);
    lstrcpynA(out, c ? c : "", n + 1 < cap ? n + 1 : cap);
}

// row r's text: the test ini's [autotest] name, else key in r's section of aimod.ini - a row of [ai_base] for difficulty
// d: [ai_easy] / [ai_hard] (the medium AI plays [ai_base] itself), then [ai_base], then (easy) the built-in easy text; a row of section autotest: the test
// ini only. *file, *sec, *at where; 0: not set, the default
internal int setting_raw(const struct setting_row *r, int d, const char *key, const char *name, const char *ini, const char *test_ini,
                         char *out, int cap, const char **file, const char **sec, const char **at, uint8_t *from) {
    *at = key;
    if (test_ini && ini_raw("autotest", name, test_ini, out, cap)) { *file = test_ini; *sec = "autotest"; *at = name; *from = 2; return 1; }
    *file = ini;
    *from = 1;
    if (r->ai && d != 1 && ini_raw(*sec = k_set_diff_sec[d], key, ini, out, cap)) return 1;
    if (lstrcmpA(r->sec, "autotest") && ini_raw(*sec = r->sec, key, ini, out, cap)) return 1;
    *from = 0;
    for (size_t i = 0; r->ai && d == 0 && i < sizeof k_settings_easy / sizeof *k_settings_easy; i++) {
        if (lstrcmpA(k_settings_easy[i].key, key)) continue;
        lstrcpynA(out, k_settings_easy[i].text, cap);
        *file = "(built-in)";
        *sec = "ai_easy";
        return 1;
    }
    return 0;
}

// row r's default into at
internal void setting_default(const struct setting_row *r, void *at) {
    if (r->kind == SETK_STR || r->kind == SETK_STR_P) lstrcpynA((char *)at, r->def_s, r->a);
    else if (r->kind == SETK_CHOICE) *(int *)at = setting_choice(r->choices, r->def_s);
    else if (r->kind == SETK_SHARE) *(float *)at = (float)(r->def / 100.0);
    else *(int *)at = r->def;
}

// text v as row r's value into at (f, s, k: the file, section and key it was read from, for a mistake's message);
// 0: a mistake (told: ini_error), at keeps the default
internal int setting_value(const struct setting_row *r, const char *f, const char *s, const char *k, const char *v, void *at) {
    char why[96];
    setting_default(r, at);
    if (r->kind == SETK_STR || r->kind == SETK_STR_P) {
        if ((int)strlen(v) >= r->a) {
            snprintf(why, sizeof why, "at most %d characters", r->a - 1);
            ini_error(f, s, k, v, why);
            return 0;
        }
        lstrcpyA((char *)at, v);
        return 1;
    }
    if (r->kind == SETK_CHOICE) {
        int i = setting_choice(r->choices, v);
        if (i < 0) {
            snprintf(why, sizeof why, "one of %s expected", r->choices);
            ini_error(f, s, k, v, why);
            return 0;
        }
        *(int *)at = i;
        return 1;
    }
    if (r->kind == SETK_SHARE) {   // a mistake: told, the default
        *(float *)at = (float)ini_share_of(f, s, k, v, r->def / 100.0, r->a / 100.0, r->b / 100.0);
        return 1;
    }
    if (r->kind == SETK_PCT) {
        *(int *)at = ini_pct_of(f, s, k, v, r->def, r->a);
        return 1;
    }
    double x;
    int n;
    if (!ini_eval(f, s, k, v, &x) || !ini_whole(f, s, k, v, x, &n)) return 0;
    int flag = r->kind == SETK_FLAG || r->kind == SETK_CHILD, lo = flag ? 0 : r->a, hi = flag ? 1 : r->b;
    if (r->kind != SETK_NUM && (n < lo || n > hi)) {
        if (flag) lstrcpyA(why, "0 or 1 expected");
        else snprintf(why, sizeof why, "%d to %d expected", lo, hi);
        ini_error(f, s, k, v, why);
        return 0;
    }
    *(int *)at = n;
    return 1;
}

// one key of row r (difficulty d) into at (name: its test ini key); 1: read (not the default)
internal int setting_read(const struct setting_row *r, int d, const char *key, const char *name, void *at, const char *ini,
                          const char *test_ini, uint8_t *from) {
    char v[MAX_PATH + 8];
    const char *f, *s, *k;
    setting_default(r, at);
    if (!setting_raw(r, d, key, name, ini, test_ini, v, sizeof v, &f, &s, &k, from)) return 0;
    setting_value(r, f, s, k, v, at);
    return 1;
}

// key k of section sec is a row's (a row of [ai_base] in [ai_easy] / [ai_hard] too)
internal int setting_known(const char *sec, const char *k) {
    int diff = !lstrcmpiA(sec, k_set_diff_sec[0]) || !lstrcmpiA(sec, k_set_diff_sec[2]);
    for (int i = 0; i < SETTING_N; i++) {
        const struct setting_row *r = &k_settings[i];
        if (lstrcmpiA(r->sec, sec) && !(diff && r->ai)) continue;
        if (r->kind != SETK_STR_P) {
            if (!lstrcmpiA(r->key, k)) return 1;
            continue;
        }
        size_t len = strlen(r->key);   // key0 .. key15
        char *end = NULL;
        long p = !_strnicmp(k, r->key, len) && k[len] >= '0' && k[len] <= '9' ? strtol(k + len, &end, 10) : -1;
        if (p >= 0 && p < 16 && !*end) return 1;
    }
    return 0;
}

// sec is a section of aimod.ini (not the test ini's own)
internal int setting_section_known(const char *sec) {
    for (int s = 0; s < SETTING_SECTION_N; s++)
        if (k_setting_sections[s].kind != SECK_TEST && !lstrcmpiA(k_setting_sections[s].sec, sec)) return 1;
    return 0;
}

// the keys of a whole section that are no row: told
internal void settings_unknown(const char *ini, const char *sec) {
    static char keys[16384];   // the section's key names, each 0-ended, a 0 after the last
    DWORD n = GetPrivateProfileStringA(sec, NULL, "", keys, sizeof keys, ini);
    for (const char *k = keys; k < keys + n && *k; k += strlen(k) + 1)
        if (!setting_known(sec, k)) ini_unknown(ini, sec, k);
}

// a CHILD row that isn't set, in the values at base (g_set, or a difficulty's g_set_ai): its master's word - default
// (0) keeps the child's own default, enabled (1) is on, disabled (2) off; the master is read before it (an earlier row)
internal void setting_follow(const struct setting_row *r, uint8_t *base) {
    int m = *(int *)(base + k_settings[r->a].off);
    if (m) *(int *)(base + r->off) = m == 1;
}

internal void settings_read(const char *ini, const char *test_ini) {
    for (int i = 0; i < SETTING_N; i++) {
        const struct setting_row *r = &k_settings[i];
        g_set_from[i] = 0;
        if (r->ai) {
            for (int d = 0; d < 3; d++) {
                uint8_t from = 0;
                if (!setting_read(r, d, r->key, r->name, (uint8_t *)&g_set_ai[d] + r->off, ini, test_ini, &from) &&
                    r->kind == SETK_CHILD)
                    setting_follow(r, (uint8_t *)&g_set_ai[d]);
                if (from > g_set_from[i]) g_set_from[i] = from;
            }
            continue;
        }
        uint8_t *at = (uint8_t *)&g_set + r->off;
        if (r->kind != SETK_STR_P) {
            if (!setting_read(r, 0, r->key, r->name, at, ini, test_ini, &g_set_from[i]) && r->kind == SETK_CHILD)
                setting_follow(r, (uint8_t *)&g_set);
            continue;
        }
        for (int p = 0; p < 16; p++) {
            char key[48];
            snprintf(key, sizeof key, "%s%d", r->key, p);
            uint8_t from = 0;
            setting_read(r, 0, key, key, at + p * r->a, ini, test_ini, &from);
            if (from > g_set_from[i]) g_set_from[i] = from;
        }
    }
    for (int s = 0; s < SETTING_SECTION_N; s++)
        if (k_setting_sections[s].kind != SECK_TEST) settings_unknown(ini, k_setting_sections[s].sec);
    static char names[4096];   // and the sections that are none (an old [medium]: the medium AI plays [ai_base])
    DWORD n = GetPrivateProfileSectionNamesA(names, sizeof names, ini);
    for (const char *sec = names; sec < names + n && *sec; sec += strlen(sec) + 1)
        if (!setting_section_known(sec)) ini_unknown(ini, sec, NULL);
}

// row r's value at at as text (out, cap)
internal void setting_text(const struct setting_row *r, const uint8_t *at, char *out, int cap) {
    switch (r->kind) {
    case SETK_PCT: snprintf(out, cap, "%d%%", *(const int *)at); break;
    case SETK_SHARE: snprintf(out, cap, "%.4g%%", *(const float *)at * 100.0); break;
    case SETK_CHOICE: setting_choice_name(r->choices, *(const int *)at, out, cap); break;
    case SETK_STR:   // a key for a server stays out of the log
        lstrcpynA(out, !lstrcmpA(r->key, "api_key") && *at ? "(set)" : (const char *)at, cap);
        break;
    default: snprintf(out, cap, "%d", *(const int *)at);
    }
}

// the values in force, a line per section, the test ini's own marked * (the log opens after settings_read); [ai_base] as
// the medium AI has it, [ai_easy] and [ai_hard] where theirs differ
internal void settings_log(FILE *f) {
    const char *sec = NULL;
    char v[MAX_PATH + 8];
    for (int i = 0; i < SETTING_N; i++) {
        const struct setting_row *r = &k_settings[i];
        const uint8_t *at = r->ai ? (const uint8_t *)&g_set_ai[1] + r->off : (const uint8_t *)&g_set + r->off;
        if (!sec || lstrcmpA(sec, r->sec)) {
            fprintf(f, "%s# settings [%s]:", sec ? "\n" : "", r->sec);
            sec = r->sec;
        }
        const char *mark = g_set_from[i] == 2 ? "*" : "";
        if (r->kind != SETK_STR_P) {
            setting_text(r, at, v, sizeof v);
            fprintf(f, " %s%s=%s", mark, r->key, v);
            continue;
        }
        for (int p = 0; p < 16; p++)
            if (at[p * r->a]) fprintf(f, " %s%s%d=%s", mark, r->key, p, (const char *)(at + p * r->a));
    }
    if (sec) fputc('\n', f);
    for (int d = 0; d < 3; d += 2) {
        fprintf(f, "# settings [%s] (where not as the base):", k_set_diff_sec[d]);
        for (int i = 0; i < SETTING_N; i++) {
            const struct setting_row *r = &k_settings[i];
            if (!r->ai) continue;
            const uint8_t *at = (const uint8_t *)&g_set_ai[d] + r->off, *med = (const uint8_t *)&g_set_ai[1] + r->off;
            if (!memcmp(at, med, 4)) continue;
            setting_text(r, at, v, sizeof v);
            fprintf(f, " %s=%s", r->key, v);
        }
        fputc('\n', f);
    }
}
