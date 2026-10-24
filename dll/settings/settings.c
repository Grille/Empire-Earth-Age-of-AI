// THE SETTINGS, READ IN ONE PLACE: the rows (rows.h) become struct settings g_set, filled by settings_read when the DLL
// loads - each row's text from the test ini's [autotest] (a test run), else from its section of aimod.ini, else its
// default - and checked by its kind; a mistake is logged (ini.c: file, section, key) and the row keeps its default.
// The parts read g_set instead of the file: one reading of each key, one rule for the test ini's overrides, and the
// keys of a whole section (k_settings_whole) known, so a key that is none of them is told. Included by eeaimod.c
// after base/ini.c, before every part that reads a setting.
#include "settings/rows.h"

#define SET_FIELD_FLAG(name, a) int name;
#define SET_FIELD_INT(name, a) int name;
#define SET_FIELD_PCT(name, a) int name;
#define SET_FIELD_CHOICE(name, a) int name;
#define SET_FIELD_STR(name, a) char name[a];
#define SET_FIELD_STR_P(name, a) char name[16][a];
#define SET_FIELD_X(kind, sec, key, def, a, b) SET_FIELD_##kind(key, a)
#define SET_FIELD_S(kind, sec, key, def, a, b) SET_FIELD_##kind(sec##_##key, a)
struct settings { SETTINGS(SET_FIELD_X, SET_FIELD_S) };
internal struct settings g_set;

#define SET_ID_X(kind, sec, key, def, a, b) SETTING_##key,
#define SET_ID_S(kind, sec, key, def, a, b) SETTING_##sec##_##key,
enum { SETTINGS(SET_ID_X, SET_ID_S) SETTING_N };

enum { SK_FLAG, SK_INT, SK_PCT, SK_CHOICE, SK_STR, SK_STR_P };
// name: the field's, and the key that overrides it in the test ini; choices: a CHOICE's names, '|' between them
struct setting_row { const char *sec, *key, *name; int kind; size_t off; int def; const char *def_s, *choices; int a, b; };
#define SET_ROW_FLAG(sec, key, name, def, a, b) { #sec, #key, #name, SK_FLAG, offsetof(struct settings, name), def, NULL, NULL, a, b },
#define SET_ROW_INT(sec, key, name, def, a, b) { #sec, #key, #name, SK_INT, offsetof(struct settings, name), def, NULL, NULL, a, b },
#define SET_ROW_PCT(sec, key, name, def, a, b) { #sec, #key, #name, SK_PCT, offsetof(struct settings, name), def, NULL, NULL, a, b },
#define SET_ROW_CHOICE(sec, key, name, def, a, b) { #sec, #key, #name, SK_CHOICE, offsetof(struct settings, name), 0, def, a, 0, b },
#define SET_ROW_STR(sec, key, name, def, a, b) { #sec, #key, #name, SK_STR, offsetof(struct settings, name), 0, def, NULL, a, b },
#define SET_ROW_STR_P(sec, key, name, def, a, b) { #sec, #key, #name, SK_STR_P, offsetof(struct settings, name), 0, def, NULL, a, b },
#define SET_ROW_X(kind, sec, key, def, a, b) SET_ROW_##kind(sec, key, key, def, a, b)
#define SET_ROW_S(kind, sec, key, def, a, b) SET_ROW_##kind(sec, key, sec##_##key, def, a, b)
internal const struct setting_row k_settings[SETTING_N] = { SETTINGS(SET_ROW_X, SET_ROW_S) };

// where each row's value came from: 0 its default, 1 aimod.ini, 2 the test ini ([autotest])
internal uint8_t g_set_from[SETTING_N];
internal const char *settings_section(int id) {   // the section a row's value was read from
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

// row r's text: the test ini's [autotest] name, else key in r's section of aimod.ini (a row of section autotest: the
// test ini only); *file, *sec, *key where; 0: not set, the default
internal int setting_raw(const struct setting_row *r, const char *key, const char *name, const char *ini, const char *test_ini,
                         char *out, int cap, const char **file, const char **sec, const char **at, uint8_t *from) {
    if (test_ini && ini_raw("autotest", name, test_ini, out, cap)) { *file = test_ini; *sec = "autotest"; *at = name; *from = 2; return 1; }
    if (lstrcmpA(r->sec, "autotest") && ini_raw(r->sec, key, ini, out, cap)) { *file = ini; *sec = r->sec; *at = key; *from = 1; return 1; }
    return 0;
}

// one key of row r into at (name: its test ini key); 1: read (not the default)
internal int setting_read(const struct setting_row *r, const char *key, const char *name, void *at, const char *ini,
                          const char *test_ini, uint8_t *from) {
    char v[MAX_PATH + 8], why[96];
    const char *f, *s, *k;
    if (r->kind == SK_STR || r->kind == SK_STR_P) {
        lstrcpynA((char *)at, r->def_s, r->a);
        if (!setting_raw(r, key, name, ini, test_ini, v, sizeof v, &f, &s, &k, from)) return 0;
        if ((int)strlen(v) >= r->a) {
            snprintf(why, sizeof why, "at most %d characters", r->a - 1);
            ini_error(f, s, k, v, why);
            return 0;
        }
        lstrcpyA((char *)at, v);
        return 1;
    }
    if (r->kind == SK_CHOICE) {
        *(int *)at = setting_choice(r->choices, r->def_s);
        if (!setting_raw(r, key, name, ini, test_ini, v, sizeof v, &f, &s, &k, from)) return 0;
        int i = setting_choice(r->choices, v);
        if (i < 0) {
            snprintf(why, sizeof why, "one of %s expected", r->choices);
            ini_error(f, s, k, v, why);
            return 0;
        }
        *(int *)at = i;
        return 1;
    }
    *(int *)at = r->def;
    if (!setting_raw(r, key, name, ini, test_ini, v, sizeof v, &f, &s, &k, from)) return 0;
    if (r->kind == SK_PCT) {
        int p = ini_pct_of(f, s, k, v, -1, r->a);
        if (p < 0) return 0;
        *(int *)at = p;
        return 1;
    }
    double x;
    int n;
    if (!ini_eval(f, s, k, v, &x) || !ini_whole(f, s, k, v, x, &n)) return 0;
    int lo = r->kind == SK_FLAG ? 0 : r->a, hi = r->kind == SK_FLAG ? 1 : r->b;
    if (n < lo || n > hi) {
        if (r->kind == SK_FLAG) lstrcpyA(why, "0 or 1 expected");
        else snprintf(why, sizeof why, "%d to %d expected", lo, hi);
        ini_error(f, s, k, v, why);
        return 0;
    }
    *(int *)at = n;
    return 1;
}

// the keys of a whole section that are no row: told
internal void settings_unknown(const char *ini, const char *sec) {
    static char keys[8192];   // the section's key names, each 0-ended, a 0 after the last
    DWORD n = GetPrivateProfileStringA(sec, NULL, "", keys, sizeof keys, ini);
    for (const char *k = keys; k < keys + n && *k; k += strlen(k) + 1) {
        int known = 0;
        for (int i = 0; i < SETTING_N && !known; i++) {
            const struct setting_row *r = &k_settings[i];
            if (lstrcmpiA(r->sec, sec)) continue;
            if (r->kind != SK_STR_P) { known = !lstrcmpiA(r->key, k); continue; }
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
        uint8_t *at = (uint8_t *)&g_set + r->off;
        g_set_from[i] = 0;
        if (r->kind != SK_STR_P) {
            setting_read(r, r->key, r->name, at, ini, test_ini, &g_set_from[i]);
            continue;
        }
        for (int p = 0; p < 16; p++) {
            char key[48];
            snprintf(key, sizeof key, "%s%d", r->key, p);
            uint8_t from = 0;
            setting_read(r, key, key, at + p * r->a, ini, test_ini, &from);
            if (from > g_set_from[i]) g_set_from[i] = from;
        }
    }
    for (size_t s = 0; s < sizeof k_settings_whole / sizeof *k_settings_whole; s++) settings_unknown(ini, k_settings_whole[s]);
}

// the values in force, a line per section, the test ini's own marked * (the log opens after settings_read)
internal void settings_log(FILE *f) {
    const char *sec = NULL;
    for (int i = 0; i < SETTING_N; i++) {
        const struct setting_row *r = &k_settings[i];
        const uint8_t *at = (const uint8_t *)&g_set + r->off;
        if (!sec || lstrcmpA(sec, r->sec)) {
            fprintf(f, "%s# settings [%s]:", sec ? "\n" : "", r->sec);
            sec = r->sec;
        }
        const char *mark = g_set_from[i] == 2 ? "*" : "";
        char name[32];
        switch (r->kind) {
        case SK_FLAG: case SK_INT: fprintf(f, " %s%s=%d", mark, r->key, *(const int *)at); break;
        case SK_PCT: fprintf(f, " %s%s=%d%%", mark, r->key, *(const int *)at); break;
        case SK_CHOICE:
            setting_choice_name(r->choices, *(const int *)at, name, sizeof name);
            fprintf(f, " %s%s=%s", mark, r->key, name);
            break;
        case SK_STR:   // a key for a server stays out of the log
            fprintf(f, " %s%s=%s", mark, r->key, !lstrcmpA(r->key, "api_key") && *at ? "(set)" : (const char *)at);
            break;
        default:
            for (int p = 0; p < 16; p++)
                if (at[p * r->a]) fprintf(f, " %s%s%d=%s", mark, r->key, p, (const char *)(at + p * r->a));
        }
    }
    if (sec) fputc('\n', f);
}
