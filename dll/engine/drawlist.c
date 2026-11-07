// DRAW LIMIT ([display] draw_limit, default 4096; 0 = the original 512). EEROUnits::vf0 (0x503B83) collects what
// is on screen each frame into a fixed array of 512 records (0x148 bytes, the global block at 0x8D2CB8: records at
// +0x78 = 0x8D2D30, count 0x912538); the render thread draws them (0x50BD9D). Units, buildings and every missile
// in flight or stuck in the ground share it (a 360-archer fight reached 445), so in big fights and at high
// resolutions the list was full and whatever came last - the farthest objects - blinked out. The small objects
// (type +0x619: decorations, 1536 records of 0x3C at 0x8FBD30) have their own array: moved the same way, three times
// the size as in the game (12288 at the default).
// The array is moved to a larger one: each record is built with the game's constructor (0x506442), and the
// references (found by a disassembly scan of the exe, all in the render code) and the two 512 limits point there.
// (DRAW_REC, DRAW_OLD_N and the operands: game/api/hooks.c patch_draw_list, patch_draw_small)
internal int g_draw_limit = 4096;
internal uint8_t *g_draw_recs, *g_draw_small_recs;

internal void drawlist_install(void) {
    g_draw_limit = g_set.draw_limit;
    if (g_draw_limit <= DRAW_OLD_N) {
        if (g_log) fprintf(g_log, "# draw limit: the original %d\n", DRAW_OLD_N);
        return;
    }
    if (g_draw_limit > 32768) g_draw_limit = 32768;
    int ok = 0, n = 0, r = patch_draw_list(g_draw_limit, &g_draw_recs, &ok, &n);
    if (!r) {
        if (g_log) fprintf(g_log, "# draw limit: exe differs (%d of %d operands), the original %d\n", ok, n, DRAW_OLD_N);
        return;
    }
    if (r < 0) {
        if (g_log) fprintf(g_log, "# draw limit: no memory, the original %d\n", DRAW_OLD_N);
        return;
    }
    if (g_log) fprintf(g_log, "# draw limit: %d objects on screen (was %d), records at %p\n", g_draw_limit, DRAW_OLD_N,
                       g_draw_recs);
    r = patch_draw_small(g_draw_limit * 3, &g_draw_small_recs, &ok, &n);
    if (g_log && r > 0) fprintf(g_log, "# draw limit: %d small objects (was %d)\n", g_draw_limit * 3, DRAW_SMALL_OLD_N);
    else if (g_log) fprintf(g_log, "# draw limit: small objects the original %d (%s)\n", DRAW_SMALL_OLD_N,
                            r ? "no memory" : "exe differs");
}

// the draw list is the larger one (render_shadows.c: the casters off the screen collected only then)
internal int draw_list_moved(void) { return g_draw_recs != NULL; }

// DRAWING MANY UNITS ([patches] draw_many_units). Two costs of the collection's that grow with what is on the screen
// (the 8000-unit save: 1000 objects drawn a frame, the drawing thread a third in the collection, 2026-11-06):
// - before it draws an object it asks whether its list of what it drew has it already (hooks_functions.pick_listed: an
//   object on several tiles is met again) - a search through the whole list, 1175 entries on average: the list is
//   mirrored in a hash set instead, made anew when the list starts over (fewer entries or another array) and topped
//   up with the entries added since;
// - each object drawn takes a user of its type's pictures (EEGraphicObject.acquire), and acquire makes and frees 16
//   empty maps whatever it does with them - 32 allocations a call, used only by the first user, who loads the models:
//   a user more of loaded models is counted directly, under the engine's lock as acquire does.
#define PL_SLOTS (1 << 16)   // the set's room: two thirds full at most, else the game's search
internal ee_unit *g_pl_set[PL_SLOTS];
internal const ee_pick_entry *g_pl_entries;   // the list's array the set holds
internal uint32_t g_pl_n;                     // its entries in it
internal ee_unit *g_pl_ends[2];               // the first and the last of them: the list filled anew to as many
internal uint8_t __thiscall hook_pick_listed(void *p, ee_unit *u) {
    ee_pick_list *l = p;
    uint32_t n = pick_list_get_count(l);
    if (n > PL_SLOTS * 2 / 3) return pick_listed(p, u);
    if (n && (pick_list_get_entries(l) != g_pl_entries || (g_pl_n && n >= g_pl_n &&
        (pick_entry_get_unit(pick_list_entry(l, 0)) != g_pl_ends[0] ||
         pick_entry_get_unit(pick_list_entry(l, g_pl_n - 1)) != g_pl_ends[1]))))
        g_pl_n = n + 1;   // another array, other entries: all anew
    if (n < g_pl_n) {
        memset(g_pl_set, 0, sizeof g_pl_set);
        g_pl_n = 0;
        g_pl_entries = n ? pick_list_get_entries(l) : NULL;
    }
    for (; g_pl_n < n; g_pl_n++) {
        ee_unit *e = pick_entry_get_unit(pick_list_entry(l, g_pl_n));
        uint32_t h = (uint32_t)((uintptr_t)e >> 3) * 2654435761u >> 16;
        while (g_pl_set[h] && g_pl_set[h] != e) h = (h + 1) & (PL_SLOTS - 1);
        g_pl_set[h] = e;
        if (!g_pl_n) g_pl_ends[0] = e;
        g_pl_ends[1] = e;
    }
    uint32_t h = (uint32_t)((uintptr_t)u >> 3) * 2654435761u >> 16;
    for (; g_pl_set[h]; h = (h + 1) & (PL_SLOTS - 1))
        if (g_pl_set[h] == u) return 1;
    return 0;
}
typedef void (__thiscall *ts_lock_fn)(void *cs);
internal ts_lock_fn g_ts_enter, g_ts_leave;
internal dmg_model_acquire_fn g_acquire;   // the game's
internal int __thiscall hook_acquire(ee_dmg_model *g) {
    void *cs = graphics_lock();
    if (cs) {
        g_ts_enter(cs);
        int users = dmg_model_get_users(g);
        if (users && !dmg_model_get_loading(g)) {
            dmg_model_set_users(g, users + 1);
            g_ts_leave(cs);
            return 0;
        }
        g_ts_leave(cs);
    }
    return g_acquire(g);
}
internal void draw_many_install(void) {
    if (!g_set.draw_many_units) { if (g_log) fprintf(g_log, "# draw many units: off\n"); return; }
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
    g_ts_enter = lle ? (ts_lock_fn)(void *)GetProcAddress(lle, "?Enter@TSCriticalSection@@QAEXXZ") : NULL;
    g_ts_leave = lle ? (ts_lock_fn)(void *)GetProcAddress(lle, "?Leave@TSCriticalSection@@QAEXXZ") : NULL;
    int a = patch_pick_listed(hook_pick_listed), b = g_ts_enter && g_ts_leave && patch_acquire(hook_acquire, &g_acquire);
    if (g_log) fprintf(g_log, "# draw many units: the list's search %s, acquire %s\n", a ? "hooked" : "NOT hooked (exe differs)",
                       b ? "hooked" : "NOT hooked (exe differs)");
}

// drawcount: objects collected for the last frame (units etc. / small objects). Tests.
// drawcount hash: the last update's lists into the log (collect.c collect_print: the mod's walk against the game's)
internal volatile LONG g_uc_print_ask;   // collect.c
internal void cmd_drawcount(char *out, const char *args) {
    while (*args == ' ') args++;
    if (!strncmp(args, "hash", 4)) { g_uc_print_ask = 1; REPLY("ok: the next update's lists go to the log (# collect)\n"); return; }
    REPLY("objects %u of %d, small objects %u of %d\n", draw_count(),
          g_draw_recs ? g_draw_limit : DRAW_OLD_N, draw_small_count(), g_draw_small_recs ? g_draw_limit * 3 : DRAW_SMALL_OLD_N);
}
