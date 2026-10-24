// SHIP ROUTES (`ship_routes`, default 0 until measured; docs/engine-notes.md "Routes"): a ship's route planned whole
// and in straight lines instead of the game's. The game's route request (path.c) gives a tile A* in the 8 tile
// directions, a long route in pieces of 8-18 tiles through its sector graph, a new piece whenever that few waypoints
// are left - each piece its own split between straight and 45 degrees, so a route at any other angle zigzags (12-21%
// longer, tests/probes/ship_long.eeai). Here a ship's request is answered at once: an A* over the whole map on the
// game's own tile test (the type's terrain byte, the edge of the map's diamond, the water's class against the ship's
// mode - the shallows, which the naval movement refuses -, every object on the tile asked), tiles nearer to land than
// the ship's hull radius (half its collision sphere) dearer so the route keeps to the middle of a channel, then pulled
// straight - from each corner on to the furthest tile still in a clear line - and given as the unit's waypoints, one
// every 2 tiles along each straight leg (the game's obstacle detour, RepathAroundObstacle, rebuilds the list up to a
// few points ahead and drops the rest: with the corners alone it dropped the destination). The movement tests only
// the tile under the ship's centre (EEPMNaval::vf21), so the clear line needs no hull's width: SR_MARGIN keeps it off
// the tiles' corners. The movement turns at the corners in its own arcs. A target it can't reach (land, another sea) or a search too big
// goes to the game's request as before. A group sent far gets no sector route for a cluster of ships (the game's
// chain of auto-waypoint goals through the sector centres, EEActiveGroup::vf16): each ship asks for its own route.
// Only the 2017 update (AoC's request takes another argument).

#define SR_COST_STRAIGHT 10
#define SR_COST_DIAGONAL 14
#define SR_COST_SHORE 6                 // for each tile of the hull radius a tile lies too near to land
#define SR_RADIUS_MAX 3                 // tiles: the hull radius looked at, at most
#define SR_MARGIN 0.3f                  // the clear line's half-width in tiles
#define SR_RETARGET 6                   // tiles: a target it can't enter moves to the nearest one it can this near
#define SR_STEP 2.0f                    // a waypoint at least every this many tiles along a straight leg
#define SR_MAX_POINTS 512

internal int g_sr_on, g_sr_log;
internal float g_sr_radius;             // this search's ship: its hull radius in tiles
internal unsigned g_sr_routed, g_sr_passed;   // routes given, requests left to the game
internal struct {
    uint32_t n;                         // tiles (the map's size squared) the arrays hold
    uint32_t gen;                       // this search's stamp: an entry of another is unset
    uint32_t *seen, *closed;            // stamps
    int32_t *g, *parent;
    uint8_t *pass;                      // 1 passable, 2 not (valid with seen_pass == gen)
    uint32_t *seen_pass;
    uint32_t *heap;                     // tile indexes, by f
    float *f;
    int heap_n;
} g_sr;

internal int sr_is_ship(ee_unit *u) {
    ee_unit_type *td = unit_get_type_data(u);
    return td && type_terrain_ok(td, 1) && !type_terrain_ok(td, 0);
}

internal int sr_alloc(uint32_t n) {
    if (n <= g_sr.n) return 1;
    free(g_sr.seen); free(g_sr.closed); free(g_sr.g); free(g_sr.parent); free(g_sr.pass); free(g_sr.seen_pass); free(g_sr.heap); free(g_sr.f);
    g_sr.seen = calloc(n, 4); g_sr.closed = calloc(n, 4); g_sr.g = calloc(n, 4); g_sr.parent = calloc(n, 4);
    g_sr.pass = calloc(n, 1); g_sr.seen_pass = calloc(n, 4); g_sr.heap = calloc(n, 4); g_sr.f = calloc(n, 4);
    g_sr.n = g_sr.seen && g_sr.closed && g_sr.g && g_sr.parent && g_sr.pass && g_sr.seen_pass && g_sr.heap && g_sr.f ? n : 0;
    g_sr.gen = 0;
    return g_sr.n != 0;
}

// the game's tile test for u (0x41D36A) with the naval movement's test of the water (EEPMNaval::vf21), but for the
// units that move, cached for this search
internal int sr_pass(ee_map *map, ee_unit *u, ee_unit_type *td, int size, int x, int y) {
    if (x < 0 || y < 0 || x >= size || y >= size) return 0;
    uint32_t i = (uint32_t)(y * size + x);
    if (g_sr.seen_pass[i] == g_sr.gen) return g_sr.pass[i] == 1;
    g_sr.seen_pass[i] = g_sr.gen;
    g_sr.pass[i] = 2;
    if (x + y == map_get_edge_sum(map) || x + y == map_get_edge_sum_2(map) || y - x == map_get_edge_diff(map) || x - y == map_get_edge_diff(map)) return 0;
    ee_tile *tile = map_get_tiles(map)[i];
    if (!tile || !type_terrain_ok(td, tile_get_kind(tile))) return 0;
    int sea = tile_get_sea_class(tile), mode = unit_get_sea_mode(u);   // the naval movement's own test (its route search's sea modifier)
    if ((sea < 0 && mode != 0) || (sea > 0 && mode == 3)) return 0;
    for (void *node = tile_get_objects(tile); node; node = tilenode_get_next(node)) {
        ee_unit *o = tilenode_get_object(node);   // units that move are left to the movement (they won't be there)
        ee_unit_stats *st = o && o != u ? unit_get_stats(o) : NULL;
        if (o && o != u && !(st && stat_get_speed(st) > 0) && !unit_lets_pass(o, u)) return 0;
    }
    g_sr.pass[i] = 1;
    return 1;
}

// is the straight line between two points clear, SR_MARGIN to each side
internal int sr_clear(ee_map *map, ee_unit *u, ee_unit_type *td, int size, float ax, float ay, float bx, float by) {
    float dx = bx - ax, dy = by - ay, len = sqrtf(dx * dx + dy * dy);
    int steps = (int)(len / 0.2f) + 1;
    for (int k = 0; k <= steps; k++) {
        float px = ax + dx * (float)k / (float)steps, py = ay + dy * (float)k / (float)steps;
        for (int c = 0; c < 4; c++) {
            float qx = px + (c & 1 ? SR_MARGIN : -SR_MARGIN), qy = py + (c & 2 ? SR_MARGIN : -SR_MARGIN);
            if (!sr_pass(map, u, td, size, (int)floorf(qx), (int)floorf(qy))) return 0;
        }
    }
    return 1;
}

// how much nearer to land than the hull radius the tile lies (tiles, 0: clear)
internal float sr_crowded(ee_map *map, ee_unit *u, ee_unit_type *td, int size, int x, int y) {
    int r = (int)ceilf(g_sr_radius);
    float nearest = g_sr_radius + 1;
    for (int dy = -r; dy <= r; dy++)
        for (int dx = -r; dx <= r; dx++) {
            if (!dx && !dy) continue;
            float d = sqrtf((float)(dx * dx + dy * dy)) - 0.5f;   // to the blocked tile's near side
            if (d < nearest && !sr_pass(map, u, td, size, x + dx, y + dy)) nearest = d;
        }
    return nearest < g_sr_radius ? g_sr_radius - nearest : 0;
}

internal void sr_heap_push(uint32_t i) {
    int k = g_sr.heap_n++;
    while (k > 0) {
        int up = (k - 1) / 2;
        if (g_sr.f[g_sr.heap[up]] <= g_sr.f[i]) break;
        g_sr.heap[k] = g_sr.heap[up];
        k = up;
    }
    g_sr.heap[k] = i;
}
internal uint32_t sr_heap_pop(void) {
    uint32_t top = g_sr.heap[0], last = g_sr.heap[--g_sr.heap_n];
    int k = 0;
    for (;;) {
        int c = 2 * k + 1;
        if (c >= g_sr.heap_n) break;
        if (c + 1 < g_sr.heap_n && g_sr.f[g_sr.heap[c + 1]] < g_sr.f[g_sr.heap[c]]) c++;
        if (g_sr.f[last] <= g_sr.f[g_sr.heap[c]]) break;
        g_sr.heap[k] = g_sr.heap[c];
        k = c;
    }
    if (g_sr.heap_n) g_sr.heap[k] = last;
    return top;
}

// u's route from tile from to tile to as its waypoints; 0: left to the game
internal int sr_route(ee_unit *u, int *from, int *to, void *caller, int sectors) {
    ee_map *map = game_map();
    ee_unit_type *td = unit_get_type_data(u);
    int size = map ? (int)map_get_size(map) : 0;
    const char *why = !map || !td ? "no map" : unit_get_waypoints_left(u) > 0 ? "waypoints left" : size <= 0 || !sr_alloc((uint32_t)(size * size)) ? "no memory" : NULL;
    if (why) {
        if (g_log && g_sr_log) fprintf(g_log, "# shiproute: %06x -> %d,%d: %s - the game's\n", unit_get_serial(u) & 0xFFFFFF, to[0], to[1], why);
        return 0;
    }
    if (++g_sr.gen == 0) {   // the stamps wrapped: clear them
        memset(g_sr.seen, 0, g_sr.n * 4); memset(g_sr.closed, 0, g_sr.n * 4); memset(g_sr.seen_pass, 0, g_sr.n * 4);
        g_sr.gen = 1;
    }
    int sx = from[0], sy = from[1], tx = to[0], ty = to[1];
    g_sr_radius = type_get_collision(td) * 0.5f;
    if (g_sr_radius < 0.35f) g_sr_radius = 0.35f;
    if (g_sr_radius > SR_RADIUS_MAX) g_sr_radius = SR_RADIUS_MAX;
    if (sx < 0 || sy < 0 || sx >= size || sy >= size) return 0;
    if (!sr_pass(map, u, td, size, tx, ty)) {   // a target it can't enter (shallows, land, a building): the nearest it can, as the game's search ends there
        int best = -1, bx = tx, by = ty;
        for (int dy = -SR_RETARGET; dy <= SR_RETARGET; dy++)
            for (int dx = -SR_RETARGET; dx <= SR_RETARGET; dx++) {
                int d2 = dx * dx + dy * dy;
                if ((best < 0 || d2 < best) && sr_pass(map, u, td, size, tx + dx, ty + dy)) { best = d2; bx = tx + dx; by = ty + dy; }
            }
        if (best < 0) {
            if (g_log && g_sr_log) fprintf(g_log, "# shiproute: %06x -> %d,%d: no water within %d tiles of the target - the game's\n", unit_get_serial(u) & 0xFFFFFF, tx, ty, SR_RETARGET);
            return 0;
        }
        tx = bx;
        ty = by;
    }
    uint32_t start = (uint32_t)(sy * size + sx), goal = (uint32_t)(ty * size + tx);
    g_sr.heap_n = 0;
    g_sr.seen[start] = g_sr.gen;
    g_sr.g[start] = 0;
    g_sr.parent[start] = -1;
    g_sr.f[start] = 0;
    sr_heap_push(start);
    int expanded = 0, found = 0;
    while (g_sr.heap_n && expanded < 4 * size * size) {
        uint32_t i = sr_heap_pop();
        if (g_sr.closed[i] == g_sr.gen) continue;
        g_sr.closed[i] = g_sr.gen;
        expanded++;
        if (i == goal) { found = 1; break; }
        int x = (int)i % size, y = (int)i / size;
        for (int d = 0; d < 8; d++) {
            static const int ddx[8] = { 1, -1, 0, 0, 1, 1, -1, -1 }, ddy[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
            int nx = x + ddx[d], ny = y + ddy[d];
            if (!sr_pass(map, u, td, size, nx, ny)) continue;
            if (d >= 4 && (!sr_pass(map, u, td, size, nx, y) || !sr_pass(map, u, td, size, x, ny))) continue;   // no corner cutting
            uint32_t j = (uint32_t)(ny * size + nx);
            if (g_sr.closed[j] == g_sr.gen) continue;
            int shore = (int)(sr_crowded(map, u, td, size, nx, ny) * SR_COST_SHORE + 0.5f);
            int g = g_sr.g[i] + (d >= 4 ? SR_COST_DIAGONAL : SR_COST_STRAIGHT) + shore;
            if (g_sr.seen[j] == g_sr.gen && g >= g_sr.g[j]) continue;
            g_sr.seen[j] = g_sr.gen;
            g_sr.g[j] = g;
            g_sr.parent[j] = (int32_t)i;
            int hx = abs(tx - nx), hy = abs(ty - ny);
            g_sr.f[j] = (float)g + (float)(SR_COST_STRAIGHT * (hx > hy ? hx - hy : hy - hx) + SR_COST_DIAGONAL * (hx < hy ? hx : hy));
            sr_heap_push(j);
        }
    }
    if (!found) {
        if (g_log && g_sr_log) fprintf(g_log, "# shiproute: %06x %d,%d -> %d,%d: no route (%d tiles searched) - the game's\n", unit_get_serial(u) & 0xFFFFFF, sx, sy, tx, ty, expanded);
        return 0;
    }
    // the tiles from the goal back, then pulled straight from the start: from each kept tile on to the furthest one
    // still in a clear line
    static uint32_t path[65536];
    int n = 0;
    for (int32_t i = (int32_t)goal; i >= 0 && n < 65536; i = g_sr.parent[i]) path[n++] = (uint32_t)i;
    static float pts[SR_MAX_POINTS][3];
    int np = 0, corners = 0, at = n - 1;
    while (at > 0 && np < SR_MAX_POINTS - 1) {
        float ax = (float)(path[at] % (uint32_t)size) + 0.5f, ay = (float)(path[at] / (uint32_t)size) + 0.5f;
        int next = at - 1;
        while (next > 0 && sr_clear(map, u, td, size, ax, ay, (float)(path[next - 1] % (uint32_t)size) + 0.5f, (float)(path[next - 1] / (uint32_t)size) + 0.5f)) next--;
        float bx = (float)(path[next] % (uint32_t)size) + 0.5f, by = (float)(path[next] / (uint32_t)size) + 0.5f;
        int steps = (int)ceilf(sqrtf((bx - ax) * (bx - ax) + (by - ay) * (by - ay)) / SR_STEP);
        for (int k = 1; k <= steps && np < SR_MAX_POINTS - 1; k++) {
            pts[np][0] = ax + (bx - ax) * (float)k / (float)steps;
            pts[np][1] = ay + (by - ay) * (float)k / (float)steps;
            pts[np][2] = 0;
            np++;
        }
        corners++;
        at = next;
    }
    if (np == 0) return 0;
    // the last point where the goal wants it (a group's place inside the tile)
    ee_goal *goal_obj = unit_get_goal_object(u);
    if (goal_obj) {
        vec3 t = goal_target(goal_obj, u);
        if ((int)floorf(t.x) == tx && (int)floorf(t.y) == ty) { pts[np - 1][0] = t.x; pts[np - 1][1] = t.y; }
    }
    for (int k = 0; k < np; k++) unit_add_waypoint(u, pts[k]);
    g_sr_routed++;
    if (g_log && g_sr_log) {
        fprintf(g_log, "# shiproute: %06x %d,%d -> %d,%d (from %p, sectors %d): %d tiles (%d searched), %d legs, %d points:", unit_get_serial(u) & 0xFFFFFF, sx, sy,
                tx, ty, caller, sectors & 0xFF, n, expanded, corners, np);
        for (int k = 0; k < np && k < 40; k++) fprintf(g_log, " %.1f,%.1f", pts[k][0], pts[k][1]);
        fprintf(g_log, "\n");
    }
    return 1;
}

internal void __thiscall hook_path_request(void *mgr, ee_unit *u, int *from, int *to, int sync, int limit, int sectors, void *ctx) {
    if (g_sr_on && u && from && to && sr_is_ship(u) && sr_route(u, from, to, __builtin_return_address(0), sectors)) return;
    if (g_sr_on && u && sr_is_ship(u)) g_sr_passed++;
    path_request(mgr, u, from, to, sync, limit, sectors, ctx);
}

internal void __thiscall hook_group_sector_route(void *group, int *from, int *to, void *units) {
    if (g_sr_on && units) {
        int ships = 0, others = 0;
        for (ee_unit **p = unitlist_get_begin(units); p && p < unitlist_get_end(units); p++) {
            if (*p && sr_is_ship(*p)) ships++;
            else others++;
        }
        if (ships && !others) {
            if (g_log && g_sr_log) fprintf(g_log, "# shiproute: a group's %d ships to %d,%d without the sector route\n", ships, to ? to[0] : -1, to ? to[1] : -1);
            return;
        }
    }
    group_sector_route(group, from, to, units);
}

// SHIP TURNS (`ship_turns`, default 0 until measured): the naval movement brakes to a crawl whenever the waypoint
// lies more than 45 degrees off the bow - a new order while under way, a corner - and turns almost on the spot
// (tests/probes/ship_reorder.eeai: 0.6 -> 0.09 in 1.7 s, 6-7 s for a 100-degree turn). A ship turning at its top
// rate w at speed v sweeps a circle of radius v / w and still reaches a point d away at e off the bow while
// d >= 2 (v / w) sin e: so it keeps the speed v = w d / (2 sin e) (at most its top speed, a tenth to spare) and turns
// in an arc - wide towards a far point, tight and slower towards a near one. Within 45 degrees and on the last
// waypoint (the approach and stop) the game's decision stays.
#define ST_SPARE 0.9f
#define ST_MIN_RADIUS 0.6f              // tiles: a tighter circle is the game's crawl
#define ST_HULL 1.0f                    // tiles: another ship this near the circle is in the way
internal int g_st_on;
internal unsigned g_st_kept;             // ticks the game would have braked and the arc kept the speed

// is the circle (cx, cy, r) free for u: every tile in it one u may enter (the route's test, a new stamp: things move),
// no other unit that doesn't let it pass within the circle and a hull of it
internal int st_circle_clear(ee_unit *u, float cx, float cy, float r) {
    ee_map *map = game_map();
    ee_unit_type *td = unit_get_type_data(u);
    int size = map ? (int)map_get_size(map) : 0;
    if (!td || size <= 0 || !sr_alloc((uint32_t)(size * size))) return 0;
    if (++g_sr.gen == 0) {
        memset(g_sr.seen, 0, g_sr.n * 4); memset(g_sr.closed, 0, g_sr.n * 4); memset(g_sr.seen_pass, 0, g_sr.n * 4);
        g_sr.gen = 1;
    }
    int x0 = (int)floorf(cx - r), x1 = (int)floorf(cx + r), y0 = (int)floorf(cy - r), y1 = (int)floorf(cy + r);
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            float dx = (float)x + 0.5f - cx, dy = (float)y + 0.5f - cy;
            if (dx * dx + dy * dy <= (r + 0.5f) * (r + 0.5f) && !sr_pass(map, u, td, size, x, y)) return 0;
        }
    int reach = (int)ceilf(r + ST_HULL) + 1;
    for (struct grid_it it = grid_it_box((int)cx, (int)cy, reach, 1); grid_it_next(&it); ) {
        ee_unit *o = it.u;
        if (o == u || unit_get_hp(o) <= 0 || unit_lets_pass(o, u)) continue;
        vec3 q = unit_get_pos(o);
        float dx = q.x - cx, dy = q.y - cy;
        if (dx * dx + dy * dy <= (r + ST_HULL) * (r + ST_HULL)) return 0;
    }
    return 1;
}

internal int __thiscall hook_ship_speed(void *model, ee_unit *u, float dist, float angle) {
    int d = model_speed_decide(model, u, dist, angle);
    float e = fabsf(angle);
    if (!g_st_on || !u || e <= 0.7853982f || unit_get_waypoints_left(u) < 2 || dist <= 0) return d;
    ee_physics *ph = game_physics();
    float w = unit_max_turn(u), v = unit_get_speed(u), top = unit_max_speed(u), step = ph ? phys_get_step_s(ph) * unit_accel(u) : 0;
    float s = sinf(e), keep = s > 0.001f ? ST_SPARE * w * dist / (2 * s) : top;
    if (keep > top) keep = top;
    // the circle it would sweep must be free (water it may enter, no other ship in it): else a tighter, slower one, at
    // the least the game's crawl - a U-turn at speed out of a packed fleet ran into the others and the game gave up
    vec3 p = unit_get_pos(u), f = unit_get_facing(u);
    float wp[3], fl = sqrtf(f.x * f.x + f.y * f.y);
    if (w <= 0 || fl <= 0 || unit_waypoint(u, wp)) return d;
    float side = (f.x * (wp[1] - p.y) - f.y * (wp[0] - p.x)) >= 0 ? 1.0f : -1.0f, nx = -f.y / fl * side, ny = f.x / fl * side;
    for (;;) {
        float r = keep / w;
        if (r < ST_MIN_RADIUS) return d;
        if (st_circle_clear(u, p.x + nx * r, p.y + ny * r, r)) break;
        keep *= 0.6f;
    }
    int arc = v < keep ? 3 : v > keep + step ? 4 : 2;
    if (arc != 4 && d == 4) g_st_kept++;
    return arc;
}

internal void shiproute_install(const char *aimod_ini, const char *autotest_ini) {
    g_st_on = ini_int("general", "ship_turns", 0, aimod_ini);
    if (autotest_ini) g_st_on = ini_int("autotest", "ship_turns", g_st_on, autotest_ini);
    int st = g_st_on ? patch_ship_speed_decide(hook_ship_speed) : 0;
    if (g_log) fprintf(g_log, "# shipturns: ship_turns=%d (hooks %s)\n", g_st_on, !g_st_on ? "-" : st == 2 ? "ok" : "FAILED");
    g_sr_on = ini_int("general", "ship_routes", 0, aimod_ini);
    if (autotest_ini) {
        g_sr_on = ini_int("autotest", "ship_routes", g_sr_on, autotest_ini);
        g_sr_log = ini_int("autotest", "ship_route_log", 0, autotest_ini);
    }
    int ok = g_sr_on ? patch_path_request(hook_path_request) && patch_group_sector_route(hook_group_sector_route) : 0;
    if (g_log) fprintf(g_log, "# shiproute: ship_routes=%d (hook %s)%s\n", g_sr_on, !g_sr_on ? "-" : ok ? "ok" : build_is("aoc") ? "not in AoC" : "FAILED",
                       g_sr_log ? ", routes logged" : "");
}
