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
//   spot's third loss: never again. With towns_remove_unbuilt a town going up whose citizens are all dead is lost too,
//   its unfinished buildings removed: their 1-hp foundations kept drawing the game's repair round's citizens
//   (attack.c ONE AT A TIME).
// 2 SETTLEMENT: the drop site a Settlement, never a Town Center.
// 8 FORWARD BASE (the player: "not just when the AI is ahead - a great way to push in a stalemate, a fortified position
//   from which to attack and retreat under cover, able to heal and train quick reinforcements; the point is to be
//   quick about it, the citizens travel with the army and drop the base under its cover within a few minutes"): where
//   the army is - its staging point with 8+ of its soldiers (stage.c: just out of the enemy's reach on the way to the
//   front), else its biggest group out in the field (8+ land soldiers in an 8-tile cell 32+ tiles from its base),
//   pulled back towards its base to the first tile out of the enemy's reach it knows (stage.c stage_danger) - 32+
//   tiles from its base and its drop sites and other forward bases, its army at least 0.8 x the
//   enemy's it knows, a tower it can pay now (the AIs' stock is spent as it comes: 100-250 wood and stone at a time
//   against 250 and 450 for two towers and a Hospital; the budget's reserve holds training back for 3 minutes): towns_citizens + 2 of its nearest free citizens walk there and
//   build, 4 on each building - two towers towards the front, a Hospital (healing), a production building of the kind
//   army_plan last picked (else Barracks), two towers on the flanks, and once the two front towers stand an 11-tile
//   wall 3 tiles in front of them, straight at the nearest of the 8 directions, its ends open (defence.c's front wall:
//   the wall manager's 0x497C3F, 3 tries 3 minutes apart, not into an enemy army). No drop site: its citizens go back
//   to the game's economy once it stands. One going up at a time, at most 2; outside towns_max.
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
// The forward base at the army (8; 12 with FORWARD), from epoch 5, 60 min, off - on: huge 2 AIs 8 soldiers +5.0 (5.6),
// workers +19.2 (11.5); 12 soldiers +5.0 (7.8), workers +22.8 (12.1); large 3 AIs 8 soldiers -0.1 (5.7), kills -11.7
// (8.1); 12 soldiers +3.8 (7.3), kills -8.9 (8.7) - within noise, workers lean lower (its 8 citizens walk far). Its
// two towers stood 3-5 minutes after the decision, the rest followed as the money came (the AIs hold 100-250 wood
// and stone at a time); later bases followed the army up to 166 tiles out. Off.
// 16 SPREAD (the player 2026-10-31: "if the AI can't expand everything else becomes moot"; two AIs on a huge map
//   should share half of it each by the end): 1v1 on huge Continental with 1, 120 min, each AI held under one town
//   (0.8) - every free mine group lay beyond 90 tiles of the base (24 of 40) or by a drop site, and a town waited for
//   the home mines to fill (the commonest "no new town"). With 16 (on top of 1) a town goes up without waiting, at
//   the explored resources (mines, wood, food: tn_site_spread) nearest to any of its towns - 32 tiles out at least,
//   however far (the player: "custom scenarios are a thing - a 512 x 512 desert with one oasis in the middle"): the
//   AI spreads ring by ring; one town going up per TN_SPREAD_BUILDING workers at a time. No count limit
//   (the player: "if the map allows 100 towns because it's ridiculously large and the AI has the pop to support it,
//   that's ok"; towns_max doesn't apply): a new town needs TN_SPREAD_WORKERS workers for every town it would then
//   hold. The safety checks (army outmatched, enemies in its base, one at a time) stay. With towns_care (on) the spot and
//   its way must be safe (tn_safe; more parts measured, off: TN_CARE).
// 128 FORT (the player 2026-11-08: "inland they also usefull, if an island/region (land maps) isnt fully claimed yet the ai
//   should reinforce its outposts"; expand.c FORT on other islands): a resource town while its continent isn't fully claimed
//   - an enemy building known on it, or an explored mine with no drop site within 18 tiles, decided from what is there, not by
//   the map's kind: at least 4 towers, the first four on the corners of a square round the mine nearest its drop site (defence.c
//   SITE FORTS' square), rebuilt up to 8 times and never dropped for want of money; walls on the three sides away from the drop
//   site once two towers stand; a garrison on foot, held from the game's commander and the force (armies.c ar_take: only
//   soldiers at home, never those out on a push), one soldier per three citizens, 3 at least, 8 with enemies seen there in
//   the last 5 minutes, 12 at most, a quarter of its soldiers over all towns. It takes orders from here only and goes back to
//   the game when the town is lost or the continent claimed.
// 256 TAKE MINES (the player 2026-11-08: "attacking these mines and fortefing them after is an important step, it gives more
//   resouces and starves the enemy (no cheating ofcourse)"): the force's pushes (armies.c ar_route, ar_target) go for the
//   enemy drop sites it knows with a mine it has seen within 10 tiles, the others only when there is none. Such a drop site
//   down, no other enemy building it knows within 22 tiles and nobody's drop site within 18: tn_claim founds a town there
//   (the force first clears what else stands within 22 tiles: armies.c ar_near_building)
//   (claim: no houses, at least four towers on a square round the mine, walls, rebuilt as FORT's, no garrison: the soldiers of
//   FORT were missed at the front), and the force stays until its drop site and two towers stand, AR_CLAIM_MS at most.
//   THE CREW (the player: "yes" to the citizens travelling with the army; of the mines taken with citizens walking from home
//   the town stood 8-15 minutes after the mine fell, the force holding 4): a push at a drop site by mines takes TN_CREW free
//   citizens along, TN_CREW_BEHIND tiles behind the force's middle; they found the town, and go home when the force does.
#define TN_MAX 128   // a table size, not a limit
#define TN_SPREAD_WORKERS 15
#define TN_SPREAD_BUILDING 50
#define TN_PROJ 24
#define TN_GARRISON 12
#define TN_CREW 6
#define TN_CREW_BEHIND 12
enum { TK_DROP, TK_TOWER, TK_PROD, TK_HOUSE, TK_N };
internal const char *const k_tk[TK_N] = { "drop site", "tower", "production building", "house" };
struct tn_proj { ee_unit *b; int id, kind, type, want, state, tries, waits; ivec2 p; };   // want: a production building's type   // state 0 to come, 1 going up, 2 standing, 3 dropped
struct tn_town {
    ivec2 c, toward;                 // the mine group's middle; the side to face (the enemy it knows)
    int state;                       // 1 founding (citizens walking, buildings going up), 2 held, 0 lost
    int forward, wall_tries;         // a forward base (8), else a resource town; its wall's orders
    uint32_t wall_t;
    struct tn_proj proj[TN_PROJ]; int nproj;
    ee_unit *cit[12]; int cid[12], ncit; uint8_t sent_mine[12];   // sent_mine: assigned to the town's mines once - the game's economy's from then on
    uint32_t t, plan_t;
    uint32_t hurt_t;                 // a building of it destroyed or a citizen of it killed (towns_care)
    struct sfort fort; int fort_set; // FORT: its square (defence.c), walls ordered every fwall_t
    int claim;                       // TAKE MINES: founded where the force razed a drop site
    uint32_t fwall_t, gar_t, foe_t;  // FORT: walls and garrison last looked at, enemies seen there last
    ee_unit *gar[TN_GARRISON]; int gid[TN_GARRISON], ngar;   // FORT: its garrison
};
internal struct tn_st {
    uint32_t t, plan_t, log_t;
    struct tn_town town[TN_MAX]; int n;
    struct { ivec2 c; int losses; uint32_t again; } bad[16]; int nbad;
    unsigned plans, founded, held, lost, laid, built, rebuilt, orders, assigned, sent, unpaid, prods, walls, guards, scouts, scouts_back, grown, gaveup;
    uint32_t grow_t;   // SPREAD's last growth look
    uint32_t lost_t;   // its last town lost (towns_care: a pause)
    ee_unit *scout; int scout_id; uint32_t scout_t;   // SPREAD's scout
    uint32_t fsave;   // saving for a forward base since (0: not)
    int open;         // FORT: its continent isn't fully claimed (tn_plan)
    const char *why;
} g_tn[16];
internal int g_towns = 0, g_v_towns = -1, g_tn_max = 3, g_tn_cits = 6, g_tn_towers = 2, g_tn_remove = 0;
#define TOWNS(idx) PV(idx, g_v_towns, g_towns)
#define TN_FORT(idx) (TOWNS(idx) & 128)
#define TN_TAKE(idx) (TOWNS(idx) & 256)
#define TN_RAID(idx) (TOWNS(idx) & 512)   // MINE RAIDS: armies.c ar_det_tick
#define TN_FORTS(idx, t) (TN_FORT(idx) || (t)->claim)   // this town is built as a fort
#define TN_TRIES(idx, t, kind) (TN_FORTS(idx, t) && (kind) == TK_TOWER ? 8 : 3)   // times a building is laid

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
// its buildings laid but not finished removed (vf24 (0, 0, 0): refunded, as econ.c removes stacked ones): their 1-hp
// foundations drew the game's repair round's citizens to them (attack.c ONE AT A TIME); the count
internal int tn_remove_unbuilt(struct tn_town *t) {
    int removed = 0;
    for (int j = 0; j < t->nproj; j++) {
        struct tn_proj *p = &t->proj[j];
        ee_unit *bs = p->state == 1 && tn_alive(p) ? unit_get_physics(p->b) : NULL;
        if (p->state != 1 || !tn_alive(p) || (bs && unit_get_finished(bs))) continue;
        unit_destroy(p->b, 0, 0, 0);
        p->b = NULL; p->state = 3; removed++;
    }
    return removed;
}
internal int tn_is_drop(int t) { return UT_IN(t, UT_TOWNS, UT_X_SINGLE_PLAYER_SETTLEMENT); }

// the drop sites of every player and the enemy buildings pl knows (intel.c's memory), once per plan
struct tn_world { ivec2 drop[1024]; int nd; ivec2 own[512]; int no; ivec2 foe[1024]; int nf; };   // drop: other players', own: its own and its towns
internal void tn_world(ee_player *pl, unsigned idx, struct tn_world *w) {
    w->nd = w->nf = w->no = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        if (unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !tn_is_drop(unit_type_get_id(it.td))) continue;
        if (unit_get_owner(u) == pl) { if (w->no < 512) w->own[w->no++] = unit_get_tile(u); }
        else if (w->nd < 1024) w->drop[w->nd++] = unit_get_tile(u);
    }
    for (int k = 0; k < TN_MAX; k++)   // its own towns going up count as its drop sites
        if (g_tn[idx].town[k].state && w->no < 512) w->own[w->no++] = g_tn[idx].town[k].c;
    struct intel *in = &g_intel[idx];
    for (int k = 0; k < in->n && w->nf < 1024; k++) {
        struct intel_rec *r = &in->r[k];
        ee_player *o = r->owner > 0 && r->owner < 16 ? player_at(r->owner) : NULL;
        if (r->kind == 0 && o && is_enemy(pl, o)) w->foe[w->nf++] = iv2(r->x, r->y);
    }
}
// the home continent's best free mine group: its middle, its mines in o_n; 0 none
internal int g_tn_why[7];   // the last pick's mines: on the home continent, unexplored, too near / far from the base, by a drop site, by an enemy building, lost there or defended (towns_care: enemy soldiers, the enemy's reach), an unsafe way or too poor for its distance
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
// SPREAD's site: the map in cells of TN_CELL tiles, each cell's explored resources on the home continent summed (a
// mine 3, a berry bush 2, a tree 1 - a forest counted to TN_CELL_WOOD trees a cell, so a mine group outweighs a wood),
// from the unit grid (the zones' site lists hold the mines only: 44 on a huge Continental; animals walk off); a spot
// is worth its cell and the 8 round it (a mine group spans cells); of the spots worth TN_CELL_MIN (a mine; a grove) the
// one nearest to any of its towns, however far (a desert with one oasis: the oasis), with the same checks as tn_site.
// No limit on zones or sites: a custom map can be any size.
#define TN_CELL 8
#define TN_CELL_WOOD 12
#define TN_CELL_MIN 3
// The player 2026-10-31: "I avoid many small spots that need defending and push a clear frontline I can defend";
// scale it with the economy; towns far off get towers and walls. A spot within TN_COMPACT of a town first (else the
// nearest anywhere); never more than TN_FRONT_STEP nearer to the enemy than its town nearest to it (the line moves
// up as a whole, the back fills first); a town only with a surplus (tn_surplus); 60+ tiles from the base a wall in
// front of its towers (tn_wall).
#define TN_COMPACT 45
#define TN_FRONT_STEP 20
#define TN_FAR_WALL 60
#define TN_HOUSES 5   // the Town Center's full defence bonus
#define TN_CELLS (128 * 128)   // a 1024-tile map; a bigger one folds its far cells onto the edge
internal struct tn_cell { int sc, n, sx, sy; uint16_t wood, mines; } g_tn_cells[TN_CELLS];
// towns_care (the player's game 2026-11-05: blue laid 6 towns in 11 minutes, 82-292 tiles out on one mine each, their
// foundations razed as they were laid, again and again - "far away bases make the AI more interesting, but it has to
// happen with sense"): a spot without enemy soldiers it knows within TN_CARE_SOLDIERS, out of the enemy's reach and
// where it lost no fight (stage.c's danger map), its way from its nearest town (or base) clear of enemy soldiers it
// knows, their towers, and 22 tiles of their buildings (not through enemy land) - part 1, the setting. Parts for
// v_towns_care only: 2 beyond TN_COMPACT of its towns only for 2 mines or more; 4 the pace (at most 2 towns going up,
// none for 5 minutes after one is lost); 8 hold under attack (nothing laid again, no builder sent while enemy soldiers
// are in sight of the town); 16 give up a town going up that they hurt (its foundations removed, its citizens home).
// Measured (1v1 huge Continental, epoch 5, 120 min, mirrored, B - A per game, se in brackets): 1 alone, 96 games:
// losses -17.9 (10.0), soldiers +5.1 (2.8), kills +13.6 (13.8), workers -0.5 (2.8); 179 towns stood against 205, 27
// lost against 31, about 103 tiles out on both sides - on. 1+4, 96 games: soldiers -3.6 (2.7), losses +9.2 (9.9),
// towns stood 167 against 214. 48 games each: 2 alone razed +12.0 (4.6), losses +26.4 (18.0), towns stood 102 against
// 143; 4 alone soldiers +9.3 (3.9), losses +21.9 (15.4); 8 (with 16, given up at the first sight of soldiers) 54
// towns given up, outcome even; 1+2+4 even, towns stood 73 against 99; 1+2+4+8 workers -8.3 (5.8). Off.
#define TN_CARE_TRIES 24
#define TN_CARE_SOLDIERS 20
#define TN_CARE_PAUSE 300000   // part 4: no new town for 5 minutes after one is lost
internal int g_tn_care = 0, g_v_tn_care = -1;
#define TN_CARE(idx) PV(idx, g_v_tn_care, g_tn_care)   // its parts (above)
internal struct { ivec2 p; int mines, key, compact; } g_tn_cand[1024];
internal int stage_danger(unsigned idx, ivec2 p);   // stage.c: in the enemy's reach or a lost fight
internal int tn_safe(ee_player *pl, unsigned idx, ivec2 base, struct tn_world *w, ivec2 p, int mines, int compact) {
    if ((TN_CARE(idx) & 2) && !compact && mines < 2) { g_tn_why[6]++; return 0; }
    if (!(TN_CARE(idx) & 1)) return 1;
    if (raiders_known(pl, p, TN_CARE_SOLDIERS) || stage_danger(idx, p)) { g_tn_why[5]++; return 0; }
    ivec2 from = base;
    int d = ivec2_dist2(p, base);
    for (int k = 0; k < w->no; k++) if (ivec2_dist2(p, w->own[k]) < d) { d = ivec2_dist2(p, w->own[k]); from = w->own[k]; }
    if (path_raided(pl, from.x, from.y, p.x, p.y) || route_covered(pl, from.x, from.y, p.x, p.y)) { g_tn_why[6]++; return 0; }
    float dx = (float)(p.x - from.x), dy = (float)(p.y - from.y), l2 = dx * dx + dy * dy;
    for (int k = 0; k < w->nf; k++) {   // the way's nearest point to each enemy building it knows
        float t = l2 > 0 ? ((float)(w->foe[k].x - from.x) * dx + (float)(w->foe[k].y - from.y) * dy) / l2 : 0;
        t = t < 0 ? 0 : t > 1 ? 1 : t;
        float ex = (float)from.x + dx * t - (float)w->foe[k].x, ey = (float)from.y + dy * t - (float)w->foe[k].y;
        if (ex * ex + ey * ey < 22.0f * 22.0f) { g_tn_why[6]++; return 0; }
    }
    return 1;
}
internal int tn_site_spread(ee_player *pl, unsigned idx, ivec2 base, unsigned hc, struct tn_world *w, ivec2 *o, int *o_n) {
    ee_map *map = game_map();
    int side = map ? ((int)map_get_size(map) + TN_CELL - 1) / TN_CELL : 0;
    if (side <= 0) return 0;
    if (side > 128) side = 128;
    memset(g_tn_cells, 0, sizeof g_tn_cells);
    memset(g_tn_why, 0, sizeof g_tn_why);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *s = it.u;
        int r = unit_get_resource(s);
        if (r < 2 || r > 6 || unit_get_amount(s) <= 0 || unit_get_hp(s) <= 0) continue;
        if (r == 2 && unit_type_get_id(it.td) != UT(RES_FORAGE_BUSH)) continue;   // berries; animals walk off, fish swim
        ivec2 sp = unit_get_tile(s);
        if (sp.x < 0 || sp.y < 0 || area_id(sp, 2) != hc) continue;
        g_tn_why[0]++;
        if (intel_looked_ago(pl, sp.x, sp.y) == 0xFFFFFFFFu) { g_tn_why[1]++; continue; }   // explored only
        int cx = sp.x / TN_CELL, cy = sp.y / TN_CELL;
        struct tn_cell *c = &g_tn_cells[(cy < side ? cy : side - 1) * side + (cx < side ? cx : side - 1)];
        if (r == 3 && c->wood >= TN_CELL_WOOD) continue;
        c->sc += r >= 4 ? 3 : r == 2 ? 2 : 1;
        c->wood += r == 3; c->mines += r >= 4;
        c->n++; c->sx += sp.x; c->sy += sp.y;
    }
    // the front: its town nearest to an enemy building it knows - a new town at most TN_FRONT_STEP nearer than that
    int front = -1;
    if (w->nf)
        for (int k = -1; k < w->no; k++) {
            ivec2 q = k < 0 ? base : w->own[k];
            int d = 0x7FFFFFFF;
            for (int f = 0; f < w->nf; f++) if (ivec2_dist2(q, w->foe[f]) < d) d = ivec2_dist2(q, w->foe[f]);
            d = (int)sqrtf((float)d);
            if (front < 0 || d < front) front = d;
        }
    int nk = 0;
    for (int i = 0; i < side * side; i++) {
        struct tn_cell *c = &g_tn_cells[i];
        if (!c->n) continue;
        int sc = 0, mines = 0, gx = i % side, gy = i / side;
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++)
                if (gx + dx >= 0 && gx + dx < side && gy + dy >= 0 && gy + dy < side) {
                    struct tn_cell *e = &g_tn_cells[(gy + dy) * side + gx + dx];
                    sc += e->sc; mines += e->mines;
                }
        if (sc < TN_CELL_MIN) continue;
        ivec2 p = iv2(c->sx / c->n, c->sy / c->n);
        int dn = ivec2_dist2(p, base);
        for (int k = 0; k < w->no; k++) if (ivec2_dist2(p, w->own[k]) < dn) dn = ivec2_dist2(p, w->own[k]);
        if (dn < 32 * 32) { g_tn_why[2]++; continue; }
        int ok = 1;
        for (int k = 0; k < w->nd && ok; k++) if (ivec2_dist2(p, w->drop[k]) < 18 * 18) ok = 0;
        if (!ok) { g_tn_why[3]++; continue; }
        for (int k = 0; k < w->nf && ok; k++) if (ivec2_dist2(p, w->foe[k]) < 22 * 22) ok = 0;
        if (!ok) { g_tn_why[4]++; continue; }
        for (int k = 0; k < g_tn[idx].nbad && ok; k++)
            if (ivec2_dist2(p, g_tn[idx].bad[k].c) < 12 * 12 && (g_tn[idx].bad[k].losses >= 3 || game_time() < g_tn[idx].bad[k].again)) ok = 0;
        if (!ok || threat_at(idx, p) > 0) { g_tn_why[5]++; continue; }
        int df = 0x7FFFFFFF;   // the nearest enemy building it knows: the back country first
        for (int k = 0; k < w->nf; k++) if (ivec2_dist2(p, w->foe[k]) < df) df = ivec2_dist2(p, w->foe[k]);
        df = df == 0x7FFFFFFF ? 0 : (int)sqrtf((float)df);
        if (front >= 0 && df < front - TN_FRONT_STEP) { g_tn_why[4]++; continue; }   // ahead of its line
        int key = (int)sqrtf((float)dn) * 4 - (sc < 24 ? sc : 24) - (df < 160 ? df : 160);   // the nearest; richer, or farther from the enemy, within a few tiles
        if (nk < (int)(sizeof g_tn_cand / sizeof *g_tn_cand)) { g_tn_cand[nk].p = p; g_tn_cand[nk].mines = mines; g_tn_cand[nk].key = key; g_tn_cand[nk].compact = dn <= TN_COMPACT * TN_COMPACT; nk++; }
    }
    // within reach of a town first, else the nearest anywhere (an oasis); with towns_care the first whose spot and way
    // are safe (tn_safe: costly, so in this order and TN_CARE_TRIES at most)
    for (int tries = 0; tries < (TN_CARE(idx) & 3 ? TN_CARE_TRIES : 1); tries++) {
        int bj = -1;
        for (int j = 0; j < nk; j++)
            if (g_tn_cand[j].key != 0x7FFFFFFF && (bj < 0 || g_tn_cand[j].compact > g_tn_cand[bj].compact ||
                                                   (g_tn_cand[j].compact == g_tn_cand[bj].compact && g_tn_cand[j].key < g_tn_cand[bj].key))) bj = j;
        if (bj < 0) return 0;
        g_tn_cand[bj].key = 0x7FFFFFFF;
        if ((TN_CARE(idx) & 3) && !tn_safe(pl, idx, base, w, g_tn_cand[bj].p, g_tn_cand[bj].mines, g_tn_cand[bj].compact)) continue;
        *o = g_tn_cand[bj].p; *o_n = g_tn_cand[bj].mines;
        return 1;
    }
    return 0;
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
    if (kind == TK_HOUSE) return ut_has(UT(B_HOUSE)) && ld_can_build(pl, UT(B_HOUSE)) ? UT(B_HOUSE) : 0;
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
internal struct tn_crew { ee_unit *u[TN_CREW]; int id[TN_CREW], n; uint32_t t; } g_tn_crew[16];   // TAKE MINES: the citizens behind the push
internal int tn_has_cit(unsigned idx, ee_unit *u) {
    for (int j = 0; j < g_tn_crew[idx].n; j++) if (g_tn_crew[idx].u[j] == u) return 1;
    for (int k = 0; k < TN_MAX; k++) {
        struct tn_town *t = &g_tn[idx].town[k];
        if (t->state) for (int j = 0; j < t->ncit; j++) if (t->cit[j] == u) return 1;
    }
    return 0;
}
internal void tn_take_cits(ee_player *pl, unsigned idx, struct tn_town *t, int n, unsigned hc);
internal int stage_army(unsigned idx, ivec2 *at, ivec2 *front);   // stage.c: its soldiers at the staging point
internal int stage_danger(unsigned idx, ivec2 p);                  // stage.c: in the enemy's reach or a lost fight
// its biggest group of land soldiers out in the field (an 8-tile cell 32+ tiles from its base), pulled back towards
// its base out of the enemy's reach: the spot, the group's middle as the front; the group's size (0: none)
internal int tn_army_spot(ee_player *pl, unsigned idx, ivec2 base, unsigned hc, ivec2 *o, ivec2 *front) {
    static struct { ivec2 c; int n, sx, sy; } cell[64];
    int nc = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !land_soldier(unit_type_get_id(it.td))) continue;
        ivec2 p = unit_get_tile(u);
        if (ivec2_dist2(p, base) < 32 * 32 || area_id(p, 2) != hc) continue;
        ivec2 c = iv2(p.x / 8, p.y / 8);
        int k;
        for (k = 0; k < nc && (cell[k].c.x != c.x || cell[k].c.y != c.y); k++) ;
        if (k == nc) { if (nc == 64) continue; cell[nc].c = c; cell[nc].n = cell[nc].sx = cell[nc].sy = 0; nc++; }
        cell[k].n++; cell[k].sx += p.x; cell[k].sy += p.y;
    }
    int best = -1;
    for (int k = 0; k < nc; k++) if (best < 0 || cell[k].n > cell[best].n) best = k;
    if (best < 0 || cell[best].n < 8) return 0;
    ivec2 m = iv2(cell[best].sx / cell[best].n, cell[best].sy / cell[best].n);
    *front = m;
    float dx = (float)(base.x - m.x), dy = (float)(base.y - m.y), len = sqrtf(dx * dx + dy * dy);
    for (float d = 0; d <= len - 32.0f; d += 1.0f) {   // back towards home to the first tile out of the enemy's reach
        ivec2 q = iv2(m.x + (int)lroundf(dx * d / len), m.y + (int)lroundf(dy * d / len));
        if (area_id(q, 2) != hc || area_id(q, 1) || stage_danger(idx, q)) continue;
        *o = q;
        return cell[best].n;
    }
    return 0;
}
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
// a forward base's wall: once its two front towers stand, 11 tiles straight across the enemy's side, 3 tiles in front
// of them (defence.c's front wall)
internal int tn_far(ee_player *pl, unsigned idx, struct tn_town *t) {   // SPREAD: a resource town 60+ tiles from the base
    ee_civ_mgr *cm = player_get_civ_mgr(pl); ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    return (TOWNS(idx) & 16) && bm && build_mgr_get_base_x(bm) != -10000 && ivec2_dist2(t->c, build_mgr_get_base(bm)) >= TN_FAR_WALL * TN_FAR_WALL;
}
internal void tn_wall(ee_player *pl, unsigned idx, struct tn_town *t, uint32_t now) {
    if (!(t->forward || tn_far(pl, idx, t)) || t->wall_tries >= 3 || (t->wall_t && now - t->wall_t < 180000)) return;
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
internal ivec2 forward_front;   // tn_plan: the front the forward base faces
internal struct tn_town *tn_found(ee_player *pl, unsigned idx, struct tn_st *s, ivec2 c, int nmines, struct tn_world *w, unsigned hc, uint32_t now, int forward) {
    struct tn_town *t = NULL;
    for (int k = 0; k < TN_MAX && !t; k++) if (!s->town[k].state) t = &s->town[k];
    if (!t) return NULL;
    memset(t, 0, sizeof *t);
    t->c = c; t->state = 1; t->t = now ? now : 1;
    t->toward = c;   // the side to face: the nearest enemy building it knows, else none (round it)
    int bd = 0x7FFFFFFF;
    for (int k = 0; k < w->nf; k++) { int d = ivec2_dist2(c, w->foe[k]); if (d < bd) { bd = d; t->toward = w->foe[k]; } }
    t->forward = forward;
    if (forward) {   // towers first (the cover), the Hospital, production, the flanks
        t->toward = forward_front;
        t->proj[t->nproj++].kind = TK_TOWER; t->proj[t->nproj++].kind = TK_TOWER;
        if (ut_has(UT(B_HOSPITAL))) { t->proj[t->nproj].kind = TK_PROD; t->proj[t->nproj].want = UT(B_HOSPITAL); t->nproj++; }
        t->proj[t->nproj].kind = TK_PROD; t->proj[t->nproj].want = tn_prod_type(pl, idx); t->nproj++;
        t->proj[t->nproj++].kind = TK_TOWER; t->proj[t->nproj++].kind = TK_TOWER;
    } else {
        t->proj[t->nproj++].kind = TK_DROP;
        // SPREAD (the player: "I usually build ~2 towers per resource patch; houses in a Town Center's area raise its
        // defence, up to 5 for the full bonus"): 2 towers and 1 more for every 4 mines of its group, and 5 houses
        int towers = (TOWNS(idx) & 16) ? 2 + nmines / 4 : g_tn_towers;
        if (towers < g_tn_towers) towers = g_tn_towers;
        if (TN_FORT(idx) && towers < 4) towers = 4;
        for (int k = 0; k < towers && t->nproj < TN_PROJ - 5; k++) t->proj[t->nproj++].kind = TK_TOWER;
        for (int k = 0; (TOWNS(idx) & 16) && k < TN_HOUSES && t->nproj < TN_PROJ; k++) t->proj[t->nproj++].kind = TK_HOUSE;
    }
    tn_take_cits(pl, idx, t, forward ? g_tn_cits + 2 : g_tn_cits, hc);
    s->founded++;
    tn_say(idx, now, "a %s at %d,%d (%d mines within 10 tiles, %d tiles from its base), %d citizens go", forward ? "forward base" : "resource town", c.x, c.y, nmines,
           (int)sqrtf((float)ivec2_dist2(c, build_mgr_get_base(civ_mgr_get_build_mgr(player_get_civ_mgr(pl))))), t->ncit);
    return t;
}
// TAKE MINES: the mines it has seen within r of c
internal int tn_mine_near(ee_player *pl, ivec2 c, int r) {
    int n = 0;
    ee_zone **zb = player_get_zones_begin(pl); ee_zone **ze = player_get_zones_end(pl);
    for (int z = 0; zb && zb + z < ze && z < 256; z++) {
        ee_zone *zone = zb[z];
        void **sb = zone ? zone_get_sites_begin(zone) : NULL, **se = zone ? zone_get_sites_end(zone) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096; k++) {
            ee_unit *m = sb[k];
            int res = m ? unit_get_resource(m) : 0;
            if (res < 4 || res > 6 || unit_get_amount(m) <= 0) continue;
            ivec2 sp = unit_get_tile(m);
            n += ivec2_dist2(sp, c) <= r * r && intel_looked_ago(pl, sp.x, sp.y) != 0xFFFFFFFFu;
        }
    }
    return n;
}
internal int tn_mine_drop(ee_player *pl, ee_unit *u, ee_unit_type *td) { return tn_is_drop(unit_type_get_id(td)) && tn_mine_near(pl, unit_get_tile(u), 10) > 0; }
internal void tn_crew_alive(ee_player *pl, unsigned idx);
// TAKE MINES: a town where the force has just razed what stood at c; 1: founded
internal int tn_claim(ee_player *pl, unsigned idx, ivec2 c, uint32_t now) {
    struct tn_st *s = &g_tn[idx];
    ee_build_mgr *bm = civ_mgr_get_build_mgr(player_get_civ_mgr(pl));
    if (!(TN_TAKE(idx) || TN_RAID(idx)) || !bm || build_mgr_get_base_x(bm) == -10000 || base_threatened(idx) || current_epoch(pl) < FARM_EPOCH || workforce(pl) < 20) return 0;
    unsigned hc = (unsigned)area_id(build_mgr_get_base(bm), 2);
    if (!hc || (unsigned)area_id(c, 2) != hc) return 0;
    for (int k = 0; k < TN_MAX; k++) if (s->town[k].state && ((s->town[k].claim && s->town[k].state == 1) || ivec2_dist2(s->town[k].c, c) < 18 * 18)) return 0;
    int nm = tn_mine_near(pl, c, 12);
    if (!nm) { tn_say(idx, now, "no town at %d,%d: no mine within 12 tiles", c.x, c.y); return 0; }
    static struct tn_world w;
    tn_world(pl, idx, &w);
    // (no look at the enemy buildings it remembers: armies.c has just looked at those it sees within 22 tiles, and the memory still holds the one razed this moment, on this very tile - 23 of 29 claims refused)
    for (int k = 0; k < w.nd; k++) if (ivec2_dist2(c, w.drop[k]) < 18 * 18) { tn_say(idx, now, "no town at %d,%d: a drop site at %d,%d", c.x, c.y, w.drop[k].x, w.drop[k].y); return 0; }
    for (int k = 0; k < w.no; k++) if (ivec2_dist2(c, w.own[k]) < 18 * 18) { tn_say(idx, now, "no town at %d,%d: its own drop site at %d,%d", c.x, c.y, w.own[k].x, w.own[k].y); return 0; }
    for (int k = 0; k < s->nbad; k++) if (ivec2_dist2(c, s->bad[k].c) < 12 * 12 && (s->bad[k].losses >= 2 || now < s->bad[k].again)) return 0;
    struct tn_town *t = tn_found(pl, idx, s, c, nm, &w, hc, now, 0);
    if (!t) return 0;
    t->claim = 1; s->plans++;
    tn_crew_alive(pl, idx);
    if (g_tn_crew[idx].n) {   // THE CREW: they are here already
        t->ncit = 0;
        for (int k = 0; k < g_tn_crew[idx].n; k++) { t->cit[t->ncit] = g_tn_crew[idx].u[k]; t->cid[t->ncit] = g_tn_crew[idx].id[k]; t->sent_mine[t->ncit++] = 0; }
        g_tn_crew[idx].n = 0;
        if (t->ncit < 3) tn_take_cits(pl, idx, t, g_tn_cits, hc);
    }
    while (t->nproj && t->proj[t->nproj - 1].kind == TK_HOUSE) t->nproj--;
    int towers = 0;
    for (int j = 0; j < t->nproj; j++) towers += t->proj[j].kind == TK_TOWER;
    for (; towers < 4 && t->nproj < TN_PROJ; towers++) t->proj[t->nproj++].kind = TK_TOWER;
    tn_say(idx, now, "the mines at %d,%d taken (%d within 12 tiles) - its own town goes up, %d citizens", c.x, c.y, nm, t->ncit);
    return 1;
}
// MINE RAIDS: the town at c - 1 secured (its drop site and every tower it still means to have standing, two at least), 0 not yet, -1 gone
internal int tn_claim_secured(unsigned idx, ivec2 c) {
    for (int k = 0; k < TN_MAX; k++) {
        struct tn_town *t = &g_tn[idx].town[k];
        if (!t->state || !t->claim || ivec2_dist2(t->c, c) > 18 * 18) continue;
        int up = 0, drop = 0, wait = 0;
        for (int j = 0; j < t->nproj; j++) {
            struct tn_proj *p = &t->proj[j];
            if (p->kind == TK_DROP) drop += p->state == 2;
            if (p->kind == TK_TOWER) { up += p->state == 2; wait += p->state < 2; }
        }
        return drop && up >= 2 && !wait;
    }
    return -1;
}
// TAKE MINES: the town at c still needs the force (its drop site or two towers not standing)
internal int tn_claim_going(unsigned idx, ivec2 c) {
    for (int k = 0; k < TN_MAX; k++) {
        struct tn_town *t = &g_tn[idx].town[k];
        if (!t->state || !t->claim || ivec2_dist2(t->c, c) > 18 * 18) continue;
        int up = 0, drop = 0;
        for (int j = 0; j < t->nproj; j++) { up += t->proj[j].kind == TK_TOWER && t->proj[j].state == 2; drop += t->proj[j].kind == TK_DROP && t->proj[j].state == 2; }
        return !drop || up < 2;
    }
    return 0;
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
internal int ar_take(ee_player *pl, ee_unit *u);   // armies.c
internal int ar_det_has(unsigned idx, ee_unit *u);   // armies.c: a soldier of its detachment
internal int tn_garrison(unsigned idx, ee_unit *u) {
    if (!u) return 0;
    if (TN_RAID(idx) && ar_det_has(idx, u)) return 1;
    if (!TN_FORT(idx)) return 0;
    for (int k = 0; k < TN_MAX; k++)
        for (int j = 0; g_tn[idx].town[k].state && j < g_tn[idx].town[k].ngar; j++) if (g_tn[idx].town[k].gar[j] == u) return 1;
    return 0;
}
// the garrison back to the game (the force takes it up as any free soldier)
internal void tn_crew_alive(ee_player *pl, unsigned idx) {
    struct tn_crew *c = &g_tn_crew[idx];
    int m = 0;
    for (int k = 0; k < c->n; k++)
        if (c->u[k] && unit_get_hp(c->u[k]) > 0 && unit_get_id(c->u[k]) == c->id[k] && unit_get_owner(c->u[k]) == pl) { c->u[m] = c->u[k]; c->id[m++] = c->id[k]; }
    c->n = m;
}
// a push at a drop site by mines sets out: its citizens, the free ones nearest at
internal void tn_crew_start(ee_player *pl, unsigned idx, ivec2 at, uint32_t now) {
    struct tn_crew *c = &g_tn_crew[idx];
    ee_build_mgr *bm = civ_mgr_get_build_mgr(player_get_civ_mgr(pl));
    if (!(TN_TAKE(idx) || TN_RAID(idx)) || c->n || !bm || build_mgr_get_base_x(bm) == -10000 || base_threatened(idx) || current_epoch(pl) < FARM_EPOCH || workforce(pl) < 20) return;
    unsigned hc = (unsigned)area_id(build_mgr_get_base(bm), 2);
    if (!hc) return;
    static struct tn_town tmp;
    memset(&tmp, 0, sizeof tmp);
    tmp.c = at;
    tn_take_cits(pl, idx, &tmp, TN_CREW, hc);
    for (int k = 0; k < tmp.ncit && k < TN_CREW; k++) { c->u[k] = tmp.cit[k]; c->id[k] = tmp.cid[k]; c->n = k + 1; }
    c->t = 0;
    tn_say(idx, now, "%d citizens go with the push", c->n);
}
// they walk TN_CREW_BEHIND tiles behind the force's middle, on the side of home
internal void tn_crew_follow(ee_player *pl, unsigned idx, ivec2 mid, ivec2 home, uint32_t now) {
    struct tn_crew *c = &g_tn_crew[idx];
    tn_crew_alive(pl, idx);
    if (!c->n || (c->t && now - c->t < 8000)) return;
    c->t = now ? now : 1;
    float dx = (float)(home.x - mid.x), dy = (float)(home.y - mid.y), l = sqrtf(dx * dx + dy * dy);
    ivec2 p = l > 1 ? iv2(mid.x + (int)lroundf(dx / l * TN_CREW_BEHIND), mid.y + (int)lroundf(dy / l * TN_CREW_BEHIND)) : mid;
    for (int k = 0; k < c->n; k++)
        if (ivec2_dist2(unit_get_tile(c->u[k]), p) > 5 * 5 && !goal_near(c->u[k], (float)p.x + 0.5f, (float)p.y + 0.5f, 3.0f))
            unit_goal(c->u[k], goal_move((float)p.x + 0.5f, (float)p.y + 0.5f), 0, 0x1F40);
}
// the force goes home: its citizens too, back to the game's economy
internal void tn_crew_release(ee_player *pl, unsigned idx) {
    struct tn_crew *c = &g_tn_crew[idx];
    tn_crew_alive(pl, idx);
    ee_build_mgr *bm = civ_mgr_get_build_mgr(player_get_civ_mgr(pl));
    for (int k = 0; bm && k < c->n; k++) {
        ivec2 h = build_mgr_get_base(bm);
        unit_goal(c->u[k], goal_move((float)h.x + 0.5f, (float)h.y + 0.5f), 0, 0x1F40);
    }
    c->n = 0;
}
internal void tn_gar_release(ee_player *pl, struct tn_town *t) {
    ee_combat_mgr *cm = player_get_combat_mgr(pl);
    for (int k = 0; k < t->ngar; k++) {
        ee_unit *u = t->gar[k];
        if (!u || unit_get_hp(u) <= 0 || unit_get_id(u) != t->gid[k] || unit_get_owner(u) != pl) continue;
        unit_set_ai_held(u, 0);
        if (cm) combat_mgr_register_unit(cm, u);
    }
    t->ngar = 0;
}
internal void tn_garrison_run(ee_player *pl, unsigned idx, struct tn_st *s, struct tn_town *t, unsigned hc, int drop_ok, uint32_t now) {
    int m = 0;
    for (int k = 0; k < t->ngar; k++) {
        ee_unit *u = t->gar[k];
        if (u && unit_get_hp(u) > 0 && unit_get_id(u) == t->gid[k] && unit_get_owner(u) == pl) { t->gar[m] = u; t->gid[m++] = t->gid[k]; }
    }
    t->ngar = m;
    int foe = raiders_near(pl, t->c, 15);
    if (foe) t->foe_t = now ? now : 1;
    if (!s->open) { tn_gar_release(pl, t); return; }
    if (!drop_ok) return;
    int want = t->ncit / 3 > 3 ? t->ncit / 3 : 3;
    if (t->foe_t && now - t->foe_t < 300000 && want < 8) want = 8;
    if (want > TN_GARRISON) want = TN_GARRISON;
    if (t->ngar < want && (!t->gar_t || now - t->gar_t >= 20000) && !base_threatened(idx)) {
        t->gar_t = now ? now : 1;
        float mine, theirs;
        ee_combat_mgr *am = player_get_combat_mgr(pl);
        struct { ee_unit *u; int d; } cand[64];
        int nc = 0, all = 0, kept = 0;
        for (int k = 0; k < TN_MAX; k++) if (s->town[k].state) kept += s->town[k].ngar;
        for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
            ee_unit *u = it.u;
            int ty = unit_type_get_id(it.td);
            if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || unit_get_frozen(u) || !land_soldier(ty) || !is_soldier(it.td) || is_ship_or_air(it.td) ||
                is_transport(ty) || is_siege(ty)) continue;
            all++;
            if ((unsigned)area_id(unit_get_tile(u), 2) != hc || unit_fighting(u) || landing_has_unit(pl, u) || tn_garrison(idx, u)) continue;
            int d = ivec2_dist2(unit_get_tile(u), t->c);
            if (nc < 64) { cand[nc].u = u; cand[nc++].d = d; }
            else { int wj = 0; for (int j = 1; j < 64; j++) if (cand[j].d > cand[wj].d) wj = j; if (d < cand[wj].d) { cand[wj].u = u; cand[wj].d = d; } }
        }
        int room = all / 4 - kept, take = want - t->ngar > 4 ? 4 : want - t->ngar;
        if (take > room) take = room;
        if (take > 0 && !ex_outmatched(pl, &mine, &theirs)) {
            while (take > 0 && nc > 0) {
                int bj = 0;
                for (int j = 1; j < nc; j++) if (cand[j].d < cand[bj].d) bj = j;
                ee_unit *u = cand[bj].u;
                cand[bj] = cand[--nc];
                if (unit_get_ai_held(u) ? !ar_take(pl, u) : !am || !combat_mgr_unit_free(am, unit_get_physics(u))) continue;
                if (!unit_get_ai_held(u)) player_hold_unit(pl, u);
                t->gar[t->ngar] = u; t->gid[t->ngar++] = unit_get_id(u);
                s->guards++; take--;
            }
        }
    }
    if (foe) return;   // they fight what comes; walked back once it's over
    g_sg_order = 1;
    for (int k = 0; k < t->ngar; k++) {
        ee_unit *u = t->gar[k];
        if (ivec2_dist2(unit_get_tile(u), t->c) > 8 * 8 && !unit_get_goal(u)) s->orders += unit_goal(u, goal_move((float)t->c.x + 0.5f, (float)t->c.y + 0.5f), 0, 0x1F40);
    }
    g_sg_order = 0;
}
// FORT: its square, round the mine nearest the drop site; the side towards the drop site stays open for the citizens
internal void tn_fort_set(ee_player *pl, unsigned idx, struct tn_town *t, unsigned hc, uint32_t now) {
    if (t->fort_set) return;
    ivec2 mid = t->proj[0].p, c = mid;
    int bd = 0x7FFFFFFF, half = 0;
    ee_zone **zb = player_get_zones_begin(pl); ee_zone **ze = player_get_zones_end(pl);
    for (int z = 0; zb && zb + z < ze && z < 256; z++) {
        ee_zone *zone = zb[z];
        void **sb = zone ? zone_get_sites_begin(zone) : NULL, **se = zone ? zone_get_sites_end(zone) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096; k++) {
            ee_unit *m = sb[k];
            int r = m ? unit_get_resource(m) : 0;
            if (r < 4 || r > 6 || unit_get_amount(m) <= 0) continue;
            ivec2 sp = unit_get_tile(m);
            int d = ivec2_dist2(sp, mid);
            if (d >= bd || (unsigned)area_id(sp, 2) != hc) continue;
            ee_unit_type *td = unit_get_type_data(m);
            int sz = td ? unit_type_get_size(td) : 0;
            bd = d; c = sp; half = sz > 1 && sz < 8 ? sz / 2 : 0;
        }
    }
    c = iv2(c.x + half, c.y + half);
    memset(&t->fort, 0, sizeof t->fort);
    t->fort.x = c.x; t->fort.y = c.y; t->fort.t = now ? now : 1;
    int gx = mid.x - c.x, gy = mid.y - c.y;
    t->fort.open = abs(gx) >= abs(gy) ? (gx < 0 ? 0 : 1) : (gy < 0 ? 2 : 3);
    t->fort_set = 1;
    tn_say(idx, now, "a fort round the mine at %d,%d, open on side %d (its drop site at %d,%d)", c.x, c.y, t->fort.open, mid.x, mid.y);
}
// FORT: the walls, once two corners stand and no enemy is near
internal void tn_fort_walls(ee_player *pl, unsigned idx, struct tn_town *t, uint32_t now) {
    if (!t->fort_set || (t->fwall_t && now - t->fwall_t < 20000) || raiders_near(pl, t->c, 12)) return;
    int up = 0;
    for (int j = 0; j < t->nproj; j++) up += t->proj[j].kind == TK_TOWER && t->proj[j].state == 2;
    void *wm = civ_mgr_get_wall_mgr(player_get_civ_mgr(pl));
    if (up >= 2 && wm) { t->fwall_t = now ? now : 1; sf_walls(pl, idx, &t->fort, wm, player_stock_at(pl), now); }
}
internal void tn_lost(unsigned idx, struct tn_st *s, struct tn_town *t, uint32_t now) {
    ee_unit *any_gar = t->ngar ? t->gar[0] : NULL;
    if (any_gar && unit_get_owner(any_gar)) tn_gar_release(unit_get_owner(any_gar), t);
    int k;
    for (k = 0; k < s->nbad && ivec2_dist2(s->bad[k].c, t->c) >= 12 * 12; k++) ;
    if (k == s->nbad && s->nbad < 16) { s->bad[k].c = t->c; s->bad[k].losses = 0; s->nbad++; }
    if (k < 16) { s->bad[k].losses++; s->bad[k].again = now + 480000; }
    s->lost++;
    s->lost_t = now ? now : 1;
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
                tn_say(idx, now, "%s %d at %d,%d destroyed%s", k_tk[p->kind], p->type, p->p.x, p->p.y, p->tries < TN_TRIES(idx, t, p->kind) ? " - laid again" : "");
                p->b = NULL; p->state = p->tries < TN_TRIES(idx, t, p->kind) ? 0 : 3;
                t->hurt_t = now ? now : 1;
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
    if (t->forward) drop_ok = 1;   // none: its buildings go up at once
    for (int j = 0; j < t->ncit; j++)   // its citizens alive
        if (!t->cit[j] || unit_get_hp(t->cit[j]) <= 0 || unit_get_id(t->cit[j]) != t->cid[j]) {
            t->cit[j] = t->cit[t->ncit - 1]; t->cid[j] = t->cid[t->ncit - 1]; t->sent_mine[j] = t->sent_mine[t->ncit - 1]; t->ncit--; j--;
            t->hurt_t = now ? now : 1;
        }
    if (!any && !t->ncit && now - t->t > 60000) { tn_lost(idx, s, t, now); return; }
    // towns_remove_unbuilt: going up with all its citizens dead - nobody plans or builds it any more, and its
    // foundations (1 hp: damaged) drew the game's repair round's citizens to them (attack.c ONE AT A TIME); removed
    // as econ.c removes stacked ones (vf24 (0, 0, 0): refunded) and the town lost
    if (g_tn_remove && t->state == 1 && any && !t->ncit && now - t->t > 60000) {
        tn_say(idx, now, "the town at %d,%d: its citizens dead, %d unfinished buildings removed", t->c.x, t->c.y, tn_remove_unbuilt(t));
        tn_lost(idx, s, t, now);
        return;
    }
    // towns_care: enemy soldiers in sight at a resource town (not remembered ones: those stalled towns for good) - it
    // lays nothing again and sends no builder while they are there; before its drop site stands and hurt by them (a
    // building destroyed or a citizen killed in the last 30 s) it is given up (its foundations removed, its citizens
    // home) rather than fed to them. Given up at the first sight of them, 13 towns were dropped in 24 games where the
    // other side lost one: soldiers passing by mostly leave a town going up alone
    int hot = (TN_CARE(idx) & 8) && !t->forward && raiders_near(pl, t->c, 12);
    if (hot && (TN_CARE(idx) & 16) && t->state == 1 && !drop_ok && t->hurt_t && now - t->hurt_t < 30000) {
        s->gaveup++;
        tn_say(idx, now, "the town at %d,%d given up: enemy soldiers hurt it, %d unfinished buildings removed, %d citizens home", t->c.x, t->c.y, tn_remove_unbuilt(t), t->ncit);
        ivec2 home = build_mgr_get_base(civ_mgr_get_build_mgr(player_get_civ_mgr(pl)));
        for (int k = 0; k < t->ncit; k++) unit_goal(t->cit[k], goal_move((float)home.x + 0.5f, (float)home.y + 0.5f), 0, 0x1F40);
        t->ncit = 0;
        tn_lost(idx, s, t, now);
        return;
    }
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
    if (TN_FORTS(idx, t) && !t->forward) { tn_fort_walls(pl, idx, t, now); if (TN_FORT(idx)) tn_garrison_run(pl, idx, s, t, hc, drop_ok, now); }
    else tn_wall(pl, idx, t, now);
    if (t->state == 1 && t->forward && standing + dropped == t->nproj) t->ncit = 0;   // its citizens back to the game
    if (t->state == 1 && drop_ok && standing + dropped == t->nproj) { t->state = 2; s->held++; tn_say(idx, now, "the town at %d,%d stands (%d buildings)", t->c.x, t->c.y, standing); }
    if (hot) return;
    // the next building (every 8 s), the drop site alone first; one it can't pay waits (the drop site always)
    if (t->ncit &&(!t->plan_t || now - t->plan_t >= 8000)) {
        t->plan_t = now ? now : 1;
        int going = 0;
        for (int j = 0; j < t->nproj; j++) going += t->proj[j].state == 1;
        for (int j = 0; j < t->nproj && going < 2; j++) {
            struct tn_proj *p = &t->proj[j];
            if (p->state) continue;
            if (p->kind != TK_DROP && !drop_ok && !t->proj[0].state) break;
            int ty = tn_kind_type(pl, p->kind, p->want);
            ivec2 at = iv2(0, 0), c = t->c;
            if (!ty) { if (p->kind != TK_DROP && !(TN_FORTS(idx, t) && p->kind == TK_TOWER) && ++p->waits > 30) { p->state = 3; tn_say(idx, now, "no %s it can pay for 4 minutes - dropped", k_tk[p->kind]); } break; }
            if (p->kind == TK_PROD && !t->forward && t->proj[0].state) c = t->proj[0].p;
            if (p->kind == TK_PROD && t->forward) {   // behind the towers
                float ax = (float)(t->toward.x - t->c.x), ay = (float)(t->toward.y - t->c.y), l = sqrtf(ax * ax + ay * ay);
                if (l > 1) c = iv2(t->c.x - (int)lroundf(4 * ax / l), t->c.y - (int)lroundf(4 * ay / l));
            }
            if (p->kind == TK_TOWER) {   // 5 tiles from the drop site, towards the enemy it knows (else round it)
                ivec2 d0 = !t->forward && t->proj[0].state ? t->proj[0].p : t->c;
                float ax = (float)(t->toward.x - d0.x), ay = (float)(t->toward.y - d0.y), l = sqrtf(ax * ax + ay * ay);
                float ang = l > 1 ? atan2f(ay, ax) : 0;
                int nth = 0;
                for (int k = 0; k < j; k++) nth += t->proj[k].kind == TK_TOWER;
                ang += (nth & 1 ? 1 : -1) * (0.6f + 0.6f * (float)(nth / 2)) * (l > 1 ? 1.0f : 2.6f);
                c = iv2(d0.x + (int)lroundf(5 * cosf(ang)), d0.y + (int)lroundf(5 * sinf(ang)));
            }
            if (p->kind == TK_TOWER && TN_FORTS(idx, t) && !t->forward && t->proj[0].state) {   // FORT: the first four on its corners
                tn_fort_set(pl, idx, t, hc, now);
                int nth = 0;
                for (int k = 0; k < j; k++) nth += t->proj[k].kind == TK_TOWER;
                if (nth < 4) sf_corner(&t->fort, nth, &c.x, &c.y);
            }
            if (tier_refuses(pl, ty)) break;   // spend_tiers: an outpost's turn first
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
    for (int j = 0; j < t->nproj; j++) if (t->proj[j].state == 1) want[j] = t->proj[j].kind == TK_DROP ? (drop_ok ? 3 : 64) : t->forward ? 4 : 3;
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
        if (t->forward) {   // to the base while nothing is laid; once it stands the game's again
            if (t->state == 1 && !any && ivec2_dist2(unit_get_tile(u), t->c) > 64 && !goal_near(u, (float)t->c.x + 0.5f, (float)t->c.y + 0.5f, 4.0f)) {
                s->orders += unit_goal(u, goal_move((float)t->c.x + 0.5f, (float)t->c.y + 0.5f), 0, 0x1F40);
                s->sent++;
            }
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
// what a forward base needs in stock to start: two towers and the Hospital
internal void tn_forward_need(ee_player *pl, int need[7]) {
    int tt = ld_kind_type(pl, LDB_TOWER);
    for (int r = 2; r < 7; r++)
        need[r] = 2 * (tt ? unit_cost(pl, tt, r) : unit_cost(pl, UT(B_GUARD_TOWER_MIDDLE), r)) + (ut_has(UT(B_HOSPITAL)) ? unit_cost(pl, UT(B_HOSPITAL), r) : 0);
}
// the resources a town's drop site waits for (expand.c expand_reserve, the budget's reserve): a Town Center's for 90 s,
// then a Settlement's, until it is laid (3 minutes at most)
internal void towns_reserve(ee_player *pl, int reserve[7]) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    if (!TOWNS(idx)) return;
    uint32_t now = game_time();
    if (g_tn[idx].fsave && now - g_tn[idx].fsave < 180000) {   // a forward base saved for
        int need[7] = { 0 };
        tn_forward_need(pl, need);
        for (int r = 2; r < 7; r++) reserve[r] += need[r];
    }
    for (int k = 0; k < TN_MAX; k++) {
        struct tn_town *t = &g_tn[idx].town[k];
        if (t->state != 1 || t->forward || t->proj[0].state || t->proj[0].tries || now - t->t > 180000) continue;   // its first drop site
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
// SPREAD's guard (removed): a town with enemy soldiers within 18 tiles got the nearest free soldiers within 90 until
// those there were 1.3 x the enemy - 1v1 huge Continental, 24 games: 3275 orders, B - A losses +175 (se 36), kills -167
// (51): they walked in a few at a time and died one by one (muster's lesson at home). A town's defence needs the army
// to gather first.
// SPREAD's scout (towns bit 32, with 16): no site left and land of the home continent it has never seen - its fastest
// free soldier walks to the unseen cell nearest to its towns (a player sends a scout; the AI knew only what passed by:
// 15-30% of its continent's resources unseen at minute 120). With care (the player: "just feeding soldiers into enemy
// defences is how this all started"; the first scout - any soldier, any unseen cell - cost workers -12 and kills
// -23 over no scout): only land no nearer to the enemy than its front town (tn_front) and outside every defence it
// knows (threat_at); back home as soon as an enemy soldier is within TN_SCOUT_SIGHT or a known defence covers it
// (tn_scout_watch). One at a time, a new one after 90 s.
#define TN_SCOUT_SIGHT 12
internal int tn_front(struct tn_world *w, ivec2 base, ivec2 p, int *df) {   // the front's distance to the enemy; *df p's
    int front = -1;
    for (int k = -1; k < w->no && w->nf; k++) {
        ivec2 q = k < 0 ? base : w->own[k];
        int d = 0x7FFFFFFF;
        for (int f = 0; f < w->nf; f++) if (ivec2_dist2(q, w->foe[f]) < d) d = ivec2_dist2(q, w->foe[f]);
        d = (int)sqrtf((float)d);
        if (front < 0 || d < front) front = d;
    }
    int d = 0x7FFFFFFF;
    for (int f = 0; f < w->nf; f++) if (ivec2_dist2(p, w->foe[f]) < d) d = ivec2_dist2(p, w->foe[f]);
    *df = w->nf ? (int)sqrtf((float)d) : 0x7FFFFFFF;
    return front;
}
internal void tn_scout(ee_player *pl, unsigned idx, struct tn_st *s, ivec2 base, unsigned hc, struct tn_world *w, uint32_t now) {
    if (s->scout && unit_get_hp(s->scout) > 0 && unit_get_id(s->scout) == s->scout_id && now - s->scout_t < 90000) return;
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0, best = 0x7FFFFFFF;
    ivec2 to = base;
    for (int y = TN_CELL / 2; y < size; y += TN_CELL)
        for (int x = TN_CELL / 2; x < size; x += TN_CELL) {
            ivec2 p = iv2(x, y);
            if (intel_looked_ago(pl, x, y) != 0xFFFFFFFFu || area_id(p, 2) != hc || threat_at(idx, p) > 0) continue;
            int df, front = tn_front(w, base, p, &df);
            if (front >= 0 && df < front) continue;   // beyond its line
            int d = ivec2_dist2(p, base);
            for (int k = 0; k < w->no; k++) if (ivec2_dist2(p, w->own[k]) < d) d = ivec2_dist2(p, w->own[k]);
            if (d < best) { best = d; to = p; }
        }
    if (best == 0x7FFFFFFF) return;   // all of it seen, or only beyond its line
    ee_unit *sc = NULL;
    float fast = 0;
    for (struct grid_it it = grid_it_box(base.x, base.y, 40, 1); grid_it_next(&it); ) {   // the fastest idle one at home
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || !is_soldier(it.td) || unit_get_goal(u)) continue;
        float v = unit_max_speed(u);
        if (v > fast) { fast = v; sc = u; }
    }
    if (!sc || !unit_goal(sc, goal_move((float)to.x + 0.5f, (float)to.y + 0.5f), 0, 0x1F40)) return;
    s->scout = sc; s->scout_id = unit_get_id(sc); s->scout_t = now ? now : 1; s->scouts++;
    tn_say(idx, now, "scout to %d,%d (unseen land, %d tiles from its nearest town)", to.x, to.y, (int)sqrtf((float)best));
}
// the scout on its way: an enemy soldier in sight or a known defence over it - home
internal void tn_scout_watch(ee_player *pl, unsigned idx, struct tn_st *s, ivec2 base) {
    ee_unit *sc = s->scout;
    if (!sc || unit_get_hp(sc) <= 0 || unit_get_id(sc) != s->scout_id) { s->scout = NULL; return; }
    ivec2 at = unit_get_tile(sc);
    int seen = threat_at(idx, at) > 0;
    for (struct grid_it it = grid_it_box(at.x, at.y, TN_SCOUT_SIGHT, 1); grid_it_next(&it) && !seen; ) {
        ee_player *o = unit_get_owner(it.u);
        seen = o && o != pl && unit_get_hp(it.u) > 0 && is_soldier(it.td) && is_enemy(pl, o);
    }
    if (!seen) return;
    unit_goal(sc, goal_move((float)base.x + 0.5f, (float)base.y + 0.5f), 0, 0x1F40);
    s->scout = NULL;   // its job ends; a new one after the pause
    s->scouts_back++;
}
// SPREAD's growth (towns bit 64, with 16; the player 2026-10-31: "if the AI has spare wood and stone it should consider expanding its existing
// infrastructure" - towns got only what they were founded with): every 30 s, with wood and stone beyond what the
// budget holds back (spend_reserve) by TN_GROW_WOOD / TN_GROW_STONE, one more project at the standing town with the
// fewest buildings that has nothing going up: towers up to TN_GROW_TOWERS, then a Hospital, a Temple, a University,
// then production (army_plan's last pick, else Barracks) up to TN_GROW_PROD. Its builders: 3 of the town's citizens
// (the nearest free ones when it has fewer). Measured (1v1 huge Continental, 120 min, 24 games, towns=1 against 17 with
// it): 12 projects an AI (167 towers, 117 production and civil in all), B - A losses +52.6 (se 25.9), kills -55.6
// (30.8), workers -8.5 (6.3) - worse than 17 without it (losses -11.3, even). Off: its own bit.
#define TN_GROW_WOOD 400
#define TN_GROW_STONE 200
#define TN_GROW_TOWERS 6
#define TN_GROW_PROD 3
internal void tn_grow(ee_player *pl, unsigned idx, struct tn_st *s, unsigned hc, uint32_t now) {
    if (s->grow_t && now >= s->grow_t && now - s->grow_t < 30000) return;
    s->grow_t = now ? now : 1;
    int res[7], *stock = player_stock_at(pl);
    if (!stock) return;
    spend_reserve(pl, idx, res, NULL);
    if (stock[3] - res[3] < TN_GROW_WOOD || stock[4] - res[4] < TN_GROW_STONE) return;
    struct tn_town *best = NULL;
    int fewest = 0x7FFFFFFF;
    for (int k = 0; k < TN_MAX; k++) {
        struct tn_town *t = &s->town[k];
        if (t->state != 2 || t->forward || t->nproj >= TN_PROJ) continue;
        int busy = 0, n = 0;
        for (int j = 0; j < t->nproj; j++) { busy |= t->proj[j].state < 2; n += t->proj[j].state == 2; }
        if (!busy && n < fewest) { fewest = n; best = t; }
    }
    if (!best) return;
    int towers = 0, prods = 0, has[3] = { 0 };
    static const enum ut_name civil[3] = { UT_B_HOSPITAL, UT_B_TEMPLE, UT_B_UNIVERSITY };
    for (int j = 0; j < best->nproj; j++) {
        struct tn_proj *p = &best->proj[j];
        if (p->state == 3) continue;
        towers += p->kind == TK_TOWER;
        if (p->kind != TK_PROD) continue;
        int civ = 0;
        for (int c = 0; c < 3; c++) if (p->want == g_ut[civil[c]]) { has[c] = 1; civ = 1; }
        prods += !civ;
    }
    int kind = -1, want = 0;
    if (towers < TN_GROW_TOWERS) kind = TK_TOWER;
    for (int c = 0; c < 3 && kind < 0; c++)
        if (!has[c] && ut_has(g_ut[civil[c]]) && ld_can_build(pl, g_ut[civil[c]])) { kind = TK_PROD; want = g_ut[civil[c]]; }
    if (kind < 0 && prods < TN_GROW_PROD) { kind = TK_PROD; want = tn_prod_type(pl, idx); }
    if (kind < 0) return;
    struct tn_proj *p = &best->proj[best->nproj++];
    memset(p, 0, sizeof *p);
    p->kind = kind; p->want = want;
    if (best->ncit < 3) tn_take_cits(pl, idx, best, 3, hc);
    s->grown++;
    tn_say(idx, now, "grows the town at %d,%d: a %s%s%d (wood %d, stone %d beyond the reserve)", best->c.x, best->c.y, k_tk[kind],
           want ? " of type " : "", want, stock[3] - res[3], stock[4] - res[4]);
}
internal int idle_citizens(ee_player *pl);   // econ.c: unassigned ones plus those beyond their resource's target
// SPREAD's pace: citizens to spare (a town's towns_citizens idle or beyond their resource's target) and twice a Town
// Center's price in stock
internal int tn_surplus(ee_player *pl) {
    if (idle_citizens(pl) < g_tn_cits) return 0;
    int *stock = player_stock_at(pl), tc = UT(B_TOWN_CENTER);
    for (int r = 2; r < 7 && stock; r++) if (stock[r] < 2 * unit_cost(pl, tc, r)) return 0;
    return stock != NULL;
}
// FORT: its continent isn't fully claimed - an enemy building it knows on it, or an explored mine no drop site is near
internal int tn_open(ee_player *pl, unsigned hc, struct tn_world *w) {
    for (int k = 0; k < w->nf; k++) if ((unsigned)area_id(w->foe[k], 2) == hc) return 1;
    ee_zone **zb = player_get_zones_begin(pl); ee_zone **ze = player_get_zones_end(pl);
    for (int z = 0; zb && zb + z < ze && z < 256; z++) {
        ee_zone *zone = zb[z];
        void **sb = zone ? zone_get_sites_begin(zone) : NULL, **se = zone ? zone_get_sites_end(zone) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096; k++) {
            ee_unit *m = sb[k];
            int r = m ? unit_get_resource(m) : 0, claimed = 0;
            if (r < 4 || r > 6 || unit_get_amount(m) <= 0) continue;
            ivec2 sp = unit_get_tile(m);
            if ((unsigned)area_id(sp, 2) != hc || intel_looked_ago(pl, sp.x, sp.y) == 0xFFFFFFFFu) continue;
            for (int j = 0; j < w->nd && !claimed; j++) claimed = ivec2_dist2(sp, w->drop[j]) < 18 * 18;
            for (int j = 0; j < w->no && !claimed; j++) claimed = ivec2_dist2(sp, w->own[j]) < 18 * 18;
            if (!claimed) return 1;
        }
    }
    return 0;
}
internal void tn_plan(ee_player *pl, unsigned idx, struct tn_st *s, unsigned hc, ivec2 base, uint32_t now) {
    float mine = 0, theirs = 0;
    int n = 0, going = 0, nf = 0, fgoing = 0;
    for (int k = 0; k < TN_MAX; k++) {
        struct tn_town *t = &s->town[k];
        if (t->forward) { nf += t->state != 0; fgoing += t->state == 1; }
        else { n += t->state != 0; going += t->state == 1; }
    }
    const char *why = NULL;
    if (current_epoch(pl) < FARM_EPOCH) why = "before the Copper Age";
    else if (workforce(pl) < 20) why = "fewer than 20 workers";
    else if (base_threatened(idx)) why = "enemy soldiers in its base";
    if (!why) {
        static struct tn_world w;
        tn_world(pl, idx, &w);
        if (TN_FORT(idx)) s->open = tn_open(pl, hc, &w);
        ivec2 c, front;
        int nm = 0, done = 0;
        ex_outmatched(pl, &mine, &theirs);
        if (TOWNS(idx) & 8) {   // a forward base where the army gathers, under its cover
            int rich = 1, *st = player_stock_at(pl);
            rich = ld_kind_type(pl, LDB_TOWER) != 0;   // a tower it can pay now: the rest as the money comes (the game spends it as it comes)
            int army = stage_army(idx, &c, &front), clear = 1;
            if (army < 8) army = tn_army_spot(pl, idx, base, hc, &c, &front);
            if (army) {
                if (ivec2_dist2(c, base) < 32 * 32) clear = 0;
                for (int k = 0; k < w.no && clear; k++) if (ivec2_dist2(c, w.own[k]) < 32 * 32) clear = 0;
                for (int k = 0; k < TN_MAX && clear; k++) if (s->town[k].state && s->town[k].forward && ivec2_dist2(c, s->town[k].c) < 32 * 32) clear = 0;
            }
            if (fgoing) why = "a forward base going up";
            else if (nf >= 2) why = "holds 2 forward bases";
            else if (army < 8) why = "no group of 8 soldiers out in the field";
            else if (!clear) why = "its army within 32 tiles of its base or a town";
            else if (mine < 0.8f * theirs) why = "its army under 0.8 x the enemy's";
            else if (!st || !rich) {
                why = "saving for a forward base";
                if (!s->fsave || now - s->fsave > 240000) s->fsave = now ? now : 1;   // 3 minutes of saving, then a minute's pause
            }
            else { forward_front = front; s->plans++; s->fsave = 0; tn_found(pl, idx, s, c, 0, &w, hc, now, 1); done = 1; why = NULL; }
            if (why && strcmp(why, "saving for a forward base")) s->fsave = 0;
        }
        if (!done && (TOWNS(idx) & 1)) {
            why = NULL;
            int most = (TOWNS(idx) & 16) && workforce(pl) >= 2 * TN_SPREAD_BUILDING ? workforce(pl) / TN_SPREAD_BUILDING : 1;
            if ((TN_CARE(idx) & 4) && most > 2) most = 2;
            if (going >= most) why = "a town going up";
            else if ((TN_CARE(idx) & 4) && s->lost_t && now - s->lost_t < TN_CARE_PAUSE) why = "a town lost in the last 5 minutes";
            else if (!(TOWNS(idx) & 16) && n >= g_tn_max) why = "holds towns_max towns";
            else if ((TOWNS(idx) & 16) && workforce(pl) < TN_SPREAD_WORKERS * (n + 2)) why = "too few workers for another town";   // its base and the new one count
            else if ((TOWNS(idx) & 16) && !tn_surplus(pl)) why = "no surplus for a town";
            else if (mine < theirs) why = "its army outmatched";
        }
        if (!done && (TOWNS(idx) & 1) && !why) {
            int fs = tn_free_slots(pl, hc, &w);
            if (fs >= 4 && !(TOWNS(idx) & 16)) { if (!why) why = "free places at its mines"; }   // SPREAD: not waiting for them
            else if (!((TOWNS(idx) & 16) ? tn_site_spread : tn_site)(pl, idx, base, hc, &w, &c, &nm)) {
                why = "nothing free to settle";
                if ((TOWNS(idx) & 48) == 48) tn_scout(pl, idx, s, base, hc, &w, now);
                if (g_autotest) tn_say(idx, now, "%d resources on the home continent, %d unexplored; places turned down: %d too near or far, %d by a drop site, %d by an enemy building, %d lost there or defended, %d unsafe way or too poor",
                                       g_tn_why[0], g_tn_why[1], g_tn_why[2], g_tn_why[3], g_tn_why[4], g_tn_why[5], g_tn_why[6]);
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
    if (s->scout) tn_scout_watch(pl, idx, s, base);
    if ((TOWNS(idx) & 80) == 80) tn_grow(pl, idx, s, hc, now);
    if (!s->plan_t || now - s->plan_t >= 30000) { s->plan_t = now ? now : 1; tn_plan(pl, idx, s, hc, base, now); }
    if (g_autotest && g_log && (!s->log_t || now / 300000 != s->log_t / 300000) && s->plans) {
        s->log_t = now ? now : 1;
        int n = 0, held = 0;
        for (int k = 0; k < TN_MAX; k++) { n += s->town[k].state != 0; held += s->town[k].state == 2; }
        tn_say(idx, now, "status - %d towns (%d standing) | planned %u founded %u stood %u lost %u unpaid %u | laid %u built %u laid again %u orders %u assigned %u sent %u | production %u walls %u | guards sent %u scouts %u (%u turned back) | grown %u | given up %u",
               n, held, s->plans, s->founded, s->held, s->lost, s->unpaid, s->laid, s->built, s->rebuilt, s->orders, s->assigned, s->sent, s->prods, s->walls, s->guards, s->scouts, s->scouts_back, s->grown, s->gaveup);
    }
}
internal void towns_new_game(void) { memset(g_tn, 0, sizeof g_tn); memset(g_tn_crew, 0, sizeof g_tn_crew); }
// the towns key's fields (resource_towns, settlement, forward_production, forward_base, spread, scout, grow - its
// struct's order) as the bits the code tests: 1, 2, 4, 8, 16, 32, 64
internal void towns_install(const char *autotest_ini) {
    g_towns = SETTING_BITS(g_set_ai[1].towns);
    g_tn_max = g_set_ai[1].towns_max;
    g_tn_cits = g_set_ai[1].towns_citizens;
    g_tn_towers = g_set_ai[1].towers_per_town;
    g_tn_remove = g_set_ai[1].towns_remove_unbuilt;
    g_tn_care = g_set_ai[1].towns_care ? 1 : 0;   // the safe spot and way: the other parts measured no gain (TN_CARE)
    if (g_tn_max > TN_MAX) g_tn_max = TN_MAX;
    if (g_tn_cits > 12) g_tn_cits = 12;
    if (g_tn_towers > TN_PROJ - 1) g_tn_towers = TN_PROJ - 1;
    if (autotest_ini) {   // v_towns=resource_towns|spread (the same names; a number: the bits)
        char v[128];
        GetPrivateProfileStringA("autotest", "v_towns", "", v, sizeof v, autotest_ini);
        int f[8] = { 0 };
        char why[96];
        if (!v[0]) g_v_towns = -1;
        else if (v[0] >= '0' && v[0] <= '9') g_v_towns = atoi(v);
        else if (setting_flags(&k_settings[SETTING_towns], v, f, why, sizeof why)) { g_v_towns = -1; if (g_log) fprintf(g_log, "# towns: v_towns=%s: %s\n", v, why); }
        else g_v_towns = setting_bits(f, 7);
        g_v_tn_care = ini_int("autotest", "v_towns_care", -1, autotest_ini);
    }
    if (g_log) fprintf(g_log, "# towns: towns %d (variant %d), at most %d, %d citizens, %d towers each, care %d (variant %d)\n", g_towns, g_v_towns, g_tn_max, g_tn_cits, g_tn_towers, g_tn_care, g_v_tn_care);
}
