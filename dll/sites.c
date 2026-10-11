// SAFE SITES: replacement for the resource manager's mine search (0x48C204) and food search (0x48BBA8).
// The originals take the FIRST acceptable site in zone order, anywhere on the map, and only check for
// enemies at the site itself - the game's pathfinding then walks citizens straight through enemy
// defences (observed: red mining iron under yellow's towers 25 tiles away, losing its workforce).
// The leftover pass (0x48A1AE) also dumps spare citizens on any free mine whether or not that resource
// needs workers. Here we pick the NEAREST acceptable site to the AI's base, within a maximum distance,
// and reject sites whose straight route from the base passes known enemies (the game's own threat check
// 0x4660B9 sampled every 2 tiles). Leftover citizens only go to mines whose resource is below target;
// otherwise the game falls back to wood. Included by eeaimod.c before econ.c.

static int g_site_max_dist = 20;   // tiles from the base, aimod.ini [all] site_max_distance
static const uintptr_t k_mine_sites[] = { 0x48a247, 0x48a25f, 0x48a277, 0x48a952, 0x48a9f4, 0x48aa2b };
static const uintptr_t k_food_sites[] = { 0x48ab8d, 0x48ac2b, 0x48ae14, 0x48aeb4, 0x48aefa, 0x48af99 };
#define LEFTOVER_SITE_END 0x48a2b9   // call sites below this are in the leftover pass 0x48A1AE

typedef char (__thiscall *EnemyNearFn)(void *, int, int);

static int route_safe(void *bm, const int base[2], const int to[2]) {
    int dx = to[0] - base[0], dy = to[1] - base[1];
    int len = (int)sqrtf((float)(dx * dx + dy * dy));
    for (int t = 3; t < len; t += 2)   // skip the first tiles: our own base
        if (((EnemyNearFn)FN_ENEMY_NEAR)(bm, base[0] + dx * t / len, base[1] + dy * t / len)) return 0;
    return 1;
}

// Drop sites of a player from the unit grid: Town Center 35, Capitol 41, Settlement 43/395 take everything,
// Granary 52/698 only food. Up to `max` entries {x, y, food_only}.
static int drop_sites(void *pl, int (*out)[3], int max) {
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return 0;
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    unsigned count = FIELD(grid, 0x10, unsigned) * FIELD(grid, 0x14, unsigned);
    int n = 0;
    for (unsigned c = 0; cells && c < count; c++)
        for (uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **); b && b < FIELD(cells + c * 16, 8, uint8_t **); b++) {
            uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
            if (!td || FIELD(u, 0x18, void *) != pl || n >= max) continue;
            if (FIELD(u, 0x1C, int) / FIELD(grid, 8, int) != (int)(c / FIELD(grid, 0x14, unsigned)) ||
                FIELD(u, 0x20, int) / FIELD(grid, 8, int) != (int)(c % FIELD(grid, 0x14, unsigned))) continue;
            int t = FIELD(td, 0x4B8, int), food = t == 52 || t == 698;
            if (!food && t != 35 && t != 41 && t != 43 && t != 395) continue;
            out[n][0] = FIELD(u, 0x1C, int); out[n][1] = FIELD(u, 0x20, int); out[n][2] = food;
            n++;
        }
    return n;
}
static int drop_dist(int (*d)[3], int nd, int x, int y, int food) {
    int best = 999;
    for (int i = 0; i < nd; i++) {
        if (d[i][2] && !food) continue;
        int dx = d[i][0] - x, dy = d[i][1] - y, r = (int)sqrtf((float)(dx * dx + dy * dy));
        if (r < best) best = r;
    }
    return best;
}

// type 2 = food (forage sites, zone activity ignored like the original), 4..6 = mines.
static uint8_t *select_site(uint8_t *rm, int type, int need_drop, int max_d2, uint8_t *unit) {
    void *pl = rm_player(rm);
    uint8_t *cm = FIELD(rm, RM_OWNER, uint8_t *);
    void *bm = cm ? FIELD(cm, 0x3C, void *) : NULL;
    if (!pl || !bm) return NULL;
    int base[2] = { FIELD(bm, 0x12C, int), FIELD(bm, 0x130, int) };
    if (base[0] == -10000 || base[1] == -10000) return NULL;
    int lim = g_site_max_dist * g_site_max_dist;
    if (max_d2 <= 0 || max_d2 > lim) max_d2 = lim;
    unsigned cap = ((unsigned (__thiscall *)(void *, int))FN_TABLE_VALUE)((void *)0x917BB0, 0x16);
    short (__thiscall *region)(void *, int *, int) = (void *)FN_REGION;
    int ref_pos[2] = { base[0], base[1] };
    if (unit) { ref_pos[0] = FIELD(unit, 0x1C, int); ref_pos[1] = FIELD(unit, 0x20, int); }
    short ref_region = region((void *)0x917BC8, ref_pos, 2);
    uint8_t *best = NULL;
    int best_d2 = 0x7FFFFFFF;
    void **zb = FIELD(pl, 0xB50, void **), **ze = FIELD(pl, 0xB54, void **);
    for (int z = 0; zb && zb + z < ze; z++) {
        uint8_t *zone = zb[z];
        if (!zone || (type != 2 && !zone[0x18])) continue;
        uint8_t *s = ((uint8_t *(__thiscall *)(void *, int))FN_ZONE_SITE)(zone, type);
        if (!s) continue;
        if (type == 2 && FIELD(FIELD(s, 0x2C, uint8_t *), 0x4B8, int) != 0x37) continue;   // forage only
        if (FIELD(s, 0x40, int) <= 0 || FIELD(FIELD(s, 0x14, uint8_t *), 4, int) == 1) continue;
        if (need_drop && !FIELD(zone, 0x14, int)) continue;
        int pos[2] = { FIELD(s, 0x1C, int), FIELD(s, 0x20, int) };
        int dx = pos[0] - base[0], dy = pos[1] - base[1], d2 = dx * dx + dy * dy;
        if (d2 >= max_d2 || d2 >= best_d2) continue;
        if (!((char (__thiscall *)(void *, void *))FN_EXPLORED)(FIELD(s, 0x14, void *), pl)) continue;
        if (((unsigned (__thiscall *)(void *, void *))FN_SITE_GATHERERS)((void *)0x918300, s) >= cap) continue;
        if (region((void *)0x917BC8, pos, 2) != ref_region) continue;
        if (((EnemyNearFn)FN_ENEMY_NEAR)(bm, pos[0], pos[1])) continue;
        if (!route_safe(bm, base, pos)) continue;
        best = s;
        best_d2 = d2;
    }
    return best;
}

// MINE CHOICE (aimod.ini [all] mine_choice, default on): the original mine search takes the first acceptable mine in
// reverse zone order - with mines next to its town centers free, P2 sent 20 citizens to iron 35 tiles from any drop
// site (3-5 iron per worker-minute against 10-12 for wood next door). Mines are practically endless (300000), so the
// walk decides the income: take the free mine nearest to one of the AI's drop sites (Town Center, Capitol,
// Settlement) instead, with the original's checks (active zone, explored, gatherer cap, no enemy near, the unit's
// continent) and no known enemy on the straight way from that drop site. Nothing found -> the original search.
static int g_mine_choice = 1;
static struct { void *pl; uint32_t t; int n; int d[64][3]; } g_drops[16];
static uint8_t *select_mine_near_drop(uint8_t *rm, int type, uint8_t *unit) {
    void *pl = rm_player(rm);
    uint8_t *cm = FIELD(rm, RM_OWNER, uint8_t *);
    void *bm = cm ? FIELD(cm, 0x3C, void *) : NULL;
    if (!pl || !bm) return NULL;
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    uint32_t now = game_time();
    if (g_drops[pi].pl != pl || now < g_drops[pi].t || now - g_drops[pi].t > 5000) {   // cached 5 s
        g_drops[pi].pl = pl; g_drops[pi].t = now;
        g_drops[pi].n = drop_sites(pl, g_drops[pi].d, 64);
    }
    int nd = g_drops[pi].n, (*d)[3] = g_drops[pi].d;
    unsigned cap = ((unsigned (__thiscall *)(void *, int))FN_TABLE_VALUE)((void *)0x917BB0, 0x16);
    short (__thiscall *region)(void *, int *, int) = (void *)FN_REGION;
    short ref_region = 0;
    if (unit) { int up[2] = { FIELD(unit, 0x1C, int), FIELD(unit, 0x20, int) }; ref_region = region((void *)0x917BC8, up, 2); }
    uint8_t *best = NULL;
    int best_d = 0x7FFFFFFF;
    void **zb = FIELD(pl, 0xB50, void **), **ze = FIELD(pl, 0xB54, void **);
    for (int z = 0; zb && zb + z < ze; z++) {
        uint8_t *zone = zb[z];
        if (!zone || !zone[0x18]) continue;
        uint8_t *s = ((uint8_t *(__thiscall *)(void *, int))FN_ZONE_SITE)(zone, type);
        if (!s || FIELD(s, 0x40, int) <= 0 || FIELD(FIELD(s, 0x14, uint8_t *), 4, int) == 1) continue;
        int pos[2] = { FIELD(s, 0x1C, int), FIELD(s, 0x20, int) };
        int k = -1, dd = 0x7FFFFFFF;   // nearest drop site for all resources (squared distance)
        for (int i = 0; i < nd; i++) {
            int dx = d[i][0] - pos[0], dy = d[i][1] - pos[1];
            if (!d[i][2] && dx * dx + dy * dy < dd) { dd = dx * dx + dy * dy; k = i; }
        }
        if (k < 0 || dd >= best_d) continue;
        if (!((char (__thiscall *)(void *, void *))FN_EXPLORED)(FIELD(s, 0x14, void *), pl)) continue;
        if (((unsigned (__thiscall *)(void *, void *))FN_SITE_GATHERERS)((void *)0x918300, s) >= cap) continue;
        if (unit && region((void *)0x917BC8, pos, 2) != ref_region) continue;
        if (((EnemyNearFn)FN_ENEMY_NEAR)(bm, pos[0], pos[1])) continue;
        int from[2] = { d[k][0], d[k][1] };
        if (!route_safe(bm, from, pos)) continue;
        best = s;
        best_d = dd;
    }
    return best;
}

static void trace_site(uint8_t *rm, const char *what, uintptr_t caller, void *unit, void *r) {
    if (!tracing(rm)) return;
    char buf[300];
    snprintf(buf, sizeof buf, "@%06x %s", (unsigned)caller, what);
    trace_obj(buf, sizeof buf, "unit", unit);
    trace_obj(buf, sizeof buf, "->", r);
    trace_emit(buf);
}

static void *__fastcall hook_find_mine(uint8_t *rm, void *edx, int type, uint8_t *unit, uintptr_t rev) {
    (void)edx;
    uintptr_t caller = CALLER();
    if (!cfg_of(rm_player(rm))->safe_sites) {
        uint8_t *r = g_mine_choice && type >= 4 && type < 7 ? select_mine_near_drop(rm, type, unit) : NULL;
        return r ? r : ((Fn3)FN_FIND_MINE)(rm, (uintptr_t)type, (uintptr_t)unit, rev);
    }
    if (caller < LEFTOVER_SITE_END && type >= 2 && type < 7) {
        // leftover pass: only mines whose resource actually needs more workers
        int have = FIELD(rm, RM_LISTS + type * 0x14 + 0x10, int), want = ((int *)(rm + RM_TARGET))[type];
        if (have >= want) { trace_site(rm, "find_mine(leftover, not needed)", caller, unit, NULL); return NULL; }
    }
    uint8_t *r = select_site(rm, type, 0, 0, unit);
    char what[48];
    wsprintfA(what, "find_mine(type %d)", type);
    trace_site(rm, what, caller, unit, r);
    return r;
}

static void *__fastcall hook_find_food(uint8_t *rm, void *edx, uintptr_t need_drop, uintptr_t near_base, uint8_t *unit) {
    (void)edx;
    if (!cfg_of(rm_player(rm))->safe_sites) return ((Fn3)FN_FIND_FOOD)(rm, need_drop, near_base, (uintptr_t)unit);
    void *pl = rm_player(rm);
    int r6 = pl ? FIELD(pl, 0xE10, int) : 6;
    int max_d2 = (near_base & 0xFF) ? r6 * r6 * 4 : r6 * r6 * 36;   // same passes as the original
    uint8_t *r = select_site(rm, 2, (int)(need_drop & 0xFF), max_d2, unit);
    trace_site(rm, (near_base & 0xFF) ? "find_food(near)" : "find_food(far)", CALLER(), unit, r);
    return r;
}

static void sites_install(const char *aimod_ini, const char *autotest_ini) {
    g_site_max_dist = cfg_int(aimod_ini, autotest_ini, 1, "site_max_distance", 20);
    g_mine_choice = cfg_int(aimod_ini, autotest_ini, 1, "mine_choice", 1);
    int any = g_cfg[0].safe_sites || g_cfg[1].safe_sites || g_cfg[2].safe_sites;
    int ok = 0, total = 0;
    if (any || g_mine_choice)
        for (size_t i = 0; i < sizeof k_mine_sites / sizeof *k_mine_sites; i++, total++)
            ok += patch_call(k_mine_sites[i], FN_FIND_MINE, hook_find_mine);
    if (any) {
        for (size_t i = 0; i < sizeof k_food_sites / sizeof *k_food_sites; i++, total++)
            ok += patch_call(k_food_sites[i], FN_FIND_FOOD, hook_find_food);
    }
    if (g_log) fprintf(g_log, "# safe_sites hooks: %d/%d, max distance %d; mine_choice=%d\n", ok, total, g_site_max_dist, g_mine_choice);
}
