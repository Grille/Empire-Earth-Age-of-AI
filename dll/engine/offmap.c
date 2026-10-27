// A UNIT PUT OFF THE MAP: a unit's tile changes through the tile setter 0x616F9D (EEComplexUnit.set_tile, 10 calls):
// its map cell looked up by y * size + x - none past the map's square - and, with x changed, its place in the world's
// x-sorted unit list again (EEWorld.resort 0x5310B3), whose walk runs off the list's end for an x past every unit's and
// links the unit into nothing: a read of NULL+8 at 0x628D6F. The player's game crashed there at 594.7 min (save
// "standing army"; the stack: a holder letting its units out, EEUCTransport vf64 0x60A2ED -> 0x5EEAE8 -> the placing
// 0x5F043B -> the tile setter) and three replays of the same save didn't (aimod-55, 2026-10-27) - so the tile came from
// something of that session, still unknown. Here a tile off the map (not the game's own (-10000, -10000), which takes
// a unit off the map: garrisoned, loaded) is the nearest tile on it, the unit's position (matrix [3], [7]) moved there
// too - else the next step's tile is off the map again - and the first OFFMAP_LOG are logged: the unit, its owner,
// type and order, where it was and where it was to go, and the caller (one of the 10 calls) - the next time tells
// whose unload or move it was. Every unit, every player: a crash fix (asg_queue.c's kind).
#define OFFMAP_LOG 20
internal unsigned g_offmap_n;
internal void __thiscall hook_set_tile(ee_unit *u, uint32_t x, uint32_t y, char link) {
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0, ix = (int)x, iy = (int)y;
    if (size > 0 && (ix < 0 || iy < 0 || ix >= size || iy >= size) && !(ix == -10000 && iy == -10000)) {
        int cx = ix < 0 ? 0 : ix >= size ? size - 1 : ix, cy = iy < 0 ? 0 : iy >= size ? size - 1 : iy;
        if (g_log && g_offmap_n++ < OFFMAP_LOG) {
            ee_unit_type *td = unit_get_type_data(u);
            ee_player *o = unit_get_owner(u);
            ee_goal *g = unit_get_goal(u);
            fprintf(g_log, "# engine fix: P%d's unit (type %d, order %s) put off the map at %.1f min: from %d,%d to %d,%d (map %d) - "
                    "%d,%d instead; called from %06x\n", o ? player_get_index(o) : -1, td ? unit_type_get_id(td) : -1,
                    g ? rtti_name(g) : "none", game_time() / 60000.0, unit_get_x(u), unit_get_y(u), ix, iy, size, cx, cy,
                    (unsigned)(uintptr_t)__builtin_return_address(0) - 5);
            fflush(g_log);
        }
        unit_set_matrix(u, 3, (float)cx + 0.5f);
        unit_set_matrix(u, 7, (float)cy + 0.5f);
        x = (uint32_t)cx; y = (uint32_t)cy;
    }
    unit_set_tile(u, x, y, link);
}
internal void offmap_install(void) {
    int of = 0, ok = patch_set_tile(hook_set_tile, &of);
    if (g_log) fprintf(g_log, "# engine fix: units put off the map pulled back (tile setter calls %d/%d)\n", ok, of);
}
