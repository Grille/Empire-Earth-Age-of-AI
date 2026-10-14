// THE TEXT FILES (<game>\aimod_ai\*.ini: the personalities, _system.ini, _template.ini) and the EXPRESSIONS in them.
// A file: sections [name] / [name:<language>], "key = value" lines, comments from ; or # to the line's end (outside a
// string). A value is an expression - over several lines while a ( [ or { is open:
//   "text"                          a string; \" \\ \n escapes. {placeholders} in it - {name} - or expressions -
//                                   {opinion > 30 ? "friend" : "stranger"}: the quotes inside the braces are the
//                                   expression's, not the string's end
//   ["a", "b", ...]                 a list: one of its items, at random
//   allied ? "Welcome back."        without ":" - nothing when the condition doesn't hold
//   85, opinion > 30 ? "a" : "b"    numbers, any expression
// A list leaves out what is nothing and picks among the rest, a list in a list counting with each of its items (as
// if it stood there item by item); a list with nothing left is nothing. a ?? b: a, unless it is nothing. A
// personality's line is such a value: [allied ? "Welcome back.", opinion < -50 ? angry] ?? ["Greetings.", "Hello."]
// A mistake is logged with the file and line (# texts:) and that key is skipped.
// The expressions: numbers, "texts" (or 'texts', the same rules), lists, names (a placeholder of the text, a [vars]
// entry, a game value - x_builtin in diplo.c), ( ), + - * / %, < <= > >= == !=, and or not (only the words: && || ! are a mistake),
// a ? b : c, a ? b, a ?? b, the functions chance(n) (true n % of the time), min, max, abs, round, and the players'
// relations: them.opinion(me) (x_method in diplo.c). A player (me, them, ...) is a value: its name, equal only to itself.
// A value has a number, a text or both: the epoch is 3 and "Copper Age", a placeholder "12" is 12 and "12". The number
// counts in arithmetic, in comparisons with a number and as true/false; the text where the value ends up in the text
// and where it meets a text ("Age: " + epoch). + adds when both have numbers, else joins the texts; comparisons
// compare numbers when both have one, else texts (case ignored) - an epoch's name is the game's language's, so
// conditions compare its number (epoch >= epoch.middle), never its name. epoch.prehistoric .. epoch.space are the
// epochs' numbers 1..15, the same in every language (a "." is part of a name). True: a number not 0, a player, a text not
// empty; nothing is false and an empty text. Only what is chosen is worked out (?: ?? and or). No loops, no
// assignments: a text can't hang the game. A placeholder that is not a valid expression - or a single name nobody
// knows - stays in the text as written (logged); an unknown name inside an expression counts 0.
// Included by eeaimod.c before persona.c.
#define XVARS_MAX 48
#define XVAL_CAP 240
struct xvars { int n; char *key[XVARS_MAX], *val[XVARS_MAX]; };   // val: an expression's source
struct xctx {
    void *pl, *other, *third;               // the speaker, {name}, {enemy} - NULL: found by the placeholders' names
    int found;                              // pl/other/third looked up (x_builtin)
    const char *const *pn, *const *pv;      // the text's placeholders
    int np;
    const struct xvars *vars[3];            // searched in order: the personality's, the file's
    int depth, level, unknown, bad, circle; // nesting: vars in vars, the parser (all together); unknown names; errors; too deep
    const char *top;                        // the source being worked out (for the mistakes' messages)
};
// a number, a text or both - or nothing (w 0); w: how many items it was chosen from (a list's weight in a list);
// pl: a player (its index + 1; its text is its name) - equal only to itself; miss: a player who isn't there (lord
// without one) - nothing, which may only be checked (lord ? .., lord ?? .., lord == them): used, it is a mistake
struct xval { int has_n, has_s, w, pl; double n; char s[XVAL_CAP], miss[24]; };
static struct xvars g_vars_sys, g_vars_tpl;   // _system.ini [vars] (+ [vars:<language>]), _template.ini [vars] (texts.c)
static int x_builtin(struct xctx *c, const char *name, struct xval *v);   // diplo.c: the game's values
// diplo.c: a relation, them.opinion(me) - 0: name is none (v NULL: only that checked)
static int x_method(struct xctx *c, const char *name, const struct xval *a, int na, struct xval *v);
static void x_decode(const char *name, struct xval *v);   // diplo.c: a placeholder's text that names a number (an epoch)
static int x_fill(char *out, int cap, const char *tmpl, struct xctx *c);
static int rnd(int n);   // persona.c: the display generator
static int g_x_logged;   // expression errors written to the log (at most 60)
// STRICT PARSING ([general] strict_parsing=1, for whoever writes the texts; default 0: a mistake is logged and skipped,
// the game goes on): every mistake also comes up at once - those in the files at startup in a message box, those
// found while the game runs (an unknown name, a broken expression) as a system line in the game (x_errors_show,
// from the AIs' updates). An unknown name, or a placeholder nobody fills, counts as a mistake too.
static int g_strict;
static CRITICAL_SECTION g_x_lock;   // the queue: the language model's thread fills texts too
static char g_x_queue[8][240];
static int g_x_queued, g_x_total, g_x_dropped;
static void x_error(const char *fmt, ...) {
    char msg[240];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    if (g_log && g_x_logged < 60) { g_x_logged++; fprintf(g_log, "# texts: %s\n", msg); }
    if (!g_strict) return;
    EnterCriticalSection(&g_x_lock);
    g_x_total++;
    int seen = 0;   // the same mistake again (a line tried three times): once
    for (int i = 0; i < g_x_queued; i++) seen |= !strcmp(g_x_queue[i], msg);
    if (seen) g_x_total--;
    else if (g_x_queued < 8) lstrcpynA(g_x_queue[g_x_queued++], msg, sizeof g_x_queue[0]);
    else g_x_dropped++;
    LeaveCriticalSection(&g_x_lock);
}

static void xvars_set(struct xvars *v, const char *key, const char *val) {
    int i = 0;
    while (i < v->n && lstrcmpiA(v->key[i], key)) i++;
    if (i == XVARS_MAX) return;
    if (i < v->n) free(v->val[i]);
    else { v->key[i] = _strdup(key); v->n++; }
    v->val[i] = _strdup(val);
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
static void xnothing(struct xval *v) { v->has_n = v->has_s = v->w = v->pl = 0; v->n = 0; v->s[0] = v->miss[0] = 0; }
static void xnum(struct xval *v, double n) { v->has_n = 1; v->has_s = v->pl = 0; v->w = 1; v->n = n; v->miss[0] = 0; }
static void xstr(struct xval *v, const char *s) {   // a text - and its number when it is one ("12")
    lstrcpynA(v->s, s ? s : "", XVAL_CAP);
    v->has_s = 1;
    v->w = 1;
    v->pl = 0;
    v->miss[0] = 0;
    v->has_n = xnumeric(v->s, &v->n);
}
static void xdual(struct xval *v, double n, const char *s) { xstr(v, s); v->has_n = 1; v->n = n; }
static int xis_num(const struct xval *v, double *n) { if (v->has_n) *n = v->n; return v->has_n; }
static double xto_num(const struct xval *v) { return v->has_n ? v->n : 0; }
static int xtrue(const struct xval *v) { return v->w && (v->pl || (v->has_n ? v->n != 0 : v->s[0] != 0)); }   // a player: true, whatever its name
static void xto_text(const struct xval *v, char *out, int cap) {
    if (v->has_s || !v->w) { lstrcpynA(out, v->w ? v->s : "", cap); return; }
    double r = v->n < 0 ? -v->n : v->n;
    if (r - (double)(long long)r < 1e-9 && r < 1e15) snprintf(out, (size_t)cap, "%lld", (long long)v->n);
    else {
        snprintf(out, (size_t)cap, "%.2f", v->n);
        char *e = out + strlen(out);
        while (e > out && e[-1] == '0') *--e = 0;
        if (e > out && e[-1] == '.') e[-1] = 0;
    }
}

// a string literal at s (" or '): its text into out (cap bytes) - escapes undone, {braces} kept as written (an
// expression's quotes are its own there); *end after the closing quote. Returns the length, -1 on a mistake (*why).
// out NULL, cap 0: only checked.
static int x_string(const char *s, char *out, int cap, const char **end, const char **why) {
    char q0 = *s, q = 0;
    int o = 0, depth = 0;
    for (s++;; s++) {
        char c = *s;
        if (!c || c == '\n' || c == '\r') { *why = depth ? "a { without its }" : "a string without its closing quote"; return -1; }
        if (depth) {   // in {braces}: an expression, kept as written
            if (q) { if (c == q) q = 0; else if (c == '\\' && s[1] && s[1] != '\n') { if (o < cap - 1) out[o++] = c; c = *++s; } }
            else if (c == '"' || c == '\'') q = c;
            else if (c == '{') depth++;
            else if (c == '}') depth--;
        } else if (c == q0) { if (cap > 0) out[o < cap ? o : cap - 1] = 0; *end = s + 1; return o; }
        else if (c == '\\') {
            char e = s[1];
            if (e != 'n' && e != '"' && e != '\'' && e != '\\') { *why = "an unknown escape (\\\" \\\\ \\n)"; return -1; }
            c = e == 'n' ? '\n' : e;
            s++;
        } else if (c == '{') depth = 1;
        if (o < cap - 1) out[o++] = c;
    }
}

// the parser evaluates as it reads; skip: only reads (a branch not chosen)
// check: only the syntax (the files when read: skip, and the strings' placeholders too); why: what is wrong, if known
struct xp { const char *s; struct xctx *c; int err, skip, check; const char *why; };
static int x_syntax(const char *src, const char **why);
static int x_fills_ok(const char *text, const char **why);
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
// a player who isn't there, used without a check (strict parsing only: else it is nothing, quietly)
static void x_missing(struct xctx *c, const char *who) {
    if (g_strict) x_error("%s is used, but there is none here - check it first (%s ? ... / %s ?? ...): %.100s", who, who, who, c->top ? c->top : "");
}
static void x_use(struct xp *p, const struct xval *v) { if (v->miss[0] && !p->skip) x_missing(p->c, v->miss); }
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
    if (p->skip) { xnothing(v); return; }
    for (int i = 0; i < c->np; i++)
        if (!strcmp(c->pn[i], name)) { xstr(v, c->pv[i]); if (!v->has_n) x_decode(name, v); return; }
    for (int k = 0; k < 3; k++) {
        const char *def = xvars_get(c->vars[k], name);
        if (!def) continue;
        if (c->depth >= 6) { c->circle = 1; p->err = 1; return; }   // vars naming each other in a circle
        c->depth++;
        struct xp q = { def, c, 0, 0, 0, NULL };
        xp_expr(&q, v);
        xp_space(&q);
        if (q.err || *q.s || c->circle) p->err = 1;
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
    xnothing(v);
    if (++p->c->level > 24) { p->c->level--; p->err = 1; return; }
    if (*s == '(') {
        p->s++;
        xp_expr(p, v);
        if (!xp_op(p, ")")) p->err = 1;
    } else if (*s == '"' || *s == '\'') {   // a string: its placeholders filled
        char t[XVAL_CAP], f[XVAL_CAP];
        const char *why;
        if (x_string(s, t, sizeof t, &p->s, &why) < 0) { p->err = 1; goto out; }
        if (p->skip) { xstr(v, ""); if (p->check && !x_fills_ok(t, &p->why)) p->err = 1; }
        else { x_fill(f, sizeof f, t, p->c); xstr(v, f); }
    } else if (*s == '[') {   // a list: one of its items - each of them as often as the items it was chosen from
        struct xval it;
        int total = 0;
        p->s++;
        for (;;) {
            if (xp_op(p, "]")) break;
            xp_expr(p, &it);
            if (p->err) goto out;
            x_use(p, &it);
            if (!p->skip && it.w > 0) {
                total += it.w;
                if (rnd(total) < it.w) *v = it;
            }
            if (xp_op(p, ",")) continue;
            if (!xp_op(p, "]")) p->err = 1;
            break;
        }
        v->w = total;
        if (!total) xnothing(v);
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
        int f = !lstrcmpiA(name, "chance") && na == 1 ? 1 : (!lstrcmpiA(name, "min") || !lstrcmpiA(name, "max")) && na ? 2 :
                !lstrcmpiA(name, "abs") && na == 1 ? 3 : !lstrcmpiA(name, "round") && na == 1 ? 4 : 0;
        for (int i = 0; f && i < na; i++) x_use(p, &a[i]);   // a function's arguments (a relation checks its players)
        if (!f) {   // a relation: them.opinion(me)
            if (!x_method(p->c, name, a, na, p->skip ? NULL : v)) { p->err = 1; p->why = "an unknown function or relation (or the wrong number of arguments)"; }
            else if (p->skip) xnothing(v);
        }
        else if (p->skip) xnothing(v);
        else if (f == 1) xnum(v, rnd(100) < xto_num(&a[0]));
        else if (f == 2) {
            int mx = !lstrcmpiA(name, "max");
            double r = xto_num(&a[0]);
            for (int i = 1; i < na; i++) { double x = xto_num(&a[i]); if (mx ? x > r : x < r) r = x; }
            xnum(v, r);
        } else if (f == 3) { double x = xto_num(&a[0]); xnum(v, x < 0 ? -x : x); }
        else { double x = xto_num(&a[0]); xnum(v, (double)(long long)(x < 0 ? x - 0.5 : x + 0.5)); }
    } else p->err = 1;
out:
    p->c->level--;
}
static void xp_unary(struct xp *p, struct xval *v) {
    if (xp_op(p, "-")) { xp_unary(p, v); x_use(p, v); xnum(v, -xto_num(v)); return; }
    if (xp_word(p, "not")) { xp_unary(p, v); xnum(v, !xtrue(v)); return; }
    if (*p->s == '!' && p->s[1] != '=') { p->err = 1; p->why = "! - write not"; xnothing(v); return; }
    xp_primary(p, v);
}
static void xp_mul(struct xp *p, struct xval *v) {
    xp_unary(p, v);
    while (!p->err) {
        int op = xp_op(p, "*") ? '*' : xp_op(p, "/") ? '/' : xp_op(p, "%") ? '%' : 0;
        if (!op) return;
        struct xval b;
        xp_unary(p, &b);
        x_use(p, v);
        x_use(p, &b);
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
        x_use(p, v);
        x_use(p, &b);
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
             xp_op(p, "<") ? '<' : xp_op(p, ">") ? '>' : 0;
    if (!op) return;
    struct xval b;
    xp_add(p, &b);
    double x, y;
    int d;
    if (!v->w || !b.w) { xnum(v, 0); return; }   // nothing compares with nothing: false
    if (v->pl && b.pl) d = v->pl - b.pl;   // players: the same one or not
    else if (xis_num(v, &x) && xis_num(&b, &y)) d = x < y ? -1 : x > y;
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
    if (!p->err && (xp_space(p), *p->s == '&')) { p->err = 1; p->why = "&& - write and"; }
    while (!p->err && xp_word(p, "and")) {
        struct xval b;
        int sk = p->skip, t = xtrue(v);
        p->skip = sk || !t;
        xp_cmp(p, &b);
        p->skip = sk;
        xnum(v, t && xtrue(&b));
        if (!p->err && (xp_space(p), *p->s == '&')) { p->err = 1; p->why = "&& - write and"; }
    }
}
static void xp_or(struct xp *p, struct xval *v) {
    xp_and(p, v);
    if (!p->err && (xp_space(p), *p->s == '|')) { p->err = 1; p->why = "|| - write or"; }
    while (!p->err && xp_word(p, "or")) {
        struct xval b;
        int sk = p->skip, t = xtrue(v);
        p->skip = sk || t;
        xp_and(p, &b);
        p->skip = sk;
        xnum(v, t || xtrue(&b));
        if (!p->err && (xp_space(p), *p->s == '|')) { p->err = 1; p->why = "|| - write or"; }
    }
}
static void xp_coalesce(struct xp *p, struct xval *v) {   // a ?? b: a, unless it is nothing
    xp_or(p, v);
    while (!p->err && xp_op(p, "??")) {
        struct xval b;
        int sk = p->skip, have = v->w > 0;
        p->skip = sk || have;
        xp_or(p, &b);
        p->skip = sk;
        if (!have) *v = b;
    }
}
static void xp_expr(struct xp *p, struct xval *v) {   // c ? a : b, c ? a (nothing when c doesn't hold)
    if (++p->c->level > 24) { p->c->level--; p->err = 1; return; }
    xp_coalesce(p, v);
    if (!p->err && xp_op(p, "?")) {
        struct xval a, b;
        int sk = p->skip, t = xtrue(v);
        p->skip = sk || !t;
        xp_expr(p, &a);
        p->skip = sk;
        xnothing(&b);
        if (!p->err && xp_op(p, ":")) {
            p->skip = sk || t;
            xp_expr(p, &b);
            p->skip = sk;
        }
        if (!p->err) *v = t ? a : b;
    }
    p->c->level--;
}
// evaluate the whole of src (an expression and nothing after it); 0: not an expression
static void x_log_bad(const char *what, const char *src) { x_error("%s: %.120s", what, src); }
static int x_eval(const char *src, struct xctx *c, struct xval *v) {   // its value ends up in a text: a line, a {placeholder}
    struct xp p = { src, c, 0, 0, 0, NULL };
    int unk = c->unknown, top = !c->top;
    if (top) c->top = src;
    xp_expr(&p, v);
    xp_space(&p);
    int ok = !p.err && !*p.s && !c->circle;
    if (ok) x_use(&p, v);
    if (top) c->top = NULL;
    if (c->circle && !c->depth) { c->circle = 0; x_log_bad("[vars] naming each other in a circle", src); }
    if (ok && !c->depth && c->unknown > unk) x_log_bad("an unknown name (counted 0) in", src);   // a line, a condition
    return ok;
}

// the syntax of an expression, nothing worked out (no game needed: the files are checked when read); 0: a mistake, *why
// what it is when known
static int x_syntax(const char *src, const char **why) {
    struct xctx c = { 0 };
    c.found = 1;   // no players looked up
    struct xp p = { src, &c, 0, 1, 1, NULL };
    struct xval v;
    xp_expr(&p, &v);
    xp_space(&p);
    if (!p.err && *p.s) { p.err = 1; if (!p.why) p.why = "something after the expression"; }
    if (p.err) *why = p.why ? p.why : "not an expression";
    return !p.err;
}
static const char *x_close(const char *s);
// a text's {placeholders} that are expressions (not a plain name): their syntax
static int x_fills_ok(const char *text, const char **why) {
    for (const char *s = strchr(text, '{'); s; s = strchr(s + 1, '{')) {
        const char *close = x_close(s);
        char src[400];
        int n = close ? (int)(close - s - 1) : -1;
        if (n < 0 || n >= (int)sizeof src) continue;
        memcpy(src, s + 1, (size_t)n);
        src[n] = 0;
        if (strpbrk(src, " <>=?\"'(+[!&|") && !x_syntax(src, why)) return 0;
        s = close;
    }
    return 1;
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
// out = tmpl with its {placeholders} filled: the text's own (exact name), else the expression's value (nothing: no
// text); one that is not an expression, or a lone unknown name, stays as written. Returns the length.
static int x_fill(char *out, int cap, const char *tmpl, struct xctx *c) {
    int o = 0, top = !c->top;
    if (top) c->top = tmpl;
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
                if (ok && !lone && c->unknown > unk) x_log_bad("an unknown name (counted 0) in", src);
                else if (lone && g_strict) x_log_bad("a placeholder nobody fills", src);
                else if (!ok && strpbrk(src, " <>=?\"'(+[")) { c->bad++; x_log_bad("not an expression", src); }
            }
        }
        if (rep) { while (*rep && o < cap - 1) out[o++] = *rep++; s = close + 1; }
        else out[o++] = *s++;
    }
    out[o] = 0;
    if (top) c->top = NULL;
    return o;
}
// the strings a value offers as they stand - a string, or the items of a list that are plain strings (no
// condition, no expression around them): fn(text) for each (the colours)
static int x_plain_strings(const char *src, void (*fn)(const char *text, void *arg), void *arg) {
    char t[XVAL_CAP];
    const char *e, *why;
    int n = 0;
    while (*src == ' ') src++;
    if (*src == '"' || *src == '\'') {
        if (x_string(src, t, sizeof t, &e, &why) >= 0) { while (*e == ' ') e++; if (!*e) { fn(t, arg); n++; } }
        return n;
    }
    if (*src != '[') return 0;
    for (const char *s = src + 1; *s;) {   // the list's top-level items
        while (*s == ' ' || *s == ',') s++;
        const char *b = s;
        int d = 0;
        for (; *s && (d || (*s != ',' && *s != ']')); s++) {
            if (*s == '"' || *s == '\'') { if (x_string(s, t, sizeof t, &e, &why) < 0) return n; s = e - 1; }
            else if (*s == '(' || *s == '[' || *s == '{') d++;
            else if (*s == ')' || *s == ']' || *s == '}') d--;
        }
        if ((*b == '"' || *b == '\'') && x_string(b, t, sizeof t, &e, &why) >= 0) {
            while (*e == ' ') e++;
            if (e == s) { fn(t, arg); n++; }
        }
        if (*s == ']' || !*s) break;
    }
    return n;
}

// the file format: a value is a string (TV_STRING: its text, a template), a number or another expression (its source)
enum { TV_STRING, TV_NUMBER, TV_EXPR };
struct tval {
    int kind, line;
    char *text;   // a string's text (escapes undone), a template
    char *src;    // the value's source as an expression, on one line, without comments
    double n;     // a number
};
struct tlex { const char *s, *file; int line, err; };
static void tl_error(struct tlex *l, const char *what) {
    x_error("%s:%d: %s - skipped", l->file, l->line, what);
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
static void tval_free(struct tval *v) { free(v->text); free(v->src); memset(v, 0, sizeof *v); }
static int tl_key_line(const char *t) {   // a line that starts a key ("name =", not "a == b") or a section
    while (*t == ' ' || *t == '\t') t++;
    if (*t == '[') return 1;
    const char *k = t;
    while (*t && (isalnum((unsigned char)*t) || strchr("_:.-", *t))) t++;
    if (t == k) return 0;
    while (*t == ' ' || *t == '\t') t++;
    return *t == '=' && t[1] != '=';
}
// a value: to the line's end, over line breaks while a ( [ or { is open (a line that starts a key or a section ends
// it: the bracket was never closed); comments left out
static void tl_value(struct tlex *l, struct tval *v) {
    memset(v, 0, sizeof *v);
    v->line = l->line;
    tl_blank(l, 0);
    size_t cap = 256, o = 0;
    char *r = malloc(cap);
    int depth = 0;
    for (;;) {
        char c = *l->s;
        if (!c || (c == '\n' && !depth)) break;
        if (c == '\n' && tl_key_line(l->s + 1)) { l->line++; l->s++; break; }   // the next key: depth tells
        if (c == '\n' || c == '\r' || c == '\t') { if (c == '\n') l->line++; l->s++; c = ' '; }
        else if (c == ';' || c == '#') { while (*l->s && *l->s != '\n') l->s++; continue; }
        else if (c == '"' || c == '\'') {   // a string, copied as written
            const char *e, *why;
            if (x_string(l->s, NULL, 0, &e, &why) < 0) { tl_error(l, why); free(r); return; }
            size_t len = (size_t)(e - l->s);
            if (o + len + 2 >= cap) r = realloc(r, cap = (o + len) * 2 + 2);
            memcpy(r + o, l->s, len);
            o += len;
            l->s = e;
            continue;
        } else {
            if (c == '(' || c == '[' || c == '{') depth++;
            if ((c == ')' || c == ']' || c == '}') && --depth < 0) { tl_error(l, "a ) ] or } too many"); free(r); return; }
            l->s++;
        }
        if (c == ' ' && (!o || r[o - 1] == ' ')) continue;
        if (o + 2 >= cap) r = realloc(r, cap *= 2);
        r[o++] = c;
    }
    while (o && r[o - 1] == ' ') o--;
    r[o] = 0;
    if (depth) { int cur = l->line; l->line = v->line; tl_error(l, "a ( [ or { without its end"); l->line = cur; free(r); return; }
    if (!o) { tl_error(l, "a value missing"); free(r); return; }
    v->src = r;
    const char *e, *why;
    char *t = malloc(o + 1);
    double n;
    if ((*r == '"' || *r == '\'') && x_string(r, t, (int)o + 1, &e, &why) >= 0 && !*e) { v->kind = TV_STRING; v->text = t; }
    else {
        free(t);
        char *ne;
        n = strtod(r, &ne);
        if (ne != r && !*ne) { v->kind = TV_NUMBER; v->n = n; }
        else v->kind = TV_EXPR;
    }
    // the syntax now (an expression, a string's placeholders): a mistake comes up when the file is read, not when the
    // line is said
    if (v->kind == TV_EXPR ? !x_syntax(r, &why) : v->kind == TV_STRING && !x_fills_ok(v->text, &why)) {
        int cur = l->line;
        l->line = v->line;
        tl_error(l, why);
        l->line = cur;
        tval_free(v);
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
            else {
                lstrcpynA(section, l.s + 1, (int)(e - l.s));
                l.s = e + 1;
                tl_blank(&l, 0);
                if (*l.s == ';' || *l.s == '#') while (*l.s && *l.s != '\n') l.s++;
                if (*l.s && *l.s != '\n') tl_error(&l, "something after the section");
            }
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
        if (l.err) {   // on to the next line that starts a key or a section (past the rest of a broken value)
            const char *b = l.s;
            while (b > text && (b[-1] == ' ' || b[-1] == '\t')) b--;
            if ((b == text || b[-1] == '\n') && tl_key_line(b)) continue;   // already there
            for (;;) {
                while (*l.s && *l.s != '\n') l.s++;
                if (!*l.s) return;
                l.s++;
                l.line++;
                if (tl_key_line(l.s)) break;
            }
        }
    }
}
