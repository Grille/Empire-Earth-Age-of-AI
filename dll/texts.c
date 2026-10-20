// TEXTS: the mod's own words outside the personalities, from <game>\aimod_ai\ (a file starting with "_" is no
// personality): _system.eeai - the system lines the player reads, system = { ... } (English; in a German game the
// built-in German texts come first); _template.eeai - the language model's instructions, prompt = { ... } (English:
// the model is told which language to answer in). A missing file or key keeps the built-in text; "texts dump" (control
// pipe) writes both files with every key and its built-in text (to aimod_ai\, existing files are kept unless "texts
// dump force"). In braces: expressions, as in the personalities' lines - {me}, {them}, {them.epoch}, {opinion} (expr.c)
// - and a few texts only the code has ({tribute}, {terms}, ...): each key's are listed above it in the dump; an unknown
// one writes nothing (logged). "\n" in a value is a line break. The names outside the records (vars) are expressions
// for the texts: _system.eeai's for what the player reads (a personality's own first), _template.eeai's for the
// prompts. Included by eeaimod.c after persona.c.
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
static char *g_sys_base[N_SYS], *g_tpl_val[N_TPL];
static int g_texts_loaded;

static int text_index(const struct text_def *t, int n, const char *key) {
    for (int i = 0; i < n; i++) if (!lstrcmpiA(t[i].key, key)) return i;
    return -1;
}
static void texts_dir(char *dir) {
    lstrcpyA(dir, g_ai_dir);   // <game>\aimod_ai\, [general] ai_dir or [autotest] ai_dir
}
static int g_texts_sys_n, g_texts_tpl_n, g_texts_var_n;   // keys read (log)
// _system.eeai: system = { strings }, names (vars); _template.eeai: prompt = { strings }, names (vars) - the format:
// expr.c THE TEXT FILES
static void texts_key(void *arg, const char *sec, const char *key, struct tval *v, struct tlex *l) {
    int sys = arg != NULL;
    if (!*sec) {
        if (!lstrcmpiA(key, "include")) { tl_error(l, "include() is for the personalities"); return; }
        xvars_set(sys ? &g_vars_sys : &g_vars_tpl, key, v->src);
        g_texts_var_n++;
        return;
    }
    if (lstrcmpiA(sec, sys ? "system" : "prompt")) { tl_error(l, sys ? "an unknown record (system)" : "an unknown record (prompt)"); return; }
    int i = sys ? text_index(k_sys, N_SYS, key) : text_index(k_tpl, N_TPL, key);
    if (i < 0) { tl_error(l, "an unknown key"); return; }
    if (v->kind != TV_STRING) { tl_error(l, "a string expected (\"...\")"); return; }
    char **slot = sys ? &g_sys_base[i] : &g_tpl_val[i];
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
static void tpl_vars_scan(void) {   // the built-in names (vars) of _template.eeai, for x_fn_known and x_var_known
    for (int i = 0; i < N_TPL_VARS; i++)
        if (strchr(k_tpl_vars[i].key, '(')) { char t[96]; wsprintfA(t, "%s = 0\n", k_tpl_vars[i].key); x_fn_scan(t); }
        else x_var_add(k_tpl_vars[i].key, strlen(k_tpl_vars[i].key));
}
static void ph_add(char *out, size_t cap, const char *text) {   // text's {name} placeholders into out (" a b ")
    for (const char *s = text; s && (s = strchr(s, '{')) != NULL; s++) {
        const char *e = s + 1;
        while (isalnum((unsigned char)*e) || *e == '_') e++;
        size_t n = (size_t)(e - s - 1);
        if (*e != '}' || !n || n > 40) continue;
        char w[48];
        w[0] = ' ';
        memcpy(w + 1, s + 1, n);
        w[n + 1] = ' ';
        w[n + 2] = 0;
        if (!strstr(out, w) && strlen(out) + n + 2 < cap) lstrcatA(out, w + 1);
    }
}
// expr.c x_name_check: the placeholders the code fills for a key - those its built-in texts use ({url}, {what} ...);
// for the names (vars: they may serve any key) and the rest: all of them
static const char *x_code_placeholders(const char *file, const char *sec, const char *key) {
    static char one[1000], all[6000];
    if (!all[0]) {
        lstrcpyA(all, " tribute tribute_total ");
        for (int i = 0; i < N_SYS; i++) { ph_add(all, sizeof all, k_sys[i].en); ph_add(all, sizeof all, k_sys[i].de); }
        for (int i = 0; i < N_TPL; i++) ph_add(all, sizeof all, k_tpl[i].en);
    }
    int sys = !lstrcmpiA(file, "_system.eeai"), tpl = !lstrcmpiA(file, "_template.eeai");
    if ((sys && !lstrcmpiA(sec, "system")) || (tpl && !lstrcmpiA(sec, "prompt"))) {
        int i = sys ? text_index(k_sys, N_SYS, key) : text_index(k_tpl, N_TPL, key);
        lstrcpyA(one, " ");
        if (i >= 0) { ph_add(one, sizeof one, sys ? k_sys[i].en : k_tpl[i].en); if (sys) ph_add(one, sizeof one, k_sys[i].de); }
        return one;
    }
    if (!lstrcmpiA(sec, "lines")) return " tribute tribute_total ";   // a personality's line (line_ctx_init)
    return all;
}
static void texts_load(void) {
    if (g_texts_loaded) return;
    g_texts_loaded = 1;
    for (int i = 0; i < N_TPL_VARS; i++) xvars_set(&g_vars_tpl, k_tpl_vars[i].key, k_tpl_vars[i].en);   // the file's replace them
    texts_file("_system.eeai", 1);
    texts_file("_template.eeai", 0);
    if (g_log) fprintf(g_log, "# texts: _system.eeai %d keys, _template.eeai %d keys (the rest built in), %d names (vars)\n",
                       g_texts_sys_n, g_texts_tpl_n, g_texts_var_n);
}
// a system line's text in the game's language: the built-in German for "de", _system.eeai's, the built-in English
static const char *sys_text(const char *key) {
    texts_load();
    int i = text_index(k_sys, N_SYS, key);
    if (i < 0) return key;
    return lang_matches("de") && k_sys[i].de ? k_sys[i].de : g_sys_base[i] ? g_sys_base[i] : k_sys[i].en;
}
static const char *tpl_text(const char *key) {
    texts_load();
    int i = text_index(k_tpl, N_TPL, key);
    return i < 0 ? key : g_tpl_val[i] ? g_tpl_val[i] : k_tpl[i].en;
}
// out = tmpl filled (x_fill, with these names - vars): its expressions - {me}, {them.epoch}, {opinion} - for pl speaking to
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
static void text_write(FILE *f, const char *acp, int pre, int ind) {
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
        if (from) fprintf(f, "\n%*s", ind + 4, "");
        fprintf(f, "\"%.*s\"", to - from, e + from);
        if (to >= o) break;
        from = to;
        col = ind + 4;
    }
    fputc(')', f);
}
// the names (vars) of the file as written - comments, line breaks - read before it is written anew: every one outside
// the records (all: _system.eeai), or those that aren't built in (_template.eeai) - an entry from its line (and the
// comment lines right above it) to the next one, a record or the end. NULL: none
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
    int o = 0, rec = 0, keep = 0;
    const char *chunk = NULL, *com = NULL;   // the entry's start; the first of the comment lines right above a line
    for (const char *l = b; *l; ) {
        const char *nl = strchr(l, '\n'), *e = nl ? nl + 1 : l + strlen(l), *t = l;
        while (*t == ' ' || *t == '\t') t++;
        int key = !rec && (tl_key_line(l) || !strncmp(t, "include(", 8)), open = 0;
        if (key) {   // the entry before ends here (with the comments above this line)
            const char *end = com ? com : l, *eq = strchr(t, '=');
            if (chunk && keep) { memcpy(out + o, chunk, (size_t)(end - chunk)); o += (int)(end - chunk); }
            chunk = NULL;
            open = eq && eq < e && x_record_open(eq + 1);
        }
        if (open) rec = 1;
        else if (key && strncmp(t, "include(", 8)) {
            char name[48];
            int n = 0;
            while ((isalnum((unsigned char)t[n]) || t[n] == '_') && n < 47) { name[n] = t[n]; n++; }
            name[n] = 0;
            int j = 0;
            while (!all && j < N_TPL_VARS && !xkey_is(k_tpl_vars[j].key, name)) j++;
            chunk = com ? com : l;
            keep = all || j == N_TPL_VARS;   // a built-in one is written afresh
        } else if (rec && *t == '}') rec = 0;
        com = !rec && *t == '#' ? (com ? com : l) : NULL;
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
static void text_placeholders(FILE *f, const char *a, const char *b, int ind) {   // "# in braces: {me} {them}" - those the texts use
    char seen[1000] = "";
    for (int t = 0; t < 2; t++)
        for (const char *s = t ? b : a; s && (s = strchr(s, '{')) != NULL; s++) {
            const char *c = x_close(s);   // the matching brace: quotes and nested braces skipped
            if (!c || c - s > 70) continue;
            char ph[72];
            lstrcpynA(ph, s, (int)(c - s) + 2);
            if (!strstr(seen, ph) && strlen(seen) + strlen(ph) < sizeof seen - 2) { lstrcatA(seen, " "); lstrcatA(seen, ph); }
        }
    if (seen[0]) fprintf(f, "%*s# in braces:%s\n", ind, "", seen);
}
static void var_write(FILE *f, const char *key, const char *val) {   // "name(a,b)" as it is written: name(a, b) = ...
    for (; *key; key++) { fputc(*key, f); if (*key == ',') fputc(' ', f); }
    fprintf(f, " = %s\n", val);
}
// the head of the dumped files (gen_persona.py writes the same into _persona.eeai): the rest is in README.md
static const char k_format_help[] =
    "#\n"
    "# key = value, a value is an expression: \"a string\" ({placeholders} and {expressions} in it), [\"a list\", \"of\n"
    "# strings\"] (one at random), allied ? \"a condition\", a ?? b, numbers ... The keys the mod reads stand in records\n"
    "# (lines = { ... }); a name outside them is the file's own, for its values. The format, the expressions and every\n"
    "# value of the game (opinion, them.soldiers, epoch.middle, ...): README.md in this folder.\n";
// TEXTS CORPUS (texts corpus): every expression of the text files worked out, for comparing two builds of the
// engine (docs/testing.md "Text engine comparison") - the personalities' lines, vars, decide, colours and style,
// the system lines and prompt pieces filled, aimod.ini's numbers; each for every AI speaking to every other player,
// the generators set to fixed values before each one and put back after (the game goes on as if nothing happened).
// One line each: what was worked out, whether it worked, its value (text, number, weight, items) and the generators'
// state after it (the draws it made).
static FILE *g_corpus;
static int nplayers(void);   // diplo.c
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
        fprintf(g_corpus, " w %d n %d:%.9g s %d pl %d [", v->w, v->has_n, v->has_n ? v->n : 0, v->has_s, v->pl);
        corpus_esc(t);
        fputc(']', g_corpus);
        for (int i = 0; i < v->ln; i++) { xto_text(xl_at(v, i), t, sizeof t); fputs(i ? " | " : " items: ", g_corpus); corpus_esc(t); }
    }
    fprintf(g_corpus, " rnd %08x %08x", g_rnd, g_rnd_game);
}
static void corpus_eval(const char *tag, const char *src, void *pl, void *other, void *third, const struct xvars *own, int game, int nogame) {
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
static void corpus_fill(const char *tag, const char *tmpl, void *pl, void *other, void *third, const struct xvars *vars) {
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
    for (int pi = -1; pi < g_npersonas; pi++) {   // computer.eeai, then the personalities
        struct persona *p = pi < 0 ? &g_computer : &g_personas[pi];
        const char *pf = pi < 0 ? "computer.eeai" : p->file;
        wsprintfA(tag, "%s color", pf);
        corpus_eval(tag, p->color, NULL, NULL, NULL, &p->vars, 0, 1);
        for (int k = 0; k < p->nstyle; k++) { wsprintfA(tag, "%s style %s", pf, p->style_key[k]); corpus_eval(tag, p->style_val[k], NULL, NULL, NULL, &p->vars, 1, 1); }
        for (int a = 1; a < np && a < 16; a++) {
            if (!g_players[a] || !pls[a]) continue;
            for (int b = 1; b < np && b < 16; b++) {
                if (b == a || !pls[b]) continue;
                void *third = NULL;
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
    for (int a = 1; a < np && a < 16; a++) {
        if (!g_players[a] || !pls[a]) continue;
        for (int b = 1; b < np && b < 16; b++) {
            if (b == a || !pls[b]) continue;
            void *third = NULL;
            for (int t = 1; t < np && t < 16 && !third; t++) if (t != a && t != b && pls[t]) third = pls[t];
            for (int k = 0; k < g_vars_sys.n; k++) {
                if (strchr(g_vars_sys.key[k], '(')) continue;
                snprintf(tag, sizeof tag, "_system P%d>P%d vars %s", a, b, g_vars_sys.key[k]);
                corpus_eval(tag, g_vars_sys.val[k], pls[a], pls[b], third, NULL, 0, 0);
            }
            for (int i = 0; i < N_SYS; i++) { snprintf(tag, sizeof tag, "_system P%d>P%d %s", a, b, k_sys[i].key); corpus_fill(tag, sys_text(k_sys[i].key), pls[a], pls[b], third, &g_vars_sys); }
            for (int i = 0; i < N_TPL; i++) { snprintf(tag, sizeof tag, "_template P%d>P%d %s", a, b, k_tpl[i].key); corpus_fill(tag, tpl_text(k_tpl[i].key), pls[a], pls[b], third, &g_vars_tpl); }
        }
    }
    {   // the hand-written cases (scripts/text_engine_cases.txt: an expression a line, # comments): the syntax check,
        // then worked out with P2 speaking to P1 (P3 the enemy) with computer.eeai's and _system.eeai's vars
        char cp[MAX_PATH], ln[1024];
        wsprintfA(cp, "%s_aimod\\scripts\\text_engine_cases.txt", g_game_dir);
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
// _system.eeai's vars and P's personality's; texts line P <situation> [Q [R]]: the variant P would say now, filled
// (R: the third player - {enemy})
static void cmd_texts(char *out, const char *args) {
    char dir[MAX_PATH], path[MAX_PATH];
    int force = strstr(args, "force") != NULL;
    if (!strncmp(args, "corpus", 6)) {   // texts corpus [file]: default aimod_dumps\texts_corpus.txt
        const char *f = args + 6;
        while (*f == ' ') f++;
        if (*f) lstrcpynA(path, f, sizeof path);
        else wsprintfA(path, "%s\\texts_corpus.txt", g_dump_dir);
        texts_corpus(out, path);
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
        unsigned h = *(unsigned *)G_LOCAL_PLAYER;
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
            if (v.w > 1) REPLY("; one of %d", v.w);
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
              p == &g_computer ? "computer.eeai" : p->file, buf);
        return;
    }
    if (strncmp(args, "dump", 4)) {
        texts_load();
        REPLY("texts: _system.eeai %d keys, _template.eeai %d keys read; %d system texts, %d prompt texts built in; language %s; "
              "names (vars): _system.eeai %d, _template.eeai %d\n", g_texts_sys_n, g_texts_tpl_n, N_SYS, N_TPL, g_lang, g_vars_sys.n, g_vars_tpl.n);
        return;
    }
    texts_dir(dir);
    for (int sys = 1; sys >= 0; sys--) {
        wsprintfA(path, "%s%s", dir, sys ? "_system.eeai" : "_template.eeai");
        if (!force && GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) { REPLY("kept %s (texts dump force overwrites)\n", path); continue; }
        char *kept = vars_kept(path, sys);   // the file's own names (vars), as written
        FILE *f = fopen(path, "wb");
        if (!f) { REPLY("error: cannot write %s\n", path); free(kept); continue; }
        const struct text_def *t = sys ? k_sys : k_tpl;
        int nt = sys ? N_SYS : N_TPL;
        if (sys) {
            fputs("# _system.eeai - the system lines eeaimod shows the player (white, from nobody in particular). Not a\n"
                  "# personality. English (in a German game the built-in German lines come first). A key left out keeps the\n"
                  "# built-in text. In braces: expressions - {me} the AI, {them} you - and the few texts only the mod has\n"
                  "# (listed above each key).\n"
                  "# Written by \"texts dump\" with the built-in texts.\n", f);
            fputs(k_format_help, f);
            fputs("#\n# The names here (vars) serve the system lines and every personality's lines (a personality's own\n"
                  "# first).\n", f);
            fputs(kept ? "\n" : "#mood = opinion > 30% ? \"gladly\" : \"\"\n", f);
            if (kept) fputs(kept, f);
            fputs("\nsystem = {\n", f);
        } else {
            fputs("# _template.eeai - what eeaimod tells the language model ([llm] in aimod.ini): who the AI is, the game as it\n"
                  "# stands, the situations, the tools. English: the model is told which language to answer in. A key left out\n"
                  "# keeps the built-in text. In braces: expressions - {me} the AI, {them} the one it talks to, {them.epoch},\n"
                  "# {opinion} - and the few texts only the mod has (each key's are listed above it). The pieces are joined\n"
                  "# with a space between them.\n"
                  "# Written by \"texts dump\" with the built-in texts.\n", f);
            fputs(k_format_help, f);
            fputs("#\n# The names here (vars) serve the prompts only (English, like them). The ones written here are built in\n"
                  "# (the pieces of game_state_other, tribute_room): change them, or add your own below them.\n\n", f);
            for (int i = 0; i < N_TPL_VARS; i++) {
                if (k_tpl_vars[i].note) { fputs("# ", f); fputs(k_tpl_vars[i].note, f); fputs("\n", f); }
                var_write(f, k_tpl_vars[i].key, k_tpl_vars[i].en);
            }
            if (kept) { fputs("\n", f); fputs(kept, f); }   // the file's own, as written, after them
            fputs("\nprompt = {\n", f);
        }
        for (int i = 0; i < nt; i++) {
            if (t[i].note) { fputs("    # ", f); fputs(t[i].note, f); fputs("\n", f); }
            text_placeholders(f, t[i].en, NULL, 4);
            fprintf(f, "    %s = ", t[i].key);
            text_write(f, t[i].en, 4 + (int)strlen(t[i].key) + 3, 4);
            fputs("\n", f);
        }
        fputs("}\n", f);
        fclose(f);
        free(kept);
        REPLY("written %s\n", path);
    }
}
