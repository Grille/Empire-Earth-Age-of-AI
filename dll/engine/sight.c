// SIGHT COUNTS (`sight_counts`, default 1; 0 = the original). A map tile counts per player the units that see it
// (game/api/map.c: 0 unexplored, 1 explored, 2 + n), in 16 bits, but every reader of the game looks at the low byte
// only and the add (0x519F4F) restarts a low byte of 0 at 2. In a crowd of hundreds of units a tile's count reached
// 256: the draw list's collection (0x503B83) took it for unexplored and left out the tile with everything on it,
// at 257 for explored only (the remembered objects, no units) - random tiles of a big army blinked as units came and
// went, and the count, restarted at 2, fell below the units still seeing it (the tile went to fog before they left).
// The add and the remove (0x519E26) are replaced: the count kept whole, written by sight_raw so that its low byte
// still reads 0, 1, or 2 .. 255 for 2 and more - every reader of the game unchanged - and the minimap's rebuild
// (0x514A8A), which adds a tile's count less one to its pixel, given the rest. Only these three change the count by
// one (a scan of the exe for writes through the slot's shift: the setter 0x51979C besides, which writes 0, 1 or 2).
// The map keeps a second count for the viewed player (its view table, MAP_VIEW_ROWS: 16 bits, +1 / -1 beside the
// tiles'); its refill (0x55BC4E: a load, the viewed player changed) took the tiles' low byte - under a crowd it
// started low and its plain -1 drove it to 0 or below while units still saw the tile. It is given the whole count.

internal int g_sight_on = 1;
internal unsigned g_sight_peak;   // the highest count since the start (sightcount)

// THE TILES EVER SEEN, a bit per player (godview's black, godmode.c god_eyes; the player 2026-11-16: unexplored terrain
// not black in godview). The computer players have the whole map explored from the start (count 1 on every tile), so
// their counts can't tell what they have seen: the add marks it here. The counts are one buffer (map.sight_players:
// each tile's sight pointer into it, the tiles outside the map's diamond none), a slot of `stride` bytes a tile: the
// slot's tile is looked up (slot_tile, made when the buffer is new - a new game, a load: what is in sight then counts
// as seen, a load forgets what was seen before it). The bits are only written by the simulation; arrays once handed
// out are never freed (the render thread reads them), a bigger map takes new ones.
internal struct {
    ee_map *map;
    ee_player *p0;      // the game's first player: another game
    uint8_t *base;      // the buffer's start (the lowest tile's counts)
    uint8_t *k0_at;     // tile k0's counts: the buffer moved when they did
    size_t k0, stride;  // the first tile there is; bytes a slot
    unsigned n, slots;  // tiles a side; slots in the buffer
    uint32_t *slot_tile;   // slot -> the tile's index y * n + x
    uint16_t *bits;     // n * n, bit p: player p has seen the tile; NULL: the buffer not as expected
    uint16_t *bits_mem; // the bits' array (cap tiles)
    size_t cap;
    int failed;
} g_seen;
internal int seen_fail(const char *why) {
    g_seen.bits = NULL;
    if (g_log && !g_seen.failed++) log_locked("# sight: %s - godview shows the explored, not the seen\n", why);
    return 0;
}
internal int seen_ready(void) {
    ee_map *map = game_map();
    ee_tile **tiles = map ? map_get_tiles(map) : NULL;
    unsigned n = map ? map_get_size(map) : 0;
    if (!tiles || n < 2) return 0;
    size_t nn = (size_t)n * n;
    ee_player *p0 = game_players()[0];
    if (map == g_seen.map && n == g_seen.n && p0 == g_seen.p0 && g_seen.k0 < nn && tiles[g_seen.k0] &&
        (uint8_t *)tile_get_sight(tiles[g_seen.k0]) == g_seen.k0_at)
        return g_seen.bits != NULL;
    g_seen.map = map, g_seen.n = n, g_seen.p0 = p0, g_seen.bits = NULL, g_seen.k0_at = NULL;
    uint8_t *lo = NULL, *hi = NULL, *prev = NULL;
    size_t stride = 0, count = 0;
    for (size_t k = 0; k < nn; k++) {
        if (!tiles[k]) continue;
        uint8_t *a = (uint8_t *)tile_get_sight(tiles[k]);
        if (!a) return seen_fail("a tile without counts");
        if (!g_seen.k0_at) g_seen.k0 = k, g_seen.k0_at = a;
        if (!lo || a < lo) lo = a;
        if (!hi || a > hi) hi = a;
        if (prev && a > prev && (!stride || (size_t)(a - prev) < stride)) stride = (size_t)(a - prev);
        prev = a, count++;
    }
    if (!count || !stride || stride > 64 || (size_t)(hi - lo) / stride + 1 != count) return seen_fail("the counts not one slot a tile");
    if (nn > g_seen.cap) {
        uint16_t *b = calloc(nn, sizeof *b);
        uint32_t *st = calloc(nn, sizeof *st);
        if (!b || !st) { free(b); free(st); return seen_fail("no memory"); }
        g_seen.slot_tile = st, g_seen.bits_mem = b, g_seen.cap = nn;   // (the old ones stay: the render thread may still read them)
    }
    uint16_t *b = g_seen.bits_mem;
    unsigned np = game_player_count() < 16 ? game_player_count() : 16;
    for (size_t k = 0; k < nn; k++) {
        b[k] = 0;
        if (!tiles[k]) continue;
        uint8_t *a = (uint8_t *)tile_get_sight(tiles[k]);
        size_t off = (size_t)(a - lo);
        if (off % stride) return seen_fail("a tile's counts off the slots");
        g_seen.slot_tile[off / stride] = (uint32_t)k;
        const uint16_t *s = (const uint16_t *)a;
        for (unsigned p = 0; p < np; p++) if (sight_count(s[p]) >= 2) b[k] |= (uint16_t)(1u << p);
    }
    g_seen.base = lo, g_seen.stride = stride, g_seen.slots = (unsigned)count;
    g_seen.bits = b;
    return 1;
}
// player p has seen tile x, y: 1, 0 not, -1 not known (no game, the buffer not as expected, sight_counts off)
internal int sight_seen_ever(int x, int y, int p) {
    uint16_t *b = g_seen.bits;
    unsigned n = g_seen.n;
    if (!g_sight_on || !b || x < 0 || y < 0 || (unsigned)x >= n || (unsigned)y >= n || p < 0 || p >= 16) return -1;
    return (b[(size_t)y * n + (unsigned)x] >> p) & 1;
}

internal void __thiscall hook_sight_add(ee_tile *tile, ee_player *pl) {
    int pi = player_get_index(pl);
    uint16_t *s = tile_get_sight(tile) + pi;
    unsigned c = sight_count(*s);
    c = c < 2 ? 2 : c < SIGHT_MAX ? c + 1 : c;   // unexplored or explored: seen by one (the game's own step)
    if (c > g_sight_peak) g_sight_peak = c;
    *s = sight_raw(c);
    if (seen_ready() && pi >= 0 && pi < 16) {
        size_t off = (size_t)((uint8_t *)(s - pi) - g_seen.base), j = off / g_seen.stride;
        if (off % g_seen.stride == 0 && j < g_seen.slots) g_seen.bits[g_seen.slot_tile[j]] |= (uint16_t)(1u << pi);
    }
}

internal void __thiscall hook_sight_sub(ee_tile *tile, ee_player *pl) {
    uint16_t *s = tile_get_sight(tile) + player_get_index(pl);
    unsigned c = sight_count(*s);
    if (c > 1) *s = sight_raw(c - 1);
}

// the game's adds the low byte less one; the rest of the count on top (the viewed player's, as the game's)
internal void __thiscall hook_minimap_tile(void *mm, ee_tile *tile, int x, int y) {
    int eyes = god_eyes();   // godview: a tile that player has never seen left unpainted (black)
    if (eyes >= 0 && seen_ready() && sight_seen_ever(x, y, eyes) == 0) return;
    minimap_tile(mm, tile, x, y);
    if (!tile) return;
    uint16_t raw = tile_get_sight(tile)[player_get_index(game_players()[local_player_index()])];
    for (unsigned n = sight_count(raw) - (raw & 0xFFu); n; n--) minimap_sight_add(mm, x, y);
}

// the view table's refill took each tile's low byte: the count whole after it (the copy's own writes, row by row)
internal void __thiscall hook_view_sight_copy(ee_player *pl) {
    player_view_sight_copy(pl);
    ee_map *map = game_map();
    if (!map || !pl) return;
    unsigned n = map_get_size(map), pi = (unsigned)player_get_index(pl);
    ee_tile **tiles = map_get_tiles(map);
    for (unsigned y = 0; y < n; y++) {
        void *row = map_view_row(map, y);
        for (unsigned x = 0; x < n; x++) {
            ee_tile *t = tiles[y * n + x];
            uint16_t *c = t ? viewrow_count_at(row, x) : NULL;
            if (c) *c = (uint16_t)sight_count(tile_get_sight(t)[pi]);
        }
    }
}

internal void sight_install(void) {
    g_sight_on = g_set.sight_counts;
    if (!g_sight_on) {
        if (g_log) fprintf(g_log, "# sight counts: the original (a low byte)\n");
        return;
    }
    int n = patch_tile_sight(hook_sight_add, hook_sight_sub), m = patch_minimap_tile(hook_minimap_tile),
        v = patch_view_sight_copy(hook_view_sight_copy);
    if (g_log) fprintf(g_log, "# sight counts: 16 bits (%d calls of the add and remove, %d of the minimap's, %d of the view table's)\n",
                       n, m, v);
}

// sightcount [x y [player [refill]]]: the highest count since the start; with a tile, its counts (the player's, or all
// slots) and the view table's; refill first refills the view table for the viewed player as a load does (the hook,
// or with sight_counts=0 the game's own)
internal void cmd_sightcount(char *out, const char *args) {
    int x = -1, y = -1, p = -1;
    char what[16] = "";
    sscanf(args, "%d %d %d %15s", &x, &y, &p, what);
    if (!strcmp(what, "refill")) {
        ee_player *viewed = game_players()[local_player_index()];
        if (g_sight_on) hook_view_sight_copy(viewed);
        else player_view_sight_copy(viewed);
    }
    REPLY("sight_counts %d, highest count %u%s\n", g_sight_on, g_sight_peak, *what ? ", view table refilled" : "");
    ee_map *map = game_map();
    if (x < 0 || !map) return;
    uint32_t n = map_get_size(map);
    if ((unsigned)x >= n || (unsigned)y >= n) { REPLY("no tile %d,%d\n", x, y); return; }
    ee_tile *t = map_get_tiles(map)[(unsigned)y * n + (unsigned)x];
    if (!t) return;
    uint16_t *s = tile_get_sight(t);
    for (int i = 0; i < 16; i++)
        if (p < 0 ? i < (int)game_player_count() : i == p) REPLY("  slot %d: %u (raw %04X)\n", i, sight_count(s[i]), s[i]);
    uint16_t *v = viewrow_count_at(map_view_row(map, (unsigned)y), (unsigned)x);
    if (v) REPLY("  view table (the viewed player's): %d\n", (int16_t)*v);
    int ready = seen_ready();
    REPLY("  seen ever (godview's black): %s", ready ? "" : "not known");
    for (int i = 0; ready && i < (int)game_player_count() && i < 16; i++) REPLY(" P%d %d", i, sight_seen_ever(x, y, i));
    REPLY("\n");
}
