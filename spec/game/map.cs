/// the map's unit grid, MAP +0x181724
unsafe struct ee_grid
{
    [offset(0x4), found(s)]
    ee_grid_cell* cells;

    [offset(0x8), found(s)]
    int32 cell_size;

    [offset(0x10), found(s)]
    int32 nx;

    [offset(0x14), found(s)]
    int32 ny;
}

/// a cell of the unit grid
[c_prefix("cell")]
unsafe struct ee_grid_cell
{
    [offset(0x4), found(s)]
    EEUnit** units_begin;
}

/// the map, [game.map] (EEMap)
unsafe struct EEMap
{
    /// port_fields EEMap (x6): a row of VIEWROW per y, 0xC each
    [offset((ee)0x17B598, (aoc)0x18F49C), found(c)]
    void* view_rows;

    /// [y * size + x]
    [offset((ee)0x1816D4, (aoc)0x1955F0), found(c)]
    ee_tile** tiles;

    /// the path search's tile test (0x41D36A): x + y on it is off the map's diamond
    [offset((ee)0x1816EC, (aoc)0x195608), found(c)]
    int32 edge_sum;

    /// the same, the other edge
    [offset((ee)0x1816F0, (aoc)0x19560C), found(c)]
    int32 edge_sum_2;

    /// the same for y - x and x - y
    [offset((ee)0x1816F4, (aoc)0x195610), found(c)]
    int32 edge_diff;

    /// tiles a side
    [offset((ee)0x1816FC, (aoc)0x195618), found(c)]
    uint32 size;

    /// the unit grid (game/grid.c)
    [offset((ee)0x181724, (aoc)0x195640), found(c)]
    ee_grid* grid;

    /// bit 0: water
    [offset((ee)0x181730, (aoc)0x19564C), found(c)]
    uint32 flags;
}

/// a unit's visibility record, its +0x14 (a site's: its ownership flag too)
[c_prefix("siteown")]
unsafe struct ee_unit_vis
{
    [offset(0x4), found(s)]
    int32 flag;

    /// explored_game 0x404A8A, AoC's 0x4042E2
    [offset(0xC), found(c)]
    uint16* seen;
}

/// a node of a tile's object list (TILE_OBJECTS)
[c_type("void")]
unsafe struct ee_tile_node
{
    /// the path search's tile test (0x41D36A / AoC 0x41990C)
    [offset(0x4), found(c)]
    EEUnit* @object;

    /// the same
    [offset(0x8), found(c)]
    void* next;
}

/// a map tile
unsafe struct ee_tile
{
    /// +4 its terrain kind (type_terrain_ok)
    /// the terrain kind (an index into TD_TERRAIN_OK; 0 land, 1 water - a guess)
    [offset(0x4), found(c)]
    int32 kind;

    /// the objects on it, a list of {?, object +4, next +8}
    [offset(0x8), found(c)]
    void* objects;

    /// [player index]
    /// its sight count per player slot, a uint16 each (map.fn_tile_sight_add 0x519F4F, AoC 0x5247ED: the same code; =
    /// SITEOWN_SEEN, a unit's +0x14 is its tile)
    [offset(0xC), found(c)]
    uint16* sight;

    [offset(0x10), found(c)]
    float32 height;

    /// EEPMNaval::vf21's step test (0x550300 / AoC the same read): a signed byte against UNIT_SEA_MODE
    [offset(0x14), found(c)]
    int8 sea_class;
}

/// a row of the map's per-tile view table (MAP_VIEW_ROWS): {first x, length, data}
[c_type("void")]
unsafe struct ee_view_row
{
    /// 0x55BC4E (AoC: the same code): the row's first x
    [offset(0x0), found(c)]
    uint32 start;

    /// the path search's and the camera's bounds tests (x - start < len)
    [offset(0x4), found(c)]
    uint32 len;

    /// 0x55BC4E: 4 bytes an x, its uint16 the viewed player's sight count
    [offset(0x8), found(c)]
    uint8* data;
}

/// map's data of the game, places in its code and numbers
static unsafe class map
{
    /// thiscall(tile, player), ret 4: a unit more sees it
    [address((ee)0x00519F4F, (aoc)0x005247ED)]
    static code fn_tile_sight_add;

    /// thiscall(tile, player), ret 4: a unit less sees it
    [address((ee)0x00519E26, (aoc)0x005246C4)]
    static code fn_tile_sight_sub;

    /// the map's areas (regions, continents)
    [address((ee)0x00917BC8, (aoc)0x00930BF0)]
    static block map_areas;
}

/// the game's functions map.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class map_functions
{
    /// the area object at a tile (thiscall on the map areas map.map_areas)
    [address((ee)0x00425F16, (aoc)0x00422AD9), thiscall]
    static extern uint8* area_at_game(void* p1, int32* p2);

    /// thiscall(minimap, tile x, y): a unit more sees the tile (its pixel's count +1)
    [address((ee)0x0051426E, (aoc)0x0051E8BF), thiscall]
    static extern void minimap_sight_add(void* mm, int32 x, int32 y);

    /// thiscall(minimap, tile, x, y), ret 0xC: the tile's sight count into the minimap's (its rebuild 0x5150F2)
    [address((ee)0x00514A8A, (aoc)0x0051F0DB), thiscall]
    static extern void minimap_tile(void* mm, ee_tile* tile, int32 x, int32 y);

    /// thiscall(0x917BC8, int pos[2], 2) -> short region id
    [address((ee)0x00425F9F, (aoc)0x00422B86), thiscall]
    static extern uint16 region_game(void* p1, int32* p2, int32 p3);

    /// thiscall(player): every tile's sight count (its low byte) into MAP_VIEW_ROWS - a load, the viewed player
    /// changed
    [address((ee)0x0055BC4E, (aoc)0x0056AEC0), thiscall]
    static extern void view_sight_copy(EEPlayer* pl);
}
