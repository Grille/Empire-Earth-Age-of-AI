// UNITS BEFORE BUILDINGS (`units_first`): a unit that picked a building by itself turns on an enemy unit that comes
// near, between its shots. The unit scripts search for a target only when idle (generic attack.tai: Idle's
// CanISeeEnemy - the scan 0x61AC11 through the search unit_functions.find_target); once a unit attacks
// (FaceEnemyUnit, AttackEnemyUnit, WaitForReload) it keeps its target until that dies or goes out of sight, so a unit
// whose first look found a building shot at it while the enemy's soldiers walked up and shot it (the player
// 2026-10-28, P6's tanks at their town: "they just attack the first thing they see - once they looked at a building
// they keep shooting at that"). Running that search again doesn't help: it rates a building beside the unit (in range,
// near) above a soldier a few tiles off - on a copy of the player's save 6438 looks found a unit 13 times. So the
// unit's own rule: every UF_EVERY ms at the safe point, a unit waiting to reload or turning to its target whose attack
// goal is its own (goal +0x1C 0: not an order - a player's click or the AI's order is never changed) and aimed at a
// building takes the nearest enemy unit within its range + UF_REACH tiles that its player sees and its weapon can hurt
// (the to-hit table's bonus above 0, as the search asks) - in the same goal (unit_functions.attack_retarget, the game's
// own: the target swapped, CheckRange as after an order; an attack-move under the goal stays). Every unit of every
// player alike: the same on every PC. Measured on that save (638-672 min, exact runs): 70 turns in 2438 looks; P6
// killed 185 and lost 229 (off: 177, 209) - the player's defenders turn as well -, with attack_commit 189 / 169 (commit
// alone 224 / 133). Off until a map that sets an attack against defenders among their buildings shows it.

#define UF_EVERY 500
#define UF_REACH 4
#define UF_STATE_WAIT_RELOAD 2021   // WaitForReload
#define UF_STATE_FACE 2012          // FaceEnemyUnit

internal int g_uf_on;
internal unsigned g_uf_looked, g_uf_switched;   // `units_first` lines: building attackers looked at, targets changed

internal int uf_building(ee_unit *u) {
    ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
    if (!td) return 0;
    switch (unit_type_get_class_id(td)) {
    case CLS_BUILDING: case CLS_NAVAL_BUILDING: case CLS_GATE: case CLS_FORTRESS: case CLS_BUILDING_PRIEST: case CLS_SPACE_BUILDING: return 1;
    default: return 0;
    }
}
// a unit worth turning on: one that walks, sails or hovers (not aircraft passing over, shots, animals, resources)
internal int uf_unit(ee_unit_type *td) {
    switch (unit_type_get_class_id(td)) {
    case CLS_LAND: case CLS_LAND_CITIZEN: case CLS_LAND_PRIEST: case CLS_LAND_TRANSPORT: case CLS_LAND_MILITARY_HERO: case CLS_NAVAL:
    case CLS_NAVAL_TRANSPORT: case CLS_NAVAL_CITIZEN: case CLS_SUBMERGED_NAVAL: case CLS_HELICOPTER: case CLS_HELICOPTER_TRANSPORT:
    case CLS_HELICOPTER_PRIEST: return 1;
    default: return 0;
    }
}
internal int uf_seen(ee_unit *u, unsigned pi) {
    ee_unit_vis *v = unit_get_vis(u);
    uint16_t *vis = v ? unit_vis_get_seen(v) : NULL;
    return vis && pi < 16 && sight_count(vis[pi]) != 0;
}
// the nearest enemy unit u can hurt within its range + UF_REACH that its player sees, NULL none
internal ee_unit *uf_pick(ee_unit *u) {
    ee_player *pl = unit_get_owner(u);
    ee_unit_type *td = unit_get_type_data(u);
    ee_unit_stats *st = unit_get_stats(u);
    if (!pl || !td) return NULL;
    unsigned pi = (unsigned)player_get_index(pl);
    float r = (st ? unit_stats_get_range(st) : unit_type_get_range(td)) + UF_REACH;
    ivec2 p = unit_get_tile(u);
    ee_unit *best = NULL;
    long bd = 0;
    for (struct grid_it it = grid_it_box(p.x, p.y, (int)r + 1, 1); grid_it_next(&it); ) {
        ee_unit *e = it.u;
        ee_player *o = unit_get_owner(e);
        if (!it.td || !uf_unit(it.td) || unit_get_hp(e) <= 0 || unit_get_x(e) < 0 || !is_enemy(pl, o)) continue;
        long d = ivec2_dist2(unit_get_tile(e), p);
        if ((float)d > r * r || (best && d >= bd) || !uf_seen(e, pi) || attack_bonus(td, it.td, (int)pi, player_get_index(o)) <= 0.0f) continue;
        best = e; bd = d;
    }
    return best;
}

// the safe point (control.c's hook_sim_receive)
internal void units_first_tick(void) {
    static uint32_t last;
    uint32_t now = game_time();
    if (!g_uf_on || !game_map()) return;
    if (now < last) last = 0;   // a new game
    if (last && now - last < UF_EVERY) return;
    last = now ? now : 1;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        unsigned st = unit_ai_state_id(u);
        if (st != UF_STATE_WAIT_RELOAD && st != UF_STATE_FACE) continue;
        ee_goal *g = unit_get_goal(u);
        if (!g || !goal_is_attack(g) || goal_get_ordered(g) || unit_get_hp(u) <= 0 || !uf_building(goal_get_target(g))) continue;
        g_uf_looked++;
        ee_unit *t = uf_pick(u);
        if (!t) continue;
        attack_retarget(u, g, t);
        g_uf_switched++;
    }
    static uint32_t logged;
    if (g_log && (now < logged || now - logged >= 60000)) {
        logged = now;
        fprintf(g_log, "# units_first: at %.1f min: %u looks at a building's attackers, %u turned on a unit\n", now / 60000.0,
                g_uf_looked, g_uf_switched);
    }
}

internal void units_first_new_game(void) { g_uf_looked = g_uf_switched = 0; }

internal void units_first_install(void) {
    g_uf_on = g_set.units_first;
    if (g_log) fprintf(g_log, "# units_first: %d\n", g_uf_on);
}
