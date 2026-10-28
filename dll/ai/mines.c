// FILL THE MINES ([ai_base] mine_fill, bits; [autotest] v_mine_fill for the variant players). Included by eeaimod.c after
// food.c. The playtester: "every mine it can get hold of should be filled; the only exception is low population limits
// where room is needed for the army - but even then filling the home island's mines is always in budget."
// Measured against the base AI (docs/economy.md "Mines"): on Small Islands e5 the home island's mines have 30 worker
// slots (6 a mine) near the Town Center; the mod's AI worked 10-17 of them all game while 25-41 citizens cut wood (up
// to 1800 wood unused) and 12-17 stood idle from minute 40; the base AI built Settlements and worked 76 slots by the
// end. Mines hold 300000 - a mine near a drop site is income for the whole game.
// Bits:
// 1 FILL: from the Copper Age on (before farms food comes first) with 30+ workers, each metal's target is at least the
//   worker slots of its mines within 12 tiles of a drop site (Town Center, Capitol, Settlement) - with a population cap
//   under 120 only those on the home continent - unless 1500+ of it lie beyond the reserve already (gold piled to 1600
//   unused). The workers come from wood while it piles up (500 beyond its need, down to the wood need's workers, 4 at
//   least), from food while 1000+ lie beyond its reserve (down to 60%), and from the citizens beyond every target; the
//   rebalance (econ.c) moves up to 4 a pass.
// 2 DROP: a Settlement (100 wood) at the home continent's unworked mine nearest to a drop site with none within 12 tiles
//   - every 2 minutes while none is queued or unfinished and the AI can pay for it, 6 a game at most: queued with its
//   spot (3-6 tiles from the mine, placeable, no enemy soldiers within 12) on the build manager's epoch queue.
// 4 FIRST: the slots before wood and food beyond a fifth / a third of the workers each.
// Measured against the base AI (hard, 60 min, 20 seeds x both seats, paired with the shipped build): FILL + DROP alone
// changed next to nothing (the givers are rarely there: wood is spent as fast as it comes, mine gatherers +-0); with
// FIRST (7) the mines got 5-8 more gatherers, kills - losses rose by 7-13 on the islands, but the workers fell by 4-12
// (the wood went short) and Small Islands without the cheat lost more (base AI wins 2 -> 7 of 40). Off (0).
internal int g_mine_fill = 0, g_v_mine_fill = -1;
#define MINE_FILL(idx) PV(idx, g_v_mine_fill, g_mine_fill)
internal struct mf_st {
    uint32_t t, drop_t;
    int slots[7], home_slots[7], g[7], add[7], drops_queued, far_x, far_y, far_r;
} g_mf[16];

internal void mines_new_game(void) { memset(g_mf, 0, sizeof g_mf); }

// the slots of the mines near a drop site (all / the base's continent), and the free far mine of the home continent
// nearest to a drop site (DROP), every 10 s
internal void mf_scan(ee_player *pl, unsigned idx, uint32_t now) {
    struct mf_st *m = &g_mf[idx];
    if (m->t && now >= m->t && now - m->t < 10000) return;
    m->t = now ? now : 1;
    memset(m->slots, 0, sizeof m->slots); memset(m->home_slots, 0, sizeof m->home_slots); memset(m->g, 0, sizeof m->g);
    m->far_r = 0;
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!bm || build_mgr_get_base_x(bm) == -10000) return;
    short hr = (short)map_area(build_mgr_get_base(bm), 2);
    unsigned cap = table_value(0x16);
    drops_cached(pl);
    static ee_unit *seen[1024];
    int ns = 0, bd = 0x7FFFFFFF;
    ee_zone **tb = player_get_zones_begin(pl); ee_zone **te = player_get_zones_end(pl);
    for (int t = 0; tb && tb + t < te && t < 256; t++) {
        ee_zone *zone = tb[t];
        void **sb = zone ? zone_get_sites_begin(zone) : NULL, **se = zone ? zone_get_sites_end(zone) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096; k++) {
            ee_unit *s = sb[k];
            int r = s ? unit_get_resource(s) : 0, j;
            if (r < 4 || r > 6 || unit_get_amount(s) <= 0) continue;
            for (j = 0; j < ns && seen[j] != s; j++) ;
            if (j < ns) continue;
            if (ns < 1024) seen[ns++] = s;
            ivec2 pos = unit_get_tile(s);
            int dd = drop_nearest(idx, pos, 0, NULL), home = (short)map_area(pos, 2) == hr;
            if (dd <= 144) {
                m->slots[r] += (int)cap;
                if (home) m->home_slots[r] += (int)cap;
                m->g[r] += (int)site_gatherers(s);
                continue;
            }
            if (!home || dd >= bd || !zone_get_active(zone)) continue;
            if (!explored_by(s, pl)) continue;
            if (build_mgr_enemy_near(bm, pos.x, pos.y) || site_danger_res(pl, s, 0, r)) continue;
            bd = dd; m->far_x = pos.x; m->far_y = pos.y; m->far_r = r;
        }
    }
}
internal int mf_active(ee_player *pl, uint32_t now) {
    return current_epoch(pl) >= FARM_EPOCH && now >= 3 * 60000 && workforce(pl) >= 30;   // before farms food comes first (Continental e0: fewer workers)
}
// FILL: after the worker targets (hook_compute_targets, after econ_targets)
internal void mine_fill(ee_resource_mgr *rm) {
    ee_player *pl = resource_mgr_player(rm);
    if (!pl) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    uint32_t now = game_time();
    struct mf_st *m = &g_mf[idx];
    if (now < m->t) memset(m, 0, sizeof *m);   // a new game
    memset(m->add, 0, sizeof m->add);
    if (!(MINE_FILL(idx) & 1) || !mf_active(pl, now)) return;
    mf_scan(pl, idx, now);
    uint32_t *target = resource_mgr_target_at(rm);
    int *stock = player_stock_at(pl), *demand = resource_mgr_demand_at(rm), reserve[7];
    budget_reserve(pl, reserve);
    int low = player_get_pop_cap(pl) < 120, gap = 0, want[7] = { 0 };
    for (int r = 4; r < 7; r++) {
        want[r] = low ? m->home_slots[r] : m->slots[r];
        if (stock[r] - reserve[r] > 1500) want[r] = 0;   // piling up already (gold to 1600 unused): not this one
        if ((int)target[r] < want[r]) gap += want[r] - (int)target[r];
    }
    if (gap <= 0) return;
    // the givers: citizens beyond every target, wood piling up, food piling up
    int total = 0, sum = 0;
    for (int r = 0; r < 7; r++) total += rm_count(rm, r);
    for (int r = 2; r < 7; r++) sum += (int)target[r];
    int free_ = total - sum > 0 ? total - sum : 0;
    int wneed = demand[3] > reserve[3] ? demand[3] : reserve[3];
    if (wneed > 3000) wneed = 3000;
    int wood_spare = stock[3] - wneed - 500, food_spare = stock[2] - reserve[2] - 1000;
    int wfloor = 4, ffloor = (int)target[2] * 3 / 5;
    int taken = free_ < gap ? free_ : gap;
    if (wood_spare > 0) {
        float pw = g_rate[idx].per_worker[3];
        int keep = (int)((float)(wneed > stock[3] ? wneed - stock[3] : 0) / (2.0f * (pw > 2.0f ? pw : 2.0f))) + 2;
        if (keep > wfloor) wfloor = keep;
        while (taken < gap && (int)target[3] > wfloor) { target[3]--; taken++; }
    }
    if (food_spare > 0) while (taken < gap && (int)target[2] > ffloor) { target[2]--; taken++; }
    if (MINE_FILL(idx) & 4) {   // FIRST: the slots before wood and food beyond a fifth / a third of the workers each
        int wf = total / 5 > 4 ? total / 5 : 4, ff = total / 3 > 6 ? total / 3 : 6;
        while (taken < gap && ((int)target[3] > wf || (int)target[2] > ff)) {
            if ((int)target[3] - wf >= (int)target[2] - ff) target[3]--; else target[2]--;
            taken++;
        }
    }
    for (int r = 4; r < 7 && taken > 0; r++) {   // the metals by their gap, stone first (cheapest walls and towers come later)
        int g = want[r] - (int)target[r];
        if (g <= 0) continue;
        if (g > taken) g = taken;
        target[r] += (uint32_t)g; m->add[r] += g; taken -= g;
    }
}
// pl's build manager when a Settlement can be queued now: none queued (queued: those count too), none going up, it
// can build and pay for one; *stuck (may be NULL) the one going up that stops it
internal ee_build_mgr *settle_free(ee_player *pl, int queued, ee_unit **stuck) {
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    void *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    if (!bm || !um || !ut_has(UT(B_SETTLEMENT)) || (queued && (bq_has(bm, UT(B_SETTLEMENT)) || bq_has(bm, UT(X_SINGLE_PLAYER_SETTLEMENT))))) return NULL;
    uint8_t *q = type_list(um, UT(B_SETTLEMENT));   // one being built: wait for it
    for (unsigned k = 0, f = q ? deque_front(q) : 0, n = q ? deque_count(q) : 0; k < n; k++) {
        ee_unit *b = deque_at(q, f + k), *st = b ? unit_get_physics(b) : NULL;
        if (b && unit_get_hp(b) > 0 && (!st || !unit_get_finished(st))) { if (stuck) *stuck = b; return NULL; }
    }
    return can_build(pl, UT(B_SETTLEMENT)) ? bm : NULL;   // can't build or pay now: NULL
}

// DROP: every 2 minutes (econ_tick)
internal void mine_drops(ee_player *pl, unsigned idx, uint32_t now) {
    struct mf_st *m = &g_mf[idx];
    if (!((MINE_FILL(idx) & 2) || (PV(idx, g_v_cit_plan, g_cit_plan) & 4)) || !mf_active(pl, now) || m->drops_queued >= 6) return;   // citizen_plan DROP too
    if (m->drop_t && now >= m->drop_t && now - m->drop_t < 120000) return;
    m->drop_t = now ? now : 1;
    mf_scan(pl, idx, now);
    if (!m->far_r) return;
    ee_build_mgr *bm = settle_free(pl, 1, NULL);
    if (!bm) return;
    int e = current_epoch(pl);
    if (e < 0 || e >= 14) return;
    for (int r = 3; r <= 6; r++)
        for (int ax = -r; ax <= r; ax++)
            for (int ay = -r; ay <= r; ay++) {
                if (abs(ax) != r && abs(ay) != r) continue;
                int x = m->far_x + ax, y = m->far_y + ay;
                if (!can_place(pl, UT(B_SETTLEMENT), iv2(x, y)) || raiders_near(pl, iv2(x, y), 12)) continue;
                int entry[3] = { UT(B_SETTLEMENT), x, y };
                build_mgr_queue_add(bm, e, entry);
                m->drops_queued++;
                if (g_log) {
                    EnterCriticalSection(&g_lock);
                    fprintf(g_log, "# mines: P%u at %.1f min: a Settlement queued at %d,%d for the mine (%d) at %d,%d\n", idx, now / 60000.0,
                            x, y, m->far_r, m->far_x, m->far_y);
                    LeaveCriticalSection(&g_lock);
                }
                return;
            }
}

// WOOD DROP ([ai_base] wood_drop, default on - the player's call, not measured): nothing in the game or the mod put a
// drop site by the forest its woodcutters work. The player's game (2026-10-28, minute 700): P7's 135 woodcutters cut a
// forest 54 tiles from its nearest drop site, 1.1 wood per worker-minute against 5-9 for the other resources. Every
// minute, while no Settlement is going up and the AI can pay for one: the woodcutters whose tree lies 15+ tiles from a
// drop site, the tree with most of them within 8 tiles; 8 or more there - a Settlement laid at the placeable spot
// nearest the middle of their trees (2-12 tiles, the forest's edge), no enemy building or soldiers near. Such a forest
// found and the Settlement not paid: its cost held back from training for 2 minutes (wood_reserve) - P7's wood was
// spent as it came (65 in stock at minute 700). On a copy of that game P7 laid one at 701.4: the woodcutters' walk
// 53 -> 6-9 tiles, 0.3-2 -> 4-9 wood per worker-minute.
internal int g_wood_drop = 1;
internal uint32_t g_wd_want[16];   // a forest waits for its Settlement: since
internal void wood_reserve(ee_player *pl, int reserve[7]) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    uint32_t now = game_time();
    if (!g_wd_want[idx] || now < g_wd_want[idx] || now - g_wd_want[idx] > 120000) return;
    for (int r = 2; r < 7; r++) reserve[r] += unit_cost(pl, UT(B_SETTLEMENT), r);
}
internal void wood_drops(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_wood_drop || (last[idx] && now >= last[idx] && now - last[idx] < 60000)) return;
    last[idx] = now ? now : 1;
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    ee_resource_mgr *rm = cm ? civ_mgr_get_resource_mgr(cm) : NULL;
    if (!rm) return;
    drops_cached(pl);
    if (!g_drops[idx].n) return;
    static ivec2 fs[1024];
    int nf = 0;
    uint8_t *dq = resource_mgr_list_at(rm, 3);
    for (unsigned k = 0, f = deque_front(dq), n = deque_count(dq); k < n && nf < 1024; k++) {
        ee_unit *u = deque_at(dq, f + k); ee_goal *goal = u ? unit_get_goal(u) : NULL;
        ee_unit *site = goal && !strcmp(rtti_name(goal), "EEUGGather@@") ? goal_get_site(goal) : NULL;
        if (!site) continue;
        ivec2 p = unit_get_tile(site);
        if (drop_nearest(idx, p, 0, NULL) >= 15 * 15) fs[nf++] = p;
    }
    int best = -1, bn = 0;
    for (int i = 0; i < nf; i++) {
        int c = 0;
        for (int j = 0; j < nf; j++) c += ivec2_dist2(fs[i], fs[j]) <= 8 * 8;
        if (c > bn) { bn = c; best = i; }
    }
    g_wd_want[idx] = 0;
    if (bn < 8) return;
    int sx = 0, sy = 0;
    for (int j = 0; j < nf; j++) if (ivec2_dist2(fs[best], fs[j]) <= 8 * 8) { sx += fs[j].x; sy += fs[j].y; }
    ivec2 mid = iv2(sx / bn, sy / bn);
    ee_civ_mgr *cm2 = player_get_civ_mgr(pl);
    ee_build_mgr *any = cm2 ? civ_mgr_get_build_mgr(cm2) : NULL;
    int e = current_epoch(pl);
    if (!any || e < 0 || e >= 14) return;
    static uint32_t told[16];   // why not: every 5 minutes
    const char *why = NULL;
    ee_build_mgr *bm = NULL;
    ee_unit *stuck = NULL;
    if (build_mgr_enemy_near(any, mid.x, mid.y)) why = "an enemy near";
    else if (!(bm = settle_free(pl, 0, &stuck))) {   // the queue's own Settlements don't count: P7's held the pile's 60 entries
        why = stuck ? "a Settlement going up" : "can't build one";
        if (ut_has(UT(B_SETTLEMENT)) && !can_build(pl, UT(B_SETTLEMENT))) { g_wd_want[idx] = now ? now : 1; why = "can't pay - held back"; }   // the wood held back from now on
    }
    if (why && g_log && (!told[idx] || now < told[idx] || now - told[idx] >= 300000)) {
        told[idx] = now ? now : 1;
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# wood: P%u at %.1f min: %d woodcutters 15+ tiles from a drop site, %d of them at the trees by %d,%d - waits: %s\n",
                idx, now / 60000.0, nf, bn, mid.x, mid.y, why);
        if (stuck) fprintf(g_log, "# wood: P%u: the Settlement going up at %d,%d (hp %d)\n", idx, unit_get_x(stuck), unit_get_y(stuck), unit_get_hp(stuck));
        LeaveCriticalSection(&g_lock);
    }
    if (why) return;
    for (int r = 2; r <= 12; r++)
        for (int ax = -r; ax <= r; ax++)
            for (int ay = -r; ay <= r; ay++) {
                if (abs(ax) != r && abs(ay) != r) continue;
                ivec2 p = iv2(mid.x + ax, mid.y + ay);
                if (!can_place(pl, UT(B_SETTLEMENT), p) || raiders_near(pl, p, 12)) continue;
                // laid at once by the build manager's placement (a builder, the foundation): an entry on its queue
                // vanished within seconds for P6 (epoch 11) without ever reaching the placement
                char ok = hook_place(bm, UT(B_SETTLEMENT), p.x, p.y);
                if (g_log) {
                    EnterCriticalSection(&g_lock);
                    fprintf(g_log, "# wood: P%u at %.1f min: %d woodcutters 15+ tiles from a drop site, %d of them at the trees by %d,%d (the "
                            "nearest drop site %d tiles) - a Settlement %s at %d,%d\n", idx, now / 60000.0, nf, bn, mid.x, mid.y,
                            (int)sqrtf((float)drop_nearest(idx, mid, 0, NULL)), ok ? "laid" : "not placed", p.x, p.y);
                    LeaveCriticalSection(&g_lock);
                }
                return;
            }
}
internal void mines_log(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_log || !g_autotest || (!g_mine_fill && g_v_mine_fill < 0)) return;
    if (now / 300000 == last[idx] / 300000 && last[idx]) return;
    last[idx] = now ? now : 1;
    struct mf_st *m = &g_mf[idx];
    mf_scan(pl, idx, now);
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# mines: P%u at %u min: fill %d, slots S%d G%d I%d (home S%d G%d I%d), gatherers S%d G%d I%d, added S%d G%d I%d, "
            "settlements queued %d\n", idx, now / 60000, MINE_FILL(idx), m->slots[4], m->slots[5], m->slots[6], m->home_slots[4],
            m->home_slots[5], m->home_slots[6], m->g[4], m->g[5], m->g[6], m->add[4], m->add[5], m->add[6], m->drops_queued);
    LeaveCriticalSection(&g_lock);
}
internal void mines_install(const char *autotest_ini) {
    g_mine_fill = g_set_ai[1].mine_fill;
    g_wood_drop = g_set_ai[1].wood_drop;
    if (autotest_ini) g_v_mine_fill = ini_int("autotest", "v_mine_fill", -1, autotest_ini);
}
