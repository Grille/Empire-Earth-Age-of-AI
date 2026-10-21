// FILL THE MINES ([all] mine_fill, bits; [autotest] v_mine_fill for the variant players). Included by eeaimod.c after
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
static int g_mine_fill = 0, g_v_mine_fill = -1;
#define MINE_FILL(idx) PV(idx, g_v_mine_fill, g_mine_fill)
static struct mf_st {
    uint32_t t, drop_t;
    int slots[7], home_slots[7], g[7], add[7], drops_queued, far_x, far_y, far_r;
} g_mf[16];

static void mines_new_game(void) { memset(g_mf, 0, sizeof g_mf); }

// the slots of the mines near a drop site (all / the base's continent), and the free far mine of the home continent
// nearest to a drop site (DROP), every 10 s
static void mf_scan(void *pl, unsigned idx, uint32_t now) {
    struct mf_st *m = &g_mf[idx];
    if (m->t && now >= m->t && now - m->t < 10000) return;
    m->t = now ? now : 1;
    memset(m->slots, 0, sizeof m->slots); memset(m->home_slots, 0, sizeof m->home_slots); memset(m->g, 0, sizeof m->g);
    m->far_r = 0;
    uint8_t *cm = FIELD(pl, OF(PL, 0xB90), uint8_t *), *bm = cm ? FIELD(cm, OF(CM, 0x3C), uint8_t *) : NULL;
    if (!bm || FIELD(bm, OF(BM, 0x12C), int) == -10000) return;
    int base[2] = { FIELD(bm, OF(BM, 0x12C), int), FIELD(bm, OF(BM, 0x130), int) };
    short (__thiscall *region)(void *, int *, int) = (void *)GA(FN_REGION);
    short hr = region((void *)GA(G_MAP_AREAS), base, 2);
    unsigned cap = GF(FN_TABLE_VALUE)((void *)GA(G_TABLE_VALUES), 0x16);
    drops_cached(pl);
    static uint8_t *seen[1024];
    int ns = 0, bd = 0x7FFFFFFF;
    void **tb = FIELD(pl, OF(PL, 0xB50), void **), **te = FIELD(pl, OF(PL, 0xB54), void **);
    for (int t = 0; tb && tb + t < te && t < 256; t++) {
        uint8_t *zone = tb[t];
        void **sb = zone ? FIELD(zone, OF(ZONE, 0x8), void **) : NULL, **se = zone ? FIELD(zone, OF(ZONE, 0xC), void **) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096; k++) {
            uint8_t *s = sb[k];
            int r = s ? FIELD(s, OF(UNIT, 0x38), int) : 0, j;
            if (r < 4 || r > 6 || FIELD(s, OF(UNIT, 0x40), int) <= 0) continue;
            for (j = 0; j < ns && seen[j] != s; j++) ;
            if (j < ns) continue;
            if (ns < 1024) seen[ns++] = s;
            int pos[2] = { FIELD(s, OF(UNIT, 0x1C), int), FIELD(s, OF(UNIT, 0x20), int) }, dd = 0x7FFFFFFF;
            for (int i = 0; i < g_drops[idx].n; i++) {
                int *d = g_drops[idx].d[i], dx = d[0] - pos[0], dy = d[1] - pos[1];
                if (!d[2] && dx * dx + dy * dy < dd) dd = dx * dx + dy * dy;
            }
            int home = region((void *)GA(G_MAP_AREAS), pos, 2) == hr;
            if (dd <= 144) {
                m->slots[r] += (int)cap;
                if (home) m->home_slots[r] += (int)cap;
                m->g[r] += (int)GF(FN_SITE_GATHERERS)((void *)GA(G_GAME_STATE), s);
                continue;
            }
            if (!home || dd >= bd || !zone[OF(ZONE, 0x18)]) continue;
            if (!GF(FN_EXPLORED)(FIELD(s, OF(UNIT, 0x14), void *), pl)) continue;
            if (((EnemyNearFn)GA(FN_ENEMY_NEAR))(bm, pos[0], pos[1]) || site_danger_res(pl, s, 0, r)) continue;
            bd = dd; m->far_x = pos[0]; m->far_y = pos[1]; m->far_r = r;
        }
    }
}
static int mf_active(void *pl, uint32_t now) {
    return current_epoch(pl) >= FARM_EPOCH && now >= 3 * 60000 && workforce(pl) >= 30;   // before farms food comes first (Continental e0: fewer workers)
}
// FILL: after the worker targets (hook_compute_targets, after econ_targets)
static void mine_fill(ee_resource_mgr *rm) {
    void *pl = rm_player(rm);
    if (!pl) return;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    uint32_t now = game_time();
    struct mf_st *m = &g_mf[idx];
    if (now < m->t) memset(m, 0, sizeof *m);   // a new game
    memset(m->add, 0, sizeof m->add);
    if (!(MINE_FILL(idx) & 1) || !mf_active(pl, now)) return;
    mf_scan(pl, idx, now);
    uint32_t *target = (uint32_t *)FIELD_AT(rm, RM_TARGET);
    int *stock = (int *)((uint8_t *)pl + PL_STOCK), *demand = (int *)FIELD_AT(rm, RM_DEMAND), reserve[7];
    budget_reserve(pl, reserve);
    int low = FIELD(pl, OF(PL, 0xCCC), int) < 120, gap = 0, want[7] = { 0 };
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
// DROP: every 2 minutes (econ_tick)
static void mine_drops(void *pl, unsigned idx, uint32_t now) {
    struct mf_st *m = &g_mf[idx];
    if (!((MINE_FILL(idx) & 2) || (PV(idx, g_v_cit_plan, g_cit_plan) & 4)) || !mf_active(pl, now) || m->drops_queued >= 6) return;   // citizen_plan DROP too
    if (m->drop_t && now >= m->drop_t && now - m->drop_t < 120000) return;
    m->drop_t = now ? now : 1;
    mf_scan(pl, idx, now);
    if (!m->far_r) return;
    uint8_t *cm = FIELD(pl, OF(PL, 0xB90), uint8_t *), *bm = cm ? FIELD(cm, OF(CM, 0x3C), uint8_t *) : NULL;
    void *um = cm ? FIELD(cm, OF(CM, 0x44), void *) : NULL;
    if (!bm || !um || !ut_has(UT(B_SETTLEMENT)) || bq_has(bm, UT(B_SETTLEMENT)) || bq_has(bm, UT(X_SINGLE_PLAYER_SETTLEMENT))) return;
    uint8_t *q = type_list(um, UT(B_SETTLEMENT));   // one being built: wait for it
    for (unsigned k = 0, f = q ? FIELD(q, 0xC, unsigned) : 0, n = q ? FIELD(q, 0x10, unsigned) : 0; k < n; k++) {
        uint8_t *b = deque_at(q, f + k), *st = b ? FIELD(b, OF(UNIT, 0x30), uint8_t *) : NULL;
        if (b && FIELD(b, OF(UNIT, 0x3C), int) > 0 && (!st || !FIELD(st, OF(UNIT, 0x340), uint8_t))) return;
    }
    if (!GF(FN_CAN_BUILD)(pl, UT(B_SETTLEMENT), 0x20, 0)) return;   // can't build or pay now
    int e = current_epoch(pl);
    if (e < 0 || e >= 14) return;
    for (int r = 3; r <= 6; r++)
        for (int ax = -r; ax <= r; ax++)
            for (int ay = -r; ay <= r; ay++) {
                if (abs(ax) != r && abs(ay) != r) continue;
                int x = m->far_x + ax, y = m->far_y + ay;
                if (!can_place(pl, UT(B_SETTLEMENT), x, y) || raiders_near(pl, x, y, 12)) continue;
                int entry[3] = { UT(B_SETTLEMENT), x, y };
                GF(FN_BQ_ADD)(bm + 4 + e * 0x14, entry);
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
static void mines_log(void *pl, unsigned idx, uint32_t now) {
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
static void mines_install(const char *aimod_ini, const char *autotest_ini) {
    g_mine_fill = cfg_int(aimod_ini, autotest_ini, 1, "mine_fill", 0);
    if (autotest_ini) g_v_mine_fill = ini_int("autotest", "v_mine_fill", -1, autotest_ini);
}
