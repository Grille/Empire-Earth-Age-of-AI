// TEXTS: the mod's own words, from <game>\aimod_ai\mod\: system.eeai - the system lines the player reads, system = {
// ... }; prompt.eeai - the language model's instructions, prompt = { ... }, the last layer of every personality
// (persona.c FILES): a personality's own prompt = { ... } and its other layers' come first (tpl_find). English (the
// model is told which language to answer in). The files are part of the mod like the DLL: every key the code asks for
// must be in them (a missing one is a mistake, told when they are read); without them (or without computer.eeai) the
// mod says nothing - no personalities, no AI voice, no AI diplomacy (g_texts_missing). In braces: expressions, as in
// the personalities' lines - {me}, {them}, {them.epoch}, {opinion} (expr.c) - and a few texts only the code has
// ({tribute}, {terms}, ...: k_sys, k_tpl); an unknown one writes nothing (logged). "\n" in a value is a line break.
// The names outside the records (vars) are expressions for the texts: system.eeai's for what the player reads (a
// personality's own first), those of the file a prompt text comes from for the prompts. Included by eeaimod.c after
// persona.c.
// THE KEYS the code asks for - their texts are in the files only (mod/system.eeai, mod/prompt.eeai, a
// personality's prompt = { }): each key with the placeholders the code fills in it ({tribute}, {url} ...;
// the game's values - {me}, {offer_minutes} - and the files' names are no placeholders)
struct text_key { const char *key, *ph; };
// k_sys: the system lines the player reads (mod/system.eeai)
static const struct text_key k_sys[] = {
    { "now_enemy", "" }, { "now_ally", "" }, { "vassal_ask_lord", "" }, { "block_peace", "ally" },
    { "block_unity", "" }, { "veto_own", "ally" }, { "veto_block", "ally" }, { "alliance_offer", "" },
    { "capitulation", "tribute" }, { "peace_offer", "" }, { "tribute_sent", "tribute" }, { "victory_vassals", "" },
    { "victory_none", "" }, { "victory_not_yet", "names" }, { "chat_unread", "" }, { "chat_unanswered", "" },
    { "chat_filtered", "" }, { "chat_nobody", "" }, { "chat_who", "names" }, { "chat_down", "url" },
    { "chat_busy", "" }, { "llm_down", "url error" }, { "llm_failed", "error" }, { "llm_wait", "" },
    { "llm_wait_down", "url" }, { "talk_to", "text" }, { "food", "" }, { "wood", "" }, { "stone", "" },
    { "gold", "" }, { "iron", "" }, { "nothing", "" }, { "opinion1", "" }, { "opinion2", "" }, { "opinion3", "" },
    { "opinion4", "" }, { "opinion5", "" },
};
#define N_SYS (int)(sizeof k_sys / sizeof *k_sys)
#include "texts_prompt.h"   // k_tpl: the language model's instructions
#define N_TPL (int)(sizeof k_tpl / sizeof *k_tpl)
static char *g_sys_base[N_SYS];
static int g_texts_loaded;

static int text_index(const struct text_key *t, int n, const char *key) {
    for (int i = 0; i < n; i++) if (!lstrcmpiA(t[i].key, key)) return i;
    return -1;
}
static void texts_dir(char *dir) {
    wsprintfA(dir, "%smod\\", g_ai_dir);   // <game>\aimod_ai\mod\, [general] ai_dir or [autotest] ai_dir
}
static int g_texts_sys_n, g_texts_var_n;   // keys read (log)
// mod/system.eeai: system = { strings }, names (vars) - the format: expr.c THE TEXT FILES
static void texts_key(void *arg, const char *sec, const char *key, struct tval *v, struct tlex *l) {
    (void)arg;
    if (!*sec) {
        if (!lstrcmpiA(key, "include") || !lstrcmpiA(key, "register_personality")) { tl_error(l, "include() and register_personality() are for the personalities"); return; }
        xvars_set(&g_vars_sys, key, v->src);
        g_texts_var_n++;
        return;
    }
    if (lstrcmpiA(sec, "system")) { tl_error(l, "an unknown record (system)"); return; }
    int i = text_index(k_sys, N_SYS, key);
    if (i < 0) { tl_error(l, "an unknown key"); return; }
    if (v->kind != TV_STRING) { tl_error(l, "a string expected (\"...\")"); return; }
    free(g_sys_base[i]);
    g_sys_base[i] = _strdup(v->text);
    g_texts_sys_n++;
}
static void texts_file(void) {
    char path[MAX_PATH];
    texts_dir(path);
    lstrcatA(path, "system.eeai");
    char *text = read_text_acp(path);
    if (!text) { x_error("mod/system.eeai: no such file - the mod says nothing"); g_texts_missing = 1; return; }
    tfile_parse("mod/system.eeai", text, texts_key, NULL);
    free(text);
    for (int i = 0; i < N_SYS; i++) if (!g_sys_base[i]) x_error("mod/system.eeai: no text for %s", k_sys[i].key);
}
// THE PROMPTS: mod/prompt.eeai is read as a layer (persona.c lib_get into g_prompt), its prompt = { ... } as any
// personality's (persona_key): a text by key, kept in the file's persona
static int tpl_key(const char *key) { return text_index(k_tpl, N_TPL, key); }
static void persona_prompt_key(struct persona *p, const char *key, struct tval *v, struct tlex *l) {
    int i = tpl_key(key);
    if (i < 0) { tl_error(l, "an unknown prompt key"); return; }
    if (v->kind != TV_STRING) { tl_error(l, "a string expected (\"...\")"); return; }
    if (!p->prompt) p->prompt = calloc(N_TPL, sizeof *p->prompt);
    free(p->prompt[i]);
    p->prompt[i] = _strdup(v->text);
}
static void prompt_check(const struct persona *p) {   // mod/prompt.eeai: a text for every key
    for (int i = 0; i < N_TPL; i++) if (!p->prompt || !p->prompt[i]) x_error("mod/prompt.eeai: no text for %s", k_tpl[i].key);
}
// the text of prompt key for the model of AI pl: its personality's own, else the first of its layers' (mod/prompt.eeai
// the last); *vars: the names it is filled with - those of the file it came from (with its layers')
static const char *tpl_find(ee_player *pl, const char *key, const struct xvars **vars) {
    const struct persona *p = persona_of(pl);
    if (!p) p = g_has_computer ? &g_computer : &g_prompt;   // an AI without a personality, a human player
    int i = tpl_key(key);
    if (i < 0) x_error("eeaimod: no prompt key %s", key);
    for (int l = -1; i >= 0 && l < p->nlayer; l++) {
        const struct persona *q = l < 0 ? p : p->layer[l];
        if (q->prompt && q->prompt[i]) { *vars = &q->vars; return q->prompt[i]; }
    }
    *vars = &g_prompt.vars;
    return key;   // no such key, or none in mod/prompt.eeai (told when it was read)
}
static const char *tpl_text(ee_player *pl, const char *key) { const struct xvars *v; return tpl_find(pl, key, &v); }
// expr.c x_name_check: the placeholders the code fills for a key (k_sys, k_tpl; a situation's description, sit_ and
// pair_, and a personality's line: {tribute}, {tribute_total} - line_ctx_init, llm_subst); for the names (vars: they
// may serve any key) and the rest: all of them
static const char *x_code_placeholders(const char *file, const char *sec, const char *key) {
    static char one[400], all[2000];
    if (!all[0]) {
        lstrcpyA(all, " tribute tribute_total ");
        for (int t = 0; t < 2; t++)
            for (int i = 0; i < (t ? N_TPL : N_SYS); i++) {
                const char *ph = t ? k_tpl[i].ph : k_sys[i].ph;
                for (const char *w = ph; *w; ) {
                    int n = (int)strcspn(w, " ");
                    char x[48];
                    snprintf(x, sizeof x, " %.*s ", n < 40 ? n : 40, w);
                    if (!strstr(all, x) && strlen(all) + n + 2 < sizeof all) lstrcatA(all, x + 1);
                    w += n;
                    while (*w == ' ') w++;
                }
            }
    }
    (void)file;
    int sys = !lstrcmpiA(sec, "system"), tpl = !lstrcmpiA(sec, "prompt");   // system: only mod/system.eeai has it
    if (sys || tpl) {
        int i = sys ? text_index(k_sys, N_SYS, key) : text_index(k_tpl, N_TPL, key);
        int sit = tpl && (!_strnicmp(key, "sit_", 4) || !_strnicmp(key, "pair_", 5));
        wsprintfA(one, " %s %s", i < 0 ? "" : sys ? k_sys[i].ph : k_tpl[i].ph, sit ? "tribute tribute_total " : "");
        return one;
    }
    if (!lstrcmpiA(sec, "lines")) return " tribute tribute_total ";   // a personality's line (line_ctx_init)
    return all;
}
static void texts_load(void) {
    if (g_texts_loaded) return;
    g_texts_loaded = 1;
    texts_file();
    if (g_log) fprintf(g_log, "# texts: mod/system.eeai %d keys, %d names (vars)\n", g_texts_sys_n, g_texts_var_n);
}
// a system line's text: mod/system.eeai's (the key itself when it has none - told when it was read)
static const char *sys_text(const char *key) {
    texts_load();
    int i = text_index(k_sys, N_SYS, key);
    return i >= 0 && g_sys_base[i] ? g_sys_base[i] : key;
}
// out = tmpl filled (x_fill, with these names - vars): its expressions - {me}, {them.epoch}, {opinion} - for pl speaking to
// other (NULL: the local player; in a system line the reader) with third as enemy, and the few texts only the code has
// - {tribute}, {terms} ... - from the name/value pairs (NULL-terminated). None of those may have a value's name: it
// would hide that value ({epoch} the other's here, mine in a line) - such a pair is a mistake. Returns the length.
static void *g_xc_pl, *g_xc_other;   // tput: who llm_system's pieces are for
static int tsubv_vars(char *out, int cap, const char *tmpl, ee_player *pl, ee_player *other, ee_player *third, va_list ap0,
                      const struct xvars *vars) {
    const char *pn[24], *pv[24];
    int np = 0;
    va_list ap;
    va_copy(ap, ap0);
    for (const char *n; np < 24 && (n = va_arg(ap, const char *)) != NULL; np++) {
        const char *v = va_arg(ap, const char *);
        if (g_x_host.is_value(n)) x_error("{%s}: a placeholder with the name of a value (eeaimod)", n);
        pn[np] = n;
        pv[np] = v ? v : "";
    }
    va_end(ap);
    struct xctx c = { 0 };
    c.pl = pl;
    c.other = other;
    c.third = third;
    c.pn = pn; c.pv = pv; c.np = np;
    c.vars[0] = vars;
    return x_fill(out, cap, tmpl, &c);
}
// a prompt piece (key) filled for pl speaking to other - pl's model is the one told it (tpl_find: its personality's)
static int tsub(char *out, int cap, const char *key, ee_player *pl, ee_player *other, ...) {
    const struct xvars *vars;
    const char *tmpl = tpl_find(pl, key, &vars);
    va_list ap;
    va_start(ap, other);
    int n = tsubv_vars(out, cap, tmpl, pl, other, NULL, ap, vars);
    va_end(ap);
    return n;
}
// a prompt piece for pl speaking to other, third the enemy
static int tsub3(char *out, int cap, const char *key, ee_player *pl, ee_player *other, ee_player *third, ...) {
    const struct xvars *vars;
    const char *tmpl = tpl_find(pl, key, &vars);
    va_list ap;
    va_start(ap, third);
    int n = tsubv_vars(out, cap, tmpl, pl, other, third, ap, vars);
    va_end(ap);
    return n;
}
// a system line about AI pl (me) for the reader other (them; NULL: the local player), third the enemy, filled (4
// rotating buffers: a few may be in use at once)
static const char *sysf(const char *key, ee_player *pl, ee_player *other, ee_player *third, ...) {
    static char buf[4][512];
    static int k;
    char *out = buf[k++ & 3];
    va_list ap;
    va_start(ap, third);
    tsubv_vars(out, sizeof buf[0], sys_text(key), pl, other, third, ap, &g_vars_sys);
    va_end(ap);
    return out;
}
// append a prompt piece to t (cap bytes, *o used), filled for g_xc_pl towards g_xc_other - with a space between it and
// what is there (the ini trims its values), not before a line break or punctuation
static void tput(char *t, int *o, int cap, const char *key, ...) {
    if (*o >= cap - 2) return;
    const struct xvars *vars;
    const char *p = tpl_find(g_xc_pl, key, &vars);
    if (*o && t[*o - 1] != ' ' && t[*o - 1] != '\n' && *p && *p != ' ' && *p != '\n' && !strchr(".,;:)", *p)) t[(*o)++] = ' ';
    va_list ap;
    va_start(ap, key);
    *o += tsubv_vars(t + *o, cap - *o, p, g_xc_pl, g_xc_other, NULL, ap, vars);
    va_end(ap);
}
static const char *tnum(char *buf, int n) { wsprintfA(buf, "%d", n); return buf; }

// TEXTS CORPUS (texts corpus): every expression of the text files worked out, for comparing two builds of the
// engine (docs/testing.md "Text engine comparison") - the personalities' lines, vars, decide, colours and style,
// the system lines and prompt pieces filled, aimod.ini's numbers; each for every AI speaking to every other player,
// the generators set to fixed values before each one and put back after (the game goes on as if nothing happened).
// One line each: what was worked out, whether it worked, its value (text, number, weight, items) and the generators'
// state after it (the draws it made).
static FILE *g_corpus;
static void corpus_esc(const char *s) {
    for (; *s; s++) {
        if (*s == '\n') fputs("\\n", g_corpus);
        else if (*s == '\\') fputs("\\\\", g_corpus);
        else fputc(*s, g_corpus);
    }
}
static char g_corpus_msgs[2048];   // the mistakes' messages of one evaluation (x_error, g_x_capture)
static void corpus_begin(void) { g_corpus_msgs[0] = 0; g_x_capture = g_corpus_msgs; g_x_capture_cap = sizeof g_corpus_msgs; }
static void corpus_end(void) {
    g_x_capture = NULL;
    if (g_corpus_msgs[0]) { fputs(" msgs ", g_corpus); corpus_esc(g_corpus_msgs); }
    fputc('\n', g_corpus);
}
static void corpus_val(const char *tag, const struct xval *v, int ok, int failed) {
    char t[XVAL_CAP];
    fprintf(g_corpus, "%s | ok %d failed %d", tag, ok, failed);
    if (ok && v->w) {
        xto_text(v, t, sizeof t);
        fprintf(g_corpus, " w %d n %d:%.9g s %d pl %d [", v->w, v->has_n, v->has_n ? v->n : 0, v->has_s, v->obj);
        corpus_esc(t);
        fputc(']', g_corpus);
        for (int i = 0; i < v->ln; i++) { xto_text(xl_at(v, i), t, sizeof t); fputs(i ? " | " : " items: ", g_corpus); corpus_esc(t); }
    }
    fprintf(g_corpus, " rnd %08x %08x", g_rnd, g_rnd_game);
}
static void corpus_eval(const char *tag, const char *src, ee_player *pl, ee_player *other, ee_player *third, const struct xvars *own, int game, int nogame) {
    if (!src) return;
    uint32_t r0 = g_rnd, g0 = g_rnd_game;
    g_rnd = 0x13579BDFu; g_rnd_game = 0x2468ACE0u;
    struct line_ctx l;
    line_ctx_init(&l, pl, other, third);
    l.c.vars[1] = own;
    l.c.game = game;
    l.c.nogame = nogame;
    if (nogame) { l.c.pl = l.c.other = l.c.third = NULL; l.c.np = 0; }
    struct xval v;
    corpus_begin();
    int ok = x_eval(src, &l.c, &v);
    corpus_val(tag, &v, ok, l.c.failed);
    corpus_end();
    g_rnd = r0; g_rnd_game = g0;
}
static void corpus_fill(const char *tag, const char *tmpl, ee_player *pl, ee_player *other, ee_player *third, const struct xvars *vars) {
    static const char *pn[200], *pv[200];
    static char names[200][48];
    static int np = -1;
    if (np < 0) {   // every placeholder the code fills somewhere, as "<name>"
        np = 0;
        const char *all = x_code_placeholders("", "", "");
        for (const char *s = all; *s && np < 200; ) {
            while (*s == ' ') s++;
            int k = (int)strcspn(s, " ");
            if (k && k < 40) { names[np][0] = 0; lstrcpynA(names[np], s, k + 1); pn[np] = names[np]; np++; }
            s += k;
        }
        static char vals[200][48];
        for (int i = 0; i < np; i++) { wsprintfA(vals[i], "<%s>", names[i]); pv[i] = vals[i]; }
    }
    uint32_t r0 = g_rnd, g0 = g_rnd_game;
    g_rnd = 0x13579BDFu; g_rnd_game = 0x2468ACE0u;
    struct xctx c = { 0 };
    c.pl = pl; c.other = other; c.third = third;
    c.pn = pn; c.pv = pv; c.np = np;
    c.vars[0] = vars;
    static char out[8192];
    corpus_begin();
    x_fill(out, sizeof out, tmpl, &c);
    g_x_capture = NULL;
    fprintf(g_corpus, "%s | failed %d [", tag, c.failed);
    corpus_esc(out);
    fprintf(g_corpus, "] rnd %08x %08x", g_rnd, g_rnd_game);
    corpus_end();
    g_rnd = r0; g_rnd_game = g0;
}
static void texts_corpus(char *out, const char *path) {
    texts_load();
    g_corpus = fopen(path, "wb");
    if (!g_corpus) { REPLY("error: cannot write %s\n", path); return; }
    int np = nplayers(), n0 = 0;
    void *pls[16];
    for (int j = 1; j < np && j < 16; j++) pls[j] = player_at(j);
    char tag[200];
    for (int pi = -1; pi < g_npersonas; pi++) {   // mod/computer.eeai, then the personalities
        struct persona *p = pi < 0 ? &g_computer : &g_personas[pi];
        const char *pf = p->file;
        wsprintfA(tag, "%s color", pf);
        corpus_eval(tag, p->color, NULL, NULL, NULL, &p->vars, 0, 1);
        for (int k = 0; k < p->nstyle; k++) { wsprintfA(tag, "%s style %s", pf, p->style_key[k]); corpus_eval(tag, p->style_val[k], NULL, NULL, NULL, &p->vars, 1, 1); }
        for (int a = 1; a < np && a < 16; a++) {
            if (!g_players[a] || !pls[a]) continue;
            for (int b = 1; b < np && b < 16; b++) {
                if (b == a || !pls[b]) continue;
                ee_player *third = NULL;
                for (int t = 1; t < np && t < 16 && !third; t++) if (t != a && t != b && pls[t]) third = pls[t];
                for (int k = 0; k < L_N; k++) {
                    snprintf(tag, sizeof tag, "%s P%d>P%d line %s", pf, a, b, k_lines[k].key);
                    corpus_eval(tag, p->line[k], pls[a], pls[b], third, &p->vars, 0, 0);
                }
                for (int k = 0; k < p->vars.n; k++) {
                    if (strchr(p->vars.key[k], '(')) continue;
                    snprintf(tag, sizeof tag, "%s P%d>P%d vars %s", pf, a, b, p->vars.key[k]);
                    corpus_eval(tag, p->vars.val[k], pls[a], pls[b], third, &p->vars, 0, 0);
                }
                for (int k = 0; k < D_N; k++) {
                    snprintf(tag, sizeof tag, "%s P%d>P%d decide %s", pf, a, b, k_decide_keys[k]);
                    corpus_eval(tag, p->decide[k], pls[a], pls[b], third, &p->vars, 1, 0);
                }
                n0++;
            }
        }
    }
    for (int a = 1; a < np && a < 16; a++) {   // what each AI would say: the line chosen (its source, the situation)
        if (!g_players[a] || !pls[a]) continue;
        for (int b = 1; b < np && b < 16; b++) {
            if (b == a || !pls[b]) continue;
            ee_player *third = NULL;
            for (int t = 1; t < np && t < 16 && !third; t++) if (t != a && t != b && pls[t]) third = pls[t];
            for (int k = 0; k < L_N; k++) {
                uint32_t r0 = g_rnd, g0 = g_rnd_game;
                g_rnd = 0x13579BDFu; g_rnd_game = 0x2468ACE0u;
                struct line_ctx l;
                line_ctx_init(&l, pls[a], pls[b], third);
                char buf[512];
                int kk = k, cnt = 0;
                corpus_begin();
                struct persona *p = line_source(pls[a], k, &kk, &l.c, 0, buf, sizeof buf, &cnt);
                snprintf(tag, sizeof tag, "said P%d>P%d %s", a, b, k_lines[k].key);
                fprintf(g_corpus, "%s | %s as %s, one of %d [", tag, p ? p->file : "-",
                        k_lines[kk].key, p ? cnt : 0);
                if (p) corpus_esc(buf);
                fprintf(g_corpus, "] rnd %08x %08x", g_rnd, g_rnd_game);
                corpus_end();
                g_rnd = r0; g_rnd_game = g0;
            }
        }
    }
    for (int a = 1; a < np && a < 16; a++) {
        if (!g_players[a] || !pls[a]) continue;
        for (int b = 1; b < np && b < 16; b++) {
            if (b == a || !pls[b]) continue;
            ee_player *third = NULL;
            for (int t = 1; t < np && t < 16 && !third; t++) if (t != a && t != b && pls[t]) third = pls[t];
            for (int k = 0; k < g_vars_sys.n; k++) {
                if (strchr(g_vars_sys.key[k], '(')) continue;
                snprintf(tag, sizeof tag, "mod/system P%d>P%d vars %s", a, b, g_vars_sys.key[k]);
                corpus_eval(tag, g_vars_sys.val[k], pls[a], pls[b], third, NULL, 0, 0);
            }
            for (int i = 0; i < N_SYS; i++) { snprintf(tag, sizeof tag, "mod/system P%d>P%d %s", a, b, k_sys[i].key); corpus_fill(tag, sys_text(k_sys[i].key), pls[a], pls[b], third, &g_vars_sys); }
            for (int i = 0; i < N_TPL; i++) {   // as P<a>'s model is told it (its personality's, its layers', mod/prompt.eeai's)
                const struct xvars *vars;
                const char *t = tpl_find(pls[a], k_tpl[i].key, &vars);
                snprintf(tag, sizeof tag, "prompt P%d>P%d %s", a, b, k_tpl[i].key);
                corpus_fill(tag, t, pls[a], pls[b], third, vars);
            }
        }
    }
    {   // the hand-written cases (scripts/text_engine_cases.txt: an expression a line, # comments): the syntax check,
        // then worked out with P2 speaking to P1 (P3 the enemy) with mod/computer.eeai's and mod/system.eeai's vars
        char cp[MAX_PATH], ln[1024], repo[MAX_PATH];   // the repository: EEAIMOD_REPO (run_test.ps1), else <game>\_aimod
        if (!GetEnvironmentVariableA("EEAIMOD_REPO", repo, sizeof repo) || !repo[0]) wsprintfA(repo, "%s_aimod", g_game_dir);
        snprintf(cp, sizeof cp, "%s\\scripts\\text_engine_cases.txt", repo);
        FILE *cf = fopen(cp, "rb");
        int no = 0;
        while (cf && fgets(ln, sizeof ln, cf)) {
            for (char *e = ln + strlen(ln); e > ln && (e[-1] == '\n' || e[-1] == '\r'); ) *--e = 0;
            no++;
            if (!ln[0] || ln[0] == '#') continue;
            const char *why = NULL;
            int syn = x_syntax(ln, &why);
            snprintf(tag, sizeof tag, "case %d syntax %d %s", no, syn, syn ? "" : why);
            corpus_eval(tag, ln, pls[2 < np ? 2 : 1], pls[1], 3 < np ? pls[3] : NULL, &g_computer.vars, 0, 0);
            snprintf(tag, sizeof tag, "case %d fill", no);
            corpus_fill(tag, ln, pls[2 < np ? 2 : 1], pls[1], 3 < np ? pls[3] : NULL, &g_vars_sys);
        }
        if (cf) fclose(cf);
    }
    {   // aimod.ini's numbers: every key's value as the ini reader works it out
        FILE *ini = fopen(g_aimod_ini, "rb");
        char ln[512], sec[64] = "";
        while (ini && fgets(ln, sizeof ln, ini)) {
            char *s = ln;
            while (*s == ' ' || *s == '\t') s++;
            if (*s == '[') { lstrcpynA(sec, s + 1, (int)strcspn(s + 1, "]") + 1); continue; }
            char *eq = strchr(s, '=');
            if (*s == ';' || *s == '#' || !eq) continue;
            *eq = 0;
            char *v = eq + 1, *c = strchr(v, ';');
            if (c) *c = 0;
            for (char *e = v + strlen(v); e > v && (e[-1] == '\n' || e[-1] == '\r' || e[-1] == ' ' || e[-1] == '\t'); ) *--e = 0;
            double x = 0;
            const char *why = NULL;
            corpus_begin();
            int ok = x_const_eval(v, &x, &why);
            g_x_capture = NULL;
            fprintf(g_corpus, "aimod.ini [%s] %s = %s | ok %d %.9g %s", sec, s, v, ok, ok ? x : 0, ok ? "" : why);
            corpus_end();
        }
        if (ini) fclose(ini);
    }
    fclose(g_corpus);
    g_corpus = NULL;
    REPLY("ok: texts corpus written to %s (%d personality pairs)\n", path, n0);
}
// texts eval [P [Q]] <expression>: its value, P speaking to Q (the local player when left out), with
// mod/system.eeai's vars and P's personality's; texts line P <situation> [Q [R]]: the variant P would say now, filled
// (R: the third player - {enemy})
static void cmd_texts(char *out, const char *args) {
    char path[MAX_PATH];
    if (!strncmp(args, "corpus", 6)) {   // texts corpus [file]: default aimod_dumps\texts_corpus.txt
        const char *f = args + 6;
        while (*f == ' ') f++;
        if (*f) lstrcpynA(path, f, sizeof path);
        else wsprintfA(path, "%s\\texts_corpus.txt", g_dump_dir);
        texts_corpus(out, path);
        return;
    }
    if (!strncmp(args, "bench", 5)) {   // texts bench N P Q <expression>: N times as texts eval works it out - the time
        int cnt = 0, a = 0, b = 0, n = 0;   // and the draws one takes (filters in AI code must be quick)
        const char *s = args + 5;
        if (sscanf(s, "%d %d %d %n", &cnt, &a, &b, &n) < 3 || cnt < 1 || !s[n]) { REPLY("usage: texts bench N P Q <expression>\n"); return; }
        s += n;
        struct line_ctx l;
        line_ctx_init(&l, player_at(a), player_at(b), NULL);
        l.c.vars[0] = NULL;
        struct xval v;
        LARGE_INTEGER f, t0, t1;
        unsigned d0 = g_rnd_draws;
        int ok = 1;
        QueryPerformanceFrequency(&f);
        QueryPerformanceCounter(&t0);
        for (int i = 0; i < cnt && ok; i++) ok = x_eval(s, &l.c, &v);
        QueryPerformanceCounter(&t1);
        char t[XVAL_CAP];
        if (!ok) { REPLY("error: %s\n", l.c.failed ? g_x_last_fail : "not an expression"); return; }
        xto_text(&v, t, sizeof t);
        REPLY("%.2f us each, %.1f draws each, %d times: %.60s\n", (double)(t1.QuadPart - t0.QuadPart) * 1e6 / (double)f.QuadPart / cnt,
              (double)(g_rnd_draws - d0) / cnt, cnt, t);
        return;
    }
    if (!strncmp(args, "eval", 4) || !strncmp(args, "line", 4)) {
        int line = args[0] == 'l', a = 0, b = -1, c3 = -1, n = 0;
        const char *s = args + 4;
        while (*s == ' ') s++;
        if (sscanf(s, "%d%n", &a, &n) == 1) {
            s += n;
            while (*s == ' ') s++;
            if (!line && sscanf(s, "%d%n", &b, &n) == 1) { s += n; while (*s == ' ') s++; }
        }
        char key[40] = "";
        if (line && sscanf(s, "%39s %d %d", key, &b, &c3) < 1) { REPLY("usage: texts line P situation [Q [R]]\n"); return; }
        unsigned h = *(unsigned *)GA(G_LOCAL_PLAYER);
        void *pa = player_at(a), *pb = player_at(b >= 0 ? b : (int)h);
        struct line_ctx l;
        line_ctx_init(&l, pa, pb, c3 > 0 ? player_at(c3) : NULL);
        g_say_tribute[5] = 200;   // an example {tribute}
        if (!line) {
            struct xval v;
            char t[XVAL_CAP];
            l.c.vars[0] = NULL;
            if (!x_eval(s, &l.c, &v)) {
                if (l.c.failed) REPLY("error: %s\n", g_x_last_fail);
                else REPLY("error: not an expression: %s\n", s);
                return;
            }
            if (!v.w) { REPLY("nothing%s\n", l.c.failed ? "  (a mistake left something out - see the log)" : ""); return; }
            xto_text(&v, t, sizeof t);
            char num[48] = "";
            if (v.has_n && v.has_s) { struct xval nv; xnum(&nv, v.n); xto_text(&nv, num, sizeof num); }
            REPLY("%s%s%s%s%s  (%s%s", v.has_s ? "\"" : "", t, v.has_s ? "\"" : "", num[0] ? " = " : "", num,
                  v.has_n && v.has_s ? "number and text" : v.has_s ? "text" : "number", l.c.failed ? "; a mistake left something out - see the log" : "");
            if (v.ln) REPLY("; a list of %d", v.ln);
            for (int i = 0; i < v.ln; i++) { xto_text(xl_at(&v, i), t, sizeof t); REPLY("%s%s", i ? " | " : ": ", t); }
            REPLY(")\n");
            return;
        }
        int k = -1;
        for (int i = 0; i < L_N; i++) if (!lstrcmpiA(k_lines[i].key, key)) k = i;
        if (k < 0 || !pa) { REPLY("error: no situation '%s' or no player %d\n", key, a); return; }
        int kk = k, cnt;
        char buf[256];
        struct persona *p = line_source(pa, k, &kk, &l.c, 0, buf, sizeof buf, &cnt);
        if (!p) { REPLY("P%d has no line for %s that may be said now\n", a, key); return; }
        REPLY("P%d %s%s%s, one of %d (%s): %s\n", a, key, kk != k ? " as " : "", kk != k ? k_lines[kk].key : "", cnt,
              p->file, buf);
        return;
    }
    int tn = 0;
    for (int i = 0; g_prompt.prompt && i < N_TPL; i++) tn += g_prompt.prompt[i] != NULL;
    REPLY("texts: mod/system.eeai %d of %d keys, mod/prompt.eeai %d of %d; language %s; names (vars): mod/system.eeai %d, "
          "mod/prompt.eeai %d%s\n", g_texts_sys_n, N_SYS, tn, N_TPL, g_lang, g_vars_sys.n, g_prompt.vars.n,
          g_texts_missing ? " - the mod's files are missing: it says nothing" : "");
}
