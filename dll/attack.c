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
static void __fastcall hook_asg_route(uint8_t *self, void *edx, int *start, int *dest) {
    (void)edx;
    void *pl = FIELD(self, 0x190, void *);
    void *target = FIELD(self, 0x11C, void *);
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
static unsigned g_ram_waits;
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
    // breach: the scorer rates walls below zero (never chosen while anything else is around); the subgroup's
    // breach segment and its neighbours become the best targets
    int bx, by;
    uint8_t *etd = FIELD(enemy, 0x2C, uint8_t *);
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
static struct { int tx, ty, ttype, px, py; uint32_t last; unsigned logged; } g_siege_st[16];

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

static void siege_train(void *pl, unsigned pi, int have, uint32_t now) {
    int want = g_threat[pi].n ? 2 + g_threat[pi].n / 4 : 0;
    for (int k = 0; k < 64 && !want; k++)
        if (g_breaches[k].sg && FIELD(g_breaches[k].sg, 0x190, void *) == pl) want = 2;
    if (want > 4) want = 4;
    if (have >= want || (now >= g_siege_train[pi].last_try && now - g_siege_train[pi].last_try < 30000)) return;
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
    siege_train(pl, pi, ns, now);
    if (ns < 2) return;
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
    float dir = atan2f(sy - (float)ty, sx - (float)tx), r = srange - 0.7f, px = 0, py = 0;
    int found = 0;
    for (int k = 0; k <= 12 && !found; k++) {
        float a = dir + (k & 1 ? 1.0f : -1.0f) * (float)((k + 1) / 2) * 0.2618f;   // 0, +-15, +-30 ... +-90 degrees
        px = (float)tx + 0.5f + r * cosf(a); py = (float)ty + 0.5f + r * sinf(a);
        found = !covered(pi, px, py, tx, ty);
    }
    if (!found) return;
    int changed = tx != g_siege_st[pi].tx || ty != g_siege_st[pi].ty;
    g_siege_st[pi].tx = tx; g_siege_st[pi].ty = ty; g_siege_st[pi].ttype = FIELD(FIELD(t, 0x2C, uint8_t *), 0x4B8, int);
    g_siege_st[pi].px = (int)px; g_siege_st[pi].py = (int)py;
    // siege: move to the firing position, then attack (orders only when not already doing that)
    int ordered = 0;
    for (int i = 0; i < ns; i++) {
        uint8_t *u = siege[i], *g = FIELD(u, 0x1EC, uint8_t *);
        if (g && *(uintptr_t *)g == VT_GOAL_ATTACK && FIELD(g, 0x30, uint8_t *) == t) continue;
        float ux = (float)FIELD(u, 0x1C, int) + 0.5f - px, uy = (float)FIELD(u, 0x20, int) + 0.5f - py;
        if (ux * ux + uy * uy > 4.0f) {
            if (g && *(uintptr_t *)g == VT_GOAL_LOCATION && !changed) continue;   // on its way
            ordered += unit_goal(u, goal_move(px, py), 0, 0x1F40) && unit_goal(u, goal_attack(u, t), 1, 0x1F40);
        } else
            ordered += unit_goal(u, goal_attack(u, t), 0, 0x1F40);
    }
    // escorts: idle soldiers (no goal) to the firing position
    int want = ns * 3 / 2 < 4 ? 4 : ns * 3 / 2, escorts = 0;
    for (unsigned c = 0; cells && c < count && escorts < want && escorts < MAXE; c++)
        for (uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **); b && b < FIELD(cells + c * 16, 8, uint8_t **) && escorts < want; b++) {
            uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
            if (!td || FIELD(u, 0x18, void *) != pl || FIELD(u, 0x1EC, void *) || FIELD(u, 0x3C, int) <= 0) continue;
            int fam = FIELD(td, 0x4B4, int), type = FIELD(td, 0x4B8, int);
            if (fam != 5 || is_siege(type) || type == 32 || type == 102 || type == 287) continue;   // soldiers only (no citizens, rams)
            float ex = (float)FIELD(u, 0x1C, int) - px, ey = (float)FIELD(u, 0x20, int) - py;
            if (ex * ex + ey * ey < 9.0f) { escorts++; continue; }   // already there
            escorts += unit_goal(u, goal_move(px + (float)(escorts % 3) - 1.0f, py + (float)(escorts / 3 % 3) - 1.0f), 0, 0x1F40);
        }
    if (g_log && (changed || ordered) && g_siege_st[pi].logged < 40) {
        g_siege_st[pi].logged++;
        fprintf(g_log, "# siege: P%u at %.1f min: %d siege (range %.1f) at %.0f,%.0f -> type %d at %d,%d from %.0f,%.0f; %d ordered, %d escorts\n",
                pi, now / 60000.0, ns, srange, sx, sy, g_siege_st[pi].ttype, tx, ty, px, py, ordered, escorts);
        fflush(g_log);
    }
}

static void attack_tick(void *pl, uint32_t now) {
    threat_refresh(pl, now);   // also with avoid_towers=0: the route log compares both
    siege_tick(pl, now);
    {
        unsigned i = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
        if (g_log && now / 300000 != g_mil[i].last_log / 300000) {
            fprintf(g_log, "# army: P%u at %u min: %d soldiers; military requests %u, trained %u (substituted %u), failed short of "
                    "food %u wood %u stone %u gold %u iron %u; budget left %d/%d/%d/%d/%d\n", i, now / 60000,
                    army_size(pl), g_mil[i].req, g_mil[i].trained, g_mil[i].subs, g_mil[i].short_of[2], g_mil[i].short_of[3],
                    g_mil[i].short_of[4], g_mil[i].short_of[5], g_mil[i].short_of[6], (int)g_bud[i].mil[2],
                    (int)g_bud[i].mil[3], (int)g_bud[i].mil[4], (int)g_bud[i].mil[5], (int)g_bud[i].mil[6]);
            fprintf(g_log, "# army: P%u failed types:", i);
            for (int k = 0; k < 4; k++)
                if (g_mil[i].fcount[k])
                    fprintf(g_log, " type %d from %06x x%u", g_mil[i].ftype[k] & 0x7FF,
                            (unsigned)g_mil[i].ftype[k] >> 11, g_mil[i].fcount[k]);
            fputc('\n', g_log);
            fprintf(g_log, "# leash: P%u: enemies in tower range skipped as targets %u times; rams held back %u (all), siege trained %u, "
                    "failed %u, no factory %u\n", i, g_leash_stats[i].skipped, g_ram_waits, g_siege_train[i].trained,
                    g_siege_train[i].failed, g_siege_train[i].no_factory);
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
    g_avoid_margin = cfg_int(aimod_ini, autotest_ini, 1, "avoid_towers_margin", 6);
    g_weak_spots = cfg_int(aimod_ini, autotest_ini, 1, "attack_weak_spots", 1);
    g_weak_radius = cfg_int(aimod_ini, autotest_ini, 1, "attack_weak_spot_radius", 60);
    g_leash = cfg_int(aimod_ini, autotest_ini, 1, "attack_leash", 1);
    g_breach = cfg_int(aimod_ini, autotest_ini, 1, "attack_breach", 1);
    g_siege = cfg_int(aimod_ini, autotest_ini, 1, "attack_siege", 1);
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
static void cmd_spawn(char *out, const char *args) {
    int p, type, x, y, n = 1;
    if (sscanf(args, "%d %d %d %d %d", &p, &type, &x, &y, &n) < 4 || p < 0 || p > 15 || type < 0 || type >= 0x300 || n < 1 || n > 100) {
        REPLY("error: usage: spawn <player> <type> <x> <y> [count]\n");
        return;
    }
    void *srv = *(void **)G_SERVER_OBJ;
    if (!((void **)G_TYPES)[type] || !srv) { REPLY("error: no type %d or no game\n", type); return; }
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

// Soldiers of player P (any player, AI or not) grouped by current goal (unit+0x1EC): count, centre, and the
// goal's first dwords of the first unit (to find what it is after).
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
