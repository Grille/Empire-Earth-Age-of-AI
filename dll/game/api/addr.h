// THE GAME'S ADDRESSES: every address the mod uses in Empire Earth.exe, by name, with its value in each exe build the
// mod knows - one column per k_exe_builds entry, in that order (0: the build hasn't got it). game_build (game/exe.c)
// picks the column when the DLL loads (addr_select): GA(NAME) is the address, GF(NAME) the function there, typed by
// its row, GL(NAME) a list of addresses (its count: sizeof GL(NAME) / sizeof *GL(NAME)). Another build - Art of
// Conquest, another release, a patched exe - is one more hash and one more value per row. The names: FN_ a function,
// CALL_ a call (patched), SITE_/CODE_ an instruction or operand, G_ data, VT_ a vtable, VTS_ a vtable slot's address,
// SLOT_ a vtable slot's index (a class may have gained a virtual function), SIZE_ an object's size, IAT_ an import
// slot, RVA_ a place in an engine DLL. Tables of their own for one build: editor_layout.h (generated for the 2017
// update: apply_editor_layout only there). The objects' fields: fields.h, the same columns.
// Art of Conquest's column: scripts/port_addrs.py (the two exes' code paired; its HAND table the few found by hand,
// each with its reason). Not ready: nothing ran there yet - the mod stays off there.
#define ADDR_BUILDS 2
internal const struct exe_build { uint32_t hash; const char *name; int ready; } k_exe_builds[ADDR_BUILDS] = {
    { 0x4FB6A479, "ee", 1 },    // the 2017 update (GOG, Steam, empireearth.eu Vanilla)
    { 0x0644BA54, "aoc", 0 },   // The Art of Conquest, EE-AOC.exe (GOG, Steam)
};

#include "game/api/functions_addr.h"   // the rows of the functions the mod calls
#include "game/api/hooks_addr.h"       // the rows of what it patches to hook the game

// The rows by who uses them: functions_addr.h what functions.h calls, hooks_addr.h what hooks.h patches, here the rest -
// data, imports, and what the code not moved over to functions.h / hooks.h yet uses (engine/, test/, voice/ ...: its
// rows go over with it). One table: the three composed below.
// F(name, return type, calling convention, (parameters), value per build): a function; D(name, value per build)
#define ADDRS_OTHER(F, D) \
    D(RET_ASG_LAND_HANDOFF_GIVE, 0x0044AF88, 0x0044A6A3) \
    D(LEFTOVER_SITE_END, 0x0048A2B9, 0x0048EC34)                              /* call sites below this are in the leftover pass 0x48A1AE */ \
    D(G_FLOAT_1_600, 0x0082CBCC, 0x00842374)                                  /* the float 1/600 (the minimap's y scale operand) */ \
    D(VT_GOAL_BUILD, 0x008319F0, 0x0084740C) \
    /* EEUGLoadTransport: the holder's goal and each unit's part (0x614833), the holder at +0x50 */ \
    D(VT_GOAL_LOAD, 0x00831DC0, 0x008477F8) \
    D(VT_GOAL_UNLOAD, 0x00831F5C, 0x008479A0)                                 /* EEUGUnloadTransport (new 0x40, ctor 0x615A1F(transport, x, y, flag)) */ \
    D(G_FLOAT_1_800, 0x008358A8, 0x0084B940)                                  /* the float 1/800 (the minimap's x scale operand) */ \
    /* the scenario editor's file buttons: 6 of {id, x, y, u0, v0, u1, v1, tooltip} */ \
    D(G_EDITOR_FILE_BUTTONS, 0x008AE0A0, 0x008C47B0) \
    D(G_COLORS_BEGIN, 0x008CFA74, 0x008E610C)                                 /* the player colour table (vector begin, end) */ \
    D(G_COLORS_END, 0x008CFA78, 0x008E6110) \
    D(G_HIT_TABLE_A, 0x008D2008, 0x008E870C)                                  /* the hit table's variants by game mode */ \
    D(G_HIT_TABLE, 0x008D200C, 0x008E8708)                                    /* [target category][weapon kind]: 0 = it cannot hit */ \
    D(G_HIT_TABLE_B, 0x008D2010, 0x008E8710) \
    D(G_BEHAVIOUR_TABLE, 0x008D2044, 0x008E8744)                              /* the unit behaviours (stances) */ \
    D(G_DAMAGE_TABLE, 0x008D2050, 0x008E8750)                                 /* [attacker class + target class x 6] */ \
    D(G_SELECT_RECT, 0x008D2778, 0x008E8F18)                                  /* a default selection rectangle */ \
    D(G_DRAW_N, 0x00912538, 0x00928CD8)                                       /* draw list counts: objects, small objects */ \
    D(G_DRAW_SMALL_N, 0x0091253C, 0x00928CDC) \
    D(G_TABLE_VALUES, 0x00917BB0, 0x0092FCA0)                                 /* FN_TABLE_VALUE's table (0x16: the population limit) */ \
    D(G_GAME_DATA, 0x00917BB8, 0x0092FCA8)                                    /* game data loaded (unit limit +0x2EC, chat time +0x2CC) */ \
    D(G_MAP_AREAS, 0x00917BC8, 0x00930BF0)                                    /* the map's areas (regions, continents) */ \
    D(G_BONUS_TABLE, 0x00917DE0, 0x009318E8)                                  /* FN_ATTACK_BONUS's object */ \
    D(G_GAME_STATE, 0x00918300, 0x00930D40)                                   /* object; vtable slot 11 called after a speed change */ \
    D(G_PLAYERS, 0x00918374, 0x00930DB4) \
    D(G_MAP, 0x009183BC, 0x00930DFC)                                          /* ptr; +0x1816FC = map size in tiles */ \
    D(G_GAME_SPEED, 0x009183C0, 0x00930E00)                                   /* float; set like EEMGameSpeed::Apply (0x4B6BD4) */ \
    D(G_PLAYERS_N, 0x009183CC, 0x00930E0C)                                    /* the number of players */ \
    D(G_POP_LIMIT, 0x009183D4, 0x00930E14)                                    /* the population limit game setting */ \
    D(G_END_EPOCH, 0x009187F4, 0x00931234)                                    /* the game's end epoch */ \
    D(G_GAME_MODE, 0x00918808, 0x00931248) \
    D(G_SCENARIO_FLAG, 0x00918819, 0x00931259)                                /* byte: game is a scenario/campaign (settings 0x918410 + 0x409) */ \
    D(G_GAME_TYPE, 0x00918BF4, 0x00931634)                                    /* 0 random map, 1 scenario, 2 saved game, 3/4 other */ \
    D(G_DIFFICULTY, 0x00918C14, 0x00931654)                                   /* the AIs' difficulty (the setup's) */ \
    D(G_CAMERA, 0x00918CD0, 0x00931710)                                       /* EECameraControl; +0x78/+0x7C/+0x80 look-at x/y/z (tiles), +0xE0 zoom */ \
    D(G_SCROLL_SPEED, 0x00918DB0, 0x009317F0)                                 /* float */ \
    D(G_SM_MGR, 0x00918E24, 0x00931864)                                       /* the scenario manager */ \
    D(G_LOCAL_PLAYER, 0x00918E84, 0x009318C4)                                 /* int: index of the player on this PC (the human in single player) */ \
    D(G_CHEATS_ON, 0x00918E9A, 0x009318DA) \
    D(G_TEAMS_LOCKED, 0x00918E9F, 0x009318DF) \
    D(G_PAUSED, 0x00918EA2, 0x009318E2)                                       /* byte, toggled by EEAUserPauseGame::Execute (0x4064F1) */ \
    D(G_SETTINGS, 0x00918EA8, 0x00930308)                                     /* the game settings object */ \
    D(G_PLAYER_NAME, 0x00919308, 0x00930768)                                  /* the player name (UWideString) */ \
    D(G_SCROLL_SETTING, 0x009193A8, 0x00930828)                               /* float, settings slider x 0.01 */ \
    D(G_SOUND_PERCENT, 0x009193B8, 0x00930838) \
    D(G_GAME_RES_H, 0x009193F8, 0x00930878) \
    D(G_GAME_RES_W, 0x009193FC, 0x0093087C) \
    D(G_SCREEN, 0x00919428, 0x009308A8)                                       /* the current screen index */ \
    D(G_FULLSCREEN, 0x0091945A, 0x009308DB) \
    D(G_RESOURCES, 0x00919770, 0x009318EC)                                    /* the string resources */ \
    D(G_WORLD_CLOCK, 0x00919774, 0x009318F0)                                  /* ptr to EEPhysics; +0xD0 = game time in ms */ \
    D(G_WORLD, 0x0091977C, 0x009318F8)                                        /* action queue owner: QueueAction 0x66192F (thiscall, takes action ptr) */ \
    D(G_RASTERIZER, 0x00919794, 0x00931910) \
    D(G_UI_MANAGER, 0x009197A0, 0x0093191C) \
    D(G_GAME_WINDOW, 0x009197A4, 0x00931920)                                  /* GEWindow* (+0 HWND) */ \
    D(G_MOUSE, 0x009197A8, 0x00931924)                                        /* RTSMouse */ \
    D(G_SOUND_MANAGER, 0x00919848, 0x009319C4) \
    /* EEClient/EEServer (lockstep server, slot 6 takes a message); UI checks [+0x10] != 0 and vtable+0x10 before */ \
    /* pausing */ \
    D(G_SERVER, 0x0091984C, 0x009319C8) \
    D(G_CHEAT_PARSER, 0x00919858, 0x009319D4) \
    D(G_INPUT_MSGS_DONE, 0x00919B68, 0x00931CF0)                              /* the user-input messages processed so far */ \
    D(G_EFFECTS, 0x00919E70, 0x00931FF8)                                      /* tech effects, 0x44 each */ \
    D(G_EPOCH_TECHS, 0x0093B1B8, 0x0095CF80)                                  /* per epoch 0x10: the tech lists */ \
    D(G_TYPES, 0x009418A0, 0x009636AC) \

#define ADDRS(F, D) ADDRS_FUNCTIONS(F, D) ADDRS_HOOKS(F, D) ADDRS_OTHER(F, D)

// numbers that aren't addresses: SLOT_ a virtual function's index in its class's vtable (patch_vtable, a call through
// the vtable; slot 0, the deleting destructor in both builds, stays a plain 0), SIZE_ the size of a game object the
// mod makes itself (FN_NEW, then its constructor), RVA_ a place in an engine DLL relative to its base - each build
// ships its own DLLs (scripts/port_dll.py); what the DLLs export is taken by name instead. A patch there checks the
// bytes first: a DLL from elsewhere is left alone.
#define ADDR_NUMBERS_OTHER(D) \
    D(SIZE_PAUSE_ACTION, 0xC, 0xC)      /* EEAUserPauseGame: vtable, refcount, byte */ \
    D(SIZE_CHEAT_MSG, 0x18, 0x18)       /* FN_CHEAT_MSG_CTOR */ \
    D(SIZE_CAMERA_ACTION, 0x1C, 0x1C)   /* FN_CAMACT_CTOR */ \
    D(SIZE_NEW_UNIT_MSG, 0x20, 0x20)    /* FN_NEW_UNIT_MSG_CTOR */ \
    D(SIZE_MSG_CHAT, 0x24, 0x24)        /* FN_CHAT_MSG_CTOR */ \
    D(SIZE_SAVE_ACTION, 0x30, 0x30)     /* FN_SAVE_ACTION_CTOR */ \
    D(SIZE_GOAL_MOVE, 0x38, 0x38)       /* FN_GOAL_MOVE_CTOR */ \
    D(SIZE_GOAL_ATTACK, 0x40, 0x44)     /* FN_GOAL_ATTACK_CTOR */ \
    D(SIZE_GOAL_UNLOAD, 0x40, 0x40)     /* FN_GOAL_UNLOAD_CTOR */ \
    D(SIZE_GOAL_BUILD, 0x5C, 0x5C)      /* FN_BUILD_GOAL */ \
    D(SIZE_GOAL_LOAD, 0x68, 0x68)       /* FN_GOAL_LOAD_CTOR */ \

#define ADDR_NUMBERS(D) ADDR_NUMBERS_FUNCTIONS(D) ADDR_NUMBERS_HOOKS(D) ADDR_NUMBERS_OTHER(D)

// lists: L(name, count, (values of build 1), (values of build 2) ...)
#define ADDR_LISTS_OTHER(L) \

#define ADDR_LISTS(L) ADDR_LISTS_FUNCTIONS(L) ADDR_LISTS_HOOKS(L) ADDR_LISTS_OTHER(L)

#define ADDR_ENUM_F(n, r, cc, a, ...) A_##n,
#define ADDR_ENUM_D(n, ...) A_##n,
enum addr_name { ADDRS(ADDR_ENUM_F, ADDR_ENUM_D) ADDR_NUMBERS(ADDR_ENUM_D) A_N };
#define ADDR_TYPE_F(n, r, cc, a, ...) typedef r (cc *n##_fn) a;
#define ADDR_TYPE_D(n, ...)
ADDRS(ADDR_TYPE_F, ADDR_TYPE_D)
#define ADDR_ROW_F(n, r, cc, a, ...) { __VA_ARGS__ },
#define ADDR_ROW_D(n, ...) { __VA_ARGS__ },
internal const uint32_t k_addr[A_N][ADDR_BUILDS] = { ADDRS(ADDR_ROW_F, ADDR_ROW_D) ADDR_NUMBERS(ADDR_ROW_D) };
#define ADDR_NAME_F(n, r, cc, a, ...) #n,
#define ADDR_NAME_D(n, ...) #n,
internal const char *const k_addr_name[A_N] = { ADDRS(ADDR_NAME_F, ADDR_NAME_D) ADDR_NUMBERS(ADDR_NAME_D) };
#define ADDR_UNPAREN(...) { __VA_ARGS__ }
#define ADDR_LIST_COL(v) ADDR_UNPAREN v,
#define ADDR_LIST(n, k, ...) static const uint32_t k_addr_list_##n[ADDR_BUILDS][k] = { ADDR_LIST_EACH(__VA_ARGS__) };
#define ADDR_LIST_EACH(v1, v2) ADDR_LIST_COL(v1) ADDR_LIST_COL(v2)   // one per build
ADDR_LISTS(ADDR_LIST)

internal int g_exe_build = -1;           // the column of the running exe (-1: none known - the mod stays off)
// the running build by its name ("ee", "aoc"): for code that differs by more than an address (AoC's rewrites)
internal int build_is(const char *name) { return g_exe_build >= 0 && !lstrcmpA(k_exe_builds[g_exe_build].name, name); }
internal uintptr_t g_addr[A_N];
#define GA(n) (g_addr[A_##n])
#define GF(n) ((n##_fn)g_addr[A_##n])
#define GL(n) (k_addr_list_##n[g_exe_build])
internal int g_addr_missing;               // names the running build hasn't got (logged at the start: addr_log)

// the running exe's column into g_addr; the names it hasn't got (0) are counted - the caller logs them
internal int addr_select(int build) {
    int missing = 0;
    g_exe_build = build;
    for (int i = 0; i < A_N; i++) missing += !(g_addr[i] = k_addr[i][build]);
    image_ranges();
    return missing;
}
internal void addr_log(FILE *log) {   // the names the running build hasn't got: what uses them doesn't work there
    if (!log || !g_addr_missing) return;
    fprintf(log, "# addresses: build %d hasn't got %d of %d:", g_exe_build, g_addr_missing, A_N);
    for (int i = 0; i < A_N; i++) if (!g_addr[i]) fprintf(log, " %s", k_addr_name[i]);
    fputc(10, log);
}
