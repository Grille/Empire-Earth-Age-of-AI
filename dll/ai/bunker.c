// BUNKERS: the Fortress (type 672, CLS_FORTRESS, tech 3072 - Copper Age) holds units like a transport (type +0x54C
// capacity, 40): inside they are off the map, safe, and (the playtester) don't count toward the population limit. The
// game's AI never builds or uses it. The playtester's uses: citizens out of bombing runs, a refuge for an outpost's
// gatherers until an army comes, a reserve at low population limits that replaces losses at once. A garrison is
// something else (the playtester): citizens put into a building for good, to upgrade it (type +0x550, its most: Town
// Center 15, Capitol 50, Settlement 5, Granary 8; the trigger attribute Garrison Level) - no shelter. Included by eeaimod.c
// after landing.c (the load goal as the transports': EEUGLoadTransport 0x61487F on the holder, its part for each unit).

#define FORTRESS_TYPE UT(B_FORTRESS)
internal int bk_capacity(ee_unit *u) {   // what a building holds as a transport does, 0 = none
    ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
    return td && unit_type_get_class_id(td) == CLS_FORTRESS ? unit_type_get_transport_capacity(td) : 0;
}
internal int bk_held(ee_unit *u) { return unit_count_inside(u); }   // units inside

struct bk_scan { ee_player *pl; ivec2 p; int r, nf, nc; ee_unit *f[8], *c[128]; };
internal void bk_unit(ee_unit *u, void *ctx) {
    struct bk_scan *b = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    int t = unit_type_get_id(td);
    if (unit_get_owner(u) != b->pl || unit_get_hp(u) <= 0 || ivec2_dist2(unit_get_tile(u), b->p) > b->r * b->r) return;
    if (bk_capacity(u) > 0 && b->nf < 8) b->f[b->nf++] = u;
    else if (is_citizen_type(t) && b->nc < 128) b->c[b->nc++] = u;
}
// the holder loads these units (each walks in): 1 if ordered
internal int bk_load(ee_unit *f, ee_unit **units, int n) {
    if (!n) return 0;
    ee_goal *g = goal_load(f, units, n);
    if (!g) return 0;   // refused: deleted by unit_goal, no parts from it
    for (int j = 0; j < n; j++) unit_goal(units[j], goal_load_part(g), 0, 0x1F40);
    return 1;
}
// the holder lets everyone out: 1 if ordered. The game takes an unload to a spot it can't put them on and does nothing:
// on the player's save "standing army" P7's fortress at 116,334 was told 1394 times in 15 minutes to let its 17 out at
// the tile 3 south and held them all (halberdiers, ballistas - its army and its room); 3 north (its Town Center) and
// 3 north-east the same, 3 east they came out at once. A spot that works isn't told by the tiles' buildings (a guess
// from them ruled out east too), so: 3 tiles out on a side on land, and a holder holding as many at its next unload
// as at this one tries the next side
internal struct { ee_unit *f; int side, held; } g_bk_out[32];
internal unsigned g_bk_out_next;
internal int bk_unload(ee_unit *f) {
    static const int dir[8][2] = { { 0, 3 }, { 3, 0 }, { 0, -3 }, { -3, 0 }, { 3, 3 }, { -3, 3 }, { 3, -3 }, { -3, -3 } };
    int fx = unit_get_x(f), fy = unit_get_y(f), held = bk_held(f), k = 0;
    while (k < 32 && g_bk_out[k].f != f) k++;
    if (k == 32) { k = (int)(g_bk_out_next++ & 31); g_bk_out[k].f = f; g_bk_out[k].side = 0; g_bk_out[k].held = -1; }
    else if (g_bk_out[k].held == held) g_bk_out[k].side = (g_bk_out[k].side + 1) & 7;   // nothing came out the last time
    g_bk_out[k].held = held;
    int s = g_bk_out[k].side;
    for (int t = 0; t < 8 && !area_id(iv2(fx + dir[s][0], fy + dir[s][1]), 2); t++) s = (s + 1) & 7;
    g_bk_out[k].side = s;
    ee_goal *g = goal_unload(f, fx + dir[s][0], fy + dir[s][1], 1);
    return g ? unit_goal(f, g, 0, 0x1F40) : 0;
}

// bunker P x y [r]: P's fortresses within r (10) of x,y and what they hold; bunker P x y r load: they load P's citizens
// there (up to their capacity); bunker P x y r unload: they let everyone out beside them (tests)
internal void cmd_bunker(char *out, const char *args) {
    struct bk_scan b = { 0 };
    int p = 0;
    char verb[16] = "";
    b.r = 10;
    if (sscanf(args, "%d %d %d %d %15s", &p, &b.p.x, &b.p.y, &b.r, verb) < 3 || p <= 0 || p >= 16 || !st_player(p)) {
        REPLY("error: bunker P x y [r [load|unload]]\n");
        return;
    }
    b.pl = st_player(p);
    grid_each(bk_unit, &b);
    int pop1 = player_population(b.pl, 0, 0), pop2 = player_population(b.pl, 0, 1);
    REPLY("ok: P%d population %d (with fortresses' %d), %d fortresses, %d citizens within %d of %d,%d:", p, pop1, pop2, b.nf, b.nc, b.r, b.p.x, b.p.y);
    int used = 0;
    for (int k = 0; k < b.nf; k++) {
        ee_unit *f = b.f[k];
        int cap = bk_capacity(f), held = bk_held(f), fx = unit_get_x(f), fy = unit_get_y(f);
        ee_unit *bs = unit_get_physics(f);
        REPLY(" [%d,%d %s, hp %d, holds %d of %d", fx, fy, bs && unit_get_finished(bs) ? "finished" : "going up", unit_get_hp(f), held, cap);
        if (!strcmp(verb, "load")) {
            int n = cap - held;
            if (n > b.nc - used) n = b.nc - used;
            if (n > 0 && bk_load(f, b.c + used, n)) { REPLY(", loads %d", n); used += n; }
        } else if (!strcmp(verb, "unload") && held > 0) {
            if (bk_unload(f)) REPLY(", unloads");
        }
        REPLY("]");
    }
    REPLY("\n");
}

// SHELTER ([ai_base] bunkers, default 1): citizens under fire - one of them hurt within 6 tiles in the last 30 s, or enemy
// soldiers within 8 tiles (raiders_near) - go into a fortress of theirs with room within 15 tiles instead of back to
// their drop site (citizens_safety), and come out to work again once nothing has hurt anyone within 10 tiles of the
// fortress for 45 s and no enemy soldiers stand near it. A fortress is built beside the drop site nearest the base
// point (without a fortress within 10 tiles) once enemy aircraft are possible (World War I, an enemy there too) or 3
// citizens were hurt in the last 2 minutes; a second one from 40 citizens. Not through the build queue: the build
// manager's placement (0x46580A) failed every fortress, at a spot or with its own search (0,0) - the foundation is laid
// as the landing's forward base lays its (0x62EF8A, which pays), when it has the wood, 3 gatherers build it, an
// unfinished one gets 2 again every minute (the game's builders mostly finish it first); its wood is held back from
// unit training while it waits (bunker_reserve). tests/scenarios/bombing_late.eeai (the bombing at 35 min, P3 off vs on):
// 20 seeds - workers 57.4 vs 66.2, buildings lost 2.4 vs 1.4, games down to 4-28 workers 4 vs 2 (10-seed batches
// gave 72.6, 61.7 and 52.2 against 53.1: noise); shelling and raid_ww1 (the fortress up only after minute 20 there:
// a WW1 start leaves little wood) about even.
internal int g_bunkers = 1;   // [ai_base] bunkers
internal int g_bk_res = 0xFFFF;   // [ai_base] bunker_reserve (below)
internal int bk_room(ee_player *pl);
internal struct shelf_st { int want; unsigned in, out; uint32_t t; } g_shelf[16];   // the shelf (below): a fortress more wanted, citizens in/out
internal struct { int nf, nall; ee_unit *f[8], *all[8]; int nq[8]; ee_unit *q[8][40]; uint32_t build_t, quiet_t[8]; unsigned in, out; int need_wood; uint32_t shelter[8]; int walk[8]; uint8_t store[8]; } g_bk[16];   // shelter: when citizens were sent in (the quiet release); walk: units on their way in; store: the shelf's
internal int bk_hurt_recent(unsigned pi, int x, int y, int r, uint32_t ms) {
    uint32_t now = game_time();
    int n = 0;
    for (int k = 0; k < HURT_N; k++) {
        typeof(&g_hurt[0].h[0]) h = &g_hurt[pi].h[k];
        if (h->t && now >= h->t && now - h->t <= ms && abs(h->x - x) <= r && abs(h->y - y) <= r) n++;
    }
    return n;
}
struct bk_own { ee_player *pl; int nf, nall; ee_unit *f[8], *all[8]; int nw; ee_unit *w[256]; };
internal ee_unit *bk_walking_into(ee_unit *u) {   // the holder unit u walks into (its load goal's part), else NULL
    ee_goal *g = unit_get_goal(u);
    ee_unit *h = g && goal_is_load(g) ? goal_get_holder(g) : NULL;
    return h != u ? h : NULL;   // the holder's own goal names itself
}
internal void bk_own_unit(ee_unit *u, void *ctx) {   // finished ones (f) and foundations too (all); who walks in (w)
    struct bk_own *o = ctx;
    if (unit_get_owner(u) != o->pl || unit_get_hp(u) <= 0) return;
    ee_unit *h = bk_walking_into(u);
    if (h && o->nw < 256) o->w[o->nw++] = h;
    if (bk_capacity(u) <= 0) return;
    ee_unit *bs = unit_get_physics(u);
    if (o->nall < 8) o->all[o->nall++] = u;
    if (bs && unit_get_finished(bs) && o->nf < 8) o->f[o->nf++] = u;
}
internal void bk_refresh(ee_player *pl, unsigned pi) {   // the player's fortresses (each pass of citizens_safety, every 4 s)
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
        uint8_t st = 0;
        for (int j = 0; j < g_bk[pi].nf; j++) if (g_bk[pi].f[j] == o.f[k]) { q = g_bk[pi].quiet_t[j]; sh = g_bk[pi].shelter[j]; st = g_bk[pi].store[j]; }
        g_bk[pi].quiet_t[k] = q;
        g_bk[pi].shelter[k] = sh;
        g_bk[pi].store[k] = st;
    }
    g_bk[pi].nf = o.nf;
    memcpy(g_bk[pi].f, o.f, sizeof o.f);
    memset(g_bk[pi].nq, 0, sizeof g_bk[pi].nq);
}
// citizen u is under fire: queued for the nearest fortress with room within 15 tiles - 1 if so
internal int bk_shelter(ee_player *pl, ee_unit *u) {
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    if (!g_bunkers || !g_bk[pi].nf) return 0;
    int ux = unit_get_x(u), uy = unit_get_y(u), best = -1, bd = 15 * 15 + 1;
    ee_unit *h = bk_walking_into(u);   // on its way into one of them already (the 4 s passes queued it again and again:
    for (int k = 0; h && k < g_bk[pi].nf; k++) if (g_bk[pi].f[k] == h) return 1;   // the fortress re-ordered, over-filled)
    if (bk_hurt_recent(pi, ux, uy, 6, 30000) < 1 && !raiders_near(pl, iv2(ux, uy), 8)) return 0;   // not under fire right now
    for (int k = 0; k < g_bk[pi].nf; k++) {
        ee_unit *f = g_bk[pi].f[k];
        int d = ivec2_dist2(unit_get_tile(f), iv2(ux, uy));
        if (d < bd && !g_bk[pi].store[k] && bk_held(f) + g_bk[pi].walk[k] + g_bk[pi].nq[k] < bk_capacity(f) && g_bk[pi].nq[k] < 40) { bd = d; best = k; }
    }
    if (best < 0) return 0;
    g_bk[pi].q[best][g_bk[pi].nq[best]++] = u;
    return 1;
}
internal void bk_flush(ee_player *pl, unsigned pi, uint32_t now) {   // the queued citizens walk in; quiet fortresses let them out
    for (int k = 0; k < g_bk[pi].nf; k++) {
        ee_unit *f = g_bk[pi].f[k];
        int fx = unit_get_x(f), fy = unit_get_y(f);
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
        if (bk_hurt_recent(pi, fx, fy, 10, 45000) || raiders_near(pl, iv2(fx, fy), 10)) { g_bk[pi].quiet_t[k] = 0; continue; }
        if (!g_bk[pi].quiet_t[k]) { g_bk[pi].quiet_t[k] = now ? now : 1; continue; }
        if (now - g_bk[pi].quiet_t[k] < 8000) continue;   // two quiet looks
        if (bk_unload(f)) {
            g_bk[pi].out += (unsigned)held;
            g_bk[pi].shelter[k] = 0;
            if (g_log) fprintf(g_log, "# bunker: P%u at %.1f min: the fortress at %d,%d lets %d out (quiet)\n", pi, now / 60000.0, fx, fy, held);
        }
    }
}
// builders for fortress b: the n nearest gathering citizens that aren't building it already
internal int bk_builders(ee_player *pl, ee_unit *b, int n) {
    void *cm = player_get_civ_mgr(pl);
    ee_resource_mgr *rm = cm ? civ_mgr_get_resource_mgr(cm) : NULL;
    int bx = unit_get_x(b), by = unit_get_y(b), have = 0, sent = 0;
    if (!rm) return 0;
    for (int r = 0; r < 7; r++) {   // already on it
        uint8_t *list = resource_mgr_list_at(rm, r);
        for (unsigned k = 0, first = deque_front(list), cnt = deque_count(list); k < cnt; k++) {
            ee_unit *u = deque_at(list, first + k); ee_goal *g = u ? unit_get_goal(u) : NULL;
            if (g && !strcmp(rtti_name(g), "EEUGBuildRepair@@") && goal_get_building(g) == b) have++;
        }
    }
    while (have + sent < n) {
        ee_unit *best = NULL;
        int bd = 0x7FFFFFFF;
        for (int r = 2; r < 7; r++) {
            uint8_t *list = resource_mgr_list_at(rm, r);
            for (unsigned k = 0, first = deque_front(list), cnt = deque_count(list); k < cnt; k++) {
                ee_unit *u = deque_at(list, first + k);
                if (!citizen_free(u, 0)) continue;
                int d = ivec2_dist2(unit_get_tile(u), iv2(bx, by));
                if (d < bd) { bd = d; best = u; }
            }
        }
        if (!best) break;
        void *mem = goal_build_alloc();
        if (!mem) break;
        ee_goal *goal = build_goal_init(mem, pl, b, 1);
        if (!unit_goal(best, goal, 0, 0x1F51)) break;   // with the unit type's vf29 (+0x74) as the game's assignment
        sent++;
    }
    return sent;
}
internal void bk_build(ee_player *pl, unsigned pi, uint32_t now) {
    if (g_bk[pi].build_t && now >= g_bk[pi].build_t && now - g_bk[pi].build_t < 60000) return;
    g_bk[pi].build_t = now ? now : 1;
    int e = current_epoch(pl);
    if (e < 2 || e >= 14) return;
    for (int k = 0; k < g_bk[pi].nall; k++) {   // unfinished: builders
        ee_unit *bs = unit_get_physics(g_bk[pi].all[k]);
        if (!bs || !unit_get_finished(bs)) {
            int n = bk_builders(pl, g_bk[pi].all[k], 2);
            if (n && g_log) fprintf(g_log, "# bunker: P%u at %.1f min: %d more builders for the fortress at %d,%d\n", pi, now / 60000.0, n,
                                    unit_get_x(g_bk[pi].all[k]), unit_get_y(g_bk[pi].all[k]));
        }
    }
    void *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL; ee_resource_mgr *rm = cm ? civ_mgr_get_resource_mgr(cm) : NULL;
    if (!bm || !rm) return;
    int air = 0, cit = 0;
    for (int q = 1; q < 16 && !air; q++) {
        void *o = st_player(q);
        air = o && o != pl && is_enemy(pl, o) && current_epoch(o) >= 9 && e >= 9;
    }
    for (int t = 0; t < 7; t++) cit += FIELD(resource_mgr_list_at(rm, t), 0x10, int);
    int want = air || bk_hurt_recent(pi, 0, 0, 10000, 120000) >= 3 ? (cit >= 40 ? 2 : 1) : 0;
    if (g_bk_res >> pi & 1 && bk_room(pl) <= 5 && want < 1) want = 1;   // the reserve: near the population cap
    if (g_shelf[pi].want && want < g_bk[pi].nall + 1) want = g_bk[pi].nall + 1 < 8 ? g_bk[pi].nall + 1 : 8;   // the shelf: full
    g_bk[pi].need_wood = 0;
    if (g_bk[pi].nall >= want) return;
    // 350 wood (0x555710 also wants a free builder at that moment: it passed in 2 of 10 games before minute 20, the
    // game's own builders take the foundation up anyway)
    ee_unit_type *od = type_data(FORTRESS_TYPE);
    ee_tech_tree *tt = player_get_tech_tree(pl);
    ee_tech *tech = od && tt ? tech_tree_lookup(tt, unit_type_get_tech(od)) : NULL;
    int cost = tech && tech_get_data(tech) ? tech_data_get_cost(tech_get_data(tech), 3) : 350;   // the cost lives in its tech (wood)
    if ((player_stock_at(pl))[3] < cost) { g_bk[pi].need_wood = cost; return; }   // can't pay yet: saved for (bunker_reserve)
    // the drop site nearest the base point without a fortress within 10 tiles
    ivec2 base = build_mgr_get_base(bm), d = iv2(0, 0);
    int bd = 0x7FFFFFFF;
    drops_cached(pl);
    for (int i = 0; i < g_drops[pi].n; i++) {
        const struct drop *p = &g_drops[pi].d[i];
        int dd = ivec2_dist2(p->p, base), held_near = 0;
        for (int k = 0; k < g_bk[pi].nall && !held_near; k++)
            held_near = abs(unit_get_x(g_bk[pi].all[k]) - p->p.x) <= 10 && abs(unit_get_y(g_bk[pi].all[k]) - p->p.y) <= 10;
        if (!p->food && !held_near && dd < bd) { bd = dd; d = p->p; }
    }
    if (bd == 0x7FFFFFFF) return;
    for (int r = 4; r <= 9; r++)
        for (int ax = -r; ax <= r; ax++)
            for (int ay = -r; ay <= r; ay++) {
                int x = d.x + ax, y = d.y + ay;
                if ((abs(ax) != r && abs(ay) != r) || !can_place(pl, FORTRESS_TYPE, iv2(x, y)) || !can_place(pl, FORTRESS_TYPE, iv2(x + 1, y + 1)) ||
                    !can_place(pl, FORTRESS_TYPE, iv2(x - 1, y - 1)) || raiders_near(pl, iv2(x, y), 8)) continue;
                ee_unit *b = place_foundation(pl, FORTRESS_TYPE, x, y);
                int sent = b ? bk_builders(pl, b, 3) : 0;
                if (g_log) fprintf(g_log, "# bunker: P%u at %.1f min: fortress foundation at %d,%d %s, %d builders (%s, %d citizens, %d fortresses)\n", pi,
                                   now / 60000.0, x, y, b ? "laid" : "refused", sent, air ? "aircraft possible" : "citizens hurt", cit, g_bk[pi].nall);
                return;
            }
}
// the wood a fortress waits for, held back from unit training (as the fields' wood and a landing's transports)
internal void bunker_reserve(ee_player *pl, int *reserve) {
    if (g_bunkers && pl) reserve[3] += g_bk[(unsigned)player_get_index(pl) & 15].need_wood;
}

// RESERVE ([ai_base] bunker_reserve, a mask of players, default all; the playtester: "another use is to amass units at
// lower population limits, so losses can immediately be drawn from them"): units in a fortress don't count toward
// the population (0x556E11 adds them only when asked: (x, 1); the training's room uses (1, 0)). Every 10 s: with the
// map full (population at the cap - 1), up to 10 free land soldiers at home (in no group: 0x4707D8 on the combat
// manager, within 25 tiles of the base point; 4 stay out) go into the fortress with the most room, and training fills
// the map again; once the population is 4 below the cap with nothing loaded for a minute (losses), or enemy soldiers
// come within 20 tiles of it, the fullest fortress lets everyone out (not one where citizens shelter). The room the
// training sees (cap - in production - population) was no guide: its count of units in production ran up to 16-30
// while nothing came out, the reserve filled to 39 and never let anyone out - P3's losses fell (23 -> 14), its kills
// didn't rise. tests/scenarios/lowpop.eeai (unit_limit 60 = 20 a player, cap 30; 10 seeds, P3 off vs on): kills 21.3 vs 21.3,
// losses 23.1 vs 19.0, buildings lost 9.4 vs 9.5 - no clear gain: of 153 soldiers ordered in, about 30 were inside at
// once (the AI's own groups take free soldiers back), and its units-in-production count, not the cap, held training
// back - the mod's own orders queued with no room (fixed: train_room, econ.c). With that, 20 seeds, P3 off vs on (both
// with train_room): P3 kills 22.7 vs 28.8, losses 27.2 vs 23.2, P2's losses 21.7 vs 27.4. On.
internal unsigned g_bk_res_in[16], g_bk_res_out[16];
internal uint32_t g_bk_res_load_t[16];
internal int bk_room(ee_player *pl) {   // cap - population on the map (the fortresses' units left out)
    int cap = unit_limit(pl);
    return cap - player_population(pl, 0, 0);
}
struct bk_free { ee_player *pl; ee_combat_mgr *cm; ivec2 b; int n; ee_unit *u[64]; };
internal void bk_free_unit(ee_unit *u, void *ctx) {
    struct bk_free *f = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    if (unit_get_owner(u) != f->pl || unit_get_hp(u) <= 0 || !td || f->n >= 64) return;
    if (ivec2_dist2(unit_get_tile(u), f->b) > 25 * 25 || !is_soldier(td) || is_ship_or_air(td)) return;
    if (combat_mgr_unit_free(f->cm, unit_get_physics(u))) f->u[f->n++] = u;
}
internal void bk_reserve_tick(ee_player *pl, unsigned pi, uint32_t now) {
    static uint32_t last[16];
    if (!(g_bk_res >> pi & 1) || !g_bk[pi].nf) return;
    if (last[pi] && now >= last[pi] && now - last[pi] < 10000) return;
    last[pi] = now ? now : 1;
    int room = bk_room(pl), threat = -1;
    for (int k = 0; k < g_bk[pi].nf && threat < 0; k++) {
        ee_unit *fo = g_bk[pi].f[k];
        if (!g_bk[pi].shelter[k] && !g_bk[pi].store[k] && bk_held(fo) > 0 && raiders_near(pl, unit_get_tile(fo), 20)) threat = k;
    }
    if (room <= 1 && threat < 0) {
        void *cm2 = player_get_civ_mgr(pl);
        ee_build_mgr *bm = cm2 ? civ_mgr_get_build_mgr(cm2) : NULL;
        struct bk_free f = { pl, player_get_combat_mgr(pl), bm ? build_mgr_get_base(bm) : iv2(0, 0), 0, { 0 } };
        if (!f.cm || !bm) return;
        grid_each(bk_free_unit, &f);
        int best = -1, space = 0;
        for (int k = 0; k < g_bk[pi].nf; k++) {
            int s = bk_capacity(g_bk[pi].f[k]) - bk_held(g_bk[pi].f[k]) - g_bk[pi].walk[k];
            if (s > space && !g_bk[pi].store[k]) { space = s; best = k; }
        }
        int n = f.n - 4;
        if (n > 10) n = 10;
        if (n > space) n = space;
        if (best < 0 || n <= 0 || !bk_load(g_bk[pi].f[best], f.u, n)) return;
        g_bk_res_in[pi] += (unsigned)n;
        g_bk_res_load_t[pi] = now ? now : 1;
        if (g_log) fprintf(g_log, "# reserve: P%u at %.1f min: room %d - %d soldiers into the fortress at %d,%d (holds %d; %u in, %u out so far)\n", pi,
                           now / 60000.0, room, n, unit_get_x(g_bk[pi].f[best]), unit_get_y(g_bk[pi].f[best]), bk_held(g_bk[pi].f[best]),
                           g_bk_res_in[pi], g_bk_res_out[pi]);
    } else if (threat >= 0 || (room >= 4 && (!g_bk_res_load_t[pi] || now - g_bk_res_load_t[pi] >= 60000))) {
        int best = threat, held = threat >= 0 ? bk_held(g_bk[pi].f[threat]) : 0;
        for (int k = 0; k < g_bk[pi].nf && threat < 0; k++) {
            ee_unit *fo = g_bk[pi].f[k];
            int h = bk_held(fo);
            if (h > held && !g_bk[pi].shelter[k] && !g_bk[pi].store[k]) { held = h; best = k; }
        }
        if (best < 0) return;
        ee_unit *fo = g_bk[pi].f[best];
        if (!bk_unload(fo)) return;
        g_bk_res_out[pi] += (unsigned)held;
        if (g_log) fprintf(g_log, "# reserve: P%u at %.1f min: room %d%s - the fortress at %d,%d lets %d out (%u in, %u out so far)\n", pi, now / 60000.0,
                           room, threat >= 0 ? ", enemies near" : "", unit_get_x(fo), unit_get_y(fo), held, g_bk_res_in[pi], g_bk_res_out[pi]);
    }
}

// THE SHELF ([ai_base] shelve_citizens; the player: P6 sat at the population limit with 460 citizens and 100000+ of
// every resource, nothing it could train had room - fortresses are great for that, shelve unused citizens for later
// instead of deleting them): every 10 s, while the population stands at the cap (cap - population on the map <= 5, units
// queued waiting: the in production count alone is no guide - P6 in that save had 480 queued with 500 places free, its
// stables working flat out) and food, wood, gold and iron each lie at shelve_stock or more, up to 20 citizens go into
// a fortress of the shelf - the nearest gatherers (not builders) of the resource with the most in stock, never fewer
// than cap / 8 (at least 40) left out - and no new citizens are trained (hook_cit_wanted: no more than there are); with no room in the shelf's
// fortresses one more is built (up to 8). Out again, a fortress a time: once one of those four falls under half of
// shelve_stock, or the citizens out are under 3/4 of what is kept out (losses). The shelter and the reserve keep off
// the shelf's fortresses; enemies near one keep them in.
internal int g_shelve = 1, g_v_shelve = -1, g_shelve_stock = 5000;
#define SHELVE(idx) PV(idx, g_v_shelve, g_shelve)
internal int shelf_rich(const int *st, int k) { return st[2] >= k && st[3] >= k && st[5] >= k && st[6] >= k; }
internal int shelf_keep(ee_player *pl) { int k = unit_limit(pl) / 8; return k > 40 ? k : 40; }
internal int shelf_held(unsigned pi) {   // citizens on the shelf (its fortresses hold only them)
    int n = 0;
    for (int k = 0; k < g_bk[pi].nf; k++) if (g_bk[pi].store[k]) n += bk_held(g_bk[pi].f[k]);
    return n;
}
// hook_cit_wanted (citizens.c): while the shelf works or holds citizens, no more than there are - on the map or in it
internal int shelf_wanted(ee_player *pl, int n, int have) {
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    if (!SHELVE(pi) || !g_bunkers) return n;
    uint32_t now = game_time();
    int on = g_shelf[pi].t && now >= g_shelf[pi].t && now - g_shelf[pi].t < 30000;
    return (on || shelf_held(pi) > 0) && n > have ? have : n;
}
internal int shelf_citizens_out(ee_player *pl) {
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    ee_resource_mgr *rm = cm ? civ_mgr_get_resource_mgr(cm) : NULL;
    int n = 0;
    for (int r = 0; rm && r < 7; r++) n += deque_count(resource_mgr_list_at(rm, r));
    return n;
}
internal void shelf_tick(ee_player *pl, unsigned pi, uint32_t now) {
    static uint32_t last[16];
    if (!SHELVE(pi) || !g_bunkers || current_epoch(pl) < 2) { g_shelf[pi].want = 0; return; }
    if (last[pi] && now >= last[pi] && now - last[pi] < 10000) return;
    last[pi] = now ? now : 1;
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    ee_unit_mgr *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    ee_resource_mgr *rm = cm ? civ_mgr_get_resource_mgr(cm) : NULL;
    if (!um || !rm) return;
    int *st = player_stock_at(pl), keep = shelf_keep(pl), out = shelf_citizens_out(pl);
    int room = unit_limit(pl) - player_population(pl, 1, 0), queued = unit_mgr_get_in_production(um);
    g_shelf[pi].want = 0;
    if (!shelf_rich(st, g_shelve_stock / 2) || out < keep * 3 / 4) {   // out again: the fullest of the shelf's, if quiet
        int best = -1, held = 0;
        for (int k = 0; k < g_bk[pi].nf; k++) {
            ee_unit *fo = g_bk[pi].f[k];
            int h = bk_held(fo);
            if (!g_bk[pi].store[k]) continue;
            if (h <= 0 && !g_bk[pi].walk[k]) { g_bk[pi].store[k] = 0; continue; }   // empty: an ordinary fortress again
            if (h > held && !raiders_near(pl, unit_get_tile(fo), 15)) { held = h; best = k; }
        }
        if (best < 0 || !bk_unload(g_bk[pi].f[best])) return;
        g_shelf[pi].out += (unsigned)held;
        if (g_log) fprintf(g_log, "# shelf: P%u at %.1f min: %d citizens out of the fortress at %d,%d (out %d, keep %d, stock %d/%d/%d/%d; %u in, %u out so far)\n",
                           pi, now / 60000.0, held, unit_get_x(g_bk[pi].f[best]), unit_get_y(g_bk[pi].f[best]), out, keep, st[2], st[3], st[5], st[6],
                           g_shelf[pi].in, g_shelf[pi].out);
        return;
    }
    if (room > 5 || queued < 1 || !shelf_rich(st, g_shelve_stock) || out <= keep) return;
    g_shelf[pi].t = now ? now : 1;
    int best = -1, space = 0;   // a fortress of the shelf with room, else an empty one
    for (int k = 0; k < g_bk[pi].nf; k++) {
        ee_unit *fo = g_bk[pi].f[k];
        int s = bk_capacity(fo) - bk_held(fo) - g_bk[pi].walk[k];
        if (g_bk[pi].shelter[k] || (!g_bk[pi].store[k] && bk_held(fo) + g_bk[pi].walk[k] > 0)) continue;
        if (s > space + (g_bk[pi].store[k] ? -1 : 0)) { space = s; best = k; }
    }
    if (best < 0 || space < 5) { g_shelf[pi].want = 1; return; }
    int n = out - keep;
    if (n > 20) n = 20;
    if (n > space) n = space;
    int rich = 2;   // the resource with the most in stock: its gatherers first
    for (int r = 3; r < 7; r++) if (st[r] > st[rich]) rich = r;
    ee_unit *fo = g_bk[pi].f[best], *pick[20];
    ivec2 fp = unit_get_tile(fo);
    int np = 0;
    for (int pass = 0; pass < 2 && np < n; pass++)
        while (np < n) {
            ee_unit *bu = NULL;
            int bd = 60 * 60;
            for (int r = 2; r < 7; r++) {
                if ((pass == 0) != (r == rich)) continue;
                uint8_t *list = resource_mgr_list_at(rm, r);
                for (unsigned k = 0, first = deque_front(list), cnt = deque_count(list); k < cnt; k++) {
                    ee_unit *u = deque_at(list, first + k);
                    int d = u ? ivec2_dist2(unit_get_tile(u), fp) : 0x7FFFFFFF, dup = 0;
                    if (d >= bd || !citizen_free(u, 0) || bk_walking_into(u)) continue;
                    for (int j = 0; j < np && !dup; j++) dup = pick[j] == u;
                    if (!dup) { bd = d; bu = u; }
                }
            }
            if (!bu) break;
            pick[np++] = bu;
        }
    if (!np || !bk_load(fo, pick, np)) return;
    g_bk[pi].store[best] = 1;
    g_bk[pi].walk[best] += np;
    g_shelf[pi].in += (unsigned)np;
    if (g_log) fprintf(g_log, "# shelf: P%u at %.1f min: room %d - %d citizens into the fortress at %d,%d (holds %d; out %d, keep %d, stock %d/%d/%d/%d; %u in, %u out so far)\n",
                       pi, now / 60000.0, room, np, fp.x, fp.y, bk_held(fo), out, keep, st[2], st[3], st[5], st[6], g_shelf[pi].in, g_shelf[pi].out);
}

// a new game (the game start hook): the fortresses and the citizens queued for them are the last game's
internal void bunker_new_game(void) {
    memset(g_bk, 0, sizeof g_bk);
    memset(g_shelf, 0, sizeof g_shelf);
    memset(g_bk_res_in, 0, sizeof g_bk_res_in);
    memset(g_bk_res_out, 0, sizeof g_bk_res_out);
    memset(g_bk_out, 0, sizeof g_bk_out);
    memset(g_bk_res_load_t, 0, sizeof g_bk_res_load_t);
}
