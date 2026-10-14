// TEXTS: the mod's own words outside the personalities, from <game>\aimod_ai\ (a file starting with "_" is no
// personality): _system.ini - the system lines the player reads, [system] (English) and [system:<language>] (the game's
// language, as the personalities' [lines:de]); _template.ini - the language model's instructions, [prompt] (English:
// the model is told which language to answer in). A missing file or key keeps the built-in text; "texts dump" (control
// pipe) writes both files with every key and its built-in text (to aimod_ai\, existing files are kept unless "texts
// dump force"). Placeholders in braces - {ai}, {name}, ... - each key's own are listed above it in the dump; an unknown
// one stays as written. "\n" in a value is a line break. Included by eeaimod.c after persona.c.
struct text_def { const char *key, *en, *de, *note; };
static const struct text_def k_sys[] = {
    { "now_enemy", "{ai} is now your enemy.", "{ai} ist jetzt Ihr Feind.", "an AI's stance towards you changed" },
    { "now_ally", "{ai} is now your ally.", "{ai} ist jetzt mit Ihnen verb\xfcndet.", NULL },
    { "vassal_ask_lord", "{ai} is a vassal of {lord}: negotiate with {lord}.", "{ai} ist Vasall von {lord}: verhandeln Sie mit {lord}.",
      "your peace offer to a vassal" },
    { "block_peace", "{ai} and {name} make peace: they are in the same alliance.",
      "{ai} und {name} schlie\xdf" "en Frieden: sie stehen im selben B\xfcndnis.", "two members of your alliance block made peace" },
    { "block_unity", "{ai} is allied with your allies and offers peace: turn allied within 2 minutes, or the alliance breaks.",
      "{ai} ist mit Ihren Verb\xfcndeten verb\xfcndet und bietet Frieden an: stellen Sie binnen 2 Minuten auf Verb\xfcndet, sonst "
      "zerbricht das B\xfcndnis.", NULL },
    { "veto_own", "{ai} refuses: its ally {ally} won't make peace with {name}.",
      "{ai} lehnt ab: der Verb\xfcndete {ally} will keinen Frieden mit {name}.", "a peace offer refused for an ally's sake" },
    { "veto_block", "{ai} refuses: no peace with {name} while allied with {ally}.",
      "{ai} lehnt ab: kein Frieden mit {name} und dessen B\xfcndnis mit {ally}.", NULL },
    { "alliance_offer", "{ai} offers an alliance against {enemy}: set {ai} to allied within 2 minutes to accept.",
      "{ai} bietet ein B\xfcndnis gegen {enemy} an: stellen Sie {ai} binnen 2 Minuten auf Verb\xfcndet, um anzunehmen.", NULL },
    { "capitulation", "{ai} surrenders and offers {tribute}: set {ai} to allied within 2 minutes to accept.",
      "{ai} kapituliert und bietet {tribute}: stellen Sie {ai} binnen 2 Minuten auf Verb\xfcndet, um anzunehmen.", NULL },
    { "peace_offer", "{ai} offers peace: set {ai} to allied within 2 minutes to accept.",
      "{ai} bietet Frieden an: stellen Sie {ai} binnen 2 Minuten auf Verb\xfcndet, um anzunehmen.",
      "the language model's peace offer ([llm] decide)" },
    { "tribute_sent", "{ai} sends you a tribute: {tribute}.", "{ai} schickt Ihnen Tribut: {tribute}.", NULL },
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
    { "chat_filtered", "No answer: {name} doesn't get your message (chat filter).",
      "Keine Antwort: {name} bekommt die Nachricht nicht (Chat-Filter).", NULL },
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
    { "talk_to", "to {name}: {text}", "an {name}: {text}", "an AI's message to another AI ([llm] ai_talk_show)" },
    { "food", "food", "Nahrung", "resources in a tribute ({tribute})" },
    { "wood", "wood", "Holz", NULL },
    { "stone", "stone", "Stein", NULL },
    { "gold", "gold", "Gold", NULL },
    { "iron", "iron", "Eisen", NULL },
    { "nothing", "nothing", "nichts", NULL },
};
#define N_SYS (int)(sizeof k_sys / sizeof *k_sys)
#include "texts_prompt.h"   // k_tpl: the language model's instructions
#define N_TPL (int)(sizeof k_tpl / sizeof *k_tpl)
static char *g_sys_base[N_SYS], *g_sys_lang[N_SYS], *g_tpl_val[N_TPL];
static int g_texts_loaded;

static char *text_unescape(const char *v) {   // "\n" -> a line break
    char *r = _strdup(v), *w = r;
    for (const char *s = v; *s; s++) {
        if (s[0] == '\\' && s[1] == 'n') { *w++ = '\n'; s++; continue; }
        *w++ = *s;
    }
    *w = 0;
    return r;
}
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
static int g_texts_sys_n, g_texts_tpl_n;   // keys read (log)
static void texts_file(const char *name, int sys) {
    char path[MAX_PATH];
    texts_dir(path);
    lstrcatA(path, name);
    char *text = read_text_acp(path);
    if (!text) return;
    int section = 0;   // 1 [system] / [prompt], 2 [system:<game language>]
    for (char *ln = strtok(text, "\n"); ln; ln = strtok(NULL, "\n")) {
        char *s = trim(ln);
        if (!*s || *s == ';' || *s == '#') continue;
        if (*s == '[') {
            char *close = strchr(s, ']');
            if (close) *close = 0;
            section = sys ? (!lstrcmpiA(s, "[system") ? 1 : !_strnicmp(s, "[system:", 8) && lang_matches(s + 8) ? 2 : 0)
                          : !lstrcmpiA(s, "[prompt");
            continue;
        }
        char *eq = strchr(s, '=');
        if (!eq || !section) continue;
        *eq = 0;
        char *key = trim(s), *val = trim(eq + 1);
        int i = sys ? text_index(k_sys, N_SYS, key) : text_index(k_tpl, N_TPL, key);
        if (i < 0) { if (g_log) fprintf(g_log, "# texts: %s: unknown key '%s' - ignored\n", name, key); continue; }
        char **slot = sys ? (section == 2 ? &g_sys_lang[i] : &g_sys_base[i]) : &g_tpl_val[i];
        free(*slot);
        *slot = text_unescape(val);
        if (sys) g_texts_sys_n++; else g_texts_tpl_n++;
    }
    free(text);
}
static void texts_load(void) {
    if (g_texts_loaded) return;
    g_texts_loaded = 1;
    texts_file("_system.ini", 1);
    texts_file("_template.ini", 0);
    if (g_log) fprintf(g_log, "# texts: _system.ini %d keys, _template.ini %d keys (the rest built in)\n", g_texts_sys_n, g_texts_tpl_n);
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
// out = tmpl with its {placeholders} filled from the name/value pairs (NULL-terminated); unknown ones stay; returns
// the length
static int tsubv(char *out, int cap, const char *tmpl, va_list ap0) {
    int o = 0;
    for (const char *s = tmpl; *s && o < cap - 1;) {
        const char *rep = NULL;
        size_t kl = 0;
        if (*s == '{') {
            const char *close = strchr(s, '}');
            va_list ap;
            va_copy(ap, ap0);
            for (const char *n; close && (n = va_arg(ap, const char *)) != NULL;) {
                const char *v = va_arg(ap, const char *);
                if (strlen(n) == (size_t)(close - s - 1) && !strncmp(s + 1, n, close - s - 1)) { rep = v ? v : ""; kl = close - s + 1; break; }
            }
            va_end(ap);
        }
        if (rep) { while (*rep && o < cap - 1) out[o++] = *rep++; s += kl; }
        else out[o++] = *s++;
    }
    out[o] = 0;
    return o;
}
static int tsub(char *out, int cap, const char *tmpl, ...) {
    va_list ap;
    va_start(ap, tmpl);
    int n = tsubv(out, cap, tmpl, ap);
    va_end(ap);
    return n;
}
// a system line, filled (4 rotating buffers: a few may be in use at once)
static const char *sysf(const char *key, ...) {
    static char buf[4][512];
    static int k;
    char *out = buf[k++ & 3];
    va_list ap;
    va_start(ap, key);
    tsubv(out, sizeof buf[0], sys_text(key), ap);
    va_end(ap);
    return out;
}
// append a prompt piece to t (cap bytes, *o used), filled - with a space between it and what is there (the ini
// trims its values), not before a line break or punctuation
static void tput(char *t, int *o, int cap, const char *key, ...) {
    if (*o >= cap - 2) return;
    const char *p = tpl_text(key);
    if (*o && t[*o - 1] != ' ' && t[*o - 1] != '\n' && *p && *p != ' ' && *p != '\n' && !strchr(".,;:)", *p)) t[(*o)++] = ' ';
    va_list ap;
    va_start(ap, key);
    *o += tsubv(t + *o, cap - *o, p, ap);
    va_end(ap);
}
static const char *tnum(char *buf, int n) { wsprintfA(buf, "%d", n); return buf; }

// texts dump [force]: both files with every key and its built-in text (UTF-8)
static void text_write(FILE *f, const char *acp) {   // the game's code page -> UTF-8, line breaks as \n
    WCHAR w[2048];
    char u[6144];
    int n = MultiByteToWideChar(CP_ACP, 0, acp, -1, w, 2048);
    if (n <= 0) return;
    WideCharToMultiByte(CP_UTF8, 0, w, -1, u, sizeof u, NULL, NULL);
    for (const char *s = u; *s; s++) { if (*s == '\n') fputs("\\n", f); else fputc(*s, f); }
}
static void text_placeholders(FILE *f, const char *a, const char *b) {   // "; {ai} {name}" - those the texts use
    char seen[400] = "";
    for (int t = 0; t < 2; t++)
        for (const char *s = t ? b : a; s && (s = strchr(s, '{')) != NULL; s++) {
            const char *c = strchr(s, '}');
            if (!c || c - s > 20) continue;
            char ph[24];
            lstrcpynA(ph, s, (int)(c - s) + 2);
            if (!strstr(seen, ph) && strlen(seen) + strlen(ph) < sizeof seen - 2) { lstrcatA(seen, " "); lstrcatA(seen, ph); }
        }
    if (seen[0]) fprintf(f, "; placeholders:%s\n", seen);
}
static void cmd_texts(char *out, const char *args) {
    char dir[MAX_PATH], path[MAX_PATH];
    int force = strstr(args, "force") != NULL;
    if (strncmp(args, "dump", 4)) {
        texts_load();
        REPLY("texts: _system.ini %d keys, _template.ini %d keys read; %d system texts, %d prompt texts built in; language %s\n",
              g_texts_sys_n, g_texts_tpl_n, N_SYS, N_TPL, g_lang);
        return;
    }
    texts_dir(dir);
    for (int sys = 1; sys >= 0; sys--) {
        wsprintfA(path, "%s%s", dir, sys ? "_system.ini" : "_template.ini");
        if (!force && GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) { REPLY("kept %s (texts dump force overwrites)\n", path); continue; }
        FILE *f = fopen(path, "wb");
        if (!f) { REPLY("error: cannot write %s\n", path); continue; }
        if (sys) {
            fputs("; _system.ini - the system lines eeaimod shows the player (white, from nobody in particular). Not a\n"
                  "; personality. [system] is English; [system:de] (or :fr, :es ... - the game's language) replaces what it has.\n"
                  "; A key left out keeps the built-in text. Placeholders in braces are filled in; \\n is a line break.\n"
                  "; Written by \"texts dump\" with the built-in texts.\n\n[system]\n", f);
            for (int i = 0; i < N_SYS; i++) {
                if (k_sys[i].note) { fputs("; ", f); fputs(k_sys[i].note, f); fputs("\n", f); }
                text_placeholders(f, k_sys[i].en, NULL);
                fprintf(f, "%s=", k_sys[i].key); text_write(f, k_sys[i].en); fputs("\n", f);
            }
            fputs("\n[system:de]\n", f);
            for (int i = 0; i < N_SYS; i++)
                if (k_sys[i].de) { fprintf(f, "%s=", k_sys[i].key); text_write(f, k_sys[i].de); fputs("\n", f); }
        } else {
            fputs("; _template.ini - what eeaimod tells the language model ([llm] in aimod.ini): who the AI is, the game as it\n"
                  "; stands, the situations, the tools. English: the model is told which language to answer in. A key left out\n"
                  "; keeps the built-in text. Placeholders in braces are filled in (each key's are listed above it); \\n is a\n"
                  "; line break. The pieces are joined with a space between them.\n"
                  "; Written by \"texts dump\" with the built-in texts.\n\n[prompt]\n", f);
            for (int i = 0; i < N_TPL; i++) {
                if (k_tpl[i].note) { fputs("; ", f); fputs(k_tpl[i].note, f); fputs("\n", f); }
                text_placeholders(f, k_tpl[i].en, NULL);
                fprintf(f, "%s=", k_tpl[i].key); text_write(f, k_tpl[i].en); fputs("\n", f);
            }
        }
        fclose(f);
        REPLY("written %s\n", path);
    }
}
