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

internal void rd3_shadow_frame(void);
internal void rd3_shadow_vc(rd3_vcb *vc);
internal void rd3_shadow_pc(rd3_pcb *pc);
internal void rd3_shadow_bind(ID3D11DeviceContext *x);
// the list drawn, in its order (the shadows' maps first), and emptied - all its states bound, whoever used the context
// before (the 2D path, the present, the overlay set their own)
internal void rf_replay(void) {
    if (!g_rf.n || g_rf.in_submit) return;
    g_rf.in_submit = 1;
    ID3D11DeviceContext *x = g_d3d.ctx;
    rd3_shadow_frame();   // the maps from the list's casters and ground (nothing while the shadows are the game's)
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
    // what is bound, set again where a batch's differs
    rf_cmd b;
    memset(&b, 0, sizeof b);
    b.vc = b.pc = -1;
    b.topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
    int first = 1;
    for (int i = 0; i < g_rf.n; i++) {
        const rf_cmd *c = &g_rf.cmd[i];
        if (first || c->rtv != b.rtv || c->dsv != b.dsv) ID3D11DeviceContext_OMSetRenderTargets(x, 1, &c->rtv, c->dsv);
        if (first || memcmp(&c->vport, &b.vport, sizeof b.vport)) ID3D11DeviceContext_RSSetViewports(x, 1, &c->vport);
        if (first || c->blend != b.blend) ID3D11DeviceContext_OMSetBlendState(x, c->blend, NULL, 0xFFFFFFFF);
        if (first || c->depth != b.depth) ID3D11DeviceContext_OMSetDepthStencilState(x, c->depth, 0);
        if (first || c->raster != b.raster) ID3D11DeviceContext_RSSetState(x, c->raster);
        if (first || c->srv != b.srv) ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &c->srv);
        if (first || c->samp != b.samp) ID3D11DeviceContext_PSSetSamplers(x, 0, 1, (ID3D11SamplerState **)&c->samp);
        if (c->topo != b.topo) ID3D11DeviceContext_IASetPrimitiveTopology(x, c->topo);
        if (c->vc != b.vc) {
            rd3_vcb vc = g_rf.vcs[c->vc];
            rd3_shadow_vc(&vc);   // this frame's map
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
internal void rf_scene_end(void) {
    rd3_flush();
    rf_replay();
    if (g_rd3.vb_pos > RD3_VB_BYTES / 4 * 3) g_rd3.vb_pos = RD3_VB_BYTES;
    if (g_rd3.ib_pos > RD3_IB_BYTES / 4 * 3) g_rd3.ib_pos = RD3_IB_BYTES;
    if (g_rd3.wb_pos > RD3_WB_BYTES / 4 * 3) g_rd3.wb_pos = RD3_WB_BYTES;
}
