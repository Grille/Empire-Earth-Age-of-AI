// WILD FOOD ([all] food_plan, bits; [autotest] v_food_plan for the variant players). Included by eeaimod.c after early.c.
// The playtester: "hunting is an important starting step. Animals reproduce - a sustainable source if not overhunted,
// though usually collecting them as quickly as possible is better. Ditch hunting for farming as soon as possible, but
// even in late-epoch starts farms are a huge investment that can wait a bit while other sources (herds, forage,
// fish) are still available." And: early food is for citizens.
// What the game's AI does (docs/economy.md "Food"): the food passes of the assignment (0x48A554) go forage near the
// base (0x48BBA8, the FIRST free bush in zone order within 2 x player +0xE10 tiles, not the nearest), finished fields,
// hunting (0x48BA3B: one hunter an animal and pass, from the hunt list rm +0x88 - built once at the AI's start from
// the world's food units of class 0x11 and only ever pruned, so animals born or come later are never hunted), forage
// farther out, hunting farther out, then field foundations or fishing boats. They fill only the food target, and the
// assignment never takes a gatherer back: citizens sent off food (to a mine as leftovers, after a raid sent them
// home) stayed there - 21 food gatherers down to 1 at minute 20 with 20 on gold (1400 gold unused), target 30.
// Bits:
// 1 PLAN: before farms the worker plan (econ_targets) counts the citizens' food - the Town Center trains them
//   before the epoch is paid: 50 food a citizen for the next 10 while the citizens are short of what the AI wants -
//   and the wood demand only as far as real buildings need it (the queued ones, as LATE WOOD; the game's demand of
//   refused requests ran to 4500 wood in epoch 0 and took half the workers).
// 2 PULL: every assignment (10 s) up to 4 gatherers move to food from the list farthest above its target while food
//   is 2+ under its target: to an animal (bit 8) or the nearest free bush.
// 4 NEAR: the forage search takes the free bush nearest to a food drop site (Town Center, Capitol, Settlement,
//   Granary) within the pass's reach, with the game's checks (explored, gatherer cap, the unit's continent, no enemy
//   near, known towers on the way) - as mine_choice does for mines.
// 8 HUNT: before the game's passes, idle citizens go hunting - every animal (the hunt list or not) within
//   food_hunt_range (15) tiles of a drop site that takes hunters (Town Center, Capitol, Settlement), up to 3 hunters an
//   animal, the nearest to a drop site first; the fierce ones (tiger, wolf) are left alone. Before farms only.
// Measured against the base AI (hard, 60 min, 20 seeds x both seats, paired with the shipped build; docs/economy.md
// "Food"): PLAN + PULL (3, the default) on Continental e0 - the Stone Age 7.7 / 8.7 minutes earlier (no cheat / the
// cheat), workers +9.1 / +6.5 at minute 30, food gatherers 15-19 instead of 9-11 (the base AI: 10-21), the base AI's
// wins 0 -> 1 / 4 -> 4, the mod's 13 -> 22 / 1 -> 2; on the islands (e5, only PULL acts) the same within the noise.
// HUNT alone cost workers (-3 / -4 at 30 min): 23 of 28 citizens chased 15 animals (2300 food) across the map, and the
// food gathered by minute 20 rose 5% with twice the food gatherers - the walk decides wild food's rate. NEAR and CAMP
// (below) gave more workers but no better fights. Off.
static int g_food_plan = 3, g_v_food_plan = -1, g_hunt_range = 15;   // [all] food_hunt_range: tiles from a drop site
#define FOOD_PLAN(idx) PV(idx, g_v_food_plan, g_food_plan)
static struct { unsigned pulled, hunt_sent, near_pick; } g_fd[16];

static void food_new_game(void);

static int food_fierce(int t) { return t == UT(RES_TIGER) || t == UT(RES_WOLF); }
// a free animal to hunt for unit u (NULL: the best for any): within food_hunt_range tiles of a non-granary drop site, food left,
// not fierce, under 3 hunters, explored, the unit's continent, no enemy near, no known towers on the way
static uint8_t *food_animal(void *pl, ee_resource_mgr *rm, uint8_t *u) {
    uint8_t *cm = FIELD(rm, RM_OWNER, uint8_t *);
    void *bm = cm ? FIELD(cm, OF(CM, 0x3C), void *) : NULL;
    if (!bm) return NULL;
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    drops_cached(pl);
    short (__thiscall *region)(void *, int *, int) = (void *)GA(FN_REGION);
    short ur = 0;
    if (u) { int up[2] = { FIELD(u, OF(UNIT, 0x1C), int), FIELD(u, OF(UNIT, 0x20), int) }; ur = region((void *)GA(G_MAP_AREAS), up, 2); }
    uint8_t *best = NULL;
    int bd = g_hunt_range * g_hunt_range + 1;
    for (int i = 0; i < g_drops[pi].n; i++) {
        int *d = g_drops[pi].d[i];
        if (d[2]) continue;
        for (struct grid_it it = grid_it_box(d[0], d[1], g_hunt_range, 1); grid_it_next(&it); ) {
            uint8_t *a = it.u, *td = it.td;
            if (!td || FIELD(a, OF(UNIT, 0x38), int) != 2 || FIELD(a, OF(UNIT, 0x40), int) <= 0 || FIELD(td, OF(TD, 0x4C4), int) != 0x11) continue;
            if (food_fierce(FIELD(td, OF(TD, 0x4B8), int))) continue;
            int x = FIELD(a, OF(UNIT, 0x1C), int), y = FIELD(a, OF(UNIT, 0x20), int), dx = x - d[0], dy = y - d[1], d2 = dx * dx + dy * dy;
            if (d2 >= bd) continue;
            if (GF(FN_SITE_GATHERERS)((void *)GA(G_GAME_STATE), a) >= 3) continue;
            int pos[2] = { x, y };
            if (u && region((void *)GA(G_MAP_AREAS), pos, 2) != ur) continue;
            if (((EnemyNearFn)GA(FN_ENEMY_NEAR))(bm, x, y) || site_danger(pl, a, 1)) continue;
            best = a; bd = d2;
        }
    }
    return best;
}
// the free bush nearest to a food drop site (NEAR), within max_d2 of the base; NULL: none
static uint8_t *food_bush(void *pl, ee_resource_mgr *rm, uint8_t *u, int max_d2) {
    uint8_t *cm = FIELD(rm, RM_OWNER, uint8_t *);
    void *bm = cm ? FIELD(cm, OF(CM, 0x3C), void *) : NULL;
    if (!bm) return NULL;
    int base[2] = { FIELD(bm, OF(BM, 0x12C), int), FIELD(bm, OF(BM, 0x130), int) };
    if (base[0] == -10000 || base[1] == -10000) return NULL;
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    drops_cached(pl);
    unsigned cap = GF(FN_TABLE_VALUE)((void *)GA(G_TABLE_VALUES), 0x16);
    short (__thiscall *region)(void *, int *, int) = (void *)GA(FN_REGION);
    short ur = 0;
    if (u) { int up[2] = { FIELD(u, OF(UNIT, 0x1C), int), FIELD(u, OF(UNIT, 0x20), int) }; ur = region((void *)GA(G_MAP_AREAS), up, 2); }
    uint8_t *best = NULL;
    int bd = 0x7FFFFFFF;
    void **zb = FIELD(pl, OF(PL, 0xB50), void **), **ze = FIELD(pl, OF(PL, 0xB54), void **);
    for (int z = 0; zb && zb + z < ze && z < 256; z++) {
        uint8_t *zone = zb[z];
        void **sb = zone ? FIELD(zone, OF(ZONE, 0x8), void **) : NULL, **se = zone ? FIELD(zone, OF(ZONE, 0xC), void **) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096; k++) {
            uint8_t *s = sb[k], *td = s ? FIELD(s, OF(UNIT, 0x2C), uint8_t *) : NULL;
            if (!td || FIELD(s, OF(UNIT, 0x38), int) != 2 || FIELD(td, OF(TD, 0x4B8), int) != UT(RES_FORAGE_BUSH) || FIELD(s, OF(UNIT, 0x40), int) <= 0) continue;
            if (FIELD(FIELD(s, OF(UNIT, 0x14), uint8_t *), OF(SITEOWN, 0x4), int) == 1) continue;
            int pos[2] = { FIELD(s, OF(UNIT, 0x1C), int), FIELD(s, OF(UNIT, 0x20), int) }, dx = pos[0] - base[0], dy = pos[1] - base[1];
            if (dx * dx + dy * dy >= max_d2) continue;
            int dd = 0x7FFFFFFF;   // to the nearest drop site that takes food
            for (int i = 0; i < g_drops[pi].n; i++) {
                int *d = g_drops[pi].d[i], ex = d[0] - pos[0], ey = d[1] - pos[1];
                if (ex * ex + ey * ey < dd) dd = ex * ex + ey * ey;
            }
            if (dd >= bd) continue;
            if (!GF(FN_EXPLORED)(FIELD(s, OF(UNIT, 0x14), void *), pl)) continue;
            if (GF(FN_SITE_GATHERERS)((void *)GA(G_GAME_STATE), s) >= cap) continue;
            if (u && region((void *)GA(G_MAP_AREAS), pos, 2) != ur) continue;
            if (((EnemyNearFn)GA(FN_ENEMY_NEAR))(bm, pos[0], pos[1]) || site_danger_res(pl, s, 1, 2)) continue;
            best = s; bd = dd;
        }
    }
    return best;
}
// hook_find_food (sites.c) with NEAR: the nearest-to-a-drop bush within the pass's reach (near: 2 x +0xE10, else 6 x)
static uint8_t *food_near_pick(ee_resource_mgr *rm, int near_base, uint8_t *unit) {
    void *pl = rm_player(rm);
    if (!pl) return NULL;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!(FOOD_PLAN(idx) & 4)) return NULL;
    int r6 = FIELD(pl, OF(PL, 0xE10), int);
    uint8_t *s = food_bush(pl, rm, unit, near_base ? r6 * r6 * 4 : r6 * r6 * 36);
    if (s) g_fd[idx].near_pick++;
    return s;
}
// before the assignment (hook_assign_citizens): idle citizens to animals (HUNT, before farms)
static void food_hunt(ee_resource_mgr *rm) {
    void *pl = rm_player(rm);
    if (!pl) return;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!(FOOD_PLAN(idx) & 8) || current_epoch(pl) >= FARM_EPOCH) return;
    uint32_t *target = (uint32_t *)FIELD_AT(rm, RM_TARGET);
    int room = (int)target[2] - rm_count(rm, 2);
    uint8_t *dq = FIELD_AT(rm, RM_LISTS);   // list 0: idle
    for (unsigned k = FIELD(dq, 0x10, unsigned), first = FIELD(dq, 0xC, unsigned); k-- > 0 && room > 0; ) {
        uint8_t *u = deque_at(dq, first + k);
        if (!u || FIELD(u, OF(UNIT, 0x3C), int) <= 0 || FIELD(u, OF(UNIT, 0x33B), uint8_t) || FIELD(u, OF(UNIT, 0x341), uint8_t) || FIELD(u, OF(UNIT, 0x68), int)) continue;
        uint8_t *td = FIELD(u, OF(UNIT, 0x2C), uint8_t *);
        if (!td || !is_citizen_type(FIELD(td, OF(TD, 0x4B8), int)) || FIELD(u, OF(UNIT, 0x1EC), void *)) continue;
        uint8_t *a = food_animal(pl, rm, u);
        if (!a) break;
        int before = rm_count(rm, 2);
        GF(FN_ASSIGN_TO_SITE)(rm, u, a, 0);
        if (rm_count(rm, 2) == before) break;
        g_fd[idx].hunt_sent++;
        room--;
    }
}
// after the assignment (hook_assign_citizens, after econ_rebalance): PULL
static void food_pull(ee_resource_mgr *rm) {
    void *pl = rm_player(rm);
    if (!pl) return;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!(FOOD_PLAN(idx) & 2)) return;
    uint32_t *target = (uint32_t *)FIELD_AT(rm, RM_TARGET);
    int pre = current_epoch(pl) < FARM_EPOCH, moves = 0, dry = 0, r6 = FIELD(pl, OF(PL, 0xE10), int);
    while (moves < 4 && (int)target[2] - rm_count(rm, 2) >= 2) {
        int d = -1, best = 1;
        for (int t = 3; t < 7; t++) {
            int s = rm_count(rm, t) - (int)target[t];
            if (!(dry >> t & 1) && s > best) { best = s; d = t; }
        }
        if (d < 0) break;
        uint8_t *u = movable_gatherer(rm, d);
        if (!u) { dry |= 1 << d; continue; }
        uint8_t *site = pre && (FOOD_PLAN(idx) & 8) ? food_animal(pl, rm, u) : NULL;
        if (!site) site = food_bush(pl, rm, u, r6 * r6 * 36);
        if (!site) break;   // no free, safe food site in reach (fields: the fields pass)
        int before = rm_count(rm, 2);
        GF(FN_ASSIGN_TO_SITE)(rm, u, site, 0);
        if (rm_count(rm, 2) == before) { dry |= 1 << d; continue; }
        g_fd[idx].pulled++;
        moves++;
    }
}
// PLAN (econ_targets, before farms): the citizens' food and the real wood need
static void food_plan_needs(void *pl, ee_resource_mgr *rm, int *need, const int *stock, const int *reserve) {
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!(FOOD_PLAN(idx) & 1)) return;
    int wf = workforce(pl), want = (int)(FIELD(pl, OF(PL, 0xBDC), float) * (float)FIELD(pl, OF(PL, 0xCCC), int));
    if (copper_mode(pl) == 1 && want > g_min_workforce) want = g_min_workforce;   // the plan saves from there on
    int more = want - wf;
    if (more > 10) more = 10;
    if (more > 0) need[2] += 50 * more;
    void *cm = FIELD(rm, RM_OWNER, void *);
    uint8_t *bm = cm ? FIELD(cm, OF(CM, 0x3C), uint8_t *) : NULL;
    int queued = 0, nq = 0;
    for (int e = 0; bm && e <= current_epoch(pl) && e < 14 && nq < 3; e++)
        for (unsigned i = 0; nq < 3; i++) {
            int *ent = bq_entry(bm, e, i);
            if (!ent) break;
            int c = unit_cost(pl, ent[0], 3);
            if (c > 0) { queued += c; nq++; }
        }
    int w = reserve[3] > queued ? reserve[3] : queued;
    w += field_wood(idx) + 100;
    if (need[3] > w - stock[3]) need[3] = w - stock[3] > 0 ? w - stock[3] : 0;
}
// 32 CAMP (before farms): a Settlement (a drop site for everything) by the richest wild food the town's drop sites are
//   far from - every 2 minutes while none is queued or being built and the AI can pay for one (0x555710), 2 a game: the
//   bushes and animals within 30 tiles of the base, each counted with the food within 6 tiles of it, those 10+ tiles
//   from every drop site; 1500+ food there -> a placeable spot 2-4 tiles from it without enemy soldiers within 12,
//   queued on the epoch's build queue with its position. The walk decides wild food's rate: with twice the food
//   gatherers (HUNT, PULL) the food gathered by minute 20 rose 5%.
static struct { uint32_t t; int n; } g_camp[16];
static void food_camp(void *pl, unsigned idx, uint32_t now) {
    if (!(FOOD_PLAN(idx) & 32) || current_epoch(pl) >= FARM_EPOCH) return;
    if (now < g_camp[idx].t) memset(&g_camp[idx], 0, sizeof g_camp[idx]);
    if (g_camp[idx].n >= 2 || (g_camp[idx].t && now - g_camp[idx].t < 120000)) return;
    g_camp[idx].t = now ? now : 1;
    uint8_t *cm = FIELD(pl, OF(PL, 0xB90), uint8_t *), *bm = cm ? FIELD(cm, OF(CM, 0x3C), uint8_t *) : NULL;
    void *um = cm ? FIELD(cm, OF(CM, 0x44), void *) : NULL;
    if (!bm || !um || !ut_has(UT(B_SETTLEMENT)) || FIELD(bm, OF(BM, 0x12C), int) == -10000 || bq_has(bm, UT(B_SETTLEMENT)) || bq_has(bm, UT(X_SINGLE_PLAYER_SETTLEMENT))) return;
    uint8_t *q = type_list(um, UT(B_SETTLEMENT));
    for (unsigned k = 0, f = q ? FIELD(q, 0xC, unsigned) : 0, n = q ? FIELD(q, 0x10, unsigned) : 0; k < n; k++) {
        uint8_t *b = deque_at(q, f + k), *st = b ? FIELD(b, OF(UNIT, 0x30), uint8_t *) : NULL;
        if (b && FIELD(b, OF(UNIT, 0x3C), int) > 0 && (!st || !FIELD(st, OF(UNIT, 0x340), uint8_t))) return;
    }
    if (!GF(FN_CAN_BUILD)(pl, UT(B_SETTLEMENT), 0x20, 0)) return;
    int bx = FIELD(bm, OF(BM, 0x12C), int), by = FIELD(bm, OF(BM, 0x130), int);
    static int fx[512], fy[512], ff[512];
    int n = 0;
    for (struct grid_it it = grid_it_box(bx, by, 30, 1); grid_it_next(&it) && n < 512; ) {   // animals
        uint8_t *a = it.u, *td = it.td;
        if (!td || FIELD(a, OF(UNIT, 0x38), int) != 2 || FIELD(a, OF(UNIT, 0x40), int) <= 0 || FIELD(td, OF(TD, 0x4C4), int) != 0x11 || food_fierce(FIELD(td, OF(TD, 0x4B8), int))) continue;
        fx[n] = FIELD(a, OF(UNIT, 0x1C), int); fy[n] = FIELD(a, OF(UNIT, 0x20), int); ff[n++] = FIELD(a, OF(UNIT, 0x40), int);
    }
    void **zb = FIELD(pl, OF(PL, 0xB50), void **), **ze = FIELD(pl, OF(PL, 0xB54), void **);   // bushes
    for (int z = 0; zb && zb + z < ze && z < 256; z++) {
        void **sb = zb[z] ? FIELD(zb[z], OF(ZONE, 0x8), void **) : NULL, **se = zb[z] ? FIELD(zb[z], OF(ZONE, 0xC), void **) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096 && n < 512; k++) {
            uint8_t *st = sb[k], *td = st ? FIELD(st, OF(UNIT, 0x2C), uint8_t *) : NULL;
            if (!td || FIELD(st, OF(UNIT, 0x38), int) != 2 || FIELD(td, OF(TD, 0x4B8), int) != UT(RES_FORAGE_BUSH) || FIELD(st, OF(UNIT, 0x40), int) <= 0) continue;
            int x = FIELD(st, OF(UNIT, 0x1C), int), y = FIELD(st, OF(UNIT, 0x20), int);
            if ((x - bx) * (x - bx) + (y - by) * (y - by) > 900) continue;
            if (!GF(FN_EXPLORED)(FIELD(st, OF(UNIT, 0x14), void *), pl)) continue;
            fx[n] = x; fy[n] = y; ff[n++] = FIELD(st, OF(UNIT, 0x40), int);
        }
    }
    unsigned pi = idx & 15;
    drops_cached(pl);
    int best = -1, bf = 1499;
    for (int i = 0; i < n; i++) {
        int dd = 0x7FFFFFFF;
        for (int j = 0; j < g_drops[pi].n; j++) {
            int *d = g_drops[pi].d[j], ex = d[0] - fx[i], ey = d[1] - fy[i];
            if (ex * ex + ey * ey < dd) dd = ex * ex + ey * ey;
        }
        if (dd < 100 || ((EnemyNearFn)GA(FN_ENEMY_NEAR))(bm, fx[i], fy[i])) continue;
        int sum = 0;
        for (int j = 0; j < n; j++) if ((fx[j] - fx[i]) * (fx[j] - fx[i]) + (fy[j] - fy[i]) * (fy[j] - fy[i]) <= 36) sum += ff[j];
        if (sum > bf) { bf = sum; best = i; }
    }
    if (best < 0) return;
    int e = current_epoch(pl);
    for (int r = 2; r <= 4; r++)
        for (int ax = -r; ax <= r; ax++)
            for (int ay = -r; ay <= r; ay++) {
                if (abs(ax) != r && abs(ay) != r) continue;
                int x = fx[best] + ax, y = fy[best] + ay;
                if (!can_place(pl, UT(B_SETTLEMENT), x, y) || raiders_near(pl, x, y, 12)) continue;
                int entry[3] = { UT(B_SETTLEMENT), x, y };
                GF(FN_BQ_ADD)(bm + 4 + e * 0x14, entry);
                g_camp[idx].n++;
                if (g_log) {
                    EnterCriticalSection(&g_lock);
                    fprintf(g_log, "# food: P%u at %.1f min: a Settlement queued at %d,%d by %d wild food (cost F%d W%d S%d G%d I%d)\n", idx,
                            now / 60000.0, x, y, bf, unit_cost(pl, UT(B_SETTLEMENT), 2), unit_cost(pl, UT(B_SETTLEMENT), 3), unit_cost(pl, UT(B_SETTLEMENT), 4),
                            unit_cost(pl, UT(B_SETTLEMENT), 5), unit_cost(pl, UT(B_SETTLEMENT), 6));
                    LeaveCriticalSection(&g_lock);
                }
                return;
            }
}
static void food_log(void *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    food_camp(pl, idx, now);
    if (!g_log || !g_autotest || (!g_food_plan && g_v_food_plan < 0)) return;
    if (now / 300000 == last[idx] / 300000 && last[idx]) return;
    last[idx] = now ? now : 1;
    (void)pl;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# food: P%u at %u min: plan %d, pulled %u, hunters sent %u, nearest bushes %u\n", idx, now / 60000,
            FOOD_PLAN(idx), g_fd[idx].pulled, g_fd[idx].hunt_sent, g_fd[idx].near_pick);
    LeaveCriticalSection(&g_lock);
}
static void food_install(const char *aimod_ini, const char *autotest_ini) {
    g_food_plan = cfg_int(aimod_ini, autotest_ini, 1, "food_plan", 3);
    g_hunt_range = cfg_int(aimod_ini, autotest_ini, 1, "food_hunt_range", 15);
    if (autotest_ini) g_v_food_plan = ini_int("autotest", "v_food_plan", -1, autotest_ini);
}
// 16 WILD (econ_fields, from the Copper Age on): no builders for field foundations while wild food is free in reach -
//   a free bush or animal (as the game's own food passes take them before fields), or on a water map with fish
//   (player +0xEFA) 3000+ fish food known while the fishing boats are under 8 or keep up (food 4+ under its target with
//   8 boats: fields). The game itself builds fields only for the shortfall left after wild food; the mod's field pass
//   built every foundation at once (islands e5: 8 fields by minute 10 and no fishing boat until minute 20, the base AI
//   6 boats and no field). Measured (islands e5, food_plan 19 against 3): the fields waited until minute 25-30, but the
//   game's fishing boats came no sooner (0-1 until minute 20) - workers -7 to -11 at minute 30, the base AI's wins on
//   Small Islands 2 -> 5 of 40. Fish comes too slowly to replace the fields; off.
static int food_fields_wait(void *pl, ee_resource_mgr *rm) {
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!(FOOD_PLAN(idx) & 16) || current_epoch(pl) < FARM_EPOCH) return 0;
    int r6 = FIELD(pl, OF(PL, 0xE10), int);
    if (food_bush(pl, rm, NULL, r6 * r6 * 36) || food_animal(pl, rm, NULL)) return 1;
    if (!FIELD(pl, OF(PL, 0xEFA), uint8_t)) return 0;
    int fish = 0, boats = 0;
    void **qb = FIELD(rm, OF(RM, 0x98), void **), **qe = FIELD(rm, OF(RM, 0x9C), void **);
    for (void **h = qb; qb && h < qe && h - qb < 4096; h++)
        if (*h && FIELD(*h, OF(UNIT, 0x40), int) > 0) fish += FIELD(*h, OF(UNIT, 0x40), int);
    if (fish < 3000) return 0;
    uint8_t *list = FIELD_AT(rm, RM_LISTS + 2 * 0x14);
    for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), n = FIELD(list, 0x10, unsigned); k < n && k < 1024; k++) {
        uint8_t *u = deque_at(list, first + k), *td = u ? FIELD(u, OF(UNIT, 0x2C), uint8_t *) : NULL;
        if (td && FIELD(u, OF(UNIT, 0x3C), int) > 0 && FIELD(td, OF(TD, 0x4B4), int) == 22) boats++;
    }
    int short_ = (int)FIELD(rm, RM_TARGET + 2 * 4, uint32_t) - rm_count(rm, 2);
    return !(boats >= 8 && short_ >= 4);
}
static void food_new_game(void) { memset(g_fd, 0, sizeof g_fd); memset(g_camp, 0, sizeof g_camp); }
