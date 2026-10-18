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

static void navy_new_game(void) {
    memset(g_nv_fish, 0, sizeof g_nv_fish);
    memset(g_nv_fish_food, 0, sizeof g_nv_fish_food);
    memset(g_nv_boat_kills, 0, sizeof g_nv_boat_kills);
    g_nv_nfish = g_nv_fish_left = 0;
    g_nv_fish_t = g_nv_log_t = 0;
    memset(g_fish_last, 0, sizeof g_fish_last);
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
}

static void navy_tick(void *pl, uint32_t now) {
    (void)pl;
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
