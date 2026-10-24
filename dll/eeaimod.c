// eeaimod.dll - Empire Earth AI mod.
// Loaded automatically: Low-Level Engine.dll LoadLibrary()s every *.dll in the game folder.
//
// This file: the whole DLL put together - every part is a .c file of its folder (base/ the DLL's own helpers, game/
// the game's addresses, objects and types, engine/ fixes and features of the game, ai/ the computer players, script/
// the text language, voice/ what the AI says, test/ the pipe and the tests), #included below in one translation unit
// (README.md). Here: the game start, the AI's update hooks that call every part, install and DllMain.

#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <math.h>
#include <tlhelp32.h>
#include <io.h>      // _chsize (log_rotate)
#include <psapi.h>   // types only (K32GetProcessMemoryInfo by GetProcAddress)

#include "base/lang.h"   // the C the code is written in: internal, PRINTF_LIKE
struct ustring;
#include "base/log.c"
#include "base/patch.c"
#include "base/reply.c"
#include "base/paths.c"
#include "base/mistakes.c"
#include "base/math.c"
#include "base/vec.h"             // vec2, vec3, ivec2: positions and tiles as one value
#include "game/api/api.c"         // the game's code and objects: typed functions, data accessors, hooks (game/api/)
#include "game/players.c"
#include "game/tech.c"      // a tech node's children (kept differently by build)
#include "game/random.c"
#include "voice/say.h"   // the AI's door to what it says (voice/)


internal void autotest_tick(uint32_t now);
internal volatile int g_rm_seen;   // an AI resource manager has ticked (else the sim loop drives autotest_tick)
internal void execute(const char *cmd, char *out);
internal char g_setup_path[MAX_PATH];
internal void autotest_first_tick(ee_player *pl);
internal void settings_apply(ee_player *pl);
internal void econ_tick(ee_player *pl, uint32_t now);
internal void persona_new_game(ee_setup *setup);
internal void scripts_new_game(ee_setup *setup);   // test/scripts.c
internal void persona_assign(ee_player *pl);
internal void diplo_new_game(void);
internal void chat_new_game(void);
internal void llm_new_game(void);
internal void shipturns_new_game(void);   // engine/shiproute.c
internal void collide_new_game(void);      // engine/collide.c
internal void persist_load(const char *name);
internal void econ_targets(ee_resource_mgr *rm);
internal void econ_rebalance(ee_resource_mgr *rm);
internal void econ_fields(ee_resource_mgr *rm);
internal void food_hunt(ee_resource_mgr *rm);   // food.c
internal void food_pull(ee_resource_mgr *rm);
internal void food_new_game(void);
internal void mine_fill(ee_resource_mgr *rm);   // mines.c
internal void mines_new_game(void);
internal void cz_targets(ee_resource_mgr *rm);   // citizens.c
internal void wf_targets(ee_resource_mgr *rm);   // citizens.c: workforce_plan
internal void wf_tick(ee_player *pl, uint32_t now);
internal void wf_new_game(void);
internal void ap_targets(ee_resource_mgr *rm);   // army.c
internal void citizens_safety(ee_resource_mgr *rm);
internal void attack_tick(ee_player *pl, uint32_t now);
internal void vanilla_tick(ee_player *pl, uint32_t now);
internal void ecolog_tick(ee_player *pl, uint32_t now);   // ecolog.c: the economy log (autotest)
internal void ecolog_new_game(void);
internal void cz_tick(ee_player *pl, uint32_t now);   // citizens.c: citizen production
internal void cz_new_game(void);
internal void am_tick(ee_player *pl, uint32_t now);   // army.c: army spending
internal void mt_tick(ee_player *pl, uint32_t now);   // metal.c: gold and iron
internal void mt_install(const char *autotest_ini);
internal void am_new_game(void);
internal void mass_tick(ee_player *pl, uint32_t now);   // mass.c: the army as one
internal void mass_new_game(void);
internal void mass_install(const char *autotest_ini);
internal void cover_tick(ee_player *pl, uint32_t now);   // cover.c: fall back to cover
internal void cover_new_game(void);
internal void cover_install(const char *autotest_ini);
internal void expand_tick(ee_player *pl, uint32_t now);   // expand.c: other islands
internal void expand_new_game(void);
internal void expand_install(const char *autotest_ini);
internal void tw_tick(ee_player *pl, uint32_t now);   // transports.c: the transports' script states (a test's log)
internal void tw_new_game(void);
internal void tw_install(const char *autotest_ini);
internal void ap_new_game(void);
internal void mt_new_game(void);   // metal.c
internal void army_plan_tick(ee_player *pl, unsigned idx, uint32_t now);
internal void ap_install(const char *autotest_ini);
internal void early_new_game(void);   // early.c
internal void landing_tick(ee_player *pl, uint32_t now);
internal void navy_tick(ee_player *pl, uint32_t now);
internal void intel_tick(ee_player *pl, uint32_t now);
internal void intel_new_game(int from_save);
internal void econ_new_game(void);
internal void attack_new_game(void);
internal void aim_new_game(void);
internal void landing_new_game(void);
internal void navy_new_game(void);
internal void bunker_new_game(void);
internal void defence_new_game(void);
internal void sites_new_game(void);
internal void intel_install(void);
internal void landing_install(void);
internal void navy_install(const char *autotest_ini);
internal PRINTF_LIKE(1, 2) void log_line(const char *fmt, ...);
internal BOOL install_srand(void);
#include "base/ini.c"
#include "settings/settings.c"   // the settings: aimod.ini's keys as rows, read once into g_set
#include "engine/scenario_ai.c"
#include "game/world.c"
#include "ai/econ_rows.c"

internal char __thiscall hook_compute_targets(ee_resource_mgr *rm) {
    log_state(rm, "pre_targets", game_time());
    char changed = rm_compute_targets(rm);
    if (!is_vanilla(rm_player(rm))) { econ_targets(rm); mine_fill(rm); cz_targets(rm); wf_targets(rm); ap_targets(rm); }   // mines.c: every mine slot; citizens.c
    log_state(rm, "post_targets", game_time());
    return changed;
}

internal char __thiscall hook_assign_citizens(ee_resource_mgr *rm) {
    if (is_vanilla(rm_player(rm))) {
        char r = rm_assign_citizens(rm);
        log_state(rm, "post_assign", game_time());
        return r;
    }
    econ_fields(rm);   // farm fields (econ.c)
    citizens_safety(rm);   // away from known enemy towers (attack.c)
    food_hunt(rm);   // idle citizens to animals (food.c)
    char r = rm_assign_citizens(rm);
    econ_rebalance(rm);
    food_pull(rm);   // gatherers back to food (food.c)
    log_state(rm, "post_assign", game_time());
    return r;
}

#include "test/memdump.c"


// the player each AI's personality and settings were applied to (reset by hook_game_start: the next game's player
// can sit at the same address - its settings were skipped, a hard AI kept the game's cheat)
internal void *g_settings_done[16];
// Called every AI tick; logs a heartbeat row per player every 10 s of game time.
internal void __thiscall hook_rm_update(ee_resource_mgr *rm, uint32_t now) {
    static uint32_t last[16];
    static uint8_t dumped[16];   // bit0: 10 s snapshot taken, bit1: 60 s snapshot taken
    ee_player *pl = rm_player(rm);
    if (pl) {
        unsigned idx = (unsigned)player_get_index(pl) & 15;
        ctl_note_player(pl);
        if (g_settings_done[idx] != pl) {   // the personality first: its style (ai_style=1) is part of the settings
            g_settings_done[idx] = pl;
            if (!is_vanilla(pl)) {
                persona_assign(pl);
                settings_apply(pl);
            }
        }
        if (now < 5000) dumped[idx] = 0;   // new game in the same session

        // Diagnostics: log the managers' one-time setup flags at game start. (An early theory blamed
        // them for the scenario bug - wrong; the real cause is the scenario flag, see hook_cp_update.)
        static uint8_t fixed[16];
        if (now >= 5000) fixed[idx] = 0;
        void *cm = rm_get_owner(rm);
        if (now < 2000 && !fixed[idx]) {
            fixed[idx] = 1;
            uint8_t *flags[3] = {
                cm_setup_done_at(cm),
                bm_initialized_at(cm_get_build_mgr(cm)),
                rs_initialized_at(cm_get_research_mgr(cm)),
            };
            if (g_log) {
                EnterCriticalSection(&g_lock);
                fprintf(g_log, "# start: P%d t=%u flags cm75=%d bm148=%d rs118=%d\n",
                        idx, now, *flags[0], *flags[1], *flags[2]);
                fflush(g_log);
                LeaveCriticalSection(&g_lock);
            }
            autotest_first_tick(pl);   // test overrides (after settings_apply above)
        }
        if (!(dumped[idx] & 1) && now >= 10000) { dumped[idx] |= 1; dump_player(rm, now); }
        if (!(dumped[idx] & 2) && now >= 60000) { dumped[idx] |= 2; dump_player(rm, now); }
        if (now < last[idx] || now - last[idx] >= 10000) {
            last[idx] = now;
            log_state(rm, "tick", now);
        }
    }
    rm_update(rm, now);
    if (pl && is_vanilla(pl)) vanilla_tick(pl, now);   // the game's own AI: only the mod's log lines (attack.c)
    else if (pl) { intel_tick(pl, now); econ_tick(pl, now); attack_tick(pl, now); landing_tick(pl, now); navy_tick(pl, now);
                wf_tick(pl, now); army_plan_tick(pl, (unsigned)player_get_index(pl) & 15, now); }
    if (pl) ecolog_tick(pl, now);
    if (pl) cz_tick(pl, now);
    if (pl) am_tick(pl, now);
    if (pl) mt_tick(pl, now);
    if (pl) mass_tick(pl, now);
    if (pl) cover_tick(pl, now);
    if (pl) expand_tick(pl, now);
    if (pl) tw_tick(pl, now);
    g_rm_seen = 1;
    autotest_tick(now);
}

#include "test/autotest.c"


// Game start: EEGameState::Start(setup, ?) 0x52599B, thiscall on 0x917588, called once at 0x4CEC14.
// setup+0x3D8 = mode (0 random map, 3 editor test-play, 2 saved game, 4 campaign, other = scenario from
// Scenarios\), setup+0x3A8 = UString file name (char* at +0x3AC). With scenario_mode=1 a -loadgame start
// is switched to the scenario path, loading Scenarios\<name>.scn - the path where editor scenarios break.

// aimod_econ.csv in normal play: over 20 MB at the start of a session or a game (a long game writes ~10 MB), it
// becomes aimod_econ.old.csv (the older one deleted) and a new log starts - it was appended to forever. Autotest
// keeps the whole log (the tests and scripts parse it). At a game start the log is open and other threads write to
// it (the render and window threads, the pipe, the crash handler - not all under g_lock): it is copied and cut to 0
// in place, the FILE stays the same (closed and reopened, a writer without the lock wrote into a freed FILE). The
// CRT locks the stream for each call; _lock_file keeps the others out between the copy and the cut.
#define LOG_ROTATE_BYTES (20u << 20)
internal char g_log_path[MAX_PATH];
internal BOOL log_copy(const char *from, const char *to) {   // CopyFile can't: the log is open for writing
    HANDLE a = CreateFileA(from, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (a == INVALID_HANDLE_VALUE) return FALSE;
    HANDLE b = CreateFileA(to, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    BOOL ok = b != INVALID_HANDLE_VALUE;
    static char buf[65536];
    DWORD got, put;
    while (ok && ReadFile(a, buf, sizeof buf, &got, NULL) && got) ok = WriteFile(b, buf, got, &put, NULL) && put == got;
    if (b != INVALID_HANDLE_VALUE) CloseHandle(b);
    CloseHandle(a);
    return ok;
}
internal void log_rotate(void) {
    WIN32_FILE_ATTRIBUTE_DATA fa;
    if (g_autotest || !g_log_path[0] || !GetFileAttributesExA(g_log_path, GetFileExInfoStandard, &fa)
        || (!fa.nFileSizeHigh && fa.nFileSizeLow < LOG_ROTATE_BYTES)) return;
    char old[MAX_PATH];
    lstrcpynA(old, g_log_path, sizeof old - 8);
    char *dot = strrchr(old, '.');
    if (!dot) return;
    lstrcpyA(dot, ".old.csv");
    EnterCriticalSection(&g_lock);
    BOOL ok;
    if (!g_log) ok = MoveFileExA(g_log_path, old, MOVEFILE_REPLACE_EXISTING);   // the session start: not open yet
    else {
        _lock_file(g_log);
        fflush(g_log);
        ok = log_copy(g_log_path, old) && !_chsize(_fileno(g_log), 0);   // "a": the next write goes to the (new) end
        _unlock_file(g_log);
    }
    if (g_log) {
        fprintf(g_log, "# log: %lu MB moved to %s%s\n", fa.nFileSizeLow >> 20, old, ok ? "" : " - FAILED");
        fflush(g_log);
    }
    LeaveCriticalSection(&g_lock);
}

internal int unit_types_load(void);   // unitnames.c
internal char __thiscall hook_game_start(void *state, ee_setup *setup, void *p2) {
    char *name = FIELD(setup_map_name_at(setup), 4, char *);   // the UString's characters
    int mode = setup_get_mode(setup);
    log_rotate();
    if (g_log) {
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# game start: mode=%d name=\"%s\" game_type=%d\n", mode, name ? name : "(null)", *game_type_var());
        // raw setup struct (16 player slots of 0x38 bytes, then game settings up to ~0x410)
        for (int o = 0; o < 0x420; o += 32) {
            fprintf(g_log, "# setup +%03x:", o);
            for (int k = 0; k < 32; k++) fprintf(g_log, "%s%02x", (k & 3) ? "" : " ", ((uint8_t *)setup)[o + k]);
            fputc('\n', g_log);
        }
        for (int i = 0; i < 16; i++) {   // the slot's name: a UString (its characters at +4)
            const char *pn = FIELD(slot_name_at(setup_slot_at(setup, i)), 4, const char *);
            fprintf(g_log, "# setup slot %d type=%d name=\"%s\"\n", i, slot_get_type(setup_slot_at(setup, i)), pn ? pn : "");
        }
        fprintf(g_log, "# setup seed(world+0x54)=%u\n", game_world() ? world_get_seed(game_world()) : 0);
        fflush(g_log);
        LeaveCriticalSection(&g_lock);
    }
    if (g_autotest && g_rm.map[0] && mode == 2) randmap_rewrite(setup);
    if (g_autotest && g_auto.scenario_mode && mode == 2 && name) {
        size_t n = strlen(name);
        if (n > 4 && !lstrcmpiA(name + n - 4, ".ees")) memcpy(name + n - 4, ".scn", 4);   // same length, in place
        setup_set_mode(setup, 1);
        log_line("# game start: switched to scenario mode, loading Scenarios\\%s\n", name);
    }
    unit_types_load();   // the game's types by name (UT(B_FARM) ...) before any AI code runs - the data is loaded by now
    // the last game's AI players are gone: a second game in one session (loading capittest after an autosave)
    // crashed in world_scan on a stale g_players entry for a slot the new game has no AI in
    memset(g_players, 0, sizeof g_players);
    memset(g_settings_done, 0, sizeof g_settings_done);
    scripts_new_game(setup);   // a test's scripts: their game.setup blocks choose for the planning below
    persona_new_game(setup);   // personalities and their colours, before the game creates the players
    diplo_new_game();
    chat_new_game();
    llm_new_game();
    intel_new_game(mode == 2);
    // the systems' per-game state: game times start over and the units, subgroups and players are new objects that
    // can reuse the last game's addresses (the "time went backwards" checks in the ticks stay as a fallback)
    econ_new_game();
    ecolog_new_game();
    attack_new_game();
    aim_new_game();
    shipturns_new_game();
    collide_new_game();
    landing_new_game();
    navy_new_game();
    bunker_new_game();
    early_new_game();
    food_new_game();
    mines_new_game();
    cz_new_game();
    wf_new_game();
    am_new_game();
    mass_new_game();
    cover_new_game();
    expand_new_game();
    tw_new_game();
    ap_new_game();
    mt_new_game();
    defence_new_game();
    sites_new_game();
    if (mode == 2) persist_load(name);   // a saved game: the mod's state from its .aimod file
    char r = game_start(state, setup, p2);
    log_line("# game start returned %d\n", r);
    return r;
}

#include "game/unitnames.c"
#include "game/roles.c"
#include "test/control.c"
#include "game/grid.c"
#include "base/watch.c"
#include "test/trace.c"
#include "ai/settings.c"
#include "voice/chat.c"
#include "test/scen.c"
#include "ai/sites.c"
#include "ai/econ.c"
#include "ai/attack.c"
#include "ai/stats.c"
#include "ai/defence.c"
#include "ai/forces.c"
#include "ai/landing.c"
#include "ai/bunker.c"
#include "ai/early.c"
#include "ai/food.c"
#include "ai/mines.c"
#include "ai/citizens.c"
#include "ai/army.c"
#include "ai/intel.c"
#include "script/expr.c"
#include "game_script/constants.c"
#include "voice/persona.c"
#include "voice/texts.c"
#include "ai/diplo.c"
#include "game_script/relations.c"
#include "test/scripts.c"
#include "ai/navy.c"
#include "ai/fleet.c"
#include "ai/mass.c"
#include "ai/expand.c"
#include "ai/transports.c"
#include "ai/metal.c"
#include "ai/cover.c"
#include "persist.c"
#include "engine/aim.c"
#include "engine/camera.c"
#include "base/headless_win.c"
#include "engine/display.c"
#include "engine/playername.c"
#include "engine/sound.c"
#include "engine/select.c"
#include "engine/shiproute.c"
#include "engine/turns.c"
#include "engine/collide.c"
#include "engine/groupmove.c"
#include "engine/drawlist.c"
#include "engine/sight.c"
#include "engine/asg_queue.c"
#include "voice/llm.c"
#include "test/testcmd.c"
#include "test/testopp.c"
#include "test/fleetmove.c"
#include "test/ecolog.c"


// No aimod.ini next to the DLL: tell the player and stop, instead of running on built-in defaults nobody
// tested as a whole (the defaults in the code are only for keys missing from an existing, older file).
// Runs inside DllMain: the engine loads its DLLs at runtime (user32 is up), and TerminateProcess skips the
// other DLLs' detach handlers, which would run under the loader lock.
internal void config_missing(const char *ini) {
    char msg[MAX_PATH + 512];
    wsprintfA(msg, "The Empire Earth AI mod (eeaimod.dll) needs its settings file:\n\n%s\n\n"
                   "Copy aimod.ini from the mod's config folder next to Empire Earth.exe, or delete eeaimod.dll "
                   "to play without the mod.\n\nEmpire Earth will close now.", ini);
    if (!g_autotest)   // a headless test has nobody to click the box: it quits at once (run_test.ps1 checks first)
        MessageBoxA(NULL, msg, "Empire Earth AI mod: aimod.ini missing", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
    TerminateProcess(GetCurrentProcess(), 1);
}

// Autotest: [autotest] config = the aimod.ini to play with, ai_dir = the personality folder (aimod_ai), each absolute
// or relative to the game folder; unset = the files next to the game as in normal play. Every reader of the settings
// goes through g_aimod_ini / g_ai_dir, so the whole file ([general], [all], the difficulties, [display], [llm] ...)
// comes from the chosen one. The personality folder otherwise: aimod.ini [general] ai_dir (the same path rules), so a
// developer's game reads the repository's files (ai_dir = _aimod\config\aimod_ai) instead of a copy. A folder that
// isn't there: the one next to the game, told in the log (g_ai_dir_why). config_paths picks the file, config_ai_dir
// the folder once the settings are read.
internal char g_ai_dir_why[MAX_PATH + 64];
internal void game_path(const char *v, char *out, int cap) {   // absolute, or relative to the game folder; '/' as '\'
    if (v[1] == ':' || (v[0] == '\\' && v[1] == '\\')) lstrcpynA(out, v, cap);
    else snprintf(out, cap, "%s%s", g_game_dir, v);
    for (char *c = out; *c; c++) if (*c == '/') *c = '\\';
}
internal void config_paths(void) {
    char v[MAX_PATH];
    if (g_autotest) {
        GetPrivateProfileStringA("autotest", "config", "", v, sizeof v, g_ini);
        if (v[0]) game_path(v, g_aimod_ini, MAX_PATH);
    }
}
internal void config_ai_dir(void) {
    if (!g_set.ai_dir[0]) return;
    const char *from = settings_section(SETTING_ai_dir);
    char dir[MAX_PATH];
    game_path(g_set.ai_dir, dir, MAX_PATH - 1);
    size_t n = strlen(dir);
    while (n > 3 && (dir[n - 1] == '\\' || dir[n - 1] == ' ')) dir[--n] = 0;
    DWORD a = GetFileAttributesA(dir);
    if (a == INVALID_FILE_ATTRIBUTES || !(a & FILE_ATTRIBUTE_DIRECTORY)) {
        snprintf(g_ai_dir_why, sizeof g_ai_dir_why, "[%s] ai_dir %s: no such folder", from, dir);
        return;
    }
    lstrcatA(dir, "\\");
    lstrcpyA(g_ai_dir, dir);
}

// THE TEXT LANGUAGE'S HOST (script/host.h): what it asks the game - its values and relations (diplo.c), constants
// (game_script/constants.c), the players as objects, the two generators and the function files (persona.c), the code's
// placeholders (texts.c), the commands a script may call (scripts.c). Set before any text or setting is read.
internal void x_host_install(void) {
    g_x_host.value = x_builtin;
    g_x_host.is_value = x_is_value;
    g_x_host.name_valid = x_name_valid;
    g_x_host.method = x_method;
    g_x_host.object = x_player_n;
    g_x_host.constant = ee_constant;
    g_x_host.rnd_show = rnd;
    g_x_host.rnd_decide = rnd_game;
    g_x_host.scan_functions = x_fn_scan_dir;
    g_x_host.placeholders = x_code_placeholders;
    g_x_host.command = x_command_verb;
    g_x_host.code_record = x_code_record;
    g_x_host.kind = x_kind_of;
    g_x_host.root_kind = x_root_kind;
    g_x_host.fn_kind = x_fn_kind;
    g_x_host.member = x_obj_member_of;
    g_x_host.set = x_obj_set;
    g_x_host.const_type = x_const_type;
    g_x_host.name_type = x_name_type;
    g_ini_eval = x_const_eval;
}

internal void install(HMODULE self) {
    x_host_install();
    // The engine may FreeLibrary() DLLs that aren't render plugins; our hooks must outlive that.
    HMODULE pinned;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
                       (LPCSTR)&install, &pinned);

    char path[MAX_PATH], dir[MAX_PATH];
    GetModuleFileNameA(NULL, path, MAX_PATH);   // the game folder: the exe's (a test run's own DLL is elsewhere)
    char *slash = strrchr(path, '\\');
    if (slash) {
        slash[1] = 0;
        lstrcpyA(g_game_dir, path);
        wsprintfA(g_ai_dir, "%saimod_ai\\", path);
        lstrcpyA(dir, path);
        slot_dir(dir);   // the test ini, the log and the dumps: the game folder, a test slot's own folder
        wsprintfA(g_dump_dir, "%saimod_dumps", dir);
        wsprintfA(path, "%saimod_autotest.ini", dir);
        load_autotest(path);
        lstrcpyA(g_ini, path);
        wsprintfA(g_aimod_ini, "%saimod.ini", g_game_dir);
        config_paths();
        // required: missing -> error and quit; [general] enabled=0 -> no hooks
        if (GetFileAttributesA(g_aimod_ini) == INVALID_FILE_ATTRIBUTES) config_missing(g_aimod_ini);
        settings_read(g_aimod_ini, g_autotest ? g_ini : NULL);
        config_ai_dir();
        wsprintfA(g_log_path, "%saimod_econ.csv", dir);
        if (!g_set.enabled) {
            FILE *f = fopen(g_log_path, "a");
            if (f) { fputs("# eeaimod: switched off in aimod.ini (enabled=0) - nothing installed\n", f); fclose(f); }
            return;
        }
        g_fix_scenario_ai = g_set.scenario_ai;
        g_mem_dumps = g_autotest || g_set.memory_dumps;
        if (g_mem_dumps || g_autotest || g_set.control_pipe)
            CreateDirectoryA(g_dump_dir, NULL);   // memory dumps, shot / ddinfo pictures
    }
    g_session = GetTickCount();
    InitializeCriticalSection(&g_lock);
    log_rotate();
    g_log = fopen(g_log_path, "a");
    if (g_log) {
        SYSTEMTIME st;
        GetLocalTime(&st);
        fprintf(g_log, "# session %04d-%02d-%02d %02d:%02d:%02d\n",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        log_header();
        ini_flush();   // a setting's mistakes found before the log was open
        settings_log(g_log);
        api_log(g_log);
        fprintf(g_log, "# config: %s (personalities: %s%s%s)\n", g_aimod_ini, g_ai_dir, g_ai_dir_why[0] ? " - " : "", g_ai_dir_why);
        char me[MAX_PATH];
        if (GetEnvironmentVariableA("EEAIMOD_DLL", me, sizeof me) && GetModuleFileNameA(self, me, sizeof me))
            fprintf(g_log, "# dll: %s (the test run's own, built %s %s)\n", me, __DATE__, __TIME__);
        if (g_slot) fprintf(g_log, "# slot: %d (pipe eeaimod%d, ports %d/%d, files in %s)\n", g_slot, g_slot,
                            33334 + 2 * g_slot, 33335 + 2 * g_slot, dir);
        // the game's settings: eesettings.ini through the optional dinput.dll (eesettings.c), else the registry
        char gs[MAX_PATH + 64];
        if (!GetEnvironmentVariableA("EEAIMOD_SETTINGS_FILE", gs, sizeof gs)) lstrcpyA(gs, "registry (no dinput.dll of the mod)");
        fprintf(g_log, "# game settings: %s\n", gs);
    }

    BOOL a = patch_rm_compute_targets(hook_compute_targets);
    BOOL b = patch_rm_assign_citizens(hook_assign_citizens);
    BOOL c = patch_rm_update(hook_rm_update);
    BOOL f = patch_cp_update(hook_cp_update);
    BOOL gs = patch_game_start(hook_game_start);   // also personalities: always
    if (g_log) {
        fprintf(g_log, "# eeaimod loaded: hook targets=%s assign=%s update=%s\n",
                a ? "ok" : "FAILED", b ? "ok" : "FAILED", c ? "ok" : "FAILED");
        fprintf(g_log, "# fix scenario_ai: %s (hook %s)\n", g_fix_scenario_ai ? "on" : "off", f ? "ok" : "FAILED");
        BOOL d = patch_parse_cmdline(hook_parse_cmdline);
        BOOL e = patch_load_saved(hook_load_saved);
        fprintf(g_log, "# game start hook: %s\n", gs ? "ok" : "FAILED");
        fprintf(g_log, "# cmdline hooks: parse=%s load=%s\n", d ? "ok" : "FAILED", e ? "ok" : "FAILED");
        if (g_autotest)
            fprintf(g_log, "# autotest: skip_intro=%s quit_minutes=%g cmdline=%s\n",
                    g_auto.skip_intro ? (skip_intro_movies() ? "ok" : "FAILED") : "off",
                    g_auto.quit_minutes, GetCommandLineA());
        if (g_vanilla_mask) {
            fprintf(g_log, "# vanilla: players");
            for (int i = 1; i < 16; i++) if (is_vanilla_idx(i)) fprintf(g_log, " %d", i);
            fprintf(g_log, " play the game's own AI (difficulty %d, cheat %d; -1: as the others)\n", g_rm.v_difficulty, g_rm.v_cheat);
        }
        fflush(g_log);
    }
    // normal play: aimod.ini [general] skip_intro = 1 skips the intro movies (autotest has its own key)
    if (!g_autotest && g_set.skip_intro) {
        BOOL ok = skip_intro_movies();
        if (g_log) fprintf(g_log, "# skip_intro: %s\n", ok ? "ok" : "FAILED");
    }
    econ_install(g_autotest ? g_ini : NULL);
    food_install(g_autotest ? g_ini : NULL);
    mines_install(g_autotest ? g_ini : NULL);
    cz_install(g_autotest ? g_ini : NULL);
    ap_install(g_autotest ? g_ini : NULL);
    mt_install(g_autotest ? g_ini : NULL);
    mass_install(g_autotest ? g_ini : NULL);
    cover_install(g_autotest ? g_ini : NULL);
    expand_install(g_autotest ? g_ini : NULL);
    tw_install(g_autotest ? g_ini : NULL);
    attack_install();
    landing_install();
    navy_install(g_autotest ? g_ini : NULL);
    intel_install();
    defence_install();
    chat_install();
    persona_install(g_autotest ? g_ini : NULL);
    diplo_install(g_autotest ? g_ini : NULL);
    persist_install();
    aim_install(g_autotest ? g_ini : NULL);
    camera_install();
    display_install(g_autotest ? g_ini : NULL);
    sound_install();
    select_install();
    turns_install();   // before ship_turns: it finds the models by their slot 7, which ship_turns replaces
    collide_install();
    groupmove_install();
    shiproute_install(g_autotest ? g_ini : NULL);
    drawlist_install();
    sight_install();
    asg_queue_install();
    llm_install();
    ctl_start(g_autotest || g_set.control_pipe);
    if (g_autotest && g_auto.game_seed) {
        BOOL ok = install_srand(), start = start_hook();
        if (g_log) fprintf(g_log, "# autotest: game_seed=%d (srand hook %s, start message hook %s)\n", g_auto.game_seed,
                           ok ? "ok" : "FAILED", start ? "ok" : "FAILED");
    }
    if (g_autotest) { watch_start(g_ini); trace_start(g_ini); sync_trace_start(g_ini); }
    AddVectoredExceptionHandler(1, crash_handler);   // logs the first exceptions (module+offset, backtrace) - also in normal play
}


// A test run's own DLL (run_test.ps1 -Dll; docs/testing.md): EEAIMOD_DLL names a copy outside the game folder (the
// engine loads every DLL in there, so it can't sit beside this one). The game folder's eeaimod.dll then loads that
// file and installs nothing itself; the copy pins itself in install() and takes the game folder from the exe. A copy
// that doesn't load: nothing installed, one log line (the runner stops a game that doesn't report autotest mode) -
// never this DLL in its place, which would test the wrong build. Normal play never sets the variable.
internal int run_other_dll(HINSTANCE self) {
    char want[MAX_PATH], full[MAX_PATH], me[MAX_PATH];
    if (!GetEnvironmentVariableA("EEAIMOD_DLL", want, sizeof want) || !want[0]) return 0;
    if (!GetFullPathNameA(want, sizeof full, full, NULL)) lstrcpyA(full, want);
    if (GetModuleFileNameA(self, me, sizeof me) && !lstrcmpiA(me, full)) return 0;   // this is the copy
    // LoadLibrary inside DllMain: the loader lock is the same thread's (re-entrant); the copy's own DllMain runs now
    if (LoadLibraryA(full)) return 1;
    DWORD err = GetLastError();
    char dir[MAX_PATH], path[MAX_PATH + 32];
    if (GetEnvironmentVariableA("EEAIMOD_DLL_TOLD", dir, sizeof dir)) return 1;   // logged once (loaded several times)
    SetEnvironmentVariableA("EEAIMOD_DLL_TOLD", "1");
    GetModuleFileNameA(NULL, dir, MAX_PATH);
    char *slash = strrchr(dir, '\\');
    if (slash) slash[1] = 0; else dir[0] = 0;
    slot_dir(dir);
    wsprintfA(path, "%saimod_econ.csv", dir);
    FILE *f = fopen(path, "a");
    if (f) { fprintf(f, "# eeaimod: EEAIMOD_DLL %s didn't load (error %lu) - nothing installed\n", full, err); fclose(f); }
    return 1;
}

BOOL WINAPI DllMain(HINSTANCE inst, DWORD reason, LPVOID reserved) {
    (void)reserved;
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(inst);
        if (run_other_dll(inst)) return TRUE;
        uint32_t hash;
        int build = game_build(&hash);
        if (build > 0) install(inst);
        else if (build < 0) build_unknown(hash);
    }
    return TRUE;
}
