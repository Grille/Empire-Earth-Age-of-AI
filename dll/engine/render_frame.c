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
// frame's again in the ground's place. The game's view is made in the update too: such a frame's units are drawn from
// the camera of the last update. The camera moved since (scrolling goes on, on this thread: camera.c) is the list's
// to show (rf_shift): every 3D draw's constants moved by it as the list is drawn - the vertices stay as they were
// placed, the shadows' maps with them.
typedef struct { rf_cmd c; rd3_vcb vc; rd3_pcb pc; int v0, nv, i0, ni, w0, nw; } rf_kept;
typedef struct {
    rb_vertex *v;
    uint32_t *i;
    float (*w)[16];
    rf_kept *b;
    int nv, ni, nw, nb, cap_v, cap_i, cap_w, cap_b;
} rf_ground;
internal rf_ground g_kg;   // the last frame drawn with an update (a frame during a step records none)
internal int g_kg_skip;   // this frame is drawn during a step: nothing recorded
internal LONG g_kg_drawn; // ground batches drawn again (the pipe's picture)
internal float g_kg_cam[3];   // the camera's look-at at the last update: the view the game's draws have
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
internal void rf_ground_frame(int skip) {
    g_kg_skip = skip;
    if (skip) return;
    ee_camera *cam = game_camera();
    if (cam) memcpy(g_kg_cam, camera_look_at(cam), sizeof g_kg_cam);
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

internal void rd3_shadow_frame(void);
internal void rd3_shadow_vc(rd3_vcb *vc);
internal void rd3_shadow_pc(rd3_pcb *pc);
internal void rd3_shadow_bind(ID3D11DeviceContext *x);
internal int rd3_shadow_only(int role);
internal void rd3_fxaa(ID3D11RenderTargetView *rtv, const D3D11_VIEWPORT *vp);
// a frame during a step: a 3D draw's constants moved by the camera's move since the last update (rf_ground) - the
// units' (a view with the camera's place in it) by the world's shift, the ground's and the water's (drawn in the
// camera's space, a view without one) by that shift seen through the camera
internal void rf_shift(rd3_vcb *vc, int ro) {
    ee_camera *cam = game_camera();
    if (!g_kg_skip || ro < RO_TERRAIN || ro > RO_WATER || !cam) return;
    // the look-at's height too: it follows the ground under it (the picture went up or down a hill's worth)
    const float *at = camera_look_at(cam), *v = g_rd3.scene_view;
    float d[3] = { at[0] - g_kg_cam[0], at[1] - g_kg_cam[1], at[2] - g_kg_cam[2] }, t[3];
    if (d[0] == 0 && d[1] == 0 && d[2] == 0) return;
    if (vc->view[12] != 0 || vc->view[13] != 0 || vc->view[14] != 0) for (int a = 0; a < 3; a++) t[a] = -d[a];
    else for (int a = 0; a < 3; a++) t[a] = -(d[0] * v[a] + d[1] * v[4 + a] + d[2] * v[8 + a]);
    // the translation before the matrix (row vectors): its last row plus t's combination of the first three
    for (int c = 0; c < 4; c++) {
        vc->vpm[12 + c] += t[0] * vc->vpm[c] + t[1] * vc->vpm[4 + c] + t[2] * vc->vpm[8 + c];
        vc->view[12 + c] += t[0] * vc->view[c] + t[1] * vc->view[4 + c] + t[2] * vc->view[8 + c];
    }
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
internal float g_kg_trace_dx;   // and the camera moved this far a frame meanwhile (as scrolling does)
internal void rf_scene_end(void) {
    rd3_flush();
    rf_replay();
    ee_camera *cam = game_camera();
    if (g_kg_trace > 0 && cam) {   // the camera's look-at against the eye of the view the units were drawn with
        g_kg_trace--;
        if (g_kg_trace_dx != 0) camera_pan(g_kg_trace_dx, 0);
        const float *v = g_rd3.scene_view, *at = camera_look_at(cam);
        float e[3];
        for (int j = 0; j < 3; j++) e[j] = -(v[12] * v[4 * j] + v[13] * v[4 * j + 1] + v[14] * v[4 * j + 2]);
        log_locked("# camtrace2: %lu %s at %.3f %.3f %.3f eye %.3f %.3f %.3f, at the update %.3f %.3f %.3f\n",
                   (unsigned long)GetTickCount(), g_kg_skip ? "step" : "real", (double)at[0], (double)at[1], (double)at[2],
                   (double)e[0], (double)e[1], (double)e[2], (double)g_kg_cam[0], (double)g_kg_cam[1], (double)g_kg_cam[2]);
    }
    if (g_rd3.vb_pos > RD3_VB_BYTES / 4 * 3) g_rd3.vb_pos = RD3_VB_BYTES;
    if (g_rd3.ib_pos > RD3_IB_BYTES / 4 * 3) g_rd3.ib_pos = RD3_IB_BYTES;
    if (g_rd3.wb_pos > RD3_WB_BYTES / 4 * 3) g_rd3.wb_pos = RD3_WB_BYTES;
}
