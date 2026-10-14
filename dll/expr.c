// EXPRESSIONS in the texts (personalities' lines, _system.ini, _template.ini): a placeholder may hold an expression,
// {opinion > 30 ? "friend" : "stranger"}, and a [vars] section may name one (address = opinion > 30 ? "friend" :
// "stranger", then {address} in a line). A personality's line variant may start with a condition, "[opinion < -50]
// Go away.": the variants whose condition holds are picked from, those without one only when none holds.
// The language: numbers, "strings" (or 'strings'), names (a placeholder of the text, a [vars] entry, a game value -
// x_builtin in diplo.c), ( ), + - * / %, < <= > >= == != (= is ==), and or not (&& || !), a ? b : c, and the
// functions pick(a, b, ...) (one at random), chance(n) (true n % of the time), min, max, abs, round.
// A value has a number, a text or both: the epoch is 3 and "Copper Age", a placeholder "12" is 12 and "12". The number
// counts in arithmetic, in comparisons with a number and as true/false; the text where the value ends up in the text
// and where it meets a text ("Age: " + epoch, epoch == "Copper Age"). + adds when both have numbers, else joins the
// texts; comparisons compare numbers when both have one, else texts (case ignored). True: a number not 0, a text not
// empty. No loops, no assignments: a text can't hang the game. A placeholder that is not a
// valid expression - or a single name nobody knows - stays as written (as before); an unknown name inside an
// expression counts as 0. A [vars] value that is not an expression is plain text (its placeholders filled).
// Included by eeaimod.c before persona.c.
#define XVARS_MAX 48
#define XVAL_CAP 240
struct xvars { int n; char *key[XVARS_MAX], *val[XVARS_MAX]; };
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

static void xvars_set(struct xvars *v, const char *key, const char *val) {
    for (int i = 0; i < v->n; i++)
        if (!lstrcmpiA(v->key[i], key)) { free(v->val[i]); v->val[i] = _strdup(val); return; }
    if (v->n < XVARS_MAX) { v->key[v->n] = _strdup(key); v->val[v->n++] = _strdup(val); }
}
static const char *xvars_get(const struct xvars *v, const char *key) {
    for (int i = 0; v && i < v->n; i++) if (!lstrcmpiA(v->key[i], key)) return v->val[i];
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
static void xp_ident(struct xp *p, const char *name, struct xval *v) {
    struct xctx *c = p->c;
    for (int i = 0; i < c->np; i++)
        if (!strcmp(c->pn[i], name)) { xstr(v, c->pv[i]); if (!v->has_n) x_decode(name, v); return; }
    for (int k = 0; k < 3; k++) {
        const char *def = xvars_get(c->vars[k], name);
        if (!def) continue;
        if (!lstrcmpiA(def, name)) { xstr(v, def); return; }   // sire=sire: a word, not itself
        if (c->depth >= 6) { c->circle = 1; p->err = 1; return; }   // vars naming each other in a circle
        c->depth++;
        int unk = c->unknown;
        struct xp q = { def, c, 0 };
        xp_expr(&q, v);
        xp_space(&q);
        if (c->circle) p->err = 1;
        else if (q.err || *q.s || c->unknown > unk) {   // not an expression (or names nobody knows): plain text, filled
            c->unknown = unk;
            char t[XVAL_CAP];
            x_fill(t, sizeof t, def, c);
            xstr(v, t);
        } else if (v->has_s && strchr(v->s, '{')) {   // a text from an expression may hold placeholders too
            char t[XVAL_CAP];
            x_fill(t, sizeof t, v->s, c);
            xstr(v, t);
        }
        c->depth--;
        return;
    }
    if (x_builtin(c, name, v)) return;
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
        while (*s && *s != q) { if (o < XVAL_CAP - 1) t[o++] = *s; s++; }
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
        while ((isalnum((unsigned char)*s) || *s == '_') && n < 39) name[n++] = *s++;
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
        if (*s == '"' || *s == '\'') { char q = *s; for (s++; *s && *s != q; s++) {} if (!*s) return NULL; continue; }
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
                int lone = ok && c->unknown > unk && strspn(src, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_") == (size_t)n;
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
// a line variant's condition: "[expr] text" -> 1 holds / 0 not, *text after it; -1 no condition ("[else]" too)
static int x_condition(const char *line, const char **text, struct xctx *c) {
    *text = line;
    if (*line != '[') return -1;
    int d = 0;
    const char *e = line;
    for (; *e; e++) {
        if (*e == '"' || *e == '\'') { char q = *e; for (e++; *e && *e != q; e++) {} if (!*e) return -1; continue; }
        if (*e == '[') d++;
        else if (*e == ']' && --d == 0) break;
    }
    if (!*e) return -1;
    char src[300];
    int n = (int)(e - line - 1);
    if (n <= 0 || n >= (int)sizeof src) return -1;
    memcpy(src, line + 1, (size_t)n);
    src[n] = 0;
    const char *t = e + 1;
    while (*t == ' ') t++;
    if (!lstrcmpiA(src, "else")) { *text = t; return -1; }
    struct xval v;
    int unk = c->unknown;
    if (!x_eval(src, c, &v)) { x_log_bad("condition is not an expression", src); return -1; }
    if (c->unknown > unk && strspn(src, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_") == (size_t)n) return -1;   // "[sighs]"
    *text = t;
    return xtrue(&v);
}
// split a value at | outside quotes, brackets and braces (an expression may say ||): calls f(piece) for each
static void x_split(char *val, void (*f)(char *piece, void *arg), void *arg) {
    int d = 0;
    char q = 0, *start = val;
    for (char *s = val;; s++) {
        if (q) { if (*s == q) q = 0; else if (!*s) q = 0; }
        else if (*s == '"' && d) q = *s;
        else if (*s == '[' || *s == '{' || *s == '(') d++;
        else if ((*s == ']' || *s == '}' || *s == ')') && d) d--;
        if (!*s || (*s == '|' && !d && !q)) {
            int end = !*s;
            *s = 0;
            f(start, arg);
            if (end) return;
            start = s + 1;
        }
    }
}
