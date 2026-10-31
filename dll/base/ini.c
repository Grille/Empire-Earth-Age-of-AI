// THE INI FILES' NUMBERS (ageofai.ini, ageofai_autotest.ini): every number setting is read here, as a constant expression
// of the text language (expr.c x_const_eval) - numbers, % (35% is 0.35), + - * / ( ), min, max, abs, round, floor; no
// names, no chance() or rnd(): a setting is a fixed value (tests repeat, every PC of a multiplayer game reads the same).
// A share is written as one, 35% or 0.35, whatever the code keeps: ini_share_of gives the fraction (navy_share=25% ->
// 0.25) - so a bare navy_share=25 is 2500%, out of its range. A mistake (no number, a whole
// number expected) is logged with file, section and key, and the key keeps its default - never read half
// (GetPrivateProfileInt took the digits in front: 25% as 25, 0.35 as 0); a value out of its range (a sanity bound) is
// logged as a warning and used as it is. Missing or empty: the default. An
// inline comment (; to the line's end) is cut off. Included by eeaimod.c before everything that reads a setting.
internal int (*g_ini_eval)(const char *src, double *out, const char **why, int *is_bool);   // the text language's x_const_eval (eeaimod.c install)

internal char g_ini_pending[8][200];   // mistakes found before the log is open (load_autotest): written when it opens
internal int g_ini_npending;
internal void ini_tell(const char *line) {
    static uint32_t told[32];   // once: a key of [ai_base] or [autotest] is read for each difficulty
    uint32_t h = 2166136261u;
    for (const char *t = line; *t; t++) h = (h ^ (uint8_t)*t) * 16777619u;
    h |= 1;
    for (int i = 0; i < 32; i++) if (told[i] == h) return;
    for (int i = 0; i < 32; i++) if (!told[i]) { told[i] = h; break; }
    if (g_log) { fprintf(g_log, "# ini: %s\n", line); fflush(g_log); }
    else if (g_ini_npending < 8) lstrcpynA(g_ini_pending[g_ini_npending++], line, sizeof g_ini_pending[0]);
}
internal char *g_ini_why;   // set (MAX_PATH): a mistake's or a warning's reason goes here instead of the log (the settings program)
internal int g_ini_told;   // with g_ini_why: the worst told since the program zeroed it - 1 a warning, 2 a mistake
internal void ini_error(const char *file, const char *sec, const char *key, const char *val, const char *why) {
    if (g_ini_why) { lstrcpynA(g_ini_why, why, MAX_PATH); g_ini_told = 2; return; }
    const char *name = strrchr(file, '\\');
    char line[200];
    snprintf(line, sizeof line, "%s [%s] %s=%.40s: %s - the default instead", name ? name + 1 : file, sec, key, val, why);
    ini_tell(line);
}
internal void ini_warn(const char *file, const char *sec, const char *key, const char *val, const char *why) {   // used anyway
    if (g_ini_why) { if (g_ini_told < 2) { lstrcpynA(g_ini_why, why, MAX_PATH); g_ini_told = 1; } return; }
    const char *name = strrchr(file, '\\');
    char line[200];
    snprintf(line, sizeof line, "%s [%s] %s=%.40s: %s - used anyway", name ? name + 1 : file, sec, key, val, why);
    ini_tell(line);
}
internal void ini_unknown(const char *file, const char *sec, const char *key) {   // a key that is no setting (NULL: a section that is none)
    const char *name = strrchr(file, '\\');
    char line[200];
    if (!key) snprintf(line, sizeof line, "%s [%.60s]: no such section (a misspelt or an old one?) - ignored", name ? name + 1 : file, sec);
    else snprintf(line, sizeof line, "%s [%s] %.60s: no such setting (a misspelt key?) - ignored", name ? name + 1 : file, sec, key);
    ini_tell(line);
}
internal void ini_flush(void) {   // the log just opened
    for (int i = 0; i < g_ini_npending && g_log; i++) fprintf(g_log, "# ini: %s\n", g_ini_pending[i]);
    g_ini_npending = 0;
}
// the raw text of a key (out, cap); 0: not there or empty
internal int ini_raw(const char *sec, const char *key, const char *file, char *out, int cap) {
    GetPrivateProfileStringA(sec, key, "\x01", out, (DWORD)cap, file);
    char *c = strchr(out, ';');   // an inline comment
    if (c) *c = 0;
    for (char *e = out + strlen(out); e > out && (e[-1] == ' ' || e[-1] == '\t'); ) *--e = 0;
    return out[0] && out[0] != '\x01';
}
// v worked out; 0: a mistake (told)
// a number - a bool (true, false, 1 < 2) only where bool_ok, a 0 / 1 key's: anywhere else a mistake, not a 1 or 0
internal int ini_eval_as(const char *file, const char *sec, const char *key, const char *v, double *out, int bool_ok) {
    const char *why = NULL;
    int b = 0;
    if (!g_ini_eval(v, out, &why, &b)) {
        ini_error(file, sec, key, v, why ? why : "a number expected");
        return 0;
    }
    if (b && !bool_ok) {
        ini_error(file, sec, key, v, "a number expected (true and false only for a 0 / 1 key)");
        return 0;
    }
    return 1;
}
internal int ini_eval(const char *file, const char *sec, const char *key, const char *v, double *out) {
    return ini_eval_as(file, sec, key, v, out, 0);
}
internal int ini_whole(const char *file, const char *sec, const char *key, const char *v, double x, int *out) {
    if (x != (double)(long long)x || x < -2147483647.0 || x > 2147483647.0) {
        ini_error(file, sec, key, v, "a whole number expected (a share of a whole number: round(...))");
        return 0;
    }
    *out = (int)x;
    return 1;
}
// a whole number - GetPrivateProfileInt's replacement
internal int ini_int(const char *sec, const char *key, int def, const char *file) {
    char v[128];
    double x;
    int r;
    if (!ini_raw(sec, key, file, v, sizeof v) || !ini_eval(file, sec, key, v, &x) || !ini_whole(file, sec, key, v, x, &r)) return def;
    return r;
}
// a share kept as it is (home_guard_share=35% -> 0.35), lo .. hi (outside it a warning, kept)
internal double ini_share_of(const char *file, const char *sec, const char *key, const char *v, double def, double lo, double hi) {
    double x;
    if (!ini_eval(file, sec, key, v, &x)) return def;
    if (!(x >= lo && x <= hi)) {
        char why[96];
        snprintf(why, sizeof why, "a share %g%% to %g%% expected", lo * 100, hi * 100);
        ini_warn(file, sec, key, v, why);
    }
    return x;
}
// any number (autotest speed=7.5)
internal double ini_num(const char *sec, const char *key, double def, const char *file) {
    char v[128];
    double x;
    return ini_raw(sec, key, file, v, sizeof v) && ini_eval(file, sec, key, v, &x) ? x : def;
}
