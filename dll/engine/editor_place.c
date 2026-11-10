// THE EDITOR'S "Sanity check" BOX: a check box in the strip under the units panel, ticked as the game is, shown only while
// a unit type is being placed (the strip holds a selected unit's stats otherwise). Unticked,
// a unit can be placed where it would collide with another or on ground that doesn't suit it. What may be placed is the
// ghost's own mark (EEGhostUnit.can_place, drawn green or red), which the preview works out for each ghost at every mouse
// move and EESEAPlaceUnit reads at the click; so the editor's own two calls of the preview (the game's aren't touched) say
// yes afterwards for a ghost whose ground is all on the map. Never for a gate: it is made of the wall under it. A wall's
// line has a preview of its own and keeps its rules.
// Beside it the block's two fields, Rows and Columns: the extra ghosts of the block go into the game's own vector, so they
// are drawn, marked and placed (EESEAPlaceUnit takes one unit of each ghost that can be placed, in one action) as any
// ghost. Only a type the game makes a single ghost of: an airport's runways and a granary's farms have types of their own.
#define EP_ID 0x3F9          // after the search box's (editor_search.c); the fields' the next four
#define EP_WIDTH 140.0f
#define EP_HEIGHT 18.0f
#define EP_GAP 18.0f         // under the list: inside the strip
#define EP_ROW_GAP 3.0f      // the fields' line under the box
#define EP_LABEL_ROWS 40.0f
#define EP_LABEL_COLUMNS 58.0f
#define EP_FIELD_WIDTH 28.0f
#define EP_FIELD_CHARS 2
#define EP_BLOCK_MAX 50      // a side: what a frame's checks and records can take
enum { EP_ROWS_LABEL, EP_ROWS, EP_COLUMNS_LABEL, EP_COLUMNS, EP_FIELDS };
internal ee_se_unit *g_ep_panel;       // the editor's units panel, while it is
internal ee_ui_control *g_ep_box;
internal uint32_t g_ep_shown;          // the box's display mode as made
internal ee_ui_control *g_ep_field[EP_FIELDS];
internal uint32_t g_ep_field_shown[EP_FIELDS];
internal void *g_ep_vt[40];            // the number fields' vtable (+1), its RTTI pointer first
internal long (__thiscall *g_ep_mouse_up)(ee_ui_control *);   // UIEditBox's

internal int ep_on_map(int x, int y) {
    ee_map *map = game_map();
    unsigned n = map ? map_get_size(map) : 0;
    return map && (unsigned)x < n && (unsigned)y < n && map_get_tiles(map)[(unsigned)y * n + (unsigned)x];
}
// the ghost placeable if its ground is on the map; its cells as the preview's are for a place that passes
internal void ep_allow(ee_ghost_unit *g) {
    ee_unit_type *td = ghost_unit_get_type_data(g);
    if (!td || unit_type_get_family(td) == CLS_GATE) return;
    int size = unit_type_get_size(td), x0 = ghost_unit_get_tile_x(g), y0 = ghost_unit_get_tile_y(g);
    if (size < 2) size = 1;
    else x0 -= size / 2, y0 -= size / 2;
    for (int x = x0; x < x0 + size; x++)
        for (int y = y0; y < y0 + size; y++)
            if (!ep_on_map(x, y)) return;
    for (int i = 0; i <= size && i < 12; i++)
        for (int j = 0; j <= size && j < 12; j++) ghost_unit_set_cells(g, i * 12 + j, 1);
    ghost_unit_set_can_place(g, 1);
}
internal ee_ghost_unit *ep_ghost(void *v, uint32_t i) { return *(ee_ghost_unit **)type_stats_game(v, (int)i); }
// a field's number: 1 when it holds none
internal int ep_count(int field) {
    const char *t = g_ep_field[field] ? edit_text(g_ep_field[field]) : NULL;
    int n = 0;
    for (; t && *t >= '0' && *t <= '9'; t++) n = n * 10 + *t - '0';
    return n < 1 ? 1 : n > EP_BLOCK_MAX ? EP_BLOCK_MAX : n;
}
// a plain unit type being placed, its ghosts all of one type (the block's or the game's single one)
internal int ep_block_on(void *v, uint32_t n) {
    if (!n || !g_ep_panel || !se_unit_get_placing(g_ep_panel) || se_unit_get_placing_wall(g_ep_panel) || se_unit_get_placing_gate(g_ep_panel)) return 0;
    ee_unit_type *td = ghost_unit_get_type_data(ep_ghost(v, 0));
    for (uint32_t i = 1; i < n; i++) if (ghost_unit_get_type_data(ep_ghost(v, i)) != td) return 0;
    return 1;
}
// the vector made as long as the block: the extra ghosts are the first one's kind, the choice of another type starts over
internal void ep_block_fit(void *v, uint32_t want) {
    ee_ghost_unit *g = ep_ghost(v, 0);
    while (vector_size(v) > want) ghost_units_drop_last();
    while (vector_size(v) < want && ghost_unit_add(unit_type_get_id(ghost_unit_get_type_data(g)), ghost_unit_get_player(g))) {}
}
// the extra ghosts laid around the first, which the game has put under the mouse, each a footprint from the next: its
// pose (the first's turn) moved by whole tiles, so a type the game doesn't snap to the tiles stays where the mouse has it
internal void ep_block_lay(void *v, int rows, int columns) {
    ee_ghost_unit *g = ep_ghost(v, 0);
    ee_unit_type *td = ghost_unit_get_type_data(g);
    ee_map *map = game_map();
    int size = unit_type_get_size(td), step = size < 2 ? 1 : size;
    uint32_t i = 1, n = vector_size(v);
    for (int r = 0; r < rows; r++)
        for (int c = 0; c < columns; c++) {
            int dx = (c - (columns - 1) / 2) * step, dy = (r - (rows - 1) / 2) * step;
            if (!dx && !dy) continue;
            if (i >= n) return;
            ee_ghost_unit *e = ep_ghost(v, i++);
            float *t = ghost_unit_transform_at(e);
            memcpy(t, ghost_unit_transform_at(g), 16 * sizeof(float));
            t[3] += (float)dx, t[7] += (float)dy;
            int x = ghost_unit_get_tile_x(g) + dx, y = ghost_unit_get_tile_y(g) + dy, on = ep_on_map(x, y);
            if (on) t[11] = fmaxf(mesh_z(map_mesh_at(map), t[3], t[7]), 0.0f);
            ghost_unit_set_tile_x(e, x);
            ghost_unit_set_tile_y(e, y);
            ghost_unit_set_frame(e, ghost_unit_get_frame(g));
            ghost_unit_set_anim(e, ghost_unit_get_anim(g));
            ghost_unit_set_stands_on(e, ghost_unit_get_stands_on(g));
            ghost_unit_check(e);
            if (!on) ghost_unit_set_can_place(e, 0);
        }
}
internal void __cdecl hook_ep_preview(float *loc) {
    void *v = ghost_units();
    ghosts_enter();
    uint32_t n = vector_size(v);
    int block = ep_block_on(v, n);
    // the game's preview sees its one ghost: several of a type it would lay out as a granary's farms (a farm placed alone
    // is such a type), and the block's checks would run twice. Inside the lock: the fill doesn't see the vector short.
    if (block) ghost_units_set_end(1);
    ghost_unit_preview(loc);
    if (block) {
        ghost_units_set_end(n);
        int rows = ep_count(EP_ROWS), columns = ep_count(EP_COLUMNS);
        ep_block_fit(v, (uint32_t)rows * (uint32_t)columns);
        if (ghosts_changed()) ep_block_lay(v, rows, columns);
    }
    if (g_ep_box && !ui_check_box_get_checked_state((ui_check_box *)g_ep_box))
        for (uint32_t i = 0; i < vector_size(v); i++) ep_allow(ep_ghost(v, i));
    ghosts_leave();
}

// a click in a number field selects what it holds: it holds a 1 from the start, and a digit typed after it would make 15 of
// 5. At the button's release: while it is down the mouse's moves still drag a selection of their own
internal long __thiscall hook_ep_mouse_up(ee_ui_control *box) {
    long r = g_ep_mouse_up(box);
    edit_box_select_text(box);
    return r;
}
// a label (its word) or a number field holding 1, next on the line; hidden until the panel is placing
internal void ep_field_add(int i, const char *label, float *left, float width, float top, ee_ui_form *form, int *index) {
    float rect[4] = { top + EP_HEIGHT, *left, *left + width, top };   // bottom, left, right, top
    __attribute__((aligned(8))) uint8_t w[32];
    wstring_init_text(w, label ? word(label) : "1");
    ee_ui_control *c = label ? text_label_new((unsigned)(EP_ID + 1 + i), w, ui_get_font_label(), rect, form, index)
                             : edit_box_line_new((unsigned)(EP_ID + 1 + i), EP_FIELD_CHARS, rect, form, index);
    if (c && !label) {
        edit_box_set_text(c, w);
        if (!g_ep_mouse_up && edit_box_vtable_copy(g_ep_vt, (int)(sizeof g_ep_vt / sizeof *g_ep_vt))) {
            g_ep_mouse_up = (long (__thiscall *)(ee_ui_control *))*edit_box_vtable_mouse_up(g_ep_vt);
            *edit_box_vtable_mouse_up(g_ep_vt) = (void *)hook_ep_mouse_up;
        }
        if (g_ep_mouse_up) *(void ***)c = g_ep_vt + 1;
    }
    wstring_free(w);
    *left += width;
    if (!c) return;
    ui_control_set_bg_opacity(c, 0.4f);   // as the panel's own controls
    editor_units_add_control(form, c);
    g_ep_field[i] = c;
    g_ep_field_shown[i] = ui_control_get_mode_now(c);
    ui_control_set_display_mode(c, 0);
}
internal void ep_panel_init(ee_se_unit *p, ee_ui_control *list, ee_ui_form *form, int *index) {
    float top = ui_control_get_bottom(list) + EP_GAP, left = ui_control_get_left(list) + 6.0f;
    float rect[4] = { top + EP_HEIGHT, left, left + EP_WIDTH, top };   // bottom, left, right, top
    __attribute__((aligned(8))) uint8_t w[32];
    wstring_init_text(w, word("sanity_check"));
    ee_ui_control *box = check_box_new(EP_ID, w, rect, form, index);
    wstring_free(w);
    if (!box) return;
    ui_control_set_bg_opacity(box, 0.4f);   // as the panel's own controls
    ui_check_box_label_yellow((ui_check_box *)box);
    ui_check_box_check((ui_check_box *)box, 1);
    editor_units_add_control(form, box);
    g_ep_panel = p;
    g_ep_box = box;
    g_ep_shown = ui_control_get_mode_now(box);
    ui_control_set_display_mode(box, 0);
    if (g_log) log_locked("# editor: sanity check box at %.0f,%.0f\n", left, top);
    top += EP_HEIGHT + EP_ROW_GAP;
    ep_field_add(EP_ROWS_LABEL, "rows", &left, EP_LABEL_ROWS, top, form, index);
    ep_field_add(EP_ROWS, NULL, &left, EP_FIELD_WIDTH, top, form, index);
    left += 8.0f;
    ep_field_add(EP_COLUMNS_LABEL, "columns", &left, EP_LABEL_COLUMNS, top, form, index);
    ep_field_add(EP_COLUMNS, NULL, &left, EP_FIELD_WIDTH, top, form, index);
}
internal void __thiscall hook_ep_update(ee_se_unit *p) {
    eese_unit_update(p);
    if (p != g_ep_panel || !g_ep_box) return;
    int on = se_unit_get_placing(p) != 0;
    ui_control_set_display_mode(g_ep_box, on ? g_ep_shown : 0);
    for (int i = 0; i < EP_FIELDS; i++) if (g_ep_field[i]) ui_control_set_display_mode(g_ep_field[i], on ? g_ep_field_shown[i] : 0);
}
// the panel deleted: the box first, as the search box (editor_search.c)
internal void ep_panel_delete(ee_se_unit *p) {
    if (p != g_ep_panel) return;
    if (g_ep_box) ((void (__thiscall *)(void *, int))(*(void ***)g_ep_box)[0])(g_ep_box, 1);
    for (int i = 0; i < EP_FIELDS; i++) {
        if (g_ep_field[i]) ((void (__thiscall *)(void *, int))(*(void ***)g_ep_field[i])[0])(g_ep_field[i], 1);
        g_ep_field[i] = NULL;
    }
    g_ep_box = NULL;
    g_ep_panel = NULL;
}
