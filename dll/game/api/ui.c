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
internal inline void scroll_action_init(ee_scroll *obj) { *(uint32_t *)obj = data_ui_vt_scroll; }
// the arrow keys' scrolling (display.c gives it to the editor, which has none): an EEUIEKeyboardScroll event (vtable,
// refcount 1, 0, the direction: 0 up, 1 down, 2 left, 3 right - its Execute moves the camera) made in the game's
// memory, and a form's hotkey for a DIK key code given as the game's own form does (0x64D0AB): any modifiers
// (0x7FFF), the event the second of the four, the last flag set
internal inline void *key_scroll_new(int dir) {
    uint32_t *ev = game_new(value_ui_size_key_scroll);
    if (!ev) return NULL;
    ev[0] = data_ui_vt_key_scroll;
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
internal inline const char *edit_text(void *edit) { return ((const char *(__thiscall *)(void *, unsigned long))*(void **)data_ui_iat_edit_get_text)(edit, 0); }
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
