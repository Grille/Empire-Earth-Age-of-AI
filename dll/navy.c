// NAVY (docs/navy.md): what each AI has afloat, who holds which waters, the food from fish, the ships lost - the
// baseline for a standing navy (observation, no orders). The one decision changed: the fishing boat type (stage 0,
// hook_fish_type below).
//   # navy: (autotest only, every 2 game minutes per AI) docks, fishing boats, warships by type, submarines, carriers,
//     transports, the warships in its own waters / an enemy's waters, the enemy's warships in its waters, the ships
//     it lost and sank (diplo.c kill counts), the food its boats took from the fish, its income and stock.
//   navy types A B [all] / navy table (test commands): the ship types' data and the game's hit factor table.
// Included by eeaimod.c after diplo.c (g_kills, player_at), landing.c (is_transport, is_fishing_boat), econ.c (g_bud).

// The type families (+0x4B4, diplo.c kill_kind): 4 ships, 10 submarines, 18 transports, 21 carriers, 22 fishing boats.
// Transports and fishing boats also by their type ids (landing.c), in case a family differs.
enum { NV_NONE, NV_WAR, NV_SUB, NV_CARRIER, NV_TRANSPORT, NV_FISH };
static int navy_kind(uint8_t *td) {
    int fam = FIELD(td, 0x4B4, int), t = FIELD(td, 0x4B8, int);
    if (fam == 22 || is_fishing_boat(t)) return NV_FISH;
    if (fam == 18 || is_transport(t)) return NV_TRANSPORT;
    if (fam == 10) return NV_SUB;
    if (fam == 21) return NV_CARRIER;
    if (fam == 4) return FIELD(td, 0x510, int) > 0 ? NV_WAR : NV_NONE;
    return NV_NONE;
}

// FOOD FROM FISH: the fish (type 0x13B, nature's; resource amount +0x40) as they stand every 15 s; what a fish lost
// since the last look goes to the owner of the fishing boat gathering it (its goal +0x38 = the site, compared as a
// pointer only), or to its last gatherer; a fish gone (eaten up) gives its last amount to its last gatherer.
#define NV_FISH_MAX 1024
static struct { uint8_t *u; int id, amount, owner; } g_nv_fish[NV_FISH_MAX];
static int g_nv_nfish, g_nv_fish_food[16], g_nv_fish_left;
static uint32_t g_nv_fish_t, g_nv_log_t;
static int g_fish_last[16];   // the fishing boat last picked per player (hook_fish_type, its log line)
struct nv_map { uint32_t t; uint8_t done, kind; float strength, water; int across, enemies; };   // navy_detect
static struct nv_map g_nv_map[16];
// the navy account (navy_income) per AI and resource; saved with the game (persist.c)
static float g_nv_acc[16][7];
// what the account paid and refused (the budget line): ship orders by type
struct nv_orders { unsigned paid, refused, spent[7], held_room, docks_asked; int want, nt; uint32_t want_t, dock_t; struct { int type; unsigned paid, refused; } t[8]; };
static struct nv_orders g_nv_ord[16];
// own ships and docks, counted for every player every 10 s (navy_count)
struct nv_own { int war, docks, docks_built, fish; };
static struct nv_own g_nv_own[16];
static uint32_t g_nv_count_t;

static void navy_new_game(void) {
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
}

static void navy_fish(void) {
    static struct { uint8_t *site; int owner; } gath[512];
    static int seen[NV_FISH_MAX];
    int ng = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {   // the boats at work: their sites
        uint8_t *o = FIELD(it.u, 0x18, uint8_t *), *goal = FIELD(it.u, 0x1EC, uint8_t *);
        if (!o || !goal || FIELD(it.u, 0x3C, int) <= 0 || navy_kind(it.td) != NV_FISH || ng >= 512) continue;
        gath[ng].site = FIELD(goal, 0x38, uint8_t *); gath[ng++].owner = FIELD(o, PL_INDEX, int) & 15;
    }
    memset(seen, 0, sizeof seen);
    int left = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        if (FIELD(it.td, 0x4B8, int) != 0x13B) continue;
        uint8_t *u = it.u;
        int id = FIELD(u, 0x60, int), amt = FIELD(u, 0x40, int), owner = -1, k;
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
static int navy_waters(int x, int y) {
    int best = -1, bd = 401;
    for (struct grid_it it = grid_it_box(x, y, 20, 1); grid_it_next(&it); ) {
        int fam = FIELD(it.td, 0x4B4, int);
        uint8_t *o = FIELD(it.u, 0x18, uint8_t *);
        if ((fam != 2 && fam != 29) || !o || FIELD(it.u, 0x3C, int) <= 0) continue;
        int pi = FIELD(o, PL_INDEX, int), dx = FIELD(it.u, 0x1C, int) - x, dy = FIELD(it.u, 0x20, int) - y;
        if (pi <= 0 || pi >= 16 || dx * dx + dy * dy >= bd) continue;
        bd = dx * dx + dy * dy; best = pi;
    }
    return best;
}

struct nv_count {
    int docks, yards, n[6], hp[6], home, away, foe_home, foe_home_hp;
    int nt; struct { int type, n; } wt[24];   // warships, submarines and carriers by type
};
static void navy_log_all(uint32_t now) {
    static struct nv_count c[16];
    memset(c, 0, sizeof c);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        uint8_t *o = FIELD(it.u, 0x18, uint8_t *);
        int x = FIELD(it.u, 0x1C, int), y = FIELD(it.u, 0x20, int), t = FIELD(it.td, 0x4B8, int);
        if (!o || FIELD(it.u, 0x3C, int) <= 0) continue;
        int pi = FIELD(o, PL_INDEX, int);
        if (pi <= 0 || pi >= 16) continue;
        struct nv_count *m = &c[pi];
        if (t == 36) { m->docks++; continue; }
        if (t == 65) { m->yards++; continue; }
        int k = navy_kind(it.td);
        if (k == NV_NONE) continue;
        m->n[k]++; m->hp[k] += FIELD(it.u, 0x3C, int);
        if (k != NV_WAR && k != NV_SUB && k != NV_CARRIER) continue;
        int j;
        for (j = 0; j < m->nt && m->wt[j].type != t; j++) ;
        if (j < 24) { if (j == m->nt) { m->nt++; m->wt[j].type = t; m->wt[j].n = 0; } m->wt[j].n++; }
        if (x < 0) continue;
        int w = navy_waters(x, y);
        void *pw = w > 0 ? player_at(w) : NULL;
        if (w == pi) m->home++;
        else if (pw && is_enemy(o, pw)) {
            m->away++;
            c[w].foe_home++; c[w].foe_home_hp += FIELD(it.u, 0x3C, int);
        }
    }
    EnterCriticalSection(&g_lock);
    for (int p = 1; p < nplayers(); p++) {
        void *pl = player_at(p);
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
        fprintf(g_log, "] subs %d carriers %d transports %d | home %d away %d foes-home %d (hp %d) | lost %u sunk %u boats-lost %u boats-sunk %u"
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
static void navy_detect(void *pl, unsigned idx, uint32_t now) {
    static struct nv_bld b;
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL, *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    unsigned hc = bm ? area_id(FIELD(bm, 0x12C, int), FIELD(bm, 0x130, int), 2) : 0;
    if (!hc || !grid) return;
    memset(b.n, 0, sizeof b.n);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        int fam = FIELD(it.td, 0x4B4, int);
        uint8_t *o = FIELD(it.u, 0x18, uint8_t *);
        if ((fam != 2 && fam != 29) || !o || FIELD(it.u, 0x3C, int) <= 0) continue;
        int pi = FIELD(o, PL_INDEX, int);
        if (pi <= 0 || pi >= 16 || b.n[pi] >= 64) continue;
        b.x[pi][b.n[pi]] = FIELD(it.u, 0x1C, int); b.y[pi][b.n[pi]++] = FIELD(it.u, 0x20, int);
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
    int nx = FIELD(grid, 0x10, int) * FIELD(grid, 8, int), ny = FIELD(grid, 0x14, int) * FIELD(grid, 8, int), sea = 0, all = 0;
    for (int x = 1; x < nx; x += 4)
        for (int y = 1; y < ny; y += 4, all++) sea += area_id(x, y, 1) != 0;
    struct nv_map *m = &g_nv_map[idx];
    m->done = 1; m->t = now; m->across = across; m->enemies = enemies;
    m->strength = enemies ? (float)across / enemies : 0.0f;
    m->water = all ? (float)sea / all : 0.0f;
    m->kind = !enemies || !across ? NV_LAND : across == enemies ? NV_ISLANDS : NV_MIXED;
    uint8_t *w = *(uint8_t **)0x009183BC;
    if (g_log) fprintf(g_log, "# navy: P%u map at %.1f min: %s (strength %.2f) - %d of %d enemies across the water, water %.0f%% of the map, "
                       "home continent %u; the game's water flag %d, +0xEFA %d\n", idx, now / 60000.0,
                       m->kind == NV_ISLANDS ? "islands" : m->kind == NV_MIXED ? "mixed" : "land", m->strength, across, enemies,
                       100.0f * m->water, hc, w ? (int)(FIELD(w, 0x181730, uint32_t) & 1) : -1, (int)FIELD(pl, 0xEFA, uint8_t));
}

// NAVY ACCOUNT ([all] navy, navy_share; docs/navy.md stage 1). Every ship order went through the military budget,
// fishing boats included, and the army took the account: 1000-1700 failed ship requests per AI and hour on island
// maps, 2-4 warships at most. On a water map (navy_detect) navy_share % x the strength of every wood, gold and iron
// income (EEComputerPlayer::AddResource, econ.c hook_add_resource) goes into a navy account (at most NV_ACC_CAP of
// each: when the game's AI asks for no ship, the rest stays the army's):
// - every order but a warship's (army, citizens, the fishing boats, the siege and landing orders) sees the account
//   as part of the reserve (econ.c train_budget): the army can't take it;
// - the game's (and the mod's) warship, submarine and carrier orders spend the account (or the stock beyond the
//   hoard cap, as the army does) instead of the military account, which pays nothing for them;
// - fishing boats, and the transports a landing orders, are economy: not paid from (nor charged to) the military
//   account, limited only by the reserve.
// Which ships the game's AI asks for is its own (stage 2 changes that). `# navy: P<n> budget` every 5 minutes.
#define NV_ACC_CAP 1200
static int g_navy = 1, g_navy_share = 25;
static float navy_share(unsigned idx) {
    return g_navy && g_nv_map[idx & 15].done ? (float)g_navy_share / 100.0f * g_nv_map[idx & 15].strength : 0.0f;
}
static void navy_income(unsigned idx, int type, int amount) {
    float sh = navy_share(idx);
    if (sh <= 0 || (type != 3 && type != 5 && type != 6)) return;
    float *a = &g_nv_acc[idx & 15][type];
    *a += (float)amount * sh;
    if (*a > NV_ACC_CAP) *a = NV_ACC_CAP;
}
static int navy_type_kind(int type) {
    uint8_t *td = type_data(type);
    return td ? navy_kind(td) : NV_NONE;
}
// a warship order the navy account pays (econ.c hook_train_units)
static int navy_pays(unsigned idx, int type) {
    int k = navy_type_kind(type);
    return navy_share(idx) > 0 && (k == NV_WAR || k == NV_SUB || k == NV_CARRIER);
}
// an order paid as economy: fishing boats, the landing's transports
static int navy_economy(int type, int landing_order) {
    int k = navy_type_kind(type);
    return g_navy && (k == NV_FISH || (landing_order && k == NV_TRANSPORT));
}
// every other order: the account is not there - and once the account pays the warship the game last asked for (within
// 2 minutes), that ship's whole cost: the army took the iron down to the reserve + 200 again and again while the
// account held 44 of it (Small Islands e9: 400-500 frigate orders refused in 25 minutes, the account at 800)
static int navy_account(unsigned idx, int r);
static void navy_hold(void *pl, unsigned idx, int reserve[7]) {
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
static int navy_account(unsigned idx, int r) {
    const float *a = g_nv_acc[idx & 15];
    return r == 3 || r == 5 || r == 6 ? (int)(a[3] + a[5] + a[6]) : 0;
}
static void navy_paid(unsigned idx, int type, const int *spent, int got) {   // after a warship order
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
    if (k < 8) { if (got) o->t[k].paid++; else o->t[k].refused++; }
}
static void navy_budget_log(void *pl, unsigned idx, uint32_t now) {
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
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}
// own warships (with submarines and carriers), docks (36, also unfinished) and fishing boats of every player
static void navy_count(void) {
    memset(g_nv_own, 0, sizeof g_nv_own);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        uint8_t *o = FIELD(it.u, 0x18, uint8_t *);
        if (!o || FIELD(it.u, 0x3C, int) <= 0) continue;
        int pi = FIELD(o, PL_INDEX, int);
        if (pi <= 0 || pi >= 16) continue;
        if (FIELD(it.td, 0x4B8, int) == 36) {
            uint8_t *st = FIELD(it.u, 0x30, uint8_t *);
            g_nv_own[pi].docks++;
            g_nv_own[pi].docks_built += st && FIELD(st, 0x340, uint8_t);
            continue;
        }
        int k = navy_kind(it.td);
        if (k == NV_WAR || k == NV_SUB || k == NV_CARRIER) g_nv_own[pi].war++;
        else if (k == NV_FISH) g_nv_own[pi].fish++;
    }
}

// DOCKS (with the navy account): the game's building plan keeps docks up to its "min military buildings" - 1.5-2.2
// per AI on island maps, often one for the fishing boats, the warships and the transports together. Once a minute
// on a water map: fewer than 2 docks (unfinished ones count), none unfinished, none queued (0x465640) and the dock
// buildable and payable now (0x555710) -> one more through the build manager's own queue and placement (0x46802D(bm,
// 36, 1, 0), as more_granaries asks for a granary; the build manager finds the shore spot). On islands a third while
// the account waits to pay a warship or 12+ fishing boats share the docks.
static void navy_docks(void *pl, unsigned idx, uint32_t now) {
    struct nv_orders *o = &g_nv_ord[idx];
    struct nv_own *w = &g_nv_own[idx];
    if (navy_share(idx) <= 0 || (now >= o->dock_t && now - o->dock_t < 60000)) return;
    o->dock_t = now;
    int want = 2 + (g_nv_map[idx].kind == NV_ISLANDS && (w->fish >= 12 || (o->want && now - o->want_t < 120000)));
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
    if (!bm || w->docks >= want || w->docks_built < w->docks || ((char (__thiscall *)(void *, int))0x00465640)(bm, 36)) return;
    if (!((char (__thiscall *)(void *, int, int, int))0x00555710)(pl, 36, 0x20, 0)) return;
    ((void (__thiscall *)(void *, int, int, int))0x0046802D)(bm, 36, 1, 0);
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
#define FN_FISH_TYPE 0x0046D209
static int g_fish_boat = 1;
static int __fastcall hook_fish_type(void *o, void *edx) {
    (void)edx;
    int t = ((int (__thiscall *)(void *))FN_FISH_TYPE)(o);
    void *pl = o ? FIELD(o, 0x4C, void *) : NULL;
    if (!g_fish_boat || !pl || !FIELD(pl, PL_TECHTREE, void *)) return t;
    static const int boats[] = { 108, 109, 365, 211, 332 };
    int best = 0, bt = 0;
    for (size_t k = 0; k < sizeof boats / sizeof *boats; k++) {
        uint8_t *td = type_data(boats[k]);
        if (!td || FIELD(td, 0x4B8, int) != boats[k] || FIELD(td, 0x4B4, int) != 22 || !ld_type_avail(pl, boats[k])) continue;
        if (FIELD(td, 0x4E0, int) > bt) { bt = FIELD(td, 0x4E0, int); best = boats[k]; }
    }
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (best && best != g_fish_last[idx] && g_log)
        fprintf(g_log, "# fishing: P%u at %.1f min: epoch %d - the game asks for boat %d, trains %d\n", idx, game_time() / 60000.0,
                current_epoch(pl) + 1, t, best);
    if (best) g_fish_last[idx] = best;
    return best ? best : t;
}
static void navy_install(const char *aimod_ini, const char *autotest_ini) {
    g_fish_boat = cfg_int(aimod_ini, autotest_ini, 1, "fishing_boat", 1);
    int ok = g_fish_boat ? patch_call(0x0048F823, FN_FISH_TYPE, (void *)hook_fish_type) + patch_call(0x0048FB16, FN_FISH_TYPE, (void *)hook_fish_type) : 0;
    if (g_log) fprintf(g_log, "# fishing: boat choice %s (hooks %d/2)\n", g_fish_boat ? "newest available" : "the game's", ok);
    g_navy = cfg_int(aimod_ini, autotest_ini, 1, "navy", 1);
    g_navy_share = cfg_int(aimod_ini, autotest_ini, 1, "navy_share", 25);
    if (g_navy_share < 0) g_navy_share = 0;
    if (g_navy_share > 80) g_navy_share = 80;
    if (g_log) fprintf(g_log, "# navy: account %s, share %d%% on islands\n", g_navy ? "on" : "off", g_navy_share);
}

static void navy_tick(void *pl, uint32_t now) {
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    struct nv_map *m = &g_nv_map[idx];
    if (now < m->t) memset(m, 0, sizeof *m);
    if (!m->done && now >= 10000 && now - m->t >= 10000) { m->t = now; navy_detect(pl, idx, now); }
    if (now < g_nv_count_t || now - g_nv_count_t >= 10000) { g_nv_count_t = now; navy_count(); }
    navy_budget_log(pl, idx, now);
    navy_docks(pl, idx, now);
    if (!g_auto.enabled || !g_log) return;
    if (now < g_nv_fish_t) g_nv_fish_t = g_nv_log_t = 0;
    if (now - g_nv_fish_t >= 15000) { g_nv_fish_t = now; navy_fish(); }
    if (now >= 120000 && now / 120000 != g_nv_log_t / 120000) { g_nv_log_t = now; navy_log_all(now); }
}

// navy types A B [all]: per type A..B (naval families only, or every type with all) - family, the hit table's target
// category +0x4C4, weapon kind +0x4C8, damage class +0x4C0, base hit points +0x55C, attack +0x510, range +0x420,
// min range +0x41C, area +0x424, reload +0x50C ms, tech +0x4E0; for naval types also the cost (P2's prices) and P2's
// stats record (0x492969: hp +0x144, attack +0xD8, range +0x9C, speed +0xAC, armour +0xE0 per class 0..5)
static void cmd_navy(char *out, const char *args) {
    if (!strncmp(args, "types", 5)) {
        int a = 0, b = G_TYPES_N - 1;
        sscanf(args + 5, "%d %d", &a, &b);
        int all = strstr(args, "all") != NULL;
        void *pl = player_at(2);
        if (a < 0) a = 0;
        if (b >= G_TYPES_N) b = G_TYPES_N - 1;
        for (int t = a; t <= b; t++) {
            uint8_t *td = type_data(t);
            if (!td) continue;
            int naval = navy_kind(td) != NV_NONE || FIELD(td, 0x4B4, int) == 4;
            if (!naval && !all) continue;
            REPLY("T %d fam %d cat %d kind %d cls %d hp %d att %d rng %.1f min %.1f area %.2f rel %d tech %d",
                  t, FIELD(td, 0x4B4, int), FIELD(td, 0x4C4, int), FIELD(td, 0x4C8, int), FIELD(td, 0x4C0, int),
                  FIELD(td, 0x55C, int), FIELD(td, 0x510, int), FIELD(td, 0x420, float), FIELD(td, 0x41C, float),
                  FIELD(td, 0x424, float), FIELD(td, 0x50C, int), FIELD(td, 0x4E0, int));
            if (naval && pl) {
                REPLY(" cost %d %d %d %d %d", unit_cost(pl, t, 2), unit_cost(pl, t, 3), unit_cost(pl, t, 4), unit_cost(pl, t, 5), unit_cost(pl, t, 6));
                uint8_t **rec = ((uint8_t **(__thiscall *)(void *, int))0x00492969)((uint8_t *)pl + 0x874, t);
                uint8_t *r = rec ? *rec : NULL;
                if (r) {
                    REPLY(" | hp %d att %.1f rng %.1f spd %.2f arm", FIELD(r, 0x144, int), FIELD(r, 0xD8, float), FIELD(r, 0x9C, float), FIELD(r, 0xAC, float));
                    for (int k = 0; k < 6; k++) REPLY(" %g", FIELD(r, 0xE0 + k * 4, float));
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
            int c = FIELD(td, 0x4C4, int), k = FIELD(td, 0x4C8, int);
            if (c >= 0 && c < 64 && c >= nc) nc = c + 1;
            if (k >= 0 && k < 64 && k >= nk) nk = k + 1;
        }
        REPLY("categories %d kinds %d; tables 8D200C %p 8D2008 %p 8D2010 %p mode %d\n", nc, nk, *(void **)0x008D200C,
              *(void **)0x008D2008, *(void **)0x008D2010, *(int *)0x00918808);
        uint8_t *tb = *(uint8_t **)0x008D200C;
        for (int c = 0; tb && c < nc; c++) {
            float *row = FIELD(tb, c * 4, float *);
            REPLY("c%d:", c);
            for (int k = 0; row && k < nk; k++) REPLY(" %g", row[k]);
            REPLY("\n");
        }
        REPLY("class table 8D2050 [attacker + target x 6]:");
        for (int k = 0; k < 36; k++) REPLY(" %g", ((float *)0x008D2050)[k]);
        REPLY("\n");
        return;
    }
    REPLY("error: navy types A B [all] | navy table\n");
}
