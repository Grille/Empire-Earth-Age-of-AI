// THE TERRAIN PAINTER'S LIMITS (with `terrain_palette`, [editor]): fields on the editor's terrain panel that say which
// ground a stroke takes. Heights and slopes are the editor's height numbers (a height over half a height step, 0.204
// tiles), a slope the highest corner of a tile less its lowest - the game's cliffs are 3 and more. A field left empty is
// no limit. A tile's texture goes by the tile (its height: the middle of its corners), a corner's colour by the corner
// (its own height, the steepest of the four tiles round it, a cliff if one of them is), and the objects of a brush like
// a forest by the tile they would stand on.
// "Water at": the game's terrain paint leaves the colours of corners at -3 and lower alone. That number is the field's,
// -3 as it shows at the start; set to another, corners at it and lower are given the water's colours by a stroke.
// "Cliffs": anything, no cliffs, only cliffs. A cliff is a tile of a cliff's kind, of a cliff's texture, or steep.
// Everything at its start (no limits, -3, anything): the game's own paint, nothing of this runs.
#define TF_ID 0x400             // the controls' ids: above the editor's own and the elevation panel's boxes
#define TF_LABEL 72.0f
#define TF_EDIT 44.0f
#define TF_LINE 26.0f
#define TF_HEIGHT 18.0f
#define TF_COLUMN 12.0f         // between the columns, and right of the picture of the texture
#define TF_COMBO 120.0f
#define TF_WATER_GAME -3.0f
#define TF_EPS 0.001f
enum { TF_HEIGHT_MIN, TF_HEIGHT_MAX, TF_SLOPE_MIN, TF_SLOPE_MAX, TF_WATER, TF_FIELDS };
enum { TF_ANYTHING, TF_NO_CLIFFS, TF_ONLY_CLIFFS, TF_CLIFF_MODES };
internal const char *const k_tf_label[TF_FIELDS] = { "Min height", "Max height", "Min slope", "Max slope", "Water at" };
internal const char *const k_tf_cliffs[TF_CLIFF_MODES] = { "Anything", "No cliffs", "Only cliffs" };
internal ee_se_terrain *g_tf_panel;                 // the editor's terrain panel, while it is
internal ee_ui_control *g_tf_label[TF_FIELDS + 1], *g_tf_box[TF_FIELDS], *g_tf_combo;
internal char g_tf_text[TF_FIELDS][16] = { "", "", "", "", "-3" };   // the fields as they were last read
internal int g_tf_cliffs = TF_ANYTHING;             // the list as it was last read
internal float g_tf_height[2], g_tf_slope[2], g_tf_water;
internal unsigned g_tf_strokes, g_tf_tiles[2], g_tf_corners[3];   // limited strokes: tiles left, painted; corners left, coloured, watered
internal unsigned g_tf_objects[2];                                  // the objects of limited strokes: left, let through

internal float tf_number(const char *t, float none) {
    char *end;
    double v = strtod(t, &end);
    return end == t ? none : (float)v;
}
// the controls read, what they say worked out; 1: a stroke has limits
internal int tf_read(void) {
    for (int i = 0; i < TF_FIELDS; i++)
        if (g_tf_box[i]) {
            const char *t = edit_text(g_tf_box[i]);
            lstrcpynA(g_tf_text[i], t ? t : "", sizeof g_tf_text[i]);
        }
    if (g_tf_combo) {
        uint32_t line = ui_combo_box_get_current_selection((ui_combo_box *)g_tf_combo);
        if (line < TF_CLIFF_MODES) g_tf_cliffs = (int)line;
    }
    g_tf_height[0] = tf_number(g_tf_text[TF_HEIGHT_MIN], -INFINITY);
    g_tf_height[1] = tf_number(g_tf_text[TF_HEIGHT_MAX], INFINITY);
    g_tf_slope[0] = tf_number(g_tf_text[TF_SLOPE_MIN], -INFINITY);
    g_tf_slope[1] = tf_number(g_tf_text[TF_SLOPE_MAX], INFINITY);
    g_tf_water = tf_number(g_tf_text[TF_WATER], TF_WATER_GAME);
    return isfinite(g_tf_height[0]) || isfinite(g_tf_height[1]) || isfinite(g_tf_slope[0]) || isfinite(g_tf_slope[1]) ||
           g_tf_water != TF_WATER_GAME || g_tf_cliffs != TF_ANYTHING;
}

internal int tf_in(float v, const float range[2]) { return v >= range[0] - TF_EPS && v <= range[1] + TF_EPS; }
// a cliff to the list, whatever made it one
internal int tf_is_cliff(ee_map *map, int x, int y) {
    const ee_tile *tile = tc_tile(map, x, y);
    return tile && (tile_is_cliff(tile) || tc_texture_is_cliff(map, x, y) || tc_steep(map, x, y));
}
internal int tf_cliffs_ok(int cliff) { return g_tf_cliffs == TF_ANYTHING || cliff == (g_tf_cliffs == TF_ONLY_CLIFFS); }
// a tile's middle and slope in the editor's height numbers; 0: a corner off the map
internal int tf_measure(ee_map *map, int x, int y, float *mid, float *slope) {
    float unit = map_get_height_step(map) * 0.5f, lo = INFINITY, hi = -INFINITY, sum = 0.0f;
    for (int k = 0; k < 4; k++) {
        float h = ground_corner(map, x + (k & 1), y + (k >> 1));
        if (isnan(h)) return 0;
        sum += h;
        if (h < lo) lo = h;
        if (h > hi) hi = h;
    }
    *mid = sum / 4.0f / unit;
    *slope = (hi - lo) / unit;
    return 1;
}
internal int tf_tile_ok(ee_map *map, int x, int y) {
    float mid, slope;
    return tf_measure(map, x, y, &mid, &slope) && tf_in(mid, g_tf_height) && tf_in(slope, g_tf_slope) && tf_cliffs_ok(tf_is_cliff(map, x, y));
}
internal int tf_corner_ok(ee_map *map, int x, int y) {
    float steepest = 0.0f, h = ground_corner(map, x, y) / (map_get_height_step(map) * 0.5f);
    int cliff = 0;
    if (isnan(h) || !tf_in(h, g_tf_height)) return 0;
    for (int k = 0; k < 4; k++) {
        float mid, slope;
        int tx = x - (k & 1), ty = y - (k >> 1);
        if (tf_measure(map, tx, ty, &mid, &slope) && slope > steepest) steepest = slope;
        cliff |= tf_is_cliff(map, tx, ty);
    }
    return tf_in(steepest, g_tf_slope) && tf_cliffs_ok(cliff);
}

// the set of a colour type, -1: none (the list's "new" item, a type gone meanwhile)
internal int tf_color_set(int type) {
    if (type >= TP_ID) return (unsigned)(type - TP_ID) < g_tp_types ? (int)g_tp_type[type - TP_ID].set : -1;
    const ee_color_type *t = color_type((uint32_t)type);
    return t ? (int)color_type_get_set(t) : -1;
}

// a stroke's rectangle. As the game's a tile (x, y) and the corner (x + 1, y + 1) go together. The colours first: the
// tests of the corners read the kinds and textures of tiles the stroke is about to change
internal void __thiscall hook_tf_paint(ee_map *map, int x_lo, int x_hi, int y_hi, int y_lo, int terrain, int color) {
    if (!tf_read()) { map_paint_terrain(map, x_lo, x_hi, y_hi, y_lo, terrain, color); return; }
    g_tf_strokes++;
    void *mesh = tp_mesh();
    int set = tf_color_set(color);
    float unit = map_get_height_step(map) * 0.5f;
    for (int y = y_lo; mesh && set >= 0 && y <= y_hi; y++)
        for (int x = x_lo; x <= x_hi; x++) {
            if (!tc_tile(map, x, y)) continue;
            if (!tf_corner_ok(map, x + 1, y + 1)) { g_tf_corners[0]++; continue; }
            if (ground_corner(map, x + 1, y + 1) / unit >= g_tf_water + 1.0f - TF_EPS) {
                g_tc_set_color(mesh, (unsigned long)x + 1, (unsigned long)y + 1, (unsigned long)set);
                g_tf_corners[1]++;
            } else if (g_tf_water != TF_WATER_GAME) {
                g_tc_set_water(mesh, (unsigned long)x + 1, (unsigned long)y + 1);
                g_tf_corners[2]++;
            } else g_tf_corners[0]++;
        }
    g_tf_mute = 1;
    for (int y = y_lo; y <= y_hi; y++)
        for (int x = x_lo; x <= x_hi; x++) {
            if (!tc_tile(map, x, y)) continue;
            int ok = tf_tile_ok(map, x, y);
            g_tf_tiles[ok]++;
            if (ok) map_paint_terrain(map, x, x, y, y, terrain, color);
        }
    g_tf_mute = 0;
}
// the objects of a stroke, each asked for by its tile: the random choice of the game's loop stays as it was
internal int8_t hook_tf_can_place(void *players, int32_t type, int32_t x, int32_t y, int32_t p5, int32_t p6) {
    ee_map *map = game_map();
    if (tf_read() && map) {
        int ok = tf_tile_ok(map, x, y);
        g_tf_objects[ok]++;
        if (!ok) return 0;
    }
    return ambient_can_place(players, type, x, y, p5, p6);
}

internal ee_ui_control *tf_label(unsigned id, const char *text, float left, float top, ee_ui_form *form, int *index, void *font) {
    float rect[4] = { top + TF_HEIGHT, left, left + TF_LABEL, top };   // bottom, left, right, top
    __attribute__((aligned(8))) uint8_t w[32];
    wstring_init_text(w, text);
    ee_ui_control *label = text_label_new(id, w, font, rect, form, index);
    wstring_free(w);
    if (!label) return NULL;
    ui_control_set_bg_opacity(label, 0.4f);   // as the panel's own controls
    editor_terrain_add_control(form, label);
    return label;
}
internal void tf_set_text(ee_ui_control *box, const char *text) {
    __attribute__((aligned(8))) uint8_t w[32];
    wstring_init_text(w, text);
    edit_box_set_text(box, w);
    wstring_free(w);
}
// a field, a label with its box
internal void tf_field(int i, float left, float top, ee_ui_form *form, int *index, void *font) {
    g_tf_label[i] = tf_label(TF_ID + 2 * (unsigned)i, k_tf_label[i], left, top, form, index, font);
    float rect[4] = { top + TF_HEIGHT, left + TF_LABEL, left + TF_LABEL + TF_EDIT, top };
    g_tf_box[i] = edit_box_new(TF_ID + 2 * (unsigned)i + 1, rect, form, index);
    if (!g_tf_box[i]) return;
    ui_control_set_bg_opacity(g_tf_box[i], 0.4f);
    tf_set_text(g_tf_box[i], g_tf_text[i]);
    editor_terrain_add_control(form, g_tf_box[i]);
}
internal void tf_list(float left, float top, ee_ui_form *form, int *index, void *font) {
    unsigned id = TF_ID + 2 * TF_FIELDS;
    g_tf_label[TF_FIELDS] = tf_label(id, "Cliffs", left, top, form, index, font);
    float rect[4] = { top + TF_HEIGHT, left + TF_LABEL, left + TF_LABEL + TF_COMBO, top };
    g_tf_combo = combo_box_new(id + 1, font, rect, form, index);
    if (!g_tf_combo) return;
    ui_combo_box *box = (ui_combo_box *)g_tf_combo;
    for (int i = 0; i < TF_CLIFF_MODES; i++) {
        __attribute__((aligned(8))) uint8_t w[32];
        wstring_init_text(w, k_tf_cliffs[i]);
        combo_box_append(box, w);
        wstring_free(w);
    }
    combo_box_select(box, (unsigned)g_tf_cliffs);
    ui_control_set_bg_opacity(ui_combo_box_get_text_box(box), 0.4f);
    combo_box_style(box);
    editor_terrain_add_control(form, g_tf_combo);
}
internal void tf_panel_init(ee_se_terrain *p, ee_ui_form *form, int *index, void *font) {
    const ee_ui_control *title = se_terrain_get_color_label(p), *picture = se_terrain_get_preview(p);
    if (!title || !picture) return;
    float left = ui_control_get_right(picture) + TF_COLUMN, top = ui_control_get_top(title);
    float right = left + TF_LABEL + TF_EDIT + TF_COLUMN;
    tf_field(TF_HEIGHT_MIN, left, top, form, index, font);
    tf_field(TF_HEIGHT_MAX, left, top + TF_LINE, form, index, font);
    tf_field(TF_WATER, left, top + 2 * TF_LINE, form, index, font);
    tf_field(TF_SLOPE_MIN, right, top, form, index, font);
    tf_field(TF_SLOPE_MAX, right, top + TF_LINE, form, index, font);
    tf_list(right, top + 2 * TF_LINE, form, index, font);
    g_tf_panel = p;
    if (g_log) log_locked("# editor: terrain panel's limits at %.0f,%.0f\n", left, top);
}
// the panel deleted (the editor closed): its controls first, what they say kept for the next
internal void tf_panel_delete(ee_se_terrain *p) {
    if (p != g_tf_panel) return;
    tf_read();
    for (int i = 0; i < TF_FIELDS + 2; i++) {
        ee_ui_control **c = i <= TF_FIELDS ? &g_tf_label[i] : &g_tf_combo;
        if (*c) ((void (__thiscall *)(void *, int))(*(void ***)*c)[0])(*c, 1);
        *c = NULL;
    }
    for (int i = 0; i < TF_FIELDS; i++) {
        if (g_tf_box[i]) ((void (__thiscall *)(void *, int))(*(void ***)g_tf_box[i])[0])(g_tf_box[i], 1);
        g_tf_box[i] = NULL;
    }
    g_tf_panel = NULL;
}

// pipe: palette limits [min_height max_height min_slope max_slope water anything|no|only] - "_" an empty field; the
// fields set as a player would type them, and what the limited strokes have done
internal void tf_cmd_diff(char *out);
internal void tf_cmd_limits(char *out, const char *args) {
    if (strstr(args, "diff")) { tf_cmd_diff(out); return; }
    char v[TF_FIELDS + 1][16];
    int n = sscanf(args, "%15s %15s %15s %15s %15s %15s", v[0], v[1], v[2], v[3], v[4], v[5]);
    static const int order[TF_FIELDS] = { TF_HEIGHT_MIN, TF_HEIGHT_MAX, TF_SLOPE_MIN, TF_SLOPE_MAX, TF_WATER };
    for (int i = 0; i < n && i < TF_FIELDS; i++) {
        lstrcpynA(g_tf_text[order[i]], strcmp(v[i], "_") ? v[i] : "", sizeof g_tf_text[0]);
        if (g_tf_box[order[i]]) tf_set_text(g_tf_box[order[i]], g_tf_text[order[i]]);
    }
    if (n > TF_FIELDS) {
        g_tf_cliffs = !strcmp(v[TF_FIELDS], "no") ? TF_NO_CLIFFS : !strcmp(v[TF_FIELDS], "only") ? TF_ONLY_CLIFFS : TF_ANYTHING;
        if (g_tf_combo) combo_box_select((ui_combo_box *)g_tf_combo, (unsigned)g_tf_cliffs);
    }
    int limited = tf_read();
    REPLY("limits: height \"%s\" .. \"%s\", slope \"%s\" .. \"%s\", water at \"%s\", %s (limited %d, panel %d); strokes %u, tiles left %u painted %u, corners left %u coloured %u watered %u, objects left %u placed %u\n",
          g_tf_text[TF_HEIGHT_MIN], g_tf_text[TF_HEIGHT_MAX], g_tf_text[TF_SLOPE_MIN], g_tf_text[TF_SLOPE_MAX], g_tf_text[TF_WATER],
          k_tf_cliffs[g_tf_cliffs], limited, g_tf_panel != NULL, g_tf_strokes, g_tf_tiles[0], g_tf_tiles[1], g_tf_corners[0], g_tf_corners[1], g_tf_corners[2], g_tf_objects[0], g_tf_objects[1]);
}

// pipe: palette limits diff - what differs from `palette ground snap`, measured apart from the limits: the tiles with
// another texture and the corners with another colour, the range of their heights and slopes and how many are cliffs
internal void tf_cmd_diff(char *out) {
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0;
    if (size <= 0 || size != g_tc_snap.size) { REPLY("error: no snap of this map\n"); return; }
    float mid_range[2] = { INFINITY, -INFINITY }, slope_range[2] = { INFINITY, -INFINITY }, corner_range[2] = { INFINITY, -INFINITY };
    float unit = map_get_height_step(map) * 0.5f;
    unsigned tiles = 0, cliffs = 0, corners = 0;
    for (int y = 0; y < size; y++)
        for (int x = 0; x < size; x++) {
            int i = y * size + x;
            const ee_corner *c = map_corner(map, x, y);
            float mid, slope;
            if (c && g_tc_snap.color[i] != corner_get_color(c)) {
                float h = corner_get_height(c) / unit;
                corners++;
                if (h < corner_range[0]) corner_range[0] = h;
                if (h > corner_range[1]) corner_range[1] = h;
            }
            if (g_tc_snap.texture[i] == (uint8_t)map_tile_texture(map, x, y) || !tf_measure(map, x, y, &mid, &slope)) continue;
            tiles++;
            cliffs += (unsigned)(tile_is_cliff(tc_tile(map, x, y)) || tc_steep(map, x, y));
            if (mid < mid_range[0]) mid_range[0] = mid;
            if (mid > mid_range[1]) mid_range[1] = mid;
            if (slope < slope_range[0]) slope_range[0] = slope;
            if (slope > slope_range[1]) slope_range[1] = slope;
        }
    REPLY("diff: textures %u (heights %.2f .. %.2f, slopes %.2f .. %.2f, cliffs %u), colours %u (heights %.2f .. %.2f)\n",
          tiles, mid_range[0], mid_range[1], slope_range[0], slope_range[1], cliffs, corners, corner_range[0], corner_range[1]);
}

internal int terrain_filter_install(void) {
    return patch_map_paint_terrain(hook_tf_paint) + patch_ambient_can_place(hook_tf_can_place);
}
