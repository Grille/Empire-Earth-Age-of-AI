// THE TEXT FILES (<game>\aimod_ai\*.eeai: the personalities; mod\*.eeai: computer, prompt, system) and the EXPRESSIONS in them.
// A file: "name = value" lines - a name the file's lines use (or, for a few names, what the file is about), records
// "lines = { key = value, ... }" (a comma after each entry; the keys the code reads - another record is the file's
// own name, its entries name.key), include("pack_base.eeai"),
// register_personality() - and comments from # to the line's end (outside a string). A value is an expression - over
// several lines while a ( [ or { is open:
//   "text"                          a string; \" \\ \n escapes. {expressions} in it - {them} -
//                                   {opinion > 30 ? "friend" : "stranger"}: the quotes inside the braces are the
//                                   expression's, not the string's end; a string stays on its line
//   ("a "  <line break>  "b")       strings side by side in ( ): one string (a value of only these: a string, also
//                                   where a key takes strings only - x_fold); in [ ] a missing comma stays a mistake
//   ["a", "b", ...]                 a list: a collection of values - what is done with it is its consumer's (a line,
//                                   a {placeholder}, pick(l): one item at random; contains, first, item, len: it whole)
//   allied ? "Welcome back."        without ":" - nothing when the condition doesn't hold
//   85, opinion > 30 ? "a" : "b"    numbers, any expression
// A list leaves out what is nothing and picks among the rest, a list in a list counting with each of its items (as
// if it stood there item by item); a list with nothing left is nothing. a ?? b: a, unless it is nothing. A
// personality's line is such a value: [allied ? "Welcome back.", opinion < -50 ? angry] ?? ["Greetings.", "Hello."]
// A mistake is logged with the file and line (# texts:) and that key is skipped.
// The expressions: numbers (70% is 0.7 - % only right after a number), "texts" (or 'texts', the same rules), lists,
// names (a placeholder of the text, a vars entry, a value of the host's - script/host.h), ( ), + - * /,
// < <= > >= == !=, and or not (only the words: && || ! are a mistake), a ? b : c, a ? b, a ?? b, the functions
// chance(p) (true with the probability p, 0..1: chance(70%)), rnd(n) (0 <= x < n, n 1 when left out), min, max, abs,
// round, floor (down to a whole number), log2 (of a number above 0), the lists' len(l), first(l), item(l, i) (from 1), contains(l, x) and
// map_range(x, [limits], [values]) (the value of the first limit x is below, the last past them all - a value no
// list is a list of one), str(x) / number(x) (its text only / its number, a text's when it is one, else nothing), and
// the host's relations: them.opinion(me) (script/host.h method). A player (me, them, ...) is a value: its
// name, equal only to itself. A vector, vec(x, y), vec(x, y, z), vec(x, y, z, w), is one value of 2 to 4 numbers (a
// place, a colour - not a list: a list's items are values each): home.x .y .z .w its numbers (of a parameter,
// variable or vars entry; after a call or ( ) too: step(home).x, (a + b).y), + - two vectors of as many numbers, * a vector and a number, / a vector by a number, == !=
// (no order), distance(a, b); rgb(r, g, b) a colour, 0 to 255 each, as the vector of those / 255 (vec(r, g, b) takes
// the 0 to 1 itself); in a text (12, 40), a command's argument its numbers one each - anything else with one a mistake.
// A record, { a = 1, b = "x", c = { ... } }, is one value of named members (a comma after each, the last's may be left
// out; each worked out when the record is): r.a, r.c.d, f().a its members - one it doesn't have a mistake, has(r,
// "a") checks; in a text {a = 1, b = x}; no comparison (its members are compared), no arithmetic. A file's own record
// (colors = { dark = ... }) is its names colors.dark ... (each worked out where it is used, like any name of the
// file's), colors the whole record.
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
// empty; nothing is false and an empty text. Only what is chosen is worked out (?: ?? and or). A placeholder that is
// not a valid expression stays in the text as written (logged); \{ \} in a string are braces that are text.
// FUNCTIONS AND STATEMENTS: (a, b) => value, (i) => { statements } is a function, a value like any other; called by
// the name it is in (row(3)), its parameters and the names of the code around where it was made visible (those read
// only: frames, x_call_value), the values it made freed when it returns but what it hands back (never a function made
// in it). Statements (a block's, a script's - one a line): name = value, return [value], an expression. The text
// files hold names, records and the declarations include(), register_personality() only - nothing runs from the
// top: their values are worked out where the code needs them, for whoever speaks to whom - and nothing loops: a text
// can't hang the game. Scripts (scripts.c,
// docs/scripts.md) add the commands, wait, wait_until, every (bounded), for (bounded), player(n), time, stop and 20s / 19.5min,
// and triggers: trigger({ when, run, loop, for, name }), after(t, f), enable(t), disable(t) (TRIGGERS below).
// MISTAKES WHILE WORKING OUT (x_fail; the file was fine, the value isn't): an unknown name, a text in arithmetic
// ("a" * 2), a text and a number added or compared ("12" + 1, "12" == 12), chance(70), a player who isn't there used
// without a check (lord.soldiers without a lord - x_missing). Nothing is guessed: the
// mistake is logged (once) and fails what it is in - up to the nearest list item, which is left out, else the whole
// value: a line then falls back like one with nothing to say, a {placeholder} writes nothing. Not "nothing": that
// would make "not foo" true. The names are checked when the files are read too (x_name_check) - told, the key kept.
// THE ENGINE: an expression is parsed once into a tree (x_parse; cached by its text, x_compiled) and the tree is
// worked out (xe); the file check (x_syntax) is the same parser with the checks the files get when they are read, in
// the order they appear. The values' texts and lists live in a region (x_alloc) that the outermost evaluation starts
// afresh. Included by eeaimod.c before persona.c.
#define XVARS_MAX 48
#define XVAL_CAP 512   // the callers' text buffers (xto_text cuts there); a value's own text has no limit
// val: an expression's source; dotted: a file record's member among the keys (colors.dark) - else no name is
// looked up as one (x_vars_group, a member path's head: what every dotted name would pay for)
// first, next: the keys by their name's hash (an index + 1, 0 none) - every name worked out looks itself up here
#define XVARS_HASH 64
struct xvars { int n, dotted; char *key[XVARS_MAX], *val[XVARS_MAX]; unsigned char first[XVARS_HASH], next[XVARS_MAX]; };
struct xval;
struct xctx {
    void *pl, *other, *third;               // the host's: the speaker, them, a third - them NULL: the local player
    const char *const *fpn;                 // a function's parameters while its body is worked out (x_call)
    const struct xval *fpv;
    int fnp;
    int found;                              // pl/other/third looked up (the host's value)
    const char *const *pn, *const *pv;      // the text's placeholders
    int np;
    const struct xvars *vars[3];            // searched in order: the personality's, the file's
    int depth, level, failed, bad, circle;  // nesting: vars in vars, the parser's levels (all together); mistakes while
                                            // working it out (x_fail, told); not an expression; too deep
    int nogame;                             // before the game (a personality's color): its values and players are mistakes
    int game;                               // a decision (decide): it changes the game - draws from the game's generator
    int fixed;                              // an ini file's number (ini.c): no names, no draws, no lists - told quietly
    int missed;                             // x_missing from the game's values (diplo.c): the primary fails
    int script;                             // a script (scenarios): its functions (wait, wait_until, every, for ...) and commands
    int calls;                              // functions called inside each other now
    const char *top;                        // the source being worked out (for the mistakes' messages)
};
// a number, a text or both - or nothing (w 0); w: how many items it was chosen from (a list's weight in a list);
// obj: an object of the host's, okind its kind (host.h; a player: its index + 1; its text is its name) - equal only to
// itself; miss: one who
// isn't there (lord
// without one) - nothing, which may only be checked (lord ? .., lord ?? .., lord == them): used, it is a mistake
// it, ln: a list's items (ln 0: no list) - its value meanwhile is one of them, at random; kind: a function (fn: its
// definition and where it was made), stop (a for / every body's "no more"), a vector (vc: its vn numbers, 2..4 -
// .x .y .z .w; in the region like a text), a record (rec: its members by name) or a trigger (n: its index -
// TRIGGERS below) - neither number nor text
enum { XV_PLAIN, XV_FN, XV_STOP, XV_VEC, XV_REC, XV_TRIG };
struct xfn;
struct xrec;
struct xval { int has_n, has_s, w, obj, okind, ln, kind, vn; double n; const char *s; const struct xval *it; const struct xfn *fn; const double *vc; const struct xrec *rec; char miss[24]; };
struct xrec { int n; const char **key; struct xval *val; };   // a record's members in the order written (in the region)
internal int xl_n(const struct xval *v) { return v->ln ? v->ln : v->w > 0; }   // its items (one value: 1, nothing: 0)
internal const struct xval *xl_at(const struct xval *v, int i) { return v->ln ? &v->it[i] : v; }
internal struct xvars g_vars_sys;   // mod/system.eeai vars (texts.c)
internal int x_fill(char *out, int cap, const char *tmpl, struct xctx *c);
#include "script/host.h"   // what the language asks the program it runs in (the game: eeaimod.c x_host_install)
// a script's world (test/scripts.c, the script runner): the game's time in seconds, waiting until a game second (the script's
// fiber yields to the game), a pipe command run with its reply
internal struct { double (*now)(void); void (*wait)(double until); void (*command)(const char *cmd, char *reply, int cap); } g_x_hooks;

// THE VALUES' REGIONS: their texts and lists, in chunks. The shared one (the lines, prompts, settings) starts afresh
// with the outermost evaluation (x_eval, x_fill from outside): what a caller reads of a value it got lives until the
// next. A script has its own (its variables outlive anything else worked out meanwhile), and a function's call frees
// what it made (x_mark / x_release) but what it hands back.
struct xchunk { struct xchunk *next; size_t cap, used; char d[]; };
struct xregion { struct xchunk *first, *cur; int busy; };
internal struct xregion g_xr_shared, *g_xr = &g_xr_shared;
internal void *x_alloc(size_t n) {
    struct xregion *r = g_xr;
    n = (n + 7) & ~(size_t)7;
    if (!r->cur || r->cur->used + n > r->cur->cap) {
        size_t cap = n > 65536 ? n : 65536;
        struct xchunk *k = malloc(sizeof *k + cap);
        if (!k) return NULL;
        k->next = NULL; k->cap = cap; k->used = 0;
        if (r->cur) r->cur->next = k; else r->first = k;
        r->cur = k;
    }
    void *p = r->cur->d + r->cur->used;
    r->cur->used += n;
    return p;
}
internal void x_region_reset(struct xregion *r) {   // empty again (its first chunk kept)
    if (!r->first) return;
    for (struct xchunk *k = r->first->next, *nx; k; k = nx) { nx = k->next; free(k); }
    r->first->next = NULL;
    r->first->used = 0;
    r->cur = r->first;
}
internal void x_region_free(struct xregion *r) {
    for (struct xchunk *k = r->first, *nx; k; k = nx) { nx = k->next; free(k); }
    r->first = r->cur = NULL;
}
struct xmark { struct xchunk *k; size_t used; };
internal struct xmark x_mark(void) {   // the region's top now
    if (!g_xr->cur) x_alloc(8);
    struct xmark m = { g_xr->cur, g_xr->cur ? g_xr->cur->used : 0 };
    return m;
}
internal void x_release(struct xmark m) {   // everything made since the mark freed
    if (!m.k) return;
    for (struct xchunk *k = m.k->next, *nx; k; k = nx) { nx = k->next; free(k); }
    m.k->next = NULL;
    m.k->used = m.used;
    g_xr->cur = m.k;
}
internal const char *x_strdup(const char *s, size_t n) {   // n bytes of s as a text in the region
    char *r = x_alloc(n + 1);
    if (!r) return "";
    memcpy(r, s, n);
    r[n] = 0;
    return r;
}


// vars keys: "name" or a function "name(a,b)" (its parameters, tfile_parse) - the same name is the same entry
internal int xkey_is(const char *key, const char *name) {
    size_t n = strcspn(key, "(");
    return n == strcspn(name, "(") && !_strnicmp(key, name, n);
}
internal unsigned xvars_hash(const char *key) {   // its name's (up to a function's "("), case ignored as xkey_is does
    unsigned h = 2166136261u;
    for (; *key && *key != '('; key++) h = (h ^ (unsigned char)tolower((unsigned char)*key)) * 16777619u;
    return h % XVARS_HASH;
}
internal int xvars_find(const struct xvars *v, const char *key) {
    if (!v || !v->n) return -1;
    for (int i = v->first[xvars_hash(key)]; i; i = v->next[i - 1]) if (xkey_is(v->key[i - 1], key)) return i - 1;
    return -1;
}
internal void xvars_set(struct xvars *v, const char *key, const char *val) {
    int i = xvars_find(v, key);
    if (i < 0 && v->n == XVARS_MAX) return;
    if (i >= 0) { free(v->val[i]); free(v->key[i]); }
    else {
        unsigned h = xvars_hash(key);
        i = v->n++;
        v->next[i] = v->first[h];
        v->first[h] = (unsigned char)(i + 1);
    }
    v->key[i] = _strdup(key);
    v->val[i] = _strdup(val);
    if (strchr(key, '.')) v->dotted = 1;
}
internal const char *xvars_get(const struct xvars *v, const char *key) {   // a variable (no function)
    int i = xvars_find(v, key);
    return i >= 0 && !strchr(v->key[i], '(') ? v->val[i] : NULL;
}
internal const char *xvars_fn(const struct xvars *v, const char *name, const char **par) {   // a function: its body, *par "a,b)"
    int i = xvars_find(v, name);
    if (i < 0 || !strchr(v->key[i], '(')) return NULL;
    *par = strchr(v->key[i], '(') + 1;
    return v->val[i];
}
// THE FUNCTIONS of all the files in the folder (name and how many parameters): the files are checked when read, a
// function may come from a file read later (an include, mod/system.eeai) - scanned from every file first (x_fn_known)
internal struct { char name[40]; int n; } g_xfn[96];
internal int g_xfn_n, g_xfn_scanned;
// the parameters ("a,b)") of the function whose definition is being read: its body's syntax check takes them for
// players where a path starts with one (by.kills(of)) - at run time the frame has them (c->fpn)
internal const char *g_x_check_par;
internal int x_check_par_has(const char *s, int n) {
    for (const char *p = g_x_check_par; p && *p && *p != ')'; ) {
        int k = (int)strcspn(p, ",)");
        if (k == n && !strncmp(p, s, (size_t)n)) return 1;
        p += k;
        if (*p == ',') p++;
    }
    return 0;
}
// THE NAMES (vars) of all the files too (x_name_check: an entry may come from another file, or later in this one)
internal char g_xvn[256][40];
internal int g_xvn_n;
internal void x_var_add(const char *b, size_t n) { if (n && n < 40 && g_xvn_n < 256) lstrcpynA(g_xvn[g_xvn_n++], b, (int)n + 1); }
// the lines "name(a, b) =" of a file, and "name =" outside its records - a record too (its members: name.key) unless
// the program reads it itself (lines, personality ...: g_x_host.code_record)
internal void x_fn_scan(const char *text) {
    int depth = 0;   // records (and braces) open at the line's start
    for (const char *s = text; *s; ) {
        while (*s == ' ' || *s == '\t') s++;
        const char *b = s;
        while (isalnum((unsigned char)*s) || *s == '_') s++;
        size_t n = (size_t)(s - b);
        if (!depth && n && *s != '(') {
            const char *e = s;
            while (*e == ' ' || *e == '\t') e++;
            if (*e == '=' && e[1] != '=') {
                char nm[40];
                lstrcpynA(nm, b, (int)(n < 39 ? n : 39) + 1);
                if (e[1 + strspn(e + 1, " \t")] != '{' || !g_x_host.code_record(nm)) x_var_add(b, n);
            }
        }
        for (const char *q = s; *q && *q != '\n' && *q != '#'; q++) {   // the line's braces, strings left out
            if (*q == '"' || *q == '\'') {
                char c = *q;
                for (q++; *q && *q != c && *q != '\n'; q++) if (*q == '\\' && q[1]) q++;
                if (*q != c) break;
            } else if (*q == '{') depth++;
            else if (*q == '}' && depth) depth--;
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
internal int x_fn_known(const char *name) {   // its parameter count, -1: no function of that name
    if (!g_xfn_scanned) { g_xfn_scanned = 1; g_x_host.scan_functions(); }
    for (int i = 0; i < g_xfn_n; i++) if (!lstrcmpiA(g_xfn[i].name, name)) return g_xfn[i].n;
    return -1;
}
internal int x_var_known(const char *name) {
    if (!g_xfn_scanned) { g_xfn_scanned = 1; g_x_host.scan_functions(); }
    for (int i = 0; i < g_xvn_n; i++) if (!lstrcmpiA(g_xvn[i], name)) return 1;
    return 0;
}
// THE NAMES CHECK while a file is read (tfile_parse): a name that is nothing known - no parameter, constant, vars
// entry of any file, game value or placeholder the code fills for this key - is told with its file and line (strict
// parsing: in the startup box); the key is kept, at run time the name fails what it is in (x_fail)
internal const char *g_x_check_file, *g_x_check_ph;   // the file being read; " a b " the code's placeholders for the key
internal int g_x_check_line;
internal int xvec_member(const char *m) {   // a vector's number by its name: .x .y .z .w - 0..3, -1 none
    return !strcmp(m, "x") ? 0 : !strcmp(m, "y") ? 1 : !strcmp(m, "z") ? 2 : !strcmp(m, "w") ? 3 : -1;
}
internal int x_name_known(const char *name) {   // a parameter, constant, vars entry, game value or the code's placeholder
    struct xval t;
    char w[48];
    if (strlen(name) > 40) return 1;
    const char *dot = strchr(name, '.');
    if (dot && dot - name < 40) {   // r.a, home.x: a member path - its head a parameter or vars entry (a file's record)
        lstrcpynA(w, name, (int)(dot - name) + 1);
        if (x_check_par_has(w, (int)strlen(w)) || x_var_known(w)) return 1;
    }
    wsprintfA(w, " %s ", name);
    return x_check_par_has(name, (int)strlen(name)) || g_x_host.constant(name, &t) || x_var_known(name) || g_x_host.name_valid(name) ||
           (g_x_check_ph && strstr(g_x_check_ph, w));
}
internal void x_name_check(const char *name) {
    if (!g_x_check_file || x_name_known(name)) return;
    x_error("%s:%d: an unknown name: %s", g_x_check_file, g_x_check_line, name);
}

internal int xnumeric(const char *s, double *n) {   // the whole text a number ("12", not " 12" or "12 ")
    char *e;
    if (!((*s >= '0' && *s <= '9') || *s == '-' || *s == '.')) return 0;
    *n = strtod(s, &e);
    return e != s && !*e;
}
internal void xnothing(struct xval *v) { v->has_n = v->has_s = v->w = v->obj = v->okind = v->ln = v->kind = v->vn = 0; v->n = 0; v->s = ""; v->it = NULL; v->fn = NULL; v->vc = NULL; v->rec = NULL; v->miss[0] = 0; }
internal void xnum(struct xval *v, double n) { v->has_n = 1; v->has_s = v->obj = v->okind = v->ln = v->kind = v->vn = 0; v->w = 1; v->n = n; v->s = ""; v->it = NULL; v->fn = NULL; v->vc = NULL; v->rec = NULL; v->miss[0] = 0; }
internal void xvec(struct xval *v, const double *c, int n) {   // a vector of n numbers (2..4), copied into the region
    double *m = x_alloc((size_t)n * sizeof *m);
    xnothing(v);
    if (!m) return;
    memcpy(m, c, (size_t)n * sizeof *m);
    v->w = 1; v->kind = XV_VEC; v->vn = n; v->vc = m;
}
internal void xstr(struct xval *v, const char *s) {   // a text, only that: "12" in a file is no number
    if (!s) s = "";
    v->s = x_strdup(s, strlen(s));
    v->has_s = 1;
    v->w = 1;
    v->has_n = 0;
    v->obj = v->okind = v->ln = v->kind = v->vn = 0;
    v->it = NULL;
    v->fn = NULL;
    v->vc = NULL;
    v->rec = NULL;
    v->miss[0] = 0;
}
// a placeholder the code fills ({tribute}, {minutes}): a text - and its number when it is one, the code's numbers
// coming as texts
internal void xstr_or_num(struct xval *v, const char *s) { xstr(v, s); v->has_n = xnumeric(v->s, &v->n); }
internal void xdual(struct xval *v, double n, const char *s) { xstr(v, s); v->has_n = 1; v->n = n; }
internal void xdual_kept(struct xval *v, double n, const char *s) {   // the same with a text that stays (not copied)
    v->s = s; v->has_s = v->has_n = 1; v->n = n; v->w = 1;
    v->obj = v->okind = v->ln = v->kind = v->vn = 0; v->it = NULL; v->fn = NULL; v->vc = NULL; v->rec = NULL; v->miss[0] = 0;
}
internal int xis_num(const struct xval *v, double *n) { if (v->has_n) *n = v->n; return v->has_n; }
internal double xto_num(const struct xval *v) { return v->has_n ? v->n : 0; }
internal int xtrue(const struct xval *v) { return v->w && (v->ln || v->obj || v->kind || (v->has_n ? v->n != 0 : v->s[0] != 0)); }   // a list, a player, a function: true
// n to dec decimals, a half away from 0 (0.625 -> 0.63, 62.5 -> 63) - printf's own rounding takes a half to the even
// digit (0.62, 62), which nobody writing a text expects
internal double x_round_dec(double n, int dec) {
    double m = 1, a = n < 0 ? -n : n;
    for (int i = 0; i < dec; i++) m *= 10;
    if (a * m >= 9e15) return n;
    a = (double)(long long)(a * m + 0.5) / m;
    return n < 0 ? -a : a;
}
internal void xnum_text(double n, char *out, int cap) {   // a number as a text: whole, or up to 2 decimals
    double r = n < 0 ? -n : n;
    if (r - (double)(long long)r < 1e-9 && r < 1e15) snprintf(out, (size_t)cap, "%lld", (long long)n);
    else {
        snprintf(out, (size_t)cap, "%.2f", x_round_dec(n, 2));
        char *e = out + strlen(out);
        while (e > out && e[-1] == '0') *--e = 0;
        if (e > out && e[-1] == '.') e[-1] = 0;
    }
}
internal void xto_text(const struct xval *v, char *out, int cap) {
    if (v->ln) {   // a list: its items, as it would be written - no one of them (that is pick's, a line's, a {placeholder}'s)
        int o = 0;
        out[o++] = '[';
        for (int i = 0; i < v->ln && o < cap - 6; i++) {
            if (i) { out[o++] = ','; out[o++] = ' '; }
            xto_text(&v->it[i], out + o, cap - o - 2);
            o += (int)strlen(out + o);
        }
        if (o > cap - 2) o = cap - 2;
        out[o++] = ']';
        out[o] = 0;
        return;
    }
    if (v->kind == XV_VEC && v->w) {   // a vector: (12, 40) - its numbers as a number is written
        int o = 0;
        out[o++] = '(';
        for (int i = 0; i < v->vn && o < cap - 6; i++) {
            if (i) { out[o++] = ','; out[o++] = ' '; }
            xnum_text(v->vc[i], out + o, cap - o - 2);
            o += (int)strlen(out + o);
        }
        if (o > cap - 2) o = cap - 2;
        out[o++] = ')';
        out[o] = 0;
        return;
    }
    if (v->kind == XV_REC && v->w) {   // a record: {a = 1, b = text} - its members as written
        int o = 0;
        out[o++] = '{';
        for (int i = 0; i < v->rec->n && o < cap - 8; i++) {
            o += snprintf(out + o, (size_t)(cap - o), "%s%s = ", i ? ", " : "", v->rec->key[i]);
            if (o >= cap - 4) { o = cap - 4; break; }
            xto_text(&v->rec->val[i], out + o, cap - o - 2);
            o += (int)strlen(out + o);
        }
        if (o > cap - 2) o = cap - 2;
        out[o++] = '}';
        out[o] = 0;
        return;
    }
    if (v->kind == XV_TRIG && v->w) { snprintf(out, (size_t)cap, "trigger %d", (int)v->n + 1); return; }
    if (v->kind && v->w) { lstrcpynA(out, v->kind == XV_FN ? "function" : "stop", cap); return; }
    if (v->has_s || !v->w) { lstrcpynA(out, v->w ? v->s : "", cap); return; }
    xnum_text(v->n, out, cap);
}
internal const struct xval *xrec_get(const struct xval *r, const char *key) {   // a record's member by name; NULL: none
    for (int i = 0; r->kind == XV_REC && i < r->rec->n; i++) if (!strcmp(r->rec->key[i], key)) return &r->rec->val[i];
    return NULL;
}
internal const char *xtext_of(const struct xval *v) {   // its text, whole (in the region for a number, a list)
    if (v->ln || ((v->kind == XV_VEC || v->kind == XV_REC || v->kind == XV_TRIG) && v->w)) { char t[XVAL_CAP]; xto_text(v, t, sizeof t); return x_strdup(t, strlen(t)); }
    if (v->kind && v->w) return v->kind == XV_FN ? "function" : "stop";
    if (v->has_s || !v->w) return v->w ? v->s : "";
    char t[48];
    xnum_text(v->n, t, sizeof t);
    return x_strdup(t, strlen(t));
}

// a string literal at s (" or '): its text into out (cap bytes) - escapes undone, {braces} kept as written (an
// expression's quotes are its own there); *end after the closing quote. Returns the length, -1 on a mistake (*why).
// out NULL, cap 0: only checked.
#define X_LBRACE '\x01'   // \{ and \} in a string: braces that are text, not a placeholder (x_fill writes them)
#define X_RBRACE '\x02'
internal int x_string(const char *s, char *out, int cap, const char **end, const char **why) {
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
            if (e != 'n' && e != '"' && e != '\'' && e != '\\' && e != '{' && e != '}') { *why = "an unknown escape (\\\" \\\\ \\n \\{ \\})"; return -1; }
            c = e == 'n' ? '\n' : e == '{' ? X_LBRACE : e == '}' ? X_RBRACE : e;   // \{ \}: a brace as it is, no placeholder
            s++;
        } else if (c == '{') depth = 1;
        if (o < cap - 1) out[o++] = c;
    }
}

struct xcheck;
struct xscope;
internal int x_fills_ok(const char *text, const char **why, struct xcheck *k, struct xscope *sc);
// a draw: the host's display generator - in a decision (c->game) its other one, which every PC draws alike
internal void xdual(struct xval *v, double n, const char *s);
internal int x_rnd(const struct xctx *c, int n) { return c->game ? g_x_host.rnd_decide(n) : g_x_host.rnd_show(n); }
internal double x_rnd01(const struct xctx *c) { return x_rnd(c, 1 << 30) / (double)(1 << 30); }   // 0 <= x < 1
// a list's one item at random - for a consumer that wants one value (a line, a {placeholder}, pick(l)); a single value
// stays itself, nothing stays nothing
internal void x_pick(const struct xctx *c, struct xval *v) {
    if (v->ln) *v = v->it[v->ln > 1 ? x_rnd(c, v->ln) : 0];
}
// a player who isn't there (lord without one), used without a check (lord ? .. / lord ?? .. / lord == them): a
// mistake like any other (x_fail) - a line built on a lord that isn't there makes no sense
internal void x_missing(struct xctx *c, const char *who) {
    x_error("%s is used, but there is none here - check it first (%s ? ... / %s ?? ...): %.100s", who, who, who, c->top ? c->top : "");
    c->failed++;
    c->missed = 1;
}

// ===== THE PARSER: the source into a tree =====
// A node: one of the expressions or statements. lvl: the nesting levels entering it takes (the old parser's own
// levels - an expression, a primary, a unary operator: at most 24 deep together within one expression); depth: its
// deepest path, itself included (a function's body counts on its own). line, at: where it starts in the source.
enum { XN_NUM, XN_STR, XN_NAME, XN_CALL, XN_LIST, XN_NEG, XN_NOT, XN_MUL, XN_ADD, XN_CMP, XN_AND, XN_OR, XN_COAL, XN_COND,
       XN_LAMBDA, XN_BLOCK, XN_ASSIGN, XN_RETURN, XN_MEMBER, XN_REC, XN_LET, XN_IF, XN_WHILE, XN_FOR, XN_BREAK, XN_CONTINUE,
       XN_INDEX, XN_MCALL };
struct xnode {
    unsigned char k, op, lvl, depth;
    int n_items;                 // a list's items, a call's arguments (all; the first 8 are used), a function's
                                 // parameters, a block's statements
    int line;                    // the line in the source (1 the first)
    double n;                    // a number
    const char *s;               // a name, a string's text (as a template: its {placeholders} filled when worked out)
    const char *at;              // where it starts in the source (a statement's text for the messages)
    struct xnode *a, *b, *c;     // operands: a op b, c ? a : b (c the condition); a function's body; a value;
                                 // if c { a } else b (a block, or the next if); while c { a }; for s in a..b { c }
                                 // (b NULL: in the list a)
    struct xnode **items;        // a list's items, a call's arguments, a function's parameters (names), statements
    const char *cs;              // a name that is a kind. or unit. constant: its text once looked up (its number in n)
};
#define X_LEVELS 24   // the levels of the old parser - a deeper expression is none (not an expression)
#define X_CALLS 64    // functions called inside each other
// the parser's state
struct xps {
    const char *s, *why, *src0;
    int err, par;          // par: in ( ), not in [ ] (strings side by side join there)
    int nl;                // a line break in the space last skipped, which ends at nl_pos
    const char *nl_pos;
    int line;              // the line of lc (counting up from src0)
    const char *lc;
    char *mem;             // the nodes' memory (one block: copied out when done)
    size_t used, cap;
};
internal int xps_line(struct xps *p, const char *at) {   // the line at a place in the source
    if (at < p->lc) { p->lc = p->src0; p->line = 1; }
    for (; p->lc < at; p->lc++) if (*p->lc == '\n') p->line++;
    return p->line;
}
internal struct xnode *xn_new(struct xps *p, int k, const char *at) {
    if (p->used + sizeof(struct xnode) > p->cap) {
        size_t cap = p->cap ? p->cap * 2 : 4096;
        char *m = realloc(p->mem, cap);
        if (!m) { p->err = 1; return NULL; }
        p->mem = m;
        p->cap = cap;
    }
    struct xnode *n = (struct xnode *)(p->mem + p->used);   // an offset until copied out (x_relocate)
    p->used += sizeof *n;
    memset(n, 0, sizeof *n);
    n->k = (unsigned char)k;
    n->at = at;
    n->line = xps_line(p, at);
    return n;
}
// the nodes live in p->mem, which moves as it grows: they are kept as offsets (+1, 0 none) while parsing
#define XOFF(p, n) ((n) ? (struct xnode *)((char *)(n) - (p)->mem + 1) : NULL)
#define XPTR(p, o) ((o) ? (struct xnode *)((p)->mem + (uintptr_t)(o) - 1) : NULL)
// spaces, line breaks and comments (# to the line's end); nl: whether a line break was among them
internal void xps_space(struct xps *p) {
    const char *b = p->s;
    int nl = 0;
    for (;;) {
        char c = *p->s;
        if (c == ' ' || c == '\t' || c == '\r') p->s++;
        else if (c == '\n') { nl = 1; p->s++; }
        else if (c == '#') { while (*p->s && *p->s != '\n') p->s++; }
        else break;
    }
    if (p->s != b) { p->nl = nl; p->nl_pos = p->s; }
}
internal int xps_newline_before(struct xps *p) { return p->nl && p->nl_pos == p->s; }   // after xps_space
internal int xps_word(struct xps *p, const char *w) {   // a keyword (and, or, not, return), not the start of a longer name
    xps_space(p);
    size_t n = strlen(w);
    if (_strnicmp(p->s, w, n) || isalnum((unsigned char)p->s[n]) || p->s[n] == '_') return 0;
    p->s += n;
    return 1;
}
internal int xps_op(struct xps *p, const char *op) {
    xps_space(p);
    size_t n = strlen(op);
    if (strncmp(p->s, op, n)) return 0;
    p->s += n;
    return 1;
}
internal int x_word_reserved(const char *name) {   // a word of the language: no variable's or parameter's name
    static const char *const k[] = { "true", "false", "return", "and", "or", "not", "let", "if", "else", "while", "for", "in",
                                     "break", "continue" };
    for (size_t i = 0; i < sizeof k / sizeof *k; i++) if (!lstrcmpiA(name, k[i])) return 1;
    return 0;
}
internal void xps_expr(struct xps *p, struct xnode **out);
internal void xps_block(struct xps *p, struct xnode **out);
internal int xps_items_add(struct xps *p, uintptr_t **it, int *n, int *cap, struct xnode *x) {
    if (*n == *cap) {
        *cap = *cap ? *cap * 2 : 8;
        uintptr_t *m = realloc(*it, (size_t)*cap * sizeof **it);
        if (!m) { p->err = 1; return 0; }
        *it = m;
    }
    (*it)[(*n)++] = (uintptr_t)x;
    return 1;
}
// "(a, b) =>" at s: a function's head - its parameters' names (at most 8) and where its body starts; NULL: none
internal const char *xps_lambda_head(const char *s, char names[8][24], int *n) {
    *n = 0;
    s++;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
    if (*s != ')') for (;;) {
        if (!isalpha((unsigned char)*s) && *s != '_') return NULL;
        int k = 0;
        while ((isalnum((unsigned char)*s) || *s == '_') && k < 23) names[*n][k++] = *s++;
        names[*n][k] = 0;
        if (isalnum((unsigned char)*s) || *s == '_' || *s == '.' || ++*n > 8) return NULL;
        while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
        if (*s == ',') { s++; while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++; continue; }
        break;
    }
    if (*s != ')') return NULL;
    s++;
    while (*s == ' ' || *s == '\t') s++;
    return s[0] == '=' && s[1] == '>' ? s + 2 : NULL;
}
internal void xps_primary(struct xps *p, struct xnode **out) {
    xps_space(p);
    const char *s = p->s;
    struct xnode *o = NULL;
    char pn[8][24];
    int npn;
    const char *body;
    *out = NULL;
    if (*s == '(' && (body = xps_lambda_head(s, pn, &npn)) != NULL) {   // a function: (a, b) => value, or => { ... }
        uintptr_t *it = NULL;
        int n = 0, cap = 0;
        for (int i = 0; i < npn; i++) {
            for (int j = 0; j < i; j++) if (!strcmp(pn[i], pn[j])) { p->err = 1; p->why = "a parameter twice"; free(it); return; }
            if (x_word_reserved(pn[i])) { p->err = 1; p->why = "a word of the language is no name"; free(it); return; }
            struct xnode *x = xn_new(p, XN_NAME, s);
            if (!x) { free(it); return; }
            x->s = _strdup(pn[i]);
            if (!xps_items_add(p, &it, &n, &cap, XOFF(p, x))) { free(it); return; }
        }
        p->s = body;
        xps_space(p);
        int par = p->par;
        p->par = 0;
        struct xnode *b;
        if (*p->s == '{') xps_block(p, &b);
        else xps_expr(p, &b);
        p->par = par;
        if (p->err) { free(it); return; }
        o = xn_new(p, XN_LAMBDA, s);
        if (!o) { free(it); return; }
        o->n_items = n;
        o->items = (struct xnode **)it;
        o->a = b;
        o = XOFF(p, o);
    } else if (*s == '(') {
        p->s++;
        p->par++;
        xps_expr(p, &o);
        p->par--;
        if (!p->err && !xps_op(p, ")")) p->err = 1;
        if (p->err) return;
    } else if (*s == '"' || *s == '\'') {   // a string: its text, a template (filled when worked out)
        const char *e, *why, *q;
        if (x_string(s, NULL, 0, &e, &why) < 0) { p->err = 1; return; }   // (the file check tells it: tl_value)
        size_t span = (size_t)(e - s);   // the source's length: its text is no longer
        // ("a" "b"): strings side by side in ( ) are one - a long text over several lines (in a list a missing comma
        // stays a mistake)
        for (q = e; p->par; ) {
            while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') q++;
            if (*q != '"' && *q != '\'') break;
            const char *e2;
            if (x_string(q, NULL, 0, &e2, &why) < 0) { p->err = 1; return; }
            span += (size_t)(e2 - q);
            q = e = e2;
        }
        char *t = malloc(span + 1), *w = t;
        if (!t) { p->err = 1; return; }
        for (q = s; q < e; ) {   // each string's text, joined
            while (q < e && (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r')) q++;
            if (q >= e) break;
            int k = x_string(q, w, (int)(span + 1 - (size_t)(w - t)), &q, &why);
            if (k > 0) w += k;
        }
        *w = 0;
        p->s = e;
        o = xn_new(p, XN_STR, s);
        if (!o) { free(t); return; }
        o->s = t;   // copied out with the tree
        o = XOFF(p, o);
    } else if (*s == '[') {   // a list: its items
        int par = p->par, n = 0, cap = 0;
        uintptr_t *it = NULL;
        p->s++;
        p->par = 0;
        for (;;) {
            if (xps_op(p, "]")) break;
            struct xnode *x;
            xps_expr(p, &x);
            if (p->err || !xps_items_add(p, &it, &n, &cap, x)) { free(it); p->par = par; return; }
            if (xps_op(p, ",")) continue;
            if (!xps_op(p, "]")) p->err = 1;
            break;
        }
        p->par = par;
        if (p->err) { free(it); return; }
        o = xn_new(p, XN_LIST, s);
        if (!o) { free(it); return; }
        o->n_items = n;
        o->items = (struct xnode **)it;   // offsets until copied out
        o = XOFF(p, o);
    } else if (*s == '{') {   // a record: { name = value, ... } - a comma after each member (the last's may be left out)
        int par = p->par, n = 0, cap = 0;
        uintptr_t *it = NULL;
        p->s++;
        p->par = 0;
        for (;;) {
            if (xps_op(p, "}")) break;
            const char *ks = p->s;
            char key[40];
            int k = 0;
            if (!isalpha((unsigned char)*ks) && *ks != '_') { p->err = 1; p->why = "a record's member: name = value"; break; }
            while ((isalnum((unsigned char)*p->s) || *p->s == '_') && k < 39) key[k++] = *p->s++;
            key[k] = 0;
            if (isalnum((unsigned char)*p->s) || *p->s == '_') { p->err = 1; p->why = "a name over 39 characters"; break; }
            for (int i = 0; i < n && !p->err; i++)
                if (!strcmp(XPTR(p, (struct xnode *)it[i])->s, key)) { p->err = 1; p->why = "a record's member twice"; }
            if (p->err) break;
            if (!xps_op(p, "=") || *p->s == '=') { p->err = 1; p->why = "a record's member: name = value"; break; }
            struct xnode *x;
            xps_expr(p, &x);
            if (p->err) break;
            struct xnode *m = xn_new(p, XN_ASSIGN, ks);
            if (!m) break;
            m->s = _strdup(key);
            m->a = x;
            if (!xps_items_add(p, &it, &n, &cap, XOFF(p, m))) break;
            if (xps_op(p, ",")) continue;
            if (!xps_op(p, "}")) { p->err = 1; p->why = "a comma after each of a record's members"; }
            break;
        }
        p->par = par;
        if (p->err) { free(it); return; }
        o = xn_new(p, XN_REC, s);
        if (!o) { free(it); return; }
        o->n_items = n;
        o->items = (struct xnode **)it;
        o = XOFF(p, o);
    } else if ((*s >= '0' && *s <= '9') || (*s == '.' && s[1] >= '0' && s[1] <= '9')) {
        char *e;
        double n = strtod(s, &e);
        if (e > s && e[-1] == '.' && *e == '.') e--;   // 1..5: the number is 1, .. a range's
        if (*e == '%') { n /= 100; e++; }   // 70% = 0.7
        else if (*e == 's' && !isalnum((unsigned char)e[1]) && e[1] != '_') e++;   // 20s: seconds
        else if (!strncmp(e, "min", 3) && !isalnum((unsigned char)e[3]) && e[3] != '_') { n *= 60; e += 3; }   // 19.5min
        p->s = e;
        o = xn_new(p, XN_NUM, s);
        if (!o) return;
        o->n = n;
        o = XOFF(p, o);
    } else if (isalpha((unsigned char)*s) || *s == '_') {
        char name[40];
        int n = 0;
        while ((isalnum((unsigned char)*s) || *s == '_' || (*s == '.' && isalpha((unsigned char)s[1]))) && n < 39) name[n++] = *s++;
        name[n] = 0;
        p->s = s;
        if (!lstrcmpiA(name, "true") || !lstrcmpiA(name, "false")) {
            o = xn_new(p, XN_NUM, s - n);
            if (!o) return;
            o->n = !lstrcmpiA(name, "true");
            o = XOFF(p, o);
        } else {
            xps_space(p);
            if (*p->s != '(') {
                p->s = s;   // (an item right after it: l[2])
                o = xn_new(p, XN_NAME, s - n);
                if (!o) return;
                o->s = _strdup(name);
                o = XOFF(p, o);
            } else {
                p->s++;   // a function: its arguments
                int na = 0, cap = 0;
                uintptr_t *it = NULL;
                xps_space(p);
                if (*p->s == ')') p->s++;
                else for (;;) {
                    struct xnode *x;
                    xps_expr(p, &x);
                    if (p->err || !xps_items_add(p, &it, &na, &cap, x)) { free(it); return; }
                    if (xps_op(p, ",")) continue;
                    if (!xps_op(p, ")")) { p->err = 1; free(it); return; }
                    break;
                }
                o = xn_new(p, XN_CALL, s - n);
                if (!o) { free(it); return; }
                o->s = _strdup(name);
                o->n_items = na;
                o->items = (struct xnode **)it;
                o = XOFF(p, o);
            }
        }
    } else { p->err = 1; return; }
    // right after it (no space): an item, l[2] - game.players[1]; a member or method after a call, ( ) or [ ]:
    // step(home).x, (a + b).y, player(2).towers, game.players[1].opinion(p) - a name has its dots in it (home.x)
    for (;;) {
        int k = XPTR(p, o)->k;
        if (p->s[0] == '[' && k != XN_NUM) {
            const char *at = p->s++;
            struct xnode *i;
            int par = p->par;
            p->par = 1;
            xps_expr(p, &i);
            p->par = par;
            if (p->err) return;
            if (!xps_op(p, "]")) { p->err = 1; p->why = "l[i]: an item's number, then ]"; return; }
            struct xnode *x = xn_new(p, XN_INDEX, at);
            if (!x) return;
            x->a = o; x->b = i;
            o = XOFF(p, x);
            continue;
        }
        if (!(p->s[0] == '.' && isalpha((unsigned char)p->s[1]) && k != XN_NAME && k != XN_NUM)) break;
        const char *at = p->s++;
        char name[24];
        int n = 0;
        while ((isalnum((unsigned char)*p->s) || *p->s == '_') && n < 23) name[n++] = *p->s++;
        name[n] = 0;
        uintptr_t *it = NULL;
        int na = 0, cap = 0;
        if (*p->s == '(') {   // a method: its arguments
            p->s++;
            xps_space(p);
            if (*p->s == ')') p->s++;
            else for (;;) {
                struct xnode *x;
                xps_expr(p, &x);
                if (p->err || !xps_items_add(p, &it, &na, &cap, x)) { free(it); return; }
                if (xps_op(p, ",")) continue;
                if (!xps_op(p, ")")) { p->err = 1; free(it); return; }
                break;
            }
            k = XN_MCALL;
        } else k = XN_MEMBER;
        struct xnode *m = xn_new(p, k, at);
        if (!m) { free(it); return; }
        m->s = _strdup(name);
        m->a = o;
        m->n_items = na;
        m->items = (struct xnode **)it;
        o = XOFF(p, m);
    }
    XPTR(p, o)->lvl++;   // the primary's level
    *out = o;
}
internal void xps_unary(struct xps *p, struct xnode **out) {
    *out = NULL;
    const char *at = (xps_space(p), p->s);
    int neg = xps_op(p, "-"), inv = !neg && xps_word(p, "not");
    if (neg || inv) {
        struct xnode *a;
        xps_unary(p, &a);
        if (p->err) return;
        struct xnode *o = xn_new(p, neg ? XN_NEG : XN_NOT, at);
        if (!o) return;
        o->a = a;
        o->lvl = 1;
        *out = XOFF(p, o);
        return;
    }
    if (*p->s == '!' && p->s[1] != '=') { p->err = 1; p->why = "! - write not"; return; }
    xps_primary(p, out);
}
internal struct xnode *xps_bin(struct xps *p, int k, int op, struct xnode *a, struct xnode *b) {
    struct xnode *o = xn_new(p, k, XPTR(p, a)->at);
    if (!o) return NULL;
    o->op = (unsigned char)op;
    o->a = a;
    o->b = b;
    return XOFF(p, o);
}
internal void xps_mul(struct xps *p, struct xnode **out) {
    xps_unary(p, out);
    while (!p->err) {
        xps_space(p);
        if (*p->s == '%') { p->err = 1; p->why = "% belongs right after a number: 70% is 0.7 (there is no remainder)"; return; }
        int op = xps_op(p, "*") ? '*' : xps_op(p, "/") ? '/' : 0;
        if (!op) return;
        struct xnode *b;
        xps_unary(p, &b);
        if (p->err) return;
        *out = xps_bin(p, XN_MUL, op, *out, b);
    }
}
internal void xps_add(struct xps *p, struct xnode **out) {
    xps_mul(p, out);
    while (!p->err) {
        int op = xps_op(p, "+") ? '+' : xps_op(p, "-") ? '-' : 0;
        if (!op) return;
        struct xnode *b;
        xps_mul(p, &b);
        if (p->err) return;
        *out = xps_bin(p, XN_ADD, op, *out, b);
    }
}
internal void xps_cmp(struct xps *p, struct xnode **out) {
    xps_add(p, out);
    if (p->err) return;
    int op = xps_op(p, "<=") ? 'l' : xps_op(p, ">=") ? 'g' : xps_op(p, "==") ? '=' : xps_op(p, "!=") ? '!' :
             xps_op(p, "<") ? '<' : xps_op(p, ">") ? '>' : 0;
    if (!op) return;
    struct xnode *b;
    xps_add(p, &b);
    if (p->err) return;
    *out = xps_bin(p, XN_CMP, op, *out, b);
}
internal void xps_and(struct xps *p, struct xnode **out) {
    xps_cmp(p, out);
    if (!p->err && (xps_space(p), *p->s == '&')) { p->err = 1; p->why = "&& - write and"; }
    while (!p->err && xps_word(p, "and")) {
        struct xnode *b;
        xps_cmp(p, &b);
        if (p->err) return;
        *out = xps_bin(p, XN_AND, 0, *out, b);
        if (!p->err && (xps_space(p), *p->s == '&')) { p->err = 1; p->why = "&& - write and"; }
    }
}
internal void xps_or(struct xps *p, struct xnode **out) {
    xps_and(p, out);
    if (!p->err && (xps_space(p), *p->s == '|')) { p->err = 1; p->why = "|| - write or"; }
    while (!p->err && xps_word(p, "or")) {
        struct xnode *b;
        xps_and(p, &b);
        if (p->err) return;
        *out = xps_bin(p, XN_OR, 0, *out, b);
        if (!p->err && (xps_space(p), *p->s == '|')) { p->err = 1; p->why = "|| - write or"; }
    }
}
internal void xps_coalesce(struct xps *p, struct xnode **out) {   // a ?? b: a, unless it is nothing
    xps_or(p, out);
    while (!p->err && xps_op(p, "??")) {
        struct xnode *b;
        xps_or(p, &b);
        if (p->err) return;
        *out = xps_bin(p, XN_COAL, 0, *out, b);
    }
}
internal void xps_expr(struct xps *p, struct xnode **out) {   // c ? a : b, c ? a (nothing when c doesn't hold)
    *out = NULL;
    xps_coalesce(p, out);
    if (!p->err && xps_op(p, "?")) {
        struct xnode *a, *b = NULL;
        xps_expr(p, &a);
        if (!p->err && xps_op(p, ":")) xps_expr(p, &b);
        if (p->err) return;
        struct xnode *o = xn_new(p, XN_COND, XPTR(p, *out)->at);
        if (!o) return;
        o->c = *out;
        o->a = a;
        o->b = b;
        *out = XOFF(p, o);
    }
    if (!p->err && *out) XPTR(p, *out)->lvl++;   // the expression's level
}
// a name at p->s (a variable's: letters, digits, _) into name; 0: none (must: a word of the language a mistake set)
internal int xps_name(struct xps *p, char name[40], int must) {
    xps_space(p);
    const char *q = p->s;
    if (!isalpha((unsigned char)*q) && *q != '_') return 0;
    while (isalnum((unsigned char)*q) || *q == '_') q++;
    if (q - p->s >= 40) { if (must) { p->err = 1; p->why = "a name over 39 characters"; } return 0; }
    lstrcpynA(name, p->s, (int)(q - p->s) + 1);
    if (x_word_reserved(name)) { if (must) { p->err = 1; p->why = "a word of the language is no name"; } return 0; }
    p->s = q;
    return 1;
}
internal void xps_stmt(struct xps *p, struct xnode **out);
internal void xps_body(struct xps *p, struct xnode **out, const char *what) {   // a statement's { ... } on its line
    xps_space(p);
    if (*p->s != '{' || xps_newline_before(p)) { p->err = 1; p->why = what; return; }
    xps_block(p, out);
}
// if c { ... } [else if ... | else { ... }] - at the if's condition
internal void xps_if(struct xps *p, struct xnode **out, const char *at) {
    struct xnode *c, *a, *b = NULL;
    *out = NULL;
    xps_expr(p, &c);
    if (p->err) return;
    xps_body(p, &a, "if <condition> { ... } - the { on the if's line");
    if (p->err) return;
    if (xps_word(p, "else")) {
        const char *at2 = p->s;
        if (xps_word(p, "if")) xps_if(p, &b, at2);
        else xps_body(p, &b, "else { ... } or else if - the { on the else's line");
        if (p->err) return;
    }
    struct xnode *o = xn_new(p, XN_IF, at);
    if (!o) return;
    o->c = c; o->a = a; o->b = b;
    *out = XOFF(p, o);
}
// a statement: let name = value, name = value (+= -= *= /=), if, while, for, break, continue, return [value], or an
// expression (a call)
internal void xps_stmt(struct xps *p, struct xnode **out) {
    xps_space(p);
    const char *at = p->s;
    char name[40];
    *out = NULL;
    if (xps_word(p, "return")) {
        struct xnode *v = NULL;
        xps_space(p);
        if (*p->s && *p->s != '}' && !xps_newline_before(p)) { xps_expr(p, &v); if (p->err) return; }
        struct xnode *o = xn_new(p, XN_RETURN, at);
        if (!o) return;
        o->a = v;
        *out = XOFF(p, o);
        return;
    }
    if (xps_word(p, "break") || xps_word(p, "continue")) {
        struct xnode *o = xn_new(p, *at == 'b' || *at == 'B' ? XN_BREAK : XN_CONTINUE, at);
        if (o) *out = XOFF(p, o);
        return;
    }
    if (xps_word(p, "if")) { xps_if(p, out, at); return; }
    if (xps_word(p, "while")) {
        struct xnode *c, *a;
        xps_expr(p, &c);
        if (!p->err) xps_body(p, &a, "while <condition> { ... } - the { on the while's line");
        if (p->err) return;
        struct xnode *o = xn_new(p, XN_WHILE, at);
        if (!o) return;
        o->c = c; o->a = a;
        *out = XOFF(p, o);
        return;
    }
    if (xps_word(p, "for")) {   // for i in 1..5 { ... }, for x in list { ... }
        struct xnode *a, *b = NULL, *c;
        if (!xps_name(p, name, 1)) { if (!p->err) { p->err = 1; p->why = "for <name> in 1..5 { ... }, for <name> in <list> { ... }"; } return; }
        if (!xps_word(p, "in")) { p->err = 1; p->why = "for <name> in 1..5 { ... }, for <name> in <list> { ... }"; return; }
        xps_expr(p, &a);
        if (!p->err && xps_op(p, "..")) xps_expr(p, &b);
        if (!p->err) xps_body(p, &c, "for <name> in ... { ... } - the { on the for's line");
        if (p->err) return;
        struct xnode *o = xn_new(p, XN_FOR, at);
        if (!o) return;
        o->s = _strdup(name);
        o->a = a; o->b = b; o->c = c;
        *out = XOFF(p, o);
        return;
    }
    int let = xps_word(p, "let");
    const char *nm = p->s;
    if (xps_name(p, name, let)) {   // [let] name = ..., name += ...
        xps_space(p);
        const char *r = p->s;
        int op = r[1] == '=' && (*r == '+' || *r == '-' || *r == '*' || *r == '/') ? *r : 0;
        if ((*r == '=' && r[1] != '=' && r[1] != '>') || op) {
            if (let && op) { p->err = 1; p->why = "let name = value (a new variable has no value to add to)"; return; }
            p->s = r + (op ? 2 : 1);
            struct xnode *v;
            xps_expr(p, &v);
            if (p->err) return;
            if (op) {   // n += 2 is n = n + 2
                struct xnode *x = xn_new(p, XN_NAME, nm);
                if (!x) return;
                x->s = _strdup(name);
                x->lvl = 1;
                v = xps_bin(p, op == '+' || op == '-' ? XN_ADD : XN_MUL, op, XOFF(p, x), v);
                if (!v) return;
            }
            struct xnode *o = xn_new(p, let ? XN_LET : XN_ASSIGN, at);
            if (!o) return;
            o->s = _strdup(name);
            o->a = v;
            *out = XOFF(p, o);
            return;
        }
        if (let) { p->err = 1; p->why = "let name = value"; return; }
        p->s = nm;
    }
    if (p->err) return;
    if (let) { p->err = 1; p->why = "let name = value"; return; }
    xps_expr(p, out);
}
// statements, one a line, up to the block's } (brace) or the source's end (a script)
internal void xps_stmts(struct xps *p, struct xnode **out, int brace, const char *at) {
    uintptr_t *it = NULL;
    int n = 0, cap = 0;
    *out = NULL;
    for (;;) {
        xps_space(p);
        if (brace && *p->s == '}') { p->s++; break; }
        if (!*p->s) { if (brace) { p->err = 1; p->why = "a { without its }"; free(it); return; } break; }
        if (n && !xps_newline_before(p)) { p->err = 1; p->why = "one statement a line"; free(it); return; }
        struct xnode *x;
        xps_stmt(p, &x);
        if (p->err || !xps_items_add(p, &it, &n, &cap, x)) { free(it); return; }
    }
    struct xnode *o = xn_new(p, XN_BLOCK, at);
    if (!o) { free(it); return; }
    o->n_items = n;
    o->items = (struct xnode **)it;
    *out = XOFF(p, o);
}
internal void xps_block(struct xps *p, struct xnode **out) {   // at its {
    const char *at = p->s;
    p->s++;
    xps_stmts(p, out, 1, at);
}

// A COMPILED SOURCE: its tree (pointers now), its deepest level, whether it is one at all (why not, where)
struct xunit { struct xnode *root; int depth, ok, line; const char *why; };
// each node's deepest path (its own levels included); a function's body is its own: too deep there, *bad
internal unsigned char x_depth_of(struct xnode *n, int *bad) {
    if (!n) return 0;
    int d = 0, k;
    if (n->k == XN_LAMBDA || n->k == XN_BLOCK) {   // statements and bodies: each on its own
        if (n->k == XN_LAMBDA && (k = x_depth_of(n->a, bad)) > X_LEVELS) *bad = 1;
        for (int i = 0; n->k == XN_BLOCK && i < n->n_items; i++) if (x_depth_of(n->items[i], bad) > X_LEVELS) *bad = 1;
        n->depth = n->lvl;
        return n->depth;
    }
    if ((k = x_depth_of(n->a, bad)) > d) d = k;
    if ((k = x_depth_of(n->b, bad)) > d) d = k;
    if ((k = x_depth_of(n->c, bad)) > d) d = k;
    for (int i = 0; i < n->n_items; i++) if ((k = x_depth_of(n->items[i], bad)) > d) d = k;
    d += n->lvl;
    n->depth = (unsigned char)(d > 255 ? 255 : d);
    return n->depth;
}
// the tree out of the parser's block: offsets to pointers into a block of its own (kept: compiled sources stay)
internal struct xnode *x_relocate(struct xps *p, struct xnode *o, struct xnode *base) {
    if (!o) return NULL;
    struct xnode *n = (struct xnode *)((char *)base + ((uintptr_t)o - 1));
    n->a = x_relocate(p, n->a, base);
    n->b = x_relocate(p, n->b, base);
    n->c = x_relocate(p, n->c, base);
    if (n->n_items || n->items) {
        uintptr_t *it = (uintptr_t *)n->items;
        struct xnode **pi = n->n_items ? malloc((size_t)n->n_items * sizeof *pi) : NULL;
        for (int i = 0; i < n->n_items; i++) pi[i] = x_relocate(p, (struct xnode *)it[i], base);
        free(it);
        n->items = pi;
    }
    return n;
}
// src whole as an expression (prog: as statements, a script); the tree, or ok 0 and why (NULL: not an expression)
internal struct xunit x_parse(const char *src, int prog) {
    struct xps p;
    memset(&p, 0, sizeof p);
    p.s = p.src0 = p.lc = src;
    p.line = 1;
    struct xnode *root = NULL;
    if (prog) xps_stmts(&p, &root, 0, src);
    else xps_expr(&p, &root);
    xps_space(&p);
    struct xunit u = { NULL, 0, 0, 0, NULL };
    if (!p.err && *p.s) { p.err = 1; if (!p.why) p.why = "something after the expression"; }
    if (p.err || !root) {   // (what was built before the mistake is lost - a broken source is rare)
        u.line = xps_line(&p, p.s);
        free(p.mem);
        u.why = p.why;
        return u;
    }
    struct xnode *base = malloc(p.used);
    if (!base) { free(p.mem); return u; }
    memcpy(base, p.mem, p.used);
    free(p.mem);
    u.root = x_relocate(&p, root, base);
    int bad = 0;
    u.depth = x_depth_of(u.root, &bad);
    if (bad || u.depth > X_LEVELS) { u.root = NULL; return u; }   // too deep: the old parser's limit
    u.ok = 1;
    return u;
}
// the cache: an expression's text -> its tree (an expression is parsed once, however often it is worked out)
#define XC_BUCKETS 4096
struct xcentry { struct xcentry *next; uint32_t h; struct xunit u; char src[]; };
internal struct xcentry *g_xc[XC_BUCKETS];
internal int g_xc_n;
internal const struct xunit *x_compiled(const char *src) {
    uint32_t h = 2166136261u;
    for (const char *s = src; *s; s++) h = (h ^ (uint8_t)*s) * 16777619u;
    for (struct xcentry *e = g_xc[h % XC_BUCKETS]; e; e = e->next) if (e->h == h && !strcmp(e->src, src)) return &e->u;
    size_t n = strlen(src);
    struct xcentry *e = g_xc_n < 20000 ? malloc(sizeof *e + n + 1) : NULL;
    if (!e) { static struct xunit once; once = x_parse(src, 0); return &once; }   // too many (texts eval): this time only
    memcpy(e->src, src, n + 1);
    e->h = h;
    e->u = x_parse(e->src, 0);   // the tree points into the cached copy (its statements' texts)
    e->next = g_xc[h % XC_BUCKETS];
    g_xc[h % XC_BUCKETS] = e;
    g_xc_n++;
    return &e->u;
}

// the builtin functions: their number by name and argument count (0: none - a vars function or a relation)
internal int x_fn_id(const char *name, int na) {
    return !lstrcmpiA(name, "chance") && na == 1 ? 1 : (!lstrcmpiA(name, "min") || !lstrcmpiA(name, "max")) && na ? 2 :
           !lstrcmpiA(name, "abs") && na == 1 ? 3 : !lstrcmpiA(name, "round") && na == 1 ? 4 :
           !lstrcmpiA(name, "rnd") && na <= 1 ? 5 : !lstrcmpiA(name, "floor") && na == 1 ? 6 :
           !lstrcmpiA(name, "len") && na == 1 ? 7 : !lstrcmpiA(name, "first") && na == 1 ? 8 :
           !lstrcmpiA(name, "item") && na == 2 ? 9 : !lstrcmpiA(name, "contains") && na == 2 ? 10 :
           !lstrcmpiA(name, "map_range") && na == 3 ? 11 : !lstrcmpiA(name, "str") && na == 1 ? 12 :
           !lstrcmpiA(name, "number") && na == 1 ? 13 : !lstrcmpiA(name, "log2") && na == 1 ? 14 : !lstrcmpiA(name, "pick") && na == 1 ? 15 :
           !lstrcmpiA(name, "vec") && na >= 2 && na <= 4 ? 16 : !lstrcmpiA(name, "rgb") && na == 3 ? 17 :
           !lstrcmpiA(name, "distance") && na == 2 ? 18 : !lstrcmpiA(name, "has") && na == 2 ? 19 : 0;
}
internal int x_par_count(const char *par) {   // a vars function's parameters "a,b)": how many
    int want = *par == ')' ? 0 : 1;
    for (const char *q = par; *q && *q != ')'; q++) want += *q == ',';
    return want;
}

// THE HOST'S OBJECTS (host.h): a kind's member by name, NULL none; its members' names for a message
internal const struct x_kmember *x_kind_member(int kind, const char *name) {
    const struct x_kind *k = kind > 0 && kind < X_LIST_OF ? g_x_host.kind(kind) : NULL;
    for (int i = 0; k && i < k->n; i++) if (!strcmp(k->m[i].name, name)) return &k->m[i];
    return NULL;
}
internal const char *x_kind_name(int kind) { const struct x_kind *k = g_x_host.kind(kind & 0xFF); return k ? k->name : "object"; }
internal void x_kind_list(int kind, char *t, int cap) {
    const struct x_kind *k = g_x_host.kind(kind);
    int o = 0;
    t[0] = 0;
    for (int i = 0; k && i < k->n; i++) {   // as many as fit, then ...
        int len = (int)strlen(k->m[i].name) + 4;
        if (o + len + 6 > cap) { snprintf(t + o, (size_t)(cap - o), ", ..."); break; }
        o += snprintf(t + o, (size_t)(cap - o), "%s%s%s", i ? ", " : "", k->m[i].name, k->m[i].amin >= 0 ? "()" : "");
    }
}

// ===== THE CHECK: what a file gets when it is read (after its syntax) =====
// In source order: a name nobody knows (logged with the file and line - in a script a mistake), a function nobody has
// or with the wrong number of arguments, chance(70), a text compared with a number written as such ("12" == 12), a
// variable without its let, read or set before it, made twice where it is seen (a let or parameter hiding another),
// break outside a loop. The names a function sees: its parameters, its own lets and those of every block and body
// around it, the vars, the game's values.
// A scope: a function's (its parameters, its lets) or a block's inside it (blk: an if's, a loop's - the same function).
// A let is known in its whole block from the start (a function made before it may call it later), but the code of
// the same function reads it only after it (decl: passed).
// kind: what a name's value is as far as the check knows (host.h: X_ANY, a kind of the host's objects - from its let)
struct xscope { const struct xscope *up; const char *name[64]; int n; unsigned char decl[64]; int blk; short kind[64]; };
// 1: there (*own: the running function's - this scope or a block it is in), 2: the same function's, its let not passed yet
internal int xsc_find(const struct xscope *s, const char *name, size_t len, int *own) {
    for (int crossed = 0; s; s = s->up) {
        for (int i = 0; i < s->n; i++)
            if (strlen(s->name[i]) == len && !strncmp(s->name[i], name, len)) {
                if (own) *own = !crossed;
                return !crossed && !s->decl[i] ? 2 : 1;
            }
        if (!s->blk) crossed = 1;
    }
    return 0;
}
internal void xsc_add(struct xscope *s, const char *name) {
    for (int i = 0; i < s->n; i++) if (!strcmp(s->name[i], name)) return;
    if (s->n < 64) { s->decl[s->n] = 1; s->name[s->n++] = name; }
}
internal void xsc_pass(struct xscope *s, const char *name, int kind) {   // its let reached: what it is
    for (int i = 0; i < s->n; i++) if (!strcmp(s->name[i], name)) { s->decl[i] = 1; s->kind[i] = (short)kind; }
}
internal int xsc_kind(const struct xscope *s, const char *name, size_t len) {
    for (; s; s = s->up)
        for (int i = 0; i < s->n; i++) if (strlen(s->name[i]) == len && !strncmp(s->name[i], name, len)) return s->kind[i];
    return X_ANY;
}
struct xcheck { struct xctx *c; int script; const char *why; int line; char whybuf[200]; int loops; };
// a node's value as the old parser saw it while only reading: a number or a text written as such, nothing for the
// rest - what "12" == 12 is caught with when a file is read
struct xsv { int w, has_n, has_s; double n; };
internal struct xsv xsv_of(const struct xnode *n) {
    struct xsv r = { 0, 0, 0, 0 };
    if (!n) return r;
    switch (n->k) {
    case XN_NUM: r.w = r.has_n = 1; r.n = n->n; break;
    case XN_STR: r.w = r.has_s = 1; break;
    case XN_NEG: { struct xsv a = xsv_of(n->a); r.w = r.has_n = 1; r.n = -(a.has_n ? a.n : 0); break; }
    case XN_NOT: { struct xsv a = xsv_of(n->a); r.w = r.has_n = 1; r.n = !(a.w && (a.has_n ? a.n != 0 : 0)); break; }
    case XN_AND: case XN_OR: {
        struct xsv a = xsv_of(n->a), b = xsv_of(n->b);
        int t = a.w && (a.has_n ? a.n != 0 : 0), u = b.w && (b.has_n ? b.n != 0 : 0);
        r.w = r.has_n = 1;
        r.n = n->k == XN_AND ? t && u : t || u;
        break;
    }
    case XN_CMP: {
        struct xsv a = xsv_of(n->a), b = xsv_of(n->b);
        int op = n->op;
        r.w = r.has_n = 1;
        if (!a.w || !b.w) r.n = 0;
        else if (a.has_n && b.has_n) { int d = a.n < b.n ? -1 : a.n > b.n;
            r.n = op == '<' ? d < 0 : op == '>' ? d > 0 : op == 'l' ? d <= 0 : op == 'g' ? d >= 0 : op == '=' ? d == 0 : d != 0; }
        else r.n = op == 'l' || op == 'g' || op == '=';   // two texts "" and ""
        break;
    }
    case XN_COAL: { struct xsv a = xsv_of(n->a); r = a.w > 0 ? a : xsv_of(n->b); break; }
    case XN_COND: { struct xsv c = xsv_of(n->c); int t = c.w && (c.has_n ? c.n != 0 : 0);
        if (t) r = xsv_of(n->a); else if (n->b) r = xsv_of(n->b); break; }
    default: break;   // names, calls, lists, functions and arithmetic: nothing while only reading
    }
    return r;
}
internal int x_script_fn(const char *name);   // the script functions (wait, wait_until, every, for, player, cmd): their arguments, -1 none
internal int x_name_known(const char *name);
internal int xc_fail(struct xcheck *k, const struct xnode *n, const char *why) { k->why = why; k->line = n->line; return 0; }
internal int x_check(struct xcheck *k, const struct xnode *n, struct xscope *sc);
// WHAT A VALUE IS, as far as the check knows (a script's): a kind of the host's objects, X_LIST_OF one, or X_ANY. A
// path a.b.c (up to upto): its head's - a variable's from its let, the host's own (game) - and each member's: one its
// kind hasn't, or a method without ( ), a mistake (-1, told in k). *known: its head is a variable or the host's
internal int x_path_kind(struct xcheck *k, const struct xnode *n, const char *s, size_t upto, const struct xscope *sc, int *known) {
    size_t hn = strcspn(s, "."), at;
    char m[40];
    if (hn > upto) hn = upto;
    if (hn >= sizeof m) { *known = 0; return X_ANY; }
    lstrcpynA(m, s, (int)hn + 1);
    int var = xsc_find(sc, s, hn, NULL) != 0, kind = var ? xsc_kind(sc, s, hn) : g_x_host.root_kind(m);
    *known = var || kind;
    for (at = hn; at < upto && kind != X_ANY; ) {
        const char *seg = s + at + 1;
        size_t sn = strcspn(seg, ".");
        if (at + 1 + sn > upto) sn = upto - at - 1;
        if (sn >= sizeof m) return X_ANY;
        lstrcpynA(m, seg, (int)sn + 1);
        if (kind & X_LIST_OF) {
            snprintf(k->whybuf, sizeof k->whybuf, "%.*s: a list of %s - one of them: [1].%s", (int)(at + 1 + sn), s, x_kind_name(kind), m);
            xc_fail(k, n, k->whybuf);
            return -1;
        }
        const struct x_kmember *km = x_kind_member(kind, m);
        if (!km || km->amin >= 0) {
            char t[100];
            x_kind_list(kind, t, sizeof t);
            if (km) snprintf(k->whybuf, sizeof k->whybuf, "%.*s: %s is a method - %s(...)", (int)(at + 1 + sn), s, m, m);
            else snprintf(k->whybuf, sizeof k->whybuf, "%.*s: a %s has no %s - its members: %s", (int)(at + 1 + sn), s, x_kind_name(kind), m, t);
            xc_fail(k, n, k->whybuf);
            return -1;
        }
        kind = km->result;
        at += 1 + sn;
    }
    return at >= upto ? kind : X_ANY;
}
internal int x_skind(struct xcheck *k, const struct xnode *n, const struct xscope *sc) {
    if (!n || !k->script) return X_ANY;
    int known, r, a;
    const struct x_kmember *m;
    switch (n->k) {
    case XN_NAME: r = x_path_kind(k, n, n->s, strlen(n->s), sc, &known); return r < 0 ? X_ANY : r;
    case XN_CALL: {
        const char *dot = strrchr(n->s, '.');
        if (!dot) return x_script_fn(n->s) >= 0 ? g_x_host.fn_kind(n->s) : X_ANY;   // player(2)
        r = x_path_kind(k, n, n->s, (size_t)(dot - n->s), sc, &known);
        m = r > 0 ? x_kind_member(r, dot + 1) : NULL;
        return m && m->amin >= 0 ? m->result : X_ANY;
    }
    case XN_MEMBER: case XN_MCALL:
        m = x_kind_member(x_skind(k, n->a, sc), n->s);
        return m && (m->amin >= 0) == (n->k == XN_MCALL) ? m->result : X_ANY;
    case XN_INDEX: a = x_skind(k, n->a, sc); return a & X_LIST_OF ? a & 0xFF : X_ANY;
    case XN_COND: case XN_COAL: a = x_skind(k, n->a, sc); return n->b && x_skind(k, n->b, sc) == a ? a : X_ANY;
    default: return X_ANY;
    }
}
// o.name(...) of a kind: a method of it, with as many arguments
internal int x_check_method(struct xcheck *k, const struct xnode *n, int kind, const char *name, int na) {
    const struct x_kmember *m = x_kind_member(kind, name);
    char t[100];
    if (!m) {
        x_kind_list(kind, t, sizeof t);
        snprintf(k->whybuf, sizeof k->whybuf, "%s(...): a %s has no %s - its members: %s", name, x_kind_name(kind), name, t);
    } else if (m->amin < 0) snprintf(k->whybuf, sizeof k->whybuf, "%s(...): %s is no method - without ( )", name, name);
    else if (na < m->amin || na > m->amax) {
        if (m->amin == m->amax) snprintf(k->whybuf, sizeof k->whybuf, "%s takes %d argument%s", name, m->amin, m->amin == 1 ? "" : "s");
        else snprintf(k->whybuf, sizeof k->whybuf, "%s takes %d to %d arguments", name, m->amin, m->amax);
    } else return 1;
    return xc_fail(k, n, k->whybuf);
}
// a new name (a let's, a for's): none it would hide - of this block, a block around it, the code around the function,
// a value of the game's or the language's, a script function (in a script). A pipe command's may (wave, count): a
// command is only called, and a variable of its name is what the code around means
internal int x_check_new_name(struct xcheck *k, const struct xnode *n, const char *name, const struct xscope *sc) {
    int own = 0;
    if (xsc_find(sc, name, strlen(name), &own))
        snprintf(k->whybuf, sizeof k->whybuf, own ? "%s is a name here already (one let a name)" :
                 "%s is a name of the code around this function already - give this one another", name);
    else if (k->script && (x_name_known(name) || !strcmp(name, "time") || !strcmp(name, "stop") || x_script_fn(name) >= 0))
        snprintf(k->whybuf, sizeof k->whybuf, "%s is a name the language or the game has - give the variable another", name);
    else return 1;
    return xc_fail(k, n, k->whybuf);
}
internal int x_check_body(struct xcheck *k, const struct xnode *body, struct xscope *sc) {   // a block: its lets first
    if (!body || body->k != XN_BLOCK) return x_check(k, body, sc);
    for (int i = 0; i < body->n_items; i++) {   // known in the whole block - a function made before may call one later
        const struct xnode *s = body->items[i];
        if (s->k != XN_LET) continue;
        if (!x_check_new_name(k, s, s->s, sc)) return 0;
        if (sc->n < 64) { sc->decl[sc->n] = 0; sc->name[sc->n++] = s->s; }
    }
    for (int i = 0; i < body->n_items; i++) if (!x_check(k, body->items[i], sc)) return 0;
    return 1;
}
internal int x_check_block(struct xcheck *k, const struct xnode *blk, const struct xscope *sc) {   // an if's, a loop's
    struct xscope b = { sc, { 0 }, 0, { 0 }, 1, { 0 } };
    return x_check_body(k, blk, &b);
}
internal int x_check(struct xcheck *k, const struct xnode *n, struct xscope *sc) {
    if (!n) return 1;
    switch (n->k) {
    case XN_NUM: return 1;
    case XN_STR: {
        const char *why;
        g_x_check_line = k->script ? n->line : g_x_check_line;
        if (!x_fills_ok(n->s, &why, k, sc)) return xc_fail(k, n, why);   // its placeholders see the names around it
        return 1;
    }
    case XN_NAME: {
        size_t len = strcspn(n->s, ".");
        int f = xsc_find(sc, n->s, len, NULL);
        if (f == 2) { snprintf(k->whybuf, sizeof k->whybuf, "%.*s is read before its let", (int)len, n->s); return xc_fail(k, n, k->whybuf); }
        if (k->script && n->s[len]) {   // p.towers, game.players: the members its kind has (a kind known)
            int known;
            if (x_path_kind(k, n, n->s, strlen(n->s), sc, &known) < 0) return 0;
            if (known) return 1;
        }
        if (f) return 1;   // a parameter or a variable (its members: diplo.c)
        if (k->script && (!strcmp(n->s, "stop") || !strcmp(n->s, "time"))) return 1;
        if (k->script) {
            if (x_name_known(n->s)) return 1;
            snprintf(k->whybuf, sizeof k->whybuf, "an unknown name: %s", n->s);
            return xc_fail(k, n, k->whybuf);
        }
        if (g_x_check_file) { int keep = g_x_check_line; x_name_check(n->s); g_x_check_line = keep; }
        return 1;
    }
    case XN_LAMBDA: {
        struct xscope s = { sc, { 0 }, 0, { 0 }, 0, { 0 } };
        for (int i = 0; i < n->n_items; i++) {   // its parameters: none hides a name around it either
            if (!x_check_new_name(k, n->items[i], n->items[i]->s, sc)) return 0;
            xsc_add(&s, n->items[i]->s);
        }
        int loops = k->loops;   // (a break in it is none of a loop around it)
        k->loops = 0;
        int ok = x_check_body(k, n->a, &s);
        k->loops = loops;
        return ok;
    }
    case XN_BLOCK: {   // a script's statements (its top)
        return x_check_body(k, n, sc);
    }
    case XN_LET:
        if (!x_check(k, n->a, sc)) return 0;   // (its own name not yet: let n = n + 1 is a mistake)
        xsc_pass(sc, n->s, x_skind(k, n->a, sc));
        return 1;
    case XN_ASSIGN: {
        int f = xsc_find(sc, n->s, strlen(n->s), NULL);   // (the code around's too: set in its own frame)
        if (!f) snprintf(k->whybuf, sizeof k->whybuf, "%s = ...: no variable %s - let %s = ... makes one", n->s, n->s, n->s);
        else if (f == 2) snprintf(k->whybuf, sizeof k->whybuf, "%s is set before its let", n->s);
        else {
            if (!x_check(k, n->a, sc)) return 0;
            int kv = xsc_kind(sc, n->s, strlen(n->s)), kr = x_skind(k, n->a, sc);
            if (!kv || !kr || kv == kr) return 1;   // a kind it has from its let: another known one can't go in
            snprintf(k->whybuf, sizeof k->whybuf, "%s is a %s%s (its let) - a %s%s can't go in", n->s,
                     kv & X_LIST_OF ? "list of " : "", x_kind_name(kv), kr & X_LIST_OF ? "list of " : "", x_kind_name(kr));
        }
        return xc_fail(k, n, k->whybuf);
    }
    case XN_IF:
        if (!x_check(k, n->c, sc) || !x_check_block(k, n->a, sc)) return 0;
        return !n->b || (n->b->k == XN_IF ? x_check(k, n->b, sc) : x_check_block(k, n->b, sc));
    case XN_WHILE: case XN_FOR: {
        if (!x_check(k, n->k == XN_WHILE ? n->c : n->a, sc) || !x_check(k, n->b, sc)) return 0;
        struct xscope b = { sc, { 0 }, 0, { 0 }, 1, { 0 } };
        if (n->k == XN_FOR) {   // its name: a list's items' kind (for p in game.players: a player)
            if (!x_check_new_name(k, n, n->s, sc)) return 0;
            xsc_add(&b, n->s);
            int lk = n->b ? X_ANY : x_skind(k, n->a, sc);
            b.kind[0] = (short)(lk & X_LIST_OF ? lk & 0xFF : X_ANY);
        }
        k->loops++;
        int ok = x_check_body(k, n->k == XN_WHILE ? n->a : n->c, &b);
        k->loops--;
        return ok;
    }
    case XN_BREAK: case XN_CONTINUE:
        return k->loops ? 1 : xc_fail(k, n, n->k == XN_BREAK ? "break outside a loop" : "continue outside a loop");
    case XN_RETURN: return x_check(k, n->a, sc);
    case XN_MEMBER: case XN_MCALL: case XN_INDEX: {   // f(a).x, l[1].name(...): an object's - of a kind known - checked
        if (!x_check(k, n->a, sc) || !x_check(k, n->b, sc)) return 0;
        for (int i = 0; i < n->n_items; i++) if (!x_check(k, n->items[i], sc)) return 0;
        int a = x_skind(k, n->a, sc);
        if (!a) return 1;   // (a record, a vector, not known: worked out, there?)
        if (n->k == XN_INDEX) {
            if (a & X_LIST_OF) return 1;
            snprintf(k->whybuf, sizeof k->whybuf, "[...]: a %s is no list", x_kind_name(a));
            return xc_fail(k, n, k->whybuf);
        }
        if (a & X_LIST_OF) {
            snprintf(k->whybuf, sizeof k->whybuf, ".%s: a list of %s - one of them: [1].%s", n->s, x_kind_name(a), n->s);
            return xc_fail(k, n, k->whybuf);
        }
        if (n->k == XN_MCALL) return x_check_method(k, n, a, n->s, n->n_items);
        const struct x_kmember *m = x_kind_member(a, n->s);
        if (m && m->amin < 0) return 1;
        char t[100];
        x_kind_list(a, t, sizeof t);
        if (m) snprintf(k->whybuf, sizeof k->whybuf, ".%s: a method - %s(...)", n->s, n->s);
        else snprintf(k->whybuf, sizeof k->whybuf, ".%s: a %s has no %s - its members: %s", n->s, x_kind_name(a), n->s, t);
        return xc_fail(k, n, k->whybuf);
    }
    case XN_REC:
        for (int i = 0; i < n->n_items; i++) if (!x_check(k, n->items[i]->a, sc)) return 0;
        return 1;
    case XN_CALL: {
        for (int i = 0; i < n->n_items; i++) if (!x_check(k, n->items[i], sc)) return 0;   // the arguments first
        const char *name = n->s;
        int nk = n->n_items < 8 ? n->n_items : 8, f = x_fn_id(name, nk);
        struct xsv a0 = nk ? xsv_of(n->items[0]) : (struct xsv){ 0, 0, 0, 0 };
        if (f == 1 && a0.has_n && a0.n > 1) return xc_fail(k, n, "chance takes a probability, 0 to 1: chance(70%) or chance(0.7)");
        if (f) return 1;
        if (!strchr(name, '.')) {
            int v = xsc_find(sc, name, strlen(name), NULL);
            if (v == 2) { snprintf(k->whybuf, sizeof k->whybuf, "%s is called before its let", name); return xc_fail(k, n, k->whybuf); }
            if (v) return 1;   // a function in a variable
            int sf = k->script ? x_script_fn(name) : -1;
            if (sf >= 0) {
                if (sf != nk) { snprintf(k->whybuf, sizeof k->whybuf, "%s takes %d argument%s", name, sf, sf == 1 ? "" : "s"); return xc_fail(k, n, k->whybuf); }
                return 1;
            }
            if (k->script && g_x_host.command(name)) return 1;
            int want = x_fn_known(name);
            if (want >= 0) {
                if (want != nk) { snprintf(k->whybuf, sizeof k->whybuf, "%s takes %d argument%s", name, want, want == 1 ? "" : "s"); return xc_fail(k, n, k->whybuf); }
                return 1;
            }
        }
        const char *dot = strrchr(name, '.');
        if (k->script && dot) {   // p.opinion(q), game.player(2): a method of its kind (a variable of no kind known: when it runs)
            int known, r = x_path_kind(k, n, name, (size_t)(dot - name), sc, &known);
            if (r < 0) return 0;
            if (known) {
                if (r == X_ANY) return 1;
                if (r & X_LIST_OF) {
                    snprintf(k->whybuf, sizeof k->whybuf, "%s(...): a list of %s - one of them: [1].%s(...)", name, x_kind_name(r), dot + 1);
                    return xc_fail(k, n, k->whybuf);
                }
                return x_check_method(k, n, r, dot + 1, n->n_items);
            }
        }
        if (!g_x_host.method(k->c, name, NULL, nk, NULL)) {
            if (!k->script) return xc_fail(k, n, "an unknown function or relation (or the wrong number of arguments)");
            snprintf(k->whybuf, sizeof k->whybuf, "%s(...): no such function, relation or command (or the wrong number of arguments)", name);
            return xc_fail(k, n, k->whybuf);
        }
        return 1;
    }
    case XN_LIST:
        for (int i = 0; i < n->n_items; i++) if (!x_check(k, n->items[i], sc)) return 0;
        return 1;
    case XN_ADD: case XN_MUL: case XN_NEG:   // [1, 2] + 10, "a" + b: as written - a mistake before it is worked out
        if ((n->a && n->a->k == XN_LIST) || (n->b && n->b->k == XN_LIST))
            return xc_fail(k, n, "a list where one number belongs - pick(l) for one of its items, \"{l}\" in a text");
        if (n->k == XN_ADD && ((n->a && n->a->k == XN_STR) || (n->b && n->b->k == XN_STR)))
            return xc_fail(k, n, "+ adds numbers - a text is built in its string: \"Age: {epoch}\", \"{a}{b}\"");
        return x_check(k, n->a, sc) && x_check(k, n->b, sc);
    case XN_CMP: {
        if ((n->a && n->a->k == XN_LIST) || (n->b && n->b->k == XN_LIST))
            return xc_fail(k, n, "a list compared - contains(l, x) for one of its items");
        if (!x_check(k, n->a, sc) || !x_check(k, n->b, sc)) return 0;
        struct xsv a = xsv_of(n->a), b = xsv_of(n->b);   // a text and a number, both written as such: "12" == 12
        if (a.w && b.w && ((!a.has_n && !b.has_s) || (!a.has_s && !b.has_n))) {
            x_error("a text (\"\") compared with a number - a number has no quotes, number(t) makes one: ");
            k->c->failed++;
            return xc_fail(k, n, "a mistake while working it out");
        }
        return 1;
    }
    default:
        return x_check(k, n->c, sc) && x_check(k, n->a, sc) && x_check(k, n->b, sc);
    }
}

// ===== WORKING IT OUT =====
// A frame: the names a function sees - its parameters and its lets (own), and those of the code around it (copied in
// when it is called; read from and set in their own frame, up). The context's c->fpn / fpv point at them while it runs, so the game's
// values find a player in one (p.soldiers - diplo.c x_path).
#define XF_MAX 48
// keep: a frame that outlives its functions (a script's top, a trigger's copy - TRIGGERS) - taken as it is, not copied.
// up: the frame its function was made in, where the names it copied (own 0) belong - read there when used, set there
struct xframe { int n, keep; const char *name[XF_MAX]; struct xval val[XF_MAX]; unsigned char own[XF_MAX]; struct xframe *up; };
internal struct xframe *x_frame_owner(struct xframe *f, const char *name, int *idx) {   // the frame name is its own in
    for (; f; f = f->up)
        for (int i = 0; i < f->n; i++) if (f->own[i] && !strcmp(f->name[i], name)) { *idx = i; return f; }
    return NULL;
}
// the code around's variables a function set while it ran, in a frame that goes on (not kept): their values live in
// what the function's own are freed with - moved below when it returns (x_call_value), up to their frame's own end.
// base: the running call's first entry
#define XDIRTY_MAX 256
internal struct { struct xframe *f; int i; } g_xdirty[XDIRTY_MAX];
internal int g_xdirty_n, g_xdirty_base;
struct xfn { const struct xnode *lam; struct xframe *env; };   // a function value: its definition and where it was made
// err: what is being worked out failed (its mistake told, or none: not an expression); why: what it was; f: the
// frame of the function (or script) running; ret: a return given (its value in *retv); err_at: the statement it
// failed in; brk: a break (1) or continue (2) given, for the loop around; steps: loop rounds since it last waited
struct xe { struct xctx *c; int err; const char *why; struct xframe *f; int ret; struct xval *retv; const struct xnode *err_at; int kept;
            int brk; int steps; };
// a loop that never waits holds the game (a script runs on its thread): this many rounds without a wait - of all
// loops in it together, counted, not timed (every PC stops at the same one) - and it is a mistake
#define X_LOOP_STEPS 1000000
internal void xe(struct xe *e, const struct xnode *n, struct xval *v);
internal void x_use(struct xe *e, const struct xval *v) {
    if (!v->miss[0]) return;
    x_missing(e->c, v->miss);
    e->c->missed = 0;
    e->err = 1;
    e->why = "a mistake while working it out";
}
// a mistake while working it out (MISTAKES WHILE WORKING OUT above): told once, and what it is in fails
internal char g_x_last_fail[160];   // the last one, for texts eval (the log has each once only)
internal PRINTF_LIKE(2, 3) void x_fail(struct xe *e, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(g_x_last_fail, sizeof g_x_last_fail, fmt, ap);
    va_end(ap);
    if (!e->c->fixed) x_error("%s: %.100s", g_x_last_fail, e->c->top ? e->c->top : "");   // fixed: ini.c tells it
    e->c->failed++;
    e->err = 1;
    e->why = "a mistake while working it out";
}
internal int x_numeric_ok(struct xe *e, const struct xval *v) {   // v counts as a number: one, or nothing (0) - a text is a mistake
    if (v->ln) { x_fail(e, "a list (%.40s) where one number belongs - %s", xtext_of(v), "pick(l) for one of its items"); return 0; }
    if (v->kind == XV_VEC && v->w) { x_fail(e, "a vector %.40s where one number belongs - .x, .y ... for one of its numbers", xtext_of(v)); return 0; }
    if (v->kind == XV_REC && v->w) { x_fail(e, "a record %.40s where one number belongs - r.name for one of its members", xtext_of(v)); return 0; }
    if (v->kind == XV_TRIG && v->w) { x_fail(e, "a trigger where one number belongs - t.on, t.fired"); return 0; }
    if (!v->w || v->has_n) return 1;
    x_fail(e, "a text (\"%.40s\") where a number belongs - number(x) makes a number of a text", xtext_of(v));
    return 0;
}
// a text against a number ("12" == 12, "9" < 10): no comparison - neither turns into the other
internal int xmismatch(const struct xval *a, const struct xval *b) { return (!a->has_n && !b->has_s) || (!a->has_s && !b->has_n); }
internal int xvec_eq(const struct xval *a, const struct xval *b) {   // two vectors, the same numbers
    if (a->vn != b->vn) return 0;
    for (int i = 0; i < a->vn; i++) if (a->vc[i] != b->vc[i]) return 0;
    return 1;
}
internal int xcmp(const struct xval *a, const struct xval *b) {   // <0, 0, >0: players the same one or not, numbers, else texts
    double x, y;
    if (a->obj && b->obj) return a->okind != b->okind ? a->okind - b->okind : a->obj - b->obj;
    if (a->kind == XV_VEC || b->kind == XV_VEC) return a->kind == b->kind && xvec_eq(a, b) ? 0 : 2;   // equal or not
    if (a->kind == XV_REC || b->kind == XV_REC) return 2;   // (contains: a record is equal to nothing)
    if (xmismatch(a, b)) return 2;   // unequal
    if (xis_num(a, &x) && xis_num(b, &y)) return x < y ? -1 : x > y;
    return lstrcmpiA(xtext_of(a), xtext_of(b));
}
// the triggers (TRIGGERS below): t.on, t.fired, t.name read here
#define XT_MAX 128
struct xtrig { struct xval when, run; int on, fired, loop; double hold, since, period, next; char name[48]; };   // period: a timer's
internal struct xtrig g_xt[XT_MAX];
internal int g_xt_n;
// o.name (call 0) or o.name(a ...) - a member of its kind: a value read, a method called; one it hasn't, a value
// called or a method not, the wrong number of arguments: a mistake. whole: as written, for the messages
internal void x_obj_member(struct xe *e, const struct xval *o, const char *name, const struct xval *a, int na, int call,
                           struct xval *v, const char *whole) {
    const struct x_kmember *m = x_kind_member(o->okind, name);
    char t[200];
    if (!m) {
        x_kind_list(o->okind, t, sizeof t);
        x_fail(e, "%s: a %s has no %s - its members: %s", whole, x_kind_name(o->okind), name, t);
        return;
    }
    if (!call && m->amin >= 0) { x_fail(e, "%s: %s is a method - %s(...)", whole, name, name); return; }
    if (call && m->amin < 0) { x_fail(e, "%s(...): %s is no method - without ( )", whole, name); return; }
    if (call && (na < m->amin || na > m->amax)) {
        if (m->amin == m->amax) x_fail(e, "%s(...): takes %d argument%s", whole, m->amin, m->amin == 1 ? "" : "s");
        else x_fail(e, "%s(...): takes %d to %d arguments", whole, m->amin, m->amax);
        return;
    }
    struct xval oc = *o;   // (v may be o)
    xnothing(v);
    if (!g_x_host.member(e->c, &oc, name, a, na, v)) x_fail(e, "%s: not worked out", whole);
    else if (e->c->missed) { e->c->missed = 0; e->err = 1; e->why = "a mistake while working it out"; }   // told (x_missing)
}
// v's member name - a record's (a mistake when it has none: its members told), a vector's number (.x .y .z .w), an
// object's (its kind's); whole: the path as written, for the messages
internal void x_member_of(struct xe *e, const char *name, struct xval *v, const char *whole) {
    char dn[44];
    if (!whole) { snprintf(dn, sizeof dn, ".%s", name); whole = dn; }   // after a call or ( ): .name
    if (!v->w && v->miss[0]) { x_use(e, v); return; }   // lord.x without a lord: who isn't there, told
    if (v->obj && v->w) { x_obj_member(e, v, name, NULL, 0, 0, v, whole); return; }
    if (v->kind == XV_REC && v->w) {
        const struct xval *m = xrec_get(v, name);
        if (m) { *v = *m; return; }
        char t[160];
        int o = 0;
        t[0] = 0;
        for (int i = 0; i < v->rec->n && o < (int)sizeof t - 44; i++) o += snprintf(t + o, sizeof t - (size_t)o, "%s%s", i ? ", " : "", v->rec->key[i]);
        x_fail(e, "%s: no member %s - its members: %s (has(r, \"%s\") checks)", whole, name, v->rec->n ? t : "none", name);
        return;
    }
    if (v->kind == XV_VEC && v->w) {
        int m = xvec_member(name);
        if (m < 0 || m >= v->vn) { x_fail(e, "%s: %s has %d numbers - no .%s", whole, xtext_of(v), v->vn, name); return; }
        xnum(v, v->vc[m]);
        return;
    }
    if (v->kind == XV_TRIG && v->w) {   // t.on, t.fired, t.name (TRIGGERS)
        const struct xtrig *t = v->n >= 0 && v->n < g_xt_n ? &g_xt[(int)v->n] : NULL;
        if (t && !strcmp(name, "on")) { xnum(v, t->on); return; }
        if (t && !strcmp(name, "fired")) { xnum(v, t->fired); return; }
        if (t && !strcmp(name, "name")) { xstr(v, t->name); return; }
        x_fail(e, "%s: a trigger's members are on, fired, name", whole);
        return;
    }
    x_fail(e, "%s: %.40s is no record, vector or object - a member is one of theirs", whole, xtext_of(v));
}
internal void xe_ident(struct xe *e, const char *name, struct xval *v);
// a file's record as a whole - colors of "colors = { dark = ..., light = ... }", whose members are the vars entries
// colors.dark, colors.light: each member worked out now (a record in it a record again); 0: vs has none of that name
internal int x_vars_members(const struct xvars *vs, const char *name, char child[XVARS_MAX][40]) {   // its members' names
    if (!vs || !vs->dotted) return 0;
    size_t n = strlen(name);
    int nc = 0;
    for (int i = 0; i < vs->n; i++) {   // in the order written, each once
        const char *k = vs->key[i];
        if (strncmp(k, name, n) || k[n] != '.' || strchr(k, '(')) continue;
        size_t m = strcspn(k + n + 1, ".");
        int dup = m >= 40;
        for (int j = 0; j < nc && !dup; j++) dup = strlen(child[j]) == m && !strncmp(child[j], k + n + 1, m);
        if (!dup) { memcpy(child[nc], k + n + 1, m); child[nc++][m] = 0; }
    }
    return nc;
}
// a.b.missing of a file's record a.b: no such member - told with those it has (none: 0, an unknown name)
internal int x_vars_no_member(struct xe *e, const char *name) {
    const char *last = strrchr(name, '.');
    if (!last || last - name >= 120) return 0;
    char parent[128], child[XVARS_MAX][40];
    lstrcpynA(parent, name, (int)(last - name) + 1);
    for (int k = 0; k < 3; k++) {
        int nc = x_vars_members(e->c->vars[k], parent, child);
        if (!nc) continue;
        char t[160];
        int o = 0;
        for (int i = 0; i < nc && o < (int)sizeof t - 44; i++) o += snprintf(t + o, sizeof t - (size_t)o, "%s%s", i ? ", " : "", child[i]);
        x_fail(e, "%s: no member %s - its members: %s (has(r, \"%s\") checks)", name, last + 1, t, last + 1);
        return 1;
    }
    return 0;
}
internal int x_vars_group(struct xe *e, const struct xvars *vs, const char *name, struct xval *v) {
    char child[XVARS_MAX][40];
    int nc = x_vars_members(vs, name, child);
    if (!nc) return 0;
    struct xrec *r = x_alloc(sizeof *r);
    const char **key = x_alloc((size_t)nc * sizeof *key);
    struct xval *val = x_alloc((size_t)nc * sizeof *val);
    if (!r || !key || !val) { e->err = 1; return 1; }
    r->n = nc; r->key = key; r->val = val;
    for (int i = 0; i < nc && !e->err; i++) {
        char full[128];
        snprintf(full, sizeof full, "%s.%s", name, child[i]);
        key[i] = x_strdup(child[i], strlen(child[i]));
        xe_ident(e, full, &val[i]);
    }
    if (e->err) return 1;
    xnothing(v);
    v->w = 1; v->kind = XV_REC; v->rec = r;
    return 1;
}
internal void x_nogame(struct xe *e, const char *name) {   // a game value or a player where there is no game yet
    x_fail(e, "%s: no game yet - only constants, chance(), rnd(), language and vars here", name);
}
// NESTING: names, functions and placeholders worked out within each other, at most X_NEST deep - also the end of
// names naming each other in a circle. 12: a prompt's piece, its names and functions, their {placeholders}, a group
// of mod/units.eeai of groups (military: army ...) - 6 cut the prompt's fighting_recent short. Each of them counts
// its parser levels (X_LEVELS) on its own, as a lambda's body does: added up through the nesting they ran out where
// a group of mod/units.eeai sat at the end of a prompt's chain (a name's body, an expression in itself)
#define X_NEST 12
// the body of a vars entry or function (its source) worked out in this context: compiled, its value
internal int x_body(struct xe *e, const char *src, struct xval *v) {
    const struct xunit *u = x_compiled(src);
    if (!u->ok) { xnothing(v); return 0; }
    struct xe q = { e->c, 0, NULL, NULL, 0, NULL, NULL, 0, 0, 0 };
    int level = e->c->level;
    e->c->level = 0;
    xe(&q, u->root, v);
    e->c->level = level;
    return !q.err;
}
internal void xe_ident(struct xe *e, const char *name, struct xval *v) {
    struct xctx *c = e->c;
    if (c->fixed) { x_fail(e, "%s: a name - a setting is a number (35%%, 2 * 60, min(...))", name); xnothing(v); return; }
    const char *dot = strchr(name, '.');
    if (dot && dot - name < 40) {   // r.a.b, home.x: a member path - its head a parameter, variable or vars entry
        char head[40];
        lstrcpynA(head, name, (int)(dot - name) + 1);
        int own = 0, exact = 0;
        for (int i = 0; i < c->fnp && !own; i++) own = !strcmp(c->fpn[i], head);
        for (int k = 0; k < 3 && !own; k++) exact |= c->vars[k] && c->vars[k]->dotted && xvars_get(c->vars[k], name);   // a file record's member itself
        for (int k = 0; k < 3 && !own && !exact; k++) own = xvars_get(c->vars[k], head) != NULL;
        if (own) {
            struct xval h;
            xe_ident(e, head, &h);
            if (e->err) return;
            if (!(h.obj && h.w && g_x_host.value(c, name, v))) {   // (a player's: the host's own way first - p.soldiers)
                for (const char *seg = dot + 1; !e->err;) {
                    char m[40];
                    size_t k = strcspn(seg, ".");
                    lstrcpynA(m, seg, (int)(k < 39 ? k : 39) + 1);
                    x_member_of(e, m, &h, name);
                    if (!seg[k]) break;
                    seg += k + 1;
                }
                if (!e->err) *v = h;
            }
            return;
        }
    }
    for (int i = 0; i < c->fnp; i++)   // a parameter or variable of the function being worked out
        if (!strcmp(c->fpn[i], name)) {
            struct xframe *f = e->f;
            int j;
            if (f && c->fpv == f->val && !f->own[i]) {   // the code around's: as it is now in its own frame (set since?)
                struct xframe *o = x_frame_owner(f->up, name, &j);
                if (o) f->val[i] = o->val[j];
            }
            *v = c->fpv[i];
            return;
        }
    for (int i = 0; i < c->np; i++)
        if (!strcmp(c->pn[i], name)) { xstr_or_num(v, c->pv[i]); return; }
    for (int k = 0; k < 3; k++) {
        const char *def = xvars_get(c->vars[k], name);
        if (!def) continue;
        if (c->depth >= X_NEST) { c->circle = 1; e->err = 1; return; }   // vars naming each other in a circle
        const char *const *fpn = c->fpn;   // a variable sees no parameters (only its own function's body does)
        const struct xval *fpv = c->fpv;
        int fnp = c->fnp;
        c->fnp = 0;
        c->depth++;
        if (!x_body(e, def, v) || c->circle) e->err = 1;
        c->depth--;
        c->fpn = fpn; c->fpv = fpv; c->fnp = fnp;
        return;
    }
    for (int k = 0; k < 3; k++) if (x_vars_group(e, c->vars[k], name, v)) return;   // a file's record whole
    if (c->script && !strcmp(name, "stop")) { xnothing(v); v->w = 1; v->kind = XV_STOP; return; }   // for / every: no more
    if (c->script && !strcmp(name, "time")) { xnum(v, g_x_hooks.now ? g_x_hooks.now() : 0); return; }   // game seconds
    int k = g_x_host.constant(name, v);
    if (k) { e->kept = k == 2; return; }
    if (c->nogame && lstrcmpiA(name, "language") && (g_x_host.is_value(name) || strchr(name, '.'))) { x_nogame(e, name); return; }
    if (g_x_host.value(c, name, v)) return;
    if (x_vars_no_member(e, name)) return;   // colors.missing
    x_fail(e, !strcmp(name, "name") ? "an unknown name: name - the other player is them (them.name)" : "an unknown name: %s", name);
}
// a function's body (def), its parameters (par "a,b)") bound to the arguments: they alone are its names besides the
// game's values and the vars - a function calling itself ends at the nesting limit like vars in a circle
internal void x_call(struct xe *e, const char *def, const char *par, const struct xval *a, int na, struct xval *v) {
    struct xctx *c = e->c;
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
    if (n != na) { e->err = 1; return; }
    if (c->depth >= X_NEST) { c->circle = 1; e->err = 1; return; }
    const char *const *fpn = c->fpn;
    const struct xval *fpv = c->fpv;
    int fnp = c->fnp;
    c->fpn = pn; c->fpv = a; c->fnp = n;
    c->depth++;
    if (!x_body(e, def, v) || c->circle) e->err = 1;
    c->depth--;
    c->fpn = fpn; c->fpv = fpv; c->fnp = fnp;
}
internal void x_exec(struct xe *e, const struct xnode *blk);
// a value with its texts and lists copied into the region now (what a function hands back, copied past its frame)
internal void xv_copy(struct xval *dst, const struct xval *src) {
    *dst = *src;
    if (src->has_s || src->w) dst->s = x_strdup(src->s, strlen(src->s));
    if (src->kind == XV_VEC) xvec(dst, src->vc, src->vn);
    if (src->kind == XV_REC) {   // its members, each copied
        struct xrec *r = x_alloc(sizeof *r);
        const char **key = x_alloc((size_t)src->rec->n * sizeof *key + 1);
        struct xval *val = x_alloc((size_t)src->rec->n * sizeof *val + 1);
        if (!r || !key || !val) { xnothing(dst); return; }
        r->n = src->rec->n; r->key = key; r->val = val;
        for (int i = 0; i < r->n; i++) { key[i] = x_strdup(src->rec->key[i], strlen(src->rec->key[i])); xv_copy(&val[i], &src->rec->val[i]); }
        dst->rec = r;
    }
    if (src->ln) {
        struct xval *it = x_alloc((size_t)src->ln * sizeof *it);
        if (!it) { dst->ln = 0; dst->it = NULL; return; }
        for (int i = 0; i < src->ln; i++) xv_copy(&it[i], &src->it[i]);
        dst->it = it;
    }
}
internal int xv_made_in(const struct xval *v, const struct xframe *f) {   // a function made in frame f, in it or its items
    if (v->kind == XV_FN && v->fn->env == f) return 1;
    for (int i = 0; i < v->ln; i++) if (xv_made_in(&v->it[i], f)) return 1;
    for (int i = 0; v->kind == XV_REC && i < v->rec->n; i++) if (xv_made_in(&v->rec->val[i], f)) return 1;
    return 0;
}
// call function value fv with its arguments: a frame of its own (the names around where it was made - copies, read and
// set in their own frame - and its parameters), its body worked out - a value, or a block's return - and its values
// freed but what it hands back and what it set in the code around
internal void x_call_value(struct xe *e, const struct xval *fv, const struct xval *a, int na, struct xval *v) {
    struct xctx *c = e->c;
    const struct xnode *lam = fv->fn->lam;
    if (na != lam->n_items) { x_fail(e, "a function of %d argument%s called with %d", lam->n_items, lam->n_items == 1 ? "" : "s", na); return; }
    if (c->calls >= X_CALLS) { x_fail(e, "functions called inside each other %d deep - one that calls itself without end?", X_CALLS); return; }
    struct xframe *nf = malloc(sizeof *nf);
    if (!nf) { e->err = 1; return; }
    nf->n = 0;
    nf->keep = 0;   // gone when it returns: what keeps its functions copies it (TRIGGERS)
    nf->up = fv->fn->env;
    const struct xframe *env = fv->fn->env;
    for (int i = 0; env && i < env->n; i++) { nf->name[nf->n] = env->name[i]; nf->val[nf->n] = env->val[i]; nf->own[nf->n++] = 0; }
    for (int i = 0; i < na; i++) {   // the parameters (one with a name from around it: its own now)
        const char *pn = lam->items[i]->s;
        int j = 0;
        while (j < nf->n && strcmp(nf->name[j], pn)) j++;
        if (j == nf->n) { if (nf->n == XF_MAX) { free(nf); x_fail(e, "more than %d names in a function", XF_MAX); return; } nf->n++; }
        nf->name[j] = pn; nf->val[j] = a[i]; nf->own[j] = 1;
    }
    const char *const *fpn = c->fpn;
    const struct xval *fpv = c->fpv;
    int fnp = c->fnp, level = c->level, ret = e->ret;
    struct xframe *f0 = e->f;
    struct xval *retv0 = e->retv;
    struct xmark m = x_mark();
    struct xval rv;
    xnothing(&rv);
    int dbase = g_xdirty_base;
    g_xdirty_base = g_xdirty_n;
    c->fpn = nf->name; c->fpv = nf->val; c->fnp = nf->n;
    c->level = 0;
    c->calls++;
    e->f = nf;
    if (lam->a->k == XN_BLOCK) { e->retv = &rv; e->ret = 0; x_exec(e, lam->a); }
    else xe(e, lam->a, &rv);
    c->calls--;
    c->fpn = fpn; c->fpv = fpv; c->fnp = fnp; c->level = level;
    e->f = f0; e->retv = retv0; e->ret = ret;
    if (!e->err && xv_made_in(&rv, nf)) x_fail(e, "a function made inside a function can't be handed out of it");
    // what it hands back and what it set in the code around: copied aside, its values freed, copied back below
    int d = g_xdirty_base, kept = d;
    for (int i = d; i < g_xdirty_n; i++) if (g_xdirty[i].f != nf) g_xdirty[kept++] = g_xdirty[i];   // (its own frame ends)
    g_xdirty_n = kept;
    struct xregion tmp = { 0 }, *r0 = g_xr;
    g_xr = &tmp;
    struct xval t, *set = g_xdirty_n > d ? x_alloc((size_t)(g_xdirty_n - d) * sizeof *set) : NULL;
    if (!e->err) xv_copy(&t, &rv);
    for (int i = d; set && i < g_xdirty_n; i++) xv_copy(&set[i - d], &g_xdirty[i].f->val[g_xdirty[i].i]);
    g_xr = r0;
    x_release(m);
    if (!e->err) xv_copy(v, &t);
    for (int i = d; set && i < g_xdirty_n; i++) xv_copy(&g_xdirty[i].f->val[g_xdirty[i].i], &set[i - d]);
    x_region_free(&tmp);
    kept = d;   // the caller's list: one it has already once (each moved above)
    for (int i = d; i < g_xdirty_n; i++) {
        int dup = 0;
        for (int j = dbase; j < kept && !dup; j++) dup = g_xdirty[j].f == g_xdirty[i].f && g_xdirty[j].i == g_xdirty[i].i;
        if (!dup) g_xdirty[kept++] = g_xdirty[i];
    }
    g_xdirty_n = kept;
    g_xdirty_base = dbase;   // (the caller's: these are its now, also those it set itself before)
    free(nf);
}
// the script functions: wait(seconds), wait_until(time), every(step, from, until, f), player(n), cmd(text), the triggers'
internal int x_script_fn(const char *name) {
    static const struct { const char *n; int args; } k[] = { { "wait", 1 }, { "wait_until", 1 }, { "every", 4 }, { "player", 1 }, { "cmd", 1 },
                                                             { "trigger", 1 }, { "after", 2 }, { "enable", 1 }, { "disable", 1 } };
    for (size_t i = 0; i < sizeof k / sizeof *k; i++) if (!strcmp(k[i].n, name)) return k[i].args;
    return -1;
}
// a value as a command's argument: a number (whole without decimals), a text as it is, a player its number, a type
// its number (unit.x: both), a vector its numbers one argument each (a place: x y)
internal int x_cmd_arg(struct xe *e, const char *verb, const struct xval *v, char *out, int cap) {
    if ((v->kind && v->kind != XV_VEC) || !v->w || v->ln) { x_fail(e, "%s(...): %s as a command's argument", verb, v->ln ? "a list" : v->kind == XV_FN ? "a function" : v->kind == XV_REC ? "a record" : v->kind ? "stop" : "nothing"); return 0; }
    if (v->kind == XV_VEC) {
        int o = 0;
        for (int i = 0; i < v->vn; i++) {
            struct xval c;
            xnum(&c, v->vc[i]);
            if (i) o += snprintf(out + o, (size_t)(cap - o), " ");
            if (!x_cmd_arg(e, verb, &c, out + o, cap - o)) return 0;
            o += (int)strlen(out + o);
        }
        return 1;
    }
    if (v->obj) {   // a player its number; an object without one (the game) a mistake
        const struct x_kind *k = g_x_host.kind(v->okind);
        if (!k || !k->numbered) { x_fail(e, "%s(...): %s as a command's argument", verb, k ? k->name : "an object"); return 0; }
        snprintf(out, (size_t)cap, "%d", v->obj - 1);
    }
    else if (v->has_n) {
        double r = v->n < 0 ? -v->n : v->n;
        if (r - (double)(long long)r < 1e-9 && r < 1e15) snprintf(out, (size_t)cap, "%lld", (long long)v->n);
        else snprintf(out, (size_t)cap, "%.6g", v->n);
    } else {
        if (strchr(v->s, '\n')) { x_fail(e, "%s(...): a line break in a command's argument", verb); return 0; }
        lstrcpynA(out, v->s, cap);
    }
    return 1;
}
// a command run (the game's thread): its reply's first line is the value; a reply "error..." is a mistake
internal void x_command(struct xe *e, const char *cmd, struct xval *v) {
    static char reply[16384];
    reply[0] = 0;
    if (!g_x_hooks.command) { x_fail(e, "%.40s: commands only in a script", cmd); return; }
    g_x_hooks.command(cmd, reply, sizeof reply);
    char *nl = strchr(reply, '\n');
    size_t n = nl ? (size_t)(nl - reply) : strlen(reply);
    if (!strncmp(reply, "error", 5)) { x_fail(e, "%.60s -> %.*s", cmd, (int)(n < 120 ? n : 120), reply); return; }
    v->s = x_strdup(reply, n);
    v->has_s = 1; v->w = 1; v->has_n = 0; v->obj = v->okind = v->ln = 0; v->it = NULL; v->miss[0] = 0; v->kind = 0;
}
internal int g_xt_in;   // a trigger's when / run or an after's function is being worked out (TRIGGERS)
internal int x_wait_until(struct xe *e, double t) {   // the script waits until game second t (0: no script to wait in)
    if (g_xt_in) { x_fail(e, "a trigger's run can't wait - after(5s, () => ...) does it later"); return 0; }
    if (!g_x_hooks.wait) { x_fail(e, "waiting only in a script"); return 0; }
    g_x_hooks.wait(t);
    e->steps = 0;
    return 1;
}
// ===== TRIGGERS (scripts): t = trigger({ when = () => a condition, run = () => { ... }, loop = true, for = 5s, name =
// "..." }) - checked every 500 ms of game time, in the order made, by the script runner (x_triggers_tick), also while
// the script waits or after it ended: when its when holds (for: that long without a break) its run is worked out, and
// it is off then unless it loops. enable(t), disable(t); t.on, t.fired, t.name. after(5s, f): f worked out that much
// later - a trigger's run can't wait, its delays are after's. A mistake in a trigger's function turns it off (told).
// The functions keep the names around where they were made: a script's own (its top frame stays) as they are when
// they run, a function's as they were when the trigger or the after was made (copied: that frame is gone by then)
#define XA_MAX 256
struct xafter { double at; struct xval fn; char name[48]; };
internal struct xafter g_xa[XA_MAX];
internal int g_xa_n;
internal struct xregion g_xt_keep, g_xt_tmp;   // what they keep (their functions, frames); a turn's values
internal double g_xt_next;   // the next check (game seconds)
internal void x_triggers_reset(void) {   // a new game: none
    g_xt_n = g_xa_n = 0;
    g_xt_next = 0;
    x_region_free(&g_xt_keep);
    x_region_free(&g_xt_tmp);
}
// a function value kept for later (in g_xt_keep, which x_alloc uses meanwhile): the frame it was made in copied
// unless it stays (keep), each frame once (memo: a function in it made there too)
struct xt_memo { const struct xframe *from[16]; struct xframe *to[16]; int n; };
internal void xt_keep_val(struct xval *dst, const struct xval *src, struct xt_memo *m);
internal struct xframe *xt_keep_frame(struct xframe *f, struct xt_memo *m) {
    if (!f || f->keep) return f;
    for (int i = 0; i < m->n; i++) if (m->from[i] == f) return m->to[i];
    struct xframe *c = x_alloc(sizeof *c);
    if (!c) return NULL;
    if (m->n < 16) { m->from[m->n] = f; m->to[m->n++] = c; }
    c->n = f->n;
    c->keep = 1;
    for (int i = 0; i < f->n; i++) { c->name[i] = f->name[i]; c->own[i] = f->own[i]; xt_keep_val(&c->val[i], &f->val[i], m); }
    c->up = xt_keep_frame(f->up, m);   // (where its copied names belong: kept too, or the script's top)
    return c;
}
internal void xt_keep_val(struct xval *dst, const struct xval *src, struct xt_memo *m) {
    xv_copy(dst, src);
    if (src->kind != XV_FN) return;
    struct xfn *fn = x_alloc(sizeof *fn);
    if (!fn) { xnothing(dst); return; }
    fn->lam = src->fn->lam;
    fn->env = xt_keep_frame(src->fn->env, m);
    dst->fn = fn;
}
internal void xt_keep(struct xval *dst, const struct xval *src) {
    struct xregion *r0 = g_xr;
    struct xt_memo m = { { 0 }, { 0 }, 0 };
    g_xr = &g_xt_keep;
    xt_keep_val(dst, src, &m);
    g_xr = r0;
}
internal void xt_name(char *out, const char *given, const struct xctx *c) {   // its name: given, else its statement's start
    if (given) { lstrcpynA(out, given, 48); return; }
    const char *s = c->top ? c->top : "";
    int n = (int)strcspn(s, "\r\n");
    lstrcpynA(out, s, (n < 47 ? n : 47) + 1);
}
internal int xt_fn0(const struct xval *f) { return f && f->kind == XV_FN && f->w && f->fn->lam->n_items == 0; }   // () => ...
internal void xt_make(struct xe *e, const struct xval *r, struct xval *v) {   // trigger({ ... })
    static const char *const keys[] = { "when", "run", "loop", "for", "name" };
    if (r->kind != XV_REC || !r->w) { x_fail(e, "trigger(%.40s): a record - trigger({ when = () => ..., run = () => ... })", xtext_of(r)); return; }
    for (int i = 0; i < r->rec->n; i++) {
        int k = 0;
        while (k < 5 && strcmp(keys[k], r->rec->key[i])) k++;
        if (k == 5) { x_fail(e, "trigger({ %s = ... }): no such option - when, run, loop, for, name", r->rec->key[i]); return; }
    }
    const struct xval *when = xrec_get(r, "when"), *run = xrec_get(r, "run"), *loop = xrec_get(r, "loop"), *hold = xrec_get(r, "for"), *name = xrec_get(r, "name");
    if (!xt_fn0(when)) { x_fail(e, "trigger: when = () => a condition"); return; }
    if (!xt_fn0(run)) { x_fail(e, "trigger: run = () => what it does"); return; }
    if (hold && (!x_numeric_ok(e, hold) || !(hold->n >= 0))) { if (!e->err) x_fail(e, "trigger: for = a time, 0 or more (5s)"); return; }
    if (name && (!name->has_s || name->has_n)) { x_fail(e, "trigger: name = a text"); return; }
    if (g_xt_n == XT_MAX) { x_fail(e, "more than %d triggers", XT_MAX); return; }
    struct xtrig *t = &g_xt[g_xt_n];
    xt_keep(&t->when, when);
    xt_keep(&t->run, run);
    t->on = 1;
    t->fired = 0;
    t->loop = loop ? xtrue(loop) : 0;
    t->hold = hold ? hold->n : 0;
    t->since = -1;
    t->period = t->next = 0;
    xt_name(t->name, name ? name->s : NULL, e->c);
    xnothing(v);
    v->w = 1; v->kind = XV_TRIG; v->n = g_xt_n++;
}
// A TIMER (the host's game.timer(step, f)): a trigger of no condition that runs f every step game seconds from now +
// step, at its time (each tick, as the after's - not on the 500 ms beat); f returning stop ends it, disable(t) too.
// 0 and *why: not made
#define XT_MIN_PERIOD 0.5
internal int x_timer_make(struct xctx *c, double step, const struct xval *f, struct xval *v, const char **why) {
    if (!(step >= XT_MIN_PERIOD)) { *why = "game.timer(step, f): a step of 0.5s or more"; return 0; }
    if (!xt_fn0(f)) { *why = "game.timer(step, f): f a function of no arguments - () => ..."; return 0; }
    if (g_xt_n == XT_MAX) { *why = "too many triggers and timers"; return 0; }
    struct xtrig *t = &g_xt[g_xt_n];
    memset(t, 0, sizeof *t);
    xt_keep(&t->run, f);
    t->on = 1;
    t->since = -1;
    t->period = step;
    t->next = (g_x_hooks.now ? g_x_hooks.now() : 0) + step;
    xt_name(t->name, NULL, c);
    xnothing(v);
    v->w = 1; v->kind = XV_TRIG; v->n = g_xt_n++;
    return 1;
}
internal struct xtrig *xt_of(struct xe *e, const char *fn, const struct xval *t) {   // the trigger a value is
    if (t->kind == XV_TRIG && t->w && t->n >= 0 && t->n < g_xt_n) return &g_xt[(int)t->n];
    x_fail(e, "%s(%.40s): of a trigger (t = trigger({ ... }))", fn, xtext_of(t));
    return NULL;
}
// a trigger's or an after's function worked out (the triggers' turn): 0 a mistake - told, what it belongs to stops
internal int xt_call(const struct xval *fn, struct xval *out, const char *what, const char *who) {
    struct xctx c = { 0 };
    c.found = 1;
    c.script = 1;
    c.top = who;
    struct xe e = { &c, 0, NULL, NULL, 0, NULL, NULL, 0, 0, 0 };
    g_x_last_fail[0] = 0;
    x_call_value(&e, fn, NULL, 0, out);
    if (!e.err) return 1;
    x_error("trigger %s: %s stopped - %s", who, what, g_x_last_fail[0] ? g_x_last_fail : e.why ? e.why : "not worked out");
    return 0;
}
// the triggers' turn - every game tick (the script runner, after the scripts' own): the after's that are due, earliest
// first; every 500 ms each trigger that is on, in the order made. Their values live in a region emptied after it
internal void x_triggers_tick(double now) {
    if (!g_xt_n && !g_xa_n) return;
    struct xregion *r0 = g_xr;
    g_xr = &g_xt_tmp;
    g_xt_in = 1;
    for (;;) {   // the after's and timers due, earliest first (a timer behind: each step it missed)
        int k = -1, kt = -1;
        for (int i = 0; i < g_xa_n; i++) if (g_xa[i].at <= now + 1e-9 && (k < 0 || g_xa[i].at < g_xa[k].at)) k = i;
        for (int i = 0; i < g_xt_n; i++)
            if (g_xt[i].period > 0 && g_xt[i].on && g_xt[i].next <= now + 1e-9 && (kt < 0 || g_xt[i].next < g_xt[kt].next)) kt = i;
        if (kt >= 0 && (k < 0 || g_xt[kt].next < g_xa[k].at)) {
            struct xtrig *t = &g_xt[kt];
            struct xval v;
            t->next += t->period;
            t->fired = 1;
            if (!xt_call(&t->run, &v, "timer", t->name) || v.kind == XV_STOP) t->on = 0;
            continue;
        }
        if (k < 0) break;
        struct xafter a = g_xa[k];
        memmove(&g_xa[k], &g_xa[k + 1], (size_t)(g_xa_n - k - 1) * sizeof *g_xa);
        g_xa_n--;
        struct xval v;
        xt_call(&a.fn, &v, "after", a.name);
    }
    if (now + 1e-9 >= g_xt_next) {
        g_xt_next = now + 0.5;
        for (int i = 0; i < g_xt_n; i++) {
            struct xtrig *t = &g_xt[i];
            struct xval v;
            if (!t->on || t->period > 0) continue;   // (a timer: above)
            if (!xt_call(&t->when, &v, "when", t->name)) { t->on = 0; continue; }
            if (!xtrue(&v)) { t->since = -1; continue; }
            if (t->since < 0) t->since = now;
            if (now - t->since + 1e-9 < t->hold) continue;
            t->fired = 1;
            t->since = -1;
            if (!t->loop) t->on = 0;
            if (!xt_call(&t->run, &v, "run", t->name)) t->on = 0;
        }
    }
    g_xt_in = 0;
    x_region_reset(&g_xt_tmp);
    g_xr = r0;
}
internal void xe_script(struct xe *e, const char *name, const struct xval *a, int na, struct xval *v) {
    if (!strcmp(name, "trigger")) { xt_make(e, &a[0], v); return; }
    if (!strcmp(name, "enable") || !strcmp(name, "disable")) {
        struct xtrig *t = xt_of(e, name, &a[0]);
        if (!t) return;
        t->on = name[0] == 'e';
        t->since = -1;
        if (t->on && t->period > 0) t->next = (g_x_hooks.now ? g_x_hooks.now() : 0) + t->period;   // a timer: a step from now
        return;
    }
    if (!strcmp(name, "after")) {   // after(5s, () => ...): later, by the triggers' turn
        if (!x_numeric_ok(e, &a[0]) || !a[0].w || !(a[0].n >= 0)) { if (!e->err) x_fail(e, "after(time, f): a time, 0 or more"); return; }
        if (!xt_fn0(&a[1])) { x_fail(e, "after(time, f): f a function of no arguments - () => ..."); return; }
        if (g_xa_n == XA_MAX) { x_fail(e, "more than %d after's waiting", XA_MAX); return; }
        struct xafter *q = &g_xa[g_xa_n++];
        q->at = (g_x_hooks.now ? g_x_hooks.now() : 0) + a[0].n;
        xt_keep(&q->fn, &a[1]);
        xt_name(q->name, NULL, e->c);
        return;
    }
    if (!strcmp(name, "cmd")) {
        if (!a[0].has_s || a[0].has_n) { x_fail(e, "cmd(text): a command as a text"); return; }
        x_command(e, a[0].s, v);
        return;
    }
    if (!strcmp(name, "player")) {
        if (!x_numeric_ok(e, &a[0])) return;
        if (!g_x_host.object((int)xto_num(&a[0]), v)) x_fail(e, "player(%g): no such player", xto_num(&a[0]));
        return;
    }
    int fi = !strcmp(name, "every") ? 3 : -1;   // the function's place
    for (int i = 0; i < na; i++) {
        if (i == fi) { if (a[i].kind != XV_FN) { x_fail(e, "%s(...): its last argument is a function - (i) => ...", name); return; } }
        else if (!x_numeric_ok(e, &a[i]) || !a[i].w) { if (!e->err) x_fail(e, "%s(...): a number where nothing is", name); return; }
    }
    double now = g_x_hooks.now ? g_x_hooks.now() : 0;
    if (!strcmp(name, "wait")) { x_wait_until(e, now + a[0].n); return; }
    if (!strcmp(name, "wait_until")) { if (a[0].n > now) x_wait_until(e, a[0].n); return; }   // (past: no wait)
    // every(step, from, until, f): f(run) at from, from + step ... until (in); stop ends it
    if (!(a[0].n > 0)) { x_fail(e, "every(%g, ...): the step must be above 0", a[0].n); return; }
    for (int k = 1; !e->err; k++) {
        double t = a[1].n + (k - 1) * a[0].n;
        if (t > a[2].n + 1e-9) break;
        if (t > (g_x_hooks.now ? g_x_hooks.now() : 0) && !x_wait_until(e, t)) return;
        struct xval kv, r;
        xnum(&kv, k);
        x_call_value(e, &a[3], &kv, 1, &r);
        if (!e->err && r.kind == XV_STOP) break;
    }
}
internal void xe_call(struct xe *e, const struct xnode *n, struct xval *v) {
    const char *name = n->s;
    struct xval a[32];   // the arguments in order: a command, a script's function all (up to 32); the others the first 8
    int nall = 0;
    for (int i = 0; i < n->n_items; i++) {
        struct xval tmp;
        xe(e, n->items[i], nall < 32 ? &a[nall] : &tmp);
        if (nall < 32) nall++;
        if (e->err) return;
    }
    if (n->n_items > 32) { x_fail(e, "%s(...): more than 32 arguments", name); return; }
    int na = nall < 8 ? nall : 8;
    if (!strchr(name, '.'))   // a function in a variable or parameter
        for (int i = 0; i < e->c->fnp; i++)
            if (!strcmp(e->c->fpn[i], name)) {
                if (e->c->fpv[i].kind != XV_FN) { x_fail(e, "%s is no function", name); return; }
                struct xval fv = e->c->fpv[i];
                x_call_value(e, &fv, a, nall, v);
                return;
            }
    int f = x_fn_id(name, na);
    for (int i = 0; f && i < na; i++) if (a[i].miss[0]) x_use(e, &a[i]);   // a function's arguments (a relation checks its players) - each told
    if (e->err) return;
    if (e->c->fixed && (f == 1 || f == 5 || !f)) {   // a setting: fixed - no draws, no vars functions
        x_fail(e, "%s(): a setting is a fixed number (no chance(), rnd() or names)", name);
        return;
    }
    if (f == 1 && a[0].has_n && a[0].n > 1) {   // chance(70): meant 70%
        x_fail(e, "chance(%g): a probability is 0 to 1 - chance(70%%) or chance(0.7)", a[0].n);
        return;
    }
    if (f && (f <= 6 || f == 14))   // the number functions: numbers (or nothing) only
        for (int i = 0; i < na; i++) if (!x_numeric_ok(e, &a[i])) return;
    if (f == 9 ? !x_numeric_ok(e, &a[1]) : f == 11 && !x_numeric_ok(e, &a[0])) return;
    if (f == 14 && !(xto_num(&a[0]) > 0)) { x_fail(e, "log2(%g): only of a number above 0", xto_num(&a[0])); return; }
    if (f == 11)
        for (int i = 0; i < xl_n(&a[1]); i++) if (!x_numeric_ok(e, xl_at(&a[1], i))) return;
    if (f == 16 || f == 17) {   // vec(x, y [, z [, w]]), rgb(r, g, b): numbers, each there
        double c[4];
        for (int i = 0; i < na; i++) {
            if (!x_numeric_ok(e, &a[i])) return;
            if (!a[i].w) { x_fail(e, "%s(...): a number where nothing is", name); return; }
            c[i] = a[i].n;
            if (f == 17 && !(c[i] >= 0 && c[i] <= 255)) { x_fail(e, "rgb(%g, %g, %g): each 0 to 255 - vec(r, g, b) takes 0 to 1", a[0].n, a[1].n, a[2].n); return; }
            if (f == 17) c[i] /= 255;
        }
        xvec(v, c, na);
        return;
    }
    if (f == 19) {   // has(r, "name"): a record with that member (r.name would be a mistake without it)
        if (a[0].kind != XV_REC || !a[0].w) { x_fail(e, "has(%.40s, ...): of a record", xtext_of(&a[0])); return; }
        if (!a[1].has_s || a[1].has_n) { x_fail(e, "has(r, %.40s): a member's name as a text - has(r, \"name\")", xtext_of(&a[1])); return; }
        xnum(v, xrec_get(&a[0], a[1].s) != NULL);
        return;
    }
    if (f == 18) {   // distance(a, b): between two vectors of as many numbers
        if (a[0].kind != XV_VEC || a[1].kind != XV_VEC || a[0].vn != a[1].vn) {
            x_fail(e, "distance(%.30s, %.30s): of two vectors of as many numbers", xtext_of(&a[0]), xtext_of(&a[1]));
            return;
        }
        double s = 0;
        for (int i = 0; i < a[0].vn; i++) s += (a[0].vc[i] - a[1].vc[i]) * (a[0].vc[i] - a[1].vc[i]);
        xnum(v, sqrt(s));
        return;
    }
    if (!f && !strchr(name, '.')) {   // a function of vars: name(a, b) = ...
        const char *par = NULL, *def = NULL;
        for (int k = 0; k < 3 && !def; k++) def = xvars_fn(e->c->vars[k], name, &par);
        int want = def ? x_par_count(par) : x_fn_known(name);
        if (want >= 0) {
            static char why[80];
            if (want != na) { snprintf(why, sizeof why, "%s takes %d argument%s", name, want, want == 1 ? "" : "s"); e->err = 1; e->why = why; return; }
            if (!def) { snprintf(why, sizeof why, "%s: a function of another file's vars", name); e->err = 1; e->why = why; return; }
            x_call(e, def, par, a, na, v);
            return;
        }
        if (e->c->script) {   // a script's own functions and the pipe's commands
            int sf = x_script_fn(name);
            if (sf >= 0) {
                if (sf != nall) { x_fail(e, "%s takes %d argument%s", name, sf, sf == 1 ? "" : "s"); return; }
                xe_script(e, name, a, nall, v);
                return;
            }
            if (g_x_host.command(name)) {
                char cmd[512], t[256];
                int o = snprintf(cmd, sizeof cmd, "%s", name);
                for (int i = 0; i < nall; i++) {
                    if (!x_cmd_arg(e, name, &a[i], t, sizeof t)) return;
                    o += snprintf(cmd + o, sizeof cmd - (size_t)o, " %s", t);
                    if (o >= (int)sizeof cmd - 1) { x_fail(e, "%s(...): a command over %d characters", name, (int)sizeof cmd - 1); return; }
                }
                x_command(e, cmd, v);
                return;
            }
        }
    }
    if (!f) {   // a relation: them.opinion(me) - the host's own way; p.method(...) of a variable that is an object
        if (e->c->nogame) { x_nogame(e, name); return; }
        if (g_x_host.method(e->c, name, a, na, v)) return;
        const char *dot = strrchr(name, '.');
        size_t hn = strcspn(name, ".");
        int own = 0;
        for (int i = 0; dot && i < e->c->fnp && !own; i++) own = strlen(e->c->fpn[i]) == hn && !strncmp(e->c->fpn[i], name, hn);
        if (own && dot - name < 120) {
            char path[128];
            struct xval o;
            lstrcpynA(path, name, (int)(dot - name) + 1);
            xe_ident(e, path, &o);
            if (e->err) return;
            if (o.obj && o.w) { x_obj_member(e, &o, dot + 1, a, nall, 1, v, name); return; }
            if (!o.w && o.miss[0]) { x_use(e, &o); return; }
            x_fail(e, "%s(...): %.40s is no object - a method is a player's, the game's ...", name, xtext_of(&o));
            return;
        }
        e->err = 1; e->why = "an unknown function or relation (or the wrong number of arguments)";
        return;
    }
    if (f == 1) xnum(v, x_rnd01(e->c) < xto_num(&a[0]));   // a probability: chance(70%), chance(0.7)
    else if (f == 2) {
        int mx = !lstrcmpiA(name, "max");
        double r = xto_num(&a[0]);
        for (int i = 1; i < na; i++) { double x = xto_num(&a[i]); if (mx ? x > r : x < r) r = x; }
        xnum(v, r);
    } else if (f == 3) { double x = xto_num(&a[0]); xnum(v, x < 0 ? -x : x); }
    else if (f == 4) { double x = xto_num(&a[0]); xnum(v, (double)(long long)(x < 0 ? x - 0.5 : x + 0.5)); }
    else if (f == 6) { double x = xto_num(&a[0]), r = (double)(long long)x; xnum(v, r > x ? r - 1 : r); }   // down: floor(-1.5) -2
    else if (f == 15) { *v = a[0]; x_pick(e->c, v); }   // pick(l): one of its items at random (a value: itself)
    else if (f == 12 || f == 13) {   // str(x): its text only (epoch: "Copper Age"); number(x): its number - a text
        double x;                        // that is one ("12"), else nothing (number(t) ?? 0)
        if (a[0].ln) x_fail(e, "%s(%.40s): a list - pick(l) for one of its items", name, xtext_of(&a[0]));
        else if (!a[0].w) ;
        else if (f == 12) xstr(v, xtext_of(&a[0]));
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
        if (i >= 0 && i < xl_n(l)) { *v = *xl_at(l, i); v->w = 1; v->ln = 0; v->it = NULL; }
    } else if (f == 10) {   // contains(l, x): one of its items equal to x (as ==)
        int hit = 0;
        for (int i = 0; i < xl_n(&a[0]) && !hit; i++) hit = a[1].w && !xcmp(xl_at(&a[0], i), &a[1]);
        xnum(v, hit);
    }
    else if (f == 14) xnum(v, det_log2f((float)xto_num(&a[0])));   // the same bits on every PC (base/math.c)
    else xnum(v, x_rnd01(e->c) * (na ? xto_num(&a[0]) : 1));   // rnd(n): 0 <= x < n, rnd(): 0 <= x < 1
}
// a list: its items, a list in it its items in its place; a mistake in an item (told) leaves it out, so does
// nothing ([a ? "x", "y"]); none left: nothing. It holds no one of them - one at random is what a consumer that
// wants one value takes (x_pick: a line, a {placeholder}, pick(l)); used whole by len, first, item, contains,
// map_range and the filters (count, kills)
internal void xe_list(struct xe *e, const struct xnode *n, struct xval *v) {
    if (e->c->fixed) { x_fail(e, "a list - a setting is one number"); return; }
    int cnt = 0, cap = 0;
    struct xval *items = NULL;
    for (int i = 0; i < n->n_items; i++) {
        struct xval it;
        int f0 = e->c->failed;
        xe(e, n->items[i], &it);
        if (!e->err) x_use(e, &it);
        if (e->err && e->c->failed > f0 && !e->c->circle) { e->err = 0; e->why = NULL; xnothing(&it); }   // left out
        if (e->err) { free(items); return; }
        if (!it.w) continue;
        int add = it.ln ? it.ln : 1;
        if (cnt + add > cap) {
            cap = (cnt + add) * 2;
            struct xval *m = realloc(items, (size_t)cap * sizeof *m);
            if (!m) { free(items); x_fail(e, "no memory for a list"); return; }
            items = m;
        }
        for (int k = 0; k < add; k++) items[cnt++] = it.ln ? it.it[k] : it;
    }
    xnothing(v);
    if (cnt) {
        struct xval *keep = x_alloc((size_t)cnt * sizeof *keep);
        if (!keep) { free(items); x_fail(e, "no memory for a list"); return; }
        memcpy(keep, items, (size_t)cnt * sizeof *keep);
        v->it = keep; v->ln = v->w = cnt;
    }
    free(items);
}
internal void xe_str(struct xe *e, const struct xnode *n, struct xval *v) {   // a string: its placeholders filled
    int f0 = e->c->failed;
    size_t cap = strlen(n->s) * 2 + 4096;
    char *f = malloc(cap);
    if (!f) { e->err = 1; return; }
    x_fill(f, (int)cap, n->s, e->c);
    if (e->c->failed > f0) { free(f); e->err = 1; e->why = "a mistake while working it out"; return; }   // told in x_fill
    xstr(v, f);
    free(f);
}
// a variable set: the running function's own (new, or again), or one of the code around it in the frame it is its
// own in - kept there (a frame that stays: in g_xt_keep; one that goes on: moved below when this function returns)
internal void x_assign(struct xe *e, const char *name, const struct xval *v) {
    struct xframe *f = e->f;
    if (!f) { x_fail(e, "%s = ...: a variable only in a script or a function", name); return; }
    int i = 0;
    while (i < f->n && strcmp(f->name[i], name)) i++;
    if (i < f->n && !f->own[i]) {
        int j;
        struct xframe *o = x_frame_owner(f->up, name, &j);
        if (!o) { x_fail(e, "%s = ...: its frame is gone", name); return; }
        if (xv_made_in(v, f)) { x_fail(e, "%s = ...: a function made inside a function can't be handed out of it", name); return; }
        if (o->keep) {
            if (!v->has_s && !v->ln && !v->kind && !v->obj) { o->val[j] = *v; o->val[j].s = ""; }   // a number: nothing to keep
            else xt_keep(&o->val[j], v);
        } else {
            int k = g_xdirty_base;
            while (k < g_xdirty_n && !(g_xdirty[k].f == o && g_xdirty[k].i == j)) k++;
            if (k == g_xdirty_n) {
                if (k == XDIRTY_MAX) { x_fail(e, "more than %d variables of the code around set", XDIRTY_MAX); return; }
                g_xdirty[k].f = o; g_xdirty[k].i = j; g_xdirty_n++;
            }
            o->val[j] = *v;
        }
        f->val[i] = o->val[j];
        return;
    }
    if (i == f->n) { if (f->n == XF_MAX) { x_fail(e, "more than %d names in a function", XF_MAX); return; } f->n++; }
    f->name[i] = name; f->val[i] = *v; f->own[i] = 1;
    e->c->fpn = f->name; e->c->fpv = f->val; e->c->fnp = f->n;
}
internal int x_loop_round(struct xe *e, const struct xnode *s) {   // a loop's next round: 0 the loop ends
    if (e->brk == 1) { e->brk = 0; return 0; }
    e->brk = 0;
    if (e->err || e->ret) return 0;
    if (++e->steps > X_LOOP_STEPS) {
        x_fail(e, "a loop (line %d) ran %d rounds without waiting - wait(...) lets the game go on", s->line, X_LOOP_STEPS);
        return 0;
    }
    return 1;
}
internal void x_exec(struct xe *e, const struct xnode *blk);
internal void x_stmt(struct xe *e, const struct xnode *s) {
    struct xval v;
    switch (s->k) {
    case XN_LET: case XN_ASSIGN: xe(e, s->a, &v); if (!e->err) x_assign(e, s->s, &v); break;
    case XN_RETURN:
        if (s->a) xe(e, s->a, e->retv ? e->retv : &v);
        else if (e->retv) xnothing(e->retv);
        e->ret = 1;
        break;
    case XN_BREAK: e->brk = 1; break;
    case XN_CONTINUE: e->brk = 2; break;
    case XN_IF:
        for (; s; s = s->b && s->b->k == XN_IF ? s->b : NULL) {   // else if ...: the next if
            xe(e, s->c, &v);
            if (e->err) return;
            if (xtrue(&v)) { x_exec(e, s->a); return; }
            if (s->b && s->b->k != XN_IF) { x_exec(e, s->b); return; }
        }
        break;
    case XN_WHILE:
        do {
            xe(e, s->c, &v);
            if (e->err || !xtrue(&v)) break;
            x_exec(e, s->a);
        } while (x_loop_round(e, s));
        break;
    case XN_FOR: {   // for i in a..b: every whole number from a to b, both in; for x in l: each of l's items
        struct xval a, b;
        xe(e, s->a, &a);
        if (e->err) return;
        if (s->b) {
            xe(e, s->b, &b);
            if (e->err || !x_numeric_ok(e, &a) || !x_numeric_ok(e, &b)) return;
            if (!a.w || !b.w) { x_fail(e, "for %s in a..b: a number where nothing is", s->s); return; }
            for (double i = a.n; i <= b.n + 1e-9; i += 1) {
                xnum(&v, i);
                x_assign(e, s->s, &v);
                if (!e->err) x_exec(e, s->c);
                if (!x_loop_round(e, s)) break;
            }
            return;
        }
        if (a.kind) { x_fail(e, "for %s in ...: %s - a list, or a..b", s->s, a.kind == XV_FN ? "a function" : a.kind == XV_REC ? "a record" : a.kind == XV_VEC ? "a vector" : "stop"); return; }
        for (int i = 0; i < xl_n(&a); i++) {
            x_assign(e, s->s, xl_at(&a, i));
            if (!e->err) x_exec(e, s->c);
            if (!x_loop_round(e, s)) break;
        }
        break;
    }
    default: xe(e, s, &v); break;
    }
}
// a block's statements in order; a mistake ends it (err_at: the statement), a return, a break or continue too
internal void x_exec(struct xe *e, const struct xnode *blk) {
    struct xctx *c = e->c;
    const char *top = c->top;
    for (int i = 0; i < blk->n_items && !e->err && !e->ret && !e->brk; i++) {
        const struct xnode *s = blk->items[i];
        c->top = s->at;   // the mistakes' messages show the statement
        x_stmt(e, s);
        if (e->err && !e->err_at) e->err_at = s;
    }
    c->top = top;
}
// a op b with a vector among them (into a): + - two vectors of as many numbers, number by number; * a vector and a
// number (either side), / a vector by a number - anything else a mistake, nothing guessed
internal void xe_vec_op(struct xe *e, int op, struct xval *a, const struct xval *b) {
    const char *ops = op == '+' ? "+" : op == '-' ? "-" : op == '*' ? "*" : "/";
    double c[4];
    if (op == '+' || op == '-') {
        if (a->kind != XV_VEC || b->kind != XV_VEC || a->vn != b->vn) {
            x_fail(e, "%.40s %s %.40s: + and - take two vectors of as many numbers", xtext_of(a), ops, xtext_of(b));
            return;
        }
        for (int i = 0; i < a->vn; i++) c[i] = op == '+' ? a->vc[i] + b->vc[i] : a->vc[i] - b->vc[i];
        xvec(a, c, a->vn);
        return;
    }
    const struct xval *vv = a->kind == XV_VEC ? a : b, *k = vv == a ? b : a;
    if (k->kind == XV_VEC || (op == '/' && vv != a)) {
        x_fail(e, "%.40s %s %.40s: * takes a vector and a number, / a vector by a number", xtext_of(a), ops, xtext_of(b));
        return;
    }
    if (!x_numeric_ok(e, k)) return;
    double y = xto_num(k);
    if (op == '/' && y == 0) { x_fail(e, "a division by 0"); return; }
    for (int i = 0; i < vv->vn; i++) c[i] = op == '*' ? vv->vc[i] * y : vv->vc[i] / y;
    xvec(a, c, vv->vn);
}
internal void xe(struct xe *e, const struct xnode *n, struct xval *v) {
    struct xctx *c = e->c;
    xnothing(v);
    c->level += n->lvl;
    switch (n->k) {
    case XN_NUM: xnum(v, n->n); break;
    case XN_STR: xe_str(e, n, v); break;
    case XN_NAME: {
        if (n->cs) { xdual_kept(v, n->n, n->cs); break; }   // a constant that stays, looked up before
        int kept = e->kept;   // a name in a name (a vars entry's): its own
        e->kept = 0;
        xe_ident(e, n->s, v);
        // a constant the host says stays the same (kind.tank, unit.citizen): kept in the node (filters must be quick)
        if (!e->err && e->kept && v->has_s && v->has_n && v->n >= 0) {
            ((struct xnode *)n)->n = v->n;
            ((struct xnode *)n)->cs = v->s;
        }
        e->kept = kept;
        if (c->missed) { c->missed = 0; e->err = 1; e->why = "a mistake while working it out"; }   // told (x_missing)
        break;
    }
    case XN_CALL:
        xe_call(e, n, v);
        if (c->missed) { c->missed = 0; e->err = 1; e->why = "a mistake while working it out"; }
        break;
    case XN_LIST: xe_list(e, n, v); break;
    case XN_MEMBER:   // f(a).name, (a + b).x: a record's member, a vector's number, an object's
        xe(e, n->a, v);
        if (e->err) break;
        x_use(e, v);
        if (!e->err) x_member_of(e, n->s, v, NULL);
        break;
    case XN_INDEX: {   // l[i]: its item i, from 1 - none there a mistake
        struct xval l, i;
        xe(e, n->a, &l);
        if (!e->err) x_use(e, &l);
        if (!e->err) xe(e, n->b, &i);
        if (e->err) break;
        if (l.w && !l.ln) { x_fail(e, "[...]: %.40s is no list", l.kind == XV_REC ? "a record" : l.kind == XV_VEC ? "a vector" : xtext_of(&l)); break; }
        if (!x_numeric_ok(e, &i)) break;
        int k = (int)i.n;
        if (!i.w || (double)k != i.n || k < 1 || k > xl_n(&l)) { x_fail(e, "[%s]: no such item - the list has %d (the first is [1])", xtext_of(&i), xl_n(&l)); break; }
        *v = *xl_at(&l, k - 1);
        v->w = 1; v->ln = 0; v->it = NULL;
        break;
    }
    case XN_MCALL: {   // f(a).name(...), l[1].name(...): an object's method
        struct xval o, a[8];
        char whole[48];
        snprintf(whole, sizeof whole, ".%s", n->s);
        xe(e, n->a, &o);
        if (!e->err) x_use(e, &o);
        if (e->err) break;
        if (n->n_items > 8) { x_fail(e, "%s(...): more than 8 arguments", whole); break; }
        for (int i = 0; i < n->n_items && !e->err; i++) xe(e, n->items[i], &a[i]);
        if (e->err) break;
        if (!o.obj || !o.w) { x_fail(e, "%s(...): %.40s is no object - a method is a player's, the game's ...", whole, xtext_of(&o)); break; }
        x_obj_member(e, &o, n->s, a, n->n_items, 1, v, whole);
        break;
    }
    case XN_REC: {   // a record: each member worked out now, in order - a mistake in one fails it whole
        struct xrec *r = x_alloc(sizeof *r);
        const char **key = x_alloc((size_t)n->n_items * sizeof *key + 1);
        struct xval *val = x_alloc((size_t)n->n_items * sizeof *val + 1);
        if (!r || !key || !val) { e->err = 1; break; }
        r->n = n->n_items; r->key = key; r->val = val;
        for (int i = 0; i < n->n_items && !e->err; i++) {
            key[i] = n->items[i]->s;   // (the tree's: it stays)
            xe(e, n->items[i]->a, &val[i]);
            if (!e->err) x_use(e, &val[i]);
        }
        if (e->err) break;
        xnothing(v);
        v->w = 1; v->kind = XV_REC; v->rec = r;
        break;
    }
    case XN_LAMBDA: {   // a function: a value (with where it was made: the names it reads)
        struct xfn *fn = x_alloc(sizeof *fn);
        if (!fn) { e->err = 1; break; }
        fn->lam = n;
        fn->env = e->f;
        v->w = 1;
        v->kind = XV_FN;
        v->fn = fn;
        break;
    }
    case XN_NEG: case XN_NOT:
        xe(e, n->a, v);
        if (e->err) break;
        if (n->k == XN_NEG) {
            x_use(e, v);
            if (e->err) break;
            if (v->kind == XV_VEC) { double neg[4]; for (int i = 0; i < v->vn; i++) neg[i] = -v->vc[i]; xvec(v, neg, v->vn); break; }
            if (!x_numeric_ok(e, v)) break;
            xnum(v, -xto_num(v));
        } else xnum(v, !xtrue(v));
        break;
    case XN_MUL: case XN_ADD: {
        struct xval b;
        xe(e, n->a, v);
        if (e->err) break;
        xe(e, n->b, &b);
        if (e->err) break;
        x_use(e, v);
        if (!e->err) x_use(e, &b);
        if (e->err) break;
        if (v->kind == XV_VEC || b.kind == XV_VEC) { xe_vec_op(e, n->op, v, &b); break; }
        double x, y;
        if (n->k == XN_MUL || n->op == '-') {
            if (!x_numeric_ok(e, v) || !x_numeric_ok(e, &b)) break;
            x = xto_num(v); y = xto_num(&b);
            if (n->op == '/' && y == 0) { x_fail(e, "a division by 0"); break; }
            xnum(v, n->op == '*' ? x * y : n->op == '/' ? x / y : x - y);
        }
        // + adds numbers only: a text is built in its string ("Age: {epoch}", "{a}{b}") - "a" + b could mean b
        // added to the text or the text to b's items; nothing counts as 0
        else if ((v->has_s && !v->has_n) || (b.has_s && !b.has_n))
            x_fail(e, "a text (\"%.40s\") added - + adds numbers, a text is built in its string: \"Age: {epoch}\", \"{a}{b}\"", xtext_of(v->has_s && !v->has_n ? v : &b));
        else if (!x_numeric_ok(e, v) || !x_numeric_ok(e, &b)) break;
        else xnum(v, xto_num(v) + xto_num(&b));   // an epoch is a number too: epoch + 1
        break;
    }
    case XN_CMP: {
        struct xval b;
        xe(e, n->a, v);
        if (e->err) break;
        xe(e, n->b, &b);
        if (e->err) break;
        if (v->ln || b.ln) {   // [1, 2] == 1: no one of them - contains(l, x)
            x_fail(e, "a list (%.40s) compared - contains(l, x) for one of its items", xtext_of(v->ln ? v : &b));
            break;
        }
        if ((v->kind == XV_REC && v->w) || (b.kind == XV_REC && b.w)) {   // no order, no whole equality: its members
            x_fail(e, "a record (%.40s) compared - compare its members (r.name == ...)", xtext_of(v->kind == XV_REC ? v : &b));
            break;
        }
        if (!v->w || !b.w) { xnum(v, 0); break; }   // nothing compares with nothing: false
        if (v->kind == XV_VEC || b.kind == XV_VEC) {   // vectors: equal or not - no order, nothing else alike
            if (v->kind != b.kind || (n->op != '=' && n->op != '!')) {
                x_fail(e, "%.40s %s %.40s: a vector is only == or != another vector", xtext_of(v), n->op == 'l' ? "<=" : n->op == 'g' ? ">=" : n->op == '=' ? "==" : n->op == '!' ? "!=" : n->op == '<' ? "<" : ">", xtext_of(&b));
                break;
            }
            xnum(v, xvec_eq(v, &b) == (n->op == '='));
            break;
        }
        if (!(v->obj && b.obj) && xmismatch(v, &b)) {
            x_fail(e, "a text (\"%.40s\") compared with a number - a number has no quotes, number(t) makes one", xtext_of(v->has_s ? v : &b));
            break;
        }
        int d = xcmp(v, &b), op = n->op;
        xnum(v, op == '<' ? d < 0 : op == '>' ? d > 0 : op == 'l' ? d <= 0 : op == 'g' ? d >= 0 : op == '=' ? d == 0 : d != 0);
        break;
    }
    case XN_AND: case XN_OR: {
        xe(e, n->a, v);
        if (e->err) break;
        int t = xtrue(v);
        if (n->k == XN_AND ? !t : t) { xnum(v, t); break; }   // the right side not worked out
        struct xval b;
        xe(e, n->b, &b);
        if (e->err) break;
        xnum(v, xtrue(&b));
        break;
    }
    case XN_COAL:
        xe(e, n->a, v);
        if (e->err || v->w > 0) break;
        xe(e, n->b, v);
        break;
    case XN_COND: {
        xe(e, n->c, v);
        if (e->err) break;
        int t = xtrue(v);
        if (t) xe(e, n->a, v);
        else if (n->b) xe(e, n->b, v);
        else xnothing(v);
        break;
    }
    default:   // a block or a statement where a value belongs (the parser makes none)
        e->err = 1;
        break;
    }
    c->level -= n->lvl;
}
// evaluate the whole of src (an expression and nothing after it); 0: not an expression
internal void x_log_bad(const char *what, const char *src) { x_error("%s: %.120s", what, src); }
internal int x_eval(const char *src, struct xctx *c, struct xval *v) {   // its value ends up in a text: a line, a {placeholder}
    int top = !c->top;
    if (top) c->top = src;
    if (!g_xr->busy++) x_region_reset(g_xr);   // the outermost: the last one's values are done with
    const struct xunit *u = x_compiled(src);
    struct xe e = { c, 0, NULL, NULL, 0, NULL, NULL, 0, 0, 0 };
    xnothing(v);
    int level = c->level;   // its own levels (X_NEST)
    c->level = 0;
    if (!u->ok) e.err = 1;
    else xe(&e, u->root, v);
    c->level = level;
    g_xr->busy--;
    int ok = !e.err && !c->circle;
    if (ok) { x_use(&e, v); ok = !e.err; }   // a player who isn't there, written in the text
    if (top) c->top = NULL;
    if (c->circle && !c->depth) { c->circle = 0; x_log_bad("vars naming each other in a circle", src); }
    return ok;   // 0 and c->failed up: a mistake while working it out, told
}

// an ini file's number (ini.c): a constant expression - numbers, %, + - * / ( ), min, max, abs, round, floor, log2;
// 0: a mistake, *why (not logged here)
internal int x_const_eval(const char *src, double *out, const char **why) {
    struct xctx c;
    memset(&c, 0, sizeof c);
    c.found = 1; c.nogame = 1; c.fixed = 1;
    struct xval v;
    g_x_last_fail[0] = 0;
    if (!x_eval(src, &c, &v)) { *why = g_x_last_fail[0] ? g_x_last_fail : "not a number or an expression"; return 0; }
    if (!v.w || !v.has_n || v.has_s || v.obj || v.kind) { *why = "a number expected"; return 0; }
    *out = v.n;
    return 1;
}
// the syntax of an expression, and the checks a file gets when read (x_check); nothing worked out (no game needed);
// 0: a mistake, *why what it is when known
internal int x_syntax(const char *src, const char **why) {
    struct xctx c = { 0 };
    c.found = 1;   // no players looked up
    const struct xunit *u = x_compiled(src);
    if (!u->ok) { *why = u->why ? u->why : "not an expression"; return 0; }
    struct xcheck k = { &c, 0, NULL, 0, "", 0 };
    struct xscope sc = { NULL, { 0 }, 0, { 0 }, 0, { 0 } };
    if (!x_check(&k, u->root, &sc)) { *why = k.why ? k.why : "not an expression"; return 0; }
    return 1;
}

internal const char *x_close(const char *s);
// A placeholder's FORMAT: {x:%} {x:.1%} {x:.2} - after a ':' that belongs to no '?' (outside strings and brackets: the
// ternary's own colons pair with its ?s), how its number is written: % as a share (0.5 -> 50%), .N with N decimals
// (0..6; both: .1% -> 62.5%). Without one a number is written as it is (up to 2 decimals). A number is never
// written as a share by itself: 50% in an expression is only another way to write 0.5.
// A choice in front of a format must be in ( ): {(a ? 1 : 0):%} - written {a ? 1 : 0:%} the format reads as the 0's
// only (likewise {a ?? b:%}); *choice: a ? or ?? outside brackets before the format's ':'
internal int x_fmt_at(const char *s, int n, int *choice) {   // the format's ':' in s[0..n), -1: none
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
internal int x_fmt_parse(const char *f, int *dec, int *pct) {   // "%", ".N", ".N%"; 0: no format
    while (*f == ' ') f++;
    *dec = -1; *pct = 0;
    if (*f == '.') { if (f[1] < '0' || f[1] > '6') return 0; *dec = f[1] - '0'; f += 2; }
    if (*f == '%') { *pct = 1; f++; }
    while (*f == ' ') f++;
    return !*f && (*dec >= 0 || *pct);
}
internal void x_fmt_num(double n, int dec, int pct, char *out, int cap) {
    if (pct) n *= 100;
    if (dec < 0) dec = 0;
    snprintf(out, (size_t)cap, "%.*f%s", dec, x_round_dec(n, dec), pct ? "%" : "");
    const char *d = out + (*out == '-');   // no "-0", "-0.0%": a minus only on what isn't 0 as written
    while (*d == '0' || *d == '.') d++;
    if (*out == '-' && (!*d || *d == '%')) memmove(out, out + 1, strlen(out));
}
internal const char *const k_fmt_why = "a placeholder's format is %, .1%, .2 ... ({share:%}, {x:.1})";
internal const char *const k_fmt_choice = "a choice before a format goes in ( ): {(a ? 1 : 0):%}";
// a text's {placeholders} that are expressions (not a plain name): their syntax - and a plain name's (x_name_check).
// k, sc: the check of the code the string stands in (x_check) - a placeholder is checked with the names it sees there,
// a script's variables and functions among them ("{row(3)}"), and in a script a plain name like any other; NULL: a
// file's string value, on its own
internal int x_fills_ok(const char *text, const char **why, struct xcheck *k, struct xscope *sc) {
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
        int expr = strpbrk(src, " <>=?\"'(+[!&|") != NULL;
        int name = !expr && n && strspn(src, "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_.") == (size_t)n && !isdigit((unsigned char)*src);
        if ((expr || name) && k && (expr || k->script)) {
            const struct xunit *u = x_compiled(src);
            if (!u->ok) { *why = u->why ? u->why : "not an expression"; return 0; }
            if (!x_check(k, u->root, sc)) { *why = k->why ? k->why : "not an expression"; return 0; }
        }
        else if (expr) { if (!x_syntax(src, why)) return 0; }
        else if (name) x_name_check(src);
        s = close;
    }
    return 1;
}
// the end of a {placeholder} that starts at s ('{'): quotes and nested braces skipped; NULL: none
internal const char *x_close(const char *s) {
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
internal int x_fill(char *out, int cap, const char *tmpl, struct xctx *c) {
    int o = 0, top = !c->top;
    if (top) c->top = tmpl;
    for (const char *s = tmpl; *s && o < cap - 1;) {
        const char *close = *s == '{' ? x_close(s) : NULL;
        if (!close) { char ch = *s++; out[o++] = ch == X_LBRACE ? '{' : ch == X_RBRACE ? '}' : ch; continue; }
        char src[400], val[64];
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
            if (!rep && c->depth < X_NEST) {
                struct xval v;
                int f0 = c->failed;
                c->depth++;
                int ok = x_eval(src, c, &v) && !c->circle;
                c->depth--;
                if (c->circle && !c->depth) { c->circle = 0; x_log_bad("vars naming each other in a circle", src); }
                if (ok) x_pick(c, &v);   // a list said: one of its items
                if (ok && fmt && !(v.w && v.has_n && !v.obj)) {   // {name:%}: a text, a player, nothing - a mistake
                    x_error("{%s:...}: a format for a number, this is none", src);
                    c->failed++;
                    val[0] = 0; rep = val;
                }
                else if (ok && fmt) { x_fmt_num(v.n, dec, pct, val, sizeof val); rep = val; }
                else if (ok) rep = xtext_of(&v);
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
internal int x_plain_strings(const char *src, void (*fn)(const char *text, void *arg), void *arg) {
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
internal void tl_error(struct tlex *l, const char *what) {
    x_error("%s:%d: %s - skipped", l->file, l->line, what);
    l->err = 1;
}
internal void tl_blank(struct tlex *l, int lines) {   // spaces; with lines: line breaks and comments too
    for (;;) {
        char c = *l->s;
        if (c == ' ' || c == '\t' || c == '\r') l->s++;
        else if (lines && c == '\n') { l->line++; l->s++; }
        else if (lines && c == '#') while (*l->s && *l->s != '\n') l->s++;
        else return;
    }
}
internal void tval_free(struct tval *v) { free(v->text); free(v->src); memset(v, 0, sizeof *v); }
internal int tl_key_line(const char *t) {   // a line that starts a key ("name =", "name(a, b) =", not "a == b")
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
internal int x_fold(const char *r, char *t, int cap) {
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
// it: the bracket was never closed); comments left out. In a record (rec) also to a comma or the record's } outside
// brackets - neither taken (tfile_parse: the comma after each entry)
internal void tl_value(struct tlex *l, struct tval *v, int rec) {
    memset(v, 0, sizeof *v);
    v->line = l->line;
    tl_blank(l, 0);
    size_t cap = 256, o = 0;
    char *r = malloc(cap);
    int depth = 0;
    for (;;) {
        char c = *l->s;
        if (!c || (c == '\n' && !depth) || (rec && !depth && (c == ',' || c == '}'))) break;
        if (c == '\n' && tl_key_line(l->s + 1)) { l->line++; l->s++; break; }   // the next key: depth tells
        if (c == '\n' || c == '\r' || c == '\t') { if (c == '\n') l->line++; l->s++; c = ' '; }
        else if (c == '#') { while (*l->s && *l->s != '\n') l->s++; continue; }
        else if (c == ';') { tl_error(l, "; - a comment starts with #"); free(r); return; }
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
    if (v->kind == TV_EXPR ? !x_syntax(r, &why) : v->kind == TV_STRING && !x_fills_ok(v->text, &why, NULL, NULL)) {
        int cur = l->line;
        l->line = v->line;
        tl_error(l, why);
        l->line = cur;
        tval_free(v);
    }
}
// a line where reading goes on after a mistake: one that starts a key, a record's end (}) - or an old [section] / ; comment
internal int tl_stop_line(const char *t) {
    if (tl_key_line(t)) return 1;
    while (*t == ' ' || *t == '\t') t++;
    return *t == '}' || *t == '[' || *t == ';';
}
internal int tl_line_end(struct tlex *l, const char *what) {   // nothing more on the line but a comment; 0: a mistake told
    tl_blank(l, 0);
    if (*l->s == '#') while (*l->s && *l->s != '\n') l->s++;
    if (*l->s && *l->s != '\n') { tl_error(l, what); return 0; }
    return 1;
}
// after an entry of a record: its comma (taken) or the record's } (left for the reader); else a mistake, told at the
// entry's line
internal void tl_rec_sep(struct tlex *l) {
    int line = l->line;
    tl_blank(l, 1);
    if (*l->s == ',') { l->s++; return; }
    if (*l->s == '}') return;
    int cur = l->line;
    l->line = line;
    tl_error(l, "a comma after each of a record's entries: key = value,");
    l->line = cur;
}
// read a text file: fn(arg, record, key, value) for every key - record "" for a name outside the records, a record
// in a record after a dot (prompt.tools), and the declarations include("a.eeai", ...) as the key "include" with the
// list ["a.eeai", ...], register_personality() as the key "register_personality" ([]); mistakes logged with the file
// and line, their keys skipped. A record: name = { key = value, ... } - a comma after each entry (the last's may be
// left out), on one line or many, records in it
internal int tl_declaration(const char *key) { return !strcmp(key, "include") || !strcmp(key, "register_personality"); }
typedef void (*tfile_fn)(void *arg, const char *record, const char *key, struct tval *v, struct tlex *l);
#define TL_DEPTH 8
internal void tfile_parse(const char *file, const char *text, tfile_fn fn, void *arg) {
    struct tlex l = { text, file, 1, 0 };
    char record[160] = "";   // the record being read (lines; a record in it: prompt.tools)
    int record_line[TL_DEPTH], depth = 0;   // where each open record starts; how many are open
    for (;;) {
        tl_blank(&l, 1);
        if (!*l.s) {
            if (depth) { l.line = record_line[depth - 1]; tl_error(&l, "a record without its }"); }
            return;
        }
        l.err = 0;
        const char *start = l.s;
        if (*l.s == '}') {
            l.s++;
            if (!depth) tl_error(&l, "a } too many");
            else {
                char *dot = strrchr(record, '.');
                if (dot) *dot = 0;
                else record[0] = 0;
                if (--depth) tl_rec_sep(&l);   // a record in a record: an entry of it, its comma after it
                else tl_line_end(&l, "something after the record's }");
            }
        } else if (*l.s == '[') {
            tl_error(&l, "a [section]: the files have records now - lines = { greeting = ... }");
        } else if (*l.s == ';') {
            tl_error(&l, "; - a comment starts with #");
        } else {
            char key[160];
            int n = 0;
            while (*l.s && (isalnum((unsigned char)*l.s) || *l.s == '_') && n < 47) key[n++] = *l.s++;
            key[n] = 0;
            if (n && *l.s == '(' && !depth && tl_declaration(key)) {   // include("a.eeai", ...): ["a.eeai", ...]
                struct tval v;
                memset(&v, 0, sizeof v);
                v.line = l.line;
                v.kind = TV_EXPR;
                const char *b = ++l.s, *e, *why;
                while (*l.s && *l.s != ')' && *l.s != '\n')
                    if (*l.s == '"' || *l.s == '\'') { if (x_string(l.s, NULL, 0, &e, &why) < 0) break; l.s = e; }
                    else l.s++;
                if (*l.s != ')') { tl_error(&l, "a ) missing"); }
                else {
                    size_t k = (size_t)(l.s - b);
                    v.src = malloc(k + 3);
                    v.src[0] = '[';
                    memcpy(v.src + 1, b, k);
                    v.src[k + 1] = ']';
                    v.src[k + 2] = 0;
                    l.s++;
                    if (key[0] == 'r' && k) tl_error(&l, "register_personality() takes nothing");
                    else if (tl_line_end(&l, "something after the )")) fn(arg, "", key, &v, &l);
                }
                tval_free(&v);
                goto next;
            }
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
            else if (!n || *l.s != '=') tl_error(&l, "no name = value");
            else if (l.s[1 + strspn(l.s + 1, " \t")] == '{') {   // name = { - a record: its entries up to its }
                if (strchr(key, '(')) tl_error(&l, "a record has no parameters");
                else if (depth == TL_DEPTH) tl_error(&l, "records in records more than 8 deep");
                else if (strlen(record) + strlen(key) + 2 > sizeof record) tl_error(&l, "a record's path too long");
                else {
                    if (depth) lstrcatA(record, ".");
                    lstrcatA(record, key);
                    record_line[depth++] = l.line;
                    l.s = strchr(l.s, '{') + 1;
                }
            } else if (!depth && tl_declaration(key)) {
                tl_error(&l, key[0] == 'i' ? "include(\"pack_base.eeai\") - a call, not a name" : "register_personality() - a call, not a name");
            } else {
                l.s++;
                struct tval v;
                g_x_check_par = strchr(key, '(') ? strchr(key, '(') + 1 : NULL;
                g_x_check_file = file;
                g_x_check_line = l.line;
                g_x_check_ph = g_x_host.placeholders(file, record, key);
                tl_value(&l, &v, depth > 0);
                g_x_check_par = NULL;
                g_x_check_file = NULL;
                if (!l.err) {
                    fn(arg, record, key, &v, &l);
                    l.err = 0;   // a mistake the reader's user told (an unknown key): the value is read whole - on after it
                }
                tval_free(&v);
                if (!l.err && depth) tl_rec_sep(&l);
            }
        }
    next:
        if (l.err) {   // on to the next line that starts a key or ends a record (past the rest of a broken value)
            const char *b = l.s;
            while (b > text && (b[-1] == ' ' || b[-1] == '\t')) b--;
            if (l.s != start && (b == text || b[-1] == '\n') && tl_stop_line(b)) continue;   // already there (and on)
            for (;;) {
                while (*l.s && *l.s != '\n') l.s++;
                if (!*l.s) break;
                l.s++;
                l.line++;
                if (tl_stop_line(l.s)) break;
            }
        }
    }
}
