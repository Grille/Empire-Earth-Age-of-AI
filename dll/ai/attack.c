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

#define MAX_DEFENCES 256
struct defence { int x, y, owner; float range, attack; int type; ee_unit *u; };
internal struct {
    struct defence d[MAX_DEFENCES];
    int n;
    uint32_t refreshed;
} g_threat[16];

internal int g_avoid_towers = 1;        // aimod.ini [all] avoid_towers
internal int g_avoid_weight = 2000;     // route cost per tower covering a step (a step costs 10-14)
internal int g_avoid_margin = 6;        // tiles added to the tower's range


// What player pl knows of unit u. The count (u +0x14 -> +0xC, per player slot) is how many of pl's (and its allies')
// units see u now - plus a floor every unit of the human carries in the AIs' slots, everywhere and always (the AIs
// see all of the human's units; docs/engine-notes.md). [all] honest_vision (intel.c): seen = above that floor, or a
// building pl has seen before (still where it was - a human remembers it on the map), the default; 0: the count as
// the game keeps it - all-knowing about the human.
internal int g_honest_vision = 1;
internal uint8_t g_vis_floor[16][16];               // [viewer][owner]: the least count among the owner's units (intel.c)
internal int intel_remembers(ee_player *pl, ee_unit *u);   // intel.c: a building pl has seen
internal void intel_soldiers(ee_player *pl, int x, int y, int r, float *hp, float *att, float *pw, int *n);   // intel.c
internal void intel_army(ee_player *pl, float *hp, float *att, float *pw, int *n);
internal uint32_t intel_looked_ago(ee_player *pl, int x, int y);
internal int seen_by(ee_unit *u, ee_player *pl) {
    uint8_t *v = FIELD(u, OF(UNIT, 0x14), uint8_t *);
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int);
    if (!v || pi >= 16) return 0;
    uint16_t *vis = FIELD(v, 0xC, uint16_t *);
    if (!vis) return 0;
    unsigned c = vis[pi] & 0xFF;
    if (!g_honest_vision) return c != 0;
    ee_player *o = unit_owner(u);
    unsigned oi = o ? (unsigned)FIELD(o, PL_INDEX, int) & 15 : 0;
    if (c > g_vis_floor[pi][oi]) return 1;
    int fam = FIELD(FIELD(u, OF(UNIT, 0x2C), uint8_t *), OF(TD, 0x4B4), int);
    return (fam == 2 || fam == 29) && intel_remembers(pl, u);
}

// Defence building with a ground attack (AA guns and missile towers only shoot at aircraft).
internal int defence_stats(ee_unit *u, float *range, float *attack) {
    uint8_t *td = FIELD(u, OF(UNIT, 0x2C), uint8_t *);
    if (!td) return 0;
    int fam = FIELD(td, OF(TD, 0x4B4), int), type = FIELD(td, OF(TD, 0x4B8), int);
    if (fam != 2 && fam != 29) return 0;
    if (type == UT(B_AA10_GUN_88MM) || type == UT(B_AA13_MISSILE_TOWER)) return 0;
    uint8_t *st = FIELD(u, OF(UNIT, 0x34), uint8_t *);
    float r = st ? FIELD(st, OF(USTAT, 0x9C), float) : FIELD(td, OF(TD, 0x420), float);
    float a = st ? FIELD(st, OF(USTAT, 0xD8), float) : (float)FIELD(td, OF(TD, 0x510), int);
    if (!(a > 0.0f) || !(r > 1.0f) || r > 30.0f) return 0;
    *range = r; *attack = a;
    return 1;
}

internal void threat_refresh(ee_player *pl, uint32_t now) {
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (g_threat[pi].refreshed && now >= g_threat[pi].refreshed && now - g_threat[pi].refreshed < 5000) return;
    g_threat[pi].refreshed = now ? now : 1;
    int n = 0;
    for (struct grid_it it = grid_it_all(1); n < MAX_DEFENCES && grid_it_next(&it); ) {
        ee_unit *u = it.u;
        float r, a;
        if (!is_enemy(pl, unit_owner(u)) || !defence_stats(u, &r, &a) || !seen_by(u, pl)) continue;
        int x = FIELD(u, OF(UNIT, 0x1C), int), y = FIELD(u, OF(UNIT, 0x20), int), k;
        for (k = 0; k < n; k++)                     // one defence per tile (the iterator already visits a big building once)
            if (g_threat[pi].d[k].x == x && g_threat[pi].d[k].y == y) break;
        if (k < n) continue;
        struct defence *d = &g_threat[pi].d[n++];
        d->x = x; d->y = y; d->range = r; d->attack = a;
        d->owner = FIELD(unit_owner(u), PL_INDEX, int);
        d->type = FIELD(FIELD(u, OF(UNIT, 0x2C), uint8_t *), OF(TD, 0x4B8), int);
        d->u = u;
    }
    g_threat[pi].n = n;
}

// Sum of known defences covering tile (x, y), each weighted by its attack (a guard tower = 1).
internal float threat_at(unsigned pi, int x, int y) {
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
internal struct { unsigned calls, penalised; } g_route_stats[16];
internal DWORD g_route_thread;

internal int __fastcall hook_route_cost(void *self, void *edx, int *edge, uint8_t *node) {
    (void)edx;
    int cost = route_cost(self, edge, node);
    unsigned pi = (unsigned)FIELD(self, OF(ROUTEPLANNER, 0x48), int) & 15;     // planner +0x48 = own player index
    ee_player *pl = is_vanilla_idx((int)pi) ? NULL : g_players[pi];   // a vanilla player: the game's cost
    if (!g_route_thread) {
        g_route_thread = GetCurrentThreadId();
        if (g_log) fprintf(g_log, "# routes: cost hook runs on thread %lu\n", g_route_thread);
    }
    if (!pl || !node) return cost;
    g_route_stats[pi].calls++;
    float t = threat_at(pi, FIELD(node, OF(ROUTENODE, 0x3C), int), FIELD(node, OF(ROUTENODE, 0x40), int));
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
internal int g_weak_spots = 1;          // aimod.ini [all] attack_weak_spots
internal int g_weak_radius = 60;        // tiles around the original goal searched for a weak spot


// Only goals at an enemy building are retargeted: the subgroup also marches to other goals (enemy armies,
// own territory to defend), which must not be diverted.
internal int building_at(ee_player *pl, const int *dest, int r) {
    uint8_t *map = *(uint8_t **)GA(G_MAP);
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return 0;
    for (struct grid_it it = grid_it_box(dest[0], dest[1], r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        uint8_t *td = it.td;
        if (!td || FIELD(td, OF(TD, 0x4B4), int) != 2 || is_wall(FIELD(td, OF(TD, 0x4B8), int)) || !is_enemy(pl, unit_owner(u)))
            continue;
        int dx = FIELD(u, OF(UNIT, 0x1C), int) - dest[0], dy = FIELD(u, OF(UNIT, 0x20), int) - dest[1];
        if (dx * dx + dy * dy <= r * r) return 1;
    }
    return 0;
}

internal ee_unit *find_weak_spot(ee_player *pl, unsigned pi, const int *start, const int *dest) {
    uint8_t *map = *(uint8_t **)GA(G_MAP);
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return NULL;
    short home = (short)map_area((int *)start, 2);
    ee_unit *best = NULL;
    float best_score = 1e30f;
    int r = g_weak_radius;
    for (struct grid_it it = grid_it_box(dest[0], dest[1], r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        uint8_t *td = it.td;
        if (!td || FIELD(td, OF(TD, 0x4B4), int) != 2 || is_wall(FIELD(td, OF(TD, 0x4B8), int)) || !is_enemy(pl, unit_owner(u)) ||
            !seen_by(u, pl))
            continue;
        int pos[2] = { FIELD(u, OF(UNIT, 0x1C), int), FIELD(u, OF(UNIT, 0x20), int) };
        float dx = (float)(pos[0] - dest[0]), dy = (float)(pos[1] - dest[1]);
        if (dx * dx + dy * dy > (float)(r * r) || threat_at(pi, pos[0], pos[1]) > 0) continue;
        if ((short)map_area(pos, 2) != home) continue;
        float sx = (float)(pos[0] - start[0]), sy = (float)(pos[1] - start[1]);
        float score = sqrtf(sx * sx + sy * sy) + 0.5f * sqrtf(dx * dx + dy * dy);
        if (score < best_score) { best_score = score; best = u; }
    }
    return best;
}

// Breach: the route planner works on ~10-tile sectors and ignores walls, so an army marches straight to an enemy
// wall and piles up in front of it - next to the towers that guard the gap. When the straight line to the goal
// crosses a known enemy wall, the subgroup first attacks a wall segment outside known tower range, the one with
// the shortest detour along that wall (find_breach); the scorer makes that segment the units' target (walls score
// below zero otherwise). Walls are tough: without siege the breach takes long or fails.
internal int g_breach = 1;              // aimod.ini [all] attack_breach
// The owner is kept with the entry: a subgroup the game deleted (its attack over) is never read again.
internal struct { void *sg, *pl; int x, y; } g_breaches[64];   // subgroup (of player pl) -> wall tile it is to breach

internal void breach_note(ee_player *pl, void *sg, int x, int y) {
    int free_k = -1;
    for (int k = 0; k < 64; k++) {
        if (g_breaches[k].sg == sg) { g_breaches[k].x = x; g_breaches[k].y = y; if (x < 0) g_breaches[k].sg = NULL; return; }
        if (!g_breaches[k].sg && free_k < 0) free_k = k;
    }
    if (x < 0) return;
    static int next;
    if (free_k < 0) free_k = next++ & 63;   // full: overwrite one in turn (not by the subgroup's address: the heap differs between runs)
    g_breaches[free_k].sg = sg; g_breaches[free_k].pl = pl; g_breaches[free_k].x = x; g_breaches[free_k].y = y;
}

// before the game deletes subgroups of player pl (diplo.c: attacks called off): forget their breaches
internal void breach_forget(ee_player *pl) {
    for (int k = 0; k < 64; k++)
        if (g_breaches[k].sg && g_breaches[k].pl == pl) g_breaches[k].sg = NULL;
}
// every AI tick: breaches of subgroups no longer in pl's attack groups (the attack ended, the subgroup was deleted -
// nothing told the table, and a phantom breach kept siege_want at 2 and the siege firing at that wall for good)
internal int asg_live(ee_player *pl, uint8_t *sg);   // assault.c
internal void breach_prune(ee_player *pl) {
    for (int k = 0; k < 64; k++)
        if (g_breaches[k].sg && g_breaches[k].pl == pl && !asg_live(pl, g_breaches[k].sg)) g_breaches[k].sg = NULL;
}

internal int breach_of(void *sg, int *x, int *y) {
    for (int k = 0; k < 64; k++)
        if (g_breaches[k].sg == sg) { *x = g_breaches[k].x; *y = g_breaches[k].y; return 1; }
    return 0;
}

internal ee_unit *wall_at(ee_player *pl, int x, int y) {
    uint8_t *map = *(uint8_t **)GA(G_MAP);
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return NULL;
    int cs = FIELD(grid, OF(GRID, 0x8), int), nx = FIELD(grid, OF(GRID, 0x10), int), ny = FIELD(grid, OF(GRID, 0x14), int);
    uint8_t *cells = FIELD(grid, OF(GRID, 0x4), uint8_t *);
    if (cs <= 0 || !cells || x < 0 || y < 0 || x / cs >= nx || y / cs >= ny) return NULL;
    uint8_t *cell = cells + ((x / cs) * ny + y / cs) * 16;
    for (ee_unit **b = FIELD(cell, OF(GRIDCELL, 0x4), ee_unit **); b && b < FIELD(cell, OF(GRIDCELL, 0x8), ee_unit **); b++) {
        ee_unit *u = *b;
        uint8_t *td = u ? FIELD(u, OF(UNIT, 0x2C), uint8_t *) : NULL;
        if (td && FIELD(u, OF(UNIT, 0x1C), int) == x && FIELD(u, OF(UNIT, 0x20), int) == y && is_wall(FIELD(td, OF(TD, 0x4B8), int)) &&
            is_enemy(pl, unit_owner(u)) && seen_by(u, pl))
            return u;
    }
    return NULL;
}

// First known enemy wall tile on the straight line start -> dest (half-tile steps), or NULL.
internal ee_unit *wall_crossing(ee_player *pl, const int *start, const int *dest) {
    int dx = dest[0] - start[0], dy = dest[1] - start[1];
    int n = 2 * (abs(dx) > abs(dy) ? abs(dx) : abs(dy));
    for (int i = 1; i < n; i++) {
        ee_unit *w = wall_at(pl, start[0] + (dx * i + (dx >= 0 ? n / 2 : -n / 2)) / n,
                             start[1] + (dy * i + (dy >= 0 ? n / 2 : -n / 2)) / n);
        if (w) return w;
    }
    return NULL;
}

// Follows the crossed wall (8-connected wall tiles) up to 60 tiles from the crossing and takes the segment outside
// known tower range with the shortest detour start -> segment -> dest.
internal ee_unit *find_breach(ee_player *pl, unsigned pi, const int *start, const int *dest, const ee_unit *cross) {
    enum { MAXQ = 512 };
    static int qx[MAXQ], qy[MAXQ];
    int head = 0, tail = 0, cx = FIELD(cross, OF(UNIT, 0x1C), int), cy = FIELD(cross, OF(UNIT, 0x20), int);
    qx[tail] = cx; qy[tail++] = cy;
    ee_unit *best = NULL;
    float best_score = 1e30f;
    while (head < tail) {
        int x = qx[head], y = qy[head++];
        ee_unit *u = wall_at(pl, x, y);
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

internal void diplo_cancel_attacks(int ai, int j);   // diplo.c
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
internal int g_citizen_attacks;
internal void cancel_citizen_attack(int ai, int target);   // diplo.c
internal const char *g_cancel_why[16][16];   // why the next call-off (NULL: citizens) - diplo_safe_point logs it
internal void subgroup_count(uint8_t *sg, int *n, int *cit) {   // members of an attack subgroup, citizens among them
    uint8_t **b = FIELD(sg, OF(ASG, 0x54), uint8_t **), **e = FIELD(sg, OF(ASG, 0x58), uint8_t **);
    for (uint8_t **r = b; b && r < e && *n < 1000; r++) {
        ee_unit *u = *r ? FIELD(*r, OF(ASGU, 0x44), ee_unit *) : NULL;
        uint8_t *td = u ? FIELD(u, OF(UNIT, 0x2C), uint8_t *) : NULL;
        if (!td) continue;
        (*n)++;
        *cit += is_citizen_type(FIELD(td, OF(TD, 0x4B8), int));
    }
}
internal void citizen_attack_scan(ee_player *pl, uint32_t now) {
    static uint32_t last[16];
    unsigned i = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (g_citizen_attacks || (now >= last[i] && now - last[i] < 5000)) return;
    last[i] = now;
    uint8_t *cbm = FIELD(pl, OF(PL, 0xB8C), uint8_t *), *cmd = cbm ? FIELD(cbm, OF(AM, 0x48), uint8_t *) : NULL;
    if (!cmd) return;
    uint8_t *dq = cmd + 4;
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        uint8_t *g = deque_at(dq, first + k);
        ee_player *target = g ? FIELD(g, OF(AG, 0x48), ee_player *) : NULL;
        if (!target) continue;
        int n = 0, cit = 0;
        for (uint8_t **s = FIELD(g, OF(AG, 0x8), uint8_t **); s && s < FIELD(g, OF(AG, 0xC), uint8_t **); s++)
            if (*s) subgroup_count(*s, &n, &cit);
        if (n && cit * 2 >= n) {
            cancel_citizen_attack((int)i, FIELD(target, PL_INDEX, int) & 15);
            if (g_log) fprintf(g_log, "# attack: P%u's attack on P%d at %.1f min is %d citizens of %d - called off\n", i,
                               FIELD(target, PL_INDEX, int), now / 60000.0, cit, n);
        }
    }
}
internal void __fastcall hook_asg_route(void *self, void *edx, int *start, int *dest) {
    (void)edx;
    ee_player *pl = FIELD(self, OF(ASG, 0x190), void *);
    if (is_vanilla(pl)) pl = NULL;   // the game's own route: no weak spot, breach, call-off, nor its log
    void *target = FIELD(self, OF(ASG, 0x11C), void *);
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
        ee_unit *u = t > 0 && building_at(pl, dest, 4) ? find_weak_spot(pl, pi, start, dest) : NULL;
        if (u) {
            uint8_t *tg = self + OF(ASG, 0x100);
            if (g_log)
                fprintf(g_log, "# weak spot: P%u goal %d,%d (threat %.1f, target kind %d id %d) -> type %d of P%d at %d,%d\n",
                        pi, dest[0], dest[1], t, FIELD(tg, 0x10, int), FIELD(tg, 0xC, int),
                        FIELD(FIELD(u, OF(UNIT, 0x2C), uint8_t *), OF(TD, 0x4B8), int), FIELD(unit_owner(u), PL_INDEX, int),
                        FIELD(u, OF(UNIT, 0x1C), int), FIELD(u, OF(UNIT, 0x20), int));
            dest[0] = FIELD(u, OF(UNIT, 0x1C), int); dest[1] = FIELD(u, OF(UNIT, 0x20), int);
            FIELD(tg, 4, int) = dest[0]; FIELD(tg, 8, int) = dest[1];
            FIELD(tg, 0xC, int) = FIELD(u, OF(UNIT, 0x60), int);
            FIELD(tg, 0x10, int) = 1;
        }
    }
    if (pl && start && dest && g_breach) {
        unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
        ee_unit *cross = wall_crossing(pl, start, dest);
        ee_unit *w = cross ? find_breach(pl, pi, start, dest, cross) : NULL;
        breach_note(pl, self, w ? FIELD(w, OF(UNIT, 0x1C), int) : -1, w ? FIELD(w, OF(UNIT, 0x20), int) : -1);
        if (w) {
            uint8_t *tg = self + OF(ASG, 0x100);
            if (g_log)
                fprintf(g_log, "# breach: P%u goal %d,%d: wall crossed at %d,%d (threat %.1f) -> attack wall at %d,%d\n",
                        pi, dest[0], dest[1], FIELD(cross, OF(UNIT, 0x1C), int), FIELD(cross, OF(UNIT, 0x20), int),
                        threat_at(pi, FIELD(cross, OF(UNIT, 0x1C), int), FIELD(cross, OF(UNIT, 0x20), int)), FIELD(w, OF(UNIT, 0x1C), int), FIELD(w, OF(UNIT, 0x20), int));
            dest[0] = FIELD(w, OF(UNIT, 0x1C), int); dest[1] = FIELD(w, OF(UNIT, 0x20), int);
            FIELD(tg, 4, int) = dest[0]; FIELD(tg, 8, int) = dest[1];
            FIELD(tg, 0xC, int) = FIELD(w, OF(UNIT, 0x60), int);
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
    int hostile = pl && target && FIELD(pl, OF(PL, 0x9E4) + (FIELD(target, PL_INDEX, int) & 15) * 4, int);
    if (pl && target && !hostile) {
        if (g_log) fprintf(g_log, "# route: P%d marches on P%d, no enemy any more\n", FIELD(pl, PL_INDEX, int), FIELD(target, PL_INDEX, int));
        diplo_cancel_attacks(FIELD(pl, PL_INDEX, int) & 15, FIELD(target, PL_INDEX, int) & 15);
    }
    if (pl && hostile) persona_attack(pl, target);
    if (chat_level() && pl && start && dest) {
        unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
        int dx = dest[0] - start[0], dy = dest[1] - start[1];
        ai_say(pl, CHAT_ROUTE, "[ai] marching on P%d at %d,%d (%.0f tiles), %d enemy defences known",
               target ? FIELD(target, PL_INDEX, int) : -1, dest[0], dest[1], sqrt((double)(dx * dx + dy * dy)), g_threat[pi].n);
    }
    asg_route(self, start, dest);
}

// Leash: each unit of an attack subgroup picks its target itself (EECPAttackSubGroupUnit vf5 0x46286F -> vf4
// 0x4620DF scores every enemy the subgroup sees with vf2 0x461854, 0.0 = skip; the best one is attacked via the
// subgroup's slot 7, and while the unit has a target, rec+0x48, it counts as engaged and the march waits).
// Enemy units waiting next to their towers draw the whole army into tower range that way, however good the
// route. So for a land subgroup, an enemy inside known tower range scores 0 - not a target - unless the
// subgroup's own goal (asg+0x100 +4/+8) lies in tower range too; then the attack is meant to go in anyway.
// Unit record: +0x44 unit, +0x48 current target, +0x4C subgroup. Scorer argument: +4 the enemy unit.
internal int g_leash = 1;               // aimod.ini [all] attack_leash
internal struct { unsigned skipped, logged; } g_leash_stats[16];

internal int g_siege = 1;               // aimod.ini [all] attack_siege (siege controller below; also rams here)
internal unsigned g_ram_waits, g_air_skips;
internal int g_ignore_air = 1;          // aimod.ini [all] attack_ignore_air

// Enemy soldiers (no citizens, no siege) of player pl's enemies within r tiles of (x, y)?
internal int guarded(ee_player *pl, int x, int y, int r) {
    uint8_t *map = *(uint8_t **)GA(G_MAP);
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!pl || !grid) return 0;
    for (struct grid_it it = grid_it_box(x, y, r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        uint8_t *td = it.td;
        if (!td || FIELD(td, OF(TD, 0x4B4), int) != 5 || FIELD(u, OF(UNIT, 0x3C), int) <= 0 || !is_enemy(pl, unit_owner(u)))
            continue;
        int t = FIELD(td, OF(TD, 0x4B8), int);
        if (ut_role(t, UTR_CITIZEN | UTR_RAM | UTR_SIEGE))
            continue;
        int dx = FIELD(u, OF(UNIT, 0x1C), int) - x, dy = FIELD(u, OF(UNIT, 0x20), int) - y;
        if (dx * dx + dy * dy <= r * r) return 1;
    }
    return 0;
}

internal int mass_tether(uint8_t *sg, ee_unit *me, ee_unit *enemy);   // mass.c
typedef float (__thiscall *AsguScoreFn)(void *self, uint8_t *info);
internal float __fastcall hook_asgu_score(void *self, void *edx, uint8_t *info) {
    (void)edx;
    float s = asgu_score(self, info);
    uint8_t *sg = FIELD(self, OF(ASGU, 0x4C), uint8_t *);
    ee_unit *enemy = info ? FIELD(info, OF(ASGUINFO, 0x4), ee_unit *) : NULL;
    if (!sg || !enemy || *(uintptr_t *)sg != GA(VT_ASG_LAND) || is_vanilla(FIELD(sg, OF(ASG, 0x190), void *))) return s;
    // rams (melee, helpless against soldiers): no building while enemy soldiers guard it - they wait with the
    // army until the guards are dealt with
    ee_unit *me = FIELD(self, OF(ASGU, 0x44), ee_unit *);
    uint8_t *mtd = me ? FIELD(me, OF(UNIT, 0x2C), uint8_t *) : NULL;
    if (g_siege && mtd && is_ram(FIELD(mtd, OF(TD, 0x4B8), int)) && s != 0.0f &&
        guarded(FIELD(sg, OF(ASG, 0x190), void *), FIELD(enemy, OF(UNIT, 0x1C), int), FIELD(enemy, OF(UNIT, 0x20), int), 5)) {
        g_ram_waits++;
        return 0.0f;
    }
    // aircraft out of reach: no target. The scorer rates them like any enemy; the subgroup's members chase planes
    // circling over their own town and its march waits while any member is engaged (user's game: 25 of the player's
    // planes over P3's town kept its main force at its docks for 9+ minutes, AA tanks running after them). In reach
    // (its range + 3 tiles) they still shoot.
    uint8_t *etd = FIELD(enemy, OF(UNIT, 0x2C), uint8_t *);
    int efam = etd ? FIELD(etd, OF(TD, 0x4B4), int) : 0;
    if (g_ignore_air && s != 0.0f && me && (efam == 3 || efam == 7 || efam == 17)) {
        uint8_t *st = FIELD(me, OF(UNIT, 0x34), uint8_t *);
        float r = (st ? FIELD(st, OF(USTAT, 0x9C), float) : FIELD(mtd, OF(TD, 0x420), float)) + 3.0f;
        int dx = FIELD(enemy, OF(UNIT, 0x1C), int) - FIELD(me, OF(UNIT, 0x1C), int), dy = FIELD(enemy, OF(UNIT, 0x20), int) - FIELD(me, OF(UNIT, 0x20), int);
        if ((float)(dx * dx + dy * dy) > r * r) {
            g_air_skips++;
            return 0.0f;
        }
    }
    // breach: the scorer rates walls below zero (never chosen while anything else is around); the subgroup's
    // breach segment and its neighbours become the best targets
    int bx, by;
    if (etd && is_wall(FIELD(etd, OF(TD, 0x4B8), int)) && breach_of(sg, &bx, &by) &&
        abs(FIELD(enemy, OF(UNIT, 0x1C), int) - bx) <= 1 && abs(FIELD(enemy, OF(UNIT, 0x20), int) - by) <= 1)
        return FIELD(enemy, OF(UNIT, 0x1C), int) == bx && FIELD(enemy, OF(UNIT, 0x20), int) == by ? 1000.0f : 500.0f;
    if (s != 0.0f && mass_tether(sg, me, enemy)) return 0.0f;   // mass.c army_mass 16: not far from the pack
    if (!g_leash || s == 0.0f) return s;
    ee_player *pl = FIELD(sg, OF(ASG, 0x190), void *);
    if (!pl) return s;
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    int ex = FIELD(enemy, OF(UNIT, 0x1C), int), ey = FIELD(enemy, OF(UNIT, 0x20), int);
    float t = threat_at(pi, ex, ey);
    if (!(t > 0) || threat_at(pi, FIELD(sg, OF(ASG, 0x104), int), FIELD(sg, OF(ASG, 0x108), int)) > 0) return s;
    g_leash_stats[pi].skipped++;
    if (g_log && g_leash_stats[pi].logged < 10) {
        g_leash_stats[pi].logged++;
        ee_unit *u = FIELD(self, OF(ASGU, 0x44), ee_unit *);
        fprintf(g_log, "# leash: P%u at %.1f min: unit at %d,%d skips enemy at %d,%d (threat %.1f), goal %d,%d\n",
                pi, game_time() / 60000.0, u ? FIELD(u, OF(UNIT, 0x1C), int) : -1, u ? FIELD(u, OF(UNIT, 0x20), int) : -1,
                ex, ey, t, FIELD(sg, OF(ASG, 0x104), int), FIELD(sg, OF(ASG, 0x108), int));
        fflush(g_log);
    }
    return 0.0f;
}

// Waypoint i of the route deque at asg+0x60 (std::deque of {x, y, ?}, 0x155 per block: +4 map, +8 map size,
// +0xC first index, +0x10 count).
internal int *route_waypoint(uint8_t *asg, unsigned i) {
    uint8_t *dq = asg + OF(ASG, 0x60);
    unsigned idx = FIELD(dq, 0xC, unsigned) + i, blk = idx / 0x155, msz = FIELD(dq, 8, unsigned);
    uint8_t **map = FIELD(dq, 4, uint8_t **);
    if (!map || !msz) return NULL;
    if (blk >= msz) blk -= msz;
    return map[blk] ? (int *)(map[blk] + (idx % 0x155) * 12) : NULL;
}

typedef void (__thiscall *AsgFn)(void *self);
internal void __fastcall hook_asg_route_done(void *self, void *edx) {
    (void)edx;
    ee_player *pl = FIELD(self, OF(ASG, 0x190), void *);
    if (g_log && pl) {
        unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15, n = FIELD(self, OF(ASG, 0x70), unsigned), hot = 0;
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
    asg_route_done(self);
}

// Soldiers (not buildings, walls, citizens) of a player, from the unit grid.
internal int army_size(ee_player *pl) {
    uint8_t *map = *(uint8_t **)GA(G_MAP);
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return -1;
    int n = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        uint8_t *td = it.td;
        if (!td || unit_owner(u) != pl) continue;
        // units only sit in the cell of their position
        if (is_army_family(FIELD(td, OF(TD, 0x4B4), int))) n++;
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
internal struct {
    int tx, ty, ttype, px, py; uint32_t last; unsigned logged;
    int wave;                    // a wave is out: newcomers wait at the gathering point
    void *prev[24]; int nprev;   // the siege weapons at the last look (losses: those gone)
    uint32_t loss_t[3];          // the last 3 losses; all within 10 min: training paused
    uint32_t pause_until, seen_t;
    unsigned waves, lost, pauses, held, logged_min;
} g_siege_st[16];


internal ee_unit *unit_at(int x, int y, int type) {
    if (x < 0 || y < 0) return NULL;
    for (struct grid_it it = grid_it_box(x, y, 0, 1); grid_it_next(&it); ) {   // the one cell of x, y
        ee_unit *u = it.u;
        uint8_t *td = it.td;
        if (td && FIELD(u, OF(UNIT, 0x1C), int) == x && FIELD(u, OF(UNIT, 0x20), int) == y && FIELD(td, OF(TD, 0x4B8), int) == type && FIELD(u, OF(UNIT, 0x3C), int) > 0)
            return u;
    }
    return NULL;
}

// 0 = refused (dead, off the map, or +0x33B): the goal is deleted then (its vf0, as the game's callers do) - the
// caller must not touch it any more (a load goal: no parts from it)
internal int unit_goal(ee_unit *u, void *goal, int append, int prio) {
    if (!goal) return 0;
    if (FIELD(u, OF(UNIT, 0x3C), int) <= 0 || FIELD(u, OF(UNIT, 0x1C), int) == -10000 || FIELD(u, OF(UNIT, 0x33B), char)) {
        ((void (__thiscall *)(void *, int))(*(void ***)goal)[0])(goal, 1);
        return 0;
    }
    unit_set_goal(u, goal, append, 0);
    unit_say(u, prio);
    uint8_t *td = FIELD(u, OF(UNIT, 0x2C), uint8_t *);
    type_goal_assigned(td, u);
    return 1;
}

// Art of Conquest's constructor takes a fourth argument, a byte it keeps at the goal's new +0x40: 0 as the AI's own
// calls pass it
internal void *goal_attack(ee_unit *u, ee_unit *target) {
    void *g = game_new(GA(SIZE_GOAL_ATTACK));
    if (!g) return NULL;
    return goal_attack_init(g, u, target);
}

// a transport's (or a fortress's) load goal (EEUGLoadTransport, engine-notes.md: new(0x68), 0x61487F with the units'
// vector - sorted in place) given to holder; NULL when it could not be made or holder refused it (deleted by
// unit_goal: no parts from it). Each unit to walk in then gets goal_load_part(g) - its vf4 product.
internal void *goal_load(ee_unit *holder, ee_unit **units, int n) {
    uint8_t *vec[4] = { 0, (uint8_t *)units, (uint8_t *)(units + n), (uint8_t *)(units + n) };
    void *g = game_new(GA(SIZE_GOAL_LOAD));
    if (g) g = goal_load_init(g, holder, vec);
    return g && unit_goal(holder, g, 0, 0x1F40) ? g : NULL;
}
internal void *goal_load_part(void *g) { return goal_part(g); }
// an unload goal at x, y (EEUGUnloadTransport: new(0x40), 0x615A1F; flag 1 as the AI passes it), not given yet
internal void *goal_unload(ee_unit *holder, int x, int y, int flag) {
    void *g = game_new(GA(SIZE_GOAL_UNLOAD));
    return g ? goal_unload_init(g, holder, x, y, flag) : NULL;
}

internal void *goal_move(float x, float y) {
    float xy[2] = { x, y };
    void *g = game_new(GA(SIZE_GOAL_MOVE));
    return g ? goal_move_init(g, xy) : NULL;
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
internal int g_avoid_citizens = 1;
internal int nearest_drop(ee_player *pl, int x, int y, int food, int out[2]);   // sites.c
internal int tower_covers(unsigned pi, float x, float y) {
    for (int k = 0; k < g_threat[pi].n; k++) {
        struct defence *d = &g_threat[pi].d[k];
        float dx = x - (float)d->x, dy = y - (float)d->y, r = d->range + 2.0f;
        if (dx * dx + dy * dy <= r * r) return 1;
    }
    return 0;
}
internal int route_covered(ee_player *pl, int fx, int fy, int tx, int ty) {   // fx = -10000: the site only
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
internal int route_danger(ee_player *pl, int fx, int fy, int tx, int ty) {
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
internal int g_cit_threats = 1;   // [all] citizen_threats (below, DAMAGE MEMORY)
internal int raiders_near(ee_player *pl, int x, int y, int r0) {
    int rmax = g_cit_threats ? RAID_FAR : RAID_MAX;
    int r = r0 > rmax ? r0 : rmax;   // the search box; each unit then by its own reach
    uint8_t *map = *(uint8_t **)GA(G_MAP);
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid || !pl) return 0;
    for (struct grid_it it = grid_it_box(x, y, r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        uint8_t *td = it.td;
        if (!td || FIELD(u, OF(UNIT, 0x3C), int) <= 0 || abs(FIELD(u, OF(UNIT, 0x1C), int) - x) > r || abs(FIELD(u, OF(UNIT, 0x20), int) - y) > r) continue;
        int fam = FIELD(td, OF(TD, 0x4B4), int), t = FIELD(td, OF(TD, 0x4B8), int);
        if ((fam != 5 && fam != 4 && !(g_cit_threats && fam == 10)) || is_citizen_type(t) || FIELD(td, OF(TD, 0x510), int) <= 0) continue;
        uint8_t *st = FIELD(u, OF(UNIT, 0x34), uint8_t *);
        float rng = st ? FIELD(st, OF(USTAT, 0x9C), float) : FIELD(td, OF(TD, 0x420), float);
        int reach = rng > 0.0f && rng < 30.0f ? (int)(rng + 0.5f) + RAID_MARGIN : 0;
        if (reach > rmax) reach = rmax;
        if (reach < r0) reach = r0;
        if (abs(FIELD(u, OF(UNIT, 0x1C), int) - x) > reach || abs(FIELD(u, OF(UNIT, 0x20), int) - y) > reach) continue;
        ee_player *o = unit_owner(u);   // not nature (player 0: wild animals)
        if (o && FIELD(o, PL_INDEX, int) > 0 && is_enemy(pl, o) && seen_by(u, pl)) return 1;
    }
    return 0;
}
// enemy soldiers on the way from x0,y0 to x1,y1 (every 4 tiles from the 5th, within 4) or within 6 of the end
internal int path_raided(ee_player *pl, int x0, int y0, int x1, int y1) {
    float dx = (float)(x1 - x0), dy = (float)(y1 - y0), l = sqrtf(dx * dx + dy * dy);
    for (float t = 5; t < l; t += 4)
        if (raiders_near(pl, x0 + (int)(dx / l * t), y0 + (int)(dy / l * t), 4)) return 1;
    return raiders_near(pl, x1, y1, 6);
}
// where a citizen at x,y flees while the base is overrun: the rally point if the way is clear, else 12 tiles in the
// clear direction (of 8) nearest the rally point; none clear: the rally point after all
internal void flee_point(ee_player *pl, int x, int y, float rx, float ry, float *fx, float *fy) {
    *fx = rx; *fy = ry;
    if (!path_raided(pl, x, y, (int)rx, (int)ry)) return;
    float msz = (float)*(uint32_t *)(*(uint8_t **)GA(G_MAP) + OF(MAP, 0x1816FC)), best = 1e9f;
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
// Hunters' losses don't count (the animals). scenarios/shelling.eeai, bombing.eeai (10 seeds, P3 off vs on): shelling
// alive 5 vs 7 of 10, workers 7.1 vs 17.6, losses 14.3 vs 10.8, buildings lost 6.1 vs 3.9; bombing workers 42.9 vs
// 54.7, losses 25.5 vs 21.0. Idle citizens rose (bombing 2.9 vs 6.3 on average): no safe site of their resource left.
#define HURT_N 64
internal struct { unsigned at; struct { int x, y; uint32_t t; } h[HURT_N]; } g_hurt[16];
internal struct { ee_unit *u; int hp; } g_cit_hp[16][512];
internal unsigned g_hurt_count[16];
internal void hurt_note(unsigned pi, ee_unit *u, uint32_t now, int mark) {   // remembers u's hit points; a loss marks its spot
    int hp = FIELD(u, OF(UNIT, 0x3C), int);
    unsigned h = (unsigned)((uintptr_t)u >> 4) & 511;
    for (int k = 0; k < 512; k++) {
        typeof(&g_cit_hp[0][0]) e = &g_cit_hp[pi][(h + k) & 511];
        if (e->u == u) {
            if (hp < e->hp && mark) {
                typeof(&g_hurt[0].h[0]) r = &g_hurt[pi].h[g_hurt[pi].at++ % HURT_N];
                r->x = FIELD(u, OF(UNIT, 0x1C), int); r->y = FIELD(u, OF(UNIT, 0x20), int); r->t = now ? now : 1;
                g_hurt_count[pi]++;
            }
            e->hp = hp;
            return;
        }
        if (!e->u) { e->u = u; e->hp = hp; return; }
    }
    memset(g_cit_hp[pi], 0, sizeof g_cit_hp[pi]);   // full (dead citizens' entries): start over
}
internal int hurt_near(unsigned pi, int x, int y) {   // citizens hurt within 6 tiles in the last 2 minutes
    uint32_t now = game_time();
    int n = 0;
    for (int k = 0; k < HURT_N; k++) {
        typeof(&g_hurt[0].h[0]) r = &g_hurt[pi].h[k];
        if (r->t && now >= r->t && now - r->t <= 120000 && abs(r->x - x) <= 6 && abs(r->y - y) <= 6) n++;
    }
    return n;
}
internal int hurt_danger(ee_player *pl, int x, int y, int res) {   // res: 2 food, 3 wood, 4 stone, 5 gold, 6 iron, -1 unknown
    if (!g_cit_threats || !pl) return 0;
    return hurt_near((unsigned)FIELD(pl, PL_INDEX, int) & 15, x, y) >= (res >= 4 ? 3 : 1);
}
internal int site_danger_res(ee_player *pl, ee_unit *site, int food, int res) {   // from the nearest drop site
    if (!g_avoid_citizens || !pl || !site) return 0;
    int x = FIELD(site, OF(UNIT, 0x1C), int), y = FIELD(site, OF(UNIT, 0x20), int), d[2];
    if (raiders_near(pl, x, y, 6)) return 1;   // the searches refuse a raided site too (else: sent back, sent again)
    if (hurt_danger(pl, x, y, res)) return 1;
    if (!nearest_drop(pl, x, y, food, d)) d[0] = -10000;
    return route_danger(pl, d[0], d[1], x, y);
}
internal int site_danger(ee_player *pl, ee_unit *site, int food) { return site_danger_res(pl, site, food, food ? 2 : -1); }
internal void *goal_move(float x, float y);
internal int unit_goal(ee_unit *u, void *goal, int append, int prio);
internal int muster_rally(unsigned idx, float *x, float *y);   // defence.c: the rally point while the base is overrun
internal struct { unsigned sent; } g_cit_stats[16];
internal unsigned g_repair_refused[16];   // repair orders refused at buildings under attack (hook_repair_goal)
// STRAY CITIZENS: an attack that ends hands its units back to the resource manager's list 0 (not assigned) with the
// attack's last order still on them - march to the target's Town Center. The assignment only gives work to idle
// citizens, so they marched on and died there: all 34 of Hedda's, first into the player's town, then into Livia's
// (her economy dead at minute 46, the stock frozen). A citizen of list 0 walking to a point within 6 tiles of an
// enemy building and over 15 tiles from its own drop sites is sent home (not to the muster's rally point).
internal int building_at(ee_player *pl, const int *dest, int r);
internal unsigned g_noroom[16][6];   // econ.c
internal int g_bunkers;   // bunker.c: citizens under fire into a fortress
internal void bk_refresh(ee_player *pl, unsigned pi);
internal int bk_shelter(ee_player *pl, ee_unit *u);
internal void bk_flush(ee_player *pl, unsigned pi, uint32_t now);
internal void bk_build(ee_player *pl, unsigned pi, uint32_t now);
internal void bk_reserve_tick(ee_player *pl, unsigned pi, uint32_t now);
internal int g_bk_res;
internal void citizens_safety(ee_resource_mgr *rm) {   // before the citizen assignment
    ee_player *pl = rm_player(rm);
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
    if (g_bunkers || g_bk_res) bk_refresh(pl, idx);
    for (int r = 0; r < 7; r++) {
        uint8_t *list = FIELD_AT(rm, RM_LISTS + r * 0x14);
        for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), cnt = FIELD(list, 0x10, unsigned); k < cnt; k++) {
            ee_unit *u = deque_at(list, first + k);
            uint8_t *goal = u ? FIELD(u, OF(UNIT, 0x1EC), uint8_t *) : NULL;
            if (!u || FIELD(u, OF(UNIT, 0x3C), int) <= 0) continue;
            {
                const char *gd = goal ? rtti_name(goal) : "";
                doing[r][!goal ? 3 : !strcmp(gd, "EEUGGather@@") ? 0 : !strcmp(gd, "EEUGBuildRepair@@") ? 1 :
                         *(uintptr_t *)goal == GA(VT_GOAL_LOCATION) ? 2 : 3]++;
            }
            if (g_cit_threats) {   // not hunters: the animals hurt them (a farm is a building, family 2)
                ee_unit *gs = goal && !strcmp(rtti_name(goal), "EEUGGather@@") ? FIELD(goal, OF(GOAL, 0x38), ee_unit *) : NULL;
                hurt_note(idx, u, now, !(r == 2 && gs && FIELD(FIELD(gs, OF(UNIT, 0x2C), uint8_t *), OF(TD, 0x4B4), int) != 2));
            }
            if (r == 0 && goal && *(uintptr_t *)goal == GA(VT_GOAL_LOCATION)) {   // a stray: an attack's leftover order
                float gx = FIELD(goal, OF(GOAL, 0x20), float), gy = FIELD(goal, OF(GOAL, 0x24), float);
                int dest[2] = { (int)gx, (int)gy }, d[2];
                int rally = mus && fabsf(gx - rx) < 2 && fabsf(gy - ry) < 2;
                if (!rally && building_at(pl, dest, 6) &&
                    (!nearest_drop(pl, dest[0], dest[1], 0, d) || (d[0] - dest[0]) * (d[0] - dest[0]) + (d[1] - dest[1]) * (d[1] - dest[1]) > 225) &&
                    nearest_drop(pl, FIELD(u, OF(UNIT, 0x1C), int), FIELD(u, OF(UNIT, 0x20), int), 0, d) &&
                    unit_goal(u, goal_move((float)d[0] + 0.5f, (float)d[1] + 0.5f), 0, 0x1F40)) {
                    stray++;
                    continue;
                }
            }
            if (mus) {   // the base is overrun: citizens near the enemy (or working near it) flee to the rally point
                int ux = FIELD(u, OF(UNIT, 0x1C), int), uy = FIELD(u, OF(UNIT, 0x20), int);
                float dx = (float)ux - rx, dy = (float)uy - ry;
                if (dx * dx + dy * dy <= 36) continue;   // there already
                const char *gn0 = goal ? rtti_name(goal) : "";
                ee_unit *st = !goal ? NULL : !strcmp(gn0, "EEUGBuildRepair@@") ? FIELD(goal, OF(GOAL, 0x3C), ee_unit *) :
                              !strcmp(gn0, "EEUGGather@@") ? FIELD(goal, OF(GOAL, 0x38), ee_unit *) : NULL;
                int walk = goal && *(uintptr_t *)goal == GA(VT_GOAL_LOCATION), danger;
                if (walk)   // on its way somewhere: only if enemy soldiers stand on the way or at the end
                    danger = path_raided(pl, ux, uy, (int)FIELD(goal, OF(GOAL, 0x20), float), (int)FIELD(goal, OF(GOAL, 0x24), float));
                else
                    danger = raiders_near(pl, ux, uy, 8) || (st && raiders_near(pl, FIELD(st, OF(UNIT, 0x1C), int), FIELD(st, OF(UNIT, 0x20), int), 8));
                if (danger && g_bunkers && bk_shelter(pl, u)) { sheltered++; continue; }
                if (danger) {
                    float fx, fy;
                    flee_point(pl, ux, uy, rx, ry, &fx, &fy);
                    if (walk && fabsf(FIELD(goal, OF(GOAL, 0x20), float) - fx) < 2 && fabsf(FIELD(goal, OF(GOAL, 0x24), float) - fy) < 2) continue;
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
            ee_unit *site = FIELD(goal, build ? OF(GOAL, 0x3C) : OF(GOAL, 0x38), ee_unit *);
            int d[2];
            if (!site || !nearest_drop(pl, FIELD(u, OF(UNIT, 0x1C), int), FIELD(u, OF(UNIT, 0x20), int), r == 2, d)) continue;
            int raided = raiders_near(pl, FIELD(site, OF(UNIT, 0x1C), int), FIELD(site, OF(UNIT, 0x20), int), build ? 8 : 6);
            // builders near known towers only: only of fields (walls and towers at the front: the wall manager sends
            // them again and again)
            if (build && !raided && FIELD(FIELD(site, OF(UNIT, 0x2C), uint8_t *), OF(TD, 0x4B8), int) != UT(B_FARM)) continue;
            if (!build && !raided && hurt_danger(pl, FIELD(site, OF(UNIT, 0x1C), int), FIELD(site, OF(UNIT, 0x20), int), r)) raided = 1;   // hurt there
            if (!raided && (build || !site_danger_res(pl, site, r == 2, r))) continue;
            if (g_bunkers && bk_shelter(pl, u)) { sheltered++; continue; }   // under fire: into a fortress
            if (raided && abs(d[0] - FIELD(site, OF(UNIT, 0x1C), int)) <= 3 && abs(d[1] - FIELD(site, OF(UNIT, 0x20), int)) <= 3) continue;   // at home already
            if (!unit_goal(u, goal_move((float)d[0] + 0.5f, (float)d[1] + 0.5f), 0, 0x1F40)) continue;
            sent++;
            int st = FIELD(FIELD(site, OF(UNIT, 0x2C), uint8_t *), OF(TD, 0x4B8), int), j = 0;
            while (j < 4 && kinds[j][1] && kinds[j][0] != st) j++;
            if (j < 4) { kinds[j][0] = st; kinds[j][1]++; }
        }
    }
    g_cit_stats[idx].sent += sent + fled;
    if (g_bunkers) bk_flush(pl, idx, now);
    if (g_bunkers || g_bk_res) { bk_build(pl, idx, now); bk_reserve_tick(pl, idx, now); }
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
        char other[64] = "";
        for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
            ee_unit *u = it.u;
            uint8_t *td = it.td;
            if (unit_owner(u) != pl || FIELD(u, OF(UNIT, 0x3C), int) <= 0 || !is_citizen_type(FIELD(td, OF(TD, 0x4B8), int))) continue;
            uint8_t *g = FIELD(u, OF(UNIT, 0x1EC), uint8_t *);
            const char *gd = g ? rtti_name(g) : "";
            int k = !g ? 5 : !strcmp(gd, "EEUGGather@@") ? 0 : !strcmp(gd, "EEUGBuildRepair@@") ? 1 :
                    *(uintptr_t *)g == GA(VT_GOAL_LOCATION) ? 2 : *(uintptr_t *)g == GA(VT_GOAL_ATTACK) ? 3 : 4;
            all[k]++;
            if (k == 4 && !other[0]) lstrcpynA(other, gd, sizeof other);
        }
        {   // the population as the buildings' start check sees it (0x624DF8: cap vf31 < population (1, 0) + the unit -> wait)
            void *cmp = FIELD(pl, OF(PL, 0xB90), void *), *ump = cmp ? FIELD(cmp, OF(ASGU, 0x44), void *) : NULL;
            uint8_t *v0 = FIELD(pl, OF(PL, 0x18), uint8_t *), *v1 = FIELD(pl, OF(PL, 0x1C), uint8_t *);
            fprintf(g_log, "# popcheck: P%u at %.1f min: cap %d, population %d (with production %d, list +0x18 %d), +0xB10 %d, +0xB1C %d +0xB20 %d, limit %d, AI's in production %d; ordered with no room by call site 46cce6/490cac/490e12/490f1f/490fa9/mod: %u/%u/%u/%u/%u/%u\n",
                    idx, now / 60000.0, unit_limit(pl),
                    population(pl, 0, 0), population(pl, 1, 0),
                    v0 && v1 ? (int)((v1 - v0) / 4) : -1, FIELD(pl, OF(PL, 0xB10), int), FIELD(pl, OF(PL, 0xB1C), int), FIELD(pl, OF(PL, 0xB20), int), *(int *)GA(G_POP_LIMIT),
                    ump ? FIELD(ump, OF(UM, 0x13C), int) : -1, g_noroom[idx][0], g_noroom[idx][1], g_noroom[idx][2], g_noroom[idx][3], g_noroom[idx][4], g_noroom[idx][5]);
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
internal void __fastcall hook_repair_goal(ee_unit *u, void *edx, void *goal, int append, int prio) {
    (void)edx;
    ee_player *pl = u ? unit_owner(u) : NULL;
    ee_unit *b = goal && *(uintptr_t *)goal == GA(VT_GOAL_BUILD) ? FIELD(goal, OF(GOAL, 0x3C), ee_unit *) : NULL;
    if (g_avoid_citizens && pl && b && !is_vanilla(pl) && raiders_near(pl, FIELD(b, OF(UNIT, 0x1C), int), FIELD(b, OF(UNIT, 0x20), int), 8)) {
        g_repair_refused[(unsigned)FIELD(pl, PL_INDEX, int) & 15]++;
        ((void (__thiscall *)(void *, int))(*(void ***)goal)[0])(goal, 1);
        return;
    }
    unit_set_goal(u, goal, append, prio);
}

// Known defences covering tile (x, y) by their real range (+1), not counting the one at (ex, ey).
internal int covered(unsigned pi, float x, float y, int ex, int ey) {
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
internal const struct { enum ut_name type; int epoch; } k_siege_types[] = {
    { UT_GUN_SIEGE13_PALADIN_CANNON, 13 }, { UT_GUN_SIEGE10_HOWITZER_CANNON, 10 }, { UT_GUN_SIEGE09_SERPENTINE_CANNON, 9 },
    { UT_GUN_SIEGE07_BASILISK_CANNON, 7 }, { UT_GUN_SIEGE06_TREBUCHET, 6 }, { UT_GUN_SIEGE04_CATAPULT, 4 } };
internal struct { uint32_t last_try, factory_req; unsigned trained, failed, no_factory; } g_siege_train[16];

// Entry i of the build manager's queue for epoch e (bm +4 + e*0x14: deque of {type, x, y}, 0x155 per block), NULL
// past the end. 14 queues (epochs 0..13); the placement area follows at +0x11C.
internal int *bq_entry(ee_build_mgr *bm, int e, unsigned i) {
    uint8_t *dq = FIELD_AT(bm, OF(BM, 0x4) + e * 0x14);
    unsigned first = FIELD(dq, 0xC, unsigned), n = FIELD(dq, 0x10, unsigned), msz = FIELD(dq, 8, unsigned);
    uint8_t **map = FIELD(dq, 4, uint8_t **);
    if (e < 0 || e >= 14 || i >= n || !map || !msz || first >= msz * 0x155 || n > msz * 0x155) return NULL;
    unsigned idx = first + i, blk = idx / 0x155;
    if (blk >= msz) blk -= msz;
    return map[blk] ? (int *)(map[blk] + (idx % 0x155) * 12) : NULL;
}
internal int bq_has(ee_build_mgr *bm, int type) {
    for (int e = 0; e < 14; e++)
        for (unsigned i = 0; ; i++) {
            int *ent = bq_entry(bm, e, i);
            if (!ent) break;
            if (ent[0] == type) return 1;
        }
    return 0;
}

internal int siege_want(ee_player *pl, unsigned pi) {   // 2 + a quarter of the known enemy defences, at most 4; a breach: 2
    int want = g_threat[pi].n ? 2 + g_threat[pi].n / 4 : 0;
    for (int k = 0; k < 64 && !want; k++)
        if (g_breaches[k].sg && g_breaches[k].pl == pl) want = 2;
    return want > 4 ? 4 : want;
}
internal void siege_train(ee_player *pl, unsigned pi, int have, uint32_t now) {
    int want = siege_want(pl, pi);
    if (have >= want || (now >= g_siege_train[pi].last_try && now - g_siege_train[pi].last_try < 30000)) return;
    if (now < g_siege_st[pi].pause_until && g_siege_st[pi].pause_until - now <= 600000) return;   // lost too many
    g_siege_train[pi].last_try = now;
    // no siege weapon in this epoch yet (the catapult comes with the Bronze Age): no factory either - the build
    // manager can't place it before and would keep the request in its queue for good
    if (current_epoch(pl) + 1 < k_siege_types[sizeof k_siege_types / sizeof *k_siege_types - 1].epoch) return;
    // a Siege Factory of our own
    int factory = 0;
    for (struct grid_it it = grid_it_all(1); !factory && grid_it_next(&it); ) {
        ee_unit *u = it.u;
        uint8_t *td = it.td;
        if (td && unit_owner(u) == pl && FIELD(td, OF(TD, 0x4B8), int) == UT(B_SIEGE_FACTORY)) { factory = 1; break; }
    }
    if (!factory) {
        // the build manager (cm +0x3C) works through per-epoch queues (+4 + epoch*0x14, 12-byte entries {type, x, y};
        // 0,0 = it picks the place) every 3 s (0x468540); 0x468C37 appends an entry. Queue a Siege Factory.
        g_siege_train[pi].no_factory++;
        void *cm = FIELD(pl, OF(PL, 0xB90), void *);
        ee_build_mgr *bm = cm ? FIELD(cm, OF(CM, 0x3C), ee_build_mgr *) : NULL;
        int e = current_epoch(pl);
        if (bm && e >= 0 && e < 14 && ut_has(UT(B_SIEGE_FACTORY)) && !bq_has(bm, UT(B_SIEGE_FACTORY)) &&
            (!g_siege_train[pi].factory_req || now - g_siege_train[pi].factory_req >= 300000)) {
            g_siege_train[pi].factory_req = now ? now : 1;
            int entry[3] = { UT(B_SIEGE_FACTORY), 0, 0 };
            bm_queue_add(bm, e, entry);
            if (g_log) fprintf(g_log, "# siege: P%u at %.1f min queues a Siege Factory (wants %d siege, has %d)\n", pi, now / 60000.0, want, have);
            ai_say(pl, CHAT_SIEGE, "[ai] need siege (%d of %d): queued a Siege Factory", have, want);
        }
        return;
    }
    void *cm = FIELD(pl, OF(PL, 0xB90), void *), *um = cm ? FIELD(cm, OF(CM, 0x44), void *) : NULL;
    if (!um) return;
    int epoch = current_epoch(pl) + 1;   // tech tree epoch 0 = Prehistoric; tech ids count from 1
    for (size_t k = 0; k < sizeof k_siege_types / sizeof *k_siege_types; k++) {
        if (k_siege_types[k].epoch > epoch || !ut_has(g_ut[k_siege_types[k].type])) continue;
        int req[4] = { g_ut[k_siege_types[k].type], want - have, 0, 0 };
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

internal void siege_tick(ee_player *pl, uint32_t now) {
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!g_siege || (now >= g_siege_st[pi].last && now - g_siege_st[pi].last < 2000)) return;
    g_siege_st[pi].last = now;
    uint8_t *map = *(uint8_t **)GA(G_MAP);
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return;
    // own siege weapons
    enum { MAXS = 24, MAXE = 48 };
    ee_unit *siege[MAXS]; int ns = 0; float sx = 0, sy = 0, srange = 99;
    for (struct grid_it it = grid_it_all(1); ns < MAXS && grid_it_next(&it); ) {
        ee_unit *u = it.u;
        uint8_t *td = it.td;
        if (!td || unit_owner(u) != pl || !is_siege(FIELD(td, OF(TD, 0x4B8), int)) || FIELD(u, OF(UNIT, 0x3C), int) <= 0) continue;
        uint8_t *st = FIELD(u, OF(UNIT, 0x34), uint8_t *);
        float r = st ? FIELD(st, OF(USTAT, 0x9C), float) : FIELD(td, OF(TD, 0x420), float);
        if (r < srange) srange = r;
        siege[ns++] = u; sx += (float)FIELD(u, OF(UNIT, 0x1C), int); sy += (float)FIELD(u, OF(UNIT, 0x20), int);
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
    ee_unit *t = NULL; float best = 1e30f;
    for (int k = 0; k < 64; k++) {
        if (!g_breaches[k].sg || g_breaches[k].pl != pl) continue;
        ee_unit *w = wall_at(pl, g_breaches[k].x, g_breaches[k].y);
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
    int tx = FIELD(t, OF(UNIT, 0x1C), int), ty = FIELD(t, OF(UNIT, 0x20), int);
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
    g_siege_st[pi].tx = tx; g_siege_st[pi].ty = ty; g_siege_st[pi].ttype = FIELD(FIELD(t, OF(UNIT, 0x2C), uint8_t *), OF(TD, 0x4B8), int);
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
    ee_unit *out[MAXS];
    ee_unit *home[MAXS];
    ee_unit *away[MAXS]; int nout = 0, nhome = 0, naway = 0;
    for (int i = 0; i < ns; i++) {
        ee_unit *u = siege[i];
        uint8_t *g = FIELD(u, OF(UNIT, 0x1EC), uint8_t *);
        float ux = (float)FIELD(u, OF(UNIT, 0x1C), int) + 0.5f - px, uy = (float)FIELD(u, OF(UNIT, 0x20), int) + 0.5f - py;
        float hx = (float)FIELD(u, OF(UNIT, 0x1C), int) + 0.5f - gx, hy = (float)FIELD(u, OF(UNIT, 0x20), int) + 0.5f - gy;
        int committed = (g && *(uintptr_t *)g == GA(VT_GOAL_ATTACK) && FIELD(g, OF(GOAL, 0x30), ee_unit *) == t) || ux * ux + uy * uy <= 9.0f ||
                        (g && *(uintptr_t *)g == GA(VT_GOAL_LOCATION) && fabsf(FIELD(g, OF(GOAL, 0x20), float) - px) < 2 && fabsf(FIELD(g, OF(GOAL, 0x24), float) - py) < 2);
        if (committed && !changed) out[nout++] = u;
        else if (hx * hx + hy * hy <= 25.0f) home[nhome++] = u;
        else away[naway++] = u;
    }
    int launched = 0, need = siege_want(pl, pi);
    if (need > 3) need = 3;
    if (need < 2) need = 2;
    // the wave out: keep it at it (attack once there)
    for (int i = 0; i < nout; i++) {
        ee_unit *u = out[i];
        uint8_t *g = FIELD(u, OF(UNIT, 0x1EC), uint8_t *);
        if (g && *(uintptr_t *)g == GA(VT_GOAL_ATTACK) && FIELD(g, OF(GOAL, 0x30), ee_unit *) == t) continue;
        float ux = (float)FIELD(u, OF(UNIT, 0x1C), int) + 0.5f - px, uy = (float)FIELD(u, OF(UNIT, 0x20), int) + 0.5f - py;
        if (ux * ux + uy * uy <= 4.0f) unit_goal(u, goal_attack(u, t), 0, 0x1F40);
    }
    g_siege_st[pi].wave = nout > 0;
    // the rest to the gathering point (not those walking there)
    for (int i = 0; i < naway; i++) {
        ee_unit *u = away[i];
        uint8_t *g = FIELD(u, OF(UNIT, 0x1EC), uint8_t *);
        if (g && *(uintptr_t *)g == GA(VT_GOAL_LOCATION) && fabsf(FIELD(g, OF(GOAL, 0x20), float) - gx) < 2 && fabsf(FIELD(g, OF(GOAL, 0x24), float) - gy) < 2) continue;
        unit_goal(u, goal_move(gx, gy), 0, 0x1F40);
    }
    const char *hold = NULL;
    if (g_siege_st[pi].wave) hold = "a wave is out";
    else if (nhome < need) hold = "gathering";
    else if (raiders_near(pl, (int)px, (int)py, 8) || path_raided(pl, (int)gx, (int)gy, (int)px, (int)py)) hold = "enemy soldiers on the way";
    // escorts: idle soldiers (no goal) along with a wave
    int want = nhome * 3 / 2 < 4 ? 4 : nhome * 3 / 2, escorts = 0, idle = 0;
    for (struct grid_it it = grid_it_all(1); idle < 2 && grid_it_next(&it); ) {
        ee_unit *u = it.u;
        uint8_t *td = it.td;
        if (!td || unit_owner(u) != pl || FIELD(u, OF(UNIT, 0x1EC), void *) || FIELD(u, OF(UNIT, 0x3C), int) <= 0) continue;
        int fam = FIELD(td, OF(TD, 0x4B4), int);
        idle += fam == 5 && is_soldier(td);   // land soldiers (is_soldier: no citizens, siege, rams - the ram 158 was missed)
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
    g_siege_st[pi].waves++;
    // escorts: idle soldiers (no goal) to the firing position (want <= 36 with nhome <= MAXS: MAXE never cuts it short -
    // the hand-written loop checked MAXE only per cell)
    for (struct grid_it it = grid_it_all(1); escorts < want && escorts < MAXE && grid_it_next(&it); ) {
        ee_unit *u = it.u;
        uint8_t *td = it.td;
        if (unit_owner(u) != pl || FIELD(u, OF(UNIT, 0x1EC), void *) || FIELD(u, OF(UNIT, 0x3C), int) <= 0) continue;
        int fam = FIELD(td, OF(TD, 0x4B4), int);
        if (fam != 5 || !is_soldier(td)) continue;   // land soldiers only (no citizens, siege, rams - 158 too)
        float ex = (float)FIELD(u, OF(UNIT, 0x1C), int) - px, ey = (float)FIELD(u, OF(UNIT, 0x20), int) - py;
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

internal void charge_tick(uint32_t now);
// asgtrace P [off] (test): log every state change (+0x1D0) of AI P's attack subgroups - class, state, mission, goal,
// members alive and their centre, waypoints, engaged members, what the members do (march/fight/other/idle)
internal unsigned g_trace_mask;
internal struct { uint8_t *sg; int state, goal_x, goal_y; } g_trace[64];
internal void asg_trace_tick(ee_player *pl, uint32_t now) {
    unsigned p = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!(g_trace_mask & (1u << p)) || !g_log) return;
    uint8_t *cbm = FIELD(pl, OF(PL, 0xB8C), uint8_t *), *cmd = cbm ? FIELD(cbm, OF(AM, 0x48), uint8_t *) : NULL;
    if (!cmd) return;
    uint8_t *dq = cmd + 4;
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    for (unsigned k = 0; k < cnt && k < 32; k++) {
        uint8_t *g = deque_at(dq, first + k);
        for (uint8_t **sp = g ? FIELD(g, OF(AG, 0x8), uint8_t **) : NULL; sp && sp < FIELD(g, OF(AG, 0xC), uint8_t **); sp++) {
            uint8_t *sg = *sp;
            if (!sg) continue;
            int st = FIELD(sg, OF(ASG, 0x1D0), int), gx = FIELD(sg, OF(ASG, 0x104), int), gy = FIELD(sg, OF(ASG, 0x108), int), slot = -1, fre = -1;
            for (int q = 0; q < 64; q++) {
                if (g_trace[q].sg == sg) { slot = q; break; }
                if (!g_trace[q].sg && fre < 0) fre = q;
            }
            if (slot >= 0 && g_trace[slot].state == st && g_trace[slot].goal_x == gx && g_trace[slot].goal_y == gy) continue;
            if (slot < 0) { if (fre < 0) continue; slot = fre; g_trace[slot].sg = sg; g_trace[slot].state = -1; }
            int n = 0, walk = 0, fight = 0, other = 0, none = 0, engaged = 0;
            float cx = 0, cy = 0;
            for (uint8_t **r = FIELD(sg, OF(ASG, 0x54), uint8_t **); r && r < FIELD(sg, OF(ASG, 0x58), uint8_t **) && n < 1000; r++) {
                ee_unit *u = *r ? FIELD(*r, OF(ASGU, 0x44), ee_unit *) : NULL;
                uint8_t *gl;
                if (!u || FIELD(u, OF(UNIT, 0x3C), int) <= 0) continue;
                if (FIELD(*r, OF(ASGU, 0x48), void *)) engaged++;
                n++; cx += (float)FIELD(u, OF(UNIT, 0x1C), int); cy += (float)FIELD(u, OF(UNIT, 0x20), int);
                gl = FIELD(u, OF(UNIT, 0x1EC), uint8_t *);
                if (!gl) none++;
                else if (*(uintptr_t *)gl == GA(VT_GOAL_LOCATION)) walk++;
                else if (*(uintptr_t *)gl == GA(VT_GOAL_ATTACK)) fight++;
                else other++;
            }
            fprintf(g_log, "# asgtrace: P%u at %.2f min: group %u %s %p: state %d -> %d mission %d goal %d,%d; %d alive at %.0f,%.0f, "
                    "%u waypoints, %d engaged; %d march %d fight %d other %d idle", p, now / 60000.0, k, rtti_name(sg),
                    (void *)sg, g_trace[slot].state, st, FIELD(sg, OF(ASG, 0x124), int), gx, gy, n, n ? cx / n : 0.0f, n ? cy / n : 0.0f,
                    FIELD(sg, OF(ASG, 0x70), unsigned), engaged, walk, fight, other, none);
            for (int w = 0; w < 2; w++) {   // the subgroup's target unit (+0x118) and the group's (+0x28 + 0x1C)
                uint8_t *t = w ? FIELD(g, OF(AG, 0x44), uint8_t *) : FIELD(sg, OF(ASG, 0x118), uint8_t *);
                uint8_t *ttd = t ? FIELD(t, OF(UNIT, 0x2C), uint8_t *) : NULL;
                if (ttd) fprintf(g_log, "; %s target type %d at %d,%d hp %d", w ? "group" : "its", FIELD(ttd, OF(TD, 0x4B8), int),
                                 FIELD(t, OF(UNIT, 0x1C), int), FIELD(t, OF(UNIT, 0x20), int), FIELD(t, OF(UNIT, 0x3C), int));
            }
            fprintf(g_log, "; group %p +28:", (void *)g);
            for (int o = 0x28; o < 0x6C; o += 4) fprintf(g_log, " %x", FIELD(g, o, unsigned));
            fputc('\n', g_log);
            g_trace[slot].state = st; g_trace[slot].goal_x = gx; g_trace[slot].goal_y = gy;
        }
    }
}
internal void cmd_asgtrace(char *out, const char *args) {
    int p = atoi(args);
    if (p <= 0 || p >= 16) { REPLY("error: asgtrace P [off]\n"); return; }
    if (strstr(args, "off")) g_trace_mask &= ~(1u << p); else g_trace_mask |= 1u << p;
    REPLY("attack subgroup trace: mask %x\n", g_trace_mask);
}

internal void attack_log(ee_player *pl, uint32_t now);
internal void attack_tick(ee_player *pl, uint32_t now) {
    asg_trace_tick(pl, now);
    charge_tick(now);
    threat_refresh(pl, now);   // also with avoid_towers=0: the route log compares both
    citizen_attack_scan(pl, now);
    breach_prune(pl);
    siege_tick(pl, now);
    attack_log(pl, now);
}
// a vanilla player (game/players.c is_vanilla): the game's own AI - only the mod's log lines (army, battle statistics,
// spending, gathering, attack times), so the scorers see both sides
internal void vanilla_tick(ee_player *pl, uint32_t now) {
    econ_vanilla_log(pl, now);
    attack_log(pl, now);
}
// the army line every 5 minutes, the battle statistics every 10
internal void attack_log(ee_player *pl, uint32_t now) {
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
        uint8_t *st = (uint8_t *)pl + OF(PL, 0x4A8);
        for (int k = 0; k < 30; k++) {
            uint8_t *b = FIELD(st, OF(PLSTATS, 0x1C) + k * 0x18, uint8_t *), *e = FIELD(st, OF(PLSTATS, 0x20) + k * 0x18, uint8_t *);
            int ok = b && e > b && (e - b) % 8 == 0 && e - b < (1 << 20) && mem_readable(e - 8, 8);
            fprintf(g_log, " %d", ok ? FIELD(e, -8, int) : 0);
        }
        fputc('\n', g_log);
        fflush(g_log);
    }
    last_stats[pi] = now;
}


// Engine guard: an attack subgroup's member records (+0x50 vector, record +0x44 = unit) are summed up by 0x4504FD ->
// 0x450554 (thiscall(stats, unit)) without a check. A record without its unit crashed there (read [0 + 0x34]) in a
// long game, one AI step after two AIs surrendered and attacks on them were called off (0x43033E with 1: the
// subgroups hand their units back, 0x455C90). Such a record is skipped and logged.
internal void __fastcall hook_group_stat(void *stats, void *edx, ee_unit *unit) {
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
    group_stat(stats, unit);
}

// The same records read without a check by each subgroup class's vf20 (members' stats into +0x160: 0x45AD2F, land
// 0x4472A9) - a crash at 0x45AD70 (read [0 + 0x2C]) in the user's game at 49.6 min, right after the guard above had
// skipped such a record (12 minutes after attacks were called off for an alliance), and five more readers of the
// pattern. Before vf20 runs, records without a unit leave the member vector (+0x54..+0x58; the records themselves are
// left alone, in case anything else still points at them).
internal void asg_purge(uint8_t *sg) {
    uint8_t **b = FIELD(sg, OF(ASG, 0x54), uint8_t **), **e = FIELD(sg, OF(ASG, 0x58), uint8_t **), **w = b;
    if (!b || e < b || e - b > 4096) return;
    for (uint8_t **r = b; r < e; r++)
        if (*r && FIELD(*r, OF(ASGU, 0x44), ee_unit *)) *w++ = *r;
    if (w == e) return;
    FIELD(sg, OF(ASG, 0x58), uint8_t **) = w;
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
internal void asg_purge_all(ee_player *pl) {
    uint8_t *cbm = pl ? FIELD(pl, OF(PL, 0xB8C), uint8_t *) : NULL, *cmd = cbm ? FIELD(cbm, OF(AM, 0x48), uint8_t *) : NULL;
    if (!cmd) return;
    uint8_t *dq = cmd + 4;
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        uint8_t *g = deque_at(dq, first + k);
        for (uint8_t **sp = g ? FIELD(g, OF(AG, 0x8), uint8_t **) : NULL; sp && sp < FIELD(g, OF(AG, 0xC), uint8_t **); sp++)
            if (*sp) asg_purge(*sp);
    }
}
internal void __fastcall hook_asg_stats(void *sg, void *edx) {
    (void)edx;
    asg_purge(sg);
    asg_stats(sg);
}
internal void __fastcall hook_asg_land_stats(void *sg, void *edx) {
    (void)edx;
    asg_purge(sg);
    asg_land_stats(sg);
}

// The same family, now reproducible: the reference save, P2's attack called off at 86.6 min (attack_retreat, or the
// test command calloff 2 1 - with and without honest_vision or landings) crashed every time. The call-off
// (0x43033E -> 0x46FD01) hands each unit of the attack back, and each one is offered to the attack groups at once
// (0x483ADE -> 0x4824BF -> 0x450E07 -> 0x453E10) - including groups the same call-off has just freed (their subgroup
// vectors garbage, a jump to 0). While the mod calls attacks off (g_in_calloff, diplo.c) no unit is offered: "not
// taken" (the caller only notes it), the unit stays free and the game assigns it on its next pass.
internal int g_in_calloff;
internal unsigned g_calloff_skips;
internal char __fastcall hook_unit_to_group(void *cmd, void *edx, ee_unit *unit) {
    (void)edx;
    if (g_in_calloff) { g_calloff_skips++; return 0; }
    return unit_to_group(cmd, unit);
}

// UNIT LIST: every unit - also the PathPoint markers (type 223) of an attack's route - sits in one list sorted by tile
// x (unit +8 -> node {next, prev}; the head the RTSDummyUnit at 0x918314, its +4 the first), re-sorted at each move
// (0x616F9D -> 0x5310B3) and walked by range searches (0x62FE01). The call-off frees a subgroup's PathPoints
// (EECPAttackSubGroup vf26 0x45B4FB -> the unit pool's release, EEComplexUnit vf13 0x5ED71E -> vf12 -> 0x628BC6) while
// they are still in that list (mIsValid +0x2B already 0, so the destructor's check stays quiet): the node is deleted,
// the neighbours keep pointing at the unit, the pool hands the memory out again (an arrow, a stone ball) and it is
// linked in elsewhere - the list gets a loop (the stalls: 0x5310B3 / 0x62FE01 spin for good) or a NULL link (crashes
// at 0x628D6F, 0x53110D, 0x531131). Older than the 2026-10-17 review (the DLL before it stalled too), set off by the
// mod's own call-offs at the safe point (attack_retreat, citizen attacks, diplomacy). Guard: the node delete of both
// destructor parts (0x628BE7 in 0x628BC6, 0x628B7F in 0x628B58) first takes a unit still linked out of the list with the
// game's own unlink (0x628E29: the neighbours, or the head, get its links).
internal unsigned g_unlinked;
__attribute__((used)) void __cdecl unit_node_free(uint8_t **node, ee_unit *u) {
    if (!node || (!node[0] && !node[1]) || FIELD(u, OF(UNIT, 0x8), uint8_t **) != node) return;
    unit_unlink(u);
    if (g_log && g_unlinked++ < 5) {
        uint8_t *td = FIELD(u, OF(UNIT, 0x2C), uint8_t *);
        fprintf(g_log, "# engine guard: a unit (type %d) freed while still in the unit list - taken out at %.1f min\n",
                td ? FIELD(td, OF(TD, 0x4B8), int) : -1, game_time() / 60000.0);
    }
}
__attribute__((used)) uintptr_t g_op_delete;   // GA(FN_OP_DELETE), set with the hooks
__attribute__((naked, used)) void unit_node_free_thunk(void) {   // at the call of operator delete(node), ESI = the unit
    __asm__ volatile (
        "pushal\n\t"
        "pushl %esi\n\t"
        "pushl 40(%esp)\n\t"          // the node (the pushed argument: 32 pushal + 4 esi + 4 return address)
        "call _unit_node_free\n\t"
        "addl $8, %esp\n\t"
        "popal\n\t"
        "movl _g_op_delete, %eax\n\t"  // FN_OP_DELETE (eax is free: the delete's result is not used)
        "jmp *%eax\n\t");
}

// The units are reference counted (+4; 0x4FCAD3 releases one: above 1 the count goes down, else vf13) and come from a
// pool (0x9413B0, 0x348 bytes each): EEComplexUnit vf13 (0x5ED71E) runs vf12 (0x628BC6 deletes the list node, sets
// +0x2A) and gives the memory back. An attack subgroup that still holds a unit already given back releases it again
// (seen from 0x482028 under the subgroups' vf85 and from the world's deferred deletions 0x53066A, the count already
// 0): the pool then holds it twice and hands one memory out to two units - one of them is reset while it is linked
// (0x628CDD from the projectile set-up 0x5EA105), the unit list loops or breaks as above. Once the memory is out again
// such a stale release hits a live unit (mIsValid +0x2B still 1: a unit is taken out of the game - 0x628B8E, mIsValid
// 0 - before its last release; 0 such releases in normal play). Guard: a release of a unit that is back in the pool
// (count 0, +0x2A set, no node - kept until the pool's reset on reuse) or still in the game is skipped.
internal unsigned g_release_again;
internal void __fastcall hook_unit_release(ee_unit *u, void *edx) {
    (void)edx;
    int again = !FIELD(u, OF(UNIT, 0x4), int) && FIELD(u, OF(UNIT, 0x2A), uint8_t) == 1 && !FIELD(u, OF(UNIT, 0x8), void *), live = FIELD(u, OF(UNIT, 0x2B), uint8_t) != 0;
    if (again || live) {
        if (g_log && g_release_again++ < 10) {
            uint8_t *td = FIELD(u, OF(UNIT, 0x2C), uint8_t *);
            fprintf(g_log, "# engine guard: a unit (type %d) %s - not given back to the pool at %.1f min\n", td ? FIELD(td, OF(TD, 0x4B8), int) : -1,
                    again ? "released a second time" : "released while still in the game", game_time() / 60000.0);
        }
        return;
    }
    unit_release(u);
}

// RANGE SORT: 0x630466 (under EEUnitClass vf48 0x620122, a unit's look at the enemies around it) sorts the enemies in
// its range by 0x62D5A4(unit, enemy) (a distance) with std::sort (0x63A355; 0x63AB78 / 0x63B79D). Its unguarded
// partition (0x63B9E5) compares a key just computed on the x87 stack (80 bits) with the pivot's, stored as a float: the
// pivot can compare greater than itself, and the scan runs off the array - a crash at 0x62D5AC reading 0x88000F20 /
// 0x8C000F8D (6 of 360 games against the base AI with more citizens, deterministic: it takes 17+ enemies in range,
// below that the game's insertion sort is guarded). Replaced by an insertion sort on keys computed once (floats), ties
// kept in their order - the order of the game's insertion sort.
internal void __cdecl hook_range_sort(void **b, void **e, ee_unit *unit) {
    int n = b && e > b ? (int)(e - b) : 0;
    if (n < 2) return;
    if (n > 1024) { range_sort(b, e, unit); return; }
    static float key[1024];   // the simulation's thread only
    for (int i = 0; i < n; i++) key[i] = unit_distance(unit, b[i]);
    for (int i = 1; i < n; i++) {
        float k = key[i];
        void *v = b[i];
        int j = i;
        for (; j > 0 && key[j - 1] > k; j--) { key[j] = key[j - 1]; b[j] = b[j - 1]; }
        key[j] = k; b[j] = v;
    }
}

// A unit whose goal says 0xFBA (+0x1E8 -> +0x24) but whose goal object (+0x1F0) is not of the class that goal id
// belongs to: 0x5ED24B dynamic_casts +0x1F0 (0x5ED9B4) and calls 0x60FE7E on the result unchecked - NULL, read at
// +0x38 (0x60FED3). Seen with workforce_plan ARMY in one Tournament Islands Huge game (3 of 3 runs at minute 31). The
// guard answers "no" for a NULL goal, as the cast's own failure would mean (the branch then changes nothing).
internal unsigned g_goal_cast_null;
internal char __fastcall hook_goal_cast(void *goal, void *edx, void *unit) {
    (void)edx;
    if (goal) return goal_fits(goal, unit);
    if (g_log && g_goal_cast_null++ < 5)
        fprintf(g_log, "# engine guard: a unit's goal 0xFBA without its goal object (type %d) at %.1f min - skipped\n",
                unit && FIELD(unit, OF(UNIT, 0x2C), uint8_t *) ? FIELD(FIELD(unit, OF(UNIT, 0x2C), uint8_t *), OF(TD, 0x4B8), int) : -1, game_time() / 60000.0);
    return 0;
}
internal void attack_install(const char *aimod_ini, const char *autotest_ini) {
    BOOL cast_guard = patch_goal_fits(hook_goal_cast);
    if (g_log) fprintf(g_log, "# engine guard: goal 0xFBA without its object %s\n", cast_guard ? "ok" : "FAILED");
    g_op_delete = game_op_delete();
    BOOL sort_guard = patch_range_sort(hook_range_sort);
    if (g_log) fprintf(g_log, "# engine guard: the range sort %s\n", sort_guard ? "ok" : "FAILED");
    int list_guard = patch_unit_node_free(unit_node_free_thunk, NULL) + patch_unit_release(hook_unit_release);
    if (g_log) fprintf(g_log, "# engine guard: units freed while in the unit list or twice %d/3\n", list_guard);
    BOOL stat_guard = patch_group_stat(hook_group_stat);
    int sg_guard = patch_asg_land_stats(hook_asg_land_stats) + patch_asg_stats(hook_asg_stats, NULL);
    if (g_log) fprintf(g_log, "# engine guard: attack subgroup stats %d/7\n", sg_guard);
    if (g_log) fprintf(g_log, "# engine guard: attack subgroup members %s\n", stat_guard ? "ok" : "FAILED");
    BOOL add_guard = patch_unit_to_group(hook_unit_to_group);
    if (g_log) fprintf(g_log, "# engine guard: no unit offered to attack groups during a call-off %s\n", add_guard ? "ok" : "FAILED");
    g_avoid_towers = cfg_int(aimod_ini, autotest_ini, 1, "avoid_towers", 1);
    g_avoid_citizens = cfg_int(aimod_ini, autotest_ini, 1, "avoid_towers_citizens", 1);
    g_cit_threats = cfg_int(aimod_ini, autotest_ini, 1, "citizen_threats", 1);
    g_bunkers = cfg_int(aimod_ini, autotest_ini, 1, "bunkers", 1);
    g_bk_res = cfg_int(aimod_ini, autotest_ini, 1, "bunker_reserve", 0xFFFF);
    BOOL repair = patch_repair_goal(hook_repair_goal);
    if (g_log) fprintf(g_log, "# repairs under attack: refused (hook %s)\n", repair ? "ok" : "FAILED");
    g_avoid_weight = cfg_int(aimod_ini, autotest_ini, 1, "avoid_towers_weight", 2000);
    g_avoid_margin = cfg_int(aimod_ini, autotest_ini, 1, "avoid_towers_margin", 6);
    g_weak_spots = cfg_int(aimod_ini, autotest_ini, 1, "attack_weak_spots", 1);
    g_weak_radius = cfg_int(aimod_ini, autotest_ini, 1, "attack_weak_spot_radius", 60);
    g_leash = cfg_int(aimod_ini, autotest_ini, 1, "attack_leash", 1);
    g_citizen_attacks = cfg_int(aimod_ini, autotest_ini, 1, "citizen_attacks", 0);
    g_breach = cfg_int(aimod_ini, autotest_ini, 1, "attack_breach", 1);
    g_siege = cfg_int(aimod_ini, autotest_ini, 1, "attack_siege", 1);
    g_ignore_air = cfg_int(aimod_ini, autotest_ini, 1, "attack_ignore_air", 1);
    int a = patch_route_cost(hook_route_cost);
    int b = patch_asg_route(hook_asg_route);
    int c = patch_asg_route_done(hook_asg_route_done);
    int nd = 0, d = patch_asgu_score(hook_asgu_score, &nd) == nd;   // land unit records: base, hero, priests and prophets
    if (g_log) fprintf(g_log, "# attack routing: avoid_towers=%d weight=%d margin=%d weak_spots=%d radius=%d leash=%d breach=%d hooks %d/4\n",
                       g_avoid_towers, g_avoid_weight, g_avoid_margin, g_weak_spots, g_weak_radius, g_leash, g_breach, a + b + c + d);
}

// threat <player>: the enemy defences this AI knows about
internal void cmd_threat(char *out, const char *args) {
    int p = atoi(args);
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
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
internal void cmd_attacks(char *out, const char *args) {
    int p = atoi(args);
    ee_player *pl = (p > 0 && p < 16) ? g_players[p] : NULL;
    uint8_t *cbm = pl ? FIELD(pl, OF(PL, 0xB8C), uint8_t *) : NULL, *cmd = cbm ? FIELD(cbm, OF(AM, 0x48), uint8_t *) : NULL;
    if (!cmd) { REPLY("error: no AI player %d with an attack commander\n", p); return; }
    uint8_t *dq = cmd + 4;
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    REPLY("P%d: %u attack groups, next attack at %.1f min\n", p, cnt, FIELD(pl, OF(PL, 0xED8), uint32_t) / 60000.0);
    for (unsigned k = 0; k < cnt && k < 16; k++) {
        uint8_t *g = deque_at(dq, first + k);
        ee_player *target = g ? FIELD(g, OF(AG, 0x48), ee_player *) : NULL;
        if (!g) continue;
        int ns = 0;
        for (uint8_t **sp = FIELD(g, OF(AG, 0x8), uint8_t **); sp && sp < FIELD(g, OF(AG, 0xC), uint8_t **); sp++) ns++;
        REPLY(" group %u: target P%d, %d subgroups\n", k, target ? FIELD(target, PL_INDEX, int) : -1, ns);
        for (uint8_t **sp = FIELD(g, OF(AG, 0x8), uint8_t **); sp && sp < FIELD(g, OF(AG, 0xC), uint8_t **); sp++) {
            if (!*sp) continue;
            int n = 0, walk = 0, fight = 0, other = 0, none = 0, engaged = 0;
            float cx = 0, cy = 0;
            uint8_t **b = FIELD(*sp, OF(ASG, 0x54), uint8_t **), **e = FIELD(*sp, OF(ASG, 0x58), uint8_t **);
            for (uint8_t **r = b; b && r < e && n < 1000; r++) {
                ee_unit *u = *r ? FIELD(*r, OF(ASGU, 0x44), ee_unit *) : NULL;
                uint8_t *gl;
                if (!u || FIELD(u, OF(UNIT, 0x3C), int) <= 0) continue;
                if (FIELD(*r, OF(ASGU, 0x48), void *)) engaged++;
                n++; cx += (float)FIELD(u, OF(UNIT, 0x1C), int); cy += (float)FIELD(u, OF(UNIT, 0x20), int);
                gl = FIELD(u, OF(UNIT, 0x1EC), uint8_t *);
                if (!gl) none++;
                else if (*(uintptr_t *)gl == GA(VT_GOAL_LOCATION)) walk++;
                else if (*(uintptr_t *)gl == GA(VT_GOAL_ATTACK)) fight++;
                else other++;
            }
            const char *cls = rtti_name(*sp);
            REPLY("   %-28s %3d alive at %.0f,%.0f: %d march, %d fight, %d other, %d idle\n", cls, n,
                  n ? cx / n : 0.0f, n ? cy / n : 0.0f, walk, fight, other, none);
            uint8_t *sg = *sp;
            REPLY("     state %d mission %d, goal %d,%d, %u waypoints, at %d,%d, %d engaged\n",
                  FIELD(sg, OF(ASG, 0x1D0), int), FIELD(sg, OF(ASG, 0x124), int), FIELD(sg, OF(ASG, 0x104), int), FIELD(sg, OF(ASG, 0x108), int),
                  FIELD(sg, OF(ASG, 0x70), unsigned), FIELD(sg, OF(ASG, 0x234), int), FIELD(sg, OF(ASG, 0x238), int), engaged);
            if (n && strstr(args, "raw")) {   // the subgroup's fields as dwords, to compare over time
                for (uint8_t **r = b; b && r < e; r++) {   // each member: type, tile, goal (and its target)
                    ee_unit *u = *r ? FIELD(*r, OF(ASGU, 0x44), ee_unit *) : NULL;
                    if (!u || FIELD(u, OF(UNIT, 0x3C), int) <= 0) continue;
                    uint8_t *gl = FIELD(u, OF(UNIT, 0x1EC), uint8_t *);
                    ee_unit *t = NULL;
                    if (gl && *(uintptr_t *)gl == GA(VT_GOAL_ATTACK)) t = FIELD(gl, OF(GOAL, 0x30), ee_unit *);
                    REPLY("       %d at %d,%d hp %d: %s", FIELD(FIELD(u, OF(UNIT, 0x2C), uint8_t *), OF(TD, 0x4B8), int),
                          FIELD(u, OF(UNIT, 0x1C), int), FIELD(u, OF(UNIT, 0x20), int), FIELD(u, OF(UNIT, 0x3C), int), gl ? rtti_name(gl) : "-");
                    if (t) REPLY(" P%d's %d (family %d) at %d,%d", FIELD(unit_owner(t), PL_INDEX, int),
                                 FIELD(FIELD(t, OF(UNIT, 0x2C), uint8_t *), OF(TD, 0x4B8), int), FIELD(FIELD(t, OF(UNIT, 0x2C), uint8_t *), OF(TD, 0x4B4), int),
                                 FIELD(t, OF(UNIT, 0x1C), int), FIELD(t, OF(UNIT, 0x20), int));
                    REPLY(" rec+48 %08x\n", FIELD(*r, OF(ASGU, 0x48), unsigned));
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

internal int is_ship_or_air(uint8_t *td) {   // movement type +0x10C... unknown: by the ship/air families seen in tests
    int fam = FIELD(td, OF(TD, 0x4B4), int);
    return fam == 0x0B || fam == 0x0C || fam == 0x0D || fam == 0x0E || fam == 0x0F || fam == 0x10 || fam == 0x11;
}

// a new game (the game start hook): subgroups, units and times of the last game - a breach, a citizen's hit points
// (hurt_note: a new citizen at a dead one's address seemed hurt), a traced subgroup, a pending call-off's reason
internal void attack_new_game(void) {
    memset(g_threat, 0, sizeof g_threat);
    memset(g_breaches, 0, sizeof g_breaches);
    memset(g_route_stats, 0, sizeof g_route_stats);
    memset(g_vis_floor, 0, sizeof g_vis_floor);
    memset(g_cancel_why, 0, sizeof g_cancel_why);
    memset(g_leash_stats, 0, sizeof g_leash_stats);
    memset(g_siege_st, 0, sizeof g_siege_st);
    memset(g_hurt, 0, sizeof g_hurt);
    memset(g_cit_hp, 0, sizeof g_cit_hp);
    memset(g_hurt_count, 0, sizeof g_hurt_count);
    memset(g_cit_stats, 0, sizeof g_cit_stats);
    memset(g_repair_refused, 0, sizeof g_repair_refused);
    memset(g_siege_train, 0, sizeof g_siege_train);
    memset(g_trace, 0, sizeof g_trace);
}
