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

// HOOKS
// the graphics thread's Sleep(1) after every pass of its loop (display.c frame_wait): that call -> hook, Sleep's own type
typedef void (__stdcall *frame_sleep_fn)(DWORD ms);
internal inline int patch_frame_sleep(frame_sleep_fn h) {
    return patch_import_call(data_engine_display_call_frame_sleep, data_engine_display_iat_sleep, (void *)h);
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
