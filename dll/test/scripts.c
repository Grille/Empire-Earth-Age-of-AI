// SCRIPTS (autotest setup=<file>[;<file>...]; setup.eeai): scripts in the text language (expr.c; docs/scripts.md) -
// statements one a line, top to bottom, from the game's start (THE SCRIPTS' START): the pipe's commands as functions (spawn(3,
// unit.b_guard_tower_bronze, 51, 44), stats("engage", ...)), variables (let), functions ((i) => ...), if, while, for,
// and the waiting ones: wait(20s), wait_until(19.5min), every(20s, from, until, (run) => ...). A script is compiled
// to instructions and run by a machine of its own (expr.c SCRIPTS COMPILED): waiting stops the machine where it is
// and hands the turn back to the game until that game second, so the game never stops for a script and everything a
// script reads is the game as it is. Their turns are the safe point's - between the simulation's steps, where the
// pipe's commands run (script_triggers): a game without a computer player has them too, and a spawned unit is there
// at once. Several scripts run side by side, each in turn, always in the order given (the same game twice the same).
// Its values live in a region of its own. A script is checked when it is read (syntax, unknown names, functions and
// their arguments, a variable of the code around changed in a function): a mistake there and it doesn't run at all;
// a mistake while it runs (a command's error, "a" * 2) ends it - both logged "# setup: file:line: ...". Every command
// is logged: "# setup: <command> -> <reply>". script dryrun <file> <out> (a pipe command) runs one without the game:
// the time jumps to wherever it waits, every command is written to <out> with its game time (ms) instead of being run.
// The player's own script of every game, setup.eeai beside ageofai.ini with its hooks by the kind of game, runs here
// too - in any game, not a test's only (SETUP.EEAI below). A saved game carries them all (THE SCRIPTS IN A SAVED GAME).
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
    struct xunit u;         // its tree (read from a file: for the check; a saved game's has none)
    struct xcode *code;     // its instructions, and the machine that runs them once it begins (expr.c SCRIPTS COMPILED)
    struct xvm *vm;
    uint32_t wake;          // game ms it waits for
    int done;
    int hooks;              // setup.eeai: the hooks called after its top, a bit each (SETUP.EEAI below)
    struct xregion reg;     // its values
};
internal struct script g_scripts[SCRIPTS_MAX];
internal int g_nscripts = -1;
internal struct script g_own;   // setup.eeai of this game (SETUP.EEAI below; no text: none)
internal int scripts_machines(struct xvm **out, int cap);   // (below: the machines that hold the scripts' values)
internal int g_own_fresh;       // this game's clock seen before 2 s (scripts_turns)
internal int g_scripts_go;      // this game's scripts and triggers have started (THE SCRIPTS' START)
internal int g_scripts_restored;   // g_scripts are a saved game's (THE SCRIPTS IN A SAVED GAME), not a test's setup=
internal struct script *g_cur_script;
internal uint32_t g_script_now;
internal double script_now(void) { return g_script_now / 1000.0; }
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
internal void script_note(const char *what) { if (g_cur_script) log_line("# setup: %s: %s()\n", g_cur_script->path, what); }   // a hook called
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
// Called after the file's top has run, on_game first, then the one of the game's type (own_new_game picks; the
// compiler puts their calls at the end of the top's instructions, each done when the machine's hooks say so). Another
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
// a script's text made what runs (path: its file, for the messages and setup.eeai's rules): parsed, checked, compiled
// to *code (it stays, as the tree does); 0 and *why / *line
internal int script_compile(const char *path, const char *text, struct xunit *u, struct xcode **code, const char **why, int *line) {
    static char msg[200];
    *code = NULL;
    *u = x_parse(text, 1);
    if (!u->ok) { *why = u->why ? u->why : "not a script (a statement that is none, or nested too deep)"; *line = u->line; return 0; }
    struct xctx c = { 0 };
    c.found = 1;
    struct xcheck k = { &c, 1, NULL, 0, "", 0, 0, { 0 }, { 0 }, 0, 0, 0, 0 };
    struct xscope sc = { NULL, { 0 }, 0, { 0 }, 0, { 0 }, { 0 } };
    const char *f0 = g_x_check_file, *ph0 = g_x_check_ph, *par0 = g_x_check_par;
    const char *base = strrchr(path, '\\') ? strrchr(path, '\\') + 1 : strrchr(path, '/') ? strrchr(path, '/') + 1 : path;
    int own = !lstrcmpiA(base, "setup.eeai"), have;
    g_x_check_file = base; g_x_check_ph = NULL; g_x_check_par = NULL;
    int ok = x_check(&k, u->root, &sc);
    g_x_check_file = f0; g_x_check_ph = ph0; g_x_check_par = par0;
    if (!ok) { lstrcpynA(msg, k.why ? k.why : "a mistake", sizeof msg); *why = msg; *line = k.line; return 0; }
    if (own && !setup_hooks(u, &have, why, line)) return 0;   // (the checker reads it so too)
    *code = xc_compile(u->root, text, own ? k_setup_hooks : NULL, own ? HOOK_N : 0);
    if (*code && (*code)->ok) return 1;
    *why = *code ? (*code)->why : "no memory";
    *line = *code ? (*code)->why_line : 0;
    *code = NULL;
    return 0;
}
internal int script_read_in(struct script *s, const char **why, int *line) {   // s->path read and made what runs
    s->text = script_read(s->path);
    if (!s->text) { *why = "can't be read"; *line = 0; return 0; }
    return script_compile(s->path, s->text, &s->u, &s->code, why, line);
}
// a script's turn: its machine goes on until it waits, ends or stops on a mistake; then what it worked out and dropped
// is cleared out of its region when that has grown (the values it holds copied into a new one: expr.c xvm_gc - a
// loop that runs long does it on its way too)
internal void script_turn(struct script *s, uint32_t now) {
    if (!s->text || !s->code || s->done || now < s->wake) return;
    if (!s->vm && !(s->vm = xvm_new(s->code, s->hooks))) { s->done = 1; log_line("# setup: %s: no memory - not run\n", s->path); return; }
    struct xregion *r0 = g_xr;
    const char *why = "";
    g_xr = &s->reg;
    g_script_now = now;
    g_x_hooks.now = script_now; g_x_hooks.wait = NULL; g_x_hooks.command = script_command; g_x_hooks.did = script_did; g_x_hooks.note = script_note;
    g_x_machines = scripts_machines;
    g_x_safe = 1;   // (its spawns made at once, as a trigger's: units.c)
    g_cur_script = s;
    int r = xvm_turn(s->vm, &why);
    if (r == XVM_YIELD) s->wake = (uint32_t)(s->vm->wake * 1000.0 + 0.5);
    else {
        if (r == XVM_FAIL) {
            log_line("# setup: %s:%d: stopped - %s\n", s->path, s->vm->line, why);
            if (s == &g_own) x_error("setup.eeai:%d: stopped - %s", s->vm->line, why);   // the player's own file: on the screen too
        }
        log_line("# setup: %s: done\n", s->path);
        s->done = 1;
    }
    if (s->reg.total > (s->vm->gc_at ? s->vm->gc_at : X_GC_STEP)) xvm_gc(s->vm);
    g_cur_script = NULL;
    g_x_safe = 0;
    memset(&g_x_hooks, 0, sizeof g_x_hooks);
    g_xr = r0;
}
internal void scripts_load(void) {   // setup=a.eeai;b.eeai: each read, checked and compiled, once
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
            if (!script_read_in(s, &why, &line)) {
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
// (game_script/relations.c SETUP; on the texts' machine, x_setup_run: before the game there is nothing to wait for or to save) - a loaded
// game's ran when it began (the setup's mode 2; a test's random map is a load of newgame.ees, its mode 0 again by now -
// randmap.c). A mistake there ends that script
internal void script_setup_blocks(struct script *s) {
    struct xctx c = { 0 };
    c.found = 1;
    c.script = 1;
    struct xregion *r0 = g_xr;
    g_xr = &s->reg;
    g_x_last_fail[0] = 0;
    int ok = x_setup_run(s->code, &c);
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
// script's), then the hooks - from the game's start (THE SCRIPTS' START).
// No hook is called in a saved game that is loaded (they ran when the game began; the save carries on what they
// started: THE SCRIPTS IN A SAVED GAME), none in a multiplayer game (each PC would run its own
// file: a desync, as a console command there), no turn while the scenario editor is on. No file, no script.
internal void script_own_path(char path[MAX_PATH]) {
    lstrcpynA(path, g_aimod_ini, MAX_PATH - 16);
    char *slash = strrchr(path, '\\');
    lstrcpyA(slash ? slash + 1 : path, "setup.eeai");
}
// a script over where it is - its machine, its values, its text (the triggers that read them are reset with it); its
// tree and code stay, as every compiled source
internal void script_end(struct script *s) {
    x_region_free(&s->reg);
    free(s->text);
    free(s->vm);
    memset(s, 0, sizeof *s);
}
internal void scripts_restored_end(void) {   // a saved game's scripts (THE SCRIPTS IN A SAVED GAME): over with their game
    if (!g_scripts_restored) return;
    for (int i = 0; i < SCRIPTS_MAX; i++) script_end(&g_scripts[i]);
    g_nscripts = -1;
    g_scripts_restored = 0;
}
internal void own_new_game(ee_setup *setup) {
    // started by a command of its own (a load from a hook: this is its turn), or beside a test's scripts in a later
    // game of their process (they go on over a load, their triggers with them): it goes on as it is
    if (g_cur_script == &g_own || (g_own.text && g_nscripts > 0)) return;
    script_end(&g_own);   // the last game's: over where it waited
    g_script_now = 0;
    g_own_fresh = g_scripts_go = 0;
    if (g_nscripts <= 0) { x_triggers_reset(); x_units_reset(); }   // its triggers and timers, the units it spawned
    int mode = setup_get_mode(setup), humans = 0, have = 0, line;
    if (mode == 2) return;
    script_own_path(g_own.path);
    if (GetFileAttributesA(g_own.path) == INVALID_FILE_ATTRIBUTES) return;
    for (int s = 0; s < 16; s++) humans += setup_slot_get_type(setup_slot_at(setup, s)) == 1;
    if (humans > 1) { log_line("# setup: %s: not in a multiplayer game (each PC would run its own)\n", g_own.path); return; }
    const char *why;
    if (!script_read_in(&g_own, &why, &line)) {
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
// THE SCRIPTS' START: at the first safe point after the game's first step (its clock past 0: before it the game says
// none runs - game_time() 0). Until 2026-11-18 every script waited 2 s of game time (the player: "setup.eeai triggers
// with a noticable delay, why isnt it run with the first step"; the 2 s came with the first test setups, 2026-10-11).
// The AIs' first turns come later, spread over the first seconds (three AIs: 34, 375, 715 ms) - what a script gives them
// before is theirs from it: a personality (persona: g_persona_given), units, stock; nothing a script calls sets what
// their first turn's settings_apply does. A loaded game's scripts go on at once
// The scripts' turns (script_triggers, the safe point): setup.eeai's, then the test's (or a saved game's) in their
// order - from THE SCRIPTS' START on each goes on until it waits again. Only once this game's clock was seen before 2 s
// (a later game of the session: while it loads the last game's time may still stand there; a loaded game's scripts are
// fresh as they are restored), not while the scenario editor is on. 1: they have started
internal int scripts_turns(uint32_t now) {
    if (now < 2000) g_own_fresh = 1;
    if (!g_own_fresh || !game_map() || editor_is_on() || !now) return g_scripts_go;
    if (!g_scripts_go && (g_own.text || g_nscripts > 0)) log_line("# setup: the scripts start at %u ms\n", now);
    g_scripts_go = 1;
    script_turn(&g_own, now);
    for (int i = 0; i < g_nscripts; i++) script_turn(&g_scripts[i], now);
    return 1;
}
internal void run_setup(uint32_t now) { (void)now; scripts_load(); }   // (autotest_tick's: the turns are the safe point's)
// the scripts' and the triggers' turn: at every safe point between the simulation's steps (control.c hook_sim_receive,
// where the pipe's commands run too) - several a game second, so the triggers' 500 ms checks hold. No waiting in a
// trigger's function (a trigger's run has after's). The scripts' wins and defeats are carried out here too
// (relations.c x_effects_flush: the game's resign path wants this point), and their spawned units made
internal void script_triggers(uint32_t now) {
    x_effects_flush();
    if (!scripts_turns(now) || (g_nscripts <= 0 && !g_own.text)) return;
    if (now > g_script_now) g_script_now = now;
    g_x_hooks.now = script_now; g_x_hooks.command = script_command; g_x_hooks.did = script_did;
    g_x_machines = scripts_machines;
    g_x_safe = 1;   // (a trigger's spawns made at once: units.c)
    struct script *all[1 + SCRIPTS_MAX] = { &g_own };
    for (int i = 0; i < g_nscripts; i++) all[1 + i] = &g_scripts[i];
    for (int i = 0; i < 1 + g_nscripts; i++) if (all[i]->vm) all[i]->vm->reg = &all[i]->reg;   // (where its triggers' functions set its names)
    x_triggers_tick(g_script_now / 1000.0);
    g_x_safe = 0;
    memset(&g_x_hooks, 0, sizeof g_x_hooks);
    for (int i = 0; i < 1 + g_nscripts; i++) {   // what they set and dropped again, cleared out (a script that waits long, or is done, has no turn to do it in)
        struct script *s = all[i];
        if (!s->vm || s->reg.total <= (s->vm->gc_at ? s->vm->gc_at : X_GC_STEP)) continue;
        struct xregion *r0 = g_xr;
        g_xr = &s->reg;
        xvm_gc(s->vm);
        g_xr = r0;
    }
}
// THE SCRIPTS IN A SAVED GAME (persist.c's block "scripts", in the save's .ageofai file): a save carries its scripts
// whole - each one's source as it was read (for the messages), its instructions (a save runs its own: it doesn't
// depend on what setup.eeai or a scenario's file say later, nor on what a later mod would compile of them), and its
// machine: the functions it is in, their variables, the values it was working out, where it stands - so a script
// that waits anywhere goes on there, at its time. With them the triggers, timers and after's, the views they keep,
// the dicts (expr.c SAVED STATE), the spawned units' handles (game_script/units.c). A loaded game (hook_game_start's
// persist_load, after scripts_new_game) has them as they were.
// Not saved: a text's function held in a variable (a personality's - told in the log). A script whose own turn
// it is as the game is saved (a save of its own, not the queued one of the pipe's save) doesn't go on: where it
// stands is the middle of that command - told; what it started does.
// A test that runs its own scripts (setup=) keeps to them: the save's are left out there. Elsewhere the save's test
// scripts come back beside its setup.eeai.
#define SCRIPT_STATE_FORMAT 6   // 6: dicts of any value (those values at the end); 5: lists (their items after the rest); 4: structs and refs (their types with the code, a ref a value); 3: the scripts' code and machines (2: the tree's frames and where a script stood)
internal uint8_t *scripts_state_save(uint32_t *size) {
    struct script *sc[1 + SCRIPTS_MAX];
    struct xcode *code[1 + SCRIPTS_MAX];
    struct xvm *vm[1 + SCRIPTS_MAX];
    int n = 0, lost = 0, waiting = 0;
    *size = 0;
    if (g_own.text && g_own.code) sc[n++] = &g_own;
    for (int i = 0; i < g_nscripts; i++) if (g_scripts[i].code) sc[n++] = &g_scripts[i];
    if (!n) return NULL;
    struct pbuf o = { 0 };
    pb_int(&o, SCRIPT_STATE_FORMAT);
    pb_int(&o, (int32_t)g_script_now);
    pb_int(&o, n);
    for (int i = 0; i < n; i++) {
        struct script *s = sc[i];
        uint8_t kind = s != &g_own, done = s->done != 0;
        if (s == g_cur_script && !done) {
            done = 1;
            log_line("# setup: %s:%d: saves the game in its own turn - it doesn't go on after a load, what it started does\n", s->path, s->vm ? s->vm->line : 0);
        }
        pb_put(&o, &kind, 1);
        pb_put(&o, &done, 1);
        pb_str(&o, s->path);
        pb_text(&o, s->text);
        pb_int(&o, (int32_t)s->wake);
        pb_int(&o, s->hooks);
        code[i] = s->code;
        vm[i] = s->vm;
        waiting += !done;
    }
    x_state_save(&o, code, vm, n, &lost);
    x_units_state_save(&o);
    log_line("# persist: scripts: %d (%d going on), %d triggers and timers, %d after's, %d spawned units' handles, %u bytes%s\n", n, waiting,
             g_xt_n, g_xa_n, g_xslot_n, o.n, lost ? " - a text's function left out" : "");
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
    int n = pr_int(&r), nt = 0, waiting = 0;
    if (r.bad || n < 1 || n > 1 + SCRIPTS_MAX) { log_line("# persist: the save's scripts are broken - none restored\n"); return; }
    struct script *sc[1 + SCRIPTS_MAX];
    const char *src[1 + SCRIPTS_MAX];
    struct xregion *regs[1 + SCRIPTS_MAX];
    struct xcode *code[1 + SCRIPTS_MAX];
    struct xvm *vm[1 + SCRIPTS_MAX];
    x_triggers_reset();
    x_units_reset();
    for (int i = 0; i < n; i++) {
        uint8_t kind = 0, done = 0;
        pr_get(&r, &kind, 1);
        pr_get(&r, &done, 1);
        char *path = pr_str(&r), *text = pr_text(&r);
        uint32_t wake = (uint32_t)pr_int(&r);
        int hooks = pr_int(&r);
        struct script *s = !kind && !g_own.text ? &g_own : nt < SCRIPTS_MAX ? &g_scripts[nt] : NULL;
        if (r.bad || !s || !path || !text) { free(path); free(text); scripts_state_drop("broken"); return; }
        if (s != &g_own) nt++;
        memset(s, 0, sizeof *s);
        lstrcpynA(s->path, path, MAX_PATH);
        free(path);
        s->text = text;
        s->done = done;
        s->wake = wake;
        s->hooks = hooks;
        s->reg.busy = 1;
        sc[i] = s; src[i] = text; regs[i] = &s->reg;
        waiting += !done;
    }
    if (nt) { g_nscripts = nt; g_scripts_restored = 1; }
    int ok = x_state_load(&r, src, regs, code, vm, n);
    for (int i = 0; i < n; i++) { sc[i]->code = code[i]; sc[i]->vm = vm[i]; }
    if (!ok) { scripts_state_drop("broken, or a newer version's"); return; }
    x_units_state_load(&r);
    if (r.bad) { scripts_state_drop("broken"); return; }
    g_script_now = now;
    g_own_fresh = 1;
    log_line("# persist: scripts restored: %d (%d going on), %d triggers and timers, %d after's, %d spawned units' handles\n", n, waiting,
             g_xt_n, g_xa_n, g_xslot_n);
}
// THE DRY RUN (script dryrun <file> <out>): the script without the game - waiting moves the clock (from 0), the
// commands are written to <out> ("<ms> <command>") and answered "ok" (tests: a converted scenario issues the same
// commands at the same times as before). Its triggers have their turns on the way, every 500 ms up to where the
// script waits last (a script that tests triggers ends with a wait); they and its after's go when it is done
internal double g_dry_now;
internal FILE *g_dry_out;
internal double dry_now(void) { return g_dry_now; }
internal struct xvm *g_dry_vm;   // the machine run dry (its region the one in use)
// every machine that holds a script's values now (expr.c LISTS: what the lists nobody holds are found by)
internal int scripts_machines(struct xvm **out, int cap) {
    int n = 0;
    if (g_own.vm && n < cap) out[n++] = g_own.vm;
    for (int i = 0; i < SCRIPTS_MAX; i++) if (g_scripts[i].vm && n < cap) out[n++] = g_scripts[i].vm;
    if (g_dry_vm && n < cap) out[n++] = g_dry_vm;
    return n;
}
internal void dry_wait(double until) {
    for (double t = g_dry_now + 0.5; t <= until + 1e-9; t += 0.5) {
        g_dry_now = t;
        x_triggers_tick(t);
        if (g_dry_vm && g_xr->total > (g_dry_vm->gc_at ? g_dry_vm->gc_at : X_GC_STEP)) xvm_gc(g_dry_vm);   // (as script_triggers does)
    }
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
    if (sscanf(args, "dryrun %259s %259s", file, dst) != 2) { REPLY("error: usage: script dryrun <file> <out> | script state [save <file> | load <file>]\n"); return; }
    char *text = script_read(file);
    struct xunit u;
    struct xcode *code;
    const char *why = "can't be read";
    int line = 0;
    if (!text || !script_compile(file, text, &u, &code, &why, &line)) { REPLY("error: %s:%d: %s\n", file, line, why); free(text); return; }
    g_dry_out = fopen(dst, "wb");
    if (!g_dry_out) { REPLY("error: can't write %s\n", dst); free(text); return; }
    struct xregion reg = { 0 }, *r0 = g_xr;
    reg.busy = 1;
    g_xr = &reg;
    g_dry_now = 0.0;
    __typeof__(g_x_hooks) h0 = g_x_hooks;   // (a script may run this: its own are back after)
    g_x_hooks.now = dry_now; g_x_hooks.wait = NULL; g_x_hooks.command = dry_command; g_x_hooks.did = dry_did; g_x_hooks.note = NULL;
    g_x_machines = scripts_machines;
    static struct xtrig keep_t[XT_MAX];   // a game's own triggers set aside meanwhile: the dry run has none but its own
    static struct xafter keep_a[XA_MAX];
    int nt = g_xt_n, na = g_xa_n;
    double next = g_xt_next;
    memcpy(keep_t, g_xt, sizeof g_xt);
    memcpy(keep_a, g_xa, sizeof g_xa);
    g_xt_aside = keep_t; g_xt_aside_n = nt;   // (the sweep keeps what they hold meanwhile)
    g_xa_aside = keep_a; g_xa_aside_n = na;
    g_xt_n = g_xa_n = 0;
    g_xt_next = 0;
    LARGE_INTEGER qf, q0, q1;   // (the time it takes: what a game's objects cost a script - docs/testing.md "The engine's speed")
    QueryPerformanceFrequency(&qf);
    QueryPerformanceCounter(&q0);
    struct xvm *m = xvm_new(code, 0);   // (setup.eeai run dry: its top, no hook - there is no game to tell its kind)
    const char *stop = "no memory";
    struct xvm *dry0 = g_dry_vm;
    if (m) m->reg = &reg;
    g_dry_vm = m;
    int r = m ? xvm_turn(m, &stop) : XVM_FAIL;
    while (r == XVM_YIELD) { dry_wait(m->wake); r = xvm_turn(m, &stop); }   // the machine's turns, the clock moved where it waits
    g_dry_vm = dry0;
    int ok = r == XVM_DONE;
    QueryPerformanceCounter(&q1);
    if (!ok) log_line("# setup: %s:%d: stopped - %s\n", file, m ? m->line : 0, stop);
    free(m);
    memcpy(g_xt, keep_t, sizeof g_xt);
    memcpy(g_xa, keep_a, sizeof g_xa);
    g_xt_n = nt; g_xa_n = na; g_xt_next = next;
    g_xt_aside_n = g_xa_aside_n = 0;
    g_x_hooks = h0;
    g_xr = r0;
    x_region_free(&reg);
    fclose(g_dry_out);
    g_dry_out = NULL;
    free(text);   // (its tree and code stay: a dry run is a test's)
    if (ok) REPLY("ok: %s run dry to %s in %.2f ms\n", file, dst, (double)(q1.QuadPart - q0.QuadPart) * 1000.0 / (double)qf.QuadPart);
    else REPLY("error: %s stopped (the log has where) - what it did is in %s\n", file, dst);
}
