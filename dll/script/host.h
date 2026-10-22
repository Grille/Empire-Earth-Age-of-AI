// THE HOST: what the language asks the program it runs in - the names and constants of its world, its objects (a value
// that is one of them: obj), the relations between them (them.kills(me)), its two random generators, the files its
// functions come from, the commands a script may call. The language knows none of them itself; the game's are set by
// eeaimod.c x_host_install. Until then (and for a program without them) every answer is "none".
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
internal struct x_host g_x_host = { xh_value, xh_name, xh_name, xh_method, xh_object, xh_constant, xh_rnd, xh_rnd, xh_scan,
                                  xh_placeholders, xh_name };
