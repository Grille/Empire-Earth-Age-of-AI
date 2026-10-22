// THE EXE BUILDS THE MOD KNOWS (exe.c tells which one runs): their columns in raw/tables.c's tables, in this order.
// Art of Conquest's column: scripts/port_addrs.py. Not ready: the player's game runs without the mod, a test run plays it (game_build).
#define ADDR_BUILDS 2
internal const struct exe_build { uint32_t hash; const char *name; int ready; } k_exe_builds[ADDR_BUILDS] = {
    { 0x4FB6A479, "ee", 1 },    // the 2017 update (GOG, Steam, empireearth.eu Vanilla)
    { 0x0644BA54, "aoc", 0 },   // The Art of Conquest, EE-AOC.exe (GOG, Steam)
};
internal int g_exe_build = -1;   // the running build's column (-1: none known - the mod stays off)
// the running build by its name ("ee", "aoc"): for code that differs by more than an address (AoC's rewrites)
internal int build_is(const char *name) { return g_exe_build >= 0 && !lstrcmpA(k_exe_builds[g_exe_build].name, name); }
