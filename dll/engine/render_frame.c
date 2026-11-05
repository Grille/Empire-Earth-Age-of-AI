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
    if (g_kg_skip || g_ro_cur != RO_TERRAIN) return;
    rf_ground *g = &g_kg;
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
}
// a frame during a step: the last frame's ground into the list
internal void rf_ground_again(void) {
    rf_ground *g = &g_kg;
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
// the scene's end: the list drawn; the ring started over while it is empty if less than a quarter is left
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
    if (g_rd3.vb_pos > RD3_VB_BYTES / 4 * 3) g_rd3.vb_pos = RD3_VB_BYTES;
    if (g_rd3.ib_pos > RD3_IB_BYTES / 4 * 3) g_rd3.ib_pos = RD3_IB_BYTES;
    if (g_rd3.wb_pos > RD3_WB_BYTES / 4 * 3) g_rd3.wb_pos = RD3_WB_BYTES;
}
