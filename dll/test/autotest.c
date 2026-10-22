// ---- autotest mode (aimod_autotest.ini next to the DLL) ----
// The game itself accepts:  "Empire Earth.exe" -loadgame "name.ees"   (file in Data\Saved Games\,
// name must not contain '-' or '/'). This mode adds: skip intro movies, quit after N game minutes.
internal struct { int skip_intro, scenario_mode, turbo, game_seed; double quit_minutes; float speed; } g_auto;

#include "test/randmap.c"

internal void autotest_first_tick(ee_player *pl) {
    if (g_autotest) randmap_ai_toggles(pl);
}

internal void load_autotest(const char *ini) {
    g_autotest = GetFileAttributesA(ini) != INVALID_FILE_ATTRIBUTES;
    if (!g_autotest) return;
    g_auto.skip_intro = ini_int("autotest", "skip_intro", 1, ini);
    g_auto.quit_minutes = ini_num("autotest", "quit_minutes", 0, ini);   // 3.2: 3 minutes 12 seconds
    g_auto.scenario_mode = ini_int("autotest", "scenario_mode", 0, ini);
    g_auto.turbo = ini_int("autotest", "turbo", 0, ini);
    g_auto.game_seed = ini_int("autotest", "game_seed", 0, ini);
    GetPrivateProfileStringA("autotest", "setup", "", g_setup_path, sizeof g_setup_path, ini);
    randmap_load(ini);
    g_auto.speed = (float)ini_num("autotest", "speed", 0, ini);
}

internal void apply_game_speed(float s) {
    *game_speed_var() = s;
    void *obj = game_state();
    ((void (__thiscall *)(void *))(*(void ***)obj)[11])(obj);
}

// Replace `call AddMovie` (callee pops its one argument) with `add esp,4; nop; nop`.
internal BOOL skip_intro_movies(void) { return patch_skip_intro(); }

// Diagnostics for the game's own command-line handling (both __cdecl).

// A test slot's own network ports: the engine's network server - a single-player game runs through it too - binds UDP
// 33334 and TCP 33335 (push/mov immediates in TCP Protocol.dll). A second game can't, NEProtocol::Publish fails and
// NESynchronousServer::ProcessLocalUserInput later reads its missing record object (+0x16C): a crash. Slot n: +2n.
// At the command line, not in DllMain: the engine loads the module later (LoadLibrary under the loader lock is none of
// ours); loaded here it stays, and the engine's own LoadLibrary gets this copy.
internal void slot_ports(void) {
    static int done;
    if (!g_slot || done) return;
    done = 1;
    char p[MAX_PATH];
    wsprintfA(p, "%sTCP Protocol.dll", g_game_dir);
    HMODULE m = GetModuleHandleA("TCP Protocol.dll");
    if (!m) m = LoadLibraryA(p);
    int n = patch_tcp_ports(m, 2 * g_slot);
    char t[80];
    wsprintfA(t, "%d/%d (%d of 8 patched)", 33334 + 2 * g_slot, 33335 + 2 * g_slot, n);
    log_line("# slot: network ports %s\n", t);
}
internal char __cdecl hook_parse_cmdline(const char *cmd) {
    slot_ports();
    log_line("# cmdline parse: \"%s\"\n", cmd ? cmd : "(null)");
    char r = parse_cmdline(cmd);
    log_line("# cmdline parse returned %d (1 = continue to main menu)\n", r);
    return r;
}

internal int __cdecl hook_load_saved(void *name) {
    log_line("# cmdline loadgame: calling loader\n");
    int r = load_saved(name);
    log_line("# cmdline loadgame: loader returned %d\n", r);
    return r;
}

// Autotest setup=<file>[;<file>...] (paths relative to the game folder): scripts run from 2 s game time (scripts.c)
internal void run_setup(uint32_t now);

// The lockstep server tunes its pace to the machine (NESynchronousServer::TuneForSlowCPU, Low-Level Engine rva 0x55099):
// steps a second (+0x1FB) = 1000 / (the clients' measured time x +0x170), held within +0x1B8..+0x1B4, and the steps a
// tick (+0x1F9) follow; each turn message hands the physics its step rate, and the step's game time (1000 * speed /
// rate) follows it. Its measure is real time: in a long test game (turbo, other games beside it) the server went from 43
// to 124 steps a second at a moment of its own in each run (Small Islands Large, 8 at a time: minutes 31-40), the steps
// from 170 to 60 ms - and two runs of the same game_seed parted there. Test runs hold the pace every game has from its
// first steps on (44 a second: 15 steps a 350 ms tick; the range is 15-125) - a constant, the same on every machine (the
// constructor's 120 is tuned down to 44 before the first tick: held at 120 the runs parted at once). A player's game
// keeps the tuning.
#define TEST_STEPS_A_SECOND 44
internal void pin_server_pace(void) {
    uint8_t *srv = game_server();
    const unsigned rate = TEST_STEPS_A_SECOND;
    if (!srv || (FIELD(srv, OF(SERVER, 0x1B4), unsigned) == rate && FIELD(srv, OF(SERVER, 0x1B8), unsigned) == rate)) return;
    if (g_log) fprintf(g_log, "# autotest: server pace held at %u steps a second (its range was %u-%u, now %u)\n", rate,
                       FIELD(srv, OF(SERVER, 0x1B8), unsigned), FIELD(srv, OF(SERVER, 0x1B4), unsigned), FIELD(srv, OF(SERVER, 0x1FB), uint8_t));
    FIELD(srv, OF(SERVER, 0x1B4), unsigned) = rate;
    FIELD(srv, OF(SERVER, 0x1B8), unsigned) = rate;
}

internal void autotest_tick(uint32_t now) {
    if (!g_autotest) return;
    pin_server_pace();
    if (g_setup_path[0] && now >= 2000) run_setup(now);
    float cur = *game_speed_var();
    if (g_auto.speed > 0 && (cur < g_auto.speed - 0.01f || cur > g_auto.speed + 0.01f)) {
        apply_game_speed(g_auto.speed);
        if (g_log) {
            EnterCriticalSection(&g_lock);
            fprintf(g_log, "# autotest: game speed %.2f -> %.2f at t=%u\n", cur, g_auto.speed, now);
            fflush(g_log);
            LeaveCriticalSection(&g_lock);
        }
    }
    if (g_auto.quit_minutes <= 0 || now < (uint32_t)(g_auto.quit_minutes * 60000.0 + 0.5)) return;
    EnterCriticalSection(&g_lock);   // the pipe, the LLM worker and the render thread write the log too
    if (g_log) {
        fprintf(g_log, "# autotest: reached %g game minutes, exiting\n", g_auto.quit_minutes);
        fclose(g_log);
        g_log = NULL;
    }
    LeaveCriticalSection(&g_lock);
    TerminateProcess(GetCurrentProcess(), 0);   // ExitProcess crashes in the game's other threads
}

// Autotest game_seed: starting a game from the menu (EEUIELEStart::vf1 0x5C9B91, 0x67E1D6) seeds the C library
// with the clock, srand(TMGetTime()), and sends rand() along with the start message (0x5D, +0x14) - the game's
// random seed, so random-map runs differ even with the same map seed. With game_seed every srand uses that seed,
// and the start message carries it whichever start built it (hook_start_exec, trace.c: the load-game start a test
// run uses puts TMGetTime() in it directly).
internal srand_hook g_srand;
internal void __cdecl hook_srand(unsigned seed) {
    (void)seed;
    g_srand((unsigned)g_auto.game_seed);
}

internal BOOL install_srand(void) {   // the exe imports srand from msvcrt.dll
    return patch_srand(hook_srand, &g_srand);
}
