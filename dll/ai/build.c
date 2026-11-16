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
//   A WATCH TOWER - with nothing else to lay, the largest place of a territory it holds that it rarely has in sight
//                 (watch.c) gets a tower; one every BD_WATCH_EVERY ([ai_base] watch_towers).
// One new building a look, through the mod's placement (econ.c hook_place: the priority list's savings, the price in
// the bank, a builder, the foundation). None of them under raiders (12 tiles), in the reach of an enemy tower it has
// explored (work.c wk_danger), or in a territory being lost - nor a builder sent there.
// Not here yet: houses, docks, repairs - and the old economy's
// placements (defence.c's towers, towns.c, expand.c's outposts) still go through the build manager's queue, which
// nobody empties now.
#define BD_EVERY 5000
#define BD_SENDS 4
#define BD_WALL_SENDS 2   // builders sent to unfinished wall pieces a look (WALLS)
#define BD_WALL_RUN 4     // a builder on a piece within this many tiles: the stretch has one
#define BD_PILE 5   // unfinished buildings from which each gets a second builder
#define BD_GRANARIES 96   // no limit of its own (8 fed 64 farmers: the player's game of 2026-11-15, 55 citizens on food, 215 on wood with no field to go to, 25 food in the bank): one more whenever citizens wanted on food find no site
#define BD_DROP 8
#define BD_DROP_METAL 3   // a gold or iron mine's: the Settlement's middle this near (its half, a tile's gap, the mine)
#define BD_DSITES 2048
#define BD_AP_SAVE 300000
#define BD_GUARD 7
#define BD_CLAIM_EVERY 120000   // between two claims
#define BD_HOUSES_TC 2          // THE MORALE'S HOUSES: the levels a Town Center's morale takes (the game's data)
#define BD_HOUSES_CAPITOL 4     // ... a Capitol's
#define BD_HOUSE_REACH 9        // ... a House's tile within this of the building's, each way (the square is 10)
#define BD_UNI_EVERY 60000      // A UNIVERSITY A TERRITORY: one laid at most this often
#define BD_UNI_PEND 180000      // ... and the last one laid counts this long (its builder walks there)
#define BD_HOUSE_EVERY 60000
#define BD_HOUSE_PEND 180000
#define BD_SINK 10000           // wood in the bank from which it builds unasked
#define BD_SINK_EVERY 20000
#define BD_SINK_MAX 3           // of a kind in one territory
#define BD_SINK_OPEN 4          // unfinished buildings
#define BD_SINK_T 4096
#define BD_CLAIM_GUARD 3        // its soldiers in the held territory a claim starts from
#define BD_CLAIM_MINE 2         // a territory with a mine counts as this many times nearer to the base
#define BD_CLAIM_OPEN 2         // its claims not holding yet that hold the next back
#define BD_CLAIM_LONE 360000  // a claim's Settlement still missing holds the next claim back this long at most
#define BD_WATCH_EVERY 120000   // A WATCH TOWER: between two
#define BD_WATCH_NEAR 8         // ... none within this many tiles of a defence of its
#define BD_WAIT 60000   // after one is laid no other of its kind until its foundation shows (the builder walks there first: one Settlement was laid three times in 25 s)
internal struct bd_st {
    uint32_t claim_t; unsigned claims;   // A CLAIM: the last one, how many
    const char *claim_why; int claim_n[4];   // ... the last look: what held it back; territories nobody holds, quiet, beside a guarded one of its, safe
    struct { int r; uint32_t t; } claim_open[8]; unsigned nclaim_open;   // ... the last claims: territory, when (CLAIMS GOING UP)
    uint32_t watch_t;                    // A WATCH TOWER: the last one
    uint32_t ap_save_t;                  // AN AIRPORT SAVED FOR: since
    uint32_t gran_ask_t; int gran_room;  // A GRANARY BEFORE THE NEXT: asked since, the granaries with room then
    uint32_t sink_t; unsigned sinks;     // THE WOOD SINK: the same
    uint32_t uni_t; unsigned unis; ivec2 uni_at;   // A UNIVERSITY A TERRITORY: the last laid, how many, where
    uint32_t house_t; unsigned houses; ivec2 house_at;   // THE MORALE'S HOUSES: the same, and where the last was laid
    uint32_t t, log_t, laid_t[5];   // laid_t: the last granary, Settlement, tower, research building, production building laid (BD_WAIT)
    int laid_type[5];               // ... and its type
    unsigned sent, fields, granaries, settlements, towers, research, production, refused, no_spot, walls;   // walls: builders sent to unfinished wall pieces
    int open, mines, mines_drop, mines_guard;   // the last look: unfinished buildings; mines in reach, those with a drop site, with a defence
} g_bd[16];
// a citizen sent to build (as econ.c's field pass does it: a BuildRepair goal, the builder's state, its script told)
internal int sn_holding(unsigned idx, int r, ivec2 *at);   // stations.c: a push of its holds the territory it cleared
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
// AREA OF EFFECT (the player 2026-11-16: "placement of hospitals and temples is a bit wastefull right now ... overlapp
// does nothing. they should have a minimum distance of their current area of affect"): the reach of its Hospital,
// Temple or University now, in tiles from the middle - the game's circles for them (dbareaeffect's graphics: Hospital
// Range 3 to 7, Temple Range 8 to 16, University Range 8 to 18) by the range upgrades it has researched: the techs
// with an effect on the building itself of kind 2, attribute 0x28 (its area effect swapped for the next one; Temple
// 2051 4060 6061 9060, Hospital 4008 7002 9010 12051, University 4006 6002 8012 10000 13005). 0: none of those.
#define BD_AOE_ATTR 0x28
internal int bd_reach(ee_player *pl, int type) {
    int base = 0, step = 0;
    if (type == UT(B_HOSPITAL)) { base = 3; step = 1; }
    else if (type == UT(B_TEMPLE) || type == UT(B_UNIVERSITY)) { base = 8; step = 2; }
    ee_tech_tree *tt = pl ? player_get_tech_tree(pl) : NULL;
    if (!base || !tt) return base;
    int level = 0;
    for (int id = 14; id < 15000; id++) {
        ee_tech *t = tech_tree_lookup(tt, id);
        ee_tech_data *d = t && tech_get_researched(t) ? tech_get_data(t) : NULL;
        int ei = d ? tech_data_get_effects(d) : -1;
        if (ei < 0 || ei > 0x5DC) continue;
        ee_epoch_techs *list = epoch_tech_list(ei);
        unsigned n = epoch_techs_get_count(list), *ids = epoch_techs_get_ids(list);
        for (unsigned k = 0; ids && k < n && k < 64; k++) {
            if (ids[k] >= 2000) continue;
            ee_effect *ef = tech_effect(ids[k]);
            if (effect_get_type(ef) == type && effect_get_kind(ef) == 2 && effect_get_attr(ef) == BD_AOE_ATTR) { level++; break; }
        }
    }
    return base + step * level;
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
internal uint32_t g_bd_airport[16];   // airstrike.c: an Airport asked for, when (AN AIRPORT)
internal int g_bd_airport_n[16];      // ... how many it wants in all
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
                if ((abs(ax) != r && abs(ay) != r) || !can_place_open(pl, type, q) || tv_unsafe(idx, q.x, q.y) || wk_danger(idx, q)) continue;
                *out = q;
                return 1;
            }
    return 0;
}
internal void build_tick(ee_player *pl, ee_resource_mgr *rm, uint32_t now) {
    static ee_unit *freec[384], *built[192], *openb[96], *openw[256];
    static ivec2 uni[256];   // its Universities' middles (going up counts): A UNIVERSITY A TERRITORY
    int nuni = 0;
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
    int nf = 0, nbuilt = 0, no = 0, nw = 0, nfld = 0, ngran = 0, ndef = 0, ncen = 0;
    int nairport = 0;
    int gran_room = 0;   // its finished granaries with room inside (A GRANARY BEFORE THE NEXT)
    static ivec2 house[512];
    static struct { ivec2 p; int max; } town[128];
    int nhouse = 0, ntown = 0, open_house = 0;
    int open_fields = 0, open_gran = 0, open_settle = 0, open_tower = 0, open_prod = 0, open_sink = 0, count = 0, have_rb[3] = { 0, 0, 0 };
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
        if (fam != CLS_BUILDING && fam != CLS_FORTRESS) continue;
        if (is_wall(t)) {   // its wall pieces going up: WALLS below
            ee_unit *ws = unit_get_physics(u);
            if (ws && !unit_get_finished(ws) && nw < 256) openw[nw++] = u;
            continue;
        }
        ee_unit *st = unit_get_physics(u);
        int done = st && unit_get_finished(st);
        ivec2 p = unit_get_tile(u);
        float rng, att;
        if (t == UT(B_FARM)) { if (nfld < 512) fld[nfld++] = p; open_fields += !done; }
        else if (t == UT(B_GRANARY) || t == UT(B_GRANARY_IMPROVED)) { if (done && ngran < BD_GRANARIES + 8) gran[ngran++] = p; open_gran += !done; gran_room += done && unit_type_get_garrison_capacity(td) > unit_get_garrisoned(u) && wk_cost(s, ter_near(p.x, p.y)); }
        else if (t == UT(B_SETTLEMENT)) open_settle += !done;
        else if (pr_kind(t) >= 0) open_prod += !done;
        open_sink += !done && (pr_kind(t) >= 0 || t == rb[1] || t == rb[2] || t == UT(B_AIRPORT));
        for (int k = 0; k < 3; k++) if (t == rb[k]) have_rb[k]++;   // going up counts
        if (t == rb[0] && nuni < 256) { int sz = unit_type_get_size(td); uni[nuni++] = iv2(p.x + sz / 2, p.y + sz / 2); }
        if (t == UT(B_HOUSE)) { if (nhouse < 512) house[nhouse++] = p; open_house += !done; }
        if (done && (t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL)) && ntown < 128) { town[ntown].p = p; town[ntown++].max = t == UT(B_CAPITOL) ? BD_HOUSES_CAPITOL : BD_HOUSES_TC; }
        nairport += t == UT(B_AIRPORT);   // going up too (29 laid in an hour by the asker's count, which sees the finished ones)
        if (is_tower_type(t)) open_tower += !done;
        if ((is_tower_type(t) || defence_stats(u, &rng, &att)) && ndef < 512) def[ndef++] = p;
        if ((t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL) || t == UT(B_SETTLEMENT)) && nds < BD_DSITES) dsite[nds++] = p;
        if (done && (t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL) || t == UT(B_SETTLEMENT)) && ncen < 256) {
            centre[ncen].p = p; centre[ncen].town = t != UT(B_SETTLEMENT); centre[ncen++].cost = wk_cost(s, ter_near(p.x, p.y));
        }
        if (!done && st && no < 96) openb[no++] = u;
    }
    b->open = no;
    int sends = 0, pile = no - open_fields;   // the unfinished that are no fields (a row of new fields is no pile)
    for (int i = 0; i < no && sends < (pile >= BD_PILE ? 2 * BD_SENDS : BD_SENDS) && nf; i++) {   // BUILDERS
        ee_unit *site = field_site(openb[i]);
        // MORE BUILDERS WHEN THEY PILE UP: one citizen a building, as before, while few stand unfinished; 2 a building
        // from BD_PILE of them, 3 from twice that (a field always one) - the player's P4 had 13 to 23 buildings
        // unfinished all through its game with 369 citizens. (3 a building always: twice the builders in the first
        // half hour, 6 soldiers fewer at minute 28 for one of four.)
        int has = 0, bk = -1, bd = 0x7FFFFFFF;
        for (int k = 0; k < nbuilt; k++) has += built[k] == site || built[k] == openb[i];
        ivec2 p = unit_get_tile(openb[i]);
        has = has >= (unit_type_get_id(unit_get_type_data(openb[i])) == UT(B_FARM) ? 1 : pile >= 2 * BD_PILE ? 3 : pile >= BD_PILE ? 2 : 1);
        if (has || raiders_known(pl, p, 8) || tv_unsafe(idx, p.x, p.y) || wk_danger(idx, p)) continue;   // remembered raiders too: the site may lie in the fog
        unsigned land = area_id(p, 2);
        for (int k = 0; k < nf; k++) {
            ivec2 q = unit_get_tile(freec[k]);
            int d = ivec2_dist2(p, q);
            if (d >= bd || (land && area_id(q, 2) && area_id(q, 2) != land)) continue;
            bd = d; bk = k;
        }
        // nor along a way with enemy soldiers on it (P2's builders walked through P4's army to a foundation behind
        // it, 36 dead: attack.c citizens_safety)
        if (bk < 0 || path_raided(pl, unit_get_x(freec[bk]), unit_get_y(freec[bk]), p.x, p.y) || !bd_send(pl, freec[bk], openb[i])) continue;
        freec[bk] = freec[--nf];
        sends++; b->sent++;
    }
    // WALLS: a piece is given its builders by the order that lays its run (the wall manager's build-wall, as a player
    // drags one); a builder who died or was called back left it a foundation for good - the game's round for unfinished
    // foundations is off with its computer player, and the pieces counted as wall (the player 2026-11-15: "red and
    // yellow have holes in their wall with foundations laid, but they never get build"). One builder a stretch: none
    // where a builder works on a piece within BD_WALL_RUN tiles (he goes on along the run), BD_WALL_SENDS a look, the
    // buildings' care (raiders, a territory being lost, enemy towers, a raided way).
    for (int i = 0, ws = 0; i < nw && ws < BD_WALL_SENDS && nf; i++) {
        ivec2 p = unit_get_tile(openw[i]);
        int near_b = 0, bk = -1, bd = 0x7FFFFFFF;
        for (int k = 0; k < nbuilt && !near_b; k++) {
            ee_unit_type *kd = built[k] ? unit_get_type_data(built[k]) : NULL;
            ivec2 q = built[k] ? unit_get_tile(built[k]) : iv2(-1000, -1000);
            near_b = kd && is_wall(unit_type_get_id(kd)) && abs(q.x - p.x) <= BD_WALL_RUN && abs(q.y - p.y) <= BD_WALL_RUN;
        }
        if (near_b || raiders_known(pl, p, 8) || tv_unsafe(idx, p.x, p.y) || wk_danger(idx, p)) continue;
        unsigned land = area_id(p, 2);
        for (int k = 0; k < nf; k++) {
            ivec2 q = unit_get_tile(freec[k]);
            int d = ivec2_dist2(p, q);
            if (d >= bd || (land && area_id(q, 2) && area_id(q, 2) != land)) continue;
            bd = d; bk = k;
        }
        if (bk < 0 || path_raided(pl, unit_get_x(freec[bk]), unit_get_y(freec[bk]), p.x, p.y) || !bd_send(pl, freec[bk], openw[i])) continue;
        freec[bk] = freec[--nf];
        if (nbuilt < 192) built[nbuilt++] = openw[i];   // the stretch has its builder now
        ws++; b->sent++; b->walls++;
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
    // AN AIRPORT SAVED FOR (below): asked for and not paid - nothing but fields, research and production buildings is laid
    // meanwhile, BD_AP_SAVE at a stretch and then as long not (from the World War an AI's wood stood between 9 and 369 for
    // 19 minutes, a granary or a Settlement taking each 125 as it came: the wing wanted from minute 18, its first
    // Airport at minute 37)
    int ap_ask = g_bd_airport[idx] && now >= g_bd_airport[idx] && now - g_bd_airport[idx] < 120000 && nairport < g_bd_airport_n[idx] && ut_has(UT(B_AIRPORT)) && prio_buildable(pl, UT(B_AIRPORT));
    int ap_save = 0;
    if (ap_ask && !bd_pays(pl, UT(B_AIRPORT), com) && workforce(pl) >= g_min_workforce) {
        if (!b->ap_save_t || now < b->ap_save_t || now - b->ap_save_t >= 2 * BD_AP_SAVE) b->ap_save_t = now ? now : 1;
        ap_save = now - b->ap_save_t < BD_AP_SAVE;
    }
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
    if (!placed && !ap_save && current_epoch(pl) >= FARM_EPOCH && ut_has(UT(B_GRANARY)) && can_build(pl, UT(B_GRANARY)) && bd_pays(pl, UT(B_GRANARY), com) && !open_gran && !open_fields && !fields_due &&
        ngran < BD_GRANARIES && (!ngran || g_wk[idx].food_short > 0) && !wait[0] && bd_gran_first(b, idx, ngran >= BD_GRAN_PLAIN ? gran_room : 0, now)) {   // A GRANARY: by its centre in the best territory
        for (int pass = 1; pass <= 3 && !type; pass++)
            for (int c = 0; c < ncen && !type; c++) {
                if (centre[c].cost != pass) continue;
                for (int r = 4; r <= 12 && !type; r++)
                    for (int ax = -r; ax <= r && !type; ax++)
                        for (int ay = -r; ay <= r && !type; ay++) {
                            ivec2 q = iv2(centre[c].p.x + ax, centre[c].p.y + ay);
                            int room = 0;
                            if ((abs(ax) != r && abs(ay) != r) || !can_place_open(pl, UT(B_GRANARY), q) || raiders_near(pl, q, 12) || tv_unsafe(idx, q.x, q.y) || wk_danger(idx, q)) continue;
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
    // AN AIRPORT (airstrike.c af_train asks: the wing it wants has too few): by a Town Center or Capitol in a territory it
    // holds, backland first
    if (!type && !placed && ap_ask && !wait[4] && !open_prod && bd_pays(pl, UT(B_AIRPORT), com)) {
        for (int pass = 1; pass <= 2 && !type; pass++)
            for (int c = 0; c < ncen && !type; c++)
                if (centre[c].town && centre[c].cost == pass && bd_spot(pl, idx, UT(B_AIRPORT), centre[c].p, 5, 16, &at)) { type = UT(B_AIRPORT); what = "an Airport for the bombers' wing"; g_bd_airport[idx] = 0; }
        if (!type) b->no_spot++;
    }
    if (ap_save && !type) placed = 1;   // AN AIRPORT SAVED FOR: nothing after it
    if (ap_save && !type && !g_rs_want[idx]) g_rs_want[idx] = UT(B_AIRPORT) + 1;   // ... and its price held from every other spender (priority.c's row "research": the docks' towers, the training)
    b->mines = b->mines_drop = b->mines_guard = 0;
    {   // A MINE'S SETTLEMENT, A MINE'S TOWER: the cheapest territory's first, then the nearest to the base
        ivec2 base = build_mgr_get_base(bm), sp = iv2(0, 0), tp = iv2(0, 0);
        int sbest = 0x7FFFFFFF, tbest = 0x7FFFFFFF, ttype = tower_newest(pl), smetal = 0;
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
            // gold and iron: a Settlement right beside the mine, a tile's gap at most (the player 2026-11-15: "a settlemnt a
            // fe wtiles close isnt good enough for iron and gold, they deserve a settlent directly beside them with
            // maximal one tile gap") - one a few tiles off no longer counts as theirs; stone as before
            int dr = r >= 5 ? BD_DROP_METAL : BD_DROP;
            for (int j = 0; j < nds && !has_drop; j++) has_drop = ivec2_dist2(dsite[j], p) <= dr * dr + (r >= 5 ? 1 : 0);
            for (int j = 0; j < ndef && !guard; j++) guard = abs(def[j].x - p.x) <= BD_GUARD && abs(def[j].y - p.y) <= BD_GUARD;
            b->mines_drop += has_drop; b->mines_guard += guard;
            if (raiders_near(pl, p, 12) || tv_unsafe(idx, p.x, p.y) || wk_danger(idx, p)) continue;   // not a mine under an enemy tower it has seen
            if (!has_drop) { if (score < sbest) { sbest = score; sp = p; smetal = r >= 5; } }
            else if (!guard && score < tbest) { tbest = score; tp = p; }
        }
        if (!type && !placed && sbest < 0x7FFFFFFF && !open_settle && !wait[1] && ut_has(UT(B_SETTLEMENT)) && can_build(pl, UT(B_SETTLEMENT)) && bd_pays(pl, UT(B_SETTLEMENT), com)) {
            if (bd_spot(pl, idx, UT(B_SETTLEMENT), sp, 2, smetal ? BD_DROP_METAL : 6, &at)) { type = UT(B_SETTLEMENT); what = "a Settlement by a mine"; } else b->no_spot++;
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
        // ... and the ground a push has cleared and holds (stations.c THE CLEARED GROUND) first and at once: its
        // soldiers stand there for the tower - no wait for the last claim, a lone tower's Settlement or the tower
        // going up elsewhere
        int held_r = -1;
        ivec2 held_at = iv2(0, 0);
        for (int r = 0; r < s->n && held_r < 0; r++)
            if (!g_ter.t[r].water && !s->t[r].bld && sn_holding(idx, r, &held_at) && !wk_danger(idx, held_at)) held_r = r;   // by its soldiers; whatever the view still remembers of the enemy's there (3 of 4 holds got no tower: the push sees no building left, the view's count of them lasts)
        // THE CLAIM'S TURN (the player 2026-11-16, his game at minute 141: "the ai never expands into territoy that have
        // no resoucres, this causes that it never can close it border, everething is front" - 3 claims each in 141
        // minutes): with nothing else to lay it hardly ever came (test games, Continental large, 60 min: the mines'
        // Settlements and towers, granaries, the morale's Houses first in most looks, 0-2 claims) - a claim that is
        // due takes the look, what else would have been laid waits for the next (BD_EVERY). No tower it can build or
        // pay (stone): a Barracks claims it instead - it holds (territories.c TV_HOLDS: it trains; a Fortress doesn't)
        // and costs wood. A claim's Settlement still missing holds the next claim back BD_CLAIM_LONE at most (one
        // that found no spot held them all for the rest of the game).
        int ctype = !wait[2] && ttype && can_build(pl, ttype) && bd_pays(pl, ttype, com) ? ttype :
                    !wait[4] && ut_has(UT(B_BARRACKS)) && can_build(pl, UT(B_BARRACKS)) && bd_pays(pl, UT(B_BARRACKS), com) ? UT(B_BARRACKS) : 0;
        if (lone >= 0 && b->claim_t && now >= b->claim_t && now - b->claim_t >= BD_CLAIM_LONE) lone = -1;
        // CLAIMS GOING UP: not every tower going up anywhere (a mine's, a watch tower - the rich test games: 50 of 70
        // looks held back so), only its own claims laid within BD_CLAIM_LONE whose territory it doesn't hold yet,
        // BD_CLAIM_OPEN of them at most
        int claims_going = 0;
        for (unsigned k = 0; k < 8 && k < b->nclaim_open; k++) {
            int r = b->claim_open[k].r;
            claims_going += r >= 0 && r < s->n && s->t[r].role == TVR_NONE && now >= b->claim_open[k].t && now - b->claim_open[k].t < BD_CLAIM_LONE;
        }
        b->claim_why = placed ? "something else laid" : !ctype ? (wait[2] ? "a tower just laid" : "no tower or Barracks to build or pay") :
                       workforce(pl) < g_min_workforce ? "too few citizens" : held_r >= 0 ? NULL :
                       lone >= 0 ? "a claim's Settlement still missing" : claims_going >= BD_CLAIM_OPEN ? "claims still going up" :
                       b->claim_t && now >= b->claim_t && now - b->claim_t < BD_CLAIM_EVERY ? "the last one too recent" : NULL;
        memset(b->claim_n, 0, sizeof b->claim_n);
        if (!b->claim_why) {
            // a territory with a mine nobody works counts as BD_CLAIM_MINE times nearer: the claims grow towards the mines
            // (the player, the same game: "it can also not reach some resource nodes on it path trough the map")
            static uint8_t mine_r[PR_T];
            memset(mine_r, 0, sizeof mine_r);
            for (int k = 0; k < g_ter.nsite; k++) {
                unsigned r = ter_near(g_ter.site[k].x, g_ter.site[k].y);
                if (g_ter.site[k].res >= 4 && g_ter.site[k].res <= 6 && r < PR_T) mine_r[r] = 1;
            }
            int cb = held_r, cd = 0;
            for (int r = 0; r < s->n; r++) {
                struct tv_terr *t = &s->t[r];
                // its own buildings that don't hold (a mine's Settlement, Houses, fields - territories.c TV_HOLDS) no longer
                // keep a claim off: of 27 territories in the view 2 held, 6 empty, most others with a building of its
                if (held_r >= 0 || g_ter.t[r].water || t->role != TVR_NONE || t->hold || t->ebld || t->abld) continue;
                b->claim_n[0]++;
                if (t->state != TV_QUIET) continue;
                b->claim_n[1]++;
                int guarded = 0;
                for (int k = 0; k < g_ter.t[r].nbn && !guarded; k++) {
                    unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
                    // enemy soldiers its guard beats there don't count against it (TV_FOES), only one it is losing (TV_UNSAFE):
                    // a scout passing kept the claims off for good on a busy map
                    guarded = o < (unsigned)s->n && !g_ter.t[o].water && s->t[o].role != TVR_NONE && s->t[o].state != TV_UNSAFE && s->t[o].sold >= BD_CLAIM_GUARD;
                }
                if (!guarded) continue;
                b->claim_n[2]++;
                ivec2 c = g_ter.t[r].c;
                if (raiders_near(pl, c, 12) || wk_danger(idx, c)) continue;
                b->claim_n[3]++;
                int d = ivec2_dist2(c, base) / (mine_r[r] ? BD_CLAIM_MINE * BD_CLAIM_MINE : 1);
                if (cb < 0 || d < cd) { cb = r; cd = d; }
            }
            ivec2 cat;
            if (cb < 0) b->claim_why = "no territory to claim";
            else if (bd_spot(pl, idx, ctype, cb == held_r ? held_at : g_ter.t[cb].c, 0, ctype == ttype ? 8 : 12, &cat)) {
                at = cat; type = ctype; b->claim_t = now ? now : 1; b->claims++; b->claim_why = "claimed";
                b->claim_open[b->nclaim_open++ & 7].r = cb; b->claim_open[(b->nclaim_open - 1) & 7].t = now ? now : 1;
                what = ctype == ttype ? "a claim: a tower in a territory nobody holds" : "a claim: a Barracks in a territory nobody holds (no tower to build or pay)";
            }
            else { b->no_spot++; b->claim_why = "no spot for the claim"; }
        }
        // A WATCH TOWER (the player 2026-11-16: "build towers in the long term so it knows whats going on in its own
        // borders"): with nothing else to lay, a tower in the middle of the largest place of a territory it holds that
        // it has had in sight less than a quarter of the time (watch.c DIM) with no defence of its within BD_WATCH_NEAR
        // tiles (one just laid counts: the place stays dim until the tower stands) - one every BD_WATCH_EVERY
        if (!type && !placed && !open_tower && !wait[2] && ttype && can_build(pl, ttype) && bd_pays(pl, ttype, com) && workforce(pl) >= g_min_workforce &&
            (!b->watch_t || now < b->watch_t || now - b->watch_t >= BD_WATCH_EVERY)) {
            ivec2 w;
            for (int k = 0; !type && watch_tower_spot(idx, k, &w); k++) {
                int near_def = 0;
                for (int j = 0; j < ndef && !near_def; j++) near_def = ivec2_dist2(def[j], w) <= BD_WATCH_NEAR * BD_WATCH_NEAR;
                if (near_def || raiders_near(pl, w, 12) || wk_danger(idx, w)) continue;
                if (bd_spot(pl, idx, ttype, w, 0, 4, &at)) { type = ttype; what = "a watch tower over ground of its it rarely sees"; b->watch_t = now ? now : 1; watch_count(idx, 3); }
            }
        }
    }
    // A UNIVERSITY A TERRITORY (the player 2026-11-16: "it should add universities ... it prevents conversions, so atleast
    // one per territory as save space isnt a bad idea"): its Universities keep the enemy's priests from converting
    // within their reach (bd_reach: 8 tiles, 18 with every range upgrade). By each finished Town Center, Capitol or
    // Settlement no University of its reaches, where its citizens may work, no raiders or enemy towers there: one, a
    // front territory's first - one at a time, one every BD_UNI_EVERY, with nothing more urgent to lay.
    if (!type && !placed && have_rb[0] && ut_has(UT(B_UNIVERSITY)) && can_build(pl, UT(B_UNIVERSITY)) && bd_pays(pl, UT(B_UNIVERSITY), com) &&
        workforce(pl) >= g_min_workforce && (!b->uni_t || now < b->uni_t || now - b->uni_t >= BD_UNI_EVERY)) {
        int ur = bd_reach(pl, UT(B_UNIVERSITY)), bc = -1, bs = 0;
        if (b->uni_t && now >= b->uni_t && now - b->uni_t < BD_UNI_PEND && nuni < 256) uni[nuni++] = b->uni_at;   // the last one laid may not show yet
        for (int j = 0; j < ncen; j++) {
            int covered = 0;
            for (int k = 0; k < nuni && !covered; k++) covered = ivec2_dist2(uni[k], centre[j].p) < ur * ur;
            unsigned r = ter_near(centre[j].p.x, centre[j].p.y);
            if (covered || r >= (unsigned)s->n || !wk_workable(idx, centre[j].p) || raiders_near(pl, centre[j].p, 12) || wk_danger(idx, centre[j].p)) continue;
            int sc = (s->t[r].role == TVR_FRONT ? 100 : 0) + (centre[j].town ? 10 : 0);
            if (bc < 0 || sc > bs) { bc = j; bs = sc; }
        }
        if (bc >= 0) {
            b->uni_t = now ? now : 1;
            if (bd_spot(pl, idx, UT(B_UNIVERSITY), centre[bc].p, 3, 8, &at)) { b->uni_at = at; b->unis++; type = UT(B_UNIVERSITY); what = "a University for the territory (no conversions within its reach)"; } else b->no_spot++;
        }
    }
    // THE MORALE'S HOUSES (the player 2026-11-15: "the ai dosnt build houses yet in the influence are of its towncenters
    // and capitols (the morale bonus)"): the game's data (dbareaeffect.dat "TC Morale Bonus Rng 10", "Capitol Morale
    // Bonus Rng 10": a square 10 tiles each way, the most 2 and 4) and its count (the owner's finished Houses whose
    // footprint meets the square) - each House there is a level of morale for its units in that square, up to 2
    // round a Town Center and 4 round a Capitol. With nothing else to lay: a House by the Town Center or Capitol that
    // has the fewest of its number (going up counts), a front territory's first, where its citizens may work - one
    // at a time, one every BD_HOUSE_EVERY.
    if (!type && !placed && !open_house && ntown && ut_has(UT(B_HOUSE)) && can_build(pl, UT(B_HOUSE)) && bd_pays(pl, UT(B_HOUSE), com) && workforce(pl) >= g_min_workforce &&
        (!b->house_t || now < b->house_t || now - b->house_t >= BD_HOUSE_EVERY)) {
        int bt = -1, bs = 0;
        int pend = b->house_t && now >= b->house_t && now - b->house_t < BD_HOUSE_PEND;   // the last one laid may not show yet (the builder walks there): it counts, and the next keeps off its spot
        if (pend && nhouse < 512) house[nhouse++] = b->house_at;
        for (int j = 0; j < ntown; j++) {
            int have_h = 0;
            for (int k = 0; k < nhouse; k++) have_h += abs(house[k].x - town[j].p.x) <= BD_HOUSE_REACH && abs(house[k].y - town[j].p.y) <= BD_HOUSE_REACH;
            unsigned r = ter_near(town[j].p.x, town[j].p.y);
            if (have_h >= town[j].max || r >= (unsigned)s->n || !wk_workable(idx, town[j].p) || raiders_near(pl, town[j].p, 12) || wk_danger(idx, town[j].p)) continue;
            int sc = (s->t[r].role == TVR_FRONT ? 100 : 0) + (town[j].max - have_h);
            if (bt < 0 || sc > bs) { bt = j; bs = sc; }
        }
        if (bt >= 0) {
            b->house_t = now ? now : 1;
            if (bd_spot(pl, idx, UT(B_HOUSE), town[bt].p, 3, BD_HOUSE_REACH - 1, &at) && !(pend && abs(at.x - b->house_at.x) <= 3 && abs(at.y - b->house_at.y) <= 3)) { b->house_at = at; type = UT(B_HOUSE); what = "a House for the morale of its Town Center or Capitol"; } else b->no_spot++;
        }
    }
    // THE WOOD SINK (the player 2026-11-15: "the ai sould also be building more in general, right now it sits on 130k
    // wood. these can be spend in front territoies on hospitals, temples and production buildings (even when nobody
    // asked) above 10k wood it should try to get rid of it"): BD_SINK wood in the bank and nothing else to lay - a
    // Hospital, a Temple or a production building in a quiet front territory of its, the kind that territory has the
    // fewest of (BD_SINK_MAX of a kind in one at most), one every BD_SINK_EVERY while fewer than BD_SINK_OPEN of its
    // buildings stand unfinished.
    if (!type && !placed && player_stock_at(pl)[3] >= BD_SINK && open_sink < BD_SINK_OPEN && (!b->sink_t || now < b->sink_t || now - b->sink_t >= BD_SINK_EVERY)) {
        int kinds[3 + PR_NK], nk = 0;
        kinds[nk++] = rb[1]; kinds[nk++] = rb[2]; kinds[nk++] = UT(B_AIRPORT);   // an Airport near the front: the planes' reach
        for (int k = 0; k < PR_NK; k++) kinds[nk++] = g_ut[PR_KINDS[k]];
        static uint8_t have[BD_SINK_T][3 + PR_NK];
        static ivec2 aoe_at[2][256], place[BD_SINK_T];   // AREA OF EFFECT: its Hospitals' and Temples' middles; each territory's place
        int tn = s->n < BD_SINK_T ? s->n : BD_SINK_T, cnt = 0, naoe[2] = { 0, 0 }, reach[2] = { bd_reach(pl, kinds[0]), bd_reach(pl, kinds[1]) };
        memset(have, 0, (size_t)tn * sizeof *have);
        for (ee_unit *u = player_get_units(pl); u && cnt < 16384; u = unit_get_next(u), cnt++) {
            ee_unit_type *td = unit_get_type_data(u);
            if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
            unsigned r = ter_near(unit_get_x(u), unit_get_y(u));
            for (int k = 0; r < (unsigned)tn && k < nk; k++) if (kinds[k] >= 0 && unit_type_get_id(td) == kinds[k] && have[r][k] < 255) have[r][k]++;
            for (int k = 0; k < 2; k++)
                if (kinds[k] >= 0 && unit_type_get_id(td) == kinds[k] && naoe[k] < 256) {
                    int sz = unit_type_get_size(td);
                    aoe_at[k][naoe[k]++] = iv2(unit_get_x(u) + sz / 2, unit_get_y(u) + sz / 2);
                }
        }
        for (int r = 0; r < tn; r++) place[r] = g_ter.t[r].c;
        for (int j = 0; j < ncen; j++) { unsigned r = ter_near(centre[j].p.x, centre[j].p.y); if (r < (unsigned)tn) place[r] = centre[j].p; }
        // no quiet front territory with room: a production building in a quiet backland (the fronts of the player's
        // P4 were all under attack, 100,000 wood in the bank)
        int br = -1, bk = -1, bn = 0;
        for (int back = 0; back < 2 && br < 0; back++)
        for (int r = 0; r < tn; r++) {
            if (g_ter.t[r].water || s->t[r].role != (back ? TVR_BACK : TVR_FRONT) || s->t[r].state != TV_QUIET) continue;
            for (int k = back ? 3 : 0; k < nk; k++) {
                if (kinds[k] < 0 || have[r][k] >= (k == 2 ? 1 : BD_SINK_MAX) ||   // one Airport a territory (18 of one player's in the player's game)
                     !ut_has(kinds[k]) || !prio_buildable(pl, kinds[k]) || !bd_pays(pl, kinds[k], com)) continue;
                int covered = 0;   // AREA OF EFFECT: a Hospital or Temple of its already reaches the territory's place
                for (int j = 0; k < 2 && j < naoe[k] && !covered; j++) covered = ivec2_dist2(aoe_at[k][j], place[r]) < reach[k] * reach[k];
                if (covered) continue;
                if (br < 0 || have[r][k] < bn) { br = r; bk = k; bn = have[r][k]; }
            }
        }
        if (br >= 0) {
            ivec2 c = place[br];
            int near_same = 0;
            if (!raiders_near(pl, c, 12) && !wk_danger(idx, c) && bd_spot(pl, idx, kinds[bk], c, 3, 14, &at)) {
                ee_unit_type *ktd = type_data(kinds[bk]);
                int sz = ktd ? unit_type_get_size(ktd) : 0;
                ivec2 mid = iv2(at.x + sz / 2, at.y + sz / 2);
                for (int j = 0; bk < 2 && j < naoe[bk] && !near_same; j++) near_same = ivec2_dist2(aoe_at[bk][j], mid) < reach[bk] * reach[bk];   // AREA OF EFFECT: the spot itself too
                if (!near_same) { type = kinds[bk]; what = "the wood sink: one more in a front territory"; b->sink_t = now ? now : 1; b->sinks++; }
            }
            if (!type) { b->no_spot++; if (near_same) b->sink_t = now ? now : 1; }   // too near its own: the next look in BD_SINK_EVERY
        }
    }
    if (type) {
        census_build_ask(bm, type);   // the test log's count: the mod's, not the game's entry
        ok = hook_place(bm, type, at.x, at.y);
        if (!ok) b->refused++;
        else {
            int k = type == UT(B_GRANARY) ? 0 : type == UT(B_SETTLEMENT) ? 1 : type == rb[0] || type == rb[1] || type == rb[2] ? 3 : pr_kind(type) >= 0 ? 4 : 2;
            if (type == UT(B_HOUSE)) b->houses++;
            else if (k == 0) b->granaries++; else if (k == 1) b->settlements++; else if (k == 3) b->research++; else if (k == 4) { b->production++; g_pr[idx].type = 0; if (type == g_pr_more[idx]) g_pr_more[idx] = 0; } else b->towers++;
            if (type != UT(B_HOUSE)) { b->laid_t[k] = now ? now : 1; b->laid_type[k] = type; }
            for (int r = 2; r < 7; r++) g_bd_com[idx][r] += unit_cost(pl, type, r);
            if (type == UT(B_AIRPORT)) ap_laid(idx, at, now);   // defence.c: its strip is taken until it shows
        }
        if (g_log && (ok || !(b->refused % 20)))
            log_locked("# build: P%u at %.1f min: %s - %d at %d,%d (territory %u): %s\n", idx, now / 60000.0, what, type, at.x, at.y, ter_near(at.x, at.y),
                       ok ? "laid" : "not laid (the price not in the bank, or held back for what the priority list saves for)");
    }
    if (g_log && (!b->log_t || now - b->log_t >= 60000)) {
        b->log_t = now ? now : 1;
        log_locked("# build: P%u at %.1f min: %d granaries, %d fields (%d going up, %d places still without one), %d unfinished buildings | mines in reach %d, with a drop site %d, with a defence %d | "
                   "so far %u builders sent, %u fields, %u granaries, %u Settlements, %u towers, %u research and %u production buildings laid, %u not laid, %u without a spot | wall pieces going up %d, builders sent to them %u | "
                   "claims %u, the last look: %s (territories nobody holds %d, quiet %d, beside a guarded one %d, safe %d)\n",
                   idx, now / 60000.0, ngran, nfld, open_fields, fields_due, no, b->mines, b->mines_drop, b->mines_guard, b->sent, b->fields, b->granaries, b->settlements, b->towers, b->research, b->production, b->refused, b->no_spot, nw, b->walls,
                   b->claims, b->claim_why ? b->claim_why : "-", b->claim_n[0], b->claim_n[1], b->claim_n[2], b->claim_n[3]);
    }
}
internal void build_new_game(void) { memset(g_bd, 0, sizeof g_bd); memset(g_bd_com, 0, sizeof g_bd_com); produce_new_game(); }
