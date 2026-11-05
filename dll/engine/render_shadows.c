// THE DYNAMIC SHADOWS ([display] shadows = dynamic; docs/renderer.md "Shadows"): the game's shadows are a blurred patch
// a unit (EEROGroundShadows, a quad on the ground); these are cast by the sun from the units', buildings' and trees'
// shapes onto the ground only - the models are low-poly, and light and shade on them would show it. Each draw gets a
// role (rd3_shadow_role): the terrain's draws receive; the units' lit solid draws cast, and a unit standing in a shadow
// is darkened as a whole by one look at its origin's place in the map (SH_TINT - no shading of its own; off unless the
// pipe's shadows tint on: it came across as flicker, units darkening and brightening as they move); trees cast
// without the tint (in a forest the trees around would darken them all); the decals lying on the ground (roads,
// craters - in the units' render object; the farms' fields - in the game's shadows') and the water's surface receive;
// the game's blobs are not drawn. Under the water a shadow fades out with the depth: gone at g_sh.fade below the level
// (the deepest seabed measured: 0.84 under it, the water at 0 - "choke wolf", 2026-11-04).
// The frame is recorded (render_frame.c): before its list is drawn, the casters in it are drawn from the sun into a
// depth map fitted to their box, and the ground into a second one (what the hills hide from the sun) - this frame's,
// from the batches as they lie in the upload buffers (rd3_shadow_frame). The terrain and the decals then receive in the
// main pixel shader. The ground's vertices are the game's own transform into the camera's space (its view only swaps
// y and z): their place in the world needs the camera the units are drawn with, which comes later in the frame - known
// when the list is drawn (rd3_shadow_vc).
enum { SH_NONE, SH_RECEIVE, SH_CAST, SH_TINT, SH_DECAL, SH_BLOB, SH_WATER, SH_AWAY };   // SH_AWAY: casts, off the screen
#define SH_SIZE 4096
internal struct {
    int on;          // [display] shadows (the pipe's shadows switches it)
    float strength;  // how much of the sun a shadow takes away (1: all but the ambient light)
    float fade;      // the depth under the water by which a shadow is gone
    int tint;        // a unit in a shadow darkened (SH_TINT): off - it came across as flicker (the player, 2026-11-05)
    double water_z;  // the water's vertices' heights this frame, summed, and how many (its level)
    int water_n;
    float level;     // the last frame's
    int ready;       // 1 made, -1 failed
    ID3D11Texture2D *map, *tmap;   // the casters' depth from the sun, the ground's (what the hills hide from it)
    ID3D11DepthStencilView *dsv, *tdsv;
    ID3D11ShaderResourceView *srv, *tsrv;
    ID3D11SamplerState *cmp, *samp;
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps;
    ID3D11Buffer *cb;
    ID3D11RasterizerState *raster, *traster;
    ID3D11DepthStencilState *depth;
    float lo[3], hi[3];      // the casters' world box (the draws recorded since the last map)
    float tall, tall_frame;  // the tallest caster's height (tiles; the frame's so far) - the reach off the screen
    int off_frame, last_off; // the casting draws wholly off the screen (the frame's so far, the last map's)
    // the sun the map is drawn from: the light's, never lower than sun_low degrees over the horizon (the day's cycle's
    // evening: shadows of many tiles, the player's screenshot of 2026-11-05); sun_lit its true height (the shadows' colour)
    float sun[3], sun_low, sun_lit;
    float sun_set;           // the pipe's shadows height: the sun's height for the shadows set (degrees; 0 the light's)
    float sun_turn;          // the pipe's shadows turn: the sun's direction turned about the vertical (degrees)
    float amb[3], diff[3];   // the units' ambient (global + the light's) and the sun's colour
    float view[16];          // the units' camera, the last seen
    // the map: world -> u, v, depth; camera space (the ground's) -> the same, and -> the world (gw: its height); the
    // colour a shadow leaves; the water (rd3_vcb shl); on while a map of the frame's casters is drawn
    float shm[16], gm[16], gw[16], shk[4], shl[4], inv_depth;
    int valid;
    LONG passes;
    int last_cast, last_ground;   // the last map's (the pipe's shadows)
    float last_lo[3], last_hi[3];
} g_sh = { .strength = 0.8f, .fade = 0.8f, .tall = 2, .sun = { 0.08f, 0.75f, -0.65f }, .sun_low = 25, .sun_lit = 0.65f };

// from the sun: the batch's vertices placed as the main pass places them (render_3d.c's vertex shader), through shm
// into the map (sa: alpha reference / 255, alpha tested, a tree). A tree is a few flat planes crossed at its trunk (not
// much more than billboards - the player, 2026-11-05): a plane standing edge-on to the sun cast a line. In this pass
// its planes face the sun: each vertex keeps its height and its distance from the trunk (the world matrix's origin), laid
// out across the sun's rays (sr: the map's horizontal axis), on the side of the trunk it was on.
internal const char k_sh_shader[] =
    "cbuffer s : register(b2) { row_major float4x4 shm; float4 sa; float4 sr; };\n"
    "Buffer<float4> wt : register(t1);\n"
    "struct I { float3 p : POSITION; float3 n : NORMAL; float4 c : COLOR; float2 t : TEXCOORD; uint w : WORLD; };\n"
    "Texture2D t : register(t0); SamplerState s : register(s0);\n"
    "struct O { float4 p : SV_Position; float2 t : TEXCOORD0; };\n"
    "O vs(I i) { O o; uint r = i.w * 4; float4 r0 = wt.Load(r), r3 = wt.Load(r + 3); r0.w = 0;"
    " float4 p = mul(float4(i.p, 1), float4x4(r0, wt.Load(r + 1), wt.Load(r + 2), r3));"
    " if (sa.z > 0) { float2 d = p.xy - r3.xy; float k = abs(d.x) > 1e-4 ? sign(d.x) : sign(d.y);"
    "  p.xy = r3.xy + sr.xy * (length(d) * k); }"
    " float3 q = mul(p, shm).xyz; o.p = float4(q.x * 2 - 1, 1 - q.y * 2, q.z, 1); o.t = i.t; return o; }\n"
    "void ps(O i) { if (sa.y > 0 && t.Sample(s, i.t).a < sa.x) discard; }\n";

internal int rd3_shadow_init(void) {
    if (g_sh.ready) return g_sh.ready > 0;
    g_sh.ready = -1;
    ID3D11Device *dv = g_d3d.dev;
    D3D11_TEXTURE2D_DESC td = { SH_SIZE, SH_SIZE, 1, 1, DXGI_FORMAT_R32_TYPELESS, { 1, 0 }, D3D11_USAGE_DEFAULT,
                                D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE, 0, 0 };
    D3D11_DEPTH_STENCIL_VIEW_DESC dd = { .Format = DXGI_FORMAT_D32_FLOAT, .ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D };
    D3D11_SHADER_RESOURCE_VIEW_DESC sd = { .Format = DXGI_FORMAT_R32_FLOAT, .ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D };
    sd.Texture2D.MipLevels = 1;
    HRESULT hr = ID3D11Device_CreateTexture2D(dv, &td, NULL, &g_sh.map);
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateDepthStencilView(dv, (ID3D11Resource *)g_sh.map, &dd, &g_sh.dsv);
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(dv, (ID3D11Resource *)g_sh.map, &sd, &g_sh.srv);
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateTexture2D(dv, &td, NULL, &g_sh.tmap);
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateDepthStencilView(dv, (ID3D11Resource *)g_sh.tmap, &dd, &g_sh.tdsv);
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(dv, (ID3D11Resource *)g_sh.tmap, &sd, &g_sh.tsrv);
    // outside the map: depth 1 - nothing nearer the sun, lit
    D3D11_SAMPLER_DESC cs = { D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_BORDER,
                              D3D11_TEXTURE_ADDRESS_BORDER, D3D11_TEXTURE_ADDRESS_BORDER, 0, 1, D3D11_COMPARISON_LESS_EQUAL,
                              { 1, 1, 1, 1 }, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateSamplerState(dv, &cs, &g_sh.cmp);
    D3D11_SAMPLER_DESC ss = { D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP,
                              D3D11_TEXTURE_ADDRESS_WRAP, 0, 1, D3D11_COMPARISON_NEVER, { 0 }, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateSamplerState(dv, &ss, &g_sh.samp);
    ID3DBlob *code = SUCCEEDED(hr) ? d3d_compile(k_sh_shader, sizeof k_sh_shader - 1, "vs", "vs_4_0") : NULL;
    hr = code ? ID3D11Device_CreateVertexShader(dv, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_sh.vs) : E_FAIL;
    D3D_RELEASE(code);
    code = SUCCEEDED(hr) ? d3d_compile(k_sh_shader, sizeof k_sh_shader - 1, "ps", "ps_4_0") : NULL;
    hr = code ? ID3D11Device_CreatePixelShader(dv, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_sh.ps) : E_FAIL;
    D3D_RELEASE(code);
    D3D11_BUFFER_DESC bd = { 96, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(dv, &bd, NULL, &g_sh.cb);
    D3D11_RASTERIZER_DESC rs = { D3D11_FILL_SOLID, D3D11_CULL_NONE, FALSE, 0, 0, 0, TRUE, FALSE, FALSE, FALSE };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateRasterizerState(dv, &rs, &g_sh.raster);
    rs.DepthClipEnable = FALSE;   // the ground nearer the sun than the casters' box: at its near end
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateRasterizerState(dv, &rs, &g_sh.traster);
    D3D11_DEPTH_STENCIL_DESC ds = { TRUE, D3D11_DEPTH_WRITE_MASK_ALL, D3D11_COMPARISON_LESS, FALSE, 0, 0, { 0 }, { 0 } };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateDepthStencilState(dv, &ds, &g_sh.depth);
    if (FAILED(hr)) return rd_fail("the dynamic shadows", hr), 0;
    g_sh.lo[0] = g_sh.lo[1] = g_sh.lo[2] = 1e30f;
    g_sh.hi[0] = g_sh.hi[1] = g_sh.hi[2] = -1e30f;
    g_sh.ready = 1;
    return 1;
}

// a rigid 4x4 (rotation, translation; row vectors) inverted
internal void rd3_rigid_inverse(float *r, const float *m) {
    memset(r, 0, 64);
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++) r[i * 4 + j] = m[j * 4 + i];
    for (int j = 0; j < 3; j++) r[12 + j] = -(m[12] * r[j] + m[13] * r[4 + j] + m[14] * r[8 + j]);
    r[15] = 1;
}

// A draw's role (SH_), from the render object drawing it and its vertices - the probe's findings (docs/renderer.md):
// the terrain's draws receive (not the sky drawn with them: its view leaves the origin); the game's blob shadows aren't
// drawn, its fields receive; in the units' render object the lit draws that don't lie flat (their vertices' height over 1% of their width
// - the flat ones are decals: roads, craters) cast, those alpha tested at 200 or more (trees, lit by a light of their
// own) without the tint. A caster's world box goes into the frame's, its size (the box's diagonal, at least a tile) into
// d->sh_b: how much nearer the sun than its origin a shadow must be to darken it - its own body (a tank's hull over its
// wheels) must not.
internal int rd3_shadow_role(rd_device *d, rd_vb *vb, DWORD start, DWORD nv) {
    if (!g_sh.on || vb->fvf != 0x152 || !d->rs[D3DRENDERSTATE_ZENABLE] || !rd3_shadow_init()) return SH_NONE;
    if (g_ro_cur == RO_TERRAIN) return d->view._41 == 0 && d->view._42 == 0 && d->view._43 == 0 ? SH_RECEIVE : SH_NONE;
    // the game's shadows' render object draws the farms' fields too (depth written, alpha tested at 127 - the probe on
    // "choke wolf", 2026-11-04): those are kept and receive; the blobs (depth not written) aren't drawn
    if (g_ro_cur == RO_SHADOWS) return d->rs[D3DRENDERSTATE_ZWRITEENABLE] && d->tex[0] ? SH_DECAL : SH_BLOB;
    // the water's surface: in the ground's camera space too (drawn after the units: their camera known - its level)
    if (g_ro_cur == RO_WATER) {
        if (d->view._41 != 0 || d->view._42 != 0 || d->view._43 != 0) return SH_NONE;
        float iv[16], wv[16], m[16];
        rd3_rigid_inverse(iv, g_sh.view);
        rd3_mul(wv, &d->world._11, &d->view._11);
        rd3_mul(m, wv, iv);
        for (DWORD k = 0; k < nv; k++) {
            const float *p = (const float *)(vb->mem + (size_t)(start + k) * vb->stride);
            g_sh.water_z += p[0] * m[2] + p[1] * m[6] + p[2] * m[10] + m[14];
        }
        g_sh.water_n += (int)nv;
        return SH_WATER;
    }
    if (g_ro_cur != RO_UNITS) return SH_NONE;
    float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f };
    for (DWORD k = 0; k < nv; k++) {
        const float *p = (const float *)(vb->mem + (size_t)(start + k) * vb->stride);
        for (int a = 0; a < 3; a++) { if (p[a] < lo[a]) lo[a] = p[a]; if (p[a] > hi[a]) hi[a] = p[a]; }
    }
    if (hi[2] - lo[2] <= 0.01f * fmaxf(hi[0] - lo[0], hi[1] - lo[1])) return d->tex[0] ? SH_DECAL : SH_NONE;
    // unlit: no shadow. See-through it casts as solid: the game draws a building see-through while units stand behind it,
    // or under construction (material alpha 0.35 or 0.60, alpha tested, no depth written - rprobe 2026-11-05), and its
    // shadow came and went with that (the player: "they should always see the real building")
    if (!d->rs[D3DRENDERSTATE_LIGHTING]) return SH_NONE;
    const D3DMATRIX *w = &d->world;
    float wl[3] = { 1e30f, 1e30f, 1e30f }, wh[3] = { -1e30f, -1e30f, -1e30f }, vp[16];
    rd3_mul(vp, &d->view._11, &d->proj._11);
    int out = 0x3F;   // the screen's sides every corner lies beyond (none: on the screen)
    for (int c = 0; c < 8; c++) {
        float x = c & 1 ? hi[0] : lo[0], y = c & 2 ? hi[1] : lo[1], z = c & 4 ? hi[2] : lo[2];
        float q[3] = { x * w->_11 + y * w->_21 + z * w->_31 + w->_41, x * w->_12 + y * w->_22 + z * w->_32 + w->_42,
                       x * w->_13 + y * w->_23 + z * w->_33 + w->_43 };
        for (int a = 0; a < 3; a++) { if (q[a] < wl[a]) wl[a] = q[a]; if (q[a] > wh[a]) wh[a] = q[a]; }
        float cx = q[0] * vp[0] + q[1] * vp[4] + q[2] * vp[8] + vp[12], cy = q[0] * vp[1] + q[1] * vp[5] + q[2] * vp[9] + vp[13],
              cw = q[0] * vp[3] + q[1] * vp[7] + q[2] * vp[11] + vp[15];
        out &= (cx < -cw) | (cx > cw) << 1 | (cy < -cw) << 2 | (cy > cw) << 3;
    }
    if (out) g_sh.off_frame++;   // wholly off the screen: in the map only (SH_AWAY)
    for (int a = 0; a < 3; a++) { if (wl[a] < g_sh.lo[a]) g_sh.lo[a] = wl[a]; if (wh[a] > g_sh.hi[a]) g_sh.hi[a] = wh[a]; }
    float dx = wh[0] - wl[0], dy = wh[1] - wl[1], dz = wh[2] - wl[2];
    d->sh_b = fmaxf(sqrtf(dx * dx + dy * dy + dz * dz), 1.0f);
    if (dz > g_sh.tall_frame) g_sh.tall_frame = dz;
    memcpy(g_sh.view, &d->view._11, sizeof g_sh.view);
    int tree = d->rs[D3DRENDERSTATE_ALPHATESTENABLE] && (d->rs[D3DRENDERSTATE_ALPHAREF] & 255) >= 200;
    const D3DLIGHT7 *l = &d->light[0];
    float n = sqrtf(l->dvDirection.x * l->dvDirection.x + l->dvDirection.y * l->dvDirection.y + l->dvDirection.z * l->dvDirection.z);
    // the units' sun: not the trees' light, nearly overhead (0.01, 0.11, -0.99) - other objects are lit by it too, and
    // taken for the sun it laid every shadow under its caster (the player's shot, 2026-11-04)
    if (!tree && d->light_on[0] && l->dltType == D3DLIGHT_DIRECTIONAL && n > 0 && l->dvDirection.z / n < -0.05f
        && l->dvDirection.z / n > -0.9f) {
        float s[3] = { l->dvDirection.x / n, l->dvDirection.y / n, l->dvDirection.z / n };
        float h = sqrtf(s[0] * s[0] + s[1] * s[1]), low = sinf(g_sh.sun_low * 3.14159265f / 180);
        g_sh.sun_lit = -s[2];
        if (g_sh.sun_set > 0) low = sinf(g_sh.sun_set * 3.14159265f / 180);
        if (g_sh.sun_turn != 0) {
            float a = g_sh.sun_turn * 3.14159265f / 180, x = s[0];
            s[0] = x * cosf(a) - s[1] * sinf(a); s[1] = x * sinf(a) + s[1] * cosf(a);
        }
        if ((-s[2] < low || g_sh.sun_set > 0) && h > 0) { float k = sqrtf(1 - low * low) / h; s[0] *= k; s[1] *= k; s[2] = -low; }
        memcpy(g_sh.sun, s, sizeof s);
        float ga[4];
        rd3_color(ga, d->rs[D3DRENDERSTATE_AMBIENT]);
        g_sh.amb[0] = ga[0] + l->dcvAmbient.r; g_sh.amb[1] = ga[1] + l->dcvAmbient.g; g_sh.amb[2] = ga[2] + l->dcvAmbient.b;
        g_sh.diff[0] = l->dcvDiffuse.r; g_sh.diff[1] = l->dcvDiffuse.g; g_sh.diff[2] = l->dcvDiffuse.b;
    }
    return out ? SH_AWAY : tree || !g_sh.tint ? SH_CAST : SH_TINT;
}
// the list's draw only drawn into the casters' map (rf_replay leaves it out)
internal int rd3_shadow_only(int role) { return role == SH_AWAY; }
// the draw's role on the device: a change is a change of the vertex constants (a new batch)
internal void rd3_shadow_classify(rd_device *d, rd_vb *vb, DWORD start, DWORD nv) {
    int role = rd3_shadow_role(d, vb, start, nv);
    if (role != d->sh_role) { d->sh_role = role; d->changed |= RD_CH_VC; }
}

// Before the frame's list is drawn (rf_replay): its casters from the sun into the map, its ground into the terrain's,
// each batch drawn from where it lies in the upload buffers. A list without casters leaves the map as it is (one drawn
// for the interface alone), with ground but no casters it has no shadows.
internal void rd3_shadow_frame(void) {
    if (!g_sh.on || g_sh.ready <= 0) { g_sh.valid = 0; return; }
    int nc = 0, ng = 0;
    for (int i = 0; i < g_rf.n; i++) {
        const rf_cmd *c = &g_rf.cmd[i];
        if (c->topo != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) continue;
        if (c->role == SH_CAST || c->role == SH_TINT || c->role == SH_AWAY) nc++;
        else if (c->role == SH_RECEIVE || c->role == SH_WATER) ng++;
    }
    // the water's level: its vertices' mean height (no water drawn: no fading)
    if (g_sh.water_n) g_sh.level = (float)(g_sh.water_z / g_sh.water_n);
    g_sh.shl[0] = g_sh.level; g_sh.shl[1] = 1 / fmaxf(g_sh.fade, 0.01f); g_sh.shl[2] = g_sh.water_n ? 1.0f : 0.0f;
    g_sh.water_z = 0;
    g_sh.water_n = 0;
    if (!nc) { if (ng) g_sh.valid = 0; return; }
    // the tallest caster: this list's, falling slowly (a list drawn early in a frame has fewer)
    g_sh.tall = fmaxf(fmaxf(g_sh.tall_frame, g_sh.tall * 0.98f), 2);
    g_sh.tall_frame = 0;
    g_sh.last_off = g_sh.off_frame;
    g_sh.off_frame = 0;
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
    ID3D11DeviceContext *x = g_d3d.ctx;
    ID3D11ShaderResourceView *none = NULL;
    ID3D11DeviceContext_VSSetShaderResources(x, 2, 1, &none);   // the maps are targets now
    ID3D11DeviceContext_PSSetShaderResources(x, 2, 1, &none);
    ID3D11DeviceContext_PSSetShaderResources(x, 4, 1, &none);
    ID3D11DeviceContext_OMSetRenderTargets(x, 0, NULL, g_sh.dsv);
    ID3D11DeviceContext_ClearDepthStencilView(x, g_sh.dsv, D3D11_CLEAR_DEPTH, 1, 0);
    D3D11_VIEWPORT vp = { 0, 0, SH_SIZE, SH_SIZE, 0, 1 };
    ID3D11DeviceContext_RSSetViewports(x, 1, &vp);
    ID3D11DeviceContext_OMSetDepthStencilState(x, g_sh.depth, 0);
    ID3D11DeviceContext_OMSetBlendState(x, NULL, NULL, 0xFFFFFFFF);
    ID3D11DeviceContext_RSSetState(x, g_sh.raster);
    ID3D11DeviceContext_IASetInputLayout(x, g_rd3.inputs);
    UINT stride = sizeof(rb_vertex), off = 0;
    ID3D11DeviceContext_IASetVertexBuffers(x, 0, 1, &g_rd3.vb, &stride, &off);
    ID3D11DeviceContext_IASetIndexBuffer(x, g_rd3.ib, DXGI_FORMAT_R32_UINT, 0);
    ID3D11DeviceContext_IASetPrimitiveTopology(x, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    ID3D11DeviceContext_VSSetShader(x, g_sh.vs, NULL, 0);
    ID3D11DeviceContext_PSSetShader(x, g_sh.ps, NULL, 0);
    ID3D11DeviceContext_VSSetShaderResources(x, 1, 1, &g_rd3.wsrv);
    ID3D11DeviceContext_VSSetConstantBuffers(x, 2, 1, &g_sh.cb);
    ID3D11DeviceContext_PSSetConstantBuffers(x, 2, 1, &g_sh.cb);
    ID3D11DeviceContext_PSSetSamplers(x, 0, 1, &g_sh.samp);
    float cb[24];
    memcpy(cb, m, sizeof m);
    cb[18] = cb[19] = 0;
    cb[20] = r[0]; cb[21] = r[1]; cb[22] = cb[23] = 0;
    float aref = -2, tree = -1;
    ID3D11ShaderResourceView *srv = (ID3D11ShaderResourceView *)1;
    for (int i = 0; i < g_rf.n; i++) {
        const rf_cmd *c = &g_rf.cmd[i];
        if ((c->role != SH_CAST && c->role != SH_TINT && c->role != SH_AWAY) || c->topo != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) continue;
        const rd3_pcb *pc = &g_rf.pcs[c->pc];
        float a = pc->aop[3] && c->srv ? pc->misc[0] / 255.0f : -1;   // alpha tested: its texture's alpha
        float t = a >= 200 / 255.0f ? 1.0f : 0.0f;   // a tree (rd3_shadow_role's test)
        if (a != aref || t != tree) {
            aref = a; tree = t;
            cb[16] = a; cb[17] = a >= 0 ? 1.0f : 0.0f; cb[18] = t;
            ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_sh.cb, 0, NULL, cb, 0, 0);
        }
        if (c->srv != srv) { srv = c->srv; ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &srv); }
        ID3D11DeviceContext_DrawIndexed(x, c->n, c->at, c->base);
    }
    // the ground from the sun, in the same map's coordinates (camera space -> world through the units' camera, no
    // alpha test)
    rd3_rigid_inverse(g_sh.gw, g_sh.view);
    rd3_mul(g_sh.gm, g_sh.gw, m);
    ID3D11DeviceContext_OMSetRenderTargets(x, 0, NULL, g_sh.tdsv);
    ID3D11DeviceContext_ClearDepthStencilView(x, g_sh.tdsv, D3D11_CLEAR_DEPTH, 1, 0);
    ID3D11DeviceContext_RSSetState(x, g_sh.traster);
    cb[16] = cb[17] = cb[18] = 0;
    int vc = -1;
    for (int i = 0; i < g_rf.n; i++) {
        const rf_cmd *c = &g_rf.cmd[i];
        if (c->role != SH_RECEIVE || c->topo != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) continue;
        if (c->vc != vc) {
            vc = c->vc;
            rd3_mul(cb, g_rf.vcs[vc].view, g_sh.gm);
            ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_sh.cb, 0, NULL, cb, 0, 0);
        }
        ID3D11DeviceContext_DrawIndexed(x, c->n, c->at, c->base);
    }
    ID3D11DeviceContext_OMSetRenderTargets(x, 0, NULL, NULL);   // else Direct3D refuses the maps as textures
    // the colour a shadow leaves: the ambient light over the ambient and the sun on level ground, as much of it as the
    // strength says (1: the sun all gone)
    memcpy(g_sh.shm, m, sizeof m);
    float sun = fmaxf(0, g_sh.sun_lit);
    for (int a = 0; a < 3; a++) {
        float lit = g_sh.amb[a] + g_sh.diff[a] * sun;
        float k = lit > 0 ? fminf(1, g_sh.amb[a] / lit) : 1;
        g_sh.shk[a] = 1 - g_sh.strength * (1 - k);
    }
    g_sh.shk[3] = g_sh.strength;
    g_sh.inv_depth = 1 / wz;
    g_sh.valid = 1;
    g_sh.passes++;
    g_sh.last_cast = nc; g_sh.last_ground = ng;
    memcpy(g_sh.last_lo, g_sh.lo, sizeof g_sh.lo); memcpy(g_sh.last_hi, g_sh.hi, sizeof g_sh.hi);
    g_sh.lo[0] = g_sh.lo[1] = g_sh.lo[2] = 1e30f;
    g_sh.hi[0] = g_sh.hi[1] = g_sh.hi[2] = -1e30f;
}
// a batch's constants as the list draws it: this frame's map - the ground's place in it through the units' camera
internal void rd3_shadow_vc(rd3_vcb *vc) {
    vc->shp[2] = g_sh.valid ? 1.0f : 0.0f;
    if (!g_sh.valid) return;
    int role = (int)vc->shp[0];
    if (role == SH_RECEIVE || role == SH_WATER) {
        rd3_mul(vc->shm, vc->view, g_sh.gm);
        float w[16];
        rd3_mul(w, vc->view, g_sh.gw);
        vc->shz[0] = w[2]; vc->shz[1] = w[6]; vc->shz[2] = w[10]; vc->shz[3] = w[14];
    } else {
        memcpy(vc->shm, g_sh.shm, sizeof vc->shm);
        vc->shz[0] = vc->shz[1] = vc->shz[3] = 0; vc->shz[2] = 1;
    }
    memcpy(vc->shl, g_sh.shl, sizeof vc->shl);
    memcpy(vc->shk, g_sh.shk, sizeof vc->shk);
    vc->shp[3] = g_sh.inv_depth;
}
internal void rd3_shadow_pc(rd3_pcb *pc) {
    pc->shd[2] = g_sh.valid ? 1.0f : 0.0f;
    memcpy(pc->shk, g_sh.shk, sizeof pc->shk);
    pc->sht[0] = 0.3f * g_sh.inv_depth;   // a third of a tile: the terrain's map and the ground differ by its texels
}
// the maps for the main pass (rf_replay binds them with the rest)
internal void rd3_shadow_bind(ID3D11DeviceContext *x) {
    ID3D11ShaderResourceView *srv = g_sh.ready > 0 ? g_sh.srv : NULL, *tsrv = g_sh.ready > 0 ? g_sh.tsrv : NULL;
    ID3D11DeviceContext_VSSetShaderResources(x, 2, 1, &srv);
    ID3D11DeviceContext_PSSetShaderResources(x, 2, 1, &srv);
    ID3D11DeviceContext_PSSetShaderResources(x, 4, 1, &tsrv);
    if (g_sh.ready > 0) ID3D11DeviceContext_PSSetSamplers(x, 1, 1, &g_sh.cmp);
}
// the shadows' part in the constants as the draw is recorded: its role (the map's matrix and colour come when the list
// is drawn)
internal void rd3_shadow_constants(rd_device *d, rd3_vcb *vc) {
    vc->shp[0] = (float)d->sh_role;
    vc->shp[1] = SH_SIZE;
}
internal void rd3_shadow_pconstants(rd_device *d, rd3_pcb *pc) {
    pc->shd[0] = (float)d->sh_role;
    pc->shd[1] = 1.5f / SH_SIZE;   // the 3x3 comparisons' spacing
    pc->shd[3] = 0.002f;           // their bias
}

// SHADOWS FROM OFF THE SCREEN: the game collects what it draws from the terrain mesh's visible tile rows, each object
// there that passes a test of its model against the view (EEROUnits.draw) - a building off the screen wasn't drawn,
// and its shadow reaching onto the screen went with it (the day's cycle's evening shadows: a few tiles - the player,
// 2026-11-05). The collection's rows are widened toward the sun by the longest shadow the sun's height and the tallest
// caster make, as rows of their own after the game's; on those every object passes the view's test (the display DLLs'
// test of a model too, below); a draw wholly off the screen casts and isn't drawn (SH_AWAY). A tile the map hasn't got (outside its diamond: no tile) is left out,
// the game's test of a tile would read it. Only with the draw list moved: the original's 512 would fill with them.
#define SHR_ROWS 8192
#define SHR_MAP 4096       // the largest map's side the rows are widened on
#define SHR_REACH 24       // the longest reach (tiles)
internal const char k_shr_visible[] = "?IsPerspectiveModelVisible@GERasterizer@@IAEXPAVGEModel@@MPAVGEViewport@@AAVGETransformation@@AA_N3@Z";
internal const char k_shr_visible_ortho[] = "?IsOrthographicModelVisible@GERasterizer@@IAEXPAVGEModel@@PAVGEViewport@@AAVGETransformation@@AA_N3@Z";
internal int draw_list_moved(void);   // drawlist.c
internal struct {
    int on;                       // the hooks are in
    int off;                      // the pipe's shadows reach off
    long y[SHR_ROWS], x0[SHR_ROWS], x1[SHR_ROWS];
    int n, own;                   // the rows handed out this collection, the game's own (the first)
    int extra;                    // the row handed out last is one of the widened ones: its objects pass the view's test
    int lo[SHR_MAP], hi[SHR_MAP], olo[SHR_MAP], ohi[SHR_MAP];   // per y: the widened rows' x, the game's
    float reach;                  // the last collection's (tiles)
    int last_own, last_extra;     // its rows
} g_shr;

// a piece [a, b] of row y: its runs of tiles the map has
internal void shr_add(ee_tile **tiles, unsigned w, int y, int a, int b) {
    for (int x = a; x <= b && g_shr.n < SHR_ROWS; x++) {
        if (!tiles[(unsigned)y * w + (unsigned)x]) continue;
        int e = x;
        while (e < b && tiles[(unsigned)y * w + (unsigned)e + 1]) e++;
        g_shr.y[g_shr.n] = y; g_shr.x0[g_shr.n] = x; g_shr.x1[g_shr.n] = e; g_shr.n++;
        x = e;
    }
}

internal unsigned long __thiscall shr_hook_rows(const void *mesh) {
    unsigned long n = terrain_rows(mesh);
    g_shr.n = g_shr.own = g_shr.extra = 0;
    ee_map *map = game_map();
    const float *f = g_sh.sun;
    float h = sqrtf(f[0] * f[0] + f[1] * f[1]);
    unsigned w = map ? map_get_size(map) : 0;
    if (!g_shr.on || g_shr.off || !g_sh.on || !draw_list_moved() || !w || w > SHR_MAP || n > SHR_ROWS / 4 || h < 0.01f || f[2] > -0.01f) return n;
    ee_tile **tiles = map_get_tiles(map);
    for (unsigned y = 0; y < w; y++) { g_shr.lo[y] = g_shr.olo[y] = INT_MAX; g_shr.hi[y] = g_shr.ohi[y] = INT_MIN; }
    // the game's rows, as it would have had them
    for (unsigned long i = 0; i < n; i++) {
        long y, x0, x1;
        terrain_row(mesh, i, &y, &x0, &x1);
        g_shr.y[i] = y; g_shr.x0[i] = x0; g_shr.x1[i] = x1;
        if (y < 0 || (unsigned)y >= w) continue;
        if (x0 < g_shr.olo[y]) g_shr.olo[y] = (int)x0;
        if (x1 > g_shr.ohi[y]) g_shr.ohi[y] = (int)x1;
    }
    g_shr.n = g_shr.own = (int)n;
    // each swept toward the sun (the map's tiles are its world's x and y) by the reach, in half tiles
    float reach = fminf(g_sh.tall * h / -f[2], SHR_REACH) + 1, dx = -f[0] / h, dy = -f[1] / h;
    for (unsigned long i = 0; i < n; i++)
        for (float k = 0; k <= reach; k += 0.5f) {
            float yy = (float)g_shr.y[i] + dy * k;
            int x0 = (int)floorf((float)g_shr.x0[i] + dx * k), x1 = (int)ceilf((float)g_shr.x1[i] + dx * k);
            if (x0 < 0) x0 = 0;
            if (x1 > (int)w - 1) x1 = (int)w - 1;
            for (int y = (int)floorf(yy); y <= (int)ceilf(yy); y++) {
                if (y < 0 || y >= (int)w || x0 > x1) continue;
                if (x0 < g_shr.lo[y]) g_shr.lo[y] = x0;
                if (x1 > g_shr.hi[y]) g_shr.hi[y] = x1;
            }
        }
    // the widened rows' pieces outside the game's
    for (unsigned y = 0; y < w; y++) {
        if (g_shr.lo[y] > g_shr.hi[y]) continue;
        if (g_shr.olo[y] > g_shr.ohi[y]) { shr_add(tiles, w, (int)y, g_shr.lo[y], g_shr.hi[y]); continue; }
        if (g_shr.lo[y] < g_shr.olo[y]) shr_add(tiles, w, (int)y, g_shr.lo[y], g_shr.olo[y] - 1);
        if (g_shr.hi[y] > g_shr.ohi[y]) shr_add(tiles, w, (int)y, g_shr.ohi[y] + 1, g_shr.hi[y]);
    }
    g_shr.reach = reach;
    g_shr.last_own = g_shr.own;
    g_shr.last_extra = g_shr.n - g_shr.own;
    return (unsigned long)g_shr.n;
}
internal void __thiscall shr_hook_row(const void *mesh, unsigned long i, long *y, long *x0, long *x1) {
    if (!g_shr.n) { terrain_row(mesh, i, y, x0, x1); return; }
    if (i >= (unsigned long)g_shr.n) i = (unsigned long)g_shr.n - 1;
    *y = g_shr.y[i]; *x0 = g_shr.x0[i]; *x1 = g_shr.x1[i];
    g_shr.extra = (int)i >= g_shr.own;
}
internal uint8_t __thiscall shr_hook_view(const void *model, const void *viewport, uint8_t flag) {
    return model_intersects(model, viewport, flag) || g_shr.extra;
}
// The display DLLs draw a model only where the engine's test of it against the view passes
// (DX7Rasterizer::DrawModelHierarchy: GERasterizer::IsPerspectiveModelVisible / IsOrthographicModelVisible, their
// imports - the first flag visible, the second the clipping it is drawn with): in the units' render object every model
// the collection handed over passes, clipped (the game's own off the screen too, a few). Their draws wholly off the
// screen cast without being drawn (SH_AWAY).
typedef void (__thiscall *model_visible_fn)(void *ras, void *model, float lod, void *viewport, void *at, uint8_t *visible,
                                            uint8_t *clip);
typedef void (__thiscall *model_visible_ortho_fn)(void *ras, void *model, void *viewport, void *at, uint8_t *visible,
                                                  uint8_t *clip);
internal model_visible_fn g_shr_visible;
internal model_visible_ortho_fn g_shr_visible_ortho;
internal int shr_pass(void) { return g_shr.on && !g_shr.off && g_sh.on && g_ro_cur == RO_UNITS && draw_list_moved(); }
internal void __thiscall shr_hook_visible(void *ras, void *model, float lod, void *viewport, void *at, uint8_t *visible,
                                          uint8_t *clip) {
    g_shr_visible(ras, model, lod, viewport, at, visible, clip);
    if (!*visible && shr_pass()) { *visible = 1; *clip = 1; }
}
internal void __thiscall shr_hook_visible_ortho(void *ras, void *model, void *viewport, void *at, uint8_t *visible,
                                                uint8_t *clip) {
    g_shr_visible_ortho(ras, model, viewport, at, visible, clip);
    if (!*visible && shr_pass()) { *visible = 1; *clip = 1; }
}
// a display DLL loaded (render.c render_display_loaded): its imports of the test -> the hooks; how many
internal int shadow_reach_display(HMODULE m) {
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
    if (!lle) return 0;
    if (!g_shr_visible) g_shr_visible = (model_visible_fn)(void *)GetProcAddress(lle, k_shr_visible);
    if (!g_shr_visible_ortho) g_shr_visible_ortho = (model_visible_ortho_fn)(void *)GetProcAddress(lle, k_shr_visible_ortho);
    if (!g_shr_visible || !g_shr_visible_ortho) return 0;
    return patch_iat(m, "Low-Level Engine.dll", k_shr_visible, (void *)shr_hook_visible)
         + patch_iat(m, "Low-Level Engine.dll", k_shr_visible_ortho, (void *)shr_hook_visible_ortho);
}
// with the own renderer (eeaimod.c, after the draw list is moved)
internal void shadow_reach_install(void) {
    int r = patch_units_rows(shr_hook_rows, shr_hook_row), v = patch_units_view(shr_hook_view);
    g_shr.on = r == 2 && v == 5;
    if (g_log) fprintf(g_log, "# shadows: casters off the screen collected %s (rows %d/2, view tests %d/5)\n",
                       g_shr.on ? "toward the sun" : "NOT - exe differs", r, v);
}

// shadows [game | dynamic] [strength]: the shadows switched while playing (the setting's at the start); the strength
// how much of the sun a shadow takes (0..1, default 0.8). shadows fade <depth>: by how deep under the water a shadow is
// gone (default 0.8). shadows tint on | off: a unit standing in a shadow darkened (off by default). shadows sun
// <degrees>: the sun's lowest height for the shadows (default 25). shadows reach on | off: the casters off the screen
// collected (on by default). shadows height <degrees>: the sun's height for the shadows set, its direction the light's
// (0: the light's height, the default) - long evening shadows to look at without the day's cycle. shadows turn <degrees>:
// the sun's direction turned about the vertical (0 the light's, the default) - shadows toward each of the screen's edges
internal void cmd_shadows(char *out, const char *args) {
    char mode[16] = "", arg[16] = "";
    sscanf(args, "%15s %15s", mode, arg);
    float s = arg[0] ? (float)atof(arg) : -1;
    if (!strcmp(mode, "tint")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_sh.tint = arg[1] == 'n';
        else { REPLY("error: usage: shadows tint on | off\n"); return; }
    } else if (!strcmp(mode, "reach")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_shr.off = arg[1] != 'n';
        else { REPLY("error: usage: shadows reach on | off\n"); return; }
    } else if (!strcmp(mode, "height")) {
        if (s >= 0 && s <= 90) g_sh.sun_set = s;
        else { REPLY("error: usage: shadows height <the sun's height for the shadows, degrees 1..90; 0 the light's>\n"); return; }
    } else if (!strcmp(mode, "turn")) {
        if (arg[0] && fabsf(s) <= 360) g_sh.sun_turn = (float)atof(arg);
        else { REPLY("error: usage: shadows turn <degrees about the vertical, -360..360>\n"); return; }
    } else if (!strcmp(mode, "sun")) {
        if (s >= 0 && s <= 90) g_sh.sun_low = s;
        else { REPLY("error: usage: shadows sun <the sun's lowest height for the shadows, degrees 0..90>\n"); return; }
    } else if (!strcmp(mode, "fade")) {
        if (s > 0) g_sh.fade = s;
        else { REPLY("error: usage: shadows fade <depth under the water, > 0>\n"); return; }
    } else if (!strcmp(mode, "game")) g_sh.on = 0;
    else if (!strcmp(mode, "dynamic")) g_sh.on = 1;
    else if (mode[0]) { REPLY("error: usage: shadows [game | dynamic] [strength 0..1] | fade <depth> | sun <degrees> | height <degrees> | turn <degrees> | reach on | off | tint on | off\n"); return; }
    if ((!strcmp(mode, "game") || !strcmp(mode, "dynamic")) && s >= 0 && s <= 1) g_sh.strength = s;
    REPLY("shadows: %s, strength %.2f, %s, %ld maps drawn; the last: %d casting batches, %d of ground;"
          " the casters' box %.1f %.1f %.1f - %.1f %.1f %.1f, the sun %.2f %.2f %.2f, a shadow's colour %.2f %.2f %.2f;"
          " the water at %.2f%s, a shadow gone %.2f under it; units in a shadow darkened: %s; the sun at least %.0f degrees"
          " high (the light's %.0f); casters off the screen: %s, %.1f tiles (the tallest %.1f), %d rows and %d more; %d casting draws off the screen\n",
          g_sh.on ? "dynamic" : "game", (double)g_sh.strength,
          g_sh.ready > 0 ? "ready" : g_sh.ready < 0 ? "FAILED (the log says why)" : "not made yet", (long)g_sh.passes,
          g_sh.last_cast, g_sh.last_ground, (double)g_sh.last_lo[0], (double)g_sh.last_lo[1], (double)g_sh.last_lo[2],
          (double)g_sh.last_hi[0], (double)g_sh.last_hi[1], (double)g_sh.last_hi[2], (double)g_sh.sun[0],
          (double)g_sh.sun[1], (double)g_sh.sun[2], (double)g_sh.shk[0], (double)g_sh.shk[1], (double)g_sh.shk[2],
          (double)g_sh.level, g_sh.shl[2] > 0 ? "" : " (none seen)", (double)g_sh.fade, g_sh.tint ? "on" : "off",
          (double)g_sh.sun_low, (double)(asinf(fminf(1, fmaxf(-1, g_sh.sun_lit))) * 180 / 3.14159265f),
          !g_shr.on ? "not (no hooks)" : g_shr.off ? "off" : "collected", (double)g_shr.reach, (double)g_sh.tall, g_shr.last_own, g_shr.last_extra, g_sh.last_off);
}
