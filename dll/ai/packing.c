// PACKED BUILDINGS ([ai_base] tight_build, default 0 until measured; the player 2026-11-09: "it places each building
// with an 1 tile empty border around it, while human players tightly pack them, the ai build planer should try to put
// them together, with a small check that it doesnt block paths"). The game's search (0x4874F6) tests a building's square
// and one tile round it (0x4686E0, its fifth argument), and for the production buildings walks its rings from the far
// edge of the town inward. A build manager's entry with a position skips that search, so hook_place (econ.c) asks here
// first: the production buildings go to the free spot touching the most of their own kind, nearest the base. No spot:
// the game's search, as before. Houses (one tile wide) aren't this search's: the game's planner finds them a spot of its
// own (0x4874F6 for type 37, the one call without the border) and queues it with the entry - they touch already.
// A spot may not cut a way: the tiles round the footprint, walked as a ring, must have their walkable ones in one
// unbroken arc. Whatever walked through the footprint then walks round it along that arc, so nothing the building
// stands on can part two sides of the map (a building between two others has two arcs, and waits for another spot).
// That says nothing of width: a tile free beside the building with something standing a tile beyond it is a lane one
// tile wide, which a big unit can't use - a spot touches or keeps two tiles. A row ends at TB_BLOCK tiles (a long
// wall of buildings is a walk round), drop sites keep their room for fields and carriers, mines and trees their
// gatherers', a gate its way. Towers, walls, docks, drop sites, Temples, Universities and Hospitals (their search
// spreads their reach over the towns) keep the game's or the defence plan's spots.
#define TB_R 32           // tiles from the base looked at
#define TB_MORE 8         // once a spot is found, this many rings further out
#define TB_MAX_SIZE 8     // the biggest footprint looked at
#define TB_TOUCH 8        // a touching tile is worth this many tiles of distance from the base
#define TB_BLOCK 14       // a row of buildings, at most
#define TB_CLEAR_DROP 6   // tiles left to a drop site
#define TB_CLEAR_MINE 4   // to a mine or forage
#define TB_CLEAR_TREE 2   // to a tree
#define TB_CLEAR_GATE 2
#define TB_LAYOUT 64      // buildings the log's layout line looks at
internal int can_place(ee_player *pl, int type, ivec2 p);    // defence.c
internal int raiders_near(ee_player *pl, ivec2 p, int r);    // attack.c
internal unsigned area_id(ivec2 p, int kind);                // forces.c
internal int g_tight_build = 0, g_v_tight_build = -1;
internal int tb_type(int t) {   // those this places
    return UT_IN(t, UT_B_BARRACKS, UT_B_ARCHERY_RANGE, UT_B_STABLE, UT_B_SIEGE_FACTORY, UT_B_AIRPORT, UT_B_TANK_FACTORY, UT_B_CYBER_FACTORY);
}
internal int tb_kin(int t) { return tb_type(t) || UT_IN(t, UT_B_HOUSE, UT_B_HOSPITAL, UT_B_TEMPLE, UT_B_UNIVERSITY); }   // those it may touch
internal ee_unit *tb_kin_on(ee_map *map, int size, int x, int y, ee_player *pl) {   // the player's building to pack against on that tile, NULL none
    if (x < 0 || y < 0 || x >= size || y >= size) return NULL;
    ee_tile *tile = map_get_tiles(map)[y * size + x];
    for (void *node = tile ? tile_get_objects(tile) : NULL; node; node = tilenode_get_next(node)) {
        ee_unit *o = tilenode_get_object(node);
        ee_unit_type *otd = o ? unit_get_type_data(o) : NULL;
        if (otd && unit_get_owner(o) == pl && tb_kin(unit_type_get_id(otd))) return o;
    }
    return NULL;
}
// the ring round the footprint (x, y, s): one arc of walkable tiles, no lane a tile wide; *touch the ring's tiles held
// by the player's own buildings
internal int tb_ring_ok(ee_map *map, int size, int x, int y, int s, ee_player *pl, int *touch) {
    static const int8_t dx[4] = { 0, 1, 0, -1 }, dy[4] = { -1, 0, 1, 0 };   // outward, side by side
    uint8_t f[4 * (TB_MAX_SIZE + 1)];
    int n = 0, walkable = 0;
    *touch = 0;
    for (int side = 0; side < 4; side++) {
        // each side's first tile is its corner: side 0 from (x-1, y-1) along +x, 1 from (x+s, y-1) along +y ...
        int rx = side == 0 || side == 3 ? x - 1 : x + s, ry = side < 2 ? y - 1 : y + s;
        int sx = -dy[side], sy = dx[side];
        for (int i = 0; i <= s; i++) {
            int tx = rx + sx * i, ty = ry + sy * i, ok = rm_tile_ok(map, size, tx, ty) != 0;
            f[n++] = (uint8_t)ok;
            walkable += ok;
            if (!ok && tb_kin_on(map, size, tx, ty, pl)) ++*touch;
            if (ok && i && !rm_tile_ok(map, size, tx + dx[side], ty + dy[side])) return 0;   // beside the footprint, not its corner: a lane
        }
    }
    if (!walkable) return 0;
    if (walkable < n) {
        int arcs = 0;
        for (int i = 0; i < n; i++) arcs += f[i] && !f[(i + 1) % n];
        if (arcs != 1) return 0;
    }
    return walkable >= s + 2;   // the builders' and the new units' way out
}
// nothing near it that wants the room
internal int tb_clear(ee_map *map, int size, int x, int y, int s) {
    int r = TB_CLEAR_DROP;
    for (int ty = y - r; ty < y + s + r; ty++)
        for (int tx = x - r; tx < x + s + r; tx++) {
            if (tx < 0 || ty < 0 || tx >= size || ty >= size) continue;
            int ax = tx < x ? x - tx : tx >= x + s ? tx - x - s + 1 : 0, ay = ty < y ? y - ty : ty >= y + s ? ty - y - s + 1 : 0, d = ax > ay ? ax : ay;
            ee_tile *tile = map_get_tiles(map)[ty * size + tx];
            for (void *node = tile ? tile_get_objects(tile) : NULL; node; node = tilenode_get_next(node)) {
                ee_unit *o = tilenode_get_object(node);
                ee_unit_type *otd = o ? unit_get_type_data(o) : NULL;
                if (!otd) continue;
                int t = unit_type_get_id(otd), cls = unit_type_get_class_id(otd);
                if (UT_IN(t, UT_TOWNS, UT_B_GRANARY, UT_B_GRANARY_IMPROVED)) return 0;   // within TB_CLEAR_DROP: the box
                if (cls == CLS_RESOURCE && d <= (unit_type_get_resource_type(otd) == 3 ? TB_CLEAR_TREE : TB_CLEAR_MINE)) return 0;
                if (cls == CLS_GATE && d <= TB_CLEAR_GATE) return 0;
            }
        }
    return 1;
}
internal int tb_row_ok(ee_map *map, int size, int x, int y, int s) {   // the row and the column through its middle
    int mx = x + s / 2, my = y + s / 2, a = 0, b = 0;
    while (a <= TB_BLOCK && !rm_tile_ok(map, size, x - 1 - a, my)) a++;
    while (b <= TB_BLOCK && !rm_tile_ok(map, size, x + s + b, my)) b++;
    if (a + b + s > TB_BLOCK) return 0;
    a = b = 0;
    while (a <= TB_BLOCK && !rm_tile_ok(map, size, mx, y - 1 - a)) a++;
    while (b <= TB_BLOCK && !rm_tile_ok(map, size, mx, y + s + b)) b++;
    return a + b + s <= TB_BLOCK;
}
// a spot for type, packed against the player's other buildings near its base: 1 and *ox, *oy, or 0
internal int tb_spot(ee_build_mgr *bm, ee_player *pl, int type, int *ox, int *oy) {
    if (!tb_type(type) || !rm_ready() || !g_reach.walker) return 0;
    int bx = build_mgr_get_base_x(bm), by = build_mgr_get_base_y(bm);
    ee_map *map = game_map();
    ee_unit_type *td = type_data(type);
    int size = map ? (int)map_get_size(map) : 0, s = td ? unit_type_get_size(td) : 0;
    if (bx == -10000 || !size || s < 1 || s > TB_MAX_SIZE) return 0;
    unsigned land = area_id(iv2(bx, by), 2);
    if (!land) return 0;
    int found = 0, best = 0, best_r = 0, best_touch = 0;
    for (int r = 2; r <= TB_R && !(found && r > best_r + TB_MORE); r++)
        for (int ax = -r; ax <= r; ax++)
            for (int ay = -r; ay <= r; ay++) {
                if (abs(ax) != r && abs(ay) != r) continue;
                int x = bx + ax - s / 2, y = by + ay - s / 2, touch;
                if (!can_place(pl, type, iv2(x, y)) || area_id(iv2(x, y), 2) != land || area_id(iv2(x, y), 1)) continue;
                if (!tb_ring_ok(map, size, x, y, s, pl, &touch)) continue;
                int score = touch * TB_TOUCH - r;
                if ((found && score <= best) || !tb_clear(map, size, x, y, s) || !tb_row_ok(map, size, x, y, s)) continue;
                if (raiders_near(pl, iv2(x, y), 12) || build_mgr_enemy_near(bm, x, y)) continue;
                found = 1; best = score; best_r = r; best_touch = touch; *ox = x; *oy = y;
            }
    if (found && g_log) fprintf(g_log, "# packed: P%d at %.1f min: type %d size %d at %d,%d touching %d tiles, %d from the base\n",
                                player_get_index(pl) & 15, game_time() / 60000.0, type, s, *ox, *oy, best_touch, best_r);
    return found;
}
// the log's "# layout:" (every 20 minutes, the setting on or off): how close the player's production buildings stand - each one's gap to its nearest (0: touching), and how far from the base
internal void tb_layout(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    ee_build_mgr *bm = NULL;
    void *cm = player_get_civ_mgr(pl);
    if (cm) bm = civ_mgr_get_build_mgr(cm);
    if (!g_log || !bm || now / 1200000 == last[idx] / 1200000 || build_mgr_get_base_x(bm) == -10000) return;
    last[idx] = now;
    int x[TB_LAYOUT], y[TB_LAYOUT], s[TB_LAYOUT], n = 0, count = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 8192; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (n == TB_LAYOUT || !td || unit_get_hp(u) <= 0 || !tb_type(unit_type_get_id(td))) continue;
        x[n] = unit_get_x(u); y[n] = unit_get_y(u); s[n] = unit_type_get_size(td); n++;
    }
    int touching = 0, gaps = 0, most = 0;
    float dist = 0.0f;
    for (int i = 0; i < n; i++) {
        int g = 1 << 20;
        for (int j = 0; j < n; j++) {
            if (i == j) continue;
            int ax = x[j] >= x[i] + s[i] ? x[j] - x[i] - s[i] : x[i] >= x[j] + s[j] ? x[i] - x[j] - s[j] : 0;
            int ay = y[j] >= y[i] + s[i] ? y[j] - y[i] - s[i] : y[i] >= y[j] + s[j] ? y[i] - y[j] - s[j] : 0;
            int d = ax > ay ? ax : ay;
            if (d < g) g = d;
        }
        if (n > 1) { gaps += g; touching += g == 0; }
        int dx = x[i] - build_mgr_get_base_x(bm), dy = y[i] - build_mgr_get_base_y(bm), d = (int)sqrtf((float)(dx * dx + dy * dy));
        dist += (float)d;
        if (d > most) most = d;
    }
    fprintf(g_log, "# layout: P%u at %u min: %d buildings, %d touching, gap to the nearest mean %.1f, from the base mean %.1f, farthest %d\n",
            idx, now / 60000, n, touching, n > 1 ? (float)gaps / (float)n : 0.0f, n ? dist / (float)n : 0.0f, most);
}
