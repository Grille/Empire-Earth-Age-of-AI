// ---- autotest mode (aimod_autotest.ini next to the DLL) ----
// The game itself accepts:  "Empire Earth.exe" -loadgame "name.ees"   (file in Data\Saved Games\,
// name must not contain '-' or '/'). This mode adds: skip intro movies, quit after N game minutes.
static struct { int skip_intro, scenario_mode, turbo, game_seed; double quit_minutes; float speed; } g_auto;

#include "test/randmap.c"

static void autotest_first_tick(void *pl) {
    if (g_autotest) randmap_ai_toggles(pl);
}

static void load_autotest(const char *ini) {
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

static void apply_game_speed(float s) {
    *(float *)GA(G_GAME_SPEED) = s;
    void *obj = (void *)GA(G_GAME_STATE);
    ((void (__thiscall *)(void *))(*(void ***)obj)[11])(obj);
}

// Replace `call AddMovie` (callee pops its one argument) with `add esp,4; nop; nop`.
static BOOL skip_intro_movies(void) {
    const uint32_t *sites = GL(INTRO_CALLS);
    static const uint8_t stub[5] = { 0x83, 0xC4, 0x04, 0x90, 0x90 };
    for (int i = 0; i < 3; i++) {
        uint8_t *p = (uint8_t *)sites[i];
        if (p[0] != 0xE8 || sites[i] + 5 + *(int32_t *)(p + 1) != GA(FN_ADD_MOVIE)) return FALSE;
    }
    for (int i = 0; i < 3; i++) {
        DWORD old;
        VirtualProtect((void *)sites[i], 5, PAGE_EXECUTE_READWRITE, &old);
        memcpy((void *)sites[i], stub, 5);
        VirtualProtect((void *)sites[i], 5, old, &old);
    }
    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
    return TRUE;
}

// Diagnostics for the game's own command-line handling (both __cdecl).
typedef char (__cdecl *ParseFn)(const char *);
typedef int (__cdecl *LoadFn)(void *);

// A test slot's own network ports: the engine's network server - a single-player game runs through it too - binds UDP
// 33334 and TCP 33335 (push/mov immediates in TCP Protocol.dll). A second game can't, NEProtocol::Publish fails and
// NESynchronousServer::ProcessLocalUserInput later reads its missing record object (+0x16C): a crash. Slot n: +2n.
// At the command line, not in DllMain: the engine loads the module later (LoadLibrary under the loader lock is none of
// ours); loaded here it stays, and the engine's own LoadLibrary gets this copy.
static void slot_ports(void) {
    static const uint32_t imm[] = { 0x17E2, 0x196F, 0x19EB, 0x1EB8, 0x1FBD, 0x2148, 0x279C, 0x2F2C };   // RVAs
    static int done;
    if (!g_slot || done) return;
    done = 1;
    char p[MAX_PATH];
    wsprintfA(p, "%sTCP Protocol.dll", g_game_dir);
    HMODULE m = GetModuleHandleA("TCP Protocol.dll");
    if (!m) m = LoadLibraryA(p);
    int n = 0;
    for (size_t i = 0; m && i < sizeof imm / sizeof *imm; i++) {
        uintptr_t at = (uintptr_t)m + imm[i];
        uint32_t old = *(uint32_t *)at;
        if (old == 33334 || old == 33335) n += patch_code32(at, old, old + 2 * g_slot);
    }
    char t[80];
    wsprintfA(t, "%d/%d (%d of 8 patched)", 33334 + 2 * g_slot, 33335 + 2 * g_slot, n);
    log_line("# slot: network ports %s%.0d\n", t, 0);
}
static char __cdecl hook_parse_cmdline(const char *cmd) {
    slot_ports();
    log_line("# cmdline parse: \"%s\"%.0d\n", cmd ? cmd : "(null)", 0);
    char r = ((ParseFn)GA(FN_PARSE_CMDLINE))(cmd);
    log_line("# cmdline parse returned %s%d (1 = continue to main menu)\n", "", r);
    return r;
}

static int __cdecl hook_load_saved(void *name) {
    log_line("# cmdline loadgame: calling loader%s%.0d\n", "", 0);
    int r = ((LoadFn)GA(FN_LOAD_SAVED))(name);
    log_line("# cmdline loadgame: loader returned %s%d\n", "", r);
    return r;
}

// Autotest setup=<file>[;<file>...] (paths relative to the game folder): scripts run from 2 s game time (scripts.c)
static void run_setup(uint32_t now);

static void autotest_tick(uint32_t now) {
    if (!g_autotest) return;
    if (g_setup_path[0] && now >= 2000) run_setup(now);
    float cur = *(float *)GA(G_GAME_SPEED);
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
typedef void (__cdecl *SrandFn)(unsigned);
static SrandFn g_srand;
static void __cdecl hook_srand(unsigned seed) {
    (void)seed;
    g_srand((unsigned)g_auto.game_seed);
}

static BOOL patch_srand(void) {   // the exe imports srand from msvcrt.dll
    return iat_hook(GA(IAT_SRAND), "msvcrt.dll", "srand", (void *)hook_srand, (void **)&g_srand);
}
