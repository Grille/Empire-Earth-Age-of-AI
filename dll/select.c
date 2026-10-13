// BIG SELECTIONS ([general] big_selection, default 1). Double-click (all units of a type on screen) and the
// selection box send a command message (0x4BD072, 0x138 bytes, ids 0x4E/0x4F/0x50) that carries the units found
// (0x50F589) packed into 256 bytes at +0x34, length at +0x134 (one byte); 0x4BD2C4 unpacks them (also when the
// message is carried out). The packer 0x4BD15A takes at most 63 units (local budget 0x3F) - with larger armies only
// a part got selected. Format: byte 0 = the player byte of the ids (player +0x45C, id bits 24..31), then runs of
// units sharing id bits 8..23: header 0x00|n (bits 8..23 zero), 0x40|n + 1 byte (bits 8..15), 0x80|n + 2 bytes
// (bits 8..23), n <= 63, then n bytes of id bits 0..7; an empty list is 0xFF. This packer writes the same format
// for as many units as fit into the 255 bytes the length byte allows (about 240), so the message stays readable for
// an unmodded game (multiplayer) and the game's own unpacker.
#define CALL_SEL_PACK 0x004BD133
#define FN_SEL_PACK   0x004BD15A
static int g_big_selection = 1;
static unsigned g_sel_last_in, g_sel_last_out;   // tests: units found / packed by the last pack

static int sel_cmp(const void *a, const void *b) {
    uint32_t x = (*(const uint32_t *)a >> 8) & 0xFFFF, y = (*(const uint32_t *)b >> 8) & 0xFFFF;
    return x < y ? -1 : x > y;
}
static unsigned __cdecl hook_sel_pack(uint8_t *list, uint32_t player_byte, uint8_t *out) {
    if (!g_big_selection)
        return ((unsigned (__cdecl *)(void *, uint32_t, uint8_t *))FN_SEL_PACK)(list, player_byte, out);
    uint32_t *v = FIELD(list, 4, uint32_t *), *ve = FIELD(list, 8, uint32_t *);
    static uint32_t ids[1024];
    unsigned n = 0;
    for (uint32_t *p = v; v && p < ve && n < 1024; p++) ids[n++] = *p;
    qsort(ids, n, sizeof ids[0], sel_cmp);   // runs of the same bits 8..23 (the game groups them in a map)
    unsigned pos = 0, packed = 0;
    out[pos++] = (uint8_t)player_byte;
    for (unsigned i = 0; i < n;) {
        uint32_t pre = (ids[i] >> 8) & 0xFFFF;
        unsigned run = 0;
        while (i + run < n && run < 63 && ((ids[i + run] >> 8) & 0xFFFF) == pre) run++;
        unsigned head = pre == 0 ? 1 : pre < 0x100 ? 2 : 3;
        if (pos + head + 1 > 255) break;
        if (pos + head + run > 255) run = 255 - pos - head;
        if (pre == 0) out[pos++] = (uint8_t)run;
        else if (pre < 0x100) { out[pos++] = (uint8_t)(0x40 | run); out[pos++] = (uint8_t)pre; }
        else { out[pos++] = (uint8_t)(0x80 | run); out[pos++] = (uint8_t)(pre >> 8); out[pos++] = (uint8_t)pre; }
        for (unsigned k = 0; k < run; k++) out[pos++] = (uint8_t)ids[i + k];
        i += run;
        packed += run;
    }
    if (pos == 1) out[pos++] = 0xFF;
    g_sel_last_in = n;
    g_sel_last_out = packed;
    return pos & 0xFF;
}

// seltest T [x0 y0 x1 y1]: builds the double-click message for unit type T (the screen rectangle 0x8D2778, or the
// given one) for the local player without sending it: units found, packed, unpacked. Tests.
static void cmd_seltest(char *out, const char *args) {
    int type = 0;
    float r[4];
    if (sscanf(args, "%d %f %f %f %f", &type, &r[0], &r[1], &r[2], &r[3]) < 5) memcpy(r, (void *)0x008D2778, sizeof r);
    static __attribute__((aligned(8))) uint8_t msg[0x140];
    memset(msg, 0, sizeof msg);
    g_sel_last_in = g_sel_last_out = 0;
    ((void (__thiscall *)(void *, void *, int, int))0x004BD072)(msg, r, type, 0x4E);
    uint32_t *b = FIELD(msg, 0x14, uint32_t *), *e = FIELD(msg, 0x18, uint32_t *);
    REPLY("rect %.0f %.0f %.0f %.0f: found %u, packed %u in %u bytes, unpacked %d (big_selection %d)", r[0], r[1], r[2],
          r[3], g_sel_last_in, g_sel_last_out, msg[0x134], b ? (int)(e - b) : 0, g_big_selection);
    // carried out as the message does (EEMSelectUnits::vf10 -> 0x55B3FB on the player): the selected group (+0x9BC)
    void *pl = ((void **)0x00918374)[*(unsigned *)0x00918E84];
    int *grp = pl ? FIELD(pl, 0x9BC, int *) : NULL;
    if (grp) {
        ((void (__thiscall *)(void *, void *))0x0055B3FB)(pl, msg + 0x10);
        REPLY(", selected %u", ((unsigned (__thiscall *)(void *))(*(void ***)grp)[6])(grp));
    }
    REPLY("\n");
}

static void select_install(const char *aimod_ini, const char *autotest_ini) {
    g_big_selection = GetPrivateProfileIntA("general", "big_selection", 1, aimod_ini);
    if (autotest_ini) g_big_selection = GetPrivateProfileIntA("autotest", "big_selection", g_big_selection, autotest_ini);
    BOOL ok = patch_call(CALL_SEL_PACK, FN_SEL_PACK, hook_sel_pack);
    if (g_log) fprintf(g_log, "# selection: big_selection=%d (hook %s)\n", g_big_selection, ok ? "ok" : "FAILED");
}
