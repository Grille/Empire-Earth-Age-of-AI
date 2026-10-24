// THE SETTINGS, READ IN ONE PLACE: the rows (rows.h) become struct settings g_set, filled by settings_read when the DLL
// loads - each row's text from the test ini's [autotest] (a test run), else from its section of aimod.ini, else its
// default - and checked by its kind; a mistake is logged (ini.c: file, section, key) and the row keeps its default.
// The parts read g_set instead of the file: one reading of each key, one rule for the test ini's overrides, and the
// keys of a whole section (k_settings_whole) known, so a key that is none of them is told. Included by eeaimod.c
// after base/ini.c, before every part that reads a setting.
#include "settings/rows.h"

#define SET_FIELD_FLAG(key, a) int key;
#define SET_FIELD_INT(key, a) int key;
#define SET_FIELD_STR(key, a) char key[a];
#define SET_FIELD_STR_P(key, a) char key[16][a];
#define SET_FIELD(kind, sec, key, def, a, b) SET_FIELD_##kind(key, a)
struct settings { SETTINGS(SET_FIELD) };
internal struct settings g_set;

#define SET_ID(kind, sec, key, def, a, b) SETTING_##key,
enum { SETTINGS(SET_ID) SETTING_N };

enum { SK_FLAG, SK_INT, SK_STR, SK_STR_P };
struct setting_row { const char *sec, *key; int kind; size_t off; int def; const char *def_s; int a, b; };
#define SET_ROW_FLAG(sec, key, def, a, b) { #sec, #key, SK_FLAG, offsetof(struct settings, key), def, NULL, a, b },
#define SET_ROW_INT(sec, key, def, a, b) { #sec, #key, SK_INT, offsetof(struct settings, key), def, NULL, a, b },
#define SET_ROW_STR(sec, key, def, a, b) { #sec, #key, SK_STR, offsetof(struct settings, key), 0, def, a, b },
#define SET_ROW_STR_P(sec, key, def, a, b) { #sec, #key, SK_STR_P, offsetof(struct settings, key), 0, def, a, b },
#define SET_ROW(kind, sec, key, def, a, b) SET_ROW_##kind(sec, key, def, a, b)
internal const struct setting_row k_settings[SETTING_N] = { SETTINGS(SET_ROW) };

// where each row's value came from: 0 its default, 1 aimod.ini, 2 the test ini ([autotest])
internal uint8_t g_set_from[SETTING_N];
internal const char *settings_section(int id) {   // the section a row's value was read from
    return g_set_from[id] == 2 ? "autotest" : k_settings[id].sec;
}

// key's text for row r: [autotest] of the test ini, else r's section of aimod.ini (a row of section autotest: the test
// ini only); *file, *sec where; 0: not set, the default
internal int setting_raw(const struct setting_row *r, const char *key, const char *ini, const char *test_ini, char *out, int cap,
                         const char **file, const char **sec, uint8_t *from) {
    if (test_ini && ini_raw("autotest", key, test_ini, out, cap)) { *file = test_ini; *sec = "autotest"; *from = 2; return 1; }
    if (lstrcmpA(r->sec, "autotest") && ini_raw(r->sec, key, ini, out, cap)) { *file = ini; *sec = r->sec; *from = 1; return 1; }
    return 0;
}

// one key of row r into at; 1: read (not the default)
internal int setting_read(const struct setting_row *r, const char *key, void *at, const char *ini, const char *test_ini, uint8_t *from) {
    char v[MAX_PATH + 8];
    const char *f, *s;
    if (r->kind == SK_STR || r->kind == SK_STR_P) {
        lstrcpynA((char *)at, r->def_s, r->a);
        if (!setting_raw(r, key, ini, test_ini, v, sizeof v, &f, &s, from)) return 0;
        if ((int)strlen(v) >= r->a) {
            char why[64];
            snprintf(why, sizeof why, "at most %d characters", r->a - 1);
            ini_error(f, s, key, v, why);
            return 0;
        }
        lstrcpyA((char *)at, v);
        return 1;
    }
    *(int *)at = r->def;
    double x;
    int n;
    if (!setting_raw(r, key, ini, test_ini, v, sizeof v, &f, &s, from) || !ini_eval(f, s, key, v, &x) || !ini_whole(f, s, key, v, x, &n))
        return 0;
    int lo = r->kind == SK_FLAG ? 0 : r->a, hi = r->kind == SK_FLAG ? 1 : r->b;
    if (n < lo || n > hi) {
        char why[64];
        if (r->kind == SK_FLAG) lstrcpyA(why, "0 or 1 expected");
        else snprintf(why, sizeof why, "%d to %d expected", lo, hi);
        ini_error(f, s, key, v, why);
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
            setting_read(r, r->key, at, ini, test_ini, &g_set_from[i]);
            continue;
        }
        for (int p = 0; p < 16; p++) {
            char key[48];
            snprintf(key, sizeof key, "%s%d", r->key, p);
            uint8_t from = 0;
            setting_read(r, key, at + p * r->a, ini, test_ini, &from);
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
        if (r->kind == SK_FLAG || r->kind == SK_INT) fprintf(f, " %s%s=%d", mark, r->key, *(const int *)at);
        else if (r->kind == SK_STR) fprintf(f, " %s%s=%s", mark, r->key, (const char *)at);
        else
            for (int p = 0; p < 16; p++)
                if (at[p * r->a]) fprintf(f, " %s%s%d=%s", mark, r->key, p, (const char *)(at + p * r->a));
    }
    if (sec) fputc('\n', f);
}
