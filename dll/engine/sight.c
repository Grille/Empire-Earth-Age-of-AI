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

internal int g_sight_on = 1;
internal unsigned g_sight_peak;   // the highest count since the start (sightcount)

internal void __thiscall hook_sight_add(ee_tile *tile, ee_player *pl) {
    uint16_t *s = tile_get_sight(tile) + player_get_index(pl);
    unsigned c = sight_count(*s);
    c = c < 2 ? 2 : c < SIGHT_MAX ? c + 1 : c;   // unexplored or explored: seen by one (the game's own step)
    if (c > g_sight_peak) g_sight_peak = c;
    *s = sight_raw(c);
}

internal void __thiscall hook_sight_sub(ee_tile *tile, ee_player *pl) {
    uint16_t *s = tile_get_sight(tile) + player_get_index(pl);
    unsigned c = sight_count(*s);
    if (c > 1) *s = sight_raw(c - 1);
}

// the game's adds the low byte less one; the rest of the count on top (the viewed player's, as the game's)
internal void __thiscall hook_minimap_tile(void *mm, ee_tile *tile, int x, int y) {
    minimap_tile(mm, tile, x, y);
    if (!tile) return;
    uint16_t raw = tile_get_sight(tile)[player_get_index(game_players()[local_player_index()])];
    for (unsigned n = sight_count(raw) - (raw & 0xFFu); n; n--) minimap_sight_add(mm, x, y);
}

internal void sight_install(const char *aimod_ini, const char *autotest_ini) {
    g_sight_on = ini_int("general", "sight_counts", 1, aimod_ini);
    if (autotest_ini) g_sight_on = ini_int("autotest", "sight_counts", g_sight_on, autotest_ini);
    if (!g_sight_on) {
        if (g_log) fprintf(g_log, "# sight counts: the original (a low byte)\n");
        return;
    }
    int n = patch_tile_sight(hook_sight_add, hook_sight_sub), m = patch_minimap_tile(hook_minimap_tile);
    if (g_log) fprintf(g_log, "# sight counts: 16 bits (%d calls of the add and remove, %d of the minimap's)\n", n, m);
}

// sightcount [x y [player]]: the highest count since the start; with a tile, its counts (the player's, or all slots)
internal void cmd_sightcount(char *out, const char *args) {
    int x = -1, y = -1, p = -1;
    sscanf(args, "%d %d %d", &x, &y, &p);
    REPLY("sight_counts %d, highest count %u\n", g_sight_on, g_sight_peak);
    ee_map *map = game_map();
    if (x < 0 || !map) return;
    uint32_t n = map_get_size(map);
    if ((unsigned)x >= n || (unsigned)y >= n) { REPLY("no tile %d,%d\n", x, y); return; }
    ee_tile *t = map_get_tiles(map)[(unsigned)y * n + (unsigned)x];
    if (!t) return;
    uint16_t *s = tile_get_sight(t);
    for (int i = 0; i < 16; i++)
        if (p < 0 ? i < (int)game_player_count() : i == p) REPLY("  slot %d: %u (raw %04X)\n", i, sight_count(s[i]), s[i]);
}
