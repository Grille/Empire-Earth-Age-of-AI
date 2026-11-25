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

/// a random map being made (EERandomMap, EE 0x410 bytes): random_map_create makes a new one for each try. Its script's
/// values are the generator plug-in's to fill (EERMPIMSetRandomMapValues, Default.dll's parser: a keyword's number
/// into a field, the same keyword the same field in every script) - the plug-in is handed the map + 4, so its field
/// X is the map's X + 4 here. AoC's Default.dll has fields more before them: its offsets 0x10 - 0x14 later. Each
/// field's keyword below; its type is the keyword's (int, Between picked once)
[c_type("ee_random_map")]
unsafe struct EERandomMap
{
    /// MaxIterationsClumpGrowth: the land's growth runs in up to its square of rounds (Default.dll's land growth
    /// 0x1000149A), each patch's own limit it x its clumps (0x10007272)
    [offset((ee)0x2FC, (aoc)0x310), found(c)]
    int32 clump_growth;

    /// WaterBorder: tiles of water along the map's edge (with start_to_edge the start positions' distance from it,
    /// Default.dll's start positions 0x10003AE0)
    [offset((ee)0x308, (aoc)0x31C), found(c)]
    int32 water_border;

    /// OuterPushFromMapCenter: tiles the start positions of the outer ring keep from the middle (0x10003AE0)
    [offset((ee)0x314, (aoc)0x328), found(c)]
    int32 outer_push;

    /// InnerPushFromMapCenter: the same for the inner ring
    [offset((ee)0x318, (aoc)0x32C), found(c)]
    int32 inner_push;

    /// MinimumStartPositionToMapEdge: tiles more between a start and the water border
    [offset((ee)0x31C, (aoc)0x330), found(c)]
    int32 start_to_edge;

    /// ResourcePerNeutralLand by resource (Stone, Gold, Tree, Berry, Animal, Fish, Iron - the script's "Steel" too):
    /// how many of it go onto the neutral lands in all, laid one by one round their places in turn (Default.dll
    /// 0x10015299) - not so many each
    [offset((ee)0x358, (aoc)0x36C), found(c)]
    fixed int32 neutral_resources[7];

    /// the square the plug-in works in, a side (the map's edge_diff x 2: Gigantic 440): set by the script read
    /// (0x66DC35) before the plug-in reads the script
    [offset((ee)0x380, (aoc)0x394), found(c)]
    int32 side;

    /// NumInnerNeutrals: the patches of neutral land near the middle - InnerNeutralPercentLand of the land split
    /// among them (0x1000149A)
    [offset((ee)0x398, (aoc)0x3AC), found(c)]
    int32 inner_neutrals;

    /// NumOuterNeutrals: the same for the outer ones (OuterNeutralPercentLand); each its ResourcePerNeutralLand
    [offset((ee)0x39C, (aoc)0x3B0), found(c)]
    int32 outer_neutrals;

    /// RiverWidth: the water between the players' lands (the island scripts'; 0 none)
    [offset((ee)0x3B8, (aoc)0x3CC), found(c)]
    int32 river_width;

    /// the map made from the script read: the plug-in's steps (player and neutral positions, land and water, flat
    /// land, heights, shallows, the height filter) and the exe's own between them, then the textures, cliffs,
    /// resources, trees and start units; 0 made. Its one call: random_map_create's try, right after the script read
    /// (0x66F4A6: the plug-in's SetRandomMapValues and the exe's reads of the script's values)
    [address((ee)0x0066B434, (aoc)0x0068449E)]
    [call_site("random_map_generate", (ee)0x0066C4B6, (aoc)0x00685587)]
    extern int32 generate();
}

/// the random map's data of the game
static unsafe class random_map
{
    /// the map types (EERandomMap::sRandomMapGenerators): a vector of ee_map_type (MSVC: the first at +4, the end at
    /// +8), filled once at the game's start and sorted by name; the setup screens' lists, the editor's and the lobby's
    /// read it by index (0x66D5BC its count, 0x66D650 an element, 0x66D5D7 an element's title)
    [address((ee)0x00943520, (aoc)0x00965570)]
    static block types;

    /// the imm32 of cmp eax, 0xDC / ja in the script's reading (0x66DC35): the map's edge_diff gives the script its
    /// size (EERandomMap +0x254, 0 Tiny .. 5 Gigantic: IsTinyMap .. IsGiganticMap), a wider map none (an assert, the
    /// field left as it was)
    [address((ee)0x0066DDA5, (aoc)0x00686EFD)]
    static code site_script_gigantic;
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
