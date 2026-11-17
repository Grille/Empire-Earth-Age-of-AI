// THE MAP'S HERDS ([patches] map_herds; the player 2026-11-18: "just like the ai, animal ai (walking, breeding) seems
// disabled in scenarios. i skirmish it works" - and in the scenario editor's test play only after a skirmish was
// played: "this never gets properly set/cleared for testplay"). A herd (EEHerd, the world player's herds) is the
// animals' own AI: it walks its animals about and breeds them back up to its size - kill one and a young one comes of
// one of the others ("at some point thy stop breading, if i than kill one animal an small one imediatly spawn"). The
// game gives the map's animals their herds in a pass over the world's units at a game's start (EEPlayer herds_start:
// each animal or fish into a herd of its kind near it, else a new one); an animal made during play is put into one as
// it appears. Started as a scenario, from the editor's test play, and in the mod's test games (a random map after a
// save is loaded), the map's animals were in none: they stood where the map put them all game and never bred - a
// Continental test game, 60 of its 61 animals without a herd, the one herd an eagle's born during play.
// Once a game, at the first safe point from HD_AT on: the game's own pass again. It skips the animals already in a
// herd, so in a game that had its herds it changes nothing. A loaded save the same (its animals as it kept them).
// In multiplayer every player needs the same setting: the herds are the simulation's.
#define HD_AT 2000
internal int g_herds_on = 1;
internal uint32_t g_hd_t;   // the game time it ran at (0: not yet in this game)
internal void herds_count(ee_player *w, int *animals, int *without, int *herds) {
    *animals = *without = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        int fam = unit_type_get_family(it.td);
        if ((fam != KIND_ANIMAL && fam != KIND_FISH) || unit_get_hp(it.u) <= 0 || unit_get_x(it.u) < 0) continue;
        (*animals)++;
        *without += !unit_get_herd(it.u);
    }
    ee_herd **b = player_get_herds_begin(w), **e = player_get_herds_end(w);
    *herds = b && e > b ? (int)(e - b) : 0;
}
// the safe point (control.c's hook_sim_receive)
internal void herds_tick(void) {
    uint32_t now = game_time();
    if (g_hd_t && now < g_hd_t) g_hd_t = 0;   // a new game
    if (!g_herds_on || g_hd_t || now < HD_AT) return;
    g_hd_t = now ? now : 1;
    ee_player *w = game_player_count() ? game_players()[0] : NULL;
    if (!w) return;
    int a0, n0, h0, a1, n1, h1;
    herds_count(w, &a0, &n0, &h0);
    if (n0) player_herds_start(w);
    herds_count(w, &a1, &n1, &h1);
    if (g_log) log_locked("# herds: at %.1f min (game type %d): %d animals and fish, %d of them in no herd, %d herds%s - after: %d in no herd, %d herds\n",
                          now / 60000.0, *game_type_var(), a0, n0, h0, n0 ? ", the game's start pass again" : ", nothing to do", n1, h1);
}
internal void herds_install(void) {
    g_herds_on = g_set.map_herds;
    if (g_log) fprintf(g_log, "# herds: map_herds %d\n", g_herds_on);
}
