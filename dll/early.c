// EARLY GAME: the playtester's opening (docs/economy.md). Included by eeaimod.c after bunker.c.
// Against the base game's AI (hard, cheating) the mod's AI won 0 of 158: on Continental it trained no soldier until
// minute ~22, 1-6 Clubmen and Rock Throwers killed 39-48 of its citizens at the town, its citizens were sent against
// them as militia 40-63 times a game and lost, and its towers were queued but stood unfinished. The playtester:
// "I don't train military in the first epoch and rely on towers and citizens, but I go heavy on bowmen/rock slingers
// from the second epoch on. My first-age tactic is getting towers up and baiting enemy troops under the towers with my
// citizens - it needs active management."
//
// TOWERS FIRST ([all] early_towers): in the first two epochs the town's first EARLY_CORE towers stand by the Town
// Center (the base the AI plans around), EARLY_DIST tiles out - one towards the front (the defence layout's direction:
// the nearest enemy seen, else the map centre), one on the side where the citizens work - so the town and the berries
// beside it are covered (the defence layout put the game's first requests 10 tiles out, at the front). The game's own
// tower requests take these spots first (defence_spot); missing ones are queued by the mod every 10 s while stone
// allows. Every 10 s each unfinished tower within 20 tiles of the base gets 2 builders (bk_builders: the nearest
// gatherers) unless enemy soldiers stand within 8 tiles. Before farms the worker plan asks for the missing towers'
// stone (econ_targets).
// THE BAIT ([all] tower_bait): up to the Bronze Age, every 2 s - a citizen with an enemy soldier within 5 tiles (or one
// attacking it) and outside its own towers' reach walks to the nearest finished tower (or other own building that
// shoots: its range, defence_stats) within 25 tiles, to the spot 1.5 tiles behind it from the enemy: the raiders that
// follow come under the tower. Once it is there the order ends and the assignment sends it back to work (the searches
// refuse raided sites - sites.c - so work elsewhere until the raiders are gone). Citizens the militia sent at the enemy
// are left alone.
// The first army (early_army, early_soldiers), the militia (early_militia) and the economy (copper_plan=2): econ.c
// train_budget / substitute_soldier / econ_targets, defence.c home_guard / militia_answer.
#define EARLY_CORE 2
#define EARLY_DIST 4
static struct early_st {
    uint32_t tower_t, bait_t, log_t, range_t;
    struct { int x, y; uint32_t t; } pend[EARLY_CORE];   // core spots handed out recently (the foundation may not show yet)
    unsigned queued, placed, builders, baits, ranges;
    float range;   // the first tower type's range (log)
} g_early[16];

static void early_new_game(void) {
    memset(g_early, 0, sizeof g_early);
    memset(g_early_mil_refused, 0, sizeof g_early_mil_refused);
}

static int early_tower_type(void *pl) { return tower_newest(pl); }   // the best tower it can build now (as defence_tick)
static int early_base(void *pl, int *bx, int *by) {
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
    if (!bm || FIELD(bm, 0x12C, int) == -10000) return 0;
    *bx = FIELD(bm, 0x12C, int); *by = FIELD(bm, 0x130, int);
    return 1;
}
static int early_finished(uint8_t *u) { uint8_t *bs = FIELD(u, 0x30, uint8_t *); return bs && FIELD(bs, 0x340, uint8_t); }
// own towers within r of x, y: finished / all (foundations too)
static void early_towers_at(void *pl, int x, int y, int r, int *fin, int *all) {
    *fin = *all = 0;
    for (struct grid_it it = grid_it_box(x, y, r, 1); grid_it_next(&it); ) {
        uint8_t *u = it.u, *td = it.td;
        if (!td || FIELD(u, 0x18, void *) != pl || FIELD(u, 0x3C, int) <= 0 || !is_tower_type(FIELD(td, 0x4B8, int))) continue;
        if (abs(FIELD(u, 0x1C, int) - x) > r || abs(FIELD(u, 0x20, int) - y) > r) continue;
        (*all)++;
        *fin += early_finished(u);
    }
}
static int early_core_on(void *pl) {
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    return EARLY(idx, early_towers) && current_epoch(pl) <= 1 && !is_vanilla(pl);
}
// core slot k: towards the front (0), on the citizens' side across it (1)
static void early_slot(void *pl, unsigned idx, int bx, int by, int k, int *x, int *y) {
    uint32_t now = game_time();
    def_refresh(pl, idx, now);
    struct def_plan *p = &g_plan[idx];
    float dx, dy;
    if (p->nf) { dx = p->f[0].dx; dy = p->f[0].dy; }
    else {
        uint8_t *map = *(uint8_t **)GA(G_MAP);
        int n = map ? (int)FIELD(map, 0x1816FC, uint32_t) : 0;
        dx = (float)(n / 2 - bx); dy = (float)(n / 2 - by);
        float l = sqrtf(dx * dx + dy * dy);
        if (l < 1) { dx = 1; dy = 0; } else { dx /= l; dy /= l; }
    }
    if (k == 1) {   // across the front, on the side of the citizens (their centroid within 15 tiles)
        float sx = 0, sy = 0;
        for (struct grid_it it = grid_it_box(bx, by, 15, 1); grid_it_next(&it); ) {
            uint8_t *u = it.u, *td = it.td;
            if (!td || FIELD(u, 0x18, void *) != pl || FIELD(u, 0x3C, int) <= 0 || !is_citizen_type(FIELD(td, 0x4B8, int))) continue;
            sx += (float)(FIELD(u, 0x1C, int) - bx); sy += (float)(FIELD(u, 0x20, int) - by);
        }
        float px = -dy, py = dx;
        if (px * sx + py * sy < 0) { px = -px; py = -py; }
        dx = px; dy = py;
    }
    *x = bx + (int)lroundf(dx * EARLY_DIST);
    *y = by + (int)lroundf(dy * EARLY_DIST);
}
// a spot for a tower of the town's core (defence_spot, the queueing below): 0 when the core stands or none fits
static int early_core_spot(void *pl, int type, int *ox, int *oy) {
    if (!pl || !early_core_on(pl)) return 0;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    int bx, by;
    if (!early_base(pl, &bx, &by)) return 0;
    uint32_t now = game_time();
    struct early_st *st = &g_early[idx];
    for (int k = 0; k < EARLY_CORE; k++) {
        int x, y, fin, all;
        early_slot(pl, idx, bx, by, k, &x, &y);
        early_towers_at(pl, x, y, 2, &fin, &all);
        if (all) continue;
        if (st->pend[k].t && now >= st->pend[k].t && now - st->pend[k].t < 90000) continue;
        for (int r = 0; r <= 2; r++)
            for (int ax = -r; ax <= r; ax++)
                for (int ay = -r; ay <= r; ay++) {
                    if ((abs(ax) != r && abs(ay) != r) || raiders_near(pl, x + ax, y + ay, 10) || !can_place(pl, type, x + ax, y + ay))
                        continue;
                    *ox = x + ax; *oy = y + ay;
                    st->pend[k].x = *ox; st->pend[k].y = *oy; st->pend[k].t = now ? now : 1;
                    st->placed++;
                    if (g_log) fprintf(g_log, "# early: P%u tower %d at %d,%d (core %d; base %d,%d)\n", idx, type, *ox, *oy, k, bx, by);
                    return 1;
                }
    }
    return 0;
}
// the stone the core's missing towers still need (econ_targets, before farms)
static int early_tower_stone(void *pl) {
    if (!pl || !early_core_on(pl)) return 0;
    int bx, by, fin, all, type = early_tower_type(pl);
    if (!type || !early_base(pl, &bx, &by)) return 0;
    early_towers_at(pl, bx, by, EARLY_DIST + 3, &fin, &all);
    return all >= EARLY_CORE ? 0 : (EARLY_CORE - all) * unit_cost(pl, type, 4);
}
static void early_towers_tick(void *pl, unsigned idx, uint32_t now) {
    struct early_st *st = &g_early[idx];
    if (!early_core_on(pl) || (st->tower_t && now >= st->tower_t && now - st->tower_t < 10000)) return;
    st->tower_t = now ? now : 1;
    int bx, by;
    if (!early_base(pl, &bx, &by)) return;
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
    int *stock = (int *)((uint8_t *)pl + PL_STOCK), type = early_tower_type(pl), e = current_epoch(pl);
    if (type && !st->range) { uint8_t *td = type_data(type); st->range = td ? FIELD(td, 0x420, float) : 0; }
    int fin, all, x, y;
    early_towers_at(pl, bx, by, EARLY_DIST + 3, &fin, &all);
    if (!bm) return;
    int queued = tower_queued(bm);
    if (bm && type && all < EARLY_CORE && !queued && stock[4] >= unit_cost(pl, type, 4) && e >= 0 && e < 14 &&
        early_core_spot(pl, type, &x, &y)) {
        int entry[3] = { type, x, y };
        GF(FN_BQ_ADD)(bm + 4 + e * 0x14, entry);
        st->queued++;
        if (g_log) fprintf(g_log, "# early: P%u queues tower %d at %d,%d (stone %d)\n", idx, type, x, y, stock[4]);
    }
    // builders for every unfinished tower near the town (the game's own and the defence layout's too)
    uint8_t *todo[8];
    int nt = 0;
    for (struct grid_it it = grid_it_box(bx, by, 20, 1); grid_it_next(&it) && nt < 8; ) {
        uint8_t *u = it.u, *td = it.td;
        if (!td || FIELD(u, 0x18, void *) != pl || FIELD(u, 0x3C, int) <= 0 || !is_tower_type(FIELD(td, 0x4B8, int)) ||
            early_finished(u)) continue;
        todo[nt++] = u;
    }
    for (int i = 0; i < nt; i++)
        if (!raiders_near(pl, FIELD(todo[i], 0x1C, int), FIELD(todo[i], 0x20, int), 8)) st->builders += (unsigned)bk_builders(pl, todo[i], 2);
}

// own buildings that shoot (finished) near x, y: 1 when one reaches x, y (its range - 0.5)
static int early_cover(void *pl, int x, int y) {
    for (struct grid_it it = grid_it_box(x, y, 16, 1); grid_it_next(&it); ) {
        uint8_t *u = it.u;
        float r, a;
        if (FIELD(u, 0x18, void *) != pl || FIELD(u, 0x3C, int) <= 0 || !defence_stats(u, &r, &a) || !early_finished(u)) continue;
        float dx = (float)(FIELD(u, 0x1C, int) - x), dy = (float)(FIELD(u, 0x20, int) - y);
        if (dx * dx + dy * dy <= (r - 0.5f) * (r - 0.5f)) return 1;
    }
    return 0;
}
static void early_bait(void *pl, unsigned idx, uint32_t now) {
    struct early_st *st = &g_early[idx];
    if (!EARLY(idx, tower_bait) || current_epoch(pl) > 3 || (st->bait_t && now >= st->bait_t && now - st->bait_t < 2000)) return;
    st->bait_t = now ? now : 1;
    int bx, by;
    if (!early_base(pl, &bx, &by)) return;
    static struct { int x, y; float r; } cov[24];
    static struct { uint8_t *u, *t; int x, y; } en[64];
    int nc = 0, ne = 0;
    for (struct grid_it it = grid_it_box(bx, by, 36, 1); grid_it_next(&it); ) {
        uint8_t *u = it.u, *td = it.td;
        if (!td || FIELD(u, 0x3C, int) <= 0) continue;
        void *o = FIELD(u, 0x18, void *);
        float r, a;
        if (o == pl) {
            if (nc < 24 && defence_stats(u, &r, &a) && early_finished(u)) { cov[nc].x = FIELD(u, 0x1C, int); cov[nc].y = FIELD(u, 0x20, int); cov[nc++].r = r; }
        } else if (ne < 64 && o && FIELD(o, PL_INDEX, int) > 0 && is_soldier(td) && is_enemy(pl, o) && seen_by(u, pl)) {
            uint8_t *g = FIELD(u, 0x1EC, uint8_t *);
            en[ne].u = u; en[ne].x = FIELD(u, 0x1C, int); en[ne].y = FIELD(u, 0x20, int);
            en[ne++].t = g && *(uintptr_t *)g == GA(VT_GOAL_ATTACK) ? FIELD(g, 0x30, uint8_t *) : NULL;
        }
    }
    if (!nc || !ne) return;
    int sent = 0;
    for (struct grid_it it = grid_it_box(bx, by, 32, 1); grid_it_next(&it); ) {
        uint8_t *u = it.u, *td = it.td;
        if (!td || FIELD(u, 0x18, void *) != pl || FIELD(u, 0x3C, int) <= 0 || !is_citizen_type(FIELD(td, 0x4B8, int)) ||
            FIELD(u, 0x33B, uint8_t) || FIELD(u, 0x1C, int) == -10000) continue;
        int ux = FIELD(u, 0x1C, int), uy = FIELD(u, 0x20, int), hit = -1;
        for (int k = 0; k < ne && hit < 0; k++)
            if (en[k].t == u || (abs(en[k].x - ux) <= 5 && abs(en[k].y - uy) <= 5)) hit = k;
        if (hit < 0) continue;
        int best = -1;
        float bd = 25.0f * 25.0f;
        for (int c = 0; c < nc; c++) {
            float dx = (float)(cov[c].x - ux), dy = (float)(cov[c].y - uy), d = dx * dx + dy * dy;
            if (d <= (cov[c].r - 0.5f) * (cov[c].r - 0.5f)) { best = -2; break; }   // covered already: the tower fights
            if (d < bd) { bd = d; best = c; }
        }
        if (best < 0) continue;
        uint8_t *g = FIELD(u, 0x1EC, uint8_t *);
        if (g && *(uintptr_t *)g == GA(VT_GOAL_ATTACK)) continue;   // the militia's: a fight it wins
        float ex = (float)(cov[best].x - en[hit].x), ey = (float)(cov[best].y - en[hit].y), l = sqrtf(ex * ex + ey * ey);
        float sx = (float)cov[best].x + 0.5f + (l > 0.5f ? ex / l * 1.5f : 0), sy = (float)cov[best].y + 0.5f + (l > 0.5f ? ey / l * 1.5f : 0);
        if (g && *(uintptr_t *)g == GA(VT_GOAL_LOCATION) && fabsf(FIELD(g, 0x20, float) - sx) < 2 && fabsf(FIELD(g, 0x24, float) - sy) < 2)
            continue;   // on its way there
        if (unit_goal(u, goal_move(sx, sy), 0, 0x1F40)) sent++;
    }
    st->baits += (unsigned)sent;
}

// THE RANGE (early_army): the food-free soldiers of the second epoch (Slinger) come from the Archery Range (54), which the
// game's build list brought late or not at all - the army asked for Rock Throwers and Clubmen (food) and got 0-3
// soldiers in the Stone Age. From epoch 1 to 3, every 60 s: none standing or going up and none in the build manager's
// lists (0x465640) -> one requested (0x46802D(bm, Archery Range, 1, 0), as the building plan's entries do).
static void early_range(void *pl, unsigned idx, uint32_t now) {
    struct early_st *st = &g_early[idx];
    int e = current_epoch(pl);
    if (!EARLY(idx, early_army) || e < 1 || e > 3 || (st->range_t && now >= st->range_t && now - st->range_t < 60000)) return;
    st->range_t = now ? now : 1;
    void *cm = FIELD(pl, 0xB90, void *), *um = cm ? FIELD(cm, 0x44, void *) : NULL;
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
    if (!um || !bm) return;
    uint8_t *q = type_list(um, UT(B_ARCHERY_RANGE));
    for (unsigned k = 0, f = q ? FIELD(q, 0xC, unsigned) : 0, n = q ? FIELD(q, 0x10, unsigned) : 0; k < n; k++) {
        uint8_t *b = deque_at(q, f + k);
        if (b && FIELD(b, 0x3C, int) > 0) return;
    }
    if (!ut_has(UT(B_ARCHERY_RANGE)) || GF(FN_BM_QUEUED)(bm, UT(B_ARCHERY_RANGE))) return;
    GF(FN_BM_REQUEST)(bm, UT(B_ARCHERY_RANGE), 1, 0);
    st->ranges++;
    if (g_log) fprintf(g_log, "# early: P%u at %.1f min: no Archery Range - one requested\n", idx, now / 60000.0);
}

static void early_tick(void *pl, unsigned idx, uint32_t now) {
    if (is_vanilla(pl)) return;
    early_towers_tick(pl, idx, now);
    early_bait(pl, idx, now);
    early_range(pl, idx, now);
    struct early_st *st = &g_early[idx];
    if (!g_auto.enabled || !g_log || (st->log_t && now >= st->log_t && now / 60000 == st->log_t / 60000)) return;
    st->log_t = now ? now : 1;
    if (!EARLY(idx, early_towers) && !EARLY(idx, tower_bait) && !EARLY(idx, early_militia) && !EARLY(idx, early_army) &&
        copper_mode(pl) != 2) return;
    int bx = 0, by = 0, cf = 0, ca = 0, tf = 0, ta = 0;
    if (early_base(pl, &bx, &by)) { early_towers_at(pl, bx, by, EARLY_DIST + 3, &cf, &ca); early_towers_at(pl, bx, by, 20, &tf, &ta); }
    int *stock = (int *)((uint8_t *)pl + PL_STOCK);
    fprintf(g_log, "# early: P%u at %u min: epoch %d, core towers %d/%d, towers %d/%d (finished/all), range %.1f, stone %d, "
            "queued %u, placed %u, builders %u, baits %u, militia refused %u, soldiers %d, account F%.0f W%.0f G%.0f I%.0f, epoch account F%.0f G%.0f I%.0f, food %d\n",
            idx, now / 60000, current_epoch(pl), cf, ca, tf, ta, st->range, stock[4], st->queued, st->placed, st->builders,
            st->baits, g_early_mil_refused[idx], army_size(pl), g_bud[idx].mil[2], g_bud[idx].mil[3], g_bud[idx].mil[5],
            g_bud[idx].mil[6], g_epoch_acc[idx][2], g_epoch_acc[idx][5], g_epoch_acc[idx][6], stock[2]);
}
