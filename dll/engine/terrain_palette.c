// THE TERRAIN PALETTE (`terrain_palette`, [editor]): a map's own ground colours. A corner of the ground is coloured
// by a slot of its mesh's colour table - 256 slots, textures\ColorPalette.tga - and the editor paints it from a colour
// set, one of db\dbterraintype.dat's 54 records, each its first to last slot (the corner's slot within the set by a
// noise of its place). The game's sets hold 255 of the slots, but 14 of them are colour for colour another set's ramp
// in another place (Grass 40-47 nine times over, Snow - Grass Patches 152-159 five, Ice 88-95 three). A new map's
// table is handed to its mesh after its sets are registered (map_iat_color_table): such a set then gets the first
// one's slots - the same colours, so painting looks as before, in the original game too -, and the slots no set
// holds any more are free for the map's own colours (96 with the game's data). A corner painted before (a loaded
// map) keeps its slot until tp_remap moves it to the one of the same colour its set paints now.
// The map's own colour types: a list of free slots each, coloured flat (one slot), as a ramp between two colours or
// slot by slot, registered with the mesh as sets of their own after the game's - the editor's colour list shows them
// after the game's (the terrain panel's colour types and the list's items: an item of the mod's own each, its type a
// record of the mod's own with an id from TP_ID), and the paint with one of them gives map_paint_colors a game's type
// whose set is for that call the map's own's. A type given fewer slots or taken out moves the corners on the slots it
// gives back to the nearest colour of another set's.
// The map's own colours are kept in its file (engine/map_file.c, record TPAL): version 1, u16 count, then per colour
// its slot and r, g, b (0..255); then u8 the colour types, each its mode (tp_mode; a ramp's blend in the high 4 bits,
// tp_blend: 0 sRGB, so a record from before it reads the same), u8 n and its n slots. A load sets
// the new map's table up inside it, the record already read (the file is in memory first): the colours go in there;
// a map made otherwise (a random map, the editor's new map) has none.
#define TP_SLOTS 256
#define TP_VERSION 1
#define TP_TYPES 96          // the map's own colour types at most (a slot each at least)
#define TP_TYPE_SLOTS 16     // a type's slots at most (the game's have 8)
#define TP_ID 0x7000         // the map's own types' ids (the game's: their place in map.color_types)
enum tp_mode { TP_FLAT, TP_RAMP, TP_EACH };
enum tp_blend { TP_SRGB, TP_LINEAR, TP_OKLAB };   // how a ramp's middle is blended (tp_ramp)
struct tp_type { uint8_t mode, blend, n, slot[TP_TYPE_SLOTS]; unsigned set; };
internal int g_tp_on;
internal mesh_color_table_fn g_tp_color_table;
internal uint8_t g_tp_to[TP_SLOTS];      // a slot -> the slot of the same colour its set paints now (itself if kept)
internal uint8_t g_tp_free[TP_SLOTS];    // 1: no set holds the slot
internal unsigned g_tp_sets, g_tp_shared, g_tp_nfree;
internal float g_tp_base[TP_SLOTS][3];   // the game's table, as the map's setup handed it over
internal uint8_t g_tp_own[TP_SLOTS];     // 1: a colour of the map's own (a free slot)
internal uint8_t g_tp_rgb[TP_SLOTS][3];
internal struct tp_type g_tp_type[TP_TYPES];   // the map's own colour types
internal unsigned g_tp_types;
internal uint8_t g_tp_rec[TP_TYPES + 1][256];  // their records in the terrain panel (only the id read), the "new" item's last
internal int g_tp_setup_in_load;               // its setup was a load's (the record taken already)
internal ee_se_terrain *g_tp_panel;            // the editor's terrain panel, while it is
internal unsigned g_tp_list_base;              // the game's items in its colour list
internal int map_file_loading(void);                                   // map_file.c
internal const uint8_t *mf_record(const char *tag, uint32_t *len);

// the map's own colours and colour types from a TPAL record (NULL: none) - only into slots free here, a type only of
// the map's own colours; how many were left out
internal unsigned tp_parse(const uint8_t *p, uint32_t len) {
    memset(g_tp_own, 0, sizeof g_tp_own);
    g_tp_types = 0;
    unsigned n = 0, skipped = 0, o = 3;
    if (p && len >= 3 && p[0] == TP_VERSION) n = (unsigned)(p[1] | p[2] << 8);
    for (unsigned i = 0; i < n && o + 4 <= len; i++, o += 4) {
        const uint8_t *e = p + o;
        if (!g_tp_free[e[0]]) { skipped++; continue; }
        g_tp_own[e[0]] = 1;
        memcpy(g_tp_rgb[e[0]], e + 1, 3);
    }
    unsigned types = p && o < len ? p[o++] : 0;
    for (unsigned i = 0; i < types && o + 2 <= len && o + 2 + p[o + 1] <= len; i++, o += 2 + p[o + 1]) {
        struct tp_type t = { .mode = p[o] & 15, .blend = p[o] >> 4, .n = p[o + 1] };
        int ok = t.mode <= TP_EACH && t.blend <= TP_OKLAB && t.n >= 1 && t.n <= TP_TYPE_SLOTS && g_tp_types < TP_TYPES;
        for (unsigned k = 0; ok && k < t.n; k++) ok = g_tp_own[t.slot[k] = p[o + 2 + k]];
        if (ok) g_tp_type[g_tp_types++] = t;
        else skipped++;
    }
    return skipped;
}

// the map's own colour types' sets into the mesh
internal void tp_register(void *mesh) {
    for (unsigned i = 0; i < g_tp_types; i++) {
        uint32_t s[TP_TYPE_SLOTS];
        for (unsigned k = 0; k < g_tp_type[i].n; k++) s[k] = g_tp_type[i].slot[k];
        g_tp_type[i].set = mesh_register_color_set(mesh, s, g_tp_type[i].n);
    }
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

// the editor's colour list: the game's items, then the map's own types' (their items and records anew)
internal void tp_list_sync(void) {
    ee_se_terrain *p = g_tp_panel;
    ee_se_color_list *list = p ? se_terrain_get_color_list(p) : NULL;
    if (!list) return;
    int sel = se_color_list_get_selected(list);
    while (se_terrain_color_type_count(p) > g_tp_list_base) se_terrain_color_type_pop(p);
    while (se_color_list_count(list) > g_tp_list_base) se_color_list_remove(list, se_color_list_count(list) - 1);
    for (unsigned i = 0; i < g_tp_types; i++) {
        float c[TP_TYPE_SLOTS][3];
        for (unsigned k = 0; k < g_tp_type[i].n; k++)
            for (int j = 0; j < 3; j++) c[k][j] = (float)g_tp_rgb[g_tp_type[i].slot[k]][j] / 255.0f;
        se_color_list_add_colors(list, c, g_tp_type[i].n);
        ee_color_type *t = (ee_color_type *)g_tp_rec[i];
        color_type_set_id(t, TP_ID + i);
        color_type_set_set(t, g_tp_type[i].set);
        color_type_set_in_editor(t, 1);
        se_terrain_color_type_push(p, t);
    }
    float hue[12][3];   // the "new" item (terrain_dialog.c): the hues round
    for (unsigned k = 0; k < 12; k++) {
        float h = (float)k / 2.0f, f = h - (float)(int)h;
        float rise[6][3] = { { 1, f, 0 }, { 1 - f, 1, 0 }, { 0, 1, f }, { 0, 1 - f, 1 }, { f, 0, 1 }, { 1, 0, 1 - f } };
        memcpy(hue[k], rise[(int)h], sizeof hue[k]);
    }
    se_color_list_add_colors(list, hue, 12);
    ee_color_type *t = (ee_color_type *)g_tp_rec[TP_TYPES];
    color_type_set_id(t, TP_ID + TP_TYPES);
    color_type_set_in_editor(t, 1);
    se_terrain_color_type_push(p, t);
    unsigned n = se_color_list_count(list) - 1;   // without the "new" item
    if (sel >= (int)g_tp_list_base && n) se_color_list_select(list, (unsigned)sel < n ? (unsigned)sel : n - 1, 0, 0);
}

internal void *__thiscall hook_tp_panel_init(ee_se_terrain *p, uint32_t a, uint32_t b, int *index, ee_ui_form *form, void *font) {
    void *r = se_terrain_init(p, a, b, index, form, font);
    ee_se_color_list *list = se_terrain_get_color_list(p);
    unsigned types = se_terrain_color_type_count(p), items = list ? se_color_list_count(list) : 0;
    if (list && types == items && color_type_size() <= sizeof *g_tp_rec) {
        g_tp_panel = p;
        g_tp_list_base = types;
        tp_list_sync();
    }
    if (g_log) log_locked("# terrain palette: the editor's colour list, %u of the game's%s, the map's own %u\n", items,
                          g_tp_panel == p ? "" : " (its types differ: left alone)", g_tp_panel == p ? g_tp_types : 0);
    return r;
}

internal void *__thiscall hook_tp_panel_delete(ee_se_terrain *p, uint8_t flags) {
    if (p == g_tp_panel) g_tp_panel = NULL;
    return eeuifse_terrain_destroy(p, flags);
}

// the editor's paint with a colour type: one of the map's own painted as a game's whose set is for the call the
// type's (map_paint_colors reads only the set of the type it looks up)
internal void *tp_mesh(void);
internal unsigned g_tp_paints[3];   // the paint's calls: with a game's type, one of the map's own, none (the pipe's status)
internal int g_tp_paint_last = -1;
internal void __thiscall hook_tp_paint_colors(ee_map *map, int x_lo, int x_hi, int y_hi, int y_lo, int type) {
    ee_color_type *t = type >= TP_ID ? color_type(0) : NULL;
    g_tp_paint_last = type;
    if (type < TP_ID) { g_tp_paints[0]++; map_paint_colors(map, x_lo, x_hi, y_hi, y_lo, type); return; }
    g_tp_paints[!t || (unsigned)(type - TP_ID) >= g_tp_types ? 2 : 1]++;
    if (!t || (unsigned)(type - TP_ID) >= g_tp_types || !tp_mesh()) return;   // the "new" item, a type gone meanwhile: no colour
    uint32_t keep = color_type_get_set(t);
    color_type_set_set(t, g_tp_type[type - TP_ID].set);
    map_paint_colors(map, x_lo, x_hi, y_hi, y_lo, 0);
    color_type_set_set(t, keep);
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
    unsigned skipped = tp_parse((g_tp_setup_in_load = map_file_loading()) ? mf_record("TPAL", &len) : NULL, len);
    tp_apply(mesh);
    tp_register(mesh);
    if (g_log) fprintf(g_log, "# terrain palette: %u of %u colour sets share an earlier one's slots, %u slots free; the map's own colours %u, colour types %u%s\n",
                       g_tp_shared, n, g_tp_nfree, tp_owned(), g_tp_types, skipped ? " (some on slots not free here: left out)" : "");
    tp_list_sync();
}

// map_file.c's record TPAL: the map's own colours and colour types (0: none)
internal uint32_t tp_save(uint8_t *buf, uint32_t cap) {
    unsigned n = g_tp_on ? tp_owned() : 0;
    if (!n || 4 + n * 4 + g_tp_types * (2 + TP_TYPE_SLOTS) > cap) return 0;
    buf[0] = TP_VERSION, buf[1] = (uint8_t)n, buf[2] = (uint8_t)(n >> 8);
    uint8_t *e = buf + 3;
    for (unsigned s = 0; s < TP_SLOTS; s++)
        if (g_tp_own[s]) e[0] = (uint8_t)s, memcpy(e + 1, g_tp_rgb[s], 3), e += 4;
    *e++ = (uint8_t)g_tp_types;
    for (unsigned i = 0; i < g_tp_types; i++) {
        *e++ = (uint8_t)(g_tp_type[i].mode | g_tp_type[i].blend << 4), *e++ = g_tp_type[i].n;
        memcpy(e, g_tp_type[i].slot, g_tp_type[i].n), e += g_tp_type[i].n;
    }
    return (uint32_t)(e - buf);
}

// the current map's mesh, the map's own types' sets on it - registered anew when one isn't (a map set up elsewhere
// than where it ends: a load in the editor sets its map up in an object of its own, which is gone after it - the
// player's game, 2026-11-08, read a mesh kept from there; a new map at a freed one's address); NULL: no map
internal void *tp_mesh(void) {
    ee_map *map = game_map();
    void *mesh = map ? map_mesh_at(map) : NULL;
    if (!mesh) return NULL;
    unsigned n;
    uint8_t *sets = mesh_color_sets(mesh, &n);
    for (unsigned i = 0; i < g_tp_types; i++) {
        unsigned ns = 0;
        const uint32_t *s = g_tp_type[i].set < n ? color_set_slots(sets, g_tp_type[i].set, &ns) : NULL;
        int same = s && ns == g_tp_type[i].n;
        for (unsigned k = 0; same && k < ns; k++) same = s[k] == g_tp_type[i].slot[k];
        if (same) continue;
        tp_register(mesh);
        tp_apply(mesh);
        terrain_refresh();
        if (g_log) log_locked("# terrain palette: the map's own types' sets registered again on its mesh\n");
        break;
    }
    return mesh;
}

// after a load from a file: the map's table and the visible ground - its record again when the load set up no map of
// its own (else its setup took it already)
internal void tp_load(const uint8_t *p, uint32_t len) {
    if (!g_tp_on || !game_map()) return;
    if (!g_tp_setup_in_load) tp_parse(p, len);
    g_tp_setup_in_load = 0;
    void *mesh = tp_mesh();
    tp_apply(mesh);
    terrain_refresh();
    tp_list_sync();
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

// slots given back (the map's own no more): the corners on them to the nearest colour (RGB) of a slot that stays
// in a set - a game's or one of the map's own
internal void tp_release(void *mesh, const uint8_t *slots, unsigned n) {
    ee_map *map = game_map();
    uint8_t gone[TP_SLOTS] = { 0 }, to[TP_SLOTS];
    for (unsigned i = 0; i < n; i++) gone[slots[i]] = 1;
    for (unsigned s = 0; s < TP_SLOTS; s++) {
        to[s] = (uint8_t)s;
        if (!gone[s]) continue;
        float best = 1e30f, *c = mesh_color(mesh, s);
        for (unsigned t = 0; t < TP_SLOTS; t++) {
            if (gone[t] || (g_tp_free[t] && !g_tp_own[t])) continue;
            float *d = mesh_color(mesh, t), e = 0;
            for (int k = 0; k < 3; k++) e += (c[k] - d[k]) * (c[k] - d[k]);
            if (e < best) best = e, to[s] = (uint8_t)t;
        }
    }
    for (unsigned y = 0, size = map ? map_get_size(map) : 0; map && y <= size; y++) {
        const ee_corner_row *row = map_corner_row(map, y);
        for (unsigned i = 0, len = corner_row_get_len(row); i < len; i++) {
            ee_corner *c = corner_row_at(row, i);
            corner_set_color(c, to[corner_get_color(c)]);
        }
    }
    for (unsigned i = 0; i < n; i++) g_tp_own[slots[i]] = 0;
}

// A RAMP's n colours from a to b, the ends a and b themselves; the middle blended in sRGB (the 0..255 values straight:
// the game's own ramps are, db\dbterraintype.dat's sets in textures\ColorPalette.tga - 21 of its 34 distinct sets fit
// it best, the clean ones within ~3 of 255, OKLab nearly as close, linear light worst in every one), in linear light
// (the middle brighter: a dark-to-light ramp's steps crowd at the light end) or in OKLab (Bjorn Ottosson's, 2020: even
// steps of lightness, a ramp between two hues without sRGB's duller middle)
internal float tp_to_linear(float c) { c /= 255.0f; return c <= 0.04045f ? c / 12.92f : powf((c + 0.055f) / 1.055f, 2.4f); }
internal uint8_t tp_to_srgb(float c) {
    c = c < 0 ? 0 : c > 1 ? 1 : c;
    return (uint8_t)(255.0f * (c <= 0.0031308f ? c * 12.92f : 1.055f * powf(c, 1 / 2.4f) - 0.055f) + 0.5f);
}
internal void tp_oklab(const uint8_t rgb[3], float lab[3]) {
    float r = tp_to_linear(rgb[0]), g = tp_to_linear(rgb[1]), b = tp_to_linear(rgb[2]);
    float l = cbrtf(0.4122214708f * r + 0.5363325363f * g + 0.0514459929f * b);
    float m = cbrtf(0.2119034982f * r + 0.6806995451f * g + 0.1073969566f * b);
    float s = cbrtf(0.0883024619f * r + 0.2817188376f * g + 0.6299787005f * b);
    lab[0] = 0.2104542553f * l + 0.7936177850f * m - 0.0040720468f * s;
    lab[1] = 1.9779984951f * l - 2.4285922050f * m + 0.4505937099f * s;
    lab[2] = 0.0259040371f * l + 0.7827717662f * m - 0.8086757660f * s;
}
internal void tp_from_oklab(const float lab[3], uint8_t rgb[3]) {
    float l = lab[0] + 0.3963377774f * lab[1] + 0.2158037573f * lab[2];
    float m = lab[0] - 0.1055613458f * lab[1] - 0.0638541728f * lab[2];
    float s = lab[0] - 0.0894841775f * lab[1] - 1.2914855480f * lab[2];
    l = l * l * l, m = m * m * m, s = s * s * s;
    rgb[0] = tp_to_srgb(4.0767416621f * l - 3.3077115913f * m + 0.2309699292f * s);
    rgb[1] = tp_to_srgb(-1.2684380046f * l + 2.6097574011f * m - 0.3413193965f * s);
    rgb[2] = tp_to_srgb(-0.0041960863f * l - 0.7034186147f * m + 1.7076147010f * s);
}
internal void tp_ramp(const uint8_t a[3], const uint8_t b[3], unsigned n, int blend, uint8_t (*out)[3]) {
    uint8_t ea[3], eb[3];   // a copy: out may hold the ends
    memcpy(ea, a, 3), memcpy(eb, b, 3);
    float la[3], lb[3];
    if (blend == TP_OKLAB) tp_oklab(ea, la), tp_oklab(eb, lb);
    for (unsigned k = 0; k < n; k++) {
        float f = n > 1 ? (float)k / (float)(n - 1) : 0;
        if (k == 0 || k + 1 == n) memcpy(out[k], k ? eb : ea, 3);
        else if (blend == TP_LINEAR)
            for (int j = 0; j < 3; j++) out[k][j] = tp_to_srgb(tp_to_linear(ea[j]) + (tp_to_linear(eb[j]) - tp_to_linear(ea[j])) * f);
        else if (blend == TP_OKLAB) {
            float lab[3] = { la[0] + (lb[0] - la[0]) * f, la[1] + (lb[1] - la[1]) * f, la[2] + (lb[2] - la[2]) * f };
            tp_from_oklab(lab, out[k]);
        } else
            for (int j = 0; j < 3; j++) out[k][j] = (uint8_t)((ea[j] * (int)(n - 1 - k) + eb[j] * (int)k + (int)(n - 1) / 2) / (int)(n - 1));
    }
}

// colour type i of the map's own (g_tp_types: a new one) in mode with n slots coloured rgb - flat: rgb[0] (one slot),
// a ramp rgb[0] to rgb[1] in blend (tp_ramp), each slot: n colours; 0: not that many slots free (or no map, no room)
internal int tp_type_put(unsigned i, int mode, int blend, unsigned n, const uint8_t (*rgb)[3]) {
    if (mode == TP_FLAT) n = 1;
    void *mesh = tp_mesh();
    if (!g_tp_on || !mesh || mode < TP_FLAT || mode > TP_EACH || blend < TP_SRGB || blend > TP_OKLAB || n < 1 + (mode == TP_RAMP) || n > TP_TYPE_SLOTS ||
        i > g_tp_types || i >= TP_TYPES)
        return 0;
    struct tp_type t = i < g_tp_types ? g_tp_type[i] : (struct tp_type){ 0 };
    unsigned had = t.n, avail = 0;
    for (unsigned s = 0; s < TP_SLOTS; s++) avail += g_tp_free[s] && !g_tp_own[s];
    if (n > had && n - had > avail) return 0;
    if (n < had) tp_release(mesh, t.slot + n, had - n);
    for (unsigned s = 0; s < TP_SLOTS && t.n < n; s++)
        if (g_tp_free[s] && !g_tp_own[s]) t.slot[t.n++] = (uint8_t)s, g_tp_own[s] = 1;
    t.n = (uint8_t)n, t.mode = (uint8_t)mode, t.blend = (uint8_t)(mode == TP_RAMP ? blend : TP_SRGB);
    uint8_t ramp[TP_TYPE_SLOTS][3];
    if (mode == TP_RAMP) tp_ramp(rgb[0], rgb[1], n, blend, ramp);
    for (unsigned k = 0; k < n; k++) memcpy(g_tp_rgb[t.slot[k]], mode == TP_EACH ? rgb[k] : mode == TP_FLAT ? rgb[0] : ramp[k], 3);
    if (n != had) {   // a set of these slots (the old one stays in the mesh, painted with no more)
        uint32_t s[TP_TYPE_SLOTS];
        for (unsigned k = 0; k < n; k++) s[k] = t.slot[k];
        t.set = mesh_register_color_set(mesh, s, n);
    }
    g_tp_type[i] = t;
    if (i == g_tp_types) g_tp_types++;
    tp_apply(mesh);
    terrain_refresh();
    tp_list_sync();
    return 1;
}

// colour type i of the map's own taken out: its corners to the nearest colours left, its slots free again
internal int tp_type_delete(unsigned i) {
    void *mesh = tp_mesh();
    if (i >= g_tp_types || !mesh) return 0;
    tp_release(mesh, g_tp_type[i].slot, g_tp_type[i].n);
    memmove(g_tp_type + i, g_tp_type + i + 1, (g_tp_types - i - 1) * sizeof *g_tp_type);
    g_tp_types--;
    tp_apply(mesh);
    terrain_refresh();
    tp_list_sync();
    return 1;
}

internal int terrain_dialog_install(void);   // terrain_dialog.c
internal void terrain_palette_install(void) {
    g_tp_on = g_set.terrain_palette;
    int n = 0;
    if (g_tp_on)
        n = patch_mesh_color_table(hook_tp_color_table, &g_tp_color_table) + patch_map_paint_colors(hook_tp_paint_colors) +
            patch_se_terrain_init(hook_tp_panel_init) + patch_se_terrain_delete(hook_tp_panel_delete) + terrain_dialog_install() +
            terrain_cliffs_install();
    if (g_log) fprintf(g_log, "# editor: terrain_palette=%d%s\n", g_tp_on, !g_tp_on ? "" : n == 6 + TC_HOOKS ? " (hooked)" : " (FAILED)");
}

// pipe: palette type add|set <i> flat <r g b> | ramp <n> [srgb|linear|oklab] <r g b> <r g b> | each <n> <r g b> x n - a
// colour type of the map's own made (set: type i changed; a ramp blended in sRGB unless named - tp_ramp), its colours
// in the reply; palette type delete <i>; palette type select <i> (the editor's colour list's
// item, as a click on it; i the count of types: the "new" item - the dialog opens, the reply comes when it is closed);
// palette type edit <i> (the dialog for type i, as a double click on its item opens it)
internal void td_open(int i, int sel);   // terrain_dialog.c
internal void tp_cmd_type(char *out, const char *args) {
    char op[16] = "", mode[16] = "";
    int i = -1, at = 0;
    if (sscanf(args, "%15s%n", op, &at) < 1) op[0] = 0;
    args += at;
    if (strcmp(op, "add") && sscanf(args, "%d%n", &i, &at) == 1) args += at;
    if (!strcmp(op, "delete")) { REPLY(tp_type_delete((unsigned)i) ? "ok: type %d deleted\n" : "error: no type %d\n", i); return; }
    if (!strcmp(op, "edit")) {
        if (!g_tp_panel || i < 0 || (unsigned)i >= g_tp_types) { REPLY("error: no editor or no type %d\n", i); return; }
        td_open(i, (int)(g_tp_list_base + (unsigned)i));
        REPLY("ok: the dialog closed, %u types\n", g_tp_types);
        return;
    }
    if (!strcmp(op, "select")) {
        ee_se_color_list *list = g_tp_panel ? se_terrain_get_color_list(g_tp_panel) : NULL;
        if (!list || i < 0 || (unsigned)i > g_tp_types) { REPLY("error: no editor or no type %d\n", i); return; }
        se_color_list_select(list, g_tp_list_base + (unsigned)i, 1, 1);
        REPLY("ok: item %u of %u selected\n", g_tp_list_base + (unsigned)i, se_color_list_count(list));
        return;
    }
    if (strcmp(op, "add") && strcmp(op, "set")) { REPLY("error: usage: palette type add|set <i> <flat|ramp|each> ... | delete <i> | select <i>\n"); return; }
    if (op[0] == 'a') i = (int)g_tp_types;
    int m = -1, n = 1;
    if (sscanf(args, "%15s%n", mode, &at) == 1) args += at;
    m = !strcmp(mode, "flat") ? TP_FLAT : !strcmp(mode, "ramp") ? TP_RAMP : !strcmp(mode, "each") ? TP_EACH : -1;
    if (m != TP_FLAT && sscanf(args, "%d%n", &n, &at) == 1) args += at;
    int blend = -1;
    char word[16];
    if (m == TP_RAMP && sscanf(args, "%15s%n", word, &at) == 1 && (blend = !strcmp(word, "srgb") ? TP_SRGB :
        !strcmp(word, "linear") ? TP_LINEAR : !strcmp(word, "oklab") ? TP_OKLAB : -1) >= 0) args += at;
    if (blend < 0) blend = TP_SRGB;   // no word: the colours follow
    uint8_t rgb[TP_TYPE_SLOTS][3];
    int want = m == TP_FLAT ? 1 : m == TP_RAMP ? 2 : n, got = 0;
    for (; got < want && got < TP_TYPE_SLOTS; got++) {
        int r, g, b;
        if (sscanf(args, "%d %d %d%n", &r, &g, &b, &at) != 3) break;
        args += at;
        rgb[got][0] = (uint8_t)r, rgb[got][1] = (uint8_t)g, rgb[got][2] = (uint8_t)b;
    }
    if (m < 0 || i < 0 || n < 1 || got < want) { REPLY("error: usage: palette type add|set <i> flat <r g b> | ramp <n> [srgb|linear|oklab] <r g b> <r g b> | each <n> <r g b> x n\n"); return; }
    if (!tp_type_put((unsigned)i, m, blend, (unsigned)n, (const uint8_t (*)[3])rgb)) { REPLY("error: type %d not made (slots free too few, or no map)\n", i); return; }
    REPLY("ok: type %d, %u slots:", i, g_tp_type[i].n);
    for (unsigned k = 0; k < g_tp_type[i].n; k++) REPLY(" %u", g_tp_type[i].slot[k]);
    REPLY(", set %u, colours", g_tp_type[i].set);
    for (unsigned k = 0; k < g_tp_type[i].n; k++) {
        const uint8_t *c = g_tp_rgb[g_tp_type[i].slot[k]];
        REPLY(" %02x%02x%02x", c[0], c[1], c[2]);
    }
    REPLY("\n");
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
    if (!strcmp(mode, "type")) { tp_cmd_type(out, args + strspn(args, " ") + 4); return; }
    if (!strcmp(mode, "ground")) { tc_cmd_ground(out, args + strspn(args, " ") + 6); return; }
    if (mode[0]) { REPLY("error: usage: palette [get <slot> | set <slot> <r> <g> <b> | clear <slot> | remap | type ...]\n"); return; }
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
    if (g_tp_types) REPLY("; types:");
    for (unsigned i = 0; i < g_tp_types; i++)
        REPLY(" %u=%s%s/%u(set %u)", i, (const char *[]){ "flat", "ramp", "each" }[g_tp_type[i].mode],
              g_tp_type[i].mode == TP_RAMP ? (const char *[]){ "", " linear", " oklab" }[g_tp_type[i].blend] : "", g_tp_type[i].n, g_tp_type[i].set);
    if (g_tp_panel) REPLY("; editor list %u + %u, item %d selected", g_tp_list_base, se_color_list_count(se_terrain_get_color_list(g_tp_panel)) - g_tp_list_base,
                          se_color_list_get_selected(se_terrain_get_color_list(g_tp_panel)));
    if (g_tp_paints[0] + g_tp_paints[1] + g_tp_paints[2])
        REPLY("; paints with a game's type %u, the map's own %u, none %u (the last's type %d)", g_tp_paints[0], g_tp_paints[1], g_tp_paints[2], g_tp_paint_last);
    REPLY("\n");
}
