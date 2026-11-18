// THE POPULATION LIMIT'S MOST ([general] population_limit_max; the player 2026-11-19: "add a pop limit extender" - his
// own 13208 came from a loose Data\DB\dbworld.dat he had put there, which players don't have). The game data's world
// table, record 26: +0x10 the setup's default (50) and +0x14 its most (1200 in both builds' archives; game_data
// unit_limit, unit_limit_max). The setup screen's list (0x5CE77D) offers every 50 from the default to the most, and the
// game start's setup copy (0x524B8F) takes the default for a limit above the most - so the most raised in the loaded
// table is all it takes, and a game data with more keeps its own. Set from the frame hook (display.c hook_show_scene,
// fullscreen and windowed alike, from the intro on - the data is loaded before the first frame and every setup screen
// comes later): one comparison a frame. In multiplayer a PC with a lower most starts the game at the default.
internal int g_pl_told;
internal void pop_limit_frame(void) {
    int want = g_set.population_limit_max;
    ee_game_data *gd = want > 0 ? game_data() : NULL;
    if (!gd || game_data_get_unit_limit_max(gd) >= want) return;
    int was = game_data_get_unit_limit_max(gd);
    game_data_set_unit_limit_max(gd, want);
    if (g_log && !g_pl_told++) log_locked("# population limit: the setup's most %d, the game data's %d\n", want, was);
}
