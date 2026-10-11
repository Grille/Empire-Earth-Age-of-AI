// Attack routing: armies avoid known enemy defences instead of marching through them.
//
// The game plans an attack group's march (EECPASGLand::vf94 0x44BC57 -> path request 0x4A15E7) on a graph
// of path sectors. The cost of each step is EEAIGPGModifierLandAttackSubGroupRoutePlanner::vf1 0x4141E8
// (vtable 0x825CC4, slot 1): 10/14 per step + 750/3750 for owned territory + 1.85 x the target player's
// influence in the step's 9x9-tile cell (EEInfluenceTable [0x918E20], per player the sum of the units'
// type data +0x540). A guard tower counts 175 there, a house 90, so a tower line is just "more base" and
// the route goes straight through it. We add a penalty for every step inside the range of an enemy
// defence building this AI has seen (fair: only what the player could know), so routes go around.
//
// Units: 3-tile grid [[0x9183BC]+0x181724] (+4 cells of 16 bytes {?, begin, end, cap} holding EEUnit*,
//   +8 cell size, +0x10/+0x14 cells per axis). EEUnit: +0x18 owner player, +0x1C/+0x20 tile x/y,
//   +0x2C type data, +0x34 live stats (+0x9C range, +0xD8 attack; floats), +0x14 -> +0xC uint16 per player:
//   low byte != 0 = seen by that player (0x404A8A "explored").
// Type data: +0x4B4 family (2 = building, 29 = fortress), +0x4B8 type id, +0x420 range, +0x510 attack,
//   +0x55C hit points.

#define G_UNIT_GRID_OFS   0x181724
#define VT_ROUTE_PLANNER  0x00825CC4   // EEAIGPGModifierLandAttackSubGroupRoutePlanner
#define FN_ROUTE_COST     0x004141E8   // its slot 1: int thiscall(edge*, node*)
#define VT_ASG_LAND       0x00826CA8   // EECPASGLand
#define FN_ASG_ROUTE      0x0044BC57   // its slot 94: void thiscall(int start[2], int dest[2])
#define ASG_ROUTE_SLOT    94
#define FN_ASG_ROUTE_DONE 0x0044BDBF   // its slot 97: route arrived, waypoints in the deque at +0x60
#define ASG_ROUTE_DONE_SLOT 97

#define MAX_DEFENCES 256
struct defence { int x, y, owner; float range, attack; int type; };
static struct {
    struct defence d[MAX_DEFENCES];
    int n;
    uint32_t refreshed;
} g_threat[16];

static int g_avoid_towers = 1;        // aimod.ini [all] avoid_towers
static int g_avoid_weight = 2000;     // route cost per tower covering a step (a step costs 10-14)
static int g_avoid_margin = 3;        // tiles added to the tower's range

static int is_enemy(void *pl, void *other) {
    if (!other || other == pl) return 0;
    unsigned oi = (unsigned)FIELD(other, PL_INDEX, int);
    return oi < 16 && FIELD(pl, 0x9E4 + oi * 4, int) != 0;
}

static int seen_by(uint8_t *u, void *pl) {
    uint8_t *v = FIELD(u, 0x14, uint8_t *);
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int);
    if (!v || pi >= 16) return 0;
    uint16_t *vis = FIELD(v, 0xC, uint16_t *);
    return vis && (vis[pi] & 0xFF) != 0;
}

// Defence building with a ground attack (AA guns and missile towers only shoot at aircraft).
static int defence_stats(uint8_t *u, float *range, float *attack) {
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    if (!td) return 0;
    int fam = FIELD(td, 0x4B4, int), type = FIELD(td, 0x4B8, int);
    if (fam != 2 && fam != 29) return 0;
    if (type == 145 || type == 147) return 0;
    uint8_t *st = FIELD(u, 0x34, uint8_t *);
    float r = st ? FIELD(st, 0x9C, float) : FIELD(td, 0x420, float);
    float a = st ? FIELD(st, 0xD8, float) : (float)FIELD(td, 0x510, int);
    if (!(a > 0.0f) || !(r > 1.0f) || r > 30.0f) return 0;
    *range = r; *attack = a;
    return 1;
}

static void threat_refresh(void *pl, uint32_t now) {
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (g_threat[pi].refreshed && now >= g_threat[pi].refreshed && now - g_threat[pi].refreshed < 5000) return;
    g_threat[pi].refreshed = now ? now : 1;
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    int n = 0;
    if (grid) {
        uint8_t *cells = FIELD(grid, 4, uint8_t *);
        unsigned count = FIELD(grid, 0x10, unsigned) * FIELD(grid, 0x14, unsigned);
        for (unsigned c = 0; cells && c < count && n < MAX_DEFENCES; c++) {
            uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **), **e = FIELD(cells + c * 16, 8, uint8_t **);
            for (; b && b < e && n < MAX_DEFENCES; b++) {
                uint8_t *u = *b;
                float r, a;
                if (!u || !is_enemy(pl, FIELD(u, 0x18, void *)) || !defence_stats(u, &r, &a) || !seen_by(u, pl)) continue;
                int x = FIELD(u, 0x1C, int), y = FIELD(u, 0x20, int), k;
                for (k = 0; k < n; k++)                     // big buildings sit in several cells
                    if (g_threat[pi].d[k].x == x && g_threat[pi].d[k].y == y) break;
                if (k < n) continue;
                struct defence *d = &g_threat[pi].d[n++];
                d->x = x; d->y = y; d->range = r; d->attack = a;
                d->owner = FIELD(FIELD(u, 0x18, void *), PL_INDEX, int);
                d->type = FIELD(FIELD(u, 0x2C, uint8_t *), 0x4B8, int);
            }
        }
    }
    g_threat[pi].n = n;
}

// Sum of known defences covering tile (x, y), each weighted by its attack (a guard tower = 1).
static float threat_at(unsigned pi, int x, int y) {
    float t = 0;
    for (int k = 0; k < g_threat[pi & 15].n; k++) {
        struct defence *d = &g_threat[pi & 15].d[k];
        float r = d->range + (float)g_avoid_margin;
        int dx = x - d->x, dy = y - d->y;
        if ((float)(dx * dx + dy * dy) <= r * r) t += d->attack / 50.0f;
    }
    return t;
}

typedef int (__thiscall *RouteCostFn)(void *self, int *edge, uint8_t *node);
static struct { unsigned calls, penalised; } g_route_stats[16];
static DWORD g_route_thread;

static int __fastcall hook_route_cost(uint8_t *self, void *edx, int *edge, uint8_t *node) {
    (void)edx;
    int cost = ((RouteCostFn)FN_ROUTE_COST)(self, edge, node);
    unsigned pi = (unsigned)FIELD(self, 0x48, int) & 15;     // planner +0x48 = own player index
    void *pl = g_players[pi];
    if (!g_route_thread) {
        g_route_thread = GetCurrentThreadId();
        if (g_log) fprintf(g_log, "# routes: cost hook runs on thread %lu\n", g_route_thread);
    }
    if (!pl || !node) return cost;
    g_route_stats[pi].calls++;
    float t = threat_at(pi, FIELD(node, 0x3C, int), FIELD(node, 0x40, int));
    if (t > 0) {
        g_route_stats[pi].penalised++;
        if (g_avoid_towers) cost += (int)(t * (float)g_avoid_weight);
    }
    return cost;
}

// Weak spot: when the goal of a march lies inside known defences, pick instead a known enemy building near
// it that no known defence covers (the economy behind the towers, the undefended flank) - the closest to
// the group, not too far from the original goal. The subgroup's target (+0x100: +4/+8 tile, +0xC unit id
// = unit+0x60, +0x10 kind 1 = unit, +0x14 player) is switched to that building, so it attacks it on arrival.
static int g_weak_spots = 1;          // aimod.ini [all] attack_weak_spots
static int g_weak_radius = 60;        // tiles around the original goal searched for a weak spot

static uint8_t *find_weak_spot(void *pl, unsigned pi, const int *start, const int *dest) {
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return NULL;
    int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    if (cs <= 0 || !cells) return NULL;
    short (__thiscall *region)(void *, int *, int) = (void *)FN_REGION;
    short home = region((void *)0x917BC8, (int *)start, 2);
    uint8_t *best = NULL;
    float best_score = 1e30f;
    int r = g_weak_radius;
    for (int cx = (dest[0] - r) / cs; cx <= (dest[0] + r) / cs; cx++)
        for (int cy = (dest[1] - r) / cs; cy <= (dest[1] + r) / cs; cy++) {
            if (cx < 0 || cy < 0 || cx >= nx || cy >= ny) continue;
            uint8_t *cell = cells + (cx * ny + cy) * 16;
            for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **); b++) {
                uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                if (!td || FIELD(td, 0x4B4, int) != 2 || !is_enemy(pl, FIELD(u, 0x18, void *)) || !seen_by(u, pl))
                    continue;
                int pos[2] = { FIELD(u, 0x1C, int), FIELD(u, 0x20, int) };
                float dx = (float)(pos[0] - dest[0]), dy = (float)(pos[1] - dest[1]);
                if (dx * dx + dy * dy > (float)(r * r) || threat_at(pi, pos[0], pos[1]) > 0) continue;
                if (region((void *)0x917BC8, pos, 2) != home) continue;
                float sx = (float)(pos[0] - start[0]), sy = (float)(pos[1] - start[1]);
                float score = sqrtf(sx * sx + sy * sy) + 0.5f * sqrtf(dx * dx + dy * dy);
                if (score < best_score) { best_score = score; best = u; }
            }
        }
    return best;
}

typedef void (__thiscall *AsgRouteFn)(void *self, int *start, int *dest);
static void __fastcall hook_asg_route(uint8_t *self, void *edx, int *start, int *dest) {
    (void)edx;
    void *pl = FIELD(self, 0x190, void *);
    void *target = FIELD(self, 0x11C, void *);
    if (pl && start && dest && g_weak_spots) {
        unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
        float t = threat_at(pi, dest[0], dest[1]);
        uint8_t *u = t > 0 ? find_weak_spot(pl, pi, start, dest) : NULL;
        if (u) {
            uint8_t *tg = self + 0x100;
            if (g_log)
                fprintf(g_log, "# weak spot: P%u goal %d,%d (threat %.1f, target kind %d id %d) -> type %d of P%d at %d,%d\n",
                        pi, dest[0], dest[1], t, FIELD(tg, 0x10, int), FIELD(tg, 0xC, int),
                        FIELD(FIELD(u, 0x2C, uint8_t *), 0x4B8, int), FIELD(FIELD(u, 0x18, void *), PL_INDEX, int),
                        FIELD(u, 0x1C, int), FIELD(u, 0x20, int));
            dest[0] = FIELD(u, 0x1C, int); dest[1] = FIELD(u, 0x20, int);
            FIELD(tg, 4, int) = dest[0]; FIELD(tg, 8, int) = dest[1];
            FIELD(tg, 0xC, int) = FIELD(u, 0x60, int);
            FIELD(tg, 0x10, int) = 1;
        }
    }
    if (g_log && pl && start && dest) {
        unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
        int dx = dest[0] - start[0], dy = dest[1] - start[1];
        // known defences within 4 tiles of the straight line start -> dest
        int on_line = 0;
        for (int k = 0; k < g_threat[pi].n; k++) {
            struct defence *d = &g_threat[pi].d[k];
            float len2 = (float)(dx * dx + dy * dy), u = 0;
            if (len2 > 0) u = ((d->x - start[0]) * dx + (d->y - start[1]) * dy) / len2;
            if (u < 0) u = 0; else if (u > 1) u = 1;
            float px = start[0] + u * dx - d->x, py = start[1] + u * dy - d->y;
            if (px * px + py * py <= (d->range + 2) * (d->range + 2)) on_line++;
        }
        fprintf(g_log, "# route: P%u -> P%d at %.1f min: %d,%d -> %d,%d (%.0f tiles), defences known %d, "
                "on the straight line %d, at the goal %.1f; steps costed %u, penalised %u\n",
                pi, target ? FIELD(target, PL_INDEX, int) : -1, game_time() / 60000.0, start[0], start[1],
                dest[0], dest[1], sqrt((double)(dx * dx + dy * dy)), g_threat[pi].n, on_line,
                threat_at(pi, dest[0], dest[1]), g_route_stats[pi].calls, g_route_stats[pi].penalised);
        fflush(g_log);
    }
    ((AsgRouteFn)FN_ASG_ROUTE)(self, start, dest);
}

// Leash: each unit of an attack subgroup picks its target itself (EECPAttackSubGroupUnit vf5 0x46286F -> vf4
// 0x4620DF scores every enemy the subgroup sees with vf2 0x461854, 0.0 = skip; the best one is attacked via the
// subgroup's slot 7, and while the unit has a target, rec+0x48, it counts as engaged and the march waits).
// Enemy units waiting next to their towers draw the whole army into tower range that way, however good the
// route. So for a land subgroup, an enemy inside known tower range scores 0 - not a target - unless the
// subgroup's own goal (asg+0x100 +4/+8) lies in tower range too; then the attack is meant to go in anyway.
// Unit record: +0x44 unit, +0x48 current target, +0x4C subgroup. Scorer argument: +4 the enemy unit.
#define FN_ASGU_SCORE     0x00461854   // float thiscall(info *)
static const uintptr_t g_asgu_vtables[] = {   // land unit records: base, hero, priests and prophets
    0x00827844, 0x008253C0, 0x00825708, 0x008257F0, 0x00825458, 0x00825540 };
static int g_leash = 1;               // aimod.ini [all] attack_leash
static struct { unsigned skipped, logged; } g_leash_stats[16];

typedef float (__thiscall *AsguScoreFn)(void *self, uint8_t *info);
static float __fastcall hook_asgu_score(uint8_t *self, void *edx, uint8_t *info) {
    (void)edx;
    float s = ((AsguScoreFn)FN_ASGU_SCORE)(self, info);
    if (!g_leash || s == 0.0f || !info) return s;
    uint8_t *sg = FIELD(self, 0x4C, uint8_t *), *enemy = FIELD(info, 4, uint8_t *);
    if (!sg || !enemy || *(uintptr_t *)sg != VT_ASG_LAND) return s;
    void *pl = FIELD(sg, 0x190, void *);
    if (!pl) return s;
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    int ex = FIELD(enemy, 0x1C, int), ey = FIELD(enemy, 0x20, int);
    float t = threat_at(pi, ex, ey);
    if (!(t > 0) || threat_at(pi, FIELD(sg, 0x104, int), FIELD(sg, 0x108, int)) > 0) return s;    g_leash_stats[pi].skipped++;
    if (g_log && g_leash_stats[pi].logged < 10) {
        g_leash_stats[pi].logged++;
        uint8_t *u = FIELD(self, 0x44, uint8_t *);
        fprintf(g_log, "# leash: P%u at %.1f min: unit at %d,%d skips enemy at %d,%d (threat %.1f), goal %d,%d\n",
                pi, game_time() / 60000.0, u ? FIELD(u, 0x1C, int) : -1, u ? FIELD(u, 0x20, int) : -1,
                ex, ey, t, FIELD(sg, 0x104, int), FIELD(sg, 0x108, int));
        fflush(g_log);
    }
    return 0.0f;
}

// Waypoint i of the route deque at asg+0x60 (std::deque of {x, y, ?}, 0x155 per block: +4 map, +8 map size,
// +0xC first index, +0x10 count).
static int *route_waypoint(uint8_t *asg, unsigned i) {
    uint8_t *dq = asg + 0x60;
    unsigned idx = FIELD(dq, 0xC, unsigned) + i, blk = idx / 0x155, msz = FIELD(dq, 8, unsigned);
    uint8_t **map = FIELD(dq, 4, uint8_t **);
    if (!map || !msz) return NULL;
    if (blk >= msz) blk -= msz;
    return map[blk] ? (int *)(map[blk] + (idx % 0x155) * 12) : NULL;
}

typedef void (__thiscall *AsgFn)(void *self);
static void __fastcall hook_asg_route_done(uint8_t *self, void *edx) {
    (void)edx;
    void *pl = FIELD(self, 0x190, void *);
    if (g_log && pl) {
        unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15, n = FIELD(self, 0x70, unsigned), hot = 0;
        char buf[600]; int len = 0;
        for (unsigned i = 0; i < n && i < 400; i++) {
            int *w = route_waypoint(self, i);
            if (!w) break;
            int h = threat_at(pi, w[0], w[1]) > 0;
            hot += h;
            if (len < (int)sizeof buf - 24) len += sprintf(buf + len, " %d,%d%s", w[0], w[1], h ? "!" : "");
        }
        fprintf(g_log, "# route done: P%u %u waypoints, %u in reach of known defences:%s\n", pi, n, hot, buf);
        fflush(g_log);
    }
    ((AsgFn)FN_ASG_ROUTE_DONE)(self);
}

// Soldiers (not buildings, walls, citizens) of a player, from the unit grid.
static int army_size(void *pl) {
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return -1;
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    unsigned count = FIELD(grid, 0x10, unsigned) * FIELD(grid, 0x14, unsigned);
    int n = 0;
    for (unsigned c = 0; cells && c < count; c++)
        for (uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **); b && b < FIELD(cells + c * 16, 8, uint8_t **); b++) {
            uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
            if (!td || FIELD(u, 0x18, void *) != pl) continue;
            // units only sit in the cell of their position; families as in the statistics code 0x5EB000/0x55DD66
            if (FIELD(u, 0x1C, int) / FIELD(grid, 8, int) != (int)(c / FIELD(grid, 0x14, unsigned)) ||
                FIELD(u, 0x20, int) / FIELD(grid, 8, int) != (int)(c % FIELD(grid, 0x14, unsigned))) continue;
            int fam = FIELD(td, 0x4B4, int);
            if (fam == 2 || fam == 6 || fam == 0x13 || fam == 0x16 || fam == 0x18 || fam == 0x19 || fam == 0x1D || fam == 0 || fam == 1)
                continue;
            n++;
        }
    return n;
}

static void attack_tick(void *pl, uint32_t now) {
    threat_refresh(pl, now);   // also with avoid_towers=0: the route log compares both
    {
        unsigned i = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
        if (g_log && now / 300000 != g_mil[i].last_log / 300000) {
            fprintf(g_log, "# army: P%u at %u min: %d soldiers; military requests %u, trained %u, failed short of "
                    "food %u wood %u stone %u gold %u iron %u; budget left %d/%d/%d/%d/%d\n", i, now / 60000,
                    army_size(pl), g_mil[i].req, g_mil[i].trained, g_mil[i].short_of[2], g_mil[i].short_of[3],
                    g_mil[i].short_of[4], g_mil[i].short_of[5], g_mil[i].short_of[6], (int)g_bud[i].mil[2],
                    (int)g_bud[i].mil[3], (int)g_bud[i].mil[4], (int)g_bud[i].mil[5], (int)g_bud[i].mil[6]);
            fprintf(g_log, "# army: P%u failed types:", i);
            for (int k = 0; k < 4; k++)
                if (g_mil[i].fcount[k])
                    fprintf(g_log, " type %d from %06x x%u", g_mil[i].ftype[k] & 0x7FF,
                            (unsigned)g_mil[i].ftype[k] >> 11, g_mil[i].fcount[k]);
            fputc('\n', g_log);
            fprintf(g_log, "# leash: P%u: enemies in tower range skipped as targets %u times\n", i,
                    g_leash_stats[i].skipped);
            memset(&g_mil[i], 0, sizeof g_mil[i]);
        }
        g_mil[i].last_log = now;
    }
    // battle statistics every 10 min: series n = Language.dll string 31201+n (1 buildings lost, 2 buildings
    // destroyed, 3 units killed, 4 units lost, 5 conversions, 7..11 resources, 12 epoch, 13 army, 14 citizens)
    static uint32_t last_stats[16];
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (g_log && now / 600000 != last_stats[pi] / 600000) {
        // EEPlayerStatistics (player+0x4A8): series i at +0x18 + i*0x18 {?, begin, end, ..} of 8-byte samples,
        // latest value = first dword of the last sample (0x55DCD3)
        fprintf(g_log, "# battle: P%u at %u min:", pi, now / 60000);
        uint8_t *st = (uint8_t *)pl + 0x4A8;
        for (int k = 0; k < 30; k++) {
            uint8_t *b = FIELD(st, 0x1C + k * 0x18, uint8_t *), *e = FIELD(st, 0x20 + k * 0x18, uint8_t *);
            int ok = b && e > b && (e - b) % 8 == 0 && e - b < (1 << 20) && !IsBadReadPtr(e - 8, 8);
            fprintf(g_log, " %d", ok ? FIELD(e, -8, int) : 0);
        }
        fputc('\n', g_log);
        fflush(g_log);
    }
    last_stats[pi] = now;
}

static int patch_vtable(uintptr_t vt, int slot, uintptr_t expected, void *hook) {
    void **p = (void **)(vt + slot * 4);
    if (*p != (void *)expected) return 0;
    DWORD old;
    if (!VirtualProtect(p, 4, PAGE_READWRITE, &old)) return 0;
    *p = hook;
    VirtualProtect(p, 4, old, &old);
    return 1;
}

static void attack_install(const char *aimod_ini, const char *autotest_ini) {
    g_avoid_towers = cfg_int(aimod_ini, autotest_ini, 1, "avoid_towers", 1);
    g_avoid_weight = cfg_int(aimod_ini, autotest_ini, 1, "avoid_towers_weight", 2000);
    g_avoid_margin = cfg_int(aimod_ini, autotest_ini, 1, "avoid_towers_margin", 3);
    g_weak_spots = cfg_int(aimod_ini, autotest_ini, 1, "attack_weak_spots", 1);
    g_weak_radius = cfg_int(aimod_ini, autotest_ini, 1, "attack_weak_spot_radius", 60);
    g_leash = cfg_int(aimod_ini, autotest_ini, 1, "attack_leash", 1);
    int a = patch_vtable(VT_ROUTE_PLANNER, 1, FN_ROUTE_COST, hook_route_cost);
    int b = patch_vtable(VT_ASG_LAND, ASG_ROUTE_SLOT, FN_ASG_ROUTE, hook_asg_route);
    int c = patch_vtable(VT_ASG_LAND, ASG_ROUTE_DONE_SLOT, FN_ASG_ROUTE_DONE, hook_asg_route_done);
    int d = 1;
    for (unsigned k = 0; k < sizeof g_asgu_vtables / sizeof *g_asgu_vtables; k++)
        d &= patch_vtable(g_asgu_vtables[k], 2, FN_ASGU_SCORE, hook_asgu_score);
    if (g_log) fprintf(g_log, "# attack routing: avoid_towers=%d weight=%d margin=%d weak_spots=%d radius=%d leash=%d hooks %d/4\n",
                       g_avoid_towers, g_avoid_weight, g_avoid_margin, g_weak_spots, g_weak_radius, g_leash, a + b + c + d);
}

// threat <player>: the enemy defences this AI knows about
static void cmd_threat(char *out, const char *args) {
    int p = atoi(args);
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    if (!pl) { REPLY("error: no AI player %d\n", p); return; }
    threat_refresh(pl, 0);
    REPLY("P%d knows %d enemy defences (route steps costed %u, penalised %u)\n", p, g_threat[p].n,
          g_route_stats[p].calls, g_route_stats[p].penalised);
    for (int k = 0; k < g_threat[p].n; k++) {
        struct defence *d = &g_threat[p].d[k];
        REPLY("  type %3d of P%d at %d,%d range %.1f attack %.0f\n", d->type, d->owner, d->x, d->y, d->range, d->attack);
    }
}

// Enemy buildings of AI player P (all, seen or not): owner, type, tile, seen by P, known threat on them.
static void cmd_targets(char *out, const char *args) {
    int p = atoi(args);
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    if (!pl) { REPLY("error: no AI player %d\n", p); return; }
    threat_refresh(pl, 0);
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) { REPLY("error: no map\n"); return; }
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    unsigned count = FIELD(grid, 0x10, unsigned) * FIELD(grid, 0x14, unsigned), n = 0;
    for (unsigned c = 0; cells && c < count; c++)
        for (uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **); b && b < FIELD(cells + c * 16, 8, uint8_t **); b++) {
            uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
            if (!td || (FIELD(td, 0x4B4, int) != 2 && FIELD(td, 0x4B4, int) != 29) || !is_enemy(pl, FIELD(u, 0x18, void *)))
                continue;
            int x = FIELD(u, 0x1C, int), y = FIELD(u, 0x20, int);
            if (strlen(out) > 15000) { REPLY("...\n"); return; }
            REPLY("  type %3d of P%d at %d,%d %s threat %.1f\n", FIELD(td, 0x4B8, int),
                  FIELD(FIELD(u, 0x18, void *), PL_INDEX, int), x, y, seen_by(u, pl) ? "seen  " : "unseen", threat_at(p, x, y));
            n++;
        }
    REPLY("P%d: %u enemy buildings\n", p, n);
}

// Soldiers of player P (any player, AI or not) grouped by current goal (unit+0x1EC): count, centre, and the
// goal's first dwords of the first unit (to find what it is after).
static void cmd_army(char *out, const char *args) {
    int p = atoi(args);
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) { REPLY("error: no map\n"); return; }
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    unsigned count = FIELD(grid, 0x10, unsigned) * FIELD(grid, 0x14, unsigned);
    struct { const char *goal; int n; long sx, sy; uint8_t *first; } g[32];
    int ng = 0, total = 0;
    for (unsigned c = 0; cells && c < count; c++)
        for (uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **); b && b < FIELD(cells + c * 16, 8, uint8_t **); b++) {
            uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
            void *owner = u ? FIELD(u, 0x18, void *) : NULL;
            if (!td || !owner || FIELD(owner, PL_INDEX, int) != p) continue;
            if (FIELD(u, 0x1C, int) / FIELD(grid, 8, int) != (int)(c / FIELD(grid, 0x14, unsigned)) ||
                FIELD(u, 0x20, int) / FIELD(grid, 8, int) != (int)(c % FIELD(grid, 0x14, unsigned))) continue;
            int fam = FIELD(td, 0x4B4, int);
            if (fam == 2 || fam == 6 || fam == 0x13 || fam == 0x16 || fam == 0x18 || fam == 0x19 || fam == 0x1D || fam == 0 || fam == 1)
                continue;
            const char *gn = rtti_name(FIELD(u, 0x1EC, void *));
            int j = 0;
            while (j < ng && g[j].goal != gn) j++;
            if (j == ng) { if (ng == 32) continue; g[ng].goal = gn; g[ng].n = 0; g[ng].sx = g[ng].sy = 0; g[ng].first = u; ng++; }
            g[j].n++; g[j].sx += FIELD(u, 0x1C, int); g[j].sy += FIELD(u, 0x20, int);
            total++;
        }
    REPLY("P%d: %d soldiers\n", p, total);
    for (int j = 0; j < ng; j++) {
        REPLY("  %3d x %-28s centre %ld,%ld", g[j].n, g[j].goal, g[j].sx / g[j].n, g[j].sy / g[j].n);
        uint32_t *gd = FIELD(g[j].first, 0x1EC, uint32_t *);
        if (gd) for (int k = 0; k < 24; k++) REPLY(" %08x", gd[k]);
        REPLY("\n");
    }
}
