// TOWNS ON THE HOME CONTINENT ([ai_base] towns, bits; [autotest] v_towns). Included by eeaimod.c after expand.c (its
// laying and builders: place_foundation, the build goal, the resource manager's assignment); expand_tick runs it.
// The player 2026-10-27: "it has its own main city and a few outposts, it's not really expanding its empire with new
// towns and bases" - a player builds settlements as they expand, with production of their own, so the army is trained
// near where it's needed. The game's AI builds every building around its base point (the build manager's +base), and
// a Settlement by a mine at most (mines.c DROP); expand.c settles other islands only.
// The player: "overexpanding is deadly, it needs a purpose - a tactical operations center near an enemy or a resource
// node; in both cases the AI needs the resources and army to hold what it claims", ~32 tiles out at least; "a
// tactical base also needs defences, towers, a wall even".
// 1 RESOURCE TOWNS: every 30 s, when no town is going up, the mines near its drop sites full or worked out (fewer than
//   4 free places on the home continent's mines within 16 tiles of its drop sites) and its army at least the enemy's
//   it knows: the home continent's richest mine group that no other player's drop site is within 18 tiles of, none
//   of its own within 32, no enemy building it knows within 22 and no enemy defence covers - explored
//   (honest_vision), 32 to 90 tiles from its base, nearer counting more - gets a town: a Town Center (a Settlement when it can't pay one), then towers_per_town towers by it on the side of the
//   enemy it knows (else round it). towns_citizens citizens nearest the spot (wood and food gatherers and idle ones
//   first, never its miners or builders) walk there and build, 3 on each tower; once the drop site stands they gather
//   at its free mines (the game's assignment) - from then on the town is the game's economy's like every drop site.
//   Not while its army is outmatched or enemy soldiers are in its base, not before the Copper Age, at most towns_max.
//   A building destroyed is laid again (3 times); the town lost (no building of it left) - again after 8 minutes, the
//   spot's third loss: never again.
// 2 SETTLEMENT: the drop site a Settlement, never a Town Center.
// 8 FORWARD BASE (the player's tactical operations center): its army 1.5 x the enemy's it knows and 1.5 x the cost of
//   a Town Center, four towers and two production buildings in stock: a town on the line from its base to the
//   nearest enemy building it knows on the home continent, 35 tiles short of it and 32+ from its base and its drop
//   sites - a Town Center, four towers (two towards the enemy, two on the flanks), two production buildings of the
//   kind army_plan last picked (else Barracks), and once the two front towers stand an 11-tile wall 3 tiles in front
//   of them, straight at the nearest of the 8 directions, its ends open (defence.c's front wall: the wall manager's
//   0x497C3F, 3 tries 3 minutes apart, not into enemy fire or an enemy army). One at a time.
// 4 FORWARD (the player: settlements with 5 barracks and other production as they expanded, so parallel training was
//   always close to where it was needed): army_plan BUILD's next production building (army.c) goes up in the
//   standing town nearest to an enemy building it knows, when that town is 15+ tiles nearer to it than its base - by
//   the drop site, 3 of the town's citizens (or the nearest free ones) build it, laid again 3 times; one at a time,
//   at most 5 a town. Without such a town the game's build manager places it, as before.
// Measured (mirrored head-to-head, 12 games a cell, Continental map_scale large, 4 AIs; on the small test map no town:
// every mine group has somebody's drop site within 18 tiles), off - on: towns 1 from epoch 3, 60 min: soldiers +5.0
// (se 4.9), razed +4.3 (1.7), workers -9.8 (10.2); from epoch 5, 50 min: soldiers +20.3 (5.9), razed +2.3 (1.3),
// workers +13.7 (11.3) - 2-3 towns stood per AI by minute 45, their Town Centers and towers paid from the army. 3
// (Settlements, towers_per_town 1): epoch 3 soldiers +7.6 (4.5), workers +27.6 (10.1); epoch 5 soldiers +15.6 (5.5),
// kills +10.4 (5.3), workers +40.5 (7.3) - worse, a Settlement trains no citizens. Off: within an hour of AI against
// AI the mines were not what held the economy back; towns for production nearer the front (step 2) next.
// Few players on big maps (the player: mostly a feature for larger maps with fewer players), from epoch 5, 50 min,
// off - on: huge, 2 AIs: 1 soldiers +15.2 (7.0), razed +1.5 (0.9); 5 soldiers +18.4 (6.3), razed +2.3 (0.9); large, 3
// AIs: 1 soldiers +27.9 (7.4), losses -14.0 (3.1); 5 soldiers +20.2 (7.8), losses -10.1 (4.6), razed +2.3 (1.0) -
// the towns cost soldiers and lose units, FORWARD (production in the town nearest the enemy) doesn't win that back.
// With a purpose (1 = resource towns only when its mines are full, 8 = forward bases, 32+ tiles out; 9 against off,
// mirrored, 12 games a cell, from epoch 5, 90 min), off - on: huge 2 AIs soldiers +0.4 (3.7), losses -23.0 (17.9),
// workers +3.0 (3.8); large 3 AIs soldiers +3.2 (6.2), losses +9.5 (13.6), workers +8.6 (11.3) - no longer a loss, no
// gain either; 38 resource towns in the 48 games (about one per AI from minute 30), no forward base: in mirrored AI
// games neither side is 1.5 x the other. Off.
#define TN_MAX 6
#define TN_PROJ 12
enum { TK_DROP, TK_TOWER, TK_PROD, TK_N };
internal const char *const k_tk[TK_N] = { "drop site", "tower", "production building" };
struct tn_proj { ee_unit *b; int id, kind, type, want, state, tries, waits; ivec2 p; };   // want: a production building's type   // state 0 to come, 1 going up, 2 standing, 3 dropped
struct tn_town {
    ivec2 c, toward;                 // the mine group's middle; the side to face (the enemy it knows)
    int state;                       // 1 founding (citizens walking, buildings going up), 2 held, 0 lost
    int forward, wall_tries;         // a forward base (8), else a resource town; its wall's orders
    uint32_t wall_t;
    struct tn_proj proj[TN_PROJ]; int nproj;
    ee_unit *cit[12]; int cid[12], ncit; uint8_t sent_mine[12];   // sent_mine: assigned to the town's mines once - the game's economy's from then on
    uint32_t t, plan_t;
};
internal struct tn_st {
    uint32_t t, plan_t, log_t;
    struct tn_town town[TN_MAX]; int n;
    struct { ivec2 c; int losses; uint32_t again; } bad[16]; int nbad;
    unsigned plans, founded, held, lost, laid, built, rebuilt, orders, assigned, sent, unpaid, prods, walls;
    const char *why;
} g_tn[16];
internal int g_towns = 0, g_v_towns = -1, g_tn_max = 3, g_tn_cits = 6, g_tn_towers = 2;
#define TOWNS(idx) PV(idx, g_v_towns, g_towns)

internal PRINTF_LIKE(3, 4) void tn_say(unsigned idx, uint32_t now, const char *fmt, ...) {
    if (!g_log) return;
    va_list ap;
    va_start(ap, fmt);
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# towns: P%u at %.1f min: ", idx, now / 60000.0);
    vfprintf(g_log, fmt, ap);
    fputc('\n', g_log);
    LeaveCriticalSection(&g_lock);
    va_end(ap);
}
internal int tn_alive(struct tn_proj *p) {
    ee_unit *b = p->b;
    return b && unit_get_type_data(b) && unit_get_hp(b) > 0 && unit_get_id(b) == p->id;
}
internal int tn_is_drop(int t) { return UT_IN(t, UT_TOWNS, UT_X_SINGLE_PLAYER_SETTLEMENT); }

// the drop sites of every player and the enemy buildings pl knows (intel.c's memory), once per plan
struct tn_world { ivec2 drop[256]; int nd; ivec2 own[128]; int no; ivec2 foe[512]; int nf; };   // drop: other players', own: its own and its towns
internal void tn_world(ee_player *pl, unsigned idx, struct tn_world *w) {
    w->nd = w->nf = w->no = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        if (unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !tn_is_drop(unit_type_get_id(it.td))) continue;
        if (unit_get_owner(u) == pl) { if (w->no < 128) w->own[w->no++] = unit_get_tile(u); }
        else if (w->nd < 256) w->drop[w->nd++] = unit_get_tile(u);
    }
    for (int k = 0; k < TN_MAX; k++)   // its own towns going up count as its drop sites
        if (g_tn[idx].town[k].state && w->no < 128) w->own[w->no++] = g_tn[idx].town[k].c;
    struct intel *in = &g_intel[idx];
    for (int k = 0; k < in->n && w->nf < 512; k++) {
        struct intel_rec *r = &in->r[k];
        ee_player *o = r->owner > 0 && r->owner < 16 ? player_at(r->owner) : NULL;
        if (r->kind == 0 && o && is_enemy(pl, o)) w->foe[w->nf++] = iv2(r->x, r->y);
    }
}
// the home continent's best free mine group: its middle, its mines in o_n; 0 none
internal int g_tn_why[6];   // the last pick's mines: on the home continent, unexplored, too near / far from the base, by a drop site, by an enemy building, lost there or defended
internal int tn_site(ee_player *pl, unsigned idx, ivec2 base, unsigned hc, struct tn_world *w, ivec2 *o, int *o_n) {
    struct { ivec2 p; } m[512];
    int nm = 0;
    memset(g_tn_why, 0, sizeof g_tn_why);
    ee_zone **zb = player_get_zones_begin(pl); ee_zone **ze = player_get_zones_end(pl);
    for (int z = 0; zb && zb + z < ze && z < 256; z++) {
        ee_zone *zone = zb[z];
        void **sb = zone ? zone_get_sites_begin(zone) : NULL, **se = zone ? zone_get_sites_end(zone) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096 && nm < 512; k++) {
            ee_unit *s = sb[k];
            int r = s ? unit_get_resource(s) : 0;
            if (r < 4 || r > 6 || unit_get_amount(s) <= 0) continue;
            ivec2 sp = unit_get_tile(s);
            if (area_id(sp, 2) != hc) continue;
            g_tn_why[0]++;
            if (intel_looked_ago(pl, sp.x, sp.y) == 0xFFFFFFFFu) { g_tn_why[1]++; continue; }   // explored only
            m[nm++].p = sp;
        }
    }
    float best = 0;
    int found = 0;
    for (int i = 0; i < nm; i++) {
        ivec2 p = m[i].p;
        int db = (int)sqrtf((float)ivec2_dist2(p, base));
        if (db < 32 || db > 90) { g_tn_why[2]++; continue; }
        int ok = 1;
        for (int k = 0; k < w->nd && ok; k++) if (ivec2_dist2(p, w->drop[k]) < 18 * 18) ok = 0;
        for (int k = 0; k < w->no && ok; k++) if (ivec2_dist2(p, w->own[k]) < 32 * 32) ok = 0;
        if (!ok) { g_tn_why[3]++; continue; }
        for (int k = 0; k < w->nf && ok; k++) if (ivec2_dist2(p, w->foe[k]) < 22 * 22) ok = 0;
        if (!ok) { g_tn_why[4]++; continue; }
        for (int k = 0; k < g_tn[idx].nbad && ok; k++)
            if (ivec2_dist2(p, g_tn[idx].bad[k].c) < 12 * 12 && (g_tn[idx].bad[k].losses >= 3 || game_time() < g_tn[idx].bad[k].again)) ok = 0;
        if (!ok || threat_at(idx, p) > 0) { g_tn_why[5]++; continue; }
        int n = 0, sx = 0, sy = 0;
        for (int j = 0; j < nm; j++) if (ivec2_dist2(p, m[j].p) <= 10 * 10) { n++; sx += m[j].p.x; sy += m[j].p.y; }
        float sc = (float)n * 30.0f / (30.0f + (float)db);
        if (sc > best) { best = sc; *o = iv2(sx / n, sy / n); *o_n = n; found = 1; }
    }
    return found;
}
// a spot for type near c on the home continent, clear of the town's other buildings, the fewest defences on it
internal int tn_spot(ee_player *pl, unsigned idx, struct tn_town *t, int type, ivec2 c, unsigned hc, ivec2 *o) {
    ee_unit_type *td = type_data(type);
    int sz = td ? unit_type_get_size(td) : 1, found = 0;
    float best = 1e9f;
    if (sz < 1) sz = 1;
    for (int d = 1; d <= 10; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d ? 1 : 2 * d)) {
                ivec2 q = iv2(c.x + dx, c.y + dy);
                int ok = area_id(q, 2) == hc && !area_id(q, 1);
                for (int k = 0; k < t->nproj && ok; k++)   // a way between its buildings
                    if (t->proj[k].state && t->proj[k].state != 3 && abs(t->proj[k].p.x - q.x) < sz + 3 && abs(t->proj[k].p.y - q.y) < sz + 3) ok = 0;
                if (!ok || !can_place(pl, type, q)) continue;
                float sc = (float)d + 10.0f * threat_at(idx, q);
                if (sc < best) { best = sc; *o = q; found = 1; }
            }
    return found;
}
internal int tn_kind_type(ee_player *pl, int kind, int want) {
    if (kind == TK_PROD) return ld_can_build(pl, want) ? want : 0;
    if (kind == TK_TOWER) return ld_kind_type(pl, LDB_TOWER);
    if (!(TOWNS((unsigned)player_get_index(pl) & 15) & 2) && ld_can_build(pl, UT(B_TOWN_CENTER))) return UT(B_TOWN_CENTER);
    return ld_can_build(pl, UT(B_SETTLEMENT)) ? UT(B_SETTLEMENT) : 0;
}
// a citizen that may go: not mining, not building, not aboard, on the home continent
internal int tn_free_cit(ee_unit *u, unsigned hc) {
    if (unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || area_id(unit_get_tile(u), 2) != hc) return 0;
    ee_goal *g = unit_get_goal(u);
    const char *gn = g ? rtti_name(g) : "";
    if (!strcmp(gn, "EEUGBuildRepair@@")) return 0;
    if (!strcmp(gn, "EEUGGather@@")) { ee_unit *s = goal_get_site(g); int r = s ? unit_get_resource(s) : 0; if (r >= 4 && r <= 6) return 0; }
    return 1;
}
internal int tn_has_cit(unsigned idx, ee_unit *u) {
    for (int k = 0; k < TN_MAX; k++) {
        struct tn_town *t = &g_tn[idx].town[k];
        if (t->state) for (int j = 0; j < t->ncit; j++) if (t->cit[j] == u) return 1;
    }
    return 0;
}
internal void tn_take_cits(ee_player *pl, unsigned idx, struct tn_town *t, int n, unsigned hc);
// a forward base's production: the building of the soldier army_plan last picked (army.c), else Barracks
internal int tn_prod_type(ee_player *pl, unsigned idx) {
    int bt = g_ap[idx & 15].last_type ? ap_producer(pl, g_ap[idx & 15].last_type) : 0;
    return bt ? bt : UT(B_BARRACKS);
}
// free places at the mines near its drop sites (the home continent's, within 16 tiles of one of them)
internal int tn_free_slots(ee_player *pl, unsigned hc, struct tn_world *w) {
    unsigned cap = table_value(0x16);
    int free = 0;
    ee_zone **zb = player_get_zones_begin(pl); ee_zone **ze = player_get_zones_end(pl);
    for (int z = 0; zb && zb + z < ze && z < 256; z++) {
        ee_zone *zone = zb[z];
        void **sb = zone ? zone_get_sites_begin(zone) : NULL, **se = zone ? zone_get_sites_end(zone) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096; k++) {
            ee_unit *ms = sb[k];
            int r = ms ? unit_get_resource(ms) : 0;
            if (r < 4 || r > 6 || unit_get_amount(ms) <= 0) continue;
            ivec2 sp = unit_get_tile(ms);
            if (area_id(sp, 2) != hc) continue;
            int close = 0;
            for (int j = 0; j < w->no && !close; j++) close = ivec2_dist2(sp, w->own[j]) <= 16 * 16;
            unsigned g = site_gatherers(ms);
            if (close && g < cap) free += (int)(cap - g);
        }
    }
    return free;
}
// the forward base's spot: on the line from its base to the nearest enemy building it knows on the home continent,
// 35 tiles short of it, 32+ from its base and drop sites, no enemy defence on it
internal int tn_forward_site(unsigned idx, ivec2 base, unsigned hc, struct tn_world *w, ivec2 *o) {
    int bd = 0x7FFFFFFF;
    ivec2 e = base;
    for (int k = 0; k < w->nf; k++) {
        if (area_id(w->foe[k], 2) != hc) continue;
        int d = ivec2_dist2(base, w->foe[k]);
        if (d < bd) { bd = d; e = w->foe[k]; }
    }
    if (bd == 0x7FFFFFFF) return 0;
    float D = sqrtf((float)bd), at = D - 35.0f;
    if (at < 32.0f) return 0;   // the enemy within 67 tiles: its base is the front
    float ux = (float)(e.x - base.x) / D, uy = (float)(e.y - base.y) / D;
    for (int off = 0; off <= 8; off++)   // along the line, a little nearer home and to the sides
        for (int side = -off; side <= off; side += off ? 2 * off : 1) {
            float along = at - (float)off * 2.0f;
            ivec2 p = iv2(base.x + (int)lroundf(ux * along - uy * (float)side * 3.0f), base.y + (int)lroundf(uy * along + ux * (float)side * 3.0f));
            if (area_id(p, 2) != hc || area_id(p, 1) || threat_at(idx, p) > 0 || ivec2_dist2(p, base) < 32 * 32) continue;
            int ok = 1;
            for (int k = 0; k < w->no && ok; k++) if (ivec2_dist2(p, w->own[k]) < 32 * 32) ok = 0;
            for (int k = 0; k < w->nf && ok; k++) if (ivec2_dist2(p, w->foe[k]) < 22 * 22) ok = 0;
            if (ok) { *o = p; return 1; }
        }
    return 0;
}
// a forward base's wall: once its two front towers stand, 11 tiles straight across the enemy's side, 3 tiles in front
// of them (defence.c's front wall)
internal void tn_wall(ee_player *pl, unsigned idx, struct tn_town *t, uint32_t now) {
    if (!t->forward || t->wall_tries >= 3 || (t->wall_t && now - t->wall_t < 180000)) return;
    int front = 0;
    ivec2 a0 = t->c, a1 = t->c;
    for (int j = 0, n = 0; j < t->nproj; j++)
        if (t->proj[j].kind == TK_TOWER && n++ < 2 && t->proj[j].state == 2) { if (front++) a1 = t->proj[j].p; else a0 = t->proj[j].p; }
    if (front < 2) return;
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    void *wm = cm ? civ_mgr_get_wall_mgr(cm) : NULL;
    int *stock = player_stock_at(pl);
    if (!wm || !stock) return;
    static const enum ut_name wline[] = { UT_WALL_LINE };   // oldest first
    int wtype = 0;
    for (int i = 5; i >= 0 && !wtype; i--) if (ut_has(g_ut[wline[i]]) && can_build(pl, g_ut[wline[i]])) wtype = g_ut[wline[i]];
    if (!wtype) return;
    for (int r = 2; r < 7; r++) if (stock[r] < 11 * unit_cost(pl, wtype, r)) return;
    float dx = (float)(t->toward.x - t->c.x), dy = (float)(t->toward.y - t->c.y), l = sqrtf(dx * dx + dy * dy);
    if (l < 1) return;
    dx /= l; dy /= l;
    ivec2 mid = iv2((a0.x + a1.x) / 2, (a0.y + a1.y) / 2);
    int cx = mid.x + (int)lroundf(dx * 3.0f), cy = mid.y + (int)lroundf(dy * 3.0f);
    float px = -dy, py = dx, ax = fabsf(px), ay = fabsf(py);   // across the front, snapped to 8 directions
    int sx = ax > 2.414f * ay ? (px > 0 ? 1 : -1) : ay > 2.414f * ax ? 0 : (px > 0 ? 1 : -1);
    int sy = ay > 2.414f * ax ? (py > 0 ? 1 : -1) : ax > 2.414f * ay ? 0 : (py > 0 ? 1 : -1);
    int h = sx && sy ? 4 : 5;
    int a[2] = { cx + sx * h, cy + sy * h }, b[2] = { cx - sx * h, cy - sy * h };
    t->wall_t = now ? now : 1;
    t->wall_tries++;
    if (raiders_near(pl, iv2(cx, cy), 10) || raiders_near(pl, iv2(a[0], a[1]), 10) || raiders_near(pl, iv2(b[0], b[1]), 10)) return;
    ee_map *wmap = game_map();
    int mn = wmap ? (int)map_get_size(wmap) : 0, land = mn > 0;   // ends and middle on land and on the map (defence.c)
    for (int q = 0; q <= 2 && land; q++) {
        int qx = q == 0 ? a[0] : q == 1 ? b[0] : cx, qy = q == 0 ? a[1] : q == 1 ? b[1] : cy;
        land = qx >= 3 && qy >= 3 && qx < mn - 3 && qy < mn - 3 && area_id(iv2(qx, qy), 2) && !area_id(iv2(qx, qy), 1);
    }
    if (!land) return;
    build_wall(wm, a, b, wtype);
    g_tn[idx].walls++;
    tn_say(idx, now, "wall %d from %d,%d to %d,%d in front of the forward base at %d,%d", wtype, a[0], a[1], b[0], b[1], t->c.x, t->c.y);
}
internal void tn_found(ee_player *pl, unsigned idx, struct tn_st *s, ivec2 c, int nmines, struct tn_world *w, unsigned hc, uint32_t now, int forward) {
    struct tn_town *t = NULL;
    for (int k = 0; k < TN_MAX && !t; k++) if (!s->town[k].state) t = &s->town[k];
    if (!t) return;
    memset(t, 0, sizeof *t);
    t->c = c; t->state = 1; t->t = now ? now : 1;
    t->toward = c;   // the side to face: the nearest enemy building it knows, else none (round it)
    int bd = 0x7FFFFFFF;
    for (int k = 0; k < w->nf; k++) { int d = ivec2_dist2(c, w->foe[k]); if (d < bd) { bd = d; t->toward = w->foe[k]; } }
    t->proj[t->nproj++].kind = TK_DROP;
    for (int k = 0; k < (forward && g_tn_towers < 4 ? 4 : g_tn_towers) && t->nproj < TN_PROJ; k++) t->proj[t->nproj++].kind = TK_TOWER;
    t->forward = forward;
    for (int k = 0; forward && k < 2 && t->nproj < TN_PROJ; k++) { t->proj[t->nproj].kind = TK_PROD; t->proj[t->nproj].want = tn_prod_type(pl, idx); t->nproj++; }
    tn_take_cits(pl, idx, t, g_tn_cits, hc);
    s->founded++;
    tn_say(idx, now, "a %s at %d,%d (%d mines within 10 tiles, %d tiles from its base), %d citizens go", forward ? "forward base" : "resource town", c.x, c.y, nmines,
           (int)sqrtf((float)ivec2_dist2(c, build_mgr_get_base(civ_mgr_get_build_mgr(player_get_civ_mgr(pl))))), t->ncit);
}
// the town's citizens up to n: the nearest free ones
internal void tn_take_cits(ee_player *pl, unsigned idx, struct tn_town *t, int n, unsigned hc) {
    ivec2 c = t->c;
    struct { ee_unit *u; int d; } cand[64];
    int nc = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || !is_citizen_type(unit_type_get_id(it.td)) || !tn_free_cit(u, hc) || tn_has_cit(idx, u)) continue;
        int d = ivec2_dist2(unit_get_tile(u), c);
        if (nc < 64) { cand[nc].u = u; cand[nc].d = d; nc++; }
        else { int wj = 0; for (int j = 1; j < 64; j++) if (cand[j].d > cand[wj].d) wj = j; if (d < cand[wj].d) { cand[wj].u = u; cand[wj].d = d; } }
    }
    while (t->ncit < n && t->ncit < 12 && nc > 0) {
        int bj = 0;
        for (int j = 1; j < nc; j++) if (cand[j].d < cand[bj].d) bj = j;
        t->cit[t->ncit] = cand[bj].u; t->cid[t->ncit] = unit_get_id(cand[bj].u); t->sent_mine[t->ncit] = 0; t->ncit++;
        cand[bj] = cand[--nc];
    }
}
internal void tn_lost(unsigned idx, struct tn_st *s, struct tn_town *t, uint32_t now) {
    int k;
    for (k = 0; k < s->nbad && ivec2_dist2(s->bad[k].c, t->c) >= 12 * 12; k++) ;
    if (k == s->nbad && s->nbad < 16) { s->bad[k].c = t->c; s->bad[k].losses = 0; s->nbad++; }
    if (k < 16) { s->bad[k].losses++; s->bad[k].again = now + 480000; }
    s->lost++;
    tn_say(idx, now, "the town at %d,%d lost (%d times there)", t->c.x, t->c.y, k < 16 ? s->bad[k].losses : 0);
    t->state = 0;
}
// a town: its buildings (laid in order, laid again when destroyed), its builders, its citizens to its mines
internal void tn_run(ee_player *pl, unsigned idx, struct tn_st *s, struct tn_town *t, unsigned hc, uint32_t now) {
    int standing = 0, drop_ok = 0, any = 0, dropped = 0;
    for (int j = 0; j < t->nproj; j++) {
        struct tn_proj *p = &t->proj[j];
        if (p->state == 1 || p->state == 2) {
            if (!tn_alive(p)) {
                tn_say(idx, now, "%s %d at %d,%d destroyed%s", k_tk[p->kind], p->type, p->p.x, p->p.y, p->tries < 3 ? " - laid again" : "");
                p->b = NULL; p->state = p->tries < 3 ? 0 : 3;
                if (p->state == 0) s->rebuilt++;
                continue;
            }
            ee_unit *bs = unit_get_physics(p->b);
            if (p->state == 1 && bs && unit_get_finished(bs)) { p->state = 2; s->built++; tn_say(idx, now, "%s %d at %d,%d finished", k_tk[p->kind], p->type, p->p.x, p->p.y); }
            any = 1;
        }
        if (p->state == 2) standing++;
        if (p->state == 3) dropped++;
        if (p->kind == TK_DROP && p->state == 2) drop_ok = 1;
    }
    for (int j = 0; j < t->ncit; j++)   // its citizens alive
        if (!t->cit[j] || unit_get_hp(t->cit[j]) <= 0 || unit_get_id(t->cit[j]) != t->cid[j]) {
            t->cit[j] = t->cit[t->ncit - 1]; t->cid[j] = t->cid[t->ncit - 1]; t->sent_mine[j] = t->sent_mine[t->ncit - 1]; t->ncit--; j--;
        }
    if (!any && !t->ncit && now - t->t > 60000) { tn_lost(idx, s, t, now); return; }
    if (!t->proj[0].state && !t->proj[0].tries && now - t->t > 180000) {   // its first drop site not paid in 3 minutes: given up, not lost
        s->unpaid++;
        tn_say(idx, now, "the town at %d,%d given up: its drop site not paid in 3 minutes", t->c.x, t->c.y);
        int k;   // that spot not again for 5 minutes (no loss)
        for (k = 0; k < s->nbad && ivec2_dist2(s->bad[k].c, t->c) >= 12 * 12; k++) ;
        if (k == s->nbad && s->nbad < 16) { s->bad[k].c = t->c; s->bad[k].losses = 0; s->nbad++; }
        if (k < 16 && s->bad[k].again < now + 300000) s->bad[k].again = now + 300000;
        t->state = 0;
        return;
    }
    tn_wall(pl, idx, t, now);
    if (t->state == 1 && drop_ok && standing + dropped == t->nproj) { t->state = 2; s->held++; tn_say(idx, now, "the town at %d,%d stands (%d buildings)", t->c.x, t->c.y, standing); }
    // the next building (every 8 s), the drop site alone first; one it can't pay waits (the drop site always)
    if (t->ncit && (!t->plan_t || now - t->plan_t >= 8000)) {
        t->plan_t = now ? now : 1;
        int going = 0;
        for (int j = 0; j < t->nproj; j++) going += t->proj[j].state == 1;
        for (int j = 0; j < t->nproj && going < 2; j++) {
            struct tn_proj *p = &t->proj[j];
            if (p->state) continue;
            if (p->kind != TK_DROP && !drop_ok && !t->proj[0].state) break;
            int ty = tn_kind_type(pl, p->kind, p->want);
            ivec2 at = iv2(0, 0), c = t->c;
            if (!ty) { if (p->kind != TK_DROP && ++p->waits > 30) { p->state = 3; tn_say(idx, now, "no %s it can pay for 4 minutes - dropped", k_tk[p->kind]); } break; }
            if (p->kind == TK_PROD && t->proj[0].state) c = t->proj[0].p;
            if (p->kind == TK_TOWER) {   // 5 tiles from the drop site, towards the enemy it knows (else round it)
                ivec2 d0 = t->proj[0].state ? t->proj[0].p : t->c;
                float ax = (float)(t->toward.x - d0.x), ay = (float)(t->toward.y - d0.y), l = sqrtf(ax * ax + ay * ay);
                float ang = l > 1 ? atan2f(ay, ax) : 0;
                int nth = 0;
                for (int k = 0; k < j; k++) nth += t->proj[k].kind == TK_TOWER;
                ang += (nth & 1 ? 1 : -1) * (0.6f + 0.6f * (float)(nth / 2)) * (l > 1 ? 1.0f : 2.6f);
                c = iv2(d0.x + (int)lroundf(5 * cosf(ang)), d0.y + (int)lroundf(5 * sinf(ang)));
            }
            if (!tn_spot(pl, idx, t, ty, c, hc, &at)) { if (++p->waits > 30 && p->kind != TK_DROP) p->state = 3; break; }
            ee_unit *b = place_foundation(pl, ty, at.x, at.y);
            if (!b) { if (++p->waits > 30 && p->kind != TK_DROP) p->state = 3; break; }
            p->b = b; p->id = unit_get_id(b); p->type = ty; p->p = at; p->state = 1; p->tries++;
            s->laid++;
            tn_say(idx, now, "%s %d laid at %d,%d (%d citizens)", k_tk[p->kind], ty, at.x, at.y, t->ncit);
            going++;
            if (p->kind == TK_DROP) break;
        }
    }
    // builders: all on the drop site until it stands, then 3 on each other going up; the rest gather by the town
    int want[TN_PROJ] = { 0 }, have[TN_PROJ] = { 0 };
    for (int j = 0; j < t->nproj; j++) if (t->proj[j].state == 1) want[j] = t->proj[j].kind == TK_DROP ? (drop_ok ? 3 : 64) : 3;
    ee_resource_mgr *rm = civ_mgr_get_resource_mgr(player_get_civ_mgr(pl));
    for (int k = 0; k < t->ncit; k++) {
        ee_unit *u = t->cit[k];
        ee_goal *g = unit_get_goal(u);
        const char *gn = g ? rtti_name(g) : "";
        int on = -1;
        if (!strcmp(gn, "EEUGBuildRepair@@"))
            for (int j = 0; j < t->nproj; j++) if (t->proj[j].state == 1 && goal_get_building(g) == t->proj[j].b) on = j;
        if (on >= 0 && have[on] < want[on]) { have[on]++; continue; }
        int j = -1;
        for (int q = 0; q < t->nproj; q++) if (t->proj[q].state == 1 && have[q] < want[q] && (j < 0 || t->proj[q].kind == TK_DROP)) j = q;
        int gathering = !strcmp(gn, "EEUGGather@@");
        if (j >= 0 && (!gathering || !drop_ok || t->proj[j].kind == TK_PROD)) {
            void *bg = goal_build_alloc();
            if (bg) bg = build_goal_init(bg, pl, t->proj[j].b, 0);
            if (bg && unit_goal(u, bg, 0, 0x1F51)) { have[j]++; s->orders++; }
            continue;
        }
        if (!drop_ok || !rm) {   // no drop site yet and nothing to build: to the town
            if (!t->proj[0].state && ivec2_dist2(unit_get_tile(u), t->c) > 64 && !goal_near(u, (float)t->c.x + 0.5f, (float)t->c.y + 0.5f, 4.0f)) {
                s->orders += unit_goal(u, goal_move((float)t->c.x + 0.5f, (float)t->c.y + 0.5f), 0, 0x1F40);
                s->sent++;
            }
            continue;
        }
        if (t->sent_mine[k]) continue;   // the game's economy's
        if (gathering) {   // gathering by the town already
            ee_unit *st = goal_get_site(g);
            if (st && ivec2_dist2(unit_get_tile(st), t->c) <= 14 * 14) { t->sent_mine[k] = 1; continue; }
        }
        if (g && (goal_is_location(g) || goal_is_attack(g) || !strcmp(gn, "EEUGBuildRepair@@")) && !gathering) continue;   // once it stands still
        unsigned cap = table_value(0x16);
        ee_unit *best = NULL;
        int bd = 14 * 14 + 1;
        ee_zone **zb = player_get_zones_begin(pl); ee_zone **ze = player_get_zones_end(pl);
        for (int z = 0; zb && zb + z < ze && z < 256; z++) {
            ee_zone *zone = zb[z];
            void **sb = zone ? zone_get_sites_begin(zone) : NULL, **se = zone ? zone_get_sites_end(zone) : NULL;
            for (int q = 0; sb && sb + q < se && q < 4096; q++) {
                ee_unit *ms = sb[q];
                int r = ms ? unit_get_resource(ms) : 0;
                if (r < 4 || r > 6 || unit_get_amount(ms) <= 0 || site_gatherers(ms) >= cap) continue;
                int d = ivec2_dist2(unit_get_tile(ms), t->c);
                if (d < bd) { bd = d; best = ms; }
            }
        }
        if (best) { resource_mgr_assign(rm, u, best); s->assigned++; t->sent_mine[k] = 1; }
    }
}
// the resources a town's drop site waits for (expand.c expand_reserve, the budget's reserve): a Town Center's for 90 s,
// then a Settlement's, until it is laid (3 minutes at most)
internal void towns_reserve(ee_player *pl, int reserve[7]) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    if (!TOWNS(idx)) return;
    uint32_t now = game_time();
    for (int k = 0; k < TN_MAX; k++) {
        struct tn_town *t = &g_tn[idx].town[k];
        if (t->state != 1 || t->proj[0].state || t->proj[0].tries || now - t->t > 180000) continue;   // its first drop site
        int ty = now - t->t < 90000 && ut_has(UT(B_TOWN_CENTER)) && !(TOWNS(idx) & 2) ? UT(B_TOWN_CENTER) : UT(B_SETTLEMENT);
        for (int r = 2; r < 7; r++) reserve[r] += unit_cost(pl, ty, r);
    }
    for (int k = 0; k < TN_MAX; k++) {   // its next tower once the drop site is laid
        struct tn_town *t = &g_tn[idx].town[k];
        if (t->state != 1 || !t->proj[0].state) continue;
        int j;
        for (j = 1; j < t->nproj && t->proj[j].state; j++) ;
        int tt = j < t->nproj ? ld_kind_type(pl, LDB_TOWER) : 0;
        if (j < t->nproj) for (int r = 2; r < 7; r++) reserve[r] += tt ? unit_cost(pl, tt, r) : unit_cost(pl, UT(B_GUARD_TOWER_MIDDLE), r);
    }
}
// FORWARD (army.c BUILD): a production building of type in the town nearest the enemy; 1: the town takes it (laid or
// one still to come there), 0: the game's build manager as before
internal int towns_production(ee_player *pl, unsigned idx, int type) {
    struct tn_st *s = &g_tn[idx & 15];
    if (!(TOWNS(idx) & 4)) return 0;
    for (int k = 0; k < TN_MAX; k++)   // one at a time
        for (int j = 0; j < s->town[k].nproj; j++) if (s->town[k].state && s->town[k].proj[j].kind == TK_PROD && s->town[k].proj[j].state < 2) return 1;
    ee_civ_mgr *cm = player_get_civ_mgr(pl); ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!bm || build_mgr_get_base_x(bm) == -10000) return 0;
    ivec2 base = build_mgr_get_base(bm);
    static struct tn_world w;
    tn_world(pl, idx & 15, &w);
    struct tn_town *best = NULL;
    int bd = 0x7FFFFFFF, bbase = 0;
    for (int k = 0; k < TN_MAX; k++) {
        struct tn_town *t = &s->town[k];
        if (t->state != 2) continue;
        int np = 0;
        for (int j = 0; j < t->nproj; j++) np += t->proj[j].kind == TK_PROD && t->proj[j].state != 3;
        if (np >= 5 || t->nproj >= TN_PROJ) continue;
        for (int f = 0; f < w.nf; f++) {
            int d = ivec2_dist2(t->c, w.foe[f]);
            if (d < bd) { bd = d; best = t; bbase = ivec2_dist2(base, w.foe[f]); }
        }
    }
    if (!best || sqrtf((float)bd) + 15.0f > sqrtf((float)bbase)) return 0;
    struct tn_proj *p = &best->proj[best->nproj++];
    memset(p, 0, sizeof *p);
    p->kind = TK_PROD; p->want = type;
    best->state = 2;
    if (best->ncit < 3) tn_take_cits(pl, idx & 15, best, 3, (unsigned)area_id(base, 2));
    s->prods++;
    tn_say(idx & 15, game_time(), "production building %d for the town at %d,%d (%d tiles from the enemy, its base %d)", type, best->c.x, best->c.y,
           (int)sqrtf((float)bd), (int)sqrtf((float)bbase));
    return 1;
}
internal void tn_plan(ee_player *pl, unsigned idx, struct tn_st *s, unsigned hc, ivec2 base, uint32_t now) {
    float mine, theirs;
    int n = 0, going = 0;
    for (int k = 0; k < TN_MAX; k++) { n += s->town[k].state != 0; going += s->town[k].state == 1; }
    const char *why = NULL;
    if (going) why = "a town going up";
    else if (n >= g_tn_max) why = "holds towns_max towns";
    else if (current_epoch(pl) < FARM_EPOCH) why = "before the Copper Age";
    else if (workforce(pl) < 20) why = "fewer than 20 workers";
    else if (base_threatened(idx)) why = "enemy soldiers in its base";
    else if (ex_outmatched(pl, &mine, &theirs)) why = "its army outmatched";
    if (!why) {
        static struct tn_world w;
        tn_world(pl, idx, &w);
        ivec2 c;
        int nm = 0, done = 0;
        if (TOWNS(idx) & 8) {   // a forward base: the army and the stock to hold it
            int have_fwd = 0, rich = 1, *st = player_stock_at(pl), pt = tn_prod_type(pl, idx), tt = ld_kind_type(pl, LDB_TOWER);
            for (int k = 0; k < TN_MAX; k++) have_fwd |= s->town[k].state && s->town[k].forward;
            for (int r = 2; r < 7 && st; r++) {
                int need = unit_cost(pl, UT(B_TOWN_CENTER), r) + 4 * (tt ? unit_cost(pl, tt, r) : unit_cost(pl, UT(B_GUARD_TOWER_MIDDLE), r)) + 2 * unit_cost(pl, pt, r);
                if ((float)st[r] < 1.5f * (float)need) rich = 0;
            }
            if (have_fwd) why = "holds a forward base";
            else if (mine < 1.5f * theirs) why = "its army not 1.5 x the enemy's";
            else if (!st || !rich) why = "not the stock to hold a forward base";
            else if (!tn_forward_site(idx, base, hc, &w, &c)) why = "no forward spot";
            else { s->plans++; tn_found(pl, idx, s, c, 0, &w, hc, now, 1); done = 1; }
        }
        if (!done && (TOWNS(idx) & 1)) {
            int fs = tn_free_slots(pl, hc, &w);
            if (fs >= 4) { if (!why) why = "free places at its mines"; }
            else if (!tn_site(pl, idx, base, hc, &w, &c, &nm)) {
                why = "no free mines";
                if (g_autotest) tn_say(idx, now, "mines: %d on the home continent, %d unexplored, %d too near or far, %d by a drop site, %d by an enemy building, %d lost there or defended",
                                       g_tn_why[0], g_tn_why[1], g_tn_why[2], g_tn_why[3], g_tn_why[4], g_tn_why[5]);
            }
            else { s->plans++; tn_found(pl, idx, s, c, nm, &w, hc, now, 0); why = NULL; }
        }
    }
    if (why && why != s->why) { s->why = why; if (g_autotest) tn_say(idx, now, "no new town: %s", why); }
    if (!why) s->why = NULL;
}
internal void towns_act(ee_player *pl, unsigned idx, uint32_t now) {
    struct tn_st *s = &g_tn[idx];
    if (now < s->t) memset(s, 0, sizeof *s);   // a new game
    if (s->t && now - s->t < 2000) return;
    s->t = now ? now : 1;
    if (!TOWNS(idx)) return;
    ee_civ_mgr *cm = player_get_civ_mgr(pl); ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!bm || build_mgr_get_base_x(bm) == -10000) return;
    ivec2 base = build_mgr_get_base(bm);
    unsigned hc = (unsigned)area_id(base, 2);
    if (!hc) return;
    for (int k = 0; k < TN_MAX; k++) if (s->town[k].state) tn_run(pl, idx, s, &s->town[k], hc, now);
    if (!s->plan_t || now - s->plan_t >= 30000) { s->plan_t = now ? now : 1; tn_plan(pl, idx, s, hc, base, now); }
    if (g_autotest && g_log && (!s->log_t || now / 300000 != s->log_t / 300000) && s->plans) {
        s->log_t = now ? now : 1;
        int n = 0, held = 0;
        for (int k = 0; k < TN_MAX; k++) { n += s->town[k].state != 0; held += s->town[k].state == 2; }
        tn_say(idx, now, "status - %d towns (%d standing) | planned %u founded %u stood %u lost %u unpaid %u | laid %u built %u laid again %u orders %u assigned %u sent %u | production %u walls %u",
               n, held, s->plans, s->founded, s->held, s->lost, s->unpaid, s->laid, s->built, s->rebuilt, s->orders, s->assigned, s->sent, s->prods, s->walls);
    }
}
internal void towns_new_game(void) { memset(g_tn, 0, sizeof g_tn); }
internal void towns_install(const char *autotest_ini) {
    g_towns = g_set_ai[1].towns;
    g_tn_max = g_set_ai[1].towns_max;
    g_tn_cits = g_set_ai[1].towns_citizens;
    g_tn_towers = g_set_ai[1].towers_per_town;
    if (g_tn_max > TN_MAX) g_tn_max = TN_MAX;
    if (g_tn_cits > 12) g_tn_cits = 12;
    if (g_tn_towers > TN_PROJ - 1) g_tn_towers = TN_PROJ - 1;
    if (autotest_ini) g_v_towns = ini_int("autotest", "v_towns", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# towns: towns %d (variant %d), at most %d, %d citizens, %d towers each\n", g_towns, g_v_towns, g_tn_max, g_tn_cits, g_tn_towers);
}
