// EXPLORING ([ai_base] explore; the player 2026-11-20, after honest_map left his AIs standing by their Town Centers
// with no food in sight: "i usualy select all citezens and set them to auto explore until at least food is found, its
// crude but works, the ai probably can micros its citezens more targeted to uncover unexplored tile until it has what
// it needs to start (abselut focus is food until it can reach stonage)", "a scouting dog that systematicly circles the
// base should aslo be a good option, later the scouts just walk toward sthe center of each territory, missing evrething
// around it, they also should be more systematic in uncovering undsicovered terrain"). With the map handed over
// (honest_map off, intel.c) everything is explored and none of it does anything.
//   THE MAP UNEXPLORED - per AI, cells of XP_CELL x XP_CELL tiles: how many tiles of each it has not explored (the
//     game's own state, the tile's sight count for it 0). A cell all explored is never counted again - exploring is for
//     good. Counted anew every XP_EVERY, XP_SCAN cells at most a look.
//   A GOAL (xp_goal) - an explored land tile beside one it hasn't explored, on the land the unit stands on: what lies in
//     the dark is not known (land or water, how far), its edge is, and a unit there sees into it. The nearest to the
//     base first (a ring round it, then the next one out), the unit's own way counted half (along the ring, not across
//     it); none within XP_SPREAD of another explorer's goal, none under an enemy tower it has seen (work.c wk_danger),
//     none where one gave up lately (not reached in XP_GIVE_UP: a cliff, a wall) for XP_FORGET.
//   THE FOOD SEARCH - while citizens wanted on food find no site (work.c food_short), that many more of its free
//     citizens - the idle first, then the woodcutters - explore from their nearest drop site, XP_HOME_R tiles out at
//     most: in the Prehistoric age with no food worked at all as many as there are (the player's own start), else
//     XP_CIT_MAX. A look with every food citizen placed gives half of them back to work.
//   THE BASE SCOUT - from XP_DOG_CITS citizens on (before that the food search explores: the scout's 50 food is a
//     citizen's), while unexplored land lies within XP_SCOUT_R of a town of its, one Canine Scout (the player's
//     "scouting dog": 50 food at a Town Center or Capitol, 0.8 fast against a citizen's 0.5, sight 4 against 3) is
//     ordered every XP_DOG_EVERY till one comes - the economy's money, not the army's, and past the queues' depth
//     (econ.c: a Town Center's queue is its citizens') - and circles that town outward, ring after ring, as far as its
//     land goes; it stays a scout, waiting where it is when nothing is left. Every Canine Scout of its is one. While
//     the orders are refused (XP_DOG_TRIES in a row: the start's food goes to its citizens) a citizen stands in for it
//     (the idle first, then a woodcutter, the nearest), XP_SCOUT_R out, back to work when one comes or nothing is left.
//   THE TERRITORY SCOUTS (stations.c) take their goals here too: the unexplored edge inside the territory first, then
//     the part of it looked at longest ago (xp_stale), then its middle.
// The explorers are claimed (claims.c CLW_EXPLORE, routine): work.c, the builders and the stations - routine too - leave
// them alone; the citizens' safety (a task, the militia survival) takes one off, and it is no explorer then.
#define XP_CELL 8
#define XP_EVERY 2000
#define XP_STEP 250
#define XP_SCAN 2048
#define XP_SPREAD 10
#define XP_GIVE_UP 40000
#define XP_FORGET 180000
#define XP_HOME_R 40
#define XP_SCOUT_R 48
#define XP_DOG_EVERY 30000
#define XP_DOG_WAIT 120000
#define XP_DOG_CITS 10
#define XP_DOG_TRIES 3
#define XP_CIT_MAX 4
#define XP_CLAIM_T 15000
#define XP_UNITS 32
#define XP_BAD 16
#define XP_CAND 24
#define XP_STALE 300000   // a part of a territory looked at this long ago is looked at again (xp_stale)
struct xp_unit { ee_unit *u; int id; uint8_t kind; ivec2 goal, base; uint32_t since; };   // kind 1 the food search, 2 the base scout (a citizen), 3 a Canine Scout (base: its town); since: its goal since then
internal struct xp_st {
    ee_map *map; int size, n, dark; uint16_t *left; int scan;   // dark: cells with tiles unexplored
    uint32_t t, log_t, dog_t, dog_ok, step_t;   // step_t: THE NEXT GOAL AT ONCE's last step   // dog_t: a Canine Scout ordered then, dog_ok: one paid then (in training: XP_DOG_WAIT before the next)
    int nu; struct xp_unit u[XP_UNITS];
    int nbad; ivec2 bad[XP_BAD]; uint32_t bad_t[XP_BAD];
    unsigned sent, back, gave_up, goals, scouts, dog_asks, dogs, steps;
} g_xp[16];
internal uint8_t g_xp_ter[16][PR_T];   // the territories with cells it hasn't all explored (xp_count)

internal int xp_known(ee_tile *t, unsigned pi) { return (tile_get_sight(t)[pi] & 0xFFu) != 0; }   // the low byte: 0 unexplored, 1 explored, 2+ in sight (sight.c)
internal void xp_count(struct xp_st *e, ee_player *pl) {
    ee_map *map = game_map();
    ee_tile **tiles = map ? map_get_tiles(map) : NULL;
    int size = map ? (int)map_get_size(map) : 0;
    if (!tiles || size < XP_CELL) return;
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    if (map != e->map || size != e->size || !e->left) {
        free(e->left);
        e->n = (size + XP_CELL - 1) / XP_CELL; e->size = size; e->map = map; e->scan = 0;
        e->left = malloc((size_t)e->n * (size_t)e->n * sizeof *e->left);
        if (!e->left) { e->map = NULL; return; }
        for (int c = 0; c < e->n * e->n; c++) e->left[c] = 0xFFFF;
    }
    int nc = e->n * e->n, done = 0, k = 0;
    for (; k < nc && done < XP_SCAN; k++) {
        int c = (e->scan + k) % nc;
        if (!e->left[c]) continue;
        done++;
        int x0 = c % e->n * XP_CELL, y0 = c / e->n * XP_CELL, cnt = 0;
        for (int y = y0; y < y0 + XP_CELL && y < size; y++)
            for (int x = x0; x < x0 + XP_CELL && x < size; x++) {
                ee_tile *t = tiles[y * size + x];
                cnt += t && !xp_known(t, pi);
            }
        e->left[c] = (uint16_t)cnt;
    }
    e->scan = (e->scan + k) % nc;
    e->dark = 0;
    memset(g_xp_ter[pi], 0, sizeof g_xp_ter[pi]);
    for (int c = 0; c < nc; c++) {
        if (!e->left[c]) continue;
        e->dark++;
        unsigned r = ter_near(c % e->n * XP_CELL + XP_CELL / 2, c / e->n * XP_CELL + XP_CELL / 2);
        if (r < PR_T) g_xp_ter[pi][r] = 1;
    }
}
// territory r has ground player idx hasn't explored (by its cells' middles; the stations' scouts: A LOOK INSIDE)
internal int xp_ter_dark(unsigned idx, int r) { return g_set_ai[1].explore && r >= 0 && r < PR_T && g_xp_ter[idx & 15][r]; }
// the explored tile x, y lies at the edge of the unexplored: one of its four neighbours not explored
internal int xp_edge(ee_tile **tiles, int size, unsigned pi, int x, int y) {
    if (x < 0 || y < 0 || x >= size || y >= size) return 0;
    ee_tile *t = tiles[y * size + x];
    if (!t || !xp_known(t, pi)) return 0;
    static const int d[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    for (int k = 0; k < 4; k++) {
        int nx = x + d[k][0], ny = y + d[k][1];
        if (nx < 0 || ny < 0 || nx >= size || ny >= size) continue;
        ee_tile *q = tiles[ny * size + nx];
        if (q && !xp_known(q, pi)) return 1;
    }
    return 0;
}
// the land at p: its own, else the nearest within 4 tiles' (a Town Center's tile lies under the building: no land)
internal unsigned xp_land(ivec2 p) {
    unsigned l = area_id(p, 2);
    for (int r = 1; r <= 4 && !l; r++)
        for (int dy = -r; dy <= r && !l; dy++)
            for (int dx = -r; dx <= r && !l; dx++) if (dx == -r || dx == r || dy == -r || dy == r) l = area_id(iv2(p.x + dx, p.y + dy), 2);
    return l;
}
// A GOAL for a unit at p: the edge tile (see above) nearest base first, within rmax of base (0: anywhere), in territory
// ter (-1: any); self: the explorer asking (its own goal no hindrance). 1 and *out, 0 none
internal int xp_goal(unsigned idx, ee_player *pl, ivec2 p, ivec2 base, int rmax, int ter, ee_unit *self, ivec2 *out) {
    struct xp_st *e = &g_xp[idx & 15];
    ee_map *map = game_map();
    ee_tile **tiles = map ? map_get_tiles(map) : NULL;
    if (!e->left || map != e->map || !tiles || !e->dark) return 0;
    unsigned land = xp_land(p), pi = (unsigned)player_get_index(pl) & 15;
    if (!land) return 0;
    uint32_t now = game_time();
    ivec2 avoid[XP_UNITS + XP_BAD];
    int na = 0;
    for (int k = 0; k < e->nu; k++) if (e->u[k].u != self && e->u[k].goal.x >= 0) avoid[na++] = e->u[k].goal;
    for (int k = 0; k < e->nbad; k++) if (now >= e->bad_t[k] && now - e->bad_t[k] < XP_FORGET) avoid[na++] = e->bad[k];
    struct { int c; float s; } cand[XP_CAND];
    int nc = 0;
    float rr = rmax > 0 ? (float)(rmax + XP_CELL) : 0;
    for (int c = 0; c < e->n * e->n; c++) {
        if (!e->left[c]) continue;
        ivec2 cc = iv2(c % e->n * XP_CELL + XP_CELL / 2, c / e->n * XP_CELL + XP_CELL / 2);
        float db = sqrtf((float)ivec2_dist2(cc, base)), s = db + 0.5f * sqrtf((float)ivec2_dist2(cc, p));
        if (rmax > 0 && db > rr) continue;
        if (ter >= 0 && ter_near(cc.x, cc.y) != (unsigned)ter && ter_near(cc.x - XP_CELL / 2, cc.y - XP_CELL / 2) != (unsigned)ter &&
            ter_near(cc.x + XP_CELL / 2 - 1, cc.y + XP_CELL / 2 - 1) != (unsigned)ter) continue;
        if (nc == XP_CAND && s >= cand[nc - 1].s) continue;
        int j = nc < XP_CAND ? nc++ : nc - 1;
        while (j > 0 && cand[j - 1].s > s) { cand[j] = cand[j - 1]; j--; }
        cand[j].c = c; cand[j].s = s;
    }
    for (int i = 0; i < nc; i++) {
        int x0 = cand[i].c % e->n * XP_CELL, y0 = cand[i].c / e->n * XP_CELL, best = -1, bd = 0;
        for (int y = y0 - 1; y <= y0 + XP_CELL; y++)   // the cell and the ring round it: a dark cell's edge lies in its neighbour
            for (int x = x0 - 1; x <= x0 + XP_CELL; x++) {
                if (!xp_edge(tiles, e->size, pi, x, y)) continue;
                ivec2 q = iv2(x, y);
                if (area_id(q, 2) != land || (rmax > 0 && ivec2_dist2(q, base) > rmax * rmax)) continue;
                if (ter >= 0 && ter_near(x, y) != (unsigned)ter) continue;
                int close = 0;
                for (int a = 0; a < na && !close; a++) close = ivec2_dist2(q, avoid[a]) < XP_SPREAD * XP_SPREAD;
                if (close || wk_danger(idx, q)) continue;
                int d = ivec2_dist2(q, p);
                if (best < 0 || d < bd) { best = y * e->size + x; bd = d; }
            }
        if (best >= 0) { *out = iv2(best % e->size, best / e->size); return 1; }
    }
    return 0;
}
// THE TERRITORY SCOUTS' second goal: the land of territory ter (on the land of p) looked at longest ago, XP_STALE ago at
// least (intel.c's looks: a unit of its or an ally's within 8 tiles) - by the middles of the cells; 0 none
internal int xp_stale(unsigned idx, ee_player *pl, ivec2 p, int ter, ivec2 *out) {
    struct xp_st *e = &g_xp[idx & 15];
    unsigned land = xp_land(p);
    if (!e->left || !land) return 0;
    uint32_t best = 0;
    int found = 0;
    for (int c = 0; c < e->n * e->n; c++) {
        ivec2 cc = iv2(c % e->n * XP_CELL + XP_CELL / 2, c / e->n * XP_CELL + XP_CELL / 2);
        if (ter_near(cc.x, cc.y) != (unsigned)ter || area_id(cc, 2) != land || wk_danger(idx, cc)) continue;
        uint32_t ago = intel_looked_ago(pl, cc.x, cc.y);
        if (ago < XP_STALE || (found && ago <= best)) continue;
        best = ago; *out = cc; found = 1;
    }
    return found;
}

// a scout's goal g is done: no longer at the edge of the unexplored and looked at within XP_STALE (stations.c)
internal int xp_done(unsigned idx, ee_player *pl, ivec2 g) {
    struct xp_st *e = &g_xp[idx & 15];
    ee_map *map = game_map();
    ee_tile **tiles = map ? map_get_tiles(map) : NULL;
    if (g.x < 0 || !tiles || map != e->map) return 1;
    return !xp_edge(tiles, e->size, idx & 15, g.x, g.y) && intel_looked_ago(pl, g.x, g.y) < XP_STALE;
}
internal int xp_alive(const struct xp_unit *m) { return m->u && unit_get_serial(m->u) == m->id && unit_get_hp(m->u) > 0 && unit_get_x(m->u) >= 0; }
internal int xp_own(const struct xp_unit *m, ee_player *pl) { return xp_alive(m) && unit_get_owner(m->u) == pl; }   // alive and still its own (a priest may have converted it)
internal void xp_release(struct xp_st *e, int k, int stop) {
    struct xp_unit *m = &e->u[k];
    if (xp_alive(m)) {
        if (stop) { ivec2 p = unit_get_tile(m->u); unit_goal(m->u, goal_move((float)p.x + 0.5f, (float)p.y + 0.5f), 0, 0x1F40); }   // where it stands: idle at once, work.c's again
        cl_end(m->u, CLW_EXPLORE);
    }
    e->u[k] = e->u[--e->nu];
}
// the base an explorer works round: its nearest drop site that is no granary (drops_cached), else where it stands
internal ivec2 xp_base(unsigned pi, ivec2 p) {
    int k = -1;
    if (drop_nearest(pi, p, 0, &k) == 0x7FFFFFFF || k < 0) return p;
    return g_drops[pi].d[k].p;
}
internal int xp_send(unsigned idx, ee_player *pl, struct xp_unit *m, uint32_t now) {
    ivec2 p = unit_get_tile(m->u), g;
    ivec2 base = m->kind >= 2 ? m->base : xp_base(idx & 15, p);
    if (!xp_goal(idx, pl, p, base, m->kind == 1 ? XP_HOME_R : m->kind == 2 ? XP_SCOUT_R : 0, -1, m->u, &g)) return 0;
    if (!unit_goal(m->u, goal_move((float)g.x + 0.5f, (float)g.y + 0.5f), 0, 0x1F40)) return 0;
    m->goal = g; m->since = now ? now : 1;
    g_xp[idx & 15].goals++;
    return 1;
}

// a free citizen made an explorer of kind (2: round base): the idle ones first, then the woodcutters, the nearest to at
// first; 1 sent, 0 none (or nothing to explore for it)
internal int xp_take(unsigned idx, ee_player *pl, int kind, ivec2 at, ivec2 base, uint32_t now) {
    struct xp_st *e = &g_xp[idx & 15];
    if (e->nu >= XP_UNITS) return 0;
    for (int pass = 0; pass < 2; pass++) {
        ee_unit *best = NULL;
        int bd = 0, count = 0;
        for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
            ee_unit_type *td = unit_get_type_data(u);
            if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !is_citizen_type(unit_type_get_id(td))) continue;
            if (unit_get_frozen(u) || unit_get_ai_held(u) || unit_get_busy(u) || expand_has(pl, u)) continue;
            ee_goal *g = unit_get_goal(u);
            ee_unit *site = g && goal_is_gather(g) ? goal_get_site(g) : NULL;
            if (pass ? !site || unit_get_resource(site) != 3 : g != NULL) continue;
            int mine = 0, d = ivec2_dist2(unit_get_tile(u), at);
            for (int j = 0; j < e->nu && !mine; j++) mine = e->u[j].u == u;
            if (mine || !cl_may(u, CLP_ROUTINE, CLW_EXPLORE) || (best && d >= bd)) continue;
            best = u; bd = d;
        }
        if (!best) continue;
        struct xp_unit *m = &e->u[e->nu];
        memset(m, 0, sizeof *m);
        m->u = best; m->id = (int)unit_get_serial(best); m->kind = (uint8_t)kind; m->goal = iv2(-1, -1); m->base = base;
        if (!xp_send(idx, pl, m, now)) return 0;   // nothing to explore for it: it stays at work
        cl_take(best, CLP_ROUTINE, CLW_EXPLORE, XP_CLAIM_T);
        e->nu++;
        return 1;
    }
    return 0;
}

// THE NEXT GOAL AT ONCE (the player 2026-11-20: "exploring is very slow, with much idle time, scouts should get a new
// order the moment they reached their goal"): every XP_STEP, not with the look - a goal uncovered (no longer at the
// edge: the unit's sight took it in on the way) gives the next one before the unit stops; one without a goal asks
// again every XP_EVERY
internal void xp_goals(ee_player *pl, unsigned idx, uint32_t now) {
    struct xp_st *e = &g_xp[idx & 15];
    e->step_t = now ? now : 1; e->steps++;
    // each explorer's goal: reached (or no longer at the edge), not reached in XP_GIVE_UP (given up, avoided a while):
    // the next; none left: a citizen back to work, a scout stays where it is
    ee_map *map = game_map();
    ee_tile **tiles = map ? map_get_tiles(map) : NULL;
    for (int k = e->nu - 1; k >= 0 && tiles; k--) {
        struct xp_unit *m = &e->u[k];
        // dead, converted or taken by its safety since the look: let go, no order (the next goal sent a citizen its
        // safety had sent home back towards the dark until the look)
        if (!xp_own(m, pl) || !cl_may(m->u, CLP_ROUTINE, CLW_EXPLORE)) { xp_release(e, k, 0); continue; }
        int edge = m->goal.x >= 0 && xp_edge(tiles, e->size, idx, m->goal.x, m->goal.y);
        if (edge && now - m->since < XP_GIVE_UP && unit_get_goal(m->u)) continue;   // on its way
        int there = m->goal.x >= 0 && ivec2_dist2(unit_get_tile(m->u), m->goal) <= 4 && !unit_get_goal(m->u);   // stands there and it stays dark: the same goal again would hold it
        if (edge && (now - m->since >= XP_GIVE_UP || there)) {
            int b = -1;
            for (int j = 0; j < e->nbad && b < 0; j++) if (now - e->bad_t[j] >= XP_FORGET) b = j;   // a forgotten one's place
            if (b < 0) b = e->nbad < XP_BAD ? e->nbad++ : 0;
            e->bad[b] = m->goal; e->bad_t[b] = now ? now : 1; e->gave_up++;
        }
        if (m->goal.x < 0 && m->since && now - m->since < XP_EVERY) continue;   // none found lately: asked again later
        m->goal = iv2(-1, -1);
        if (xp_send(idx, pl, m, now)) continue;
        m->since = now ? now : 1;
        if (m->kind < 3) { e->back += m->kind == 1; xp_release(e, k, 1); }   // nothing left for it: back to work (a Canine Scout waits)
    }
}

// the quick step at a simulation step's end (eeaimod.c ai_step_end: the AI's own update comes every second or two -
// a step every 1.6 s on average in the first test): the explorers' next goals, every XP_STEP, for a player whose look
// ran in this game lately
internal void explore_step(ee_player *pl, uint32_t now) {
    if (!pl || is_vanilla(pl) || !g_set_ai[1].explore) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct xp_st *e = &g_xp[idx];
    if (!e->left || !e->nu || !e->t || now < e->t || now - e->t > 3 * XP_EVERY) return;
    if (e->step_t && now >= e->step_t && now - e->step_t < XP_STEP) return;
    xp_goals(pl, idx, now);
}
internal void explore_new_game(void) {
    for (int i = 0; i < 16; i++) free(g_xp[i].left);
    memset(g_xp, 0, sizeof g_xp); memset(g_xp_ter, 0, sizeof g_xp_ter);
}

internal void explore_tick(ee_player *pl, ee_resource_mgr *rm, uint32_t now) {
    (void)rm;
    if (!pl || is_vanilla(pl) || !g_set_ai[1].explore) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct xp_st *e = &g_xp[idx];
    if (now < e->t) { free(e->left); memset(e, 0, sizeof *e); memset(g_xp_ter[idx], 0, sizeof g_xp_ter[idx]); }   // a new game
    if (e->t && now - e->t < XP_EVERY) {   // between the looks: the explorers' next goals only
        if (e->left && e->nu && (!e->step_t || now < e->step_t || now - e->step_t >= XP_STEP)) xp_goals(pl, idx, now);
        return;
    }
    e->t = now ? now : 1;
    xp_count(e, pl);
    if (!e->left) return;
    int on = player_ai_toggle(pl, SM_AIT_EXPLORE);   // a scenario's Explore off: nobody explores
    drops_cached(pl);
    for (int k = e->nu - 1; k >= 0; k--) {   // the explorers: alive, still its own (its safety may have taken a citizen)
        struct xp_unit *m = &e->u[k];
        if (!xp_own(m, pl) || !on || !cl_take(m->u, CLP_ROUTINE, CLW_EXPLORE, XP_CLAIM_T)) { xp_release(e, k, 0); continue; }
    }
    if (!on) return;
    // THE FOOD SEARCH
    struct wk_st *w = &g_wk[idx];
    int nfood = 0;
    for (int k = 0; k < e->nu; k++) nfood += e->u[k].kind == 1;
    if (w->t && e->dark) {
        int all = current_epoch(pl) == 0 && w->have[2] == 0, cap = all ? XP_UNITS - 2 : XP_CIT_MAX;
        if (w->food_short > 0 && nfood < cap) {
            int want = w->food_short < cap - nfood ? w->food_short : cap - nfood;
            ivec2 at = g_drops[idx].n ? g_drops[idx].d[0].p : iv2(0, 0);
            for (; want > 0 && xp_take(idx, pl, 1, at, at, now); want--) { nfood++; e->sent++; }
        } else if (w->food_short <= 0 && nfood > 0) {   // every food citizen placed: half of them back to work
            for (int k = e->nu - 1, give = (nfood + 1) / 2; k >= 0 && give > 0; k--)
                if (e->u[k].kind == 1) { xp_release(e, k, 1); give--; e->back++; }
        }
    }
    // THE BASE SCOUT: its Canine Scouts (each one a scout, round its nearest town); none and unexplored land within
    // XP_SCOUT_R of a town: one ordered, or a citizen while none is to be had
    int dogs = 0, scouts = 0, count = 0;
    if (ut_has(UT(DOMESTIC_WOLF)))
        for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
            ee_unit_type *td = unit_get_type_data(u);
            if (!td || unit_type_get_id(td) != UT(DOMESTIC_WOLF) || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
            dogs++;
            int mine = 0;
            for (int j = 0; j < e->nu && !mine; j++) mine = e->u[j].u == u;
            if (mine || e->nu >= XP_UNITS || !cl_take(u, CLP_ROUTINE, CLW_EXPLORE, XP_CLAIM_T)) continue;
            struct xp_unit *m = &e->u[e->nu++];
            memset(m, 0, sizeof *m);
            m->u = u; m->id = (int)unit_get_serial(u); m->kind = 3; m->goal = iv2(-1, -1); m->base = xp_base(idx, unit_get_tile(u));
            e->dogs++;
        }
    for (int k = e->nu - 1; k >= 0 && dogs; k--)   // a Canine Scout out: the citizen standing in for one back to work
        if (e->u[k].kind == 2) xp_release(e, k, 1);
    if (dogs) e->dog_asks = 0;
    for (int k = 0; k < e->nu; k++) scouts += e->u[k].kind == 2;
    // not before XP_DOG_CITS citizens (the player 2026-11-20: "keep the scout dog back until at least 10 citizens,
    // before it's too huge an opportunity cost") - the food search explores meanwhile
    ee_civ_mgr *xcm = player_get_civ_mgr(pl);
    ee_unit_mgr *xum = xcm ? civ_mgr_get_unit_mgr(xcm) : NULL;
    int cits = xum ? unit_mgr_citizens(xum) : 0;
    for (int i = 0; i < g_drops[idx].n && !dogs && e->dark && cits >= XP_DOG_CITS; i++) {
        ivec2 g, t = g_drops[idx].d[i].p;
        ee_unit *bu = g_drops[idx].d[i].u;
        ee_unit_type *bt = bu ? unit_get_type_data(bu) : NULL;
        if (!bt || !UT_IN(unit_type_get_id(bt), UT_TOWNS) || !xp_goal(idx, pl, t, t, XP_SCOUT_R, -1, NULL, &g)) continue;
        if (ut_has(UT(DOMESTIC_WOLF)) && (!e->dog_t || now - e->dog_t >= XP_DOG_EVERY) && (!e->dog_ok || now - e->dog_ok >= XP_DOG_WAIT)) {   // a Canine Scout ordered, every XP_DOG_EVERY till one comes
            ee_civ_mgr *cm = player_get_civ_mgr(pl);
            ee_unit_mgr *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
            int req[4] = { UT(DOMESTIC_WOLF), 1, 0, 0 };
            unsigned got = 0;
            if (um) hook_train_units(um, req, (uintptr_t)&got);
            e->dog_t = now ? now : 1;
            if (got) e->dog_ok = now ? now : 1;
            if (!got) e->dog_asks++;   // the stock short (the budget's say: the start's food is the citizens') or no Town Center
        }
        // XP_DOG_TRIES orders refused in a row (the start's food went to its citizens): a citizen meanwhile
        if (!scouts && (!ut_has(UT(DOMESTIC_WOLF)) || e->dog_asks >= XP_DOG_TRIES) && xp_take(idx, pl, 2, t, t, now)) { scouts++; e->scouts++; }
        break;
    }
    xp_goals(pl, idx, now);
    if (g_log && (!e->log_t || now - e->log_t >= 60000)) {
        e->log_t = now ? now : 1;
        nfood = scouts = 0;
        for (int k = 0; k < e->nu; k++) { nfood += e->u[k].kind == 1; scouts += e->u[k].kind == 2; }
        int land = 0, land_dark = 0;   // the log's only: the land the AI doesn't know is land (the game's areas)
        ee_map *map = game_map();
        ee_tile **tiles = map ? map_get_tiles(map) : NULL;
        for (int y = 0; tiles && y < e->size; y++)
            for (int x = 0; x < e->size; x++) {
                ee_tile *t = tiles[y * e->size + x];
                if (!t || !area_id(iv2(x, y), 2)) continue;
                land++; land_dark += !xp_known(t, idx);
            }
        log_locked("# explore: P%u at %.1f min: land tiles unexplored %d of %d, cells %d of %d | the food search %d citizens out (food short %d), %u sent, %u back | Canine Scouts %d (%u so far, orders refused %u) | a citizen as the base scout %d (%u so far) | %u goals, %u given up, %u steps\n",
                   idx, now / 60000.0, land_dark, land, e->dark, e->n * e->n, nfood, w->food_short, e->sent, e->back, dogs, e->dogs, e->dog_asks, scouts, e->scouts, e->goals, e->gave_up, e->steps);
    }
}
