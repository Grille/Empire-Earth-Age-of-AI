// DAY AND NIGHT IN RANDOM-MAP GAMES ([display] skirmish_day_night, skirmish_start_hour; the player 2026-11-19: a
// setting over a line in setup.eeai, the easier for players). The game has a day/night cycle of its own
// (EEPhysics.day_cycle: each step moves the time of day on, a whole day in 600 s of game time) that only a scenario's
// trigger effect Day/Night Cycle Time switches on; the setting switches it on as a random map starts, and the start
// hour sets the time of day there as the effect Daylight Hour does. Both at the game's first step (its call of the
// cycle's step): set as the start returned, the hour was the game's own again by the first step (14.4 o'clock).
// A scenario or campaign keeps its author's light, a saved game its own: the save holds the switch and the hour (a
// save with the cycle on loaded on at its hour, tried 2026-11-19). The pipe's daynight switches both in any game.
internal volatile LONG g_dn_pending;   // a random map started: its first step sets the light

internal void day_night_new_game(int mode) {
    g_dn_pending = mode == 0 && (g_set.skirmish_day_night || g_set.skirmish_start_hour >= 0);   // 0 a random map (a test's too)
}
internal void __thiscall hook_day_cycle_step(ee_physics *ph) {
    if (g_dn_pending && InterlockedExchange(&g_dn_pending, 0)) {
        if (g_set.skirmish_start_hour >= 0) game_set_hour((float)g_set.skirmish_start_hour);
        if (g_set.skirmish_day_night) physics_set_day_cycle(ph, 1);
        if (g_log) log_line("# day/night: %s, %.2f o'clock at the first step\n", physics_get_day_cycle(ph) ? "the cycle on" : "no cycle",
                            (double)game_hour());
    }
    physics_day_cycle_step(ph);
}
internal void day_night_install(void) {
    int ok = patch_day_cycle_step(hook_day_cycle_step);
    if (!ok && g_log) fprintf(g_log, "# day/night: the step's call NOT hooked - skirmish_day_night and skirmish_start_hour do nothing\n");
}
