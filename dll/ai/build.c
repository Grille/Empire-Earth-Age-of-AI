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
//   THE RESEARCH BUILDINGS - one University, one Hospital and one Temple (research.c's rank 1 is researched there;
//                 the Hospital holds the population techs too, the Temple keeps the enemy's prophets' disasters
//                 off the town and its range techs are of rank 1), each as soon as the game offers it: by a Town Center or Capitol
//                 in the territory work.c likes best, 4 to 10 tiles out. Before the mines' buildings; one it can't
//                 pay is what the row "research" of the priority list saves for (g_rs_want: Settlements and
//                 towers wait, the granary and the fields go on).
//   PRODUCTION  - the building produce.c asks for (a territory's first production, a hub's next kind, one more of a
//                 busy kind), 3 to 10 tiles from a Town Center, Capitol or Settlement of its in that territory;
//                 after the research buildings, before the mines' (one it can't pay yet waits, the mines' go on).
//                 Its very first production building comes right after the University and is saved for as a
//                 research building is (the first look: behind the Hospital and the Temple it stood at minute 17
//                 to 25, no soldier until then; the player 2026-11-08: one production building until 2 farms).
//   A MINE'S SETTLEMENT - every mine of a worked territory, and of a territory beside one (the cheapest by work.c's
//                 cost first, then the nearest to the base: so the home's mines, then the fronts', then the
//                 neighbours' - that is how it takes ground) with no drop site of its within BD_DROP tiles gets a
//                 Settlement 2 to 6 tiles from it; one going up at a time.
//   A MINE'S TOWER - a mine with its drop site and no defence of its within BD_GUARD tiles gets the best tower it can
//                 build, 2 to 5 tiles from it; one going up at a time.
// One new building a look, through the mod's placement (econ.c hook_place: the priority list's savings, the price in
// the bank, a builder, the foundation). None of them under raiders (12 tiles), in the reach of an enemy tower it has
// explored (work.c wk_danger), or in a territory being lost - nor a builder sent there.
// Not here yet: houses, docks, repairs - and the old economy's
// placements (defence.c's towers, towns.c, expand.c's outposts) still go through the build manager's queue, which
// nobody empties now.
#define BD_EVERY 5000
#define BD_SENDS 4
#define BD_GRANARIES 96   // no limit of its own (8 fed 64 farmers: the player's game of 2026-11-15, 55 citizens on food, 215 on wood with no field to go to, 25 food in the bank): one more whenever citizens wanted on food find no site
#define BD_DROP 8
#define BD_DSITES 2048
#define BD_GUARD 7
#define BD_CLAIM_EVERY 120000   // between two claims
#define BD_SINK 10000           // wood in the bank from which it builds unasked
#define BD_SINK_EVERY 20000
#define BD_SINK_MAX 3           // of a kind in one territory
#define BD_SINK_OPEN 4          // unfinished buildings
#define BD_SINK_T 4096
#define BD_CLAIM_GUARD 3        // its soldiers in the held territory a claim starts from
#define BD_WAIT 60000   // after one is laid no other of its kind until its foundation shows (the builder walks there first: one Settlement was laid three times in 25 s)
internal struct bd_st {
    uint32_t claim_t; unsigned claims;   // A CLAIM: the last one, how many
    uint32_t gran_ask_t; int gran_room;  // A GRANARY BEFORE THE NEXT: asked since, the granaries with room then
    uint32_t sink_t; unsigned sinks;     // THE WOOD SINK: the same
    uint32_t t, log_t, laid_t[5];   // laid_t: the last granary, Settlement, tower, research building, production building laid (BD_WAIT)
    int laid_type[5];               // ... and its type
    unsigned sent, fields, granaries, settlements, towers, research, production, refused, no_spot;
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
// the bank pays one of type with what is laid and not paid yet (com) left alone
// A GRANARY BEFORE THE NEXT (garrison.c): 1 lay the next granary; 0 one of its has room inside - garrison.c is asked to
// fill it instead. Asked for BD_GRAN_ASK without the room going (nobody near to send): laid all the same.
#define BD_GRAN_ASK 180000
#define BD_GRAN_PLAIN 3   // its first granaries are laid without it: 8 citizens inside one at minute 15 were a quarter of its workforce
internal int bd_gran_first(struct bd_st *b, unsigned idx, int gran_room, uint32_t now) {
    if (!gran_room) { b->gran_ask_t = 0; return 1; }
    if (!b->gran_ask_t || now < b->gran_ask_t || gran_room != b->gran_room) { b->gran_ask_t = now ? now : 1; b->gran_room = gran_room; }
    if (now - b->gran_ask_t >= BD_GRAN_ASK) { b->gran_ask_t = 0; return 1; }
    g_gu_gran[idx] = now ? now : 1;
    return 0;
}
internal int bd_pays(ee_player *pl, int type, const int *com) {
    const int *stock = player_stock_at(pl);
    for (int r = 2; r < 7; r++) { int c = unit_cost(pl, type, r); if (c > 0 && stock[r] - com[r] < c) return 0; }
    return 1;
}
// a spot for a building of `type` r0..r1 tiles from p, nearest first: placeable, on land that is not being lost
internal int bd_spot(ee_player *pl, unsigned idx, int type, ivec2 p, int r0, int r1, ivec2 *out) {
    for (int r = r0; r <= r1; r++)
        for (int ax = -r; ax <= r; ax++)
            for (int ay = -r; ay <= r; ay++) {
                ivec2 q = iv2(p.x + ax, p.y + ay);
                if ((abs(ax) != r && abs(ay) != r) || !can_place(pl, type, q) || tv_unsafe(idx, q.x, q.y) || wk_danger(idx, q)) continue;
                *out = q;
                return 1;
            }
    return 0;
}
internal void build_tick(ee_player *pl, ee_resource_mgr *rm, uint32_t now) {
    static ee_unit *freec[384], *built[192], *openb[96];
    static ivec2 fld[512], gran[BD_GRANARIES + 8], def[512];
    static struct { ivec2 p; int cost, town; } centre[256];
    static ivec2 dsite[BD_DSITES];   // every Settlement, Town Center and Capitol of its, going up too: a mine's drop site (below)
    int nds = 0;
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
    int gran_room = 0;   // its finished granaries with room inside (A GRANARY BEFORE THE NEXT)
    int open_fields = 0, open_gran = 0, open_settle = 0, open_tower = 0, open_prod = 0, count = 0, have_rb[3] = { 0, 0, 0 };
    const int rb[3] = { UT(B_UNIVERSITY), UT(B_HOSPITAL), UT(B_TEMPLE) };
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
        else if (t == UT(B_GRANARY) || t == UT(B_GRANARY_IMPROVED)) { if (done && ngran < BD_GRANARIES + 8) gran[ngran++] = p; open_gran += !done; gran_room += done && unit_type_get_garrison_capacity(td) > unit_get_garrisoned(u) && wk_cost(s, ter_near(p.x, p.y)); }
        else if (t == UT(B_SETTLEMENT)) open_settle += !done;
        else if (pr_kind(t) >= 0) open_prod += !done;
        for (int k = 0; k < 3; k++) if (t == rb[k]) have_rb[k]++;   // going up counts
        if (is_tower_type(t)) open_tower += !done;
        if ((is_tower_type(t) || defence_stats(u, &rng, &att)) && ndef < 512) def[ndef++] = p;
        if ((t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL) || t == UT(B_SETTLEMENT)) && nds < BD_DSITES) dsite[nds++] = p;
        if (done && (t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL) || t == UT(B_SETTLEMENT)) && ncen < 256) {
            centre[ncen].p = p; centre[ncen].town = t != UT(B_SETTLEMENT); centre[ncen++].cost = wk_cost(s, ter_near(p.x, p.y));
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
        if (has || raiders_near(pl, p, 8) || tv_unsafe(idx, p.x, p.y) || wk_danger(idx, p)) continue;
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
    // LAID, NOT PAID: a building laid through the placement is paid when its builder gets there and the foundation
    // shows, a field when its building starts - until then (BD_WAIT at most) the price is this file's to keep: no
    // other building of its takes it, and the row "laid" of the priority list holds it from everybody else (the first
    // look at the research buildings: the University laid at 0.3 min, its 175 wood gone into fields and a tech before
    // the builder arrived - it came 5 to 8 minutes later, with every AI)
    int wait[5], shown[5] = { open_gran, open_settle, open_tower, 0, open_prod }, com[7] = { 0 };
    for (int k = 0; k < 5; k++) wait[k] = b->laid_t[k] && now >= b->laid_t[k] && now - b->laid_t[k] < BD_WAIT;
    for (int k = 0; k < 3; k++) if (b->laid_type[3] == rb[k]) shown[3] = have_rb[k];
    for (int k = 0; k < 5; k++)
        if (wait[k] && !shown[k] && b->laid_type[k] > 0) for (int r = 2; r < 7; r++) com[r] += unit_cost(pl, b->laid_type[k], r);
    for (int r = 2; r < 7 && ut_has(UT(B_FARM)); r++) com[r] += open_fields * unit_cost(pl, UT(B_FARM), r);
    memcpy(g_bd_com[idx], com, sizeof com);
    int placed = 0;
    if (ut_has(UT(B_FARM)))   // FIELDS
        for (int g = 0; g < ngran && placed < 2 && open_fields < 4; g++)
            for (int fx = -2; fx <= 2 && placed < 2 && open_fields < 4; fx += 2)
                for (int fy = -2; fy <= 2 && placed < 2 && open_fields < 4; fy += 2) {
                    ivec2 q = iv2(gran[g].x + fx, gran[g].y + fy);
                    int there = !fx && !fy;
                    for (int k = 0; k < nfld && !there; k++) there = fld[k].x == q.x && fld[k].y == q.y;
                    if (there || !can_place(pl, UT(B_FARM), q) || !bd_pays(pl, UT(B_FARM), com) || raiders_near(pl, q, 8) || tv_unsafe(idx, q.x, q.y) || wk_danger(idx, q)) continue;
                    if (!place_foundation(pl, UT(B_FARM), q.x, q.y)) continue;
                    if (nfld < 512) fld[nfld++] = q;
                    placed++; open_fields++; b->fields++;
                    for (int r = 2; r < 7; r++) com[r] += unit_cost(pl, UT(B_FARM), r);
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
    int type = 0, ok = 0;
    ivec2 at = iv2(0, 0);
    if (!placed && current_epoch(pl) >= FARM_EPOCH && ut_has(UT(B_GRANARY)) && can_build(pl, UT(B_GRANARY)) && bd_pays(pl, UT(B_GRANARY), com) && !open_gran && !open_fields && !fields_due &&
        ngran < BD_GRANARIES && (!ngran || g_wk[idx].food_short > 0) && !wait[0] && bd_gran_first(b, idx, ngran >= BD_GRAN_PLAIN ? gran_room : 0, now)) {   // A GRANARY: by its centre in the best territory
        for (int pass = 1; pass <= 3 && !type; pass++)
            for (int c = 0; c < ncen && !type; c++) {
                if (centre[c].cost != pass) continue;
                for (int r = 4; r <= 12 && !type; r++)
                    for (int ax = -r; ax <= r && !type; ax++)
                        for (int ay = -r; ay <= r && !type; ay++) {
                            ivec2 q = iv2(centre[c].p.x + ax, centre[c].p.y + ay);
                            int room = 0;
                            if ((abs(ax) != r && abs(ay) != r) || !can_place(pl, UT(B_GRANARY), q) || raiders_near(pl, q, 12) || tv_unsafe(idx, q.x, q.y) || wk_danger(idx, q)) continue;
                            for (int fx = -2; fx <= 2; fx += 2)
                                for (int fy = -2; fy <= 2; fy += 2)
                                    if ((fx || fy) && can_place(pl, UT(B_FARM), iv2(q.x + fx, q.y + fy))) room++;
                            if (room < 6) continue;
                            type = UT(B_GRANARY); at = q; what = "a granary for the fields";
                        }
            }
        if (!type) b->no_spot++;
    }
    g_rs_want[idx] = 0;
    produce_look(pl, idx, now);   // produce.c: the production building its territories ask for
    int saving = 0;   // one it can't pay is saved for (the row "research"): nothing after it in this order meanwhile
    for (int k = 0; k < 4 && !type && !placed && !saving; k++) {   // THE RESEARCH BUILDINGS - and, after the University, its first production building
        if (k == 1) {
            if (!g_pr[idx].type || !g_pr[idx].first || wait[4] || open_prod) continue;
            if (!bd_pays(pl, g_pr[idx].type, com)) { g_rs_want[idx] = g_pr[idx].type + 1; saving = 1; continue; }
            if (bd_spot(pl, idx, g_pr[idx].type, g_pr[idx].at, 3, 10, &at)) { type = g_pr[idx].type; what = g_pr[idx].why; } else b->no_spot++;
            continue;
        }
        int j = k ? k - 1 : 0;
        if (wait[3] || have_rb[j] || !ut_has(rb[j]) || !prio_buildable(pl, rb[j])) continue;
        if (!bd_pays(pl, rb[j], com)) { g_rs_want[idx] = rb[j] + 1; saving = 1; continue; }
        for (int pass = 1; pass <= 3 && !type; pass++)
            for (int c = 0; c < ncen && !type; c++)
                if (centre[c].town && centre[c].cost == pass && bd_spot(pl, idx, rb[j], centre[c].p, 4, 10, &at)) { type = rb[j]; what = j == 2 ? "a Temple" : j ? "a Hospital" : "a University"; }
        if (!type) b->no_spot++;
    }
    if (!type && !placed && g_pr[idx].type && !g_pr[idx].first && !wait[4] && !open_prod && bd_pays(pl, g_pr[idx].type, com)) {   // PRODUCTION
        if (bd_spot(pl, idx, g_pr[idx].type, g_pr[idx].at, 3, 10, &at)) { type = g_pr[idx].type; what = g_pr[idx].why; } else b->no_spot++;
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
            // its own count, not sites.c's cache of 64 drop sites (granaries among them): past 64 a Settlement just laid
            // was never seen and the mine got the next one - twenty round one mine, 303 laid (the player's game of 2026-11-15)
            int guard = 0, score = c * 1000000 + ivec2_dist2(p, base), has_drop = 0;
            for (int j = 0; j < nds && !has_drop; j++) has_drop = ivec2_dist2(dsite[j], p) <= BD_DROP * BD_DROP;
            for (int j = 0; j < ndef && !guard; j++) guard = abs(def[j].x - p.x) <= BD_GUARD && abs(def[j].y - p.y) <= BD_GUARD;
            b->mines_drop += has_drop; b->mines_guard += guard;
            if (raiders_near(pl, p, 12) || tv_unsafe(idx, p.x, p.y) || wk_danger(idx, p)) continue;   // not a mine under an enemy tower it has seen
            if (!has_drop) { if (score < sbest) { sbest = score; sp = p; } }
            else if (!guard && score < tbest) { tbest = score; tp = p; }
        }
        if (!type && !placed && sbest < 0x7FFFFFFF && !open_settle && !wait[1] && ut_has(UT(B_SETTLEMENT)) && can_build(pl, UT(B_SETTLEMENT)) && bd_pays(pl, UT(B_SETTLEMENT), com)) {
            if (bd_spot(pl, idx, UT(B_SETTLEMENT), sp, 2, 6, &at)) { type = UT(B_SETTLEMENT); what = "a Settlement by a mine"; } else b->no_spot++;
        }
        if (!type && !placed && tbest < 0x7FFFFFFF && !open_tower && !wait[2] && ttype && can_build(pl, ttype) && bd_pays(pl, ttype, com)) {
            if (bd_spot(pl, idx, ttype, tp, 2, 5, &at)) { type = ttype; what = "a tower by a mine"; } else b->no_spot++;
        }
        // A CLAIM (the player 2026-11-15: "controlling the whole map is the endgoal ... but within reason"; "the ai also
        // dosnt yet clam territoires that have no resources in it, they are still needed for map control"): a tower in
        // the middle of a territory nobody has a building in, beside one it holds where BD_CLAIM_GUARD of its soldiers
        // stand (the border is secured first) - the nearest to its base, one every BD_CLAIM_EVERY, when nothing else is
        // to lay and it has min_workforce citizens. The tower makes the territory its own (territories.c TV_HOLDS): the
        // shares station soldiers there, its citizens may work there and beside it, and the next claim starts from it.
        // ... and a building (the player, the same day: "a building and a tower"): a Settlement beside the tower of a
        // claim that has nothing else yet - before the next claim
        int lone = -1;
        for (int r = 0; r < s->n && lone < 0; r++)
            if (!g_ter.t[r].water && s->t[r].role != TVR_NONE && s->t[r].state == TV_QUIET && s->t[r].bld == 1 && s->t[r].hold >= 1) lone = r;   // its tower stands finished (territories.c TV_HOLDS)
        if (!type && !placed && lone >= 0 && !open_settle && !wait[1] && ut_has(UT(B_SETTLEMENT)) && can_build(pl, UT(B_SETTLEMENT)) && bd_pays(pl, UT(B_SETTLEMENT), com)) {
            ivec2 c = g_ter.t[lone].c;
            for (int j = 0; j < ndef; j++) if ((int)ter_near(def[j].x, def[j].y) == lone) c = iv2(def[j].x, def[j].y);
            if (!raiders_near(pl, c, 12) && !wk_danger(idx, c) && bd_spot(pl, idx, UT(B_SETTLEMENT), c, 2, 8, &at)) { type = UT(B_SETTLEMENT); what = "a claim's Settlement beside its tower"; }
        }
        if (!type && !placed && lone < 0 && !open_tower && !wait[2] && ttype && can_build(pl, ttype) && bd_pays(pl, ttype, com) && workforce(pl) >= g_min_workforce &&
            (!b->claim_t || now < b->claim_t || now - b->claim_t >= BD_CLAIM_EVERY)) {
            int cb = -1, cd = 0;
            for (int r = 0; r < s->n; r++) {
                struct tv_terr *t = &s->t[r];
                if (g_ter.t[r].water || t->role != TVR_NONE || t->bld || t->ebld || t->abld || t->state != TV_QUIET) continue;
                int guarded = 0;
                for (int k = 0; k < g_ter.t[r].nbn && !guarded; k++) {
                    unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
                    guarded = o < (unsigned)s->n && !g_ter.t[o].water && s->t[o].role != TVR_NONE && s->t[o].state == TV_QUIET && s->t[o].sold >= BD_CLAIM_GUARD;
                }
                ivec2 c = g_ter.t[r].c;
                if (!guarded || raiders_near(pl, c, 12) || wk_danger(idx, c)) continue;
                int d = ivec2_dist2(c, base);
                if (cb < 0 || d < cd) { cb = r; cd = d; }
            }
            if (cb >= 0) {
                if (bd_spot(pl, idx, ttype, g_ter.t[cb].c, 0, 8, &at)) { type = ttype; what = "a claim: a tower in a territory nobody holds"; b->claim_t = now ? now : 1; b->claims++; } else b->no_spot++;
            }
        }
    }
    // THE WOOD SINK (the player 2026-11-15: "the ai sould also be building more in general, right now it sits on 130k
    // wood. these can be spend in front territoies on hospitals, temples and production buildings (even when nobody
    // asked) above 10k wood it should try to get rid of it"): BD_SINK wood in the bank and nothing else to lay - a
    // Hospital, a Temple or a production building in a quiet front territory of its, the kind that territory has the
    // fewest of (BD_SINK_MAX of a kind in one at most), one every BD_SINK_EVERY while fewer than BD_SINK_OPEN of its
    // buildings stand unfinished.
    if (!type && !placed && player_stock_at(pl)[3] >= BD_SINK && no < BD_SINK_OPEN && (!b->sink_t || now < b->sink_t || now - b->sink_t >= BD_SINK_EVERY)) {
        int kinds[2 + PR_NK], nk = 0;
        kinds[nk++] = rb[1]; kinds[nk++] = rb[2];
        for (int k = 0; k < PR_NK; k++) kinds[nk++] = g_ut[PR_KINDS[k]];
        static uint8_t have[BD_SINK_T][2 + PR_NK];
        int tn = s->n < BD_SINK_T ? s->n : BD_SINK_T, cnt = 0;
        memset(have, 0, (size_t)tn * sizeof *have);
        for (ee_unit *u = player_get_units(pl); u && cnt < 16384; u = unit_get_next(u), cnt++) {
            ee_unit_type *td = unit_get_type_data(u);
            if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
            unsigned r = ter_near(unit_get_x(u), unit_get_y(u));
            for (int k = 0; r < (unsigned)tn && k < nk; k++) if (kinds[k] >= 0 && unit_type_get_id(td) == kinds[k] && have[r][k] < 255) have[r][k]++;
        }
        int br = -1, bk = -1, bn = 0;
        for (int r = 0; r < tn; r++) {
            if (g_ter.t[r].water || s->t[r].role != TVR_FRONT || s->t[r].state != TV_QUIET) continue;
            for (int k = 0; k < nk; k++) {
                if (kinds[k] < 0 || have[r][k] >= BD_SINK_MAX || !ut_has(kinds[k]) || !prio_buildable(pl, kinds[k]) || !bd_pays(pl, kinds[k], com)) continue;
                if (br < 0 || have[r][k] < bn) { br = r; bk = k; bn = have[r][k]; }
            }
        }
        if (br >= 0) {
            ivec2 c = g_ter.t[br].c;
            for (int j = 0; j < ncen; j++) if ((int)ter_near(centre[j].p.x, centre[j].p.y) == br) c = centre[j].p;
            if (!raiders_near(pl, c, 12) && !wk_danger(idx, c) && bd_spot(pl, idx, kinds[bk], c, 3, 14, &at)) { type = kinds[bk]; what = "the wood sink: one more in a front territory"; b->sink_t = now ? now : 1; b->sinks++; }
            else b->no_spot++;
        }
    }
    if (type) {
        census_build_ask(bm, type);   // the test log's count: the mod's, not the game's entry
        ok = hook_place(bm, type, at.x, at.y);
        if (!ok) b->refused++;
        else {
            int k = type == UT(B_GRANARY) ? 0 : type == UT(B_SETTLEMENT) ? 1 : type == rb[0] || type == rb[1] || type == rb[2] ? 3 : pr_kind(type) >= 0 ? 4 : 2;
            if (k == 0) b->granaries++; else if (k == 1) b->settlements++; else if (k == 3) b->research++; else if (k == 4) { b->production++; g_pr[idx].type = 0; if (type == g_pr_more[idx]) g_pr_more[idx] = 0; } else b->towers++;
            b->laid_t[k] = now ? now : 1; b->laid_type[k] = type;
            for (int r = 2; r < 7; r++) g_bd_com[idx][r] += unit_cost(pl, type, r);
        }
        if (g_log && (ok || !(b->refused % 20)))
            log_locked("# build: P%u at %.1f min: %s - %d at %d,%d (territory %u): %s\n", idx, now / 60000.0, what, type, at.x, at.y, ter_near(at.x, at.y),
                       ok ? "laid" : "not laid (the price not in the bank, or held back for what the priority list saves for)");
    }
    if (g_log && (!b->log_t || now - b->log_t >= 60000)) {
        b->log_t = now ? now : 1;
        log_locked("# build: P%u at %.1f min: %d granaries, %d fields (%d going up, %d places still without one), %d unfinished buildings | mines in reach %d, with a drop site %d, with a defence %d | "
                   "so far %u builders sent, %u fields, %u granaries, %u Settlements, %u towers, %u research and %u production buildings laid, %u not laid, %u without a spot\n",
                   idx, now / 60000.0, ngran, nfld, open_fields, fields_due, no, b->mines, b->mines_drop, b->mines_guard, b->sent, b->fields, b->granaries, b->settlements, b->towers, b->research, b->production, b->refused, b->no_spot);
    }
}
internal void build_new_game(void) { memset(g_bd, 0, sizeof g_bd); memset(g_bd_com, 0, sizeof g_bd_com); produce_new_game(); }
