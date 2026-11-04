// SETTLEMENT UPGRADES ([ai_base] garrison_upgrades, default on - the player 2026-11-04: "as long as it only pulls from
// excess wood its a nobrainer"; head-to-head level; [autotest] v_garrison_upgrades). Included
// by eeaimod.c after armies.c (econ.c citizen_free, attack.c raiders_near and unit_goal, cover.c cv_built, landing.c,
// expand.c, towns.c: their citizens are theirs).
// The player (2026-11-04): "the ai has huge amounts of useless citizens right now, it should focus on upgrading its
// settlements with them, since that gives resource and defence boni" - on his save P3 had 427 citizens, 284 of them
// on wood with 43000 wood in stock. The game's garrison (spec EEUGLoadGarrison): a citizen that enters a Settlement,
// Town Center or Capitol is used up and counts inside it; a Settlement with 5 becomes a Town Center, a Town Center with
// 15 a Capitol (each step once its build tech is open: a new unit, the hit points' fraction kept), a Capitol takes 50.
// What it gives: stone, gold and iron dropped there from a mine within 4.5 tiles of the building count 5% more per
// citizen inside up to 5, 2.5% up to 15, 1.5% beyond (0x62F4D3) - a Town Center by its mines +25..50%, a full Capitol
// +102%; nothing for wood, hunting or foraging. And the hit points (2000, 4000, 5000), a Town Center's citizens and
// research, the morale of the soldiers near it. The game's own AI garrisons only idle citizens on a 5 / 60 minute
// timer (0x46E51E, 0x46E817), and the mod keeps its citizens at work, so it hardly ever does.
// Every GU_EVERY: the holder with the most metal mines in the bonus's reach (their gatherers counting) that has room and
// no enemy near - a Settlement before the rest (5 make a Town Center: a second citizen producer too) - gets up to
// GU_BATCH spare citizens on its land, the nearest first: idle ones, then wood cutters while the wood stock is over
// GU_WOOD. A holder ordered waits GU_WAIT (they walk there) before it is ordered again.
// "# garrison:" at every order, and once a game minute (autotest) the towns by kind and what was sent.
#define GU_EVERY 20000
#define GU_WOOD 5000
#define GU_BATCH 10
#define GU_WAIT 60000
#define GU_REACH2 20.0f            // the bonus: a mine within this squared distance of the building (0x833AB4)
#define GU_MINES_R 5
#define GU_SAFE_R 12               // no enemy soldiers within this of the holder
internal int g_garrison = 0, g_v_garrison = -1;
#define GARRISON(idx) PV(idx, g_v_garrison, g_garrison)
internal struct gu_st {
    uint32_t t, log_t;
    struct { int id; uint32_t t; } sent[32]; int nsent;   // holders ordered lately (gu_key: their tile - a building's id is no key here, and the upgrade is a new unit on it), when
    unsigned rounds, citizens, idle, wood;               // orders, citizens sent (of them idle, from wood)
} g_gu[16];
internal int expand_has(ee_player *pl, ee_unit *u);   // expand.c
internal int tn_has_cit(unsigned idx, ee_unit *u);    // towns.c
internal int gu_town(ee_unit_type *td) {   // 1 a Settlement, 2 a Town Center, 3 a Capitol, else 0
    int t = unit_type_get_id(td);
    return t == UT(B_SETTLEMENT) ? 1 : t == UT(B_TOWN_CENTER) ? 2 : t == UT(B_CAPITOL) ? 3 : 0;
}
// the metal mines whose drops the holder would raise: each mine with stone, gold or iron left within the bonus's reach,
// counted once and once more for each of its gatherers
internal int gu_mines(ee_unit *h) {
    vec3 hp = unit_get_pos(h);
    ivec2 t = unit_get_tile(h);
    int score = 0;
    for (struct grid_it it = grid_it_box(t.x, t.y, GU_MINES_R, 1); grid_it_next(&it); ) {
        ee_unit *s = it.u;
        int r = unit_get_resource(s);
        if (r < 4 || r > 6 || unit_get_amount(s) <= 0) continue;
        vec3 sp = unit_get_pos(s);
        float dx = sp.x - hp.x, dy = sp.y - hp.y;
        if (dx * dx + dy * dy > GU_REACH2) continue;
        score += 1 + (int)site_gatherers(s);
    }
    return score;
}
internal int gu_key(ee_unit *u) { ivec2 t = unit_get_tile(u); return t.x * 4096 + t.y; }
internal int gu_recent(struct gu_st *g, int id, uint32_t now) {
    for (int k = 0; k < g->nsent; k++) if (g->sent[k].id == id && now - g->sent[k].t < GU_WAIT) return 1;
    return 0;
}
internal void gu_note(struct gu_st *g, int id, uint32_t now) {
    int k = 0, old = 0;
    while (k < g->nsent && g->sent[k].id != id) k++;
    if (k == g->nsent) {
        if (g->nsent < 32) g->nsent++;
        else { for (int j = 1; j < 32; j++) if (g->sent[j].t < g->sent[old].t) old = j; k = old; }
    }
    g->sent[k].id = id; g->sent[k].t = now;
}
internal void garrison_tick(ee_player *pl, uint32_t now) {
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    struct gu_st *g = &g_gu[idx];
    if (now < g->t) memset(g, 0, sizeof *g);   // a new game
    if (!pl || !GARRISON(idx) || (g->t && now - g->t < GU_EVERY)) return;
    g->t = now ? now : 1;
    // the holder: the most metal mines in reach, a Settlement first
    ee_unit *best = NULL;
    int best_score = 0, best_kind = 0, towns[4] = { 0 };
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        int kind = gu_town(it.td);
        if (!kind || unit_get_owner(u) != pl || !cv_built(u) || unit_get_hp(u) <= 0) continue;
        towns[kind]++;
        int room = unit_type_get_garrison_capacity(it.td) - unit_get_garrisoned(u);
        ee_goal *hg = unit_get_goal(u);
        if (room <= 0 || (hg && goal_is_garrison(hg)) || gu_recent(g, gu_key(u), now)) continue;
        int score = gu_mines(u);
        if (!score || raiders_near(pl, unit_get_tile(u), GU_SAFE_R)) continue;
        if (!best || (kind == 1) > (best_kind == 1) || ((kind == 1) == (best_kind == 1) && score > best_score)) { best = u; best_score = score; best_kind = kind; }
    }
    if (g_log && g_autotest && (!g->log_t || now - g->log_t >= 60000)) {
        g->log_t = now ? now : 1;
        fprintf(g_log, "# garrison: P%u at %.1f min: settlements %d, town centers %d, capitols %d; %u orders, %u citizens sent (%u idle, %u from wood)\n",
                idx, now / 60000.0, towns[1], towns[2], towns[3], g->rounds, g->citizens, g->idle, g->wood);
    }
    if (!best) return;
    ee_unit_type *btd = unit_get_type_data(best);
    int room = unit_type_get_garrison_capacity(btd) - unit_get_garrisoned(best), want = room < GU_BATCH ? room : GU_BATCH;
    // the citizens: the nearest spare ones on its land
    ivec2 bp = unit_get_tile(best);
    unsigned land = map_area(bp, 2);
    int wood_spare = player_get_stock(pl, 3) > GU_WOOD;
    static ee_unit *cand[4096];
    static int cd[4096];
    int nc = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it) && nc < 4096; ) {
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || !is_citizen_type(unit_type_get_id(it.td))) continue;
        int idle = citizen_free(u, 1);
        if (!idle) {
            if (!wood_spare || !citizen_free(u, 0)) continue;
            ee_unit *site = goal_get_site(unit_get_goal(u));
            if (!site || unit_get_resource(site) != 3) continue;
        }
        ivec2 p = unit_get_tile(u);
        if (map_area(p, 2) != land || landing_has_unit(pl, u) || expand_has(pl, u) || tn_has_cit(idx, u)) continue;
        cand[nc] = u; cd[nc] = ivec2_dist2(p, bp) - (idle ? 1000000 : 0); nc++;   // the idle first
    }
    int n = 0;
    ee_unit *pick[GU_BATCH];
    while (n < want && n < nc) {   // the nearest want of them (a partial selection)
        int b = n;
        for (int k = n + 1; k < nc; k++) if (cd[k] < cd[b]) b = k;
        ee_unit *tu = cand[n]; cand[n] = cand[b]; cand[b] = tu;
        int td = cd[n]; cd[n] = cd[b]; cd[b] = td;
        pick[n] = cand[n]; n++;
    }
    if (!n) return;
    int idle_n = 0;
    for (int k = 0; k < n; k++) idle_n += cd[k] < 0;
    uint8_t *vec[4] = { 0, (uint8_t *)pick, (uint8_t *)(pick + n), (uint8_t *)(pick + n) };
    void *mem = goal_garrison_alloc();
    ee_goal *gg = mem ? goal_garrison_init(mem, best, vec) : NULL;
    if (!gg || !unit_goal(best, gg, 0, 0x2EE8)) return;   // the game's own AI's state for the holder (0x46CA7E)
    for (int k = 0; k < n; k++) unit_goal(pick[k], goal_part(gg), 0, 0x1F40);
    gu_note(g, gu_key(best), now);
    g->rounds++; g->citizens += (unsigned)n; g->idle += (unsigned)idle_n; g->wood += (unsigned)(n - idle_n);
    if (g_log) fprintf(g_log, "# garrison: P%u at %.1f min: %d citizens (%d idle, %d from wood; wood %d) into the %s at %d,%d (%d inside of %d, metal mines near: %d)\n",
                       idx, now / 60000.0, n, idle_n, n - idle_n, player_get_stock(pl, 3), best_kind == 1 ? "Settlement" : best_kind == 2 ? "Town Center" : "Capitol",
                       bp.x, bp.y, unit_get_garrisoned(best), unit_type_get_garrison_capacity(btd), best_score);
}
internal void garrison_new_game(void) { memset(g_gu, 0, sizeof g_gu); }
internal void garrison_install(const char *autotest_ini) {
    g_garrison = g_set_ai[1].garrison_upgrades;
    if (autotest_ini) g_v_garrison = ini_int("autotest", "v_garrison_upgrades", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# garrison: settlement upgrades %d (variant %d)\n", g_garrison, g_v_garrison);
}
