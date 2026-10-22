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
// the unit's id, +0x60 - by address until 2026-10-22, and a unit new in a dead one's memory went uncounted, as the heap
// happened to reuse it: two runs of the same game differed by one now and then) - trained citizens (tc) and soldiers (ts, army families) and their prices (unit_cost, econ.c): citizens' food
// (tcF), soldiers' food (tsF) and other resources (tsX). Units trained and killed within the same minute are missed.
// "# soldier costs:" once a game: every land soldier type with an attack and its price F/W/S/G/I (the food-free ones
// are what an early army could be made of without taking the citizens' food).
internal struct eco_dead { unsigned n, town, near_, far_, army, bld, ship, air, unk; } g_eco_dead[16];
internal uint32_t g_eco_t[16];

#define ECO_IDS 8192
internal uint32_t g_eco_ids[16][2][ECO_IDS];   // the units of the last scan and of this one (ids, open addressing)
internal unsigned g_eco_nids[16], g_eco_cur[16];
internal struct eco_spend { unsigned tc, ts, tcF, tsF, tsX; } g_eco_sp[16];
internal int g_eco_costs_logged;

internal void ecolog_new_game(void) {
    memset(g_eco_dead, 0, sizeof g_eco_dead); memset(g_eco_t, 0, sizeof g_eco_t);
    memset(g_eco_ids, 0, sizeof g_eco_ids); memset(g_eco_nids, 0, sizeof g_eco_nids); memset(g_eco_sp, 0, sizeof g_eco_sp);
    g_eco_costs_logged = 0;
}
internal int eco_in(uint32_t *t, uint32_t id, int add) {   // 1 when id is in t (add: put it there)
    for (unsigned k = 0, h = (id * 2654435761u) % ECO_IDS; k < ECO_IDS; k++, h = (h + 1) % ECO_IDS) {
        if (t[h] == id) return 1;
        if (!t[h]) { if (add) t[h] = id; return 0; }
    }
    return 0;
}
internal int eco_new_id(unsigned pi, uint32_t id) {   // 1 when id wasn't in the last scan; this scan remembers it
    uint32_t *cur = g_eco_ids[pi][g_eco_cur[pi]], *last = g_eco_ids[pi][g_eco_cur[pi] ^ 1];
    if (!id || g_eco_nids[pi] >= ECO_IDS * 3 / 4) return 0;
    if (!eco_in(cur, id, 1)) g_eco_nids[pi]++;
    return !eco_in(last, id, 0);
}
internal void eco_soldier_costs(ee_player *pl) {   // once a game: the land soldiers' prices
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
internal void eco_mines(ee_player *pl, struct eco_mines *c) {
    memset(c, 0, sizeof *c);
    static ee_unit *seen[1024];
    int ns = 0;
    unsigned cap = table_value(0x16);
    drops_cached(pl);
    unsigned pi = (unsigned)pidx(pl);
    int base_reg = -1;   // the base's continent
    {   ee_civ_mgr *cm = player_get_civ_mgr(pl); uint8_t *bm = cm ? FIELD(cm, OF(CM, 0x3C), uint8_t *) : NULL;
        int b[2] = { bm ? FIELD(bm, OF(BM, 0x12C), int) : -10000, bm ? FIELD(bm, OF(BM, 0x130), int) : -10000 };
        if (b[0] != -10000 && b[1] != -10000) base_reg = (short)map_area(b, 2); }
    void **tb = player_get_zones_begin(pl), **te = player_get_zones_end(pl);
    for (int t = 0; tb && tb + t < te && t < 256; t++) {
        uint8_t *town = tb[t];
        void **sb = town ? FIELD(town, OF(ZONE, 0x8), void **) : NULL, **se = town ? FIELD(town, OF(ZONE, 0xC), void **) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096; k++) {
            ee_unit *st = sb[k];
            int r = st ? unit_get_resource(st) : 0, j;
            if (r < 4 || r > 6 || unit_get_amount(st) <= 0) continue;   // stone, gold, iron with something left
            for (j = 0; j < ns && seen[j] != st; j++) ;
            if (j < ns) continue;
            if (ns < 1024) seen[ns++] = st;
            int x = unit_get_x(st), y = unit_get_y(st), dd = 0x7FFFFFFF;
            unsigned g = site_gatherers(st);
            for (int i = 0; i < g_drops[pi].n; i++) {
                int *d = g_drops[pi].d[i], dx = d[0] - x, dy = d[1] - y;
                if (!d[2] && dx * dx + dy * dy < dd) dd = dx * dx + dy * dy;
            }
            if (dd <= 144) { c->n[r]++; c->slots[r] += (int)cap; c->g[r] += (int)g; }
            else c->far_[r] += (int)g;
            if (dd > 144 && FIELD(town, OF(ZONE, 0x18), uint8_t) && explored_by(st, pl)) {
                int pos[2] = { x, y };   // known, explored, no drop site within 12 tiles, on the base's continent
                if (base_reg < 0 || (short)map_area(pos, 2) == base_reg) c->nd[r]++;
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
internal const char *eco_goal(ee_unit *u, int *res);
struct eco_food { int fo, fh, ff, fb, fx, hl, hlF, hl15, hl30, an, an30, anF30, anNew30, fsN, fsF, fsN20, fsF20, fish, fishF, fld, fldOK; };
internal int eco_drop_d2(ee_player *pl, int x, int y) {   // squared tiles to the nearest drop site for everything
    unsigned pi = (unsigned)pidx(pl);
    int dd = 0x7FFFFFFF;
    for (int i = 0; i < g_drops[pi].n; i++) {
        int *d = g_drops[pi].d[i], dx = d[0] - x, dy = d[1] - y;
        if (!d[2] && dx * dx + dy * dy < dd) dd = dx * dx + dy * dy;
    }
    return dd;
}
internal void eco_food(ee_player *pl, ee_resource_mgr *rm, struct eco_food *f) {
    memset(f, 0, sizeof *f);
    if (!rm) return;
    drops_cached(pl);
    uint8_t *list = FIELD_AT(rm, RM_LISTS + 2 * 0x14);
    for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), n = FIELD(list, 0x10, unsigned); k < n && k < 1024; k++) {
        ee_unit *u = deque_at(list, first + k);
        ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
        if (!td || unit_get_hp(u) <= 0) continue;
        int res;
        if (strcmp(eco_goal(u, &res), "gather")) continue;
        if (type_get_family(td) == 22) f->fb++;
        else if (res == UT(RES_FORAGE_BUSH)) f->fo++;
        else if (res == UT(B_FARM)) f->ff++;
        else {
            uint8_t *g = unit_get_goal(u);
            ee_unit *s = FIELD(g, OF(GOAL, 0x38), ee_unit *);
            ee_unit_type *st = s ? unit_get_type_data(s) : NULL;
            if (st && type_get_category(st) == 0x11) f->fh++; else f->fx++;
        }
    }
    void **hb = FIELD(rm, OF(RM, 0x88), void **), **he = FIELD(rm, OF(RM, 0x8C), void **);
    for (void **h = hb; hb && h < he && h - hb < 4096; h++) {
        ee_unit *a = *h;
        if (!a || !unit_get_valid(a) || unit_get_amount(a) <= 0) continue;
        f->hl++; f->hlF += unit_get_amount(a);
        int d2 = eco_drop_d2(pl, unit_get_x(a), unit_get_y(a));
        if (d2 <= 225) f->hl15++;
        if (d2 <= 900) f->hl30++;
    }
    void **qb = FIELD(rm, OF(RM, 0x98), void **), **qe = FIELD(rm, OF(RM, 0x9C), void **);
    for (void **h = qb; qb && h < qe && h - qb < 4096; h++)
        if (*h && unit_get_amount(*h) > 0) { f->fish++; f->fishF += unit_get_amount(*h); }
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (!td || unit_get_resource(u) != 2 || unit_get_amount(u) <= 0 || type_get_category(td) != 0x11) continue;
        f->an++;
        if (eco_drop_d2(pl, unit_get_x(u), unit_get_y(u)) > 900) continue;
        f->an30++; f->anF30 += unit_get_amount(u);
        void **h = hb;
        while (hb && h < he && *h != u) h++;
        if (!hb || h >= he) f->anNew30++;
    }
    ee_civ_mgr *cm = player_get_civ_mgr(pl); uint8_t *bm = cm ? FIELD(cm, OF(CM, 0x3C), uint8_t *) : NULL;
    int bx = bm ? FIELD(bm, OF(BM, 0x12C), int) : -10000, by = bm ? FIELD(bm, OF(BM, 0x130), int) : -10000;
    void **zb = player_get_zones_begin(pl), **ze = player_get_zones_end(pl);
    for (int z = 0; zb && zb + z < ze && z < 256; z++) {
        uint8_t *zone = zb[z];
        void **sb = zone ? FIELD(zone, OF(ZONE, 0x8), void **) : NULL, **se = zone ? FIELD(zone, OF(ZONE, 0xC), void **) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096; k++) {
            ee_unit *st = sb[k]; ee_unit_type *td = st ? unit_get_type_data(st) : NULL;
            if (!td || unit_get_resource(st) != 2 || type_get_id(td) != UT(RES_FORAGE_BUSH) || unit_get_amount(st) <= 0) continue;
            int x = unit_get_x(st), y = unit_get_y(st), dx = x - bx, dy = y - by;
            if (eco_drop_d2(pl, x, y) <= 900) { f->fsN++; f->fsF += unit_get_amount(st); }
            if (bx != -10000 && dx * dx + dy * dy < g_site_max_dist * g_site_max_dist) { f->fsN20++; f->fsF20 += unit_get_amount(st); }
        }
    }
    void *um = cm ? FIELD(cm, OF(CM, 0x44), void *) : NULL;
    uint8_t *fq = type_list(um, UT(B_FARM));
    for (unsigned k = 0, first = fq ? FIELD(fq, 0xC, unsigned) : 0, n = fq ? FIELD(fq, 0x10, unsigned) : 0; k < n && k < 512; k++) {
        ee_unit *fl = deque_at(fq, first + k), *b = fl ? unit_get_physics(fl) : NULL;
        if (!fl) continue;
        f->fld++;
        if (b && unit_get_finished(b)) f->fldOK++;
    }
}

internal const char *eco_goal(ee_unit *u, int *res) {   // what a citizen does; res: the gather site's type id (or -1)
    uint8_t *g = unit_get_goal(u);
    *res = -1;
    if (!g) return "none";
    if (goal_is_location(g)) return "walk";
    if (goal_is_attack(g)) return "attack";
    const char *n = rtti_name(g);
    if (!strcmp(n, "EEUGGather@@")) {
        ee_unit *s = FIELD(g, OF(GOAL, 0x38), ee_unit *);
        ee_unit_type *td = s ? unit_get_type_data(s) : NULL;
        if (td) *res = type_get_id(td);
        return "gather";
    }
    if (!strcmp(n, "EEUGBuildRepair@@")) return "build";
    if (!strcmp(n, "EEUGLoadTransport@@")) return "load";
    return "other";
}

internal void ecolog_tick(ee_player *pl, uint32_t now) {
    if (!g_autotest || !g_log || !pl) return;
    unsigned idx = (unsigned)pidx(pl);
    if (now >= g_eco_t[idx] && now / 60000 == g_eco_t[idx] / 60000 && g_eco_t[idx]) return;
    g_eco_t[idx] = now ? now : 1;
    void *cm = player_get_civ_mgr(pl), *ump = cm ? FIELD(cm, OF(CM, 0x44), void *) : NULL;
    ee_resource_mgr *rm = cm ? FIELD(cm, OF(CM, 0x38), ee_resource_mgr *) : NULL;
    int cit = 0, gat[7] = { 0 }, build = 0, walk = 0, att = 0, load = 0, other = 0, none = 0, idle0 = 0;
    for (int r = 0; rm && r < 7; r++) {   // the resource lists (deques): who gathers for which resource, list 0 idle
        uint8_t *list = FIELD_AT(rm, RM_LISTS + r * 0x14);
        for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), n = FIELD(list, 0x10, unsigned); k < n && k < 1024; k++) {
            ee_unit *u = deque_at(list, first + k);
            if (!u || unit_get_hp(u) <= 0) continue;
            int res;
            const char *g = eco_goal(u, &res);
            if (!strcmp(g, "gather")) gat[r]++;
            else if (r == 0 && !strcmp(g, "none")) idle0++;
        }
    }
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {   // every citizen on the map
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || !is_citizen_type(type_get_id(td))) continue;
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
            ee_unit *u = it.u;
            ee_unit_type *td = it.td;
            if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0) continue;
            int t = type_get_id(td), fam = type_get_family(td), is_cit = is_citizen_type(t);
            if (!is_cit && !is_army_family(fam)) continue;
            if (!eco_new_id(idx, ((uint32_t)unit_get_id(u))) || first) continue;
            if (is_cit) { sp->tc++; sp->tcF += (unsigned)unit_cost(pl, t, 2); }
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
            army_size(pl), population(pl, 1, 0),
            unit_limit(pl), ump ? FIELD(ump, OF(UM, 0x13C), int) : -1,
            d->n, d->town, d->near_, d->far_, d->army, d->bld, d->ship, d->air, d->unk, is_vanilla(pl) ? 0u : g_site_refused[idx],
            sp->tc, sp->tcF, sp->ts, sp->tsF, sp->tsX, mn.n[4], mn.slots[4], mn.g[4], mn.far_[4], mn.n[5], mn.slots[5], mn.g[5],
            mn.far_[5], mn.n[6], mn.slots[6], mn.g[6], mn.far_[6], mn.nd[4], mn.nd[5], mn.nd[6],
            fd.fo, fd.fh, fd.ff, fd.fb, fd.fx, fd.hl, fd.hlF, fd.hl15, fd.hl30, fd.an, fd.an30, fd.anF30, fd.anNew30, fd.fsN, fd.fsF, fd.fsN20, fd.fsF20, fd.fish, fd.fishF, fd.fld, fd.fldOK);
    LeaveCriticalSection(&g_lock);
}

// a unit of v killed by k (diplo.c hook_kill_stat, n 3 = a unit): citizens only
internal void eco_death(ee_unit *dead, int k, int v) {
    if (!g_autotest || !g_log || !dead) return;
    ee_unit_type *td = unit_get_type_data(dead);
    ee_player *pl = unit_get_owner(dead);
    if (!td || !pl || !is_citizen_type(type_get_id(td))) return;
    int x = unit_get_x(dead), y = unit_get_y(dead), dd[2], dist = -1;
    if (nearest_drop(pl, x, y, 1, dd)) dist = (int)sqrtf((float)((dd[0] - x) * (dd[0] - x) + (dd[1] - y) * (dd[1] - y)));
    int res;
    const char *goal = eco_goal(dead, &res);
    int own = 0, foe = 0, best = 0x7FFFFFFF;
    ee_unit *att = NULL;   // the killer's nearest unit
    void *kp = player_at(k);
    for (struct grid_it it = grid_it_box(x, y, 12, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *t = it.td;
        if (!t || unit_get_hp(u) <= 0) continue;
        ee_player *o = unit_get_owner(u);
        int ux = unit_get_x(u) - x, uy = unit_get_y(u) - y, d2 = ux * ux + uy * uy;
        int fam = type_get_family(t);
        int sc = d2 + (is_army_family(fam) ? 0 : fam == 0x1D ? 1000 : 2000);   // soldiers, ships, aircraft first, then towers
        if (o == kp && sc < best && d2 <= 144 && fam != 6 && fam != 22 && fam != 0 && fam != 1 && !is_citizen_type(type_get_id(t))) { best = sc; att = u; }
        if (!is_army_family(fam)) continue;
        if (o == pl && d2 <= 100) own++;
        else if (o == kp && d2 <= 64) foe++;
    }
    int atype = att ? type_get_id(unit_get_type_data(att)) : -1, acls = att ? type_get_family(unit_get_type_data(att)) : -1;
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
