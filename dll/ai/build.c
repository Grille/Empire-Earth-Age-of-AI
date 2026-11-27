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
#define BD_HOUSE_EVERY 60000
#define BD_HERD_EVERY 120000   // A HERD'S SETTLEMENT: between two
#define BD_FISH_EVERY 120000   // A FISHING DOCK: between two
#define BD_FISH_DOCKS 4        // ... while it has fewer docks (going up too)
#define BD_FISH_NEAR 30        // ... the fish this near a drop site of its
#define BD_FISH_R 16           // ... and no dock of its this near the fish
#define BD_HOUSE_PEND 180000
#define BD_PAIR_R 5             // THE PAIR: its first Hospital and Temple this far apart at most (each way, corner to corner)
#define BD_SINK 10000           // wood in the bank from which it builds unasked
#define BD_SINK_EVERY 20000
#define BD_STONE_SINK 5000      // stone in the bank from which THE STONE SINK lays the wood sink's kinds that cost stone (territories.c BW_SINK)
#define BD_SINK_MAX 3           // of a kind in one territory
#define BD_SINK_OPEN 4          // unfinished buildings
#define BD_SINK_T 4096
#define BD_AOE 3                // the wood sink's kinds with an area of effect: Hospital, Temple, University (its first three)
#define BD_CLAIM_GUARD 3        // its soldiers in the held territory a claim starts from
#define BD_CLAIM_MINE 2         // a territory with a mine counts as this many times nearer to the base
#define BD_CLAIM_OPEN 2         // its claims not holding yet that hold the next back
#define BD_CLAIM_LONE 360000  // a claim's Settlement still missing holds the next claim back this long at most
#define BD_WATCH_EVERY 120000   // A WATCH TOWER: between two
#define BD_WATCH_NEAR 8         // ... none within this many tiles of a defence of its
#define BD_HOLD_R 12      // HOLDING THE GROUND: enemy soldiers or buildings this near an idle citizen of its
#define BD_HOLD_TOWER 8   // ... a tower of its this near the idle ones' middle: none laid
#define BD_HOLD_PROD 16   // ... a production building of its this near: none laid
#define BD_HOLD_EVERY 20000
#define BD_HOLD_N 8       // ... builders sent to it at most
#define BD_HOLD_KEEP 180000   // ... its builders kept at it this long since they were sent (attack.c citizens_safety: bd_hold_site)
#define BD_HOLD_SITES 16
internal struct { ee_unit *b; int id; uint32_t t; } g_bd_holds[16][BD_HOLD_SITES];
internal void bd_hold_mark(unsigned idx, ee_unit *b, uint32_t now) {
    int k = 0;
    for (int j = 0; j < BD_HOLD_SITES; j++) {
        if (g_bd_holds[idx & 15][j].b == b && g_bd_holds[idx & 15][j].id == unit_get_serial(b)) { k = j; break; }
        if (g_bd_holds[idx & 15][j].t < g_bd_holds[idx & 15][k].t) k = j;
    }
    g_bd_holds[idx & 15][k].b = b; g_bd_holds[idx & 15][k].id = unit_get_serial(b); g_bd_holds[idx & 15][k].t = now ? now : 1;
}
// attack.c citizens_safety: a building HOLDING THE GROUND sent its builders to lately - they stay at it, raiders near or
// not (sent back from it each look, they went to and fro between it and the drop site, the player 2026-11-19: "the
// selected citecens (and it grou) get send forth and back betwehen construction sites, never getting done either")
internal int bd_hold_site(ee_player *pl, ee_unit *b) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    uint32_t now = game_time();
    for (int j = 0; b && j < BD_HOLD_SITES; j++)
        if (g_bd_holds[idx][j].b == b && g_bd_holds[idx][j].id == unit_get_serial(b) && g_bd_holds[idx][j].t && now >= g_bd_holds[idx][j].t && now - g_bd_holds[idx][j].t < BD_HOLD_KEEP) return 1;
    return 0;
}
enum { BD_PICK_AIRPORT = 1, BD_PICK_HERD, BD_PICK_FOREST, BD_PICK_AIRPORT_FWD };   // build_tick's pick
#define BD_WAIT 60000   // after one is laid no other of its kind until its foundation shows (the builder walks there first: one Settlement was laid three times in 25 s)
// ACROSS THE WATER (work.c): the lands where a foundation of its found no free citizen on its land to build it - the
// ferry (ferry.c) brings citizens there as to free places at mines, with no drop site of its there yet
#define BD_WAIT_LANDS 8
#define BD_FOREST_IDLE 8        // TO THE TREES ACROSS: idle citizens for a Settlement by trees across the water
#define BD_FOREST_EVERY 60000
internal struct { uint32_t t; int n; unsigned land[BD_WAIT_LANDS]; ivec2 at[BD_WAIT_LANDS]; } g_bd_wait[16];
internal int bd_waits(unsigned idx, unsigned land, ivec2 *at) {
    uint32_t now = game_time();
    if (!g_bd_wait[idx & 15].t || now < g_bd_wait[idx & 15].t || now - g_bd_wait[idx & 15].t > 3 * BD_EVERY) return 0;
    for (int k = 0; k < g_bd_wait[idx & 15].n; k++) if (g_bd_wait[idx & 15].land[k] == land) { if (at) *at = g_bd_wait[idx & 15].at[k]; return 1; }
    return 0;
}
internal int bd_citizen_on(ee_player *pl, unsigned land) {   // a citizen of its stands on that land
    int count = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (td && unit_get_hp(u) > 0 && unit_get_x(u) >= 0 && is_citizen_type(unit_type_get_id(td)) && area_id(unit_get_tile(u), 2) == land) return 1;
    }
    return 0;
}
internal struct bd_st {
    uint32_t claim_t; unsigned claims;   // A CLAIM: the last one, how many
    const char *claim_why; int claim_n[4];   // ... the last look: what held it back; territories nobody holds, quiet, beside a guarded one of its, safe
    struct { int r; uint32_t t; } claim_open[8]; unsigned nclaim_open;   // ... the last claims: territory, when (CLAIMS GOING UP)
    uint32_t watch_t;                    // A WATCH TOWER: the last one
    uint32_t hold_t; unsigned holds, hold_sent;   // HOLDING THE GROUND: the last laid, how many, builders sent
    uint32_t ap_save_t;                  // AN AIRPORT SAVED FOR: since
    unsigned ap_fwds, ap_fwd_none;       // THE AIRPORTS MOVE UP: laid; looks with the ask and no centre of its nearer
    uint32_t gran_ask_t; int gran_room;  // A GRANARY BEFORE THE NEXT: asked since, the granaries with room then
    uint32_t sink_t; unsigned sinks;     // THE WOOD SINK: the same
    uint32_t house_t; unsigned houses; ivec2 house_at;   // THE MORALE'S HOUSES: the same, and where the last was laid
    uint32_t herd_t; unsigned herds;     // A HERD'S SETTLEMENT: the last laid, how many
    uint32_t forest_t; unsigned forests; int forest_n[3]; unsigned forest_nospot;   // TO THE TREES ACROSS: the last look, how many laid; the last look's territories on a land with no drop site of its, with a tree, out of danger; no spot found
    uint32_t fdock_t; unsigned fdocks;   // A FISHING DOCK: the same
    uint32_t t, log_t, laid_t[6];   // laid_t: the last granary, Settlement, tower, research building, production building, Airport laid (BD_WAIT)
    int laid_type[6];               // ... and its type
    unsigned sent, fields, granaries, settlements, towers, research, production, refused, no_spot, walls, across;   // walls: builders sent to unfinished wall pieces
    unsigned farm_wait, farm_nowood, farm_extra;   // FARMS PAID FIRST: builders not sent for want of the wood, let go for it, let go as a field's second
    int open, lost_ground, mines, mines_drop, mines_guard;   // the last look: unfinished buildings, those of them LEFT ON LOST GROUND; mines in reach, those with a drop site, with a defence
} g_bd[16];
// a herd's living grown animals (A HERD'S SETTLEMENT: more than HERDS KEPT leaves, one to hunt)
internal int bd_herd_grown(ee_herd *h) {
    ee_unit **b = herd_get_members_begin(h), **e = herd_get_members_end(h);
    int n = 0;
    for (ee_unit **m = b; b && m < e && m - b < 256; m++) n += *m && unit_get_hp(*m) > 0 && unit_get_x(*m) >= 0 && unit_get_scale(*m) >= 1.0f;
    return n;
}
// a citizen sent to build (as econ.c's field pass does it: a BuildRepair goal, the builder's state, its script told)
internal int sn_holding(unsigned idx, int r, ivec2 *at);   // stations.c: a push of its holds the territory it cleared
internal int bd_send(ee_player *pl, ee_unit *cit, ee_unit *building) {
    if (pc_held(cit)) return 0;   // the player's (THE PLAYER'S ORDERS FIRST, messages.c)
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
    for (int id = 14; id < G_TECH_IDS; id++) {
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
// THE AIRPORTS MOVE UP ([ai_base] airport_forward, on by the player's call of 2026-11-27 - "yes, turn it on", on the
// measurement below; [autotest] v_airport_forward for the variant players; airstrike.c THE BOMBER'S REACH
// asks: the position of its force that the wing would strike lies beyond its bombers' reach from every Airport of
// its - the place where its force meets the enemy next and the reach given, the place its aircraft's bases move
// towards, airguard.c THE AIR BASES. The player: "a human player moves its airfields forward to reach deeper into
// enemy territory ... the ai's airplanes are all stuck at home since the ai never extends their reach"; AN AIRPORT
// below lays them by its towns, the backland first): while no Airport of its is within that reach of the place, one
// more by the Town Center, Capitol or Settlement of its that is nearest the place, in a territory it holds (the
// backland or a front) - when that is BD_AP_FWD_GAIN tiles nearer than its nearest Airport; BD_AP_FWD_MAX Airports in
// all at most, none while one goes up.
// Its aircraft move there by themselves (airguard.c THE AIR BASES).
// MEASURED 2026-11-27, head-to-head mirrored, Continental huge (320 tiles a side) from World War I, air_rich.eeai, 4
// AIs, 90 min, 12 games (all ended well), with it - without: 1.2 Airports moved up an AI (7 looks with no centre
// nearer); the bombers' runs 110.2 / 83.8 with their targets down 47.3 / 36.6, the wing's waves 15.0 / 15.8, bombers
// lost 35.2 / 33.2, the forward Airport's way to the front at its longest tenth 88 / 101 tiles - and the game level:
// soldiers -9.0 (se 9.1), kills -16.8 (22.6), losses +13.3 (14.7), razed -2.1 (10.3), epoch -0.12 (0.09), workers +2.4
// (3.3). Two tries before it: asked for the worth-most target beyond the reach anywhere (60 min, 8 games) 2.4 an AI,
// razed +6.6 (5.5), kills +16.3 (19.4), workers -5.8 (4.0) - Airports towards enemies its force was not at, where no
// aircraft moved; without the shooters' spots (90 min, 12 games) 0.2 an AI - the wing's position in a fight is mostly
// theirs - and soldiers +13.5 (8.7) all the same: what two like sides differ by on this map. More bombing, no gain
// in the game shown: the player's call - on.
internal struct { ivec2 at; float reach; uint32_t t; } g_bd_ap_fwd[16];
internal int g_bd_ap_fwd_on = 0, g_v_bd_ap_fwd = -1;
#define BD_AP_FWD_GAIN 20
#define BD_AP_FWD_MAX 8
internal int bd_pays(ee_player *pl, int type, const int *com) {
    const int *stock = player_stock_at(pl);
    for (int r = 2; r < 7; r++) { int c = unit_cost(pl, type, r); if (c > 0 && stock[r] - com[r] < c) return 0; }
    return 1;
}
// SCARCE GROUND ([ai_base] compact_build; the player 2026-11-23: "on larger maps it doesnt matter, there is always more
// space, but one some its a genuine resource the ai shouldnt waste"): on a land of BD_SMALL_LAND open tiles or fewer
// (territory.c's areas of its territories: an AoC planet some 600, an islet) bd_spot takes of its first BD_COMPACT_N
// spots from the nearest ring out the one whose ring of tiles round the footprint has the most that can't be walked -
// buildings, the land's edge, trees, cliffs, a mine - the nearest of equals: the open ground stays in one piece, where
// a granary's 6 x 6 of fields still fits. Nor onto a place round a granary of its that could still take a field (a
// Farm the mod lays there, the game's Robotic Farms in the Space Age). Before, the nearest spot: houses, Hospitals and
// towers dotted over a planet's middle.
#define BD_SMALL_LAND 2500
#define BD_COMPACT_N 16
internal struct { ivec2 g[BD_GRANARIES + 8]; int n; } g_bd_gr[16];   // build_tick: its finished granaries
internal int bd_small_land(ivec2 p) {
    unsigned land = area_id(p, 2);
    int open = 0;
    if (!land) return 0;
    for (int r = 0; r < g_ter.n && open <= BD_SMALL_LAND; r++) if (!g_ter.t[r].water && area_id(g_ter.t[r].c, 2) == land) open += g_ter.t[r].area;
    return open <= BD_SMALL_LAND;
}
internal int bd_hug(ee_map *map, int size, ivec2 q, int s) {   // the ring round the footprint q .. q + s - 1: its tiles not to be walked
    int n = 0;
    for (int ty = q.y - 1; ty <= q.y + s; ty++)
        for (int tx = q.x - 1; tx <= q.x + s; tx++)
            if ((tx < q.x || ty < q.y || tx >= q.x + s || ty >= q.y + s) && !rm_tile_ok(map, size, tx, ty)) n++;
    return n;
}
internal int bd_on_field_place(unsigned idx, ivec2 q, int s) {   // the footprint over a place round one of its granaries (a field there already: can_place says no)
    for (int k = 0; k < g_bd_gr[idx].n; k++)
        for (int fx = -2; fx <= 2; fx += 2)
            for (int fy = -2; fy <= 2; fy += 2) {
                int x0 = g_bd_gr[idx].g[k].x + fx, y0 = g_bd_gr[idx].g[k].y + fy;
                if ((fx || fy) && q.x < x0 + 2 && x0 < q.x + s && q.y < y0 + 2 && y0 < q.y + s) return 1;
            }
    return 0;
}
// a spot for a building of `type` r0..r1 tiles from p, nearest first: placeable, on land that is not being lost
internal int bd_spot(ee_player *pl, unsigned idx, int type, ivec2 p, int r0, int r1, ivec2 *out) {
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0, sz = type_data(type) ? unit_type_get_size(type_data(type)) : 1;
    int compact = g_set_ai[1].compact_build && map && !is_field_type(type) && bd_small_land(p), found = 0, best = -1;   // SCARCE GROUND
    if (sz < 1) sz = 1;
    for (int r = r0; r <= r1; r++)
        for (int ax = -r; ax <= r; ax++)
            for (int ay = -r; ay <= r; ay++) {
                ivec2 q = iv2(p.x + ax, p.y + ay);
                if ((abs(ax) != r && abs(ay) != r) || (compact && bd_on_field_place(idx, q, sz)) || !can_place_open(pl, type, q) || tv_unsafe(idx, q.x, q.y) || wk_danger(idx, q)) continue;
                if (!compact) { *out = q; return 1; }
                int h = bd_hug(map, size, q, sz);
                if (h > best) { best = h; *out = q; }
                if (++found >= BD_COMPACT_N) return 1;
            }
    return found > 0;
}
// THE FIELD LINE's type to lay: the newest the game offers this player now - in Art of Conquest's Space Age the Robotic
// Farm, which farms itself (the player 2026-11-24: Farms were laid where the game no longer let him order one -
// lay_foundation skips the build menu's test); 0: none
internal int bd_field_type(ee_player *pl) {
    for (int k = UT_FIELDS_N - 1; k >= 0; k--) { int t = field_type(k); if (ut_has(t) && prio_buildable(pl, t)) return t; }
    return 0;
}
internal void build_tick(ee_player *pl, ee_resource_mgr *rm, uint32_t now) {
    static ee_unit *freec[384], *built[192], *builder[192], *openb[96], *openw[256], *idlec[384];   // builder: built[]'s citizen   // idlec: citizens with no order (HOLDING THE GROUND - freec has gatherers only)
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
    int nf = 0, nbuilt = 0, no = 0, nw = 0, nfld = 0, ngran = 0, ndef = 0, ncen = 0, nidle = 0;
    int nairport = 0, open_airport = 0, nap = 0;
    static ivec2 app[32];
    static ivec2 dock[32];
    int ndock = 0;
    int gran_room = 0;   // its finished granaries with room inside (A GRANARY BEFORE THE NEXT)
    static ivec2 house[512], prodp[256];
    int nprod = 0;
    static struct { ivec2 p; int max; } town[128];
    int nhouse = 0, ntown = 0, open_house = 0;
    int open_fields = 0, open_gran = 0, open_settle = 0, open_tower = 0, open_prod = 0, open_sink = 0, count = 0, have_rb[3] = { 0, 0, 0 }, lost_ground = 0;
    ivec2 rb_at[3] = { { 0, 0 }, { 0, 0 }, { 0, 0 } };   // where one of each research building is (THE PAIR)
    const int rb[3] = { UT(B_UNIVERSITY), UT(B_HOSPITAL), UT(B_TEMPLE) };
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {   // what it has: citizens, foundations, granaries, fields, defences, centres
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
        int t = unit_type_get_id(td), fam = unit_type_get_class_id(td);
        if (is_citizen_type(t)) {
            ee_goal *g = unit_get_goal(u);
            if (g && !strcmp(rtti_name(g), "EEUGBuildRepair@@")) { if (nbuilt < 192) { builder[nbuilt] = u; built[nbuilt++] = goal_get_building(g); } }
            else if (nf < 384 && citizen_free(u, 0) && (!g || !strcmp(rtti_name(g), "EEUGGather@@")) && cl_may(u, CLP_ROUTINE, CLW_BUILD)) freec[nf++] = u;
            else if (!g && nidle < 384 && citizen_free(u, 1) && cl_may(u, CLP_HOLD, CLW_HOLD)) idlec[nidle++] = u;   // ORDERS BY PRIORITY (claims.c): none another holds
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
        // LEFT ON LOST GROUND (the player's game 2026-11-18: "p2 never expanded" - its Settlement by a herd and a mine's
        // tower laid across the water in a territory the other AI then settled stood at 1 hp from minute 10 and 31 to the
        // end, and one of each going up at a time held every later Settlement and tower back: none laid for 55 minutes,
        // across the water or at home, 236 citizens idle): a foundation where its citizens don't work (work.c wk_cost 0:
        // another's buildings there, a territory being lost or out of reach) no longer counts as going up. It stays laid,
        // built once the ground is workable again (the builders' care below keeps them off it meanwhile).
        int going = !done && wk_cost(s, ter_near(p.x, p.y));
        lost_ground += !done && !going;
        if (t == UT(B_DOCK) && ndock < 32) dock[ndock++] = p;   // A FISHING DOCK: its docks, going up too
        float rng, att;
        if (is_field_type(t)) { if (nfld < 512) fld[nfld++] = p; open_fields += !done; if (done) cg_track_field(idx, u, now); }   // THE FIELD LINE: the game's Robotic Farms too; CLEARING GROUND: its last farmer
        else if (t == UT(B_GRANARY) || t == UT(B_GRANARY_IMPROVED)) { if (done && ngran < BD_GRANARIES + 8) gran[ngran++] = p; open_gran += going; gran_room += done && unit_type_get_garrison_capacity(td) > unit_get_garrisoned(u) && wk_cost(s, ter_near(p.x, p.y)); }
        else if (t == UT(B_SETTLEMENT)) open_settle += going;
        else if (pr_kind(t) >= 0) { open_prod += going; if (nprod < 256) prodp[nprod++] = p; }
        open_sink += going && (pr_kind(t) >= 0 || t == rb[0] || t == rb[1] || t == rb[2] || t == UT(B_AIRPORT));
        if (done && pr_kind(t) >= 0) cg_track(idx, u, now);   // CLEARING GROUND (clearing.c): its last training
        for (int k = 0; k < 3; k++) if (t == rb[k]) { if (!have_rb[k]) rb_at[k] = p; have_rb[k]++; }   // going up counts
        if (t == UT(B_HOUSE)) { if (nhouse < 512) house[nhouse++] = p; open_house += going; }
        if (done && (t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL)) && ntown < 128) { town[ntown].p = p; town[ntown++].max = t == UT(B_CAPITOL) ? BD_HOUSES_CAPITOL : BD_HOUSES_TC; }
        if (t == UT(B_AIRPORT) && nap < 32) app[nap++] = p;
        nairport += t == UT(B_AIRPORT);   // going up too (29 laid in an hour by the asker's count, which sees the finished ones)
        open_airport += t == UT(B_AIRPORT) && !done;   // LAID, NOT PAID: its foundation shows
        if (is_tower_type(t)) open_tower += going;
        if ((is_tower_type(t) || defence_stats(u, &rng, &att)) && ndef < 512) def[ndef++] = p;
        if ((t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL) || t == UT(B_SETTLEMENT)) && (done || going) && nds < BD_DSITES) dsite[nds++] = p;
        if (done && (t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL) || t == UT(B_SETTLEMENT)) && ncen < 256) {
            centre[ncen].p = p; centre[ncen].town = t != UT(B_SETTLEMENT); centre[ncen++].cost = wk_cost(s, ter_near(p.x, p.y));
        }
        if (!done && st && no < 96) openb[no++] = u;
    }
    b->open = no; b->lost_ground = lost_ground;
    g_bd_gr[idx].n = ngran; memcpy(g_bd_gr[idx].g, gran, (size_t)ngran * sizeof gran[0]);   // SCARCE GROUND: the places round them
    int sends = 0, pile = no - open_fields;   // the unfinished that are no fields (a row of new fields is no pile)
    // FARMS PAID FIRST ([ai_base] farm_builders_paid; the player 2026-11-23: "lots of p3 citezens where bogged down trying
    // to build up a farm witout having the wood. the ai should only send citecens to build a farm once it has the needed
    // wood, and not more than 1 per farm lot"). A field takes its wood when its building starts - laid it has 1 hit
    // point, and its builder stands at it until the wood is there. A granary's 8 came as foundations at once, each got a
    // builder with 28 wood in the bank, and HOLDING THE GROUND sent its idle citizens to the nearest unfinished
    // building, a field: on Plains (seed 3, 50 min) 20-45 citizen-minutes an AI stood at unstarted fields, up to 23 at
    // one. Now a field not started gets its builder only while the wood in stock pays it after those not started that
    // have one, a field has one builder at most whoever sent them (the others let go: a move to where it stands, so it
    // is idle there at once and work.c gives it work - not a halt, which in the middle of a fight crashed the game's
    // land movement, defence.c NO CHASE), one not started whose wood is not there is let go too, and HOLDING THE
    // GROUND sends nobody to a field.
    int fwood = player_get_stock(pl, 3);
    int farm_paid = g_set_ai[1].farm_builders_paid;
    for (int pass = 0; farm_paid && pass < 2; pass++)   // the started first (their wood is paid), then those not started, in order
        for (int i = 0; i < no; i++) {
            int ft = unit_type_get_id(unit_get_type_data(openb[i])), fcost = unit_cost(pl, ft, 3);   // its own: a Robotic Farm's is another's
            if (!is_field_type(ft) || (unit_get_hp(openb[i]) > 1) != !pass) continue;
            ee_unit *site = field_site(openb[i]);
            int kept = 0;
            for (int k = 0; k < nbuilt; k++) {
                if (built[k] != site && built[k] != openb[i]) continue;
                int go = kept || (pass && fwood < fcost && fcost > 0);
                if (!go) { kept = 1; if (pass) fwood -= fcost; continue; }
                if (pc_held(builder[k])) continue;   // the player's (THE PLAYER'S ORDERS FIRST)
                cl_end(builder[k], CLW_HOLD);
                if (!unit_goal(builder[k], goal_move((float)unit_get_x(builder[k]) + 0.5f, (float)unit_get_y(builder[k]) + 0.5f), 0, 0x1F40)) continue;
                if (kept) b->farm_extra++; else b->farm_nowood++;
                built[k] = NULL;
            }
        }
    g_bd_wait[idx].n = 0; g_bd_wait[idx].t = now ? now : 1;
    for (int i = 0; i < no && sends < (pile >= BD_PILE ? 2 * BD_SENDS : BD_SENDS) && nf; i++) {   // BUILDERS
        ee_unit *site = field_site(openb[i]);
        // MORE BUILDERS WHEN THEY PILE UP: one citizen a building, as before, while few stand unfinished; 2 a building
        // from BD_PILE of them, 3 from twice that (a field always one) - the player's P4 had 13 to 23 buildings
        // unfinished all through its game with 369 citizens. (3 a building always: twice the builders in the first
        // half hour, 6 soldiers fewer at minute 28 for one of four.)
        int has = 0, bk = -1, bd = 0x7FFFFFFF;
        for (int k = 0; k < nbuilt; k++) has += built[k] == site || built[k] == openb[i];
        ivec2 p = unit_get_tile(openb[i]);
        int ot = unit_type_get_id(unit_get_type_data(openb[i])), field = is_field_type(ot), fcost = field ? unit_cost(pl, ot, 3) : 0;
        has = has >= (field ? 1 : pile >= 2 * BD_PILE ? 3 : pile >= BD_PILE ? 2 : 1);
        if (has || raiders_known(pl, p, 8) || tv_unsafe(idx, p.x, p.y) || wk_danger(idx, p)) continue;   // remembered raiders too: the site may lie in the fog
        int unpaid = farm_paid && field && unit_get_hp(openb[i]) <= 1;   // FARMS PAID FIRST
        if (unpaid && fcost > 0 && fwood < fcost) { b->farm_wait++; continue; }
        unsigned land = area_id(p, 2);
        for (int k = 0; k < nf; k++) {
            ivec2 q = unit_get_tile(freec[k]);
            int d = ivec2_dist2(p, q);
            if (d >= bd || (land && area_id(q, 2) && area_id(q, 2) != land)) continue;
            bd = d; bk = k;
        }
        // nor along a way with enemy soldiers on it (P2's builders walked through P4's army to a foundation behind
        // it, 36 dead: attack.c citizens_safety)
        if (bk < 0 && land && g_bd_wait[idx].n < BD_WAIT_LANDS) {   // ACROSS THE WATER: nobody of its on that land to build it
            int k = 0;
            while (k < g_bd_wait[idx].n && g_bd_wait[idx].land[k] != land) k++;
            if (k == g_bd_wait[idx].n) { g_bd_wait[idx].land[k] = land; g_bd_wait[idx].at[k] = p; g_bd_wait[idx].n++; }
        }
        if (bk < 0 || path_raided(pl, unit_get_x(freec[bk]), unit_get_y(freec[bk]), p.x, p.y) || !bd_send(pl, freec[bk], openb[i])) continue;
        freec[bk] = freec[--nf];
        sends++; b->sent++;
        if (unpaid) fwood -= fcost;
    }
    // HOLDING THE GROUND ([ai_base] hold_ground; the player 2026-11-19, his game: "both player landed and build
    // settlments, p2 managed to build a tower first, the citezens of p3 just gave up instead of continuing building their
    // outpost, they should try to build their own towers and production buildings, anething is better than stanting
    // around waiting to be cleared off" - P2's soldiers brought down P3's tower going up and went for its Settlement, 4 of
    // its citizens stood idle beside them: BUILDERS sends none where enemy soldiers are within 8 tiles or the territory is
    // being lost, which at home waits for the soldiers' help, and over the water waits to be cleared off). Its idle
    // citizens (no order) with enemy soldiers it knows within BD_HOLD_R, or in a territory where it has seen enemy
    // buildings or soldiers: those of the land with most of them lay, by their middle, a tower if none of its stands or
    // goes up within BD_HOLD_TOWER, else a production building (produce.c's first kind it can build) if none within
    // BD_HOLD_PROD - the nearest spot out of every enemy tower's reach it knows (work.c wk_danger), one every
    // BD_HOLD_EVERY - and build it, BD_HOLD_N of them; nothing to lay: the nearest unfinished building of its there.
    // MEASURED 2026-11-19 (head-to-head, Small Islands large, 4 AIs from epoch 5, 60 min, seeds 1-12 mirrored, 24 games,
    // both sides push_across and fight_production): 203 towers and 107 production buildings laid by the B seats; held
    // at the end islands 1.35 against 1.06, miners off the home island 11.3 against 5.1, drop sites there 5.1 against
    // 3.0, buildings there 10.8 against 6.4, idle citizens 0 against 0.3; soldiers -6.8 (se 5.3), kills +10.4 (9.8),
    // losses +12.0 (9.9), razed +0.2 (0.5). More ground held for more fighting: ON. (Its first run took the idle ones
    // from freec, which holds gatherers only: it never fired, and its numbers - soldiers +4.6, citizens off the home
    // island 23.9 against 17.6 - are what the noise of these 24 games looks like.)
    if (HOLD_GROUND(idx) && nidle) {
        static ee_unit *hc[384];
        static unsigned hl[384];
        int nh = 0;
        for (int k = 0; k < nidle; k++) {   // idle only: those gathering are at work
            ee_unit *u = idlec[k];
            ivec2 q = unit_get_tile(u);
            unsigned r = ter_near(q.x, q.y);
            int hot = raiders_known(pl, q, BD_HOLD_R) || (r < (unsigned)s->n && !g_ter.t[r].water && (s->t[r].ebld > 0 || s->t[r].foe > 0));
            if (hot && area_id(q, 2)) { hl[nh] = area_id(q, 2); hc[nh++] = u; }
        }
        unsigned land = 0;
        for (int k = 0, bn = 0; k < nh; k++) {   // the land with most of them
            int c = 0;
            for (int j = 0; j < nh; j++) c += hl[j] == hl[k];
            if (c > bn) { bn = c; land = hl[k]; }
        }
        long sx = 0, sy = 0;
        int nl = 0;
        for (int k = 0; k < nh; k++) if (hl[k] == land) { hc[nl] = hc[k]; sx += unit_get_x(hc[k]); sy += unit_get_y(hc[k]); nl++; }
        ee_unit *job = NULL;
        const char *what = NULL;
        if (nl) {
            ivec2 m = iv2((int)(sx / nl), (int)(sy / nl));
            static const int none[7];
            int near_def = 0, near_prod = 0, type = 0;
            for (int j = 0; j < ndef && !near_def; j++) near_def = area_id(def[j], 2) == land && ivec2_dist2(def[j], m) <= BD_HOLD_TOWER * BD_HOLD_TOWER;
            for (int j = 0; j < nprod && !near_prod; j++) near_prod = area_id(prodp[j], 2) == land && ivec2_dist2(prodp[j], m) <= BD_HOLD_PROD * BD_HOLD_PROD;
            int ttype = tower_newest(pl);
            if (!near_def && ttype && can_build(pl, ttype) && bd_pays(pl, ttype, none)) { type = ttype; what = "a tower"; }
            else if (near_def && !near_prod)
                for (int k = 0; k < PR_NK && !type; k++) {
                    int c = g_ut[PR_KINDS[k]];
                    if (ut_has(c) && can_build(pl, c) && bd_pays(pl, c, none)) { type = c; what = "a production building"; }
                }
            if (type && (!b->hold_t || now < b->hold_t || now - b->hold_t >= BD_HOLD_EVERY)) {
                ivec2 at = iv2(-1, -1);
                for (int d = 2; d <= 10 && at.x < 0; d++)
                    for (int dx = -d; dx <= d && at.x < 0; dx++)
                        for (int dy = -d; dy <= d; dy += (abs(dx) == d ? 1 : 2 * d)) {
                            ivec2 q = iv2(m.x + dx, m.y + dy);
                            if (area_id(q, 2) != land || area_id(q, 1) || wk_danger(idx, q) || !can_place_open(pl, type, q)) continue;
                            at = q;
                            break;
                        }
                job = at.x >= 0 ? lay_foundation(pl, type, at.x, at.y) : NULL;
                b->hold_t = now ? now : 1;
                if (job) b->holds++;
                if (g_log) log_locked("# build: P%u at %.1f min: holding the ground on land %u - %s (type %d) %s at %d,%d, %d idle citizens there with the enemy near\n", idx, now / 60000.0, land, what, type,
                                      job ? "laid" : "has no spot", at.x, at.y, nl);
            }
            if (!job) {   // nothing to lay: the nearest unfinished building of its on that land, out of enemy towers' reach
                int bd = BD_HOLD_PROD * BD_HOLD_PROD + 1;
                for (int i = 0; i < no; i++) {
                    ivec2 p = unit_get_tile(openb[i]);
                    int d = ivec2_dist2(p, m);
                    if (farm_paid && is_field_type(unit_type_get_id(unit_get_type_data(openb[i])))) continue;   // FARMS PAID FIRST: no field
                    if (d < bd && area_id(p, 2) == land && !wk_danger(idx, p)) { bd = d; job = openb[i]; }
                }
            }
            if (job) bd_hold_mark(idx, job, now);   // its builders stay at it (attack.c citizens_safety)
            for (int k = 0, sent = 0; job && k < nl && sent < BD_HOLD_N; k++)
                if (cl_take(hc[k], CLP_HOLD, CLW_HOLD, CL_CIT_T) && bd_send(pl, hc[k], job)) { sent++; b->hold_sent++; }
        }
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
    // the Airport has its own slot (5): booked as a tower (2) it held every tower back BD_WAIT, and its price stayed held
    // after its foundation showed, the towers' going-up count not seeing it; wait[5] holds nothing back but its price
    int wait[6], shown[6] = { open_gran, open_settle, open_tower, 0, open_prod, open_airport }, com[7] = { 0 };
    for (int k = 0; k < 6; k++) wait[k] = b->laid_t[k] && now >= b->laid_t[k] && now - b->laid_t[k] < BD_WAIT;
    for (int k = 0; k < 3; k++) if (b->laid_type[3] == rb[k]) shown[3] = have_rb[k];
    for (int k = 0; k < 6; k++)
        if (wait[k] && !shown[k] && b->laid_type[k] > 0) for (int r = 2; r < 7; r++) com[r] += unit_cost(pl, b->laid_type[k], r);
    int fty = bd_field_type(pl);   // the field it lays (THE FIELD LINE); the room for one: a Farm's footprint (the line's are alike)
    for (int r = 2; r < 7 && fty; r++) com[r] += open_fields * unit_cost(pl, fty, r);
    memcpy(g_bd_com[idx], com, sizeof com);
    // AN AIRPORT SAVED FOR (below): asked for and not paid - nothing but fields, research and production buildings is laid
    // meanwhile, BD_AP_SAVE at a stretch and then as long not (from the World War an AI's wood stood between 9 and 369 for
    // 19 minutes, a granary or a Settlement taking each 125 as it came: the wing wanted from minute 18, its first
    // Airport at minute 37)
    int ap_ask = g_bd_airport[idx] && now >= g_bd_airport[idx] && now - g_bd_airport[idx] < 120000 && nairport < g_bd_airport_n[idx] && ut_has(UT(B_AIRPORT)) && prio_buildable(pl, UT(B_AIRPORT));
    int ap_save = 0, ap_norm = ap_ask, ap_fwd = -1;
    if (PV(idx, g_v_bd_ap_fwd, g_bd_ap_fwd_on) && g_bd_ap_fwd[idx].t && now >= g_bd_ap_fwd[idx].t && now - g_bd_ap_fwd[idx].t < 120000 && nap && nairport < BD_AP_FWD_MAX &&
        !open_airport && ut_has(UT(B_AIRPORT)) && prio_buildable(pl, UT(B_AIRPORT))) {   // THE AIRPORTS MOVE UP
        float nearest = 1.0e9f;
        for (int k = 0; k < nap; k++) { float d = sqrtf((float)ivec2_dist2(app[k], g_bd_ap_fwd[idx].at)); if (d < nearest) nearest = d; }
        if (nearest > g_bd_ap_fwd[idx].reach) {
            nearest -= (float)BD_AP_FWD_GAIN;
            for (int c = 0; c < ncen; c++) {
                float d = sqrtf((float)ivec2_dist2(centre[c].p, g_bd_ap_fwd[idx].at));
                if ((centre[c].cost == 1 || centre[c].cost == 2) && d < nearest) { nearest = d; ap_fwd = c; }
            }
            if (ap_fwd >= 0) ap_ask = 1; else b->ap_fwd_none++;
        }
    }
    if (ap_ask && !bd_pays(pl, UT(B_AIRPORT), com) && workforce(pl) >= g_min_workforce) {
        if (!b->ap_save_t || now < b->ap_save_t || now - b->ap_save_t >= 2 * BD_AP_SAVE) b->ap_save_t = now ? now : 1;
        ap_save = now - b->ap_save_t < BD_AP_SAVE;
    }
    int placed = 0;
    if (fty)   // FIELDS
        for (int g = 0; g < ngran && placed < 2 && open_fields < 4; g++)
            for (int fx = -2; fx <= 2 && placed < 2 && open_fields < 4; fx += 2)
                for (int fy = -2; fy <= 2 && placed < 2 && open_fields < 4; fy += 2) {
                    ivec2 q = iv2(gran[g].x + fx, gran[g].y + fy);
                    int there = !fx && !fy;
                    for (int k = 0; k < nfld && !there; k++) there = fld[k].x == q.x && fld[k].y == q.y;
                    if (there || !can_place(pl, fty, q) || !bd_pays(pl, fty, com) || raiders_near(pl, q, 8) || tv_unsafe(idx, q.x, q.y) || wk_danger(idx, q)) continue;
                    if (!lay_foundation(pl, fty, q.x, q.y)) continue;
                    if (nfld < 512) fld[nfld++] = q;
                    placed++; open_fields++; b->fields++;
                    for (int r = 2; r < 7; r++) com[r] += unit_cost(pl, fty, r);
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
    // the step that chose type, where that step books its choice (BD_PICK_*): booked once the choice reaches the lay
    // below - A CLAIM takes over a look's choice, and its Airport ask cleared, its herd's clock started, counted the
    // Settlement it never laid
    int pick = 0;
    ivec2 at = iv2(0, 0);
    // THE GRANARY OF ITS LINE: the Granary, or the Improved Granary once that is the one it may build - in Art of
    // Conquest's Space Age the plain one is no longer to be had (Small Planets 2026-11-23: no granary, no field, every
    // AI with 30-60 citizens idle and none on food from minute 25)
    int gtype = ut_has(UT(B_GRANARY)) && can_build(pl, UT(B_GRANARY)) ? UT(B_GRANARY) : ut_has(UT(B_GRANARY_IMPROVED)) && can_build(pl, UT(B_GRANARY_IMPROVED)) ? UT(B_GRANARY_IMPROVED) : 0;
    ivec2 cen_p = iv2(-1, -1), town_p = iv2(-1, -1);   // CLEARING GROUND (clearing.c): the best centre, the best town - where a want is
    for (int pass = 1; pass <= 3; pass++)
        for (int c = 0; c < ncen; c++)
            if (centre[c].cost == pass) { if (cen_p.x < 0) cen_p = centre[c].p; if (centre[c].town && town_p.x < 0) town_p = centre[c].p; }
    if (!placed && !ap_save && current_epoch(pl) >= FARM_EPOCH && gtype && bd_pays(pl, gtype, com) && !open_gran && !open_fields && !fields_due &&
        ngran < BD_GRANARIES && (!ngran || g_wk[idx].food_short > 0) && !wait[0] && bd_gran_first(b, idx, ngran >= BD_GRAN_PLAIN ? gran_room : 0, now)) {   // A GRANARY: by its centre in the best territory
        // ROOM AS THERE IS: a place for 6 of its 8 fields first; with none while it has no granary at all (a planet of 20
        // x 20 tiles round its town - Small Planets: 1283 looks without a spot by minute 30, food from the bushes only,
        // then none) the one with the most room, 2 fields at least - fewer fields are food, no granary is none
        int broom = 1;
        ivec2 bq = iv2(0, 0);
        for (int pass = 1; pass <= 3 && !type; pass++)
            for (int c = 0; c < ncen && !type; c++) {
                if (centre[c].cost != pass) continue;
                for (int r = 4; r <= 12 && !type; r++)
                    for (int ax = -r; ax <= r && !type; ax++)
                        for (int ay = -r; ay <= r && !type; ay++) {
                            ivec2 q = iv2(centre[c].p.x + ax, centre[c].p.y + ay);
                            int room = 0;
                            if ((abs(ax) != r && abs(ay) != r) || !can_place_open(pl, gtype, q) || raiders_near(pl, q, 12) || tv_unsafe(idx, q.x, q.y) || wk_danger(idx, q)) continue;
                            for (int fx = -2; fx <= 2; fx += 2)
                                for (int fy = -2; fy <= 2; fy += 2)
                                    if ((fx || fy) && can_place(pl, UT(B_FARM), iv2(q.x + fx, q.y + fy))) room++;
                            if (room > broom && pass == 1) { broom = room; bq = q; }   // ROOM AS THERE IS: in its best territories
                            if (room < 6) continue;
                            type = gtype; at = q; what = "a granary for the fields";
                        }
            }
        if (!type && broom >= 2 && !ngran) { type = gtype; at = bq; what = "a granary for the fields there is room for"; }
        if (!type) { b->no_spot++; if (cen_p.x >= 0) cg_want(idx, gtype, cen_p, 4, 12, 2, 0, now); }   // its 8 fields' places with it
    }
    g_rs_want[idx] = 0;
    produce_look(pl, idx, now);   // produce.c: the production building its territories ask for
    int saving = 0;   // one it can't pay is saved for (the row "research"): nothing after it in this order meanwhile
    for (int k = 0; k < 4 && !type && !placed && !saving; k++) {   // THE RESEARCH BUILDINGS - and, after the University, its first production building
        if (k == 1) {
            if (!g_pr[idx].type || !g_pr[idx].first || wait[4] || open_prod) continue;
            if (!bd_pays(pl, g_pr[idx].type, com)) { g_rs_want[idx] = g_pr[idx].type + 1; saving = 1; continue; }
            if (bd_spot(pl, idx, g_pr[idx].type, g_pr[idx].at, 3, 10, &at)) { type = g_pr[idx].type; what = g_pr[idx].why; } else { b->no_spot++; cg_want(idx, g_pr[idx].type, g_pr[idx].at, 3, 10, 0, 1, now); }
            continue;
        }
        int j = k ? k - 1 : 0;
        if (wait[3] || have_rb[j] || !ut_has(rb[j]) || !prio_buildable(pl, rb[j])) continue;
        if (!bd_pays(pl, rb[j], com)) { g_rs_want[idx] = rb[j] + 1; saving = 1; continue; }
        // THE PAIR (the player 2026-11-16: "some asurance that atleast one hospital overlaps with a temple would be
        // good"): a Hospital keeps the plague's sick alive, a Temple keeps them from infecting others - stations.c's
        // QUARANTINE stands them beside a Hospital on the side of a Temple that reaches there. Its first Hospital goes
        // beside a Temple of its, its first Temple beside a Hospital (whichever comes second), BD_PAIR_R tiles at most:
        // the Temple's reach (8 from its edge) then covers the Hospital's side toward it. The wood sink's others keep
        // their reach apart (AREA OF EFFECT); no spot there: by a town as before.
        int other = j == 1 ? 2 : j == 2 ? 1 : -1;
        if (other > 0 && have_rb[other] && bd_spot(pl, idx, rb[j], rb_at[other], 3, BD_PAIR_R, &at)) { type = rb[j]; what = j == 2 ? "a Temple beside its Hospital" : "a Hospital beside its Temple"; }
        for (int pass = 1; pass <= 3 && !type; pass++)
            for (int c = 0; c < ncen && !type; c++)
                if (centre[c].town && centre[c].cost == pass && bd_spot(pl, idx, rb[j], centre[c].p, 4, 10, &at)) { type = rb[j]; what = j == 2 ? "a Temple" : j ? "a Hospital" : "a University"; }
        if (!type) { b->no_spot++; if (town_p.x >= 0) cg_want(idx, rb[j], town_p, 4, 10, 0, 1, now); }
    }
    if (!type && !placed && g_pr[idx].type && !g_pr[idx].first && !wait[4] && !open_prod && bd_pays(pl, g_pr[idx].type, com)) {   // PRODUCTION
        if (bd_spot(pl, idx, g_pr[idx].type, g_pr[idx].at, 3, 10, &at)) { type = g_pr[idx].type; what = g_pr[idx].why; } else { b->no_spot++; cg_want(idx, g_pr[idx].type, g_pr[idx].at, 3, 10, 0, 1, now); }
    }
    // AN AIRPORT (airstrike.c af_train asks: the wing it wants has too few): by a Town Center or Capitol in a territory it
    // holds, backland first
    if (!type && !placed && ap_ask && !wait[4] && !open_prod && bd_pays(pl, UT(B_AIRPORT), com)) {
        if (ap_fwd >= 0 && bd_spot(pl, idx, UT(B_AIRPORT), centre[ap_fwd].p, 5, 16, &at)) { type = UT(B_AIRPORT); what = "an Airport nearer where its force meets the enemy"; pick = BD_PICK_AIRPORT_FWD; }   // THE AIRPORTS MOVE UP
        for (int pass = 1; pass <= 2 && !type && ap_norm; pass++)
            for (int c = 0; c < ncen && !type; c++)
                if (centre[c].town && centre[c].cost == pass && bd_spot(pl, idx, UT(B_AIRPORT), centre[c].p, 5, 16, &at)) { type = UT(B_AIRPORT); what = "an Airport for the bombers' wing"; pick = BD_PICK_AIRPORT; }
        if (!type) { b->no_spot++; if (town_p.x >= 0) cg_want(idx, UT(B_AIRPORT), town_p, 5, 16, 0, 1, now); }
    }
    cg_clear(pl, idx, now);   // CLEARING GROUND (clearing.c): a want unmet long enough gets its ground
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
            // TOWER FIRST across the water (c 5 or 10: work.c ACROSS THE WATER; the player 2026-11-14: "the citezens
            // imediatly start building a tower ... than a towncenter"): its mine gets its tower before its Settlement
            if ((c == 5 || c == 10) && !guard) { if (score < tbest) { tbest = score; tp = p; } }
            else if (!has_drop) { if (score < sbest) { sbest = score; sp = p; smetal = r >= 5; } }
            else if (!guard && score < tbest) { tbest = score; tp = p; }
        }
        if (!type && !placed && sbest < 0x7FFFFFFF && !open_settle && !wait[1] && ut_has(UT(B_SETTLEMENT)) && can_build(pl, UT(B_SETTLEMENT)) && bd_pays(pl, UT(B_SETTLEMENT), com)) {
            if (bd_spot(pl, idx, UT(B_SETTLEMENT), sp, 2, smetal ? BD_DROP_METAL : 6, &at)) { type = UT(B_SETTLEMENT); what = "a Settlement by a mine"; } else b->no_spot++;
        }
        if (!type && !placed && tbest < 0x7FFFFFFF && !open_tower && !wait[2] && ttype && can_build(pl, ttype) && bd_pays(pl, ttype, com)) {
            if (bd_spot(pl, idx, ttype, tp, 2, 5, &at)) { type = ttype; what = "a tower by a mine"; } else b->no_spot++;
        }
        // A HERD'S SETTLEMENT (the player 2026-11-18, a Stone Age game whose AIs had hunted bare the ground round their
        // drop sites while a herd of 8 grazed 50 tiles off: "yes, drop settlemts near heards"): before farms, while
        // citizens wanted on food find no site (work.c food_short), a wild herd with more grown animals than HERDS KEPT
        // leaves (one to hunt) out of every drop site's hunting reach ([ai_base] food_hunt_range) gets a Settlement 2
        // to 6 tiles from it - in a territory its citizens work or beside one, the cheapest first, then the nearest to
        // the base; one going up at a time, one every BD_HERD_EVERY (a herd walks: one laid where it stood is soon off)
        if (!type && !placed && !open_settle && !wait[1] && g_wk[idx].food_short > 0 && g_herd_keep >= 0 &&
            (current_epoch(pl) < FARM_EPOCH || end_epoch() < FARM_EPOCH) &&
            (!b->herd_t || now < b->herd_t || now - b->herd_t >= BD_HERD_EVERY) &&
            ut_has(UT(B_SETTLEMENT)) && can_build(pl, UT(B_SETTLEMENT)) && bd_pays(pl, UT(B_SETTLEMENT), com)) {
            int hbest = 0x7FFFFFFF, reach2 = g_hunt_range * g_hunt_range;
            ivec2 hp = iv2(0, 0);
            for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
                ee_unit *a = it.u;
                ee_player *o = unit_get_owner(a);
                if (!o || player_get_index(o) != 0 || unit_type_get_family(it.td) != KIND_ANIMAL || unit_get_resource(a) != 2 ||
                    unit_get_hp(a) <= 0 || unit_get_scale(a) < 1.0f || food_fierce(unit_type_get_id(it.td))) continue;
                ee_herd *h = unit_get_herd(a);
                if (!h || bd_herd_grown(h) <= g_herd_keep) continue;
                ivec2 p = unit_get_tile(a);
                int c = wk_cost(s, ter_near(p.x, p.y)), reached = 0;
                if (!c) continue;
                int score = c * 1000000 + ivec2_dist2(p, base);
                if (score >= hbest) continue;
                for (int j = 0; j < nds && !reached; j++) reached = ivec2_dist2(dsite[j], p) <= reach2;
                if (reached || raiders_near(pl, p, 12) || tv_unsafe(idx, p.x, p.y) || wk_danger(idx, p)) continue;
                hbest = score; hp = p;
            }
            if (hbest < 0x7FFFFFFF) {
                if (bd_spot(pl, idx, UT(B_SETTLEMENT), hp, 2, 6, &at)) { type = UT(B_SETTLEMENT); what = "a Settlement by a herd"; pick = BD_PICK_HERD; }
                else b->no_spot++;
            }
        }
        // TO THE TREES ACROSS ([ai_base] forest_across; work.c): while BD_FOREST_IDLE or more of its citizens stand idle,
        // the trees on a land with no drop site of its and no foundation of its waiting for builders - in a territory its
        // citizens may work, the cheapest first, then the nearest to the base, out of danger - get a Settlement 2 to 10
        // tiles off, one every BD_FOREST_EVERY; ACROSS THE WATER below lays it with nobody there, the ferry brings the
        // builders and then the wood cutters
        if (!type && !placed && !open_settle && !wait[1] && FOREST_ACROSS(idx) && EXPAND_ACROSS(idx) && (g_wk[idx].idle >= BD_FOREST_IDLE || g_wk[idx].want[3] - g_wk[idx].have[3] >= BD_FOREST_IDLE) &&   // work.c WOOD ACROSS: short of wood cutters as many
            (!b->forest_t || now < b->forest_t || now - b->forest_t >= BD_FOREST_EVERY) &&
            ut_has(UT(B_SETTLEMENT)) && can_build(pl, UT(B_SETTLEMENT)) && bd_pays(pl, UT(B_SETTLEMENT), com)) {
            b->forest_t = now ? now : 1;
            int fbest = 0x7FFFFFFF;
            ivec2 fp = iv2(0, 0);
            b->forest_n[0] = b->forest_n[1] = b->forest_n[2] = 0;
            for (int r = 0; r < s->n; r++) {
                int c = g_ter.t[r].water ? 0 : wk_cost(s, (unsigned)r);
                if (!c) continue;
                unsigned fl = area_id(g_ter.t[r].c, 2);
                if (fl && (rs_drop_land(idx, fl) || bd_waits(idx, fl, NULL))) continue;
                b->forest_n[0]++;
                ee_unit *tree = wk_tree(pl, s, g_ter.t[r].c, 24, r);
                if (!tree) continue;
                b->forest_n[1]++;
                ivec2 p = unit_get_tile(tree);
                unsigned tl = area_id(p, 2);
                int score = c * 1000000 + ivec2_dist2(p, base);
                if (!tl || rs_drop_land(idx, tl) || bd_waits(idx, tl, NULL) || raiders_near(pl, p, 12) || tv_unsafe(idx, p.x, p.y)) continue;
                b->forest_n[2]++;
                if (score >= fbest) continue;
                fbest = score; fp = p;
            }
            if (fbest < 0x7FFFFFFF) {
                if (bd_spot(pl, idx, UT(B_SETTLEMENT), fp, 2, 10, &at)) { type = UT(B_SETTLEMENT); what = "a Settlement by trees across the water"; pick = BD_PICK_FOREST; }
                else { b->no_spot++; b->forest_nospot++; }
            }
        }
        // A FISHING DOCK (the player 2026-11-18: "improve fishing" - his Stone Age game had 75 fish nobody worked: a dock
        // came only as a harbour, and the harbours run only while the fleet does, which wants an enemy across the water):
        // while citizens wanted on food find no site and the seas are fished (sea_fishing), the fish nearest a drop site
        // of its - BD_FISH_NEAR tiles at most, no dock of its within BD_FISH_R of it - gets a dock on the shore beside it,
        // on that drop site's land; BD_FISH_DOCKS docks at most, one every BD_FISH_EVERY. Laid as a harbour's dock is
        // (fleet.c hb_lay: the game's placement says it laid a dock it didn't); fleet.c boats_tick trains the boats there
        if (!type && !placed && g_wk[idx].food_short > 0 && SEA_FISHING(idx) && ndock < BD_FISH_DOCKS && nds &&
            (!b->fdock_t || now < b->fdock_t || now - b->fdock_t >= BD_FISH_EVERY) &&
            ut_has(UT(B_DOCK)) && can_build(pl, UT(B_DOCK))) {
            int fbest = BD_FISH_NEAR * BD_FISH_NEAR + 1;
            ivec2 fp = iv2(0, 0);
            unsigned fland = 0;
            for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
                if (unit_type_get_id(it.td) != UT(RES_FISH) || unit_get_amount(it.u) <= 0) continue;
                ivec2 p = unit_get_tile(it.u);
                int d = 0x7FFFFFFF, dj = -1, docked = 0;
                for (int j = 0; j < nds; j++) { int dd = ivec2_dist2(dsite[j], p); if (dd < d) { d = dd; dj = j; } }
                if (dj < 0 || d >= fbest) continue;
                for (int j = 0; j < ndock && !docked; j++) docked = ivec2_dist2(dock[j], p) <= BD_FISH_R * BD_FISH_R;
                if (docked || raiders_near(pl, p, 12) || wk_danger(idx, p)) continue;
                fbest = d; fp = p; fland = area_id(dsite[dj], 2);
            }
            if (fland) {
                int laid = 0, tried = 0;
                for (int r = 1; r <= 8 && !laid && !tried; r++)
                    for (int ax = -r; ax <= r && !laid && !tried; ax++)
                        for (int ay = -r; ay <= r && !laid && !tried; ay++) {
                            ivec2 q = iv2(fp.x + ax, fp.y + ay);
                            if ((abs(ax) != r && abs(ay) != r) || !can_place(pl, UT(B_DOCK), q) || fl_dock_land(q) != fland || tv_unsafe(idx, q.x, q.y) || wk_danger(idx, q)) continue;
                            tried = 1;   // the spot: laid now, or the price not there yet (the next look)
                            if (!hb_lay(pl, idx, bm, current_epoch(pl), UT(B_DOCK), q)) break;
                            laid = 1; placed = 1; b->fdock_t = now ? now : 1; b->fdocks++;
                            for (int rr = 2; rr < 7; rr++) g_bd_com[idx][rr] += unit_cost(pl, UT(B_DOCK), rr);
                            if (g_log) log_locked("# build: P%u at %.1f min: a fishing dock - %d at %d,%d by the fish at %d,%d: laid\n", idx, now / 60000.0, UT(B_DOCK), q.x, q.y, fp.x, fp.y);
                        }
                if (!tried) b->no_spot++;
            }
        }
        // A CLAIM (the player 2026-11-15: "controlling the whole map is the endgoal ... but within reason"; "the ai also
        // dosnt yet clam territoires that have no resources in it, they are still needed for map control"): a tower in
        // the middle of a territory nobody has a building in, beside one it holds where BD_CLAIM_GUARD of its soldiers
        // stand (the border is secured first) - the nearest to its base, one every BD_CLAIM_EVERY (divided by the
        // strategy plan's spread, plans.c; BD_CLAIM_OPEN times it going up), when nothing else is to lay and it has
        // min_workforce citizens. The tower makes the territory its own (territories.c TV_HOLDS): the
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
        float sf = plan_f(idx, LN_SPREAD);   // plans.c: its plan's spread - claims that often more, that many more going up
        uint32_t claim_every = (uint32_t)((float)BD_CLAIM_EVERY / sf);
        int claim_open = (int)((float)BD_CLAIM_OPEN * sf + 0.5f);
        if (claim_open < 1) claim_open = 1;
        for (unsigned k = 0; k < 8 && k < b->nclaim_open; k++) {
            int r = b->claim_open[k].r;
            claims_going += r >= 0 && r < s->n && s->t[r].role == TVR_NONE && now >= b->claim_open[k].t && now - b->claim_open[k].t < BD_CLAIM_LONE;
        }
        b->claim_why = placed ? "something else laid" : !ctype ? (wait[2] ? "a tower just laid" : "no tower or Barracks to build or pay") :
                       workforce(pl) < g_min_workforce ? "too few citizens" : held_r >= 0 ? NULL :
                       lone >= 0 ? "a claim's Settlement still missing" : claims_going >= claim_open ? "claims still going up" :
                       b->claim_t && now >= b->claim_t && now - b->claim_t < claim_every ? "the last one too recent" : NULL;
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
                    // a scout passing kept the claims off for good on a busy map. A tower of its there guards it too ([ai_base]
                    // claim_from_towers; the player 2026-11-22, "p2 is very passive (barely expanding)": his P2's home island
                    // full at minute 72, its foothold across the water - a gold mine's tower and Settlement, 63 and 79 free
                    // beside it - never had 3 soldiers stationed, no claim in the 80 minutes since). Off: on his save (minute
                    // 169 -> 200, 5 game_seeds) P2 held 8 -> 10.8 territories with it, 8 -> 11 without - its force had come to
                    // the foothold by then (the stations' share from minute 152), the claims came of the soldiers either way
                    guarded = o < (unsigned)s->n && !g_ter.t[o].water && s->t[o].role != TVR_NONE && s->t[o].state != TV_UNSAFE &&
                              (s->t[o].sold >= BD_CLAIM_GUARD || (g_set_ai[1].claim_from_towers && s->t[o].def > 0));
                }
                if (!guarded) continue;
                b->claim_n[2]++;
                ivec2 c = g_ter.t[r].c;
                if (raiders_near(pl, c, 12) || wk_danger(idx, c)) continue;
                b->claim_n[3]++;
                // mine_r holds the first PR_T territories (as produce.c's tables): one past them counts no mine
                int d = ivec2_dist2(c, base) / (r < PR_T && mine_r[r] ? BD_CLAIM_MINE * BD_CLAIM_MINE : 1);
                if (cb < 0 || d < cd) { cb = r; cd = d; }
            }
            ivec2 cat;
            if (cb < 0) b->claim_why = "no territory to claim";
            else if (bd_spot(pl, idx, ctype, cb == held_r ? held_at : g_ter.t[cb].c, 0, ctype == ttype ? 8 : 12, &cat)) {
                at = cat; type = ctype; pick = 0; b->claim_t = now ? now : 1; b->claims++; b->claim_why = "claimed";
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
    // buildings stand unfinished. Its kinds with an area of effect (AREA OF EFFECT below) go only where none of its kind
    // reaches yet - the University among them (the player 2026-11-16: "it should add universities ... it prevents
    // conversions, so atleast one per territory as save space isnt a bad idea"; its own step at first, every minute
    // with the money, made it the first building beside every new Settlement - the player 2026-11-17: "a bit agressive
    // ... i also fear it got a bit of a sepcial case isntead of being part of teh regular systems"). No front territory
    // to build in: a University or a production building in a quiet backland.
    // THE STONE SINK ([ai_base] stone_sink; territories.c BORDER TOWERS has its towers): with BD_STONE_SINK stone in the
    // bank the kinds that cost stone too - the Hospital, the Temple, the University - are laid by these rules without
    // BD_SINK wood (the price itself still in the bank: bd_pays)
    int wsink = player_stock_at(pl)[3] >= BD_SINK, ssink = g_set_ai[1].stone_sink && player_stock_at(pl)[4] >= BD_STONE_SINK;
    if (!type && !placed && (wsink || ssink) && open_sink < BD_SINK_OPEN && (!b->sink_t || now < b->sink_t || now - b->sink_t >= BD_SINK_EVERY)) {
        int kinds[4 + PR_NK], nk = 0;
        kinds[nk++] = rb[1]; kinds[nk++] = rb[2]; kinds[nk++] = rb[0]; kinds[nk++] = UT(B_AIRPORT);   // the area's three (BD_AOE), an Airport near the front: the planes' reach
        for (int k = 0; k < PR_NK; k++) kinds[nk++] = g_ut[PR_KINDS[k]];
        static uint8_t have[BD_SINK_T][4 + PR_NK];
        static ivec2 aoe_at[BD_AOE][256], place[BD_SINK_T];   // AREA OF EFFECT: its Hospitals', Temples' and Universities' middles; each territory's place
        int tn = s->n < BD_SINK_T ? s->n : BD_SINK_T, cnt = 0, naoe[BD_AOE] = { 0, 0, 0 }, reach[BD_AOE] = { bd_reach(pl, kinds[0]), bd_reach(pl, kinds[1]), bd_reach(pl, kinds[2]) };
        memset(have, 0, (size_t)tn * sizeof *have);
        for (ee_unit *u = player_get_units(pl); u && cnt < 16384; u = unit_get_next(u), cnt++) {
            ee_unit_type *td = unit_get_type_data(u);
            if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
            unsigned r = ter_near(unit_get_x(u), unit_get_y(u));
            for (int k = 0; r < (unsigned)tn && k < nk; k++) if (kinds[k] >= 0 && unit_type_get_id(td) == kinds[k] && have[r][k] < 255) have[r][k]++;
            for (int k = 0; k < BD_AOE; k++)
                if (kinds[k] >= 0 && unit_type_get_id(td) == kinds[k] && naoe[k] < 256) {
                    int sz = unit_type_get_size(td);
                    aoe_at[k][naoe[k]++] = iv2(unit_get_x(u) + sz / 2, unit_get_y(u) + sz / 2);
                }
        }
        for (int r = 0; r < tn; r++) place[r] = g_ter.t[r].c;
        for (int j = 0; j < ncen; j++) { unsigned r = ter_near(centre[j].p.x, centre[j].p.y); if (r < (unsigned)tn) place[r] = centre[j].p; }
        // no quiet front territory with room: a University or a production building in a quiet backland (the fronts of
        // the player's P4 were all under attack, 100,000 wood in the bank)
        int br = -1, bk = -1, bn = 0;
        for (int back = 0; back < 2 && br < 0; back++)
        for (int r = 0; r < tn; r++) {
            if (g_ter.t[r].water || s->t[r].role != (back ? TVR_BACK : TVR_FRONT) || s->t[r].state != TV_QUIET) continue;
            for (int k = back ? 2 : 0; k < nk; k++) {
                if (kinds[k] < 0 || (back && k == 3) || have[r][k] >= (k == 3 ? 1 : BD_SINK_MAX) ||   // one Airport a territory (18 of one player's in the player's game)
                     !ut_has(kinds[k]) || !prio_buildable(pl, kinds[k]) || !bd_pays(pl, kinds[k], com)) continue;
                if (!wsink && (k >= BD_AOE || unit_cost(pl, kinds[k], 4) <= 0)) continue;   // THE STONE SINK: only those that cost stone
                int covered = 0;   // AREA OF EFFECT: one of its kind already reaches the territory's place
                for (int j = 0; k < BD_AOE && j < naoe[k] && !covered; j++) covered = ivec2_dist2(aoe_at[k][j], place[r]) < reach[k] * reach[k];
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
                for (int j = 0; bk < BD_AOE && j < naoe[bk] && !near_same; j++) near_same = ivec2_dist2(aoe_at[bk][j], mid) < reach[bk] * reach[bk];   // AREA OF EFFECT: the spot itself too
                if (!near_same) { type = kinds[bk]; what = s->t[br].role == TVR_FRONT ? (wsink ? "the wood sink: one more in a front territory" : "the stone sink: one more in a front territory") : (wsink ? "the wood sink: one more in a backland" : "the stone sink: one more in a backland"); b->sink_t = now ? now : 1; b->sinks++; }
            }
            if (!type) { b->no_spot++; if (near_same) b->sink_t = now ? now : 1; }   // too near its own: the next look in BD_SINK_EVERY
        }
    }
    if (type) {
        census_build_ask(bm, type);   // the test log's count: the mod's, not the game's entry
        if (pick == BD_PICK_AIRPORT) g_bd_airport[idx] = 0;
        else if (pick == BD_PICK_AIRPORT_FWD) { g_bd_ap_fwd[idx].t = 0; b->ap_fwds++; }
        else if (pick == BD_PICK_HERD) { b->herd_t = now ? now : 1; b->herds++; }
        else if (pick == BD_PICK_FOREST) b->forests++;
        // ACROSS THE WATER: on a land with no citizen of its the game's placement makes no foundation (it looks for a
        // builder first): laid every minute from 43 to 59, it stood once the outposts' citizens came - laid as its own,
        // a builder's start pays it, and BUILDERS above has the ferry bring citizens to it
        unsigned aland = area_id(at, 2);
        if (EXPAND_ACROSS(idx) && aland && !bd_citizen_on(pl, aland)) { ok = lay_foundation(pl, type, at.x, at.y) != NULL; if (ok) b->across++; }
        else ok = hook_place(bm, type, at.x, at.y);
        if (!ok) b->refused++;
        else {
            int k = type == UT(B_GRANARY) || type == UT(B_GRANARY_IMPROVED) ? 0 : type == UT(B_SETTLEMENT) ? 1 : type == rb[0] || type == rb[1] || type == rb[2] ? 3 : pr_kind(type) >= 0 ? 4 : type == UT(B_AIRPORT) ? 5 : 2;
            if (type == UT(B_HOUSE)) b->houses++;
            else if (k == 0) b->granaries++; else if (k == 1) b->settlements++; else if (k == 3) b->research++; else if (k == 4) { b->production++; g_pr[idx].type = 0; if (type == g_pr_more[idx]) g_pr_more[idx] = 0; } else if (k == 2) b->towers++;
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
        log_locked("# build: P%u at %.1f min: %d granaries, %d fields (%d going up, %d places still without one), %d unfinished buildings (%d left on lost ground) | mines in reach %d, with a drop site %d, with a defence %d | "
                   "so far %u builders sent (fields: %u waited for the wood, %u let go for it, %u let go as a second), %u fields, %u granaries, %u Settlements, %u towers, %u research and %u production buildings laid, %u not laid, %u without a spot | wall pieces going up %d, builders sent to them %u | "
                   "claims %u, the last look: %s (territories nobody holds %d, quiet %d, beside a guarded one %d, safe %d) | Settlements by a herd %u, by trees across the water %u (the last look: %d territories on a land with no drop site of its, %d with a tree, %d out of danger; %u found no spot), fishing docks %u, laid across the water %u\n",
                   idx, now / 60000.0, ngran, nfld, open_fields, fields_due, no, b->lost_ground, b->mines, b->mines_drop, b->mines_guard, b->sent, b->farm_wait, b->farm_nowood, b->farm_extra, b->fields, b->granaries, b->settlements, b->towers, b->research, b->production, b->refused, b->no_spot, nw, b->walls,
                   b->claims, b->claim_why ? b->claim_why : "-", b->claim_n[0], b->claim_n[1], b->claim_n[2], b->claim_n[3], b->herds, b->forests, b->forest_n[0], b->forest_n[1], b->forest_n[2], b->forest_nospot, b->fdocks, b->across);
    }
}
internal void build_new_game(void) { memset(g_bd, 0, sizeof g_bd); memset(g_bd_holds, 0, sizeof g_bd_holds); memset(g_bd_com, 0, sizeof g_bd_com); produce_new_game(); }
