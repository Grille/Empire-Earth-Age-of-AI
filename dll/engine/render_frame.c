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
    void *soft;      // its soft edges are drawn apart, with this depth state (rf_edges); NULL: drawn whole
    int sil;         // its part in the silhouettes (render_silhouette.c SIL_), of a batch that writes the depth
    int wat;         // its place around the water's surface in this list (RF_W_, rf_replay)
    int role, ro;    // the shadows' role (render_shadows.c SH_), the render object drawing it
    UINT at, n;      // the indices in the upload buffer
    INT base;        // the vertices' (in vertices)
    UINT inst, inst_at;  // instances of one list (render_instance.c): how many, where their entries start; 0: a plain draw
    ID3D11Buffer *gvb;   // the ground's own vertices (render_ground.c): drawn from them, no indices
    ID3D11ShaderResourceView *ghs;   // and its texture's heights (rgb_height); NULL: its borders as the game's
} rf_cmd;
internal struct {
    rf_cmd *cmd;
    rd3_vcb *vcs;
    rd3_pcb *pcs;
    int n, cap, nvc, cap_vc, npc, cap_pc;
    int vc_new, pc_new;     // g_rd3.vc / pc changed since the last snapshot
    LONG replays, forced;   // lists drawn; of them inside a scene (not at its end) - the minute's line
    LONG wraps;             // the ring full within a scene
    LONGLONG qpc;           // time in drawing the lists (rf_replay: the shadows' maps with them) - the minute's line
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
#define RF_EDGES_REF 200   // alpha tested at this or more: a tree, its edge a narrow band - not worth drawing a forest twice
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
    c->soft = g_ro_cur == RO_UNITS && g_rd3.blended && g_rd3.pc.aop[3] && g_rd3.pc.misc[0] < RF_EDGES_REF ? g_rd3.depth_soft : NULL;
    c->sil = g_ro_cur == RO_UNITS && g_rd3.depth_soft ? g_rd3.sil : 0;
    c->at = at; c->n = n; c->base = base;
    c->gvb = NULL;
    c->ghs = NULL;
    c->inst = c->inst_at = 0;
}
// a list's draws of one triangles' list (render_instance.c): at, n its place in the pool, inst_at where its entries start
internal void rf_record_inst(UINT at, UINT n, UINT inst, UINT inst_at) {
    int k = g_rf.n;
    rf_record(at, n, 0);
    if (g_rf.n > k) { g_rf.cmd[k].inst = inst; g_rf.cmd[k].inst_at = inst_at; }
}
// the batch's draw call (its index buffer the ring's, or the instances' pool)
internal void rf_call(ID3D11DeviceContext *x, const rf_cmd *c) {
    if (c->inst) {
        if (!g_in_pool) { ID3D11DeviceContext_IASetIndexBuffer(x, g_in_ib, DXGI_FORMAT_R32_UINT, 0); g_in_pool = 1; }
        ID3D11DeviceContext_DrawIndexedInstanced(x, c->n, c->inst, c->at, 0, c->inst_at);
        return;
    }
    if (g_in_pool) { ID3D11DeviceContext_IASetIndexBuffer(x, g_rd3.ib, DXGI_FORMAT_R32_UINT, 0); g_in_pool = 0; }
    ID3D11DeviceContext_DrawIndexed(x, c->n, c->at, c->base);
}
// the instance stream, slot 1
internal void rf_bind_ids(ID3D11DeviceContext *x) {
    UINT stride = 4, off = 0;
    ID3D11DeviceContext_IASetVertexBuffers(x, 1, 1, &g_in_ids, &stride, &off);
}

// THE GROUND KEPT (render_3d.c "the frame during a step"): a frame drawn while the simulation runs a step skips the
// render objects' update, and the ground's draw draws nothing without it (the ground's patches are made in the update,
// with the units' visible region - that update can't run alone). So each frame's ground batches are kept on the CPU as
// they are flushed - vertices, indices, world matrices, states, constants - and a frame during a step draws the last
// frame's again in the ground's place. The camera moved since (scrolling goes on, on this thread: camera.c) is the
// list's to show (rf_shift, below) - the vertices stay as they were placed, the shadows' maps with them.
// the interface's (g_ku) hold their surface too: drawn again, its pixels are uploaded if they changed since - a text
// the game wrote anew after the last update (the population's count) was drawn empty in the frames without one
typedef struct { rf_cmd c; rd3_vcb vc; rd3_pcb pc; int v0, nv, i0, ni, w0, nw, morph; rd_surface *src; int keyed; } rf_kept;
typedef struct {
    rb_vertex *v;
    uint32_t *i;
    rb_world *w;
    rf_kept *b;
    int nv, ni, nw, nb, cap_v, cap_i, cap_w, cap_b;
} rf_ground;
internal int rf_camera_view(float *v, const float *m);
internal rf_ground g_kg;   // the last frame drawn with an update (a frame during a step records none)
internal rf_ground g_kw;   // its water (the same: rf_water_again)
internal int g_kw_surface = -1;   // of those the surface's (the ones before the fifth queue's effects: render_ground.c rg_hook); -1 not known
internal rf_ground g_ks;   // the game's shadows' decals, the farms' fields: the same (their draw, as the ground's, draws
                           // nothing during a step - every frame drawn during one showed the farms without their fields,
                           // the player 2026-11-06: "farms flicker in visibility", the pipe's record: fields 0 in those)
// the farms' stalks among g_ks's vertices, swayed again in the frames during a step (collect.c fsw_tags_*)
internal void fsw_tags_clear(void);
internal void fsw_tags_keep(int v0);
internal void fsw_tags_apply(int v0, int nv);
internal void fsw_pend_clear(void);
internal void fsw_rb_tag(uint32_t base, uint32_t nv);
// The interface's draws too, for the frames without the game's update at all (render_3d.c g_rwl_bare: the interface's
// lock held by a thread waiting to write): each form's DrawPolygons draws the list its CopyData (the interface's update)
// made for the frame and empties it - the command buttons, the cancel button while placing a building, parts of the
// minimap were missing from every such frame (the player at stepload 1000: they flickered after a click; the pipe's
// record bare: 26000 pixels a frame, the buttons). Recorded in every frame whose interface was updated, steps' too.
internal rf_ground g_ku;
// And the text: the game draws it after the render objects, from what the interface's draw left for it (the chat, the
// messages, the line at the top: 2 to 44 batches a frame outside any render object, onto the interface's target) - a
// frame without the update replays the interface's batches instead of its draw, and the text was left out: every such
// frame showed the text dim, what the interface's own batches hold of it (the player, 2026-11-11: "text flickers too
// (messages and ui)"; record bare: 45000 - 56000 pixels of the messages a frame). From the end of the interface's draw in
// a frame with its update to the present (the mouse cursor is the present's), what is drawn outside the render objects
// is kept with it.
internal int g_ku_text;
internal int g_rwl_bare_now;   // this frame goes without the game's update (render_3d.c rd_hook_graphics_update)
internal int g_kg_skip;   // this frame is drawn during a step: nothing recorded
internal LONG g_kg_drawn; // ground batches drawn again (the pipe's picture)
internal float g_kg_eye[3];   // the camera's eye at the last update (the pipe's picture camtrace)
internal float g_kg_view[16];    // the camera's view at the last update: the ground's camera space is made from it
internal int g_kg_view_ok;
internal float g_kg_view_ground[16];   // the camera's view the ground and the water were put into its space with
internal int g_kg_view_ground_ok;
internal void rf_ground_clear(rf_ground *g) {
    for (int k = 0; k < g->nb; k++) {
        D3D_RELEASE(g->b[k].c.srv);
        if (g->b[k].src) rd_sf_release(g->b[k].src);
    }
    g->nv = g->ni = g->nw = g->nb = 0;
    if (g == &g_ks) fsw_tags_clear();
}
// A target or depth buffer freed (render.c rd_surface_free): what is kept to be drawn again holds its views without a
// reference - the kept batches here, the ground's states (render_ground.c) - and a frame during a step bound them freed.
// The game makes its targets anew at another resolution: the editor's Test, into a game of another size, died at the
// first such frame (the player, 2026-11-11: "game died", in d3d11 under rd3_bars with the kept ground's target). A
// store that drew on it is dropped (the next frame with an update records it anew); the current target forgotten too.
internal int rg_target_gone(ID3D11RenderTargetView *rtv, ID3D11DepthStencilView *dsv);   // render_ground.c
internal void rf_target_gone(ID3D11RenderTargetView *rtv, ID3D11DepthStencilView *dsv) {
    if (!rtv && !dsv) return;
    rf_ground *const kept[4] = { &g_kg, &g_kw, &g_ks, &g_ku };
    int dropped = 0;
    for (int j = 0; j < 4; j++)
        for (int k = 0; k < kept[j]->nb; k++)
            if ((rtv && kept[j]->b[k].c.rtv == rtv) || (dsv && kept[j]->b[k].c.dsv == dsv)) { rf_ground_clear(kept[j]); dropped++; break; }
    int states = rg_target_gone(rtv, dsv);
    if (dropped || states)
        log_locked("# render: a %s freed that frames during a step would draw on again: %d kept stores dropped, the ground's states of %d forgotten\n",
                   rtv ? "target" : "depth buffer", dropped, states);
    if (rtv && g_rd3.rtv == rtv) g_rd3.rtv = NULL;
    if (dsv && g_rd3.dsv == dsv) g_rd3.dsv = NULL;
}
// a view that is the identity: a draw in pixels (the 2D mode's)
internal int rf_pixels(const float *view) {
    static const float ident[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
    int same = 1;
    for (int i = 0; i < 16; i++) same &= view[i] == ident[i];
    return same;
}
// the batch being flushed (rd3_flush, before its matrices' place is added), when it is the ground's - not what the ground's
// pass draws in pixels: an airport's lines to its targets (rf_ground_lines), which kept stood where the last update's camera
// had put them
internal void rf_keep_ground(void) {
    if (g_ro_cur == RO_TERRAIN && rf_pixels(g_rd3.vc.view)) return;
    int text = g_ro_cur < 0 && g_ku_text && !g_rwl_bare_now;   // the interface's text (g_ku_text)
    if (!text && (g_ro_cur == RO_UI ? g_rwl_bare_now : g_kg_skip || (g_ro_cur != RO_TERRAIN && g_ro_cur != RO_WATER && g_ro_cur != RO_SHADOWS)))
        return;
    rf_ground *g = text || g_ro_cur == RO_UI ? &g_ku : g_ro_cur == RO_WATER ? &g_kw : g_ro_cur == RO_SHADOWS ? &g_ks : &g_kg;
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
    b->morph = g_rb.morph;
    b->src = g == &g_ku && g_rd3.srv && g_rd3.srv_of && g_rd3.srv_of->kind != RK_TARGET ? g_rd3.srv_of : NULL;
    b->keyed = g_rd3.srv_keyed;
    if (b->src) rd_sf_addref(b->src);
    b->vc = g_rd3.vc; b->pc = g_rd3.pc;
    b->v0 = g->nv; b->nv = g_rb.nv; b->i0 = g->ni; b->ni = g_rb.ni; b->w0 = g->nw; b->nw = g_rb.nw;
    if (g == &g_ks) fsw_tags_keep(b->v0);
    memcpy(g->v + g->nv, g_rb.v, (size_t)g_rb.nv * sizeof *g->v);
    memcpy(g->i + g->ni, g_rb.i, (size_t)g_rb.ni * sizeof *g->i);
    memcpy(g->w + g->nw, g_rb.w, (size_t)g_rb.nw * sizeof *g->w);
    g->nv += g_rb.nv; g->ni += g_rb.ni; g->nw += g_rb.nw;
}
// the render objects' update: a frame drawn as before (skip 0) - its ground recorded anew - or during a step (skip 1:
// the last one's drawn again - the units' view is that frame's update's; one frame older was one frame's camera move
// off - the ground and the debris on it jumped against each other while scrolling, the player 2026-11-05)
internal void rf_camera_frame(void);
internal void rf_view_now(void);
internal void rf_depth_frame(void);
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
// the eight corners of the frustum through rectangle r on the map, as GetFrustrumWorldPoints makes them; 0: not known
internal int rf_corners(const void *viewport, const float *r, float (*w)[3]) {
    float s = viewport_scale(viewport);
    if (!g_gm_denorm || !(s != 0)) return 0;
    const float *c = viewport_centre(viewport);
    float x0 = r[1], x1 = r[2], y0 = r[3], y1 = r[0];
    if (x1 - x0 <= 2) x1 = x0 + 2;   // as GetFrustrumWorldPoints
    if (y1 - y0 <= 2) y1 = y0 + 2;
    for (int i = 0; i < 8; i++) {
        float p[3] = { ((i & 1 ? x1 : x0) - c[0]) / s, -((i & 2 ? y1 : y0) - c[1]) / s, (float)(i >> 2) };
        g_gm_denorm(viewport, p, i & 4 ? viewport_far(viewport) : viewport_near(viewport), w[i]);
        for (int a = 0; a < 2; a++) if (!(w[i][a] > -1e6f && w[i][a] < 1e6f)) return 0;
    }
    return 1;
}
// the larger side of the region the frustum through rectangle r would give, tiles less one (-1: not known)
internal long rf_region_size(const void *viewport, const float *r) {
    ee_map *map = game_map();
    float w[8][3];
    if (!map || !rf_corners(viewport, r, w)) return -1;
    float lo[2] = { 1e30f, 1e30f }, hi[2] = { -1e30f, -1e30f };
    for (int i = 0; i < 8; i++)
        for (int a = 0; a < 2; a++) {
            if (w[i][a] < lo[a]) lo[a] = w[i][a];
            if (w[i][a] > hi[a]) hi[a] = w[i][a];
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
// A zoom out under way (the player: "do you consider that the flat zoom out moves many tiles backward first before
// gaining real height?"): the camera's ride from zoom 0.2 to 1 takes the eye about 24 tiles back over the ground
// before it climbs, and the frames drawn during a step until the next update show the ground made for the eye of
// that update - the screen's bottom third the sky in nearly every frame of the ride (the 8000-unit save under 300 ms
// steps, a zoom out from 0.2). A region reaching further behind did nothing: TransformToCamera marks each vertex
// against the view's planes, all through the eye, and leaves out a triangle wholly behind one of them - the ground
// behind the eye is always below its bottom plane. So for the ground's update the viewport's camera is pulled back
// over the ground to where the ride ends (the end made as the camera's step makes it, at blend 1), its turn and
// height kept and its far plane longer by as much - the region, the marks and the ground's camera space all of that
// camera - and set back after (GEViewport::SetCamera both times: its inverse and projections made again). The
// ground's view to correct from is the camera TransformToCamera ran with (g_kg_view_ground, rf_to_camera).
typedef void (__thiscall *gm_set_camera_fn)(void *viewport, const float *t);
internal gm_set_camera_fn g_gm_set_camera;
internal void *g_gm_viewport;           // the map's viewport, as the region's call gets it
internal float g_gm_ride;               // how far the last update's camera was pulled back (the pipe's picture)
internal float g_gm_pull[16], g_gm_was[16], g_gm_far_was;
internal float g_gm_rest_zoom;          // the camera's zoom when its last ride ended
#define GM_RIDE_MAX 30.0f               // tiles; the longest zoom out rides about 24 (above)
internal int g_gm_pulled;
internal void rf_pull_begin(void) {
    g_gm_pulled = 0;
    g_gm_ride = 0;
    if (!g_gm_viewport || g_gm_pools < 2 || !(g_gm_margin > 0)) return;
    if (!g_gm_set_camera) {
        HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
        if (lle) g_gm_set_camera = (gm_set_camera_fn)(void *)GetProcAddress(lle, "?SetCamera@GEViewport@@QAEXABVGETransformation@@@Z");
        if (!g_gm_set_camera) return;
    }
    ee_camera *cam = game_camera();
    if (!cam || camera_fixed_view()) return;
    // only a zoom out: its zoom above where the last ride ended. A pan's ride has the same zoom, and on a map made new
    // in the editor its end lay 42 tiles from the eye - the ground made for that camera, the sky blue for as long as
    // an arrow key was held (the player, 2026-11-08)
    float zoom = camera_get_zoom(cam);
    if (!(camera_get_blend(cam) < 1)) { g_gm_rest_zoom = zoom; return; }
    if (camera_get_snap(cam) || !(zoom > g_gm_rest_zoom)) return;
    const float *m = viewport_camera(g_gm_viewport), *at = camera_look_at(cam);
    float end[16];
    camera_make_transformation(cam, end, 1.0f);
    float ex = end[3] - at[0], ey = end[7] - at[1], nx = m[3] - at[0], ny = m[7] - at[1];
    if (ex * ex + ey * ey <= nx * nx + ny * ny) return;   // a zoom in: the eye comes nearer
    float dx = end[3] - m[3], dy = end[7] - m[7];
    if (dx * dx + dy * dy > GM_RIDE_MAX * GM_RIDE_MAX) return;
    g_gm_ride = sqrtf(dx * dx + dy * dy);
    memcpy(g_gm_was, m, sizeof g_gm_was);
    memcpy(g_gm_pull, m, sizeof g_gm_pull);
    g_gm_pull[3] = end[3];
    g_gm_pull[7] = end[7];
    g_gm_set_camera(g_gm_viewport, g_gm_pull);
    float *depth = viewport_far_at(g_gm_viewport);
    g_gm_far_was = *depth;
    *depth += g_gm_ride;
    g_gm_pulled = 1;
}
internal void rf_pull_end(void) {
    if (!g_gm_pulled) return;
    *viewport_far_at(g_gm_viewport) = g_gm_far_was;
    g_gm_set_camera(g_gm_viewport, g_gm_was);
    g_gm_pulled = 0;
}
// PAST THE MESH'S ARRAYS (a camera further back than the 88 tiles' region can follow - the ground and the water are the map's
// by then, render_ground.c): the rows the units and trees are collected from are made here, as GetTileRowsInVolume makes
// them - for each row of the map the hull of the frustum's eight corners on the map, within the map's own row - and the
// game's UpdateVisibleTerrain gets a nearer far depth and if need be a smaller rectangle, so that its region fits: what is
// left of its use (the highlights on the ground) is the part near the camera's foot. Two lists: the simulation's copy reads them.
#define VR_MAX 4096
typedef struct { int n; long y[VR_MAX], x0[VR_MAX], x1[VR_MAX]; } vr_list;
internal vr_list g_vr[2];
internal volatile LONG g_vr_cur = -1;   // the list in use, -1: the game's rows
internal unsigned long vr_rows(const void *mesh) { LONG c = g_vr_cur; return c < 0 ? terrain_rows(mesh) : (unsigned long)g_vr[c].n; }
internal void vr_row(const void *mesh, unsigned long i, long *y, long *x0, long *x1) {
    LONG c = g_vr_cur;
    if (c < 0) { terrain_row(mesh, i, y, x0, x1); return; }
    *y = g_vr[c].y[i]; *x0 = g_vr[c].x0[i]; *x1 = g_vr[c].x1[i];
}
// the span of a segment within the band of row y (y .. y + 1)
internal void vr_band(const float *a, const float *b, float y, float *lo, float *hi) {
    float ya = a[1], yb = b[1];
    if (fmaxf(ya, yb) < y || fminf(ya, yb) > y + 1) return;
    float t0 = 0, t1 = 1;
    if (ya != yb) {
        float p = (y - ya) / (yb - ya), q = (y + 1 - ya) / (yb - ya);
        t0 = fmaxf(0, fminf(p, q)); t1 = fminf(1, fmaxf(p, q));
    }
    float x0 = a[0] + (b[0] - a[0]) * t0, x1 = a[0] + (b[0] - a[0]) * t1;
    *lo = fminf(*lo, fminf(x0, x1)); *hi = fmaxf(*hi, fmaxf(x0, x1));
}
internal void vr_make(const void *viewport, const float *was) {
    ee_map *map = game_map();
    float w[8][3], r[4];
    rf_scale_rect(r, was, 1.1f, 1.2f, 1.1f);   // a little beyond the screen: a step's frames move the camera without the rows
    if (!map || !rf_corners(viewport, r, w)) { g_vr_cur = -1; return; }
    vr_list *o = &g_vr[g_vr_cur == 0 ? 1 : 0];
    int size = (int)map_get_size(map), n = 0;
    float ylo = 1e30f, yhi = -1e30f;
    for (int i = 0; i < 8; i++) { ylo = fminf(ylo, w[i][1]); yhi = fmaxf(yhi, w[i][1]); }
    int y0 = (int)floorf(ylo), y1 = (int)floorf(yhi);
    if (y0 < 0) y0 = 0;
    for (int y = y0; y <= y1 && y < size && n < VR_MAX; y++) {
        float lo = 1e30f, hi = -1e30f;
        for (int i = 0; i < 8; i++) for (int j = i + 1; j < 8; j++) vr_band(w[i], w[j], (float)y, &lo, &hi);
        if (lo > hi) continue;
        const int *row = map_view_row(map, (unsigned)y);
        int first = row[0], last = row[0] + row[1] - 1, x0 = (int)floorf(lo), x1 = (int)floorf(hi);
        if (x0 < first) x0 = first;
        if (x1 > last) x1 = last;
        if (x0 > x1) continue;
        o->y[n] = y; o->x0[n] = x0; o->x1[n] = x1; n++;
    }
    o->n = n;
    g_vr_cur = o == &g_vr[0] ? 0 : 1;
}
// What lies on the ground - a farm's fields, the roads, the craters - the effects' update finds by the mesh's visible rows too,
// reading the vector itself, no call to hook: with the region cut down past the arrays only those round the camera's foot came
// (the player, 2026-11-10: "on heiger zoom, the farms only render when being somewhat centered"). Through that update the
// vector holds the rows made above.
internal long g_vr_flat[VR_MAX][3];
internal void vr_effects_update(ee_ro_effects *o) {
    ee_map *map = game_map();
    LONG c = g_vr_cur;
    if (c < 0 || !map) { ro_effects_update(o); return; }
    const vr_list *l = &g_vr[c];
    for (int i = 0; i < l->n; i++) { g_vr_flat[i][0] = l->y[i]; g_vr_flat[i][1] = l->x0[i]; g_vr_flat[i][2] = l->x1[i]; }
    long **rows = map_visible_rows_at(map), *first = rows[0], *end = rows[1];
    rows[0] = g_vr_flat[0];
    rows[1] = g_vr_flat[0] + l->n * 3;
    ro_effects_update(o);
    rows[0] = first;
    rows[1] = end;
}
// the region the game's update may make: far depth, then the rectangle, drawn in until it fits the arrays; 0: it can't be told
internal int rf_region_fit(void *viewport, float *r, const float *was) {
    float *fd = viewport_far_at(viewport), lowest = viewport_near(viewport) + 2, k = 1;
    for (int tries = 0; tries < 48; tries++) {
        long n = rf_region_size(viewport, r);
        if (n < 0) return 0;
        if (n <= GM_TILES) return 1;
        if (*fd * 0.9f > lowest) *fd *= 0.9f;
        else { k *= 0.9f; rf_scale_rect(r, was, k, k, k); }
    }
    return 0;
}
internal void __thiscall rf_hook_terrain_visible(void *mesh, const void *camera, void *viewport, unsigned long a, unsigned long b,
                                                 unsigned long c, uint8_t fixed) {
    g_gm_viewport = viewport;
    if (!g_gm_denorm) {
        HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
        if (lle) g_gm_denorm = (gm_denorm_fn)(void *)GetProcAddress(lle, "?DenormalizePerspectivePoint@GEViewport@@QBEXABVGE3DPoint@@MAAV2@@Z");
    }
    // the camera pulled back (rf_pull_begin) for the region too - unless its region at the screen's own rectangle is
    // beyond the mesh's arrays already: then as it was
    if (g_gm_pulled) {
        long n0 = rf_region_size(viewport, viewport_rect(viewport));
        if (n0 >= 0 && n0 <= GM_TILES) camera = g_gm_pull;
        else rf_pull_end();
    }
    float *r = viewport_rect(viewport), was[4], far0 = viewport_far(viewport);
    memcpy(was, r, sizeof was);
    long full = fixed ? -1 : rf_region_size(viewport, r);
    int over = full > GM_TILES, fits = 1;
    if (over) vr_make(viewport, was);
    else g_vr_cur = -1;
    // the fixed camera's frustum is the orthographic one: as it was; without the pools the mesh's own would overflow
    int on = !fixed && g_gm_pools >= 2 && !over;
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
    if (over) fits = rf_region_fit(viewport, r, was);
    if (fits) terrain_visible(mesh, camera, viewport, a, b, c, fixed);
    *viewport_far_at(viewport) = far0;
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
    g_kg_view_ground_ok = rf_camera_view(g_kg_view_ground, viewport_camera(viewport));   // its space (rf_pull_begin)
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
internal void rd3_shadow_new_frame(void);   // render_shadows.c: the water's fading for the new frame
internal void shadow_light_read(void);     // render_shadows.c: the game's light for this frame
internal void rf_ground_frame(int skip) {
    g_ku_text = 0;
    g_kg_skip = skip;
    shadow_light_read();
    rf_camera_frame();
    rf_depth_frame();
    rf_view_now();
    if (!g_rwl_bare_now) rf_ground_clear(&g_ku);
    if (skip) return;
    rd3_shadow_new_frame();
    ee_camera *cam = game_camera();
    if (!cam) return;
    const float *m = (const float *)camera_transformation_at(cam);
    g_kg_eye[0] = m[3]; g_kg_eye[1] = m[7]; g_kg_eye[2] = m[11];
    g_kg_view_ok = rf_camera_view(g_kg_view, m);
    rf_ground_clear(&g_kg);
    rf_ground_clear(&g_kw);
    g_kw_surface = -1;
    rf_ground_clear(&g_ks);
}
// a frame during a step: the last frame's ground into the list
// The ground's pass also draws the effects of the third queue, the lines from an airport to its targets (EEE3DMarchingLine)
// among them: in pixels, at points the draw takes to the screen with the camera now (rf_view_now). Kept with the ground,
// a frame during a step showed them where the last update's camera had put them while the ground under them moved on (the
// player, 2026-11-09: "they propably dont get their transforms form the render thread yet", "look like the jittering terrain
// problem we had"). So such a frame draws them itself, from the queues it took over.
// Their points are the 3D view's pixels (0x50DD81: from the view's corner, under the top bar), drawn by the viewport the
// device has: a real frame's is the 3D view's, which the game's ground draw sets - a frame during a step doesn't run it,
// and the device still had the interface's of the frame before, the whole screen: the lines stood the top bar's height
// higher (42 pixels at 2560 x 1440) - an airport's from its roof instead of its door, every frame during a step, a
// paused game's too (the player, 2026-11-12: "every few frames it jumps, this also happen on a paused came with no
// moving camera"; rprobe: viewport 0 42 2560x1260 against 0 0 2560x1440, the same points). So the last real frame's
// viewport of the ground's draw is put on the device for them, and the pixels' projection the game makes for its own idea
// of the screen (2 / width, -2 / height: the whole screen's then) made the 3D view's - what the game sets meanwhile is
// noted (rd_dev_set_viewport, rd_dev_set_transform) and is the device's again after them.
internal D3DVIEWPORT7 g_kg_vport, g_kgl_vport;   // the real frame's ground's; the game's own while the lines draw
internal D3DMATRIX g_kgl_proj;
internal int g_kg_vport_ok, g_kg_lines;
internal void rf_ground_vport(rd_device *d) {   // a real frame's ground drawn: its viewport
    if (d && d->viewport.dwWidth && d->viewport.dwHeight) g_kg_vport = d->viewport, g_kg_vport_ok = 1;
}
// the game's projection for pixels (an orthographic one: the view is the identity) as the 3D view's
internal void rf_lines_proj(D3DMATRIX *m, const D3DMATRIX *game) {
    *m = *game;
    if (game->_12 == 0 && game->_21 == 0 && game->_34 == 0 && game->_44 == 1)
        m->_11 = 2.0f / (float)g_kg_vport.dwWidth, m->_22 = -2.0f / (float)g_kg_vport.dwHeight;
}
internal int rf_ground_line(void *ctx, int q, uint32_t key, void *e) {
    (void)ctx, (void)key;
    if (q == 2 && effect_is_marching_line(e)) effect_draw(e);
    return 1;
}
internal void rf_ground_lines(void) {
    rd_device *d = g_rd3.prep_dev;
    int put = d && g_kg_vport_ok;
    if (put) {
        g_kgl_vport = d->viewport, g_kgl_proj = d->proj;
        d->viewport = g_kg_vport, rf_lines_proj(&d->proj, &g_kgl_proj);
        d->changed |= RD_CH_VC;
        g_kg_lines = 1;
    }
    effect_queues_each(effect_queues_at(), rf_ground_line, NULL);
    if (put) {
        g_kg_lines = 0;
        d->viewport = g_kgl_vport, d->proj = g_kgl_proj;
        d->changed |= RD_CH_VC;
    }
}
// the kept batches k0 .. k1 - 1 drawn again
internal void rf_ground_again_part(rf_ground *g, int k0, int k1) {
    if (!g_kg_skip || !g->nb) return;
    if (k1 > g->nb) k1 = g->nb;
    rd3_flush();
    for (int k = k0; k < k1; k++) {
        const rf_kept *b = &g->b[k];
        g_rd3.rtv = b->c.rtv; g_rd3.dsv = b->c.dsv;
        g_rd3.blend = b->c.blend; g_rd3.depth = b->c.depth; g_rd3.raster = b->c.raster; g_rd3.samp = b->c.samp;
        g_rd3.srv = b->c.srv;
        if (b->src) {
            ID3D11ShaderResourceView *now = rd_gpu_texture(b->src, b->keyed);
            if (now) g_rd3.srv = now;
            b->src->rf_used = g_rd_epoch;
        }
        g_rd3.vport = b->c.vport;
        g_rb.topo = b->c.topo;
        g_rb.morph = b->morph;
        g_rd3.vc = b->vc; g_rd3.pc = b->pc;
        g_rf.vc_new = g_rf.pc_new = 1;
        memcpy(g_rb.v, g->v + b->v0, (size_t)b->nv * sizeof *g_rb.v);
        if (g == &g_ks) fsw_tags_apply(b->v0, b->nv);
        memcpy(g_rb.i, g->i + b->i0, (size_t)b->ni * sizeof *g_rb.i);
        memcpy(g_rb.w, g->w + b->w0, (size_t)b->nw * sizeof *g_rb.w);
        g_rb.nv = b->nv; g_rb.ni = b->ni; g_rb.nw = b->nw;
        rd3_flush();
        g_kg_drawn++;
    }
    g_rd3.bound = 0;   // the game's next draw binds all its states again
}
internal void rf_ground_again(rf_ground *g) { rf_ground_again_part(g, 0, g->nb); }

internal void rd3_rigid_inverse(float *r, const float *m);
internal void rd3_shadow_frame(void);
internal void rd3_shadow_vc(rd3_vcb *vc);
internal void rd3_shadow_pc(rd3_pcb *pc);
internal void rd3_shadow_bind(ID3D11DeviceContext *x);
internal void rd3_water_reflect(void);   // render_water.c
internal void rg_replay(ID3D11DeviceContext *x, const rf_cmd *c);   // render_ground.c
internal int rd3_water_pass(ID3D11DeviceContext *x, const rf_cmd *c);
internal int rd3_water_first(void);
internal void rd3_water_steps(ID3D11DeviceContext *x, int lastw, rf_cmd *b, int *first);
internal int rd3_water_last(void);
internal int rd3_water_late(const rf_cmd *c);
internal int rd3_water_high(const rf_cmd *c);
internal int rd3_shadow_only(int role);
internal void rd3_silhouettes(ID3D11DeviceContext *x, rf_cmd *b, int *first);   // render_silhouette.c
internal int rao_on(void);   // render_ao.c
internal void rd3_ao_ground(ID3D11DeviceContext *x, ID3D11DepthStencilView *dsv);
internal void rd3_ao(ID3D11DeviceContext *x, ID3D11RenderTargetView *rtv, const D3D11_VIEWPORT *vp, ID3D11DepthStencilView *dsv, const float *vpm);
internal void rf_draw(ID3D11DeviceContext *x, const rf_cmd *c, rf_cmd *b, int *first);
internal void rf_edges_part(ID3D11DeviceContext *x, int part);
internal void rd3_fxaa(ID3D11RenderTargetView *rtv, const D3D11_VIEWPORT *vp, ID3D11DepthStencilView *dsv, const float *vpm);
internal void rd3_bars(ID3D11DeviceContext *x, ID3D11RenderTargetView *rtv, const D3D11_VIEWPORT *vp, int back);
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
// The ray through a point of the screen (the game's mouse coordinates) for the camera's transformation m, its eye and direction
// in the world (camera.c camera_zoom_at_mouse): the view from m as a frame makes it, the lens and the 3D view's rectangle the
// last drawn frame's - it isn't the window, it lies between the top bar and the bottom panel (1261 of 1440 rows high: taken
// as the window it aimed some 14 to 50 pixels above the cursor, a few tiles far out; the player 2026-11-10).
internal int rf_screen_ray(const float *m, float mx, float my, float *eye, float *d) {
    float v[16], inv[16], p[16];
    const D3D11_VIEWPORT *port = &g_rd3.scene_port;
    if (g_rd3.scene_view[15] != 1.0f || !(port->Width > 0) || !(port->Height > 0) || !rf_camera_view(v, m)) return 0;
    float x = (mx - port->TopLeftX) / port->Width, y = (my - port->TopLeftY) / port->Height;
    rd3_rigid_inverse(inv, g_rd3.scene_view);
    rd3_mul(p, inv, g_rd3.scene_vp);
    if (p[0] == 0 || p[5] == 0) return 0;
    float xv = (2.0f * x - 1.0f) / p[0], yv = (1.0f - 2.0f * y) / p[5], zv = p[11] < 0 ? -1.0f : 1.0f;
    for (int a = 0; a < 3; a++) {
        d[a] = xv * v[a * 4] + yv * v[a * 4 + 1] + zv * v[a * 4 + 2];
        eye[a] = m[a * 4 + 3];
    }
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
        camera_anchor_frame(cam, g_cam_now, t);
    }
    g_cam_now_ok = rf_camera_view(g_cam_view_now, g_cam_now);
}
// The units' draw takes world points to the screen's pixels itself (0x50DD81: the selected units' health bars, the
// building's footprint lines, the goal markers, the waypoints' lines) and the depth range of a few objects from their
// depth in the camera - through the map's viewport's world into the camera, made at the camera's step. Drawn in pixels,
// no view to correct (rf_shift), the bars stood while the units moved under them with the camera, up to a step's
// scrolling behind (the player: "the healthbars of selected units dont draw based on camera position"). Those reads
// take it from here (spec engine_display.viewport_view_ops): each frame the inverse of the camera's transformation now,
// the viewport's own while there is none.
internal float g_view_now[16];
internal void rf_view_now(void) {
    if (!g_cam_now_ok) { memcpy(g_view_now, viewport_view(), sizeof g_view_now); return; }
    const float *m = g_cam_now;   // by rows, the eye in the fourth column: rotated back, the eye through it
    for (int r = 0; r < 3; r++) {
        for (int c = 0; c < 3; c++) g_view_now[r * 4 + c] = m[c * 4 + r];
        g_view_now[r * 4 + 3] = -(m[r] * m[3] + m[4 + r] * m[7] + m[8 + r] * m[11]);
    }
    g_view_now[12] = g_view_now[13] = g_view_now[14] = 0;
    g_view_now[15] = 1;
}
internal void rf_view_install(void) {
    memcpy(g_view_now, viewport_view(), sizeof g_view_now);
    int n = patch_viewport_view(g_view_now);
    if (g_log) fprintf(g_log, "# render: the units' draw projects with the camera now (%d places)\n", n);
}
// THE FRAME'S DEPTHS (the player, 2026-11-11: "when zooming in the near plane clips, and when zooming out the fog and draw
// distance kick in. are the projection matrices updates on the simulation thread??"). They are: the camera's step (0x51EE75,
// in the simulation thread's loop between its steps) sets the map viewport's near depth - the nearest ground under three
// points of the screen less about 14 tiles, 0.1 .. 85 (0x51CE31, GEViewport::SetFrontClippingDistance) - and its far depth
// (SetRearClippingDistance: camera.c's far value); the projection and the fog's range come from them. The camera itself is
// made anew every frame (rf_camera_frame), the step comes every third frame or so - with or without a step load - and far out
// the eye moves 12 tiles a frame in a zoom: zooming in from 200 tiles back the ground at the bottom of the screen came 57
// tiles away with the near depth at 62, the bottom third of the picture cut off in one frame of three; zooming out the far
// depth stayed at 99 for two frames with the top of the screen 180 tiles away. So a frame whose camera isn't the step's gets
// depths of its own: the near depth the step's less how much nearer the ground under the screen's bottom edge is than from
// the step's camera (when that is more than RF_NEAR_SLACK: the game's 14 tiles take a pan), the far depth the one the next
// step sets (camera.c camera_far_pending), the fog's range moved with it. Every draw with the map's perspective gets them,
// the kept ones too (rf_shift), and so do what reads the depth back (the water, the ambient occlusion, the smoothing, the
// silhouettes), the ground's chunks' test against the view (render_ground.c) and the display DLL's test of a model
// (render_shadows.c shr_hook_visible). Where the step's camera is the frame's nothing changes.
#define RF_NEAR_LEAST 0.1f   // the game's least near depth (0x82C6F4)
#define RF_NEAR_SLACK 1.0f   // tiles the ground may come nearer before the near depth follows
internal struct {
    int on;                // this frame's depths aren't the step's
    int off;               // the pipe: picture depths off
    float n0, f0, n, f;    // the step's near and far, this frame's
    float moved;           // the eye's way from the step's
    float was[16], was_z;  // the step's camera, the depth of the ground under the screen's bottom edge from it
    long frames, nearer, further;   // frames with depths of their own, with a nearer near depth, a further far depth (the pipe)
} g_dz;
// the depth along the view of the nearest ground under the screen's bottom edge (its corners and middle) for the camera's
// transformation m; -1: none
internal float rf_bottom_depth(const float *m, float low) {
    const D3D11_VIEWPORT *port = &g_rd3.scene_port;
    float eye[3], f[3], d[3], at[3], best = -1;
    if (!rf_screen_ray(m, port->TopLeftX + port->Width * 0.5f, port->TopLeftY + port->Height * 0.5f, eye, f)) return -1;
    float fl = sqrtf(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    if (!(fl > 0)) return -1;
    for (int i = 0; i < 3; i++) {
        if (!rf_screen_ray(m, port->TopLeftX + port->Width * 0.5f * (float)i, port->TopLeftY + port->Height - 1, eye, d)) continue;
        if (!camera_ground_point(eye, d, low, at)) continue;
        float z = ((at[0] - eye[0]) * f[0] + (at[1] - eye[1]) * f[1] + (at[2] - eye[2]) * f[2]) / fl;
        if (best < 0 || z < best) best = z;
    }
    return best;
}
internal void rf_depth_frame(void) {
    g_dz.on = 0;
    void *vp = opacity_viewport();
    ee_camera *cam = game_camera();
    if (g_dz.off || !vp || !cam || !g_cam_now_ok || camera_fixed_view() || !(g_rd3.scene_port.Height > 0)) return;
    float was[16];
    memcpy(was, viewport_camera(vp), sizeof was);   // (the step writes it on its thread: a copy, read once)
    float n0 = viewport_near(vp), f0 = viewport_far(vp), n = n0, f = camera_far_pending(f0);
    if (!(n0 > 0) || !(f0 > n0)) return;
    float dx = g_cam_now[3] - was[3], dy = g_cam_now[7] - was[7], dz = g_cam_now[11] - was[11];
    float moved = sqrtf(dx * dx + dy * dy + dz * dz);
    if (moved > 0.01f) {
        float low = camera_look_at(cam)[2];
        if (memcmp(g_dz.was, was, sizeof was)) { memcpy(g_dz.was, was, sizeof was); g_dz.was_z = rf_bottom_depth(was, low); }
        float now = rf_bottom_depth(g_cam_now, low);
        if (g_dz.was_z > 0 && now >= 0 && g_dz.was_z - now > RF_NEAR_SLACK) n = fmaxf(fminf(n0, RF_NEAR_LEAST), n0 - (g_dz.was_z - now));
    }
    // on even where they are the step's: a frame during a step draws the ground with the projection of the last frame
    // that updated it, and the step may have moved the viewport's since (at a step load of 30 ms the kept ground cut at
    // a near depth of 85 while the step's was 62)
    g_dz.n0 = n0; g_dz.f0 = f0; g_dz.n = n; g_dz.f = f; g_dz.moved = moved;
    g_dz.on = 1;
    if (n == n0 && f == f0) return;
    g_dz.frames++;
    if (n < n0) g_dz.nearer++;
    if (f > f0) g_dz.further++;
}
// the near and far depths of a view x projection m (its depth a + b / w, w along the view's axis); 0: no perspective along
// w's axis, or none of the map's: the sky (a box 10000 tiles out, drawn with the terrain) has the map's near depth and a
// far depth of 20002 - given the frame's it was cut off, and above the horizon stood what the frames before left there
// (the game clears only the depth; the player, 2026-11-11: "sky is gone, just uncleared buffer"). A far depth beyond the
// furthest the camera gives (camera.c camera_far_most; with a margin for the matrices' rounding) isn't the map's.
#define RF_FAR_SLACK 1.25f
internal int rf_depth_of(const float *m, float *n, float *f) {
    int k = 0;
    for (int j = 1; j < 3; j++) if (fabsf(m[j * 4 + 3]) > fabsf(m[k * 4 + 3])) k = j;
    if (!(fabsf(m[k * 4 + 3]) > 1e-6f)) return 0;
    float a = m[k * 4 + 2] / m[k * 4 + 3], b = m[14] - a * m[15];
    for (int j = 0; j < 3; j++) if (fabsf(m[j * 4 + 2] - a * m[j * 4 + 3]) > 1e-4f * fabsf(m[k * 4 + 2])) return 0;
    if (!(a > 1.0f) || !(b < 0)) return 0;
    *n = -b / a;
    *f = b / (1.0f - a);
    return *f <= camera_far_most() * RF_FAR_SLACK;
}
// whether depths n, f are this frame's (to the matrix's rounding: one that has them is left as it is, to the bit)
internal int rf_depth_same(float n, float f) { return fabsf(n - g_dz.n) <= 1e-3f * g_dz.n && fabsf(f - g_dz.f) <= 1e-3f * g_dz.f; }
// a view x projection of the map's perspective with this frame's depths
internal void rf_depth_vpm(float *m) {
    float n, f;
    if (!g_dz.on || !rf_depth_of(m, &n, &f) || rf_depth_same(n, f)) return;
    float a = g_dz.f / (g_dz.f - g_dz.n), b = -g_dz.n * a;
    for (int j = 0; j < 3; j++) m[j * 4 + 2] = a * m[j * 4 + 3];
    m[14] = a * m[15] + b;
}
// a draw's constants: its view x projection, and the fog's range (far - 8 .. far, the game's) ending at the frame's far
// depth, its width as it was - the fog and the depth of one far value. The game's draws don't always have that: the
// fog's range is set at each draw from the viewport as it is then, the projection at the frame's start, and the
// simulation's step changes the viewport meanwhile; the kept draws of a frame during a step have the update's; the
// ground (render_ground.c) got the frame's depths before it came here and the fog of the states the game set for it.
// Moved by the two far depths' difference (471d71e7) the fog of a draw whose projection was older than its fog ended
// below 0 - zooming in, every unit in the fog's colour for a frame (the player, 2026-11-11: "when zooming in from far
// all units a drawn in fog color for a frame sometimes"; a test game, wheel zooms at a 30 - 100 ms step load: 100 and
// more such draws a run, fog 435 .. 443 in a projection with a far depth of 1018, the frame's 443); and a draw whose
// depths were the frame's already kept the fog of an older, nearer far depth - zooming out the top of the screen in
// the fog ("when zooming out quickly the top sometimes is drawn in fog for a frame": a far depth of 296 with the fog
// ending at 139). camera.c's far value isn't the fog's end either: at rest after a zoom in it was 115 and the step's
// far depth, the fog's with it, 296 (the step copies it when the camera moves).
// THE FOG'S START ([camera] fog_start; the player, 2026-11-12: "add a fog start distance, with long render distances
// propper fogging becomes desiarble"): the game's band is 8 tiles before its end whatever the view distance - with the
// mod's long views the far land stayed clear and then faded within those tiles. A share of the end (which grows as the
// camera goes out), so the haze keeps its shape at every zoom; never later than the game's own start. Linear fog only
// (the game's); the water's fog under the level is the same fog (render_3d.c fog_at) and follows.
internal float g_rf_fog_start = -1;   // the pipe's picture fog, -1 the setting's
internal void rf_fog_start(rd3_vcb *vc) {
    float s = g_rf_fog_start >= 0 ? g_rf_fog_start : g_set.fog_start;
    if (vc->fog[0] != 3 || s <= 0) return;
    float a = vc->fog[2] * s;
    if (a < vc->fog[1]) vc->fog[1] = a;
}
internal void rf_depth_vc(rd3_vcb *vc) {
    float n, f;
    if (g_dz.on && rf_depth_of(vc->vpm, &n, &f)) {
        if (!rf_depth_same(n, f)) rf_depth_vpm(vc->vpm);
        if (vc->fog[0] == 3 && fabsf(vc->fog[2] - g_dz.f) > 1e-3f * g_dz.f) { vc->fog[1] += g_dz.f - vc->fog[2]; vc->fog[2] = g_dz.f; }
    }
    rf_fog_start(vc);
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
#define RF_FIELD_BIAS 5e-6f   // of the depth buffer's 0..1: about 0.001 tiles at the usual zoom, the matrices' rounding about 0.0002
internal void rf_shift(rd3_vcb *vc, int ro) {
    float now[16], inv[16], k[16], t[16];
    if (ro < RO_TERRAIN || ro > RO_WATER || !g_cam_now_ok) return;
    // a draw in pixels (the 2D mode's view, the identity: the selected units' health bars, the building's footprint
    // lines, placed with the camera now by rf_view_now) has no camera to correct - taken for the ground's (no place in
    // its view) it was turned by the two cameras' rounding even with the camera still, its depth 0 then below the near
    // plane on one side of a line: the bars there cut off (the player's game: 71 bars from x 628 to 1061, those left
    // of 900 not drawn - "it changes with camera movement, when the camera is still the healthbars are stable")
    if (rf_pixels(vc->view)) return;
    rf_depth_vc(vc);   // this frame's depths (above)
    memcpy(now, g_cam_view_now, sizeof now);
    if (vc->view[12] != 0 || vc->view[13] != 0 || vc->view[14] != 0) {
        rd3_rigid_inverse(inv, vc->view);
        rd3_mul(k, now, inv);   // V_now V^-1
    } else {
        int ground = (ro == RO_TERRAIN || ro == RO_WATER) && g_kg_view_ground_ok;
        if (!ground && !g_kg_view_ok) return;
        float sinv[16], a[16];
        rd3_rigid_inverse(inv, ground ? g_kg_view_ground : g_kg_view);
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
    // a farm's field lies in the ground's plane, its depth the ground's to the last bit while both come from the same
    // camera; moved here they come by different matrices, their rounding parts them - the field and the ground fought
    // while the camera moved, not while it stood (the player, 2026-11-08). So a field moved here is a hair nearer
    if (ro == RO_SHADOWS)
        for (int i = 0; i < 4; i++) vc->vpm[i * 4 + 2] -= RF_FIELD_BIAS * vc->vpm[i * 4 + 3];
}
// the list's shaders, buffers and maps bound (the batches' own states come with them)
internal void rf_bind(ID3D11DeviceContext *x) {
    ID3D11DeviceContext_IASetInputLayout(x, g_rd3.inputs);
    ID3D11DeviceContext_VSSetShader(x, g_rd3.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(x, g_rd3.ps, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(x, 0, 1, &g_rd3.vcb);
    ID3D11DeviceContext_VSSetShaderResources(x, 1, 1, &g_rd3.wsrv);
    ID3D11DeviceContext_VSSetShaderResources(x, 5, 1, &g_mo_srv);   // the animation's pool (render_morph.c)
    ID3D11DeviceContext_PSSetConstantBuffers(x, 1, 1, &g_rd3.pcb);
    UINT stride = sizeof(rb_vertex), off = 0;
    ID3D11DeviceContext_IASetVertexBuffers(x, 0, 1, &g_rd3.vb, &stride, &off);
    ID3D11DeviceContext_IASetIndexBuffer(x, g_rd3.ib, DXGI_FORMAT_R32_UINT, 0);
    g_in_pool = 0;
    rf_bind_ids(x);
    rd3_shadow_bind(x);
}
// a viewport of the game's coordinates on the target as drawn (a larger target: render.c g_uis); NULL rtv: the larger one
internal D3D11_VIEWPORT rs_port(const D3D11_VIEWPORT *v, ID3D11RenderTargetView *rtv) {
    D3D11_VIEWPORT r = *v;
    if (g_uis.rtv && (!rtv || rtv == g_uis.rtv)) {
        r.TopLeftX *= g_uis.sx, r.Width *= g_uis.sx;
        r.TopLeftY *= g_uis.sy, r.Height *= g_uis.sy;
    }
    return r;
}
// THE SOFT EDGES: the units' and buildings' textures are drawn blended, depth written and alpha tested at 127 - what is half
// see-through at a texture's edge writes the depth like the solid rest. Whatever lies behind and comes later in the list
// fails against it, and the edge shows what was there before: the ground through the silo behind a ladder's rails (the
// player, 2026-11-09). So such a batch is drawn in two parts by its texture's alpha (not the material's: a unit fading out
// stays as it was): the solid part in the list's order, the rest without writing the depth after the last of them, over
// everything solid. The order among the edges themselves is left as it is: their overlaps are a few pixels.
#define RF_EDGES_SOLID (250.0f / 255)
internal struct {
    ID3D11Buffer *cb[3];   // the shader's edp: everything, the solid part, the edges
    int force, fail, part;
} g_fe = { .force = -1 };
// made once, the shader's constant bound; whether this list's batches are drawn in two parts
internal int rf_edges(ID3D11DeviceContext *x) {
    if (!g_fe.cb[0] && !g_fe.fail) {
        static const float v[3] = { 0, RF_EDGES_SOLID, -RF_EDGES_SOLID };
        for (int i = 0; i < 3 && !g_fe.fail; i++) {
            float k[4] = { v[i], 0, 0, 0 };
            D3D11_BUFFER_DESC bd = { sizeof k, D3D11_USAGE_IMMUTABLE, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
            D3D11_SUBRESOURCE_DATA sd = { k, 0, 0 };
            HRESULT hr = ID3D11Device_CreateBuffer(g_d3d.dev, &bd, &sd, &g_fe.cb[i]);
            if (FAILED(hr)) { g_fe.fail = 1; rd_fail("the soft edges' constants", hr); }
        }
    }
    if (g_fe.fail) return 0;
    g_fe.part = 0;
    ID3D11DeviceContext_PSSetConstantBuffers(x, 2, 1, &g_fe.cb[0]);
    return g_fe.force < 0 ? g_set.unit_soft_edges : g_fe.force;
}
internal void rf_edges_part(ID3D11DeviceContext *x, int part) {
    if (part == g_fe.part) return;
    g_fe.part = part;
    ID3D11DeviceContext_PSSetConstantBuffers(x, 2, 1, &g_fe.cb[part]);
}
// the shader's wsp, which pixels of a draw are wanted: 0 all, 1 under the water's level, 2 not under it, 3 over the land, 4 over
// the water and under its level, 5 over the water and not under it; 6 and 7 as 1 and 2 with the split RF_HIGH_SPLIT higher
// (rf_water_split). A slot of its own: the shadows' maps leave their constants in the edges' (rd3_shadow_frame)
#define RF_HIGH_SPLIT 0.1f
internal struct { ID3D11Buffer *cb[8]; int part, fail; } g_wsp;
internal void rf_water_part(ID3D11DeviceContext *x, int part) {
    if (!g_wsp.cb[0] && !g_wsp.fail) {
        static const float v[8][3] = { { 0, 0, 0 }, { 1, 0, 0 }, { -1, 0, 0 }, { 0, 1, 0 }, { 1, 2, 0 }, { -1, 2, 0 }, { 1, 0, RF_HIGH_SPLIT }, { -1, 0, RF_HIGH_SPLIT } };
        for (int i = 0; i < 8 && !g_wsp.fail; i++) {
            float k[4] = { v[i][0], v[i][1], v[i][2], 0 };
            D3D11_BUFFER_DESC bd = { sizeof k, D3D11_USAGE_IMMUTABLE, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
            D3D11_SUBRESOURCE_DATA sd = { k, 0, 0 };
            HRESULT hr = ID3D11Device_CreateBuffer(g_d3d.dev, &bd, &sd, &g_wsp.cb[i]);
            if (FAILED(hr)) { g_wsp.fail = 1; rd_fail("the water's sides' constants", hr); }
        }
        g_wsp.part = -1;
    }
    if (g_wsp.fail || part == g_wsp.part) return;
    g_wsp.part = part;
    ID3D11DeviceContext_PSSetConstantBuffers(x, 5, 1, &g_wsp.cb[part]);
}
// a batch of the list drawn, its states set where they differ from the batch before (b); first: all of them
internal void rf_draw(ID3D11DeviceContext *x, const rf_cmd *c, rf_cmd *b, int *first) {
    if (rd3_shadow_only(c->role)) return;   // a caster off the screen: in the map only
    if (*first || c->rtv != b->rtv || c->dsv != b->dsv) ID3D11DeviceContext_OMSetRenderTargets(x, 1, &c->rtv, c->dsv);
    if (*first || memcmp(&c->vport, &b->vport, sizeof b->vport)) {
        D3D11_VIEWPORT port = rs_port(&c->vport, c->rtv);
        ID3D11DeviceContext_RSSetViewports(x, 1, &port);
    }
    if (*first || c->blend != b->blend) ID3D11DeviceContext_OMSetBlendState(x, c->blend, NULL, 0xFFFFFFFF);
    if (*first || c->depth != b->depth) ID3D11DeviceContext_OMSetDepthStencilState(x, c->depth, 0);
    if (*first || c->raster != b->raster) ID3D11DeviceContext_RSSetState(x, c->raster);
    if (*first || c->srv != b->srv) ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &c->srv);
    if (*first || c->samp != b->samp) ID3D11DeviceContext_PSSetSamplers(x, 0, 1, (ID3D11SamplerState **)&c->samp);
    if (c->topo != b->topo) ID3D11DeviceContext_IASetPrimitiveTopology(x, c->topo);
    if (c->vc != b->vc || c->ro != b->ro) {
        rd3_vcb vc = g_rf.vcs[c->vc];
        rd3_shadow_vc(&vc);   // this frame's map
        rf_shift(&vc, c->ro);
        if (g_uis.rtv && c->rtv == g_uis.rtv) vc.vp[0] /= g_uis.sx, vc.vp[1] /= g_uis.sy;   // half a pixel of the picture's
        ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_rd3.vcb, 0, NULL, &vc, 0, 0);
    }
    if (c->pc != b->pc) {
        rd3_pcb pc = g_rf.pcs[c->pc];
        rd3_shadow_pc(&pc);
        ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_rd3.pcb, 0, NULL, &pc, 0, 0);
    }
    *b = *c;
    *first = 0;
    if (rd3_water_pass(x, c)) return;
    if (c->gvb) rg_replay(x, c);
    else rf_call(x, c);
    g_rb.batches++;
}
// The 3D scene of the list drawn (s: its first batch with the map's camera; more: the interface follows in this list): what
// it wrote outside its viewport put back and, before the interface, the scene smoothed (render_fxaa.c), everything bound
// again. By its last batch and not where the next one is no longer 3D: the silhouettes leave the states unbound, and with
// no water on the screen they come right before the interface - no frame of a land view was smoothed.
internal void rf_scene_done(ID3D11DeviceContext *x, const rf_cmd *s, rf_cmd *b, int *first, int more) {
    D3D11_VIEWPORT port = rs_port(&s->vport, s->rtv);
    rd3_bars(x, s->rtv, &port, 1);
    float vpm[16];
    memcpy(vpm, g_rf.vcs[s->vc].vpm, sizeof vpm);
    rf_depth_vpm(vpm);   // the depth as the frame's draws wrote it
    if (more) rd3_fxaa(s->rtv, &port, s->dsv, vpm);
    rf_bind(x);
    *first = 1;
    b->topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
}
// A batch's place around the water's surface. RF_W_SPLIT: see-through and no depth written, in two parts by its height
// (render_water.c rd3_water_late). RF_W_TOP: what the 3D passes draw in pixels (the selected units' health bars, the lines
// of orders) knows no height and stands over the picture: after the surface and the water's own effects (the player,
// 2026-11-10: "the confirmation cicles are drawn behind the water surface, as do the healthbasr of selected units, and on
// smoke etc"). RF_W_RING: the circles on the ground, a move's and the selected units', drawn at depth 0 (the viewport's depth
// range) and left to what comes after to cover them: over the land they stay where the game draws them, or a circle would
// lie over the building in front of it; over the water, as the first kind.
enum { RF_W_PLACE, RF_W_SPLIT, RF_W_TOP, RF_W_RING };
// a see-through batch's part under the level (under 1) or over it (0): split RF_HIGH_SPLIT higher where render_water.c says
internal int rf_water_split(const rf_cmd *c, int under) { return rd3_water_high(c) ? (under ? 6 : 7) : (under ? 1 : 2); }
internal int rf_water_place(const rf_cmd *c, int lastw, int i) {
    if (lastw < 0 || g_wsp.fail) return RF_W_PLACE;
    if (i < lastw && (c->ro == RO_TERRAIN || c->ro == RO_UNITS) && rf_pixels(g_rf.vcs[c->vc].view)) return RF_W_TOP;
    if (!rd3_water_late(c)) return RF_W_PLACE;
    return c->vport.MaxDepth <= 0 && i < lastw ? RF_W_RING : RF_W_SPLIT;
}
// the list drawn, in its order (the shadows' maps first), and emptied - all its states bound, whoever used the context
// before (the 2D path, the present, the overlay set their own). At the first of the interface's draws after the 3D
// scene the scene is smoothed (rf_scene_done).
internal void rf_replay(void) {
    if (!g_rf.n || g_rf.in_submit) return;
    g_rf.in_submit = 1;
    LONGLONG t0 = rd_qpc();
    ID3D11DeviceContext *x = g_d3d.ctx;
    int edges = rf_edges(x), laste = -1, lastu = -1;
    if (edges) for (int i = g_rf.n - 1; i >= 0 && laste < 0; i--) if (g_rf.cmd[i].soft) laste = i;
    rd3_shadow_frame();   // the maps from the list's casters and ground (nothing while the shadows are the game's)
    // the maps leave their constants in the edges' slot (b2): bound again at the next part asked for - until then the
    // mirrored picture and the list's batches before the first soft one read the light's matrix as their edp
    g_fe.part = -1;
    if (g_fe.fail) { ID3D11Buffer *none = NULL; ID3D11DeviceContext_PSSetConstantBuffers(x, 2, 1, &none); }
    rd3_water_reflect();  // the mirrored picture for the water's surface
    rf_bind(x);
    // what is bound, set again where a batch's differs
    rf_cmd b;
    memset(&b, 0, sizeof b);
    b.vc = b.pc = -1;
    b.topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    rf_water_part(x, 0);
    int first = 1, lastw = rd3_water_last(), firstw = lastw >= 0 ? rd3_water_first() : -1, lastp = -1, first3 = -1, last3 = -1;
    for (int i = 0; i < g_rf.n; i++) {
        g_rf.cmd[i].wat = rf_water_place(&g_rf.cmd[i], lastw, i);
        if (g_rf.cmd[i].ro == RO_WATER) lastp = i;
    }
    const rf_cmd *scene = NULL;
    int unit3 = -1, lastg = -1;   // a batch of the units' with the camera's own projection (the ground's view is another: render_ao.c); the ground's last batch
    for (int i = 0; i < g_rf.n; i++) {
        const rf_cmd *c = &g_rf.cmd[i];
        if (c->ro < RO_TERRAIN || c->ro > RO_WATER) continue;
        if (first3 < 0) first3 = i;
        last3 = i;
        if (!scene && !rf_pixels(g_rf.vcs[c->vc].view)) scene = c;
        if (unit3 < 0 && c->ro == RO_UNITS && !rf_pixels(g_rf.vcs[c->vc].view)) unit3 = i;
        if (c->ro == RO_TERRAIN && c->gvb) lastg = i;
    }
    // the silhouettes after the units' last batch drawn in its place: one held back for after the water is passed over below
    for (int i = g_rf.n - 1; i >= 0 && lastu < 0; i--) if (g_rf.cmd[i].ro == RO_UNITS && !(i < lastw && (g_rf.cmd[i].wat == RF_W_SPLIT || g_rf.cmd[i].wat == RF_W_TOP))) lastu = i;
    for (int i = 0; i < g_rf.n; i++) {
        const rf_cmd *c = &g_rf.cmd[i];
        if (i == first3 && scene) { D3D11_VIEWPORT port = rs_port(&scene->vport, scene->rtv); rd3_bars(x, scene->rtv, &port, 0); }
        if (i == firstw) {
            rd3_water_steps(x, lastw, &b, &first);
            for (int j = 0; j < g_rf.n; j++) if (g_rf.cmd[j].wat == RF_W_SPLIT) { rf_water_part(x, rf_water_split(&g_rf.cmd[j], 1)); rf_draw(x, &g_rf.cmd[j], &b, &first); }
            rf_water_part(x, 4);
            for (int j = 0; j < g_rf.n; j++) if (g_rf.cmd[j].wat == RF_W_RING) rf_draw(x, &g_rf.cmd[j], &b, &first);
        }
        if (i < lastw && (c->wat == RF_W_SPLIT || c->wat == RF_W_TOP)) continue;
        rf_water_part(x, c->wat == RF_W_SPLIT ? rf_water_split(c, 0) : c->wat == RF_W_RING ? 3 : 0);
        if (!g_fe.fail) rf_edges_part(x, edges && c->soft ? 1 : 0);
        rf_draw(x, c, &b, &first);
        if (i == lastg) rd3_ao_ground(x, c->dsv);
        if (i == laste) {
            rf_edges_part(x, 2);
            for (int j = 0; j <= laste; j++) if (g_rf.cmd[j].soft) { rf_cmd e = g_rf.cmd[j]; e.depth = e.soft; rf_draw(x, &e, &b, &first); }
            rf_edges_part(x, 0);
        }
        if (i == lastu && scene && rao_on()) {   // all that writes the depth drawn (render_ao.c), the water and the silhouettes yet to come
            D3D11_VIEWPORT port = rs_port(&scene->vport, scene->rtv);
            float vpm[16];
            memcpy(vpm, g_rf.vcs[unit3 >= 0 ? g_rf.cmd[unit3].vc : scene->vc].vpm, sizeof vpm);
            rf_depth_vpm(vpm);   // the depth as the frame's draws wrote it
            rd3_ao(x, scene->rtv, &port, scene->dsv, vpm);
            rf_bind(x);
            first = 1;
            b.topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
        }
        if (i == lastu) rd3_silhouettes(x, &b, &first);
        if (i == lastw) {
            for (int j = 0; j < lastw; j++) if (g_rf.cmd[j].wat == RF_W_SPLIT) { rf_water_part(x, rf_water_split(&g_rf.cmd[j], 0)); rf_draw(x, &g_rf.cmd[j], &b, &first); }
            rf_water_part(x, 5);
            for (int j = 0; j < lastw; j++) if (g_rf.cmd[j].wat == RF_W_RING) rf_draw(x, &g_rf.cmd[j], &b, &first);
        }
        if (i == lastp && lastw >= 0) {
            rf_water_part(x, 0);
            for (int j = 0; j < lastw; j++) if (g_rf.cmd[j].wat == RF_W_TOP) rf_draw(x, &g_rf.cmd[j], &b, &first);
        }
        if (i == last3 && scene) rf_scene_done(x, scene, &b, &first, i + 1 < g_rf.n);
    }
    rf_water_part(x, 0);
    g_rf.n = g_rf.nvc = g_rf.npc = 0;
    g_rf.vc_new = g_rf.pc_new = 1;   // the next batch's constants a snapshot of their own
    g_rf.replays++;
    g_rf.qpc += rd_qpc() - t0;
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
            const float *gv = g_kg.b[0].vc.view, *gw = g_kg.w[g_kg.b[0].w0].m;
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
        if (*pos[k] == RD3_RESTART) { was[k] = 0; continue; }   // (not used this frame: still to start over)
        UINT used = *pos[k] >= was[k] && *pos[k] <= cap[k] ? *pos[k] - was[k] : *pos[k] <= cap[k] ? *pos[k] : 0;
        if (*pos[k] >= cap[k] || cap[k] - *pos[k] < used + used / 8) *pos[k] = RD3_RESTART;   // with an eighth to spare
        was[k] = *pos[k] == RD3_RESTART ? 0 : *pos[k];
    }
}
