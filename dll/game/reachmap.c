// REACH MAP: the map's terrain as a coarse graph - which parts of it a land unit can walk between (the player
// 2026-10-31: "can land and/or sea units path from this side of it to that? the value for each side pair cached - it
// could also speed up pathfinding"). The map in RM_CELL-tile cells; a cell's walkable tiles fall into pieces (connected
// inside the cell: a river, a cliff or a shore through it makes two), each piece a node; a node links to the pieces of
// the 8 cells round it that a step reaches (a diagonal step only past two walkable tiles, as a unit squeezes through);
// the nodes joined by links are a region - one lookup answers "can it walk from here to there". A tile is walkable as
// the route planner tests it for a citizen (sr_pass): its terrain (the citizen's terrain table, inside the map's
// diamond - one-tile fords of shallow water walkable) and nothing standing on it that doesn't move - trees, mines,
// buildings, walls (a forest blocks like a wall: built from the terrain alone, 60% of the corridors through it failed).
// Trees fall and buildings rise, so the map is built again every RM_REBUILD_MS of game time - deterministic, the same
// on every PC. Not yet: ships.
// GATES: a gate's tiles are pieces of their own, in none of the map's regions: which player walks through is
// the game's test (rm_gate_open) - its owner and the players it is allied with (allies' units count as its own), unless
// the gate is locked. Each player has its regions on top (rm_region_of): the map's joined through the gates open to it,
// worked out again every RM_GATE_MS (a lock, an alliance changes). A walled-in enemy is then sealed off for the AI, and
// a front along the wall is a wall.
// The game's own AI areas (EEAIContinentManager, map_areas) give a region per area too, but areas are irregular and
// their neighbours unknown - this one is the mod's, the same in both builds. Used by ai/fronts.c.
#define RM_CELL 4
#define RM_REBUILD_MS 30000
#define RM_GATE_MS 1000
#define RM_GATES 4096              // gates kept; the tiles of more count blocked
#define RM_PLAYERS 16
#define RM_PIECES 4              // pieces kept per cell; a fifth and more join the fourth (rare: a maze of water)
#define RM_SIDE_MAX 256          // a 1024-tile map
#define RM_NODES (RM_SIDE_MAX * RM_SIDE_MAX * RM_PIECES)
internal struct {
    ee_map *map; int size, side;           // the map it was built for
    uint8_t piece[1024 * 1024];            // per tile: its piece in its cell, 0xFF not walkable
    uint32_t link[RM_NODES];               // per node: bit d * 4 + p - the neighbour cell in direction d, its piece p
    int region[RM_NODES];                  // per node: its region (-1 no node)
    int regions, nodes;
    uint32_t built;                        // game time it was built
    uint8_t kind_on_map[256];              // the terrain kinds the map has (rm_walks_like_citizen compares those)
    uint8_t walk[1024 * 1024];             // per tile: walkable (rm_tile_ok), this build's
    uint16_t gate_at[1024 * 1024];         // per tile: its gate (index into gate), RM_NO_GATE none
    int16_t node_gate[RM_NODES];           // per node: its gate, -1 none
    struct { short x, y; } gate[RM_GATES]; // a tile of each gate (the gate itself looked up there again: it may be gone)
    int gates;
    uint16_t gate_open[RM_GATES];          // per gate: bit p - open to player p (rm_players)
    int *player_region;                    // [RM_PLAYERS][regions]: the map's region -> player p's (rm_players)
    uint32_t players_at, players_built;    // game time worked out, and for the map built then
} g_reach;
#define RM_NO_GATE 0xFFFF
internal const int8_t k_rm_dx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 }, k_rm_dy[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
internal int rm_gate_open(ee_unit *g, int p) { return gate_opens_for(g, player_at(p)); }   // game/api/unit.c
internal ee_unit *rm_gate_on(ee_tile *tile) {   // the gate standing on a tile, NULL none
    for (void *node = tile ? tile_get_objects(tile) : NULL; node; node = tilenode_get_next(node)) {
        ee_unit *o = tilenode_get_object(node);
        ee_unit_type *otd = o ? unit_get_type_data(o) : NULL;
        if (otd && unit_type_get_class_id(otd) == CLS_GATE) return o;
    }
    return NULL;
}
// may a citizen stand there: 0 no, 1 yes, 2 a gate's tile (rm_gate_open says for whom)
internal int rm_tile_ok(ee_map *map, int size, int x, int y) {
    if (x < 0 || y < 0 || x >= size || y >= size) return 0;
    if (x + y <= map_get_edge_sum(map) || x + y >= map_get_edge_sum_2(map) || y - x >= map_get_edge_diff(map) || x - y >= map_get_edge_diff(map)) return 0;
    ee_tile *tile = map_get_tiles(map)[y * size + x];
    static ee_unit_type *walker;
    if (!walker) walker = type_data(UT(CITIZEN));
    if (!tile || !walker || !unit_type_terrain_ok(walker, tile_get_kind(tile))) return 0;
    int gate = 0;
    for (void *node = tile_get_objects(tile); node; node = tilenode_get_next(node)) {   // what stands there and doesn't move
        ee_unit *o = tilenode_get_object(node);
        ee_unit_type *otd = o ? unit_get_type_data(o) : NULL;
        ee_unit_stats *st = o ? unit_get_stats(o) : NULL;
        if (!o || !otd || (st && unit_stats_get_speed(st) > 0)) continue;
        if (unit_type_get_class_id(otd) == CLS_GATE && unit_get_hp(o) > 0) { gate = 1; continue; }
        if (unit_lets_pass(o, NULL)) continue;   // the game's test, as the route planner asks it: a farm, rubble, the dead
        return 0;
    }
    return gate ? 2 : 1;
}
#define RM_W(x, y) ((x) >= 0 && (y) >= 0 && (x) < size && (y) < size && g_reach.walk[(y) * size + (x)])
#define RM_GATE_AT(x, y) g_reach.gate_at[(y) * size + (x)]
internal int rm_find(int *up, int a) { while (up[a] != a) a = up[a] = up[up[a]]; return a; }
internal void rm_build(ee_map *map) {
    int size = (int)map_get_size(map);
    if (size <= 0 || size > 1024) { g_reach.map = map; g_reach.size = 0; return; }   // a bigger map: none (rm_ready says so)
    int side = (size + RM_CELL - 1) / RM_CELL;
    DWORD t0 = GetTickCount();   // the log's only: nothing decided by time
    g_reach.map = map; g_reach.size = size; g_reach.side = side;
    memset(g_reach.piece, 0xFF, (size_t)size * (size_t)size);
    memset(g_reach.kind_on_map, 0, sizeof g_reach.kind_on_map);
    memset(g_reach.gate_at, 0xFF, (size_t)size * (size_t)size * sizeof *g_reach.gate_at);
    g_reach.gates = 0;
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++) {
            int ok = rm_tile_ok(map, size, x, y);
            ee_tile *tile = map_get_tiles(map)[y * size + x];
            if (tile) g_reach.kind_on_map[tile_get_kind(tile) & 0xFF] = 1;
            if (ok == 2) {   // a gate's tile: the gate it belongs to, numbered as first met (a gate's tiles touch)
                ee_unit *g = rm_gate_on(tile);
                int k = -1;
                for (int d = 4; d < 8 && k < 0; d++) {   // the tiles before it (left, the row above): the gate's index there
                    int nx = x + k_rm_dx[d], ny = y + k_rm_dy[d];
                    if (nx < 0 || ny < 0 || nx >= size) continue;
                    uint16_t gk = RM_GATE_AT(nx, ny);
                    if (gk != RM_NO_GATE && rm_gate_on(map_get_tiles(map)[ny * size + nx]) == g) k = gk;
                }
                if (k < 0 && g_reach.gates < RM_GATES) {
                    k = g_reach.gates++;
                    g_reach.gate[k].x = (short)x; g_reach.gate[k].y = (short)y;
                }
                if (k < 0) ok = 0;   // more gates than kept: a wall
                else RM_GATE_AT(x, y) = (uint16_t)k;
            }
            g_reach.walk[y * size + x] = (uint8_t)(ok != 0);
        }
    static short stack[RM_CELL * RM_CELL * 2];
    // pieces: each cell's walkable tiles flooded inside it
    for (int cy = 0; cy < side; cy++)
        for (int cx = 0; cx < side; cx++) {
            int x0 = cx * RM_CELL, y0 = cy * RM_CELL, np = 0, plain = -1;   // plain: the last piece that isn't a gate's
            for (int y = y0; y < y0 + RM_CELL && y < size; y++)
                for (int x = x0; x < x0 + RM_CELL && x < size; x++) {
                    if (g_reach.piece[y * size + x] != 0xFF || !RM_W(x, y)) continue;
                    uint16_t gk = RM_GATE_AT(x, y);
                    // no piece left: a gate's tiles count a wall, others join the last piece that isn't a gate's (none: a wall)
                    if (np >= RM_PIECES && (gk != RM_NO_GATE || plain < 0)) { g_reach.walk[y * size + x] = 0; continue; }
                    uint8_t p = (uint8_t)(np < RM_PIECES ? np++ : plain);
                    if (gk == RM_NO_GATE) plain = p;
                    int sp = 0;
                    g_reach.piece[y * size + x] = p;
                    stack[sp++] = (short)x; stack[sp++] = (short)y;
                    while (sp) {
                        int ty = stack[--sp], tx = stack[--sp];
                        for (int d = 0; d < 8; d++) {
                            int nx = tx + k_rm_dx[d], ny = ty + k_rm_dy[d];
                            if (nx < x0 || ny < y0 || nx >= x0 + RM_CELL || ny >= y0 + RM_CELL || nx >= size || ny >= size) continue;
                            if (g_reach.piece[ny * size + nx] != 0xFF || !RM_W(nx, ny) || RM_GATE_AT(nx, ny) != gk) continue;
                            if ((d & 1) && (!RM_W(tx + k_rm_dx[d], ty) || !RM_W(tx, ty + k_rm_dy[d]))) continue;
                            g_reach.piece[ny * size + nx] = p;
                            if (sp + 2 <= (int)(sizeof stack / sizeof *stack)) { stack[sp++] = (short)nx; stack[sp++] = (short)ny; }
                        }
                    }
                }
        }
    // links: a step from a tile of one cell to a tile of the next
    memset(g_reach.link, 0, (size_t)side * (size_t)side * RM_PIECES * sizeof *g_reach.link);
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++) {
            uint8_t p = g_reach.piece[y * size + x];
            if (p == 0xFF) continue;
            int cx = x / RM_CELL, cy = y / RM_CELL;
            for (int d = 0; d < 8; d++) {
                int nx = x + k_rm_dx[d], ny = y + k_rm_dy[d];
                if (nx < 0 || ny < 0 || nx >= size || ny >= size || (nx / RM_CELL == cx && ny / RM_CELL == cy)) continue;
                uint8_t q = g_reach.piece[ny * size + nx];
                if (q == 0xFF) continue;
                if ((d & 1) && (g_reach.piece[y * size + nx] == 0xFF || g_reach.piece[ny * size + x] == 0xFF)) continue;
                int ddx = nx / RM_CELL - cx, ddy = ny / RM_CELL - cy, dir = 0;
                while (k_rm_dx[dir] != ddx || k_rm_dy[dir] != ddy) dir++;
                g_reach.link[(cy * side + cx) * RM_PIECES + p] |= 1u << (dir * 4 + q);
            }
        }
    // regions: the nodes joined by links
    static int up[RM_NODES];
    int n = side * side * RM_PIECES, nodes = 0;
    for (int i = 0; i < n; i++) { up[i] = i; g_reach.node_gate[i] = -1; }
    static uint8_t has[RM_NODES];
    memset(has, 0, (size_t)n);
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++)
            if (g_reach.piece[y * size + x] != 0xFF) {
                int i = ((y / RM_CELL) * side + x / RM_CELL) * RM_PIECES + g_reach.piece[y * size + x];
                has[i] = 1;
                if (RM_GATE_AT(x, y) != RM_NO_GATE) g_reach.node_gate[i] = (int16_t)RM_GATE_AT(x, y);
            }
    for (int i = 0; i < n; i++) {
        if (!has[i]) continue;
        nodes++;
        if (g_reach.node_gate[i] >= 0) continue;   // a gate: in no region of the map's (rm_players joins through it)
        int c = i / RM_PIECES, cx = c % side, cy = c / side;
        for (int b = 0; b < 32; b++) {
            if (!(g_reach.link[i] >> b & 1)) continue;
            int d = b / 4, q = b % 4, j = ((cy + k_rm_dy[d]) * side + cx + k_rm_dx[d]) * RM_PIECES + q;
            if (g_reach.node_gate[j] >= 0) continue;
            int a = rm_find(up, i), z = rm_find(up, j);
            if (a != z) up[a < z ? z : a] = a < z ? a : z;   // the lower root: the same labels whatever the order
        }
    }
    int regions = 0;
    for (int i = 0; i < n; i++) g_reach.region[i] = -1;
    for (int i = 0; i < n; i++) {
        if (!has[i]) continue;
        int r = rm_find(up, i);
        if (g_reach.region[r] < 0) g_reach.region[r] = regions++;   // the root's label, numbered in node order
        g_reach.region[i] = g_reach.region[r];
    }
    g_reach.regions = regions; g_reach.nodes = nodes;
    g_reach.built = game_time();
    static ee_map *logged;   // once a map
    if (g_log && logged != map && (logged = map)) fprintf(g_log, "# reach map: %d tiles, %d x %d cells, %d nodes, %d regions, %d gates, built in %lu ms\n", size, side, side, nodes, regions, g_reach.gates, (unsigned long)(GetTickCount() - t0));
    free(g_reach.player_region);
    g_reach.player_region = malloc((size_t)RM_PLAYERS * (size_t)(regions ? regions : 1) * sizeof *g_reach.player_region);
    g_reach.players_built = g_reach.built - 1;   // rm_players works them out on first use
}
// each player's regions: the map's joined through the gates open to it - worked out on use, again after RM_GATE_MS
internal void rm_players(void) {
    uint32_t now = game_time();
    if (!g_reach.player_region || (g_reach.players_built == g_reach.built && now >= g_reach.players_at && now - g_reach.players_at < RM_GATE_MS)) return;
    g_reach.players_built = g_reach.built; g_reach.players_at = now;
    ee_map *map = g_reach.map;
    int size = g_reach.size, side = g_reach.side, regions = g_reach.regions;
    for (int k = 0; k < g_reach.gates; k++) {
        ee_unit *g = rm_gate_on(map_get_tiles(map)[g_reach.gate[k].y * size + g_reach.gate[k].x]);
        g_reach.gate_open[k] = 0;
        for (int p = 0; g && p < RM_PLAYERS; p++) if (rm_gate_open(g, p)) g_reach.gate_open[k] |= (uint16_t)(1u << p);
    }
    static ee_map *logged;   // once a map: how many gates let someone through, to how many players on average
    if (g_log && logged != map && g_reach.gates && (logged = map)) {
        int some = 0, bits = 0;
        for (int k = 0; k < g_reach.gates; k++) {
            some += g_reach.gate_open[k] != 0;
            for (int p = 0; p < RM_PLAYERS; p++) bits += g_reach.gate_open[k] >> p & 1;
        }
        fprintf(g_log, "# reach map: %d gates, %d open to someone, to %.1f players each\n", g_reach.gates, some, (double)bits / g_reach.gates);
    }
    static int up[RM_NODES];
    for (int p = 0; p < RM_PLAYERS; p++) {
        int *pr = g_reach.player_region + p * regions;
        for (int r = 0; r < regions; r++) up[r] = r;
        // a gate's nodes join each other's regions too: the first region met stands for the gate's
        static int gate_root[RM_GATES];
        for (int k = 0; k < g_reach.gates; k++) gate_root[k] = -1;
        for (int i = 0, n = side * side * RM_PIECES; i < n; i++) {
            int k = g_reach.node_gate[i];
            if (k < 0 || !(g_reach.gate_open[k] >> p & 1)) continue;
            int c = i / RM_PIECES, cx = c % side, cy = c / side;
            for (int b = 0; b < 32; b++) {
                if (!(g_reach.link[i] >> b & 1)) continue;
                int d = b / 4, j = ((cy + k_rm_dy[d]) * side + cx + k_rm_dx[d]) * RM_PIECES + b % 4;
                int r = g_reach.region[j];
                if (r < 0) continue;   // another gate's node
                if (gate_root[k] < 0) { gate_root[k] = r; continue; }
                int a = rm_find(up, gate_root[k]), z = rm_find(up, r);
                if (a != z) up[a < z ? z : a] = a < z ? a : z;
            }
        }
        for (int r = 0; r < regions; r++) pr[r] = rm_find(up, r);   // the lowest region of each join: deterministic
    }
}
// the reach map of the current map, built on first use (0: none - no map, or bigger than 1024 tiles)
internal int rm_ready(void) {
    ee_map *map = game_map();
    if (!map) return 0;
    uint32_t now = game_time();
    if (g_reach.map != map || g_reach.size != (int)map_get_size(map) || now < g_reach.built || now - g_reach.built >= RM_REBUILD_MS) rm_build(map);
    return g_reach.size > 0;
}
internal void rm_new_game(void) { g_reach.map = NULL; g_reach.size = 0; }   // the next game's map may sit at the same address
internal int rm_node(int x, int y) {   // the node of tile (x, y), -1 none (not walkable)
    if (!rm_ready() || x < 0 || y < 0 || x >= g_reach.size || y >= g_reach.size) return -1;
    uint8_t p = g_reach.piece[y * g_reach.size + x];
    return p == 0xFF ? -1 : ((y / RM_CELL) * g_reach.side + x / RM_CELL) * RM_PIECES + p;
}
// a node's region for player p (its regions joined through the gates open to it): a gate's node takes the region of a
// node beside it if the gate is open to p; -1 none
internal int rm_node_region_of(int p, int i) {
    if (i < 0 || p < 0 || p >= RM_PLAYERS || !g_reach.player_region) return -1;
    rm_players();
    int r = g_reach.region[i];
    if (r < 0) {
        int k = g_reach.node_gate[i];
        if (k < 0 || !(g_reach.gate_open[k] >> p & 1)) return -1;
        int c = i / RM_PIECES, cx = c % g_reach.side, cy = c / g_reach.side;
        for (int b = 0; b < 32 && r < 0; b++)
            if (g_reach.link[i] >> b & 1) r = g_reach.region[((cy + k_rm_dy[b / 4]) * g_reach.side + cx + k_rm_dx[b / 4]) * RM_PIECES + b % 4];
        if (r < 0) return -1;
    }
    return g_reach.player_region[p * g_reach.regions + r];
}
internal int rm_region_of(int p, int x, int y) { return rm_node_region_of(p, rm_node(x, y)); }

// THE CORRIDOR (general route_corridor, with land_routes): a land route's search (engine/shiproute.c sr_find) only
// over the cells of the coarse route through the reach map and RM_MARGIN cells round them - the citizens' long trips
// were 59% of the route search work in the player's late game, a whole-map search each. rm_corridor marks those cells
// (rm_in_corridor asks): 1 marked; 0 start and goal in different walking regions - no land route at all (the search
// filled every tile it could reach to learn that: 58000 tiles once, the picture stood 100 ms); -1 can't tell (no
// reach map, or a type that walks otherwise than a citizen - the map is a citizen's; -2 / -3 its start / goal off
// walkable land, 2 tiles round too). For player p: through the gates open to it (-1 also: at a gate shut to it).
// The coarse route is an A* over the nodes (a straight cell step 10, a diagonal 14, the octile estimate); buildings
// and walls aren't in the map, so a search the corridor fails is searched again without it (shiproute.c).
#define RM_MARGIN 3
internal struct {
    uint32_t gen, mark;
    uint32_t seen[RM_NODES], closed[RM_NODES], cell[RM_SIDE_MAX * RM_SIDE_MAX];
    int g[RM_NODES], parent[RM_NODES], heap[RM_NODES], heap_n;
    float f[RM_NODES];
} g_rmc;
internal void rmc_push(int i) {
    int k = g_rmc.heap_n++;
    while (k > 0) { int p = (k - 1) / 2; if (g_rmc.f[g_rmc.heap[p]] <= g_rmc.f[i]) break; g_rmc.heap[k] = g_rmc.heap[p]; k = p; }
    g_rmc.heap[k] = i;
}
internal int rmc_pop(void) {
    int top = g_rmc.heap[0], last = g_rmc.heap[--g_rmc.heap_n], k = 0;
    for (;;) {
        int c = 2 * k + 1;
        if (c >= g_rmc.heap_n) break;
        if (c + 1 < g_rmc.heap_n && g_rmc.f[g_rmc.heap[c + 1]] < g_rmc.f[g_rmc.heap[c]]) c++;
        if (g_rmc.f[g_rmc.heap[c]] >= g_rmc.f[last]) break;
        g_rmc.heap[k] = g_rmc.heap[c]; k = c;
    }
    if (g_rmc.heap_n) g_rmc.heap[k] = last;
    return top;
}
// the node of (x, y) or of the nearest walkable tile within 2 tiles of it, -1 none
internal int rm_node_near(int x, int y) {
    int n = rm_node(x, y);
    for (int r = 1; r <= 2 && n < 0; r++)
        for (int dy = -r; dy <= r && n < 0; dy++)
            for (int dx = -r; dx <= r && n < 0; dx += (abs(dy) == r ? 1 : 2 * r)) n = rm_node(x + dx, y + dy);
    return n;
}
internal int rm_walks_like_citizen(ee_unit_type *td) {
    static ee_unit_type *walker;
    if (!walker) walker = type_data(UT(CITIZEN));
    if (!td || !walker) return 0;
    if (td == walker) return 1;
    const uint8_t *a = unit_type_terrain_ok_at(td), *b = unit_type_terrain_ok_at(walker);
    for (int k = 0; k < 18; k++) if (g_reach.kind_on_map[k] && !a[k] != !b[k]) return 0;   // only the kinds this map has
    return 1;
}
internal int rm_corridor(int sx, int sy, int tx, int ty, ee_unit_type *td, int p) {
    if (!rm_walks_like_citizen(td) || unit_type_get_id(td) == 0xD1) return -1;   // 0xD1 passes enemy gates too
    int ns = rm_node_near(sx, sy), nt = rm_node_near(tx, ty);   // a citizen at its tree, a target beside a mine
    if (ns < 0) return -2;
    if (nt < 0) return -3;
    int rs = rm_node_region_of(p, ns), rt = rm_node_region_of(p, nt);
    if (rs < 0 || rt < 0) return -1;   // at a gate shut to it
    if (rs != rt) return 0;
    int side = g_reach.side, tcx = (nt / RM_PIECES) % side, tcy = (nt / RM_PIECES) / side;
    if (!++g_rmc.gen) { memset(g_rmc.seen, 0, sizeof g_rmc.seen); memset(g_rmc.closed, 0, sizeof g_rmc.closed); g_rmc.gen = 1; }
    g_rmc.heap_n = 0;
    g_rmc.seen[ns] = g_rmc.gen; g_rmc.g[ns] = 0; g_rmc.parent[ns] = -1; g_rmc.f[ns] = 0;
    rmc_push(ns);
    int found = 0;
    while (g_rmc.heap_n) {
        int i = rmc_pop();
        if (g_rmc.closed[i] == g_rmc.gen) continue;
        g_rmc.closed[i] = g_rmc.gen;
        if (i == nt) { found = 1; break; }
        int c = i / RM_PIECES, cx = c % side, cy = c / side;
        for (int b = 0; b < 32; b++) {
            if (!(g_reach.link[i] >> b & 1)) continue;
            int d = b / 4, j = ((cy + k_rm_dy[d]) * side + cx + k_rm_dx[d]) * RM_PIECES + b % 4;
            if (g_rmc.closed[j] == g_rmc.gen) continue;
            if (g_reach.node_gate[j] >= 0 && !(g_reach.gate_open[g_reach.node_gate[j]] >> p & 1)) continue;   // a gate shut to p
            int g = g_rmc.g[i] + ((d & 1) ? 14 : 10);
            if (g_rmc.seen[j] == g_rmc.gen && g >= g_rmc.g[j]) continue;
            g_rmc.seen[j] = g_rmc.gen; g_rmc.g[j] = g; g_rmc.parent[j] = i;
            int hx = abs(tcx - (cx + k_rm_dx[d])), hy = abs(tcy - (cy + k_rm_dy[d]));
            g_rmc.f[j] = (float)g + (float)(10 * (hx > hy ? hx - hy : hy - hx) + 14 * (hx < hy ? hx : hy));
            if (g_rmc.heap_n < RM_NODES) rmc_push(j);
        }
    }
    if (!found) return 0;   // the same region: can't happen - but no corridor then
    if (!++g_rmc.mark) { memset(g_rmc.cell, 0, sizeof g_rmc.cell); g_rmc.mark = 1; }
    for (int i = nt; i >= 0; i = g_rmc.parent[i]) {
        int c = i / RM_PIECES, cx = c % side, cy = c / side;
        for (int dy = -RM_MARGIN; dy <= RM_MARGIN; dy++)
            for (int dx = -RM_MARGIN; dx <= RM_MARGIN; dx++)
                if (cx + dx >= 0 && cy + dy >= 0 && cx + dx < side && cy + dy < side) g_rmc.cell[(cy + dy) * side + cx + dx] = g_rmc.mark;
        if (i == ns) break;
    }
    return 1;
}
internal int rm_in_corridor(int x, int y) {   // tile (x, y) in the corridor rm_corridor marked last
    return x >= 0 && y >= 0 && x < g_reach.size && y < g_reach.size && g_rmc.cell[(y / RM_CELL) * g_reach.side + x / RM_CELL] == g_rmc.mark;
}
