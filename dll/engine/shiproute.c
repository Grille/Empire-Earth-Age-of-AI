// SHIP ROUTES (`ship_routes`, default 0 until measured; docs/engine-notes.md "Routes"): a ship's route planned whole
// and in straight lines instead of the game's. The game's route request (path.c) gives a tile A* in the 8 tile
// directions, a long route in pieces of 8-18 tiles through its sector graph, a new piece whenever that few waypoints
// are left - each piece its own split between straight and 45 degrees, so a route at any other angle zigzags (12-21%
// longer, tests/probes/ship_long.eeai). Here a ship's request is answered at once: an A* over the whole map on the
// game's own tile test (the type's terrain byte, the edge of the map's diamond, the water's class against the ship's
// mode - the shallows, which the naval movement refuses -, every object on the tile asked), tiles nearer to land than
// the ship's hull radius (half its formation size, the larger of its two spheres) dearer so the route keeps to the middle of a channel, then pulled
// straight - from each corner on to the furthest tile still in a clear line - and given as the unit's waypoints, one
// every 2 tiles along each straight leg (the game's obstacle detour, RepathAroundObstacle, rebuilds the list up to a
// few points ahead and drops the rest: with the corners alone it dropped the destination). The movement tests only
// the tile under the ship's centre (EEPMNaval::vf21), so the clear line needs no hull's width: SR_MARGIN keeps it off
// the tiles' corners. The movement turns at the corners in its own arcs. A target it can't reach (land, another sea) or a search too big
// goes to the game's request as before. A group sent far gets no sector route for a cluster of ships (the game's
// chain of auto-waypoint goals through the sector centres, EEActiveGroup::vf16): each ship asks for its own route.
// Only the 2017 update (AoC's request takes another argument). `land_routes` (default 0 until measured): the same for
// land units (and hovercraft: model_kind's MODEL_LAND) - the game's tile test as it is (buildings, walls, gates, the
// terrain byte), without the ships' water test and their dearer tiles near the shore; each search timed (fmove sum
// prints them: land units ask for routes far more often than ships). `land_widen` (with land_routes, default 0 until
// measured; the player: units scraped every corner on their way): the pulled route's corners hug what they round - a
// tile's centre beside it, the leg past its corner by SR_MARGIN -, so each corner of a land unit's route is moved away
// from what blocks around it, towards where the room grows, up to LR_WIDEN and only while the room still grows (in a
// gap it stops in the middle: a choke point is still taken) and both legs to it stay clear; then the points along the
// legs as before. `danger_routes` ([ai_base]; the player: AI citizens walk into enemy reach - a danger map per player
// instead of each manager's checks, and fishing boats and transports have the same problem): an AI's unit that can't
// fight (a citizen, or no attack: fishing boats, transports) pays SR_COST_DANGER more for each threat on a tile of
// its owner's danger map (game/danger.c), so it goes round where it can and through only where it must; the route is
// pulled straight only where the line meets no more danger than its two ends.

#define SR_COST_STRAIGHT 10
#define SR_COST_DIAGONAL 14
#define SR_COST_SHORE 6                 // for each tile of the hull radius a tile lies too near to land
#define SR_COST_DANGER 40               // for each threat on a tile of the owner's danger map (danger_routes)
#define SR_RADIUS_MAX 3                 // tiles: the hull radius looked at, at most
#define SR_MARGIN 0.3f                  // the clear line's half-width in tiles
#define SR_RETARGET 6                   // tiles: a target it can't enter moves to the nearest one it can this near
#define SR_STEP 2.0f                    // a waypoint at least every this many tiles along a straight leg
#define SR_MAX_POINTS 512
#define SR_SEARCH_PER_D2 64             // tiles a search may expand per tile of the trip's length squared
#define SR_SEARCH_MIN 4096              // and at least this many
#define SR_SHARED 32                    // routes kept for others to join (share_routes)
#define SR_SHARED_MS 1000               // game ms a kept route stays valid
#define SR_JOIN_MAX 4096                // tiles a search for a kept route may expand
#define SR_STEP_TILES 20000             // tiles the searches of one simulation step may expand before pieces (route_budget)
#define SR_PIECE_TILES 1024             // a piece's search, at most
#define SR_PIECE_TRIP 16                // tiles: a trip this long or longer may get a piece
#define SR_PIECE_PRIORITY 3             // budgets a step may spend on the units already holding a piece
#define SR_PIECE_SLOTS 4096
#define SR_JOIN_NEAR 24                // tiles from its start a kept route must pass to be joined
#define SR_JOIN_SLACK 8                 // tiles' cost it looks on for a better join once it reached the route
#define LR_WIDEN 1.5f                   // tiles: a land route's corner moved at most this far (land_widen)
#define LR_LOOK 3                       // tiles around a corner looked at for what blocks
#define LR_GAP 0.6f                     // tiles: the room wanted at a corner beyond the unit's own radius

internal int g_sr_on, g_sr_log, g_lr_on, g_lr_widen;
internal unsigned g_lr_corners, g_lr_widened;   // land_widen: corners looked at, moved
internal double g_lr_room0, g_lr_room1;         // their room before and after (tiles, summed)
internal int g_sr_ship;                 // this search's unit is a ship (the water test, the shore)
internal unsigned g_lr_routes;           // land searches made, and their time (ms): all, the longest
internal double g_lr_ms, g_lr_ms_max;
internal float g_sr_radius;             // this search's ship: its hull radius in tiles
internal unsigned g_sr_routed, g_sr_passed;   // routes given, requests left to the game
internal unsigned g_sr_tiles;           // tiles the searches expanded, all
internal const uint8_t *g_sr_danger;    // this search's danger map (danger_routes), NULL: none
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
    int32_t *on_path;                   // a kept route's index of the tile (valid with path_mark == mark)
    uint32_t *path_mark, mark;
} g_sr;

// Shared routes: a group sent far asked for a route per unit, each a search over the map - 14-58 searches of 17000-
// 35000 tiles in one simulation step, 50-100 ms that held the world's lock, the picture standing (the player's game
// at minute 750, 2026-10-28). So a route found is kept a while (SR_SHARED_MS), and the next request to the same tile
// by a unit that passes the same tiles at the same cost - the same owner (a gate lets its own side through), terrain
// kinds, ship or not with its sea mode and hull radius, danger map; an army of several types shares them - searches
// towards the target only till it meets the kept route (SR_JOIN_MAX tiles), then a little further for a cheaper join
// (its own way there plus the route's rest), and takes the route on from there, pulled straight as one. Not met: a
// search of its own, kept in turn.
#define SR_KINDS 18                     // the type's terrain bytes (unit_type +0x5D6 .. +0x5E7)
typedef struct {
    ee_map *map;
    ee_player *owner;
    const uint8_t *danger;
    uint8_t kinds[SR_KINDS];
    int ship, sea_mode, n;              // n: tiles, start to goal
    float radius;
    uint32_t goal, at, mark;            // at: game time kept; mark: its tiles' stamp in on_path
    uint32_t *tiles;
    int32_t *g;                         // the cost from its start to each tile
} sr_shared;
internal sr_shared g_sr_kept[SR_SHARED];
internal int g_sr_kept_next;
internal int g_sr_share, g_sr_budget;
internal float g_sr_weight = 1.0f;     // the A*'s estimate times this (tests: route_weight)
internal uint32_t g_sr_budget_step;     // the simulation step the budget counts
internal unsigned g_sr_budget_used, g_sr_pieces;   // its tiles so far; first pieces given
internal uint32_t g_sr_piece_holder[SR_PIECE_SLOTS];   // units holding a piece, by serial (a slot each; a clash only loses priority)
internal unsigned g_sr_joined, g_sr_join_missed;   // requests answered from a kept route, kept routes too far

internal int sr_is_ship(ee_unit *u) {
    ee_unit_type *td = unit_get_type_data(u);
    return td && unit_type_terrain_ok(td, 1) && !unit_type_terrain_ok(td, 0);
}

internal int sr_is_land(ee_unit *u) {
    ee_unit_type *td = unit_get_type_data(u);
    return td && model_kind(unit_type_get_model(td)) == MODEL_LAND;
}

// a request this planner answers: a ship's with ship_routes, a land unit's with land_routes
internal int sr_wants(ee_unit *u) { return u && ((g_sr_on && sr_is_ship(u)) || (g_lr_on && sr_is_land(u))); }

internal int sr_alloc(uint32_t n) {
    if (n <= g_sr.n) return 1;
    free(g_sr.seen); free(g_sr.closed); free(g_sr.g); free(g_sr.parent); free(g_sr.pass); free(g_sr.seen_pass); free(g_sr.heap); free(g_sr.f);
    free(g_sr.on_path); free(g_sr.path_mark);
    g_sr.seen = calloc(n, 4); g_sr.closed = calloc(n, 4); g_sr.g = calloc(n, 4); g_sr.parent = calloc(n, 4);
    g_sr.pass = calloc(n, 1); g_sr.seen_pass = calloc(n, 4); g_sr.heap = calloc(n, 4); g_sr.f = calloc(n, 4);
    g_sr.on_path = calloc(n, 4); g_sr.path_mark = calloc(n, 4);
    g_sr.n = g_sr.seen && g_sr.closed && g_sr.g && g_sr.parent && g_sr.pass && g_sr.seen_pass && g_sr.heap && g_sr.f &&
             g_sr.on_path && g_sr.path_mark ? n : 0;
    g_sr.gen = 0;
    g_sr.mark = 0;
    for (int k = 0; k < SR_SHARED; k++) g_sr_kept[k].n = 0;   // their marks are gone with the arrays
    return g_sr.n != 0;
}

// a new stamp: every cached entry unset (a search, or a test of things that move)
internal void sr_stamp(void) {
    if (++g_sr.gen == 0) {   // the stamps wrapped: clear them
        memset(g_sr.seen, 0, g_sr.n * 4); memset(g_sr.closed, 0, g_sr.n * 4); memset(g_sr.seen_pass, 0, g_sr.n * 4);
        g_sr.gen = 1;
    }
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
    if (!tile || !unit_type_terrain_ok(td, tile_get_kind(tile))) return 0;
    int sea = tile_get_sea_class(tile), mode = unit_get_sea_mode(u);   // the naval movement's own test (its route search's sea modifier)
    if (g_sr_ship && ((sea < 0 && mode != 0) || (sea > 0 && mode == 3))) return 0;
    for (void *node = tile_get_objects(tile); node; node = tilenode_get_next(node)) {
        ee_unit *o = tilenode_get_object(node);   // units that move are left to the movement (they won't be there)
        ee_unit_stats *st = o && o != u ? unit_get_stats(o) : NULL;
        if (o && o != u && !(st && unit_stats_get_speed(st) > 0) && !unit_lets_pass(o, u)) return 0;
    }
    g_sr.pass[i] = 1;
    return 1;
}

// is the straight line between two points clear, margin to each side
internal int sr_danger_at(int size, float x, float y) {
    int tx = (int)floorf(x), ty = (int)floorf(y);
    return g_sr_danger && tx >= 0 && ty >= 0 && tx < size && ty < size ? g_sr_danger[ty * size + tx] : 0;
}
internal int sr_clear_by(ee_map *map, ee_unit *u, ee_unit_type *td, int size, float ax, float ay, float bx, float by, float margin) {
    float dx = bx - ax, dy = by - ay, len = sqrtf(dx * dx + dy * dy);
    int steps = (int)(len / 0.2f) + 1;
    int da = sr_danger_at(size, ax, ay), db = sr_danger_at(size, bx, by), lim = da > db ? da : db;   // a search's danger map: no more than at the ends
    for (int k = 0; k <= steps; k++) {
        float px = ax + dx * (float)k / (float)steps, py = ay + dy * (float)k / (float)steps;
        if (g_sr_danger && sr_danger_at(size, px, py) > lim) return 0;
        for (int c = 0; c < 4; c++) {
            float qx = px + (c & 1 ? margin : -margin), qy = py + (c & 2 ? margin : -margin);
            if (!sr_pass(map, u, td, size, (int)floorf(qx), (int)floorf(qy))) return 0;
        }
    }
    return 1;
}
internal int sr_clear(ee_map *map, ee_unit *u, ee_unit_type *td, int size, float ax, float ay, float bx, float by) {
    return sr_clear_by(map, u, td, size, ax, ay, bx, by, SR_MARGIN);
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

// the room around a point: its distance to the nearest tile it may not enter (LR_LOOK + 1 when none within LR_LOOK);
// *away the way from what blocks, weighted by nearness
internal float lr_room(ee_map *map, ee_unit *u, ee_unit_type *td, int size, float x, float y, vec2 *away) {
    int fx = (int)floorf(x), fy = (int)floorf(y);
    float room = LR_LOOK + 1;
    vec2 a = v2(0, 0);
    for (int dy = -LR_LOOK; dy <= LR_LOOK; dy++)
        for (int dx = -LR_LOOK; dx <= LR_LOOK; dx++) {
            int tx = fx + dx, ty = fy + dy;
            if (sr_pass(map, u, td, size, tx, ty)) continue;
            float ex = x < tx ? tx - x : x > tx + 1 ? x - tx - 1 : 0, ey = y < ty ? ty - y : y > ty + 1 ? y - ty - 1 : 0;
            float d = sqrtf(ex * ex + ey * ey);
            if (d < room) room = d;
            vec2 from = v2(x - ((float)tx + 0.5f), y - ((float)ty + 0.5f));
            float l2 = vec2_dot(from, from);
            if (l2 > 1e-6f) a = vec2_add(a, vec2_scale(from, 1.0f / (l2 * sqrtf(l2))));
        }
    if (away) *away = a;
    return room;
}

// land_widen: corner c (between the legs from prev and to next) moved away from what blocks around it till it has the
// room wanted (want: the unit's radius + LR_GAP; further only lengthens the route), while the room grows, up to
// LR_WIDEN, both legs clear
internal void lr_widen(ee_map *map, ee_unit *u, ee_unit_type *td, int size, float *c, const float *prev, const float *next, float want) {
    vec2 away;
    float room = lr_room(map, u, td, size, c[0], c[1], &away), l = vec2_len(away), room0 = room;
    g_lr_corners++;
    g_lr_room0 += room0;
    if (l > 1e-6f && room < want) {
        away = vec2_scale(away, 1.0f / l);
        float bx = c[0], by = c[1];
        for (float s = 0.25f; s <= LR_WIDEN + 0.01f; s += 0.25f) {
            float qx = c[0] + away.x * s, qy = c[1] + away.y * s, r = lr_room(map, u, td, size, qx, qy, NULL);
            if (r <= room + 0.01f) break;   // past the middle of the room
            if (!sr_clear(map, u, td, size, prev[0], prev[1], qx, qy) || !sr_clear(map, u, td, size, qx, qy, next[0], next[1])) break;
            bx = qx; by = qy; room = r;
            if (room >= want) break;
        }
        if (bx != c[0] || by != c[1]) { c[0] = bx; c[1] = by; g_lr_widened++; }
    }
    g_lr_room1 += room;
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

// the step's cost into tile j from i in direction d (the A*'s and the join's), -1: not passable
internal int sr_step_cost(ee_map *map, ee_unit *u, ee_unit_type *td, int size, int x, int y, int d, uint32_t *jout) {
    static const int ddx[8] = { 1, -1, 0, 0, 1, 1, -1, -1 }, ddy[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
    int nx = x + ddx[d], ny = y + ddy[d];
    if (!sr_pass(map, u, td, size, nx, ny)) return -1;
    if (d >= 4 && (!sr_pass(map, u, td, size, nx, y) || !sr_pass(map, u, td, size, x, ny))) return -1;   // no corner cutting
    uint32_t j = (uint32_t)(ny * size + nx);
    *jout = j;
    int shore = (int)(sr_crowded(map, u, td, size, nx, ny) * SR_COST_SHORE + 0.5f);
    return (d >= 4 ? SR_COST_DIAGONAL : SR_COST_STRAIGHT) + shore + (g_sr_danger ? g_sr_danger[j] * SR_COST_DANGER : 0);
}

// a kept route u may join: to the same tile, its tiles passed at the same cost, kept recently, the one passing
// nearest to its start (within SR_JOIN_NEAR tiles)
internal sr_shared *sr_kept_for(ee_map *map, ee_unit *u, ee_unit_type *td, uint32_t start, uint32_t goal) {
    uint32_t now = game_time();
    ee_player *owner = unit_get_owner(u);
    int mode = g_sr_ship ? unit_get_sea_mode(u) : 0, size = (int)map_get_size(map);
    int sx = (int)(start % (uint32_t)size), sy = (int)(start / (uint32_t)size);
    sr_shared *best = NULL;
    int best_d = SR_JOIN_NEAR + 1;
    for (int k = 0; k < SR_SHARED; k++) {   // the one that passes nearest: an army's units come from all over the map
        sr_shared *e = &g_sr_kept[k];
        if (!(e->n > 0 && e->goal == goal && e->map == map && e->owner == owner && e->danger == g_sr_danger && e->ship == g_sr_ship &&
              e->sea_mode == mode && e->radius == g_sr_radius && !memcmp(e->kinds, unit_type_terrain_ok_at(td), SR_KINDS) &&
              now >= e->at && now - e->at <= SR_SHARED_MS)) continue;
        for (int i = 0; i < e->n; i++) {
            int dx = abs((int)(e->tiles[i] % (uint32_t)size) - sx), dy = abs((int)(e->tiles[i] / (uint32_t)size) - sy);
            int d = dx > dy ? dx : dy;
            if (d < best_d) { best_d = d; best = e; }
        }
    }
    return best;
}

// keep the route just found (its tiles goal back to start in path, their costs in g_sr.g)
internal void sr_keep(ee_map *map, ee_unit *u, ee_unit_type *td, uint32_t goal, const uint32_t *path, int n) {
    sr_shared *e = &g_sr_kept[g_sr_kept_next];
    g_sr_kept_next = (g_sr_kept_next + 1) % SR_SHARED;
    uint32_t *t = realloc(e->tiles, (size_t)n * sizeof *t);
    int32_t *g = t ? realloc(e->g, (size_t)n * sizeof *g) : NULL;
    if (t) e->tiles = t;
    if (g) e->g = g;
    if (!t || !g) { e->n = 0; return; }
    for (int k = 0; k < n; k++) { e->tiles[k] = path[n - 1 - k]; e->g[k] = g_sr.g[path[n - 1 - k]]; }
    e->map = map; e->owner = unit_get_owner(u); e->danger = g_sr_danger; e->ship = g_sr_ship;
    e->sea_mode = g_sr_ship ? unit_get_sea_mode(u) : 0; e->radius = g_sr_radius;
    memcpy(e->kinds, unit_type_terrain_ok_at(td), SR_KINDS);
    e->goal = goal; e->at = game_time(); e->n = n;
    e->mark = 0;   // marked when first joined
}

// u's way from start to the kept route e: an A* towards the route's goal till it meets the route (within SR_JOIN_MAX
// tiles), then SR_JOIN_SLACK on for a cheaper join (its way there + the route's rest); the tiles, goal back to start,
// into path; 0: not met
internal int sr_join(ee_map *map, ee_unit *u, ee_unit_type *td, int size, sr_shared *e, uint32_t start, uint32_t *path, int cap, int *expanded) {
    if (!e->mark || g_sr.path_mark[e->tiles[0]] != e->mark) {   // its tiles marked with their index
        if (++g_sr.mark == 0) { memset(g_sr.path_mark, 0, g_sr.n * 4); g_sr.mark = 1; }
        e->mark = g_sr.mark;
        for (int k = 0; k < e->n; k++) { g_sr.path_mark[e->tiles[k]] = e->mark; g_sr.on_path[e->tiles[k]] = k; }
    }
    int32_t total = e->g[e->n - 1];
    int tx = (int)(e->goal % (uint32_t)size), ty = (int)(e->goal / (uint32_t)size);
    g_sr.heap_n = 0;
    g_sr.seen[start] = g_sr.gen;
    g_sr.g[start] = 0;
    g_sr.parent[start] = -1;
    g_sr.f[start] = 0;
    sr_heap_push(start);
    int best_k = -1;
    int32_t best = 0;
    float reach = 0;
    uint32_t best_tile = 0;
    while (g_sr.heap_n && *expanded < SR_JOIN_MAX) {
        uint32_t i = sr_heap_pop();
        if (g_sr.closed[i] == g_sr.gen) continue;
        // from the route's first tile met on, a better join only a little further: one further along the route saves
        // its way there, and the route is pulled straight past the join anyway
        if (best_k >= 0 && g_sr.f[i] > reach) break;
        g_sr.closed[i] = g_sr.gen;
        (*expanded)++;
        g_sr_tiles++;
        if (g_sr.path_mark[i] == e->mark) {
            int k = g_sr.on_path[i];
            int32_t c = g_sr.g[i] + (total - e->g[k]);
            if (best_k < 0) reach = g_sr.f[i] + (float)(SR_JOIN_SLACK * SR_COST_STRAIGHT);
            if (best_k < 0 || c < best) { best_k = k; best = c; best_tile = i; }
        }
        int x = (int)i % size, y = (int)i / size;
        for (int d = 0; d < 8; d++) {
            uint32_t j;
            int step = sr_step_cost(map, u, td, size, x, y, d, &j);
            if (step < 0 || g_sr.closed[j] == g_sr.gen) continue;
            int g = g_sr.g[i] + step;
            if (g_sr.seen[j] == g_sr.gen && g >= g_sr.g[j]) continue;
            g_sr.seen[j] = g_sr.gen;
            g_sr.g[j] = g;
            g_sr.parent[j] = (int32_t)i;
            int nx = (int)j % size, ny = (int)j / size, hx = abs(tx - nx), hy = abs(ty - ny);
            g_sr.f[j] = (float)g + g_sr_weight * (float)(SR_COST_STRAIGHT * (hx > hy ? hx - hy : hy - hx) + SR_COST_DIAGONAL * (hx < hy ? hx : hy));
            sr_heap_push(j);
        }
    }
    if (best_k < 0) return 0;
    int n = 0;
    for (int k = e->n - 1; k > best_k && n < cap; k--) path[n++] = e->tiles[k];   // the route's rest, goal first
    for (int32_t i = (int32_t)best_tile; i >= 0 && n < cap; i = g_sr.parent[i]) path[n++] = (uint32_t)i;   // then its own way back
    return n;
}

// the A* from tile sx,sy to tx,ty: its tiles, goal back to start, into path; 0: none found. partial: a first piece
// only, at most that many tiles searched, up to the tile nearest the target (*whole 0 then)
internal int sr_find(ee_map *map, ee_unit *u, ee_unit_type *td, int size, int sx, int sy, int tx, int ty, uint32_t *path, int path_max, int *expanded,
                     int partial, int *whole) {
    uint32_t start = (uint32_t)(sy * size + sx), goal = (uint32_t)(ty * size + tx);
    g_sr.heap_n = 0;
    g_sr.seen[start] = g_sr.gen;
    g_sr.g[start] = 0;
    g_sr.parent[start] = -1;
    g_sr.f[start] = 0;
    sr_heap_push(start);
    // A target it can't reach is known only once the search has filled all it can reach: in a long game a few
    // tiles' trip to a spot inside a closed area searched 58000 tiles, about 100 ms in one simulation step - which
    // holds the world's lock, so the picture stood that long several times a second (the player's game at minute
    // 750, 2026-10-28). So the search is bounded by the trip: SR_SEARCH_PER_D2 tiles per tile of its length squared,
    // at least SR_SEARCH_MIN; beyond that it is left to the game's request, as when nothing is found. In that game's
    // 19317 searches it stopped 58, none of them one that found a route.
    int dx0 = abs(tx - sx), dy0 = abs(ty - sy), trip = dx0 > dy0 ? dx0 : dy0;
    long long cap = (long long)SR_SEARCH_PER_D2 * trip * trip;
    if (cap < SR_SEARCH_MIN) cap = SR_SEARCH_MIN;
    if (cap > 4LL * size * size) cap = 4LL * size * size;
    if (partial && cap > partial) cap = partial;
    int found = 0, near_h = -1;
    uint32_t nearest = start;
    *whole = 1;
    while (g_sr.heap_n && *expanded < cap) {
        uint32_t i = sr_heap_pop();
        if (g_sr.closed[i] == g_sr.gen) continue;
        g_sr.closed[i] = g_sr.gen;
        (*expanded)++;
        g_sr_tiles++;
        if (i == goal) { found = 1; break; }
        int x = (int)i % size, y = (int)i / size;
        if (partial) {   // the piece's end: the tile searched nearest the target
            int hx = abs(tx - x), hy = abs(ty - y), h = hx > hy ? hx : hy;
            if (near_h < 0 || h < near_h) { near_h = h; nearest = i; }
        }
        for (int d = 0; d < 8; d++) {
            uint32_t j;
            int step = sr_step_cost(map, u, td, size, x, y, d, &j);
            if (step < 0 || g_sr.closed[j] == g_sr.gen) continue;
            int g = g_sr.g[i] + step;
            if (g_sr.seen[j] == g_sr.gen && g >= g_sr.g[j]) continue;
            g_sr.seen[j] = g_sr.gen;
            g_sr.g[j] = g;
            g_sr.parent[j] = (int32_t)i;
            int nx = (int)j % size, ny = (int)j / size, hx = abs(tx - nx), hy = abs(ty - ny);
            g_sr.f[j] = (float)g + g_sr_weight * (float)(SR_COST_STRAIGHT * (hx > hy ? hx - hy : hy - hx) + SR_COST_DIAGONAL * (hx < hy ? hx : hy));
            sr_heap_push(j);
        }
    }
    if (!found && partial && *expanded >= cap && nearest != start) {
        found = 1;
        goal = nearest;
        *whole = 0;
    }
    if (!found) {
        if (g_log && g_sr_log) fprintf(g_log, "# shiproute: %06x %d,%d -> %d,%d: no route (%d tiles searched%s) - the game's\n", unit_get_serial(u) & 0xFFFFFF, sx, sy, tx, ty, *expanded,
                                       *expanded >= cap ? ", the trip's bound" : "");
        return 0;
    }
    int n = 0;
    for (int32_t i = (int32_t)goal; i >= 0 && n < path_max; i = g_sr.parent[i]) path[n++] = (uint32_t)i;
    return n;
}

// u's route from tile from to tile to as its waypoints; 0: left to the game
internal int sr_search(ee_unit *u, int *from, int *to, void *caller, int sectors) {
    ee_map *map = game_map();
    ee_unit_type *td = unit_get_type_data(u);
    int size = map ? (int)map_get_size(map) : 0;
    const char *why = !map || !td ? "no map" : unit_get_waypoints_left(u) > 0 ? "waypoints left" : size <= 0 || !sr_alloc((uint32_t)(size * size)) ? "no memory" : NULL;
    if (why) {
        if (g_log && g_sr_log) fprintf(g_log, "# shiproute: %06x -> %d,%d: %s - the game's\n", unit_get_serial(u) & 0xFFFFFF, to[0], to[1], why);
        return 0;
    }
    sr_stamp();
    int sx = from[0], sy = from[1], tx = to[0], ty = to[1];
    g_sr_ship = sr_is_ship(u);
    g_sr_radius = unit_type_get_formation_size(td) * 0.5f;
    if (g_sr_radius < 0.35f) g_sr_radius = 0.35f;
    if (g_sr_radius > SR_RADIUS_MAX) g_sr_radius = SR_RADIUS_MAX;
    if (!g_sr_ship) g_sr_radius = 0;   // a land unit: no dearer tiles near what blocks
    if (sx < 0 || sy < 0 || sx >= size || sy >= size) return 0;
    if (!sr_pass(map, u, td, size, tx, ty)) {   // a target it can't enter (shallows, land, a building): the nearest it can, as the game's search ends there
        int best = -1, bx = tx, by = ty;
        for (int dy = -SR_RETARGET; dy <= SR_RETARGET; dy++)
            for (int dx = -SR_RETARGET; dx <= SR_RETARGET; dx++) {
                int d2 = dx * dx + dy * dy;
                if ((best < 0 || d2 < best) && sr_pass(map, u, td, size, tx + dx, ty + dy)) { best = d2; bx = tx + dx; by = ty + dy; }
            }
        if (best < 0) {
            if (g_log && g_sr_log) fprintf(g_log, "# shiproute: %06x -> %d,%d: nothing it can enter within %d tiles of the target - the game's\n", unit_get_serial(u) & 0xFFFFFF, tx, ty, SR_RETARGET);
            return 0;
        }
        tx = bx;
        ty = by;
    }
    uint32_t start = (uint32_t)(sy * size + sx), goal = (uint32_t)(ty * size + tx);
    static uint32_t path[65536];
    int n = 0, expanded = 0, joined = 0, whole = 1;
    // The step's budget (route_budget): the tiles the searches of this simulation step expanded - counted, not
    // timed, so every PC of a multiplayer game decides alike. Past it a long trip gets a first piece only (the
    // nearest the target SR_PIECE_TILES of search reach); when the unit has walked it, the game asks again
    // (PrepareToMove: not there yet) and a later step gives the rest.
    ee_physics *ph = game_physics();
    uint32_t step = ph ? physics_get_steps(ph) : 0;
    if (step != g_sr_budget_step) { g_sr_budget_step = step; g_sr_budget_used = 0; }
    // A unit with a piece asks again a few tiles on, and in a big order's steps the budget is spent again: it walked on
    // piece by piece (six in a row), each a search of its own. So one that holds a piece goes before the new ones:
    // its whole route unless the step has spent SR_PIECE_PRIORITY budgets.
    int dxt = abs(tx - sx), dyt = abs(ty - sy);
    uint32_t serial = unit_get_serial(u), slot = serial % SR_PIECE_SLOTS;
    int holds = g_sr_piece_holder[slot] == serial;
    unsigned allowed = holds ? SR_PIECE_PRIORITY * SR_STEP_TILES : SR_STEP_TILES;
    int piece = g_sr_budget && g_sr_budget_used >= allowed && (dxt > dyt ? dxt : dyt) >= SR_PIECE_TRIP ? SR_PIECE_TILES : 0;
    sr_shared *kept = g_sr_share ? sr_kept_for(map, u, td, start, goal) : NULL;
    if (kept) {
        n = sr_join(map, u, td, size, kept, start, path, 65536, &expanded);
        if (n) { joined = 1; g_sr_joined++; }
        else { g_sr_join_missed++; sr_stamp(); }   // a search of its own, its stamps fresh
    }
    if (!joined) {
        n = sr_find(map, u, td, size, sx, sy, tx, ty, path, 65536, &expanded, piece, &whole);
        if (!n) { g_sr_budget_used += (unsigned)expanded; return 0; }
        if (!whole) { g_sr_pieces++; g_sr_piece_holder[slot] = serial; }
        else if (g_sr_share) sr_keep(map, u, td, goal, path, n);
    }
    if (whole && holds) g_sr_piece_holder[slot] = 0;
    g_sr_budget_used += (unsigned)expanded;
    // pulled straight from the start: from each kept tile on to the furthest one still in a clear line
    static float pts[SR_MAX_POINTS][3], cn[SR_MAX_POINTS][2];
    int np = 0, nc = 0, at = n - 1;
    cn[nc][0] = (float)(path[at] % (uint32_t)size) + 0.5f; cn[nc][1] = (float)(path[at] / (uint32_t)size) + 0.5f; nc++;
    while (at > 0 && nc < SR_MAX_POINTS) {
        float ax = (float)(path[at] % (uint32_t)size) + 0.5f, ay = (float)(path[at] / (uint32_t)size) + 0.5f;
        int next = at - 1;
        while (next > 0 && sr_clear(map, u, td, size, ax, ay, (float)(path[next - 1] % (uint32_t)size) + 0.5f, (float)(path[next - 1] / (uint32_t)size) + 0.5f)) next--;
        cn[nc][0] = (float)(path[next] % (uint32_t)size) + 0.5f; cn[nc][1] = (float)(path[next] / (uint32_t)size) + 0.5f; nc++;
        at = next;
    }
    int corners = nc - 1;
    if (g_lr_widen && !g_sr_ship)   // the corners between start and goal moved off what they round
        for (int c = 1; c < nc - 1; c++) lr_widen(map, u, td, size, cn[c], cn[c - 1], cn[c + 1], unit_type_get_formation_size(td) * 0.5f + LR_GAP);
    for (int c = 1; c < nc; c++) {
        float ax = cn[c - 1][0], ay = cn[c - 1][1], bx = cn[c][0], by = cn[c][1];
        int steps = (int)ceilf(sqrtf((bx - ax) * (bx - ax) + (by - ay) * (by - ay)) / SR_STEP);
        for (int k = 1; k <= steps && np < SR_MAX_POINTS - 1; k++) {
            pts[np][0] = ax + (bx - ax) * (float)k / (float)steps;
            pts[np][1] = ay + (by - ay) * (float)k / (float)steps;
            pts[np][2] = 0;
            np++;
        }
    }
    if (np == 0) return 0;
    // the last point where the goal wants it (a group's place inside the tile)
    ee_goal *goal_obj = unit_get_goal_object(u);
    if (goal_obj) {
        vec3 t = goal_target(goal_obj, u);
        if (whole && (int)floorf(t.x) == tx && (int)floorf(t.y) == ty) { pts[np - 1][0] = t.x; pts[np - 1][1] = t.y; }
    }
    for (int k = 0; k < np; k++) unit_add_waypoint(u, pts[k], 1);   // 1: a point on the map's edge lines kept
    g_sr_routed++;
    if (g_sr_danger) {
        g_dr_routes++;
        for (int k = 0; k < n; k++) g_dr_tiles += g_sr_danger[path[k]] != 0;
    }
    if (g_log && g_sr_log) {
        fprintf(g_log, "# shiproute: %06x %d,%d -> %d,%d (from %p, sectors %d, P%d type %d): %d tiles (%d searched%s), %d legs, %d points:", unit_get_serial(u) & 0xFFFFFF, sx, sy,
                tx, ty, caller, sectors & 0xFF, unit_get_owner(u) ? player_get_index(unit_get_owner(u)) : -1, unit_type_get_id(td), n, expanded, joined ? ", joined a kept route" : !whole ? ", a first piece" : "", corners, np);
        for (int k = 0; k < np && k < 40; k++) fprintf(g_log, " %.1f,%.1f", pts[k][0], pts[k][1]);
        fprintf(g_log, "\n");
    }
    return 1;
}

// danger_routes: an AI player's unit that can't fight (a citizen, or a type without an attack) routed by its owner's
// danger map; its soldier on the way somewhere (a move goal) by the map's lost fights
internal int sr_route(ee_unit *u, int *from, int *to, void *caller, int sectors) {
    ee_player *pl = unit_get_owner(u);
    ee_unit_type *td = unit_get_type_data(u);
    ee_map *map = game_map();
    int pi = pl ? player_get_index(pl) : 0;
    g_sr_danger = NULL;
    if (map && td && pi > 0 && !player_is_human(pl)) {
        int size = (int)map_get_size(map);
        ee_goal *g = unit_get_goal_object(u);
        if (is_citizen_type(unit_type_get_id(td)) || unit_type_get_attack(td) <= 0) g_sr_danger = danger_map((unsigned)pi, DANGER_THREATS, size);
        else if (g && goal_is_location(g) && (g_sr_danger = danger_map((unsigned)pi, DANGER_LOST, size)) != NULL) g_dr_lost_routes++;
    }
    int done = sr_search(u, from, to, caller, sectors);
    g_sr_danger = NULL;
    return done;
}

// the searches of the simulation step running (control.c's step hook reads and clears them: a slow step's line)
internal struct { unsigned n, tiles, longest_tiles; double ms, longest_ms; int lx0, ly0, lx1, ly1, lfound; } g_sr_step;

internal void __thiscall hook_path_request(void *mgr, ee_unit *u, int *from, int *to, int sync, int limit, int sectors, void *ctx) {
    if (sr_wants(u) && from && to) {
        int land = !sr_is_ship(u);
        LARGE_INTEGER t0, t1, f;
        QueryPerformanceCounter(&t0);
        unsigned tiles0 = g_sr_tiles;
        int done = sr_route(u, from, to, __builtin_return_address(0), sectors);
        QueryPerformanceCounter(&t1);
        QueryPerformanceFrequency(&f);
        double ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)f.QuadPart;
        if (land) {
            g_lr_routes++;
            g_lr_ms += ms;
            if (ms > g_lr_ms_max) g_lr_ms_max = ms;
        }
        g_sr_step.n++;
        g_sr_step.ms += ms;
        g_sr_step.tiles += g_sr_tiles - tiles0;
        if (ms > g_sr_step.longest_ms) {
            g_sr_step.longest_ms = ms;
            g_sr_step.longest_tiles = g_sr_tiles - tiles0;
            g_sr_step.lx0 = from[0]; g_sr_step.ly0 = from[1]; g_sr_step.lx1 = to[0]; g_sr_step.ly1 = to[1];
            g_sr_step.lfound = done;
        }
        if (done) return;
        g_sr_passed++;
    }
    path_mgr_request(mgr, u, from, to, sync, limit, sectors, ctx);
}

internal void __thiscall hook_group_sector_route(ee_group *group, int *from, int *to, void *units) {
    if (g_sr_on && units) {
        int ships = 0, others = 0;
        for (ee_unit **p = unitlist_get_begin(units); p && p < unitlist_get_end(units); p++) {
            if (sr_wants(*p)) ships++;
            else others++;
        }
        if (ships && !others) {
            if (g_log && g_sr_log) fprintf(g_log, "# shiproute: a group's %d units to %d,%d without the sector route\n", ships, to ? to[0] : -1, to ? to[1] : -1);
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
internal int g_st_on, g_lt_on;           // ship_turns; land_turns: the steering along the route (slot 7) for land units too
internal unsigned g_st_kept;             // ticks the game would have braked and the arc kept the speed

// is the circle (cx, cy, r) free for u: every tile in it one u may enter (the route's test, a new stamp: things move),
// no other unit that doesn't let it pass within the circle and a hull of it
internal int st_circle_clear(ee_unit *u, float cx, float cy, float r) {
    ee_map *map = game_map();
    ee_unit_type *td = unit_get_type_data(u);
    int size = map ? (int)map_get_size(map) : 0;
    if (!td || size <= 0 || !sr_alloc((uint32_t)(size * size))) return 0;
    sr_stamp();
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

// STEERING: the movement steers straight at the current waypoint and takes it only within the type's +0x474 and on
// its tile, so a waypoint off to the side within the ship's turning circle can't be hit - the ship overshoots, steers
// back hard and circles it (a submarine at 5.65 tiles/s looped one and a half times around one, then once more at the
// end). Here the tick's turn (slot 7) is given another point: the one on the route ST_AHEAD turning radii (at the top
// speed, v / w) away from the ship - it slides along the route as the ship moves, so the ship homes in on the route
// and rounds a corner in an arc - when the line there is clear, else the waypoint. The waypoints stay the game's (its
// arrival, the obstacle detour and the approach to the last read them); one passed, or within a turning radius, is
// dropped for the next when that lies in a clear line. From the top speed, not the speed now: from rest the radius is
// small - the ship steered at the near waypoint, drifted while it turned and swung 30 degrees past the route, then 10
// back. The last waypoint, the turn to a facing and the stop are the game's.
#define ST_AHEAD 4.0f
#define ST_SCAN 8                       // waypoints looked along for the point
internal unsigned g_st_skipped;          // waypoints dropped
internal unsigned g_st_steered;          // ticks steered at a point on the route
internal unsigned g_st_passing;          // ticks a leg's end was passed through (another move queued)
internal ee_unit *g_st_aim_unit;         // this tick's unit steered at a point and its distance (for the speed's turn)
internal float g_st_aim_dist, g_st_aim_x, g_st_aim_y;

// the stamp of the tiles' cache once a step: the clear lines of all ships in a step share it (a route search takes
// its own)
internal void st_stamp_step(void) {
    static unsigned last = ~0u;
    ee_physics *ph = game_physics();
    unsigned step = ph ? physics_get_steps(ph) : 0;
    if (step != last || g_sr.gen == 0) { sr_stamp(); last = step; }
}

// per ship the distance its point was last at (by serial, a small open table: a ship not found takes the slot over)
#define ST_MEM_UNITS 1024
#define ST_GROW 1.05f                   // the point goes out by this a tick
#define ST_SHRINK 0.85f                 // and in by this a try
#define ST_AIM_MARGIN 0.1f              // tiles: the line to the point (the movement tests only the tile under the centre)
internal struct { uint32_t id; float reach; } g_st_mem[ST_MEM_UNITS];
// a new game starts the table empty: it steers the simulation, so every PC of a game must start alike
internal void shipturns_new_game(void) { memset(g_st_mem, 0, sizeof g_st_mem); }

internal float *st_unit_mem(ee_unit *u) {
    uint32_t id = unit_get_serial(u), h = (id * 2654435761u) % ST_MEM_UNITS;
    for (int k = 0; k < 8; k++) {
        uint32_t i = (h + (uint32_t)k) % ST_MEM_UNITS;
        if (g_st_mem[i].id == id) return &g_st_mem[i].reach;
        if (!g_st_mem[i].id) { g_st_mem[i].id = id; g_st_mem[i].reach = 0; return &g_st_mem[i].reach; }
    }
    g_st_mem[h].id = id;
    g_st_mem[h].reach = 0;
    return &g_st_mem[h].reach;
}

// drops the waypoints u has passed or can't steer onto
internal void st_drop_passed(ee_unit *u, ee_map *map, ee_unit_type *td, int size, float px, float py, float radius) {
    for (int k = 0; k < 4 && unit_get_waypoints_left(u) >= 2; k++) {
        float a[3], b[3];
        if (unit_waypoint_at(u, 0, a) || unit_waypoint_at(u, 1, b)) break;
        float dx = a[0] - px, dy = a[1] - py;
        int passed = (px - a[0]) * (b[0] - a[0]) + (py - a[1]) * (b[1] - a[1]) > 0;
        if (!passed && dx * dx + dy * dy >= radius * radius) break;
        if (!sr_clear(map, u, td, size, px, py, b[0], b[1]) || unit_pop_waypoint(u)) break;
        g_st_skipped++;
    }
}

// PASSING THROUGH: a leg's end with another move queued after it (shift) is an in-between point to the game's arrival
// (EEUnitClass::vf34: within the type's +0x474 on its tile, when the current order is a move, +5 1, and more are
// queued), but its speed step brakes for it as for a stop - on the last waypoint no faster than the distance left,
// and slower within the stopping distance - and the steering ends there: a dreadnought slowed from 0.6 to 0.34 at each
// queued point, the game's crawled at 0.1 where the next leg turned more than 45 degrees. While the next order is a
// move too, its target continues the route: the point slides on towards it and the speed step is given the way on.
// the order after u's current one when u passes its leg's end without stopping - on its last waypoint, the next order a
// move whose target lies in a clear line from that end (into onward) - else NULL: where the next leg starts round land
// the point slid onto the blocked line and the ship circled short of the end
internal ee_goal *st_pass_through(ee_unit *u, float *onward) {
    if (unit_get_waypoints_left(u) != 1 || unit_orders(u) < 2) return NULL;
    ee_goal *g = unit_order_at(u, 0), *next = unit_order_at(u, 1);
    if (!g || !next || goal_get_state(g) != 1 || goal_get_state(next) != 1) return NULL;
    ee_map *map = game_map();
    ee_unit_type *td = unit_get_type_data(u);
    int size = map ? (int)map_get_size(map) : 0;
    float end[3];
    if (!td || size <= 0 || !sr_alloc((uint32_t)(size * size)) || unit_waypoint_at(u, 0, end)) return NULL;
    st_stamp_step();
    vec3 t = goal_target(next, u);
    if (!sr_clear(map, u, td, size, end[0], end[1], t.x, t.y)) return NULL;
    onward[0] = t.x; onward[1] = t.y; onward[2] = t.z;
    return next;
}

// the point on u's route (from p along its waypoints, then on to onward when given) reach away from p, the furthest
// such: 1, else the waypoint is nearer to steer at (it lies further, or the route ends within reach)
internal int st_aim_point(ee_unit *u, float px, float py, float reach, const float *onward, float *out) {
    float pts[ST_SCAN + 1][3];
    int n = 0, left = unit_get_waypoints_left(u);
    for (; n < left && n < ST_SCAN; n++)
        if (unit_waypoint_at(u, n, pts[n])) return 0;
    if (onward && n == left) { pts[n][0] = onward[0]; pts[n][1] = onward[1]; pts[n][2] = onward[2]; n++; }
    float from[2] = {px, py};
    for (int i = 0; i < n; i++) {
        float *pt = pts[i], dx = pt[0] - px, dy = pt[1] - py;
        if (dx * dx + dy * dy >= reach * reach) {
            if (i == 0) return 0;
            float sx = pt[0] - from[0], sy = pt[1] - from[1], fx = from[0] - px, fy = from[1] - py;   // the leg's way out of the circle
            float qa = sx * sx + sy * sy, qb = 2 * (fx * sx + fy * sy), qc = fx * fx + fy * fy - reach * reach, disc = qb * qb - 4 * qa * qc;
            if (qa <= 0 || disc < 0) return 0;
            float t = (-qb + sqrtf(disc)) / (2 * qa);
            out[0] = from[0] + sx * t; out[1] = from[1] + sy * t; out[2] = pt[2];
            return 1;
        }
        from[0] = pt[0]; from[1] = pt[1];
    }
    if (n < 2) return 0;
    out[0] = pts[n - 1][0]; out[1] = pts[n - 1][1]; out[2] = pts[n - 1][2];   // the route ends within reach: its end
    return 1;
}

internal int fo_follow_aim(ee_unit *u, const float *pos, float *aim);   // groupmove.c: formation_follow's point
internal float ff_speed(ee_unit *u, float s, float angle);              // ... and its speed
internal void ff_turn(ee_unit *u, const float *left, float *rate, float *heading);   // ... and a ship's turn
internal void __thiscall hook_ship_turn_towards(void *model, ee_unit *u, float *pos, float *target, float *left, float *rate, float *heading, char facing) {
    float aim[3], *to = target;
    if (!facing && fo_follow_aim(u, pos, aim)) {   // a unit of a formation's march (land units and ships)
        physics_model_turn_towards(model, u, pos, aim, left, rate, heading, facing);
        if (model_kind(model) != MODEL_LAND) ff_turn(u, left, rate, heading);   // a ship turns steadily (land units at a fixed rate in their own tick)
        return;
    }
    if (u == g_st_aim_unit) g_st_aim_unit = NULL;
    ee_map *map = game_map();
    ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
    int size = map ? (int)map_get_size(map) : 0, wps = u ? unit_get_waypoints_left(u) : 0;
    float w = u ? unit_max_turn(u) : 0, onward[3];
    int on = model_kind(model) == MODEL_LAND ? g_lt_on : g_st_on;
    ee_goal *next = on && u ? st_pass_through(u, onward) : NULL;
    if (on && !facing && td && size > 0 && w > 0 && (wps >= 2 || next) && sr_alloc((uint32_t)(size * size))) {
        st_stamp_step();
        float reach = ST_AHEAD * unit_max_speed(u) / w;
        st_drop_passed(u, map, td, size, pos[0], pos[1], reach / ST_AHEAD);
        // the leg's end passed (or within a turning radius) on the way to the next order's target: it moves onto the
        // ship, so the game's arrival takes it next tick - the point had slid on, the ship cut the corner, never came
        // within the type's +0x474 of the end and circled it
        float a[3];
        if (next && unit_get_waypoints_left(u) == 1 && !unit_waypoint_at(u, 0, a)) {
            float dx = a[0] - pos[0], dy = a[1] - pos[1], rad = reach / ST_AHEAD;
            int passed = (pos[0] - a[0]) * (onward[0] - a[0]) + (pos[1] - a[1]) * (onward[1] - a[1]) > 0;
            if ((passed || dx * dx + dy * dy < rad * rad) && sr_clear(map, u, td, size, pos[0], pos[1], onward[0], onward[1]))
                unit_set_waypoint_at(u, 0, pos[0], pos[1]);
        }
        // a point whose line clips land comes nearer along the route (from the far one straight to the waypoint the
        // heading jumped up to 20 degrees along a coast); it goes out again slowly - from the full distance each tick
        // the point flicked between two and the heading by a turn step (3 degrees)
        float *last = st_unit_mem(u), r = last && *last > 0 && *last * ST_GROW < reach ? *last * ST_GROW : reach;
        int found = 0;
        for (; r >= reach * 0.2f; r *= ST_SHRINK)
            if ((found = st_aim_point(u, pos[0], pos[1], r, next ? onward : NULL, aim) && sr_clear_by(map, u, td, size, pos[0], pos[1], aim[0], aim[1], ST_AIM_MARGIN))) break;
        if (last) *last = found ? r : 0;
        if (found) {
            to = aim;
            g_st_steered++;
        } else if (!unit_waypoint(u, aim)) to = aim;   // the current one (it may have moved on)
        float dx = to[0] - pos[0], dy = to[1] - pos[1];
        g_st_aim_unit = u;
        g_st_aim_dist = sqrtf(dx * dx + dy * dy);
        g_st_aim_x = to[0];
        g_st_aim_y = to[1];
    }
    physics_model_turn_towards(model, u, pos, to, left, rate, heading, facing);
}

// the speed step at a leg's end that is passed through: given the way on to the next order's target, it neither
// brakes for the stop nor caps the speed at the distance left; a ship of a formation's march at its speed there
internal float __thiscall hook_ship_speed_step(void *model, ee_unit *u, float dist, float angle) {
    float wp[3], t[3];
    if (g_st_on && u && st_pass_through(u, t) && !unit_waypoint(u, wp)) {
        dist += sqrtf((t[0] - wp[0]) * (t[0] - wp[0]) + (t[1] - wp[1]) * (t[1] - wp[1]));
        g_st_passing++;
    }
    return ff_speed(u, physics_model_speed_step(model, u, dist, angle), angle);
}

internal int __thiscall hook_ship_speed(void *model, ee_unit *u, float dist, float angle) {
    int d = physics_model_speed_decide(model, u, dist, angle), aimed = u && u == g_st_aim_unit;
    float e = fabsf(angle), wp[3], onward[3];
    if (aimed) {   // the turn steered at another point this tick: the arc's distance and side are to it
        dist = g_st_aim_dist;
        wp[0] = g_st_aim_x; wp[1] = g_st_aim_y;
        g_st_aim_unit = NULL;
    }
    if (!g_st_on || !u || e <= 0.7853982f || (unit_get_waypoints_left(u) < 2 && !st_pass_through(u, onward)) || dist <= 0) return d;
    ee_physics *ph = game_physics();
    float w = unit_max_turn(u), v = unit_get_speed(u), top = unit_max_speed(u), step = ph ? physics_get_step_s(ph) * unit_accel(u) : 0;
    float s = sinf(e), keep = s > 0.001f ? ST_SPARE * w * dist / (2 * s) : top;
    if (keep > top) keep = top;
    // the circle it would sweep must be free (water it may enter, no other ship in it): else a tighter, slower one, at
    // the least the game's crawl - a U-turn at speed out of a packed fleet ran into the others and the game gave up
    vec3 p = unit_get_pos(u), f = unit_get_facing(u);
    float fl = sqrtf(f.x * f.x + f.y * f.y);
    if (w <= 0 || fl <= 0 || (!aimed && unit_waypoint(u, wp))) return d;
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

// a test run's simulation step of SR_SLOW_STEP_MS or more, logged with the route searches in it: a step holds the
// world's lock for its whole time, and the picture waits (render_3d.c's minute line counts those waits)
#define SR_SLOW_STEP_MS 40
internal void sr_step_timed(double ms) {
    if (ms >= SR_SLOW_STEP_MS && g_log)
        log_locked("# slow step: %.0f ms; route searches %u, %.1f ms, %u tiles; the longest %.1f ms, %u tiles, %d,%d -> %d,%d%s\n",
                   ms, g_sr_step.n, g_sr_step.ms, g_sr_step.tiles, g_sr_step.longest_ms, g_sr_step.longest_tiles, g_sr_step.lx0,
                   g_sr_step.ly0, g_sr_step.lx1, g_sr_step.ly1, g_sr_step.n ? g_sr_step.lfound ? " (routed)" : " (the game's)" : "");
    memset(&g_sr_step, 0, sizeof g_sr_step);
}

internal void shiproute_install(const char *autotest_ini) {
    g_st_on = g_set.ship_turns;
    g_lt_on = g_set.land_turns;
    int lt = g_lt_on || (g_set.formations && g_set.formation_follow) ? patch_land_turn_towards(hook_ship_turn_towards) : 0;   // formation_follow steers there too
    int follow = g_set.formations && g_set.formation_follow;   // formation_follow steers ships and sets their speed there too
    int st = g_st_on ? patch_ship_speed_decide(hook_ship_speed) : 0;
    st += g_st_on || follow ? patch_ship_turn_towards(hook_ship_turn_towards) + patch_ship_speed_step(hook_ship_speed_step) : 0;
    if (g_log) fprintf(g_log, "# shipturns: ship_turns=%d (hooks %s) land_turns=%d (hooks %d/2)\n", g_st_on, !g_st_on && !follow ? "-" : st == (g_st_on ? 6 : 4) ? "ok" : "FAILED", g_lt_on, lt);
    g_sr_on = g_set.ship_routes;
    if (autotest_ini) g_sr_log = ini_int("autotest", "ship_route_log", 0, autotest_ini);
    g_lr_on = g_set.land_routes;
    g_lr_widen = g_lr_on && g_set.land_widen;
    g_sr_share = (g_sr_on || g_lr_on) && g_set.share_routes;
    g_sr_budget = (g_sr_on || g_lr_on) && g_set.route_budget;
    g_sr_weight = (float)g_set.route_weight / 100.0f;
    int ok = g_sr_on || g_lr_on ? patch_path_request(hook_path_request) && patch_group_sector_route(hook_group_sector_route) : 0;
    if (g_log) fprintf(g_log, "# shiproute: ship_routes=%d land_routes=%d (corners widened: %d, shared: %d, step budget: %d) (hook %s)%s\n", g_sr_on, g_lr_on, g_lr_widen, g_sr_share, g_sr_budget, !g_sr_on && !g_lr_on ? "-" : ok ? "ok" : build_is("aoc") ? "not in AoC" : "FAILED",
                       g_sr_log ? ", routes logged" : "");
}
