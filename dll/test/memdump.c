// ---- memory snapshots: raw dumps of a player's AI objects, for diffing working vs broken games ----
// Autotest, or aimod.ini [general] memory_dumps=1: 2 files per AI and game, never deleted (17,000 files / 0.4 GB
// in normal play before) - off for players.
static char g_dump_dir[MAX_PATH];   // also the folder of the control pipe's shot / ddinfo dump pictures
static int g_mem_dumps;
static DWORD g_session;

// Copy `len` bytes of our own memory without risking an access violation on unmapped pages.
static void dump_region(FILE *f, const char *tag, const void *p, uint32_t len) {
    static uint8_t buf[0x2000];
    SIZE_T got = 0;
    if (len > sizeof buf) len = sizeof buf;
    memset(buf, 0, len);   // the static buffer held the last region
    if (p) ReadProcessMemory(GetCurrentProcess(), p, buf, len, &got);
    uint32_t hdr[3] = { (uint32_t)(uintptr_t)p, len, (uint32_t)got };
    char name[16] = {0};
    lstrcpynA(name, tag, sizeof name);
    fwrite(name, 1, sizeof name, f);
    fwrite(hdr, sizeof hdr, 1, f);
    fwrite(buf, 1, len, f);   // unreadable tail stays zero; `got` records how much was valid
}

static void dump_player(void *rm, uint32_t now) {
    void *pl = rm_player(rm);
    void *cm = FIELD(rm, RM_OWNER, void *);
    if (!pl || !cm || !g_dump_dir[0] || !g_mem_dumps) return;
    char path[MAX_PATH];
    static unsigned run;   // distinguishes several games within one session (game times repeat)
    static uint32_t last_now = 0xFFFFFFFF;
    if (now < last_now) run++;
    last_now = now;
    wsprintfA(path, "%s\\s%08lx_r%u_P%d_t%06u.bin", g_dump_dir, g_session, run, FIELD(pl, PL_INDEX, int), now);
    FILE *f = fopen(path, "wb");
    if (!f) return;
    dump_region(f, "player", pl, 0x1000);
    dump_region(f, "civmgr", cm, 0x100);
    // sub-managers referenced by the civilization manager (+0x38 = resource manager, ...)
    for (int off = 0x30; off <= 0x50; off += 4) {
        char tag[16];
        wsprintfA(tag, "cm+%02x", off);
        dump_region(f, tag, FIELD(cm, off, void *), 0x400);
    }
    dump_region(f, "pl+b8c", FIELD(pl, 0xB8C, void *), 0x400);
    dump_region(f, "pl+b90", FIELD(pl, 0xB90, void *), 0x400);
    dump_region(f, "pl+9d4", FIELD(pl, PL_TECHTREE, void *), 0x800);
    // towns / resource centers (player +0xB50 vector of pointers)
    void **tb = FIELD(pl, 0xB50, void **), **te = FIELD(pl, 0xB54, void **);
    for (int i = 0; tb && tb + i < te && i < 32; i++) {
        char tag[16];
        wsprintfA(tag, "town%02d", i);
        dump_region(f, tag, tb[i], 0x100);
    }
    fclose(f);
}
