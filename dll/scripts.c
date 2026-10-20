// SCRIPTS (autotest setup=<file>[;<file>...]): scenario scripts in the text language (expr.c; docs/scripts.md) -
// statements one a line, top to bottom, from 2 s game time: the pipe's commands as functions (spawn(3,
// unit.b_guard_tower_bronze, 51, 44), stats("engage", ...)), variables, functions ((i) => ...), and the waiting ones:
// wait(20s), at(19.5min, () => ...), every(20s, from, until, (run) => ...), for(1, 5, (i) => ...). Each script runs in
// a fiber of its own on the game's thread: waiting hands the turn back to the game until that game second, so the
// game never stops for a script and everything a script reads is the game as it is. Several scripts run side by side,
// each in turn, always in the order given (the same game twice the same). Its values live in a region of its own.
// A script is checked when it is read (syntax, unknown names, functions and their arguments, a variable of the code
// around changed in a function): a mistake there and it doesn't run at all; a mistake while it runs (a command's
// error, "a" * 2) ends it - both logged "# setup: file:line: ...". Every command is logged as before:
// "# setup: <command> -> <reply>". script dryrun <file> <out> (a pipe command) runs one without the game: the time
// jumps to wherever it waits, every command is written to <out> with its game time (ms) instead of being run.
// Included by eeaimod.c after expr.c and control.c.
#define SCRIPTS_MAX 4
struct script {
    char path[MAX_PATH];
    char *text;
    struct xunit u;
    LPVOID fiber;
    uint32_t wake;          // game ms it waits for
    int done;
    struct xregion reg;     // its values
};
static struct script g_scripts[SCRIPTS_MAX];
static int g_nscripts = -1;
static LPVOID g_main_fiber;
static struct script *g_cur_script;
static uint32_t g_script_now;
static double script_now(void) { return g_script_now / 1000.0; }
static void script_wait(double until) {   // in the script's fiber: the game goes on until that game second
    struct script *s = g_cur_script;
    s->wake = (uint32_t)(until * 1000.0 + 0.5);
    SwitchToFiber(g_main_fiber);
}
static void script_command(const char *cmd, char *reply, int cap) {
    (void)cap;   // execute's replies are as long as the pipe's (16384)
    execute(cmd, reply);
    char *nl = strchr(reply, '\n'), *rest = nl ? nl + 1 : NULL;
    if (!g_log) return;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# setup: %s -> %.*s\n", cmd, nl ? (int)(nl - reply) : (int)strlen(reply), reply);
    while (rest && *rest) {   // further lines of a multi-line reply, as comments
        nl = strchr(rest, '\n');
        fprintf(g_log, "#   %.*s\n", nl ? (int)(nl - rest) : (int)strlen(rest), rest);
        rest = nl ? nl + 1 : NULL;
    }
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}
static char *script_read(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *t = n >= 0 ? malloc((size_t)n + 1) : NULL;
    if (t && fread(t, 1, (size_t)n, f) != (size_t)n) { free(t); t = NULL; }
    fclose(f);
    if (t) t[n] = 0;
    return t;
}
// read and check a script: 0 and *why / *line - not run
static int script_load(const char *path, char **text, struct xunit *u, const char **why, int *line) {
    static char msg[160];
    *text = script_read(path);
    if (!*text) { *why = "can't be read"; *line = 0; return 0; }
    *u = x_parse(*text, 1);
    if (!u->ok) { *why = u->why ? u->why : "not a script (a statement that is none, or nested too deep)"; *line = u->line; return 0; }
    struct xctx c = { 0 };
    c.found = 1;
    struct xcheck k = { &c, 1, NULL, 0, "" };
    struct xscope sc = { NULL, { 0 }, 0 };
    const char *f0 = g_x_check_file, *ph0 = g_x_check_ph, *par0 = g_x_check_par;
    const char *base = strrchr(path, '\\') ? strrchr(path, '\\') + 1 : strrchr(path, '/') ? strrchr(path, '/') + 1 : path;
    g_x_check_file = base; g_x_check_ph = NULL; g_x_check_par = NULL;
    int ok = x_check(&k, u->root, &sc);
    g_x_check_file = f0; g_x_check_ph = ph0; g_x_check_par = par0;
    if (!ok) { lstrcpynA(msg, k.why ? k.why : "a mistake", sizeof msg); *why = msg; *line = k.line; return 0; }
    return 1;
}
// a script's statements, top to bottom (its top frame: the file's own names); a mistake ends it
static int script_run(const char *path, const struct xunit *u) {
    struct xctx c = { 0 };
    c.found = 1;
    c.script = 1;
    struct xframe *top = calloc(1, sizeof *top);
    if (!top) return 0;
    struct xval rv;
    struct xe e = { &c, 0, NULL, top, 0, &rv, NULL };
    c.fpn = top->name; c.fpv = top->val; c.fnp = 0;
    g_x_last_fail[0] = 0;
    x_exec(&e, u->root);
    if (e.err && g_log) {
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# setup: %s:%d: stopped - %s\n", path, e.err_at ? e.err_at->line : 0,
                g_x_last_fail[0] ? g_x_last_fail : e.why ? e.why : "not worked out");
        fflush(g_log);
        LeaveCriticalSection(&g_lock);
    }
    free(top);
    return !e.err;
}
static void WINAPI script_fiber(void *arg) {
    struct script *s = arg;
    script_run(s->path, &s->u);
    log_line("# setup: %s: done%.0d\n", s->path, 0);
    s->done = 1;
    for (;;) SwitchToFiber(g_main_fiber);
}
// every game tick from 2 s on (autotest_tick): each script that waits no longer goes on until it waits again
static void run_setup(uint32_t now) {
    if (g_nscripts < 0) {
        g_nscripts = 0;
        for (const char *p = g_setup_path; *p && g_nscripts < SCRIPTS_MAX; ) {   // setup=a.eeai;b.eeai (or ,): side by side
            int n = (int)strcspn(p, ",;");
            struct script *s = &g_scripts[g_nscripts];
            memset(s, 0, sizeof *s);
            lstrcpynA(s->path, p, n + 1 < MAX_PATH ? n + 1 : MAX_PATH);
            p += n + (p[n] == ',' || p[n] == ';');
            const char *why;
            int line;
            if (!script_load(s->path, &s->text, &s->u, &why, &line)) {
                if (g_log) { EnterCriticalSection(&g_lock); fprintf(g_log, "# setup: %s:%d: %s - not run\n", s->path, line, why); fflush(g_log); LeaveCriticalSection(&g_lock); }
                continue;
            }
            log_line("# setup: %s%.0d\n", s->path, 0);
            if (!g_main_fiber) g_main_fiber = IsThreadAFiber() ? GetCurrentFiber() : ConvertThreadToFiber(NULL);
            s->fiber = g_main_fiber ? CreateFiber(1 << 20, script_fiber, s) : NULL;
            if (!s->fiber) { log_line("# setup: %s: no fiber - not run%.0d\n", s->path, 0); continue; }
            s->reg.busy = 1;   // its evaluations never start its region afresh
            g_nscripts++;
        }
    }
    g_script_now = now;
    for (int i = 0; i < g_nscripts; i++) {
        struct script *s = &g_scripts[i];
        if (s->done || !s->fiber || now < s->wake) continue;
        struct xregion *r0 = g_xr;
        g_xr = &s->reg;
        g_x_hooks.now = script_now; g_x_hooks.wait = script_wait; g_x_hooks.command = script_command;
        g_cur_script = s;
        SwitchToFiber(s->fiber);
        g_cur_script = NULL;
        memset(&g_x_hooks, 0, sizeof g_x_hooks);
        g_xr = r0;
    }
}
// THE DRY RUN (script dryrun <file> <out>): the script without the game - waiting moves the clock (from 2 s), the
// commands are written to <out> ("<ms> <command>") and answered "ok" (tests: a converted scenario issues the same
// commands at the same times as before)
static double g_dry_now;
static FILE *g_dry_out;
static double dry_now(void) { return g_dry_now; }
static void dry_wait(double until) { if (until > g_dry_now) g_dry_now = until; }
static void dry_command(const char *cmd, char *reply, int cap) {
    fprintf(g_dry_out, "%u %s\n", (unsigned)(g_dry_now * 1000.0 + 0.5), cmd);
    lstrcpynA(reply, "ok", cap);
}
static void cmd_script(char *out, const char *args) {
    char file[MAX_PATH], dst[MAX_PATH];
    if (sscanf(args, "dryrun %259s %259s", file, dst) != 2) { REPLY("error: usage: script dryrun <file> <out>\n"); return; }
    char *text;
    struct xunit u;
    const char *why;
    int line;
    if (!script_load(file, &text, &u, &why, &line)) { REPLY("error: %s:%d: %s\n", file, line, why); return; }
    g_dry_out = fopen(dst, "wb");
    if (!g_dry_out) { REPLY("error: can't write %s\n", dst); free(text); return; }
    struct xregion reg = { 0 }, *r0 = g_xr;
    reg.busy = 1;
    g_xr = &reg;
    g_dry_now = 2.0;
    __typeof__(g_x_hooks) h0 = g_x_hooks;   // (a script may run this: its own are back after)
    g_x_hooks.now = dry_now; g_x_hooks.wait = dry_wait; g_x_hooks.command = dry_command;
    int ok = script_run(file, &u);
    g_x_hooks = h0;
    g_xr = r0;
    x_region_free(&reg);
    fclose(g_dry_out);
    g_dry_out = NULL;
    free(text);   // (its tree stays: a dry run is a test's)
    REPLY(ok ? "ok: %s run dry to %s\n" : "error: %s stopped (the log has where) - what it did is in %s\n", file, dst);
}
