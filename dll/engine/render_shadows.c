// THE DYNAMIC SHADOWS ([display] shadows = dynamic; docs/renderer.md "Shadows"): the game's shadows are a blurred patch
// a unit (EEROGroundShadows, a quad on the ground); these are cast by the sun from the units', buildings' and trees'
// shapes onto the ground only - the models are low-poly, and light and shade on them would show it. Each draw gets a
// role (rd3_shadow_role): the terrain's draws receive; the units' lit solid draws cast, and a unit standing in a shadow
// is darkened as a whole by one look at its origin's place in the map (SH_TINT - no shading of its own); trees cast
// without the tint (in a forest the trees around would darken them all).
// The ground comes before the units, and its vertices are the game's own transform into the camera's space (its view
// only swaps y and z): their place in the world needs the camera the units are drawn with, this frame's. So both
// passes run when the units are done (rd3_shadow_units_done): the casters' batches, copied as they were drawn, from
// the sun into a depth map; then the ground's batches, copied too, once more over the picture, multiplying it by the
// shadow - depth tested against what is there (a unit in front stays as it is), each pixel once (the pass writes its
// depth a hair nearer: the ground's layers lie on each other), the decals on it darkened with it. No frame of lag.
// The map covers the casters drawn (the game draws what is in sight), seen along the sun: a shadow lies under its
// caster in the map's coordinates.
enum { SH_NONE, SH_RECEIVE, SH_CAST, SH_TINT };
#define SH_SIZE 4096
#define SH_VERTS (512 * 1024)
#define SH_INDICES (SH_VERTS * 3)
#define SH_WORLDS 16384
#define SH_RECORDS 4096
#define SH_GROUND 512
// a batch: its indices; a caster's texture and alpha test, a piece of ground's camera (view x projection, view)
typedef struct { UINT at, n; ID3D11ShaderResourceView *srv; float aref; float vpm[16], view[16], hp[2]; } sh_record;
typedef struct {   // a frame's copied batches
    rb_vertex *v;
    uint32_t *i;
    float (*w)[16];
    int nv, ni, nw, nrec, full, cap_rec;
    sh_record *rec;
} sh_list;
internal struct {
    int on;          // [display] shadows (the pipe's shadows switches it)
    float strength;  // how much of the sun a shadow takes away (1: all but the ambient light)
    int ready;       // 1 made, -1 failed
    ID3D11Texture2D *map;
    ID3D11DepthStencilView *dsv;
    ID3D11ShaderResourceView *srv;
    ID3D11SamplerState *cmp, *samp;
    ID3D11VertexShader *vs, *gvs;
    ID3D11PixelShader *ps, *gps;
    ID3D11Buffer *cb, *gcb, *vb, *ib, *wb;
    ID3D11ShaderResourceView *wsrv;
    ID3D11RasterizerState *raster;
    ID3D11DepthStencilState *depth;
    ID3D11BlendState *mul;
    sh_list cast, ground;
    float lo[3], hi[3];   // the casters' world box
    float sun[3];
    float amb[3], diff[3];   // the units' ambient (global + the light's) and the sun's colour
    float view[16];          // the units' camera this frame
    int have_view;
    // the map: world -> u, v, depth; the colour a shadow leaves; on once one is drawn (the units' tint reads it)
    float shm[16], shk[4], inv_depth;
    int valid;
    LONG passes;
    int last_rec, last_nv, last_ground, last_full;   // the last map's (the pipe's shadows)
    float last_lo[3], last_hi[3];
} g_sh = { .strength = 0.8f, .sun = { 0.08f, 0.75f, -0.65f } };

#define SH_VS_IN \
    "Buffer<float4> wt : register(t1);\n" \
    "struct I { float3 p : POSITION; float3 n : NORMAL; float4 c : COLOR; float2 t : TEXCOORD; uint w : WORLD; };\n" \
    "float4 world_pos(I i) { uint r = i.w * 4; float4 r0 = wt.Load(r); r0.w = 0;" \
    " return mul(float4(i.p, 1), float4x4(r0, wt.Load(r + 1), wt.Load(r + 2), wt.Load(r + 3))); }\n"
// the casters from the sun (sa: alpha reference / 255, alpha tested)
internal const char k_sh_shader[] =
    "cbuffer s : register(b2) { row_major float4x4 shm; float4 sa; };\n"
    SH_VS_IN
    "Texture2D t : register(t0); SamplerState s : register(s0);\n"
    "struct O { float4 p : SV_Position; float2 t : TEXCOORD0; };\n"
    "O vs(I i) { O o; float3 q = mul(world_pos(i), shm).xyz; o.p = float4(q.x * 2 - 1, 1 - q.y * 2, q.z, 1); o.t = i.t;"
    " return o; }\n"
    "void ps(O i) { if (sa.y > 0 && t.Sample(s, i.t).a < sa.x) discard; }\n";
// the ground once more: placed as the main pass placed it (render_3d.c's vertex shader: the same expression), its
// place in the map through the units' camera (tom = the ground's view x the units' view inverted x the map's); the
// pixel the shadow's colour, 3x3 comparisons apart by sp.x; sp.y the depth a hair nearer, sp.z the comparisons' bias
internal const char k_sh_ground[] =
    "cbuffer g : register(b2) { row_major float4x4 vpm; row_major float4x4 tom; float4 shk; float4 vp; float4 sp; };\n"
    SH_VS_IN
    "Texture2D<float> sm : register(t2); SamplerComparisonState sc : register(s1);\n"
    "struct O { float4 p : SV_Position; float3 q : TEXCOORD0; };\n"
    "O vs(I i) { O o; float4 p = world_pos(i); o.p = mul(p, vpm); o.p.xy += vp.xy * o.p.w; o.p.z -= sp.y * o.p.w;"
    " o.q = mul(p, tom).xyz; return o; }\n"
    "float4 ps(O i) : SV_Target { float z = min(i.q.z, 0.9999) - sp.z, l = 0;"
    " for (int y = -1; y <= 1; y++) for (int x = -1; x <= 1; x++) l += sm.SampleCmpLevelZero(sc, i.q.xy + float2(x, y) * sp.x, z);"
    " return float4(lerp(shk.rgb, 1, l / 9), 1); }\n";

internal HRESULT rd3_shadow_shaders(const char *src, size_t n, ID3D11VertexShader **vs, ID3D11PixelShader **ps) {
    ID3DBlob *code = d3d_compile(src, n, "vs", "vs_4_0");
    HRESULT hr = code ? ID3D11Device_CreateVertexShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, vs) : E_FAIL;
    D3D_RELEASE(code);
    code = SUCCEEDED(hr) ? d3d_compile(src, n, "ps", "ps_4_0") : NULL;
    hr = code ? ID3D11Device_CreatePixelShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, ps) : E_FAIL;
    D3D_RELEASE(code);
    return hr;
}
internal int sh_list_init(sh_list *l, int verts, int worlds, int recs) {
    l->v = malloc((size_t)verts * sizeof *l->v);
    l->i = malloc((size_t)verts * 3 * sizeof *l->i);
    l->w = malloc((size_t)worlds * sizeof *l->w);
    l->rec = calloc((size_t)recs, sizeof *l->rec);
    l->cap_rec = recs;
    return l->v && l->i && l->w && l->rec;
}
internal int rd3_shadow_init(void) {
    if (g_sh.ready) return g_sh.ready > 0;
    g_sh.ready = -1;
    ID3D11Device *dv = g_d3d.dev;
    D3D11_TEXTURE2D_DESC td = { SH_SIZE, SH_SIZE, 1, 1, DXGI_FORMAT_R32_TYPELESS, { 1, 0 }, D3D11_USAGE_DEFAULT,
                                D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE, 0, 0 };
    HRESULT hr = ID3D11Device_CreateTexture2D(dv, &td, NULL, &g_sh.map);
    D3D11_DEPTH_STENCIL_VIEW_DESC dd = { .Format = DXGI_FORMAT_D32_FLOAT, .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateDepthStencilView(dv, (ID3D11Resource *)g_sh.map, &dd, &g_sh.dsv);
    D3D11_SHADER_RESOURCE_VIEW_DESC sd = { .Format = DXGI_FORMAT_R32_FLOAT, .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D };
    sd.Texture2D.MipLevels = 1;
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(dv, (ID3D11Resource *)g_sh.map, &sd, &g_sh.srv);
    // outside the map: depth 1 - nothing nearer the sun, lit
    D3D11_SAMPLER_DESC cs = { D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_BORDER,
                              D3D11_TEXTURE_ADDRESS_BORDER, D3D11_TEXTURE_ADDRESS_BORDER, 0, 1, D3D11_COMPARISON_LESS_EQUAL,
                              { 1, 1, 1, 1 }, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateSamplerState(dv, &cs, &g_sh.cmp);
    D3D11_SAMPLER_DESC ss = { D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP,
                              D3D11_TEXTURE_ADDRESS_WRAP, 0, 1, D3D11_COMPARISON_NEVER, { 0 }, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateSamplerState(dv, &ss, &g_sh.samp);
    if (SUCCEEDED(hr)) hr = rd3_shadow_shaders(k_sh_shader, sizeof k_sh_shader - 1, &g_sh.vs, &g_sh.ps);
    if (SUCCEEDED(hr)) hr = rd3_shadow_shaders(k_sh_ground, sizeof k_sh_ground - 1, &g_sh.gvs, &g_sh.gps);
    D3D11_BUFFER_DESC bd = { 80, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(dv, &bd, NULL, &g_sh.cb);
    bd.ByteWidth = 176;
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(dv, &bd, NULL, &g_sh.gcb);
    D3D11_BUFFER_DESC ud = { SH_VERTS * sizeof(rb_vertex), D3D11_USAGE_DYNAMIC, D3D11_BIND_VERTEX_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(dv, &ud, NULL, &g_sh.vb);
    ud.ByteWidth = SH_INDICES * 4; ud.BindFlags = D3D11_BIND_INDEX_BUFFER;
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(dv, &ud, NULL, &g_sh.ib);
    ud.ByteWidth = SH_WORLDS * 64; ud.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(dv, &ud, NULL, &g_sh.wb);
    D3D11_SHADER_RESOURCE_VIEW_DESC wd = { .Format = DXGI_FORMAT_R32G32B32A32_FLOAT, .ViewDimension = D3D11_SRV_DIMENSION_BUFFER };
    wd.Buffer.NumElements = SH_WORLDS * 4;
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(dv, (ID3D11Resource *)g_sh.wb, &wd, &g_sh.wsrv);
    D3D11_RASTERIZER_DESC rs = { D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE, 0, 0, 0, TRUE, FALSE, FALSE, FALSE };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateRasterizerState(dv, &rs, &g_sh.raster);
    D3D11_DEPTH_STENCIL_DESC ds = { TRUE, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_LESS, FALSE, 0, 0, { 0 }, { 0 } };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateDepthStencilState(dv, &ds, &g_sh.depth);
    D3D11_BLEND_DESC bl = { 0 };   // the picture times the shadow's colour, its alpha kept
    bl.RenderTarget[0] = (D3D11_RENDER_TARGET_BLEND_DESC){ TRUE, D3D11_BLEND_ZERO, D3D11_BLEND_SRC_COLOR, D3D11_BLEND_OP_ADD,
                                                            D3D11_BLEND_ZERO, D3D11_BLEND_ONE, D3D11_BLEND_OP_ADD,
                                                            D3D11_COLOR_WRITE_ENABLE_ALL };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBlendState(dv, &bl, &g_sh.mul);
    if (FAILED(hr)) return rd_fail("the dynamic shadows", hr), 0;
    if (!sh_list_init(&g_sh.cast, SH_VERTS, SH_WORLDS, SH_RECORDS) || !sh_list_init(&g_sh.ground, SH_VERTS / 4, SH_WORLDS / 4, SH_GROUND))
        return rd_fail("the dynamic shadows' batches", E_OUTOFMEMORY), 0;
    g_sh.lo[0] = g_sh.lo[1] = g_sh.lo[2] = 1e30f;
    g_sh.hi[0] = g_sh.hi[1] = g_sh.hi[2] = -1e30f;
    g_sh.ready = 1;
    return 1;
}

// A draw's role (SH_), from the render object drawing it and its vertices - the probe's findings (docs/renderer.md):
// the terrain's draws receive; in the units' render object the lit draws that don't lie flat (their vertices' height
// over 1% of their width - the flat ones are decals: roads, fields, craters) cast, those alpha tested at 200 or more
// (trees, lit by a light of their own) without the tint. A caster's world box goes into the frame's, its size (the
// box's diagonal, at least a tile) into d->sh_b: how much nearer the sun than its origin a shadow must be to darken it
// - its own body (a tank's hull over its wheels) must not.
internal int rd3_shadow_role(rd_device *d, rd_vb *vb, DWORD start, DWORD nv) {
    if (!g_sh.on || vb->fvf != 0x152 || !d->rs[D3DRENDERSTATE_ZENABLE] || !rd3_shadow_init()) return SH_NONE;
    if (g_ro_cur == RO_TERRAIN) return SH_RECEIVE;
    if (g_ro_cur != RO_UNITS || !d->rs[D3DRENDERSTATE_LIGHTING]) return SH_NONE;
    float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f };
    for (DWORD k = 0; k < nv; k++) {
        const float *p = (const float *)(vb->mem + (size_t)(start + k) * vb->stride);
        for (int a = 0; a < 3; a++) { if (p[a] < lo[a]) lo[a] = p[a]; if (p[a] > hi[a]) hi[a] = p[a]; }
    }
    if (hi[2] - lo[2] <= 0.01f * fmaxf(hi[0] - lo[0], hi[1] - lo[1])) return SH_NONE;
    const D3DMATRIX *w = &d->world;
    float wl[3] = { 1e30f, 1e30f, 1e30f }, wh[3] = { -1e30f, -1e30f, -1e30f };
    for (int c = 0; c < 8; c++) {
        float x = c & 1 ? hi[0] : lo[0], y = c & 2 ? hi[1] : lo[1], z = c & 4 ? hi[2] : lo[2];
        float q[3] = { x * w->_11 + y * w->_21 + z * w->_31 + w->_41, x * w->_12 + y * w->_22 + z * w->_32 + w->_42,
                       x * w->_13 + y * w->_23 + z * w->_33 + w->_43 };
        for (int a = 0; a < 3; a++) { if (q[a] < wl[a]) wl[a] = q[a]; if (q[a] > wh[a]) wh[a] = q[a]; }
    }
    for (int a = 0; a < 3; a++) { if (wl[a] < g_sh.lo[a]) g_sh.lo[a] = wl[a]; if (wh[a] > g_sh.hi[a]) g_sh.hi[a] = wh[a]; }
    float dx = wh[0] - wl[0], dy = wh[1] - wl[1], dz = wh[2] - wl[2];
    d->sh_b = fmaxf(sqrtf(dx * dx + dy * dy + dz * dz), 1.0f);
    memcpy(g_sh.view, &d->view._11, sizeof g_sh.view);
    g_sh.have_view = 1;
    int tree = d->rs[D3DRENDERSTATE_ALPHATESTENABLE] && (d->rs[D3DRENDERSTATE_ALPHAREF] & 255) >= 200;
    const D3DLIGHT7 *l = &d->light[0];
    if (!tree && d->light_on[0] && l->dltType == D3DLIGHT_DIRECTIONAL && l->dvDirection.z < -0.05f) {   // the units' sun
        float n = sqrtf(l->dvDirection.x * l->dvDirection.x + l->dvDirection.y * l->dvDirection.y + l->dvDirection.z * l->dvDirection.z);
        g_sh.sun[0] = l->dvDirection.x / n; g_sh.sun[1] = l->dvDirection.y / n; g_sh.sun[2] = l->dvDirection.z / n;
        float ga[4];
        rd3_color(ga, d->rs[D3DRENDERSTATE_AMBIENT]);
        g_sh.amb[0] = ga[0] + l->dcvAmbient.r; g_sh.amb[1] = ga[1] + l->dcvAmbient.g; g_sh.amb[2] = ga[2] + l->dcvAmbient.b;
        g_sh.diff[0] = l->dcvDiffuse.r; g_sh.diff[1] = l->dcvDiffuse.g; g_sh.diff[2] = l->dcvDiffuse.b;
    }
    return tree ? SH_CAST : SH_TINT;
}
// the draw's role on the device: a change is a change of the vertex constants (a new batch)
internal void rd3_shadow_classify(rd_device *d, rd_vb *vb, DWORD start, DWORD nv) {
    int role = rd3_shadow_role(d, vb, start, nv);
    if (role != d->sh_role) { d->sh_role = role; d->changed |= RD_CH_VC; }
}

// a batch of the frame's casters or ground (rd3_flush, before its upload: its world indices still its own) copied
internal void rd3_shadow_capture(int role) {
    sh_list *l = role == SH_RECEIVE ? &g_sh.ground : &g_sh.cast;
    int cap_v = l == &g_sh.cast ? SH_VERTS : SH_VERTS / 4, cap_w = l == &g_sh.cast ? SH_WORLDS : SH_WORLDS / 4;
    if (g_rb.topo != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST || g_sh.ready <= 0 || l->full) return;
    if (l->nv + g_rb.nv > cap_v || l->ni + g_rb.ni > cap_v * 3 || l->nw + g_rb.nw > cap_w || l->nrec >= l->cap_rec) {
        l->full = 1;   // the rest of the frame's batches left out (the pipe's shadows tells)
        return;
    }
    memcpy(l->w + l->nw, g_rb.w, (size_t)g_rb.nw * sizeof *g_rb.w);
    for (int k = 0; k < g_rb.nv; k++) { l->v[l->nv + k] = g_rb.v[k]; l->v[l->nv + k].w += (uint32_t)l->nw; }
    for (int k = 0; k < g_rb.ni; k++) l->i[l->ni + k] = g_rb.i[k] + (uint32_t)l->nv;
    sh_record *r = &l->rec[l->nrec++];
    r->at = (UINT)l->ni; r->n = (UINT)g_rb.ni;
    if (l == &g_sh.cast) {
        r->srv = g_rd3.srv;
        if (r->srv) ID3D11ShaderResourceView_AddRef(r->srv);   // a texture freed before the pass
        r->aref = g_rd3.pc.aop[3] ? g_rd3.pc.misc[0] / 255.0f : -1;
    } else {
        memcpy(r->vpm, g_rd3.vc.vpm, sizeof r->vpm);
        memcpy(r->view, g_rd3.vc.view, sizeof r->view);
        r->hp[0] = g_rd3.vc.vp[0]; r->hp[1] = g_rd3.vc.vp[1];
    }
    l->nv += g_rb.nv; l->ni += g_rb.ni; l->nw += g_rb.nw;
}

internal int rd3_shadow_upload(const sh_list *l) {
    ID3D11Buffer *b[3] = { g_sh.vb, g_sh.ib, g_sh.wb };
    const void *src[3] = { l->v, l->i, l->w };
    size_t n[3] = { (size_t)l->nv * sizeof *l->v, (size_t)l->ni * 4, (size_t)l->nw * 64 };
    for (int k = 0; k < 3; k++) {
        D3D11_MAPPED_SUBRESOURCE ms;
        if (FAILED(ID3D11DeviceContext_Map(g_d3d.ctx, (ID3D11Resource *)b[k], 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) return 0;
        memcpy(ms.pData, src[k], n[k]);
        ID3D11DeviceContext_Unmap(g_d3d.ctx, (ID3D11Resource *)b[k], 0);
    }
    return 1;
}
internal void rd3_shadow_reset(void) {
    for (int k = 0; k < g_sh.cast.nrec; k++) if (g_sh.cast.rec[k].srv) ID3D11ShaderResourceView_Release(g_sh.cast.rec[k].srv);
    g_sh.cast.nv = g_sh.cast.ni = g_sh.cast.nw = g_sh.cast.nrec = g_sh.cast.full = 0;
    g_sh.ground.nv = g_sh.ground.ni = g_sh.ground.nw = g_sh.ground.nrec = g_sh.ground.full = 0;
    g_sh.lo[0] = g_sh.lo[1] = g_sh.lo[2] = 1e30f;
    g_sh.hi[0] = g_sh.hi[1] = g_sh.hi[2] = -1e30f;
    g_sh.have_view = 0;
}
// a rigid 4x4 (rotation, translation; row vectors) inverted
internal void rd3_rigid_inverse(float *r, const float *m) {
    memset(r, 0, 64);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) r[i * 4 + j] = m[j * 4 + i];
    for (int j = 0; j < 3; j++) r[12 + j] = -(m[12] * r[j] + m[13] * r[4 + j] + m[14] * r[8 + j]);
    r[15] = 1;
}
// the pipeline's states for a pass of its own (the main pass binds its own again: rd3_dirty)
internal void rd3_shadow_states(ID3D11VertexShader *vs, ID3D11PixelShader *ps, ID3D11Buffer *cb) {
    ID3D11DeviceContext *x = g_d3d.ctx;
    ID3D11DeviceContext_RSSetState(x, g_sh.raster);
    ID3D11DeviceContext_IASetInputLayout(x, g_rd3.inputs);
    UINT stride = sizeof(rb_vertex), off = 0;
    ID3D11DeviceContext_IASetVertexBuffers(x, 0, 1, &g_sh.vb, &stride, &off);
    ID3D11DeviceContext_IASetIndexBuffer(x, g_sh.ib, DXGI_FORMAT_R32_UINT, 0);
    ID3D11DeviceContext_IASetPrimitiveTopology(x, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(x, vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(x, ps, NULL, 0);
    ID3D11DeviceContext_VSSetShaderResources(x, 1, 1, &g_sh.wsrv);
    ID3D11DeviceContext_VSSetConstantBuffers(x, 2, 1, &cb);
    ID3D11DeviceContext_PSSetConstantBuffers(x, 2, 1, &cb);
}
// the casters from the sun into the map
internal int rd3_shadow_map(void) {
    // the map's axes: r and u across the sun's rays, f along them (depth: farther from the sun)
    const float *f = g_sh.sun;
    float r[3] = { f[1], -f[0], 0 };   // f x up
    float rl = sqrtf(r[0] * r[0] + r[1] * r[1]);
    if (rl < 1e-4f) { r[0] = 1; r[1] = 0; } else { r[0] /= rl; r[1] /= rl; }
    float u[3] = { r[1] * f[2] - r[2] * f[1], r[2] * f[0] - r[0] * f[2], r[0] * f[1] - r[1] * f[0] };
    float mn[3] = { 1e30f, 1e30f, 1e30f }, mx[3] = { -1e30f, -1e30f, -1e30f };
    for (int c = 0; c < 8; c++) {
        float p[3] = { c & 1 ? g_sh.hi[0] : g_sh.lo[0], c & 2 ? g_sh.hi[1] : g_sh.lo[1], c & 4 ? g_sh.hi[2] : g_sh.lo[2] };
        float q[3] = { p[0] * r[0] + p[1] * r[1] + p[2] * r[2], p[0] * u[0] + p[1] * u[1] + p[2] * u[2],
                       p[0] * f[0] + p[1] * f[1] + p[2] * f[2] };
        for (int a = 0; a < 3; a++) { if (q[a] < mn[a]) mn[a] = q[a]; if (q[a] > mx[a]) mx[a] = q[a]; }
    }
    for (int a = 0; a < 3; a++) { mn[a] -= 1; mx[a] += 1; }   // a tile around, the comparisons' reach
    float wx = mx[0] - mn[0], wy = mx[1] - mn[1], wz = mx[2] - mn[2];
    float m[16] = { r[0] / wx, -u[0] / wy, f[0] / wz, 0, r[1] / wx, -u[1] / wy, f[1] / wz, 0,
                    r[2] / wx, -u[2] / wy, f[2] / wz, 0, -mn[0] / wx, mx[1] / wy, -mn[2] / wz, 1 };
    if (!rd3_shadow_upload(&g_sh.cast)) return 0;
    ID3D11DeviceContext *x = g_d3d.ctx;
    ID3D11ShaderResourceView *none = NULL;
    ID3D11DeviceContext_VSSetShaderResources(x, 2, 1, &none);   // the map is the target now
    ID3D11DeviceContext_PSSetShaderResources(x, 2, 1, &none);
    ID3D11DeviceContext_OMSetRenderTargets(x, 0, NULL, g_sh.dsv);
    ID3D11DeviceContext_ClearDepthStencilView(x, g_sh.dsv, D3D11_CLEAR_DEPTH, 1, 0);
    D3D11_VIEWPORT vp = { 0, 0, SH_SIZE, SH_SIZE, 0, 1 };
    ID3D11DeviceContext_RSSetViewports(x, 1, &vp);
    ID3D11DeviceContext_OMSetDepthStencilState(x, g_sh.depth, 0);
    ID3D11DeviceContext_OMSetBlendState(x, NULL, NULL, 0xFFFFFFFF);
    rd3_shadow_states(g_sh.vs, g_sh.ps, g_sh.cb);
    ID3D11DeviceContext_PSSetSamplers(x, 0, 1, &g_sh.samp);
    float cb[20];
    memcpy(cb, m, sizeof m);
    cb[18] = cb[19] = 0;
    float aref = -2;
    for (int k = 0; k < g_sh.cast.nrec; k++) {
        sh_record *e = &g_sh.cast.rec[k];
        if (e->aref != aref) {
            aref = e->aref;
            cb[16] = aref; cb[17] = aref >= 0 && e->srv ? 1.0f : 0.0f;
            ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_sh.cb, 0, NULL, cb, 0, 0);
        }
        ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &e->srv);
        ID3D11DeviceContext_DrawIndexed(x, e->n, e->at, 0);
    }
    // the colour a shadow leaves: the ambient light over the ambient and the sun on level ground, as much of it as the
    // strength says (1: the sun all gone)
    memcpy(g_sh.shm, m, sizeof m);
    float sun = fmaxf(0, -f[2]);
    for (int a = 0; a < 3; a++) {
        float lit = g_sh.amb[a] + g_sh.diff[a] * sun;
        float k = lit > 0 ? fminf(1, g_sh.amb[a] / lit) : 1;
        g_sh.shk[a] = 1 - g_sh.strength * (1 - k);
    }
    g_sh.shk[3] = g_sh.strength;
    g_sh.inv_depth = 1 / wz;
    return 1;
}
// the ground once more, times the shadow, into the picture (the main pass's target and depth, bound)
internal void rd3_shadow_ground(void) {
    ID3D11DeviceContext *x = g_d3d.ctx;
    if (!g_sh.ground.nrec || !g_rd3.rtv || !g_rd3.dsv || !rd3_shadow_upload(&g_sh.ground)) return;
    ID3D11DeviceContext_OMSetRenderTargets(x, 1, &g_rd3.rtv, g_rd3.dsv);
    ID3D11DeviceContext_RSSetViewports(x, 1, &g_rd3.vport);
    ID3D11DeviceContext_OMSetDepthStencilState(x, g_sh.depth, 0);
    ID3D11DeviceContext_OMSetBlendState(x, g_sh.mul, NULL, 0xFFFFFFFF);
    rd3_shadow_states(g_sh.gvs, g_sh.gps, g_sh.gcb);
    ID3D11DeviceContext_PSSetShaderResources(x, 2, 1, &g_sh.srv);
    ID3D11DeviceContext_PSSetSamplers(x, 1, 1, &g_sh.cmp);
    float iv[16], vm[16];   // the units' camera inverted, x the map's: camera space -> the map
    rd3_rigid_inverse(iv, g_sh.view);
    rd3_mul(vm, iv, g_sh.shm);
    float cb[44];
    memcpy(cb + 32, g_sh.shk, 16);
    cb[40] = 1.5f / SH_SIZE;   // the comparisons' spacing, the depth a hair nearer, the comparisons' bias
    cb[41] = 2e-6f;
    cb[42] = 0.002f;
    cb[43] = 0;
    for (int k = 0; k < g_sh.ground.nrec; k++) {
        sh_record *e = &g_sh.ground.rec[k];
        memcpy(cb, e->vpm, 64);
        rd3_mul(cb + 16, e->view, vm);
        cb[36] = e->hp[0]; cb[37] = e->hp[1]; cb[38] = cb[39] = 0;
        ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_sh.gcb, 0, NULL, cb, 0, 0);
        ID3D11DeviceContext_DrawIndexed(x, e->n, e->at, 0);
    }
}
// the units drawn (the hook of their render object): this frame's casters into the map, the ground darkened by it
internal void rd3_shadow_units_done(void) {
    rd_device *d = g_rd3.prep_dev;
    if (!g_sh.on || g_sh.ready <= 0 || !d) return;
    rd3_flush();
    if (g_sh.cast.nrec && g_sh.have_view && rd3_shadow_map()) {
        g_sh.last_rec = g_sh.cast.nrec; g_sh.last_nv = g_sh.cast.nv; g_sh.last_full = g_sh.cast.full || g_sh.ground.full;
        g_sh.last_ground = g_sh.ground.nrec;
        memcpy(g_sh.last_lo, g_sh.lo, sizeof g_sh.lo); memcpy(g_sh.last_hi, g_sh.hi, sizeof g_sh.hi);
        rd3_shadow_ground();
        g_sh.valid = 1;
        g_sh.passes++;
        d->changed |= RD_CH_VC;   // the units' tint: the new map's matrix
    }
    rd3_shadow_reset();
    rd3_dirty();
}
// the shadow map for the units' tint (rd3_prepare binds it with the rest)
internal void rd3_shadow_bind(ID3D11DeviceContext *x) {
    ID3D11ShaderResourceView *srv = g_sh.ready > 0 ? g_sh.srv : NULL;
    ID3D11DeviceContext_VSSetShaderResources(x, 2, 1, &srv);
}
// the shadows' part in the vertex shader's constants
internal void rd3_shadow_constants(rd_device *d, rd3_vcb *vc) {
    memcpy(vc->shm, g_sh.shm, sizeof vc->shm);
    memcpy(vc->shk, g_sh.shk, sizeof vc->shk);
    vc->shp[0] = (float)d->sh_role;
    vc->shp[1] = SH_SIZE;
    vc->shp[2] = g_sh.on && g_sh.valid ? 1.0f : 0.0f;
    vc->shp[3] = g_sh.inv_depth;
}

// shadows [game | dynamic] [strength]: the shadows switched while playing (the setting's at the start); the strength
// how much of the sun a shadow takes (0..1, default 0.8)
internal void cmd_shadows(char *out, const char *args) {
    char mode[16] = "";
    float s = -1;
    sscanf(args, "%15s %f", mode, &s);
    if (!strcmp(mode, "game")) g_sh.on = 0;
    else if (!strcmp(mode, "dynamic")) g_sh.on = 1;
    else if (mode[0]) { REPLY("error: usage: shadows [game | dynamic] [strength 0..1]\n"); return; }
    if (s >= 0 && s <= 1) g_sh.strength = s;
    REPLY("shadows: %s, strength %.2f, %s, %ld maps drawn; the last: %d casting batches (%d vertices), %d of ground%s;"
          " the casters' box %.1f %.1f %.1f - %.1f %.1f %.1f, the sun %.2f %.2f %.2f, a shadow's colour %.2f %.2f %.2f\n",
          g_sh.on ? "dynamic" : "game", (double)g_sh.strength,
          g_sh.ready > 0 ? "ready" : g_sh.ready < 0 ? "FAILED (the log says why)" : "not made yet", (long)g_sh.passes,
          g_sh.last_rec, g_sh.last_nv, g_sh.last_ground, g_sh.last_full ? " (some left out: full)" : "",
          (double)g_sh.last_lo[0], (double)g_sh.last_lo[1], (double)g_sh.last_lo[2], (double)g_sh.last_hi[0],
          (double)g_sh.last_hi[1], (double)g_sh.last_hi[2], (double)g_sh.sun[0], (double)g_sh.sun[1],
          (double)g_sh.sun[2], (double)g_sh.shk[0], (double)g_sh.shk[1], (double)g_sh.shk[2]);
}
