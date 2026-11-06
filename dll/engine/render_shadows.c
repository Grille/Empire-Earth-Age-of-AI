// THE DYNAMIC SHADOWS ([display] shadows = dynamic; docs/renderer.md "Shadows"): the game's shadows are a blurred patch
// a unit (EEROGroundShadows, a quad on the ground); these are cast by the sun from the units', buildings' and trees'
// shapes onto the ground, and onto the units, buildings and trees too (g_sh.all: per pixel, the lights' diffuse part
// taken out of their colour - a face turned from the sun keeps its own shading, nothing darkens twice). Each draw gets a
// role (rd3_shadow_role): the terrain's draws receive; the units' lit solid draws cast, and a unit standing in a shadow
// is darkened as a whole by one look at its origin's place in the map (SH_TINT - no shading of its own; off unless the
// pipe's shadows tint on: it came across as flicker, units darkening and brightening as they move); trees cast
// without the tint (in a forest the trees around would darken them all); the decals lying on the ground (roads,
// craters - in the units' render object; the farms' fields - in the game's shadows') and the water's surface receive;
// the game's blobs are not drawn. Under the water a shadow fades out with the depth: gone at g_sh.fade below the level
// (the deepest seabed measured: 0.84 under it, the water at 0 - "choke wolf", 2026-11-04).
// The frame is recorded (render_frame.c): before its list is drawn, the casters in it are drawn from the sun into a
// depth map fitted to their box - this frame's, from the batches as they lie in the upload buffers (rd3_shadow_frame);
// the hills' shadows are a map of their own over the whole map (shadow_hills). The terrain and the decals then receive
// in the main pixel shader, a shadow leaving a point the ambient part of the light it faces (the game's light, as its
// baked shading: a slope turned away loses nothing). Until 2026-11-05 the ground was drawn from the sun into a second
// depth map, which kept the casters' shadows off the slopes it hid from the shadows' sun - never lower than 25
// degrees, while the evening's baked shading is the lower light's: a tower's shadow on a hill broke off on its lit far
// slope (the player's shot). The ground's vertices are the game's own transform into the camera's space (its view only
// swaps y and z): their place in the world needs the camera the units are drawn with, which comes later in the frame - known
// when the list is drawn (rd3_shadow_vc).
// SH_AWAY: casts, not drawn (off the screen, a tree's card); SH_GONE: neither (a tree off the screen); SH_TREE: drawn,
// not casting (a tree whose card casts)
enum { SH_NONE, SH_RECEIVE, SH_CAST, SH_TINT, SH_DECAL, SH_BLOB, SH_WATER, SH_AWAY, SH_GONE, SH_TREE };
#define SH_SIZE 4096
#define SH_BLOCK 8   // tiles a block of the ground's highest and lowest (g_sh.bmax)
internal struct {
    int on;          // [display] shadows (the pipe's shadows switches it)
    float strength;  // how much of the sun a shadow takes away (1: all but the ambient light)
    float fade;      // the depth under the water by which a shadow is gone
    float water;     // how much of a shadow the water's surface takes (it shows the sky's light too - the player, 2026-11-05:
                     // "the shadow on the water is still a bit too strong")
    int tint;        // a unit in a shadow darkened (SH_TINT): off - it came across as flicker (the player, 2026-11-05)
    int nocards;     // the pipe's shadows cards off: trees cast themselves (rd3_shadow_card)
    int all;         // the units, buildings and trees receive too, per pixel (on; the pipe: shadows all on | off)
    double water_z;  // the water's vertices' heights this frame, summed, and how many (its level)
    int water_n;
    int water_on;    // the fading under the water on: water seen in this frame or the last drawn with an update
    int water_frame; // water seen in this frame (rd3_shadow_new_frame)
    double ground_c[3];   // the terrain's vertex colours this frame, summed, and how many - the game's baked shading
    int ground_n;
    float ground_mean[3]; // the last frame's mean (the pipe's reply: does it follow the time of day?)
    float level;     // the last frame's
    int ready;       // 1 made, -1 failed
    ID3D11Texture2D *map;   // the casters' depth from the sun
    ID3D11DepthStencilView *dsv;
    ID3D11ShaderResourceView *srv;
    ID3D11SamplerState *cmp, *samp, *lin;
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps;
    ID3D11Buffer *cb;
    ID3D11RasterizerState *raster;
    // the hills' shadows (shadow_hills): the height below which the hills hide the sun, a texel each 1/hr tile over
    // the map (hn a side); the texels' ground kept (from the map hmap, cw its corners a side); the sun it was made for
    ID3D11Texture2D *hill;
    ID3D11ShaderResourceView *hsrv;
    float *hceil, *hground;
    int hn, hr, cw, hill_ok, nohills;
    const void *hmap;
    float hsun[3];
    volatile LONG hjob;      // its making on the thread: 0 none, 1 under way (hceil the thread's), 2 made (to upload)
    HANDLE hgo, hthread;
    int hfail;               // the texture or the thread not made: no hills' shadows (the log says so, once)
    LONG hill_made;
    float hill_ms;           // the last making's time
    ID3D11DepthStencilState *depth;
    float lo[3], hi[3];      // the casters' world box (the draws recorded since the last map)
    float tall, tall_frame;  // the tallest caster's height (tiles; the frame's so far) - the reach off the screen
    // the ground's highest (bmax) and lowest (bmax + bw * bw) in blocks of SH_BLOCK tiles, bw a side: a caster on a hill
    // casts farther by its height over the ground on the screen (the player's shot, 2026-11-05: a tower on a hill, its
    // shadow gone with it off the screen)
    float *bmax;
    float glow;              // the map's lowest ground (-50 before a map's is known): no caster below it
    int bw;
    int off_frame, last_off; // the casting draws wholly off the screen (the frame's so far, the last map's)
    // the sun the map is drawn from: the light's, never lower than sun_low degrees over the horizon (the day's cycle's
    // evening: shadows of many tiles, the player's screenshot of 2026-11-05); sun_lit its true height (the shadows' colour)
    float sun[3], sun_low, sun_lit;
    // the shadows' share by the light's height (the day's cycle: out as the sun sets, in as the moon rises - the
    // light froze below 3 degrees and jumped to the moon's direction before, 2026-11-05) and when it was last moved
    float dusk;
    float moon;              // a shadow's share by moonlight (the pipe: shadows moon <0..1>)
    DWORD dusk_tick;
    float light[3];          // the light's own direction (the terrain's baked shading follows it)
    float hlight[3];         // the hills' sun (shadow_hills)
    float sun_set;           // the pipe's shadows height: the sun's height for the shadows set (degrees; 0 the light's)
    float hsoft;             // the hills' shadow full this far (tiles) below the ground's ceiling (the pipe: shadows hillsoft)
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
} g_sh = { .strength = 0.8f, .fade = 0.8f, .water = 0.5f, .moon = 0.75f, .glow = -50, .all = 1, .tall = 2, .sun = { 0.08f, 0.75f, -0.65f }, .light = { 0.08f, 0.75f, -0.65f }, .hlight = { 0.08f, 0.75f, -0.65f }, .sun_low = 25, .sun_lit = 0.65f, .hsoft = 0.75f };

// from the sun: the batch's vertices placed as the main pass places them (render_3d.c's vertex shader), through shm
// into the map (sa: alpha reference / 255, alpha tested). A tree casts its card (rd3_shadow_card), not itself. The
// alpha test reads the texture at most one mip level down: the map has about 60 texels a tile, fewer than a tree's
// texture over it, and at the level that fits a palm's fronds' alpha was averaged away - only a crown's dense middle
// passed the trees' 229, a smooth blob (the player's shot, 2026-11-05).
internal const char k_sh_shader[] =
    "cbuffer s : register(b2) { row_major float4x4 shm; float4 sa; };\n"
    "Buffer<float4> wt : register(t1);\n"
    "struct I { float3 p : POSITION; float3 n : NORMAL; float4 c : COLOR; float2 t : TEXCOORD; uint w : WORLD; };\n"
    RD3_MORPH_HLSL
    "Texture2D t : register(t0); SamplerState s : register(s0);\n"
    "struct O { float4 p : SV_Position; float2 t : TEXCOORD0; };\n"
    "O vs(I i, uint id : SV_VertexID) { O o; morph(i, id); uint r = i.w * 8; float4 r0 = wt.Load(r); r0.w = 0;"
    " float4 p = mul(float4(i.p, 1), float4x4(r0, wt.Load(r + 1), wt.Load(r + 2), wt.Load(r + 3)));"
    " float3 q = mul(p, shm).xyz; o.p = float4(q.x * 2 - 1, 1 - q.y * 2, q.z, 1); o.t = i.t; return o; }\n"
    "void ps(O i) { if (sa.y <= 0) return; float w, h; t.GetDimensions(w, h); float2 u = i.t * float2(w, h);"
    " float l = clamp(0.5 * log2(max(dot(ddx(u), ddx(u)), dot(ddy(u), ddy(u)))), 0, 1);"
    " if (t.SampleLevel(s, i.t, l).a < sa.x) discard; }\n";

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
    // outside the map: depth 1 - nothing nearer the sun, lit
    D3D11_SAMPLER_DESC cs = { D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT, D3D11_TEXTURE_ADDRESS_BORDER,
                              D3D11_TEXTURE_ADDRESS_BORDER, D3D11_TEXTURE_ADDRESS_BORDER, 0, 1, D3D11_COMPARISON_LESS_EQUAL,
                              { 1, 1, 1, 1 }, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateSamplerState(dv, &cs, &g_sh.cmp);
    D3D11_SAMPLER_DESC ss = { D3D11_FILTER_MIN_MAG_MIP_POINT, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP,
                              D3D11_TEXTURE_ADDRESS_WRAP, 0, 1, D3D11_COMPARISON_NEVER, { 0 }, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateSamplerState(dv, &ss, &g_sh.samp);
    D3D11_SAMPLER_DESC ls = { D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP,
                              D3D11_TEXTURE_ADDRESS_CLAMP, 0, 1, D3D11_COMPARISON_NEVER, { 0 }, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateSamplerState(dv, &ls, &g_sh.lin);
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
// walls.c sets it while a debris' model is drawn: a decal the ground's shape bends (building_lean), its vertices no longer
// flat - it cast a shadow of a pile hanging in the air (the player, 2026-11-05: "since its basically a decal we should
// consider disabling shadows for debris")
internal int g_sh_decal;
internal const float *g_mo_box;   // a blended draw's box, lo and hi (render_morph.c): its vertices aren't in the buffer
internal int rd3_shadow_role(rd_device *d, rd_vb *vb, DWORD start, DWORD nv) {
    if (!g_sh.on || vb->fvf != 0x152 || !d->rs[D3DRENDERSTATE_ZENABLE] || !rd3_shadow_init()) return SH_NONE;
    if (g_ro_cur == RO_TERRAIN) {
        if (d->view._41 != 0 || d->view._42 != 0 || d->view._43 != 0) return SH_NONE;
        for (DWORD k = 0; k < nv; k++) {
            const uint8_t *c = vb->mem + (size_t)(start + k) * vb->stride + 24;   // the diffuse colour: B G R A
            g_sh.ground_c[0] += c[2]; g_sh.ground_c[1] += c[1]; g_sh.ground_c[2] += c[0];
        }
        g_sh.ground_n += (int)nv;
        return SH_RECEIVE;
    }
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
    if (g_mo_box) for (int a = 0; a < 3; a++) { lo[a] = g_mo_box[a]; hi[a] = g_mo_box[3 + a]; }
    else for (DWORD k = 0; k < nv; k++) {
        const float *p = (const float *)(vb->mem + (size_t)(start + k) * vb->stride);
        for (int a = 0; a < 3; a++) { if (p[a] < lo[a]) lo[a] = p[a]; if (p[a] > hi[a]) hi[a] = p[a]; }
    }
    if (g_sh_decal || hi[2] - lo[2] <= 0.01f * fmaxf(hi[0] - lo[0], hi[1] - lo[1])) return d->tex[0] ? SH_DECAL : SH_NONE;
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
    // under the map's lowest ground the game keeps objects out of sight (one at -5854 in the player's game, 2026-11-05:
    // the casters' box and the map's depth with it 2000 times as deep, every shadow gone): those cast nothing, and no
    // caster's box reaches down there
    if (wh[2] < g_sh.glow - 2) return SH_NONE;
    wl[2] = fmaxf(wl[2], g_sh.glow - 2);
    if (out) g_sh.off_frame++;   // wholly off the screen: in the map only (SH_AWAY)
    for (int a = 0; a < 3; a++) { if (wl[a] < g_sh.lo[a]) g_sh.lo[a] = wl[a]; if (wh[a] > g_sh.hi[a]) g_sh.hi[a] = wh[a]; }
    float dx = wh[0] - wl[0], dy = wh[1] - wl[1], dz = wh[2] - wl[2];
    d->sh_b = fmaxf(sqrtf(dx * dx + dy * dy + dz * dz), 1.0f);
    if (dz > g_sh.tall_frame) g_sh.tall_frame = dz;
    memcpy(g_sh.view, &d->view._11, sizeof g_sh.view);
    int tree = d->rs[D3DRENDERSTATE_ALPHATESTENABLE] && (d->rs[D3DRENDERSTATE_ALPHAREF] & 255) >= 200;
    d->sh_card = tree && !g_sh.nocards;   // a tree casts its card (rd3_shadow_card) and is drawn without casting - off the screen not at all
    const D3DLIGHT7 *l = &d->light[0];
    float n = sqrtf(l->dvDirection.x * l->dvDirection.x + l->dvDirection.y * l->dvDirection.y + l->dvDirection.z * l->dvDirection.z);
    // the units' sun: not the trees' light, nearly overhead (0.01, 0.11, -0.99) - other objects are lit by it too, and
    // taken for the sun it laid every shadow under its caster (the player's shot, 2026-11-04)
    // A light at the horizon (the sun set, the moon not yet up) is no sun: its height 0 fades the shadows out
    // (shadow_dusk), its direction kept
    if (!tree && d->light_on[0] && l->dltType == D3DLIGHT_DIRECTIONAL && n > 0 && l->dvDirection.z / n >= -0.05f)
        g_sh.sun_lit = fmaxf(0, -l->dvDirection.z / n);
    else if (!tree && d->light_on[0] && l->dltType == D3DLIGHT_DIRECTIONAL && n > 0 && l->dvDirection.z / n > -0.9f) {
        float s[3] = { l->dvDirection.x / n, l->dvDirection.y / n, l->dvDirection.z / n };
        float h = sqrtf(s[0] * s[0] + s[1] * s[1]), low = sinf(g_sh.sun_low * 3.14159265f / 180);
        // the light turned by more than 20 degrees at once (the sun's light handed over to the moon's): out, then in
        if (s[0] * g_sh.light[0] + s[1] * g_sh.light[1] + s[2] * g_sh.light[2] < 0.94f) g_sh.dusk = 0;
        g_sh.sun_lit = -s[2];
        memcpy(g_sh.light, s, sizeof g_sh.light);
        if (g_sh.sun_set > 0) low = sinf(g_sh.sun_set * 3.14159265f / 180);
        if (g_sh.sun_turn != 0) {
            float a = g_sh.sun_turn * 3.14159265f / 180, x = s[0];
            s[0] = x * cosf(a) - s[1] * sinf(a); s[1] = x * sinf(a) + s[1] * cosf(a);
        }
        // the hills' sun: the light's own height (their map covers the whole map: no reach to keep short), the pipe's
        // height and turn as the casters'
        memcpy(g_sh.hlight, s, sizeof s);
        if ((-s[2] < low || g_sh.sun_set > 0) && h > 0) { float k = sqrtf(1 - low * low) / h; s[0] *= k; s[1] *= k; s[2] = -low; }
        if (g_sh.sun_set > 0) memcpy(g_sh.hlight, s, sizeof s);
        memcpy(g_sh.sun, s, sizeof s);
        float ga[4];
        rd3_color(ga, d->rs[D3DRENDERSTATE_AMBIENT]);
        g_sh.amb[0] = ga[0] + l->dcvAmbient.r; g_sh.amb[1] = ga[1] + l->dcvAmbient.g; g_sh.amb[2] = ga[2] + l->dcvAmbient.b;
        g_sh.diff[0] = l->dcvDiffuse.r; g_sh.diff[1] = l->dcvDiffuse.g; g_sh.diff[2] = l->dcvDiffuse.b;
    }
    if (d->sh_card) return out ? SH_GONE : SH_TREE;
    return out ? SH_AWAY : !g_sh.tint ? SH_CAST : SH_TINT;
}
// A TREE'S CARD (the player, 2026-11-05): the game's camera never turns, and its trees are curved cards made for it -
// "there no real 3d shapes, they are curved billboards to sell the illusion better, but from the side or behind they
// break". Turned toward the sun as a whole they cast narrow (a plane edge-on), flipped with the wind's wave (two
// directions nearly as wide) and lost multi-tree objects' crowns from their trunks (turned about the object's origin).
// So a tree casts the shape the camera sees: each piece of it (its triangles joined by shared vertices: a crown, a
// card) laid flat along the camera's view - its width across the screen and its height kept - and stood up facing the
// sun about its own middle. Made each draw from the vertices as the game animated them, cast only (SH_AWAY); the tree
// itself is drawn without casting.
internal int rd3_prepare(rd_device *d, int clamp);
internal void rd3_append(rd_device *d, D3DPRIMITIVETYPE type, rd_vb *vb, DWORD start, DWORD nv, const WORD *idx, DWORD ni);
#define SH_CARD_MAX 4096
internal struct {
    uint8_t v[SH_CARD_MAX * 36];   // the card's vertices (FVF 0x152), in the world
    int up[SH_CARD_MAX], n[SH_CARD_MAX];   // a vertex's piece (union-find), a piece's vertices
    float cx[SH_CARD_MAX], cy[SH_CARD_MAX];   // a piece's middle
} g_card;
internal int card_piece(int i) {
    while (g_card.up[i] != i) i = g_card.up[i] = g_card.up[g_card.up[i]];
    return i;
}
internal void rd3_shadow_card(rd_device *d, D3DPRIMITIVETYPE type, rd_vb *vb, DWORD start, DWORD nv, const WORD *idx,
                              DWORD ni) {
    if (type != D3DPT_TRIANGLELIST || nv > SH_CARD_MAX || vb->stride != 36) return;
    // across the screen (the camera's x axis in the world) and across the sun's rays, on the same side
    float ax = d->view._11, ay = d->view._21, al = sqrtf(ax * ax + ay * ay);
    float sx = g_sh.sun[1], sy = -g_sh.sun[0], sl = sqrtf(sx * sx + sy * sy);
    if (al < 1e-4f || sl < 1e-4f) return;
    ax /= al; ay /= al; sx /= sl; sy /= sl;
    if (ax * sx + ay * sy < 0) { sx = -sx; sy = -sy; }
    // the pieces
    DWORD nt = (idx ? ni : nv) / 3;
    for (DWORD k = 0; k < nv; k++) { g_card.up[k] = (int)k; g_card.n[k] = 0; g_card.cx[k] = g_card.cy[k] = 0; }
    for (DWORD t = 0; t < nt; t++) {
        int a = idx ? idx[t * 3] : (int)(t * 3), b = idx ? idx[t * 3 + 1] : (int)(t * 3 + 1), c = idx ? idx[t * 3 + 2] : (int)(t * 3 + 2);
        if ((DWORD)a >= nv || (DWORD)b >= nv || (DWORD)c >= nv) return;
        a = card_piece(a); b = card_piece(b); c = card_piece(c);
        g_card.up[b] = a;
        g_card.up[card_piece(c)] = a;
    }
    // the vertices in the world, each piece's middle
    const D3DMATRIX *w = &d->world;
    for (DWORD k = 0; k < nv; k++) {
        uint8_t *o = g_card.v + k * 36;
        memcpy(o, vb->mem + (size_t)(start + k) * 36, 36);
        float *p = (float *)o, x = p[0], y = p[1], z = p[2];
        p[0] = x * w->_11 + y * w->_21 + z * w->_31 + w->_41;
        p[1] = x * w->_12 + y * w->_22 + z * w->_32 + w->_42;
        p[2] = x * w->_13 + y * w->_23 + z * w->_33 + w->_43;
        int r = card_piece((int)k);
        g_card.cx[r] += p[0]; g_card.cy[r] += p[1]; g_card.n[r]++;
    }
    // each laid flat along the camera's view and stood up across the sun's rays about its middle
    for (DWORD k = 0; k < nv; k++) {
        float *p = (float *)(g_card.v + k * 36);
        int r = card_piece((int)k);
        float cx = g_card.cx[r] / (float)g_card.n[r], cy = g_card.cy[r] / (float)g_card.n[r];
        float s = (p[0] - cx) * ax + (p[1] - cy) * ay;
        p[0] = cx + sx * s; p[1] = cy + sy * s;
        if (p[0] < g_sh.lo[0]) g_sh.lo[0] = p[0];
        if (p[0] > g_sh.hi[0]) g_sh.hi[0] = p[0];
        if (p[1] < g_sh.lo[1]) g_sh.lo[1] = p[1];
        if (p[1] > g_sh.hi[1]) g_sh.hi[1] = p[1];
    }
    // drawn into the map only, in the world (no world matrix of its own)
    D3DMATRIX keep = d->world;
    int role = d->sh_role;
    memset(&d->world, 0, sizeof d->world);
    d->world._11 = d->world._22 = d->world._33 = d->world._44 = 1;
    d->sh_role = SH_AWAY;
    d->changed |= RD_CH_VC;
    rd_vb card = { NULL, 0, vb->caps, vb->fvf, nv, 36, nv, g_card.v };
    if (rd3_prepare(d, 0)) rd3_append(d, type, &card, 0, nv, idx, ni);
    d->world = keep;
    d->sh_role = role;
    d->changed |= RD_CH_VC;
}

// the list's draw left out of the frame (rf_replay): only drawn into the casters' map, or nowhere
internal int rd3_shadow_only(int role) { return role == SH_AWAY || role == SH_GONE; }
// the draw's role on the device: a change is a change of the vertex constants (a new batch)
internal void rd3_shadow_classify(rd_device *d, rd_vb *vb, DWORD start, DWORD nv) {
    d->sh_card = 0;
    int role = rd3_shadow_role(d, vb, start, nv);
    rec_count(role, nv);
    if (role != d->sh_role) { d->sh_role = role; d->changed |= RD_CH_VC; }
}

// THE HILLS' SHADOWS (the player, 2026-11-05: "the last step would be terrain shadows"): a map over the whole map of
// the height below which the hills hide the sun - each texel's the higher of its ground and the texel before it toward
// the sun, lowered by the sun's slope over the step between them: one sweep over the map, made again when the sun has
// turned by about a third of a degree (the day's cycle) or the map changed. Everything that receives compares its height
// with it (render_3d.c's shader): the ground, the decals, the water, and - shadows all on - the units, buildings and
// trees, a tall one's top in the sun while its foot is in a hill's shadow. Off the screen too: the map has every hill.
// The ground's corners' heights are the game's own (ground_corner), the world's x and y its tiles.
#define SH_HILL_TEXELS 1024   // the map's side at most (texels): 4 a tile up to a 256-tile map, 2 up to 512
// the sweep, on a thread of its own (a hitch of 10 ms at 840 texels a side in the frame, each time the sun turned on):
// along the axis the sun's rays run more along, a line of texels at a time from the sun's side; the texel before one
// toward the sun lies a step back on that axis and the same distance across it for every texel (between two of the
// line before)
internal DWORD WINAPI shadow_hills_thread(void *arg) {
    (void)arg;
    for (;;) {
        WaitForSingleObject(g_sh.hgo, INFINITE);
        LARGE_INTEGER t0, t1, tf;
        QueryPerformanceCounter(&t0);
        int n = g_sh.hn, hr = g_sh.hr;
        float *c = g_sh.hceil;
        const float *f = g_sh.hsun;
        memcpy(c, g_sh.hground, (size_t)n * n * sizeof(float));
        float fh = sqrtf(f[0] * f[0] + f[1] * f[1]), dx = f[0] / fh, dy = f[1] / fh, slope = -f[2] / fh;
        int xmajor = fabsf(dx) >= fabsf(dy);
        float a = xmajor ? dx : dy, b = xmajor ? dy : dx;
        float t = 1 / fabsf(a), off = -b * t, drop = slope * t / hr;   // texels across, tiles down a step
        int s = a > 0 ? 1 : -1, k0 = (int)floorf(off);
        float fm = off - (float)k0;
        int lo = k0 < 0 ? -k0 : 0, hi = n - 1 - (k0 + 1 > 0 ? k0 + 1 : 0);   // m with both of m + k0, m + k0 + 1 inside
        // a line's step along it, across it (x major: a column - texels n apart, its neighbours 1)
        int along = xmajor ? 1 : n, across = xmajor ? n : 1;
        for (int l = 1; l < n; l++) {
            int at = s > 0 ? l : n - 1 - l;
            float *me = c + (size_t)at * along, *pr = c + (size_t)(at - s) * along;
            for (int m = lo; m <= hi; m++) {
                const float *p = pr + (size_t)(m + k0) * across;
                float h = p[0] + (p[across] - p[0]) * fm - drop, *q = me + (size_t)m * across;
                if (h > *q) *q = h;
            }
        }
        QueryPerformanceCounter(&t1);
        QueryPerformanceFrequency(&tf);
        g_sh.hill_ms = (float)(1000.0 * (double)(t1.QuadPart - t0.QuadPart) / (double)tf.QuadPart);
        InterlockedExchange(&g_sh.hjob, 2);
    }
}
internal void shadow_hills(void) {
    ee_map *map = game_map();
    unsigned w = map ? map_get_size(map) : 0;
    const float *f = g_sh.hlight;
    float fh = sqrtf(f[0] * f[0] + f[1] * f[1]);
    if (g_sh.hfail || !w || w > 2048 || fh < 1e-3f || f[2] >= 0) { g_sh.hill_ok = 0; return; }
    // each texel's ground, once a map (the terrain doesn't change in a game): its middle's height from its tile's four
    // corners; a map under way finished first (its buffers the thread's)
    if (map != g_sh.hmap || (int)w + 1 != g_sh.cw) {
        if (g_sh.hjob == 1) return;
        InterlockedExchange(&g_sh.hjob, 0);
        free(g_sh.hground); free(g_sh.hceil);
        int cw = (int)w + 1, hr = SH_HILL_TEXELS / (int)w;
        if (hr > 4) hr = 4;
        if (hr < 1) hr = 1;
        int n = (int)w * hr;
        float *corner = malloc((size_t)cw * cw * sizeof(float));
        g_sh.hground = malloc((size_t)n * n * sizeof(float));
        g_sh.hceil = malloc((size_t)n * n * sizeof(float));
        g_sh.hmap = map; g_sh.cw = cw; g_sh.hr = hr; g_sh.hn = n; g_sh.hill_ok = 0;
        D3D_RELEASE(g_sh.hsrv);
        D3D_RELEASE(g_sh.hill);
        if (!corner || !g_sh.hground || !g_sh.hceil) { free(corner); rd_fail("the hills' shadows' memory", E_OUTOFMEMORY); g_sh.hfail = 1; return; }
        // and the ground's highest and lowest in blocks of SH_BLOCK tiles (the reach off the screen: shr_hook_rows)
        int bw = ((int)w + SH_BLOCK - 1) / SH_BLOCK;
        free(g_sh.bmax);
        g_sh.bmax = malloc((size_t)bw * bw * 2 * sizeof(float));
        g_sh.bw = g_sh.bmax ? bw : 0;
        for (int i = 0; g_sh.bmax && i < bw * bw; i++) { g_sh.bmax[i] = -1e30f; g_sh.bmax[bw * bw + i] = 1e30f; }
        float glow = 1e30f;
        for (int y = 0; y < cw; y++)
            for (int x = 0; x < cw; x++) {
                float h = ground_corner(map, x, y);
                corner[y * cw + x] = isnan(h) ? -1e9f : h;   // off the map: hides nothing
                if (!isnan(h)) glow = fminf(glow, h);
                if (isnan(h) || !g_sh.bmax) continue;
                int b = (y / SH_BLOCK < bw ? y / SH_BLOCK : bw - 1) * bw + (x / SH_BLOCK < bw ? x / SH_BLOCK : bw - 1);
                g_sh.bmax[b] = fmaxf(g_sh.bmax[b], h);
                g_sh.bmax[bw * bw + b] = fminf(g_sh.bmax[bw * bw + b], h);
            }
        if (glow < 1e29f) g_sh.glow = glow;
        for (int j = 0; j < n; j++)
            for (int i = 0; i < n; i++) {
                float x = (i + 0.5f) / hr, y = (j + 0.5f) / hr;
                int ix = (int)x, iy = (int)y;
                float fx = x - ix, fy = y - iy;
                const float *k = corner + iy * cw + ix;
                g_sh.hground[j * n + i] = (k[0] * (1 - fx) + k[1] * fx) * (1 - fy) + (k[cw] * (1 - fx) + k[cw + 1] * fx) * fy;
            }
        free(corner);
        D3D11_TEXTURE2D_DESC td = { (UINT)g_sh.hn, (UINT)g_sh.hn, 1, 1, DXGI_FORMAT_R32_FLOAT, { 1, 0 }, D3D11_USAGE_DEFAULT,
                                    D3D11_BIND_SHADER_RESOURCE, 0, 0 };
        HRESULT hr2 = ID3D11Device_CreateTexture2D(g_d3d.dev, &td, NULL, &g_sh.hill);
        if (SUCCEEDED(hr2)) hr2 = ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)g_sh.hill, NULL, &g_sh.hsrv);
        if (FAILED(hr2)) { rd_fail("the hills' shadows", hr2); g_sh.hfail = 1; return; }
    }
    // a map made: into the texture (its sun the one it was made for)
    if (g_sh.hjob == 2) {
        ID3D11DeviceContext_UpdateSubresource(g_d3d.ctx, (ID3D11Resource *)g_sh.hill, 0, NULL, g_sh.hceil,
                                              (UINT)g_sh.hn * sizeof(float), 0);
        g_sh.hill_ok = 1;
        g_sh.hill_made++;
        InterlockedExchange(&g_sh.hjob, 0);
    }
    if (g_sh.hjob || (g_sh.hill_ok && f[0] * g_sh.hsun[0] + f[1] * g_sh.hsun[1] + f[2] * g_sh.hsun[2] > 0.99998f)) return;
    if (!g_sh.hthread) {
        g_sh.hgo = CreateEventA(NULL, FALSE, FALSE, NULL);
        g_sh.hthread = g_sh.hgo ? CreateThread(NULL, 0, shadow_hills_thread, NULL, 0, NULL) : NULL;
        if (!g_sh.hthread) { rd_fail("the hills' shadows' thread", E_FAIL); g_sh.hfail = 1; return; }
    }
    memcpy(g_sh.hsun, f, sizeof g_sh.hsun);
    InterlockedExchange(&g_sh.hjob, 1);
    SetEvent(g_sh.hgo);
}

// the shadows' share now: from none with the light 2 degrees over the horizon to all at 12 (a smooth step), reached
// within 1.5 s - no shadow pops in or out, nor turns at once when the light hands over to the moon
internal float shadow_dusk(void) {
    float deg = asinf(fminf(1, fmaxf(0, g_sh.sun_lit))) * 180 / 3.14159265f, t = fminf(1, fmaxf(0, (deg - 2) / 10));
    t = t * t * (3 - 2 * t);
    // by moonlight a share of its own (19 o'clock to 6: the light bluish - the player: "moon should probably be a bit
    // softer 75% maybe")
    float hour = game_hour();
    if (hour >= 19 || hour < 6) t *= g_sh.moon;
    DWORD now = GetTickCount();
    float step = g_sh.dusk_tick ? (float)(now - g_sh.dusk_tick) / 1500.0f : 1;
    g_sh.dusk_tick = now;
    g_sh.dusk = g_sh.dusk < t ? fminf(t, g_sh.dusk + step) : fmaxf(t, g_sh.dusk - step);
    return g_sh.dusk;
}

// Before the frame's list is drawn (rf_replay): its casters from the sun into the map, each batch drawn from where it
// lies in the upload buffers. A list without casters leaves the map as it is (one drawn
// for the interface alone), with ground but no casters it has no shadows.
internal void rd3_shadow_frame(void) {
    if (!g_sh.on || g_sh.ready <= 0) { g_sh.valid = g_sh.hill_ok = 0; return; }
    int nc = 0, ng = 0;
    for (int i = 0; i < g_rf.n; i++) {
        const rf_cmd *c = &g_rf.cmd[i];
        if (c->topo != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) continue;
        if (c->role == SH_CAST || c->role == SH_TINT || c->role == SH_AWAY) nc++;
        else if (c->role == SH_RECEIVE || c->role == SH_WATER) ng++;
    }
    // the water's level: its vertices' mean height, and the fading under it on - for the frame, not this list: a
    // frame's list is drawn in parts (a texture uploaded that it draws with, ...), and a part drawn after the water
    // had none of it and switched the fading off again - so did a frame drawn during a step, which draws the last
    // update's water again, unmeasured (render_frame.c rf_ground_again): the ships' shadows on the seabed at full
    // strength in those, faded in the others, a flicker at the steps' pace (the player, 2026-11-06: "the shadows of
    // the cruisers ... flicker severely", "when i slow the game down the pace of flicker also slows down"; the pipe's
    // record: 102 by the ships in a frame with an update, 99 in one during a step, the same 100 with the fading off).
    // Off only after a frame drawn with an update saw no water at all (rd3_shadow_new_frame).
    if (g_sh.water_n) {
        g_sh.level = (float)(g_sh.water_z / g_sh.water_n);
        g_sh.water_on = g_sh.water_frame = 1;
    }
    g_sh.shl[0] = g_sh.level; g_sh.shl[1] = 1 / fmaxf(g_sh.fade, 0.01f); g_sh.shl[2] = g_sh.water_on ? 1.0f : 0.0f;
    g_sh.water_z = 0;
    g_sh.water_n = 0;
    if (g_sh.ground_n)
        for (int a = 0; a < 3; a++) g_sh.ground_mean[a] = (float)(g_sh.ground_c[a] / g_sh.ground_n);
    g_sh.ground_c[0] = g_sh.ground_c[1] = g_sh.ground_c[2] = 0;
    g_sh.ground_n = 0;
    // the ground's place in the world (through the units' camera), the colour a shadow leaves - the ambient light over
    // the ambient and the sun on level ground, as much of it as the strength says (1: the sun all gone) -, the hills'
    if (ng) {
        rd3_rigid_inverse(g_sh.gw, g_sh.view);
        float sun = fmaxf(0, g_sh.sun_lit), strength = g_sh.strength * shadow_dusk();
        for (int a = 0; a < 3; a++) {
            float lit = g_sh.amb[a] + g_sh.diff[a] * sun;
            float k = lit > 0 ? fminf(1, g_sh.amb[a] / lit) : 1;
            g_sh.shk[a] = 1 - strength * (1 - k);
        }
        g_sh.shk[3] = strength;
        shadow_hills();
    }
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
    ID3D11DeviceContext_VSSetShaderResources(x, 5, 1, &g_mo_srv);
    ID3D11DeviceContext_VSSetConstantBuffers(x, 2, 1, &g_sh.cb);
    ID3D11DeviceContext_PSSetConstantBuffers(x, 2, 1, &g_sh.cb);
    ID3D11DeviceContext_PSSetSamplers(x, 0, 1, &g_sh.samp);
    float cb[24];
    memcpy(cb, m, sizeof m);
    for (int k = 18; k < 24; k++) cb[k] = 0;
    float aref = -2;
    ID3D11ShaderResourceView *srv = (ID3D11ShaderResourceView *)1;
    for (int i = 0; i < g_rf.n; i++) {
        const rf_cmd *c = &g_rf.cmd[i];
        if ((c->role != SH_CAST && c->role != SH_TINT && c->role != SH_AWAY) || c->topo != D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST) continue;
        const rd3_pcb *pc = &g_rf.pcs[c->pc];
        float a = pc->aop[3] && c->srv ? pc->misc[0] / 255.0f : -1;   // alpha tested: its texture's alpha
        if (a != aref) {
            aref = a;
            cb[16] = a; cb[17] = a >= 0 ? 1.0f : 0.0f;
            ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_sh.cb, 0, NULL, cb, 0, 0);
        }
        if (c->srv != srv) { srv = c->srv; ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &srv); }
        ID3D11DeviceContext_DrawIndexed(x, c->n, c->at, c->base);
    }
    ID3D11DeviceContext_OMSetRenderTargets(x, 0, NULL, NULL);   // else Direct3D refuses the map as a texture
    rd3_mul(g_sh.gm, g_sh.gw, m);   // the ground (camera space) into the map
    memcpy(g_sh.shm, m, sizeof m);
    g_sh.inv_depth = 1 / wz;
    g_sh.valid = 1;
    g_sh.passes++;
    g_sh.last_cast = nc; g_sh.last_ground = ng;
    memcpy(g_sh.last_lo, g_sh.lo, sizeof g_sh.lo); memcpy(g_sh.last_hi, g_sh.hi, sizeof g_sh.hi);
    g_sh.lo[0] = g_sh.lo[1] = g_sh.lo[2] = 1e30f;
    g_sh.hi[0] = g_sh.hi[1] = g_sh.hi[2] = -1e30f;
}
// a frame begins (render_frame.c rf_ground_frame) - drawn with an update: the water's fading as the last such frame saw
internal void rd3_shadow_new_frame(void) {
    g_sh.water_on = g_sh.water_frame;
    g_sh.water_frame = 0;
}
// a batch's constants as the list draws it: this frame's map - the ground's place in it through the units' camera
internal void rd3_shadow_vc(rd3_vcb *vc) {
    vc->shp[2] = g_sh.valid ? 1.0f : 0.0f;
    if (!g_sh.valid && !(g_sh.hill_ok && !g_sh.nohills)) return;
    int role = (int)vc->shp[0];
    const float *l = g_sh.light;
    if (role == SH_RECEIVE || role == SH_WATER) {
        if (g_sh.valid) rd3_mul(vc->shm, vc->view, g_sh.gm);
        float w[16];
        rd3_mul(w, vc->view, g_sh.gw);
        for (int a = 0; a < 4; a++) { vc->shx[a] = w[a * 4]; vc->shy[a] = w[a * 4 + 1]; vc->shz[a] = w[a * 4 + 2]; }
        // the light in the ground's camera space: a normal n there lies in the world as n times w's rotation
        for (int a = 0; a < 3; a++) vc->shs[a] = w[a * 4] * l[0] + w[a * 4 + 1] * l[1] + w[a * 4 + 2] * l[2];
        vc->shs[3] = role == SH_RECEIVE ? 1.0f : 0.0f;   // the water: as level ground
    } else {
        memcpy(vc->shm, g_sh.shm, sizeof vc->shm);
        for (int a = 0; a < 4; a++) vc->shx[a] = vc->shy[a] = vc->shz[a] = 0;
        vc->shx[0] = vc->shy[1] = vc->shz[2] = 1;
        memcpy(vc->shs, l, sizeof g_sh.light);   // a decal: in the world
        vc->shs[3] = role == SH_DECAL ? 1.0f : 0.0f;
    }
    memcpy(vc->sha, g_sh.amb, sizeof g_sh.amb);
    memcpy(vc->shf, g_sh.diff, sizeof g_sh.diff);
    memcpy(vc->shl, g_sh.shl, sizeof vc->shl);
    memcpy(vc->shk, g_sh.shk, sizeof vc->shk);
    vc->shp[3] = g_sh.inv_depth;
}
internal void rd3_shadow_pc(rd3_pcb *pc) {
    pc->shd[2] = g_sh.valid ? 1.0f : 0.0f;
    memcpy(pc->shk, g_sh.shk, sizeof pc->shk);
    pc->sht[0] = g_sh.water;
    pc->sht[1] = g_sh.all ? 1.0f : 0.0f;
    pc->sht[2] = 0.15f * g_sh.inv_depth;   // a caster against its own shape in the map: the low-poly faces' slopes
    // the hills' map: the world's x, y -> its u, v; the shadow's soft edge and its bias (tiles: the ground's triangles
    // and the map's texels differ a little); on
    pc->shh[0] = g_sh.hn ? 1.0f / (float)(g_sh.hn / g_sh.hr) : 0;
    pc->shh[1] = g_sh.hsoft;
    pc->shh[2] = 0.05f;
    pc->shh[3] = g_sh.hill_ok && !g_sh.nohills ? 1.0f : 0.0f;
}
// the maps for the main pass (rf_replay binds them with the rest)
internal void rd3_shadow_bind(ID3D11DeviceContext *x) {
    ID3D11ShaderResourceView *srv = g_sh.ready > 0 ? g_sh.srv : NULL, *hsrv = g_sh.ready > 0 ? g_sh.hsrv : NULL;
    ID3D11DeviceContext_VSSetShaderResources(x, 2, 1, &srv);
    ID3D11DeviceContext_PSSetShaderResources(x, 2, 1, &srv);
    ID3D11DeviceContext_PSSetShaderResources(x, 4, 1, &hsrv);
    if (g_sh.ready > 0) {
        ID3D11DeviceContext_PSSetSamplers(x, 1, 1, &g_sh.cmp);
        ID3D11DeviceContext_PSSetSamplers(x, 2, 1, &g_sh.lin);
    }
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
    int lo[SHR_MAP], hi[SHR_MAP], olo[SHR_MAP], ohi[SHR_MAP];   // per y: the widened rows' x, the game's
    float reach;                  // the last collection's (tiles)
    float rise;                   // its ground's highest toward the sun over the screen's lowest, within the reach
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
    g_shr.n = g_shr.own = 0;
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
    // each swept toward the sun (the map's tiles are its world's x and y) by the reach, in half tiles: as far as a
    // caster as tall as the tallest, on the highest ground of the blocks that far toward the sun, can cast onto the
    // lowest ground of the screen's (without the blocks - no map of them yet - the tallest on level ground)
    float slope = -f[2] / h, dx = -f[0] / h, dy = -f[1] / h, reach = fminf(g_sh.tall / slope, SHR_REACH);
    g_shr.rise = 0;
    if (g_sh.bmax && g_sh.hmap == map && g_sh.bw == ((int)w + SH_BLOCK - 1) / SH_BLOCK) {
        int bw = g_sh.bw;
        const float *bmin = g_sh.bmax + bw * bw;
        float low = 1e30f;
        for (unsigned long i = 0; i < n; i++) {
            int by = (int)g_shr.y[i] / SH_BLOCK;
            if (by < 0 || by >= bw) continue;
            for (int bx = (int)g_shr.x0[i] / SH_BLOCK; bx <= (int)g_shr.x1[i] / SH_BLOCK && bx < bw; bx++)
                if (bx >= 0) low = fminf(low, bmin[by * bw + bx]);
        }
        reach = 0;
        for (float k = 0.5f; k <= SHR_REACH && low < 1e29f; k += 0.5f) {
            float high = -1e30f;
            for (unsigned long i = 0; i < n; i++) {
                int by = (int)floorf((float)g_shr.y[i] + dy * k) / SH_BLOCK;
                if (by < 0 || by >= bw) continue;
                int b0 = (int)floorf((float)g_shr.x0[i] + dx * k) / SH_BLOCK, b1 = (int)ceilf((float)g_shr.x1[i] + dx * k) / SH_BLOCK;
                for (int bx = b0 < 0 ? 0 : b0; bx <= b1 && bx < bw; bx++) high = fmaxf(high, g_sh.bmax[by * bw + bx]);
            }
            if (high + g_sh.tall - low >= (k - 1) * slope) { reach = k; g_shr.rise = fmaxf(g_shr.rise, high - low); }
        }
    }
    reach += 1;
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
}
// while the rows are widened every object on them passes, the game's own rows' too: one there whose box has just left
// the picture (its tile still in a visible row) lost its shadow with it (the player, 2026-11-05: "the unit and building
// shadow still vanishes the second their exact bounding leave the image")
internal uint8_t __thiscall shr_hook_view(const void *model, const void *viewport, uint8_t flag) {
    return g_shr.n > 0 || model_intersects(model, viewport, flag) || rf_object_again(model, viewport, flag);   // the objects' margin
}
// The display DLLs draw a model only where the engine's test of it against the view passes
// (DX7Rasterizer::DrawModelHierarchy: GERasterizer::IsPerspectiveModelVisible / IsOrthographicModelVisible, their
// imports - the first flag visible, the second the clipping it is drawn with): in the units' render object every model
// the collection handed over passes, clipped (the game's own off the screen too, a few). Their draws wholly off the
// screen cast without being drawn (SH_AWAY). While every model passes, the test isn't asked at all: its answer was
// overruled but its time spent - about 0.7 ms of the units' 12 a frame over the 8000-unit save's army (a sphere against
// six planes, per model). Besides visible and clip it writes only the rasterizer's flags of the planes the model crosses
// (+0x158..0x15D), which nothing reads - not the engine, not either display DLL -, and clip 1 is what the overruled
// ones got: the GPU clips anyway.
typedef void (__thiscall *model_visible_fn)(void *ras, void *model, float lod, void *viewport, void *at, uint8_t *visible,
                                            uint8_t *clip);
typedef void (__thiscall *model_visible_ortho_fn)(void *ras, void *model, void *viewport, void *at, uint8_t *visible,
                                                  uint8_t *clip);
internal model_visible_fn g_shr_visible;
internal model_visible_ortho_fn g_shr_visible_ortho;
internal int shr_pass(void) { return g_shr.on && !g_shr.off && g_sh.on && g_ro_cur == RO_UNITS && draw_list_moved(); }
internal void __thiscall shr_hook_visible(void *ras, void *model, float lod, void *viewport, void *at, uint8_t *visible,
                                          uint8_t *clip) {
    if (shr_pass()) { *visible = 1; *clip = 1; return; }
    g_shr_visible(ras, model, lod, viewport, at, visible, clip);
    if (!*visible && g_ro_cur == RO_UNITS && g_om_margin > 0) {   // the objects' margin (render_frame.c)
        float was[4][3];
        rf_view_widen(viewport, 1.0f + g_om_margin, was);
        g_shr_visible(ras, model, lod, viewport, at, visible, clip);
        rf_view_restore(viewport, was);
        if (*visible) *clip = 1;
    }
}
internal void __thiscall shr_hook_visible_ortho(void *ras, void *model, void *viewport, void *at, uint8_t *visible,
                                                uint8_t *clip) {
    if (shr_pass()) { *visible = 1; *clip = 1; return; }
    g_shr_visible_ortho(ras, model, viewport, at, visible, clip);
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
    if (!strcmp(mode, "hour")) {   // the game's time of day (as the daylight-hour effect sets it)
        if (s >= 0 && s < 24) game_set_hour(s);
        else { REPLY("error: usage: shadows hour <0..24>\n"); return; }
    } else if (!strcmp(mode, "tint")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_sh.tint = arg[1] == 'n';
        else { REPLY("error: usage: shadows tint on | off\n"); return; }
    } else if (!strcmp(mode, "all")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_sh.all = arg[1] == 'n';
        else { REPLY("error: usage: shadows all on | off\n"); return; }
    } else if (!strcmp(mode, "hillsoft")) {
        if (s > 0 && s <= 8) g_sh.hsoft = s;
        else { REPLY("error: usage: shadows hillsoft <tiles below the ground's ceiling a hill's shadow is full at, 0..8>\n"); return; }
    } else if (!strcmp(mode, "hills")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_sh.nohills = arg[1] != 'n';
        else { REPLY("error: usage: shadows hills on | off\n"); return; }
    } else if (!strcmp(mode, "cards")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_sh.nocards = arg[1] != 'n';
        else { REPLY("error: usage: shadows cards on | off\n"); return; }
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
    } else if (!strcmp(mode, "water")) {
        if (s >= 0 && s <= 1) g_sh.water = s;
        else { REPLY("error: usage: shadows water <how much of a shadow the water's surface takes, 0..1>\n"); return; }
    } else if (!strcmp(mode, "moon")) {
        if (s >= 0 && s <= 1) g_sh.moon = s;
        else { REPLY("error: usage: shadows moon <a shadow's share by moonlight, 0..1>\n"); return; }
    } else if (!strcmp(mode, "game")) g_sh.on = 0;
    else if (!strcmp(mode, "dynamic")) g_sh.on = 1;
    else if (mode[0]) { REPLY("error: usage: shadows [game | dynamic] [strength 0..1] | fade <depth> | water <0..1> | moon <0..1> | sun <degrees> | height <degrees> | turn <degrees> | hour <0..24> | reach on | off | cards on | off | all on | off | hills on | off | hillsoft <tiles> | tint on | off\n"); return; }
    if ((!strcmp(mode, "game") || !strcmp(mode, "dynamic")) && s >= 0 && s <= 1) g_sh.strength = s;
    REPLY("shadows: %s, strength %.2f, %s, %ld maps drawn; the last: %d casting batches, %d of ground;"
          " the casters' box %.1f %.1f %.1f - %.1f %.1f %.1f, the sun %.2f %.2f %.2f, a shadow's colour %.2f %.2f %.2f, the light's %.2f %.2f %.2f, the hour %.2f;"
          " the water at %.2f%s, a shadow gone %.2f under it, %.2f of it on the surface; units in a shadow darkened: %s, receiving: %s; by the light's height %.2f of a shadow (by moonlight at most %.2f); the sun at least %.0f degrees"
          " high (the light's %.0f); casters off the screen: %s, %.1f tiles (the tallest %.1f, the ground's rise %.1f), %d rows and %d more; %d casting draws off the screen;"
          " the ground's mean colour %.0f %.0f %.0f; the hills' shadows: %s (full %.2f tiles under the ceiling), %dx%d texels (%d a tile), made %ld times, the last in %.1f ms\n",
          g_sh.on ? "dynamic" : "game", (double)g_sh.strength,
          g_sh.ready > 0 ? "ready" : g_sh.ready < 0 ? "FAILED (the log says why)" : "not made yet", (long)g_sh.passes,
          g_sh.last_cast, g_sh.last_ground, (double)g_sh.last_lo[0], (double)g_sh.last_lo[1], (double)g_sh.last_lo[2],
          (double)g_sh.last_hi[0], (double)g_sh.last_hi[1], (double)g_sh.last_hi[2], (double)g_sh.sun[0],
          (double)g_sh.sun[1], (double)g_sh.sun[2], (double)g_sh.shk[0], (double)g_sh.shk[1], (double)g_sh.shk[2], (double)g_sh.diff[0], (double)g_sh.diff[1], (double)g_sh.diff[2], (double)game_hour(),
          (double)g_sh.level, g_sh.shl[2] > 0 ? "" : " (none seen)", (double)g_sh.fade, (double)g_sh.water, g_sh.tint ? "on" : "off", g_sh.all ? "everything" : "the ground", (double)g_sh.dusk, (double)g_sh.moon,
          (double)g_sh.sun_low, (double)(asinf(fminf(1, fmaxf(-1, g_sh.sun_lit))) * 180 / 3.14159265f),
          !g_shr.on ? "not (no hooks)" : g_shr.off ? "off" : "collected", (double)g_shr.reach, (double)g_sh.tall, (double)g_shr.rise, g_shr.last_own, g_shr.last_extra, g_sh.last_off,
          (double)g_sh.ground_mean[0], (double)g_sh.ground_mean[1], (double)g_sh.ground_mean[2],
          g_sh.nohills ? "off" : g_sh.hill_ok ? "on" : "not made", (double)g_sh.hsoft, g_sh.hn, g_sh.hn, g_sh.hr, (long)g_sh.hill_made, (double)g_sh.hill_ms);
}
