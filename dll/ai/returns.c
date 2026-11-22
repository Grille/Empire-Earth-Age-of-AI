// THE RETURNS: what each kind of force of a player's brings back for what stands in it - the value it kills against
// the value it loses and the value fielded, the recent fighting weighed most (RT_HALF). Step 1 of the strategy plans
// (the player 2026-11-22: "dont make it a fixed share, it depends a lot on the enemy player, if they invest hevealy
// into anti air it be mostly useless"; then "the ai is fairly static in the way it aproches the game, every ai
// attempting the same style of play"): the force's sizes are to follow what pays against this enemy, and to know
// that, what pays is counted first. Only counted and logged so far - nothing reads it yet.
// The kinds (rt_kind): land soldiers (siege and heroes too), land anti-air (KIND_LAND_AA: the flak, AA tanks, Stingers -
// the bombers' losses on the first look were two thirds to land units), warships, fighters (airguard.c's roles: air fighters,
// fighter/bombers, anti-tank planes), bombers, helicopters that fight, defences (buildings that shoot: towers, anti-air
// guns, tank obstacles), the rest (citizens, fishing boats, transports, buildings that don't shoot) - a target only,
// and the killer when none of the others is known.
// A kill (diplo.c hook_kill_stat, a unit or a building killed, every player, the same on every PC): the dead one's
// base price (cover.c cv_value, F+W+S+G+I) goes to its killer's kind as killed (the unit that last hit it,
// unit_get_attacker, when it is the killer's) and to its owner's kind as lost - by the killer's kind too (lost_to: its
// aircraft brought down by the enemy's defences, by its fighters). What is fielded: the price of each kind it has
// alive, finished, every RT_EVERY. The sums fall by half every RT_HALF of game time; a game loaded starts from none.
// "# returns:" every RT_LOG a player of the mod's (tests).
#define RT_EVERY 10000
#define RT_HALF 600000
#define RT_LOG 300000
#define RT_STEP 0.98851402f   // 0.5 ^ (RT_EVERY / RT_HALF)
#define RT_PRIOR 1000.0f      // rt_worth's: killed and lost alike before the fighting says more (about 5 fighters, 2.5 bombers)
#define RT_WORTH_MIN 0.2f
#define RT_WORTH_MAX 2.0f
enum { RK_LAND, RK_LAND_AA, RK_NAVY, RK_FIGHTER, RK_BOMBER, RK_HELI, RK_DEF, RK_OTHER, RK_N };
internal const char *const k_rk[RK_N] = { "land", "land anti-air", "navy", "fighters", "bombers", "helicopters", "defences", "other" };
internal struct rt_st {
    float killed[RK_N], lost[RK_N], lost_to[RK_N][RK_N], field[RK_N];   // decayed by RT_HALF; field: as last counted
    uint32_t all_killed[RK_N], all_lost[RK_N];                          // the game's, not decayed (the log's)
    uint32_t field_t, log_t;
} g_rt[16];
internal uint32_t g_rt_decay_t;
internal int rt_kind(ee_unit_type *td) {
    if (!td) return RK_OTHER;
    int c = unit_type_get_class_id(td), t = unit_type_get_id(td), att = unit_type_get_attack(td) > 0;
    if (c == CLS_AIRPLANE) return ag_fighter(t) ? RK_FIGHTER : unit_type_get_family(td) == KIND_BOMBER ? RK_BOMBER : RK_OTHER;
    if (c == CLS_HELICOPTER) return att ? RK_HELI : RK_OTHER;
    if (c == CLS_BUILDING || c == CLS_FORTRESS || c == CLS_NAVAL_BUILDING) return att || is_tower_type(t) ? RK_DEF : RK_OTHER;
    int m = cv_mil(td);
    return m == 2 ? RK_NAVY : m == 1 ? (unit_type_get_family(td) == KIND_LAND_AA ? RK_LAND_AA : RK_LAND) : RK_OTHER;
}
// a unit or building killed (diplo.c hook_kill_stat): k the killer's index
internal void rt_death(ee_unit *dead, int k) {
    ee_player *vp = dead ? unit_get_owner(dead) : NULL;
    ee_unit_type *td = dead ? unit_get_type_data(dead) : NULL;
    int v = vp ? player_get_index(vp) : 0;
    if (!td || v <= 0 || v > 15 || k <= 0 || k > 15 || k == v) return;
    ee_unit *e = unit_get_attacker(dead);
    ee_player *ep = e ? unit_get_owner(e) : NULL;
    int kk = ep && player_get_index(ep) == k ? rt_kind(unit_get_type_data(e)) : RK_OTHER, vk = rt_kind(td);   // (referenced by the dead one: still there)
    int price = cv_value(vp, unit_type_get_id(td));
    if (price <= 0) return;
    struct rt_st *a = &g_rt[k], *b = &g_rt[v];
    a->killed[kk] += (float)price; a->all_killed[kk] += (uint32_t)price;
    b->lost[vk] += (float)price; b->lost_to[vk][kk] += (float)price; b->all_lost[vk] += (uint32_t)price;
}
internal void returns_tick(ee_player *pl, uint32_t now) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct rt_st *r = &g_rt[idx];
    if (now < g_rt_decay_t) { memset(g_rt, 0, sizeof g_rt); g_rt_decay_t = 0; }   // a new game (or one loaded from before)
    if (!g_rt_decay_t) g_rt_decay_t = now ? now : 1;
    while (now - g_rt_decay_t >= RT_EVERY) {   // every player's sums, once for all, by the game's time - a step of RT_EVERY at a time,
        for (int p = 0; p < 16; p++)            // its factor a constant: no runtime library's exp2f (multiplayer: every PC the same)
            for (int k = 0; k < RK_N; k++) {
                g_rt[p].killed[k] *= RT_STEP; g_rt[p].lost[k] *= RT_STEP;
                for (int j = 0; j < RK_N; j++) g_rt[p].lost_to[k][j] *= RT_STEP;
            }
        g_rt_decay_t += RT_EVERY;
    }
    if (r->field_t && now - r->field_t < RT_EVERY) return;
    r->field_t = now ? now : 1;
    static uint16_t n[G_TYPES_N];
    memset(n, 0, sizeof n);
    int count = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        ee_unit *st = unit_get_physics(u);
        int t = td ? unit_type_get_id(td) : -1;
        if (t < 0 || t >= G_TYPES_N || unit_get_hp(u) <= 0 || !st || !unit_get_finished(st) || n[t] == 0xFFFF) continue;
        n[t]++;
    }
    for (int k = 0; k < RK_N; k++) r->field[k] = 0;
    for (int t = 0; t < G_TYPES_N; t++) if (n[t]) r->field[rt_kind(type_data(t))] += (float)n[t] * (float)cv_value(pl, t);
    if (!g_autotest || !g_log || (r->log_t && now - r->log_t < RT_LOG)) return;
    r->log_t = now ? now : 1;
    static char line[1536];
    line[0] = 0;
    for (int k = 0; k < RK_N; k++) {
        if (!r->killed[k] && !r->lost[k] && !r->field[k] && !r->all_killed[k] && !r->all_lost[k]) continue;
        out_printf(line, sizeof line, " | %s: killed %.0f lost %.0f fielded %.0f", k_rk[k], r->killed[k], r->lost[k], r->field[k]);
        if (r->lost[k] >= 1.0f) {   // what took them: the two worst
            int w1 = -1, w2 = -1;
            for (int j = 0; j < RK_N; j++) {
                if (r->lost_to[k][j] < 1.0f) continue;
                if (w1 < 0 || r->lost_to[k][j] > r->lost_to[k][w1]) { w2 = w1; w1 = j; }
                else if (w2 < 0 || r->lost_to[k][j] > r->lost_to[k][w2]) w2 = j;
            }
            if (w1 >= 0) out_printf(line, sizeof line, " (to %s %.0f%s", k_rk[w1], r->lost_to[k][w1], w2 >= 0 ? "," : ")");
            if (w2 >= 0) out_printf(line, sizeof line, " %s %.0f)", k_rk[w2], r->lost_to[k][w2]);
        }
        out_printf(line, sizeof line, ", in all %u/%u", r->all_killed[k], r->all_lost[k]);
    }
    log_locked("# returns: P%u at %.1f min (killed, lost: value, halved every %d min)%s\n", idx, now / 60000.0, RT_HALF / 60000, line);
}
// THE AIR'S WORTH (airguard.c THE AIR FORCE'S SIZE, [autotest] v_air_worth): what its fighters (bombers) killed for
// what they lost lately, RT_PRIOR on both sides, RT_WORTH_MIN to RT_WORTH_MAX - its part of the air force's share is
// that many times its size: bombers that die over the enemy's anti-air shrink to a few that keep trying (and grow
// again as the losses fade), fighters that win their fights grow
internal float rt_worth(unsigned idx, int bombers) {
    struct rt_st *r = &g_rt[idx & 15];
    int k = bombers ? RK_BOMBER : RK_FIGHTER;
    float w = (r->killed[k] + RT_PRIOR) / (r->lost[k] + RT_PRIOR);
    return w < RT_WORTH_MIN ? RT_WORTH_MIN : w > RT_WORTH_MAX ? RT_WORTH_MAX : w;
}
