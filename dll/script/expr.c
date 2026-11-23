// THE TEXT FILES (<game>\ageofai\personalities\*.eeai: the personalities; mod\*.eeai: computer, prompt, system) and the EXPRESSIONS in them.
// A file: "name = value" lines - a name the file's lines use (or, for a few names, what the file is about), records
// "lines = { key = value, ... }" (a comma after each entry; the keys the code reads - another record is the file's
// own name, its entries name.key), include("pack_base.eeai"),
// register_personality() - and comments from // to the line's end (outside a string). A value is an expression - over
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
// < <= > >= == !=, && || ! (the words and, or, not are a mistake), a ? b : c, a ? b, a ?? b, the functions
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
// the name it is in (row(3)), its parameters and the names of the code around where it was made visible (set there
// too: slots and views, SCRIPTS COMPILED); what it hands back is never a function made in it. Statements (a block's, a script's - one a line): name = value, return [value], an expression. The text
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
// THE ENGINE: an expression is parsed once into a tree (x_parse; cached by its text, x_compiled), the tree
// once into instructions (x_unit_code) and those are run by a machine (SCRIPTS COMPILED: the scripts each on their
// own, the texts all on one - xvm_eval); the file check (x_syntax) is the same parser with the checks the files get when they are read, in
// the order they appear. The values' texts and lists live in a region (x_alloc) that the outermost evaluation starts
// afresh. Included by eeaimod.c before persona.c.
// a file's names (and its records' members) - mod/system.eeai's hold every word of language.eeai (word.vassal: 51 in
// 2026-11), so well above them; at most 254 (first, next: an index + 1 in a byte)
#define XVARS_MAX 128
#define XVAL_CAP 512   // the callers' text buffers (xto_text cuts there); a value's own text has no limit
// val: an expression's source; dotted: a file record's member among the keys (colors.dark) - else no name is
// looked up as one (x_vars_group, a member path's head: what every dotted name would pay for)
// first, next: the keys by their name's hash (an index + 1, 0 none) - every name worked out looks itself up here
#define XVARS_HASH 64
struct xvars { int n, dotted; char *key[XVARS_MAX], *val[XVARS_MAX]; unsigned char first[XVARS_HASH], next[XVARS_MAX]; };
struct xval;
struct xview;
struct xctx {
    void *pl, *other, *third;               // the host's: the speaker, them, a third - them NULL: the local player
    const char *const *fpn;                 // a function's parameters while its body is worked out (x_call)
    const struct xval *fpv;
    int fnp;
    int found;                              // pl/other/third looked up (the host's value)
    const char *const *pn, *const *pv;      // the text's placeholders
    int np;
    const struct xvars *vars[3];            // searched in order: the personality's, the file's
    int depth, failed, bad, circle;         // nesting: vars in vars; mistakes while
                                            // working it out (x_fail, told); not an expression; too deep
    int nogame;                             // before the game (a personality's color): its values and players are mistakes
    int game;                               // a decision (decide): it changes the game - draws from the game's generator
    int fixed;                              // an ini file's value (ini.c): no draws, no lists, told quietly - 1 no names, 2
                                            // only the key's section's (vars[0], x_const_vec2)
    int missed;                             // x_missing from the game's values (diplo.c): the primary fails
    int script;                             // a script (scenarios): its functions (wait, wait_until, every, for ...) and commands
    int calls;                              // functions called inside each other now
    const struct xview *view;               // a compiled function's names (THE VIEW), NULL: none runs
    const char *top;                        // the source being worked out (for the mistakes' messages)
};
// a number, a text or both - or nothing (w 0); w: how many items it was chosen from (a list's weight in a list);
// obj: an object of the host's, okind its kind (host.h; a player: its index + 1; its text is its name) - equal only to
// itself; miss: one who isn't there (lord without one; who: x_miss_name, NULL none) - nothing, which may only be
// checked (lord ? .., lord ?? .., lord == them): used, it is a mistake
// it, ln: a list's items (ln 0: no list) - its value meanwhile is one of them, at random; kind: a function (fn: its
// definition and where it was made), stop (a for / every body's "no more"), a vector (vc: its vn numbers, 2..4 -
// .x .y .z .w; in the region like a text), a record (rec: its members by name) or a trigger (n: its index -
// TRIGGERS below) - neither number nor text - or a dict (dict: its entries, DICTS below; one table, shared by every
// value that holds it: d[k] = v is seen through all of them)
// XV_UNSET: a compiled function's slot before its let; XV_REF: a place (ref: STRUCTS AND REFS) - only in a ref's slot
// and on the machine's stack on its way there, never a value the language hands on
// XV_HLIST: a list (LISTS: hl its table - a handle, shared by every value that holds it)
enum { XV_PLAIN, XV_FN, XV_STOP, XV_VEC, XV_REC, XV_TRIG, XV_DICT, XV_UNSET, XV_REF, XV_HLIST };
struct xfn;
struct xrec;
struct xdict;
struct xref;
struct xhlist;
// (56 bytes, its small numbers a byte each: every value worked out is written and copied whole - it was 88)
struct xval {
    double n; const char *s; const struct xval *it; union { const struct xfn *fn; const struct xref *ref; struct xhlist *hl; }; const double *vc;
    const struct xrec *rec; struct xdict *dict;
    const char *miss; int w, obj, ln; unsigned char has_n, has_s, kind, vn, okind;
};
// a record's members in the order written (in the region); ty: a struct's value (STRUCTS AND REFS: its type, key the
// type's own names - not the value's), NULL a plain record
struct xstype;
struct xrec { int n; const char **key; struct xval *val; const struct xstype *ty; };
// a struct as the code that runs knows it: its fields' names in their order, its methods' names and functions (mfi:
// each one's number among the code's functions). The fields' types are the check's alone - a value is what it is
struct xstype { const char *name; int nf; const char **fname; int nm; const char **mname; int *mfi; };
// A LIST AS AN ITEM (list<list<int>>: a row). A list in a list is its items there (the language's lists are flat:
// [greeting, more_lines]) - so a list that is to stay one item is boxed: a record of this type, its items the
// record's values (no names), which nothing splices. The check says where (it knows the list's type: an item that is
// a row - XI_ITEM's b, XI_BOX); read out again (l[2], for row in l, first(l), a ref's value) it is the list it was.
// An empty row is a box of none (as a value of its own an empty list is nothing, as ever)
internal const struct xstype g_x_boxty = { "list", 0, NULL, 0, NULL, NULL };
internal int xl_n(const struct xval *v) { return v->ln ? v->ln : v->w > 0; }   // its items (one value: 1, nothing: 0)
internal const struct xval *xl_at(const struct xval *v, int i) { return v->ln ? &v->it[i] : v; }
internal int xb_is(const struct xval *v) { return v->kind == XV_REC && v->w && v->rec && v->rec->ty == &g_x_boxty; }   // a boxed list (a row)
// the same of a value that may be a box: a row's items
// LISTS (list<T>: docs/scripts.md "Lists"): a list is a table of its own, which every value that holds it shares (a
// handle, as a dict) - it grows (add, insert), shrinks (remove_at, clear), its items are set in it (l[2] = v, l[2].x
// = v: the item itself is the place), it holds null where its items may be one. A seq is the other: a value, built
// whole ([...]) and copied. The items are the list's own (xo_copy: malloc'd, apart from every region) and copied out
// when read (xhl_out) - a set or a removal frees what was there, so a value read before must not point into it. A
// list nobody holds any more is freed by the marking from every machine (xhl_sweep - the runner names them,
// g_x_machines) and with the game (x_triggers_reset).
struct xhlist { struct xhlist *next; struct xval *it; int n, cap, mark, num; };
internal struct xhlist *g_xhl;
// a dict's entry (DICTS): its key - kobj an object's number, kn an int, ks a text (malloc'd) -, its value - vn, vs
// (malloc'd) of the four plain types, else xv: a value of the dict's own (as a list's item: xo_copy, malloc'd)
struct xdent { int kobj; double kn; char *ks; double vn; char *vs; int vhas_s; struct xval *xv; };
// ix: THE INDEX (xdict_find) - nix places, an entry's number + 1 in each; nidx: the entries in it so far; odd: a text
// key among them that isn't plain (x_plain); mark: the sweep's (as a list's)
struct xdict { struct xdict *next; int kt, vt, n, cap; struct xdent def, *e; int *ix, nix, nidx, odd, mark; };
internal struct xdict *g_xdicts;
internal int g_xdict_n;   // the dicts in g_xdicts
internal int g_xhl_n, g_xhl_made;   // lists, made since the last sweep
struct xvm;
internal void xvm_copy(struct xval *dst, const struct xval *src);   // (THE MACHINE: a value copied into the region now)
internal int (*g_x_machines)(struct xvm **out, int cap);   // the runner's machines (test/scripts.c), NULL: none - no sweep
// a list's method: l.add(x), l.insert(i, x), l.remove_at(i), l.clear(), l.index_of(x) (number + 1 in the check's
// table and the machine's: XHM_)
enum { XHM_ADD = 1, XHM_INSERT, XHM_REMOVE_AT, XHM_CLEAR, XHM_INDEX_OF };
internal int xhm_id(const char *name) {
    static const char *const k[] = { "add", "insert", "remove_at", "clear", "index_of" };
    for (int i = 0; i < 5; i++) if (!strcmp(name, k[i])) return i + 1;
    return 0;
}
internal int xli_n(const struct xval *v) { return xb_is(v) ? v->rec->n : xl_n(v); }
internal const struct xval *xli_at(const struct xval *v, int i) { return xb_is(v) ? &v->rec->val[i] : xl_at(v, i); }
internal struct xvars g_vars_sys;   // mod/system.eeai vars (texts.c)
internal int x_fill(char *out, int cap, const char *tmpl, struct xctx *c);
#include "script/host.h"   // what the language asks the program it runs in (the game: eeaimod.c x_host_install)
// a script's world (test/scripts.c, the script runner): the game's time in seconds, waiting until a game second (unused: a
// script's wait is its machine stopping - SCRIPTS COMPILED), a pipe command run with its reply, an effect on the game told (p.win(), p.gold = 500 - the
// host's: 1 only told, as a dry run's, 0 to be done)
internal struct {
    double (*now)(void); void (*wait)(double until); void (*command)(const char *cmd, char *reply, int cap);
    int (*did)(const char *what);
    void (*note)(const char *what);   // what a compiled script tells its runner (a hook of its top called)
} g_x_hooks;

// THE VALUES' REGIONS: their texts and lists, in chunks. The shared one (the lines, prompts, settings) starts afresh
// with the outermost evaluation (x_eval, x_fill from outside): what a caller reads of a value it got lives until the
// next. A script has its own (its variables outlive anything else worked out meanwhile), emptied of what it
// dropped by copying what it holds (xvm_gc: after a turn, and in a turn's loops - a loop that never waits must not
// pile up what each round made). total: its chunks' bytes
struct xchunk { struct xchunk *next; size_t cap, used; char d[]; };
struct xregion { struct xchunk *first, *cur; int busy; size_t total; };
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
        r->total += cap;
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
    r->total = r->first->cap;
}
internal void x_region_free(struct xregion *r) {
    for (struct xchunk *k = r->first, *nx; k; k = nx) { nx = k->next; free(k); }
    r->first = r->cur = NULL;
    r->total = 0;
}
internal const char *x_strdup(const char *s, size_t n) {   // n bytes of s as a text in the region
    char *r = x_alloc(n + 1);
    if (!r) return "";
    memcpy(r, s, n);
    r[n] = 0;
    return r;
}


// a vars function whose body is statements, name(a, b) { ... }: its value is X_BODY, then "(a, b) => { ... }" (tfile_parse)
#define X_BODY '\x01'
// two names or texts the same, their case ignored - exactly as lstrcmpiA tells it (the user's language: an umlaut's
// capital and small letter the same), but without asking it while every character is plain ASCII (x_plain_char):
// Windows' compare takes ~100 ns, and a name looked up by its letters meets dozens of names on its way (a builtin's
// was found after up to 22 of them). Not plain: a character above 0x7E or below a space (Windows ignores some, folds
// others), the apostrophe and the hyphen (its word order weighs them apart from the letters)
internal int x_plain_char(unsigned c) { return c - 0x20u <= 0x5Eu && c != '-' && c != 39; }
internal int x_plain(const char *s) {   // a text of plain characters only
    for (; *s; s++) if (!x_plain_char((unsigned char)*s)) return 0;
    return 1;
}
internal int x_ieq(const char *a, const char *b) {
    for (const unsigned char *p = (const unsigned char *)a, *q = (const unsigned char *)b; ; p++, q++) {
        unsigned x = *p, y = *q;
        if ((x && !x_plain_char(x)) || (y && !x_plain_char(y))) return !lstrcmpiA(a, b);
        if (x - 'A' < 26u) x += 32;
        if (y - 'A' < 26u) y += 32;
        if (x != y) return 0;
        if (!x) return 1;
    }
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
    if (i < 0 && v->n == XVARS_MAX) { x_error("%s: more than %d names in one file - left out", key, XVARS_MAX); return; }
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
internal struct { char name[40]; int n, blk; } g_xfn[96];   // blk: its body is statements (name(a) { ... })
internal int g_xfn_n, g_xfn_scanned;
// the parameters ("a,b)") of the function whose definition is being read: its body's syntax check takes them for
// players where a path starts with one (by.kills(of)) - at run time the context has them (c->fpn)
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
// THE NAMES (vars) of all the files too (x_name_check: an entry may come from another file, or later in this one) -
// each one's data (name = value, a record) or worked out where it is used (name => ...)
internal char g_xvn[256][40];
internal unsigned char g_xvd[256];
internal int g_xvn_n;
// THE FIELDS of all the files (let name = ... at a file's top): one per AI, read and set by the host (field_get / set)
internal char g_xfld[64][40];
internal int g_xfld_n;
internal void x_field_add(const char *b, size_t n) { if (n && n < 40 && g_xfld_n < 64) lstrcpynA(g_xfld[g_xfld_n++], b, (int)n + 1); }
internal int x_field_known(const char *name) {
    if (!g_xfn_scanned) { g_xfn_scanned = 1; g_x_host.scan_functions(); }
    for (int i = 0; i < g_xfld_n; i++) if (x_ieq(g_xfld[i], name)) return 1;
    return 0;
}
internal void x_var_add(const char *b, size_t n, int data) {
    if (n && n < 40 && g_xvn_n < 256) { g_xvd[g_xvn_n] = (unsigned char)data; lstrcpynA(g_xvn[g_xvn_n++], b, (int)n + 1); }
}
// a comment: // to the line's end, as in the game's own scripts (.rmv, .tai) - a # is a mistake (the game's # is a
// preprocessor's: #define, #include)
internal int x_comment(const char *s) { return s[0] == '/' && s[1] == '/'; }
#define X_HASH_WHY "# - a comment starts with // (as in the game's own scripts)"
internal int x_string_at(const char *s);
internal int x_string(const char *s, char *out, int cap, const char **end, const char **why);
// the lines "name(a, b) =>" of a file, and "name =" / "name =>" outside its records - a record too (its members:
// name.key) unless the program reads it itself (lines, personality ...: g_x_host.code_record)
internal void x_fn_scan(const char *text) {
    int depth = 0;   // records (and braces) open at the line's start
    for (const char *s = text; *s; ) {
        while (*s == ' ' || *s == '\t') s++;
        const char *b = s;
        while (isalnum((unsigned char)*s) || *s == '_') s++;
        size_t n = (size_t)(s - b);
        if (!depth && n == 3 && !strncmp(b, "let", 3) && (*s == ' ' || *s == '	')) {   // let name = ...: a field
            const char *f = s;
            while (*f == ' ' || *f == '	') f++;
            const char *fb = f;
            while (isalnum((unsigned char)*f) || *f == '_') f++;
            x_field_add(fb, (size_t)(f - fb));
        } else if (!depth && n && *s != '(') {
            const char *e = s;
            while (*e == ' ' || *e == '\t') e++;
            if (*e == '=' && e[1] != '=') {
                char nm[40];
                lstrcpynA(nm, b, (int)(n < 39 ? n : 39) + 1);
                if (e[1 + strspn(e + 1, " \t")] != '{' || !g_x_host.code_record(nm)) x_var_add(b, n, e[1] != '>');
            }
        }
        for (const char *q = s; *q && *q != '\n' && !x_comment(q); q++) {   // the line's braces, strings left out
            if (x_string_at(q)) {
                const char *e, *why;
                if (x_string(q, NULL, 0, &e, &why) < 0) break;
                q = e - 1;
            } else if (*q == '{') depth++;
            else if (*q == '}' && depth) depth--;
        }
        if (n && n < 40 && *s == '(') {
            int k = 1, empty = 1;
            for (s++; *s && *s != ')' && *s != '\n'; s++) { if (*s == ',') k++; else if (*s != ' ') empty = 0; }
            if (*s == ')') {
                const char *e = s + 1;
                while (*e == ' ' || *e == '\t') e++;
                if (((*e == '=' && e[1] != '=') || *e == '{') && g_xfn_n < 96) {
                    lstrcpynA(g_xfn[g_xfn_n].name, b, (int)n + 1);
                    g_xfn[g_xfn_n].blk = *e == '{';
                    g_xfn[g_xfn_n++].n = empty ? 0 : k;
                }
            }
        }
        while (*s && *s != '\n') s++;
        if (*s) s++;
    }
}
internal int x_fn_blk(const char *name) {   // a function of the files whose body is statements
    if (!g_xfn_scanned) { g_xfn_scanned = 1; g_x_host.scan_functions(); }
    for (int i = 0; i < g_xfn_n; i++) if (x_ieq(g_xfn[i].name, name)) return g_xfn[i].blk;
    return 0;
}
internal int x_fn_known(const char *name) {   // its parameter count, -1: no function of that name
    if (!g_xfn_scanned) { g_xfn_scanned = 1; g_x_host.scan_functions(); }
    for (int i = 0; i < g_xfn_n; i++) if (x_ieq(g_xfn[i].name, name)) return g_xfn[i].n;
    return -1;
}
internal int x_var_known(const char *name) {
    if (!g_xfn_scanned) { g_xfn_scanned = 1; g_x_host.scan_functions(); }
    for (int i = 0; i < g_xvn_n; i++) if (x_ieq(g_xvn[i], name)) return 1;
    return 0;
}
internal int x_var_data(const char *name) {   // a name of the files that is data (name = value): worked out the same anywhere
    if (!g_xfn_scanned) { g_xfn_scanned = 1; g_x_host.scan_functions(); }
    for (int i = 0; i < g_xvn_n; i++) if (x_ieq(g_xvn[i], name)) return g_xvd[i];
    return 0;
}
// THE NAMES CHECK while a file is read (tfile_parse): a name that is nothing known - no parameter, constant, vars
// entry of any file, game value or placeholder the code fills for this key - is told with its file and line (strict
// parsing: in the startup box); the key is kept, at run time the name fails what it is in (x_fail)
internal const char *g_x_check_file, *g_x_check_ph;   // the file being read; " a b " the code's placeholders for the key
internal const char *g_x_check_has;   // " lord " the names the code guarantees for the key (g_x_host.has): not null there
internal int g_x_check_line;
internal int xvec_member(const char *m) {   // a vector's number by its name: .x .y .z .w - 0..3, -1 none
    return !strcmp(m, "x") ? 0 : !strcmp(m, "y") ? 1 : !strcmp(m, "z") ? 2 : !strcmp(m, "w") ? 3 : -1;
}
internal int x_name_known(const char *name) {   // a parameter, constant, vars entry, game value or the code's placeholder
    struct xval t;
    char w[48];
    if (strlen(name) > 40) return 1;
    const char *dot = strchr(name, '.');
    if (dot && dot - name < 40) {   // r.a, home.x: a member path - its head a parameter, vars entry (a file's record) or field
        lstrcpynA(w, name, (int)(dot - name) + 1);
        if (x_check_par_has(w, (int)strlen(w)) || x_var_known(w) || x_field_known(w)) return 1;
    }
    wsprintfA(w, " %s ", name);
    return x_check_par_has(name, (int)strlen(name)) || g_x_host.constant(name, &t) || x_var_known(name) || x_field_known(name) || g_x_host.name_valid(name) ||
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
// who isn't there, as a value's miss: the name kept for good - a path of the files or a script's (few: each once), up
// to 23 characters as the messages show it - so a value holds a word, not the text
internal const char *x_miss_name(const char *who) {
    static const char **names;
    static int n, cap;
    char t[24];
    lstrcpynA(t, who, sizeof t);
    if (!t[0]) return NULL;
    for (int i = 0; i < n; i++) if (!strcmp(names[i], t)) return names[i];
    if (n == cap) {
        const char **m = realloc((void *)names, (size_t)(cap ? cap * 2 : 32) * sizeof *m);
        if (!m) return "someone";
        names = m;
        cap = cap ? cap * 2 : 32;
    }
    char *d = _strdup(t);
    if (!d) return "someone";
    return names[n++] = d;
}
internal void xnothing(struct xval *v) { v->has_n = v->has_s = v->w = v->obj = v->okind = v->ln = v->kind = v->vn = 0; v->n = 0; v->s = ""; v->it = NULL; v->fn = NULL; v->vc = NULL; v->rec = NULL; v->miss = NULL; }
internal void xnum(struct xval *v, double n) { v->has_n = 1; v->has_s = v->obj = v->okind = v->ln = v->kind = v->vn = 0; v->w = 1; v->n = n; v->s = ""; v->it = NULL; v->fn = NULL; v->vc = NULL; v->rec = NULL; v->miss = NULL; }
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
    v->miss = NULL;
}
// a placeholder the code fills ({tribute}, {minutes}): a text - and its number when it is one, the code's numbers
// coming as texts
internal void xstr_or_num(struct xval *v, const char *s) { xstr(v, s); v->has_n = xnumeric(v->s, &v->n); }
internal void xdual(struct xval *v, double n, const char *s) { xstr(v, s); v->has_n = 1; v->n = n; }
internal void xdual_kept(struct xval *v, double n, const char *s) {   // the same with a text that stays (not copied)
    v->s = s; v->has_s = v->has_n = 1; v->n = n; v->w = 1;
    v->obj = v->okind = v->ln = v->kind = v->vn = 0; v->it = NULL; v->fn = NULL; v->vc = NULL; v->rec = NULL; v->miss = NULL;
}
// a number and nothing else (no text with it, no list it was drawn from, nobody missing): the machine works + - * /
// and the comparisons of two such out itself
#define XV_IS_NUM(v) ((v)->has_n && (v)->w && !(v)->has_s && !(v)->ln && !(v)->kind && !(v)->obj && !(v)->miss)
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
    if (r - (double)(long long)r < 1e-9 && r < 1e15) {   // (its digits by hand: printf takes ~100 ns, and most numbers said are whole)
        char t[24];
        long long x = (long long)n;
        unsigned long long u = x < 0 ? 0ull - (unsigned long long)x : (unsigned long long)x;
        int k = sizeof t, o = 0;
        do t[--k] = (char)('0' + u % 10); while (u /= 10);
        if (x < 0) t[--k] = '-';
        while (k < (int)sizeof t && o < cap - 1) out[o++] = t[k++];
        if (cap > 0) out[o] = 0;
    } else {
        snprintf(out, (size_t)cap, "%.2f", x_round_dec(n, 2));
        char *e = out + strlen(out);
        while (e > out && e[-1] == '0') *--e = 0;
        if (e > out && e[-1] == '.') e[-1] = 0;
    }
}
internal void xto_text(const struct xval *v, char *out, int cap) {
    if (v->kind == XV_HLIST && v->w && v->hl) {   // a list: its items, whole (a handle is one value: no item picked)
        int o = 0;
        out[o++] = '[';
        for (int i = 0; i < v->hl->n && o < cap - 6; i++) {
            if (i) { out[o++] = ','; out[o++] = ' '; }
            xto_text(&v->hl->it[i], out + o, cap - o - 2);
            o += (int)strlen(out + o);
        }
        if (o > cap - 2) o = cap - 2;
        out[o++] = ']';
        out[o] = 0;
        return;
    }
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
    if (xb_is(v)) {   // a row: as the list it is
        int o = 0;
        out[o++] = '[';
        for (int i = 0; i < v->rec->n && o < cap - 6; i++) {
            if (i) { out[o++] = ','; out[o++] = ' '; }
            xto_text(&v->rec->val[i], out + o, cap - o - 2);
            o += (int)strlen(out + o);
        }
        if (o > cap - 2) o = cap - 2;
        out[o++] = ']';
        out[o] = 0;
        return;
    }
    if (v->kind == XV_REC && v->w) {   // a record: {a = 1, b = text} - its members as written; a struct: point(1, 2)
        int o = 0, st = v->rec->ty != NULL;
        if (st) o = snprintf(out, (size_t)(cap > 8 ? cap - 8 : 0), "%s", v->rec->ty->name);
        if (o > cap - 8) o = cap > 8 ? cap - 8 : 0;
        out[o++] = st ? '(' : '{';
        for (int i = 0; i < v->rec->n && o < cap - 8; i++) {
            if (st) o += snprintf(out + o, (size_t)(cap - o), "%s", i ? ", " : "");
            else o += snprintf(out + o, (size_t)(cap - o), "%s%s = ", i ? ", " : "", v->rec->key[i]);
            if (o >= cap - 4) { o = cap - 4; break; }
            xto_text(&v->rec->val[i], out + o, cap - o - 2);
            o += (int)strlen(out + o);
        }
        if (o > cap - 2) o = cap - 2;
        out[o++] = st ? ')' : '}';
        out[o] = 0;
        return;
    }
    if (v->kind == XV_TRIG && v->w) { snprintf(out, (size_t)cap, "trigger %d", (int)v->n + 1); return; }
    if (v->kind && v->w) { lstrcpynA(out, v->kind == XV_FN ? "function" : "stop", cap); return; }
    if (v->has_s || !v->w) { lstrcpynA(out, v->w ? v->s : "", cap); return; }
    xnum_text(v->n, out, cap);
}
internal const struct xval *xrec_get(const struct xval *r, const char *key) {   // a record's member by name; NULL: none
    for (int i = 0; r->kind == XV_REC && r->rec->key && i < r->rec->n; i++) if (!strcmp(r->rec->key[i], key)) return &r->rec->val[i];
    return NULL;
}
internal void xnothing(struct xval *v);
internal void *x_alloc(size_t n);
internal void xb_box(struct xval *v) {   // v (a list, one value, nothing) made one item: a row
    int n = xl_n(v);
    struct xrec *rec = x_alloc(sizeof *rec);
    struct xval *one = n && !v->ln ? x_alloc(sizeof *one) : NULL;
    if (!rec || (n && !v->ln && !one)) return;   // (no memory: as it is)
    if (one) *one = *v;
    rec->n = n; rec->key = NULL; rec->val = v->ln ? (struct xval *)(uintptr_t)v->it : one; rec->ty = &g_x_boxty;
    xnothing(v);
    v->w = 1; v->kind = XV_REC; v->rec = rec;
}
internal void xb_unbox(struct xval *v) {   // a row read out of its list: the list it is
    if (!xb_is(v)) return;
    const struct xrec *r = v->rec;
    xnothing(v);
    if (r->n) { v->it = r->val; v->ln = v->w = r->n; }
}
internal const char *xtext_of(const struct xval *v) {   // its text, whole (in the region for a number, a list)
    if (v->ln || ((v->kind == XV_VEC || v->kind == XV_REC || v->kind == XV_TRIG || v->kind == XV_HLIST) && v->w)) { char t[XVAL_CAP]; xto_text(v, t, sizeof t); return x_strdup(t, strlen(t)); }
    if (v->kind && v->w) return v->kind == XV_FN ? "function" : "stop";
    if (v->has_s || !v->w) return v->w ? v->s : "";
    char t[48];
    xnum_text(v->n, t, sizeof t);
    return x_strdup(t, strlen(t));
}

// a string literal at s (" or '): its text into out (cap bytes) - escapes undone, {braces} kept as written (an
// expression's quotes are its own there); *end after the closing quote. Returns the length, -1 on a mistake (*why).
// out NULL, cap 0: only checked. A verbatim string @"..." (as C#'s: a path, @"Data\Scenarios\x.scn") has neither
// escapes nor placeholders - a \ and a { are text - and "" for a quote in it.
#define X_LBRACE '\x01'   // \{ and \} in a string: braces that are text, not a placeholder (x_fill writes them)
#define X_RBRACE '\x02'
internal int x_string_at(const char *s) { return *s == '"' || *s == '\'' || (*s == '@' && s[1] == '"'); }   // a string starts
internal int x_string(const char *s, char *out, int cap, const char **end, const char **why) {
    if (*s == '@') {   // verbatim
        int o = 0;
        for (s += 2;; s++) {
            char c = *s;
            if (!c || c == '\n' || c == '\r') { *why = "a string without its closing quote"; return -1; }
            if (c == '"' && s[1] != '"') { if (cap > 0) out[o < cap ? o : cap - 1] = 0; *end = s + 1; return o; }
            if (c == '"') s++;   // "": a quote
            else if (c == '{') c = X_LBRACE;
            else if (c == '}') c = X_RBRACE;
            if (o < cap - 1) out[o++] = c;
        }
    }
    char q0 = *s, q = 0, raw = 0;
    int o = 0, depth = 0;
    for (s++;; s++) {
        char c = *s;
        if (!c || c == '\n' || c == '\r') { *why = depth ? "a { without its }" : "a string without its closing quote"; return -1; }
        if (depth) {   // in {braces}: an expression, kept as written
            if (raw) { if (c == '"' && s[1] == '"') { if (o < cap - 1) out[o++] = c; c = *++s; } else if (c == '"') q = raw = 0; }
            else if (q) { if (c == q) q = 0; else if (c == '\\' && s[1] && s[1] != '\n') { if (o < cap - 1) out[o++] = c; c = *++s; } }
            else if (c == '@' && s[1] == '"') { if (o < cap - 1) out[o++] = c; c = *++s; q = raw = 1; }
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
       XN_INDEX, XN_MCALL, XN_SETM, XN_DICT, XN_SETI, XN_STRUCT, XN_REF, XN_HLIST };
struct xnode {
    unsigned char k, op, lvl, depth;
    unsigned char sk;            // the check's notes for the compiler (STRUCTS AND REFS, LISTS): 1 a set or call through a
                                 // struct's place, 2 a struct's method of a value that is no place; as a seq's item: 4
                                 // a row (boxed; on a list's method: its value boxed), 8 kept when null, 16 a list
                                 // spliced; 32 a [...] made a list; 64 an item of a list as a place; 128 a list's method
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
    const char *end;             // where a primary ends in the source (a name, a call, an item, a member: the language
                                 // server's record - the type of what stands before a dot); NULL the rest
    struct xgen *gen;            // a generic function's lambda: its type parameters and copies (GENERICS); NULL the rest
    const char *s0;              // a call of a generic function or method: its name as written - s its copy's, made so by
                                 // each check anew (a tree of the cache is any script's with that text); NULL the rest
};
// STRUCTS AND REFS (a script's; docs/scripts.md "Structs", "Refs"). struct point { x: int  y: int = 5  add(v: int) { ... } }
// at the script's top is a type of its own: point(), point(3), point(0, 1) make a value - its fields in order, those
// left out their defaults (declared, else 0, "", false, an empty list, null for a T?, a struct's own) - a.x reads a
// field, a.x = 5 sets it, a.add(2) calls a method (its fields there: this.x). A VALUE, as a number is: let b = a is a
// copy, a function gets a copy, a list holds copies. What changes one changes the place it is in - a variable, a
// field of one, a list's item (l[2].x = 5, l[2] = v): the value there is made anew with the change and put back
// (a struct's value is a block that is never written again: a copy shares it, a set makes the next one).
// A ref is a second name for such a place: a parameter (shift(ref p: point, by: int), called shift(ref a, 2)), a
// local (ref q = armies[2]) - and a method's this. It is a variable and the way from it (fields by number, items by
// number, taken when the ref is made), never a pointer: it survives a wait and a save. It lives in its slot only -
// read, it is the value there; no list, record or field holds one, none is handed back, and a function made where
// one is doesn't see it (it may run when the place is gone).
// The types are the check's (x_check: what a field takes, what is a place); the code that runs knows a struct's
// value by the type it carries (xrec.ty) - the check marks the sets and calls that are a struct's (xnode.sk) for the
// compiler, which can't tell a.x = 5 of a struct from p.gold = 5 of a player. The script being read: its structs'
// names from before it is parsed (a type is written before its struct too), their declarations for the check.
#define XST_MAX 100
#define XT_STRUCT 128   // a struct's type: XT_STRUCT + its number (host.h's types end below)
internal struct { char name[40]; const struct xnode *decl; char iface; } g_xst[XST_MAX];   // iface: an interface (GENERICS)
internal int g_xst_n;
internal int g_x_in_ph;   // the check is in a string's {placeholder} (a text's code: no struct's method there)
internal int xst_find(const char *name) {
    for (int i = 0; i < g_xst_n; i++) if (!strcmp(g_xst[i].name, name)) return i;
    return -1;
}
// "struct name" and "interface name" at a line's start, each in src: the names (x_parse, before a script's statements
// are read)
internal void xst_scan(const char *src) {
    g_xst_n = 0;
    for (const char *s = src; s && *s; s = strchr(s, '\n') ? strchr(s, '\n') + 1 : NULL) {
        while (*s == ' ' || *s == '\t') s++;
        int w = !strncmp(s, "struct", 6) && (s[6] == ' ' || s[6] == '\t') ? 6 : !strncmp(s, "interface", 9) && (s[9] == ' ' || s[9] == '\t') ? 9 : 0;
        if (!w) continue;
        const char *q = s + w;
        while (*q == ' ' || *q == '\t') q++;
        int k = 0;
        char name[40];
        while ((isalnum((unsigned char)*q) || *q == '_') && k < 39) name[k++] = *q++;
        name[k] = 0;
        if (!k || isalnum((unsigned char)*q) || *q == '_' || g_xst_n == XST_MAX) continue;   // (told when it is parsed)
        lstrcpynA(g_xst[g_xst_n].name, name, sizeof g_xst[0].name);
        g_xst[g_xst_n].iface = w == 9;
        g_xst[g_xst_n++].decl = NULL;
    }
}
// THE TYPES THAT HOLD TYPES (the check's: host.h's types are numbers): a function's with its parameters' types and
// its result's (func<int, text, point>: the last is the result, void none), a record's with its members' names and
// types ({ at = vec(1, 2), units = 10 }), and a wrap - a whole type (a list, one or null) as a list's item
// (list<list<int>>, list<int?>: X_LIST_OF of a wrap). Each is an entry here, the same type the same entry (compared
// by its number), its number a type's base from XT_CMP0 on; they stay for the session (a few hundred at most: the
// types the scripts and texts write and the check works out).
enum { XCT_WRAP = 1, XCT_FN, XCT_REC, XCT_HLIST, XCT_DICT };   // HLIST: a list<T> (LISTS), t[0] its items' type; DICT: dict<K, V> (DICTS), t[0] K, t[1] V
#define XCT_MEMBERS 16
#define XCT_MAX 2048
#define XT_CMP0 0x400
struct xctype { int kind, n, ret; int t[XCT_MEMBERS]; const char *name[XCT_MEMBERS]; };   // WRAP: t[0]; FN: t[0..n), ret; REC: name[], t[]
internal struct xctype g_xct[XCT_MAX];
internal int g_xct_n;
internal int xct_base(int i) { return (((i >> 8) + 1) << 10) | (i & 0xFF); }   // (bits 8 and 9 are the list's and the null's)
internal const struct xctype *xct_of(int t) {   // a type's entry (its base's), NULL: none of these
    int b = X_TBASE(t), i = b >= XT_CMP0 ? ((((b >> 10) - 1) << 8) | (b & 0xFF)) : -1;
    return i >= 0 && i < g_xct_n ? &g_xct[i] : NULL;
}
internal int xct_get(int kind, int n, const int *t, int ret, const char *const *name) {   // its type (a base), -1: no room
    if (n > XCT_MEMBERS) return -1;
    for (int i = 0; i < g_xct_n; i++) {
        const struct xctype *c = &g_xct[i];
        int same = c->kind == kind && c->n == n && c->ret == ret;
        for (int j = 0; same && j < n; j++) same = c->t[j] == t[j] && (!name || !strcmp(c->name[j], name[j]));
        if (same) return xct_base(i);
    }
    if (g_xct_n == XCT_MAX) return -1;
    struct xctype *c = &g_xct[g_xct_n];
    c->kind = kind; c->n = n; c->ret = ret;
    for (int j = 0; j < n; j++) { c->t[j] = t[j]; c->name[j] = name ? _strdup(name[j]) : NULL; }
    return xct_base(g_xct_n++);
}
// a list of items of type e (any type: a list, one or null too - those wrapped); the items' type of a list's type
internal int xt_list_of(int e) {
    if (e < 0) return -1;
    if (!(e & (X_LIST_OF | X_NULLABLE))) return X_LIST_OF | e;
    int w = xct_get(XCT_WRAP, 1, &e, 0, NULL);
    return w < 0 ? X_LIST_OF : X_LIST_OF | w;
}
internal int xt_elem(int l) {
    const struct xctype *c = xct_of(l);
    return c && c->kind == XCT_WRAP ? c->t[0] : X_TBASE(l);
}
internal int xt_is_hl(int t) { const struct xctype *c = t & X_LIST_OF ? NULL : xct_of(t); return c && c->kind == XCT_HLIST; }   // a list<T> (or one or null)
internal int xt_hl_elem(int t) { return xt_is_hl(t) ? xct_of(t)->t[0] : X_ANY; }
internal int xt_items(int t) { return t & X_LIST_OF ? xt_elem(t) : xt_hl_elem(t); }   // a seq's or a list's items' type (0: neither, or not known)
internal int xt_is_dict(int t) { const struct xctype *c = t & X_LIST_OF ? NULL : xct_of(t); return c && c->kind == XCT_DICT; }   // a dict<K, V> (or one or null)
internal int xt_dict_key(int t) { return xt_is_dict(t) ? xct_of(t)->t[0] : X_ANY; }
internal int xt_dict_val(int t) { return xt_is_dict(t) ? xct_of(t)->t[1] : X_ANY; }
// a dict's key: an int, a text or an object of a kind (by its number, as == tells them apart); its value: any type -
// the four plain ones kept as they are (the personalities' dict fields read and write those), any other as a value
// of the dict's own (DICTS)
internal int xt_dict_key_ok(int k) { return k == XT_INT || k == XT_TEXT || (k > 0 && k < XT_INT); }
internal int xt_plain_val(int v) { return v == XT_INT || v == XT_FLOAT || v == XT_BOOL || v == XT_TEXT; }
// a type as written at *ps (after a ':'): its letters into out - names, ?, and inside < > also commas and spaces
// (func<int, text>); *ps after it. Its length, 0 none
internal int xps_type_scan(const char **ps, char *out, int cap) {
    const char *s = *ps;
    int k = 0, depth = 0;
    for (; *s && k < cap - 1; s++) {
        if (*s == '<') depth++;
        else if (*s == '>' && depth > 0) depth--;
        else if (!(isalnum((unsigned char)*s) || *s == '_' || *s == '?' || (depth > 0 && (*s == ',' || *s == ' ')))) break;
        out[k++] = *s;
    }
    out[k] = 0;
    *ps = s;
    return k;
}
#define X_LEVELS 24   // the levels of the old parser - a deeper expression is none (not an expression)
#define X_CALLS 64    // functions called inside each other
// the parser within itself: ( [ { ref - not, a function's body, a block - at most X_DEEP deep, else a mistake while the
// file is read: each level is a frame or ten on the stack (about 1.2 KB; 1 MB, the game's and the checker's, ran out at
// 800 brackets), and a tree that deep is past X_LEVELS anyway but for a body's own count
#define X_DEEP 128
// the parser's state
struct xps {
    const char *s, *why, *src0;
    int err, par;          // par: in ( ), not in [ ] (strings side by side join there)
    int deep;              // how deep the parser is in itself (X_DEEP)
    int nl;                // a line break in the space last skipped, which ends at nl_pos
    const char *nl_pos;
    int line;              // the line of lc (counting up from src0)
    const char *lc;
    char *mem;             // the nodes' memory (one block: copied out when done)
    size_t used, cap;
};
internal int xps_deeper(struct xps *p) {   // one level further in (xps_shallower back out); 0 past X_DEEP: p->err
    if (++p->deep <= X_DEEP) return 1;
    p->err = 1;
    p->why = "nested too deep - brackets, functions and blocks within each other";
    return 0;
}
internal void xps_shallower(struct xps *p) { p->deep--; }
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
// spaces, line breaks and comments (// to the line's end); nl: whether a line break was among them
internal void xps_space(struct xps *p) {
    const char *b = p->s;
    int nl = 0;
    for (;;) {
        char c = *p->s;
        if (c == ' ' || c == '\t' || c == '\r') p->s++;
        else if (c == '\n') { nl = 1; p->s++; }
        else if (x_comment(p->s)) { while (*p->s && *p->s != '\n') p->s++; }
        else if (c == '#' && !p->err) { p->err = 1; p->why = X_HASH_WHY; break; }
        else break;
    }
    if (p->s != b) { p->nl = nl; p->nl_pos = p->s; }
}
internal int xps_newline_before(struct xps *p) { return p->nl && p->nl_pos == p->s; }   // after xps_space
internal int xps_word(struct xps *p, const char *w) {   // a keyword (return, in ...), not the start of a longer name
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
    if (n == 1 && strchr("+-*/", *op) && p->s[1] == '=') return 0;   // p.gold += 100: the statement's, not an operator
    p->s += n;
    return 1;
}
internal int x_word_reserved(const char *name) {   // a word of the language: no variable's or parameter's name
    static const char *const k[] = { "true", "false", "null", "return", "and", "or", "not", "let", "if", "else", "while", "for", "in",
                                     "break", "continue", "struct", "ref", "this", "interface" };
    for (size_t i = 0; i < sizeof k / sizeof *k; i++) if (x_ieq(name, k[i])) return 1;
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
// GENERICS (a script's; docs/scripts.md "Interfaces and generic functions"): interface name { field: type
// method(a: type): type } at the script's top is a contract - the members a type must have, read by its shape (a
// struct, one of the game's kinds: unit, player ... - whatever has them; struct point: name { ... } promises it, held
// to it there). It exists only while the script is read: a value is never of an interface. A function takes one as
// its type parameter's bound, nearest<T: positioned>(from: vec2, all: seq<T>): T? { ... }, or as a parameter's type,
// f(a: positioned) (a type parameter of its own for that parameter). Its body is checked once, against the contract:
// a T is its interface there (an interface is struct-like - its fields and methods are a struct's, without code; its
// own name in its members the type that has it), so it reads the contract's members only, sets none, makes none.
// Each call with types of its own makes a copy (x_parse_copy: the declaration parsed again from its source, each type
// parameter's name the call's type - then an ordinary function, checked and compiled as written that way), the call
// made one of the copy (its name nearest<unit>); the function as written is never compiled - the machine knows no
// interface. A call inside a generic body, of types not known yet, is only held against the contract. A struct's
// method the same way: add<T: ipoint>(value: T), add(value: ipoint) - its copies its struct's methods of their own
// (add<cell>, xg_mcall). In an interface its own name is the type that has it (add(value: ipoint): the implementing
// struct's own type), add<T: ipoint>(value: T) asks for a method that takes any ipoint (xg_fits).
#define XG_MAX 4      // type parameters a function
#define XG_COPIES 32  // copies a generic function
struct xgen {
    int n;                       // its type parameters: their names ("#2": the one of parameter 2, f(a: positioned)), bounds
    char name[XG_MAX][24];
    int iface[XG_MAX];
    int np;                      // its parameters' types as written ("#2" for an interface's), for the calls' types
    char ptype[8][64];
    char rtype[64];              // its result's type as written ("" none)
    char sname[40];              // a method's: its struct's (or interface's) name - "" a function's
    const char *src0, *at;       // where it is written: its source, its declaration's start (a method's: its name; a copy is parsed from there)
    int ncopy;
    struct { int bind[XG_MAX]; struct xnode *let; } copy[XG_COPIES];
};
internal struct { char name[24]; int type; } g_xps_tp[XG_MAX];   // the type parameters of the function being parsed
internal int g_xps_ntp, g_xps_in_iface;    // their number; an interface's declaration is being parsed (its name a type there)
internal const int *g_xg_bind;             // the types of the copy being parsed (x_parse_copy), NULL none
internal const char *g_x_type_why;          // why a type written isn't one, when it is more than unknown (an interface)
internal int g_xps_sig;                     // xps_lambda makes a method's signature only (an interface's: no body)
internal struct xnode *g_xg_top;            // the check's: the script's top block and scope, where the copies go;
internal struct xscope *g_xg_topsc;         // inside a generic function as written (its types its bounds: no copies)
internal int g_xg_abstract;
// A TYPE as written (name: type): int, float, bool, text, vec2..vec4, epoch, color, unit_type, function, record,
// trigger, a kind of the host's objects (player); seq<T> a sequence of them, list<T> a list, dict<K, V> a dict, T? or
// null - its code (host.h), -1 none such.
// Types are written with < > only: [ ] is the language's for lists as values and for indexing (list<text> = [])
internal int x_type_named(const char *t) {
    static const char *const own[] = { "int", "float", "bool", "text", "vec2", "vec3", "vec4", "epoch", "color", "unit_type",
                                       "function", "record", "trigger" };
    while (*t == ' ') t++;
    size_t n = strlen(t);
    int flags = 0;
    while (n && t[n - 1] == ' ') n--;
    if (n && t[n - 1] == '?') { flags |= X_NULLABLE; n--; }
    char b[96];
    if (!n || n >= sizeof b) return -1;
    lstrcpynA(b, t, (int)n + 1);
    if (n > 5 && !strncmp(b, "seq<", 4) && b[n - 1] == '>') {   // seq<T>: of any type - seq<seq<int>>, no seq<int?> (a seq leaves a null out)
        b[n - 1] = 0;
        int e = x_type_named(b + 4), l = e >= 0 && !(e & X_NULLABLE) ? xt_list_of(e) : -1;
        return l < 0 ? -1 : l | flags;
    }
    if (n > 6 && !strncmp(b, "list<", 5) && b[n - 1] == '>') {   // list<T>: a list (LISTS), its items of any type, one or null too - no seq<T>? (a row is never none)
        b[n - 1] = 0;
        int e = x_type_named(b + 5), l = e >= 0 && !((e & X_LIST_OF) && (e & X_NULLABLE)) ? xct_get(XCT_HLIST, 1, &e, 0, NULL) : -1;
        return l < 0 ? -1 : l | flags;
    }
    if (n > 6 && !strncmp(b, "dict<", 5) && b[n - 1] == '>') {   // dict<K, V> (DICTS): K an int, a text or an object, V any type
        int depth = 0, kv[2];
        char *comma = NULL;
        b[n - 1] = 0;
        for (char *q = b + 5; *q && !comma; q++) {
            if (*q == '<') depth++;
            else if (*q == '>') depth--;
            else if (*q == ',' && !depth) comma = q;
        }
        if (!comma) return -1;
        *comma = 0;
        kv[0] = x_type_named(b + 5); kv[1] = x_type_named(comma + 1);
        if (!xt_dict_key_ok(kv[0]) || kv[1] < 0) return -1;
        int d = xct_get(XCT_DICT, 2, kv, 0, NULL);
        return d < 0 ? -1 : d | flags;
    }
    if (n > 6 && !strncmp(b, "func<", 5) && b[n - 1] == '>') {   // func<int, text, point>: its parameters' types, then its result's (void: none)
        int ty[XCT_MEMBERS], np = 0, depth = 0;
        b[n - 1] = 0;
        for (char *q = b + 5, *part = b + 5;; q++) {
            if (*q == '<') depth++;
            if (*q == '>') depth--;
            if (*q && !(*q == ',' && !depth)) continue;
            int last = !*q;
            *q = 0;
            while (*part == ' ') part++;
            if (np == XCT_MEMBERS) return -1;
            if ((ty[np] = last && !strcmp(part, "void") ? XT_VOID : x_type_named(part)) < 0) return -1;
            np++;
            if (last) break;
            part = q + 1;
        }
        int f = xct_get(XCT_FN, np - 1, ty, ty[np - 1], NULL);
        return f < 0 ? -1 : f | flags;
    }
    if (strchr(b, '<') || strchr(b, '>') || strchr(b, '?') || strchr(b, ',') || strchr(b, ' ')) return -1;
    for (int i = 0; i < g_xps_ntp; i++) if (!strcmp(b, g_xps_tp[i].name)) return g_xps_tp[i].type | flags;   // a type parameter (GENERICS)
    for (size_t i = 0; i < sizeof own / sizeof *own; i++) if (!strcmp(b, own[i])) return (XT_INT + (int)i) | flags;
    for (int k = 1; k < XT_INT; k++) { const struct x_kind *kd = g_x_host.kind(k); if (kd && !strcmp(kd->name, b)) return k | flags; }
    int si = xst_find(b);
    if (si >= 0 && g_xst[si].iface && !g_xps_in_iface) {   // an interface: no value's type
        static char why[200];
        snprintf(why, sizeof why, "%s is an interface - no value is one: a parameter's type (f(a: %s)) or a type parameter's bound (f<T: %s>(a: T), seq<T>)", b, b, b);
        g_x_type_why = why;
        return -1;
    }
    if (si >= 0) return (XT_STRUCT + si) | flags;   // a struct of the script being read
    return -1;
}
// "(a, b) =>" at s: a function's head - its parameters' names (at most 8), each one's type as written after a ':'
// ("" none), and where its body starts; NULL: none. brace: a declaration's - name(a, b) { ... } too (at its {).
// (a: int): point => ..., name(a: int): point { ... }: what it gives back, written after the ) - into g_xps_ret ("" none),
// which the head's xps_lambda reads before the body. A parameter's default, b: int = 2, up to the , or ) it ends at:
// its source in g_xps_def (NULL none) - xps_lambda parses it
internal char g_xps_ret[64];
internal const char *g_xps_def[8], *g_xps_def_end[8];
internal const char *xps_lambda_head(const char *s, char names[8][24], char types[8][64], int *n, int brace) {
    g_xps_ret[0] = 0;
    memset(g_xps_def, 0, sizeof g_xps_def);
    *n = 0;
    s++;
    while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
    if (*s != ')') for (;;) {
        int isref = !strncmp(s, "ref", 3) && (s[3] == ' ' || s[3] == '\t');   // ref name: type - a place, not a value (its type "&type")
        if (isref) for (s += 3; *s == ' ' || *s == '\t'; s++) {}
        if ((!isalpha((unsigned char)*s) && *s != '_') || *n >= 8) return NULL;   // (a 9th: no head - before names[8])
        int k = 0;
        while ((isalnum((unsigned char)*s) || *s == '_') && k < 23) names[*n][k++] = *s++;
        names[*n][k] = 0;
        if (isalnum((unsigned char)*s) || *s == '_' || *s == '.') return NULL;
        types[*n][0] = isref ? '&' : 0;
        types[*n][isref] = 0;
        while (*s == ' ' || *s == '\t') s++;
        if (*s == ':') {   // its type
            for (s++; *s == ' ' || *s == '\t'; s++) {}
            k = isref + xps_type_scan(&s, types[*n] + isref, 64 - isref);
            if (k == isref) return NULL;
        }
        while (*s == ' ' || *s == '\t') s++;
        if (*s == '=' && s[1] != '=' && s[1] != '>') {   // its default
            const char *d = ++s;
            int depth = 0;
            for (;;) {
                if (!*s) return NULL;
                if (x_string_at(s)) {
                    const char *e, *why;
                    if (x_string(s, NULL, 0, &e, &why) < 0) return NULL;
                    s = e;
                    continue;
                }
                if (*s == '(' || *s == '[' || *s == '{') depth++;
                else if (*s == ')' || *s == ']' || *s == '}') { if (!depth) break; depth--; }
                else if (*s == ',' && !depth) break;
                s++;
            }
            g_xps_def[*n] = d;
            g_xps_def_end[*n] = s;
        }
        ++*n;
        while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++;
        if (*s == ',') { s++; while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r') s++; continue; }
        break;
    }
    if (*s != ')') return NULL;
    s++;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == ':') {   // its result's type
        for (s++; *s == ' ' || *s == '\t'; s++) {}
        if (!xps_type_scan(&s, g_xps_ret, sizeof g_xps_ret)) return NULL;
        while (*s == ' ' || *s == '\t') s++;
    }
    if (brace == 2) return *s == '{' || (s[0] == '=' && s[1] == '>') ? NULL : s;   // an interface's method: its head alone
    if (brace && *s == '{') return s;
    return s[0] == '=' && s[1] == '>' ? s + 2 : NULL;
}
// a function from its head (xps_lambda_head's names and types) and its body at body - one value, or { ... }: its
// XN_LAMBDA (an offset), NULL with p->err. method: 1 a struct's (its first parameter is its this), 2 a declared
// function (name(a: int) ...), 0 a function value. A parameter's default (2026-11-21, docs/scripts.md "Functions"):
// a declared function's or a method's, the last ones only, its type written - on the parameter's node (a): the call
// leaves it out, the function works it out at its start (xc_function)
internal struct xnode *xps_lambda(struct xps *p, const char *s, char pn[8][24], char pt[8][64], int npn, const char *body, int method) {
    uintptr_t *it = NULL;
    int n = 0, cap = 0, doff = method == 1, had = 0;
    const char *def[9] = { 0 }, *dend[9] = { 0 };   // (the head's, before the body - a function in a default has its own)
    for (int i = doff; i < npn && i - doff < 8; i++) { def[i] = g_xps_def[i - doff]; dend[i] = g_xps_def_end[i - doff]; }
    int ret = g_xps_ret[0] ? x_type_named(g_xps_ret) : X_ANY;   // (the head's, read before the body: a function in it has its own)
    if (ret < 0) { p->err = 1; p->why = "a function's result: an unknown type - name(a: int): float { ... }, (a: int): text => ..."; return NULL; }
    for (int i = 0; i < npn; i++) {
        for (int j = 0; j < i; j++) if (!strcmp(pn[i], pn[j])) { p->err = 1; p->why = "a parameter twice"; free(it); return NULL; }
        if (x_word_reserved(pn[i]) && !(method == 1 && !i)) { p->err = 1; p->why = "a word of the language is no name"; free(it); return NULL; }
        int isref = pt[i][0] == '&';   // ref name: type (xps_lambda_head)
        int ty = pt[i][isref] ? x_type_named(pt[i] + isref) : X_ANY;
        if (ty < 0 || (isref && !ty)) {
            p->err = 1;
            p->why = isref && !ty ? "ref name: type - a ref's type is written" :
                     "an unknown type - int, float, bool, text, vec2, vec3, vec4, epoch, color, unit_type, function, record, trigger, player, game, slot, a struct; seq<T> a sequence (of no T?), list<T> a list (shared, it grows), T? or null, func<A, B, R> a function of A and B that gives an R";
            free(it);
            return NULL;
        }
        struct xnode *x = xn_new(p, XN_NAME, s);
        if (!x) { free(it); return NULL; }
        x->s = _strdup(pn[i]);
        x->n = ty;   // its type (the check's), X_ANY none written
        x->op = (unsigned char)(isref ? 'r' : 0);   // a ref: a place
        x = XOFF(p, x);   // (as an offset: the parser's block may move while a default is made)
        if (def[i] || had) {
            p->err = 1;
            p->why = !def[i] ? "the parameters with a default come last - f(a: int, b: int = 2)" :
                     !method ? "a default is a declared function's or a method's - a function value is called with all its arguments" :
                     isref ? "a ref has no default - it names the caller's place" :
                     !pt[i][0] ? "a parameter with a default has its type written - b: int = 2" : NULL;
            if (p->why) { free(it); return NULL; }
            p->err = 0;
            had = 1;
            const char *keep = p->s;   // its source parsed where it stands (its line, its mistakes')
            int par = p->par;
            p->s = def[i];
            p->par = 1;
            struct xnode *d;
            xps_expr(p, &d);
            xps_space(p);
            p->par = par;
            if (!p->err && p->s != dend[i]) { p->err = 1; p->why = "a parameter's default: one value - b: int = 2"; }
            if (p->err) { free(it); return NULL; }
            p->s = keep;
            XPTR(p, x)->a = d;
        }
        if (!xps_items_add(p, &it, &n, &cap, x)) { free(it); return NULL; }
    }
    p->s = body;
    int par = p->par;
    struct xnode *b = NULL;
    if (!g_xps_sig) {   // (an interface's method: no body - GENERICS)
        xps_space(p);
        p->par = 0;
        if (*p->s == '{') xps_block(p, &b);
        else xps_expr(p, &b);
        p->par = par;
    }
    if (p->err) { free(it); return NULL; }
    struct xnode *o = xn_new(p, XN_LAMBDA, s);
    if (!o) { free(it); return NULL; }
    o->n_items = n;
    o->items = (struct xnode **)it;
    o->a = b;
    o->n = ret;   // what it gives back, as written (X_ANY: not written)
    return XOFF(p, o);
}
internal void xps_primary(struct xps *p, struct xnode **out) {
    xps_space(p);
    const char *s = p->s;
    struct xnode *o = NULL;
    char pn[8][24], pt[8][64];
    int npn;
    const char *body;
    *out = NULL;
    if (!strncmp(s, "ref", 3) && (s[3] == ' ' || s[3] == '\t')) {   // ref a, ref l[2].pos: a place handed to a ref parameter
        p->s = s + 3;
        struct xnode *pl = NULL;
        if (xps_deeper(p)) xps_primary(p, &pl);
        xps_shallower(p);
        if (p->err) return;
        struct xnode *x = xn_new(p, XN_REF, s);
        if (!x) return;
        x->a = pl;
        x->lvl = 1;
        *out = XOFF(p, x);
        return;
    }
    if (*s == '(' && (body = xps_lambda_head(s, pn, pt, &npn, 0)) != NULL) {   // a function: (a: int, b) => value, or => { ... }
        if (!(o = xps_lambda(p, s, pn, pt, npn, body, 0))) return;
    } else if (*s == '(') {
        p->s++;
        p->par++;
        xps_expr(p, &o);
        p->par--;
        if (!p->err && !xps_op(p, ")")) p->err = 1;
        if (p->err) return;
    } else if (x_string_at(s)) {   // a string: its text, a template (filled when worked out)
        const char *e, *why, *q;
        if (x_string(s, NULL, 0, &e, &why) < 0) { p->err = 1; return; }   // (the file check tells it: tl_value)
        size_t span = (size_t)(e - s);   // the source's length: its text is no longer
        // ("a" "b"): strings side by side in ( ) are one - a long text over several lines (in a list a missing comma
        // stays a mistake)
        for (q = e; p->par; ) {
            while (*q == ' ' || *q == '\t' || *q == '\n' || *q == '\r') q++;
            if (!x_string_at(q)) break;
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
    } else if (*s == '{') {   // { name = value, [key] = value, ... }: a record, or what the place it goes into is (a dict,
        int par = p->par, n = 0, cap = 0;   // a struct: x_check XN_REC) - a comma after each entry (the last's may be left out)
        uintptr_t *it = NULL;
        p->s++;
        p->par = 0;
        for (;;) {
            if (xps_op(p, "}")) break;
            const char *ks = p->s;
            char key[40];
            int k = 0;
            struct xnode *kx = NULL;   // [key] = value: a key worked out (a dict's) - the entry's s ""
            if (*ks == '[') {
                p->s++;
                p->par++;
                xps_expr(p, &kx);
                p->par--;
                if (!p->err && !xps_op(p, "]")) { p->err = 1; p->why = "{ [key] = value }: a ] missing"; }
                if (p->err) break;
            } else {
                if (!isalpha((unsigned char)*ks) && *ks != '_') { p->err = 1; p->why = "{ name = value } or { [key] = value }"; break; }
                while ((isalnum((unsigned char)*p->s) || *p->s == '_') && k < 39) key[k++] = *p->s++;
                if (isalnum((unsigned char)*p->s) || *p->s == '_') { p->err = 1; p->why = "a name over 39 characters"; break; }
            }
            key[k] = 0;
            for (int i = 0; i < n && !p->err && k; i++)
                if (!strcmp(XPTR(p, (struct xnode *)it[i])->s, key)) { p->err = 1; p->why = "{ ... }: a name twice"; }
            if (p->err) break;
            if (!xps_op(p, "=") || *p->s == '=') { p->err = 1; p->why = kx ? "{ [key] = value }" : "{ name = value }"; break; }
            struct xnode *x;
            xps_expr(p, &x);
            if (p->err) break;
            struct xnode *m = xn_new(p, XN_ASSIGN, ks);
            if (!m) break;
            m->s = _strdup(key);
            m->a = x;
            m->b = kx;
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
        int fl = 0;   // written as a float: 5.0, .72, 1e3 - or with a unit: 70% (0.7), 20s, 19.5min (seconds)
        for (const char *q = s; q < e && !(s[0] == '0' && (s[1] == 'x' || s[1] == 'X')); q++) fl |= *q == '.' || *q == 'e' || *q == 'E';
        if (*e == 'x' && !fl && e[1] >= '0' && e[1] <= '9') {   // 1920x1080: a size, vec(1920, 1080)
            char *f;
            double m = strtod(e + 1, &f);
            uintptr_t *it = NULL;
            int na = 0, cap = 0;
            for (int i = 0; i < 2; i++) {
                struct xnode *x = xn_new(p, XN_NUM, i ? e + 1 : s);
                if (!x) { free(it); return; }
                x->n = i ? m : n;
                x->op = 'i';
                if (!xps_items_add(p, &it, &na, &cap, XOFF(p, x))) { free(it); return; }
            }
            p->s = f;
            o = xn_new(p, XN_CALL, s);
            if (!o) { free(it); return; }
            o->s = _strdup("vec");
            o->n_items = na;
            o->items = (struct xnode **)it;
            o = XOFF(p, o);
        } else {
            if (*e == '%') { n /= 100; e++; fl = 1; }   // 70% = 0.7
            else if (*e == 's' && !isalnum((unsigned char)e[1]) && e[1] != '_') { e++; fl = 1; }   // 20s: seconds
            else if (!strncmp(e, "min", 3) && !isalnum((unsigned char)e[3]) && e[3] != '_') { n *= 60; e += 3; fl = 1; }   // 19.5min
            p->s = e;
            o = xn_new(p, XN_NUM, s);
            if (!o) return;
            o->n = n;
            o->op = (unsigned char)(fl ? 'f' : 'i');   // its type: a float or an int (the check's)
            o = XOFF(p, o);
        }
    } else if (isalpha((unsigned char)*s) || *s == '_') {
        char name[40];
        int n = 0;
        while ((isalnum((unsigned char)*s) || *s == '_' || (*s == '.' && isalpha((unsigned char)s[1]))) && n < 39) name[n++] = *s++;
        name[n] = 0;
        p->s = s;
        if (x_ieq(name, "true") || x_ieq(name, "false")) {
            o = xn_new(p, XN_NUM, s - n);
            if (!o) return;
            o->n = x_ieq(name, "true");
            o->op = 'b';   // a bool
            o = XOFF(p, o);
        } else if (x_ieq(name, "null")) {   // nothing there: lord == null
            o = xn_new(p, XN_NUM, s - n);
            if (!o) return;
            o->op = 'n';
            o = XOFF(p, o);
        } else if (!strcmp(name, "list") && *p->s == '<') {   // list<T>(...): a new list (LISTS) - its type in n, what it is made of in a
            const char *q = s - n;
            char t[96];
            int ty = xps_type_scan(&q, t, sizeof t) ? x_type_named(t) : -1;
            if (ty < 0 || (ty & X_NULLABLE) || !xt_is_hl(ty)) { p->err = 1; p->why = "list<T>(...) - T a type: list<int>(), list<point>([a, b])"; return; }
            p->s = q;
            xps_space(p);
            if (*p->s != '(') { p->err = 1; p->why = "list<T>(...): a new list - list<int>() an empty one, list<int>([1, 2]) of these, list<int>(s) of a seq's or a list's items"; return; }
            p->s++;
            xps_space(p);
            struct xnode *d = NULL;
            if (*p->s != ')') { p->par++; xps_expr(p, &d); p->par--; }
            if (!p->err && !xps_op(p, ")")) { p->err = 1; p->why = "list<T>(...): one value - a seq or a list of its items, or none"; }
            if (p->err) return;
            o = xn_new(p, XN_HLIST, s - n);
            if (!o) return;
            o->n = ty;
            o->a = d;
            o = XOFF(p, o);
        } else if (!strcmp(name, "dict") && *p->s == '<') {   // dict<K, V>(default): a new dict (DICTS) - its type in n
            char tb[96] = "dict";
            const char *q = p->s;
            xps_type_scan(&q, tb + 4, (int)sizeof tb - 4);
            int ty = x_type_named(tb);
            if (ty < 0 || (ty & X_NULLABLE) || !xt_is_dict(ty)) {
                char *in = tb + 5, *c = strchr(in, ',');
                if (c) *c = 0;
                p->err = 1;
                p->why = c && tb[strlen(tb) - 1] != '?' && !xt_dict_key_ok(x_type_named(in)) ? "dict<K, V>: a key is an int, a text or an object (player ...)" :
                         "dict<K, V>(default) - K an int, a text or an object (player ...), V a type: dict<player, int>(0), dict<text, point>(point(0, 0))";
                return;
            }
            p->s = q;
            xps_space(p);
            if (*p->s != '(') { p->err = 1; p->why = "dict<K, V>(default) - the default: what a key without an entry holds"; return; }
            p->s++;
            p->par++;
            struct xnode *d;
            xps_expr(p, &d);
            p->par--;
            struct xnode *en = NULL;   // dict<K, V>(default, { key = value, ... }): its entries
            if (!p->err && xps_op(p, ",")) { p->par++; xps_expr(p, &en); p->par--; }
            if (!p->err && !xps_op(p, ")")) { p->err = 1; p->why = "dict<K, V>(default) or dict<K, V>(default, { ... }): a ) missing"; }
            if (p->err) return;
            o = xn_new(p, XN_DICT, s - n);
            if (!o) return;
            o->n = ty;
            o->a = d;
            o->b = en;
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
        XPTR(p, o)->end = p->s;   // (what came so far ends here: an item or a member may follow)
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
    if (xps_word(p, "not")) { p->err = 1; p->why = "not x is written !x"; return; }
    int neg = xps_op(p, "-"), inv = !neg && *p->s == '!' && p->s[1] != '=' && (p->s++, 1);   // !x (x != y: the comparison's)
    if (neg || inv) {
        struct xnode *a = NULL;
        if (xps_deeper(p)) xps_unary(p, &a);
        xps_shallower(p);
        if (p->err) return;
        struct xnode *o = xn_new(p, neg ? XN_NEG : XN_NOT, at);
        if (!o) return;
        o->a = a;
        o->lvl = 1;
        *out = XOFF(p, o);
        return;
    }
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
// a && b, a || b - as in C#, TypeScript, C (2026-11-21, the player: "the difference between & and && starts to
// matter"): the words and, or, not are mistakes that name the operator; a single & or | is no operator (yet: kept
// for bits)
internal int xps_logic(struct xps *p, char c) {   // && (c '&') or || next: taken
    xps_space(p);
    if (xps_word(p, c == '&' ? "and" : "or")) { p->err = 1; p->why = c == '&' ? "a and b is written a && b" : "a or b is written a || b"; return 0; }
    if (*p->s != c) return 0;
    if (p->s[1] != c) { p->err = 1; p->why = c == '&' ? "& alone is no operator - a && b: both" : "| alone is no operator - a || b: either"; return 0; }
    p->s += 2;
    return 1;
}
internal void xps_and(struct xps *p, struct xnode **out) {
    xps_cmp(p, out);
    while (!p->err && xps_logic(p, '&')) {
        struct xnode *b;
        xps_cmp(p, &b);
        if (p->err) return;
        *out = xps_bin(p, XN_AND, 0, *out, b);
    }
}
internal void xps_or(struct xps *p, struct xnode **out) {
    xps_and(p, out);
    while (!p->err && xps_logic(p, '|')) {
        struct xnode *b;
        xps_and(p, &b);
        if (p->err) return;
        *out = xps_bin(p, XN_OR, 0, *out, b);
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
internal void xps_cond(struct xps *p, struct xnode **out) {   // c ? a : b, c ? a (nothing when c doesn't hold)
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
internal void xps_expr(struct xps *p, struct xnode **out) {   // an expression, one level further in (X_DEEP)
    *out = NULL;
    if (xps_deeper(p)) xps_cond(p, out);
    xps_shallower(p);
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
// a type as written that is an interface's name alone (or with ?, a ref's &): the interface's number, -1 none (GENERICS)
internal int xps_iface_named(const char *t) {
    char b[40];
    int k = 0;
    if (*t == '&') t++;
    while ((isalnum((unsigned char)*t) || *t == '_') && k < 39) b[k++] = *t++;
    b[k] = 0;
    if (*t == '?') t++;
    int si = k && !*t ? xst_find(b) : -1;
    return si >= 0 && g_xst[si].iface ? si : -1;
}
// <T: positioned, U: other> at s: the type parameters' names and their bounds as written - after the > (NULL: not of
// that form: a < in an expression)
internal const char *xps_tparams(const char *s, char tn[XG_MAX][24], char tb[XG_MAX][40], int *n) {
    *n = 0;
    for (s++;;) {
        int k = 0;
        while (*s == ' ') s++;
        while ((isalnum((unsigned char)*s) || *s == '_') && k < 23) tn[*n][k++] = *s++;
        tn[*n][k] = 0;
        while (*s == ' ') s++;
        if (!k || isdigit((unsigned char)tn[*n][0]) || *s++ != ':' || *n == XG_MAX) return NULL;
        while (*s == ' ') s++;
        k = 0;
        while ((isalnum((unsigned char)*s) || *s == '_') && k < 39) tb[*n][k++] = *s++;
        tb[*n][k] = 0;
        while (*s == ' ') s++;
        if (!k) return NULL;
        ++*n;
        if (*s == ',') { s++; continue; }
        if (*s != '>') return NULL;
        return s + 1;
    }
}
// a declaration's type parameters (name, for the messages): tn / tb its <T: bound> as written (*ntp), each bound an
// interface, each name no type's; then each parameter of an interface's type a type parameter of its own ("#i" - its
// type in pt written so), but in an interface's own declaration (self its number, else -1) its own name: the type
// that has it. Their types the parse's (g_xps_tp; a copy's: its call's, g_xg_bind), tt their bounds. 0: p->err
internal int xps_gen_head(struct xps *p, const char *name, char tn[XG_MAX][24], char tb[XG_MAX][40], int *ntp, char pt[8][64], int npn, int self, int tt[XG_MAX]) {
    for (int j = 0; j < *ntp; j++) {   // each bound an interface; each name no type's
        int si = xst_find(tb[j]), dup = 0;
        for (int i = 0; i < j; i++) dup |= !strcmp(tn[i], tn[j]);
        p->err = 1;
        if (si < 0 || !g_xst[si].iface) { static char w[160]; snprintf(w, sizeof w, "%s<%s: %s>: an interface's name after the : (interface %s { ... })", name, tn[j], tb[j], tb[j]); p->why = w; return 0; }
        if (dup) { p->why = "a type parameter twice"; return 0; }
        if (x_type_named(tn[j]) >= 0 || xst_find(tn[j]) >= 0) { p->why = "a type parameter's name: a type has it already - T, U ..."; return 0; }
        p->err = 0;
        tt[j] = XT_STRUCT + si;
    }
    for (int i = 0; i < npn; i++) {   // f(a: positioned): a parameter of an interface - a type parameter of its own ("#i")
        int si = xps_iface_named(pt[i]);
        if (si < 0 || si == self) continue;
        if (*ntp == XG_MAX) { p->err = 1; p->why = "more than 4 type parameters"; return 0; }
        int isref = pt[i][0] == '&', nul = pt[i][strlen(pt[i]) - 1] == '?';
        snprintf(tn[*ntp], sizeof tn[0], "#%d", i);
        tt[(*ntp)++] = XT_STRUCT + si;
        snprintf(pt[i], sizeof pt[0], "%s#%d%s", isref ? "&" : "", i, nul ? "?" : "");
    }
    for (int j = 0; j < *ntp; j++) { lstrcpynA(g_xps_tp[j].name, tn[j], sizeof g_xps_tp[0].name); g_xps_tp[j].type = g_xg_bind ? g_xg_bind[j] : tt[j]; }
    g_xps_ntp = *ntp;
    return 1;
}
// a generic function's or method's record (GENERICS), for its calls' copies: at where its declaration starts (a
// method's: its name), sname its struct's name ("" a function), ret its result's type as written
internal struct xgen *xps_gen_new(struct xps *p, const char *at, const char *sname, int ntp, char tn[XG_MAX][24], const int *tt, int npn, char pt[8][64], const char *ret) {
    struct xgen *g = calloc(1, sizeof *g);
    if (!g) { p->err = 1; p->why = "no memory"; return NULL; }
    g->n = ntp;
    for (int j = 0; j < ntp; j++) { lstrcpynA(g->name[j], tn[j], sizeof g->name[0]); g->iface[j] = tt[j]; }
    g->np = npn;
    for (int i = 0; i < npn; i++) lstrcpynA(g->ptype[i], pt[i], sizeof g->ptype[0]);
    lstrcpynA(g->rtype, ret, sizeof g->rtype);
    lstrcpynA(g->sname, sname, sizeof g->sname);
    g->src0 = p->src0;
    g->at = at;
    return g;
}
internal void xps_stmt(struct xps *p, struct xnode **out) {
    xps_space(p);
    const char *at = p->s;
    char name[40];
    *out = NULL;
    {   // name(a: int) => value, name(a: int) { ... }: a function declared - a let of it (op 'D', on it and its function),
        // moved to the script's top (xps_stmts): known in the whole file, never set anew
        const char *q = p->s;
        int k = 0;
        while ((isalnum((unsigned char)*q) || *q == '_') && k < 39) name[k++] = *q++;
        name[k] = 0;
        char pn[8][24], pt[8][64], tn[XG_MAX][24], tb[XG_MAX][40];
        int npn, ntp = 0;
        if (k && *q == '<') {   // name<T: positioned, U: other>(...): its type parameters (GENERICS)
            const char *e = xps_tparams(q, tn, tb, &ntp);
            if (e && *e == '(') q = e; else ntp = -1;   // (none of the form: no declaration - what it is the rest tells)
        }
        const char *body = k && ntp >= 0 && !isdigit((unsigned char)name[0]) && *q == '(' && !x_word_reserved(name) ? xps_lambda_head(q, pn, pt, &npn, 1) : NULL;
        if (body) {
            int tt[XG_MAX];
            char ret[64];
            lstrcpynA(ret, g_xps_ret, sizeof ret);
            if (!xps_gen_head(p, name, tn, tb, &ntp, pt, npn, -1, tt)) return;   // (a copy's types: its call's - x_parse_copy)
            struct xnode *f = xps_lambda(p, q, pn, pt, npn, body, 2);
            g_xps_ntp = 0;
            if (!f) return;
            if (ntp && !g_xg_bind) {   // the function as written: generic - its copies are made by its calls
                struct xgen *g = xps_gen_new(p, at, "", ntp, tn, tt, npn, pt, ret);
                if (!g) return;
                XPTR(p, f)->gen = g;
            }
            XPTR(p, f)->op = 'D';
            struct xnode *o = xn_new(p, XN_LET, at);
            if (!o) return;
            o->s = _strdup(name);
            o->a = f;
            o->n = X_ANY;
            o->op = 'D';
            *out = XOFF(p, o);
            return;
        }
    }
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
    int isref = xps_word(p, "ref");   // ref q = l[2], ref q: point = a.pos - a second name for a place (a let, op 'r')
    int let = isref || xps_word(p, "let");
    const char *nm = p->s;
    if (xps_name(p, name, let)) {   // [let] name [: type] = ..., name += ...
        xps_space(p);
        int ty = X_ANY;
        if (let && *p->s == ':') {   // let n: float = 5 - its type as written
            char t[64];
            for (p->s++; *p->s == ' ' || *p->s == '\t'; p->s++) {}
            xps_type_scan(&p->s, t, sizeof t);
            if ((ty = x_type_named(t)) < 0) {
                p->err = 1;
                p->why = "let name: type = ... - an unknown type (int, float, bool, text, vec2, vec3, vec4, epoch, color, unit_type, function, record, trigger, player, game, slot, a struct; seq<T> a sequence (of no T?), list<T> a list (shared, it grows), T? or null, func<A, B, R> a function of A and B that gives an R)";
                return;
            }
            xps_space(p);
        }
        const char *r = p->s;
        int op = r[1] == '=' && (*r == '+' || *r == '-' || *r == '*' || *r == '/') ? *r : 0;
        if ((*r == '=' && r[1] != '=' && r[1] != '>') || op) {
            if (let && op) { p->err = 1; p->why = "let name = value (a new variable has no value to add to)"; return; }
            p->s = r + (op ? 2 : 1);
            struct xnode *v;
            if (isref) xps_primary(p, &v);   // (a place: a name, its fields and items - no value worked out)
            else xps_expr(p, &v);
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
            o->n = ty;   // a let's type as written (X_ANY: its value's)
            o->op = (unsigned char)(isref ? 'r' : 0);
            *out = XOFF(p, o);
            return;
        }
        if (let) { p->err = 1; p->why = isref ? "ref name = a place (a variable, a field of one, a list's item)" : "let name = value"; return; }
        p->s = nm;
    }
    if (p->err) return;
    if (let) { p->err = 1; p->why = isref ? "ref name = a place (a variable, a field of one, a list's item)" : "let name = value"; return; }
    xps_expr(p, out);
    if (p->err || !*out) return;
    const char *r = p->s;   // an object's member set: slot.persona = ..., game.slots[2].color = ..., p.gold += 100
    while (*r == ' ' || *r == '\t') r++;
    int op = r[1] == '=' && (*r == '+' || *r == '-' || *r == '*' || *r == '/') ? *r : 0;
    if (!op && (*r != '=' || r[1] == '=')) return;
    struct xnode *t = XPTR(p, *out), *obj, *v;
    char member[40];
    if (t->k == XN_INDEX) {   // d[k] = value, d[k] += 1: a dict's entry - the dict and the key worked out once (the op kept)
        struct xnode *ta = t->a, *tb = t->b;   // (offsets: the nodes move as they grow)
        p->s = r + (op ? 2 : 1);
        xps_expr(p, &v);
        if (p->err) return;
        struct xnode *o = xn_new(p, XN_SETI, at);
        if (!o) return;
        o->a = ta; o->b = tb; o->c = v;
        o->op = (unsigned char)op;
        *out = XOFF(p, o);
        return;
    }
    const char *dot = t->k == XN_NAME ? strrchr(t->s, '.') : NULL;
    if (t->k == XN_MEMBER) { obj = t->a; lstrcpynA(member, t->s, sizeof member); }
    else if (dot && dot - t->s < 40) {   // a.b.c = ...: the object a.b
        char head[40];
        lstrcpynA(head, t->s, (int)(dot - t->s) + 1);
        lstrcpynA(member, dot + 1, sizeof member);
        const char *at0 = t->at;
        struct xnode *h = xn_new(p, XN_NAME, at0);
        if (!h) return;
        h->s = _strdup(head);
        h->lvl = 1;
        obj = XOFF(p, h);
    } else if (t->k == XN_NAME && !strcmp(t->s, "this") && !op) {   // this = ...: a method's struct whole (a word of the language: no name above)
        p->s = r + 1;
        xps_expr(p, &v);
        if (p->err) return;
        struct xnode *o = xn_new(p, XN_ASSIGN, at);
        if (!o) return;
        o->s = _strdup("this");
        o->a = v;
        *out = XOFF(p, o);
        return;
    } else { p->err = 1; p->why = "= sets a variable (name = ...) or an object's member (slot.persona = ...)"; return; }
    p->s = r + (op ? 2 : 1);
    xps_expr(p, &v);
    if (p->err) return;
    // p.gold += 100: p.gold = p.gold + 100 (its object a name of its own); l[1].hp -= 50: the set keeps the op - its
    // object is worked out once (a call in it not made twice) and is in the tree once
    if (op && t->k != XN_MEMBER && !(v = xps_bin(p, op == '+' || op == '-' ? XN_ADD : XN_MUL, op, *out, v))) return;
    struct xnode *o = xn_new(p, XN_SETM, at);
    if (!o) return;
    o->a = obj; o->b = v;
    if (t->k == XN_MEMBER) o->op = (unsigned char)op;
    o->s = _strdup(member);
    *out = XOFF(p, o);
}
// struct name { field: type [= default] ... method(a: int) { ... } ... } - after the word struct (STRUCTS AND REFS):
// an XN_STRUCT, s its name, n its type; its members in items, one a line: a field an XN_NAME (op 'F': s, n its type, a
// its default or NULL), a method its function (op 'M', s its name) with this, a ref of the struct, as its first parameter
// a struct's or an interface's (iface) method after its name mn (written at ms) - p->s at its ( or its <: name(a: int)
// { ... }, name(a: int) => value, an interface's name(a: int): type alone; generic (GENERICS) with <T: positioned> or a
// parameter of an interface's type - its copies made by its calls (xg_mcall). Its function with this (a ref of the
// struct sname, of type sty) first: an offset, NULL p->err
internal struct xnode *xps_method(struct xps *p, const char *sname, int sty, const char *mn, const char *ms, int iface) {
    char pn[8][24], pt[8][64], qn[8][24], qt[8][64], tn[XG_MAX][24], tb[XG_MAX][40], ret[64];
    int npn, ntp = 0, tt[XG_MAX];
    const char *q = p->s;
    if (*q == '<' && (!(q = xps_tparams(q, tn, tb, &ntp)) || *q != '(')) {
        p->err = 1;
        p->why = iface ? "an interface's generic method: name<T: an interface>(a: T): type" : "a generic method: name<T: an interface>(a: T) { ... }";
        return NULL;
    }
    const char *body = xps_lambda_head(q, pn, pt, &npn, iface ? 2 : 1);
    if (!body || npn > 7) {
        p->err = 1;
        p->why = body ? "a method takes at most 7 parameters" : iface ? "an interface's method: its head alone - name(a: int): type, no code" : "a method: name(a: int) { ... } or name(a: int) => value";
        return NULL;
    }
    for (int i = 0; i < npn; i++) {
        if (iface && g_xps_def[i]) { p->err = 1; p->why = "an interface's method: no parameter's default (the struct's own method has it)"; return NULL; }
        if (!strcmp(pn[i], "this")) { p->err = 1; p->why = "this is the method's own struct - no parameter's name"; return NULL; }
    }
    lstrcpynA(ret, g_xps_ret, sizeof ret);
    if (!xps_gen_head(p, mn, tn, tb, &ntp, pt, npn, iface ? sty - XT_STRUCT : -1, tt)) return NULL;
    lstrcpynA(qn[0], "this", sizeof qn[0]);
    snprintf(qt[0], sizeof qt[0], "&%.30s", sname);   // (a struct's name: 30 letters at most)
    for (int i = 0; i < npn; i++) { memcpy(qn[i + 1], pn[i], sizeof qn[0]); memcpy(qt[i + 1], pt[i], sizeof qt[0]); }
    g_xps_sig = iface;
    struct xnode *x = xps_lambda(p, q, qn, qt, npn + 1, body, 1);
    g_xps_sig = 0;
    g_xps_ntp = 0;
    if (!x) return NULL;
    XPTR(p, x)->op = 'M';
    XPTR(p, x)->s = _strdup(mn);
    if (ntp && !g_xg_bind && !(XPTR(p, x)->gen = xps_gen_new(p, ms, sname, ntp, tn, tt, npn, pt, ret))) return NULL;
    return x;
}
// struct name { ... } - and interface name { ... } (iface: GENERICS - its methods without code, its fields without
// defaults, its own name a type in it); struct name: a, b { ... } promises the interfaces a and b (its node's cs)
internal void xps_struct_in(struct xps *p, struct xnode **out, const char *at, int iface) {
    char name[40], mn[40], promise[120] = "";
    uintptr_t *it = NULL;
    int n = 0, cap = 0, first = 1;
    *out = NULL;
    if (!xps_name(p, name, 1)) { if (!p->err) { p->err = 1; p->why = iface ? "interface name { field: type  method(a: type): type ... }" : "struct name { field: type ... }"; } return; }
    int ty = x_type_named(name);
    if (strlen(name) > 30) { p->err = 1; p->why = iface ? "an interface's name over 30 characters" : "a struct's name over 30 characters"; return; }
    if (ty < XT_STRUCT) { p->err = 1; p->why = ty < 0 ? "more than 100 structs and interfaces in a script" : iface ? "an interface's name: a type has it already" : "a struct's name: a type has it already"; return; }
    xps_space(p);
    if (!iface && *p->s == ':') {   // struct point: ipoint, other { ... }: the interfaces it promises
        int k = 0;
        for (p->s++; *p->s && *p->s != '{' && *p->s != '\n' && k < (int)sizeof promise - 1; p->s++) promise[k++] = *p->s;
        promise[k] = 0;
        xps_space(p);
    }
    if (*p->s != '{' || xps_newline_before(p)) { p->err = 1; p->why = iface ? "interface name { ... } - the { on the interface's line" : "struct name { ... } - the { on the struct's line"; return; }
    p->s++;
    for (;; first = 0) {
        xps_space(p);
        if (*p->s == '}') { p->s++; break; }
        if (!*p->s) { p->err = 1; p->why = "a struct { without its }"; break; }
        if (!first && !xps_newline_before(p)) { p->err = 1; p->why = "a struct's members: one a line"; break; }
        const char *ms = p->s;
        if (!xps_name(p, mn, 1)) { if (!p->err) { p->err = 1; p->why = iface ? "an interface's member: field: type, or method(a: int): type" : "a struct's member: field: type, or method(a: int) { ... }"; } break; }
        for (int i = 0; i < n && !p->err; i++)
            if (!strcmp(XPTR(p, (struct xnode *)it[i])->s, mn)) { p->err = 1; p->why = iface ? "an interface's member twice" : "a struct's member twice"; }
        if (p->err) break;
        struct xnode *x;
        if (*p->s == '(' || *p->s == '<') {   // a method: its function, this first
            if (!(x = xps_method(p, name, ty, mn, ms, iface))) break;
        } else {   // a field: its type, its default
            xps_space(p);
            if (*p->s != ':') { p->err = 1; p->why = "a struct's field: name: type (int, float, text, a struct ...)"; break; }
            char t[64];
            for (p->s++; *p->s == ' ' || *p->s == '\t'; p->s++) {}
            int ft = xps_type_scan(&p->s, t, sizeof t) ? x_type_named(t) : -1;
            if (ft <= 0) { p->err = 1; p->why = "a struct's field: name: type - an unknown type (int, float, bool, text, vec2 ..., player, a struct; seq<T> a sequence (of no T?), list<T> a list (shared, it grows), T? or null)"; break; }
            struct xnode *d = NULL;
            const char *r = p->s;
            while (*r == ' ' || *r == '\t') r++;
            if (*r == '=' && r[1] != '=' && iface) { p->err = 1; p->why = "an interface's field: name: type - no default (a struct that has it gives its own)"; break; }
            if (*r == '=' && r[1] != '=') { p->s = r + 1; xps_expr(p, &d); if (p->err) break; }
            struct xnode *f = xn_new(p, XN_NAME, ms);
            if (!f) break;
            f->s = _strdup(mn);
            f->n = ft;
            f->a = d;
            f->op = 'F';
            x = XOFF(p, f);
        }
        if (!xps_items_add(p, &it, &n, &cap, x)) break;
    }
    if (p->err) { free(it); return; }
    struct xnode *o = xn_new(p, XN_STRUCT, at);
    if (!o) { free(it); return; }
    o->s = _strdup(name);
    o->n = ty;
    o->n_items = n;
    o->items = (struct xnode **)it;
    o->op = (unsigned char)(iface ? 'I' : 0);
    o->cs = promise[0] ? _strdup(promise) : NULL;
    *out = XOFF(p, o);
}
internal void xps_struct(struct xps *p, struct xnode **out, const char *at, int iface) {
    int keep = g_xps_in_iface;
    g_xps_in_iface = iface;
    xps_struct_in(p, out, at, iface);
    g_xps_in_iface = keep;
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
        const char *st = p->s;
        int iface = 0;
        if (xps_word(p, "struct") || (iface = xps_word(p, "interface")) != 0) {
            if (brace || p->src0 != at) { p->s = st; p->err = 1; p->why = iface ? "an interface is declared at the script's top" : "a struct is declared at the script's top"; free(it); return; }
            xps_struct(p, &x, st, iface);
        } else xps_stmt(p, &x);
        if (!p->err && brace && x && XPTR(p, x)->k == XN_LET && XPTR(p, x)->op == 'D') {
            p->s = st;
            p->err = 1;
            p->why = "a function is declared at the script's top - inside a block: let name = (a: int) => ...";
        }
        if (p->err || !xps_items_add(p, &it, &n, &cap, x)) { free(it); return; }
    }
    for (int i = 0, d = 0; i < n; i++) {   // the declared functions first, in their order: known (and made) before the rest runs
        struct xnode *x = XPTR(p, (struct xnode *)it[i]);
        if (x->k != XN_LET || x->op != 'D') continue;
        uintptr_t keep = it[i];
        memmove(it + d + 1, it + d, (size_t)(i - d) * sizeof *it);
        it[d++] = keep;
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
    *out = NULL;
    if (xps_deeper(p)) xps_stmts(p, out, 1, at);
    xps_shallower(p);
}

// A COMPILED SOURCE: its tree (pointers now), its deepest level, whether it is one at all (why not, where)
// code: a text's instructions, made of the tree when it is first worked out (x_unit_code); src: the source it points into
struct xcode;
struct xunit { struct xnode *root; int depth, ok, line; const char *why; struct xcode *code; const char *src; };
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
    g_x_type_why = NULL;
    struct xnode *root = NULL;
    if (prog) xst_scan(src);   // its structs' names: a type is written before its struct too
    if (prog) xps_stmts(&p, &root, 0, src);
    else xps_expr(&p, &root);
    xps_space(&p);
    struct xunit u = { NULL, 0, 0, 0, NULL, NULL, src };
    if (!p.err && *p.s) { p.err = 1; if (!p.why) p.why = "something after the expression"; }
    if (p.err || !root) {   // (what was built before the mistake is lost - a broken source is rare)
        u.line = xps_line(&p, p.s);
        free(p.mem);
        u.why = p.why && g_x_type_why && strstr(p.why, "unknown type") ? g_x_type_why : p.why;   // (an interface written as a value's type)
        g_x_type_why = NULL;
        return u;
    }
    g_x_type_why = NULL;
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
// A GENERIC FUNCTION'S COPY (GENERICS): its declaration parsed again from where it is written, each type parameter
// the type in bind (in g's order) - a declared function's let of its own, its places the original's (its lines, the
// language server's columns); a method's: its function (op 'M'). NULL: it doesn't parse so (a type it can't take -
// told by its check then)
internal struct xnode *x_parse_copy(const struct xgen *g, const int *bind) {
    struct xps p;
    memset(&p, 0, sizeof p);
    p.src0 = p.lc = g->src0;
    p.s = g->at;
    p.line = 1;
    struct xnode *root = NULL;
    g_xg_bind = bind;
    char mn[40];
    if (!g->sname[0]) xps_stmt(&p, &root);
    else if (xps_name(&p, mn, 1)) root = xps_method(&p, g->sname, x_type_named(g->sname), mn, g->at, 0);
    g_xg_bind = NULL;
    g_x_type_why = NULL;
    if (p.err || !root) { free(p.mem); return NULL; }
    struct xnode *base = malloc(p.used);
    if (!base) { free(p.mem); return NULL; }
    memcpy(base, p.mem, p.used);
    free(p.mem);
    struct xnode *r = x_relocate(&p, root, base);
    int bad = 0;
    x_depth_of(r, &bad);
    return bad || (g->sname[0] ? r->k != XN_LAMBDA || r->op != 'M' : r->k != XN_LET || r->op != 'D') ? NULL : r;
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
// THE BUILTIN FUNCTIONS: name, arguments lo..hi, their number (x_fn_id; also listed by the language server, check/lsp.c)
internal const struct { const char *name; signed char lo, hi, id; } k_x_fns[] = {
    { "chance", 1, 1, 1 }, { "min", 1, 127, 2 }, { "max", 1, 127, 2 }, { "abs", 1, 1, 3 }, { "round", 1, 1, 4 }, { "rnd", 0, 1, 5 },
    { "floor", 1, 1, 6 }, { "len", 1, 1, 7 }, { "first", 1, 1, 8 }, { "item", 2, 2, 9 }, { "contains", 2, 2, 10 },
    { "map_range", 3, 3, 11 }, { "str", 1, 1, 12 }, { "number", 1, 1, 13 }, { "log2", 1, 1, 14 }, { "pick", 1, 1, 15 },
    { "vec", 2, 4, 16 }, { "rgb", 3, 3, 17 }, { "distance", 2, 2, 18 }, { "has", 2, 2, 19 }, { "int", 1, 1, 20 }, { "ceil", 1, 1, 21 } };
#define N_X_FNS (int)(sizeof k_x_fns / sizeof *k_x_fns)
internal int x_fn_id(const char *name, int na) {
    for (int i = 0; i < N_X_FNS; i++) if (na >= k_x_fns[i].lo && na <= k_x_fns[i].hi && x_ieq(name, k_x_fns[i].name)) return k_x_fns[i].id;
    return 0;
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
struct xscope { const struct xscope *up; const char *name[64]; int n; unsigned char decl[64]; int blk; short kind[64]; const struct xnode *fn[64]; };
// 1: there (*own: the running function's - this scope or a block it is in), 2: the same function's, its let not passed yet
internal int xsc_find(const struct xscope *s, const char *name, size_t len, int *own) {
    for (int crossed = 0; s; s = s->up) {
        for (int i = 0; i < s->n; i++)
            if (strlen(s->name[i]) == len && !strncmp(s->name[i], name, len)) {
                if (own) *own = !crossed;
                return !crossed && !(s->decl[i] & 1) ? 2 : 1;
            }
        if (!s->blk) crossed = 1;
    }
    return 0;
}
// decl's bits: 1 its let passed, 2 a ref (a place: a ref parameter, a ref local, a method's this), 4 a for's variable
// (the loop's copy of an item). A name's bits (0: no variable); *outer: it is a function's around the one being checked
internal int xsc_bits(const struct xscope *s, const char *name, size_t len, int *outer) {
    for (int crossed = 0; s; s = s->up) {
        for (int i = 0; i < s->n; i++)
            if (strlen(s->name[i]) == len && !strncmp(s->name[i], name, len)) { if (outer) *outer = crossed; return s->decl[i] | 0x80; }
        if (!s->blk) crossed = 1;
    }
    return 0;
}
internal void xsc_add(struct xscope *s, const char *name) {
    for (int i = 0; i < s->n; i++) if (!strcmp(s->name[i], name)) return;
    if (s->n < 64) { s->decl[s->n] = 1; s->name[s->n++] = name; }
}
internal void xsc_pass(struct xscope *s, const char *name, int kind, const struct xnode *fn) {   // its let reached: what it is
    for (int i = 0; i < s->n; i++) if (!strcmp(s->name[i], name)) { s->decl[i] |= 1; s->kind[i] = (short)kind; s->fn[i] = fn; }
}
// the function a variable holds as far as the check knows (its let's (a, b) => ..., not set since): NULL not known
internal const struct xnode *xsc_fn(const struct xscope *s, const char *name, size_t len) {
    for (; s; s = s->up)
        for (int i = 0; i < s->n; i++) if (strlen(s->name[i]) == len && !strncmp(s->name[i], name, len)) return s->fn[i];
    return NULL;
}
internal void xsc_fn_set(const struct xscope *s, const char *name, const struct xnode *fn) {   // (a variable set anew)
    for (; s; s = s->up)
        for (int i = 0; i < s->n; i++) if (!strcmp(s->name[i], name)) { ((struct xscope *)s)->fn[i] = fn; return; }
}
internal int xsc_kind(const struct xscope *s, const char *name, size_t len) {
    for (; s; s = s->up)
        for (int i = 0; i < s->n; i++) if (strlen(s->name[i]) == len && !strncmp(s->name[i], name, len)) return s->kind[i];
    return X_ANY;
}
// nn: the paths known not to be null where the check is (narrowed: p.lord inside if p.lord != null { ... }) - a stack,
// each condition's pushed for what it guards and taken off after; a path set anew is struck (its length 0). nn_low: a
// function's body sees only those from there up (it runs later: those around it hold no more then)
struct xcheck { struct xctx *c; int script; const char *why; int line; char whybuf[200]; int loops; int ctx;
                const char *nn[32]; unsigned char nl[32]; int n_nn, nn_low; int texts;
                int body; };   // body: inside statements (a function's { }): a text file's statement functions may be called there
internal int xn_null(const struct xnode *n) { return n && n->k == XN_NUM && n->op == 'n'; }   // the null literal
internal int xn_narrowed(const struct xcheck *k, const char *s, size_t len) {
    for (int i = k->nn_low; i < k->n_nn; i++) if (k->nl[i] == len && !strncmp(k->nn[i], s, len)) return 1;
    return 0;
}
// a checked value's key: a path (p.lord), or a call of paths and numbers (me.attack_minutes(them) - the game's relations
// give the same within a check); its text kept in a ring the stack points into. 0: none (f(x).y, a + b: not narrowed)
internal char g_xn_keys[256][64];
internal int g_xn_key_n;
internal const char *xn_key(const struct xnode *x, int *len) {
    char *b = g_xn_keys[g_xn_key_n++ & 255];
    int o;
    if (x->k == XN_NAME) o = snprintf(b, 64, "%s", x->s);
    else if (x->k == XN_CALL) {
        o = snprintf(b, 64, "%s(", x->s);
        for (int i = 0; i < x->n_items && o < 63; i++) {
            const struct xnode *a = x->items[i];
            if (a->k == XN_NAME) o += snprintf(b + o, (size_t)(64 - o), "%s%s", i ? "," : "", a->s);
            else if (a->k == XN_NUM && a->op != 'n') o += snprintf(b + o, (size_t)(64 - o), "%s%g", i ? "," : "", a->n);
            else return NULL;
        }
        if (o < 63) o += snprintf(b + o, (size_t)(64 - o), ")");
    } else return NULL;
    if (o >= 63) return NULL;
    *len = o;
    return b;
}
internal void xn_push(struct xcheck *k, const struct xnode *x) {
    int len;
    const char *key = x ? xn_key(x, &len) : NULL;
    if (key && k->n_nn < 32) { k->nn[k->n_nn] = key; k->nl[k->n_nn++] = (unsigned char)len; }
}
internal int xn_checked(const struct xcheck *k, const struct xnode *x) {   // x's key on the stack: not null here
    int len;
    const char *key = xn_key(x, &len);
    return key && xn_narrowed(k, key, (size_t)len);
}
// what holds when cond is want (1 true, 0 false): the paths it shows aren't null - x != null, x == null (false), a && b
// (true), a || b (false), !; a path alone (until bools are required: lord ? ...) - pushed onto k's stack
internal void xn_facts(struct xcheck *k, const struct xnode *c, int want) {
    if (!c) return;
    switch (c->k) {
    case XN_CMP:
        if ((c->op == '!') == (want != 0) && (c->op == '!' || c->op == '=')) {
            xn_push(k, xn_null(c->a) ? c->b : xn_null(c->b) ? c->a : NULL);
        }
        break;
    case XN_AND: if (want) { xn_facts(k, c->a, 1); xn_facts(k, c->b, 1); } break;
    case XN_OR: if (!want) { xn_facts(k, c->a, 0); xn_facts(k, c->b, 0); } break;
    case XN_NOT: xn_facts(k, c->a, !want); break;
    case XN_NAME: case XN_CALL: if (want) xn_push(k, c); break;
    default: break;
    }
}
internal void xn_has(struct xcheck *k) {   // the key's guaranteed names (g_x_check_has): there from the start
    for (const char *w = g_x_check_has; w && *w && k->n_nn < 32; ) {
        while (*w == ' ') w++;
        size_t n = strcspn(w, " ");
        if (n) { k->nn[k->n_nn] = w; k->nl[k->n_nn++] = (unsigned char)n; }
        w += n;
    }
}
internal void xn_strike(struct xcheck *k, const char *name) {   // name set anew: it and its paths (name.lord) not known any more
    size_t len = strlen(name);
    for (int i = 0; i < k->n_nn; i++)
        if (k->nl[i] >= len && !strncmp(k->nn[i], name, len) && (k->nl[i] == len || k->nn[i][len] == '.' || k->nn[i][len] == '(')) k->nl[i] = 0;
}
// a node's value as the old parser saw it while only reading: a number or a text written as such, nothing for the
// rest - what "12" == 12 is caught with when a file is read
struct xsv { int w, has_n, has_s; double n; };
internal struct xsv xsv_of(const struct xnode *n) {
    struct xsv r = { 0, 0, 0, 0 };
    if (!n) return r;
    switch (n->k) {
    case XN_NUM: if (n->op != 'n') { r.w = r.has_n = 1; r.n = n->n; } break;   // (null: nothing)
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
internal const char *xty_name(int t, char *buf, int cap);
internal const char *xan(const char *name);   // ("n" before a word that starts with a vowel: an int)
// a member of the language's own types: a vector's x, y, z, w; a trigger's on, fired - its type, -1 none
// THE CHECK'S STRUCTS (STRUCTS AND REFS): a type that is a struct (not a list of them, not one or null); its
// declaration; a member of it by name - a field ('F': *idx its number among the fields) or a method ('M': among the methods)
internal int xst_is(int t) { return t >= XT_STRUCT && t < XT_STRUCT + g_xst_n && g_xst[t - XT_STRUCT].decl; }
internal const struct xnode *xst_member(int t, const char *name, int what, int *idx) {
    const struct xnode *d = xst_is(t) ? g_xst[t - XT_STRUCT].decl : NULL;
    for (int i = 0, j = 0; d && i < d->n_items; i++) {
        if (d->items[i]->op != what) continue;
        if (!strcmp(d->items[i]->s, name)) { if (idx) *idx = j; return d->items[i]; }
        j++;
    }
    return NULL;
}
internal void xst_list(int t, int what, char *out, int cap) {   // its fields' or methods' names, for a message
    const struct xnode *d = xst_is(t) ? g_xst[t - XT_STRUCT].decl : NULL;
    int o = 0;
    out[0] = 0;
    for (int i = 0; d && i < d->n_items && o < cap - 30; i++)
        if (d->items[i]->op == what && !strchr(d->items[i]->s, '<')) o += snprintf(out + o, (size_t)(cap - o), "%s%s", o ? ", " : "", d->items[i]->s);
    if (!o) lstrcpynA(out, "none", cap);
}
// a place's variable (a.b.c: a; l[2].x: l), "" none: a call's value, a list written out ...
internal const char *x_root_name(const struct xnode *n) {
    while (n && (n->k == XN_MEMBER || n->k == XN_INDEX)) n = n->a;
    return n && n->k == XN_NAME ? n->s : "";
}
internal int x_root_this(const struct xnode *n) { const char *r = x_root_name(n); return !strncmp(r, "this", 4) && (!r[4] || r[4] == '.'); }
// whether a method's code changes its struct (this set, a field or item of it set, handed on as a ref, a method of it
// called that does): such a one is called on a place only - on a value that is dropped it would change nothing
internal int x_sets_this(const struct xnode *n, int st, int depth) {
    if (!n) return 0;
    if ((n->k == XN_SETM || n->k == XN_SETI || n->k == XN_REF || (n->k == XN_LET && n->op == 'r')) && x_root_this(n->a)) return 1;
    if (n->k == XN_ASSIGN && !strcmp(n->s, "this")) return 1;
    if (n->k == XN_MCALL && x_root_this(n->a)) return 1;   // (a method of a field or an item: taken as changing)
    if (n->k == XN_CALL && !strncmp(n->s, "this.", 5)) {
        const struct xnode *m = strchr(n->s + 5, '.') ? NULL : xst_member(st, n->s + 5, 'M', NULL);
        if (!m || depth > 8 || x_sets_this(m->a, st, depth + 1)) return 1;
    }
    if (x_sets_this(n->a, st, depth) || x_sets_this(n->b, st, depth) || x_sets_this(n->c, st, depth)) return 1;
    for (int i = 0; i < n->n_items; i++) if (x_sets_this(n->items[i], st, depth)) return 1;
    return 0;
}
// a type's value when none is given (a struct's field left out): a number 0, a bool false, a text "", a list empty, a
// T? null, a struct its own fields' defaults - 0: it has none (a player, a vector ...: given, or declared)
internal int xst_has_default(int t, int depth) {
    if ((t & (X_NULLABLE | X_LIST_OF)) || t == XT_INT || t == XT_FLOAT || t == XT_BOOL || t == XT_TEXT || xt_is_hl(t)) return 1;   // (a list: a new empty one)
    const struct xnode *d = xst_is(t) && depth < 16 ? g_xst[t - XT_STRUCT].decl : NULL;
    for (int i = 0; d && i < d->n_items; i++)
        if (d->items[i]->op == 'F' && !d->items[i]->a && !xst_has_default((int)d->items[i]->n, depth + 1)) return 0;
    return d != NULL;
}
// WHAT A FUNCTION GIVES BACK, for a call of one the check knows (fd: its function): the type written after its
// parameters (name(a: int): float { ... }) - a declared function or a method with statements that hands back a value
// has one (x_check) -, else for one of a single value (name(a: int) => a * 2) that value's type, worked out with its
// parameters and the script's top as its names (sc: any scope of the script - a declared function's and a method's
// code sees its top); a function that calls itself on the way: not known there. X_ANY: not known
internal int x_stype(struct xcheck *k, const struct xnode *n, const struct xscope *sc);
// here: fd is being read where it is written (a function as a value: its names are sc's)
internal int x_fn_result_at(struct xcheck *k, const struct xnode *fd, const struct xscope *sc, int here) {
    if (!fd || fd->k != XN_LAMBDA) return X_ANY;
    if ((int)fd->n) return (int)fd->n;
    const struct xnode *b = fd->a;
    if (!b || b->k == XN_BLOCK) return X_ANY;
    if (b->k == XN_CALL && !strchr(b->s, '.') && xst_find(b->s) >= 0) return XT_STRUCT + xst_find(b->s);   // => point(...)
    if ((fd->op != 'D' && fd->op != 'M' && !here) || fd->sk || !sc) return X_ANY;   // (a let's function: where it was made isn't known here)
    struct xscope s = { NULL, { 0 }, 0, { 0 }, 0, { 0 }, { 0 } };
    while (!here && sc->up) sc = sc->up;
    s.up = sc;
    for (int i = 0; i < fd->n_items && s.n < 64; i++) {
        s.name[s.n] = fd->items[i]->s; s.kind[s.n] = (short)(int)fd->items[i]->n; s.decl[s.n++] = (unsigned char)(fd->items[i]->op == 'r' ? 3 : 1);
    }
    int low = k->nn_low, line = k->line, r;
    const char *why = k->why;
    k->nn_low = k->n_nn;
    ((struct xnode *)fd)->sk = 1;   // (being worked out: a call of itself in it is not known)
    r = x_stype(k, b, &s);
    ((struct xnode *)fd)->sk = 0;
    k->nn_low = low; k->why = why; k->line = line;
    return r;
}
internal int x_fn_result(struct xcheck *k, const struct xnode *fd, const struct xscope *sc) { return x_fn_result_at(k, fd, sc, 0); }
// A FUNCTION'S TYPE from its code (fd): func<its parameters' types, what it gives back> - a declared one with
// statements and no type written gives nothing (void). "function" alone: one with a ref parameter (no type says a
// ref), a method, more parameters than a type holds
internal int x_fn_type(struct xcheck *k, const struct xnode *fd, const struct xscope *sc, int here) {
    int ty[XCT_MEMBERS];
    if (!fd || fd->k != XN_LAMBDA || fd->op == 'M' || fd->n_items > XCT_MEMBERS) return XT_FN;
    for (int i = 0; i < fd->n_items; i++) { if (fd->items[i]->op == 'r') return XT_FN; ty[i] = (int)fd->items[i]->n; }
    int blk = fd->a && fd->a->k == XN_BLOCK;
    int ret = (int)fd->n ? (int)fd->n : blk ? (fd->op == 'D' ? XT_VOID : X_ANY) : x_fn_result_at(k, fd, sc, here);
    int f = xct_get(XCT_FN, fd->n_items, ty, ret, NULL);
    return f < 0 ? XT_FN : f;
}
internal int xt_own_member(int t, const char *m) {
    if (xst_is(t)) { const struct xnode *f = xst_member(t, m, 'F', NULL); return f ? (int)f->n : -1; }
    int vn = t == XT_VEC2 ? 2 : t == XT_VEC3 ? 3 : t == XT_VEC4 ? 4 : 0;
    if (vn && m[0] && !m[1] && strchr("xyzw", m[0]) && strchr("xyzw", m[0]) - "xyzw" < vn) return XT_FLOAT;
    if (t == XT_TRIG) return !strcmp(m, "on") ? XT_BOOL : !strcmp(m, "fired") ? XT_INT : -1;
    const struct xctype *c = t & (X_LIST_OF | X_NULLABLE) ? NULL : xct_of(t);
    for (int i = 0; c && c->kind == XCT_REC && i < c->n; i++) if (!strcmp(c->name[i], m)) return c->t[i];   // a record's member (0: of a type not known)
    return -1;
}
// r.f(...) of a record r: no method of it - its function member is called from a variable (as a list's item is)
internal int x_rec_call(struct xcheck *k, const struct xnode *n, int r, const char *m) {
    char tl[80];
    if (r != XT_REC && xt_own_member(r, m) < 0) snprintf(k->whybuf, sizeof k->whybuf, "%s(...): a %s has no %s", m, xty_name(r, tl, sizeof tl), m);
    else snprintf(k->whybuf, sizeof k->whybuf, "%s(...): a record has no methods - its function is called from a variable: let f = r.%s, then f(...)", m, m);
    return xc_fail(k, n, k->whybuf);
}
// A RECORD'S TYPE (a script's): its members' names and types as written - { at = vec(70, 50), units = 10 } is a
// record { at, units } whose at is a vec2 - in the order of their names (the same members written in another order:
// the same type), so r.unitz is told when the file is read. More than XCT_MEMBERS members, a text's record: any
// record (XT_REC), its members worked out when it runs
internal int x_rec_type(struct xcheck *k, const struct xnode *n, const struct xscope *sc) {
    int ty[XCT_MEMBERS], ord[XCT_MEMBERS];
    const char *name[XCT_MEMBERS];
    if (!k->script || n->n_items > XCT_MEMBERS) return XT_REC;
    for (int i = 0; i < n->n_items; i++) {   // (by name: a few, sorted as they are put in)
        int j = i;
        while (j > 0 && strcmp(n->items[ord[j - 1]]->s, n->items[i]->s) > 0) { ord[j] = ord[j - 1]; j--; }
        ord[j] = i;
    }
    for (int i = 0; i < n->n_items; i++) { name[i] = n->items[ord[i]]->s; ty[i] = x_stype(k, n->items[ord[i]]->a, sc); }
    int r = xct_get(XCT_REC, n->n_items, ty, 0, name);
    return r < 0 ? XT_REC : r;
}
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
        if ((kind & X_NULLABLE) && !xn_narrowed(k, s, at)) {   // a member of what may be null: checked first
            snprintf(k->whybuf, sizeof k->whybuf, "%.*s may be null (none there) - check %.*s != null first, or %.*s ?? ...",
                     (int)at, s, (int)at, s, (int)at, s);
            xc_fail(k, n, k->whybuf);
            return -1;
        }
        kind &= ~X_NULLABLE;
        if (kind & X_LIST_OF) {
            char t[40];
            snprintf(k->whybuf, sizeof k->whybuf, "%.*s: a %s - one of its items: [1].%s", (int)(at + 1 + sn), s, xty_name(kind, t, sizeof t), m);
            xc_fail(k, n, k->whybuf);
            return -1;
        }
        if (kind >= XT_INT) {   // the language's own: a vector's x, y, z, w; a trigger's on; a record's (not known yet)
            int own = xt_own_member(kind, m);
            if (kind == XT_REC) return X_ANY;
            if (own >= 0) { kind = own; at += 1 + sn; continue; }
            char t[40];
            if (xst_member(kind, m, 'M', NULL)) snprintf(k->whybuf, sizeof k->whybuf, "%.*s: %s is a method - %s(...)", (int)(at + 1 + sn), s, m, m);
            else snprintf(k->whybuf, sizeof k->whybuf, "%.*s: a%s %s has no %s", (int)(at + 1 + sn), s, xan(xty_name(kind, t, sizeof t)), t, m);
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
    if (at >= upto && (kind & X_NULLABLE) && xn_narrowed(k, s, upto)) kind &= ~X_NULLABLE;   // checked: there
    return at >= upto ? kind : X_ANY;
}
// "n" where a type's name takes "an" (an int, an epoch - a unit, a unit type)
internal const char *xan(const char *name) { return strchr("aeio", *name) || (*name == 'u' && strncmp(name, "unit", 4)) ? "n" : ""; }
// a type's name for the messages: int, list of player, player? ...
internal const char *xty_name(int t, char *buf, int cap) {
    static const char *const own[] = { "int", "float", "bool", "text", "vec2", "vec3", "vec4", "epoch", "color", "unit type",
                                       "function", "record", "trigger", "stop" };
    int b = X_TBASE(t), o = 0;
    const struct xctype *c = xct_of(t);
    char in[80];
    const char *n = !b ? "value" : b < XT_INT ? x_kind_name(b) : b - XT_INT < (int)(sizeof own / sizeof *own) ? own[b - XT_INT] :
                    b >= XT_STRUCT && b < XT_STRUCT + g_xst_n ? g_xst[b - XT_STRUCT].name : "value";
    if (t == X_NULLABLE) { lstrcpynA(buf, "null", cap); return buf; }   // the literal: none of any type
    if (c && c->kind == XCT_WRAP) n = xty_name(c->t[0], in, sizeof in);   // a list's items that are lists, or one or null
    else if (c && c->kind == XCT_FN) {   // func<int, text, point>
        o = snprintf(in, sizeof in, "func<");
        for (int i = 0; i <= c->n && o < (int)sizeof in - 12; i++) {
            char one[40];
            o += snprintf(in + o, sizeof in - (size_t)o, "%s%s", i ? ", " : "", i == c->n && c->ret == XT_VOID ? "void" : xty_name(i < c->n ? c->t[i] : c->ret, one, sizeof one));
        }
        snprintf(in + (o < (int)sizeof in - 2 ? o : (int)sizeof in - 2), 2, ">");
        n = in;
    } else if (c && c->kind == XCT_REC) {   // record { at, units }
        o = snprintf(in, sizeof in, "record {");
        for (int i = 0; i < c->n && o < (int)sizeof in - 24; i++) o += snprintf(in + o, sizeof in - (size_t)o, "%s%.16s", i ? ", " : " ", c->name[i]);
        snprintf(in + (o < (int)sizeof in - 3 ? o : (int)sizeof in - 3), 3, " }");
        n = in;
    } else if (c && c->kind == XCT_HLIST) {
        char one[60];
        snprintf(in, sizeof in, "list<%s>", xty_name(c->t[0], one, sizeof one));
        n = in;
    } else if (c && c->kind == XCT_DICT) {
        char one[30], two[40];
        snprintf(in, sizeof in, "dict<%s, %s>", xty_name(c->t[0], one, sizeof one), xty_name(c->t[1], two, sizeof two));
        n = in;
    } else if (b == XT_VOID) n = "nothing";
    snprintf(buf, (size_t)cap, "%s%s%s%s", t & X_LIST_OF ? "seq<" : "", n, t & X_LIST_OF ? ">" : "", t & X_NULLABLE ? "?" : "");
    return buf;
}
internal int xt_numeric(int t) { return t == XT_INT || t == XT_FLOAT; }
// a value of type from where one of type to belongs: the same, an int for a float (widened), a T or null for a T? (a
// T? for a T: no - checked != null first); not known either way: yes (worked out then)
internal int xt_fits(int to, int from) {
    if (!to || !from || to == from) return 1;
    if (to == XT_FLOAT && from == XT_INT) return 1;
    if (to & X_NULLABLE) return xt_fits(to & ~X_NULLABLE, from & ~X_NULLABLE);
    if (from & X_NULLABLE) return 0;
    if ((to & X_LIST_OF) && (from & X_LIST_OF)) return !X_TBASE(to) || !X_TBASE(from) || xt_fits(xt_elem(to), xt_elem(from));
    if ((to & X_LIST_OF) || (from & X_LIST_OF)) return 0;
    const struct xctype *a = xct_of(to), *b = xct_of(from);
    if ((to == XT_FN && b && b->kind == XCT_FN) || (from == XT_FN && a && a->kind == XCT_FN)) return 1;   // "function": any; one of no types known
    if ((to == XT_REC && b && b->kind == XCT_REC) || (from == XT_REC && a && a->kind == XCT_REC)) return 1;   // "record": any
    if (!a || !b || a->kind != b->kind || a->n != b->n) return 0;
    if (a->kind == XCT_FN) {   // a function for a function: the same parameters, a result that fits (none wanted: any)
        for (int i = 0; i < a->n; i++) if (a->t[i] && b->t[i] && a->t[i] != b->t[i]) return 0;
        return a->ret == XT_VOID || xt_fits(a->ret, b->ret);
    }
    if (a->kind == XCT_HLIST) return !a->t[0] || !b->t[0] || a->t[0] == b->t[0];   // a list for a list: of the very items (it is shared)
    if (a->kind == XCT_DICT) return a->t[0] == b->t[0] && a->t[1] == b->t[1];   // a dict for a dict: the very keys and values (it is shared)
    if (a->kind == XCT_REC) {   // a record for a record: the same members, each fitting
        for (int i = 0; i < a->n; i++) if (strcmp(a->name[i], b->name[i]) || !xt_fits(a->t[i], b->t[i])) return 0;
        return 1;
    }
    return 0;
}
// the type of a or b (?:, ??): the same, a float for an int and a float, else not known
internal int xt_join(int a, int b) {
    if (a == b) return a;
    if (a == X_NULLABLE || b == X_NULLABLE) return a == X_NULLABLE ? b | X_NULLABLE : a | X_NULLABLE;   // null or a T: a T?
    if ((a & ~X_NULLABLE) == (b & ~X_NULLABLE)) return a | b;
    if (xt_numeric(a & ~X_NULLABLE) && xt_numeric(b & ~X_NULLABLE)) return XT_FLOAT | ((a | b) & X_NULLABLE);
    if ((a & X_LIST_OF) && (b & X_LIST_OF) && (!X_TBASE(a) || !X_TBASE(b)))   // a list and [] (its items not known): that list
        return (X_TBASE(a) ? a : b) | ((a | b) & X_NULLABLE);
    return X_ANY;
}
internal int x_stype(struct xcheck *k, const struct xnode *n, const struct xscope *sc);
// what a value of type from needs to go where one of type to belongs (the end of a message)
internal const char *xt_hint(int to, int from) {
    const struct xctype *a = xct_of(to), *b = xct_of(from);
    if (a && b && a->kind == XCT_REC && b->kind == XCT_REC && a->n == b->n && !((to | from) & X_LIST_OF)) {   // records of the same names: the member that differs
        static char why[120];
        char ta[40], tb[40];
        for (int i = 0; i < a->n; i++)
            if (!strcmp(a->name[i], b->name[i]) && !xt_fits(a->t[i], b->t[i])) {
                snprintf(why, sizeof why, " - its %s is a%s %s, not a%s %s", a->name[i], xan(xty_name(b->t[i], tb, sizeof tb)), tb, xan(xty_name(a->t[i], ta, sizeof ta)), ta);
                return why;
            }
    }
    return to == XT_INT && from == XT_FLOAT ? " - int(...), round(...)" : (from & X_NULLABLE) && to && !(to & X_NULLABLE) ?
           " - it may be null: check it != null first, or ... ?? a value" : "";
}
// a builtin's result (x_fn_id f) from its arguments' types
internal int xt_builtin(struct xcheck *k, const struct xnode *n, int f, const struct xscope *sc) {
    int a0 = n->n_items ? x_stype(k, n->items[0], sc) : X_ANY, r = a0;
    switch (f) {
    case 1: case 10: case 19: return XT_BOOL;                     // chance, contains, has
    case 2:                                                        // min, max: their numbers'
        for (int i = 1; i < n->n_items; i++) r = xt_join(r, x_stype(k, n->items[i], sc));
        return xt_numeric(r) ? r : X_ANY;
    case 3: return xt_numeric(a0) ? a0 : X_ANY;                   // abs
    case 4: case 6: case 7: case 20: case 21: return XT_INT;      // round, floor, len, int, ceil
    case 5: case 14: case 18: return XT_FLOAT;                    // rnd, log2, distance
    case 8: case 9: case 15:   // first, item, pick: one of its items - or none (an empty list, item past its end); a list
        if (xt_is_hl(a0)) return xt_hl_elem(a0) | X_NULLABLE;
        if (!(a0 & X_LIST_OF)) return f == 15 ? a0 : X_ANY;   // written with an item that always holds has one: pick([10, 20])
        if (!X_TBASE(a0)) return X_ANY;
        if (f != 9 && n->items[0]->k == XN_LIST)
            for (int i = 0; i < n->items[0]->n_items; i++) {
                const struct xnode *it = n->items[0]->items[i];
                if (!(it->k == XN_COND && !it->b) && !(x_stype(k, it, sc) & X_NULLABLE)) return xt_elem(a0);
            }
        return xt_elem(a0) | X_NULLABLE;
    case 11: r = n->n_items > 2 ? x_stype(k, n->items[2], sc) : X_ANY; return xt_items(r);   // map_range
    case 12: return XT_TEXT;                                       // str
    case 13: return XT_FLOAT | X_NULLABLE;                         // number: nothing for a text that is none
    case 16: return n->n_items == 2 ? XT_VEC2 : n->n_items == 3 ? XT_VEC3 : XT_VEC4;   // vec
    case 17: return XT_VEC3;                                       // rgb
    default: return X_ANY;
    }
}
// WHAT A VALUE IS, worked out when the file is read (a script's): host.h's types - X_ANY where it can't be told
internal int x_stype(struct xcheck *k, const struct xnode *n, const struct xscope *sc) {
    if (!n || !(k->script || k->texts)) return X_ANY;
    int known, r, a, b;
    const struct x_kmember *m;
    switch (n->k) {
    case XN_NUM: return n->op == 'b' ? XT_BOOL : n->op == 'f' ? XT_FLOAT : n->op == 'i' ? XT_INT : n->op == 'n' ? X_NULLABLE : X_ANY;
    case XN_STR: return XT_TEXT;
    case XN_LAMBDA: return k->script ? x_fn_type(k, n, sc, 1) : XT_FN;   // func<int, point>: its parameters and result
    case XN_REC: return n->op == 'D' || n->op == 'S' ? (int)n->n : x_rec_type(k, n, sc);   // (made a dict or a struct where it goes: x_check)
    case XN_LIST:   // its items' type when they have one
        if (k->script && (int)n->n) return (int)n->n;   // (worked out when it was checked: x_check XN_LIST)
        r = n->n_items ? x_stype(k, n->items[0], sc) : X_ANY;
        for (int i = 1; i < n->n_items && r; i++) { a = x_stype(k, n->items[i], sc); r = r == a ? r : xt_numeric(r) && xt_numeric(a) ? XT_FLOAT : X_ANY; }
        return r & X_LIST_OF ? X_LIST_OF : X_LIST_OF | r;   // (a list in a list: its items are the list's)
    case XN_NAME: {
        size_t len = strcspn(n->s, ".");
        int var = xsc_find(sc, n->s, len, NULL) != 0;
        if (var && !n->s[len] && k->script) {   // a declared function named as a value: its type
            const struct xnode *fd = xsc_fn(sc, n->s, len);
            if (fd && fd->op == 'D') return x_fn_type(k, fd, sc, 0);
        }
        if (!var && !n->s[len] && !strcmp(n->s, "time")) return XT_FLOAT;   // the script's: game seconds
        if (!var && !n->s[len] && !strcmp(n->s, "stop")) return XT_STOP;
        if (!var && (r = g_x_host.const_type(n->s)) != X_ANY) return r;   // unit.x, epoch.middle, color.red
        if (!var && !n->s[len] && (r = g_x_host.name_type(n->s)) != X_ANY)   // minutes, epoch, lord (checked: there)
            return (r & X_NULLABLE) && xn_narrowed(k, n->s, len) ? r & ~X_NULLABLE : r;
        r = x_path_kind(k, n, n->s, strlen(n->s), sc, &known);
        return r < 0 ? X_ANY : r;
    }
    case XN_REF: return x_stype(k, n->a, sc);
    case XN_CALL: {
        const char *dot = strrchr(n->s, '.');
        int f = x_fn_id(n->s, n->n_items < 8 ? n->n_items : 8);
        if (k->script && !dot && xst_find(n->s) >= 0 && !xsc_find(sc, n->s, strlen(n->s), NULL)) return XT_STRUCT + xst_find(n->s);   // point(...)
        if (f && !xsc_find(sc, n->s, strlen(n->s), NULL)) return xt_builtin(k, n, f, sc);
        if (!dot && k->script && xsc_find(sc, n->s, strlen(n->s), NULL)) {   // make(): point { ... }; a variable of a function's type: its result
            const struct xnode *fd = xsc_fn(sc, n->s, strlen(n->s));
            const struct xctype *c = xct_of(xsc_kind(sc, n->s, strlen(n->s)));
            if ((r = x_fn_result(k, fd, sc)) != X_ANY) return r;
            if (c && c->kind == XCT_FN && !(xsc_kind(sc, n->s, strlen(n->s)) & X_LIST_OF)) return c->ret == XT_VOID ? X_ANY : c->ret;
            return X_ANY;
        }
        if (!dot) return x_script_fn(n->s) >= 0 ? g_x_host.fn_kind(n->s) : X_ANY;   // player(2)
        r = x_path_kind(k, n, n->s, (size_t)(dot - n->s), sc, &known);
        if (r > 0 && xst_is(r)) return x_fn_result(k, xst_member(r, dot + 1, 'M', NULL), sc);   // a.scaled(2): a struct's method
        if (r > 0 && xt_is_hl(r)) return xhm_id(dot + 1) == XHM_INDEX_OF ? XT_INT | X_NULLABLE : X_ANY;   // l.index_of(x)
        m = r > 0 && r < XT_INT ? x_kind_member(r, dot + 1) : NULL;
        r = m && m->amin >= 0 ? m->result : X_ANY;
        return (r & X_NULLABLE) && xn_checked(k, n) ? r & ~X_NULLABLE : r;   // me.attack_minutes(them) != null ? ...
    }
    case XN_MEMBER: case XN_MCALL:
        a = x_stype(k, n->a, sc);
        if (n->k == XN_MEMBER && a >= XT_INT && xt_own_member(a, n->s) >= 0) return xt_own_member(a, n->s);   // v.x, t.on
        if (n->k == XN_MCALL && xst_is(a)) return x_fn_result(k, xst_member(a, n->s, 'M', NULL), sc);
        if (n->k == XN_MCALL && xt_is_hl(a)) return xhm_id(n->s) == XHM_INDEX_OF ? XT_INT | X_NULLABLE : X_ANY;
        m = a > 0 && a < XT_INT ? x_kind_member(a, n->s) : NULL;
        return m && (m->amin >= 0) == (n->k == XN_MCALL) ? m->result : X_ANY;
    case XN_INDEX: a = x_stype(k, n->a, sc); return a & X_NULLABLE ? X_ANY : xt_is_dict(a) ? xt_dict_val(a) : xt_items(a);
    case XN_HLIST: case XN_DICT: return (int)n->n;
    case XN_COND: {   // (no else: nothing) - each side where its condition holds: p.lord != null ? p.lord : me
        int keep = k->n_nn;
        if (!n->b) return X_ANY;
        xn_facts(k, n->c, 1); a = x_stype(k, n->a, sc); k->n_nn = keep;
        xn_facts(k, n->c, 0); b = x_stype(k, n->b, sc); k->n_nn = keep;
        return xt_join(a, b);
    }
    case XN_COAL: a = x_stype(k, n->a, sc); b = x_stype(k, n->b, sc); return xt_join(a & ~X_NULLABLE, b);
    case XN_NEG: a = x_stype(k, n->a, sc); return xt_numeric(a) || (a >= XT_VEC2 && a <= XT_VEC4) ? a : X_ANY;
    case XN_ADD: case XN_MUL:   // int with int an int (but / a float), a float in it a float; a vector's own
        a = x_stype(k, n->a, sc); b = x_stype(k, n->b, sc);
        if (a >= XT_VEC2 && a <= XT_VEC4) return a;
        if (b >= XT_VEC2 && b <= XT_VEC4) return b;
        if (!xt_numeric(a) || !xt_numeric(b)) return X_ANY;
        return a == XT_INT && b == XT_INT && n->op != '/' ? XT_INT : XT_FLOAT;
    case XN_CMP: case XN_AND: case XN_OR: case XN_NOT: return XT_BOOL;
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
    else if (k->script && xst_find(name) >= 0) snprintf(k->whybuf, sizeof k->whybuf, "%s is a struct of this script - give the variable another name", name);
    else return 1;
    return xc_fail(k, n, k->whybuf);
}
// a script's game.setup(() => ...) - a statement at its top: run before the game starts (the host's SETUP), where
// none of the script's names is yet - its function sees none of them
internal int x_is_setup(const struct xnode *s) {
    return s->k == XN_CALL && !strcmp(s->s, "game.setup") && s->n_items == 1 && s->items[0]->k == XN_LAMBDA && !s->items[0]->n_items;
}
internal const struct xnode *g_x_ret_fn;   // the function being checked (x_check XN_LAMBDA): whose a return is
// the type wanted of the list or the { ... } about to be checked - a let's written one, a parameter's, a field's, the
// list's own for an item written as a list: x_check XN_LIST / XN_REC takes it (and clears it). Set for a list or a
// { ... } as written only (X_WANT)
internal int g_x_want;
#define X_WANT(node, type) (g_x_want = (node) && ((node)->k == XN_LIST || (node)->k == XN_REC) ? (type) : 0)
// statements that hand back on every way through them: the last a return, or an if with an else whose branches all do
internal int x_returns(const struct xnode *b) {
    if (b && b->k == XN_BLOCK) b = b->n_items ? b->items[b->n_items - 1] : NULL;
    if (!b) return 0;
    if (b->k == XN_RETURN) return 1;
    if (b->k == XN_IF) return b->b && x_returns(b->a) && x_returns(b->b);
    return 0;
}
internal int x_leaves(const struct xnode *b) {   // a block that always leaves where it is: its last a return, break, continue
    if (b && b->k == XN_BLOCK) b = b->n_items ? b->items[b->n_items - 1] : NULL;
    return b && (b->k == XN_RETURN || b->k == XN_BREAK || b->k == XN_CONTINUE);
}
// THE LANGUAGE SERVER'S RECORD (check/lsp.c - its check's process only, g_xr_out set: definitions, references,
// completion by type): each name the check passes, as a line to g_xr_out - where it is in the file, what it is (a
// variable and where its let is; a struct, a struct's member and where they are declared; a member of one of the
// host's kinds; a function or name of the language's or the host's; a name of the files' tops, which the server
// resolves - it knows where they are) and its type. "@@U path line col len what name type dpath dline dcol decl"
// (tabs between; lines from 0, columns in UTF-16 units: LSP's). The server's question (g_xr_q*: the type of what ends
// at a place, a dot about to follow) answered "@@T type" and a line "@@M name m|v type kind" per member.
// A tree's places point into the source it was parsed from, which is not always the file: a text's value is its lines
// joined (tl_value keeps where each character came from), an expression's tree is shared by every value of its text
// (x_compiled: the check of each says which value it is), a string's placeholders are cut out of its text (in an
// expression: the string's text unescaped, its source in the tree). Where a check's source came from is a stack
// (g_xrc), pushed where each check starts: XR_FILE the file's own text (off: where the source starts in it - a text
// file's function after its head "(a,b) => "; lo: the head's length, its parameters found on the key's line, head),
// XR_POS a value joined (pos: each character's place in the file), XR_STR a string's text (raw: its source in the code
// of the one below), XR_SUB a placeholder's (off: where it starts in the text below). code: the source the tree points
// into - NULL until the next compiled one (x_syntax_at, x_fills_ok set it)
enum { XR_FILE, XR_POS, XR_STR, XR_SUB };
struct xr_ctx { int kind; const char *code; size_t n; const int *pos; long off, lo; const char *raw, *head; struct xr_ctx *up; };
internal void (*g_xr_out)(const char *line);
internal struct xr_ctx *g_xrc;
internal char g_xr_path[MAX_PATH], g_xr_qpath[MAX_PATH];   // the file being checked; the question's file
internal const char *g_xr_text, *g_xr_pathc;   // its text; its path kept (a declaration's)
internal int *g_xr_ls, g_xr_nls, g_xr_utf8, g_xr_busy;   // its lines' starts; UTF-8 (a script) or the code page's (a text file)
internal int g_xr_qline = -1, g_xr_qcol, g_xr_qdone;
internal int xr_wc(char c) { return isalnum((unsigned char)c) || c == '_'; }
internal void xr_push(struct xr_ctx *c) { c->up = g_xrc; g_xrc = c; }
internal void xr_file(const char *path, const char *text, int utf8) {   // the file a check reads now
    if (!g_xr_out) return;
    if (!GetFullPathNameA(path, MAX_PATH, g_xr_path, NULL)) lstrcpynA(g_xr_path, path, MAX_PATH);
    g_xr_pathc = _strdup(g_xr_path);   // (a few a check's process: kept for the declarations found in it)
    g_xr_text = text;
    g_xr_utf8 = utf8;
    free(g_xr_ls);
    g_xr_nls = 0;
    int cap = 256;
    g_xr_ls = malloc(sizeof *g_xr_ls * (size_t)cap);
    for (const char *s = text; g_xr_ls; ) {
        if (g_xr_nls == cap) { int *t = realloc(g_xr_ls, sizeof *t * (size_t)(cap *= 2)); if (!t) break; g_xr_ls = t; }
        g_xr_ls[g_xr_nls++] = (int)(s - text);
        if (!(s = strchr(s, '\n'))) break;
        s++;
    }
}
internal void xr_lc(long off, int *line, int *col) {   // a place in the file: its line, its column
    int lo = 0, hi = g_xr_nls - 1;
    while (lo < hi) { int m = (lo + hi + 1) / 2; if (g_xr_ls[m] <= off) lo = m; else hi = m - 1; }
    *line = lo;
    const char *s = g_xr_text + (g_xr_nls ? g_xr_ls[lo] : 0), *e = g_xr_text + off;
    int c = 0;
    for (; s < e; s++) {
        unsigned char b = (unsigned char)*s;
        if (!g_xr_utf8 || (b & 0xC0) != 0x80) c += g_xr_utf8 && b >= 0xF0 ? 2 : 1;   // (a 4-byte character: two units)
    }
    *col = c;
}
internal const char *xr_word(const char *s, const char *w, size_t wl, const char *lim) {   // w whole from s on, before lim
    for (const char *b = s; *s && s + wl <= lim; s++)
        if (!strncmp(s, w, wl) && !xr_wc(s[wl]) && (s == b || !xr_wc(s[-1]))) return s;
    return NULL;
}
// character i of the text a string's source makes (raw at its quote, or "a" "b" side by side in ( )): where it is
// in the source - x_string's rules (an escape two characters for one, a verbatim string's "" one; the code in {braces}
// as written)
internal const char *xr_str_src(const char *s, long i) {
    long o = 0;
    for (;;) {
        while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n' || *s == '(') s++;
        if (*s == '@' && s[1] == '"') {
            for (s += 2; *s && *s != '\n'; s++, o++) {
                if (o == i) return s;
                if (*s == '"') { if (s[1] != '"') break; s++; }
            }
        } else if (*s == '"' || *s == '\'') {
            char q = *s;
            int depth = 0;
            for (s++; *s && *s != '\n'; s++, o++) {
                if (o == i) return s;
                if (depth) { if (*s == '{') depth++; else if (*s == '}') depth--; }
                else if (*s == q) break;
                else if (*s == '\\' && s[1]) s++;
                else if (*s == '{') depth = 1;
            }
        } else return NULL;
        if (*s != '"' && *s != '\'') return NULL;
        s++;   // past its closing quote: the next string, side by side
    }
}
internal long xr_off(const struct xr_ctx *c, long i) {   // a place in c's code: where it is in the file, -1 not known
    while (c) {
        if (!c->code || i < 0 || i > (long)c->n) return -1;
        if (c->kind == XR_POS) return i < (long)c->n ? c->pos[i] : c->n ? c->pos[c->n - 1] + 1 : -1;
        if (c->kind == XR_SUB) { i += c->off; c = c->up; continue; }
        if (c->kind == XR_STR) {
            const char *q = xr_str_src(c->raw, i);
            if (!q || !c->up || !c->up->code) return -1;
            i = (long)(q - c->up->code);
            c = c->up;
            continue;
        }
        if (i >= c->lo) return c->off + i;
        size_t wl = 0;   // a text file's function's head as its key has it: a parameter's name found on the key's line
        while (xr_wc(c->code[i + (long)wl])) wl++;
        const char *f = wl && c->head ? xr_word(c->head, c->code + i, wl, g_xr_text + c->off + c->lo) : NULL;
        return f ? (long)(f - g_xr_text) : -1;
    }
    return -1;
}
internal const struct xr_ctx *xr_ctx_of(const char *p) {   // the source p is in: the check's, or one it is inside (a
    for (const struct xr_ctx *c = g_xrc; p && c; c = c->up)   // struct declared in the script, read in a placeholder)
        if (c->code && p >= c->code && p <= c->code + c->n) return c;
    return NULL;
}
internal int xr_at(const char *p, int *line, int *col) {   // a place in the code being checked: its line and column
    const struct xr_ctx *c = xr_ctx_of(p);
    if (!c || !g_xr_text) return 0;
    long o = xr_off(c, (long)(p - c->code));
    if (o < 0) return 0;
    xr_lc(o, line, col);
    return 1;
}
// where a declaration's name is: on its node's line after its start (let x, for x in, a parameter in its head, a
// struct's name and fields) - a method's node starts at its ( and has it before (a generic one's: its <T: ...> between,
// where its name is its record's)
internal const char *xr_name_in(const struct xnode *n, const char *name) {
    const struct xr_ctx *c = n ? xr_ctx_of(n->at) : NULL;
    if (!c) return NULL;
    size_t wl = strlen(name);
    if (n->k == XN_LAMBDA && n->gen && n->gen->sname[0] && !strncmp(n->gen->at, name, wl)) return n->gen->at;
    const char *lim = n->at;
    while (*lim && *lim != '\n') lim++;
    const char *w = xr_word(n->at, name, wl, lim);
    if (!w && n->at - c->code >= (long)wl && !strncmp(n->at - wl, name, wl)) w = n->at - wl;
    return w;
}
struct xr_decl { const char *name, *path; int line, col; };   // a variable's declaration, by its name's text (the node's own)
internal struct xr_decl *g_xr_decl;
internal int g_xr_ndecl, g_xr_decl_cap;
internal void xr_put(int line, int col, size_t len, char what, const char *name, int type, const char *dpath, int dline, int dcol, int decl) {
    char t[80], b[MAX_PATH * 2 + 300];
    if (type) xty_name(type, t, sizeof t); else t[0] = 0;
    snprintf(b, sizeof b, "@@U\t%s\t%d\t%d\t%d\t%c\t%.80s\t%s\t%s\t%d\t%d\t%d", g_xr_path, line, col, (int)len, what, name, t,
             dpath ? dpath : "", dline, dcol, decl);
    g_xr_out(b);
}
internal void xr_decl(const char *name, const struct xnode *n, int type) {   // a variable declared (its name: the node's text)
    int line, col;
    const char *w = g_xr_out && !g_xr_busy ? xr_name_in(n, name) : NULL;
    if (!w || !xr_at(w, &line, &col)) return;
    if (g_xr_ndecl == g_xr_decl_cap) {
        int c = g_xr_decl_cap ? g_xr_decl_cap * 2 : 256;
        struct xr_decl *t = realloc(g_xr_decl, sizeof *t * (size_t)c);
        if (!t) return;
        g_xr_decl = t; g_xr_decl_cap = c;
    }
    g_xr_decl[g_xr_ndecl++] = (struct xr_decl){ name, g_xr_pathc, line, col };
    xr_put(line, col, strlen(name), 'v', name, type, g_xr_pathc, line, col, 1);
}
internal const struct xr_decl *xr_decl_of(const struct xscope *s, const char *name, size_t len) {   // a variable's in scope
    for (; s; s = s->up)
        for (int i = 0; i < s->n; i++)
            if (strlen(s->name[i]) == len && !strncmp(s->name[i], name, len)) {
                for (int d = g_xr_ndecl - 1; d >= 0; d--) if (g_xr_decl[d].name == s->name[i]) return &g_xr_decl[d];
                return NULL;
            }
    return NULL;
}
// a struct's member (what 'F' a field, 'M' a method) or the struct itself (name NULL): where it is declared
internal int xr_struct_at(int t, const char *name, int what, int *line, int *col) {
    const struct xnode *d = xst_is(t) ? g_xst[t - XT_STRUCT].decl : NULL;
    const struct xnode *m = d && name ? xst_member(t, name, what, NULL) : d;
    const char *w = m ? xr_name_in(m, name ? name : d->s) : NULL;
    return w && xr_at(w, line, col);
}
internal void xr_members(int t) {   // the question's answer: t and its members
    char b[300], ty[80];
    snprintf(b, sizeof b, "@@T\t%s", t ? xty_name(t, ty, sizeof ty) : "");
    g_xr_out(b);
    int base = t & ~X_NULLABLE;
    const struct x_kind *kd = base > 0 && base < XT_INT ? g_x_host.kind(base) : NULL;
    for (int i = 0; kd && i < kd->n; i++) {
        snprintf(b, sizeof b, "@@M\t%s\t%c\t%s\t%s", kd->m[i].name, kd->m[i].amin >= 0 ? 'm' : 'v', kd->m[i].result ? xty_name(kd->m[i].result, ty, sizeof ty) : "", kd->name);
        g_xr_out(b);
    }
    const struct xnode *d = xst_is(base) ? g_xst[base - XT_STRUCT].decl : NULL;
    for (int i = 0; d && i < d->n_items; i++) {
        const struct xnode *m = d->items[i];
        if (strchr(m->s, '<')) continue;   // (a generic method's copy: the method as written is listed)
        int mt = m->op == 'F' ? (int)m->n : m->k == XN_LAMBDA ? (int)m->n : 0;
        snprintf(b, sizeof b, "@@M\t%s\t%c\t%s\t%s", m->s, m->op == 'M' ? 'm' : 'v', mt ? xty_name(mt, ty, sizeof ty) : "", d->s);
        g_xr_out(b);
    }
    int vn = base == XT_VEC2 ? 2 : base == XT_VEC3 ? 3 : base == XT_VEC4 ? 4 : 0;
    for (int i = 0; i < vn; i++) { snprintf(b, sizeof b, "@@M\t%c\tv\tfloat\t%s", "xyzw"[i], xty_name(base, ty, sizeof ty)); g_xr_out(b); }
    if (base == XT_TRIG) { g_xr_out("@@M\ton\tv\tbool\ttrigger"); g_xr_out("@@M\tfired\tv\tint\ttrigger"); }
    if (xt_is_hl(base)) {
        static const char *const k[] = { "add", "insert", "remove_at", "clear", "index_of" };
        for (int i = 0; i < 5; i++) { snprintf(b, sizeof b, "@@M\t%s\tm\t%s\tlist", k[i], i == 4 ? "int?" : ""); g_xr_out(b); }
    }
    const struct xctype *c = t & (X_LIST_OF | X_NULLABLE) ? NULL : xct_of(base);
    for (int i = 0; c && c->kind == XCT_REC && i < c->n; i++) {
        snprintf(b, sizeof b, "@@M\t%s\tv\t%s\trecord", c->name[i], c->t[i] ? xty_name(c->t[i], ty, sizeof ty) : "");
        g_xr_out(b);
    }
}
internal void xr_end(const char *p, int t) {   // what ends at p is a t: the server's question when it asks of there
    int line, col;
    if (g_xr_qline < 0 || g_xr_qdone || lstrcmpiA(g_xr_path, g_xr_qpath) || !xr_at(p, &line, &col)) return;
    if (line != g_xr_qline || col != g_xr_qcol) return;
    g_xr_qdone = 1;
    xr_members(t);
}
// a member m of what is a t, at p: a member of a host's kind, a struct's field or method (call: a method)
internal int xr_member_at(const char *p, const char *m, size_t mn, int t, int call) {
    int line, col, dl, dc, b = t & ~X_NULLABLE;
    char nm[48], full[96];
    if (mn >= sizeof nm || (b & X_LIST_OF) || !xr_at(p, &line, &col)) return X_ANY;
    lstrcpynA(nm, m, (int)mn + 1);
    if (b > 0 && b < XT_INT) {
        const struct x_kmember *km = x_kind_member(b, nm);
        if (!km) return X_ANY;
        snprintf(full, sizeof full, "%s.%s", x_kind_name(b), nm);
        xr_put(line, col, mn, 'm', full, km->result, NULL, 0, 0, 0);
        return km->result;
    }
    if (xst_is(b)) {
        const struct xnode *f = xst_member(b, nm, call ? 'M' : 'F', NULL);
        if (!f) return X_ANY;
        snprintf(full, sizeof full, "%s.%s", g_xst[b - XT_STRUCT].name, nm);
        int ok = xr_struct_at(b, nm, call ? 'M' : 'F', &dl, &dc), ft = (int)f->n;
        xr_put(line, col, mn, 's', full, ft, ok ? g_xr_pathc : NULL, ok ? dl : 0, ok ? dc : 0, 0);
        return f->op == 'F' ? ft : X_ANY;
    }
    if (b >= XT_INT) { int own = xt_own_member(b, nm); return own < 0 ? X_ANY : own; }   // v.x, t.on: the language's own
    return X_ANY;
}
// a name as written (a path: a.b.c; call: its last part called) at p: its head - a variable, a struct, a function,
// a name of the host's or of the files' tops - then each member
internal void xr_path(struct xcheck *k, const struct xnode *n, const char *s, const char *p, const struct xscope *sc, int call) {
    size_t len = strlen(s), hn = strcspn(s, ".");
    int line, col, t = X_ANY, last = !s[hn], f;
    char head[48];
    if (hn >= sizeof head || !xr_at(p, &line, &col)) return;
    lstrcpynA(head, s, (int)hn + 1);
    if (xsc_find(sc, s, hn, NULL)) {   // a variable: its let
        const struct xr_decl *d = xr_decl_of(sc, s, hn);
        t = xsc_kind(sc, s, hn);
        xr_put(line, col, hn, 'v', head, t, d ? d->path : NULL, d ? d->line : 0, d ? d->col : 0, 0);
    } else if (k->script && last && call && (f = xst_find(head)) >= 0) {   // point(...): the struct
        int dl, dc, ok = xr_struct_at(XT_STRUCT + f, NULL, 0, &dl, &dc);
        xr_put(line, col, hn, 'S', head, XT_STRUCT + f, ok ? g_xr_pathc : NULL, ok ? dl : 0, ok ? dc : 0, 0);
        return;
    } else if (last && call) {   // a function of the language's, a script's or the host's (a command) - else a text file's
        int own = x_fn_id(head, n->n_items < 8 ? n->n_items : 8) || x_script_fn(head) >= 0 || g_x_host.command(head);
        xr_put(line, col, hn, own ? 'f' : 't', head, x_stype(k, n, sc), NULL, 0, 0, 0);
        return;
    } else if ((t = g_x_host.const_type(s)) != X_ANY) {   // epoch.middle, unit.x: a constant, whole
        xr_put(line, col, len, 'f', s, t, NULL, 0, 0, 0);
        xr_end(p + len, t);
        return;
    } else if (!k->script && x_field_known(head)) {   // a personality's field (let at a file's top)
        t = x_stype(k, n, sc);
        xr_put(line, col, hn, 't', head, last ? t : X_ANY, NULL, 0, 0, 0);
        if (last) { xr_end(p + len, t); return; }
        t = X_ANY;
    } else if ((t = g_x_host.root_kind(head)) != 0 || (t = g_x_host.name_type(head)) != X_ANY ||
               (k->script && (x_name_known(head) || !strcmp(head, "time") || !strcmp(head, "stop"))) ||
               (!k->script && g_x_host.name_valid(head) && !x_var_data(head) && !x_var_data(s))) {   // the host's: them, game, opinion
        xr_put(line, col, hn, 'f', head, t, NULL, 0, 0, 0);
    } else {   // a name of the files' tops, a record's member (rec.key) whole: the server knows where they are
        xr_put(line, col, len, 't', s, last ? x_stype(k, n, sc) : X_ANY, NULL, 0, 0, 0);
        return;
    }
    for (size_t o = hn; s[o] == '.'; ) {
        xr_end(p + o, t);
        const char *m = s + o + 1;
        size_t mn = strcspn(m, ".");
        t = xr_member_at(p + (m - s), m, mn, t, call && !m[mn]);
        o += 1 + mn;
    }
    if (!call) xr_end(p + len, t);
}
// the check is at node n: what it names recorded (the check's own state as it was: x_stype may note a mistake it
// doesn't make here)
internal void xr_node(struct xcheck *k, const struct xnode *n, const struct xscope *sc) {
    char why[sizeof k->whybuf];
    const char *w0 = k->why;
    int l0 = k->line, line, col;
    memcpy(why, k->whybuf, sizeof why);
    g_xr_busy = 1;
    switch (n->k) {
    case XN_NAME: if (n->op != 'F') xr_path(k, n, n->s, n->at, sc, 0); break;
    case XN_CALL: xr_path(k, n, n->s, n->at, sc, 1); xr_end(n->end, x_stype(k, n, sc)); break;
    case XN_MEMBER: case XN_MCALL:
        xr_member_at(n->at + 1, n->s, strlen(n->s), x_stype(k, n->a, sc), n->k == XN_MCALL);
        xr_end(n->end, x_stype(k, n, sc));
        break;
    case XN_INDEX: xr_end(n->end, x_stype(k, n, sc)); break;
    case XN_ASSIGN: {   // x = ...: its variable
        const char *w = xr_name_in(n, n->s);
        const struct xr_decl *d = xr_decl_of(sc, n->s, strlen(n->s));
        if (w && xr_at(w, &line, &col)) xr_put(line, col, strlen(n->s), d || k->script ? 'v' : 't', n->s, xsc_kind(sc, n->s, strlen(n->s)), d ? d->path : NULL, d ? d->line : 0, d ? d->col : 0, 0);
        break;
    }
    case XN_SETM: {   // a.b = ...: its member, after what it is of
        const char *w = n->a && n->a->end ? n->a->end : NULL;
        if (w && *w == '.') xr_member_at(w + 1, n->s, strlen(n->s), x_stype(k, n->a, sc), 0);
        break;
    }
    case XN_STRUCT: {   // its name, its members: declared
        int t = (int)n->n, dl, dc;
        char full[96];
        if (xr_struct_at(t, NULL, 0, &dl, &dc)) xr_put(dl, dc, strlen(n->s), 'S', n->s, t, g_xr_pathc, dl, dc, 1);
        for (int i = 0; i < n->n_items; i++) {
            const struct xnode *m = n->items[i];
            if (strchr(m->s, '<')) continue;   // (a generic method's copy: the method as written is recorded)
            snprintf(full, sizeof full, "%s.%s", n->s, m->s);
            if (xr_struct_at(t, m->s, m->op, &dl, &dc)) xr_put(dl, dc, strlen(m->s), 's', full, m->op == 'F' ? (int)m->n : 0, g_xr_pathc, dl, dc, 1);
        }
        break;
    }
    default: break;
    }
    g_xr_busy = 0;
    k->why = w0;
    k->line = l0;
    memcpy(k->whybuf, why, sizeof why);
}
internal int x_check_body(struct xcheck *k, const struct xnode *body, struct xscope *sc) {   // a block: its lets first
    if (!body || body->k != XN_BLOCK) return x_check(k, body, sc);
    if (k->script && !sc->up && !sc->blk)   // the script's top: its structs known from the start (a type, a method used above its struct)
        for (int i = 0; i < body->n_items; i++) {
            const struct xnode *s = body->items[i];
            int ti = s->k == XN_STRUCT ? (int)s->n - XT_STRUCT : -1;
            if (ti < 0 || ti >= g_xst_n) continue;
            if (g_xst[ti].decl && g_xst[ti].decl != s) { snprintf(k->whybuf, sizeof k->whybuf, "struct %s: declared twice", s->s); return xc_fail(k, s, k->whybuf); }
            g_xst[ti].decl = s;
        }
    if (k->script && !sc->up && !sc->blk)   // the script's top: its setup blocks, on their own
        for (int i = 0; i < body->n_items; i++) {
            if (!x_is_setup(body->items[i])) continue;
            struct xscope none = { NULL, { 0 }, 0, { 0 }, 0, { 0 }, { 0 } };
            if (!x_check(k, body->items[i]->items[0], &none)) return 0;
        }
    for (int i = 0; i < body->n_items; i++) {   // known in the whole block - a function made before may call one later
        const struct xnode *s = body->items[i];
        if (s->k != XN_LET) continue;
        if (!x_check_new_name(k, s, s->s, sc)) return 0;
        if (sc->n < 64) { sc->decl[sc->n] = s->op == 'r' ? 2 : 0; sc->name[sc->n++] = s->s; }
        if (g_xr_out) xr_decl(s->s, s, 0);
    }
    // (the language server's record, g_xr_out: a statement with a mistake doesn't end the check - the names of the
    // rest are recorded too; the result is still the mistake)
    int keep = k->n_nn, top = k->script && !sc->up && !sc->blk, bad = 0;
    if (top) { g_xg_top = (struct xnode *)body; g_xg_topsc = sc; }   // (GENERICS: where a generic function's copies go)
    for (int i = 0; i < body->n_items; i++) {
        const struct xnode *s = body->items[i];
        // the script's declared functions and its structs' methods: their code after the rest of its top (below) - they
        // run when they are called, and by then every let of the top is a variable of a type known (score.add(1)
        // of a let further down); here only that the name is a function, for the calls of it
        if (top && s->k == XN_LET && s->op == 'D' && s->a && s->a->k == XN_LAMBDA) { xsc_pass(sc, s->s, XT_FN, s->a); continue; }
        if (!(top && x_is_setup(s)) && !x_check(k, s, sc)) { if (!g_xr_out) { k->n_nn = keep; return 0; } bad = 1; continue; }
        if (s->k == XN_IF && !s->b && x_leaves(s->a)) xn_facts(k, s->c, 0);   // if p.lord == null { return }: there below
    }
    k->n_nn = keep;
    for (int i = 0; top && i < body->n_items; i++) {   // (the copies of generic functions its calls make: on its end, here too)
        const struct xnode *s = body->items[i];
        if (s->k == XN_LET && s->op == 'D' && s->a && s->a->k == XN_LAMBDA) {
            int gen = s->a->gen != NULL, copy = strchr(s->s, '<') != NULL, busy = g_xr_busy;
            g_xg_abstract += gen;   // a generic function as written: checked against its bounds
            if (copy) g_xr_busy = 1;   // (the language server's record: the function as written has the places)
            int ok = x_check(k, s->a, sc);
            g_xg_abstract -= gen;
            g_xr_busy = busy;
            if (!ok && copy) {   // a copy's mistake: which copy
                char w[sizeof k->whybuf];
                snprintf(w, sizeof w, "%s (in %s)", k->why ? k->why : "a mistake", s->s);
                lstrcpynA(k->whybuf, w, sizeof k->whybuf);
                k->why = k->whybuf;
            }
            if (!ok) { if (!g_xr_out) { g_xg_top = NULL; g_xg_topsc = NULL; return 0; } bad = 1; }
        }
    }
    // the structs' methods (an interface's have no code): a generic one as written against its bounds, a copy (add<cell>)
    // as its own - until a pass makes none new (a copy's calls may make more: of a struct checked before, a function's)
    int done[XST_MAX] = { 0 };
    for (int more = top, fdone = body->n_items; more; ) {
        more = 0;
        for (int i = 0; i < body->n_items; i++) {
            const struct xnode *s = body->items[i];
            if (i >= fdone && s->k == XN_LET && s->op == 'D' && s->a && s->a->k == XN_LAMBDA) {   // a function's copy a method's call made
                int busy = g_xr_busy;
                g_xr_busy = 1;
                int ok = x_check(k, s->a, sc);
                g_xr_busy = busy;
                if (!ok) { char w[sizeof k->whybuf]; snprintf(w, sizeof w, "%s (in %s)", k->why ? k->why : "a mistake", s->s); lstrcpynA(k->whybuf, w, sizeof k->whybuf); k->why = k->whybuf; }
                if (!ok) { if (!g_xr_out) { g_xg_top = NULL; g_xg_topsc = NULL; return 0; } bad = 1; }
            }
            int si = s->k == XN_STRUCT ? (int)s->n - XT_STRUCT : -1;
            if (si < 0 || si >= XST_MAX || s->op == 'I') continue;
            for (int j = done[si]; j < s->n_items; j++) {
                const struct xnode *f = s->items[j];
                if (f->op != 'M') continue;
                int gen = f->gen != NULL, copy = strchr(f->s, '<') != NULL, busy = g_xr_busy;
                g_xg_abstract += gen;
                if (copy) g_xr_busy = 1;
                int ok = x_check(k, f, sc);
                g_xg_abstract -= gen;
                g_xr_busy = busy;
                if (!ok && copy) { char w[sizeof k->whybuf]; snprintf(w, sizeof w, "%s (in %s.%s)", k->why ? k->why : "a mistake", s->s, f->s); lstrcpynA(k->whybuf, w, sizeof k->whybuf); k->why = k->whybuf; }
                if (!ok) { if (!g_xr_out) { g_xg_top = NULL; g_xg_topsc = NULL; return 0; } bad = 1; }
            }
            done[si] = s->n_items;
        }
        for (int i = 0; i < body->n_items && !more; i++) {
            const struct xnode *s = body->items[i];
            int si = s->k == XN_STRUCT ? (int)s->n - XT_STRUCT : -1;
            more = (si >= 0 && si < XST_MAX && s->op != 'I' && done[si] < s->n_items) || (i >= fdone && s->k == XN_LET && s->op == 'D');
        }
        fdone = body->n_items;
    }
    if (top) {   // the copies among the declared functions: made before the rest of the top runs (as xps_stmts puts them)
        struct xnode **it = body->items;
        for (int i = 0, d = 0; i < body->n_items; i++) {
            struct xnode *x = it[i];
            if (x->k != XN_LET || x->op != 'D') continue;
            memmove(it + d + 1, it + d, (size_t)(i - d) * sizeof *it);
            it[d++] = x;
        }
        g_xg_top = NULL;
        g_xg_topsc = NULL;
    }
    return !bad;
}
internal int x_check_block(struct xcheck *k, const struct xnode *blk, const struct xscope *sc) {   // an if's, a loop's
    struct xscope b = { sc, { 0 }, 0, { 0 }, 1, { 0 }, { 0 } };
    return x_check_body(k, blk, &b);
}
// THE CHECK OF STRUCTS AND REFS.
// A place: a variable, a field of a place that is a struct, an item of a place that is a list - what a set changes
// and a ref names (a call's value, a list written out are none: a change there would be dropped)
internal int x_place_path(struct xcheck *k, const struct xnode *n, const char *s, size_t len, const struct xscope *sc) {
    size_t hn = strcspn(s, ".");
    int known;
    if (hn > len) hn = len;
    if (!xsc_find(sc, s, hn, NULL)) return 0;
    for (size_t at = hn; at < len; at += 1 + strcspn(s + at + 1, ".")) {   // each a.b before a field: a struct
        int t = x_path_kind(k, n, s, at, sc, &known);
        if (t < 0 || !xst_is(t)) return 0;
    }
    return 1;
}
internal int x_place(struct xcheck *k, const struct xnode *n, const struct xscope *sc) {
    if (!n) return 0;
    if (n->k == XN_NAME) return x_place_path(k, n, n->s, strlen(n->s), sc);
    if (n->k == XN_MEMBER) return x_place(k, n->a, sc) && xst_is(x_stype(k, n->a, sc));
    if (n->k == XN_INDEX) {
        int t = x_stype(k, n->a, sc);
        if (xt_is_hl(t) && !(t & X_NULLABLE)) { ((struct xnode *)n)->sk |= 64; return 1; }   // a list's item: a place whatever holds the list
        return x_place(k, n->a, sc) && (t & X_LIST_OF) && !(t & X_NULLABLE);
    }
    return 0;
}
// a place on the way through a list's item (l[2].x, a.units[1].pos): changed in the list - a for's copy of a struct
// that holds the list changes the list too
internal int x_via_hl(struct xcheck *k, const struct xnode *n, const struct xscope *sc) {
    for (; n && (n->k == XN_MEMBER || n->k == XN_INDEX); n = n->a) if (n->k == XN_INDEX && xt_is_hl(x_stype(k, n->a, sc))) return 1;
    return 0;
}
// l.add(x) ... of a list<T> lt (LISTS): a method of it, its arguments of its items' type (marked for the compiler)
internal int x_check_hlmethod(struct xcheck *k, const struct xnode *n, int lt, const char *mname, const struct xscope *sc) {
    static const signed char want[] = { 0, 1, 2, 1, 0, 1 };
    int id = xhm_id(mname), e = xt_hl_elem(lt);
    char a[60], b[60];
    if (!id) { snprintf(k->whybuf, sizeof k->whybuf, "%s(...): a list has no %s - its methods: add, insert, remove_at, clear, index_of (its items: l[i], len(l), for x in l)", mname, mname); return xc_fail(k, n, k->whybuf); }
    if (g_x_in_ph) return xc_fail(k, n, "a list's method in a {placeholder} - a statement's (let i = l.index_of(x), then {i})");
    if (n->n_items != want[id]) { snprintf(k->whybuf, sizeof k->whybuf, "%s takes %d argument%s", mname, want[id], want[id] == 1 ? "" : "s"); return xc_fail(k, n, k->whybuf); }
    for (int i = 0; i < n->n_items; i++) {
        const struct xnode *ai = n->items[i];
        int ta = x_stype(k, ai, sc), tw = (id == XHM_INSERT && !i) || id == XHM_REMOVE_AT ? XT_INT : e;
        if (ai->k == XN_REF) return xc_fail(k, ai, "ref ...: a list's method takes values");
        if (tw == XT_INT ? !ta || ta == XT_INT : xt_fits(tw, ta)) continue;
        snprintf(k->whybuf, sizeof k->whybuf, "%s(...): its argument %d is a%s %s - a%s %s can't go in%s", mname, i + 1, xan(xty_name(tw, a, sizeof a)), a,
                 xan(xty_name(ta, b, sizeof b)), b, xt_hint(tw, ta));
        return xc_fail(k, n, k->whybuf);
    }
    ((struct xnode *)n)->sk = (unsigned char)(128 | ((id == XHM_ADD || id == XHM_INSERT) && (e & X_LIST_OF) ? 4 : 0));
    return 1;
}
#define X_PLACE_WHY "a variable, a field of one (a.pos) or a list's item (l[2]) - any other value is dropped with its change"
// a ref of a function around the one being checked: not seen there (a function made here may run when the place is gone) - 1: told
internal int x_ref_far(struct xcheck *k, const struct xnode *n, const char *name, size_t len, const struct xscope *sc) {
    int outer = 0;
    if (!(xsc_bits(sc, name, len, &outer) & 2) || !outer) return 0;
    snprintf(k->whybuf, sizeof k->whybuf, "%.*s is a ref (a place) of the code around this function - a function made there doesn't see it: it may run when the place is gone (let v = %.*s copies the value)",
             (int)len, name, (int)len, name);
    xc_fail(k, n, k->whybuf);
    return 1;
}
// a change to a for's variable that is a struct: the loop's copy of the item, not the list's - 1: told
internal int x_loop_copy(struct xcheck *k, const struct xnode *n, const char *root, const struct xscope *sc) {
    size_t len = strcspn(root, ".");
    if (!(xsc_bits(sc, root, len, NULL) & 4)) return 0;
    snprintf(k->whybuf, sizeof k->whybuf, "%.*s is the loop's copy of an item - a change to it is dropped: for i in 1..len(l) { l[i]... } changes the list's", (int)len, root);
    xc_fail(k, n, k->whybuf);
    return 1;
}
internal int x_check_refarg(struct xcheck *k, const struct xnode *r, struct xscope *sc) {   // ref <place>, a call's argument
    if (!x_check(k, r->a, sc)) return 0;
    if (!x_place(k, r->a, sc)) return xc_fail(k, r, "ref ...: a place - " X_PLACE_WHY);
    return !x_loop_copy(k, r, x_via_hl(k, r->a, sc) ? "" : x_root_name(r->a), sc);
}
// a call's arguments against the parameters of the function the check knows (fd's, from its number first on): as
// many, a ref for a ref and none else, each of its parameter's type (a ref: the very type - it is written through)
internal int x_check_args(struct xcheck *k, const struct xnode *n, const struct xnode *fd, int first, const char *name, const struct xscope *sc) {
    int want = fd->n_items - first, need = want;   // need: those without a default (theirs the last)
    while (need > 0 && fd->items[first + need - 1]->a) need--;
    if (n->n_items < need || n->n_items > want) {
        if (need == want) snprintf(k->whybuf, sizeof k->whybuf, "%s takes %d argument%s", name, want, want == 1 ? "" : "s");
        else snprintf(k->whybuf, sizeof k->whybuf, "%s takes %d to %d arguments (the last %d with a default)", name, need, want, want - need);
        return xc_fail(k, n, k->whybuf);
    }
    for (int i = 0; i < n->n_items; i++) {
        const struct xnode *pi = fd->items[first + i], *ai = n->items[i];
        int tp = (int)pi->n, ta = x_stype(k, ai, sc), pref = pi->op == 'r';
        char a[40], b[40];
        if (pref != (ai->k == XN_REF)) {
            if (pref) snprintf(k->whybuf, sizeof k->whybuf, "%s(...): its %s is a ref - %s(ref ...) hands it the place", name, pi->s, name);
            else snprintf(k->whybuf, sizeof k->whybuf, "%s(...): its %s is no ref - the value, without ref", name, pi->s);
            return xc_fail(k, n, k->whybuf);
        }
        if (pref ? !ta || ta == tp : xt_fits(tp, ta)) continue;
        if (g_xg_abstract && xst_is(X_TBASE(tp)) && g_xst[X_TBASE(tp) - XT_STRUCT].iface && X_TBASE(ta) != X_TBASE(tp)) {   // (GENERICS: a T's method of the interface's own type)
            const char *in = g_xst[X_TBASE(tp) - XT_STRUCT].name;
            snprintf(k->whybuf, sizeof k->whybuf, "%s(...): its %s is of the type itself (an %s's: whatever has it) - not a%s %s; one that takes any: %s<T: %s>(%s: T) in the interface", name, pi->s,
                     in, xan(xty_name(ta, b, sizeof b)), b, name, in, pi->s);
            return xc_fail(k, n, k->whybuf);
        }
        snprintf(k->whybuf, sizeof k->whybuf, pref ? "%s(...): its %s is the place of a%s %s - not of a%s %s%s" : "%s(...): its %s is a%s %s - a%s %s can't go in%s", name, pi->s,
                 xan(xty_name(tp, a, sizeof a)), a, xan(xty_name(ta, b, sizeof b)), b, pref ? "" : xt_hint(tp, ta));
        return xc_fail(k, n, k->whybuf);
    }
    return 1;
}
internal int xg_want(const struct xnode *fd, int i);   // (GENERICS, below)
internal int xg_mcall(struct xcheck *k, struct xnode *n, int st, const struct xnode *m, const char *mname, const struct xscope *sc, const struct xnode **copy);
// the type wanted of a call's argument number i (from 0), where the check knows the function: a struct's field
// (point(...)), a declared function's or a let's parameter, a variable's of a function's type, a struct's method's
internal int x_arg_want(struct xcheck *k, const struct xnode *n, int i, const struct xscope *sc) {
    const char *name = n->s, *dot = strrchr(name, '.');
    int v = xsc_find(sc, name, strlen(name), NULL), known;
    if (!dot && !v && xst_find(name) >= 0) {
        const struct xnode *d = g_xst[xst_find(name)].decl;
        for (int j = 0, f = 0; d && j < d->n_items; j++) if (d->items[j]->op == 'F' && f++ == i) return (int)d->items[j]->n;
        return 0;
    }
    if (!dot && v == 1) {
        const struct xnode *fd = xsc_fn(sc, name, strlen(name));
        const struct xctype *c = xct_of(xsc_kind(sc, name, strlen(name)));
        if (fd) return xg_want(fd, i);   // (a generic function's parameter of a T: the argument tells the T - GENERICS)
        return c && c->kind == XCT_FN && i < c->n ? c->t[i] : 0;
    }
    if (dot && xsc_find(sc, name, strcspn(name, "."), NULL) == 1) {
        const char *why = k->why;
        int line = k->line, r = x_path_kind(k, n, name, (size_t)(dot - name), sc, &known);
        k->why = why; k->line = line;   // (a mistake in the path: told where the call is checked)
        const struct xnode *m = r > 0 && xst_is(r) ? xst_member(r, dot + 1, 'M', NULL) : NULL;
        int id = r > 0 && xt_is_hl(r) ? xhm_id(dot + 1) : 0;
        if (id == XHM_ADD || id == XHM_INDEX_OF || (id == XHM_INSERT && i == 1)) return xt_hl_elem(r);
        return xg_want(m, i + 1);
    }
    return 0;
}
// a.method(...) of a struct st: a method of it, its arguments its parameters' - one that changes its struct on a place
// only (place; root: the place's variable). Marked for the compiler
internal int x_check_smethod(struct xcheck *k, const struct xnode *n, int st, const char *mname, int place, const char *root, const struct xscope *sc) {
    const struct xnode *m = xst_member(st, mname, 'M', NULL);
    char t[40], l[120];
    if (!m) {
        xst_list(st, 'M', l, sizeof l);
        if (xst_member(st, mname, 'F', NULL)) snprintf(k->whybuf, sizeof k->whybuf, "%s(...): %s is a field - without ( )", mname, mname);
        else snprintf(k->whybuf, sizeof k->whybuf, "%s(...): a %s has no method %s - its methods: %s", mname, xty_name(st, t, sizeof t), mname, l);
        return xc_fail(k, n, k->whybuf);
    }
    if (m->gen) {   // a generic method: of its copy for the arguments' types (GENERICS)
        const struct xnode *c = NULL;
        int r = xg_mcall(k, (struct xnode *)n, st, m, mname, sc, &c);
        if (r != 1) return r == 2;
        m = c;
    }
    if (!x_check_args(k, n, m, 1, mname, sc)) return 0;
    if (x_sets_this(m->a, st, 0)) {
        if (g_x_in_ph) {   // (a text's code calls it on a copy: x_ph_method)
            snprintf(k->whybuf, sizeof k->whybuf, "%s(...) changes its %s - a statement's, not a {placeholder}'s", mname, xty_name(st, t, sizeof t));
            return xc_fail(k, n, k->whybuf);
        }
        if (!place) {
            snprintf(k->whybuf, sizeof k->whybuf, "%s(...) changes its %s - called on " X_PLACE_WHY, mname, xty_name(st, t, sizeof t));
            return xc_fail(k, n, k->whybuf);
        }
        if (x_loop_copy(k, n, root, sc)) return 0;
    }
    ((struct xnode *)n)->sk = (unsigned char)(place ? 1 : 2);
    return 1;
}
// point(a, b ...): a struct's value - its fields in order, each of its type; one left out has a default
internal int x_check_new(struct xcheck *k, const struct xnode *n, int sti, const struct xscope *sc) {
    const struct xnode *d = g_xst[sti].decl;
    char a[40], b[40], l[120];
    int nf = 0;
    if (g_xst[sti].iface) { snprintf(k->whybuf, sizeof k->whybuf, "%s(...): %s is an interface - no value is made of one", g_xst[sti].name, g_xst[sti].name); return xc_fail(k, n, k->whybuf); }
    if (g_x_in_ph) return xc_fail(k, n, "a struct made in a {placeholder} - let v = ... first, then {v}");
    if (!d) return xc_fail(k, n, "a struct that isn't declared");
    xst_list(XT_STRUCT + sti, 'F', l, sizeof l);
    for (int i = 0; i < d->n_items; i++) {
        const struct xnode *f = d->items[i];
        if (f->op != 'F') continue;
        int ft = (int)f->n;
        if (nf < n->n_items) {
            int ta = x_stype(k, n->items[nf], sc);
            if (n->items[nf]->k == XN_REF || !xt_fits(ft, ta)) {
                snprintf(k->whybuf, sizeof k->whybuf, "%s(...): its %s is a%s %s - a%s %s can't go in%s", d->s, f->s, xan(xty_name(ft, a, sizeof a)), a,
                         xan(xty_name(ta, b, sizeof b)), b, xt_hint(ft, ta));
                return xc_fail(k, n, k->whybuf);
            }
        } else if (!f->a && !xst_has_default(ft, 0)) {
            snprintf(k->whybuf, sizeof k->whybuf, "%s(...): its %s (a%s %s) has no default - given here, or declared: %s: %s = ...", d->s, f->s,
                     xan(xty_name(ft, a, sizeof a)), a, f->s, a);
            return xc_fail(k, n, k->whybuf);
        }
        nf++;
    }
    if (n->n_items > nf) {
        snprintf(k->whybuf, sizeof k->whybuf, "%s takes at most %d value%s - its fields: %s", d->s, nf, nf == 1 ? "" : "s", l);
        return xc_fail(k, n, k->whybuf);
    }
    return 1;
}
internal int xst_holds(int t, int target, int depth) {   // a struct t that has target inside it, itself or through its fields' structs
    if (t == target) return 1;
    const struct xnode *d = xst_is(t) && depth < 16 ? g_xst[t - XT_STRUCT].decl : NULL;
    for (int i = 0; d && i < d->n_items; i++) if (d->items[i]->op == 'F' && xst_holds((int)d->items[i]->n, target, depth + 1)) return 1;
    return 0;
}
// a struct's declaration: its name no other's, each field's default a value of its type that needs none of the
// script's names, no struct inside itself (a list of them or one or null ends it)
// THE CHECK OF GENERICS (GENERICS above). An interface's own name in its members: the type that has it
internal int xg_subst(int t, int iface, int u) { return X_TBASE(t) == iface && !(t & X_LIST_OF) ? u | (t & X_NULLABLE) : t; }
// a type as written (t) of a generic function or method g: the type parameter it is (T, T?, ref T - *nul its ?), -1
// another; xg_canon: written without spaces, each type parameter its number in map ($0 ...) - 0 it names none
internal int xg_tp_of(const struct xgen *g, const char *t, int *nul) {
    char b[64];
    *nul = 0;
    if (!g) return -1;
    while (*t == '&' || *t == ' ') t++;
    lstrcpynA(b, t, sizeof b);
    size_t n = strlen(b);
    while (n && b[n - 1] == ' ') b[--n] = 0;
    if (n && b[n - 1] == '?') { *nul = 1; b[--n] = 0; }
    for (int j = 0; j < g->n; j++) if (!strcmp(b, g->name[j])) return j;
    return -1;
}
internal int xg_canon(const struct xgen *g, const char *t, const int *map, char *out, int cap) {
    int o = 0, any = 0;
    while (*t == '&') t++;
    for (const char *q = t; *q && o < cap - 4; ) {
        if (*q == ' ') { q++; continue; }
        int hit = -1;
        for (int j = 0; g && j < g->n && hit < 0; j++) {
            size_t wl = strlen(g->name[j]);
            if (!strncmp(q, g->name[j], wl) && (q == t || !(isalnum((unsigned char)q[-1]) || q[-1] == '_')) && !(isalnum((unsigned char)q[wl]) || q[wl] == '_')) hit = j;
        }
        if (hit < 0) { out[o++] = *q++; continue; }
        o += snprintf(out + o, (size_t)(cap - o), "$%d", map ? map[hit] : hit);
        q += strlen(g->name[hit]);
        any = 1;
    }
    out[o] = 0;
    return any;
}
internal int xg_fits(int u, int iface, char *why, int cap);
// a struct's method f against an interface's m that are generic (either): what each takes and gives back the same
// up to the type parameters' names - m's T one of f's, its bound one f's bound takes (f takes all m's caller may
// hand it); f's T where m has a type of its own: one f's bound takes (f takes more). 0: *why (it starts "its ...")
internal int xg_fits_gen(const struct xnode *m, const struct xnode *f, int iface, int u, char *why, int cap) {
    const struct xgen *gm = m->gen, *gf = f->gen;
    int map[XG_MAX], inv[XG_MAX], np = m->n_items - 1, nul, nul2;
    char ta[60], tb[60], ca[80], cb[80];
    for (int j = 0; j < XG_MAX; j++) map[j] = inv[j] = -1;
    for (int j = 0; j < np; j++) {   // m's T as a parameter: f's must be one of its own there
        int a = xg_tp_of(gm, gm ? gm->ptype[j] : "", &nul), b = xg_tp_of(gf, gf ? gf->ptype[j] : "", &nul2);
        if (a < 0) continue;
        if (b < 0) {
            snprintf(why, (size_t)cap, "its %s takes a%s %s as argument %d - the interface's any %s (%s<T: %s>(...) takes any)", m->s,
                     xan(xty_name((int)f->items[j + 1]->n, ta, sizeof ta)), ta, j + 1, g_xst[gm->iface[a] - XT_STRUCT].name, m->s, g_xst[gm->iface[a] - XT_STRUCT].name);
            return 0;
        }
        if (nul != nul2 || (map[a] >= 0 && map[a] != b) || (inv[b] >= 0 && inv[b] != a)) {
            snprintf(why, (size_t)cap, "its %s's argument %d isn't of the interface's type parameter as there", m->s, j + 1);
            return 0;
        }
        map[a] = b; inv[b] = a;
    }
    for (int a = 0; gm && a < gm->n; a++) {   // each bound: f's takes what m's promises
        if (map[a] < 0) continue;
        char w[200];
        if (!xg_fits(gm->iface[a], gf->iface[map[a]], w, sizeof w)) { const char *nb = g_xst[gf->iface[map[a]] - XT_STRUCT].name; snprintf(why, (size_t)cap, "its %s takes only a%s %s - the interface's any %s", m->s, xan(nb), nb, g_xst[gm->iface[a] - XT_STRUCT].name); return 0; }
    }
    for (int j = 0; j <= np; j++) {   // each parameter (then the result, j == np) as written, the type parameters by their numbers
        const char *wm = j < np ? gm ? gm->ptype[j] : "" : gm ? gm->rtype : "", *wf = j < np ? gf ? gf->ptype[j] : "" : gf ? gf->rtype : "";
        int am = xg_canon(gm, wm, NULL, ca, sizeof ca), af = xg_canon(gf, wf, inv, cb, sizeof cb);
        if (j < np && (m->items[j + 1]->op == 'r') != (f->items[j + 1]->op == 'r')) { snprintf(why, (size_t)cap, "its %s's argument %d is %s ref - the interface's %s", m->s, j + 1, f->items[j + 1]->op == 'r' ? "a" : "no", m->items[j + 1]->op == 'r' ? "is" : "isn't"); return 0; }
        if (am || af) {
            int b = af && j < np ? xg_tp_of(gf, wf, &nul2) : -1;
            if (!wm[0]) wm = xty_name(xg_subst(j < np ? (int)m->items[j + 1]->n : (int)m->n, iface, u), ta, sizeof ta);
            if (!wf[0]) wf = xty_name(j < np ? (int)f->items[j + 1]->n : (int)f->n, tb, sizeof tb);
            if (!am && b >= 0 && inv[b] < 0) {   // f's own T where m takes a type: one its bound takes
                int want = xg_subst((int)m->items[j + 1]->n, iface, u);
                char w[200];
                if ((want & X_NULLABLE) && !nul2) { snprintf(why, (size_t)cap, "its %s takes no null as argument %d - the interface's a%s %s", m->s, j + 1, xan(xty_name(want, ta, sizeof ta)), ta); return 0; }
                if (!xg_fits(want & ~X_NULLABLE, gf->iface[b], w, sizeof w)) { snprintf(why, (size_t)cap, "its %s's argument %d: %s", m->s, j + 1, w); return 0; }
                continue;
            }
            if (am != af || strcmp(ca, cb)) {
                if (j < np) snprintf(why, (size_t)cap, "its %s takes a%s %s as argument %d - the interface's a%s %s", m->s, xan(wf), wf, j + 1, xan(wm), wm);
                else snprintf(why, (size_t)cap, "its %s gives a%s %s - the interface's a%s %s", m->s, xan(wf), wf, xan(wm), wm);
                return 0;
            }
            continue;
        }
        if (j == np) {
            int rt = xg_subst((int)m->n, iface, u);
            if ((int)m->n && (int)f->n && !xt_fits(rt, (int)f->n)) {
                snprintf(why, (size_t)cap, "its %s gives a%s %s, not a%s %s", m->s, xan(xty_name((int)f->n, ta, sizeof ta)), ta, xan(xty_name(rt, tb, sizeof tb)), tb);
                return 0;
            }
            continue;
        }
        int wp = xg_subst((int)m->items[j + 1]->n, iface, u), hp = (int)f->items[j + 1]->n;
        if (hp != wp) {
            snprintf(why, (size_t)cap, "its %s takes a%s %s as argument %d, the interface's a%s %s", m->s, xan(xty_name(hp, ta, sizeof ta)), ta, j + 1, xan(xty_name(wp, tb, sizeof tb)), tb);
            return 0;
        }
    }
    return 1;
}
// u has iface's shape: each field (a member of the kind's, a struct's field, the language's own: v.x) readable, of a
// type its field's takes; each method with as many arguments, a struct's of the very types (the interface's own
// name: u), what it gives back one the interface's takes. 0: *why says what is missing
internal int xg_fits_in(int u, int iface, char *why, int cap);
internal int xg_fits(int u, int iface, char *why, int cap) {
    static struct { int u, iface; } on[16];   // (the fits being checked: one asked again on the way holds - its answer is the outer one's)
    static int non;
    for (int i = 0; i < non; i++) if (on[i].u == u && on[i].iface == iface) return 1;
    if (non == 16) return 1;
    on[non].u = u; on[non].iface = iface; non++;
    int r = xg_fits_in(u, iface, why, cap);
    non--;
    return r;
}
internal int xg_fits_in(int u, int iface, char *why, int cap) {
    const struct xnode *d = g_xst[iface - XT_STRUCT].decl;
    const char *ti = g_xst[iface - XT_STRUCT].name;
    char tu[60], ta[60], tb[60];
    if (u == iface) return 1;
    xty_name(u, tu, sizeof tu);
    char au[70];
    snprintf(au, sizeof au, "a%s %s", xan(tu), tu);
    if (u & X_NULLABLE) { snprintf(why, (size_t)cap, "%s may be null - it is no %s (a T? parameter takes one that may be)", au, ti); return 0; }
    for (int i = 0; d && i < d->n_items; i++) {
        const struct xnode *m = d->items[i];
        if (m->op == 'F') {
            int want = xg_subst((int)m->n, iface, u), have = -1;
            if (u > 0 && u < XT_INT) { const struct x_kmember *km = x_kind_member(u, m->s); if (km && km->amin < 0) have = km->result; }
            else if (xst_is(u)) { const struct xnode *f = xst_member(u, m->s, 'F', NULL); if (f) have = (int)f->n; }
            else if (u >= XT_INT) have = xt_own_member(u, m->s);
            if (have < 0) { snprintf(why, (size_t)cap, "%s is no %s - it has no %s", au, ti, m->s); return 0; }
            if (have && !xt_fits(want, have)) {
                snprintf(why, (size_t)cap, "%s is no %s - its %s is a%s %s, not a%s %s", au, ti, m->s, xan(xty_name(have, ta, sizeof ta)), ta, xan(xty_name(want, tb, sizeof tb)), tb);
                return 0;
            }
            continue;
        }
        int np = m->n_items - 1, rt = xg_subst((int)m->n, iface, u);   // a method (its this first)
        if (u > 0 && u < XT_INT) {
            const struct x_kmember *km = x_kind_member(u, m->s);
            if (!km || km->amin < 0) { snprintf(why, (size_t)cap, "%s is no %s - it has no method %s", au, ti, m->s); return 0; }
            if (m->gen) { snprintf(why, (size_t)cap, "%s is no %s - its %s takes no type of the caller's (the interface's is generic)", au, ti, m->s); return 0; }
            if (np < km->amin || np > km->amax) { snprintf(why, (size_t)cap, "%s is no %s - its %s takes %d to %d arguments, the interface's %d", au, ti, m->s, km->amin, km->amax, np); return 0; }
            if ((int)m->n && km->result && !xt_fits(rt, km->result)) {
                snprintf(why, (size_t)cap, "%s is no %s - its %s gives a%s %s, not a%s %s", au, ti, m->s, xan(xty_name(km->result, ta, sizeof ta)), ta, xan(xty_name(rt, tb, sizeof tb)), tb);
                return 0;
            }
            continue;
        }
        const struct xnode *f = xst_is(u) ? xst_member(u, m->s, 'M', NULL) : NULL;
        if (!f) { snprintf(why, (size_t)cap, "%s is no %s - it has no method %s", au, ti, m->s); return 0; }
        if (f->n_items - 1 != np) { snprintf(why, (size_t)cap, "%s is no %s - its %s takes %d argument%s, the interface's %d", au, ti, m->s, f->n_items - 1, f->n_items == 2 ? "" : "s", np); return 0; }
        if (m->gen || f->gen) {   // (a generic method on either side)
            char w[200];
            if (xg_fits_gen(m, f, iface, u, w, sizeof w)) continue;
            snprintf(why, (size_t)cap, "%s is no %s - %s", au, ti, w);
            return 0;
        }
        for (int j = 1; j <= np; j++) {
            int wp = xg_subst((int)m->items[j]->n, iface, u), hp = (int)f->items[j]->n;
            if (hp == wp && (m->items[j]->op == 'r') == (f->items[j]->op == 'r')) continue;
            snprintf(why, (size_t)cap, "%s is no %s - its %s takes a%s %s%s as argument %d, the interface's a%s %s%s", au, ti, m->s,
                     xan(xty_name(hp, ta, sizeof ta)), ta, f->items[j]->op == 'r' ? " (ref)" : "", j, xan(xty_name(wp, tb, sizeof tb)), tb, m->items[j]->op == 'r' ? " (ref)" : "");
            return 0;
        }
        if ((int)m->n && (int)f->n && !xt_fits(rt, (int)f->n)) {
            snprintf(why, (size_t)cap, "%s is no %s - its %s gives a%s %s, not a%s %s", au, ti, m->s, xan(xty_name((int)f->n, ta, sizeof ta)), ta, xan(xty_name(rt, tb, sizeof tb)), tb);
            return 0;
        }
    }
    return 1;
}
// a parameter's type as written (t) against its argument's (ta): the type parameters it names bound in bind - T, T?,
// seq<T>, list<T>, dict<K, V> taken apart; a type of none of them is the copy's check's. 0: they disagree, or one
// can't be told (*why)
internal int xg_unify(const struct xgen *g, const char *t, int ta, int *bind, char *why, int cap) {
    char b[64], tn[60], tm[60];
    while (*t == '&' || *t == ' ') t++;
    lstrcpynA(b, t, sizeof b);
    size_t n = strlen(b);
    while (n && b[n - 1] == ' ') b[--n] = 0;
    int nul = n && b[n - 1] == '?';
    if (nul) b[--n] = 0;
    for (int j = 0; j < g->n; j++) {
        if (strcmp(b, g->name[j])) continue;
        int u = nul ? ta & ~X_NULLABLE : ta;
        if (!X_TBASE(u)) return 1;   // (null, a type not known: it tells nothing - another argument may)
        if (bind[j] && bind[j] != u) {
            snprintf(why, (size_t)cap, "%s is a%s %s here, a%s %s before", b[0] == '#' ? "its type" : b, xan(xty_name(u, tn, sizeof tn)), tn, xan(xty_name(bind[j], tm, sizeof tm)), tm);
            return 0;
        }
        bind[j] = u;
        return 1;
    }
    int at = ta & ~X_NULLABLE;
    if (n > 5 && !strncmp(b, "seq<", 4) && b[n - 1] == '>') {
        b[n - 1] = 0;
        if (!(at & X_LIST_OF)) return 1;   // (no seq: the copy's check tells)
        return xg_unify(g, b + 4, xt_items(at), bind, why, cap);
    }
    if (n > 6 && !strncmp(b, "list<", 5) && b[n - 1] == '>') {
        b[n - 1] = 0;
        if (!xt_is_hl(at)) return 1;
        return xg_unify(g, b + 5, xt_items(at), bind, why, cap);
    }
    if (n > 6 && !strncmp(b, "dict<", 5) && b[n - 1] == '>') {
        int depth = 0;
        char *comma = NULL;
        b[n - 1] = 0;
        for (char *q = b + 5; *q && !comma; q++) { if (*q == '<') depth++; else if (*q == '>') depth--; else if (*q == ',' && !depth) comma = q; }
        if (!comma || !xt_is_dict(at)) return 1;
        *comma = 0;
        return xg_unify(g, b + 5, xt_dict_key(at), bind, why, cap) && xg_unify(g, comma + 1, xt_dict_val(at), bind, why, cap);
    }
    for (int j = 0; j < g->n; j++) {   // a type parameter where its type can't be read off (func<T, int>)
        const char *w = strstr(b, g->name[j]);
        size_t wl = strlen(g->name[j]);
        if (w && (w == b || !(isalnum((unsigned char)w[-1]) || w[-1] == '_')) && !(isalnum((unsigned char)w[wl]) || w[wl] == '_')) {
            snprintf(why, (size_t)cap, "%s can't be read off a%s %s - name it in a parameter of its own (T, T?, seq<T>, list<T>, dict<K, V>)", g->name[j],
                     xan(xty_name(ta, tn, sizeof tn)), tn);
            return 0;
        }
    }
    return 1;
}
// a call of a generic function (gf its lambda as written): its type parameters' types from the arguments, each held
// against its bound; then its copy for them - made once, put at the script's top as if declared there - and the call
// made one of the copy (n->s). 0 a mistake; 1 the call is the copy's now; 2 inside a generic function as written (its
// types its bounds: only held against them - its own copies make theirs)
// a call's (n, of g - name its name for the messages) type parameters' types from its arguments, each held against its
// bound: 0 a mistake
internal int xg_binds(struct xcheck *k, struct xnode *n, const struct xgen *g, const char *name, int *bind, const struct xscope *sc) {
    char why[200];
    if (n->n_items > g->np) { snprintf(k->whybuf, sizeof k->whybuf, "%s takes %d argument%s", name, g->np, g->np == 1 ? "" : "s"); return xc_fail(k, n, k->whybuf); }
    for (int i = 0; i < n->n_items; i++) {
        const struct xnode *a = n->items[i]->k == XN_REF ? n->items[i]->a : n->items[i];
        if (!xg_unify(g, g->ptype[i], x_stype(k, a, sc), bind, why, sizeof why)) {
            snprintf(k->whybuf, sizeof k->whybuf, "%s(...): its argument %d - %s", name, i + 1, why);
            return xc_fail(k, n, k->whybuf);
        }
    }
    for (int j = 0; j < g->n; j++) {
        if (!bind[j]) {
            snprintf(k->whybuf, sizeof k->whybuf, "%s(...): %s isn't told by the arguments - an argument of it whose type is known (null tells none: a let with its type first)",
                     name, g->name[j][0] == '#' ? "a parameter's type" : g->name[j]);
            return xc_fail(k, n, k->whybuf);
        }
        if (!xg_fits(bind[j], g->iface[j], why, sizeof why)) { snprintf(k->whybuf, sizeof k->whybuf, "%s(...): %s", name, why); return xc_fail(k, n, k->whybuf); }
        if (!g_xg_abstract && xst_is(bind[j]) && g_xst[bind[j] - XT_STRUCT].iface) {   // (a { ... } or [ ... ] typed by the interface)
            snprintf(k->whybuf, sizeof k->whybuf, "%s(...): its types can't be told from a value written out - a let with its type first", name);
            return xc_fail(k, n, k->whybuf);
        }
    }
    return 1;
}
// a copy's name: name<unit>, f<point, unit>
internal void xg_copy_name(const struct xgen *g, const char *name, const int *bind, char *nm, int cap) {
    char tn[60];
    int o = snprintf(nm, (size_t)cap, "%s<", name);
    for (int j = 0; j < g->n && o < cap; j++) o += snprintf(nm + o, (size_t)(cap - o), "%s%s", j ? ", " : "", xty_name(bind[j], tn, sizeof tn));
    if (o < cap) snprintf(nm + o, (size_t)(cap - o), ">");
}
internal int xg_call(struct xcheck *k, struct xnode *n, const struct xnode *gf, const struct xscope *sc) {
    struct xgen *g = gf->gen;
    int bind[XG_MAX] = { 0 };
    if (!xg_binds(k, n, g, n->s, bind, sc)) return 0;
    if (g_xg_abstract) return 2;
    struct xnode *let = NULL;
    for (int c = 0; c < g->ncopy && !let; c++) if (!memcmp(g->copy[c].bind, bind, sizeof bind)) let = g->copy[c].let;
    if (!let) {
        if (g->ncopy == XG_COPIES) { snprintf(k->whybuf, sizeof k->whybuf, "%s(...): more than %d copies - one for each call's types", n->s, XG_COPIES); return xc_fail(k, n, k->whybuf); }
        if (!g_xg_top || !g_xg_topsc) return xc_fail(k, n, "a generic function is called from a script");
        if (g_xg_topsc->n >= 64) { snprintf(k->whybuf, sizeof k->whybuf, "%s(...): more than 64 names at the script's top (each copy of a generic function one)", n->s); return xc_fail(k, n, k->whybuf); }
        if (!(let = x_parse_copy(g, bind))) { snprintf(k->whybuf, sizeof k->whybuf, "%s(...): its copy for these types doesn't read", n->s); return xc_fail(k, n, k->whybuf); }
        char nm[160];   // its name: nearest<unit>, f<point, unit> - once at the top
        xg_copy_name(g, n->s, bind, nm, sizeof nm);
        int o = (int)strlen(nm);
        for (int tries = 2; xsc_find(g_xg_topsc, nm, strlen(nm), NULL) && tries < 99 && o + 4 < (int)sizeof nm; tries++) snprintf(nm + o, sizeof nm - (size_t)o, "#%d", tries);
        let->s = _strdup(nm);
        struct xnode **it = realloc(g_xg_top->items, sizeof *it * (size_t)(g_xg_top->n_items + 1));
        if (!it || !let->s) return xc_fail(k, n, "no memory");
        g_xg_top->items = it;
        it[g_xg_top->n_items++] = let;
        struct xscope *ts = g_xg_topsc;
        ts->name[ts->n] = let->s; ts->decl[ts->n] = 1; ts->kind[ts->n] = (short)XT_FN; ts->fn[ts->n] = let->a; ts->n++;
        memcpy(g->copy[g->ncopy].bind, bind, sizeof bind);
        g->copy[g->ncopy++].let = let;
    }
    if (!n->s0) n->s0 = n->s;
    n->s = let->s;
    return 1;
}
// a call of a struct's generic method m (n: a.b.name(...) or <value>.name(...), st its struct): its copy for the
// arguments' types - a method of the struct of its own, made once (its name add<cell>; checked with the struct's
// methods, x_check_body) - and the call made one of it (n->s). 0 a mistake; 1 *copy the copy's function; 2 inside a
// generic function or method as written (only held against the bounds), or of an interface's method (a T's)
internal int xg_mcall(struct xcheck *k, struct xnode *n, int st, const struct xnode *m, const char *mname, const struct xscope *sc, const struct xnode **copy) {
    struct xgen *g = m->gen;
    int bind[XG_MAX] = { 0 };
    if (!xg_binds(k, n, g, mname, bind, sc)) return 0;
    if (g_xg_abstract || g_xst[st - XT_STRUCT].iface) return 2;
    struct xnode *c = NULL, *d = (struct xnode *)g_xst[st - XT_STRUCT].decl;
    for (int i = 0; i < g->ncopy && !c; i++) if (!memcmp(g->copy[i].bind, bind, sizeof bind)) c = g->copy[i].let;
    if (!c) {
        char nm[160];
        if (g->ncopy == XG_COPIES) { snprintf(k->whybuf, sizeof k->whybuf, "%s(...): more than %d copies - one for each call's types", mname, XG_COPIES); return xc_fail(k, n, k->whybuf); }
        if (!(c = x_parse_copy(g, bind))) { snprintf(k->whybuf, sizeof k->whybuf, "%s(...): its copy for these types doesn't read", mname); return xc_fail(k, n, k->whybuf); }
        xg_copy_name(g, mname, bind, nm, sizeof nm);
        struct xnode **it = realloc(d->items, sizeof *it * (size_t)(d->n_items + 1));
        if (!it || !(c->s = _strdup(nm))) return xc_fail(k, n, "no memory");
        d->items = it;
        it[d->n_items++] = c;
        memcpy(g->copy[g->ncopy].bind, bind, sizeof bind);
        g->copy[g->ncopy++].let = c;
    }
    if (!n->s0) n->s0 = n->s;
    if (n->k == XN_CALL) {   // a.b.add -> a.b.add<cell>
        const char *dot = strrchr(n->s, '.');
        size_t l = dot ? (size_t)(dot - n->s) + 1 : 0, cl = strlen(c->s);
        char *w = malloc(l + cl + 1);
        if (!w) return xc_fail(k, n, "no memory");
        memcpy(w, n->s, l);
        memcpy(w + l, c->s, cl + 1);
        n->s = w;
    } else n->s = c->s;
    *copy = c;
    return 1;
}
// the type a function's or a method's parameter i (from 0: fd's own items, a method's this first) wants of a list or
// a { ... } written as its argument: its own - none (0) for a generic one's of a type parameter (GENERICS: the
// argument tells the type)
internal int xg_want(const struct xnode *fd, int i) {
    if (!fd || i >= fd->n_items) return 0;
    int t = (int)fd->items[i]->n;
    if (!fd->gen) return t;
    for (int depth = 0; depth < 4 && t && !xst_is(X_TBASE(t)); depth++) t = xt_is_dict(t & ~X_NULLABLE) ? xt_dict_val(t & ~X_NULLABLE) : xt_items(t & ~X_NULLABLE);
    return t && xst_is(X_TBASE(t)) && g_xst[X_TBASE(t) - XT_STRUCT].iface ? 0 : (int)fd->items[i]->n;
}
internal int x_check_struct(struct xcheck *k, const struct xnode *n, struct xscope *sc) {
    int t = (int)n->n, nf = 0;
    char a[40], b[40];
    if (xsc_find(sc, n->s, strlen(n->s), NULL) || x_name_known(n->s) || x_script_fn(n->s) >= 0 || g_x_host.command(n->s) || !strcmp(n->s, "time") || !strcmp(n->s, "stop")) {
        snprintf(k->whybuf, sizeof k->whybuf, "struct %s: %s is a name already - give the struct another", n->s, n->s);
        return xc_fail(k, n, k->whybuf);
    }
    for (int i = 0; i <= 8; i++) if (x_fn_id(n->s, i)) return xc_fail(k, n, "a struct's name: a function of the language has it");
    for (int i = 0; i < n->n_items; i++) {
        const struct xnode *f = n->items[i];
        if (f->op == 'M') continue;   // (its code: after the script's top - x_check_body)
        int ft = (int)f->n;
        nf++;
        if (xst_is(ft) && xst_holds(ft, t, 0)) {
            snprintf(k->whybuf, sizeof k->whybuf, "struct %s: its %s holds a %s again, without end - %s? (one or null) or list<%s> ends it", n->s, f->s, n->s, n->s, n->s);
            return xc_fail(k, f, k->whybuf);
        }
        if (!f->a) continue;
        struct xscope none = { NULL, { 0 }, 0, { 0 }, 0, { 0 }, { 0 } };   // (a default sees none of the script's names: it is worked out wherever one is made)
        X_WANT(f->a, ft);
        if (!x_check(k, f->a, &none)) return 0;
        int tv = x_stype(k, f->a, &none);
        if (xt_fits(ft, tv)) continue;
        snprintf(k->whybuf, sizeof k->whybuf, "%s: %s = ...: a%s %s can't go in%s", f->s, xty_name(ft, a, sizeof a), xan(xty_name(tv, b, sizeof b)), b, xt_hint(ft, tv));
        return xc_fail(k, f, k->whybuf);
    }
    if (nf > 32) return xc_fail(k, n, "a struct of more than 32 fields");
    for (const char *w = n->cs; w && *w; ) {   // struct point: ipoint { ... } - what it promises, held to it here (GENERICS)
        while (*w == ' ' || *w == ',' || *w == '\t') w++;
        char nm[40], why[200];
        int q = 0;
        while ((isalnum((unsigned char)*w) || *w == '_') && q < 39) nm[q++] = *w++;
        nm[q] = 0;
        if (!q) { if (*w) return xc_fail(k, n, "struct name: an interface, another { ... } - the interfaces' names after the :"); break; }
        int si = xst_find(nm);
        if (si < 0 || !g_xst[si].iface) { snprintf(k->whybuf, sizeof k->whybuf, "struct %s: %s - no interface (interface %s { ... })", n->s, nm, nm); return xc_fail(k, n, k->whybuf); }
        if (!g_xst[si].decl) continue;   // (one declared twice: told there)
        if (!xg_fits(t, XT_STRUCT + si, why, sizeof why)) { snprintf(k->whybuf, sizeof k->whybuf, "struct %s: %s - %s", n->s, nm, why); return xc_fail(k, n, k->whybuf); }
    }
    return 1;
}
// an entry's key of a { ... } written as a constant (a name, ["a"], [3]): 1 a text (*s), 2 a number (*v), 0 worked out
internal int x_key_const(const struct xnode *e, const char **s, double *v) {
    const struct xnode *x = e->b;
    if (!x) return 0;
    if (x->k == XN_STR && !strchr(x->s, '{')) { *s = x->s; return 1; }
    if (x->k == XN_NUM && x->op == 'i') { *v = x->n; return 2; }
    return 0;
}
// { name = value, [key] = value, ... } (a script's): WHAT THE PLACE IT GOES INTO IS (the type wanted there: X_WANT) -
// a dict<K, V>'s entries ([key] = value, a key a value: ["stone"], [p2] - a name is a field, never a text; its
// default V's own - 0, "", false,
// null, a struct's defaults - or dict<K, V>(default, { ... })'s), a struct's fields by name (the others their
// defaults), else a record (its members names). Made a dict or a struct it is marked so (op 'D' / 'S', its type in n)
internal int x_check_rec(struct xcheck *k, const struct xnode *n, struct xscope *sc) {
    int want = k->script ? g_x_want & ~X_NULLABLE : 0, own = n->sk & 1;   // (own: its dict<K, V>(default, { ... }) gives the default)
    char a[60], b[60], c[60], l[120];
    g_x_want = 0;
    if ((want & X_LIST_OF) || (!xt_is_dict(want) && !xst_is(want))) want = 0;
    ((struct xnode *)n)->op = (unsigned char)(!want ? 0 : xt_is_dict(want) ? 'D' : 'S');
    ((struct xnode *)n)->n = want;
    if (!want) {   // a record
        for (int i = 0; i < n->n_items; i++) {
            if (n->items[i]->b) return xc_fail(k, n->items[i], "{ [key] = ... }: a dict's entry - where a dict<K, V> goes (let d: dict<int, text> = { [1] = \"a\" }); a record's members are names");
            if (!x_check(k, n->items[i]->a, sc)) return 0;
        }
        return 1;
    }
    xty_name(want, a, sizeof a);
    if (xst_is(want) && g_xst[want - XT_STRUCT].iface) {   // (inside a generic function: its T)
        snprintf(k->whybuf, sizeof k->whybuf, "{ ... } as a %s: an interface - no value is made of one", a);
        return xc_fail(k, n, k->whybuf);
    }
    if (g_x_in_ph) {
        snprintf(k->whybuf, sizeof k->whybuf, "a %s made in a {placeholder} - let v: %s = { ... } first, then {v}", a, a);
        return xc_fail(k, n, k->whybuf);
    }
    if (xt_is_dict(want)) {
        int kt = xt_dict_key(want), vt = xt_dict_val(want);
        xty_name(vt, b, sizeof b);
        xty_name(kt, c, sizeof c);
        if (!own && !(vt & X_NULLABLE) && (xt_is_hl(vt) || xt_is_dict(vt) || !xst_has_default(vt, 0))) {
            if (xt_is_hl(vt) || xt_is_dict(vt))
                snprintf(k->whybuf, sizeof k->whybuf, "{ ... } as a %s: its default would be one %s, the same for every key without an entry - values of %s? (null the default)",
                         a, xt_is_hl(vt) ? "list" : "dict", b);
            else snprintf(k->whybuf, sizeof k->whybuf, "{ ... } as a %s: a%s %s has no default for a key without an entry - %s(default, { ... }) gives it", a, xan(b), b, a);
            return xc_fail(k, n, k->whybuf);
        }
        for (int i = 0; i < n->n_items; i++) {
            const struct xnode *e = n->items[i];
            const char *ks, *ks2;
            double kv, kv2;
            int tk, tv, kc = x_key_const(e, &ks, &kv);
            char tn[60];
            if (!e->b) {   // a word is no stand-in for a text (the player 2026-11-21): name = is a field's
                if (kt == XT_TEXT)
                    snprintf(k->whybuf, sizeof k->whybuf, "{ %s = ... }: a name is a record's or a struct's field - a %s's entry is [key] = value: [\"%s\"] = ..., [%s] = ... for the variable's value",
                             e->s, a, e->s, e->s);
                else snprintf(k->whybuf, sizeof k->whybuf, "{ %s = ... }: a name is a record's or a struct's field - a %s's entry is [key] = value, its key a%s %s",
                              e->s, a, xan(c), c);
                return xc_fail(k, e, k->whybuf);
            }
            if (!x_check(k, e->b, sc)) return 0;
            tk = x_stype(k, e->b, sc);
            if (!xt_fits(kt, tk)) {
                snprintf(k->whybuf, sizeof k->whybuf, "{ [...] = ... }: the %s's keys are %ss - not a%s %s%s", a, c, xan(xty_name(tk, tn, sizeof tn)), tn, xt_hint(kt, tk));
                return xc_fail(k, e, k->whybuf);
            }
            for (int j = 0; j < i && kc; j++)
                if (x_key_const(n->items[j], &ks2, &kv2) == kc && (kc == 1 ? !strcmp(ks, ks2) : kv == kv2)) {
                    if (kc == 1) snprintf(k->whybuf, sizeof k->whybuf, "{ ... }: the key \"%.40s\" twice", ks);
                    else snprintf(k->whybuf, sizeof k->whybuf, "{ ... }: the key %g twice", kv);
                    return xc_fail(k, e, k->whybuf);
                }
            X_WANT(e->a, vt);
            if (!x_check(k, e->a, sc)) return 0;
            tv = x_stype(k, e->a, sc);
            if (xt_fits(vt, tv)) continue;
            snprintf(k->whybuf, sizeof k->whybuf, "{ [...] = ... }: the %s's values are %ss - a%s %s can't go in%s", a, b, xan(xty_name(tv, tn, sizeof tn)), tn, xt_hint(vt, tv));
            return xc_fail(k, e, k->whybuf);
        }
        return 1;
    }
    xst_list(want, 'F', l, sizeof l);   // a struct: its fields by name
    for (int i = 0; i < n->n_items; i++) {
        const struct xnode *e = n->items[i], *f = e->b ? NULL : xst_member(want, e->s, 'F', NULL);
        int ft = f ? (int)f->n : 0, tv;
        if (e->b) snprintf(k->whybuf, sizeof k->whybuf, "{ [...] = ... }: a %s's fields are names - its fields: %s", a, l);
        else if (!f && xst_member(want, e->s, 'M', NULL)) snprintf(k->whybuf, sizeof k->whybuf, "{ %s = ... }: %s is a %s's method - its fields: %s", e->s, e->s, a, l);
        else if (!f) snprintf(k->whybuf, sizeof k->whybuf, "{ %s = ... }: a %s has no field %s - its fields: %s", e->s, a, e->s, l);
        else {
            if (g_xr_out && g_xrc && !g_xr_busy) xr_member_at(e->at, e->s, strlen(e->s), want, 0);   // (the language server's record: the field)
            X_WANT(e->a, ft);
            if (!x_check(k, e->a, sc)) return 0;
            tv = x_stype(k, e->a, sc);
            if (xt_fits(ft, tv)) continue;
            snprintf(k->whybuf, sizeof k->whybuf, "{ %s = ... }: the field is a%s %s - a%s %s can't go in%s", e->s, xan(xty_name(ft, b, sizeof b)), b,
                     xan(xty_name(tv, c, sizeof c)), c, xt_hint(ft, tv));
        }
        return xc_fail(k, e, k->whybuf);
    }
    const struct xnode *d = g_xst[want - XT_STRUCT].decl;
    for (int i = 0; i < d->n_items; i++) {   // a field left out: its default (one of a type with none is given)
        const struct xnode *f = d->items[i];
        int given = 0;
        if (f->op != 'F' || f->a || xst_has_default((int)f->n, 0)) continue;
        for (int j = 0; j < n->n_items && !given; j++) given = !strcmp(n->items[j]->s, f->s);
        if (given) continue;
        snprintf(k->whybuf, sizeof k->whybuf, "{ ... } as a %s: its %s (a%s %s) has no default - given here (%s = ...), or declared: %s: %s = ...", a, f->s,
                 xan(xty_name((int)f->n, b, sizeof b)), b, f->s, f->s, b);
        return xc_fail(k, n, k->whybuf);
    }
    return 1;
}
internal int x_check(struct xcheck *k, const struct xnode *n, struct xscope *sc) {
    if (!n) return 1;
    if (n->s0) ((struct xnode *)n)->s = n->s0;   // (a generic's call: of the copy this check makes it - GENERICS)
    if (g_xr_out && g_xrc && !g_xr_busy && n->at) xr_node(k, n, sc);   // (the language server's record)
    switch (n->k) {
    case XN_NUM: return 1;
    case XN_STR: {
        const char *why;
        g_x_check_line = k->script ? n->line : g_x_check_line;
        struct xr_ctx xs = { XR_STR, n->s, strlen(n->s), NULL, 0, 0, n->at, NULL, NULL };   // (its placeholders: where they are)
        if (g_xr_out && g_xrc) xr_push(&xs);
        int ok = x_fills_ok(n->s, &why, k, sc);   // its placeholders see the names around it
        if (g_xrc == &xs) g_xrc = xs.up;
        if (!ok) return xc_fail(k, n, why);
        return 1;
    }
    case XN_NAME: {
        size_t len = strcspn(n->s, ".");
        int f = xsc_find(sc, n->s, len, NULL);
        if (f == 2) { snprintf(k->whybuf, sizeof k->whybuf, "%.*s is read before its let", (int)len, n->s); return xc_fail(k, n, k->whybuf); }
        if (f && x_ref_far(k, n, n->s, len, sc)) return 0;
        if ((k->script || k->texts) && n->s[len]) {   // p.towers, game.players: the members its kind has (a kind known)
            int known;
            if (x_path_kind(k, n, n->s, strlen(n->s), sc, &known) < 0) return 0;
            if (known) return 1;
        }
        if (f && k->script && !n->s[len]) {   // a generic function is called, never a value (GENERICS: its copies are made by calls)
            const struct xnode *gd = xsc_fn(sc, n->s, len);
            if (gd && gd->gen) { snprintf(k->whybuf, sizeof k->whybuf, "%s is a generic function - it is called (its types from the call's arguments), no value", n->s); return xc_fail(k, n, k->whybuf); }
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
        struct xscope s = { sc, { 0 }, 0, { 0 }, 0, { 0 }, { 0 } };
        for (int i = 0; i < n->n_items; i++) {   // its parameters: none hides a name around it either
            const struct xnode *pi = n->items[i];
            if (!x_check_new_name(k, pi, pi->s, sc)) return 0;
            int ty = (int)pi->n ? (int)pi->n : k->ctx;   // its type: as written, else where the function goes (every's run)
            if (!ty && k->script) {   // a script's: written where nothing tells it
                snprintf(k->whybuf, sizeof k->whybuf, "%s: a parameter's type - (%s: int) => ... (int, float, text, player ...)", pi->s, pi->s);
                return xc_fail(k, pi, k->whybuf);
            }
            if ((int)pi->n && k->ctx && !xt_fits((int)pi->n, k->ctx)) {   // every's run as a text
                char a[40], b[40];
                snprintf(k->whybuf, sizeof k->whybuf, "%s: %s - it gets a%s %s here", pi->s, xty_name((int)pi->n, a, sizeof a),
                         xan(xty_name(k->ctx, b, sizeof b)), b);
                return xc_fail(k, pi, k->whybuf);
            }
            if (pi->a && k->script) {   // its default: a value of its type that needs none of the script's names (it is worked
                struct xscope none = { NULL, { 0 }, 0, { 0 }, 0, { 0 }, { 0 } };   // out where a call leaves it out)
                X_WANT(pi->a, ty);
                if (!x_check(k, pi->a, &none)) return 0;
                int tv = x_stype(k, pi->a, &none);
                if (!xt_fits(ty, tv)) {
                    char a[40], b[40];
                    snprintf(k->whybuf, sizeof k->whybuf, "%s: %s = ...: a%s %s can't go in%s", pi->s, xty_name(ty, a, sizeof a), xan(xty_name(tv, b, sizeof b)), b, xt_hint(ty, tv));
                    return xc_fail(k, pi, k->whybuf);
                }
            }
            xsc_add(&s, pi->s);
            if (g_xr_out) xr_decl(pi->s, pi, ty);
            if (s.n) s.kind[s.n - 1] = (short)ty;
            if (s.n && pi->op == 'r') s.decl[s.n - 1] |= 2;   // a ref: a place
        }
        int loops = k->loops, ctx = k->ctx, low = k->nn_low;   // (a break in it is none of a loop around it; a function in it
        k->loops = k->ctx = 0;                                 // has no context; it runs later: a != null around it holds no
        k->nn_low = k->n_nn;                                   // more then)
        int blk = n->a && n->a->k == XN_BLOCK;
        const struct xnode *fn0 = g_x_ret_fn;   // the function a return in it is of
        g_x_ret_fn = n;
        k->body += blk;
        X_WANT(n->a, (int)n->n);   // (=> [a, b] of a result's type written)
        int ok = x_check_body(k, n->a, &s);
        k->body -= blk;
        g_x_ret_fn = fn0;
        k->loops = loops;
        k->ctx = ctx;
        k->nn_low = low;
        if (ok && k->script && (int)n->n) {   // its result's type written: what it gives back fits
            int tw = (int)n->n, tv = X_ANY;
            char a[40], b[40];
            if (!blk && !xt_fits(tw, tv = x_stype(k, n->a, &s))) {
                snprintf(k->whybuf, sizeof k->whybuf, "a function of a%s %s gives back a%s %s%s", xan(xty_name(tw, a, sizeof a)), a, xan(xty_name(tv, b, sizeof b)), b, xt_hint(tw, tv));
                return xc_fail(k, n->a, k->whybuf);
            }
            if (blk && !(tw & X_NULLABLE) && !x_returns(n->a)) {
                snprintf(k->whybuf, sizeof k->whybuf, "a function of a%s %s may end without a return - every way through it hands one back (or its type is %s?: none)",
                         xan(xty_name(tw, a, sizeof a)), a, a);
                return xc_fail(k, n, k->whybuf);
            }
        }
        return ok;
    }
    case XN_BLOCK: {   // a script's statements (its top)
        return x_check_body(k, n, sc);
    }
    case XN_STRUCT: return x_check_struct(k, n, sc);
    case XN_HLIST: {   // list<T>(...): of a seq's or a list's items (a [...] there is made the list itself), or none
        int ta;
        char a[60], b[60], c[60];
        if (!k->script) return xc_fail(k, n, "list<T>(...): a script's - a text holds no list");
        if (g_x_in_ph) return xc_fail(k, n, "a list made in a {placeholder} - let l = list<T>(...) first, then {l}");
        if (!n->a) return 1;
        X_WANT(n->a, (int)n->n);
        if (!x_check(k, n->a, sc)) return 0;
        if (n->a->k == XN_LIST) return 1;
        ta = x_stype(k, n->a, sc);
        if (!ta || (!(ta & X_NULLABLE) && xt_items(ta) && xt_fits(xt_hl_elem((int)n->n), xt_items(ta)))) return 1;
        snprintf(k->whybuf, sizeof k->whybuf, "%s(...): of a seq or a list of %ss - not of a%s %s", xty_name((int)n->n, a, sizeof a), xty_name(xt_hl_elem((int)n->n), b, sizeof b),
                 xan(xty_name(ta, c, sizeof c)), c);
        return xc_fail(k, n, k->whybuf);
    }
    case XN_DICT: {   // dict<K, V>(default): the default a V - a list or a dict as one would be every missing key's one table: null
        int vt = xt_dict_val((int)n->n), ta;
        char a[60], b[60], c[60];
        if (!k->script && !xt_plain_val(vt)) return xc_fail(k, n, "dict<K, V>(...): a text's dict holds an int, a float, a bool or a text");
        X_WANT(n->a, vt);
        if (!x_check(k, n->a, sc)) return 0;
        ta = x_stype(k, n->a, sc);
        if (!xt_fits(vt, ta))
            snprintf(k->whybuf, sizeof k->whybuf, "%s(...): its default is what a key without an entry holds - a%s %s, not a%s %s%s", xty_name((int)n->n, a, sizeof a),
                     xan(xty_name(vt, b, sizeof b)), b, xan(xty_name(ta, c, sizeof c)), c, xt_hint(vt, ta));
        else if ((xt_is_hl(vt) || xt_is_dict(vt)) && !(vt & X_NULLABLE))
            snprintf(k->whybuf, sizeof k->whybuf, "%s(...): its default would be one %s, the same for every key without an entry - values of %s? and null the default (d[k] = %s(...) where a key is new)",
                     xty_name((int)n->n, a, sizeof a), xt_is_hl(vt) ? "list" : "dict", xty_name(vt, b, sizeof b), xty_name(vt, c, sizeof c));
        else if (!n->b) return 1;
        else if (!k->script) return xc_fail(k, n, "dict<K, V>(default, { ... }): a script's - a text's dict starts without entries");
        else if (n->b->k != XN_REC) return xc_fail(k, n, "dict<K, V>(default, { ... }): its entries written out - { [key] = value, ... }");
        else {   // its entries: a { ... } made the dict (its default this one: marked)
            ((struct xnode *)n->b)->sk |= 1;
            X_WANT(n->b, (int)n->n);
            return x_check(k, n->b, sc);
        }
        return xc_fail(k, n, k->whybuf);
    }
    case XN_REF: return xc_fail(k, n, "ref ...: only as an argument, to a parameter that is a ref (name(ref p: type) { ... }), or as ref name = a place");
    case XN_LET:
        X_WANT(n->a, n->op == 'r' ? 0 : (int)n->n);
        if (!x_check(k, n->a, sc)) return 0;   // (its own name not yet: let n = n + 1 is a mistake)
        if (n->op == 'r') {   // ref q = l[2]: a second name for the place - its type the place's (written: the very one)
            int tw = (int)n->n, tv = x_stype(k, n->a, sc);
            char a[40], b[40];
            if (!x_place(k, n->a, sc)) { snprintf(k->whybuf, sizeof k->whybuf, "ref %s = ...: a place - " X_PLACE_WHY, n->s); return xc_fail(k, n, k->whybuf); }
            if (x_loop_copy(k, n, x_via_hl(k, n->a, sc) ? "" : x_root_name(n->a), sc)) return 0;
            if (tw && tv && tw != tv) {
                snprintf(k->whybuf, sizeof k->whybuf, "ref %s: %s = ...: the place is a%s %s's", n->s, xty_name(tw, a, sizeof a), xan(xty_name(tv, b, sizeof b)), b);
                return xc_fail(k, n, k->whybuf);
            }
            xsc_pass(sc, n->s, tw ? tw : tv, NULL);
            return 1;
        }
        {
            int tw = (int)n->n, tv = x_stype(k, n->a, sc);   // its type: as written (let n: float = 5), else its value's
            if (!xt_fits(tw, tv)) {
                char a[40], b[40];
                snprintf(k->whybuf, sizeof k->whybuf, "let %s: %s = ...: a%s %s can't go in%s", n->s, xty_name(tw, a, sizeof a),
                         xan(xty_name(tv, b, sizeof b)), b, xt_hint(tw, tv));
                return xc_fail(k, n, k->whybuf);
            }
            xsc_pass(sc, n->s, tw ? tw : tv, n->a && n->a->k == XN_LAMBDA ? n->a : NULL);
        }
        return 1;
    case XN_ASSIGN: {
        int f = xsc_find(sc, n->s, strlen(n->s), NULL);   // (the code around's too: set in its own frame)
        if (!f && !k->script && x_field_known(n->s)) return x_check(k, n->a, sc);   // a personality's field (let at its top)
        const struct xnode *fd = f ? xsc_fn(sc, n->s, strlen(n->s)) : NULL;
        if (fd && fd->op == 'D') snprintf(k->whybuf, sizeof k->whybuf, "%s = ...: %s is a function declared with %s(...) - only a let's variable is set", n->s, n->s, n->s);
        else if (!f) snprintf(k->whybuf, sizeof k->whybuf, "%s = ...: no variable %s - let %s = ... makes one", n->s, n->s, n->s);
        else if (f == 2) snprintf(k->whybuf, sizeof k->whybuf, "%s is set before its let", n->s);
        else if (x_ref_far(k, n, n->s, strlen(n->s), sc)) return 0;
        else {
            X_WANT(n->a, xsc_kind(sc, n->s, strlen(n->s)));
            if (!x_check(k, n->a, sc)) return 0;
            int kv = xsc_kind(sc, n->s, strlen(n->s)), kr = x_stype(k, n->a, sc);
            xsc_fn_set(sc, n->s, NULL);   // the function it holds: not known any more (set in an if, a loop ...)
            xn_strike(k, n->s);   // nor that it isn't null - unless what it gets never is (q = p.lord ?? p: there below)
            if (xt_fits(kv, kr) && (kv & X_NULLABLE) && kr && !(kr & X_NULLABLE) && k->n_nn < 32) { k->nn[k->n_nn] = n->s; k->nl[k->n_nn++] = (unsigned char)strlen(n->s); }
            if (xt_fits(kv, kr)) return 1;   // a type it has from its let: another can't go in (an int a float can't)
            char tv[40], tr[40];
            snprintf(k->whybuf, sizeof k->whybuf, "%s is a%s %s (its let) - a%s %s can't go in%s", n->s, xan(xty_name(kv, tv, sizeof tv)), tv,
                     xan(xty_name(kr, tr, sizeof tr)), tr, xt_hint(kv, kr));
        }
        return xc_fail(k, n, k->whybuf);
    }
    case XN_IF: {   // each branch where its condition holds (if p.lord != null { p.lord.name })
        int keep = k->n_nn, ok;
        if (!x_check(k, n->c, sc)) return 0;
        xn_facts(k, n->c, 1);
        ok = x_check_block(k, n->a, sc);
        k->n_nn = keep;
        if (!ok) return 0;
        xn_facts(k, n->c, 0);
        ok = !n->b || (n->b->k == XN_IF ? x_check(k, n->b, sc) : x_check_block(k, n->b, sc));
        k->n_nn = keep;
        return ok;
    }
    case XN_AND: case XN_OR: case XN_COND: {   // the right side where the left holds: p.lord != null and p.lord.towers > 0
        int keep = k->n_nn, ok;
        const struct xnode *c = n->k == XN_COND ? n->c : n->a;
        if (!x_check(k, c, sc)) return 0;
        xn_facts(k, c, n->k != XN_OR);
        ok = x_check(k, n->k == XN_COND ? n->a : n->b, sc);
        k->n_nn = keep;
        if (ok && n->k == XN_COND) { xn_facts(k, c, 0); ok = x_check(k, n->b, sc); k->n_nn = keep; }
        return ok;
    }
    case XN_WHILE: case XN_FOR: {
        if (!x_check(k, n->k == XN_WHILE ? n->c : n->a, sc) || !x_check(k, n->b, sc)) return 0;
        int keep = k->n_nn;
        if (n->k == XN_WHILE) xn_facts(k, n->c, 1);   // while l != null { ... }
        struct xscope b = { sc, { 0 }, 0, { 0 }, 1, { 0 }, { 0 } };
        if (n->k == XN_FOR) {   // its name: a list's items' kind (for p in game.players: a player)
            if (!x_check_new_name(k, n, n->s, sc)) return 0;
            xsc_add(&b, n->s);
            if (g_xr_out) xr_decl(n->s, n, 0);
            b.decl[0] |= 4;   // the loop's copy of an item
            int lk = n->b ? X_ANY : x_stype(k, n->a, sc);   // a range's: an int from ints, else a float
            if (xt_is_dict(lk)) return xc_fail(k, n, "for ... in a dict: a dict's entries have no order to go through - a list of its keys beside it does (keys.add(k) where one is new)");
            if (n->b) { int ta = x_stype(k, n->a, sc), tb = x_stype(k, n->b, sc); b.kind[0] = (short)(ta == XT_INT && tb == XT_INT ? XT_INT : xt_numeric(ta) && xt_numeric(tb) ? XT_FLOAT : X_ANY); }
            else b.kind[0] = (short)(lk & X_NULLABLE ? X_ANY : xt_items(lk));
        }
        k->loops++;
        int ok = x_check_body(k, n->k == XN_WHILE ? n->a : n->c, &b);
        k->loops--;
        k->n_nn = keep;
        return ok;
    }
    case XN_BREAK: case XN_CONTINUE:
        return k->loops ? 1 : xc_fail(k, n, n->k == XN_BREAK ? "break outside a loop" : "continue outside a loop");
    case XN_SETM: {   // slot.persona = ...: a member its kind lets be set (a kind known)
        if (!x_check(k, n->a, sc)) return 0;
        if (n->b && (n->b->k == XN_LIST || n->b->k == XN_REC)) {   // a list or a { ... } written into a struct's field: of the field's type
            const struct xnode *f = xst_member(x_stype(k, n->a, sc) & ~X_NULLABLE, n->s, 'F', NULL);
            g_x_want = f ? (int)f->n : 0;
        }
        if (!x_check(k, n->b, sc)) return 0;
        int a = x_stype(k, n->a, sc) & ~X_NULLABLE;
        const struct xctype *rc = xct_of(a);
        if (k->script && (a == XT_REC || (rc && rc->kind == XCT_REC && !(a & X_LIST_OF)))) {
            snprintf(k->whybuf, sizeof k->whybuf, ".%s = ...: a record's members aren't set - a struct's fields are (struct name { %s: ... }), or a new record goes in its place: r = { ... }", n->s, n->s);
            return xc_fail(k, n, k->whybuf);
        }
        if (!a || a == XT_REC) return 1;
        const struct x_kmember *m = a & X_LIST_OF || a >= XT_INT ? NULL : x_kind_member(a, n->s);
        char t[100], tv[40], tm[40];
        int tr = x_stype(k, n->b, sc);
        if (xst_is(a)) {   // a.x = 5 of a struct: its field, in a place - the value there made anew with it (marked for the compiler)
            const struct xnode *f = xst_member(a, n->s, 'F', NULL);
            int ft = f ? (int)f->n : 0;
            if (g_xst[a - XT_STRUCT].iface)   // (a generic function's T: one of the game's objects or a struct's copy - not yet)
                snprintf(k->whybuf, sizeof k->whybuf, ".%s = ...: through an interface (%s) a field is read, not set", n->s, xty_name(a, tv, sizeof tv));
            else if (x_stype(k, n->a, sc) & X_NULLABLE) snprintf(k->whybuf, sizeof k->whybuf, ".%s = ...: a %s that may be null - check it != null first", n->s, xty_name(a, tv, sizeof tv));
            else if (!f) { xst_list(a, 'F', t, sizeof t); snprintf(k->whybuf, sizeof k->whybuf, ".%s = ...: a %s has no field %s - its fields: %s", n->s, xty_name(a, tv, sizeof tv), n->s, t); }
            else if (!x_place(k, n->a, sc)) snprintf(k->whybuf, sizeof k->whybuf, ".%s = ...: set in " X_PLACE_WHY, n->s);
            else if (x_loop_copy(k, n, x_via_hl(k, n->a, sc) ? "" : x_root_name(n->a), sc)) return 0;
            else if (n->op && (!xt_numeric(ft) || !xt_numeric(tr)))
                snprintf(k->whybuf, sizeof k->whybuf, ".%s %c= ...: numbers only - the field is a%s %s, the value a%s %s", n->s, n->op,
                         xan(xty_name(ft, tm, sizeof tm)), tm, xan(xty_name(tr, tv, sizeof tv)), tv);
            else {
                if (n->op) tr = ft == XT_INT && tr == XT_INT && n->op != '/' ? XT_INT : XT_FLOAT;
                if (xt_fits(ft, tr)) { ((struct xnode *)n)->sk = 1; return 1; }
                snprintf(k->whybuf, sizeof k->whybuf, ".%s = ...: the field is a%s %s - a%s %s can't go in%s", n->s, xan(xty_name(ft, tm, sizeof tm)), tm,
                         xan(xty_name(tr, tv, sizeof tv)), tv, xt_hint(ft, tr));
            }
            return xc_fail(k, n, k->whybuf);
        }
        if (n->op && m && m->set) {   // l[1].hp -= 50: the member's now op b, as + - * / type it
            if (!xt_numeric(m->result) || !xt_numeric(tr)) {
                snprintf(k->whybuf, sizeof k->whybuf, ".%s %c= ...: numbers only - a %s's %s is a%s %s, the value a%s %s", n->s, n->op,
                         x_kind_name(a), n->s, xan(xty_name(m->result, tm, sizeof tm)), tm, xan(xty_name(tr, tv, sizeof tv)), tv);
                return xc_fail(k, n, k->whybuf);
            }
            tr = m->result == XT_INT && tr == XT_INT && n->op != '/' ? XT_INT : XT_FLOAT;
        }
        if (a & X_LIST_OF) snprintf(k->whybuf, sizeof k->whybuf, ".%s = ...: a %s - one of them: [1].%s", n->s, xty_name(a, tv, sizeof tv), n->s);
        else if (a >= XT_INT) snprintf(k->whybuf, sizeof k->whybuf, ".%s = ...: a %s has no members to set", n->s, xty_name(a, tv, sizeof tv));
        else if (!m) { x_kind_list(a, t, sizeof t); snprintf(k->whybuf, sizeof k->whybuf, ".%s = ...: a %s has no %s - its members: %s", n->s, x_kind_name(a), n->s, t); }
        else if (!m->set) snprintf(k->whybuf, sizeof k->whybuf, ".%s = ...: a %s's %s can't be set", n->s, x_kind_name(a), n->s);
        else if (!xt_fits(m->result, tr))   // p.gold = 10.5: an int's place
            snprintf(k->whybuf, sizeof k->whybuf, ".%s = ...: a %s's %s is a%s %s - a%s %s can't go in%s", n->s, x_kind_name(a), n->s,
                     xan(xty_name(m->result, tm, sizeof tm)), tm, xan(xty_name(tr, tv, sizeof tv)), tv,
                     xt_hint(m->result, tr));
        else return 1;
        return xc_fail(k, n, k->whybuf);
    }
    case XN_RETURN: {   // what its function gives back: of the type written for it - a declared function or a method writes one
        X_WANT(n->a, g_x_ret_fn ? (int)g_x_ret_fn->n : 0);
        if (!x_check(k, n->a, sc)) return 0;
        const struct xnode *fn = g_x_ret_fn;
        int tw = fn ? (int)fn->n : X_ANY, tv = n->a ? x_stype(k, n->a, sc) : X_ANY;
        char a[40], b[40];
        if (!k->script || !fn) return 1;
        if (!tw && n->a && (fn->op == 'D' || fn->op == 'M'))
            return xc_fail(k, n, "return <value>: the function writes what it gives back after its parameters - name(a: int): float { ... }");
        if (!tw) return 1;
        if (!n->a && !(tw & X_NULLABLE)) {
            snprintf(k->whybuf, sizeof k->whybuf, "return: a function of a%s %s hands one back", xan(xty_name(tw, a, sizeof a)), a);
            return xc_fail(k, n, k->whybuf);
        }
        if (!n->a || xt_fits(tw, tv)) return 1;
        snprintf(k->whybuf, sizeof k->whybuf, "return: a function of a%s %s - a%s %s can't go in%s", xan(xty_name(tw, a, sizeof a)), a, xan(xty_name(tv, b, sizeof b)), b, xt_hint(tw, tv));
        return xc_fail(k, n, k->whybuf);
    }
    case XN_SETI: {   // d[k] = v of a dict; l[i] = v of a list the check knows: its item, in a place (marked for the compiler)
        if (!x_check(k, n->a, sc) || !x_check(k, n->b, sc)) return 0;
        int a = x_stype(k, n->a, sc);
        X_WANT(n->c, xt_is_dict(a) ? xt_dict_val(a) : xt_items(a));   // (a row written out, a list's item, a dict's value)
        if (!x_check(k, n->c, sc)) return 0;
        int ti = x_stype(k, n->b, sc), tv = x_stype(k, n->c, sc), it = xt_items(a);
        char tn[60], tm[60];
        if (xt_is_dict(a)) {   // d[k] = v: k a K, v a V (op: numbers)
            int kt = xt_dict_key(a), vt = xt_dict_val(a);
            if (a & X_NULLABLE) snprintf(k->whybuf, sizeof k->whybuf, "[...] = ...: a %s that may be null - check it != null first", xty_name(a, tn, sizeof tn));
            else if (!xt_fits(kt, ti))
                snprintf(k->whybuf, sizeof k->whybuf, "[...] = ...: the dict's keys are %ss - not a%s %s%s", xty_name(kt, tm, sizeof tm), xan(xty_name(ti, tn, sizeof tn)), tn, xt_hint(kt, ti));
            else if (n->op && (!xt_numeric(vt) || !xt_numeric(tv)))
                snprintf(k->whybuf, sizeof k->whybuf, "[...] %c= ...: numbers only - the dict's values are %ss, the value a%s %s", n->op, xty_name(vt, tm, sizeof tm),
                         xan(xty_name(tv, tn, sizeof tn)), tn);
            else {
                if (n->op) tv = vt == XT_INT && tv == XT_INT && n->op != '/' ? XT_INT : XT_FLOAT;
                if (xt_fits(vt, tv)) return 1;
                snprintf(k->whybuf, sizeof k->whybuf, "[...] = ...: the dict's values are %ss - a%s %s can't go in%s", xty_name(vt, tm, sizeof tm),
                         xan(xty_name(tv, tn, sizeof tn)), tn, xt_hint(vt, tv));
            }
            return xc_fail(k, n, k->whybuf);
        }
        if (xt_is_hl(a)) {   // l[i] = v of a list: its item, set in the list (whatever holds it: no place needed)
            if (a & X_NULLABLE) snprintf(k->whybuf, sizeof k->whybuf, "[...] = ...: a %s that may be null - check it != null first", xty_name(a, tn, sizeof tn));
            else if (ti && ti != XT_INT) snprintf(k->whybuf, sizeof k->whybuf, "[...] = ...: a list's item is l[an int] - not a %s", xty_name(ti, tn, sizeof tn));
            else if (n->op && (!xt_numeric(it) || !xt_numeric(tv)))
                snprintf(k->whybuf, sizeof k->whybuf, "[...] %c= ...: numbers only - the item is a%s %s, the value a%s %s", n->op, xan(xty_name(it, tm, sizeof tm)), tm,
                         xan(xty_name(tv, tn, sizeof tn)), tn);
            else {
                if (n->op) tv = it == XT_INT && tv == XT_INT && n->op != '/' ? XT_INT : XT_FLOAT;
                if (xt_fits(it, tv)) { ((struct xnode *)n)->sk = (unsigned char)(it & X_LIST_OF ? 4 : 0); return 1; }   // (4: a row - the value boxed)
                snprintf(k->whybuf, sizeof k->whybuf, "[...] = ...: the list's items are %ss - a%s %s can't go in%s", xty_name(it, tm, sizeof tm),
                         xan(xty_name(tv, tn, sizeof tn)), tn, xt_hint(it, tv));
            }
            return xc_fail(k, n, k->whybuf);
        }
        if (!(a & X_LIST_OF)) return 1;
        if (a & X_NULLABLE) snprintf(k->whybuf, sizeof k->whybuf, "[...] = ...: a %s that may be null - check it != null first", xty_name(a, tn, sizeof tn));
        else if (!x_place(k, n->a, sc)) snprintf(k->whybuf, sizeof k->whybuf, "[...] = ...: set in " X_PLACE_WHY);
        else if (x_loop_copy(k, n, x_via_hl(k, n->a, sc) ? "" : x_root_name(n->a), sc)) return 0;
        else if (ti && ti != XT_INT) snprintf(k->whybuf, sizeof k->whybuf, "[...] = ...: a list's item is l[an int] - not a %s", xty_name(ti, tn, sizeof tn));
        else if (n->op && it && (!xt_numeric(it) || !xt_numeric(tv)))
            snprintf(k->whybuf, sizeof k->whybuf, "[...] %c= ...: numbers only - the item is a%s %s, the value a%s %s", n->op, xan(xty_name(it, tm, sizeof tm)), tm,
                     xan(xty_name(tv, tn, sizeof tn)), tn);
        else {
            if (n->op && it) tv = it == XT_INT && tv == XT_INT && n->op != '/' ? XT_INT : XT_FLOAT;
            if (!(tv & X_NULLABLE) && xt_fits(it, tv)) { ((struct xnode *)n)->sk = (unsigned char)(it & X_LIST_OF ? 5 : 1); return 1; }   // (4: a row - the value boxed)
            snprintf(k->whybuf, sizeof k->whybuf, "[...] = ...: the list's items are %ss - a%s %s can't go in%s", xty_name(it, tm, sizeof tm),
                     xan(xty_name(tv, tn, sizeof tn)), tn, tv & X_NULLABLE ? " (a list holds no null)" : xt_hint(it, tv));
        }
        return xc_fail(k, n, k->whybuf);
    }
    case XN_MEMBER: case XN_MCALL: case XN_INDEX: {   // f(a).x, l[1].name(...): an object's - of a kind known - checked
        if (!x_check(k, n->a, sc) || !x_check(k, n->b, sc)) return 0;
        int hasref = 0;
        for (int i = 0; i < n->n_items; i++) {
            if (n->items[i]->k == XN_REF) { if (!x_check_refarg(k, n->items[i], sc)) return 0; hasref = 1; continue; }
            if (k->script && n->k == XN_MCALL && (n->items[i]->k == XN_LIST || n->items[i]->k == XN_REC)) {   // a list or a { ... } written as a method's argument
                int ta0 = x_stype(k, n->a, sc);
                const struct xnode *m0 = xst_member(ta0, n->s, 'M', NULL);
                g_x_want = xg_want(m0, i + 1);
                if (xt_is_hl(ta0) && (xhm_id(n->s) == XHM_ADD || xhm_id(n->s) == XHM_INDEX_OF || (xhm_id(n->s) == XHM_INSERT && i == 1))) g_x_want = xt_hl_elem(ta0);
            }
            if (!x_check(k, n->items[i], sc)) return 0;
        }
        int a = x_stype(k, n->a, sc);
        char ta[40];
        if (hasref && (g_x_in_ph || !(n->k == XN_MCALL && xst_is(a)))) return xc_fail(k, n, "ref ...: only to a parameter that is a ref - a struct's method's, a declared function's (in a statement)");
        if (a & X_NULLABLE) {   // first(l).name: what may be null - checked first (a variable: let f = first(l); f != null)
            snprintf(k->whybuf, sizeof k->whybuf, "%s%s: a %s may be null (none there) - in a variable, checked != null first; or ... ?? a value",
                     n->k == XN_INDEX ? "[...]" : ".", n->k == XN_INDEX ? "" : n->s, xty_name(a, ta, sizeof ta));
            return xc_fail(k, n, k->whybuf);
        }
        if (!a || a == XT_REC) return 1;   // (not known, a record: worked out, there?)
        if (n->k == XN_INDEX && xt_is_dict(a)) {   // d[k]: k a K
            int ti = x_stype(k, n->b, sc), kt = xt_dict_key(a);
            if (xt_fits(kt, ti)) return 1;
            char tk[40];
            snprintf(k->whybuf, sizeof k->whybuf, "[...]: the dict's keys are %ss - not a%s %s%s", xty_name(kt, tk, sizeof tk), xan(xty_name(ti, ta, sizeof ta)), ta, xt_hint(kt, ti));
            return xc_fail(k, n, k->whybuf);
        }
        if (n->k == XN_INDEX) {
            int ti = x_stype(k, n->b, sc);
            if (!(a & X_LIST_OF) && !xt_is_hl(a)) snprintf(k->whybuf, sizeof k->whybuf, "[...]: a %s is no list", xty_name(a, ta, sizeof ta));
            else if (ti && ti != XT_INT) snprintf(k->whybuf, sizeof k->whybuf, "[...]: a list's item is l[an int] - not a %s%s", xty_name(ti, ta, sizeof ta), ti == XT_FLOAT ? " (int(...), round(...))" : "");
            else return 1;
            return xc_fail(k, n, k->whybuf);
        }
        if ((a & X_LIST_OF) || (xt_is_hl(a) && n->k == XN_MEMBER)) {
            snprintf(k->whybuf, sizeof k->whybuf, ".%s: a %s - one of its items: [1].%s", n->s, xty_name(a, ta, sizeof ta), n->s);
            return xc_fail(k, n, k->whybuf);
        }
        if (a >= XT_INT && n->k == XN_MEMBER && xt_own_member(a, n->s) >= 0) return 1;
        if (n->k == XN_MCALL && xst_is(a)) return x_check_smethod(k, n, a, n->s, x_place(k, n->a, sc), x_via_hl(k, n->a, sc) ? "" : x_root_name(n->a), sc);
        if (n->k == XN_MCALL && xt_is_hl(a)) return x_check_hlmethod(k, n, a, n->s, sc);
        if (n->k == XN_MCALL && xct_of(a) && xct_of(a)->kind == XCT_REC) return x_rec_call(k, n, a, n->s);
        if (a >= XT_INT) {
            snprintf(k->whybuf, sizeof k->whybuf, ".%s: a%s %s has no %s", n->s, xan(xty_name(a, ta, sizeof ta)), ta, n->s);
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
    case XN_REC: return x_check_rec(k, n, sc);
    case XN_CALL: {
        const char *name = n->s;
        int hasref = 0, smeth = 0, hlm = 0;
        for (int i = 0; i < n->n_items; i++) {   // the arguments first - every's function gets its parameter's type: (run) => ...
            int ctx = k->ctx;
            if (n->items[i]->k == XN_REF) { if (!x_check_refarg(k, n->items[i], sc)) return 0; hasref = 1; continue; }   // ref a: a place
            if (k->script && !strcmp(name, "every") && i == n->n_items - 1 && !xsc_find(sc, name, strlen(name), NULL)) k->ctx = XT_INT;
            if (k->script && (n->items[i]->k == XN_LIST || n->items[i]->k == XN_REC)) g_x_want = x_arg_want(k, n, i, sc);   // a list or a { ... } written as an argument: of its parameter's type
            int ok = x_check(k, n->items[i], sc);
            k->ctx = ctx;
            if (!ok) return 0;
        }
        if (k->script && !strchr(name, '.') && xsc_find(sc, name, strlen(name), NULL) == 1) {   // a generic function's call: of its copy (GENERICS)
            const struct xnode *gf = xsc_fn(sc, name, strlen(name));
            if (gf && gf->gen) {
                int r = xg_call(k, (struct xnode *)n, gf, sc);
                if (r != 1) return r == 2;   // (2: in a generic function as written - held against the bounds only)
                name = n->s;
            }
        }
        if (x_ref_far(k, n, name, strcspn(name, "."), sc)) return 0;
        if (hasref && g_x_in_ph) return xc_fail(k, n, "ref ...: in a statement, not in a {placeholder}");
        if (k->script) {   // STRUCTS AND REFS: point(...) makes one; a.method(...) of a variable that is one; a ref to a function known
            const char *d2 = strrchr(name, '.');
            int vf = xsc_find(sc, name, strlen(name), NULL), known;
            if (!d2 && !vf && xst_find(name) >= 0) return hasref ? xc_fail(k, n, "ref ...: a struct's fields take values") : x_check_new(k, n, xst_find(name), sc);
            if (d2 && xsc_find(sc, name, strcspn(name, "."), NULL) == 1) {
                int r0 = x_path_kind(k, n, name, (size_t)(d2 - name), sc, &known);
                if (r0 < 0) return 0;
                smeth = xst_is(r0) ? r0 : 0;
                hlm = xt_is_hl(r0) ? r0 : 0;
            }
            if (smeth) return x_check_smethod(k, n, smeth, d2 + 1, x_place_path(k, n, name, (size_t)(d2 - name), sc), name, sc);
            if (d2 && hlm) {   // l.add(x): a list's method
                if (hlm & X_NULLABLE) { snprintf(k->whybuf, sizeof k->whybuf, "%.*s may be null (no list there) - check it != null first", (int)(d2 - name), name); return xc_fail(k, n, k->whybuf); }
                if (hasref) return xc_fail(k, n, "ref ...: a list's method takes values");
                return x_check_hlmethod(k, n, hlm, d2 + 1, sc);
            }
            if (hasref && !(!d2 && vf == 1 && xsc_fn(sc, name, strlen(name))))
                return xc_fail(k, n, "ref ...: only to a parameter that is a ref - of a function the script declares (name(ref p: type) { ... }) or a struct's method");
        } else if (hasref) return xc_fail(k, n, "ref ...: a script's");
        int nk = n->n_items < 8 ? n->n_items : 8, f = x_fn_id(name, nk);
        struct xsv a0 = nk ? xsv_of(n->items[0]) : (struct xsv){ 0, 0, 0, 0 };
        if (f == 1 && a0.has_n && a0.n > 1) return xc_fail(k, n, "chance takes a probability, 0 to 1: chance(70%) or chance(0.7)");
        if (f) return 1;
        if (!xsc_find(sc, name, strlen(name), NULL))   // a relation's, a method's, a command's: a player that is there
            for (int i = 0; i < n->n_items; i++) {     // (opinion(lord) without a lord: a mistake - a function of a let's
                char tn[40];                           // checks its parameters' types)
                int ta = x_stype(k, n->items[i], sc);
                if (!(ta & X_NULLABLE) || ta == X_NULLABLE) continue;
                snprintf(k->whybuf, sizeof k->whybuf, "%s(...): its argument %d, a %s, may be null - check it != null first", name, i + 1, xty_name(ta, tn, sizeof tn));
                return xc_fail(k, n, k->whybuf);
            }
        if (!strchr(name, '.')) {
            int v = xsc_find(sc, name, strlen(name), NULL);
            if (v == 2) { snprintf(k->whybuf, sizeof k->whybuf, "%s is called before its let", name); return xc_fail(k, n, k->whybuf); }
            const struct xnode *fd = v ? xsc_fn(sc, name, strlen(name)) : NULL;
            if (fd && k->script && !x_check_args(k, n, fd, 0, name, sc)) return 0;   // a function a let made: its parameters' count and types
            if (v && !fd && k->script) {   // a variable of a function's type (f: func<int, int>): its parameters' count and types; of another type: no function
                int kv = xsc_kind(sc, name, strlen(name));
                const struct xctype *c = kv & X_LIST_OF ? NULL : xct_of(kv);
                char a[60], b[40];
                if (c && c->kind == XCT_FN) {
                    if ((kv & X_NULLABLE) && !xn_narrowed(k, name, strlen(name))) {
                        snprintf(k->whybuf, sizeof k->whybuf, "%s may be null (no function there) - check %s != null first", name, name);
                        return xc_fail(k, n, k->whybuf);
                    }
                    if (c->n != n->n_items) {
                        snprintf(k->whybuf, sizeof k->whybuf, "%s takes %d argument%s (a %s)", name, c->n, c->n == 1 ? "" : "s", xty_name(kv & ~X_NULLABLE, a, sizeof a));
                        return xc_fail(k, n, k->whybuf);
                    }
                    for (int i = 0; i < n->n_items; i++) {
                        int ta = x_stype(k, n->items[i], sc);
                        if (xt_fits(c->t[i], ta)) continue;
                        snprintf(k->whybuf, sizeof k->whybuf, "%s(...): its argument %d is a%s %s - a%s %s can't go in%s", name, i + 1, xan(xty_name(c->t[i], a, sizeof a)), a,
                                 xan(xty_name(ta, b, sizeof b)), b, xt_hint(c->t[i], ta));
                        return xc_fail(k, n, k->whybuf);
                    }
                } else if (kv && X_TBASE(kv) != XT_FN) {
                    snprintf(k->whybuf, sizeof k->whybuf, "%s(...): %s is a%s %s - no function", name, name, xan(xty_name(kv, a, sizeof a)), a);
                    return xc_fail(k, n, k->whybuf);
                }
            }
            if (v) return 1;   // a function in a variable
            int sf = k->script ? x_script_fn(name) : -1;
            if (sf >= 0) {
                if (sf != nk) { snprintf(k->whybuf, sizeof k->whybuf, "%s takes %d argument%s", name, sf, sf == 1 ? "" : "s"); return xc_fail(k, n, k->whybuf); }
                return 1;
            }
            if (k->script && g_x_host.command(name)) return 1;
            int want = x_fn_known(name);
            if (want >= 0 && !k->script && !k->body && x_fn_blk(name)) {   // statements change state: never from a line
                snprintf(k->whybuf, sizeof k->whybuf, "%s(...) has statements - it runs from an on_... handler or another such function, not from a line or a name", name);
                return xc_fail(k, n, k->whybuf);
            }
            if (want >= 0) {
                if (want != nk) { snprintf(k->whybuf, sizeof k->whybuf, "%s takes %d argument%s", name, want, want == 1 ? "" : "s"); return xc_fail(k, n, k->whybuf); }
                return 1;
            }
        }
        const char *dot = strrchr(name, '.');
        if (k->script && !strcmp(name, "game.setup"))   // (one at the script's top is checked there, not here)
            return xc_fail(k, n, "game.setup(() => { ... }): a statement at the script's top (its function runs before the game starts)");
        if ((k->script || k->texts) && dot) {   // p.opinion(q), game.player(2): a method of its kind (a variable of no kind known: when it runs)
            int known, r = x_path_kind(k, n, name, (size_t)(dot - name), sc, &known);
            if (r < 0) return 0;
            if (known && (r & X_NULLABLE)) {   // lord.opinion(them): checked first
                snprintf(k->whybuf, sizeof k->whybuf, "%.*s may be null (none there) - check %.*s != null first", (int)(dot - name), name, (int)(dot - name), name);
                return xc_fail(k, n, k->whybuf);
            }
            if (known && k->script) {
                if (r == X_ANY) return 1;
                if (r & X_LIST_OF) {
                    char tl[60];
                    snprintf(k->whybuf, sizeof k->whybuf, "%s(...): a %s - one of its items: [1].%s(...) (a seq has no methods; a list<T> add, insert ...)", name, xty_name(r, tl, sizeof tl), dot + 1);
                    return xc_fail(k, n, k->whybuf);
                }
                if (xt_is_dict(r)) {
                    char tl[60];
                    snprintf(k->whybuf, sizeof k->whybuf, "%s(...): a %s has no methods - d[k] is a key's entry, d[k] = v sets it", name, xty_name(r, tl, sizeof tl));
                    return xc_fail(k, n, k->whybuf);
                }
                if (r == XT_REC || (xct_of(r) && xct_of(r)->kind == XCT_REC)) return x_rec_call(k, n, r, dot + 1);
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
    case XN_LIST: {
        // A LIST'S ITEMS (a script's): of one type T. An item is a T (one item), a list of T (its items, spliced in) or a T
        // or null (left out when null) - nothing else. The type wanted where the list is written says what T is: for a
        // list<list<int>> a list<int> is one item (a row, marked for the compiler: kept whole) and a list<list<int>> is
        // spliced. No type wanted: the items' own, lists spliced (flat, as ever) - a list of lists writes its type.
        int want = g_x_want, W, E, T = 0, hw;
        char a[60], b[60];
        g_x_want = 0;
        hw = k->script && xt_is_hl(want) && xt_hl_elem(want);   // [...] where a list goes: made the list
        W = hw || (k->script && (want & X_LIST_OF) && xt_elem(want)) ? want & ~X_NULLABLE : 0;
        E = hw ? xt_hl_elem(W) : W ? xt_elem(W) : 0;
        if (hw) ((struct xnode *)n)->sk |= 32;
        if (hw && g_x_in_ph) return xc_fail(k, n, "a list made in a {placeholder} - let l: list<T> = [...] first, then {l}");
        for (int i = 0; i < n->n_items; i++) {
            if (k->script && ((E & X_LIST_OF) || xt_is_hl(E) || n->items[i]->k == XN_REC)) X_WANT(n->items[i], E);   // a list written as an item of rows: a row; of lists: a list; a { ... } an item
            int ok = x_check(k, n->items[i], sc);
            g_x_want = 0;
            if (!ok) return 0;
        }
        if (!k->script) return 1;
        for (int i = 0; i < n->n_items; i++) {
            const struct xnode *it = n->items[i];
            int ti = x_stype(k, it, sc), e;
            if (hw && (E & X_NULLABLE) && xt_fits(E, ti)) { ((struct xnode *)it)->sk |= 8; continue; }   // one or null, kept as either
            if (E & X_LIST_OF && (ti & X_NULLABLE)) {   // a row that may be null: as an item it would be an empty row - none and empty are one at run time
                snprintf(k->whybuf, sizeof k->whybuf, "[...]: its item %d may be null - a list of lists holds rows, never none (check it != null first, or ... ?? [])", i + 1);
                return xc_fail(k, it, k->whybuf);
            }
            if (!ti && ((E & X_LIST_OF) || xt_is_hl(E))) {   // a list of lists: whether it is a row or rows to splice is the item's type - it must be known
                snprintf(k->whybuf, sizeof k->whybuf, "[...]: its item %d's type can't be told - a list of lists needs it (a row, or rows to splice): in a variable with its type first", i + 1);
                return xc_fail(k, it, k->whybuf);
            }
            if (!ti || ti == X_NULLABLE) continue;   // (not known: checked when it is worked out; null: left out)
            if (E) {
                if (xt_fits(E | X_NULLABLE, ti)) { if (E & X_LIST_OF) ((struct xnode *)it)->sk |= 4; continue; }   // one item
                if ((ti & X_LIST_OF) && !(ti & X_NULLABLE) && (!xt_elem(ti) || xt_fits(E, xt_elem(ti)))) continue;   // its items
                if (xt_is_hl(ti) && !(ti & X_NULLABLE) && xt_fits(E, xt_hl_elem(ti))) { ((struct xnode *)it)->sk |= 16; continue; }   // a list's items
                snprintf(k->whybuf, sizeof k->whybuf, "[...]: its item %d is a%s %s - the list's are %ss: one of them, a list of them, or one or null", i + 1,
                         xan(xty_name(ti, a, sizeof a)), a, xty_name(E, b, sizeof b));
                return xc_fail(k, it, k->whybuf);
            }
            e = ti & ~X_NULLABLE;
            if (e & X_LIST_OF) e = xt_elem(e);
            if (!e) continue;
            if (!T || xt_fits(e, T)) T = e;   // (the first; an int before a float: floats)
            else if (!xt_fits(T, e)) {
                snprintf(k->whybuf, sizeof k->whybuf, "[...]: its item %d is a%s %s, the ones before %ss - a seq's items are of one type (a seq of seqs: its type written, let l: seq<seq<int>> = ...)",
                         i + 1, xan(xty_name(ti, a, sizeof a)), a, xty_name(T, b, sizeof b));
                return xc_fail(k, it, k->whybuf);
            }
        }
        ((struct xnode *)n)->n = W ? W : xt_list_of(T);
        return 1;
    }
    case XN_ADD: case XN_MUL: case XN_NEG:   // [1, 2] + 10, "a" + b: as written - a mistake before it is worked out
        if ((n->a && n->a->k == XN_LIST) || (n->b && n->b->k == XN_LIST))
            return xc_fail(k, n, "a list where one number belongs - pick(l) for one of its items, \"{l}\" in a text");
        if (n->k == XN_ADD && ((n->a && n->a->k == XN_STR) || (n->b && n->b->k == XN_STR)))
            return xc_fail(k, n, "+ adds numbers - a text is built in its string: \"Age: {epoch}\", \"{a}{b}\"");
        if (!x_check(k, n->a, sc) || !x_check(k, n->b, sc)) return 0;
        for (int i = 0; i < 2; i++) {   // each side a number (or a vector): an epoch, a text, a player is none
            int t = x_stype(k, i ? n->b : n->a, sc);
            char tn[40];
            if (i && !n->b) continue;
            if (t & X_NULLABLE) {   // p.war_minutes + 1: a number that may be none
                snprintf(k->whybuf, sizeof k->whybuf, "%c: a%s %s may be null - check it != null first, or (... ?? 0)", n->k == XN_NEG ? '-' : n->op ? n->op : '+',
                         xan(xty_name(t, tn, sizeof tn)), tn);
                return xc_fail(k, n, k->whybuf);
            }
            if (!t || xt_numeric(t) || (t >= XT_VEC2 && t <= XT_VEC4)) continue;
            snprintf(k->whybuf, sizeof k->whybuf, "%c: a%s %s is no number%s", n->k == XN_NEG ? '-' : n->op ? n->op : '+',
                     xan(xty_name(t, tn, sizeof tn)), tn,
                     t == XT_EPOCH || t == XT_COLOR ? " - int(...) for its number" : t == XT_TEXT ? " (a text is built in its string: \"{a}{b}\")" : "");
            return xc_fail(k, n, k->whybuf);
        }
        return 1;
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
        if (xn_null(n->a) || xn_null(n->b)) {   // x == null: x may be null (a T?, or not known)
            int tx = x_stype(k, xn_null(n->a) ? n->b : n->a, sc);
            char tn[40];
            if (n->op != '=' && n->op != '!') return xc_fail(k, n, "null has no order - x == null or x != null");
            if (!tx || (tx & X_NULLABLE)) return 1;
            snprintf(k->whybuf, sizeof k->whybuf, "a%s %s is never null - compared with null", xan(xty_name(tx, tn, sizeof tn)), tn);
            return xc_fail(k, n, k->whybuf);
        }
        int ta = x_stype(k, n->a, sc) & ~X_NULLABLE, tb = x_stype(k, n->b, sc) & ~X_NULLABLE;   // both known: of one type
        int order = n->op != '=' && n->op != '!';
        if (k->script && (ta == XT_REC || tb == XT_REC || (xct_of(ta) && xct_of(ta)->kind == XCT_REC && !(ta & X_LIST_OF)) || (xct_of(tb) && xct_of(tb)->kind == XCT_REC && !(tb & X_LIST_OF))))
            return xc_fail(k, n, "a record compared - compare its members (r.name == ...); a struct is == another of the same fields");
        if (!ta || !tb || (xt_numeric(ta) && xt_numeric(tb)) || (ta == tb && !(order && (ta == XT_BOOL || ta >= XT_FN || ta < XT_INT || (ta & X_LIST_OF)))))
            return 1;
        char tn[40], tm[40];
        snprintf(k->whybuf, sizeof k->whybuf, "a%s %s %s a%s %s%s", xan(xty_name(ta, tn, sizeof tn)), tn,
                 ta == tb ? "has no order - compared with" : "compared with", xan(xty_name(tb, tm, sizeof tm)), tm,
                 ta == XT_EPOCH || tb == XT_EPOCH ? " - epoch.middle, or int(epoch) for its number" :
                 ta == XT_COLOR || tb == XT_COLOR ? " - color.red, or int(color) for its number" : "");
        return xc_fail(k, n, k->whybuf);
    }
    default:
        return x_check(k, n->c, sc) && x_check(k, n->a, sc) && x_check(k, n->b, sc);
    }
}

// ===== WORKING IT OUT =====
// THE VIEW (SCRIPTS COMPILED): a function's variables are slots, read and set by their numbers - but a name
// is still looked up by its letters where the language does that (a text's {placeholder}, p.gold of a variable p, the
// host's p.opinion(q)): each running function's names and slots, then those of the code around where it was made
// keep: one that outlives its function (a script's top, a trigger's copy) - taken as it is, not copied
struct xview { const char *const *name; struct xval *val; int n; struct xview *up; int keep; };
// A REF AT WORK (STRUCTS AND REFS): the view its variable is in, the variable's slot there and the way on from its
// value, a step each - a field's number (>= 0), or an item's (-1 - its place from 0). Numbers only: saved as they are
// hl: a list's item is its place - item slot of the list (w NULL), the way on from there
struct xref { struct xview *w; struct xhlist *hl; int slot, np; int path[]; };
// the value at a ref's place as it is now, NULL: none (*why) - no place in it, or the place is gone since the ref
// was made (a list set anew with fewer items)
internal const struct xval *xref_peek(const struct xval *rv, const char **why) {
    const struct xref *r = rv->kind == XV_REF ? rv->ref : NULL;
    const char *none;
    if (!why) why = &none;
    if (r && r->hl && (r->slot < 0 || r->slot >= r->hl->n)) { *why = "a ref's place is gone: its list has fewer items now"; return NULL; }
    if (!r || (!r->hl && (!r->w || r->slot < 0 || r->slot >= r->w->n))) { *why = "a ref without its place (a ref parameter handed a value?)"; return NULL; }
    const struct xval *v = r->hl ? &r->hl->it[r->slot] : &r->w->val[r->slot];
    if (v->kind == XV_UNSET || v->kind == XV_REF) { *why = "a ref to a variable before its let"; return NULL; }
    for (int i = 0; i < r->np; i++) {
        int p = r->path[i];
        if (p >= 0 ? !(v->kind == XV_REC && v->w && v->rec->ty && p < v->rec->n) : -1 - p >= xli_n(v)) {
            *why = p >= 0 ? "a ref's place is gone: what held its field is no such struct any more" : "a ref's place is gone: its list has fewer items now";
            return NULL;
        }
        v = p >= 0 ? &v->rec->val[p] : xli_at(v, -1 - p);
    }
    return v;
}
// a name's slot, NULL none (or not set yet) - a ref's: the value at its place
internal const struct xval *x_view_find(const struct xctx *c, const char *name, size_t len) {
    for (const struct xview *w = c->view; w; w = w->up)
        for (int i = 0; i < w->n; i++)
            if (w->val[i].kind != XV_UNSET && !strncmp(w->name[i], name, len) && !w->name[i][len])
                return w->val[i].kind == XV_REF ? xref_peek(&w->val[i], NULL) : &w->val[i];
    return NULL;
}
// a function value: its definition and where it was made (SCRIPTS COMPILED) - function fi of code, on the machine vm;
// venv: the view of the function it was made in (NULL: one without names - a text's top)
struct xvm;
struct xe;
struct xfn { struct xcode *code; int fi; struct xview *venv; struct xvm *vm; };
internal void xvm_call(struct xe *e, const struct xval *fv, const struct xval *a, int na, struct xval *v);
internal int xfn_nparams(const struct xfn *fn);
// err: what is being worked out failed (its mistake told, or none: not an expression); why: what it was; kept: the
// name just looked up is a constant that stays (xop_name); steps: loop rounds since it last waited
struct xe { struct xctx *c; int err; const char *why; int kept; int steps; };
// a loop that never waits holds the game (a script runs on its thread): this many rounds without a wait - of all
// loops in it together, counted, not timed (every PC stops at the same one) - and it is a mistake
#define X_LOOP_STEPS 1000000
internal int xvm_eval(struct xcode *k, struct xe *e, struct xval *v, const struct xview *up);
internal struct xcode *x_unit_code(const struct xunit *u);
internal void x_use(struct xe *e, const struct xval *v) {
    if (!v->miss) return;
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
    if (v->kind == XV_HLIST && v->w) { x_fail(e, "a list where one number belongs - l[i], len(l)"); return 0; }
    if (!v->w || v->has_n) return 1;
    x_fail(e, "a text (\"%.40s\") where a number belongs - number(x) makes a number of a text", xtext_of(v));
    return 0;
}
// the value at a ref's place (STRUCTS AND REFS) - an item as l[i] gives it; 0: none (told)
internal int xref_get(struct xe *e, const struct xval *rv, struct xval *out) {
    const char *why;
    const struct xval *v = xref_peek(rv, &why);
    if (!v) { xnothing(out); x_fail(e, "%s", why); return 0; }
    int item = rv->ref->np && rv->ref->path[rv->ref->np - 1] < 0;
    if (rv->ref->hl) xvm_copy(out, v);   // (in a list: copied out - the list may change it)
    else *out = *v;
    if (item) { out->w = 1; out->ln = 0; out->it = NULL; }
    xb_unbox(out);   // (a row: the list it is)
    return 1;
}
internal int xref_step(struct xe *e, struct xval *rv, int step) {   // the ref in rv a step further: a field's number, an item's (-1 - its place)
    const struct xref *r = rv->ref;
    struct xref *q = x_alloc(sizeof *q + (size_t)(r->np + 1) * sizeof(int));
    if (!q) { e->err = 1; return 0; }
    q->w = r->w; q->hl = r->hl; q->slot = r->slot; q->np = r->np + 1;
    memcpy(q->path, r->path, (size_t)r->np * sizeof(int));
    q->path[r->np] = step;
    rv->ref = q;
    return 1;
}
// cur with the value at the end of path (np steps) replaced by nv, in out: each struct and list on the way made anew
// with its one member changed - the blocks they were are never written (every copy made of them before keeps its
// value: a struct is a value). 0: the way is gone (told)
internal int xref_build(struct xe *e, const struct xval *cur, const int *path, int np, const struct xval *nv, struct xval *out) {
    struct xval child;
    if (!np) { *out = *nv; return 1; }
    int p = path[0];
    if (p >= 0) {
        if (!(cur->kind == XV_REC && cur->w && cur->rec->ty && p < cur->rec->n)) { x_fail(e, "a place is gone: what held its field is no such struct any more"); return 0; }
        if (!xref_build(e, &cur->rec->val[p], path + 1, np - 1, nv, &child)) return 0;
        struct xrec *rec = x_alloc(sizeof *rec);
        struct xval *val = x_alloc((size_t)cur->rec->n * sizeof *val + 1);
        if (!rec || !val) { e->err = 1; return 0; }
        memcpy(val, cur->rec->val, (size_t)cur->rec->n * sizeof *val);
        val[p] = child;
        rec->n = cur->rec->n; rec->key = cur->rec->key; rec->val = val; rec->ty = cur->rec->ty;
        *out = *cur;
        out->rec = rec;
        return 1;
    }
    int ix = -1 - p, n = xli_n(cur), row = xb_is(cur);   // (row: the way goes through a list in a list)
    if (ix >= n) { x_fail(e, "a place is gone: its list has fewer items now"); return 0; }
    if (!xref_build(e, xli_at(cur, ix), path + 1, np - 1, nv, &child)) return 0;
    // a row's place set whole (ref r = rows[2], then r = [1, 2]): the list boxed again - the check let only a row's
    // type through there
    if (xb_is(xli_at(cur, ix)) && np == 1 && !xb_is(&child)) xb_box(&child);
    if (!child.w || child.ln) { x_fail(e, "[...] = ...: %s can't be a list's item", child.ln ? "a list" : "nothing"); return 0; }
    if (!cur->ln && !row) { *out = child; return 1; }   // (one value, a list of one: the item is the value)
    struct xval *it = x_alloc((size_t)n * sizeof *it);
    if (!it) { e->err = 1; return 0; }
    memcpy(it, row ? cur->rec->val : cur->it, (size_t)n * sizeof *it);
    it[ix] = child;
    *out = *cur;
    if (row) {   // the row made anew, a row still
        struct xrec *rec = x_alloc(sizeof *rec);
        if (!rec) { e->err = 1; return 0; }
        rec->n = n; rec->key = NULL; rec->val = it; rec->ty = &g_x_boxty;
        out->rec = rec;
    } else out->it = it;
    return 1;
}
// A LIST'S ITEMS (LISTS above): a value made the list's own - its texts, numbers, items and members malloc'd, a
// function's definition too (only one of the script's top or a kept one: a function made inside a function goes
// with its frame); 0: not one it can hold (told)
internal const char k_xo_empty[1] = "";
internal int xo_ok(struct xe *e, const struct xval *v) {
    if (v->kind == XV_REF) { x_fail(e, "a ref can't be kept in a list or a dict"); return 0; }
    if (v->kind == XV_FN && v->fn && v->fn->venv && !v->fn->venv->keep) { x_fail(e, "a function made inside a function can't be kept in a list or a dict - one of the script's top can"); return 0; }
    for (int i = 0; i < v->ln; i++) if (!xo_ok(e, &v->it[i])) return 0;
    for (int i = 0; v->kind == XV_REC && v->rec && i < v->rec->n; i++) if (!xo_ok(e, &v->rec->val[i])) return 0;
    return 1;
}
internal void xo_copy(struct xval *dst, const struct xval *src) {
    *dst = *src;
    dst->s = src->s && src->s[0] ? _strdup(src->s) : k_xo_empty;
    if (src->kind == XV_VEC && src->vc) {
        double *m = malloc((size_t)src->vn * sizeof *m);
        if (m) memcpy(m, src->vc, (size_t)src->vn * sizeof *m);
        dst->vc = m;
    }
    if (src->kind == XV_REC && src->rec) {
        struct xrec *r = malloc(sizeof *r);
        struct xval *val = malloc((size_t)src->rec->n * sizeof *val + 1);
        const char **key = src->rec->ty ? NULL : malloc((size_t)src->rec->n * sizeof *key + 1);
        if (r) { r->n = src->rec->n; r->ty = src->rec->ty; r->key = src->rec->ty ? src->rec->key : key; r->val = val; }
        for (int i = 0; r && val && i < r->n; i++) {
            xo_copy(&val[i], &src->rec->val[i]);
            if (key) key[i] = _strdup(src->rec->key[i]);
        }
        dst->rec = r;
    }
    if (src->kind == XV_FN && src->fn) {
        struct xfn *fn = malloc(sizeof *fn);
        if (fn) *fn = *src->fn;
        dst->fn = fn;
    }
    if (src->ln) {
        struct xval *it = malloc((size_t)src->ln * sizeof *it);
        for (int i = 0; it && i < src->ln; i++) xo_copy(&it[i], &src->it[i]);
        dst->it = it;
    }
}
internal void xo_free(struct xval *v) {
    if (v->s && v->s != k_xo_empty) free((char *)v->s);
    if (v->kind == XV_VEC) free((double *)v->vc);
    if (v->kind == XV_REC && v->rec) {
        struct xrec *r = (struct xrec *)v->rec;
        for (int i = 0; r->val && i < r->n; i++) { xo_free(&r->val[i]); if (!r->ty && r->key) free((char *)r->key[i]); }
        free(r->val);
        if (!r->ty) free((void *)r->key);
        free(r);
    }
    if (v->kind == XV_FN) free((void *)v->fn);
    for (int i = 0; v->it && i < v->ln; i++) xo_free((struct xval *)&v->it[i]);
    if (v->ln) free((void *)v->it);
}
// a value of a list's or a dict's own, read: copied into the region now (the table may change it meanwhile)
internal void xo_out(const struct xval *it, struct xval *v) {
    if (!it->s[0] && !it->ln && (it->kind == XV_PLAIN || it->kind == XV_HLIST || it->kind == XV_DICT || it->kind == XV_TRIG)) *v = *it;
    else xvm_copy(v, it);
}
// item i of a list, read: copied out, a row the list it is
internal void xhl_out(const struct xhlist *h, int i, struct xval *v) {
    xo_out(&h->it[i], v);
    xb_unbox(v);
}
internal struct xhlist *xhl_new(void) {
    struct xhlist *h = calloc(1, sizeof *h);
    if (!h) return NULL;
    h->next = g_xhl; g_xhl = h;
    g_xhl_n++; g_xhl_made++;
    return h;
}
internal void xhl_value(struct xval *v, struct xhlist *h) { xnothing(v); if (h) { v->w = 1; v->kind = XV_HLIST; v->hl = h; } }
internal int xhl_room(struct xe *e, struct xhlist *h, int n) {
    if (n <= h->cap) return 1;
    int cap = h->cap ? h->cap * 2 : 8;
    while (cap < n) cap *= 2;
    struct xval *m = cap <= (1 << 24) ? realloc(h->it, (size_t)cap * sizeof *m) : NULL;
    if (!m) { x_fail(e, "a list of more than %d items - no memory", h->cap); return 0; }
    h->it = m; h->cap = cap;
    return 1;
}
// v into a list: at place i (from 0) - a new item there when ins, else the one there replaced (its own freed)
internal int xhl_put(struct xe *e, struct xhlist *h, int i, const struct xval *v, int ins) {
    if (!xo_ok(e, v)) return 0;
    if (ins) {
        if (!xhl_room(e, h, h->n + 1)) return 0;
        memmove(&h->it[i + 1], &h->it[i], (size_t)(h->n - i) * sizeof *h->it);
        h->n++;
    } else {   // copied before the old item is freed: v may share its texts, vectors and records (l[i].x = 1: the item
        // with one field set, a shallow copy of the old one - xref_build)
        struct xval t;
        xo_copy(&t, v);
        xo_free(&h->it[i]);
        h->it[i] = t;
        return 1;
    }
    xo_copy(&h->it[i], v);
    return 1;
}
internal void xhl_free(struct xhlist *h) {
    for (int i = 0; i < h->n; i++) xo_free(&h->it[i]);
    free(h->it);
    free(h);
}
internal void xhl_free_all(void) {   // (a new game)
    while (g_xhl) { struct xhlist *h = g_xhl; g_xhl = h->next; xhl_free(h); }
    g_xhl_n = g_xhl_made = 0;
}
// a new list of a seq's items, or a list's (a copy), or none (nothing: an empty one)
internal void xhl_make(struct xe *e, const struct xval *src, struct xval *v) {
    struct xhlist *h = xhl_new();
    if (!h) { e->err = 1; return; }
    const struct xhlist *from = src->kind == XV_HLIST && src->w ? src->hl : NULL;
    int n = from ? from->n : xl_n(src);
    if (n && !xhl_room(e, h, n)) return;
    for (int i = 0; i < n && !e->err; i++) {
        const struct xval *it = from ? &from->it[i] : xl_at(src, i);
        if (!from && !xo_ok(e, it)) break;
        xo_copy(&h->it[h->n++], it);
    }
    xhl_value(v, h);
}
// THE SWEEP: every list that a value of a machine (its stack: its frames' slots and what it works out), a trigger,
// an after, a kept view leads to is marked; the rest freed. Only where nothing else holds a value: between turns,
// at a loop's next round in a script's own turn (xvm_gc)
internal int g_xhl_gen;
internal void xhl_mark_view(const struct xview *w);
internal void xhl_mark(const struct xval *v) {
    if (v->kind == XV_HLIST && v->hl && v->hl->mark != g_xhl_gen) {
        v->hl->mark = g_xhl_gen;
        for (int i = 0; i < v->hl->n; i++) xhl_mark(&v->hl->it[i]);
    }
    if (v->kind == XV_FN && v->fn) xhl_mark_view(v->fn->venv);
    if (v->kind == XV_REF && v->ref) { xhl_mark_view(v->ref->w); if (v->ref->hl) { struct xval t; xhl_value(&t, v->ref->hl); xhl_mark(&t); } }
    for (int i = 0; v->it && i < v->ln; i++) xhl_mark(&v->it[i]);
    for (int i = 0; v->kind == XV_REC && v->rec && i < v->rec->n; i++) xhl_mark(&v->rec->val[i]);
    if (v->kind == XV_DICT && v->dict && v->dict->mark != g_xhl_gen) {   // a dict: its values that aren't plain
        struct xdict *d = v->dict;
        d->mark = g_xhl_gen;
        if (d->def.xv) xhl_mark(d->def.xv);
        for (int i = 0; i < d->n; i++) if (d->e[i].xv) xhl_mark(d->e[i].xv);
    }
}
// (a view's values: a frame's are on its machine's stack too - each view once a sweep)
#define XHL_VIEWS 4096
internal const struct xview *g_xhl_seen[XHL_VIEWS];
internal int g_xhl_nseen;
internal void xhl_mark_view(const struct xview *w) {
    for (; w; w = w->up) {
        for (int i = 0; i < g_xhl_nseen; i++) if (g_xhl_seen[i] == w) return;
        if (g_xhl_nseen < XHL_VIEWS) g_xhl_seen[g_xhl_nseen++] = w;
        for (int i = 0; i < w->n; i++) xhl_mark(&w->val[i]);
    }
}
internal void xhl_sweep(void);   // (below the machine: it reads them)
internal int x_same(const struct xval *a, const struct xval *b);
internal int xcmp_as(const struct xval *a, const struct xval *b, int eq);
internal void xhl_method(struct xe *e, int id, struct xval *l, struct xval *a, int na, struct xval *v) {
    static const char *const k[] = { "", "add", "insert", "remove_at", "clear", "index_of" };
    struct xhlist *h = l->kind == XV_HLIST && l->w ? l->hl : NULL;
    xnothing(v);
    if (!h) { x_fail(e, ".%s(...): %s - no list", k[id], l->w ? xtext_of(l) : "null"); return; }
    for (int i = 0; i < na; i++) x_use(e, &a[i]);
    if (e->err) return;
    int i = id == XHM_INSERT || id == XHM_REMOVE_AT ? (int)a[0].n : 0;
    if ((id == XHM_INSERT || id == XHM_REMOVE_AT) && (!x_numeric_ok(e, &a[0]) || (double)i != a[0].n || i < 1 || i > h->n + (id == XHM_INSERT))) {
        if (!e->err) x_fail(e, ".%s(%s%s): no such place - the list has %d (the first is 1%s)", k[id], xtext_of(&a[0]), id == XHM_INSERT ? ", ..." : "", h->n, id == XHM_INSERT ? ", one past the last: at its end" : "");
        return;
    }
    switch (id) {
    case XHM_ADD: xhl_put(e, h, h->n, &a[0], 1); break;
    case XHM_INSERT: xhl_put(e, h, i - 1, &a[1], 1); break;
    case XHM_REMOVE_AT:
        xo_free(&h->it[i - 1]);
        memmove(&h->it[i - 1], &h->it[i], (size_t)(h->n - i) * sizeof *h->it);
        h->n--;
        break;
    case XHM_CLEAR: for (int j = 0; j < h->n; j++) xo_free(&h->it[j]); h->n = 0; break;
    case XHM_INDEX_OF:   // the first item the same as x (as ==, a row item for item): its number, none: nothing
        for (int j = 0; j < h->n; j++) {
            struct xval it = h->it[j];
            xb_unbox(&it);
            if (it.ln || a[0].ln ? x_same(&it, &a[0]) : !xcmp_as(&it, &a[0], 1)) { xnum(v, j + 1); break; }
        }
        break;
    default: x_fail(e, "the script's instructions are broken (a list's method)"); break;
    }
}
// a struct's field by name: its number, -1 none
internal int xst_field(const struct xstype *ty, const char *name) {
    for (int i = 0; i < ty->nf; i++) if (!strcmp(ty->fname[i], name)) return i;
    return -1;
}
// two values the same through and through: structs of one type field by field, lists item by item, the rest as ==
internal int xcmp_as(const struct xval *a, const struct xval *b, int eq);
internal int x_same(const struct xval *a, const struct xval *b) {
    if (a->kind == XV_HLIST || b->kind == XV_HLIST) return a->kind == b->kind && a->hl == b->hl;   // a list: the same one
    if (xb_is(a) || xb_is(b)) {   // a row: as the list it is
        struct xval ua = *a, ub = *b;
        xb_unbox(&ua); xb_unbox(&ub);
        return x_same(&ua, &ub);
    }
    if (a->ln || b->ln) {
        if (xl_n(a) != xl_n(b)) return 0;
        for (int i = 0; i < xl_n(a); i++) if (!x_same(xl_at(a, i), xl_at(b, i))) return 0;
        return 1;
    }
    if (!a->w || !b->w) return !a->w && !b->w;
    if (a->kind == XV_REC && b->kind == XV_REC) {
        if (!a->rec->ty || a->rec->ty != b->rec->ty || a->rec->n != b->rec->n) return 0;
        for (int i = 0; i < a->rec->n; i++) if (!x_same(&a->rec->val[i], &b->rec->val[i])) return 0;
        return 1;
    }
    if (a->kind != b->kind) return 0;
    if (a->kind == XV_FN) return a->fn == b->fn;
    if (a->kind == XV_DICT) return a->dict == b->dict;
    return !xcmp_as(a, b, 1);
}
// a text against a number ("12" == 12, "9" < 10): no comparison - neither turns into the other
internal int xmismatch(const struct xval *a, const struct xval *b) { return (!a->has_n && !b->has_s) || (!a->has_s && !b->has_n); }
internal int xvec_eq(const struct xval *a, const struct xval *b) {   // two vectors, the same numbers
    if (a->vn != b->vn) return 0;
    for (int i = 0; i < a->vn; i++) if (a->vc[i] != b->vc[i]) return 0;
    return 1;
}
// <0, 0, >0: players the same one or not, numbers, else texts; eq: only whether they are the same is asked (0 or not)
internal int xcmp_as(const struct xval *a, const struct xval *b, int eq) {
    double x, y;
    if (a->kind == XV_HLIST || b->kind == XV_HLIST) return a->kind == b->kind && a->hl == b->hl ? 0 : 2;   // a list: the same one
    if (a->obj && b->obj) {
        if (a->okind != b->okind) return a->okind - b->okind;
        int ka = g_x_host.obj_key(a), kb = g_x_host.obj_key(b);
        return ka < kb ? -1 : ka > kb;
    }
    if (a->kind == XV_VEC || b->kind == XV_VEC) return a->kind == b->kind && xvec_eq(a, b) ? 0 : 2;   // equal or not
    if (a->kind == XV_REC || b->kind == XV_REC)   // (contains: a record is equal to nothing - a struct to one with the same fields)
        return a->kind == b->kind && a->rec->ty && a->rec->ty == b->rec->ty && x_same(a, b) ? 0 : 2;
    if (xmismatch(a, b)) return 2;   // unequal
    if (xis_num(a, &x) && xis_num(b, &y)) return x < y ? -1 : x > y;
    return eq ? !x_ieq(xtext_of(a), xtext_of(b)) : lstrcmpiA(xtext_of(a), xtext_of(b));
}
// DICTS: dict<K, V>(default) - an entry per key, the default for every key without one (so d[k] is always a value, none
// guessed). K: int, text or an object's kind (player ...: by its index, as == tells them apart); V: any type - an
// int, a float, a bool or a text kept as it is (vn, vs: the personalities' dict fields read and write those), any
// other as a value of the dict's own (xv: copied in, copied out when read, as a list's item - so d[k].x = 5 is no
// place: the check says so). A dict is a table of its own, outside the values' regions: the values holding it share
// it (d[k] = v is seen through every one); one nobody holds any more is freed by the lists' sweep (xhl_sweep), all of
// them with the game (x_triggers_reset). Its types are the check's (dict<K, V>: XCT_DICT); a key's and a plain
// value's are checked again when it is worked out, another value's can't be told then.
// (struct xdent, struct xdict: beside the lists' table, LISTS - the sweep goes through both)
internal int g_xdict_keep;   // the dicts made now are their maker's to free (a personality's fields: x_dict_free), not the list's
internal int xdict_plain(const struct xdict *d) { return xt_plain_val(d->vt); }
internal void xdict_vfree(struct xdent *x) {   // an entry's value
    free(x->vs);
    x->vs = NULL;
    if (x->xv) { xo_free(x->xv); free(x->xv); x->xv = NULL; }
}
internal struct xval *xdict_own(const struct xval *v) {   // a value made the dict's; NULL: no memory
    struct xval *m = malloc(sizeof *m);
    if (m) xo_copy(m, v);
    return m;
}
internal void x_dict_free(struct xdict *d) {
    if (!d) return;
    for (int i = 0; i < d->n; i++) { free(d->e[i].ks); xdict_vfree(&d->e[i]); }
    xdict_vfree(&d->def);
    free(d->e);
    free(d->ix);
    free(d);
}
internal void xdict_put(struct xdict *d, struct xdent *x) {   // an entry as it is (a saved game's): x's texts taken over
    if (d->n == d->cap) {
        struct xdent *m = realloc(d->e, (size_t)(d->cap = d->cap ? d->cap * 2 : 8) * sizeof *m);
        if (!m) { free(x->ks); xdict_vfree(x); return; }
        d->e = m;
    }
    d->e[d->n++] = *x;
}
internal void x_dicts_free(void) {
    while (g_xdicts) {
        struct xdict *d = g_xdicts;
        g_xdicts = d->next;
        x_dict_free(d);
    }
    g_xdict_n = 0;
}
internal const char *xdict_tname(int t, char *buf, int cap) {   // a key's or value's type for the messages
    return t < XT_INT ? lstrcpynA(buf, x_kind_name(t), cap) : xty_name(t, buf, cap);
}
// v fits type t (a key's: k 1) - into *de; 0: a mistake told (what: "[...]", "dict<...>(...)")
internal int xdict_fit(struct xe *e, int t, const struct xval *v, struct xdent *de, int k, const char *what) {
    char tn[40];
    memset(de, 0, sizeof *de);
    if (t < XT_INT) {   // an object of a kind: by its index
        if (v->obj && v->w && v->okind == t) { de->kobj = v->obj; return 1; }
    } else if (t == XT_TEXT) {
        if (v->w && v->has_s && !v->obj && !v->kind && !v->ln) {
            if (k) de->ks = (char *)v->s; else { de->vs = _strdup(v->s); de->vhas_s = 1; }   // (a key: v's own text - xdict_set copies one it keeps)
            return 1;
        }
    } else if (v->w && v->has_n && !v->obj && !v->kind && !v->ln && (t != XT_INT || v->n == floor(v->n))) {   // int float bool
        if (k) de->kn = v->n; else de->vn = v->n;
        return 1;
    }
    x_fail(e, "%s: %.40s is no %s - the dict's %s are %ss", what, xtext_of(v), xdict_tname(t, tn, sizeof tn), k ? "keys" : "values", tn);
    return 0;
}
// THE INDEX: a dict of more than a few entries finds one by its key's hash, not by walking them all (a text key was a
// compare of Windows' with every entry). Made and added to when the dict is asked (entries come from xdict_set, a
// saved game's xdict_put, a personality's fields read back: nidx tells what is new). A text key is the same as
// lstrcmpiA tells it - which no hash knows for a key that isn't plain ASCII: with one such in the dict (odd), or
// asked for, it is walked as before
internal unsigned xdict_hash(const struct xdict *d, const struct xdent *k) {
    unsigned h = 2166136261u;
    if (d->kt < XT_INT) h = (unsigned)k->kobj;
    else if (d->kt == XT_TEXT) {
        for (const unsigned char *s = (const unsigned char *)k->ks; *s; s++) h = (h ^ (*s - 'A' < 26u ? *s + 32u : *s)) * 16777619u;
    } else {
        double x = k->kn == 0 ? 0 : k->kn;   // (-0 is 0)
        uint64_t b;
        memcpy(&b, &x, sizeof b);
        h = (unsigned)(b ^ b >> 32);
    }
    h ^= h >> 16; h *= 0x85EBCA6Bu; h ^= h >> 13; h *= 0xC2B2AE35u; h ^= h >> 16;
    return h;
}
internal void xdict_index(struct xdict *d) {
    if (d->nidx > d->n) d->nidx = d->nix = 0;   // (emptied and filled anew: a personality's fields read back)
    if (d->n * 2 > d->nix) {
        int nix = 64;
        while (nix < d->n * 4) nix *= 2;
        int *m = realloc(d->ix, (size_t)nix * sizeof *m);
        if (!m) { d->nix = d->nidx = 0; return; }
        memset(m, 0, (size_t)nix * sizeof *m);
        d->ix = m; d->nix = nix; d->nidx = 0; d->odd = 0;
    }
    for (int i = d->nidx; i < d->n; i++) {
        if (d->kt == XT_TEXT && !x_plain(d->e[i].ks)) d->odd = 1;
        unsigned h = xdict_hash(d, &d->e[i]) & (unsigned)(d->nix - 1);
        while (d->ix[h]) h = (h + 1) & (unsigned)(d->nix - 1);
        d->ix[h] = i + 1;
    }
    d->nidx = d->n;
}
internal int xdict_same(const struct xdict *d, const struct xdent *x, const struct xdent *k) {
    return d->kt < XT_INT ? x->kobj == k->kobj : d->kt == XT_TEXT ? x_ieq(x->ks, k->ks) : x->kn == k->kn;
}
internal struct xdent *xdict_find(struct xdict *d, const struct xdent *k) {
    if (d->n > 8 && (d->kt != XT_TEXT || x_plain(k->ks))) {
        if (d->nidx != d->n) xdict_index(d);
        if (d->nix && !d->odd) {
            for (unsigned h = xdict_hash(d, k) & (unsigned)(d->nix - 1); d->ix[h]; h = (h + 1) & (unsigned)(d->nix - 1))
                if (xdict_same(d, &d->e[d->ix[h] - 1], k)) return &d->e[d->ix[h] - 1];
            return NULL;
        }
    }
    for (int i = 0; i < d->n; i++) if (xdict_same(d, &d->e[i], k)) return &d->e[i];
    return NULL;
}
internal void xdict_value(const struct xdict *d, const struct xdent *x, struct xval *v) {   // an entry's value (or the default's)
    if (!xdict_plain(d)) { if (x->xv) xo_out(x->xv, v); else xnothing(v); }
    else if (d->vt == XT_TEXT) xstr(v, x->vs ? x->vs : "");
    else xnum(v, x->vn);
}
// val as a value of d's into *nv: a plain one checked (xdict_fit), another made the dict's own; 0: a mistake told
internal int xdict_val(struct xe *e, struct xdict *d, const struct xval *val, struct xdent *nv, const char *what) {
    if (xdict_plain(d)) return xdict_fit(e, d->vt, val, nv, 0, what);
    memset(nv, 0, sizeof *nv);
    if (!xo_ok(e, val)) return 0;
    if (!(nv->xv = xdict_own(val))) { e->err = 1; return 0; }
    return 1;
}
internal void xdict_get(struct xe *e, struct xdict *d, const struct xval *key, struct xval *v) {   // d[key]
    struct xdent k;
    if (!xdict_fit(e, d->kt, key, &k, 1, "[...]")) return;
    const struct xdent *x = xdict_find(d, &k);
    xdict_value(d, x ? x : &d->def, v);
}
internal void xdict_set(struct xe *e, struct xdict *d, const struct xval *key, const struct xval *val) {   // d[key] = val
    struct xdent k, nv;
    if (!xdict_fit(e, d->kt, key, &k, 1, "[...] = ...")) return;
    if (!xdict_val(e, d, val, &nv, "[...] = ...")) return;
    struct xdent *x = xdict_find(d, &k);
    if (x) xdict_vfree(x);
    else {
        if (k.ks && !(k.ks = _strdup(k.ks))) { xdict_vfree(&nv); e->err = 1; return; }   // (the key was the value's own text)
        if (d->n == d->cap) {
            struct xdent *m = realloc(d->e, (size_t)(d->cap ? d->cap * 2 : 8) * sizeof *m);
            if (!m) { free(k.ks); xdict_vfree(&nv); e->err = 1; return; }
            d->e = m;
            d->cap = d->cap ? d->cap * 2 : 8;
        }
        x = &d->e[d->n++];
        *x = k;
    }
    x->vn = nv.vn; x->vs = nv.vs; x->vhas_s = nv.vhas_s; x->xv = nv.xv;
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
    if (!v->w && v->miss) { x_use(e, v); return; }   // lord.x without a lord: who isn't there, told
    if (v->obj && v->w) { x_obj_member(e, v, name, NULL, 0, 0, v, whole); return; }
    if (xb_is(v)) { x_fail(e, "%s: a list has no members", whole); return; }
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
    r->n = nc; r->key = key; r->val = val; r->ty = NULL;
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
// of mod/units.eeai of groups (military: army ...) - 6 cut the prompt's fighting_recent short.
#define X_NEST 12
// the body of a vars entry or function (its source) worked out in this context: compiled, its value - of the names
// of the code that asked for it it sees none by their letters (a function's parameters: c->fpn, x_call)
internal int x_body(struct xe *e, const char *src, struct xval *v) {
    struct xcode *k = x_unit_code(x_compiled(src));
    if (!k) { xnothing(v); return 0; }
    struct xe q = { e->c, 0, NULL, 0, 0 };
    return xvm_eval(k, &q, v, NULL);
}
// a member path's value (name: r.a.b, p.gold - the whole path) from its head's, h (used): a player's the host's own
// way first (p.soldiers), else member by member
internal void x_path_walk(struct xe *e, const char *name, struct xval *h, struct xval *v) {
    if (h->obj && h->w && (!g_x_host.value_kind || h->okind == g_x_host.value_kind) && g_x_host.value(e->c, name, v)) return;
    for (const char *seg = strchr(name, '.') + 1; !e->err;) {
        char m[40];
        size_t k = strcspn(seg, ".");
        lstrcpynA(m, seg, (int)(k < 39 ? k : 39) + 1);
        x_member_of(e, m, h, name);
        if (!seg[k]) break;
        seg += k + 1;
    }
    if (!e->err) *v = *h;
}
internal void xe_ident(struct xe *e, const char *name, struct xval *v) {
    struct xctx *c = e->c;
    if (c->fixed == 1) { x_fail(e, "%s: a name - a setting is a number (35%%, 2 * 60, min(...))", name); xnothing(v); return; }
    const char *dot = strchr(name, '.');
    if (dot && dot - name < 40) {   // r.a.b, home.x: a member path - its head a parameter, variable or vars entry
        char head[40];
        lstrcpynA(head, name, (int)(dot - name) + 1);
        int own = 0, exact = 0;
        for (int i = 0; i < c->fnp && !own; i++) own = !strcmp(c->fpn[i], head);
        if (!own) own = x_view_find(c, head, strlen(head)) != NULL;
        for (int k = 0; k < 3 && !own; k++) exact |= c->vars[k] && c->vars[k]->dotted && xvars_get(c->vars[k], name);   // a file record's member itself
        for (int k = 0; k < 3 && !own && !exact; k++) own = xvars_get(c->vars[k], head) != NULL;
        if (!own && !exact) own = x_field_known(head);   // a personality's field: rival.alive
        if (own) {
            struct xval h;
            xe_ident(e, head, &h);
            if (!e->err) x_path_walk(e, name, &h, v);
            return;
        }
    }
    for (int i = 0; i < c->fnp; i++) if (!strcmp(c->fpn[i], name)) { *v = c->fpv[i]; return; }   // a parameter of the vars function being worked out
    {
        const struct xval *lv = x_view_find(c, name, strlen(name));   // (the VM's: SCRIPTS COMPILED)
        if (lv) { *v = *lv; xb_unbox(v); return; }   // (a ref's place may be a row)
    }
    for (int i = 0; i < c->np; i++)
        if (!strcmp(c->pn[i], name)) { xstr_or_num(v, c->pv[i]); return; }
    if (g_x_host.field_get(c, name, v)) return;   // a field of the speaker's personality (let at a file's top)
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
    if (c->fixed) { x_fail(e, "an unknown name: %s", name); xnothing(v); return; }
    if (c->script && !strcmp(name, "stop")) { xnothing(v); v->w = 1; v->kind = XV_STOP; return; }   // for / every: no more
    if (c->script && !strcmp(name, "time")) { xnum(v, g_x_hooks.now ? g_x_hooks.now() : 0); return; }   // game seconds
    int k = g_x_host.constant(name, v);
    if (k) { e->kept = k == 2; return; }
    if (c->nogame && !x_ieq(name, "language") && (g_x_host.is_value(name) || strchr(name, '.'))) { x_nogame(e, name); return; }
    if (g_x_host.value(c, name, v)) return;
    if (x_vars_no_member(e, name)) return;   // colors.missing
    x_fail(e, !strcmp(name, "name") ? "an unknown name: name - the other player is them (them.name)" : "an unknown name: %s", name);
}
// a function's body (def), its parameters (par "a,b)") bound to the arguments: they alone are its names besides the
// game's values and the vars - a function calling itself ends at the nesting limit like vars in a circle
internal void x_call_value(struct xe *e, const struct xval *fv, const struct xval *a, int na, struct xval *v);
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
    if (def[0] == X_BODY) {   // name(a, b) { ... }: statements - kept as the function (a, b) => { ... }, called as a script's are
        struct xcode *k = x_unit_code(x_compiled(def + 1));
        struct xval fv;
        if (!k || !xvm_eval(k, e, &fv, NULL) || fv.kind != XV_FN) { e->err = 1; return; }
        int fnp = c->fnp;   // (its parameters are its own slots; its caller's names are not its to see)
        c->fnp = 0;
        c->depth++;
        x_call_value(e, &fv, a, na, v);
        c->depth--;
        c->fnp = fnp;
        return;
    }
    const char *const *fpn = c->fpn;
    const struct xval *fpv = c->fpv;
    int fnp = c->fnp;
    c->fpn = pn; c->fpv = a; c->fnp = n;
    c->depth++;
    if (!x_body(e, def, v) || c->circle) e->err = 1;
    c->depth--;
    c->fpn = fpn; c->fpv = fpv; c->fnp = fnp;
}
// a value with its texts and lists copied into the region now (what a function hands back, copied past its frame)
internal void xv_copy(struct xval *dst, const struct xval *src) {
    *dst = *src;
    if (src->kind == XV_REF) { xnothing(dst); dst->kind = XV_UNSET; return; }   // (a place is its function's alone: none in a copy that outlives it)
    if (src->has_s || src->w) dst->s = x_strdup(src->s, strlen(src->s));
    if (src->kind == XV_VEC) xvec(dst, src->vc, src->vn);
    if (src->kind == XV_REC) {   // its members, each copied
        struct xrec *r = x_alloc(sizeof *r);
        const char **key = x_alloc((size_t)src->rec->n * sizeof *key + 1);
        struct xval *val = x_alloc((size_t)src->rec->n * sizeof *val + 1);
        if (!r || !key || !val) { xnothing(dst); return; }
        r->n = src->rec->n; r->key = key; r->val = val; r->ty = src->rec->ty;
        if (r->ty) r->key = src->rec->key;   // (a struct's names: its type's, which stay)
        for (int i = 0; i < r->n; i++) { if (!r->ty) key[i] = x_strdup(src->rec->key[i], strlen(src->rec->key[i])); xv_copy(&val[i], &src->rec->val[i]); }
        dst->rec = r;
    }
    if (src->ln) {
        struct xval *it = x_alloc((size_t)src->ln * sizeof *it);
        if (!it) { dst->ln = 0; dst->it = NULL; return; }
        for (int i = 0; i < src->ln; i++) xv_copy(&it[i], &src->it[i]);
        dst->it = it;
    }
}
// function value fv called with its arguments: on its machine, above what stands there (xvm_call)
internal void x_call_value(struct xe *e, const struct xval *fv, const struct xval *a, int na, struct xval *v) { xvm_call(e, fv, a, na, v); }
// the script functions: wait(seconds), wait_until(time), every(step, from, until, f), player(n), cmd(text), the triggers'
internal const struct { const char *n; int args; } k_x_script_fns[] = { { "wait", 1 }, { "wait_until", 1 }, { "every", 4 }, { "player", 1 }, { "cmd", 1 },
                                                                     { "trigger", 1 }, { "after", 2 }, { "enable", 1 }, { "disable", 1 } };
#define N_X_SCRIPT_FNS (int)(sizeof k_x_script_fns / sizeof *k_x_script_fns)
internal int x_script_fn(const char *name) {   // a script's own function: its arguments, -1 none such
    for (int i = 0; i < N_X_SCRIPT_FNS; i++) if (!strcmp(k_x_script_fns[i].n, name)) return k_x_script_fns[i].args;
    return -1;
}
// a value as a command's argument: a number (whole without decimals), a text as it is, a player its number, a type
// its number (unit.x: both), a vector its numbers one argument each (a place: x y)
internal int x_cmd_arg(struct xe *e, const char *verb, const struct xval *v, char *out, int cap) {
    if ((v->kind && v->kind != XV_VEC) || !v->w || v->ln) { x_fail(e, "%s(...): %s as a command's argument", verb, v->ln || v->kind == XV_HLIST ? "a list" : v->kind == XV_FN ? "a function" : v->kind == XV_REC ? "a record" : v->kind ? "stop" : "nothing"); return 0; }
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
    v->has_s = 1; v->w = 1; v->has_n = 0; v->obj = v->okind = v->ln = 0; v->it = NULL; v->miss = NULL; v->kind = 0;
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
// (fresh: made in the triggers' turn that runs - due at once, after(0s, f), it waits for the next turn, else an after
// that makes itself again would run on in that turn for ever)
struct xafter { double at; struct xval fn; char name[48]; int fresh; };
internal struct xafter g_xa[XA_MAX];
internal int g_xa_n;
// a game's own triggers and after's set aside while a dry run has the tables to itself (test/scripts.c script dryrun):
// the lists and dicts their functions hold are still kept by the sweep (xhl_mark_roots)
internal const struct xtrig *g_xt_aside;
internal const struct xafter *g_xa_aside;
internal int g_xt_aside_n, g_xa_aside_n;
internal struct xregion g_xt_keep, g_xt_tmp;   // what they keep (their functions, frames); a turn's values
internal double g_xt_next;   // the next check (game seconds)
internal void x_triggers_reset(void) {   // a new game: none (nor dicts, nor lists)
    x_dicts_free();
    xhl_free_all();
    g_xt_n = g_xa_n = 0;
    g_xt_next = 0;
    x_region_free(&g_xt_keep);
    x_region_free(&g_xt_tmp);
}
// a function value kept for later (in g_xt_keep, which x_alloc uses meanwhile): the view of the function it was made
// in copied (its slots) unless it stays (keep), each view once (memo: a function in it made there too)
struct xt_memo { const struct xview *vfrom[16]; struct xview *vto[16]; int vn; };
internal void xt_keep_val(struct xval *dst, const struct xval *src, struct xt_memo *m);
internal struct xview *xt_keep_view(struct xview *w, struct xt_memo *m) {
    if (!w || w->keep) return w;
    for (int i = 0; i < m->vn; i++) if (m->vfrom[i] == w) return m->vto[i];
    struct xview *c = x_alloc(sizeof *c);
    struct xval *val = x_alloc((size_t)w->n * sizeof *val + 1);
    if (!c || !val) return NULL;
    if (m->vn < 16) { m->vfrom[m->vn] = w; m->vto[m->vn++] = c; }
    c->name = w->name; c->val = val; c->n = w->n; c->keep = 1;
    for (int i = 0; i < w->n; i++) xt_keep_val(&val[i], &w->val[i], m);
    c->up = xt_keep_view(w->up, m);
    return c;
}
internal void xt_keep_val(struct xval *dst, const struct xval *src, struct xt_memo *m) {
    xv_copy(dst, src);
    if (src->kind != XV_FN) return;
    struct xfn *fn = x_alloc(sizeof *fn);
    if (!fn) { xnothing(dst); return; }
    *fn = *src->fn;
    fn->venv = xt_keep_view(src->fn->venv, m);
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
internal int xt_fn0(const struct xval *f) { return f && f->kind == XV_FN && f->w && xfn_nparams(f->fn) == 0; }   // () => ...
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
    // (no function: one a save couldn't keep - of no script's code, a text's {placeholder}: xs_put writes it as nothing
    // - or a broken save's; told and stopped, not called through a function that isn't there)
    if (fn->kind != XV_FN || !fn->fn || !fn->fn->vm) { xnothing(out); x_error("trigger %s: %s stopped - no function (one a save couldn't keep, or a broken save)", who, what); return 0; }
    struct xctx c = { 0 };
    c.found = 1;
    c.script = 1;
    c.top = who;
    struct xe e = { &c, 0, NULL, 0, 0 };
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
    g_xt_tmp.busy++;   // a {placeholder} filled in the turn (x_eval) never starts the region afresh under its values
    g_xt_in = 1;
    for (;;) {   // the after's and timers due, earliest first (a timer behind: each step it missed)
        int k = -1, kt = -1;
        for (int i = 0; i < g_xa_n; i++) if (!g_xa[i].fresh && g_xa[i].at <= now + 1e-9 && (k < 0 || g_xa[i].at < g_xa[k].at)) k = i;
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
    for (int i = 0; i < g_xa_n; i++) g_xa[i].fresh = 0;
    g_xt_in = 0;
    g_xt_tmp.busy--;
    x_region_reset(&g_xt_tmp);
    g_xr = r0;
}
// the bytes of a saved game's blocks written and read (the scripts': SAVED STATE at the file's end; the personalities'
// fields: voice/persona.c)
struct pbuf { uint8_t *b; uint32_t n, cap; int bad; };   // bytes written (the personalities' fields too: voice/persona.c)
internal void pb_put(struct pbuf *o, const void *p, uint32_t n) {
    if (o->n + n > o->cap) {
        uint32_t cap = (o->n + n) * 2 + 256;
        uint8_t *m = realloc(o->b, cap);
        if (!m) { o->bad = 1; return; }
        o->b = m;
        o->cap = cap;
    }
    memcpy(o->b + o->n, p, n);
    o->n += n;
}
internal void pb_str(struct pbuf *o, const char *s) { uint16_t n = (uint16_t)(strlen(s) < 65535 ? strlen(s) : 65535); pb_put(o, &n, 2); pb_put(o, s, n); }
internal void pb_num(struct pbuf *o, double x) { pb_put(o, &x, 8); }
internal void pb_int(struct pbuf *o, int32_t x) { pb_put(o, &x, 4); }
internal void pb_text(struct pbuf *o, const char *s) { uint32_t n = (uint32_t)strlen(s); pb_put(o, &n, 4); pb_put(o, s, n); }   // a long one
struct pread { const uint8_t *b; uint32_t n, o; int bad; };   // and read: past the end is bad, zeros from then on
internal void pr_get(struct pread *r, void *p, uint32_t n) {
    if (r->bad || n > r->n - r->o) { r->bad = 1; memset(p, 0, n); return; }
    memcpy(p, r->b + r->o, n);
    r->o += n;
}
internal char *pr_str(struct pread *r) {   // malloc'd
    uint16_t n = 0;
    pr_get(r, &n, 2);
    char *s = malloc((size_t)n + 1);
    if (!s) { r->bad = 1; return NULL; }
    pr_get(r, s, n);
    s[n] = 0;
    return s;
}
internal double pr_num(struct pread *r) { double x = 0; pr_get(r, &x, 8); return x; }
internal int32_t pr_int(struct pread *r) { int32_t x = 0; pr_get(r, &x, 4); return x; }
internal char *pr_text(struct pread *r) {   // malloc'd
    uint32_t n = 0;
    pr_get(r, &n, 4);
    if (r->bad || n > r->n - r->o) { r->bad = 1; n = 0; }
    char *s = malloc((size_t)n + 1);
    if (!s) { r->bad = 1; return NULL; }
    pr_get(r, s, n);
    s[n] = 0;
    return s;
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
        q->fresh = g_xt_in;
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
    // (a script's wait, wait_until and every are its machine's instructions: here they are a text's - "{wait(5)}" in a
    // script's string - where there is nothing to wait in)
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
// a struct's method called from a text's code - a {placeholder} of a script's string, which knows no types: found by
// its letters and called on a copy of the struct (the check lets only one that changes nothing be called there).
// The script that fills the string: its machine and code, set while it does (XI_STR). 1: called (or told), 0: no
// script's struct here
internal struct xvm *g_x_ph_vm;
internal struct xcode *g_x_ph_code;
internal int x_ph_method(struct xe *e, const struct xval *o, const char *mname, const struct xval *a, int na, struct xval *v);
// name(a ...) with its arguments worked out (nall of them, at most 32): a function in a variable, one of the
// language's, a vars file's, a script's, a command, a relation or method of the host's; view 0: no variable has the
// name (a script's code knows its own when it is compiled: a call by name there is none of them)
internal void x_call_apply(struct xe *e, const char *name, struct xval *a, int nall, int view, struct xval *v) {
    int na = nall < 8 ? nall : 8;
    if (!strchr(name, '.')) {   // a function in a variable or parameter
        const struct xval *lv = view ? x_view_find(e->c, name, strlen(name)) : NULL;
        for (int i = 0; i < e->c->fnp; i++)
            if (!strcmp(e->c->fpn[i], name)) {
                if (e->c->fpv[i].kind != XV_FN) { x_fail(e, "%s is no function", name); return; }
                struct xval fv = e->c->fpv[i];
                x_call_value(e, &fv, a, nall, v);
                return;
            }
        if (lv) {   // (the VM's: SCRIPTS COMPILED)
            if (lv->kind != XV_FN) { x_fail(e, "%s is no function", name); return; }
            struct xval fv = *lv;
            x_call_value(e, &fv, a, nall, v);
            return;
        }
    }
    int f = strchr(name, '.') ? 0 : x_fn_id(name, na);   // (no builtin has a dot: a relation, a method)
    if (f == 2) na = nall;   // min, max: all of theirs (x_fn_id takes up to 127, the compiler 32) - the rest take 8 at most
    for (int i = 0; f && i < na; i++) if (a[i].miss) x_use(e, &a[i]);   // a function's arguments (a relation checks its players) - each told
    if (e->err) return;
    int hlv = 0;   // a list as len's, first's, item's, contains', pick's, map_range's: its items as a seq for the call
    if (f == 7 || f == 8 || f == 9 || f == 10 || f == 11 || f == 15)
        for (int i = 0; i < na; i++)
            if (a[i].kind == XV_HLIST && a[i].w) {
                struct xhlist *h = a[i].hl;
                xnothing(&a[i]);
                if (h->n) { a[i].it = h->it; a[i].ln = a[i].w = h->n; }
                hlv = 1;
            }
    if (e->c->fixed && (f == 1 || f == 5 || !f)) {   // a setting: fixed - no draws, no vars functions
        x_fail(e, "%s(): a setting is a fixed number (no chance(), rnd() or names)", name);
        return;
    }
    if (f == 1 && a[0].has_n && a[0].n > 1) {   // chance(70): meant 70%
        x_fail(e, "chance(%g): a probability is 0 to 1 - chance(70%%) or chance(0.7)", a[0].n);
        return;
    }
    if (f && (f <= 6 || f == 14 || f >= 20))   // the number functions: numbers (or nothing) only
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
        if (dot && !own) own = x_view_find(e->c, name, hn) != NULL;
        if (own && dot - name < 120) {
            char path[128];
            struct xval o;
            lstrcpynA(path, name, (int)(dot - name) + 1);
            xe_ident(e, path, &o);
            if (e->err) return;
            if (o.obj && o.w) { x_obj_member(e, &o, dot + 1, a, nall, 1, v, name); return; }
            if (!o.w && o.miss) { x_use(e, &o); return; }
            if (o.kind == XV_REC && o.w && o.rec->ty) {
                if (!x_ph_method(e, &o, dot + 1, a, nall, v))
                    x_fail(e, "%s(...): a struct's method is called where the check knows the struct - let name: %s = ..., a parameter's type", name, o.rec->ty->name);
                return;
            }
            x_fail(e, "%s(...): %.40s is no object - a method is a player's, the game's ...", name, xtext_of(&o));
            return;
        }
        e->err = 1; e->why = "an unknown function or relation (or the wrong number of arguments)";
        return;
    }
    if (f == 1) xnum(v, x_rnd01(e->c) < xto_num(&a[0]));   // a probability: chance(70%), chance(0.7)
    else if (f == 2) {
        int mx = x_ieq(name, "max");
        double r = xto_num(&a[0]);
        for (int i = 1; i < na; i++) { double x = xto_num(&a[i]); if (mx ? x > r : x < r) r = x; }
        xnum(v, r);
    } else if (f == 3) { double x = xto_num(&a[0]); xnum(v, x < 0 ? -x : x); }
    else if (f == 4) { double x = xto_num(&a[0]); xnum(v, (double)(long long)(x < 0 ? x - 0.5 : x + 0.5)); }
    else if (f == 6) { double x = xto_num(&a[0]), r = (double)(long long)x; xnum(v, r > x ? r - 1 : r); }   // down: floor(-1.5) -2
    else if (f == 15) { *v = a[0]; x_pick(e->c, v); xb_unbox(v); }   // pick(l): one of its items at random (a value: itself)
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
        if (i >= 0 && i < xl_n(l)) { *v = *xl_at(l, i); v->w = 1; v->ln = 0; v->it = NULL; xb_unbox(v); }
    } else if (f == 10) {   // contains(l, x): one of its items equal to x (as ==)
        int hit = 0;
        for (int i = 0; i < xl_n(&a[0]) && !hit; i++) {   // (a row of a list of lists: the list it is, the same item for item)
            struct xval it = *xl_at(&a[0], i);
            xb_unbox(&it);
            hit = a[1].w && (it.ln || a[1].ln ? x_same(&it, &a[1]) : !xcmp_as(&it, &a[1], 1));
        }
        xnum(v, hit);
    }
    else if (f == 14) xnum(v, det_log2f((float)xto_num(&a[0])));   // the same bits on every PC (base/math.c)
    else if (f == 20) xnum(v, (double)(long long)xto_num(&a[0]));   // int(x): toward 0 (int(-2.5) -2), int(epoch) its number
    else if (f == 21) { double x = xto_num(&a[0]), r = (double)(long long)x; xnum(v, r < x ? r + 1 : r); }   // ceil(1.2) 2
    else xnum(v, x_rnd01(e->c) * (na ? xto_num(&a[0]) : 1));   // rnd(n): 0 <= x < n, rnd(): 0 <= x < 1
    if (hlv && (f == 8 || f == 9 || f == 11 || f == 15)) { struct xval t = *v; xvm_copy(v, &t); }   // (an item of a list: copied out)
}
internal void x_str_fill(struct xe *e, const char *tmpl, struct xval *v) {   // a string: its placeholders filled
    int f0 = e->c->failed;
    size_t cap = strlen(tmpl) * 2 + 4096;
    char small[6144], *f = cap <= sizeof small ? small : malloc(cap);   // (a short one - nearly all - without malloc)
    if (!f) { e->err = 1; return; }
    x_fill(f, (int)cap, tmpl, e->c);
    if (e->c->failed > f0) { e->err = 1; e->why = "a mistake while working it out"; }   // told in x_fill
    else xstr(v, f);
    if (f != small) free(f);
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
// THE OPERATIONS on values already worked out - what an instruction of the machine (SCRIPTS COMPILED) does with its
// operands, apart from how they came to be. Each takes its operands and leaves its value in v
internal const char k_x_knum[] = "#";   // a kept constant's text place: none - a number alone
internal void xop_name(struct xe *e, const char *name, double *kn, const char **kcs, struct xval *v) {   // a name's value
    struct xctx *c = e->c;
    if (*kcs && !c->fixed) {   // a constant that stays, looked up before (a setting takes no names: told below)
        if (*kcs == k_x_knum) xnum(v, *kn); else xdual_kept(v, *kn, *kcs);
        return;
    }
    int kept = e->kept;   // a name in a name (a vars entry's): its own
    e->kept = 0;
    xe_ident(e, name, v);
    // a constant the host says stays the same (kind.tank, unit.citizen): kept where it is named (filters must be quick)
    if (!e->err && e->kept && v->has_s && v->has_n && v->n >= 0) { *kn = v->n; *kcs = v->s; }
    else if (!e->err && e->kept && v->has_n && !v->has_s && v->w == 1 && !v->obj && !v->kind && !v->ln) { *kn = v->n; *kcs = k_x_knum; }   // (epoch.middle: a number alone)
    e->kept = kept;
    if (c->missed) { c->missed = 0; e->err = 1; e->why = "a mistake while working it out"; }   // told (x_missing)
}
internal void xop_index(struct xe *e, const struct xval *l, const struct xval *ix, struct xval *v) {   // l[i], d[k] (l used)
    struct xval i = *ix;
    if (l->kind == XV_DICT) {   // d[k]: its entry, else the default
        x_use(e, &i);
        if (!e->err) xdict_get(e, l->dict, &i, v);
        return;
    }
    if (l->kind == XV_HLIST && l->w) {   // a list's item: copied out
        int k = (int)i.n;
        if (!x_numeric_ok(e, &i)) return;
        if (!i.w || (double)k != i.n || k < 1 || k > l->hl->n) { x_fail(e, "[%s]: no such item - the list has %d (the first is [1])", xtext_of(&i), l->hl->n); return; }
        xhl_out(l->hl, k - 1, v);
        return;
    }
    if (l->w && !l->ln) { x_fail(e, "[...]: %.40s is no list", l->kind == XV_REC ? "a record" : l->kind == XV_VEC ? "a vector" : xtext_of(l)); return; }
    if (!x_numeric_ok(e, &i)) return;
    int k = (int)i.n;
    if (!i.w || (double)k != i.n || k < 1 || k > xl_n(l)) { x_fail(e, "[%s]: no such item - the list has %d (the first is [1])", xtext_of(&i), xl_n(l)); return; }
    *v = *xl_at(l, k - 1);
    v->w = 1; v->ln = 0; v->it = NULL;
    xb_unbox(v);   // (a row: the list it is)
}
internal void xop_mcall(struct xe *e, const char *name, const struct xval *o, const struct xval *a, int na, struct xval *v) {   // o.name(a ...)
    char whole[48];
    snprintf(whole, sizeof whole, ".%s", name);
    if (o->kind == XV_REC && o->w && o->rec->ty) {
        if (!x_ph_method(e, o, name, a, na, v))
            x_fail(e, "%s(...): a struct's method is called where the check knows the struct - let name: %s = ..., a parameter's type", whole, o->rec->ty->name);
        return;
    }
    if (!o->obj || !o->w) { x_fail(e, "%s(...): %.40s is no object - a method is a player's, the game's ...", whole, xtext_of(o)); return; }
    x_obj_member(e, o, name, a, na, 1, v, whole);
}
internal void xop_dict(struct xe *e, int types, const struct xval *d, struct xval *v) {   // dict<K, V>(default): a new one (d used)
    struct xdict *t = calloc(1, sizeof *t);
    if (!t) { e->err = 1; return; }
    t->kt = types >> 16;
    t->vt = types & 0xFFFF;
    if (!g_xdict_keep) { t->next = g_xdicts; g_xdicts = t; g_xdict_n++; g_xhl_made++; }   // (the sweep's to free)
    if (!xdict_val(e, t, d, &t->def, "dict<K, V>(default)")) { if (g_xdict_keep) x_dict_free(t); return; }
    xnothing(v);
    v->w = 1; v->kind = XV_DICT; v->dict = t; v->s = "a dict";
}
internal void xop_neg(struct xe *e, struct xval *v) {   // -v
    x_use(e, v);
    if (e->err) return;
    if (v->kind == XV_VEC) { double neg[4]; for (int i = 0; i < v->vn; i++) neg[i] = -v->vc[i]; xvec(v, neg, v->vn); return; }
    if (!x_numeric_ok(e, v)) return;
    xnum(v, -xto_num(v));
}
internal void xe_vec_op(struct xe *e, int op, struct xval *a, const struct xval *b);
internal void xop_arith(struct xe *e, int mul, int op, struct xval *v, struct xval *b) {   // v op b: * / (mul), + -
    x_use(e, v);
    if (!e->err) x_use(e, b);
    if (e->err) return;
    if (v->kind == XV_VEC || b->kind == XV_VEC) { xe_vec_op(e, op, v, b); return; }
    double x, y;
    if (mul || op == '-') {
        if (!x_numeric_ok(e, v) || !x_numeric_ok(e, b)) return;
        x = xto_num(v); y = xto_num(b);
        if (op == '/' && y == 0) { x_fail(e, "a division by 0"); return; }
        xnum(v, op == '*' ? x * y : op == '/' ? x / y : x - y);
    }
    // + adds numbers only: a text is built in its string ("Age: {epoch}", "{a}{b}") - "a" + b could mean b
    // added to the text or the text to b's items; nothing counts as 0
    else if ((v->has_s && !v->has_n) || (b->has_s && !b->has_n))
        x_fail(e, "a text (\"%.40s\") added - + adds numbers, a text is built in its string: \"Age: {epoch}\", \"{a}{b}\"", xtext_of(v->has_s && !v->has_n ? v : b));
    else if (!x_numeric_ok(e, v) || !x_numeric_ok(e, b)) return;
    else xnum(v, xto_num(v) + xto_num(b));   // an epoch is a number too: epoch + 1
}
// v op b compared (an: a is the null literal, bn: b is)
internal void xop_cmp(struct xe *e, int op, int an, int bn, struct xval *v, struct xval *b) {
    if (an || bn) {   // x == null, x != null: whether something is there (lord without one: none)
        const struct xval *x = an ? b : v;
        int there = x->w > 0 || x->ln;
        if (op != '=' && op != '!') { x_fail(e, "null has no order - x == null or x != null"); return; }
        xnum(v, there == (op == '!'));
        return;
    }
    if (v->ln || b->ln) {   // [1, 2] == 1: no one of them - contains(l, x)
        x_fail(e, "a list (%.40s) compared - contains(l, x) for one of its items", xtext_of(v->ln ? v : b));
        return;
    }
    if ((v->kind == XV_REC && v->w) || (b->kind == XV_REC && b->w)) {   // no order, no whole equality: its members
        const struct xval *s = v->kind == XV_REC && v->w ? v : b;
        if (s->rec->ty && (op == '=' || op == '!')) {   // a struct: the same as another when every field is (a value, as a vector)
            xnum(v, (v->kind == XV_REC && b->kind == XV_REC && v->w && b->w && x_same(v, b)) == (op == '='));
            return;
        }
        x_fail(e, s->rec->ty ? "a struct (%.40s) has no order - == or != only" : "a record (%.40s) compared - compare its members (r.name == ...)", xtext_of(s));
        return;
    }
    if (!v->w || !b->w) { xnum(v, 0); return; }   // nothing compares with nothing: false
    if (v->kind == XV_VEC || b->kind == XV_VEC) {   // vectors: equal or not - no order, nothing else alike
        if (v->kind != b->kind || (op != '=' && op != '!')) {
            x_fail(e, "%.40s %s %.40s: a vector is only == or != another vector", xtext_of(v), op == 'l' ? "<=" : op == 'g' ? ">=" : op == '=' ? "==" : op == '!' ? "!=" : op == '<' ? "<" : ">", xtext_of(b));
            return;
        }
        xnum(v, xvec_eq(v, b) == (op == '='));
        return;
    }
    if (v->kind == XV_HLIST || b->kind == XV_HLIST) {   // lists: the same one or not (its items: compare them)
        if (op != '=' && op != '!') { x_fail(e, "a list has no order - == or != (the same list)"); return; }
        xnum(v, (v->kind == b->kind && v->hl == b->hl) == (op == '='));
        return;
    }
    if (v->kind == XV_DICT || b->kind == XV_DICT) {   // dicts: the same table or not
        if (op != '=' && op != '!') { x_fail(e, "a dict has no order - == or != (the same dict)"); return; }
        xnum(v, (v->kind == b->kind && v->dict == b->dict) == (op == '='));
        return;
    }
    if (!(v->obj && b->obj) && xmismatch(v, b)) {
        x_fail(e, "a text (\"%.40s\") compared with a number - a number has no quotes, number(t) makes one", xtext_of(v->has_s ? v : b));
        return;
    }
    int d = xcmp_as(v, b, op == '=' || op == '!');
    xnum(v, op == '<' ? d < 0 : op == '>' ? d > 0 : op == 'l' ? d <= 0 : op == 'g' ? d >= 0 : op == '=' ? d == 0 : d != 0);
}
// d[k] = val, d[k] op= val (d, k and val used)
internal void xop_seti(struct xe *e, int op, const struct xval *d, const struct xval *key, struct xval *v) {
    if (d->kind == XV_HLIST && d->w) {   // l[i] = v of a list: the item in it (op: what it is, op v)
        struct xval kk = *key;
        int i = (int)key->n;
        if (!x_numeric_ok(e, &kk)) return;
        if (!key->w || (double)i != key->n || i < 1 || i > d->hl->n) { x_fail(e, "[%s]: no such item - the list has %d (the first is [1])", xtext_of(key), d->hl->n); return; }
        if (op) {
            struct xval cur;
            xhl_out(d->hl, i - 1, &cur);
            if (!x_numeric_ok(e, &cur) || !x_numeric_ok(e, v)) return;
            double x = xto_num(&cur), y = xto_num(v);
            if (op == '/' && y == 0) { x_fail(e, "a division by 0"); return; }
            xnum(v, op == '+' ? x + y : op == '-' ? x - y : op == '*' ? x * y : x / y);
        }
        xhl_put(e, d->hl, i - 1, v, 0);
        return;
    }
    if (d->kind != XV_DICT) { x_fail(e, "[...] = ...: %.40s is no dict - a seq's item is set where the check knows the seq (let l: seq<int> = ..., a parameter's type)", xtext_of(d)); return; }
    if (op) {   // d[k] += 1: what it holds now (or the default), op the value
        struct xval cur;
        xdict_get(e, d->dict, key, &cur);
        if (e->err || !x_numeric_ok(e, &cur) || !x_numeric_ok(e, v)) return;
        double x = xto_num(&cur), y = xto_num(v);
        if (op == '/' && y == 0) { x_fail(e, "a division by 0"); return; }
        xnum(v, op == '+' ? x + y : op == '-' ? x - y : op == '*' ? x * y : x / y);
    }
    xdict_set(e, d->dict, key, v);
}
// o.name = val, o.name op= val: a member its kind lets be set, by the host (o used)
internal void xop_setm(struct xe *e, const char *name, int op, const struct xval *o, struct xval *v) {
    if (o->kind == XV_REC && o->w && o->rec->ty) {
        x_fail(e, ".%s = ...: a struct's field is set where the check knows the struct - let name: %s = ..., a parameter's type", name, o->rec->ty->name);
        return;
    }
    if (!o->obj || !o->w) { x_fail(e, ".%s = ...: %.40s is no object", name, xtext_of(o)); return; }
    const struct x_kmember *m = x_kind_member(o->okind, name);
    if (!m || !m->set) { x_fail(e, ".%s = ...: a %s's %s can't be set", name, x_kind_name(o->okind), name); return; }
    if (op) {   // l[1].hp -= 50: what it is now, op the value
        struct xval cur;
        char whole[60];
        snprintf(whole, sizeof whole, ".%s %c= ...", name, op);
        x_obj_member(e, o, name, NULL, 0, 0, &cur, whole);
        if (e->err || !x_numeric_ok(e, &cur) || !x_numeric_ok(e, v)) return;
        double x = xto_num(&cur), y = xto_num(v);
        if (op == '/' && y == 0) { x_fail(e, "a division by 0"); return; }
        xnum(v, op == '+' ? x + y : op == '-' ? x - y : op == '*' ? x * y : x / y);
    }
    if (!g_x_host.set(e->c, o, name, v)) { e->c->missed = 0; e->err = 1; e->why = "a mistake while working it out"; }   // (told)
}
// evaluate the whole of src (an expression and nothing after it); 0: not an expression
internal void x_log_bad(const char *what, const char *src) { x_error("%s: %.120s", what, src); }
internal int x_eval(const char *src, struct xctx *c, struct xval *v) {   // its value ends up in a text: a line, a {placeholder}
    int top = !c->top;
    if (top) c->top = src;
    if (!g_xr->busy++) x_region_reset(g_xr);   // the outermost: the last one's values are done with
    struct xcode *k = x_unit_code(x_compiled(src));
    struct xe e = { c, 0, NULL, 0, 0 };
    xnothing(v);
    if (!k) e.err = 1;
    else xvm_eval(k, &e, v, c->view);   // (a {placeholder} of a script's string sees the script's names)
    g_xr->busy--;
    int ok = !e.err && !c->circle;
    if (ok) { x_use(&e, v); ok = !e.err; }   // a player who isn't there, written in the text
    if (top) c->top = NULL;
    if (c->circle && !c->depth) { c->circle = 0; x_log_bad("vars naming each other in a circle", src); }
    return ok;   // 0 and c->failed up: a mistake while working it out, told
}

// an ini file's number (ini.c): a constant expression - numbers, %, + - * / ( ), min, max, abs, round, floor, log2;
// true and false (1, 0) and comparisons too - *is_bool says it was one (the type check's XT_BOOL), for the reader to
// take only where a bool belongs; 0: a mistake, *why (not logged here)
internal int x_const_eval(const char *src, double *out, const char **why, int *is_bool) {
    struct xctx c;
    memset(&c, 0, sizeof c);
    c.found = 1; c.nogame = 1; c.fixed = 1;
    struct xval v;
    g_x_last_fail[0] = 0;
    if (!x_eval(src, &c, &v)) { *why = g_x_last_fail[0] ? g_x_last_fail : "not a number or an expression"; return 0; }
    if (!v.w || !v.has_n || v.has_s || v.obj || v.kind) { *why = "a number expected"; return 0; }
    *out = v.n;
    const struct xunit *u = x_compiled(src);
    struct xcheck k;
    struct xscope sc;
    memset(&k, 0, sizeof k);
    memset(&sc, 0, sizeof sc);
    k.c = &c;
    k.script = 1;   // the literals' and operators' types
    *is_bool = u->ok && x_stype(&k, u->root, &sc) == XT_BOOL;
    return 1;
}
// an ini file's vec2 (settings.c setting_vec2): a constant expression as x_const_eval's whose value is two numbers - and
// names, n pairs (name, its expression; a later one over an earlier): aoc * 150%, 1280x720
internal int x_const_vec2(const char *src, const char *const *names, int n, double out[2], const char **why) {
    static struct xvars vars;
    for (int i = 0; i < vars.n; i++) { free(vars.key[i]); free(vars.val[i]); }
    memset(&vars, 0, sizeof vars);
    for (int i = 0; i < n; i++) xvars_set(&vars, names[2 * i], names[2 * i + 1]);
    struct xctx c;
    memset(&c, 0, sizeof c);
    c.found = 1; c.nogame = 1; c.fixed = 2;
    c.vars[0] = &vars;
    struct xval v;
    g_x_last_fail[0] = 0;
    if (!x_eval(src, &c, &v)) { *why = g_x_last_fail[0] ? g_x_last_fail : "not an expression"; return 0; }
    if (v.kind != XV_VEC || v.vn != 2) { *why = "a size expected: 1024x768, vec(w, h), a name * a number"; return 0; }
    out[0] = v.vc[0], out[1] = v.vc[1];
    return 1;
}
// the syntax of an expression, and the checks a file gets when read (x_check); nothing worked out (no game needed);
// 0: a mistake, *why what it is when known
internal int x_ph_ok(struct xcheck *k, const struct xnode *root, struct xscope *sc, const char *src, const char **why);
// ph: src is a {placeholder}'s - what may be null is a mistake there. Typed (k.texts): a text's names have their types,
// the key's guaranteed ones (g_x_check_has) are there
internal int x_syntax_at(const char *src, const char **why, int ph) {
    struct xctx c = { 0 };
    c.found = 1;   // no players looked up
    const struct xunit *u = x_compiled(src);
    if (!u->ok) { *why = u->why ? u->why : "not an expression"; return 0; }
    if (g_xrc && !g_xrc->code) { g_xrc->code = u->src; g_xrc->n = strlen(u->src); }   // (the language server's record: the tree's source)
    struct xcheck k = { &c, 0, NULL, 0, "", 0, 0, { 0 }, { 0 }, 0, 0, 1, 0 };
    struct xscope sc = { NULL, { 0 }, 0, { 0 }, 0, { 0 }, { 0 } };
    xn_has(&k);
    if (x_check(&k, u->root, &sc) && (!ph || x_ph_ok(&k, u->root, &sc, src, why))) return 1;
    static char kept[sizeof k.whybuf];   // the message outlives k: its whybuf is this frame's (the caller's next calls wrote over it)
    lstrcpynA(kept, k.why ? k.why : "not an expression", sizeof kept);
    *why = kept;
    return 0;
}
internal int x_syntax(const char *src, const char **why) { return x_syntax_at(src, why, 0); }

internal const char *x_close(const char *s);
// A placeholder's FORMAT: {x:%} {x:.1%} {x:.2} - after a ':' that belongs to no '?' (outside strings and brackets: the
// ternary's own colons pair with its ?s), how its number is written: % as a share (0.5 -> 50%), .N with N decimals
// (0..6; both: .1% -> 62.5%). Without one a number is written as it is (up to 2 decimals). A number is never
// written as a share by itself: 50% in an expression is only another way to write 0.5.
// A choice in front of a format must be in ( ): {(a ? 1 : 0):%} - written {a ? 1 : 0:%} the format reads as the 0's
// only (likewise {a ?? b:%}); *choice: a ? or ?? outside brackets before the format's ':'
// a string in an expression's source, s at its start (x_string_at), up to lim: its closing quote; lim when it isn't
// closed before (\" inside, or a verbatim one's "")
internal const char *x_quote_end(const char *s, const char *lim) {
    if (*s == '@') {
        for (s += 2; s < lim; s++) if (*s == '"') { if (s + 1 < lim && s[1] == '"') s++; else return s; }
        return lim;
    }
    char q = *s;
    for (s++; s < lim && *s != q; s++) if (*s == '\\' && s + 1 < lim) s++;
    return s;
}
internal int x_fmt_at(const char *s, int n, int *choice) {   // the format's ':' in s[0..n), -1: none
    int depth = 0, q = 0;
    *choice = 0;
    for (int i = 0; i < n; i++) {
        char ch = s[i];
        if (x_string_at(s + i)) { i = (int)(x_quote_end(s + i, s + n) - s); continue; }   // a string: to its closing quote
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
        // (the language server's record: the placeholder's place in the string's text - x_check XN_STR, tl_value)
        struct xr_ctx xp = { XR_SUB, NULL, 0, NULL, (long)(s + 1 - text), 0, NULL, NULL, NULL };
        if (g_xrc && g_xrc->kind == XR_STR && g_xrc->code == text) xr_push(&xp);
        if ((expr || name) && k) {   // in an expression: the checks around it hold ("{lord}" in lord != null ? ...)
            const struct xunit *u = x_compiled(src);
            if (!u->ok) { *why = u->why ? u->why : "not an expression"; if (g_xrc == &xp) g_xrc = xp.up; return 0; }
            if (g_xrc == &xp) { xp.code = u->src; xp.n = strlen(u->src); }
            g_x_in_ph++;   // (a text's code: a struct's fields are read there, none made, no method called)
            int ok = x_check(k, u->root, sc) && x_ph_ok(k, u->root, sc, src, why);
            g_x_in_ph--;
            if (g_xrc == &xp) g_xrc = xp.up;
            if (!ok) { *why = k->why ? k->why : *why; return 0; }
        }
        else if (expr || name) { int ok = x_syntax_at(src, why, 1); if (g_xrc == &xp) g_xrc = xp.up; if (!ok) return 0; }
        else if (g_xrc == &xp) g_xrc = xp.up;
        s = close;
    }
    return 1;
}
// a {placeholder}'s value: none that may be null - the text would have a hole ("{lord}" said where there is none)
internal int x_ph_ok(struct xcheck *k, const struct xnode *root, struct xscope *sc, const char *src, const char **why) {
    int t = x_stype(k, root, sc);
    if (!(t & X_NULLABLE) || t == X_NULLABLE) return 1;
    snprintf(k->whybuf, sizeof k->whybuf, "{%.60s} may be null (none there) - check it first (... != null ? ...) or give another ({%.60s ?? ...})", src, src);
    k->why = k->whybuf;
    *why = k->whybuf;
    return 0;
}
// the end of a {placeholder} that starts at s ('{'): quotes and nested braces skipped; NULL: none
internal const char *x_close(const char *s) {
    int d = 0;
    for (; *s; s++) {
        if (x_string_at(s)) {
            s = x_quote_end(s, s + strlen(s));
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
    if (x_string_at(src)) {
        if (x_string(src, t, sizeof t, &e, &why) >= 0) { while (*e == ' ') e++; if (!*e) { fn(t, arg); n++; } }
        return n;
    }
    if (*src != '[') return 0;
    for (const char *s = src + 1; *s;) {   // the list's top-level items
        while (*s == ' ' || *s == ',') s++;
        const char *b = s;
        int d = 0;
        for (; *s && (d || (*s != ',' && *s != ']')); s++) {
            if (x_string_at(s)) { if (x_string(s, t, sizeof t, &e, &why) < 0) return n; s = e - 1; }
            else if (*s == '(' || *s == '[' || *s == '{') d++;
            else if (*s == ')' || *s == ']' || *s == '}') d--;
        }
        if (x_string_at(b) && x_string(b, t, sizeof t, &e, &why) >= 0) {
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
        else if (lines && x_comment(l->s)) while (*l->s && *l->s != '\n') l->s++;
        else if (lines && c == '#') { tl_error(l, X_HASH_WHY); while (*l->s && *l->s != '\n') l->s++; }
        else return;
    }
}
internal void tval_free(struct tval *v) { free(v->text); free(v->src); memset(v, 0, sizeof *v); }
internal int tl_key_line(const char *t) {   // a line that starts a key ("name =", "name(a, b) =", "name(a) {", "let", not "a == b")
    while (*t == ' ' || *t == '\t') t++;
    if (!strncmp(t, "let", 3) && (t[3] == ' ' || t[3] == '\t')) return 1;
    const char *k = t;
    while (*t && (isalnum((unsigned char)*t) || strchr("_:.-", *t))) t++;
    if (t == k) return 0;
    if (*t == '(') {   // a function's parameters: names and commas
        for (t++; *t && *t != ')'; t++) if (!isalnum((unsigned char)*t) && !strchr("_, ", *t)) return 0;
        if (*t++ != ')') return 0;
        while (*t == ' ' || *t == '\t') t++;
        if (*t == '{') return 1;
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
        if (!x_string_at(s)) break;
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
    int *pos = g_xr_out && g_xr_text ? malloc(sizeof *pos * cap) : NULL;   // the language server's record: each character's place
    int depth = 0;
    for (;;) {
        char c = *l->s;
        const char *cp = l->s;
        if (!c || (c == '\n' && !depth) || (rec && !depth && (c == ',' || c == '}'))) break;
        if (c == '\n' && tl_key_line(l->s + 1)) { l->line++; l->s++; break; }   // the next key: depth tells
        if (c == '\n' || c == '\r' || c == '\t') { if (c == '\n') l->line++; l->s++; c = ' '; }
        else if (x_comment(l->s)) { while (*l->s && *l->s != '\n') l->s++; continue; }
        else if (c == '#') { tl_error(l, X_HASH_WHY); free(r); free(pos); return; }
        else if (c == ';') { tl_error(l, "; - a comment starts with //"); free(r); free(pos); return; }
        else if (x_string_at(l->s)) {   // a string, copied as written
            const char *e, *why;
            if (x_string(l->s, NULL, 0, &e, &why) < 0) { tl_error(l, why); free(r); free(pos); return; }
            size_t len = (size_t)(e - l->s);
            if (o + len + 2 >= cap) { r = realloc(r, cap = (o + len) * 2 + 2); if (pos) pos = realloc(pos, sizeof *pos * cap); }
            memcpy(r + o, l->s, len);
            for (size_t i = 0; pos && i < len; i++) pos[o + i] = (int)(l->s + i - g_xr_text);
            o += len;
            l->s = e;
            continue;
        } else {
            if (c == '(' || c == '[' || c == '{') depth++;
            if ((c == ')' || c == ']' || c == '}') && --depth < 0) { tl_error(l, "a ) ] or } too many"); free(r); free(pos); return; }
            l->s++;
        }
        if (c == ' ' && (!o || r[o - 1] == ' ')) continue;
        if (o + 2 >= cap) { r = realloc(r, cap *= 2); if (pos) pos = realloc(pos, sizeof *pos * cap); }
        if (pos) pos[o] = (int)(cp - g_xr_text);
        r[o++] = c;
    }
    while (o && r[o - 1] == ' ') o--;
    r[o] = 0;
    if (depth) { int cur = l->line; l->line = v->line; tl_error(l, "a ( [ or { without its end"); l->line = cur; free(r); free(pos); return; }
    if (!o) { tl_error(l, "a value missing"); free(r); free(pos); return; }
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
    struct xr_ctx xp = { XR_POS, NULL, 0, pos, 0, 0, NULL, NULL, NULL }, xs = { XR_STR, NULL, 0, NULL, 0, 0, r, NULL, NULL };
    if (pos) {   // (the language server's record: the value's places in the file - a string's text from its source)
        xr_push(&xp);
        if (v->kind == TV_STRING) { xp.code = r; xp.n = o; xs.code = v->text; xs.n = strlen(v->text); xr_push(&xs); }
    }
    int bad = v->kind == TV_EXPR ? !x_syntax(r, &why) : v->kind == TV_STRING && !x_fills_ok(v->text, &why, NULL, NULL);
    if (pos) g_xrc = xp.up;
    free(pos);
    if (bad) {
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
    if (x_comment(l->s)) while (*l->s && *l->s != '\n') l->s++;
    if (*l->s == '#') { tl_error(l, X_HASH_WHY); return 0; }
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
// DATA: name = value outside the records holds a value that is the same wherever it is used - numbers, strings
// without placeholders, lists and records of them, the game's constants (kind.towers, unit.<id>), other data names,
// vec() and rgb(). Whatever changes with the game or the speaker (me, them, game values, a function's call, a
// placeholder) is worked out where it is used: name => ... - so the two never look alike (let, later, holds state).
// 0: not data, *why the first name or call that makes it so (in buf)
internal int x_data_node(const struct xnode *n, char *buf, int cap) {
    if (!n) return 1;
    switch (n->k) {
    case XN_NUM: break;
    case XN_STR: if (strchr(n->s, '{')) { snprintf(buf, (size_t)cap, "a placeholder in \"%.30s\"", n->s); return 0; } break;
    case XN_NAME: {
        struct xval t;
        char head[48];
        lstrcpynA(head, n->s, (int)(strcspn(n->s, ".") < 47 ? strcspn(n->s, ".") : 47) + 1);
        int fixed = !strncmp(n->s, "unit.", 5) || !strncmp(n->s, "kind.", 5) || !strncmp(n->s, "resource.", 9);   // (unit.<id>: the
        if (!fixed && !g_x_host.constant(n->s, &t) && !x_var_data(n->s) && !x_var_data(head)) {               // game's data, later)
            snprintf(buf, (size_t)cap, "%s", n->s);
            return 0;
        }
        break;
    }
    case XN_CALL:
        if (strcmp(n->s, "vec") && strcmp(n->s, "rgb")) { snprintf(buf, (size_t)cap, "%s(...)", n->s); return 0; }
        break;
    case XN_LIST: case XN_REC: case XN_NEG: case XN_NOT: case XN_MUL: case XN_ADD: case XN_CMP: case XN_AND: case XN_OR:
    case XN_COAL: case XN_COND: case XN_MEMBER: case XN_INDEX: case XN_ASSIGN: break;   // (assign: a record's member)
    default: snprintf(buf, (size_t)cap, "a function or statement"); return 0;
    }
    if (!x_data_node(n->a, buf, cap) || !x_data_node(n->b, buf, cap) || !x_data_node(n->c, buf, cap)) return 0;
    for (int i = 0; i < n->n_items; i++) if (!x_data_node(n->items[i], buf, cap)) return 0;
    return 1;
}
typedef void (*tfile_fn)(void *arg, const char *record, const char *key, struct tval *v, struct tlex *l);
internal int x_syntax(const char *src, const char **why);
// name(a, b) { ... } at a file's top: a function of statements - read as written (one statement a line), kept as
// X_BODY "(a,b) => { ... }" (x_call runs it as a script's function), checked as one; key "name(a,b)", record ""
internal void tl_body(struct tlex *l, const char *key, tfile_fn fn, void *arg, const char *file) {
    int line = l->line, depth = 0;
    const char *b = l->s;
    for (;;) {
        char c = *l->s;
        if (!c) { l->line = line; tl_error(l, "a { without its }"); return; }
        if (x_string_at(l->s)) {
            const char *e, *why;
            if (x_string(l->s, NULL, 0, &e, &why) < 0) { tl_error(l, why); return; }
            l->s = e;
            continue;
        }
        if (x_comment(l->s)) { while (*l->s && *l->s != '\n') l->s++; continue; }
        if (c == '\n') l->line++;
        else if (c == '{') depth++;
        else if (c == '}' && !--depth) { l->s++; break; }
        l->s++;
    }
    if (!tl_line_end(l, "something after the function's }")) return;
    const char *par = strchr(key, '(');
    size_t pn = strlen(par), bn = (size_t)(l->s - b);
    struct tval v;
    memset(&v, 0, sizeof v);
    v.line = line;
    v.kind = TV_EXPR;
    v.src = malloc(pn + bn + 8);
    if (!v.src) return;
    v.src[0] = X_BODY;
    memcpy(v.src + 1, par, pn);
    memcpy(v.src + 1 + pn, " => ", 4);
    memcpy(v.src + 5 + pn, b, bn);
    v.src[5 + pn + bn] = 0;
    const char *why, *head = b;
    while (g_xr_text && head > g_xr_text && head[-1] != '\n') head--;
    // (the language server's record: the body as written, its head the key's line)
    struct xr_ctx xb = { XR_FILE, NULL, 0, NULL, (long)(b - g_xr_text) - (long)(pn + 4), (long)(pn + 4), NULL, head, NULL };
    if (g_xr_out && g_xr_text) xr_push(&xb);
    g_x_check_file = file;
    g_x_check_line = line;
    int ok = x_syntax(v.src + 1, &why);
    g_x_check_file = NULL;
    if (g_xrc == &xb) g_xrc = xb.up;
    if (!ok) {
        const struct xunit *u = x_compiled(v.src + 1);
        int cur = l->line;
        l->line = line + (!u->ok && u->line > 1 ? u->line - 1 : 0);
        tl_error(l, why);
        l->line = cur;
    } else fn(arg, "", key, &v, l);
    tval_free(&v);
}
// let name = value, let name: type = value at a file's top: a field - one per AI (the host's: persona.c PERSONALITY
// STATE), starting as a number, a bool, a text or a dict<K, V>(default) - data (x_data_node) - or as null, its type an
// object or nothing (let rival: player? = null). Record "let", key the name, ":type" after it when written
internal int x_data_node(const struct xnode *n, char *buf, int cap);
internal void tl_let(struct tlex *l, tfile_fn fn, void *arg, const char *file) {
    char name[40], type[48] = "", why[200], what[64];
    int k = 0;
    tl_blank(l, 0);
    while ((isalnum((unsigned char)*l->s) || *l->s == '_') && k < 39) name[k++] = *l->s++;
    name[k] = 0;
    if (!k || isdigit((unsigned char)name[0]) || x_word_reserved(name)) { tl_error(l, "let name = value"); return; }
    tl_blank(l, 0);
    if (*l->s == ':') {   // its type as written: let n: float = 0, let g: dict<player, int> = ...
        int t = 0;
        for (l->s++; *l->s == ' ' || *l->s == '\t'; l->s++) {}
        while (*l->s && *l->s != '=' && *l->s != '\n' && t < 47) type[t++] = *l->s++;
        while (t && type[t - 1] == ' ') t--;
        type[t] = 0;
    }
    if (*l->s != '=' || l->s[1] == '=' || l->s[1] == '>') { tl_error(l, "let name = value (a field: data, or dict<K, V>(default))"); return; }
    l->s++;
    struct tval v;
    g_x_check_file = file;
    g_x_check_line = l->line;
    tl_value(l, &v, 0);
    g_x_check_file = NULL;
    if (l->err) return;
    const struct xunit *u = x_compiled(v.src);
    const struct xnode *r = u->ok ? u->root : NULL, *num = r && r->k == XN_NEG ? r->a : r;
    int dict = r && r->k == XN_DICT;
    int vt = dict ? -1 : num && num->k == XN_NUM && num->op != 'n' ? (num->op == 'b' ? XT_BOOL : num->op == 'f' ? XT_FLOAT : XT_INT) :
             r && r->k == XN_STR && !strchr(r->s, '{') ? XT_TEXT : 0;
    why[0] = 0;
    if (xn_null(r)) {   // let name: K? = null - one of the host's objects or none (which kinds it keeps: the host's)
        int t = type[0] ? x_type_named(type) : -1;
        if (t < 0 || !(t & X_NULLABLE) || (t & X_LIST_OF) || X_TBASE(t) >= XT_INT)
            snprintf(why, sizeof why, "let %s = null: a field that starts as nothing is an object or nothing - let %s: player? = null", name, name);
    } else if (type[0] && x_type_named(type) >= 0 && X_TBASE(x_type_named(type)) < XT_INT) {
        snprintf(why, sizeof why, "let %s: %s = ...: a field of an object starts as null, a handler sets it (%s = them)", name, type, name);
    } else if (r && !vt) {
        if (!x_data_node(dict ? r->a : r, what, sizeof what)) snprintf(what + strlen(what), sizeof what - strlen(what), " changes with the game");
        else snprintf(what, sizeof what, "%.40s", v.src);
        snprintf(why, sizeof why, "let %s = ...: a field starts as a number, a bool, a text or a dict<K, V>(default) - not %s", name, what);
    } else if (dict && !x_data_node(r->a, what, sizeof what)) {
        snprintf(why, sizeof why, "let %s = dict<K, V>(...): its default changes with the game (%s)", name, what);
    } else if (dict && !xt_plain_val(xt_dict_val((int)r->n))) {
        snprintf(why, sizeof why, "let %s = dict<K, V>(...): a field's dict holds an int, a float, a bool or a text", name);
    } else if (type[0]) {
        int t = dict ? 0 : x_type_named(type);
        if (dict ? strncmp(type, "dict<", 5) != 0 : !(t == vt || (t == XT_FLOAT && vt == XT_INT)))
            snprintf(why, sizeof why, "let %s: %s = ...: its value is %s", name, type, dict ? "a dict" : vt == XT_TEXT ? "a text" : vt == XT_BOOL ? "a bool" : "a number");
    }
    if (why[0]) {
        int cur = l->line;
        l->line = v.line;
        tl_error(l, why);
        l->line = cur;
    } else {
        char key[96];
        snprintf(key, sizeof key, "%s%s%s", name, type[0] && !dict ? ":" : "", dict ? "" : type);
        fn(arg, "let", key, &v, l);
        l->err = 0;
    }
    tval_free(&v);
}
#define TL_DEPTH 8
// (path: where file - its name in the messages - is; the language server's record says it, NULL the name is the path)
internal void tfile_parse_at(const char *path, const char *file, const char *text, tfile_fn fn, void *arg) {
    struct tlex l = { text, file, 1, 0 };
    xr_file(path ? path : file, text, 0);   // (the language server's record: the code page's text)
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
            tl_error(&l, "a [section]: the files have records now - lines = { greeting => ... }");
        } else if (*l.s == ';') {
            tl_error(&l, "; - a comment starts with //");
        } else {
            char key[160];
            int n = 0;
            while (*l.s && (isalnum((unsigned char)*l.s) || *l.s == '_') && n < 47) key[n++] = *l.s++;
            key[n] = 0;
            if (n == 3 && !strcmp(key, "let") && (*l.s == ' ' || *l.s == '\t')) {   // let name = value: a field
                if (depth) tl_error(&l, "let at the file's top only - a record's entries are values");
                else tl_let(&l, fn, arg, file);
                goto next;
            }
            if (n && *l.s == '(' && !depth && tl_declaration(key)) {   // include("a.eeai", ...): ["a.eeai", ...]
                struct tval v;
                memset(&v, 0, sizeof v);
                v.line = l.line;
                v.kind = TV_EXPR;
                const char *b = ++l.s, *e, *why;
                while (*l.s && *l.s != ')' && *l.s != '\n')
                    if (x_string_at(l.s)) { if (x_string(l.s, NULL, 0, &e, &why) < 0) break; l.s = e; }
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
                    char look[27];   // ",a,": a whole name among the earlier ones, "(a,b," - (count, t) has no t twice
                    const char *par = key + strcspn(key, "(");
                    wsprintfA(look, ",%s,", pnm);
                    if (!strncmp(par + 1, look + 1, (size_t)k + 1) || strstr(par, look) || ++np > 8) { why = np > 8 ? "at most 8 parameters" : "a parameter twice"; break; }
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
            int arrow = n > 0 && l.s[0] == '=' && l.s[1] == '>', isfn = n > 0 && strchr(key, '(') != NULL;
            // a record's entry: = or => as the program reads its key (host.h entry_arrow) - a record of the file's own
            // (-1) as outside the records: = data, => worked out where used
            int rule = depth && n > 0 && !isfn ? g_x_host.entry_arrow(file, record, key) : -1;
            if (n < 0) ;   // (its mistake told)
            else if (isfn && *l.s == '{') {   // name(a, b) { ... }: a function of statements
                if (depth) tl_error(&l, "a function in a record - at the file's top: name(a) { ... }");
                else tl_body(&l, key, fn, arg, file);
            } else if (!n || *l.s != '=') tl_error(&l, "no name = value (or name => ...)");
            else if (isfn && !arrow) {
                char why[120];
                snprintf(why, sizeof why, "a function is written with =>: %.*s => ...", (int)(strchr(key, ')') - key) + 1, key);
                tl_error(&l, why);
            } else if (!arrow && l.s[1 + strspn(l.s + 1, " \t")] == '{') {   // name = { - a record: its entries up to its }
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
            } else {   // name = value (data, outside the records), name => ... / name(a, b) => ...: worked out where used
                l.s += arrow ? 2 : 1;
                struct tval v;
                g_x_check_par = strchr(key, '(') ? strchr(key, '(') + 1 : NULL;
                g_x_check_file = file;
                g_x_check_line = l.line;
                g_x_check_ph = g_x_host.placeholders(file, record, key);
                g_x_check_has = g_x_host.has(file, record, key);
                tl_value(&l, &v, depth > 0);
                g_x_check_has = NULL;
                g_x_check_par = NULL;
                g_x_check_file = NULL;
                int told = 0, cur = l.line;   // = or => the wrong one: told once the value is read whole, the reader on after it
                char why[200];
                l.line = v.line;
                if (!l.err && rule >= 0 && rule != arrow) {
                    if (rule) snprintf(why, sizeof why, "%s.%s is worked out each time it is used: %s => ...", record, key, key);
                    else snprintf(why, sizeof why, "%s.%s is read once, a value: %s = ...", record, key, key);
                    tl_error(&l, why);
                    told = 1;
                } else if (!l.err && !arrow && rule < 0) {   // data: the same wherever it is used (x_data_node)
                    const struct xunit *u = x_compiled(v.src);
                    char what[64];
                    if (u->ok && !x_data_node(u->root, what, sizeof what)) {
                        snprintf(why, sizeof why, "%s = ... changes with the game (%s) - a name worked out where it is used: %s => ...",
                                 key, what, key);
                        tl_error(&l, why);
                        told = 1;
                    }
                }
                l.line = cur;
                if (told) l.err = 0;
                else if (!l.err) {
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

// ===== SCRIPTS COMPILED: a script's tree made into instructions and run by a loop of its own (the VM) - its whole state
// plain data: where each function it is in stands (a number into the instructions), their variables (slots of one stack),
// the half-worked-out values above them. A wait is that loop stopping where it is, anywhere in an expression too; no
// fiber, nothing of the program's own stack belongs to a script. What an instruction does with its operands is THE
// OPERATIONS, x_call_apply, x_member_of, x_fill (the code the tree's walk had before the machine: one meaning). Variables are slots
// read by their numbers; where the language looks a name up by its letters it finds them through THE VIEW.
//   the compiler: xc_compile - a function per lambda (the script's top is function 0), its names made slots: its
//     parameters, every let and for of its body (known before the body is compiled: a function made earlier may use a
//     let further down), hidden ones for a loop's own state. A name of a function around is (depth, slot).
//   the machine: xvm_run - until a wait (XVM_YIELD), the script's end (XVM_DONE), a mistake no list caught (XVM_FAIL),
//     or, called for one function by the program (a trigger's, a {placeholder}'s: xvm_call), that function's return.
// THE TEXTS (personalities, prompts, settings) are compiled the same way, each source on its own when it is first
// worked out (x_unit_code: a value, its top function returns it), and run on one machine of their own (xvm_eval), a
// value at a time within each other - nothing waits there and nothing of it is saved. A text's names are mostly looked
// up by their letters (XI_NAME: a placeholder, a vars entry, a vars function's parameter, the host's values).
enum { XI_STMT, XI_CONST, XI_NIL, XI_LOADL, XI_LOADU, XI_STOREL, XI_STOREU, XI_NAME, XI_STR, XI_USE, XI_POP, XI_NEG, XI_NOT,
       XI_ARITH, XI_CMP, XI_JMP, XI_JMPF, XI_AND, XI_OR, XI_COAL, XI_BOOL, XI_MEMBER, XI_INDEX, XI_MCALL, XI_DICT, XI_REC,
       XI_KEY, XI_LIST, XI_TRY, XI_ITEM, XI_LIST_END, XI_CLOSURE, XI_CALLV, XI_CALLN, XI_RET, XI_LOOP, XI_SETI, XI_SETM,
       XI_FORN, XI_FORN_NEXT, XI_FORN_STEP, XI_FORL, XI_FORL_NEXT, XI_FORL_STEP, XI_WAIT, XI_EVERY, XI_EVERY_NEXT,
       XI_EVERY_CALL, XI_EVERY_DONE, XI_HOOK, XI_FAIL, XI_END, XI_SETF, XI_PATH,
       // STRUCTS AND REFS: XI_REF a variable's place (slot a of the function b out; a ref's slot: the place it names),
       // XI_REFM / XI_REFI it a field (a: its name) / an item (its number on top) further, XI_REFGET the value there
       // (the ref stays), XI_REFSET the value on top put there (both taken), XI_DEREF the ref on top made its value,
       // XI_STORER the value on top put where the ref in slot a names, XI_NEW a struct (type a) of the b values
       // below, XI_METHOD a struct's method (a: its name) - the struct's place and b arguments below
       // XI_BOX: the list on top made one item (A LIST AS AN ITEM); XI_ITEM with b 1 does so with a list's item
       // LISTS: XI_MKLIST a new list of the seq (or list, or nothing) on top; XI_HLM a list's method (a: its XHM_, b its
       // arguments, c: the last boxed - a row) with the list below them; XI_REFH an item's place: the list below its number
       XI_REF, XI_REFM, XI_REFI, XI_REFGET, XI_REFSET, XI_DEREF, XI_STORER, XI_NEW, XI_METHOD, XI_BOX, XI_MKLIST, XI_HLM, XI_REFH,
       // XI_JSET: on to a when slot c (a parameter) is set - past its default (a call that gave it)
       XI_JSET,
       XI_COUNT };
struct xins { unsigned char op, b; unsigned short c; int a; };   // what, and its numbers (each instruction's own use)
// a number; a text or a name (cs, n: a name's constant once looked up; mk, mi: a member's name - the kind of object it
// was last read of and its number there + 1, 0 none the host reads by number: x_member_fast; ty: a struct's field or
// method - the type it was last read of, mk -1 / -2, mi its number there + 1: x_field_fast, XI_METHOD)
struct xkonst { double n; const char *s; const char *cs; short mk, mi; const struct xstype *ty; };
// slot: its names ("" a hidden one); ref: which of them are refs (the compiler's alone: a ref's slot is read and set
// through its place - NULL in code read from a save, whose instructions say it)
struct xfunc { int entry, nparams, nslots, parent, line; const char **slot; unsigned char *ref; int nopt; };   // nopt: the last
                                                                                                         // parameters with a default
struct xcode {
    struct xins *ins;
    int *line;                // each instruction's line in the source
    int n, cap;
    struct xkonst *k;
    int nk, ck;
    struct xfunc *fn;
    int nfn, cfn;
    const char *src;          // the source (its statements' texts for the messages)
    int text;                 // a text's code (xc_compile_text): the names around it are not its own (a script's, seen
                              // through the view by their letters: "{f(2)}" in a script's string)
    int ok;                   // compiled whole (else why, at why_line)
    const char *why;
    int why_line;
    struct { int fi, at; } *setup;   // a script's game.setup blocks: each one's function and where it starts in src (x_setup_run)
    int nsetup;
    struct xstype *st;        // its structs (STRUCTS AND REFS), each at its type's number
    int nst;
};
#define XC_LOOPS 256
struct xcomp {
    struct xcode *c;
    int fi, line;                       // the function being compiled, the statement's line
    int brk[XC_LOOPS], nbrk;            // jumps of break / continue waiting for their loop's end / next round
    int cont[XC_LOOPS], ncont;
    int in_loop;
    int top_end[XC_LOOPS], ntop_end;    // a return at the script's top: to its end
    int text;                           // a text's source (no script's): wait and every are no instructions there
    int at_top;                         // the statement being compiled is one of the script's top
};
internal void xcc_fail(struct xcomp *cc, const char *why) { if (cc->c->ok) { cc->c->ok = 0; cc->c->why = why; cc->c->why_line = cc->line; } }
internal int xc_emit(struct xcomp *cc, int op, int a, int b, int c) {
    struct xcode *k = cc->c;
    if (k->n == k->cap) {
        int cap = k->cap ? k->cap * 2 : 256;
        struct xins *m = realloc(k->ins, (size_t)cap * sizeof *m);
        int *l = m ? realloc(k->line, (size_t)cap * sizeof *l) : NULL;
        if (m) k->ins = m;
        if (!l) { xcc_fail(cc, "no memory for the script's instructions"); return 0; }
        k->line = l; k->cap = cap;
    }
    if (c < 0 || c > 65535 || b < 0 || b > 255) { xcc_fail(cc, "a script too large for its instructions (65535 names, 255 arguments)"); c = b = 0; }
    k->ins[k->n].op = (unsigned char)op; k->ins[k->n].b = (unsigned char)b; k->ins[k->n].c = (unsigned short)c; k->ins[k->n].a = a;
    k->line[k->n] = cc->line;
    return k->n++;
}
internal void xc_here(struct xcomp *cc, int at) { cc->c->ins[at].a = cc->c->n; }   // a jump's target: the next instruction
internal int xc_konst(struct xcomp *cc, double n, const char *s) {   // a constant's place (a text or name as often as it is written: each keeps its own lookup)
    struct xcode *k = cc->c;
    if (!s) for (int i = 0; i < k->nk; i++) if (!k->k[i].s && k->k[i].n == n) return i;
    if (k->nk == k->ck) {
        int cap = k->ck ? k->ck * 2 : 64;
        struct xkonst *m = realloc(k->k, (size_t)cap * sizeof *m);
        if (!m) { xcc_fail(cc, "no memory for the script's constants"); return 0; }
        k->k = m; k->ck = cap;
    }
    k->k[k->nk].n = n; k->k[k->nk].s = s; k->k[k->nk].cs = NULL; k->k[k->nk].mk = k->k[k->nk].mi = 0; k->k[k->nk].ty = NULL;
    return k->nk++;
}
internal int xc_func(struct xcomp *cc, int parent, int line) {
    struct xcode *k = cc->c;
    if (k->nfn == k->cfn) {
        int cap = k->cfn ? k->cfn * 2 : 16;
        struct xfunc *m = realloc(k->fn, (size_t)cap * sizeof *m);
        if (!m) { xcc_fail(cc, "no memory for the script's functions"); return 0; }
        k->fn = m; k->cfn = cap;
    }
    memset(&k->fn[k->nfn], 0, sizeof *k->fn);
    k->fn[k->nfn].parent = parent; k->fn[k->nfn].line = line;
    return k->nfn++;
}
internal int xc_slot(struct xcomp *cc, int fi, const char *name, int make) {   // a name's slot in function fi, -1 none; make: a new one (2: hidden, always new)
    struct xfunc *f = &cc->c->fn[fi];
    for (int i = 0; make != 2 && i < f->nslots; i++) if (!strcmp(f->slot[i], name)) return i;
    if (!make) return -1;
    const char **m = realloc((void *)f->slot, (size_t)(f->nslots + 1) * sizeof *m);
    unsigned char *rf = realloc(f->ref, (size_t)f->nslots + 1);
    if (rf) f->ref = rf;
    if (!m || !rf || f->nslots >= 250) { if (m) f->slot = m; xcc_fail(cc, "more than 250 names in a function"); return 0; }
    f->slot = m;
    f->slot[f->nslots] = make == 2 ? "" : name;
    f->ref[f->nslots] = 0;
    return f->nslots++;
}
// the slot sl of the function depth out of the one being compiled: a ref's (a place: read and set through it)
internal int xc_isref(struct xcomp *cc, int depth, int sl) {
    int fi = cc->fi;
    while (depth-- > 0 && fi >= 0) fi = cc->c->fn[fi].parent;
    return fi >= 0 && sl >= 0 && sl < cc->c->fn[fi].nslots && cc->c->fn[fi].ref && cc->c->fn[fi].ref[sl];
}
internal const char *xc_seg(const char *s, size_t n) {   // n letters of s as a name of the code's (it stays, as the code does)
    char *t = malloc(n + 1);
    if (!t) return "";
    memcpy(t, s, n);
    t[n] = 0;
    return t;
}
// a name's variable seen from the function being compiled: its slot, *depth functions out (0 its own); -1 no variable
internal int xc_resolve(struct xcomp *cc, const char *name, int *depth) {
    *depth = 0;
    if (strchr(name, '.')) return -1;
    for (int fi = cc->fi; fi >= 0; fi = cc->c->fn[fi].parent, ++*depth) {
        int sl = xc_slot(cc, fi, name, 0);
        if (sl >= 0) return sl;
    }
    return -1;
}
internal void xc_load(struct xcomp *cc, const char *name) {
    int depth, sl = xc_resolve(cc, name, &depth), k = xc_konst(cc, 0, name);
    const char *dot = strchr(name, '.');
    if (sl < 0 && dot && dot - name < 40) {   // w.units, p.gold of a variable: its slot, then the members (XI_PATH)
        char head[40];
        lstrcpynA(head, name, (int)(dot - name) + 1);
        int hs = xc_resolve(cc, head, &depth);
        if (hs >= 0 && xc_isref(cc, depth, hs)) {   // this.x, q.pos.x of a ref: the value at its place, then member by member
            xc_emit(cc, depth ? XI_LOADU : XI_LOADL, hs, depth, xc_konst(cc, 0, xc_seg(head, strlen(head))));
            xc_emit(cc, XI_DEREF, 0, 0, 0);
            for (const char *seg = dot + 1;;) {
                size_t sn = strcspn(seg, ".");
                xc_emit(cc, XI_MEMBER, xc_konst(cc, 0, xc_seg(seg, sn)), 0, 0);
                if (!seg[sn]) break;
                seg += sn + 1;
            }
            return;
        }
        if (hs >= 0) { xc_emit(cc, XI_PATH, k, depth, hs); return; }
    }
    if (sl < 0) xc_emit(cc, XI_NAME, k, 0, 0);
    else if (!depth) xc_emit(cc, XI_LOADL, sl, 0, k);
    else xc_emit(cc, XI_LOADU, sl, depth, k);
    if (sl >= 0 && xc_isref(cc, depth, sl)) xc_emit(cc, XI_DEREF, 0, 0, 0);   // a ref read: the value at its place
}
// the value on top into the name's variable: its own or a function's around - no variable of that name (every let has
// its slot by now: xc_declare): a field of the speaker's personality (the check let it through as one)
internal void xc_store(struct xcomp *cc, const char *name) {
    int depth, sl = xc_resolve(cc, name, &depth), k = xc_konst(cc, 0, name);
    if (sl >= 0 && xc_isref(cc, depth, sl)) {   // a ref set: the value put where it names
        if (depth) xcc_fail(cc, "a ref of the code around a function");
        xc_emit(cc, XI_STORER, sl, 0, 0);
        return;
    }
    if (sl < 0) xc_emit(cc, XI_SETF, k, 0, 0);
    else if (!depth) xc_emit(cc, XI_STOREL, sl, 0, k);
    else xc_emit(cc, XI_STOREU, sl, depth, k);
}
// a block's own names made slots before it is compiled (not a lambda's in it: its own function's)
internal void xc_declare(struct xcomp *cc, const struct xnode *s) {
    if (!s) return;
    if (s->k == XN_BLOCK) { for (int i = 0; i < s->n_items; i++) xc_declare(cc, s->items[i]); return; }
    if (s->k == XN_LET || s->k == XN_FOR) {
        int sl = xc_slot(cc, cc->fi, s->s, 1);
        if (s->k == XN_LET && s->op == 'r' && cc->c->fn[cc->fi].ref && sl < cc->c->fn[cc->fi].nslots) cc->c->fn[cc->fi].ref[sl] = 1;   // ref q = ...
    }
    if (s->k == XN_IF) { xc_declare(cc, s->a); xc_declare(cc, s->b); }
    if (s->k == XN_WHILE) xc_declare(cc, s->a);
    if (s->k == XN_FOR) xc_declare(cc, s->c);
}
internal void xc_expr(struct xcomp *cc, const struct xnode *n);
internal void xc_block(struct xcomp *cc, const struct xnode *blk);
internal int xc_function(struct xcomp *cc, const struct xnode *n) {   // a function's code, out of the way: its number
    int over = xc_emit(cc, XI_JMP, 0, 0, 0), fi = xc_func(cc, cc->fi, n->line), fi0 = cc->fi;
    int nbrk = cc->nbrk, ncont = cc->ncont, in_loop = cc->in_loop;
    cc->fi = fi;
    cc->in_loop = 0;
    for (int i = 0; i < n->n_items; i++) {
        int sl = xc_slot(cc, fi, n->items[i]->s, 1);
        if (n->items[i]->op == 'r' && cc->c->fn[fi].ref && sl < cc->c->fn[fi].nslots) cc->c->fn[fi].ref[sl] = 1;   // ref p: type, a method's this
    }
    cc->c->fn[fi].nparams = n->n_items;
    cc->c->fn[fi].entry = cc->c->n;
    for (int i = 0; i < n->n_items; i++) {   // a parameter's default: its slot still unset (the call left it out) - set
        if (!n->items[i]->a) continue;
        cc->c->fn[fi].nopt++;
        int at = xc_emit(cc, XI_JSET, 0, 0, i);
        xc_expr(cc, n->items[i]->a);
        xc_emit(cc, XI_STOREL, i, 0, xc_konst(cc, 0, n->items[i]->s));
        xc_here(cc, at);
    }
    if (n->a->k == XN_BLOCK) {
        xc_declare(cc, n->a);
        xc_block(cc, n->a);
        xc_emit(cc, XI_NIL, 0, 0, 0);   // (no return: nothing)
    } else xc_expr(cc, n->a);
    xc_emit(cc, XI_RET, 0, 0, 0);
    cc->fi = fi0;
    cc->nbrk = nbrk; cc->ncont = ncont; cc->in_loop = in_loop;
    xc_here(cc, over);
    return fi;
}
internal void xc_lambda(struct xcomp *cc, const struct xnode *n) {   // its function, and the value made of it here
    xc_emit(cc, XI_CLOSURE, xc_function(cc, n), 0, 0);
}
// A PLACE's ref left on the stack (STRUCTS AND REFS; the check saw it is one): its variable, then each field and item
internal void xc_place_path(struct xcomp *cc, const char *s, size_t len) {   // a.b.c, its first len letters
    size_t hn = strcspn(s, ".");
    char head[40];
    int depth;
    if (hn > len) hn = len;
    lstrcpynA(head, s, (int)(hn < 39 ? hn : 39) + 1);
    int sl = xc_resolve(cc, head, &depth);
    if (sl < 0) { xcc_fail(cc, "a place that is no variable's"); return; }
    xc_emit(cc, XI_REF, sl, depth, 0);
    for (size_t at = hn; at < len; ) {
        size_t sn = strcspn(s + at + 1, ".");
        if (at + 1 + sn > len) sn = len - at - 1;
        xc_emit(cc, XI_REFM, xc_konst(cc, 0, xc_seg(s + at + 1, sn)), 0, 0);
        at += 1 + sn;
    }
}
internal void xc_place(struct xcomp *cc, const struct xnode *n) {
    if (n && n->k == XN_NAME) xc_place_path(cc, n->s, strlen(n->s));
    else if (n && n->k == XN_MEMBER) { xc_place(cc, n->a); xc_emit(cc, XI_REFM, xc_konst(cc, 0, n->s), 0, 0); }
    else if (n && n->k == XN_INDEX && (n->sk & 64)) { xc_expr(cc, n->a); xc_emit(cc, XI_USE, 0, 0, 0); xc_expr(cc, n->b); xc_emit(cc, XI_REFH, 0, 0, 0); }   // a list's item
    else if (n && n->k == XN_INDEX) { xc_place(cc, n->a); xc_expr(cc, n->b); xc_emit(cc, XI_REFI, 0, 0, 0); }
    else xcc_fail(cc, "a place that is none");
}
// a type's value when a struct's field is left out (the check: it has one): the field's own (f->a), else its type's
internal void xc_new(struct xcomp *cc, const struct xnode *n, int sti);
internal void xc_default(struct xcomp *cc, int t, const struct xnode *f) {
    if (f && f->a) { xc_expr(cc, f->a); xc_emit(cc, XI_USE, 0, 0, 0); }
    else if (t & (X_NULLABLE | X_LIST_OF)) xc_emit(cc, XI_NIL, 0, 0, 0);
    else if (xt_is_hl(t)) { xc_emit(cc, XI_NIL, 0, 0, 0); xc_emit(cc, XI_MKLIST, 0, 0, 0); }   // a list: a new empty one each
    else if (t == XT_TEXT) xc_emit(cc, XI_STR, xc_konst(cc, 0, ""), 1, 0);
    else if (xst_is(t)) xc_new(cc, NULL, t - XT_STRUCT);
    else xc_emit(cc, XI_CONST, xc_konst(cc, 0, NULL), 0, 0);
}
internal void xc_new(struct xcomp *cc, const struct xnode *n, int sti) {   // point(a, b): its fields' values in order, then the struct of them
    const struct xnode *d = g_xst[sti].decl;
    int nf = 0;
    for (int i = 0; d && i < d->n_items; i++) {
        const struct xnode *f = d->items[i];
        if (f->op != 'F') continue;
        if (n && nf < n->n_items) { xc_expr(cc, n->items[nf]); xc_emit(cc, XI_USE, 0, 0, 0); }
        else xc_default(cc, (int)f->n, f);
        nf++;
    }
    xc_emit(cc, XI_NEW, sti, nf, 0);
}
// { [key] = value, ... } made a dict where one goes (x_check_rec: op 'D'), the dict on top: into a hidden slot while
// its entries are set as d[k] = v is, then on top again - the slot let go of it (it would hold the dict)
internal void xc_dict_fill(struct xcomp *cc, const struct xnode *r) {
    int h = xc_slot(cc, cc->fi, "", 2), ke = xc_konst(cc, 0, "");
    xc_emit(cc, XI_STOREL, h, 0, ke);
    for (int i = 0; i < r->n_items; i++) {
        const struct xnode *e = r->items[i];
        xc_emit(cc, XI_LOADL, h, 0, ke);
        xc_expr(cc, e->b);   // its key (a name is refused: x_check_rec)
        xc_emit(cc, XI_USE, 0, 0, 0);
        xc_expr(cc, e->a);
        xc_emit(cc, XI_USE, 0, 0, 0);
        xc_emit(cc, XI_SETI, 0, 0, 0);
    }
    xc_emit(cc, XI_LOADL, h, 0, ke);
    xc_emit(cc, XI_NIL, 0, 0, 0);
    xc_emit(cc, XI_STOREL, h, 0, ke);
}
// { name = value, ... } made a struct where one goes (op 'S'): its fields in order, each the value given or its
// default (xc_new's) - the values worked out as written: in another order than the fields', into hidden slots first
internal void xc_rec_new(struct xcomp *cc, const struct xnode *r, int sti) {
    const struct xnode *d = g_xst[sti].decl;
    int fi[32], h[32], n = r->n_items < 32 ? r->n_items : 32, ordered = 1, nf = 0, ke = xc_konst(cc, 0, "");
    for (int i = 0; i < n; i++) {
        fi[i] = -1;
        xst_member(XT_STRUCT + sti, r->items[i]->s, 'F', &fi[i]);
        if (i && fi[i] < fi[i - 1]) ordered = 0;
    }
    for (int i = 0; i < n && !ordered; i++) {
        xc_expr(cc, r->items[i]->a);
        xc_emit(cc, XI_USE, 0, 0, 0);
        h[i] = xc_slot(cc, cc->fi, "", 2);
        xc_emit(cc, XI_STOREL, h[i], 0, ke);
    }
    for (int i = 0; d && i < d->n_items; i++) {
        const struct xnode *f = d->items[i];
        int g = -1;
        if (f->op != 'F') continue;
        for (int j = 0; j < n; j++) if (fi[j] == nf) g = j;
        if (g < 0) xc_default(cc, (int)f->n, f);
        else if (ordered) { xc_expr(cc, r->items[g]->a); xc_emit(cc, XI_USE, 0, 0, 0); }
        else xc_emit(cc, XI_LOADL, h[g], 0, ke);
        nf++;
    }
    xc_emit(cc, XI_NEW, sti, nf, 0);
    for (int i = 0; i < n && !ordered; i++) { xc_emit(cc, XI_NIL, 0, 0, 0); xc_emit(cc, XI_STOREL, h[i], 0, ke); }   // (the slots let go of the values)
}
// a struct's method called (n: a.b.name(...) by its name, or <value>.name(...); sk 1: of a place, 2: of a value, which
// goes into a hidden slot to be one): the struct's place, the arguments, the call
internal void xc_method(struct xcomp *cc, const struct xnode *n) {
    const char *dot = n->k == XN_CALL ? strrchr(n->s, '.') : NULL, *mname = dot ? dot + 1 : n->s;
    if ((n->sk & 3) == 1) { if (dot) xc_place_path(cc, n->s, (size_t)(dot - n->s)); else xc_place(cc, n->a); }
    else {
        if (dot) xc_load(cc, xc_seg(n->s, (size_t)(dot - n->s))); else xc_expr(cc, n->a);
        xc_emit(cc, XI_USE, 0, 0, 0);
        int h = xc_slot(cc, cc->fi, "", 2);
        xc_emit(cc, XI_STOREL, h, 0, xc_konst(cc, 0, ""));
        xc_emit(cc, XI_REF, h, 0, 0);
    }
    for (int i = 0; i < n->n_items; i++) xc_expr(cc, n->items[i]);
    xc_emit(cc, XI_METHOD, xc_konst(cc, 0, mname), n->n_items, 0);
}
internal void xc_call(struct xcomp *cc, const struct xnode *n) {
    const char *name = n->s;
    int depth, sl = xc_resolve(cc, name, &depth), k = xc_konst(cc, 0, name), nargs = n->n_items;
    if ((n->sk & 3) && !cc->text) { xc_method(cc, n); return; }   // a struct's method (the check's mark)
    if ((n->sk & 128) && !cc->text) {   // l.add(x): a list's method - the list (its path's value), its arguments
        const char *dot = strrchr(name, '.');
        xc_load(cc, xc_seg(name, (size_t)(dot - name)));
        xc_emit(cc, XI_USE, 0, 0, 0);
        for (int i = 0; i < nargs; i++) { xc_expr(cc, n->items[i]); xc_emit(cc, XI_USE, 0, 0, 0); }
        xc_emit(cc, XI_HLM, xhm_id(dot + 1), nargs, (n->sk & 4) != 0);
        return;
    }
    if (sl < 0 && !cc->text && !strchr(name, '.') && xst_find(name) >= 0 && g_xst[xst_find(name)].decl) { xc_new(cc, n, xst_find(name)); return; }   // point(...)
    if (sl < 0 && !cc->text && !strcmp(name, "every") && nargs == 4) {   // every(step, from, until, f): a loop of its own, its state in five hidden slots
        for (int i = 0; i < 4; i++) xc_expr(cc, n->items[i]);
        int h = xc_slot(cc, cc->fi, "", 2);
        for (int i = 0; i < 4; i++) xc_slot(cc, cc->fi, "", 2);
        xc_emit(cc, XI_EVERY, 0, 0, h);
        int next = xc_emit(cc, XI_EVERY_NEXT, 0, 0, h);
        xc_emit(cc, XI_EVERY_CALL, 0, 0, h);
        int done = xc_emit(cc, XI_EVERY_DONE, 0, 0, h);
        xc_emit(cc, XI_JMP, next, 0, 0);
        xc_here(cc, next); xc_here(cc, done);
        xc_emit(cc, XI_NIL, 0, 0, 0);
        return;
    }
    if (sl < 0 && !cc->text && nargs == 1 && (!strcmp(name, "wait") || !strcmp(name, "wait_until"))) {
        xc_expr(cc, n->items[0]);
        xc_emit(cc, XI_WAIT, 0, name[4] == '_', k);
        return;
    }
    for (int i = 0; i < nargs; i++) xc_expr(cc, n->items[i]);
    if (nargs > 32) {   // (told when it runs, its arguments worked out first)
        char why[80];
        snprintf(why, sizeof why, "%.40s(...): more than 32 arguments", name);
        xc_emit(cc, XI_FAIL, xc_konst(cc, 0, _strdup(why)), 0, 0);
        return;
    }
    if (sl >= 0) {   // a function in a variable: its value now, after the arguments
        xc_load(cc, name);
        xc_emit(cc, XI_CALLV, 0, nargs, k);
    } else xc_emit(cc, XI_CALLN, k, nargs, 0);
}
internal void xc_expr(struct xcomp *cc, const struct xnode *n) {
    int at, at2;
    if (!n) { xc_emit(cc, XI_NIL, 0, 0, 0); return; }
    switch (n->k) {
    case XN_NUM: if (n->op == 'n') xc_emit(cc, XI_NIL, 0, 0, 0); else xc_emit(cc, XI_CONST, xc_konst(cc, n->n, NULL), 0, 0); break;
    case XN_STR:
        if (!strchr(n->s, '{')) {   // no placeholder: the text itself, made once (b 1) - its \{ \} written as x_fill does
            char *t = strpbrk(n->s, "\x01\x02") ? _strdup(n->s) : NULL;
            for (char *q = t; q && *q; q++) if (*q == X_LBRACE) *q = '{'; else if (*q == X_RBRACE) *q = '}';
            xc_emit(cc, XI_STR, xc_konst(cc, 0, t ? t : n->s), 1, 0);
        } else xc_emit(cc, XI_STR, xc_konst(cc, 0, n->s), 0, 0);
        break;
    case XN_NAME: xc_load(cc, n->s); break;
    case XN_CALL: xc_call(cc, n); break;
    case XN_LIST:
        xc_emit(cc, XI_LIST, 0, 0, 0);
        for (int i = 0; i < n->n_items; i++) {   // a mistake in an item leaves it out: caught at its XI_ITEM
            at = xc_emit(cc, XI_TRY, 0, 0, 0);
            xc_expr(cc, n->items[i]);
            xc_emit(cc, XI_USE, 0, 0, 0);
            {   // (the check's marks: a row kept as one item, a list spliced, a null kept)
                int sk = cc->text ? 0 : n->items[i]->sk;
                xc_emit(cc, XI_ITEM, 0, sk & 4 ? 1 : sk & 16 ? 2 : sk & 8 ? 3 : 0, 0);
            }
            xc_here(cc, at);
        }
        xc_emit(cc, XI_LIST_END, 0, 0, 0);
        if ((n->sk & 32) && !cc->text) xc_emit(cc, XI_MKLIST, 0, 0, 0);   // where a list goes: made one
        break;
    case XN_HLIST:   // list<T>(...): a [...] made the list itself, else a new one of what is given
        if (n->a && n->a->k == XN_LIST && (n->a->sk & 32)) { xc_expr(cc, n->a); break; }
        xc_expr(cc, n->a);
        xc_emit(cc, XI_USE, 0, 0, 0);
        xc_emit(cc, XI_MKLIST, 0, 0, 0);
        break;
    case XN_MEMBER:
        xc_expr(cc, n->a);
        xc_emit(cc, XI_USE, 0, 0, 0);
        xc_emit(cc, XI_MEMBER, xc_konst(cc, 0, n->s), 0, 0);
        break;
    case XN_INDEX:
        xc_expr(cc, n->a);
        xc_emit(cc, XI_USE, 0, 0, 0);
        xc_expr(cc, n->b);
        xc_emit(cc, XI_INDEX, 0, 0, 0);
        break;
    case XN_REF: xc_place(cc, n->a); break;   // ref a: the place itself, for a ref parameter
    case XN_MCALL:
        if ((n->sk & 3) && !cc->text) { xc_method(cc, n); break; }   // a struct's method (the check's mark)
        if ((n->sk & 128) && !cc->text) {   // a list's method: the list, its arguments
            xc_expr(cc, n->a);
            xc_emit(cc, XI_USE, 0, 0, 0);
            for (int i = 0; i < n->n_items; i++) { xc_expr(cc, n->items[i]); xc_emit(cc, XI_USE, 0, 0, 0); }
            xc_emit(cc, XI_HLM, xhm_id(n->s), n->n_items, (n->sk & 4) != 0);
            break;
        }
        xc_expr(cc, n->a);
        xc_emit(cc, XI_USE, 0, 0, 0);
        if (n->n_items > 8) {
            char why[80];
            snprintf(why, sizeof why, ".%.40s(...): more than 8 arguments", n->s);
            xc_emit(cc, XI_FAIL, xc_konst(cc, 0, _strdup(why)), 0, 0);
            break;
        }
        for (int i = 0; i < n->n_items; i++) xc_expr(cc, n->items[i]);
        xc_emit(cc, XI_MCALL, xc_konst(cc, 0, n->s), n->n_items, 0);
        break;
    case XN_DICT:
        xc_expr(cc, n->a);
        xc_emit(cc, XI_USE, 0, 0, 0);
        xc_emit(cc, XI_DICT, xt_dict_key((int)n->n) << 16 | xt_dict_val((int)n->n), 0, 0);   // (its K and V: what the machine keeps of the type)
        if (n->b && !cc->text) xc_dict_fill(cc, n->b);   // dict<K, V>(default, { ... })
        break;
    case XN_REC:
        if (n->op == 'D' && !cc->text) {   // made a dict: its V's default, the dict, its entries
            xc_default(cc, xt_dict_val((int)n->n), NULL);
            xc_emit(cc, XI_DICT, xt_dict_key((int)n->n) << 16 | xt_dict_val((int)n->n), 0, 0);
            xc_dict_fill(cc, n);
            break;
        }
        if (n->op == 'S' && !cc->text) { xc_rec_new(cc, n, (int)n->n - XT_STRUCT); break; }   // made a struct
        for (int i = 0; i < n->n_items; i++) { xc_expr(cc, n->items[i]->a); xc_emit(cc, XI_USE, 0, 0, 0); }
        xc_emit(cc, XI_REC, n->n_items, 0, 0);
        for (int i = 0; i < n->n_items; i++) xc_emit(cc, XI_KEY, xc_konst(cc, 0, n->items[i]->s), 0, 0);
        break;
    case XN_LAMBDA: xc_lambda(cc, n); break;
    case XN_NEG: xc_expr(cc, n->a); xc_emit(cc, XI_NEG, 0, 0, 0); break;
    case XN_NOT: xc_expr(cc, n->a); xc_emit(cc, XI_NOT, 0, 0, 0); break;
    case XN_MUL: case XN_ADD:
        xc_expr(cc, n->a);
        xc_expr(cc, n->b);
        xc_emit(cc, XI_ARITH, n->op, n->k == XN_MUL, 0);
        break;
    case XN_CMP:
        xc_expr(cc, n->a);
        xc_expr(cc, n->b);
        xc_emit(cc, XI_CMP, n->op, xn_null(n->a) | xn_null(n->b) << 1, 0);
        break;
    case XN_AND: case XN_OR:
        xc_expr(cc, n->a);
        at = xc_emit(cc, n->k == XN_AND ? XI_AND : XI_OR, 0, 0, 0);
        xc_expr(cc, n->b);
        xc_emit(cc, XI_BOOL, 0, 0, 0);
        xc_here(cc, at);
        break;
    case XN_COAL:
        xc_expr(cc, n->a);
        at = xc_emit(cc, XI_COAL, 0, 0, 0);
        xc_expr(cc, n->b);
        xc_here(cc, at);
        break;
    case XN_COND:
        xc_expr(cc, n->c);
        at = xc_emit(cc, XI_JMPF, 0, 0, 0);
        xc_expr(cc, n->a);
        at2 = xc_emit(cc, XI_JMP, 0, 0, 0);
        xc_here(cc, at);
        if (n->b) xc_expr(cc, n->b); else xc_emit(cc, XI_NIL, 0, 0, 0);
        xc_here(cc, at2);
        break;
    default: xcc_fail(cc, "a statement where a value belongs"); break;
    }
}
internal void xc_loop_end(struct xcomp *cc, int nbrk, int ncont, int next) {   // the loop's breaks to here, its continues to next
    for (int i = ncont; i < cc->ncont; i++) cc->c->ins[cc->cont[i]].a = next;
    for (int i = nbrk; i < cc->nbrk; i++) xc_here(cc, cc->brk[i]);
    cc->nbrk = nbrk; cc->ncont = ncont;
}
internal void xc_stmt(struct xcomp *cc, const struct xnode *s) {
    int at, top, next, nbrk = cc->nbrk, ncont = cc->ncont, in_loop = cc->in_loop, h, at_top = cc->at_top;
    cc->at_top = 0;
    cc->line = s->line;
    xc_emit(cc, XI_STMT, (int)(s->at - cc->c->src), 0, 0);
    switch (s->k) {
    case XN_STRUCT: {   // its methods' code, each a function of the script's top
        if (s->op == 'I') break;   // (an interface: no code - GENERICS)
        struct xstype *st = (int)s->n - XT_STRUCT >= 0 && (int)s->n - XT_STRUCT < cc->c->nst ? &cc->c->st[(int)s->n - XT_STRUCT] : NULL;
        for (int i = 0, j = 0; st && i < s->n_items; i++) {
            if (s->items[i]->op != 'M' || s->items[i]->gen) continue;   // (a generic one as written: its copies have the code)
            cc->line = s->items[i]->line;
            int fi = xc_function(cc, s->items[i]);
            if (j < st->nm) st->mfi[j++] = fi;
        }
        break;
    }
    case XN_LET: case XN_ASSIGN:
        if (s->k == XN_LET && s->op == 'D' && s->a && s->a->gen) break;   // a generic function as written: its copies have the code (GENERICS)
        if (s->k == XN_LET && s->op == 'r') xc_place(cc, s->a);   // ref q = l[2]: the place into its slot
        else xc_expr(cc, s->a);
        if (s->k == XN_LET) { h = xc_slot(cc, cc->fi, s->s, 1); xc_emit(cc, XI_STOREL, h, 0, xc_konst(cc, 0, s->s)); }
        else xc_store(cc, s->s);
        break;
    case XN_RETURN:
        if (cc->fi) { xc_expr(cc, s->a); xc_emit(cc, XI_RET, 0, 0, 0); break; }
        if (s->a) { xc_expr(cc, s->a); xc_emit(cc, XI_POP, 0, 0, 0); }   // at the script's top: its statements end there
        if (cc->ntop_end < XC_LOOPS) cc->top_end[cc->ntop_end++] = xc_emit(cc, XI_JMP, 0, 0, 0);
        break;
    case XN_BREAK: case XN_CONTINUE:
        if (!cc->in_loop) { xcc_fail(cc, "break / continue outside a loop"); break; }
        at = xc_emit(cc, XI_JMP, 0, 0, 0);
        if (s->k == XN_BREAK ? cc->nbrk == XC_LOOPS : cc->ncont == XC_LOOPS) { xcc_fail(cc, "more than 256 breaks waiting for their loops"); break; }
        if (s->k == XN_BREAK) cc->brk[cc->nbrk++] = at; else cc->cont[cc->ncont++] = at;
        break;
    case XN_SETI:
        if (s->sk & 1) {   // l[i] = v, l[i] += v of a list in a place: the item's place, its value now for an op, the value put there
            xc_place(cc, s->a);
            xc_expr(cc, s->b);
            xc_emit(cc, XI_REFI, 0, 0, 0);
            if (s->op) xc_emit(cc, XI_REFGET, 0, 0, 0);
            xc_expr(cc, s->c);
            if (s->op) xc_emit(cc, XI_ARITH, s->op, s->op == '*' || s->op == '/', 0);
            else if (s->sk & 4) xc_emit(cc, XI_BOX, 0, 0, 0);   // (rows[2] = row: the list as an item)
            xc_emit(cc, XI_REFSET, 0, 0, 0);
            break;
        }
        xc_expr(cc, s->a); xc_emit(cc, XI_USE, 0, 0, 0);
        xc_expr(cc, s->b); xc_emit(cc, XI_USE, 0, 0, 0);
        xc_expr(cc, s->c); xc_emit(cc, XI_USE, 0, 0, 0);
        if (s->sk & 4) xc_emit(cc, XI_BOX, 0, 0, 0);   // (a list's row set: the seq as its item)
        xc_emit(cc, XI_SETI, s->op, 0, 0);
        break;
    case XN_SETM:
        if (s->sk) {   // a.x = v, l[2].x -= v of a struct in a place: the field's place, the value put there
            xc_place(cc, s->a);
            xc_emit(cc, XI_REFM, xc_konst(cc, 0, s->s), 0, 0);
            if (s->op) xc_emit(cc, XI_REFGET, 0, 0, 0);
            xc_expr(cc, s->b);
            if (s->op) xc_emit(cc, XI_ARITH, s->op, s->op == '*' || s->op == '/', 0);
            xc_emit(cc, XI_REFSET, 0, 0, 0);
            break;
        }
        xc_expr(cc, s->a); xc_emit(cc, XI_USE, 0, 0, 0);
        xc_expr(cc, s->b);
        xc_emit(cc, XI_SETM, xc_konst(cc, 0, s->s), s->op, 0);
        break;
    case XN_IF: {
        int end[64], nend = 0;
        for (const struct xnode *i = s; i; i = i->b && i->b->k == XN_IF ? i->b : NULL) {   // else if ...: the next if
            xc_expr(cc, i->c);
            at = xc_emit(cc, XI_JMPF, 0, 0, 0);
            xc_block(cc, i->a);
            if (i->b && nend < 64) end[nend++] = xc_emit(cc, XI_JMP, 0, 0, 0);
            else if (i->b) xcc_fail(cc, "more than 64 else if in a row");
            xc_here(cc, at);
            if (i->b && i->b->k != XN_IF) xc_block(cc, i->b);
        }
        for (int i = 0; i < nend; i++) xc_here(cc, end[i]);
        break;
    }
    case XN_WHILE:
        top = cc->c->n;
        xc_expr(cc, s->c);
        at = xc_emit(cc, XI_JMPF, 0, 0, 0);
        cc->in_loop = 1;
        xc_block(cc, s->a);
        cc->line = s->line;
        next = xc_emit(cc, XI_LOOP, top, 0, 0);
        xc_here(cc, at);
        xc_loop_end(cc, nbrk, ncont, next);
        break;
    case XN_FOR: {
        int range = s->b != NULL, k = xc_konst(cc, 0, s->s);
        xc_expr(cc, s->a);
        if (range) xc_expr(cc, s->b);
        h = xc_slot(cc, cc->fi, "", 2);
        xc_slot(cc, cc->fi, "", 2);
        xc_emit(cc, range ? XI_FORN : XI_FORL, k, 0, h);
        top = xc_emit(cc, range ? XI_FORN_NEXT : XI_FORL_NEXT, 0, 0, h);
        xc_emit(cc, XI_STOREL, xc_slot(cc, cc->fi, s->s, 1), 0, k);
        cc->in_loop = 1;
        xc_block(cc, s->c);
        cc->line = s->line;
        next = xc_emit(cc, range ? XI_FORN_STEP : XI_FORL_STEP, 0, 0, h);
        xc_emit(cc, XI_LOOP, top, 0, 0);
        xc_here(cc, top);
        xc_loop_end(cc, nbrk, ncont, next);
        break;
    }
    case XN_BLOCK: xc_block(cc, s); break;
    default:
        if (at_top && x_is_setup(s)) {   // game.setup(() => ...): its function is the next one made (the call's first argument)
            void *m = realloc(cc->c->setup, (size_t)(cc->c->nsetup + 1) * sizeof *cc->c->setup);
            if (!m) { xcc_fail(cc, "no memory for the script's instructions"); break; }
            cc->c->setup = m;
            cc->c->setup[cc->c->nsetup].fi = cc->c->nfn;
            cc->c->setup[cc->c->nsetup++].at = (int)(s->at - cc->c->src);
        }
        xc_expr(cc, s);
        xc_emit(cc, XI_POP, 0, 0, 0);
        break;
    }
    cc->in_loop = in_loop;
}
internal void xc_block(struct xcomp *cc, const struct xnode *blk) {
    if (!blk) return;
    if (blk->k != XN_BLOCK) { xc_stmt(cc, blk); return; }
    for (int i = 0; i < blk->n_items; i++) xc_stmt(cc, blk->items[i]);
}
// a script's tree compiled (src: its text, the tree's; hooks: functions of its top called after it in this order, each
// when the machine's hooks say so - setup.eeai's). The code stays (as the tree does); ok 0: why, why_line
internal struct xcode *xc_compile(const struct xnode *root, const char *src, const char *const *hooks, int nhooks) {
    struct xcode *k = calloc(1, sizeof *k);
    struct xcomp *cc = calloc(1, sizeof *cc);
    if (!k || !cc) { free(k); free(cc); return NULL; }
    k->src = src; k->ok = 1;
    cc->c = k;
    cc->fi = xc_func(cc, -1, 1);
    for (int i = 0; root && root->k == XN_BLOCK && i < root->n_items && !k->st; i++)   // its structs (the script just checked: g_xst)
        if (root->items[i]->k == XN_STRUCT) k->st = calloc((size_t)g_xst_n + 1, sizeof *k->st);
    for (int i = 0; k->st && i < g_xst_n; i++) {
        const struct xnode *d = g_xst[i].decl;
        struct xstype *st = &k->st[i];
        st->name = d ? d->s : "";
        for (int j = 0; d && j < d->n_items; j++) if (d->items[j]->op == 'F') st->nf++; else if (!d->items[j]->gen) st->nm++;
        st->fname = calloc((size_t)st->nf + 1, sizeof *st->fname);
        st->mname = calloc((size_t)st->nm + 1, sizeof *st->mname);
        st->mfi = calloc((size_t)st->nm + 1, sizeof *st->mfi);
        if (!st->fname || !st->mname || !st->mfi) { xcc_fail(cc, "no memory for the script's structs"); st->nf = st->nm = 0; continue; }
        for (int j = 0, a = 0, b = 0; d && j < d->n_items; j++) if (d->items[j]->op == 'F') st->fname[a++] = d->items[j]->s; else if (!d->items[j]->gen) st->mname[b++] = d->items[j]->s;
        k->nst = i + 1;
    }
    xc_declare(cc, root);
    if (root && root->k == XN_BLOCK) for (int i = 0; i < root->n_items; i++) { cc->at_top = 1; xc_stmt(cc, root->items[i]); }
    else xc_block(cc, root);
    for (int i = 0; i < cc->ntop_end; i++) xc_here(cc, cc->top_end[i]);
    for (int i = 0; i < nhooks; i++) {
        int sl = xc_slot(cc, 0, hooks[i], 0), name = xc_konst(cc, 0, hooks[i]);
        if (sl < 0) continue;
        int skip = xc_emit(cc, XI_HOOK, 0, i, name);
        xc_emit(cc, XI_LOADL, sl, 0, name);
        xc_emit(cc, XI_CALLV, 0, 0, name);
        xc_emit(cc, XI_POP, 0, 0, 0);
        xc_here(cc, skip);
    }
    xc_emit(cc, XI_END, 0, 0, 0);
    free(cc);
    return k;
}
// a text's tree compiled (src: the source the tree points into): its top function works the expression out and returns
// it. Kept as small as it is - there are thousands of them (every line, every {placeholder})
internal struct xcode *xc_compile_text(const struct xnode *root, const char *src) {
    struct xcode *k = calloc(1, sizeof *k);
    struct xcomp *cc = calloc(1, sizeof *cc);
    if (!k || !cc) { free(k); free(cc); return NULL; }
    k->src = src; k->ok = 1;
    cc->c = k; cc->text = 1; cc->line = root->line;
    k->text = 1;
    cc->fi = xc_func(cc, -1, 1);
    xc_expr(cc, root);
    xc_emit(cc, XI_RET, 0, 0, 0);
    free(cc);
    void *m;
    if (k->n && (m = realloc(k->ins, (size_t)k->n * sizeof *k->ins)) != NULL) { k->ins = m; k->cap = k->n; }
    if (k->n && k->cap == k->n && (m = realloc(k->line, (size_t)k->n * sizeof *k->line)) != NULL) k->line = m;
    if (k->nk && (m = realloc(k->k, (size_t)k->nk * sizeof *k->k)) != NULL) { k->k = m; k->ck = k->nk; }
    if (k->nfn && (m = realloc(k->fn, (size_t)k->nfn * sizeof *k->fn)) != NULL) { k->fn = m; k->cfn = k->nfn; }
    return k;
}
// a text's code, compiled when it is first asked for (the unit is the cache's: it stays); NULL: no expression
internal struct xcode *x_unit_code(const struct xunit *u) {
    struct xunit *w = (struct xunit *)u;
    if (!u->ok) return NULL;
    if (!w->code) w->code = xc_compile_text(u->root, u->src);
    return w->code && w->code->ok ? w->code : NULL;
}

// THE MACHINE: one script's, or the texts' - the calls in progress (frames: each its function, where its caller goes on, its slots'
// place in the stack, its names as a view), the stack (slots, then the values being worked out), the lists' items
// being tried, where it stands (pc)
#define XVM_STACK 2048
#define XVM_FRAMES (X_CALLS + 48)   // (the texts': a frame for each value worked out within another, X_NEST and more)
#define XVM_TRIES 64
struct xvframe { struct xview view; struct xcode *code; int fn, ret_pc, base; };   // code: its function's (the texts' machine: each source its own)
struct xvtry { int sp, nfr, pc, failed; };
struct xvm {
    struct xcode *code;
    struct xctx c;                       // its context: a script's, its view the running function's
    struct xvframe fr[XVM_FRAMES];
    int nfr;
    struct xval st[XVM_STACK];
    int sp;
    struct xvtry tr[XVM_TRIES];
    int ntr;
    int pc, hooks, line, done, started;
    size_t gc_at;                        // its region's size at which what it dropped is cleared out (xvm_gc; 0: X_GC_STEP)
    struct xregion *reg;                 // its script's region (the runner's; NULL: not told) - what a trigger's function
                                         // sets in the script's top goes there (XI_STOREU)
    int text;                            // the texts' machine (xvm_eval): nothing waits on it
    double wake;                         // the game second it waits for (XVM_YIELD)
};
enum { XVM_DONE, XVM_YIELD, XVM_FAIL, XVM_RET };
internal int xvm_made_in(const struct xval *v, const struct xview *w) {   // a function made in the function of view w, in v or its items
    if (v->kind == XV_FN && v->fn && v->fn->venv == w) return 1;
    for (int i = 0; i < v->ln; i++) if (xvm_made_in(&v->it[i], w)) return 1;
    for (int i = 0; v->kind == XV_REC && v->rec && i < v->rec->n; i++) if (xvm_made_in(&v->rec->val[i], w)) return 1;
    return 0;
}
internal int xvm_has_fn(const struct xval *v) {   // a function, in v or its items
    if (v->kind == XV_FN) return 1;
    for (int i = 0; i < v->ln; i++) if (xvm_has_fn(&v->it[i])) return 1;
    for (int i = 0; v->kind == XV_REC && v->rec && i < v->rec->n; i++) if (xvm_has_fn(&v->rec->val[i])) return 1;
    return 0;
}
internal void xvm_unset(struct xval *v) { xnothing(v); v->kind = XV_UNSET; }
// a function entered: its frame on the arguments (argc values from base on); 0 a mistake told
internal int xvm_enter(struct xvm *m, struct xe *e, const struct xfn *fn, int base, int argc) {
    const struct xfunc *f = &fn->code->fn[fn->fi];
    if (argc > f->nparams || argc < f->nparams - f->nopt) {
        if (f->nopt) x_fail(e, "a function of %d to %d arguments called with %d", f->nparams - f->nopt, f->nparams, argc);
        else x_fail(e, "a function of %d argument%s called with %d", f->nparams, f->nparams == 1 ? "" : "s", argc);
        return 0;
    }
    if (e->c->calls >= X_CALLS || m->nfr >= XVM_FRAMES) { x_fail(e, "functions called inside each other %d deep - one that calls itself without end?", X_CALLS); return 0; }
    if (base + f->nslots + 64 > XVM_STACK) { x_fail(e, "a script's values piled too high (functions called inside each other, long lists)"); return 0; }
    struct xvframe *fr = &m->fr[m->nfr++];
    fr->code = fn->code; fr->fn = fn->fi; fr->ret_pc = m->pc; fr->base = base;
    fr->view.name = f->slot; fr->view.val = &m->st[base]; fr->view.n = f->nslots; fr->view.up = fn->venv; fr->view.keep = 0;
    for (int i = argc; i < f->nslots; i++) xvm_unset(&m->st[base + i]);
    m->sp = base + f->nslots;
    m->pc = f->entry;
    e->c->view = &fr->view;
    e->c->calls++;
    return 1;
}
internal struct xval *xvm_push(struct xvm *m, struct xe *e) {
    if (m->sp < XVM_STACK - 1) return &m->st[m->sp++];
    x_fail(e, "a script's values piled too high (a list of more than %d items?)", XVM_STACK);
    return &m->st[XVM_STACK - 1];
}
// the function value on top called with the argc values below it (name: as written, for the messages): a compiled
// one of this script entered, any other worked out at once
internal void xvm_callv(struct xvm *m, struct xe *e, int argc, const char *name) {
    struct xval fv = m->st[--m->sp], r, *args = &m->st[m->sp - argc];
    int base = m->sp - argc;
    xnothing(&r);
    if (fv.kind == XV_UNSET) {   // its let not reached yet: whatever else has that name
        x_call_apply(e, name, args, argc, 1, &r);
        if (e->c->missed) { e->c->missed = 0; e->err = 1; e->why = "a mistake while working it out"; }
    } else if (fv.kind != XV_FN) x_fail(e, "%s is no function", name);
    else if (fv.fn->vm == m) { xvm_enter(m, e, fv.fn, base, argc); return; }
    else x_call_value(e, &fv, args, argc, &r);
    m->sp = base;
    *xvm_push(m, e) = r;
}
// object o's member read by its number (script/host.h get) - which the instruction's constant k remembers for the
// kind of object it met last (member: its name), so a member of the game's objects is found by its letters once: v
// its value and 1, or 0: not this way (no object's plain value - by its name then, with the mistakes that tells)
internal int x_member_fast(struct xe *e, struct xkonst *k, const char *member, const struct xval *o, struct xval *v) {
    if (!o->obj || !o->w || o->ln) return 0;
    if (k->mk != o->okind) {
        const struct x_kind *kd = g_x_host.kind(o->okind);
        const struct x_kmember *km = x_kind_member(o->okind, member);
        k->mk = o->okind;
        k->mi = kd && km && km->amin < 0 ? (short)(km - kd->m + 1) : 0;
    }
    if (!k->mi) return 0;
    struct xval oc = *o, r;
    xnothing(&r);
    if (!g_x_host.get(e->c, &oc, k->mi - 1, &r)) return 0;
    *v = r;
    if (e->c->missed) { e->c->missed = 0; e->err = 1; e->why = "a mistake while working it out"; }   // told (the host's)
    return 1;
}
// a struct's field read by its number, which the instruction's constant k remembers for the type it met last
// (member: its name): v the field's value and 1, or 0: o is no struct, or has no such field (by its name then: told)
internal int x_field_fast(struct xkonst *k, const char *member, const struct xval *o, struct xval *v) {
    if (o->kind != XV_REC || !o->w || !o->rec->ty) return 0;
    const struct xstype *ty = o->rec->ty;
    if (k->ty != ty || k->mk != -1) { k->ty = ty; k->mk = -1; k->mi = (short)(xst_field(ty, member) + 1); }
    if (!k->mi) return 0;
    struct xval t = o->rec->val[k->mi - 1];   // (v may be o)
    *v = t;
    return 1;
}
// nv put where a ref names (STRUCTS AND REFS): its variable's value made anew with it (xref_build) and stored as a
// set of that variable is - one of a frame that stays, from a trigger's function, into the region that stays (XI_STOREU)
internal void xvm_ref_put(struct xvm *m, struct xe *e, struct xvframe *f, const struct xval *rv, struct xval *nv) {
    const char *why;
    struct xval built;
    x_use(e, nv);
    if (e->err) return;
    if (!xref_peek(rv, &why)) { x_fail(e, "%s", why); return; }
    const struct xref *r = rv->ref;
    struct xview *w = r->w;
    if (r->hl) {   // a list's item: made anew there (a row's place set whole: a row again)
        const struct xval *cur = &r->hl->it[r->slot];
        if (!xref_build(e, cur, r->path, r->np, nv, &built)) return;
        if (!r->np && xb_is(cur) && !xb_is(&built)) xb_box(&built);
        xhl_put(e, r->hl, r->slot, &built, 0);
        return;
    }
    if (!xref_build(e, &w->val[r->slot], r->path, r->np, nv, &built)) return;
    if (w != &f->view && xvm_made_in(&built, &f->view)) { x_fail(e, "a function made inside a function can't be handed out of it"); return; }
    if (w->keep && (g_xt_in || w != &m->fr[0].view) && (built.has_s || built.ln || built.kind || built.obj)) {   // (XI_STOREU)
        if (w == &m->fr[0].view && m->reg && !xvm_has_fn(&built)) {
            struct xregion *r0 = g_xr;
            g_xr = m->reg;
            xv_copy(&w->val[r->slot], &built);
            g_xr = r0;
        } else xt_keep(&w->val[r->slot], &built);
    } else w->val[r->slot] = built;
}
#ifndef X_GC_STEP
#define X_GC_STEP ((size_t)1 << 20)   // (a build may set another: none at all, to compare what a script works out)
#endif
internal void xvm_gc(struct xvm *m);
internal int xvm_run(struct xvm *m, struct xe *e, int until_nfr) {
    struct xctx *c = e->c;
    int ntr0 = m->ntr;
    for (;;) {
        struct xvframe *f = &m->fr[m->nfr - 1];
        struct xcode *code = f->code;
        const struct xins *in = &code->ins[m->pc++];
        struct xkonst *k = &code->k[in->op == XI_LOADL || in->op == XI_LOADU || in->op == XI_STOREL || in->op == XI_STOREU || in->op == XI_CALLV
                                    || in->op == XI_WAIT || in->op == XI_HOOK ? in->c : in->a < code->nk && in->a >= 0 ? in->a : 0];
        struct xval r, *v, *top = &m->st[m->sp ? m->sp - 1 : 0];
        struct xview *w;
        switch (in->op) {
        case XI_STMT: c->top = code->src + in->a; m->line = code->line[m->pc - 1]; break;
        case XI_CONST: xnum(xvm_push(m, e), code->k[in->a].n); break;
        case XI_NIL: xnothing(xvm_push(m, e)); break;
        case XI_LOADL: case XI_LOADU:
            w = &f->view;
            for (int d = in->b; d > 0 && w; d--) w = w->up;
            v = xvm_push(m, e);
            if (!w || in->a >= w->n || w->val[in->a].kind == XV_UNSET) { xnothing(v); xop_name(e, k->s, &k->n, &k->cs, v); }   // not set yet: whatever else has that name
            else *v = w->val[in->a];
            break;
        case XI_STOREL: m->st[f->base + in->a] = m->st[--m->sp]; break;
        case XI_STOREU:
            w = &f->view;
            for (int d = in->b; d > 0 && w; d--) w = w->up;
            v = &m->st[--m->sp];
            if (!w || in->a >= w->n) { x_fail(e, "%s = ...: its frame is gone", k->s); break; }
            if (xvm_made_in(v, &f->view)) { x_fail(e, "%s = ...: a function made inside a function can't be handed out of it", k->s); break; }
            // a frame that stays set from a trigger's function (its values are the tick's): kept - the script's top in the
            // script's own region, which is cleared of what it drops (a timer setting a text every tick must not pile
            // them up for the whole game: the triggers' own region is only freed with the game); a function, or a
            // function's frame a trigger keeps, there - also from the script's own turn (a function over that frame handed
            // to the script: its values would be the script's region's, which the next clearing frees under it)
            if (w->keep && (g_xt_in || w != &m->fr[0].view) && (v->has_s || v->ln || v->kind || v->obj)) {
                if (w == &m->fr[0].view && m->reg && !xvm_has_fn(v)) {
                    struct xregion *r0 = g_xr;
                    g_xr = m->reg;
                    xv_copy(&w->val[in->a], v);
                    g_xr = r0;
                } else xt_keep(&w->val[in->a], v);
            } else w->val[in->a] = *v;
            break;
        case XI_NAME: v = xvm_push(m, e); xnothing(v); xop_name(e, k->s, &k->n, &k->cs, v); break;
        case XI_PATH:   // a member path whose head is the slot c of the function b out: what xe_ident does once it found the head its own
            w = &f->view;
            for (int d = in->b; d > 0 && w; d--) w = w->up;
            v = xvm_push(m, e);
            xnothing(v);
            // (its let not reached yet, a text's parameters or a setting's rules in the way: by its letters, as any name)
            if (!w || in->c >= w->n || w->val[in->c].kind == XV_UNSET || c->fnp || c->fixed) { xop_name(e, k->s, &k->n, &k->cs, v); break; }
            r = w->val[in->c];
            {   // (one member of an object: by its number)
                const char *mem = strchr(k->s, '.') + 1;
                if (!strchr(mem, '.') && (x_member_fast(e, k, mem, &r, v) || x_field_fast(k, mem, &r, v))) break;
            }
            x_path_walk(e, k->s, &r, v);
            if (c->missed) { c->missed = 0; e->err = 1; e->why = "a mistake while working it out"; }   // told (x_missing)
            break;
        case XI_STR:
            v = xvm_push(m, e);
            xnothing(v);
            if (in->b) { v->s = k->s; v->has_s = v->w = 1; }   // (no placeholder: the code's own text, which stays)
            else if (code->text) x_str_fill(e, k->s, v);
            else {   // a script's string: its placeholders may call its structs' methods (x_ph_method)
                struct xvm *m0 = g_x_ph_vm;
                struct xcode *c0 = g_x_ph_code;
                g_x_ph_vm = m; g_x_ph_code = code;
                x_str_fill(e, k->s, v);
                g_x_ph_vm = m0; g_x_ph_code = c0;
            }
            break;
        case XI_USE: x_use(e, top); break;
        case XI_POP: m->sp--; break;
        case XI_NEG: xop_neg(e, top); break;
        case XI_NOT: xnum(top, !xtrue(top)); break;
        case XI_ARITH: {   // (two plain numbers: worked out here - what xop_arith comes to for them)
            struct xval *a = &m->st[--m->sp - 1], *b = a + 1;
            if (XV_IS_NUM(a) && XV_IS_NUM(b) && (in->a != '/' || b->n != 0)) {
                double x = a->n, y = b->n;
                xnum(a, in->b ? in->a == '*' ? x * y : in->a == '/' ? x / y : x - y : in->a == '-' ? x - y : x + y);
            } else xop_arith(e, in->b, in->a, a, b);
            break;
        }
        case XI_CMP: {
            struct xval *a = &m->st[--m->sp - 1], *b = a + 1;
            if (!in->b && XV_IS_NUM(a) && XV_IS_NUM(b)) {
                double x = a->n, y = b->n;
                int op = in->a;
                xnum(a, op == '<' ? x < y : op == '>' ? x > y : op == 'l' ? x <= y : op == 'g' ? x >= y : op == '=' ? x == y : x != y);
            } else xop_cmp(e, in->a, in->b & 1, in->b >> 1, a, b);
            break;
        }
        case XI_JMP: m->pc = in->a; break;
        case XI_JSET: if (m->st[f->base + in->c].kind != XV_UNSET) m->pc = in->a; break;
        case XI_JMPF: if (!xtrue(&m->st[--m->sp])) m->pc = in->a; break;
        case XI_AND: case XI_OR: {
            int t = xtrue(top);
            if (in->op == XI_AND ? !t : t) { xnum(top, t); m->pc = in->a; }   // the right side not worked out
            else m->sp--;
            break;
        }
        case XI_BOOL: xnum(top, xtrue(top)); break;
        case XI_COAL: if (top->w > 0) m->pc = in->a; else m->sp--; break;
        case XI_MEMBER: if (!x_member_fast(e, k, k->s, top, top) && !x_field_fast(k, k->s, top, top)) x_member_of(e, k->s, top, NULL); break;
        case XI_INDEX:
            xnothing(&r);
            xop_index(e, &m->st[m->sp - 2], &m->st[m->sp - 1], &r);
            m->sp--;
            m->st[m->sp - 1] = r;
            break;
        case XI_MCALL:
            xnothing(&r);
            xop_mcall(e, k->s, &m->st[m->sp - in->b - 1], &m->st[m->sp - in->b], in->b, &r);
            m->sp -= in->b;
            m->st[m->sp - 1] = r;
            break;
        case XI_DICT: r = *top; xnothing(top); xop_dict(e, in->a, &r, top); break;
        case XI_REC: {   // its members' values below, their names in the XI_KEYs after
            int n = in->a;
            struct xrec *rec = x_alloc(sizeof *rec);
            const char **key = x_alloc((size_t)n * sizeof *key + 1);
            struct xval *val = x_alloc((size_t)n * sizeof *val + 1);
            if (!rec || !key || !val) { e->err = 1; break; }
            rec->n = n; rec->key = key; rec->val = val; rec->ty = NULL;
            for (int i = 0; i < n; i++) { key[i] = code->k[code->ins[m->pc + i].a].s; val[i] = m->st[m->sp - n + i]; }
            m->pc += n;
            m->sp -= n;
            v = xvm_push(m, e);
            xnothing(v);
            v->w = 1; v->kind = XV_REC; v->rec = rec;
            break;
        }
        case XI_LIST:
            if (c->fixed) { x_fail(e, "a list - a setting is one number"); break; }
            xvm_unset(xvm_push(m, e));   // where its items begin
            break;
        case XI_TRY:
            if (m->ntr == XVM_TRIES) { x_fail(e, "lists inside each other more than %d deep", XVM_TRIES); break; }
            m->tr[m->ntr].sp = m->sp; m->tr[m->ntr].nfr = m->nfr; m->tr[m->ntr].pc = in->a; m->tr[m->ntr].failed = c->failed;
            m->ntr++;
            break;
        case XI_ITEM:
            if (m->ntr > ntr0) m->ntr--;
            if (in->b == 2) {   // a list's items spliced in: as a seq of them (copied out), none left out
                struct xhlist *h = top->kind == XV_HLIST && top->w ? top->hl : NULL;
                struct xval *it = h && h->n ? x_alloc((size_t)h->n * sizeof *it) : NULL;
                for (int j = 0; it && j < h->n; j++) { xvm_copy(&it[j], &h->it[j]); }
                xnothing(top);
                if (it) { top->it = it; top->ln = top->w = h->n; } else m->sp--;
            } else if (in->b == 3) {}   // kept, null too (a list<T?>'s)
            else if (in->b) xb_box(top);   // a list that is one item (a row of a list<list<T>>; an empty one too - none and empty are not told apart there)
            else if (!top->w) m->sp--;   // nothing: left out
            break;
        case XI_BOX: xb_box(top); break;   // the list on top made an item (l[2] = row)
        case XI_MKLIST: if (c->fixed || m->text) { x_fail(e, "a list - a script's"); break; } r = *top; xhl_make(e, &r, top); break;
        case XI_HLM: {   // a list's method: the list below the b arguments
            int base = m->sp - in->b - 1;
            if (base < 0) { x_fail(e, "the script's instructions are broken (a list)"); break; }
            if (in->c && in->b) xb_box(&m->st[m->sp - 1]);
            xhl_method(e, in->a, &m->st[base], &m->st[base + 1], in->b, &r);
            m->sp = base;
            *xvm_push(m, e) = r;
            break;
        }
        case XI_REFH: {   // a list's item as a place: the list below its number
            struct xval ix = m->st[--m->sp];
            v = &m->st[m->sp - 1];
            struct xhlist *h = v->kind == XV_HLIST && v->w ? v->hl : NULL;
            int i = (int)ix.n;
            if (!h) { x_fail(e, "[...]: %s - no list", v->w ? xtext_of(v) : "null"); break; }
            if (!x_numeric_ok(e, &ix)) break;
            if (!ix.w || (double)i != ix.n || i < 1 || i > h->n) { x_fail(e, "[%s]: no such item - the list has %d (the first is [1])", xtext_of(&ix), h->n); break; }
            struct xref *q = x_alloc(sizeof *q);
            if (!q) { e->err = 1; break; }
            q->w = NULL; q->hl = h; q->slot = i - 1; q->np = 0;
            xnothing(v);
            v->w = 1; v->kind = XV_REF; v->ref = q;
            break;
        }
        case XI_LIST_END: {   // the items above the mark: a list in it its items in its place; none left: nothing
            int mk = m->sp - 1, cnt = 0;
            while (mk > 0 && m->st[mk].kind != XV_UNSET) mk--;
            for (int i = mk + 1; i < m->sp; i++) cnt += m->st[i].ln ? m->st[i].ln : 1;
            struct xval *keep = cnt ? x_alloc((size_t)cnt * sizeof *keep) : NULL;
            if (cnt && !keep) { x_fail(e, "no memory for a list"); break; }
            for (int i = mk + 1, o = 0; i < m->sp; i++) {
                const struct xval *it = &m->st[i];
                for (int j = 0; j < (it->ln ? it->ln : 1); j++) keep[o++] = it->ln ? it->it[j] : *it;
            }
            m->sp = mk + 1;
            v = &m->st[mk];
            xnothing(v);
            if (cnt) { v->it = keep; v->ln = v->w = cnt; }
            break;
        }
        case XI_CLOSURE: {
            struct xfn *fn = x_alloc(sizeof *fn);
            if (!fn) { e->err = 1; break; }
            memset(fn, 0, sizeof *fn);
            fn->code = code; fn->fi = in->a; fn->vm = m;
            fn->venv = f->view.n || f->view.up ? &f->view : NULL;   // (a text's top has no names: nothing of it to see, or to outlive)
            v = xvm_push(m, e);
            xnothing(v);
            v->w = 1; v->kind = XV_FN; v->fn = fn;
            break;
        }
        case XI_CALLV: xvm_callv(m, e, in->b, k->s); break;
        case XI_CALLN:
            xnothing(&r);
            x_call_apply(e, k->s, &m->st[m->sp - in->b], in->b, code->text, &r);
            if (c->missed) { c->missed = 0; e->err = 1; e->why = "a mistake while working it out"; }
            m->sp -= in->b;
            *xvm_push(m, e) = r;
            break;
        case XI_RET: {
            r = m->st[--m->sp];
            if (xvm_made_in(&r, &f->view)) { x_fail(e, "a function made inside a function can't be handed out of it"); break; }
            m->sp = f->base;
            m->st[m->sp++] = r;
            m->pc = f->ret_pc;
            m->nfr--;
            c->calls--;
            c->view = m->nfr ? &m->fr[m->nfr - 1].view : NULL;
            if (m->nfr == until_nfr) return XVM_RET;
            break;
        }
        case XI_LOOP:   // a loop's next round
            if (++e->steps > X_LOOP_STEPS) { x_fail(e, "a loop (line %d) ran %d rounds without waiting - wait(...) lets the game go on", code->line[m->pc - 1], X_LOOP_STEPS); break; }
            m->pc = in->a;
            // what the rounds so far made and dropped, out of the script's region - in its own turn only (until_nfr 0,
            // its region the one in use): there every value it holds is on its stack, between two statements - also
            // when it made many lists (they are apart from the region: a loop of list<T>() hardly grows it)
            if ((g_xr->total > (m->gc_at ? m->gc_at : X_GC_STEP) || g_xhl_made > 20000) && !until_nfr && !m->text && !g_xt_in && g_xr->busy == 1) xvm_gc(m);
            break;
        case XI_SETI: m->sp -= 3; xop_seti(e, in->a, &m->st[m->sp], &m->st[m->sp + 1], &m->st[m->sp + 2]); break;
        case XI_SETM: m->sp -= 2; xop_setm(e, k->s, in->b, &m->st[m->sp], &m->st[m->sp + 1]); break;
        case XI_FORN: {   // for name in a..b: a and b below; its number and end into the hidden slots
            struct xval *a = &m->st[m->sp - 2], *b = &m->st[m->sp - 1];
            m->sp -= 2;
            if (!x_numeric_ok(e, a) || !x_numeric_ok(e, b)) break;
            if (!a->w || !b->w) { x_fail(e, "for %s in a..b: a number where nothing is", k->s); break; }
            xnum(&m->st[f->base + in->c], a->n);
            xnum(&m->st[f->base + in->c + 1], b->n);
            break;
        }
        case XI_FORN_NEXT:
            if (!(m->st[f->base + in->c].n <= m->st[f->base + in->c + 1].n + 1e-9)) m->pc = in->a;
            else xnum(xvm_push(m, e), m->st[f->base + in->c].n);
            break;
        case XI_FORN_STEP: m->st[f->base + in->c].n += 1; break;
        case XI_FORL: {   // for name in l: the list below, into the hidden slot, its place after it
            struct xval *a = &m->st[--m->sp];
            if (a->kind && a->kind != XV_HLIST) { x_fail(e, "for %s in ...: %s - a list, or a..b", k->s, a->kind == XV_FN ? "a function" : a->kind == XV_REC ? "a record" : a->kind == XV_VEC ? "a vector" : "stop"); break; }
            m->st[f->base + in->c] = *a;
            xnum(&m->st[f->base + in->c + 1], 0);
            break;
        }
        case XI_FORL_NEXT: {
            const struct xval *l = &m->st[f->base + in->c];
            int i = (int)m->st[f->base + in->c + 1].n;
            if (l->kind == XV_HLIST) {   // a list: its items as they are each round (copied out)
                if (i >= l->hl->n) m->pc = in->a;
                else { v = xvm_push(m, e); xhl_out(l->hl, i, v); }
            } else if (i >= xl_n(l)) m->pc = in->a;
            else { v = xvm_push(m, e); *v = *xl_at(l, i); xb_unbox(v); }
            break;
        }
        case XI_FORL_STEP: m->st[f->base + in->c + 1].n += 1; break;
        case XI_WAIT: {   // wait(seconds) / wait_until(time) (b): the machine stops here until then
            struct xval a = m->st[--m->sp];
            double now = g_x_hooks.now ? g_x_hooks.now() : 0;
            xnothing(xvm_push(m, e));
            if (!x_numeric_ok(e, &a) || !a.w) { if (!e->err) x_fail(e, "%s(...): a number where nothing is", k->s); break; }
            if (in->b && !(a.n > now)) break;   // (past: no wait)
            if (g_xt_in) { x_fail(e, "a trigger's run can't wait - after(5s, () => ...) does it later"); break; }
            if (until_nfr > 0 || m->text || !g_x_hooks.now) { x_fail(e, "waiting only in a script"); break; }
            m->wake = in->b ? a.n : now + a.n;
            e->steps = 0;
            return XVM_YIELD;
        }
        case XI_EVERY: {   // every(step, from, until, f): the four below into the hidden slots, its run 1
            struct xval *a = &m->st[m->sp - 4];
            m->sp -= 4;
            for (int i = 0; i < 4 && !e->err; i++) {
                if (i == 3) { if (a[i].kind != XV_FN) x_fail(e, "every(...): its last argument is a function - (i) => ..."); }
                else if (!x_numeric_ok(e, &a[i]) || !a[i].w) { if (!e->err) x_fail(e, "every(...): a number where nothing is"); }
            }
            if (e->err) break;
            if (!(a[0].n > 0)) { x_fail(e, "every(%g, ...): the step must be above 0", a[0].n); break; }
            for (int i = 0; i < 4; i++) m->st[f->base + in->c + i] = a[i];
            xnum(&m->st[f->base + in->c + 4], 1);
            break;
        }
        case XI_EVERY_NEXT: {   // this run's time: past the end - out; still to come - the machine stops until then
            const struct xval *h = &m->st[f->base + in->c];
            double t = h[1].n + (h[4].n - 1) * h[0].n, now = g_x_hooks.now ? g_x_hooks.now() : 0;
            if (t > h[2].n + 1e-9) { m->pc = in->a; break; }
            if (!(t > now)) break;
            if (g_xt_in) { x_fail(e, "a trigger's run can't wait - after(5s, () => ...) does it later"); break; }
            if (until_nfr > 0 || m->text || !g_x_hooks.now) { x_fail(e, "waiting only in a script"); break; }
            m->wake = t;
            e->steps = 0;
            return XVM_YIELD;
        }
        case XI_EVERY_CALL:   // f(run)
            xnum(xvm_push(m, e), m->st[f->base + in->c + 4].n);
            *xvm_push(m, e) = m->st[f->base + in->c + 3];
            if (!e->err) xvm_callv(m, e, 1, "every's function");
            break;
        case XI_EVERY_DONE:   // its value: stop ends it, else the next run
            if (m->st[--m->sp].kind == XV_STOP) m->pc = in->a;
            else m->st[f->base + in->c + 4].n += 1;
            break;
        case XI_HOOK:   // a hook of the script's top: called when the machine's hooks say so
            if (!((m->hooks >> in->b) & 1)) m->pc = in->a;
            else if (g_x_hooks.note) g_x_hooks.note(k->s);
            break;
        case XI_FAIL: x_fail(e, "%s", k->s); break;
        case XI_SETF: {   // name = value where no variable has the name: a field of the speaker's personality
            const char *why = NULL;
            v = &m->st[--m->sp];
            if (!g_x_host.field_set(c, k->s, v, &why)) x_fail(e, "%s = ...: no variable %s - let %s = ... makes one", k->s, k->s, k->s);
            else if (why) x_fail(e, "%s = ...: %s", k->s, why);
            break;
        }
        case XI_REF: {   // a variable's place: slot a of the function b out - a ref's slot: the place it names
            w = &f->view;
            for (int d = in->b; d > 0 && w; d--) w = w->up;
            v = xvm_push(m, e);
            xnothing(v);
            if (!w || in->a < 0 || in->a >= w->n) { x_fail(e, "a place whose frame is gone"); break; }
            if (w->val[in->a].kind == XV_REF) { *v = w->val[in->a]; break; }
            if (w->val[in->a].kind == XV_UNSET) { x_fail(e, "%s: a place before its let", w->name[in->a]); break; }
            struct xref *q = x_alloc(sizeof *q);
            if (!q) { e->err = 1; break; }
            q->w = w; q->hl = NULL; q->slot = in->a; q->np = 0;
            v->w = 1; v->kind = XV_REF; v->ref = q;
            break;
        }
        case XI_REFM: {   // the ref on top a field further
            if (!xref_get(e, top, &r)) break;
            if (r.kind != XV_REC || !r.w || !r.rec->ty) { x_fail(e, ".%s: %.40s is no struct - a field is one's", k->s, xtext_of(&r)); break; }
            if (k->ty != r.rec->ty || k->mk != -1) { k->ty = r.rec->ty; k->mk = -1; k->mi = (short)(xst_field(k->ty, k->s) + 1); }
            if (!k->mi) { x_fail(e, ".%s: a %s has no such field", k->s, r.rec->ty->name); break; }
            xref_step(e, top, k->mi - 1);
            break;
        }
        case XI_REFI: {   // the ref below an item further: its number (from 1) on top
            struct xval ix = m->st[--m->sp];
            v = &m->st[m->sp - 1];
            if (!xref_get(e, v, &r) || !x_numeric_ok(e, &ix)) break;
            int i = (int)ix.n;
            if (r.kind == XV_DICT) { x_fail(e, "[...]: a dict's entry is no place - d[k] = ... sets it"); break; }
            if (!ix.w || (double)i != ix.n || i < 1 || i > xl_n(&r)) { x_fail(e, "[%s]: no such item - the list has %d (the first is [1])", xtext_of(&ix), xl_n(&r)); break; }
            xref_step(e, v, -i);   // (-1 - its place from 0)
            break;
        }
        case XI_REFGET: if (xref_get(e, top, &r)) *xvm_push(m, e) = r; break;
        case XI_REFSET: m->sp -= 2; xvm_ref_put(m, e, f, &m->st[m->sp], &m->st[m->sp + 1]); break;
        case XI_DEREF: if (xref_get(e, top, &r)) *top = r; break;
        case XI_STORER: m->sp--; xvm_ref_put(m, e, f, &m->st[f->base + in->a], &m->st[m->sp]); break;
        case XI_NEW: {   // a struct of type a: its b fields' values below
            int n = in->b;
            const struct xstype *ty = in->a >= 0 && in->a < code->nst ? &code->st[in->a] : NULL;
            if (!ty || n != ty->nf || m->sp < n) { x_fail(e, "the script's instructions are broken (a struct)"); break; }
            struct xrec *rec = x_alloc(sizeof *rec);
            struct xval *val = x_alloc((size_t)n * sizeof *val + 1);
            if (!rec || !val) { e->err = 1; break; }
            rec->n = n; rec->key = ty->fname; rec->val = val; rec->ty = ty;
            for (int i = 0; i < n; i++) val[i] = m->st[m->sp - n + i];
            m->sp -= n;
            v = xvm_push(m, e);
            xnothing(v);
            v->w = 1; v->kind = XV_REC; v->rec = rec;
            break;
        }
        case XI_METHOD: {   // a struct's method: its struct's place and b arguments below - its function entered, this the place
            int na = in->b, base = m->sp - na - 1;
            if (base < 0 || !xref_get(e, &m->st[base], &r)) { if (!e->err) x_fail(e, "the script's instructions are broken (a method)"); break; }
            const struct xstype *ty = r.kind == XV_REC && r.w ? r.rec->ty : NULL;
            if (!ty || ty < code->st || ty >= code->st + code->nst) { x_fail(e, ".%s(...): %.40s is no struct of this script", k->s, xtext_of(&r)); break; }
            if (k->ty != ty || k->mk != -2) {
                k->ty = ty; k->mk = -2; k->mi = 0;
                for (int j = 0; j < ty->nm; j++) if (!strcmp(ty->mname[j], k->s)) k->mi = (short)(j + 1);
            }
            if (!k->mi) { x_fail(e, ".%s(...): a %s has no such method", k->s, ty->name); break; }
            struct xfn fn = { code, ty->mfi[k->mi - 1], NULL, m };
            for (w = &f->view; w->up; w = w->up) {}
            if (w->name == code->fn[0].slot) fn.venv = w;   // the script's top: its names are the method's to see (a setup block has none)
            xvm_enter(m, e, &fn, base, na + 1);
            break;
        }
        case XI_END: m->done = 1; return XVM_DONE;
        default: x_fail(e, "the script's instructions are broken (%d)", in->op); break;
        }
        if (!e->err) continue;
        int caught = 0;   // a mistake: a list's item being tried is left out (told) - else it ends what runs
        while (m->ntr > ntr0 && !caught) {
            const struct xvtry *t = &m->tr[--m->ntr];
            if (!(c->failed > t->failed && !c->circle)) continue;
            while (m->nfr > t->nfr) { m->nfr--; c->calls--; }
            c->view = &m->fr[m->nfr - 1].view;
            m->sp = t->sp;
            m->pc = t->pc;
            e->err = 0; e->why = NULL;
            caught = 1;
        }
        if (!caught) return XVM_FAIL;
    }
}
// a compiled function called by the program (a trigger's, an after's, one a {placeholder} names): worked out at once
// on its script's machine, above what stands there - it can't wait
internal void xvm_call(struct xe *e, const struct xval *fv, const struct xval *a, int na, struct xval *v) {
    struct xvm *m = fv->fn->vm;
    int pc = m->pc, nfr = m->nfr, sp = m->sp, ntr = m->ntr, calls = e->c->calls, line = m->line;
    const struct xview *view = e->c->view;
    const char *top = e->c->top;
    xnothing(v);
    if (sp + na + 8 > XVM_STACK) { x_fail(e, "a script's values piled too high"); return; }
    for (int i = 0; i < na; i++) m->st[m->sp++] = a[i];
    if (xvm_enter(m, e, fv->fn, sp, na) && xvm_run(m, e, nfr) == XVM_RET) *v = m->st[sp];
    else if (!e->err) { e->err = 1; e->why = "a mistake while working it out"; }
    m->pc = pc; m->nfr = nfr; m->sp = sp; m->ntr = ntr; m->line = line;
    e->c->calls = calls; e->c->view = view; e->c->top = top;
}
internal int x_ph_method(struct xe *e, const struct xval *o, const char *mname, const struct xval *a, int na, struct xval *v) {
    const struct xstype *ty = o->rec->ty;
    struct xcode *code = g_x_ph_code;
    struct xvm *m = g_x_ph_vm;
    int j = 0;
    if (!m || !code || ty < code->st || ty >= code->st + code->nst) return 0;
    while (j < ty->nm && strcmp(ty->mname[j], mname)) j++;
    if (j == ty->nm) { x_fail(e, ".%s(...): a %s has no such method", mname, ty->name); return 1; }
    if (na > 7) { x_fail(e, ".%s(...): more than 7 arguments", mname); return 1; }
    static const char *const nm[1] = { "this" };
    struct xval copy = *o, args[8], fv;
    struct xview tv = { nm, &copy, 1, NULL, 0 };   // the copy as a variable of its own: the method's this names it
    struct xref rf;
    rf.w = &tv; rf.hl = NULL; rf.slot = 0; rf.np = 0;
    xnothing(&args[0]);
    args[0].w = 1; args[0].kind = XV_REF; args[0].ref = &rf;
    for (int i = 0; i < na; i++) args[i + 1] = a[i];
    struct xfn fn = { code, ty->mfi[j], m->nfr && m->fr[0].view.name == code->fn[0].slot ? &m->fr[0].view : NULL, m };
    xnothing(&fv);
    fv.w = 1; fv.kind = XV_FN; fv.fn = &fn;
    xvm_call(e, &fv, args, na + 1, v);
    return 1;
}
// a machine for compiled code: its top function entered (hooks: which of the code's hooks it calls)
internal struct xvm *xvm_new(struct xcode *code, int hooks) {
    struct xvm *m = calloc(1, sizeof *m);
    if (!m) return NULL;
    m->code = code; m->hooks = hooks;
    m->c.found = 1; m->c.script = 1;
    struct xvframe *fr = &m->fr[m->nfr++];
    const struct xfunc *f = &code->fn[0];
    fr->code = code; fr->fn = 0; fr->base = 0;
    fr->view.name = f->slot; fr->view.val = m->st; fr->view.n = f->nslots; fr->view.up = NULL; fr->view.keep = 1;
    for (int i = 0; i < f->nslots; i++) xvm_unset(&m->st[i]);
    m->sp = f->nslots;
    m->c.view = &fr->view;
    return m;
}
// the machine's turn: on from where it stands until it waits, ends or stops on a mistake (*why, at m->line)
internal int xvm_turn(struct xvm *m, const char **why) {
    struct xe e = { &m->c, 0, NULL, 0, 0 };
    if (m->done) return XVM_DONE;
    if (!m->started) { m->started = 1; g_x_last_fail[0] = 0; }
    m->c.view = &m->fr[m->nfr - 1].view;
    int r = xvm_run(m, &e, 0);
    if (r == XVM_FAIL) { m->done = 1; *why = g_x_last_fail[0] ? g_x_last_fail : e.why ? e.why : "not worked out"; }
    return r;
}
// the values a machine holds copied into the region now (g_xr), each whole: what is left behind in the one before is
// garbage then (the runner frees it) - a script's memory doesn't grow with what it worked out and dropped
internal void xvm_copy(struct xval *dst, const struct xval *src) {
    *dst = *src;
    if (src->s && src->s[0]) dst->s = x_strdup(src->s, strlen(src->s));
    if (src->kind == XV_VEC && src->vc) {
        double *m = x_alloc((size_t)src->vn * sizeof *m);
        if (m) memcpy(m, src->vc, (size_t)src->vn * sizeof *m);
        dst->vc = m;
    }
    if (src->kind == XV_REC && src->rec) {
        struct xrec *r = x_alloc(sizeof *r);
        const char **key = x_alloc((size_t)src->rec->n * sizeof *key + 1);
        struct xval *val = x_alloc((size_t)src->rec->n * sizeof *val + 1);
        if (!r || !key || !val) { xnothing(dst); return; }
        r->n = src->rec->n; r->key = key; r->val = val; r->ty = src->rec->ty;
        if (r->ty) r->key = src->rec->key;   // (a struct's names: its type's, which stay)
        for (int i = 0; i < r->n; i++) { if (!r->ty) key[i] = x_strdup(src->rec->key[i], strlen(src->rec->key[i])); xvm_copy(&val[i], &src->rec->val[i]); }
        dst->rec = r;
    }
    if (src->kind == XV_REF && src->ref) {   // a ref: its way copied - the view it starts in stays where it is
        size_t n = sizeof(struct xref) + (size_t)src->ref->np * sizeof(int);
        struct xref *q = x_alloc(n);
        if (q) memcpy(q, src->ref, n);
        dst->ref = q;
    }
    if (src->kind == XV_FN && src->fn) {
        struct xfn *fn = x_alloc(sizeof *fn);
        if (fn) *fn = *src->fn;
        dst->fn = fn;
    }
    if (src->ln) {
        struct xval *it = x_alloc((size_t)src->ln * sizeof *it);
        if (!it) { dst->ln = 0; dst->it = NULL; return; }
        for (int i = 0; i < src->ln; i++) xvm_copy(&it[i], &src->it[i]);
        dst->it = it;
    }
}
internal void xvm_collect(struct xvm *m) {
    for (int i = 0; i < m->sp; i++) { struct xval t; xvm_copy(&t, &m->st[i]); m->st[i] = t; }
}
// a script's region (the one in use, g_xr) cleared of what the script dropped: the values its machine holds copied
// into a new one, the old freed - and not again before it has grown to twice that and a step
internal void xvm_gc(struct xvm *m) {
    struct xregion old = *g_xr;
    memset(g_xr, 0, sizeof *g_xr);
    g_xr->busy = old.busy;
    xvm_collect(m);
    x_region_free(&old);
    m->gc_at = g_xr->total * 2 + X_GC_STEP;
    if (g_xhl_made) xhl_sweep();   // (the lists nobody holds any more: freed too)
}
internal int xfn_nparams(const struct xfn *fn) { return fn->code->fn[fn->fi].nparams; }
// THE TEXTS' MACHINE: every text's code runs on this one - a value at a time: its top function entered above whatever
// stands there (a name's body within a line's, a {placeholder} within that ...) and run to its return
internal struct xvm *g_xtm;
internal void xhl_mark_roots(void) {
    for (int i = 0; i < g_xt_n; i++) { xhl_mark(&g_xt[i].when); xhl_mark(&g_xt[i].run); }
    for (int i = 0; i < g_xa_n; i++) xhl_mark(&g_xa[i].fn);
    for (int i = 0; i < g_xt_aside_n; i++) { xhl_mark(&g_xt_aside[i].when); xhl_mark(&g_xt_aside[i].run); }
    for (int i = 0; i < g_xa_aside_n; i++) xhl_mark(&g_xa_aside[i].fn);
    if (g_xtm) for (int j = 0; j < g_xtm->sp; j++) xhl_mark(&g_xtm->st[j]);
}
internal void xhl_sweep(void) {
    struct xvm *vm[16];
    int nvm;
    if ((!g_xhl && !g_xdicts) || !g_x_machines || (nvm = g_x_machines(vm, 16)) < 0) return;
    if (++g_xhl_gen == 0) g_xhl_gen = 1;
    g_xhl_nseen = 0;
    for (int i = 0; i < nvm; i++) {
        const struct xvm *m = vm[i];
        for (int j = 0; m && j < m->sp; j++) xhl_mark(&m->st[j]);
        for (int j = 0; m && j < m->nfr; j++) xhl_mark_view(m->fr[j].view.up);
    }
    xhl_mark_roots();
    if (g_xhl_nseen == XHL_VIEWS) return;   // (too many views to be sure of every list: none freed this time)
    for (struct xhlist **pp = &g_xhl; *pp; ) {
        struct xhlist *h = *pp;
        if (h->mark == g_xhl_gen) { pp = &h->next; continue; }
        *pp = h->next;
        xhl_free(h);
        g_xhl_n--;
    }
    for (struct xdict **pp = &g_xdicts; *pp; ) {   // (a personality's dicts aren't in it: its own)
        struct xdict *d = *pp;
        if (d->mark == g_xhl_gen) { pp = &d->next; continue; }
        *pp = d->next;
        x_dict_free(d);
        g_xdict_n--;
    }
    g_xhl_made = 0;
}
internal struct xvm *xvm_texts(void) {
    if (!g_xtm && (g_xtm = calloc(1, sizeof *g_xtm)) != NULL) g_xtm->text = 1;
    return g_xtm;
}
// code k's value in the context e->c; up: the names it may see by their letters (a {placeholder} of a script's string:
// the script's), NULL none. 0: a mistake (e->err; told or not, as it was found)
internal int xvm_eval(struct xcode *k, struct xe *e, struct xval *v, const struct xview *up) {
    struct xvm *m = xvm_texts();
    struct xctx *c = e->c;
    xnothing(v);
    if (!m) { e->err = 1; return 0; }
    int pc = m->pc, nfr = m->nfr, sp = m->sp, ntr = m->ntr, calls = c->calls, line = m->line;
    const struct xview *view = c->view;
    const char *top = c->top;
    const struct xfunc *f = &k->fn[0];
    if (nfr >= XVM_FRAMES || sp + f->nslots + 64 > XVM_STACK) { x_fail(e, "values worked out inside each other too deep"); return 0; }
    struct xvframe *fr = &m->fr[m->nfr++];
    fr->code = k; fr->fn = 0; fr->ret_pc = pc; fr->base = sp;
    fr->view.name = f->slot; fr->view.val = &m->st[sp]; fr->view.n = f->nslots; fr->view.up = (struct xview *)up; fr->view.keep = 0;
    for (int i = 0; i < f->nslots; i++) xvm_unset(&m->st[sp + i]);
    m->sp = sp + f->nslots;
    m->pc = f->entry;
    c->view = &fr->view;
    if (xvm_run(m, e, nfr) == XVM_RET) *v = m->st[sp];
    else if (!e->err) { e->err = 1; e->why = "a mistake while working it out"; }
    m->pc = pc; m->nfr = nfr; m->sp = sp; m->ntr = ntr; m->line = line;
    c->calls = calls; c->view = view; c->top = top;
    return !e->err;
}
// a script's game.setup blocks, each worked out now (test/scripts.c, at the game's start) on the texts' machine: before
// the game there is nothing to wait for (a wait there is a mistake), and its script isn't running - of its names a
// block sees none; 0: a mistake in one (told - the rest not run)
internal int x_setup_run(struct xcode *code, struct xctx *c) {
    for (int i = 0; i < code->nsetup; i++) {
        struct xfn fn = { code, code->setup[i].fi, NULL, xvm_texts() };
        struct xval fv, rv;
        struct xe e = { c, 0, NULL, 0, 0 };
        if (!fn.vm) return 0;
        xnothing(&fv);
        fv.w = 1; fv.kind = XV_FN; fv.fn = &fn;
        c->top = code->src + code->setup[i].at;
        xvm_call(&e, &fv, NULL, 0, &rv);
        c->top = NULL;
        if (e.err) return 0;
    }
    return 1;
}

// ===== SAVED STATE: the scripts' side of a saved game (test/scripts.c writes the block: THE SCRIPTS IN A SAVED GAME).
// A script is its code and its machine, both plain data, so the save is them as they are: the instructions with their
// constants and functions (a save runs its own - not what a newer mod would compile of the source), the machine's
// frames, its stack, the lists it is trying, where it stands; then the triggers, the after's, the views they keep (a
// function's slots copied for a trigger made in it), the dicts. A value is written by what it is - a number, a text,
// a list, a vector, a record, an object by its number (a unit its id: handles, no pointers of the game's), a trigger
// its place in the table, a dict its place among the dicts, a function its script, its number there and the view it
// was made in: a frame of a machine, or a kept view. A text's function (a personality's, on the texts' machine) has no place in it:
// nothing saved for it, counted (lost). A dict of plain values is written whole with the dicts, early; one of other
// values (a struct, a list ...) its types there and its default and entries at the end, after the lists' items - a
// value may name any list, dict or struct type, which are all known by then.
#define XS_VIEWS 512
struct xsave {
    struct pbuf *o;
    struct xcode *const *code;          // the scripts' code, and their machines (NULL: one that hasn't begun)
    struct xvm *const *vm;
    int n;
    struct xview *kv[XS_VIEWS];         // the kept views, each numbered by its place + 1
    int nkv;
    struct xdict **di;                  // the dicts, numbered so
    int ndi, cdi;
    struct xhlist **hl;                 // the lists, numbered so (LISTS)
    int nhl, chl;
    int lost;                           // values that couldn't be saved
};
// a view as two numbers: (0, 0) none; (1 + script, frame): that machine's frame; (-1, n): kept view n (a new one noted)
internal void xs_view_ref(struct xsave *s, struct xview *w, int *a, int *b) {
    *a = *b = 0;
    if (!w) return;
    for (int i = 0; i < s->n; i++) {
        struct xvm *m = s->vm[i];
        if (m && (char *)w >= (char *)m->fr && (char *)w < (char *)(m->fr + XVM_FRAMES)) { *a = 1 + i; *b = (int)((struct xvframe *)w - m->fr); return; }
    }
    *a = -1;
    for (int i = 0; i < s->nkv; i++) if (s->kv[i] == w) { *b = i + 1; return; }
    if (s->nkv == XS_VIEWS) { s->lost++; *a = 0; return; }
    s->kv[s->nkv++] = w;
    *b = s->nkv;
}
// a dict's number (its place + 1), a new one noted (*fresh); 0: none, no room
internal int xs_dict(struct xsave *s, struct xdict *d, int *fresh) {
    if (fresh) *fresh = 0;
    if (!d) return 0;
    for (int i = 0; i < s->ndi; i++) if (s->di[i] == d) return i + 1;
    if (s->ndi == s->cdi) {
        int cap = s->cdi ? s->cdi * 2 : 64;
        struct xdict **m = realloc(s->di, (size_t)cap * sizeof *m);
        if (!m) { s->lost++; return 0; }
        s->di = m; s->cdi = cap;
    }
    s->di[s->ndi++] = d;
    if (fresh) *fresh = 1;
    return s->ndi;
}
// a list's number (its place + 1), a new one noted (*fresh); 0: no room
internal int xs_hl(struct xsave *s, struct xhlist *h, int *fresh) {
    if (fresh) *fresh = 0;
    for (int i = 0; i < s->nhl; i++) if (s->hl[i] == h) return i + 1;
    if (s->nhl == s->chl) {
        int cap = s->chl ? s->chl * 2 : 64;
        struct xhlist **m = realloc(s->hl, (size_t)cap * sizeof *m);
        if (!m) { s->lost++; return 0; }
        s->hl = m; s->chl = cap;
    }
    s->hl[s->nhl++] = h;
    if (fresh) *fresh = 1;
    return s->nhl;
}
internal void xs_note(struct xsave *s, const struct xval *v) {   // the views, dicts and lists a value leads to
    int a, b, fresh;
    struct xhlist *h = v->kind == XV_HLIST ? v->hl : v->kind == XV_REF && v->ref ? v->ref->hl : NULL;
    if (h && xs_hl(s, h, &fresh) && fresh) for (int i = 0; i < h->n; i++) xs_note(s, &h->it[i]);
    if (v->kind == XV_FN && v->fn) xs_view_ref(s, v->fn->venv, &a, &b);
    if (v->kind == XV_REF && v->ref) xs_view_ref(s, v->ref->w, &a, &b);
    if (v->kind == XV_DICT && xs_dict(s, v->dict, &fresh) && fresh) {   // (its values that aren't plain)
        if (v->dict->def.xv) xs_note(s, v->dict->def.xv);
        for (int i = 0; i < v->dict->n; i++) if (v->dict->e[i].xv) xs_note(s, v->dict->e[i].xv);
    }
    for (int i = 0; i < v->ln; i++) xs_note(s, &v->it[i]);
    for (int i = 0; v->kind == XV_REC && v->rec && i < v->rec->n; i++) xs_note(s, &v->rec->val[i]);
}
internal void xs_put(struct xsave *s, const struct xval *v) {
    struct pbuf *o = s->o;
    int ci = -1, a, b;
    struct xval none;
    if (v->kind == XV_FN) {   // its script
        for (int i = 0; v->fn && i < s->n; i++) if (s->code[i] == v->fn->code) ci = i;
        if (ci < 0) { s->lost++; xnothing(&none); v = &none; }
    }
    uint8_t h[3] = { (uint8_t)v->kind, (uint8_t)(v->has_n != 0), (uint8_t)(v->has_s != 0) };
    pb_put(o, h, 3);
    pb_int(o, v->w); pb_int(o, v->obj); pb_int(o, v->okind); pb_int(o, v->ln);
    pb_num(o, v->n);
    pb_text(o, v->s ? v->s : "");
    for (int i = 0; i < v->ln; i++) xs_put(s, &v->it[i]);
    if (v->kind == XV_VEC) { pb_int(o, v->vn); pb_put(o, v->vc, (uint32_t)v->vn * 8); }
    else if (v->kind == XV_REC) {
        int sc = 0, ti = 0;   // a struct: its script + 1 and its type's number there (0: a plain record)
        for (int i = 0; v->rec->ty && i < s->n; i++)
            if (v->rec->ty >= s->code[i]->st && v->rec->ty < s->code[i]->st + s->code[i]->nst) { sc = i + 1; ti = (int)(v->rec->ty - s->code[i]->st); }
        if (v->rec->ty == &g_x_boxty) sc = -1;   // a row (a list as an item): no script's type
        if (v->rec->ty && !sc) s->lost++;
        pb_int(o, v->rec->n); pb_int(o, sc); pb_int(o, ti);
        for (int i = 0; i < v->rec->n; i++) { pb_str(o, sc < 0 ? "" : v->rec->key[i]); xs_put(s, &v->rec->val[i]); }
    } else if (v->kind == XV_REF) {   // a ref: the view its variable is in (-2: a list, its number), the slot, the way on
        xs_view_ref(s, v->ref ? v->ref->w : NULL, &a, &b);
        if (v->ref && v->ref->hl) { a = -2; b = xs_hl(s, v->ref->hl, NULL); }
        pb_int(o, a); pb_int(o, b); pb_int(o, v->ref ? v->ref->slot : 0); pb_int(o, v->ref ? v->ref->np : 0);
        for (int i = 0; v->ref && i < v->ref->np; i++) pb_int(o, v->ref->path[i]);
    } else if (v->kind == XV_FN) {
        xs_view_ref(s, v->fn->venv, &a, &b);
        pb_int(o, ci); pb_int(o, v->fn->fi); pb_int(o, a); pb_int(o, b);
    } else if (v->kind == XV_DICT) pb_int(o, xs_dict(s, v->dict, NULL));
    else if (v->kind == XV_HLIST) pb_int(o, xs_hl(s, v->hl, NULL));
}
internal void xs_put_dent(struct pbuf *o, const struct xdent *x) {
    pb_int(o, x->kobj); pb_num(o, x->kn); pb_num(o, x->vn); pb_int(o, (x->ks != NULL) | (x->vs != NULL) << 1 | (x->vhas_s != 0) << 2);
    if (x->ks) pb_text(o, x->ks);
    if (x->vs) pb_text(o, x->vs);
}
internal void xs_put_code(struct pbuf *o, const struct xcode *k) {   // the instructions, their lines, the constants, the functions
    pb_int(o, k->n);
    pb_put(o, k->ins, (uint32_t)k->n * sizeof *k->ins);
    pb_put(o, k->line, (uint32_t)k->n * sizeof *k->line);
    pb_int(o, k->nk);
    for (int i = 0; i < k->nk; i++) { pb_num(o, k->k[i].n); pb_int(o, k->k[i].s != NULL); if (k->k[i].s) pb_text(o, k->k[i].s); }
    pb_int(o, k->nfn);
    for (int i = 0; i < k->nfn; i++) {
        const struct xfunc *f = &k->fn[i];
        pb_int(o, f->entry); pb_int(o, f->nparams | f->nopt << 8); pb_int(o, f->nslots); pb_int(o, f->parent); pb_int(o, f->line);
        for (int j = 0; j < f->nslots; j++) pb_str(o, f->slot[j]);
    }
    pb_int(o, k->nst);   // its structs: each one's fields' names and its methods' names and functions
    for (int i = 0; i < k->nst; i++) {
        const struct xstype *t = &k->st[i];
        pb_str(o, t->name); pb_int(o, t->nf); pb_int(o, t->nm);
        for (int j = 0; j < t->nf; j++) pb_str(o, t->fname[j]);
        for (int j = 0; j < t->nm; j++) { pb_str(o, t->mname[j]); pb_int(o, t->mfi[j]); }
    }
}
// the state written: code[i] script i's code, vm[i] its machine (NULL: it hasn't begun)
internal void x_state_save(struct pbuf *o, struct xcode *const *code, struct xvm *const *vm, int n, int *lost) {
    struct xsave *s = calloc(1, sizeof *s);
    int a, b;
    if (!s) { o->bad = 1; return; }
    s->o = o; s->code = code; s->vm = vm; s->n = n;
    pb_int(o, XI_COUNT);   // the instructions this build knows (a newer build's save may hold more)
    pb_int(o, n);
    for (int i = 0; i < n; i++) {   // what the machines, the triggers and the after's lead to
        const struct xvm *m = vm[i];
        for (int j = 0; m && j < m->nfr; j++) xs_view_ref(s, m->fr[j].view.up, &a, &b);
        for (int j = 0; m && j < m->sp; j++) xs_note(s, &m->st[j]);
    }
    for (int i = 0; i < g_xt_n; i++) { xs_note(s, &g_xt[i].when); xs_note(s, &g_xt[i].run); }
    for (int i = 0; i < g_xa_n; i++) xs_note(s, &g_xa[i].fn);
    for (int i = 0; i < s->nkv; i++) {   // (the list grows as it is walked)
        struct xview *w = s->kv[i];
        xs_view_ref(s, w->up, &a, &b);
        for (int j = 0; j < w->n; j++) xs_note(s, &w->val[j]);
    }
    int ndi = s->ndi;
    pb_int(o, ndi);
    for (int i = 0; i < ndi; i++) {   // (one of values that aren't plain: those at the end)
        const struct xdict *d = s->di[i];
        pb_int(o, d->kt); pb_int(o, d->vt); pb_int(o, d->n);
        if (!xdict_plain(d)) continue;
        xs_put_dent(o, &d->def);
        for (int j = 0; j < d->n; j++) xs_put_dent(o, &d->e[j]);
    }
    int nhl = s->nhl;
    pb_int(o, nhl);   // the lists (their items at the end: a value may name any of them)
    for (int i = 0; i < n; i++) xs_put_code(o, code[i]);
    for (int i = 0; i < n; i++) {   // the machines' frames and the kept views first: a value may name any of them
        const struct xvm *m = vm[i];
        pb_int(o, m ? m->nfr : 0);
        for (int j = 0; m && j < m->nfr; j++) {
            xs_view_ref(s, m->fr[j].view.up, &a, &b);
            pb_int(o, m->fr[j].fn); pb_int(o, m->fr[j].ret_pc); pb_int(o, m->fr[j].base); pb_int(o, a); pb_int(o, b);
        }
    }
    int nkv = s->nkv;
    pb_int(o, nkv);
    for (int i = 0; i < nkv; i++) {   // a kept view's names are a function's: which script's, which
        struct xview *w = s->kv[i];
        int ci = -1, fi = -1;
        for (int c = 0; c < n && ci < 0; c++)
            for (int j = 0; j < code[c]->nfn; j++) if (code[c]->fn[j].slot == w->name) { ci = c; fi = j; break; }
        if (ci < 0) s->lost++;
        xs_view_ref(s, w->up, &a, &b);
        pb_int(o, ci); pb_int(o, fi); pb_int(o, w->n); pb_int(o, a); pb_int(o, b);
    }
    for (int i = 0; i < nkv; i++) for (int j = 0; j < s->kv[i]->n; j++) xs_put(s, &s->kv[i]->val[j]);
    for (int i = 0; i < n; i++) {
        const struct xvm *m = vm[i];
        if (!m) continue;
        pb_int(o, m->sp);
        for (int j = 0; j < m->sp; j++) xs_put(s, &m->st[j]);
        pb_int(o, m->ntr);
        for (int j = 0; j < m->ntr; j++) { pb_int(o, m->tr[j].sp); pb_int(o, m->tr[j].nfr); pb_int(o, m->tr[j].pc); pb_int(o, m->tr[j].failed); }
        pb_int(o, m->pc); pb_int(o, m->hooks); pb_int(o, m->line); pb_int(o, m->done); pb_int(o, m->started);
        pb_num(o, m->wake);
        pb_int(o, m->c.calls); pb_int(o, m->c.failed);
        pb_int(o, m->c.top && m->c.top >= m->code->src ? (int)(m->c.top - m->code->src) : -1);
    }
    pb_int(o, g_xt_n);
    for (int i = 0; i < g_xt_n; i++) {
        const struct xtrig *t = &g_xt[i];
        xs_put(s, &t->when); xs_put(s, &t->run);
        pb_int(o, t->on); pb_int(o, t->fired); pb_int(o, t->loop);
        pb_num(o, t->hold); pb_num(o, t->since); pb_num(o, t->period); pb_num(o, t->next);
        pb_str(o, t->name);
    }
    pb_int(o, g_xa_n);
    for (int i = 0; i < g_xa_n; i++) { pb_num(o, g_xa[i].at); xs_put(s, &g_xa[i].fn); pb_str(o, g_xa[i].name); }
    pb_num(o, g_xt_next);
    for (int i = 0; i < nhl; i++) {
        pb_int(o, s->hl[i]->n);
        for (int j = 0; j < s->hl[i]->n; j++) xs_put(s, &s->hl[i]->it[j]);
    }
    for (int i = 0; i < ndi; i++) {   // the dicts' values that aren't plain: the default, then each entry's key and value
        const struct xdict *d = s->di[i];
        struct xval none;
        xnothing(&none);
        if (xdict_plain(d)) continue;
        xs_put(s, d->def.xv ? d->def.xv : &none);
        for (int j = 0; j < d->n; j++) {
            struct xdent k = d->e[j];
            k.vs = NULL; k.xv = NULL;
            xs_put_dent(o, &k);
            xs_put(s, d->e[j].xv ? d->e[j].xv : &none);
        }
    }
    if (s->nhl != nhl || s->ndi != ndi) s->lost++;   // (a list or a dict the noting missed)
    free(s->hl);
    free(s->di);
    if (s->nkv != nkv) s->lost++;   // (a view the noting missed)
    *lost = s->lost;
    free(s);
}
struct xload {
    struct pread *r;
    struct xcode **code;
    struct xvm **vm;
    int n;
    struct xview *kv[XS_VIEWS];
    int nkv;
    struct xdict **di;
    int ndi;
    struct xhlist **hl;
    int nhl;
};
internal struct xview *xs_view_at(struct xload *l, int a, int b) {   // the view two numbers name; none such: bad
    if (!a) return NULL;
    if (a > 0 && a <= l->n && l->vm[a - 1] && b >= 0 && b < l->vm[a - 1]->nfr) return &l->vm[a - 1]->fr[b].view;
    if (a == -1 && b >= 1 && b <= l->nkv) return l->kv[b - 1];
    l->r->bad = 1;
    return NULL;
}
internal void xs_get(struct xload *l, struct xval *v) {   // a value read, its texts and items into the region now
    struct pread *r = l->r;
    uint8_t h[3];
    xnothing(v);
    pr_get(r, h, 3);
    int w = pr_int(r), obj = pr_int(r), okind = pr_int(r), ln = pr_int(r);
    double n = pr_num(r);
    char *t = pr_text(r);
    if (r->bad || h[0] > XV_HLIST || ln < 0 || (uint32_t)ln > r->n - r->o) { r->bad = 1; free(t); return; }
    v->kind = h[0]; v->has_n = h[1]; v->has_s = h[2]; v->w = w; v->obj = obj; v->okind = okind; v->n = n;
    if (t && t[0]) v->s = x_strdup(t, strlen(t));
    free(t);
    if (ln) {
        struct xval *it = x_alloc((size_t)ln * sizeof *it);
        if (!it) { r->bad = 1; xnothing(v); return; }
        for (int i = 0; i < ln; i++) xs_get(l, &it[i]);
        v->it = it; v->ln = ln;
    }
    if (v->kind == XV_VEC) {
        double c[4] = { 0 };
        int vn = pr_int(r);
        if (vn < 2 || vn > 4) { r->bad = 1; xnothing(v); return; }
        pr_get(r, c, (uint32_t)vn * 8);
        double *m = x_alloc((size_t)vn * sizeof *m);
        if (!m) { r->bad = 1; xnothing(v); return; }
        memcpy(m, c, (size_t)vn * sizeof *m);
        v->vn = vn; v->vc = m;
    } else if (v->kind == XV_REC) {
        int rn = pr_int(r), sc = pr_int(r), ti = pr_int(r);
        if (rn < 0 || (uint32_t)rn > r->n - r->o) { r->bad = 1; xnothing(v); return; }
        struct xrec *rec = x_alloc(sizeof *rec);
        const char **key = x_alloc((size_t)rn * sizeof *key + 1);
        struct xval *val = x_alloc((size_t)rn * sizeof *val + 1);
        if (!rec || !key || !val) { r->bad = 1; xnothing(v); return; }
        rec->n = rn; rec->key = key; rec->val = val; rec->ty = NULL;
        if (sc == -1) { rec->ty = &g_x_boxty; rec->key = NULL; }   // a row
        else if (sc) {   // a struct: its type of the script's code as it was saved - its names the type's
            if (sc < 1 || sc > l->n || !l->code[sc - 1] || ti < 0 || ti >= l->code[sc - 1]->nst || l->code[sc - 1]->st[ti].nf != rn) { r->bad = 1; xnothing(v); return; }
            rec->ty = &l->code[sc - 1]->st[ti];
            rec->key = rec->ty->fname;
        }
        for (int i = 0; i < rn; i++) { char *k = pr_str(r); if (!sc) key[i] = x_strdup(k ? k : "", k ? strlen(k) : 0); free(k); xs_get(l, &val[i]); }
        v->rec = rec;
    } else if (v->kind == XV_REF) {
        int a = pr_int(r), b = pr_int(r), slot = pr_int(r), np = pr_int(r);
        struct xhlist *hl = a == -2 && b >= 1 && b <= l->nhl ? l->hl[b - 1] : NULL;   // (a list's item: its items are read last)
        struct xview *vw = a == -2 ? NULL : xs_view_at(l, a, b);
        if ((!vw && !hl) || np < 0 || np > 64 || slot < 0 || (vw && slot >= vw->n)) { r->bad = 1; xnothing(v); return; }
        struct xref *q = x_alloc(sizeof *q + (size_t)np * sizeof(int));
        if (!q) { r->bad = 1; xnothing(v); return; }
        q->w = vw; q->hl = hl; q->slot = slot; q->np = np;
        for (int i = 0; i < np; i++) q->path[i] = pr_int(r);
        v->ref = q;
    } else if (v->kind == XV_FN) {
        int ci = pr_int(r), fi = pr_int(r), a = pr_int(r), b = pr_int(r);
        struct xfn *fn = x_alloc(sizeof *fn);
        if (!fn || ci < 0 || ci >= l->n || !l->vm[ci] || fi < 0 || fi >= l->code[ci]->nfn) { r->bad = 1; xnothing(v); return; }
        memset(fn, 0, sizeof *fn);
        fn->code = l->code[ci]; fn->fi = fi; fn->vm = l->vm[ci];
        fn->venv = xs_view_at(l, a, b);
        v->fn = fn;
    } else if (v->kind == XV_DICT) {
        int d = pr_int(r);
        if (d < 1 || d > l->ndi) { r->bad = 1; xnothing(v); return; }
        v->dict = l->di[d - 1];
    } else if (v->kind == XV_HLIST) {
        int d = pr_int(r);
        if (d < 1 || d > l->nhl) { r->bad = 1; xnothing(v); return; }
        v->hl = l->hl[d - 1];
    }
}
internal void xs_get_dent(struct pread *r, struct xdent *x) {   // its texts malloc'd (the dict's)
    memset(x, 0, sizeof *x);
    x->kobj = pr_int(r); x->kn = pr_num(r); x->vn = pr_num(r);
    int has = pr_int(r);
    if (has & 1) x->ks = pr_text(r);
    if (has & 2) x->vs = pr_text(r);
    x->vhas_s = (has & 4) != 0;
}
internal struct xcode *xs_get_code(struct pread *r, const char *src, int ops) {   // a script's code as it was saved (it stays, as a compiled one)
    struct xcode *k = calloc(1, sizeof *k);
    if (!k) { r->bad = 1; return NULL; }
    k->src = src; k->ok = 1;
    k->n = pr_int(r);
    if (k->n < 1 || (uint32_t)k->n > (r->n - r->o) / (sizeof *k->ins + sizeof *k->line)) { r->bad = 1; k->n = 0; return k; }
    k->cap = k->n;
    k->ins = malloc((size_t)k->n * sizeof *k->ins);
    k->line = malloc((size_t)k->n * sizeof *k->line);
    if (!k->ins || !k->line) { r->bad = 1; k->n = 0; return k; }
    pr_get(r, k->ins, (uint32_t)k->n * sizeof *k->ins);
    pr_get(r, k->line, (uint32_t)k->n * sizeof *k->line);
    for (int i = 0; i < k->n; i++) if (k->ins[i].op >= ops) r->bad = 1;
    k->nk = pr_int(r);
    if (k->nk < 0 || (uint32_t)k->nk > r->n - r->o) { r->bad = 1; k->nk = 0; }
    k->ck = k->nk;
    k->k = calloc((size_t)k->nk + 1, sizeof *k->k);
    if (!k->k) { r->bad = 1; k->nk = 0; return k; }
    for (int i = 0; i < k->nk && !r->bad; i++) { k->k[i].n = pr_num(r); if (pr_int(r)) k->k[i].s = pr_text(r); }
    k->nfn = pr_int(r);
    if (k->nfn < 1 || (uint32_t)k->nfn > r->n - r->o) { r->bad = 1; k->nfn = 0; }
    k->cfn = k->nfn;
    k->fn = calloc((size_t)k->nfn + 1, sizeof *k->fn);
    if (!k->fn) { r->bad = 1; k->nfn = 0; return k; }
    for (int i = 0; i < k->nfn && !r->bad; i++) {
        struct xfunc *f = &k->fn[i];
        f->entry = pr_int(r); f->nparams = pr_int(r); f->nslots = pr_int(r); f->parent = pr_int(r); f->line = pr_int(r);
        f->nopt = f->nparams >> 8; f->nparams &= 255;   // (the defaults' count above: a save before them has none)
        if (f->entry < 0 || f->entry >= k->n || f->nslots < 0 || f->nslots > 256 || f->nparams < 0 || f->nparams > f->nslots || f->nopt > f->nparams) { r->bad = 1; f->nslots = 0; break; }
        f->slot = calloc((size_t)f->nslots + 1, sizeof *f->slot);
        if (!f->slot) { r->bad = 1; f->nslots = 0; break; }
        for (int j = 0; j < f->nslots; j++) f->slot[j] = pr_str(r);
    }
    k->nst = r->bad ? 0 : pr_int(r);   // its structs
    if (k->nst < 0 || k->nst > XST_MAX) { r->bad = 1; k->nst = 0; }
    k->st = calloc((size_t)k->nst + 1, sizeof *k->st);
    if (!k->st) { r->bad = 1; k->nst = 0; return k; }
    for (int i = 0; i < k->nst && !r->bad; i++) {
        struct xstype *t = &k->st[i];
        t->name = pr_str(r);
        int nf = pr_int(r), nm = pr_int(r);
        if (nf < 0 || nf > 64 || nm < 0 || nm > 1024) { r->bad = 1; break; }
        t->fname = calloc((size_t)nf + 1, sizeof *t->fname);
        t->mname = calloc((size_t)nm + 1, sizeof *t->mname);
        t->mfi = calloc((size_t)nm + 1, sizeof *t->mfi);
        if (!t->fname || !t->mname || !t->mfi) { r->bad = 1; break; }
        for (int j = 0; j < nf; j++) { t->fname[j] = pr_str(r); if (!t->fname[j]) r->bad = 1; }
        for (int j = 0; j < nm; j++) { t->mname[j] = pr_str(r); t->mfi[j] = pr_int(r); if (!t->mname[j] || t->mfi[j] < 0 || t->mfi[j] >= k->nfn) r->bad = 1; }
        if (!t->name) r->bad = 1;
        t->nf = nf; t->nm = nm;
    }
    for (int i = 0; i < k->n && !r->bad; i++) {   // the numbers an instruction reads its constant, jump or function by: inside
        const struct xins *in = &k->ins[i];
        int op = in->op, jump = op == XI_JMP || op == XI_JMPF || op == XI_AND || op == XI_OR || op == XI_COAL || op == XI_TRY || op == XI_LOOP
                 || op == XI_FORN_NEXT || op == XI_FORL_NEXT || op == XI_EVERY_NEXT || op == XI_EVERY_DONE || op == XI_HOOK;
        if (jump && (in->a < 0 || in->a >= k->n)) r->bad = 1;
        if (op == XI_CLOSURE && (in->a < 0 || in->a >= k->nfn)) r->bad = 1;
        if ((op == XI_CONST || op == XI_NAME || op == XI_STR || op == XI_MEMBER || op == XI_MCALL || op == XI_KEY || op == XI_CALLN || op == XI_SETM
             || op == XI_FAIL || op == XI_FORN || op == XI_FORL || op == XI_REFM || op == XI_METHOD) && (in->a < 0 || in->a >= k->nk)) r->bad = 1;
        if (op == XI_NEW && (in->a < 0 || in->a >= k->nst)) r->bad = 1;
        if ((op == XI_LOADL || op == XI_LOADU || op == XI_STOREL || op == XI_STOREU || op == XI_CALLV || op == XI_WAIT || op == XI_HOOK) && in->c >= k->nk) r->bad = 1;
    }
    return k;
}
// the state read back, after x_triggers_reset: code[i] script i's code made (src[i]: its text), vm[i] its machine
// (NULL: it hadn't begun), its values in regs[i]; the triggers', the after's and the kept views' in their own region.
// 0: not what was written, or a newer build's (r->bad) - the caller starts afresh (x_triggers_reset, the scripts
// dropped: what was made so far is theirs)
internal int x_state_load(struct pread *r, const char *const *src, struct xregion *const *regs, struct xcode **code, struct xvm **vm, int n) {
    struct xload *l = calloc(1, sizeof *l);
    struct xregion *r0 = g_xr;
    int (*fref)[2] = calloc((size_t)(n > 0 ? n : 1) * XVM_FRAMES, sizeof *fref);
    static int kref[XS_VIEWS][2];
    if (!l || !fref) { free(l); free(fref); return 0; }
    l->r = r; l->code = code; l->vm = vm; l->n = n;
    int ops = pr_int(r);
    if (ops < 1 || ops > XI_COUNT || pr_int(r) != n) r->bad = 1;
    for (int i = 0; i < n; i++) { code[i] = NULL; vm[i] = NULL; }
    l->ndi = r->bad ? 0 : pr_int(r);
    if (l->ndi < 0 || (uint32_t)l->ndi > r->n - r->o) { r->bad = 1; l->ndi = 0; }
    l->di = calloc((size_t)l->ndi + 1, sizeof *l->di);
    int *dcount = calloc((size_t)l->ndi + 1, sizeof *dcount);   // (a dict of values that aren't plain: its entries, read at the end)
    if (!l->di || !dcount) { r->bad = 1; l->ndi = 0; }
    for (int i = 0; i < l->ndi && !r->bad; i++) {
        struct xdict *d = calloc(1, sizeof *d);
        if (!d) { r->bad = 1; l->ndi = i; break; }
        d->kt = pr_int(r); d->vt = pr_int(r);
        int dn = pr_int(r);
        d->next = g_xdicts; g_xdicts = d; g_xdict_n++;   // the language's list: freed with the game's, or by the sweep
        l->di[i] = d;
        if (dn < 0 || (uint32_t)dn > r->n - r->o) { r->bad = 1; break; }
        if (!xdict_plain(d)) { dcount[i] = dn; continue; }
        xs_get_dent(r, &d->def);
        for (int j = 0; j < dn && !r->bad; j++) { struct xdent x; xs_get_dent(r, &x); if (r->bad) { free(x.ks); free(x.vs); } else xdict_put(d, &x); }
    }
    l->nhl = r->bad ? 0 : pr_int(r);   // the lists, empty until their items are read (at the end)
    if (l->nhl < 0 || (uint32_t)l->nhl > r->n - r->o) { r->bad = 1; l->nhl = 0; }
    l->hl = calloc((size_t)l->nhl + 1, sizeof *l->hl);
    if (!l->hl) { r->bad = 1; l->nhl = 0; }
    for (int i = 0; i < l->nhl && !r->bad; i++) if (!(l->hl[i] = xhl_new())) { r->bad = 1; l->nhl = i; }
    for (int i = 0; i < n && !r->bad; i++) code[i] = xs_get_code(r, src[i], ops);
    for (int i = 0; i < n && !r->bad; i++) {   // the machines' frames
        int nfr = pr_int(r);
        if (nfr < 0 || nfr > XVM_FRAMES) { r->bad = 1; break; }
        if (!nfr) continue;
        struct xvm *m = calloc(1, sizeof *m);
        if (!m) { r->bad = 1; break; }
        vm[i] = m;
        m->code = code[i];
        m->c.found = 1; m->c.script = 1;
        for (int j = 0; j < nfr && !r->bad; j++) {
            struct xvframe *f = &m->fr[j];
            f->fn = pr_int(r); f->ret_pc = pr_int(r); f->base = pr_int(r);
            fref[i * XVM_FRAMES + j][0] = pr_int(r); fref[i * XVM_FRAMES + j][1] = pr_int(r);
            if (f->fn < 0 || f->fn >= code[i]->nfn || f->base < 0 || f->base + code[i]->fn[f->fn].nslots > XVM_STACK || f->ret_pc < 0 || f->ret_pc > code[i]->n) { r->bad = 1; break; }
            f->code = code[i];
            f->view.name = code[i]->fn[f->fn].slot; f->view.val = &m->st[f->base]; f->view.n = code[i]->fn[f->fn].nslots; f->view.keep = j == 0;
            m->nfr = j + 1;
        }
    }
    l->nkv = r->bad ? 0 : pr_int(r);
    if (l->nkv < 0 || l->nkv > XS_VIEWS) { r->bad = 1; l->nkv = 0; }
    g_xr = &g_xt_keep;
    for (int i = 0; i < l->nkv && !r->bad; i++) {
        int ci = pr_int(r), fi = pr_int(r), vn = pr_int(r);
        kref[i][0] = pr_int(r); kref[i][1] = pr_int(r);
        if (ci < 0 || ci >= n || fi < 0 || fi >= code[ci]->nfn || vn != code[ci]->fn[fi].nslots) { r->bad = 1; l->nkv = i; break; }
        struct xview *w = x_alloc(sizeof *w);
        struct xval *val = x_alloc((size_t)vn * sizeof *val + 1);
        if (!w || !val) { r->bad = 1; l->nkv = i; break; }
        w->name = code[ci]->fn[fi].slot; w->val = val; w->n = vn; w->up = NULL; w->keep = 1;
        for (int j = 0; j < vn; j++) xnothing(&val[j]);
        l->kv[i] = w;
    }
    for (int i = 0; i < n && !r->bad; i++)   // every view is there now: where each was made
        for (int j = 0; vm[i] && j < vm[i]->nfr; j++) vm[i]->fr[j].view.up = xs_view_at(l, fref[i * XVM_FRAMES + j][0], fref[i * XVM_FRAMES + j][1]);
    for (int i = 0; i < l->nkv && !r->bad; i++) l->kv[i]->up = xs_view_at(l, kref[i][0], kref[i][1]);
    for (int i = 0; i < l->nkv && !r->bad; i++) for (int j = 0; j < l->kv[i]->n; j++) xs_get(l, &l->kv[i]->val[j]);
    for (int i = 0; i < n && !r->bad; i++) {
        struct xvm *m = vm[i];
        if (!m) continue;
        g_xr = regs[i];
        int sp = pr_int(r);
        if (sp < 0 || sp > XVM_STACK) { r->bad = 1; break; }
        for (int j = 0; j < sp; j++) xs_get(l, &m->st[j]);
        m->sp = sp;
        m->ntr = pr_int(r);
        if (m->ntr < 0 || m->ntr > XVM_TRIES) { r->bad = 1; m->ntr = 0; break; }
        for (int j = 0; j < m->ntr; j++) { m->tr[j].sp = pr_int(r); m->tr[j].nfr = pr_int(r); m->tr[j].pc = pr_int(r); m->tr[j].failed = pr_int(r); }
        m->pc = pr_int(r); m->hooks = pr_int(r); m->line = pr_int(r); m->done = pr_int(r); m->started = pr_int(r);
        m->wake = pr_num(r);
        m->c.calls = pr_int(r); m->c.failed = pr_int(r);
        int top = pr_int(r);
        m->c.top = top >= 0 && (size_t)top <= strlen(src[i]) ? src[i] + top : NULL;
        if (m->pc < 0 || m->pc > m->code->n || (m->pc == m->code->n && !m->done)) r->bad = 1;   // (one past the end: a script that is done)
    }
    g_xr = &g_xt_keep;
    int nt = r->bad ? 0 : pr_int(r);
    if (nt < 0 || nt > XT_MAX) { r->bad = 1; nt = 0; }
    for (int i = 0; i < nt && !r->bad; i++) {
        struct xtrig *t = &g_xt[i];
        memset(t, 0, sizeof *t);
        xs_get(l, &t->when); xs_get(l, &t->run);
        t->on = pr_int(r); t->fired = pr_int(r); t->loop = pr_int(r);
        t->hold = pr_num(r); t->since = pr_num(r); t->period = pr_num(r); t->next = pr_num(r);
        char *name = pr_str(r);
        lstrcpynA(t->name, name ? name : "", sizeof t->name);
        free(name);
        g_xt_n = i + 1;
    }
    int na = r->bad ? 0 : pr_int(r);
    if (na < 0 || na > XA_MAX) { r->bad = 1; na = 0; }
    for (int i = 0; i < na && !r->bad; i++) {
        struct xafter *a = &g_xa[i];
        memset(a, 0, sizeof *a);
        a->at = pr_num(r);
        xs_get(l, &a->fn);
        char *name = pr_str(r);
        lstrcpynA(a->name, name ? name : "", sizeof a->name);
        free(name);
        g_xa_n = i + 1;
    }
    g_xt_next = pr_num(r);
    {   // the lists' items: each read, then made the list's own
        struct xregion tmp;
        struct xe e0 = { NULL, 0, NULL, 0, 0 };
        struct xctx c0;
        memset(&tmp, 0, sizeof tmp);
        memset(&c0, 0, sizeof c0);
        e0.c = &c0;
        tmp.busy = 1;
        g_xr = &tmp;
        for (int i = 0; i < l->nhl && !r->bad; i++) {
            int cnt = pr_int(r);
            if (cnt < 0 || (uint32_t)cnt > r->n - r->o) { r->bad = 1; break; }
            if (cnt && !xhl_room(&e0, l->hl[i], cnt)) { r->bad = 1; break; }
            for (int j = 0; j < cnt && !r->bad; j++) {
                struct xval t;
                xs_get(l, &t);
                if (!r->bad) xo_copy(&l->hl[i]->it[l->hl[i]->n++], &t);
            }
        }
        for (int i = 0; i < l->ndi && !r->bad; i++) {   // the dicts' values that aren't plain, made the dict's own
            struct xdict *d = l->di[i];
            struct xval t;
            if (xdict_plain(d)) continue;
            xs_get(l, &t);
            if (r->bad || !(d->def.xv = xdict_own(&t))) { r->bad = 1; break; }
            for (int j = 0; j < dcount[i] && !r->bad; j++) {
                struct xdent x;
                xs_get_dent(r, &x);
                xs_get(l, &t);
                if (!r->bad && !(x.xv = xdict_own(&t))) r->bad = 1;
                if (r->bad) { free(x.ks); xdict_vfree(&x); } else xdict_put(d, &x);
            }
        }
        x_region_free(&tmp);
    }
    free(dcount);
    free(l->di);
    free(l->hl);
    g_xr = r0;
    free(l);
    free(fref);
    return !r->bad;
}
