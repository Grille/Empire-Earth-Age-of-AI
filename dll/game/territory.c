// TERRITORIES: the map split into parts an AI can reason about (the player 2026-11-13: "its hard to make the ai decide
// based on the whole map, so i had an idea, what about spliting the map into teretories? in the begining all
// landmasses (cliffs imediatly substracted and used as split) and bodies of water are collected, than when a teretory
// is larger than 200 tiles it gets split in half ... than rate them on value: amount of resources, how many border
// tiles to unclaimed teretories (harder to hold) ... its a system for the ai to reason about the world, players never
// see or interact with it" - and of the size: "the 200 where a rough guess, well see the true size that makes sence").
// READ ONLY so far: nothing of the AI asks it yet. It is built when someone looks - the overlay's layer "territories",
// the pipe's territories, [autotest] territories_log - once a map, of the terrain and the trees as they stand then.
// How it is split:
//   the tiles - land (the terrain a citizen walks on, inside the map's diamond: a ford of shallow water too), water
//               (the terrain a ship sails), neither (cliffs and the like: in no territory, they part what they part).
//               A tree's tile is land that counts a wall for the split and joins the territory nearest to it after
//               (a wood is a wall until it is cut; the borders stay where they were drawn);
//   clearance - a tile's distance to the nearest tile that is not open ground of its kind (ter_clear: the widest
//               place of a valley is its greatest clearance, a pass its smallest on the way through);
//   the narrow places - the tiles taken from the widest down, each joining the region beside it; where two regions
//               meet, the place is a border when it is narrower than g_ter_ratio of the smaller one's widest place,
//               else they are one (ter_shed: a pass, a ford, a gap between cliffs or woods, a strait);
//   the small - a region under 1/8 of the size joins the neighbour it shares the longest border with (ter_join);
//   the mines - (after the large) a mine and the open land within TER_GROUND steps of it are one territory's, mines
//               side by side one field in the territory most of them stand in (ter_mines); what that parts from
//               its territory joins a neighbour (ter_whole);
//   the large - one of more than size x size open tiles (water: g_ter_water times as far across), or longer than
//               TER_LONG sizes from end to end (a strip of coast, a long valley), is split in halves
//               until none is, each cut the shortest one near the middle between the region's two ends (ter_split:
//               a narrowing where there is one - on open flatland nothing else can part it).
// What a territory knows: its tiles, its middle, its widest place, its neighbours with the length of each border
// (open tiles side by side: a border through a wood is none), its coast; and the facts as they are (ter_facts: the
// resource sites left, each player's buildings and units, who holds it) - what stands there, not what a player has
// seen: an AI that decides by it has to ask its own intel.
// Seen 2026-11-13 (random maps, 4 AIs, pictured by scripts/results/territories_png.py from the pipe's dump): a huge
// Continental (320 tiles, 34550 of land) is 88 land territories at size 24, 33 at 40 (531 to 1522 tiles, the middle one
// 937, 84 tiles of border each), 19 at 56; a large Small Islands (210) at 40 its 9 islands whole (436 to 1130 tiles;
// at 32 the larger ones were halved) and its sea in 5; a large Highlands 23, a plateau among cliffs one of its own with
// no border. The woods matter: without the trees the narrow places make 16 regions of the Highlands map, with them
// 68. Built in 6 ms (210) to 33 ms (320). The default 40 is where an island of that map is one territory - the size
// is the player's to settle by looking (the pipe: territories size n).
#define TER_OFF 0
#define TER_LAND 1
#define TER_WATER 2
#define TER_TREE 4                // with TER_LAND: a tree stood there at the build
#define TER_NONE 0xFFFF
#define TER_MAX 60000             // territories kept (a tile's is a uint16)
#define TER_TINY 12               // a region of fewer tiles when it meets another is no side of a border
#define TER_NOOK 15               // nor one whose widest place is under 3 tiles (clearance, 5 a tile)
#define TER_FACTS_MS 5000
#define TER_LINES 30000           // border lines drawn at most
#define TER_FAR 0x3FFFFFFF        // a way not found
#define TER_GROUND 5              // a mine's ground: the open land within this many steps of it (ter_mines)
#define TER_MINE 6                // a cut keeps this many tiles from a mine (stone, gold, iron) where it can
#define TER_MINE_COST 15          // ... a mine nearer counts as so many tiles of the cut's length
#define TER_LONG 3                // a territory whose two ends lie more than this many sizes apart (the way between them) is split too
internal int g_ter_size = 40;     // tiles across: more than size x size open tiles and it is split
internal int g_ter_ratio = 70;    // percent: a narrow place parts two regions when narrower than this share of the smaller one's widest place
internal int g_ter_trees = 1;     // trees are walls for the split
internal int g_ter_water = 2;     // water: this many times as far across before it is split
internal int g_ter_field = 8;     // mines this many steps apart at most are one field (ter_mines)
struct ter_terr {
    uint8_t water;                // 0 land, 1 water
    int area, trees;              // its open tiles; its tiles under trees
    ivec2 c;                      // its middle: the open tile nearest its tiles' mean
    int wide;                     // its widest place: the greatest clearance, in tiles
    int border, coast;            // open tiles side by side with a territory of its kind / of the other kind (pairs)
    int nb0, nbn;                 // its neighbours: g_ter.nb[nb0 .. nb0 + nbn)
    // the facts (ter_facts)
    short res[7]; int amount[7];  // resource sites with something left by resource (2 food .. 6 iron), what they hold
    short bld[16], units[16];     // per player: buildings, other units
    int8_t owner, contested;      // the player with the most buildings (0 none); an enemy of it has buildings there too
};
struct ter_nb { uint16_t to; int len; };
struct ter_site { short x, y; uint8_t res; };
internal struct {
    ee_map *map; int size;
    unsigned version;             // counted up with each build
    uint8_t *cls;                 // per tile: TER_LAND, TER_WATER (| TER_TREE), TER_OFF
    uint16_t *clr;                // per open tile: its clearance, 5 a tile (7 a diagonal step)
    uint16_t *lab;                // per tile: its territory, TER_NONE
    struct ter_terr *t; int n, n_land, n_water;
    struct ter_nb *nb; int nnb;
    struct ter_site *site; int nsite;
    int p_size, p_ratio, p_trees, p_water, p_field;   // what it was built with
    int tiles[3], tree_tiles;     // tiles by kind
    int moved, mended, pieces;    // the log's: tiles the mines' ground moved to another territory, parted pieces given to a neighbour, territories still in more than one piece
    int shed, joined, split, lost, dropped;   // the log's: regions the narrow places made, small ones joined, parts the size added, regions past TER_MAX, tiles of regions too small to be one
    unsigned build_ms;
    uint32_t facts_t, log_t, built_at;   // built_at: the game time of the build
    int shown; unsigned drawn; int lines;
    uint32_t drawn_owners;        // the holders the borders were drawn with (ter_owners)
} g_ter;
struct ter_work {
    int size, n;
    uint8_t *cls; uint16_t *d;
    uint8_t *mine;                // per tile: a mine stands there (stone, gold, iron)
    int *tl;                      // per tile: its region while they form, -1 none
    int *ord;                     // tiles in an order (ter_shed: the widest first; ter_split: by region)
    int *up, *peak, *rsz;         // per region (ter_shed): the one it joined, its widest place, its tiles
    int nreg, cap;
    int *dist, *sub;              // ter_split: per tile its ways to the region's two ends (then: how much nearer to the first; a queue)
    uint64_t *heap; int hn, hcap;
};
struct ter_edge { int a, b, len; };   // regions a < b, open tiles of theirs side by side

// a ship of the loaded data (what it sails on is water)
internal ee_unit_type *ter_ship_type(void) {
    for (int t = 0; t < G_TYPES_N; t++) {
        ee_unit_type *td = type_data(t);
        if (td && unit_type_get_class_id(td) == CLS_NAVAL && unit_type_terrain_ok(td, 1) && !unit_type_terrain_ok(td, 0)) return td;
    }
    return NULL;
}
// the tiles' kinds, the trees and the resource sites as they stand
internal void ter_tiles(ee_map *map, struct ter_work *w) {
    int size = w->size;
    ee_unit_type *walker = type_data(UT(CITIZEN)), *ship = ter_ship_type();
    ee_tile **tiles = map_get_tiles(map);
    int e1 = map_get_edge_sum(map), e2 = map_get_edge_sum_2(map), ed = map_get_edge_diff(map);
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++) {
            int i = y * size + x;
            ee_tile *tile = tiles[i];
            if (!tile || !walker || x + y <= e1 || x + y >= e2 || y - x >= ed || x - y >= ed) continue;
            int kind = tile_get_kind(tile);
            if (unit_type_terrain_ok(walker, kind)) w->cls[i] = TER_LAND;
            else if (ship ? unit_type_terrain_ok(ship, kind) : kind == 1) w->cls[i] = TER_WATER;
            else continue;
            for (void *node = tile_get_objects(tile); node; node = tilenode_get_next(node)) {
                ee_unit *o = tilenode_get_object(node);
                ee_unit_type *otd = o ? unit_get_type_data(o) : NULL;
                ee_unit_stats *st = o ? unit_get_stats(o) : NULL;
                int r = otd ? unit_type_get_resource_type(otd) : 0;
                if (r < 2 || r > 6 || (st && unit_stats_get_speed(st) > 0)) continue;   // animals walk: no site
                if (r == 3) { if (g_ter_trees && w->cls[i] == TER_LAND) w->cls[i] = TER_LAND | TER_TREE; continue; }
                if (unit_get_x(o) != x || unit_get_y(o) != y) continue;   // a site of several tiles: once, at its own
                if (unit_get_owner(o) && player_get_index(unit_get_owner(o)) > 0) continue;   // a player's farm is no site of the map
                if (!(g_ter.nsite & 255)) {
                    struct ter_site *ns = realloc(g_ter.site, (size_t)(g_ter.nsite + 256) * sizeof *ns);
                    if (!ns) continue;
                    g_ter.site = ns;
                }
                if (r >= 4) w->mine[i] = 1;
                g_ter.site[g_ter.nsite].x = (short)x; g_ter.site[g_ter.nsite].y = (short)y; g_ter.site[g_ter.nsite++].res = (uint8_t)r;
            }
        }
}
// clearance of the open tiles of kind k: two passes, 5 a step and 7 a diagonal one, from every tile that is not one
internal void ter_clear(struct ter_work *w, int k) {
    int size = w->size;
    const uint8_t *cls = w->cls;
    uint16_t *d = w->d;
    #define TER_N(dx, dy, c) { int nx = x + (dx), ny = y + (dy); \
        unsigned nv = nx >= 0 && ny >= 0 && nx < size && ny < size && cls[ny * size + nx] == k ? d[ny * size + nx] : 0u; \
        if (nv + (c) < v) v = nv + (c); }
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++) {
            if (cls[y * size + x] != k) continue;
            unsigned v = 0xFFFF;
            TER_N(-1, 0, 5) TER_N(0, -1, 5) TER_N(-1, -1, 7) TER_N(1, -1, 7)
            d[y * size + x] = (uint16_t)v;
        }
    for (int y = size - 1; y >= 0; y--)
        for (int x = size - 1; x >= 0; x--) {
            if (cls[y * size + x] != k) continue;
            unsigned v = d[y * size + x];
            TER_N(1, 0, 5) TER_N(0, 1, 5) TER_N(1, 1, 7) TER_N(-1, 1, 7)
            d[y * size + x] = (uint16_t)v;
        }
    #undef TER_N
}
// a step from tile (x, y) in direction dd among the open tiles of kind k: the tile it reaches, -1 none (a diagonal
// step only past two open tiles, as a unit squeezes through - the reach map's rule)
internal int ter_step(const struct ter_work *w, int x, int y, int dd, int k) {
    int size = w->size, nx = x + k_rm_dx[dd], ny = y + k_rm_dy[dd];
    if (nx < 0 || ny < 0 || nx >= size || ny >= size || w->cls[ny * size + nx] != k) return -1;
    if ((dd & 1) && (w->cls[y * size + nx] != k || w->cls[ny * size + x] != k)) return -1;
    return ny * size + nx;
}
// the narrow places: the open tiles of kind k from the widest down, each into the region of its widest neighbour (a
// new one when none is taken yet); two regions that meet are one unless the place parts them
internal void ter_shed(struct ter_work *w, int k) {
    int size = w->size, n = w->n, m = 0;
    unsigned maxd = 0;
    for (int i = 0; i < n; i++) if (w->cls[i] == k && w->d[i] > maxd) maxd = w->d[i];
    int *cnt = calloc((size_t)maxd + 2, sizeof *cnt);
    if (!cnt) return;
    for (int i = 0; i < n; i++) if (w->cls[i] == k) cnt[maxd - w->d[i] + 1]++;
    for (unsigned v = 1; v <= maxd + 1; v++) cnt[v] += cnt[v - 1];
    for (int i = 0; i < n; i++) if (w->cls[i] == k) { w->ord[cnt[maxd - w->d[i]]++] = i; m++; }
    free(cnt);
    for (int q = 0; q < m; q++) {
        int i = w->ord[q], x = i % size, y = i / size, best = -1, bd = -1, roots[8], nr = 0;
        for (int dd = 0; dd < 8; dd++) {
            int j = ter_step(w, x, y, dd, k);
            if (j < 0 || w->tl[j] < 0) continue;
            int r = rm_find(w->up, w->tl[j]), seen = 0;
            if ((int)w->d[j] > bd) { bd = w->d[j]; best = r; }
            for (int s = 0; s < nr; s++) seen |= roots[s] == r;
            if (!seen) roots[nr++] = r;
        }
        if (best < 0) {
            int r;
            if (w->nreg < w->cap) { r = w->nreg++; w->up[r] = r; w->peak[r] = w->d[i]; w->rsz[r] = 0; }
            else r = rm_find(w->up, w->cap - 1);   // more regions than room (never seen): they share the last
            w->rsz[r]++; w->tl[i] = r;
            continue;
        }
        w->tl[i] = best; w->rsz[best]++;
        for (int s = 0; s < nr; s++) {
            int a = rm_find(w->up, best), b = rm_find(w->up, roots[s]);
            if (a == b) continue;
            int lo = w->peak[a] < w->peak[b] ? a : b, hi = lo == a ? b : a;
            if ((int)w->d[i] * 100 < g_ter_ratio * w->peak[lo] && w->rsz[lo] >= TER_TINY && w->peak[lo] >= TER_NOOK) continue;   // a border
            w->up[lo] = hi; w->rsz[hi] += w->rsz[lo];
        }
    }
}
internal int ter_int_cmp(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }
internal int ter_edge_cmp(const void *a, const void *b) {
    uint64_t x = *(const uint64_t *)a, y = *(const uint64_t *)b;
    return x < y ? -1 : x > y;
}
// the regions' borders: every pair of regions with open tiles side by side and how many (sorted by a, then b); the
// count, *out the list (the caller frees it)
internal int ter_edges(const struct ter_work *w, struct ter_edge **out) {
    int size = w->size, nk = 0, cap = 0, ne = 0;
    uint64_t *key = NULL;
    *out = NULL;
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++) {
            int i = y * size + x, a = w->tl[i];
            if (a < 0 || (w->cls[i] & TER_TREE)) continue;
            for (int s = 0; s < 2; s++) {
                int j = s ? (y + 1 < size ? i + size : -1) : (x + 1 < size ? i + 1 : -1);
                int b = j >= 0 && !(w->cls[j] & TER_TREE) ? w->tl[j] : -1;
                if (b < 0 || b == a) continue;
                if (nk == cap) {
                    uint64_t *nkey = realloc(key, (size_t)(cap = cap ? cap * 2 : 4096) * sizeof *key);
                    if (!nkey) { free(key); return 0; }
                    key = nkey;
                }
                key[nk++] = (uint64_t)(unsigned)(a < b ? a : b) << 32 | (unsigned)(a < b ? b : a);
            }
        }
    if (!nk) { free(key); return 0; }
    qsort(key, (size_t)nk, sizeof *key, ter_edge_cmp);
    struct ter_edge *e = malloc((size_t)nk * sizeof *e);
    if (!e) { free(key); return 0; }
    for (int q = 0; q < nk; q++) {
        if (q && key[q] == key[q - 1]) { e[ne - 1].len++; continue; }
        e[ne].a = (int)(key[q] >> 32); e[ne].b = (int)(key[q] & 0xFFFFFFFFu); e[ne++].len = 1;
    }
    free(key);
    *out = e;
    return ne;
}
// the regions' open tiles and kinds (area, kind: nreg each)
internal void ter_areas(const struct ter_work *w, int *area, uint8_t *kind) {
    memset(area, 0, (size_t)w->nreg * sizeof *area);
    for (int i = 0; i < w->n; i++)
        if (w->tl[i] >= 0 && !(w->cls[i] & TER_TREE)) { area[w->tl[i]]++; kind[w->tl[i]] = w->cls[i]; }
}
internal int ter_max_area(int kind) { int s = kind == TER_WATER ? g_ter_size * g_ter_water : g_ter_size; return s * s; }
internal int ter_max_way(int kind) { return TER_LONG * (kind == TER_WATER ? g_ter_size * g_ter_water : g_ter_size); }
// the small: a region under 1/8 of its kind's size joins the neighbour of its kind it shares the longest border with,
// round after round until none is left that has one (a pond, an islet stay what they are); what is then still under
// TER_TINY tiles is no territory - a tile or two of ground among cliffs or trees, a puddle
internal void ter_join(struct ter_work *w) {
    int *area = malloc((size_t)w->nreg * sizeof *area), *to = malloc((size_t)w->nreg * sizeof *to), *len = malloc((size_t)w->nreg * sizeof *len);
    uint8_t *kind = calloc((size_t)w->nreg, 1), *busy = malloc((size_t)w->nreg);
    for (int round = 0; round < 64 && area && to && len && kind && busy; round++) {
        struct ter_edge *e;
        int ne = ter_edges(w, &e), joined = 0;
        ter_areas(w, area, kind);
        for (int r = 0; r < w->nreg; r++) { to[r] = -1; len[r] = 0; busy[r] = 0; }
        for (int q = 0; q < ne; q++) {
            int a = e[q].a, b = e[q].b;
            if (kind[a] != kind[b]) continue;
            if (area[a] * 8 < ter_max_area(kind[a]) && e[q].len > len[a]) { len[a] = e[q].len; to[a] = b; }
            if (area[b] * 8 < ter_max_area(kind[b]) && e[q].len > len[b]) { len[b] = e[q].len; to[b] = a; }
        }
        free(e);
        for (int r = 0; r < w->nreg; r++) {
            if (to[r] < 0 || busy[r] || busy[to[r]]) { if (to[r] >= 0) to[r] = -1; continue; }   // one join a region a round
            busy[r] = busy[to[r]] = 1; joined++;
        }
        if (!joined) break;
        for (int i = 0; i < w->n; i++) if (w->tl[i] >= 0 && to[w->tl[i]] >= 0) w->tl[i] = to[w->tl[i]];
        g_ter.joined += joined;
    }
    if (area && kind) {
        ter_areas(w, area, kind);
        for (int i = 0; i < w->n; i++) if (w->tl[i] >= 0 && area[w->tl[i]] < TER_TINY) { w->tl[i] = -1; g_ter.dropped++; }
    }
    free(area); free(to); free(len); free(kind); free(busy);
}
internal void ter_push(struct ter_work *w, uint64_t v) {
    if (w->hn == w->hcap) {
        uint64_t *nh = realloc(w->heap, (size_t)(w->hcap ? w->hcap * 2 : 8192) * sizeof *nh);
        if (!nh) return;
        w->heap = nh; w->hcap = w->hcap ? w->hcap * 2 : 8192;
    }
    int c = w->hn++;
    while (c > 0 && w->heap[(c - 1) / 2] > v) { w->heap[c] = w->heap[(c - 1) / 2]; c = (c - 1) / 2; }
    w->heap[c] = v;
}
internal uint64_t ter_pop(struct ter_work *w) {
    uint64_t top = w->heap[0], v = w->heap[--w->hn];
    int c = 0;
    for (;;) {
        int k = 2 * c + 1;
        if (k >= w->hn) break;
        if (k + 1 < w->hn && w->heap[k + 1] < w->heap[k]) k++;
        if (w->heap[k] >= v) break;
        w->heap[c] = w->heap[k]; c = k;
    }
    w->heap[c] = v;
    return top;
}
// the ways from the tiles pushed, inside region r (kind k): a tile's way taken where shorter than what it has (5 a
// step, 7 a diagonal one)
internal void ter_ways(struct ter_work *w, int r, int k, int *dist) {
    int size = w->size;
    while (w->hn) {
        uint64_t top = ter_pop(w);
        int i = (int)(top & 0xFFFFFFFFu), dv = (int)(top >> 32);
        if (dv > dist[i]) continue;
        for (int dd = 0; dd < 8; dd++) {
            int j = ter_step(w, i % size, i / size, dd, k), nd = dv + ((dd & 1) ? 7 : 5);
            if (j < 0 || w->tl[j] != r || nd >= dist[j]) continue;
            dist[j] = nd;
            ter_push(w, (uint64_t)(unsigned)nd << 32 | (unsigned)j);
        }
    }
}
// ... from one tile, all of the region's tiles t[0 .. m) first set unreached; the tile with the longest way
internal int ter_ways_from(struct ter_work *w, int r, int k, const int *t, int m, int from, int *dist) {
    int last = from;
    for (int q = 0; q < m; q++) dist[t[q]] = TER_FAR;
    dist[from] = 0;
    w->hn = 0; ter_push(w, (unsigned)from);
    ter_ways(w, r, k, dist);
    for (int q = 0; q < m; q++) if (dist[t[q]] > dist[last]) last = t[q];
    return last;
}
// the large: region r (kind k, its open tiles t[0 .. m)) of more than max tiles, or with its two ends more than way
// tiles of way apart (a strip of sea round a continent: 9000 tiles 15 wide and 600 long was one, under the size by its
// tiles alone), in halves, and a half still too large again
// (the player: "when a teretory is larger ... it gets split in half, continue untile evrething is under"). Its two
// ends - the tile farthest from its first one, and the one farthest from that; every tile by how much nearer it is to
// the one end than to the other; the cut where the fewest tiles lie side by side across it, among the cuts that leave
// each half 35 to 65 in a hundred of the tiles: the narrowest place near the middle - a mine within TER_MINE tiles of
// a cut counting TER_MINE_COST tiles of its length (the player 2026-11-13, of the first pictures: "mines end up on
// borders sometimes" - a mine on a border is its miners 3 tiles from ground nobody guards, and a territory's worth
// that needs the neighbour held too; of 56 mines of a huge Continental 26 lay within 3 tiles of another territory,
// about what chance gives). A mine in a pass stays on its border: there the map says so. What a cut parts from its own
// end's half goes to the other. The second half is a new region; t is put in order, the first half's tiles first.
internal void ter_split(struct ter_work *w, int r, int k, int *t, int m, int max, int way) {
    enum { CUTS = 13 };
    int size = w->size, th[CUTS], cut[CUTS] = { 0 }, mines[CUTS] = { 0 };
    if (m < 2 || w->nreg >= w->cap) return;
    int a = ter_ways_from(w, r, k, t, m, t[0], w->dist);
    int b = ter_ways_from(w, r, k, t, m, a, w->dist);
    if (m <= max && (w->dist[b] == TER_FAR || w->dist[b] <= way * 5)) return;
    int *v = malloc((size_t)m * sizeof *v);
    if (!v) return;
    ter_ways_from(w, r, k, t, m, b, w->sub);
    for (int q = 0; q < m; q++) {   // nearer to a: below; a tile neither end reaches (never seen) with b's
        int i = t[q];
        w->dist[i] = w->dist[i] == TER_FAR || w->sub[i] == TER_FAR ? TER_FAR : w->dist[i] - w->sub[i];
        v[q] = w->dist[i];
    }
    qsort(v, (size_t)m, sizeof *v, ter_int_cmp);
    for (int c = 0; c < CUTS; c++) th[c] = v[(long long)m * (70 + 5 * c) / 200];
    free(v);
    for (int q = 0; q < m; q++) {
        int i = t[q], x = i % size, y = i / size;
        // a step towards the other end is 10 less of the one way and more of the other: TER_MINE tiles from the cut
        for (int c = 0; c < CUTS && w->mine[i] && w->dist[i] != TER_FAR; c++) mines[c] += abs(w->dist[i] - th[c]) < 10 * TER_MINE;
        for (int s = 0; s < 2; s++) {
            int j = ter_step(w, x, y, s ? 2 : 0, k);
            if (j < 0 || w->tl[j] != r) continue;
            int lo = w->dist[i] < w->dist[j] ? w->dist[i] : w->dist[j], hi = w->dist[i] < w->dist[j] ? w->dist[j] : w->dist[i];
            for (int c = 0; c < CUTS; c++) cut[c] += lo < th[c] && hi >= th[c];
        }
    }
    int best = CUTS / 2;   // the middle one unless another is shorter
    for (int c = 0; c < CUTS; c++) cut[c] += TER_MINE_COST * mines[c];
    for (int c = 0; c < CUTS; c++) if (cut[c] < cut[best] || (cut[c] == cut[best] && abs(c - CUTS / 2) < abs(best - CUTS / 2))) best = c;
    // a's half: what its end reaches below the cut (the rest is b's)
    int nb = w->nreg++, h = 0, n = 0, ma = 0;
    int *queue = w->sub;
    for (int q = 0; q < m; q++) w->tl[t[q]] = nb;
    if (w->dist[a] < th[best]) { w->tl[a] = r; queue[n++] = a; }
    while (h < n) {
        int i = queue[h++];
        for (int dd = 0; dd < 8; dd++) {
            int j = ter_step(w, i % size, i / size, dd, k);
            if (j < 0 || w->tl[j] != nb || w->dist[j] >= th[best]) continue;
            w->tl[j] = r; queue[n++] = j;
        }
    }
    for (int q = 0, e = m - 1; q <= e; ) {   // a's tiles first
        if (w->tl[t[q]] == r) { q++; ma++; continue; }
        int x = t[q]; t[q] = t[e]; t[e--] = x;
    }
    g_ter.split++;
    if (!ma || ma == m) return;   // no cut (never seen): as it is
    ter_split(w, r, k, t, ma, max, way);
    ter_split(w, nb, k, t + ma, m - ma, max, way);
}
internal void ter_split_all(struct ter_work *w) {
    int nreg = w->nreg;
    int *area = malloc((size_t)nreg * sizeof *area), *start = calloc((size_t)nreg + 1, sizeof *start);
    uint8_t *kind = calloc((size_t)nreg, 1);
    if (!area || !start || !kind) { free(area); free(start); free(kind); return; }
    ter_areas(w, area, kind);
    for (int r = 0; r < nreg; r++) start[r + 1] = start[r] + area[r];
    for (int i = 0; i < w->n; i++) if (w->tl[i] >= 0 && !(w->cls[i] & TER_TREE)) w->ord[start[w->tl[i]]++] = i;
    for (int r = nreg - 1; r > 0; r--) start[r] = start[r - 1];   // the fill moved each start to its end
    start[0] = 0;
    for (int r = 0; r < nreg; r++) {
        if (area[r]) ter_split(w, r, kind[r], w->ord + start[r], area[r], ter_max_area(kind[r]), ter_max_way(kind[r]));
    }
    free(area); free(start); free(kind);
}
// the mines' ground: a mine and the ground round it are one territory's (the player 2026-11-13, of the first pictures:
// "mines end up on borders sometimes" - with territories some 30 tiles across a fifth to a third of all ground lies
// within 3 tiles of a border, and so did the mines: 8-21 of 50-56 a map, on foot; its miners and its drop site would
// stand beside ground nobody of theirs guards, and its worth would need the neighbour held too). The open land within
// TER_GROUND steps of a mine (stone, gold, iron) is the ground of the mine nearest to it; mines within g_ter_field steps
// of each other are one field (the gold and the iron of one site), and a field's ground all goes to the territory
// most of its mines stand in - the border moves round the field. Measured on five random maps (116 to 320 tiles,
// 37-56 mines each; mines with another territory within 3 steps on foot): without this step 5-21 a map; fields of
// mines 4 steps apart 6, 6, 1, 0, 4; 6 apart 1, 0, 1, 0, 0; 8 apart none, 211-530 tiles moved a map; all whose grounds
// touch (11) none either, but on a 116-tile map with 50 mines the fields chained across it and one territory took
// 1597 of 3979 land tiles. So 8 (the pipe: territories field n).
internal void ter_mines(struct ter_work *w) {
    int size = w->size, nm = 0, h = 0, t = 0;
    for (int i = 0; i < w->n; i++) { w->sub[i] = -1; nm += w->mine[i] && w->cls[i] == TER_LAND && w->tl[i] >= 0; }
    int *up = malloc((size_t)(nm + 1) * sizeof *up), *terr = malloc((size_t)(nm + 1) * sizeof *terr), *to = malloc((size_t)(nm + 1) * sizeof *to);
    if (!nm || !up || !terr || !to) { free(up); free(terr); free(to); return; }
    for (int i = 0, k = 0; i < w->n; i++)
        if (w->mine[i] && w->cls[i] == TER_LAND && w->tl[i] >= 0) { up[k] = k; terr[k] = w->tl[i]; w->sub[i] = k++; w->dist[i] = 0; w->ord[t++] = i; }
    while (h < t) {
        int i = w->ord[h++];
        for (int dd = 0; dd < 8; dd++) {
            int j = ter_step(w, i % size, i / size, dd, TER_LAND);
            if (j < 0 || w->tl[j] < 0) continue;
            if (w->sub[j] >= 0) {   // two mines' grounds meet: one field when the mines are g_ter_field steps apart at most
                int a = rm_find(up, w->sub[i]), b = rm_find(up, w->sub[j]);
                if (a != b && w->dist[i] + w->dist[j] + 1 <= g_ter_field) up[a] = b;
                continue;
            }
            if (w->dist[i] >= TER_GROUND) continue;
            w->sub[j] = w->sub[i]; w->dist[j] = w->dist[i] + 1; w->ord[t++] = j;
        }
    }
    for (int k = 0; k < nm; k++) to[k] = -1;
    for (int k = 0; k < nm; k++) {   // each field's territory: where most of its mines stand (the first of those that tie)
        int c = rm_find(up, k), most = 0;
        if (to[c] >= 0) continue;
        for (int a = 0; a < nm; a++) {
            int cnt = 0;
            if (rm_find(up, a) != c) continue;
            for (int b = 0; b < nm; b++) cnt += rm_find(up, b) == c && terr[b] == terr[a];
            if (cnt > most) { most = cnt; to[c] = terr[a]; }
        }
    }
    for (int q = 0; q < t; q++) {
        int i = w->ord[q], want = to[rm_find(up, w->sub[i])];
        if (want >= 0 && w->tl[i] != want) { w->tl[i] = want; g_ter.moved++; }
    }
    free(up); free(terr); free(to);
}
// a territory is one piece: what a cut or a mine's ground parted from a region's largest piece joins the neighbour of
// its kind it shares the longest border with (seen: 1-4 territories a map in two pieces after ter_mines - a neck the
// ground was taken through); a piece with no neighbour is a region of its own, one under TER_TINY tiles none. Twice:
// a piece may have joined another's lesser piece.
struct ter_piece { int size, terr, nb[6], len[6]; };
internal void ter_whole(struct ter_work *w) {
    int size = w->size;
    for (int pass = 0; pass < 2; pass++) {
        struct ter_piece *pc = NULL;
        int np = 0, cap = 0, fixed = 0;
        int *big = malloc((size_t)(w->nreg + 1) * sizeof *big);
        if (!big) return;
        for (int r = 0; r < w->nreg; r++) big[r] = -1;
        for (int i = 0; i < w->n; i++) w->sub[i] = -1;
        for (int i = 0; i < w->n; i++) {
            if (w->tl[i] < 0 || w->sub[i] >= 0 || (w->cls[i] & TER_TREE)) continue;
            if (np == cap) {
                struct ter_piece *npc = realloc(pc, (size_t)(cap = cap ? cap * 2 : 256) * sizeof *pc);
                if (!npc) { free(pc); free(big); return; }
                pc = npc;
            }
            int h = 0, t = 0;
            memset(&pc[np], 0, sizeof pc[np]);
            pc[np].terr = w->tl[i];
            w->sub[i] = np; w->ord[t++] = i;
            while (h < t) {
                int c = w->ord[h++];
                for (int dd = 0; dd < 8; dd++) {
                    int j = ter_step(w, c % size, c / size, dd, w->cls[i]);
                    if (j < 0 || w->sub[j] >= 0 || w->tl[j] != w->tl[i]) continue;
                    w->sub[j] = np; w->ord[t++] = j;
                }
            }
            pc[np].size = t;
            if (big[w->tl[i]] < 0 || t > pc[big[w->tl[i]]].size) big[w->tl[i]] = np;
            np++;
        }
        for (int i = 0; i < w->n; i++) {   // the lesser pieces' borders
            int c = w->tl[i] >= 0 && !(w->cls[i] & TER_TREE) ? w->sub[i] : -1;
            if (c < 0 || big[pc[c].terr] == c) continue;
            for (int dd = 0; dd < 8; dd += 2) {
                int j = ter_step(w, i % size, i / size, dd, w->cls[i]), k = 0;
                if (j < 0 || w->tl[j] < 0 || w->tl[j] == w->tl[i]) continue;
                while (k < 6 && pc[c].len[k] && pc[c].nb[k] != w->tl[j]) k++;
                if (k < 6) { pc[c].nb[k] = w->tl[j]; pc[c].len[k]++; }
            }
        }
        for (int c = 0; c < np; c++) {   // where each goes: terr from here on
            if (big[pc[c].terr] == c) continue;
            int best = -1;
            for (int k = 0; k < 6; k++) if (pc[c].len[k] && (best < 0 || pc[c].len[k] > pc[c].len[best])) best = k;
            pc[c].terr = best >= 0 ? pc[c].nb[best] : pc[c].size < TER_TINY || w->nreg >= w->cap ? -1 : w->nreg++;
            fixed++;
        }
        for (int i = 0; i < w->n && fixed; i++) if (w->tl[i] >= 0 && !(w->cls[i] & TER_TREE)) w->tl[i] = pc[w->sub[i]].terr;
        free(pc); free(big);
        g_ter.mended += fixed;
        if (!fixed) break;
    }
}
// territories still in more than one piece after that (none seen): counted for the log
internal int ter_pieces(struct ter_work *w, int nt) {
    int size = w->size, more = 0;
    int *cnt = calloc((size_t)nt + 1, sizeof *cnt);
    if (!cnt) return 0;
    for (int i = 0; i < w->n; i++) w->sub[i] = 0;
    for (int i = 0; i < w->n; i++) {
        if (w->tl[i] < 0 || w->sub[i] || (w->cls[i] & TER_TREE)) continue;
        int h = 0, t = 0;
        w->sub[i] = 1; w->ord[t++] = i;
        while (h < t) {
            int c = w->ord[h++];
            for (int dd = 0; dd < 8; dd++) {
                int j = ter_step(w, c % size, c / size, dd, w->cls[i]);
                if (j < 0 || w->sub[j] || w->tl[j] != w->tl[i]) continue;
                w->sub[j] = 1; w->ord[t++] = j;
            }
        }
        if (cnt[w->tl[i]]++ == 1) more++;
    }
    free(cnt);
    return more;
}
// the trees' tiles: each the territory of the labelled land tile it is reached from first, a wood from its edges in
internal void ter_trees(struct ter_work *w) {
    int size = w->size, h = 0, t = 0;
    for (int i = 0; i < w->n; i++) if (w->cls[i] == TER_LAND && w->tl[i] >= 0) w->ord[t++] = i;
    while (h < t) {
        int i = w->ord[h++], x = i % size, y = i / size;
        for (int dd = 0; dd < 8; dd += 2) {
            int nx = x + k_rm_dx[dd], ny = y + k_rm_dy[dd], j = ny * size + nx;
            if (nx < 0 || ny < 0 || nx >= size || ny >= size || w->cls[j] != (TER_LAND | TER_TREE) || w->tl[j] >= 0) continue;
            w->tl[j] = w->tl[i];
            w->ord[t++] = j;
        }
    }
}
internal void ter_free(void) {
    free(g_ter.cls); free(g_ter.clr); free(g_ter.lab); free(g_ter.t); free(g_ter.nb); free(g_ter.site);
    unsigned v = g_ter.version;
    memset(&g_ter, 0, sizeof g_ter);
    g_ter.version = v;
}
internal int ter_build(ee_map *map) {
    int size = (int)map_get_size(map), n = size * size;
    LARGE_INTEGER t0, t1, hz;
    ter_free();
    if (size <= 0 || size > 1024) return 0;
    QueryPerformanceFrequency(&hz); QueryPerformanceCounter(&t0);
    struct ter_work w = { 0 };
    w.size = size; w.n = n;
    w.cap = n / 2 + TER_MAX;
    w.cls = calloc((size_t)n, 1); w.d = calloc((size_t)n, sizeof *w.d); w.mine = calloc((size_t)n, 1);
    w.tl = malloc((size_t)n * sizeof *w.tl); w.ord = malloc((size_t)n * sizeof *w.ord);
    w.dist = malloc((size_t)n * sizeof *w.dist); w.sub = malloc((size_t)n * sizeof *w.sub);
    w.up = malloc((size_t)w.cap * sizeof *w.up); w.peak = malloc((size_t)w.cap * sizeof *w.peak); w.rsz = malloc((size_t)w.cap * sizeof *w.rsz);
    uint16_t *lab = malloc((size_t)n * sizeof *lab);
    int *ids = NULL, ok = w.cls && w.d && w.mine && w.tl && w.ord && w.dist && w.sub && w.up && w.peak && w.rsz && lab;
    if (ok) {
        ter_tiles(map, &w);
        for (int i = 0; i < n; i++) { w.tl[i] = -1; g_ter.tiles[w.cls[i] & 3]++; g_ter.tree_tiles += (w.cls[i] & TER_TREE) != 0; }
        for (int k = TER_LAND; k <= TER_WATER; k++) { ter_clear(&w, k); ter_shed(&w, k); }
        // the regions that are left, numbered anew from 0
        ids = malloc((size_t)(w.nreg + 1) * sizeof *ids);
        ok = ids != NULL;
    }
    if (ok) {
        int nr = 0;
        for (int r = 0; r < w.nreg; r++) ids[r] = -1;
        for (int i = 0; i < n; i++) {
            if (w.tl[i] < 0) continue;
            int r = rm_find(w.up, w.tl[i]);
            if (ids[r] < 0) ids[r] = nr++;
            w.tl[i] = ids[r];
        }
        g_ter.shed = w.nreg = nr;
        ter_join(&w);
        ter_split_all(&w);
        ter_mines(&w);
        ter_whole(&w);
        ter_trees(&w);
        // the territories: the regions with tiles, in the order the map's rows meet them
        free(ids);
        ids = malloc((size_t)(w.nreg + 1) * sizeof *ids);
        ok = ids != NULL;
    }
    if (ok) {
        int nt = 0;
        for (int r = 0; r < w.nreg; r++) ids[r] = -1;
        for (int i = 0; i < n; i++) {
            lab[i] = TER_NONE;
            if (w.tl[i] < 0) continue;
            if (ids[w.tl[i]] < 0) { if (nt >= TER_MAX) { g_ter.lost++; ids[w.tl[i]] = TER_MAX; } else ids[w.tl[i]] = nt++; }
            if (ids[w.tl[i]] < TER_MAX) lab[i] = (uint16_t)ids[w.tl[i]];
            w.tl[i] = ids[w.tl[i]] < TER_MAX ? ids[w.tl[i]] : -1;
        }
        w.nreg = nt;
        g_ter.t = calloc((size_t)nt + 1, sizeof *g_ter.t);
        long long *sx = calloc((size_t)nt + 1, sizeof *sx), *sy = calloc((size_t)nt + 1, sizeof *sy);
        int *bd = malloc((size_t)(nt + 1) * sizeof *bd);
        struct ter_edge *e = NULL;
        int ne = ter_edges(&w, &e);
        g_ter.nb = malloc((size_t)(2 * ne + 1) * sizeof *g_ter.nb);
        ok = g_ter.t && sx && sy && bd && g_ter.nb;
        if (ok) {
            for (int i = 0; i < n; i++) {
                if (lab[i] == TER_NONE) continue;
                struct ter_terr *t = &g_ter.t[lab[i]];
                if (w.cls[i] & TER_TREE) { t->trees++; continue; }
                t->water = w.cls[i] == TER_WATER;
                t->area++; sx[lab[i]] += i % size; sy[lab[i]] += i / size;
                if (w.d[i] / 5 > t->wide) t->wide = w.d[i] / 5;
            }
            for (int r = 0; r < nt; r++) bd[r] = 0x7FFFFFFF;
            for (int i = 0; i < n; i++) {
                if (lab[i] == TER_NONE || (w.cls[i] & TER_TREE)) continue;
                struct ter_terr *t = &g_ter.t[lab[i]];
                int dx = i % size - (int)(sx[lab[i]] / t->area), dy = i / size - (int)(sy[lab[i]] / t->area);
                if (dx * dx + dy * dy < bd[lab[i]]) { bd[lab[i]] = dx * dx + dy * dy; t->c = iv2(i % size, i / size); }
            }
            // each one's neighbours: the borders listed from both sides, a territory's own in a row
            for (int q = 0; q < ne; q++) { g_ter.t[e[q].a].nbn++; g_ter.t[e[q].b].nbn++; }
            for (int r = 0, at = 0; r < nt; r++) { g_ter.t[r].nb0 = at; at += g_ter.t[r].nbn; g_ter.t[r].nbn = 0; }
            for (int q = 0; q < ne; q++)
                for (int s = 0; s < 2; s++) {
                    struct ter_terr *t = &g_ter.t[s ? e[q].b : e[q].a], *o = &g_ter.t[s ? e[q].a : e[q].b];
                    g_ter.nb[t->nb0 + t->nbn].to = (uint16_t)(s ? e[q].a : e[q].b); g_ter.nb[t->nb0 + t->nbn++].len = e[q].len;
                    if (t->water == o->water) t->border += e[q].len; else t->coast += e[q].len;
                }
            g_ter.nnb = 2 * ne; g_ter.n = nt;
            g_ter.pieces = ter_pieces(&w, nt);
            for (int r = 0; r < nt; r++) { if (g_ter.t[r].water) g_ter.n_water++; else g_ter.n_land++; }
        }
        free(sx); free(sy); free(bd); free(e);
    }
    free(w.tl); free(w.ord); free(w.dist); free(w.sub); free(w.up); free(w.peak); free(w.rsz); free(w.heap); free(w.mine); free(ids);
    if (!ok) { free(w.cls); free(w.d); free(lab); ter_free(); return 0; }
    g_ter.cls = w.cls; g_ter.clr = w.d; g_ter.lab = lab;
    g_ter.map = map; g_ter.size = size;
    g_ter.p_size = g_ter_size; g_ter.p_ratio = g_ter_ratio; g_ter.p_trees = g_ter_trees; g_ter.p_water = g_ter_water; g_ter.p_field = g_ter_field;
    g_ter.version++;
    g_ter.built_at = game_time();
    QueryPerformanceCounter(&t1);
    g_ter.build_ms = (unsigned)((t1.QuadPart - t0.QuadPart) * 1000 / hz.QuadPart);
    return 1;
}
// what the build made, in a line
internal void ter_summary(char *out, size_t cap) {
    int *a = malloc((size_t)(g_ter.n + 1) * sizeof *a);
    out[0] = 0;
    if (!a) return;
    out_printf(out, cap, "map %d: %d land tiles (%d under trees), %d water, %d neither; size %d (water x%d), ratio %d%%, trees %d: %d territories",
               g_ter.size, g_ter.tiles[TER_LAND], g_ter.tree_tiles, g_ter.tiles[TER_WATER], g_ter.tiles[TER_OFF], g_ter.p_size, g_ter.p_water, g_ter.p_ratio, g_ter.p_trees, g_ter.n);
    for (int k = 0; k < 2; k++) {
        int m = 0, wide = 0;
        int border = 0;
        for (int r = 0; r < g_ter.n; r++) if (g_ter.t[r].water == k) { a[m++] = g_ter.t[r].area; border += g_ter.t[r].border; if (g_ter.t[r].wide > wide) wide = g_ter.t[r].wide; }
        if (!m) continue;
        qsort(a, (size_t)m, sizeof *a, ter_int_cmp);
        out_printf(out, cap, " | %s %d: tiles %d / %d / %d (least, middle, most), border %d a territory, the widest place %d",
                   k ? "water" : "land", m, a[0], a[m / 2], a[m - 1], border / m, wide);
    }
    out_printf(out, cap, " | %d regions by the narrow places, %d small ones joined (%d tiles in none: too few), %d parts by size%s, %d tiles moved with the mines' ground, %d parted pieces mended, %d territories in more than one piece, %d resource sites, built in %u ms",
               g_ter.shed, g_ter.joined, g_ter.dropped, g_ter.split, g_ter.lost ? " (MORE THAN KEPT)" : "", g_ter.moved, g_ter.mended, g_ter.pieces, g_ter.nsite, g_ter.build_ms);
    free(a);
}
// the territories of the map as it stands: built for it, and again when a setting of the split changed; 0: no map.
// One built while the game loads (the pipe asked: the terrain is there, the trees and mines not yet - seen: 0 trees,
// 0 sites, 16 regions where the started game has 68) is built again once the game runs; a map that really has
// neither is built anew for its first minute only.
internal int ter_ready(void) {
    ee_map *map = game_map();
    if (!map) return 0;
    if (g_ter.map == map && g_ter.size == (int)map_get_size(map) && g_ter.p_size == g_ter_size && g_ter.p_ratio == g_ter_ratio &&
        g_ter.p_trees == g_ter_trees && g_ter.p_water == g_ter_water && g_ter.p_field == g_ter_field &&
        (g_ter.nsite || g_ter.tree_tiles || game_time() == g_ter.built_at || g_ter.built_at >= 60000)) return 1;
    if (!ter_build(map)) return 0;
    if (g_log) {
        static char line[1024];
        ter_summary(line, sizeof line);
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# territories: %s\n", line);
        LeaveCriticalSection(&g_lock);
    }
    return 1;
}
internal void territory_new_game(void) { g_ter.map = NULL; }   // the next game's map may sit at the same address
// the territory of a tile, TER_NONE none (a cliff, off the map; nothing built yet)
internal unsigned territory_at(int x, int y) {
    if (!g_ter.lab || x < 0 || y < 0 || x >= g_ter.size || y >= g_ter.size) return TER_NONE;
    return g_ter.lab[y * g_ter.size + x];
}
// ... and the nearest one's within 2 tiles (a building's own tile is no open ground)
internal unsigned ter_near(int x, int y) {
    unsigned r = territory_at(x, y);
    for (int rad = 1; rad <= 2 && r == TER_NONE; rad++)
        for (int dy = -rad; dy <= rad && r == TER_NONE; dy++)
            for (int dx = -rad; dx <= rad && r == TER_NONE; dx++) r = territory_at(x + dx, y + dy);
    return r;
}
internal int ter_is_building(ee_unit_type *td) {
    int c = unit_type_get_class_id(td);
    return c == CLS_BUILDING || c == CLS_FORTRESS || c == CLS_NAVAL_BUILDING || c == CLS_GATE || is_tower_type(unit_type_get_id(td));
}
// the facts as they are: the sites left, who stands and has built where
internal void ter_facts(void) {
    ee_map *map = g_ter.map;
    int size = g_ter.size;
    for (int r = 0; r < g_ter.n; r++) {
        struct ter_terr *t = &g_ter.t[r];
        memset(t->res, 0, sizeof t->res); memset(t->amount, 0, sizeof t->amount);
        memset(t->bld, 0, sizeof t->bld); memset(t->units, 0, sizeof t->units);
        t->owner = t->contested = 0;
    }
    for (int k = 0; k < g_ter.nsite; k++) {
        struct ter_site *s = &g_ter.site[k];
        unsigned r = ter_near(s->x, s->y);
        ee_tile *tile = map_get_tiles(map)[s->y * size + s->x];
        if (r == TER_NONE || !tile) continue;
        for (void *node = tile_get_objects(tile); node; node = tilenode_get_next(node)) {
            ee_unit *o = tilenode_get_object(node);
            ee_unit_type *otd = o ? unit_get_type_data(o) : NULL;
            if (!otd || unit_type_get_resource_type(otd) != s->res || unit_get_x(o) != s->x || unit_get_y(o) != s->y || unit_get_amount(o) <= 0) continue;
            g_ter.t[r].res[s->res]++; g_ter.t[r].amount[s->res] += unit_get_amount(o);
            break;
        }
    }
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        int p = o ? player_get_index(o) : 0;
        if (p <= 0 || p > 15 || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
        unsigned r = ter_near(unit_get_x(u), unit_get_y(u));
        if (r == TER_NONE) continue;
        if (ter_is_building(it.td)) g_ter.t[r].bld[p]++; else g_ter.t[r].units[p]++;
    }
    for (int r = 0; r < g_ter.n; r++) {
        struct ter_terr *t = &g_ter.t[r];
        for (int p = 1; p < 16; p++) if (t->bld[p] > (t->owner ? t->bld[t->owner] : 0)) t->owner = (int8_t)p;
        ee_player *own = t->owner ? player_at(t->owner) : NULL;
        for (int p = 1; p < 16 && own; p++) if (p != t->owner && t->bld[p] && player_at(p) && is_enemy(own, player_at(p))) t->contested = 1;
    }
}
// a territory's line: its number, tiles, borders and what it holds
internal void ter_line(char *out, size_t cap, int r, int full) {
    static const char *const k_res[7] = { "", "", "food", "wood", "stone", "gold", "iron" };
    struct ter_terr *t = &g_ter.t[r];
    out[0] = 0;
    if (!full) {   // the overlay's: short
        out_printf(out, cap, "%s%d: %d b%d c%d", t->water ? "w" : "", r, t->area, t->border, t->coast);
        for (int k = 2; k < 7; k++) if (t->res[k] && k != 3) out_printf(out, cap, " %c%d", k_res[k][0] - 32, t->res[k]);
        return;
    }
    out_printf(out, cap, "%s %d at %d,%d: %d tiles (+%d under trees), widest %d, border %d, coast %d, neighbours", t->water ? "water" : "land", r, t->c.x, t->c.y, t->area, t->trees, t->wide, t->border, t->coast);
    for (int k = 0; k < t->nbn; k++) out_printf(out, cap, " %d:%d", g_ter.nb[t->nb0 + k].to, g_ter.nb[t->nb0 + k].len);
    for (int k = 2; k < 7; k++) if (t->res[k]) out_printf(out, cap, ", %s %d (%d)", k_res[k], t->res[k], t->amount[k]);
    for (int p = 1; p < 16; p++) if (t->bld[p] || t->units[p]) out_printf(out, cap, ", P%d %d buildings %d units", p, t->bld[p], t->units[p]);
    if (t->owner) out_printf(out, cap, ", held by P%d%s", t->owner, t->contested ? " (contested)" : "");
}
// a sum over the territories' holders: the borders drawn again when one changes hands
internal uint32_t ter_owners(void) {
    uint32_t h = 2166136261u;
    for (int r = 0; r < g_ter.n; r++) h = (h ^ (uint32_t)(uint8_t)g_ter.t[r].owner) * 16777619u;
    return h;
}
// a border's side: its territory's holder's colour, else land white, water blue
internal uint32_t ter_side_argb(unsigned r) {
    struct ter_terr *t = &g_ter.t[r];
    return t->owner ? overlay_player_argb(t->owner, 230) : t->water ? 0xB050B0FFu : 0xD0FFFFFFu;
}
// the overlay's layer "territories": the borders as two dashed lines, one inside each side in the colour of the side's
// holder (the player, 2026-11-14: "can you change the territory border to a double dotted line? with each side of the
// line in the color of the owner", then "try dashed, with one dash per tile border"; the coast is the map's own), each
// territory's middle with its line, the held ones faintly in their holder's colour
#define TER_SIDE 0.17f   // a side's dashes this far in from the border, in tiles
#define TER_DASH 10      // a dash along each tile's side, 10 sixteenths of it (overlay_line_dashed)
internal void ter_draw(int layer, uint32_t now, int borders) {
    int size = g_ter.size;
    const uint16_t *lab = g_ter.lab;
    uint32_t owners = ter_owners();
    if (borders || owners != g_ter.drawn_owners) {
        int lines = 0;
        g_ter.drawn_owners = owners;
        overlay_begin(layer, 0, now, 0);
        // a border between two tiles: none (one territory, none, or coast), else both sides' territories, which make a run
        #define TER_B(i, j) (lab[i] != lab[j] && lab[i] != TER_NONE && lab[j] != TER_NONE && g_ter.t[lab[i]].water == g_ter.t[lab[j]].water)
        #define TER_K(i, j) ((uint32_t)lab[i] << 16 | lab[j])
        for (int y = 1; y < size; y++)   // below (y) and above (y - 1)
            for (int x = 0; x < size; ) {
                int i = y * size + x, j = (y - 1) * size + x, x0 = x++;
                if (!TER_B(i, j)) continue;
                uint32_t k = TER_K(i, j);
                while (x < size && TER_B(y * size + x, (y - 1) * size + x) && TER_K(y * size + x, (y - 1) * size + x) == k) x++;
                if ((lines += 2) > TER_LINES) continue;
                overlay_line_dashed(v2((float)x0, (float)y + TER_SIDE), v2((float)x, (float)y + TER_SIDE), ter_side_argb(lab[i]), 0.14f, TER_DASH);
                overlay_line_dashed(v2((float)x0, (float)y - TER_SIDE), v2((float)x, (float)y - TER_SIDE), ter_side_argb(lab[j]), 0.14f, TER_DASH);
            }
        for (int x = 1; x < size; x++)   // right (x) and left (x - 1)
            for (int y = 0; y < size; ) {
                int i = y * size + x, j = y * size + x - 1, y0 = y++;
                if (!TER_B(i, j)) continue;
                uint32_t k = TER_K(i, j);
                while (y < size && TER_B(y * size + x, y * size + x - 1) && TER_K(y * size + x, y * size + x - 1) == k) y++;
                if ((lines += 2) > TER_LINES) continue;
                overlay_line_dashed(v2((float)x + TER_SIDE, (float)y0), v2((float)x + TER_SIDE, (float)y), ter_side_argb(lab[i]), 0.14f, TER_DASH);
                overlay_line_dashed(v2((float)x - TER_SIDE, (float)y0), v2((float)x - TER_SIDE, (float)y), ter_side_argb(lab[j]), 0.14f, TER_DASH);
            }
        #undef TER_B
        #undef TER_K
        overlay_end();
        g_ter.lines = lines;
    }
    overlay_begin(layer, 1, now, 3 * TER_FACTS_MS);
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; ) {
            unsigned r = lab[y * size + x];
            int x0 = x++, own = r == TER_NONE ? 0 : g_ter.t[r].owner;
            if (!own) continue;
            while (x < size && lab[y * size + x] == r) x++;
            if ((y & 1) || !g_ter.t[r].contested) overlay_tiles(iv2(x0, y), x - x0, overlay_player_argb(own, 40));   // contested: every other row
        }
    for (int r = 0; r < g_ter.n; r++) {
        struct ter_terr *t = &g_ter.t[r];
        char line[64];
        vec2 c = v2((float)t->c.x + 0.5f, (float)t->c.y + 0.5f);
        uint32_t argb = t->owner ? overlay_player_argb(t->owner, 255) : t->water ? 0xFF80C8FFu : 0xFFFFFFFFu;
        ter_line(line, sizeof line, r, 0);
        overlay_mark(c, OVM_SQUARE | OVM_HOLLOW, 6.0f, argb);
        overlay_text(c, 1, argb, "%s", line);
    }
    overlay_end();
}
// every step of an AI player: the layer kept up while it is shown, the log's lines (list: [autotest] territories_log)
internal void territory_tick(uint32_t now, int list) {
    static int layer = -2;
    if (layer == -2) layer = overlay_layer("territories");
    int show = overlay_on(layer), logs = g_log && list;
    if (!show && !logs) { g_ter.shown = 0; return; }
    if (g_ter.facts_t && now >= g_ter.facts_t && now - g_ter.facts_t < TER_FACTS_MS && show == g_ter.shown && g_ter.map == game_map() &&
        g_ter.p_size == g_ter_size && g_ter.p_ratio == g_ter_ratio && g_ter.p_trees == g_ter_trees && g_ter.p_water == g_ter_water && g_ter.p_field == g_ter_field) return;
    if (!ter_ready()) return;
    g_ter.facts_t = now ? now : 1;
    ter_facts();
    if (show) { ter_draw(layer, now, !g_ter.shown || g_ter.drawn != g_ter.version); g_ter.drawn = g_ter.version; }
    g_ter.shown = show;
    if (logs && (!g_ter.log_t || now < g_ter.log_t || now - g_ter.log_t >= 300000)) {
        static char line[2048];
        g_ter.log_t = now ? now : 1;
        EnterCriticalSection(&g_lock);
        for (int r = 0; r < g_ter.n; r++) { ter_line(line, sizeof line, r, 1); fprintf(g_log, "# territory: at %.1f min: %s\n", now / 60000.0, line); }
        LeaveCriticalSection(&g_lock);
    }
}
// the pipe's territories [size n] [ratio n] [trees 0|1] [water n] [field n] [list] [at x y] [dump <file>]: the split's settings
// (a change builds it anew), what it made; list: every territory; at: a tile's; dump: the tiles' territories and
// kinds into a file (a line "TER <size> <territories>", then a uint16 and then a byte a tile) for a picture of it
internal void cmd_territories(char *out, const char *args) {
    static char line[2048];
    char word[32], path[260] = "";
    int list = 0, ax = -1, ay = -1, used = 0, v;
    while (sscanf(args, " %31s%n", word, &used) == 1) {
        args += used;
        if (!strcmp(word, "list")) { list = 1; continue; }
        if (!strcmp(word, "at")) { if (sscanf(args, " %d %d%n", &ax, &ay, &used) == 2) args += used; continue; }
        if (!strcmp(word, "dump")) { while (*args == ' ') args++; snprintf(path, sizeof path, "%s", args); break; }
        if (sscanf(args, " %d%n", &v, &used) != 1) { REPLY("error: %s needs a number (size n, ratio n, trees 0|1, water n, list, at x y, dump <file>)\n", word); return; }
        args += used;
        if (!strcmp(word, "size") && v >= 8 && v <= 512) g_ter_size = v;
        else if (!strcmp(word, "ratio") && v >= 0 && v <= 100) g_ter_ratio = v;
        else if (!strcmp(word, "trees")) g_ter_trees = v != 0;
        else if (!strcmp(word, "water") && v >= 1 && v <= 16) g_ter_water = v;
        else if (!strcmp(word, "field") && v >= 0 && v <= 64) g_ter_field = v;
        else { REPLY("error: %s %d - size 8..512, ratio 0..100, trees 0|1, water 1..16\n", word, v); return; }
    }
    if (!ter_ready()) { REPLY("error: no map\n"); return; }
    ter_facts();
    ter_summary(line, sizeof line);
    REPLY("territories: %s\n", line);
    if (ax >= 0) {
        unsigned r = territory_at(ax, ay);
        if (r == TER_NONE) REPLY("%d,%d: in none (kind %d)\n", ax, ay, ax < g_ter.size && ay >= 0 && ay < g_ter.size ? g_ter.cls[ay * g_ter.size + ax] : -1);
        else { ter_line(line, sizeof line, (int)r, 1); REPLY("%d,%d (clearance %d): %s\n", ax, ay, g_ter.clr[ay * g_ter.size + ax] / 5, line); }
    }
    for (int r = 0; list && r < g_ter.n; r++) { ter_line(line, sizeof line, r, 1); REPLY("%s\n", line); }
    if (path[0]) {
        FILE *f = fopen(path, "wb");
        if (!f) { REPLY("error: can't write %s\n", path); return; }
        fprintf(f, "TER %d %d\n", g_ter.size, g_ter.n);
        fwrite(g_ter.lab, sizeof *g_ter.lab, (size_t)g_ter.size * (size_t)g_ter.size, f);
        fwrite(g_ter.cls, 1, (size_t)g_ter.size * (size_t)g_ter.size, f);
        for (int r = 0; r < g_ter.n; r++) { ter_line(line, sizeof line, r, 1); fprintf(f, "%s\n", line); }
        for (int k = 0; k < g_ter.nsite; k++) fprintf(f, "site %d %d %d\n", g_ter.site[k].x, g_ter.site[k].y, g_ter.site[k].res);
        fclose(f);
        REPLY("written: %s\n", path);
    }
}
