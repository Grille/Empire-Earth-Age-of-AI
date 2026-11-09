/// the map's unit grid, MAP +0x181724
unsafe struct ee_grid
{
    /// port_check: 353 reads in the paired grid code, AoC the same
    [offset(0x4), found(c)]
    ee_grid_cell* cells;

    /// port_check: 286 reads in the paired grid code, AoC the same
    [offset(0x8), found(c)]
    int32 cell_size;

    /// port_check: 277 reads in the paired grid code, AoC the same
    [offset(0x10), found(c)]
    int32 nx;

    /// port_check: 187 reads in the paired grid code, AoC the same
    [offset(0x14), found(c)]
    int32 ny;
}

/// a cell of the unit grid
unsafe struct ee_grid_cell
{
    /// port_check: 353 reads in the paired grid code, AoC the same
    [offset(0x4), found(c)]
    EEUnit** units_begin;
}

/// the map, [game.map] (EEMap)
unsafe struct EEMap
{
    /// its GETerrainMesh (the units' update: lea ecx, [map + 0x1C] for the visible rows, both builds)
    [offset(0x1C), found(c)]
    block mesh;

    /// the ground's corners: a row per corner y (0 .. size), 0xC each - 0x517C10 reads a tile's four, x, x + 1 of rows
    /// y and y + 1; the wall's layout (0x636CDC) its slopes
    [offset((ee)0x17B594, (aoc)0x18F498), found(h, "0x517C10 and its pair AoC 0x5222ED read it in the same instruction")]
    ee_corner_row* corner_rows;

    /// port_fields EEMap (x6): a row of VIEWROW per y, 0xC each
    [offset((ee)0x17B598, (aoc)0x18F49C), found(c)]
    void* view_rows;

    /// the map's default ground: its ee_terrain's index, what a tile without a texture of its own gets (0x51357B, the load 0x51241D)
    [offset((ee)0x181734, (aoc)0x195650), found(c)]
    int32 default_terrain;

    /// a step of the ground's height, the same on every map (0.408, 1 / sqrt 6): 0x512FB0, its pair AoC 0x51D5E8
    [offset((ee)0x1816D8, (aoc)0x1955F4), found(c)]
    float32 height_step;

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

    /// vf8 (0x5117E6): a new unit put at its place (0x5EE7D7 with its position; not for type 0x167) - the scenario's
    /// Create's last step (unit.c unit_create)
    [slot(8)]
    extern void place(EEUnit* u);

    /// the terrain paint's stroke (EESEAPaintTerrain vf1): the tiles x_lo .. x_hi, y_hi down to y_lo given the texture
    /// and kind of a terrain record (its index), then paint_colors with the colour list's type
    [address((ee)0x00512C6A, (aoc)0x0051D2A2)]
    [call_site("map_paint_terrain", (ee)0x00598D22, (aoc)0x005A8EE6)]
    extern void paint_terrain(int32 x_lo, int32 x_hi, int32 y_hi, int32 y_lo, int32 terrain, int32 color_type);

    /// the corners of tiles x_lo .. x_hi, y_hi down to y_lo coloured from a colour type's set (color_type: its
    /// record's set) - SetColorSet on each corner above the water. The editor's terrain paint (0x512C6A, its
    /// EESEAPaintTerrain's run) calls it after the textures with the colour list's type, a tile's texture change
    /// (0x512D13) with the texture's own
    [address((ee)0x00512DFB, (aoc)0x0051D433)]
    [call_site("map_paint_colors", (ee)0x00512D09, (aoc)0x0051D341)]
    extern void paint_colors(int32 x_lo, int32 x_hi, int32 y_hi, int32 y_lo, int32 color_type);

    /// the cliff pass over tiles x_lo .. x_hi - 1, y_lo .. y_hi - 1: a tile whose highest corner is one height step
    /// up at least and whose corners differ by 1.5 steps gets kind 9, its 2 x 2 block (x - 1 .. x, y - 1 .. y) textures
    /// of the map's cliff type, its corners that type's first texture's colours (map_paint_colors' own loop); a tile
    /// not 9 with 3 of its block off the map or of those textures gets kind 9 too. The end of a map's load
    /// (map_cliff_pass_load), the elevation and terrain paints' runs - around what they changed -, a new map, a change
    /// of the cliff type
    [address((ee)0x00512FB0, (aoc)0x0051D5E8)]
    [call_site("map_cliff_pass_load", (ee)0x00512869, (aoc)0x0051CEA1)]
    [call_site("map_cliff_pass_elevation", (ee)0x005982F8, (aoc)0x005A849D)]
    [call_site("map_cliff_pass_paint", (ee)0x00598D4F, (aoc)0x005A8F13)]
    extern void cliff_pass(uint32 x_lo, uint32 x_hi, int32 y_lo, int32 y_hi);

    /// a tile's texture (set: its ee_terrain.set): release 1 lets go of the tile's old one, a load's 0 (nothing held
    /// yet). Map_set_texture_load: a load's, of a tile whose saved texture is no use (none, or a cliff's: the pass lays
    /// those anew); map_set_texture_cliff: the cliff pass's
    [address((ee)0x00513941, (aoc)0x0051DFB7)]
    [call_site("map_set_texture_load", (ee)0x00512535, (aoc)0x0051CB25)]
    [call_site("map_set_texture_load", (ee)0x0051256A, (aoc)0x0051CB5A)]
    [call_site("map_set_texture_load", (ee)0x00512705, (aoc)0x0051CD3D)]
    [call_site("map_set_texture_load", (ee)0x00512751, (aoc)0x0051CD89)]
    [call_site("map_set_texture_cliff", (ee)0x0051330A, (aoc)0x0051D942)]
    extern void set_texture(int32 x, int32 y, int32 set, int8 release);

    /// a tile back to a plain ground texture (its index), its colours and kind with it. The elevation paint's
    /// clearing (0x51357B) of a tile of kind 9 that is no longer steep gives it the map's default ground
    [address((ee)0x00512D13, (aoc)0x0051D34B)]
    [call_site("map_cliff_remove", (ee)0x005136CE, (aoc)0x0051DD44)]
    extern void reset_tile(uint32 x, uint32 y, uint32 texture);

    /// the elevation paint's height change: where a corner goes below 0 or comes back the map's vf5 (0x51183B: the
    /// tiles' kinds from their textures) gives it SetWaterColorSet below, SetColorSet with its tile's texture's colour
    /// above. x_lo .. y_lo are read and written (the area it changed)
    [address((ee)0x00518CB8, (aoc)0x00523389)]
    [call_site("map_elevation_change", (ee)0x005982BC, (aoc)0x005A8461)]
    extern void elevation_change(int32* x_lo, int32* x_hi, int32* y_hi, int32* y_lo, float32 p5, float32 p6, float32 p7, float32 p8, int8 p9);
}

/// a colour type of the ground, a record of db\dbterraintype.dat (map.color_types; 0x80 bytes: a name, its id, the
/// first and last slot of the colour table, a language id ...; read at the start, 0x4D5E16)
[size(0x80), c_type("ee_color_type")]
unsafe struct ee_color_type
{
    /// its set's index in the map's mesh (RegisterColorSet's, at the map's setup 0x51371F)
    [offset(0x4), found(c)]
    uint32 set;

    /// its index in map.color_types (EESEAPaintTerrain passes it, map_paint_colors looks it up by color_type)
    [offset(0x8), found(c)]
    uint32 id;

    /// its slots of the colour table, first to last (the setup registers them as its set)
    [offset(0xC), found(c)]
    uint32 first;

    [offset(0x10), found(c)]
    uint32 last;

    /// 1: in the editor's colour list (the terrain panel's constructor takes those)
    [offset(0x1C), found(c)]
    uint8 in_editor;
}

/// a unit's visibility record, its +0x14 (a site's: its ownership flag too)
/// - it is the tile the unit stands on (ee_tile: 0x54E275 compares a tile of 0x4039AC with it, 0x502622 reads the
/// tile's height through it); kept apart until the C side's uses move to ee_tile
unsafe struct ee_unit_vis
{
    /// port_check: 152 reads in the paired zone code, AoC the same
    [offset(0x4), found(c)]
    int32 flag;

    /// explored_game 0x404A8A, AoC's 0x4042E2
    [offset(0xC), found(c)]
    uint16* seen;

    /// thiscall(site+0x14 object, player) -> bool
    [address((ee)0x00404A8A, (aoc)0x004042E2)]
    extern char8 explored(EEPlayer* pl);

    /// IsSeenBy (the AoC decompilation's): the player's sight count of the tile (ee_tile.sight) more than 1 - in its
    /// sight now; the units' collection draws the tile's doppelgangers where not
    [address((ee)0x0044E46E, (aoc)0x00449B21)]
    extern char8 seen_by(EEPlayer* pl);

    /// GetNumDoppelgangers: (ee_tile.doppelgangers_end - doppelgangers) / 4, 0 without any
    [address((ee)0x00404C70, (aoc)0x0040431C)]
    extern uint32 doppelganger_count();
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

/// a texture of the ground, a record of db\dbterrain.dat (read at the start, 0x4D54A0)
[size(0x8C), c_type("ee_terrain")]
unsafe struct ee_terrain
{
    /// its name in the data ("Cliff \\", "Grass"), the same in every language
    [offset(0x8), found(c)]
    uint8* name;

    /// its set in the map's mesh: a tile's byte 2 in map.view_rows
    [offset(0x40), found(c)]
    uint32 set;

    /// the language file's string that names it in the editor's lists (GetString)
    [offset(0x54), found(c)]
    int32 name_text;

    /// 1: in the editor's lists of textures - the terrain panel's (0x59B710) and the map panel's for its default ground (0x59198C)
    [offset(0x85), found(c)]
    uint8 in_editor;

    /// the tiles' kind it gives (ee_tile.kind): 9 a cliff's
    [offset(0x5C), found(c)]
    int32 kind;
}

/// a map tile
unsafe struct ee_tile
{
    /// vf5 (EETile, 0x516E8C): its kind set, kept apart under the water (pos: the tile's x, y; map: EEMap)
    [slot(5)]
    extern void change_kind(int32 kind, int32* pos, void* map);

    /// +4 its terrain kind (unit_type_terrain_ok)
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

    /// the buildings and trees the players remember on it under the fog (a vector: begin, end): the units' collection
    /// draws a player's where the tile isn't in its sight (ee_unit_vis.seen_by; 0x404C70 counts them)
    [offset(0x1C), found(c)]
    EEDoppelganger** doppelgangers;

    [offset(0x20), found(c)]
    EEDoppelganger** doppelgangers_end;
}

/// a row of the ground's corners (EEMap.corner_rows): {first x, length, the corners}
unsafe struct ee_corner_row
{
    /// 0x517C10: the row's first x (a corner x reads corners + (x - start) x 0xC)
    [offset(0x0), found(c)]
    uint32 start;

    /// the corners in the row (the map's diamond: few at its top and bottom)
    [offset(0x4), found(c)]
    uint32 len;

    /// 0xC a corner, its height at +0 (tiles; land about 0.4 on Highlands, its hills up to 2.8)
    [offset(0x8), found(c)]
    ee_corner* corners;
}

/// a corner of the ground (ee_corner_row.corners)
[size(0xC)]
unsafe struct ee_corner
{
    /// its height (tiles): 0x517C10, a building's z (the wall pieces stand at their tile's corner x, y)
    [offset(0x0), found(c)]
    float32 height;

    /// its colour: a slot of the mesh's colour table (map.of_mesh_color_table) - GETerrainMesh::SetColorSet picks it
    /// from a colour set, SetColorFromColorTableIndex writes it, GetVertexColor reads it, Save writes it beside the
    /// height (both builds' engines)
    [offset(0x8), found(c)]
    uint8 color;
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

/// the map's continents and areas, map.map_areas - a static object (EEAIContinentManager)
[c_type("ee_continent_mgr"), vtable((ee)0x008261FC, (aoc)0x0083A210)]
unsafe struct EEAIContinentManager
{
    /// where an idle transport lands - the continent manager's (the doc said a transport manager until 2026-10-25:
    /// two of its three callers pass map.map_areas)
    [address((ee)0x0042C49A, (aoc)0x00422658)]
    // the idle transports' call of shore_to_unload
    [call_site("shore_to_unload", (ee)0x00483E24, (aoc)0x00488490)]
    extern int32 shore_to_unload(EEUnit* transport);

    /// the area object at a tile (thiscall on the map areas map.map_areas)
    [address((ee)0x00425F16, (aoc)0x00422AD9)]
    extern uint8* area_at(int32* pt);

    /// thiscall(0x917BC8, int pos[2], 2) -> short region id
    [address((ee)0x00425F9F, (aoc)0x00422B86)]
    extern uint16 region(int32* pos, int32 kind);
}

/// the minimap (EEMiniMap)
[c_type("ee_minimap"), vtable((ee)0x0082C5A8, (aoc)0x00841D44)]
unsafe struct EEMiniMap
{
    /// thiscall(minimap, tile x, y): a unit more sees the tile (its pixel's count +1)
    [address((ee)0x0051426E, (aoc)0x0051E8BF)]
    extern void sight_add(int32 x, int32 y);

    /// thiscall(minimap, tile, x, y), ret 0xC: the tile's sight count into the minimap's (its rebuild 0x5150F2)
    [address((ee)0x00514A8A, (aoc)0x0051F0DB)]
    extern void tile(ee_tile* tile, int32 x, int32 y);

    /// thiscall(minimap): every tile again from the viewed player's sight (the viewed player changed, a new game, the
    /// scenario's Visibility-On / Off for this PC's player)
    [address((ee)0x005150F2, (aoc)0x0051F743)]
    extern void rebuild();
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

    /// ee_tile.kind of a cliff (0x512FB0's; the textures Cliff_1 .. of db\dbterrain.dat have it)
    [value(9)]
    static uint32 kind_cliff;

    /// the map's areas (regions, continents)
    [address((ee)0x00917BC8, (aoc)0x00930BF0)]
    static EEAIContinentManager map_areas;

    /// the ground's colours. The colour table is textures\ColorPalette.tga (16 x 16, a slot a pixel, the picture's top
    /// row first; read at the start into GEColors, 0x4D50BB); the colour sets are db\dbterraintype.dat's 54 records
    /// (0x80 bytes: a name, id, first and last slot, a language id ...; 0x4D5E16), a record's set the slots first to
    /// last. A new map's setup (0x51371F, from EEMap's 0x5123B3) registers the sets with its mesh (RegisterColorSet:
    /// the set's id into the record's +4) and then hands it the table: SetColorTable, its only call in both builds -
    /// thiscall on the mesh, a vector of GEColor (3 floats, 0xC each; 256 at most, the rest white)
    [import_slot("Low-Level Engine.dll", "?SetColorTable@GETerrainMesh@@QAEXAAV?$vector@VGEColor@@V?$allocator@VGEColor@@@std@@@std@@@Z")]
    [address((ee)0x00823914, (aoc)0x008378F0)]
    static block iat_color_table;

    /// GETerrainMesh::SetColorSet (thiscall on the mesh: a corner's x, y, a set from RegisterColorSet): the corner's
    /// slot from the set by the noise of its place - what map_paint_colors, the cliff pass and the map's vf5 colour with
    [import_slot("Low-Level Engine.dll", "?SetColorSet@GETerrainMesh@@QAEXKKK@Z")]
    [address((ee)0x008238FC, (aoc)0x008378D8)]
    static block iat_set_color_set;

    /// GETerrainMesh::SetWaterColorSet (x, y): a corner under the water, from the water's set
    [import_slot("Low-Level Engine.dll", "?SetWaterColorSet@GETerrainMesh@@QAEXKK@Z")]
    [address((ee)0x00823900, (aoc)0x008378DC)]
    static block iat_set_water_color_set;

    /// GETerrainMesh::SetZ (thiscall on the mesh: a corner's x, y, its height): what changes the ground's heights, a new
    /// building's flattening among them
    [import_slot("Low-Level Engine.dll", "?SetZ@GETerrainMesh@@QAEXKKM@Z")]
    [address((ee)0x0082393C, (aoc)0x00837920)]
    static block iat_set_z;

    /// GETerrainMesh::ComputeVertexNormal (x, y): a corner's normal again after its heights changed
    [import_slot("Low-Level Engine.dll", "?ComputeVertexNormal@GETerrainMesh@@QAEXKK@Z")]
    [address((ee)0x00823958, (aoc)0x0083793C)]
    static block iat_compute_normal;

    /// GETerrainMesh::ComputeShorelines (x, y): a tile's water bits again (GetWaterInfo) after the heights changed
    [import_slot("Low-Level Engine.dll", "?ComputeShorelines@GETerrainMesh@@QAEXKK@Z")]
    [address((ee)0x00823954, (aoc)0x00837938)]
    static block iat_compute_shorelines;

    /// GETerrainMesh::RegisterColorSet (thiscall on the mesh: a vector of uint32 slots, its index out): a set more,
    /// the slots copied - the map's setup's for every colour type
    [import_slot("Low-Level Engine.dll", "?RegisterColorSet@GETerrainMesh@@QAEJAAV?$vector@KV?$allocator@K@std@@@std@@AAK@Z")]
    [address((ee)0x0082391C, (aoc)0x008378F8)]
    static block iat_register_color_set;

    /// the colour types (ee_color_type), db\dbterraintype.dat's records in its order: a vector of pointers (MSVC: the
    /// first at +4, the end at +8); map_functions.color_type reads it
    [address((ee)0x008D1FF8, (aoc)0x008E86F8)]
    static block color_types;

    /// GETerrainMesh: its colour sets - a vector (MSVC: the first element at +4, the end at +8) of sets, 0x10 each, a
    /// set a vector of uint32 slots (first at +4, end at +8). SetColorSet gives a corner the slot at (n - 1) x a noise
    /// value of the corner's x & 63, y & 63 (+0x17B574's table; the 2017 update's engine 0x1003C3C2, AoC's 0x1002AE9B):
    /// a set's slots needn't follow each other, nor the same slot at the same place come from the same set
    [value((ee)0x50, (aoc)0x6C)]
    static uint32 of_mesh_color_sets;

    /// GETerrainMesh: its colour table, 256 GEColors (3 floats; SetColorTable copies them, GetVertexColor reads its corner's)
    [value((ee)0x8C, (aoc)0xC8)]
    static uint32 of_mesh_color_table;

    /// byte: the visible ground is up to date - the ground's update (0x503A7F, AoC 0x50E8D1) runs
    /// UpdateVisibleTerrain, which takes the corners' colours from the table, only while it is 0 and sets it after the
    /// draw; a camera move, an edit of the ground clear it (10 writers). Cleared, a changed colour shows at the next frame
    [address((ee)0x009128CB, (aoc)0x00929067)]
    static block terrain_current;
}

/// the game's functions map.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class map_functions
{
    /// a colour type by its id (map.color_types; NULL past the end)
    [address((ee)0x00513BE7, (aoc)0x0051E22B), cdecl]
    static extern ee_color_type* color_type(uint32 id);

    /// a texture of the ground by its index, a tile's byte 2 in map.view_rows (NULL: none)
    [address((ee)0x004D5D21, (aoc)0x004DBC39)]
    [cdecl]
    static extern ee_terrain* terrain_at(uint32 index);
}
