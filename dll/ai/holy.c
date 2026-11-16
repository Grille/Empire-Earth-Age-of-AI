// THE HOLY - priests and prophets while the game's computer player is off (the player 2026-11-16: "the ai needs to make
// use of religuess units, espacialy in the early ages they are very powerfull offensive units. they probably should get
// their own cattegory like siege. prophets are spezial strike units, they can acompany and push as siege (earthquake)
// or can strike on their own as hit and run, sneak in, spawn a desaster in unprotected territory and run away again.
// priest can be used to snatch away citecens away from the pack or lone soilders").
// Neither is one of stations.c's soldiers (a priest is of the class Land Priest, a prophet no land soldier: econ.c
// land_soldier): they are this file's alone. Every HL_EVERY for each AI:
//   TRAINING  - with a finished Temple: prophets up to HL_PROPHETS (one more every HL_PROPHET_EPOCHS epochs), priests up
//               to HL_PRIESTS, one at a time, when the bank holds the price (HL_BANK times it; nothing is saved for
//               them: the priority list knows them not).
//   PROPHETS  - a power cast where it hits most, the caster's power its cost or more (the calamities' own costs,
//               game/api goal.c calamity_cost: earthquake 50, volcano 60, fire 75, hurricane, plague, malaria 100):
//               enemy buildings (HL_BUILDINGS within HL_CLUSTER of one: earthquake, else fire, else volcano), enemy
//               soldiers (HL_SOLDIERS: plague, else hurricane), citizens (HL_CITIZENS: malaria, else plague) - what it
//               knows within HL_REACH of it. With the force meeting the enemy (stations.c sn_need_at) it goes with it:
//               HL_BEHIND tiles behind that place, as the siege. Else, power for an earthquake at least: A STRIKE -
//               the nearest enemy building it knows within HL_STRIKE with no enemy soldiers near (raiders_near) and
//               under no enemy tower (work.c wk_danger) is walked to, cast at, and it walks home again; hurt, or the
//               target guarded since, it turns home at once.
//   PRIESTS   - a conversion (the game's attack order: a priest's attack converts) of an enemy citizen or soldier
//               within HL_PRIEST_LOOK that stands alone (no other enemy soldier within HL_ALONE of it) and under no
//               tower, citizens first, the nearest; none near enemy soldiers that outnumber them. Idle: with the force
//               (HL_BEHIND behind where it meets the enemy), else at home. Hurt with enemies by: home.
// "# holy:" at every cast and strike, and once a minute in tests.
#define HL_EVERY 2000
#define HL_TRAIN_EVERY 20000
#define HL_PROPHETS 1
#define HL_PROPHET_EPOCHS 4
#define HL_PROPHETS_MAX 4
#define HL_PRIESTS 3
#define HL_BANK 1
#define HL_REACH 14
#define HL_TOWER_LOOK 16      // THE TOWERS: an enemy tower within this
#define HL_QUAKE_OFF 3        // ... gets the earthquake this many tiles before it
#define HL_CLUSTER 4
#define HL_BUILDINGS 2
#define HL_SOLDIERS 6
#define HL_CITIZENS 5
#define HL_BEHIND 8
#define HL_STRIKE 70
#define HL_PRIEST_LOOK 12
#define HL_ALONE 5
#define HL_HURT 0.6f
#define HL_UNITS 64
#define HL_SEEN 512
internal const int k_hl_tech[6] = { 1018, 1013, 1017, 1019, 1022, 1016 };   // dbtechtree: earthquake, fire, volcano, plague, hurricane, malaria
enum { HLP_QUAKE, HLP_FIRE, HLP_VOLCANO, HLP_PLAGUE, HLP_HURRICANE, HLP_MALARIA, HLP_N };
internal struct hl_st {
    uint32_t t, log_t, train_t;
    struct { int id; ivec2 strike; uint32_t strike_t; } p[HL_UNITS]; int np;   // prophets on a strike: its target, since
    struct { int id; uint32_t t; } ord[2 * HL_UNITS]; int nord;   // its last cast or conversion order, when (A GIVEN ORDER)
    unsigned casts[HLP_N], strikes, converts, trained_p, trained_c, towers;
} g_hl[16];
// A GIVEN ORDER: a cast or a conversion walks first - the unit's goal is a move meanwhile (the cast goal's own walk into
// reach, EEUGLocation) - and is not given again for HL_ORDER_WAIT while the unit has a goal (the first count: 130 to 180
// "casts" an hour, every 2 s one)
#define HL_ORDER_WAIT 20000
internal int hl_ordered(struct hl_st *h, ee_unit *u, uint32_t now) {
    int id = unit_get_id(u);
    for (int k = 0; k < h->nord; k++) if (h->ord[k].id == id) return now >= h->ord[k].t && now - h->ord[k].t < HL_ORDER_WAIT && unit_get_goal(u);
    return 0;
}
internal void hl_order_note(struct hl_st *h, ee_unit *u, uint32_t now) {
    int id = unit_get_id(u), n = 0;
    for (int k = 0; k < h->nord; k++) if (h->ord[k].id != id && now >= h->ord[k].t && now - h->ord[k].t < HL_ORDER_WAIT) h->ord[n++] = h->ord[k];
    if (n < 2 * HL_UNITS) { h->ord[n].id = id; h->ord[n++].t = now ? now : 1; }
    h->nord = n;
}
internal void holy_new_game(void) { memset(g_hl, 0, sizeof g_hl); }
// a power's cost for pl (-1: not its, or unknown)
internal int hl_cost(ee_player *pl, int k) {
    ee_tech_tree *tt = player_get_tech_tree(pl);
    ee_tech *t = tt ? tech_tree_lookup(tt, k_hl_tech[k]) : NULL;
    ee_tech_data *d = t ? tech_get_data(t) : NULL;
    return d ? calamity_cost(tech_data_get_calamity(d)) : -1;
}
internal int hl_cal(ee_player *pl, int k) {   // the power's calamity number, -1 none
    ee_tech_tree *tt = player_get_tech_tree(pl);
    ee_tech *t = tt ? tech_tree_lookup(tt, k_hl_tech[k]) : NULL;
    ee_tech_data *d = t ? tech_get_data(t) : NULL;
    return d ? tech_data_get_calamity(d) : -1;
}
// the power's own check first (goal.c calamity_check: the caster's power, the tile explored, no enemy Temple's
// protection there) - without it the first runs gave casts the game dropped at once, every 2 s one
internal int hl_cast(ee_player *pl, ee_unit *u, ivec2 at, int k) {
    if (calamity_check(hl_cal(pl, k), u, at.x, at.y) != 0) return 0;
    void *mem = goal_cast_alloc();
    ee_goal *g = mem ? goal_cast_init(mem, u, NULL, at.x, at.y, k_hl_tech[k]) : NULL;
    return g && unit_goal(u, g, 0, 0x1F40);
}
internal int hl_walk(ee_unit *u, ivec2 to) {
    ivec2 p = unit_get_tile(u);
    if (ivec2_dist2(p, to) <= 3 * 3) return 0;
    ee_goal *g = unit_get_goal(u);
    if (g && goal_is_location(g) && fabsf(goal_get_dest_x(g) - (float)to.x) < 3.0f && fabsf(goal_get_dest_y(g) - (float)to.y) < 3.0f) return 0;   // on its way
    return as_order(u, goal_move((float)to.x + 0.5f, (float)to.y + 0.5f));
}
// HL_BEHIND tiles from front towards from
internal ivec2 hl_behind(ivec2 front, ivec2 from) {
    float dx = (float)(from.x - front.x), dy = (float)(from.y - front.y), d = sqrtf(dx * dx + dy * dy);
    if (d < 1.0f) return front;
    return iv2(front.x + (int)(dx / d * HL_BEHIND), front.y + (int)(dy / d * HL_BEHIND));
}
internal void holy_tick(ee_player *pl, uint32_t now) {
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    struct hl_st *h = &g_hl[idx];
    if (!pl || GAME_AI(idx)) return;
    if (now < h->t) memset(h, 0, sizeof *h);
    if (h->t && now - h->t < HL_EVERY) return;
    h->t = now ? now : 1;
    static ee_unit *pro[HL_UNITS], *pri[HL_UNITS];
    static ivec2 town[64];
    int npro = 0, npri = 0, ntown = 0, temple = 0, count = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
        int t = unit_type_get_id(td);
        if (t == UT(PROPHET) && npro < HL_UNITS) pro[npro++] = u;
        else if (t == UT(PRIEST) && npri < HL_UNITS) pri[npri++] = u;
        else if (t == UT(B_TEMPLE) || t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL) || t == UT(B_SETTLEMENT)) {
            ee_unit *st = unit_get_physics(u);
            if (!st || !unit_get_finished(st)) continue;
            if (t == UT(B_TEMPLE)) temple = 1;
            else if (ntown < 64) town[ntown++] = unit_get_tile(u);
        }
    }
    // TRAINING
    if (temple && (!h->train_t || now < h->train_t || now - h->train_t >= HL_TRAIN_EVERY)) {
        h->train_t = now ? now : 1;
        int wp = HL_PROPHETS + current_epoch(pl) / HL_PROPHET_EPOCHS;
        if (wp > HL_PROPHETS_MAX) wp = HL_PROPHETS_MAX;
        int type = npro < wp ? UT(PROPHET) : npri < HL_PRIESTS ? UT(PRIEST) : 0;
        if (type == UT(PROPHET) && hl_cost(pl, HLP_QUAKE) < 0) type = npri < HL_PRIESTS ? UT(PRIEST) : 0;   // no powers for it
        const int *stock = player_stock_at(pl);
        for (int r = 2; type && r < 7; r++) if (stock[r] < HL_BANK * unit_cost(pl, type, r)) type = 0;
        ee_civ_mgr *cm = player_get_civ_mgr(pl);
        ee_unit_mgr *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
        if (type && um && ut_has(type) && can_build(pl, type)) {
            int req[4] = { type, 1, 0, 0 };
            unsigned got = 0;
            g_priority_mil = 1;   // its own order: no substitute
            hook_train_units(um, req, (uintptr_t)&got);
            g_priority_mil = 0;
            if (type == UT(PROPHET)) h->trained_p += got; else h->trained_c += got;
        }
    }
    ivec2 front;
    int fight = sn_need_at(idx, &front);
    // PROPHETS
    struct { int id; ivec2 strike; uint32_t strike_t; } keep[HL_UNITS];
    int nk = 0;
    static ee_unit *seen[HL_SEEN];
    for (int i = 0; i < npro; i++) {
        ee_unit *u = pro[i];
        ivec2 p = unit_get_tile(u);
        int id = unit_get_id(u), k0 = 0;
        while (k0 < h->np && h->p[k0].id != id) k0++;
        ivec2 strike = k0 < h->np ? h->p[k0].strike : iv2(-1, -1);
        uint32_t strike_t = k0 < h->np ? h->p[k0].strike_t : 0;
        ee_goal *g = unit_get_goal(u);
        int casting = (g && goal_is_cast(g)) || hl_ordered(h, u, now);
        ivec2 home = p;
        for (int k = 0, bd = -1; k < ntown; k++) { int d = ivec2_dist2(town[k], p); if (bd < 0 || d < bd) { bd = d; home = town[k]; } }
        ee_unit_type *td = unit_get_type_data(u);
        int hurt = (float)unit_get_hp(u) < HL_HURT * (float)unit_type_get_hp(td);
        int pw = unit_get_power(u), cost[HLP_N];
        for (int k = 0; k < HLP_N; k++) cost[k] = hl_cost(pl, k);
        // THE TOWERS (the player 2026-11-16: "on towers, that where prophets are espacialy usefull if the enemy forgot its
        // temple. think that the prophet dosnt has to walk up to the tower, casting the earthquake a few tiles before the
        // tower from savethy is enough to bring it down"): the nearest enemy tower it knows within HL_TOWER_LOOK (a
        // building with a ground attack, attack.c defence_stats) gets an earthquake HL_QUAKE_OFF tiles before it on the
        // prophet's side - the prophet casts from its reach of that spot, outside the tower's. One under an enemy Temple's
        // protection fails the power's check and is left.
        if (!casting && cost[HLP_QUAKE] > 0 && pw >= cost[HLP_QUAKE]) {
            ivec2 tp = p;
            int bd = -1;
            float rr, aa;
            for (struct grid_it it = grid_it_box(p.x, p.y, HL_TOWER_LOOK, 1); grid_it_next(&it); ) {
                ee_player *o = unit_get_owner(it.u);
                if (!o || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || !seen_by(it.u, pl) || !defence_stats(it.u, &rr, &aa)) continue;
                ivec2 q = unit_get_tile(it.u);
                int d = ivec2_dist2(q, p);
                if (d > HL_TOWER_LOOK * HL_TOWER_LOOK || (bd >= 0 && d >= bd)) continue;
                bd = d; tp = q;
            }
            if (bd >= 0) {
                float dx = (float)(p.x - tp.x), dy = (float)(p.y - tp.y), dl = sqrtf(dx * dx + dy * dy);
                ivec2 at = dl < 1.0f ? tp : iv2(tp.x + (int)(dx / dl * HL_QUAKE_OFF + 0.5f), tp.y + (int)(dy / dl * HL_QUAKE_OFF + 0.5f));
                if (hl_cast(pl, u, at, HLP_QUAKE)) {
                    hl_order_note(h, u, now);
                    h->casts[HLP_QUAKE]++; h->towers++;
                    strike = iv2(-1, -1); strike_t = 0;
                    if (g_log) log_locked("# holy: P%u at %.1f min: a prophet at %d,%d casts an earthquake at %d,%d before the enemy tower at %d,%d (power %d, cost %d)\n",
                                          idx, now / 60000.0, p.x, p.y, at.x, at.y, tp.x, tp.y, pw, cost[HLP_QUAKE]);
                    casting = 1;
                }
            }
        }
        if (!casting) {
            // what it knows within HL_REACH: the best place for each kind of power
            int nseen = 0;
            for (struct grid_it it = grid_it_box(p.x, p.y, HL_REACH, 1); grid_it_next(&it) && nseen < HL_SEEN; ) {
                ee_player *o = unit_get_owner(it.u);
                if (!o || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || !seen_by(it.u, pl) || ivec2_dist2(unit_get_tile(it.u), p) > HL_REACH * HL_REACH) continue;
                seen[nseen++] = it.u;
            }
            int best_n[3] = { 0, 0, 0 };   // buildings, soldiers, citizens
            ivec2 best_at[3];
            for (int a = 0; a < nseen; a++) {
                ee_unit_type *ta = unit_get_type_data(seen[a]);
                int fa = unit_type_get_class_id(ta), t = unit_type_get_id(ta);
                int kind = (fa == CLS_BUILDING || fa == CLS_FORTRESS) && !is_wall(t) ? 0 : is_citizen_type(t) ? 2 : is_soldier(ta) && fa == CLS_LAND ? 1 : -1;
                if (kind < 0) continue;
                ivec2 q = unit_get_tile(seen[a]);
                int c = 0;
                for (int b = 0; b < nseen; b++) {
                    ee_unit_type *tb = unit_get_type_data(seen[b]);
                    int fb = unit_type_get_class_id(tb), tt = unit_type_get_id(tb);
                    int kb = (fb == CLS_BUILDING || fb == CLS_FORTRESS) && !is_wall(tt) ? 0 : is_citizen_type(tt) ? 2 : is_soldier(tb) && fb == CLS_LAND ? 1 : -1;
                    c += kb == kind && ivec2_dist2(unit_get_tile(seen[b]), q) <= HL_CLUSTER * HL_CLUSTER;
                }
                if (c > best_n[kind]) { best_n[kind] = c; best_at[kind] = q; }
            }
            int pick = -1;
            ivec2 at = p;
            static const int order[3][3] = { { HLP_QUAKE, HLP_FIRE, HLP_VOLCANO }, { HLP_PLAGUE, HLP_HURRICANE, -1 }, { HLP_MALARIA, HLP_PLAGUE, -1 } };
            static const int need[3] = { HL_BUILDINGS, HL_SOLDIERS, HL_CITIZENS };
            for (int kind = 1; kind < 4 && pick < 0; kind++) {   // soldiers, then citizens, then buildings
                int kd = kind == 3 ? 0 : kind;
                if (best_n[kd] < need[kd]) continue;
                for (int j = 0; j < 3 && pick < 0; j++) { int k = order[kd][j]; if (k >= 0 && cost[k] > 0 && pw >= cost[k]) { pick = k; at = best_at[kd]; } }
            }
            if (pick >= 0 && hl_cast(pl, u, at, pick)) {
                hl_order_note(h, u, now);
                h->casts[pick]++;
                strike = iv2(-1, -1);   // done: home (below, the next look)
                strike_t = 0;
                if (g_log) log_locked("# holy: P%u at %.1f min: a prophet at %d,%d casts %s at %d,%d (power %d, cost %d; %d buildings, %d soldiers, %d citizens of the enemy there)\n",
                                      idx, now / 60000.0, p.x, p.y, (const char *[]){ "an earthquake", "a fire", "a volcano", "the plague", "a hurricane", "malaria" }[pick],
                                      at.x, at.y, pw, cost[pick], best_n[0], best_n[1], best_n[2]);
                casting = 1;
            }
        }
        if (!casting) {
            if (strike.x >= 0 && (hurt || cost[HLP_QUAKE] <= 0 || pw < cost[HLP_QUAKE] || raiders_near(pl, strike, 10) || (strike_t && now - strike_t > 300000))) strike = iv2(-1, -1);   // A STRIKE called off
            if (strike.x < 0 && !hurt && fight) hl_walk(u, hl_behind(front, home));   // with the force, as the siege
            else if (strike.x < 0 && !hurt && cost[HLP_QUAKE] > 0 && pw >= cost[HLP_QUAKE]) {   // A STRIKE: the nearest enemy building unguarded
                int bd = -1;
                for (struct grid_it it = grid_it_box(p.x, p.y, HL_STRIKE, 1); grid_it_next(&it); ) {
                    ee_player *o = unit_get_owner(it.u);
                    int fa = unit_type_get_class_id(it.td);
                    if (!o || !is_enemy(pl, o) || (fa != CLS_BUILDING && fa != CLS_FORTRESS) || is_wall(unit_type_get_id(it.td)) || unit_get_hp(it.u) <= 0 || !seen_by(it.u, pl)) continue;
                    ivec2 q = unit_get_tile(it.u);
                    int d = ivec2_dist2(q, p);
                    float rr, aa;
                    int tower = defence_stats(it.u, &rr, &aa);   // THE TOWERS: a tower is struck from outside its reach
                    if (d > HL_STRIKE * HL_STRIKE || (bd >= 0 && d >= bd) || raiders_near(pl, q, 10) || (!tower && wk_danger(idx, q))) continue;
                    bd = d; strike = q;
                    if (tower) {   // where it stands to cast: the tower's reach + 2 from it, on its side
                        float dx = (float)(p.x - q.x), dy = (float)(p.y - q.y), dl = sqrtf(dx * dx + dy * dy);
                        if (dl >= 1.0f) strike = iv2(q.x + (int)(dx / dl * (rr + 2.0f)), q.y + (int)(dy / dl * (rr + 2.0f)));
                    }
                }
                if (strike.x >= 0) {
                    strike_t = now ? now : 1; h->strikes++;
                    if (g_log) log_locked("# holy: P%u at %.1f min: a prophet at %d,%d strikes at the enemy building at %d,%d (power %d)\n", idx, now / 60000.0, p.x, p.y, strike.x, strike.y, pw);
                }
            }
            if (strike.x >= 0) hl_walk(u, strike);
            else if (!fight || hurt) hl_walk(u, home);
        }
        if (strike.x >= 0 && nk < HL_UNITS) { keep[nk].id = id; keep[nk].strike = strike; keep[nk++].strike_t = strike_t; }
    }
    memcpy(h->p, keep, (size_t)nk * sizeof *keep); h->np = nk;
    // PRIESTS
    for (int i = 0; i < npri; i++) {
        ee_unit *u = pri[i];
        ivec2 p = unit_get_tile(u);
        ee_goal *g = unit_get_goal(u);
        ee_unit *tg = g && goal_is_attack(g) ? goal_get_target(g) : NULL;
        ivec2 home = p;
        for (int k = 0, bd = -1; k < ntown; k++) { int d = ivec2_dist2(town[k], p); if (bd < 0 || d < bd) { bd = d; home = town[k]; } }
        int hurt = (float)unit_get_hp(u) < HL_HURT * (float)unit_type_get_hp(unit_get_type_data(u));
        int foes = raiders_near(pl, p, HL_ALONE + 1);
        if (hurt && foes) { hl_walk(u, home); continue; }
        if (tg && unit_get_hp(tg) > 0 && unit_get_owner(tg) != pl && raiders_near(pl, unit_get_tile(tg), HL_ALONE) <= 1) continue;   // converting
        if (hl_ordered(h, u, now)) continue;   // A GIVEN ORDER: on its way
        ee_unit *best = NULL;
        int bs = 0;
        for (struct grid_it it = grid_it_box(p.x, p.y, HL_PRIEST_LOOK, 1); grid_it_next(&it); ) {
            ee_player *o = unit_get_owner(it.u);
            int t = unit_type_get_id(it.td), cit = is_citizen_type(t);
            if (!o || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || !seen_by(it.u, pl) || unit_get_x(it.u) < 0) continue;
            if (!cit && !(is_soldier(it.td) && unit_type_get_class_id(it.td) == CLS_LAND)) continue;
            ivec2 q = unit_get_tile(it.u);
            int d = ivec2_dist2(q, p);
            if (d > HL_PRIEST_LOOK * HL_PRIEST_LOOK || raiders_near(pl, q, HL_ALONE) > (cit ? 0 : 1) || wk_danger(idx, q)) continue;
            int sc = 100000 - d + (cit ? 50000 : 0);
            if (!best || sc > bs) { best = it.u; bs = sc; }
        }
        if (best) {
            ee_goal *ga = goal_attack(u, best);
            if (ga && unit_goal(u, ga, 0, 0x1F40)) { h->converts++; hl_order_note(h, u, now); }
            continue;
        }
        if (tg) continue;
        hl_walk(u, fight ? hl_behind(front, home) : home);
    }
    if (g_log && g_autotest && (!h->log_t || now - h->log_t >= 60000)) {
        h->log_t = now ? now : 1;
        log_locked("# holy: P%u at %.1f min: %d prophets, %d priests, Temple %d | so far trained %u prophets and %u priests, casts quake %u fire %u volcano %u plague %u hurricane %u malaria %u, %u strikes, %u at towers, %u conversions ordered | earthquake costs %d\n",
                   idx, now / 60000.0, npro, npri, temple, h->trained_p, h->trained_c, h->casts[0], h->casts[1], h->casts[2], h->casts[3], h->casts[4], h->casts[5], h->strikes, h->towers, h->converts, hl_cost(pl, HLP_QUAKE));
    }
}
