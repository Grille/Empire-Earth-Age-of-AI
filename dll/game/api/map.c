// THE MAP (EEMap: game_map()), its tiles, areas and unit grid: fields, functions.

// FIELDS
// A MAP TILE (the map's tile table, MAP +0x1816D4): its +0x10 is UNIT's
// +0x14 a signed class of its water for ships (a guess: depth): below 0 only a ship whose unit_get_sea_mode is 0 enters
// it, above 0 every ship but one of mode 3 (the naval movement's step test; the game's route search adds it for ships)
// +8 the objects on it: a list of nodes, each its object +4 and the next node +8
internal inline ee_unit *tilenode_get_object(const void *node) { return FIELD(node, of_tile_node_object, ee_unit *); }
internal inline void *tilenode_get_next(const void *node) { return FIELD(node, of_tile_node_next, void *); }

// THE MAP ([data_game_map], game_map(): EEMap)
// the view table: per tile 4 bytes, the first 2 the viewed player's sight count (kept beside the tiles' own: 0x55EA8B
// adds as data_map_fn_tile_sight_add does, 0x55E904 / 0x5F896F take one off, no floor), bytes 2 and 3 other uses - a row per
// y of {first x, length, data}
internal inline void *map_view_row(const ee_map *map, unsigned y) { return (uint8_t *)map_get_view_rows(map) + y * 0xC; }
internal inline uint16_t *viewrow_count_at(const void *row, unsigned x) {
    unsigned i = x - FIELD(row, of_view_row_start, unsigned);
    return i < FIELD(row, of_view_row_len, unsigned) ? (uint16_t *)(FIELD(row, of_view_row_data, uint8_t *) + i * 4) : NULL;
}
// the ground's corner x, y (a tile's corners are x, y .. x + 1, y + 1), NULL off the map: a row of {first x, length,
// corners} per corner y, 0xC a corner - the map's diamond, short rows at its top and bottom
internal inline const ee_corner_row *map_corner_row(const ee_map *map, unsigned y) {
    return (const ee_corner_row *)((uint8_t *)map_get_corner_rows(map) + y * 0xC);
}
// a row's i-th corner (i < its length)
internal inline ee_corner *corner_row_at(const ee_corner_row *row, unsigned i) {
    return (ee_corner *)((uint8_t *)corner_row_get_corners(row) + i * size_ee_corner);
}
internal inline ee_corner *map_corner(const ee_map *map, int x, int y) {
    if (y < 0 || (unsigned)y > map_get_size(map)) return NULL;
    const ee_corner_row *row = map_corner_row(map, (unsigned)y);
    unsigned i = (unsigned)(x - (int)corner_row_get_start(row));
    return i < corner_row_get_len(row) ? corner_row_at(row, i) : NULL;
}
// its height (tiles), NAN off the map
internal inline float ground_corner(const ee_map *map, int x, int y) {
    const ee_corner *c = map_corner(map, x, y);
    return c ? corner_get_height(c) : NAN;
}
// the ground's height at a point (tiles), from its tile's four corners - exact on a tile's edges and corners, where a
// wall piece's ends lie; NAN off the map
internal inline float ground_at(const ee_map *map, float x, float y) {
    int ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - (float)ix, fy = y - (float)iy;
    float a = ground_corner(map, ix, iy), b = ground_corner(map, ix + 1, iy), c = ground_corner(map, ix, iy + 1), d = ground_corner(map, ix + 1, iy + 1);
    return (a * (1 - fx) + b * fx) * (1 - fy) + (c * (1 - fx) + d * fx) * fy;
}
// the map's diamond: a tile with x + y = edge_sum or edge_sum_2, or y - x or x - y = edge_diff, is on its edge (no
// unit enters it - the path search's tile test)
// the unit grid (map +0x181724, unit_grid()): cells of cs tiles, nx x ny of them; a cell's units begin .. end
internal inline ee_grid_cell *grid_cell_at(ee_grid_cell *cells, int i) { return (ee_grid_cell *)((uint8_t *)cells + i * 16); }   // cell (gx, gy): gx * ny + gy
internal inline ee_unit **grid_cell_get_units_end(const ee_grid_cell *cell) { return FIELD(cell, of_grid_cell_units_begin + 0x4, ee_unit **); }
// a resource site's ownership record (unit_get_vis(site)): +4 1 while the site is flagged (taken)
// a unit's visibility: a uint16 per vision slot (player +0x45C, the player's index), its low byte != 0 seen by it
// THE SIGHT COUNT. That record is the unit's map tile (TILE_SIGHT the same field), the slot its sight count: 0
// unexplored, 1 explored, 2 + n while n units of the player see it. Every reader of the game takes its low byte only
// (unit_vis_explored, the draw list's collection 0x503B83, the fog's ...), and its add restarts a low byte of 0 at 2: a tile
// seen by 255 units turned unexplored, and its count fell behind for good. engine/sight.c keeps the count so that its
// low byte means the same - 0, 1, or 2 .. 255 for any count from 2 on, the rest in the high byte -; sight_count reads
// it (a count below 256 is stored as the game stores it).
#define SIGHT_MAX 65025u   // 0xFFFF
internal inline unsigned sight_count(uint16_t raw) {
    unsigned lo = raw & 0xFFu;
    return lo < 2 ? lo : 2u + (raw >> 8) * 254u + (lo - 2u);
}
internal inline uint16_t sight_raw(unsigned count) {
    if (count < 2) return (uint16_t)count;
    unsigned d = count - 2;
    return (uint16_t)((d / 254u) << 8 | (2u + d % 254u));
}

// THE GROUND'S COLOURS (the map's mesh, map_get_mesh: Low-Level Engine.dll's GETerrainMesh): a corner's colour is a
// slot of the mesh's table (corner_get_color), the table 256 slots of 3 floats, the colour sets each a list of slots
internal inline float *mesh_color(void *mesh, unsigned slot) { return (float *)FIELD_AT(mesh, value_map_of_mesh_color_table) + slot * 3; }
// the mesh's colour sets: the first (0x10 each) and their count
internal inline uint8_t *mesh_color_sets(void *mesh, unsigned *n) {
    uint8_t *first = FIELD(mesh, value_map_of_mesh_color_sets + 4, uint8_t *), *end = FIELD(mesh, value_map_of_mesh_color_sets + 8, uint8_t *);
    *n = first ? (unsigned)(end - first) / 0x10 : 0;
    return first;
}
// set i's slots and their count
internal inline uint32_t *color_set_slots(uint8_t *sets, unsigned i, unsigned *n) {
    uint32_t *first = FIELD(sets + i * 0x10, 4, uint32_t *), *end = FIELD(sets + i * 0x10, 8, uint32_t *);
    *n = first ? (unsigned)(end - first) : 0;
    return first;
}
// the visible ground made anew at the next frame - with the table's colours now
internal inline void terrain_refresh(void) { *(volatile uint8_t *)map_terrain_current_at() = 0; }
// a colour type's record's size (map.color_types' records: a colour type of the mod's own in memory of its own)
internal inline unsigned color_type_size(void) { return size_ee_color_type; }
// a colour set more in the mesh, of n slots (RegisterColorSet copies them from an MSVC vector: its allocator, first,
// end, capacity end); its index
internal inline unsigned mesh_register_color_set(void *mesh, uint32_t *slots, unsigned n) {
    void *v[4] = { NULL, slots, slots + n, slots + n };
    unsigned long index = 0;
    ((long (__thiscall *)(void *, void *, unsigned long *))*(void **)data_map_iat_register_color_set)(mesh, v, &index);
    return (unsigned)index;
}

// FUNCTIONS
// the map's area id at pt (kind 1 water, 2 land): one id per connected region
internal inline unsigned short map_area(ivec2 p, int kind) {
    return fn_continent_mgr_region((void *)data_map_map_areas, &p.x, kind);   // &p.x: the game's int pos[2]
}
// the map's area object at a tile (regions, continents)
internal inline uint8_t *area_at(int *pt) { return fn_continent_mgr_area_at((void *)data_map_map_areas, pt); }
// the minimap (RTSMiniMap): a unit more sees the tile (x, y) - its pixel's own count of them +1

// HOOKS
// a tile's sight count for the player: a unit more (data_map_fn_tile_sight_add) or less (data_map_fn_tile_sight_sub) sees it - every
// change of the count but its setter (0x51979C: unexplored, explored, the fog's reset)
typedef void (__thiscall *tile_sight_hook)(ee_tile *tile, ee_player *pl);
internal inline int patch_tile_sight(tile_sight_hook add, tile_sight_hook sub) {
    return patch_all_calls(data_map_fn_tile_sight_add, (void *)add) + patch_all_calls(data_map_fn_tile_sight_sub, (void *)sub);
}
// the minimap's rebuild (0x5150F2: the viewed player changed, a new game) takes each tile's count from its low byte
typedef void (__thiscall *minimap_tile_hook)(void *mm, ee_tile *tile, int x, int y);
internal inline int patch_minimap_tile(minimap_tile_hook h) { return patch_all_calls((uintptr_t)fn_minimap_tile, (void *)h); }
// the view table's refill from the tiles' counts (player_view_sight_copy: a load, the viewed player changed) - their low byte
typedef void (__thiscall *view_sight_copy_hook)(ee_player *pl);
internal inline int patch_view_sight_copy(view_sight_copy_hook h) { return patch_all_calls((uintptr_t)fn_player_view_sight_copy, (void *)h); }
// a new map's colour table handed to its mesh (data_map_iat_color_table: the exe's import of SetColorTable, its colour
// sets registered just before) - the original into *orig
typedef void (__thiscall *mesh_color_table_fn)(void *mesh, void *colors);
internal inline int patch_mesh_color_table(mesh_color_table_fn h, mesh_color_table_fn *orig) {
    return iat_hook(data_map_iat_color_table, "Low-Level Engine.dll",
                    "?SetColorTable@GETerrainMesh@@QAEXAAV?$vector@VGEColor@@V?$allocator@VGEColor@@@std@@@std@@@Z", (void *)h, (void **)orig);
}
// the mesh's SetColorSet and SetWaterColorSet through the exe's imports
typedef void (__thiscall *mesh_set_color_set_fn)(void *mesh, unsigned long x, unsigned long y, unsigned long set);
typedef void (__thiscall *mesh_set_water_color_set_fn)(void *mesh, unsigned long x, unsigned long y);
internal inline int patch_mesh_set_color_set(mesh_set_color_set_fn h, mesh_set_color_set_fn *orig) {
    return iat_hook(data_map_iat_set_color_set, "Low-Level Engine.dll", "?SetColorSet@GETerrainMesh@@QAEXKKK@Z", (void *)h, (void **)orig);
}
internal inline int patch_mesh_set_water_color_set(mesh_set_water_color_set_fn h, mesh_set_water_color_set_fn *orig) {
    return iat_hook(data_map_iat_set_water_color_set, "Low-Level Engine.dll", "?SetWaterColorSet@GETerrainMesh@@QAEXKK@Z", (void *)h, (void **)orig);
}
// a cliff's kind: a tile's, the one a texture gives its tiles
internal inline int tile_is_cliff(const ee_tile *t) { return tile_get_kind(t) == (int)value_map_kind_cliff; }
internal inline int terrain_is_cliff(const ee_terrain *t) { return terrain_get_kind(t) == (int)value_map_kind_cliff; }
internal inline void tile_make_cliff(ee_tile *t, int *pos, void *map) { tile_change_kind(t, (int)value_map_kind_cliff, pos, map); }
// a tile's texture (its byte 2 in the view table), -1 off the map
internal inline int map_tile_texture(const ee_map *map, int x, int y) {
    if (y < 0 || (unsigned)y >= map_get_size(map)) return -1;
    const uint16_t *at = viewrow_count_at(map_view_row(map, (unsigned)y), (unsigned)x);
    return at ? ((const uint8_t *)at)[2] : -1;
}
// a unit's place within the map's diamond (the square's corners outside it are no map: the game never puts a unit
// there, and the minimap indexes its 64 x 64 tables with such a place unchecked - spawned units in a corner wrote
// over its dot lists and the game crashed a minute later, 0x516079 / 0x516235)
internal int spawn_on_map(ee_map *map, const float *xy) {
    int x = (int)xy[0], y = (int)xy[1];
    return x + y > map_get_edge_sum(map) && x + y < map_get_edge_sum_2(map) && abs(x - y) < map_get_edge_diff(map);
}
