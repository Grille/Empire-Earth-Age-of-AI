// THE FRAME, RECORDED (docs/renderer.md "The frame"): the batches are not drawn as the game sends them - each one goes
// into the frame's list with the states it is drawn with (targets, viewport, blend, depth, raster, texture, sampler, its
// vertex and pixel constants as a snapshot shared by the batches in a row that have the same) and its place in the
// upload buffers, and the list is drawn at once (rd3_submit): at the scene's end, or where something else needs the
// context as the game left it - a texture uploaded that a recorded batch draws with, a surface freed, the target read
// back or written by a lock, the 2D path, a Clear, the overlay, the present. What the game draws in its order the list
// keeps in it; the mod's own passes go in at places of their own with the whole frame known: the dynamic shadows' maps
// from this frame's casters before anything is drawn, the terrain receiving them in its own pixel shader
// (render_shadows.c) - not guessed from the order the game's draws come in (the darkening at the units' first draw,
// a depth of its own, the panels' clears copied: what came before, 2026-11-04).
// The upload buffers are a ring written without overwrite: a recorded batch's vertices stay where they are until the
// list is drawn - the ring starts over (discard) only with the list empty: at a scene's end once less than a quarter is
// left (a frame of the busiest base is below a tenth), within one only when it is full (the list drawn first).
typedef struct {
    ID3D11RenderTargetView *rtv;
    ID3D11DepthStencilView *dsv;
    void *blend, *depth, *raster, *samp;
    ID3D11ShaderResourceView *srv;
    D3D11_VIEWPORT vport;
    D3D11_PRIMITIVE_TOPOLOGY topo;
    int vc, pc;      // the constants' snapshots (g_rf.vcs, pcs)
    int role, ro;    // the shadows' role (render_shadows.c SH_), the render object drawing it
    UINT at, n;      // the indices in the upload buffer
    INT base;        // the vertices' (in vertices)
} rf_cmd;
internal struct {
    rf_cmd *cmd;
    rd3_vcb *vcs;
    rd3_pcb *pcs;
    int n, cap, nvc, cap_vc, npc, cap_pc;
    int vc_new, pc_new;     // g_rd3.vc / pc changed since the last snapshot
    LONG replays, forced;   // lists drawn; of them inside a scene (not at its end) - the minute's line
    LONG wraps;             // the ring full within a scene
    int in_submit;
} g_rf;

internal int rf_grow(void **p, int *cap, int need, size_t size) {
    if (need <= *cap) return 1;
    int c = *cap ? *cap * 2 : 1024;
    while (c < need) c *= 2;
    void *q = realloc(*p, (size_t)c * size);
    if (!q) return 0;
    *p = q;
    *cap = c;
    return 1;
}
// the batch just uploaded into the list, with the states it was collected under (rd3_flush)
internal void rf_record(UINT at, UINT n, INT base) {
    if (g_rf.vc_new || !g_rf.nvc) {
        if (!rf_grow((void **)&g_rf.vcs, &g_rf.cap_vc, g_rf.nvc + 1, sizeof *g_rf.vcs)) return;
        g_rf.vcs[g_rf.nvc++] = g_rd3.vc;
        g_rf.vc_new = 0;
    }
    if (g_rf.pc_new || !g_rf.npc) {
        if (!rf_grow((void **)&g_rf.pcs, &g_rf.cap_pc, g_rf.npc + 1, sizeof *g_rf.pcs)) return;
        g_rf.pcs[g_rf.npc++] = g_rd3.pc;
        g_rf.pc_new = 0;
    }
    if (!rf_grow((void **)&g_rf.cmd, &g_rf.cap, g_rf.n + 1, sizeof *g_rf.cmd)) return;
    rf_cmd *c = &g_rf.cmd[g_rf.n++];
    c->rtv = g_rd3.rtv; c->dsv = g_rd3.dsv;
    c->blend = g_rd3.blend; c->depth = g_rd3.depth; c->raster = g_rd3.raster; c->samp = g_rd3.samp;
    c->srv = g_rd3.srv;
    c->vport = g_rd3.vport;
    c->topo = g_rb.topo;
    c->vc = g_rf.nvc - 1; c->pc = g_rf.npc - 1;
    c->role = (int)g_rd3.vc.shp[0];
    c->ro = g_ro_cur;
    c->at = at; c->n = n; c->base = base;
}

// THE GROUND KEPT (render_3d.c "the frame during a step"): a frame drawn while the simulation runs a step skips the
// render objects' update, and the ground's draw draws nothing without it (the ground's patches are made in the update,
// with the units' visible region - that update can't run alone). So each frame's ground batches are kept on the CPU as
// they are flushed - vertices, indices, world matrices, states, constants - and a frame during a step draws the last
// frame's again in the ground's place. The camera moved since (scrolling goes on, on this thread: camera.c) is the
// list's to show (rf_shift, below) - the vertices stay as they were placed, the shadows' maps with them.
typedef struct { rf_cmd c; rd3_vcb vc; rd3_pcb pc; int v0, nv, i0, ni, w0, nw; } rf_kept;
typedef struct {
    rb_vertex *v;
    uint32_t *i;
    float (*w)[16];
    rf_kept *b;
    int nv, ni, nw, nb, cap_v, cap_i, cap_w, cap_b;
} rf_ground;
internal int rf_camera_view(float *v, const float *m);
internal rf_ground g_kg;   // the last frame drawn with an update (a frame during a step records none)
internal rf_ground g_kw;   // its water (the same: rf_water_again)
internal rf_ground g_ks;   // the game's shadows' decals, the farms' fields: the same (their draw, as the ground's, draws
                           // nothing during a step - every frame drawn during one showed the farms without their fields,
                           // the player 2026-11-06: "farms flicker in visibility", the pipe's record: fields 0 in those)
internal int g_kg_skip;   // this frame is drawn during a step: nothing recorded
internal LONG g_kg_drawn; // ground batches drawn again (the pipe's picture)
internal float g_kg_eye[3];   // the camera's eye at the last update (the pipe's picture camtrace)
internal float g_kg_view[16];    // the camera's view at the last update: the ground's camera space is made from it
internal int g_kg_view_ok;
internal void rf_ground_clear(rf_ground *g) {
    for (int k = 0; k < g->nb; k++) D3D_RELEASE(g->b[k].c.srv);
    g->nv = g->ni = g->nw = g->nb = 0;
}
// the batch being flushed (rd3_flush, before its matrices' place is added), when it is the ground's
internal void rf_keep_ground(void) {
    if (g_kg_skip || (g_ro_cur != RO_TERRAIN && g_ro_cur != RO_WATER && g_ro_cur != RO_SHADOWS)) return;
    rf_ground *g = g_ro_cur == RO_WATER ? &g_kw : g_ro_cur == RO_SHADOWS ? &g_ks : &g_kg;
    if (!rf_grow((void **)&g->b, &g->cap_b, g->nb + 1, sizeof *g->b) || !rf_grow((void **)&g->v, &g->cap_v, g->nv + g_rb.nv, sizeof *g->v)
        || !rf_grow((void **)&g->i, &g->cap_i, g->ni + g_rb.ni, sizeof *g->i) || !rf_grow((void **)&g->w, &g->cap_w, g->nw + g_rb.nw, sizeof *g->w))
        return;
    rf_kept *b = &g->b[g->nb++];
    memset(&b->c, 0, sizeof b->c);
    b->c.rtv = g_rd3.rtv; b->c.dsv = g_rd3.dsv;
    b->c.blend = g_rd3.blend; b->c.depth = g_rd3.depth; b->c.raster = g_rd3.raster; b->c.samp = g_rd3.samp;
    b->c.srv = g_rd3.srv;
    if (b->c.srv) ID3D11ShaderResourceView_AddRef(b->c.srv);
    b->c.vport = g_rd3.vport;
    b->c.topo = g_rb.topo;
    b->vc = g_rd3.vc; b->pc = g_rd3.pc;
    b->v0 = g->nv; b->nv = g_rb.nv; b->i0 = g->ni; b->ni = g_rb.ni; b->w0 = g->nw; b->nw = g_rb.nw;
    memcpy(g->v + g->nv, g_rb.v, (size_t)g_rb.nv * sizeof *g->v);
    memcpy(g->i + g->ni, g_rb.i, (size_t)g_rb.ni * sizeof *g->i);
    memcpy(g->w + g->nw, g_rb.w, (size_t)g_rb.nw * sizeof *g->w);
    g->nv += g_rb.nv; g->ni += g_rb.ni; g->nw += g_rb.nw;
}
// the render objects' update: a frame drawn as before (skip 0) - its ground recorded anew - or during a step (skip 1:
// the last one's drawn again - the units' view is that frame's update's; one frame older was one frame's camera move
// off - the ground and the debris on it jumped against each other while scrolling, the player 2026-11-05)
internal void rf_camera_frame(void);
// The ground's margin ([display] ground_margin; the pipe: picture margin): the kept ground is the last update's visible
// region, the screen's - a frame during a long step that had scrolled or zoomed out past it showed the sky (the
// player's 8000-unit save: half the screen before the next update). The region is made by the ground's update through
// the viewport's rectangle (UpdateVisibleTerrain: the tiles under the frustum's eight corners, the far plane's with
// them - 58 x 57 tiles on that save at the screen's own): the rectangle widened about its centre by 1 + the margin for
// that call, the region grows with it - the ground drawn beyond the screen, the units' collection walking its rows
// (their models still tested against the true view). The mesh's arrays hold 101 x 101 tiles and nothing checks it:
// the region's size is worked out first as GetFrustrumWorldPoints and GetTileRowsInVolume would make it (the corners
// by the engine's own DenormalizePerspectivePoint, their box on the map) and the factor taken back until it is at most
// GM_TILES - estimated from the last region instead, a zoom out between two updates went to 95 tiles.
// Toward the camera (the player: "the ground should extend 1 or two tiles more under the camera ... zooming out or
// moving down still exposes void"). GetTileRowsInVolume takes the hull of the eight corners, and toward the camera it
// narrows to the near ones - at the viewport's near depth, which the game sets about at the region's nearest ground
// (19.3 at zoom 1 on that save, 0.1 at zoom 0.5): the region ended at the screen's bottom edge, a point under the
// camera. The far corners (depth 58) reach far forward and to the sides by themselves. So the near depth for the
// call is the viewport's / (1 + the margin) less GM_BEHIND times the margin - below 0 the corners lie behind the eye,
// mirrored (GetFrustrumWorldPoints: the rectangle's point times the depth through the camera's inverse): -8.6 at zoom
// 1, -12 at zoom 0.5, the hull's near end wide behind the camera's foot; the bottom of the rectangle 1 + twice the
// margin; and at the cap the top gives way first (g_gm_kt), then the sides (g_gm_k), then the near end, the bottom
// last.
#define GM_TILES 88
#define GM_CLIP 4.0f    // the clip codes' planes (rf_to_camera, below)
#define GM_BEHIND 12.0f // the region's near depth: the viewport's / (1 + the margin) less this times the margin
internal float g_gm_margin = 1.0f;
internal float g_gm_k;          // the factor of the last region: its sides
internal float g_gm_kt;         // its top
internal float g_gm_kb;         // its bottom
internal float g_gm_near;       // its near depth (0: the viewport's)
internal float g_gm_depths[2];  // the viewport's near and far depths then
internal long g_gm_size[2];     // its size, tiles less one (the mesh's)
internal long g_gm_pred;        // and as worked out before (the larger side)
typedef void (__thiscall *gm_denorm_fn)(const void *viewport, const float *p, float depth, float *out);
internal gm_denorm_fn g_gm_denorm;
// The region's polygons: UpdateVisibleTerrain takes them from two pools in the mesh and checks neither - the ground's
// 12000 (a tile's two, two more for each of up to four neighbours' textures blended over it, four for a map edge's
// skirt) and the water's 5000 (a water tile's two), each followed by its vertex grid. Twice the screen's region over
// the sea ran the water's into its grid, the grid's vertices then written over the polygons (the player's 8000-unit
// save, 2026-11-05: TransformToCamera reading a water polygon's vertex at 4). The pools moved to the mod's, room for
// the whole 101 x 101 grid at the most each tile can have (address space: the pages taken as they are written); no
// margin without them. The polygons the last update made, the most since the minute's line: ground, water.
#define GM_GRID 101
#define GM_GROUND_POLYS (GM_GRID * GM_GRID * 10 + 4 * GM_GRID * 4 + 1024)
#define GM_WATER_POLYS (GM_GRID * GM_GRID * 2)
internal int g_gm_pools;        // the two moved (2), else the mesh's own
internal unsigned long g_gm_polys[2], g_gm_polys_max[2];
internal int rf_ground_pools_install(void) {
    size_t g = (size_t)GM_GROUND_POLYS * terrain_polygon_size(), w = (size_t)GM_WATER_POLYS * terrain_polygon_size();
    uint8_t *p = VirtualAlloc(NULL, g + w, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (!p) return 0;
    g_gm_pools = terrain_pools_move(p, p + g);
    return g_gm_pools;
}
// the larger side of the region the frustum through rectangle r would give, tiles less one (-1: not known)
internal long rf_region_size(const void *viewport, const float *r) {
    ee_map *map = game_map();
    float s = viewport_scale(viewport);
    if (!g_gm_denorm || !map || !(s != 0)) return -1;
    const float *c = viewport_centre(viewport);
    float x0 = r[1], x1 = r[2], y0 = r[3], y1 = r[0];
    if (x1 - x0 <= 2) x1 = x0 + 2;   // as GetFrustrumWorldPoints
    if (y1 - y0 <= 2) y1 = y0 + 2;
    float lo[2] = { 1e30f, 1e30f }, hi[2] = { -1e30f, -1e30f };
    for (int i = 0; i < 8; i++) {
        float p[3] = { ((i & 1 ? x1 : x0) - c[0]) / s, -((i & 2 ? y1 : y0) - c[1]) / s, (float)(i >> 2) }, w[3];
        g_gm_denorm(viewport, p, i & 4 ? viewport_far(viewport) : viewport_near(viewport), w);
        for (int a = 0; a < 2; a++) {
            if (!(w[a] > -1e6f && w[a] < 1e6f)) return -1;
            if (w[a] < lo[a]) lo[a] = w[a];
            if (w[a] > hi[a]) hi[a] = w[a];
        }
    }
    float top = (float)map_get_size(map) - 1;
    long big = 0;
    for (int a = 0; a < 2; a++) {
        float l = lo[a] < 0 ? 0 : lo[a] > top ? top : lo[a], h = hi[a] < 0 ? 0 : hi[a] > top ? top : hi[a];
        long n = (long)floorf(h) - (long)floorf(l);
        if (n > big) big = n;
    }
    return big;
}
// the rectangle r (bottom, left, right, top) widened about was' centre: its sides by k, its bottom by kb, its top by kt
internal void rf_scale_rect(float *r, const float *was, float k, float kb, float kt) {
    float cx = (was[1] + was[2]) * 0.5f, cy = (was[0] + was[3]) * 0.5f;
    r[1] = cx + (was[1] - cx) * k; r[2] = cx + (was[2] - cx) * k;
    r[0] = cy + (was[0] - cy) * kb; r[3] = cy + (was[3] - cy) * kt;
}
internal void __thiscall rf_hook_terrain_visible(void *mesh, const void *camera, void *viewport, unsigned long a, unsigned long b,
                                                 unsigned long c, uint8_t fixed) {
    if (!g_gm_denorm) {
        HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
        if (lle) g_gm_denorm = (gm_denorm_fn)(void *)GetProcAddress(lle, "?DenormalizePerspectivePoint@GEViewport@@QBEXABVGE3DPoint@@MAAV2@@Z");
    }
    float *r = viewport_rect(viewport), was[4];
    memcpy(was, r, sizeof was);
    // the fixed camera's frustum is the orthographic one: as it was; without the pools the mesh's own would overflow
    int on = !fixed && g_gm_pools >= 2;
    float k = on ? 1.0f + g_gm_margin : 1.0f, kt = k, kb = on ? 1.0f + 2.0f * g_gm_margin : 1.0f;
    // the near depth (above): the corners there behind the eye with a margin, mirrored - GetFrustrumWorldPoints takes
    // the rectangle's points times the depth through the camera's inverse
    float *nd = viewport_near_at(viewport), near0 = *nd, behind = on ? GM_BEHIND * g_gm_margin : 0;
    long n = -1;
    for (int tries = 0; kb > 1.0f; tries++) {
        *nd = near0 / k - behind;
        rf_scale_rect(r, was, k, kb, kt);
        n = rf_region_size(viewport, r);
        if (n >= 0 && n <= GM_TILES) break;
        if (n < 0 || tries == 24) k = kt = kb = 1.0f;   // the top first, the sides, behind, the bottom last (above)
        else if (kt > 1.02f) kt = 1.0f + (kt - 1.0f) * 0.7f;
        else if (k > 1.02f) { kt = 1.0f; k = 1.0f + (k - 1.0f) * 0.7f; }
        else if (behind > 0.3f) { kt = k = 1.0f; behind *= 0.7f; }
        else { kt = k = 1.0f; behind = 0; kb = 1.0f + (kb - 1.0f) * 0.7f; }
    }
    if (kb <= 1.0f) { k = kt = kb = 1.0f; behind = 0; *nd = near0; memcpy(r, was, sizeof was); n = rf_region_size(viewport, r); }
    terrain_visible(mesh, camera, viewport, a, b, c, fixed);
    memcpy(r, was, sizeof was);
    g_gm_near = kb > 1.0f ? *nd : 0;
    *nd = near0;
    const long *sz = terrain_mesh_visible_size(mesh);
    g_gm_size[0] = sz[0]; g_gm_size[1] = sz[1];
    g_gm_k = k;
    g_gm_kt = kt;
    g_gm_kb = kb;
    g_gm_depths[0] = near0; g_gm_depths[1] = viewport_far(viewport);
    g_gm_pred = n;
    for (int w = 0; w < 2; w++) {
        g_gm_polys[w] = terrain_polygons(mesh, w);
        if (g_gm_polys[w] > g_gm_polys_max[w]) g_gm_polys_max[w] = g_gm_polys[w];
    }
}
// The region's polygons are drawn only where they reach into the view: the display DLL's TransformToCamera (the
// ground's update, every one - the rasterizer's +0x10c) gives each vertex of the region its clip codes - against the
// viewport's left and right planes (stored), its top and bottom (made in the call from the field of view; the
// orthographic camera's: the rectangle) and its near and far depths - and a polygon wholly outside one is left out.
// With the region alone widened the draws stayed the screen's (the pipe's rprobe: 30 ground draws, 3900 vertices at
// either margin; the kept ground's depth 17 - 37 at any margin, the far plane at 50). So for that call the view is
// widened - the stored side normals' depth part times GM_CLIP (their slope's tangent), normalized; the field of view to
// 2 atan(GM_CLIP tan(fov / 2)) for the top and bottom; the rectangle as the region's; in the T&L DLL the near depth the
// region's, at least 0.05 - in both display DLLs (their vtables, at their load: render.c render_display_loaded). The
// region decides what ground there is: by the region's factors the planes still cut into it - through the eye, each a
// line on the ground from the camera's foot (triangles of sky in the bottom corners after a zoom out from 0.5 to 1
// during a step), the bottom's and the near depth level across the screen.
// the viewport's four stored side planes widened by k (each normal's depth part times it, normalized) - the stored
// ones into was; and put back
internal void rf_view_widen(void *viewport, float k, float was[4][3]) {
    for (int p = 0; p < 4; p++) {
        float *n = viewport_side_normal(viewport, p), x = n[0], y = n[1] * k, z = n[2], len = sqrtf(x * x + y * y + z * z);
        memcpy(was[p], n, sizeof was[p]);
        if (len > 0) { n[0] = x / len; n[1] = y / len; n[2] = z / len; }
    }
}
internal void rf_view_restore(void *viewport, float was[4][3]) {
    for (int p = 0; p < 4; p++) memcpy(viewport_side_normal(viewport, p), was[p], sizeof was[p]);
}
typedef void (__thiscall *gm_to_camera_fn)(void *ras, void *mesh, void *viewport, float dt, uint8_t a, uint8_t b);
internal gm_to_camera_fn g_gm_to_camera[2];   // DX7HRDisplay.dll's, DX7HRTnLDisplay.dll's
internal void rf_to_camera(int i, void *ras, void *mesh, void *viewport, float dt, uint8_t a, uint8_t b) {
    float *r = viewport_rect(viewport), was[4], planes[4][3], *fov = viewport_fov_at(viewport), fov0 = *fov;
    float k = g_gm_k, kb = g_gm_kb, kv = k > 1.0f || kb > 1.0f ? fmaxf(GM_CLIP, kb) : 1.0f;   // the planes' (above)
    float *nd = viewport_near_at(viewport), near0 = *nd;
    memcpy(was, r, sizeof was);
    if (kv > 1.0f) {
        rf_scale_rect(r, was, k, kb, g_gm_kt);
        if (fov0 > 0 && fov0 < 3.0f) *fov = 2.0f * atanf(kv * tanf(fov0 * 0.5f));
        rf_view_widen(viewport, kv, planes);
        // the near depth in the T&L DLL only: the other projects the vertices itself
        if (i == 1 && g_gm_near != 0 && g_gm_near < near0) *nd = fmaxf(g_gm_near, 0.05f);
    }
    g_gm_to_camera[i](ras, mesh, viewport, dt, a, b);
    if (kv > 1.0f) {
        memcpy(r, was, sizeof was);
        *fov = fov0;
        *nd = near0;
        rf_view_restore(viewport, planes);
    }
}
// The objects' margin ([display] object_margin; the pipe: picture objects): the units, trees and buildings are cut to
// the screen twice - the units' collection tests each object's model against the view (GEPhysicalModel::Intersects),
// the display DLL each model it draws (GERasterizer::IsPerspectiveModelVisible), both by the viewport's stored side
// planes - so a frame during a step that had scrolled showed the ground beyond the screen bare, the objects there
// came with the next update (the player: "one extra tile before trees and units vanish would be good"). One that fails
// is tested again with the planes widened by 1 + the margin (render_shadows.c's hooks of both tests, which pass the
// casters toward the sun the same way).
internal float g_om_margin = 0.1f;
internal LONG g_om_passed;   // objects passed by the margin
internal uint8_t rf_object_again(const void *model, const void *viewport, uint8_t flag) {
    if (!(g_om_margin > 0)) return 0;
    float was[4][3];
    rf_view_widen((void *)viewport, 1.0f + g_om_margin, was);
    uint8_t r = model_intersects(model, viewport, flag);
    rf_view_restore((void *)viewport, was);
    if (r) g_om_passed++;
    return r;
}
internal void __thiscall rf_hook_to_camera0(void *ras, void *mesh, void *viewport, float dt, uint8_t a, uint8_t b) { rf_to_camera(0, ras, mesh, viewport, dt, a, b); }
internal void __thiscall rf_hook_to_camera1(void *ras, void *mesh, void *viewport, float dt, uint8_t a, uint8_t b) { rf_to_camera(1, ras, mesh, viewport, dt, a, b); }
// a display DLL loaded: its TransformToCamera hooked (1 done)
internal int rf_ground_display_loaded(HMODULE m, const char *name) {
    int i = strstr(name, "TnL") != NULL;
    void *f = (void *)GetProcAddress(m, "?TransformToCamera@DX7Rasterizer@@UAEXAAVGETerrainMesh@@AAVGEViewport@@M_N2@Z");
    if (!f || g_gm_to_camera[i]) return 0;
    g_gm_to_camera[i] = (gm_to_camera_fn)f;
    return replace_rdata_ptr(m, f, i ? (void *)rf_hook_to_camera1 : (void *)rf_hook_to_camera0) > 0;
}
internal void rf_ground_frame(int skip) {
    g_kg_skip = skip;
    rf_camera_frame();
    if (skip) return;
    ee_camera *cam = game_camera();
    if (!cam) return;
    const float *m = (const float *)camera_transformation_at(cam);
    g_kg_eye[0] = m[3]; g_kg_eye[1] = m[7]; g_kg_eye[2] = m[11];
    g_kg_view_ok = rf_camera_view(g_kg_view, m);
    rf_ground_clear(&g_kg);
    rf_ground_clear(&g_kw);
    rf_ground_clear(&g_ks);
}
// a frame during a step: the last frame's ground into the list
internal void rf_ground_again(rf_ground *g) {
    if (!g_kg_skip || !g->nb) return;
    rd3_flush();
    for (int k = 0; k < g->nb; k++) {
        const rf_kept *b = &g->b[k];
        g_rd3.rtv = b->c.rtv; g_rd3.dsv = b->c.dsv;
        g_rd3.blend = b->c.blend; g_rd3.depth = b->c.depth; g_rd3.raster = b->c.raster; g_rd3.samp = b->c.samp;
        g_rd3.srv = b->c.srv;
        g_rd3.vport = b->c.vport;
        g_rb.topo = b->c.topo;
        g_rd3.vc = b->vc; g_rd3.pc = b->pc;
        g_rf.vc_new = g_rf.pc_new = 1;
        memcpy(g_rb.v, g->v + b->v0, (size_t)b->nv * sizeof *g_rb.v);
        memcpy(g_rb.i, g->i + b->i0, (size_t)b->ni * sizeof *g_rb.i);
        memcpy(g_rb.w, g->w + b->w0, (size_t)b->nw * sizeof *g_rb.w);
        g_rb.nv = b->nv; g_rb.ni = b->ni; g_rb.nw = b->nw;
        rd3_flush();
        g_kg_drawn++;
    }
    g_rd3.bound = 0;   // the game's next draw binds all its states again
}

internal void rd3_rigid_inverse(float *r, const float *m);
internal void rd3_shadow_frame(void);
internal void rd3_shadow_vc(rd3_vcb *vc);
internal void rd3_shadow_pc(rd3_pcb *pc);
internal void rd3_shadow_bind(ID3D11DeviceContext *x);
internal int rd3_shadow_only(int role);
internal void rd3_fxaa(ID3D11RenderTargetView *rtv, const D3D11_VIEWPORT *vp);
// The camera as it is now on every 3D draw (rf_shift, as the list is drawn): the camera's move and its zoom turn and
// move it (its transformation as rf_camera_frame makes it each frame), but the view the game's draws are made with
// follows only at the camera's step in the simulation thread - a frame drawn during a step showed
// the camera's place, the frames between steps the view's, up to a step's scrolling behind (the player's game, the
// pipe's picture camtrace: the eye 0.25-0.7 tiles back at a time) and the picture jumped between the two. So each
// draw's view is replaced by the camera's: its transformation M (a GETransformation, by rows with the eye in the
// fourth column) to the view by the game's axes A = M^T V, taken from a frame whose view and transformation have the
// same eye (rf_camera_axes) - the view now (M^T)^-1 A. Moving the eye alone left the zoom's turn out: it jittered
// (the player). A draw with the camera's place in its view (the units') gets V_now V^-1 before its view and
// projection; the ground's and the water's (drawn in the camera's space by a view of their own that swaps y and z -
// 1 0 0 / 0 0 1 / 0 1 0) go back through that view to the camera of its frame's update and forward through the one
// now. Without a move or a zoom nothing changes.
internal float g_cam_axes[16];   // A: the camera's transformation to its view, the game's axes
internal int g_cam_axes_ok;
// the camera's view by its transformation m (0: no axes yet)
internal int rf_camera_view(float *v, const float *m) {
    if (!g_cam_axes_ok) return 0;
    float mt[16], inv[16];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) mt[i * 4 + j] = m[j * 4 + i];
    rd3_rigid_inverse(inv, mt);
    rd3_mul(v, inv, g_cam_axes);
    return 1;
}
// The camera's transformation itself is made only in the camera's step (0x51EE75), which runs in the simulation
// thread's loop between its steps: the move (scrolling, on this thread: camera.c) sets where it goes, the step makes
// the transformation and the device's view from it. In a light game the step comes every few ms; in the player's
// 8000-unit save the transformation stood for up to six frames of 120 while the look-at moved every frame, then
// jumped (picture camtrace, 2026-11-05) - scrolling, the edge, the middle mouse and the zoom all felt slow. So every
// frame makes it here as the step would (make_transformation: only the camera's fields and the map's heights read):
// at blend 1 after a move (snap), during the zoom's ride its blend moved on by the time since the step last changed
// it - never back within one ride (a step's dt shorter than the time here), a new ride (blend set back) starts over.
internal float g_cam_now[16];   // the camera's transformation this frame (rf_camera_frame)
internal float g_cam_view_now[16];
internal int g_cam_now_ok;
internal float g_cam_blend_seen, g_cam_t;
internal LONGLONG g_cam_blend_at;
internal void rf_camera_frame(void) {
    ee_camera *cam = game_camera();
    g_cam_now_ok = 0;
    if (!cam || !game_map()) return;
    const float *m = (const float *)camera_transformation_at(cam);
    if (camera_fixed_view()) memcpy(g_cam_now, m, sizeof g_cam_now);
    else {
        LONGLONG now = rd_qpc();
        float b = camera_get_blend(cam), t;
        if (b != g_cam_blend_seen) {   // the step ran (or a new ride began)
            if (b < g_cam_blend_seen) g_cam_t = b;
            g_cam_blend_seen = b;
            g_cam_blend_at = now;
        }
        if (camera_get_snap(cam)) t = 1;
        else {
            t = b + camera_get_blend_rate(cam) * (float)((double)(now - g_cam_blend_at) / (double)rd_qpc_ms(1000));
            if (!(t <= 1)) t = 1;   // NaN too
            if (t < g_cam_t) t = g_cam_t;
            if (t < 0) t = 0;
        }
        g_cam_t = t;
        camera_make_transformation(cam, g_cam_now, t);
    }
    g_cam_now_ok = rf_camera_view(g_cam_view_now, g_cam_now);
}
// a frame drawn with an update: the axes from its view where its eye is the transformation's - to 0.01 tile: the
// view's eye made back from its rotation and place is a float's rounding off, more far out on the map (eyes 130 tiles
// out in the player's 8000-unit save). Taken before the overlay's frame end (rd_dev_end),
// which clears scene_seen: after it, at the scene's end, the axes were never taken at all - nothing was corrected
// from 91c9476 on, the frames lagged as before (the player: "it feels like before again")
internal void rf_camera_axes(void) {
    ee_camera *cam = game_camera();
    const float *m = cam ? (const float *)camera_transformation_at(cam) : NULL, *v = g_rd3.scene_view;
    if (!m || !g_rd3.scene_seen) return;
    for (int a = 0; a < 3; a++)
        if (fabsf(m[4 * a + 3] + (v[12] * v[4 * a] + v[13] * v[4 * a + 1] + v[14] * v[4 * a + 2])) > 1e-2f) return;
    float mt[16];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++) mt[i * 4 + j] = m[j * 4 + i];
    rd3_mul(g_cam_axes, mt, v);
    g_cam_axes_ok = 1;
}
internal void rf_shift(rd3_vcb *vc, int ro) {
    float now[16], inv[16], k[16], t[16];
    if (ro < RO_TERRAIN || ro > RO_WATER || !g_cam_now_ok) return;
    memcpy(now, g_cam_view_now, sizeof now);
    if (vc->view[12] != 0 || vc->view[13] != 0 || vc->view[14] != 0) {
        rd3_rigid_inverse(inv, vc->view);
        rd3_mul(k, now, inv);   // V_now V^-1
    } else {
        if (!g_kg_view_ok) return;
        float sinv[16], a[16];
        rd3_rigid_inverse(inv, g_kg_view);
        rd3_rigid_inverse(sinv, vc->view);
        rd3_mul(a, vc->view, inv);
        rd3_mul(t, a, now);
        rd3_mul(k, t, sinv);    // S V_update^-1 V_now S^-1
    }
    float dev = 0;
    for (int i = 0; i < 16; i++) dev += fabsf(k[i] - (i % 5 == 0 ? 1.0f : 0.0f));
    if (dev < 1e-5f) return;
    rd3_mul(t, k, vc->vpm);
    memcpy(vc->vpm, t, sizeof t);
    rd3_mul(t, k, vc->view);
    memcpy(vc->view, t, sizeof t);
}
// the list's shaders, buffers and maps bound (the batches' own states come with them)
internal void rf_bind(ID3D11DeviceContext *x) {
    ID3D11DeviceContext_IASetInputLayout(x, g_rd3.inputs);
    ID3D11DeviceContext_VSSetShader(x, g_rd3.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(x, g_rd3.ps, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(x, 0, 1, &g_rd3.vcb);
    ID3D11DeviceContext_VSSetShaderResources(x, 1, 1, &g_rd3.wsrv);
    ID3D11DeviceContext_PSSetConstantBuffers(x, 1, 1, &g_rd3.pcb);
    UINT stride = sizeof(rb_vertex), off = 0;
    ID3D11DeviceContext_IASetVertexBuffers(x, 0, 1, &g_rd3.vb, &stride, &off);
    ID3D11DeviceContext_IASetIndexBuffer(x, g_rd3.ib, DXGI_FORMAT_R32_UINT, 0);
    rd3_shadow_bind(x);
}
// the list drawn, in its order (the shadows' maps first), and emptied - all its states bound, whoever used the context
// before (the 2D path, the present, the overlay set their own). At the first of the interface's draws after the 3D
// scene the scene is smoothed (render_fxaa.c), and everything bound again.
internal void rf_replay(void) {
    if (!g_rf.n || g_rf.in_submit) return;
    g_rf.in_submit = 1;
    ID3D11DeviceContext *x = g_d3d.ctx;
    rd3_shadow_frame();   // the maps from the list's casters and ground (nothing while the shadows are the game's)
    rf_bind(x);
    // what is bound, set again where a batch's differs
    rf_cmd b;
    memset(&b, 0, sizeof b);
    b.vc = b.pc = -1;
    b.topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    int first = 1;
    for (int i = 0; i < g_rf.n; i++) {
        const rf_cmd *c = &g_rf.cmd[i];
        if (rd3_shadow_only(c->role)) continue;   // a caster off the screen: in the map only
        if (!first && b.ro >= RO_TERRAIN && b.ro <= RO_WATER && (c->ro < RO_TERRAIN || c->ro > RO_WATER)) {
            rd3_fxaa(b.rtv, &b.vport);
            rf_bind(x);
            first = 1;
            b.topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        }
        if (first || c->rtv != b.rtv || c->dsv != b.dsv) ID3D11DeviceContext_OMSetRenderTargets(x, 1, &c->rtv, c->dsv);
        if (first || memcmp(&c->vport, &b.vport, sizeof b.vport)) ID3D11DeviceContext_RSSetViewports(x, 1, &c->vport);
        if (first || c->blend != b.blend) ID3D11DeviceContext_OMSetBlendState(x, c->blend, NULL, 0xFFFFFFFF);
        if (first || c->depth != b.depth) ID3D11DeviceContext_OMSetDepthStencilState(x, c->depth, 0);
        if (first || c->raster != b.raster) ID3D11DeviceContext_RSSetState(x, c->raster);
        if (first || c->srv != b.srv) ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &c->srv);
        if (first || c->samp != b.samp) ID3D11DeviceContext_PSSetSamplers(x, 0, 1, (ID3D11SamplerState **)&c->samp);
        if (c->topo != b.topo) ID3D11DeviceContext_IASetPrimitiveTopology(x, c->topo);
        if (c->vc != b.vc || c->ro != b.ro) {
            rd3_vcb vc = g_rf.vcs[c->vc];
            rd3_shadow_vc(&vc);   // this frame's map
            rf_shift(&vc, c->ro);
            ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_rd3.vcb, 0, NULL, &vc, 0, 0);
        }
        if (c->pc != b.pc) {
            rd3_pcb pc = g_rf.pcs[c->pc];
            rd3_shadow_pc(&pc);
            ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_rd3.pcb, 0, NULL, &pc, 0, 0);
        }
        b = *c;
        first = 0;
        ID3D11DeviceContext_DrawIndexed(x, c->n, c->at, c->base);
        g_rb.batches++;
    }
    g_rf.n = g_rf.nvc = g_rf.npc = 0;
    g_rf.vc_new = g_rf.pc_new = 1;   // the next batch's constants a snapshot of their own
    g_rf.replays++;
    g_rd_epoch++;   // no surface is in the list now (rd_gpu_texture)
    g_rf.in_submit = 0;
}
// everything recorded drawn now: the batch so far into the list, the list
internal void rd3_submit(void) {
    rd3_flush();
    if (g_rf.n) g_rf.forced++;   // (the scene's end: rf_scene_end)
    rf_replay();
}
// the scene's end: the list drawn; each upload ring started over while it is empty if less is left than the frame
// used (rd3_upload: else the next frame's list cut where it wraps)
internal LONG g_kg_trace;   // the pipe's picture camtrace: frames left to log
internal float g_kg_trace_dx, g_kg_trace_dy;   // and the camera moved this far a frame meanwhile (as scrolling does)
internal void rf_scene_end(void) {
    rd3_flush();
    rf_replay();
    ee_camera *cam = game_camera();
    if (g_kg_trace > 0 && cam) {   // the camera's look-at against the eye of the view the units were drawn with
        g_kg_trace--;
        const float *v = g_rd3.scene_view, *at = camera_look_at(cam);
        float e[3];
        for (int j = 0; j < 3; j++) e[j] = -(v[12] * v[4 * j] + v[13] * v[4 * j + 1] + v[14] * v[4 * j + 2]);
        const float *tr = g_cam_now_ok ? g_cam_now : (const float *)camera_transformation_at(cam);
        vec3 ec = { tr[3], tr[7], tr[11] };
        log_locked("# camtrace2: %lu %s at %.3f %.3f %.3f eye %.3f %.3f %.3f, the eye at the update %.3f %.3f %.3f, the camera's eye %.3f %.3f %.3f (t %.3f, its blend %.3f, snap %d, made %d)\n",
                   (unsigned long)GetTickCount(), g_kg_skip ? "step" : "real", (double)at[0], (double)at[1], (double)at[2],
                   (double)e[0], (double)e[1], (double)e[2], (double)g_kg_eye[0], (double)g_kg_eye[1], (double)g_kg_eye[2],
                   (double)ec.x, (double)ec.y, (double)ec.z, (double)g_cam_t, (double)camera_get_blend(cam), camera_get_snap(cam), g_cam_now_ok);
        // a ground vertex back in the world by either eye: the ground's grid lies on whole tiles with the one it was made by
        for (int k = 0; k < 3 && g_kg.nv; k++) {
            float c[3], w[2][3];
            float p[3];
            memcpy(p, g_kg.v[k * (g_kg.nv - 1) / 2].fvf, sizeof p);
            const float *gv = g_kg.nb ? g_kg.b[0].vc.view : NULL;   // through the ground's own view first
            for (int j = 0; j < 3; j++) c[j] = gv ? p[0] * gv[j] + p[1] * gv[4 + j] + p[2] * gv[8 + j] : p[j];
            for (int j = 0; j < 3; j++) {
                float r = c[0] * v[4 * j] + c[1] * v[4 * j + 1] + c[2] * v[4 * j + 2];
                w[0][j] = r + e[j];
                w[1][j] = r + g_kg_eye[j];
            }
            log_locked("# camtrace2:   ground vertex %d: by the view's eye %.3f %.3f %.3f, by the update's %.3f %.3f %.3f\n", k,
                       (double)w[0][0], (double)w[0][1], (double)w[0][2], (double)w[1][0], (double)w[1][1], (double)w[1][2]);
        }
        if (g_kg.nb) {
            const float *gv = g_kg.b[0].vc.view, *gw = g_kg.w[g_kg.b[0].w0];
            log_locked("# camtrace2:   the ground's view %.3f %.3f %.3f / %.3f %.3f %.3f / %.3f %.3f %.3f / %.3f %.3f %.3f; its first world"
                       " %.3f %.3f %.3f / %.3f %.3f %.3f / %.3f %.3f %.3f / %.3f %.3f %.3f (%d batches)\n",
                       (double)gv[0], (double)gv[1], (double)gv[2], (double)gv[4], (double)gv[5], (double)gv[6], (double)gv[8], (double)gv[9], (double)gv[10],
                       (double)gv[12], (double)gv[13], (double)gv[14], (double)gw[0], (double)gw[1], (double)gw[2], (double)gw[4], (double)gw[5],
                       (double)gw[6], (double)gw[8], (double)gw[9], (double)gw[10], (double)gw[12], (double)gw[13], (double)gw[14], g_kg.nb);
        }
        if (g_kg_trace_dx != 0 || g_kg_trace_dy != 0) camera_pan(g_kg_trace_dx, g_kg_trace_dy);
    }
    static UINT was[3];   // where the frame began in each (after a wrap inside it: 0, the use a little short)
    UINT *pos[3] = { &g_rd3.vb_pos, &g_rd3.ib_pos, &g_rd3.wb_pos }, cap[3] = { g_rd3.vb_cap, g_rd3.ib_cap, g_rd3.wb_cap };
    for (int k = 0; k < 3; k++) {
        UINT used = *pos[k] >= was[k] && *pos[k] <= cap[k] ? *pos[k] - was[k] : *pos[k] <= cap[k] ? *pos[k] : 0;
        if (*pos[k] >= cap[k] || cap[k] - *pos[k] < used + used / 8) *pos[k] = cap[k];   // with an eighth to spare
        was[k] = *pos[k] >= cap[k] ? 0 : *pos[k];
    }
}
