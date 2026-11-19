// THE DIFFICULTY'S DAMAGE ([ai_base] damage_to_humans, damage_to_ais; the player 2026-11-19: "on hard 2 same units
// fighting each other the ai ones wins with half its health left, reverse on easy" - and, the choice: "damage modifiers
// always feel cheap, follow pop or epoch are better modifiers for easy"). The game builds its damage table three
// times at its load (the attack bonus of a weapon kind against a target family): plain, x 0.714 and x 1.4, and
// EEToHitTable.attack_bonus - every hit's damage (0x61DE61), the AI's estimates and the mod's own (attack.c, stats.c,
// aim.c ...) - picks one by who hits whom and the difficulty (game.game_mode, the AIs' difficulty; 0 easy, 2 hard;
// spec game.hit_table_a): easy a computer's hit on a human x 0.714 and a human's on a computer x 1.4, hard the reverse,
// medium and between two humans or two computers the plain one. The choice is the mod's now (hooks.c
// patch_damage_tables): the plain table's entry times this factor - by the difficulty's settings, 100% at every
// difficulty by default (fair: the difficulty eases off through the AI's play, its population and its epochs). A
// human here is the game's own test (EEPlayer vf29, and no player 0, nature).
internal float __attribute__((fastcall)) damage_factor(int attacker_human, int target_human) {
    if (attacker_human == target_human) return 1.0f;
    int d = game_mode();
    if (d < 0 || d > 2) d = 1;
    return attacker_human ? g_set_ai[d].damage_to_ais : g_set_ai[d].damage_to_humans;
}
internal void damage_install(void) {
    int ok = patch_damage_tables(damage_factor);
    if (g_log) fprintf(g_log, "# damage: the difficulty's tables %s - the computers' hits on humans x %.3g / %.3g / %.3g, "
                       "the humans' on computers x %.3g / %.3g / %.3g (easy / medium / hard)\n",
                       ok ? "replaced by the settings' factors" : "kept (the code differs)",
                       g_set_ai[0].damage_to_humans, g_set_ai[1].damage_to_humans, g_set_ai[2].damage_to_humans,
                       g_set_ai[0].damage_to_ais, g_set_ai[1].damage_to_ais, g_set_ai[2].damage_to_ais);
}
