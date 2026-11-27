// THE MOD'S OWN MAP TYPES (the player 2026-11-24: "i would circumvent the games own generator fully, put our entries
// directly in the list of available map types and if selected we take over fully"). The game's random maps are a
// script per type (Data\Random Map Scripts\*.rmv: some 110 numbers and placement lists) run by a generator plug-in
// (Default.dll: land grown round each player's start and round neutral points, a fractal height map, resources in
// rings) - a script can't say where land goes, so two continents shared by the players, climates by region or rivers
// are beyond it (his wish, the same day). The mod's types sit beside the game's in its list of types
// (random_map.types, what every list of the setup screens, the lobby and the editor shows) and a map of one of them is
// made here whole: the game's generator, its plug-in and the scripts are not run for it (hook_random_map_create: the
// game start's and the editor's call of the game's own). The game's types play as before.
// What the game needs of a random map (read from its own, random_map_create): the ground of the new map it hands over
// (heights, textures, cliffs), the world's objects on it (mines, trees, animals: the world player's), each playing
// player's start units (made, built and paid for as its own are), where this PC's view begins - and 0 for "made".
// Nothing else of the start asks for the generator's own object.
// IN MULTIPLAYER every PC makes the map itself from the setup, the seed and the host's options (g_mg_host): the same
// numbers on each. So nothing here
// is drawn from the clock or the game's random numbers, and no sine, cosine or arc tangent is used (the C library's
// may differ between PCs in a last bit; +, -, x, / and the square root don't) - directions are vectors, turned by
// adding a share of their normal.
// ONE ENTRY, "Age of AI" (the player 2026-11-25: one entry with its options in a dialog, not a line per kind of map):
// what it makes is [random_map]'s (map_layout - two continents, west and east or north and south - and how much of what: g->opt below), the
// scenario editor asks for them when it makes one (mapgen_dialog.c). Its numbers in the code until the steps are
// settled (then a description in .eeai).
#define MG_NAME "Age of AI"
internal int g_mapgen_added;
// the seed of the options' preview to play (the setup screen's dialog, "play this map"): the next single-player random
// map of the mod's type is made from it instead of the game's, once
internal struct { int on; uint32_t seed; } g_mg_keep;
// THE HOST'S OPTIONS IN MULTIPLAYER (step 4 of the plan): every PC makes the map itself, so every PC must make it from
// the same options - the host's. Its Start sends them to every PC as the mod's own message (hook_lobby_check: before
// the game's start message, through the same server, so they are carried out first - its own PC's too) with the
// preview's seed when "play this map" is ticked; each PC's map of that game is made from what came (mg_options), once
#define MG_OPTS (SETTING_map_mines - SETTING_map_layout + 1)
internal struct { int got; uint8_t v[MG_OPTS]; int keep; uint32_t seed; } g_mg_host;

internal int mapgen_is_ours(const char *name) { return name && !lstrcmpiA(name, MG_NAME); }

// the mod's types pushed into the game's list, once, when the game has made its own
internal void mapgen_types_add(void) {
    if (g_mapgen_added || !map_types_count()) return;
    g_mapgen_added = 1;
    const char *title = word("map_age_of_ai");   // the list's line, in the game's language (English before the words are read)
    map_types_add(MG_NAME, lstrcmpA(title, "map_age_of_ai") ? title : MG_NAME, "Default");
    if (g_log) log_locked("# mapgen: \"%s\" added to the game's %u map types\n", MG_NAME, map_types_count() - 1);
}
internal int __cdecl hook_random_map_types_load(void *path) {
    int r = random_map_types_load(path);
    mapgen_types_add();
    return r;
}

// THE BIOMES (the player 2026-11-24: "should we maybe adapt a system like minecraft? both a temperature and moisture
// layer over the map which decide what biome can generate. hot and dry is desert, moist and hot tropical"). Two layers
// over the map: the temperature - cold in the far north, hot in the near south, pushed about by noise, colder on
// hills - and the moisture - a noise of its own, wetter near water (the sea's and the rivers': 7 tiles in). Each in
// three bands, a tile's biome the pair: what its ground looks like, which trees grow how densely, which animals live
// there, how likely a mine, a forage patch or a herd is. So the biomes are not fair to each other, on purpose (he:
// "competitive players are probably happy with the basegame maps. this might lean more in a 4x flavour of the game,
// casual play with ai or online trying to build up their nations"): a desert has gold and stone and next to no wood,
// the tropics food and wood and few mines. A start still gets what a game can't begin without - its mines, two forage
// patches, a herd and one forest of timber (in a biome without timber trees a grove of the nearest kind that has).
#define MG_BIOMES 9
#define MG_MAPLE "tree_01_maple", "tree_02_maple", "tree_04_maple", "tree_30_maple"
#define MG_MIXED "tree_07_spruce", "tree_09_spruce", "tree_10_spruce", "tree_11_spruce", "tree_01_maple", "tree_04_maple"
#define MG_CONIFERS "tree_12_spruce", "tree_07_spruce", "tree_11_spruce", "tree_10_spruce"
#define MG_FIRS "tree_31_winter_spruce_1", "tree_32_winter_spruce_2", "tree_33_winter_spruce_3", "tree_34_winter_spruce_4", "tree_35_winter_spruce_5", "tree_36_winter_spruce_6"
#define MG_PALMS "tree_21_palm", "tree_22_palm", "tree_23_palm", "tree_26_palm", "tree_28_palm", "tree_29_palm"
#define MG_DEAD "tree_05_spruce_dead", "tree_06_spruce_dead", "tree_14_spruce_dead"
enum { MG_RICH_GOLD, MG_RICH_IRON, MG_RICH_STONE, MG_RICH_FORAGE, MG_RICH_HERD };
internal const struct mg_biome {
    const char *name;
    const char *land[5];         // its ground textures (db\dbterrain.dat's names) and their shares of 100, the first the plain one
    uint8_t share[5];
    const char *beach, *res, *wood;   // under water up to a step deep, under a mine, under trees
    const char *trees[6];        // what grows there
    const char *grove[6];        // a start's own forest where those are no timber
    int16_t forest_tiles;        // land tiles to a forest
    uint8_t size_lo, size_hi;    // a forest's tiles
    const char *animals[4];
    uint8_t rich[5];             // of 100 mines (gold, iron, stone), forage patches and herds drawn for a place there, how many are laid
    uint8_t by_warmth;           // a forest is spruce, mixed or maple by the warmth where it grows (mg_forest), not of trees[]
    int8_t lean;                 // and that many hundredths of the way warmer: more maple
} k_mg_biome[MG_BIOMES] = {
    // cold: dry, middling, wet
    { "ice desert", { "Snow", "Ice", "Snow - Dirt Patches", "Snow", "Snow" }, { 45, 25, 20, 5, 5 }, "Ice", "Snow - Dirt Patches", "Snow - Tufts",
      { MG_DEAD }, { MG_FIRS }, 1500, 6, 14, { "res_walrus", "res_goat" }, { 60, 100, 100, 10, 30 }, 0, 0 },
    { "tundra", { "Snow", "Snow - Grass Patches", "Snow - Tufts", "Snow", "Snow" }, { 40, 30, 20, 5, 5 }, "Ice", "Snow - Dirt Patches", "Snow - Tufts",
      { MG_FIRS }, { NULL }, 650, 18, 36, { "res_deer", "res_goat", "res_walrus" }, { 60, 100, 80, 30, 60 }, 0, 0 },
    { "taiga", { "Snow - Tufts", "Snow", "Snow - Grass Patches", "Snow", "Snow" }, { 40, 30, 20, 5, 5 }, "Ice", "Snow - Dirt Patches", "Snow - Tufts",
      { MG_FIRS }, { NULL }, 200, 36, 60, { "res_deer", "res_goat" }, { 50, 90, 60, 50, 80 }, 0, 0 },
    // mild
    // (no Dry Leaves on its open land - the player 2026-11-26: they look a forest's floor, not dry ground; green
    // enough not to be taken for the hot desert's sand: the sparse grass with plain grass and tufts, as the game's own
    // temperate maps have them)
    { "steppe", { "Grass - Sparse", "Grass", "Grass - Patches", "Grass - Tufts", "Grass - Sparse" }, { 40, 25, 15, 15, 5 }, "Beach", "Sand - Medium", "Dry Leaves",
      { MG_DEAD }, { MG_MAPLE }, 1200, 8, 20, { "res_horse", "res_goat", "res_ostrich" }, { 80, 80, 80, 50, 80 }, 0, 0 },
    { "grassland", { "Grass", "Grass - Tufts", "Grass - Tall", "Grass - Patches", "Grass" }, { 60, 10, 6, 8, 16 }, "Beach", "Sand - Medium", "Grass - Sparse",
      { MG_MAPLE }, { NULL }, 420, 26, 50, { "res_deer", "res_horse", "res_goat" }, { 70, 70, 70, 100, 100 }, 1, 15 },
    { "woodland", { "Grass", "Grass - Tall", "Grass - Tufts", "Grass - Overgrown", "Grass" }, { 40, 22, 15, 18, 5 }, "Beach", "Sand - Medium", "Grass - Sparse",
      { MG_MIXED }, { NULL }, 170, 40, 60, { "res_deer", "res_goat" }, { 50, 60, 50, 100, 90 }, 1, 0 },
    // hot
    { "desert", { "Sand - Fine", "Sand - Medium", "Desert", "Sand - Cracked", "Dark Cracked Mud" }, { 35, 25, 20, 12, 8 }, "Desert", "Pebbles - Small", "Sand - Medium",
      { MG_PALMS }, { NULL }, 1300, 5, 12, { "res_ostrich", "res_goat" }, { 100, 60, 100, 15, 20 }, 0, 0 },
    { "savanna", { "Grass - Patches", "Grass - Sparse", "Sand - Medium", "Grass", "Grass - Patches" }, { 40, 30, 15, 10, 5 }, "Beach", "Pebbles - Small", "Grass - Sparse",
      { MG_PALMS }, { NULL }, 800, 10, 24, { "res_giraffe", "res_elephant", "res_ostrich", "res_horse" }, { 90, 60, 70, 60, 100 }, 0, 0 },
    { "tropics", { "Grass - Overgrown", "Grass - Tall", "Grass", "Grass - Tufts", "Grass - Overgrown" }, { 40, 25, 20, 10, 5 }, "Beach", "Sand - Medium", "Grass - Tall",
      { MG_PALMS }, { NULL }, 160, 40, 60, { "res_elephant", "res_hippopotamus", "res_giraffe" }, { 50, 40, 40, 100, 90 }, 0, 0 },
};

// THE GENERATOR'S STATE. Tiles x, y (0 .. n - 1), corners 0 .. n: the map is the diamond of them the game made (a tile
// there has its ee_tile). Across the diamond run u = x - y and v = x + y - n, each from -n / 2 to n / 2: the screen's
// two ways - v from left to right, u from the near edge to the far one; the far end is the north. The continents
// (map_layout: mg_layout) lie apart west and east, along v, and stretch along u - or north and south, the two swapped: what
// is the land's is said along and across them (mg_along, mg_across), the weather's by u (mg_temp: the north cold).
enum { MG_GOLD, MG_IRON, MG_STONE, MG_FORAGE, MG_FISH, MG_TREE, MG_ANIMAL, MG_AMBIENT, MG_KINDS };
#define MG_PLAYERS 16
#define MG_SPOTS 1024
#define MG_FORDS 96
#define MG_BLOBS 16
struct mg {
    ee_map *map;              // the game's new map; NULL: a preview (mapgen_preview) - the generator's own arrays alone
    ee_player *world;
    int n, m;                 // tiles a side, corners a side
    uint32_t rng, seed;
    float *f;                 // [corner] how far inside the land (tiles; below 0: that far out in the water)
    float *h;                 // [corner] its height in height steps (below 0: under the water)
    float *mt;                // [corner] the mountains' height there (steps; 0: none)
    float *foot;              // [corner] 0 .. 1 how near a range's foot is (its foothills; 0: 12 tiles and more beyond it)
    float *plat;              // [corner] a plateau's height there (steps; 0: none)
    float *fjord;             // [corner] 0 .. 1 a fjord's shore: rocky whatever the coast's noise says
    uint8_t *range;           // [corner] which range's that is, from 1, or 16 + which plateau's (0: none)
    int8_t range_cliffs[32];  // a range's and a plateau's cliff type (CLIFFS_*): one for the whole of it
    uint8_t *used;            // [tile] taken: a start's yard, a river's bank, a mine, a tree; 2: a ford's gravel
    uint8_t *clim;            // [tile] its biome (k_mg_biome)
    uint8_t *marsh;           // [tile] water: 2 the sea's, 1 a river's or a pond's all under water and not frozen (marsh), 0 other inland water
    uint8_t *kind;            // [tile] a preview's tile kinds (MG_K_*: the game's rules, mg_kinds) - the game's map has its own
    int16_t *tex;             // [tile] its ground texture
    uint8_t *obj;             // [tile] the object made there: its kind (MG_GOLD ...) + 1, 0 none (what a start has near it; a preview's picture)
    uint8_t *rock;            // [tile] a preview's cliff texture there: its cliff type + 1, 0 none (mg_cliff_run)
    float *z;                 // [corner] the heights handed to the game's map (tiles)
    float half;               // n / 2: u and v reach that far
    int np;                   // the players that play
    int sx[MG_PLAYERS], sy[MG_PLAYERS];   // their starts (a tile corner: the Capitol's middle)
    int placed;               // the starts placed: 0 until the land is made (mg_starts), then np
    int start_how[5];         // the starts placed at each of mg_starts' levels (0: where it looked first ... 4: forced)
    int topped[6];            // what the starts' own gave them beyond the map's near them: forage, gold, iron, stone, forests, herds
    struct { int x, y; } spot[MG_SPOTS];  // the mines', forests' and herds' places: kept apart from each other
    int nspot;
    int tex_land[MG_BIOMES][5], tex_beach[MG_BIOMES], tex_res[MG_BIOMES], tex_wood[MG_BIOMES], tex_shore[MG_BIOMES], tex_deep, tex_marsh, tex_pebbles;
    int tree[MG_BIOMES][6], ntree[MG_BIOMES], grove[MG_BIOMES][6], ngrove[MG_BIOMES], animal[MG_BIOMES][4], nanimal[MG_BIOMES];   // the biomes' types the data has
    int wood[3][6], nwood[3], wood_made[3];   // a by_warmth biome's forests: spruce, mixed, maple; how many of each
    struct { int x, y; } spring[16];      // the rivers' springs: apart from each other
    struct { int x, y, r; } lake[12];     // the lakes: middle and radius
    struct { int x, y, r; } plateau[8];   // the plateaus
    int plateaus;
    int islands, bays, capes, fjords;
    struct { int x, y; float r; } isle[16];   // the islands: middle and radius
    int afoot;                // 1: a walker from a start reaches the other continent's (0: the sea parts them)
    int cut;                  // starts a walker from its continent's first start does not reach
    int walked, walkable;     // land tiles (no cliffs) reached on foot from the starts, and all there are
    int fords_ok, mended;     // the fords that are laid and to wade; those of them laid for a start cut off
    struct { int x, y, r; } oasis[8], swamp[4];   // the deserts' ponds, the hot wet south's swamps
    int oases, swamps;
    struct { float x, y; } ford[MG_FORDS];   // the rivers' fords, laid once all water is dug (x under 0: not laid)
    int lakes, fords;
    int made[MG_KINDS], rivers, land, water, cliffs, forests, ranges, mountain, mountain_mines, types_missing, biome_tiles[MG_BIOMES];
    int clumps, clump_mines;  // mg_clump's: how many, and the mines in them
    // THE CONTINENTS (map_layout: mg_layout): each an ellipse in the layout's frame (its shape made within it and its
    // land nine tenths of it: mg_shapes), its middle across and its half
    // widths across and along in halves of the map (times the land option); the first `homes` of them the players'
    // (seat s on continent s % homes), the others nobody's. The sea's channel between two runs along at `chan` across
    // (tiles; chan_on 0: one continent, none)
    struct { float at, wide, len; } cont[2];
    struct mg_blob { float u, v, du, dv, a, b; int bite; } blob[2][MG_BLOBS];   // a continent's shape (mg_shapes)
    int blobs[2];
    float lift[2];            // added to a continent's shape: its land nine tenths of its ellipse's (mg_shapes)
    int nc, homes, chan_on;
    float chan;
    int turned;               // the layout's rotation made: quarters clockwise (map_rotation's, or the seed's for random)
    int ring;                 // the players round their continent, not along it (mg_layout)
    int new_mines, new_forests;   // the New World's own beyond the land's (old_new_world: mg_objects)
    struct mg_opt {           // [random_map] as numbers (mg_options); "normal" a factor of exactly 1, a shift of 0: the map as before
        int lay;              // the layout: MG_L_*
        int rot;              // its rotation: quarters clockwise 0 .. 3, -1 random (from the seed: g->turned)
        int ns;               // the layout's frame: 0 the continents west and east (along them u), 1 north and south (along them v)
        float land;           // the continents' size
        float temp, wet;      // added to the temperature and the moisture (their bands: -0.3 / 0.3, 0.4 / 0.6)
        int zones;            // where it is cold and where hot: MG_ZONES_*
        float biome;          // a factor on the temperature's and the moisture's noise frequency: under 1 larger patches
        float mountains, rivers, lakes, islands, forests, mines;   // factors on how many
        int mp;               // a multiplayer game: the host's options (none came: the defaults)
        int from;             // MG_FROM_*: whose they are
        int v[MG_OPTS];       // the choices, [random_map]'s order
    } opt;
};
internal uint32_t mg_next(struct mg *g) {
    uint32_t x = g->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return g->rng = x;
}
internal float mg_rnd(struct mg *g) { return (float)(mg_next(g) >> 8) * (1.0f / 16777216.0f); }          // 0 .. 1
internal int mg_int(struct mg *g, int lo, int hi) { return lo + (int)(mg_next(g) % (uint32_t)(hi - lo + 1)); }   // lo .. hi
// how many of a thing: the map's count of it times an option's factor (1: the count itself, not through a float)
internal int mg_amount(int base, float f) { return f == 1.0f ? base : (int)((float)base * f + 0.5f); }
// THE OPTIONS ([random_map], dll_cs/settings/random_map.cs) as the steps use them: this PC's own, in a multiplayer
// game the host's (g_mg_host: its Start sent them) - none came (a host whose mod has no step 4, the type chosen
// otherwise than by the lobby's list): the defaults, as on every other PC then.
enum { MG_FROM_OWN, MG_FROM_HOST, MG_FROM_DEFAULTS };
enum { MG_ZONES_NORTH_SOUTH, MG_ZONES_POLES, MG_ZONES_MIXED };   // [random_map] map_climate_zones (mg_temp)
enum { MG_L_TWO, MG_L_ONE, MG_L_NEW_WORLD };   // [random_map] map_layout (mg_layout)
// THE LAYOUTS (the player 2026-11-26: "Two Continents WE ... and Two Continents NS"; 2026-11-27 of the ones I proposed:
// "do one continent first, then old and new world"). Two continents: 0.42 of the half wide, 0.85 long, their middles
// half the half off the map's middle, the channel between. One continent: 0.8 wide, 0.85 long in the middle (as much
// land as the two have, no channel), the players round it. Old and New World: all players on the Old World, a strip
// along one side, the New World across the channel some three quarters of the land, richer (mg_objects), nobody's at
// the start: a place to settle by sea. The two 0.16 of the half apart as the two continents are.
// THE ROTATION (map_rotation; the player 2026-11-27: "two continents are already 2 layouts, new and old world could
// be expanded into 4 to get every direction - maybe we need a layout rotation modifier?"): the layout turned by
// quarters clockwise, the first continent (seat 0's, the Old World) west, north, east, south - the frame north and
// south for 90 and 270 degrees (mg_along, mg_across), the continents on the other side of the middle for 180 and 270;
// random: a quarter from two bits of the seed (the map's draws stay as they are). The weather keeps the map's north.
internal void mg_layout(struct mg *g) {
    int l = (unsigned)g->opt.lay <= MG_L_NEW_WORLD ? g->opt.lay : MG_L_TWO;
    int r = g->opt.rot >= 0 && g->opt.rot < 4 ? g->opt.rot : (int)((g->seed >> 5) & 3);
    float s = r >= 2 ? -1.0f : 1.0f;   // (a sign on the continents' places across: 1 the first west or north)
    g->opt.ns = r & 1;
    g->turned = r;
    if (l == MG_L_ONE) {
        g->nc = g->homes = 1; g->chan_on = 0; g->chan = 0;
        g->cont[0].at = 0; g->cont[0].wide = 0.8f; g->cont[0].len = 0.85f;
    } else if (l == MG_L_NEW_WORLD) {
        // the Old World a strip along the map's side, 0.2 of the half wide, from 0.88 of the half out; 0.16 of
        // channel; the New World the rest to the far side's 0.88 - three quarters of the land (the player 2026-11-27:
        // "the new world should at least get 3/4 of the map"; a third larger than the Old World was "very similar
        // to two continents still"; widened for many players it was so again) - on every size: on 116 tiles four
        // players don't fit the strip (two starts in four forced, 210 tiles with five none), and that is the small
        // map's and the player count's, not the layout's to give up its shape for (the player: "not every map size
        // has to work for every player count")
        float ow = 0.2f, oat = -0.88f + ow, from = oat + ow + 0.16f;
        g->nc = 2; g->homes = 1; g->chan_on = 1; g->chan = (oat + ow + 0.08f) * s * g->half;
        g->cont[0].at = oat * s; g->cont[0].wide = ow; g->cont[0].len = 0.88f;
        g->cont[1].at = (from + 0.88f) * 0.5f * s; g->cont[1].wide = (0.88f - from) * 0.5f; g->cont[1].len = 0.92f;
    } else {
        g->nc = g->homes = 2; g->chan_on = 1; g->chan = 0;
        g->cont[0].at = -0.5f * s; g->cont[1].at = 0.5f * s;
        g->cont[0].wide = g->cont[1].wide = 0.42f; g->cont[0].len = g->cont[1].len = 0.85f;
    }
    // the players round their one continent when it is about as wide as long (one_continent), else along it by turns
    // on either side of its middle line as on two continents (the Old World's strip: round it, two would sit right
    // across from each other)
    g->ring = g->homes == 1 && g->cont[0].len < 1.5f * g->cont[0].wide;
}
internal int mg_humans(const ee_setup *setup) {   // the humans' slots (1 this PC's or a seated player's, 2 a network one's)
    int humans = 0;
    for (int k = 0; setup && k < 16; k++) { int t = setup_slot_get_type(setup_slot_at(setup, k)); humans += t == 1 || t == 2; }
    return humans;
}
internal void mg_options(struct mg *g, const ee_setup *setup) {
    static const float amount[5] = { 0, 0.5f, 1, 2, 3 }, land[3] = { 0.8f, 1, 1.12f };
    static const float temp[5] = { -0.4f, -0.2f, 0, 0.2f, 0.4f }, wet[5] = { -0.2f, -0.1f, 0, 0.1f, 0.2f };
    static const float biome[5] = { 2.2f, 1.41f, 1, 0.71f, 0.5f };   // a patch's area by the square: a fifth, a half, ..., four times
    struct settings s = g_set;
    // more than one human in the setup, or this PC joined: multiplayer (the host of a single-player game is a server too)
    g->opt.mp = mg_humans(setup) > 1 || net_joined();
    g->opt.from = !g->opt.mp ? MG_FROM_OWN : g_mg_host.got ? MG_FROM_HOST : MG_FROM_DEFAULTS;
    for (int i = SETTING_map_layout; i <= SETTING_map_mines; i++) {
        int *v = (int *)((uint8_t *)&s + k_settings[i].off);
        if (g->opt.from == MG_FROM_HOST) *v = g_mg_host.v[i - SETTING_map_layout];
        else if (g->opt.from == MG_FROM_DEFAULTS) setting_default(&k_settings[i], (uint8_t *)v);
        g->opt.v[i - SETTING_map_layout] = *v;
    }
#define MG_PICK(t, v) t[(unsigned)(v) < sizeof t / sizeof *t ? (unsigned)(v) : sizeof t / sizeof *t / 2]
    g->opt.lay = s.map_layout;
    g->opt.rot = s.map_rotation < 4 ? s.map_rotation : -1;   // (-1: random)
    g->opt.land = MG_PICK(land, s.map_land);
    g->opt.temp = MG_PICK(temp, s.map_climate);
    g->opt.wet = MG_PICK(wet, s.map_rain);
    g->opt.zones = (unsigned)s.map_climate_zones <= MG_ZONES_MIXED ? s.map_climate_zones : MG_ZONES_NORTH_SOUTH;
    g->opt.biome = MG_PICK(biome, s.map_biome_size);
    g->opt.mountains = MG_PICK(amount, s.map_mountains);
    g->opt.rivers = MG_PICK(amount, s.map_rivers);
    g->opt.lakes = MG_PICK(amount, s.map_lakes);
    g->opt.islands = MG_PICK(amount, s.map_islands);
    g->opt.forests = MG_PICK(amount, s.map_forests);
    g->opt.mines = MG_PICK(amount, s.map_mines);
    mg_layout(g);
#undef MG_PICK
}
internal void mg_options_log(const struct mg *g) {
    if (!g_log) return;
    static const char *const from[3] = { "", " (a multiplayer game: the host's)", " (a multiplayer game, none came from the host: the defaults)" };
    char line[400];
    int at = snprintf(line, sizeof line, "# mapgen: \"%s\" options%s:", MG_NAME, from[g->opt.from]);
    for (int i = SETTING_map_layout; i <= SETTING_map_mines && at > 0 && at < (int)sizeof line - 40; i++) {
        char name[32];
        setting_choice_name(k_settings[i].choices, g->opt.v[i - SETTING_map_layout], name, sizeof name);
        at += snprintf(line + at, sizeof line - (size_t)at, " %s %s", k_settings[i].key + 4, name);
    }
    log_locked("%s\n", line);
}

// The host's options sent (its Start, the lobby checked): [random_map]'s choices, "play this map" and its seed. The
// payload: a format byte (1), the choices' count, the choices, keep, the seed (little-endian)
internal int mg_host_send(void) {
    uint8_t b[2 + MG_OPTS + 1 + 4];
    b[0] = 1;
    b[1] = MG_OPTS;
    for (int i = SETTING_map_layout; i <= SETTING_map_mines; i++) b[2 + i - SETTING_map_layout] = (uint8_t)*(const int *)((const uint8_t *)&g_set + k_settings[i].off);
    int keep = g_mg_keep.on && setup_reveal_map(setup_lobby_at());   // (only on a revealed map: md_open_setup)
    b[2 + MG_OPTS] = (uint8_t)keep;
    memcpy(b + 3 + MG_OPTS, &g_mg_keep.seed, 4);
    int sent = mod_msg_send(MOD_MSG_MAP_OPTIONS, b, (int)sizeof b);
    if (g_log) log_locked("# mapgen: the host's \"%s\" options %s to every PC%s\n", MG_NAME, sent ? "sent" : "NOT sent (no server, the message not registered)",
                          keep ? ", with the preview's seed (play this map)" : g_mg_keep.on ? ", not the preview's seed: the map is not revealed" : "");
    if (sent) g_mg_keep.on = 0;   // its seed goes with the options: every PC's map from it
    return sent;
}
// ... and carried out on every PC, the host's too: kept for the game's map. Only from the host (on every PC its
// messages come from 0xFE), only the format known
internal void mg_host_seen(int from, const uint8_t *data, int n) {
    if (from != 0xFE || n != 2 + MG_OPTS + 1 + 4 || data[0] != 1 || data[1] != MG_OPTS) {
        if (g_log) log_locked("# mapgen: a message of map options left out (from slot %d, %d bytes, format %d) - another build's?\n", from, n, n > 0 ? data[0] : -1);
        return;
    }
    memcpy(g_mg_host.v, data + 2, MG_OPTS);
    g_mg_host.keep = data[2 + MG_OPTS];
    memcpy(&g_mg_host.seed, data + 3 + MG_OPTS, 4);
    g_mg_host.got = 1;
    if (g_log) log_locked("# mapgen: the host's \"%s\" options came%s\n", MG_NAME, g_mg_host.keep ? ", with the seed of its preview" : "");
}
// a lobby set up (the setup screen's activations): what an earlier one's host sent is not this game's
internal void mapgen_host_forget(void) { g_mg_host.got = 0; }
// the Start button's check (setup_functions.lobby_check): passed on a multiplayer game's host with the mod's type
// chosen in a random-map game, the options go - before the start message the button's code sends next
internal uint8_t __cdecl hook_lobby_check(void) {
    uint8_t ok = lobby_check();
    ee_setup *s = setup_lobby_at();
    const char *map = ok && net_hosting() && *game_type_var() == 0 && mg_humans(s) > 1 ? FIELD(setup_map_name_at(s), 4, const char *) : NULL;   // (the UString's characters)
    if (map && mapgen_is_ours(map)) mg_host_send();
    return ok;
}
// noise: a number 0 .. 1 per lattice point of a seed, smoothed between them; three octaves of it
internal float mg_lattice(int x, int y, uint32_t s) {
    uint32_t h = (uint32_t)x * 0x9E3779B1u ^ (uint32_t)y * 0x85EBCA77u ^ s * 0xC2B2AE3Du;
    h ^= h >> 15; h *= 0x2C1B3C6Du; h ^= h >> 12; h *= 0x297A2D39u; h ^= h >> 15;
    return (float)(h >> 8) * (1.0f / 16777216.0f);
}
internal float mg_noise(float x, float y, uint32_t s) {
    int ix = (int)floorf(x), iy = (int)floorf(y);
    float fx = x - (float)ix, fy = y - (float)iy;
    fx = fx * fx * (3 - 2 * fx);
    fy = fy * fy * (3 - 2 * fy);
    float a = mg_lattice(ix, iy, s), b = mg_lattice(ix + 1, iy, s), c = mg_lattice(ix, iy + 1, s), d = mg_lattice(ix + 1, iy + 1, s);
    return (a + (b - a) * fx) * (1 - fy) + (c + (d - c) * fx) * fy;
}
internal float mg_fbm(float x, float y, uint32_t s) {
    return 0.5f * mg_noise(x, y, s) + 0.3f * mg_noise(x * 2, y * 2, s + 1) + 0.2f * mg_noise(x * 4, y * 4, s + 2);
}
internal float mg_step(float lo, float hi, float v) {   // 0 below lo, 1 above hi, smooth between
    float t = (v - lo) / (hi - lo);
    t = t < 0 ? 0 : t > 1 ? 1 : t;
    return t * t * (3 - 2 * t);
}
// a corner of the map: the game's map's, a preview's the diamond the game's has - u and v within half the side and
// one more (its rows read 2026-11-26: row 0 of 150 tiles corners 74 .. 76, the middle rows 0 .. 150; a tile where its
// four corners are)
internal inline int mg_corner(const struct mg *g, int x, int y) {
    if (x < 0 || y < 0 || x > g->n || y > g->n) return 0;
    return g->map ? map_corner(g->map, x, y) != NULL : fabsf((float)(x - y)) <= g->half + 1 && fabsf((float)(x + y - g->n)) <= g->half + 1;
}
internal inline ee_tile *mg_tile(const struct mg *g, int x, int y) { return x >= 0 && y >= 0 && x < g->n && y < g->n ? map_get_tiles(g->map)[y * g->n + x] : NULL; }
// a tile of the map (a preview's: its four corners are)
internal inline int mg_on(const struct mg *g, int x, int y) {
    if (g->map) return mg_tile(g, x, y) != NULL;
    return x >= 0 && y >= 0 && x < g->n && y < g->n && mg_corner(g, x, y) && mg_corner(g, x + 1, y) && mg_corner(g, x, y + 1) && mg_corner(g, x + 1, y + 1);
}
// what a tile is to a walker: the game's map's kind, a preview's by the game's rules (mg_kinds)
enum { MG_K_OFF, MG_K_LAND, MG_K_SHALLOWS, MG_K_DEEP, MG_K_CLIFF };
internal int mg_kind(const struct mg *g, int x, int y) {
    if (!g->map) return x >= 0 && y >= 0 && x < g->n && y < g->n ? g->kind[y * g->n + x] : MG_K_OFF;
    ee_tile *t = mg_tile(g, x, y);
    return !t ? MG_K_OFF : tile_is_cliff(t) ? MG_K_CLIFF : tile_is_deep(t) ? MG_K_DEEP : tile_is_shallows(t) ? MG_K_SHALLOWS : MG_K_LAND;
}
internal inline float mg_f(const struct mg *g, int x, int y) { return mg_corner(g, x, y) ? g->f[y * g->m + x] : -99.0f; }
// THE CONTINENTS' FRAME (map_layout): along them and across - u and v west and east, v and u north and south.
// mg_side: which continent a place is on - of two the one on its side of the channel (the two continents' first under
// 0, seat 0's: the west, the north), of one that one. mg_chan: how far across from the channel (none: far). mg_from:
// a place's x, y from along and across
internal inline int mg_along(const struct mg *g, int x, int y) { return g->opt.ns ? x + y - g->n : x - y; }
internal inline int mg_across(const struct mg *g, int x, int y) { return g->opt.ns ? x - y : x + y - g->n; }
internal inline int mg_side(const struct mg *g, int x, int y) { return g->nc < 2 ? 0 : ((float)mg_across(g, x, y) > g->chan) != (g->cont[0].at > 0); }
internal inline float mg_chan(const struct mg *g, int x, int y) { return g->chan_on ? fabsf((float)mg_across(g, x, y) - g->chan) : 1e6f; }
// the continent the i-th range, plateau or lake goes to: by turns - but the New World gets two in three, the Old World
// all the players' room (on 116 tiles with four players it is some 30 tiles across: with half the mountains and lakes
// four games in five had a start forced, one cut off - 2026-11-27); the New World the wild one
internal inline int mg_feature_on(const struct mg *g, int i) { return g->nc > g->homes ? i % 3 != 2 : i % g->nc; }
internal void mg_from(const struct mg *g, float along, float across, float *x, float *y) {
    float u = g->opt.ns ? across : along, v = g->opt.ns ? along : across;
    *x = (u + v + (float)g->n) * 0.5f; *y = (v + (float)g->n - u) * 0.5f;
}
// a tile's lowest and highest corner (height steps); 0: no tile there
internal int mg_tile_span(const struct mg *g, int x, int y, float *lo, float *hi) {
    if (!mg_on(g, x, y) || !mg_corner(g, x + 1, y) || !mg_corner(g, x, y + 1) || !mg_corner(g, x + 1, y + 1)) return 0;
    const float *h = g->h + y * g->m + x;
    *lo = fminf(fminf(h[0], h[1]), fminf(h[g->m], h[g->m + 1]));
    *hi = fmaxf(fmaxf(h[0], h[1]), fmaxf(h[g->m], h[g->m + 1]));
    return 1;
}

internal float mg_temp(const struct mg *g, int x, int y);
// a point of the shore (f 0 .. 1.5 tiles inside) at least `clear` tiles from the map's edge and the middle channel,
// and the way out to sea there (ox, oy: down f's slope, length 1); 0: none found in 400 draws
internal int mg_shore_point(struct mg *g, float clear, int *px, int *py, float *ox, float *oy) {
    for (int t = 0; t < 400; t++) {
        int x = mg_int(g, 3, g->n - 3), y = mg_int(g, 3, g->n - 3);
        float f = mg_f(g, x, y), u = (float)(x - y), v = (float)(x + y - g->n);
        if (f < 0 || f > 1.5f || fminf(g->half - fabsf(u), g->half - fabsf(v)) < clear + 4 || mg_chan(g, x, y) < clear + 4) continue;
        float gx = mg_f(g, x - 2, y) - mg_f(g, x + 2, y), gy = mg_f(g, x, y - 2) - mg_f(g, x, y + 2), gl = sqrtf(gx * gx + gy * gy);
        if (gl < 0.2f) continue;
        *px = x; *py = y; *ox = gx / gl; *oy = gy / gl;
        return 1;
    }
    return 0;
}
// a tongue along a line from (x0, y0), `len` tiles the way (dx, dy), its half width `wide` at the root and 0.4 of it
// at the tip, a noise in its outline: land laid into the sea (sign 1: a cape) or sea cut into the land (-1: a bay)
internal void mg_tongue(struct mg *g, int x0, int y0, float dx, float dy, float len, float wide, int sign) {
    int reach = (int)(len + wide) + 3;
    for (int cy = y0 - reach; cy <= y0 + reach; cy++)
        for (int cx = x0 - reach; cx <= x0 + reach; cx++) {
            if (!mg_corner(g, cx, cy)) continue;
            float ex = (float)(cx - x0), ey = (float)(cy - y0), t = ex * dx + ey * dy;
            t = t < -3 ? -3 : t > len ? len : t;
            float ax = ex - dx * t, ay = ey - dy * t, r = wide * (1 - 0.6f * fmaxf(0, t) / len) + (mg_fbm((float)cx * 0.15f, (float)cy * 0.15f, g->seed + 95) - 0.5f) * 3.0f;
            float in = r - sqrtf(ax * ax + ay * ay);   // tiles inside the tongue
            float *f = &g->f[cy * g->m + cx];
            if (in > -6 && (sign > 0 ? in > *f : -in < *f)) *f = sign > 0 ? in : -in;   // 6 tiles of shoal (of shore) round it, no further
        }
}
// how far inside an island a corner is (tiles; under 0: outside it) - its radius pushed in and out by a noise
internal float mg_isle_in(const struct mg *g, int cx, int cy, int x, int y, float r) {
    return r * (0.7f + 0.6f * mg_fbm((float)cx * 0.2f, (float)cy * 0.2f, g->seed + 96)) - sqrtf((float)((cx - x) * (cx - x) + (cy - y) * (cy - y)));
}
// THE CONTINENTS' SHAPES (the player 2026-11-27: "the continents are quite obviously using a circle as a base, any
// idea to make their shape more interesting?" - an ellipse, its shore pushed some 3 tiles in and out by a noise; of
// my list: blobs along a spine, the place read warped, the land's amount kept). A continent in its ellipse's box: a
// spine of 4 - 5 blobs end to end along it, each its own width (0.7 - 1 of the ellipse's, the ends 0.65 of that -
// blunt ends made a box) and a little off the middle line; 2 - 4 lobes grown out of them (0.35 - 0.55 of the width,
// turned any way); 1 - 3 gulfs bitten into its sides (0.3 - 0.5). The blobs joined smoothly and the gulfs cut so; the
// place they are read at pushed by a slow noise (some a sixth of the width, at most half: the outline bent, its
// bumps hooked); kept a tenth of the width off the map's side and the channel, smoothly (mg_shape_kept); then the
// whole lifted or lowered until its land is nine tenths of its ellipse's (a thin one's all: mg_thin) - the same on every seed whatever outline
// came out. Not all of it: the ellipse fills its box all but the corners, and a shape with gulfs and as much land
// was pushed up against the channel and the map's side (lifts of a fifth of the width filled the gulfs again, the
// channel a straight cut 6 tiles wide - 2026-11-27, 8 seeds). The draws from a generator of their own (the map's
// draws stay as they were).
internal float mg_own_rnd(uint32_t *r) { uint32_t x = *r; x ^= x << 13; x ^= x >> 17; x ^= x << 5; *r = x; return (float)(x >> 8) * (1.0f / 16777216.0f); }
internal float mg_smax(float a, float b, float k) { float h = fmaxf(k - fabsf(a - b), 0) / k; return fmaxf(a, b) + h * h * k * 0.25f; }
// how far inside a blob (u/v units; under 0 outside): its axis (du, dv), its half lengths a along it, b across
internal float mg_blob_in(const struct mg_blob *b, float u, float v) {
    float eu = u - b->u, ev = v - b->v, p = eu * b->du + ev * b->dv, q = ev * b->du - eu * b->dv;
    return (1 - sqrtf(p * p / (b->a * b->a) + q * q / (b->b * b->b))) * fminf(b->a, b->b);
}
// how far inside the map's side and off the channel (u/v units): no land beyond (mg_land's last cut)
internal float mg_bounds(const struct mg *g, int x, int y) {
    float u = (float)(x - y), v = (float)(x + y - g->n);
    return fminf(fminf(g->half - fabsf(u), g->half - fabsf(v)), mg_chan(g, x, y)) - 4;
}
// a continent's shape lifted, kept off the map's side and the channel by a tenth of its width - smoothly: a lobe or a
// lift that came up to them bends away instead of being cut straight there
internal float mg_shape_kept(const struct mg *g, int k, float shape, float bounds) {
    float ra = g->cont[k].wide * g->half * g->opt.land;
    return -mg_smax(-shape, -(bounds - 0.1f * ra), 0.3f * ra);
}
// A THIN CONTINENT (under 0.3 as wide as long: the Old World's strip, some 30 tiles across on 210) has no gulfs, half
// the warp and all of its ellipse's land: the warp and the gulfs, scaled by its width, pinched it - 12 starts forced in
// 10 games of old_new_world (5 before) and one cut off from the others (2026-11-27)
internal inline int mg_thin(const struct mg *g, int k) { return g->cont[k].wide < 0.3f * g->cont[k].len; }
// how far inside continent k a corner is (u/v units), its lift left out
internal float mg_shape(const struct mg *g, int k, int x, int y) {
    float ra = g->cont[k].wide * g->half * g->opt.land, kw = 1.41f / ra, w = (mg_thin(g, k) ? 0.7f : 1.4f) * ra, s = (float)k * 37.0f;
    float u = (float)mg_along(g, x, y) + (mg_fbm((float)x * kw + 300 + s, (float)y * kw + 700, g->seed + 97) - 0.5f) * w;
    float v = (float)mg_across(g, x, y) + (mg_fbm((float)x * kw + 500, (float)y * kw + 100 + s, g->seed + 98) - 0.5f) * w;
    float in = -1e9f;
    for (int i = 0; i < g->blobs[k]; i++)
        if (!g->blob[k][i].bite) { float b = mg_blob_in(&g->blob[k][i], u, v); in = i ? mg_smax(in, b, 0.25f * ra) : b; }
    for (int i = 0; i < g->blobs[k]; i++)
        if (g->blob[k][i].bite) in = -mg_smax(-in, mg_blob_in(&g->blob[k][i], u, v), 0.15f * ra);
    return in;
}
internal void mg_shapes(struct mg *g) {
    uint32_t r = g->seed ^ 0x6A09E667u;
    r ^= r >> 16; r *= 0x7FEB352Du; r ^= r >> 15; r *= 0x846CA68Bu; r ^= r >> 16;
    if (!r) r = 1;
    for (int k = 0; k < g->nc; k++) {
        float ra = g->cont[k].wide * g->half * g->opt.land, rb = g->cont[k].len * g->half * g->opt.land, at = g->cont[k].at * g->half;
        struct mg_blob *b = g->blob[k];
        int nb = 0, spine = 4 + (mg_own_rnd(&r) < 0.5f);
        float gap = 1.2f * rb / (float)(spine - 1);
        for (; nb < spine; nb++) {   // (the ends 0.9 - 1 of the ellipse's length out, 0.65 as wide as the middle: blunt ends made a box)
            float t = -0.6f + 1.2f * (float)nb / (float)(spine - 1), wide = ra * (0.7f + 0.3f * mg_own_rnd(&r)) * sqrtf(1 - 1.6f * t * t);
            b[nb] = (struct mg_blob){ t * rb + (mg_own_rnd(&r) - 0.5f) * 0.3f * gap, at + (mg_own_rnd(&r) - 0.5f) * 1.6f * (ra - wide), 1, 0,
                                      gap * (0.75f + 0.25f * mg_own_rnd(&r)), wide, 0 };
        }
        for (int i = 2 + (int)(mg_own_rnd(&r) * 3.0f); i > 0 && nb < MG_BLOBS; i--, nb++) {
            const struct mg_blob *from = &b[(int)(mg_own_rnd(&r) * (float)spine) % spine];
            float du = mg_own_rnd(&r) - 0.5f, dv = mg_own_rnd(&r) - 0.5f, l = sqrtf(du * du + dv * dv);
            float au = mg_own_rnd(&r) - 0.5f, av = mg_own_rnd(&r) - 0.5f, al = sqrtf(au * au + av * av);
            float size = ra * (0.35f + 0.2f * mg_own_rnd(&r)), out = 0.6f + 0.3f * mg_own_rnd(&r), thin = 0.6f + 0.4f * mg_own_rnd(&r);
            if (l < 0.05f) { du = 0; dv = 1; l = 1; }
            if (al < 0.05f) { au = 1; av = 0; al = 1; }
            du /= l; dv /= l;
            // its middle that way from the spine blob's, `out` of the way to that one's rim - in the box
            float rim = 1 / sqrtf(du * du / (from->a * from->a) + dv * dv / (from->b * from->b));
            float cu = from->u + du * rim * out, cv = from->v + dv * rim * out;
            cu = fminf(fmaxf(cu, size - rb), rb - size); cv = fminf(fmaxf(cv, at - ra + size), at + ra - size);
            b[nb] = (struct mg_blob){ cu, cv, au / al, av / al, size, size * thin, 0 };
        }
        for (int i = mg_thin(g, k) ? 0 : 1 + (int)(mg_own_rnd(&r) * 3.0f); i > 0 && nb < MG_BLOBS; i--, nb++) {
            const struct mg_blob *from = &b[(int)(mg_own_rnd(&r) * (float)spine) % spine];
            float side = mg_own_rnd(&r) < 0.5f ? -1.0f : 1.0f, size = ra * (0.3f + 0.2f * mg_own_rnd(&r)), along = mg_own_rnd(&r) - 0.5f, len = 0.8f + 0.6f * mg_own_rnd(&r);
            b[nb] = (struct mg_blob){ from->u + along * from->a, from->v + side * (from->b + 0.1f * size), 1, 0, size * len, size, 1 };
        }
        g->blobs[k] = nb;
    }
    // the lifts, on every other corner each way: a bisection of the land's count against the ellipse's
    int w = g->n / 2 + 1;
    float *in = malloc((size_t)(w * w) * sizeof *in), *bound = malloc((size_t)(w * w) * sizeof *bound);
    for (int k = 0; k < g->nc; k++) {
        float ra = g->cont[k].wide * g->half * g->opt.land, rb = g->cont[k].len * g->half * g->opt.land, at = g->cont[k].at * g->half, lo = -ra, hi = ra;
        int want = 0, c = 0;
        g->lift[k] = 0;
        if (!in || !bound) continue;
        for (int y = 0; y <= g->n; y += 2)
            for (int x = 0; x <= g->n; x += 2, c++) {
                float da = ((float)mg_across(g, x, y) - at) / ra, db = (float)mg_along(g, x, y) / rb;
                bound[c] = mg_bounds(g, x, y);
                in[c] = mg_shape(g, k, x, y);
                want += fminf((1 - sqrtf(da * da + db * db)) * ra, bound[c]) > 0;
            }
        if (!mg_thin(g, k)) want = want * 9 / 10;
        for (int it = 0; it < 24; it++) {
            float mid = (lo + hi) * 0.5f;
            int got = 0;
            for (int i = 0; i < c; i++) got += mg_shape_kept(g, k, in[i] + mid, bound[i]) > 0;
            if (got < want) lo = mid; else hi = mid;
        }
        g->lift[k] = (lo + hi) * 0.5f;
    }
    free(in); free(bound);
}
// THE LAND: the layout's continents (mg_layout), each its shape (mg_shapes), and a rough edge to their shores - a quick
// noise (features of some 4 tiles) pushing them up to 3 tiles in and out, fading within 9 tiles of the shore: inlets
// and points, the inland untouched. f is the distance inside the land in tiles. Then the coast's own shapes (the player 2026-11-25 agreed to the list: "small offshore islands, bays and
// peninsulas, and fjords in the cold north"): capes - tongues of land 10 - 18 tiles out to sea -, bays - as far cut
// in -, 1 + n/200 of each; fjords in the cold north, 1 + n/150: a narrow cut (3 tiles of water) 14 - 24 tiles up
// the land's slope, winding a little, its shores rocky (g->fjord: cliffs); islands, 2 + n/80: 3 - 7 tiles in radius,
// its shore 8 tiles of water from any other land's - an island, no spit: deep water goes round it (mg_heights).
// Last the water along the map's edge and in the channel between two continents, whatever is there.
internal void mg_land(struct mg *g) {
    float half = g->half, ox, oy;
    int x, y;
    mg_shapes(g);
    for (y = 0; y <= g->n; y++)
        for (x = 0; x <= g->n; x++) {
            float best = -1e9f;
            for (int k = 0; k < g->nc; k++) best = fmaxf(best, mg_shape_kept(g, k, mg_shape(g, k, x, y) + g->lift[k], mg_bounds(g, x, y)));
            float f = best * 0.7071f, rough = (mg_fbm((float)x * 0.11f + 51, (float)y * 0.11f + 77, g->seed + 99) - 0.5f) * 8.0f;
            g->f[y * g->m + x] = f + rough * (1 - mg_step(2, 9, fabsf(f)));
        }
    for (int k = 1 + g->n / 200; k > 0; k--)
        if (mg_shore_point(g, 20, &x, &y, &ox, &oy)) { mg_tongue(g, x, y, ox, oy, (float)mg_int(g, 10, 18), (float)mg_int(g, 3, 5), 1); g->capes++; }
    for (int k = 1 + g->n / 200; k > 0; k--)
        if (mg_shore_point(g, 8, &x, &y, &ox, &oy)) { mg_tongue(g, x, y, -ox, -oy, (float)mg_int(g, 8, 14), (float)mg_int(g, 4, 6), -1); g->bays++; }
    for (int k = 1 + g->n / 150, t = 0; k > 0 && t < 60; t++) {
        if (!mg_shore_point(g, 8, &x, &y, &ox, &oy) || mg_temp(g, x, y) > -0.3f) continue;
        // from 3 tiles out at sea (ox, oy: the way out), up the land. Until 2026-11-27 it began 3 tiles inland - the
        // rocky shore's bluff closed its mouth: a long lake behind a cliff (the player: "the river doesn't break
        // through into the sea")
        float fx = (float)x + ox * 3, fy = (float)y + oy * 3, dx = -ox, dy = -oy;
        for (int i = 0, len = mg_int(g, 17, 27); i < len && mg_f(g, (int)fx, (int)fy) < 16; i++) {
            for (int cy = (int)fy - 6; cy <= (int)fy + 7; cy++)
                for (int cx = (int)fx - 6; cx <= (int)fx + 7; cx++) {
                    if (!mg_corner(g, cx, cy)) continue;
                    float ex = (float)cx - fx, ey = (float)cy - fy, d = sqrtf(ex * ex + ey * ey), by = 1 - mg_step(3, 6, d);
                    if (d - 1.6f < g->f[cy * g->m + cx]) g->f[cy * g->m + cx] = d - 1.6f;
                    if (by > g->fjord[cy * g->m + cx]) g->fjord[cy * g->m + cx] = by;
                }
            float turn = (mg_rnd(g) - 0.5f) * 0.5f, nx = dx - dy * turn, ny = dy + dx * turn, nl = sqrtf(nx * nx + ny * ny);
            dx = nx / nl; dy = ny / nl; fx += dx; fy += dy;
        }
        g->fjords++;
        k--;
    }
    for (int want = mg_amount(2 + g->n / 80, g->opt.islands), t = 0; g->islands < want && g->islands < 16 && t < 6000; t++) {
        x = mg_int(g, 0, g->n); y = mg_int(g, 0, g->n);
        float f = mg_f(g, x, y), u = (float)(x - y), v = (float)(x + y - g->n), r = (float)mg_int(g, 3, 7);
        int clear = f <= -8 && f >= -20 && fminf(half - fabsf(u), half - fabsf(v)) > r + 9 && mg_chan(g, x, y) > r + 8;
        // nothing within 4 tiles of it that is within 4 tiles of the land there is (the continents, a cape, an island before)
        for (int cy = y - 14; clear && cy <= y + 14; cy++)
            for (int cx = x - 14; clear && cx <= x + 14; cx++)
                clear = !mg_corner(g, cx, cy) || g->f[cy * g->m + cx] <= -4 || mg_isle_in(g, cx, cy, x, y, r) <= -4;
        if (!clear) continue;
        for (int cy = y - 18; cy <= y + 18; cy++)
            for (int cx = x - 18; cx <= x + 18; cx++) {
                if (!mg_corner(g, cx, cy)) continue;
                float in = mg_isle_in(g, cx, cy, x, y, r);
                if (in > -6 && in > g->f[cy * g->m + cx]) g->f[cy * g->m + cx] = in;
            }
        g->isle[g->islands].x = x; g->isle[g->islands].y = y; g->isle[g->islands].r = r;
        g->islands++;
    }
    for (y = 0; y <= g->n; y++)
        for (x = 0; x <= g->n; x++) {
            float u = (float)(x - y), v = (float)(x + y - g->n);
            float edge = fminf(half - fabsf(u), half - fabsf(v)) - 4, channel = mg_chan(g, x, y) - 4;
            g->f[y * g->m + x] = fminf(g->f[y * g->m + x], fminf(edge, channel) * 0.7071f);
        }
}
// THE COAST, its kind at a corner from a noise along it (the player 2026-11-24: "make the ocean beaches more dynamic,
// some wide beaches, (maybe with a cliff behind) some just rocky coasts"): beach 0 .. 1 a wide, flat beach of `width`
// tiles, bluff 0 .. 1 a cliff behind it; rocky 0 .. 1 the land up out of deep water at once; all 0: the plain shore.
internal void mg_coast(const struct mg *g, int x, int y, float *beach, float *width, float *bluff, float *rocky) {
    float c = mg_fbm((float)x * 0.022f + 800, (float)y * 0.022f + 130, g->seed + 50), c2 = mg_noise((float)x * 0.03f + 60, (float)y * 0.03f + 900, g->seed + 51);
    *beach = mg_step(0.56f, 0.64f, c);
    *width = 2 + 4 * mg_step(0.60f, 0.75f, c);
    *bluff = *beach * mg_step(0.5f, 0.6f, c2);
    *rocky = 1 - mg_step(0.36f, 0.44f, c);
    float fjord = g->fjord[y * g->m + x];   // a fjord's shores: rock, no beach
    *rocky = fmaxf(*rocky, fjord); *beach *= 1 - fjord; *bluff *= 1 - fjord;
}
// a bluff: 1.8 steps up within a tile from `at` tiles inside the land (a cliff: a tile whose corners differ by 1.5
// steps is one), down again to the plain 3 .. 12 tiles behind
internal float mg_bluff(float f, float at) { return 1.8f * mg_step(at, at + 0.8f, f) * (1 - mg_step(at + 3, at + 12, f)); }
internal float mg_start_dist(const struct mg *g, float x, float y);
// the temperature at a place before its height (its bands: cold under -0.3, hot over 0.3), by the climate zones
// ([random_map] map_climate_zones; the player 2026-11-26: "north to south (what we have now), poles (north and south
// are cold with a warm equator), none (no bias based on position, noise only)"): from the far north (-1) to the near
// south (1) along the continents; poles: 0.7 in the middle falling to -0.75 at the continents' ends (0.85 of the way)
// - cold, mild, hot, mild, cold along each, the hot band a quarter of it (from 1 at the middle a half of each
// continent was hot, deserts the most of it); mixed - a noise alone, 2.5 times as strong, its patches of warm and cold
// some 40 tiles across. Then the noise and the climate's shift. The noise's scale is the biome size's
// ([random_map] map_biome_size; normal: 0.025 a tile exactly, the maps as before): its patches larger or smaller, the
// zones' bands the same. North and south are the map's whatever the layout: with the continents north and south
// the northern one is the colder (poles: each cold at its far end, hot by the channel)
internal float mg_temp(const struct mg *g, int x, int y) {
    float k = 0.025f * g->opt.biome, u = (float)(x - y) / g->half, noise = mg_fbm((float)x * k + 90, (float)y * k + 40, g->seed + 20) - 0.5f;
    float lat = g->opt.zones == MG_ZONES_POLES ? 0.7f - 1.7f * fabsf(u) : g->opt.zones == MG_ZONES_MIXED ? 0 : u;
    return lat + noise * (g->opt.zones == MG_ZONES_MIXED ? 2.5f : 1.0f) + g->opt.temp;
}
// A MOUNTAIN RANGE (the player 2026-11-24: "cliffs, mountain ranges, rivers flowing from the mountains, increased
// chance for mine in mountains"; then, of the first ones - 5 steps high, in terraces -: "the mountains inland still
// need some work, they should also be a lot higher to justify the snow"): a line from a point well inside the land of
// the continent asked for (10 tiles), a tile a step, turned a little at random each step, n/6 .. n/3 tiles long or
// until it comes within 7 tiles of the shore (shorter than n/12: none) - wherever the starts will be: they are
// placed on the land made (mg_starts). The
// mountain along it: a crest up to 14 steps high - peaks of half to the whole of that by a noise along the line,
// lower towards its ends -, falling to the plain 10 - 15 tiles to either side, steeply by the crest (cliffs) and
// gently at its foot (to walk and build on); a pass every 30 - 45 tiles of the line, the crest down to a fifth for
// 6 tiles and up again over 7 more: a saddle to walk over (the first ones, 2 tiles and up over 4, were none - the
// peaks' flanks beside them filled the notch, the cliff pass the rest). A range has one
// cliff type, by the temperature at its middle (the player 2026-11-24, of mountains with the cold biomes' cliffs on
// top and the default below: "try to stick to a single cliff type"): the winter's but in the hot south, where it is
// the default's and the mountain bare of snow.
#define MG_PEAK 14.0f
internal int mg_range(struct mg *g, int continent) {
    float px[128], py[128];
    int x0 = mg_int(g, 0, g->n), y0 = mg_int(g, 0, g->n), want = mg_int(g, g->n / 6, g->n / 3), len = 0;
    if (mg_side(g, x0, y0) != continent || mg_f(g, x0, y0) < 10) return 0;
    float x = (float)x0, y = (float)y0, dx = mg_rnd(g) - 0.5f, dy = mg_rnd(g) - 0.5f, dl = sqrtf(dx * dx + dy * dy);
    if (dl < 0.01f) { dx = 1; dy = 0; dl = 1; }
    dx /= dl; dy /= dl;
    for (want = want > 127 ? 127 : want; len < want; len++) {
        px[len] = x; py[len] = y;
        float turn = (mg_rnd(g) - 0.5f) * 0.5f, nx = dx - dy * turn, ny = dy + dx * turn, nl = sqrtf(nx * nx + ny * ny);
        nx /= nl; ny /= nl;
        if (mg_f(g, (int)floorf(x + nx + 0.5f), (int)floorf(y + ny + 0.5f)) < 7) break;
        dx = nx; dy = ny; x += nx; y += ny;
    }
    if (len < g->n / 12 || g->ranges >= 15) return 0;
    int pass[6], npass = 0;
    for (int at = mg_int(g, 10, 18); at < len - 8 && npass < 6; at += mg_int(g, 30, 45)) pass[npass++] = at;
    for (int i = 0; i < len; i++) {
        float end = (float)(i < len - 1 - i ? i : len - 1 - i), gap = 99;
        for (int k = 0; k < npass; k++) gap = fminf(gap, fabsf((float)(i - pass[k])));
        float wide = 10 + 5 * mg_noise(px[i] * 0.08f, py[i] * 0.08f, g->seed + 60);
        float top = MG_PEAK * (0.4f + 0.6f * mg_step(0, 5, end)) * (0.5f + 0.5f * mg_noise((float)i * 0.2f, (float)g->ranges * 7.3f, g->seed + 63))
                    * (0.2f + 0.8f * mg_step(3, 10, gap));
        for (int cy = (int)py[i] - 28; cy <= (int)py[i] + 28; cy++)
            for (int cx = (int)px[i] - 28; cx <= (int)px[i] + 28; cx++) {
                if (!mg_corner(g, cx, cy)) continue;
                float ex = (float)cx - px[i], ey = (float)cy - py[i], d = sqrtf(ex * ex + ey * ey), s = 1 - d / wide, by = 1 - d / (wide + 12);
                float v = s > 0 ? top * s * s * sqrtf(sqrtf(s)) : 0;   // (1 - d / wide) ^ 2.25: a sharp crest, a flat foot
                if (by > g->foot[cy * g->m + cx]) g->foot[cy * g->m + cx] = by;
                if (v > g->mt[cy * g->m + cx]) { g->mt[cy * g->m + cx] = v; g->range[cy * g->m + cx] = (uint8_t)(g->ranges + 1); }
            }
    }
    g->range_cliffs[g->ranges] = mg_temp(g, (int)px[len / 2], (int)py[len / 2]) > 0.3f ? CLIFFS_DEFAULT : CLIFFS_WINTER;
    return 1;
}
// A PLATEAU (the player 2026-11-25: "terraces"): flat land 3.2 steps over the plain, 7 - 12 tiles in radius, its
// outline pushed in and out by a noise, a cliff all round but for two ramps (three from 10 tiles' radius) - 4 tiles
// wide, 6 long, evenly round it from a direction drawn - so it is always reached: a place for a fortress, a ramp a
// chokepoint. Well inside the land (14 tiles), off the ranges and their foothills,
// one plateau's rim 6 tiles from another's. Its cliffs are the red rock in the hot south, the winter's in the cold
// north, the default's between - one type each, as a range's. tight 0 .. 3: a small map's.
#define MG_PLATEAU 3.2f
internal int mg_plateau(struct mg *g, int continent, int tight) {
    int x0 = mg_int(g, 0, g->n), y0 = mg_int(g, 0, g->n), r = mg_int(g, 7, 12), ramps = r >= 10 ? 3 : 2;
    if (g->plateaus >= 8 || mg_side(g, x0, y0) != continent ||mg_f(g, x0, y0) < (float)(14 - 2 * tight)) return 0;
    for (int cy = y0 - r - 6; cy <= y0 + r + 6; cy += 3)
        for (int cx = x0 - r - 6; cx <= x0 + r + 6; cx += 3)
            if (!mg_corner(g, cx, cy) || g->mt[cy * g->m + cx] > 0.1f || g->foot[cy * g->m + cx] > 0.3f || g->plat[cy * g->m + cx] > 0 || mg_f(g, cx, cy) < 6) return 0;
    float rx[3], ry[3], dx = mg_rnd(g) - 0.5f, dy = mg_rnd(g) - 0.5f, dl = sqrtf(dx * dx + dy * dy);
    if (dl < 0.01f) { dx = 1; dy = 0; dl = 1; }
    rx[0] = dx / dl; ry[0] = dy / dl;
    for (int k = 1; k < ramps; k++) {   // turned by half a circle, or by a third each (cos, sin of 120 degrees)
        float c = ramps == 2 ? -1.0f : -0.5f, sn = ramps == 2 ? 0.0f : 0.8660254f;
        rx[k] = rx[k - 1] * c - ry[k - 1] * sn; ry[k] = rx[k - 1] * sn + ry[k - 1] * c;
    }
    for (int cy = y0 - r - 8; cy <= y0 + r + 8; cy++)
        for (int cx = x0 - r - 8; cx <= x0 + r + 8; cx++) {
            if (!mg_corner(g, cx, cy)) continue;
            float ex = (float)(cx - x0), ey = (float)(cy - y0), w = 0;
            float in = (float)r * (0.65f + 0.7f * mg_fbm((float)cx * 0.09f, (float)cy * 0.09f, g->seed + 90)) - sqrtf(ex * ex + ey * ey);   // tiles inside its rim
            for (int k = 0; k < ramps; k++)
                if (ex * rx[k] + ey * ry[k] > 0) w = fmaxf(w, 1 - mg_step(2.0f, 3.5f, fabsf(ex * ry[k] - ey * rx[k])));
            float steep = mg_step(0, 0.9f, in), up = MG_PLATEAU * (steep + (mg_step(-3, 3, in) - steep) * w);
            if (up > g->plat[cy * g->m + cx]) { g->plat[cy * g->m + cx] = up; g->range[cy * g->m + cx] = (uint8_t)(16 + g->plateaus); }
        }
    float temp = mg_temp(g, x0, y0);
    g->range_cliffs[15 + g->plateaus] = temp > 0.3f ? CLIFFS_ROCKY : temp < -0.3f ? CLIFFS_WINTER : CLIFFS_DEFAULT;
    g->plateau[g->plateaus].x = x0; g->plateau[g->plateaus].y = y0; g->plateau[g->plateaus].r = r;
    g->plateaus++;
    return 1;
}
// THE HEIGHTS. The plain shore rises 0.3 steps a tile to the plain at 1 step, the sea falls as gently to 5 steps
// down; a wide beach lies nearly flat for its width (0.08 a tile, the sea before it shallow twice as far), a rocky
// coast and a beach's bluff are cliffs. Inland the plain swells by up to 0.8 steps; HILL COUNTRY (the player
// 2026-11-25: "hills as well as lakes would be my first ideas") where a slow third layer - the land's roughness, as
// temperature and moisture are layers - says so, and as foothills within 12 tiles of a range: rolling hills of up to
// 3 steps, some 14 tiles across, never a cliff (0.8 steps a tile at their steepest). The mountains (the ranges
// before: g->mt) keep 5 tiles off the shore, a noise makes their sides rugged (0.75 .. 1.25 of their height).
internal void mg_heights(struct mg *g) {
    for (int y = 0; y <= g->n; y++)
        for (int x = 0; x <= g->n; x++) {
            float f = g->f[y * g->m + x], hn = mg_fbm((float)x * 0.035f + 17, (float)y * 0.035f + 5, g->seed + 10), beach, width, bluff, rocky;
            mg_coast(g, x, y, &beach, &width, &bluff, &rocky);
            if (f < 0) {
                // the sea: deeper the further out. Its first 0.75 steps are shallows a land unit walks through
                // (map.c tile_is_shallows) - 5 tiles of them off a wide beach: the channel between the continents is
                // deep whatever its shores are, and so is a ring 3 - 5 tiles off an island's shore
                float deep = f * (0.3f - 0.15f * beach + 1.0f * rocky);
                deep = fminf(deep, -1.5f * (1 - mg_step(2, 5, mg_chan(g, x, y))));
                for (int i = 0; i < g->islands; i++) {
                    float in = abs(x - g->isle[i].x) > 16 || abs(y - g->isle[i].y) > 16 ? -99 : mg_isle_in(g, x, y, g->isle[i].x, g->isle[i].y, g->isle[i].r);
                    if (in > -5 && in < -3) deep = fminf(deep, -1.2f);
                }
                g->h[y * g->m + x] = fmaxf(deep, -5.0f);
                continue;
            }
            float base = fminf(f * 0.3f, 1.0f), flat = fminf(f * 0.08f, base);
            float h = base + (flat - base) * beach * (1 - mg_step(width, width + 3, f)) + bluff * mg_bluff(f, width) + rocky * mg_bluff(f, 0);
            float hilly = fmaxf(mg_step(0.5f, 0.62f, mg_fbm((float)x * 0.012f + 400, (float)y * 0.012f + 900, g->seed + 70)), 0.8f * mg_step(0.1f, 0.5f, g->foot[y * g->m + x]));
            float hn2 = 0.7f * mg_noise((float)x * 0.07f + 17, (float)y * 0.07f + 5, g->seed + 71) + 0.3f * mg_noise((float)x * 0.14f + 3, (float)y * 0.14f + 8, g->seed + 72);
            h += fmaxf(0, hn - 0.5f) * 1.6f * mg_step(5, 14, f);
            h += hilly * mg_step(4, 10, f) * (1 - mg_step(0.2f, 1.0f, g->plat[y * g->m + x])) * 6.0f * fmaxf(0, hn2 - 0.38f);   // a plateau's top is flat
            h += g->plat[y * g->m + x];
            float m = g->mt[y * g->m + x] * mg_step(5, 10, f);
            m *= 0.75f + 0.5f * mg_fbm((float)x * 0.2f + 31, (float)y * 0.2f + 9, g->seed + 62);
            g->mt[y * g->m + x] = m;
            g->h[y * g->m + x] = h + m;
        }
}
// THE STARTS, on the land made (the player 2026-11-26: "instead of deciding them first the map gets generated first
// independently, with the mod picking good starts after and adding the extra resource patches if needed"). Before,
// the starts came first and every range, plateau, hill, lake and river kept 11 - 24 tiles off them: each start sat
// in the same bare ring, and on a small map the features found no room (116 tiles, 4 players: no range, no lake,
// whatever the options). Now the land is made whole, and once the ground has its tile kinds (the first pass of
// mg_steps) the players are placed on it by turns on the two continents, each in its share of its continent's
// length - where it looks first is that share's middle, by turns on either side of the continent's middle line,
// 0.6 of the way to its shore -, at the best place within reach of it. A place (a tile corner) needs a yard round it of what
// levelling can't make ground to build on - no water, cliff, mountain or plateau (mg_starts_fields: bad) - 7 tiles
// every way, no water within 9 (the yard's levelling fades out at 10: a river's bed or a lake's shore stays as it is)
// and to be 8 tiles inside the land. Its worth: room (the share of the land 18 tiles round to build on, flat enough
// as well: farms, houses, walls), smooth ground (the heights' spread 8 tiles round: hills are levelled, but the less
// the better), water within 22 - 40 tiles (docks, fish), the biome's food (its forage and herds; counted twice - the
// first epoch lives on them) - less the further from where it looked first along the continent, 0.15 of that
// across it. The levels, the first with a place: 0 within reach (0.45 of the share, 12 .. 40 tiles), 1 within twice
// that, 2 so with 5 tiles of yard, water 6 off and 6 tiles inside the land, 3 with 3, 4 and 4, 4 forced: the place
// within twice the reach furthest from all of those (its yard levelled, whatever is there). A start keeps 0.6 of a
// share (16 tiles at least) from its continent's others, 16 from the other's.
// Nothing here is drawn at random: the places follow from the land, the land from the seed.
struct mg_fields {
    uint8_t *bad, *wet;   // [tile] the Chebyshev distance to water, a cliff, a mountain or a plateau (off the map too) / to water
    int *room;            // [(n + 1)^2] the summed area of the tiles to build on: none of those, under 0.8 steps between its corners
    double *hs, *hq;      // [(m + 1)^2] the summed area of the corners' heights and their squares
};
// the distances by two passes over the tiles (each tile the least of its 8 neighbours' + 1): exact for Chebyshev's
internal void mg_cheb(uint8_t *d, int n) {
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            int v = d[y * n + x];
            if (x > 0) v = v < d[y * n + x - 1] + 1 ? v : d[y * n + x - 1] + 1;
            if (y > 0)
                for (int dx = -1; dx <= 1; dx++)
                    if (x + dx >= 0 && x + dx < n && d[(y - 1) * n + x + dx] + 1 < v) v = d[(y - 1) * n + x + dx] + 1;
            d[y * n + x] = (uint8_t)(v > 255 ? 255 : v);
        }
    for (int y = n - 1; y >= 0; y--)
        for (int x = n - 1; x >= 0; x--) {
            int v = d[y * n + x];
            if (x < n - 1) v = v < d[y * n + x + 1] + 1 ? v : d[y * n + x + 1] + 1;
            if (y < n - 1)
                for (int dx = -1; dx <= 1; dx++)
                    if (x + dx >= 0 && x + dx < n && d[(y + 1) * n + x + dx] + 1 < v) v = d[(y + 1) * n + x + dx] + 1;
            d[y * n + x] = (uint8_t)(v > 255 ? 255 : v);
        }
}
internal int mg_starts_fields(const struct mg *g, struct mg_fields *f) {
    int n = g->n, m = g->m;
    f->bad = malloc((size_t)(n * n)); f->wet = malloc((size_t)(n * n));
    f->room = calloc((size_t)((n + 1) * (n + 1)), sizeof *f->room);
    f->hs = calloc((size_t)((m + 1) * (m + 1)), sizeof *f->hs); f->hq = calloc((size_t)((m + 1) * (m + 1)), sizeof *f->hq);
    if (!f->bad || !f->wet || !f->room || !f->hs || !f->hq) return 0;
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            float lo, hi;
            // by the heights alone, not the tiles' kinds: the game's cliff pass makes a few more cliffs than a
            // preview's (a tile beside a steep one, by its textures) - a start placed by them parted a map from its
            // preview. Water is the game's rule (mg_kinds), a cliff a tile with 1.2 steps between its corners or more
            // (the game's at 1.5, those it adds beside them a tile further: outside the yard's middle)
            int i = y * m + x, bad = 1, steep = 1, wet = 0;
            if (mg_tile_span(g, x, y, &lo, &hi)) {
                const float *h = g->h + i;
                int dry = (h[0] >= 0) + (h[1] >= 0) + (h[m] >= 0) + (h[m + 1] >= 0);
                float mt = fmaxf(fmaxf(g->mt[i], g->mt[i + 1]), fmaxf(g->mt[i + m], g->mt[i + m + 1]));
                float pl = fmaxf(fmaxf(g->plat[i], g->plat[i + 1]), fmaxf(g->plat[i + m], g->plat[i + m + 1]));
                wet = lo < 0 && dry < 3;
                bad = wet || hi - lo >= 1.2f || mt >= 0.3f || pl >= 0.3f;
                steep = hi - lo >= 0.8f;
            }
            f->bad[y * n + x] = bad ? 0 : 255;
            f->wet[y * n + x] = wet ? 0 : 255;
            f->room[(y + 1) * (n + 1) + x + 1] = (!bad && !steep) + f->room[y * (n + 1) + x + 1] + f->room[(y + 1) * (n + 1) + x] - f->room[y * (n + 1) + x];
        }
    mg_cheb(f->bad, n);
    mg_cheb(f->wet, n);
    for (int y = 0; y < m; y++)
        for (int x = 0; x < m; x++) {
            double h = mg_corner(g, x, y) ? (double)g->h[y * m + x] : 0;
            int at = (y + 1) * (m + 1) + x + 1, up = y * (m + 1) + x + 1, left = (y + 1) * (m + 1) + x, both = y * (m + 1) + x;
            f->hs[at] = h + f->hs[up] + f->hs[left] - f->hs[both];
            f->hq[at] = h * h + f->hq[up] + f->hq[left] - f->hq[both];
        }
    return 1;
}
internal void mg_starts_fields_free(struct mg_fields *f) { free(f->bad); free(f->wet); free(f->room); free(f->hs); free(f->hq); }
// a square's sum of a summed area (side w + 1 of it), x0 .. x1 and y0 .. y1 clipped to 0 .. w - 1
internal double mg_area(const double *s, const int *si, int w, int x0, int y0, int x1, int y1, int *count) {
    x0 = x0 < 0 ? 0 : x0; y0 = y0 < 0 ? 0 : y0; x1 = x1 > w - 1 ? w - 1 : x1; y1 = y1 > w - 1 ? w - 1 : y1;
    *count = (x1 - x0 + 1) * (y1 - y0 + 1);
    int a = (y1 + 1) * (w + 1) + x1 + 1, b = y0 * (w + 1) + x1 + 1, c = (y1 + 1) * (w + 1) + x0, d = y0 * (w + 1) + x0;
    return s ? s[a] - s[b] - s[c] + s[d] : (double)(si[a] - si[b] - si[c] + si[d]);
}
// a place's worth (see above); ax, ay where it looked first, reach the reach
internal float mg_start_worth(const struct mg *g, const struct mg_fields *f, int x, int y, float ax, float ay, float reach) {
    int n = g->n, c;
    double room = mg_area(NULL, f->room, n, x - 18, y - 18, x + 17, y + 17, &c) / (37.0 * 37.0);   // (off the map: no room)
    double s = mg_area(f->hs, NULL, g->m, x - 8, y - 8, x + 8, y + 8, &c), q = mg_area(f->hq, NULL, g->m, x - 8, y - 8, x + 8, y + 8, &c);
    double var = q / c - (s / c) * (s / c);
    float rough = sqrtf((float)(var > 0 ? var : 0)), wet = (float)f->wet[y * n + x];
    float water = wet <= 22 ? 1 : wet >= 40 ? 0 : (40 - wet) / 18;
    const struct mg_biome *b = &k_mg_biome[g->clim[y * n + x]];
    float food = (float)(b->rich[MG_RICH_FORAGE] + b->rich[MG_RICH_HERD]) / 200.0f;
    // out of its share along the continent (u) costs, across it (v) 0.15 of that: the continent's width is the
    // start's to use (the player 2026-11-26: "still placed too much on the vertical line through the center of the
    // continent, they could spread way more horizontal")
    // (the players round one continent: as far either way - they keep apart by `apart`)
    float du = (float)(x - y) - (ax - ay), dv = (float)(x + y) - (ax + ay);
    if (g->opt.ns) { float t = du; du = dv; dv = t; }   // (along the continents is v there)
    float off = (du * du + (g->ring ? 1.0f : 0.15f) * dv * dv) * 0.5f / (reach * reach);
    return 3 * (float)room - 2 * rough + water + 2 * food - 1.5f * off;
}
// The players on one continent (one_continent, old_new_world: homes 1) sit round it instead: where they look first
// evenly round the ellipse at 0.6 of the way from its middle to its shore (the shore its shape's: mg_ring_place) - the first across it (two players: on either
// side, in one climate), the rest turned on by a table (no sine at run time: every PC's map the same) -, their shares
// of that ring's length; a single player in the middle.
internal const float k_mg_turn[MG_PLAYERS][2] = {   // cos, sin of a full turn / (i + 1)
    { 1.00000000f, 0 }, { -1.00000000f, 0 }, { -0.50000000f, 0.86602540f }, { 0, 1.00000000f }, { 0.30901699f, 0.95105652f },
    { 0.50000000f, 0.86602540f }, { 0.62348980f, 0.78183148f }, { 0.70710678f, 0.70710678f }, { 0.76604444f, 0.64278761f },
    { 0.80901699f, 0.58778525f }, { 0.84125353f, 0.54064082f }, { 0.86602540f, 0.50000000f }, { 0.88545603f, 0.46472317f },
    { 0.90096887f, 0.43388374f }, { 0.91354546f, 0.40673664f }, { 0.92387953f, 0.38268343f },
};
// the longest run of continent k's land (f over 0) on a line from (u0, v0) the way (du, dv), t from lo to hi
// (u/v units, a step of 1) -> its ends a, b; 0: none
internal int mg_land_run(const struct mg *g, int k, float u0, float v0, float du, float dv, float lo, float hi, float *a, float *b) {
    int steps = (int)(hi - lo) + 1, best = 0;
    for (int i = 0, run = -1; i <= steps; i++) {
        int land = 0;
        if (i < steps) {
            float t = lo + (float)i, x, y;
            mg_from(g, u0 + du * t, v0 + dv * t, &x, &y);
            int cx = (int)floorf(x + 0.5f), cy = (int)floorf(y + 0.5f);
            land = mg_f(g, cx, cy) > 0 && mg_side(g, cx, cy) == k;
        }
        if (land && run < 0) run = i;
        if (!land && run >= 0) {
            if (i - run > best) { best = i - run; *a = lo + (float)run; *b = lo + (float)(i - 1); }
            run = -1;
        }
    }
    return best > 0;
}
// the j-th of count places round the home continent (u, v: the frame's along and across): the way round the ellipse
// as before, 0.6 of the way out on the land that way as it came out (mg_shapes: no longer the ellipse)
internal void mg_ring_place(const struct mg *g, int j, int count, float *u, float *v) {
    float ra = g->cont[0].wide * g->half * g->opt.land, rb = g->cont[0].len * g->half * g->opt.land, c = g->turned >= 2 ? -1.0f : 1.0f, s = 0;
    const float *turn = k_mg_turn[count - 1];
    for (int t = 0; t < j; t++) { float nc = c * turn[0] - s * turn[1]; s = s * turn[0] + c * turn[1]; c = nc; }
    float mid = g->cont[0].at * g->half, du = 0.6f * rb * s, dv = 0.6f * ra * c, l = sqrtf(du * du + dv * dv), a, b;
    *u = count > 1 ? du : 0;
    *v = mid + (count > 1 ? dv : 0);
    if (count < 2 || l <= 0 || !mg_land_run(g, 0, 0, mid, du / l, dv / l, 0, l / 0.6f * 1.3f, &a, &b)) return;
    float t = a + 0.6f * (b - a);
    *u = du / l * t; *v = mid + dv / l * t;
}
// Round one continent the shares are in tiles (u and v run 1.41 a tile), and the starts keep apart by no more than
// 0.9 of the nearest two places' distance: on the Old World of a small map (some 30 tiles across) the 16 tiles
// asked of two across from each other made one in four a forced start (2026-11-27, 10 games of 116 tiles)
// THE MAINLAND of each home continent: the largest piece of its land (tiles) a walker crosses by the game's tile kinds
// - shallows waded, a river's deep water too (a start cut off by one gets fords: mg_afoot), the sea and cliffs not.
// A start goes on it, never on a piece the ellipse's tip or the noise left apart (2026-11-27: the Old World's strip on
// a small map ended in one, and a start went there). -> the pieces' ids per tile (-1 none), main[c] continent c's
// (-1 none); NULL no memory
internal int *mg_mainland(const struct mg *g, int main[2]) {
    int n = g->n, *comp = malloc((size_t)(n * n) * sizeof *comp), *queue = malloc((size_t)(n * n) * sizeof *queue), size[2] = { 0, 0 }, id = 0;
    main[0] = main[1] = -1;
    if (!comp || !queue) { free(comp); free(queue); return NULL; }
    for (int i = 0; i < n * n; i++) comp[i] = -1;
    for (int i0 = 0; i0 < n * n; i0++) {
        if (comp[i0] >= 0 || mg_kind(g, i0 % n, i0 / n) != MG_K_LAND) continue;
        int head = 0, tail = 0, land = 0;
        queue[tail++] = i0; comp[i0] = id;
        while (head < tail) {
            int x = queue[head] % n, y = queue[head] / n;
            head++;
            land += mg_kind(g, x, y) == MG_K_LAND;
            for (int k = 0; k < 9; k++) {
                int tx = x + k % 3 - 1, ty = y + k / 3 - 1, kind = mg_kind(g, tx, ty);
                float lo, hi;
                if (kind == MG_K_OFF || kind == MG_K_CLIFF || comp[ty * n + tx] >= 0) continue;
                if (kind == MG_K_DEEP && !(mg_f(g, tx, ty) > 0.5f && mg_tile_span(g, tx, ty, &lo, &hi) && lo >= -1.0f)) continue;   // (the sea, a lake's middle)
                comp[ty * n + tx] = id;
                queue[tail++] = ty * n + tx;
            }
        }
        int c = mg_side(g, i0 % n, i0 / n);
        if (c < 2 && land > size[c]) { size[c] = land; main[c] = id; }
        id++;
    }
    free(queue);
    return comp;
}
internal void mg_starts(struct mg *g) {
    struct mg_fields f = { 0 };
    int main[2], *piece = mg_mainland(g, main);
    int n = g->n, homes = g->homes, count[2] = { (g->np + homes - 1) / homes, (g->np + homes - 2) / homes }, ok = mg_starts_fields(g, &f);
    float closest = 1e9f;
    for (int a = 0; g->ring && a < count[0]; a++)
        for (int b = a + 1; b < count[0]; b++) {
            float ua, va, ub, vb, xa, ya, xb, yb;
            mg_ring_place(g, a, count[0], &ua, &va); mg_ring_place(g, b, count[0], &ub, &vb);
            mg_from(g, ua, va, &xa, &ya); mg_from(g, ub, vb, &xb, &yb);
            closest = fminf(closest, sqrtf((xa - xb) * (xa - xb) + (ya - yb) * (ya - yb)));
        }
    for (int i = 0; i < g->np; i++) {
        int k = i % homes, j = i / homes, bx = -1, by = -1;
        float share, u, v;
        if (!g->ring) {
            float length = 2 * 0.72f * g->cont[k].len * g->half * g->opt.land;
            share = length / (float)count[k];
            // where it looks first: its share's middle along the continent, and by turns on either side of the
            // land's middle there, 0.6 of the way to its shore - the longest stretch of land across it there (mg_shapes;
            // none: the ellipse's) - a continent's one start in the middle
            u = (((float)j + 0.5f) / (float)count[k] - 0.5f) * length;
            float ra = g->cont[k].wide * g->half * g->opt.land, rb = g->cont[k].len * g->half * g->opt.land, a, b;
            float mid = g->cont[k].at * g->half, wide = ra * sqrtf(fmaxf(0, 1 - u * u / (rb * rb)));
            if (mg_land_run(g, k, u, 0, 0, 1, mid - 1.2f * ra, mid + 1.2f * ra, &a, &b)) { mid = (a + b) * 0.5f; wide = (b - a) * 0.5f; }
            v = mid + (count[k] > 1 ? (j & 1 ? 0.6f : -0.6f) * wide : 0);
        } else {
            float ra = g->cont[0].wide * g->half * g->opt.land, rb = g->cont[0].len * g->half * g->opt.land;
            share = 6.2831853f * 0.6f * sqrtf((ra * ra + rb * rb) * 0.5f) / (float)count[0] * 0.70710678f;
            mg_ring_place(g, j, count[0], &u, &v);
        }
        float reach = fminf(40, fmaxf(12, 0.45f * share)), apart = fmaxf(16, 0.6f * share);
        if (g->ring) apart = fminf(apart, 0.9f * closest);
        float ax, ay;
        mg_from(g, u, v, &ax, &ay);
        for (int level = 0; ok && level < 5 && bx < 0; level++) {
            static const int8_t yards[4] = { 7, 7, 5, 3 }, drys[4] = { 9, 9, 6, 4 }, insides[4] = { 8, 8, 6, 4 };
            float within = level ? 2 * reach : reach, best = -1e30f;
            int yard = yards[level & 3], dry = drys[level & 3], inside = insides[level & 3];
            for (int y = (int)(ay - within); y <= (int)(ay + within); y++)
                for (int x = (int)(ax - within); x <= (int)(ax + within); x++) {
                    float dx = (float)x - ax, dy = (float)y - ay;
                    if (x < 1 || y < 1 || x >= n - 1 || y >= n - 1 || mg_side(g, x, y) != k ||dx * dx + dy * dy > within * within) continue;
                    if (piece && main[k] >= 0 && piece[y * n + x] != main[k]) continue;   // (on its continent's mainland)
                    float in = mg_f(g, x, y);
                    if (level < 4 && (in < (float)inside || f.bad[y * n + x] < yard || f.wet[y * n + x] < dry)) continue;
                    int isle = 0;   // never on an offshore island (one 3 tiles off its shore: its continent's for mg_side)
                    for (int s = 0; s < g->islands && !isle; s++)
                        isle = abs(x - g->isle[s].x) < 20 && abs(y - g->isle[s].y) < 20 && mg_isle_in(g, x, y, g->isle[s].x, g->isle[s].y, g->isle[s].r) > -3;
                    if (isle) continue;
                    int crowded = 0;
                    for (int o = 0; o < i && !crowded; o++) {
                        float ex = (float)(x - g->sx[o]), ey = (float)(y - g->sy[o]), need = o % homes == k ? apart : 16;
                        crowded = ex * ex + ey * ey < need * need;
                    }
                    if (crowded) continue;
                    int clear = f.bad[y * n + x] < f.wet[y * n + x] ? f.bad[y * n + x] : f.wet[y * n + x];
                    float w = level < 4 ? mg_start_worth(g, &f, x, y, ax, ay, reach) : (float)clear - 0.05f * sqrtf(dx * dx + dy * dy);
                    if (w > best) { best = w; bx = x; by = y; }
                }
            if (bx >= 0) g->start_how[level]++;
        }
        if (bx < 0) { bx = (int)ax; by = (int)ay; g->start_how[4]++; }   // (no memory for the fields, or no land: where it looked first)
        g->sx[i] = bx; g->sy[i] = by;
    }
    g->placed = g->np;
    mg_starts_fields_free(&f);
    free(piece);
}
// a start's yard: the ground within 7 tiles levelled to its own mean height there (the land's, not the water's),
// fading out to 10 - but for water, a mountain's flank and a plateau beyond 7 tiles: they stay as they are -, the
// yard kept free
internal void mg_yards(struct mg *g) {
    for (int i = 0; i < g->np; i++) {
        int bx = g->sx[i], by = g->sy[i], c = 0;
        float mean = 0;
        for (int y = by - 7; y <= by + 7; y++)
            for (int x = bx - 7; x <= bx + 7; x++)
                if (mg_corner(g, x, y) && (x - bx) * (x - bx) + (y - by) * (y - by) < 49 && g->h[y * g->m + x] >= 0.3f) { mean += g->h[y * g->m + x]; c++; }
        mean = c ? mean / (float)c : 1.0f;
        for (int y = by - 10; y <= by + 10; y++)
            for (int x = bx - 10; x <= bx + 10; x++) {
                if (!mg_corner(g, x, y)) continue;
                float d = sqrtf((float)((x - bx) * (x - bx) + (y - by) * (y - by))), w = 1 - mg_step(7, 10, d), *h = &g->h[y * g->m + x];
                if (d >= 7 && (*h < 0.3f || g->mt[y * g->m + x] >= 0.3f || g->plat[y * g->m + x] >= 0.3f)) continue;
                if (*h < 0 && mg_chan(g, x, y) < 8) continue;   // never the channel between continents (a forced start's yard by it joined them, 2026-11-27)
                *h += (mean - *h) * w;
                if (d < 7 && mg_on(g, x, y)) g->used[y * g->n + x] = 1;
            }
    }
}
internal float mg_start_dist(const struct mg *g, float x, float y) {   // to the nearest start placed (tiles)
    float best = 1e9f;
    for (int i = 0; i < g->placed; i++) {
        float dx = x - (float)g->sx[i], dy = y - (float)g->sy[i], d = sqrtf(dx * dx + dy * dy);
        if (d < best) best = d;
    }
    return best;
}
// A RIVER: from a spring well inside the land (8 tiles) downhill of f - towards the nearest shore
// - a tile a step, its way turned a little at random each step, until it is in the sea or in water dug before it (a
// tributary; a lake's water too); one that finds no water in 250 steps or winds more than twice the straight way is
// not dug. Dug: the bed 0.95 steps under the water along its way (water 2 - 3
// tiles wide, the tiles of its middle too deep to walk through: under the shallows' 0.75 steps, map.c
// tile_is_shallows - a river is in the way), out through the sea's shallows to its deep water, its valley rising
// 0.3 a tile from the banks for 10 tiles (hills make way for it), a small pond at the spring; its banks (3 tiles)
// kept free. It ends inland at its spring; a ford every 12 - 18 tiles of its way lets units over (mg_fords).
// mountain: one from the mountains (the player 2026-11-24: "rivers flowing from the mountains") - its spring at a
// range's foot; no river's way goes over a mountain, no spring is within 20 tiles of another. fx, fy: from there (a
// lake's middle: its outflow), -1: a spring drawn.
internal int mg_river(struct mg *g, int mountain, int fx, int fy) {
    float px[256], py[256];
    int x0 = fx >= 0 ? fx : mg_int(g, 0, g->n), y0 = fx >= 0 ? fy : mg_int(g, 0, g->n), len = 0, out = 0, joined = 0;
    if (g->rivers >= 16) return 0;
    if (fx < 0) {
        if (mg_f(g, x0, y0) < 8) return 0;
        if (g->mt[y0 * g->m + x0] > 1.5f || (mountain && g->mt[y0 * g->m + x0] < 0.2f) || g->h[y0 * g->m + x0] < 0.5f || g->plat[y0 * g->m + x0] > 0) return 0;
        for (int i = 0; i < g->rivers; i++)
            if ((g->spring[i].x - x0) * (g->spring[i].x - x0) + (g->spring[i].y - y0) * (g->spring[i].y - y0) < 20 * 20) return 0;
    }
    float x = (float)x0, y = (float)y0, dx = 0, dy = 0;
    for (; len < 250; len++) {
        int ix = (int)floorf(x + 0.5f), iy = (int)floorf(y + 0.5f);
        float f = mg_f(g, ix, iy);
        if (f < -90 || g->mt[iy * g->m + ix] > 1.8f || g->plat[iy * g->m + ix] > 0.3f) return 0;
        px[len] = x; py[len] = y;
        if (f < -1.5f && (g->h[iy * g->m + ix] < -1.0f || f < -8)) break;   // in the sea's deep water
        if (g->h[iy * g->m + ix] >= 0) out = 1;                         // out of the lake it began in
        else if (out && f >= 0) { joined = 1; break; }                  // into a river or a lake dug before
        // down the land's slope, mixed with the way so far and a random push across it
        float gx = mg_f(g, ix - 2, iy) - mg_f(g, ix + 2, iy), gy = mg_f(g, ix, iy - 2) - mg_f(g, ix, iy + 2), gl = sqrtf(gx * gx + gy * gy);
        if (gl > 0.001f) { gx /= gl; gy /= gl; }
        dx = dx * 0.75f + gx * 0.25f; dy = dy * 0.75f + gy * 0.25f;
        float turn = (mg_rnd(g) - 0.5f) * 0.7f, nx = dx - dy * turn, ny = dy + dx * turn, nl = sqrtf(nx * nx + ny * ny);
        if (nl < 0.001f) { nx = gx; ny = gy; nl = 1; }
        dx = nx / nl; dy = ny / nl;
        x += dx; y += dy;
    }
    float wx = px[len < 250 ? len : 0] - px[0], wy = py[len < 250 ? len : 0] - py[0];
    if (len >= 250 || len < 12 || (float)len > 2 * sqrtf(wx * wx + wy * wy)) return 0;
    g->spring[g->rivers].x = x0; g->spring[g->rivers].y = y0;
    for (int i = 0; i <= len; i++) {
        float pond = i < 2 && fx < 0 ? 1.0f : 0;
        for (int cy = (int)py[i] - 10; cy <= (int)py[i] + 11; cy++)
            for (int cx = (int)px[i] - 10; cx <= (int)px[i] + 11; cx++) {
                if (!mg_corner(g, cx, cy)) continue;
                float ex = (float)cx - px[i], ey = (float)cy - py[i], d = fmaxf(0, sqrtf(ex * ex + ey * ey) - pond);
                float bed = d < 1.4f ? fmaxf(-0.95f, -1.9f + 1.407f * d) : 0.1f + 0.3f * (d - 1.5f) + g->mt[cy * g->m + cx] + g->plat[cy * g->m + cx];   // a mountain's flank and a plateau stay
                if (bed < g->h[cy * g->m + cx]) g->h[cy * g->m + cx] = bed;
                if (d < 3 && mg_on(g, cx, cy) && !g->used[cy * g->n + cx]) g->used[cy * g->n + cx] = 1;
            }
    }
    for (int i = fx < 0 ? mg_int(g, 6, 10) : mg_int(g, 13, 16); i < len - (joined ? 4 : 7) && g->fords < MG_FORDS - 16; i += mg_int(g, 12, 18)) {
        g->ford[g->fords].x = px[i]; g->ford[g->fords].y = py[i];
        g->fords++;
    }
    return 1;
}
// a basin dug: r tiles in radius, a noise in its outline, `deep` steps deep in the middle, the ground rising 0.5 a
// tile from its rim; its shore (a tile and a half) kept free
internal void mg_basin(struct mg *g, int x0, int y0, int r, float deep) {
    for (int cy = y0 - r - 6; cy <= y0 + r + 6; cy++)
        for (int cx = x0 - r - 6; cx <= x0 + r + 6; cx++) {
            if (!mg_corner(g, cx, cy)) continue;
            float d = sqrtf((float)((cx - x0) * (cx - x0) + (cy - y0) * (cy - y0)));
            float s = 1 - d * (0.75f + 0.5f * mg_fbm((float)cx * 0.15f, (float)cy * 0.15f, g->seed + 80)) / (float)r;
            float bed = s > 0 ? -deep * mg_step(0, 0.6f, s) : -s * (float)r * 0.5f;
            if (bed < g->h[cy * g->m + cx]) g->h[cy * g->m + cx] = bed;
            if (s > -1.5f / (float)r && mg_on(g, cx, cy)) g->used[cy * g->n + cx] = 1;
        }
}
// the moisture a place has of the noise alone - before the water near it and the mountains' rain shadow (mg_biomes);
// its scale the biome size's, as the temperature's
internal float mg_wet0(const struct mg *g, int x, int y) {
    float k = 0.03f * g->opt.biome;
    return 0.42f + g->opt.wet + (mg_fbm((float)x * k + 510, (float)y * k + 260, g->seed + 40) - 0.5f) * 1.5f;
}
// is the ground round (x0, y0) plain land for a feature of r tiles: no mountain, foothill, plateau or water, `inside`
// tiles inside the land, 14 from the lakes, oases and swamps so far
internal int mg_plain(const struct mg *g, int x0, int y0, int r, float inside) {
    if (mg_f(g, x0, y0) < inside) return 0;
    for (int i = 0; i < g->lakes + g->oases + g->swamps; i++) {
        int j = i - g->lakes, k = j - g->oases;
        int ox = j < 0 ? g->lake[i].x : k < 0 ? g->oasis[j].x : g->swamp[k].x, oy = j < 0 ? g->lake[i].y : k < 0 ? g->oasis[j].y : g->swamp[k].y;
        int apart = 14 + r + (j < 0 ? g->lake[i].r : k < 0 ? g->oasis[j].r : g->swamp[k].r);
        if ((ox - x0) * (ox - x0) + (oy - y0) * (oy - y0) < apart * apart) return 0;
    }
    for (int cy = y0 - r - 3; cy <= y0 + r + 3; cy += 3)
        for (int cx = x0 - r - 3; cx <= x0 + r + 3; cx += 3)
            if (!mg_corner(g, cx, cy) || g->mt[cy * g->m + cx] > 0.2f || g->foot[cy * g->m + cx] > 0.45f || g->plat[cy * g->m + cx] > 0 || g->h[cy * g->m + cx] < 0.8f || mg_f(g, cx, cy) < 5) return 0;
    return 1;
}
// AN OASIS (the player 2026-11-25 agreed to the list: "a desert oasis (pond, palms, forage)"): a pond of 2 - 3 tiles'
// radius where the land will be desert - hot and, of the noise alone, dry; its palms and its forage patch come with
// the objects (mg_objects), the water makes the ground round it less dry (mg_biomes).
internal int mg_oasis(struct mg *g) {
    int x0 = mg_int(g, 0, g->n), y0 = mg_int(g, 0, g->n), r = mg_int(g, 2, 3);
    if (g->oases >= 8 || mg_temp(g, x0, y0) < 0.4f || mg_wet0(g, x0, y0) > 0.3f || !mg_plain(g, x0, y0, r, 8)) return 0;
    mg_basin(g, x0, y0, r, 1.2f);
    g->oasis[g->oases].x = x0; g->oasis[g->oases].y = y0; g->oasis[g->oases].r = r;
    g->oases++;
    return 1;
}
// A SWAMP (the same list: "tropical swamp with trees standing in marsh"): where the land will be hot and wet, 9 - 13
// tiles in radius - the plain let down to the water's level and broken up by a fine noise: pools some 3 tiles across, 0.6 steps deep at most
// (four tenths of it; marsh their bed, the player's rule: marsh under water only) between low ground, where its trees
// stand thick (mg_objects). Its pools are shallows: land units wade through it, nothing is built in them. tight 0 .. 3: where
// no such place was found - smaller, less hot and wet.
internal int mg_swamp(struct mg *g, int tight) {
    int x0 = mg_int(g, 0, g->n), y0 = mg_int(g, 0, g->n), r = mg_int(g, 9, 13 - tight);
    if (g->swamps >= 4 || mg_temp(g, x0, y0) < 0.3f - 0.04f * (float)tight || mg_wet0(g, x0, y0) < 0.58f - 0.03f * (float)tight || !mg_plain(g, x0, y0, r, 8)) return 0;
    for (int cy = y0 - r; cy <= y0 + r; cy++)
        for (int cx = x0 - r; cx <= x0 + r; cx++) {
            if (!mg_corner(g, cx, cy)) continue;
            float d = sqrtf((float)((cx - x0) * (cx - x0) + (cy - y0) * (cy - y0))) * (0.8f + 0.4f * mg_fbm((float)cx * 0.12f, (float)cy * 0.12f, g->seed + 85));
            float w = 1 - mg_step((float)r - 3, (float)r, d), low = fmaxf(-0.6f, 0.2f + (mg_noise((float)cx * 0.3f + 9, (float)cy * 0.3f + 4, g->seed + 86) - 0.5f) * 2.6f);   // its pools shallows: waded through
            g->h[cy * g->m + cx] += (low - g->h[cy * g->m + cx]) * w;
        }
    g->swamp[g->swamps].x = x0; g->swamp[g->swamps].y = y0; g->swamp[g->swamps].r = r;
    g->swamps++;
    return 1;
}
// A LAKE (the player 2026-11-25): a basin 5 - 8 tiles in radius, its outline pushed in and out by a noise, 2.2 steps
// deep in the middle, by turns on the two continents, well inside the land (12 tiles), 25 from another lake, on the plain (no
// mountain, no foothills, no water there yet). The water is the sea's level - the game has one water plane. Its
// shore (a tile) is kept free. tight 0 .. 3: a small map's.
internal int mg_lake(struct mg *g, int tight) {
    int x0 = mg_int(g, 0, g->n), y0 = mg_int(g, 0, g->n), r = mg_int(g, 5, 8);
    if (g->lakes >= 12 || mg_side(g, x0, y0) != mg_feature_on(g, g->lakes) || mg_f(g, x0, y0) < (float)(12 - 2 * tight)) return 0;
    for (int i = 0; i < g->lakes; i++)
        if ((g->lake[i].x - x0) * (g->lake[i].x - x0) + (g->lake[i].y - y0) * (g->lake[i].y - y0) < 25 * 25) return 0;
    for (int cy = y0 - r - 3; cy <= y0 + r + 3; cy += 3)
        for (int cx = x0 - r - 3; cx <= x0 + r + 3; cx += 3)
            if (!mg_corner(g, cx, cy) || g->mt[cy * g->m + cx] > 0.2f || g->foot[cy * g->m + cx] > 0.45f || g->plat[cy * g->m + cx] > 0 || g->h[cy * g->m + cx] < 0.8f || mg_f(g, cx, cy) < 5) return 0;
    mg_basin(g, x0, y0, r, 2.2f);
    g->lake[g->lakes].x = x0; g->lake[g->lakes].y = y0; g->lake[g->lakes].r = r;
    g->lakes++;
    return 1;
}
// THE FORDS, once all water is dug (the player 2026-11-25: "water at -1 can be walked trough, so you can use that
// for fords" - the first ones were gravel bars over the water): at a ford the river's bed comes up to 0.4 steps under
// the water, 2 tiles up and down its way - shallows, which the game's land units walk through (map.c
// tile_is_shallows: a water tile whose lowest corner is no more than 0.75 steps down); gravel under it and on its
// banks shows where it is. None in a lake, by a pond or by the sea.
internal void mg_ford_lay(struct mg *g, int k) {
    int fx = (int)floorf(g->ford[k].x + 0.5f), fy = (int)floorf(g->ford[k].y + 0.5f);
    for (int cy = fy - 4; cy <= fy + 4; cy++)
        for (int cx = fx - 4; cx <= fx + 4; cx++) {
            float ex = (float)cx - g->ford[k].x, ey = (float)cy - g->ford[k].y, d = sqrtf(ex * ex + ey * ey);
            if (!mg_corner(g, cx, cy) || d > 3.0f) continue;
            float bed = -0.4f - 0.55f * mg_step(2.2f, 3.0f, d);
            if (g->h[cy * g->m + cx] < bed) g->h[cy * g->m + cx] = bed;
            if (mg_on(g, cx, cy) && d < 2.0f) g->used[cy * g->n + cx] = 2;
        }
}
internal void mg_fords(struct mg *g) {
    for (int k = 0; k < g->fords; k++) {
        int fx = (int)floorf(g->ford[k].x + 0.5f), fy = (int)floorf(g->ford[k].y + 0.5f);
        int off = !mg_corner(g, fx, fy) || mg_f(g, fx, fy) < 6;
        for (int i = 0; i < g->lakes; i++) off |= (g->lake[i].x - fx) * (g->lake[i].x - fx) + (g->lake[i].y - fy) * (g->lake[i].y - fy) < (g->lake[i].r + 4) * (g->lake[i].r + 4);
        for (int i = 0; i < g->oases; i++) off |= (g->oasis[i].x - fx) * (g->oasis[i].x - fx) + (g->oasis[i].y - fy) * (g->oasis[i].y - fy) < (g->oasis[i].r + 4) * (g->oasis[i].r + 4);
        if (off) g->ford[k].x = -99; else mg_ford_lay(g, k);
    }
}
// THE WAY ON FOOT, once the tiles have their kinds (the game's own: deep water, shallows, cliffs). What a land unit
// reaches from a tile, 8 ways over all but deep water and cliffs: seen[] of those tiles set to mark.
internal void mg_walk(const struct mg *g, int x0, int y0, uint8_t *seen, int *queue, uint8_t mark) {
    int n = g->n, head = 0, tail = 0;
    queue[tail++] = y0 * n + x0;
    seen[y0 * n + x0] = mark;
    while (head < tail) {
        int x = queue[head] % n, y = queue[head] / n;
        head++;
        for (int k = 0; k < 9; k++) {
            int tx = x + k % 3 - 1, ty = y + k / 3 - 1;
            int kind = mg_kind(g, tx, ty);
            if (kind == MG_K_OFF || seen[ty * n + tx] || kind == MG_K_DEEP || kind == MG_K_CLIFF) continue;
            seen[ty * n + tx] = mark;
            queue[tail++] = ty * n + tx;
        }
    }
}
// A start a walker from its continent's first does not reach (a river from a range's foot to the sea on either side
// of it, say): the way to it from what is reached (seen[] = mark) that goes through the fewest tiles of a river's
// deep water - found level by level, a level the tiles reached through as many of them -, and a ford where it
// goes through some. Only a river's water is gone through: inland, no deeper than its bed - not the sea's, a lake's
// middle, a cliff. -> the fords laid (0: no such way). queue: 2 n n, from: n n, cost: n n.
internal int mg_ford_way(struct mg *g, const uint8_t *seen, uint8_t mark, int to, int *queue, int *from, uint8_t *cost) {
    int n = g->n, *next = queue + n * n, head = 0, tail = 0, more = 0, target = g->sy[to] * n + g->sx[to], laid = 0;
    memset(cost, 255, (size_t)(n * n));
    for (int i = 0; i < n * n; i++)
        if (seen[i] == mark) { cost[i] = 0; from[i] = -1; queue[tail++] = i; }
    for (int level = 0; level < 200 && cost[target] == 255 && tail; level++) {
        for (; head < tail; head++) {
            int x = queue[head] % n, y = queue[head] / n;
            for (int k = 0; k < 9; k++) {
                int tx = x + k % 3 - 1, ty = y + k / 3 - 1, at = ty * n + tx;
                int kind = mg_kind(g, tx, ty);
                float lo, hi;
                if (kind == MG_K_OFF || cost[at] != 255 || kind == MG_K_CLIFF) continue;
                if (kind != MG_K_DEEP) { cost[at] = (uint8_t)level; from[at] = queue[head]; queue[tail++] = at; }
                else if (mg_f(g, tx, ty) > 0.5f && mg_tile_span(g, tx, ty, &lo, &hi) && lo >= -1.0f) { cost[at] = (uint8_t)(level + 1); from[at] = queue[head]; next[more++] = at; }
            }
        }
        memcpy(queue, next, (size_t)more * sizeof *queue);
        head = 0; tail = more; more = 0;
    }
    if (cost[target] == 255) return 0;
    // back along the way: a ford at the middle of each run of deep tiles
    for (int i = target, run = 0, sx = 0, sy = 0; i >= 0; i = from[i]) {
        int deep = mg_kind(g, i % n, i / n) == MG_K_DEEP;
        if (deep) { run++; sx += i % n; sy += i / n; continue; }
        if (run && g->fords < MG_FORDS) {
            g->ford[g->fords].x = (float)sx / (float)run + 0.5f; g->ford[g->fords].y = (float)sy / (float)run + 0.5f;
            mg_ford_lay(g, g->fords++);
            laid++;
        }
        run = sx = sy = 0;
    }
    return laid;
}
// The map's promises looked at: the continents apart on foot (g->afoot 0: two continents' starts, or the New World no
// start's), a continent's starts
// reached from one another (g->cut 0) - with rivers too deep to walk through they were not always (the first huge
// map had one behind a range and the two rivers from its ends). mend: where one isn't, fords laid on the way to it
// (mg_ford_way) - into the heights only: the ground is to be set again. -> the fords laid.
internal int mg_afoot(struct mg *g, int mend) {
    int n = g->n, *queue = malloc((size_t)(n * n) * 2 * sizeof *queue), *from = malloc((size_t)(n * n) * sizeof *from), laid = 0;
    uint8_t *seen = calloc((size_t)(n * n), 1), *cost = malloc((size_t)(n * n));
    g->afoot = g->cut = g->walked = g->walkable = g->fords_ok = 0;
    for (int i = 0; queue && from && seen && cost && i < g->np; i++) {
        int at = g->sy[i] * n + g->sx[i], way;
        uint8_t mark = (uint8_t)(1 + i % g->homes);
        if (seen[at] == mark) continue;
        if (seen[at]) { g->afoot = 1; continue; }
        if (i >= g->homes) {
            way = mend ? mg_ford_way(g, seen, mark, i, queue, from, cost) : 0;
            laid += way;
            g->cut += !way;
        }
        mg_walk(g, g->sx[i], g->sy[i], seen, queue, mark);
    }
    for (int i = 0; seen && i < n * n; i++) {
        int land = mg_kind(g, i % n, i / n) == MG_K_LAND;
        g->walkable += land;
        g->walked += land && seen[i];
        if (land && seen[i] && mg_side(g, i % n, i / n) >= g->homes) g->afoot = 1;   // (the New World reached on foot)
    }
    for (int k = 0; k < g->fords; k++) {
        int kind = g->ford[k].x < 0 ? MG_K_OFF : mg_kind(g, (int)g->ford[k].x, (int)g->ford[k].y);
        g->fords_ok += kind != MG_K_OFF && kind != MG_K_DEEP && kind != MG_K_CLIFF;
    }
    free(queue); free(from); free(seen); free(cost);
    return laid;
}
internal int mg_type(const char *ident) { return unit_type_named(ident, (int)strlen(ident)); }
// a tile's biome from the two layers. The distance to water first (tiles, 4 ways, the sea's and the rivers'), from every
// water tile outwards. THE RAIN SHADOW: the map has a wind (one of the four screen directions, by the seed); a mountain
// up the wind within 32 tiles dries a place (by up to 0.28 of the moisture), one down the wind within 12 wets it (0.15)
// - a range with forest on one flank and steppe on the other.
internal void mg_biomes(struct mg *g) {
    int n = g->n, *queue = malloc((size_t)(n * n) * sizeof *queue), head = 0, tail = 0;
    uint8_t *wd = calloc((size_t)(n * n), 1);
    if (!queue || !wd) { free(queue); free(wd); memset(g->clim, 4, (size_t)(n * n)); return; }   // no memory: grassland all over
    memset(wd, 255, (size_t)(n * n));
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            float lo, hi;
            if (mg_tile_span(g, x, y, &lo, &hi) && lo < 0) { wd[y * n + x] = 0; queue[tail++] = y * n + x; }
        }
    while (head < tail) {
        int i = queue[head++], x = i % n, y = i / n;
        static const int8_t side[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
        for (int s = 0; s < 4 && wd[i] < 8; s++) {
            int nx = x + side[s][0], ny = y + side[s][1];
            if (!mg_on(g, nx, ny) || wd[ny * n + nx] != 255) continue;
            wd[ny * n + nx] = (uint8_t)(wd[i] + 1);
            queue[tail++] = ny * n + nx;
        }
    }
    // the sea: the water joined to the water round the land through tiles with a corner that was water before the
    // rivers were dug (f under 0) - a river's and a pond's water is not reached, its mouth is
    uint8_t *sea = g->marsh;
    memset(sea, 0, (size_t)(n * n));   // a second pass (mapgen_make: fords laid after the first) begins anew
    int wind_x = g->seed >> 3 & 1 ? 1 : -1, wind_y = g->seed >> 4 & 1 ? 1 : -1;
    head = tail = 0;
    for (int i = 0; i < n * n; i++)
        if (!wd[i] && mg_f(g, i % n, i / n) < -3.0f) { sea[i] = 2; queue[tail++] = i; }
    while (head < tail) {
        int i = queue[head++], x = i % n, y = i / n;
        static const int8_t side[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
        for (int s = 0; s < 4; s++) {
            int nx = x + side[s][0], ny = y + side[s][1], j = ny * n + nx;
            if (!mg_on(g, nx, ny) || wd[j] || sea[j]) continue;
            if (fminf(fminf(mg_f(g, nx, ny), mg_f(g, nx + 1, ny)), fminf(mg_f(g, nx, ny + 1), mg_f(g, nx + 1, ny + 1))) >= 0) continue;
            sea[j] = 2;
            queue[tail++] = j;
        }
    }
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            float lo = 0, hi = 0;
            mg_tile_span(g, x, y, &lo, &hi);
            int bare = g->range[y * g->m + x] && g->range[y * g->m + x] < 16 && g->range_cliffs[g->range[y * g->m + x] - 1] != CLIFFS_WINTER;   // a hot range: no snow up there
            float temp = mg_temp(g, x, y) - (bare ? 0 : fmaxf(0, hi - 2.0f) * 0.07f);
            float by_water = 1 - (float)(wd[y * n + x] < 7 ? wd[y * n + x] : 7) / 7.0f;
            float wet = mg_wet0(g, x, y) + 0.22f * by_water;
            float up = 0, down = 0;
            for (int d = 4; d <= 32; d += 4) {
                int ux = x - wind_x * d * 7 / 10, uy = y - wind_y * d * 7 / 10, vx = x + wind_x * d * 7 / 10, vy = y + wind_y * d * 7 / 10;
                if (mg_corner(g, ux, uy)) up = fmaxf(up, g->mt[uy * g->m + ux] * (1 - (float)d / 36.0f));
                if (d <= 12 && mg_corner(g, vx, vy)) down = fmaxf(down, g->mt[vy * g->m + vx] * (1 - (float)d / 16.0f));
            }
            wet += 0.15f * mg_step(1.5f, 6, down) - 0.28f * mg_step(1.5f, 6, up);
            int b = (temp < -0.3f ? 0 : temp > 0.3f ? 2 : 1) * 3 + (wet < 0.4f ? 0 : wet > 0.6f ? 2 : 1);
            g->clim[y * n + x] = (uint8_t)b;
            if (sea[y * n + x] != 2) g->marsh[y * n + x] = !wd[y * n + x] && hi < 0 && temp >= -0.3f;   // inland, all under water, not frozen
        }
    free(queue); free(wd);
}
// THE TEXTURES, by their names in the game's data (db\dbterrain.dat; the scripts, Common Terrain Painting.rmv, spell
// them without the blanks): a biome's five for its land, picked by a noise value in their shares; its beach for water
// up to a step deep, Underwater below; one under mines, one under trees. Marsh is the rivers' and ponds'
// bed where they don't freeze (the player 2026-11-24, of the first maps' marsh on the land by all water: "dont use
// the marshland like that, but instead of sand used for inland rivers and lakes could look quite good, just not as an
// extra border around the beach"; then: "only use marshland underwater ... use biome appropriate texture for river
// shores") - on tiles all under water, a tile the water's edge runs through has the biome's ground. A wide beach has
// its sand on the land too, a rocky coast pebbles; a mountain's walkable ground is its biome's (up there it is
// colder: mg_biomes - a rock texture of its own beside the cliffs "comes off weird", the player 2026-11-24); cliffs
// get theirs from the game's cliff pass.
// A texture gives its tiles their kind (ee_terrain.kind: what may stand and walk there - the land textures have kinds
// of their own, Grass 0). A name the data hasn't got, or a land texture whose kind is water's or a cliff's: the
// biome's plain one, Grass for that - told in the log.
internal void mg_textures_load(struct mg *g) {
    int grass = terrain_named("Grass"), def = grass >= 0 ? grass : g->map ? map_get_default_terrain(g->map) : 0, bad = 0;
    char names[400] = "";
#define MG_TEX(to, name, land) do { \
        int t_ = terrain_named(name); \
        if (t_ < 0 || ((land) && !terrain_is_land(terrain_at((uint32_t)t_)))) { \
            if (!strstr(names, name) && strlen(names) + strlen(name) + 3 < sizeof names) { strcat(names, names[0] ? ", " : ""); strcat(names, name); } \
            bad++; t_ = -1; \
        } \
        (to) = t_; } while (0)
    for (int b = 0; b < MG_BIOMES; b++) {
        const struct mg_biome *k = &k_mg_biome[b];
        for (int i = 0; i < 5; i++) {
            MG_TEX(g->tex_land[b][i], k->land[i], 1);
            if (g->tex_land[b][i] < 0) g->tex_land[b][i] = i && g->tex_land[b][0] >= 0 ? g->tex_land[b][0] : def;
        }
        MG_TEX(g->tex_beach[b], k->beach, 0);
        MG_TEX(g->tex_res[b], k->res, 1);
        MG_TEX(g->tex_wood[b], k->wood, 1);
        g->tex_shore[b] = g->tex_beach[b] >= 0 && terrain_is_land(terrain_at((uint32_t)g->tex_beach[b])) ? g->tex_beach[b] : -1;
        if (g->tex_beach[b] < 0) g->tex_beach[b] = def;
        if (g->tex_res[b] < 0) g->tex_res[b] = g->tex_land[b][0];
        if (g->tex_wood[b] < 0) g->tex_wood[b] = g->tex_land[b][0];
        for (int i = 0; i < 6; i++) {
            int t = k->trees[i] ? mg_type(k->trees[i]) : -1, v = k->grove[i] ? mg_type(k->grove[i]) : -1;
            if (t >= 0 && type_data(t)) g->tree[b][g->ntree[b]++] = t;
            if (v >= 0 && type_data(v)) g->grove[b][g->ngrove[b]++] = v;
            g->types_missing += (k->trees[i] && (t < 0 || !type_data(t))) + (k->grove[i] && (v < 0 || !type_data(v)));
        }
        for (int i = 0; i < 4; i++) {
            int t = k->animals[i] ? mg_type(k->animals[i]) : -1;
            if (t >= 0 && type_data(t)) g->animal[b][g->nanimal[b]++] = t;
            g->types_missing += k->animals[i] && (t < 0 || !type_data(t));
        }
    }
    static const char *const wood[3][6] = { { MG_CONIFERS }, { MG_MIXED }, { MG_MAPLE } };
    for (int k = 0; k < 3; k++)
        for (int i = 0; i < 6; i++) {
            int t = wood[k][i] ? mg_type(wood[k][i]) : -1;
            if (t >= 0 && type_data(t)) g->wood[k][g->nwood[k]++] = t;
            g->types_missing += wood[k][i] && (t < 0 || !type_data(t));
        }
    MG_TEX(g->tex_deep, "Underwater", 0);
    MG_TEX(g->tex_marsh, "Marsh", 0);
    MG_TEX(g->tex_pebbles, "Pebbles - Small", 1);
    if (g->tex_deep < 0) g->tex_deep = def;
#undef MG_TEX
    if (g_log && (bad || g->types_missing))
        log_locked("# mapgen: %d ground textures not used (not in the data, or a water's or a cliff's kind of tile): %s; %d tree and animal types not in the data\n",
                   bad, names, g->types_missing);
}
// a tile's ground texture: the game's map's (its tile's kind follows the texture's), a preview's own
internal void mg_set_tex(struct mg *g, int x, int y, int tex) {
    g->tex[y * g->n + x] = (int16_t)tex;
    if (g->map) map_reset_tile(g->map, (uint32_t)x, (uint32_t)y, (uint32_t)tex);
}
// does the cliff pass's rock lie on the tile: a cliff (a step up, 1.5 steps between its corners) lays its type's
// textures on its 2 x 2 block x - 1 .. x, y - 1 .. y (mg_cliff_run), so on a tile whose block's partners right and
// below are one - by the heights, as the pass decides, or a cliff of the game's map. Those around a cliff are of
// the land's kind: free for a tree or a mine, whose ground went over the rock (the player 2026-11-26: "the ground
// texture for trees and mines overwrites cliff textures, stop that")
internal int mg_rock(const struct mg *g, int x, int y) {
    for (int j = 0; j < 4; j++) {
        float lo, hi;
        int cx = x + (j & 1), cy = y + (j >> 1);
        if (mg_tile_span(g, cx, cy, &lo, &hi) && hi >= 1.0f && hi - lo >= 1.4999f) return 1;
        if (mg_kind(g, cx, cy) == MG_K_CLIFF) return 1;
    }
    return 0;
}
// a forest's or a mine's ground on the tile - not over the cliffs' rock
internal void mg_set_floor(struct mg *g, int x, int y, int tex) {
    if (!mg_rock(g, x, y)) mg_set_tex(g, x, y, tex);
}
// a preview's tile kinds by the game's rules (spec map.kind_water, kind_shallows): under the water - fewer than 3
// corners at its level or above - deep below 0.75 steps, shallows above; the cliffs come after (mg_cliff_run)
internal void mg_kinds(struct mg *g) {
    int n = g->n;
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            float lo, hi;
            uint8_t *k = &g->kind[y * n + x];
            if (!mg_tile_span(g, x, y, &lo, &hi)) { *k = MG_K_OFF; continue; }
            const float *h = g->h + y * g->m + x;
            int dry = (h[0] >= 0) + (h[1] >= 0) + (h[g->m] >= 0) + (h[g->m + 1] >= 0);
            *k = lo < 0 && dry < 3 ? (lo < -0.75f ? MG_K_DEEP : MG_K_SHALLOWS) : MG_K_LAND;
        }
    memset(g->rock, 0, (size_t)(n * n));
}
// a preview's cliffs of a run of row y's tiles (x0 .. x1 - 1) of one cliff type, as the game's cliff pass makes them
// for that run (0x512FB0, mg_steps calls it as map_cliffs_run): first a tile whose highest corner is a step up at
// least and whose corners differ by 1.5 steps (less 0.0001) is a cliff, its 2 x 2 block (x - 1 .. x, y - 1 .. y) the
// type's textures (g->rock: the type + 1); then a tile of the run with 3 of its block off the map or of this type's
// textures laid so far is one too
internal void mg_cliff_run(struct mg *g, int x0, int x1, int y, int type) {
    int n = g->n;
    for (int x = x0; x < x1; x++) {
        float lo, hi;
        if (!mg_tile_span(g, x, y, &lo, &hi) || hi < 1.0f || hi - lo < 1.4999f) continue;
        g->kind[y * n + x] = MG_K_CLIFF;
        for (int j = 0; j < 4; j++)
            if (mg_on(g, x - (j & 1), y - (j >> 1))) g->rock[(y - (j >> 1)) * n + x - (j & 1)] = (uint8_t)(type + 1);
    }
    for (int x = x0; x < x1; x++) {
        int block = 0, k = g->kind[y * n + x];
        if (k == MG_K_OFF || k == MG_K_CLIFF) continue;
        for (int j = 0; j < 4; j++) {
            int bx = x - (j & 1), by = y - (j >> 1);
            block += !mg_on(g, bx, by) || g->rock[by * n + bx] == type + 1;
        }
        if (block >= 3) g->kind[y * n + x] = MG_K_CLIFF;
    }
}
internal void mg_textures(struct mg *g) {
    for (int y = 0; y < g->n; y++)
        for (int x = 0; x < g->n; x++) {
            float lo, hi, beach, width, bluff, rocky, f = mg_f(g, x, y);
            if (!mg_tile_span(g, x, y, &lo, &hi)) continue;
            int b = g->clim[y * g->n + x], sort = g->marsh[y * g->n + x];
            int p = (int)(mg_fbm((float)x * 0.11f + 300, (float)y * 0.11f + 700, g->seed + 30) * 160.0f - 30.0f), k = 0;   // 0 .. 100, evened out
            p = p < 0 ? 0 : p > 99 ? 99 : p;
            while (k < 4 && p >= k_mg_biome[b].share[k]) p -= k_mg_biome[b].share[k++];
            int tex = g->tex_land[b][k];
            mg_coast(g, x, y, &beach, &width, &bluff, &rocky);
            if (lo < 0) {
                // the sea's: beach, Underwater only below 3 steps (the player: shallower it looked wrong, beach sand
                // there); a river's and a lake's: marsh down to 1 step, its shore the biome's ground, frozen its ice
                if (g->used[y * g->n + x] == 2 && sort != 2 && b >= 3 && g->tex_pebbles >= 0) tex = g->tex_pebbles;   // a ford
                else if (lo < -3.0f) tex = g->tex_deep;
                else if (sort == 1 && lo > -1.0f && g->tex_marsh >= 0) tex = g->tex_marsh;
                else if (sort == 2 || (sort == 1 && g->tex_marsh >= 0)) tex = g->tex_beach[b];   // a lake's middle the beach's sand
                else if (hi < 0) tex = g->tex_beach[b];
                g->water++;
            } else {
                // a wide beach's sand on the land, a rocky coast's pebbles
                if (g->used[y * g->n + x] == 2 && b >= 3 && g->tex_pebbles >= 0) tex = g->tex_pebbles;   // a ford's bank
                else if (beach > 0.5f && f < width && g->tex_shore[b] >= 0) tex = g->tex_shore[b];
                else if (rocky > 0.5f && f < 2.5f && b >= 3 && g->tex_pebbles >= 0) tex = g->tex_pebbles;
                g->land++;
                g->biome_tiles[b]++;
                g->mountain += g->mt[y * g->m + x] > 2.0f;
            }
            mg_set_tex(g, x, y, tex);
        }
}

// THE WORLD'S OBJECTS. A tile is free for one when it is land of the plain (its corners 0.3 steps up at least and
// within 0.8 of each other), no cliff, not taken and not up a mountain (its foot only: what the plain reaches).
internal int mg_free(const struct mg *g, int x, int y) {
    float lo, hi;
    return mg_tile_span(g, x, y, &lo, &hi) && !g->used[y * g->n + x] && lo >= 0.3f && hi - lo < 0.8f && g->mt[y * g->m + x] < 1.4f && mg_kind(g, x, y) != MG_K_CLIFF;
}
// an object of the world's: the game's unit, a preview's mark on its tile -> 1 made
internal int mg_object(struct mg *g, int type, int x, int y, float angle, int kind) {
    ee_unit_type *t = type >= 0 ? type_data(type) : NULL;
    int made = !t ? 0 : g->map ? unit_type_create_on(t, g->world, x, y, g->map, angle) != NULL : 1;
    if (made) g->obj[y * g->n + x] = (uint8_t)(kind + 1);
    g->made[kind] += made;
    return made;
}
internal int mg_spot_far(const struct mg *g, int x, int y, int apart) {
    for (int i = 0; i < g->nspot; i++) {
        int dx = g->spot[i].x - x, dy = g->spot[i].y - y;
        if (dx * dx + dy * dy < apart * apart) return 0;
    }
    return 1;
}
internal void mg_spot_add(struct mg *g, int x, int y) {
    if (g->nspot < MG_SPOTS) { g->spot[g->nspot].x = x; g->spot[g->nspot].y = y; g->nspot++; }
}
// a free tile lo .. hi tiles from (cx, cy), `apart` from every spot so far: a random point of the ring, 80 tries
internal int mg_ring(struct mg *g, int cx, int cy, int lo, int hi, int apart, int *ox, int *oy) {
    for (int t = 0; t < 80; t++) {
        int dx = mg_int(g, -hi, hi), dy = mg_int(g, -hi, hi), d2 = dx * dx + dy * dy;
        if (d2 < lo * lo || d2 > hi * hi || !mg_free(g, cx + dx, cy + dy) || !mg_spot_far(g, cx + dx, cy + dy, apart)) continue;
        *ox = cx + dx; *oy = cy + dy;
        return 1;
    }
    return 0;
}
// a free tile anywhere, `away` from every start and `apart` from every spot: 200 tries
internal int mg_anywhere(struct mg *g, int away, int apart, int *ox, int *oy) {
    for (int t = 0; t < 200; t++) {
        int x = mg_int(g, 0, g->n - 1), y = mg_int(g, 0, g->n - 1);
        if (!mg_free(g, x, y) || mg_start_dist(g, (float)x, (float)y) < (float)away || !mg_spot_far(g, x, y, apart)) continue;
        *ox = x; *oy = y;
        return 1;
    }
    return 0;
}
// a mine or a forage patch: the object; the biome's ground for one only on its own tile - the 4 corners right under
// it (the player 2026-11-25: on the four tiles beside it too it was a patch round it) -, those beside it kept free
internal int mg_mine(struct mg *g, int kind, int x, int y) {
    static const char *const ident[4] = { "res_gold_mine", "res_iron", "res_stone_mine", "res_forage_bush" };
    static const int8_t side[5][2] = { { 0, 0 }, { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    if (!mg_object(g, mg_type(ident[kind]), x, y, 0, kind)) return 0;
    int b = g->clim[y * g->n + x], tex = kind == MG_FORAGE ? g->tex_wood[b] : g->tex_res[b];
    for (int k = 0; k < 5; k++) {
        int tx = x + side[k][0], ty = y + side[k][1];
        if (!mg_free(g, tx, ty)) continue;
        if (!k) mg_set_floor(g, tx, ty, tex);
        g->used[ty * g->n + tx] = 1;
    }
    mg_spot_add(g, x, y);
    return 1;
}
// A CLUMP of mines (the player 2026-11-25: "a low chance of 2 or even more near each other instead of them evenly
// spread over the map"): a place worth a Settlement, walls and a fight. After a mine at x, y, with `chance` in 100,
// 1 more (6 times in 10), 2 (3) or 3 (1) 4 - 6 tiles from it and from each other - room for six miners each and a
// wall round them all -, a gold mine's mostly iron or stone, an iron's or a stone's mostly gold, else its own kind
// (a district of mixed ores is more to hold than a double). within > 0: no further than that from cx, cy (a
// plateau's top). Not within 10 tiles of a start (as the land's other objects). Returns the
// mines laid beside it; each counts against the map's mines, not on top of them (mg_objects).
internal int mg_clump(struct mg *g, int kind, int x, int y, int chance, int cx, int cy, int within, int laid[3]) {
    static const int8_t other[3][2] = { { MG_IRON, MG_STONE }, { MG_GOLD, MG_GOLD }, { MG_GOLD, MG_IRON } };
    if (mg_int(g, 0, 99) >= chance || mg_start_dist(g, (float)x, (float)y) < 10) return 0;
    int r = mg_int(g, 0, 9), extra = r < 6 ? 1 : r < 9 ? 2 : 3, n = 0;
    for (int i = 0; i < extra; i++) {
        int k = mg_int(g, 0, 9) < 6 ? other[kind][mg_int(g, 0, 1)] : kind, ex, ey;
        if (!mg_ring(g, x, y, 4, 6, 4, &ex, &ey)) break;
        if ((within > 0 && (ex - cx) * (ex - cx) + (ey - cy) * (ey - cy) > within * within) || mg_start_dist(g, (float)ex, (float)ey) < 10) continue;
        if (mg_mine(g, k, ex, ey)) { laid[k]++; n++; }
    }
    if (n) { g->clumps++; g->clump_mines += n + 1; }    return n;
}
// a forest: free tiles grown from a seed tile, a neighbour of a tile it has at a time - as many as the seed's biome
// says; a tree of the biome's on 9 in 10 of them, its forest floor under all. grove: a start's own - the biome's
// timber trees where its own are none, 28 - 50 tiles. In the mild green biomes (by_warmth) a forest is all spruce,
// mixed or all maple by the warmth at its seed (the player 2026-11-25: "the pure variants also should exist,
// especially towards cold pure spruce"): from the mild band's cold edge to its warm one, give or take a quarter -
// at the cold edge spruce only, in the middle mixed three times in five, at the warm edge maple only.
internal void mg_forest(struct mg *g, int x, int y, int grove) {
    static const int8_t side[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    int b = g->clim[y * g->n + x], other = grove && g->ngrove[b], kinds = other ? g->ngrove[b] : g->ntree[b];
    const int *trees = other ? g->grove[b] : g->tree[b];
    if (k_mg_biome[b].by_warmth && !other) {
        float lo, hi, warm;
        mg_tile_span(g, x, y, &lo, &hi);
        warm = (mg_temp(g, x, y) - fmaxf(0, hi - 2.0f) * 0.07f + 0.3f) / 0.6f + (float)k_mg_biome[b].lean / 100.0f + (mg_rnd(g) - 0.5f) * 0.5f;
        int k = warm < 0.35f ? 0 : warm > 0.65f ? 2 : 1;
        if (g->nwood[k]) { trees = g->wood[k]; kinds = g->nwood[k]; g->wood_made[k]++; }    }
    if (!kinds) return;
    int want = grove ? mg_int(g, 28, 50) : mg_int(g, k_mg_biome[b].size_lo, k_mg_biome[b].size_hi), n = 1, tx[60], ty[60];
    tx[0] = x; ty[0] = y;
    g->used[y * g->n + x] = 1;
    for (int t = 0; n < want && t < want * 8; t++) {
        int from = mg_int(g, 0, n - 1), s = mg_int(g, 0, 3), nx = tx[from] + side[s][0], ny = ty[from] + side[s][1];
        if (!mg_free(g, nx, ny)) continue;
        g->used[ny * g->n + nx] = 1;
        tx[n] = nx; ty[n] = ny; n++;
    }
    for (int i = 0; i < n; i++) {
        mg_set_floor(g, tx[i], ty[i], g->tex_wood[b]);
        if (mg_int(g, 0, 9)) mg_object(g, trees[mg_int(g, 0, kinds - 1)], tx[i], ty[i], mg_rnd(g) * 6.2831853f, MG_TREE);
    }
    g->forests++;
    mg_spot_add(g, x, y);
}
// a herd: 3 - 6 animals of one kind of the biome on free tiles within 2 of a spot (they walk: the tiles stay free)
internal void mg_herd(struct mg *g, int x, int y) {
    int b = g->clim[y * g->n + x];
    if (!g->nanimal[b]) return;
    int type = g->animal[b][mg_int(g, 0, g->nanimal[b] - 1)], want = mg_int(g, 3, 6);
    for (int t = 0, n = 0; n < want && t < 30; t++) {
        int ax = x + mg_int(g, -2, 2), ay = y + mg_int(g, -2, 2);
        if (mg_free(g, ax, ay) && mg_object(g, type, ax, ay, mg_rnd(g) * 6.2831853f, MG_ANIMAL)) n++;
    }
    mg_spot_add(g, x, y);
}
internal int mg_rich(struct mg *g, int x, int y, int what) { return mg_int(g, 0, 99) < k_mg_biome[g->clim[y * g->n + x]].rich[what]; }
// the objects of a kind within r tiles of (cx, cy)
internal int mg_near(const struct mg *g, int cx, int cy, int r, int kind) {
    int c = 0;
    for (int y = cy - r; y <= cy + r; y++)
        for (int x = cx - r; x <= cx + r; x++)
            c += x >= 0 && y >= 0 && x < g->n && y < g->n && (x - cx) * (x - cx) + (y - cy) * (y - cy) <= r * r && g->obj[y * g->n + x] == kind + 1;
    return c;
}
// The land's objects - places drawn by its size, each laid as the biome there gives it, none within 10 tiles of a
// start (its yard and the ring round it its houses and farms take: forests 8 tiles out walled a start in) -, then
// each start's own topped up (the player 2026-11-26: "adding the extra resource patches if needed"): what the land
// already has near it counts. A start has at least 2 forage patches within 12 tiles, 2 gold, 2 iron and 2 stone
// mines within 20 (its own laid 13 - 20 out: far enough to want a Settlement), 30 trees within 16 (else a grove of
// timber, whatever its biome) and two more forests as the biome gives them where it has fewer than 60, a herd within
// 18 (3 animals). A start on rich land gets less of its own; the map is not even - the player 2026-11-25 on
// fairness: "this might lean more in a 4x flavour of the game". (With one forage patch where the biome gave no
// second, a tundra start stood at 23 people after 38 minutes, the others at 91 - 168: the first epoch has no farms,
// its food is forage, game and fish - so the forage is never less than 2.)
internal void mg_starts_own(struct mg *g) {
    static const struct { int8_t kind, n, lo, hi, within; } own[4] = { { MG_FORAGE, 2, 8, 11, 12 }, { MG_GOLD, 2, 13, 16, 20 }, { MG_IRON, 2, 14, 18, 20 }, { MG_STONE, 2, 15, 20, 20 } };
    int x, y;
    for (int i = 0; i < g->np; i++) {
        int sx = g->sx[i], sy = g->sy[i];
        for (int k = 0; k < 4; k++)
            for (int j = mg_near(g, sx, sy, own[k].within, own[k].kind); j < own[k].n; j++)
                if (mg_ring(g, sx, sy, own[k].lo, own[k].hi, 5, &x, &y) && mg_mine(g, own[k].kind, x, y)) g->topped[k]++;
        int trees = mg_near(g, sx, sy, 16, MG_TREE);
        for (int j = 0; j < 3; j++) {
            if (j ? trees >= 60 : trees >= 30) continue;
            if (mg_ring(g, sx, sy, 10, 15, 6, &x, &y) && (!j || mg_int(g, 0, 99) < 42000 / k_mg_biome[g->clim[y * g->n + x]].forest_tiles)) {
                mg_forest(g, x, y, !j);
                g->topped[4]++;
                trees = mg_near(g, sx, sy, 16, MG_TREE);
            }
        }
        if (mg_near(g, sx, sy, 18, MG_ANIMAL) < 3 && mg_ring(g, sx, sy, 11, 16, 6, &x, &y)) { mg_herd(g, x, y); g->topped[5]++; }
    }
}
internal void mg_objects(struct mg *g) {
    int x, y;
    // the land's mines and forage (the player 2026-11-25: a quarter of the mines of before - a gold or an iron mine
    // per 2600 land tiles, a stone mine per 4000 - "would still be plenty"; a start's own stay): drawn by turns
    // (gold, iron, stone, forage, gold ...), so that a clump's mines of another kind are taken off that kind's draws
    // still to come (owed) - clumps leave the map no richer
    static const struct { int8_t kind; int16_t tiles; } wild[4] = { { MG_GOLD, 2600 }, { MG_IRON, 2600 }, { MG_STONE, 4000 }, { MG_FORAGE, 1100 } };
    int draws[4], owed[3] = { 0, 0, 0 }, most = 0;
    for (int k = 0; k < 4; k++) { draws[k] = mg_amount(g->land / wild[k].tiles, wild[k].kind == MG_FORAGE ? 1.0f : g->opt.mines); most = draws[k] > most ? draws[k] : most; }
    for (int j = 0; j < most; j++)
        for (int k = 0; k < 4; k++) {
            int kind = wild[k].kind;
            if (j >= draws[k] || !mg_anywhere(g, 10, 9, &x, &y) || !mg_rich(g, x, y, kind)) continue;
            if (kind != MG_FORAGE && owed[kind] > 0) { owed[kind]--; continue; }   // one that would have been laid
            if (mg_mine(g, kind, x, y) && kind != MG_FORAGE) mg_clump(g, kind, x, y, 18, 0, 0, 0, owed);
        }
    for (int j = mg_amount(g->land / 150, g->opt.forests); j > 0; j--)
        if (mg_anywhere(g, 10, 6, &x, &y) && mg_int(g, 0, 999) < 150000 / k_mg_biome[g->clim[y * g->n + x]].forest_tiles) mg_forest(g, x, y, 0);
    for (int j = g->land / 600; j > 0; j--)
        if (mg_anywhere(g, 10, 8, &x, &y) && mg_rich(g, x, y, MG_RICH_HERD)) mg_herd(g, x, y);
    // THE NEW WORLD's riches (old_new_world): on it one and a half times again the mines its land's share of the
    // draws gave it - each laid, not only drawn (its biome's richness still says yes or no to a place), a clump twice
    // as often: veins worth a Settlement - and half again of the forests. The Old World has its players' own (8 of
    // each start's: mg_starts_own); the New one is to be worth the crossing and the fight for it
    if (g->nc > g->homes) {
        int land_new = 0, spare_new[3] = { 0, 0, 0 };
        for (int i = 0; i < g->n * g->n; i++) land_new += mg_kind(g, i % g->n, i / g->n) == MG_K_LAND && mg_side(g, i % g->n, i / g->n) >= g->homes;
        for (int k = 0; k < 3; k++)
            for (int j = mg_amount((land_new * 3 / 2 + wild[k].tiles / 2) / wild[k].tiles, g->opt.mines), t = 0; j > 0 && t < 600; t++) {   // (rounded: a small map's New World gets some)
                if (!mg_anywhere(g, 10, 9, &x, &y) || mg_side(g, x, y) < g->homes || !mg_rich(g, x, y, wild[k].kind) || !mg_mine(g, wild[k].kind, x, y)) continue;
                j--;
                g->new_mines += 1 + mg_clump(g, wild[k].kind, x, y, 36, 0, 0, 0, spare_new);
            }
        for (int j = mg_amount(land_new / 300, g->opt.forests), t = 0; j > 0 && t < 2000; t++) {
            if (!mg_anywhere(g, 10, 6, &x, &y) || mg_side(g, x, y) < g->homes) continue;
            j--;
            if (mg_int(g, 0, 999) < 150000 / k_mg_biome[g->clim[y * g->n + x]].forest_tiles) { mg_forest(g, x, y, 0); g->new_forests++; }
        }
        if (g->map && g_log) log_locked("# mapgen: the New World's own - %d land tiles, %d mines and %d forests beyond the land's\n", land_new, g->new_mines, g->new_forests);
    }
    // an oasis' own: two stands of the biome's trees (the deserts' and the savanna's are palms) and a forage patch by
    // its pond; a swamp's: a tree of the biome's on a quarter of its low ground, the rest of that kept free of mines
    for (int k = 0; k < g->oases; k++) {
        for (int j = 0; j < 2; j++)
            if (mg_ring(g, g->oasis[k].x, g->oasis[k].y, g->oasis[k].r + 2, g->oasis[k].r + 5, 3, &x, &y)) mg_forest(g, x, y, 0);
        if (mg_ring(g, g->oasis[k].x, g->oasis[k].y, g->oasis[k].r + 2, g->oasis[k].r + 6, 3, &x, &y)) mg_mine(g, MG_FORAGE, x, y);
    }
    for (int k = 0; k < g->swamps; k++)
        for (y = g->swamp[k].y - g->swamp[k].r; y <= g->swamp[k].y + g->swamp[k].r; y++)
            for (x = g->swamp[k].x - g->swamp[k].r; x <= g->swamp[k].x + g->swamp[k].r; x++) {
                float lo, hi;
                int dx = x - g->swamp[k].x, dy = y - g->swamp[k].y, b;
                if (dx * dx + dy * dy > (g->swamp[k].r - 1) * (g->swamp[k].r - 1) || !mg_tile_span(g, x, y, &lo, &hi) || lo < 0.02f || hi > 1.0f || g->used[y * g->n + x]) continue;
                g->used[y * g->n + x] = 1;
                b = g->clim[y * g->n + x];
                if (g->ntree[b] && mg_int(g, 0, 3) == 0) {
                    mg_set_floor(g, x, y, g->tex_wood[b]);
                    mg_object(g, g->tree[b][mg_int(g, 0, g->ntree[b] - 1)], x, y, mg_rnd(g) * 6.2831853f, MG_TREE);
                }
            }
    // a plateau's own: a gold mine within half its radius of its middle - a reason to hold it -, a clump every other
    // time, on its top (2 tiles in from its cliffs); those are a plateau's extra, not taken off the land's
    int spare[3] = { 0, 0, 0 };
    for (int k = 0; k < g->plateaus && g->opt.mines > 0; k++)
        if (mg_ring(g, g->plateau[k].x, g->plateau[k].y, 0, g->plateau[k].r / 2, 5, &x, &y) && mg_mine(g, MG_GOLD, x, y))
            mg_clump(g, MG_GOLD, x, y, 50, g->plateau[k].x, g->plateau[k].y, g->plateau[k].r - 2, spare);
    // the mountains' own (the player 2026-11-24: "increased chance for mine in mountains"): a gold, iron or stone
    // mine by turns per 360 tiles of the ranges' feet (their lowest 1.4 steps), whatever the biome; a clump 3 times
    // in 10 (a vein where the rock comes up), its mines counted among those of the feet
    int foot = 0, turn = 0;
    for (int ty = 0; ty < g->n; ty++)
        for (int tx = 0; tx < g->n; tx++) foot += g->mt[ty * g->m + tx] > 0.3f && g->mt[ty * g->m + tx] < 1.4f;
    for (int j = mg_amount(foot / 360, g->opt.mines), t = 0; j > 0 && t < 40000; t++) {
        static const int8_t kinds[3] = { MG_GOLD, MG_IRON, MG_STONE };
        x = mg_int(g, 0, g->n - 1); y = mg_int(g, 0, g->n - 1);
        if (g->mt[y * g->m + x] < 0.3f || !mg_free(g, x, y) || !mg_spot_far(g, x, y, 7)) continue;
        int kind = kinds[turn++ % 3], more;
        if (!mg_mine(g, kind, x, y)) continue;
        more = mg_clump(g, kind, x, y, 30, 0, 0, 0, spare);
        g->mountain_mines += 1 + more;
        j -= 1 + more;
    }
    // fish: in water a step deep at least, 2 - 7 tiles off a shore, 5 tiles apart - 5 a player and one per 1500 tiles of the map
    int fish = mg_type("res_fish"), first = g->nspot;
    for (int j = 5 * g->np + g->n * g->n / 1500, t = 0; j > 0 && t < 20000; t++) {
        float lo, hi, f;
        x = mg_int(g, 0, g->n - 1); y = mg_int(g, 0, g->n - 1);
        if (!mg_tile_span(g, x, y, &lo, &hi) || hi > -1.0f || (f = mg_f(g, x, y)) > -2 || f < -7) continue;
        int clear = 1;
        for (int i = first; clear && i < g->nspot; i++) clear = (g->spot[i].x - x) * (g->spot[i].x - x) + (g->spot[i].y - y) * (g->spot[i].y - y) >= 25;
        if (!clear || !mg_object(g, fish, x, y, 0, MG_FISH)) continue;
        mg_spot_add(g, x, y);
        j--;
    }
    for (int k = 0; k < g->lakes; k++) {   // a lake's: one in its middle, a second in one of 7 tiles' radius and more
        float lo, hi;
        for (int j = 0; j < (g->lake[k].r >= 7 ? 2 : 1); j++) {
            x = g->lake[k].x + (j ? 2 : -1); y = g->lake[k].y + (j ? 1 : -1);
            if (mg_tile_span(g, x, y, &lo, &hi) && hi <= -1.0f) mg_object(g, fish, x, y, 0, MG_FISH);
        }
    }
}

// THE GROUND'S SCATTER: stones, plants and the water's own - the game's maps have them (a ground texture of
// db\dbterrain.dat carries a list of unit types and how many of 100 tiles get one: "Beach" 15 of seagrass and
// seaweed, "RM Ambients - Forest" 1 of ferns, bamboo, bushes ...; its map maker's fill 0x66EA64 lays them - the mod's
// maps, textured tile by tile, got none: bare land, the player's list 2026-11-25). The mod's own sets, by what a
// tile is - the first that fits; of 1000 such tiles `rate` get an object of the set, a land set's in patches (0.3 to
// 2.5 times the rate by a noise: meadows of flowers, not confetti). They are the world's objects as the game's
// are: no obstacle, built over.
enum { MG_SC_LILIES, MG_SC_REEDS, MG_SC_SEA, MG_SC_REEF, MG_SC_FLOES, MG_SC_BERGS, MG_SC_STRAND, MG_SC_SCREE, MG_SC_BIOME, MG_SCATTERS = MG_SC_BIOME + MG_BIOMES };
#define MG_ROCKS "amb_rock_3", "amb_rock_4", "amb_rock_5", "amb_rock_6", "amb_rock_7", "amb_rock_8", "amb_rock_9", "amb_rock_10"
#define MG_FLOWERS "amb_flowers01_roses", "amb_flowers02_daisies", "amb_flowers03_daffodils", "amb_flowers04_thistles", "amb_flowers05_violets", "amb_flowers06_violets_2"
internal const struct mg_scatter_set {
    const char *what;
    int16_t rate;
    const char *ident[16];
} k_mg_scatter[MG_SCATTERS] = {
    { "a river's or a lake's water that isn't frozen", 70, { "amb_lillypads1", "amb_lillypads2", "amb_lillypads3", "amb_lillypads4", "amb_lillypads5" } },
    { "its bank", 160, { "amb_cattails_1", "amb_cattails_2", "amb_cattails_1", "amb_tall_grass" } },
    { "the sea up to a step deep", 60, { "amb_seagrass1", "amb_seagrass2", "amb_seagrass3", "amb_seastalks1", "amb_seastalks2", "amb_seastalks3", "amb_seaweed1", "amb_seaweed2", "amb_seaweed3", "amb_seaweed4", "amb_seaweed5" } },
    { "the same in the hot south", 90, { "amb_coral_1", "amb_coral_2", "amb_coral_fan", "amb_seagrass1", "amb_seastalks1", "amb_seaweed1", "amb_seaweed3", "amb_starfish2" } },
    { "the cold north's sea up to a step deep", 25, { "amb_icefloat1", "amb_icefloat2", "amb_icefloat3", "amb_icefloat4" } },
    { "the cold north's deeper sea", 5, { "amb_iceberg1", "amb_iceberg2", "amb_iceberg3", "amb_iceberg4", "amb_icefloat2", "amb_icefloat4" } },
    { "a wide beach's sand", 25, { "amb_clam_1", "amb_clam_2", "amb_starfish_01", "amb_starfish2", "amb_starfish3", "amb_giant_clam", "amb_clam_1", "amb_starfish3" } },
    { "a range's foot", 55, { MG_ROCKS, "amb_rock_granite", "amb_rock_large", "amb_rock_outcrop_1", "amb_rock_outcrop_2", "amb_rock_outcrop_3" } },
    { "ice desert", 8, { MG_ROCKS } },
    { "tundra", 14, { MG_ROCKS, "amb_log", "amb_tree_stump_1" } },
    { "taiga", 24, { "amb_log", "amb_tree_stump_1", "amb_tree_stump_2", "amb_tree_stump_3", "amb_tree_stump_4", "amb_rock_3", "amb_rock_5", "amb_rock_7" } },
    { "steppe", 26, { "amb_tall_grass", "amb_tumbleweed", "amb_crab_grass", "amb_rock_4", "amb_rock_6", "amb_rock_8", "amb_cowskull", "amb_bush", "amb_clump1", "amb_clump2" } },
    { "grassland", 45, { MG_FLOWERS, "amb_tall_grass", "amb_bush", "amb_clump1", "amb_clump2", "amb_crab_grass", "amb_rock_3", "amb_log" } },
    { "woodland", 60, { "amb_fern_1", "amb_fern_2", "amb_fern_patch", "amb_bush", "amb_log", "amb_tall_grass", "amb_tree_stump_1", "amb_tree_stump_3", "amb_flowers05_violets", "amb_flowers02_daisies", "amb_flowers04_thistles" } },
    { "desert", 20, { MG_ROCKS, "amb_rock_granite", "amb_rock_outcrop_3", "amb_cowskull", "amb_dinosaur_bones", "amb_aloe", "amb_tumbleweed", "amb_skeleton" } },
    { "savanna", 32, { "amb_tall_grass", "amb_aloe", "amb_bush", "amb_crab_grass", "amb_clump1", "amb_rock_5", "amb_rock_9", "amb_cowskull", "amb_tumbleweed" } },
    { "tropics", 70, { "amb_fern_1", "amb_fern_2", "amb_fern_patch", "amb_bamboo", "amb_bamboo", "amb_tall_grass", "amb_bush", "amb_flowers01_roses", "amb_flowers03_daffodils", "amb_log" } },
};
internal void mg_scatter(struct mg *g) {
    static const int8_t side[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    int type[MG_SCATTERS][16], kinds[MG_SCATTERS], n = g->n;
    for (int k = 0; k < MG_SCATTERS; k++) {
        kinds[k] = 0;
        for (int i = 0; i < 16 && k_mg_scatter[k].ident[i]; i++) {
            int t = mg_type(k_mg_scatter[k].ident[i]);
            if (t >= 0 && type_data(t)) type[k][kinds[k]++] = t; else g->types_missing++;
        }
    }
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            float lo, hi, beach, width, bluff, rocky;
            if (!mg_tile_span(g, x, y, &lo, &hi) || mg_kind(g, x, y) == MG_K_CLIFF) continue;
            int b = g->clim[y * n + x], water = g->marsh[y * n + x], set = -1, patchy = 0;
            if (lo < 0) {
                if (water == 1) set = lo > -1.0f && g->used[y * n + x] != 2 ? MG_SC_LILIES : -1;   // none on a ford
                else if (water == 2 && hi < 0) set = b < 3 ? (lo > -1.0f ? MG_SC_FLOES : MG_SC_BERGS) : lo > -1.0f ? (b >= 6 ? MG_SC_REEF : MG_SC_SEA) : -1;
            } else if (g->used[y * n + x] == 1) {
                // a bank: land beside a river's or a lake's open water
                for (int k = 0; k < 4 && set < 0; k++)
                    if (mg_on(g, x + side[k][0], y + side[k][1]) && g->marsh[(y + side[k][1]) * n + x + side[k][0]] == 1) set = MG_SC_REEDS;
                if (set >= 0 && mg_start_dist(g, (float)x, (float)y) < 9) set = -1;   // not in a start's yard
            } else if (!g->used[y * n + x] && lo >= 0.05f) {
                mg_coast(g, x, y, &beach, &width, &bluff, &rocky);
                if (beach > 0.5f && mg_f(g, x, y) < width && g->tex_shore[b] >= 0) set = MG_SC_STRAND;
                else if (g->mt[y * g->m + x] > 0.3f) set = g->mt[y * g->m + x] < 4.0f ? MG_SC_SCREE : -1;
                else { set = MG_SC_BIOME + b; patchy = 1; }
            }
            if (set < 0 || !kinds[set]) continue;
            float rate = (float)k_mg_scatter[set].rate;
            if (patchy) rate *= 0.3f + 2.2f * mg_step(0.48f, 0.66f, mg_fbm((float)x * 0.09f + 70, (float)y * 0.09f + 20, g->seed + 97));
            if (mg_rnd(g) * 1000.0f < rate) mg_object(g, type[set][mg_int(g, 0, kinds[set] - 1)], x, y, mg_rnd(g) * 6.2831853f, MG_AMBIENT);
        }
}

// a start unit as the game's random map makes one: the type's unit at a point of the map, built, paid for
internal ee_unit *mapgen_unit(int type, ee_player *pl, float x, float y, ee_map *map) {
    ee_unit_type *t = ut_has(type) ? type_data(type) : NULL;
    ee_unit *u = t ? unit_type_create_at(t, pl, x, y, map, 0.0f) : NULL;
    ee_unit *ph = u ? unit_get_physics(u) : NULL;
    if (!ph) return u;
    unit_set_built(ph, 1, 0, 0);
    unit_set_paid_for(ph, 1);
    return u;
}

// THE GENERATOR for a map of n tiles a side and np players: its state and arrays (map: the game's new map, NULL a
// preview); NULL: no memory
internal void mg_delete(struct mg *g) {
    if (!g) return;
    free(g->f); free(g->h); free(g->mt); free(g->range); free(g->foot); free(g->plat); free(g->fjord); free(g->used); free(g->clim);
    free(g->marsh); free(g->kind); free(g->tex); free(g->obj); free(g->rock); free(g->z); free(g);
}
internal struct mg *mg_new(ee_map *map, int n, int np, uint32_t seed, const ee_setup *setup) {
    struct mg *g = n > 0 && np > 0 ? calloc(1, sizeof *g) : NULL;
    if (!g) return NULL;
    g->map = map; g->n = n; g->m = n + 1; g->half = (float)n * 0.5f;
    g->seed = seed; g->rng = seed * 2654435761u | 1;
    g->np = np > MG_PLAYERS ? MG_PLAYERS : np;
    mg_options(g, setup);
    size_t c = (size_t)(g->m * g->m), t = (size_t)(n * n);
    g->f = calloc(c, sizeof *g->f); g->h = calloc(c, sizeof *g->h); g->mt = calloc(c, sizeof *g->mt); g->range = calloc(c, 1);
    g->foot = calloc(c, sizeof *g->foot); g->plat = calloc(c, sizeof *g->plat); g->fjord = calloc(c, sizeof *g->fjord);
    g->used = calloc(t, 1); g->clim = calloc(t, 1); g->marsh = calloc(t, 1); g->tex = calloc(t, sizeof *g->tex);
    g->kind = map ? NULL : calloc(t, 1); g->obj = calloc(t, 1); g->rock = map ? NULL : calloc(t, 1); g->z = map ? calloc(c, sizeof *g->z) : NULL;
    if (!g->f || !g->h || !g->mt || !g->range || !g->foot || !g->plat || !g->fjord || !g->used || !g->clim || !g->marsh || !g->tex || !g->obj
        || (map ? !g->z : !g->kind || !g->rock)) { mg_delete(g); return NULL; }
    return g;
}

// THE MAP MADE, step by step - but for the start units: into the game's map, or a preview's into the generator's own
// arrays by the same steps and draws (the map OK will make), without the ground's scatter (last of all: nothing
// after it to keep the same)
internal void mg_steps(struct mg *g) {
    int n = g->n;
    if (g->map) mg_options_log(g);
    mg_land(g);
    // the ranges: 2 + n/100, on a small map fewer (1 + n^2/12000: 2 on 116 - 150 tiles - three took the middle of
    // its continents, once the starts no longer kept them off)
    int ranges = 2 + n / 100 < 1 + n * n / 12000 ? 2 + n / 100 : 1 + n * n / 12000;
    for (int want = mg_amount(ranges, g->opt.mountains), t = 0; g->ranges < want && t < 40000; t++) g->ranges += mg_range(g, mg_feature_on(g, g->ranges));
    for (int want = mg_amount(1 + n / 120, g->opt.mountains), t = 0; g->plateaus < want && t < 40000; t++) mg_plateau(g, mg_feature_on(g, g->plateaus), t / 10000);
    mg_heights(g);
    for (int want = mg_amount(1 + n / 110, g->opt.lakes), t = 0; g->lakes < want && t < 20000; t++) mg_lake(g, t / 5000);
    for (int want = 1 + n / 150, t = 0; g->oases < want && t < 6000; t++) mg_oasis(g);
    for (int want = 1 + n / 250, t = 0; g->swamps < want && t < 24000; t++) mg_swamp(g, t / 6000);
    for (int k = 0; k < g->lakes && g->opt.rivers > 0; k++)   // a lake's outflow
        for (int t = 0, was = g->rivers; t < 40 && g->rivers == was; t++) g->rivers += mg_river(g, 0, g->lake[k].x, g->lake[k].y);
    for (int want = g->rivers + mg_amount(2 + n / 60, g->opt.rivers), t = 0; g->rivers < want && t < 6000; t++) g->rivers += mg_river(g, t < 3000, -1, -1);   // from the mountains while such are found
    mg_fords(g);
    float step = g->map ? map_get_height_step(g->map) : 1;
    mg_textures_load(g);
    // the ground, its textures and cliffs - and, once the game has given the tiles their kinds, the starts placed on
    // it and their yards levelled (the ground made again), then the way on foot between a continent's starts: where
    // one is cut off, fords are laid for it and the ground is made again
    for (int mends = 0; ; ) {
        for (int i = 0; i < g->m * g->m; i++) {
            if (g->h[i] > -0.05f && g->h[i] < 0) g->h[i] = 0;   // no film of water
            if (g->map) g->z[i] = g->h[i] * step;
        }
        if (g->map) map_ground_set(g->map, g->z);
        mg_biomes(g);
        g->land = g->water = g->mountain = 0;
        memset(g->biome_tiles, 0, sizeof g->biome_tiles);
        mg_textures(g);
        // the cliffs, a row's runs of one type at a time: a mountain's and a plateau's all of its own type, the others (the coasts')
        // the winter's in the cold biomes and the data's default else
        if (!g->map) mg_kinds(g);
        for (int y = 0; y < n; y++)
            for (int x = 0, x0 = 0, type = -1; x <= n; x++) {
                int r = x < n && (g->mt[y * g->m + x] > 0.3f || g->plat[y * g->m + x] > 0.3f) ? g->range[y * g->m + x] : 0;
                int t = x == n ? -1 : r ? g->range_cliffs[r - 1] : g->clim[y * n + x] < 3 ? CLIFFS_WINTER : CLIFFS_DEFAULT;
                if (t == type) continue;
                if (type >= 0 && g->map) map_cliffs_run(g->map, x0, x, y, type);
                else if (type >= 0) mg_cliff_run(g, x0, x, y, type);
                x0 = x; type = t;
            }
        if (!g->placed) { mg_starts(g); mg_yards(g); continue; }
        int laid = mg_afoot(g, mends++ < 3);
        if (!laid) break;
        g->mended += laid;
    }
    for (int i = 0; i < n * n; i++) g->cliffs += mg_kind(g, i % n, i / n) == MG_K_CLIFF;
    // as the game's maps have it (Continental 0, the island types 1, Plains 2): 1 the players apart by water or a
    // continent to reach by it, 0 one continent's
    if (g->map) map_set_flags(g->map, g->nc > 1 ? 1 : 0);
    mg_objects(g);
    mg_starts_own(g);
    if (g->map) mg_scatter(g);
}

// WHO STARTS WHERE (the player 2026-11-26: "next the per team sorting, and then randomize the teams' positions in the
// list, so that team 1 (usually player 1) doesn't always start in the north west"). The starts are the map's places
// (seats: seat s on continent s & 1, the (s / 2)-th along it, mg_starts; the players round one continent: the s-th
// round it) whoever sits there - a preview's map is the
// game's whatever the teams. The players get them by team: the game makes its players from the setup's slots in
// order, one for each slot of a computer, a human or an observer (0, 1, 2: game start 0x52599B), and allies those of
// one team (a slot's +0x28; 0 none, an observer none). A team's players sit on one continent side by side; one too
// large for either continent's seats left is split over both. The teams go in an order drawn - the largest first -
// each to a continent where it fits whole (the one with more seats free; a tie drawn), the blocks of a continent along
// it in an order drawn, from one end or the other. The draws from a generator of their own seeded by the map's seed
// (the map's own draws stay as they were): the same on every PC. seat[i], team[i]: player i's seat and team (0 none).
internal int mg_draw(uint32_t *r, int m) {
    uint32_t x = *r;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *r = x;
    return (int)(x % (uint32_t)m);
}
internal void mg_seats(const struct mg *g, const ee_setup *setup, int *seat, int *team) {
    int np = g->np, key[MG_PLAYERS], id[MG_PLAYERS], size[MG_PLAYERS], order[MG_PLAYERS], nt = 0, k = 0;
    uint32_t r = g->seed * 0x9E3779B1u ^ 0x7EA35u;
    if (!r) r = 1;
    for (int i = 0; i < np; i++) {   // the players' teams: the i-th slot of a computer, a human or an observer is player i
        int t = 0;
        while (setup && k < 16) {
            const ee_setup_slot *sl = setup_slot_at(setup, k++);
            int type = setup_slot_get_type(sl);
            if (type == 0 || type == 1 || type == 2) { t = type == 2 ? 0 : setup_slot_get_team(sl); break; }
        }
        team[i] = t > 0 ? t : 0;
        key[i] = t > 0 ? t : 1000 + i;   // none: a team of its own
        seat[i] = -1;
    }
    for (int i = 0; i < np; i++) {
        int j = 0;
        while (j < nt && id[j] != key[i]) j++;
        if (j == nt) { id[nt] = key[i]; size[nt++] = 0; }
        size[j]++;
    }
    for (int i = 0; i < nt; i++) order[i] = i;
    for (int i = nt - 1; i > 0; i--) { int j = mg_draw(&r, i + 1), t = order[i]; order[i] = order[j]; order[j] = t; }
    for (int i = 1; i < nt; i++)   // the largest first (stable: the drawn order among those of a size)
        for (int j = i; j > 0 && size[order[j]] > size[order[j - 1]]; j--) { int t = order[j]; order[j] = order[j - 1]; order[j - 1] = t; }
    int homes = g->homes, left[2] = { (np + homes - 1) / homes, (np + homes - 2) / homes }, nb = 0;   // (one continent: all its seats)
    struct { int team, k, n; } block[2 * MG_PLAYERS];
    for (int o = 0; o < nt; o++)
        for (int want = size[order[o]]; want > 0; ) {
            int fits0 = left[0] >= want, fits1 = left[1] >= want, c;
            if (homes == 1) c = 0;
            else if (fits0 != fits1) c = fits1;
            else c = left[0] > left[1] ? 0 : left[1] > left[0] ? 1 : mg_draw(&r, 2);
            int take = want < left[c] ? want : left[c];
            block[nb].team = order[o]; block[nb].k = c; block[nb].n = take; nb++;
            left[c] -= take; want -= take;
        }
    for (int c = 0; c < homes; c++) {
        int on[2 * MG_PLAYERS], m = 0, seats = (np + homes - 1 - c) / homes, from_end = mg_draw(&r, 2), at = 0;
        for (int b = 0; b < nb; b++) if (block[b].k == c) on[m++] = b;
        for (int i = m - 1; i > 0; i--) { int j = mg_draw(&r, i + 1), t = on[i]; on[i] = on[j]; on[j] = t; }
        for (int b = 0; b < m; b++)
            for (int q = 0; q < block[on[b]].n; q++, at++) {
                int j = from_end ? seats - 1 - at : at, i = 0;
                while (i < np && (seat[i] >= 0 || key[i] != id[block[on[b]].team])) i++;   // the team's next player without a seat
                if (i < np) seat[i] = homes * j + c;
            }
    }
    for (int i = 0, s = 0; i < np; i++)   // (none left without one; a guard all the same: the seats nobody has)
        if (seat[i] < 0) {
            for (s = 0; s < np; s++) { int taken = 0; for (int o = 0; o < np; o++) taken |= seat[o] == s; if (!taken) break; }
            seat[i] = s < np ? s : i;
        }
}

// THE GAME'S MAP MADE. players: the game's vector of the players that play, the world first; human: this PC's index in
// it (0: none); start: where its view begins
internal int mapgen_make(const ee_setup *setup, void *players, int human, uint32_t seed, ee_map *map, float *start) {
    ee_player **pb = FIELD(players, 4, ee_player **), **pe = FIELD(players, 8, ee_player **);
    int count = pb ? (int)(pe - pb) : 0, n = (int)map_get_size(map), units = 0;
    struct mg *g = count > 1 ? mg_new(map, n, count - 1, seed, setup) : NULL;
    if (!g) {
        notice("mapgen", "the map type \"%s\" could not be made (%d players, %d tiles a side) - an empty map instead", MG_NAME, count - 1, n);
        start[0] = start[1] = (float)n * 0.5f; start[2] = 0;
        return 0;
    }
    g->world = pb[0];
    uint32_t keep = 0;   // the preview's seed instead of the game's: the next single-player random map's, or the host's
    int kept = 0;
    if (g_mg_keep.on && setup && setup_get_mode(setup) == 0) {   // (the setup screen's: a random-map game)
        g_mg_keep.on = 0;
        if (!setup_reveal_map(setup)) {   // (unticked after the dialog: its fog would hide a map seen - md_open_setup)
            if (g_log) log_locked("# mapgen: the seed of the options' preview, %u, left out - the map is not revealed\n", g_mg_keep.seed);
        } else if (!g->opt.mp) { kept = 1; keep = g_mg_keep.seed; }
        else if (g_log) log_locked("# mapgen: the seed of the options' preview, %u, left out - a multiplayer game plays the host's\n", g_mg_keep.seed);
    }
    if (g->opt.from == MG_FROM_HOST && g_mg_host.keep) { kept = 1; keep = g_mg_host.seed; }
    if (kept) {
        if (g_log) log_locked("# mapgen: the seed of the options' preview, %u, instead of the game's %u%s\n", keep, seed, g->opt.from == MG_FROM_HOST ? " (the host's)" : "");
        seed = g->seed = keep; g->rng = seed * 2654435761u | 1;
    }
    g_mg_host.got = 0;   // this game's (the next game's options come with its own Start)
    mg_steps(g);
    int seat[MG_PLAYERS], team[MG_PLAYERS];
    mg_seats(g, setup, seat, team);
    start[0] = start[1] = g->half + 0.5f;
    for (int i = 0; i < g->np; i++) {
        ee_player *pl = pb[i + 1];
        float cx = (float)g->sx[seat[i]], cy = (float)g->sy[seat[i]];
        units += mapgen_unit(UT(B_CAPITOL), pl, cx, cy, map) != NULL;
        int want = (int)player_get_start_citizens(pl);
        for (int k = 0; k < want && k < 12; k++)
            units += mapgen_unit(k & 1 ? UT(FEMALE_CITIZEN) : UT(CITIZEN), pl, cx + 3.5f + (float)(k / 6), cy - 2.5f + (float)(k % 6), map) != NULL;
        if (i + 1 == human) { start[0] = cx + 0.5f; start[1] = cy + 0.5f; }
    }
    start[2] = ground_at(map, start[0], start[1]);
    if (g_log) {
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# mapgen: \"%s\" made - seed %u, %d tiles a side, %d land and %d water tiles, %d cliff tiles, %d mountain ranges of %d tiles with %d mines at their feet, %d plateaus, %d lakes, %d rivers with %d fords (%d laid for a start cut off), %d oases, %d swamps, %d islands, %d capes, %d bays, %d fjords, the continents %s on foot, %d starts cut off from their continent's, %d of %d land tiles reached from the starts, %d players (%d start units) - each's start (/ its team) at",
                MG_NAME, seed, n, g->land, g->water, g->cliffs, g->ranges, g->mountain, g->mountain_mines, g->plateaus, g->lakes, g->rivers, g->fords_ok, g->mended, g->oases, g->swamps, g->islands, g->capes, g->bays, g->fjords, g->afoot ? "JOINED" : "apart", g->cut, g->walked, g->walkable, g->np, units);
        for (int i = 0; i < g->np; i++) {
            fprintf(g_log, " %d,%d", g->sx[seat[i]], g->sy[seat[i]]);
            if (team[i]) fprintf(g_log, "/%d", team[i]);
        }
        fprintf(g_log, "; gold %d iron %d stone %d (%d of them in %d clumps) forage %d fish %d trees %d in %d forests (of the mild green biomes' %d spruce, %d mixed, %d maple), animals %d, scatter %d; the view from %.1f %.1f; land tiles by biome:", g->made[MG_GOLD],
                g->made[MG_IRON], g->made[MG_STONE], g->clump_mines, g->clumps, g->made[MG_FORAGE], g->made[MG_FISH], g->made[MG_TREE], g->forests, g->wood_made[0], g->wood_made[1], g->wood_made[2],
                g->made[MG_ANIMAL], g->made[MG_AMBIENT], (double)start[0], (double)start[1]);
        for (int b = 0; b < MG_BIOMES; b++) fprintf(g_log, " %s %d", k_mg_biome[b].name, g->biome_tiles[b]);
        fprintf(g_log, "; the players' starts in:");
        for (int i = 0; i < g->np; i++) fprintf(g_log, " %s", k_mg_biome[g->clim[g->sy[seat[i]] * n + g->sx[seat[i]]]].name);
        fprintf(g_log, "; the starts placed where they looked first %d, further out %d, with a smaller yard %d, with the smallest %d, forced %d;"
                       " their own laid beyond what the land had near them: forage %d gold %d iron %d stone %d forests %d herds %d\n",
                g->start_how[0], g->start_how[1], g->start_how[2], g->start_how[3], g->start_how[4],
                g->topped[0], g->topped[1], g->topped[2], g->topped[3], g->topped[4], g->topped[5]);
        LeaveCriticalSection(&g_lock);
    }
    mg_delete(g);
    return 0;
}

// THE PREVIEW (the player 2026-11-25: the options' dialog shows what they make): the map of n tiles a side for np
// players from seed, by the same steps into the generator's own arrays - no game's map, no units: in the editor the
// map OK makes (its seed is drawn before), on the setup screen the map of a game started with "play this map": the
// game's map's corners (mg_corner) and tile kinds by its rules (mg_kinds, mg_cliff_run), so that the walks, the
// fords and the free tiles come out alike - checked on maps of 150 - 440 tiles, the editor's and a game's: every lake,
// river, mine and forest where the preview had it (until the diamond was one corner wider, 2026-11-26, a game's map
// parted from its preview now and then) - painted top down into px, size x size (a 32-bit DIB's 0x00RRGGBB,
// the top row first): the far end, the north, up, the continents left and right as the game's minimap shows them. A
// tile's colour: its biome's ground (a forest darker), the sea deeper darker, shallows and inland water lighter, a
// mountain toward rock, a cliff dark, lit from the north-west by its corners' heights; the mines as dots (gold
// yellow, iron red, stone grey, forage purple), the starts as white squares. -> what was made (log line), 0 no memory
internal int mapgen_preview(int n, int np, uint32_t seed, uint32_t *px, int size, char *what, int cap) {
    static const uint8_t land[MG_BIOMES][3] = { { 236, 240, 244 }, { 214, 222, 218 }, { 196, 208, 200 }, { 190, 178, 120 }, { 120, 165, 80 },
                                                { 95, 150, 70 }, { 226, 204, 150 }, { 196, 176, 100 }, { 70, 155, 70 } };
    static const uint32_t dot[4] = { 0xFFD400, 0xC84828, 0xB4B4B4, 0xB040B0 };   // gold, iron, stone, forage
    struct mg *g = mg_new(NULL, n, np, seed, NULL);
    if (!g) return 0;
    mg_steps(g);
    float half = g->half, scale = (float)n / (float)size;
    for (int j = 0; j < size; j++)
        for (int i = 0; i < size; i++) {
            float u = ((float)j + 0.5f) * scale - half, v = ((float)i + 0.5f) * scale - half;
            int x = (int)floorf((u + v + (float)n) * 0.5f), y = (int)floorf((v + (float)n - u) * 0.5f);
            uint32_t *o = &px[j * size + i];
            float lo, hi;
            if (!mg_tile_span(g, x, y, &lo, &hi)) { *o = 0x202020; continue; }
            int t = y * n + x, b = g->clim[t], kind = g->kind[t];
            float c[3], *hc = g->h + y * g->m + x;
            for (int k = 0; k < 3; k++) c[k] = land[b][k];
            if (kind == MG_K_DEEP || kind == MG_K_SHALLOWS) {
                float d = mg_step(-0.75f, -5.0f, lo);
                static const float sea[2][3] = { { 70, 128, 178 }, { 22, 50, 104 } }, inland[3] = { 72, 122, 150 };
                for (int k = 0; k < 3; k++) c[k] = g->marsh[t] == 2 || kind == MG_K_DEEP ? sea[0][k] + (sea[1][k] - sea[0][k]) * d : inland[k];
            } else if (kind == MG_K_CLIFF) {
                c[0] = 96; c[1] = 86; c[2] = 76;
            } else {
                if (g->tex[t] == g->tex_shore[b] && g->tex_shore[b] >= 0) { c[0] = 222; c[1] = 206; c[2] = 160; }
                else if (g->tex[t] == g->tex_pebbles && g->tex_pebbles >= 0) { c[0] = 160; c[1] = 150; c[2] = 130; }
                if (g->obj[t] == MG_TREE + 1) { c[0] *= 0.45f; c[1] *= 0.62f; c[2] *= 0.45f; }
                float rock = 0.7f * mg_step(2, 7, g->mt[y * g->m + x]);
                if (b >= 3) for (int k = 0; k < 3; k++) c[k] += (125 - c[k]) * rock;
                float shade = 1 - 0.16f * ((hc[g->m] - hc[1]) + (hc[0] - hc[g->m + 1]));   // the north corner minus the south one, the west minus the east
                shade = shade < 0.55f ? 0.55f : shade > 1.35f ? 1.35f : shade;
                for (int k = 0; k < 3; k++) c[k] *= shade;
            }
            uint32_t r = (uint32_t)fminf(255, c[0]), gg = (uint32_t)fminf(255, c[1]), bb = (uint32_t)fminf(255, c[2]);
            *o = r << 16 | gg << 8 | bb;
        }
    // the mines and the starts over it: at their tiles' middles
    int rad = size / 220 > 0 ? size / 220 : 1;
    for (int t = 0; t < n * n + g->np; t++) {
        int x, y, r = rad;
        uint32_t col;
        if (t < n * n) {
            int o = g->obj[t] - 1;
            if (o < MG_GOLD || o > MG_FORAGE) continue;
            x = t % n; y = t / n; col = dot[o];
        } else { x = g->sx[t - n * n]; y = g->sy[t - n * n]; col = 0xFFFFFF; r = rad + 2; }
        float u = (float)(x - y), v = (float)(x + y + 1 - n);
        int ci = (int)((v + half) / scale), cj = (int)((u + half) / scale);
        for (int dj = -r - 1; dj <= r + 1; dj++)
            for (int di = -r - 1; di <= r + 1; di++) {
                int pi = ci + di, pj = cj + dj, edge = abs(di) > r || abs(dj) > r;
                if (pi >= 0 && pj >= 0 && pi < size && pj < size) px[pj * size + pi] = edge ? 0x101010 : col;
            }
    }
    snprintf(what, (size_t)cap, "%d land tiles, %d cliff tiles, %d ranges, %d lakes, %d rivers, %d islands, gold %d iron %d stone %d, %d forests",
             g->land, g->cliffs, g->ranges, g->lakes, g->rivers, g->islands, g->made[MG_GOLD], g->made[MG_IRON], g->made[MG_STONE], g->forests);
    mg_delete(g);
    return 1;
}

internal int __cdecl hook_random_map_create(ee_setup *setup, void *players, int human, void *name, uint32_t seed, ee_map *map, float *start) {
    if (mapgen_is_ours(name ? FIELD(name, 4, const char *) : NULL)) return mapgen_make(setup, players, human, seed, map, start);
    int r = random_map_create(setup, players, human, name, seed, map, start);
    if (g_log) log_locked("# mapgen: the game's own map made (%d): its flags %u\n", r, map_get_flags(map));
    return r;
}

// the game's list is made at its start, which may be over when the mod's patches go in: then the types are added now
internal void mapgen_install(void) {
    int of = 0, n = patch_random_map_create(hook_random_map_create, &of), l = patch_random_map_types_load(hook_random_map_types_load);
    int c = patch_lobby_check(hook_lobby_check);
    mod_msg_on(MOD_MSG_MAP_OPTIONS, mg_host_seen);
    mapgen_types_add();
    if (g_log) fprintf(g_log, "# mapgen: the mod's map types - the game's map maker's calls %d/%d, its list's load %s, the Start button's check %s%s\n", n, of,
                       l ? "hooked" : "FAILED", c ? "hooked" : "FAILED", g_mapgen_added ? " (the list was there: added now)" : "");
}
