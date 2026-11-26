// ART OF CONQUEST'S ENERGY FIELDS: the field between a Wall - Space's posts is an effect the game hangs on each piece
// (unit effect, EEE3DEnergyWall) where a piece is finished or laid in a game - never by the scenario editor's placing
// (it skips them there), and a scenario's walls come from its file without one: in a scenario, the editor's test play
// too, they stood bare (the player 2026-11-26: "in my current game the energi fields dont render at all" - 53 of 54
// pieces placed in the editor without one). And the game's fire uses the same slot: a damaged building's fire ends by
// deleting the effect in it, so a space wall repaired lost its field. Each second of game time (at a step's end, the
// same step on every PC; the editor's time stays 0) every piece that should have one by the game's own test and has
// none gets it as the game makes it (energy_wall_wanted / energy_wall_make). A saved game's walls keep theirs.
internal uint32_t g_ew_next;   // game time of the next look
internal void energy_walls_step(void) {
    uint32_t now = game_time();
    if (!build_is("aoc") || (now < g_ew_next && g_ew_next - now <= 1000)) return;   // further off: a new game's time
    g_ew_next = now + 1000;
    int n = 0;
    for (unsigned i = 0; i < game_player_count() && i < 16; i++) {
        ee_player *pl = game_players()[i];
        for (ee_unit *u = pl ? player_get_units(pl) : NULL; u; u = unit_get_next(u))
            if (energy_wall_wanted(u)) { energy_wall_make(u); n++; }
    }
    if (n) log_line("# energy fields: %d space wall pieces given theirs (a scenario's, or a fire's end took it)\n", n);
}
