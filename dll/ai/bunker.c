// BUNKERS: the Fortress (type 672, family 29, tech 3072 - Copper Age) holds units like a transport (type +0x54C
// capacity, 40): inside they are off the map, safe, and (the playtester) don't count toward the population limit. The
// game's AI never builds or uses it. The playtester's uses: citizens out of bombing runs, a refuge for an outpost's
// gatherers until an army comes, a reserve at low population limits that replaces losses at once. A garrison is
// something else (the playtester): citizens put into a building for good, to upgrade it (type +0x550, its most: Town
// Center 15, Capitol 50, Settlement 5, Granary 8; the trigger attribute Garrison Level) - no shelter. Included by eeaimod.c
// after landing.c (the load goal as the transports': EEUGLoadTransport 0x61487F on the holder, its part for each unit).

#define FORTRESS_TYPE UT(B_FORTRESS)
internal int bk_capacity(ee_unit *u) {   // what a building holds as a transport does, 0 = none
    ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
    return td && type_get_family(td) == 29 ? type_get_transport_capacity(td) : 0;
}
internal int bk_held(ee_unit *u) { return units_inside(u); }   // units inside

struct bk_scan { ee_player *pl; ivec2 p; int r, nf, nc; ee_unit *f[8], *c[128]; };
internal void bk_unit(ee_unit *u, void *ctx) {
    struct bk_scan *b = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    int t = type_get_id(td);
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
internal int bk_unload(ee_unit *f, int x, int y) {
    ee_goal *g = goal_unload(f, x, y, 1);
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
    int pop1 = population(b.pl, 0, 0), pop2 = population(b.pl, 0, 1);
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
internal int g_bunkers = 1;   // [all] bunkers
internal int g_bk_res = 0xFFFF;   // [all] bunker_reserve (below)
internal int bk_room(ee_player *pl);
internal struct { int nf, nall; ee_unit *f[8], *all[8]; int nq[8]; ee_unit *q[8][40]; uint32_t build_t, quiet_t[8]; unsigned in, out; int need_wood; uint32_t shelter[8]; int walk[8]; } g_bk[16];   // shelter: when citizens were sent in (the quiet release); walk: units on their way in
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
        for (int j = 0; j < g_bk[pi].nf; j++) if (g_bk[pi].f[j] == o.f[k]) { q = g_bk[pi].quiet_t[j]; sh = g_bk[pi].shelter[j]; }
        g_bk[pi].quiet_t[k] = q;
        g_bk[pi].shelter[k] = sh;
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
    if (bk_hurt_recent(pi, ux, uy, 6, 30000) < 1 && !raiders_near(pl, ux, uy, 8)) return 0;   // not under fire right now
    for (int k = 0; k < g_bk[pi].nf; k++) {
        ee_unit *f = g_bk[pi].f[k];
        int d = ivec2_dist2(unit_get_tile(f), iv2(ux, uy));
        if (d < bd && bk_held(f) + g_bk[pi].walk[k] + g_bk[pi].nq[k] < bk_capacity(f) && g_bk[pi].nq[k] < 40) { bd = d; best = k; }
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
internal int bk_builders(ee_player *pl, ee_unit *b, int n) {
    void *cm = player_get_civ_mgr(pl);
    ee_resource_mgr *rm = cm ? cm_get_resource_mgr(cm) : NULL;
    int bx = unit_get_x(b), by = unit_get_y(b), have = 0, sent = 0;
    if (!rm) return 0;
    for (int r = 0; r < 7; r++) {   // already on it
        uint8_t *list = rm_list_at(rm, r);
        for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), cnt = FIELD(list, 0x10, unsigned); k < cnt; k++) {
            ee_unit *u = deque_at(list, first + k); ee_goal *g = u ? unit_get_goal(u) : NULL;
            if (g && !strcmp(rtti_name(g), "EEUGBuildRepair@@") && goal_get_building(g) == b) have++;
        }
    }
    while (have + sent < n) {
        ee_unit *best = NULL;
        int bd = 0x7FFFFFFF;
        for (int r = 2; r < 7; r++) {
            uint8_t *list = rm_list_at(rm, r);
            for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), cnt = FIELD(list, 0x10, unsigned); k < cnt; k++) {
                ee_unit *u = deque_at(list, first + k);
                if (!citizen_free(u, 0)) continue;
                int d = ivec2_dist2(unit_get_tile(u), iv2(bx, by));
                if (d < bd) { bd = d; best = u; }
            }
        }
        if (!best) break;
        void *mem = goal_build_alloc();
        if (!mem) break;
        ee_goal *goal = goal_build_init(mem, pl, b, 1);
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
    ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL; ee_resource_mgr *rm = cm ? cm_get_resource_mgr(cm) : NULL;
    if (!bm || !rm) return;
    int air = 0, cit = 0;
    for (int q = 1; q < 16 && !air; q++) {
        void *o = st_player(q);
        air = o && o != pl && is_enemy(pl, o) && current_epoch(o) >= 9 && e >= 9;
    }
    for (int t = 0; t < 7; t++) cit += FIELD(rm_list_at(rm, t), 0x10, int);
    int want = air || bk_hurt_recent(pi, 0, 0, 10000, 120000) >= 3 ? (cit >= 40 ? 2 : 1) : 0;
    if (g_bk_res >> pi & 1 && bk_room(pl) <= 5 && want < 1) want = 1;   // the reserve: near the population cap
    g_bk[pi].need_wood = 0;
    if (g_bk[pi].nall >= want) return;
    // 350 wood (0x555710 also wants a free builder at that moment: it passed in 2 of 10 games before minute 20, the
    // game's own builders take the foundation up anyway)
    ee_unit_type *od = type_data(FORTRESS_TYPE);
    ee_tech_tree *tt = player_get_tech_tree(pl);
    ee_tech *tech = od && tt ? tech_lookup(tt, type_get_tech(od)) : NULL;
    int cost = tech && tech_get_data(tech) ? techdata_get_cost(tech_get_data(tech), 3) : 350;   // the cost lives in its tech (wood)
    if ((player_stock_at(pl))[3] < cost) { g_bk[pi].need_wood = cost; return; }   // can't pay yet: saved for (bunker_reserve)
    // the drop site nearest the base point without a fortress within 10 tiles
    ivec2 base = bm_get_base(bm), d = iv2(0, 0);
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
                if ((abs(ax) != r && abs(ay) != r) || !can_place(pl, FORTRESS_TYPE, x, y) || !can_place(pl, FORTRESS_TYPE, x + 1, y + 1) ||
                    !can_place(pl, FORTRESS_TYPE, x - 1, y - 1) || raiders_near(pl, x, y, 8)) continue;
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
internal unsigned g_bk_res_in[16], g_bk_res_out[16];
internal uint32_t g_bk_res_load_t[16];
internal int bk_room(ee_player *pl) {   // cap - population on the map (the fortresses' units left out)
    int cap = unit_limit(pl);
    return cap - population(pl, 0, 0);
}
struct bk_free { ee_player *pl; ee_combat_mgr *cm; ivec2 b; int n; ee_unit *u[64]; };
internal void bk_free_unit(ee_unit *u, void *ctx) {
    struct bk_free *f = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    if (unit_get_owner(u) != f->pl || unit_get_hp(u) <= 0 || !td || f->n >= 64) return;
    if (ivec2_dist2(unit_get_tile(u), f->b) > 25 * 25 || !is_soldier(td) || is_ship_or_air(td)) return;
    if (cm_unit_free(f->cm, unit_get_physics(u))) f->u[f->n++] = u;
}
internal void bk_reserve_tick(ee_player *pl, unsigned pi, uint32_t now) {
    static uint32_t last[16];
    if (!(g_bk_res >> pi & 1) || !g_bk[pi].nf) return;
    if (last[pi] && now >= last[pi] && now - last[pi] < 10000) return;
    last[pi] = now ? now : 1;
    int room = bk_room(pl), threat = -1;
    for (int k = 0; k < g_bk[pi].nf && threat < 0; k++) {
        ee_unit *fo = g_bk[pi].f[k];
        if (!g_bk[pi].shelter[k] && bk_held(fo) > 0 && raiders_near(pl, unit_get_x(fo), unit_get_y(fo), 20)) threat = k;
    }
    if (room <= 1 && threat < 0) {
        void *cm2 = player_get_civ_mgr(pl);
        ee_build_mgr *bm = cm2 ? cm_get_build_mgr(cm2) : NULL;
        struct bk_free f = { pl, player_get_combat_mgr(pl), bm ? bm_get_base(bm) : iv2(0, 0), 0, { 0 } };
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
                           now / 60000.0, room, n, unit_get_x(g_bk[pi].f[best]), unit_get_y(g_bk[pi].f[best]), bk_held(g_bk[pi].f[best]),
                           g_bk_res_in[pi], g_bk_res_out[pi]);
    } else if (threat >= 0 || (room >= 4 && (!g_bk_res_load_t[pi] || now - g_bk_res_load_t[pi] >= 60000))) {
        int best = threat, held = threat >= 0 ? bk_held(g_bk[pi].f[threat]) : 0;
        for (int k = 0; k < g_bk[pi].nf && threat < 0; k++) {
            ee_unit *fo = g_bk[pi].f[k];
            int h = bk_held(fo);
            if (h > held && !g_bk[pi].shelter[k]) { held = h; best = k; }
        }
        if (best < 0) return;
        ee_unit *fo = g_bk[pi].f[best];
        if (!bk_unload(fo, unit_get_x(fo), unit_get_y(fo) + 3)) return;
        g_bk_res_out[pi] += (unsigned)held;
        if (g_log) fprintf(g_log, "# reserve: P%u at %.1f min: room %d%s - the fortress at %d,%d lets %d out (%u in, %u out so far)\n", pi, now / 60000.0,
                           room, threat >= 0 ? ", enemies near" : "", unit_get_x(fo), unit_get_y(fo), held, g_bk_res_in[pi], g_bk_res_out[pi]);
    }
}

// a new game (the game start hook): the fortresses and the citizens queued for them are the last game's
internal void bunker_new_game(void) {
    memset(g_bk, 0, sizeof g_bk);
    memset(g_bk_res_in, 0, sizeof g_bk_res_in);
    memset(g_bk_res_out, 0, sizeof g_bk_res_out);
    memset(g_bk_res_load_t, 0, sizeof g_bk_res_load_t);
}
