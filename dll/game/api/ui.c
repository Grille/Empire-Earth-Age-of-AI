// THE UI: the game's mouse (ee_mouse), the camera and scroll, forms and screens, chat lines and colours, cheats:
// fields, functions, hooks.

// FIELDS
// THE CAMERA ([data_game_camera], game_camera()) AND THE SCROLL ACTION (EEAScroll)
internal inline float *camera_look_at(const ee_camera *cam) { return (float *)FIELD_AT(cam, of_camera_x); }   // +0x78 x, y, z
// +0x44 .. +0x47 the keys held: left, right, down, up
internal inline uint8_t scroll_get_key(const ee_scroll *s, int k) { return FIELD(s, of_scroll_keys + k, uint8_t); }
internal inline void scroll_set_key(ee_scroll *s, int k, uint8_t v) { FIELD(s, of_scroll_keys + k, uint8_t) = v; }
// the UI: the in-game form's top bar mode (0 age, 1 clock, 2 FPS), a single-player screen's controls, the native chat's player

// THE GAME'S MOUSE (game_mouse(): the cursor in the game's coordinates)
// +0xA4 frozen: the update then only reads the wheel

// FUNCTIONS
// THE INTERFACE'S LOCK (UIManager.lock, a TSCriticalSection: its CRITICAL_SECTION at +0, the engine's Enter / Leave keep
// a count and the owner beside it) taken without waiting: 1 held (left with ui_lock_leave), 0 another thread holds it,
// -1 no interface yet
typedef void (__thiscall *ts_cs_fn)(void *cs);
internal inline ts_cs_fn ts_cs_import(const char *name) {
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
    return lle ? (ts_cs_fn)(void *)GetProcAddress(lle, name) : NULL;
}
internal inline int ui_lock_try(void) {
    static ts_cs_fn enter;
    ee_uim *m = (ee_uim *)ui_manager();
    void *cs = m ? uim_get_lock(m) : NULL;
    if (!enter) enter = ts_cs_import("?Enter@TSCriticalSection@@QAEXXZ");
    if (!cs || !enter) return -1;
    if (!TryEnterCriticalSection((CRITICAL_SECTION *)cs)) return 0;
    enter(cs);   // its own count, at once (held already)
    LeaveCriticalSection((CRITICAL_SECTION *)cs);
    return 1;
}
internal inline void ui_lock_leave(void) {
    static ts_cs_fn leave;
    if (!leave) leave = ts_cs_import("?Leave@TSCriticalSection@@QAEXXZ");
    ee_uim *m = (ee_uim *)ui_manager();
    if (m && leave) leave(uim_get_lock(m));
}
// THE UI AND THE PLAYERS' LINES (voice/)
// a chat or system line on ui: the sender's colour (rgb floats), name and the line (UWideStrings), shown ms
internal inline void ui_show_line(void *ui, const void *rgb, void *name, void *line, int ms) { fn_ui_line(ui, (void *)rgb, name, line, ms, 0); }
// a typed line read as a cheat by the parser: its code and argument (0: no cheat)
// the colour table's rgb of a colour index (the original of persona.c's hook)
// one of the game's own AI lines (base: its string block, ms shown; the original of persona.c's hook)
internal inline void *camera_action_alloc(void) { return game_new(value_ui_size_camera_action); }
// the camera's move to a tile (EEACenterCamera): made, then set (it queues itself)
// the scroll action executed (the original of camera.c's hook), the camera moved by dx, dy; a scroll action object
// made by hand (camera.c's test: vtable, then its fields)
internal inline void camera_pan(float dx, float dy) { fn_camera_move((void *)data_game_camera, dx, dy); }
// the camera's step makes a fixed view, not its transformation by blend (game.camera_fixed)
internal inline int camera_fixed_view(void) { return *game_camera_fixed_at() != 0; }
internal inline void scroll_action_init(ee_scroll *obj) { *(uint32_t *)obj = data_ui_vt_scroll; }
// the arrow keys' scrolling (display.c gives it to the editor, which has none): an EEUIEKeyboardScroll event (vtable,
// refcount 1, 0, the direction: 0 up, 1 down, 2 left, 3 right - its Execute moves the camera) made in the game's
// memory, and a form's hotkey for a DIK key code given as the game's own form does (0x64D0AB): any modifiers
// (0x7FFF), the event the second of the four, the last flag set
internal inline void *key_scroll_new(int dir) {
    uint32_t *ev = game_new(value_ui_size_key_scroll);
    if (!ev) return NULL;
    ev[0] = vt_eeuie_keyboard_scroll;
    ev[1] = 1;
    ev[2] = 0;
    ev[3] = (uint32_t)dir;
    return ev;
}
internal inline void ui_form_add_hotkey(void *form, unsigned long key, void *ev) {
    typedef void (__thiscall *AddHotKeyFn)(void *, unsigned long, unsigned long, unsigned long, unsigned long, void *, void *, void *, void *, unsigned long, char);
    (*(AddHotKeyFn *)data_ui_iat_uiform_add_hotkey)(form, key, 0x7FFF, 0x7FFF, 0x7FFF, NULL, ev, NULL, NULL, 0, 1);
}
// the mouse's position kept in its range, its update (the original of display.c's hook)
// the UI manager's current form, an edit box's text, the settings saved to the registry (as the options screens do)
internal inline ee_ui_form *ui_current_form(void *uim) { return ((ee_ui_form *(__thiscall *)(void *))*(void **)data_ui_iat_uim_form)(uim); }
// a form with the 3D view to scroll: the game's screen (EEUIFMainGame) or the scenario editor's
internal inline int ui_form_has_view(const ee_ui_form *f) {
    return f && (*(const uintptr_t *)f == vt_eeuif_main_game || *(const uintptr_t *)f == vt_eeuif_scenario_editor);
}
// the game's screen itself (EEUIFMainGame)
internal inline int ui_form_is_game(const ee_ui_form *f) { return f && *(const uintptr_t *)f == vt_eeuif_main_game; }
internal inline const char *edit_text(void *edit) { return ((const char *(__thiscall *)(void *, unsigned long))*(void **)data_ui_iat_edit_get_text)(edit, 0); }
// a text label's text (UITextLabel::SetText(const UWideString &, bool) - the game passes 1: the diplomacy screen)
internal inline void label_set_text(void *label, void *wstr) {
    ((void (__thiscall *)(void *, void *, char))*(void **)data_ui_imports_iat_text_label_set_text)(label, wstr, 1);
}
// THE SCENARIO EDITOR'S CONTROLS (engine/editor_search.c): an edit box made as the trigger screen makes its own
// (0x57ED81: UIEditBox's constructor with the editor's font, then the exe's UIEditBox vtable) - rect a GERectangle
// {bottom, left, right, top} in the form's coordinates, index the form's running control index (the box takes one);
// NULL: no memory. A control into the form, shown with group's controls; into the editor's units panel (its group)
internal inline ee_ui_control *edit_box_new(unsigned id, const float rect[4], ee_ui_form *form, int *index) {
    ee_ui_control *box = game_new(size_ui_edit_box);
    if (!box) return NULL;
    typedef void *(__thiscall *ctor_fn)(void *, unsigned long, void *, unsigned long, char, const float *, ee_ui_form *,
                                        unsigned long, unsigned long, char, char);
    (*(ctor_fn *)data_ui_iat_edit_ctor)(box, id, ui_get_font_edit(), 5, 0, rect, form, (unsigned long)*index, 2, 1, 0);
    *(uint32_t *)box = vt_ui_edit_box;
    (*index)++;
    return box;
}
internal inline void ui_form_add_control(ee_ui_form *form, ee_ui_control *c, unsigned long group) {
    ((void (__thiscall *)(ee_ui_form *, ee_ui_control *, unsigned long))*(void **)data_ui_imports_iat_form_add_control)(form, c, group);
}
internal inline void editor_units_add_control(ee_ui_form *form, ee_ui_control *c) { ui_form_add_control(form, c, value_editor_group_units); }
// the terrain panel's colour types (EEUIFSETerrain.color_types: an MSVC vector of pointers, its allocator first), an
// item of its colour list each: how many; one more at the end (the game's push_back); the last off (nothing to free)
internal inline unsigned se_terrain_color_type_count(const ee_se_terrain *p) {
    uint8_t *v = se_terrain_color_types_at(p);
    ee_color_type **first = FIELD(v, 4, ee_color_type **), **end = FIELD(v, 8, ee_color_type **);
    return first ? (unsigned)(end - first) : 0;
}
internal inline void se_terrain_color_type_push(ee_se_terrain *p, ee_color_type *t) {
    void *item = t;
    ptr_vector_push(se_terrain_color_types_at(p), &item);
}
internal inline void se_terrain_color_type_pop(ee_se_terrain *p) {
    if (se_terrain_color_type_count(p)) FIELD(se_terrain_color_types_at(p), 8, ee_color_type **)--;
}
// an item more in the colour list: n colours (3 floats each) of the caller's, copied (an MSVC vector of GEColor)
internal inline void se_color_list_add_colors(ee_se_color_list *list, float (*colors)[3], unsigned n) {
    void *v[4] = { NULL, colors, colors + n, colors + n };
    se_color_list_add(list, v);
}
// an edit box of the mod's own kind: UIEditBox's vtable with its RTTI pointer before it copied into vt (room entries:
// ui.edit_box_slots + 1) - the box's vtable is then vt + 1; the copy's UpdateKeyboard slot (called for the box while
// it has the keyboard) for the caller's hook, NULL: no room
internal inline void **edit_box_vtable_copy(void **vt, int room) {
    int n = (int)value_ui_edit_box_slots + 1;
    if (room < n) return NULL;
    memcpy(vt, (void **)(uintptr_t)vt_ui_edit_box - 1, (size_t)n * sizeof *vt);
    return &vt[1 + slot_ui_control_update_keyboard];
}
// the form's pixel x, y within the control's rectangle (GERectangle::Contains, as UIForm::UpdateMouseLocation finds
// the control under the mouse)
internal inline int ui_control_contains(const ee_ui_control *c, float x, float y) {
    float t = ui_control_get_top(c), b = ui_control_get_bottom(c);
    return x >= ui_control_get_left(c) && x <= ui_control_get_right(c) && y >= (t < b ? t : b) && y <= (t < b ? b : t);
}
internal inline int ui_control_under_mouse(const ee_ui_control *c, const ee_ui_form *f) {
    return ui_control_contains(c, ui_form_get_mouse_x(f), ui_form_get_mouse_y(f));
}
// the minimap's point (its 800 x 600 pixels) under the form's pixel x, y, as its mouse finds it (0x65EAFA): by the
// scale its code reads (hooks.minimap_fx_ops / fy_ops - the mod's own resolution's when display.c redirected them),
// kept in game.minimap_bounds
internal inline void minimap_point(const eeui_mini_map *c, float x, float y, float p[2]) {
    ee_window *w = game_window();
    float fx = *(const float *)(uintptr_t)*(const uint32_t *)data_hooks_minimap_fx_ops[0];
    float fy = *(const float *)(uintptr_t)*(const uint32_t *)data_hooks_minimap_fy_ops[0];
    const float *r = (const float *)game_minimap_bounds_at();   // bottom, left, right, top
    p[0] = (x - eeui_mini_map_get_origin_x(c)) / ((float)(unsigned)window_get_width(w) * fx);
    p[1] = (y - eeui_mini_map_get_origin_y(c)) / ((float)(unsigned)window_get_height(w) * fy);
    if (p[0] < r[1]) p[0] = r[1];
    if (p[0] > r[2]) p[0] = r[2];
    if (p[1] < r[3]) p[1] = r[3];
    if (p[1] > r[0]) p[1] = r[0];
}
// an EEAMiniMap of the caller's (0x1C bytes) that centres the camera on the minimap's point p when run
internal inline ee_minimap_action *minimap_action_init(void *mem, const float p[2]) {
    memset(mem, 0, size_eea_mini_map);
    *(uint32_t *)mem = vt_eea_mini_map;
    ((uint32_t *)mem)[1] = 1;   // its reference count
    ee_minimap_action *a = mem;
    minimap_action_set_x(a, p[0]);
    minimap_action_set_y(a, p[1]);
    return a;
}
// every edit box's UpdateKeyboard (the exe's import: its vtables reach it through it, the mod's copy too) - the
// original into *orig
internal inline int patch_edit_box_keyboard(ui_control_update_keyboard_fn h, ui_control_update_keyboard_fn *orig) {
    return iat_hook(data_ui_iat_edit_keyboard, "Low-Level Engine.dll", "?UpdateKeyboard@UIEditBox@@UAEJXZ", (void *)h, (void **)orig);
}
// a control event of the UI database by its id, made by the event creator that knows it (ui.event_creators: the
// index in that list) for form; NULL: none
internal inline ee_ui_event *ui_event_new(int creator, ee_ui_form *form, int id) {
    return ((ee_ui_event *(__cdecl *)(ee_ui_form *, int))(uintptr_t)data_ui_event_creators[creator])(form, id);
}
// A COMMAND'S CIRCLES (transports.c): the graphical effects' kinds by their ids (gfx_kind_game's records; the same in
// both builds) - what the code that draws them shows them for
enum gfx_kind {
    GFX_ORDER_ON = 0xB8,       // the circle at a unit's goal, drawn with the unit (order_marks, per unit); its colour the unit's order's - yellow an unload, else purple (seen with a load)
    GFX_ORDER_ATTACK = 0xB9,   // the attack-ground command's (order_marks)
    GFX_ORDER_AT = 0xBA,       // the yellow circle at a goal (order_marks: one for the group); with a unit it is drawn on the unit (seen), its size scaled by the unit's - without, the record's size ends it before it is grown (seen): gfx_mark sizes it
    GFX_CLICK = 0xD1,          // the click confirmation on the ground (the mouse click, 0x404A26; 0x663138)
    GFX_CLICK_ON = 0xD3,       // the click confirmation on the unit clicked
    GFX_GOAL_0 = 0x60, GFX_GOAL_1 = 0x62, GFX_GOAL_2 = 0xD2,   // the selected units' goal markers (the graphical goal
    GFX_GOAL_3 = 0xD4, GFX_GOAL_4 = 0xD5, GFX_GOAL_5 = 0xD6,   // manager 0x4FCDF9, its six kinds 0..5: 0x4FCB50)
};
enum { ORDER_MARKS_GOALS = 0xBB };   // order_marks' how: a GFX_ORDER_ON at each unit's goal (the group's unload's)
// graphical effects by handle - added (the original of the click's hook), gone already (they fade) or taken away (as
// the game's area effects end theirs); a command's circles as the game draws them (every PC, seen by owner - a player
// index): of kind k at a tile's centre on the ground, or at each of a group's units' goals
internal inline int gfx_alive(int handle) { return fn_effect_mgr_get(*(void **)data_ui_gfx_effects, handle) != NULL; }
internal inline void gfx_remove(int handle) { if (gfx_alive(handle)) fn_effect_mgr_remove(*(void **)data_ui_gfx_effects, handle); }
// - sized as for a unit of type size_of (NULL: the record's own size): drawn without the unit (with one it is drawn on
// the unit, coloured by its order), then the size the effect's set-up (vf12, run once in effect_mgr_add) gives a unit's
internal inline void gfx_mark(enum gfx_kind k, const ee_unit_type *size_of, int x, int y, int owner) {
    void *fx = *(void **)data_ui_gfx_effects, *rec = fn_gfx_kind_game((unsigned)k);
    ee_map *map = game_map();
    unsigned n = map ? map_get_size(map) : 0;
    if (!fx || !rec || x < 0 || y < 0 || (unsigned)x >= n || (unsigned)y >= n) return;
    ee_tile *t = map_get_tiles(map)[(unsigned)y * n + (unsigned)x];
    float pos[3] = { (float)x + 0.5f, (float)y + 0.5f, t && tile_get_height(t) > 0.0f ? tile_get_height(t) : 0.0f };
    int h = 0;
    fn_effect_mgr_add(fx, NULL, pos, rec, &h, 0);
    ee_gfx *e = h ? fn_effect_mgr_get(fx, h) : NULL;
    if (!e) return;
    if (size_of) {
        float s = FIELD(size_of, of_unit_type_mark_size, float) * 1.25f;   // vf12's factor (0x827588)
        FIELD(e, of_gfx_size, float) *= s;
        FIELD(e, of_gfx_size_2, float) = s;
    }
    FIELD(e, of_gfx_shown, uint8_t) = 1;
    FIELD(e, of_gfx_owner, int) = owner;
    FIELD(e, of_gfx_0x2e, uint8_t) = 0;
}

// HOOKS
// the camera's scroll action (camera.c), and its timer's period: push 25 (AoC 10) ms -> value
typedef void (__thiscall *scroll_hook)(ee_scroll *self);
internal inline int patch_scroll_execute(scroll_hook h) { return patch_vtable(data_ui_vt_scroll, value_ui_slot_scroll_execute, (uintptr_t)fn_scroll_execute, (void *)h); }
// the windowed mode's input (display.c): the mouse's update (its 4 calls), the cooperative level of Low-Level Engine's
// input (push 5 -> push 6, exclusive -> non-exclusive: the mouse's Resume and the device setup), no minimize on focus
// loss (je -> jmp)
// the scroll action's timer (camera.c): its push old_ms -> ms
internal inline int patch_scroll_timer(uint8_t old_ms, uint8_t ms) {
    uint8_t *p = (uint8_t *)data_ui_site_scroll_timer;
    if (p[-1] != 0x6A || p[0] != old_ms) return 0;
    DWORD old;
    VirtualProtect(p, 1, PAGE_EXECUTE_READWRITE, &old);
    *p = ms;
    VirtualProtect(p, 1, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 1);
    return 1;
}
// the single-player menu's six buttons (playername.c): their handlers (vtable slot 1) -> hooks[i], the originals into orig
internal inline int patch_sp_buttons(void *const hooks[6], void *orig[6]) {
    int ok = 0;
    for (int i = 0; i < 6; i++) {
        orig[i] = (void *)data_ui_sp_fns[i];
        ok += patch_vtable(data_ui_sp_vtables[i], value_ui_slot_sp_button, data_ui_sp_fns[i], hooks[i]);
    }
    return ok;
}
// the editor's main form built (display.c)
// a command's circle (transports.c): the mouse click's one effect (on: the unit clicked, its id +0x24; NULL the ground)
internal inline int click_mark_on_id(const void *on) { return FIELD(on, of_unit_serial, int); }
// the group's unload order's circles (transports.c: the spread comes first)
// the players' colours (voice/persona.c)
