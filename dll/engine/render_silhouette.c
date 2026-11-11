// THE SILHOUETTES ([display] unit_silhouettes; the pipe: picture sil on | off): the game makes a whole building see-through
// when something stands behind it (the player, 2026-11-09: "im not a fan of how units behind buildings turn the whole building
// transparent", and of the choices "silhouettes in player color", "use the games simple lighting"). The building stays whole
// (collect.c uc_opacity); a unit behind it is drawn over it once more in its player's colour, lit as it is.
// Only where a building is what the pixel shows and the unit lies behind that: a unit's own front, the unit before it,
// hills and trees show nothing through. The depth can't tell who wrote it, so the frame's list draws the buildings alone
// into a depth of their own, and the unit's pixel compares itself with that and that with the picture's.
// The units go into a picture of their own, solid and with a depth, and that over the scene see-through as one (the player,
// 2026-11-09: "rendering the silluetes transparent makes the shape hard to read since depth gets thrown out"): an arm before
// the body stays before it, instead of every layer of the model blending in the order of its triangles.
// They fade with the distance behind the face they appear on (the player, 2026-11-10: "is it possible to fade silluetes outt
// with distance to the face they apear on?"; [display] silhouette_fade_start, silhouette_fade_end, the pipe: picture silfade):
// the depths of the unit's pixel and of the building's there, turned back into distances along the view.
// The fill knows a record's kind (sil_tag), the draw only the record: the game's draw of a record is hooked to say whose
// draws follow, and a batch holds one kind (sil_batch); its player's colour is per draw, the entry's free number. A cloaked
// unit is a kind of its own: no silhouette and no shadow (render_shadows.c), either would show where it is.
enum { SIL_UNIT = 1, SIL_BUILDING = 2, SIL_CLOAKED = 3 };
#define SIL_TABLE (1u << 16)
typedef struct { const void *rec; uint32_t gen; int tag; } sil_e;
internal sil_e g_sil_t[SIL_TABLE];
internal uint32_t g_sil_gen;
internal struct {
    int force, fail, tag, in;   // the pipe's (-1: the ini's); the record being drawn
    int ship;                   // ... is a ship's (render_instance.c fills its hull's open bottom)
    float col;                  // its player's colour, 0xRRGGBB as a whole number
    float fs, fe;               // the pipe's fade: the tiles behind the face it starts and ends (-1: the ini's)
    ID3D11Buffer *cb;           // the fade's constants for the shader
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps, *over;   // the units' into their picture, that picture over the scene
    void *blend, *solid;
    ID3D11DepthStencilState *depth;
    ID3D11Texture2D *zt, *bt, *ct, *ut;   // the picture's depth copied, the buildings' depth, the units' picture and its depth
    ID3D11ShaderResourceView *zs, *bs, *cs;
    ID3D11DepthStencilView *bd, *ud;
    ID3D11RenderTargetView *cr;
    UINT w, h;
} g_sl = { .force = -1, .fs = -1, .fe = -1 };
internal int sil_on(void) { return g_sl.force < 0 ? g_set.unit_silhouettes : g_sl.force; }
internal uint32_t sil_hash(const void *rec) { return ((uint32_t)(uintptr_t)rec >> 3) * 2654435761u & (SIL_TABLE - 1); }
// the fill: this walk's record is of this kind (0: of neither)
internal void sil_tag(const void *rec, uint32_t gen, int tag) {
    g_sil_gen = gen;
    if (!tag) return;
    uint32_t h = sil_hash(rec);
    for (int n = 0; n < 16; n++, h = (h + 1) & (SIL_TABLE - 1))
        if (g_sil_t[h].rec == rec || g_sil_t[h].gen != gen) { g_sil_t[h].rec = rec, g_sil_t[h].gen = gen, g_sil_t[h].tag = tag; return; }
}
internal int sil_of(const void *rec) {
    uint32_t h = sil_hash(rec);
    for (int n = 0; n < 16; n++, h = (h + 1) & (SIL_TABLE - 1)) {
        if (g_sil_t[h].gen != g_sil_gen) return 0;
        if (g_sil_t[h].rec == rec) return g_sil_t[h].tag;
    }
    return 0;
}
internal uint32_t sil_byte(float v) { return v <= 0.0f ? 0 : v >= 1.0f ? 255 : (uint32_t)(v * 255.0f); }
internal int ud_record(ee_ro_units *ro, void *rec);   // render_units.c: the mod's own record draw
internal void __thiscall sil_hook_draw(ee_ro_units *ro, void *rec) {
    int kind = sil_of(rec);
    g_sh_cloaked = kind == SIL_CLOAKED;
    g_sl.tag = sil_on() && kind != SIL_CLOAKED ? kind : 0;
    g_sl.in = 1;
    g_sl.ship = class_is_ship(draw_record_get_class_id(rec));
    if (g_sl.tag == SIL_UNIT) {
        vec3 c = draw_record_get_color(rec);
        g_sl.col = (float)(sil_byte(c.x) << 16 | sil_byte(c.y) << 8 | sil_byte(c.z));
    }
    if (!ud_record(ro, rec)) ro_units_draw_record(ro, rec);
    g_sl.in = g_sl.ship = 0;
    g_sh_cloaked = 0;
}
// a tree's record: it hides what stands behind it as a building does (and render_instance.c draws it)
internal int ut_record(ee_ro_units *ro, float *at, void *rec);   // render_units.c: the mod's own tree draw
internal void __thiscall sil_hook_tree(ee_ro_units *ro, void *at, void *rec) {
    g_sl.tag = sil_on() ? SIL_BUILDING : 0;
    g_sl.in = 1;
    if (!ut_record(ro, at, rec)) ro_units_draw_tree(ro, at, rec);
    g_sl.in = 0;
}
internal int sil_install(void) {
    static int done;
    if (done || (!g_set.unit_silhouettes && !g_set.unit_instancing && g_set.shadows != 1)) return done;   // (render_instance.c: a unit's record; render_shadows.c: a cloaked one's)
    return done = patch_units_draw_record(sil_hook_draw, NULL) + patch_units_draw_tree(sil_hook_tree);
}
// a draw's batch holds one kind: the batch so far ended when the kind changes
internal void rd3_flush(void);
internal inline void sil_batch(void) {
    int t = g_sl.in ? g_sl.tag : 0;
    if (t != g_rd3.sil) { rd3_flush(); g_rd3.sil = t; }
}
internal inline float sil_colour(void) { return g_sl.in && g_sl.tag == SIL_UNIT ? g_sl.col : 0.0f; }

internal int sil_ready(ID3D11DeviceContext *x, ID3D11DepthStencilView *dsv) {
    ID3D11Device *dv = g_d3d.dev;
    HRESULT hr = S_OK;
    if (!g_sl.ps) {
        ID3DBlob *code = d3d_compile(k_rd3_shader, sizeof k_rd3_shader - 1, "vsil", "vs_4_0");
        hr = code ? ID3D11Device_CreateVertexShader(dv, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_sl.vs) : E_FAIL;
        D3D_RELEASE(code);
        code = SUCCEEDED(hr) ? d3d_compile(k_rd3_shader, sizeof k_rd3_shader - 1, "psil", "ps_4_0") : NULL;
        if (SUCCEEDED(hr)) hr = code ? ID3D11Device_CreatePixelShader(dv, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_sl.ps) : E_FAIL;
        D3D_RELEASE(code);
        code = SUCCEEDED(hr) ? d3d_compile(k_rd3_shader, sizeof k_rd3_shader - 1, "pslc", "ps_4_0") : NULL;
        if (SUCCEEDED(hr)) hr = code ? ID3D11Device_CreatePixelShader(dv, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_sl.over) : E_FAIL;
        D3D_RELEASE(code);
        D3D11_DEPTH_STENCIL_DESC ds = { .DepthEnable = TRUE, .DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL, .DepthFunc = D3D11_COMPARISON_LESS_EQUAL };
        if (SUCCEEDED(hr)) hr = ID3D11Device_CreateDepthStencilState(dv, &ds, &g_sl.depth);
        D3D11_BUFFER_DESC fb = { 16, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
        if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(dv, &fb, NULL, &g_sl.cb);
        g_sl.blend = rd3_cache(g_rd3.blends, &g_rd3.n_blend, 32, 1 | D3DBLEND_SRCALPHA << 1 | D3DBLEND_INVSRCALPHA << 5, rd3_mk_blend);
        g_sl.solid = rd3_cache(g_rd3.blends, &g_rd3.n_blend, 32, 0, rd3_mk_blend);
    }
    ID3D11Resource *zr = NULL;
    ID3D11Texture2D *zt = NULL;
    int ok = 0;
    ID3D11View_GetResource((ID3D11View *)dsv, &zr);
    if (SUCCEEDED(hr) && zr && SUCCEEDED(ID3D11Resource_QueryInterface(zr, &IID_ID3D11Texture2D, (void **)&zt))) {
        D3D11_TEXTURE2D_DESC zd;
        ID3D11Texture2D_GetDesc(zt, &zd);
        if (zd.SampleDesc.Count == 1 && zd.Format == DXGI_FORMAT_D32_FLOAT) {
            if (!g_sl.zt || g_sl.w != zd.Width || g_sl.h != zd.Height) {
                D3D_RELEASE(g_sl.bd); D3D_RELEASE(g_sl.bs); D3D_RELEASE(g_sl.zs); D3D_RELEASE(g_sl.bt); D3D_RELEASE(g_sl.zt);
                D3D_RELEASE(g_sl.cr); D3D_RELEASE(g_sl.cs); D3D_RELEASE(g_sl.ct); D3D_RELEASE(g_sl.ud); D3D_RELEASE(g_sl.ut);
                D3D11_TEXTURE2D_DESC nd = { zd.Width, zd.Height, 1, 1, DXGI_FORMAT_R32_TYPELESS, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
                D3D11_SHADER_RESOURCE_VIEW_DESC sv = { .Format = DXGI_FORMAT_R32_FLOAT, .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D, .Texture2D = { 0, 1 } };
                D3D11_DEPTH_STENCIL_VIEW_DESC dv2 = { .Format = DXGI_FORMAT_D32_FLOAT, .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D };
                hr = ID3D11Device_CreateTexture2D(dv, &nd, NULL, &g_sl.zt);
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(dv, (ID3D11Resource *)g_sl.zt, &sv, &g_sl.zs);
                nd.BindFlags |= D3D11_BIND_DEPTH_STENCIL;
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateTexture2D(dv, &nd, NULL, &g_sl.bt);
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(dv, (ID3D11Resource *)g_sl.bt, &sv, &g_sl.bs);
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateDepthStencilView(dv, (ID3D11Resource *)g_sl.bt, &dv2, &g_sl.bd);
                nd.BindFlags = D3D11_BIND_DEPTH_STENCIL; nd.Format = DXGI_FORMAT_D32_FLOAT;
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateTexture2D(dv, &nd, NULL, &g_sl.ut);
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateDepthStencilView(dv, (ID3D11Resource *)g_sl.ut, &dv2, &g_sl.ud);
                nd.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE; nd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateTexture2D(dv, &nd, NULL, &g_sl.ct);
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateRenderTargetView(dv, (ID3D11Resource *)g_sl.ct, NULL, &g_sl.cr);
                if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(dv, (ID3D11Resource *)g_sl.ct, NULL, &g_sl.cs);
                g_sl.w = zd.Width; g_sl.h = zd.Height;
            }
            if (SUCCEEDED(hr)) { ID3D11DeviceContext_CopySubresourceRegion(x, (ID3D11Resource *)g_sl.zt, 0, 0, 0, 0, zr, 0, NULL); ok = 1; }
        }
    }
    D3D_RELEASE(zt); D3D_RELEASE(zr);
    if (FAILED(hr)) { g_sl.fail = 1; rd_fail("the silhouettes", hr); return 0; }
    return ok;
}
// after the list's last batch of the units (rf_replay): b, first as for rf_draw - everything bound again after
internal void rd3_silhouettes(ID3D11DeviceContext *x, rf_cmd *b, int *first) {
    if (!sil_on() || g_sl.fail) return;
    const rf_cmd *u = NULL, *bl = NULL;
    for (int i = 0; i < g_rf.n && !(u && bl); i++) {
        if (g_rf.cmd[i].sil == SIL_UNIT && !u) u = &g_rf.cmd[i];
        if (g_rf.cmd[i].sil == SIL_BUILDING && !bl) bl = &g_rf.cmd[i];
    }
    if (!u || !bl || !bl->dsv || !sil_ready(x, bl->dsv)) return;
    ID3D11DeviceContext_ClearDepthStencilView(x, g_sl.bd, D3D11_CLEAR_DEPTH, 1.0f, 0);
    for (int i = 0; i < g_rf.n; i++) {
        if (g_rf.cmd[i].sil != SIL_BUILDING) continue;
        rf_cmd e = g_rf.cmd[i];
        e.rtv = NULL; e.dsv = g_sl.bd;
        rf_draw(x, &e, b, first);
    }
    static const FLOAT none[4] = { 0, 0, 0, 0 };
    ID3D11DeviceContext_ClearRenderTargetView(x, g_sl.cr, none);
    ID3D11DeviceContext_ClearDepthStencilView(x, g_sl.ud, D3D11_CLEAR_DEPTH, 1.0f, 0);
    ID3D11DeviceContext_OMSetRenderTargets(x, 1, &g_sl.cr, g_sl.ud);
    b->rtv = g_sl.cr; b->dsv = g_sl.ud;
    // the depth as the view's distance: depth = a + b / w, w a multiple of the distance along the view (the third column of
    // the view x projection against the fourth; the camera's place, which the frame moves, falls out of b)
    float m[16];
    memcpy(m, g_rf.vcs[u->vc].vpm, sizeof m);
    rf_depth_vpm(m);   // the depth as the frame's draws wrote it (render_frame.c)
    int ka = 0;
    for (int j = 1; j < 3; j++) if (fabsf(m[j * 4 + 3]) > fabsf(m[ka * 4 + 3])) ka = j;
    float wl = sqrtf(m[3] * m[3] + m[7] * m[7] + m[11] * m[11]), za = fabsf(m[ka * 4 + 3]) > 1e-6f ? m[ka * 4 + 2] / m[ka * 4 + 3] : 0;
    float fs = g_sl.fs >= 0 ? g_sl.fs : (float)g_set.silhouette_fade_start, fe = g_sl.fe >= 0 ? g_sl.fe : (float)g_set.silhouette_fade_end;
    float fade[4] = { za, wl > 1e-6f ? (m[14] - za * m[15]) / wl : 0, fs, wl > 1e-6f ? fe : 0 };   // (no perspective: no fade)
    ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_sl.cb, 0, NULL, fade, 0, 0);
    ID3D11DeviceContext_PSSetConstantBuffers(x, 4, 1, &g_sl.cb);
    ID3D11ShaderResourceView *srv[3] = { g_sl.bs, g_sl.zs, NULL };
    ID3D11DeviceContext_PSSetShaderResources(x, 10, 2, srv);
    ID3D11DeviceContext_VSSetShader(x, g_sl.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(x, g_sl.ps, NULL, 0);
    for (int i = 0; i < g_rf.n; i++) {
        if (g_rf.cmd[i].sil != SIL_UNIT) continue;
        rf_cmd e = g_rf.cmd[i];
        e.rtv = g_sl.cr; e.dsv = g_sl.ud; e.depth = g_sl.depth; e.blend = g_sl.solid;
        rf_draw(x, &e, b, first);
    }
    // the viewport stays the units': the triangle over the whole target is cut to it
    ID3D11DeviceContext_OMSetRenderTargets(x, 1, &u->rtv, NULL);
    srv[0] = srv[1] = NULL; srv[2] = g_sl.cs;
    ID3D11DeviceContext_PSSetShaderResources(x, 10, 3, srv);
    ID3D11DeviceContext_OMSetBlendState(x, g_sl.blend, NULL, 0xFFFFFFFF);
    ID3D11DeviceContext_OMSetDepthStencilState(x, NULL, 0);
    ID3D11DeviceContext_RSSetState(x, NULL);
    ID3D11DeviceContext_IASetInputLayout(x, NULL);
    ID3D11DeviceContext_IASetPrimitiveTopology(x, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(x, g_d3d.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(x, g_sl.over, NULL, 0);
    ID3D11DeviceContext_Draw(x, 3, 0);
    srv[2] = NULL;
    ID3D11DeviceContext_PSSetShaderResources(x, 10, 3, srv);
    rf_bind(x);
    *first = 1;
    b->topo = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
}
