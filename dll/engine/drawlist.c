// DRAW LIMIT ([display] draw_limit, default 4096; 0 = the original 512). EEROUnits::vf0 (0x503B83) collects what
// is on screen each frame into a fixed array of 512 records (0x148 bytes, the global block at 0x8D2CB8: records at
// +0x78 = 0x8D2D30, count 0x912538); the render thread draws them (0x50BD9D). Units, buildings and every missile
// in flight or stuck in the ground share it (a 360-archer fight reached 445), so in big fights and at high
// resolutions the list was full and whatever came last - the farthest objects - blinked out. The small objects
// (trees, decorations: 1536 records of 0x3C at 0x8FBD30) have their own array and are not changed.
// The array is moved to a larger one: each record is built with the game's constructor (0x506442), and the
// references (found by a disassembly scan of the exe, all in the render code) and the two 512 limits point there.
// (DRAW_REC, DRAW_OLD_N and the operands: game/api/hooks.c patch_draw_list)
internal int g_draw_limit = 4096;
internal uint8_t *g_draw_recs;

internal void drawlist_install(const char *aimod_ini, const char *autotest_ini) {
    g_draw_limit = ini_int("display", "draw_limit", 4096, aimod_ini);
    if (autotest_ini) g_draw_limit = ini_int("autotest", "draw_limit", g_draw_limit, autotest_ini);
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
}

// drawcount: objects collected for the last frame (units etc. / small objects). Tests.
internal void cmd_drawcount(char *out, const char *args) {
    (void)args;
    REPLY("objects %u of %d, small objects %u of 1536\n", draw_count(),
          g_draw_recs ? g_draw_limit : DRAW_OLD_N, draw_small_count());
}
