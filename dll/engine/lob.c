// LOBBING SHOT BY SHOT (`lob_each_shot`; docs/engine-notes.md "Walls and fire"). A unit whose stats may lob (the
// stats' lob: the bows but the crossbow, the slinger, the pilum, the trebuchet) shoots flat until one of its shots
// hits a wall on the way (0x61F6A4 sets its alt_attack); from then on it lobs - animation 8, the high arc, 1.5 x its
// reload - until it gets a new goal, the wall gone or not (the player 2026-11-07: "they don't switch back, even if
// the obstacle is cleared"). The player's idea: look before every shot. At the start of each attack
// (EEUGAttackUnit::tick in AttackEnemyUnit, 2001 - where the game chooses the animation by alt_attack, and fires at
// once or at its strike frame) the game's own line of fire from the unit to its target (line_of_fire_game, what the
// AI's target choice asks for the units that can't lob: a wall standing taller than the line between the two
// launch and hit points): a wall in the way - the high arc, else flat. A hill in the way stays the game's (it never
// lobs over one; a flat shot's arc mostly clears it).
// alt_attack holds the state - the game's own byte, saved with the unit and the same on every PC: 0 flat, 1 the
// game's (a flat shot hit a wall the look missed), 2 our lob, 3.. lobs held after such a hit, counted down shot by
// shot before it looks again (else it would aim flat into that wall every other shot). The game only tests the byte
// for 0 / not 0, and sets 1 only on a flat shot. In multiplayer every player needs the same setting.
#define LOB_ATTACK 2001   // the attack state (unit_ai_state_id): its animation set, the shot fired or due
#define LOB_HOLD 4        // lobs after the game saw a flat shot hit a wall, before it looks again
internal int g_lob_on;
internal struct { unsigned looked, flat, lobbed, hit, held; } g_lob;

internal void __thiscall hook_lob_tick(ee_attack_goal *g, ee_unit *u) {
    ee_unit_stats *st = unit_get_stats(u);
    ee_unit *t = attack_goal_get_target(g);
    if (t && st && unit_stats_get_lob(st) && unit_ai_state_id(u) == LOB_ATTACK) {
        unsigned a = unit_get_alt_attack(u), now;
        if (a == 1) { now = 2 + LOB_HOLD; g_lob.hit++; }
        else if (a > 2) { now = a - 1; g_lob.held++; }
        else {
            ee_unit *b = NULL; uint32_t x = 0; int y = 0;
            now = line_of_fire_game(u, t, &b, &x, &y) && b && b != t ? 2 : 0;
            g_lob.looked++;
            if (now) g_lob.lobbed++; else g_lob.flat++;
        }
        if (now != a) unit_set_alt_attack(u, (uint8_t)now);
    }
    eeug_attack_unit_tick(g, u);
}

// lob [reset]: the shots looked at (flat / lobbed), the game's wall hits after a flat look, the lobs held after them
internal void cmd_lob(char *out, const char *args) {
    if (strstr(args, "reset")) memset(&g_lob, 0, sizeof g_lob);
    REPLY("lob_each_shot=%d: looked %u (flat %u, lobbed %u); a flat shot hit a wall %u, lobs held after %u\n", g_lob_on,
          g_lob.looked, g_lob.flat, g_lob.lobbed, g_lob.hit, g_lob.held);
}

internal void lob_install(void) {
    g_lob_on = g_set.lob_each_shot;
    int ok = g_lob_on ? patch_attack_unit_tick(hook_lob_tick) : 0;
    if (g_log) fprintf(g_log, "# lob: lob_each_shot=%d (%s)\n", g_lob_on, !g_lob_on ? "the game's: lobs until a new goal" : ok ? "hooked" : "FAILED");
}
