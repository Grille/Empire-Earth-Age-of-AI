// CLIFFS AND THE WATER'S EDGE IN THE EDITOR (with `terrain_palette`, [editor]). A cliff is ground like any other: a
// tile's texture and its corners' colour slots; only its kind (9, what the units' terrain tables keep them off) says
// cliff. The game's cliff pass (map_cliff_pass) makes it from the heights: every steep tile kind 9, its block the map's
// cliff textures, its corners the colours of the first of them. It runs after the terrain paint and at every load -
// and a load first takes the cliff textures off the tiles (map_set_texture_load), for the pass to lay them anew - so
// nothing painted on a cliff stayed. Here the pass keeps the kinds and leaves the ground alone:
//   a terrain paint (TC_KEEP): what was painted stays, on a steep tile too, and that tile keeps its kind 9;
//   a load (TC_LOAD): a tile with a texture that isn't a cliff's where the pass would lay one was painted so - every
//   other steep tile's block was given cliff textures when it was saved - and keeps it; the colours are the file's;
//   an elevation paint, with the panel's "Cliff color" ticked as it is by default, as the game does: cliffs laid and
//   taken off. Unticked, TC_KEEP, and a tile no longer steep keeps its texture and colours (its kind is ground's).
// Everything else (a new map, a random map, a change of the map's cliff type) is the game's.
// The cliff textures are in the terrain panel's list of textures, where the game has them out (their records' in_editor;
// the map panel's list of default grounds stays without them). A cliff's texture painted on ground that isn't steep is
// only a texture: its tile can be walked on, as after a load (tc_flat) - what makes a cliff is its height. Three of them
// have the wrong name in the data (Brick, Pavement twice): one has its own string in the language file, the rock
// cliffs none (TC_NAME's own).
// The panel's other box, "Water color": unticked, an elevation paint leaves a corner's colour alone when it goes below
// 0 (SetWaterColorSet) or comes back (SetColorSet from its tile's texture's colour type).
// The two boxes are the game's own check boxes, read when a paint runs; what they were left at holds until the game
// ends. A map that never had a cliff painted on loads as it always did: same textures, colours and kinds.
#define TC_CLIFF_ID 0x3FA       // the boxes' control ids: above the editor's own
#define TC_WATER_ID 0x3FB
#define TC_WIDTH 140.0f
#define TC_HEIGHT 20.0f
#define TC_GAP 0.0f
#define TC_NAME 0x7F00          // name_text of a rock cliff: strings of the mod's own, far above the language file's
enum { TC_LAY, TC_KEEP, TC_LOAD };
internal ee_se_elevation *g_tc_panel;       // the editor's elevation panel, while it is
internal ee_ui_control *g_tc_cliff_box, *g_tc_water_box;
internal int g_tc_cliff = 1, g_tc_water = 1;   // the boxes as they were last read
internal int g_tc_mode;                     // the cliff pass running now: how
internal int g_tc_mute;                     // the mesh's corner colours are left alone for now
internal mesh_set_color_set_fn g_tc_set_color;
internal mesh_set_water_color_set_fn g_tc_set_water;
internal resource_string_fn g_tc_string;
internal unsigned g_tc_kept_load, g_tc_kept_paint, g_tc_filled, g_tc_flat;

// the terrain panel's list being filled: the cliffs are in it, under the right names
internal const struct { const char *name; int text; } k_tc_names[] = {
    { "Cliffwinter /", 22019 }, { "CliffRock \\", TC_NAME }, { "CliffRock /", TC_NAME + 1 },
};
internal const char *const k_tc_own[] = { "cliff_rock_1", "cliff_rock_2" };   // the words (language.eeai)
internal int __thiscall hook_tc_string(void *res, int id, void *s) {
    if (id >= TC_NAME && id < TC_NAME + (int)(sizeof k_tc_own / sizeof *k_tc_own)) {
        __attribute__((aligned(8))) uint8_t w[32];
        wstring_init_text(w, word(k_tc_own[id - TC_NAME]));
        wstring_copy(s, w);
        wstring_free(w);
        return 0;
    }
    return g_tc_string(res, id, s);
}
internal void tc_list_cliffs(int on) {
    for (uint32_t i = 0; i < 256; i++) {
        ee_terrain *t = terrain_at(i);
        if (!t || !terrain_is_cliff(t)) continue;
        terrain_set_in_editor(t, (uint8_t)on);
        for (unsigned k = 0; on && k < sizeof k_tc_names / sizeof *k_tc_names; k++)
            if (!strcmp((const char *)terrain_get_name(t), k_tc_names[k].name)) terrain_set_name_text(t, k_tc_names[k].text);
    }
}

internal void __thiscall hook_tc_set_color(void *mesh, unsigned long x, unsigned long y, unsigned long set) {
    if (!g_tc_mute) g_tc_set_color(mesh, x, y, set);
}
internal void __thiscall hook_tc_set_water(void *mesh, unsigned long x, unsigned long y) {
    if (!g_tc_mute) g_tc_set_water(mesh, x, y);
}

// the boxes' states, kept for when they are gone
internal void tc_read_boxes(void) {
    if (g_tc_cliff_box) g_tc_cliff = ui_check_box_get_checked_state((ui_check_box *)g_tc_cliff_box) != 0;
    if (g_tc_water_box) g_tc_water = ui_check_box_get_checked_state((ui_check_box *)g_tc_water_box) != 0;
}

// the tile, NULL off the map
internal ee_tile *tc_tile(ee_map *map, int x, int y) {
    int size = (int)map_get_size(map);
    return x < 0 || y < 0 || x >= size || y >= size ? NULL : map_get_tiles(map)[y * size + x];
}
// its texture is a cliff's
internal int tc_texture_is_cliff(ee_map *map, int x, int y) {
    const ee_terrain *t = terrain_at((uint32_t)map_tile_texture(map, x, y));
    return t && terrain_is_cliff(t);
}
// the pass's test
internal int tc_steep(ee_map *map, int x, int y) {
    float lo = INFINITY, hi = -INFINITY, step = map_get_height_step(map);
    for (int k = 0; k < 4; k++) {
        float h = ground_corner(map, x + (k & 1), y + (k >> 1));
        if (isnan(h)) return 0;
        if (h < lo) lo = h;
        if (h > hi) hi = h;
    }
    return hi >= step && hi - lo >= step * 1.5f - 0.0001f;
}

// the pass's last step counts the block's tiles with a cliff's texture: for a tile whose texture was painted over and
// that is steep still the same - no tile of a map without any such one changes
internal void tc_fill(ee_map *map, int x_lo, int x_hi, int y_lo, int y_hi) {
    int size = (int)map_get_size(map);
    if (x_lo < 0) x_lo = 0;
    if (y_lo < 0) y_lo = 0;
    if (x_hi > size) x_hi = size;
    if (y_hi > size) y_hi = size;
    for (int y = y_lo; y < y_hi; y++)
        for (int x = x_lo; x < x_hi; x++) {
            ee_tile *tile = tc_tile(map, x, y);
            if (!tile || tile_is_cliff(tile)) continue;
            int n = 0, painted = 0;
            for (int k = 0; k < 4; k++) {
                int bx = x - (k & 1), by = y - (k >> 1);
                const ee_tile *t = tc_tile(map, bx, by);
                if (!t || tc_texture_is_cliff(map, bx, by)) n++;
                else if (tile_is_cliff(t) && tc_steep(map, bx, by)) { n++; painted = 1; }
            }
            if (n > 2 && painted) {
                int pos[2] = { x, y };
                tile_make_cliff(tile, pos, map);
                g_tc_filled++;
            }
        }
}

// a cliff's texture on tiles with nothing steep beside them (the block the pass colours is 2 x 2 round a steep tile):
// the kind the texture and the pass's gap rule gave them made walkable again, the map's default ground's
internal void tc_flat(ee_map *map, int x_lo, int x_hi, int y_lo, int y_hi) {
    const ee_terrain *ground = terrain_at((uint32_t)map_get_default_terrain(map));
    int size = (int)map_get_size(map);
    if (!ground) return;
    if (x_lo < 0) x_lo = 0;
    if (y_lo < 0) y_lo = 0;
    if (x_hi > size) x_hi = size;
    if (y_hi > size) y_hi = size;
    for (int y = y_lo; y < y_hi; y++)
        for (int x = x_lo; x < x_hi; x++) {
            ee_tile *tile = tc_tile(map, x, y);
            if (!tile || !tile_is_cliff(tile) || !tc_texture_is_cliff(map, x, y)) continue;
            int by = 0;
            for (int dy = -1; dy <= 1 && !by; dy++)
                for (int dx = -1; dx <= 1 && !by; dx++) by = tc_steep(map, x + dx, y + dy);
            if (by) continue;
            int pos[2] = { x, y };
            tile_change_kind(tile, terrain_get_kind(ground), pos, map);
            g_tc_flat++;
        }
}

internal void tc_pass(ee_map *map, int mode, uint32_t x_lo, uint32_t x_hi, int32_t y_lo, int32_t y_hi) {
    g_tc_mode = mode;
    g_tc_mute = mode != TC_LAY;      // its corners' colours
    map_cliff_pass(map, x_lo, x_hi, y_lo, y_hi);
    g_tc_mute = 0;
    g_tc_mode = TC_LAY;
    tc_fill(map, (int)x_lo, (int)x_hi, y_lo, y_hi);
    if (mode != TC_LAY) tc_flat(map, (int)x_lo, (int)x_hi, y_lo, y_hi);
}
internal void __thiscall hook_tc_pass_load(ee_map *map, uint32_t x_lo, uint32_t x_hi, int32_t y_lo, int32_t y_hi) {
    tc_pass(map, TC_LOAD, x_lo, x_hi, y_lo, y_hi);
    if (g_log) fprintf(g_log, "# editor: load's cliffs: %u textures kept, %u tiles filled\n", g_tc_kept_load, g_tc_filled);
}
internal void __thiscall hook_tc_pass_paint(ee_map *map, uint32_t x_lo, uint32_t x_hi, int32_t y_lo, int32_t y_hi) {
    tc_pass(map, TC_KEEP, x_lo, x_hi, y_lo, y_hi);
}
internal void __thiscall hook_tc_pass_elevation(ee_map *map, uint32_t x_lo, uint32_t x_hi, int32_t y_lo, int32_t y_hi) {
    tc_read_boxes();
    tc_pass(map, g_tc_cliff ? TC_LAY : TC_KEEP, x_lo, x_hi, y_lo, y_hi);
}

// a load's: a tile with a cliff's texture keeps it (the game's: the map's default ground here, for the pass to lay it
// anew - and one on no steep tile, left so by the box, would be lost). Its kind the caller gives it: ground's
internal void __thiscall hook_tc_load_texture(ee_map *map, int32_t x, int32_t y, int32_t set, int8_t release) {
    if (tc_texture_is_cliff(map, x, y)) set = map_tile_texture(map, x, y);
    map_set_texture(map, x, y, set, release);
}
// the pass's
internal void __thiscall hook_tc_cliff_texture(ee_map *map, int32_t x, int32_t y, int32_t set, int8_t release) {
    if (g_tc_mode == TC_KEEP) { g_tc_kept_paint++; return; }
    if (g_tc_mode == TC_LOAD && !tc_texture_is_cliff(map, x, y)) { g_tc_kept_load++; return; }
    map_set_texture(map, x, y, set, release);
}
// the elevation paint's taking a cliff off a tile no longer steep: its kind the caller still gives it
internal void __thiscall hook_tc_remove(ee_map *map, uint32_t x, uint32_t y, uint32_t texture) {
    if (g_tc_cliff) map_reset_tile(map, x, y, texture);
}
internal void __thiscall hook_tc_elevation(ee_map *map, int32_t *x_lo, int32_t *x_hi, int32_t *y_hi, int32_t *y_lo, float p5, float p6, float p7, float p8, int8_t p9) {
    tc_read_boxes();
    g_tc_mute = !g_tc_water;
    map_elevation_change(map, x_lo, x_hi, y_hi, y_lo, p5, p6, p7, p8, p9);
    g_tc_mute = 0;
}

// one box of the panel, under what stands at top / left
internal ee_ui_control *tc_box(unsigned id, const char *text, float left, float top, int checked, ee_ui_form *form, int *index) {
    float rect[4] = { top + TC_HEIGHT, left, left + TC_WIDTH, top };   // bottom, left, right, top
    __attribute__((aligned(8))) uint8_t w[32];
    wstring_init_text(w, text);
    ee_ui_control *box = check_box_new(id, w, rect, form, index);
    wstring_free(w);
    if (!box) return NULL;
    ui_control_set_bg_opacity(box, 0.4f);   // as the panel's own controls
    ui_check_box_label_yellow((ui_check_box *)box);
    if (checked) ui_check_box_check((ui_check_box *)box, 1);
    editor_elevation_add_control(form, box);
    return box;
}

internal void *__thiscall hook_tc_panel_init(ee_se_elevation *p, uint32_t a, uint32_t b, int *index, ee_ui_form *form, void *font) {
    void *r = se_elevation_init(p, a, b, index, form, font);
    const ee_ui_control *last = (const ee_ui_control *)se_elevation_get_smooth(p);
    if (!last) return r;
    float left = ui_control_get_left(last), top = ui_control_get_bottom(last) + TC_GAP;
    g_tc_cliff_box = tc_box(TC_CLIFF_ID, word("cliff_color"), left, top, g_tc_cliff, form, index);
    g_tc_water_box = tc_box(TC_WATER_ID, word("water_color"), left, top + TC_HEIGHT + TC_GAP, g_tc_water, form, index);
    g_tc_panel = p;
    if (g_log) log_locked("# editor: elevation panel's boxes at %.0f,%.0f\n", left, top);
    return r;
}

// the panel deleted (the editor closed): the boxes first, as the units panel's search box
internal void *__thiscall hook_tc_panel_delete(ee_se_elevation *p, uint8_t flags) {
    if (p == g_tc_panel) {
        tc_read_boxes();
        if (g_tc_cliff_box) ((void (__thiscall *)(void *, int))(*(void ***)g_tc_cliff_box)[0])(g_tc_cliff_box, 1);
        if (g_tc_water_box) ((void (__thiscall *)(void *, int))(*(void ***)g_tc_water_box)[0])(g_tc_water_box, 1);
        g_tc_cliff_box = g_tc_water_box = NULL;
        g_tc_panel = NULL;
    }
    return eeuifse_elevation_destroy(p, flags);
}

// pipe: palette ground snap - the map's tiles' kinds and textures and its corners' colours kept; palette ground: what
// has changed since, the steep tiles and the tiles of a cliff's kind and texture now, the corners under the water
#define TC_SNAP (1024 * 1024)
internal struct { int size; uint8_t kind[TC_SNAP], texture[TC_SNAP], color[TC_SNAP]; } g_tc_snap;
internal void tc_cmd_ground(char *out, const char *args) {
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0, snap = strstr(args, "snap") != NULL;
    if (size <= 0 || size >= 1024) { REPLY("error: no map\n"); return; }
    unsigned kinds = 0, textures = 0, colors = 0, steep = 0, cliff_kind = 0, cliff_texture = 0, under = 0, tiles = 0;
    uint32_t sum = 0;
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++) {
            int i = y * size + x;
            const ee_tile *tile = tc_tile(map, x, y);
            const ee_corner *c = map_corner(map, x, y);
            uint8_t k = tile ? (uint8_t)tile_get_kind(tile) : 0xFF, t = (uint8_t)map_tile_texture(map, x, y), s = c ? corner_get_color(c) : 0;
            if (snap) { g_tc_snap.kind[i] = k; g_tc_snap.texture[i] = t; g_tc_snap.color[i] = s; continue; }
            sum = (sum * 31 + (uint32_t)k + (uint32_t)t * 257 + (uint32_t)s * 65537) * 2654435761u;
            tiles += tile != NULL;
            kinds += g_tc_snap.kind[i] != k;
            textures += g_tc_snap.texture[i] != t;
            colors += g_tc_snap.color[i] != s;
            if (tile && tc_steep(map, x, y)) steep++;
            if (tile && tile_is_cliff(tile)) cliff_kind++;
            if (tile && tc_texture_is_cliff(map, x, y)) cliff_texture++;
            if (c && corner_get_height(c) < 0.0f) under++;
        }
    if (snap) { g_tc_snap.size = size; REPLY("ok: ground kept\n"); return; }
    tc_read_boxes();
    REPLY("ground: boxes cliff %d water %d; size %d tiles %u changed kinds %u textures %u colours %u; steep %u kind_cliff %u texture_cliff %u; corners under water %u; kept at load %u, by paints %u, filled %u, flat %u; sum %08x\n",
          g_tc_cliff, g_tc_water, size, tiles, kinds, textures, colors, steep, cliff_kind, cliff_texture, under, g_tc_kept_load, g_tc_kept_paint, g_tc_filled, g_tc_flat, sum);
}

// how many of its hooks went in, of TC_HOOKS
#define TC_HOOKS 15
internal int terrain_cliffs_install(void) {
    return patch_resource_string(hook_tc_string, &g_tc_string) + patch_mesh_set_color_set(hook_tc_set_color, &g_tc_set_color) + patch_mesh_set_water_color_set(hook_tc_set_water, &g_tc_set_water) +
           patch_map_cliff_pass_load(hook_tc_pass_load) + patch_map_cliff_pass_paint(hook_tc_pass_paint) +
           patch_map_cliff_pass_elevation(hook_tc_pass_elevation) + patch_map_set_texture_load(hook_tc_load_texture, NULL) +
           patch_map_set_texture_cliff(hook_tc_cliff_texture) + patch_map_cliff_remove(hook_tc_remove) +
           patch_map_elevation_change(hook_tc_elevation) + patch_se_elevation_init(hook_tc_panel_init) + patch_se_elevation_delete(hook_tc_panel_delete);
}
