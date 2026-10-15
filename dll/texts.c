// TEXTS: the mod's own words outside the personalities, from <game>\aimod_ai\ (a file starting with "_" is no
// personality): _system.eeai - the system lines the player reads, [system] (English) and [system:<language>] (the game's
// language, as the personalities' [lines:de]); _template.eeai - the language model's instructions, [prompt] (English:
// the model is told which language to answer in). A missing file or key keeps the built-in text; "texts dump" (control
// pipe) writes both files with every key and its built-in text (to aimod_ai\, existing files are kept unless "texts
// dump force"). In braces: expressions, as in the personalities' lines - {me}, {them}, {them.epoch}, {opinion} (expr.c)
// - and a few texts only the code has ({tribute}, {terms}, ...): each key's are listed above it in the dump; an unknown
// one stays as written. "\n" in a value is a line break. [vars] may name expressions: _system.eeai [vars] /
// [vars:<language>] for what the player reads (a personality's own [vars] first), _template.eeai [vars] for the prompts.
// Included by eeaimod.c after persona.c.
struct text_def { const char *key, *en, *de, *note; };
static const struct text_def k_sys[] = {
    { "now_enemy", "{me} is now your enemy.", "{me} ist jetzt Ihr Feind.", "an AI's stance towards you changed" },
    { "now_ally", "{me} is now your ally.", "{me} ist jetzt mit Ihnen verb\xfcndet.", NULL },
    { "vassal_ask_lord", "{me} is a vassal of {lord}: negotiate with {lord}.", "{me} ist Vasall von {lord}: verhandeln Sie mit {lord}.",
      "your peace offer to a vassal" },
    { "block_peace", "{me} and {ally} make peace: they are in the same alliance.",
      "{me} und {ally} schlie\xdf" "en Frieden: sie stehen im selben B\xfcndnis.", "two members of your alliance block made peace" },
    { "block_unity", "{me} is allied with your allies and offers peace: turn allied within {offer_minutes} minutes, or the alliance breaks.",
      "{me} ist mit Ihren Verb\xfcndeten verb\xfcndet und bietet Frieden an: stellen Sie binnen {offer_minutes} Minuten auf Verb\xfcndet, sonst "
      "zerbricht das B\xfcndnis.", NULL },
    { "veto_own", "{me} refuses: its ally {ally} won't make peace with {them}.",
      "{me} lehnt ab: der Verb\xfcndete {ally} will keinen Frieden mit {them}.", "a peace offer refused for an ally's sake" },
    { "veto_block", "{me} refuses: no peace with {them} while allied with {ally}.",
      "{me} lehnt ab: kein Frieden mit {them} und dessen B\xfcndnis mit {ally}.", NULL },
    { "alliance_offer", "{me} offers an alliance against {enemy}: set {me} to allied within {offer_minutes} minutes to accept.",
      "{me} bietet ein B\xfcndnis gegen {enemy} an: stellen Sie {me} binnen {offer_minutes} Minuten auf Verb\xfcndet, um anzunehmen.", NULL },
    { "capitulation", "{me} surrenders and offers {tribute}: set {me} to allied within {offer_minutes} minutes to accept.",
      "{me} kapituliert und bietet {tribute}: stellen Sie {me} binnen {offer_minutes} Minuten auf Verb\xfcndet, um anzunehmen.", NULL },
    { "peace_offer", "{me} offers peace: set {me} to allied within {offer_minutes} minutes to accept.",
      "{me} bietet Frieden an: stellen Sie {me} binnen {offer_minutes} Minuten auf Verb\xfcndet, um anzunehmen.",
      "the language model's peace offer ([llm] decide)" },
    { "tribute_sent", "{me} sends you a tribute: {tribute}.", "{me} schickt Ihnen Tribut: {tribute}.", NULL },
    { "victory_vassals", "All remaining opponents have submitted to you. Type 'victory' in the chat to end the game as the "
      "winner - or play on.", "Alle verbliebenen Gegner haben sich Euch unterworfen. Schreibt 'sieg' in den Chat, um das Spiel "
      "als Sieger zu beenden - oder spielt weiter.", "victory over vassals (the chat words 'victory' and 'sieg' stay)" },
    { "victory_none", "No opponent stands against you any more. Type 'victory' in the chat to end the game as the winner - or "
      "play on.", "Kein Gegner steht mehr gegen Euch. Schreibt 'sieg' in den Chat, um das Spiel als Sieger zu beenden - oder "
      "spielt weiter.", NULL },
    { "victory_not_yet", "No victory yet - not submitted: {names}.", "Noch kein Sieg - nicht unterworfen: {names}.", NULL },
    { "chat_unread", "The computer players don't read the chat (AI voice off: [llm] enabled=1 in aimod.ini).",
      "Die Computerspieler lesen den Chat nicht (KI-Stimme aus: [llm] enabled=1 in aimod.ini).",
      "your chat line gets no answer" },
    { "chat_unanswered", "The computer players don't answer the chat ([llm] chat=1 in aimod.ini).",
      "Die Computerspieler beantworten den Chat nicht ([llm] chat=1 in aimod.ini).", NULL },
    { "chat_filtered", "No answer: {me} doesn't get your message (chat filter).",
      "Keine Antwort: {me} bekommt die Nachricht nicht (Chat-Filter).", NULL },
    { "chat_nobody", "No answer: your chat filter reaches no computer player.",
      "Keine Antwort: Ihr Chat-Filter erreicht keinen Computerspieler.", NULL },
    { "chat_who", "No answer: who do you mean? Name one ({names}) or choose them in the chat filter.",
      "Keine Antwort: wen meinen Sie? Nennen Sie einen Namen ({names}) oder w\xe4hlen Sie ihn im Chat-Filter.", NULL },
    { "chat_down", "No answer: the language model ({url}) can't be reached.",
      "Keine Antwort: das Sprachmodell ({url}) ist nicht erreichbar.", NULL },
    { "chat_busy", "No answer: the language model is too busy - try again in a moment.",
      "Keine Antwort: das Sprachmodell hat zu viel zu tun - versuchen Sie es gleich noch einmal.", NULL },
    { "llm_down", "AI voice: no answer from {url} ({error}) - the computer players say their fixed lines.",
      "KI-Stimme: keine Antwort von {url} ({error}) - die Computerspieler sprechen ihre festen Zeilen.", NULL },
    { "llm_failed", "AI voice: no usable answer this time ({error}) - just this once the computer player says its fixed "
      "line or goes by the rules.",
      "KI-Stimme: diesmal keine brauchbare Antwort ({error}) - nur dieses eine Mal spricht der Computerspieler seine feste "
      "Zeile oder folgt den Regeln.", "one answer failed, the server is there (llm_down: it can't be reached) - once a game" },
    { "llm_wait", "AI voice: the language model is taking long - the game is paused until it answers (resume it to play on).",
      "KI-Stimme: das Sprachmodell braucht lange - das Spiel ist pausiert, bis es antwortet (fortsetzen, um weiterzuspielen).",
      "[llm] fallback=pause: the game waits for the model" },
    { "llm_wait_down", "AI voice: {url} can't be reached - the game is paused, trying again every 5 seconds (resume it to play on).",
      "KI-Stimme: {url} ist nicht erreichbar - das Spiel ist pausiert, neuer Versuch alle 5 Sekunden (fortsetzen, um weiterzuspielen).",
      NULL },
    { "talk_to", "to {them}: {text}", "an {them}: {text}", "an AI's message to another AI ([llm] ai_talk_show)" },
    { "food", "food", "Nahrung", "resources in a tribute ({tribute})" },
    { "wood", "wood", "Holz", NULL },
    { "stone", "stone", "Stein", NULL },
    { "gold", "gold", "Gold", NULL },
    { "iron", "iron", "Eisen", NULL },
    { "nothing", "nothing", "nichts", NULL },
    { "opinion1", "hostile", "feindselig", "an opinion in words: its text - {opinion}, {them.opinion(me)} - while conditions count its number: 1 -100..-51, 2 -50..-16, 3 -15..15, 4 16..50, 5 51..100" },
    { "opinion2", "unfriendly", "ablehnend", NULL },
    { "opinion3", "indifferent", "gleichg\xfcltig", NULL },
    { "opinion4", "friendly", "freundlich", NULL },
    { "opinion5", "trusting", "vertrauensvoll", NULL },
};
#define N_SYS (int)(sizeof k_sys / sizeof *k_sys)
#include "texts_prompt.h"   // k_tpl: the language model's instructions
#define N_TPL (int)(sizeof k_tpl / sizeof *k_tpl)
static char *g_sys_base[N_SYS], *g_sys_lang[N_SYS], *g_tpl_val[N_TPL];
static int g_texts_loaded;

static int text_index(const struct text_def *t, int n, const char *key) {
    for (int i = 0; i < n; i++) if (!lstrcmpiA(t[i].key, key)) return i;
    return -1;
}
static void texts_dir(char *dir) {
    GetModuleFileNameA(NULL, dir, MAX_PATH);
    char *bs = strrchr(dir, '\\');
    if (bs) bs[1] = 0;
    lstrcatA(dir, "aimod_ai\\");
}
static int g_texts_sys_n, g_texts_tpl_n, g_texts_var_n;   // keys read (log)
static struct xvars g_vars_sys_lang;   // _system.eeai [vars:<language>], merged into g_vars_sys after loading
// _system.eeai: [system] / [system:<language>] (strings), [vars] / [vars:<language>]; _template.eeai: [prompt]
// (strings), [vars] - the format: expr.c THE TEXT FILES
static void texts_key(void *arg, const char *sec, const char *key, struct tval *v, struct tlex *l) {
    int sys = arg != NULL;
    if (!lstrcmpiA(sec, "vars") || (!_strnicmp(sec, "vars:", 5) && sys)) {
        if (sec[4] && !lang_matches(sec + 5)) return;
        xvars_set(!sys ? &g_vars_tpl : sec[4] ? &g_vars_sys_lang : &g_vars_sys, key, v->src);
        g_texts_var_n++;
        return;
    }
    int lang = sys && !_strnicmp(sec, "system:", 7);
    if (sys ? lstrcmpiA(sec, "system") && !lang : lstrcmpiA(sec, "prompt")) { tl_error(l, "an unknown section"); return; }
    if (lang && !lang_matches(sec + 7)) return;
    int i = sys ? text_index(k_sys, N_SYS, key) : text_index(k_tpl, N_TPL, key);
    if (i < 0) { tl_error(l, "an unknown key"); return; }
    if (v->kind != TV_STRING) { tl_error(l, "a string expected (\"...\")"); return; }
    char **slot = sys ? (lang ? &g_sys_lang[i] : &g_sys_base[i]) : &g_tpl_val[i];
    free(*slot);
    *slot = _strdup(v->text);
    if (sys) g_texts_sys_n++; else g_texts_tpl_n++;
}
static void texts_file(const char *name, int sys) {
    char path[MAX_PATH];
    texts_dir(path);
    lstrcatA(path, name);
    char *text = read_text_acp(path);
    if (!text) return;
    tfile_parse(name, text, texts_key, sys ? (void *)1 : NULL);
    free(text);
}
static void tpl_vars_scan(void) {   // the built-in functions of _template.eeai [vars], for x_fn_known
    for (int i = 0; i < N_TPL_VARS; i++)
        if (strchr(k_tpl_vars[i].key, '(')) { char t[96]; wsprintfA(t, "%s = 0\n", k_tpl_vars[i].key); x_fn_scan(t); }
}
static void texts_load(void) {
    if (g_texts_loaded) return;
    g_texts_loaded = 1;
    for (int i = 0; i < N_TPL_VARS; i++) xvars_set(&g_vars_tpl, k_tpl_vars[i].key, k_tpl_vars[i].en);   // the file's replace them
    texts_file("_system.eeai", 1);
    texts_file("_template.eeai", 0);
    for (int i = 0; i < g_vars_sys_lang.n; i++) xvars_set(&g_vars_sys, g_vars_sys_lang.key[i], g_vars_sys_lang.val[i]);
    if (g_log) fprintf(g_log, "# texts: _system.eeai %d keys, _template.eeai %d keys (the rest built in), %d [vars]\n",
                       g_texts_sys_n, g_texts_tpl_n, g_texts_var_n);
}
// a system line's text in the game's language: [system:<language>], the built-in German for "de", [system], the
// built-in English
static const char *sys_text(const char *key) {
    texts_load();
    int i = text_index(k_sys, N_SYS, key);
    if (i < 0) return key;
    int de = !lstrcmpiA(g_lang, "de");
    return g_sys_lang[i] ? g_sys_lang[i] : de && k_sys[i].de ? k_sys[i].de : g_sys_base[i] ? g_sys_base[i] : k_sys[i].en;
}
static const char *tpl_text(const char *key) {
    texts_load();
    int i = text_index(k_tpl, N_TPL, key);
    return i < 0 ? key : g_tpl_val[i] ? g_tpl_val[i] : k_tpl[i].en;
}
// out = tmpl filled (x_fill, with these [vars]): its expressions - {me}, {them.epoch}, {opinion} - for pl speaking to
// other (NULL: the local player; in a system line the reader) with third as enemy, and the few texts only the code has
// - {tribute}, {terms} ... - from the name/value pairs (NULL-terminated). None of those may have a value's name: it
// would hide that value ({epoch} the other's here, mine in a line) - such a pair is a mistake. Returns the length.
static void *g_xc_pl, *g_xc_other;   // tput: who llm_system's pieces are for
static int tsubv_vars(char *out, int cap, const char *tmpl, void *pl, void *other, void *third, va_list ap0,
                      const struct xvars *vars) {
    const char *pn[24], *pv[24];
    int np = 0;
    va_list ap;
    va_copy(ap, ap0);
    for (const char *n; np < 24 && (n = va_arg(ap, const char *)) != NULL; np++) {
        const char *v = va_arg(ap, const char *);
        if (x_is_value(n)) x_error("{%s}: a placeholder with the name of a value (eeaimod)", n);
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
// a prompt piece (_template.eeai) filled for pl speaking to other
static int tsub(char *out, int cap, const char *tmpl, void *pl, void *other, ...) {
    va_list ap;
    va_start(ap, other);
    int n = tsubv_vars(out, cap, tmpl, pl, other, NULL, ap, &g_vars_tpl);
    va_end(ap);
    return n;
}
// a prompt piece for pl speaking to other, third the enemy
static int tsub3(char *out, int cap, const char *tmpl, void *pl, void *other, void *third, ...) {
    va_list ap;
    va_start(ap, third);
    int n = tsubv_vars(out, cap, tmpl, pl, other, third, ap, &g_vars_tpl);
    va_end(ap);
    return n;
}
// a system line about AI pl (me) for the reader other (them; NULL: the local player), third the enemy, filled (4
// rotating buffers: a few may be in use at once)
static const char *sysf(const char *key, void *pl, void *other, void *third, ...) {
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
    const char *p = tpl_text(key);
    if (*o && t[*o - 1] != ' ' && t[*o - 1] != '\n' && *p && *p != ' ' && *p != '\n' && !strchr(".,;:)", *p)) t[(*o)++] = ' ';
    va_list ap;
    va_start(ap, key);
    *o += tsubv_vars(t + *o, cap - *o, p, g_xc_pl, g_xc_other, NULL, ap, &g_vars_tpl);
    va_end(ap);
}
static const char *tnum(char *buf, int n) { wsprintfA(buf, "%d", n); return buf; }

// texts dump [force]: both files with every key and its built-in text (UTF-8)
// a string as the files write it: "...", the game's code page -> UTF-8, \" \\ \n - inside {braces} as written (an
// expression's quotes are its own). One longer than the line (pre: what is on it already) in pieces, a line each:
// ("..." <line break> "...") - the file's reader joins strings side by side in ( ); cut after a space outside braces
#define DUMP_LINE 118
static void text_write(FILE *f, const char *acp, int pre) {
    WCHAR w[2048];
    char u[6144], e[12288];
    int n = MultiByteToWideChar(CP_ACP, 0, acp, -1, w, 2048);
    if (n <= 0) return;
    WideCharToMultiByte(CP_UTF8, 0, w, -1, u, sizeof u, NULL, NULL);
    int depth = 0, o = 0, cut[512], nc = 0;
    for (const char *s = u; *s && o < (int)sizeof e - 3; s++) {
        if (*s == '{') depth++;
        else if (*s == '}' && depth) depth--;
        if (*s == '\n') { e[o++] = '\\'; e[o++] = 'n'; }
        else if (!depth && (*s == '"' || *s == '\\')) { e[o++] = '\\'; e[o++] = *s; }
        else e[o++] = *s;
        if (*s == ' ' && !depth && nc < 512) cut[nc++] = o;   // a piece may end after this space
    }
    e[o] = 0;
    if (pre + o + 2 <= DUMP_LINE || !nc) { fprintf(f, "\"%s\"", e); return; }
    int from = 0, col = pre + 1, i = 0;
    fputc('(', f);
    for (;;) {
        int to = o;
        if (col + (o - from) + 3 > DUMP_LINE) {   // the last cut that keeps the piece on its line (else the next one)
            int best = -1;
            for (; i < nc && col + (cut[i] - from) + 2 <= DUMP_LINE; i++) best = cut[i];
            if (best <= from && i < nc) best = cut[i++];
            if (best > from) to = best;
        }
        fprintf(f, "%s\"%.*s\"", from ? "\n    " : "", to - from, e + from);
        if (to >= o) break;
        from = to;
        col = 4;
    }
    fputc(')', f);
}
// the [vars] of the file as written - comments, line breaks - read before it is written anew: every [vars] /
// [vars:<language>] section with its head (all: _system.eeai), or the entries of [vars] that aren't built in
// (_template.eeai: an entry from its key line - and the comment lines right above it - to the next one). NULL: none
static char *vars_kept(const char *path, int all) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *b = malloc((size_t)size + 1), *out = malloc((size_t)size + 2);
    size = (long)fread(b, 1, (size_t)size, f);
    fclose(f);
    b[size] = 0;
    int o = 0, in = 0, keep = 0;
    const char *chunk = NULL, *com = NULL;   // the entry's start; the first of the comment lines right above a key
    for (const char *l = b; *l; ) {
        const char *nl = strchr(l, '\n'), *e = nl ? nl + 1 : l + strlen(l), *t = l;
        while (*t == ' ' || *t == '\t') t++;
        int head = *l == '[', key = !head && in && !all && tl_key_line(l);
        if (head || key) {   // the entry before ends here (with the comments above this key)
            const char *end = key && com ? com : l;
            if (chunk && keep) { memcpy(out + o, chunk, (size_t)(end - chunk)); o += (int)(end - chunk); }
            chunk = NULL;
        }
        if (head) {
            in = !_strnicmp(l, "[vars]", 6) || !_strnicmp(l, "[vars:", 6);
            if (in && all) { chunk = l; keep = 1; }   // the whole section, its head too
        } else if (key) {
            char name[48];
            int n = 0;
            while ((isalnum((unsigned char)t[n]) || t[n] == '_') && n < 47) { name[n] = t[n]; n++; }
            name[n] = 0;
            int j = 0;
            while (j < N_TPL_VARS && !xkey_is(k_tpl_vars[j].key, name)) j++;
            chunk = com ? com : l;
            keep = j == N_TPL_VARS;   // a built-in one is written afresh
        }
        if (!head && in && !all) com = *t == ';' || *t == '#' ? (com ? com : l) : NULL;
        l = e;
    }
    if (chunk && keep) { size_t r = strlen(chunk); memcpy(out + o, chunk, r); o += (int)r; }
    while (o && (out[o - 1] == '\n' || out[o - 1] == '\r' || out[o - 1] == ' ')) o--;   // its blank lines at the end
    free(b);
    if (!o) { free(out); return NULL; }
    out[o++] = '\n';
    out[o] = 0;
    return out;
}
static void text_placeholders(FILE *f, const char *a, const char *b) {   // "; {me} {them}" - those the texts use
    char seen[1000] = "";
    for (int t = 0; t < 2; t++)
        for (const char *s = t ? b : a; s && (s = strchr(s, '{')) != NULL; s++) {
            const char *c = x_close(s);   // the matching brace: quotes and nested braces skipped
            if (!c || c - s > 70) continue;
            char ph[72];
            lstrcpynA(ph, s, (int)(c - s) + 2);
            if (!strstr(seen, ph) && strlen(seen) + strlen(ph) < sizeof seen - 2) { lstrcatA(seen, " "); lstrcatA(seen, ph); }
        }
    if (seen[0]) fprintf(f, "; in braces:%s\n", seen);
}
static void var_write(FILE *f, const char *key, const char *val) {   // "name(a,b)" as it is written: name(a, b) = ...
    for (; *key; key++) { fputc(*key, f); if (*key == ',') fputc(' ', f); }
    fprintf(f, " = %s\n", val);
}
// the head of the dumped files (gen_persona.py writes the same into _persona.eeai): the rest is in README.md
static const char k_format_help[] =
    ";\n"
    "; key = value, a value is an expression: \"a string\" ({placeholders} and {expressions} in it), [\"a list\", \"of\n"
    "; strings\"] (one at random), allied ? \"a condition\", a ?? b, numbers ... The format, the expressions and every\n"
    "; value of the game (opinion, them.soldiers, epoch.middle, ...): README.md in this folder.\n";
// texts eval [P [Q]] <expression>: its value, P speaking to Q (the local player when left out), with
// _system.eeai's [vars] and P's personality's; texts line P <situation> [Q [R]]: the variant P would say now, filled
// (R: the third player - {enemy})
static void cmd_texts(char *out, const char *args) {
    char dir[MAX_PATH], path[MAX_PATH];
    int force = strstr(args, "force") != NULL;
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
        unsigned h = *(unsigned *)0x00918E84;
        void *pa = player_at(a), *pb = player_at(b >= 0 ? b : (int)h);
        struct line_ctx l;
        line_ctx_init(&l, pa, pb, c3 > 0 ? player_at(c3) : NULL);
        g_say_tribute[5] = 200;   // an example {tribute}
        if (!line) {
            struct xval v;
            char t[XVAL_CAP];
            l.c.vars[0] = NULL;
            if (!x_eval(s, &l.c, &v)) { REPLY("error: not an expression: %s\n", s); return; }
            if (!v.w) { REPLY("nothing%s\n", l.c.unknown ? "  (unknown names counted 0)" : ""); return; }
            xto_text(&v, t, sizeof t);
            char num[48] = "";
            if (v.has_n && v.has_s) { struct xval nv; xnum(&nv, v.n); xto_text(&nv, num, sizeof num); }
            REPLY("%s%s%s%s%s  (%s%s", v.has_s ? "\"" : "", t, v.has_s ? "\"" : "", num[0] ? " = " : "", num,
                  v.has_n && v.has_s ? "number and text" : v.has_s ? "text" : "number", l.c.unknown ? "; unknown names counted 0" : "");
            if (v.w > 1) REPLY("; one of %d", v.w);
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
              p == &g_computer ? "computer.eeai" : p->file, buf);
        return;
    }
    if (strncmp(args, "dump", 4)) {
        texts_load();
        REPLY("texts: _system.eeai %d keys, _template.eeai %d keys read; %d system texts, %d prompt texts built in; language %s; "
              "[vars]: _system.eeai %d, _template.eeai %d\n", g_texts_sys_n, g_texts_tpl_n, N_SYS, N_TPL, g_lang, g_vars_sys.n, g_vars_tpl.n);
        return;
    }
    texts_dir(dir);
    for (int sys = 1; sys >= 0; sys--) {
        wsprintfA(path, "%s%s", dir, sys ? "_system.eeai" : "_template.eeai");
        if (!force && GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) { REPLY("kept %s (texts dump force overwrites)\n", path); continue; }
        char *kept = vars_kept(path, sys);   // the file's own [vars], as written
        FILE *f = fopen(path, "wb");
        if (!f) { REPLY("error: cannot write %s\n", path); free(kept); continue; }
        if (sys) {
            fputs("; _system.eeai - the system lines eeaimod shows the player (white, from nobody in particular). Not a\n"
                  "; personality. [system] is English; [system:de] (or :fr, :es ... - the game's language) replaces what it has.\n"
                  "; A key left out keeps the built-in text. In braces: expressions - {me} the AI, {them} you - and the few\n"
                  "; texts only the mod has (listed above each key).\n"
                  "; Written by \"texts dump\" with the built-in texts.\n", f);
            fputs(k_format_help, f);
            fputs(";\n; [vars] here serve the system lines and every personality's lines (a personality's own [vars] first);\n"
                  "; [vars:de] (or :fr ...) replaces them in a game of that language.\n", f);
            fputs(kept ? "\n" : ";[vars]\n;mood = opinion > 30 ? \"gladly\" : \"\"\n", f);
            if (kept) fputs(kept, f);
            fputs("\n[system]\n", f);
            for (int i = 0; i < N_SYS; i++) {
                if (k_sys[i].note) { fputs("; ", f); fputs(k_sys[i].note, f); fputs("\n", f); }
                text_placeholders(f, k_sys[i].en, NULL);
                fprintf(f, "%s = ", k_sys[i].key); text_write(f, k_sys[i].en, (int)strlen(k_sys[i].key) + 3); fputs("\n", f);
            }
            fputs("\n[system:de]\n", f);
            for (int i = 0; i < N_SYS; i++)
                if (k_sys[i].de) { fprintf(f, "%s = ", k_sys[i].key); text_write(f, k_sys[i].de, (int)strlen(k_sys[i].key) + 3); fputs("\n", f); }
        } else {
            fputs("; _template.eeai - what eeaimod tells the language model ([llm] in aimod.ini): who the AI is, the game as it\n"
                  "; stands, the situations, the tools. English: the model is told which language to answer in. A key left out\n"
                  "; keeps the built-in text. In braces: expressions - {me} the AI, {them} the one it talks to, {them.epoch},\n"
                  "; {opinion} - and the few texts only the mod has (each key's are listed above it). The pieces are joined\n"
                  "; with a space between them.\n"

                  "; Written by \"texts dump\" with the built-in texts.\n", f);
            fputs(k_format_help, f);
            fputs(";\n; [vars] here serve the prompts only (English, like them). The ones written here are built in (the\n"
                  "; pieces of game_state_other, tribute_room): change them, or add your own below them.\n\n[vars]\n", f);
            for (int i = 0; i < N_TPL_VARS; i++) {
                if (k_tpl_vars[i].note) { fputs("; ", f); fputs(k_tpl_vars[i].note, f); fputs("\n", f); }
                var_write(f, k_tpl_vars[i].key, k_tpl_vars[i].en);
            }
            if (kept) { fputs("\n", f); fputs(kept, f); }   // the file's own, as written, after them
            fputs("\n[prompt]\n", f);
            for (int i = 0; i < N_TPL; i++) {
                if (k_tpl[i].note) { fputs("; ", f); fputs(k_tpl[i].note, f); fputs("\n", f); }
                text_placeholders(f, k_tpl[i].en, NULL);
                fprintf(f, "%s = ", k_tpl[i].key); text_write(f, k_tpl[i].en, (int)strlen(k_tpl[i].key) + 3); fputs("\n", f);
            }
        }
        fclose(f);
        free(kept);
        REPLY("written %s\n", path);
    }
}
