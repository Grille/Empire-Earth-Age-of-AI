/// a random map type the game setup offers, an element of random_map.types (0x38 bytes; its constructor 0x66DBF7 / AoC
/// 0x686D4F makes the three strings). The game makes one per Data\Random Map Scripts\*.rmv at its start
/// (random_map_types_load) and reads only two lines of the file for it: UseMapGenerator and StringTableID
[size(0x38), c_type("ee_map_type")]
unsafe struct ee_map_type
{
    /// the script's name without ".rmv" ("Continental"): what the setup's map_name holds - a start finds the type by
    /// it (random_map_create compares the two without case), the lists show the types by index
    [offset(0x0), found(c)]
    wstring name;

    /// what the lists show: the language file's string of the script's StringTableID, without one the name
    [offset(0x14), found(c)]
    wstring title;

    /// the script's UseMapGenerator ("Default"): the name of the map generator plug-in (a DLL of the game folder, type
    /// 'rmap') random_map_create looks up
    [offset(0x28), found(c)]
    ustring generator;
}

/// the random map's data of the game
static unsafe class random_map
{
    /// the map types (EERandomMap::sRandomMapGenerators): a vector of ee_map_type (MSVC: the first at +4, the end at
    /// +8), filled once at the game's start and sorted by name; the setup screens' lists, the editor's and the lobby's
    /// read it by index (0x66D5BC its count, 0x66D650 an element, 0x66D5D7 an element's title)
    [address((ee)0x00943520, (aoc)0x00965570)]
    static block types;
}

[c_prefix("")]
static unsafe class random_map_functions
{
    /// a random map made into a new, empty map (EERandomMap::CreateRandomMap's caller, "Playable random map created
    /// after %d attempts"): the type found by name, its generator plug-in by the type's generator, then up to 11
    /// tries of a new EERandomMap (0x410 bytes: the script read, the plug-in's steps and the exe's own between them -
    /// 0x66B434 - heights, land and water, textures, cliffs, resources, forests, the players' start units) until its
    /// start positions stand on land. players: the game's players that play (a vector, the world first), human: this
    /// PC's player's index in it (0: none - start is then the map's middle), name: the type's (a string, its text at
    /// +4), start: where this PC's view begins (x, y, the ground's height). 0: made. Its two calls: the game start of a
    /// random map (EEGameState::start, setup.mode 0) and the scenario editor's random map (EESEACreateRandomMap.run)
    [address((ee)0x0066D402, (aoc)0x0068655A), cdecl]
    [call_site("random_map_create", (ee)0x005268C6, (aoc)0x00531639)]
    [call_site("random_map_create", (ee)0x005966EF, (aoc)0x005A69AB)]
    static extern int32 random_map_create(ee_setup* setup, void* players, int32 human, void* name, uint32 seed, EEMap* map, float32* start);

    /// the map types read (EERandomMap::Initialize): the generator plug-ins listed, then a type per *.rmv of
    /// Data\Random Map Scripts\ pushed into random_map.types and the vector sorted. Once, at the game's start
    [address((ee)0x0066D6AC, (aoc)0x00686804), cdecl]
    [call_site("random_map_types_load", (ee)0x005371E5, (aoc)0x00542389)]
    static extern int32 random_map_types_load(void* path);

    /// a type more at the vector's end, copied from e (thiscall on random_map.types: the load's own push)
    [address((ee)0x0066FA49, (aoc)0x00688DE1), thiscall]
    static extern void random_map_types_add(void* types, ee_map_type* e);
}
