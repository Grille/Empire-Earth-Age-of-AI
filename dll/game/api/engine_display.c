// THE DISPLAY (the engine DLLs' rasterizer, screens, surfaces, window, input devices, texts and fonts): fields,
// functions.

// FIELDS (AoC ships its own builds of the engine DLLs - scripts/port_dll.py)
// the rasterizer (DX7Rasterizer): its per-screen states +0x20[screen], +0x6C[0] the device state (its +0x1C the device)
internal inline ee_screen_state *ras_get_screen_state(const ee_rasterizer *ras, unsigned screen) { return FIELD(ras, E_RAS_SCREEN_STATES, ee_screen_state **)[screen]; }
internal inline ee_screen_state **ras_get_screen_states(const ee_rasterizer *ras) { return FIELD(ras, E_RAS_SCREEN_STATES, ee_screen_state **); }
internal inline ee_draw_state **ras_get_dstates(const ee_rasterizer *ras) { return FIELD(ras, E_RAS_DSTATES, ee_draw_state **); }
internal inline void *dstate_get_device(const ee_draw_state *ds) { return FIELD(ds, E_DSTATE_DEVICE, void *); }
// a screen's state: its screen, front and back surfaces, window
internal inline ee_screen *scrstate_get_screen(const ee_screen_state *st) { return FIELD(st, E_SCRSTATE_SCREEN, ee_screen *); }
internal inline ee_surface *scrstate_get_front(const ee_screen_state *st) { return FIELD(st, E_SCRSTATE_FRONT, ee_surface *); }
internal inline ee_surface *scrstate_get_back(const ee_screen_state *st) { return FIELD(st, E_SCRSTATE_BACK, ee_surface *); }
internal inline ee_window *scrstate_get_window(const ee_screen_state *st) { return FIELD(st, E_SCRSTATE_WINDOW, ee_window *); }
// the frame statistics (display.c frame_stats): +0x2C4 a float sum, +0x328 / +0x330 / +0x338 counters, +0x32C frames,
// +0x334 the last time (ms); the averages over 12 frames +0x2C8 +0x2D0 +0x2DC +0x2E4, the rates per second +0x2CC
// +0x2D4 +0x2D8 (frames) +0x2E0
enum scrstate_stat { SS_SUM, SS_SUM_AVG, SS_SUM_RATE, SS_A_AVG, SS_A_RATE, SS_FPS, SS_B_AVG, SS_B_RATE, SS_C_AVG, SS_A,
                     SS_FRAMES, SS_B, SS_LAST, SS_C };
internal inline void *scrstate_stat_at(ee_screen_state *st, enum scrstate_stat k) {
    const uint8_t off[] = { 0x0, 0x4, 0x8, 0xC, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x64, 0x68, 0x6C, 0x70, 0x74 };   // from +0x2C4
    return FIELD_AT(st, E_SCRSTATE_STATS + off[k]);
}
#define SS_F(st, k) (*(float *)scrstate_stat_at(st, k))      // the floats (sums, averages, rates)
#define SS_U(st, k) (*(unsigned *)scrstate_stat_at(st, k))   // the counters, the last time (ms)
// a screen (DX7Screen): its modes +0x10 .. +0x14 (0x14 each), the mode +0x38, the desktop's +0x3C, fullscreen +0x3D,
// its DirectDraw +0x30
internal inline uint8_t *screen_get_modes_begin(const ee_screen *scr) { return FIELD(scr, E_SCREEN_MODES, uint8_t *); }
internal inline uint8_t *screen_get_modes_end(const ee_screen *scr) { return FIELD(scr, E_SCREEN_MODES + 0x4, uint8_t *); }
internal inline void *screen_get_ddraw(const ee_screen *scr) { return FIELD(scr, E_SCREEN_DDRAW, void *); }   // IDirectDraw7 *
internal inline unsigned screen_get_mode(const ee_screen *scr) { return FIELD(scr, E_SCREEN_MODE, unsigned); }
internal inline void screen_set_mode(ee_screen *scr, unsigned v) { FIELD(scr, E_SCREEN_MODE, unsigned) = v; }
internal inline uint8_t screen_get_desktop(const ee_screen *scr) { return FIELD(scr, E_SCREEN_DESKTOP, uint8_t); }
internal inline uint8_t screen_get_fullscreen(const ee_screen *scr) { return FIELD(scr, E_SCREEN_FULLSCREEN, uint8_t); }
// a surface (DX7Surface): its DirectDraw surface +0x48, the DC handed out +0x54, +0x5D 1 while that DC is out
internal inline void *surf_get_dds(const ee_surface *s) { return FIELD(s, E_SURF_DDS, void *); }   // IDirectDrawSurface7 *
internal inline void surf_set_dc(ee_surface *s, HDC dc) { FIELD(s, E_SURF_DC, HDC) = dc; }
internal inline uint8_t surf_get_dc_out(const ee_surface *s) { return FIELD(s, E_SURF_DC_OUT, uint8_t); }
internal inline void surf_set_dc_out(ee_surface *s, uint8_t v) { FIELD(s, E_SURF_DC_OUT, uint8_t) = v; }
// the game window (GEWindow): its HWND, +0x4 / +0x8 (floats), the game's width and height
internal inline HWND window_get_hwnd(const ee_window *w) { return FIELD(w, E_WINDOW_HWND, HWND); }
internal inline float window_get_4(const ee_window *w) { return FIELD(w, E_WINDOW_0x4, float); }
internal inline float window_get_8(const ee_window *w) { return FIELD(w, E_WINDOW_0x8, float); }
internal inline int window_get_width(const ee_window *w) { return FIELD(w, E_WINDOW_WIDTH, int); }
internal inline int window_get_height(const ee_window *w) { return FIELD(w, E_WINDOW_HEIGHT, int); }
// the DirectInput mouse (ISMouse): +0x4 suspended (window inactive), +0x18 the buttons' bytes, +0x44 the device
internal inline uint8_t imouse_get_suspended(const ee_input_mouse *ms) { return FIELD(ms, E_MOUSE_SUSPENDED, uint8_t); }
internal inline uint8_t *imouse_buttons_at(ee_input_mouse *ms) { return (uint8_t *)FIELD_AT(ms, E_MOUSE_BUTTONS); }
internal inline void **imouse_get_device(const ee_input_mouse *ms) { return FIELD(ms, E_MOUSE_DEVICE, void **); }
// a text (GEText): its string +0x34, font +0x44; a font (GEFont): its HFONT +0x18, the bitmap font +0x24
internal inline const char *text_get_string(const ee_text *t) { return FIELD(t, E_TEXT_STRING, const char *); }
internal inline ee_font *text_get_font(const ee_text *t) { return FIELD(t, E_TEXT_FONT, ee_font *); }
internal inline HFONT font_get_hfont(const ee_font *f) { return FIELD(f, E_FONT_HFONT, HFONT); }
internal inline void *font_get_bitmap(const ee_font *f) { return FIELD(f, E_FONT_BITMAP, void *); }
internal inline void font_set_bitmap(ee_font *f, void *b) { FIELD(f, E_FONT_BITMAP, void *) = b; }
