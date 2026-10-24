// THE SETTINGS, READ IN ONE PLACE: the rows (rows.h) become struct settings g_set and, for [all], struct settings_ai
// g_set_ai[3] (a value per AI difficulty), filled by settings_read when the DLL loads - each row's text from the test
// ini's [autotest] (a test run), else from its section of aimod.ini (a row of [all]: the difficulty's own section
// first), else its default - and checked by its kind; a mistake is logged (ini.c: file, section, key) and the row
// keeps its default. The parts read g_set / g_set_ai instead of the file: one reading of each key, one rule for the
// test ini's overrides, and the keys of a whole section (k_settings_whole) known, so a key that is none of them is
// told. Included by eeaimod.c after base/ini.c, before every part that reads a setting.
#include "settings/rows.h"

#define SET_FIELD_FLAG(name, a) int name;
#define SET_FIELD_INT(name, a) int name;
#define SET_FIELD_NUM(name, a) int name;
#define SET_FIELD_PCT(name, a) int name;
#define SET_FIELD_SHARE(name, a) float name;
#define SET_FIELD_CHOICE(name, a) int name;
#define SET_FIELD_STR(name, a) char name[a];
#define SET_FIELD_STR_P(name, a) char name[16][a];
#define SET_FIELD_X(kind, sec, key, def, a, b) SET_FIELD_##kind(key, a)
#define SET_FIELD_S(kind, sec, key, def, a, b) SET_FIELD_##kind(sec##_##key, a)
#define SET_NONE(...)
#define SET_FIELD_D(kind, key, def, a, b) SET_FIELD_##kind(key, a)
struct settings { SETTINGS(SET_FIELD_X, SET_FIELD_S, SET_NONE) };
struct settings_ai { SETTINGS(SET_NONE, SET_NONE, SET_FIELD_D) };
internal struct settings g_set;
internal struct settings_ai g_set_ai[3];   // 0 easy, 1 medium, 2 hard
internal const char *const k_set_diff[3] = { "easy", "medium", "hard" };

#define SET_ID_X(kind, sec, key, def, a, b) SETTING_##key,
#define SET_ID_S(kind, sec, key, def, a, b) SETTING_##sec##_##key,
#define SET_ID_D(kind, key, def, a, b) SETTING_##key,
enum { SETTINGS(SET_ID_X, SET_ID_S, SET_ID_D) SETTING_N };

enum { SETK_FLAG, SETK_INT, SETK_NUM, SETK_PCT, SETK_SHARE, SETK_CHOICE, SETK_STR, SETK_STR_P };
// name: the field's, and the key that overrides it in the test ini; choices: a CHOICE's names, '|' between them;
// ai: a row of [all] (its field in struct settings_ai, a value per difficulty)
struct setting_row { const char *sec, *key, *name; int kind, ai; size_t off; int def; const char *def_s, *choices; int a, b; };
#define SET_ROW_FLAG(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_FLAG, ai, offsetof(st, name), def, NULL, NULL, a, b },
#define SET_ROW_INT(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_INT, ai, offsetof(st, name), def, NULL, NULL, a, b },
#define SET_ROW_NUM(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_NUM, ai, offsetof(st, name), def, NULL, NULL, a, b },
#define SET_ROW_PCT(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_PCT, ai, offsetof(st, name), def, NULL, NULL, a, b },
#define SET_ROW_SHARE(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_SHARE, ai, offsetof(st, name), def, NULL, NULL, a, b },
#define SET_ROW_CHOICE(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_CHOICE, ai, offsetof(st, name), 0, def, a, 0, b },
#define SET_ROW_STR(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_STR, ai, offsetof(st, name), 0, def, NULL, a, b },
#define SET_ROW_STR_P(sec, key, name, st, ai, def, a, b) { #sec, #key, #name, SETK_STR_P, ai, offsetof(st, name), 0, def, NULL, a, b },
#define SET_ROW_X(kind, sec, key, def, a, b) SET_ROW_##kind(sec, key, key, struct settings, 0, def, a, b)
#define SET_ROW_S(kind, sec, key, def, a, b) SET_ROW_##kind(sec, key, sec##_##key, struct settings, 0, def, a, b)
#define SET_ROW_D(kind, key, def, a, b) SET_ROW_##kind(all, key, key, struct settings_ai, 1, def, a, b)
internal const struct setting_row k_settings[SETTING_N] = { SETTINGS(SET_ROW_X, SET_ROW_S, SET_ROW_D) };

#define SET_EASY(key, text) { #key, text },
internal const struct { const char *key, *text; } k_settings_easy[] = { SETTINGS_EASY(SET_EASY) };

// where each row's value came from: 0 its default, 1 aimod.ini, 2 the test ini ([autotest]); a row of [all]: the
// furthest of its difficulties
internal uint8_t g_set_from[SETTING_N];
internal const char *settings_section(int id) {   // the section a row's value was read from (not for a row of [all])
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

// row r's text: the test ini's [autotest] name, else key in r's section of aimod.ini - a row of [all] for difficulty
// d: [easy] / [medium] / [hard], then [all], then (easy) the built-in easy text; a row of section autotest: the test
// ini only. *file, *sec, *at where; 0: not set, the default
internal int setting_raw(const struct setting_row *r, int d, const char *key, const char *name, const char *ini, const char *test_ini,
                         char *out, int cap, const char **file, const char **sec, const char **at, uint8_t *from) {
    *at = key;
    if (test_ini && ini_raw("autotest", name, test_ini, out, cap)) { *file = test_ini; *sec = "autotest"; *at = name; *from = 2; return 1; }
    *file = ini;
    *from = 1;
    if (r->ai && ini_raw(*sec = k_set_diff[d], key, ini, out, cap)) return 1;
    if (lstrcmpA(r->sec, "autotest") && ini_raw(*sec = r->sec, key, ini, out, cap)) return 1;
    *from = 0;
    for (size_t i = 0; r->ai && d == 0 && i < sizeof k_settings_easy / sizeof *k_settings_easy; i++) {
        if (lstrcmpA(k_settings_easy[i].key, key)) continue;
        lstrcpynA(out, k_settings_easy[i].text, cap);
        *file = "(built-in)";
        *sec = "easy";
        return 1;
    }
    return 0;
}

// one key of row r (difficulty d) into at (name: its test ini key); 1: read (not the default)
internal int setting_read(const struct setting_row *r, int d, const char *key, const char *name, void *at, const char *ini,
                          const char *test_ini, uint8_t *from) {
    char v[MAX_PATH + 8], why[96];
    const char *f, *s, *k;
    if (r->kind == SETK_STR || r->kind == SETK_STR_P) {
        lstrcpynA((char *)at, r->def_s, r->a);
        if (!setting_raw(r, d, key, name, ini, test_ini, v, sizeof v, &f, &s, &k, from)) return 0;
        if ((int)strlen(v) >= r->a) {
            snprintf(why, sizeof why, "at most %d characters", r->a - 1);
            ini_error(f, s, k, v, why);
            return 0;
        }
        lstrcpyA((char *)at, v);
        return 1;
    }
    if (r->kind == SETK_CHOICE) {
        *(int *)at = setting_choice(r->choices, r->def_s);
        if (!setting_raw(r, d, key, name, ini, test_ini, v, sizeof v, &f, &s, &k, from)) return 0;
        int i = setting_choice(r->choices, v);
        if (i < 0) {
            snprintf(why, sizeof why, "one of %s expected", r->choices);
            ini_error(f, s, k, v, why);
            return 0;
        }
        *(int *)at = i;
        return 1;
    }
    if (r->kind == SETK_SHARE) {
        double def = r->def / 100.0;
        *(float *)at = (float)def;
        if (!setting_raw(r, d, key, name, ini, test_ini, v, sizeof v, &f, &s, &k, from)) return 0;
        *(float *)at = (float)ini_share_of(f, s, k, v, def, r->a / 100.0, r->b / 100.0);
        return 1;
    }
    *(int *)at = r->def;
    if (!setting_raw(r, d, key, name, ini, test_ini, v, sizeof v, &f, &s, &k, from)) return 0;
    if (r->kind == SETK_PCT) {
        *(int *)at = ini_pct_of(f, s, k, v, r->def, r->a);
        return 1;
    }
    double x;
    int n;
    if (!ini_eval(f, s, k, v, &x) || !ini_whole(f, s, k, v, x, &n)) return 0;
    int lo = r->kind == SETK_FLAG ? 0 : r->a, hi = r->kind == SETK_FLAG ? 1 : r->b;
    if (r->kind != SETK_NUM && (n < lo || n > hi)) {
        if (r->kind == SETK_FLAG) lstrcpyA(why, "0 or 1 expected");
        else snprintf(why, sizeof why, "%d to %d expected", lo, hi);
        ini_error(f, s, k, v, why);
        return 0;
    }
    *(int *)at = n;
    return 1;
}

// the keys of a whole section that are no row: told
internal void settings_unknown(const char *ini, const char *sec) {
    static char keys[16384];   // the section's key names, each 0-ended, a 0 after the last
    DWORD n = GetPrivateProfileStringA(sec, NULL, "", keys, sizeof keys, ini);
    int diff = !lstrcmpiA(sec, "easy") || !lstrcmpiA(sec, "medium") || !lstrcmpiA(sec, "hard");
    for (const char *k = keys; k < keys + n && *k; k += strlen(k) + 1) {
        int known = 0;
        for (int i = 0; i < SETTING_N && !known; i++) {
            const struct setting_row *r = &k_settings[i];
            if (lstrcmpiA(r->sec, sec) && !(diff && r->ai)) continue;
            if (r->kind != SETK_STR_P) { known = !lstrcmpiA(r->key, k); continue; }
            size_t len = strlen(r->key);   // key0 .. key15
            char *end = NULL;
            long p = !_strnicmp(k, r->key, len) && k[len] >= '0' && k[len] <= '9' ? strtol(k + len, &end, 10) : -1;
            known = p >= 0 && p < 16 && !*end;
        }
        if (!known) ini_unknown(ini, sec, k);
    }
}

internal void settings_read(const char *ini, const char *test_ini) {
    for (int i = 0; i < SETTING_N; i++) {
        const struct setting_row *r = &k_settings[i];
        g_set_from[i] = 0;
        if (r->ai) {
            for (int d = 0; d < 3; d++) {
                uint8_t from = 0;
                setting_read(r, d, r->key, r->name, (uint8_t *)&g_set_ai[d] + r->off, ini, test_ini, &from);
                if (from > g_set_from[i]) g_set_from[i] = from;
            }
            continue;
        }
        uint8_t *at = (uint8_t *)&g_set + r->off;
        if (r->kind != SETK_STR_P) {
            setting_read(r, 0, r->key, r->name, at, ini, test_ini, &g_set_from[i]);
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
    for (size_t s = 0; s < sizeof k_settings_whole / sizeof *k_settings_whole; s++) settings_unknown(ini, k_settings_whole[s]);
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

// the values in force, a line per section, the test ini's own marked * (the log opens after settings_read); [all] as
// the medium AI has it, [easy] and [hard] where theirs differ
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
        fprintf(f, "# settings [%s] (where not as medium):", k_set_diff[d]);
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
