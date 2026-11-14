// SCRIPTS (autotest setup=<file>[;<file>...]): scenario scripts in the text language (expr.c; docs/scripts.md) -
// statements one a line, top to bottom, from 2 s game time: the pipe's commands as functions (spawn(3,
// unit.b_guard_tower_bronze, 51, 44), stats("engage", ...)), variables (let), functions ((i) => ...), if, while, for,
// and the waiting ones: wait(20s), wait_until(19.5min), every(20s, from, until, (run) => ...). Each script runs in
// a fiber of its own on the game's thread: waiting hands the turn back to the game until that game second, so the
// game never stops for a script and everything a script reads is the game as it is. Several scripts run side by side,
// each in turn, always in the order given (the same game twice the same). Its values live in a region of its own.
// A script is checked when it is read (syntax, unknown names, functions and their arguments, a variable of the code
// around changed in a function): a mistake there and it doesn't run at all; a mistake while it runs (a command's
// error, "a" * 2) ends it - both logged "# setup: file:line: ...". Every command is logged as before:
// "# setup: <command> -> <reply>". script dryrun <file> <out> (a pipe command) runs one without the game: the time
// jumps to wherever it waits, every command is written to <out> with its game time (ms) instead of being run.
// The player's own script of every game, setup.eeai beside ageofai.ini with its hooks by the kind of game, runs here
// too - in any game, not a test's only (SETUP.EEAI below).
// Included by eeaimod.c after expr.c and control.c.
#include "test/cmd_verbs.h"
internal int x_command_verb(const char *name) {   // a pipe command (control.c execute) - a script calls it as a function
    for (size_t i = 0; i < sizeof k_cmd_verbs / sizeof *k_cmd_verbs; i++) if (!strcmp(k_cmd_verbs[i], name)) return 1;
    return 0;
}
#define SCRIPTS_MAX 4
struct script {
    char path[MAX_PATH];
    char *text;
    struct xunit u;
    LPVOID fiber;
    uint32_t wake;          // game ms it waits for
    int done;
    int hooks;              // setup.eeai: the hooks called after its top, a bit each (SETUP.EEAI below)
    int safe;               // its turns are the safe point's (safe_turn): setup.eeai, a saved game's scripts
    struct xframe *top;     // its top frame once it runs: the file's own names (script_run)
    struct xposs *plan;     // a saved game's that waited: where it stood, to go on there (expr.c WHERE A SCRIPT STANDS)
    struct xposs pos;       // where it stands now (g_xp while it runs)
    struct xregion reg;     // its values
};
internal struct script g_scripts[SCRIPTS_MAX];
internal int g_nscripts = -1;
internal struct script g_own;   // setup.eeai of this game (SETUP.EEAI below; no text: none)
internal int g_own_fresh;       // this game's clock seen before 2 s (safe_turns)
internal int g_scripts_restored;   // g_scripts are a saved game's (THE SCRIPTS IN A SAVED GAME), not a test's setup=
internal LPVOID g_main_fiber;
internal struct script *g_cur_script;
internal uint32_t g_script_now;
internal double script_now(void) { return g_script_now / 1000.0; }
internal void script_wait(double until) {   // in the script's fiber: the game goes on until that game second
    struct script *s = g_cur_script;
    s->wake = (uint32_t)(until * 1000.0 + 0.5);
    SwitchToFiber(g_main_fiber);
}
internal void script_command(const char *cmd, char *reply, int cap) {
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
internal int script_did(const char *what) {   // an effect (relations.c x_did): logged, then done
    log_line("# setup: %s\n", what);
    return 0;
}
internal char *script_read(const char *path) {
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
// SETUP.EEAI'S HOOKS: functions declared at its top by these names, no parameters, statements - on_game() { ... }.
// Called after the file's top has run, on_game first, then the one of the game's type (own_new_game picks). Another
// on_... name there is a mistake (a hook's name mistyped would never run, and nothing would say so), one not of that
// form too. 0 and *why / *line; *have: a bit for each the file has
enum { HOOK_GAME, HOOK_SKIRMISH, HOOK_SCENARIO, HOOK_CAMPAIGN, HOOK_N };
internal const char *const k_setup_hooks[HOOK_N] = { "on_game", "on_skirmish", "on_scenario", "on_campaign" };
internal int setup_hooks(const struct xunit *u, int *have, const char **why, int *line) {
    static char msg[160];
    *have = 0;
    for (int i = 0; u->root && i < u->root->n_items; i++) {
        const struct xnode *s = u->root->items[i];
        if (s->k != XN_LET || strncmp(s->s, "on_", 3)) continue;
        int k = 0;
        while (k < HOOK_N && strcmp(k_setup_hooks[k], s->s)) k++;
        if (k == HOOK_N) snprintf(msg, sizeof msg, "%.40s: no such hook - on_game, on_skirmish, on_scenario, on_campaign", s->s);
        else if (s->op != 'D' || !s->a || s->a->k != XN_LAMBDA || s->a->n_items || !s->a->a || s->a->a->k != XN_BLOCK)
            snprintf(msg, sizeof msg, "a hook takes nothing and runs statements: %s() { ... }", k_setup_hooks[k]);
        else { *have |= 1 << k; continue; }
        *why = msg;
        *line = s->line;
        return 0;
    }
    return 1;
}
// a script's text compiled and checked (path: its file, for the messages and setup.eeai's rules): 0 and *why / *line
internal int script_compile(const char *path, const char *text, struct xunit *u, const char **why, int *line) {
    static char msg[200];
    *u = x_parse(text, 1);
    if (!u->ok) { *why = u->why ? u->why : "not a script (a statement that is none, or nested too deep)"; *line = u->line; return 0; }
    struct xctx c = { 0 };
    c.found = 1;
    struct xcheck k = { &c, 1, NULL, 0, "", 0, 0, { 0 }, { 0 }, 0, 0, 0, 0 };
    struct xscope sc = { NULL, { 0 }, 0, { 0 }, 0, { 0 }, { 0 } };
    const char *f0 = g_x_check_file, *ph0 = g_x_check_ph, *par0 = g_x_check_par;
    const char *base = strrchr(path, '\\') ? strrchr(path, '\\') + 1 : strrchr(path, '/') ? strrchr(path, '/') + 1 : path;
    g_x_check_file = base; g_x_check_ph = NULL; g_x_check_par = NULL;
    int ok = x_check(&k, u->root, &sc);
    g_x_check_file = f0; g_x_check_ph = ph0; g_x_check_par = par0;
    if (!ok) { lstrcpynA(msg, k.why ? k.why : "a mistake", sizeof msg); *why = msg; *line = k.line; return 0; }
    int have;
    if (!lstrcmpiA(base, "setup.eeai") && !setup_hooks(u, &have, why, line)) return 0;   // (the checker reads it so too)
    return 1;
}
// read and check a script: 0 and *why / *line - not run
internal int script_load(const char *path, char **text, struct xunit *u, const char **why, int *line) {
    *text = script_read(path);
    if (!*text) { *why = "can't be read"; *line = 0; return 0; }
    return script_compile(path, *text, u, why, line);
}
// a script's statements, top to bottom (its top frame: the file's own names); a mistake ends it. The top frame stays
// (keep): its triggers' and after's functions read its names as they are when they run, also after it ended.
// hooks: setup.eeai's to call then (bits of k_setup_hooks), each a function of its top frame called as a statement of
// the file would call it - a mistake in one ends the script at the hook's own line. s: the script (NULL: a dry run's) -
// its top frame kept for the saved games, and a saved game's that waited goes on where it stood (its plan: the top
// frame it had, then its statements or the hook it was in found again - expr.c WHERE A SCRIPT STANDS)
internal int script_run(const char *path, const struct xunit *u, int hooks, struct script *s) {
    struct xctx c = { 0 };
    c.found = 1;
    c.script = 1;
    struct xposs *plan = s && s->top ? s->plan : NULL;
    struct xframe *top = plan ? s->top : calloc(1, sizeof *top);
    if (!top) return 0;
    top->keep = 1;
    if (s) s->top = top;
    struct xval rv;
    struct xe e = { &c, 0, NULL, top, 0, &rv, NULL, 0, 0, 0 };
    c.fpn = top->name; c.fpv = top->val; c.fnp = top->n;
    g_x_last_fail[0] = 0;
    int k0 = 0, in_hook = plan && plan->n && plan->e[0].k == XP_HOOK;
    if (plan) {
        int line;
        x_stack_resumable(plan, &line);
        log_line("# setup: %s: goes on where it was saved (line %d)\n", path, line);
        plan->at = 0;
        g_xres = plan;
        if (in_hook) k0 = plan->e[0].i;
    }
    if (!in_hook) x_exec(&e, u->root);
    for (int k = k0; k < HOOK_N && !e.err; k++) {
        int j = 0;
        if (!((hooks >> k) & 1)) continue;
        while (j < top->n && strcmp(top->name[j], k_setup_hooks[k])) j++;
        if (j == top->n || top->val[j].kind != XV_FN) continue;   // (setup_hooks saw it declared)
        struct xval fv = top->val[j], r;
        struct xpos *xp = xp_push(XP_HOOK, NULL);
        if (xp) xp->i = k;
        if (g_xres) {
            if (xres_next(XP_HOOK)) xres_call(&e, &r);
            else { g_xres = NULL; x_fail(&e, "the saved script's place can't be found again (a hook)"); }
        } else {
            log_line("# setup: %s: %s()\n", path, k_setup_hooks[k]);
            g_xp_call_direct = 1;
            x_call_value(&e, &fv, NULL, 0, &r);
        }
        xp_pop(xp);
    }
    g_xres = NULL;
    if (e.err && g_log) {
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# setup: %s:%d: stopped - %s\n", path, e.err_at ? e.err_at->line : 0,
                g_x_last_fail[0] ? g_x_last_fail : e.why ? e.why : "not worked out");
        fflush(g_log);
        LeaveCriticalSection(&g_lock);
    }
    if (e.err && g_cur_script == &g_own)   // the player's own file: told on the screen too, as a text's mistake
        x_error("setup.eeai:%d: stopped - %s", e.err_at ? e.err_at->line : 0, g_x_last_fail[0] ? g_x_last_fail : e.why ? e.why : "not worked out");
    return !e.err;
}
internal void WINAPI script_fiber(void *arg) {
    struct script *s = arg;
    script_run(s->path, &s->u, s->hooks, s);
    log_line("# setup: %s: done\n", s->path);
    s->done = 1;
    for (;;) SwitchToFiber(g_main_fiber);
}
internal void scripts_load(void) {   // setup=a.eeai;b.eeai: each read and checked, once (its fiber: run_setup)
    if (g_nscripts < 0) {
        g_nscripts = 0;
        x_triggers_reset();
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
            log_line("# setup: %s\n", s->path);
            s->reg.busy = 1;   // its evaluations never start its region afresh
            g_nscripts++;
        }
    }
}
// THE GAME'S START (eeaimod.c hook_game_start, before the personalities are planned; scripts_new_game below): the
// scripts read - a test's once, setup.eeai for this game -, and in a new game their game.setup blocks run
// (game_script/relations.c SETUP) - a loaded game's ran when it began (the setup's mode 2; a test's random map is a
// load of newgame.ees, its mode 0 again by now - randmap.c). A mistake there ends that script
internal void script_setup_blocks(struct script *s) {
    struct xctx c = { 0 };
    c.found = 1;
    c.script = 1;
    struct xregion *r0 = g_xr;
    g_xr = &s->reg;
    g_x_last_fail[0] = 0;
    int ok = x_setup_run(&s->u, &c);
    g_xr = r0;
    if (ok) return;
    s->done = 1;
    log_line("# setup: %s: game.setup stopped - %s\n", s->path, g_x_last_fail[0] ? g_x_last_fail : "not worked out");
    if (s == &g_own) x_error("setup.eeai: game.setup stopped - %s", g_x_last_fail[0] ? g_x_last_fail : "not worked out");
}
// SETUP.EEAI (beside ageofai.ini; config/setup.eeai is the shipped one): the player's own script of every game on this
// PC - a script as a test's (docs/scripts.md: statements, waits, triggers, timers, the pipe's commands), read anew at
// each game's start, with a hook for every kind of game (SETUP.EEAI'S HOOKS above):
//   on_game()       every new game, before the game type's own
//   on_skirmish()   a random-map game (the setup's mode 0)
//   on_campaign()   a campaign's scenario (mode 4)
//   on_scenario()   a scenario (1; the editor's test play, 3, is one)
// Its top runs first (its lets are the hooks' shared names; its game.setup blocks before the game starts, as any
// script's), then the hooks - in its fiber from 2 s game time. Its turns are the safe point's (script_triggers, every
// pass of the simulation's loop): a game without a computer player has them too, and its commands run where the
// pipe's do; a test's scripts keep the AI update's.
// No hook is called in a saved game that is loaded (they ran when the game began; the save carries on what they
// started: THE SCRIPTS IN A SAVED GAME), none in a multiplayer game (each PC would run its own
// file: a desync, as a console command there), no turn while the scenario editor is on. No file, no script.
internal void script_own_path(char path[MAX_PATH]) {
    lstrcpynA(path, g_aimod_ini, MAX_PATH - 16);
    char *slash = strrchr(path, '\\');
    lstrcpyA(slash ? slash + 1 : path, "setup.eeai");
}
// a script over where it is - its fiber (but the one whose turn it is: it can't end itself), its values, its text and
// top frame (the triggers that read them are reset with it); its tree stays, as every compiled source
internal void script_end(struct script *s) {
    if (s->fiber && s != g_cur_script) DeleteFiber(s->fiber);
    x_region_free(&s->reg);
    free(s->text);
    free(s->top);
    free(s->plan);
    memset(s, 0, sizeof *s);
}
internal void scripts_restored_end(void) {   // a saved game's scripts (THE SCRIPTS IN A SAVED GAME): over with their game
    if (!g_scripts_restored) return;
    for (int i = 0; i < SCRIPTS_MAX; i++) script_end(&g_scripts[i]);
    g_nscripts = -1;
    g_scripts_restored = 0;
}
internal void own_new_game(ee_setup *setup) {
    // started by a command of its own (a load from a hook: this is its fiber), or beside a test's scripts in a later
    // game of their process (they go on over a load, their triggers with them): it goes on as it is
    if (g_cur_script == &g_own || (g_own.text && g_nscripts > 0)) return;
    script_end(&g_own);   // the last game's: over where it waited
    g_script_now = 0;
    g_own_fresh = 0;
    if (g_nscripts <= 0) { x_triggers_reset(); x_units_reset(); }   // its triggers and timers, the units it spawned
    int mode = setup_get_mode(setup), humans = 0, have = 0, line;
    if (mode == 2) return;
    script_own_path(g_own.path);
    if (GetFileAttributesA(g_own.path) == INVALID_FILE_ATTRIBUTES) return;
    for (int s = 0; s < 16; s++) humans += setup_slot_get_type(setup_slot_at(setup, s)) == 1;
    if (humans > 1) { log_line("# setup: %s: not in a multiplayer game (each PC would run its own)\n", g_own.path); return; }
    const char *why;
    if (!script_load(g_own.path, &g_own.text, &g_own.u, &why, &line)) {
        log_line("# setup: %s:%d: %s - not run\n", g_own.path, line, why);
        x_error("setup.eeai:%d: %s - not run", line, why);
        free(g_own.text);
        g_own.text = NULL;
        return;
    }
    int type = mode == 0 ? HOOK_SKIRMISH : mode == 4 ? HOOK_CAMPAIGN : HOOK_SCENARIO;
    setup_hooks(&g_own.u, &have, &why, &line);
    g_own.hooks = have & (1 << HOOK_GAME | 1 << type);
    g_own.reg.busy = 1;   // its evaluations never start its region afresh
    g_own.safe = 1;
    log_line("# setup: %s (%s: the setup's mode %d)\n", g_own.path, k_setup_hooks[type], mode);
}
internal void scripts_new_game(ee_setup *setup) {
    persona_setup_clear();
    if (g_cur_script != &g_own) scripts_restored_end();
    if (g_autotest && g_setup_path[0]) scripts_load();
    own_new_game(setup);
    if (setup_get_mode(setup) == 2) return;
    g_x_setup = setup;
    if (g_own.text) script_setup_blocks(&g_own);   // first: a test's choice is the later one
    for (int i = 0; i < g_nscripts; i++) script_setup_blocks(&g_scripts[i]);
    g_x_setup = NULL;
}
// The safe point's turns (script_triggers): setup.eeai's, and a saved game's scripts' (restored: whoever ran them when
// it was saved) - from 2 s on each goes on until it waits again, as a test's script in run_setup. Only once this
// game's clock was seen before 2 s (a later game of the session: while it loads the last game's time may still stand
// there; a loaded game's scripts are fresh as they are restored). The thread's fiber anew each turn: the simulation's
// thread of a later game may be another
internal void safe_turn(struct script *s, uint32_t now) {
    if (!s->text || s->done || !s->safe || now < s->wake) return;
    g_main_fiber = IsThreadAFiber() ? GetCurrentFiber() : ConvertThreadToFiber(NULL);
    if (!s->fiber && g_main_fiber) s->fiber = CreateFiber(1 << 20, script_fiber, s);
    if (!s->fiber) { s->done = 1; log_line("# setup: %s: no fiber - not run\n", s->path); return; }
    struct xregion *r0 = g_xr;
    g_xr = &s->reg;
    g_script_now = now;
    g_x_hooks.now = script_now; g_x_hooks.wait = script_wait; g_x_hooks.command = script_command; g_x_hooks.did = script_did;
    g_x_safe = 1;   // (its spawns made at once, as a trigger's: units.c)
    g_cur_script = s;
    g_xp = &s->pos;
    SwitchToFiber(s->fiber);
    g_xp = NULL;
    g_cur_script = NULL;
    g_x_safe = 0;
    memset(&g_x_hooks, 0, sizeof g_x_hooks);
    g_xr = r0;
}
internal void safe_turns(uint32_t now) {
    if (now < 2000) { g_own_fresh = 1; return; }
    if (!g_own_fresh || !game_map() || editor_is_on()) return;
    safe_turn(&g_own, now);
    for (int i = 0; i < g_nscripts; i++) safe_turn(&g_scripts[i], now);
}
// every game tick from 2 s on (autotest_tick): each script that waits no longer goes on until it waits again
internal void run_setup(uint32_t now) {
    scripts_load();
    g_script_now = now;
    for (int i = 0; i < g_nscripts; i++) {   // their fibers on this thread (the game's start may be another's)
        struct script *s = &g_scripts[i];
        if (s->done || s->fiber || s->safe) continue;
        if (!g_main_fiber) g_main_fiber = IsThreadAFiber() ? GetCurrentFiber() : ConvertThreadToFiber(NULL);
        s->fiber = g_main_fiber ? CreateFiber(1 << 20, script_fiber, s) : NULL;
        if (!s->fiber) { s->done = 1; log_line("# setup: %s: no fiber - not run\n", s->path); }
    }
    for (int i = 0; i < g_nscripts; i++) {
        struct script *s = &g_scripts[i];
        if (s->done || !s->fiber || s->safe || now < s->wake) continue;
        struct xregion *r0 = g_xr;
        g_xr = &s->reg;
        g_x_hooks.now = script_now; g_x_hooks.wait = script_wait; g_x_hooks.command = script_command; g_x_hooks.did = script_did;
        g_cur_script = s;
        g_xp = &s->pos;
        SwitchToFiber(s->fiber);
        g_xp = NULL;
        g_cur_script = NULL;
        memset(&g_x_hooks, 0, sizeof g_x_hooks);
        g_xr = r0;
    }
}
// the triggers' turn: at every safe point between the simulation's steps (control.c hook_sim_receive, where the pipe's
// commands run too) - several a game second, so their 500 ms checks hold; the scripts' own turns (an AI's update,
// about one a game second) would make it one a second. No waiting there (a trigger's run has after's). The scripts'
// wins and defeats are carried out here too (relations.c x_effects_flush: the game's resign path wants this point), and
// their spawned units made
internal void script_triggers(uint32_t now) {
    x_effects_flush();
    safe_turns(now);
    if ((g_nscripts <= 0 && !g_own.text) || now < 2000) return;
    if (now > g_script_now) g_script_now = now;
    g_x_hooks.now = script_now; g_x_hooks.command = script_command; g_x_hooks.did = script_did;
    g_x_safe = 1;   // (a trigger's spawns made at once: units.c)
    x_triggers_tick(g_script_now / 1000.0);
    g_x_safe = 0;
    memset(&g_x_hooks, 0, sizeof g_x_hooks);
}
// THE SCRIPTS IN A SAVED GAME (persist.c's block "scripts", in the save's .ageofai file): a save carries its scripts -
// each one's source as it was read (a save doesn't depend on what setup.eeai or a scenario's file say later), whether
// it has run, and what its functions go on with: its top frame's names, the triggers, timers and after's with the
// frames they keep, the dicts (expr.c SAVED STATE), the spawned units' handles (game_script/units.c). A loaded game
// (hook_game_start's persist_load, after scripts_new_game) has them as they were - the timers at their times, the
// triggers with their conditions' state, the variables' values - compiled anew from the saved source.
// A script that waits (wait, wait_until, every) as the game is saved is saved with where it stands - the blocks and
// statements, the loops, the functions it is in with their names - and goes on there after the load (expr.c WHERE A
// SCRIPT STANDS). One that can't be found again - it waits below an expression (g(f()) + 1 where f waits), or holds
// the moment for a spawned unit - doesn't go on: told in the log as it is saved and loaded and, setup.eeai's, on the
// screen; what it started before does. One that hasn't begun (a save in the first 2 s) starts at its top.
// A test that runs its own scripts (setup=) keeps to them: the save's are left out there. Elsewhere the save's test
// scripts come back beside its setup.eeai, their turns the safe point's (safe_turns).
#define SCRIPT_STATE_FORMAT 2   // 2: with where a waiting script stands
enum { SS_NEW, SS_WAITING, SS_DONE };
internal uint8_t *scripts_state_save(uint32_t *size) {
    struct script *sc[1 + SCRIPTS_MAX];
    const struct xnode *roots[1 + SCRIPTS_MAX];
    struct xframe *tops[1 + SCRIPTS_MAX];
    const struct xposs *stacks[1 + SCRIPTS_MAX];
    int n = 0, lost = 0, waiting = 0, stuck = 0;
    *size = 0;
    if (g_own.text) sc[n++] = &g_own;
    for (int i = 0; i < g_nscripts; i++) sc[n++] = &g_scripts[i];
    if (!n) return NULL;
    struct pbuf o = { 0 };
    pb_int(&o, SCRIPT_STATE_FORMAT);
    pb_int(&o, (int32_t)g_script_now);
    pb_int(&o, n);
    for (int i = 0; i < n; i++) {
        struct script *s = sc[i];
        uint8_t kind = s != &g_own, state = s->done ? SS_DONE : s->fiber || s->plan ? SS_WAITING : SS_NEW;
        int line = 0;
        pb_put(&o, &kind, 1);
        pb_put(&o, &state, 1);
        pb_str(&o, s->path);
        pb_text(&o, s->text);
        pb_int(&o, (int32_t)s->wake);
        pb_int(&o, s->hooks);
        roots[i] = s->u.root;
        tops[i] = s->top;
        stacks[i] = NULL;
        if (state != SS_WAITING) continue;
        waiting++;
        if (!s->fiber) stacks[i] = s->plan;   // a loaded game's, saved again before its turn came
        else if (s != g_cur_script && x_stack_resumable(&s->pos, &line)) stacks[i] = &s->pos;
        if (stacks[i]) continue;
        stuck++;
        log_line("# setup: %s:%d: can't be found again after a load as the game is saved now - %s; what it started goes on\n", s->path, line,
                 s == g_cur_script ? "it is its own turn (a save of its own)" : "it waits below an expression (a waiting function is called as a statement of its own), or holds for a spawned unit");
    }
    x_state_save(&o, roots, tops, stacks, n, &lost);
    x_units_state_save(&o);
    log_line("# persist: scripts: %d (%d waiting, %d of them not to be found again), %d triggers and timers, %d after's, %d spawned units' handles, %u bytes%s\n",
             n, waiting, stuck, g_xt_n, g_xa_n, g_xslot_n, o.n, lost ? " - a function of a text's placeholder left out" : "");
    if (o.bad) { free(o.b); return NULL; }
    *size = o.n;
    return o.b;
}
internal void scripts_state_drop(const char *why) {   // what was read so far undone: the game goes on without scripts
    script_end(&g_own);
    for (int i = 0; i < SCRIPTS_MAX; i++) script_end(&g_scripts[i]);
    g_nscripts = -1;
    g_scripts_restored = 0;
    x_triggers_reset();
    x_units_reset();
    log_line("# persist: the save's scripts can't be read (%s) - none restored\n", why);
}
internal void scripts_state_load(const uint8_t *p, uint32_t size) {
    struct pread r = { p, size, 0, 0 };
    if (g_nscripts > 0 || g_own.text || g_cur_script) {
        log_line("# persist: the save's scripts left out - %s\n", g_nscripts > 0 ? "this test runs its own (setup=)" : "a script of this session goes on");
        return;
    }
    if (pr_int(&r) != SCRIPT_STATE_FORMAT) { log_line("# persist: the save's scripts are of another format - none restored\n"); return; }
    uint32_t now = (uint32_t)pr_int(&r);
    int n = pr_int(&r), nt = 0, waiting = 0, stuck = 0;
    if (r.bad || n < 1 || n > 1 + SCRIPTS_MAX) { log_line("# persist: the save's scripts are broken - none restored\n"); return; }
    struct script *sc[1 + SCRIPTS_MAX];
    const struct xnode *roots[1 + SCRIPTS_MAX];
    struct xframe *tops[1 + SCRIPTS_MAX] = { 0 };
    struct xregion *regs[1 + SCRIPTS_MAX];
    struct xposs *plans[1 + SCRIPTS_MAX] = { 0 };
    uint8_t states[1 + SCRIPTS_MAX] = { 0 };
    x_triggers_reset();
    x_units_reset();
    for (int i = 0; i < n; i++) {
        uint8_t kind = 0, state = 0;
        pr_get(&r, &kind, 1);
        pr_get(&r, &state, 1);
        char *path = pr_str(&r), *text = pr_text(&r);
        uint32_t wake = (uint32_t)pr_int(&r);
        int hooks = pr_int(&r), line = 0;
        struct script *s = !kind && !g_own.text ? &g_own : nt < SCRIPTS_MAX ? &g_scripts[nt] : NULL;
        const char *why = "";
        if (r.bad || !s || !path || !text) { free(path); free(text); scripts_state_drop("broken"); return; }
        if (s != &g_own) nt++;
        memset(s, 0, sizeof *s);
        lstrcpynA(s->path, path, MAX_PATH);
        free(path);
        s->text = text;
        if (!script_compile(s->path, text, &s->u, &why, &line)) {
            log_line("# persist: %s:%d: %s\n", s->path, line, why);
            scripts_state_drop("a script this version of the mod doesn't read");
            return;
        }
        s->done = state == SS_DONE;
        s->wake = wake;
        states[i] = state;
        s->hooks = hooks;
        s->safe = 1;
        s->reg.busy = 1;
        sc[i] = s; roots[i] = s->u.root; regs[i] = &s->reg;
    }
    if (nt) { g_nscripts = nt; g_scripts_restored = 1; }
    if (!x_state_load(&r, roots, tops, regs, plans, n)) { scripts_state_drop("broken, or another version's"); return; }
    for (int i = 0; i < n; i++) {
        struct script *s = sc[i];
        s->top = tops[i];
        s->plan = plans[i];
        if (states[i] != SS_WAITING) { free(s->plan); s->plan = NULL; continue; }
        waiting++;
        if (s->plan && s->top) continue;   // it goes on where it stood, at its time
        stuck++;
        s->done = 1;
        log_line("# setup: %s: was waiting where it can't be found again as this game was saved - it doesn't go on, what it started does\n", s->path);
        if (s == &g_own) x_error("setup.eeai: was waiting where it can't go on from as this game was saved (its timers and triggers do)");
    }
    x_units_state_load(&r);
    if (r.bad) { scripts_state_drop("broken"); return; }
    g_script_now = now;
    g_own_fresh = 1;
    log_line("# persist: scripts restored: %d (%d were waiting, %d of them not to be found again), %d triggers and timers, %d after's, %d spawned units' handles\n",
             n, waiting, stuck, g_xt_n, g_xa_n, g_xslot_n);
}
// THE DRY RUN (script dryrun <file> <out>): the script without the game - waiting moves the clock (from 2 s), the
// commands are written to <out> ("<ms> <command>") and answered "ok" (tests: a converted scenario issues the same
// commands at the same times as before). Its triggers have their turns on the way, every 500 ms up to where the
// script waits last (a script that tests triggers ends with a wait); they and its after's go when it is done
internal double g_dry_now;
internal FILE *g_dry_out;
internal double dry_now(void) { return g_dry_now; }
internal void dry_wait(double until) {
    for (double t = g_dry_now + 0.5; t <= until + 1e-9; t += 0.5) { g_dry_now = t; x_triggers_tick(t); }
    if (until > g_dry_now) g_dry_now = until;
}
internal void dry_command(const char *cmd, char *reply, int cap) {
    fprintf(g_dry_out, "%u %s\n", (unsigned)(g_dry_now * 1000.0 + 0.5), cmd);
    lstrcpynA(reply, "ok", cap);
}
internal int dry_did(const char *what) {   // an effect: written as a command is, not done
    fprintf(g_dry_out, "%u %s\n", (unsigned)(g_dry_now * 1000.0 + 0.5), what);
    return 1;
}
internal void cmd_script(char *out, const char *args) {
    char file[MAX_PATH], dst[MAX_PATH];
    if (!strncmp(args, "state", 5)) {   // the scripts saved and read back at once, as a save and its load do (a test) -
        // or into a file (state save <file>) and out of one (state load <file>: another game's, another process's)
        uint32_t n = 0;
        int to = sscanf(args, "state save %259s", file) == 1, from = !to && sscanf(args, "state load %259s", file) == 1;
        if (g_cur_script) { REPLY("error: script state: not from a script (its own turn can't end)\n"); return; }
        uint8_t *b = from ? (uint8_t *)script_read(file) : scripts_state_save(&n);
        if (!b) { REPLY(from ? "error: %s can't be read\n" : "error: no script to save\n", file); return; }
        if (to || from) {
            FILE *f = fopen(file, to ? "wb" : "rb");
            if (f && to) fwrite(b, 1, n, f);
            if (f && from) { fseek(f, 0, SEEK_END); n = (uint32_t)ftell(f); }
            if (f) fclose(f);
            if (to) { free(b); REPLY(f ? "ok: %u bytes written to %s\n" : "error: %u bytes, but can't write %s\n", n, file); return; }
        }
        script_end(&g_own);
        for (int i = 0; i < SCRIPTS_MAX; i++) script_end(&g_scripts[i]);
        if (g_nscripts > 0) g_nscripts = 0;
        g_scripts_restored = 0;
        scripts_state_load(b, n);
        free(b);
        REPLY("ok: %u bytes saved and read back - %d triggers and timers, %d after's (the log has the rest)\n", n, g_xt_n, g_xa_n);
        return;
    }
    if (sscanf(args, "dryrun %259s %259s", file, dst) != 2) { REPLY("error: usage: script dryrun <file> <out> | script state\n"); return; }
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
    g_x_hooks.now = dry_now; g_x_hooks.wait = dry_wait; g_x_hooks.command = dry_command; g_x_hooks.did = dry_did;
    static struct xtrig keep_t[XT_MAX];   // a game's own triggers set aside meanwhile: the dry run has none but its own
    static struct xafter keep_a[XA_MAX];
    int nt = g_xt_n, na = g_xa_n;
    double next = g_xt_next;
    memcpy(keep_t, g_xt, sizeof g_xt);
    memcpy(keep_a, g_xa, sizeof g_xa);
    g_xt_n = g_xa_n = 0;
    g_xt_next = 0;
    int ok = script_run(file, &u, 0, NULL);   // (setup.eeai run dry: its top, no hook - there is no game to tell its kind)
    memcpy(g_xt, keep_t, sizeof g_xt);
    memcpy(g_xa, keep_a, sizeof g_xa);
    g_xt_n = nt; g_xa_n = na; g_xt_next = next;
    g_x_hooks = h0;
    g_xr = r0;
    x_region_free(&reg);
    fclose(g_dry_out);
    g_dry_out = NULL;
    free(text);   // (its tree stays: a dry run is a test's)
    REPLY(ok ? "ok: %s run dry to %s\n" : "error: %s stopped (the log has where) - what it did is in %s\n", file, dst);
}
