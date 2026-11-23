// THE WEIGHTED TRANSPARENCY ([display] unit_transparency; the pipe: picture oit on | off). Art of Conquest's crystals and
// glass are textures half see-through over their whole face, not only at an edge: the soft edges (render_frame.c rf_edges)
// draw what is see-through of a texture after everything solid, without writing the depth and in the list's order - right
// for a ladder's rails, a few pixels wide, wrong for a crystal: inside one cluster's mesh and between clusters the later
// faces covered the earlier whichever stood in front (the player, 2026-11-23: "many materials in aoc make use of half
// trancparency, wich look quite bad witout sorting"). Sorting wouldn't do: a cluster is one draw, and triangles sorted
// every frame cost what the units' own draw saved. So a draw of such a texture (oit_glass, measured at its upload), or of
// a see-through material (a unit fading out), has its see-through part - by the texture's alpha and the material's
// (rf_edges parts 3 and 4) - go into two sums in any order (McGuire and Bavoil 2013): its colour times its alpha and a
// weight falling with the distance, and the product of what each layer lets through; at the soft edges' place one pass
// lays their weighted mean over the scene, as strong as the layers together cover. The layers' order is only in the
// weight: a dark pane before a bright one comes out between the two. The solid part writes the depth as before, so the
// see-through part is hidden by what stands before it and covers what stands behind it.
// Only those: with every soft edge in the sums (tried first) the trees came out darker and muddier - many layers of
// nearly solid leaves averaged instead of the front one covering - and every unit was drawn once more for nothing.
// The batches: the units' (units, buildings, the map's objects), blended as the game's usual alpha (source alpha, one
// less it), depth written, not drawn in pixels or at depth 0 (the health bars, the circles on the ground). The rest -
// added glows, the water's split - as before.
internal struct {
    int force, fail;
    ID3D11PixelShader *ps, *over;   // a batch's see-through part into the sums; the sums over the scene
    ID3D11BlendState *sums;         // added colours, the let-through multiplied
    void *over_blend;
    ID3D11Texture2D *at, *rt;
    ID3D11RenderTargetView *ar, *rr;
    ID3D11ShaderResourceView *as, *rs;
    UINT w, h;
    LONG passes, draws;             // the pipe's: passes run, batches drawn into the sums
} g_oit = { .force = -1 };
#define OIT_ALPHA_BLEND (1u | D3DBLEND_SRCALPHA << 1 | D3DBLEND_INVSRCALPHA << 5)   // rd3_mk_blend's key
internal int oit_on(void) { return !g_oit.fail && !g_fe.fail && (g_oit.force < 0 ? g_set.unit_transparency : g_oit.force); }
// A texture see-through over much of its face: of its texels that show (alpha 16 or more) a quarter or more half
// see-through (under 230) - a crystal's or a window's; a leaf's or a rail's soft edge is a thin band of its outline
#define OIT_GLASS_SHARE 0.25f
internal int oit_glass(const uint32_t *px, size_t n) {
    size_t shown = 0, half = 0;
    for (size_t i = 0; i < n; i++) {
        uint32_t a = px[i] >> 24;
        if (a >= 16) { shown++; if (a < 230) half++; }
    }
    // (measured on Art of Conquest's: the crystals' and the interface's glass 0.29 .. 1.0, the trees' and the rest 0.08 .. 0.21)
    int glass = shown >= 64 && (float)half >= OIT_GLASS_SHARE * (float)shown;
    return glass;
}
// the batch being recorded (rd3_flush: its draws' entries still in g_rb.w): a glass texture, or a see-through material
internal int oit_batch(void) {
    if (g_ro_cur != RO_UNITS || g_rd3.bkey != OIT_ALPHA_BLEND || !g_rd3.depth_soft || !(g_rd3.vport.MaxDepth > 0)
        || rf_pixels(g_rd3.vc.view) || !oit_on()) return 0;
    // the map's camera only: the interface's 3D pieces - the crowns of the top bar and over the minimap, glass by their
    // texture - have their view at the origin (rd3_prepare) and viewports of their own, outside the scene's sums
    if (g_rd3.vc.view[12] == 0 && g_rd3.vc.view[13] == 0 && g_rd3.vc.view[14] == 0) return 0;
    if (g_rd3.srv_of && g_rd3.srv_of->glass) return 1;
    if (g_rd3.vc.lit[0] && !g_rd3.vc.src[0])
        for (int k = 0; k < g_rb.nw; k++) if (g_rb.w[k].diff[3] < RF_EDGES_SOLID) return 1;
    return 0;
}
// the shaders, the states and the two sums the size of the scene's depth (rtv the scene's target); 0: not now
internal int oit_ready(ID3D11DepthStencilView *dsv) {
    ID3D11Device *dv = g_d3d.dev;
    HRESULT hr = S_OK;
    if (!g_oit.ps) {
        ID3DBlob *code = d3d_compile(k_rd3_shader, sizeof k_rd3_shader - 1, "psoit", "ps_4_0");
        hr = code ? ID3D11Device_CreatePixelShader(dv, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_oit.ps) : E_FAIL;
        D3D_RELEASE(code);
        code = SUCCEEDED(hr) ? d3d_compile(k_rd3_shader, sizeof k_rd3_shader - 1, "poitc", "ps_4_0") : NULL;
        if (SUCCEEDED(hr)) hr = code ? ID3D11Device_CreatePixelShader(dv, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_oit.over) : E_FAIL;
        D3D_RELEASE(code);
        D3D11_BLEND_DESC bd = { .IndependentBlendEnable = TRUE };
        bd.RenderTarget[0] = (D3D11_RENDER_TARGET_BLEND_DESC){ TRUE, D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,
                                                               D3D11_BLEND_ONE, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL };
        bd.RenderTarget[1] = (D3D11_RENDER_TARGET_BLEND_DESC){ TRUE, D3D11_BLEND_ZERO, D3D11_BLEND_INV_SRC_COLOR, D3D11_BLEND_OP_ADD,
                                                               D3D11_BLEND_ZERO, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD, D3D11_COLOR_WRITE_ENABLE_ALL };
        if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBlendState(dv, &bd, &g_oit.sums);
        g_oit.over_blend = rd3_cache(g_rd3.blends, &g_rd3.n_blend, 32, OIT_ALPHA_BLEND, rd3_mk_blend);
        if (FAILED(hr)) { g_oit.fail = 1; rd_fail("the weighted transparency", hr); return 0; }
    }
    ID3D11Resource *zr = NULL;
    ID3D11Texture2D *zt = NULL;
    int ok = 0;
    ID3D11View_GetResource((ID3D11View *)dsv, &zr);
    if (zr && SUCCEEDED(ID3D11Resource_QueryInterface(zr, &IID_ID3D11Texture2D, (void **)&zt))) {
        D3D11_TEXTURE2D_DESC zd;
        ID3D11Texture2D_GetDesc(zt, &zd);
        if (zd.SampleDesc.Count == 1) {
            if (!g_oit.at || g_oit.w != zd.Width || g_oit.h != zd.Height) {
                D3D_RELEASE(g_oit.as); D3D_RELEASE(g_oit.ar); D3D_RELEASE(g_oit.at);
                D3D_RELEASE(g_oit.rs); D3D_RELEASE(g_oit.rr); D3D_RELEASE(g_oit.rt);
                D3D11_TEXTURE2D_DESC nd = { zd.Width, zd.Height, 1, 1, DXGI_FORMAT_R16G16B16A16_FLOAT, { 1, 0 }, D3D11_USAGE_DEFAULT,
                                            D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE, 0, 0 };
                hr = ID3D11Device_CreateTexture2D(dv, &nd, NULL, &g_oit.at);
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateRenderTargetView(dv, (ID3D11Resource *)g_oit.at, NULL, &g_oit.ar);
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(dv, (ID3D11Resource *)g_oit.at, NULL, &g_oit.as);
                nd.Format = DXGI_FORMAT_R16_FLOAT;
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateTexture2D(dv, &nd, NULL, &g_oit.rt);
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateRenderTargetView(dv, (ID3D11Resource *)g_oit.rt, NULL, &g_oit.rr);
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(dv, (ID3D11Resource *)g_oit.rt, NULL, &g_oit.rs);
                g_oit.w = zd.Width; g_oit.h = zd.Height;
                if (FAILED(hr)) { g_oit.fail = 1; rd_fail("the weighted transparency's targets", hr); }
            }
            ok = SUCCEEDED(hr);
        }
    }
    D3D_RELEASE(zt); D3D_RELEASE(zr);
    return ok;
}
// at the soft edges' place in the list (rf_replay, after its batch last): the see-through parts of the batches up to it
// into the sums, the sums over the scene (f: the list's first batch with the map's camera); b, first as for rf_draw -
// everything bound again after. Only the map's own (oit_batch leaves the interface's pieces out; with the crowns in, the
// scene's sums were laid over the crown's corner alone): a batch on another target or viewport, or all of them when the
// sums can't be made, drawn as an edge instead (RF_PART_M_SEE, no depth written)
internal int oit_scene(const rf_cmd *c, const rf_cmd *f) {   // (the rectangle: the ground's depth range is another than the units')
    return c->rtv == f->rtv && c->dsv == f->dsv && c->vport.TopLeftX == f->vport.TopLeftX && c->vport.TopLeftY == f->vport.TopLeftY
        && c->vport.Width == f->vport.Width && c->vport.Height == f->vport.Height;
}
internal void oit_pass(ID3D11DeviceContext *x, int last, const rf_cmd *f, rf_cmd *b, int *first) {
    int ok = f && f->dsv && f->rtv && oit_ready(f->dsv);
    rf_edges_part(x, RF_PART_M_SEE);
    for (int j = 0; j <= last; j++) {
        const rf_cmd *c = &g_rf.cmd[j];
        if (c->oit && !(ok && oit_scene(c, f))) { rf_cmd e = *c; e.depth = e.soft; rf_draw(x, &e, b, first); }
    }
    if (!ok) return;
    static const FLOAT none[4] = { 0, 0, 0, 0 }, all[4] = { 1, 1, 1, 1 };
    ID3D11DeviceContext_ClearRenderTargetView(x, g_oit.ar, none);
    ID3D11DeviceContext_ClearRenderTargetView(x, g_oit.rr, all);
    g_oit_rtv[0] = g_oit.ar, g_oit_rtv[1] = g_oit.rr, g_oit_scene = f->rtv;
    ID3D11DeviceContext_PSSetShader(x, g_oit.ps, NULL, 0);
    *first = 1;
    for (int j = 0; j <= last; j++) {
        const rf_cmd *c = &g_rf.cmd[j];
        if (!c->oit || !oit_scene(c, f)) continue;
        rf_cmd e = *c;
        e.rtv = g_oit.ar; e.depth = e.soft; e.blend = g_oit.sums;
        rf_draw(x, &e, b, first);
        g_oit.draws++;
    }
    // the triangle over the whole target, cut to the scene's viewport
    D3D11_VIEWPORT port = rs_port(&f->vport, f->rtv);
    ID3D11DeviceContext_RSSetViewports(x, 1, &port);
    ID3D11DeviceContext_OMSetRenderTargets(x, 1, &g_oit_scene, NULL);
    ID3D11ShaderResourceView *srv[2] = { g_oit.as, g_oit.rs };
    ID3D11DeviceContext_PSSetShaderResources(x, 16, 2, srv);
    ID3D11DeviceContext_OMSetBlendState(x, g_oit.over_blend, NULL, 0xFFFFFFFF);
    ID3D11DeviceContext_OMSetDepthStencilState(x, NULL, 0);
    ID3D11DeviceContext_RSSetState(x, NULL);
    ID3D11DeviceContext_IASetInputLayout(x, NULL);
    ID3D11DeviceContext_IASetPrimitiveTopology(x, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(x, g_d3d.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(x, g_oit.over, NULL, 0);
    ID3D11DeviceContext_Draw(x, 3, 0);
    srv[0] = srv[1] = NULL;
    ID3D11DeviceContext_PSSetShaderResources(x, 16, 2, srv);
    g_oit_rtv[0] = g_oit_rtv[1] = g_oit_scene = NULL;
    g_oit.passes++;
    rf_bind(x);
    *first = 1;
    b->topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
}
