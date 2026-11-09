// THE EDITOR'S "Sanity check" BOX: a check box in the strip under the units panel, ticked as the game is, shown only while
// a unit type is being placed (the strip holds a selected unit's stats otherwise). Unticked,
// a unit can be placed where it would collide with another or on ground that doesn't suit it. What may be placed is the
// ghost's own mark (EEGhostUnit.can_place, drawn green or red), which the preview works out for each ghost at every mouse
// move and EESEAPlaceUnit reads at the click; so the editor's own two calls of the preview (the game's aren't touched) say
// yes afterwards for a ghost whose ground is all on the map. Never for a gate: it is made of the wall under it. A wall's
// line has a preview of its own and keeps its rules.
#define EP_ID 0x3F9          // after the search box's (editor_search.c)
#define EP_WIDTH 140.0f
#define EP_HEIGHT 18.0f
#define EP_GAP 18.0f         // under the list: inside the strip
internal ee_se_unit *g_ep_panel;       // the editor's units panel, while it is
internal ee_ui_control *g_ep_box;
internal uint32_t g_ep_shown;          // the box's display mode as made

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
internal void __cdecl hook_ep_preview(float *loc) {
    ghost_unit_preview(loc);
    if (!g_ep_box || ui_check_box_get_checked_state((ui_check_box *)g_ep_box)) return;
    void *v = ghost_units();
    for (uint32_t i = 0; i < vector_size(v); i++) ep_allow(*(ee_ghost_unit **)type_stats_game(v, (int)i));
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
}
internal void __thiscall hook_ep_update(ee_se_unit *p) {
    eese_unit_update(p);
    if (p == g_ep_panel && g_ep_box) ui_control_set_display_mode(g_ep_box, se_unit_get_placing(p) ? g_ep_shown : 0);
}
// the panel deleted: the box first, as the search box (editor_search.c)
internal void ep_panel_delete(ee_se_unit *p) {
    if (p != g_ep_panel) return;
    if (g_ep_box) ((void (__thiscall *)(void *, int))(*(void ***)g_ep_box)[0])(g_ep_box, 1);
    g_ep_box = NULL;
    g_ep_panel = NULL;
}
