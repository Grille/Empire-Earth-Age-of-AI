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
// a display DLL's LockVB started anew at `room` vertices instead of display_vb_vertices: 1 done, 0 other code
internal inline int display_lock_vb_room(HMODULE m, uint32_t room) {
    uintptr_t f = (uintptr_t)GetProcAddress(m, "?LockVB@DX7Rasterizer@@IAEJKKAAPAUD3DVERTEX_STD_COLOR@@@Z");
    return f && patch_code32(f + value_engine_display_of_lock_vb_room, value_engine_display_vb_vertices, room);
}

// HOOKS
// the units' draw's world into the camera (the map's viewport's, made at the camera's step - render_frame.c
// rf_view_now): its operands in the draw -> t (a GETransformation as the viewport's: 16 floats), the number changed;
// the viewport's own
internal inline int patch_viewport_view(const float *t) {
    int n = 0;
    for (int i = 0; i < (int)(sizeof data_engine_display_viewport_view_ops / sizeof *data_engine_display_viewport_view_ops); i++)
        n += patch_code32(data_engine_display_viewport_view_ops[i], data_engine_display_viewport_view, (uint32_t)(uintptr_t)t);
    return n;
}
internal inline const float *viewport_view(void) { return (const float *)(uintptr_t)data_engine_display_viewport_view; }
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
// the ground's visible region made (GETerrainMesh::UpdateVisibleTerrain on the mesh: the camera's transformation, the
// viewport, three numbers, the fixed camera's flag): its call in the ground's update -> a hook of the import's type;
// the viewport's rectangle its frustum goes through (bottom, left, right, top); the region's size in tiles less one
typedef void (__thiscall *terrain_visible_fn)(void *mesh, const void *camera, void *viewport, unsigned long a, unsigned long b,
                                              unsigned long c, uint8_t fixed);
internal inline void terrain_visible(void *mesh, const void *camera, void *viewport, unsigned long a, unsigned long b,
                                     unsigned long c, uint8_t fixed) {
    (*(terrain_visible_fn *)data_engine_display_iat_terrain_visible)(mesh, camera, viewport, a, b, c, fixed);
}
internal inline int patch_terrain_visible(terrain_visible_fn h) {
    return patch_import_call(data_engine_display_call_terrain_visible, data_engine_display_iat_terrain_visible, (void *)h);
}
internal inline float *viewport_rect(void *viewport) { return (float *)((uint8_t *)viewport + value_engine_display_of_viewport_rect); }
// its frustum's other numbers: the pixels' scale, the centre (x, y), the near and far depths
internal inline float viewport_scale(const void *viewport) { return *(const float *)((const uint8_t *)viewport + value_engine_display_of_viewport_scale); }
internal inline const float *viewport_centre(const void *viewport) { return (const float *)((const uint8_t *)viewport + value_engine_display_of_viewport_centre); }
internal inline float viewport_near(const void *viewport) { return *(const float *)((const uint8_t *)viewport + value_engine_display_of_viewport_near); }
internal inline float viewport_far(const void *viewport) { return *(const float *)((const uint8_t *)viewport + value_engine_display_of_viewport_far); }
internal inline float *viewport_near_at(void *viewport) { return (float *)((uint8_t *)viewport + value_engine_display_of_viewport_near); }
internal inline float *viewport_fov_at(void *viewport) { return (float *)((uint8_t *)viewport + value_engine_display_of_viewport_fov); }
internal inline float *viewport_far_at(void *viewport) { return (float *)((uint8_t *)viewport + value_engine_display_of_viewport_far); }
internal inline const float *viewport_camera(const void *viewport) { return (const float *)((const uint8_t *)viewport + value_engine_display_of_viewport_camera); }
// its side planes' normals (i: left, right, top, bottom; x, y the depth, z)
internal inline float *viewport_side_normal(void *viewport, int i) { return (float *)((uint8_t *)viewport + value_engine_display_of_viewport_side_normals + i * 0x18); }
internal inline const long *terrain_mesh_visible_size(const void *mesh) { return (const long *)((const uint8_t *)mesh + value_engine_display_of_mesh_visible_size); }
// the mesh's polygon pools (UpdateVisibleTerrain's, unchecked): a polygon's size, the mesh's own room in polygons
internal inline uint32_t terrain_polygon_size(void) { return value_engine_display_size_mesh_polygon; }
internal inline uint32_t terrain_ground_polygons_room(void) { return value_engine_display_mesh_ground_polygons; }
internal inline uint32_t terrain_water_polygons_room(void) { return value_engine_display_mesh_water_polygons; }
// UpdateVisibleTerrain's lea eax, [mesh + off] at `at` made a mov eax, to: 1 done, 0 other code
internal int terrain_pool_move(uint8_t *at, uint32_t off, void *to) {
    DWORD old;
    if (at[0] != 0x8D || (at[1] & 0xF8) != 0x80 || (at[1] & 7) == 4 || *(uint32_t *)(at + 2) != off
        || !VirtualProtect(at, 6, PAGE_EXECUTE_READWRITE, &old)) return 0;
    at[0] = 0xB8;
    *(uint32_t *)(at + 1) = (uint32_t)(uintptr_t)to;
    at[5] = 0x90;
    VirtualProtect(at, 6, old, &old);
    FlushInstructionCache(GetCurrentProcess(), at, 6);
    return 1;
}
// UpdateVisibleTerrain's polygons from the pools at ground and water instead of the mesh's own (there is one mesh, the
// game's): how many of the two moved
internal inline int terrain_pools_move(void *ground, void *water) {
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
    uint8_t *f = lle ? (uint8_t *)(void *)GetProcAddress(lle, "?UpdateVisibleTerrain@GETerrainMesh@@QAEXABVGETransformation@@AAVGEViewport@@KKK_N@Z") : NULL;
    if (!f) return 0;
    return terrain_pool_move(f + value_engine_display_of_visible_ground_pool, value_engine_display_of_mesh_ground_polygons, ground)
           + terrain_pool_move(f + value_engine_display_of_visible_water_pool, value_engine_display_of_mesh_water_polygons, water);
}
// the polygons the last update made: the ground's (water 0) or the water's (1), over all their textures' groups
internal inline unsigned long terrain_polygons(const void *mesh, int water) {
    const uint8_t *v = (const uint8_t *)mesh + (water ? value_engine_display_of_mesh_water_groups : value_engine_display_of_mesh_ground_groups);
    const uint8_t *g = *(uint8_t *const *)(v + 4), *end = *(uint8_t *const *)(v + 8);
    unsigned long n = 0;
    for (; g && g < end; g += value_engine_display_size_mesh_group)
        n += (unsigned long)(*(uint8_t *const *)(g + 0xC) - *(uint8_t *const *)(g + 8)) / 4;
    return n;
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
// the units' collection's list of what it drew: its i-th entry; the graphic objects' users' lock (a TSCriticalSection)
internal inline ee_pick_entry *pick_list_entry(ee_pick_list *l, uint32_t i) {
    return (ee_pick_entry *)((uint8_t *)pick_list_get_entries(l) + (size_t)i * size_ee_pick_entry);
}
internal inline void *graphics_lock(void) { return *(void **)engine_display_graphics_lock_at(); }
// EEGraphicObject.acquire's start -> h, *orig the game's (push ebp; mov ebp, esp; sub esp, 0x1B0 in both builds)
internal inline int patch_acquire(dmg_model_acquire_fn h, dmg_model_acquire_fn *orig) {
    static const uint8_t pro[9] = { 0x55, 0x8B, 0xEC, 0x81, 0xEC, 0xB0, 0x01, 0x00, 0x00 };
    void *o = NULL;
    if (!patch_entry((uintptr_t)fn_dmg_model_acquire, pro, 9, (void *)h, &o)) return 0;
    *orig = (dmg_model_acquire_fn)o;
    return 1;
}
