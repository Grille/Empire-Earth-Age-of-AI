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
// Type data: +0x4B4 class_id (CLS_BUILDING, CLS_FORTRESS), +0x4B8 type id, +0x420 range, +0x510 attack,
//   +0x55C hit points.

#define MAX_DEFENCES 256
struct defence { int x, y, owner; float range, attack; int type; ee_unit *u; };
internal struct {
    struct defence d[MAX_DEFENCES];
    int n;
    uint32_t refreshed;
} g_threat[16];

internal int g_avoid_towers = 1;        // ageofai.ini [ai_base] avoid_towers
internal int g_avoid_weight = 2000;     // route cost per tower covering a step (a step costs 10-14)
internal int g_avoid_margin = 6;        // tiles added to the tower's range


// What player pl knows of unit u. The count (u +0x14 -> +0xC, per player slot) is how many of pl's (and its allies')
// units see u now - plus a floor every unit of the human carries in the AIs' slots, everywhere and always (the AIs
// see all of the human's units; docs/engine-notes.md). [ai_base] honest_vision (intel.c): seen = above that floor, or a
// building pl has seen before (still where it was - a human remembers it on the map), the default; 0: the count as
// the game keeps it - all-knowing about the human.
internal int g_honest_vision = 1;
internal uint16_t g_vis_floor[16][16];               // [viewer][owner]: the least count among the owner's units (intel.c)
internal int intel_remembers(ee_player *pl, ee_unit *u);   // intel.c: a building pl has seen
internal void intel_soldiers(ee_player *pl, int x, int y, int r, float *hp, float *att, float *pw, int *n);   // intel.c
internal void intel_army(ee_player *pl, float *hp, float *att, float *pw, int *n);
internal uint32_t intel_looked_ago(ee_player *pl, int x, int y);
internal int seen_by(ee_unit *u, ee_player *pl) {
    ee_unit_vis *v = unit_get_vis(u);
    unsigned pi = (unsigned)player_get_index(pl);
    if (!v || pi >= 16) return 0;
    uint16_t *vis = unit_vis_get_seen(v);
    if (!vis) return 0;
    unsigned c = sight_count(vis[pi]);
    if (!g_honest_vision) return c != 0;
    ee_player *o = unit_get_owner(u);
    unsigned oi = o ? (unsigned)player_get_index(o) & 15 : 0;
    if (c > g_vis_floor[pi][oi]) return 1;
    int fam = unit_type_get_class_id(unit_get_type_data(u));
    return (fam == CLS_BUILDING || fam == CLS_FORTRESS) && intel_remembers(pl, u);
}

// Defence building with a ground attack (AA guns and missile towers only shoot at aircraft).
internal int defence_stats(ee_unit *u, float *range, float *attack) {
    ee_unit_type *td = unit_get_type_data(u);
    if (!td) return 0;
    int fam = unit_type_get_class_id(td), type = unit_type_get_id(td);
    if (fam != CLS_BUILDING && fam != CLS_FORTRESS) return 0;
    if (type == UT(B_AA10_GUN_88MM) || type == UT(B_AA13_MISSILE_TOWER)) return 0;
    ee_unit_stats *st = unit_get_stats(u);
    float r = st ? unit_stats_get_range(st) : unit_type_get_range(td);
    float a = st ? unit_stats_get_attack(st) : (float)unit_type_get_attack(td);
    if (!(a > 0.0f) || !(r > 1.0f) || r > 30.0f) return 0;
    *range = r; *attack = a;
    return 1;
}

internal void threat_refresh(ee_player *pl, uint32_t now) {
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    if (g_threat[pi].refreshed && now >= g_threat[pi].refreshed && now - g_threat[pi].refreshed < 5000) return;
    g_threat[pi].refreshed = now ? now : 1;
    int n = 0;
    for (struct grid_it it = grid_it_all(1); n < MAX_DEFENCES && grid_it_next(&it); ) {
        ee_unit *u = it.u;
        float r, a;
        if (!is_enemy(pl, unit_get_owner(u)) || !defence_stats(u, &r, &a) || !seen_by(u, pl)) continue;
        int x = unit_get_x(u), y = unit_get_y(u), k;
        for (k = 0; k < n; k++)                     // one defence per tile (the iterator already visits a big building once)
            if (g_threat[pi].d[k].x == x && g_threat[pi].d[k].y == y) break;
        if (k < n) continue;
        struct defence *d = &g_threat[pi].d[n++];
        d->x = x; d->y = y; d->range = r; d->attack = a;
        d->owner = player_get_index(unit_get_owner(u));
        d->type = unit_type_get_id(unit_get_type_data(u));
        d->u = u;
    }
    g_threat[pi].n = n;
}

// Sum of known defences covering tile (x, y), each weighted by its attack (a guard tower = 1).
internal float threat_at(unsigned pi, ivec2 p) {
    float t = 0;
    for (int k = 0; k < g_threat[pi & 15].n; k++) {
        struct defence *d = &g_threat[pi & 15].d[k];
        float r = d->range + (float)g_avoid_margin;
        if ((float)ivec2_dist2(p, iv2(d->x, d->y)) <= r * r) t += d->attack / 50.0f;
    }
    return t;
}

typedef int (__thiscall *RouteCostFn)(void *self, int *edge, uint8_t *node);
internal struct { unsigned calls, penalised; } g_route_stats[16];
internal DWORD g_route_thread;

internal int __thiscall hook_route_cost(ee_route_planner *self, int *edge, ee_route_node *node) {
    int cost = route_planner_cost(self, edge, node);
    unsigned pi = (unsigned)route_planner_get_player_index(self) & 15;     // planner +0x48 = own player index
    ee_player *pl = is_vanilla_idx((int)pi) ? NULL : g_players[pi];   // a vanilla player: the game's cost
    if (!g_route_thread) {
        g_route_thread = GetCurrentThreadId();
        if (g_log) fprintf(g_log, "# routes: cost hook runs on thread %lu\n", g_route_thread);
    }
    if (!pl || !node) return cost;
    g_route_stats[pi].calls++;
    float t = threat_at(pi, iv2(route_node_get_x(node), route_node_get_y(node)));
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
internal int g_weak_spots = 1;          // ageofai.ini [ai_base] attack_weak_spots
internal int g_weak_radius = 60;        // tiles around the original goal searched for a weak spot


// Only goals at an enemy building are retargeted: the subgroup also marches to other goals (enemy armies,
// own territory to defend), which must not be diverted.
internal int building_at(ee_player *pl, const int *dest, int r) {
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid) return 0;
    for (struct grid_it it = grid_it_box(dest[0], dest[1], r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (!td || unit_type_get_class_id(td) != CLS_BUILDING || is_wall(unit_type_get_id(td)) || !is_enemy(pl, unit_get_owner(u)))
            continue;
        if (ivec2_dist2(unit_get_tile(u), iv2(dest[0], dest[1])) <= r * r) return 1;
    }
    return 0;
}

internal ee_unit *find_weak_spot(ee_player *pl, unsigned pi, const int *start, const int *dest) {
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid) return NULL;
    short home = (short)map_area(iv2(start[0], start[1]), 2);
    ee_unit *best = NULL;
    float best_score = 1e30f;
    int r = g_weak_radius;
    for (struct grid_it it = grid_it_box(dest[0], dest[1], r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (!td || unit_type_get_class_id(td) != CLS_BUILDING || is_wall(unit_type_get_id(td)) || !is_enemy(pl, unit_get_owner(u)) ||
            !seen_by(u, pl))
            continue;
        ivec2 pos = unit_get_tile(u);
        float d2 = (float)ivec2_dist2(pos, iv2(dest[0], dest[1]));
        if (d2 > (float)(r * r) || threat_at(pi, pos) > 0) continue;
        if ((short)map_area(pos, 2) != home) continue;
        float score = sqrtf((float)ivec2_dist2(pos, iv2(start[0], start[1]))) + 0.5f * sqrtf(d2);
        if (score < best_score) { best_score = score; best = u; }
    }
    return best;
}

// Breach: the route planner works on ~10-tile sectors and ignores walls, so an army marches straight to an enemy
// wall and piles up in front of it - next to the towers that guard the gap. When the straight line to the goal
// crosses a known enemy wall, the subgroup first attacks a wall segment outside known tower range, the one with
// the shortest detour along that wall (find_breach); the scorer makes that segment the units' target (walls score
// below zero otherwise). Walls are tough: without siege the breach takes long or fails.
internal int g_breach = 1;              // ageofai.ini [ai_base] attack_breach
// The owner is kept with the entry: a subgroup the game deleted (its attack over) is never read again.
internal struct { ee_attack_subgroup *sg; ee_player *pl; int x, y; } g_breaches[64];   // subgroup (of player pl) -> wall tile it is to breach

internal void breach_note(ee_player *pl, ee_attack_subgroup *sg, int x, int y) {
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
internal int asg_live(ee_player *pl, ee_attack_subgroup *sg);   // forces.c
internal void breach_prune(ee_player *pl) {
    for (int k = 0; k < 64; k++)
        if (g_breaches[k].sg && g_breaches[k].pl == pl && !asg_live(pl, g_breaches[k].sg)) g_breaches[k].sg = NULL;
}

internal int breach_of(ee_attack_subgroup *sg, int *x, int *y) {
    for (int k = 0; k < 64; k++)
        if (g_breaches[k].sg == sg) { *x = g_breaches[k].x; *y = g_breaches[k].y; return 1; }
    return 0;
}

internal ee_unit *wall_at(ee_player *pl, int x, int y) {
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid) return NULL;
    int cs = grid_get_cell_size(grid), nx = grid_get_nx(grid), ny = grid_get_ny(grid);
    ee_grid_cell *cells = grid_get_cells(grid);
    if (cs <= 0 || !cells || x < 0 || y < 0 || x / cs >= nx || y / cs >= ny) return NULL;
    ee_grid_cell *cell = grid_cell_at(cells, (x / cs) * ny + y / cs);
    for (ee_unit **b = grid_cell_get_units_begin(cell); b && b < grid_cell_get_units_end(cell); b++) {
        ee_unit *u = *b;
        ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
        if (td && unit_get_x(u) == x && unit_get_y(u) == y && is_wall(unit_type_get_id(td)) &&
            is_enemy(pl, unit_get_owner(u)) && seen_by(u, pl))
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
    int head = 0, tail = 0, cx = unit_get_x(cross), cy = unit_get_y(cross);
    qx[tail] = cx; qy[tail++] = cy;
    ee_unit *best = NULL;
    float best_score = 1e30f;
    while (head < tail) {
        int x = qx[head], y = qy[head++];
        ee_unit *u = wall_at(pl, x, y);
        if (!u) continue;
        if (!(threat_at(pi, iv2(x, y)) > 0)) {
            float ax = (float)(x - start[0]), ay = (float)(y - start[1]), bx = (float)(dest[0] - x), by = (float)(dest[1] - y);
            float score = sqrtf(ax * ax + ay * ay) + sqrtf(bx * bx + by * by);
            if (score < best_score) { best_score = score; best = u; }
        }
        for (int ddx = -1; ddx <= 1; ddx++)
            for (int ddy = -1; ddy <= 1; ddy++) {
                int nx = x + ddx, ny = y + ddy, seen = 0;
                if (ivec2_dist2(iv2(nx, ny), iv2(cx, cy)) > 60 * 60 || tail >= MAXQ) continue;
                for (int k = 0; k < tail && !seen; k++) seen = qx[k] == nx && qy[k] == ny;
                if (!seen && wall_at(pl, nx, ny)) { qx[tail] = nx; qy[tail++] = ny; }
            }
    }
    return best;
}

internal void diplo_cancel_attacks(int ai, int j);   // diplo.c
typedef void (__thiscall *AsgRouteFn)(void *self, int *start, int *dest);
// CITIZEN ATTACKS ([ai_base] citizen_attacks, default 0): an AI that trains no soldiers (short of wood: 1237 requests in
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
internal void subgroup_count(ee_attack_subgroup *sg, int *n, int *cit) {   // members of an attack subgroup, citizens among them
    ee_asg_unit **b = attack_subgroup_get_units_begin(sg); ee_asg_unit **e = attack_subgroup_get_units_end(sg);
    for (ee_asg_unit **r = b; b && r < e && *n < 1000; r++) {
        ee_unit *u = *r ? asg_unit_get_unit(*r) : NULL;
        ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
        if (!td) continue;
        (*n)++;
        *cit += is_citizen_type(unit_type_get_id(td));
    }
}
internal void citizen_attack_scan(ee_player *pl, uint32_t now) {
    static uint32_t last[16];
    unsigned i = (unsigned)player_get_index(pl) & 15;
    if (g_citizen_attacks || (now >= last[i] && now - last[i] < 5000)) return;
    last[i] = now;
    ee_combat_mgr *cbm = player_get_combat_mgr(pl); ee_attack_cmd *cmd = cbm ? combat_mgr_get_commander(cbm) : NULL;
    if (!cmd) return;
    uint8_t *dq = attack_cmd_groups_at(cmd);
    unsigned first = deque_front(dq), cnt = deque_count(dq);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        ee_attack_group *g = deque_at(dq, first + k);
        ee_player *target = g ? attack_group_get_target(g) : NULL;
        if (!target) continue;
        int n = 0, cit = 0;
        for (ee_attack_subgroup **s = attack_group_get_subgroups_begin(g); s && s < attack_group_get_subgroups_end(g); s++)
            if (*s) subgroup_count(*s, &n, &cit);
        if (n && cit * 2 >= n) {
            cancel_citizen_attack((int)i, player_get_index(target) & 15);
            if (g_log) fprintf(g_log, "# attack: P%u's attack on P%d at %.1f min is %d citizens of %d - called off\n", i,
                               player_get_index(target), now / 60000.0, cit, n);
        }
    }
}
struct wv;
internal struct wv *wave_focus(ee_player *pl, unsigned pi, ee_attack_subgroup *sg, int *start, int *dest);   // stage.c
internal void wave_focus_weak(struct wv *w, ivec2 at, int id);
internal void __thiscall hook_asg_route(ee_attack_subgroup *self, int *start, int *dest) {
    ee_player *pl = attack_subgroup_get_player(self);
    if (is_vanilla(pl)) pl = NULL;   // the game's own route: no weak spot, breach, call-off, nor its log
    void *target = attack_subgroup_get_target(self);
    if (pl && target && !g_citizen_attacks) {
        int n = 0, cit = 0;
        subgroup_count(self, &n, &cit);
        if (n && cit * 2 >= n) {
            cancel_citizen_attack(player_get_index(pl) & 15, player_get_index(target) & 15);
            if (g_log) fprintf(g_log, "# attack: P%d's subgroup on P%d is %d citizens of %d - attack called off\n",
                               player_get_index(pl), player_get_index(target), cit, n);
        }
    }
    struct wv *wave = pl && start && dest ? wave_focus(pl, (unsigned)player_get_index(pl) & 15, self, start, dest) : NULL;
    if (pl && start && dest && g_weak_spots) {
        unsigned pi = (unsigned)player_get_index(pl) & 15;
        float t = threat_at(pi, iv2(dest[0], dest[1]));
        ee_unit *u = t > 0 && building_at(pl, dest, 4) ? find_weak_spot(pl, pi, start, dest) : NULL;
        if (u) {
            ee_location *tg = attack_subgroup_target_rec_at(self);
            if (g_log)
                fprintf(g_log, "# weak spot: P%u goal %d,%d (threat %.1f, target kind %d id %d) -> type %d of P%d at %d,%d\n",
                        pi, dest[0], dest[1], t, location_get_kind(tg), location_get_unit_id(tg),
                        unit_type_get_id(unit_get_type_data(u)), player_get_index(unit_get_owner(u)),
                        unit_get_x(u), unit_get_y(u));
            dest[0] = unit_get_x(u); dest[1] = unit_get_y(u);
            location_set_unit(tg, dest[0], dest[1], unit_get_id(u));
            if (wave) wave_focus_weak(wave, iv2(dest[0], dest[1]), unit_get_id(u));   // the wave's goal now
        }
    }
    if (pl && start && dest && g_breach) {
        unsigned pi = (unsigned)player_get_index(pl) & 15;
        ee_unit *cross = wall_crossing(pl, start, dest);
        ee_unit *w = cross ? find_breach(pl, pi, start, dest, cross) : NULL;
        breach_note(pl, self, w ? unit_get_x(w) : -1, w ? unit_get_y(w) : -1);
        if (w) {
            ee_location *tg = attack_subgroup_target_rec_at(self);
            if (g_log)
                fprintf(g_log, "# breach: P%u goal %d,%d: wall crossed at %d,%d (threat %.1f) -> attack wall at %d,%d\n",
                        pi, dest[0], dest[1], unit_get_x(cross), unit_get_y(cross),
                        threat_at(pi, unit_get_tile(cross)), unit_get_x(w), unit_get_y(w));
            dest[0] = unit_get_x(w); dest[1] = unit_get_y(w);
            location_set_unit(tg, dest[0], dest[1], unit_get_id(w));
        }
    }
    if (g_log && pl && start && dest) {
        unsigned pi = (unsigned)player_get_index(pl) & 15;
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
                pi, target ? player_get_index(target) : -1, game_time() / 60000.0, start[0], start[1],
                dest[0], dest[1], sqrt((double)(dx * dx + dy * dy)), g_threat[pi].n, on_line,
                threat_at(pi, iv2(dest[0], dest[1])), g_route_stats[pi].calls, g_route_stats[pi].penalised);
        fflush(g_log);
    }
    // the target player is chosen once, at the launch (0x46F4FF): after peace or an alliance the march goes on -
    // diplo.c calls such attacks off at the next safe point (never here: it deletes this subgroup)
    int hostile = pl && target && player_get_stance(pl, player_get_index(target) & 15);
    if (pl && target && !hostile) {
        if (g_log) fprintf(g_log, "# route: P%d marches on P%d, no enemy any more\n", player_get_index(pl), player_get_index(target));
        diplo_cancel_attacks(player_get_index(pl) & 15, player_get_index(target) & 15);
    }
    if (pl && hostile) persona_attack(pl, target);
    if (chat_level() && pl && start && dest) {
        unsigned pi = (unsigned)player_get_index(pl) & 15;
        ai_say(pl, CHAT_ROUTE, "[ai] marching on P%d at %d,%d (%.0f tiles), %d enemy defences known",
               target ? player_get_index(target) : -1, dest[0], dest[1], sqrt((double)ivec2_dist2(iv2(dest[0], dest[1]), iv2(start[0], start[1]))), g_threat[pi].n);
    }
    attack_subgroup_plan_route(self, start, dest);
}

// Leash: each unit of an attack subgroup picks its target itself (EECPAttackSubGroupUnit vf5 0x46286F -> vf4
// 0x4620DF scores every enemy the subgroup sees with vf2 0x461854, 0.0 = skip; the best one is attacked via the
// subgroup's slot 7, and while the unit has a target, rec+0x48, it counts as engaged and the march waits).
// Enemy units waiting next to their towers draw the whole army into tower range that way, however good the
// route. So for a land subgroup, an enemy inside known tower range scores 0 - not a target - unless the
// subgroup's own goal (asg+0x100 +4/+8) lies in tower range too; then the attack is meant to go in anyway.
// Unit record: +0x44 unit, +0x48 current target, +0x4C subgroup. Scorer argument: +4 the enemy unit.
internal int g_leash = 1;               // ageofai.ini [ai_base] attack_leash
internal struct { unsigned skipped, logged; } g_leash_stats[16];

internal int g_siege = 1;               // ageofai.ini [ai_base] attack_siege (siege controller below; also rams here)
internal unsigned g_ram_waits, g_air_skips, g_lost_skips;   // g_lost_skips: enemies in a lost fight not chosen (danger_routes)
internal int g_danger_routes;           // [ai_base] danger_routes (game/danger.c)
internal int g_avoid_lost;              // [ai_base] avoid_lost_fights: its DANGER_LOST layer (cover.c cv_zones)
internal int g_ignore_air = 1;          // ageofai.ini [ai_base] attack_ignore_air

// Enemy soldiers (no citizens, no siege) of player pl's enemies within r tiles of (x, y)?
internal int guarded(ee_player *pl, int x, int y, int r) {
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!pl || !grid) return 0;
    for (struct grid_it it = grid_it_box(x, y, r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (!td || unit_type_get_class_id(td) != CLS_LAND || unit_get_hp(u) <= 0 || !is_enemy(pl, unit_get_owner(u)))
            continue;
        int t = unit_type_get_id(td);
        if (ut_role(t, UTR_CITIZEN | UTR_RAM | UTR_SIEGE))
            continue;
        if (ivec2_dist2(unit_get_tile(u), iv2(x, y)) <= r * r) return 1;
    }
    return 0;
}

internal int mass_tether(ee_attack_subgroup *sg, ee_unit *me, ee_unit *enemy);   // mass.c
internal int stage_skip(ee_attack_subgroup *sg, ee_unit *me, ee_unit *enemy);    // stage.c
typedef float (__thiscall *AsguScoreFn)(void *self, uint8_t *info);
internal float __thiscall hook_asgu_score(ee_asg_unit *self, ee_asgu_info *info) {
    float s = asg_unit_score(self, info);
    ee_attack_subgroup *sg = asg_unit_get_subgroup(self);
    ee_unit *enemy = info ? asgu_info_get_enemy(info) : NULL;
    if (!sg || !enemy || !attack_subgroup_is_land(sg) || is_vanilla(attack_subgroup_get_player(sg))) return s;
    // rams (melee, helpless against soldiers): no building while enemy soldiers guard it - they wait with the
    // army until the guards are dealt with
    ee_unit *me = asg_unit_get_unit(self);
    ee_unit_type *mtd = me ? unit_get_type_data(me) : NULL;
    if (g_siege && mtd && is_ram(unit_type_get_id(mtd)) && s != 0.0f &&
        guarded(attack_subgroup_get_player(sg), unit_get_x(enemy), unit_get_y(enemy), 5)) {
        g_ram_waits++;
        return 0.0f;
    }
    // aircraft out of reach: no target. The scorer rates them like any enemy; the subgroup's members chase planes
    // circling over their own town and its march waits while any member is engaged (user's game: 25 of the player's
    // planes over P3's town kept its main force at its docks for 9+ minutes, AA tanks running after them). In reach
    // (its range + 3 tiles) they still shoot.
    ee_unit_type *etd = unit_get_type_data(enemy);
    int efam = etd ? unit_type_get_class_id(etd) : 0;
    if (g_ignore_air && s != 0.0f && me && (efam == CLS_AIRPLANE || efam == CLS_HELICOPTER || efam == CLS_HELICOPTER_TRANSPORT)) {
        ee_unit_stats *st = unit_get_stats(me);
        float r = (st ? unit_stats_get_range(st) : unit_type_get_range(mtd)) + 3.0f;
        if ((float)ivec2_dist2(unit_get_tile(enemy), unit_get_tile(me)) > r * r) {
            g_air_skips++;
            return 0.0f;
        }
    }
    // breach: the scorer rates walls below zero (never chosen while anything else is around); the subgroup's
    // breach segment and its neighbours become the best targets
    int bx, by;
    if (etd && is_wall(unit_type_get_id(etd)) && breach_of(sg, &bx, &by) &&
        abs(unit_get_x(enemy) - bx) <= 1 && abs(unit_get_y(enemy) - by) <= 1)
        return unit_get_x(enemy) == bx && unit_get_y(enemy) == by ? 1000.0f : 500.0f;
    if (s != 0.0f && mass_tether(sg, me, enemy)) return 0.0f;   // mass.c army_mass 16: not far from the pack
    if (s != 0.0f && stage_skip(sg, me, enemy)) return 0.0f;    // stage.c: the army waits at its staging point
    // danger_routes: no enemy in a lost fight (game/danger.c DANGER_LOST, cover.c cv_zones) beyond its own range + 2 -
    // the members ran at a standing army one by one and died (the player's save "standing army"); in reach it fights
    ee_map *map = game_map();
    const uint8_t *lz = g_danger_routes && s != 0.0f && me && map ?
                        danger_map((unsigned)player_get_index(attack_subgroup_get_player(sg)), DANGER_LOST, (int)map_get_size(map)) : NULL;
    if (lz) {
        int size = (int)map_get_size(map), ex = unit_get_x(enemy), ey = unit_get_y(enemy);
        ee_unit_stats *st = unit_get_stats(me);
        float r = (st ? unit_stats_get_range(st) : unit_type_get_range(mtd)) + 2.0f;
        if (ex >= 0 && ey >= 0 && ex < size && ey < size && lz[ey * size + ex] && (float)ivec2_dist2(unit_get_tile(enemy), unit_get_tile(me)) > r * r) {
            g_lost_skips++;
            return 0.0f;
        }
    }
    if (!g_leash || s == 0.0f) return s;
    ee_player *pl = attack_subgroup_get_player(sg);
    if (!pl) return s;
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    int ex = unit_get_x(enemy), ey = unit_get_y(enemy);
    float t = threat_at(pi, iv2(ex, ey));
    if (!(t > 0) || threat_at(pi, iv2(attack_subgroup_get_goal_x(sg), attack_subgroup_get_goal_y(sg))) > 0) return s;
    g_leash_stats[pi].skipped++;
    if (g_log && g_leash_stats[pi].logged < 10) {
        g_leash_stats[pi].logged++;
        ee_unit *u = asg_unit_get_unit(self);
        fprintf(g_log, "# leash: P%u at %.1f min: unit at %d,%d skips enemy at %d,%d (threat %.1f), goal %d,%d\n",
                pi, game_time() / 60000.0, u ? unit_get_x(u) : -1, u ? unit_get_y(u) : -1,
                ex, ey, t, attack_subgroup_get_goal_x(sg), attack_subgroup_get_goal_y(sg));
        fflush(g_log);
    }
    return 0.0f;
}

// Waypoint i of the route deque at asg+0x60 (std::deque of {x, y, ?}, 0x155 per block: +4 map, +8 map size,
// +0xC first index, +0x10 count).
internal int *route_waypoint(ee_attack_subgroup *asg, unsigned i) {
    uint8_t *dq = attack_subgroup_route_at(asg);
    unsigned idx = deque_front(dq) + i, blk = idx / 0x155, msz = deque_map_size(dq);
    uint8_t **map = FIELD(dq, 4, uint8_t **);
    if (!map || !msz) return NULL;
    if (blk >= msz) blk -= msz;
    return map[blk] ? (int *)(map[blk] + (idx % 0x155) * 12) : NULL;
}

// overlay layer "routes" ([display] overlay): a subgroup's route as its player's line from waypoint to waypoint, a
// red ring at each one in reach of known defences, a ring at the goal; kept 3 game minutes or until its next route
internal void route_overlay(ee_attack_subgroup *sg) {
    static int layer = -2;
    if (layer == -2) layer = overlay_layer("routes");
    ee_player *pl = attack_subgroup_get_player(sg);
    if (!overlay_on(layer) || !pl) return;
    unsigned pi = (unsigned)player_get_index(pl) & 15, n = attack_subgroup_get_waypoints(sg);
    uint32_t col = overlay_player_argb((int)pi, 200);
    overlay_begin(layer, (uintptr_t)sg, game_time(), 180000);
    vec2 prev = v2(0, 0);
    for (unsigned i = 0; i < n && i < 400; i++) {
        int *w = route_waypoint(sg, i);
        if (!w) break;
        vec2 p = v2((float)w[0] + 0.5f, (float)w[1] + 0.5f);
        if (i) overlay_line(prev, p, col, 0.25f);
        if (threat_at(pi, iv2(w[0], w[1])) > 0) overlay_circle(p, 0.8f, 0xC0FF2020, 0.2f);
        if (i + 1 == n) overlay_circle(p, 1.5f, col, 0.3f);
        prev = p;
    }
    overlay_end();
}

// overlay layer "danger": a player's danger map - in its colour, a faint fill where enemies reach (DANGER_THREATS), a
// little more where its fight is lost (DANGER_LOST), each zone's edge strong (a fill alone tinted half the map); runs
// of tiles along a row, again every 5 game seconds
internal int danger_class(const uint8_t *th, const uint8_t *lost, int size, int x, int y) {
    if (x < 0 || y < 0 || x >= size || y >= size) return 0;
    return (lost && lost[y * size + x]) ? 2 : (th && th[y * size + x]) ? 1 : 0;
}
internal void danger_overlay(unsigned pi, uint32_t now) {
    static int layer = -2;
    static uint32_t drawn[16];
    if (layer == -2) layer = overlay_layer("danger");
    ee_map *map = game_map();
    if (!overlay_on(layer) || !map || pi >= 16 || (drawn[pi] && now - drawn[pi] < 5000)) return;
    drawn[pi] = now;
    int size = (int)map_get_size(map);
    const uint8_t *th = danger_map(pi, DANGER_THREATS, size), *lost = danger_map(pi, DANGER_LOST, size);
    overlay_begin(layer, pi, now, 30000);
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; ) {
            int k = danger_class(th, lost, size, x, y), e = x + 1;
            if (!k) { x++; continue; }
            while (e < size && danger_class(th, lost, size, e, y) == k) e++;
            overlay_tiles(iv2(x, y), e - x, overlay_player_argb((int)pi, k == 2 ? 40 : 18));
            for (int t = x; t < e; t++)   // the edge: a neighbour of a lower class
                if (danger_class(th, lost, size, t - 1, y) < k || danger_class(th, lost, size, t + 1, y) < k ||
                    danger_class(th, lost, size, t, y - 1) < k || danger_class(th, lost, size, t, y + 1) < k)
                    overlay_tiles(iv2(t, y), 1, overlay_player_argb((int)pi, k == 2 ? 150 : 90));
            x = e;
        }
    overlay_end();
}

typedef void (__thiscall *AsgFn)(void *self);
internal void __thiscall hook_asg_route_done(ee_attack_subgroup *self) {
    ee_player *pl = attack_subgroup_get_player(self);
    if (g_log && pl) {
        unsigned pi = (unsigned)player_get_index(pl) & 15, n = attack_subgroup_get_waypoints(self), hot = 0;
        char buf[600]; int len = 0;
        for (unsigned i = 0; i < n && i < 400; i++) {
            int *w = route_waypoint(self, i);
            if (!w) break;
            int h = threat_at(pi, iv2(w[0], w[1])) > 0;
            hot += h;
            if (len < (int)sizeof buf - 24) len += sprintf(buf + len, " %d,%d%s", w[0], w[1], h ? "!" : "");
        }
        fprintf(g_log, "# route done: P%u %u waypoints, %u in reach of known defences:%s\n", pi, n, hot, buf);
        fflush(g_log);
    }
    route_overlay(self);
    attack_subgroup_route_done(self);
}

// Soldiers (not buildings, walls, citizens) of a player, from the unit grid.
internal int army_size(ee_player *pl) {
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid) return -1;
    int n = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (!td || unit_get_owner(u) != pl) continue;
        // units only sit in the cell of their position
        if (is_army_family(unit_type_get_class_id(td))) n++;
    }
    return n;
}

// Siege like a player: the AI never sends its siege weapons into its attacks (they stay at home). Every 2 s each
// AI picks a siege target - the nearest known enemy defence its siege outranges, or a wall segment one of its
// subgroups is to breach - and a firing position at (siege range - 0.7) from it, on the side facing the siege,
// where no other known defence reaches. The siege moves there and attacks; idle soldiers (1.5 per siege
// weapon, at least 4) guard the firing position instead of charging in.
// Orders are given like the subgroup's (EECPAttackSubGroup::vf57 0x45CBEA): unit 0x5F05BD(goal, append, 0),
// 0x5EFCF0(state: 0x1F40 PrepareToMove), typedata slot 29 (unit). Goals: EEUGAttackUnit (new 0x40, ctor 0x60AD45(attacker, target,
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
        ee_unit_type *td = it.td;
        if (td && unit_get_x(u) == x && unit_get_y(u) == y && unit_type_get_id(td) == type && unit_get_hp(u) > 0)
            return u;
    }
    return NULL;
}

// 0 = refused (dead, off the map, or +0x33B): the goal is deleted then (its vf0, as the game's callers do) - the
// caller must not touch it any more (a load goal: no parts from it)
internal int unit_goal(ee_unit *u, ee_goal *goal, int append, unsigned state) {
    if (!goal) return 0;
    if (unit_get_hp(u) <= 0 || unit_get_x(u) == -10000 || ((char)unit_get_frozen(u))) {
        ((void (__thiscall *)(void *, int))(*(void ***)goal)[0])(goal, 1);
        return 0;
    }
    unit_assign_goal(u, goal, append, 0);
    unit_set_state(u, state);
    ee_unit_type *td = unit_get_type_data(u);
    unit_type_goal_assigned(td, u);
    return 1;
}

// Art of Conquest's constructor takes a fourth argument, a byte it keeps at the goal's new +0x40: 0 as the AI's own
// calls pass it
internal ee_goal *goal_attack(ee_unit *u, ee_unit *target) {
    void *mem = goal_attack_alloc();
    if (!mem) return NULL;
    return goal_attack_init(mem, u, target);
}

// a transport's (or a fortress's) load goal (EEUGLoadTransport, engine-notes.md: new(0x68), 0x61487F with the units'
// vector - sorted in place) given to holder; NULL when it could not be made or holder refused it (deleted by
// unit_goal: no parts from it). Each unit to walk in then gets goal_load_part(g) - its vf4 product.
internal ee_goal *goal_load(ee_unit *holder, ee_unit **units, int n) {
    uint8_t *vec[4] = { 0, (uint8_t *)units, (uint8_t *)(units + n), (uint8_t *)(units + n) };
    void *mem = goal_load_alloc();
    ee_goal *g = mem ? goal_load_init(mem, holder, vec) : NULL;
    return g && unit_goal(holder, g, 0, 0x1F40) ? g : NULL;
}
internal ee_goal *goal_load_part(ee_goal *g) { return goal_part(g); }
// an unload goal at x, y (EEUGUnloadTransport: new(0x40), 0x615A1F; flag 1 as the AI passes it), not given yet
internal ee_goal *goal_unload(ee_unit *holder, int x, int y, int flag) {
    void *mem = goal_unload_alloc();
    return mem ? goal_unload_init(mem, holder, x, y, flag) : NULL;
}

// A GROUP OF THE GAME'S OWN with a target, as the scenario effect "Create AI Military Group" makes one (0x5AB8BE,
// docs/triggers.md) - an EECPAttackGroup whose subgroups march, fight and fall back as the AI's own attacks do (those on
// a player, mission 1, have no target and walk legs). Mission 7 (attack the goal unit); units by serial, ones the combat
// manager calls free (0x4707D8: in no group). Task (0x34, ctor 0x441FB3): +4 location (0x47C8B7 = location 0x18, ctor
// 0x47C4E8, point at +4), target unit via 0x442246, +0x20 target player, +0x28 mission, +0x2C 2, +0x30 1. Strategy
// (0x44, ctor 0x473BC4, reset 0x473CA7): +0x3C 4 attack / 5 defend. Then cm 0x47067A(task, strategy, vector<int> ids)
// - *ok - and [cm+0x48] 0x45106C(same). The pipe's aigroup, army.c's outdated units.
internal ee_attack_group *ag_launch(ee_player *pl, const int *serials, int n, ee_unit *target, int *ok, int *nsg) {
    ee_combat_mgr *cm = player_get_combat_mgr(pl);
    ee_attack_cmd *cmd = cm ? combat_mgr_get_commander(cm) : NULL;
    *ok = 0; *nsg = 0;
    if (!cmd || n <= 0 || !target) return NULL;
    ee_player *tpl = unit_get_owner(target);
    __attribute__((aligned(8))) uint8_t task_mem[0x40], loc_mem[0x20], strat_mem[0x50];
    ee_task *task = (ee_task *)task_mem; ee_strategy *strat = (ee_strategy *)strat_mem; ee_location *loc = (ee_location *)loc_mem;
    task_init(task);
    location_init(loc);
    strategy_init(strat);
    task_set_target(task, target);
    int pt[2] = { unit_get_x(target), unit_get_y(target) };
    point_set(loc_mem + 4, pt);
    location_set(task_location_at(task), loc);
    task_set_target_player(task, game_players()[player_get_index(tpl) & 15]);
    task_set_unknown_28(task, 7); task_set_unknown_2c(task, 2); task_set_unknown_30(task, 1);
    strategy_reset(strat);
    strategy_set_unknown_3c(strat, 4); strategy_set_unknown_40(strat, 0);
    // vector<int> {allocator, first, last, end} with the game's heap (its dtor 0x61ABEC frees with the game's delete)
    __attribute__((aligned(8))) uint8_t vec[16] = { 0 };
    int *ids = game_new((unsigned)n * 4);
    memcpy(ids, serials, (size_t)n * 4);
    FIELD(vec, 4, int *) = ids; FIELD(vec, 8, int *) = ids + n; FIELD(vec, 0xC, int *) = ids + n;
    *ok = combat_mgr_attack_plan_ok(cm, task, strat, vec) != 0;
    ee_attack_group *g = *ok ? attack_cmd_new_group(cmd, task, strat, vec) : NULL;
    vector_free(vec);
    task_free(task);
    strategy_free(strat);
    *nsg = g && attack_group_get_subgroups_begin(g) ? (int)(attack_group_get_subgroups_end(g) - attack_group_get_subgroups_begin(g)) : 0;
    return g;
}
internal ee_goal *goal_move(float x, float y) {
    float xy[2] = { x, y };
    void *mem = goal_move_alloc();
    return mem ? goal_move_init(mem, xy) : NULL;
}

// CITIZENS AND TOWERS (ageofai.ini [ai_base] avoid_towers_citizens, default 1). The game's site searches check for enemies
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
internal int nearest_drop(ee_player *pl, ivec2 p, int food, ivec2 *out);   // sites.c
internal int tower_covers(unsigned pi, float x, float y) {
    for (int k = 0; k < g_threat[pi].n; k++) {
        struct defence *d = &g_threat[pi].d[k];
        float r = d->range + 2.0f;
        if (vec2_dist2(v2(x, y), v2((float)d->x, (float)d->y)) <= r * r) return 1;
    }
    return 0;
}
internal int route_covered(ee_player *pl, int fx, int fy, int tx, int ty) {   // fx = -10000: the site only
    if (!pl) return 0;
    unsigned pi = (unsigned)player_get_index(pl) & 15;
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
internal int g_cit_threats = 1;   // [ai_base] citizen_threats (below, DAMAGE MEMORY)
internal int cv_mem_raider(ee_player *pl, ivec2 p, int r0, int rmax);   // cover.c threat_memory: a remembered one
internal void cv_mem_threats(ee_player *pl, unsigned pi);
internal int raiders_near(ee_player *pl, ivec2 p, int r0) {
    int rmax = g_cit_threats ? RAID_FAR : RAID_MAX;
    int r = r0 > rmax ? r0 : rmax;   // the search box; each unit then by its own reach
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid || !pl) return 0;
    for (struct grid_it it = grid_it_box(p.x, p.y, r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (!td || unit_get_hp(u) <= 0 || abs(unit_get_x(u) - p.x) > r || abs(unit_get_y(u) - p.y) > r) continue;
        int fam = unit_type_get_class_id(td), t = unit_type_get_id(td);
        if ((fam != CLS_LAND && fam != CLS_NAVAL && !(g_cit_threats && fam == CLS_SUBMERGED_NAVAL)) || is_citizen_type(t) || unit_type_get_attack(td) <= 0) continue;
        ee_unit_stats *st = unit_get_stats(u);
        float rng = st ? unit_stats_get_range(st) : unit_type_get_range(td);
        int reach = rng > 0.0f && rng < 30.0f ? (int)(rng + 0.5f) + RAID_MARGIN : 0;
        if (reach > rmax) reach = rmax;
        if (reach < r0) reach = r0;
        if (abs(unit_get_x(u) - p.x) > reach || abs(unit_get_y(u) - p.y) > reach) continue;
        ee_player *o = unit_get_owner(u);   // not nature (player 0: wild animals)
        if (o && player_get_index(o) > 0 && is_enemy(pl, o) && seen_by(u, pl)) return 1;
    }
    return 0;
}
// raiders_near or one it remembers there (threat_memory) - only where a citizen is sent: the builders' and gatherers'
// sites, the builders' orders, the way (its first runs had it in every use - shelters, the sites of towers and drop
// sites, the defence - and read worse head-to-head, unmirrored: docs/testing.md threat_memory)
internal int raiders_known(ee_player *pl, ivec2 p, int r0) {
    return raiders_near(pl, p, r0) || cv_mem_raider(pl, p, r0, g_cit_threats ? RAID_FAR : RAID_MAX);
}
// enemy soldiers on the way from x0,y0 to x1,y1 (every 4 tiles from the 5th, within 4) or within 6 of the end
internal int path_raided(ee_player *pl, int x0, int y0, int x1, int y1) {
    float dx = (float)(x1 - x0), dy = (float)(y1 - y0), l = sqrtf(dx * dx + dy * dy);
    for (float t = 5; t < l; t += 4)
        if (raiders_known(pl, iv2(x0 + (int)(dx / l * t), y0 + (int)(dy / l * t)), 4)) return 1;
    return raiders_known(pl, iv2(x1, y1), 6);
}
// where a citizen at x,y flees while the base is overrun: the rally point if the way is clear, else 12 tiles in the
// clear direction (of 8) nearest the rally point; none clear: the rally point after all
internal void flee_point(ee_player *pl, int x, int y, float rx, float ry, float *fx, float *fy) {
    *fx = rx; *fy = ry;
    if (!path_raided(pl, x, y, (int)rx, (int)ry)) return;
    float msz = (float)map_get_size(game_map()), best = 1e9f;
    static const float dir[8][2] = { { 1, 0 }, { .7071f, .7071f }, { 0, 1 }, { -.7071f, .7071f }, { -1, 0 },
                                     { -.7071f, -.7071f }, { 0, -1 }, { .7071f, -.7071f } };
    for (int k = 0; k < 8; k++) {
        float px = (float)x + dir[k][0] * 12, py = (float)y + dir[k][1] * 12;
        if (msz > 8) {
            px = px < 3 ? 3 : px > msz - 4 ? msz - 4 : px;
            py = py < 3 ? 3 : py > msz - 4 ? msz - 4 : py;
        }
        float d = vec2_dist2(v2(px, py), v2(rx, ry));
        if (d >= best || path_raided(pl, x, y, (int)px, (int)py)) continue;
        best = d; *fx = px; *fy = py;
    }
}
// DAMAGE MEMORY ([ai_base] citizen_threats, default 1; the playtester: "they should just stay away from the frontline, a
// single artillery in range can kill multiple citizens in a single shot; gold and iron might be worth some risk, but
// wood can be gathered elsewhere, and farms rebuilt at a safer spot - in the WW1 save lots of citizens gather wood right
// under my artillery and my planes"). raiders_near saw land units and ships within 15 tiles only, and nothing it
// can't see: artillery shooting from the fog, aircraft. With citizen_threats: the reach up to 27 tiles (ships of
// class 10 too), and every 4 s (citizens_safety) each citizen's hit points are compared with the last look - where
// one lost some, the spot is remembered for 2 minutes. A site within 6 tiles of such spots is dangerous: food and wood
// from one (they can be had elsewhere), stone, gold and iron from 3 (worth some risk). Aircraft count only where they
// do harm, so a town they merely fly over keeps working. The searches refuse such sites too (no sent back, sent again).
// Hunters' losses don't count (the animals). tests/scenarios/shelling.eeai, bombing.eeai (10 seeds, P3 off vs on): shelling
// alive 5 vs 7 of 10, workers 7.1 vs 17.6, losses 14.3 vs 10.8, buildings lost 6.1 vs 3.9; bombing workers 42.9 vs
// 54.7, losses 25.5 vs 21.0. Idle citizens rose (bombing 2.9 vs 6.3 on average): no safe site of their resource left.
#define HURT_N 64
internal struct { unsigned at; struct { int x, y; uint32_t t; } h[HURT_N]; } g_hurt[16];
internal struct { ee_unit *u; int hp; } g_cit_hp[16][512];
internal unsigned g_hurt_count[16];
internal void hurt_note(unsigned pi, ee_unit *u, uint32_t now, int mark) {   // remembers u's hit points; a loss marks its spot
    int hp = unit_get_hp(u);
    unsigned h = (unsigned)((uintptr_t)u >> 4) & 511;
    for (int k = 0; k < 512; k++) {
        typeof(&g_cit_hp[0][0]) e = &g_cit_hp[pi][(h + k) & 511];
        if (e->u == u) {
            if (hp < e->hp && mark) {
                typeof(&g_hurt[0].h[0]) r = &g_hurt[pi].h[g_hurt[pi].at++ % HURT_N];
                r->x = unit_get_x(u); r->y = unit_get_y(u); r->t = now ? now : 1;
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
    return hurt_near((unsigned)player_get_index(pl) & 15, x, y) >= (res >= 4 ? 3 : 1);
}
internal int site_danger_res(ee_player *pl, ee_unit *site, int food, int res) {   // from the nearest drop site
    if (!g_avoid_citizens || !pl || !site) return 0;
    int x = unit_get_x(site), y = unit_get_y(site);
    if (raiders_near(pl, iv2(x, y), 6)) return 1;   // the searches refuse a raided site too (else: sent back, sent again)
    if (hurt_danger(pl, x, y, res)) return 1;
    ivec2 d = iv2(-10000, 0);
    nearest_drop(pl, iv2(x, y), food, &d);
    return route_danger(pl, d.x, d.y, x, y);
}
internal int site_danger(ee_player *pl, ee_unit *site, int food) { return site_danger_res(pl, site, food, food ? 2 : -1); }
// DANGER MAP ([ai_base] danger_routes; game/danger.c): this AI's map of enemy reach, every 4 s (citizens_safety's tick) -
// what the checks above look at, as tiles: each enemy defence it has seen (g_threat) at its range + 2, each enemy
// soldier or warship it sees at its reach as raiders_near counts it (its range + RAID_MARGIN, up to RAID_FAR), each
// spot where its citizens were hurt in the last 2 minutes within 6. Its units that can't fight are routed round them
// (engine/shiproute.c).
internal unsigned g_dr_refreshed;   // tests: maps made
internal void danger_refresh(ee_player *pl, uint32_t now) {
    ee_map *map = game_map();
    if (!g_danger_routes || !pl || !map) return;
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    threat_refresh(pl, now);
    if (!danger_begin(pi, DANGER_THREATS, (int)map_get_size(map), now)) return;
    g_dr_refreshed++;
    for (int k = 0; k < g_threat[pi].n; k++) {
        struct defence *d = &g_threat[pi].d[k];
        danger_disc(pi, DANGER_THREATS, (float)d->x + 0.5f, (float)d->y + 0.5f, d->range + 2.0f);
    }
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (!td || unit_get_hp(u) <= 0) continue;
        int fam = unit_type_get_class_id(td);
        if ((fam != CLS_LAND && fam != CLS_NAVAL && fam != CLS_SUBMERGED_NAVAL) || is_citizen_type(unit_type_get_id(td)) || unit_type_get_attack(td) <= 0) continue;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !seen_by(u, pl)) continue;
        ee_unit_stats *st = unit_get_stats(u);
        float rng = st ? unit_stats_get_range(st) : unit_type_get_range(td);
        float reach = rng > 0.0f && rng < 30.0f ? rng + RAID_MARGIN : RAID_MARGIN;
        if (reach > RAID_FAR) reach = RAID_FAR;
        vec3 q = unit_get_pos(u);
        danger_disc(pi, DANGER_THREATS, q.x, q.y, reach);
    }
    cv_mem_threats(pl, pi);   // threat_memory: those it remembers
    for (int k = 0; k < HURT_N; k++) {
        typeof(&g_hurt[0].h[0]) r = &g_hurt[pi].h[k];
        if (r->t && now >= r->t && now - r->t <= 120000) danger_disc(pi, DANGER_THREATS, (float)r->x + 0.5f, (float)r->y + 0.5f, 6.0f);
    }
    static uint32_t logged[16];
    if (g_log && (!logged[pi] || now / 300000 != logged[pi] / 300000)) {   // the first, then every 5 minutes
        logged[pi] = now;
        fprintf(g_log, "# danger: P%u at %.1f min: %d tiles in enemy reach (%d defences known), %d where its fight is lost; all players: %u routes by a danger map, "
                "%u danger tiles on them, %u soldiers' routes round lost fights, %u marches stopped at their edge, %u enemies in them not chosen as targets\n", pi, now / 60000.0, danger_count(pi, DANGER_THREATS),
                g_threat[pi].n, danger_count(pi, DANGER_LOST), g_dr_routes, g_dr_tiles, g_dr_lost_routes, g_dr_legs_held, g_lost_skips);
    }
    danger_overlay(pi, now);
}
internal ee_goal *goal_move(float x, float y);
internal int unit_goal(ee_unit *u, ee_goal *goal, int append, unsigned state);
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
internal void shelf_tick(ee_player *pl, unsigned pi, uint32_t now);   // bunker.c
internal int g_bk_res;
internal void citizens_safety(ee_resource_mgr *rm) {   // before the citizen assignment
    ee_player *pl = resource_mgr_player(rm);
    if (!pl) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    static uint32_t next[16], logged[16];
    uint32_t now = game_time();
    if (now < next[idx] && next[idx] - now <= 4000) return;
    next[idx] = now + 4000;
    danger_refresh(pl, now);
    if (!g_avoid_citizens) return;
    threat_refresh(pl, now);
    int sent = 0, fled = 0, aside = 0, stray = 0, kinds[4][2] = { { 0 } };   // site types sent back from (log)
    int doing[7][4] = { { 0 } };   // per list: gathering, building, walking, nothing (the minute's log)
    float rx, ry;
    int mus = muster_rally(idx, &rx, &ry), sheltered = 0;
    if (g_bunkers || g_bk_res) bk_refresh(pl, idx);
    for (int r = 0; r < 7; r++) {
        uint8_t *list = resource_mgr_list_at(rm, r);
        for (unsigned k = 0, first = deque_front(list), cnt = deque_count(list); k < cnt; k++) {
            ee_unit *u = deque_at(list, first + k);
            ee_goal *goal = u ? unit_get_goal(u) : NULL;
            if (!u || unit_get_hp(u) <= 0) continue;
            {
                const char *gd = goal ? rtti_name(goal) : "";
                doing[r][!goal ? 3 : !strcmp(gd, "EEUGGather@@") ? 0 : !strcmp(gd, "EEUGBuildRepair@@") ? 1 :
                         goal_is_location(goal) ? 2 : 3]++;
            }
            if (g_cit_threats) {   // not hunters: the animals hurt them (a farm is a building, CLS_BUILDING)
                ee_unit *gs = goal && !strcmp(rtti_name(goal), "EEUGGather@@") ? goal_get_site(goal) : NULL;
                hurt_note(idx, u, now, !(r == 2 && gs && unit_type_get_class_id(unit_get_type_data(gs)) != CLS_BUILDING));
            }
            if (r == 0 && goal && goal_is_location(goal)) {   // a stray: an attack's leftover order
                float gx = goal_get_dest_x(goal), gy = goal_get_dest_y(goal);
                int dest[2] = { (int)gx, (int)gy };
                ivec2 d;
                int rally = mus && fabsf(gx - rx) < 2 && fabsf(gy - ry) < 2;
                if (!rally && building_at(pl, dest, 6) &&
                    (!nearest_drop(pl, iv2(dest[0], dest[1]), 0, &d) || ivec2_dist2(d, iv2(dest[0], dest[1])) > 225) &&
                    nearest_drop(pl, unit_get_tile(u), 0, &d) &&
                    unit_goal(u, goal_move((float)d.x + 0.5f, (float)d.y + 0.5f), 0, 0x1F40)) {
                    stray++;
                    continue;
                }
            }
            if (mus) {   // the base is overrun: citizens near the enemy (or working near it) flee to the rally point
                int ux = unit_get_x(u), uy = unit_get_y(u);
                if (vec2_dist2(v2((float)ux, (float)uy), v2(rx, ry)) <= 36) continue;   // there already
                const char *gn0 = goal ? rtti_name(goal) : "";
                ee_unit *st = !goal ? NULL : !strcmp(gn0, "EEUGBuildRepair@@") ? goal_get_building(goal) :
                              !strcmp(gn0, "EEUGGather@@") ? goal_get_site(goal) : NULL;
                int walk = goal && goal_is_location(goal), danger;
                if (walk)   // on its way somewhere: only if enemy soldiers stand on the way or at the end
                    danger = path_raided(pl, ux, uy, (int)goal_get_dest_x(goal), (int)goal_get_dest_y(goal));
                else
                    danger = raiders_near(pl, iv2(ux, uy), 8) || (st && raiders_near(pl, unit_get_tile(st), 8));
                if (danger && g_bunkers && bk_shelter(pl, u)) { sheltered++; continue; }
                if (danger) {
                    float fx, fy;
                    flee_point(pl, ux, uy, rx, ry, &fx, &fy);
                    if (walk && fabsf(goal_get_dest_x(goal) - fx) < 2 && fabsf(goal_get_dest_y(goal) - fy) < 2) continue;
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
            ee_unit *site = build ? goal_get_building(goal) : goal_get_site(goal);
            ivec2 d;
            if (!site || !nearest_drop(pl, unit_get_tile(u), r == 2, &d)) continue;
            int raided = raiders_known(pl, unit_get_tile(site), build ? 8 : 6);
            // builders near known towers only: only of fields (walls and towers at the front: the wall manager sends
            // them again and again)
            if (build && !raided && unit_type_get_id(unit_get_type_data(site)) != UT(B_FARM)) continue;
            if (!build && !raided && hurt_danger(pl, unit_get_x(site), unit_get_y(site), r)) raided = 1;   // hurt there
            if (!raided && (build || !site_danger_res(pl, site, r == 2, r))) continue;
            if (g_bunkers && bk_shelter(pl, u)) { sheltered++; continue; }   // under fire: into a fortress
            if (raided && abs(d.x - unit_get_x(site)) <= 3 && abs(d.y - unit_get_y(site)) <= 3) continue;   // at home already
            if (!unit_goal(u, goal_move((float)d.x + 0.5f, (float)d.y + 0.5f), 0, 0x1F40)) continue;
            sent++;
            int st = unit_type_get_id(unit_get_type_data(site)), j = 0;
            while (j < 4 && kinds[j][1] && kinds[j][0] != st) j++;
            if (j < 4) { kinds[j][0] = st; kinds[j][1]++; }
        }
    }
    g_cit_stats[idx].sent += sent + fled;
    if (g_bunkers) bk_flush(pl, idx, now);
    if (g_bunkers || g_bk_res) { bk_build(pl, idx, now); bk_reserve_tick(pl, idx, now); shelf_tick(pl, idx, now); }
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
            ee_unit_type *td = it.td;
            if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || !is_citizen_type(unit_type_get_id(td))) continue;
            ee_goal *g = unit_get_goal(u);
            const char *gd = g ? rtti_name(g) : "";
            int k = !g ? 5 : !strcmp(gd, "EEUGGather@@") ? 0 : !strcmp(gd, "EEUGBuildRepair@@") ? 1 :
                    goal_is_location(g) ? 2 : goal_is_attack(g) ? 3 : 4;
            all[k]++;
            if (k == 4 && !other[0]) lstrcpynA(other, gd, sizeof other);
        }
        {   // the population as the buildings' start check sees it (0x624DF8: cap vf31 < population (1, 0) + the unit -> wait)
            ee_civ_mgr *cmp = player_get_civ_mgr(pl); void *ump = cmp ? civ_mgr_get_unit_mgr(cmp) : NULL;
            uint8_t *v0 = player_get_list_begin(pl), *v1 = player_get_list_end(pl);
            fprintf(g_log, "# popcheck: P%u at %.1f min: cap %d, population %d (with production %d, list +0x18 %d), +0xB10 %d, +0xB1C %d +0xB20 %d, limit %d, AI's in production %d (queued %d); ordered with no room by call site 46cce6/490cac/490e12/490f1f/490fa9/mod: %u/%u/%u/%u/%u/%u\n",
                    idx, now / 60000.0, unit_limit(pl),
                    player_population(pl, 0, 0), player_population(pl, 1, 0),
                    v0 && v1 ? (int)((v1 - v0) / 4) : -1, player_get_pop_b10(pl), player_get_pop_b1c(pl), player_get_pop_b20(pl), pop_limit_setting(),
                    ump ? unit_mgr_get_in_production(ump) : -1, units_queued(pl), g_noroom[idx][0], g_noroom[idx][1], g_noroom[idx][2], g_noroom[idx][3], g_noroom[idx][4], g_noroom[idx][5]);
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
internal void __thiscall hook_repair_goal(ee_unit *u, ee_goal *goal, int8_t append, int prio) {
    ee_player *pl = u ? unit_get_owner(u) : NULL;
    ee_unit *b = goal && goal_is_build(goal) ? goal_get_building(goal) : NULL;
    if (g_avoid_citizens && pl && b && !is_vanilla(pl) && raiders_known(pl, unit_get_tile(b), 8)) {
        g_repair_refused[(unsigned)player_get_index(pl) & 15]++;
        ((void (__thiscall *)(void *, int))(*(void ***)goal)[0])(goal, 1);
        return;
    }
    unit_assign_goal(u, goal, append, prio);
}

// Known defences covering tile (x, y) by their real range (+1), not counting the one at (ex, ey).
internal int covered(unsigned pi, float x, float y, int ex, int ey) {
    for (int k = 0; k < g_threat[pi].n; k++) {
        struct defence *d = &g_threat[pi].d[k];
        if (d->x == ex && d->y == ey) continue;
        float r = d->range + 1.5f;
        if (vec2_dist2(v2(x, y), v2((float)d->x, (float)d->y)) <= r * r) return 1;
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
    uint8_t *dq = build_mgr_queue_at(bm, e);
    unsigned first = deque_front(dq), n = deque_count(dq), msz = deque_map_size(dq);
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
        ee_unit_type *td = it.td;
        if (td && unit_get_owner(u) == pl && unit_type_get_id(td) == UT(B_SIEGE_FACTORY)) { factory = 1; break; }
    }
    if (!factory) {
        // the build manager (cm +0x3C) works through per-epoch queues (+4 + epoch*0x14, 12-byte entries {type, x, y};
        // 0,0 = it picks the place) every 3 s (0x468540); 0x468C37 appends an entry. Queue a Siege Factory.
        g_siege_train[pi].no_factory++;
        void *cm = player_get_civ_mgr(pl);
        ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
        int e = current_epoch(pl);
        if (bm && e >= 0 && e < 14 && ut_has(UT(B_SIEGE_FACTORY)) && !bq_has(bm, UT(B_SIEGE_FACTORY)) &&
            (!g_siege_train[pi].factory_req || now - g_siege_train[pi].factory_req >= 300000)) {
            g_siege_train[pi].factory_req = now ? now : 1;
            int entry[3] = { UT(B_SIEGE_FACTORY), 0, 0 };
            build_mgr_queue_add(bm, e, entry);
            if (g_log) fprintf(g_log, "# siege: P%u at %.1f min queues a Siege Factory (wants %d siege, has %d)\n", pi, now / 60000.0, want, have);
            ai_say(pl, CHAT_SIEGE, "[ai] need siege (%d of %d): queued a Siege Factory", have, want);
        }
        return;
    }
    void *cm = player_get_civ_mgr(pl), *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    if (!um) return;
    int epoch = current_epoch(pl) + 1;   // tech tree epoch 0 = Prehistoric; tech ids count from 1
    for (size_t k = 0; k < sizeof k_siege_types / sizeof *k_siege_types; k++) {
        if (k_siege_types[k].epoch > epoch || !ut_has(g_ut[k_siege_types[k].type])) continue;
        int req[4] = { g_ut[k_siege_types[k].type], want - have, 0, 0 };
        unsigned got = 0;
        g_priority_mil = 1;
        hook_train_units(um, req, (uintptr_t)&got);
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
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    if (!g_siege || (now >= g_siege_st[pi].last && now - g_siege_st[pi].last < 2000)) return;
    g_siege_st[pi].last = now;
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid) return;
    // own siege weapons
    enum { MAXS = 24, MAXE = 48 };
    ee_unit *siege[MAXS]; int ns = 0; float sx = 0, sy = 0, srange = 99;
    for (struct grid_it it = grid_it_all(1); ns < MAXS && grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (!td || unit_get_owner(u) != pl || !is_siege(unit_type_get_id(td)) || unit_get_hp(u) <= 0) continue;
        ee_unit_stats *st = unit_get_stats(u);
        float r = st ? unit_stats_get_range(st) : unit_type_get_range(td);
        if (r < srange) srange = r;
        siege[ns++] = u; sx += (float)unit_get_x(u); sy += (float)unit_get_y(u);
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
        float d = vec2_dist2(v2((float)g_breaches[k].x, (float)g_breaches[k].y), v2(sx, sy));
        if (w && d < best) { best = d; t = w; }
    }
    if (!t) {
        threat_refresh(pl, now);
        for (int k = 0; k < g_threat[pi].n; k++) {
            struct defence *d = &g_threat[pi].d[k];
            if (d->range + 0.5f >= srange) continue;
            float dd = vec2_dist2(v2((float)d->x, (float)d->y), v2(sx, sy));
            if (dd < best) { best = dd; t = unit_at(d->x, d->y, d->type); }
        }
    }
    if (!t) return;
    int tx = unit_get_x(t), ty = unit_get_y(t);
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
    g_siege_st[pi].tx = tx; g_siege_st[pi].ty = ty; g_siege_st[pi].ttype = unit_type_get_id(unit_get_type_data(t));
    g_siege_st[pi].px = (int)px; g_siege_st[pi].py = (int)py;
    // WAVES: every siege weapon used to be sent to the firing position the moment it existed - one by one, without
    // escorts, through the player's army camped outside the town (user's game: "1 ordered, 0 escorts" again and
    // again, 45 Howitzers of P3 lost). Now they gather at the town (the drop site nearest the target, 3 tiles back)
    // and go together: at least 3 (or all it wants, if fewer), at least 2 idle soldiers along, and no enemy soldiers
    // at the firing position or on the way there. While a wave is out (siege attacking the target or on the way),
    // newcomers wait for the next one.
    ivec2 d;
    if (!nearest_drop(pl, iv2(tx, ty), 0, &d)) return;
    float gx = (float)d.x - (float)tx, gy = (float)d.y - (float)ty, gl = sqrtf(gx * gx + gy * gy);
    if (gl > 0) { gx = (float)d.x + 0.5f + gx / gl * 3.0f; gy = (float)d.y + 0.5f + gy / gl * 3.0f; }
    else { gx = (float)d.x + 0.5f; gy = (float)d.y + 0.5f; }
    ee_unit *out[MAXS];
    ee_unit *home[MAXS];
    ee_unit *away[MAXS]; int nout = 0, nhome = 0, naway = 0;
    for (int i = 0; i < ns; i++) {
        ee_unit *u = siege[i];
        ee_goal *g = unit_get_goal(u);
        float ox = (float)unit_get_x(u) + 0.5f - px, oy = (float)unit_get_y(u) + 0.5f - py;
        float hx = (float)unit_get_x(u) + 0.5f - gx, hy = (float)unit_get_y(u) + 0.5f - gy;
        int committed = (g && goal_is_attack(g) && goal_get_target(g) == t) || ox * ox + oy * oy <= 9.0f ||
                        (g && goal_is_location(g) && fabsf(goal_get_dest_x(g) - px) < 2 && fabsf(goal_get_dest_y(g) - py) < 2);
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
        ee_goal *g = unit_get_goal(u);
        if (g && goal_is_attack(g) && goal_get_target(g) == t) continue;
        float ox = (float)unit_get_x(u) + 0.5f - px, oy = (float)unit_get_y(u) + 0.5f - py;
        if (ox * ox + oy * oy <= 4.0f) unit_goal(u, goal_attack(u, t), 0, 0x1F40);
    }
    g_siege_st[pi].wave = nout > 0;
    // the rest to the gathering point (not those walking there)
    for (int i = 0; i < naway; i++) {
        ee_unit *u = away[i];
        ee_goal *g = unit_get_goal(u);
        if (g && goal_is_location(g) && fabsf(goal_get_dest_x(g) - gx) < 2 && fabsf(goal_get_dest_y(g) - gy) < 2) continue;
        unit_goal(u, goal_move(gx, gy), 0, 0x1F40);
    }
    const char *hold = NULL;
    if (g_siege_st[pi].wave) hold = "a wave is out";
    else if (nhome < need) hold = "gathering";
    else if (raiders_near(pl, iv2((int)px, (int)py), 8) || path_raided(pl, (int)gx, (int)gy, (int)px, (int)py)) hold = "enemy soldiers on the way";
    // escorts: idle soldiers (no goal) along with a wave
    int want = nhome * 3 / 2 < 4 ? 4 : nhome * 3 / 2, escorts = 0, idle = 0;
    for (struct grid_it it = grid_it_all(1); idle < 2 && grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (!td || unit_get_owner(u) != pl || unit_get_goal(u) || unit_get_hp(u) <= 0) continue;
        int fam = unit_type_get_class_id(td);
        idle += fam == CLS_LAND && is_soldier(td);   // land soldiers (is_soldier: no citizens, siege, rams - the ram 158 was missed)
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
        ee_unit_type *td = it.td;
        if (unit_get_owner(u) != pl || unit_get_goal(u) || unit_get_hp(u) <= 0) continue;
        int fam = unit_type_get_class_id(td);
        if (fam != CLS_LAND || !is_soldier(td)) continue;   // land soldiers only (no citizens, siege, rams - 158 too)
        float ex = (float)unit_get_x(u) - px, ey = (float)unit_get_y(u) - py;
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
internal struct { ee_attack_subgroup *sg; int state, goal_x, goal_y; } g_trace[64];
internal void asg_trace_tick(ee_player *pl, uint32_t now) {
    unsigned p = (unsigned)player_get_index(pl) & 15;
    if (!(g_trace_mask & (1u << p)) || !g_log) return;
    ee_combat_mgr *cbm = player_get_combat_mgr(pl); ee_attack_cmd *cmd = cbm ? combat_mgr_get_commander(cbm) : NULL;
    if (!cmd) return;
    uint8_t *dq = attack_cmd_groups_at(cmd);
    unsigned first = deque_front(dq), cnt = deque_count(dq);
    for (unsigned k = 0; k < cnt && k < 32; k++) {
        ee_attack_group *g = deque_at(dq, first + k);
        for (ee_attack_subgroup **sp = g ? attack_group_get_subgroups_begin(g) : NULL; sp && sp < attack_group_get_subgroups_end(g); sp++) {
            ee_attack_subgroup *sg = *sp;
            if (!sg) continue;
            int st = attack_subgroup_get_state(sg), gx = attack_subgroup_get_goal_x(sg), gy = attack_subgroup_get_goal_y(sg), slot = -1, fre = -1;
            for (int q = 0; q < 64; q++) {
                if (g_trace[q].sg == sg) { slot = q; break; }
                if (!g_trace[q].sg && fre < 0) fre = q;
            }
            if (slot >= 0 && g_trace[slot].state == st && g_trace[slot].goal_x == gx && g_trace[slot].goal_y == gy) continue;
            if (slot < 0) { if (fre < 0) continue; slot = fre; g_trace[slot].sg = sg; g_trace[slot].state = -1; }
            int n = 0, walk = 0, fight = 0, other = 0, none = 0, engaged = 0;
            float cx = 0, cy = 0;
            for (ee_asg_unit **r = attack_subgroup_get_units_begin(sg); r && r < attack_subgroup_get_units_end(sg) && n < 1000; r++) {
                ee_unit *u = *r ? asg_unit_get_unit(*r) : NULL;
                ee_goal *gl;
                if (!u || unit_get_hp(u) <= 0) continue;
                if (asg_unit_get_engaged(*r)) engaged++;
                n++; cx += (float)unit_get_x(u); cy += (float)unit_get_y(u);
                gl = unit_get_goal(u);
                if (!gl) none++;
                else if (goal_is_location(gl)) walk++;
                else if (goal_is_attack(gl)) fight++;
                else other++;
            }
            fprintf(g_log, "# asgtrace: P%u at %.2f min: group %u %s %p: state %d -> %d mission %d goal %d,%d; %d alive at %.0f,%.0f, "
                    "%u waypoints, %d engaged; %d march %d fight %d other %d idle", p, now / 60000.0, k, rtti_name(sg),
                    (void *)sg, g_trace[slot].state, st, attack_subgroup_get_mission(sg), gx, gy, n, n ? cx / n : 0.0f, n ? cy / n : 0.0f,
                    attack_subgroup_get_waypoints(sg), engaged, walk, fight, other, none);
            for (int w = 0; w < 2; w++) {   // the subgroup's target unit (+0x118) and the group's (+0x28 + 0x1C)
                ee_unit *t = w ? attack_group_get_target_unit(g) : attack_subgroup_get_target_unit(sg);
                ee_unit_type *ttd = t ? unit_get_type_data(t) : NULL;
                if (ttd) fprintf(g_log, "; %s target type %d at %d,%d hp %d", w ? "group" : "its", unit_type_get_id(ttd),
                                 unit_get_x(t), unit_get_y(t), unit_get_hp(t));
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

// A NEW TARGET FOR A WAVE ON A PLAYER WHO IS OUT ([ai_base] attack_retarget): a group's target player (+0x48, its
// subgroups' +0x11C) is chosen once at the launch and never again - on the player's game (2026-10-28, minute 642) P6's
// main wave, 400 soldiers in some 40 subgroups of 10, still attacked P4, out by then: with no building of P4 left its
// subgroups walked the game's legs for a player (home, coast points, a wait of 1-3 minutes at each), the ones past the
// player's town held at the staging point a few at a time, and the wave never pushed. Every 10 s a group whose target
// is out or no enemy any more gets the living enemy with the known building (walls aside) nearest the group's middle;
// the subgroups plan their next legs for it. "# retarget:" lines. Measured on a copy of that save (638-670 min; runs
// from a save are exact): P6's routes went to P1 (468, before 6; to P4 0, before 463), its pushes sent 103 soldiers
// (before 45), but it lost 214 units (before 149) for 141 kills (125) and 15 buildings (17) - so off. With wave_focus
// too: 253 buildings, 4151 kills (wave_focus alone razed P4's leftover buildings: 298). The wave still reaches the
// stage a subgroup at a time (10-60 marches a minute pass it), so the pushes stay small.
internal int g_attack_retarget = 0;
struct rt_pick { ee_player *pl; ivec2 at; ee_unit *best; long bd; };
internal void rt_pick_unit(ee_unit *u, void *ctx) {
    struct rt_pick *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    ee_player *o = unit_get_owner(u);
    if (!td || !o || player_get_index(o) <= 0 || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !is_enemy(s->pl, o) || is_out(o)) return;
    int fam = unit_type_get_class_id(td);
    if ((fam != CLS_BUILDING && fam != CLS_FORTRESS) || is_wall(unit_type_get_id(td)) || !explored_by(u, s->pl)) return;
    long d = ivec2_dist2(unit_get_tile(u), s->at);
    if (!s->best || d < s->bd) { s->best = u; s->bd = d; }
}
internal void wave_retarget(ee_player *pl, uint32_t now) {
    static uint32_t last[16];
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    if (!g_attack_retarget) return;
    if (now < last[idx]) last[idx] = 0;   // a new game
    if (last[idx] && now - last[idx] < 10000) return;
    last[idx] = now ? now : 1;
    ee_combat_mgr *cbm = player_get_combat_mgr(pl); ee_attack_cmd *cmd = cbm ? combat_mgr_get_commander(cbm) : NULL;
    if (!cmd) return;
    uint8_t *dq = attack_cmd_groups_at(cmd);
    for (unsigned k = 0, first = deque_front(dq), cnt = deque_count(dq); k < cnt && k < 64; k++) {
        ee_attack_group *g = deque_at(dq, first + k);
        ee_player *old = g ? attack_group_get_target(g) : NULL;
        if (!old || (!is_out(old) && is_enemy(pl, old))) continue;
        long sx = 0, sy = 0;
        int n = 0, nsg = 0;
        for (ee_attack_subgroup **sp = attack_group_get_subgroups_begin(g); sp && sp < attack_group_get_subgroups_end(g); sp++)
            for (ee_asg_unit **r = *sp ? attack_subgroup_get_units_begin(*sp) : NULL; r && r < attack_subgroup_get_units_end(*sp); r++) {
                ee_unit *u = *r ? asg_unit_get_unit(*r) : NULL;
                if (!u || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
                sx += unit_get_x(u); sy += unit_get_y(u); n++;
            }
        if (!n) continue;
        struct rt_pick s = { pl, iv2((int)(sx / n), (int)(sy / n)), NULL, 0 };
        grid_each(rt_pick_unit, &s);
        if (!s.best) continue;
        ee_player *nt = game_players()[player_get_index(unit_get_owner(s.best)) & 15];
        attack_group_set_target(g, nt);
        for (ee_attack_subgroup **sp = attack_group_get_subgroups_begin(g); sp && sp < attack_group_get_subgroups_end(g); sp++)
            if (*sp && attack_subgroup_get_target(*sp) == old) { attack_subgroup_set_target(*sp, nt); nsg++; }
        if (g_log) fprintf(g_log, "# retarget: P%u at %.1f min: group %p on P%d (%s) -> P%d, its type %d at %d,%d nearest to the group's middle %d,%d - %d "
                           "members, %d subgroups\n", idx, now / 60000.0, (void *)g, player_get_index(old), is_out(old) ? "out" : "no enemy",
                           player_get_index(nt), unit_type_get_id(unit_get_type_data(s.best)), unit_get_x(s.best), unit_get_y(s.best),
                           s.at.x, s.at.y, n, nsg);
    }
}

internal void attack_log(ee_player *pl, uint32_t now);
internal void attack_tick(ee_player *pl, uint32_t now) {
    wave_retarget(pl, now);
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
        unsigned i = (unsigned)player_get_index(pl) & 15;
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
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    if (g_log && now / 600000 != last_stats[pi] / 600000) {
        // EEPlayerStatistics (player+0x4A8): series i at +0x18 + i*0x18 {?, begin, end, ..} of 8-byte samples,
        // latest value = first dword of the last sample (0x55DCD3)
        fprintf(g_log, "# battle: P%u at %u min:", pi, now / 60000);
        ee_player_stats *st = player_stats_at(pl);
        for (int k = 0; k < 30; k++) {
            uint8_t *b = player_stats_get_begin(st, k), *e = player_stats_get_end(st, k);
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
internal void __thiscall hook_group_stat(void *stats, ee_unit *unit) {
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
internal void asg_purge(ee_attack_subgroup *sg) {
    ee_asg_unit **b = attack_subgroup_get_units_begin(sg); ee_asg_unit **e = attack_subgroup_get_units_end(sg); ee_asg_unit **w = b;
    if (!b || e < b || e - b > 4096) return;
    for (ee_asg_unit **r = b; r < e; r++)
        if (*r && asg_unit_get_unit(*r)) *w++ = *r;
    if (w == e) return;
    attack_subgroup_set_units_end(sg, w);
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
    ee_combat_mgr *cbm = pl ? player_get_combat_mgr(pl) : NULL; ee_attack_cmd *cmd = cbm ? combat_mgr_get_commander(cbm) : NULL;
    if (!cmd) return;
    uint8_t *dq = attack_cmd_groups_at(cmd);
    unsigned first = deque_front(dq), cnt = deque_count(dq);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        ee_attack_group *g = deque_at(dq, first + k);
        for (ee_attack_subgroup **sp = g ? attack_group_get_subgroups_begin(g) : NULL; sp && sp < attack_group_get_subgroups_end(g); sp++)
            if (*sp) asg_purge(*sp);
    }
}
internal void __thiscall hook_asg_stats(ee_attack_subgroup *sg) {
    asg_purge(sg);
    attack_subgroup_stats(sg);
}
internal void __thiscall hook_asg_land_stats(ee_attack_subgroup *sg) {
    asg_purge(sg);
    attack_subgroup_land_stats(sg);
}

// The same family, now reproducible: the reference save, P2's attack called off at 86.6 min (attack_retreat, or the
// test command calloff 2 1 - with and without honest_vision or landings) crashed every time. The call-off
// (0x43033E -> 0x46FD01) hands each unit of the attack back, and each one is offered to the attack groups at once
// (0x483ADE -> 0x4824BF -> 0x450E07 -> 0x453E10) - including groups the same call-off has just freed (their subgroup
// vectors garbage, a jump to 0). While the mod calls attacks off (g_in_calloff, diplo.c) no unit is offered: "not
// taken" (the caller only notes it), the unit stays free and the game assigns it on its next pass.
internal int g_in_calloff;
internal unsigned g_calloff_skips;
internal char __thiscall hook_unit_to_group(ee_attack_cmd *cmd, ee_unit *unit) {
    if (g_in_calloff) { g_calloff_skips++; return 0; }
    return attack_cmd_unit_to_group(cmd, unit);
}

// UNIT LIST: every unit - also the PathPoint markers (type 223) of an attack's route - sits in one list sorted by tile
// x (unit +8 -> node {next, prev}; the head the RTSDummyUnit at 0x918314, its +4 the first), re-sorted at each move
// (0x616F9D -> 0x5310B3) and walked by range searches (0x62FE01). The call-off frees a subgroup's PathPoints
// (EECPAttackSubGroup vf26 0x45B4FB -> the unit pool's release, EEComplexUnit vf13 0x5ED71E -> vf12 -> 0x628BC6) while
// they are still in that list (mIsValid +0x2B already 0, so the destructor's check stays quiet): the node is deleted,
// the neighbours keep pointing at the unit, the pool hands the memory out again (an arrow, a stone ball) and it is
// linked in elsewhere - the list gets a loop (the stalls: 0x5310B3 / 0x62FE01 spin for good) or a NULL link (crashes
// at 0x628D6F, 0x53110D, 0x531131). Older than the 2026-10-17 review (the DLL before it stalled too), set off by the
// mod's own call-offs at the safe point (attack_retreat then, citizen attacks, diplomacy). Guard: the node delete of both
// destructor parts (0x628BE7 in 0x628BC6, 0x628B7F in 0x628B58) first takes a unit still linked out of the list with the
// game's own unlink (0x628E29: the neighbours, or the head, get its links).
internal unsigned g_unlinked;
__attribute__((used)) void __cdecl unit_node_free(uint8_t **node, ee_unit *u) {
    if (!node || (!node[0] && !node[1]) || ((uint8_t **)unit_get_list_node(u)) != node) return;
    unit_unlink(u);
    if (g_log && g_unlinked++ < 5) {
        ee_unit_type *td = unit_get_type_data(u);
        fprintf(g_log, "# engine guard: a unit (type %d) freed while still in the unit list - taken out at %.1f min\n",
                td ? unit_type_get_id(td) : -1, game_time() / 60000.0);
    }
}
__attribute__((used)) uintptr_t g_op_delete;   // game_op_delete(), set with the hooks
__attribute__((naked, used)) void unit_node_free_thunk(void) {   // at the call of operator delete(node), ESI = the unit
    __asm__ volatile (
        "pushal\n\t"
        "pushl %esi\n\t"
        "pushl 40(%esp)\n\t"          // the node (the pushed argument: 32 pushal + 4 esi + 4 return address)
        "call _unit_node_free\n\t"
        "addl $8, %esp\n\t"
        "popal\n\t"
        "movl _g_op_delete, %eax\n\t"  // data_unit_fn_op_delete (eax is free: the delete's result is not used)
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
internal void __thiscall hook_unit_release(ee_unit *u) {
    int again = !unit_get_refs(u) && unit_get_released(u) == 1 && !unit_get_list_node(u), live = unit_get_valid(u) != 0;
    if (again || live) {
        if (g_log && g_release_again++ < 10) {
            ee_unit_type *td = unit_get_type_data(u);
            fprintf(g_log, "# engine guard: a unit (type %d) %s - not given back to the pool at %.1f min\n", td ? unit_type_get_id(td) : -1,
                    again ? "released a second time" : "released while still in the game", game_time() / 60000.0);
        }
        return;
    }
    ee_complex_unit_release(u);   // the class's own (the slot holds this hook)
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
internal char __thiscall hook_goal_cast(ee_goal *goal, void *unit) {
    if (goal) return goal_fits(goal, unit);
    if (g_log && g_goal_cast_null++ < 5)
        fprintf(g_log, "# engine guard: a unit's goal 0xFBA without its goal object (type %d) at %.1f min - skipped\n",
                unit && unit_get_type_data(unit) ? unit_type_get_id(unit_get_type_data(unit)) : -1, game_time() / 60000.0);
    return 0;
}
internal void attack_install(void) {
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
    g_avoid_towers = g_set_ai[1].avoid_towers;
    g_avoid_citizens = g_set_ai[1].avoid_towers_citizens;
    g_cit_threats = g_set_ai[1].citizen_threats;
    g_danger_routes = g_set_ai[1].danger_routes;
    g_avoid_lost = g_set_ai[1].avoid_lost_fights;
    g_bunkers = g_set_ai[1].bunkers;
    g_bk_res = g_set_ai[1].bunker_reserve;
    BOOL repair = patch_repair_goal(hook_repair_goal);
    if (g_log) fprintf(g_log, "# repairs under attack: refused (hook %s)\n", repair ? "ok" : "FAILED");
    g_avoid_weight = g_set_ai[1].avoid_towers_weight;
    g_avoid_margin = g_set_ai[1].avoid_towers_margin;
    g_weak_spots = g_set_ai[1].attack_weak_spots;
    g_weak_radius = g_set_ai[1].attack_weak_spot_radius;
    g_leash = g_set_ai[1].attack_leash;
    g_citizen_attacks = g_set_ai[1].citizen_attacks;
    g_breach = g_set_ai[1].attack_breach;
    g_siege = g_set_ai[1].attack_siege;
    g_ignore_air = g_set_ai[1].attack_ignore_air;
    g_attack_retarget = g_set_ai[1].attack_retarget;
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
    ee_combat_mgr *cbm = pl ? player_get_combat_mgr(pl) : NULL; ee_attack_cmd *cmd = cbm ? combat_mgr_get_commander(cbm) : NULL;
    if (!cmd) { REPLY("error: no AI player %d with an attack commander\n", p); return; }
    uint8_t *dq = attack_cmd_groups_at(cmd);
    unsigned first = deque_front(dq), cnt = deque_count(dq);
    REPLY("P%d: %u attack groups, next attack at %.1f min\n", p, cnt, player_get_next_attack(pl) / 60000.0);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        ee_attack_group *g = deque_at(dq, first + k);
        ee_player *target = g ? attack_group_get_target(g) : NULL;
        if (!g) continue;
        int ns = 0;
        for (ee_attack_subgroup **sp = attack_group_get_subgroups_begin(g); sp && sp < attack_group_get_subgroups_end(g); sp++) ns++;
        REPLY(" group %u: target P%d, %d subgroups\n", k, target ? player_get_index(target) : -1, ns);
        for (ee_attack_subgroup **sp = attack_group_get_subgroups_begin(g); sp && sp < attack_group_get_subgroups_end(g); sp++) {
            if (!*sp) continue;
            int n = 0, walk = 0, fight = 0, other = 0, none = 0, engaged = 0;
            float cx = 0, cy = 0;
            ee_asg_unit **b = attack_subgroup_get_units_begin(*sp); ee_asg_unit **e = attack_subgroup_get_units_end(*sp);
            for (ee_asg_unit **r = b; b && r < e && n < 1000; r++) {
                ee_unit *u = *r ? asg_unit_get_unit(*r) : NULL;
                ee_goal *gl;
                if (!u || unit_get_hp(u) <= 0) continue;
                if (asg_unit_get_engaged(*r)) engaged++;
                n++; cx += (float)unit_get_x(u); cy += (float)unit_get_y(u);
                gl = unit_get_goal(u);
                if (!gl) none++;
                else if (goal_is_location(gl)) walk++;
                else if (goal_is_attack(gl)) fight++;
                else other++;
            }
            const char *cls = rtti_name(*sp);
            REPLY("   %-28s %3d alive at %.0f,%.0f: %d march, %d fight, %d other, %d idle\n", cls, n,
                  n ? cx / n : 0.0f, n ? cy / n : 0.0f, walk, fight, other, none);
            ee_attack_subgroup *sg = *sp;
            REPLY("     state %d mission %d, goal %d,%d, %u waypoints, at %d,%d, %d engaged\n",
                  attack_subgroup_get_state(sg), attack_subgroup_get_mission(sg), attack_subgroup_get_goal_x(sg), attack_subgroup_get_goal_y(sg),
                  attack_subgroup_get_waypoints(sg), attack_subgroup_get_x(sg), attack_subgroup_get_y(sg), engaged);
            if (n && strstr(args, "raw")) {   // the subgroup's fields as dwords, to compare over time
                for (ee_asg_unit **r = b; b && r < e; r++) {   // each member: type, tile, goal (and its target)
                    ee_unit *u = *r ? asg_unit_get_unit(*r) : NULL;
                    if (!u || unit_get_hp(u) <= 0) continue;
                    ee_goal *gl = unit_get_goal(u);
                    ee_unit *t = NULL;
                    if (gl && goal_is_attack(gl)) t = goal_get_target(gl);
                    REPLY("       %d at %d,%d hp %d: %s", unit_type_get_id(unit_get_type_data(u)),
                          unit_get_x(u), unit_get_y(u), unit_get_hp(u), gl ? rtti_name(gl) : "-");
                    if (t) REPLY(" P%d's %d (family %d) at %d,%d", player_get_index(unit_get_owner(t)),
                                 unit_type_get_id(unit_get_type_data(t)), unit_type_get_class_id(unit_get_type_data(t)),
                                 unit_get_x(t), unit_get_y(t));
                    REPLY(" rec+48 %08x\n", (unsigned)(uintptr_t)asg_unit_get_engaged(*r));
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

// a ship or an aircraft, by the class of its type data (bunkers, outposts and two test commands pick land soldiers by
// it). Until 2026-10-25 it tested the classes 11..17 - bombs, missiles, priests, the land and helicopter transports -
// so a fortress took ships and aircraft near it as candidates and left priests and strategist heroes out. Head-to-head
// (h2h.sh, tests/scenarios/lowpop.eeai, 20 seeds, the old test for one AI of each game): B - A soldiers -0.65 (se 1.0),
// kills +1.3 (se 4.0), losses -1.7 (se 4.0), none lost - no difference either way; the right classes stay.
internal int is_ship_or_air(ee_unit_type *td) {
    switch (unit_type_get_class_id(td)) {
    case CLS_NAVAL: case CLS_SUBMERGED_NAVAL: case CLS_NAVAL_TRANSPORT: case CLS_NAVAL_BUILDING: case CLS_NAVAL_CITIZEN:
    case CLS_AIRPLANE: case CLS_HELICOPTER: case CLS_HELICOPTER_TRANSPORT: case CLS_HELICOPTER_PRIEST:
        return 1;
    default:
        return 0;
    }
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
