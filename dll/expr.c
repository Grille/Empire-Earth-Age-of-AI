// THE TEXT FILES (<game>\aimod_ai\*.ini: the personalities, _system.ini, _template.ini) and the EXPRESSIONS in them.
// A file: sections [name] / [name:<language>], "key = value" lines, comments from ; or # to the line's end (outside a
// string). A value is an expression - over several lines while a ( [ or { is open:
//   "text"                          a string; \" \\ \n escapes. {expressions} in it - {them} -
//                                   {opinion > 30 ? "friend" : "stranger"}: the quotes inside the braces are the
//                                   expression's, not the string's end; a string stays on its line
//   ("a "  <line break>  "b")       strings side by side in ( ): one string (a value of only these: a string, also
//                                   where a key takes strings only - x_fold); in [ ] a missing comma stays a mistake
//   ["a", "b", ...]                 a list: its items; as one value (in a text, + < ...) one of them, at random
//   allied ? "Welcome back."        without ":" - nothing when the condition doesn't hold
//   85, opinion > 30 ? "a" : "b"    numbers, any expression
// A list leaves out what is nothing and picks among the rest, a list in a list counting with each of its items (as
// if it stood there item by item); a list with nothing left is nothing. a ?? b: a, unless it is nothing. A
// personality's line is such a value: [allied ? "Welcome back.", opinion < -50 ? angry] ?? ["Greetings.", "Hello."]
// A mistake is logged with the file and line (# texts:) and that key is skipped.
// The expressions: numbers (70% is 0.7 - % only right after a number), "texts" (or 'texts', the same rules), lists,
// names (a placeholder of the text, a [vars] entry, a game value - x_builtin in diplo.c), ( ), + - * /,
// < <= > >= == !=, and or not (only the words: && || ! are a mistake), a ? b : c, a ? b, a ?? b, the functions
// chance(p) (true with the probability p, 0..1: chance(70%)), rnd(n) (0 <= x < n, n 1 when left out), min, max, abs,
// round, floor (down to a whole number), log2 (of a number above 0), the lists' len(l), first(l), item(l, i) (from 1), contains(l, x) and
// map_range(x, [limits], [values]) (the value of the first limit x is below, the last past them all - a value no
// list is a list of one), str(x) / number(x) (its text only / its number, a text's when it is one, else nothing), and
// the players' relations: them.opinion(me) (x_method in diplo.c). A player (me, them, ...) is a value: its
// name, equal only to itself.
// A placeholder's number is written with up to 2 decimals, or by its format after the last ':' that belongs to no
// '?' (x_fmt_at): {x:%} a share (0.5 -> 50%), {x:.1%}, {x:.2} - 50% in an expression is only 0.5, never a format.
// A choice before a format goes in ( ): {(a ? 1 : 0):%}.
// A value has a number, a text or both: the epoch is 3 and "Copper Age", a placeholder the code fills with "12" is 12
// and "12" - a string in a file is a text only ("12" + 1 is "121"; "12" == 12 is a mistake, false). The number
// counts in arithmetic, in comparisons with a number and as true/false; the text where the value ends up in the text
// and where it meets a text ("Age: " + epoch). + adds when both have numbers, else joins the texts; comparisons
// compare numbers when both have one, else texts (case ignored) - a text with no number against a number with no text
// is a mistake (logged; only != holds) - an epoch's name is the game's language's, so
// conditions compare its number (epoch >= epoch.middle), never its name. epoch.prehistoric .. epoch.space are the
// epochs' numbers 1..15, the same in every language (a "." is part of a name); a player's colour likewise
// (them.color == color.red, color.blue .. 1..17). True: a number not 0, a player, a text not
// empty; nothing is false and an empty text. Only what is chosen is worked out (?: ?? and or). No loops, no
// assignments: a text can't hang the game. A placeholder that is not a valid expression stays in the text as written
// (logged).
// MISTAKES WHILE WORKING OUT (x_fail; the file was fine, the value isn't): an unknown name, a text in arithmetic
// ("a" * 2), a text and a number added or compared ("12" + 1, "12" == 12), chance(70), a player who isn't there used
// without a check (lord.soldiers without a lord - x_missing). Nothing is guessed: the
// mistake is logged (once) and fails what it is in - up to the nearest list item, which is left out, else the whole
// value: a line then falls back like one with nothing to say, a {placeholder} writes nothing. Not "nothing": that
// would make "not foo" true. The names are checked when the files are read too (x_name_check) - told, the key kept.
// Included by eeaimod.c before persona.c.
#define XVARS_MAX 48
#define XVAL_CAP 512   // a text value (a [vars] function may return a whole prompt paragraph)
struct xvars { int n; char *key[XVARS_MAX], *val[XVARS_MAX]; };   // val: an expression's source
struct xval;
struct xctx {
    void *pl, *other, *third;               // the speaker, them, enemy - them NULL: the local player
    const char *const *fpn;                 // a function's parameters while its body is worked out (x_call)
    const struct xval *fpv;
    int fnp;
    int found;                              // pl/other/third looked up (x_builtin)
    const char *const *pn, *const *pv;      // the text's placeholders
    int np;
    const struct xvars *vars[3];            // searched in order: the personality's, the file's
    int depth, level, failed, bad, circle;  // nesting: vars in vars, the parser (all together); mistakes while working
                                            // it out (x_fail, told); not an expression; too deep
    int nogame;                             // before the game (a personality's color): its values and players are mistakes
    int game;                               // a decision ([decide]): it changes the game - draws from the game's generator
    int fixed;                              // an ini file's number (ini.c): no names, no draws, no lists - told quietly
    int missed;                             // x_missing from the game's values (diplo.c): the primary fails (xp_primary)
    const char *top;                        // the source being worked out (for the mistakes' messages)
};
// a number, a text or both - or nothing (w 0); w: how many items it was chosen from (a list's weight in a list);
// pl: a player (its index + 1; its text is its name) - equal only to itself; miss: a player who isn't there (lord
// without one) - nothing, which may only be checked (lord ? .., lord ?? .., lord == them): used, it is a mistake
// li, ln: a list's items (ln 0: no list) - its value meanwhile is one of them, at random
struct xval { int has_n, has_s, w, pl, li, ln; double n; char s[XVAL_CAP], miss[24]; };
// LISTS: a [ ] keeps its items - flattened, a list in a list giving its own - for what wants them all (len, first,
// item, contains, map_range); used as one value (in a text, + - < ...) it is the item it picked. The items live
// until the outermost x_eval ends (g_xl_busy); a list too big for them is one value only (len counts it whole).
#define XL_VALS 384
#define XL_IDX 1024
#define XL_ITEMS 256   // in one list
static struct xval g_xl_val[XL_VALS];
static int g_xl_idx[XL_IDX], g_xl_nval, g_xl_nidx, g_xl_busy, g_xl_told;
static int xl_n(const struct xval *v) { return v->ln ? v->ln : v->w > 0; }   // its items (one value: 1, nothing: 0)
static const struct xval *xl_at(const struct xval *v, int i) { return v->ln ? &g_xl_val[g_xl_idx[v->li + i]] : v; }
static struct xvars g_vars_sys, g_vars_tpl;   // _system.ini [vars] (+ [vars:<language>]), _template.ini [vars] (texts.c)
static int x_builtin(struct xctx *c, const char *name, struct xval *v);   // diplo.c: the game's values
static int x_is_value(const char *name);   // diplo.c: a name that is a value of its own (them, epoch, minutes ...)
// diplo.c: a relation, them.opinion(me) - 0: name is none (v NULL: only that checked)
static int x_method(struct xctx *c, const char *name, const struct xval *a, int na, struct xval *v);
static int x_fill(char *out, int cap, const char *tmpl, struct xctx *c);
static int rnd(int n);   // persona.c: the display generator
static int rnd_game(int n);   // persona.c: the game's (decisions)
static float det_log2f(float x);   // diplo.c: log2 with the same bits on every PC
static int g_x_logged;   // expression errors written to the log (at most 60)
// STRICT PARSING ([general] strict_parsing=1, for whoever writes the texts; default 0: a mistake is logged and skipped,
// the game goes on): every mistake also comes up at once - those in the files at startup in a message box, those
// found while the game runs (an unknown name, a text in arithmetic ...) as a system line in the game (x_errors_show,
// from the AIs' updates).
static int g_strict;
static CRITICAL_SECTION g_x_lock;   // the queue: filled on the game's thread, read by the startup box's (x_errors_box)
static char g_x_queue[8][240];
static int g_x_queued, g_x_total, g_x_dropped;
static void x_error(const char *fmt, ...) {
    char msg[240];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    static uint32_t told[64];   // the same mistake again (a line said again, a placeholder in every prompt): once
    uint32_t h = 2166136261u;
    for (const char *s = msg; *s; s++) h = (h ^ (uint8_t)*s) * 16777619u;
    h |= 1;
    if (told[h % 64] == h) return;
    told[h % 64] = h;
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

// [vars] keys: "name" or a function "name(a,b)" (its parameters, tfile_parse) - the same name is the same entry
static int xkey_is(const char *key, const char *name) {
    size_t n = strcspn(key, "(");
    return n == strcspn(name, "(") && !_strnicmp(key, name, n);
}
static int xvars_find(const struct xvars *v, const char *key) {
    for (int i = 0; v && i < v->n; i++) if (xkey_is(v->key[i], key)) return i;
    return -1;
}
static void xvars_set(struct xvars *v, const char *key, const char *val) {
    int i = xvars_find(v, key);
    if (i < 0 && v->n == XVARS_MAX) return;
    if (i >= 0) { free(v->val[i]); free(v->key[i]); }
    else i = v->n++;
    v->key[i] = _strdup(key);
    v->val[i] = _strdup(val);
}
static const char *xvars_get(const struct xvars *v, const char *key) {   // a variable (no function)
    int i = xvars_find(v, key);
    return i >= 0 && !strchr(v->key[i], '(') ? v->val[i] : NULL;
}
static const char *xvars_fn(const struct xvars *v, const char *name, const char **par) {   // a function: its body, *par "a,b)"
    int i = xvars_find(v, name);
    if (i < 0 || !strchr(v->key[i], '(')) return NULL;
    *par = strchr(v->key[i], '(') + 1;
    return v->val[i];
}
// THE FUNCTIONS of all the files in the folder (name and how many parameters): the files are checked when read, a
// function may come from a file read later (an include, _system.eeai) - scanned from every file first (x_fn_known)
static struct { char name[40]; int n; } g_xfn[96];
static int g_xfn_n, g_xfn_scanned;
static void x_fn_scan_dir(void);   // persona.c: every .eeai of the folder through x_fn_scan
// the parameters ("a,b)") of the function whose definition is being read: its body's syntax check takes them for
// players where a path starts with one (by.kills(of)) - at run time the frame has them (c->fpn)
static const char *g_x_check_par;
static int x_check_par_has(const char *s, int n) {
    for (const char *p = g_x_check_par; p && *p && *p != ')'; ) {
        int k = (int)strcspn(p, ",)");
        if (k == n && !strncmp(p, s, (size_t)n)) return 1;
        p += k;
        if (*p == ',') p++;
    }
    return 0;
}
// THE [vars] NAMES of all the files too (x_name_check: an entry may come from another file, or later in this one)
static char g_xvn[256][40];
static int g_xvn_n;
static void x_var_add(const char *b, size_t n) { if (n && n < 40 && g_xvn_n < 256) lstrcpynA(g_xvn[g_xvn_n++], b, (int)n + 1); }
static void x_fn_scan(const char *text) {   // the lines "name(a, b) =" of a file, and "name =" in [vars] / [vars:..]
    int vars = 0;
    for (const char *s = text; *s; ) {
        if (*s == '[') vars = !_strnicmp(s + 1, "vars", 4) && (s[5] == ']' || s[5] == ':');   // a section
        while (*s == ' ' || *s == '\t') s++;
        const char *b = s;
        while (isalnum((unsigned char)*s) || *s == '_') s++;
        size_t n = (size_t)(s - b);
        if (vars && n && *s != '(') {
            const char *e = s;
            while (*e == ' ' || *e == '\t') e++;
            if (*e == '=' && e[1] != '=') x_var_add(b, n);
        }
        if (n && n < 40 && *s == '(') {
            int k = 1, empty = 1;
            for (s++; *s && *s != ')' && *s != '\n'; s++) { if (*s == ',') k++; else if (*s != ' ') empty = 0; }
            if (*s == ')') {
                const char *e = s + 1;
                while (*e == ' ' || *e == '\t') e++;
                if (*e == '=' && e[1] != '=' && g_xfn_n < 96) {
                    lstrcpynA(g_xfn[g_xfn_n].name, b, (int)n + 1);
                    g_xfn[g_xfn_n++].n = empty ? 0 : k;
                }
            }
        }
        while (*s && *s != '\n') s++;
        if (*s) s++;
    }
}
static int x_fn_known(const char *name) {   // its parameter count, -1: no function of that name
    if (!g_xfn_scanned) { g_xfn_scanned = 1; x_fn_scan_dir(); }
    for (int i = 0; i < g_xfn_n; i++) if (!lstrcmpiA(g_xfn[i].name, name)) return g_xfn[i].n;
    return -1;
}
static int x_var_known(const char *name) {
    if (!g_xfn_scanned) { g_xfn_scanned = 1; x_fn_scan_dir(); }
    for (int i = 0; i < g_xvn_n; i++) if (!lstrcmpiA(g_xvn[i], name)) return 1;
    return 0;
}
// THE NAMES CHECK while a file is read (tfile_parse): a name that is nothing known - no parameter, constant, [vars]
// entry of any file, game value or placeholder the code fills for this key - is told with its file and line (strict
// parsing: in the startup box); the key is kept, at run time the name fails what it is in (x_fail)
static const char *g_x_check_file, *g_x_check_ph;   // the file being read; " a b " the code's placeholders for the key
static int g_x_check_line;
static int x_name_valid(const char *name);   // diplo.c: a name x_builtin takes (them, epoch, them.lord.soldiers ...)
static const char *x_code_placeholders(const char *file, const char *sec, const char *key);   // texts.c
static int x_const(const char *name, struct xval *v);
static void x_name_check(const char *name) {
    struct xval t;
    char w[48];
    if (!g_x_check_file || strlen(name) > 40) return;
    wsprintfA(w, " %s ", name);
    if (x_check_par_has(name, (int)strlen(name)) || x_const(name, &t) || x_var_known(name) || x_name_valid(name) ||
        (g_x_check_ph && strstr(g_x_check_ph, w))) return;
    x_error("%s:%d: an unknown name: %s", g_x_check_file, g_x_check_line, name);
}

static int xnumeric(const char *s, double *n) {   // the whole text a number ("12", not " 12" or "12 ")
    char *e;
    if (!((*s >= '0' && *s <= '9') || *s == '-' || *s == '.')) return 0;
    *n = strtod(s, &e);
    return e != s && !*e;
}
static void xnothing(struct xval *v) { v->has_n = v->has_s = v->w = v->pl = v->ln = 0; v->n = 0; v->s[0] = v->miss[0] = 0; }
static void xnum(struct xval *v, double n) { v->has_n = 1; v->has_s = v->pl = v->ln = 0; v->w = 1; v->n = n; v->miss[0] = 0; }
static void xstr(struct xval *v, const char *s) {   // a text, only that: "12" in a file is no number
    lstrcpynA(v->s, s ? s : "", XVAL_CAP);
    v->has_s = 1;
    v->w = 1;
    v->has_n = 0;
    v->pl = v->ln = 0;
    v->miss[0] = 0;
}
// a placeholder the code fills ({tribute}, {minutes}): a text - and its number when it is one, the code's numbers
// coming as texts
static void xstr_or_num(struct xval *v, const char *s) { xstr(v, s); v->has_n = xnumeric(v->s, &v->n); }
static void xdual(struct xval *v, double n, const char *s) { xstr(v, s); v->has_n = 1; v->n = n; }
static int xis_num(const struct xval *v, double *n) { if (v->has_n) *n = v->n; return v->has_n; }
static double xto_num(const struct xval *v) { return v->has_n ? v->n : 0; }
static int xtrue(const struct xval *v) { return v->w && (v->pl || (v->has_n ? v->n != 0 : v->s[0] != 0)); }   // a player: true, whatever its name
// n to dec decimals, a half away from 0 (0.625 -> 0.63, 62.5 -> 63) - printf's own rounding takes a half to the even
// digit (0.62, 62), which nobody writing a text expects
static double x_round_dec(double n, int dec) {
    double m = 1, a = n < 0 ? -n : n;
    for (int i = 0; i < dec; i++) m *= 10;
    if (a * m >= 9e15) return n;
    a = (double)(long long)(a * m + 0.5) / m;
    return n < 0 ? -a : a;
}
static void xto_text(const struct xval *v, char *out, int cap) {
    if (v->has_s || !v->w) { lstrcpynA(out, v->w ? v->s : "", cap); return; }
    double r = v->n < 0 ? -v->n : v->n;
    if (r - (double)(long long)r < 1e-9 && r < 1e15) snprintf(out, (size_t)cap, "%lld", (long long)v->n);
    else {
        snprintf(out, (size_t)cap, "%.2f", x_round_dec(v->n, 2));
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
struct xp { const char *s; struct xctx *c; int err, skip, check; const char *why; int par; };   // par: in ( ), not in [ ]
static int x_syntax(const char *src, const char **why);
static int x_fills_ok(const char *text, const char **why);
static void xp_space(struct xp *p) { while (*p->s == ' ' || *p->s == '\t' || *p->s == '\n' || *p->s == '\r') p->s++; }   // line breaks: the built-in [vars]
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
// a draw: the display generator - in a decision (c->game) the game's, which every PC draws alike (persona.c)
static int x_rnd(const struct xctx *c, int n) { return c->game ? rnd_game(n) : rnd(n); }
static double x_rnd01(const struct xctx *c) { return x_rnd(c, 1 << 30) / (double)(1 << 30); }   // 0 <= x < 1
// a player who isn't there (lord without one), used without a check (lord ? .. / lord ?? .. / lord == them): a
// mistake like any other (x_fail) - a line built on a lord that isn't there makes no sense
static void x_missing(struct xctx *c, const char *who) {
    x_error("%s is used, but there is none here - check it first (%s ? ... / %s ?? ...): %.100s", who, who, who, c->top ? c->top : "");
    c->failed++;
    c->missed = 1;
}
static void x_use(struct xp *p, const struct xval *v) {
    if (!v->miss[0] || p->skip) return;
    x_missing(p->c, v->miss);
    p->c->missed = 0;
    p->err = 1;
    p->why = "a mistake while working it out";
}
// a mistake while working it out (MISTAKES WHILE WORKING OUT above): told once, and what it is in fails
static char g_x_last_fail[160];   // the last one, for texts eval (the log has each once only)
static void x_fail(struct xp *p, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_x_last_fail, sizeof g_x_last_fail, fmt, ap);
    va_end(ap);
    if (!p->c->fixed) x_error("%s: %.100s", g_x_last_fail, p->c->top ? p->c->top : "");   // fixed: ini.c tells it
    p->c->failed++;
    p->err = 1;
    p->why = "a mistake while working it out";
}
static int x_numeric_ok(struct xp *p, const struct xval *v) {   // v counts as a number: one, or nothing (0) - a text is a mistake
    if (!v->w || v->has_n) return 1;
    char t[XVAL_CAP];
    xto_text(v, t, sizeof t);
    x_fail(p, "a text (\"%.40s\") where a number belongs - number(x) makes a number of a text", t);
    return 0;
}
// a text against a number ("12" == 12, "9" < 10): no comparison - neither turns into the other
static int xmismatch(const struct xval *a, const struct xval *b) { return (!a->has_n && !b->has_s) || (!a->has_s && !b->has_n); }
static int xcmp(const struct xval *a, const struct xval *b) {   // <0, 0, >0: players the same one or not, numbers, else texts
    double x, y;
    if (a->pl && b->pl) return a->pl - b->pl;
    if (xmismatch(a, b)) return 2;   // unequal
    if (xis_num(a, &x) && xis_num(b, &y)) return x < y ? -1 : x > y;
    char l[XVAL_CAP], r[XVAL_CAP];
    xto_text(a, l, sizeof l);
    xto_text(b, r, sizeof r);
    return lstrcmpiA(l, r);
}
// epoch.<name>: an epoch's number, in every language the same (the names the game's own are in another language)
static const char *const k_epoch_const[15] = { "prehistoric", "stone", "copper", "bronze", "dark", "middle", "renaissance",
    "imperial", "industrial", "atomic_wwi", "atomic_wwii", "atomic_modern", "digital", "nano", "space" };
// units.<kind>: which units kills counts (diplo.c x_method) - a value of its own, its text its name
static const char *const k_units_const[6] = { "all", "army", "airforce", "marine", "military", "civilian" };
static int x_units_filter(const struct xval *v) {   // the kind a units.<kind> value stands for; -1: none
    if (!v->has_s || v->has_n || _strnicmp(v->s, "units.", 6)) return -1;
    for (int k = 0; k < 6; k++) if (!lstrcmpiA(v->s + 6, k_units_const[k])) return k;
    return -1;
}
static int color_index(const char *s);   // persona.c: a colour's index 0..16 by its name
static int x_const(const char *name, struct xval *v) {
    if (!_strnicmp(name, "color.", 6)) {   // color.red: a colour's number 1..17, as X.color has it
        int k = isalpha((unsigned char)name[6]) ? color_index(name + 6) : -1;
        if (k < 0) return 0;
        xnum(v, k + 1);
        return 1;
    }
    if (!_strnicmp(name, "units.", 6)) {
        for (int k = 0; k < 6; k++)
            if (!lstrcmpiA(name + 6, k_units_const[k])) { char t[24]; wsprintfA(t, "units.%s", k_units_const[k]); xstr(v, t); return 1; }
        return 0;
    }
    if (_strnicmp(name, "epoch.", 6)) return 0;
    for (int e = 0; e < 15; e++) if (!lstrcmpiA(name + 6, k_epoch_const[e])) { xnum(v, e + 1); return 1; }
    return 0;
}
static void x_nogame(struct xp *p, const char *name) {   // a game value or a player where there is no game yet
    x_fail(p, "%s: no game yet - only constants, chance(), rnd(), language and [vars] here", name);
}
static void xp_ident(struct xp *p, const char *name, struct xval *v) {
    struct xctx *c = p->c;
    if (p->skip) { if (p->check) x_name_check(name); xnothing(v); return; }
    if (c->fixed) { x_fail(p, "%s: a name - a setting is a number (35%%, 2 * 60, min(...))", name); xnothing(v); return; }
    for (int i = 0; i < c->fnp; i++)   // a parameter of the function being worked out
        if (!strcmp(c->fpn[i], name)) { *v = c->fpv[i]; return; }
    for (int i = 0; i < c->np; i++)
        if (!strcmp(c->pn[i], name)) { xstr_or_num(v, c->pv[i]); return; }
    for (int k = 0; k < 3; k++) {
        const char *def = xvars_get(c->vars[k], name);
        if (!def) continue;
        if (c->depth >= 6) { c->circle = 1; p->err = 1; return; }   // vars naming each other in a circle
        const char *const *fpn = c->fpn;   // a variable sees no parameters (only its own function's body does)
        const struct xval *fpv = c->fpv;
        int fnp = c->fnp;
        c->fnp = 0;
        c->depth++;
        struct xp q = { def, c, 0, 0, 0, NULL, 0 };
        xp_expr(&q, v);
        xp_space(&q);
        if (q.err || *q.s || c->circle) p->err = 1;
        c->depth--;
        c->fpn = fpn; c->fpv = fpv; c->fnp = fnp;
        return;
    }
    if (x_const(name, v)) return;
    if (c->nogame && lstrcmpiA(name, "language") && (x_is_value(name) || strchr(name, '.'))) { x_nogame(p, name); return; }
    if (x_builtin(c, name, v)) return;
    x_fail(p, !strcmp(name, "name") ? "an unknown name: name - the other player is them (them.name)" : "an unknown name: %s", name);
}
// a function's body (def), its parameters (par "a,b)") bound to the arguments: they alone are its names besides the
// game's values and the [vars] - a function calling itself ends at the nesting limit like vars in a circle
static void x_call(struct xp *p, const char *def, const char *par, const struct xval *a, int na, struct xval *v) {
    struct xctx *c = p->c;
    char names[8][24];
    const char *pn[8];
    int n = 0;
    while (*par && *par != ')' && n < 8) {
        int k = 0;
        while (*par && *par != ',' && *par != ')' && k < 23) names[n][k++] = *par++;
        names[n][k] = 0;
        pn[n] = names[n];
        n++;
        if (*par == ',') par++;
    }
    if (n != na) { p->err = 1; return; }
    if (c->depth >= 6) { c->circle = 1; p->err = 1; return; }
    const char *const *fpn = c->fpn;
    const struct xval *fpv = c->fpv;
    int fnp = c->fnp;
    c->fpn = pn; c->fpv = a; c->fnp = n;
    c->depth++;
    struct xp q = { def, c, 0, 0, 0, NULL, 0 };
    xp_expr(&q, v);
    xp_space(&q);
    if (q.err || *q.s || c->circle) p->err = 1;
    c->depth--;
    c->fpn = fpn; c->fpv = fpv; c->fnp = fnp;
}
static void xp_primary(struct xp *p, struct xval *v) {
    xp_space(p);
    const char *s = p->s;
    xnothing(v);
    if (++p->c->level > 24) { p->c->level--; p->err = 1; return; }
    if (*s == '(') {
        p->s++;
        p->par++;
        xp_expr(p, v);
        p->par--;
        if (!xp_op(p, ")")) p->err = 1;
    } else if (*s == '"' || *s == '\'') {   // a string: its placeholders filled
        char t[XVAL_CAP], f[XVAL_CAP];
        const char *why;
        if (x_string(s, t, sizeof t, &p->s, &why) < 0) { p->err = 1; goto out; }
        // ("a" "b"): strings side by side in ( ) are one - a long text over several lines (in a list a missing comma
        // stays a mistake)
        for (; p->par; ) {
            xp_space(p);
            if (*p->s != '"' && *p->s != '\'') break;
            char t2[XVAL_CAP];
            size_t n = strlen(t);
            if (x_string(p->s, t2, sizeof t2, &p->s, &why) < 0) { p->err = 1; goto out; }
            lstrcpynA(t + n, t2, (int)(sizeof t - n));
        }
        if (p->skip) { xstr(v, ""); if (p->check && !x_fills_ok(t, &p->why)) p->err = 1; }
        else {
            int f0 = p->c->failed;
            x_fill(f, sizeof f, t, p->c);
            if (p->c->failed > f0) { p->err = 1; p->why = "a mistake while working it out"; goto out; }   // told in x_fill
            xstr(v, f);
        }
    } else if (*s == '[' && p->c->fixed && !p->skip) {   // a setting: one number, no random item
        x_fail(p, "a list - a setting is one number");
        p->err = 1;
    } else if (*s == '[') {   // a list: one of its items - each of them as often as the items it was chosen from
        struct xval it;
        int total = 0, par = p->par, idx[XL_ITEMS], n = 0, lost = 0;
        p->s++;
        p->par = 0;
        for (;;) {
            if (xp_op(p, "]")) break;
            const char *item = p->s;
            int f0 = p->c->failed;
            xp_expr(p, &it);
            if (!p->err) x_use(p, &it);
            if (p->err && !p->skip && p->c->failed > f0 && !p->c->circle) {   // a mistake (told): the item left out -
                p->err = 0;                                                    // read again, not worked out, to its end
                p->why = NULL;
                p->s = item;
                p->skip = 1;
                xp_expr(p, &it);
                p->skip = 0;
                xnothing(&it);
            }
            if (p->err) goto out;
            if (!p->skip && it.w > 0) {
                total += it.w;
                if (x_rnd(p->c, total) < it.w) *v = it;
                if (it.ln) for (int k = 0; k < it.ln; k++) { if (n < XL_ITEMS) idx[n++] = g_xl_idx[it.li + k]; else lost = 1; }   // its items
                else if (it.w > 1 || n == XL_ITEMS || g_xl_nval == XL_VALS) lost = 1;   // a list kept whole no more
                else { g_xl_val[g_xl_nval] = it; idx[n++] = g_xl_nval++; }
            }
            if (xp_op(p, ",")) continue;
            if (!xp_op(p, "]")) p->err = 1;
            break;
        }
        p->par = par;
        v->w = total;
        v->ln = 0;
        if (!total) xnothing(v);
        else if (!lost && g_xl_nidx + n <= XL_IDX) {
            memcpy(g_xl_idx + g_xl_nidx, idx, (size_t)n * sizeof *idx);
            v->li = g_xl_nidx;
            v->ln = n;
            g_xl_nidx += n;
        } else if (!g_xl_told) { g_xl_told = 1; x_error("a list too long to keep its items (len, first, item ... see one): %.100s", p->c->top ? p->c->top : ""); }
    } else if ((*s >= '0' && *s <= '9') || (*s == '.' && s[1] >= '0' && s[1] <= '9')) {
        char *e;
        double n = strtod(s, &e);
        if (*e == '%') { n /= 100; e++; }   // 70% = 0.7
        xnum(v, n);
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
                !lstrcmpiA(name, "abs") && na == 1 ? 3 : !lstrcmpiA(name, "round") && na == 1 ? 4 :
                !lstrcmpiA(name, "rnd") && na <= 1 ? 5 : !lstrcmpiA(name, "floor") && na == 1 ? 6 :
                !lstrcmpiA(name, "len") && na == 1 ? 7 : !lstrcmpiA(name, "first") && na == 1 ? 8 :
                !lstrcmpiA(name, "item") && na == 2 ? 9 : !lstrcmpiA(name, "contains") && na == 2 ? 10 :
                !lstrcmpiA(name, "map_range") && na == 3 ? 11 : !lstrcmpiA(name, "str") && na == 1 ? 12 :
                !lstrcmpiA(name, "number") && na == 1 ? 13 : !lstrcmpiA(name, "log2") && na == 1 ? 14 : 0;
        for (int i = 0; f && i < na; i++) x_use(p, &a[i]);   // a function's arguments (a relation checks its players)
        if (p->err) goto out;
        if (p->c->fixed && !p->skip && (f == 1 || f == 5 || !f)) {   // a setting: fixed - no draws, no [vars] functions
            x_fail(p, "%s(): a setting is a fixed number (no chance(), rnd() or names)", name);
            goto out;
        }
        if (f == 1 && a[0].has_n && a[0].n > 1 && (p->check || !p->skip)) {   // chance(70): meant 70%
            if (p->check) { p->err = 1; p->why = "chance takes a probability, 0 to 1: chance(70%) or chance(0.7)"; goto out; }
            x_fail(p, "chance(%g): a probability is 0 to 1 - chance(70%%) or chance(0.7)", a[0].n);
            goto out;
        }
        if (!p->skip && f && (f <= 6 || f == 14))   // the number functions: numbers (or nothing) only
            for (int i = 0; i < na; i++) if (!x_numeric_ok(p, &a[i])) goto out;
        if (!p->skip && (f == 9 ? !x_numeric_ok(p, &a[1]) : f == 11 && !x_numeric_ok(p, &a[0]))) goto out;
        if (!p->skip && f == 14 && !(xto_num(&a[0]) > 0)) { x_fail(p, "log2(%g): only of a number above 0", xto_num(&a[0])); goto out; }
        if (!p->skip && f == 11)
            for (int i = 0; i < xl_n(&a[1]); i++) if (!x_numeric_ok(p, xl_at(&a[1], i))) goto out;
        if (!f && !strchr(name, '.')) {   // a function of [vars]: name(a, b) = ...
            const char *par = NULL, *def = NULL;
            for (int k = 0; k < 3 && !def; k++) def = xvars_fn(p->c->vars[k], name, &par);
            int want = def ? (*par == ')' ? 0 : 1) : x_fn_known(name);
            for (const char *q = par; def && *q && *q != ')'; q++) want += *q == ',';
            if (want >= 0) {
                static char why[80];
                if (want != na) { snprintf(why, sizeof why, "%s takes %d argument%s", name, want, want == 1 ? "" : "s"); p->err = 1; p->why = why; goto out; }
                if (p->skip) { xnothing(v); goto out; }
                if (!def) { snprintf(why, sizeof why, "%s: a function of another file's [vars]", name); p->err = 1; p->why = why; goto out; }
                x_call(p, def, par, a, na, v);
                goto out;
            }
        }
        if (!f) {   // a relation: them.opinion(me)
            if (p->c->nogame && !p->skip) x_nogame(p, name);
            else if (!x_method(p->c, name, a, na, p->skip ? NULL : v)) { p->err = 1; p->why = "an unknown function or relation (or the wrong number of arguments)"; }
            else if (p->skip) xnothing(v);
        }
        else if (p->skip) xnothing(v);
        else if (f == 1) xnum(v, x_rnd01(p->c) < xto_num(&a[0]));   // a probability: chance(70%), chance(0.7)
        else if (f == 2) {
            int mx = !lstrcmpiA(name, "max");
            double r = xto_num(&a[0]);
            for (int i = 1; i < na; i++) { double x = xto_num(&a[i]); if (mx ? x > r : x < r) r = x; }
            xnum(v, r);
        } else if (f == 3) { double x = xto_num(&a[0]); xnum(v, x < 0 ? -x : x); }
        else if (f == 4) { double x = xto_num(&a[0]); xnum(v, (double)(long long)(x < 0 ? x - 0.5 : x + 0.5)); }
        else if (f == 6) { double x = xto_num(&a[0]), r = (double)(long long)x; xnum(v, r > x ? r - 1 : r); }   // down: floor(-1.5) -2
        else if (f == 12 || f == 13) {   // str(x): its text only (epoch: "Copper Age"); number(x): its number - a text
            double x;                        // that is one ("12"), else nothing (number(t) ?? 0)
            char t[XVAL_CAP];
            if (!a[0].w) ;
            else if (f == 12) { xto_text(&a[0], t, sizeof t); xstr(v, t); }
            else if (a[0].has_n) xnum(v, a[0].n);
            else if (xnumeric(a[0].s, &x)) xnum(v, x);
        }
        else if (f == 7) xnum(v, a[0].ln ? a[0].ln : a[0].w);   // len: how many items (one value 1, nothing 0)
        else if (f == 8 || f == 9 || f == 11) {   // an item: first(l), item(l, i) from 1, map_range(x, limits, values)
            const struct xval *l = &a[f == 11 ? 2 : 0];
            int i = f == 8 ? 0 : f == 9 ? (int)xto_num(&a[1]) - 1 : 0;
            if (f == 11) {   // the value of the first limit x is below, the last value past them all
                double x = xto_num(&a[0]);
                while (i < xl_n(&a[1]) && x >= xto_num(xl_at(&a[1], i))) i++;
            }
            if (i >= 0 && i < xl_n(l)) { *v = *xl_at(l, i); v->w = 1; v->ln = 0; }
        } else if (f == 10) {   // contains(l, x): one of its items equal to x (as ==)
            int hit = 0;
            for (int i = 0; i < xl_n(&a[0]) && !hit; i++) hit = a[1].w && !xcmp(xl_at(&a[0], i), &a[1]);
            xnum(v, hit);
        }
        else if (f == 14) xnum(v, det_log2f((float)xto_num(&a[0])));   // the same bits on every PC (diplo.c)
        else xnum(v, x_rnd01(p->c) * (na ? xto_num(&a[0]) : 1));   // rnd(n): 0 <= x < n, rnd(): 0 <= x < 1
    } else p->err = 1;
out:
    if (p->c->missed) { p->c->missed = 0; p->err = 1; p->why = "a mistake while working it out"; }   // told (x_missing)
    p->c->level--;
}
static void xp_unary(struct xp *p, struct xval *v) {
    int neg = xp_op(p, "-"), inv = !neg && xp_word(p, "not");
    if (neg || inv) {   // an operator counts in the nesting too: "- - - ... x" recursed without a limit
        if (++p->c->level > 24) { p->c->level--; p->err = 1; xnothing(v); return; }
        xp_unary(p, v);
        if (neg && !p->err) { x_use(p, v); if (p->err || (!p->skip && !x_numeric_ok(p, v))) { p->c->level--; return; } xnum(v, -xto_num(v)); }
        else xnum(v, !xtrue(v));
        p->c->level--;
        return;
    }
    if (*p->s == '!' && p->s[1] != '=') { p->err = 1; p->why = "! - write not"; xnothing(v); return; }
    xp_primary(p, v);
}
static void xp_mul(struct xp *p, struct xval *v) {
    xp_unary(p, v);
    while (!p->err) {
        xp_space(p);
        if (*p->s == '%') { p->err = 1; p->why = "% belongs right after a number: 70% is 0.7 (there is no remainder)"; return; }
        int op = xp_op(p, "*") ? '*' : xp_op(p, "/") ? '/' : 0;
        if (!op) return;
        struct xval b;
        xp_unary(p, &b);
        if (p->err || p->skip) { xnothing(v); continue; }
        x_use(p, v);
        if (!p->err) x_use(p, &b);
        if (p->err || !x_numeric_ok(p, v) || !x_numeric_ok(p, &b)) return;
        double x = xto_num(v), y = xto_num(&b);
        if (op == '/' && y == 0) { x_fail(p, "a division by 0"); return; }
        xnum(v, op == '*' ? x * y : x / y);
    }
}
static void xp_add(struct xp *p, struct xval *v) {
    xp_mul(p, v);
    while (!p->err) {
        int op = xp_op(p, "+") ? '+' : xp_op(p, "-") ? '-' : 0;
        if (!op) return;
        struct xval b;
        xp_mul(p, &b);
        if (p->err || p->skip) { xnothing(v); continue; }
        x_use(p, v);
        if (!p->err) x_use(p, &b);
        if (p->err) return;
        double x, y;
        if (op == '-') {
            if (!x_numeric_ok(p, v) || !x_numeric_ok(p, &b)) return;
            xnum(v, xto_num(v) - xto_num(&b));
        }
        else if (!v->w || !b.w) { if (!v->w) *v = b; v->ln = 0; }   // nothing: 0 to a number, "" to a text
        else if (xis_num(v, &x) && xis_num(&b, &y)) xnum(v, x + y);   // numbers (an epoch is one too: epoch + 1)
        else if (v->has_s && b.has_s) {   // texts joined ("Age: " + epoch: its text)
            char l[XVAL_CAP], r[XVAL_CAP], j[XVAL_CAP];
            xto_text(v, l, sizeof l);
            xto_text(&b, r, sizeof r);
            snprintf(j, sizeof j, "%s%s", l, r);
            xstr(v, j);
        } else {   // a text and a number: neither turns into the other
            char t[XVAL_CAP];
            xto_text(v->has_s ? v : &b, t, sizeof t);
            x_fail(p, "a text (\"%.40s\") and a number added - \"...{x}\" or str(x) for a text, number(t) for a number", t);
            return;
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
    if (p->err) return;
    if (!v->w || !b.w) { xnum(v, 0); return; }   // nothing compares with nothing: false
    if (!(v->pl && b.pl) && xmismatch(v, &b)) {
        char l[XVAL_CAP];
        xto_text(v->has_s ? v : &b, l, sizeof l);
        x_fail(p, "a text (\"%.40s\") compared with a number - a number has no quotes, number(t) makes one", l);
        return;
    }
    int d = xcmp(v, &b);
    xnum(v,op == '<' ? d < 0 : op == '>' ? d > 0 : op == 'l' ? d <= 0 : op == 'g' ? d >= 0 : op == '=' ? d == 0 : d != 0);
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
    struct xp p = { src, c, 0, 0, 0, NULL, 0 };
    int top = !c->top;
    if (top) c->top = src;
    if (!g_xl_busy++) g_xl_nval = g_xl_nidx = 0;   // the outermost: the last one's lists are done with
    xp_expr(&p, v);
    g_xl_busy--;
    xp_space(&p);
    int ok = !p.err && !*p.s && !c->circle;
    if (ok) { x_use(&p, v); ok = !p.err; }   // a player who isn't there, written in the text
    if (top) c->top = NULL;
    if (c->circle && !c->depth) { c->circle = 0; x_log_bad("[vars] naming each other in a circle", src); }
    return ok;   // 0 and c->failed up: a mistake while working it out, told
}

// an ini file's number (ini.c): a constant expression - numbers, %, + - * / ( ), min, max, abs, round, floor, log2;
// 0: a mistake, *why (not logged here)
static int x_const_eval(const char *src, double *out, const char **why) {
    struct xctx c;
    memset(&c, 0, sizeof c);
    c.found = 1; c.nogame = 1; c.fixed = 1;
    struct xval v;
    g_x_last_fail[0] = 0;
    if (!x_eval(src, &c, &v)) { *why = g_x_last_fail[0] ? g_x_last_fail : "not a number or an expression"; return 0; }
    if (!v.w || !v.has_n || v.has_s || v.pl) { *why = "a number expected"; return 0; }
    *out = v.n;
    return 1;
}
// the syntax of an expression, nothing worked out (no game needed: the files are checked when read); 0: a mistake, *why
// what it is when known
static int x_syntax(const char *src, const char **why) {
    struct xctx c = { 0 };
    c.found = 1;   // no players looked up
    struct xp p = { src, &c, 0, 1, 1, NULL, 0 };
    struct xval v;
    xp_expr(&p, &v);
    xp_space(&p);
    if (!p.err && *p.s) { p.err = 1; if (!p.why) p.why = "something after the expression"; }
    if (p.err) *why = p.why ? p.why : "not an expression";
    return !p.err;
}
static const char *x_close(const char *s);
// A placeholder's FORMAT: {x:%} {x:.1%} {x:.2} - after a ':' that belongs to no '?' (outside strings and brackets: the
// ternary's own colons pair with its ?s), how its number is written: % as a share (0.5 -> 50%), .N with N decimals
// (0..6; both: .1% -> 62.5%). Without one a number is written as it is (up to 2 decimals). A number is never
// written as a share by itself: 50% in an expression is only another way to write 0.5.
// A choice in front of a format must be in ( ): {(a ? 1 : 0):%} - written {a ? 1 : 0:%} the format reads as the 0's
// only (likewise {a ?? b:%}); *choice: a ? or ?? outside brackets before the format's ':'
static int x_fmt_at(const char *s, int n, int *choice) {   // the format's ':' in s[0..n), -1: none
    int depth = 0, q = 0;
    *choice = 0;
    for (int i = 0; i < n; i++) {
        char ch = s[i];
        if (ch == '"' || ch == '\'') {   // a string: to its closing quote (\" inside)
            for (i++; i < n && s[i] != ch; i++) if (s[i] == '\\' && i + 1 < n) i++;
            continue;
        }
        if (ch == '(' || ch == '[' || ch == '{') depth++;
        else if (ch == ')' || ch == ']' || ch == '}') depth--;
        else if (depth) ;
        else if (ch == '?') { *choice = 1; if (i + 1 < n && s[i + 1] == '?') i++; else q++; }   // ?? is no ternary
        else if (ch == ':') { if (q) q--; else return i; }
    }
    return -1;
}
static int x_fmt_parse(const char *f, int *dec, int *pct) {   // "%", ".N", ".N%"; 0: no format
    while (*f == ' ') f++;
    *dec = -1; *pct = 0;
    if (*f == '.') { if (f[1] < '0' || f[1] > '6') return 0; *dec = f[1] - '0'; f += 2; }
    if (*f == '%') { *pct = 1; f++; }
    while (*f == ' ') f++;
    return !*f && (*dec >= 0 || *pct);
}
static void x_fmt_num(double n, int dec, int pct, char *out, int cap) {
    if (pct) n *= 100;
    if (dec < 0) dec = 0;
    snprintf(out, (size_t)cap, "%.*f%s", dec, x_round_dec(n, dec), pct ? "%" : "");
    const char *d = out + (*out == '-');   // no "-0", "-0.0%": a minus only on what isn't 0 as written
    while (*d == '0' || *d == '.') d++;
    if (*out == '-' && (!*d || *d == '%')) memmove(out, out + 1, strlen(out));
}
static const char *const k_fmt_why = "a placeholder's format is %, .1%, .2 ... ({share:%}, {x:.1})";
static const char *const k_fmt_choice = "a choice before a format goes in ( ): {(a ? 1 : 0):%}";
// a text's {placeholders} that are expressions (not a plain name): their syntax - and a plain name's (x_name_check)
static int x_fills_ok(const char *text, const char **why) {
    for (const char *s = strchr(text, '{'); s; s = strchr(s + 1, '{')) {
        const char *close = x_close(s);
        char src[400];
        int n = close ? (int)(close - s - 1) : -1;
        if (n < 0 || n >= (int)sizeof src) continue;
        memcpy(src, s + 1, (size_t)n);
        src[n] = 0;
        int choice, f = x_fmt_at(src, n, &choice), dec, pct;
        if (f >= 0) {
            if (!x_fmt_parse(src + f + 1, &dec, &pct)) { *why = k_fmt_why; return 0; }
            if (choice) { *why = k_fmt_choice; return 0; }
            while (f > 0 && src[f - 1] == ' ') f--;
            src[n = f] = 0;
        }
        if (strpbrk(src, " <>=?\"'(+[!&|")) { if (!x_syntax(src, why)) return 0; }
        else if (n && strspn(src, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.") == (size_t)n && !isdigit((unsigned char)*src))
            x_name_check(src);
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
// text); a mistake in one (an unknown name: "Hi {foo}") writes nothing, c->failed up - a string in an expression
// fails with it; braces that hold no expression stay as written. Returns the length.
static int x_fill(char *out, int cap, const char *tmpl, struct xctx *c) {
    int o = 0, top = !c->top;
    if (top) c->top = tmpl;
    for (const char *s = tmpl; *s && o < cap - 1;) {
        const char *close = *s == '{' ? x_close(s) : NULL;
        if (!close) { out[o++] = *s++; continue; }
        char src[400], val[XVAL_CAP];
        int n = (int)(close - s - 1);
        const char *rep = NULL;
        int fmt = 0, dec = -1, pct = 0;
        if (n > 0 && n < (int)sizeof src) {
            memcpy(src, s + 1, (size_t)n);
            src[n] = 0;
            int choice, f = x_fmt_at(src, n, &choice);
            if (f >= 0) {   // {x:%}: the expression before the ':', its number written so
                fmt = x_fmt_parse(src + f + 1, &dec, &pct) ? 1 : -1;
                if (fmt > 0 && choice) { x_error("%s: {%.60s}", k_fmt_choice, src); fmt = -2; }
                while (f > 0 && src[f - 1] == ' ') f--;
                src[f] = 0;
            }
            for (int i = 0; i < c->np && !rep; i++) if (!strcmp(c->pn[i], src)) rep = c->pv[i];
            if (fmt < 0) { if (fmt == -1) x_error("%s: {%.60s}", k_fmt_why, src); c->failed++; val[0] = 0; rep = val; }
            else if (rep && fmt) {   // the code's placeholder: a number only ("12" is one here)
                char *e;
                double x = strtod(rep, &e);
                if (e == rep || *e) { x_error("{%s:...}: a format for a number, %s is none", src, rep); c->failed++; val[0] = 0; }
                else x_fmt_num(x, dec, pct, val, sizeof val);
                rep = val;
            }
            if (!rep && c->depth < 6) {
                struct xval v;
                int f0 = c->failed;
                c->depth++;
                int ok = x_eval(src, c, &v) && !c->circle;
                c->depth--;
                if (c->circle && !c->depth) { c->circle = 0; x_log_bad("[vars] naming each other in a circle", src); }
                if (ok && fmt && !(v.w && v.has_n && !v.pl)) {   // {name:%}: a text, a player, nothing - a mistake
                    x_error("{%s:...}: a format for a number, this is none", src);
                    c->failed++;
                    val[0] = 0; rep = val;
                }
                else if (ok && fmt) { x_fmt_num(v.n, dec, pct, val, sizeof val); rep = val; }
                else if (ok) { xto_text(&v, val, sizeof val); rep = val; }
                else if (c->failed > f0) { val[0] = 0; rep = val; }   // a mistake (told): nothing written ("Hi {foo}")
                else if (strpbrk(src, " <>=?\"'(+[")) { c->bad++; x_log_bad("not an expression", src); }
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
static int tl_key_line(const char *t) {   // a line that starts a key ("name =", "name(a, b) =", not "a == b") or a section
    if (*t == '[') return 1;   // a section: [ at the line's start (an indented [ is a list in a value)
    while (*t == ' ' || *t == '\t') t++;
    const char *k = t;
    while (*t && (isalnum((unsigned char)*t) || strchr("_:.-", *t))) t++;
    if (t == k) return 0;
    if (*t == '(') {   // a function's parameters: names and commas
        for (t++; *t && *t != ')'; t++) if (!isalnum((unsigned char)*t) && !strchr("_, ", *t)) return 0;
        if (*t++ != ')') return 0;
    }
    while (*t == ' ' || *t == '\t') t++;
    return *t == '=' && t[1] != '=';
}
// "a" or ("a" <line break> "b"): a value of strings only, side by side in ( ) - their text joined into t
// (cap: the source's length + 1 is enough); 0: something else
static int x_fold(const char *r, char *t, int cap) {
    const char *s = r, *e, *why;
    int par = *s == '(', o = 0, n = 0;
    if (par) s++;
    for (;;) {
        while (*s == ' ') s++;
        if (*s != '"' && *s != '\'') break;
        int k = x_string(s, t + o, cap - o, &e, &why);
        if (k < 0) return 0;
        o += k;
        n++;
        s = e;
    }
    if (par) { if (*s != ')') return 0; s++; while (*s == ' ') s++; }
    return n && !*s && (par || n == 1);   // more than one: in ( ) only
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
    const char *why;
    char *t = malloc(o + 1);
    double n;
    if (x_fold(r, t, (int)o + 1)) { v->kind = TV_STRING; v->text = t; }   // "a", "a" "b", ("a" "b")
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
            char key[160];
            int n = 0;
            while (*l.s && (isalnum((unsigned char)*l.s) || strchr("_:.-", *l.s)) && n < 47) key[n++] = *l.s++;
            key[n] = 0;
            if (n && *l.s == '(') {   // a function: name(a, b) - its parameters into the key as "name(a,b)"
                const char *why = NULL;
                int np = 0;
                key[n++] = *l.s++;
                for (;;) {
                    while (*l.s == ' ') l.s++;
                    if (*l.s == ')' && !np) break;
                    char pnm[24];
                    int k = 0;
                    while ((isalnum((unsigned char)*l.s) || *l.s == '_') && k < 23) pnm[k++] = *l.s++;
                    pnm[k] = 0;
                    if (!k || isdigit((unsigned char)pnm[0])) { why = "a parameter must be a name: name(a, b) = ..."; break; }
                    char look[26];
                    wsprintfA(look, "%s,", pnm);
                    if (strstr(key + strcspn(key, "(") + 1, look) || ++np > 8) { why = np > 8 ? "at most 8 parameters" : "a parameter twice"; break; }
                    if (n + k + 2 < (int)sizeof key) { memcpy(key + n, pnm, (size_t)k); n += k; key[n++] = ','; key[n] = 0; }
                    while (*l.s == ' ') l.s++;
                    if (*l.s == ',') { l.s++; continue; }
                    if (*l.s != ')') why = "name(a, b) = ...: a ) missing";
                    break;
                }
                if (!why) { l.s++; if (key[n - 1] == ',') n--; key[n++] = ')'; key[n] = 0; }
                else { tl_error(&l, why); n = -1; }
            }
            if (n >= 0) tl_blank(&l, 0);
            if (n < 0) ;   // (its mistake told)
            else if (!n || *l.s != '=') tl_error(&l, "no key = value");

            else {
                l.s++;
                struct tval v;
                g_x_check_par = strchr(key, '(') ? strchr(key, '(') + 1 : NULL;
                g_x_check_file = file;
                g_x_check_line = l.line;
                g_x_check_ph = x_code_placeholders(file, section, key);
                tl_value(&l, &v);
                g_x_check_par = NULL;
                g_x_check_file = NULL;
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
