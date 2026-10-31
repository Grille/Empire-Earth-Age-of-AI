// REACH MAP: the map's terrain as a coarse graph - which parts of it a land unit can walk between (the player
// 2026-10-31: "can land and/or sea units path from this side of it to that? the value for each side pair cached - it
// could also speed up pathfinding"). The map in RM_CELL-tile cells; a cell's walkable tiles fall into pieces (connected
// inside the cell: a river, a cliff or a shore through it makes two), each piece a node; a node links to the pieces of
// the 8 cells round it that a step reaches (a diagonal step only past two walkable tiles, as a unit squeezes through);
// the nodes joined by links are a region - one lookup answers "can it walk from here to there". Built once a map
// from the terrain (a citizen's terrain table, as the route planner asks, inside the map's diamond - one-tile fords of
// shallow water walkable): deterministic, the same on every PC. Not yet: buildings and walls (they come and go), ships.
// The game's own AI areas (EEAIContinentManager, map_areas) give a region per area too, but areas are irregular and
// their neighbours unknown - this one is the mod's, the same in both builds. Used by ai/fronts.c.
#define RM_CELL 4
#define RM_PIECES 4              // pieces kept per cell; a fifth and more join the fourth (rare: a maze of water)
#define RM_SIDE_MAX 256          // a 1024-tile map
#define RM_NODES (RM_SIDE_MAX * RM_SIDE_MAX * RM_PIECES)
internal struct {
    ee_map *map; int size, side;           // the map it was built for
    uint8_t piece[1024 * 1024];            // per tile: its piece in its cell, 0xFF not walkable
    uint32_t link[RM_NODES];               // per node: bit d * 4 + p - the neighbour cell in direction d, its piece p
    int region[RM_NODES];                  // per node: its region (-1 no node)
    int regions, nodes;
} g_reach;
internal const int8_t k_rm_dx[8] = { 1, 1, 0, -1, -1, -1, 0, 1 }, k_rm_dy[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
internal int rm_tile_ok(ee_map *map, int size, int x, int y) {   // may a citizen stand there
    if (x < 0 || y < 0 || x >= size || y >= size) return 0;
    if (x + y <= map_get_edge_sum(map) || x + y >= map_get_edge_sum_2(map) || y - x >= map_get_edge_diff(map) || x - y >= map_get_edge_diff(map)) return 0;
    ee_tile *tile = map_get_tiles(map)[y * size + x];
    static ee_unit_type *walker;
    if (!walker) walker = type_data(UT(CITIZEN));
    return tile && walker && unit_type_terrain_ok(walker, tile_get_kind(tile));
}
internal int rm_find(int *up, int a) { while (up[a] != a) a = up[a] = up[up[a]]; return a; }
internal void rm_build(ee_map *map) {
    int size = (int)map_get_size(map);
    if (size <= 0 || size > 1024) { g_reach.map = map; g_reach.size = 0; return; }   // a bigger map: none (rm_ready says so)
    int side = (size + RM_CELL - 1) / RM_CELL;
    DWORD t0 = GetTickCount();   // the log's only: nothing decided by time
    g_reach.map = map; g_reach.size = size; g_reach.side = side;
    memset(g_reach.piece, 0xFF, (size_t)size * (size_t)size);
    static short stack[RM_CELL * RM_CELL * 2];
    // pieces: each cell's walkable tiles flooded inside it
    for (int cy = 0; cy < side; cy++)
        for (int cx = 0; cx < side; cx++) {
            int x0 = cx * RM_CELL, y0 = cy * RM_CELL, np = 0;
            for (int y = y0; y < y0 + RM_CELL && y < size; y++)
                for (int x = x0; x < x0 + RM_CELL && x < size; x++) {
                    if (g_reach.piece[y * size + x] != 0xFF || !rm_tile_ok(map, size, x, y)) continue;
                    uint8_t p = (uint8_t)(np < RM_PIECES ? np++ : RM_PIECES - 1);
                    int sp = 0;
                    g_reach.piece[y * size + x] = p;
                    stack[sp++] = (short)x; stack[sp++] = (short)y;
                    while (sp) {
                        int ty = stack[--sp], tx = stack[--sp];
                        for (int d = 0; d < 8; d++) {
                            int nx = tx + k_rm_dx[d], ny = ty + k_rm_dy[d];
                            if (nx < x0 || ny < y0 || nx >= x0 + RM_CELL || ny >= y0 + RM_CELL || nx >= size || ny >= size) continue;
                            if (g_reach.piece[ny * size + nx] != 0xFF || !rm_tile_ok(map, size, nx, ny)) continue;
                            if ((d & 1) && (!rm_tile_ok(map, size, tx + k_rm_dx[d], ty) || !rm_tile_ok(map, size, tx, ty + k_rm_dy[d]))) continue;
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
    for (int i = 0; i < n; i++) up[i] = i;
    static uint8_t has[RM_NODES];
    memset(has, 0, (size_t)n);
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++)
            if (g_reach.piece[y * size + x] != 0xFF) has[((y / RM_CELL) * side + x / RM_CELL) * RM_PIECES + g_reach.piece[y * size + x]] = 1;
    for (int i = 0; i < n; i++) {
        if (!has[i]) continue;
        nodes++;
        int c = i / RM_PIECES, cx = c % side, cy = c / side;
        for (int b = 0; b < 32; b++) {
            if (!(g_reach.link[i] >> b & 1)) continue;
            int d = b / 4, q = b % 4, j = ((cy + k_rm_dy[d]) * side + cx + k_rm_dx[d]) * RM_PIECES + q;
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
    if (g_log) fprintf(g_log, "# reach map: %d tiles, %d x %d cells, %d nodes, %d regions, built in %lu ms\n", size, side, side, nodes, regions, (unsigned long)(GetTickCount() - t0));
}
// the reach map of the current map, built on first use (0: none - no map, or bigger than 1024 tiles)
internal int rm_ready(void) {
    ee_map *map = game_map();
    if (!map) return 0;
    if (g_reach.map != map || g_reach.size != (int)map_get_size(map)) rm_build(map);
    return g_reach.size > 0;
}
internal void rm_new_game(void) { g_reach.map = NULL; g_reach.size = 0; }   // the next game's map may sit at the same address
internal int rm_node(int x, int y) {   // the node of tile (x, y), -1 none (not walkable)
    if (!rm_ready() || x < 0 || y < 0 || x >= g_reach.size || y >= g_reach.size) return -1;
    uint8_t p = g_reach.piece[y * g_reach.size + x];
    return p == 0xFF ? -1 : ((y / RM_CELL) * g_reach.side + x / RM_CELL) * RM_PIECES + p;
}
internal int rm_region(int x, int y) { int n = rm_node(x, y); return n < 0 ? -1 : g_reach.region[n]; }   // -1 not walkable
