// THE RELOAD (`reload` = game | ee | aoc): the 2017 update waits a unit's whole reload between blows; Art of Conquest
// waits reload x a random factor between its world table's +0x984 and +0x988 - 0.8 and 1.2 in its dbworld.dat - drawn
// from the game's generator (unit.reload_wait at its four places: the attack ticks', EEUGAttackGround and
// EEUGAttackUnit, and the fire's two). game: each build its own. ee in AoC: the four calls set the whole reload.
// aoc in the 2017 update: its four inline stores (hooks.reload_tick_cmp / reload_fire_store) go to reload_aoc, the
// same draw by the same generator (physics.random_range), AoC's 0.8 and 1.2 - the 2017 update's world table hasn't
// those rows. The draws are the game's own: every PC of a multiplayer game must have the same setting.
// (AoC's random wait seemed to cut swings off - hunters never landing a blow, a howitzer firing every half second -
// but that was AoC reading the 2017 update's shorter world table past its end: aoc_data_order, data_order.c.)
#define RL_GAME 0
#define RL_EE 1
#define RL_AOC 2
internal int g_rl_mode;
#define RL_AOC_LO 0.8f   // AoC's dbworld.dat, rows +0x984 / +0x988
#define RL_AOC_HI 1.2f

internal void __thiscall hook_reload_wait(ee_unit *u, int reload) { unit_set_reload(u, reload); }   // AoC: ee

__attribute__((used)) void __stdcall reload_aoc(ee_unit *u, int reload) {   // the 2017 update: aoc
    ee_physics *ph = game_physics();
    unit_set_reload(u, ph ? (int)(physics_random_range(ph, RL_AOC_LO, RL_AOC_HI) * (float)reload) : reload);
}
// at the four stores: the unit in esi, the reload in eax; every register kept, ZF set (the ticks' je skips their store)
__attribute__((naked, used)) void reload_aoc_thunk(void) {
    __asm__ volatile (
        "pushal\n\t"
        "pushl %eax\n\t"
        "pushl %esi\n\t"
        "call _reload_aoc@8\n\t"
        "popal\n\t"
        "cmpl %esp, %esp\n\t"
        "ret\n\t");
}

internal void reload_install(void) {
    g_rl_mode = g_set.reload;
    const char *mode = g_rl_mode == RL_EE ? "ee" : g_rl_mode == RL_AOC ? "aoc" : "game";
    int aoc = build_is("aoc"), ok = 0, of = 0;
    if (g_rl_mode == RL_EE && aoc) ok = patch_reload_wait(hook_reload_wait, &of);
    else if (g_rl_mode == RL_AOC && !aoc) ok = patch_reload_stores(reload_aoc_thunk);
    else { if (g_log) fprintf(g_log, "# reload: %s (the game's own)\n", mode); return; }
    if (g_log) fprintf(g_log, "# reload: %s (%s %d/%d)\n", mode, aoc ? "AoC's random wait's calls" : "the 2017 update's stores", ok, 4);
}
