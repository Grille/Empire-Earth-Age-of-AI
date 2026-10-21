// ECONOMY AND CITIZEN-LOSS LOG (autotest only; scripts/eco_timeline.py, docs/testing.md "Against the base-game AI"):
// the same lines for the mod's AIs and the vanilla players, so their economies can be laid side by side. Nothing
// here acts on the game - it only reads.
// "# eco: P<i> at <m> min: k=v ..." every game minute: citizens on the map by what they do (their goal +0x1EC:
// gather - by the resource of the site's list -, build/repair, walk, attack, load a transport, other, none), soldiers,
// population (0x556E11 with production) and cap (vf31), units in production (unit manager +0x13C), citizens killed
// so far (cdead) - at the town (cd_town: within 6 tiles of a drop site), near it (cd_near: 7-15), far out (cd_far) -
// and by what (cd_army soldiers, cd_bld towers / buildings, cd_ship ships, cd_air aircraft, cd_unk unknown), the
// dangerous sites the searches refused so far (refused, sites.c; mod only).
// "# death: P<v> citizen ..." for every citizen killed (both sides): where (tiles from the nearest drop site), what it
// was doing, the killer (player, type, class), the victim's own soldiers within 10 tiles and the killer side's
// soldiers within 8.
// The killer itself is not known at the kill statistics call (0x617780: vf28's attacker argument and its register are
// reused by then) - the line names the nearest unit of the killer's player within 12 tiles (soldier, tower, ship ...).
// Mines (in the # eco line): the stone / gold / iron sites the AI knows (its towns' site lists, player +0xB50, as
// navy.c navy_nodes) - m<R>n mines within 12 tiles of a drop site (not granaries), m<R>slots their worker slots (table
// value 0x16 a mine), m<R>g their gatherers (0x52E5F7), m<R>far gatherers at mines farther out, m<R>nd known mines
// (active zone, explored) on the base's continent with no drop site within 12 tiles. The zones are the map's, shared
// by all players, and 0x52E5F7 counts every player's gatherers: m<R>far is mostly the other side's miners at its own
// mines, m<R>g at mines near one's own drop sites nearly all one's own.
// Spending (in the # eco line): every unit of the player in the minute's scan that wasn't there at the last one (by
// address: a dead unit's memory is reused, so the set is the last scan's, not every unit ever seen) - trained citizens (tc) and soldiers (ts, army families) and their prices (unit_cost, econ.c): citizens' food
// (tcF), soldiers' food (tsF) and other resources (tsX). Units trained and killed within the same minute are missed.
// "# soldier costs:" once a game: every land soldier type with an attack and its price F/W/S/G/I (the food-free ones
// are what an early army could be made of without taking the citizens' food).
static struct eco_dead { unsigned n, town, near_, far_, army, bld, ship, air, unk; } g_eco_dead[16];
static uint32_t g_eco_t[16];

#define ECO_IDS 8192
static uint32_t g_eco_ids[16][2][ECO_IDS];   // the units of the last scan and of this one (addresses, open addressing)
static unsigned g_eco_nids[16], g_eco_cur[16];
static struct eco_spend { unsigned tc, ts, tcF, tsF, tsX; } g_eco_sp[16];
static int g_eco_costs_logged;

static void ecolog_new_game(void) {
    memset(g_eco_dead, 0, sizeof g_eco_dead); memset(g_eco_t, 0, sizeof g_eco_t);
    memset(g_eco_ids, 0, sizeof g_eco_ids); memset(g_eco_nids, 0, sizeof g_eco_nids); memset(g_eco_sp, 0, sizeof g_eco_sp);
    g_eco_costs_logged = 0;
}
static int eco_in(uint32_t *t, uint32_t id, int add) {   // 1 when id is in t (add: put it there)
    for (unsigned k = 0, h = (id * 2654435761u) % ECO_IDS; k < ECO_IDS; k++, h = (h + 1) % ECO_IDS) {
        if (t[h] == id) return 1;
        if (!t[h]) { if (add) t[h] = id; return 0; }
    }
    return 0;
}
static int eco_new_id(unsigned pi, uint32_t id) {   // 1 when id wasn't in the last scan; this scan remembers it
    uint32_t *cur = g_eco_ids[pi][g_eco_cur[pi]], *last = g_eco_ids[pi][g_eco_cur[pi] ^ 1];
    if (!id || g_eco_nids[pi] >= ECO_IDS * 3 / 4) return 0;
    if (!eco_in(cur, id, 1)) g_eco_nids[pi]++;
    return !eco_in(last, id, 0);
}
static void eco_soldier_costs(void *pl) {   // once a game: the land soldiers' prices
    if (g_eco_costs_logged) return;
    g_eco_costs_logged = 1;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# soldier costs: type F/W/S/G/I:");
    for (int t = 0; t < 724; t++) {
        if (!land_soldier(t) || soldier_score(t) <= 0) continue;
        fprintf(g_log, " %d %d/%d/%d/%d/%d", t, unit_cost(pl, t, 2), unit_cost(pl, t, 3), unit_cost(pl, t, 4), unit_cost(pl, t, 5), unit_cost(pl, t, 6));
    }
    fputc('\n', g_log);
    LeaveCriticalSection(&g_lock);
}
struct eco_mines { int n[7], slots[7], g[7], far_[7], nd[7]; };
static void eco_mines(void *pl, struct eco_mines *c) {
    memset(c, 0, sizeof *c);
    static uint8_t *seen[1024];
    int ns = 0;
    unsigned cap = GF(FN_TABLE_VALUE)((void *)GA(G_TABLE_VALUES), 0x16);
    drops_cached(pl);
    unsigned pi = (unsigned)pidx(pl);
    int base_reg = -1;   // the base's continent
    {   uint8_t *cm = FIELD(pl, 0xB90, uint8_t *), *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
        int b[2] = { bm ? FIELD(bm, 0x12C, int) : -10000, bm ? FIELD(bm, 0x130, int) : -10000 };
        if (b[0] != -10000 && b[1] != -10000) base_reg = ((short (__thiscall *)(void *, int *, int))GA(FN_REGION))((void *)GA(G_MAP_AREAS), b, 2); }
    void **tb = FIELD(pl, 0xB50, void **), **te = FIELD(pl, 0xB54, void **);
    for (int t = 0; tb && tb + t < te && t < 256; t++) {
        uint8_t *town = tb[t];
        void **sb = town ? FIELD(town, 8, void **) : NULL, **se = town ? FIELD(town, 0xC, void **) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096; k++) {
            uint8_t *st = sb[k];
            int r = st ? FIELD(st, 0x38, int) : 0, j;
            if (r < 4 || r > 6 || FIELD(st, 0x40, int) <= 0) continue;   // stone, gold, iron with something left
            for (j = 0; j < ns && seen[j] != st; j++) ;
            if (j < ns) continue;
            if (ns < 1024) seen[ns++] = st;
            int x = FIELD(st, 0x1C, int), y = FIELD(st, 0x20, int), dd = 0x7FFFFFFF;
            unsigned g = GF(FN_SITE_GATHERERS)((void *)GA(G_GAME_STATE), st);
            for (int i = 0; i < g_drops[pi].n; i++) {
                int *d = g_drops[pi].d[i], dx = d[0] - x, dy = d[1] - y;
                if (!d[2] && dx * dx + dy * dy < dd) dd = dx * dx + dy * dy;
            }
            if (dd <= 144) { c->n[r]++; c->slots[r] += (int)cap; c->g[r] += (int)g; }
            else c->far_[r] += (int)g;
            if (dd > 144 && FIELD(town, 0x18, uint8_t) && GF(FN_EXPLORED)(FIELD(st, 0x14, void *), pl)) {
                int pos[2] = { x, y };   // known, explored, no drop site within 12 tiles, on the base's continent
                if (base_reg < 0 || ((short (__thiscall *)(void *, int *, int))GA(FN_REGION))((void *)GA(G_MAP_AREAS), pos, 2) == base_reg) c->nd[r]++;
            }
        }
    }
}

// Food sources (in the # eco line): the food list's gatherers by what they gather - fo forage (berry bushes, res_forage_bush),
// fh hunt (animals: food units of class +0x4C4 0x11 - Deer, Goat, Ostrich, Elephant ...; the carcass too), ff fields
// (b_farm), fb fishing boats (family 22), fx anything else. The hunt list (rm +0x88: built once at the AI's start
// (0x48D263 -> 0x48B778) from the world's food units of class 0x11, sorted by distance, pruned of the dead every 15 s;
// fish, class 0x12, go to rm +0x98): hl its entries with food left, hlF their food, hl15 / hl30 those within 15 / 30
// tiles of a drop site (not granaries). Animals on the map (grid, class 0x11, food left): an all, an30 within 30 tiles
// of a drop site, anF30 their food, anNew30 those within 30 that are not in the hunt list (born or come later - the
// game never adds them). Forage (the sites of every zone, res_forage_bush, food left): fsN / fsF within 30 tiles, fsN20 / fsF20
// within site_max_distance of the base (what wild_food counts). Fish: fish / fishF in rm +0x98. Fields: fld all,
// fldOK finished.
static const char *eco_goal(uint8_t *u, int *res);
struct eco_food { int fo, fh, ff, fb, fx, hl, hlF, hl15, hl30, an, an30, anF30, anNew30, fsN, fsF, fsN20, fsF20, fish, fishF, fld, fldOK; };
static int eco_drop_d2(void *pl, int x, int y) {   // squared tiles to the nearest drop site for everything
    unsigned pi = (unsigned)pidx(pl);
    int dd = 0x7FFFFFFF;
    for (int i = 0; i < g_drops[pi].n; i++) {
        int *d = g_drops[pi].d[i], dx = d[0] - x, dy = d[1] - y;
        if (!d[2] && dx * dx + dy * dy < dd) dd = dx * dx + dy * dy;
    }
    return dd;
}
static void eco_food(void *pl, uint8_t *rm, struct eco_food *f) {
    memset(f, 0, sizeof *f);
    if (!rm) return;
    drops_cached(pl);
    uint8_t *list = rm + RM_LISTS + 2 * 0x14;
    for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), n = FIELD(list, 0x10, unsigned); k < n && k < 1024; k++) {
        uint8_t *u = deque_at(list, first + k), *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
        if (!td || FIELD(u, 0x3C, int) <= 0) continue;
        int res;
        if (strcmp(eco_goal(u, &res), "gather")) continue;
        if (FIELD(td, 0x4B4, int) == 22) f->fb++;
        else if (res == UT(RES_FORAGE_BUSH)) f->fo++;
        else if (res == UT(B_FARM)) f->ff++;
        else {
            uint8_t *g = FIELD(u, 0x1EC, uint8_t *), *s = FIELD(g, 0x38, uint8_t *), *st = s ? FIELD(s, 0x2C, uint8_t *) : NULL;
            if (st && FIELD(st, 0x4C4, int) == 0x11) f->fh++; else f->fx++;
        }
    }
    void **hb = FIELD(rm, 0x88, void **), **he = FIELD(rm, 0x8C, void **);
    for (void **h = hb; hb && h < he && h - hb < 4096; h++) {
        uint8_t *a = *h;
        if (!a || !a[0x2B] || FIELD(a, 0x40, int) <= 0) continue;
        f->hl++; f->hlF += FIELD(a, 0x40, int);
        int d2 = eco_drop_d2(pl, FIELD(a, 0x1C, int), FIELD(a, 0x20, int));
        if (d2 <= 225) f->hl15++;
        if (d2 <= 900) f->hl30++;
    }
    void **qb = FIELD(rm, 0x98, void **), **qe = FIELD(rm, 0x9C, void **);
    for (void **h = qb; qb && h < qe && h - qb < 4096; h++)
        if (*h && FIELD(*h, 0x40, int) > 0) { f->fish++; f->fishF += FIELD(*h, 0x40, int); }
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        uint8_t *u = it.u, *td = it.td;
        if (!td || FIELD(u, 0x38, int) != 2 || FIELD(u, 0x40, int) <= 0 || FIELD(td, 0x4C4, int) != 0x11) continue;
        f->an++;
        if (eco_drop_d2(pl, FIELD(u, 0x1C, int), FIELD(u, 0x20, int)) > 900) continue;
        f->an30++; f->anF30 += FIELD(u, 0x40, int);
        void **h = hb;
        while (hb && h < he && *h != u) h++;
        if (!hb || h >= he) f->anNew30++;
    }
    uint8_t *cm = FIELD(pl, 0xB90, uint8_t *), *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
    int bx = bm ? FIELD(bm, 0x12C, int) : -10000, by = bm ? FIELD(bm, 0x130, int) : -10000;
    void **zb = FIELD(pl, 0xB50, void **), **ze = FIELD(pl, 0xB54, void **);
    for (int z = 0; zb && zb + z < ze && z < 256; z++) {
        uint8_t *zone = zb[z];
        void **sb = zone ? FIELD(zone, 8, void **) : NULL, **se = zone ? FIELD(zone, 0xC, void **) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096; k++) {
            uint8_t *st = sb[k], *td = st ? FIELD(st, 0x2C, uint8_t *) : NULL;
            if (!td || FIELD(st, 0x38, int) != 2 || FIELD(td, 0x4B8, int) != UT(RES_FORAGE_BUSH) || FIELD(st, 0x40, int) <= 0) continue;
            int x = FIELD(st, 0x1C, int), y = FIELD(st, 0x20, int), dx = x - bx, dy = y - by;
            if (eco_drop_d2(pl, x, y) <= 900) { f->fsN++; f->fsF += FIELD(st, 0x40, int); }
            if (bx != -10000 && dx * dx + dy * dy < g_site_max_dist * g_site_max_dist) { f->fsN20++; f->fsF20 += FIELD(st, 0x40, int); }
        }
    }
    void *um = cm ? FIELD(cm, 0x44, void *) : NULL;
    uint8_t *fq = type_list(um, UT(B_FARM));
    for (unsigned k = 0, first = fq ? FIELD(fq, 0xC, unsigned) : 0, n = fq ? FIELD(fq, 0x10, unsigned) : 0; k < n && k < 512; k++) {
        uint8_t *fl = deque_at(fq, first + k), *b = fl ? FIELD(fl, 0x30, uint8_t *) : NULL;
        if (!fl) continue;
        f->fld++;
        if (b && b[0x340]) f->fldOK++;
    }
}

static const char *eco_goal(uint8_t *u, int *res) {   // what a citizen does; res: the gather site's type id (or -1)
    uint8_t *g = FIELD(u, 0x1EC, uint8_t *);
    *res = -1;
    if (!g) return "none";
    if (*(uintptr_t *)g == GA(VT_GOAL_LOCATION)) return "walk";
    if (*(uintptr_t *)g == GA(VT_GOAL_ATTACK)) return "attack";
    const char *n = rtti_name(g);
    if (!strcmp(n, "EEUGGather@@")) {
        uint8_t *s = FIELD(g, 0x38, uint8_t *), *td = s ? FIELD(s, 0x2C, uint8_t *) : NULL;
        if (td) *res = FIELD(td, 0x4B8, int);
        return "gather";
    }
    if (!strcmp(n, "EEUGBuildRepair@@")) return "build";
    if (!strcmp(n, "EEUGLoadTransport@@")) return "load";
    return "other";
}

static void ecolog_tick(void *pl, uint32_t now) {
    if (!g_auto.enabled || !g_log || !pl) return;
    unsigned idx = (unsigned)pidx(pl);
    if (now >= g_eco_t[idx] && now / 60000 == g_eco_t[idx] / 60000 && g_eco_t[idx]) return;
    g_eco_t[idx] = now ? now : 1;
    void *cm = FIELD(pl, 0xB90, void *), *ump = cm ? FIELD(cm, 0x44, void *) : NULL;
    uint8_t *rm = cm ? FIELD(cm, 0x38, uint8_t *) : NULL;
    int cit = 0, gat[7] = { 0 }, build = 0, walk = 0, att = 0, load = 0, other = 0, none = 0, idle0 = 0;
    for (int r = 0; rm && r < 7; r++) {   // the resource lists (deques): who gathers for which resource, list 0 idle
        uint8_t *list = rm + RM_LISTS + r * 0x14;
        for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), n = FIELD(list, 0x10, unsigned); k < n && k < 1024; k++) {
            uint8_t *u = deque_at(list, first + k);
            if (!u || FIELD(u, 0x3C, int) <= 0) continue;
            int res;
            const char *g = eco_goal(u, &res);
            if (!strcmp(g, "gather")) gat[r]++;
            else if (r == 0 && !strcmp(g, "none")) idle0++;
        }
    }
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {   // every citizen on the map
        uint8_t *u = it.u, *td = it.td;
        if (FIELD(u, 0x18, void *) != pl || FIELD(u, 0x3C, int) <= 0 || !is_citizen_type(FIELD(td, 0x4B8, int))) continue;
        cit++;
        int res;
        const char *g = eco_goal(u, &res);
        if (!strcmp(g, "build")) build++;
        else if (!strcmp(g, "walk")) walk++;
        else if (!strcmp(g, "attack")) att++;
        else if (!strcmp(g, "load")) load++;
        else if (!strcmp(g, "other")) other++;
        else if (!strcmp(g, "none")) none++;
    }
    {   // units new since the last look: trained citizens and soldiers and what they cost
        int first = !g_eco_t[idx] || now < 30000;   // the first look: the starting units
        g_eco_cur[idx] ^= 1;
        memset(g_eco_ids[idx][g_eco_cur[idx]], 0, sizeof g_eco_ids[idx][0]);
        g_eco_nids[idx] = 0;
        struct eco_spend *sp = &g_eco_sp[idx];
        for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
            uint8_t *u = it.u, *td = it.td;
            if (FIELD(u, 0x18, void *) != pl || FIELD(u, 0x3C, int) <= 0) continue;
            int t = FIELD(td, 0x4B8, int), fam = FIELD(td, 0x4B4, int), cit = is_citizen_type(t);
            if (!cit && !is_army_family(fam)) continue;
            if (!eco_new_id(idx, (uint32_t)(uintptr_t)u) || first) continue;
            if (cit) { sp->tc++; sp->tcF += (unsigned)unit_cost(pl, t, 2); }
            else {
                sp->ts++; sp->tsF += (unsigned)unit_cost(pl, t, 2);
                for (int r = 3; r < 7; r++) sp->tsX += (unsigned)unit_cost(pl, t, r);
            }
        }
    }
    eco_soldier_costs(pl);
    struct eco_mines mn;
    eco_mines(pl, &mn);
    struct eco_food fd;
    eco_food(pl, rm, &fd);
    struct eco_spend *sp = &g_eco_sp[idx];
    struct eco_dead *d = &g_eco_dead[idx];
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# eco: P%u at %u min: cit=%d gF=%d gW=%d gS=%d gG=%d gI=%d g0=%d build=%d walk=%d attack=%d load=%d other=%d none=%d "
            "idle0=%d soldiers=%d pop=%d cap=%d inprod=%d cdead=%u cd_town=%u cd_near=%u cd_far=%u cd_army=%u cd_bld=%u "
            "cd_ship=%u cd_air=%u cd_unk=%u refused=%u tc=%u tcF=%u ts=%u tsF=%u tsX=%u mSn=%d mSslots=%d mSg=%d mSfar=%d "
            "mGn=%d mGslots=%d mGg=%d mGfar=%d mIn=%d mIslots=%d mIg=%d mIfar=%d mSnd=%d mGnd=%d mInd=%d "
            "fo=%d fh=%d ff=%d fb=%d fx=%d hl=%d hlF=%d hl15=%d hl30=%d an=%d an30=%d anF30=%d anNew30=%d fsN=%d fsF=%d fsN20=%d fsF20=%d fish=%d fishF=%d fld=%d fldOK=%d\n",
            idx, now / 60000, cit, gat[2], gat[3], gat[4], gat[5], gat[6], gat[0], build, walk, att, load, other, none, idle0,
            army_size(pl), GF(FN_POPULATION)(pl, 1, 0),
            ((int (__thiscall *)(void *, int))(*(void ***)pl)[31])(pl, 0), ump ? FIELD(ump, 0x13C, int) : -1,
            d->n, d->town, d->near_, d->far_, d->army, d->bld, d->ship, d->air, d->unk, is_vanilla(pl) ? 0u : g_site_refused[idx],
            sp->tc, sp->tcF, sp->ts, sp->tsF, sp->tsX, mn.n[4], mn.slots[4], mn.g[4], mn.far_[4], mn.n[5], mn.slots[5], mn.g[5],
            mn.far_[5], mn.n[6], mn.slots[6], mn.g[6], mn.far_[6], mn.nd[4], mn.nd[5], mn.nd[6],
            fd.fo, fd.fh, fd.ff, fd.fb, fd.fx, fd.hl, fd.hlF, fd.hl15, fd.hl30, fd.an, fd.an30, fd.anF30, fd.anNew30, fd.fsN, fd.fsF, fd.fsN20, fd.fsF20, fd.fish, fd.fishF, fd.fld, fd.fldOK);
    LeaveCriticalSection(&g_lock);
}

// a unit of v killed by k (diplo.c hook_kill_stat, n 3 = a unit): citizens only
static void eco_death(uint8_t *dead, int k, int v) {
    if (!g_auto.enabled || !g_log || !dead) return;
    uint8_t *td = FIELD(dead, 0x2C, uint8_t *);
    void *pl = FIELD(dead, 0x18, void *);
    if (!td || !pl || !is_citizen_type(FIELD(td, 0x4B8, int))) return;
    int x = FIELD(dead, 0x1C, int), y = FIELD(dead, 0x20, int), dd[2], dist = -1;
    if (nearest_drop(pl, x, y, 1, dd)) dist = (int)sqrtf((float)((dd[0] - x) * (dd[0] - x) + (dd[1] - y) * (dd[1] - y)));
    int res;
    const char *goal = eco_goal(dead, &res);
    int own = 0, foe = 0, best = 0x7FFFFFFF;
    uint8_t *att = NULL;   // the killer's nearest unit
    void *kp = player_at(k);
    for (struct grid_it it = grid_it_box(x, y, 12, 1); grid_it_next(&it); ) {
        uint8_t *u = it.u, *t = it.td;
        if (!t || FIELD(u, 0x3C, int) <= 0) continue;
        void *o = FIELD(u, 0x18, void *);
        int ux = FIELD(u, 0x1C, int) - x, uy = FIELD(u, 0x20, int) - y, d2 = ux * ux + uy * uy;
        int fam = FIELD(t, 0x4B4, int);
        int sc = d2 + (is_army_family(fam) ? 0 : fam == 0x1D ? 1000 : 2000);   // soldiers, ships, aircraft first, then towers
        if (o == kp && sc < best && d2 <= 144 && fam != 6 && fam != 22 && fam != 0 && fam != 1 && !is_citizen_type(FIELD(t, 0x4B8, int))) { best = sc; att = u; }
        if (!is_army_family(fam)) continue;
        if (o == pl && d2 <= 100) own++;
        else if (o == kp && d2 <= 64) foe++;
    }
    int atype = att ? FIELD(FIELD(att, 0x2C, uint8_t *), 0x4B8, int) : -1, acls = att ? FIELD(FIELD(att, 0x2C, uint8_t *), 0x4B4, int) : -1;
    struct eco_dead *d = &g_eco_dead[v & 15];
    d->n++;
    if (dist >= 0 && dist <= 6) d->town++; else if (dist >= 0 && dist <= 15) d->near_++; else d->far_++;
    // the killer's class (+0x4B4): 2 / 19 / 24 / 25 buildings and towers (as the kill statistics count them), ships
    // 4 / 10 / 18 / 21, aircraft 3 / 7 / 17
    if (!att) d->unk++;
    else if (acls == 2 || acls == 19 || acls == 24 || acls == 25 || acls == 29) d->bld++;
    else if (acls == 4 || acls == 10 || acls == 18 || acls == 21) d->ship++;
    else if (acls == 3 || acls == 7 || acls == 17) d->air++;
    else d->army++;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# death: P%d citizen at %.1f min at %d,%d, %d tiles from a drop site, %s (site type %d), by P%d (nearest: type %d class %d at %d tiles); "
            "own soldiers within 10: %d, the killer's within 8: %d\n", v, game_time() / 60000.0, x, y, dist, goal,
            res, k, atype, acls, att ? (int)sqrtf((float)(best % 1000)) : -1, own, foe);
    LeaveCriticalSection(&g_lock);
}
