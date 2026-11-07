// THE MOD'S BLOCK IN A MAP FILE: a scenario or a saved game (.scn, .ees: one format) carries the mod's own data at
// its end, after the game's lumps - records {tag (4 characters), u32 length, the bytes}, then u32 the block's length
// (this trailer counted) and "AoAI". The game's loader reads the lumps its header's table lists and nothing after
// them (2026-10-22: a lump appended unlisted is never read, the game plays as from the file without it): the original
// game loads such a file as if the block weren't there. Written by the save's writer (save_write: a save to disk,
// the editor's Save too, and the editor's quicksave its test play starts from) into the same file after the game's
// lumps; read when a game loads from a file (load_file), which takes the whole file into memory first
// (iat_memfile_load): the block from that memory's end. A record's owner reads it after the load (mf_record).
#define MF_MAGIC 0x49416F41u   // "AoAI" in the file
#define MF_MAX (1u << 20)      // a block larger is no block of ours
internal int g_mf_on;
internal memfile_load_fn g_mf_memfile_load;
internal int g_mf_loading;              // inside load_file
internal uint8_t *g_mf_block;           // the loaded file's records, NULL none
internal uint32_t g_mf_len;

// the records the block is made of: each owner's tag, what it writes (its length; 0 nothing) and what it reads after a
// load (NULL, 0: the file had none)
internal uint32_t tp_save(uint8_t *buf, uint32_t cap);           // terrain_palette.c: the map's own colours
internal void tp_load(const uint8_t *p, uint32_t len);
internal const struct { char tag[5]; uint32_t (*save)(uint8_t *buf, uint32_t cap); void (*load)(const uint8_t *p, uint32_t len); } k_mf_records[] = {
    { "TPAL", tp_save, tp_load },
};

internal int mf_put(ee_fs_file *f, const void *p, uint32_t n) {
    uint32_t put = 0;
    return fs_file_write(f, (uint8_t *)p, n, &put) == 0 && put == n;
}

internal int __cdecl hook_mf_save_write(void *objects, uint32_t time, ee_fs_file *header, ee_fs_file *file) {
    int r = save_write(objects, time, header, file);
    if (r != 0 || !file) return r;
    static uint8_t buf[1 << 16];
    uint32_t total = 0, n = 0;
    int ok = 1;
    for (size_t i = 0; i < sizeof k_mf_records / sizeof *k_mf_records && ok; i++) {
        uint32_t len = k_mf_records[i].save(buf, sizeof buf);
        if (!len) continue;
        ok = mf_put(file, k_mf_records[i].tag, 4) && mf_put(file, &len, 4) && mf_put(file, buf, len);
        total += 8 + len;
        n++;
    }
    if (n && ok) {
        uint32_t trailer[2] = { total + 8, MF_MAGIC };
        ok = mf_put(file, trailer, sizeof trailer);
    }
    if (n && g_log) fprintf(g_log, "# map file: the mod's block written, %u records, %u bytes%s\n", n, total + 8, ok ? "" : " - FAILED");
    return r;
}

// the block at the end of a file loaded into memory, its position left as it was
internal void mf_read(ee_fs_file *mem) {
    uint32_t len = 0, tr[2] = { 0, 0 }, got = 0;
    int at = 0;
    if (fs_file_get_length(mem, &len) || len < 8 || fs_file_get_position(mem, &at)) return;
    if (!fs_file_set_position(mem, 8, 1, 0) && !fs_file_read(mem, (uint8_t *)tr, 8, &got) && got == 8 && tr[1] == MF_MAGIC &&
        tr[0] > 8 && tr[0] <= len && tr[0] <= MF_MAX && (g_mf_block = (uint8_t *)malloc(tr[0] - 8)) &&
        !fs_file_set_position(mem, (int)tr[0], 1, 0) && !fs_file_read(mem, g_mf_block, tr[0] - 8, &got) && got == tr[0] - 8)
        g_mf_len = tr[0] - 8;
    else free(g_mf_block), g_mf_block = NULL;
    fs_file_set_position(mem, at, 0, 0);
}

internal int __thiscall hook_mf_memfile_load(ee_fs_file *mem, ee_fs_file *from) {
    int r = g_mf_memfile_load(mem, from);
    if (r == 0 && g_mf_loading && !g_mf_block) mf_read(mem);
    return r;
}

// the loaded file's record of tag, NULL none (a record that runs past the block ends the search)
internal const uint8_t *mf_record(const char *tag, uint32_t *len) {
    for (uint32_t o = 0; g_mf_block && o + 8 <= g_mf_len;) {
        uint32_t n;
        memcpy(&n, g_mf_block + o + 4, 4);
        if (n > g_mf_len - o - 8) break;
        if (!memcmp(g_mf_block + o, tag, 4)) { *len = n; return g_mf_block + o + 8; }
        o += 8 + n;
    }
    *len = 0;
    return NULL;
}

internal uint8_t __thiscall hook_mf_load_file(void *self, ee_fs_file *file) {
    free(g_mf_block), g_mf_block = NULL, g_mf_len = 0;
    g_mf_loading = 1;
    uint8_t r = load_file(self, file);
    g_mf_loading = 0;
    if (g_log && g_mf_block) fprintf(g_log, "# map file: the mod's block read, %u bytes\n", g_mf_len + 8);
    for (size_t i = 0; r && i < sizeof k_mf_records / sizeof *k_mf_records; i++) {
        uint32_t len;
        const uint8_t *p = mf_record(k_mf_records[i].tag, &len);
        k_mf_records[i].load(p, len);
    }
    return r;
}

// a game load from a file under way (a new map's setup then belongs to it)
internal int map_file_loading(void) { return g_mf_loading; }

// with the terrain palette, its only user so far
internal void map_file_install(void) {
    g_mf_on = g_set.terrain_palette;
    if (!g_mf_on) return;
    int of = 0, n = patch_save_write(hook_mf_save_write, &of) + patch_load_file(hook_mf_load_file, &of);
    int m = patch_memfile_load(hook_mf_memfile_load, &g_mf_memfile_load);
    if (g_log) fprintf(g_log, "# map file: the mod's block in scenarios and saves, calls %d/%d, the memory file's load %s\n", n, of, m ? "hooked" : "FAILED");
}
