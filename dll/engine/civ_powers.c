// CIVILIZATION POWERS IN SKIRMISHES ([general] skirmish_civ_powers, Art of Conquest only; the player, 2026-11-23: "a
// feature to disable the civ special abilities in aoc for skirmishes"). AoC gives every civilization a power on top of
// its bonuses - db\dbcivpowers.dat's 23 (Flaming Arrows, Slavery, Priest Tower, Crusaders ... Just In Time
// Manufacturing): civilization attributes 38-60 beside the 2017 update's 1-37 in the player's list of them
// (player_civ_attributes: (level << 6) | id). They take effect in two ways only: applied (apply_civ_attribute - a
// power's grants its tech, the special unit's or building's, and does its own: the game's start, a civilization file's
// load, Adaptation's steal all go through it) and asked (has_civ_power, some 45 places: Slavery's captives, Flaming
// Arrows' fire, Adaptation's steal on a conversion ...). Off, in a random-map game: a power is applied as nothing and
// asked, no civilization has it - and once the players' civilizations are there the powers are taken out of their
// lists, so a save holds none either (its .ageofai keeps the flag: a saved game plays on as it began).
internal uint8_t g_cp_off;   // this game without the civilization powers
internal int g_cp_hooked;
internal has_civ_power_fn g_cp_has;
internal apply_civ_attribute_fn g_cp_apply;
#define CP_FIRST 38
#define CP_LAST 60
internal int cp_is_power(int attribute) { int id = attribute & 0x3F; return id >= CP_FIRST && id <= CP_LAST; }

internal char __thiscall cp_hook_has(void *powers, void *player, int power) {
    return g_cp_off ? 0 : g_cp_has(powers, player, power);
}
internal char __thiscall cp_hook_apply(void *table, int attribute, int player_id) {
    if (g_cp_off && cp_is_power(attribute)) return 1;   // done, as far as its callers go: the next attribute's turn
    return g_cp_apply(table, attribute, player_id);
}

// a game's start, before the game's (loading: before a saved game's file is read - its block sets the flag): a random
// map by the setting, a scenario, campaign or the editor's test with its powers
internal void civ_powers_new_game(int mode, int loading) {
    if (loading) { g_cp_off = 0; return; }
    if (mode != 2) g_cp_off = g_cp_hooked && mode == 0 && !g_set.skirmish_civ_powers;
    if (g_cp_hooked) log_line("# civilization powers: %s\n", g_cp_off ? "OFF in this game (skirmish_civ_powers=0)" : "as the game has them");
}
// the end of each step (eeaimod.c ai_step_end) in such a game: the powers out of the players' lists. Not once at the
// game's start: its players are made and their civilizations loaded and applied over its first steps after it
// returns, one here, the others a step or two later (and a civilization loaded later brings its own) - and not where
// an attribute is applied, which goes along the list. 16 lists of some 17 numbers a step
internal void civ_powers_step(void) {
    if (!g_cp_off) return;
    for (unsigned i = 0; i < game_player_count() && i < 16; i++) {
        ee_player *pl = game_players()[i];
        uint16_t **v = pl ? player_civ_attributes(pl) : NULL;
        if (!v || !v[0] || v[1] <= v[0]) continue;
        uint16_t *w = v[0];
        for (uint16_t *a = v[0]; a < v[1]; a++) {
            if (!cp_is_power(*a)) *w++ = *a;
            else log_line("# civilization powers: player %u's power %d taken out of its civilization\n", i, *a & 0x3F);
        }
        v[1] = w;
    }
}
internal void civ_powers_install(void) {
    if (!build_is("aoc")) return;
    g_cp_hooked = patch_civ_powers(cp_hook_has, &g_cp_has, cp_hook_apply, &g_cp_apply);
    if (g_log) fprintf(g_log, "# civilization powers: skirmish_civ_powers=%d (%s)\n", g_set.skirmish_civ_powers,
                       g_cp_hooked ? "hooked" : "FAILED: not as expected - the powers stay");
}
