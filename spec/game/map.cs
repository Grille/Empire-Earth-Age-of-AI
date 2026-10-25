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

/// the map, [G_MAP] (EEMap)
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

    /// FN_EXPLORED 0x404A8A, AoC's 0x4042E2
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
    /// its sight count per player slot, a uint16 each (FN_TILE_SIGHT_ADD 0x519F4F, AoC 0x5247ED: the same code; =
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
