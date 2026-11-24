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

// THE GENERATOR'S STATE. Tiles x, y (0 .. n - 1), corners 0 .. n: the map is the diamond of them the game made (a tile
// there has its ee_tile). Across the diamond run u = x - y and v = x + y - n, each from -n / 2 to n / 2: the screen's
// two ways - v from left to right, u from the near edge to the far one. The continents lie apart along v, left and
// right, and stretch along u; the climates change along u: winter in the far north, desert in the near south.
enum { MG_GRASS, MG_WINTER, MG_DESERT, MG_CLIMATES };
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
    uint8_t *used;            // [tile] taken: a start's yard, a river's bank, a mine, a tree
    uint8_t *clim;            // [tile] its climate
    float half;               // n / 2: u and v reach that far
    int np;                   // the players that play
    int sx[MG_PLAYERS], sy[MG_PLAYERS];   // their starts (a tile corner: the Capitol's middle)
    struct { int x, y; } spot[MG_SPOTS];  // the mines', forests' and herds' places: kept apart from each other
    int nspot;
    int tex_land[MG_CLIMATES][5], tex_beach[MG_CLIMATES], tex_res[MG_CLIMATES], tex_wood[MG_CLIMATES], tex_deep;
    int made[MG_KINDS], rivers, land, water, cliffs;
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
// THE HEIGHTS: the shore rises 0.3 steps a tile to the plain at 1 step, the sea falls as gently to 5 steps down; hills
// of up to 3.5 steps inland, from a second noise. (A tile whose corners differ by 1.5 steps is a cliff: slopes stay
// under that but where hills are steepest.)
internal void mg_heights(struct mg *g) {
    for (int y = 0; y <= g->n; y++)
        for (int x = 0; x <= g->n; x++) {
            float f = g->f[y * g->m + x], hn = mg_fbm((float)x * 0.035f + 17, (float)y * 0.035f + 5, g->seed + 10);
            g->h[y * g->m + x] = f < 0 ? fmaxf(f * 0.3f, -5.0f)
                                       : fminf(f * 0.3f, 1.0f) + fmaxf(0, hn - 0.56f) / 0.44f * 3.5f * mg_step(5, 14, f);
        }
}
// THE STARTS: the players by turns on the two continents, a continent's own spread along its length; a start less
// than 10 tiles inside the land moved to the nearest corner within 12 tiles that is (none: the one farthest inside),
// the ground round it levelled to the plain (whole within 7 tiles, fading to 11), its yard (7 tiles) kept free
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
// units walk round it.
internal int mg_river(struct mg *g) {
    float px[256], py[256];
    int x0 = mg_int(g, 0, g->n), y0 = mg_int(g, 0, g->n), len = 0;
    if (mg_f(g, x0, y0) < 8 || mg_start_dist(g, (float)x0, (float)y0) < 13) return 0;
    float x = (float)x0, y = (float)y0, dx = 0, dy = 0;
    for (; len < 250; len++) {
        int ix = (int)floorf(x + 0.5f), iy = (int)floorf(y + 0.5f);
        float f = mg_f(g, ix, iy);
        if (f < -90 || mg_start_dist(g, x, y) < 11) return 0;
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
// THE CLIMATES: along u - winter at the far end, desert at the near one, grassland between, the borders pushed about by noise
internal void mg_climates(struct mg *g) {
    for (int y = 0; y < g->n; y++)
        for (int x = 0; x < g->n; x++) {
            float t = (float)(x - y) / g->half + (mg_fbm((float)x * 0.025f + 90, (float)y * 0.025f + 40, g->seed + 20) - 0.5f) * 0.7f;
            g->clim[y * g->n + x] = (uint8_t)(t < -0.3f ? MG_WINTER : t > 0.3f ? MG_DESERT : MG_GRASS);
        }
}
// THE TEXTURES, by their names in the game's data (db\dbterrain.dat; the scripts' own, Common Terrain Painting.rmv,
// which spells them without the blanks): a climate's five
// for its land, picked by a noise value in the shares its script gives them; its beach for water up to a step deep,
// Underwater below; one under mines, one under trees. A name the data hasn't got: the map's default ground.
internal void mg_textures_load(struct mg *g) {
    static const char *const land[MG_CLIMATES][5] = {
        { "Grass", "Grass - Tufts", "Grass - Tall", "Grass - Patches", "Grass" },
        { "Snow", "Snow - Grass Patches", "Snow - Tufts", "Ice", "Snow" },
        { "Sand - Fine", "Sand - Medium", "Desert", "Grass - Patches", "Sand - Cracked" } };
    static const char *const beach[MG_CLIMATES] = { "Beach", "Ice", "Desert" };
    static const char *const res[MG_CLIMATES] = { "Sand - Medium", "Snow - Dirt Patches", "Pebbles - Small" };
    static const char *const wood[MG_CLIMATES] = { "Grass - Sparse", "Snow - Tufts", "Grass - Sparse" };
    int def = map_get_default_terrain(g->map), missing = 0;
#define MG_TEX(to, name) do { int t_ = terrain_named(name); if (t_ < 0) { missing++; t_ = def; } (to) = t_; } while (0)
    for (int c = 0; c < MG_CLIMATES; c++) {
        for (int k = 0; k < 5; k++) MG_TEX(g->tex_land[c][k], land[c][k]);
        MG_TEX(g->tex_beach[c], beach[c]);
        MG_TEX(g->tex_res[c], res[c]);
        MG_TEX(g->tex_wood[c], wood[c]);
    }
    MG_TEX(g->tex_deep, "Underwater");
#undef MG_TEX
    if (missing) notice("mapgen", "%d of the ground textures the map type \"%s\" names are not in the game's data - the default ground instead", missing, MG_NAME);
}
internal void mg_textures(struct mg *g) {
    // the shares of a climate's five textures, of 100 (the first the plain one)
    static const uint8_t share[MG_CLIMATES][5] = { { 66, 7, 3, 6, 18 }, { 50, 7, 18, 6, 19 }, { 33, 26, 17, 9, 15 } };
    for (int y = 0; y < g->n; y++)
        for (int x = 0; x < g->n; x++) {
            float lo, hi;
            if (!mg_tile_span(g, x, y, &lo, &hi)) continue;
            int c = g->clim[y * g->n + x], tex;
            if (lo < 0) {
                tex = lo > -1.0f ? g->tex_beach[c] : g->tex_deep;
                g->water++;
            } else {
                int p = (int)(mg_fbm((float)x * 0.11f + 300, (float)y * 0.11f + 700, g->seed + 30) * 160.0f - 30.0f), k = 0;   // 0 .. 100, evened out
                p = p < 0 ? 0 : p > 99 ? 99 : p;
                while (k < 4 && p >= share[c][k]) p -= share[c][k++];
                tex = g->tex_land[c][k];
                g->land++;
            }
            map_reset_tile(g->map, (uint32_t)x, (uint32_t)y, (uint32_t)tex);
        }
}

// THE WORLD'S OBJECTS. A tile is free for one when it is land of the plain (its corners 0.3 steps up at least and
// within 0.8 of each other), no cliff and not taken.
internal int mg_free(const struct mg *g, int x, int y) {
    float lo, hi;
    return mg_tile_span(g, x, y, &lo, &hi) && !g->used[y * g->n + x] && lo >= 0.3f && hi - lo < 0.8f && !tile_is_cliff(mg_tile(g, x, y));
}
internal int mg_type(const char *ident) { return unit_type_named(ident, (int)strlen(ident)); }
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
// a mine or a forage patch: the object, its tile and the four beside it the climate's ground for one
internal void mg_mine(struct mg *g, int kind, int x, int y) {
    static const char *const ident[4] = { "res_gold_mine", "res_iron", "res_stone_mine", "res_forage_bush" };
    static const int8_t side[5][2] = { { 0, 0 }, { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    if (!mg_object(g, mg_type(ident[kind]), x, y, 0, kind)) return;
    int c = g->clim[y * g->n + x], tex = kind == MG_FORAGE ? g->tex_wood[c] : g->tex_res[c];
    for (int k = 0; k < 5; k++) {
        int tx = x + side[k][0], ty = y + side[k][1];
        if (!mg_free(g, tx, ty)) continue;
        map_reset_tile(g->map, (uint32_t)tx, (uint32_t)ty, (uint32_t)tex);
        g->used[ty * g->n + tx] = 1;
    }
    mg_spot_add(g, x, y);
}
// a forest: 28 - 60 free tiles grown from a seed tile, a neighbour of a tile it has at a time; a tree on 9 in 10 of
// them, of the seed's climate (a grassland forest of maples or of spruces), the climate's forest floor under all
internal void mg_forest(struct mg *g, int x, int y) {
    static const char *const trees[4][6] = {
        { "tree_01_maple", "tree_02_maple", "tree_04_maple", "tree_30_maple", NULL, NULL },
        { "tree_07_spruce", "tree_09_spruce", "tree_10_spruce", "tree_11_spruce", NULL, NULL },
        { "tree_31_winter_spruce_1", "tree_32_winter_spruce_2", "tree_33_winter_spruce_3", "tree_34_winter_spruce_4", "tree_35_winter_spruce_5", "tree_36_winter_spruce_6" },
        { "tree_21_palm", "tree_22_palm", "tree_23_palm", "tree_26_palm", NULL, NULL } };
    static const int8_t side[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    int c = g->clim[y * g->n + x], set = c == MG_WINTER ? 2 : c == MG_DESERT ? 3 : mg_int(g, 0, 1), kinds = trees[set][4] ? 6 : 4;
    int want = mg_int(g, 28, 60), n = 1, tx[60], ty[60];
    tx[0] = x; ty[0] = y;
    g->used[y * g->n + x] = 1;
    for (int t = 0; n < want && t < want * 8; t++) {
        int from = mg_int(g, 0, n - 1), s = mg_int(g, 0, 3), nx = tx[from] + side[s][0], ny = ty[from] + side[s][1];
        if (!mg_free(g, nx, ny)) continue;
        g->used[ny * g->n + nx] = 1;
        tx[n] = nx; ty[n] = ny; n++;
    }
    for (int i = 0; i < n; i++) {
        map_reset_tile(g->map, (uint32_t)tx[i], (uint32_t)ty[i], (uint32_t)g->tex_wood[c]);
        if (mg_int(g, 0, 9)) mg_object(g, mg_type(trees[set][mg_int(g, 0, kinds - 1)]), tx[i], ty[i], mg_rnd(g) * 6.2831853f, MG_TREE);
    }
    mg_spot_add(g, x, y);
}
// a herd: 3 - 6 animals of one kind of the climate on free tiles within 2 of a spot (they walk: the tiles stay free)
internal void mg_herd(struct mg *g, int x, int y) {
    static const char *const animals[MG_CLIMATES][5] = {
        { "res_deer", "res_goat", "res_horse", "res_elephant", "res_giraffe" },
        { "res_deer", "res_goat", "res_walrus", "res_deer", "res_goat" },
        { "res_giraffe", "res_elephant", "res_ostrich", "res_goat", "res_horse" } };
    int type = mg_type(animals[g->clim[y * g->n + x]][mg_int(g, 0, 4)]), want = mg_int(g, 3, 6);
    for (int t = 0, n = 0; n < want && t < 30; t++) {
        int ax = x + mg_int(g, -2, 2), ay = y + mg_int(g, -2, 2);
        if (mg_free(g, ax, ay) && mg_object(g, type, ax, ay, mg_rnd(g) * 6.2831853f, MG_ANIMAL)) n++;
    }
    mg_spot_add(g, x, y);
}
// each player's own round its start (2 forage patches, 2 gold, 2 iron, 2 stone, 3 forests, a herd - the mines far
// enough out to want a Settlement), then the land's: by its size, away from the starts
internal void mg_objects(struct mg *g) {
    static const struct { int8_t kind, n, lo, hi; } own[4] = { { MG_FORAGE, 2, 8, 11 }, { MG_GOLD, 2, 13, 16 }, { MG_IRON, 2, 14, 18 }, { MG_STONE, 2, 15, 20 } };
    int x, y;
    for (int i = 0; i < g->np; i++) {
        for (int k = 0; k < 4; k++)
            for (int j = 0; j < own[k].n; j++)
                if (mg_ring(g, g->sx[i], g->sy[i], own[k].lo, own[k].hi, 5, &x, &y)) mg_mine(g, own[k].kind, x, y);
        for (int j = 0; j < 3; j++)
            if (mg_ring(g, g->sx[i], g->sy[i], 10, 15, 6, &x, &y)) mg_forest(g, x, y);
        if (mg_ring(g, g->sx[i], g->sy[i], 11, 16, 6, &x, &y)) mg_herd(g, x, y);
    }
    static const struct { int8_t kind; int16_t tiles; } wild[4] = { { MG_GOLD, 800 }, { MG_IRON, 800 }, { MG_STONE, 1300 }, { MG_FORAGE, 1500 } };
    for (int k = 0; k < 4; k++)
        for (int j = g->land / wild[k].tiles; j > 0; j--)
            if (mg_anywhere(g, 22, 9, &x, &y)) mg_mine(g, wild[k].kind, x, y);
    for (int j = g->land / 380; j > 0; j--)
        if (mg_anywhere(g, 16, 7, &x, &y)) mg_forest(g, x, y);
    for (int j = g->land / 1200; j > 0; j--)
        if (mg_anywhere(g, 18, 8, &x, &y)) mg_herd(g, x, y);
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
        g->used = calloc((size_t)(n * n), 1);
        g->clim = calloc((size_t)(n * n), 1);
        z = calloc((size_t)(g->m * g->m), sizeof *z);
    }
    if (!g || !g->f || !g->h || !g->used || !g->clim || !z || g->np < 1 || !g->world) {
        notice("mapgen", "the map type \"%s\" could not be made (%d players, %d tiles a side) - an empty map instead", MG_NAME, count - 1, n);
        start[0] = start[1] = (float)n * 0.5f; start[2] = 0;
        return 0;
    }
    mg_land(g);
    mg_heights(g);
    mg_starts(g);
    for (int want = 2 + n / 60, t = 0; g->rivers < want && t < 4000; t++) g->rivers += mg_river(g);
    float step = map_get_height_step(map);
    for (int i = 0; i < g->m * g->m; i++) {
        if (g->h[i] > -0.05f && g->h[i] < 0) g->h[i] = 0;   // no film of water
        z[i] = g->h[i] * step;
    }
    map_ground_set(map, z);
    mg_climates(g);
    mg_textures_load(g);
    mg_textures(g);
    map_cliff_pass(map, 0, (uint32_t)n, 0, n);
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
        fprintf(g_log, "# mapgen: \"%s\" made - seed %u, %d tiles a side, %d land and %d water tiles, %d cliff tiles, %d rivers, %d players (%d start units) at",
                MG_NAME, seed, n, g->land, g->water, g->cliffs, g->rivers, g->np, units);
        for (int i = 0; i < g->np; i++) fprintf(g_log, " %d,%d", g->sx[i], g->sy[i]);
        fprintf(g_log, "; gold %d iron %d stone %d forage %d fish %d trees %d animals %d; the view from %.1f %.1f; the map's flags %u\n", g->made[MG_GOLD],
                g->made[MG_IRON], g->made[MG_STONE], g->made[MG_FORAGE], g->made[MG_FISH], g->made[MG_TREE], g->made[MG_ANIMAL], (double)start[0], (double)start[1], map_get_flags(map));
        LeaveCriticalSection(&g_lock);
    }
    free(z); free(g->f); free(g->h); free(g->used); free(g->clim); free(g);
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
