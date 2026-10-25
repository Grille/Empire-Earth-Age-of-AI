// THE MAP (EEMap: game_map()), its tiles, areas and unit grid: fields, functions.

// FIELDS
// A MAP TILE (the map's tile table, MAP +0x1816D4): its +0x10 is UNIT's
// +0x14 a signed class of its water for ships (a guess: depth): below 0 only a ship whose unit_get_sea_mode is 0 enters
// it, above 0 every ship but one of mode 3 (the naval movement's step test; the game's route search adds it for ships)
// +8 the objects on it: a list of nodes, each its object +4 and the next node +8
internal inline ee_unit *tilenode_get_object(const void *node) { return FIELD(node, of_tile_node_object, ee_unit *); }
internal inline void *tilenode_get_next(const void *node) { return FIELD(node, of_tile_node_next, void *); }

// THE MAP ([G_MAP], game_map(): EEMap)
// the view table: per tile 4 bytes, the first 2 the viewed player's sight count (kept beside the tiles' own: 0x55EA8B
// adds as FN_TILE_SIGHT_ADD does, 0x55E904 / 0x5F896F take one off, no floor), bytes 2 and 3 other uses - a row per
// y of {first x, length, data}
internal inline void *map_view_row(const ee_map *map, unsigned y) { return (uint8_t *)map_get_view_rows(map) + y * 0xC; }
internal inline uint16_t *viewrow_count_at(const void *row, unsigned x) {
    unsigned i = x - FIELD(row, of_view_row_start, unsigned);
    return i < FIELD(row, of_view_row_len, unsigned) ? (uint16_t *)(FIELD(row, of_view_row_data, uint8_t *) + i * 4) : NULL;
}
// the map's diamond: a tile with x + y = edge_sum or edge_sum_2, or y - x or x - y = edge_diff, is on its edge (no
// unit enters it - the path search's tile test)
// the unit grid (map +0x181724, unit_grid()): cells of cs tiles, nx x ny of them; a cell's units begin .. end
internal inline ee_grid_cell *grid_cell_at(ee_grid_cell *cells, int i) { return (ee_grid_cell *)((uint8_t *)cells + i * 16); }   // cell (gx, gy): gx * ny + gy
internal inline ee_unit **cell_get_units_end(const ee_grid_cell *cell) { return FIELD(cell, of_cell_units_begin + 0x4, ee_unit **); }
// a resource site's ownership record (unit_get_vis(site)): +4 1 while the site is flagged (taken)
// a unit's visibility: a uint16 per vision slot (player +0x45C, the player's index), its low byte != 0 seen by it
// THE SIGHT COUNT. That record is the unit's map tile (TILE_SIGHT the same field), the slot its sight count: 0
// unexplored, 1 explored, 2 + n while n units of the player see it. Every reader of the game takes its low byte only
// (FN_EXPLORED, the draw list's collection 0x503B83, the fog's ...), and its add restarts a low byte of 0 at 2: a tile
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

// FUNCTIONS
// the map's area id at pt (kind 1 water, 2 land): one id per connected region
internal inline unsigned short map_area(ivec2 p, int kind) {
    return fn_region_game((void *)data_map_map_areas, &p.x, kind);   // &p.x: the game's int pos[2]
}
// the map's area object at a tile (regions, continents)
internal inline uint8_t *area_at(int *pt) { return fn_area_at_game((void *)data_map_map_areas, pt); }
// the minimap (RTSMiniMap): a unit more sees the tile (x, y) - its pixel's own count of them +1

// HOOKS
// a tile's sight count for the player: a unit more (FN_TILE_SIGHT_ADD) or less (FN_TILE_SIGHT_SUB) sees it - every
// change of the count but its setter (0x51979C: unexplored, explored, the fog's reset)
typedef void (__thiscall *tile_sight_hook)(ee_tile *tile, ee_player *pl);
internal inline int patch_tile_sight(tile_sight_hook add, tile_sight_hook sub) {
    return patch_all_calls(data_map_fn_tile_sight_add, (void *)add) + patch_all_calls(data_map_fn_tile_sight_sub, (void *)sub);
}
// the minimap's rebuild (0x5150F2: the viewed player changed, a new game) takes each tile's count from its low byte
typedef void (__thiscall *minimap_tile_hook)(void *mm, ee_tile *tile, int x, int y);
internal inline int patch_minimap_tile(minimap_tile_hook h) { return patch_all_calls((uintptr_t)fn_minimap_tile, (void *)h); }
// the view table's refill from the tiles' counts (FN_VIEW_SIGHT_COPY: a load, the viewed player changed) - their low byte
typedef void (__thiscall *view_sight_copy_hook)(ee_player *pl);
internal inline int patch_view_sight_copy(view_sight_copy_hook h) { return patch_all_calls((uintptr_t)fn_view_sight_copy, (void *)h); }
