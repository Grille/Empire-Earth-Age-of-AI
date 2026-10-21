// DRAW LIMIT ([display] draw_limit, default 4096; 0 = the original 512). EEROUnits::vf0 (0x503B83) collects what
// is on screen each frame into a fixed array of 512 records (0x148 bytes, the global block at 0x8D2CB8: records at
// +0x78 = 0x8D2D30, count 0x912538); the render thread draws them (0x50BD9D). Units, buildings and every missile
// in flight or stuck in the ground share it (a 360-archer fight reached 445), so in big fights and at high
// resolutions the list was full and whatever came last - the farthest objects - blinked out. The small objects
// (trees, decorations: 1536 records of 0x3C at 0x8FBD30) have their own array and are not changed.
// The array is moved to a larger one: each record is built with the game's constructor (0x506442), and the
// references (found by a disassembly scan of the exe, all in the render code) and the two 512 limits point there.
#define DRAW_REC 0x148
#define DRAW_OLD_N 0x200
static int g_draw_limit = 4096;
static uint8_t *g_draw_recs;

static void drawlist_install(const char *aimod_ini, const char *autotest_ini) {
    g_draw_limit = ini_int("display", "draw_limit", 4096, aimod_ini);
    if (autotest_ini) g_draw_limit = ini_int("autotest", "draw_limit", g_draw_limit, autotest_ini);
    if (g_draw_limit <= DRAW_OLD_N) {
        if (g_log) fprintf(g_log, "# draw limit: the original %d\n", DRAW_OLD_N);
        return;
    }
    if (g_draw_limit > 32768) g_draw_limit = 32768;
    // the operands to move, checked first (all or nothing)
    const uint32_t *at = GL(DRAW_SITES);   // in this order (addr.h)
    const struct { uint32_t at, want; int kind; } k[] = {
        { at[0], 0x200, 0 },                     // cmp [0x912538], 0x200 (0x5008B8: room for one more)
        { at[1], 0x200, 0 },                     // push 0x200 (0x5035AD: its sub-objects)
        { at[2], GA(DRAW_OLD) + OF(DRAWREC, 0x58), 1 },       // mov esi, records +0x58 (init loop at game start)
        { at[3], GA(DRAW_OLD) + DRAW_OLD_N * DRAW_REC + OF(DRAWREC, 0x58), 2 },   // cmp esi, its end
        { at[4], GA(DRAW_OLD), 1 },              // push records (0x500D10: frame start)
        { at[5], GA(DRAW_OLD), 1 },              // mov edi, records (collection)
        { at[6], GA(DRAW_OLD) + OF(DRAWREC, 0xFC), 1 },       // add eax, records +0xFC
        { at[7], GA(DRAW_OLD), 1 },              // push records (frame end)
        { at[8], GA(DRAW_OLD), 1 },              // push records (0x50BD9D: drawing)
    };
    int n = (int)(sizeof k / sizeof k[0]), ok = 0;
    for (int i = 0; i < n; i++) ok += *(uint32_t *)(uintptr_t)k[i].at == k[i].want;
    if (ok != n) {
        if (g_log) fprintf(g_log, "# draw limit: exe differs (%d of %d operands), the original %d\n", ok, n, DRAW_OLD_N);
        return;
    }
    g_draw_recs = VirtualAlloc(NULL, (SIZE_T)g_draw_limit * DRAW_REC, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!g_draw_recs) {
        if (g_log) fprintf(g_log, "# draw limit: no memory, the original %d\n", DRAW_OLD_N);
        return;
    }
    for (int i = 0; i < g_draw_limit; i++)
        GF(FN_DRAW_REC_CTOR)(g_draw_recs + (size_t)i * DRAW_REC);
    uint32_t base = (uint32_t)(uintptr_t)g_draw_recs;
    for (int i = 0; i < n; i++) {
        uint32_t v = k[i].kind == 0 ? (uint32_t)g_draw_limit
                   : k[i].kind == 1 ? base + (k[i].want - GA(DRAW_OLD))
                   : base + (uint32_t)g_draw_limit * DRAW_REC + 0x58;
        patch_code32(k[i].at, k[i].want, v);
    }
    if (g_log) fprintf(g_log, "# draw limit: %d objects on screen (was %d), records at %p\n", g_draw_limit, DRAW_OLD_N,
                       g_draw_recs);
}

// drawcount: objects collected for the last frame (units etc. / small objects). Tests.
static void cmd_drawcount(char *out, const char *args) {
    (void)args;
    REPLY("objects %u of %d, small objects %u of 1536\n", *(uint32_t *)GA(G_DRAW_N),
          g_draw_recs ? g_draw_limit : DRAW_OLD_N, *(uint32_t *)GA(G_DRAW_SMALL_N));
}
