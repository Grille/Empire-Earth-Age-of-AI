// eeaimod.dll - Empire Earth AI mod.
// Loaded automatically: Low-Level Engine.dll LoadLibrary()s every *.dll in the game folder.
//
// Stage 1: observation only. Hooks the computer player's resource manager and logs its
// economy state to aimod_econ.csv. Does not change game behaviour.

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <math.h>
#include <tlhelp32.h>
#include <psapi.h>   // types only (K32GetProcessMemoryInfo by GetProcAddress)

// ---- addresses in Empire Earth.exe 1.04 (ImageBase 0x400000) ----
#define CALL_COMPUTE_TARGETS 0x0048D347  // call EECPResourceManager::ComputeGathererTargets (0x48D0A2)
#define CALL_ASSIGN_CITIZENS 0x0048D36A  // call EECPResourceManager::AssignCitizens        (0x48A554)
#define FN_COMPUTE_TARGETS   0x0048D0A2
#define FN_ASSIGN_CITIZENS   0x0048A554
#define CALL_RM_UPDATE       0x0046E3C6  // call EECPResourceManager::Update(now)              (0x48D263)
#define FN_RM_UPDATE         0x0048D263
#define G_WORLD_CLOCK        0x00919774  // ptr; +0xD0 = game time in ms

// EECPResourceManager layout (from decompiled ctor/serializer 0x489F76 / 0x48C5B2)
#define RM_OWNER        0x130  // -> object whose +0x4C is the EEComputerPlayer
#define RM_LISTS        0x0A4  // 7 unit lists, stride 0x14, count at +0x10
#define RM_DEMAND       0x134  // int[7] outstanding resource need
#define RM_TARGET       0x150  // uint[7] desired gatherers
#define RM_TIMERS       0x16C  // uint[5] next-run game times (ms)
#define RM_INITIALIZED  0x187  // byte, timers are only reset while this is 0
// EEPlayer / EEComputerPlayer
#define PL_INDEX        0x45C
#define PL_STOCK        0xAF0  // int[] resources, indexed by resource type
#define PL_DIFFICULTY   0xBA4
#define PL_CHEAT        0xEFB
#define PL_TOGGLES      0xEF4  // AI toggle bytes 0xEF4..0xF05 (Aktiv, Cheat, Angreifen, ...)
#define PL_TOGGLES_LEN  18
#define PL_TECHTREE     0x9D4  // -> tech tree; +0x4E4 = current epoch index (0 = Prehistoric)
#define TT_EPOCH        0x4E4
// EECPCivilizationManager (= rm->owner; main AI update 0x46E2C7)
#define CM_PROJECT      0x50   // current construction project; new projects only start when NULL
#define CM_TIMERS       0x58   // uint[6]: next epoch, bldg-count check, mil bldgs (31s), build (0x46E51E), 0x46E817, 59s
#define CM_TIMERS_N     6
#define CM_FLAGS        0x74   // bytes 0x74..0x7C
#define CM_SETUP_DONE   0x75   // one-time initial setup (0x46E2C7 top) runs while this is 0
#define CM_BUILD_MGR    0x3C   // EECPBuildManager    (Update 0x468540)
#define CM_RESEARCH_MGR 0x40   // EECPResearchManager (Update 0x486A74)
#define BM_INITIALIZED  0x148  // build lists initialised by 0x465E4F while 0
#define RS_INITIALIZED  0x118  // research schedule initialised while 0
#define CM_FLAGS_LEN    9

// Clearing the managers' setup flags at scenario start did not revive the AI (runs 6/7); off for now.
#define FIX_CLEAR_SETUP_FLAGS 0

typedef char (__thiscall *RmFn)(void *self);
typedef void (__thiscall *RmUpdateFn)(void *self, uint32_t now);

static FILE *g_log;
static CRITICAL_SECTION g_lock;

static void autotest_tick(uint32_t now);
static volatile int g_rm_seen;   // an AI resource manager has ticked (else the sim loop drives autotest_tick)
static void execute(const char *cmd, char *out);
static char g_setup_path[MAX_PATH];
static void ctl_note_player(void *pl);
static void autotest_first_tick(void *pl);
static void settings_apply(void *pl);
static void econ_tick(void *pl, uint32_t now);
static void persona_new_game(uint8_t *setup);
static void persona_assign(void *pl);
static void diplo_new_game(void);
static void chat_new_game(void);
static void llm_new_game(void);
static void persist_load(const char *name);
static void econ_targets(uint8_t *rm);
static void econ_rebalance(uint8_t *rm);
static void econ_fields(uint8_t *rm);
static void citizens_safety(uint8_t *rm);
static void attack_tick(void *pl, uint32_t now);
static void assault_tick(void *pl, uint32_t now);
static void landing_tick(void *pl, uint32_t now);
static void intel_tick(void *pl, uint32_t now);
static void intel_new_game(int from_save);
static void econ_new_game(void);
static void attack_new_game(void);
static void aim_new_game(void);
static void landing_new_game(void);
static void bunker_new_game(void);
static void defence_new_game(void);
static void assault_new_game(void);
static void sites_new_game(void);
static void intel_install(const char *aimod_ini, const char *autotest_ini);
static void landing_install(const char *aimod_ini, const char *autotest_ini);
static void assault_install(const char *aimod_ini, const char *autotest_ini);
static void log_line(const char *fmt, const char *a, int b);
static BOOL patch_call(uintptr_t site, uintptr_t expected, void *hook);
static void *g_players[16];   // control.c
static BOOL patch_srand(void);
#define G_SCENARIO_FLAG 0x00918819   // byte: game is a scenario/campaign (settings 0x918410 + 0x409)
static int g_clear_scenario_flag;

// FIX (scenario bug): the scenario/campaign branch of the game start (0x52599B) sets gamestate+0x411,
// which is copied to the settings byte 0x918819. Several AI routines skip their normal planning when it
// is set (apparently scenario authors were expected to script the AI), so in scenarios played from the
// menu the AI only gathers. Editor test-play and saved games don't set it, which is why they work.
// We clear the byte only while a computer player updates (EEComputerPlayer::Update 0x43F91A, called
// at 0x42F3CD for active AIs), so triggers, civ selection and the rest of the game still see a scenario.
#define CALL_CP_UPDATE 0x0042F3CD
#define FN_CP_UPDATE   0x0043F91A
static int g_fix_scenario_ai = 1;    // aimod.ini [fixes] scenario_ai=0 turns it off
static uint8_t g_is_scenario;        // the flag as the game set it (it reads 0 inside the AI update)
typedef void (__thiscall *CpUpdateFn)(void *pl, uint32_t now);

static void __fastcall hook_cp_update(void *pl, void *edx, uint32_t now) {
    (void)edx;
    uint8_t *flag = (uint8_t *)G_SCENARIO_FLAG;
    uint8_t saved = *flag;
    g_is_scenario = saved;
    if (g_fix_scenario_ai) *flag = 0;
    ((CpUpdateFn)FN_CP_UPDATE)(pl, now);
    *flag = saved;
}

#define FIELD(p, off, T) (*(T *)((uint8_t *)(p) + (off)))

// `n` bytes at p readable, from the page table (IsBadReadPtr probes by raising access violations: each one went
// through the crash log's first-chance handler)
static int mem_readable(const void *p, size_t n) {
    uintptr_t a = (uintptr_t)p, end = a + n;
    if (!p || end < a) return 0;
    while (a < end) {
        MEMORY_BASIC_INFORMATION mbi;
        if (!VirtualQuery((void *)a, &mbi, sizeof mbi) || mbi.State != MEM_COMMIT || (mbi.Protect & (PAGE_GUARD | PAGE_NOACCESS))
            || !(mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE
                                | PAGE_EXECUTE_WRITECOPY))) return 0;
        a = (uintptr_t)mbi.BaseAddress + mbi.RegionSize;
    }
    return 1;
}

static uint32_t game_time(void) {
    uint8_t *clock = *(uint8_t **)G_WORLD_CLOCK;
    return clock ? FIELD(clock, 0xD0, uint32_t) : 0;
}

static void *rm_player(void *rm) {
    void *owner = FIELD(rm, RM_OWNER, void *);
    return owner ? FIELD(owner, 0x4C, void *) : NULL;
}

static void log_state(void *rm, const char *phase, uint32_t now) {
    void *pl = rm_player(rm);
    if (!g_log || !pl) return;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "%u,%d,%s,%d,%d", now, FIELD(pl, PL_INDEX, int), phase,
            FIELD(pl, PL_DIFFICULTY, int), FIELD(pl, PL_CHEAT, uint8_t));
    for (int i = 0; i < 7; i++) fprintf(g_log, ",%d", FIELD(pl, PL_STOCK + i * 4, int));
    for (int i = 0; i < 7; i++) fprintf(g_log, ",%d", FIELD(rm, RM_DEMAND + i * 4, int));
    for (int i = 0; i < 7; i++) fprintf(g_log, ",%u", FIELD(rm, RM_TARGET + i * 4, uint32_t));
    for (int i = 0; i < 7; i++) fprintf(g_log, ",%u", FIELD(rm, RM_LISTS + i * 0x14 + 0x10, uint32_t));
    fprintf(g_log, ",%d", FIELD(rm, RM_INITIALIZED, uint8_t));
    for (int i = 0; i < 5; i++) fprintf(g_log, ",%u", FIELD(rm, RM_TIMERS + i * 4, uint32_t));
    void *cm = FIELD(rm, RM_OWNER, void *);
    fprintf(g_log, ",%p", FIELD(cm, CM_PROJECT, void *));
    for (int i = 0; i < CM_TIMERS_N; i++) fprintf(g_log, ",%u", FIELD(cm, CM_TIMERS + i * 4, uint32_t));
    fputc(',', g_log);
    for (int i = 0; i < CM_FLAGS_LEN; i++) fprintf(g_log, "%02x", FIELD(cm, CM_FLAGS + i, uint8_t));
    fputc(',', g_log);
    for (int i = 0; i < PL_TOGGLES_LEN; i++) fprintf(g_log, "%02x", FIELD(pl, PL_TOGGLES + i, uint8_t));
    fprintf(g_log, ",%d\n", FIELD(FIELD(pl, PL_TECHTREE, void *), TT_EPOCH, int));
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}

// __fastcall with a dummy edx parameter matches __thiscall for argument-less methods.
static char __fastcall hook_compute_targets(void *rm, void *edx) {
    (void)edx;
    log_state(rm, "pre_targets", game_time());
    char changed = ((RmFn)FN_COMPUTE_TARGETS)(rm);
    econ_targets((uint8_t *)rm);
    log_state(rm, "post_targets", game_time());
    return changed;
}

static char __fastcall hook_assign_citizens(void *rm, void *edx) {
    (void)edx;
    econ_fields((uint8_t *)rm);   // farm fields (econ.c)
    citizens_safety((uint8_t *)rm);   // away from known enemy towers (attack.c)
    char r = ((RmFn)FN_ASSIGN_CITIZENS)(rm);
    econ_rebalance((uint8_t *)rm);
    log_state(rm, "post_assign", game_time());
    return r;
}

// ---- memory snapshots: raw dumps of a player's AI objects, for diffing working vs broken games ----
// Autotest, or aimod.ini [general] memory_dumps=1: 2 files per AI and game, never deleted (17,000 files / 0.4 GB
// in normal play before) - off for players.
static char g_dump_dir[MAX_PATH];   // also the folder of the control pipe's shot / ddinfo dump pictures
static int g_mem_dumps;
static DWORD g_session;

// Copy `len` bytes of our own memory without risking an access violation on unmapped pages.
static void dump_region(FILE *f, const char *tag, const void *p, uint32_t len) {
    static uint8_t buf[0x2000];
    SIZE_T got = 0;
    if (len > sizeof buf) len = sizeof buf;
    memset(buf, 0, len);   // the static buffer held the last region
    if (p) ReadProcessMemory(GetCurrentProcess(), p, buf, len, &got);
    uint32_t hdr[3] = { (uint32_t)(uintptr_t)p, len, (uint32_t)got };
    char name[16] = {0};
    lstrcpynA(name, tag, sizeof name);
    fwrite(name, 1, sizeof name, f);
    fwrite(hdr, sizeof hdr, 1, f);
    fwrite(buf, 1, len, f);   // unreadable tail stays zero; `got` records how much was valid
}

static void dump_player(void *rm, uint32_t now) {
    void *pl = rm_player(rm);
    void *cm = FIELD(rm, RM_OWNER, void *);
    if (!pl || !cm || !g_dump_dir[0] || !g_mem_dumps) return;
    char path[MAX_PATH];
    static unsigned run;   // distinguishes several games within one session (game times repeat)
    static uint32_t last_now = 0xFFFFFFFF;
    if (now < last_now) run++;
    last_now = now;
    wsprintfA(path, "%s\\s%08lx_r%u_P%d_t%06u.bin", g_dump_dir, g_session, run, FIELD(pl, PL_INDEX, int), now);
    FILE *f = fopen(path, "wb");
    if (!f) return;
    dump_region(f, "player", pl, 0x1000);
    dump_region(f, "civmgr", cm, 0x100);
    // sub-managers referenced by the civilization manager (+0x38 = resource manager, ...)
    for (int off = 0x30; off <= 0x50; off += 4) {
        char tag[16];
        wsprintfA(tag, "cm+%02x", off);
        dump_region(f, tag, FIELD(cm, off, void *), 0x400);
    }
    dump_region(f, "pl+b8c", FIELD(pl, 0xB8C, void *), 0x400);
    dump_region(f, "pl+b90", FIELD(pl, 0xB90, void *), 0x400);
    dump_region(f, "pl+9d4", FIELD(pl, PL_TECHTREE, void *), 0x800);
    // towns / resource centers (player +0xB50 vector of pointers)
    void **tb = FIELD(pl, 0xB50, void **), **te = FIELD(pl, 0xB54, void **);
    for (int i = 0; tb && tb + i < te && i < 32; i++) {
        char tag[16];
        wsprintfA(tag, "town%02d", i);
        dump_region(f, tag, tb[i], 0x100);
    }
    fclose(f);
}

// the player each AI's personality and settings were applied to (reset by hook_game_start: the next game's player
// can sit at the same address - its settings were skipped, a hard AI kept the game's cheat)
static void *g_settings_done[16];
// Called every AI tick; logs a heartbeat row per player every 10 s of game time.
static void __fastcall hook_rm_update(void *rm, void *edx, uint32_t now) {
    (void)edx;
    static uint32_t last[16];
    static uint8_t dumped[16];   // bit0: 10 s snapshot taken, bit1: 60 s snapshot taken
    void *pl = rm_player(rm);
    if (pl) {
        unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
        ctl_note_player(pl);
        if (g_settings_done[idx] != pl) {   // the personality first: its [style] (ai_style=1) is part of the settings
            g_settings_done[idx] = pl;
            persona_assign(pl);
            settings_apply(pl);
        }
        static int traced;
        if (!traced) {   // one-time EBP-chain walk to find the game's main loop
            traced = 1;
            uintptr_t *fp = (uintptr_t *)__builtin_frame_address(0);
            char line[512];
            int n = wsprintfA(line, "# stack (thread %lu):", GetCurrentThreadId());
            for (int d = 0; d < 16 && fp; d++) {
                uintptr_t pair[2];
                SIZE_T got;
                if (!ReadProcessMemory(GetCurrentProcess(), fp, pair, sizeof pair, &got)) break;
                n += wsprintfA(line + n, " %08lx", (unsigned long)pair[1]);
                if (pair[0] <= (uintptr_t)fp) break;
                fp = (uintptr_t *)pair[0];
            }
            if (g_log) { EnterCriticalSection(&g_lock); fprintf(g_log, "%s\n", line); fflush(g_log); LeaveCriticalSection(&g_lock); }
        }
        if (now < 5000) dumped[idx] = 0;   // new game in the same session

        // Diagnostics: log the managers' one-time setup flags at game start. (An early theory blamed
        // them for the scenario bug - wrong; the real cause is the scenario flag, see hook_cp_update.)
        static uint8_t fixed[16];
        if (now >= 5000) fixed[idx] = 0;
        void *cm = FIELD(rm, RM_OWNER, void *);
        if (now < 2000 && !fixed[idx]) {
            fixed[idx] = 1;
            uint8_t *flags[3] = {
                (uint8_t *)cm + CM_SETUP_DONE,
                (uint8_t *)FIELD(cm, CM_BUILD_MGR, void *) + BM_INITIALIZED,
                (uint8_t *)FIELD(cm, CM_RESEARCH_MGR, void *) + RS_INITIALIZED,
            };
            if (g_log) {
                EnterCriticalSection(&g_lock);
                fprintf(g_log, "# start: P%d t=%u flags cm75=%d bm148=%d rs118=%d%s\n",
                        idx, now, *flags[0], *flags[1], *flags[2], FIX_CLEAR_SETUP_FLAGS ? " -> cleared" : "");
                fflush(g_log);
                LeaveCriticalSection(&g_lock);
            }
#if FIX_CLEAR_SETUP_FLAGS
            for (int i = 0; i < 3; i++) *flags[i] = 0;
#endif
            // Experiment: 0x918819 = "scenario/campaign game" (settings+0x409, copied from gamestate+0x411,
            // which the scenario branch of the game start sets). Several AI routines check it.
            autotest_first_tick(pl);   // test overrides (after settings_apply below ran once)
            if (g_clear_scenario_flag && *(uint8_t *)G_SCENARIO_FLAG) {
                *(uint8_t *)G_SCENARIO_FLAG = 0;
                log_line("# experiment: cleared scenario flag 0x918819%s%.0d\n", "", 0);
            }
        }
        if (!(dumped[idx] & 1) && now >= 10000) { dumped[idx] |= 1; dump_player(rm, now); }
        if (!(dumped[idx] & 2) && now >= 60000) { dumped[idx] |= 2; dump_player(rm, now); }
        if (now < last[idx] || now - last[idx] >= 10000) {
            last[idx] = now;
            log_state(rm, "tick", now);
        }
    }
    ((RmUpdateFn)FN_RM_UPDATE)(rm, now);
    if (pl) { intel_tick(pl, now); econ_tick(pl, now); attack_tick(pl, now); assault_tick(pl, now); landing_tick(pl, now); }
    g_rm_seen = 1;
    autotest_tick(now);
}

// ---- autotest mode (aimod_autotest.ini next to the DLL) ----
// The game itself accepts:  "Empire Earth.exe" -loadgame "name.ees"   (file in Data\Saved Games\,
// name must not contain '-' or '/'). This mode adds: skip intro movies, quit after N game minutes.
#define INTRO_LIST_CALLS { 0x00538909, 0x0053892A, 0x00538945 }  // call AddMovie(path) in PlayIntro 0x538867
#define FN_ADD_MOVIE     0x00441432                               // __thiscall, ret 4
#define G_GAME_SPEED     0x009183C0   // float; set like EEMGameSpeed::Apply (0x4B6BD4)
#define G_SPEED_NOTIFY   0x00918300   // object; vtable slot 11 called after a speed change
static struct { int enabled, skip_intro, quit_minutes, scenario_mode, turbo, game_seed; float speed; } g_auto;

#include "randmap.c"

static void autotest_first_tick(void *pl) {
    if (g_auto.enabled) randmap_ai_toggles(pl);
}

static void load_autotest(const char *ini) {
    g_auto.enabled = GetFileAttributesA(ini) != INVALID_FILE_ATTRIBUTES;
    if (!g_auto.enabled) return;
    g_auto.skip_intro = GetPrivateProfileIntA("autotest", "skip_intro", 1, ini);
    g_auto.quit_minutes = GetPrivateProfileIntA("autotest", "quit_minutes", 0, ini);
    g_auto.scenario_mode = GetPrivateProfileIntA("autotest", "scenario_mode", 0, ini);
    g_auto.turbo = GetPrivateProfileIntA("autotest", "turbo", 0, ini);
    g_auto.game_seed = GetPrivateProfileIntA("autotest", "game_seed", 0, ini);
    GetPrivateProfileStringA("autotest", "setup", "", g_setup_path, sizeof g_setup_path, ini);
    g_clear_scenario_flag = GetPrivateProfileIntA("autotest", "clear_scenario_flag", 0, ini);
    randmap_load(ini);
    char buf[32];
    GetPrivateProfileStringA("autotest", "speed", "0", buf, sizeof buf, ini);
    g_auto.speed = (float)atof(buf);
}

static void apply_game_speed(float s) {
    *(float *)G_GAME_SPEED = s;
    void *obj = (void *)G_SPEED_NOTIFY;
    ((void (__thiscall *)(void *))(*(void ***)obj)[11])(obj);
}

// Replace `call AddMovie` (callee pops its one argument) with `add esp,4; nop; nop`.
static BOOL skip_intro_movies(void) {
    static const uintptr_t sites[] = INTRO_LIST_CALLS;
    static const uint8_t stub[5] = { 0x83, 0xC4, 0x04, 0x90, 0x90 };
    for (int i = 0; i < 3; i++) {
        uint8_t *p = (uint8_t *)sites[i];
        if (p[0] != 0xE8 || sites[i] + 5 + *(int32_t *)(p + 1) != FN_ADD_MOVIE) return FALSE;
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
#define CALL_PARSE_CMDLINE 0x00535EF2   // call ParseCommandLine(const char *) 0x522A36
#define FN_PARSE_CMDLINE   0x00522A36
#define CALL_LOAD_SAVED    0x00522BF8   // call LoadSavedGameFromCmdline(UString *) 0x521F94
#define FN_LOAD_SAVED      0x00521F94
typedef char (__cdecl *ParseFn)(const char *);
typedef int (__cdecl *LoadFn)(void *);

static void log_line(const char *fmt, const char *a, int b) {
    if (!g_log) return;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, fmt, a, b);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}

static char __cdecl hook_parse_cmdline(const char *cmd) {
    log_line("# cmdline parse: \"%s\"%.0d\n", cmd ? cmd : "(null)", 0);
    char r = ((ParseFn)FN_PARSE_CMDLINE)(cmd);
    log_line("# cmdline parse returned %s%d (1 = continue to main menu)\n", "", r);
    return r;
}

static int __cdecl hook_load_saved(void *name) {
    log_line("# cmdline loadgame: calling loader%s%.0d\n", "", 0);
    int r = ((LoadFn)FN_LOAD_SAVED)(name);
    log_line("# cmdline loadgame: loader returned %s%d\n", "", r);
    return r;
}

// Autotest setup=<file> (path relative to the game folder): control-pipe commands, one per line (# comments), run
// once at 2 s game time, when every AI has started - e.g. spawn lines that turn a map into a test scenario.
// Setup file: control-pipe commands, run at 2 s game time; a line "@<minutes> <command>" waits until that game time.
#define SETUP_MAX 1024
static struct { uint32_t at; char cmd[200]; } g_setup[SETUP_MAX];
static int g_setup_n = -1;
static void run_setup(uint32_t now) {
    static char reply[16384];
    if (g_setup_n < 0) {
        g_setup_n = 0;
        FILE *f = fopen(g_setup_path, "r");
        log_line(f ? "# setup: %s%.0d\n" : "# setup: %s - CANNOT OPEN%.0d\n", g_setup_path, 0);
        if (!f) return;
        char line[512];
        while (fgets(line, sizeof line, f)) {
            char *s = line, *e;
            while (*s == ' ' || *s == '\t') s++;
            for (e = s + strlen(s); e > s && (e[-1] == '\n' || e[-1] == '\r' || e[-1] == ' '); ) *--e = 0;
            if (!*s || *s == '#') continue;
            if (g_setup_n == SETUP_MAX) { log_line("# setup: %s: more than %d commands - the rest left out\n", g_setup_path, SETUP_MAX); break; }
            uint32_t at = 0;
            if (*s == '@') {
                at = (uint32_t)(atof(s + 1) * 60000.0);
                while (*s && *s != ' ') s++;
                while (*s == ' ') s++;
            }
            g_setup[g_setup_n].at = at;
            lstrcpynA(g_setup[g_setup_n++].cmd, s, sizeof g_setup[0].cmd);
        }
        fclose(f);
    }
    for (int i = 0; i < g_setup_n; i++) {
        if (!g_setup[i].cmd[0] || now < g_setup[i].at) continue;
        reply[0] = 0;
        execute(g_setup[i].cmd, reply);
        char *nl = strchr(reply, '\n'), *rest = nl ? nl + 1 : NULL;
        if (nl) *nl = 0;
        if (g_log) {
            EnterCriticalSection(&g_lock);
            fprintf(g_log, "# setup: %s -> %s", g_setup[i].cmd, reply);
            fputc(10, g_log);
            while (rest && *rest) {   // further lines of a multi-line reply, as comments
                nl = strchr(rest, '\n');
                if (nl) *nl = 0;
                fprintf(g_log, "#   %s\n", rest);
                rest = nl ? nl + 1 : NULL;
            }
            fflush(g_log);
            LeaveCriticalSection(&g_lock);
        }
        g_setup[i].cmd[0] = 0;
    }
}

static void autotest_tick(uint32_t now) {
    if (!g_auto.enabled) return;
    if (g_setup_path[0] && now >= 2000) run_setup(now);
    float cur = *(float *)G_GAME_SPEED;
    if (g_auto.speed > 0 && (cur < g_auto.speed - 0.01f || cur > g_auto.speed + 0.01f)) {
        apply_game_speed(g_auto.speed);
        if (g_log) {
            EnterCriticalSection(&g_lock);
            fprintf(g_log, "# autotest: game speed %.2f -> %.2f at t=%u\n", cur, g_auto.speed, now);
            fflush(g_log);
            LeaveCriticalSection(&g_lock);
        }
    }
    if (g_auto.quit_minutes <= 0 || now < (uint32_t)g_auto.quit_minutes * 60000u) return;
    if (g_log) {
        fprintf(g_log, "# autotest: reached %d game minutes, exiting\n", g_auto.quit_minutes);
        fclose(g_log);
        g_log = NULL;
    }
    TerminateProcess(GetCurrentProcess(), 0);   // ExitProcess crashes in the game's other threads
}


// Game start: EEGameState::Start(setup, ?) 0x52599B, thiscall on 0x917588, called once at 0x4CEC14.
// setup+0x3D8 = mode (0 random map, 3 editor test-play, 2 saved game, 4 campaign, other = scenario from
// Scenarios\), setup+0x3A8 = UString file name (char* at +0x3AC). With scenario_mode=1 a -loadgame start
// is switched to the scenario path, loading Scenarios\<name>.scn - the path where editor scenarios break.
#define CALL_GAME_START 0x004CEC14
#define FN_GAME_START   0x0052599B
#define SETUP_MODE      0x3D8
#define SETUP_NAME      0x3AC
typedef char (__thiscall *GameStartFn)(void *state, void *setup, void *p2);

// aimod_econ.csv in normal play: over 20 MB at the start of a session or a game (a long game writes ~10 MB), it
// becomes aimod_econ.old.csv (the older one deleted) and a new log starts - it was appended to forever. Autotest
// keeps the whole log (the tests and scripts parse it).
#define LOG_ROTATE_BYTES (20u << 20)
static char g_log_path[MAX_PATH];
static void log_rotate(void) {
    WIN32_FILE_ATTRIBUTE_DATA fa;
    if (g_auto.enabled || !g_log_path[0] || !GetFileAttributesExA(g_log_path, GetFileExInfoStandard, &fa)
        || (!fa.nFileSizeHigh && fa.nFileSizeLow < LOG_ROTATE_BYTES)) return;
    char old[MAX_PATH];
    lstrcpynA(old, g_log_path, sizeof old - 8);
    char *dot = strrchr(old, '.');
    if (!dot) return;
    lstrcpyA(dot, ".old.csv");
    EnterCriticalSection(&g_lock);
    int was_open = g_log != NULL;
    if (g_log) { fclose(g_log); g_log = NULL; }
    BOOL ok = MoveFileExA(g_log_path, old, MOVEFILE_REPLACE_EXISTING);
    if (was_open) g_log = fopen(g_log_path, "a");
    if (g_log) {
        fprintf(g_log, "# log: %lu MB moved to %s%s\n", fa.nFileSizeLow >> 20, old, ok ? "" : " - FAILED");
        fflush(g_log);
    }
    LeaveCriticalSection(&g_lock);
}

static char __fastcall hook_game_start(void *state, void *edx, uint8_t *setup, void *p2) {
    (void)edx;
    char *name = *(char **)(setup + SETUP_NAME);
    int mode = *(int *)(setup + SETUP_MODE);
    log_rotate();
    if (g_log) {
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# game start: mode=%d name=\"%s\" game_type=%d\n", mode, name ? name : "(null)", *(int *)0x918BF4);
        // raw setup struct (16 player slots of 0x38 bytes, then game settings up to ~0x410)
        for (int o = 0; o < 0x420; o += 32) {
            fprintf(g_log, "# setup +%03x:", o);
            for (int k = 0; k < 32; k++) fprintf(g_log, "%s%02x", (k & 3) ? "" : " ", setup[o + k]);
            fputc('\n', g_log);
        }
        for (int i = 0; i < 16; i++) {   // slot name = UString at slot+8 (char* at +0xC)
            const char *pn = *(const char **)(setup + i * 0x38 + 0xC);
            fprintf(g_log, "# setup slot %d type=%d name=\"%s\"\n", i, *(int *)(setup + i * 0x38), pn ? pn : "");
        }
        fprintf(g_log, "# setup seed(world+0x54)=%u\n", *(void **)0x0091977C ? *(unsigned *)(*(uint8_t **)0x0091977C + 0x54) : 0);
        fflush(g_log);
        LeaveCriticalSection(&g_lock);
    }
    if (g_auto.enabled && g_rm.map[0] && mode == 2) randmap_rewrite(setup);
    if (g_auto.enabled && g_auto.scenario_mode && mode == 2 && name) {
        size_t n = strlen(name);
        if (n > 4 && !lstrcmpiA(name + n - 4, ".ees")) memcpy(name + n - 4, ".scn", 4);   // same length, in place
        *(int *)(setup + SETUP_MODE) = 1;
        log_line("# game start: switched to scenario mode, loading Scenarios\\%s%.0d\n", name, 0);
    }
    // the last game's AI players are gone: a second game in one session (loading capittest after an autosave)
    // crashed in world_scan on a stale g_players entry for a slot the new game has no AI in
    memset(g_players, 0, sizeof g_players);
    memset(g_settings_done, 0, sizeof g_settings_done);
    persona_new_game(setup);   // personalities and their colours, before the game creates the players
    diplo_new_game();
    chat_new_game();
    llm_new_game();
    intel_new_game(mode == 2);
    // the systems' per-game state: game times start over and the units, subgroups and players are new objects that
    // can reuse the last game's addresses (the "time went backwards" checks in the ticks stay as a fallback)
    econ_new_game();
    attack_new_game();
    aim_new_game();
    landing_new_game();
    bunker_new_game();
    defence_new_game();
    assault_new_game();
    sites_new_game();
    if (mode == 2) persist_load(name);   // a saved game: the mod's state from its .aimod file
    char r = ((GameStartFn)FN_GAME_START)(state, setup, p2);
    log_line("# game start returned %s%d\n", "", r);
    return r;
}

#include "control.c"
#include "watch.c"
#include "trace.c"
#include "settings.c"
#include "chat.c"
#include "scen.c"
#include "sites.c"
#include "econ.c"
#include "attack.c"
#include "stats.c"
#include "defence.c"
#include "assault.c"
#include "landing.c"
#include "bunker.c"
#include "intel.c"
#include "expr.c"
#include "persona.c"
#include "texts.c"
#include "diplo.c"
#include "persist.c"
#include "aim.c"
#include "camera.c"
#include "display.c"
#include "sound.c"
#include "select.c"
#include "drawlist.c"
#include "llm.c"
static char g_ini[MAX_PATH], g_aimod_ini[MAX_PATH];

// Redirect an E8 rel32 call, verifying it currently targets `expected`.
static BOOL patch_call(uintptr_t site, uintptr_t expected, void *hook) {
    uint8_t *p = (uint8_t *)site;
    if (p[0] != 0xE8 || site + 5 + *(int32_t *)(p + 1) != expected) return FALSE;
    DWORD old;
    if (!VirtualProtect(p, 5, PAGE_EXECUTE_READWRITE, &old)) return FALSE;
    *(int32_t *)(p + 1) = (int32_t)((uintptr_t)hook - (site + 5));
    VirtualProtect(p, 5, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 5);
    return TRUE;
}

static void log_header(void) {
    fputs("time_ms,player,phase,difficulty,cheat", g_log);
    for (int i = 0; i < 7; i++) fprintf(g_log, ",stock%d", i);
    for (int i = 0; i < 7; i++) fprintf(g_log, ",demand%d", i);
    for (int i = 0; i < 7; i++) fprintf(g_log, ",target%d", i);
    for (int i = 0; i < 7; i++) fprintf(g_log, ",list%d", i);
    fputs(",rm_init,t_farm,t_20s,t_targets,t_assign,t_cleanup"
          ",cm_project,cm_t_epoch,cm_t_bcount,cm_t_milbldg,cm_t_build,cm_t_e817,cm_t_59s,cm_flags74"
          ",toggles_ef4,epoch\n", g_log);
}

static BOOL is_game_process(void) {
    char path[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);
    const char *name = strrchr(path, '\\');
    if (name && lstrcmpiA(name + 1, "Empire Earth.exe") == 0) return TRUE;
    // a renamed copy of the 1.04 exe (same build timestamp as the CD/GOG/Steam 1.04 exe)
    uint8_t *base = (uint8_t *)GetModuleHandleA(NULL);
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + ((IMAGE_DOS_HEADER *)base)->e_lfanew);
    return nt->FileHeader.TimeDateStamp == 0x594D0000;
}

// No aimod.ini next to the DLL: tell the player and stop, instead of running on built-in defaults nobody
// tested as a whole (the defaults in the code are only for keys missing from an existing, older file).
// Runs inside DllMain: the engine loads its DLLs at runtime (user32 is up), and TerminateProcess skips the
// other DLLs' detach handlers, which would run under the loader lock.
static void config_missing(const char *ini) {
    char msg[MAX_PATH + 512];
    wsprintfA(msg, "The Empire Earth AI mod (eeaimod.dll) needs its settings file:\n\n%s\n\n"
                   "Copy aimod.ini from the mod's config folder next to Empire Earth.exe, or delete eeaimod.dll "
                   "to play without the mod.\n\nEmpire Earth will close now.", ini);
    MessageBoxA(NULL, msg, "Empire Earth AI mod: aimod.ini missing", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
    TerminateProcess(GetCurrentProcess(), 1);
}

static void install(HMODULE self) {
    // The engine may FreeLibrary() DLLs that aren't render plugins; our hooks must outlive that.
    HMODULE pinned;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                       (LPCSTR)&install, &pinned);

    char path[MAX_PATH];
    GetModuleFileNameA(self, path, MAX_PATH);
    char *slash = strrchr(path, '\\');
    if (slash) {
        *slash = 0;
        wsprintfA(g_dump_dir, "%s\\aimod_dumps", path);
        *slash = '\\';
        lstrcpyA(slash + 1, "aimod.ini");   // required: missing -> error and quit; [general] enabled=0 -> no hooks
        if (GetFileAttributesA(path) == INVALID_FILE_ATTRIBUTES) config_missing(path);
        if (!GetPrivateProfileIntA("general", "enabled", 1, path)) {
            lstrcpyA(slash + 1, "aimod_econ.csv");
            FILE *f = fopen(path, "a");
            if (f) { fputs("# eeaimod: switched off in aimod.ini (enabled=0) - nothing installed\n", f); fclose(f); }
            return;
        }
        g_fix_scenario_ai = GetPrivateProfileIntA("general", "scenario_ai", GetPrivateProfileIntA("fixes", "scenario_ai", 1, path), path);
        lstrcpyA(g_aimod_ini, path);
        lstrcpyA(slash + 1, "aimod_autotest.ini");
        load_autotest(path);
        lstrcpyA(g_ini, path);
        lstrcpyA(slash + 1, "aimod_econ.csv");
        g_mem_dumps = g_auto.enabled || GetPrivateProfileIntA("general", "memory_dumps", 0, g_aimod_ini);
        if (g_mem_dumps || g_auto.enabled || GetPrivateProfileIntA("general", "control_pipe", 0, g_aimod_ini))
            CreateDirectoryA(g_dump_dir, NULL);   // memory dumps, shot / ddinfo pictures
        lstrcpynA(g_log_path, path, sizeof g_log_path);
    }
    g_session = GetTickCount();
    InitializeCriticalSection(&g_lock);
    log_rotate();
    g_log = fopen(path, "a");
    if (g_log) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(g_log, "# session %04d-%02d-%02d %02d:%02d:%02d\n",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        log_header();
    }

    BOOL a = patch_call(CALL_COMPUTE_TARGETS, FN_COMPUTE_TARGETS, hook_compute_targets);
    BOOL b = patch_call(CALL_ASSIGN_CITIZENS, FN_ASSIGN_CITIZENS, hook_assign_citizens);
    BOOL c = patch_call(CALL_RM_UPDATE, FN_RM_UPDATE, hook_rm_update);
    BOOL f = patch_call(CALL_CP_UPDATE, FN_CP_UPDATE, hook_cp_update);
    BOOL gs = patch_call(CALL_GAME_START, FN_GAME_START, hook_game_start);   // also personalities: always
    if (g_log) {
        fprintf(g_log, "# eeaimod loaded: hook targets=%s assign=%s update=%s\n",
                a ? "ok" : "FAILED", b ? "ok" : "FAILED", c ? "ok" : "FAILED");
        fprintf(g_log, "# fix scenario_ai: %s (hook %s)\n", g_fix_scenario_ai ? "on" : "off", f ? "ok" : "FAILED");
        BOOL d = patch_call(CALL_PARSE_CMDLINE, FN_PARSE_CMDLINE, hook_parse_cmdline);
        BOOL e = patch_call(CALL_LOAD_SAVED, FN_LOAD_SAVED, hook_load_saved);
        fprintf(g_log, "# game start hook: %s\n", gs ? "ok" : "FAILED");
        fprintf(g_log, "# cmdline hooks: parse=%s load=%s\n", d ? "ok" : "FAILED", e ? "ok" : "FAILED");
        if (g_auto.enabled)
            fprintf(g_log, "# autotest: skip_intro=%s quit_minutes=%d cmdline=%s\n",
                    g_auto.skip_intro ? (skip_intro_movies() ? "ok" : "FAILED") : "off",
                    g_auto.quit_minutes, GetCommandLineA());
        fflush(g_log);
    }
    // normal play: aimod.ini [general] skip_intro = 1 skips the intro movies (autotest has its own key)
    if (!g_auto.enabled && GetPrivateProfileIntA("general", "skip_intro", 0, g_aimod_ini)) {
        BOOL ok = skip_intro_movies();
        if (g_log) fprintf(g_log, "# skip_intro: %s\n", ok ? "ok" : "FAILED");
    }
    econ_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    attack_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    assault_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    landing_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    intel_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    defence_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    chat_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    persona_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    diplo_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    persist_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    aim_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    camera_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    display_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    sound_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    select_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    drawlist_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    llm_install(g_aimod_ini, g_auto.enabled ? g_ini : NULL);
    ctl_start(g_auto.enabled || GetPrivateProfileIntA("general", "control_pipe", 0, g_aimod_ini));
    if (g_auto.enabled && g_auto.game_seed) {
        BOOL ok = patch_srand();
        if (g_log) fprintf(g_log, "# autotest: game_seed=%d (srand hook %s)\n", g_auto.game_seed, ok ? "ok" : "FAILED");
    }
    if (g_auto.enabled) { watch_start(g_ini); trace_start(g_ini); }
    AddVectoredExceptionHandler(1, crash_handler);   // logs the first exceptions (module+offset, backtrace) - also in normal play
}

// Autotest game_seed: starting a game from the menu (EEUIELEStart::vf1 0x5C9B91, 0x67E1D6) seeds the C library
// with the clock, srand(TMGetTime()), and sends rand() along with the start message (0x5D, +0x14) - the game's
// random seed, so random-map runs differ even with the same map seed. With game_seed every srand uses that seed.
#define IAT_SRAND 0x00824644
typedef void (__cdecl *SrandFn)(unsigned);
static SrandFn g_srand;
static void __cdecl hook_srand(unsigned seed) {
    (void)seed;
    g_srand((unsigned)g_auto.game_seed);
}

static BOOL patch_srand(void) {
    void **p = (void **)IAT_SRAND;
    DWORD old;
    if (!VirtualProtect(p, 4, PAGE_READWRITE, &old)) return FALSE;
    g_srand = (SrandFn)*p;
    *p = (void *)hook_srand;
    VirtualProtect(p, 4, old, &old);
    return g_srand != NULL;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(inst);
        if (is_game_process()) install(inst);
    }
    return TRUE;
}
