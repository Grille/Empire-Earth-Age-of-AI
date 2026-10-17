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

#define VT_ROUTE_PLANNER  0x00825CC4   // EEAIGPGModifierLandAttackSubGroupRoutePlanner
#define FN_ROUTE_COST     0x004141E8   // its slot 1: int thiscall(edge*, node*)
#define VT_ASG_LAND       0x00826CA8   // EECPASGLand
#define FN_ASG_ROUTE      0x0044BC57   // its slot 94: void thiscall(int start[2], int dest[2])
#define ASG_ROUTE_SLOT    94
#define FN_ASG_ROUTE_DONE 0x0044BDBF   // its slot 97: route arrived, waypoints in the deque at +0x60
#define ASG_ROUTE_DONE_SLOT 97

#define MAX_DEFENCES 256
struct defence { int x, y, owner; float range, attack; int type; uint8_t *u; };
static struct {
    struct defence d[MAX_DEFENCES];
    int n;
    uint32_t refreshed;
} g_threat[16];

static int g_avoid_towers = 1;        // aimod.ini [all] avoid_towers
static int g_avoid_weight = 2000;     // route cost per tower covering a step (a step costs 10-14)
static int g_avoid_margin = 6;        // tiles added to the tower's range

static int is_enemy(void *pl, void *other) {
    if (!other || other == pl) return 0;
    unsigned oi = (unsigned)FIELD(other, PL_INDEX, int);
    return oi < 16 && FIELD(pl, 0x9E4 + oi * 4, int) != 0;
}

// What player pl knows of unit u. The count (u +0x14 -> +0xC, per player slot) is how many of pl's (and its allies')
// units see u now - plus a floor every unit of the human carries in the AIs' slots, everywhere and always (the AIs
// see all of the human's units; docs/engine-notes.md). [all] honest_vision (intel.c): seen = above that floor, or a
// building pl has seen before (still where it was - a human remembers it on the map); 0 (the default): the count as
// the game keeps it - all-knowing about the human.
static int g_honest_vision = 0;
static uint8_t g_vis_floor[16][16];               // [viewer][owner]: the least count among the owner's units (intel.c)
static int intel_remembers(void *pl, uint8_t *u);   // intel.c: a building pl has seen
static void intel_soldiers(void *pl, int x, int y, int r, float *hp, float *att, float *pw, int *n);   // intel.c
static void intel_army(void *pl, float *hp, float *att, float *pw, int *n);
static uint32_t intel_looked_ago(void *pl, int x, int y);
static int seen_by(uint8_t *u, void *pl) {
    uint8_t *v = FIELD(u, 0x14, uint8_t *);
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int);
    if (!v || pi >= 16) return 0;
    uint16_t *vis = FIELD(v, 0xC, uint16_t *);
    if (!vis) return 0;
    unsigned c = vis[pi] & 0xFF;
    if (!g_honest_vision) return c != 0;
    void *o = FIELD(u, 0x18, void *);
    unsigned oi = o ? (unsigned)FIELD(o, PL_INDEX, int) & 15 : 0;
    if (c > g_vis_floor[pi][oi]) return 1;
    int fam = FIELD(FIELD(u, 0x2C, uint8_t *), 0x4B4, int);
    return (fam == 2 || fam == 29) && intel_remembers(pl, u);
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
                d->u = u;
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

// Walls and gates are no weak spot (no economy behind them, only a detour).
static int is_wall(int type) {
    return (type >= 467 && type <= 473) || type == 294 || type == 442 || type == 443 || type == 444 ||
           type == 454 || type == 40 || type == 48;
}

// Only goals at an enemy building are retargeted: the subgroup also marches to other goals (enemy armies,
// own territory to defend), which must not be diverted.
static int building_at(void *pl, const int *dest, int r) {
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return 0;
    int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    if (cs <= 0 || !cells) return 0;
    for (int cx = (dest[0] - r) / cs; cx <= (dest[0] + r) / cs; cx++)
        for (int cy = (dest[1] - r) / cs; cy <= (dest[1] + r) / cs; cy++) {
            if (cx < 0 || cy < 0 || cx >= nx || cy >= ny) continue;
            uint8_t *cell = cells + (cx * ny + cy) * 16;
            for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **); b++) {
                uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                if (!td || FIELD(td, 0x4B4, int) != 2 || is_wall(FIELD(td, 0x4B8, int)) || !is_enemy(pl, FIELD(u, 0x18, void *)))
                    continue;
                int dx = FIELD(u, 0x1C, int) - dest[0], dy = FIELD(u, 0x20, int) - dest[1];
                if (dx * dx + dy * dy <= r * r) return 1;
            }
        }
    return 0;
}

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
                if (!td || FIELD(td, 0x4B4, int) != 2 || is_wall(FIELD(td, 0x4B8, int)) || !is_enemy(pl, FIELD(u, 0x18, void *)) ||
                    !seen_by(u, pl))
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

// Breach: the route planner works on ~10-tile sectors and ignores walls, so an army marches straight to an enemy
// wall and piles up in front of it - next to the towers that guard the gap. When the straight line to the goal
// crosses a known enemy wall, the subgroup first attacks a wall segment outside known tower range, the one with
// the shortest detour along that wall (find_breach); the scorer makes that segment the units' target (walls score
// below zero otherwise). Walls are tough: without siege the breach takes long or fails.
static int g_breach = 1;              // aimod.ini [all] attack_breach
static struct { void *sg; int x, y; } g_breaches[64];   // subgroup -> wall tile it is to breach

static void breach_note(void *sg, int x, int y) {
    int free_k = -1;
    for (int k = 0; k < 64; k++) {
        if (g_breaches[k].sg == sg) { g_breaches[k].x = x; g_breaches[k].y = y; if (x < 0) g_breaches[k].sg = NULL; return; }
        if (!g_breaches[k].sg && free_k < 0) free_k = k;
    }
    if (x < 0) return;
    if (free_k < 0) free_k = (int)((uintptr_t)sg >> 4) & 63;   // full: overwrite one
    g_breaches[free_k].sg = sg; g_breaches[free_k].x = x; g_breaches[free_k].y = y;
}

// before the game deletes subgroups of player pl (diplo.c: attacks called off): forget their breaches
static void breach_forget(void *pl) {
    for (int k = 0; k < 64; k++)
        if (g_breaches[k].sg && FIELD(g_breaches[k].sg, 0x190, void *) == pl) g_breaches[k].sg = NULL;
}

static int breach_of(void *sg, int *x, int *y) {
    for (int k = 0; k < 64; k++)
        if (g_breaches[k].sg == sg) { *x = g_breaches[k].x; *y = g_breaches[k].y; return 1; }
    return 0;
}

static uint8_t *wall_at(void *pl, int x, int y) {
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return NULL;
    int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    if (cs <= 0 || !cells || x < 0 || y < 0 || x / cs >= nx || y / cs >= ny) return NULL;
    uint8_t *cell = cells + ((x / cs) * ny + y / cs) * 16;
    for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **); b++) {
        uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
        if (td && FIELD(u, 0x1C, int) == x && FIELD(u, 0x20, int) == y && is_wall(FIELD(td, 0x4B8, int)) &&
            is_enemy(pl, FIELD(u, 0x18, void *)) && seen_by(u, pl))
            return u;
    }
    return NULL;
}

// First known enemy wall tile on the straight line start -> dest (half-tile steps), or NULL.
static uint8_t *wall_crossing(void *pl, const int *start, const int *dest) {
    int dx = dest[0] - start[0], dy = dest[1] - start[1];
    int n = 2 * (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
    for (int i = 1; i < n; i++) {
        uint8_t *w = wall_at(pl, start[0] + (dx * i + (dx >= 0 ? n / 2 : -n / 2)) / n,
                             start[1] + (dy * i + (dy >= 0 ? n / 2 : -n / 2)) / n);
        if (w) return w;
    }
    return NULL;
}

// Follows the crossed wall (8-connected wall tiles) up to 60 tiles from the crossing and takes the segment outside
// known tower range with the shortest detour start -> segment -> dest.
static uint8_t *find_breach(void *pl, unsigned pi, const int *start, const int *dest, const uint8_t *cross) {
    enum { MAXQ = 512 };
    static int qx[MAXQ], qy[MAXQ];
    int head = 0, tail = 0, cx = FIELD(cross, 0x1C, int), cy = FIELD(cross, 0x20, int);
    qx[tail] = cx; qy[tail++] = cy;
    uint8_t *best = NULL;
    float best_score = 1e30f;
    while (head < tail) {
        int x = qx[head], y = qy[head++];
        uint8_t *u = wall_at(pl, x, y);
        if (!u) continue;
        if (!(threat_at(pi, x, y) > 0)) {
            float ax = (float)(x - start[0]), ay = (float)(y - start[1]), bx = (float)(dest[0] - x), by = (float)(dest[1] - y);
            float score = sqrtf(ax * ax + ay * ay) + sqrtf(bx * bx + by * by);
            if (score < best_score) { best_score = score; best = u; }
        }
        for (int ddx = -1; ddx <= 1; ddx++)
            for (int ddy = -1; ddy <= 1; ddy++) {
                int nx = x + ddx, ny = y + ddy, seen = 0;
                if ((nx - cx) * (nx - cx) + (ny - cy) * (ny - cy) > 60 * 60 || tail >= MAXQ) continue;
                for (int k = 0; k < tail && !seen; k++) seen = qx[k] == nx && qy[k] == ny;
                if (!seen && wall_at(pl, nx, ny)) { qx[tail] = nx; qy[tail++] = ny; }
            }
    }
    return best;
}

static void diplo_cancel_attacks(int ai, int j);   // diplo.c
typedef void (__thiscall *AsgRouteFn)(void *self, int *start, int *dest);
// CITIZEN ATTACKS ([all] citizen_attacks, default 0): an AI that trains no soldiers (short of wood: 1237 requests in
// 5 minutes, none trained) still launched its attacks on time - and the groups took citizens: 34 of them walked
// into the player's army at the target, the soldiers stayed home. A subgroup's members are records at +0x54..+0x58
// (record +0x44 = unit); at least half of them citizens: that AI's attacks on that target are called off at the next
// safe point (diplo.c, the same call as for peace: units sent home and handed back) - the citizens go back to work.
// Checked when a subgroup is routed and every 5 s over the running attacks (citizen_attack_scan): the last leg of an
// attack - from the route's end to the target building - is an order of its own (move to 101.5,98.5 beside the
// player's Town Center, all 43 of Hedda's citizens) that no route computation sees. Attack commander = player
// +0xB8C +0x48: its groups a deque at +4 (count +0x14), a group's target player +0x48, its subgroups a vector +8..+0xC
// (0x4510F2 / 0x454B90, the call-off).
static int g_citizen_attacks;
static void cancel_citizen_attack(int ai, int target);   // diplo.c
static const char *g_cancel_why[16][16];   // why the next call-off (NULL: citizens) - diplo_safe_point logs it
static void subgroup_count(uint8_t *sg, int *n, int *cit) {   // members of an attack subgroup, citizens among them
    uint8_t **b = FIELD(sg, 0x54, uint8_t **), **e = FIELD(sg, 0x58, uint8_t **);
    for (uint8_t **r = b; b && r < e && *n < 1000; r++) {
        uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
        if (!td) continue;
        (*n)++;
        *cit += is_citizen_type(FIELD(td, 0x4B8, int));
    }
}
static void citizen_attack_scan(void *pl, uint32_t now) {
    static uint32_t last[16];
    unsigned i = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (g_citizen_attacks || (now >= last[i] && now - last[i] < 5000)) return;
    last[i] = now;
    uint8_t *cbm = FIELD(pl, 0xB8C, uint8_t *), *cmd = cbm ? FIELD(cbm, 0x48, uint8_t *) : NULL;
    if (!cmd) return;
    uint8_t *dq = cmd + 4;
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        uint8_t *g = deque_at(dq, first + k), *target = g ? FIELD(g, 0x48, uint8_t *) : NULL;
        if (!target) continue;
        int n = 0, cit = 0;
        for (uint8_t **s = FIELD(g, 8, uint8_t **); s && s < FIELD(g, 0xC, uint8_t **); s++)
            if (*s) subgroup_count(*s, &n, &cit);
        if (n && cit * 2 >= n) {
            cancel_citizen_attack((int)i, FIELD(target, PL_INDEX, int) & 15);
            if (g_log) fprintf(g_log, "# attack: P%u's attack on P%d at %.1f min is %d citizens of %d - called off\n", i,
                               FIELD(target, PL_INDEX, int), now / 60000.0, cit, n);
        }
    }
}
static void __fastcall hook_asg_route(uint8_t *self, void *edx, int *start, int *dest) {
    (void)edx;
    void *pl = FIELD(self, 0x190, void *);
    void *target = FIELD(self, 0x11C, void *);
    if (pl && target && !g_citizen_attacks) {
        int n = 0, cit = 0;
        subgroup_count(self, &n, &cit);
        if (n && cit * 2 >= n) {
            cancel_citizen_attack(FIELD(pl, PL_INDEX, int) & 15, FIELD(target, PL_INDEX, int) & 15);
            if (g_log) fprintf(g_log, "# attack: P%d's subgroup on P%d is %d citizens of %d - attack called off\n",
                               FIELD(pl, PL_INDEX, int), FIELD(target, PL_INDEX, int), cit, n);
        }
    }
    if (pl && start && dest && g_weak_spots) {
        unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
        float t = threat_at(pi, dest[0], dest[1]);
        uint8_t *u = t > 0 && building_at(pl, dest, 4) ? find_weak_spot(pl, pi, start, dest) : NULL;
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
    if (pl && start && dest && g_breach) {
        unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
        uint8_t *cross = wall_crossing(pl, start, dest);
        uint8_t *w = cross ? find_breach(pl, pi, start, dest, cross) : NULL;
        breach_note(self, w ? FIELD(w, 0x1C, int) : -1, w ? FIELD(w, 0x20, int) : -1);
        if (w) {
            uint8_t *tg = self + 0x100;
            if (g_log)
                fprintf(g_log, "# breach: P%u goal %d,%d: wall crossed at %d,%d (threat %.1f) -> attack wall at %d,%d\n",
                        pi, dest[0], dest[1], FIELD(cross, 0x1C, int), FIELD(cross, 0x20, int),
                        threat_at(pi, FIELD(cross, 0x1C, int), FIELD(cross, 0x20, int)), FIELD(w, 0x1C, int), FIELD(w, 0x20, int));
            dest[0] = FIELD(w, 0x1C, int); dest[1] = FIELD(w, 0x20, int);
            FIELD(tg, 4, int) = dest[0]; FIELD(tg, 8, int) = dest[1];
            FIELD(tg, 0xC, int) = FIELD(w, 0x60, int);
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
    // the target player is chosen once, at the launch (0x46F4FF): after peace or an alliance the march goes on -
    // diplo.c calls such attacks off at the next safe point (never here: it deletes this subgroup)
    int hostile = target && FIELD(pl, 0x9E4 + (FIELD(target, PL_INDEX, int) & 15) * 4, int);
    if (pl && target && !hostile) {
        if (g_log) fprintf(g_log, "# route: P%d marches on P%d, no enemy any more\n", FIELD(pl, PL_INDEX, int), FIELD(target, PL_INDEX, int));
        diplo_cancel_attacks(FIELD(pl, PL_INDEX, int) & 15, FIELD(target, PL_INDEX, int) & 15);
    }
    if (pl && hostile) persona_attack(pl, target);
    if (g_chat && pl && start && dest) {
        unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
        int dx = dest[0] - start[0], dy = dest[1] - start[1];
        ai_say(pl, CHAT_ROUTE, "[ai] marching on P%d at %d,%d (%.0f tiles), %d enemy defences known",
               target ? FIELD(target, PL_INDEX, int) : -1, dest[0], dest[1], sqrt((double)(dx * dx + dy * dy)), g_threat[pi].n);
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

static int g_siege = 1;               // aimod.ini [all] attack_siege (siege controller below; also rams here)
static unsigned g_ram_waits, g_air_skips;
static int g_ignore_air = 0;          // aimod.ini [all] attack_ignore_air
static int is_ram(int type) { return type == 102 || type == 287 || type == 158; }

// Enemy soldiers (no citizens, no siege) of player pl's enemies within r tiles of (x, y)?
static int guarded(void *pl, int x, int y, int r) {
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!pl || !grid) return 0;
    int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    if (cs <= 0 || !cells) return 0;
    for (int cx = (x - r) / cs; cx <= (x + r) / cs; cx++)
        for (int cy = (y - r) / cs; cy <= (y + r) / cs; cy++) {
            if (cx < 0 || cy < 0 || cx >= nx || cy >= ny) continue;
            uint8_t *cell = cells + (cx * ny + cy) * 16;
            for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **); b++) {
                uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                if (!td || FIELD(td, 0x4B4, int) != 5 || FIELD(u, 0x3C, int) <= 0 || !is_enemy(pl, FIELD(u, 0x18, void *)))
                    continue;
                int t = FIELD(td, 0x4B8, int);
                if (t == 32 || t == 480 || is_ram(t) || (t >= 102 && t <= 107) || t == 141 || t == 142 || t == 602 || t == 603)
                    continue;
                int dx = FIELD(u, 0x1C, int) - x, dy = FIELD(u, 0x20, int) - y;
                if (dx * dx + dy * dy <= r * r) return 1;
            }
        }
    return 0;
}

typedef float (__thiscall *AsguScoreFn)(void *self, uint8_t *info);
static float __fastcall hook_asgu_score(uint8_t *self, void *edx, uint8_t *info) {
    (void)edx;
    float s = ((AsguScoreFn)FN_ASGU_SCORE)(self, info);
    uint8_t *sg = FIELD(self, 0x4C, uint8_t *), *enemy = info ? FIELD(info, 4, uint8_t *) : NULL;
    if (!sg || !enemy || *(uintptr_t *)sg != VT_ASG_LAND) return s;
    // rams (melee, helpless against soldiers): no building while enemy soldiers guard it - they wait with the
    // army until the guards are dealt with
    uint8_t *me = FIELD(self, 0x44, uint8_t *), *mtd = me ? FIELD(me, 0x2C, uint8_t *) : NULL;
    if (g_siege && mtd && is_ram(FIELD(mtd, 0x4B8, int)) && s != 0.0f &&
        guarded(FIELD(sg, 0x190, void *), FIELD(enemy, 0x1C, int), FIELD(enemy, 0x20, int), 5)) {
        g_ram_waits++;
        return 0.0f;
    }
    // aircraft out of reach: no target. The scorer rates them like any enemy; the subgroup's members chase planes
    // circling over their own town and its march waits while any member is engaged (user's game: 25 of the player's
    // planes over P3's town kept its main force at its docks for 9+ minutes, AA tanks running after them). In reach
    // (its range + 3 tiles) they still shoot.
    uint8_t *etd = FIELD(enemy, 0x2C, uint8_t *);
    int efam = etd ? FIELD(etd, 0x4B4, int) : 0;
    if (g_ignore_air && s != 0.0f && me && (efam == 3 || efam == 7 || efam == 17)) {
        uint8_t *st = FIELD(me, 0x34, uint8_t *);
        float r = (st ? FIELD(st, 0x9C, float) : FIELD(mtd, 0x420, float)) + 3.0f;
        int dx = FIELD(enemy, 0x1C, int) - FIELD(me, 0x1C, int), dy = FIELD(enemy, 0x20, int) - FIELD(me, 0x20, int);
        if ((float)(dx * dx + dy * dy) > r * r) {
            g_air_skips++;
            return 0.0f;
        }
    }
    // breach: the scorer rates walls below zero (never chosen while anything else is around); the subgroup's
    // breach segment and its neighbours become the best targets
    int bx, by;
    if (etd && is_wall(FIELD(etd, 0x4B8, int)) && breach_of(sg, &bx, &by) &&
        abs(FIELD(enemy, 0x1C, int) - bx) <= 1 && abs(FIELD(enemy, 0x20, int) - by) <= 1)
        return FIELD(enemy, 0x1C, int) == bx && FIELD(enemy, 0x20, int) == by ? 1000.0f : 500.0f;
    if (!g_leash || s == 0.0f) return s;
    void *pl = FIELD(sg, 0x190, void *);
    if (!pl) return s;
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    int ex = FIELD(enemy, 0x1C, int), ey = FIELD(enemy, 0x20, int);
    float t = threat_at(pi, ex, ey);
    if (!(t > 0) || threat_at(pi, FIELD(sg, 0x104, int), FIELD(sg, 0x108, int)) > 0) return s;
    g_leash_stats[pi].skipped++;
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

// Siege like a player: the AI never sends its siege weapons into its attacks (they stay at home). Every 2 s each
// AI picks a siege target - the nearest known enemy defence its siege outranges, or a wall segment one of its
// subgroups is to breach - and a firing position at (siege range - 0.7) from it, on the side facing the siege,
// where no other known defence reaches. The siege moves there and attacks; idle soldiers (1.5 per siege
// weapon, at least 4) guard the firing position instead of charging in.
// Orders are given like the subgroup's (EECPAttackSubGroup::vf57 0x45CBEA): unit 0x5F05BD(goal, append, 0),
// 0x5EFCF0(priority), typedata slot 29 (unit). Goals: EEUGAttackUnit (new 0x40, ctor 0x60AD45(attacker, target,
// 1)), EEUGLocation (new 0x38, ctor 0x614CE6(float xy[2], 1, 1, 0, 0.0, 0, 0); +0x20/+0x24 = xy).
#define VT_GOAL_ATTACK   0x00831968
#define VT_GOAL_LOCATION 0x00831E64
static struct {
    int tx, ty, ttype, px, py; uint32_t last; unsigned logged;
    int wave;                    // a wave is out: newcomers wait at the gathering point
    void *prev[24]; int nprev;   // the siege weapons at the last look (losses: those gone)
    uint32_t loss_t[3];          // the last 3 losses; all within 10 min: training paused
    uint32_t pause_until, seen_t;
    unsigned waves, lost, pauses, held, logged_min;
} g_siege_st[16];

static int is_siege(int type) {
    return type == 104 || type == 105 || type == 106 || type == 107 || type == 141 || type == 142 || type == 602 || type == 603;
}

static uint8_t *unit_at(int x, int y, int type) {
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return NULL;
    int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    if (cs <= 0 || !cells || x < 0 || y < 0 || x / cs >= nx || y / cs >= ny) return NULL;
    uint8_t *cell = cells + ((x / cs) * ny + y / cs) * 16;
    for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **); b++) {
        uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
        if (td && FIELD(u, 0x1C, int) == x && FIELD(u, 0x20, int) == y && FIELD(td, 0x4B8, int) == type && FIELD(u, 0x3C, int) > 0)
            return u;
    }
    return NULL;
}

static int unit_goal(uint8_t *u, void *goal, int append, int prio) {
    if (!goal || FIELD(u, 0x3C, int) <= 0 || FIELD(u, 0x1C, int) == -10000 || FIELD(u, 0x33B, char)) return 0;
    ((void (__thiscall *)(void *, void *, char, int))0x005F05BD)(u, goal, (char)append, 0);
    ((void (__thiscall *)(void *, int))0x005EFCF0)(u, prio);
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    ((void (__thiscall *)(void *, void *))(*(void ***)td)[29])(td, u);
    return 1;
}

static void *goal_attack(uint8_t *u, uint8_t *target) {
    void *g = ((void *(__cdecl *)(unsigned))0x00683718)(0x40);
    return g ? ((void *(__thiscall *)(void *, void *, void *, int))0x0060AD45)(g, u, target, 1) : NULL;
}

static void *goal_move(float x, float y) {
    float xy[2] = { x, y };
    void *g = ((void *(__cdecl *)(unsigned))0x00683718)(0x38);
    return g ? ((void *(__thiscall *)(void *, float *, int, char, char, float, char, char))0x00614CE6)(g, xy, 1, 1, 0, 0.0f, 0, 0) : NULL;
}

// CITIZENS AND TOWERS (aimod.ini [all] avoid_towers_citizens, default 1). The game's site searches check for enemies
// only at the site itself (0x4660B9), and once a citizen gathers somewhere it walks the same way until it dies - past
// enemy towers too. With slow projectiles leading their targets (aim.c) every such trip is a loss. Danger = the site
// lies within a known enemy defence's range + 2 tiles, or the straight way from the nearest drop site to it crosses
// one (g_threat: defences this AI has seen). New sites: the searches (sites.c) refuse dangerous ones. Citizens already
// gathering at one are sent back to their drop site every 10 s (as a player would); there the assignment gives them
// new work. Also gatherers and builders with enemy soldiers within 6 tiles of their site (raiders_near) - a few
// bowmen at a raided field got every citizen sent to rebuild or farm it, one by one. Builders of any building with
// enemy soldiers within 8 tiles of it, from every list (also the idle and build lists), every 4 s: a player's army in
// the base got the AI's citizens sent into it one by one to a Capitol it was rebuilding, a tower, walls (112 citizens
// in 20 minutes). Builders of walls and towers near known enemy towers only (no soldiers there) are left alone: the
// wall manager sends them again and again.
static int g_avoid_citizens = 1;
static int nearest_drop(void *pl, int x, int y, int food, int out[2]);   // sites.c
static int tower_covers(unsigned pi, float x, float y) {
    for (int k = 0; k < g_threat[pi].n; k++) {
        struct defence *d = &g_threat[pi].d[k];
        float dx = x - (float)d->x, dy = y - (float)d->y, r = d->range + 2.0f;
        if (dx * dx + dy * dy <= r * r) return 1;
    }
    return 0;
}
static int route_covered(void *pl, int fx, int fy, int tx, int ty) {   // fx = -10000: the site only
    if (!pl) return 0;
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    threat_refresh(pl, game_time());
    if (!g_threat[pi].n) return 0;
    if (tower_covers(pi, (float)tx, (float)ty)) return 1;
    if (fx == -10000) return 0;
    float dx = (float)(tx - fx), dy = (float)(ty - fy);
    int n = (int)sqrtf(dx * dx + dy * dy);
    for (int t = 2; t < n; t++)   // not the first tiles: the drop site itself may be in range (towers at the base)
        if (tower_covers(pi, (float)fx + dx * (float)t / (float)n, (float)fy + dy * (float)t / (float)n)) return 1;
    return 0;
}
static int route_danger(void *pl, int fx, int fy, int tx, int ty) {
    return g_avoid_citizens && route_covered(pl, fx, fy, tx, ty);
}
// Raiders: enemy soldiers (land units or ships with an attack, not citizens) this AI can see within r tiles of x, y -
// or within their own range + RAID_MARGIN if that is more (up to RAID_MAX): 8 tiles kept citizens safe from clubmen,
// but in the later ages riflemen and artillery shoot from there already.
// Not the game's 0x4660B9: that reads the enemy influence, which enemy buildings make too - builders at the front were
// sent back every 10 s (166 times in 13 minutes).
#define RAID_MARGIN 3
#define RAID_MAX    15
#define RAID_FAR    27   // citizen_threats: artillery (12-14), mortars (10-11), ships (24) at their own reach
static int g_cit_threats = 1;   // [all] citizen_threats (below, DAMAGE MEMORY)
static int raiders_near(void *pl, int x, int y, int r0) {
    int rmax = g_cit_threats ? RAID_FAR : RAID_MAX;
    int r = r0 > rmax ? r0 : rmax;   // the search box; each unit then by its own reach
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid || !pl) return 0;
    int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    for (int cx = (x - r) / cs; cx <= (x + r) / cs; cx++)
        for (int cy = (y - r) / cs; cy <= (y + r) / cs; cy++) {
            if (cx < 0 || cy < 0 || cx >= nx || cy >= ny) continue;
            uint8_t *cell = cells + (cx * ny + cy) * 16;
            for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **); b++) {
                uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                if (!td || FIELD(u, 0x3C, int) <= 0 || abs(FIELD(u, 0x1C, int) - x) > r || abs(FIELD(u, 0x20, int) - y) > r) continue;
                int fam = FIELD(td, 0x4B4, int), t = FIELD(td, 0x4B8, int);
                if ((fam != 5 && fam != 4 && !(g_cit_threats && fam == 10)) || t == 32 || t == 480 || FIELD(td, 0x510, int) <= 0) continue;
                uint8_t *st = FIELD(u, 0x34, uint8_t *);
                float rng = st ? FIELD(st, 0x9C, float) : FIELD(td, 0x420, float);
                int reach = rng > 0.0f && rng < 30.0f ? (int)(rng + 0.5f) + RAID_MARGIN : 0;
                if (reach > rmax) reach = rmax;
                if (reach < r0) reach = r0;
                if (abs(FIELD(u, 0x1C, int) - x) > reach || abs(FIELD(u, 0x20, int) - y) > reach) continue;
                void *o = FIELD(u, 0x18, void *);   // not nature (player 0: wild animals)
                if (o && FIELD(o, PL_INDEX, int) > 0 && is_enemy(pl, o) && seen_by(u, pl)) return 1;
            }
        }
    return 0;
}
// enemy soldiers on the way from x0,y0 to x1,y1 (every 4 tiles from the 5th, within 4) or within 6 of the end
static int path_raided(void *pl, int x0, int y0, int x1, int y1) {
    float dx = (float)(x1 - x0), dy = (float)(y1 - y0), l = sqrtf(dx * dx + dy * dy);
    for (float t = 5; t < l; t += 4)
        if (raiders_near(pl, x0 + (int)(dx / l * t), y0 + (int)(dy / l * t), 4)) return 1;
    return raiders_near(pl, x1, y1, 6);
}
// where a citizen at x,y flees while the base is overrun: the rally point if the way is clear, else 12 tiles in the
// clear direction (of 8) nearest the rally point; none clear: the rally point after all
static void flee_point(void *pl, int x, int y, float rx, float ry, float *fx, float *fy) {
    *fx = rx; *fy = ry;
    if (!path_raided(pl, x, y, (int)rx, (int)ry)) return;
    float msz = (float)*(uint32_t *)(*(uint8_t **)G_MAP + 0x1816FC), best = 1e9f;
    static const float dir[8][2] = { { 1, 0 }, { .7071f, .7071f }, { 0, 1 }, { -.7071f, .7071f }, { -1, 0 },
                                     { -.7071f, -.7071f }, { 0, -1 }, { .7071f, -.7071f } };
    for (int k = 0; k < 8; k++) {
        float px = (float)x + dir[k][0] * 12, py = (float)y + dir[k][1] * 12;
        if (msz > 8) {
            px = px < 3 ? 3 : px > msz - 4 ? msz - 4 : px;
            py = py < 3 ? 3 : py > msz - 4 ? msz - 4 : py;
        }
        float d = (px - rx) * (px - rx) + (py - ry) * (py - ry);
        if (d >= best || path_raided(pl, x, y, (int)px, (int)py)) continue;
        best = d; *fx = px; *fy = py;
    }
}
// DAMAGE MEMORY ([all] citizen_threats, default 1; the playtester: "they should just stay away from the frontline, a
// single artillery in range can kill multiple citizens in a single shot; gold and iron might be worth some risk, but
// wood can be gathered elsewhere, and farms rebuilt at a safer spot - in the WW1 save lots of citizens gather wood right
// under my artillery and my planes"). raiders_near saw land units and ships within 15 tiles only, and nothing it
// can't see: artillery shooting from the fog, aircraft. With citizen_threats: the reach up to 27 tiles (ships of
// family 10 too), and every 4 s (citizens_safety) each citizen's hit points are compared with the last look - where
// one lost some, the spot is remembered for 2 minutes. A site within 6 tiles of such spots is dangerous: food and wood
// from one (they can be had elsewhere), stone, gold and iron from 3 (worth some risk). Aircraft count only where they
// do harm, so a town they merely fly over keeps working. The searches refuse such sites too (no sent back, sent again).
// Hunters' losses don't count (the animals). scenarios/shelling.txt, bombing.txt (10 seeds, P3 off vs on): shelling
// alive 5 vs 7 of 10, workers 7.1 vs 17.6, losses 14.3 vs 10.8, buildings lost 6.1 vs 3.9; bombing workers 42.9 vs
// 54.7, losses 25.5 vs 21.0. Idle citizens rose (bombing 2.9 vs 6.3 on average): no safe site of their resource left.
#define HURT_N 64
static struct { unsigned at; struct { int x, y; uint32_t t; } h[HURT_N]; } g_hurt[16];
static struct { uint8_t *u; int hp; } g_cit_hp[16][512];
static unsigned g_hurt_count[16];
static void hurt_note(unsigned pi, uint8_t *u, uint32_t now, int mark) {   // remembers u's hit points; a loss marks its spot
    int hp = FIELD(u, 0x3C, int);
    unsigned h = (unsigned)((uintptr_t)u >> 4) & 511;
    for (int k = 0; k < 512; k++) {
        typeof(&g_cit_hp[0][0]) e = &g_cit_hp[pi][(h + k) & 511];
        if (e->u == u) {
            if (hp < e->hp && mark) {
                typeof(&g_hurt[0].h[0]) r = &g_hurt[pi].h[g_hurt[pi].at++ % HURT_N];
                r->x = FIELD(u, 0x1C, int); r->y = FIELD(u, 0x20, int); r->t = now ? now : 1;
                g_hurt_count[pi]++;
            }
            e->hp = hp;
            return;
        }
        if (!e->u) { e->u = u; e->hp = hp; return; }
    }
    memset(g_cit_hp[pi], 0, sizeof g_cit_hp[pi]);   // full (dead citizens' entries): start over
}
static int hurt_near(unsigned pi, int x, int y) {   // citizens hurt within 6 tiles in the last 2 minutes
    uint32_t now = game_time();
    int n = 0;
    for (int k = 0; k < HURT_N; k++) {
        typeof(&g_hurt[0].h[0]) r = &g_hurt[pi].h[k];
        if (r->t && now >= r->t && now - r->t <= 120000 && abs(r->x - x) <= 6 && abs(r->y - y) <= 6) n++;
    }
    return n;
}
static int hurt_danger(void *pl, int x, int y, int res) {   // res: 2 food, 3 wood, 4 stone, 5 gold, 6 iron, -1 unknown
    if (!g_cit_threats || !pl) return 0;
    return hurt_near((unsigned)FIELD(pl, PL_INDEX, int) & 15, x, y) >= (res >= 4 ? 3 : 1);
}
static int site_danger_res(void *pl, uint8_t *site, int food, int res) {   // from the nearest drop site
    if (!g_avoid_citizens || !pl || !site) return 0;
    int x = FIELD(site, 0x1C, int), y = FIELD(site, 0x20, int), d[2];
    if (raiders_near(pl, x, y, 6)) return 1;   // the searches refuse a raided site too (else: sent back, sent again)
    if (hurt_danger(pl, x, y, res)) return 1;
    if (!nearest_drop(pl, x, y, food, d)) d[0] = -10000;
    return route_danger(pl, d[0], d[1], x, y);
}
static int site_danger(void *pl, uint8_t *site, int food) { return site_danger_res(pl, site, food, food ? 2 : -1); }
static void *goal_move(float x, float y);
static int unit_goal(uint8_t *u, void *goal, int append, int prio);
static int muster_rally(unsigned idx, float *x, float *y);   // defence.c: the rally point while the base is overrun
static struct { unsigned sent; } g_cit_stats[16];
static unsigned g_repair_refused[16];   // repair orders refused at buildings under attack (hook_repair_goal)
// STRAY CITIZENS: an attack that ends hands its units back to the resource manager's list 0 (not assigned) with the
// attack's last order still on them - march to the target's Town Center. The assignment only gives work to idle
// citizens, so they marched on and died there: all 34 of Hedda's, first into the player's town, then into Livia's
// (her economy dead at minute 46, the stock frozen). A citizen of list 0 walking to a point within 6 tiles of an
// enemy building and over 15 tiles from its own drop sites is sent home (not to the muster's rally point).
static int building_at(void *pl, const int *dest, int r);
static int g_bunkers;   // bunker.c: citizens under fire into a fortress
static void bk_refresh(void *pl, unsigned pi);
static int bk_shelter(void *pl, uint8_t *u);
static void bk_flush(void *pl, unsigned pi, uint32_t now);
static void bk_build(void *pl, unsigned pi, uint32_t now);
static void citizens_safety(uint8_t *rm) {   // before the citizen assignment
    void *pl = rm_player(rm);
    if (!g_avoid_citizens || !pl) return;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    static uint32_t next[16], logged[16];
    uint32_t now = game_time();
    if (now < next[idx] && next[idx] - now <= 4000) return;
    next[idx] = now + 4000;
    threat_refresh(pl, now);
    int sent = 0, fled = 0, aside = 0, stray = 0, kinds[4][2] = { { 0 } };   // site types sent back from (log)
    int doing[7][4] = { { 0 } };   // per list: gathering, building, walking, nothing (the minute's log)
    float rx, ry;
    int mus = muster_rally(idx, &rx, &ry), sheltered = 0;
    if (g_bunkers) bk_refresh(pl, idx);
    for (int r = 0; r < 7; r++) {
        uint8_t *list = rm + RM_LISTS + r * 0x14;
        for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), cnt = FIELD(list, 0x10, unsigned); k < cnt; k++) {
            uint8_t *u = deque_at(list, first + k), *goal = u ? FIELD(u, 0x1EC, uint8_t *) : NULL;
            if (!u || FIELD(u, 0x3C, int) <= 0) continue;
            {
                const char *gd = goal ? rtti_name(goal) : "";
                doing[r][!goal ? 3 : !strcmp(gd, "EEUGGather@@") ? 0 : !strcmp(gd, "EEUGBuildRepair@@") ? 1 :
                         *(uintptr_t *)goal == VT_GOAL_LOCATION ? 2 : 3]++;
            }
            if (g_cit_threats) {   // not hunters: the animals hurt them (a farm is a building, family 2)
                uint8_t *gs = goal && !strcmp(rtti_name(goal), "EEUGGather@@") ? FIELD(goal, 0x38, uint8_t *) : NULL;
                hurt_note(idx, u, now, !(r == 2 && gs && FIELD(FIELD(gs, 0x2C, uint8_t *), 0x4B4, int) != 2));
            }
            if (r == 0 && goal && *(uintptr_t *)goal == VT_GOAL_LOCATION) {   // a stray: an attack's leftover order
                float gx = FIELD(goal, 0x20, float), gy = FIELD(goal, 0x24, float);
                int dest[2] = { (int)gx, (int)gy }, d[2];
                int rally = mus && fabsf(gx - rx) < 2 && fabsf(gy - ry) < 2;
                if (!rally && building_at(pl, dest, 6) &&
                    (!nearest_drop(pl, dest[0], dest[1], 0, d) || (d[0] - dest[0]) * (d[0] - dest[0]) + (d[1] - dest[1]) * (d[1] - dest[1]) > 225) &&
                    nearest_drop(pl, FIELD(u, 0x1C, int), FIELD(u, 0x20, int), 0, d) &&
                    unit_goal(u, goal_move((float)d[0] + 0.5f, (float)d[1] + 0.5f), 0, 0x1F40)) {
                    stray++;
                    continue;
                }
            }
            if (mus) {   // the base is overrun: citizens near the enemy (or working near it) flee to the rally point
                int ux = FIELD(u, 0x1C, int), uy = FIELD(u, 0x20, int);
                float dx = (float)ux - rx, dy = (float)uy - ry;
                if (dx * dx + dy * dy <= 36) continue;   // there already
                const char *gn0 = goal ? rtti_name(goal) : "";
                uint8_t *st = !goal ? NULL : !strcmp(gn0, "EEUGBuildRepair@@") ? FIELD(goal, 0x3C, uint8_t *) :
                              !strcmp(gn0, "EEUGGather@@") ? FIELD(goal, 0x38, uint8_t *) : NULL;
                int walk = goal && *(uintptr_t *)goal == VT_GOAL_LOCATION, danger;
                if (walk)   // on its way somewhere: only if enemy soldiers stand on the way or at the end
                    danger = path_raided(pl, ux, uy, (int)FIELD(goal, 0x20, float), (int)FIELD(goal, 0x24, float));
                else
                    danger = raiders_near(pl, ux, uy, 8) || (st && raiders_near(pl, FIELD(st, 0x1C, int), FIELD(st, 0x20, int), 8));
                if (danger && g_bunkers && bk_shelter(pl, u)) { sheltered++; continue; }
                if (danger) {
                    float fx, fy;
                    flee_point(pl, ux, uy, rx, ry, &fx, &fy);
                    if (walk && fabsf(FIELD(goal, 0x20, float) - fx) < 2 && fabsf(FIELD(goal, 0x24, float) - fy) < 2) continue;
                    if (unit_goal(u, goal_move(fx, fy), 0, 0x1F40)) {
                        fled++;
                        if (fx != rx || fy != ry) aside++;
                    }
                }
                continue;
            }
            if (!goal) continue;
            const char *gn = rtti_name(goal);
            int build = !strcmp(gn, "EEUGBuildRepair@@");
            if (!build && strcmp(gn, "EEUGGather@@")) continue;
            uint8_t *site = FIELD(goal, build ? 0x3C : 0x38, uint8_t *);
            int d[2];
            if (!site || !nearest_drop(pl, FIELD(u, 0x1C, int), FIELD(u, 0x20, int), r == 2, d)) continue;
            int raided = raiders_near(pl, FIELD(site, 0x1C, int), FIELD(site, 0x20, int), build ? 8 : 6);
            // builders near known towers only: only of fields (walls and towers at the front: the wall manager sends
            // them again and again)
            if (build && !raided && FIELD(FIELD(site, 0x2C, uint8_t *), 0x4B8, int) != 0x2A) continue;
            if (!build && !raided && hurt_danger(pl, FIELD(site, 0x1C, int), FIELD(site, 0x20, int), r)) raided = 1;   // hurt there
            if (!raided && (build || !site_danger_res(pl, site, r == 2, r))) continue;
            if (g_bunkers && bk_shelter(pl, u)) { sheltered++; continue; }   // under fire: into a fortress
            if (raided && abs(d[0] - FIELD(site, 0x1C, int)) <= 3 && abs(d[1] - FIELD(site, 0x20, int)) <= 3) continue;   // at home already
            if (!unit_goal(u, goal_move((float)d[0] + 0.5f, (float)d[1] + 0.5f), 0, 0x1F40)) continue;
            sent++;
            int st = FIELD(FIELD(site, 0x2C, uint8_t *), 0x4B8, int), j = 0;
            while (j < 4 && kinds[j][1] && kinds[j][0] != st) j++;
            if (j < 4) { kinds[j][0] = st; kinds[j][1]++; }
        }
    }
    g_cit_stats[idx].sent += sent + fled;
    if (g_bunkers) { bk_flush(pl, idx, now); bk_build(pl, idx, now); }
    (void)sheltered;
    static uint32_t doing_t[16];
    if (g_log && (now < doing_t[idx] || now - doing_t[idx] >= 60000)) {   // what the citizens do, per resource list
        doing_t[idx] = now;
        char b[256] = "";
        static const char *nm = "-?FWSGI";
        for (int r = 0; r < 7; r++) {
            size_t l = strlen(b);
            snprintf(b + l, sizeof b - l, " %c %d/%d/%d/%d", nm[r], doing[r][0], doing[r][1], doing[r][2], doing[r][3]);
        }
        int all[6] = { 0 };   // every citizen on the map: gathering, building, walking, attacking, other goal, none
        uint8_t *map = *(uint8_t **)G_MAP, *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
        int cells = grid ? FIELD(grid, 0x10, int) * FIELD(grid, 0x14, int) : 0;
        uint8_t *cl = grid ? FIELD(grid, 4, uint8_t *) : NULL;
        char other[64] = "";
        for (int ci = 0; ci < cells; ci++)
            for (uint8_t **bp = FIELD(cl + ci * 16, 4, uint8_t **); bp && bp < FIELD(cl + ci * 16, 8, uint8_t **); bp++) {
                uint8_t *u = *bp, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                if (!td || FIELD(u, 0x18, void *) != pl || FIELD(u, 0x3C, int) <= 0 || (FIELD(td, 0x4B8, int) != 32 && FIELD(td, 0x4B8, int) != 480)) continue;
                uint8_t *g = FIELD(u, 0x1EC, uint8_t *);
                const char *gd = g ? rtti_name(g) : "";
                int k = !g ? 5 : !strcmp(gd, "EEUGGather@@") ? 0 : !strcmp(gd, "EEUGBuildRepair@@") ? 1 :
                        *(uintptr_t *)g == VT_GOAL_LOCATION ? 2 : *(uintptr_t *)g == VT_GOAL_ATTACK ? 3 : 4;
                all[k]++;
                if (k == 4 && !other[0]) lstrcpynA(other, gd, sizeof other);
            }
        fprintf(g_log, "# doing: P%u at %.1f min (list: gather/build/walk/nothing):%s; all citizens: gather %d build %d walk %d attack %d other %d (%s) none %d\n",
                idx, now / 60000.0, b, all[0], all[1], all[2], all[3], all[4], other, all[5]);
    }
    if (stray && g_log) fprintf(g_log, "# citizens: P%u at %.1f min: %d unassigned citizens marching on an enemy town (an "
                                "attack's leftover order) sent home\n", idx, now / 60000.0, stray);
    if (fled && g_log) fprintf(g_log, "# citizens: P%u at %.1f min: %d fled (base overrun): %d to the rally point, %d aside "
                               "(enemies on the way)\n", idx, now / 60000.0, fled, fled - aside, aside);
    if (g_log && (sent || now / 300000 != logged[idx] / 300000) && (g_cit_stats[idx].sent || g_site_refused[idx] || g_repair_refused[idx])) {
        logged[idx] = now;
        char ks[64] = "";
        for (int j = 0; j < 4 && kinds[j][1]; j++) { size_t l = strlen(ks); snprintf(ks + l, sizeof ks - l, " %dx%d", kinds[j][0], kinds[j][1]); }
        fprintf(g_log, "# citizens: P%u at %.1f min: %d sent back from sites near enemy towers or raiders (site type x count:%s; %u in all, %u dangerous sites refused, %u repairs refused, %u hurt)\n",
                idx, now / 60000.0, sent, ks, g_cit_stats[idx].sent, g_site_refused[idx], g_repair_refused[idx], g_hurt_count[idx]);
    }
}

// REPAIRS UNDER ATTACK: the computer player's repair round (0x46D77D, every 31 s) sends a citizen to each damaged
// building - also to the wall or dock a player's army is hitting that moment: one citizen every 31 s walked into the
// army (P3: 17 in 15 minutes, sent back by citizens_safety only once it was on its way). Its order (EEUGBuildRepair,
// 0x60BAD0) is refused at the handing-over (0x5F05BD at 0x46DA88) while enemy soldiers stand within 8 tiles (or their range + 3) of the
// building (as citizens_safety): the goal deleted (its vf0, which lets go of the building), the citizen keeps its work.
#define CALL_REPAIR_GOAL 0x0046DA88
#define VT_GOAL_BUILD    0x008319F0
static void __fastcall hook_repair_goal(uint8_t *u, void *edx, uint8_t *goal, int append, int prio) {
    (void)edx;
    void *pl = u ? FIELD(u, 0x18, void *) : NULL;
    uint8_t *b = goal && *(uintptr_t *)goal == VT_GOAL_BUILD ? FIELD(goal, 0x3C, uint8_t *) : NULL;
    if (g_avoid_citizens && pl && b && raiders_near(pl, FIELD(b, 0x1C, int), FIELD(b, 0x20, int), 8)) {
        g_repair_refused[(unsigned)FIELD(pl, PL_INDEX, int) & 15]++;
        ((void (__thiscall *)(void *, int))(*(void ***)goal)[0])(goal, 1);
        return;
    }
    ((void (__thiscall *)(void *, void *, char, int))0x005F05BD)(u, goal, (char)append, prio);
}

// Known defences covering tile (x, y) by their real range (+1), not counting the one at (ex, ey).
static int covered(unsigned pi, float x, float y, int ex, int ey) {
    for (int k = 0; k < g_threat[pi].n; k++) {
        struct defence *d = &g_threat[pi].d[k];
        if (d->x == ex && d->y == ey) continue;
        float dx = x - (float)d->x, dy = y - (float)d->y, r = d->range + 1.5f;
        if (dx * dx + dy * dy <= r * r) return 1;
    }
    return 0;
}

// Siege training: the AI builds a Siege Factory now and then but hardly trains siege weapons. While it knows
// enemy defences (or a wall to breach) and has fewer than 2..4 siege weapons, it requests the best one of its
// epoch - through the train hook, so the budget applies (unit manager: player +0xB90 -> +0x44; request
// {type, count, building 0 = any, area 0}). Types newest first with their tech epoch; a type the civ can't
// train simply trains 0.
static const struct { int type, epoch; } k_siege_types[] = {
    { 603, 13 }, { 602, 10 }, { 107, 9 }, { 141, 7 }, { 105, 6 }, { 104, 4 } };
static struct { uint32_t last_try, factory_req; unsigned trained, failed, no_factory; } g_siege_train[16];

// Entry i of the build manager's queue for epoch e (bm +4 + e*0x14: deque of {type, x, y}, 0x155 per block), NULL
// past the end. 14 queues (epochs 0..13); the placement area follows at +0x11C.
static int *bq_entry(uint8_t *bm, int e, unsigned i) {
    uint8_t *dq = bm + 4 + e * 0x14;
    unsigned first = FIELD(dq, 0xC, unsigned), n = FIELD(dq, 0x10, unsigned), msz = FIELD(dq, 8, unsigned);
    uint8_t **map = FIELD(dq, 4, uint8_t **);
    if (e < 0 || e >= 14 || i >= n || !map || !msz || first >= msz * 0x155 || n > msz * 0x155) return NULL;
    unsigned idx = first + i, blk = idx / 0x155;
    if (blk >= msz) blk -= msz;
    return map[blk] ? (int *)(map[blk] + (idx % 0x155) * 12) : NULL;
}
static int bq_has(uint8_t *bm, int type) {
    for (int e = 0; e < 14; e++)
        for (unsigned i = 0; ; i++) {
            int *ent = bq_entry(bm, e, i);
            if (!ent) break;
            if (ent[0] == type) return 1;
        }
    return 0;
}

static int siege_want(void *pl, unsigned pi) {   // 2 + a quarter of the known enemy defences, at most 4; a breach: 2
    int want = g_threat[pi].n ? 2 + g_threat[pi].n / 4 : 0;
    for (int k = 0; k < 64 && !want; k++)
        if (g_breaches[k].sg && FIELD(g_breaches[k].sg, 0x190, void *) == pl) want = 2;
    return want > 4 ? 4 : want;
}
static void siege_train(void *pl, unsigned pi, int have, uint32_t now) {
    int want = siege_want(pl, pi);
    if (have >= want || (now >= g_siege_train[pi].last_try && now - g_siege_train[pi].last_try < 30000)) return;
    if (now < g_siege_st[pi].pause_until && g_siege_st[pi].pause_until - now <= 600000) return;   // lost too many
    g_siege_train[pi].last_try = now;
    // no siege weapon in this epoch yet (the catapult comes with the Bronze Age): no factory either - the build
    // manager can't place it before and would keep the request in its queue for good
    if (current_epoch(pl) + 1 < k_siege_types[sizeof k_siege_types / sizeof *k_siege_types - 1].epoch) return;
    // a Siege Factory (type 63) of our own
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    uint8_t *cells = grid ? FIELD(grid, 4, uint8_t *) : NULL;
    unsigned count = grid ? FIELD(grid, 0x10, unsigned) * FIELD(grid, 0x14, unsigned) : 0;
    int factory = 0;
    for (unsigned c = 0; cells && c < count && !factory; c++)
        for (uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **); b && b < FIELD(cells + c * 16, 8, uint8_t **); b++) {
            uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
            if (td && FIELD(u, 0x18, void *) == pl && FIELD(td, 0x4B8, int) == 63) { factory = 1; break; }
        }
    if (!factory) {
        // the build manager (cm +0x3C) works through per-epoch queues (+4 + epoch*0x14, 12-byte entries {type, x, y};
        // 0,0 = it picks the place) every 3 s (0x468540); 0x468C37 appends an entry. Queue a Siege Factory.
        g_siege_train[pi].no_factory++;
        void *cm = FIELD(pl, 0xB90, void *);
        uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
        int e = current_epoch(pl);
        if (bm && e >= 0 && e < 14 && !bq_has(bm, 63) &&
            (!g_siege_train[pi].factory_req || now - g_siege_train[pi].factory_req >= 300000)) {
            g_siege_train[pi].factory_req = now ? now : 1;
            int entry[3] = { 63, 0, 0 };
            ((void (__thiscall *)(void *, int *))0x00468C37)(bm + 4 + e * 0x14, entry);
            if (g_log) fprintf(g_log, "# siege: P%u at %.1f min queues a Siege Factory (wants %d siege, has %d)\n", pi, now / 60000.0, want, have);
            ai_say(pl, CHAT_SIEGE, "[ai] need siege (%d of %d): queued a Siege Factory", have, want);
        }
        return;
    }
    void *cm = FIELD(pl, 0xB90, void *), *um = cm ? FIELD(cm, 0x44, void *) : NULL;
    if (!um) return;
    int epoch = current_epoch(pl) + 1;   // tech tree epoch 0 = Prehistoric; tech ids count from 1
    for (size_t k = 0; k < sizeof k_siege_types / sizeof *k_siege_types; k++) {
        if (k_siege_types[k].epoch > epoch) continue;
        int req[4] = { k_siege_types[k].type, want - have, 0, 0 };
        unsigned got = 0;
        g_priority_mil = 1;
        hook_train_units(um, NULL, req, (uintptr_t)&got);
        g_priority_mil = 0;
        if (got) {
            g_siege_train[pi].trained += got;
            if (g_log) fprintf(g_log, "# siege: P%u at %.1f min trains %u x type %d (has %d, wants %d)\n", pi, now / 60000.0, got,
                               req[0], have, want);
            ai_say(pl, CHAT_SIEGE, "[ai] training %u siege (type %d), have %d of %d", got, req[0], have, want);
            return;
        }
    }
    g_siege_train[pi].failed++;
}

static void siege_tick(void *pl, uint32_t now) {
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!g_siege || (now >= g_siege_st[pi].last && now - g_siege_st[pi].last < 2000)) return;
    g_siege_st[pi].last = now;
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return;
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    unsigned count = FIELD(grid, 0x10, unsigned) * FIELD(grid, 0x14, unsigned);
    // own siege weapons
    enum { MAXS = 24, MAXE = 48 };
    uint8_t *siege[MAXS]; int ns = 0; float sx = 0, sy = 0, srange = 99;
    for (unsigned c = 0; cells && c < count && ns < MAXS; c++)
        for (uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **); b && b < FIELD(cells + c * 16, 8, uint8_t **) && ns < MAXS; b++) {
            uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
            if (!td || FIELD(u, 0x18, void *) != pl || !is_siege(FIELD(td, 0x4B8, int)) || FIELD(u, 0x3C, int) <= 0) continue;
            uint8_t *st = FIELD(u, 0x34, uint8_t *);
            float r = st ? FIELD(st, 0x9C, float) : FIELD(td, 0x420, float);
            if (r < srange) srange = r;
            siege[ns++] = u; sx += (float)FIELD(u, 0x1C, int); sy += (float)FIELD(u, 0x20, int);
        }
    // losses: siege weapons of the last look gone now (3 within 10 minutes: no new ones for 10 minutes - a Howitzer is
    // 170 wood and 170 iron, and P3 trained 45 in 40 minutes into the player's army)
    if (now < g_siege_st[pi].seen_t) {   // a new game
        g_siege_st[pi].nprev = 0; g_siege_st[pi].wave = 0; g_siege_st[pi].pause_until = 0;
        memset(g_siege_st[pi].loss_t, 0, sizeof g_siege_st[pi].loss_t);
    }
    for (int i = 0; i < g_siege_st[pi].nprev; i++) {
        int alive = 0;
        for (int j = 0; j < ns && !alive; j++) alive = siege[j] == g_siege_st[pi].prev[i];
        if (alive) continue;
        g_siege_st[pi].lost++;
        g_siege_st[pi].loss_t[0] = g_siege_st[pi].loss_t[1];
        g_siege_st[pi].loss_t[1] = g_siege_st[pi].loss_t[2];
        g_siege_st[pi].loss_t[2] = now ? now : 1;
        if (g_siege_st[pi].loss_t[0] && now - g_siege_st[pi].loss_t[0] < 600000 &&
            !(now < g_siege_st[pi].pause_until && g_siege_st[pi].pause_until - now <= 600000)) {
            g_siege_st[pi].pause_until = now + 600000;
            g_siege_st[pi].pauses++;
            if (g_log) fprintf(g_log, "# siege: P%u at %.1f min lost 3 siege within %.1f min - no new ones for 10 min\n", pi,
                               now / 60000.0, (now - g_siege_st[pi].loss_t[0]) / 60000.0);
        }
    }
    g_siege_st[pi].nprev = ns;
    for (int i = 0; i < ns; i++) g_siege_st[pi].prev[i] = siege[i];
    g_siege_st[pi].seen_t = now;
    siege_train(pl, pi, ns, now);
    if (ns < 1) return;
    sx /= (float)ns; sy /= (float)ns;
    // target: a breach wall of one of our subgroups, else the nearest known defence we outrange
    uint8_t *t = NULL; float best = 1e30f;
    for (int k = 0; k < 64; k++) {
        if (!g_breaches[k].sg || FIELD(g_breaches[k].sg, 0x190, void *) != pl) continue;
        uint8_t *w = wall_at(pl, g_breaches[k].x, g_breaches[k].y);
        float dx = (float)g_breaches[k].x - sx, dy = (float)g_breaches[k].y - sy;
        if (w && dx * dx + dy * dy < best) { best = dx * dx + dy * dy; t = w; }
    }
    if (!t) {
        threat_refresh(pl, now);
        for (int k = 0; k < g_threat[pi].n; k++) {
            struct defence *d = &g_threat[pi].d[k];
            if (d->range + 0.5f >= srange) continue;
            float dx = (float)d->x - sx, dy = (float)d->y - sy;
            if (dx * dx + dy * dy < best) { best = dx * dx + dy * dy; t = unit_at(d->x, d->y, d->type); }
        }
    }
    if (!t) return;
    int tx = FIELD(t, 0x1C, int), ty = FIELD(t, 0x20, int);
    // firing position: range - 0.7 from the target, facing the siege, outside every other known defence
    // (the direction rotated by fixed steps, no trigonometry: the C runtime's sin/cos may differ between Windows
    // versions, and in multiplayer every PC must decide the same)
    static const float k_cos15[7] = { 1.0f, 0.96592583f, 0.86602540f, 0.70710678f, 0.5f, 0.25881905f, 0.0f };
    static const float k_sin15[7] = { 0.0f, 0.25881905f, 0.5f, 0.70710678f, 0.86602540f, 0.96592583f, 1.0f };
    float ux = sx - (float)tx, uy = sy - (float)ty, ul = sqrtf(ux * ux + uy * uy), r = srange - 0.7f, px = 0, py = 0;
    if (ul > 0) { ux /= ul; uy /= ul; } else { ux = 1; uy = 0; }
    int found = 0;
    for (int k = 0; k <= 12 && !found; k++) {
        int st = (k + 1) / 2;   // 0, +-15, +-30 ... +-90 degrees
        float c = k_cos15[st], s = k & 1 ? k_sin15[st] : -k_sin15[st];
        px = (float)tx + 0.5f + r * (ux * c - uy * s); py = (float)ty + 0.5f + r * (ux * s + uy * c);
        found = !covered(pi, px, py, tx, ty);
    }
    if (!found) return;
    int changed = tx != g_siege_st[pi].tx || ty != g_siege_st[pi].ty;
    g_siege_st[pi].tx = tx; g_siege_st[pi].ty = ty; g_siege_st[pi].ttype = FIELD(FIELD(t, 0x2C, uint8_t *), 0x4B8, int);
    g_siege_st[pi].px = (int)px; g_siege_st[pi].py = (int)py;
    // WAVES: every siege weapon used to be sent to the firing position the moment it existed - one by one, without
    // escorts, through the player's army camped outside the town (user's game: "1 ordered, 0 escorts" again and
    // again, 45 Howitzers of P3 lost). Now they gather at the town (the drop site nearest the target, 3 tiles back)
    // and go together: at least 3 (or all it wants, if fewer), at least 2 idle soldiers along, and no enemy soldiers
    // at the firing position or on the way there. While a wave is out (siege attacking the target or on the way),
    // newcomers wait for the next one.
    int d[2];
    if (!nearest_drop(pl, tx, ty, 0, d)) return;
    float gx = (float)d[0] - (float)tx, gy = (float)d[1] - (float)ty, gl = sqrtf(gx * gx + gy * gy);
    if (gl > 0) { gx = (float)d[0] + 0.5f + gx / gl * 3.0f; gy = (float)d[1] + 0.5f + gy / gl * 3.0f; }
    else { gx = (float)d[0] + 0.5f; gy = (float)d[1] + 0.5f; }
    uint8_t *out[MAXS], *home[MAXS], *away[MAXS]; int nout = 0, nhome = 0, naway = 0;
    for (int i = 0; i < ns; i++) {
        uint8_t *u = siege[i], *g = FIELD(u, 0x1EC, uint8_t *);
        float ux = (float)FIELD(u, 0x1C, int) + 0.5f - px, uy = (float)FIELD(u, 0x20, int) + 0.5f - py;
        float hx = (float)FIELD(u, 0x1C, int) + 0.5f - gx, hy = (float)FIELD(u, 0x20, int) + 0.5f - gy;
        int committed = (g && *(uintptr_t *)g == VT_GOAL_ATTACK && FIELD(g, 0x30, uint8_t *) == t) || ux * ux + uy * uy <= 9.0f ||
                        (g && *(uintptr_t *)g == VT_GOAL_LOCATION && fabsf(FIELD(g, 0x20, float) - px) < 2 && fabsf(FIELD(g, 0x24, float) - py) < 2);
        if (committed && !changed) out[nout++] = u;
        else if (hx * hx + hy * hy <= 25.0f) home[nhome++] = u;
        else away[naway++] = u;
    }
    int ordered = 0, launched = 0, need = siege_want(pl, pi);
    if (need > 3) need = 3;
    if (need < 2) need = 2;
    // the wave out: keep it at it (attack once there)
    for (int i = 0; i < nout; i++) {
        uint8_t *u = out[i], *g = FIELD(u, 0x1EC, uint8_t *);
        if (g && *(uintptr_t *)g == VT_GOAL_ATTACK && FIELD(g, 0x30, uint8_t *) == t) continue;
        float ux = (float)FIELD(u, 0x1C, int) + 0.5f - px, uy = (float)FIELD(u, 0x20, int) + 0.5f - py;
        if (ux * ux + uy * uy <= 4.0f) ordered += unit_goal(u, goal_attack(u, t), 0, 0x1F40);
    }
    g_siege_st[pi].wave = nout > 0;
    // the rest to the gathering point (not those walking there)
    for (int i = 0; i < naway; i++) {
        uint8_t *u = away[i], *g = FIELD(u, 0x1EC, uint8_t *);
        if (g && *(uintptr_t *)g == VT_GOAL_LOCATION && fabsf(FIELD(g, 0x20, float) - gx) < 2 && fabsf(FIELD(g, 0x24, float) - gy) < 2) continue;
        unit_goal(u, goal_move(gx, gy), 0, 0x1F40);
    }
    const char *hold = NULL;
    if (g_siege_st[pi].wave) hold = "a wave is out";
    else if (nhome < need) hold = "gathering";
    else if (raiders_near(pl, (int)px, (int)py, 8) || path_raided(pl, (int)gx, (int)gy, (int)px, (int)py)) hold = "enemy soldiers on the way";
    // escorts: idle soldiers (no goal) along with a wave
    int want = nhome * 3 / 2 < 4 ? 4 : nhome * 3 / 2, escorts = 0, idle = 0;
    for (unsigned c = 0; cells && c < count && idle < 2; c++)
        for (uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **); b && b < FIELD(cells + c * 16, 8, uint8_t **) && idle < 2; b++) {
            uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
            if (!td || FIELD(u, 0x18, void *) != pl || FIELD(u, 0x1EC, void *) || FIELD(u, 0x3C, int) <= 0) continue;
            int fam = FIELD(td, 0x4B4, int), type = FIELD(td, 0x4B8, int);
            idle += fam == 5 && !is_siege(type) && type != 32 && type != 480 && type != 102 && type != 287;
        }
    if (!hold && idle < 2) hold = "no escorts";
    if (hold) {
        if (strcmp(hold, "a wave is out")) g_siege_st[pi].held++;
        if (g_log && (changed || now / 60000 != g_siege_st[pi].logged_min) && strcmp(hold, "a wave is out") && g_siege_st[pi].logged < 80) {
            g_siege_st[pi].logged++;
            g_siege_st[pi].logged_min = now / 60000;
            fprintf(g_log, "# siege: P%u at %.1f min holds %d siege at %.0f,%.0f (%d there, %d wanted; %s) - target type %d at %d,%d\n",
                    pi, now / 60000.0, ns, gx, gy, nhome, need, hold, g_siege_st[pi].ttype, tx, ty);
        }
        return;
    }
    for (int i = 0; i < nhome; i++)
        launched += unit_goal(home[i], goal_move(px, py), 0, 0x1F40) && unit_goal(home[i], goal_attack(home[i], t), 1, 0x1F40);
    ordered += launched;
    g_siege_st[pi].waves++;
    // escorts: idle soldiers (no goal) to the firing position
    for (unsigned c = 0; cells && c < count && escorts < want && escorts < MAXE; c++)
        for (uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **); b && b < FIELD(cells + c * 16, 8, uint8_t **) && escorts < want; b++) {
            uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
            if (!td || FIELD(u, 0x18, void *) != pl || FIELD(u, 0x1EC, void *) || FIELD(u, 0x3C, int) <= 0) continue;
            int fam = FIELD(td, 0x4B4, int), type = FIELD(td, 0x4B8, int);
            if (fam != 5 || is_siege(type) || type == 32 || type == 480 || type == 102 || type == 287) continue;   // soldiers only (no citizens, rams)
            float ex = (float)FIELD(u, 0x1C, int) - px, ey = (float)FIELD(u, 0x20, int) - py;
            if (ex * ex + ey * ey < 9.0f) { escorts++; continue; }   // already there
            escorts += unit_goal(u, goal_move(px + (float)(escorts % 3) - 1.0f, py + (float)(escorts / 3 % 3) - 1.0f), 0, 0x1F40);
        }
    if (g_log && g_siege_st[pi].logged < 80) {
        g_siege_st[pi].logged++;
        fprintf(g_log, "# siege: P%u at %.1f min: a wave of %d siege (range %.1f) from %.0f,%.0f -> type %d at %d,%d from %.0f,%.0f; %d escorts\n",
                pi, now / 60000.0, launched, srange, gx, gy, g_siege_st[pi].ttype, tx, ty, px, py, escorts);
        fflush(g_log);
    }
}

static void charge_tick(uint32_t now);
// asgtrace P [off] (test): log every state change (+0x1D0) of AI P's attack subgroups - class, state, mission, goal,
// members alive and their centre, waypoints, engaged members, what the members do (march/fight/other/idle)
static unsigned g_trace_mask;
static struct { uint8_t *sg; int state, goal_x, goal_y; } g_trace[64];
static void asg_trace_tick(void *pl, uint32_t now) {
    unsigned p = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!(g_trace_mask & (1u << p)) || !g_log) return;
    uint8_t *cbm = FIELD(pl, 0xB8C, uint8_t *), *cmd = cbm ? FIELD(cbm, 0x48, uint8_t *) : NULL;
    if (!cmd) return;
    uint8_t *dq = cmd + 4;
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    for (unsigned k = 0; k < cnt && k < 32; k++) {
        uint8_t *g = deque_at(dq, first + k);
        for (uint8_t **sp = g ? FIELD(g, 8, uint8_t **) : NULL; sp && sp < FIELD(g, 0xC, uint8_t **); sp++) {
            uint8_t *sg = *sp;
            if (!sg) continue;
            int st = FIELD(sg, 0x1D0, int), gx = FIELD(sg, 0x104, int), gy = FIELD(sg, 0x108, int), slot = -1, fre = -1;
            for (int q = 0; q < 64; q++) {
                if (g_trace[q].sg == sg) { slot = q; break; }
                if (!g_trace[q].sg && fre < 0) fre = q;
            }
            if (slot >= 0 && g_trace[slot].state == st && g_trace[slot].goal_x == gx && g_trace[slot].goal_y == gy) continue;
            if (slot < 0) { if (fre < 0) continue; slot = fre; g_trace[slot].sg = sg; g_trace[slot].state = -1; }
            int n = 0, walk = 0, fight = 0, other = 0, none = 0, engaged = 0;
            float cx = 0, cy = 0;
            for (uint8_t **r = FIELD(sg, 0x54, uint8_t **); r && r < FIELD(sg, 0x58, uint8_t **) && n < 1000; r++) {
                uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL, *gl;
                if (!u || FIELD(u, 0x3C, int) <= 0) continue;
                if (FIELD(*r, 0x48, void *)) engaged++;
                n++; cx += (float)FIELD(u, 0x1C, int); cy += (float)FIELD(u, 0x20, int);
                gl = FIELD(u, 0x1EC, uint8_t *);
                if (!gl) none++;
                else if (*(uintptr_t *)gl == VT_GOAL_LOCATION) walk++;
                else if (*(uintptr_t *)gl == VT_GOAL_ATTACK) fight++;
                else other++;
            }
            fprintf(g_log, "# asgtrace: P%u at %.2f min: group %u %s %p: state %d -> %d mission %d goal %d,%d; %d alive at %.0f,%.0f, "
                    "%u waypoints, %d engaged; %d march %d fight %d other %d idle", p, now / 60000.0, k, rtti_name(sg),
                    (void *)sg, g_trace[slot].state, st, FIELD(sg, 0x124, int), gx, gy, n, n ? cx / n : 0.0f, n ? cy / n : 0.0f,
                    FIELD(sg, 0x70, unsigned), engaged, walk, fight, other, none);
            for (int w = 0; w < 2; w++) {   // the subgroup's target unit (+0x118) and the group's (+0x28 + 0x1C)
                uint8_t *t = w ? FIELD(g, 0x44, uint8_t *) : FIELD(sg, 0x118, uint8_t *);
                uint8_t *ttd = t ? FIELD(t, 0x2C, uint8_t *) : NULL;
                if (ttd) fprintf(g_log, "; %s target type %d at %d,%d hp %d", w ? "group" : "its", FIELD(ttd, 0x4B8, int),
                                 FIELD(t, 0x1C, int), FIELD(t, 0x20, int), FIELD(t, 0x3C, int));
            }
            fprintf(g_log, "; group %p +28:", (void *)g);
            for (int o = 0x28; o < 0x6C; o += 4) fprintf(g_log, " %x", FIELD(g, o, unsigned));
            fputc('\n', g_log);
            g_trace[slot].state = st; g_trace[slot].goal_x = gx; g_trace[slot].goal_y = gy;
        }
    }
}
static void cmd_asgtrace(char *out, const char *args) {
    int p = atoi(args);
    if (p <= 0 || p >= 16) { REPLY("error: asgtrace P [off]\n"); return; }
    if (strstr(args, "off")) g_trace_mask &= ~(1u << p); else g_trace_mask |= 1u << p;
    REPLY("attack subgroup trace: mask %x\n", g_trace_mask);
}

static void attack_tick(void *pl, uint32_t now) {
    asg_trace_tick(pl, now);
    charge_tick(now);
    threat_refresh(pl, now);   // also with avoid_towers=0: the route log compares both
    citizen_attack_scan(pl, now);
    siege_tick(pl, now);
    {
        unsigned i = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
        if (g_log && now / 300000 != g_mil[i].last_log / 300000) {
            fprintf(g_log, "# army: P%u at %u min: %d soldiers; military requests %u, trained %u (substituted %u, saved for %u), "
                    "failed short of food %u wood %u stone %u gold %u iron %u; budget left %d/%d/%d/%d/%d\n", i, now / 60000,
                    army_size(pl), g_mil[i].req, g_mil[i].trained, g_mil[i].subs, g_mil[i].saving, g_mil[i].short_of[2], g_mil[i].short_of[3],
                    g_mil[i].short_of[4], g_mil[i].short_of[5], g_mil[i].short_of[6], (int)g_bud[i].mil[2],
                    (int)g_bud[i].mil[3], (int)g_bud[i].mil[4], (int)g_bud[i].mil[5], (int)g_bud[i].mil[6]);
            fprintf(g_log, "# army: P%u failed types:", i);
            for (int k = 0; k < 4; k++)
                if (g_mil[i].fcount[k])
                    fprintf(g_log, " type %d from %06x x%u", g_mil[i].ftype[k] & 0x7FF,
                            (unsigned)g_mil[i].ftype[k] >> 11, g_mil[i].fcount[k]);
            fputc('\n', g_log);
            fprintf(g_log, "# leash: P%u: enemies in tower range skipped as targets %u times; rams held back %u, aircraft out of reach ignored %u (all), siege trained %u, "
                    "failed %u, no factory %u; siege waves %u, lost %u, training paused %u times, held %u times\n", i,
                    g_leash_stats[i].skipped, g_ram_waits, g_air_skips, g_siege_train[i].trained, g_siege_train[i].failed,
                    g_siege_train[i].no_factory, g_siege_st[i].waves, g_siege_st[i].lost, g_siege_st[i].pauses, g_siege_st[i].held);
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

// Engine guard: an attack subgroup's member records (+0x50 vector, record +0x44 = unit) are summed up by 0x4504FD ->
// 0x450554 (thiscall(stats, unit)) without a check. A record without its unit crashed there (read [0 + 0x34]) in a
// long game, one AI step after two AIs surrendered and attacks on them were called off (0x43033E with 1: the
// subgroups hand their units back, 0x455C90). Such a record is skipped and logged.
#define CALL_GROUP_STAT 0x00450546
#define FN_GROUP_STAT   0x00450554
static void __fastcall hook_group_stat(void *stats, void *edx, uint8_t *unit) {
    (void)edx;
    if (!unit) {
        static int logged;
        if (g_log && logged < 20) {
            logged++;
            fprintf(g_log, "# engine guard: attack subgroup member without its unit skipped at %.1f min\n", game_time() / 60000.0);
            fflush(g_log);
        }
        return;
    }
    ((void (__thiscall *)(void *, void *))FN_GROUP_STAT)(stats, unit);
}

// The same records read without a check by each subgroup class's vf20 (members' stats into +0x160: 0x45AD2F, land
// 0x4472A9) - a crash at 0x45AD70 (read [0 + 0x2C]) in the user's game at 49.6 min, right after the guard above had
// skipped such a record (12 minutes after attacks were called off for an alliance), and five more readers of the
// pattern. Before vf20 runs, records without a unit leave the member vector (+0x54..+0x58; the records themselves are
// left alone, in case anything else still points at them).
#define FN_ASG_STATS      0x0045AD2F
#define FN_ASG_LAND_STATS 0x004472A9
static void asg_purge(uint8_t *sg) {
    uint8_t **b = FIELD(sg, 0x54, uint8_t **), **e = FIELD(sg, 0x58, uint8_t **), **w = b;
    if (!b || e < b || e - b > 4096) return;
    for (uint8_t **r = b; r < e; r++)
        if (*r && FIELD(*r, 0x44, uint8_t *)) *w++ = *r;
    if (w == e) return;
    FIELD(sg, 0x58, uint8_t **) = w;
    static int logged;
    if (g_log && logged < 20) {
        logged++;
        fprintf(g_log, "# engine guard: %d attack subgroup members without their unit taken out at %.1f min\n", (int)(e - w),
                game_time() / 60000.0);
        fflush(g_log);
    }
}
// every attack subgroup of AI pl: before the mod calls its attacks off (0x43033E with 1, which reads the members first,
// 0x454B90) - with a record without its unit it crashed there at once (test: nullmember P off)
static void asg_purge_all(void *pl) {
    uint8_t *cbm = pl ? FIELD(pl, 0xB8C, uint8_t *) : NULL, *cmd = cbm ? FIELD(cbm, 0x48, uint8_t *) : NULL;
    if (!cmd) return;
    uint8_t *dq = cmd + 4;
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        uint8_t *g = deque_at(dq, first + k);
        for (uint8_t **sp = g ? FIELD(g, 8, uint8_t **) : NULL; sp && sp < FIELD(g, 0xC, uint8_t **); sp++)
            if (*sp) asg_purge(*sp);
    }
}
static void __fastcall hook_asg_stats(uint8_t *sg, void *edx) {
    (void)edx;
    asg_purge(sg);
    ((void (__thiscall *)(void *))FN_ASG_STATS)(sg);
}
static void __fastcall hook_asg_land_stats(uint8_t *sg, void *edx) {
    (void)edx;
    asg_purge(sg);
    ((void (__thiscall *)(void *))FN_ASG_LAND_STATS)(sg);
}
static const uintptr_t k_asg_vtables[] = { 0x00827620, 0x00826610, 0x00826818, 0x00826F10, 0x008270F8, 0x00827318 };

// The same family, now reproducible: the reference save, P2's attack called off at 86.6 min (attack_retreat, or the
// test command calloff 2 1 - with and without honest_vision or landings) crashed every time. The call-off
// (0x43033E -> 0x46FD01) hands each unit of the attack back, and each one is offered to the attack groups at once
// (0x483ADE -> 0x4824BF -> 0x450E07 -> 0x453E10) - including groups the same call-off has just freed (their subgroup
// vectors garbage, a jump to 0). While the mod calls attacks off (g_in_calloff, diplo.c) no unit is offered: "not
// taken" (the caller only notes it), the unit stays free and the game assigns it on its next pass.
#define CALL_UNIT_TO_GROUP 0x004824BF
#define FN_UNIT_TO_GROUP   0x00450E07
static int g_in_calloff;
static unsigned g_calloff_skips;
static char __fastcall hook_unit_to_group(void *cmd, void *edx, uint8_t *unit) {
    (void)edx;
    if (g_in_calloff) { g_calloff_skips++; return 0; }
    return ((char (__thiscall *)(void *, void *))FN_UNIT_TO_GROUP)(cmd, unit);
}

static void attack_install(const char *aimod_ini, const char *autotest_ini) {
    BOOL stat_guard = patch_call(CALL_GROUP_STAT, FN_GROUP_STAT, hook_group_stat);
    int sg_guard = patch_vtable(VT_ASG_LAND, 20, FN_ASG_LAND_STATS, hook_asg_land_stats);
    for (unsigned k = 0; k < sizeof k_asg_vtables / sizeof *k_asg_vtables; k++)
        sg_guard += patch_vtable(k_asg_vtables[k], 20, FN_ASG_STATS, hook_asg_stats);
    if (g_log) fprintf(g_log, "# engine guard: attack subgroup stats %d/7\n", sg_guard);
    if (g_log) fprintf(g_log, "# engine guard: attack subgroup members %s\n", stat_guard ? "ok" : "FAILED");
    BOOL add_guard = patch_call(CALL_UNIT_TO_GROUP, FN_UNIT_TO_GROUP, hook_unit_to_group);
    if (g_log) fprintf(g_log, "# engine guard: no unit offered to attack groups during a call-off %s\n", add_guard ? "ok" : "FAILED");
    g_avoid_towers = cfg_int(aimod_ini, autotest_ini, 1, "avoid_towers", 1);
    g_avoid_citizens = cfg_int(aimod_ini, autotest_ini, 1, "avoid_towers_citizens", 1);
    g_cit_threats = cfg_int(aimod_ini, autotest_ini, 1, "citizen_threats", 1);
    g_bunkers = cfg_int(aimod_ini, autotest_ini, 1, "bunkers", 1);
    BOOL repair = patch_call(CALL_REPAIR_GOAL, 0x005F05BD, hook_repair_goal);
    if (g_log) fprintf(g_log, "# repairs under attack: refused (hook %s)\n", repair ? "ok" : "FAILED");
    g_avoid_weight = cfg_int(aimod_ini, autotest_ini, 1, "avoid_towers_weight", 2000);
    g_avoid_margin = cfg_int(aimod_ini, autotest_ini, 1, "avoid_towers_margin", 6);
    g_weak_spots = cfg_int(aimod_ini, autotest_ini, 1, "attack_weak_spots", 1);
    g_weak_radius = cfg_int(aimod_ini, autotest_ini, 1, "attack_weak_spot_radius", 60);
    g_leash = cfg_int(aimod_ini, autotest_ini, 1, "attack_leash", 1);
    g_citizen_attacks = cfg_int(aimod_ini, autotest_ini, 1, "citizen_attacks", 0);
    g_breach = cfg_int(aimod_ini, autotest_ini, 1, "attack_breach", 1);
    g_siege = cfg_int(aimod_ini, autotest_ini, 1, "attack_siege", 1);
    g_ignore_air = cfg_int(aimod_ini, autotest_ini, 1, "attack_ignore_air", 0);
    int a = patch_vtable(VT_ROUTE_PLANNER, 1, FN_ROUTE_COST, hook_route_cost);
    int b = patch_vtable(VT_ASG_LAND, ASG_ROUTE_SLOT, FN_ASG_ROUTE, hook_asg_route);
    int c = patch_vtable(VT_ASG_LAND, ASG_ROUTE_DONE_SLOT, FN_ASG_ROUTE_DONE, hook_asg_route_done);
    int d = 1;
    for (unsigned k = 0; k < sizeof g_asgu_vtables / sizeof *g_asgu_vtables; k++)
        d &= patch_vtable(g_asgu_vtables[k], 2, FN_ASGU_SCORE, hook_asgu_score);
    if (g_log) fprintf(g_log, "# attack routing: avoid_towers=%d weight=%d margin=%d weak_spots=%d radius=%d leash=%d breach=%d hooks %d/4\n",
                       g_avoid_towers, g_avoid_weight, g_avoid_margin, g_weak_spots, g_weak_radius, g_leash, g_breach, a + b + c + d);
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

// attacks P (test): AI P's attack groups - target, and per subgroup its class, members alive and their centre, and how
// many of them march (EEUGLocation), fight (EEUGAttackUnit), sit in or wait for a transport, or do nothing
static void cmd_attacks(char *out, const char *args) {
    int p = atoi(args);
    void *pl = (p > 0 && p < 16) ? g_players[p] : NULL;
    uint8_t *cbm = pl ? FIELD(pl, 0xB8C, uint8_t *) : NULL, *cmd = cbm ? FIELD(cbm, 0x48, uint8_t *) : NULL;
    if (!cmd) { REPLY("error: no AI player %d with an attack commander\n", p); return; }
    uint8_t *dq = cmd + 4;
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    REPLY("P%d: %u attack groups, next attack at %.1f min\n", p, cnt, FIELD(pl, 0xED8, uint32_t) / 60000.0);
    for (unsigned k = 0; k < cnt && k < 16; k++) {
        uint8_t *g = deque_at(dq, first + k), *target = g ? FIELD(g, 0x48, uint8_t *) : NULL;
        if (!g) continue;
        int ns = 0;
        for (uint8_t **sp = FIELD(g, 8, uint8_t **); sp && sp < FIELD(g, 0xC, uint8_t **); sp++) ns++;
        REPLY(" group %u: target P%d, %d subgroups\n", k, target ? FIELD(target, PL_INDEX, int) : -1, ns);
        for (uint8_t **sp = FIELD(g, 8, uint8_t **); sp && sp < FIELD(g, 0xC, uint8_t **); sp++) {
            if (!*sp) continue;
            int n = 0, walk = 0, fight = 0, other = 0, none = 0, engaged = 0;
            float cx = 0, cy = 0;
            uint8_t **b = FIELD(*sp, 0x54, uint8_t **), **e = FIELD(*sp, 0x58, uint8_t **);
            for (uint8_t **r = b; b && r < e && n < 1000; r++) {
                uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL, *gl;
                if (!u || FIELD(u, 0x3C, int) <= 0) continue;
                if (FIELD(*r, 0x48, void *)) engaged++;
                n++; cx += (float)FIELD(u, 0x1C, int); cy += (float)FIELD(u, 0x20, int);
                gl = FIELD(u, 0x1EC, uint8_t *);
                if (!gl) none++;
                else if (*(uintptr_t *)gl == VT_GOAL_LOCATION) walk++;
                else if (*(uintptr_t *)gl == VT_GOAL_ATTACK) fight++;
                else other++;
            }
            const char *cls = rtti_name(*sp);
            REPLY("   %-28s %3d alive at %.0f,%.0f: %d march, %d fight, %d other, %d idle\n", cls, n,
                  n ? cx / n : 0.0f, n ? cy / n : 0.0f, walk, fight, other, none);
            uint8_t *sg = *sp;
            REPLY("     state %d mission %d, goal %d,%d, %u waypoints, at %d,%d, %d engaged\n",
                  FIELD(sg, 0x1D0, int), FIELD(sg, 0x124, int), FIELD(sg, 0x104, int), FIELD(sg, 0x108, int),
                  FIELD(sg, 0x70, unsigned), FIELD(sg, 0x234, int), FIELD(sg, 0x238, int), engaged);
            if (n && strstr(args, "raw")) {   // the subgroup's fields as dwords, to compare over time
                for (uint8_t **r = b; b && r < e; r++) {   // each member: type, tile, goal (and its target)
                    uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL;
                    if (!u || FIELD(u, 0x3C, int) <= 0) continue;
                    uint8_t *gl = FIELD(u, 0x1EC, uint8_t *), *t = NULL;
                    if (gl && *(uintptr_t *)gl == VT_GOAL_ATTACK) t = FIELD(gl, 0x30, uint8_t *);
                    REPLY("       %d at %d,%d hp %d: %s", FIELD(FIELD(u, 0x2C, uint8_t *), 0x4B8, int),
                          FIELD(u, 0x1C, int), FIELD(u, 0x20, int), FIELD(u, 0x3C, int), gl ? rtti_name(gl) : "-");
                    if (t) REPLY(" P%d's %d (family %d) at %d,%d", FIELD(FIELD(t, 0x18, uint8_t *), PL_INDEX, int),
                                 FIELD(FIELD(t, 0x2C, uint8_t *), 0x4B8, int), FIELD(FIELD(t, 0x2C, uint8_t *), 0x4B4, int),
                                 FIELD(t, 0x1C, int), FIELD(t, 0x20, int));
                    REPLY(" rec+48 %08x\n", FIELD(*r, 0x48, unsigned));
                }
                for (int o = 0x100; o < 0x260; o += 0x20) {
                    REPLY("     %03x:", o);
                    for (int q = 0; q < 0x20; q += 4) REPLY(" %08x", FIELD(sg, o + q, unsigned));
                    REPLY("\n");
                }
            }
        }
    }
}

// attacknow P (test): AI P's next attack is due now (player +0xED8 = the game time) - the game launches it at its next
// look, if it has a target
static void cmd_attacknow(char *out, const char *args) {
    int p = atoi(args);
    void *pl = (p > 0 && p < 16) ? g_players[p] : NULL;
    if (!pl) { REPLY("error: no AI player %d\n", p); return; }
    uint32_t was = FIELD(pl, 0xED8, uint32_t);
    FIELD(pl, 0xED8, uint32_t) = game_time();
    REPLY("P%d: next attack %.1f -> %.1f min\n", p, was / 60000.0, game_time() / 60000.0);
}

// nullmember P (test): the first member record (of an attack subgroup of AI P with 2+ members) loses its unit (+0x44
// = 0) - the state the engine guards (hook_group_stat, asg_purge) are for
static void cmd_nullmember(char *out, const char *args) {
    int p = atoi(args);
    void *pl = (p > 0 && p < 16) ? g_players[p] : NULL;
    uint8_t *cbm = pl ? FIELD(pl, 0xB8C, uint8_t *) : NULL, *cmd = cbm ? FIELD(cbm, 0x48, uint8_t *) : NULL;
    if (!cmd) { REPLY("error: no AI player %d with an attack commander\n", p); return; }
    uint8_t *dq = cmd + 4;
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        uint8_t *g = deque_at(dq, first + k);
        for (uint8_t **sp = g ? FIELD(g, 8, uint8_t **) : NULL; sp && sp < FIELD(g, 0xC, uint8_t **); sp++) {
            int n = 0, cit = 0;
            if (!*sp) continue;
            subgroup_count(*sp, &n, &cit);
            uint8_t **b = FIELD(*sp, 0x54, uint8_t **);
            if (n < 2 || !b || !*b) continue;
            uint8_t *u = FIELD(*b, 0x44, uint8_t *);
            FIELD(*b, 0x44, uint8_t *) = NULL;
            REPLY("P%d subgroup %p (vt %06x, %d members): record %p lost its unit %p\n", p, (void *)*sp,
                  *(unsigned *)*sp, n, (void *)*b, (void *)u);
            uint8_t *target = FIELD(g, 0x48, uint8_t *);
            if (strstr(args, "off") && target) {   // and the attack called off at once, as diplo_safe_point does
                breach_forget(pl);
                asg_purge_all(pl);
                ((void (__thiscall *)(void *, void *, int))0x0043033E)(pl, target, 1);
                REPLY("P%d's attack on P%d called off\n", p, FIELD(target, PL_INDEX, int));
            }
            return;
        }
    }
    REPLY("P%d: no attack subgroup with 2+ members (groups %u)\n", p, cnt);
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

// spawn <player> <type> <x> <y> [count]: create units in a running game the way the game's own debug spawner does
// (0x64C561): an EEMHackANewUnit message (operator new 0x683718, 0x20 bytes; ctor 0x650B3C thiscall(int *type,
// float xy[2], char player)) handed to the lockstep server ([0x91984C] slot 6: (msg, 0xFF)). The simulation
// executes it in a coming tick (EEMHackANewUnit::vf10 0x4B7044: ground height, buildings checked against the
// map) - like any player command. Several units go on a grid of 2 tiles around x, y.
#define G_TYPES   0x009418A0
#define G_SERVER_OBJ 0x0091984C
// give P amount (test): player P's stock of food, wood, stone, gold and iron each grows by amount
static void cmd_give(char *out, const char *args) {
    int p = 0, amount = 0;
    sscanf(args, "%d %d", &p, &amount);
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    if (!pl) { REPLY("error: give P amount\n"); return; }
    int *stock = (int *)((uint8_t *)pl + PL_STOCK);
    for (int k = 2; k < 7; k++) stock[k] += amount;
    REPLY("P%d: stock %d/%d/%d/%d/%d\n", p, stock[2], stock[3], stock[4], stock[5], stock[6]);
}

// train P type [count] (test): AI P orders count (default 1) units of a type through its own unit manager, as a
// priority order (the stock above the epoch reserve) - unlike spawn, the units come from its factories and join its pool
static void cmd_train(char *out, const char *args) {
    int p = 0, type = 0, count = 1;
    sscanf(args, "%d %d %d", &p, &type, &count);
    void *pl = (p > 0 && p < 16) ? g_players[p] : NULL;
    void *cm = pl ? FIELD(pl, 0xB90, void *) : NULL, *um = cm ? FIELD(cm, 0x44, void *) : NULL;
    if (!um || type <= 0) { REPLY("error: train P type [count] (AI player)\n"); return; }
    int req[4] = { type, count, 0, 0 };
    unsigned got = 0;
    g_priority_mil = 1;
    hook_train_units(um, NULL, req, (uintptr_t)&got);
    g_priority_mil = 0;
    REPLY("P%d: %u of %d x type %d ordered\n", p, got, count, type);
}

static void cmd_spawn(char *out, const char *args) {
    int p, type, x, y, n = 1;
    if (sscanf(args, "%d %d %d %d %d", &p, &type, &x, &y, &n) < 4 || p < 0 || p > 15 || type < 0 || type >= 0x300 || n < 1 || n > 100) {
        REPLY("error: usage: spawn <player> <type> <x> <y> [count]\n");
        return;
    }
    void *srv = *(void **)G_SERVER_OBJ;
    if (!((void **)G_TYPES)[type] || !srv) { REPLY("error: no type %d or no game\n", type); return; }
    {   // off the map the game crashes (150,140 on a 128-tile map)
        uint8_t *map = *(uint8_t **)G_MAP, *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
        int w = grid ? FIELD(grid, 8, int) * FIELD(grid, 0x10, int) : 0, h = grid ? FIELD(grid, 8, int) * FIELD(grid, 0x14, int) : 0;
        if (x < 1 || y < 1 || x >= w - 2 || y >= h - 2) { REPLY("error: %d,%d is off the map (%d x %d)\n", x, y, w, h); return; }
    }
    int side = 1;
    while (side * side < n) side++;
    for (int i = 0; i < n; i++) {
        float xy[2] = { (float)(x + (i % side) * 2) + 0.5f, (float)(y + (i / side) * 2) + 0.5f };
        void *msg = ((void *(__cdecl *)(unsigned))0x00683718)(0x20);
        if (!msg) { REPLY("error: out of memory\n"); return; }
        ((void (__thiscall *)(void *, int *, float *, char))0x00650B3C)(msg, &type, xy, (char)p);
        ((void (__thiscall *)(void *, void *, int))(*(void ***)srv)[6])(srv, msg, 0xFF);
    }
    REPLY("ok: %d x type %d for P%d at %d,%d queued\n", n, type, p, x, y);
}

// spawnline <player> <type> <x0> <y0> <x1> <y1> [step]: one unit every <step> tiles (default 1) along the line,
// e.g. a wall. Same spawner as spawn.
static void cmd_spawnline(char *out, const char *args) {
    int p, type, x0, y0, x1, y1, step = 1;
    if (sscanf(args, "%d %d %d %d %d %d %d", &p, &type, &x0, &y0, &x1, &y1, &step) < 6 || p < 0 || p > 15 || type < 0 ||
        type >= 0x300 || step < 1) {
        REPLY("error: usage: spawnline <player> <type> <x0> <y0> <x1> <y1> [step]\n");
        return;
    }
    void *srv = *(void **)G_SERVER_OBJ;
    if (!((void **)G_TYPES)[type] || !srv) { REPLY("error: no type %d or no game\n", type); return; }
    int dx = x1 - x0, dy = y1 - y0, len = abs(dx) > abs(dy) ? abs(dx) : abs(dy), n = 0;
    for (int i = 0; i <= len; i += step) {
        float xy[2] = { (float)x0 + (len ? (float)(dx * i) / (float)len : 0) + 0.5f,
                        (float)y0 + (len ? (float)(dy * i) / (float)len : 0) + 0.5f };
        void *msg = ((void *(__cdecl *)(unsigned))0x00683718)(0x20);
        if (!msg) break;
        ((void (__thiscall *)(void *, int *, float *, char))0x00650B3C)(msg, &type, xy, (char)p);
        ((void (__thiscall *)(void *, void *, int))(*(void ***)srv)[6])(srv, msg, 0xFF);
        n++;
    }
    REPLY("ok: %d x type %d for P%d from %d,%d to %d,%d queued\n", n, type, p, x0, y0, x1, y1);
}

// bqueue P: the build manager's per-epoch queues (bq_entry); $ = can be built now (0x555710, the build manager's
// check: available in this epoch, affordable, a builder)
static void cmd_bqueue(char *out, const char *args) {
    int p = atoi(args);
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    void *cm = pl ? FIELD(pl, 0xB90, void *) : NULL;
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
    if (!bm) { REPLY("error: no AI player %d\n", p); return; }
    for (int e = 0; e < 14; e++) {
        int *ent = bq_entry(bm, e, 0);
        if (!ent) continue;
        REPLY("epoch %d:", e);
        for (unsigned i = 1; ent && i <= 60; ent = bq_entry(bm, e, i++))
            REPLY(" %d%s", ent[0], ((char (__thiscall *)(void *, int, int, int))0x00555710)(pl, ent[0], 0x20, 0) ? "$" : "");
        REPLY("\n");
    }
}

// count P: all units and buildings of player P by type (type: count)
static void cmd_count(char *out, const char *args) {
    int p = atoi(args);
    static int n[0x300];
    memset(n, 0, sizeof n);
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) { REPLY("error: no map\n"); return; }
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    unsigned count = FIELD(grid, 0x10, unsigned) * FIELD(grid, 0x14, unsigned);
    for (unsigned c = 0; cells && c < count; c++)
        for (uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **); b && b < FIELD(cells + c * 16, 8, uint8_t **); b++) {
            uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
            void *owner = u ? FIELD(u, 0x18, void *) : NULL;
            if (!td || !owner || FIELD(owner, PL_INDEX, int) != p) continue;
            if (FIELD(u, 0x1C, int) / FIELD(grid, 8, int) != (int)(c / FIELD(grid, 0x14, unsigned)) ||
                FIELD(u, 0x20, int) / FIELD(grid, 8, int) != (int)(c % FIELD(grid, 0x14, unsigned))) continue;
            int t = FIELD(td, 0x4B8, int);
            if (t >= 0 && t < 0x300) n[t]++;
        }
    REPLY("P%d:", p);
    for (int t = 0; t < 0x300; t++) if (n[t]) REPLY(" %d:%d", t, n[t]);
    REPLY("\n");
}

// objs P T: every unit of type T of player P - tile, hit points / the type's, goal (tests: are buildings finished?)
static void cmd_objs(char *out, const char *args) {
    int p = 0, t = -1;
    sscanf(args, "%d %d", &p, &t);
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) { REPLY("error: no map\n"); return; }
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    unsigned count = FIELD(grid, 0x10, unsigned) * FIELD(grid, 0x14, unsigned);
    REPLY("P%d type %d:", p, t);
    for (unsigned c = 0; cells && c < count; c++)
        for (uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **); b && b < FIELD(cells + c * 16, 8, uint8_t **); b++) {
            uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
            void *owner = u ? FIELD(u, 0x18, void *) : NULL;
            if (!td || !owner || FIELD(owner, PL_INDEX, int) != p || (t >= 0 && FIELD(td, 0x4B8, int) != t) || strlen(out) > 15000) continue;
            if (FIELD(u, 0x1C, int) / FIELD(grid, 8, int) != (int)(c / FIELD(grid, 0x14, unsigned)) ||
                FIELD(u, 0x20, int) / FIELD(grid, 8, int) != (int)(c % FIELD(grid, 0x14, unsigned))) continue;
            if (t < 0) REPLY(" T%d", FIELD(td, 0x4B8, int));   // all types: each with its type
            REPLY(" [%d,%d hp %d/%g %s]", FIELD(u, 0x1C, int), FIELD(u, 0x20, int), FIELD(u, 0x3C, int),
                  FIELD(td, 0x55C, float), rtti_name(FIELD(u, 0x1EC, void *)));
        }
    REPLY("\n");
}

// found P T x y: lay out a building of type T for player P at x, y, unfinished (1 hp) - as the build manager does
// (0x62EF8A cdecl(player, type, x, y), which also pays its cost, except for fields). Tests: an enemy tower going up.
static void *player_at(int i);
static void cmd_found(char *out, const char *args) {
    int p, t, x, y;
    if (sscanf(args, "%d %d %d %d", &p, &t, &x, &y) < 4) { REPLY("error: usage: found P T x y\n"); return; }
    void *pl = player_at(p);   // diplo.c: any player, human or AI
    if (!pl || t < 0 || t >= 724 || !((void **)0x009418A0)[t]) { REPLY("error: no player %d or type %d\n", p, t); return; }
    void *u = ((void *(__cdecl *)(void *, int, int, int))0x0062EF8A)(pl, t, x, y);
    REPLY("ok: type %d for P%d at %d,%d: %p\n", t, p, x, y, u);
}

// build P bx by x y r: player P's citizens within r tiles of x, y build (or repair) P's building at bx, by - the
// game's order (EEUGBuildRepair: new 0x5C, 0x60BAD0(goal, player, building, 1)). Tests: enemy builders at a tower.
static void cmd_build(char *out, const char *args) {
    int p, bx, by, x, y, r, n = 0;
    if (sscanf(args, "%d %d %d %d %d %d", &p, &bx, &by, &x, &y, &r) < 6) { REPLY("error: usage: build P bx by x y r\n"); return; }
    void *pl = player_at(p);
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!pl || !grid) { REPLY("error: no player %d or map\n", p); return; }
    int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
    uint8_t *cells = FIELD(grid, 4, uint8_t *), *bld = NULL, *civ[32];
    int nc = 0;
    for (int cx = (x - r - 4) / cs; cx <= (x + r + 4) / cs; cx++)   // wide enough for the building too
        for (int cy = (y - r - 4) / cs; cy <= (y + r + 4) / cs; cy++) {
            if (cx < 0 || cy < 0 || cx >= nx || cy >= ny) continue;
            uint8_t *cell = cells + (cx * ny + cy) * 16;
            for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **); b++) {
                uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                if (!td || FIELD(u, 0x18, void *) != pl || FIELD(u, 0x3C, int) <= 0) continue;
                int t = FIELD(td, 0x4B8, int), ux = FIELD(u, 0x1C, int), uy = FIELD(u, 0x20, int);
                if (ux == bx && uy == by && (FIELD(td, 0x4B4, int) == 2 || FIELD(td, 0x4B4, int) == 29)) bld = u;
                else if ((t == 32 || t == 480) && abs(ux - x) <= r && abs(uy - y) <= r && nc < 32) {
                    int dup = 0;
                    for (int k = 0; k < nc; k++) dup |= civ[k] == u;
                    if (!dup) civ[nc++] = u;
                }
            }
        }
    if (!bld) { REPLY("error: no building of P%d at %d,%d\n", p, bx, by); return; }
    for (int k = 0; k < nc; k++) {
        void *g = ((void *(__cdecl *)(unsigned))0x00683718)(0x5C);
        if (g) g = ((void *(__thiscall *)(void *, void *, void *, int))0x0060BAD0)(g, pl, bld, 1);
        n += unit_goal(civ[k], g, 0, 0x1F51);
    }
    REPLY("ok: %d citizens of P%d build at %d,%d\n", n, p, bx, by);
}

// watchtype P T [off]: hardware write-watch on the first unit of type T of player P, at offset off (default 0x2C,
// the type data: who changes a unit's type). Tests.
static void watch_set(uintptr_t addr, int len);
static void cmd_watchtype(char *out, const char *args) {
    int p = 0, t = 0;
    unsigned off = 0x2C;
    sscanf(args, "%d %d %x", &p, &t, &off);
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) { REPLY("error: no map\n"); return; }
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    unsigned count = FIELD(grid, 0x10, unsigned) * FIELD(grid, 0x14, unsigned);
    for (unsigned c = 0; cells && c < count; c++)
        for (uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **); b && b < FIELD(cells + c * 16, 8, uint8_t **); b++) {
            uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
            void *owner = u ? FIELD(u, 0x18, void *) : NULL;
            if (!td || !owner || FIELD(owner, PL_INDEX, int) != p || FIELD(td, 0x4B8, int) != t) continue;
            watch_set((unsigned)(uintptr_t)(u + off), 4);
            REPLY("ok: watching unit %p +%x (type %d of P%d at %d,%d)\n", u, off, t, p, FIELD(u, 0x1C, int), FIELD(u, 0x20, int));
            return;
        }
    REPLY("error: no unit of type %d for P%d\n", t, p);
}

// hurt P T x y r dmg: damage every object of type T of player P within r tiles of x, y the way an attack does
// (0x61E0B2: target vf 0x70 thiscall(attacker, damage, attacker's player index, 1)); attacker = the object itself
// (tests: destroy farm fields, buildings)
static void cmd_hurt(char *out, const char *args) {
    int p, t, x, y, r, dmg, n = 0;
    if (sscanf(args, "%d %d %d %d %d %d", &p, &t, &x, &y, &r, &dmg) < 6) { REPLY("error: usage: hurt P T x y r dmg\n"); return; }
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) { REPLY("error: no map\n"); return; }
    int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
    uint8_t *cells = FIELD(grid, 4, uint8_t *), *hit[64];
    for (int cx = (x - r) / cs; cx <= (x + r) / cs; cx++)
        for (int cy = (y - r) / cs; cy <= (y + r) / cs; cy++) {
            if (cx < 0 || cy < 0 || cx >= nx || cy >= ny) continue;
            uint8_t *cell = cells + (cx * ny + cy) * 16;
            for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **) && n < 64; b++) {
                uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                void *owner = u ? FIELD(u, 0x18, void *) : NULL;
                if (!td || !owner || FIELD(owner, PL_INDEX, int) != p || FIELD(td, 0x4B8, int) != t || FIELD(u, 0x3C, int) <= 0) continue;
                if (abs(FIELD(u, 0x1C, int) - x) > r || abs(FIELD(u, 0x20, int) - y) > r) continue;
                int dup = 0;
                for (int k = 0; k < n; k++) dup |= hit[k] == u;
                if (!dup) hit[n++] = u;
            }
        }
    for (int k = 0; k < n; k++)   // collected first: a dying object may leave its cell
        (*(void (__thiscall **)(void *, void *, int, int, int))(FIELD(hit[k], 0, uint8_t *) + 0x70))(hit[k], hit[k], dmg, p, 1);
    REPLY("ok: %d objects hit for %d\n", n, dmg);
}

// Soldiers of player P (any player, AI or not) grouped by current goal (unit+0x1EC): count, centre, and the
// goal's first dwords of the first unit (to find what it is after).
// STRESS WAVES (tests): a grown AI against ever stronger attacks. "wave A D pct [x y]" gives player A (e.g. the idle
// P1) pct% of player D's army, type by type - the most of each type D has had at a wave so far (a mirror: the right
// epoch, a fair mix, and waves that keep growing while D's army shrinks; ships and aircraft left out), at x,y
// (default: beside A's buildings); A and D are made enemies (the test maps ally the idle human with every AI).
// "charge A D" (then every 10 s, at most 80 orders a pass): each soldier of A attacks the nearest soldier of D within 15 tiles, else the nearest citizen within
// 20, else D's nearest building (one already fighting a unit of D keeps it). The first runs went for the nearest
// building only: 500 archers at the walls razed 16 buildings in 45 minutes. Setup lines, e.g. "@60 wave 1 2 50",
// "@60.5 charge 1 2" ... - the minute D falls (game over) is the score.
struct w_scan { int n; uint8_t *u[1024]; };
static void units_of(int p, int want, struct w_scan *o) {   // want: 0 soldiers (cmd_army's filter), 1 the rest
                                                             // (buildings, walls, towers), 2 citizens
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    o->n = 0;
    if (!grid) return;
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    int cs = FIELD(grid, 8, int);
    unsigned ny = FIELD(grid, 0x14, unsigned), count = FIELD(grid, 0x10, unsigned) * ny;
    for (unsigned c = 0; cells && c < count; c++)
        for (uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **); b && b < FIELD(cells + c * 16, 8, uint8_t **); b++) {
            uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
            void *owner = u ? FIELD(u, 0x18, void *) : NULL;
            if (!td || !owner || FIELD(owner, PL_INDEX, int) != p || FIELD(u, 0x3C, int) <= 0) continue;
            if (FIELD(u, 0x1C, int) / cs != (int)(c / ny) || FIELD(u, 0x20, int) / cs != (int)(c % ny)) continue;
            int fam = FIELD(td, 0x4B4, int), ty = FIELD(td, 0x4B8, int);
            int civ = ty == 32 || ty == 480, sold = !civ && !(fam == 2 || fam == 6 || fam == 0x13 || fam == 0x16 ||
                fam == 0x18 || fam == 0x19 || fam == 0x1D || fam == 0 || fam == 1);
            if (want == 2 ? !civ : want == 1 ? civ || sold : !sold) continue;   // 1: the rest - buildings, walls, towers
            if (o->n < 1024) o->u[o->n++] = u;
        }
}
static int is_ship_or_air(uint8_t *td) {   // movement type +0x10C... unknown: by the ship/air families seen in tests
    int fam = FIELD(td, 0x4B4, int);
    return fam == 0x0B || fam == 0x0C || fam == 0x0D || fam == 0x0E || fam == 0x0F || fam == 0x10 || fam == 0x11;
}
static void cmd_wave(char *out, const char *args) {
    int a, d, pct, x = -1, y = -1;
    if (sscanf(args, "%d %d %d %d %d", &a, &d, &pct, &x, &y) < 3 || a < 0 || a > 15 || d < 0 || d > 15 || pct < 1) {
        REPLY("error: usage: wave A D pct [x y]\n");
        return;
    }
    static struct w_scan sc;
    uint8_t **pls = (uint8_t **)G_PLAYERS;
    if (!pls[a] || !pls[d]) { REPLY("error: no player %d or %d\n", a, d); return; }
    void (__thiscall *set_dip)(void *, void *, int, int) = (void *)FN_SET_DIPLOMACY;
    if (FIELD(pls[a], 0x9E4 + d * 4, int) != 1) set_dip(pls[a], pls[d], 1, 0);   // 1 = enemy
    if (FIELD(pls[d], 0x9E4 + a * 4, int) != 1) set_dip(pls[d], pls[a], 1, 0);
    // the spot: A's Town Center (or Capitol, or any building) at the first wave, kept for the later ones (D razes A's
    // base once they are at war). Beside A's buildings on a 2-tile grid, part of each wave stood in a pocket for good.
    static struct { uint32_t t; int x, y; } spot[16];
    if (game_time() < spot[a].t) spot[a].t = 0, spot[a].x = -1;   // a new game
    if (x < 0 && spot[a].t && spot[a].x >= 0) x = spot[a].x, y = spot[a].y;
    if (x < 0) {
        units_of(a, 1, &sc);
        for (int i = 0; i < sc.n; i++) {
            int t = FIELD(FIELD(sc.u[i], 0x2C, uint8_t *), 0x4B8, int);
            if (x < 0 || t == 35 || t == 41) x = FIELD(sc.u[i], 0x1C, int), y = FIELD(sc.u[i], 0x20, int);
            if (t == 35) break;
        }
        if (x < 0) { REPLY("error: P%d has no buildings, give x y\n", a); return; }
    }
    spot[a].t = game_time() ? game_time() : 1; spot[a].x = x; spot[a].y = y;
    units_of(d, 0, &sc);
    // the mix: per type the most D has had at a wave (new game: game time went back)
    static struct { uint32_t t; int nt, type[64], cnt[64], fam[64]; } mix[16];
    uint32_t now = game_time();
    if (now < mix[d].t) mix[d].nt = 0;
    mix[d].t = now;
    int ct[64] = { 0 }, total = 0, sent = 0, left = 0;
    for (int i = 0; i < sc.n; i++) {
        uint8_t *td = FIELD(sc.u[i], 0x2C, uint8_t *);
        int t = FIELD(td, 0x4B8, int), j = 0;
        if (is_ship_or_air(td)) { left++; continue; }
        while (j < mix[d].nt && mix[d].type[j] != t) j++;
        if (j == mix[d].nt) {
            if (j == 64) continue;
            mix[d].type[j] = t; mix[d].cnt[j] = 0; mix[d].fam[j] = FIELD(td, 0x4B4, int); mix[d].nt++;
        }
        ct[j]++; total++;
    }
    int base = 0;
    for (int j = 0; j < mix[d].nt; j++) {
        if (ct[j] > mix[d].cnt[j]) mix[d].cnt[j] = ct[j];
        base += mix[d].cnt[j];
    }
    int nt = mix[d].nt, *type = mix[d].type, *cnt = mix[d].cnt, *fam = mix[d].fam;
    REPLY("wave: P%d gets %d%% of P%d's army (%d soldiers now, mix of %d) at %d,%d:", a, pct, d, total, base, x, y);
    char tmp[256], args2[64];
    static const int ring[8][2] = { { 9, 0 }, { 0, 9 }, { -9, 0 }, { 0, -9 }, { 6, 6 }, { -6, 6 }, { -6, -6 }, { 6, -6 } };
    int chunk = 0;
    for (int j = 0; j < nt; j++) {
        int n = (cnt[j] * pct + 50) / 100;
        while (n > 0) {   // groups of 16 (a 4x4 grid, 2 tiles apart) around the spot
            int k = n > 16 ? 16 : n, r = chunk++ % 8;
            sprintf(args2, "%d %d %d %d %d", a, type[j], x + ring[r][0] - 3, y + ring[r][1] - 3, k);
            tmp[0] = 0;   // cmd_spawn appends its reply
            cmd_spawn(tmp, args2);
            n -= k; sent += k;
        }
        REPLY(" %dx%d(f%d)", (cnt[j] * pct + 50) / 100, type[j], fam[j]);
    }
    REPLY(" - %d units (%d ships/aircraft left out)\n", sent, left);
}
static struct { int a, d; uint32_t t, logged; unsigned rot; } g_charge = { -1, -1, 0, 0, 0 };
static void charge_pass(char *out, int a, int d) {
    int n = 0, kept = 0, idle = 0, marching = 0;
    static struct w_scan mine, foe, bld, civ;
    units_of(a, 0, &mine);
    units_of(d, 0, &foe);
    units_of(d, 1, &bld);
    units_of(d, 2, &civ);
    for (int i0 = 0; i0 < mine.n; i0++) {
        int i = (int)((i0 + g_charge.rot) % (unsigned)mine.n);
        uint8_t *u = mine.u[i], *goal = FIELD(u, 0x1EC, uint8_t *);
        if (goal && !strcmp(rtti_name(goal), "EEUGAttackUnit@@")) {
            uint8_t *t = FIELD(goal, 0x30, uint8_t *);
            void *to = t ? FIELD(t, 0x18, void *) : NULL;
            if (to && FIELD(to, PL_INDEX, int) == d && FIELD(t, 0x3C, int) > 0) { kept++; continue; }
        }
        if (goal && !strcmp(rtti_name(goal), "EEUGLocation@@")) { marching++; continue; }
        if (++idle > 80) continue;   // 80 orders a pass: 500 at once lost their goal within 10 s (the path search)
        float ux = (float)FIELD(u, 0x1C, int), uy = (float)FIELD(u, 0x20, int), best = 15.0f * 15.0f;
        uint8_t *tgt = NULL;
        for (int k = 0; k < foe.n; k++) {
            float dx = (float)FIELD(foe.u[k], 0x1C, int) - ux, dy = (float)FIELD(foe.u[k], 0x20, int) - uy;
            if (dx * dx + dy * dy < best) { best = dx * dx + dy * dy; tgt = foe.u[k]; }
        }
        if (!tgt) {
            best = 20.0f * 20.0f;
            for (int k = 0; k < civ.n; k++) {
                float dx = (float)FIELD(civ.u[k], 0x1C, int) - ux, dy = (float)FIELD(civ.u[k], 0x20, int) - uy;
                if (dx * dx + dy * dy < best) { best = dx * dx + dy * dy; tgt = civ.u[k]; }
            }
        }
        if (!tgt) {
            best = 1e12f;
            for (int k = 0; k < bld.n; k++) {
                float dx = (float)FIELD(bld.u[k], 0x1C, int) - ux, dy = (float)FIELD(bld.u[k], 0x20, int) - uy;
                if (dx * dx + dy * dy < best) { best = dx * dx + dy * dy; tgt = bld.u[k]; }
            }
        }
        if (!tgt) continue;
        float tx = (float)FIELD(tgt, 0x1C, int), ty = (float)FIELD(tgt, 0x20, int);
        if ((tx - ux) * (tx - ux) + (ty - uy) * (ty - uy) > 20.0f * 20.0f)   // far: walk there first
            n += unit_goal(u, goal_move(tx + (ux - tx) * 0.1f, ty + (uy - ty) * 0.1f), 0, 0x1F40);
        else n += unit_goal(u, goal_attack(u, tgt), 0, 0x1F40);
    }
    g_charge.rot += 80;
    if (out) sprintf(out + strlen(out), "charge: P%d %d soldiers, %d without a target, %d sent, %d marching, %d fighting; P%d %d "
                     "soldiers, %d citizens, %d buildings\n", a, mine.n, idle, n, marching, kept, d, foe.n, civ.n, bld.n);
}
// charge A D: from now on every 10 s (attack_tick), logged every minute; "charge off" stops it
static void cmd_charge(char *out, const char *args) {
    int a, d;
    if (!strncmp(args, "off", 3)) { g_charge.a = g_charge.d = -1; REPLY("charge: off\n"); return; }
    if (sscanf(args, "%d %d", &a, &d) < 2 || a < 0 || a > 15 || d < 0 || d > 15) { REPLY("error: usage: charge A D | off\n"); return; }
    g_charge.a = a; g_charge.d = d; g_charge.t = g_charge.logged = game_time();
    charge_pass(out, a, d);
}
static void charge_tick(uint32_t now) {
    if (g_charge.d < 0) return;
    if (now < g_charge.t) { g_charge.a = g_charge.d = -1; return; }   // a new game
    if (now - g_charge.t < 10000) return;
    g_charge.t = now;
    static char line[256];
    line[0] = 0;
    charge_pass(now - g_charge.logged >= 60000 ? line : NULL, g_charge.a, g_charge.d);
    if (line[0] && g_log) {
        g_charge.logged = now;
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# %.1f min %s", now / 60000.0, line);
        fflush(g_log);
        LeaveCriticalSection(&g_lock);
    }
}
static void cmd_army(char *out, const char *args) {
    int p = 0, only = -1;   // army P [type]: only that unit type
    sscanf(args, "%d %d", &p, &only);
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
            if (only >= 0 && FIELD(td, 0x4B8, int) != only) continue;
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
