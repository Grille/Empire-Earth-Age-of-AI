// THE TEXT FILES (<game>\aimod_ai\*.ini: the personalities, _system.ini, _template.ini) and the EXPRESSIONS in them.
// A file: sections [name] / [name:<language>], "key = value" lines, comments from ; or # to the line's end (outside a
// string). A value is
//   "text"                        a string; \" \\ \n escapes. {placeholders} in it - {name} - or expressions -
//                                 {opinion > 30 ? "friend" : "stranger"}: the quotes inside the braces are the
//                                 expression's, not the string's end
//   [ "a", "b" if <expression> ]  a list, over as many lines as it likes (a comma after the last item is fine); an item
//                                 with "if" is said only while its condition holds - the items whose condition holds
//                                 are picked from, those without one only when none does
//   85 / opinion > 30 ? "a" : "b" a number or an expression, to the line's end ([vars], the traits)
// A mistake is logged with the file and line (# texts:) and that key is skipped.
// The expressions: numbers, "texts" (or 'texts', the same escapes), names (a placeholder of the text, a [vars] entry,
// a game value - x_builtin in diplo.c), ( ), + - * / %, < <= > >= == != (= is ==), and or not (&& || !), a ? b : c,
// and the functions pick(a, b, ...) (one at random), chance(n) (true n % of the time), min, max, abs, round.
// A value has a number, a text or both: the epoch is 3 and "Copper Age", a placeholder "12" is 12 and "12". The number
// counts in arithmetic, in comparisons with a number and as true/false; the text where the value ends up in the text
// and where it meets a text ("Age: " + epoch). + adds when both have numbers, else joins the texts; comparisons
// compare numbers when both have one, else texts (case ignored) - an epoch's name is the game's language's, so
// conditions compare its number (epoch >= epoch.middle), never its name. epoch.prehistoric .. epoch.space are the
// epochs' numbers 1..15, the same in every language (a "." is part of a name). True: a number not 0, a text not
// empty. No loops, no assignments: a text can't hang the game. A placeholder that is not a valid expression - or a
// single name nobody knows - stays in the text as written (logged); an unknown name inside an expression counts 0.
// A [vars] entry is a string (a text with placeholders) or an expression.
// Included by eeaimod.c before persona.c.
#define XVARS_MAX 48
#define XVAL_CAP 240
struct xvars { int n; char *key[XVARS_MAX], *val[XVARS_MAX]; uint8_t text[XVARS_MAX]; };   // text: a string, else an expression
struct xctx {
    void *pl, *other, *third;               // the speaker, {name}, {enemy} - NULL: found by the placeholders' names
    int found;                              // pl/other/third looked up (x_builtin)
    const char *const *pn, *const *pv;      // the text's placeholders
    int np;
    const struct xvars *vars[3];            // searched in order: the personality's, the file's
    int depth, level, unknown, bad, circle; // nesting: vars in vars, the parser (all together); unknown names; errors; too deep
};
struct xval { int has_n, has_s; double n; char s[XVAL_CAP]; };   // a number, a text or both
static struct xvars g_vars_sys, g_vars_tpl;   // _system.ini [vars] (+ [vars:<language>]), _template.ini [vars] (texts.c)
static int x_builtin(struct xctx *c, const char *name, struct xval *v);   // diplo.c: the game's values
static void x_decode(const char *name, struct xval *v);   // diplo.c: a placeholder's text that names a number (an epoch)
static int x_fill(char *out, int cap, const char *tmpl, struct xctx *c);
static int rnd(int n);   // persona.c: the display generator
static int g_x_logged;   // expression errors written to the log (at most 30)

static void xvars_set(struct xvars *v, const char *key, const char *val, int text) {
    int i = 0;
    while (i < v->n && lstrcmpiA(v->key[i], key)) i++;
    if (i == XVARS_MAX) return;
    if (i < v->n) free(v->val[i]);
    else { v->key[i] = _strdup(key); v->n++; }
    v->val[i] = _strdup(val);
    v->text[i] = (uint8_t)text;
}
static const char *xvars_get(const struct xvars *v, const char *key, int *text) {
    for (int i = 0; v && i < v->n; i++) if (!lstrcmpiA(v->key[i], key)) { *text = v->text[i]; return v->val[i]; }
    return NULL;
}

static int xnumeric(const char *s, double *n) {   // the whole text a number ("12", not " 12" or "12 ")
    char *e;
    if (!((*s >= '0' && *s <= '9') || *s == '-' || *s == '.')) return 0;
    *n = strtod(s, &e);
    return e != s && !*e;
}
static void xnum(struct xval *v, double n) { v->has_n = 1; v->has_s = 0; v->n = n; }
static void xstr(struct xval *v, const char *s) {   // a text - and its number when it is one ("12")
    lstrcpynA(v->s, s ? s : "", XVAL_CAP);
    v->has_s = 1;
    v->has_n = xnumeric(v->s, &v->n);
}
static void xdual(struct xval *v, double n, const char *s) { xstr(v, s); v->has_n = 1; v->n = n; }
static int xis_num(const struct xval *v, double *n) { if (v->has_n) *n = v->n; return v->has_n; }
static double xto_num(const struct xval *v) { return v->has_n ? v->n : 0; }
static int xtrue(const struct xval *v) { return v->has_n ? v->n != 0 : v->s[0] != 0; }
static void xto_text(const struct xval *v, char *out, int cap) {
    if (v->has_s) { lstrcpynA(out, v->s, cap); return; }
    double r = v->n < 0 ? -v->n : v->n;
    if (r - (double)(long long)r < 1e-9 && r < 1e15) snprintf(out, (size_t)cap, "%lld", (long long)v->n);
    else {
        snprintf(out, (size_t)cap, "%.2f", v->n);
        char *e = out + strlen(out);
        while (e > out && e[-1] == '0') *--e = 0;
        if (e > out && e[-1] == '.') e[-1] = 0;
    }
}

// the parser evaluates as it reads (both branches of ?: - only the chosen one counts)
struct xp { const char *s; struct xctx *c; int err; };
static void xp_space(struct xp *p) { while (*p->s == ' ' || *p->s == '\t') p->s++; }
static int xp_word(struct xp *p, const char *w) {   // a keyword (and, or, not), not the start of a longer name
    xp_space(p);
    size_t n = strlen(w);
    if (_strnicmp(p->s, w, n) || isalnum((unsigned char)p->s[n]) || p->s[n] == '_') return 0;
    p->s += n;
    return 1;
}
static int xp_op(struct xp *p, const char *op) {
    xp_space(p);
    size_t n = strlen(op);
    if (strncmp(p->s, op, n)) return 0;
    p->s += n;
    return 1;
}
static void xp_expr(struct xp *p, struct xval *v);
// epoch.<name>: an epoch's number, in every language the same (the names the game's own are in another language)
static const char *const k_epoch_const[15] = { "prehistoric", "stone", "copper", "bronze", "dark", "middle", "renaissance",
    "imperial", "industrial", "atomic_wwi", "atomic_wwii", "atomic_modern", "digital", "nano", "space" };
static int x_const(const char *name, struct xval *v) {
    if (_strnicmp(name, "epoch.", 6)) return 0;
    for (int e = 0; e < 15; e++) if (!lstrcmpiA(name + 6, k_epoch_const[e])) { xnum(v, e + 1); return 1; }
    return 0;
}
static void xp_ident(struct xp *p, const char *name, struct xval *v) {
    struct xctx *c = p->c;
    for (int i = 0; i < c->np; i++)
        if (!strcmp(c->pn[i], name)) { xstr(v, c->pv[i]); if (!v->has_n) x_decode(name, v); return; }
    for (int k = 0; k < 3; k++) {
        int text;
        const char *def = xvars_get(c->vars[k], name, &text);
        if (!def) continue;
        if (c->depth >= 6) { c->circle = 1; p->err = 1; return; }   // vars naming each other in a circle
        c->depth++;
        char t[XVAL_CAP];
        if (text) { x_fill(t, sizeof t, def, c); xstr(v, t); }   // a string: its placeholders filled
        else {
            struct xp q = { def, c, 0 };
            xp_expr(&q, v);
            xp_space(&q);
            if (q.err || *q.s) p->err = 1;
            else if (v->has_s && strchr(v->s, '{')) { x_fill(t, sizeof t, v->s, c); xstr(v, t); }   // a text it chose
        }
        if (c->circle) p->err = 1;
        c->depth--;
        return;
    }
    if (x_const(name, v) || x_builtin(c, name, v)) return;
    c->unknown++;
    xnum(v, 0);
}
static void xp_primary(struct xp *p, struct xval *v) {
    xp_space(p);
    const char *s = p->s;
    xnum(v, 0);
    if (++p->c->level > 24) { p->c->level--; p->err = 1; return; }
    if (*s == '(') {
        p->s++;
        xp_expr(p, v);
        if (!xp_op(p, ")")) p->err = 1;
    } else if (*s == '"' || *s == '\'') {
        char q = *s++, t[XVAL_CAP];
        int o = 0;
        for (; *s && *s != q; s++) {
            char ch = *s;
            if (ch == '\\' && s[1]) { ch = *++s; if (ch == 'n') ch = '\n'; }   // \" \' \\ \n
            if (o < XVAL_CAP - 1) t[o++] = ch;
        }
        if (!*s) { p->err = 1; goto out; }
        t[o] = 0;
        p->s = s + 1;
        xstr(v, t);
    } else if ((*s >= '0' && *s <= '9') || (*s == '.' && s[1] >= '0' && s[1] <= '9')) {
        char *e;
        xnum(v, strtod(s, &e));
        p->s = e;
    } else if (isalpha((unsigned char)*s) || *s == '_') {
        char name[40];
        int n = 0;
        while ((isalnum((unsigned char)*s) || *s == '_' || (*s == '.' && isalpha((unsigned char)s[1]))) && n < 39) name[n++] = *s++;
        name[n] = 0;
        p->s = s;
        if (!lstrcmpiA(name, "true")) { xnum(v, 1); goto out; }
        if (!lstrcmpiA(name, "false")) { xnum(v, 0); goto out; }
        xp_space(p);
        if (*p->s != '(') { xp_ident(p, name, v); goto out; }
        p->s++;   // a function: its arguments
        struct xval a[8];
        int na = 0;
        xp_space(p);
        if (*p->s == ')') p->s++;
        else for (;;) {
            struct xval tmp;
            xp_expr(p, na < 8 ? &a[na] : &tmp);
            if (na < 8) na++;
            if (p->err) goto out;
            if (xp_op(p, ",")) continue;
            if (!xp_op(p, ")")) p->err = 1;
            break;
        }
        if (p->err) goto out;
        if (!lstrcmpiA(name, "pick") && na) *v = a[rnd(na)];
        else if (!lstrcmpiA(name, "chance") && na == 1) xnum(v, rnd(100) < xto_num(&a[0]));
        else if ((!lstrcmpiA(name, "min") || !lstrcmpiA(name, "max")) && na) {
            int mx = !lstrcmpiA(name, "max");
            double r = xto_num(&a[0]);
            for (int i = 1; i < na; i++) { double x = xto_num(&a[i]); if (mx ? x > r : x < r) r = x; }
            xnum(v, r);
        } else if (!lstrcmpiA(name, "abs") && na == 1) { double x = xto_num(&a[0]); xnum(v, x < 0 ? -x : x); }
        else if (!lstrcmpiA(name, "round") && na == 1) { double x = xto_num(&a[0]); xnum(v, (double)(long long)(x < 0 ? x - 0.5 : x + 0.5)); }
        else p->err = 1;
    } else p->err = 1;
out:
    p->c->level--;
}
static void xp_unary(struct xp *p, struct xval *v) {
    if (xp_op(p, "-")) { xp_unary(p, v); xnum(v, -xto_num(v)); return; }
    if (xp_op(p, "!") || xp_word(p, "not")) { xp_unary(p, v); xnum(v, !xtrue(v)); return; }
    xp_primary(p, v);
}
static void xp_mul(struct xp *p, struct xval *v) {
    xp_unary(p, v);
    while (!p->err) {
        int op = xp_op(p, "*") ? '*' : xp_op(p, "/") ? '/' : xp_op(p, "%") ? '%' : 0;
        if (!op) return;
        struct xval b;
        xp_unary(p, &b);
        double x = xto_num(v), y = xto_num(&b);
        xnum(v, op == '*' ? x * y : y == 0 ? 0 : op == '/' ? x / y : x - y * (double)(long long)(x / y));
    }
}
static void xp_add(struct xp *p, struct xval *v) {
    xp_mul(p, v);
    while (!p->err) {
        int op = xp_op(p, "+") ? '+' : xp_op(p, "-") ? '-' : 0;
        if (!op) return;
        struct xval b;
        xp_mul(p, &b);
        double x, y;
        if (op == '-') xnum(v, xto_num(v) - xto_num(&b));
        else if (xis_num(v, &x) && xis_num(&b, &y)) xnum(v, x + y);
        else {   // texts joined
            char l[XVAL_CAP], r[XVAL_CAP], j[XVAL_CAP];
            xto_text(v, l, sizeof l);
            xto_text(&b, r, sizeof r);
            snprintf(j, sizeof j, "%s%s", l, r);
            xstr(v, j);
        }
    }
}
static void xp_cmp(struct xp *p, struct xval *v) {
    xp_add(p, v);
    if (p->err) return;
    int op = xp_op(p, "<=") ? 'l' : xp_op(p, ">=") ? 'g' : xp_op(p, "==") ? '=' : xp_op(p, "!=") ? '!' :
             xp_op(p, "<") ? '<' : xp_op(p, ">") ? '>' : xp_op(p, "=") ? '=' : 0;
    if (!op) return;
    struct xval b;
    xp_add(p, &b);
    double x, y;
    int d;
    if (xis_num(v, &x) && xis_num(&b, &y)) d = x < y ? -1 : x > y;
    else {
        char l[XVAL_CAP], r[XVAL_CAP];
        xto_text(v, l, sizeof l);
        xto_text(&b, r, sizeof r);
        d = lstrcmpiA(l, r);
    }
    xnum(v, op == '<' ? d < 0 : op == '>' ? d > 0 : op == 'l' ? d <= 0 : op == 'g' ? d >= 0 : op == '=' ? d == 0 : d != 0);
}
static void xp_and(struct xp *p, struct xval *v) {
    xp_cmp(p, v);
    while (!p->err && (xp_op(p, "&&") || xp_word(p, "and"))) {
        struct xval b;
        xp_cmp(p, &b);
        xnum(v, xtrue(v) && xtrue(&b));
    }
}
static void xp_or(struct xp *p, struct xval *v) {
    xp_and(p, v);
    while (!p->err && (xp_op(p, "||") || xp_word(p, "or"))) {
        struct xval b;
        xp_and(p, &b);
        xnum(v, xtrue(v) || xtrue(&b));
    }
}
static void xp_expr(struct xp *p, struct xval *v) {
    if (++p->c->level > 24) { p->c->level--; p->err = 1; return; }
    xp_or(p, v);
    if (!p->err && xp_op(p, "?")) {
        struct xval a, b;
        xp_expr(p, &a);
        if (!p->err && !xp_op(p, ":")) p->err = 1;
        if (!p->err) xp_expr(p, &b);
        if (!p->err) *v = xtrue(v) ? a : b;
    }
    p->c->level--;
}
// evaluate the whole of src (an expression and nothing after it); 0: not an expression
static int x_eval(const char *src, struct xctx *c, struct xval *v) {
    struct xp p = { src, c, 0 };
    xp_expr(&p, v);
    xp_space(&p);
    return !p.err && !*p.s;
}
static void x_log_bad(const char *what, const char *src) {
    if (g_log && g_x_logged < 30) { g_x_logged++; fprintf(g_log, "# texts: %s: %.120s\n", what, src); }
}

// the end of a {placeholder} that starts at s ('{'): quotes and nested braces skipped; NULL: none
static const char *x_close(const char *s) {
    int d = 0;
    for (; *s; s++) {
        if (*s == '"' || *s == '\'') {
            char q = *s;
            for (s++; *s && *s != q; s++) if (*s == '\\' && s[1]) s++;
            if (!*s) return NULL;
            continue;
        }
        if (*s == '{') d++;
        else if (*s == '}' && --d == 0) return s;
    }
    return NULL;
}
// out = tmpl with its {placeholders} filled: the text's own (exact name), else the expression's value; one that is
// not an expression, or a lone unknown name, stays as written. Returns the length.
static int x_fill(char *out, int cap, const char *tmpl, struct xctx *c) {
    int o = 0;
    for (const char *s = tmpl; *s && o < cap - 1;) {
        const char *close = *s == '{' ? x_close(s) : NULL;
        if (!close) { out[o++] = *s++; continue; }
        char src[400], val[XVAL_CAP];
        int n = (int)(close - s - 1);
        const char *rep = NULL;
        if (n > 0 && n < (int)sizeof src) {
            memcpy(src, s + 1, (size_t)n);
            src[n] = 0;
            for (int i = 0; i < c->np && !rep; i++) if (!strcmp(c->pn[i], src)) rep = c->pv[i];
            if (!rep && c->depth < 6) {
                struct xval v;
                int unk = c->unknown;
                c->depth++;
                int ok = x_eval(src, c, &v) && !c->circle;
                c->depth--;
                if (c->circle && !c->depth) { c->circle = 0; x_log_bad("[vars] naming each other in a circle", src); }
                int lone = ok && c->unknown > unk && strspn(src, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.") == (size_t)n;
                if (ok && !lone) { xto_text(&v, val, sizeof val); rep = val; }
                if (ok && !lone && c->unknown > unk) x_log_bad("unknown name (counted 0) in", src);
                else if (!ok && strpbrk(src, " <>=?\"'(+")) { c->bad++; x_log_bad("not an expression", src); }
            }
        }
        if (rep) { while (*rep && o < cap - 1) out[o++] = *rep++; s = close + 1; }
        else out[o++] = *s++;
    }
    out[o] = 0;
    return o;
}
// the file format
enum { TV_STRING, TV_NUMBER, TV_LIST, TV_EXPR };
#define TV_ITEMS 16
struct tval {
    int kind, line;
    char *text;                               // a string's text (escapes undone), an expression's / number's source
    double n;                                 // a number
    int nitems;
    char *item[TV_ITEMS], *cond[TV_ITEMS];    // a list: its strings and their conditions (NULL: none)
};
struct tlex { const char *s, *file; int line, err; };
static void tl_error(struct tlex *l, const char *what) {
    if (g_log && g_x_logged < 60) { g_x_logged++; fprintf(g_log, "# texts: %s:%d: %s - skipped\n", l->file, l->line, what); }
    l->err = 1;
}
static void tl_blank(struct tlex *l, int lines) {   // spaces; with lines: line breaks and comments too
    for (;;) {
        char c = *l->s;
        if (c == ' ' || c == '\t' || c == '\r') l->s++;
        else if (lines && c == '\n') { l->line++; l->s++; }
        else if (lines && (c == ';' || c == '#')) while (*l->s && *l->s != '\n') l->s++;
        else return;
    }
}
static void tl_line_end(struct tlex *l) {   // nothing but a comment may follow a value on its line
    tl_blank(l, 0);
    if (*l->s == ';' || *l->s == '#') while (*l->s && *l->s != '\n') l->s++;
    if (*l->s && *l->s != '\n') tl_error(l, "something after the value");
}
// a string at l->s (the opening quote): its text, malloc'd; NULL on a mistake
static char *tl_string(struct tlex *l) {
    size_t cap = 128, o = 0;
    char *r = malloc(cap), q = 0;
    int depth = 0;
    for (const char *s = l->s + 1;; s++) {
        char c = *s;
        if (o + 3 >= cap) r = realloc(r, cap *= 2);
        if (!c || c == '\n') { tl_error(l, depth ? "a { without its }" : "a string without its closing \""); free(r); return NULL; }
        if (depth) {   // in {braces}: an expression, kept as written - its quotes are its own
            if (q) { if (c == q) q = 0; else if (c == '\\' && s[1] && s[1] != '\n') r[o++] = *s++; }
            else if (c == '"' || c == '\'') q = c;
            else if (c == '{') depth++;
            else if (c == '}') depth--;
            r[o++] = *s;
            continue;
        }
        if (c == '"') { r[o] = 0; l->s = s + 1; return r; }
        if (c == '\\') {
            char e = s[1];
            if (e != 'n' && e != '"' && e != '\\') { tl_error(l, "an unknown escape (\\\" \\\\ \\n)"); free(r); return NULL; }
            r[o++] = e == 'n' ? '\n' : e;
            s++;
            continue;
        }
        if (c == '{') depth = 1;
        r[o++] = c;
    }
}
// an expression's source from l->s to the end of the line or (in a list) to a , or ] outside brackets and quotes
static char *tl_expr(struct tlex *l, int in_list) {
    const char *b = l->s, *s = b;
    int d = 0;
    char q = 0;
    for (; *s && *s != '\n'; s++) {
        if (q) { if (*s == q) q = 0; else if (*s == '\\' && s[1] && s[1] != '\n') s++; continue; }
        if (*s == '"' || *s == '\'') q = *s;
        else if (*s == '(' || *s == '[' || *s == '{') d++;
        else if ((*s == ')' || *s == ']' || *s == '}') && d) d--;
        else if (!d && (*s == ';' || *s == '#')) break;
        else if (!d && in_list && (*s == ',' || *s == ']')) break;
    }
    const char *e = s;
    while (e > b && (e[-1] == ' ' || e[-1] == '\t' || e[-1] == '\r')) e--;
    l->s = s;
    if (e == b) { tl_error(l, "an expression missing"); return NULL; }
    char *r = malloc((size_t)(e - b) + 1);
    memcpy(r, b, (size_t)(e - b));
    r[e - b] = 0;
    return r;
}
static void tval_free(struct tval *v) {
    free(v->text);
    for (int i = 0; i < v->nitems; i++) { free(v->item[i]); free(v->cond[i]); }
    memset(v, 0, sizeof *v);
}
static void tl_value(struct tlex *l, struct tval *v) {
    memset(v, 0, sizeof *v);
    v->line = l->line;
    tl_blank(l, 0);
    if (*l->s == '"') {
        v->kind = TV_STRING;
        if ((v->text = tl_string(l)) != NULL) tl_line_end(l);
    } else if (*l->s == '[') {
        v->kind = TV_LIST;
        l->s++;
        for (;;) {
            tl_blank(l, 1);
            if (*l->s == ']') { l->s++; break; }
            if (*l->s != '"') { tl_error(l, *l->s ? "a list item that is no string" : "a list without its ]"); return; }
            char *it = tl_string(l), *cond = NULL;
            if (!it) return;
            tl_blank(l, 0);
            if (!strncmp(l->s, "if", 2) && !isalnum((unsigned char)l->s[2]) && l->s[2] != '_') {
                l->s += 2;
                tl_blank(l, 0);
                if (!(cond = tl_expr(l, 1))) { free(it); return; }
            }
            if (v->nitems < TV_ITEMS) { v->item[v->nitems] = it; v->cond[v->nitems++] = cond; }
            else { free(it); free(cond); if (g_log) fprintf(g_log, "# texts: %s:%d: more than %d items - the rest ignored\n", l->file, l->line, TV_ITEMS); }
            int at = l->line;
            tl_blank(l, 1);
            if (*l->s == ',') { l->s++; continue; }
            if (*l->s == ']') { l->s++; break; }
            if (l->line == at) tl_error(l, "a , or ] missing between list items");
            else { int cur = l->line; l->line = v->line; tl_error(l, "a list without its ]"); l->line = cur; }
            return;
        }
        tl_line_end(l);
    } else {
        if (!(v->text = tl_expr(l, 0))) return;
        char *e;
        double n = strtod(v->text, &e);
        if (e != v->text && !*e) { v->kind = TV_NUMBER; v->n = n; }
        else v->kind = TV_EXPR;
    }
}
// read a text file: fn(arg, section, key, value) for every key ("" before the first section); mistakes logged with
// the file and line, their keys skipped
typedef void (*tfile_fn)(void *arg, const char *section, const char *key, struct tval *v, struct tlex *l);
static void tfile_parse(const char *file, const char *text, tfile_fn fn, void *arg) {
    struct tlex l = { text, file, 1, 0 };
    char section[48] = "";
    for (;;) {
        tl_blank(&l, 1);
        if (!*l.s) return;
        l.err = 0;
        if (*l.s == '[') {
            const char *e = strchr(l.s, ']'), *nl = strchr(l.s, '\n');
            if (!e || (nl && e > nl) || e - l.s - 1 >= (int)sizeof section) { l.s++; lstrcpyA(section, "?"); tl_error(&l, "a section without its ], with its keys"); }
            else { lstrcpynA(section, l.s + 1, (int)(e - l.s)); l.s = e + 1; tl_line_end(&l); }
        } else {
            char key[48];
            int n = 0;
            while (*l.s && (isalnum((unsigned char)*l.s) || strchr("_:.-", *l.s)) && n < 47) key[n++] = *l.s++;
            key[n] = 0;
            tl_blank(&l, 0);
            if (!n || *l.s != '=') tl_error(&l, "no key = value");
            else {
                l.s++;
                struct tval v;
                tl_value(&l, &v);
                if (!l.err && strcmp(section, "?")) fn(arg, section, key, &v, &l);
                tval_free(&v);
            }
        }
        if (l.err) {   // on to the next line that starts a key or a section (past the rest of a broken list)
            const char *b = l.s;
            while (b > text && (b[-1] == ' ' || b[-1] == '\t')) b--;
            if ((b == text || b[-1] == '\n') && (*l.s == '[' || isalpha((unsigned char)*l.s))) continue;   // already there
            for (;;) {
                while (*l.s && *l.s != '\n') l.s++;
                if (!*l.s) return;
                l.s++;
                l.line++;
                const char *t = l.s;
                while (*t == ' ' || *t == '\t') t++;
                if (*t == '[' || isalpha((unsigned char)*t) || *t == ';' || *t == '#') break;
            }
        }
    }
}
