// THE TERRAIN PALETTE (`terrain_palette`, [editor]): a map's own ground colours. A corner of the ground is coloured
// by a slot of its mesh's colour table - 256 slots, textures\ColorPalette.tga - and the editor paints it from a colour
// set, one of db\dbterraintype.dat's 54 records, each its first to last slot (the corner's slot within the set by a
// noise of its place). The game's sets hold 255 of the slots, but 14 of them are colour for colour another set's ramp
// in another place (Grass 40-47 nine times over, Snow - Grass Patches 152-159 five, Ice 88-95 three). A new map's
// table is handed to its mesh after its sets are registered (map_iat_color_table): such a set then gets the first
// one's slots - the same colours, so painting looks as before, in the original game too -, and the slots no set
// holds any more are free for the map's own colours (96 with the game's data). A corner painted before (a loaded
// map) keeps its slot until tp_remap moves it to the one of the same colour its set paints now.
// The map's own colours are kept in its file (engine/map_file.c, record TPAL): version 1, u16 count, then per colour
// its slot and r, g, b (0..255). A load sets the new map's table up inside it, the record already read (the file is
// in memory first): the colours go in there; a map made otherwise (a random map, the editor's new map) has none.
#define TP_SLOTS 256
#define TP_VERSION 1
internal int g_tp_on;
internal mesh_color_table_fn g_tp_color_table;
internal uint8_t g_tp_to[TP_SLOTS];      // a slot -> the slot of the same colour its set paints now (itself if kept)
internal uint8_t g_tp_free[TP_SLOTS];    // 1: no set holds the slot
internal unsigned g_tp_sets, g_tp_shared, g_tp_nfree;
internal float g_tp_base[TP_SLOTS][3];   // the game's table, as the map's setup handed it over
internal uint8_t g_tp_own[TP_SLOTS];     // 1: a colour of the map's own (a free slot)
internal uint8_t g_tp_rgb[TP_SLOTS][3];
internal int map_file_loading(void);                                   // map_file.c
internal const uint8_t *mf_record(const char *tag, uint32_t *len);

// the map's own colours from a TPAL record (NULL: none) - only into slots free here; how many were not
internal unsigned tp_parse(const uint8_t *p, uint32_t len) {
    memset(g_tp_own, 0, sizeof g_tp_own);
    unsigned n = 0, skipped = 0;
    if (p && len >= 3 && p[0] == TP_VERSION) n = (unsigned)(p[1] | p[2] << 8);
    for (unsigned i = 0; i < n && 3 + i * 4 + 4 <= len; i++) {
        const uint8_t *e = p + 3 + i * 4;
        if (!g_tp_free[e[0]]) { skipped++; continue; }
        g_tp_own[e[0]] = 1;
        memcpy(g_tp_rgb[e[0]], e + 1, 3);
    }
    return skipped;
}

// the map's own colours into its mesh's table, the game's in the free slots that aren't
internal void tp_apply(void *mesh) {
    for (unsigned s = 0; s < TP_SLOTS; s++) {
        if (!g_tp_free[s]) continue;
        float *c = mesh_color(mesh, s);
        if (g_tp_own[s]) for (int k = 0; k < 3; k++) c[k] = (float)g_tp_rgb[s][k] / 255.0f;
        else memcpy(c, g_tp_base[s], sizeof g_tp_base[s]);
    }
}

internal unsigned tp_owned(void) {
    unsigned n = 0;
    for (unsigned s = 0; s < TP_SLOTS; s++) n += g_tp_own[s];
    return n;
}

// sets a and b of n slots: the same colours in the same order
internal int tp_same(void *mesh, const uint32_t *a, const uint32_t *b, unsigned n) {
    for (unsigned k = 0; k < n; k++)
        if (a[k] >= TP_SLOTS || b[k] >= TP_SLOTS || memcmp(mesh_color(mesh, a[k]), mesh_color(mesh, b[k]), 3 * sizeof(float))) return 0;
    return 1;
}

internal void __thiscall hook_tp_color_table(void *mesh, void *colors) {
    g_tp_color_table(mesh, colors);
    unsigned n;
    uint8_t *sets = mesh_color_sets(mesh, &n);
    for (unsigned s = 0; s < TP_SLOTS; s++) g_tp_to[s] = (uint8_t)s, g_tp_free[s] = 1;
    g_tp_sets = n;
    g_tp_shared = 0;
    for (unsigned i = 0; i < n; i++) {
        unsigned ni, nj;
        uint32_t *si = color_set_slots(sets, i, &ni);
        for (unsigned j = 0; j < i && ni; j++) {
            uint32_t *sj = color_set_slots(sets, j, &nj);
            if (nj != ni || si == sj || !tp_same(mesh, si, sj, ni)) continue;
            for (unsigned k = 0; k < ni; k++) g_tp_to[si[k]] = (uint8_t)sj[k];
            memcpy(si, sj, ni * sizeof *si);   // an earlier set's slots: shared already, if it shared
            g_tp_shared++;
            break;
        }
    }
    for (unsigned i = 0; i < n; i++) {
        unsigned ni;
        uint32_t *si = color_set_slots(sets, i, &ni);
        for (unsigned k = 0; k < ni; k++)
            if (si[k] < TP_SLOTS) g_tp_free[si[k]] = 0;
    }
    g_tp_nfree = 0;
    for (unsigned s = 0; s < TP_SLOTS; s++) {
        g_tp_nfree += g_tp_free[s];
        memcpy(g_tp_base[s], mesh_color(mesh, s), sizeof g_tp_base[s]);
    }
    uint32_t len = 0;
    unsigned skipped = tp_parse(map_file_loading() ? mf_record("TPAL", &len) : NULL, len);
    tp_apply(mesh);
    if (g_log) fprintf(g_log, "# terrain palette: %u of %u colour sets share an earlier one's slots, %u slots free; the map's own colours %u%s\n",
                       g_tp_shared, n, g_tp_nfree, tp_owned(), skipped ? " (some on slots not free here: left out)" : "");
}

// map_file.c's record TPAL: the map's own colours (0: none)
internal uint32_t tp_save(uint8_t *buf, uint32_t cap) {
    unsigned n = g_tp_on ? tp_owned() : 0;
    if (!n || 3 + n * 4 > cap) return 0;
    buf[0] = TP_VERSION, buf[1] = (uint8_t)n, buf[2] = (uint8_t)(n >> 8);
    uint8_t *e = buf + 3;
    for (unsigned s = 0; s < TP_SLOTS; s++)
        if (g_tp_own[s]) e[0] = (uint8_t)s, memcpy(e + 1, g_tp_rgb[s], 3), e += 4;
    return (uint32_t)(e - buf);
}

// after a load from a file: its record again (the map's setup inside the load took it already) - the map's table and
// the visible ground
internal void tp_load(const uint8_t *p, uint32_t len) {
    ee_map *map = game_map();
    if (!g_tp_on || !map) return;
    tp_parse(p, len);
    tp_apply(map_mesh_at(map));
    terrain_refresh();
}

// the map's corners on a free slot not its own to the slot of the same colour their set paints now; how many (on[s]:
// per slot before, if given)
internal unsigned tp_remap(ee_map *map, int move, unsigned *on) {
    unsigned moved = 0, size = map_get_size(map);
    for (unsigned y = 0; y <= size; y++) {
        const ee_corner_row *row = map_corner_row(map, y);
        for (unsigned i = 0, len = corner_row_get_len(row); i < len; i++) {
            ee_corner *c = corner_row_at(row, i);
            uint8_t s = corner_get_color(c);
            if (on) on[s]++;
            if (!g_tp_free[s] || g_tp_own[s] || g_tp_to[s] == s) continue;
            if (move) corner_set_color(c, g_tp_to[s]);
            moved++;
        }
    }
    if (move && moved) terrain_refresh();
    return moved;
}

internal void terrain_palette_install(void) {
    g_tp_on = g_set.terrain_palette;
    int ok = g_tp_on ? patch_mesh_color_table(hook_tp_color_table, &g_tp_color_table) : 0;
    if (g_log) fprintf(g_log, "# editor: terrain_palette=%d%s\n", g_tp_on, !g_tp_on ? "" : ok ? " (hooked)" : " (FAILED)");
}

// pipe: palette - the sets shared, the free slots and the corners on them, the map's own colours; palette
// get <slot>; palette set <slot> <r> <g> <b> (0..255; a free slot becomes one of the map's own colours, kept in its
// file - the corners painted on it before moved off first -, any other changes until the next map; shown at the next
// frame); palette clear <slot> (the game's colour again); palette remap (the corners on free slots moved)
internal void cmd_palette(char *out, const char *args) {
    ee_map *map = game_map();
    if (!map) { REPLY("error: no map\n"); return; }
    void *mesh = map_mesh_at(map);
    char mode[16] = "";
    int s = -1, r = 0, g = 0, b = 0;
    int n = sscanf(args, "%15s %d %d %d %d", mode, &s, &r, &g, &b);
    if (!strcmp(mode, "get") || !strcmp(mode, "set") || !strcmp(mode, "clear")) {
        if (n < 2 || s < 0 || s >= TP_SLOTS || (mode[0] == 's' && n < 5)) {
            REPLY("error: usage: palette get <slot> | set <slot> <r> <g> <b> | clear <slot>\n");
            return;
        }
        float *c = mesh_color(mesh, (unsigned)s);
        if (mode[0] == 's' && g_tp_on && g_tp_free[s]) {
            if (!g_tp_own[s]) tp_remap(map, 1, NULL);
            g_tp_own[s] = 1;
            g_tp_rgb[s][0] = (uint8_t)r, g_tp_rgb[s][1] = (uint8_t)g, g_tp_rgb[s][2] = (uint8_t)b;
            tp_apply(mesh);
        } else if (mode[0] == 's') c[0] = (float)r / 255.0f, c[1] = (float)g / 255.0f, c[2] = (float)b / 255.0f;
        else if (mode[0] == 'c' && g_tp_on && g_tp_free[s]) g_tp_own[s] = 0, tp_apply(mesh);
        if (mode[0] != 'g') terrain_refresh();
        REPLY("ok: slot %d %d %d %d%s\n", s, (int)(c[0] * 255.0f + 0.5f), (int)(c[1] * 255.0f + 0.5f), (int)(c[2] * 255.0f + 0.5f),
              g_tp_own[s] ? " (the map's own)" : g_tp_free[s] ? " (free)" : "");
        return;
    }
    if (!strcmp(mode, "remap")) { REPLY("ok: %u corners moved off free slots\n", tp_remap(map, 1, NULL)); return; }
    if (mode[0]) { REPLY("error: usage: palette [get <slot> | set <slot> <r> <g> <b> | clear <slot> | remap]\n"); return; }
    unsigned on[TP_SLOTS] = { 0 }, left = tp_remap(map, 0, on);
    REPLY("on=%d sets=%u shared=%u free=%u own=%u, corners to move off them %u:", g_tp_on, g_tp_sets, g_tp_shared, g_tp_nfree,
          tp_owned(), left);
    for (unsigned a = 0; a < TP_SLOTS; a++) {   // the free slots as runs, the corners on a run after it
        if (!g_tp_on || !g_tp_free[a]) continue;
        unsigned e = a, cnt = 0;
        while (e + 1 < TP_SLOTS && g_tp_free[e + 1]) e++;
        for (unsigned k = a; k <= e; k++) cnt += on[k];
        if (e > a) REPLY(" %u-%u", a, e);
        else REPLY(" %u", a);
        if (cnt) REPLY("(%u)", cnt);
        a = e;
    }
    if (tp_owned()) REPLY("; the map's own:");
    for (unsigned a = 0; a < TP_SLOTS; a++)
        if (g_tp_own[a]) REPLY(" %u=%02x%02x%02x", a, g_tp_rgb[a][0], g_tp_rgb[a][1], g_tp_rgb[a][2]);
    REPLY("\n");
}
