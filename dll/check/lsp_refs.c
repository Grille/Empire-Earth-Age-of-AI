// THE SERVER'S RECORD (stage 3 of the language server, check/lsp.c): go to a name's definition, find its uses, its
// type in the hover, and after a dot the members of what stands before it by its type - from a check of the scope
// with the record on (expr.c THE LANGUAGE SERVER'S RECORD: each name the check passes - where it is, what it is,
// where it is declared, its type), its lines read here. The record of all the names is made when first asked after
// a change (g_lsp_gen) - one check of the scope, as the mistakes' is. The type before a dot is a check of its own:
// of the text as it is without the dot and the word after it, so that what stands before is whole - in a script,
// failing that, of its line made "let __lsp_q = <what stands before the dot>" (a script is one tree: a line half
// typed leaves it none). A name of the files' tops (what 't': a text file's entries, functions, fields) is resolved
// here, by the files' names as completion knows them (lsp_names_load: the file's own first, then its layers' - the
// one the game would use for that file).

// A USE: path line col len what name type - dpath dline dcol where it is declared (when the check knows: a variable,
// a struct and its members), decl: this is the declaration. Lines from 0, columns UTF-16 units
struct lsp_use { const char *path, *dpath; int line, col, len, dline, dcol, decl; char what; char name[96], type[80]; };
internal struct lsp_use *g_lsp_use;
internal int g_lsp_nuse, g_lsp_use_cap;
internal unsigned g_lsp_gen, g_lsp_use_gen = ~0u;   // the texts' changes; the record's is of
internal char g_lsp_use_scope[MAX_PATH];
internal char **g_lsp_paths;   // the paths the records name, each once
internal int g_lsp_npaths;
internal const char *lsp_intern(const char *s) {
    if (!*s) return "";
    for (int i = 0; i < g_lsp_npaths; i++) if (!lstrcmpiA(g_lsp_paths[i], s)) return g_lsp_paths[i];
    char **t = realloc(g_lsp_paths, sizeof *t * (size_t)(g_lsp_npaths + 1));
    if (!t) return "";
    g_lsp_paths = t;
    return g_lsp_paths[g_lsp_npaths++] = _strdup(s);
}
internal int lsp_u16(const char *s, size_t n) {   // n bytes of UTF-8 as UTF-16 units
    int c = 0;
    for (size_t i = 0; i < n; i++) { unsigned char b = (unsigned char)s[i]; if ((b & 0xC0) != 0x80) c += b >= 0xF0 ? 2 : 1; }
    return c;
}
internal void lsp_uses_parse(char *out) {   // a record's @@U lines into g_lsp_use
    g_lsp_nuse = 0;
    for (char *l = out; l && *l; ) {
        char *e = strchr(l, '\n');
        if (e) *e = 0;
        size_t n = strlen(l);
        if (n && l[n - 1] == '\r') l[--n] = 0;
        if (!strncmp(l, "@@U\t", 4)) {
            char *f[12];
            int nf = 0;
            for (char *s = l + 4; s && nf < 12; ) { f[nf++] = s; if ((s = strchr(s, '\t')) != NULL) *s++ = 0; }
            if (nf == 11) {
                if (g_lsp_nuse == g_lsp_use_cap) {
                    int c = g_lsp_use_cap ? g_lsp_use_cap * 2 : 1024;
                    struct lsp_use *t = realloc(g_lsp_use, sizeof *t * (size_t)c);
                    if (!t) return;
                    g_lsp_use = t; g_lsp_use_cap = c;
                }
                struct lsp_use *u = &g_lsp_use[g_lsp_nuse++];
                u->path = lsp_intern(f[0]);
                u->line = atoi(f[1]); u->col = atoi(f[2]); u->len = atoi(f[3]);
                u->what = f[4][0];
                lstrcpynA(u->name, f[5], sizeof u->name);
                lstrcpynA(u->type, f[6], sizeof u->type);
                u->dpath = lsp_intern(f[7]);
                u->dline = atoi(f[8]); u->dcol = atoi(f[9]); u->decl = atoi(f[10]);
            }
        }
        l = e ? e + 1 : NULL;
    }
}
internal void lsp_uses_get(int d) {   // the record of d's scope, as the open files are now
    char scope[MAX_PATH];
    lsp_scope(g_lsp_doc[d].path, scope);
    if (g_lsp_use_gen == g_lsp_gen && !lstrcmpiA(scope, g_lsp_use_scope)) return;
    struct lsb out = { 0 };
    lsp_run(scope, "", &out);
    lsp_uses_parse(out.s);
    free(out.s);
    g_lsp_use_gen = g_lsp_gen;
    lstrcpyA(g_lsp_use_scope, scope);
}
internal const struct lsp_use *lsp_use_at(const char *path, int line, int ch) {   // the use at the place (the shortest there)
    const struct lsp_use *best = NULL;
    for (int i = 0; i < g_lsp_nuse; i++) {
        const struct lsp_use *u = &g_lsp_use[i];
        if (u->line != line || ch < u->col || ch > u->col + u->len || lstrcmpiA(u->path, path)) continue;
        if (!best || u->len < best->len || (u->len == best->len && *u->dpath && !*best->dpath)) best = u;
    }
    return best;
}
// the use at the place - else a struct's name where a type is written (let b: ledger, list<post>: text the tree has no
// node of): the struct's declaration
internal const struct lsp_use *lsp_use_here(int d, int line, int ch) {
    const struct lsp_use *u = lsp_use_at(g_lsp_doc[d].path, line, ch);
    size_t len;
    const char *l = u ? NULL : lsp_line(g_lsp_doc[d].text, line, &len);
    if (!l) return u;
    size_t b = lsp_col(l, len, ch), e = b;
    while (b && lsp_wc(l[b - 1])) b--;
    while (e < len && lsp_wc(l[e])) e++;
    for (int i = 0; i < g_lsp_nuse && b < e; i++) {
        const struct lsp_use *v = &g_lsp_use[i];
        if (v->what == 'S' && v->decl && strlen(v->name) == e - b && !strncmp(v->name, l + b, e - b) && !lstrcmpiA(v->path, g_lsp_doc[d].path)) return v;
    }
    return NULL;
}
// a place as LSP's Location - an open file by the editor's own URI for it
internal void lsp_loc(struct lsb *b, const char *path, int line, int col, int len) {
    int d = -1;
    for (int i = 0; i < g_lsp_ndoc && d < 0; i++) if (!lstrcmpiA(g_lsp_doc[i].path, path)) d = i;
    lsb_str(b, "{\"uri\":");
    if (d >= 0) lsb_json(b, g_lsp_doc[d].uri);
    else lsp_path_uri(path, b);
    char r[160];
    snprintf(r, sizeof r, ",\"range\":{\"start\":{\"line\":%d,\"character\":%d},\"end\":{\"line\":%d,\"character\":%d}}}", line, col, line, col + len);
    lsb_str(b, r);
}
internal int lsp_decl_len(const struct lsp_use *u) {   // its declaration's name's length: a member's after the struct's
    const char *dot = strrchr(u->name, '.');
    return (int)strlen(dot && u->what == 's' ? dot + 1 : u->name);
}
// a name of the files' tops used in file: where it is defined for that file (its own, else its layers')
struct lsp_def { const char *path; int line, col, len; };
internal int lsp_t_def(const char *file, const char *name, struct lsp_def *out) {
    static struct { const char *file; char name[96]; int ok; struct lsp_def d; } cache[64];
    static int nc;
    static unsigned gen = ~0u;
    if (gen != g_lsp_gen) { nc = 0; gen = g_lsp_gen; }
    for (int i = 0; i < nc; i++) if (cache[i].file == file && !strcmp(cache[i].name, name)) { *out = cache[i].d; return cache[i].ok; }
    char head[48];
    size_t hn = strcspn(name, ".");
    lstrcpynA(head, name, (int)(hn < 47 ? hn : 47) + 1);
    char *text = lsp_file_text(file);
    int ok = 0;
    if (text) {
        lsp_names_load(file, text);
        free(text);
        for (int i = 0; i < g_lsp_nnames && !ok; i++)
            if (!strcmp(g_lsp_names[i].name, head)) { out->path = lsp_intern(g_lsp_names[i].path); out->line = g_lsp_names[i].line; out->col = g_lsp_names[i].col; out->len = (int)strlen(head); ok = 1; }
    }
    if (nc < 64) { cache[nc].file = file; lstrcpynA(cache[nc].name, name, sizeof cache[nc].name); cache[nc].ok = ok; if (ok) cache[nc].d = *out; nc++; }
    return ok;
}
// whether v is a use of what u is: a variable or a struct's member by its declaration, a member of a kind or a
// function by its name, a name of the files' tops by where it is defined for each file
internal int lsp_same(const struct lsp_use *u, const struct lsp_use *v, const struct lsp_def *td) {
    if (u->what != v->what) return 0;
    if (u->what == 'v' || u->what == 's' || u->what == 'S') {
        if (*u->dpath && *v->dpath) return u->dpath == v->dpath && u->dline == v->dline && u->dcol == v->dcol;
        return u->path == v->path && !strcmp(u->name, v->name);
    }
    if (strcmp(u->name, v->name)) return 0;
    if (u->what != 't' || !td) return 1;
    struct lsp_def vd;
    return lsp_t_def(v->path, v->name, &vd) && vd.path == td->path && vd.line == td->line && vd.col == td->col;
}

// textDocument/definition: where the name at the place is declared - null not known (the game's own, a member of its
// objects: no file has it)
internal void lsp_definition(int d, int line, int ch, struct lsb *out) {
    lsp_uses_get(d);
    const struct lsp_use *u = lsp_use_here(d, line, ch);
    struct lsp_def td;
    if (u && *u->dpath) { lsb_str(out, "\"result\":"); lsp_loc(out, u->dpath, u->dline, u->dcol, lsp_decl_len(u)); }
    else if (u && u->what == 't' && lsp_t_def(u->path, u->name, &td)) { lsb_str(out, "\"result\":"); lsp_loc(out, td.path, td.line, td.col, td.len); }
    else lsb_str(out, "\"result\":null");
}
// textDocument/references: every use of the name at the place in the scope (the declaration too: incl)
struct lsp_locs { struct lsp_def *l; int n, cap; };
internal void lsp_locs_add(struct lsp_locs *s, const char *path, int line, int col, int len) {   // each place once
    for (int i = 0; i < s->n; i++) if (s->l[i].path == path && s->l[i].line == line && s->l[i].col == col) return;
    if (s->n == s->cap) {
        int c = s->cap ? s->cap * 2 : 64;
        struct lsp_def *t = realloc(s->l, sizeof *t * (size_t)c);
        if (!t) return;
        s->l = t; s->cap = c;
    }
    s->l[s->n++] = (struct lsp_def){ path, line, col, len };
}
// a struct's name in its script's text, where the record has none: written as a type (b: ledger, list<post>) - each
// whole word outside strings and comments (no variable may have a struct's name)
internal void lsp_struct_words(const struct lsp_use *u, struct lsp_locs *s) {
    char *text = lsp_file_text(u->path);
    size_t wl = strlen(u->name);
    int line = 0;
    for (const char *p = text, *ls = text; p && *p; p++) {
        if (*p == '\n') { line++; ls = p + 1; continue; }
        if (p[0] == '/' && p[1] == '/') { while (p[1] && p[1] != '\n') p++; continue; }
        if (*p == '"' || *p == '\'') { char q = *p; for (p++; *p && *p != q && *p != '\n'; p++) if (*p == '\\' && p[1]) p++; if (!*p || *p == '\n') p--; continue; }
        if ((p == text || !lsp_wc(p[-1])) && !strncmp(p, u->name, wl) && !lsp_wc(p[wl])) lsp_locs_add(s, u->path, line, lsp_u16(ls, (size_t)(p - ls)), (int)wl);
    }
    free(text);
}
internal void lsp_references(int d, int line, int ch, int incl, struct lsb *out) {
    lsp_uses_get(d);
    const struct lsp_use *u = lsp_use_here(d, line, ch);
    if (!u) { lsb_str(out, "\"result\":null"); return; }
    struct lsp_use me = *u;   // (lsp_t_def loads names: g_lsp_use stays, but keep it apart)
    struct lsp_def td;
    struct lsp_locs s = { 0 };
    int tdok = me.what == 't' && lsp_t_def(me.path, me.name, &td);
    if (incl && tdok) lsp_locs_add(&s, td.path, td.line, td.col, td.len);
    for (int i = 0; i < g_lsp_nuse; i++) {
        const struct lsp_use *v = &g_lsp_use[i];
        if ((incl || !v->decl) && lsp_same(&me, v, tdok ? &td : NULL)) lsp_locs_add(&s, v->path, v->line, v->col, v->len);
    }
    if (me.what == 'S' && *me.dpath) {   // the struct's name written as a type: its declaration's file's text
        struct lsp_use w = me;
        w.path = me.dpath;
        lsp_struct_words(&w, &s);
        for (int i = 0; !incl && i < s.n; i++) if (s.l[i].path == me.dpath && s.l[i].line == me.dline && s.l[i].col == me.dcol) s.l[i] = s.l[--s.n];
    }
    lsb_str(out, "\"result\":[");
    for (int i = 0; i < s.n; i++) { if (i) lsb_str(out, ","); lsp_loc(out, s.l[i].path, s.l[i].line, s.l[i].col, s.l[i].len); }
    lsb_str(out, "]");
    free(s.l);
}
// what the record knows of the name at the place, for the hover: its kind's char ('v' a variable, 's' a struct's
// member, 'S' a struct, 'm' a member of the game's objects, 'f' the language's or the game's, 't' the files'), 0 none
internal int lsp_use_hover(int d, int line, int ch, struct lsb *h) {
    lsp_uses_get(d);
    const struct lsp_use *u = lsp_use_here(d, line, ch);
    if (!u) return 0;
    char m[600], ty[96] = "";
    const char *type = u->type;
    for (int i = 0; i < g_lsp_nuse && !*type && *u->dpath; i++)   // a declaration: its type as its uses have it
        if (g_lsp_use[i].type[0] && lsp_same(u, &g_lsp_use[i], NULL)) type = g_lsp_use[i].type;
    if (*type) snprintf(ty, sizeof ty, ": %s", type);
    const char *dot = strrchr(u->name, '.');
    switch (u->what) {
    case 'v':
        if (u->decl) snprintf(m, sizeof m, "**%s**%s - declared here", u->name, ty);
        else if (*u->dpath) snprintf(m, sizeof m, "**%s**%s - a variable (its declaration: line %d%s)", u->name, ty, u->dline + 1, u->dpath == u->path ? "" : " of another file");
        else snprintf(m, sizeof m, "**%s**%s - a variable", u->name, ty);
        break;
    case 's': snprintf(m, sizeof m, "**%s**%s - a member of struct %.*s (line %d)", dot ? dot + 1 : u->name, ty, dot ? (int)(dot - u->name) : 0, u->name, u->dline + 1); break;
    case 'S': snprintf(m, sizeof m, "**%s** - a struct of this script (line %d)", u->name, u->dline + 1); break;
    case 'm': {
        snprintf(m, sizeof m, "**%s**%s", u->name, ty);
        if (!strncmp(u->name, "player.", 7))
            for (int i = 0; i < N_XMEMBERS; i++) if (!strcmp(k_xmembers[i].name, u->name + 7)) { lstrcatA(m, "\n\n"); lstrcpynA(m + strlen(m), k_xmembers[i].doc, (int)(sizeof m - strlen(m))); }
        break;
    }
    default: {
        int cmd = 0;
        for (size_t i = 0; i < sizeof k_cmd_verbs / sizeof *k_cmd_verbs && !cmd; i++) cmd = !strcmp(k_cmd_verbs[i], u->name);
        snprintf(m, sizeof m, "**%s**%s%s", u->name, ty, u->what != 'f' ? "" : x_script_fn(u->name) >= 0 ? " - a script's function (docs/scripts.md)" :
                 cmd ? " - a command (the control pipe's: docs/testing.md)" : "");
    }
    }
    lsb_str(h, m);
    return u->what;
}

// AFTER A DOT: the members of what stands before it, by its type - a check of the text without the dot and the word
// after it, asked for the type that ends where the dot was. 0: no type known there (completion's own guess then)
internal int lsp_q_members(int d, const char *text, size_t n, int line, int col, struct lsp_items *it) {
    char *keep = g_lsp_doc[d].text, q[MAX_PATH + 48], scope[MAX_PATH];
    size_t keep_n = g_lsp_doc[d].n;
    g_lsp_doc[d].text = (char *)text;
    g_lsp_doc[d].n = n;
    snprintf(q, sizeof q, "%s\t%d\t%d", g_lsp_doc[d].path, line, col);
    lsp_scope(g_lsp_doc[d].path, scope);
    struct lsb out = { 0 };
    lsp_run(scope, q, &out);
    g_lsp_doc[d].text = keep;
    g_lsp_doc[d].n = keep_n;
    int got = 0, player = 0;
    for (char *l = out.s; l && *l; ) {
        char *e = strchr(l, '\n');
        if (e) *e = 0;
        size_t ln = strlen(l);
        if (ln && l[ln - 1] == '\r') l[--ln] = 0;
        if (!strncmp(l, "@@T\t", 4)) player = !strcmp(l + 4, "player") || !strcmp(l + 4, "player?");
        if (!strncmp(l, "@@M\t", 4) && !player) {   // name, m|v, type, kind
            char *f[4];
            int nf = 0;
            for (char *s = l + 4; s && nf < 4; ) { f[nf++] = s; if ((s = strchr(s, '\t')) != NULL) *s++ = 0; }
            if (nf == 4) {
                int call = f[1][0] == 'm';
                char det[160], ins[64];
                snprintf(det, sizeof det, "%s%s%s (%s)", call ? "a method" : "", call && *f[2] ? " - " : "", f[2], f[3]);
                snprintf(ins, sizeof ins, "%s($0)", f[0]);
                lsp_item(it, f[0], call ? 2 : 10, det, NULL, call ? ins : NULL, call, NULL);   // Method / Property
                got++;
            }
        }
        l = e ? e + 1 : NULL;
    }
    free(out.s);
    if (player) { lsp_members(it, "them"); got++; }   // a player: completion's list (the relations, the docs)
    return got;
}
internal int lsp_typed_members(int d, int line, int ch, int script, struct lsp_items *it) {
    const char *text = g_lsp_doc[d].text;
    size_t len;
    const char *l = lsp_line(text, line, &len);
    if (!l) return 0;
    size_t b = lsp_col(l, len, ch), w = b, we = b;
    while (w && lsp_wc(l[w - 1])) w--;
    while (we < len && lsp_wc(l[we])) we++;
    if (!w || l[w - 1] != '.') return 0;
    size_t dot = w - 1, lo = (size_t)(l - text), n = g_lsp_doc[d].n;
    char *cut = malloc(n + 1);   // the text without the dot and the word after it
    if (!cut) return 0;
    memcpy(cut, text, lo + dot);
    memcpy(cut + lo + dot, l + we, n - (lo + we));
    cut[n - (we - dot)] = 0;
    int got = lsp_q_members(d, cut, n - (we - dot), line, lsp_u16(l, dot), it);
    free(cut);
    if (got || !script) return got;
    size_t ps = dot;   // a script: its line made a let of what stands before the dot - a name, calls, items and members
    for (int guard = 0; guard < 64; guard++) {
        if (ps && (l[ps - 1] == ')' || l[ps - 1] == ']')) {
            int depth = 0;
            size_t q = ps;
            while (q) { char c = l[--q]; if (c == ')' || c == ']') depth++; else if ((c == '(' || c == '[') && !--depth) break; }
            if (depth) return 0;
            ps = q;
            continue;
        }
        size_t q = ps;
        while (q && lsp_wc(l[q - 1])) q--;
        if (q == ps) break;
        ps = q;
        if (ps && l[ps - 1] == '.') { ps--; continue; }
        break;
    }
    size_t ind = 0;
    while (ind < len && (l[ind] == ' ' || l[ind] == '\t')) ind++;
    if (ps >= dot || ps < ind) return 0;
    static const char let[] = "let __lsp_q = ";
    size_t nl = ind + sizeof let - 1 + (dot - ps), rest = n - (lo + len), m = lo + nl + rest;
    char *t = malloc(m + 1);
    if (!t) return 0;
    memcpy(t, text, lo + ind);
    memcpy(t + lo + ind, let, sizeof let - 1);
    memcpy(t + lo + ind + sizeof let - 1, l + ps, dot - ps);
    memcpy(t + lo + nl, l + len, rest);
    t[m] = 0;
    got = lsp_q_members(d, t, m, line, lsp_u16(t + lo, nl), it);
    free(t);
    return got;
}
