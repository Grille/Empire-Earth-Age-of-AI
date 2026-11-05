// THE DISPLAY (the engine DLLs' rasterizer, screens, surfaces, window, input devices, texts and fonts): fields,
// functions.

// FIELDS (AoC ships its own builds of the engine DLLs - scripts/re/port_dll.py)
// the rasterizer (DX7Rasterizer): its per-screen states +0x20[screen], +0x6C[0] the device state (its +0x1C the device)
internal inline ee_screen_state *rasterizer_get_screen_state(const ee_rasterizer *ras, unsigned screen) { return FIELD(ras, of_rasterizer_screen_states, ee_screen_state **)[screen]; }
// a screen's state: its screen, front and back surfaces, window
// the frame statistics (display.c frame_stats): +0x2C4 a float sum, +0x328 / +0x330 / +0x338 counters, +0x32C frames,
// +0x334 the last time (ms); the averages over 12 frames +0x2C8 +0x2D0 +0x2DC +0x2E4, the rates per second +0x2CC
// +0x2D4 +0x2D8 (frames) +0x2E0
enum scrstate_stat { SS_SUM, SS_SUM_AVG, SS_SUM_RATE, SS_A_AVG, SS_A_RATE, SS_FPS, SS_B_AVG, SS_B_RATE, SS_C_AVG, SS_A,
                     SS_FRAMES, SS_B, SS_LAST, SS_C };
internal inline void *screen_state_stat_at(ee_screen_state *st, enum scrstate_stat k) {
    const uint8_t off[] = { 0x0, 0x4, 0x8, 0xC, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x64, 0x68, 0x6C, 0x70, 0x74 };   // from +0x2C4
    return FIELD_AT(st, of_screen_state_stats + off[k]);
}
#define SS_F(st, k) (*(float *)screen_state_stat_at(st, k))      // the floats (sums, averages, rates)
#define SS_U(st, k) (*(unsigned *)screen_state_stat_at(st, k))   // the counters, the last time (ms)
// a screen (DX7Screen): its modes +0x10 .. +0x14 (0x14 each), the mode +0x38, the desktop's +0x3C, fullscreen +0x3D,
// its DirectDraw +0x30
internal inline uint8_t *screen_get_modes_end(const ee_screen *scr) { return FIELD(scr, of_screen_modes_begin + 0x4, uint8_t *); }
// a surface (DX7Surface): its DirectDraw surface +0x48, the DC handed out +0x54, +0x5D 1 while that DC is out
internal inline void surface_set_dc(ee_surface *s, HDC dc) { FIELD(s, of_surface_dc, HDC) = dc; }
// the game window (GEWindow): its HWND, +0x4 / +0x8 (floats), the game's width and height
internal inline HWND window_get_hwnd(const ee_window *w) { return FIELD(w, of_window_hwnd, HWND); }
// the DirectInput mouse (ISMouse): +0x4 suspended (window inactive), +0x18 the buttons' bytes, +0x44 the device
// a text (GEText): its string +0x34, font +0x44; a font (GEFont): its HFONT +0x18, the bitmap font +0x24
internal inline HFONT font_get_hfont(const ee_font *f) { return FIELD(f, of_font_hfont, HFONT); }
// a model's vertices (DX7StandardVertexArray's data): the i-th, the size of one; the vertex buffer's room (in vertices)
// for one material
internal inline ee_vertex *vertex_at(ee_vertex *data, uint32_t i) { return (ee_vertex *)((uint8_t *)data + (size_t)i * size_d3_dvertex); }
internal inline uint32_t vertex_size(void) { return size_d3_dvertex; }
internal inline uint32_t display_vb_vertices(void) { return value_engine_display_vb_vertices; }

// HOOKS
// the graphics thread's Sleep(1) after every pass of its loop (display.c frame_wait): that call -> hook, Sleep's own type
typedef void (__stdcall *frame_sleep_fn)(DWORD ms);
internal inline int patch_frame_sleep(frame_sleep_fn h) {
    return patch_import_call(data_engine_display_call_frame_sleep, data_engine_display_iat_sleep, (void *)h);
}
// the units' collection's visible tile rows (the terrain mesh's: how many, then each by index - its y, first and last
// x) and its test of an object's model against the view: those calls -> hooks of the imports' own types; the imports
// themselves for the hooks
typedef unsigned long (__thiscall *terrain_rows_fn)(const void *mesh);
typedef void (__thiscall *terrain_row_fn)(const void *mesh, unsigned long i, long *y, long *x0, long *x1);
typedef uint8_t (__thiscall *model_intersects_fn)(const void *model, const void *viewport, uint8_t flag);   // bool: al
internal inline unsigned long terrain_rows(const void *mesh) { return (*(terrain_rows_fn *)data_engine_display_iat_terrain_rows)(mesh); }
internal inline void terrain_row(const void *mesh, unsigned long i, long *y, long *x0, long *x1) {
    (*(terrain_row_fn *)data_engine_display_iat_terrain_row)(mesh, i, y, x0, x1);
}
internal inline uint8_t model_intersects(const void *model, const void *viewport, uint8_t flag) {
    return (*(model_intersects_fn *)data_engine_display_iat_model_intersects)(model, viewport, flag);
}
internal inline int patch_units_rows(terrain_rows_fn h, terrain_row_fn h_row) {
    return patch_import_call(data_engine_display_call_units_rows, data_engine_display_iat_terrain_rows, (void *)h)
         + patch_import_call(data_engine_display_call_units_row, data_engine_display_iat_terrain_row, (void *)h_row);
}
internal inline int patch_units_view(model_intersects_fn h) {
    int n = 0;
    for (size_t i = 0; i < sizeof data_engine_display_calls_units_view / sizeof *data_engine_display_calls_units_view; i++)
        n += patch_import_call(data_engine_display_calls_units_view[i], data_engine_display_iat_model_intersects, (void *)h);
    return n;
}
// its call of mouse_update on the path that draws a frame (one of mouse_update's sites) -> hook
internal inline int patch_frame_mouse(mouse_update_fn h) { return patch_call(data_engine_display_call_frame_mouse, (uintptr_t)fn_mouse_update, (void *)h); }
// DX7HRTnLDisplay.dll at base: the vertex buffer its rasterizer states create in system memory (D3DVBCAPS_SYSTEMMEMORY
// 0x800 added to D3DVBCAPS_WRITEONLY 0x10000, as its other buffer has it), only while the code still asks for video memory:
// 1, 2 when it has the flag already (dreXmod 3.4's FixTnLVertexBuffer does the same in memory), 0 other code
internal inline int tnl_vb_sysmem(uintptr_t base) {
    uintptr_t at = base + value_engine_display_rva_tnl_vb_caps;
    return *(uint32_t *)at == 0x10800 ? 2 : patch_code32(at, 0x10000, 0x10800);
}
