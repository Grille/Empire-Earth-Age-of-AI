// STORMS ([ai_base] storm_dodge; the game's computer player off) - the player 2026-11-18: "ships dont avoid enemy
// storms yet i think, getting away from them should override evrething else sinc ethey be sunk un seconds by it".
// The prophet's hurricane is a unit of its caster's (type hurricane, 9999 hit points, attack 0: no warship to the
// fleet's scan, nothing at all to the AI before this) that stands where it was cast for 40-60 s and hits every unit of
// the caster's enemies within about 5 tiles of its middle for 100 about every second, whatever its kind - measured
// 2026-11-18 (Small Islands from epoch 9, `cast`): Good Hope frigates 1350 -> 250 in about 10 s, a dreadnought losing
// 100 a second at 4.3 tiles and nothing at 5.1; the Middle Ages' frigate (540) is gone in 5 s. An ally's storm hurts
// nobody of the alliance (the caster's stance decides, EECCHurricane::vf14 0x4AB020). The Anti-Matter Storm (its unit
// anti_matter_storm) is taken as the same - not measured.
// THE DODGE: every ST_LOOK each ship of the AI's (warships, transports, fishing boats) within ST_R of an enemy's storm
// it sees sails straight away from the storm's middle to open water of its sea ST_OUT out (the nearest way out of the
// directions round that, none into another storm), whatever it was doing - storm_tick runs after every manager that
// orders ships (eeaimod.c, after expand_tick), so its order is the one the ship carries out, given again while a
// manager's took its place; once out the managers have it back. And the storms are danger zones of the fleet's scan
// (fleet.c fl_scan): the squadrons send nobody in (sq_safe, the raids), sq_care takes out those inside.
// "# storms:" every minute with any (the dodge off too): storms seen, dodge orders, ships caught in one at a look, ships lost in one
// (in reach at the last look and gone at the next).
// MEASURED 2026-11-18, head-to-head (Small Islands large, 4 AIs, 60 min, seeds 1-12 mirrored, the dodge B; the AIs'
// prophets cast about 5 hurricanes an AI a game): from epoch 9 (22 games, 2 lost to the renderer's crash at
// engine+3b72c) ships lost in a storm 0.19 per AI against 1.85, looks with a ship caught in one 39 against 86, B - A
// kills +3.6 (se 3.2), losses -0.2 (4.1); from epoch 5 (24) lost in one 0.90 against 3.50, caught 33 against 50,
// losses -4.0 (3.7), kills -1.0 (4.5). On.
#define ST_LOOK 250
#define ST_OUT 12          // sailed to this far from the storm's middle
#define ST_MAX 16          // storms at a time
#define ST_IN 48           // ships of an AI in reach at a time (followed for the losses)
internal struct st_world { struct { ee_unit *u; ivec2 p; ee_player *o; } s[ST_MAX]; int n; uint32_t t; } g_st;
internal struct st_st {
    struct { ee_unit *u; int id; ivec2 p; } in[ST_IN]; int nin;
    ee_unit *known[8]; int ki;            // the storms counted lately
    uint32_t t, log_t;
    unsigned seen, orders, caught, lost;
} g_stp[16];
// the storms on the map (all players'), once every ST_LOOK for all AIs
internal void st_find(uint32_t now) {
    if (g_st.t && now >= g_st.t && now - g_st.t < ST_LOOK) return;
    g_st.t = now ? now : 1;
    g_st.n = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        if (!is_storm(unit_type_get_id(it.td)) || unit_get_hp(it.u) <= 0 || unit_get_x(it.u) < 0 || g_st.n == ST_MAX) continue;
        g_st.s[g_st.n].u = it.u; g_st.s[g_st.n].p = unit_get_tile(it.u); g_st.s[g_st.n++].o = unit_get_owner(it.u);
    }
}
internal int st_hits(ee_player *pl, int k) { return is_enemy(g_st.s[k].o, pl) && seen_by(g_st.s[k].u, pl); }
// a point of the sea oc ST_OUT from the storm k's middle, the way from it to the ship first, then turning by 30
// degrees either way; none in reach of another storm that hits pl
internal int st_way_out(ee_player *pl, int k, ivec2 at, int oc, ivec2 *out) {
    ivec2 c = g_st.s[k].p;
    float dx = (float)(at.x - c.x), dy = (float)(at.y - c.y), len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f) { dx = 1.0f; dy = 0.0f; len = 1.0f; }
    float a0 = atan2f(dy, dx);
    for (int i = 0; i <= 10; i++) {   // 0, +30, -30, +60 ... 180
        float a = a0 + (float)((i + 1) / 2) * (i & 1 ? 0.5236f : -0.5236f);
        ivec2 q = iv2(c.x + (int)lroundf(cosf(a) * ST_OUT), c.y + (int)lroundf(sinf(a) * ST_OUT));
        if ((int)area_id(q, 1) != oc) continue;
        int bad = 0;
        for (int j = 0; j < g_st.n && !bad; j++) bad = j != k && st_hits(pl, j) && ivec2_dist2(g_st.s[j].p, q) <= ST_R * ST_R;
        if (!bad) { *out = q; return 1; }
    }
    return 0;
}
internal void st_log(unsigned idx, struct st_st *z, uint32_t now) {
    if (!g_log || (!z->seen && !z->caught) || (z->log_t && now >= z->log_t && now - z->log_t < 60000)) return;
    z->log_t = now ? now : 1;
    log_locked("# storms: P%u at %.1f min: enemy storms seen %u, dodge orders %u, ships caught in one %u (looks), lost in one %u\n",
               idx, now / 60000.0, z->seen, z->orders, z->caught, z->lost);
}
internal void storm_tick(ee_player *pl, uint32_t now) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct st_st *z = &g_stp[idx];
    if (now < z->t) memset(z, 0, sizeof *z);   // a new game
    if (GAME_AI(idx) || (z->t && now - z->t < ST_LOOK)) return;   // the counts with the dodge off too (a test's A side)
    int dodge = STORM_DODGE(idx);
    z->t = now ? now : 1;
    st_find(now);
    static struct { ee_unit *u; int id; ivec2 p; } in[ST_IN];
    int nin = 0;
    for (int k = 0; k < g_st.n; k++) {
        if (!st_hits(pl, k)) continue;
        int j = 0;
        while (j < 8 && z->known[j] != g_st.s[k].u) j++;
        if (j == 8) { z->known[z->ki] = g_st.s[k].u; z->ki = (z->ki + 1) & 7; z->seen++; }
        ivec2 c = g_st.s[k].p;
        for (struct grid_it it = grid_it_box(c.x, c.y, ST_R, 1); grid_it_next(&it); ) {
            ee_unit *u = it.u;
            int fam = unit_type_get_class_id(it.td);
            if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
            if (fam != CLS_NAVAL && fam != CLS_SUBMERGED_NAVAL && fam != CLS_NAVAL_TRANSPORT && fam != CLS_NAVAL_CITIZEN) continue;
            ivec2 at = unit_get_tile(u);
            if (ivec2_dist2(at, c) > ST_R * ST_R) continue;
            int twice = 0;   // in reach of another storm too: dodged from the first
            for (int i = 0; i < nin && !twice; i++) twice = in[i].u == u;
            if (twice) continue;
            if (nin < ST_IN) { in[nin].u = u; in[nin].id = unit_get_serial(u); in[nin++].p = at; }
            z->caught++;
            ivec2 q;
            if (!dodge || !st_way_out(pl, k, at, fl_ship_ocean(u), &q)) continue;
            if (goal_near(u, (float)q.x + 0.5f, (float)q.y + 0.5f, 2.0f)) continue;   // on its way out already
            if (!cl_take(u, CLP_SURVIVE, CLW_STORM, CL_SURVIVE_T)) continue;   // ORDERS BY PRIORITY (claims.c): before the repair and the squadron
            if (unit_goal(u, goal_move((float)q.x + 0.5f, (float)q.y + 0.5f), 0, 0x1F40)) z->orders++;
        }
    }
    // LOST IN ONE: in reach at the last look, not found alive near where it was now
    for (int i = 0; i < z->nin; i++) {
        int found = 0;
        for (int j = 0; j < nin && !found; j++) found = in[j].u == z->in[i].u && in[j].id == z->in[i].id;
        for (struct grid_it it = grid_it_box(z->in[i].p.x, z->in[i].p.y, 3, 1); !found && grid_it_next(&it); )
            found = it.u == z->in[i].u && unit_get_serial(it.u) == z->in[i].id && unit_get_hp(it.u) > 0;
        if (!found) z->lost++;
    }
    z->nin = nin;
    for (int i = 0; i < nin; i++) { z->in[i].u = in[i].u; z->in[i].id = in[i].id; z->in[i].p = in[i].p; }
    st_log(idx, z, now);
}
