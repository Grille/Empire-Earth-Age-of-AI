// THE UNITS' COLLECTION, THE MOD'S OWN (the pipe: picture collect own | game; the game's by default). The units' update
// (EEROUnits::vf0 0x503B83, AoC 0x50EABE) fills the draw lists the frame draws from: after its setup (the lists
// emptied, the selection, the unit under the mouse) it walks the visible tiles inside the picker's lock - every
// object on a tile and the parts it is made of, each the view's test asks for (EEROUnits.drawn) into a record of the
// objects' list or the trees', sorted by its opacity; then on a tile out of the player's sight the buildings and trees
// it remembers there (doppelgangers) - and after it the records are finished and the music measured. collect_walk is
// that walk, the game's fills still making each record: the first step to a collection of the mod's own that reads
// a copy of the world the simulation makes, never the world (docs/renderer.md "The units' collection"). The update's
// first instruction of the walk (engine_display.units_walk, mov ecx, [picker_lock]) calls collect_thunk: with the walk
// the game's it does what the instruction did, else the walk is ours and the update goes on after the game's
// (units_walk_end). Its locals are the update's (its frame: walk_this ...). The calls the walk makes at the game's call
// sites go to what those sites call now - a hook there (walls.c's record fill, drawlist.c's search of the picker, the
// shadows' rows) is called as from the game's walk.
typedef void (__thiscall *uc_lock_fn)(void *cs);
internal uc_lock_fn g_uc_enter, g_uc_leave;
internal LONG g_uc_walks;   // walks of ours (the pipe's reply)
internal uint32_t g_uc_dopp;  // the doppelgangers our last walk filled (the lists' print)
__attribute__((used)) uintptr_t g_uc_lock_at, g_uc_end;   // the picker's lock, where the update goes on (collect_install)
__attribute__((used)) volatile int g_uc_own;              // the walk ours (the pipe)

__attribute__((used)) void __cdecl collect_walk(uint8_t *f) {
    units_walk w = units_walk_of(f);
    ee_ro_units *ro = w.ro;
    ee_player *pl = w.pl;
    ee_map *map = w.map;
    void *info = w.info, *dset = w.doppelgangers;
    void *dset_end = *(void **)((uint8_t *)dset + 4);   // the set's head: its find's answer when not met
    void *lock = *(void **)engine_display_picker_lock_at();
    ee_pick_list *picker = (ee_pick_list *)engine_display_pick_list_at();
    uint8_t *maps = engine_display_sort_maps_at();
    int free_cam = *game_free_camera_at() != 0;
    uint8_t *recs = draw_records(), *small = draw_small_records();
    uint32_t room = draw_room(), small_room = draw_small_room(), *n = draw_count_at(), *sn = draw_small_count_at();
    const void *mesh = map_mesh_at(map);
    g_uc_enter(lock);
    pick_list_reset(picker);
    uint32_t dopp = 0;
    unsigned long nrows = w.rows(mesh);
    for (unsigned long r = 0; r < nrows; r++) {
        long y, x0, x1;
        w.row(mesh, r, &y, &x0, &x1);
        for (long x = x0; x <= x1; x++) {
            uint32_t size = map_get_size(map);
            ee_tile *t = (uint32_t)x < size && (uint32_t)y < size ? map_get_tiles(map)[(uint32_t)y * size + (uint32_t)x] : NULL;
            ee_unit_vis *v = (ee_unit_vis *)t;
            if (!t || (!unit_vis_explored(v, pl) && !free_cam)) continue;
            for (void *node = tile_get_objects(t); node; node = tile_node_get_next(node)) {
                // an object made of parts (a ship and its sails ...): from its first part on
                ee_unit *o = tile_node_get_object(node);
                if (unit_get_physics(o)) {
                    o = unit_get_physics(o);
                    while (unit_get_chain(o)) o = unit_get_chain(o);
                }
                do {
                    if (ro_units_drawn(ro, o, v, w.met) && !w.listed(picker, o)) {
                        if (!unit_type_get_is_tree(unit_get_type_data(o))) {
                            if (unit_get_physics(o)) *w.complex_n += 1.0f;
                            uint8_t *rec = recs + (size_t)*n * DRAW_REC;
                            int see = ro_units_opacity(ro, o, draw_record_opacity(rec));
                            w.fill(ro, o, (float *)rec, maps + (see ? 0xC : 0), info);
                            ++*n;
                        } else {
                            uint8_t *rec = small + (size_t)*sn * DRAW_SMALL_REC;
                            int see = ro_units_opacity(ro, o, draw_small_record_opacity(rec));
                            ro_units_fill_object(ro, o, rec, maps + (see ? 0x30 : 0x24), info);
                            ++*sn;
                        }
                    }
                    o = unit_get_physics(o) ? unit_get_chained(unit_get_physics(o)) : NULL;
                } while (o);
            }
            if (unit_vis_seen_by(v, pl) || free_cam) continue;
            for (uint32_t i = 0; i < unit_vis_doppelganger_count(v); i++) {
                ee_doppelganger *d = tile_get_doppelgangers(t)[i];
                if (doppelganger_get_player(d) != pl) continue;
                ee_unit *u = doppelganger_get_unit(d);
                // a building under construction on the map: only its owner's
                if (unit_get_list_node(u) && unit_get_physics(u) && unit_is_foundation(unit_get_physics(u)) && unit_get_owner(u) != pl)
                    continue;
                ee_unit_type *ty = unit_get_type_data(u);
                if (unit_type_get_size(ty) > 1) {   // on several tiles: once
                    void *found;
                    if (*(void **)doppelganger_set_find(dset, &found, &d) != dset_end) continue;
                    doppelganger_set_insert(dset, &found, &d);
                }
                dopp++;
                if (!unit_type_get_is_tree(ty))
                    ro_units_fill_doppelganger(ro, d, recs + (size_t)*n * DRAW_REC, n,
                                               maps + (unit_type_get_is_obstacle(ty) ? 0xC : 0), room, (int)x, (int)y, info);
                else
                    ro_units_fill_doppelganger_object(ro, d, small + (size_t)*sn * DRAW_SMALL_REC, sn, maps + 0x24, small_room,
                                                      (int)x, (int)y, info);
            }
        }
    }
    g_uc_leave(lock);
    g_uc_dopp = dopp;
    g_uc_walks++;
}

// at the update's mov ecx, [picker_lock] (a call now): the game's walk goes on with ecx as it was to be; ours instead
// returns past it, to the update after the game's walk (the stack as the game's left it: the call's return dropped)
__attribute__((naked, used)) void collect_thunk(void) {
    __asm__("cmpl $0, _g_uc_own\n\t"
            "jne 1f\n\t"
            "movl _g_uc_lock_at, %ecx\n\t"
            "movl (%ecx), %ecx\n\t"
            "ret\n"
            "1:\n\t"
            "pushl %ebp\n\t"
            "call _collect_walk\n\t"
            "addl $8, %esp\n\t"
            "jmp *_g_uc_end");
}

internal int g_uc_installed;
internal void collect_install(void) {
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
    g_uc_enter = lle ? (uc_lock_fn)(void *)GetProcAddress(lle, "?Enter@TSCriticalSection@@QAEXXZ") : NULL;
    g_uc_leave = lle ? (uc_lock_fn)(void *)GetProcAddress(lle, "?Leave@TSCriticalSection@@QAEXXZ") : NULL;
    g_uc_lock_at = (uintptr_t)engine_display_picker_lock_at();
    g_uc_end = (uintptr_t)engine_display_units_walk_end_at();
    uint8_t want[6] = { 0x8B, 0x0D };   // mov ecx, [picker_lock]
    memcpy(want + 2, &g_uc_lock_at, 4);
    g_uc_installed = g_uc_enter && g_uc_leave && patch_insn6_call((uintptr_t)engine_display_units_walk_at(), want, collect_thunk);
    if (g_log) fprintf(g_log, "# collect: the units' walk %s (the game's until the pipe's picture collect own)\n",
                       g_uc_installed ? "hooked" : "NOT hooked (exe differs)");
}

// the pipe's picture collect own | game: 0 done, -1 not installed
internal int collect_own(int on) {
    if (!g_uc_installed) return -1;
    g_uc_own = on;
    return 0;
}

// THE LISTS' PRINT (the pipe's drawcount hash): what the last update collected, to hold the mod's walk against the
// game's in a paused game - the counts, the picker's, the sort maps' sizes and a hash of the records' contents but
// their vectors (+0x58 .. +0xDC: allocations of their own); taken on the render thread right after the units' update
internal void collect_print(void) {
    if (!g_uc_print_ask) return;
    g_uc_print_ask = 0;
    uint32_t n = *draw_count_at(), sn = *draw_small_count_at(), h = 2166136261u, hs = 2166136261u;
    const uint8_t *recs = draw_records(), *small = draw_small_records();
    for (uint32_t i = 0; i < n; i++) {
        const uint8_t *r = recs + (size_t)i * DRAW_REC;
        for (int k = 0; k < DRAW_REC; k++)
            if (k < 0x58 || k >= 0xDC) h = (h ^ r[k]) * 16777619u;
    }
    // the trees' by their dwords too: some of them sway with the clock (the paused game's trees move)
    uint32_t col[DRAW_SMALL_REC / 4];
    for (int k = 0; k < DRAW_SMALL_REC / 4; k++) col[k] = 2166136261u;
    for (uint32_t i = 0; i < sn * DRAW_SMALL_REC; i++) {
        hs = (hs ^ small[i]) * 16777619u;
        col[i % DRAW_SMALL_REC / 4] = (col[i % DRAW_SMALL_REC / 4] ^ small[i]) * 16777619u;
    }
    char cols[DRAW_SMALL_REC / 4 * 9 + 1];
    for (int k = 0; k < DRAW_SMALL_REC / 4; k++) snprintf(cols + k * 9, 10, " %08x", col[k]);
    log_locked("# collect: the trees' records by their dwords:%s\n", cols);
    const uint8_t *maps = engine_display_sort_maps_at();
    log_locked("# collect: the walk %s - objects %u (hash %08x), trees %u (hash %08x), picker %u, sort maps %u %u %u %u %u %u; doppelgangers %u (the mod's last walk)\n",
               g_uc_own ? "the mod's" : "the game's", n, h, sn, hs, pick_list_get_count((ee_pick_list *)engine_display_pick_list_at()),
               *(const uint32_t *)(maps + 8), *(const uint32_t *)(maps + 0x14), *(const uint32_t *)(maps + 0x20),
               *(const uint32_t *)(maps + 0x2C), *(const uint32_t *)(maps + 0x38), *(const uint32_t *)(maps + 0x44), g_uc_dopp);
}
