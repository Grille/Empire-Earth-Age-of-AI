// BUNKERS: the Fortress (type 672, family 29, tech 3072 - Copper Age) holds units like a transport (type +0x54C
// capacity, 40): inside they are off the map, safe, and (the playtester) don't count toward the population limit. The
// game's AI never builds or uses it. The playtester's uses: citizens out of bombing runs, a refuge for an outpost's
// gatherers until an army comes, a reserve at low population limits that replaces losses at once. Garrisons proper
// (type +0x550: Town Center 15, Capitol 50, Settlement 5, Granary 8) are a different thing. Included by eeaimod.c
// after landing.c (the load goal as the transports': EEUGLoadTransport 0x61487F on the holder, its part for each unit).

#define FORTRESS_TYPE 672
static int bk_capacity(uint8_t *u) {   // what a building holds as a transport does, 0 = none
    uint8_t *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
    return td && FIELD(td, 0x4B4, int) == 29 ? FIELD(td, 0x54C, int) : 0;
}
static int bk_held(uint8_t *u) { return ((int (__thiscall *)(void *))0x0042E347)(u); }   // units inside

struct bk_scan { void *pl; int x, y, r, nf, nc; uint8_t *f[8], *c[128]; };
static void bk_unit(uint8_t *u, void *ctx) {
    struct bk_scan *b = ctx;
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    int dx = FIELD(u, 0x1C, int) - b->x, dy = FIELD(u, 0x20, int) - b->y, t = FIELD(td, 0x4B8, int);
    if (FIELD(u, 0x18, void *) != b->pl || FIELD(u, 0x3C, int) <= 0 || dx * dx + dy * dy > b->r * b->r) return;
    if (bk_capacity(u) > 0 && b->nf < 8) b->f[b->nf++] = u;
    else if ((t == 32 || t == 480) && b->nc < 128) b->c[b->nc++] = u;
}
// the holder loads these units (each walks in): 1 if ordered
static int bk_load(uint8_t *f, uint8_t **units, int n) {
    if (!n) return 0;
    uint8_t *vec[4] = { 0, (uint8_t *)units, (uint8_t *)(units + n), (uint8_t *)(units + n) };
    void *g = ((void *(__cdecl *)(unsigned))0x00683718)(0x68);
    if (g) g = ((void *(__thiscall *)(void *, void *, void *, int))0x0061487F)(g, f, vec, 1);
    if (!g) return 0;
    unit_goal(f, g, 0, 0x1F40);
    for (int j = 0; j < n; j++) {
        void *pg = ((void *(__thiscall *)(void *))(*(void ***)g)[4])(g);
        unit_goal(units[j], pg, 0, 0x1F40);
    }
    return 1;
}
static int bk_unload(uint8_t *f, int x, int y) {
    void *g = ((void *(__cdecl *)(unsigned))0x00683718)(0x40);
    if (g) g = ((void *(__thiscall *)(void *, void *, int, int, int))0x00615A1F)(g, f, x, y, 1);
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
    REPLY("ok: P%d %d fortresses, %d citizens within %d of %d,%d:", p, b.nf, b.nc, b.r, b.x, b.y);
    int used = 0;
    for (int k = 0; k < b.nf; k++) {
        uint8_t *f = b.f[k];
        int cap = bk_capacity(f), held = bk_held(f), fx = FIELD(f, 0x1C, int), fy = FIELD(f, 0x20, int);
        uint8_t *bs = FIELD(f, 0x30, uint8_t *);
        REPLY(" [%d,%d %s, hp %d, holds %d of %d", fx, fy, bs && FIELD(bs, 0x340, uint8_t) ? "finished" : "going up", FIELD(f, 0x3C, int), held, cap);
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
// unit training while it waits (bunker_reserve). scenarios/bombing_late.txt (the bombing at 35 min, 10 seeds, P3 off
// vs on): workers 53.1 vs 72.6, buildings lost 4.9 vs 1.0, the worst game 1 vs 38 workers; shelling and raid_ww1
// (the fortress up only after minute 20 there: a WW1 start leaves little wood) about even.
static int g_bunkers = 1;   // [all] bunkers
static struct { int nf, nall; uint8_t *f[8], *all[8]; int nq[8]; uint8_t *q[8][40]; uint32_t build_t, quiet_t[8]; unsigned in, out; int need_wood; } g_bk[16];
static int bk_hurt_recent(unsigned pi, int x, int y, int r, uint32_t ms) {
    uint32_t now = game_time();
    int n = 0;
    for (int k = 0; k < HURT_N; k++) {
        typeof(&g_hurt[0].h[0]) h = &g_hurt[pi].h[k];
        if (h->t && now >= h->t && now - h->t <= ms && abs(h->x - x) <= r && abs(h->y - y) <= r) n++;
    }
    return n;
}
struct bk_own { void *pl; int nf, nall; uint8_t *f[8], *all[8]; };
static void bk_own_unit(uint8_t *u, void *ctx) {   // finished ones (f) and foundations too (all)
    struct bk_own *o = ctx;
    if (FIELD(u, 0x18, void *) != o->pl || FIELD(u, 0x3C, int) <= 0 || bk_capacity(u) <= 0) return;
    uint8_t *bs = FIELD(u, 0x30, uint8_t *);
    if (o->nall < 8) o->all[o->nall++] = u;
    if (bs && FIELD(bs, 0x340, uint8_t) && o->nf < 8) o->f[o->nf++] = u;
}
static void bk_refresh(void *pl, unsigned pi) {   // the player's fortresses (each pass of citizens_safety, every 4 s)
    struct bk_own o = { pl, 0, 0, { 0 }, { 0 } };
    grid_each(bk_own_unit, &o);
    g_bk[pi].nall = o.nall;
    memcpy(g_bk[pi].all, o.all, sizeof o.all);
    for (int k = 0; k < o.nf; k++) {   // keep each fortress's quiet time across passes
        uint32_t q = 0;
        for (int j = 0; j < g_bk[pi].nf; j++) if (g_bk[pi].f[j] == o.f[k]) q = g_bk[pi].quiet_t[j];
        g_bk[pi].quiet_t[k] = q;
    }
    g_bk[pi].nf = o.nf;
    memcpy(g_bk[pi].f, o.f, sizeof o.f);
    memset(g_bk[pi].nq, 0, sizeof g_bk[pi].nq);
}
// citizen u is under fire: queued for the nearest fortress with room within 15 tiles - 1 if so
static int bk_shelter(void *pl, uint8_t *u) {
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!g_bunkers || !g_bk[pi].nf) return 0;
    int ux = FIELD(u, 0x1C, int), uy = FIELD(u, 0x20, int), best = -1, bd = 15 * 15 + 1;
    if (bk_hurt_recent(pi, ux, uy, 6, 30000) < 1 && !raiders_near(pl, ux, uy, 8)) return 0;   // not under fire right now
    for (int k = 0; k < g_bk[pi].nf; k++) {
        uint8_t *f = g_bk[pi].f[k];
        int dx = FIELD(f, 0x1C, int) - ux, dy = FIELD(f, 0x20, int) - uy;
        if (dx * dx + dy * dy < bd && bk_held(f) + g_bk[pi].nq[k] < bk_capacity(f) && g_bk[pi].nq[k] < 40) { bd = dx * dx + dy * dy; best = k; }
    }
    if (best < 0) return 0;
    g_bk[pi].q[best][g_bk[pi].nq[best]++] = u;
    return 1;
}
static void bk_flush(void *pl, unsigned pi, uint32_t now) {   // the queued citizens walk in; quiet fortresses let them out
    for (int k = 0; k < g_bk[pi].nf; k++) {
        uint8_t *f = g_bk[pi].f[k];
        int fx = FIELD(f, 0x1C, int), fy = FIELD(f, 0x20, int);
        if (g_bk[pi].nq[k] && bk_load(f, g_bk[pi].q[k], g_bk[pi].nq[k])) {
            g_bk[pi].in += (unsigned)g_bk[pi].nq[k];
            g_bk[pi].quiet_t[k] = 0;
            if (g_log) fprintf(g_log, "# bunker: P%u at %.1f min: %d citizens into the fortress at %d,%d (holds %d)\n", pi, now / 60000.0, g_bk[pi].nq[k], fx, fy, bk_held(f));
            continue;
        }
        int held = bk_held(f);
        if (held <= 0) continue;
        if (bk_hurt_recent(pi, fx, fy, 10, 45000) || raiders_near(pl, fx, fy, 10)) { g_bk[pi].quiet_t[k] = 0; continue; }
        if (!g_bk[pi].quiet_t[k]) { g_bk[pi].quiet_t[k] = now ? now : 1; continue; }
        if (now - g_bk[pi].quiet_t[k] < 8000) continue;   // two quiet looks
        if (bk_unload(f, fx, fy + 3)) {
            g_bk[pi].out += (unsigned)held;
            if (g_log) fprintf(g_log, "# bunker: P%u at %.1f min: the fortress at %d,%d lets %d out (quiet)\n", pi, now / 60000.0, fx, fy, held);
        }
    }
}
// builders for fortress b: the n nearest gathering citizens that aren't building it already
static int bk_builders(void *pl, uint8_t *b, int n) {
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *rm = cm ? FIELD(cm, 0x38, uint8_t *) : NULL;
    int bx = FIELD(b, 0x1C, int), by = FIELD(b, 0x20, int), have = 0, sent = 0;
    if (!rm) return 0;
    for (int r = 0; r < 7; r++) {   // already on it
        uint8_t *list = rm + RM_LISTS + r * 0x14;
        for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), cnt = FIELD(list, 0x10, unsigned); k < cnt; k++) {
            uint8_t *u = deque_at(list, first + k), *g = u ? FIELD(u, 0x1EC, uint8_t *) : NULL;
            if (g && !strcmp(rtti_name(g), "EEUGBuildRepair@@") && FIELD(g, 0x3C, uint8_t *) == b) have++;
        }
    }
    while (have + sent < n) {
        uint8_t *best = NULL;
        int bd = 0x7FFFFFFF;
        for (int r = 2; r < 7; r++) {
            uint8_t *list = rm + RM_LISTS + r * 0x14;
            for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), cnt = FIELD(list, 0x10, unsigned); k < cnt; k++) {
                uint8_t *u = deque_at(list, first + k);
                if (!citizen_free(u, 0)) continue;
                int dx = FIELD(u, 0x1C, int) - bx, dy = FIELD(u, 0x20, int) - by;
                if (dx * dx + dy * dy < bd) { bd = dx * dx + dy * dy; best = u; }
            }
        }
        if (!best) break;
        void *goal = ((void *(__cdecl *)(unsigned))FN_NEW)(0x5C);
        if (!goal) break;
        ((void (__thiscall *)(void *, void *, void *, int))FN_BUILD_GOAL)(goal, pl, b, 1);
        ((void (__thiscall *)(void *, void *, int, int))FN_SET_GOAL)(best, goal, 0, 0);
        ((void (__thiscall *)(void *, int))FN_UNIT_SAY)(best, 0x1F51);
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
        uint8_t *bs = FIELD(g_bk[pi].all[k], 0x30, uint8_t *);
        if (!bs || !FIELD(bs, 0x340, uint8_t)) {
            int n = bk_builders(pl, g_bk[pi].all[k], 2);
            if (n && g_log) fprintf(g_log, "# bunker: P%u at %.1f min: %d more builders for the fortress at %d,%d\n", pi, now / 60000.0, n,
                                    FIELD(g_bk[pi].all[k], 0x1C, int), FIELD(g_bk[pi].all[k], 0x20, int));
        }
    }
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL, *rm = cm ? FIELD(cm, 0x38, uint8_t *) : NULL;
    if (!bm || !rm) return;
    int air = 0, cit = 0;
    for (int q = 1; q < 16 && !air; q++) {
        void *o = st_player(q);
        air = o && o != pl && is_enemy(pl, o) && current_epoch(o) >= 9 && e >= 9;
    }
    for (int t = 0; t < 7; t++) cit += FIELD(rm, RM_LISTS + t * 0x14 + 0x10, int);
    int want = air || bk_hurt_recent(pi, 0, 0, 10000, 120000) >= 3 ? (cit >= 40 ? 2 : 1) : 0;
    g_bk[pi].need_wood = 0;
    if (g_bk[pi].nall >= want) return;
    // 350 wood (0x555710 also wants a free builder at that moment: it passed in 2 of 10 games before minute 20, the
    // game's own builders take the foundation up anyway)
    uint8_t *od = ((uint8_t **)0x009418A0)[FORTRESS_TYPE], *tt = FIELD(pl, PL_TECHTREE, uint8_t *);
    uint8_t *tech = od && tt ? ((uint8_t *(__thiscall *)(void *, int))0x004019A1)(tt, FIELD(od, 0x4E0, int)) : NULL;
    int cost = tech && FIELD(tech, 8, uint8_t *) ? FIELD(FIELD(tech, 8, uint8_t *), 0x28, int) : 350;   // the cost lives in its tech (wood)
    if (((int *)((uint8_t *)pl + PL_STOCK))[3] < cost) { g_bk[pi].need_wood = cost; return; }   // can't pay yet: saved for (bunker_reserve)
    // the drop site nearest the base point without a fortress within 10 tiles
    int bx = FIELD(bm, 0x12C, int), by = FIELD(bm, 0x130, int), d[2] = { 0, 0 }, bd = 0x7FFFFFFF;
    drops_cached(pl);
    for (int i = 0; i < g_drops[pi].n; i++) {
        int *p = g_drops[pi].d[i], dd = (p[0] - bx) * (p[0] - bx) + (p[1] - by) * (p[1] - by), held_near = 0;
        for (int k = 0; k < g_bk[pi].nall && !held_near; k++)
            held_near = abs(FIELD(g_bk[pi].all[k], 0x1C, int) - p[0]) <= 10 && abs(FIELD(g_bk[pi].all[k], 0x20, int) - p[1]) <= 10;
        if (!p[2] && !held_near && dd < bd) { bd = dd; d[0] = p[0]; d[1] = p[1]; }
    }
    if (bd == 0x7FFFFFFF) return;
    for (int r = 4; r <= 9; r++)
        for (int ax = -r; ax <= r; ax++)
            for (int ay = -r; ay <= r; ay++) {
                int x = d[0] + ax, y = d[1] + ay;
                if ((abs(ax) != r && abs(ay) != r) || !can_place(pl, FORTRESS_TYPE, x, y) || !can_place(pl, FORTRESS_TYPE, x + 1, y + 1) ||
                    !can_place(pl, FORTRESS_TYPE, x - 1, y - 1) || raiders_near(pl, x, y, 8)) continue;
                uint8_t *b = ((uint8_t *(__cdecl *)(void *, int, int, int))0x0062EF8A)(pl, FORTRESS_TYPE, x, y);
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
