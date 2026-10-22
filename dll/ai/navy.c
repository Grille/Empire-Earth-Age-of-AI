// NAVY (docs/navy.md): what each AI has afloat, who holds which waters, the food from fish, the ships lost - the
// baseline for a standing navy. Decisions changed: the fishing boat type (stage 0, hook_fish_type), the navy account,
// docks and fleet room (stage 1), the warship types (stage 2, the fleet mix: navy_pick).
//   # navy: (autotest only, every 2 game minutes per AI) docks, fishing boats, warships by type, submarines, carriers,
//     transports, the warships in its own waters / an enemy's waters, the enemy's warships in its waters, the ships
//     it lost and sank (diplo.c kill counts), the food its boats took from the fish, its income and stock.
//   navy types A B [all] / navy table (test commands): the ship types' data and the game's hit factor table.
// Included by eeaimod.c after diplo.c (g_kills, player_at), landing.c (is_transport, is_fishing_boat), econ.c (g_bud).

// The type families (+0x4B4, diplo.c kill_kind): 4 ships, 10 submarines, 18 transports, 21 carriers, 22 fishing boats.
// Transports and fishing boats also by their type ids (landing.c), in case a family differs.
enum { NV_NONE, NV_WAR, NV_SUB, NV_CARRIER, NV_TRANSPORT, NV_FISH };
internal int navy_kind(uint8_t *td) {
    int fam = FIELD(td, OF(TD, 0x4B4), int), t = FIELD(td, OF(TD, 0x4B8), int);
    if (fam == 22 || is_fishing_boat(t)) return NV_FISH;
    if (fam == 18 || is_transport(t)) return NV_TRANSPORT;
    if (fam == 10) return NV_SUB;
    if (fam == 21) return NV_CARRIER;
    if (fam == 4) return FIELD(td, OF(TD, 0x510), int) > 0 ? NV_WAR : NV_NONE;
    return NV_NONE;
}

// FOOD FROM FISH: the fish (res_fish, nature's; resource amount +0x40) as they stand every 15 s; what a fish lost
// since the last look goes to the owner of the fishing boat gathering it (its goal +0x38 = the site, compared as a
// pointer only), or to its last gatherer; a fish gone (eaten up) gives its last amount to its last gatherer.
#define NV_FISH_MAX 1024
internal struct { ee_unit *u; int id, amount, owner; } g_nv_fish[NV_FISH_MAX];
internal int g_nv_nfish, g_nv_fish_food[16], g_nv_fish_left;
internal uint32_t g_nv_fish_t, g_nv_log_t;
internal int g_fish_last[16];   // the fishing boat last picked per player (hook_fish_type, its log line)
struct nv_map { uint32_t t; uint8_t done, kind; float strength, water; int across, enemies; unsigned home; };   // navy_detect (home: its continent)
internal struct nv_map g_nv_map[16];
// the navy account (navy_income) per AI and resource; saved with the game (persist.c)
internal float g_nv_acc[16][7];
// what the account paid and refused (the budget line): ship orders by type
struct nv_orders { unsigned paid, refused, spent[7], held_room, docks_asked; int want, last, nt; uint32_t want_t, dock_t; struct { int type; unsigned paid, refused; } t[8]; };
internal struct nv_orders g_nv_ord[16];
// own ships and docks, counted for every player every 10 s (navy_count)
struct nv_own { int war, docks, docks_built, fish, role[6]; };   // role: warships by the fleet mix's roles (NR_*)
internal struct nv_own g_nv_own[16];
internal uint32_t g_nv_count_t;

internal void nv_mix_reset(void);   // the fleet mix (below)
internal void fleet_new_game(void);   // fleet.c (stage 3)
internal void fleet_install(const char *aimod_ini, const char *autotest_ini);
internal void fleet_tick(ee_player *pl, uint32_t now);
internal int nv_role(uint8_t *td);
internal int fleet_sheltering(unsigned idx);   // fleet.c: the fleet shelters from a much stronger enemy fleet (no build-up then)
internal int fleet_builds(unsigned idx);   // fleet.c: stage 4 (navy_fleet=2) for this AI - the fleet's size target below
// THE FLEET'S SIZE (stage 4, navy_fleet=2; docs/navy.md): the playtester expects 20+ warships at a low population
// limit and more with higher ones - navy_fleet_pop % (15) of the AI's population cap x the map strength (cap 176 in
// the 2-AI island tests: 26; cap 135: 20). Below it the account's share grows toward navy_share_max (50) with what is
// missing, the room held for the fleet and the docks wanted follow it, and fleet.c trains the missing warships itself.
internal int g_nv_target[16], g_navy_fleet_pop = 15, g_navy_share_max = 50;
// the resource nodes the AI knows (navy_nodes, below)
struct nv_nodes { int n[7], worked[7], g[7], slots[7], near_slots[7], remote[7], other[7]; int fx[7], fy[7]; ee_unit *free_far[7]; uint32_t t; };
internal struct nv_nodes g_nv_nodes[16];
struct nv_mine { int pull, want[7], given; uint32_t t; };   // navy_mines (below)
internal struct nv_mine g_nv_mine[16];

internal void navy_new_game(void) {
    memset(g_nv_fish, 0, sizeof g_nv_fish);
    memset(g_nv_fish_food, 0, sizeof g_nv_fish_food);
    memset(g_nv_boat_kills, 0, sizeof g_nv_boat_kills);
    g_nv_nfish = g_nv_fish_left = 0;
    g_nv_fish_t = g_nv_log_t = 0;
    memset(g_fish_last, 0, sizeof g_fish_last);
    memset(g_nv_map, 0, sizeof g_nv_map);
    memset(g_nv_acc, 0, sizeof g_nv_acc);
    memset(g_nv_ord, 0, sizeof g_nv_ord);
    memset(g_nv_own, 0, sizeof g_nv_own);
    g_nv_count_t = 0;
    memset(g_nv_target, 0, sizeof g_nv_target);
    memset(g_nv_nodes, 0, sizeof g_nv_nodes);
    memset(g_nv_mine, 0, sizeof g_nv_mine);
    nv_mix_reset();
    fleet_new_game();
    memset(g_nv_lost_type, 0, sizeof g_nv_lost_type);
    memset(g_nv_sunk_type, 0, sizeof g_nv_sunk_type);
}

internal void navy_fish(void) {
    static struct { ee_unit *site; int owner; } gath[512];
    static int seen[NV_FISH_MAX];
    int ng = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {   // the boats at work: their sites
        ee_player *o = unit_owner(it.u);
        uint8_t *goal = FIELD(it.u, OF(UNIT, 0x1EC), uint8_t *);
        if (!o || !goal || FIELD(it.u, OF(UNIT, 0x3C), int) <= 0 || navy_kind(it.td) != NV_FISH || ng >= 512) continue;
        gath[ng].site = FIELD(goal, OF(GOAL, 0x38), ee_unit *); gath[ng++].owner = FIELD(o, PL_INDEX, int) & 15;
    }
    memset(seen, 0, sizeof seen);
    int left = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        if (FIELD(it.td, OF(TD, 0x4B8), int) != UT(RES_FISH)) continue;
        ee_unit *u = it.u;
        int id = FIELD(u, OF(UNIT, 0x60), int), amt = FIELD(u, OF(UNIT, 0x40), int), owner = -1, k;
        left += amt > 0 ? amt : 0;
        for (int g = 0; g < ng; g++) if (gath[g].site == u) { owner = gath[g].owner; break; }
        for (k = 0; k < g_nv_nfish; k++) if (g_nv_fish[k].u == u && g_nv_fish[k].id == id) break;
        if (k == g_nv_nfish) {
            if (g_nv_nfish >= NV_FISH_MAX) continue;
            g_nv_nfish++;
            g_nv_fish[k].u = u; g_nv_fish[k].id = id; g_nv_fish[k].amount = amt; g_nv_fish[k].owner = -1;
        }
        seen[k] = 1;
        if (owner >= 0) g_nv_fish[k].owner = owner;
        int d = g_nv_fish[k].amount - amt;
        if (d > 0 && g_nv_fish[k].owner >= 0) g_nv_fish_food[g_nv_fish[k].owner] += d;
        g_nv_fish[k].amount = amt;
    }
    for (int k = 0; k < g_nv_nfish; k++)   // gone: eaten up
        if (!seen[k]) {
            if (g_nv_fish[k].owner >= 0 && g_nv_fish[k].amount > 0) g_nv_fish_food[g_nv_fish[k].owner] += g_nv_fish[k].amount;
            g_nv_fish[k--] = g_nv_fish[--g_nv_nfish];
            seen[k + 1] = seen[g_nv_nfish];
        }
    g_nv_fish_left = left;
}

// WATERS: a ship is in the waters of the player whose building (families 2, 29) is nearest within 20 tiles
internal int navy_waters(int x, int y) {
    int best = -1, bd = 401;
    for (struct grid_it it = grid_it_box(x, y, 20, 1); grid_it_next(&it); ) {
        int fam = FIELD(it.td, OF(TD, 0x4B4), int);
        ee_player *o = unit_owner(it.u);
        if ((fam != 2 && fam != 29) || !o || FIELD(it.u, OF(UNIT, 0x3C), int) <= 0) continue;
        int pi = FIELD(o, PL_INDEX, int), dx = FIELD(it.u, OF(UNIT, 0x1C), int) - x, dy = FIELD(it.u, OF(UNIT, 0x20), int) - y;
        if (pi <= 0 || pi >= 16 || dx * dx + dy * dy >= bd) continue;
        bd = dx * dx + dy * dy; best = pi;
    }
    return best;
}

struct nv_count {
    int docks, yards, n[6], hp[6], home, away, foe_home, foe_home_hp;
    int nt; struct { int type, n; } wt[24];   // warships, submarines and carriers by type
};
internal void navy_log_all(uint32_t now) {
    static struct nv_count c[16];
    memset(c, 0, sizeof c);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_player *o = unit_owner(it.u);
        int x = FIELD(it.u, OF(UNIT, 0x1C), int), y = FIELD(it.u, OF(UNIT, 0x20), int), t = FIELD(it.td, OF(TD, 0x4B8), int);
        if (!o || FIELD(it.u, OF(UNIT, 0x3C), int) <= 0) continue;
        int pi = FIELD(o, PL_INDEX, int);
        if (pi <= 0 || pi >= 16) continue;
        struct nv_count *m = &c[pi];
        if (t == UT(B_DOCK)) { m->docks++; continue; }
        if (t == UT(B_NAVY_YARD)) { m->yards++; continue; }
        int k = navy_kind(it.td);
        if (k == NV_NONE) continue;
        m->n[k]++; m->hp[k] += FIELD(it.u, OF(UNIT, 0x3C), int);
        if (k != NV_WAR && k != NV_SUB && k != NV_CARRIER) continue;
        int j;
        for (j = 0; j < m->nt && m->wt[j].type != t; j++) ;
        if (j < 24) { if (j == m->nt) { m->nt++; m->wt[j].type = t; m->wt[j].n = 0; } m->wt[j].n++; }
        if (x < 0) continue;
        int w = navy_waters(x, y);
        ee_player *pw = w > 0 ? player_at(w) : NULL;
        if (w == pi) m->home++;
        else if (pw && is_enemy(o, pw)) {
            m->away++;
            c[w].foe_home++; c[w].foe_home_hp += FIELD(it.u, OF(UNIT, 0x3C), int);
        }
    }
    EnterCriticalSection(&g_lock);
    for (int p = 1; p < nplayers(); p++) {
        ee_player *pl = player_at(p);
        if (!pl || is_out(pl) || !g_players[p]) continue;   // the AIs only
        struct nv_count *m = &c[p];
        uint32_t lost = 0, sunk = 0, blost = 0, bsunk = 0;
        for (int j = 0; j < 16; j++) {
            lost += g_kills[j][p][K_NAVY]; sunk += g_kills[p][j][K_NAVY];
            blost += g_nv_boat_kills[j][p]; bsunk += g_nv_boat_kills[p][j];
        }
        const int *st = (const int *)((uint8_t *)pl + PL_STOCK);
        fprintf(g_log, "# navy: P%d at %u min: docks %d yards %d | fishing %d food %d (fish left %d) | war %d hp %d [",
                p, now / 60000, m->docks, m->yards, m->n[NV_FISH], g_nv_fish_food[p], g_nv_fish_left, m->n[NV_WAR], m->hp[NV_WAR]);
        for (int j = 0; j < m->nt; j++) fprintf(g_log, "%s%dx%d", j ? " " : "", m->wt[j].type, m->wt[j].n);
        int ro[7] = { 0 }, rl[7] = { 0 }, rs[7] = { 0 };   // by the mix's roles (6: transports): afloat, lost, sunk
        for (int j = 0; j < m->nt; j++) ro[nv_role(type_data(m->wt[j].type))] += m->wt[j].n;
        for (int t = 0; t < G_TYPES_N; t++) {
            if (!g_nv_lost_type[p][t] && !g_nv_sunk_type[p][t]) continue;
            uint8_t *td = type_data(t);
            int r = td && navy_kind(td) == NV_TRANSPORT ? 6 : nv_role(td);
            rl[r] += g_nv_lost_type[p][t]; rs[r] += g_nv_sunk_type[p][t];
        }
        fprintf(g_log, "] roles F%d L%d A%d S%d AA%d | lost-roles F%d L%d A%d S%d AA%d T%d | sunk-roles F%d L%d A%d S%d AA%d T%d |",
                ro[1], ro[2], ro[3], ro[4], ro[5], rl[1], rl[2], rl[3], rl[4], rl[5], rl[6], rs[1], rs[2], rs[3], rs[4], rs[5], rs[6]);
        fprintf(g_log, " subs %d carriers %d transports %d | home %d away %d foes-home %d (hp %d) | lost %u sunk %u boats-lost %u boats-sunk %u"
                       " | income F%d W%d S%d G%d I%d | stock F%d W%d S%d G%d I%d\n",
                m->n[NV_SUB], m->n[NV_CARRIER], m->n[NV_TRANSPORT], m->home, m->away, m->foe_home, m->foe_home_hp,
                lost, sunk, blost, bsunk, g_bud[p].income[2], g_bud[p].income[3], g_bud[p].income[4], g_bud[p].income[5],
                g_bud[p].income[6], st[2], st[3], st[4], st[5], st[6]);
    }
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}

// WATER MAP (docs/navy.md stage 1), once per AI and game from the map itself, 10 s in (the start buildings stand):
// its home continent (area_id kind 2 at the build manager's base point +0x12C/+0x130, as landing.c's home), every
// enemy with buildings (families 2, 29) - across the water when none of them stands on that continent - and the
// map's ocean tiles (kind 1, every 4th tile). The strength: the share of enemies across the water (1 islands, 0 land,
// between: mixed); the navy's share of the income scales with it. Logged with the game's own flags for comparison:
// world (0x9183BC) +0x181730 bit 0 (the game's "water map", 0x4307A7) and player +0xEFA (that and 10+ fish). An
// enemy without any building yet: asked again every 10 s up to 3 minutes, then decided on the others.
#define NV_LAND 0
#define NV_MIXED 1
#define NV_ISLANDS 2
struct nv_bld { int x[16][64], y[16][64], n[16]; };
internal void navy_detect(ee_player *pl, unsigned idx, uint32_t now) {
    static struct nv_bld b;
    void *cm = FIELD(pl, OF(PL, 0xB90), void *);
    uint8_t *bm = cm ? FIELD(cm, OF(CM, 0x3C), uint8_t *) : NULL, *map = *(uint8_t **)GA(G_MAP);
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    unsigned hc = bm ? area_id(FIELD(bm, OF(BM, 0x12C), int), FIELD(bm, OF(BM, 0x130), int), 2) : 0;
    if (!hc || !grid) return;
    memset(b.n, 0, sizeof b.n);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        int fam = FIELD(it.td, OF(TD, 0x4B4), int);
        ee_player *o = unit_owner(it.u);
        if ((fam != 2 && fam != 29) || !o || FIELD(it.u, OF(UNIT, 0x3C), int) <= 0) continue;
        int pi = FIELD(o, PL_INDEX, int);
        if (pi <= 0 || pi >= 16 || b.n[pi] >= 64) continue;
        b.x[pi][b.n[pi]] = FIELD(it.u, OF(UNIT, 0x1C), int); b.y[pi][b.n[pi]++] = FIELD(it.u, OF(UNIT, 0x20), int);
    }
    int enemies = 0, across = 0, none = 0;
    for (int p = 1; p < nplayers(); p++) {
        void *pp = player_at(p);
        if (!pp || is_out(pp) || !is_enemy(pl, pp)) continue;
        if (!b.n[p]) { none++; continue; }
        int home = 0;
        for (int k = 0; k < b.n[p] && !home; k++) home = area_id(b.x[p][k], b.y[p][k], 2) == hc;
        enemies++; across += !home;
    }
    if (none && now < 180000) return;   // an enemy without buildings yet: wait for them
    int nx = FIELD(grid, OF(GRID, 0x10), int) * FIELD(grid, OF(GRID, 0x8), int), ny = FIELD(grid, OF(GRID, 0x14), int) * FIELD(grid, OF(GRID, 0x8), int), sea = 0, all = 0;
    for (int x = 1; x < nx; x += 4)
        for (int y = 1; y < ny; y += 4, all++) sea += area_id(x, y, 1) != 0;
    struct nv_map *m = &g_nv_map[idx];
    m->done = 1; m->t = now; m->across = across; m->enemies = enemies; m->home = hc;
    m->strength = enemies ? (float)across / enemies : 0.0f;
    m->water = all ? (float)sea / all : 0.0f;
    m->kind = !enemies || !across ? NV_LAND : across == enemies ? NV_ISLANDS : NV_MIXED;
    uint8_t *w = *(uint8_t **)GA(G_MAP);
    if (g_log) fprintf(g_log, "# navy: P%u map at %.1f min: %s (strength %.2f) - %d of %d enemies across the water, water %.0f%% of the map, "
                       "home continent %u; the game's water flag %d, +0xEFA %d\n", idx, now / 60000.0,
                       m->kind == NV_ISLANDS ? "islands" : m->kind == NV_MIXED ? "mixed" : "land", m->strength, across, enemies,
                       100.0f * m->water, hc, w ? (int)(FIELD(w, OF(MAP, 0x181730), uint32_t) & 1) : -1, (int)FIELD(pl, OF(PL, 0xEFA), uint8_t));
}

// NAVY ACCOUNT ([all] navy, navy_share; docs/navy.md stage 1). Every ship order went through the military budget,
// fishing boats included, and the army took the account: 1000-1700 failed ship requests per AI and hour on island
// maps, 2-4 warships at most. On a water map (navy_detect) navy_share % x the strength of every wood, gold and iron
// income (EEComputerPlayer::AddResource, econ.c hook_add_resource) goes into a navy account (at most NV_ACC_CAP of
// each: when the game's AI asks for no ship, the rest stays the army's; 1200 held the wood a landing's transports
// needed - Small Islands e5, 20 seeds: landings sailed 4.0 -> 1.3 per AI):
// - every order but a warship's (army, citizens, fishing boats) sees the account as part of the reserve (econ.c
//   train_budget): the army can't take it. Not the priority orders (siege, a landing's transports and fleet): they
//   spend the stock above the reserve as before;
// - the game's (and the mod's) warship, submarine and carrier orders spend the account (or the stock beyond the
//   hoard cap, as the army does) instead of the military account, which pays nothing for them;
// - fishing boats, and the transports a landing orders, are economy: not paid from (nor charged to) the military
//   account, limited only by the reserve.
// Which ships the game's AI asks for is its own (stage 2 changes that). `# navy: P<n> budget` every 5 minutes.
#define NV_ACC_CAP 600
internal int g_navy = 1, g_navy_share = 25;
internal int g_v_navy = -1;   // head-to-head (autotest): [autotest] v_navy for the variant_players (econ.c PV)
internal float navy_share(unsigned idx) {
    if (!PV(idx, g_v_navy, g_navy) || !g_nv_map[idx & 15].done) return 0.0f;
    float sh = (float)g_navy_share;
    int t = g_nv_target[idx & 15], miss = t - g_nv_own[idx & 15].war;
    if (t > 0 && miss > 0 && g_navy_share_max > g_navy_share && fleet_builds(idx) && !fleet_sheltering(idx)) sh += (float)(g_navy_share_max - g_navy_share) * (float)miss / (float)t;
    return sh / 100.0f * g_nv_map[idx & 15].strength;
}
// stage 4: the account is the navy's own money - a warship spends it even where the reserve (epochs, landing ...)
// would hold the stock (econ.c train_budget); the others never saw it (navy_hold): Large Islands e9, 524 frigate
// orders refused in an hour with the account full (W600 G600 I546) - the stock above the reserve was short of iron
internal int navy_earmarked(unsigned idx) { return fleet_builds(idx) && g_nv_target[idx & 15] > 0; }
// army.c: the map is known (navy_detect) and an enemy stands on this AI's continent (land or mixed)
internal int navy_land(unsigned idx) { return g_nv_map[idx & 15].done && g_nv_map[idx & 15].kind != NV_ISLANDS; }
internal void navy_income(unsigned idx, int type, int amount) {
    float sh = navy_share(idx);
    if (sh <= 0 || (type != 3 && type != 5 && type != 6)) return;
    float *a = &g_nv_acc[idx & 15][type];
    *a += (float)amount * sh;
    if (*a > NV_ACC_CAP) *a = NV_ACC_CAP;
}
internal int navy_type_kind(int type) {
    uint8_t *td = type_data(type);
    return td ? navy_kind(td) : NV_NONE;
}
// a warship order the navy account pays (econ.c hook_train_units)
internal int navy_pays(unsigned idx, int type) {
    int k = navy_type_kind(type);
    return navy_share(idx) > 0 && (k == NV_WAR || k == NV_SUB || k == NV_CARRIER);
}
// an order paid as economy: fishing boats, the landing's transports
internal int navy_economy(unsigned idx, int type, int landing_order) {
    int k = navy_type_kind(type);
    return PV(idx, g_v_navy, g_navy) && (k == NV_FISH || (landing_order && k == NV_TRANSPORT));
}
// every other order: the account is not there - and once the account pays the warship the game last asked for (within
// 2 minutes), that ship's whole cost: the army took the iron down to the reserve + 200 again and again while the
// account held 44 of it (Small Islands e9: 400-500 frigate orders refused in 25 minutes, the account at 800)
internal int navy_account(unsigned idx, int r);
internal void navy_hold(ee_player *pl, unsigned idx, int reserve[7]) {
    struct nv_orders *o = &g_nv_ord[idx & 15];
    uint32_t now = game_time();
    int want = o->want && now >= o->want_t && now - o->want_t < 120000 ? o->want : 0, total = 0;
    for (int r = 3; r < 7 && want; r++) if (r != 4) total += unit_cost(pl, want, r);
    if (want && navy_account(idx, 3) < total) want = 0;
    for (int r = 3; r < 7; r++) {
        if (r == 4) continue;
        int h = (int)g_nv_acc[idx & 15][r], c = want ? unit_cost(pl, want, r) : 0;
        reserve[r] += h > c ? h : c;
    }
}
// one pool for a warship: the AIs' iron comes in slowly (Small Islands e9: 25% of it paid a frigate's 125 iron only at
// minute 25, 430 orders refused, while the account's wood and gold lay unused) - a warship takes any of its
// resources up to the whole account; what it took beyond that resource's part is charged to the account's others
// (navy_paid), which the army gets instead
internal int navy_account(unsigned idx, int r) {
    const float *a = g_nv_acc[idx & 15];
    return r == 3 || r == 5 || r == 6 ? (int)(a[3] + a[5] + a[6]) : 0;
}
internal void navy_paid(unsigned idx, int type, const int *spent, int got) {   // after a warship order
    struct nv_orders *o = &g_nv_ord[idx & 15];
    for (int r = 2; r < 7; r++) {
        if (spent[r] <= 0) continue;
        o->spent[r] += (unsigned)spent[r];
        float *a = g_nv_acc[idx & 15], over = (float)spent[r];
        if (r != 3 && r != 5 && r != 6) continue;
        for (int pass = 0; pass < 3 && over > 0; pass++) {   // its own part first, then the others, the largest first
            int q = r;
            if (pass) { q = 0; for (int x = 3; x < 7; x++) if (x != 4 && (!q || a[x] > a[q])) q = x; }
            float take = a[q] < over ? a[q] : over;
            a[q] -= take; over -= take;
        }
    }
    int k;
    for (k = 0; k < o->nt && o->t[k].type != type; k++) ;
    if (k == o->nt && k < 8) { o->nt++; o->t[k].type = type; o->t[k].paid = o->t[k].refused = 0; }
    if (got) o->paid++; else o->refused++;
    if (!got) { o->want = type; o->want_t = game_time(); } else if (o->want == type) o->want = 0;
    o->last = type;
    if (k < 8) { if (got) o->t[k].paid++; else o->t[k].refused++; }
}
// THE NODES (docs/navy.md stage 5): the resource sites the AI knows (its towns, player +0xB50: +8/+0xC the sites;
// site +0x38 the resource, +0x40 the amount left), on its home island (navy_detect's continent) and elsewhere - how
// many it works (gatherers: the world's per-site map 0x52E5F7, the cap table 0x917BB0 entry 0x16 as the mine search),
// how many lie far (13+ tiles) from a drop site (Town Center, Capitol, Settlement: the walk decides a mine's income).
// The playtester: "exploit all resource nodes on its island quite quickly, then expand to other islands".
internal void navy_nodes(ee_player *pl, unsigned idx, uint32_t now) {
    struct nv_nodes *c = &g_nv_nodes[idx & 15];
    unsigned hc = g_nv_map[idx & 15].home;
    memset(c, 0, sizeof *c);
    c->t = now ? now : 1;
    if (!hc) return;
    static ee_unit *seen[512];
    int ns = 0, bd[7];
    for (int r = 0; r < 7; r++) bd[r] = 0x7FFFFFFF;
    unsigned cap = table_value(0x16);
    drops_cached(pl);
    unsigned pi = idx & 15;
    void **tb = FIELD(pl, OF(PL, 0xB50), void **), **te = FIELD(pl, OF(PL, 0xB54), void **);
    for (int t = 0; tb && tb + t < te && t < 256; t++) {
        uint8_t *town = tb[t];
        void **sb = town ? FIELD(town, OF(ZONE, 0x8), void **) : NULL, **se = town ? FIELD(town, OF(ZONE, 0xC), void **) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096; k++) {
            ee_unit *s = sb[k];
            int r = s ? FIELD(s, OF(UNIT, 0x38), int) : 0, j;
            if (r < 4 || r > 6 || FIELD(s, OF(UNIT, 0x40), int) <= 0) continue;   // stone, gold, iron
            for (j = 0; j < ns && seen[j] != s; j++) ;
            if (j < ns) continue;
            if (ns < 512) seen[ns++] = s;
            int x = FIELD(s, OF(UNIT, 0x1C), int), y = FIELD(s, OF(UNIT, 0x20), int);
            if (area_id(x, y, 2) != hc) { c->other[r]++; continue; }
            unsigned g = site_gatherers(s);
            int dd = 0x7FFFFFFF;
            for (int i = 0; i < g_drops[pi].n; i++) {
                int *d = g_drops[pi].d[i], dx = d[0] - x, dy = d[1] - y;
                if (!d[2] && dx * dx + dy * dy < dd) dd = dx * dx + dy * dy;
            }
            c->n[r]++; c->g[r] += (int)g; c->slots[r] += (int)cap; c->worked[r] += g > 0;
            if (dd <= 144) c->near_slots[r] += (int)cap;
            else {
                c->remote[r]++;
                // the unworked far one nearest to a drop site: where a Settlement would claim it (navy_claim)
                if (!g && dd < bd[r] && explored_by(s, pl)) { bd[r] = dd; c->free_far[r] = s; c->fx[r] = x; c->fy[r] = y; }
            }
        }
    }
}
// THE MINES ON A WATER MAP ([all] navy_mines; [autotest] v_navy_mines for the variant players; docs/navy.md stage 5).
// From the Copper Age on the game's worker targets follow its demand, and its food demand never falls (econ.c LATE
// WOOD): on islands the metals had 2-5 gatherers at the 12 slots of the home island's iron mines by minute 20
// (navy_nodes: 6 a mine, 2 gold and 2 iron mines on a Small Islands home), while the navy's orders were refused for
// iron 400-500 times an hour and 40+ woodcutters piled up 5000 wood. The playtester: exploit every node on the island
// quickly - early the priorities (food and wood first), later everything it can get hold of. On a water map (the navy
// account on), after the game's targets and econ.c's own changes:
// - wanted on gold and iron: the target + the workers that bring the navy's missing warships' metal within 10 minutes
//   (stage 4: the target - afloat, at most 6, about 100 gold and 100 iron a ship of the mix - a frigate 125 iron, a
//   battleship 250 gold - less the account and the stock beyond the reserve); from minute 15 with 35+ workers at
//   least every slot at the home island's mines near a drop site (12 tiles); never more than the home island's slots;
// - the pull: steps of 2 every 10 s toward that while the wood stock is 500 beyond its need or the food stock 400
//   beyond its reserve and the mines take those sent (metal workers no more than 6 under their targets); 4 back
//   whenever the food stock is under its reserve + 100;
// - taken from wood first while it piles up (down to half its target, 4 at least), then food (down to 60% of it).
internal int g_navy_mines = 1, g_v_navy_mines = -1;
internal int workforce(ee_player *pl);   // econ.c
internal void navy_mines(ee_player *pl, unsigned idx, ee_resource_mgr *rm, uint32_t *target, const int *stock, const int *reserve, const int *demand) {
    idx &= 15;
    struct nv_nodes *c = &g_nv_nodes[idx];
    struct nv_mine *m = &g_nv_mine[idx];
    uint32_t now = game_time();
    if (now < m->t) memset(m, 0, sizeof *m);   // a new game
    if (!PV(idx, g_v_navy_mines, g_navy_mines) || navy_share(idx) <= 0 || !c->t) { m->pull = m->given = 0; return; }
    int want[7] = { 0 }, gap[7] = { 0 }, gaps = 0, miss = fleet_builds(idx) ? g_nv_target[idx] - g_nv_own[idx].war : 0;
    if (miss > 6) miss = 6;
    int wf = workforce(pl);
    for (int r = 5; r < 7; r++) {
        int beyond = stock[r] - reserve[r], need = (miss > 0 ? miss : 0) * 100 - (int)g_nv_acc[idx][r] - (beyond > 0 ? beyond : 0);
        float pw = g_rate[idx].per_worker[r];
        int nav = need > 0 ? (int)((float)need / (10.0f * (pw > 4.0f ? pw : 4.0f)) + 0.99f) : 0, w = (int)target[r] + nav;
        if (now >= 900000 && wf >= 35 && w < c->near_slots[r]) w = c->near_slots[r];
        if (w > c->slots[r]) w = c->slots[r];
        want[r] = w;
        gap[r] = w > (int)target[r] ? w - (int)target[r] : 0;
        gaps += gap[r];
    }
    int food_spare = stock[2] - reserve[2], wood_spare = stock[3] - (demand[3] > 300 ? demand[3] : 300);
    if (!m->t || now - m->t >= 10000) {
        m->t = now ? now : 1;
        int unplaced = rm_count(rm, 5) + rm_count(rm, 6) < (int)(target[5] + target[6]) + m->given - 6;
        if (food_spare < 100) m->pull -= 4;
        else if (m->pull < gaps && (food_spare > 400 || wood_spare > 500) && !unplaced) m->pull += 2;
    }
    if (m->pull > gaps) m->pull = gaps;
    if (m->pull < 0) m->pull = 0;
    m->want[5] = want[5]; m->want[6] = want[6];
    int taken = 0, wfloor = (int)target[3] / 2 > 4 ? (int)target[3] / 2 : 4, ffloor = (int)target[2] * 3 / 5 > 6 ? (int)target[2] * 3 / 5 : 6;
    while (taken < m->pull && wood_spare > 500 && (int)target[3] > wfloor) { target[3]--; taken++; }
    while (taken < m->pull && (int)target[2] > ffloor) { target[2]--; taken++; }
    int g5 = gaps > 0 ? taken * gap[5] / gaps : 0;
    target[5] += (uint32_t)g5; target[6] += (uint32_t)(taken - g5);
    m->given = taken;
}
internal void navy_nodes_log(unsigned idx, uint32_t now) {
    struct nv_nodes *c = &g_nv_nodes[idx & 15];
    struct nv_mine *m = &g_nv_mine[idx & 15];
    static const char rn[7] = { '-', '-', 'F', 'W', 'S', 'G', 'I' };
    fprintf(g_log, "# nodes: P%u at %u min: home island", idx, now / 60000);
    for (int r = 4; r < 7; r++) fprintf(g_log, " %c %d/%d worked (%d of %d slots, %d near, %d far)", rn[r], c->worked[r], c->n[r], c->g[r], c->slots[r], c->near_slots[r], c->remote[r]);
    fprintf(g_log, " | elsewhere S%d G%d I%d | drop sites %d | mines pull %d (wanted G%d I%d)\n", c->other[4], c->other[5], c->other[6],
            g_drops[idx & 15].n, m->given, m->want[5], m->want[6]);
}
internal void navy_mix_log(ee_player *pl, unsigned idx, uint32_t now);
internal void navy_budget_log(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_log || navy_share(idx) <= 0 || (now >= last[idx] && now - last[idx] < 300000)) return;
    last[idx] = now;
    struct nv_orders *o = &g_nv_ord[idx];
    struct nv_own *w = &g_nv_own[idx];
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# navy: P%u budget at %u min: share %.0f%% account W%.0f G%.0f I%.0f | paid W%u G%u I%u | ship orders paid %u refused %u [",
            idx, now / 60000, 100.0f * navy_share(idx), g_nv_acc[idx][3], g_nv_acc[idx][5], g_nv_acc[idx][6], o->spent[3], o->spent[5],
            o->spent[6], o->paid, o->refused);
    for (int k = 0; k < o->nt; k++) fprintf(g_log, "%s%d:%u/%u", k ? " " : "", o->t[k].type, o->t[k].paid, o->t[k].refused);
    fprintf(g_log, "] | warships %d docks %d (%d built, %u asked) fishing %d | room held %u | epoch %d\n", w->war, w->docks, w->docks_built,
            o->docks_asked, w->fish, o->held_room, current_epoch(pl) + 1);
    navy_mix_log(pl, idx, now);
    navy_nodes_log(idx, now);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}
// own warships (with submarines and carriers), docks (36, also unfinished) and fishing boats of every player
internal void navy_count(void) {
    memset(g_nv_own, 0, sizeof g_nv_own);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_player *o = unit_owner(it.u);
        if (!o || FIELD(it.u, OF(UNIT, 0x3C), int) <= 0) continue;
        int pi = FIELD(o, PL_INDEX, int);
        if (pi <= 0 || pi >= 16) continue;
        if (FIELD(it.td, OF(TD, 0x4B8), int) == UT(B_DOCK)) {
            uint8_t *st = FIELD(it.u, OF(UNIT, 0x30), uint8_t *);
            g_nv_own[pi].docks++;
            g_nv_own[pi].docks_built += st && FIELD(st, OF(UNIT, 0x340), uint8_t);
            continue;
        }
        int k = navy_kind(it.td);
        if (k == NV_WAR || k == NV_SUB || k == NV_CARRIER) { g_nv_own[pi].war++; g_nv_own[pi].role[nv_role(it.td)]++; }
        else if (k == NV_FISH) g_nv_own[pi].fish++;
    }
}

// FLEET ROOM (with the navy account): at the population cap the warships the account pays for never came - the army
// filled every free slot first (the bombardment fleet had the same: landing_room_reserve). On a water map with a
// finished dock, the room for the fleet's missing warships is held from the army's orders (econ.c train_call): up to
// 4 x the map strength warships, no more than the account pays for of the type the game last asked for.
internal int navy_room_reserve(ee_player *pl) {
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    struct nv_orders *o = &g_nv_ord[idx];
    float sh = navy_share(idx);
    int t = o->want ? o->want : o->last, total = 0;
    int missing = (fleet_builds(idx) ? g_nv_target[idx] : (int)(4.0f * g_nv_map[idx].strength + 0.5f)) - g_nv_own[idx].war;   // stage 4: the fleet's target
    if (sh <= 0 || !t || missing <= 0 || !g_nv_own[idx].docks_built) return (int)(o->held_room = 0);
    for (int r = 3; r < 7; r++) if (r != 4) total += unit_cost(pl, t, r);
    int n = total > 0 ? navy_account(idx, 3) / total : 0;
    o->held_room = (unsigned)(n < missing ? n : missing);
    return (int)o->held_room;
}

// FLEET MIX ([all] navy_mix; [autotest] v_navy_mix for the variant players; docs/navy.md stage 2). The game's warship
// choice is its RPS vote or a random trainable candidate: frigates, galleys and battleships as they come, nothing
// against the fleet it meets. Where the navy account pays (navy_pays), a warship or submarine the game asks for at one
// of its own training sites (econ.c hook_train_units; not the mod's orders, not a landing's) is trained as the type the
// mix wants. The game's request entry is counted down as if its own type came (0x490AD6 subtracts what 0x49014F
// trained), so it asks for no more than before - no duplicate orders, as with the land unit substitution.
// - Roles, from the game's hit table 0x8D200C [target category][weapon kind] (docs/navy.md 1) and the type's family:
//   F frigates (hit ships and submarines: factor > 0 against c7), L battleships (category c21), A galleys and
//   catapult ships (x3+ against c21), S attack submarines (family 10 that hits ships), AA cruisers (hit only aircraft).
//   Missile subs (hit no ship) and carriers stay out (a carrier the game asks for stays its own).
// - The base mix (the playtester): frigates first, a battleship per 2 frigates, galleys or catapult ships 0.5 - or,
//   once an attack submarine is available (e10), submarines 1 instead (they beat battleships and galleys and are hit
//   only by frigates and towers).
// - The enemy fleet seen (intel memory, 10 minutes - honest vision; with honest_vision 0 what it sees now): + 0.5
//   frigates per galley or submarine seen, + 0.25 battleships per frigate, + 0.75 galleys/subs per battleship;
//   cruisers only with aircraft seen (a carrier counts 3): 0.5 + aircraft / 8, at most 2. Battleships x the share of
//   the enemy's seen fleet that is not submarines (battleships can't hit them).
// - The coast (the playtester: the x4-5 of catapults, cannons and artillery against battleships is real): seen enemy
//   units and buildings within 4 tiles of the sea that hit battleships x3 or more scale the battleships by 4 / (4 + n).
// - Each order: the role furthest below its share of the fleet (own warships + those paid in the last minute + 1) -
//   when its type can't be paid now (stock and account), the next role below its share that can, else it waits for
//   the first (waiting cost the mix AI a ship an hour; it did not mend the h2h: 8.6 -> 7.5 units net a game
//   lost, 160/120 games - so navy_mix stays 0); in the role the type with the best factor x attack / (reload + 2 s) x hit points / cost^2 (Lanchester: a fleet
//   of N ships is worth N^2) against the role's target (F: galleys/subs, L: frigates, A/S: battleships, AA: aircraft).
//   Only types available by their tech record's test (ld_type_avail). A type the budget refused for 2 minutes is
//   skipped for one (the account saves for another role); one the game refused though it was paid for (no building)
//   for 5 minutes (not when it is the game's own type).
// `# navy: P<n> mix at N min:` every 5 minutes: wanted shares, own fleet by role, the types, the enemy seen, what the
// game asked for and what was trained.
enum { NR_NONE, NR_FRIG, NR_LINE, NR_ANTI, NR_SUB, NR_AA, NR_N };
internal const char *const k_nr[NR_N] = { "-", "F", "L", "A", "S", "AA" };
#define NV_C_SUB 7      // target categories (+0x4C4, the hit table's rows): submarines
#define NV_C_FRIG 9     // frigates, cruisers, transports, fishing boats
#define NV_C_LINE 21    // battleships
#define NV_C_GALLEY 47  // galleys, galleons, catapult ships
internal int g_navy_mix = 0, g_v_navy_mix = -1;
internal int8_t g_nv_role[G_TYPES_N];   // role + 1 per type (0: not worked out yet; the table and the type data are fixed)
internal int g_nv_nc, g_nv_nk;          // the hit table's size: categories, weapon kinds (cmd_navy's count)
internal uint64_t g_nv_aircat;          // the categories of aircraft (families 3, 7, 17)
internal float nv_f(int kind, int cat) {   // the hit factor [cat][kind]; 0 outside the table
    uint8_t *tb = *(uint8_t **)GA(G_HIT_TABLE);
    if (!g_nv_nk) {
        for (int t = 0; t < G_TYPES_N; t++) {
            uint8_t *td = type_data(t);
            if (!td) continue;
            int c = FIELD(td, OF(TD, 0x4C4), int), k = FIELD(td, OF(TD, 0x4C8), int), fam = FIELD(td, OF(TD, 0x4B4), int);
            if (c >= 0 && c < 64 && c >= g_nv_nc) g_nv_nc = c + 1;
            if (k >= 0 && k < 64 && k >= g_nv_nk) g_nv_nk = k + 1;
            if ((fam == 3 || fam == 7 || fam == 17) && c >= 0 && c < 64) g_nv_aircat |= 1ull << c;
        }
    }
    if (!tb || kind < 0 || cat < 0 || kind >= g_nv_nk || cat >= g_nv_nc) return 0;
    float *row = FIELD(tb, cat * 4, float *);
    return row ? row[kind] : 0;
}
internal float nv_air_f(int kind) {   // the best factor against an aircraft category
    float b = 0;
    nv_f(0, 0);
    for (int c = 0; c < 64; c++) if (g_nv_aircat >> c & 1) { float f = nv_f(kind, c); if (f > b) b = f; }
    return b;
}
internal int nv_role(uint8_t *td) {
    int t = td ? FIELD(td, OF(TD, 0x4B8), int) : -1;
    if (t < 0 || t >= G_TYPES_N) return NR_NONE;
    if (g_nv_role[t]) return g_nv_role[t] - 1;
    int r = NR_NONE, fam = FIELD(td, OF(TD, 0x4B4), int), kind = FIELD(td, OF(TD, 0x4C8), int), cat = FIELD(td, OF(TD, 0x4C4), int);
    // ships only by their target category (the Naval Mine 386 is family 4, category 22, 1350 attack and 1 hit point);
    // not the hero ships Golden Hind and Agincourt (x07_golden_hind, x09_agincourt_battleship: tech 9035 - no dock trains them)
    int ship = cat == NV_C_SUB || cat == NV_C_FRIG || cat == NV_C_LINE || cat == NV_C_GALLEY;
    if ((fam == 4 || fam == 10) && ship && t != UT(X09_AGINCOURT_BATTLESHIP) && t != UT(X07_GOLDEN_HIND) && FIELD(td, OF(TD, 0x510), int) > 0 && !is_transport(t) && !is_fishing_boat(t)) {
        if (nv_f(kind, NV_C_FRIG) <= 0 && nv_f(kind, NV_C_LINE) <= 0 && nv_f(kind, NV_C_GALLEY) <= 0) r = nv_air_f(kind) > 0 ? NR_AA : NR_NONE;
        else if (fam == 10) r = NR_SUB;
        else if (nv_f(kind, NV_C_LINE) >= 3.0f) r = NR_ANTI;
        else if (cat == NV_C_LINE) r = NR_LINE;
        else r = NR_FRIG;
    }
    g_nv_role[t] = (int8_t)(r + 1);
    return r;
}
internal float nv_score(ee_player *pl, int t, int role) {   // in its role: factor x attack / (reload + 2 s) x hp / cost^2
    uint8_t *td = type_data(t);
    int kind = FIELD(td, OF(TD, 0x4C8), int), cost = 0;
    for (int r = 2; r < 7; r++) cost += unit_cost(pl, t, r);
    float f = role == NR_LINE ? nv_f(kind, NV_C_FRIG) : role == NR_AA ? nv_air_f(kind) : role != NR_FRIG ? nv_f(kind, NV_C_LINE) :
              nv_f(kind, NV_C_GALLEY) > nv_f(kind, NV_C_SUB) ? nv_f(kind, NV_C_GALLEY) : nv_f(kind, NV_C_SUB);
    if (cost <= 0 || f <= 0) return 0;
    return f * (float)FIELD(td, OF(TD, 0x510), int) * 1000.0f / (float)(FIELD(td, OF(TD, 0x50C), int) + 2000) * (float)FIELD(td, OF(TD, 0x55C), int) / ((float)cost * (float)cost);
}
struct nv_pick {
    ee_player *pl; uint32_t cand_t, seen_t, log_t;
    int nc, ctype[24], crole[24];                  // the warship types available (every 30 s)
    int e[NR_N], air, carriers, coast;             // the enemy seen
    float w[NR_N];                                 // the wanted shares (before what is available)
    int ring_role[8]; uint32_t ring_t[8]; int ring_i;   // paid in the last minute: not afloat yet
    int wait_type, skip_type, bad_type[4]; uint32_t wait_since, skip_until, bad_until[4]; int bad_i;
    unsigned asked[NR_N], trained[NR_N], changed, skipped, bad, cheaper;
};
internal struct nv_pick g_nv_pk[16];
internal void nv_mix_reset(void) { memset(g_nv_pk, 0, sizeof g_nv_pk); }
internal void nv_cands(ee_player *pl, struct nv_pick *pk, uint32_t now) {
    if (pk->pl == pl && now >= pk->cand_t && now - pk->cand_t < 30000) return;
    pk->pl = pl; pk->cand_t = now ? now : 1; pk->nc = 0;
    for (int t = 1; t < G_TYPES_N && pk->nc < 24; t++) {
        uint8_t *td = type_data(t);
        if (!td || FIELD(td, OF(TD, 0x4B8), int) != t) continue;
        int r = nv_role(td);
        if (r == NR_NONE || !ld_type_avail(pl, t)) continue;
        pk->ctype[pk->nc] = t; pk->crole[pk->nc++] = r;
    }
}
internal int nv_near_sea(int x, int y) {
    static const int d[5][2] = { { 0, 0 }, { 4, 0 }, { -4, 0 }, { 0, 4 }, { 0, -4 } };
    for (int k = 0; k < 5; k++) if (area_id(x + d[k][0], y + d[k][1], 1)) return 1;
    return 0;
}
internal void nv_seen_one(struct nv_pick *pk, uint8_t *td, int x, int y) {
    if (!td) return;
    int fam = FIELD(td, OF(TD, 0x4B4), int);
    if (fam == 3 || fam == 7 || fam == 17) { pk->air++; return; }
    if (fam == 21) { pk->carriers++; return; }
    int r = nv_role(td);
    if (r != NR_NONE) { pk->e[r]++; return; }
    if (fam == 4 || fam == 10 || fam == 18 || fam == 22 || FIELD(td, OF(TD, 0x510), int) <= 0 || x < 0) return;
    if (nv_f(FIELD(td, OF(TD, 0x4C8), int), NV_C_LINE) >= 3.0f && nv_near_sea(x, y)) pk->coast++;   // hits battleships x3+ from the coast
}
// the enemy seen: intel's memory (10 minutes; buildings until looked at again) - or, with the game's view, every
// enemy unit it sees now; then the wanted shares
internal void nv_seen(ee_player *pl, unsigned idx, struct nv_pick *pk, uint32_t now) {
    pk->seen_t = now ? now : 1;
    memset(pk->e, 0, sizeof pk->e); pk->air = pk->carriers = pk->coast = 0;
    if (g_honest_vision) {
        struct intel *in = &g_intel[idx & 15];
        for (int k = 0; k < in->n; k++) {
            struct intel_rec *e = &in->r[k];
            if (e->kind != 0 && in->now - e->t > 600000) continue;
            nv_seen_one(pk, type_data(e->type), e->x, e->y);
        }
    } else
        for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
            ee_player *o = unit_owner(it.u);
            if (!o || FIELD(it.u, OF(UNIT, 0x3C), int) <= 0 || !is_enemy(pl, o) || !seen_by(it.u, pl)) continue;
            nv_seen_one(pk, it.td, FIELD(it.u, OF(UNIT, 0x1C), int), FIELD(it.u, OF(UNIT, 0x20), int));
        }
    int afloat = pk->e[NR_FRIG] + pk->e[NR_LINE] + pk->e[NR_ANTI] + pk->e[NR_AA];
    float air = (float)(pk->air + 3 * pk->carriers);
    pk->w[NR_FRIG] = 2.0f + 0.5f * (float)(pk->e[NR_ANTI] + pk->e[NR_SUB]);
    pk->w[NR_LINE] = (1.0f + 0.25f * (float)pk->e[NR_FRIG]) * (float)(1 + afloat) / (float)(1 + afloat + pk->e[NR_SUB]) * 4.0f / (4.0f + (float)pk->coast);
    pk->w[NR_ANTI] = 0.5f + 0.75f * (float)pk->e[NR_LINE];
    pk->w[NR_SUB] = 1.0f + 0.75f * (float)pk->e[NR_LINE];
    pk->w[NR_AA] = air > 0 ? (0.5f + air / 8.0f > 2.0f ? 2.0f : 0.5f + air / 8.0f) : 0.0f;
}
// what each role has (own afloat + paid in the last minute) and the best type per role it may train now
internal void nv_roles(ee_player *pl, unsigned idx, struct nv_pick *pk, uint32_t now, int best[NR_N], float w[NR_N], int have[NR_N]) {
    float bs[NR_N] = { 0 };
    memset(best, 0, NR_N * sizeof *best);
    for (int k = 0; k < pk->nc; k++) {
        int t = pk->ctype[k], r = pk->crole[k], skip = t == pk->skip_type && now < pk->skip_until;
        for (int b = 0; b < 4; b++) skip |= t == pk->bad_type[b] && now < pk->bad_until[b];
        float s = skip ? 0 : nv_score(pl, t, r);
        if (s > bs[r]) { bs[r] = s; best[r] = t; }
    }
    for (int r = 0; r < NR_N; r++) {
        w[r] = best[r] ? pk->w[r] : 0.0f;
        have[r] = g_nv_own[idx].role[r];
    }
    if (best[NR_SUB]) w[NR_ANTI] = 0;   // e10+: submarines take the galleys' place
    for (int q = 0; q < 8; q++) if (pk->ring_t[q] && now >= pk->ring_t[q] && now - pk->ring_t[q] < 60000) have[pk->ring_role[q]]++;
}
// the type to train for the game's warship order asked (0: the game's own)
internal int navy_pick(ee_player *pl, unsigned idx, int asked) {
    if ((!PV(idx, g_v_navy_mix, g_navy_mix) && !fleet_builds(idx)) || !navy_pays(idx, asked)) return 0;   // stage 4: the mix too
    int k = navy_type_kind(asked);
    if (k != NV_WAR && k != NV_SUB) return 0;
    uint32_t now = game_time();
    struct nv_pick *pk = &g_nv_pk[idx & 15];
    nv_cands(pl, pk, now);
    if (!pk->seen_t || now < pk->seen_t || now - pk->seen_t >= 30000) nv_seen(pl, idx, pk, now);
    int best[NR_N], have[NR_N], all = 0, br = 0;
    float w[NR_N], wsum = 0;
    nv_roles(pl, idx, pk, now, best, w, have);
    for (int r = 1; r < NR_N; r++) { wsum += w[r]; all += have[r]; }
    if (wsum <= 0) return 0;
    // the role furthest below its share - or, when its type can't be paid now, the next one below its share that can
    // (h2h, 160 games: waiting for the first choice cost the AI with the mix a ship an hour)
    const int *st = (const int *)((uint8_t *)pl + PL_STOCK);
    int pool = navy_account(idx, 3), done[NR_N] = { 0 };
    for (int pass = 0; pass < NR_N; pass++) {
        int r0 = 0;
        float d0 = 0;
        for (int r = 1; r < NR_N; r++) {
            float d = w[r] / wsum * (float)(all + 1) - (float)have[r];
            if (w[r] > 0 && !done[r] && (!r0 || d > d0)) { d0 = d; r0 = r; }
        }
        // stage 4: any role it can pay, the most missing first - the iron the frigates need comes in slowly while the
        // account's wood and gold lay unused (Small Islands e5: account W600 G574 I41, 488 frigate orders refused)
        if (!r0 || (pass && d0 <= 0 && !fleet_builds(idx))) break;
        done[r0] = 1;
        if (!pass) br = r0;
        int pay = 1, total = 0;
        for (int q = 2; q < 7; q++) { int c = unit_cost(pl, best[r0], q); total += c; pay &= c <= st[q]; }
        if (pay && total <= pool) { if (r0 != br) pk->cheaper++; return best[r0]; }
    }
    return br ? best[br] : 0;
}
// stage 4 (fleet.c fl_build): the type of the mod's own warship order - the mix's choice, asked as its first ship type
internal int navy_fleet_type(ee_player *pl, unsigned idx) {
    struct nv_pick *pk = &g_nv_pk[idx & 15];
    nv_cands(pl, pk, game_time());
    for (int k = 0; k < pk->nc; k++) if (pk->crole[k] != NR_AA) return navy_pick(pl, idx, pk->ctype[k]);
    return 0;
}
// after the order: counts, the wait for a refused type, a type the game can't train
internal void navy_pick_done(ee_player *pl, unsigned idx, int pick, int asked, const int *before, const int *stock, int blocked) {
    struct nv_pick *pk = &g_nv_pk[idx & 15];
    uint32_t now = game_time();
    int paid = 0, payable = 1, r = nv_role(type_data(pick));
    for (int q = 2; q < 7; q++) { paid |= stock && before[q] > stock[q]; payable &= !stock || unit_cost(pl, pick, q) <= stock[q]; }
    if (paid) {
        pk->asked[nv_role(type_data(asked))]++; pk->trained[r]++; pk->changed += pick != asked;
        pk->ring_role[pk->ring_i] = r; pk->ring_t[pk->ring_i] = now ? now : 1; pk->ring_i = (pk->ring_i + 1) & 7;
        pk->wait_type = 0;
    } else if (blocked) {   // the budget refused it: wait 2 minutes for it, then another role for one
        if (pk->wait_type != pick || now < pk->wait_since) { pk->wait_type = pick; pk->wait_since = now; }
        else if (now - pk->wait_since >= 120000) { pk->skip_type = pick; pk->skip_until = now + 60000; pk->wait_type = 0; pk->skipped++; }
    } else if (payable && pick != asked) {   // paid for and not trained: no building for it (the game's own type: its check passed)
        pk->bad_type[pk->bad_i] = pick; pk->bad_until[pk->bad_i] = now + 300000; pk->bad_i = (pk->bad_i + 1) & 3; pk->bad++;
        if (g_log) fprintf(g_log, "# navy: P%u at %.1f min: mix type %d (for %d) not trained though payable - skipped 5 minutes\n", idx, now / 60000.0, pick, asked);
    }
}
internal void navy_mix_log(ee_player *pl, unsigned idx, uint32_t now) {
    struct nv_pick *pk = &g_nv_pk[idx & 15];
    if (!g_log || (!PV(idx, g_v_navy_mix, g_navy_mix) && !fleet_builds(idx)) || navy_share(idx) <= 0) return;
    nv_cands(pl, pk, now);
    nv_seen(pl, idx, pk, now);
    int best[NR_N], have[NR_N];
    float w[NR_N], wsum = 0;
    nv_roles(pl, idx, pk, now, best, w, have);
    for (int r = 1; r < NR_N; r++) wsum += w[r];
    fprintf(g_log, "# navy: P%u mix at %u min: want", idx, now / 60000);
    for (int r = 1; r < NR_N; r++) fprintf(g_log, " %s%.0f%%", k_nr[r], wsum > 0 ? 100.0f * w[r] / wsum : 0.0f);
    fprintf(g_log, " | have");
    for (int r = 1; r < NR_N; r++) fprintf(g_log, " %s%d", k_nr[r], g_nv_own[idx & 15].role[r]);
    fprintf(g_log, " | types");
    for (int r = 1; r < NR_N; r++) fprintf(g_log, " %s%d", k_nr[r], best[r]);
    fprintf(g_log, " | enemy seen");
    for (int r = 1; r < NR_N; r++) fprintf(g_log, " %s%d", k_nr[r], pk->e[r]);
    fprintf(g_log, " air %d carriers %d coast %d | asked", pk->air, pk->carriers, pk->coast);
    for (int r = 0; r < NR_N; r++) fprintf(g_log, " %s%u", k_nr[r], pk->asked[r]);
    fprintf(g_log, " trained");
    for (int r = 1; r < NR_N; r++) fprintf(g_log, " %s%u", k_nr[r], pk->trained[r]);
    fprintf(g_log, " (changed %u, skipped %u, no building %u, next role payable now %u)\n", pk->changed, pk->skipped, pk->bad, pk->cheaper);
}

// DOCKS (with the navy account): the game's building plan keeps docks up to its "min military buildings" - 1.5-2.2
// per AI on island maps, often one for the fishing boats, the warships and the transports together. Once a minute
// on a water map: fewer than 2 docks (unfinished ones count), none unfinished, none queued (0x465640) and the dock
// buildable and payable now (0x555710) -> one more through the build manager's own queue and placement (0x46802D(bm,
// 36, 1, 0), as more_granaries asks for a granary; the build manager finds the shore spot). On islands a third while
// the account waits to pay a warship or 12+ fishing boats share the docks.
internal void navy_docks(ee_player *pl, unsigned idx, uint32_t now) {
    struct nv_orders *o = &g_nv_ord[idx];
    struct nv_own *w = &g_nv_own[idx];
    if (navy_share(idx) <= 0 || (now >= o->dock_t && now - o->dock_t < 60000)) return;
    o->dock_t = now;
    int want = 2 + (g_nv_map[idx].kind == NV_ISLANDS && (w->fish >= 12 || (o->want && now - o->want_t < 120000)));
    if (fleet_builds(idx) && g_nv_target[idx] > 0) {   // stage 4: a dock per 8 warships of the target (training in parallel), at most 5
        int f = 2 + g_nv_target[idx] / 8;
        if (f > 5) f = 5;
        if (f > want) want = f;
    }
    void *cm = FIELD(pl, OF(PL, 0xB90), void *);
    ee_build_mgr *bm = cm ? FIELD(cm, OF(CM, 0x3C), ee_build_mgr *) : NULL;
    if (!bm || !ut_has(UT(B_DOCK)) || w->docks >= want || w->docks_built < w->docks || bm_queued(bm, UT(B_DOCK))) return;
    if (!can_build(pl, UT(B_DOCK))) return;
    bm_request(bm, UT(B_DOCK));
    o->docks_asked++;
    if (g_log) fprintf(g_log, "# navy: P%u at %.1f min: docks %d of %d - one more requested\n", idx, now / 60000.0, w->docks, want);
}

// FISHING BOAT ([all] fishing_boat, default 1; docs/navy.md stage 0): the game's pick 0x46D209 (thiscall on the
// economy object, +0x4C = the player) goes by the epoch index alone: below 3 the Stone boat 108, below 9 the Bronze
// boat 109, below 14 the Modern boat 211 - an epoch-11 unit (tech 11090). The Imperial boat 365 (tech 8090) and the
// Digital boat 332 (13098) are never asked for, and from the epoch index 9 on it asks for one it can't train: no
// fishing at all in e10 (e9 starts: 0 boats, food stopped at minute ~30). Both callers (0x48F819 counts the boats,
// 0x48FB05 orders one when none of that type exists or is in production) get the newest fishing boat (family 22,
// highest tech) the player has available by its tech record's own test (landing.c ld_type_avail: an upgraded line's
// old type is not available any more), every epoch; none available: the game's pick.
internal int g_fish_boat = 1;
internal int __fastcall hook_fish_type(void *o, void *edx) {
    (void)edx;
    int t = fish_type(o);
    ee_player *pl = o ? FIELD(o, OF(PL, 0x4C), void *) : NULL;
    if (!g_fish_boat || !pl || !FIELD(pl, PL_TECHTREE, void *) || is_vanilla(pl)) return t;
    int best = 0, bt = 0;
    for (int b = 0; b < G_TYPES_N; b++) {
        if (!is_fishing_boat(b) || !ld_type_avail(pl, b)) continue;
        if (FIELD(type_data(b), OF(TD, 0x4E0), int) > bt) { bt = FIELD(type_data(b), OF(TD, 0x4E0), int); best = b; }
    }
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (best && best != g_fish_last[idx] && g_log)
        fprintf(g_log, "# fishing: P%u at %.1f min: epoch %d - the game asks for boat %d, trains %d\n", idx, game_time() / 60000.0,
                current_epoch(pl) + 1, t, best);
    if (best) g_fish_last[idx] = best;
    return best ? best : t;
}
internal void navy_install(const char *aimod_ini, const char *autotest_ini) {
    g_fish_boat = cfg_int(aimod_ini, autotest_ini, 1, "fishing_boat", 1);
    int ok = g_fish_boat ? patch_fish_type(hook_fish_type, NULL) : 0;
    if (g_log) fprintf(g_log, "# fishing: boat choice %s (hooks %d/2)\n", g_fish_boat ? "newest available" : "the game's", ok);
    g_navy = cfg_int(aimod_ini, autotest_ini, 1, "navy", 1);
    g_navy_share = cfg_pct(aimod_ini, autotest_ini, 1, "navy_share", 25, 100);
    if (g_navy_share < 0) g_navy_share = 0;
    if (g_navy_share > 80) g_navy_share = 80;
    g_navy_mix = cfg_int(aimod_ini, autotest_ini, 1, "navy_mix", 0);
    g_navy_fleet_pop = cfg_pct(aimod_ini, autotest_ini, 1, "navy_fleet_pop", 15, 100);
    g_navy_share_max = cfg_pct(aimod_ini, autotest_ini, 1, "navy_share_max", 50, 100);
    if (g_navy_share_max > 80) g_navy_share_max = 80;
    if (autotest_ini) g_v_navy = ini_int("autotest", "v_navy", -1, autotest_ini);
    if (autotest_ini) g_v_navy_mix = ini_int("autotest", "v_navy_mix", -1, autotest_ini);
    g_navy_mines = cfg_int(aimod_ini, autotest_ini, 1, "navy_mines", 1);
    if (autotest_ini) g_v_navy_mines = ini_int("autotest", "v_navy_mines", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# navy: account %s, share %d%% on islands, fleet mix %s (v_navy %d, v_navy_mix %d for the variant players)\n",
                       g_navy ? "on" : "off", g_navy_share, g_navy_mix ? "on" : "off", g_v_navy, g_v_navy_mix);
    fleet_install(aimod_ini, autotest_ini);
}

internal int mt_fleet_target(ee_player *pl, unsigned idx, int t);   // metal.c metal_plan FLEET: at most what the sea needs
internal void navy_tick(ee_player *pl, uint32_t now) {
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    struct nv_map *m = &g_nv_map[idx];
    if (now < m->t) memset(m, 0, sizeof *m);
    if (!m->done && now >= 10000 && now - m->t >= 10000) { m->t = now; navy_detect(pl, idx, now); }
    if (now < g_nv_count_t || now - g_nv_count_t >= 10000) { g_nv_count_t = now; navy_count(); }
    if (fleet_builds(idx)) {   // the fleet's size target (stage 4): navy_fleet_pop % of the population cap x the map strength
        int cap = unit_limit(pl);
        g_nv_target[idx] = mt_fleet_target(pl, idx, (int)((float)cap * (float)g_navy_fleet_pop / 100.0f * m->strength + 0.5f));   // metal.c FLEET
    } else g_nv_target[idx] = 0;
    if (m->done && (now < g_nv_nodes[idx].t || now - g_nv_nodes[idx].t >= 60000)) navy_nodes(pl, idx, now);   // once a minute
    navy_budget_log(pl, idx, now);
    navy_docks(pl, idx, now);
    fleet_tick(pl, now);
    if (!g_autotest || !g_log) return;
    if (now < g_nv_fish_t) g_nv_fish_t = g_nv_log_t = 0;
    if (now - g_nv_fish_t >= 15000) { g_nv_fish_t = now; navy_fish(); }
    if (now >= 120000 && now / 120000 != g_nv_log_t / 120000) { g_nv_log_t = now; navy_log_all(now); }
}

// navy types A B [all]: per type A..B (naval families only, or every type with all) - family, the hit table's target
// category +0x4C4, weapon kind +0x4C8, damage class +0x4C0, base hit points +0x55C, attack +0x510, range +0x420,
// min range +0x41C, area +0x424, reload +0x50C ms, tech +0x4E0; for naval types also the cost (P2's prices) and P2's
// stats record (0x492969: hp +0x144, attack +0xD8, range +0x9C, speed +0xAC, armour +0xE0 per class 0..5)
internal void cmd_navy(char *out, const char *args) {
    if (!strncmp(args, "types", 5)) {
        int a = 0, b = G_TYPES_N - 1;
        sscanf(args + 5, "%d %d", &a, &b);
        int all = strstr(args, "all") != NULL;
        ee_player *pl = player_at(2);
        if (a < 0) a = 0;
        if (b >= G_TYPES_N) b = G_TYPES_N - 1;
        for (int t = a; t <= b; t++) {
            uint8_t *td = type_data(t);
            if (!td) continue;
            int naval = navy_kind(td) != NV_NONE || FIELD(td, OF(TD, 0x4B4), int) == 4;
            if (!naval && !all) continue;
            REPLY("T %d fam %d cat %d kind %d cls %d hp %d att %d rng %.1f min %.1f area %.2f rel %d tech %d",
                  t, FIELD(td, OF(TD, 0x4B4), int), FIELD(td, OF(TD, 0x4C4), int), FIELD(td, OF(TD, 0x4C8), int), FIELD(td, OF(TD, 0x4C0), int),
                  FIELD(td, OF(TD, 0x55C), int), FIELD(td, OF(TD, 0x510), int), FIELD(td, OF(TD, 0x420), float), FIELD(td, OF(TD, 0x41C), float),
                  FIELD(td, OF(TD, 0x424), float), FIELD(td, OF(TD, 0x50C), int), FIELD(td, OF(TD, 0x4E0), int));
            if (naval && pl) {
                REPLY(" cost %d %d %d %d %d", unit_cost(pl, t, 2), unit_cost(pl, t, 3), unit_cost(pl, t, 4), unit_cost(pl, t, 5), unit_cost(pl, t, 6));
                uint8_t **rec = type_stats(pl, t);
                uint8_t *r = rec ? *rec : NULL;
                if (r) {
                    REPLY(" | hp %d att %.1f rng %.1f spd %.2f arm", FIELD(r, OF(USTAT, 0x144), int), FIELD(r, OF(USTAT, 0xD8), float), FIELD(r, OF(USTAT, 0x9C), float), FIELD(r, OF(USTAT, 0xAC), float));
                    for (int k = 0; k < 6; k++) REPLY(" %g", FIELD(r, OF(USTAT, 0xE0) + k * 4, float));
                }
            }
            REPLY("\n");
        }
        return;
    }
    if (!strncmp(args, "table", 5)) {   // the hit factor table 0x8D200C [target category][weapon kind] (and the 6x6 class table)
        int nc = 0, nk = 0;
        for (int t = 0; t < G_TYPES_N; t++) {
            uint8_t *td = type_data(t);
            if (!td) continue;
            int c = FIELD(td, OF(TD, 0x4C4), int), k = FIELD(td, OF(TD, 0x4C8), int);
            if (c >= 0 && c < 64 && c >= nc) nc = c + 1;
            if (k >= 0 && k < 64 && k >= nk) nk = k + 1;
        }
        REPLY("categories %d kinds %d; tables 8D200C %p 8D2008 %p 8D2010 %p mode %d\n", nc, nk, *(void **)GA(G_HIT_TABLE),
              *(void **)GA(G_HIT_TABLE_A), *(void **)GA(G_HIT_TABLE_B), *(int *)GA(G_GAME_MODE));
        uint8_t *tb = *(uint8_t **)GA(G_HIT_TABLE);
        for (int c = 0; tb && c < nc; c++) {
            float *row = FIELD(tb, c * 4, float *);
            REPLY("c%d:", c);
            for (int k = 0; row && k < nk; k++) REPLY(" %g", row[k]);
            REPLY("\n");
        }
        REPLY("class table 8D2050 [attacker + target x 6]:");
        for (int k = 0; k < 36; k++) REPLY(" %g", ((float *)GA(G_DAMAGE_TABLE))[k]);
        REPLY("\n");
        return;
    }
    REPLY("error: navy types A B [all] | navy table\n");
}
