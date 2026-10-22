// Defences: how the AI fortifies its base. Included by eeaimod.c after attack.c (uses its defence_stats, is_wall).

// DEFENCE LAYOUT (aimod.ini [all] defence_layout, default on). The game's AI decides when and how many towers to
// build (its building plan, cm 0x46CD18) and puts each where its spot finder (0x466ED4) finds room along paths from
// the base - observed: 4 bronze towers scattered one by one on the side away from the enemy, none at the mines. A
// player masses towers where the attacks come from and guards his outlying mines. Here the AI's own tower requests
// (build queue entries without a position) are placed instead:
//   front: towards the nearest enemy building this AI has seen (up to 2 enemies; nothing seen yet -> the map centre),
//          defence_distance (10) tiles from the base centre, slots 3 tiles apart across that direction - close
//          enough that the towers cover each other (range 5+);
//   outposts: one slot on the enemy side of every own drop site 12+ tiles from the base (settlements at mines).
//   Order: 3 at the first front, 3 at the second, the outposts, then 2 more per front.
// Each slot is checked with the game's own placement test (0x62E940, every tile of the footprint) within 2 tiles.
// WALLS (defence_walls, default on): the game's wall manager (cm+0x48) works on a ring around the base one segment at
// a time and in practice leaves one stray piece with a gate. Walls belong in front of towers: once 2 towers stand at a
// front, an 11-tile wall arc goes up 3 tiles in front of them (the wall manager's own 0x497C3F(from, to, type): the
// citizens get the same build-wall order a player gives). The ends stay open - own units walk round, attackers on
// foot must break through or walk the length of it under tower fire.
internal int g_def = 1, g_def_dist = 10, g_def_walls = 1, g_def_more = 1;
internal int g_aa = 1, g_aa_ahead = 1, g_aa_ahead_b = 3;   // [all] anti_air, aa_ahead, aa_ahead_buildings (below)
internal int g_hold = 0;   // [all] hold_town (below)
internal uint8_t g_def_off[16];   // per player, pipe "defence P 0|1" (A/B scenarios)
internal unsigned g_def_towers[16], g_def_walls_built[16];
struct front { int fx, fy, enemy; float dx, dy; int wall_tries; uint32_t wall_t; };
internal struct def_plan {
    ee_player *pl; uint32_t t; int bx, by, nf, nout;
    struct front f[2];
    int out[8][2];
    struct { int x, y; uint32_t t; } pend[8];   // slots given out recently (the construction may not show yet)
} g_plan[16];

internal unsigned area_id(int x, int y, int kind);   // forces.c: 1 the sea, 2 the land there (0 none)
internal const enum ut_name k_tower_line[] = { UT_TOWER_LINE };   // oldest first
internal int tower_newest(ee_player *pl) {   // the best tower the game lets this player build now (0: none)
    for (int i = 6; i >= 0; i--)
        if (ut_has(g_ut[k_tower_line[i]]) && can_build(pl, g_ut[k_tower_line[i]])) return g_ut[k_tower_line[i]];
    return 0;
}
internal int bq_has(ee_build_mgr *bm, int type);   // attack.c
internal int tower_queued(ee_build_mgr *bm) {   // a tower of the line in the build manager's queues
    for (int i = 0; i < 7; i++) if (bq_has(bm, g_ut[k_tower_line[i]])) return 1;
    return 0;
}
internal int can_place(ee_player *pl, int type, int x, int y) {
    ee_unit_type *td = type_data(type);
    ee_map *map = game_map();
    int n = map ? (int)map_get_size(map) : 0, sz = td ? type_get_size(td) : 0;
    if (!td || x < 2 || y < 2 || x >= n - 2 || y >= n - 2) return 0;
    if (sz < 1) sz = 1;
    return can_place_area(pl, type, x, y, sz) != 0;
}

struct def_scan { ee_player *pl; int bx, by; int d2[16], x[16], y[16]; int towers, tx[64], ty[64]; int walls, wx[256], wy[256]; };
internal void def_scan_unit(ee_unit *u, void *ctx) {
    struct def_scan *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    ee_player *owner = unit_get_owner(u);
    int fam = type_get_family(td), x = unit_get_x(u), y = unit_get_y(u);
    if (fam != 2 && fam != 29) return;
    if (owner == s->pl) {
        if (is_tower_type(type_get_id(td)) && s->towers < 64) { s->tx[s->towers] = x; s->ty[s->towers++] = y; }
        else if (is_wall(type_get_id(td)) && s->walls < 256) { s->wx[s->walls] = x; s->wy[s->walls++] = y; }
        return;
    }
    if (!owner || !is_enemy(s->pl, owner) || !seen_by(u, s->pl)) return;
    unsigned pi = (unsigned)player_get_index(owner) & 15;
    int d2 = (x - s->bx) * (x - s->bx) + (y - s->by) * (y - s->by);
    if (d2 < s->d2[pi]) { s->d2[pi] = d2; s->x[pi] = x; s->y[pi] = y; }
}

internal struct def_scan g_scan;   // last scan (own towers), sim thread only
internal void def_refresh(ee_player *pl, unsigned idx, uint32_t now) {
    struct def_plan *p = &g_plan[idx];
    if (p->pl == pl && now >= p->t && now - p->t < 30000) return;
    if (p->pl != pl) memset(p, 0, sizeof *p);
    p->pl = pl; p->t = now;
    void *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
    p->nf = p->nout = 0;
    if (!bm || bm_get_base_x(bm) == -10000) return;
    p->bx = bm_get_base_x(bm); p->by = bm_get_base_y(bm);
    struct def_scan *s = &g_scan;
    memset(s, 0, sizeof *s);
    s->pl = pl; s->bx = p->bx; s->by = p->by;
    for (int i = 0; i < 16; i++) s->d2[i] = 0x7FFFFFFF;
    grid_each(def_scan_unit, s);
    struct front old[2];
    memcpy(old, p->f, sizeof old);
    for (int k = 0; k < 2; k++) {   // the two nearest enemies
        int best = -1;
        for (int i = 0; i < 16; i++)
            if (s->d2[i] != 0x7FFFFFFF && (best < 0 || s->d2[i] < s->d2[best]) && (k == 0 || i != p->f[0].enemy)) best = i;
        float dx, dy;
        if (best >= 0) { dx = (float)(s->x[best] - p->bx); dy = (float)(s->y[best] - p->by); }
        else if (k == 0) {   // nothing seen yet: towards the map centre
            ee_map *map = game_map();
            int n = map ? (int)map_get_size(map) : 0;
            dx = (float)(n / 2 - p->bx); dy = (float)(n / 2 - p->by);
        } else break;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 4.0f) break;
        struct front *f = &p->f[p->nf];
        f->dx = dx / len; f->dy = dy / len; f->enemy = best;
        f->fx = p->bx + (int)lroundf(f->dx * (float)g_def_dist);
        f->fy = p->by + (int)lroundf(f->dy * (float)g_def_dist);
        // keep the wall state while the front stays where it was
        f->wall_tries = 0; f->wall_t = 0;
        for (int j = 0; j < 2; j++)
            if (abs(old[j].fx - f->fx) <= 2 && abs(old[j].fy - f->fy) <= 2) { f->wall_tries = old[j].wall_tries; f->wall_t = old[j].wall_t; }
        p->nf++;
    }
    static int d[64][3];
    int nd = drop_sites(pl, d, 64);
    for (int i = 0; i < nd && p->nout < 8; i++) {
        int dx = d[i][0] - p->bx, dy = d[i][1] - p->by;
        if (d[i][2] || dx * dx + dy * dy < 144 || !p->nf) continue;
        p->out[p->nout][0] = d[i][0] + (int)lroundf(p->f[0].dx * 3.0f);
        p->out[p->nout][1] = d[i][1] + (int)lroundf(p->f[0].dy * 3.0f);
        p->nout++;
    }
}

// Slot i of the plan in build order, 0 when there is none.
internal int def_slot(struct def_plan *p, int i, int *x, int *y) {
    static const int first[3] = { 0, -1, 1 }, more[2] = { -2, 2 };
    for (int k = 0; k < p->nf; k++, i -= 3)
        if (i < 3) { *x = p->f[k].fx + (int)lroundf(-p->f[k].dy * 3.0f * first[i]); *y = p->f[k].fy + (int)lroundf(p->f[k].dx * 3.0f * first[i]); return 1; }
    if (i < p->nout) { *x = p->out[i][0]; *y = p->out[i][1]; return 1; }
    i -= p->nout;
    for (int k = 0; k < p->nf; k++, i -= 2)
        if (i < 2) { *x = p->f[k].fx + (int)lroundf(-p->f[k].dy * 3.0f * more[i]); *y = p->f[k].fy + (int)lroundf(p->f[k].dx * 3.0f * more[i]); return 1; }
    return 0;
}

// From the build manager's placement (econ.c hook_place): a position for a tower request without one.
internal int defence_spot(ee_player *pl, int type, int *ox, int *oy) {
    if (!g_def || !pl || !is_tower_type(type)) return 0;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    if (g_def_off[idx]) return 0;
    if (early_core_spot(pl, type, ox, oy)) return 1;   // early.c: the town's first towers by the Town Center
    uint32_t now = game_time();
    def_refresh(pl, idx, now);
    struct def_plan *p = &g_plan[idx];
    struct def_scan *s = &g_scan;
    if (s->pl != pl) { p->t = now - 60000; def_refresh(pl, idx, now); }   // the scan belongs to another AI
    int x, y;
    threat_refresh(pl, now);   // slots inside known enemy defences would cost the builders (attack.c)
    for (int i = 0; def_slot(p, i, &x, &y); i++) {
        int taken = 0;
        for (int t = 0; t < s->towers && !taken; t++) taken = abs(s->tx[t] - x) <= 2 && abs(s->ty[t] - y) <= 2;
        for (int t = 0; t < 8 && !taken; t++)
            taken = p->pend[t].t && now - p->pend[t].t < 90000 && abs(p->pend[t].x - x) <= 2 && abs(p->pend[t].y - y) <= 2;
        if (taken) continue;
        for (int r = 0; r <= 2; r++)
            for (int ax = -r; ax <= r; ax++)
                for (int ay = -r; ay <= r; ay++) {
                    if ((abs(ax) != r && abs(ay) != r) || route_covered(pl, p->bx, p->by, x + ax, y + ay) ||
                        raiders_near(pl, x + ax, y + ay, 10) ||   // an enemy army there: its builders would walk into it
                        !can_place(pl, type, x + ax, y + ay)) continue;   // the builders' way too
                    *ox = x + ax; *oy = y + ay;
                    int o = 0;
                    for (int t = 1; t < 8; t++) if (p->pend[t].t < p->pend[o].t) o = t;
                    p->pend[o].x = *ox; p->pend[o].y = *oy; p->pend[o].t = now ? now : 1;
                    g_def_towers[idx]++;
                    if (g_log) fprintf(g_log, "# defence: P%u tower %d at %d,%d (slot %d; base %d,%d, front %d,%d)\n",
                                       idx, type, *ox, *oy, i, p->bx, p->by, p->nf ? p->f[0].fx : -1, p->nf ? p->f[0].fy : -1);
                    return 1;
                }
    }
    return 0;
}

// Tile (x, y) within 1 tile of the segment a-b (sampled at 11 points).
internal int on_segment(int x, int y, const int *a, const int *b) {
    for (int j = 0; j <= 10; j++)
        if (abs(x - (a[0] + (b[0] - a[0]) * j / 10)) <= 1 && abs(y - (a[1] + (b[1] - a[1]) * j / 10)) <= 1) return 1;
    return 0;
}

// Walls: per AI every 20 s (econ_tick). Best wall type the player can build now (0x555710: available, affordable).
internal void defence_tick(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_def || !g_def_walls || g_def_off[idx] || (now >= last[idx] && now - last[idx] < 20000)) return;
    last[idx] = now;
    def_refresh(pl, idx, now);
    struct def_plan *p = &g_plan[idx];
    struct def_scan *s = &g_scan;
    if (s->pl != pl) return;
    void *cm = player_get_civ_mgr(pl), *wm = cm ? cm_get_wall_mgr(cm) : NULL;
    ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
    if (!wm || !bm) return;
    int *stock = player_stock_at(pl);
    // Towers of our own: the game's plan asks for few (P3: none in the first 15 minutes of a Bronze start, with 685
    // stone in stock - stone has hardly another use). While a slot of the plan is free and the stone allows (100 kept
    // back), one tower per minute goes into the build queue with its position; nothing while one is still queued.
    static uint32_t last_tower[16];
    if (g_def_more && (!last_tower[idx] || now < last_tower[idx] || now - last_tower[idx] >= 60000)) {
        int queued = tower_queued(bm), ttype = tower_newest(pl);
        int x, y, e = current_epoch(pl);
        if (!queued && ttype && stock[4] >= unit_cost(pl, ttype, 4) + 100 && e >= 0 && e < 14 &&
            defence_spot(pl, ttype, &x, &y)) {
            int entry[3] = { ttype, x, y };
            bm_queue_add(bm, e, entry);
            last_tower[idx] = now ? now : 1;
            if (g_log) fprintf(g_log, "# defence: P%u queues tower %d at %d,%d (stone %d)\n", idx, ttype, x, y, stock[4]);
            ai_say(pl, CHAT_TOWER, "[ai] extra tower (type %d) at %d,%d, stone %d", ttype, x, y, stock[4]);
        }
    }
    for (int k = 0; k < p->nf; k++) {
        struct front *f = &p->f[k];
        if (f->wall_tries >= 3 || (f->wall_t && now - f->wall_t < 180000)) continue;
        int ntow = 0;
        for (int t = 0; t < s->towers; t++)
            ntow += abs(s->tx[t] - f->fx) <= 7 && abs(s->ty[t] - f->fy) <= 7;
        if (ntow < 2) continue;
        static const enum ut_name wline[] = { UT_WALL_LINE };   // oldest first
        int wtype = 0;
        for (int i = 5; i >= 0 && !wtype; i--)   // the newest it can build
            if (ut_has(g_ut[wline[i]]) && can_build(pl, g_ut[wline[i]])) wtype = g_ut[wline[i]];
        if (!wtype) continue;
        int ok = 1;
        for (int r = 2; r < 7; r++) if (stock[r] < 11 * unit_cost(pl, wtype, r)) ok = 0;
        if (!ok) continue;
        int cx = f->fx + (int)lroundf(f->dx * 3.0f), cy = f->fy + (int)lroundf(f->dy * 3.0f);
        int a[2] = { cx + (int)lroundf(-f->dy * 5.0f), cy + (int)lroundf(f->dx * 5.0f) };
        int b[2] = { cx - (int)lroundf(-f->dy * 5.0f), cy - (int)lroundf(f->dx * 5.0f) };
        threat_refresh(pl, now);
        if (route_covered(pl, p->bx, p->by, a[0], a[1]) || route_covered(pl, p->bx, p->by, b[0], b[1]) ||
            route_covered(pl, p->bx, p->by, cx, cy)) continue;   // the builders would walk into enemy fire
        if (raiders_near(pl, a[0], a[1], 10) || raiders_near(pl, b[0], b[1], 10) || raiders_near(pl, cx, cy, 10))
            continue;   // or into an enemy army standing there
        // the arc on the map and on land, ends and middle: on an island map an arc into the water (or off the map)
        // crashed the game's wall placement (0x497C3F -> 0x636E30, a null tile object), twice on Small Islands
        ee_map *wmap = game_map();
        int mn = wmap ? (int)map_get_size(wmap) : 0, land = mn > 0;
        for (int q = 0; q <= 2 && land; q++) {
            int qx = q == 0 ? a[0] : q == 1 ? b[0] : cx, qy = q == 0 ? a[1] : q == 1 ? b[1] : cy;
            land = qx >= 3 && qy >= 3 && qx < mn - 3 && qy < mn - 3 && area_id(qx, qy, 2) && !area_id(qx, qy, 1);
        }
        if (!land) { f->wall_tries++; f->wall_t = now ? now : 1; continue; }
        int have = 0;   // wall pieces standing on the arc (then a new order only repairs)
        for (int w = 0; w < s->walls; w++) have += on_segment(s->wx[w], s->wy[w], a, b);
        if (have >= 8) continue;
        build_wall(wm, a, b, wtype);
        f->wall_tries++;
        f->wall_t = now ? now : 1;
        g_def_walls_built[idx]++;
        if (g_log) fprintf(g_log, "# defence: P%u wall %d from %d,%d to %d,%d in front of %d towers (front %d,%d)\n",
                           idx, wtype, a[0], a[1], b[0], b[1], ntow, f->fx, f->fy);
        ai_say(pl, CHAT_WALL, "[ai] walling the front at %d,%d (%d towers there)", f->fx, f->fy, ntow);
        return;   // one order per tick
    }
}

// COUNTER-TOWER (same switch): a player walks citizens up to an AI's mine or berries and puts a tower beside it - the
// AI let it be built and the tower killed its gatherers. Enemy defences under construction ([u+0x30]+0x340 = 0) it
// can see within 12 tiles of its buildings or citizens get the same answer as siege: the nearest soldiers (or, short
// of soldiers, its citizens) attack the builders - the enemy citizens whose EEUGBuildRepair holds it - spread over
// them, and the foundation only when nobody builds it. Finished towers are the siege weapons' job (attack.c siege_tick).
// COUNTER-SIEGE (aimod.ini [all] counter_siege, default on): catapults outrange towers (bronze: 7 against 5) and the
// AI just watched its towers being shelled. A player sends a few soldiers out at them. Every 2 s per AI: enemy siege
// weapons (and rams) it can see within their range + 4 tiles of one of its buildings are threats; its soldiers within
// 20 tiles of one - idle, marching, or fighting something more than 4 tiles away - attack it (up to 5 each, nearest
// first, priority like the siege controller's orders). Only when they are at least 2x + 3 the enemy soldiers within
// 8 tiles of the siege weapon: a sally into the escorting army (siege_towers: 14 defenders against 36) lost the base
// that towers and walls would have held.
internal int g_csiege = 1;
internal unsigned g_csiege_orders[16], g_csiege_skipped[16];
struct cs_scan {
    ee_player *pl;
    int nb, bx[256], by[256];           // own buildings
    int ns; ee_unit *sold[128];          // own soldiers
    int ne; ee_unit *esiege[16];         // enemy siege weapons seen
    int nx; int ex[256], ey[256];        // enemy soldiers (positions)
    int nc; int cx[128], cy[128];        // own citizens (positions)
    ee_unit *civ[128];                   // own citizens
    int nu; ee_unit *eu[128];            // enemy soldiers and citizens (militia)
    int nt; ee_unit *etow[16];           // enemy defences under construction, seen
};
internal void cs_unit(ee_unit *u, void *ctx) {
    struct cs_scan *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    ee_player *owner = unit_get_owner(u);
    if (unit_get_hp(u) <= 0 || !owner) return;
    int fam = type_get_family(td), type = type_get_id(td), x = unit_get_x(u), y = unit_get_y(u);
    if (owner == s->pl) {
        if ((fam == 2 || fam == 29) && s->nb < 256) { s->bx[s->nb] = x; s->by[s->nb++] = y; }
        else if (is_soldier(td) && s->ns < 128) s->sold[s->ns++] = u;
        else if (is_citizen_type(type) && s->nc < 128) { s->civ[s->nc] = u; s->cx[s->nc] = x; s->cy[s->nc++] = y; }
    } else if (is_enemy(s->pl, owner)) {
        ee_unit *bs = unit_get_physics(u);
        float r, a;
        if ((is_soldier(td) || is_citizen_type(type)) && s->nu < 128 && seen_by(u, s->pl)) s->eu[s->nu++] = u;
        if ((is_siege(type) || is_ram(type)) && s->ne < 16 && seen_by(u, s->pl)) s->esiege[s->ne++] = u;
        else if ((fam == 2 || fam == 29) && bs && !unit_get_finished(bs) && s->nt < 16 && defence_stats(u, &r, &a) &&
                 seen_by(u, s->pl)) s->etow[s->nt++] = u;
        else if (is_soldier(td) && s->nx < 256) { s->ex[s->nx] = x; s->ey[s->nx++] = y; }
    }
}
// CITIZEN MILITIA (aimod.ini [all] citizen_militia, default on). A player answers citizens that walk up to his mine or
// berries and start a tower there with his own citizens, and in the early ages his citizens beat off a small rush.
// The AI's citizens never fought. Every 2 s (with the counter-siege scan):
// - enemy citizens building a defence (EEUGBuildRepair, target +0x3C unfinished, a ground attack) within 12 tiles of
//   the AI's buildings or citizens, any epoch;
// - up to the Bronze Age: enemy units attacking (EEUGAttackUnit, target +0x30) a unit of the AI within 10 tiles of
//   its citizens;
// are attacked by the AI's citizens within 10 tiles, at most 3 per enemy and against at most 6 - only when those
// citizens' strength (hit points x attack) is 3 times that of the enemies within 6 tiles: Bronze Age soldiers (119 hp)
// killed 10 citizens of a count-based militia while losing 49 hit points. Gathering or idle citizens only; afterwards the assignment puts them back to work.
// [all] militia_estimate (default 0): 1 = the citizens decide with the stats estimate (dll/ai/stats.c) instead, in every
// epoch (the playtester: "a hard limit isn't right... tower builders without guards should always be engaged; troops
// in later ages should be avoided, but many units are faster than citizens - caught alone and running, they get
// chased down"): enemies without soldiers among them (builders, a foundation) when the citizens win (ratio >= 1);
// soldiers when the citizens win clearly (ratio >= 1.5), or when they are caught - the soldiers attack one of ours
// and are faster than citizens - and would still win (militia_caught_share 1: the share of the attackers destroyed;
// 0.33 had 3 citizens attack a Mk V tank 42 times - 0.2 of their damage gets through, a third of a tank is no use). A fallback ("when the
// citizens have a chance and no military is near to do the job"): not when own soldiers within 12 tiles win on their own.
internal int g_militia = 1, g_militia_est = 0;
internal float g_militia_share = 1.0f;   // [all] militia_caught_share: the share of the attackers caught citizens must still destroy
internal unsigned g_militia_caught[16];
internal unsigned g_militia_orders[16];
internal float unit_power(ee_unit *u) {   // hit points x attack (live stats), a rough fighting strength
    ee_unit_stats *st = unit_get_stats(u);
    float a = st ? stat_get_attack(st) : (float)type_get_attack(unit_get_type_data(u));
    return (float)unit_get_hp(u) * (a > 0 ? a : 0);
}
internal struct st_side g_mil_own, g_mil_foe;   // fight_estimate (dll/ai/stats.c)
internal int militia_answer(struct cs_scan *s, ee_unit *t, int *nfree) {   // citizens -> t; returns the number ordered
    int x = unit_get_x(t), y = unit_get_y(t), enemies = 0, ordered = 0;
    float epow = 0, cpow = 0, fast = 1e9f, cspeed = 0;
    unsigned pi = (unsigned)player_get_index(s->pl) & 15;
    int est = g_fight_est || g_militia_est || EARLY(pi, early_militia), soldiers = 0;
    if (est) { st_side_clear(&g_mil_own); st_side_clear(&g_mil_foe); }
    for (int k = 0; k < s->nu; k++) {
        ee_unit *e = s->eu[k];
        if (unit_get_hp(e) <= 0 || abs(unit_get_x(e) - x) > 6 || abs(unit_get_y(e) - y) > 6) continue;
        enemies++;
        epow += unit_power(e);
        if (est) st_side_add(&g_mil_foe, e);
        if (is_soldier(unit_get_type_data(e))) { soldiers++; if (st_speed(e) < fast) fast = st_speed(e); }
    }
    if (enemies < 1) {   // a foundation: no fight, only its hit points
        enemies = 1; epow = unit_power(t);
        if (est) st_side_add(&g_mil_foe, t);
    }
    if (enemies > 6) return 0;
    for (int c = 0, n = 0; c < s->nc && n < 3 * enemies; c++)
        if (s->civ[c] && abs(s->cx[c] - x) <= 10 && abs(s->cy[c] - y) <= 10) {
            cpow += unit_power(s->civ[c]); n++;
            if (est) st_side_add(&g_mil_own, s->civ[c]);
            if (st_speed(s->civ[c]) > cspeed) cspeed = st_speed(s->civ[c]);
        }
    // EARLY MILITIA ([all] early_militia; dll/ai/early.c): against soldiers the citizens fight only next to their own
    // soldiers or under their own towers, and only where the estimate has them win without a loss - in the first
    // epoch 1-6 Clubmen and Rock Throwers killed 39-48 citizens a game at the town, militia ordered 40-63 times a
    // game. Otherwise the citizens fall back under a tower (tower_bait) and the tower does the fighting.
    if (EARLY(pi, early_militia) && soldiers) {
        int cover = early_cover(s->pl, x, y);
        for (int k = 0; k < s->ns && !cover; k++)
            cover = s->sold[k] && abs(unit_get_x(s->sold[k]) - x) <= 12 && abs(unit_get_y(s->sold[k]) - y) <= 12;
        struct st_result r;
        if (cover && g_mil_own.n) st_estimate(&g_mil_own, &g_mil_foe, &r);
        // their damage while ours destroys them, in citizens: under one
        float lost = cover && g_mil_own.n && r.time[0] < 1e9f ? r.dps_contact[1] * r.time[0] * (float)g_mil_own.n / g_mil_own.hp : 99.0f;
        if (lost >= 1.0f) { g_early_mil_refused[pi]++; return 0; }
    }
    if (g_militia_est) {
        if (!g_mil_own.n) return 0;
        // a fallback (the playtester): soldiers near that win it on their own do the job instead
        static struct st_side mil_sold;
        st_side_clear(&mil_sold);
        for (int k = 0; k < s->ns; k++)
            if (s->sold[k] && abs(unit_get_x(s->sold[k]) - x) <= 12 && abs(unit_get_y(s->sold[k]) - y) <= 12) st_side_add(&mil_sold, s->sold[k]);
        if (mil_sold.n && st_ratio(&mil_sold, &g_mil_foe) >= 1.0f) return 0;
        float stand = st_ratio(&g_mil_own, &g_mil_foe);
        if (!soldiers) { if (stand < 1.0f) return 0; }   // builders, a foundation
        else if (stand < 1.5f) {
            struct st_result r;   // caught: running is no way out - fight if the citizens still do some good
            st_estimate(&g_mil_own, &g_mil_foe, &r);
            float share = r.time[0] < 1e9f ? r.time[1] / (r.time[0] > 0.1f ? r.time[0] : 0.1f) : 0;
            int attacking = unit_get_goal(t) && goal_is_attack(unit_get_goal(t));
            if (!attacking || fast <= cspeed * 1.1f || share < g_militia_share) return 0;
            g_militia_caught[(unsigned)player_get_index(s->pl) & 15]++;
        }
    } else {
        float stand = g_fight_est ? st_ratio(&g_mil_own, &g_mil_foe) : (epow > 0 ? cpow / epow : 1e6f);
        if (stand < 3.0f) return 0;   // citizens beat clubmen and builders, not real soldiers
    }
    for (int c = 0; c < s->nc && ordered < 3 * enemies; c++) {
        ee_unit *u = s->civ[c];
        if (!u || abs(s->cx[c] - x) > 10 || abs(s->cy[c] - y) > 10) continue;
        ee_goal *g = unit_get_goal(u);
        if (g && goal_is_attack(g)) { s->civ[c] = NULL; continue; }   // already fighting
        if (g && strcmp(rtti_name(g), "EEUGGather@@")) continue;   // building, walking: leave it
        if (unit_goal(u, goal_attack(u, t), 0, 0x1F40)) ordered++;
        s->civ[c] = NULL;   // one target per citizen per tick
    }
    *nfree -= ordered;
    return ordered;
}
internal void militia(ee_player *pl, unsigned idx, struct cs_scan *s, uint32_t now) {
    if (!g_militia || !s->nc || !s->nu) return;
    int early = g_militia_est || current_epoch(pl) <= 3, nfree = s->nc, total = 0, targets = 0;
    char what[160] = "";
    for (int k = 0; k < s->nu && nfree > 2; k++) {
        ee_unit *e = s->eu[k];
        ee_goal *g = unit_get_goal(e);
        ee_unit *t;
        if (!g || unit_get_hp(e) <= 0) continue;
        int x = unit_get_x(e), y = unit_get_y(e), why = 0;
        if (!strcmp(rtti_name(g), "EEUGBuildRepair@@") && (t = goal_get_building(g)) && unit_get_owner(t) != pl) {
            ee_unit *bs = unit_get_physics(t);
            float r, a;
            if (bs && !unit_get_finished(bs) && defence_stats(t, &r, &a)) why = 1;   // a tower going up
        } else if (early && goal_is_attack(g) && (t = goal_get_target(g)) && unit_get_owner(t) == pl)
            why = 2;   // attacking one of ours
        if (!why) continue;
        int close = 0;
        for (int b = 0; why == 1 && b < s->nb && !close; b++) close = abs(s->bx[b] - x) <= 12 && abs(s->by[b] - y) <= 12;
        for (int c = 0; c < s->nc && !close; c++) close = abs(s->cx[c] - x) <= (why == 1 ? 12 : 10) && abs(s->cy[c] - y) <= (why == 1 ? 12 : 10);
        if (!close) continue;
        int n = militia_answer(s, e, &nfree);
        if (n) {
            total += n; targets++;
            size_t l = strlen(what);
            if (l < sizeof what - 32)
                snprintf(what + l, sizeof what - l, " %s P%d type %d at %d,%d", why == 1 ? "builder" : "attacker",
                         player_get_index(unit_get_owner(e)), type_get_id(unit_get_type_data(e)), x, y);
        }
    }
    if (!total) return;
    g_militia_orders[idx] += (unsigned)total;
    if (g_log) fprintf(g_log, "# militia: P%u at %.1f min: %d citizens against %d enemies:%s (caught so far %u)\n", idx, now / 60000.0, total, targets, what, g_militia_caught[idx]);
}
internal void counter_siege(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_csiege || g_def_off[idx] || (now >= last[idx] && now - last[idx] < 2000)) return;
    last[idx] = now;
    static struct cs_scan s;
    memset(&s, 0, sizeof s);
    s.pl = pl;
    grid_each(cs_unit, &s);
    militia(pl, idx, &s, now);
    for (int i = 0; i < s.ne + s.nt; i++) {
        int tower = i >= s.ne;   // an enemy defence going up near our buildings or citizens (counter-tower)
        ee_unit *e = tower ? s.etow[i - s.ne] : s.esiege[i];
        ee_unit_type *etd = unit_get_type_data(e); ee_unit_stats *st = unit_get_stats(e);
        int x = unit_get_x(e), y = unit_get_y(e);
        float r = tower ? 12.0f : (st ? stat_get_range(st) : type_get_range(etd)) + 4.0f;
        int threat = 0;
        for (int b = 0; b < s.nb && !threat; b++)
            threat = (float)((s.bx[b] - x) * (s.bx[b] - x) + (s.by[b] - y) * (s.by[b] - y)) <= r * r;
        for (int c = 0; tower && c < s.nc && !threat; c++)
            threat = (float)((s.cx[c] - x) * (s.cx[c] - x) + (s.cy[c] - y) * (s.cy[c] - y)) <= r * r;
        if (!threat) continue;
        // a tower going up: its builders first (a player kills the citizens, the foundation is harmless without them)
        ee_unit *bl[8];
        ee_unit *site = tower ? field_site(e) : NULL;   // what their EEUGBuildRepair holds (+0x3C)
        int nbl = 0;
        for (int k = 0; tower && k < s.nu && nbl < 8; k++) {
            ee_goal *g = unit_get_goal(s.eu[k]);
            if (g && unit_get_hp(s.eu[k]) > 0 && (goal_get_building(g) == site || goal_get_building(g) == e) &&
                !strcmp(rtti_name(g), "EEUGBuildRepair@@")) bl[nbl++] = s.eu[k];
        }
        int guards = 0;
        for (int k = 0; k < s.nx; k++) guards += abs(s.ex[k] - x) <= 8 && abs(s.ey[k] - y) <= 8;
        // responders: nearest first, up to 5
        int pick[5], np = 0, avail = 0, already = 0;
        for (int k = 0; k < s.ns; k++) {
            ee_unit *u = s.sold[k];
            if (!u) continue;   // already ordered this tick
            ee_goal *g = unit_get_goal(u);
            int dx = unit_get_x(u) - x, dy = unit_get_y(u) - y, d2 = dx * dx + dy * dy;
            if (d2 > 400) continue;
            if (g && goal_is_attack(g)) {
                ee_unit *t = goal_get_target(g);
                int mine = t == e;
                for (int b = 0; b < nbl && !mine; b++) mine = t == bl[b];
                if (mine) { already++; s.sold[k] = NULL; continue; }
                if (t && abs(unit_get_x(t) - unit_get_x(u)) <= 4 && abs(unit_get_y(t) - unit_get_y(u)) <= 4)
                    continue;   // busy fighting close by
            }
            avail++;
            if (np < 5) pick[np++] = k;
            else {   // keep the 5 nearest
                int worst = 0, fd = -1;
                for (int j = 0; j < 5; j++) {
                    ee_unit *v = s.sold[pick[j]];
                    int vx = unit_get_x(v) - x, vy = unit_get_y(v) - y;
                    if (vx * vx + vy * vy > fd) { fd = vx * vx + vy * vy; worst = j; }
                }
                if (d2 < fd) pick[worst] = k;
            }
        }
        if (already >= 5) continue;
        if (avail + already < 2 * guards + 3) {
            int nfree = s.nc, n = 0;   // a tower going up and too few soldiers: the citizens go at the builders
            for (int b = 0; tower && g_militia && b < nbl && nfree > 2; b++) n += militia_answer(&s, bl[b], &nfree);
            if (tower && g_militia && !nbl) n += militia_answer(&s, e, &nfree);   // nobody building: the foundation
            if (n) g_militia_orders[idx] += (unsigned)n;
            else g_csiege_skipped[idx]++;
            continue;
        }
        int ordered = 0;
        for (int j = 0; j < np && already + ordered < 5; j++) {
            ee_unit *u = s.sold[pick[j]];
            ee_unit *t = nbl ? bl[(already + ordered) % nbl] : e;   // spread over the builders
            if (unit_goal(u, goal_attack(u, t), 0, 0x1F40)) ordered++;
            s.sold[pick[j]] = NULL;   // one target per unit per tick
        }
        g_csiege_orders[idx] += (unsigned)ordered;
        if (ordered)
            ai_say(pl, CHAT_COUNTER, "[ai] enemy siege at %d,%d - %d soldiers sent against it (%d guards)", x, y,
                   ordered + already, guards);
        if (ordered && g_log)
            fprintf(g_log, "# counter-%s: P%u at %.1f min: enemy type %d at %d,%d (%d guards, %d builders) <- %d soldiers (%d already on it)\n",
                    tower ? "tower" : "siege", idx, now / 60000.0, type_get_id(etd), x, y, guards, nbl, ordered, already);
    }
}

// HOME GUARD (aimod.ini [all] home_guard = soldiers per epoch, default 4; 0 = off): the unit manager (0x490AD6)
// only trains what is ordered into its request queues - mostly the next attack's group, ordered shortly before the
// attack. Between attacks nothing is trained: at minute 45 a Bronze Age AI had 10 soldiers, 25 requests in 5
// minutes and 1555 food + 1554 wood in its military account; a raid met 2-5 defenders. A player keeps a force at
// home. Every 10 s: fewer than max(6 + home_guard x epoch, home_guard_share x military room) own soldiers within 25
// tiles of the base (room = population cap x (1 - citizen share): 0.35 x 166 x 0.55 = 32) -> up to 5 more through
// the train hook (budget applies), type: the strongest trainable land soldier (substitution picks what the budget
// pays for).
internal int g_home_guard = 4;
internal float g_home_share = 0.35f;   // aimod.ini home_guard_share: of the military room (cap x (1 - citizen share))
internal unsigned g_home_trained[16];
struct home_scan { ee_player *pl; int bx, by, n; };
internal void home_unit(ee_unit *u, void *ctx) {
    struct home_scan *h = ctx;
    if (unit_get_owner(u) != h->pl || unit_get_hp(u) <= 0 || !is_soldier(unit_get_type_data(u))) return;
    int dx = unit_get_x(u) - h->bx, dy = unit_get_y(u) - h->by;
    h->n += dx * dx + dy * dy <= 625;
}
internal int home_soldiers(ee_player *pl, int bx, int by) {
    struct home_scan h = { pl, bx, by, 0 };
    grid_each(home_unit, &h);
    return h.n;
}
internal int home_soldiers_now(ee_player *pl) {
    void *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
    return bm && bm_get_base_x(bm) != -10000 ? home_soldiers(pl, bm_get_base_x(bm), bm_get_base_y(bm)) : -1;
}
internal void home_guard(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_home_guard || g_def_off[idx] || (now >= last[idx] && now - last[idx] < 10000)) return;
    last[idx] = now;
    void *cm = player_get_civ_mgr(pl), *um = cm ? cm_get_unit_mgr(cm) : NULL;
    ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
    if (!um || !bm || bm_get_base_x(bm) == -10000) return;
    int e = current_epoch(pl), target = 6 + g_home_guard * e;
    // and a share of the military room: population cap (vf31 - follows the human's population unless Ignorieren) x
    // (1 - citizen share +0xBDC)
    int cap = unit_limit(pl);
    float share = cfg_of(pl)->home_guard_share;   // a difficulty's or personality's own, else [all]
    int room = (int)((float)cap * (1.0f - player_get_citizen_share(pl)) * (share >= 0 ? share : g_home_share));
    if (room > target) target = room;
    // early_army (econ.c train_budget): no soldiers in the first epoch - towers and citizens; early_soldiers=N (the
    // alternative to the towers): N soldiers there from minute 2
    if (EARLY(idx, early_army) && e == 0) {
        target = now >= 120000 ? EARLY(idx, early_soldiers) : 0;
        if (!target) return;
    }
    int home = home_soldiers(pl, bm_get_base_x(bm), bm_get_base_y(bm));
    if (home >= target) return;
    int big[7] = { 0, 0, 1 << 20, 1 << 20, 1 << 20, 1 << 20, 1 << 20 }, got = 0;
    int type = substitute_soldier(pl, idx, big, 1, &got);   // the strongest the game can train now
    if (!type) return;
    int req[4] = { type, target - home < 5 ? target - home : 5, 0, 0 };
    unsigned n = 0;
    g_hg_call = 1;
    hook_train_units(um, NULL, req, (uintptr_t)&n);
    g_hg_call = 0;
    g_home_trained[idx] += n;
    if (n) ai_say(pl, CHAT_GUARD, "[ai] home guard %d of %d, training %u more", home, target, n);
    if (n && g_log) fprintf(g_log, "# home guard: P%u at %.1f min: %d of %d soldiers at home, %u more (type %d asked)\n",
                            idx, now / 60000.0, home, target, n, type);
}

// MUSTER ([all] muster, default 1; with fight_estimate the sides are compared with dll/ai/stats.c's estimate): an enemy army in the base got the AI's defenders one by one - every soldier the
// home guard trained walked into it alone, and the next one, and the next. A player would pull back what is left,
// gather, and strike once there is a force that can win. Every 3 s per AI: enemy soldiers within 22 tiles of the
// base (strength = hit points x attack) against its own soldiers within 35 and its towers within 22. Only when the
// fight at home is hopeless - at least 8 enemies, twice the strength of soldiers and towers: muster - every own
// soldier there goes to a rally point 18 tiles beyond the base, on the side away from the enemy's centre; new ones
// join as they are trained. Strike once the soldiers at the rally point are 1.3 x the enemy in the base: each attacks
// the nearest enemy soldier. The muster ends without a strike when the enemy is gone (fewer than 3), when soldiers
// and towers can stand up to it again, or when the enemy follows to the rally point a third time (then it fights
// where it is) - and no new muster for 30 s / 60 s. Citizens near the enemy go to the rally point too
// (citizens_safety, attack.c) - their drop site is where the fight is - unless enemy soldiers stand on the way: then
// 12 tiles in the clear direction nearest the rally point (Hedda's citizens, working east of her base, crossed it to
// the rally point in the west - through the player's army camped there).
// First version (any superiority, 5 enemies, no way out): assault scenario, 4 seeds - the defender gave up a base it
// would have held (9 attackers against 4 soldiers, towers and militia: off 88% alive, on 62%), the attackers followed
// to the rally point, its fleeing citizens stopped the economy and it surrendered.
internal int g_muster = 1;
internal struct { int on, moves; float rx, ry; uint32_t since, cool, cool_len; unsigned episodes, orders, strikes; } g_mus[16];
struct mus_scan {
    ee_player *pl; int bx, by, en, no, ne;
    float epow, opow, tpow, ex, ey;
    ee_unit *own[256], *enemy[128];
    struct st_side *os, *es;   // fight_estimate: own soldiers and towers, the enemies (dll/ai/stats.c)
};
internal struct mus_scan g_ms;
internal struct st_side g_ms_own, g_ms_foe, g_ms_rally;
internal void mus_unit(ee_unit *u, void *ctx) {
    struct mus_scan *m = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    if (unit_get_hp(u) <= 0 || !td) return;
    ee_player *o = unit_get_owner(u);
    int x = unit_get_x(u), y = unit_get_y(u), dx = x - m->bx, dy = y - m->by, d2 = dx * dx + dy * dy;
    float tr, ta;
    if (o == m->pl && d2 <= 22 * 22 && defence_stats(u, &tr, &ta)) {
        m->tpow += (float)unit_get_hp(u) * ta;
        if (m->os) st_side_add(m->os, u);
        return;
    }
    if (!is_soldier(td)) return;
    if (o == m->pl) {
        if (d2 > 35 * 35) return;
        if (m->no < 256) m->own[m->no++] = u;
        m->opow += unit_power(u);
        if (m->os) st_side_add(m->os, u);
    } else if (o && player_get_index(o) > 0 && is_enemy(m->pl, o) && d2 <= 22 * 22) {
        float p = unit_power(u);
        if (m->es) st_side_add(m->es, u);
        m->epow += p;
        m->ex += (float)x * p;
        m->ey += (float)y * p;
        m->en++;
        if (m->ne < 128) m->enemy[m->ne++] = u;
    }
}
internal void mus_rally(unsigned idx, struct mus_scan *s) {
    float cx = s->epow > 0 ? s->ex / s->epow : (float)s->bx + 1, cy = s->epow > 0 ? s->ey / s->epow : (float)s->by;
    float dx = (float)s->bx - cx, dy = (float)s->by - cy, l = sqrtf(dx * dx + dy * dy);
    if (l < 1.0f) { dx = 1.0f; dy = 0.0f; l = 1.0f; }
    float msz = (float)map_get_size(game_map());
    float rx = (float)s->bx + dx / l * 18.0f, ry = (float)s->by + dy / l * 18.0f;
    if (msz > 8) {
        rx = rx < 3 ? 3 : rx > msz - 4 ? msz - 4 : rx;
        ry = ry < 3 ? 3 : ry > msz - 4 ? msz - 4 : ry;
    }
    g_mus[idx].rx = rx;
    g_mus[idx].ry = ry;
}
internal int muster_rally(unsigned idx, float *x, float *y) {   // citizens_safety: the rally point while mustering
    if (idx >= 16 || !g_mus[idx].on) return 0;
    *x = g_mus[idx].rx;
    *y = g_mus[idx].ry;
    return 1;
}
internal ee_unit *mus_nearest_enemy(struct mus_scan *s, ee_unit *u) {
    ee_unit *best = NULL;
    int bd = 0x7FFFFFFF, x = unit_get_x(u), y = unit_get_y(u);
    for (int k = 0; k < s->ne; k++) {
        ee_unit *e = s->enemy[k];
        if (unit_get_hp(e) <= 0) continue;
        int dx = unit_get_x(e) - x, dy = unit_get_y(e) - y, d = dx * dx + dy * dy;
        if (d < bd) { bd = d; best = e; }
    }
    return best;
}
internal void muster(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_muster || g_def_off[idx] || (now >= last[idx] && now - last[idx] < 3000)) return;
    last[idx] = now;
    void *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
    if (!bm || bm_get_base_x(bm) == -10000) { g_mus[idx].on = 0; return; }
    struct mus_scan *s = &g_ms;
    memset(s, 0, sizeof *s);
    s->pl = pl;
    s->bx = bm_get_base_x(bm);
    s->by = bm_get_base_y(bm);
    if (g_fight_est) {
        st_side_clear(&g_ms_own); st_side_clear(&g_ms_foe);
        s->os = &g_ms_own; s->es = &g_ms_foe;
    }
    grid_each(mus_unit, s);
    // how the home side stands against the enemy in the base: summed hit points x attack, or the fight estimate
    float stand = g_fight_est ? st_ratio(&g_ms_own, &g_ms_foe) : (s->epow > 0 ? (s->opow + s->tpow) / s->epow : 1e6f);
    if (!g_mus[idx].on) {
        if (s->en < 8 || stand >= 0.5f ||
            (g_mus[idx].cool && now - g_mus[idx].cool < g_mus[idx].cool_len)) return;
        g_mus[idx].on = 1;
        g_mus[idx].moves = 0;
        g_mus[idx].since = now ? now : 1;
        g_mus[idx].episodes++;
        mus_rally(idx, s);
        if (g_log) fprintf(g_log, "# muster: P%u at %.1f min: %d enemy soldiers in the base (strength %.0f) against %d own "
                           "(%.0f) and towers (%.0f), %s %.2f - gathering at %.0f,%.0f\n", idx, now / 60000.0, s->en, s->epow, s->no, s->opow,
                           s->tpow, g_fight_est ? "estimate" : "ratio", stand, g_mus[idx].rx, g_mus[idx].ry);
        ai_say(pl, CHAT_GUARD, "[ai] base overrun (%d enemies), gathering at %.0f,%.0f", s->en, g_mus[idx].rx, g_mus[idx].ry);
    } else {
        if (s->en < 3 || stand >= 1.0f) {
            g_mus[idx].on = 0;
            g_mus[idx].cool = now ? now : 1;
            g_mus[idx].cool_len = 30000;
            if (g_log) fprintf(g_log, "# muster: P%u at %.1f min: %s - muster ends after %.1f min\n", idx, now / 60000.0,
                               s->en < 3 ? "the enemy is gone from the base" : "soldiers and towers can hold the base again",
                               (now - g_mus[idx].since) / 60000.0);
            return;
        }
        float rpow = 0;
        int rn = 0, near_rally = 0;
        if (g_fight_est) st_side_clear(&g_ms_rally);
        for (int k = 0; k < s->no; k++) {
            ee_unit *u = s->own[k];
            float dx = (float)unit_get_x(u) - g_mus[idx].rx, dy = (float)unit_get_y(u) - g_mus[idx].ry;
            if (dx * dx + dy * dy <= 100) {
                rpow += unit_power(u); rn++;
                if (g_fight_est) st_side_add(&g_ms_rally, u);
            }
        }
        float gathered = g_fight_est ? st_ratio(&g_ms_rally, &g_ms_foe) : (s->epow > 0 ? rpow / s->epow : 1e6f);
        if (gathered >= 1.3f) {   // strike
            int ordered = 0;
            for (int k = 0; k < s->no; k++) {
                ee_unit *u = s->own[k];
                ee_unit *t = mus_nearest_enemy(s, u);
                if (t && unit_goal(u, goal_attack(u, t), 0, 0x1F40)) ordered++;
            }
            g_mus[idx].on = 0;
            g_mus[idx].cool = now ? now : 1;
            g_mus[idx].cool_len = 30000;
            g_mus[idx].strikes++;
            if (g_log) fprintf(g_log, "# muster: P%u at %.1f min: strike - %d gathered (strength %.0f) against %d enemies "
                               "(%.0f), %s %.2f, %d ordered\n", idx, now / 60000.0, rn, rpow, s->en, s->epow,
                               g_fight_est ? "estimate" : "ratio", gathered, ordered);
            ai_say(pl, CHAT_GUARD, "[ai] strike with %d against %d", rn, s->en);
            return;
        }
        for (int k = 0; k < s->ne && !near_rally; k++) {
            float dx = (float)unit_get_x(s->enemy[k]) - g_mus[idx].rx, dy = (float)unit_get_y(s->enemy[k]) - g_mus[idx].ry;
            near_rally = dx * dx + dy * dy <= 100;
        }
        if (near_rally && ++g_mus[idx].moves >= 3) {   // it follows: no getting away - fight where it is
            g_mus[idx].on = 0;
            g_mus[idx].cool = now ? now : 1;
            g_mus[idx].cool_len = 60000;
            int ordered = 0;
            for (int k = 0; k < s->no; k++) {
                ee_unit *u = s->own[k];
                ee_unit *t = mus_nearest_enemy(s, u);
                if (t && unit_goal(u, goal_attack(u, t), 0, 0x1F40)) ordered++;
            }
            if (g_log) fprintf(g_log, "# muster: P%u at %.1f min: the enemy follows to the rally point - fighting where it is "
                               "(%d ordered)\n", idx, now / 60000.0, ordered);
            return;
        }
        if (near_rally) {
            mus_rally(idx, s);
            if (g_log) fprintf(g_log, "# muster: P%u at %.1f min: enemies at the rally point - now %.0f,%.0f\n", idx,
                               now / 60000.0, g_mus[idx].rx, g_mus[idx].ry);
        }
    }
    int ordered = 0;   // everyone who isn't there or on the way
    for (int k = 0; k < s->no; k++) {
        ee_unit *u = s->own[k];
        ee_goal *goal = unit_get_goal(u);
        float dx = (float)unit_get_x(u) - g_mus[idx].rx, dy = (float)unit_get_y(u) - g_mus[idx].ry;
        if (dx * dx + dy * dy <= 36) continue;
        if (goal && goal_is_location(goal) && fabsf(goal_get_dest_x(goal) - g_mus[idx].rx) < 2 &&
            fabsf(goal_get_dest_y(goal) - g_mus[idx].ry) < 2) continue;
        if (unit_goal(u, goal_move(g_mus[idx].rx, g_mus[idx].ry), 0, 0x1F40)) ordered++;
    }
    g_mus[idx].orders += ordered;
}

// HOLD THE TOWN ([all] hold_town, default 0). Soldiers carry a behaviour (unit +0x208: an EEUnitBehavior of the
// table at [0x8D2044] - 0 Scout, 1 Stand Ground, 2 Guard, 3 Harass, 4 Cautious, 5 Aggressive; set by the player's
// command EEMSetUnitBehavior 0x4BFF1A for units whose type data +0x621 allows it). Every soldier is Aggressive: it
// chases what it sees. A player's army camped outside an AI's town (artillery, outranging the towers, with its
// guards) drew the defenders out one by one - user's game: P2 lost 320 units and killed 18 in 33 minutes, P3 285
// and 43, while the home guard replaced each. Every 3 s per AI (not while mustering): enemy soldiers (siege too) it
// sees within 35 tiles of its base against its home soldiers there, by hit points x attack (not the towers: a
// soldier that walks out leaves their range). Home: within 35 tiles, also members of an attack group still forming
// (the attack commander takes nearly all soldiers into its groups early), not those marching out. Weaker than 1.2 x
// the enemy: the home soldiers pull back to 6 tiles beyond the base, away from the enemy, and Guard there (held
// where they stood, the artillery shelled them: 15 of 15 lost in 1.5 minutes). 1.3 x the enemy or more: a sortie -
// all Aggressive, each attacks its nearest enemy, together (then 60 s before the next). No enemy near: Aggressive.
// scripts/camp.eeai (8 artillery, 16 machine gunners, 6 tanks camped 15-18 tiles from P2's Town Center, P2 with 12
// machine gunners of its own), 6 pairs: P2 lost 22 vs 31 units, killed 21 vs 26, the camp kept 11 vs 7 of 30 -
// fewer losses, but the camp (within the towers' support there) survives more often. A camp that plays like the
// user's (scripts/camp2.eeai: reinforced every 2 minutes, pushing in every 4 and pulling back): 5 pairs, P2 killed
// 27 vs 36, lost 51 vs 59, kept 4 vs 17 citizens and its Town Center 2 vs 5 times, the camp grew to 53 vs 46 - pulled
// back, they leave the town to the pushes. Off.
internal struct { int holding; uint32_t sortie_t; unsigned holds, sorties, set; } g_hold_st[16];
internal int landing_has_unit(ee_player *pl, ee_unit *u);   // landing.c
internal ee_unit *g_asg_units[512];
internal int g_asg_n;
internal void asg_units(ee_player *pl) {   // the units of AI pl's attack subgroups (records +0x44)
    g_asg_n = 0;
    ee_combat_mgr *cbm = player_get_combat_mgr(pl); ee_attack_cmd *cmd = cbm ? am_get_commander(cbm) : NULL;
    if (!cmd) return;
    uint8_t *dq = ac_groups_at(cmd);
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        ee_attack_group *g = deque_at(dq, first + k);
        for (ee_attack_subgroup **sp = g ? ag_get_subgroups_begin(g) : NULL; sp && sp < ag_get_subgroups_end(g); sp++) {
            ee_asg_unit **b = *sp ? asg_get_units_begin(*sp) : NULL; ee_asg_unit **e = *sp ? asg_get_units_end(*sp) : NULL;
            for (ee_asg_unit **r = b; b && r < e && g_asg_n < 512; r++)
                if (*r && asgu_get_unit(*r)) g_asg_units[g_asg_n++] = asgu_get_unit(*r);
        }
    }
}
internal int set_behaviour(ee_unit *u, int b) {   // 1 when changed
    void **tab = behaviour_table();
    ee_unit_type *td = unit_get_type_data(u);
    if (!tab || !tab[b] || !td || !type_get_behaviours(td) || unit_get_behaviour(u) == tab[b]) return 0;
    unit_set_behaviour(u, tab[b]);
    return 1;
}
internal void hold_town(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_hold || g_def_off[idx] || (now >= last[idx] && now - last[idx] < 3000)) return;
    if (now < last[idx]) memset(&g_hold_st[idx], 0, sizeof g_hold_st[idx]);   // a new game
    last[idx] = now;
    if (g_mus[idx].on) return;   // the muster has them
    void *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
    void **tab = behaviour_table();
    if (!bm || bm_get_base_x(bm) == -10000 || !tab) return;
    struct mus_scan *s = &g_ms;
    memset(s, 0, sizeof *s);
    s->pl = pl;
    s->bx = bm_get_base_x(bm);
    s->by = bm_get_base_y(bm);
    grid_each(mus_unit, s);   // enemies within 22, own within 35: count the enemies within 35 here
    if (g_fight_est) { st_side_clear(&g_ms_own); st_side_clear(&g_ms_foe); }
    float epow = 0;
    int en = 0;
    ee_unit *enemy[128];
    {
        for (struct grid_it it = grid_it_box(s->bx, s->by, 35, 1); grid_it_next(&it); ) {
            ee_unit *u = it.u;
            ee_unit_type *td = it.td;
            if (unit_get_hp(u) <= 0) continue;
            int dx = unit_get_x(u) - s->bx, dy = unit_get_y(u) - s->by, type = type_get_id(td);
            ee_player *o = unit_get_owner(u);
            if (dx * dx + dy * dy > 35 * 35 || !o || o == pl || player_get_index(o) <= 0 || !is_enemy(pl, o)) continue;
            if ((!is_soldier(td) && !is_siege(type)) || !seen_by(u, pl)) continue;
            epow += unit_power(u);
            if (g_fight_est) st_side_add(&g_ms_foe, u);
            if (en < 128) enemy[en] = u;
            en++;
        }
    }
    asg_units(pl);
    ee_unit *home[256];
    int nh = 0;
    float hpow = 0;
    // home: every soldier within 35 tiles - also those of an attack group still forming there (the attack commander
    // takes nearly all soldiers into its groups long before they leave) - but not one marching out (a move order to a
    // point beyond 35 tiles); attack groups' members beyond 35 tiles: as the game has them (Aggressive)
    for (int j = 0; j < g_asg_n; j++) {
        ee_unit *u = g_asg_units[j];
        int dx = unit_get_x(u) - s->bx, dy = unit_get_y(u) - s->by;
        if (dx * dx + dy * dy > 35 * 35) set_behaviour(u, 5);
    }
    for (int k = 0; k < s->no; k++) {
        ee_unit *u = s->own[k];
        ee_goal *g = unit_get_goal(u);
        if (landing_has_unit(pl, u)) continue;   // the landing leads them (it ordered loaded transports at their docks against raiders)
        if (g && goal_is_location(g)) {
            float dx = goal_get_dest_x(g) - (float)s->bx, dy = goal_get_dest_y(g) - (float)s->by;
            if (dx * dx + dy * dy > 35.0f * 35.0f) { set_behaviour(u, 5); continue; }   // marching out
        }
        home[nh++] = u;
        hpow += unit_power(u);
        if (g_fight_est) st_side_add(&g_ms_own, u);
    }
    // the home soldiers against the enemies near (towers left out: they don't follow the soldiers out of their range)
    float stand = g_fight_est ? st_ratio(&g_ms_own, &g_ms_foe) : (epow > 0 ? hpow / epow : 1e6f);
    if (en == 0) {   // nothing near: as the game has them
        for (int k = 0; k < nh; k++) set_behaviour(home[k], 5);
        if (g_hold_st[idx].holding && g_log) fprintf(g_log, "# hold: P%u at %.1f min: no enemy near the town - soldiers free again\n", idx, now / 60000.0);
        g_hold_st[idx].holding = 0;
        return;
    }
    if (stand >= 1.3f && (!g_hold_st[idx].sortie_t || now - g_hold_st[idx].sortie_t >= 60000)) {
        int ordered = 0;
        for (int k = 0; k < nh; k++) {
            ee_unit *u = home[k];
            ee_unit *best = NULL;
            set_behaviour(u, 5);
            int bd = 0x7FFFFFFF, x = unit_get_x(u), y = unit_get_y(u);
            for (int j = 0; j < en && j < 128; j++) {
                if (unit_get_hp(enemy[j]) <= 0) continue;
                int dx = unit_get_x(enemy[j]) - x, dy = unit_get_y(enemy[j]) - y, d = dx * dx + dy * dy;
                if (d < bd) { bd = d; best = enemy[j]; }
            }
            if (best && unit_goal(u, goal_attack(u, best), 0, 0x1F40)) ordered++;
        }
        g_hold_st[idx].sortie_t = now ? now : 1;
        g_hold_st[idx].holding = 0;
        g_hold_st[idx].sorties++;
        if (g_log) fprintf(g_log, "# hold: P%u at %.1f min: sortie - %d soldiers (strength %.0f) against %d enemies (%.0f), %d ordered\n",
                           idx, now / 60000.0, nh, hpow, en, epow, ordered);
        return;
    }
    if (stand < 1.2f) {   // towers don't count: they don't follow the soldiers out of their range
        // held where they stood, the artillery that outranges them shelled them there (camp test: 15 of 15 lost in 1.5
        // minutes): back to a point 6 tiles beyond the base, away from the enemy's centre, and Guard there
        float cx = 0, cy = 0, w = 0;
        for (int j = 0; j < en && j < 128; j++) {
            float p = unit_power(enemy[j]);
            cx += (float)unit_get_x(enemy[j]) * p; cy += (float)unit_get_y(enemy[j]) * p; w += p;
        }
        float dx = w > 0 ? (float)s->bx - cx / w : 1.0f, dy = w > 0 ? (float)s->by - cy / w : 0.0f, l = sqrtf(dx * dx + dy * dy);
        if (l < 1.0f) { dx = 1.0f; dy = 0.0f; l = 1.0f; }
        float hx = (float)s->bx + 0.5f + dx / l * 6.0f, hy = (float)s->by + 0.5f + dy / l * 6.0f;
        int changed = 0;
        for (int k = 0; k < nh; k++) {
            ee_unit *u = home[k];
            ee_goal *g = unit_get_goal(u);
            changed += set_behaviour(u, 2);
            float ux = (float)unit_get_x(u) - hx, uy = (float)unit_get_y(u) - hy;
            if (ux * ux + uy * uy <= 16.0f) continue;
            if (g && goal_is_location(g) && fabsf(goal_get_dest_x(g) - hx) < 2 && fabsf(goal_get_dest_y(g) - hy) < 2) continue;
            unit_goal(u, goal_move(hx + (float)(k % 5) - 2.0f, hy + (float)(k / 5 % 5) - 2.0f), 0, 0x1F40);
        }
        g_hold_st[idx].set += (unsigned)changed;
        if (!g_hold_st[idx].holding) {
            g_hold_st[idx].holds++;
            if (g_log) fprintf(g_log, "# hold: P%u at %.1f min: %d enemies near the town (strength %.0f) against %d soldiers (%.0f) "
                               "and towers (%.0f) - holding (Guard)\n", idx, now / 60000.0, en, epow, nh, hpow, s->tpow);
        }
        g_hold_st[idx].holding = 1;
    }
}

// basemap P [r]: the map around AI player P's base (build manager +0x12C/+0x130), 2 characters per tile:
// T tower (own defence with a ground attack), W wall, G gate, C Town Center/Capitol/Settlement, b other own building,
// c citizen, s soldier, m mine (stone/gold/iron site), digits = other players' buildings/units (player index),
// o planned tower slot (defence layout).
struct basemap { ee_player *pl; int x0, y0, r; char *g; };
internal void basemap_unit(ee_unit *u, void *ctx) {
    struct basemap *m = ctx;
    int x = unit_get_x(u) - m->x0 + m->r, y = unit_get_y(u) - m->y0 + m->r, w = 2 * m->r + 1;
    if (x < 0 || y < 0 || x >= w || y >= w) return;
    ee_unit_type *td = unit_get_type_data(u);
    ee_player *owner = unit_get_owner(u);
    int type = type_get_id(td), fam = type_get_family(td);
    char ch, *cell = &m->g[y * w + x];
    float r, a;
    if (owner != m->pl) {
        if (!owner) return;
        int pi = player_get_index(owner);
        if (pi <= 0) return;   // nature / gaia
        ch = (char)('0' + pi % 10);
    } else if (is_wall(type)) ch = ut_role(type, UTR_GATE) ? 'G' : 'W';
    else if (defence_stats(u, &r, &a)) ch = 'T';
    else if (UT_IN(type, UT_TOWNS, UT_X_SINGLE_PLAYER_SETTLEMENT)) ch = 'C';
    else if (fam == 2 || fam == 29) ch = 'b';
    else if (is_citizen_type(type)) ch = 'c';
    else ch = 's';
    static const char prio[] = ".mcs0123456789bWGCT";   // what wins when several things share a tile
    if (strchr(prio, ch) > strchr(prio, *cell)) *cell = ch;
}
internal void cmd_basemap(char *out, const char *args) {
    int p = 0, r = 20;
    sscanf(args, "%d %d", &p, &r);
    if (r < 5) r = 5;
    if (r > 40) r = 40;
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    void *cm = pl ? player_get_civ_mgr(pl) : NULL;
    ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
    if (!bm) { REPLY("error: no AI player %d\n", p); return; }
    static char g[81 * 81];
    int w = 2 * r + 1;
    memset(g, '.', (size_t)(w * w));
    struct basemap m = { pl, bm_get_base_x(bm), bm_get_base_y(bm), r, g };
    ee_zone **zb = player_get_zones_begin(pl); ee_zone **ze = player_get_zones_end(pl);   // mines
    for (int z = 0; zb && zb + z < ze; z++)
        for (int t = 4; zb[z] && t < 7; t++) {
            ee_unit *s = zone_site(zb[z], t);
            if (!s) continue;
            int x = unit_get_x(s) - m.x0 + r, y = unit_get_y(s) - m.y0 + r;
            if (x >= 0 && y >= 0 && x < w && y < w) g[y * w + x] = 'm';
        }
    grid_each(basemap_unit, &m);
    unsigned pi = (unsigned)p & 15;   // planned tower slots of the defence layout: o
    if (g_plan[pi].pl == pl) {
        int sx, sy;
        for (int i = 0; def_slot(&g_plan[pi], i, &sx, &sy); i++) {
            int gx = sx - m.x0 + r, gy = sy - m.y0 + r;
            if (gx >= 0 && gy >= 0 && gx < w && gy < w && g[gy * w + gx] == '.') g[gy * w + gx] = 'o';
        }
    }
    REPLY("P%d base %d,%d (x down, y right)\n", p, m.x0, m.y0);
    for (int x = 0; x < w; x++) {
        char line[2 * 81 + 2];
        for (int y = 0; y < w; y++) { line[2 * y] = g[y * w + x]; line[2 * y + 1] = ' '; }
        line[2 * w] = 0;
        REPLY("%s\n", line);
    }
}


// defence P [0|1]: switch the defence layout off/on for one player (scenario A/B tests)
internal void cmd_defence(char *out, const char *args) {
    int p = -1, on = -1;
    sscanf(args, "%d %d", &p, &on);
    if (p < 0 || p > 15) { REPLY("error: defence P [0|1]\n"); return; }
    if (on >= 0) g_def_off[p] = !on;
    REPLY("defence P%d %s (towers placed %u, wall orders %u)\n", p, g_def_off[p] ? "off" : "on", g_def_towers[p], g_def_walls_built[p]);
}

// ENGINE GUARD: the damage effect (smoke/fire on damaged buildings, EEEDamagedEffect::vf19 0x4F8048) asks the
// object's model (unit vtable +0x54) for its damage points: 0x4D291F(index, state) looks up the model's map at +0x14C
// by state, then the map found there by the index. Twice (in ~60 runs, not reproducible, since the defence layout) the
// inner map had no head (+4 = 0) and the lookup crashed (0x4D3BD3, read 00000004). The call at 0x4F80A4 goes through
// a check: such a model gets "no damage points" (the effect then shows one), logged once per type with the unit.
__attribute__((used)) ee_unit *g_dmg_unit;   // the effect's unit: esi at the call site
internal uint8_t *rb_find(uint8_t *map, unsigned key) {   // MSVC map: +4 head; node +0 left, +4 parent, +8 right, +0xC key, +0x15 nil
    uint8_t *head = FIELD(map, 4, uint8_t *), *res, *n;
    if (!head) return NULL;
    res = head; n = FIELD(head, 4, uint8_t *);
    while (n && !n[0x15]) {
        if (FIELD(n, 0xC, unsigned) < key) n = FIELD(n, 8, uint8_t *);
        else { res = n; n = FIELD(n, 0, uint8_t *); }
    }
    return (res == head || key < FIELD(res, 0xC, unsigned)) ? NULL : res;
}
__attribute__((used)) int __fastcall hook_damage_points(uint8_t *model, void *edx, int index, unsigned state) {
    (void)edx;
    uint8_t *n = model ? rb_find(dmgmodel_map_at(model), state) : NULL, *sub = n ? FIELD(n, 0x10, uint8_t *) : NULL;
    if (n && (!sub || !FIELD(sub, 4, void *))) {
        static int logged;
        ee_unit *u = g_dmg_unit;
        ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
        if (g_log && logged < 20) {
            logged++;
            fprintf(g_log, "# engine guard: damage points of a model without them - unit type %d owner P%d at %d,%d, state %u index %d\n",
                    td ? type_get_id(td) : -1, u && unit_get_owner(u) ? player_get_index(unit_get_owner(u)) : -1,
                    u ? unit_get_x(u) : -1, u ? unit_get_y(u) : -1, state, index);
            fflush(g_log);
        }
        return 0;
    }
    return damage_points(model, index, state);
}
__attribute__((naked, used)) void damage_points_thunk(void) {
    __asm__("movl %esi, _g_dmg_unit\n\t"
            "jmp @hook_damage_points@16");
}

internal void defence_install(const char *aimod_ini, const char *autotest_ini) {
    g_def = cfg_int(aimod_ini, autotest_ini, 1, "defence_layout", 1);
    g_def_dist = cfg_int(aimod_ini, autotest_ini, 1, "defence_distance", 10);
    g_def_walls = cfg_int(aimod_ini, autotest_ini, 1, "defence_walls", 1);
    g_def_more = cfg_int(aimod_ini, autotest_ini, 1, "defence_towers", 1);
    g_csiege = cfg_int(aimod_ini, autotest_ini, 1, "counter_siege", 1);
    g_militia = cfg_int(aimod_ini, autotest_ini, 1, "citizen_militia", 1);
    g_militia_est = cfg_int(aimod_ini, autotest_ini, 1, "militia_estimate", 0);
    g_militia_share = cfg_share(aimod_ini, autotest_ini, 1, "militia_caught_share", 1, 0, 1);
    g_home_guard = cfg_int(aimod_ini, autotest_ini, 1, "home_guard", 4);
    g_home_share = cfg_share(aimod_ini, autotest_ini, 1, "home_guard_share", 0.35, 0, 1);
    g_muster = cfg_int(aimod_ini, autotest_ini, 1, "muster", 1);
    g_fight_est = cfg_int(aimod_ini, autotest_ini, 1, "fight_estimate", 0);
    g_aa = cfg_int(aimod_ini, autotest_ini, 1, "anti_air", 1);
    g_aa_ahead = cfg_int(aimod_ini, autotest_ini, 1, "aa_ahead", 1);
    g_aa_ahead_b = cfg_int(aimod_ini, autotest_ini, 1, "aa_ahead_buildings", 3);
    g_hold = cfg_int(aimod_ini, autotest_ini, 1, "hold_town", 0);
    BOOL guard = patch_damage_points(damage_points_thunk);
    if (g_log) fprintf(g_log, "# engine guard: damage points %s\n", guard ? "ok" : "FAILED");
    if (g_log) fprintf(g_log, "# defence: layout=%d distance=%d walls=%d own towers=%d counter-siege=%d home guard=%d/epoch, %.2f of the military room, fight_estimate=%d hold_town=%d\n", g_def, g_def_dist, g_def_walls, g_def_more, g_csiege, g_home_guard, g_home_share, g_fight_est, g_hold);
}

// ANTI-AIR ([all] anti_air, default 1). The game's unit choice hardly asks for anti-air: the user's planes overran
// both AIs' towns, one asked for an AA tank once in 5 minutes among hundreds of orders, built 2 88mm guns and lost its
// few AA tanks for good. Every 5 s: enemy aircraft it can see within 25 tiles of its drop sites (the most seen in the
// last 5 minutes counts). While there were any: it wants 2 + half their number in anti-air (at most 16), a quarter of
// that (1..4) as AA buildings next to the drop site nearest to where the aircraft were - the rest AA units, the
// newest of its epoch, up to 3 at a time every 15 s; one building at a time in the build queue, a minute apart.
// Orders as the siege's (priority: the stock above the epoch reserve, not the army's account); losses are replaced.
// AHEAD ([all] aa_ahead, default 1; the playtester: "they only build as reaction? then it's already too late, getting
// them up while the town is actively bombed is nearly impossible"): from World War I, once an enemy is there too, AA
// stands before any aircraft come - an AA building at each of the first aa_ahead_buildings (3) drop sites nearest the
// base point that has none within 7 tiles, and 2 AA units; enemy aircraft seen anywhere count (half of them, they fly
// fast), and the most seen is kept 10 minutes instead of 5. scenarios/bombing.eeai (10 seeds, P3 reactive vs ahead): 25
// aircraft and 4 bombers - workers 36 vs 52, buildings lost 4.3 vs 1.2, losses 25 vs 19; 12 aircraft (they did little
// harm) - workers 65 vs 57: the guns cost a little where the air threat stays small.
internal const struct { enum ut_name type; int epoch; } k_aa_units[] = {
    { UT_AA13_TANK_SKYWATCHER, 13 }, { UT_AA10_STINGER_SOLDIER, 12 }, { UT_AA10_TANK, 10 } };
internal const struct { enum ut_name type; int epoch; } k_aa_bldgs[] = { { UT_B_AA13_MISSILE_TOWER, 13 }, { UT_B_AA10_GUN_88MM, 10 } };
struct aa_scan { ee_player *pl; int nair, have_u, have_b, nall, nbx, bxy[16][2]; float axs, ays; };
internal void aa_scan_unit(ee_unit *u, void *ctx) {   // anti_air: own AA, enemy aircraft
    struct aa_scan *a = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    if (unit_get_hp(u) <= 0) return;
    int fam = type_get_family(td), t = type_get_id(td);
    ee_player *o = unit_get_owner(u), *pl = a->pl;
    if (o == pl) {
        for (size_t k = 0; k < sizeof k_aa_units / sizeof *k_aa_units; k++) a->have_u += t == g_ut[k_aa_units[k].type];
        for (size_t k = 0; k < sizeof k_aa_bldgs / sizeof *k_aa_bldgs; k++)
            if (t == g_ut[k_aa_bldgs[k].type]) {
                a->have_b++;
                if (a->nbx < 16) { a->bxy[a->nbx][0] = unit_get_x(u); a->bxy[a->nbx++][1] = unit_get_y(u); }
            }
        return;
    }
    if ((fam != 3 && fam != 7 && fam != 17) || !o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !seen_by(u, pl)) return;
    a->nall++;
    int x = unit_get_x(u), y = unit_get_y(u), d[2];
    if (!nearest_drop(pl, x, y, 0, d) || (d[0] - x) * (d[0] - x) + (d[1] - y) * (d[1] - y) > 625) return;
    a->nair++; a->axs += (float)x; a->ays += (float)y;
}
internal struct { uint32_t last, seen_t, train_t, build_t, all_t; int air, ax, ay, all; unsigned trained, queued; } g_aa_st[16];
internal void anti_air(ee_player *pl, unsigned idx, uint32_t now) {
    if (!g_aa || !pl) return;
    if (now < g_aa_st[idx].last) memset(&g_aa_st[idx], 0, sizeof g_aa_st[idx]);   // a new game
    if (g_aa_st[idx].last && now - g_aa_st[idx].last < 5000) return;
    g_aa_st[idx].last = now ? now : 1;
    int epoch = current_epoch(pl) + 1;   // tech ids count from 1
    if (epoch < 10) return;
    struct aa_scan a = { 0 };   // each unit once (grid_each): counted by hand per grid cell, an AA building in 2-4 cells
    a.pl = pl;                   // counted 2-4 times - fewer AA units trained, the AA buildings stopped early
    grid_each(aa_scan_unit, &a);
    int nair = a.nair, have_u = a.have_u, have_b = a.have_b, nall = a.nall, nbx = a.nbx;
    int (*bxy)[2] = a.bxy;
    float axs = a.axs, ays = a.ays;
    if (nair) {
        if (nair >= g_aa_st[idx].air || now - g_aa_st[idx].seen_t > 300000) g_aa_st[idx].air = nair;
        g_aa_st[idx].seen_t = now ? now : 1;
        g_aa_st[idx].ax = (int)(axs / (float)nair); g_aa_st[idx].ay = (int)(ays / (float)nair);
    } else if (g_aa_st[idx].seen_t && now - g_aa_st[idx].seen_t > (g_aa_ahead ? 600000u : 300000u)) g_aa_st[idx].air = 0;
    int ahead = 0, air = g_aa_st[idx].air;
    if (g_aa_ahead) {
        if (nall >= g_aa_st[idx].all || now - g_aa_st[idx].all_t > 600000) { g_aa_st[idx].all = nall; g_aa_st[idx].all_t = now ? now : 1; }
        if (g_aa_st[idx].all / 2 > air) air = g_aa_st[idx].all / 2;
        for (int q = 1; q < 16 && !ahead; q++) {
            void *o = st_player(q);   // every player (g_players: the AIs only)
            ahead = o && o != pl && is_enemy(pl, o) && current_epoch(o) + 1 >= 10;
        }
        if (!ahead && !air) return;
    } else if (!air) return;
    int want = 2 + (air + 1) / 2;
    if (want > 16) want = 16;
    int want_b = air ? 1 + want / 4 : 0;
    if (want_b > 4) want_b = 4;
    // ahead: the drop sites nearest the base point without an AA building within 7 tiles - the first of them gets the
    // next one, as many as aa_ahead_buildings
    int cover[2] = { 0, 0 }, uncovered = 0;
    if (ahead) {
        unsigned pi = (unsigned)player_get_index(pl) & 15;
        void *cm0 = player_get_civ_mgr(pl);
        ee_build_mgr *bm0 = cm0 ? cm_get_build_mgr(cm0) : NULL;
        int bx0 = bm0 ? bm_get_base_x(bm0) : 0, by0 = bm0 ? bm_get_base_y(bm0) : 0, used[64] = { 0 };
        drops_cached(pl);
        for (int pick = 0; pick < g_aa_ahead_b && pick < g_drops[pi].n; pick++) {
            int bi = -1, bd = 0x7FFFFFFF;
            for (int i = 0; i < g_drops[pi].n && i < 64; i++) {
                int *d = g_drops[pi].d[i], dd = (d[0] - bx0) * (d[0] - bx0) + (d[1] - by0) * (d[1] - by0);
                if (!used[i] && !d[2] && dd < bd) { bd = dd; bi = i; }
            }
            if (bi < 0) break;
            used[bi] = 1;
            int *d = g_drops[pi].d[bi], covered = 0;
            for (int k = 0; k < nbx && !covered; k++) covered = abs(bxy[k][0] - d[0]) <= 7 && abs(bxy[k][1] - d[1]) <= 7;
            if (!covered && !uncovered++) { cover[0] = d[0]; cover[1] = d[1]; }
        }
        if (have_b + uncovered > want_b) want_b = have_b + uncovered;
        if (want_b > 4 + g_aa_ahead_b) want_b = 4 + g_aa_ahead_b;
        if (want < 2 + want_b) want = 2 + want_b;
    }
    int want_u = want - (have_b < want_b ? have_b : want_b);
    void *cm = player_get_civ_mgr(pl), *um = cm ? cm_get_unit_mgr(cm) : NULL;
    ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
    // AA units
    if (um && have_u < want_u && (!g_aa_st[idx].train_t || now - g_aa_st[idx].train_t >= 15000)) {
        g_aa_st[idx].train_t = now ? now : 1;
        for (size_t k = 0; k < sizeof k_aa_units / sizeof *k_aa_units; k++) {
            if (k_aa_units[k].epoch > epoch || !ut_has(g_ut[k_aa_units[k].type])) continue;
            int n = want_u - have_u > 3 ? 3 : want_u - have_u;
            int req[4] = { g_ut[k_aa_units[k].type], n, 0, 0 };
            unsigned got = 0;
            g_priority_mil = 1;
            hook_train_units(um, NULL, req, (uintptr_t)&got);
            g_priority_mil = 0;
            if (!got) continue;
            g_aa_st[idx].trained += got;
            if (g_log) fprintf(g_log, "# anti-air: P%u at %.1f min trains %u x type %d (enemy aircraft %d, AA units %d of %d, AA buildings %d of %d)\n",
                               idx, now / 60000.0, got, req[0], g_aa_st[idx].air, have_u, want_u, have_b, want_b);
            break;
        }
    }
    // AA buildings: next to the drop site nearest to where the aircraft were
    int queued = 0;
    for (size_t k = 0; bm && k < sizeof k_aa_bldgs / sizeof *k_aa_bldgs; k++) queued |= bq_has(bm, g_ut[k_aa_bldgs[k].type]);
    int e = current_epoch(pl), d[2];
    if (uncovered && !g_aa_st[idx].air) { d[0] = cover[0]; d[1] = cover[1]; }   // ahead: no aircraft over a drop site yet
    if (bm && !queued && have_b < want_b && e >= 0 && e < 14 && (!g_aa_st[idx].build_t || now - g_aa_st[idx].build_t >= 60000) &&
        ((uncovered && !g_aa_st[idx].air) || nearest_drop(pl, g_aa_st[idx].ax, g_aa_st[idx].ay, 0, d))) {
        int bt = 0;
        for (size_t k = 0; k < sizeof k_aa_bldgs / sizeof *k_aa_bldgs && !bt; k++)
            if (k_aa_bldgs[k].epoch <= epoch && ut_has(g_ut[k_aa_bldgs[k].type]) && can_build(pl, g_ut[k_aa_bldgs[k].type]))
                bt = g_ut[k_aa_bldgs[k].type];
        for (int r = 3; bt && r <= 7; r++)
            for (int ax = -r; ax <= r; ax++)
                for (int ay = -r; ay <= r; ay++) {
                    if ((abs(ax) != r && abs(ay) != r) || !can_place(pl, bt, d[0] + ax, d[1] + ay) ||
                        raiders_near(pl, d[0] + ax, d[1] + ay, 8)) continue;
                    int entry[3] = { bt, d[0] + ax, d[1] + ay };
                    bm_queue_add(bm, e, entry);
                    g_aa_st[idx].build_t = now ? now : 1;
                    g_aa_st[idx].queued++;
                    if (g_log) fprintf(g_log, "# anti-air: P%u at %.1f min queues type %d at %d,%d (enemy aircraft %d near %d,%d, %d seen anywhere; AA buildings %d of %d%s)\n",
                                       idx, now / 60000.0, bt, d[0] + ax, d[1] + ay, g_aa_st[idx].air, g_aa_st[idx].ax, g_aa_st[idx].ay, g_aa_st[idx].all,
                                       have_b, want_b, uncovered && !g_aa_st[idx].air ? ", ahead" : "");
                    goto placed;
                }
        g_aa_st[idx].build_t = now ? now : 1;   // no room: try again in a minute
    }
placed:;
}

// UPGRADE STRAGGLERS: a researched line upgrade (effect kind 0x13, 0x5BA59D: e.g. 3051 guard tower 47 -> 461)
// converts the player's finished objects of the old type on the spot (0x62F192 cdecl(unit, player index, type data
// 0x9418A0[new type]) - hit points in proportion), and from then on the old type can't be built. Nothing converts what
// is finished later: a tower still a foundation or in the AI's build queue at that moment (the queue names the type:
// 47) was finished as the old type and stayed it (the Khan: 4 Paleo towers beside 4 Copper ones, 3051 researched).
// Every 30 s of game time, at the safe point between simulation steps: per AI and line (guard towers, walls, gates)
// the newest type it can build (0x555710), and every finished object of an older type of that line that it can no
// longer build is converted (still buildable = its upgrade not researched: the Copper tower comes with its own tech,
// the conversion of the old ones is the paid upgrade - paid_upgrades).
internal const enum ut_name k_upg_names[3][7] = {
    { UT_TOWER_LINE },        // guard towers Paleo..Digital
    { UT_WALL_LINE, UT_N },   // walls Copper..Digital
    { UT_GATE_LINE, UT_N },   // gates Copper..Digital
};
#define UPG(l, i) (k_upg_names[l][i] == UT_N ? -1 : g_ut[k_upg_names[l][i]])   // the type, -1 past the line's end
internal struct { ee_player *pl; int best[3]; uint8_t avail[3][7]; ee_unit *u[64]; int to[64], n; } g_upg;
internal void upg_scan(ee_unit *u, void *ctx) {
    (void)ctx;
    if (g_upg.n >= 64 || unit_get_owner(u) != g_upg.pl || unit_get_hp(u) <= 0) return;
    ee_unit_type *td = unit_get_type_data(u); ee_unit *st = unit_get_physics(u);
    if (!td || !st || !unit_get_finished(st)) return;   // finished only
    if (((int)unit_get_list_node(u)) && ((int)unit_get_chain(st))) return;  // the effect's own condition (0x5BA645)
    int t = type_get_id(td);
    for (int l = 0; l < 3; l++)
        for (int i = 0; i < 7 && UPG(l, i) >= 0; i++)
            if (UPG(l, i) == t && i < g_upg.best[l] && !g_upg.avail[l][i]) {
                g_upg.u[g_upg.n] = u;
                g_upg.to[g_upg.n++] = UPG(l, g_upg.best[l]);
                return;
            }
}
internal unsigned g_upg_done[16];
internal void upgrade_stragglers(void) {   // the safe point (control.c hook_sim_receive)
    static uint32_t last;
    uint32_t now = game_time();
    if (!now || (last && now >= last && now - last < 30000)) return;
    last = now;
    for (int p = 1; p < 16; p++) {
        ee_player *pl = g_players[p];
        if (!pl || player_get_out(pl) || is_vanilla(pl)) continue;
        int any = 0;
        for (int l = 0; l < 3; l++) {
            g_upg.best[l] = -1;
            for (int i = 0; i < 7; i++) {
                g_upg.avail[l][i] = UPG(l, i) >= 0 &&
                    can_build(pl, UPG(l, i));
                if (g_upg.avail[l][i]) g_upg.best[l] = i;
            }
            any |= g_upg.best[l] > 0;
        }
        if (!any) continue;
        g_upg.pl = pl;
        g_upg.n = 0;
        grid_each(upg_scan, NULL);   // collected first: the conversion replaces the object
        int pi = player_get_index(pl);
        for (int k = 0; k < g_upg.n; k++) {
            ee_unit_type *ntd = type_data(g_upg.to[k]);
            if (!ntd) continue;
            int from = type_get_id(unit_get_type_data(g_upg.u[k])), x = unit_get_x(g_upg.u[k]), y = unit_get_y(g_upg.u[k]);
            unit_convert(g_upg.u[k], pi, ntd);
            g_upg_done[p & 15]++;
            if (g_log) fprintf(g_log, "# upgrades: P%d at %.1f min: %d at %d,%d left behind by its upgrade - now %d\n", p,
                               now / 60000.0, from, x, y, g_upg.to[k]);
        }
    }
}

// The AI's build queue names types: an entry queued before its line's upgrade (the defence planner's "tower 47" in
// the Stone Age) was laid out afterwards as the old type, which the upgrade then never converts - a player's build
// menu only offers the new one. hook_place (econ.c) asks here: a tower, wall or gate type the AI can no longer
// build becomes the newest one of its line it can.
internal int upgraded_type(ee_player *pl, int type) {
    if (!pl) return type;
    for (int l = 0; l < 3; l++)
        for (int i = 0; i < 7 && UPG(l, i) >= 0; i++) {
            if (UPG(l, i) != type) continue;
            if (can_build(pl, type)) return type;
            for (int j = 6; j > i; j--)
                if (UPG(l, j) >= 0 && can_build(pl, UPG(l, j))) {
                    if (g_log) fprintf(g_log, "# upgrades: P%d places %d instead of %d (upgraded since it was queued)\n",
                                       player_get_index(pl), UPG(l, j), type);
                    return UPG(l, j);
                }
            return type;
        }
    return type;
}

// a new game (the game start hook): plans, scans and musters of the last game (its players, groups and
// times; a muster still on sent the citizens to the old rally point)
internal void defence_new_game(void) {
    memset(g_plan, 0, sizeof g_plan);
    memset(&g_scan, 0, sizeof g_scan);
    memset(g_def_towers, 0, sizeof g_def_towers);
    memset(g_def_walls_built, 0, sizeof g_def_walls_built);
    memset(g_csiege_orders, 0, sizeof g_csiege_orders);
    memset(g_csiege_skipped, 0, sizeof g_csiege_skipped);
    memset(g_militia_caught, 0, sizeof g_militia_caught);
    memset(g_militia_orders, 0, sizeof g_militia_orders);
    memset(g_home_trained, 0, sizeof g_home_trained);
    memset(g_mus, 0, sizeof g_mus);
    memset(g_hold_st, 0, sizeof g_hold_st);
    memset(g_aa_st, 0, sizeof g_aa_st);
    memset(g_upg_done, 0, sizeof g_upg_done);
}
