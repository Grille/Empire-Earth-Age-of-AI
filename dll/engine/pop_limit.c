// THE POPULATION LIMIT'S MOST ([general] population_limit_max; the player 2026-11-19: "add a pop limit extender" - his
// own 13208 came from a loose Data\DB\dbworld.dat he had put there, which players don't have). The game data's world
// table, record 26: +0x10 the setup's default (50) and +0x14 its most (1200 in both builds' archives; game_data
// unit_limit, unit_limit_max). The setup screen's list (0x5CE77D) offers every 50 from the default to the most, and the
// game start's setup copy (0x524B8F) takes the default for a limit above the most - so the most raised in the loaded
// table is all it takes, and a game data with more keeps its own. Set from the frame hook (display.c hook_show_scene,
// fullscreen and windowed alike, from the intro on - the data is loaded before the first frame and every setup screen
// comes later): one comparison a frame. In multiplayer a PC with a lower most starts the game at the default.
//
// THE STEPS (the player, the same day: "the 50 steps work great for the games own limit, here they just become noise -
// maybe 50 until 200, then 100 steps until 1500, then 500 steps"): the list's five places of the step (hooks.c
// patch_limit_steps) call these instead - the list from the default by them, the most its last value when it isn't on
// a step (a data file's 13208), a click's position and the setting's position by the same walk. With the most at the
// game's own (population_limit_max = 0) the game's own steps stay. The multiplayer lobby's list is its own (50 to 1200,
// fixed in its code: EELobbyGameOptionsCtrl 0x67AE6B) and stays the game's.
internal int g_pl_told;
internal void pop_limit_frame(void) {
    int want = g_set.population_limit_max;
    ee_game_data *gd = want > 0 ? game_data() : NULL;
    if (!gd || game_data_get_unit_limit_max(gd) >= want) return;
    int was = game_data_get_unit_limit_max(gd);
    game_data_set_unit_limit_max(gd, want);
    if (g_log && !g_pl_told++) log_locked("# population limit: the setup's most %d, the game data's %d\n", want, was);
}

internal int pl_first(void) { ee_game_data *gd = game_data(); return gd ? game_data_get_unit_limit(gd) : 50; }
internal int pl_most(void) { ee_game_data *gd = game_data(); return gd ? game_data_get_unit_limit_max(gd) : 1200; }
// the list's value after v: 50s below 200, 100s below 1500, then 500s; the most last; 0 past it
internal int __attribute__((fastcall)) pl_next(int v) {
    int n = v + (v < 200 ? 50 : v < 1500 ? 100 : 500), most = pl_most();
    return n <= most ? n : v < most ? most : 0;
}
// v's position: the last value of the list at or below it (a setting off the steps - from before, or a saved game's -
// shows at the one below)
internal int __attribute__((fastcall)) pl_index(int v) {
    int i = 0;
    for (int x = pl_first(), n; (n = pl_next(x)) && n <= v && i < 100000; x = n) i++;
    return i;
}
internal int __attribute__((fastcall)) pl_value(int pos) {
    int x = pl_first();
    for (int i = 0, n; i < pos && (n = pl_next(x)); i++) x = n;
    return x;
}
internal void pop_limit_install(void) {
    if (g_set.population_limit_max <= 0) return;
    int ok = patch_limit_steps(pl_next, pl_index, pl_value);
    if (g_log) fprintf(g_log, "# population limit: the setup's steps 50 to 200, 100 to 1500, then 500 - %s\n", ok ? "hooked 5/5" : "the code differs: the game's 50s");
}
