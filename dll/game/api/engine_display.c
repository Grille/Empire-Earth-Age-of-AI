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
// the file name of a model's k-th material's texture ("" none)
internal inline const char *model_texture_name(const ee_model *m, uint32_t k) {
    const char *s = ustring_get_text((const ee_ustring *)((const uint8_t *)model_get_texture_names(m) + (size_t)k * size_u_string));
    return s ? s : "";
}
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
// the list's entries an array of room of its own, made as its constructor makes them (0x50E45F: the game's new, the
// count before them - its destructor's delete[] -, each a GETransformation (transform_ctor), +0x40 0, +0x44 1.0, +0x48
// 0); only while it holds none. The old array is left (a reader may still have it). 1 done, 0 not empty or no memory
typedef void (__thiscall *transform_ctor_fn)(void *t);
internal inline int pick_list_grow(ee_pick_list *l, uint32_t room, transform_ctor_fn transform_ctor) {
    if (pick_list_get_count(l)) return 0;
    uint8_t *a = game_new(4 + (uintptr_t)room * size_ee_pick_entry);
    if (!a) return 0;
    *(uint32_t *)a = room;
    for (uint32_t i = 0; i < room; i++) {
        uint8_t *e = a + 4 + (size_t)i * size_ee_pick_entry;
        transform_ctor(e);
        *(uint32_t *)(e + 0x40) = 0;
        *(float *)(e + 0x44) = 1.0f;
        *(uint32_t *)(e + 0x48) = 0;
    }
    pick_list_set_entries(l, (ee_pick_entry *)(a + 4));
    pick_list_set_room(l, room);
    return 1;
}
// THE UNITS' OPACITY's inputs (collect.c's port of EEROUnits.opacity): the show-hidden mode, the float constants of
// its code (read from the exe, as its code reads them), the viewport its land buildings fade by (its camera's inverse,
// front and rear distances), GETransformation::Apply and GETerrainMesh::GetMeshZ through the exe's imports, the rotting
// state and the moved bit
typedef struct { float hidden, one, cloaked, zero, front, half, range; } opacity_consts;
internal inline int show_hidden_mode(void) { return *(const int *)data_engine_display_show_hidden; }
internal inline opacity_consts opacity_consts_now(void) {
    opacity_consts c = { *(const float *)data_engine_display_hidden_opacity, *(const float *)data_engine_display_float_one,
                         *(const float *)data_engine_display_cloaked_opacity, *(const float *)data_engine_display_float_zero,
                         *(const float *)data_engine_display_front_fade, *(const float *)data_engine_display_float_half,
                         *(const float *)data_engine_display_cloaked_range };
    return c;
}
internal inline void *opacity_viewport(void) { return (void *)(uintptr_t)data_engine_display_opacity_viewport; }
internal inline const void *opacity_view_inverse(void) {
    return (const uint8_t *)(uintptr_t)data_engine_display_opacity_viewport + value_engine_display_of_viewport_inverse;
}
typedef void (__thiscall *transform_apply_fn)(const void *t, const float *in, float *out);
typedef float (__thiscall *mesh_z_fn)(const void *mesh, float x, float y);
internal inline void transform_apply(const void *t, const float *in, float *out) {
    (*(transform_apply_fn *)data_engine_display_iat_transform_apply)(t, in, out);
}
internal inline float mesh_z(const void *mesh, float x, float y) { return (*(mesh_z_fn *)data_engine_display_iat_mesh_z)(mesh, x, y); }
internal inline int unit_rotting(const ee_unit *u) {
    ee_ai_state *s = unit_get_ai_state(u);
    return (s ? ai_state_get_id(s) : 0) == value_engine_display_state_rotting;
}
internal inline int unit_moved(const ee_unit *u) { return (unit_get_hidden_flags(u) & value_engine_display_hidden_moved) == value_engine_display_hidden_moved; }
// a player's stance toward another, by its index (EEPlayer.stance; the opacity dims a cloak to one it isn't 0 to - the
// AoC decompilation's mEnemies)
internal inline int32_t player_stance_to(const ee_player *p, int index) { return ((const int32_t *)player_stance_at(p))[index]; }

// THE UNITS' VIEW TEST's inputs (collect.c's port of EEROUnits.drawn): the camera's yaw, a tree's shift, the dead
// state, the path point's type, a unit with food; its three calls of GEPhysicalModel::Intersects as they stand now (the
// object's own, the remembered one's, the tree's: render_shadows.c hooks them); a physical model of its own on the
// stack, as the game's code makes one (GETransformation's constructor, the model 0, the scale 1, the frame 0) and its
// transformation - SetOrientationYPR, operator=, SetModel, the destructor through the exe's imports
internal inline float camera_yaw(void) { return *(const float *)data_engine_display_camera_yaw; }
internal inline float tree_shift(void) { return *(const float *)data_engine_display_tree_shift; }
internal inline int unit_dead(const ee_unit *u) {
    ee_ai_state *s = unit_get_ai_state(u);
    return (s ? ai_state_get_id(s) : 0) == value_engine_display_state_dead;
}
internal inline int type_is_path_point(const ee_unit_type *t) { return (uint32_t)unit_type_get_id(t) == value_engine_display_type_path_point; }
internal inline int unit_holds_food(const ee_unit *u) {
    return (uint32_t)unit_get_resource(u) == value_engine_display_resource_food && unit_get_amount(u) > 0;
}
internal inline model_intersects_fn units_view_call(int i) {
    return (model_intersects_fn)call_site_target(data_engine_display_calls_units_view[i]);
}
typedef void (__thiscall *pm_this_fn)(void *o);
typedef void (__thiscall *pm_orient_fn)(void *t, float yaw, float pitch, float roll);
typedef void *(__thiscall *pm_assign_fn)(void *t, const void *from);
typedef void (__thiscall *pm_set_model_fn)(void *pm, void *model);
internal inline uint32_t physical_model_size(void) { return value_engine_display_size_physical_model; }
internal inline void physical_model_new(void *pm) {
    (*(pm_this_fn *)data_engine_display_iat_transform_new)(pm);
    *(void **)((uint8_t *)pm + value_engine_display_of_physical_model_model) = NULL;
    *(float *)((uint8_t *)pm + value_engine_display_of_physical_model_scale) = 1.0f;
    *(uint32_t *)((uint8_t *)pm + value_engine_display_of_physical_model_frame) = 0;
}
internal inline void physical_model_free(void *pm) { (*(pm_this_fn *)data_engine_display_iat_model_free)(pm); }
internal inline void physical_model_set_model(void *pm, void *model) { (*(pm_set_model_fn *)data_engine_display_iat_set_model)(pm, model); }
internal inline void physical_model_set_frame(void *pm, uint32_t frame) { *(uint32_t *)((uint8_t *)pm + value_engine_display_of_physical_model_frame) = frame; }
internal inline void physical_model_set_scale(void *pm, float s) { *(float *)((uint8_t *)pm + value_engine_display_of_physical_model_scale) = s; }
internal inline void transform_orient(void *t, float yaw, float pitch, float roll) { (*(pm_orient_fn *)data_engine_display_iat_set_orientation)(t, yaw, pitch, roll); }
internal inline void transform_assign(void *t, const void *from) { (*(pm_assign_fn *)data_engine_display_iat_transform_assign)(t, from); }
// GEModel::PrepareTransforms through the exe's import slot (every call of the exe's): the original in *orig
typedef void (__thiscall *model_prepare_fn)(ee_model *m);
internal inline int patch_prepare_transforms(model_prepare_fn h, model_prepare_fn *orig) {
    return iat_hook(data_engine_display_iat_prepare_transforms, "Low-Level Engine.dll", "?PrepareTransforms@GEModel@@QAEXXZ", (void *)h, (void **)orig);
}
// GETransformation::Apply (this one x other into out: 4 x 4 floats each), the engine's export; NULL before it is loaded
typedef void (__thiscall *transform_mul_fn)(const float *t, const float *other, float *out);
internal inline transform_mul_fn transform_mul_export(void) {
    HMODULE m = GetModuleHandleA("Low-Level Engine.dll");
    return m ? (transform_mul_fn)(void *)GetProcAddress(m, "?Apply@GETransformation@@QBEXABV1@AAV1@@Z") : NULL;
}

// THE EFFECTS' QUEUES (collect.c: made by the simulation at a step's end, inside the world's lock): the game's five
// maps; the map's table of effects by region; an effect's calls and its reference (at +4, plain: inside the lock only)
#define EFFECT_QUEUES_SIZE 60
internal inline uint8_t *effect_queues_at(void) { return (uint8_t *)data_engine_display_effect_queues; }
// the light the units are drawn with, as the units' draw hands it to the device: its direction (the sun's or the moon's),
// ambient colour x intensity, the light's colour x intensity, both faded with the picture; 0 when the units are drawn
// without one
// the mesh's water colour set: its index among the colour sets, and the scale from a height to 0 .. 1 over its slots
internal inline unsigned mesh_water_set(void *mesh) { return FIELD(mesh, value_engine_display_of_mesh_water_set, unsigned); }
internal inline float mesh_water_scale(void *mesh) { return FIELD(mesh, value_engine_display_of_mesh_water_scale, float); }
// the water's groups, one per frame of its animation; a tile's group; a group's texture now (UpdateWaterAnimation moves them along)
internal inline unsigned mesh_water_groups(const void *mesh) {
    const uint8_t *v = (const uint8_t *)mesh + value_engine_display_of_mesh_water_groups;
    const uint8_t *g = *(uint8_t *const *)(v + 4);
    return g ? (unsigned)(*(uint8_t *const *)(v + 8) - g) / value_engine_display_size_mesh_group : 0;
}
internal inline unsigned mesh_water_patch(const void *mesh, unsigned x, unsigned y) { return ((const unsigned *)((const uint8_t *)mesh + value_engine_display_of_mesh_water_patches))[(x % 33) * 33 + y % 33]; }
// the water's animation: the frame its first group shows (group j: that + j, round the groups), the ms a frame (UpdateWaterAnimation)
internal inline unsigned mesh_water_frame(void *mesh) { return FIELD(mesh, value_engine_display_of_mesh_water_frame, unsigned); }
internal inline unsigned mesh_water_frame_ms(void *mesh) { return FIELD(mesh, value_engine_display_of_mesh_water_frame_ms, unsigned); }
internal inline void *mesh_water_texture(const void *mesh, unsigned group) {
    return *(void **)(*(uint8_t *const *)((const uint8_t *)mesh + value_engine_display_of_mesh_water_groups + 4) + group * value_engine_display_size_mesh_group);
}
internal inline int game_light(float dir[3], float amb[3], float diff[3]) {
    if (!*(const uint8_t *)data_engine_display_light_on) return 0;
    const float *d = (const float *)data_engine_display_light_direction, *a = (const float *)data_engine_display_light_ambient,
                *c = (const float *)data_engine_display_light_color;
    float fade = *(const float *)data_engine_display_light_fade, ai = *(const float *)data_engine_display_light_ambient_intensity * fade,
          li = *(const float *)data_engine_display_light_intensity * fade;
    for (int i = 0; i < 3; i++) dir[i] = d[i], amb[i] = a[i] * ai, diff[i] = c[i] * li;
    return 1;
}
internal inline const uint8_t *map_effect_regions(const ee_map *m) { return *(const uint8_t *const *)((const uint8_t *)m + value_engine_display_of_map_effect_regions); }
internal inline int effect_regions_size(const uint8_t *t) { return *(const int *)(t + 8); }
internal inline uint32_t effect_regions_width(const uint8_t *t) { return *(const uint32_t *)(t + 0x10); }
internal inline uint32_t effect_regions_height(const uint8_t *t) { return *(const uint32_t *)(t + 0x14); }
// a region's effects (none out of the table): their first, *n of them
internal inline void **effect_region(const uint8_t *t, uint32_t x, uint32_t y, uint32_t *n) {
    *n = 0;
    if (x >= effect_regions_width(t) || y >= effect_regions_height(t)) return NULL;
    const uint8_t *r = *(const uint8_t *const *)(t + 4) + (size_t)(x * effect_regions_height(t) + y) * 0x10;
    void **first = *(void **const *)(r + 4), **end = *(void **const *)(r + 8);
    if (first) *n = (uint32_t)(end - first);
    return first;
}
internal inline int effect_shown(const void *e) {
    uint8_t l = ((const uint8_t *)e)[value_engine_display_of_effect_local_only];
    return !l || (l == 1 && *(const int *)((const uint8_t *)e + value_engine_display_of_effect_player) == (int)local_player_index());
}
internal inline void effect_prepare(void *e) { ((void (__thiscall *)(void *))(*(void ***)e)[value_engine_display_slot_effect_prepare])(e); }
internal inline int effect_queue(void *e, void *queues) { return ((int (__thiscall *)(void *, void *))(*(void ***)e)[value_engine_display_slot_effect_queue])(e, queues); }
internal inline int effect_is_marching_line(const void *e) { return *(const uint32_t *)e == vt_eee3_d_marching_line; }
internal inline void effect_draw(void *e) { ((int (__thiscall *)(void *))(*(void ***)e)[value_engine_display_slot_effect_draw])(e); }
internal inline void effect_reference(void *e) { ++*(uint32_t *)((uint8_t *)e + 4); }
internal inline uint32_t effect_references(const void *e) { return *(const uint32_t *)((const uint8_t *)e + 4); }
// a queue's entries in their order (the tree's nodes: left +0, parent +4, right +8, the key +0xC, the deque +0x10, its
// "is the head" byte +0x25; the deque: blocks +4, their count +8, front +0xC, size +0x10, 0x400 entries a block). Each
// entry's call; 0 ends it
typedef int (*effect_entry_fn)(void *ctx, int queue, uint32_t key, void *effect);
internal inline int effect_queues_each(uint8_t *queues, effect_entry_fn f, void *ctx) {
    for (int q = 0; q < 5; q++) {
        uint8_t *head = *(uint8_t **)(queues + q * 12 + 4), *n = *(uint8_t **)head;
        while (n != head) {
            uint8_t *d = n + 0x10;
            uint32_t front = *(uint32_t *)(d + 0xC), size = *(uint32_t *)(d + 0x10), blocks = *(uint32_t *)(d + 8);
            void ***map = *(void ****)(d + 4);
            for (uint32_t k = 0; k < size; k++) {
                uint32_t at = front + k, b = at >> 10;
                if (b >= blocks) b -= blocks;
                if (!f(ctx, q, *(uint32_t *)(n + 0xC), map[b][at & 0x3FF])) return 0;
            }
            uint8_t *r = *(uint8_t **)(n + 8);
            if (!r[0x25]) {
                while (!(*(uint8_t **)r)[0x25]) r = *(uint8_t **)r;
                n = r;
            } else {
                uint8_t *p = *(uint8_t **)(n + 4);
                while (!p[0x25] && n == *(uint8_t **)(p + 8)) n = p, p = *(uint8_t **)(p + 4);
                n = p;
            }
        }
    }
    return 1;
}

// THE UNITS' FILL's inputs (collect.c's port of FillUnit): its float constants, the mesh's rows (is a tile on it), a
// tile's light byte as the build tests it (EE: not 0; AoC: its low 7 bits), the imports UWideString::operator=,
// Apply of a vector, UMArcCos (its answer the FPU's, unrounded: an x87 long double in st0) and Scale, a gate type cast
// to the gate's class, a graphics object acquired as the build does it (AoC through its vtable, slot 1), a player's
// stats of a type, the sort keys
typedef struct { float tint_alpha, bar_full, marker_step, tilt_offset, marker_min_z, sink_302, sink_31f; } fill_consts;
internal inline fill_consts fill_consts_now(void) {
    fill_consts c = { *(const float *)data_engine_display_tint_alpha, *(const float *)data_engine_display_bar_full,
                      *(const float *)data_engine_display_marker_color_step, *(const float *)data_engine_display_tilt_offset,
                      *(const float *)data_engine_display_marker_min_z,
                      build_is("aoc") ? *(const float *)data_engine_display_sink_302 : 0.0f,
                      build_is("aoc") ? *(const float *)data_engine_display_sink_31f : 0.0f };
    return c;
}
internal inline uint32_t producing_none(void) { return value_engine_display_producing_none; }
internal inline int mesh_has_tile(const void *mesh, int x, int y) {
    const uint8_t *m = (const uint8_t *)mesh;
    if ((uint32_t)y >= *(const uint32_t *)(m + value_engine_display_of_mesh_rows_n)) return 0;
    const int32_t *row = (const int32_t *)(*(const uint8_t *const *)(m + value_engine_display_of_mesh_rows) + (size_t)(uint32_t)y * 0xC);
    return (uint32_t)(x - row[0]) < (uint32_t)row[1];
}
internal inline int map_tile_lit(const ee_map *map, int x, int y) {
    const uint8_t *row = *(const uint8_t *const *)((const uint8_t *)map + value_engine_display_of_map_light_rows) + (size_t)(uint32_t)y * 0xC;
    uint8_t b = (*(const uint8_t *const *)(row + 8))[(size_t)(x - *(const int32_t *)row) * 4 + 3];
    return build_is("aoc") ? (b & 0x7F) != 0 : b != 0;
}
typedef void (__thiscall *wstring_assign_fn)(void *to, const void *from);
typedef void (__thiscall *transform_apply_vector_fn)(const void *t, const float *v, float *out);
typedef long double (__cdecl *arc_cos_fn)(float x);
typedef void (__thiscall *transform_scale_fn)(void *t, float x, float y, float z);
internal inline void wstring_assign(void *to, const void *from) { (*(wstring_assign_fn *)data_engine_display_iat_wstring_assign)(to, from); }
internal inline void transform_apply_vector(const void *t, const float *v, float *out) {
    (*(transform_apply_vector_fn *)data_engine_display_iat_transform_apply_vector)(t, v, out);
}
internal inline long double arc_cos(float x) { return (*(arc_cos_fn *)data_engine_display_iat_arc_cos)(x); }
internal inline void transform_scale(void *t, float x, float y, float z) { (*(transform_scale_fn *)data_engine_display_iat_transform_scale)(t, x, y, z); }
internal inline void *gate_class_of(ee_unit_type *t) {
    return rt_dynamic_cast(t, 0, (void *)(uintptr_t)data_engine_display_rtti_unit_class, (void *)(uintptr_t)data_engine_display_rtti_gate_class, 0);
}
internal inline void graphics_acquire(ee_dmg_model *g) {
    if (build_is("aoc")) ((int (__thiscall *)(ee_dmg_model *))(*(void ***)g)[1])(g);
    else dmg_model_acquire(g);
}
internal inline ee_unit_stats *player_stats_of(const ee_player *p, int type_id) {
    return (*(ee_unit_stats ***)(player_type_stats_at(p) + 4))[type_id];
}
internal inline uint32_t graphics_anim_key(const ee_dmg_model *g, int anim) { return ((const uint32_t *)dmg_model_anim_keys_at(g))[anim]; }
// the fill's lists: a vector's first element (MSVC: +4), its count (EEGraphicObject's +4 vector: player.vector_size)
internal inline void *vector_first(const void *v) { return *(void *const *)((const uint8_t *)v + 4); }
// a unit by its id as the world's lookup answers (the fill's: no check of its player first, unlike unit_by_id); whether
// a group holds a unit (its vf3)
internal inline ee_unit *world_unit(uint32_t id) { return world_unit_by_id((ee_world *)(uintptr_t)data_game_game_state, id); }
internal inline int group_contains(void *grp, ee_unit *u) {
    return ((uint8_t (__thiscall *)(void *, ee_unit *))(*(void ***)grp)[value_engine_display_slot_group_contains])(grp, u) != 0;
}

// THE TREES' FILL's inputs (collect.c's port of FillObject): the scale's and the sway's steps, the sway's clock, UMCos
// (its answer the FPU's, unrounded), GETransformation::SetPosition, whether EE hands a tree to the ambient sounds
typedef struct { float scale_step, sway_step; uint32_t clock; } tree_consts;
internal inline tree_consts tree_consts_now(void) {
    tree_consts c = { *(const float *)data_engine_display_tree_scale_step, *(const float *)data_engine_display_sway_step,
                      *(const uint32_t *)data_engine_display_sway_clock };
    return c;
}
typedef long double (__cdecl *cos_fn)(float x);
typedef void (__thiscall *set_position_fn)(void *t, float x, float y, float z);
internal inline long double game_cos_fpu(float x) { return (*(cos_fn *)data_game_iat_cos)(x); }
internal inline void transform_set_position(void *t, float x, float y, float z) { (*(set_position_fn *)data_engine_display_iat_set_position)(t, x, y, z); }
// the doppelgangers' fills' (collect.c uc_fill_dopp): GETransformation::MakeIdentity
typedef void (__thiscall *make_identity_fn)(void *t);
internal inline void transform_identity(void *t) { (*(make_identity_fn *)data_engine_display_iat_make_identity)(t); }

// the ghosts' fill's (collect.c uc_fill_ghosts): the placement ghosts, their lock, whether they changed, their records'
// vector, a record's size and parts (after its ee_draw_record: the footprint's heights and cells, the tile), the lift
internal inline void *ghost_units(void) { return (void *)(uintptr_t)data_engine_display_ghost_units; }
internal inline void *ghost_lock(void) { return *(void *const *)data_engine_display_ghost_lock; }
internal inline int ghosts_changed(void) { return *(const uint8_t *)data_engine_display_ghosts_changed != 0; }
internal inline void ghosts_enter(void) {
    static ts_cs_fn enter;
    if (!enter) enter = ts_cs_import("?Enter@TSCriticalSection@@QAEXXZ");
    if (enter) enter(ghost_lock());
}
internal inline void ghosts_leave(void) {
    static ts_cs_fn leave;
    if (!leave) leave = ts_cs_import("?Leave@TSCriticalSection@@QAEXXZ");
    if (leave) leave(ghost_lock());
}
// a ghost as Create makes one, on the end of the vector; NULL: no memory (the vector's as it was)
internal inline ee_ghost_unit *ghost_unit_add(int type, ee_player *pl) {
    ee_ghost_unit *g = game_new(size_ee_ghost_unit);
    if (!g) return NULL;
    ghost_unit_init(g, type, pl, 0);
    world_push_ghost(game_state(), g);
    return g;
}
// the vector's last ghost deleted, as the world's clear (0x52E1AD) does
internal inline void ghost_units_drop_last(void) {
    ee_ghost_unit ***end = (ee_ghost_unit ***)((uint8_t *)ghost_units() + 8);
    ee_ghost_unit *g = *--*end;
    ((void (__thiscall *)(void *, int))(*(void ***)g)[0])(g, 1);
}
// the vector's end moved to its first n, the ones behind it kept: for a call inside the ghosts' lock, put back after it
internal inline void ghost_units_set_end(uint32_t n) {
    uint8_t *v = (uint8_t *)ghost_units();
    *(void ***)(v + 8) = *(void ***)(v + 4) + n;
}
internal inline void *ghost_records(void) { return (void *)(uintptr_t)data_engine_display_ghost_records; }
internal inline float ghost_lift(void) { return *(const float *)data_engine_display_ghost_lift; }
internal inline uint32_t ghost_record_size(void) { return value_engine_display_size_ghost_record; }
internal inline uint8_t *ghost_record_heights(uint8_t *r) { return r + value_engine_display_of_ghost_record_heights; }
internal inline uint8_t *ghost_record_cells(uint8_t *r) { return r + value_engine_display_of_ghost_record_cells; }
internal inline int32_t *ghost_record_tile(uint8_t *r) { return (int32_t *)(r + value_engine_display_of_ghost_record_tile); }
internal inline int tree_has_ambient(const ee_unit_type *t) {
    return build_is("aoc") || (uint32_t)unit_type_get_resource_type(t) == value_engine_display_resource_wood;
}

// EEGraphicObject.acquire's start -> h, *orig the game's (push ebp; mov ebp, esp; sub esp, 0x1B0 in both builds)
internal inline int patch_acquire(dmg_model_acquire_fn h, dmg_model_acquire_fn *orig) {
    static const uint8_t pro[9] = { 0x55, 0x8B, 0xEC, 0x81, 0xEC, 0xB0, 0x01, 0x00, 0x00 };
    void *o = NULL;
    if (!patch_entry((uintptr_t)fn_dmg_model_acquire, pro, 9, (void *)h, &o)) return 0;
    *orig = (dmg_model_acquire_fn)o;
    return 1;
}
