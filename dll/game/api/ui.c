// THE UI: the game's mouse (ee_mouse), the camera and scroll, forms and screens, chat lines and colours, cheats:
// fields, functions, hooks.

// FIELDS
// THE CAMERA ([G_CAMERA], game_camera()) AND THE SCROLL ACTION (EEAScroll)
internal inline float *camera_look_at(const ee_camera *cam) { return (float *)FIELD_AT(cam, CAMERA_POS); }   // +0x78 x, y, z
internal inline float camera_get_x(const ee_camera *cam) { return FIELD(cam, CAMERA_POS, float); }
internal inline float camera_get_y(const ee_camera *cam) { return FIELD(cam, CAMERA_POS + 0x4, float); }
internal inline float camera_get_zoom(const ee_camera *cam) { return FIELD(cam, CAMERA_ZOOM, float); }
internal inline unsigned scroll_get_ticks(const ee_scroll *s) { return FIELD(s, SCROLL_TICKS, unsigned); }    // +0x40 elapsed (10 ms)
internal inline void scroll_set_ticks(ee_scroll *s, unsigned v) { FIELD(s, SCROLL_TICKS, unsigned) = v; }
// +0x44 .. +0x47 the keys held: left, right, down, up
internal inline uint8_t scroll_get_key(const ee_scroll *s, int k) { return FIELD(s, SCROLL_KEYS + k, uint8_t); }
internal inline void scroll_set_key(ee_scroll *s, int k, uint8_t v) { FIELD(s, SCROLL_KEYS + k, uint8_t) = v; }
// the UI: the in-game form's top bar mode (0 age, 1 clock, 2 FPS), a single-player screen's controls, the native chat's player
internal inline int uiform_get_top_bar(const ee_ui_form *form) { return FIELD(form, UIFORM_TOP_BAR, int); }
internal inline void uiform_set_top_bar(ee_ui_form *form, int v) { FIELD(form, UIFORM_TOP_BAR, int) = v; }
internal inline uint8_t **uiscreen_get_controls(const ee_ui_screen *screen) { return FIELD(screen, UISCREEN_CONTROLS, uint8_t **); }
internal inline ee_player *nativechat_get_player(const ee_native_chat *nc) { return FIELD(nc, NATIVECHAT_PLAYER, ee_player *); }

// THE GAME'S MOUSE (game_mouse(): the cursor in the game's coordinates)
internal inline float mouse_get_x(const ee_mouse *m) { return FIELD(m, MOUSE_X, float); }            // +0x4
internal inline void mouse_set_x(ee_mouse *m, float v) { FIELD(m, MOUSE_X, float) = v; }
internal inline float mouse_get_y(const ee_mouse *m) { return FIELD(m, MOUSE_Y, float); }            // +0x8
internal inline void mouse_set_y(ee_mouse *m, float v) { FIELD(m, MOUSE_Y, float) = v; }
internal inline float mouse_get_y_max(const ee_mouse *m) { return FIELD(m, MOUSE_Y_MAX, float); }       // +0x14
internal inline float mouse_get_x_min(const ee_mouse *m) { return FIELD(m, MOUSE_X_MIN, float); }       // +0x18
internal inline float mouse_get_x_max(const ee_mouse *m) { return FIELD(m, MOUSE_X_MAX, float); }       // +0x1C
internal inline float mouse_get_y_min(const ee_mouse *m) { return FIELD(m, MOUSE_Y_MIN, float); }       // +0x20
// +0xA4 frozen: the update then only reads the wheel
internal inline uint8_t mouse_get_frozen(const ee_mouse *m) { return FIELD(m, MOUSE_FROZEN, uint8_t); }
internal inline void mouse_set_frozen(ee_mouse *m, uint8_t v) { FIELD(m, MOUSE_FROZEN, uint8_t) = v; }

// FUNCTIONS
// THE UI AND THE PLAYERS' LINES (voice/)
// a chat or system line on ui: the sender's colour (rgb floats), name and the line (UWideStrings), shown ms
internal inline void ui_line(void *ui, const void *rgb, void *name, void *line, int ms) { GF(FN_UI_LINE)(ui, (void *)rgb, name, line, ms, 0); }
// a typed line read as a cheat by the parser: its code and argument (0: no cheat)
internal inline void cheat_parse(void *parser, void *wtext, int *code, int *arg) { GF(FN_CHEAT_PARSE)(parser, wtext, code, arg); }
// the colour table's rgb of a colour index (the original of persona.c's hook)
internal inline void *game_color_rgb(unsigned idx) { return GF(FN_COLOR_RGB)(idx); }
// one of the game's own AI lines (base: its string block, ms shown; the original of persona.c's hook)
internal inline char native_chat(ee_native_chat *pim, int base, int ms) { return GF(FN_NATIVE_CHAT)(pim, base, ms); }
internal inline void *camera_action_alloc(void) { return game_new(GA(SIZE_CAMERA_ACTION)); }
// the camera's move to a tile (EEACenterCamera): made, then set (it queues itself)
internal inline void *camera_action_init(void *mem) { return GF(FN_CAMACT_CTOR)(mem); }
internal inline void camera_action_set(void *act, int x, int y) { GF(FN_CAMACT_SET)(act, x, y); }
// the scroll action executed (the original of camera.c's hook), the camera moved by dx, dy; a scroll action object
// made by hand (camera.c's test: vtable, then its fields)
internal inline void scroll_execute(void *self) { GF(FN_SCROLL_EXECUTE)(self); }
internal inline void camera_move(float dx, float dy) { GF(FN_CAMERA_MOVE)((void *)GA(G_CAMERA), dx, dy); }
internal inline void scroll_action_init(ee_scroll *obj) { *(uint32_t *)obj = GA(VT_SCROLL); }
// the arrow keys' scrolling (display.c gives it to the editor, which has none): an EEUIEKeyboardScroll event (vtable,
// refcount 1, 0, the direction: 0 up, 1 down, 2 left, 3 right - its Execute moves the camera) made in the game's
// memory, and a form's hotkey for a DIK key code given as the game's own form does (0x64D0AB): any modifiers
// (0x7FFF), the event the second of the four, the last flag set
internal inline void *key_scroll_new(int dir) {
    uint32_t *ev = game_new(GA(SIZE_KEY_SCROLL));
    if (!ev) return NULL;
    ev[0] = GA(VT_KEY_SCROLL);
    ev[1] = 1;
    ev[2] = 0;
    ev[3] = (uint32_t)dir;
    return ev;
}
internal inline void uiform_add_hotkey(void *form, unsigned long key, void *ev) {
    typedef void (__thiscall *AddHotKeyFn)(void *, unsigned long, unsigned long, unsigned long, unsigned long, void *, void *, void *, void *, unsigned long, char);
    (*(AddHotKeyFn *)GA(IAT_UIFORM_ADD_HOTKEY))(form, key, 0x7FFF, 0x7FFF, 0x7FFF, NULL, ev, NULL, NULL, 0, 1);
}
// the mouse's position kept in its range, its update (the original of display.c's hook)
internal inline void mouse_clamp(void *m) { GF(FN_MOUSE_CLAMP)(m); }
internal inline void mouse_update(ee_mouse *m) { GF(FN_MOUSE_UPDATE)(m); }
// the UI manager's current form, an edit box's text, the settings saved to the registry (as the options screens do)
internal inline ee_ui_form *ui_current_form(void *uim) { return ((ee_ui_form *(__thiscall *)(void *))*(void **)GA(IAT_UIM_FORM))(uim); }
internal inline const char *edit_text(void *edit) { return ((const char *(__thiscall *)(void *, unsigned long))*(void **)GA(IAT_EDIT_GET_TEXT))(edit, 0); }
// A COMMAND'S CIRCLES (transports.c): graphical effects by handle - added (the original of the click's hook), gone
// already (they fade) or taken away (as the game's area effects end theirs); a click's circle on a tile's centre for the
// player on this PC (only this PC's own command's: it draws for the PC's player)
internal inline int gfx_add(void *fx, void *on, float *pos, void *kind, int *handle, int arg) { return GF(FN_GFX_ADD)(fx, on, pos, kind, handle, arg); }
internal inline int gfx_alive(int handle) { return GF(FN_GFX_GET)(*(void **)GA(G_GFX_EFFECTS), handle) != NULL; }
internal inline void gfx_remove(int handle) { if (gfx_alive(handle)) GF(FN_GFX_REMOVE)(*(void **)GA(G_GFX_EFFECTS), handle); }
internal inline void mark_tile(int x, int y) { float p[3] = { (float)x + 0.5f, (float)y + 0.5f, 0.0f }; GF(FN_MARK_AT)(p, NULL); }

// HOOKS
// the camera's scroll action (camera.c), and its timer's period: push 25 (AoC 10) ms -> value
typedef void (__thiscall *scroll_hook)(ee_scroll *self);
internal inline int patch_scroll_execute(scroll_hook h) { return patch_vtable(GA(VT_SCROLL), GA(SLOT_SCROLL_EXECUTE), GA(FN_SCROLL_EXECUTE), (void *)h); }
// the windowed mode's input (display.c): the mouse's update (its 4 calls), the cooperative level of Low-Level Engine's
// input (push 5 -> push 6, exclusive -> non-exclusive: the mouse's Resume and the device setup), no minimize on focus
// loss (je -> jmp)
typedef void (__thiscall *mouse_update_hook)(ee_mouse *m);
internal inline int patch_mouse_update(mouse_update_hook h, int *of) { return patch_calls(PATCH_LIST(MOUSE_UPDATE_SITES), GA(FN_MOUSE_UPDATE), (void *)h, of); }
typedef char (__thiscall *native_chat_hook)(ee_native_chat *pim, int base, int ms);
internal inline int patch_native_chat(native_chat_hook h, int *of) { return patch_calls(PATCH_LIST(NATIVE_CHAT_SITES), GA(FN_NATIVE_CHAT), (void *)h, of); }
// the scroll action's timer (camera.c): its push old_ms -> ms
internal inline int patch_scroll_timer(uint8_t old_ms, uint8_t ms) {
    uint8_t *p = (uint8_t *)GA(SITE_SCROLL_TIMER);
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
        orig[i] = (void *)GL(SP_FNS)[i];
        ok += patch_vtable(GL(SP_VTABLES)[i], GA(SLOT_SP_BUTTON), GL(SP_FNS)[i], hooks[i]);
    }
    return ok;
}
// the editor's main form built (display.c)
typedef void *(__thiscall *editor_form_hook)(void *form, void *owner);
internal inline int patch_editor_form(editor_form_hook h) { return patch_call(GA(SITE_EDITOR_FORM), GA(FN_EDITOR_FORM), (void *)h); }
// a command's circle (transports.c): the mouse click's one effect (on: the unit clicked, its id +0x24; NULL the ground)
typedef int (__thiscall *click_mark_hook)(void *fx, void *on, float *pos, void *kind, int *handle, int arg);
internal inline int patch_click_mark(click_mark_hook h) { return patch_call(GA(CALL_CLICK_MARK), GA(FN_GFX_ADD), (void *)h); }
internal inline int click_mark_on_id(const void *on) { return FIELD(on, UNIT_SERIAL, int); }
// the players' colours (voice/persona.c)
typedef void *(__cdecl *player_color_hook)(unsigned idx);
internal inline int patch_player_color(player_color_hook h) { return patch_call(GA(CALL_PLAYER_COLOR), GA(FN_COLOR_RGB), (void *)h); }
