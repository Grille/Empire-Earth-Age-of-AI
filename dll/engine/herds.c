// THE MAP'S HERDS - the pipe's and the scripts' `herds` (the player 2026-11-18: "animal ai (walking, breeding) seems
// disabled in scenarios. i skirmish it works"; then "disable the fix again, the first greek scenario for example
// depends on the animals not breading, but sorting animal into herds should become a command, than setup or individual
// scripts can enable it beside other skirmish features"). A herd (EEHerd, the world player's herds) is the animals' own
// AI: it walks its animals about and breeds them back up to its size - kill one and a young one comes of one of the
// others. A skirmish started from the menu puts the map's animals into herds as it starts (EEPlayer herds_start: each
// animal into a herd of its kind near it, else a new one); a scenario, the scenario editor's test play (unless a
// skirmish was played before) and the mod's test games (a random map after a save is loaded) leave them in none - they
// stand where the map put them and never breed (a Continental test game: 80 animals and fish, none in a herd; with the
// pass 7 herds, its deer 8 -> 13 in 4 minutes). herds_make runs the game's own pass now: an animal already in a herd is
// left as it is. config/setup.eeai's on_skirmish calls it, so a test's random map plays as a skirmish from the menu.
// The herds are the simulation's: in multiplayer only where every player runs it.
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
// the map's animals into herds now (at the safe point: a pipe command, a script); its reply in out
internal void herds_make(char *out, size_t cap) {
    ee_player *w = game_player_count() ? game_players()[0] : NULL;
    if (!w || !game_time()) { snprintf(out, cap, "error: no game running\n"); return; }
    int a0, n0, h0, a1, n1, h1;
    herds_count(w, &a0, &n0, &h0);
    if (n0) player_herds_start(w);
    herds_count(w, &a1, &n1, &h1);
    snprintf(out, cap, "ok: %d animals and fish, %d of them in no herd, %d herds before - %d in no herd, %d herds after (fish and eagles stay out, as in a skirmish)\n",
             a0, n0, h0, n1, h1);
}
