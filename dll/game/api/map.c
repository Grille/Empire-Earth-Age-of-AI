// THE MAP (EEMap: game_map()), its tiles, areas and unit grid: fields, functions.

// FIELDS
// A MAP TILE (the map's tile table, MAP +0x1816D4): its +0x10 is UNIT's
internal inline float tile_get_height(const ee_tile *tile) { return FIELD(tile, TILE_HEIGHT, float); }
internal inline int tile_get_kind(const ee_tile *tile) { return FIELD(tile, TILE_KIND, int); }   // +4 its terrain kind (type_terrain_ok)
// +0x14 a signed class of its water for ships (a guess: depth): below 0 only a ship whose unit_get_sea_mode is 0 enters
// it, above 0 every ship but one of mode 3 (the naval movement's step test; the game's route search adds it for ships)
internal inline int tile_get_sea_class(const ee_tile *tile) { return FIELD(tile, TILE_SEA_CLASS, int8_t); }
// +8 the objects on it: a list of nodes, each its object +4 and the next node +8
internal inline void *tile_get_objects(const ee_tile *tile) { return FIELD(tile, TILE_OBJECTS, void *); }
internal inline ee_unit *tilenode_get_object(const void *node) { return FIELD(node, TILENODE_OBJECT, ee_unit *); }
internal inline void *tilenode_get_next(const void *node) { return FIELD(node, TILENODE_NEXT, void *); }

// THE MAP ([G_MAP], game_map(): EEMap)
internal inline ee_tile **map_get_tiles(const ee_map *map) { return FIELD(map, MAP_TILES, ee_tile **); }   // [y * size + x]
internal inline uint32_t map_get_size(const ee_map *map) { return FIELD(map, MAP_SIZE, uint32_t); }     // tiles a side
internal inline uint32_t map_get_flags(const ee_map *map) { return FIELD(map, MAP_FLAGS, uint32_t); }    // bit 0: water
// the map's diamond: a tile with x + y = edge_sum or edge_sum_2, or y - x or x - y = edge_diff, is on its edge (no
// unit enters it - the path search's tile test)
internal inline int map_get_edge_sum(const ee_map *map) { return FIELD(map, MAP_EDGE_SUM, int); }
internal inline int map_get_edge_sum_2(const ee_map *map) { return FIELD(map, MAP_EDGE_SUM_2, int); }
internal inline int map_get_edge_diff(const ee_map *map) { return FIELD(map, MAP_EDGE_DIFF, int); }
internal inline ee_grid *map_get_grid(const ee_map *map) { return FIELD(map, MAP_GRID, ee_grid *); }    // the unit grid (game/grid.c)
// the unit grid (map +0x181724, unit_grid()): cells of cs tiles, nx x ny of them; a cell's units begin .. end
internal inline ee_grid_cell *grid_get_cells(const ee_grid *grid) { return FIELD(grid, GRID_CELLS, ee_grid_cell *); }
internal inline int grid_get_cell_size(const ee_grid *grid) { return FIELD(grid, GRID_CELL_SIZE, int); }
internal inline int grid_get_nx(const ee_grid *grid) { return FIELD(grid, GRID_NX, int); }
internal inline int grid_get_ny(const ee_grid *grid) { return FIELD(grid, GRID_NY, int); }
internal inline ee_grid_cell *grid_cell_at(ee_grid_cell *cells, int i) { return (ee_grid_cell *)((uint8_t *)cells + i * 16); }   // cell (gx, gy): gx * ny + gy
internal inline ee_unit **cell_get_units_begin(const ee_grid_cell *cell) { return FIELD(cell, GRIDCELL_UNITS, ee_unit **); }
internal inline ee_unit **cell_get_units_end(const ee_grid_cell *cell) { return FIELD(cell, GRIDCELL_UNITS + 0x4, ee_unit **); }
// a resource site's ownership record (unit_get_vis(site)): +4 1 while the site is flagged (taken)
internal inline int siteown_get_flag(const ee_unit_vis *so) { return FIELD(so, SITEOWN_FLAG, int); }
// a unit's visibility: a uint16 per vision slot (player +0x45C, the player's index), its low byte != 0 seen by it
internal inline uint16_t *siteown_get_seen(const ee_unit_vis *v) { return FIELD(v, SITEOWN_SEEN, uint16_t *); }

// FUNCTIONS
// the map's area id at pt (kind 1 water, 2 land): one id per connected region
internal inline unsigned short map_area(ivec2 p, int kind) {
    return GF(FN_REGION)((void *)GA(G_MAP_AREAS), &p.x, kind);   // &p.x: the game's int pos[2]
}
// the map's area object at a tile (regions, continents)
internal inline uint8_t *area_at(int *pt) { return GF(FN_AREA_AT)((void *)GA(G_MAP_AREAS), pt); }
