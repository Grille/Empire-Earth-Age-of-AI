// THE MAP (EEMap: game_map()), its tiles, areas and unit grid: fields, functions.

// FIELDS
// A MAP TILE (the map's tile table, MAP +0x1816D4): its +0x10 is UNIT's
internal inline float tile_get_height(const void *tile) { return FIELD(tile, TILE_HEIGHT, float); }

// THE MAP ([G_MAP], game_map(): EEMap)
internal inline uint8_t **map_get_tiles(const void *map) { return FIELD(map, MAP_TILES, uint8_t **); }   // [y * size + x]
internal inline uint32_t map_get_size(const void *map) { return FIELD(map, MAP_SIZE, uint32_t); }     // tiles a side
internal inline uint32_t map_get_flags(const void *map) { return FIELD(map, MAP_FLAGS, uint32_t); }    // bit 0: water
internal inline uint8_t *map_get_grid(const void *map) { return FIELD(map, MAP_GRID, uint8_t *); }    // the unit grid (game/grid.c)
// the unit grid (map +0x181724, unit_grid()): cells of cs tiles, nx x ny of them; a cell's units begin .. end
internal inline uint8_t *grid_get_cells(const void *grid) { return FIELD(grid, GRID_CELLS, uint8_t *); }
internal inline int grid_get_cell_size(const void *grid) { return FIELD(grid, GRID_CELL_SIZE, int); }
internal inline int grid_get_nx(const void *grid) { return FIELD(grid, GRID_NX, int); }
internal inline int grid_get_ny(const void *grid) { return FIELD(grid, GRID_NY, int); }
internal inline ee_unit **cell_get_units_begin(const void *cell) { return FIELD(cell, GRIDCELL_UNITS, ee_unit **); }
internal inline ee_unit **cell_get_units_end(const void *cell) { return FIELD(cell, GRIDCELL_UNITS + 0x4, ee_unit **); }
// a resource site's ownership record (unit_get_vis(site)): +4 1 while the site is flagged (taken)
internal inline int siteown_get_flag(const void *so) { return FIELD(so, SITEOWN_FLAG, int); }

// FUNCTIONS
// the map's area id at pt (kind 1 water, 2 land): one id per connected region
internal inline unsigned short map_area(int *pt, int kind) {
    return ((unsigned short (__thiscall *)(void *, int *, int))GA(FN_REGION))((void *)GA(G_MAP_AREAS), pt, kind);
}
// the map's area object at a tile (regions, continents)
internal inline uint8_t *area_at(int *pt) { return GF(FN_AREA_AT)((void *)GA(G_MAP_AREAS), pt); }
