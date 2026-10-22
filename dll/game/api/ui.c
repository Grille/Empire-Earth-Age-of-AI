// THE UI: the game's mouse (ee_mouse), the camera and scroll, forms and screens, chat lines and colours, cheats:
// fields, functions, hooks.

// FIELDS
// THE CAMERA ([G_CAMERA], game_camera()) AND THE SCROLL ACTION (EEAScroll)
internal inline float *camera_look_at(const void *cam) { return (float *)FIELD_AT(cam, CAMERA_POS); }   // +0x78 x, y, z
internal inline float camera_get_x(const void *cam) { return FIELD(cam, CAMERA_POS, float); }
internal inline float camera_get_y(const void *cam) { return FIELD(cam, CAMERA_POS + 0x4, float); }
internal inline float camera_get_zoom(const void *cam) { return FIELD(cam, CAMERA_ZOOM, float); }
internal inline unsigned scroll_get_ticks(const void *s) { return FIELD(s, SCROLL_TICKS, unsigned); }    // +0x40 elapsed (10 ms)
internal inline void scroll_set_ticks(void *s, unsigned v) { FIELD(s, SCROLL_TICKS, unsigned) = v; }
// +0x44 .. +0x47 the keys held: left, right, down, up
internal inline uint8_t scroll_get_key(const void *s, int k) { return FIELD(s, SCROLL_KEYS + k, uint8_t); }
internal inline void scroll_set_key(void *s, int k, uint8_t v) { FIELD(s, SCROLL_KEYS + k, uint8_t) = v; }
// the UI: the in-game form's top bar mode (0 age, 1 clock, 2 FPS), a single-player screen's controls, the native chat's player
internal inline int uiform_get_top_bar(const void *form) { return FIELD(form, UIFORM_TOP_BAR, int); }
internal inline void uiform_set_top_bar(void *form, int v) { FIELD(form, UIFORM_TOP_BAR, int) = v; }
internal inline uint8_t **uiscreen_get_controls(const void *screen) { return FIELD(screen, UISCREEN_CONTROLS, uint8_t **); }
internal inline ee_player *nativechat_get_player(const void *nc) { return FIELD(nc, NATIVECHAT_PLAYER, ee_player *); }

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
internal inline void *cheat_parser(void) { return *(void **)GA(G_CHEAT_PARSER); }
// the display and the input (engine/): the camera (EECameraControl: +0x78 look-at, +0xE0 zoom), the scroll speed and
// its setting, the window (GEWindow), the rasterizer and the current screen's index, the UI manager, the mouse
// (RTSMouse), the resolution and fullscreen settings, the sound setting, the draw list's counts, the selection's default
// rectangle, the editor's file buttons, the player name (a UWideString)
internal inline uint8_t *game_camera(void) { return (uint8_t *)GA(G_CAMERA); }
internal inline float scroll_speed(void) { return *(float *)GA(G_SCROLL_SPEED); }
internal inline float scroll_setting(void) { return *(float *)GA(G_SCROLL_SETTING); }
internal inline void *ui_manager(void) { return *(void **)GA(G_UI_MANAGER); }
internal inline ee_mouse *game_mouse(void) { return *(ee_mouse **)GA(G_MOUSE); }

// THE UI AND THE PLAYERS' LINES (voice/)
// a chat or system line on ui: the sender's colour (rgb floats), name and the line (UWideStrings), shown ms
internal inline void ui_line(void *ui, const void *rgb, void *name, void *line, int ms) { GF(FN_UI_LINE)(ui, (void *)rgb, name, line, ms, 0); }
// a typed line read as a cheat by the parser: its code and argument (0: no cheat)
internal inline void cheat_parse(void *parser, void *wtext, int *code, int *arg) { GF(FN_CHEAT_PARSE)(parser, wtext, code, arg); }
// the colour table's rgb of a colour index (the original of persona.c's hook)
internal inline void *game_color_rgb(unsigned idx) { return GF(FN_COLOR_RGB)(idx); }
// one of the game's own AI lines (base: its string block, ms shown; the original of persona.c's hook)
internal inline char native_chat(uint8_t *pim, int base, int ms) { return GF(FN_NATIVE_CHAT)(pim, base, ms); }
internal inline void *camera_action_alloc(void) { return game_new(GA(SIZE_CAMERA_ACTION)); }
// the camera's move to a tile (EEACenterCamera): made, then set (it queues itself)
internal inline void *camera_action_init(void *mem) { return GF(FN_CAMACT_CTOR)(mem); }
internal inline void camera_action_set(void *act, int x, int y) { GF(FN_CAMACT_SET)(act, x, y); }
// the scroll action executed (the original of camera.c's hook), the camera moved by dx, dy; a scroll action object
// made by hand (camera.c's test: vtable, then its fields)
internal inline void scroll_execute(void *self) { GF(FN_SCROLL_EXECUTE)(self); }
internal inline void camera_move(float dx, float dy) { GF(FN_CAMERA_MOVE)((void *)GA(G_CAMERA), dx, dy); }
internal inline void scroll_action_init(uint8_t *obj) { *(uint32_t *)obj = GA(VT_SCROLL); }
// the mouse's position kept in its range, its update (the original of display.c's hook)
internal inline void mouse_clamp(void *m) { ((void (__thiscall *)(void *))GA(FN_MOUSE_CLAMP))(m); }
internal inline void mouse_update(ee_mouse *m) { ((void (__thiscall *)(void *))GA(FN_MOUSE_UPDATE))(m); }
// the UI manager's current form, an edit box's text, the settings saved to the registry (as the options screens do)
internal inline uint8_t *ui_current_form(void *uim) { return ((uint8_t *(__thiscall *)(void *))*(void **)GA(IAT_UIM_FORM))(uim); }
internal inline const char *edit_text(void *edit) { return ((const char *(__thiscall *)(void *, unsigned long))*(void **)GA(IAT_EDIT_GET_TEXT))(edit, 0); }

// HOOKS
// the camera's scroll action (camera.c), and its timer's period: push 25 (AoC 10) ms -> value
typedef void (__fastcall *scroll_hook)(uint8_t *self, void *edx);
internal inline int patch_scroll_execute(scroll_hook h) { return patch_vtable(GA(VT_SCROLL), GA(SLOT_SCROLL_EXECUTE), GA(FN_SCROLL_EXECUTE), (void *)h); }
// the windowed mode's input (display.c): the mouse's update (its 4 calls), the cooperative level of Low-Level Engine's
// input (push 5 -> push 6, exclusive -> non-exclusive: the mouse's Resume and the device setup), no minimize on focus
// loss (je -> jmp)
typedef void (__fastcall *mouse_update_hook)(ee_mouse *m, void *edx);
internal inline int patch_mouse_update(mouse_update_hook h, int *of) { return patch_calls(PATCH_LIST(MOUSE_UPDATE_SITES), GA(FN_MOUSE_UPDATE), (void *)h, of); }
typedef char (__fastcall *native_chat_hook)(uint8_t *pim, void *edx, int base, int ms);
internal inline int patch_native_chat(native_chat_hook h, int *of) { return patch_calls(PATCH_LIST(NATIVE_CHAT_SITES), GA(FN_NATIVE_CHAT), (void *)h, of); }
