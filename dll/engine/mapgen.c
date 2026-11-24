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
// IN MULTIPLAYER every PC makes the map itself from the setup and the seed: the same numbers on each. So nothing here
// is drawn from the clock or the game's random numbers, and no sine, cosine or arc tangent is used (the C library's
// may differ between PCs in a last bit; +, -, x, / and the square root don't) - directions are vectors, turned by
// adding a share of their normal.
// TWO CONTINENTS so far, its numbers in the code until the steps are settled (then a description in .eeai).
#define MG_NAME "Two Continents"
internal int g_mapgen_added;

internal int mapgen_is_ours(const char *name) { return name && !lstrcmpiA(name, MG_NAME); }

// the mod's types pushed into the game's list, once, when the game has made its own
internal void mapgen_types_add(void) {
    if (g_mapgen_added || !map_types_count()) return;
    g_mapgen_added = 1;
    const char *title = word("map_two_continents");   // the list's line, in the game's language (English before the words are read)
    map_types_add(MG_NAME, lstrcmpA(title, "map_two_continents") ? title : MG_NAME, "Default");
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
} k_mg_biome[MG_BIOMES] = {
    // cold: dry, middling, wet
    { "ice desert", { "Snow", "Ice", "Snow - Dirt Patches", "Snow", "Snow" }, { 45, 25, 20, 5, 5 }, "Ice", "Snow - Dirt Patches", "Snow - Tufts",
      { MG_DEAD }, { MG_FIRS }, 1500, 6, 14, { "res_walrus", "res_goat" }, { 60, 100, 100, 10, 30 } },
    { "tundra", { "Snow", "Snow - Grass Patches", "Snow - Tufts", "Snow", "Snow" }, { 40, 30, 20, 5, 5 }, "Ice", "Snow - Dirt Patches", "Snow - Tufts",
      { MG_FIRS }, { NULL }, 650, 18, 36, { "res_deer", "res_goat", "res_walrus" }, { 60, 100, 80, 30, 60 } },
    { "taiga", { "Snow - Tufts", "Snow", "Snow - Grass Patches", "Snow", "Snow" }, { 40, 30, 20, 5, 5 }, "Ice", "Snow - Dirt Patches", "Snow - Tufts",
      { MG_FIRS }, { NULL }, 200, 36, 60, { "res_deer", "res_goat" }, { 50, 90, 60, 50, 80 } },
    // mild
    { "steppe", { "Grass - Sparse", "Dry Leaves", "Grass - Patches", "Grass", "Grass - Sparse" }, { 40, 20, 25, 10, 5 }, "Beach", "Sand - Medium", "Dry Leaves",
      { MG_DEAD }, { MG_MAPLE }, 1200, 8, 20, { "res_horse", "res_goat", "res_ostrich" }, { 80, 80, 80, 50, 80 } },
    { "grassland", { "Grass", "Grass - Tufts", "Grass - Tall", "Grass - Patches", "Grass" }, { 60, 10, 6, 8, 16 }, "Beach", "Sand - Medium", "Grass - Sparse",
      { MG_MAPLE }, { NULL }, 420, 26, 50, { "res_deer", "res_horse", "res_goat" }, { 70, 70, 70, 100, 100 } },
    { "woodland", { "Grass", "Grass - Tall", "Grass - Tufts", "Grass - Overgrown", "Grass" }, { 40, 22, 15, 18, 5 }, "Beach", "Sand - Medium", "Grass - Sparse",
      { MG_MIXED }, { NULL }, 170, 40, 60, { "res_deer", "res_goat" }, { 50, 60, 50, 100, 90 } },
    // hot
    { "desert", { "Sand - Fine", "Sand - Medium", "Desert", "Sand - Cracked", "Dark Cracked Mud" }, { 35, 25, 20, 12, 8 }, "Desert", "Pebbles - Small", "Sand - Medium",
      { MG_PALMS }, { NULL }, 1300, 5, 12, { "res_ostrich", "res_goat" }, { 100, 60, 100, 15, 20 } },
    { "savanna", { "Grass - Patches", "Grass - Sparse", "Sand - Medium", "Grass", "Grass - Patches" }, { 40, 30, 15, 10, 5 }, "Beach", "Pebbles - Small", "Grass - Sparse",
      { MG_PALMS }, { NULL }, 800, 10, 24, { "res_giraffe", "res_elephant", "res_ostrich", "res_horse" }, { 90, 60, 70, 60, 100 } },
    { "tropics", { "Grass - Overgrown", "Grass - Tall", "Grass", "Grass - Tufts", "Grass - Overgrown" }, { 40, 25, 20, 10, 5 }, "Beach", "Sand - Medium", "Grass - Tall",
      { MG_PALMS }, { NULL }, 160, 40, 60, { "res_elephant", "res_hippopotamus", "res_giraffe" }, { 50, 40, 40, 100, 90 } },
};

// THE GENERATOR'S STATE. Tiles x, y (0 .. n - 1), corners 0 .. n: the map is the diamond of them the game made (a tile
// there has its ee_tile). Across the diamond run u = x - y and v = x + y - n, each from -n / 2 to n / 2: the screen's
// two ways - v from left to right, u from the near edge to the far one. The continents lie apart along v, left and
// right, and stretch along u; the far end is the north.
enum { MG_GOLD, MG_IRON, MG_STONE, MG_FORAGE, MG_FISH, MG_TREE, MG_ANIMAL, MG_KINDS };
#define MG_PLAYERS 16
#define MG_SPOTS 1024
struct mg {
    ee_map *map;
    ee_player *world;
    int n, m;                 // tiles a side, corners a side
    uint32_t rng, seed;
    float *f;                 // [corner] how far inside the land (tiles; below 0: that far out in the water)
    float *h;                 // [corner] its height in height steps (below 0: under the water)
    float *mt;                // [corner] the mountains' height there (steps; 0: none)
    uint8_t *range;           // [corner] which range's that is, from 1 (0: none)
    int8_t range_cliffs[16];  // a range's cliff type (CLIFFS_*): one for the whole mountain
    uint8_t *used;            // [tile] taken: a start's yard, a river's bank, a mine, a tree
    uint8_t *clim;            // [tile] its biome (k_mg_biome)
    uint8_t *marsh;           // [tile] water: 2 the sea's, 1 a river's or a pond's all under water and not frozen (marsh), 0 other inland water
    float half;               // n / 2: u and v reach that far
    int np;                   // the players that play
    int sx[MG_PLAYERS], sy[MG_PLAYERS];   // their starts (a tile corner: the Capitol's middle)
    struct { int x, y; } spot[MG_SPOTS];  // the mines', forests' and herds' places: kept apart from each other
    int nspot;
    int tex_land[MG_BIOMES][5], tex_beach[MG_BIOMES], tex_res[MG_BIOMES], tex_wood[MG_BIOMES], tex_shore[MG_BIOMES], tex_deep, tex_marsh, tex_pebbles;
    int tree[MG_BIOMES][6], ntree[MG_BIOMES], grove[MG_BIOMES][6], ngrove[MG_BIOMES], animal[MG_BIOMES][4], nanimal[MG_BIOMES];   // the biomes' types the data has
    struct { int x, y; } spring[16];      // the rivers' springs: apart from each other
    int made[MG_KINDS], rivers, land, water, cliffs, forests, ranges, mountain, mountain_mines, types_missing, biome_tiles[MG_BIOMES];
};
internal uint32_t mg_next(struct mg *g) {
    uint32_t x = g->rng;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    return g->rng = x;
}
internal float mg_rnd(struct mg *g) { return (float)(mg_next(g) >> 8) * (1.0f / 16777216.0f); }          // 0 .. 1
internal int mg_int(struct mg *g, int lo, int hi) { return lo + (int)(mg_next(g) % (uint32_t)(hi - lo + 1)); }   // lo .. hi
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
internal inline int mg_corner(const struct mg *g, int x, int y) { return x >= 0 && y >= 0 && x <= g->n && y <= g->n && map_corner(g->map, x, y); }
internal inline ee_tile *mg_tile(const struct mg *g, int x, int y) { return x >= 0 && y >= 0 && x < g->n && y < g->n ? map_get_tiles(g->map)[y * g->n + x] : NULL; }
internal inline float mg_f(const struct mg *g, int x, int y) { return mg_corner(g, x, y) ? g->f[y * g->m + x] : -99.0f; }
// a tile's lowest and highest corner (height steps); 0: no tile there
internal int mg_tile_span(const struct mg *g, int x, int y, float *lo, float *hi) {
    if (!mg_tile(g, x, y) || !mg_corner(g, x + 1, y) || !mg_corner(g, x, y + 1) || !mg_corner(g, x + 1, y + 1)) return 0;
    const float *h = g->h + y * g->m + x;
    *lo = fminf(fminf(h[0], h[1]), fminf(h[g->m], h[g->m + 1]));
    *hi = fmaxf(fmaxf(h[0], h[1]), fmaxf(h[g->m], h[g->m + 1]));
    return 1;
}

// THE LAND: two continents, each an ellipse about the middle of its half of the map (0.42 of the half wide, 0.85 long),
// its shore pushed in and out by noise; water along the map's edge and in a channel down the middle whatever the
// noise says. f is the distance inside the land in tiles, as the ellipses and the noise give it.
internal void mg_land(struct mg *g) {
    float half = g->half, ra = 0.42f * half, rb = 0.85f * half;
    for (int y = 0; y <= g->n; y++)
        for (int x = 0; x <= g->n; x++) {
            float u = (float)(x - y), v = (float)(x + y - g->n), best = -1e9f;
            for (int k = 0; k < 2; k++) {
                float da = (v - (k ? 0.5f : -0.5f) * half) / ra, db = u / rb;
                float fk = (1 - sqrtf(da * da + db * db)) * ra;
                if (fk > best) best = fk;
            }
            best += (mg_fbm((float)x * 0.045f, (float)y * 0.045f, g->seed) - 0.5f) * 0.5f * ra;
            float edge = fminf(half - fabsf(u), half - fabsf(v)) - 4, channel = fabsf(v) - 4;
            g->f[y * g->m + x] = fminf(best, fminf(edge, channel)) * 0.7071f;
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
}
// a bluff: 1.8 steps up within a tile from `at` tiles inside the land (a cliff: a tile whose corners differ by 1.5
// steps is one), down again to the plain 3 .. 12 tiles behind
internal float mg_bluff(float f, float at) { return 1.8f * mg_step(at, at + 0.8f, f) * (1 - mg_step(at + 3, at + 12, f)); }
internal float mg_start_dist(const struct mg *g, float x, float y);
// the temperature at a place before its height: cold (under -0.3) in the far north, hot (over 0.3) in the near south,
// pushed about by a noise
internal float mg_temp(const struct mg *g, int x, int y) {
    return (float)(x - y) / g->half + (mg_fbm((float)x * 0.025f + 90, (float)y * 0.025f + 40, g->seed + 20) - 0.5f) * 1.0f;
}
// A MOUNTAIN RANGE (the player 2026-11-24: "cliffs, mountain ranges, rivers flowing from the mountains, increased
// chance for mine in mountains"; then, of the first ones - 5 steps high, in terraces -: "the mountains inland still
// need some work, they should also be a lot higher to justify the snow"): a line from a point well inside the land of
// the continent asked for (10 tiles, 24 from any start), a tile a step, turned a little at random each step, n/6 ..
// n/3 tiles long or until it comes within 7 tiles of the shore or 20 of a start (shorter than n/12: none). The
// mountain along it: a crest up to 14 steps high - peaks of half to the whole of that by a noise along the line,
// lower towards its ends -, falling to the plain 10 - 15 tiles to either side, steeply by the crest (cliffs) and
// gently at its foot (to walk and build on); a pass every 30 - 45 tiles of the line, the crest down to a fifth there:
// a saddle to walk over. tight 0 .. 3: a small map's - the starts 3 and 2 tiles nearer for each. A range has one
// cliff type, by the temperature at its middle (the player 2026-11-24, of mountains with the cold biomes' cliffs on
// top and the default below: "try to stick to a single cliff type"): the winter's but in the hot south, where it is
// the default's and the mountain bare of snow.
#define MG_PEAK 14.0f
internal int mg_range(struct mg *g, int continent, int tight) {
    float px[128], py[128];
    int x0 = mg_int(g, 0, g->n), y0 = mg_int(g, 0, g->n), want = mg_int(g, g->n / 6, g->n / 3), len = 0;
    if ((x0 + y0 > g->n) != continent || mg_f(g, x0, y0) < 10 || mg_start_dist(g, (float)x0, (float)y0) < (float)(24 - 3 * tight)) return 0;
    float x = (float)x0, y = (float)y0, dx = mg_rnd(g) - 0.5f, dy = mg_rnd(g) - 0.5f, dl = sqrtf(dx * dx + dy * dy);
    if (dl < 0.01f) { dx = 1; dy = 0; dl = 1; }
    dx /= dl; dy /= dl;
    for (want = want > 127 ? 127 : want; len < want; len++) {
        px[len] = x; py[len] = y;
        float turn = (mg_rnd(g) - 0.5f) * 0.5f, nx = dx - dy * turn, ny = dy + dx * turn, nl = sqrtf(nx * nx + ny * ny);
        nx /= nl; ny /= nl;
        if (mg_f(g, (int)floorf(x + nx + 0.5f), (int)floorf(y + ny + 0.5f)) < 7 || mg_start_dist(g, x + nx, y + ny) < (float)(20 - 2 * tight)) break;
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
                    * (0.2f + 0.8f * mg_step(1, 5, gap));
        for (int cy = (int)py[i] - 16; cy <= (int)py[i] + 16; cy++)
            for (int cx = (int)px[i] - 16; cx <= (int)px[i] + 16; cx++) {
                if (!mg_corner(g, cx, cy)) continue;
                float ex = (float)cx - px[i], ey = (float)cy - py[i], s = 1 - sqrtf(ex * ex + ey * ey) / wide;
                float v = s > 0 ? top * s * s * sqrtf(sqrtf(s)) : 0;   // (1 - d / wide) ^ 2.25: a sharp crest, a flat foot
                if (v > g->mt[cy * g->m + cx]) { g->mt[cy * g->m + cx] = v; g->range[cy * g->m + cx] = (uint8_t)(g->ranges + 1); }
            }
    }
    g->range_cliffs[g->ranges] = mg_temp(g, (int)px[len / 2], (int)py[len / 2]) > 0.3f ? CLIFFS_DEFAULT : CLIFFS_WINTER;
    return 1;
}
// THE HEIGHTS. The plain shore rises 0.3 steps a tile to the plain at 1 step, the sea falls as gently to 5 steps
// down; a wide beach lies nearly flat for its width (0.08 a tile, the sea before it shallow twice as far), a rocky
// coast and a beach's bluff are cliffs. Hills of up to 3.5 steps inland, from a second noise. The mountains (the
// ranges before: g->mt) keep 5 tiles off the shore and 12 off the starts, a noise makes their sides rugged (0.75 ..
// 1.25 of their height).
internal void mg_heights(struct mg *g) {
    for (int y = 0; y <= g->n; y++)
        for (int x = 0; x <= g->n; x++) {
            float f = g->f[y * g->m + x], hn = mg_fbm((float)x * 0.035f + 17, (float)y * 0.035f + 5, g->seed + 10), beach, width, bluff, rocky;
            mg_coast(g, x, y, &beach, &width, &bluff, &rocky);
            if (f < 0) { g->h[y * g->m + x] = fmaxf(f * (0.3f - 0.15f * beach + 1.0f * rocky), -5.0f); continue; }
            float base = fminf(f * 0.3f, 1.0f), flat = fminf(f * 0.08f, base);
            float h = base + (flat - base) * beach * (1 - mg_step(width, width + 3, f)) + bluff * mg_bluff(f, width) + rocky * mg_bluff(f, 0);
            h += fmaxf(0, hn - 0.56f) / 0.44f * 3.5f * mg_step(5, 14, f);
            float m = g->mt[y * g->m + x] * mg_step(5, 10, f) * mg_step(12, 16, mg_start_dist(g, (float)x, (float)y));
            m *= 0.75f + 0.5f * mg_fbm((float)x * 0.2f + 31, (float)y * 0.2f + 9, g->seed + 62);
            g->mt[y * g->m + x] = m;
            g->h[y * g->m + x] = h + m;
        }
}
// THE STARTS: the players by turns on the two continents, a continent's own spread along its length; a start less
// than 10 tiles inside the land moved to the nearest corner within 12 tiles that is (none: the one farthest inside).
// Then (mg_yards, once the heights are there - the mountains keep off the starts) the ground round it levelled to the
// plain (whole within 7 tiles, fading to 11), its yard (7 tiles) kept free
internal void mg_starts(struct mg *g) {
    int count[2] = { g->np - g->np / 2, g->np / 2 };
    for (int i = 0; i < g->np; i++) {
        int k = i & 1, j = i / 2;
        float v = (k ? 0.5f : -0.5f) * g->half, u = (((float)j + 0.5f) / (float)count[k] - 0.5f) * 2 * 0.72f * 0.85f * g->half;
        int x0 = (int)floorf((u + v + (float)g->n) * 0.5f), y0 = (int)floorf((v + (float)g->n - u) * 0.5f), bx = x0, by = y0;
        int nearest = 1 << 30;
        for (int y = y0 - 12; mg_f(g, x0, y0) < 10 && y <= y0 + 12; y++)
            for (int x = x0 - 12; x <= x0 + 12; x++) {
                int d = (x - x0) * (x - x0) + (y - y0) * (y - y0);
                if (mg_f(g, x, y) >= 10 ? d < nearest : nearest == 1 << 30 && mg_f(g, x, y) > mg_f(g, bx, by)) { bx = x; by = y; if (mg_f(g, x, y) >= 10) nearest = d; }
            }
        g->sx[i] = bx; g->sy[i] = by;
    }
}
internal void mg_yards(struct mg *g) {
    for (int i = 0; i < g->np; i++) {
        int bx = g->sx[i], by = g->sy[i];
        for (int y = by - 11; y <= by + 11; y++)
            for (int x = bx - 11; x <= bx + 11; x++) {
                if (!mg_corner(g, x, y)) continue;
                float d = sqrtf((float)((x - bx) * (x - bx) + (y - by) * (y - by))), w = 1 - mg_step(7, 11, d);
                g->h[y * g->m + x] += (1.0f - g->h[y * g->m + x]) * w;
                if (d < 7 && mg_tile(g, x, y)) g->used[y * g->n + x] = 1;
            }
    }
}
internal float mg_start_dist(const struct mg *g, float x, float y) {   // to the nearest start (tiles)
    float best = 1e9f;
    for (int i = 0; i < g->np; i++) {
        float dx = x - (float)g->sx[i], dy = y - (float)g->sy[i], d = sqrtf(dx * dx + dy * dy);
        if (d < best) best = d;
    }
    return best;
}
// A RIVER: from a spring well inside the land (8 tiles, 13 from any start) downhill of f - towards the nearest shore
// - a tile a step, its way turned a little at random each step, until it is in the sea; one that comes within 11
// tiles of a start, finds no sea in 250 steps or winds more than twice the straight way is not dug. Dug: the bed 0.8
// steps under the water along its way, the banks rising 0.6 a tile from it (no cliffs: water 2 - 3 tiles wide), a
// small pond at the spring; its banks (3 tiles) kept free. It ends inland at its spring: the land stays one piece,
// units walk round it. mountain: one from the mountains (the player 2026-11-24: "rivers flowing from the mountains") -
// its spring at a range's foot; no river's way goes over a mountain, no spring is within 20 tiles of another.
internal int mg_river(struct mg *g, int mountain) {
    float px[256], py[256];
    int x0 = mg_int(g, 0, g->n), y0 = mg_int(g, 0, g->n), len = 0;
    if (mg_f(g, x0, y0) < 8 || mg_start_dist(g, (float)x0, (float)y0) < 13) return 0;
    if (g->mt[y0 * g->m + x0] > 1.5f || (mountain && g->mt[y0 * g->m + x0] < 0.2f) || g->rivers >= 16) return 0;
    for (int i = 0; i < g->rivers; i++)
        if ((g->spring[i].x - x0) * (g->spring[i].x - x0) + (g->spring[i].y - y0) * (g->spring[i].y - y0) < 20 * 20) return 0;
    float x = (float)x0, y = (float)y0, dx = 0, dy = 0;
    for (; len < 250; len++) {
        int ix = (int)floorf(x + 0.5f), iy = (int)floorf(y + 0.5f);
        float f = mg_f(g, ix, iy);
        if (f < -90 || mg_start_dist(g, x, y) < 11 || g->mt[iy * g->m + ix] > 1.8f) return 0;
        px[len] = x; py[len] = y;
        if (f < -1.5f) break;
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
        float pond = i < 2 ? 1.0f : 0;
        for (int cy = (int)py[i] - 7; cy <= (int)py[i] + 8; cy++)
            for (int cx = (int)px[i] - 7; cx <= (int)px[i] + 8; cx++) {
                if (!mg_corner(g, cx, cy)) continue;
                float ex = (float)cx - px[i], ey = (float)cy - py[i], d = fmaxf(0, sqrtf(ex * ex + ey * ey) - pond), bed = -0.8f + 0.6f * d;
                if (bed < g->h[cy * g->m + cx]) g->h[cy * g->m + cx] = bed;
                if (d < 3 && mg_tile(g, cx, cy)) g->used[cy * g->n + cx] = 1;
            }
    }
    return 1;
}
internal int mg_type(const char *ident) { return unit_type_named(ident, (int)strlen(ident)); }
// a tile's biome from the two layers. The distance to water first (tiles, 4 ways, the sea's and the rivers'), from every
// water tile outwards
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
            if (!mg_tile(g, nx, ny) || wd[ny * n + nx] != 255) continue;
            wd[ny * n + nx] = (uint8_t)(wd[i] + 1);
            queue[tail++] = ny * n + nx;
        }
    }
    // the sea: the water joined to the water round the land through tiles with a corner that was water before the
    // rivers were dug (f under 0) - a river's and a pond's water is not reached, its mouth is
    uint8_t *sea = g->marsh;
    head = tail = 0;
    for (int i = 0; i < n * n; i++)
        if (!wd[i] && mg_f(g, i % n, i / n) < -3.0f) { sea[i] = 2; queue[tail++] = i; }
    while (head < tail) {
        int i = queue[head++], x = i % n, y = i / n;
        static const int8_t side[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
        for (int s = 0; s < 4; s++) {
            int nx = x + side[s][0], ny = y + side[s][1], j = ny * n + nx;
            if (!mg_tile(g, nx, ny) || wd[j] || sea[j]) continue;
            if (fminf(fminf(mg_f(g, nx, ny), mg_f(g, nx + 1, ny)), fminf(mg_f(g, nx, ny + 1), mg_f(g, nx + 1, ny + 1))) >= 0) continue;
            sea[j] = 2;
            queue[tail++] = j;
        }
    }
    for (int y = 0; y < n; y++)
        for (int x = 0; x < n; x++) {
            float lo = 0, hi = 0;
            mg_tile_span(g, x, y, &lo, &hi);
            int bare = g->range[y * g->m + x] && g->range_cliffs[g->range[y * g->m + x] - 1] != CLIFFS_WINTER;   // a hot range: no snow up there
            float temp = mg_temp(g, x, y) - (bare ? 0 : fmaxf(0, hi - 2.0f) * 0.07f);
            float by_water = 1 - (float)(wd[y * n + x] < 7 ? wd[y * n + x] : 7) / 7.0f;
            float wet = 0.42f + (mg_fbm((float)x * 0.03f + 510, (float)y * 0.03f + 260, g->seed + 40) - 0.5f) * 1.5f + 0.22f * by_water;
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
    int grass = terrain_named("Grass"), def = grass >= 0 ? grass : map_get_default_terrain(g->map), bad = 0;
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
    MG_TEX(g->tex_deep, "Underwater", 0);
    MG_TEX(g->tex_marsh, "Marsh", 0);
    MG_TEX(g->tex_pebbles, "Pebbles - Small", 1);
    if (g->tex_deep < 0) g->tex_deep = def;
#undef MG_TEX
    if (g_log && (bad || g->types_missing))
        log_locked("# mapgen: %d ground textures not used (not in the data, or a water's or a cliff's kind of tile): %s; %d tree and animal types not in the data\n",
                   bad, names, g->types_missing);
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
                // the sea's: beach, deep; a river's: marsh all under water, its shore the biome's ground, frozen its ice
                if (sort == 2) tex = lo > -1.0f ? g->tex_beach[b] : g->tex_deep;
                else if (sort == 1 && g->tex_marsh >= 0) tex = g->tex_marsh;
                else if (hi < 0) tex = g->tex_beach[b];
                g->water++;
            } else {
                // a wide beach's sand on the land, a rocky coast's pebbles
                if (beach > 0.5f && f < width && g->tex_shore[b] >= 0) tex = g->tex_shore[b];
                else if (rocky > 0.5f && f < 2.5f && b >= 3 && g->tex_pebbles >= 0) tex = g->tex_pebbles;
                g->land++;
                g->biome_tiles[b]++;
                g->mountain += g->mt[y * g->m + x] > 2.0f;
            }
            map_reset_tile(g->map, (uint32_t)x, (uint32_t)y, (uint32_t)tex);
        }
}

// THE WORLD'S OBJECTS. A tile is free for one when it is land of the plain (its corners 0.3 steps up at least and
// within 0.8 of each other), no cliff, not taken and not up a mountain (its foot only: what the plain reaches).
internal int mg_free(const struct mg *g, int x, int y) {
    float lo, hi;
    return mg_tile_span(g, x, y, &lo, &hi) && !g->used[y * g->n + x] && lo >= 0.3f && hi - lo < 0.8f && g->mt[y * g->m + x] < 1.4f && !tile_is_cliff(mg_tile(g, x, y));
}
internal ee_unit *mg_object(struct mg *g, int type, int x, int y, float angle, int kind) {
    ee_unit_type *t = type >= 0 ? type_data(type) : NULL;
    ee_unit *u = t ? unit_type_create_on(t, g->world, x, y, g->map, angle) : NULL;
    if (u) g->made[kind]++;
    return u;
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
// a mine or a forage patch: the object, its tile and the four beside it the biome's ground for one
internal void mg_mine(struct mg *g, int kind, int x, int y) {
    static const char *const ident[4] = { "res_gold_mine", "res_iron", "res_stone_mine", "res_forage_bush" };
    static const int8_t side[5][2] = { { 0, 0 }, { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    if (!mg_object(g, mg_type(ident[kind]), x, y, 0, kind)) return;
    int b = g->clim[y * g->n + x], tex = kind == MG_FORAGE ? g->tex_wood[b] : g->tex_res[b];
    for (int k = 0; k < 5; k++) {
        int tx = x + side[k][0], ty = y + side[k][1];
        if (!mg_free(g, tx, ty)) continue;
        map_reset_tile(g->map, (uint32_t)tx, (uint32_t)ty, (uint32_t)tex);
        g->used[ty * g->n + tx] = 1;
    }
    mg_spot_add(g, x, y);
}
// a forest: free tiles grown from a seed tile, a neighbour of a tile it has at a time - as many as the seed's biome
// says; a tree of the biome's on 9 in 10 of them, its forest floor under all. grove: a start's own - the biome's
// timber trees where its own are none, 28 - 50 tiles
internal void mg_forest(struct mg *g, int x, int y, int grove) {
    static const int8_t side[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    int b = g->clim[y * g->n + x], other = grove && g->ngrove[b], kinds = other ? g->ngrove[b] : g->ntree[b];
    const int *trees = other ? g->grove[b] : g->tree[b];
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
        map_reset_tile(g->map, (uint32_t)tx[i], (uint32_t)ty[i], (uint32_t)g->tex_wood[b]);
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
// each start's own - 2 forage patches, 2 gold, 2 iron and 2 stone mines (far enough out to want a Settlement), a grove
// of timber and a herd whatever its biome, two more forests as the biome gives them -, then the land's: places drawn
// by its size, away from the starts, each laid as the biome there gives it. (With one forage patch where the biome
// gave no second, a tundra start stood at 23 people after 38 minutes, the others at 91 - 168: the first epoch has no
// farms, its food is forage, game and fish.)
internal void mg_objects(struct mg *g) {
    static const struct { int8_t kind, n, lo, hi; } own[4] = { { MG_FORAGE, 2, 8, 11 }, { MG_GOLD, 2, 13, 16 }, { MG_IRON, 2, 14, 18 }, { MG_STONE, 2, 15, 20 } };
    int x, y;
    for (int i = 0; i < g->np; i++) {
        for (int k = 0; k < 4; k++)
            for (int j = 0; j < own[k].n; j++)
                if (mg_ring(g, g->sx[i], g->sy[i], own[k].lo, own[k].hi, 5, &x, &y)) mg_mine(g, own[k].kind, x, y);
        for (int j = 0; j < 3; j++)
            if (mg_ring(g, g->sx[i], g->sy[i], 10, 15, 6, &x, &y) && (!j || mg_int(g, 0, 99) < 42000 / k_mg_biome[g->clim[y * g->n + x]].forest_tiles))
                mg_forest(g, x, y, !j);
        if (mg_ring(g, g->sx[i], g->sy[i], 11, 16, 6, &x, &y)) mg_herd(g, x, y);
    }
    static const struct { int8_t kind; int16_t tiles; } wild[4] = { { MG_GOLD, 650 }, { MG_IRON, 650 }, { MG_STONE, 1000 }, { MG_FORAGE, 1100 } };
    for (int k = 0; k < 4; k++)
        for (int j = g->land / wild[k].tiles; j > 0; j--)
            if (mg_anywhere(g, 22, 9, &x, &y) && mg_rich(g, x, y, wild[k].kind)) mg_mine(g, wild[k].kind, x, y);
    for (int j = g->land / 150; j > 0; j--)
        if (mg_anywhere(g, 16, 6, &x, &y) && mg_int(g, 0, 999) < 150000 / k_mg_biome[g->clim[y * g->n + x]].forest_tiles) mg_forest(g, x, y, 0);
    for (int j = g->land / 900; j > 0; j--)
        if (mg_anywhere(g, 18, 8, &x, &y) && mg_rich(g, x, y, MG_RICH_HERD)) mg_herd(g, x, y);
    // the mountains' own (the player 2026-11-24: "increased chance for mine in mountains"): a gold, iron or stone
    // mine by turns per 90 tiles of the ranges' feet (their lowest 1.4 steps), whatever the biome
    int foot = 0, turn = 0;
    for (int ty = 0; ty < g->n; ty++)
        for (int tx = 0; tx < g->n; tx++) foot += g->mt[ty * g->m + tx] > 0.3f && g->mt[ty * g->m + tx] < 1.4f;
    for (int j = foot / 90, t = 0; j > 0 && t < 40000; t++) {
        static const int8_t kinds[3] = { MG_GOLD, MG_IRON, MG_STONE };
        x = mg_int(g, 0, g->n - 1); y = mg_int(g, 0, g->n - 1);
        if (g->mt[y * g->m + x] < 0.3f || !mg_free(g, x, y) || !mg_spot_far(g, x, y, 7)) continue;
        mg_mine(g, kinds[turn++ % 3], x, y);
        g->mountain_mines++;
        j--;
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

// THE MAP MADE, step by step. players: the game's vector of the players that play, the world first; human: this PC's
// index in it (0: none); start: where its view begins
internal int mapgen_make(void *players, int human, uint32_t seed, ee_map *map, float *start) {
    ee_player **pb = FIELD(players, 4, ee_player **), **pe = FIELD(players, 8, ee_player **);
    int count = pb ? (int)(pe - pb) : 0, n = (int)map_get_size(map), units = 0;
    struct mg *g = calloc(1, sizeof *g);
    float *z = NULL;
    if (g) {
        g->map = map; g->n = n; g->m = n + 1; g->half = (float)n * 0.5f;
        g->seed = seed; g->rng = seed * 2654435761u | 1;
        g->world = count ? pb[0] : NULL;
        g->np = count - 1 > MG_PLAYERS ? MG_PLAYERS : count - 1;
        g->f = calloc((size_t)(g->m * g->m), sizeof *g->f);
        g->h = calloc((size_t)(g->m * g->m), sizeof *g->h);
        g->mt = calloc((size_t)(g->m * g->m), sizeof *g->mt);
        g->range = calloc((size_t)(g->m * g->m), 1);
        g->used = calloc((size_t)(n * n), 1);
        g->clim = calloc((size_t)(n * n), 1);
        g->marsh = calloc((size_t)(n * n), 1);
        z = calloc((size_t)(g->m * g->m), sizeof *z);
    }
    if (!g || !g->f || !g->h || !g->mt || !g->range || !g->used || !g->clim || !g->marsh || !z || g->np < 1 || !g->world) {
        notice("mapgen", "the map type \"%s\" could not be made (%d players, %d tiles a side) - an empty map instead", MG_NAME, count - 1, n);
        start[0] = start[1] = (float)n * 0.5f; start[2] = 0;
        return 0;
    }
    mg_land(g);
    mg_starts(g);
    for (int want = 2 + n / 100, t = 0; g->ranges < want && t < 40000; t++) g->ranges += mg_range(g, g->ranges & 1, t / 10000);
    mg_heights(g);
    mg_yards(g);
    for (int want = 2 + n / 60, t = 0; g->rivers < want && t < 6000; t++) g->rivers += mg_river(g, t < 3000);   // from the mountains while such are found
    float step = map_get_height_step(map);
    for (int i = 0; i < g->m * g->m; i++) {
        if (g->h[i] > -0.05f && g->h[i] < 0) g->h[i] = 0;   // no film of water
        z[i] = g->h[i] * step;
    }
    map_ground_set(map, z);
    mg_biomes(g);
    mg_textures_load(g);
    mg_textures(g);
    // the cliffs, a row's runs of one type at a time: a mountain's all of its range's type, the others (the coasts')
    // the winter's in the cold biomes and the data's default else
    for (int y = 0; y < n; y++)
        for (int x = 0, x0 = 0, type = -1; x <= n; x++) {
            int r = x < n && g->mt[y * g->m + x] > 0.3f ? g->range[y * g->m + x] : 0;
            int t = x == n ? -1 : r ? g->range_cliffs[r - 1] : g->clim[y * n + x] < 3 ? CLIFFS_WINTER : CLIFFS_DEFAULT;
            if (t == type) continue;
            if (type >= 0) map_cliffs_run(map, x0, x, y, type);
            x0 = x; type = t;
        }
    for (int i = 0; i < n * n; i++) g->cliffs += map_get_tiles(map)[i] && tile_is_cliff(map_get_tiles(map)[i]);
    map_set_flags(map, 1);   // as the game's island maps have it (Continental 0, the island types 1, Plains 2): the players are apart by water
    mg_objects(g);
    start[0] = start[1] = g->half + 0.5f;
    for (int i = 0; i < g->np; i++) {
        ee_player *pl = pb[i + 1];
        float cx = (float)g->sx[i], cy = (float)g->sy[i];
        units += mapgen_unit(UT(B_CAPITOL), pl, cx, cy, map) != NULL;
        int want = (int)player_get_start_citizens(pl);
        for (int k = 0; k < want && k < 12; k++)
            units += mapgen_unit(k & 1 ? UT(FEMALE_CITIZEN) : UT(CITIZEN), pl, cx + 3.5f + (float)(k / 6), cy - 2.5f + (float)(k % 6), map) != NULL;
        if (i + 1 == human) { start[0] = cx + 0.5f; start[1] = cy + 0.5f; }
    }
    start[2] = ground_at(map, start[0], start[1]);
    if (g_log) {
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# mapgen: \"%s\" made - seed %u, %d tiles a side, %d land and %d water tiles, %d cliff tiles, %d mountain ranges of %d tiles with %d mines at their feet, %d rivers, %d players (%d start units) at",
                MG_NAME, seed, n, g->land, g->water, g->cliffs, g->ranges, g->mountain, g->mountain_mines, g->rivers, g->np, units);
        for (int i = 0; i < g->np; i++) fprintf(g_log, " %d,%d", g->sx[i], g->sy[i]);
        fprintf(g_log, "; gold %d iron %d stone %d forage %d fish %d trees %d in %d forests, animals %d; the view from %.1f %.1f; land tiles by biome:", g->made[MG_GOLD],
                g->made[MG_IRON], g->made[MG_STONE], g->made[MG_FORAGE], g->made[MG_FISH], g->made[MG_TREE], g->forests, g->made[MG_ANIMAL], (double)start[0], (double)start[1]);
        for (int b = 0; b < MG_BIOMES; b++) fprintf(g_log, " %s %d", k_mg_biome[b].name, g->biome_tiles[b]);
        fprintf(g_log, "; the starts in:");
        for (int i = 0; i < g->np; i++) fprintf(g_log, " %s", k_mg_biome[g->clim[g->sy[i] * n + g->sx[i]]].name);
        fprintf(g_log, "\n");
        LeaveCriticalSection(&g_lock);
    }
    free(z); free(g->f); free(g->h); free(g->mt); free(g->range); free(g->used); free(g->clim); free(g->marsh); free(g);
    return 0;
}
internal int __cdecl hook_random_map_create(ee_setup *setup, void *players, int human, void *name, uint32_t seed, ee_map *map, float *start) {
    if (mapgen_is_ours(name ? FIELD(name, 4, const char *) : NULL)) return mapgen_make(players, human, seed, map, start);
    int r = random_map_create(setup, players, human, name, seed, map, start);
    if (g_log) log_locked("# mapgen: the game's own map made (%d): its flags %u\n", r, map_get_flags(map));
    return r;
}

// the game's list is made at its start, which may be over when the mod's patches go in: then the types are added now
internal void mapgen_install(void) {
    int of = 0, n = patch_random_map_create(hook_random_map_create, &of), l = patch_random_map_types_load(hook_random_map_types_load);
    mapgen_types_add();
    if (g_log) fprintf(g_log, "# mapgen: the mod's map types - the game's map maker's calls %d/%d, its list's load %s%s\n", n, of, l ? "hooked" : "FAILED",
                       g_mapgen_added ? " (the list was there: added now)" : "");
}
