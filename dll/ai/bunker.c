// BUNKERS: the Fortress (type 672, family 29, tech 3072 - Copper Age) holds units like a transport (type +0x54C
// capacity, 40): inside they are off the map, safe, and (the playtester) don't count toward the population limit. The
// game's AI never builds or uses it. The playtester's uses: citizens out of bombing runs, a refuge for an outpost's
// gatherers until an army comes, a reserve at low population limits that replaces losses at once. A garrison is
// something else (the playtester): citizens put into a building for good, to upgrade it (type +0x550, its most: Town
// Center 15, Capitol 50, Settlement 5, Granary 8; the trigger attribute Garrison Level) - no shelter. Included by eeaimod.c
// after landing.c (the load goal as the transports': EEUGLoadTransport 0x61487F on the holder, its part for each unit).

#define FORTRESS_TYPE UT(B_FORTRESS)
static int bk_capacity(uint8_t *u) {   // what a building holds as a transport does, 0 = none
    uint8_t *td = u ? FIELD(u, OF(UNIT, 0x2C), uint8_t *) : NULL;
    return td && FIELD(td, OF(TD, 0x4B4), int) == 29 ? FIELD(td, OF(TD, 0x54C), int) : 0;
}
static int bk_held(uint8_t *u) { return GF(FN_UNITS_INSIDE)(u); }   // units inside

struct bk_scan { void *pl; int x, y, r, nf, nc; uint8_t *f[8], *c[128]; };
static void bk_unit(uint8_t *u, void *ctx) {
    struct bk_scan *b = ctx;
    uint8_t *td = FIELD(u, OF(UNIT, 0x2C), uint8_t *);
    int dx = FIELD(u, OF(UNIT, 0x1C), int) - b->x, dy = FIELD(u, OF(UNIT, 0x20), int) - b->y, t = FIELD(td, OF(TD, 0x4B8), int);
    if (FIELD(u, OF(UNIT, 0x18), void *) != b->pl || FIELD(u, OF(UNIT, 0x3C), int) <= 0 || dx * dx + dy * dy > b->r * b->r) return;
    if (bk_capacity(u) > 0 && b->nf < 8) b->f[b->nf++] = u;
    else if (is_citizen_type(t) && b->nc < 128) b->c[b->nc++] = u;
}
// the holder loads these units (each walks in): 1 if ordered
static int bk_load(uint8_t *f, uint8_t **units, int n) {
    if (!n) return 0;
    void *g = goal_load(f, units, n);
    if (!g) return 0;   // refused: deleted by unit_goal, no parts from it
    for (int j = 0; j < n; j++) unit_goal(units[j], goal_load_part(g), 0, 0x1F40);
    return 1;
}
static int bk_unload(uint8_t *f, int x, int y) {
    void *g = goal_unload(f, x, y, 1);
    return g ? unit_goal(f, g, 0, 0x1F40) : 0;
}

// bunker P x y [r]: P's fortresses within r (10) of x,y and what they hold; bunker P x y r load: they load P's citizens
// there (up to their capacity); bunker P x y r unload: they let everyone out beside them (tests)
static void cmd_bunker(char *out, const char *args) {
    struct bk_scan b = { 0 };
    int p = 0;
    char verb[16] = "";
    b.r = 10;
    if (sscanf(args, "%d %d %d %d %15s", &p, &b.x, &b.y, &b.r, verb) < 3 || p <= 0 || p >= 16 || !st_player(p)) {
        REPLY("error: bunker P x y [r [load|unload]]\n");
        return;
    }
    b.pl = st_player(p);
    grid_each(bk_unit, &b);
    int pop1 = GF(FN_POPULATION)(b.pl, 0, 0), pop2 = GF(FN_POPULATION)(b.pl, 0, 1);
    REPLY("ok: P%d population %d (with fortresses' %d), %d fortresses, %d citizens within %d of %d,%d:", p, pop1, pop2, b.nf, b.nc, b.r, b.x, b.y);
    int used = 0;
    for (int k = 0; k < b.nf; k++) {
        uint8_t *f = b.f[k];
        int cap = bk_capacity(f), held = bk_held(f), fx = FIELD(f, OF(UNIT, 0x1C), int), fy = FIELD(f, OF(UNIT, 0x20), int);
        uint8_t *bs = FIELD(f, OF(UNIT, 0x30), uint8_t *);
        REPLY(" [%d,%d %s, hp %d, holds %d of %d", fx, fy, bs && FIELD(bs, OF(UNIT, 0x340), uint8_t) ? "finished" : "going up", FIELD(f, OF(UNIT, 0x3C), int), held, cap);
        if (!strcmp(verb, "load")) {
            int n = cap - held;
            if (n > b.nc - used) n = b.nc - used;
            if (n > 0 && bk_load(f, b.c + used, n)) { REPLY(", loads %d", n); used += n; }
        } else if (!strcmp(verb, "unload") && held > 0) {
            if (bk_unload(f, fx, fy + 3)) REPLY(", unloads");
        }
        REPLY("]");
    }
    REPLY("\n");
}

// SHELTER ([all] bunkers, default 1): citizens under fire - one of them hurt within 6 tiles in the last 30 s, or enemy
// soldiers within 8 tiles (raiders_near) - go into a fortress of theirs with room within 15 tiles instead of back to
// their drop site (citizens_safety), and come out to work again once nothing has hurt anyone within 10 tiles of the
// fortress for 45 s and no enemy soldiers stand near it. A fortress is built beside the drop site nearest the base
// point (without a fortress within 10 tiles) once enemy aircraft are possible (World War I, an enemy there too) or 3
// citizens were hurt in the last 2 minutes; a second one from 40 citizens. Not through the build queue: the build
// manager's placement (0x46580A) failed every fortress, at a spot or with its own search (0,0) - the foundation is laid
// as the landing's forward base lays its (0x62EF8A, which pays), when it has the wood, 3 gatherers build it, an
// unfinished one gets 2 again every minute (the game's builders mostly finish it first); its wood is held back from
// unit training while it waits (bunker_reserve). scenarios/bombing_late.eeai (the bombing at 35 min, P3 off vs on):
// 20 seeds - workers 57.4 vs 66.2, buildings lost 2.4 vs 1.4, games down to 4-28 workers 4 vs 2 (10-seed batches
// gave 72.6, 61.7 and 52.2 against 53.1: noise); shelling and raid_ww1 (the fortress up only after minute 20 there:
// a WW1 start leaves little wood) about even.
static int g_bunkers = 1;   // [all] bunkers
static int g_bk_res = 0xFFFF;   // [all] bunker_reserve (below)
static int bk_room(void *pl);
static struct { int nf, nall; uint8_t *f[8], *all[8]; int nq[8]; uint8_t *q[8][40]; uint32_t build_t, quiet_t[8]; unsigned in, out; int need_wood; uint32_t shelter[8]; int walk[8]; } g_bk[16];   // shelter: when citizens were sent in (the quiet release); walk: units on their way in
static int bk_hurt_recent(unsigned pi, int x, int y, int r, uint32_t ms) {
    uint32_t now = game_time();
    int n = 0;
    for (int k = 0; k < HURT_N; k++) {
        typeof(&g_hurt[0].h[0]) h = &g_hurt[pi].h[k];
        if (h->t && now >= h->t && now - h->t <= ms && abs(h->x - x) <= r && abs(h->y - y) <= r) n++;
    }
    return n;
}
struct bk_own { void *pl; int nf, nall; uint8_t *f[8], *all[8]; int nw; uint8_t *w[256]; };
static uint8_t *bk_walking_into(uint8_t *u) {   // the holder unit u walks into (its load goal's part), else NULL
    uint8_t *g = FIELD(u, OF(UNIT, 0x1EC), uint8_t *), *h = g && *(uintptr_t *)g == GA(VT_GOAL_LOAD) ? FIELD(g, OF(GOAL, 0x50), uint8_t *) : NULL;
    return h != u ? h : NULL;   // the holder's own goal names itself
}
static void bk_own_unit(uint8_t *u, void *ctx) {   // finished ones (f) and foundations too (all); who walks in (w)
    struct bk_own *o = ctx;
    if (FIELD(u, OF(UNIT, 0x18), void *) != o->pl || FIELD(u, OF(UNIT, 0x3C), int) <= 0) return;
    uint8_t *h = bk_walking_into(u);
    if (h && o->nw < 256) o->w[o->nw++] = h;
    if (bk_capacity(u) <= 0) return;
    uint8_t *bs = FIELD(u, OF(UNIT, 0x30), uint8_t *);
    if (o->nall < 8) o->all[o->nall++] = u;
    if (bs && FIELD(bs, OF(UNIT, 0x340), uint8_t) && o->nf < 8) o->f[o->nf++] = u;
}
static void bk_refresh(void *pl, unsigned pi) {   // the player's fortresses (each pass of citizens_safety, every 4 s)
    static struct bk_own o;   // 1 KB of walkers: not on the stack
    memset(&o, 0, sizeof o);
    o.pl = pl;
    grid_each(bk_own_unit, &o);
    for (int k = 0; k < o.nf; k++) {
        g_bk[pi].walk[k] = 0;
        for (int j = 0; j < o.nw; j++) g_bk[pi].walk[k] += o.w[j] == o.f[k];
    }
    g_bk[pi].nall = o.nall;
    memcpy(g_bk[pi].all, o.all, sizeof o.all);
    for (int k = 0; k < o.nf; k++) {   // keep each fortress's quiet time across passes
        uint32_t q = 0;
        uint32_t sh = 0;
        for (int j = 0; j < g_bk[pi].nf; j++) if (g_bk[pi].f[j] == o.f[k]) { q = g_bk[pi].quiet_t[j]; sh = g_bk[pi].shelter[j]; }
        g_bk[pi].quiet_t[k] = q;
        g_bk[pi].shelter[k] = sh;
    }
    g_bk[pi].nf = o.nf;
    memcpy(g_bk[pi].f, o.f, sizeof o.f);
    memset(g_bk[pi].nq, 0, sizeof g_bk[pi].nq);
}
// citizen u is under fire: queued for the nearest fortress with room within 15 tiles - 1 if so
static int bk_shelter(void *pl, uint8_t *u) {
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!g_bunkers || !g_bk[pi].nf) return 0;
    int ux = FIELD(u, OF(UNIT, 0x1C), int), uy = FIELD(u, OF(UNIT, 0x20), int), best = -1, bd = 15 * 15 + 1;
    uint8_t *h = bk_walking_into(u);   // on its way into one of them already (the 4 s passes queued it again and again:
    for (int k = 0; h && k < g_bk[pi].nf; k++) if (g_bk[pi].f[k] == h) return 1;   // the fortress re-ordered, over-filled)
    if (bk_hurt_recent(pi, ux, uy, 6, 30000) < 1 && !raiders_near(pl, ux, uy, 8)) return 0;   // not under fire right now
    for (int k = 0; k < g_bk[pi].nf; k++) {
        uint8_t *f = g_bk[pi].f[k];
        int dx = FIELD(f, OF(UNIT, 0x1C), int) - ux, dy = FIELD(f, OF(UNIT, 0x20), int) - uy;
        if (dx * dx + dy * dy < bd && bk_held(f) + g_bk[pi].walk[k] + g_bk[pi].nq[k] < bk_capacity(f) && g_bk[pi].nq[k] < 40) { bd = dx * dx + dy * dy; best = k; }
    }
    if (best < 0) return 0;
    g_bk[pi].q[best][g_bk[pi].nq[best]++] = u;
    return 1;
}
static void bk_flush(void *pl, unsigned pi, uint32_t now) {   // the queued citizens walk in; quiet fortresses let them out
    for (int k = 0; k < g_bk[pi].nf; k++) {
        uint8_t *f = g_bk[pi].f[k];
        int fx = FIELD(f, OF(UNIT, 0x1C), int), fy = FIELD(f, OF(UNIT, 0x20), int);
        if (g_bk[pi].nq[k] && bk_load(f, g_bk[pi].q[k], g_bk[pi].nq[k])) {
            g_bk[pi].in += (unsigned)g_bk[pi].nq[k];
            g_bk[pi].quiet_t[k] = 0;
            g_bk[pi].shelter[k] = now ? now : 1;
            if (g_log) fprintf(g_log, "# bunker: P%u at %.1f min: %d citizens into the fortress at %d,%d (holds %d)\n", pi, now / 60000.0, g_bk[pi].nq[k], fx, fy, bk_held(f));
            continue;
        }
        int held = bk_held(f);
        if (held <= 0) {   // empty - unless citizens are still walking in
            if (g_bk[pi].shelter[k] && now - g_bk[pi].shelter[k] > 30000) g_bk[pi].shelter[k] = 0;
            continue;
        }
        if (!g_bk[pi].shelter[k]) continue;   // the reserve's soldiers only: bk_reserve_tick
        if (bk_hurt_recent(pi, fx, fy, 10, 45000) || raiders_near(pl, fx, fy, 10)) { g_bk[pi].quiet_t[k] = 0; continue; }
        if (!g_bk[pi].quiet_t[k]) { g_bk[pi].quiet_t[k] = now ? now : 1; continue; }
        if (now - g_bk[pi].quiet_t[k] < 8000) continue;   // two quiet looks
        if (bk_unload(f, fx, fy + 3)) {
            g_bk[pi].out += (unsigned)held;
            g_bk[pi].shelter[k] = 0;
            if (g_log) fprintf(g_log, "# bunker: P%u at %.1f min: the fortress at %d,%d lets %d out (quiet)\n", pi, now / 60000.0, fx, fy, held);
        }
    }
}
// builders for fortress b: the n nearest gathering citizens that aren't building it already
static int bk_builders(void *pl, uint8_t *b, int n) {
    void *cm = FIELD(pl, OF(PL, 0xB90), void *);
    ee_resource_mgr *rm = cm ? FIELD(cm, OF(CM, 0x38), ee_resource_mgr *) : NULL;
    int bx = FIELD(b, OF(UNIT, 0x1C), int), by = FIELD(b, OF(UNIT, 0x20), int), have = 0, sent = 0;
    if (!rm) return 0;
    for (int r = 0; r < 7; r++) {   // already on it
        uint8_t *list = FIELD_AT(rm, RM_LISTS + r * 0x14);
        for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), cnt = FIELD(list, 0x10, unsigned); k < cnt; k++) {
            uint8_t *u = deque_at(list, first + k), *g = u ? FIELD(u, OF(UNIT, 0x1EC), uint8_t *) : NULL;
            if (g && !strcmp(rtti_name(g), "EEUGBuildRepair@@") && FIELD(g, OF(GOAL, 0x3C), uint8_t *) == b) have++;
        }
    }
    while (have + sent < n) {
        uint8_t *best = NULL;
        int bd = 0x7FFFFFFF;
        for (int r = 2; r < 7; r++) {
            uint8_t *list = FIELD_AT(rm, RM_LISTS + r * 0x14);
            for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), cnt = FIELD(list, 0x10, unsigned); k < cnt; k++) {
                uint8_t *u = deque_at(list, first + k);
                if (!citizen_free(u, 0)) continue;
                int dx = FIELD(u, OF(UNIT, 0x1C), int) - bx, dy = FIELD(u, OF(UNIT, 0x20), int) - by;
                if (dx * dx + dy * dy < bd) { bd = dx * dx + dy * dy; best = u; }
            }
        }
        if (!best) break;
        void *goal = GF(FN_NEW)(GA(SIZE_GOAL_BUILD));
        if (!goal) break;
        GF(FN_BUILD_GOAL)(goal, pl, b, 1);
        if (!unit_goal(best, goal, 0, 0x1F51)) break;   // with the unit type's vf29 (+0x74) as the game's assignment
        sent++;
    }
    return sent;
}
static void bk_build(void *pl, unsigned pi, uint32_t now) {
    if (g_bk[pi].build_t && now >= g_bk[pi].build_t && now - g_bk[pi].build_t < 60000) return;
    g_bk[pi].build_t = now ? now : 1;
    int e = current_epoch(pl);
    if (e < 2 || e >= 14) return;
    for (int k = 0; k < g_bk[pi].nall; k++) {   // unfinished: builders
        uint8_t *bs = FIELD(g_bk[pi].all[k], OF(UNIT, 0x30), uint8_t *);
        if (!bs || !FIELD(bs, OF(UNIT, 0x340), uint8_t)) {
            int n = bk_builders(pl, g_bk[pi].all[k], 2);
            if (n && g_log) fprintf(g_log, "# bunker: P%u at %.1f min: %d more builders for the fortress at %d,%d\n", pi, now / 60000.0, n,
                                    FIELD(g_bk[pi].all[k], OF(UNIT, 0x1C), int), FIELD(g_bk[pi].all[k], OF(UNIT, 0x20), int));
        }
    }
    void *cm = FIELD(pl, OF(PL, 0xB90), void *);
    uint8_t *bm = cm ? FIELD(cm, OF(CM, 0x3C), uint8_t *) : NULL, *rm = cm ? FIELD(cm, OF(CM, 0x38), uint8_t *) : NULL;
    if (!bm || !rm) return;
    int air = 0, cit = 0;
    for (int q = 1; q < 16 && !air; q++) {
        void *o = st_player(q);
        air = o && o != pl && is_enemy(pl, o) && current_epoch(o) >= 9 && e >= 9;
    }
    for (int t = 0; t < 7; t++) cit += FIELD(rm, RM_LISTS + t * 0x14 + 0x10, int);
    int want = air || bk_hurt_recent(pi, 0, 0, 10000, 120000) >= 3 ? (cit >= 40 ? 2 : 1) : 0;
    if (g_bk_res >> pi & 1 && bk_room(pl) <= 5 && want < 1) want = 1;   // the reserve: near the population cap
    g_bk[pi].need_wood = 0;
    if (g_bk[pi].nall >= want) return;
    // 350 wood (0x555710 also wants a free builder at that moment: it passed in 2 of 10 games before minute 20, the
    // game's own builders take the foundation up anyway)
    uint8_t *od = type_data(FORTRESS_TYPE), *tt = FIELD(pl, PL_TECHTREE, uint8_t *);
    uint8_t *tech = od && tt ? GF(FN_TECH_LOOKUP)(tt, FIELD(od, OF(TD, 0x4E0), int)) : NULL;
    int cost = tech && FIELD(tech, OF(TECH, 0x8), uint8_t *) ? FIELD(FIELD(tech, OF(TECH, 0x8), uint8_t *), OF(TECHDATA, 0x28), int) : 350;   // the cost lives in its tech (wood)
    if (((int *)((uint8_t *)pl + PL_STOCK))[3] < cost) { g_bk[pi].need_wood = cost; return; }   // can't pay yet: saved for (bunker_reserve)
    // the drop site nearest the base point without a fortress within 10 tiles
    int bx = FIELD(bm, OF(BM, 0x12C), int), by = FIELD(bm, OF(BM, 0x130), int), d[2] = { 0, 0 }, bd = 0x7FFFFFFF;
    drops_cached(pl);
    for (int i = 0; i < g_drops[pi].n; i++) {
        int *p = g_drops[pi].d[i], dd = (p[0] - bx) * (p[0] - bx) + (p[1] - by) * (p[1] - by), held_near = 0;
        for (int k = 0; k < g_bk[pi].nall && !held_near; k++)
            held_near = abs(FIELD(g_bk[pi].all[k], OF(UNIT, 0x1C), int) - p[0]) <= 10 && abs(FIELD(g_bk[pi].all[k], OF(UNIT, 0x20), int) - p[1]) <= 10;
        if (!p[2] && !held_near && dd < bd) { bd = dd; d[0] = p[0]; d[1] = p[1]; }
    }
    if (bd == 0x7FFFFFFF) return;
    for (int r = 4; r <= 9; r++)
        for (int ax = -r; ax <= r; ax++)
            for (int ay = -r; ay <= r; ay++) {
                int x = d[0] + ax, y = d[1] + ay;
                if ((abs(ax) != r && abs(ay) != r) || !can_place(pl, FORTRESS_TYPE, x, y) || !can_place(pl, FORTRESS_TYPE, x + 1, y + 1) ||
                    !can_place(pl, FORTRESS_TYPE, x - 1, y - 1) || raiders_near(pl, x, y, 8)) continue;
                uint8_t *b = GF(FN_PLACE_BUILDING)(pl, FORTRESS_TYPE, x, y);
                int sent = b ? bk_builders(pl, b, 3) : 0;
                if (g_log) fprintf(g_log, "# bunker: P%u at %.1f min: fortress foundation at %d,%d %s, %d builders (%s, %d citizens, %d fortresses)\n", pi,
                                   now / 60000.0, x, y, b ? "laid" : "refused", sent, air ? "aircraft possible" : "citizens hurt", cit, g_bk[pi].nall);
                return;
            }
}
// the wood a fortress waits for, held back from unit training (as the fields' wood and a landing's transports)
static void bunker_reserve(void *pl, int *reserve) {
    if (g_bunkers && pl) reserve[3] += g_bk[(unsigned)FIELD(pl, PL_INDEX, int) & 15].need_wood;
}

// RESERVE ([all] bunker_reserve, a mask of players, default all; the playtester: "another use is to amass units at
// lower population limits, so losses can immediately be drawn from them"): units in a fortress don't count toward
// the population (0x556E11 adds them only when asked: (x, 1); the training's room uses (1, 0)). Every 10 s: with the
// map full (population at the cap - 1), up to 10 free land soldiers at home (in no group: 0x4707D8 on the combat
// manager, within 25 tiles of the base point; 4 stay out) go into the fortress with the most room, and training fills
// the map again; once the population is 4 below the cap with nothing loaded for a minute (losses), or enemy soldiers
// come within 20 tiles of it, the fullest fortress lets everyone out (not one where citizens shelter). The room the
// training sees (cap - in production - population) was no guide: its count of units in production ran up to 16-30
// while nothing came out, the reserve filled to 39 and never let anyone out - P3's losses fell (23 -> 14), its kills
// didn't rise. scenarios/lowpop.eeai (unit_limit 60 = 20 a player, cap 30; 10 seeds, P3 off vs on): kills 21.3 vs 21.3,
// losses 23.1 vs 19.0, buildings lost 9.4 vs 9.5 - no clear gain: of 153 soldiers ordered in, about 30 were inside at
// once (the AI's own groups take free soldiers back), and its units-in-production count, not the cap, held training
// back - the mod's own orders queued with no room (fixed: train_room, econ.c). With that, 20 seeds, P3 off vs on (both
// with train_room): P3 kills 22.7 vs 28.8, losses 27.2 vs 23.2, P2's losses 21.7 vs 27.4. On.
static unsigned g_bk_res_in[16], g_bk_res_out[16];
static uint32_t g_bk_res_load_t[16];
static int bk_room(void *pl) {   // cap - population on the map (the fortresses' units left out)
    int cap = unit_limit(pl);
    return cap - GF(FN_POPULATION)(pl, 0, 0);
}
struct bk_free { void *pl; uint8_t *cm; int bx, by, n; uint8_t *u[64]; };
static void bk_free_unit(uint8_t *u, void *ctx) {
    struct bk_free *f = ctx;
    uint8_t *td = FIELD(u, OF(UNIT, 0x2C), uint8_t *);
    if (FIELD(u, OF(UNIT, 0x18), void *) != f->pl || FIELD(u, OF(UNIT, 0x3C), int) <= 0 || !td || f->n >= 64) return;
    int dx = FIELD(u, OF(UNIT, 0x1C), int) - f->bx, dy = FIELD(u, OF(UNIT, 0x20), int) - f->by;
    if (dx * dx + dy * dy > 25 * 25 || !is_soldier(td) || is_ship_or_air(td)) return;
    if (GF(FN_CM_UNIT_FREE)(f->cm, FIELD(u, OF(UNIT, 0x30), void *))) f->u[f->n++] = u;
}
static void bk_reserve_tick(void *pl, unsigned pi, uint32_t now) {
    static uint32_t last[16];
    if (!(g_bk_res >> pi & 1) || !g_bk[pi].nf) return;
    if (last[pi] && now >= last[pi] && now - last[pi] < 10000) return;
    last[pi] = now ? now : 1;
    int room = bk_room(pl), threat = -1;
    for (int k = 0; k < g_bk[pi].nf && threat < 0; k++) {
        uint8_t *fo = g_bk[pi].f[k];
        if (!g_bk[pi].shelter[k] && bk_held(fo) > 0 && raiders_near(pl, FIELD(fo, OF(UNIT, 0x1C), int), FIELD(fo, OF(UNIT, 0x20), int), 20)) threat = k;
    }
    if (room <= 1 && threat < 0) {
        void *cm2 = FIELD(pl, OF(PL, 0xB90), void *);
        uint8_t *bm = cm2 ? FIELD(cm2, OF(CM, 0x3C), uint8_t *) : NULL;
        struct bk_free f = { pl, FIELD(pl, OF(PL, 0xB8C), uint8_t *), bm ? FIELD(bm, OF(BM, 0x12C), int) : 0, bm ? FIELD(bm, OF(BM, 0x130), int) : 0, 0, { 0 } };
        if (!f.cm || !bm) return;
        grid_each(bk_free_unit, &f);
        int best = -1, space = 0;
        for (int k = 0; k < g_bk[pi].nf; k++) {
            int s = bk_capacity(g_bk[pi].f[k]) - bk_held(g_bk[pi].f[k]) - g_bk[pi].walk[k];
            if (s > space) { space = s; best = k; }
        }
        int n = f.n - 4;
        if (n > 10) n = 10;
        if (n > space) n = space;
        if (best < 0 || n <= 0 || !bk_load(g_bk[pi].f[best], f.u, n)) return;
        g_bk_res_in[pi] += (unsigned)n;
        g_bk_res_load_t[pi] = now ? now : 1;
        if (g_log) fprintf(g_log, "# reserve: P%u at %.1f min: room %d - %d soldiers into the fortress at %d,%d (holds %d; %u in, %u out so far)\n", pi,
                           now / 60000.0, room, n, FIELD(g_bk[pi].f[best], OF(UNIT, 0x1C), int), FIELD(g_bk[pi].f[best], OF(UNIT, 0x20), int), bk_held(g_bk[pi].f[best]),
                           g_bk_res_in[pi], g_bk_res_out[pi]);
    } else if (threat >= 0 || (room >= 4 && (!g_bk_res_load_t[pi] || now - g_bk_res_load_t[pi] >= 60000))) {
        int best = threat, held = threat >= 0 ? bk_held(g_bk[pi].f[threat]) : 0;
        for (int k = 0; k < g_bk[pi].nf && threat < 0; k++) {
            uint8_t *fo = g_bk[pi].f[k];
            int h = bk_held(fo);
            if (h > held && !g_bk[pi].shelter[k]) { held = h; best = k; }
        }
        if (best < 0) return;
        uint8_t *fo = g_bk[pi].f[best];
        if (!bk_unload(fo, FIELD(fo, OF(UNIT, 0x1C), int), FIELD(fo, OF(UNIT, 0x20), int) + 3)) return;
        g_bk_res_out[pi] += (unsigned)held;
        if (g_log) fprintf(g_log, "# reserve: P%u at %.1f min: room %d%s - the fortress at %d,%d lets %d out (%u in, %u out so far)\n", pi, now / 60000.0,
                           room, threat >= 0 ? ", enemies near" : "", FIELD(fo, OF(UNIT, 0x1C), int), FIELD(fo, OF(UNIT, 0x20), int), held, g_bk_res_in[pi], g_bk_res_out[pi]);
    }
}

// a new game (the game start hook): the fortresses and the citizens queued for them are the last game's
static void bunker_new_game(void) {
    memset(g_bk, 0, sizeof g_bk);
    memset(g_bk_res_in, 0, sizeof g_bk_res_in);
    memset(g_bk_res_out, 0, sizeof g_bk_res_out);
    memset(g_bk_res_load_t, 0, sizeof g_bk_res_load_t);
}
