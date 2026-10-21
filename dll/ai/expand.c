// EXPANSION TO OTHER ISLANDS (docs/economy.md "Other islands"). Included by eeaimod.c after mass.c (landing.c's
// transports and unit goals, fleet.c's fleet, navy.c's map, mines.c's mine scan).
// The playtester: "work every resource node on the home island fast, then expand to other islands - early it must
// prioritise, later everything it can get hold of"; "a lone mining camp with a few citizens on an empty island won't
// hold - a few towers and even barracks for reinforcement are needed".
//
// THE ISLANDS LOG (autotest, both sides - the vanilla AI too): "# isl: P<n> at <m> min:" every 5 game minutes, the
// land continents (area_id kind 2) by what they are to the player: home (its base point's), own (its buildings, no
// enemy's), contest (its and an enemy's), enemy (an enemy's), ally (an ally's only - the idle human of the tests), empty
// (nobody's). Per class: continents with a food or mine site (a dock's shore area has an id of its own), the resource sites in the zones (player +0xB50: the
// map's zones, every island - stone, gold, iron mines, food; trees are no zone sites: W stays 0), the player's gatherers
// by resource there (citizens whose gather goal's site lies there), the other players' miners there, its drop sites (Town Center, Capitol,
// Settlement), buildings, citizens and land soldiers there. Then its transports, the citizens aboard, and the empty
// continent with the most mines nearest to its base (tiles to its nearest mine). "# isl map:" once a game: every
// continent with sites - its mines, wood, food and a tile of it. "# isl new:" every building first seen (once a game
// minute) off its owner's home continent - how and when the AIs settle other islands.
#define EX_NC 64
enum { EXC_HOME, EXC_OWN, EXC_CONTEST, EXC_ENEMY, EXC_ALLY, EXC_EMPTY, EXC_N };
static const char *const k_exc[EXC_N] = { "home", "own", "contest", "enemy", "ally", "empty" };
struct ex_cont {
    unsigned id;
    int site[7], slots, x, y;                  // sites with something left by resource (2 food .. 6 iron), mine slots, a tile
    short bld[16], drop[16], cit[16], sold[16], tower[16];   // per player (index)
    short gat[16][7];                          // gatherers per player and resource of the site
    short mexp[16], eseen[16], etow[16];       // per AI (viewer): mines it has looked at, enemy buildings / defences it knows there
    short town[16];                            // Town Centers and Capitols per player (a player's home island)
    short etown[16];                           // per AI (viewer): enemy Town Centers and Capitols it knows there
    int msx, msy, mn;                          // the mines' positions summed (their middle)
};
static struct ex_scan { uint32_t t; int n; struct ex_cont c[EX_NC]; int tr[16], aboard[16]; } g_exs;
static uint32_t g_ex_log_t[16], g_ex_new_t;
static int g_ex_map_logged;
static struct { int id, p; uint32_t t; uint16_t known; } g_ex_seen[512];   // buildings off their owner's home continent already logged (# isl new): since, the enemies that saw it (# isl seen)
static int g_ex_nseen;
static int ex_home(ee_player *pl);

static struct ex_cont *ex_cont_of(unsigned id, int add) {
    if (!id) return NULL;
    for (int k = 0; k < g_exs.n; k++) if (g_exs.c[k].id == id) return &g_exs.c[k];
    if (!add || g_exs.n >= EX_NC) return NULL;
    struct ex_cont *c = &g_exs.c[g_exs.n++];
    memset(c, 0, sizeof *c);
    c->id = id; c->x = c->y = -1;
    return c;
}
// the whole map, at most once a second (every player's line reads the same scan)
static void ex_scan(ee_player *pl, uint32_t now, int new_log) {
    if (!new_log && g_exs.t && now >= g_exs.t && now - g_exs.t < 1000) return;
    memset(&g_exs, 0, sizeof g_exs);
    g_exs.t = now ? now : 1;
    unsigned cap = GF(FN_TABLE_VALUE)((void *)GA(G_TABLE_VALUES), 0x16);
    static uint8_t *seen[4096];
    int ns = 0;
    void **zb = FIELD(pl, OF(PL, 0xB50), void **), **ze = FIELD(pl, OF(PL, 0xB54), void **);
    for (int z = 0; zb && zb + z < ze && z < 256; z++) {
        uint8_t *zone = zb[z];
        void **sb = zone ? FIELD(zone, OF(ZONE, 0x8), void **) : NULL, **se = zone ? FIELD(zone, OF(ZONE, 0xC), void **) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096; k++) {
            uint8_t *s = sb[k];
            int r = s ? FIELD(s, OF(UNIT, 0x38), int) : 0, j;
            if (r < 2 || r > 6 || FIELD(s, OF(UNIT, 0x40), int) <= 0) continue;
            for (j = 0; j < ns && seen[j] != s; j++) ;
            if (j < ns) continue;
            if (ns < 4096) seen[ns++] = s;
            int x = FIELD(s, OF(UNIT, 0x1C), int), y = FIELD(s, OF(UNIT, 0x20), int);
            struct ex_cont *c = ex_cont_of(area_id(x, y, 2), 1);
            if (!c) continue;
            c->site[r]++;
            if (r >= 4) {
                c->slots += (int)cap; c->msx += x; c->msy += y; c->mn++;
                for (int v = 1; v < 16; v++)
                    // looked at by its own or an ally's unit (intel.c, honest_vision; 0: the game's view, all of them):
                    // the sites' explored flag is set for every AI from the start (# isl known mines: 100% at minute 1)
                    if (g_players[v] && intel_looked_ago(g_players[v], x, y) != 0xFFFFFFFFu) c->mexp[v]++;
            }
            if (c->x < 0 || r >= 4) { c->x = x; c->y = y; }
        }
    }
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        uint8_t *u = it.u, *td = it.td;
        ee_player *o = unit_owner(u);
        if (!o || FIELD(u, OF(UNIT, 0x3C), int) <= 0) continue;
        int pi = FIELD(o, PL_INDEX, int), fam = FIELD(td, OF(TD, 0x4B4), int), t = FIELD(td, OF(TD, 0x4B8), int), x = FIELD(u, OF(UNIT, 0x1C), int), y = FIELD(u, OF(UNIT, 0x20), int);
        if (pi <= 0 || pi >= 16) continue;
        if (is_transport(t)) { g_exs.tr[pi]++; continue; }
        if (x < 0) { if (is_citizen_type(t)) g_exs.aboard[pi]++; continue; }
        struct ex_cont *c = ex_cont_of(area_id(x, y, 2), 1);
        if (!c) continue;
        if (fam == 2 || fam == 29) {
            if (is_wall(t)) continue;
            c->bld[pi]++;
            // # isl new: off its home continent, first seen - on land with sites (a dock stands in its shore area, a
            // continent of its own)
            if (new_log && g_players[pi] && c->id != (unsigned)ex_home(g_players[pi]) && (c->site[2] || c->mn)) {
                int id = FIELD(u, OF(UNIT, 0x60), int), j;
                for (j = 0; j < g_ex_nseen && !(g_ex_seen[j].id == id && g_ex_seen[j].p == pi); j++) ;
                if (j == g_ex_nseen && g_ex_nseen < 512) {
                    g_ex_seen[g_ex_nseen].id = id; g_ex_seen[g_ex_nseen].t = now; g_ex_seen[g_ex_nseen].known = 0; g_ex_seen[g_ex_nseen++].p = pi;
                    uint8_t *bs = FIELD(u, OF(UNIT, 0x30), uint8_t *);
                    EnterCriticalSection(&g_lock);
                    fprintf(g_log, "# isl new: P%d at %.1f min: type %d at %d,%d on c%u (home c%d)%s\n", pi, now / 60000.0, t, x, y, c->id,
                            ex_home(g_players[pi]), bs && bs[OF(UNIT, 0x340)] ? "" : " - a foundation");
                    LeaveCriticalSection(&g_lock);
                }
                // # isl seen: the first time each enemy AI knows it (honest_vision's seen or remembered) - how late
                // the players notice each other's expansions (the playtester: map control)
                for (int v = 1; j < g_ex_nseen && v < 16; v++) {
                    if (!g_players[v] || v == pi || (g_ex_seen[j].known >> v & 1) || !is_enemy(g_players[v], o) || !seen_by(u, g_players[v])) continue;
                    g_ex_seen[j].known |= (uint16_t)(1u << v);
                    EnterCriticalSection(&g_lock);
                    fprintf(g_log, "# isl seen: P%d at %.1f min: P%d's type %d on c%u (there since %.1f min)\n", v, now / 60000.0, pi, t, c->id,
                            g_ex_seen[j].t / 60000.0);
                    LeaveCriticalSection(&g_lock);
                }
            }
            if (UT_IN(t, UT_TOWNS, UT_X_SINGLE_PLAYER_SETTLEMENT)) c->drop[pi]++;
            if (t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL)) c->town[pi]++;
            float rr, a;
            int def = defence_stats(u, &rr, &a);
            if (def) c->tower[pi]++;
            for (int v = 1; v < 16; v++)   // what each AI knows of its enemies there (honest_vision: seen, or remembered)
                if (g_players[v] && v != pi && is_enemy(g_players[v], o) && seen_by(u, g_players[v])) {
                    c->eseen[v]++; c->etow[v] += def;
                    if (t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL)) c->etown[v]++;
                }
        } else if (is_citizen_type(t)) {
            c->cit[pi]++;
            uint8_t *g = FIELD(u, OF(UNIT, 0x1EC), uint8_t *), *s = g && !strcmp(rtti_name(g), "EEUGGather@@") ? FIELD(g, OF(GOAL, 0x38), uint8_t *) : NULL;
            int r = s && FIELD(s, OF(UNIT, 0x2C), uint8_t *) ? FIELD(s, OF(UNIT, 0x38), int) : 0;
            if (r < 2 || r > 6) continue;
            struct ex_cont *sc = ex_cont_of(area_id(FIELD(s, OF(UNIT, 0x1C), int), FIELD(s, OF(UNIT, 0x20), int), 2), 1);
            if (sc) sc->gat[pi][r]++;
        } else if (is_soldier(td)) c->sold[pi]++;
    }
}
static int ex_home(ee_player *pl) {
    uint8_t *cm = FIELD(pl, OF(PL, 0xB90), uint8_t *), *bm = cm ? FIELD(cm, OF(CM, 0x3C), uint8_t *) : NULL;
    if (!bm || FIELD(bm, OF(BM, 0x12C), int) == -10000) return 0;
    return (int)area_id(FIELD(bm, OF(BM, 0x12C), int), FIELD(bm, OF(BM, 0x130), int), 2);
}
// a continent's class for player pl (home: hc)
static int ex_class(ee_player *pl, const struct ex_cont *c, unsigned hc) {
    if (c->id == hc) return EXC_HOME;
    int pi = pidx(pl), mine = c->bld[pi] > 0, foe = 0, ally = 0;
    for (int p = 1; p < 16; p++) {
        if (p == pi || !c->bld[p]) continue;
        void *pp = player_at(p);
        if (pp && is_enemy(pl, pp)) foe = 1; else ally = 1;
    }
    return mine ? (foe ? EXC_CONTEST : EXC_OWN) : foe ? EXC_ENEMY : ally ? EXC_ALLY : EXC_EMPTY;
}
static void ex_log(ee_player *pl, unsigned idx, uint32_t now) {
    unsigned hc = (unsigned)ex_home(pl);
    if (!hc) return;
    ex_scan(pl, now, 0);
    int pi = (int)idx;
    struct { int n, site[7], my[7], oth, drop, bld, cit, sold, tower; } k[EXC_N];
    memset(k, 0, sizeof k);
    uint8_t *cm = FIELD(pl, OF(PL, 0xB90), uint8_t *), *bm = cm ? FIELD(cm, OF(CM, 0x3C), uint8_t *) : NULL;
    int bx = FIELD(bm, OF(BM, 0x12C), int), by = FIELD(bm, OF(BM, 0x130), int), near_d = -1, near_m = 0, foe_s = 0, foe_b = 0, km = 0, am = 0;
    for (int j = 0; j < g_exs.n; j++) { am += g_exs.c[j].mn; km += g_exs.c[j].mexp[pi]; }   // the map's mines and those it has explored
    for (int j = 0; j < g_exs.n; j++) {
        struct ex_cont *c = &g_exs.c[j];
        int cl = ex_class(pl, c, hc), mines = c->site[4] + c->site[5] + c->site[6];
        if (c->id == hc)   // the enemies on its home island: landings, raids, forward bases
            for (int p = 1; p < 16; p++) {
                void *pp = p != pi ? player_at(p) : NULL;
                if (pp && is_enemy(pl, pp)) { foe_s += c->sold[p]; foe_b += c->bld[p]; }
            }
        if (!c->site[2] && !mines && c->id != hc) continue;   // no site: a dock's shore area (an id of its own) or a bare islet
        k[cl].n++;
        for (int r = 2; r < 7; r++) { k[cl].site[r] += c->site[r]; k[cl].my[r] += c->gat[pi][r]; }
        for (int p = 1; p < 16; p++) if (p != pi) k[cl].oth += c->gat[p][4] + c->gat[p][5] + c->gat[p][6];
        k[cl].drop += c->drop[pi]; k[cl].bld += c->bld[pi]; k[cl].cit += c->cit[pi]; k[cl].sold += c->sold[pi]; k[cl].tower += c->tower[pi];
        if (cl == EXC_EMPTY && mines && c->x >= 0) {
            int d = (int)sqrtf((float)((c->x - bx) * (c->x - bx) + (c->y - by) * (c->y - by)));
            if (near_d < 0 || d < near_d) { near_d = d; near_m = mines; }
        }
    }
    EnterCriticalSection(&g_lock);
    if (!g_ex_map_logged) {
        g_ex_map_logged = 1;
        fprintf(g_log, "# isl map: %d continents:", g_exs.n);
        for (int j = 0; j < g_exs.n; j++) {
            struct ex_cont *c = &g_exs.c[j];
            fprintf(g_log, " c%u S%d G%d I%d W%d F%d at %d,%d;", c->id, c->site[4], c->site[5], c->site[6], c->site[3], c->site[2], c->x, c->y);
        }
        fputc('\n', g_log);
    }
    fprintf(g_log, "# isl: P%u at %u min:", idx, now / 60000);
    for (int cl = 0; cl < EXC_N; cl++)
        fprintf(g_log, " %s n=%d S%d G%d I%d W%d F%d my F%d W%d S%d G%d I%d oth %d drop %d bld %d tow %d cit %d sold %d |", k_exc[cl], k[cl].n,
                k[cl].site[4], k[cl].site[5], k[cl].site[6], k[cl].site[3], k[cl].site[2], k[cl].my[2], k[cl].my[3], k[cl].my[4], k[cl].my[5],
                k[cl].my[6], k[cl].oth, k[cl].drop, k[cl].bld, k[cl].tower, k[cl].cit, k[cl].sold);
    fprintf(g_log, " tr %d aboard %d | near_empty %d (%d mines) | home_foe sold %d bld %d | known mines %d of %d\n", g_exs.tr[pi], g_exs.aboard[pi], near_d, near_m,
            foe_s, foe_b, km, am);
    unsigned *ov = g_oversea[idx];
    if (ov[0]) fprintf(g_log, "# overseas mines: P%u at %u min: %u searches, %u found a home mine (%s), %u an overseas one, %u replaced\n", idx,
                       now / 60000, ov[0], ov[1], g_overseas_mines && !is_vanilla(pl) ? "refused" : "shipped to", ov[2], ov[3]);
    LeaveCriticalSection(&g_lock);
}


// ---- THE OUTPOST ([all] expand_islands, bits, default 0; [autotest] v_expand_islands for the variant players) ----
// Measured first (# isl, base AI at hard, 60 min, 20 seeds x both seats; docs/economy.md "Other islands"): on the
// test maps only Small Islands has a free island (7 mines, 42 slots, more than the home's 30); the base AI settles it in
// 97-100% of the games (minute 31 without the cheat, 23 with it), the mod's AI never on its own.
// 1 SETTLE (water maps: navy_detect's islands or mixed, with landing on - its transport hooks): from the Copper Age and
//   minute expand_minute (8), once its miners fill half the home island's mine slots (or from minute 15), with 25+
//   workers and its land army not weaker than the enemy army it knows (intel.c), the island with the most mines (the
//   nearest first) that holds no ally's building, no enemy town (Town Center, Capitol) it has seen and no enemy defence
//   it knows, at most 3 known enemy buildings and no more known enemy soldiers within 20 tiles than its guard; never one
//   it lost 3 times (8 minutes after a loss); an island with fewer than half its mines looked at is scouted first by
//   the empty transport (phase 4: an enemy town, defence or 4+ buildings seen - never again);
//   - the money: a transport, a Settlement and a tower are held back from unit orders (econ.c train_budget, as a
//     landing's transports) until they are laid - the AI spends its stock as it comes;
//   - one transport (its own, idle and empty, the last trip's first, else trained as a landing's: paid as economy),
//     expand_citizens (8) citizens nearest the home shore (no miners, builders last) and expand_guard (3) land soldiers
//     at home (free ones first) go to the home shore nearest the base and the island, board there (the load goal once
//     the transport is there and 3 in 4 are near, or 2 minutes), and the transport unloads at the island's shore
//     nearest its mines; the sea on the way held (landing.c ld_sea_threat / ld_sea_risk against all its warships):
//     unloaded at home at once (waiting loaded at the shore, 9 of 10 transports were sunk with everyone aboard);
//   - ashore the citizens lay a Settlement by the mines (3 builders, all of them until it stands), then a tower between
//     the mines and the sea and one by the Settlement (and with bit 2 a barracks) - the game's own creation (0x62EF8A)
//     and builders (EEUGBuildRepair), as landing.c's forward base; a building destroyed is laid again (3 times);
//   - once the Settlement stands, every island citizen without work gathers at the island's free mine nearest to it
//     (the game's assignment 0x48C81C; citizen_plan's goal and the metal targets count those slots); the guard stays
//     by the Settlement and attacks the enemies it sees within 14 tiles;
//   - more citizens (expand_trips, 4 trips in all) every 2 minutes while the island has fewer than its mine slots, no
//     enemy was seen at the outpost for a minute and the army is not outmatched; a guard short of expand_guard goes too;
//   - lost: no building and no citizen of it on the island (after it was ashore) for 20 s - again after 8 minutes, 3
//     losses: that island never again.
//   The transport and the passengers are its own: their subgroups' orders refused (landing.c hook_asg_give_goal), the
//   transport subgroup's update skipped, the game's transport manager's crossing and its unload retry refused, the
//   landing never takes the transport, its citizens or soldiers; soldiers on the outpost's island stay its guard.
// 2 BARRACKS: a barracks at the outpost after the second tower.
// 4 ALLY: islands with an ally's buildings count as free too (the tests' idle human: off).
// Large maps (docs/economy.md "Large island maps"; the playtester: most play Large / Huge maps with a high population,
// several free islands; "briefly interrupting home work to claim an island is fine if it is cheap; a few towers on
// every island won't hold"):
// 8 EARLY: the plan from expand_minute without waiting for the home mines to be half worked, from 20 workers.
// 16 EXTRA: the citizens it ships are additional - citizen_plan's goal (citizens.c) grows by the outpost's citizens
//   until its Settlement stands (then the mines' slots near it count, as every drop site's), and the passengers come
//   from wood first (never from the mines).
// 32 MORE: more islands, one after the other and only what it holds: once an outpost holds (its Settlement and its
//   towers stand, no enemy seen there for 3 minutes, its trips made or its slots filled) it is left to the game's
//   economy (its citizens keep the Settlement's mines, the game rebuilds and repairs) and the next is planned - at most
//   expand_max (3) islands besides home; the next free island nearest to what it holds (concentrated, not spread);
//   a held island with nothing of it left is lost (again after 8 minutes, 3 losses: never again).
// 64 FORWARD: an outpost nearer the enemy's known buildings than its home becomes a forward base - a barracks and two
//   more towers there (a ring by the Settlement) - only while it holds the sea (sea_held: no enemy warship seen in its
//   own waters for 5 minutes and its warships at least the enemy's it knows of; the playtester: a forward base with open
//   flanks lets the enemy sneak into the economic heart at home). Home's towers, guard and militia stay as they are;
//   the soldiers the forward barracks trains are the game's army (only the outpost's own guard is held there).
// 128 FOLLOW (docs/economy.md "More metal"; the playtester: the outpost follows sea control and the army, it doesn't
//   lead them): a new outpost and every further trip only while it holds the sea (sea_held) and its land army is at
//   least 1.5 x the enemy army it knows (not merely not weaker); its guard at least 4; a ring of four towers (not two);
//   the idle citizens board first (they have no work at home: the large maps' 31-63 idle at minute 60).
//   Measured as 185 (57 + 128) against the base AI (40 games a cell, Small / Tournament / Large Islands Large-Huge
//   and Small Islands Small, both cheats): miners on other islands +1 to +4 at minute 30, enemy soldiers on the home
//   island -2 to -5 without the cheat, but the other islands' miners at 60 back to even or below (-7 (3) on Small
//   Islands Large) and kills - losses -13 .. +15, all within the noise; off.
static int g_expand = 0, g_v_expand = -1, g_ex_cits = 8, g_ex_guard = 3, g_ex_min = 8, g_ex_trips = 4, g_ex_max = 3;
#define EXPAND(idx) PV(idx, g_v_expand, g_expand)
#define EX_GUARD(idx) (EXPAND(idx) & 128 && g_ex_guard < 4 ? 4 : g_ex_guard)   // FOLLOW: a guard of 4 at least
enum { XK_TOWER, XK_DROP, XK_BARRACKS };
static const char *const k_xk[3] = { "tower", "drop site", "barracks" };
struct ex_proj { uint8_t *b; int id, kind, type, state, tries, x, y, waits; };   // state 0 to come, 1 going up, 2 standing, 3 dropped
#define EX_PROJ 8
struct ex_unit { uint8_t *u; int id; };
static struct ex_st {
    int active; unsigned cont; int ocean, mx, my, ux, uy, uwx, uwy;   // the outpost's island, its sea, the mines' middle, the unload tile and its water
    int hx, hy, hwx, hwy;                 // the home shore where they board and the water in front
    struct ex_proj proj[EX_PROJ]; int nproj;
    uint32_t t, log_t, base_t, empty_t, trip_t, next_try, plan_t;
    int ever_ashore, trips, drop_ok, ncit, nsold;
    int fphase;                           // the ferry: 0 none, 1 a transport, 2 boarding, 3 at sea, 4 scouting (empty)
    struct ex_unit ship, pass[16], last; int npass;
    uint32_t fsince, freissue, sea_t, load_t;
    uint32_t ftrain_t; int guard_trip;
    struct { unsigned cont; int lost; uint32_t until; } gone[8]; int ngone;
    unsigned plans, sails, ashore, drops, lost, given, orders, assigned, built, trained;
    uint32_t res_t;                       // the reserve for the drop site and a tower: since (expand_reserve)
    uint32_t foe_t;                       // enemies seen at the outpost last
    struct { unsigned cont; int x, y; } held[4]; int nheld;   // MORE: the islands it holds besides home (left to the game)
    struct ex_unit guard[4]; int nguard;  // FORWARD: the outpost's own guard (the rest there is the game's army)
    int forward, fwd_added;               // FORWARD: the outpost is nearer the enemy; its barracks and towers added
    unsigned forwards, graduated, held_lost, sea_no;
    uint32_t fsay_t, ftrained_t;          // the last "can't train a transport" line, the last transport ordered
    unsigned scouted, scouts, scout_bad;  // the island the ferry's transport has looked at (phase 4)
    uint32_t scout_at;                    // the scouting transport at the island's shore since
} g_xs[16];

static int ex_alive(struct ex_unit *e, ee_player *pl) {
    uint8_t *u = e->u, *td = u ? FIELD(u, OF(UNIT, 0x2C), uint8_t *) : NULL;
    return td && FIELD(u, OF(UNIT, 0x3C), int) > 0 && FIELD(u, OF(UNIT, 0x60), int) == e->id && unit_owner(u) == pl;
}
static PRINTF_LIKE(3, 4) void ex_say(unsigned idx, uint32_t now, const char *fmt, ...) {
    if (!g_log) return;
    char buf[400];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# expand: P%u at %.1f min: %s\n", idx, now / 60000.0, buf);
    LeaveCriticalSection(&g_lock);
}
static int ex_cargo(uint8_t *sh) { return GF(FN_UNITS_INSIDE)(sh); }
// the ferry's transport and passengers, and soldiers on an outpost's island (landing.c's order hooks)
static int expand_ship(uint8_t *u) {
    if (!u) return 0;
    for (int p = 1; p < 16; p++) if (g_xs[p].fphase && g_xs[p].ship.u == u) return p;
    return 0;
}
static int expand_refuses(uint8_t *sg, uint8_t *u) {
    ee_player *pl = FIELD(sg, OF(ASG, 0x190), void *);
    unsigned pi = pl ? (unsigned)FIELD(pl, PL_INDEX, int) & 15 : 0;
    struct ex_st *x = &g_xs[pi];
    if (!u || (!x->fphase && !x->active)) return 0;
    if (x->fphase) {
        if (x->ship.u == u) return 1;
        for (int k = 0; k < x->npass; k++) if (x->pass[k].u == u) return 1;
    }
    int ux = FIELD(u, OF(UNIT, 0x1C), int);
    if (!x->active || ux < 0 || area_id(ux, FIELD(u, OF(UNIT, 0x20), int), 2) != x->cont || !is_soldier(FIELD(u, OF(UNIT, 0x2C), uint8_t *))) return 0;
    if (!(EXPAND(pi) & 64)) return 1;
    for (int k = 0; k < x->nguard; k++) if (x->guard[k].u == u) return 1;   // FORWARD: its guard only
    return 0;
}
static int expand_has(ee_player *pl, uint8_t *u) {
    struct ex_st *x = &g_xs[(unsigned)FIELD(pl, PL_INDEX, int) & 15];
    for (int k = 0; x->fphase && k < x->npass; k++) if (x->pass[k].u == u) return 1;
    return 0;
}
// the drop site and a tower an outpost waits for (planned and short of them, or ashore with them not laid yet): held
// back from every unit order (econ.c train_budget, as a landing's transports) - the AI spends its stock as it comes,
// and outposts were held up 5-10 minutes with "can't pay"
static void expand_reserve(ee_player *pl, int reserve[7]) {
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    struct ex_st *x = &g_xs[idx];
    if (!EXPAND(idx) || g_landing_order) return;   // its own transport (and a landing's) spend it
    if (x->fphase == 1) {   // the transport it waits for (as landing_reserve)
        int tt = ld_transport_type(pl);
        for (int r = 2; r < 7 && tt; r++) reserve[r] += unit_cost(pl, tt, r);
    }
    if (!x->res_t || game_time() - x->res_t > 180000) return;
    int tt = ld_kind_type(pl, LDB_TOWER);
    for (int r = 2; r < 7; r++) reserve[r] += unit_cost(pl, UT(B_SETTLEMENT), r) + (tt ? unit_cost(pl, tt, r) : unit_cost(pl, UT(B_GUARD_TOWER_MIDDLE), r));
}
static int expand_leads(uint8_t *sg) {   // a subgroup holding the ferry's transport: its update skipped
    ee_player *pl = FIELD(sg, OF(ASG, 0x190), void *);
    unsigned pi = pl ? (unsigned)FIELD(pl, PL_INDEX, int) & 15 : 0;
    struct ex_st *x = &g_xs[pi];
    if (!x->fphase || !x->ship.u) return 0;
    for (uint8_t **r = FIELD(sg, OF(ASG, 0x54), uint8_t **); r && r < FIELD(sg, OF(ASG, 0x58), uint8_t **); r++)
        if (*r && FIELD(*r, OF(ASGU, 0x44), uint8_t *) == x->ship.u) return 1;
    return 0;
}
// the unload a ferry may take from the game (its own retry at its own point while it unloads)
static int expand_unload_ok(uint8_t *u, int gx, int gy) {
    int p = expand_ship(u);
    return p && g_xs[p].fphase == 3 && abs(gx - g_xs[p].ux) <= 4 && abs(gy - g_xs[p].uy) <= 4;
}

// a land tile of continent cont with room (3x3 free) and the sea oc within 2 tiles, nearest to (x, y) within r
static int ex_shore(unsigned cont, int oc, int x, int y, int r, int *ox, int *oy, int *wx, int *wy) {
    for (int d = 0; d <= r; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d || !d ? 1 : 2 * d)) {
                int tx = x + dx, ty = y + dy, ax, ay;
                if (area_id(tx, ty, 2) != cont || area_id(tx, ty, 1)) continue;
                if ((int)ocean_near(tx, ty, 2, &ax, &ay) != oc || !ld_tile_free(tx, ty)) continue;
                *ox = tx; *oy = ty;
                if (wx) { *wx = ax; *wy = ay; }
                return 1;
            }
    return 0;
}
static int ex_gone(struct ex_st *x, unsigned cont, uint32_t now) {
    for (int k = 0; k < x->ngone; k++) if (x->gone[k].cont == cont) return x->gone[k].lost >= 3 || now < x->gone[k].until;
    return 0;
}
static int ex_mark_lost(struct ex_st *x, unsigned cont, uint32_t now) {   // the island's losses so far
    int k = 0;
    while (k < x->ngone && x->gone[k].cont != cont) k++;
    if (k == x->ngone) { if (k == 8) return 0; x->ngone++; x->gone[k].cont = cont; x->gone[k].lost = 0; }
    x->gone[k].until = now + 480000;
    return ++x->gone[k].lost;
}
// the ferry's end: the passengers still at home are free again, a transport with someone aboard unloads at the land
// nearest to it
static void ex_ferry_end(ee_player *pl, unsigned idx, struct ex_st *x, uint32_t now, const char *why) {
    // those still walking to the transport first (its load goal alive): their part of it replaced by a stop where they
    // are - left as it was, the unload below deleted the load goal and the next order to them called the deleted
    // goal (_purecall, 2 of 80 island games)
    if (x->fphase == 2 && ex_alive(&x->ship, pl))
        for (int k = 0; k < x->npass; k++) {
            uint8_t *u = x->pass[k].u;
            if (!ex_alive(&x->pass[k], pl) || FIELD(u, OF(UNIT, 0x1C), int) < 0) continue;
            x->orders += unit_goal(u, goal_move((float)FIELD(u, OF(UNIT, 0x1C), int) + 0.5f, (float)FIELD(u, OF(UNIT, 0x20), int) + 0.5f), 0, 0x1F40);
        }
    if (x->fphase >= 2 && ex_alive(&x->ship, pl) && ex_cargo(x->ship.u) > 0) {
        uint8_t *sh = x->ship.u;
        int x0 = FIELD(sh, OF(UNIT, 0x1C), int), y0 = FIELD(sh, OF(UNIT, 0x20), int), tx, ty;
        unsigned c = x->fphase == 3 ? x->cont : area_id(x->hx, x->hy, 2);
        if (ex_shore(c, x->ocean, x0, y0, 30, &tx, &ty, NULL, NULL) || ex_shore(area_id(x->hx, x->hy, 2), x->ocean, x0, y0, 40, &tx, &ty, NULL, NULL))
            x->orders += unit_goal(sh, ld_unload(sh, tx, ty), 0, 0x1F40);
    }
    if (why) ex_say(idx, now, "%s", why);
    x->fphase = 0; x->npass = 0; x->ship.u = NULL;
}

// the free island for a new outpost (0: none) - and why not
static unsigned ex_pick(ee_player *pl, unsigned idx, struct ex_st *x, unsigned hc, int ocean, int bx, int by, uint32_t now, const char **why) {
    unsigned best = 0;
    float bs = -1e9f;
    int any = 0;
    for (int j = 0; j < g_exs.n; j++) {
        struct ex_cont *c = &g_exs.c[j];
        if (c->id == hc || c->mn < 2) continue;
        if ((EXPAND(idx) & 32) && c->bld[idx]) continue;   // MORE: one it holds (or a landing's forward base) is no new one
        any = 1;
        int ally = 0, town = c->etown[idx];
        for (int p = 1; p < 16; p++) {
            if (p == (int)idx || !c->bld[p]) continue;
            void *pp = player_at(p);
            if (pp && !is_enemy(pl, pp)) ally = 1;
        }
        // an enemy's town (Town Center, Capitol) it knows there (honest_vision: seen or remembered): its home island,
        // never a free one. Until 2026-10-20 every enemy town counted, seen or not (no player knows where the others
        // started - that was the game's knowledge); now only what it saw. Where the islands and their mines are comes
        // from the game's resource zones (the AI's own map of sites, as the base AI has it); whether one is free only
        // from looking: an island with fewer than half its mines looked at (intel.c - the sites' explored flag is set
        // for everyone from the start) is scouted first by the ferry's transport (ex_ferry_scout), and an enemy town
        // seen there before the ferry sails or once ashore ends the outpost (that island never again; ex_enemy_home).
        if (town || (ally && !(EXPAND(idx) & 4)) || c->etow[idx] || c->eseen[idx] > 3 || ex_gone(x, c->id, now)) continue;
        int mx = c->msx / c->mn, my = c->msy / c->mn, n = 0;
        float hp = 0, att = 0;
        intel_soldiers(pl, mx, my, 20, &hp, &att, NULL, &n);
        if (n > EX_GUARD(idx)) continue;
        int wx, wy;
        if ((int)ocean_near(mx, my, 25, &wx, &wy) == 0) continue;
        float d = sqrtf((float)((mx - bx) * (mx - bx) + (my - by) * (my - by)));
        for (int k = 0; (EXPAND(idx) & 32) && k < x->nheld; k++) {   // MORE: concentrated - the nearest of what it holds
            float dh = sqrtf((float)((mx - x->held[k].x) * (mx - x->held[k].x) + (my - x->held[k].y) * (my - x->held[k].y)));
            if (dh < d) d = dh;
        }
        float sc = 10.0f * (float)c->mn - 0.25f * d - 15.0f * (float)c->eseen[idx];
        if (sc > bs) { bs = sc; best = c->id; }
    }
    (void)ocean;
    *why = !any ? "no other island with 2+ mines" : !best ? "every island with mines held, defended or lost" : NULL;
    return best;
}
// the passengers: citizens nearest the home shore (within 35 tiles), soldiers at home (free ones first)
struct ex_pick_scan { ee_player *pl; void *cm; struct ex_st *x; unsigned hc; int cx, cy, nc, ns; uint8_t *c[48], *s[48]; int cd[48], sd[48]; };
static void ex_pick_unit(uint8_t *u, void *ctx) {
    struct ex_pick_scan *p = ctx;
    uint8_t *td = FIELD(u, OF(UNIT, 0x2C), uint8_t *);
    if (unit_owner(u) != p->pl || FIELD(u, OF(UNIT, 0x3C), int) <= 0 || !td || FIELD(u, OF(UNIT, 0x1C), int) < 0 || FIELD(u, OF(UNIT, 0x33B), uint8_t)) return;
    int x = FIELD(u, OF(UNIT, 0x1C), int), y = FIELD(u, OF(UNIT, 0x20), int), d = (x - p->cx) * (x - p->cx) + (y - p->cy) * (y - p->cy);
    if (d > 35 * 35 || area_id(x, y, 2) != p->hc || landing_has_unit(p->pl, u)) return;
    int t = FIELD(td, OF(TD, 0x4B8), int);
    uint8_t *g0 = FIELD(u, OF(UNIT, 0x1EC), uint8_t *);
    if (g0 && strstr(rtti_name(g0), "Load")) return;   // a part of a load goal (perhaps of a transport sunk since): never ordered
    if (is_citizen_type(t)) {
        uint8_t *g = FIELD(u, OF(UNIT, 0x1EC), uint8_t *);
        const char *gn = g ? rtti_name(g) : "";
        if (!strcmp(gn, "EEUGBuildRepair@@")) d += 400;   // builders last
        if (!g && (EXPAND((unsigned)FIELD(p->pl, PL_INDEX, int) & 15) & 128)) d -= 2600;   // FOLLOW: the idle first
        if (!strcmp(gn, "EEUGGather@@")) {   // miners stay (the citizens off wood and food come back through the plan's goal)
            uint8_t *st = FIELD(g, OF(GOAL, 0x38), uint8_t *);
            int r = st && FIELD(st, OF(UNIT, 0x2C), uint8_t *) ? FIELD(st, OF(UNIT, 0x38), int) : 0;
            if (r >= 4 && r <= 6) return;
            if (r == 3 && (EXPAND((unsigned)FIELD(p->pl, PL_INDEX, int) & 15) & 16)) d -= 1300;   // EXTRA: wood first (it piles up on large maps)
        }
        if (p->nc < 48) { p->c[p->nc] = u; p->cd[p->nc++] = d; }
    } else if (land_soldier(t) && is_soldier(td) && !is_ship_or_air(td) && !is_transport(t) && !is_siege(t) && !as_fighting(u) && p->ns < 48) {   // land soldiers only (a transport counted as one and got a part of its own load goal: _purecall)
        if (!GF(FN_CM_UNIT_FREE)(p->cm, FIELD(u, OF(UNIT, 0x30), void *))) d += 10000;   // in a group: after the free ones
        p->s[p->ns] = u; p->sd[p->ns++] = d;
    }
}
static int ex_take(uint8_t **u, int *d, int n) {   // the nearest left (index), -1: none
    int b = -1;
    for (int k = 0; k < n; k++) if (u[k] && (b < 0 || d[k] < d[b])) b = k;
    return b;
}
// phase 1: a transport (its own, idle and empty, else trained), then the passengers
static float ex_sea_risk(ee_player *pl, struct ex_st *x, uint8_t *trtd, float hp);   // below
static void ex_ferry_ship(ee_player *pl, unsigned idx, struct ex_st *x, uint32_t now) {
    void *cm = FIELD(pl, OF(PL, 0xB90), void *);
    uint8_t *um = cm ? FIELD(cm, OF(CM, 0x44), uint8_t *) : NULL;
    uint8_t *best = NULL;
    int bd = 0x7FFFFFFF, all = 0, nl = 0, nld = 0, no = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        uint8_t *u = it.u;
        if (unit_owner(u) != pl || FIELD(u, OF(UNIT, 0x3C), int) <= 0 || !is_transport(FIELD(it.td, OF(TD, 0x4B8), int)) || FIELD(u, OF(UNIT, 0x1C), int) < 0) continue;
        all++;
        if (landing_has_unit(pl, u)) { nl++; continue; }
        if (ex_cargo(u) > 0) { nld++; continue; }
        if ((int)ocean_near(FIELD(u, OF(UNIT, 0x1C), int), FIELD(u, OF(UNIT, 0x20), int), 2, NULL, NULL) != x->ocean) { no++; continue; }
        int dx = FIELD(u, OF(UNIT, 0x1C), int) - x->hwx, dy = FIELD(u, OF(UNIT, 0x20), int) - x->hwy, d = dx * dx + dy * dy;
        if (u == x->last.u && FIELD(u, OF(UNIT, 0x60), int) == x->last.id) d = -1;   // the last trip's
        if (d < bd) { bd = d; best = u; }
    }
    if (!best) {
        // (not while one it ordered is still being built: 2 minutes - the landing took the new ones now and then)
        if (now - x->fsince >= (EXPAND(idx) & 8 ? 720000u : 360000u) && (!x->ftrained_t || now - x->ftrained_t >= 120000)) { ex_ferry_end(pl, idx, x, now, "no transport - given up");   // EARLY: 12 minutes (the early wood is spent on buildings as it comes)
            x->next_try = now + 600000; if (!x->ever_ashore) x->active = 0; return; }
        int tt = ld_transport_type(pl);
        if (um && tt && now >= x->ftrain_t) {   // as a landing's transport: economy, the stock above the reserve
            x->ftrain_t = now + 30000;
            int req[4] = { tt, 1, 0, 0 };
            unsigned got = 0;
            g_priority_mil = 1; g_landing_order = 1; g_expand_order = (EXPAND(idx) & 8) != 0;
            hook_train_units(um, NULL, req, (uintptr_t)&got);
            g_priority_mil = 0; g_landing_order = 0; g_expand_order = 0;
            if (got) { x->trained++; x->ftrain_t = now + 90000; x->ftrained_t = now ? now : 1; }
            // EARLY: tried again at every tick (2 s) while it can't pay - the wood for it (250) is spent by the buildings
            // within seconds of coming in (6 minutes of "can't train" at minute 15-21 with 150-257 wood passing through)
            else if (EXPAND(idx) & 8) { x->ftrain_t = now + 2000; if (now - x->fsay_t < 30000) return; }
            x->fsay_t = now;
            int *st = (int *)((uint8_t *)pl + PL_STOCK);
            ex_say(idx, now, "%s a transport (type %d) - transports %d (landing's %d, loaded %d, other sea %d), landing phase %d, stock %d,%d,%d,%d,%d",
                   got ? "trains" : "can't train", tt, all, nl, nld, no, g_ld[idx].phase, st[2], st[3], st[4], st[5], st[6]);
        }
        return;
    }
    x->ship.u = best; x->ship.id = FIELD(best, OF(UNIT, 0x60), int);
    ex_scan(pl, now, 0);
    {   struct ex_cont *c = ex_cont_of(x->cont, 0);   // an island it has hardly looked at: scouted first, empty
        if (x->scouted != x->cont && c && c->mexp[idx] * 2 < c->mn) {
            float risk = ex_sea_risk(pl, x, FIELD(best, OF(UNIT, 0x2C), uint8_t *), (float)FIELD(best, OF(UNIT, 0x3C), int));
            if (risk > 0.4f) {   // as the ferry's crossing (2 of 3 scouting transports were sunk without the check)
                ex_say(idx, now, "won't scout the island %u - the sea on the way held (risk %.2f)", x->cont, risk);
                x->ship.u = NULL; x->fphase = 0; x->active = 0; x->next_try = now + 180000;
                return;
            }
            x->fphase = 4; x->fsince = now; x->scouts++; x->scout_at = 0;
            x->orders += unit_goal(best, goal_move((float)x->uwx + 0.5f, (float)x->uwy + 0.5f), 0, 0x1F40);
            ex_say(idx, now, "scouts the island %u first (%d of %d mines looked at) - the transport at %d,%d to %d,%d", x->cont, c->mexp[idx], c->mn,
                   FIELD(best, OF(UNIT, 0x1C), int), FIELD(best, OF(UNIT, 0x20), int), x->uwx, x->uwy);
            return;
        } }
    // the passengers
    static struct ex_pick_scan p;
    memset(&p, 0, sizeof p);
    p.pl = pl; p.cm = cm; p.x = x; p.hc = area_id(x->hx, x->hy, 2); p.cx = x->hx; p.cy = x->hy;
    grid_each(ex_pick_unit, &p);
    int cap = ld_cap(FIELD(FIELD(best, OF(UNIT, 0x2C), uint8_t *), OF(TD, 0x4B8), int)), guard = x->guard_trip ? EX_GUARD(idx) - x->nsold : 0;
    if (guard < 0) guard = 0;
    int cits = x->ever_ashore ? cap - guard : g_ex_cits;
    if (cits > cap - guard) cits = cap - guard;
    if (cits > g_ex_cits + 4) cits = g_ex_cits + 4;
    if (x->ever_ashore) {   // more for the mines: up to the slots left
        int slots = 0;
        for (int j = 0; j < g_exs.n; j++) if (g_exs.c[j].id == x->cont) slots = g_exs.c[j].slots;
        if (cits > slots - x->ncit) cits = slots - x->ncit;
    }
    x->npass = 0;
    for (int k = 0; k < cits && x->npass < 16; k++) {
        int b = ex_take(p.c, p.cd, p.nc);
        if (b < 0) break;
        x->pass[x->npass].u = p.c[b]; x->pass[x->npass++].id = FIELD(p.c[b], OF(UNIT, 0x60), int); p.c[b] = NULL;
    }
    int nc = x->npass;
    for (int k = 0; k < guard && x->npass < 16; k++) {
        int b = ex_take(p.s, p.sd, p.ns);
        if (b < 0) break;
        x->pass[x->npass].u = p.s[b]; x->pass[x->npass++].id = FIELD(p.s[b], OF(UNIT, 0x60), int); p.s[b] = NULL;
        int g = 0;   // FORWARD: the guard list (the dead ones' places reused)
        while (g < x->nguard && ex_alive(&x->guard[g], pl)) g++;
        if (g < 4) { x->guard[g] = x->pass[x->npass - 1]; if (g == x->nguard) x->nguard++; }
    }
    if (nc < 3) { ex_ferry_end(pl, idx, x, now, "fewer than 3 citizens near the shore - later"); x->next_try = now + 120000; if (!x->ever_ashore) x->active = 0; return; }
    x->fphase = 2; x->fsince = now; x->freissue = 0; x->load_t = 0; x->sea_t = 0;
    ex_say(idx, now, "boarding %d citizens and %d soldiers at %d,%d for the island %u (mines by %d,%d, unload at %d,%d), transport at %d,%d",
           nc, x->npass - nc, x->hx, x->hy, x->cont, x->mx, x->my, x->ux, x->uy, FIELD(best, OF(UNIT, 0x1C), int), FIELD(best, OF(UNIT, 0x20), int));
}
// the sea on the way: enemy ships, submarines and aircraft that hit the transport, against its warships (landing.c)
static float ex_sea_risk(ee_player *pl, struct ex_st *x, uint8_t *trtd, float hp) {
    static struct ld_sea e, o;
    memset(&o, 0, sizeof o);
    ld_sea_threat(pl, trtd, x->hwx, x->hwy, x->uwx, x->uwy, x->ux, x->uy, &e);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        uint8_t *u = it.u;
        if (unit_owner(u) != pl || FIELD(u, OF(UNIT, 0x3C), int) <= 0 || FIELD(u, OF(UNIT, 0x1C), int) < 0) continue;
        int fam = FIELD(it.td, OF(TD, 0x4B4), int), t = FIELD(it.td, OF(TD, 0x4B8), int);
        if ((fam == 4 || fam == 10) && !is_transport(t) && !is_fishing_boat(t) && FIELD(it.td, OF(TD, 0x510), int) > 0) ld_sea_add(&o, &e, u);
    }
    return ld_sea_risk(&e, &o, hp);
}
// phase 4: the empty transport looks at the island (its mines' water): enemy town, defence or 4+ known buildings - that
// island never again; looked at - back to the home shore for the passengers (phase 1 again); 4 minutes: given up
static void ex_ferry_scout(ee_player *pl, unsigned idx, struct ex_st *x, uint32_t now) {
    uint8_t *sh = x->ship.u;
    ex_scan(pl, now, 0);
    struct ex_cont *c = ex_cont_of(x->cont, 0);
    // looked at: half its mines (intel.c), or the transport 10 s at the island's shore water (it sees the shore and
    // what stands by it - the mines often lie beyond its sight)
    int dx = FIELD(sh, OF(UNIT, 0x1C), int) - x->uwx, dy = FIELD(sh, OF(UNIT, 0x20), int) - x->uwy;
    if (dx * dx + dy * dy <= 36) { if (!x->scout_at) x->scout_at = now ? now : 1; } else x->scout_at = 0;
    int seen = (c && c->mexp[idx] * 2 >= c->mn) || (x->scout_at && now - x->scout_at >= 10000);
    if (c && (c->etown[idx] || c->etow[idx] || c->eseen[idx] > 3)) {
        while (ex_mark_lost(x, x->cont, now) < 3 && x->ngone < 8) ;
        x->orders += unit_goal(sh, goal_move((float)x->hwx + 0.5f, (float)x->hwy + 0.5f), 0, 0x1F40);
        ex_say(idx, now, "scouted the island %u: %s (%d known enemy buildings) - never again", x->cont, c->etown[idx] ? "an enemy town" : c->etow[idx] ?
               "enemy defences" : "enemy buildings", c->eseen[idx]);
        x->fphase = 0; x->npass = 0; x->ship.u = NULL; x->active = 0; x->next_try = now + 30000; x->scout_bad++;
        return;
    }
    if (seen) {
        x->scouted = x->cont; x->fphase = 1; x->fsince = now;
        x->orders += unit_goal(sh, goal_move((float)x->hwx + 0.5f, (float)x->hwy + 0.5f), 0, 0x1F40);
        ex_say(idx, now, "scouted the island %u: free (%d of %d mines looked at, %d known enemy buildings) - the passengers next", x->cont, c->mexp[idx],
               c->mn, c->eseen[idx]);
        return;
    }
    if (now - x->fsince >= 240000) {
        x->orders += unit_goal(sh, goal_move((float)x->hwx + 0.5f, (float)x->hwy + 0.5f), 0, 0x1F40);
        ex_say(idx, now, "scouting the island %u: not looked at in 4 minutes - later", x->cont);
        x->fphase = 0; x->npass = 0; x->ship.u = NULL; x->active = 0; x->next_try = now + 180000;
        return;
    }
    if (!as_goal_near(sh, (float)x->uwx + 0.5f, (float)x->uwy + 0.5f, 3.0f))
        x->orders += unit_goal(sh, goal_move((float)x->uwx + 0.5f, (float)x->uwy + 0.5f), 0, 0x1F40);
}
// phase 2: to the home shore, aboard, then off
static void ex_ferry_board(ee_player *pl, unsigned idx, struct ex_st *x, uint32_t now) {
    uint8_t *sh = x->ship.u;
    int n = 0, aboard = 0, cits = 0;
    for (int k = 0; k < x->npass; k++)
        if (ex_alive(&x->pass[k], pl)) { x->pass[n++] = x->pass[k]; }
    x->npass = n;
    for (int k = 0; k < x->npass; k++) {
        uint8_t *u = x->pass[k].u;
        int ab = FIELD(u, OF(UNIT, 0x1C), int) < 0;
        aboard += ab; cits += ab && is_citizen_type(FIELD(FIELD(u, OF(UNIT, 0x2C), uint8_t *), OF(TD, 0x4B8), int));
    }
    int sdx = FIELD(sh, OF(UNIT, 0x1C), int) - x->hwx, sdy = FIELD(sh, OF(UNIT, 0x20), int) - x->hwy, there = sdx * sdx + sdy * sdy <= 49;
    if (!there && !as_goal_near(sh, (float)x->hwx + 0.5f, (float)x->hwy + 0.5f, 2.0f) && !x->load_t)
        x->orders += unit_goal(sh, goal_move((float)x->hwx + 0.5f, (float)x->hwy + 0.5f), 0, 0x1F40);
    int all = aboard == x->npass && x->npass;
    if (all || (x->load_t && now - x->load_t >= 90000 && aboard * 10 >= x->npass * 7) || (now - x->fsince >= 240000 && cits >= 3)) {
        if (cits < 3) { ex_ferry_end(pl, idx, x, now, "boarding failed"); x->next_try = now + 300000; if (!x->ever_ashore) x->active = 0; return; }
        uint8_t *td = FIELD(sh, OF(UNIT, 0x2C), uint8_t *);
        float risk = ex_sea_risk(pl, x, td, (float)FIELD(sh, OF(UNIT, 0x3C), int));
        if (risk > 0.4f) {   // the sea held: unloaded at home at once (waiting loaded at the shore, 9 of 10 such transports were sunk with everyone aboard)
            char why[96];
            snprintf(why, sizeof why, "the sea on the way held (risk %.2f) - called off, unloaded at home", risk);
            ex_ferry_end(pl, idx, x, now, why); x->next_try = now + 180000; if (!x->ever_ashore) x->active = 0;
            return;
        }
        n = 0;   // those left at home are free again
        for (int k = 0; k < x->npass; k++) if (FIELD(x->pass[k].u, OF(UNIT, 0x1C), int) < 0) x->pass[n++] = x->pass[k];
        x->npass = n;
        void *g = ld_unload(sh, x->ux, x->uy);
        x->orders += unit_goal(sh, g, 0, 0x1F40);
        x->fphase = 3; x->fsince = now; x->sails++;
        ex_say(idx, now, "sails with %d aboard (%d citizens) to %d,%d", aboard, cits, x->ux, x->uy);
        return;
    }
    if (now - x->fsince >= 300000) {
        if (g_log) {   // what kept them: the transport, the passengers' goals
            char gs[160] = "";
            for (int k = 0; k < x->npass && k < 6; k++) {
                uint8_t *u = x->pass[k].u, *g = FIELD(u, OF(UNIT, 0x1EC), uint8_t *);
                size_t l0 = strlen(gs);
                snprintf(gs + l0, sizeof gs - l0, " %d,%d:%.14s", FIELD(u, OF(UNIT, 0x1C), int), FIELD(u, OF(UNIT, 0x20), int), g ? rtti_name(g) : "none");
            }
            ex_say(idx, now, "boarding: %d of %d aboard, the transport at %d,%d (goal %s), passengers%s", aboard, x->npass, FIELD(sh, OF(UNIT, 0x1C), int),
                   FIELD(sh, OF(UNIT, 0x20), int), FIELD(sh, OF(UNIT, 0x1EC), void *) ? rtti_name(FIELD(sh, OF(UNIT, 0x1EC), void *)) : "none", gs);
        }
        ex_ferry_end(pl, idx, x, now, "boarding failed"); x->next_try = now + 300000; if (!x->ever_ashore) x->active = 0; return;
    }
    // the passengers to the shore first (as landing.c: loading straight away, units strewn 30-40 tiles off walked to
    // a transport strewn along the coast); the transport there and 3 in 4 near, or 2 minutes: the load goal (again
    // every 20 s for those not aboard)
    int close = 0;
    for (int k = 0; k < x->npass; k++) {
        uint8_t *u = x->pass[k].u;
        if (FIELD(u, OF(UNIT, 0x1C), int) < 0) { close++; continue; }
        int dx = FIELD(u, OF(UNIT, 0x1C), int) - x->hx, dy = FIELD(u, OF(UNIT, 0x20), int) - x->hy;
        if (dx * dx + dy * dy <= 64) { close++; continue; }
        if (!x->load_t && !as_goal_near(u, (float)x->hx + 0.5f, (float)x->hy + 0.5f, 3.0f))   // before the load goal
            x->orders += unit_goal(u, goal_move((float)x->hx + 0.5f, (float)x->hy + 0.5f), 0, 0x1F40);
    }
    if (g_log && (now - x->fsince) % 30000 < 2000)
        ex_say(idx, now, "boarding - the transport at %d,%d (%s, %d to the shore water %d,%d), %d of %d near the shore %d,%d, %d aboard%s", FIELD(sh, OF(UNIT, 0x1C), int),
               FIELD(sh, OF(UNIT, 0x20), int), FIELD(sh, OF(UNIT, 0x1EC), void *) ? rtti_name(FIELD(sh, OF(UNIT, 0x1EC), void *)) : "no goal",
               (int)sqrtf((float)(sdx * sdx + sdy * sdy)), x->hwx, x->hwy, close, x->npass, x->hx, x->hy, aboard, x->load_t ? ", loading" : "");
    if (!x->load_t && !((there && close * 4 >= x->npass * 3) || now - x->fsince >= 120000)) return;
    if (x->freissue && now - x->freissue < 20000) return;
    x->freissue = now ? now : 1;
    if (!x->load_t) x->load_t = now ? now : 1;
    uint8_t *list[16];
    int m = 0;
    for (int k = 0; k < x->npass; k++) if (FIELD(x->pass[k].u, OF(UNIT, 0x1C), int) >= 0) list[m++] = x->pass[k].u;
    if (!m) return;
    void *g = goal_load(sh, list, m);
    // the transport's goal now, or it is gone: a load goal given to a transport busy with the game's own (an unload,
    // the transport manager's) ended at once and was deleted - its parts then called the deleted goal (_purecall,
    // 2 of 80 island games)
    if (!g || FIELD(sh, OF(UNIT, 0x1EC), void *) != g) return;
    x->orders++;
    for (int k = 0; k < m; k++) x->orders += unit_goal(list[k], goal_load_part(g), 0, 0x1F40);
}
// phase 3: at sea - unloaded again where it stopped with someone aboard; all ashore: the transport back, the ferry ends
static void ex_ferry_sail(ee_player *pl, unsigned idx, struct ex_st *x, uint32_t now) {
    uint8_t *sh = x->ship.u, *cur = FIELD(sh, OF(UNIT, 0x1EC), uint8_t *);
    int n = ex_cargo(sh);
    if (n <= 0) {
        int ashore = 0;
        for (int k = 0; k < x->npass; k++) ashore += ex_alive(&x->pass[k], pl) && FIELD(x->pass[k].u, OF(UNIT, 0x1C), int) >= 0;
        x->ashore++; x->ever_ashore = 1; x->empty_t = 0;
        if (x->nproj && !x->base_t) x->base_t = now;
        x->orders += unit_goal(sh, goal_move((float)x->hwx + 0.5f, (float)x->hwy + 0.5f), 0, 0x1F40);
        x->last = x->ship;
        x->fphase = 0; x->npass = 0; x->ship.u = NULL; x->trip_t = now ? now : 1;
        ex_say(idx, now, "ashore on the island %u - %d landed", x->cont, ashore);
        return;
    }
    if (now - x->fsince >= 360000) { ex_ferry_end(pl, idx, x, now, "no landing within 6 minutes - unloaded where it is"); x->next_try = now + 300000; if (!x->ever_ashore) x->active = 0; return; }
    if (cur && *(uintptr_t *)cur == GA(VT_GOAL_UNLOAD)) return;
    int x0 = FIELD(sh, OF(UNIT, 0x1C), int), y0 = FIELD(sh, OF(UNIT, 0x20), int), tx = x->ux, ty = x->uy, dx = x0 - x->ux, dy = y0 - x->uy;
    if (dx * dx + dy * dy <= 144) ex_shore(x->cont, x->ocean, x0, y0, 12, &tx, &ty, NULL, NULL);   // off the island: the nearest shore tile with room
    x->orders += unit_goal(sh, ld_unload(sh, tx, ty), 0, 0x1F40);
}

// THE OUTPOST'S WORK (every 2 s): the projects, builders, miners and the guard
struct ex_isle { ee_player *pl; unsigned cont; int nc, ns, nb; uint8_t *c[64], *s[32]; int foe; uint8_t *fu; int fd, fx, fy; };
static void ex_isle_unit(uint8_t *u, void *ctx) {
    struct ex_isle *e = ctx;
    uint8_t *td = FIELD(u, OF(UNIT, 0x2C), uint8_t *);
    ee_player *o = unit_owner(u);
    if (!o || !td || FIELD(u, OF(UNIT, 0x3C), int) <= 0 || FIELD(u, OF(UNIT, 0x1C), int) < 0) return;
    int x = FIELD(u, OF(UNIT, 0x1C), int), y = FIELD(u, OF(UNIT, 0x20), int);
    if (o == e->pl) {
        if (area_id(x, y, 2) != e->cont) return;
        int fam = FIELD(td, OF(TD, 0x4B4), int), t = FIELD(td, OF(TD, 0x4B8), int);
        if (fam == 2 || fam == 29) e->nb++;
        else if (is_citizen_type(t)) { if (e->nc < 64) e->c[e->nc++] = u; }
        else if (is_soldier(td) && !is_ship_or_air(td) && !is_transport(t) && e->ns < 32) e->s[e->ns++] = u;
        return;
    }
    if (FIELD(o, PL_INDEX, int) <= 0 || !is_enemy(e->pl, o) || !seen_by(u, e->pl)) return;   // enemies seen near the outpost
    if (!is_soldier(td) && !is_citizen_type(FIELD(td, OF(TD, 0x4B8), int)) && !is_siege(FIELD(td, OF(TD, 0x4B8), int))) return;
    int d = (x - e->fx) * (x - e->fx) + (y - e->fy) * (y - e->fy);
    if (d > 14 * 14 || area_id(x, y, 2) != e->cont) return;
    e->foe++;
    if (!e->fu || d < e->fd) { e->fu = u; e->fd = d; }
}
static int ex_kind_type(ee_player *pl, int kind) {
    if (kind == XK_TOWER) return ld_kind_type(pl, LDB_TOWER);
    if (kind == XK_BARRACKS) return ld_can_build(pl, UT(B_BARRACKS)) ? UT(B_BARRACKS) : 0;
    return ld_can_build(pl, UT(B_SETTLEMENT)) ? UT(B_SETTLEMENT) : 0;   // a Settlement (a Town Center first took the stone of the towers)
}
static int ex_spot(ee_player *pl, unsigned idx, struct ex_st *x, int type, int cx, int cy, int *ox, int *oy) {
    uint8_t *td = type_data(type);
    int sz = td ? FIELD(td, OF(TD, 0x58C), int) : 1, found = 0;
    float best = 1e9f;
    if (sz < 1) sz = 1;
    for (int d = 1; d <= 12; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d ? 1 : 2 * d)) {
                int px = cx + dx, py = cy + dy, ok = area_id(px, py, 2) == x->cont && !area_id(px, py, 1);
                for (int k = 0; k < x->nproj && ok; k++)   // a way between its buildings
                    if (x->proj[k].state && x->proj[k].state != 3 && abs(x->proj[k].x - px) < sz + 3 && abs(x->proj[k].y - py) < sz + 3) ok = 0;
                if (!ok || !can_place(pl, type, px, py)) continue;
                float sc = (float)d + 10.0f * threat_at(idx, px, py);
                if (sc < best) { best = sc; *ox = px; *oy = py; found = 1; }
            }
    return found;
}
static int ex_proj_alive(struct ex_proj *p) {
    uint8_t *b = p->b, *td = b ? FIELD(b, OF(UNIT, 0x2C), uint8_t *) : NULL;
    return td && FIELD(b, OF(UNIT, 0x3C), int) > 0 && FIELD(b, OF(UNIT, 0x60), int) == p->id;
}
// the free mine of the island nearest to (x, y): its gatherers below the cap
static uint8_t *ex_free_mine(ee_player *pl, struct ex_st *x, int px, int py) {
    unsigned cap = GF(FN_TABLE_VALUE)((void *)GA(G_TABLE_VALUES), 0x16);
    uint8_t *best = NULL;
    int bd = 0x7FFFFFFF;
    void **zb = FIELD(pl, OF(PL, 0xB50), void **), **ze = FIELD(pl, OF(PL, 0xB54), void **);
    for (int z = 0; zb && zb + z < ze && z < 256; z++) {
        uint8_t *zone = zb[z];
        void **sb = zone ? FIELD(zone, OF(ZONE, 0x8), void **) : NULL, **se = zone ? FIELD(zone, OF(ZONE, 0xC), void **) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096; k++) {
            uint8_t *s = sb[k];
            int r = s ? FIELD(s, OF(UNIT, 0x38), int) : 0;
            if (r < 4 || r > 6 || FIELD(s, OF(UNIT, 0x40), int) <= 0) continue;
            int sx = FIELD(s, OF(UNIT, 0x1C), int), sy = FIELD(s, OF(UNIT, 0x20), int), d = (sx - px) * (sx - px) + (sy - py) * (sy - py);
            if (d >= bd || area_id(sx, sy, 2) != x->cont) continue;
            if (GF(FN_SITE_GATHERERS)((void *)GA(G_GAME_STATE), s) >= cap) continue;
            best = s; bd = d;
        }
    }
    return best;
}
// SEA CONTROL as the AI knows it (honest_vision: intel.c's memory): no enemy warship seen in its own waters (navy.c
// navy_waters - the waters of its nearest building) within 5 minutes, and its warships at least the enemy's it saw
// within 10 minutes
static int sea_held(ee_player *pl, unsigned idx, int *mine, int *theirs, int *inw) {
    struct intel *in = &g_intel[idx & 15];
    int foe = 0, n = 0, own = 0;
    for (int k = 0; k < in->n; k++) {
        struct intel_rec *e = &in->r[k];
        if (e->kind != 2 || in->now - e->t > 600000) continue;
        uint8_t *td = type_data(e->type);
        int nk = td ? navy_kind(td) : NV_NONE;
        if (nk != NV_WAR && nk != NV_SUB) continue;
        n++;
        if (e->x >= 0 && in->now - e->t <= 300000 && navy_waters(e->x, e->y) == (int)idx) foe++;
    }
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        if (unit_owner(it.u) != pl || FIELD(it.u, OF(UNIT, 0x3C), int) <= 0) continue;
        int nk = navy_kind(it.td);
        own += nk == NV_WAR || nk == NV_SUB;
    }
    if (mine) *mine = own;
    if (theirs) *theirs = n;
    if (inw) *inw = foe;
    return !foe && own >= n;
}
// FORWARD: the outpost nearer the enemy's known buildings than its home - a barracks and two more towers there, while
// it holds the sea (once; a project that waits for the sea is laid when it holds it again)
static void ex_forward(ee_player *pl, unsigned idx, struct ex_st *x, uint32_t now) {
    if (x->fwd_added) return;
    if (!x->forward) {
        struct intel *in = &g_intel[idx & 15];
        uint8_t *cm = FIELD(pl, OF(PL, 0xB90), uint8_t *), *bm = cm ? FIELD(cm, OF(CM, 0x3C), uint8_t *) : NULL;
        if (!bm) return;
        int bx = FIELD(bm, OF(BM, 0x12C), int), by = FIELD(bm, OF(BM, 0x130), int);
        float dh = 1e9f, dp = 1e9f;   // the enemy's known buildings nearest to home / to the outpost
        for (int k = 0; k < in->n; k++) {
            struct intel_rec *e = &in->r[k];
            if (e->kind != 0 || e->x < 0) continue;
            float a = (float)((e->x - bx) * (e->x - bx) + (e->y - by) * (e->y - by)), b = (float)((e->x - x->mx) * (e->x - x->mx) + (e->y - x->my) * (e->y - x->my));
            if (a < dh) dh = a;
            if (b < dp) dp = b;
        }
        if (dp >= dh) return;
        x->forward = 1; x->forwards++;
        ex_say(idx, now, "the outpost on the island %u is nearer the enemy (%d tiles) than home (%d) - a forward base once it holds the sea",
               x->cont, (int)sqrtf(dp), (int)sqrtf(dh));
    }
    int mine = 0, theirs = 0, inw = 0;
    if (!sea_held(pl, idx, &mine, &theirs, &inw)) {
        if (!x->sea_no || now - x->sea_no >= 300000) {
            x->sea_no = now ? now : 1;
            ex_say(idx, now, "forward base waits - the sea not held (warships %d, the enemy's known %d, %d seen in its waters)", mine, theirs, inw);
        }
        return;
    }
    static const int add[] = { XK_BARRACKS, XK_TOWER, XK_TOWER };
    for (int k = 0; k < 3 && x->nproj < EX_PROJ; k++) {
        if (add[k] == XK_BARRACKS && (EXPAND(idx) & 2)) continue;   // BARRACKS has one already
        memset(&x->proj[x->nproj], 0, sizeof x->proj[0]);
        x->proj[x->nproj++].kind = add[k];
    }
    x->fwd_added = 1;
    ex_say(idx, now, "forward base on the island %u: a barracks and two towers (warships %d, the enemy's known %d)", x->cont, mine, theirs);
}
// MORE: the outpost holds - its Settlement and towers stand (or were dropped), no enemy there for 3 minutes, its trips
// made or its slots filled: left to the game's economy, the next island planned
static int ex_holds(ee_player *pl, unsigned idx, struct ex_st *x, uint32_t now) {
    int slots = 0;
    for (int j = 0; j < g_exs.n; j++) if (g_exs.c[j].id == x->cont) slots = g_exs.c[j].slots;
    for (int j = 0; j < x->nproj; j++) if (x->proj[j].state != 2 && x->proj[j].state != 3) return 0;
    if ((x->foe_t && now - x->foe_t < 180000) || x->fphase || x->ncit < 3 || (x->trips < g_ex_trips && x->ncit + 3 <= slots)) return 0;
    if (x->nheld >= 4) return 0;
    int dx0 = x->mx, dy0 = x->my;
    for (int j = 0; j < x->nproj; j++) if (x->proj[j].kind == XK_DROP && x->proj[j].state == 2) { dx0 = x->proj[j].x; dy0 = x->proj[j].y; }
    x->held[x->nheld].cont = x->cont; x->held[x->nheld].x = dx0; x->held[x->nheld++].y = dy0;
    x->active = 0; x->graduated++; x->next_try = now + 60000; x->nguard = 0; x->forward = x->fwd_added = 0;
    ex_say(idx, now, "the island %u holds (%d citizens, %d soldiers, %d buildings laid) - left to the game, %d held now", x->cont, x->ncit, x->nsold,
           x->nproj, x->nheld);
    (void)pl;
    return 1;
}
static void ex_outpost(ee_player *pl, unsigned idx, struct ex_st *x, uint32_t now) {
    int dx0 = x->mx, dy0 = x->my;   // the outpost's middle: its drop site once laid
    for (int j = 0; j < x->nproj; j++) if (x->proj[j].kind == XK_DROP && x->proj[j].state && x->proj[j].state != 3) { dx0 = x->proj[j].x; dy0 = x->proj[j].y; }
    static struct ex_isle e;
    memset(&e, 0, sizeof e);
    e.pl = pl; e.cont = x->cont; e.fx = dx0; e.fy = dy0;
    for (struct grid_it it = grid_it_box(dx0, dy0, 40, 1); grid_it_next(&it); ) ex_isle_unit(it.u, &e);
    x->ncit = e.nc; x->nsold = e.ns;
    if (e.foe) x->foe_t = now ? now : 1;
    // lost: nothing of it on the island any more (and no ferry on its way)
    if (x->ever_ashore && !e.nc && !e.nb && x->fphase != 3) {
        if (!x->empty_t) x->empty_t = now ? now : 1;
        if (now - x->empty_t >= 20000) {
            int n = ex_mark_lost(x, x->cont, now);
            x->lost++; x->active = 0;
            if (x->fphase) ex_ferry_end(pl, idx, x, now, NULL);
            ex_say(idx, now, "outpost lost on the island %u (%d times) - %s", x->cont, n, n >= 3 ? "given up" : "again in 8 minutes");
            if (n >= 3) x->given++;
            x->next_try = now + 60000;
            return;
        }
        return;
    }
    x->empty_t = 0;
    if (!e.nc && !e.ns) return;
    // the projects: going up -> standing or destroyed; standing -> destroyed: laid again (3 times)
    int drop_ok = 0;
    for (int j = 0; j < x->nproj; j++) {
        struct ex_proj *p = &x->proj[j];
        if (p->state == 1 || p->state == 2) {
            uint8_t *bs = ex_proj_alive(p) ? FIELD(p->b, OF(UNIT, 0x30), uint8_t *) : NULL;
            if (!ex_proj_alive(p)) {
                ex_say(idx, now, "%s %d at %d,%d destroyed%s", k_xk[p->kind], p->type, p->x, p->y, p->tries < 3 ? " - laid again" : "");
                p->b = NULL; p->state = p->tries < 3 ? 0 : 3;
                continue;
            }
            if (p->state == 1 && bs && bs[OF(UNIT, 0x340)]) {
                p->state = 2;
                if (p->kind == XK_DROP) x->drops++;
                x->built++;
                ex_say(idx, now, "%s %d at %d,%d finished", k_xk[p->kind], p->type, p->x, p->y);
            }
        }
        if (p->kind == XK_DROP && p->state == 2) drop_ok = 1;
    }
    x->drop_ok = drop_ok;
    if (drop_ok && (EXPAND(idx) & 64)) ex_forward(pl, idx, x, now);
    if (drop_ok && (EXPAND(idx) & 32) && ex_holds(pl, idx, x, now)) return;
    {   int laid = 0;   // the drop site and the first tower laid: no more reserve for them
        for (int j = 0; j < x->nproj; j++) laid += x->proj[j].state && x->proj[j].kind != XK_BARRACKS;
        x->res_t = laid >= 2 ? 0 : now ? now : 1; }
    // the next project (every 8 s): in order, with its spot; one it can't pay waits (the drop site always)
    if (e.nc && (!x->plan_t || now - x->plan_t >= 8000)) {
        x->plan_t = now ? now : 1;
        int going = 0;
        for (int j = 0; j < x->nproj; j++) going += x->proj[j].state == 1;
        for (int j = 0; j < x->nproj && going < 2; j++) {
            struct ex_proj *p = &x->proj[j];
            if (p->state) continue;
            int t = ex_kind_type(pl, p->kind), px = 0, py = 0;
            if (!t) {
                if (p->kind != XK_DROP && ++p->waits > 30) { p->state = 3; ex_say(idx, now, "no %s it can pay for 4 minutes - dropped", k_xk[p->kind]); }
                break;
            }
            int cx = x->mx, cy = x->my;
            if (p->kind == XK_TOWER) {   // between the mines and the sea (the first), else by the drop site
                int first = 1;
                for (int k = 0; k < j; k++) first &= x->proj[k].kind != XK_TOWER;
                if (first) { cx = (x->mx + x->ux) / 2; cy = (x->my + x->uy) / 2; } else { cx = dx0; cy = dy0; }
            } else if (p->kind == XK_BARRACKS) { cx = dx0; cy = dy0; }
            if (!ex_spot(pl, idx, x, t, cx, cy, &px, &py)) { if (++p->waits > 30 && p->kind != XK_DROP) p->state = 3; break; }
            uint8_t *b = GF(FN_PLACE_BUILDING)(pl, t, px, py);
            if (!b) { if (++p->waits > 30 && p->kind != XK_DROP) p->state = 3; break; }
            p->b = b; p->id = FIELD(b, OF(UNIT, 0x60), int); p->type = t; p->x = px; p->y = py; p->state = 1; p->tries++;
            ex_say(idx, now, "%s %d laid at %d,%d (%d citizens, %d soldiers on the island)", k_xk[p->kind], t, px, py, e.nc, e.ns);
            going++;
            if (p->kind == XK_DROP) break;   // the drop site alone first
        }
    }
    // builders: 3 on the drop site (all while it is not finished), 2 on another; the rest gather once the drop site stands
    int want[EX_PROJ] = { 0 }, have[EX_PROJ] = { 0 }, nb = 0;
    for (int j = 0; j < x->nproj; j++) if (x->proj[j].state == 1) want[j] = x->proj[j].kind == XK_DROP ? (drop_ok ? 3 : 64) : 2;
    ee_resource_mgr *rm = FIELD(FIELD(pl, OF(PL, 0xB90), uint8_t *), OF(CM, 0x38), ee_resource_mgr *);
    for (int k = 0; k < e.nc; k++) {
        uint8_t *u = e.c[k], *g = FIELD(u, OF(UNIT, 0x1EC), uint8_t *);
        const char *gn = g ? rtti_name(g) : "";
        int on = -1;
        if (!strcmp(gn, "EEUGBuildRepair@@"))
            for (int j = 0; j < x->nproj; j++) if (x->proj[j].state == 1 && FIELD(g, OF(GOAL, 0x3C), uint8_t *) == x->proj[j].b) on = j;
        if (on >= 0 && have[on] < want[on]) { have[on]++; continue; }
        int j = -1;
        for (int q = 0; q < x->nproj; q++) if (x->proj[q].state == 1 && have[q] < want[q] && (j < 0 || x->proj[q].kind == XK_DROP)) j = q;
        int gathering = !strcmp(gn, "EEUGGather@@");
        if (j >= 0 && (!gathering || !drop_ok)) {
            void *bg = GF(FN_NEW)(GA(SIZE_GOAL_BUILD));
            if (bg) bg = GF(FN_BUILD_GOAL)(bg, pl, x->proj[j].b, 0);
            if (bg && unit_goal(u, bg, 0, 0x1F51)) { have[j]++; nb++; x->orders++; }
            continue;
        }
        if (gathering || !drop_ok || !rm) continue;
        if (g && (*(uintptr_t *)g == GA(VT_GOAL_LOCATION) || *(uintptr_t *)g == GA(VT_GOAL_ATTACK) || !strcmp(gn, "EEUGBuildRepair@@"))) continue;   // once it stands still
        uint8_t *s = ex_free_mine(pl, x, dx0, dy0);
        if (!s) continue;
        GF(FN_ASSIGN_TO_SITE)(rm, u, s, 0);
        x->assigned++;
    }
    // the guard: by the drop site, attacking what it sees within 14 tiles
    for (int k = 0; k < e.ns; k++) {
        uint8_t *u = e.s[k];
        if (as_fighting(u)) continue;
        if (e.fu) { x->orders += unit_goal(u, goal_attack(u, e.fu), 0, 0x1F40); continue; }
        int ddx = FIELD(u, OF(UNIT, 0x1C), int) - dx0, ddy = FIELD(u, OF(UNIT, 0x20), int) - dy0;
        if (ddx * ddx + ddy * ddy > 64 && !as_goal_near(u, (float)dx0 + 0.5f, (float)dy0 + 2.5f, 3.0f))
            x->orders += unit_goal(u, goal_move((float)dx0 + 0.5f, (float)dy0 + 2.5f), 0, 0x1F40);
    }
    (void)nb;
}
// the enemy army it knows (intel.c: seen within 10 minutes) stronger than its own land army (strength: sqrt(hit points x
// attack), landing.c) - with the cheat the base AI's waves took the citizens shipped off and the home they left
static int ex_outmatched(ee_player *pl, float *mine, float *theirs) {
    float eh = 0, ea = 0, oh = 0, oa = 0;
    int en = 0;
    intel_army(pl, &eh, &ea, NULL, &en);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        uint8_t *u = it.u;
        if (unit_owner(u) != pl || FIELD(u, OF(UNIT, 0x3C), int) <= 0 || FIELD(u, OF(UNIT, 0x1C), int) < 0 || !land_soldier(FIELD(it.td, OF(TD, 0x4B8), int))) continue;
        oh += (float)FIELD(u, OF(UNIT, 0x3C), int); oa += ld_attack(u);
    }
    *mine = ld_strength(oh, oa); *theirs = ld_strength(eh, ea);
    return *theirs > *mine;
}
// an enemy town (Town Center, Capitol) seen on the outpost's island - an enemy's home, not a free island: the ferry
// back home (unloaded at the home shore), the outpost off, that island never again
static void ex_enemy_home(ee_player *pl, unsigned idx, struct ex_st *x, uint32_t now) {
    ex_scan(pl, now, 0);
    struct ex_cont *c = ex_cont_of(x->cont, 0);
    if (!c || !c->etown[idx]) return;
    while (ex_mark_lost(x, x->cont, now) < 3 && x->ngone < 8) ;
    if (x->fphase == 3) x->fphase = 2;   // unloaded at the home shore, not on that island
    if (x->fphase) ex_ferry_end(pl, idx, x, now, NULL);
    x->active = 0; x->given++; x->next_try = now + 60000;
    ex_say(idx, now, "an enemy town seen on the island %u - an enemy's home: outpost off, never again", x->cont);
}
// FOLLOW: why not now (NULL: the sea held, the army 1.5 x the enemy's it knows)
static const char *ex_follow_no(ee_player *pl, unsigned idx, float mine, float theirs) {
    if (!(EXPAND(idx) & 128)) return NULL;
    int a, b, c;
    if (!sea_held(pl, idx, &a, &b, &c)) return "the sea not held (FOLLOW)";
    if (mine < 1.5f * theirs) return "its army not 1.5 x the enemy's it knows (FOLLOW)";
    return NULL;
}
// a new outpost (phase 0, every 30 s) or more citizens for it
static void ex_plan(ee_player *pl, unsigned idx, struct ex_st *x, uint32_t now) {
    static const char *last_why[16];
    const char *why = NULL;
    uint8_t *cm = FIELD(pl, OF(PL, 0xB90), uint8_t *), *bm = cm ? FIELD(cm, OF(CM, 0x3C), uint8_t *) : NULL;
    int bx = bm ? FIELD(bm, OF(BM, 0x12C), int) : -10000, by = bm ? FIELD(bm, OF(BM, 0x130), int) : -10000;
    unsigned hc = (unsigned)ex_home(pl);
    if (x->active) {   // more citizens: the drop site stands, no Town Center of its own there, slots left, trips left
        if (!x->drop_ok || x->trips >= g_ex_trips || (x->trip_t && now - x->trip_t < 120000) || (x->foe_t && now - x->foe_t < 60000)) return;   // not into a fight
        ex_scan(pl, now, 0);
        int slots = 0;
        for (int j = 0; j < g_exs.n; j++) if (g_exs.c[j].id == x->cont) slots = g_exs.c[j].slots;
        float mine, theirs;
        if (x->ncit + 3 > slots || ex_outmatched(pl, &mine, &theirs) || ex_follow_no(pl, idx, mine, theirs)) return;
        x->guard_trip = x->nsold < EX_GUARD(idx);
        x->fphase = 1; x->fsince = now; x->ftrain_t = 0; x->trips++;
        ex_say(idx, now, "more citizens for the island %u (%d there, %d mine slots, %d soldiers)", x->cont, x->ncit, slots, x->nsold);
        return;
    }
    if (bx == -10000 || !hc) return;
    if (EXPAND(idx) & 32) {   // MORE: a held island with nothing of it left is lost
        ex_scan(pl, now, 0);
        for (int k = 0; k < x->nheld; k++) {
            struct ex_cont *hcn = ex_cont_of(x->held[k].cont, 0);
            if (hcn && (hcn->bld[idx] || hcn->cit[idx])) continue;
            int n = ex_mark_lost(x, x->held[k].cont, now);
            x->held_lost++;
            ex_say(idx, now, "the held island %u lost (%d times)", x->held[k].cont, n);
            x->held[k] = x->held[--x->nheld]; k--;
        }
        if (x->nheld >= g_ex_max) why = "holds expand_max islands";
    }
    if (why) ;
    else if (!g_nv_map[idx].done || g_nv_map[idx].kind == NV_LAND) why = "no water map";
    else if (!g_landing) why = "landing off (its transport hooks)";
    else if (current_epoch(pl) < FARM_EPOCH || now < (uint32_t)g_ex_min * 60000u) why = "too early";
    if (!why) {
        ex_scan(pl, now, 0);
        struct ex_cont *h = ex_cont_of(hc, 0);
        unsigned cap = GF(FN_TABLE_VALUE)((void *)GA(G_TABLE_VALUES), 0x16);
        int slots = h ? (h->site[4] + h->site[5] + h->site[6]) * (int)cap : 0, miners = h ? h->gat[idx][4] + h->gat[idx][5] + h->gat[idx][6] : 0;
        int early = EXPAND(idx) & 8;
        if (!early && now < 900000 && miners * 2 < slots) why = "the home island's mines first";
        else if (workforce(pl) < (early ? 20 : 25)) why = early ? "fewer than 20 workers" : "fewer than 25 workers";
        else { float mine, theirs; if (ex_outmatched(pl, &mine, &theirs)) why = "the enemy army it knows is stronger than its own";
               else if (ex_follow_no(pl, idx, mine, theirs)) why = ex_follow_no(pl, idx, mine, theirs); }
    }
    // its sea: its docks'
    int ocean = 0;
    if (!why) {
        for (struct grid_it it = grid_it_all(1); grid_it_next(&it) && !ocean; )
            if (unit_owner(it.u) == pl && FIELD(it.td, OF(TD, 0x4B8), int) == UT(B_DOCK) && FIELD(it.u, OF(UNIT, 0x3C), int) > 0)
                ocean = (int)ocean_near(FIELD(it.u, OF(UNIT, 0x1C), int), FIELD(it.u, OF(UNIT, 0x20), int), 4, NULL, NULL);
        if (!ocean) why = "no dock";
    }
    unsigned cont = 0;
    if (!why) cont = ex_pick(pl, idx, x, hc, ocean, bx, by, now, &why);
    struct ex_cont *c = cont ? ex_cont_of(cont, 0) : NULL;
    if (!why && !c) why = "no island";
    int mx = 0, my = 0;
    if (!why) {
        mx = c->msx / c->mn; my = c->msy / c->mn;
        if (!ex_shore(cont, ocean, mx, my, 30, &x->ux, &x->uy, &x->uwx, &x->uwy)) why = "no shore of its sea on that island";
    }
    if (!why) {   // the home shore: near the base, towards the island
        int best = 0x7FFFFFFF, found = 0;
        for (int d = 0; d <= 40; d += 2)
            for (int ddx = -d; ddx <= d; ddx += 2)
                for (int ddy = -d; ddy <= d; ddy += (abs(ddx) == d || !d ? 2 : 2 * d)) {
                    int tx = bx + ddx, ty = by + ddy, wx, wy;
                    if (area_id(tx, ty, 2) != hc || area_id(tx, ty, 1) || (int)ocean_near(tx, ty, 2, &wx, &wy) != ocean || !ld_tile_free(tx, ty)) continue;
                    int sc = (int)sqrtf((float)(ddx * ddx + ddy * ddy)) * 2 + (int)sqrtf((float)((tx - x->ux) * (tx - x->ux) + (ty - x->uy) * (ty - x->uy)));
                    if (sc < best) { best = sc; x->hx = tx; x->hy = ty; x->hwx = wx; x->hwy = wy; found = 1; }
                }
        if (!found) why = "no shore of its sea at home";
    }
    if (why) {
        if (g_log && (why != last_why[idx] || !x->log_t || now - x->log_t >= 300000)) { x->log_t = now ? now : 1; ex_say(idx, now, "none planned - %s", why); }
        last_why[idx] = why;
        return;
    }
    last_why[idx] = NULL;
    x->active = 1; x->cont = cont; x->ocean = ocean; x->mx = mx; x->my = my;
    x->ever_ashore = 0; x->trips = 1; x->drop_ok = 0; x->empty_t = 0; x->base_t = 0; x->plan_t = 0; x->ncit = x->nsold = 0;
    x->nguard = 0; x->forward = x->fwd_added = 0; x->sea_no = 0;
    memset(x->proj, 0, sizeof x->proj);
    static const int plan[] = { XK_DROP, XK_TOWER, XK_TOWER, XK_BARRACKS }, ring[] = { XK_DROP, XK_TOWER, XK_TOWER, XK_TOWER, XK_TOWER, XK_BARRACKS };
    int fol = (EXPAND(idx) & 128) != 0;   // FOLLOW: a ring of four towers
    x->nproj = (fol ? 5 : 3) + ((EXPAND(idx) & 2) != 0);
    for (int j = 0; j < x->nproj; j++) x->proj[j].kind = fol ? ring[j] : plan[j];
    x->guard_trip = 1;
    x->fphase = 1; x->fsince = now; x->ftrain_t = 0; x->plans++; x->res_t = now ? now : 1;
    ex_say(idx, now, "plans an outpost on the island %u (%d mines explored, %d known enemy buildings, mines by %d,%d) - unload at %d,%d, board at %d,%d",
           cont, c->mexp[idx], c->eseen[idx], mx, my, x->ux, x->uy, x->hx, x->hy);
}
static void expand_act(ee_player *pl, unsigned idx, uint32_t now) {
    struct ex_st *x = &g_xs[idx];
    if (now < x->t) { memset(x, 0, sizeof *x); }   // a new game (also expand_new_game)
    if (x->t && now - x->t < 2000) return;
    x->t = now ? now : 1;
    if (!EXPAND(idx)) { if (x->fphase) ex_ferry_end(pl, idx, x, now, NULL); x->active = 0; return; }
    if (x->fphase >= 2 && !ex_alive(&x->ship, pl)) {
        int aboard = 0;
        for (int k = 0; k < x->npass; k++) aboard += x->pass[k].u && FIELD(x->pass[k].u, OF(UNIT, 0x3C), int) > 0 && FIELD(x->pass[k].u, OF(UNIT, 0x1C), int) < 0;
        ex_say(idx, now, "the transport lost (%s)", x->fphase == 3 ? "at sea" : x->fphase == 4 ? "scouting" : "boarding");
        x->fphase = 0; x->npass = 0; x->ship.u = NULL; x->next_try = now + 300000;
        if (!x->ever_ashore) { ex_mark_lost(x, x->cont, now); x->active = 0; x->lost++; }
        (void)aboard;
    }
    if (x->active) ex_enemy_home(pl, idx, x, now);
    if (x->fphase == 1) ex_ferry_ship(pl, idx, x, now);
    else if (x->fphase == 2) ex_ferry_board(pl, idx, x, now);
    else if (x->fphase == 3) ex_ferry_sail(pl, idx, x, now);
    else if (x->fphase == 4) ex_ferry_scout(pl, idx, x, now);
    if (x->active && x->ever_ashore) ex_outpost(pl, idx, x, now);
    if (!x->fphase && now >= x->next_try && (!x->plan_t || x->active || now - x->plan_t >= 30000)) {
        if (!x->active) x->plan_t = now ? now : 1;
        ex_plan(pl, idx, x, now);
    }
    if (g_autotest && g_log && (!x->log_t || now / 300000 != x->log_t / 300000) && x->plans) {   // (log_t is shared with ex_plan's why lines)
        x->log_t = now ? now : 1;
        ex_say(idx, now, "status - %s island %u, ferry %d, citizens %d soldiers %d there, drop site %d | plans %u sails %u ashore %u drop sites %u built %u lost %u "
               "assigned %u transports trained %u orders %u | held %d (graduated %u, lost %u) forward %u%s | scouted %u (enemy %u)", x->active ? "outpost on" : "no outpost, last", x->cont, x->fphase, x->ncit, x->nsold, x->drop_ok,
               x->plans, x->sails, x->ashore, x->drops, x->built, x->lost, x->assigned, x->trained, x->orders, x->nheld, x->graduated, x->held_lost,
               x->forwards, x->fwd_added ? " (base added)" : "", x->scouts, x->scout_bad);
    }
}

// EXTRA: the citizens an outpost still needs before its Settlement stands (citizen_plan's goal, citizens.c) - then the
// mines' slots near it count, as every drop site's
static int expand_cit_need(ee_player *pl, unsigned idx) {
    struct ex_st *x = &g_xs[idx & 15];
    (void)pl;
    if (!(EXPAND(idx) & 16) || !x->active || x->drop_ok) return 0;
    int n = g_ex_cits - x->ncit;
    return n > 0 ? n : 0;
}
static void expand_new_game(void) {
    memset(&g_exs, 0, sizeof g_exs);
    memset(g_ex_log_t, 0, sizeof g_ex_log_t);
    memset(g_xs, 0, sizeof g_xs);
    g_ex_map_logged = 0;
    g_ex_nseen = 0; g_ex_new_t = 0;
}
static void expand_tick(ee_player *pl, uint32_t now) {
    if (!pl) return;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (now < g_ex_log_t[idx]) g_ex_log_t[idx] = 0;   // a new game (also expand_new_game)
    if (now < g_ex_new_t) g_ex_new_t = 0;
    if (g_autotest && g_log && (!g_ex_new_t || now - g_ex_new_t >= 60000)) { g_ex_new_t = now ? now : 1; ex_scan(pl, now, 1); }   // once a game minute: # isl new
    if (g_autotest && g_log && now >= 60000 && (!g_ex_log_t[idx] || now / 300000 != g_ex_log_t[idx] / 300000)) {
        g_ex_log_t[idx] = now ? now : 1;
        ex_log(pl, idx, now);
    }
    if (!is_vanilla(pl) && (g_expand || g_v_expand > 0)) expand_act(pl, idx, now);
}
static void expand_install(const char *aimod_ini, const char *autotest_ini) {
    g_expand = cfg_int(aimod_ini, autotest_ini, 1, "expand_islands", 0);
    if (autotest_ini) g_v_expand = ini_int("autotest", "v_expand_islands", -1, autotest_ini);
    g_ex_cits = cfg_int(aimod_ini, autotest_ini, 1, "expand_citizens", 8);
    g_ex_guard = cfg_int(aimod_ini, autotest_ini, 1, "expand_guard", 3);
    g_ex_min = cfg_int(aimod_ini, autotest_ini, 1, "expand_minute", 8);
    g_ex_trips = cfg_int(aimod_ini, autotest_ini, 1, "expand_trips", 4);
    g_ex_max = cfg_int(aimod_ini, autotest_ini, 1, "expand_max", 3);
    if (g_ex_max < 1) g_ex_max = 1;
    if (g_ex_max > 4) g_ex_max = 4;
    if (g_ex_cits < 3) g_ex_cits = 3;
    if (g_ex_cits > 12) g_ex_cits = 12;
    if (g_ex_guard < 0) g_ex_guard = 0;
    if (g_ex_guard > 4) g_ex_guard = 4;
    if (g_log) fprintf(g_log, "# expand: expand_islands %d (variant %d), %d citizens, guard %d, from minute %d, %d trips, at most %d islands\n", g_expand,
                       g_v_expand, g_ex_cits, g_ex_guard, g_ex_min, g_ex_trips, g_ex_max);
}
