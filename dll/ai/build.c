// BUILDING BY TERRITORY - the second piece of the mod's own economy, played while the game's computer player is off
// ([ai_base] game_ai = 0; work.c is the first: the citizens' work). The game's build manager, its plan and its
// repair rounds don't run then: nothing was built at all.
// The player, 2026-11-14: "should we handle citezens and buildings over territors too maybe? ... farming and
// woodworks is reserved for save backside territories where possible", and on what to build first: "yes do that, a
// settlment and a tower for every mine".
// Every BD_EVERY per AI, on the territory view (territories.c) and work.c's worked territories (wk_cost):
//   BUILDERS    - a foundation of its nobody builds (a field, a Settlement, a tower, a granary, whatever stands
//                 unfinished: the game's round for them is off too) gets the nearest free citizen on its land,
//                 BD_SENDS a look; none where enemy soldiers stand within 8 tiles or the territory is being lost.
//   FIELDS      - the 8 places round each finished granary (2 tiles apart, as the game lays them out) that hold no
//                 field: a foundation (place_foundation: a field pays its 50 wood when its building starts), 2 a
//                 look, 4 unfinished at most.
//   A GRANARY   - from the Copper Age, none going up, every field laid: the first one at once, the next when
//                 citizens wanted on food found no site (work.c food_short). By a Town Center, Capitol or Settlement
//                 in the territory work.c likes best - backland first, his "farming ... reserved for save backside
//                 territories" -, 4 to 12 tiles out with room for 6 of its 8 fields; BD_GRANARIES at most.
//   A MINE'S SETTLEMENT - every mine of a worked territory, and of a territory beside one (the cheapest by work.c's
//                 cost first, then the nearest to the base: so the home's mines, then the fronts', then the
//                 neighbours' - that is how it takes ground) with no drop site of its within BD_DROP tiles gets a
//                 Settlement 2 to 6 tiles from it; one going up at a time.
//   A MINE'S TOWER - a mine with its drop site and no defence of its within BD_GUARD tiles gets the best tower it can
//                 build, 2 to 5 tiles from it; one going up at a time.
// One new building a look, through the mod's placement (econ.c hook_place: the priority list's savings, the price in
// the bank, a builder, the foundation). None of them under raiders (12 tiles) or in a territory being lost.
// Not here yet: houses, the production buildings, docks, research buildings, repairs - and the old economy's
// placements (defence.c's towers, towns.c, expand.c's outposts) still go through the build manager's queue, which
// nobody empties now.
#define BD_EVERY 5000
#define BD_SENDS 4
#define BD_GRANARIES 8
#define BD_DROP 8
#define BD_GUARD 7
#define BD_WAIT 60000   // after one is laid no other of its kind until its foundation shows (the builder walks there first: one Settlement was laid three times in 25 s)
internal struct bd_st {
    uint32_t t, log_t, laid_t[3];   // laid_t: the last granary, Settlement, tower laid (BD_WAIT)
    unsigned sent, fields, granaries, settlements, towers, refused, no_spot;
    int open, mines, mines_drop, mines_guard;   // the last look: unfinished buildings; mines in reach, those with a drop site, with a defence
} g_bd[16];
// a citizen sent to build (as econ.c's field pass does it: a BuildRepair goal, the builder's state, its script told)
internal int bd_send(ee_player *pl, ee_unit *cit, ee_unit *building) {
    void *mem = goal_build_alloc();
    if (!mem) return 0;
    ee_goal *goal = build_goal_init(mem, pl, building, 1);
    unit_assign_goal(cit, goal, 0, 0);
    unit_set_state(cit, 0x1F51);
    ee_unit_type *td = unit_get_type_data(cit);
    (*(void (__thiscall **)(void *, void *))(unit_type_get_vtable(td) + 0x74))(td, cit);
    return 1;
}
// a spot for a building of `type` r0..r1 tiles from p, nearest first: placeable, on land that is not being lost
internal int bd_spot(ee_player *pl, unsigned idx, int type, ivec2 p, int r0, int r1, ivec2 *out) {
    for (int r = r0; r <= r1; r++)
        for (int ax = -r; ax <= r; ax++)
            for (int ay = -r; ay <= r; ay++) {
                ivec2 q = iv2(p.x + ax, p.y + ay);
                if ((abs(ax) != r && abs(ay) != r) || !can_place(pl, type, q) || tv_unsafe(idx, q.x, q.y)) continue;
                *out = q;
                return 1;
            }
    return 0;
}
internal void build_tick(ee_player *pl, ee_resource_mgr *rm, uint32_t now) {
    static ee_unit *freec[384], *built[192], *openb[96];
    static ivec2 fld[512], gran[BD_GRANARIES + 8], def[512];
    static struct { ivec2 p; int cost; } centre[64];
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct bd_st *b = &g_bd[idx];
    struct tv_st *s = &g_tv[idx];
    (void)rm;
    if (now < b->t) memset(b, 0, sizeof *b);
    if (b->t && now - b->t < BD_EVERY) return;
    b->t = now ? now : 1;
    if (!s->n || s->version != g_ter.version) return;
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0;
    if (!bm || !size) return;
    int nf = 0, nbuilt = 0, no = 0, nfld = 0, ngran = 0, ndef = 0, ncen = 0;
    int open_fields = 0, open_gran = 0, open_settle = 0, open_tower = 0, count = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {   // what it has: citizens, foundations, granaries, fields, defences, centres
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
        int t = unit_type_get_id(td), fam = unit_type_get_class_id(td);
        if (is_citizen_type(t)) {
            ee_goal *g = unit_get_goal(u);
            if (g && !strcmp(rtti_name(g), "EEUGBuildRepair@@")) { if (nbuilt < 192) built[nbuilt++] = goal_get_building(g); }
            else if (nf < 384 && citizen_free(u, 0) && (!g || !strcmp(rtti_name(g), "EEUGGather@@"))) freec[nf++] = u;
            continue;
        }
        if ((fam != CLS_BUILDING && fam != CLS_FORTRESS) || is_wall(t)) continue;
        ee_unit *st = unit_get_physics(u);
        int done = st && unit_get_finished(st);
        ivec2 p = unit_get_tile(u);
        float rng, att;
        if (t == UT(B_FARM)) { if (nfld < 512) fld[nfld++] = p; open_fields += !done; }
        else if (t == UT(B_GRANARY) || t == UT(B_GRANARY_IMPROVED)) { if (done && ngran < BD_GRANARIES + 8) gran[ngran++] = p; open_gran += !done; }
        else if (t == UT(B_SETTLEMENT)) open_settle += !done;
        if (is_tower_type(t)) open_tower += !done;
        if ((is_tower_type(t) || defence_stats(u, &rng, &att)) && ndef < 512) def[ndef++] = p;
        if (done && (t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL) || t == UT(B_SETTLEMENT)) && ncen < 64) {
            centre[ncen].p = p; centre[ncen++].cost = wk_cost(s, ter_near(p.x, p.y));
        }
        if (!done && st && no < 96) openb[no++] = u;
    }
    b->open = no;
    int sends = 0;
    for (int i = 0; i < no && sends < BD_SENDS && nf; i++) {   // BUILDERS
        ee_unit *site = field_site(openb[i]);
        int has = 0, bk = -1, bd = 0x7FFFFFFF;
        for (int k = 0; k < nbuilt && !has; k++) has = built[k] == site || built[k] == openb[i];
        ivec2 p = unit_get_tile(openb[i]);
        if (has || raiders_near(pl, p, 8) || tv_unsafe(idx, p.x, p.y)) continue;
        unsigned land = area_id(p, 2);
        for (int k = 0; k < nf; k++) {
            ivec2 q = unit_get_tile(freec[k]);
            int d = ivec2_dist2(p, q);
            if (d >= bd || (land && area_id(q, 2) && area_id(q, 2) != land)) continue;
            bd = d; bk = k;
        }
        if (bk < 0 || !bd_send(pl, freec[bk], openb[i])) continue;
        freec[bk] = freec[--nf];
        sends++; b->sent++;
    }
    int placed = 0;
    if (ut_has(UT(B_FARM)))   // FIELDS
        for (int g = 0; g < ngran && placed < 2 && open_fields < 4; g++)
            for (int fx = -2; fx <= 2 && placed < 2 && open_fields < 4; fx += 2)
                for (int fy = -2; fy <= 2 && placed < 2 && open_fields < 4; fy += 2) {
                    ivec2 q = iv2(gran[g].x + fx, gran[g].y + fy);
                    int there = !fx && !fy;
                    for (int k = 0; k < nfld && !there; k++) there = fld[k].x == q.x && fld[k].y == q.y;
                    if (there || !can_place(pl, UT(B_FARM), q) || !can_pay(pl, UT(B_FARM)) || raiders_near(pl, q, 8) || tv_unsafe(idx, q.x, q.y)) continue;
                    if (!place_foundation(pl, UT(B_FARM), q.x, q.y)) continue;
                    if (nfld < 512) fld[nfld++] = q;
                    placed++; open_fields++; b->fields++;
                }
    int fields_due = 0;   // places round its granaries still without a field that could hold one
    for (int g = 0; g < ngran; g++)
        for (int fx = -2; fx <= 2; fx += 2)
            for (int fy = -2; fy <= 2; fy += 2) {
                ivec2 q = iv2(gran[g].x + fx, gran[g].y + fy);
                int there = !fx && !fy;
                for (int k = 0; k < nfld && !there; k++) there = fld[k].x == q.x && fld[k].y == q.y;
                fields_due += !there && can_place(pl, UT(B_FARM), q);
            }
    const char *what = NULL;
    int type = 0, ok = 0, wait[3];
    for (int k = 0; k < 3; k++) wait[k] = b->laid_t[k] && now >= b->laid_t[k] && now - b->laid_t[k] < BD_WAIT;
    ivec2 at = iv2(0, 0);
    if (!placed && current_epoch(pl) >= FARM_EPOCH && ut_has(UT(B_GRANARY)) && can_build(pl, UT(B_GRANARY)) && !open_gran && !open_fields && !fields_due &&
        ngran < BD_GRANARIES && (!ngran || g_wk[idx].food_short > 0) && !wait[0]) {   // A GRANARY: by its centre in the best territory
        for (int pass = 1; pass <= 3 && !type; pass++)
            for (int c = 0; c < ncen && !type; c++) {
                if (centre[c].cost != pass) continue;
                for (int r = 4; r <= 12 && !type; r++)
                    for (int ax = -r; ax <= r && !type; ax++)
                        for (int ay = -r; ay <= r && !type; ay++) {
                            ivec2 q = iv2(centre[c].p.x + ax, centre[c].p.y + ay);
                            int room = 0;
                            if ((abs(ax) != r && abs(ay) != r) || !can_place(pl, UT(B_GRANARY), q) || raiders_near(pl, q, 12) || tv_unsafe(idx, q.x, q.y)) continue;
                            for (int fx = -2; fx <= 2; fx += 2)
                                for (int fy = -2; fy <= 2; fy += 2)
                                    if ((fx || fy) && can_place(pl, UT(B_FARM), iv2(q.x + fx, q.y + fy))) room++;
                            if (room < 6) continue;
                            type = UT(B_GRANARY); at = q; what = "a granary for the fields";
                        }
            }
        if (!type) b->no_spot++;
    }
    b->mines = b->mines_drop = b->mines_guard = 0;
    {   // A MINE'S SETTLEMENT, A MINE'S TOWER: the cheapest territory's first, then the nearest to the base
        ivec2 base = build_mgr_get_base(bm), sp = iv2(0, 0), tp = iv2(0, 0);
        int sbest = 0x7FFFFFFF, tbest = 0x7FFFFFFF, ttype = tower_newest(pl);
        drops_cached(pl);
        for (int k = 0; k < g_ter.nsite; k++) {
            int r = g_ter.site[k].res;
            ivec2 p = iv2(g_ter.site[k].x, g_ter.site[k].y);
            if (r < 4 || r > 6) continue;
            int c = wk_cost(s, ter_near(p.x, p.y));
            ee_unit *m = c ? wk_tile_site(map, size, p.x, p.y, r) : NULL;
            if (!m || !explored_by(m, pl)) continue;
            b->mines++;
            int dk = -1, dd = drop_nearest(idx, p, 0, &dk), guard = 0, score = c * 1000000 + ivec2_dist2(p, base);
            int has_drop = dk >= 0 && dd <= BD_DROP * BD_DROP;
            for (int j = 0; j < ndef && !guard; j++) guard = abs(def[j].x - p.x) <= BD_GUARD && abs(def[j].y - p.y) <= BD_GUARD;
            b->mines_drop += has_drop; b->mines_guard += guard;
            if (raiders_near(pl, p, 12) || tv_unsafe(idx, p.x, p.y)) continue;
            if (!has_drop) { if (score < sbest) { sbest = score; sp = p; } }
            else if (!guard && score < tbest) { tbest = score; tp = p; }
        }
        if (!type && !placed && sbest < 0x7FFFFFFF && !open_settle && !wait[1] && ut_has(UT(B_SETTLEMENT)) && can_build(pl, UT(B_SETTLEMENT))) {
            if (bd_spot(pl, idx, UT(B_SETTLEMENT), sp, 2, 6, &at)) { type = UT(B_SETTLEMENT); what = "a Settlement by a mine"; } else b->no_spot++;
        }
        if (!type && !placed && tbest < 0x7FFFFFFF && !open_tower && !wait[2] && ttype && can_build(pl, ttype)) {
            if (bd_spot(pl, idx, ttype, tp, 2, 5, &at)) { type = ttype; what = "a tower by a mine"; } else b->no_spot++;
        }
    }
    if (type) {
        census_build_ask(bm, type);   // the test log's count: the mod's, not the game's entry
        ok = hook_place(bm, type, at.x, at.y);
        if (!ok) b->refused++;
        else if (type == UT(B_GRANARY)) { b->granaries++; b->laid_t[0] = now ? now : 1; }
        else if (type == UT(B_SETTLEMENT)) { b->settlements++; b->laid_t[1] = now ? now : 1; }
        else { b->towers++; b->laid_t[2] = now ? now : 1; }
        if (g_log && (ok || !(b->refused % 20)))
            log_locked("# build: P%u at %.1f min: %s - %d at %d,%d (territory %u): %s\n", idx, now / 60000.0, what, type, at.x, at.y, ter_near(at.x, at.y),
                       ok ? "laid" : "not laid (the price not in the bank, or held back for what the priority list saves for)");
    }
    if (g_log && (!b->log_t || now - b->log_t >= 60000)) {
        b->log_t = now ? now : 1;
        log_locked("# build: P%u at %.1f min: %d granaries, %d fields (%d going up, %d places still without one), %d unfinished buildings | mines in reach %d, with a drop site %d, with a defence %d | "
                   "so far %u builders sent, %u fields, %u granaries, %u Settlements, %u towers laid, %u not laid, %u without a spot\n",
                   idx, now / 60000.0, ngran, nfld, open_fields, fields_due, no, b->mines, b->mines_drop, b->mines_guard, b->sent, b->fields, b->granaries, b->settlements, b->towers, b->refused, b->no_spot);
    }
}
internal void build_new_game(void) { memset(g_bd, 0, sizeof g_bd); }
