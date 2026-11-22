// THE SERVER'S KNOWLEDGE (stage 2 of the language server, check/lsp.c): completion and hover - from the tables the
// game itself goes by (lang.h's members and values with their docs, the situations, every record's keys and how each
// is written, = or =>, the constants, the functions, a script's commands) and from the files: the names a file and its
// layers define (with the comment above them), the situations' descriptions in mod\prompt.eeai. What the cursor is in
// is read off the text before it (lsp_where) - a record's key at a line's start, a member after a dot, a value
// anywhere else - with no parse of its own: a half-typed line still gets its answer. Columns are UTF-16 units (LSP's).

// a file's text as UTF-8: an open one's as the editor has it, else the file's (UTF-8, or the code page's made UTF-8)
internal char *lsp_acp_utf8(const char *s) {
    int wn = MultiByteToWideChar(CP_ACP, 0, s, -1, NULL, 0);
    wchar_t *w = wn > 0 ? malloc(sizeof *w * (size_t)wn) : NULL;
    if (!w) return _strdup(s);
    MultiByteToWideChar(CP_ACP, 0, s, -1, w, wn);
    int un = WideCharToMultiByte(CP_UTF8, 0, w, -1, NULL, 0, NULL, NULL);
    char *u = un > 0 ? malloc((size_t)un) : NULL;
    if (u) WideCharToMultiByte(CP_UTF8, 0, w, -1, u, un, NULL, NULL);
    free(w);
    return u ? u : _strdup(s);
}
internal char *lsp_file_text(const char *path) {
    for (int i = 0; i < g_lsp_ndoc; i++) if (!lstrcmpiA(g_lsp_doc[i].path, path)) return _strdup(g_lsp_doc[i].text ? g_lsp_doc[i].text : "");
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *t = n >= 0 && n < (1 << 22) ? malloc((size_t)n + 1) : NULL;
    if (t && fread(t, 1, (size_t)n, f) != (size_t)n) { free(t); t = NULL; }
    fclose(f);
    if (!t) return NULL;
    t[n] = 0;
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, t, -1, NULL, 0) > 0) return t;
    char *u = lsp_acp_utf8(t);
    free(t);
    return u;
}
// line n (0 the first) of text: its start, its length without the line's end in *len - NULL past the last
internal const char *lsp_line(const char *text, int n, size_t *len) {
    const char *s = text;
    for (int i = 0; i < n && s; i++) { s = strchr(s, '\n'); if (s) s++; }
    if (!s) return NULL;
    size_t k = strcspn(s, "\n");
    if (k && s[k - 1] == '\r') k--;
    *len = k;
    return s;
}
internal size_t lsp_col(const char *l, size_t len, int ch) {   // column ch (UTF-16 units) of line l: its byte
    size_t i = 0;
    for (int c = 0; i < len && c < ch; c++) {
        unsigned char b = (unsigned char)l[i];
        int n = b < 0x80 ? 1 : b < 0xE0 ? 2 : b < 0xF0 ? 3 : 4;
        if (n == 4) c++;   // (a pair of UTF-16 units)
        i += (size_t)n;
    }
    return i < len ? i : len;
}
internal int lsp_wc(char c) { return isalnum((unsigned char)c) || c == '_'; }

// THE RECORDS a line is in: read line by line as tfile_parse reads them - name = { opens one at a line's end, } alone
// closes it, a value's brackets counted (strings and comments left out)
struct lsp_scan { char stack[8][48]; int ns, depth; char key[48]; };   // depth: a value's brackets open; key: the entry's
internal int lsp_key_at(const char *l, size_t n, char *key, size_t *after) {   // "  key =" / "  key =>": 1, its key
    size_t i = 0, k = 0;
    while (i < n && (l[i] == ' ' || l[i] == '\t')) i++;
    while (i < n && lsp_wc(l[i]) && k < 47) key[k++] = l[i++];
    key[k] = 0;
    if (!k || isdigit((unsigned char)key[0])) return 0;
    while (i < n && (l[i] == ' ' || l[i] == '\t')) i++;
    if (i >= n || l[i] != '=' || (i + 1 < n && l[i + 1] == '=')) return 0;
    if (after) *after = i + (i + 1 < n && l[i + 1] == '>' ? 2 : 1);
    return 1;
}
internal int lsp_rest_empty(const char *l, size_t n, size_t i) {   // only spaces or a comment from i on
    while (i < n && (l[i] == ' ' || l[i] == '\t')) i++;
    return i >= n || (l[i] == '/' && i + 1 < n && l[i + 1] == '/');
}
internal void lsp_scan_line(struct lsp_scan *s, const char *l, size_t n) {
    char key[48];
    size_t after;
    if (!s->depth) {
        if (lsp_key_at(l, n, key, &after)) {
            size_t j = after;
            while (j < n && (l[j] == ' ' || l[j] == '\t')) j++;
            if (j < n && l[j] == '{' && l[after - 1] != '>' && lsp_rest_empty(l, n, j + 1)) {   // name = { ...
                if (s->ns < 8) lstrcpyA(s->stack[s->ns], key);
                s->ns++;
                s->key[0] = 0;
                return;
            }
            if (s->ns) lstrcpyA(s->key, key);
        }
        size_t j = 0;
        while (j < n && (l[j] == ' ' || l[j] == '\t')) j++;
        if (j < n && l[j] == '}' && s->ns && lsp_rest_empty(l, n, j + 1 + (j + 1 < n && l[j + 1] == ',')))  { s->ns--; s->key[0] = 0; return; }
    }
    for (size_t i = 0; i < n; i++) {
        char c = l[i];
        if (c == '/' && i + 1 < n && l[i + 1] == '/') break;
        if (c == '"' || c == '\'') {   // a string: its {placeholders} are no brackets of the value
            int verb = i && l[i - 1] == '@', bd = 0;
            for (i++; i < n; i++) {
                if (!verb && l[i] == '\\') { i++; continue; }
                if (!bd && l[i] == c) break;
                if (!verb && l[i] == '{') bd++;
                else if (l[i] == '}' && bd) bd--;
            }
            continue;
        }
        if (c == '[' || c == '(' || c == '{') s->depth++;
        else if ((c == ']' || c == ')' || c == '}') && s->depth) s->depth--;
    }
}

// WHERE THE CURSOR IS
enum { LW_NONE, LW_TOP, LW_KEY, LW_MEMBER, LW_VALUE };
struct lsp_at { int kind; char rec[48], key[48], base[96], word[64]; int script; };   // rec: the record (its outermost), key: the entry's
internal void lsp_where(const char *text, int line, int ch, struct lsp_at *at) {
    memset(at, 0, sizeof *at);
    struct lsp_scan s;
    memset(&s, 0, sizeof s);
    size_t len;
    for (int i = 0; i < line; i++) {
        const char *l = lsp_line(text, i, &len);
        if (!l) return;
        lsp_scan_line(&s, l, len);
    }
    const char *l = lsp_line(text, line, &len);
    if (!l) return;
    size_t end = lsp_col(l, len, ch);
    int in_str = 0, bd = 0, pd = 0;   // in a string (its {placeholders} open), the line's brackets
    for (size_t i = 0; i < end; i++) {
        char c = l[i];
        if (!in_str) {
            if (c == '/' && i + 1 < end && l[i + 1] == '/') { at->kind = LW_NONE; return; }
            if (c == '"' || c == '\'') { in_str = c; bd = 0; continue; }
            if (c == '[' || c == '(' || c == '{') pd++;
            else if ((c == ']' || c == ')' || c == '}') && pd) pd--;
            continue;
        }
        if (c == '\\' && !(i && l[i - 1] == '@')) { i++; continue; }
        if (!bd && c == in_str) in_str = 0;
        else if (c == '{') bd++;
        else if (c == '}' && bd) bd--;
    }
    if (in_str && !bd) return;   // in a string's text: nothing to offer
    size_t w = end;
    while (w && lsp_wc(l[w - 1])) w--;
    lstrcpynA(at->word, l + w, (int)(end - w + 1 < sizeof at->word ? end - w + 1 : sizeof at->word));
    if (s.ns) lstrcpyA(at->rec, s.stack[0]);
    char key[48];
    size_t after;
    if (!s.depth && lsp_key_at(l, len, key, &after) && after <= end) lstrcpyA(at->key, key);   // its key on this line
    else lstrcpyA(at->key, s.key);
    if (w && l[w - 1] == '.') {   // a member: the chain before the dot
        size_t b = w - 1;
        while (b && (lsp_wc(l[b - 1]) || l[b - 1] == '.')) b--;
        lstrcpynA(at->base, l + b, (int)(w - 1 - b + 1 < sizeof at->base ? w - 1 - b + 1 : sizeof at->base));
        at->kind = LW_MEMBER;
        return;
    }
    int start = 1;   // only spaces before the word
    for (size_t i = 0; i < w && start; i++) start = l[i] == ' ' || l[i] == '\t';
    if (!in_str && !s.depth && !pd && start) { at->kind = s.ns ? LW_KEY : LW_TOP; at->key[0] = 0; return; }
    at->kind = LW_VALUE;
}

// THE NAMES a file defines at its top: name = data, name => worked out, name(a) => / { } a function, let a field
struct lsp_name { char name[48]; int kind; char *doc; char detail[160]; };   // kind: LSP's CompletionItemKind
internal struct lsp_name *g_lsp_names;
internal int g_lsp_nnames, g_lsp_names_cap;
internal void lsp_names_clear(void) {
    for (int i = 0; i < g_lsp_nnames; i++) free(g_lsp_names[i].doc);
    g_lsp_nnames = 0;
}
internal void lsp_names_of(const char *text, const char *file) {   // file: the name its docs give ("mod\computer.eeai")
    struct lsp_scan s;
    memset(&s, 0, sizeof s);
    size_t len;
    int comment_from = -1;   // the first line of the comment just above
    for (int i = 0;; i++) {
        const char *l = lsp_line(text, i, &len);
        if (!l) break;
        int top = !s.ns && !s.depth;
        lsp_scan_line(&s, l, len);
        if (!top) { comment_from = -1; continue; }
        if (len >= 2 && l[0] == '/' && l[1] == '/') { if (comment_from < 0) comment_from = i; continue; }
        char name[48] = "", detail[160] = "";
        int kind = 0;
        size_t k = 0, j = 0;
        if (len > 4 && !strncmp(l, "let ", 4)) {
            for (j = 4; j < len && (l[j] == ' ' || l[j] == '\t'); j++) ;
            while (j < len && lsp_wc(l[j]) && k < 47) name[k++] = l[j++];
            kind = 5;   // Field
            snprintf(detail, sizeof detail, "a field (each AI its own, saved) - %s", file);
        } else {
            while (j < len && lsp_wc(l[j]) && k < 47) name[k++] = l[j++];
            name[k] = 0;
            if (j < len && l[j] == '(') {
                size_t e = j;
                while (e < len && l[e] != ')') e++;
                kind = 3;   // Function
                snprintf(detail, sizeof detail, "%s%.*s - %s", name, (int)(e < len ? e - j + 1 : e - j), l + j, file);
            } else {
                while (j < len && (l[j] == ' ' || l[j] == '\t')) j++;
                if (j < len && l[j] == '=' && (j + 1 >= len || l[j + 1] != '=')) {
                    int arrow = j + 1 < len && l[j + 1] == '>';
                    size_t v = j + (arrow ? 2 : 1);
                    while (v < len && l[v] == ' ') v++;
                    if (!arrow && v < len && l[v] == '{' && g_x_host.code_record(name)) { comment_from = -1; continue; }   // lines = { ...
                    kind = arrow ? 6 : 21;   // Variable (worked out) / Constant (data)
                    snprintf(detail, sizeof detail, "%s - %s", arrow ? "worked out where it is used" : "data", file);
                }
            }
        }
        name[k] = 0;
        if (!kind || !k || g_lsp_nnames > 2000) { comment_from = -1; continue; }
        if (g_lsp_nnames == g_lsp_names_cap) {
            int c = g_lsp_names_cap ? g_lsp_names_cap * 2 : 128;
            struct lsp_name *t = realloc(g_lsp_names, sizeof *t * (size_t)c);
            if (!t) return;
            g_lsp_names = t; g_lsp_names_cap = c;
        }
        struct lsp_name *nm = &g_lsp_names[g_lsp_nnames++];
        lstrcpyA(nm->name, name);
        lstrcpyA(nm->detail, detail);
        nm->kind = kind;
        struct lsb d = { 0 };   // the comment above it, then the line itself
        for (int c = comment_from; c >= 0 && c < i; c++) {
            size_t cl;
            const char *cs = lsp_line(text, c, &cl);
            if (!cs) break;
            size_t o = cl > 2 && cs[2] == ' ' ? 3 : 2;
            lsb_put(&d, cs + o, cl - o);
            lsb_str(&d, "\n");
        }
        lsb_str(&d, "\n```eeai\n");
        lsb_put(&d, l, len < 400 ? len : 400);
        lsb_str(&d, "\n```");
        nm->doc = d.s;
        comment_from = -1;
    }
}
// the names of path and of its layers: in a personality folder mod\computer.eeai, mod\prompt.eeai, mod\system.eeai
// and its own include(...)s; a script its own only
internal void lsp_names_load(const char *path, const char *text) {
    lsp_names_clear();
    char scope[MAX_PATH];
    lsp_scope(path, scope);
    lsp_names_of(text, "this file");
    if (!lstrcmpiA(scope, path)) return;
    static const char *const layers[] = { "mod\\computer.eeai", "mod\\prompt.eeai", "mod\\system.eeai" };
    char p[MAX_PATH];
    for (int i = 0; i < 3; i++) {
        snprintf(p, sizeof p, "%s\\%s", scope, layers[i]);
        if (!lstrcmpiA(p, path)) continue;
        char *t = lsp_file_text(p);
        if (t) { lsp_names_of(t, layers[i]); free(t); }
    }
    for (const char *inc = strstr(text, "include("); inc; inc = strstr(inc + 8, "include(")) {   // include("a.eeai", ...)
        const char *e = strchr(inc, ')');
        for (const char *q = strchr(inc, '"'); q && e && q < e; ) {
            const char *qe = strchr(q + 1, '"');
            if (!qe || qe > e) break;
            char f[MAX_PATH];
            snprintf(f, sizeof f, "%.*s", (int)(qe - q - 1), q + 1);
            for (char *c = f; *c; c++) if (*c == '/') *c = '\\';
            snprintf(p, sizeof p, "%s\\%s", scope, f);
            char *t = lsp_file_text(p);
            if (t) { lsp_names_of(t, f); free(t); }
            q = strchr(qe + 1, '"');
        }
    }
}

// THE ITEMS of an answer
struct lsp_items { struct lsb b; int n; };
internal void lsp_item(struct lsp_items *it, const char *label, int kind, const char *detail, const char *doc, const char *insert, int snippet, const char *sort) {
    lsb_str(&it->b, it->n++ ? ",{\"label\":" : "{\"label\":");
    lsb_json(&it->b, label);
    char k[32];
    snprintf(k, sizeof k, ",\"kind\":%d", kind);
    lsb_str(&it->b, k);
    if (detail && *detail) { lsb_str(&it->b, ",\"detail\":"); lsb_json(&it->b, detail); }
    if (doc && *doc) { lsb_str(&it->b, ",\"documentation\":{\"kind\":\"markdown\",\"value\":"); lsb_json(&it->b, doc); lsb_str(&it->b, "}"); }
    if (insert) { lsb_str(&it->b, ",\"insertText\":"); lsb_json(&it->b, insert); }
    if (snippet) lsb_str(&it->b, ",\"insertTextFormat\":2");
    if (sort) { lsb_str(&it->b, ",\"sortText\":"); lsb_json(&it->b, sort); }
    lsb_str(&it->b, "}");
}
internal const char *lsp_type_text(int t, char *out, int cap) {   // a type as the language writes it
    int base = X_TBASE(t), list = (t & X_LIST_OF) != 0;
    const char *n = base == XT_INT ? "int" : base == XT_FLOAT ? "float" : base == XT_BOOL ? "bool" : base == XT_TEXT ? "text" :
                    base == XT_VEC2 ? "vec2" : base == XT_VEC3 ? "vec3" : base == XT_VEC4 ? "vec4" : base == XT_EPOCH ? "epoch" :
                    base == XT_COLOR ? "color" : base == XT_UTYPE ? "unit type" : base == XT_TRIG ? "trigger" :
                    base == XK_PLAYER ? "player" : base == XK_UNIT ? "unit" : base == XK_GAME ? "game" : base == XK_SLOT ? "slot" : "";
    snprintf(out, (size_t)cap, "%s%s%s", list ? "list of " : "", n, *n && (t & X_NULLABLE) ? "?" : "");
    return out;
}

// a situation's description: mod\prompt.eeai's sit_<key> (what the language model is told of it), its chance,
// cooldown, audience and fallback
internal void lsp_sit_doc(const char *scope, int k, struct lsb *d) {
    char p[MAX_PATH], want[64];
    snprintf(p, sizeof p, "%s\\mod\\prompt.eeai", scope);
    snprintf(want, sizeof want, "sit_%s", k_lines[k].key);
    char *t = lsp_file_text(p);
    const char *s = t ? strstr(t, want) : NULL;
    while (s && lsp_wc(s[strlen(want)])) s = strstr(s + 1, want);
    if (s && (s = strchr(s, '"')) && strchr(s, '\n') > s) {
        const char *e = s + 1;
        int bd = 0;
        for (; *e && *e != '\n' && (bd || *e != '"'); e++) { if (*e == '\\' && e[1]) e++; else if (*e == '{') bd++; else if (*e == '}' && bd) bd--; }
        lsb_str(d, "When ");
        lsb_put(d, s + 1, (size_t)(e - s - 1));
        lsb_str(d, ".\n\n");
    }
    free(t);
    char m[300];
    int fb = line_fallback(k);
    if (k_lines[k].chance < 100) snprintf(m, sizeof m, "Said %d%% of the time at talk = 50%%", k_lines[k].chance);
    else lstrcpyA(m, "Always said");
    lsb_str(d, m);
    snprintf(m, sizeof m, ", heard by %s.", k_lines[k].aud == A_ALL ? "everyone" : k_lines[k].aud == A_ALLIES ? "its allies" : "the player it is about ({them}) only");
    lsb_str(d, m);
    if (k_lines[k].cooldown) { snprintf(m, sizeof m, " At most every %d s.", k_lines[k].cooldown); lsb_str(d, m); }
    if (fb >= 0) { snprintf(m, sizeof m, " Without a line: %s.", k_lines[fb].key); lsb_str(d, m); }
}

// a builtin function's description: its piece of README.md's rows of functions ("| Functions | `chance(p)` - ...;
// `rnd()` - ...") - the README beside the personalities is the one place they are explained (it ships with them)
internal void lsp_fn_doc(const char *readme, const char *name, struct lsb *d) {
    char want[48];
    snprintf(want, sizeof want, "`%s(", name);
    for (const char *r = readme; r && (r = strstr(r, "\n| ")); r++) {
        const char *e = strchr(r + 1, '\n'), *c1 = strstr(r + 3, " | ");
        if (!e || !c1 || c1 > e || c1 - r < 11 || strncmp(c1 - 8, "unctions", 8)) continue;   // its first cell: ... Functions
        for (const char *p = c1 + 3; p < e; ) {   // its pieces, "; " between them
            const char *pe = strstr(p, "; ");
            if (!pe || pe > e) pe = e;
            const char *w = strstr(p, want);
            if (w && w < pe) {
                size_t n = (size_t)(pe - p);
                while (n && (p[n - 1] == '|' || p[n - 1] == ' ' || p[n - 1] == '\r')) n--;
                lsb_put(d, p, n);
                return;
            }
            p = pe + 2;
        }
    }
}
// a record's keys, written = or => as the reader takes them (persona.c x_code_arrow)
internal void lsp_key_item(struct lsp_items *it, const char *file, const char *rec, const char *key, const char *detail, const char *doc, const char *present) {
    char ins[96], look[64];
    int arrow = x_code_arrow(file, rec, key) == 1;
    snprintf(ins, sizeof ins, "%s %s ", key, arrow ? "=>" : "=");
    snprintf(look, sizeof look, " %s ", key);
    lsp_item(it, key, 10, detail, doc, ins, 0, strstr(present, look) ? "1" : "0");   // Property; the ones not written yet first
}
internal const char *const k_lsp_pers_doc[][2] = {
    { "name", "The player's name in the game - who it is, chosen once: a text, or a list in order of preference (the first no other player has). A loaded game finds the personality by it." },
    { "title", "A title before the name (\"Khan\" makes \"Khan Temur\") - worked out every 10 s from the game: `epoch < epoch.dark ? \"Chieftain\" : \"King\"`. A new title is the situation `title`." },
    { "civilization", "Its civilization's name as the game shows it - worked out every 10 s, like the title." },
    { "character", "For the AI voice ([llm]): who this is, in a few words - the model plays it." },
    { "color", "Favoured colours on random maps, the first free one wins: `[\"blue\", \"cyan\"]`, `color.red`, `rgb(128, 64, 0)`." },
    { "aggression", "0% to 100%: betrays, refuses peace" }, { "loyalty", "0% to 100%: keeps its alliances; a vassal under 50% rebels" },
    { "pride", "0% to 100%: how long it holds a grudge, fights on, serves" }, { "greed", "0% to 100%: what a tribute is worth to it" },
    { "forgiveness", "0% to 100%: how fast a grudge fades" }, { "caution", "0% to 100%: capitulates earlier, pays more" },
    { "talk", "0% to 100%: how often it says the lines that are not always said" },
};
internal const char *const k_lsp_rec_doc[][2] = {
    { "personality", "Who it is: name, title, civilization, colour, traits, character." },
    { "lines", "What it says: a line per situation, `greeting => [...]` - worked out each time it is said." },
    { "style", "ageofai.ini settings for this AI (ai_style=1): read once." },
    { "priorities", "What this AI saves for (ai_style=1): worked out every 10 s." },
    { "decide", "Its decisions as numbers: peace_score, seek_ally, ally_score - worked out at each." },
    { "prompt", "What its language model is told, instead of mod\\prompt.eeai's texts." },
};
internal const char *const k_lsp_decide_doc[D_N] = {
    "Whether it takes peace with them: a score, above 0 yes (opinion and more - mod\\computer.eeai's rule the base)",
    "Whether it looks for an ally now: above 0 yes",
    "Whom it asks: the score of them as an ally, the highest asked",
};
internal void lsp_keys(struct lsp_items *it, const char *text, const char *path, const char *rec) {
    char present[2000] = " ";   // the keys the record has already
    struct lsp_scan s;
    memset(&s, 0, sizeof s);
    size_t len;
    for (int i = 0;; i++) {
        const char *l = lsp_line(text, i, &len);
        if (!l) break;
        char key[48];
        if (s.ns == 1 && !s.depth && !lstrcmpiA(s.stack[0], rec) && lsp_key_at(l, len, key, NULL) && strlen(present) + strlen(key) + 2 < sizeof present) { lstrcatA(present, key); lstrcatA(present, " "); }
        lsp_scan_line(&s, l, len);
    }
    const char *file = strrchr(path, '\\') ? strrchr(path, '\\') + 1 : path;
    char scope[MAX_PATH];
    lsp_scope(path, scope);
    if (!lstrcmpiA(rec, "personality"))
        for (size_t i = 0; i < sizeof k_lsp_pers_doc / sizeof *k_lsp_pers_doc; i++) lsp_key_item(it, file, rec, k_lsp_pers_doc[i][0], i >= 5 ? "a trait" : "personality", k_lsp_pers_doc[i][1], present);
    else if (!lstrcmpiA(rec, "lines"))
        for (int k = 0; k < L_N; k++) {
            struct lsb d = { 0 };
            lsp_sit_doc(scope, k, &d);
            lsp_key_item(it, file, rec, k_lines[k].key, "a situation", d.s, present);
            free(d.s);
        }
    else if (!lstrcmpiA(rec, "style"))
        for (size_t i = 0; i < sizeof k_style_keys / sizeof *k_style_keys; i++) lsp_key_item(it, file, rec, k_style_keys[i].key, "an ageofai.ini setting", NULL, present);
    else if (!lstrcmpiA(rec, "priorities")) {
        for (size_t i = 0; i < sizeof g_prio / sizeof *g_prio; i++) lsp_key_item(it, file, rec, g_prio[i].name, "a row of the priority list: on while it holds", NULL, present);
        lsp_item(it, "b_", 10, "a building's identifier: how many it saves for (b_stable => 2)", NULL, "b_", 0, "2");
    } else if (!lstrcmpiA(rec, "decide"))
        for (int k = 0; k < D_N; k++) lsp_key_item(it, file, rec, k_decide_keys[k], "a decision", k_lsp_decide_doc[k], present);
    else if (!lstrcmpiA(rec, "prompt") || !lstrcmpiA(rec, "system")) {
        int sys = !lstrcmpiA(rec, "system");
        for (int i = 0; i < (sys ? N_SYS : N_TPL); i++) {
            const struct text_key *t = sys ? &k_sys[i] : &k_tpl[i];
            char det[200];
            snprintf(det, sizeof det, "%s%s%s", sys ? "a system text" : "a prompt text", *t->ph ? " - its placeholders: " : "", t->ph);
            lsp_key_item(it, file, rec, t->key, det, NULL, present);
        }
    }
}
internal void lsp_top(struct lsp_items *it) {   // a personality file's top: its records, let, include, the handlers
    for (size_t i = 0; i < sizeof k_lsp_rec_doc / sizeof *k_lsp_rec_doc; i++) {
        char ins[64];
        snprintf(ins, sizeof ins, "%s = {\n\t$0\n}", k_lsp_rec_doc[i][0]);
        lsp_item(it, k_lsp_rec_doc[i][0], 22, "a record", k_lsp_rec_doc[i][1], ins, 1, "0");   // Struct
    }
    lsp_item(it, "let", 14, "a field: each AI its own value, saved with the game", "`let slights = dict<player, int>(0)`", "let ${1:name} = ${0:0}", 1, "1");
    lsp_item(it, "include", 14, "a layer: its lines, names and keys after this file's own", NULL, "include(\"$0\")", 1, "1");
    lsp_item(it, "register_personality", 14, "this file is a personality (an AI may get it)", NULL, "register_personality()", 0, "1");
    for (int k = 0; k < L_N; k++) {
        char lab[64], ins[96];
        snprintf(lab, sizeof lab, "on_%s", k_lines[k].key);
        snprintf(ins, sizeof ins, "on_%s() {\n\t$0\n}", k_lines[k].key);
        lsp_item(it, lab, 23, "a handler: statements run each time the situation is raised", NULL, ins, 1, "2");   // Event
    }
}
// the kind a member chain gives (me, them, them.lord, player(2), a let field of a player...): XK_PLAYER, XK_GAME ..., 0
internal int lsp_base_kind(const char *base) {
    const char *last = strrchr(base, '.');
    last = last ? last + 1 : base;
    if (!strcmp(base, "game")) return XK_GAME;
    if (!strcmp(last, "lord") || !strcmp(last, "me") || !strcmp(last, "them") || !strcmp(last, "enemy") || !strcmp(last, "ally") ||
        !strcmp(last, "owner") || !strcmp(last, "darling") || !strcmp(last, "nemesis") || !strncmp(last, "player", 6)) return XK_PLAYER;
    for (int i = 0; i < g_lsp_nnames; i++)   // a field or name that holds a player: let x: player? = ...
        if (!strcmp(g_lsp_names[i].name, last) && g_lsp_names[i].doc && strstr(g_lsp_names[i].doc, ": player")) return XK_PLAYER;
    return 0;
}
internal void lsp_members(struct lsp_items *it, const char *base) {
    int kind = lsp_base_kind(base);
    char ty[48];
    if (kind == XK_PLAYER)
        for (int i = 0; i < N_XMEMBERS; i++) {
            const struct xmember *m = &k_xmembers[i];
            int call = m->kind == XM_REL || m->kind == XM_FILTER || m->kind == XM_DO;
            char det[96], ins[64];
            snprintf(det, sizeof det, "%s%s", call ? "a relation or method " : "", lsp_type_text(m->type, ty, sizeof ty));
            snprintf(ins, sizeof ins, call ? "%s($0)" : "%s", m->name);
            lsp_item(it, m->name, call ? 2 : 10, det, m->doc, ins, call, NULL);   // Method / Property
        }
    const struct x_kind *k = kind ? x_kind_of(kind) : NULL;
    for (int i = 0; kind != XK_PLAYER && k && i < k->n; i++) {
        char ins[64];
        snprintf(ins, sizeof ins, k->m[i].amin >= 0 ? "%s($0)" : "%s", k->m[i].name);
        lsp_item(it, k->m[i].name, k->m[i].amin >= 0 ? 2 : 10, lsp_type_text(k->m[i].result, ty, sizeof ty), NULL, ins, k->m[i].amin >= 0, NULL);
    }
    if (!strcmp(base, "epoch")) for (int i = 0; i < 15; i++) { char d[32]; snprintf(d, sizeof d, "epoch %d", i + 1); lsp_item(it, k_epoch_const[i], 20, d, NULL, NULL, 0, d + 6); }
    if (!strcmp(base, "color")) for (int i = 0; i < 17; i++) { char d[32]; snprintf(d, sizeof d, "colour %d", i + 1); lsp_item(it, k_color_names[i][0], 20, d, NULL, NULL, 0, NULL); }
    if (!strcmp(base, "resource")) { for (int i = 0; i < 5; i++) lsp_item(it, k_resource_const[i], 20, "a resource", NULL, NULL, 0, NULL); lsp_item(it, "all", 20, "every resource", NULL, NULL, 0, NULL); }
    if (!strcmp(base, "kind")) for (int i = 0; i < UNIT_KINDS; i++) if (k_kind_ident[i] && *k_kind_ident[i]) lsp_item(it, k_kind_ident[i], 20, "a kind of unit (the game's family, every epoch's)", NULL, NULL, 0, NULL);
    if (!strcmp(base, "word")) for (int i = 0; i < N_WORDS; i++) lsp_item(it, k_words[i], 21, "a word of language.eeai, in the game's language", NULL, NULL, 0, NULL);
}
internal void lsp_values(struct lsp_items *it, const struct lsp_at *at, const char *path) {
    char ty[48];
    for (int i = 0; i < N_XGLOBALS; i++) lsp_item(it, k_xglobals[i].name, 6, lsp_type_text(k_xglobals[i].type, ty, sizeof ty), k_xglobals[i].doc, NULL, 0, "1");
    lsp_item(it, "epoch", 6, "epoch", "the speaker's epoch: its name in a text, 1..15 as a number - compare it with epoch.middle", NULL, 0, "1");
    for (int t = 0; t < T_N; t++) lsp_item(it, k_trait_keys[t], 6, "a trait of the speaker (0% to 100%)", NULL, NULL, 0, "3");
    char scope[MAX_PATH], rp[MAX_PATH];
    lsp_scope(path, scope);
    snprintf(rp, sizeof rp, "%s\\README.md", scope);
    char *readme = lstrcmpiA(scope, path) ? lsp_file_text(rp) : NULL;
    for (int i = 0; i < N_X_FNS; i++) {
        if (i && !strcmp(k_x_fns[i].name, k_x_fns[i - 1].name)) continue;
        char det[64];
        if (k_x_fns[i].lo == k_x_fns[i].hi) snprintf(det, sizeof det, "a function: %d argument%s", k_x_fns[i].lo, k_x_fns[i].lo == 1 ? "" : "s");
        else snprintf(det, sizeof det, "a function: %d to %d arguments", k_x_fns[i].lo, k_x_fns[i].hi);
        char ins[48];
        snprintf(ins, sizeof ins, "%s($0)", k_x_fns[i].name);
        struct lsb fd = { 0 };
        lsp_fn_doc(readme, k_x_fns[i].name, &fd);
        lsp_item(it, k_x_fns[i].name, 3, det, fd.s, ins, 1, "2");
        free(fd.s);
    }
    free(readme);
    static const char *const prefixes[] = { "epoch.", "color.", "resource.", "kind.", "unit.", "word.", "game." };
    for (size_t i = 0; i < sizeof prefixes / sizeof *prefixes; i++) {
        lsp_item(it, prefixes[i], 9, i == 4 ? "unit.<identifier>: a unit type (docs/units.md; the game's data checks them)" : "constants", NULL, NULL, 0, "3");   // Module
    }
    lsp_item(it, "true", 14, NULL, NULL, NULL, 0, "4");
    lsp_item(it, "false", 14, NULL, NULL, NULL, 0, "4");
    lsp_item(it, "null", 14, NULL, NULL, NULL, 0, "4");
    for (int i = 0; i < g_lsp_nnames; i++) lsp_item(it, g_lsp_names[i].name, g_lsp_names[i].kind, g_lsp_names[i].detail, g_lsp_names[i].doc, NULL, 0, "0");
    const char *ph = at->rec[0] ? x_code_placeholders(path, at->rec, at->key) : NULL;   // the key's placeholders (tribute, ally ...)
    for (const char *w = ph; w && *w; ) {
        while (*w == ' ') w++;
        size_t n = strcspn(w, " ");
        if (!n) break;
        char nm[48];
        snprintf(nm, sizeof nm, "%.*s", (int)(n < 47 ? n : 47), w);
        lsp_item(it, nm, 6, "a placeholder the code fills for this key", NULL, NULL, 0, "0");
        w += n;
    }
    if (at->script) {   // a script: its own functions and the pipe's commands
        for (int i = 0; i < N_X_SCRIPT_FNS; i++) {
            char ins[48];
            snprintf(ins, sizeof ins, "%s($0)", k_x_script_fns[i].n);
            lsp_item(it, k_x_script_fns[i].n, 3, "a script's function (docs/scripts.md)", NULL, ins, 1, "1");
        }
        for (size_t i = 0; i < sizeof k_cmd_verbs / sizeof *k_cmd_verbs; i++) {
            char ins[48];
            snprintf(ins, sizeof ins, "%s($0)", k_cmd_verbs[i]);
            lsp_item(it, k_cmd_verbs[i], 3, "a command (the control pipe's: docs/testing.md)", NULL, ins, 1, "3");
        }
    }
}
// textDocument/completion: the items for the place - JSON's result
internal void lsp_complete(int d, int line, int ch, struct lsb *out) {
    struct lsp_at at;
    lsp_where(g_lsp_doc[d].text, line, ch, &at);
    char scope[MAX_PATH];
    lsp_scope(g_lsp_doc[d].path, scope);
    at.script = !lstrcmpiA(scope, g_lsp_doc[d].path);
    struct lsp_items it = { 0 };
    lsb_str(&it.b, "");
    if (at.kind != LW_NONE) lsp_names_load(g_lsp_doc[d].path, g_lsp_doc[d].text);
    if (at.kind == LW_KEY) lsp_keys(&it, g_lsp_doc[d].text, g_lsp_doc[d].path, at.rec);
    else if (at.kind == LW_TOP && !at.script) lsp_top(&it);
    else if (at.kind == LW_MEMBER) lsp_members(&it, at.base);
    else if (at.kind == LW_VALUE || at.kind == LW_TOP) lsp_values(&it, &at, g_lsp_doc[d].path);
    lsb_str(out, "\"result\":{\"isIncomplete\":false,\"items\":[");
    lsb_str(out, it.b.s ? it.b.s : "");
    lsb_str(out, "]}");
    free(it.b.s);
}
// textDocument/hover: what the word at the place is - JSON's result (null: nothing known)
internal void lsp_hover(int d, int line, int ch, struct lsb *out) {
    const char *text = g_lsp_doc[d].text;
    size_t len;
    const char *l = lsp_line(text, line, &len);
    size_t at_b = l ? lsp_col(l, len, ch) : 0, b = at_b, e = at_b;
    while (l && b && lsp_wc(l[b - 1])) b--;
    while (l && e < len && lsp_wc(l[e])) e++;
    if (!l || b == e) { lsb_str(out, "\"result\":null"); return; }
    char word[64];
    snprintf(word, sizeof word, "%.*s", (int)(e - b < 63 ? e - b : 63), l + b);
    int ech = 0;   // the word's end as a column
    for (size_t i = 0; i < e; ) { unsigned char c = (unsigned char)l[i]; int n = c < 0x80 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4; ech += n == 4 ? 2 : 1; i += (size_t)n; }
    struct lsp_at at;
    lsp_where(text, line, ech, &at);
    char scope[MAX_PATH];
    lsp_scope(g_lsp_doc[d].path, scope);
    lsp_names_load(g_lsp_doc[d].path, text);
    struct lsb h = { 0 };
    char ty[48], m[400];
    if (at.kind == LW_KEY && !lstrcmpiA(at.rec, "lines")) {
        for (int k = 0; k < L_N; k++) if (!strcmp(k_lines[k].key, word)) { snprintf(m, sizeof m, "**%s** - a situation\n\n", word); lsb_str(&h, m); lsp_sit_doc(scope, k, &h); }
    } else if (at.kind == LW_KEY && !lstrcmpiA(at.rec, "personality")) {
        for (size_t i = 0; i < sizeof k_lsp_pers_doc / sizeof *k_lsp_pers_doc; i++) if (!strcmp(k_lsp_pers_doc[i][0], word)) { snprintf(m, sizeof m, "**%s**\n\n%s", word, k_lsp_pers_doc[i][1]); lsb_str(&h, m); }
    } else if (at.kind == LW_KEY && !lstrcmpiA(at.rec, "decide")) {
        for (int k = 0; k < D_N; k++) if (!strcmp(k_decide_keys[k], word)) { snprintf(m, sizeof m, "**%s** - a decision\n\n%s", word, k_lsp_decide_doc[k]); lsb_str(&h, m); }
    } else if (at.kind == LW_TOP) {
        if (!strncmp(word, "on_", 3))
            for (int k = 0; k < L_N; k++) if (!strcmp(k_lines[k].key, word + 3)) { snprintf(m, sizeof m, "**%s** - runs each time **%s** is raised\n\n", word, word + 3); lsb_str(&h, m); lsp_sit_doc(scope, k, &h); }
        for (size_t i = 0; i < sizeof k_lsp_rec_doc / sizeof *k_lsp_rec_doc; i++) if (!strcmp(k_lsp_rec_doc[i][0], word)) { snprintf(m, sizeof m, "**%s** - %s", word, k_lsp_rec_doc[i][1]); lsb_str(&h, m); }
    } else if (at.kind == LW_MEMBER && lsp_base_kind(at.base) == XK_PLAYER) {
        for (int i = 0; i < N_XMEMBERS; i++) if (!strcmp(k_xmembers[i].name, word)) { snprintf(m, sizeof m, "**%s.%s**: %s\n\n%s", at.base, word, lsp_type_text(k_xmembers[i].type, ty, sizeof ty), k_xmembers[i].doc); lsb_str(&h, m); }
    }
    if (!h.n && at.kind != LW_MEMBER) {
        for (int i = 0; i < N_XGLOBALS && !h.n; i++) if (!strcmp(k_xglobals[i].name, word)) { snprintf(m, sizeof m, "**%s**: %s\n\n%s", word, lsp_type_text(k_xglobals[i].type, ty, sizeof ty), k_xglobals[i].doc); lsb_str(&h, m); }
        for (int i = 0; i < N_X_FNS && !h.n; i++)
            if (!strcmp(k_x_fns[i].name, word)) {
                char rp[MAX_PATH];
                snprintf(rp, sizeof rp, "%s\\README.md", scope);
                char *readme = lstrcmpiA(scope, g_lsp_doc[d].path) ? lsp_file_text(rp) : NULL;
                struct lsb fd = { 0 };
                lsp_fn_doc(readme, word, &fd);
                snprintf(m, sizeof m, "**%s(...)** - a function of the language\n\n", word);
                lsb_str(&h, m);
                lsb_str(&h, fd.s ? fd.s : "(README.md \"Expressions\")");
                free(fd.s);
                free(readme);
            }
        for (int i = 0; i < g_lsp_nnames && !h.n; i++)
            if (!strcmp(g_lsp_names[i].name, word)) { snprintf(m, sizeof m, "**%s** - %s\n\n", word, g_lsp_names[i].detail); lsb_str(&h, m); lsb_str(&h, g_lsp_names[i].doc); }
    }
    if (!h.n) { lsb_str(out, "\"result\":null"); return; }
    lsb_str(out, "\"result\":{\"contents\":{\"kind\":\"markdown\",\"value\":");
    lsb_json(out, h.s);
    lsb_str(out, "}}");
    free(h.s);
}
