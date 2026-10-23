// THE HOST: what the language asks the program it runs in - the names and constants of its world, its objects (a value
// that is one of them: obj), the relations between them (them.kills(me)), its two random generators, the files its
// functions come from, the commands a script may call. The language knows none of them itself; the game's are set by
// eeaimod.c x_host_install. Until then (and for a program without them) every answer is "none".
//
// Its objects are of kinds (a value's okind, 1..; obj its number + 1): a kind's members by name - a value (amin -1:
// p.towers) or a method and its arguments (amin..amax: p.opinion(q)) - and what each gives: X_ANY (not known before it
// is worked out), a kind, X_LIST_OF + a kind (game.players); set: a value a script may set (slot.persona = ...).
// The check reads them when a file is read (p.towerz a mistake there), the working out asks member() and set()
#define X_ANY 0
#define X_LIST_OF 0x100
// THE TYPES the check knows (a value's, as far as it can tell when a file is read): X_ANY not known, 1..63 a kind of
// the host's objects, from XT_INT the language's own; + X_LIST_OF a list of them, + X_NULLABLE or null (T?)
enum { XT_INT = 64, XT_FLOAT, XT_BOOL, XT_TEXT, XT_VEC2, XT_VEC3, XT_VEC4, XT_EPOCH, XT_COLOR, XT_UTYPE, XT_FN, XT_REC,
       XT_TRIG, XT_STOP };
#define X_NULLABLE 0x200
#define X_TBASE(t) ((t) & 0xFF)   // without list and null
struct x_kmember { const char *name; signed char amin, amax; short result; signed char set; };
struct x_kind { const char *name; int numbered; const struct x_kmember *m; int n; };   // numbered: a command takes its number
struct x_host {
    int (*value)(struct xctx *c, const char *name, struct xval *v);   // a name's value (them, epoch, them.soldiers): 0 none
    int (*is_value)(const char *name);         // a name that is a value of its own (them, epoch, minutes ...)
    int (*name_valid)(const char *name);       // a name value() takes (for the check while a file is read)
    // a relation or method, them.opinion(me) - 0: name is none (v NULL: only that checked, na the arguments)
    int (*method)(struct xctx *c, const char *name, const struct xval *a, int na, struct xval *v);
    int (*object)(int n, struct xval *v);      // object n as a value (a script's player(n)): 0 none
    int (*constant)(const char *name, struct xval *v);   // 0 none, 1 a constant, 2 one that stays (kept in the tree)
    int (*rnd_show)(int n);                    // 0 <= x < n: the display generator (what is only shown)
    int (*rnd_decide)(int n);                  // the decisions' generator (c->game: every PC draws alike)
    void (*scan_functions)(void);              // every function file read for its functions (x_fn_scan)
    const char *(*placeholders)(const char *file, const char *sec, const char *key);   // the code's, " a b ", or NULL
    int (*command)(const char *name);          // a command a script may call as a function
    // a record of the files the program reads itself (lines, personality ...) - not a name of the file's: every
    // other record is one, its members name.key
    int (*code_record)(const char *name);
    const struct x_kind *(*kind)(int k);   // kind k (1..), NULL: none
    int (*root_kind)(const char *name);    // a name of its own that is an object (game, them ...): its kind, 0 none
    int (*fn_kind)(const char *name);      // what a function of its own gives (player(n): a player), X_ANY: not known
    // o.name, o.name(a ...) worked out - its kind's member (the language checked it is one, and the arguments' count)
    int (*member)(struct xctx *c, const struct xval *o, const char *name, const struct xval *a, int na, struct xval *v);
    // o.name = v (a member it may set - checked): 0 not set (the mistake told)
    int (*set)(struct xctx *c, const struct xval *o, const char *name, const struct xval *v);
    int (*const_type)(const char *name);   // a constant's type (unit.x: XT_UTYPE, epoch.middle: XT_EPOCH), X_ANY not known
    int (*name_type)(const char *name);    // a name of its own that is a value (minutes, epoch, opinion): its type
};
internal int xh_value(struct xctx *c, const char *name, struct xval *v) { (void)c; (void)name; (void)v; return 0; }
internal int xh_name(const char *name) { (void)name; return 0; }
internal int xh_method(struct xctx *c, const char *name, const struct xval *a, int na, struct xval *v) {
    (void)c; (void)name; (void)a; (void)na; (void)v; return 0;
}
internal int xh_object(int n, struct xval *v) { (void)n; (void)v; return 0; }
internal int xh_constant(const char *name, struct xval *v) { (void)name; (void)v; return 0; }
internal int xh_rnd(int n) { (void)n; return 0; }
internal void xh_scan(void) {}
internal const char *xh_placeholders(const char *file, const char *sec, const char *key) { (void)file; (void)sec; (void)key; return NULL; }
internal const struct x_kind *xh_kind(int k) { (void)k; return NULL; }
internal int xh_member(struct xctx *c, const struct xval *o, const char *name, const struct xval *a, int na, struct xval *v) {
    (void)c; (void)o; (void)name; (void)a; (void)na; (void)v; return 0;
}
internal int xh_set(struct xctx *c, const struct xval *o, const char *name, const struct xval *v) {
    (void)c; (void)o; (void)name; (void)v; return 0;
}
internal struct x_host g_x_host = { xh_value, xh_name, xh_name, xh_method, xh_object, xh_constant, xh_rnd, xh_rnd, xh_scan,
                                  xh_placeholders, xh_name, xh_name, xh_kind, xh_name, xh_name, xh_member, xh_set, xh_name,
                                  xh_name };
