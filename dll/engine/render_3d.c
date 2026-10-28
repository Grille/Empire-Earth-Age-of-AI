// The own renderer's Direct3D 7 (render.c, docs/renderer.md): Direct3D7 (devices, vertex buffers, depth formats),
// the device (its states as the game sets them, Clear) and vertex buffers (memory the game writes, 36-byte vertices of
// FVF 0x152 in the game). The draws go to Direct3D 11 through one vertex and one pixel shader that do the fixed
// function the game uses: world-view-projection, directional lights with the material sources, vertex fog, texture
// stage 0's operations, alpha test and the colour key; the depth / blend / raster states and samplers are made once
// per combination, the vertices and indices of each draw copied into one upload buffer.

// ---- vertex buffers ----
typedef struct { void **vt; LONG refs; DWORD caps, fvf, n, stride; uint8_t *mem; } rd_vb;
internal DWORD rd_fvf_stride(DWORD fvf) {
    DWORD s = 0;
    switch (fvf & D3DFVF_POSITION_MASK) {
    case D3DFVF_XYZ: s = 12; break;
    case D3DFVF_XYZRHW: s = 16; break;
    case D3DFVF_XYZB1: s = 16; break;
    case D3DFVF_XYZB2: s = 20; break;
    case D3DFVF_XYZB3: s = 24; break;
    case D3DFVF_XYZB4: s = 28; break;
    case D3DFVF_XYZB5: s = 32; break;
    }
    if (fvf & D3DFVF_NORMAL) s += 12;
    if (fvf & D3DFVF_RESERVED1) s += 4;
    if (fvf & D3DFVF_DIFFUSE) s += 4;
    if (fvf & D3DFVF_SPECULAR) s += 4;
    DWORD tex = (fvf & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;
    for (DWORD i = 0; i < tex; i++) {
        DWORD f = (fvf >> (16 + i * 2)) & 3;   // D3DFVF_TEXCOORDSIZEn: 0 two, 1 three, 2 four, 3 one
        s += f == 0 ? 8 : f == 1 ? 12 : f == 2 ? 16 : 4;
    }
    return s;
}
internal HRESULT __stdcall rd_vb_qi(rd_vb *b, const GUID *iid, void **out) { (void)b; (void)iid; *out = NULL; return E_NOINTERFACE; }
internal ULONG __stdcall rd_vb_addref(rd_vb *b) { return (ULONG)InterlockedIncrement(&b->refs); }
internal ULONG __stdcall rd_vb_release(rd_vb *b) {
    LONG n = InterlockedDecrement(&b->refs);
    if (n > 0) return (ULONG)n;
    free(b->mem);
    free(b);
    return 0;
}
internal HRESULT __stdcall rd_vb_lock(rd_vb *b, DWORD flags, void **p, DWORD *size) {
    (void)flags;
    *p = b->mem;
    if (size) *size = b->n * b->stride;
    return D3D_OK;
}
internal HRESULT __stdcall rd_vb_unlock(rd_vb *b) { (void)b; return D3D_OK; }
internal HRESULT __stdcall rd_vb_get_desc(rd_vb *b, D3DVERTEXBUFFERDESC *d) {
    d->dwCaps = b->caps; d->dwFVF = b->fvf; d->dwNumVertices = b->n;
    return D3D_OK;
}
internal HRESULT __stdcall rd_vb_optimize(rd_vb *b, void *dev, DWORD flags) { (void)b; (void)dev; (void)flags; return D3D_OK; }

// ---- the device ----
#define RD_RS 256
#define RD_TSS 32
typedef struct {
    void **vt;
    LONG refs;
    rd_surface *target;
    D3DMATRIX world, view, proj;
    D3DVIEWPORT7 viewport;
    D3DMATERIAL7 material;
    D3DLIGHT7 light[8];
    int light_on[8];
    DWORD rs[RD_RS];
    DWORD tss[2][RD_TSS];
    rd_surface *tex[2];
    int in_scene;
    int changed;   // states other than the world matrix set to other values since rd3_prepare last ran (RD_CH_)
} rd_device;
enum { RD_CH_VC = 1, RD_CH_RS = 2, RD_CH_TEX = 4, RD_CH_TARGET = 8 };   // transforms, viewport, lights, material: the vertex constants
internal volatile LONG g_rd_draws, g_rd_frames;
internal DWORD g_rd_minute_tick;
internal LONGLONG g_rd_draw_qpc, g_rd_draw_qpc_told;   // time inside the draw calls (the game's thread), for the minute's line
internal LONGLONG rd_qpc(void) { LARGE_INTEGER t; QueryPerformanceCounter(&t); return t.QuadPart; }
internal LONGLONG rd_qpc_ms(int ms) {
    static LONGLONG per_ms;
    if (!per_ms) { LARGE_INTEGER f; QueryPerformanceFrequency(&f); per_ms = f.QuadPart / 1000; }
    return per_ms * ms;
}

// ---- the 3D pipeline ----
// the vertex shader's constants (D3D7's row vectors: v * world * view * projection, row_major in HLSL); the world
// matrix is the vertex's own (its index into the batch's table, cbuffer w), so draws that differ only in it batch
typedef struct {
    float vpm[16], view[16];   // view x projection, view
    float ldir[8][4], ldiff[8][4], lamb[8][4];
    float mdiff[4], mamb[4], memi[4], gamb[4];
    int32_t lit[4];   // lighting, lights, normalize normals
    int32_t src[4];   // diffuse, ambient, emissive from the vertex colour (1) or the material (0)
    float fog[4];     // mode (0 none, 1 exp, 2 exp2, 3 linear), start, end, density
    float vp[4];      // half a pixel in clip space (x, y), range fog
} rd3_vcb;
// the pixel shader's: stage 0's colour op and arguments, texture bound; alpha op and arguments, alpha function
typedef struct {
    int32_t op[4], aop[4];
    float fogc[4], tf[4];
    float misc[4];   // alpha reference, fog on, colour key
} rd3_pcb;
internal const char k_rd3_shader[] =
    "cbuffer v : register(b0) { row_major float4x4 vpm; row_major float4x4 view;"
    " float4 ldir[8]; float4 ldiff[8]; float4 lamb[8]; float4 mdiff, mamb, memi, gamb; int4 lit; int4 src; float4 fog;"
    " float4 vp; };\n"
    "cbuffer p : register(b1) { int4 op; int4 aop; float4 fogc; float4 tf; float4 misc; };\n"
    "Buffer<float4> wt : register(t1);\n"   // the world matrices, 4 rows each (RD3_WB_BYTES)
    "Texture2D t : register(t0); SamplerState s : register(s0);\n"
    "struct I { float3 p : POSITION; float3 n : NORMAL; float4 c : COLOR; float2 t : TEXCOORD; uint w : WORLD; };\n"
    "struct O { float4 p : SV_Position; float4 c : COLOR; float2 t : TEXCOORD0; float f : FOG; };\n"
    "O vs(I i) { O o; uint r = i.w * 4; float4x4 world = float4x4(wt.Load(r), wt.Load(r + 1), wt.Load(r + 2), wt.Load(r + 3));"
    " float4 p = mul(float4(i.p, 1), world); o.p = mul(p, vpm);"
    " o.p.xy += vp.xy * o.p.w;"
    " float4 vc = i.c.zyxw, c = vc;"   // D3DCOLOR's bytes are B G R A
    " if (lit.x) { float4 d = src.x ? vc : mdiff, a = src.y ? vc : mamb, e = src.z ? vc : memi;"
    "  float3 n = mul(i.n, (float3x3)world); if (lit.z) n = normalize(n);"
    "  float3 am = gamb.rgb, df = 0;"
    "  for (int k = 0; k < lit.y; k++) { am += lamb[k].rgb; df += ldiff[k].rgb * max(0, dot(n, -ldir[k].xyz)); }"
    "  c = float4(saturate(e.rgb + a.rgb * am + d.rgb * df), d.a); }"
    " o.c = c; o.t = i.t;"
    " float3 q = mul(p, view).xyz; float z = vp.z > 0.5 ? length(q) : q.z, f = 1;"
    " if (fog.x == 3) f = (fog.z - z) / (fog.z - fog.y); else if (fog.x == 1) f = exp(-fog.w * z);"
    " else if (fog.x == 2) f = exp(-(fog.w * z) * (fog.w * z));"
    " o.f = f; return o; }\n"   // clamped per pixel, as the driver under a wrapper does (the picture compare)
    // D3DTA_: 0 diffuse, 1 current (the diffuse at stage 0), 2 texture, 3 texture factor; D3DTOP_ 1..8
    "float4 arg(int a, float4 x, float4 d) { a = a & 15; return a == 2 ? x : a == 3 ? tf : d; }\n"
    "float4 top(int o, float4 a, float4 b) { return o == 2 ? a : o == 3 ? b : o == 4 ? a * b : o == 5 ? saturate(a * b * 2)"
    " : o == 6 ? saturate(a * b * 4) : o == 7 ? saturate(a + b) : o == 8 ? saturate(a + b - 0.5) : a * b; }\n"
    "float4 ps(O i) : SV_Target { float4 x = op.w ? t.Sample(s, i.t) : float4(1, 1, 1, 1), d = i.c, c = d;"
    " if (misc.z > 0.5 && x.a == 0) discard;"
    " if (op.x != 1) { c.rgb = top(op.x, arg(op.y, x, d), arg(op.z, x, d)).rgb;"
    "  if (aop.x != 1) c.a = top(aop.x, arg(aop.y, x, d), arg(aop.z, x, d)).a; }"
    " int A = (int)round(c.a * 255), R = (int)misc.x, f = aop.w;"
    " if (f && !(f == 8 || (f == 2 && A < R) || (f == 3 && A == R) || (f == 4 && A <= R) || (f == 5 && A > R)"
    "  || (f == 6 && A != R) || (f == 7 && A >= R))) discard;"
    " if (misc.y > 0.5) c.rgb = lerp(fogc.rgb, c.rgb, saturate(i.f));"
    " return c; }\n";

#define RD3_VB_BYTES (4u << 20)
#define RD3_IB_BYTES (1u << 20)
#define RD3_WB_BYTES (1u << 20)   // the batches' world matrices, appended like the vertices (no discard a batch)
// THE BATCH: the game draws an object at a time (5000 draws a frame in a busy base, each a few dozen triangles), and
// a draw call costs its time on the game's thread whatever its size. Draws in a row whose states match - the same
// texture, blend, material ... - differing only in their world matrix are collected and drawn as one: their vertices
// (with their matrix's index in the batch's table) and indices into the upload buffers, then one DrawIndexed. Strips
// become lists (one batch takes many). Whatever else uses the context sends the batch first (rd3_flush): a state that
// changes (rd3_prepare), a texture's upload, the 2D path, clear, read back, present, the overlay, a texture freed.
#define RB_VERTS 32768
#define RB_INDICES (RB_VERTS * 3)
#define RB_WORLDS 256
typedef struct { uint8_t fvf[36]; uint32_t w; } rb_vertex;   // FVF 0x152 and the world's index
internal struct {
    rb_vertex *v;
    uint32_t *i;
    float (*w)[16];
    int nv, ni, nw;
    D3D11_PRIMITIVE_TOPOLOGY topo;
    LONG batches;   // draw calls made (the minute's line)
} g_rb;
typedef struct { uint32_t key; void *obj; } rd3_cached;
internal struct {
    int ready, bound;
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps;
    ID3D11InputLayout *inputs;
    ID3D11Buffer *vcb, *pcb, *wb, *vb, *ib;
    ID3D11ShaderResourceView *wsrv;
    UINT vb_pos, ib_pos, wb_pos;
    rd3_vcb vc;
    rd3_pcb pc;
    // what is bound (rebound in full after the 2D path or the present - rd3_dirty)
    ID3D11RenderTargetView *rtv;
    ID3D11DepthStencilView *dsv;
    void *blend, *depth, *raster, *samp;
    ID3D11ShaderResourceView *srv;
    D3D11_VIEWPORT vport;
    rd3_cached blends[32], depths[32], rasters[8], samps[32];
    int n_blend, n_depth, n_raster, n_samp;
    // the map's camera this frame (render_overlay.c draws with it): view x projection, the viewport; the overlay drawn
    float scene_vp[16];
    D3D11_VIEWPORT scene_port;
    int scene_seen, overlay_done;
    rd_device *prep_dev;   // whose states are bound (rd3_prepare), with which clamp
    int prep_clamp;
} g_rd3;

internal void rd3_dirty(void) { g_rd3.bound = 0; }   // the 2D draw or the present set states of their own

internal int rd3_init(void) {
    if (g_rd3.ready) return g_rd3.ready > 0;
    g_rd3.ready = -1;
    if (!g_d3d.dev) return 0;
    ID3DBlob *code = d3d_compile(k_rd3_shader, sizeof k_rd3_shader - 1, "vs", "vs_4_0");
    HRESULT hr = code ? ID3D11Device_CreateVertexShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_rd3.vs) : E_FAIL;
    static const D3D11_INPUT_ELEMENT_DESC el[] = {   // FVF 0x152
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 28, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "WORLD", 0, DXGI_FORMAT_R32_UINT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 } };   // rb_vertex
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateInputLayout(g_d3d.dev, el, 5, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), &g_rd3.inputs);
    D3D_RELEASE(code);
    code = SUCCEEDED(hr) ? d3d_compile(k_rd3_shader, sizeof k_rd3_shader - 1, "ps", "ps_4_0") : NULL;
    hr = code ? ID3D11Device_CreatePixelShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_rd3.ps) : E_FAIL;
    D3D_RELEASE(code);
    D3D11_BUFFER_DESC bd = { sizeof(rd3_vcb), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_rd3.vcb);
    bd.ByteWidth = sizeof(rd3_pcb);
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_rd3.pcb);
    D3D11_BUFFER_DESC ud = { RD3_VB_BYTES, D3D11_USAGE_DYNAMIC, D3D11_BIND_VERTEX_BUFFER, D3D11_CPU_ACCESS_WRITE, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(g_d3d.dev, &ud, NULL, &g_rd3.vb);
    ud.ByteWidth = RD3_IB_BYTES; ud.BindFlags = D3D11_BIND_INDEX_BUFFER;
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(g_d3d.dev, &ud, NULL, &g_rd3.ib);
    ud.ByteWidth = RD3_WB_BYTES; ud.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(g_d3d.dev, &ud, NULL, &g_rd3.wb);
    D3D11_SHADER_RESOURCE_VIEW_DESC sd = { .Format = DXGI_FORMAT_R32G32B32A32_FLOAT, .ViewDimension = D3D11_SRV_DIMENSION_BUFFER };
    sd.Buffer.NumElements = RD3_WB_BYTES / 16;
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)g_rd3.wb, &sd, &g_rd3.wsrv);
    if (FAILED(hr)) return rd_fail("the 3D pipeline", hr), 0;
    g_rb.v = malloc(RB_VERTS * sizeof *g_rb.v);
    g_rb.i = malloc(RB_INDICES * sizeof *g_rb.i);
    g_rb.w = malloc(RB_WORLDS * sizeof *g_rb.w);
    if (!g_rb.v || !g_rb.i || !g_rb.w) return rd_fail("the 3D pipeline's batch", E_OUTOFMEMORY), 0;
    g_rd3.vb_pos = RD3_VB_BYTES;   // the first write discards
    g_rd3.ib_pos = RD3_IB_BYTES;
    g_rd3.wb_pos = RD3_WB_BYTES;
    g_rd3.ready = 1;
    return 1;
}

// a state object for key, made by mk the first time
internal void *rd3_cache(rd3_cached *c, int *n, int cap, uint32_t key, void *(*mk)(uint32_t)) {
    for (int i = 0; i < *n; i++)
        if (c[i].key == key) return c[i].obj;
    void *o = mk(key);
    if (o && *n < cap) { c[*n].key = key; c[*n].obj = o; (*n)++; }
    return o;   // past the cap: made again each time (never with the game's ~10 combinations)
}
// D3DBLEND (1..11 as Direct3D 11's; 12 / 13 both-alpha) -> Direct3D 11, for colour or for alpha
internal D3D11_BLEND rd3_blend_op(DWORD b, int src, int alpha) {
    if (b == D3DBLEND_BOTHSRCALPHA) b = src ? D3DBLEND_SRCALPHA : D3DBLEND_INVSRCALPHA;
    else if (b == D3DBLEND_BOTHINVSRCALPHA) b = src ? D3DBLEND_INVSRCALPHA : D3DBLEND_SRCALPHA;
    if (b < 1 || b > 11) b = src ? D3DBLEND_ONE : D3DBLEND_ZERO;
    if (alpha && (b == 3 || b == 4 || b == 9 || b == 10)) b += 2;   // colour factors -> their alpha's
    return (D3D11_BLEND)b;
}
internal void *rd3_mk_blend(uint32_t key) {
    D3D11_BLEND_DESC bl = { 0 };
    D3D11_RENDER_TARGET_BLEND_DESC *r = &bl.RenderTarget[0];
    r->BlendEnable = key & 1;
    DWORD s = key >> 1 & 15, d = key >> 5 & 15;
    r->SrcBlend = rd3_blend_op(s, 1, 0); r->DestBlend = rd3_blend_op(d, 0, 0); r->BlendOp = D3D11_BLEND_OP_ADD;
    r->SrcBlendAlpha = rd3_blend_op(s, 1, 1); r->DestBlendAlpha = rd3_blend_op(d, 0, 1); r->BlendOpAlpha = D3D11_BLEND_OP_ADD;
    r->RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    ID3D11BlendState *o = NULL;
    ID3D11Device_CreateBlendState(g_d3d.dev, &bl, &o);
    return o;
}
internal void *rd3_mk_depth(uint32_t key) {   // enable, write, D3DCMP (as Direct3D 11's)
    D3D11_DEPTH_STENCIL_DESC ds = { 0 };
    ds.DepthEnable = key & 1;
    ds.DepthWriteMask = key & 2 ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
    DWORD f = key >> 2 & 15;
    ds.DepthFunc = f >= 1 && f <= 8 ? (D3D11_COMPARISON_FUNC)f : D3D11_COMPARISON_LESS_EQUAL;
    ID3D11DepthStencilState *o = NULL;
    ID3D11Device_CreateDepthStencilState(g_d3d.dev, &ds, &o);
    return o;
}
internal void *rd3_mk_raster(uint32_t key) {   // D3DCULL: 1 none, 2 clockwise, 3 counter-clockwise (D3D11: clockwise is the front)
    D3D11_RASTERIZER_DESC rs = { 0 };
    rs.FillMode = D3D11_FILL_SOLID;
    rs.CullMode = key == D3DCULL_CW ? D3D11_CULL_FRONT : key == D3DCULL_CCW ? D3D11_CULL_BACK : D3D11_CULL_NONE;
    rs.DepthClipEnable = TRUE;
    ID3D11RasterizerState *o = NULL;
    ID3D11Device_CreateRasterizerState(g_d3d.dev, &rs, &o);
    return o;
}
// D3DTADDRESS (1..4 as Direct3D 11's), D3DTFG / D3DTFN (point, linear; anisotropic as linear), D3DTFP (none, point,
// linear), the mip bias in 1/16
internal void *rd3_mk_samp(uint32_t key) {
    D3D11_SAMPLER_DESC sd = { 0 };
    DWORD u = key & 7, v = key >> 3 & 7, mag = key >> 6 & 3, min = key >> 8 & 3, mip = key >> 10 & 3;
    sd.AddressU = u >= 1 && u <= 4 ? (D3D11_TEXTURE_ADDRESS_MODE)u : D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV = v >= 1 && v <= 4 ? (D3D11_TEXTURE_ADDRESS_MODE)v : D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.Filter = (D3D11_FILTER)((min >= 2 ? 0x10 : 0) | (mag >= 2 ? 0x4 : 0) | (mip == 3 ? 0x1 : 0));
    sd.MipLODBias = (float)(int8_t)(key >> 12 & 255) / 16.0f;
    sd.MaxAnisotropy = 1;
    sd.ComparisonFunc = D3D11_COMPARISON_NEVER;
    sd.MaxLOD = mip == D3DTFP_NONE || mip == 0 ? 0 : D3D11_FLOAT32_MAX;
    ID3D11SamplerState *o = NULL;
    ID3D11Device_CreateSamplerState(g_d3d.dev, &sd, &o);
    return o;
}

internal void rd3_mul(float *r, const float *a, const float *b) {   // r = a * b, 4x4 row-major
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 4; j++)
            r[i * 4 + j] = a[i * 4] * b[j] + a[i * 4 + 1] * b[4 + j] + a[i * 4 + 2] * b[8 + j] + a[i * 4 + 3] * b[12 + j];
}
internal void rd3_color(float *f, D3DCOLOR c) {
    f[0] = (float)(c >> 16 & 255) / 255.0f; f[1] = (float)(c >> 8 & 255) / 255.0f;
    f[2] = (float)(c & 255) / 255.0f; f[3] = (float)(c >> 24) / 255.0f;
}
internal void rd3_value(float *f, const D3DCOLORVALUE *c) { f[0] = c->r; f[1] = c->g; f[2] = c->b; f[3] = c->a; }
internal float rd3_float(DWORD v) { float f; memcpy(&f, &v, 4); return f; }

// A picture on the screen (the interface's backgrounds: no depth test, every texture coordinate within 0..1) is
// sampled with CLAMP whatever the game asks: a background is tiles of 256x256 textures stretched to the screen, and with
// WRAP the bilinear filter takes its edge's half texel from the tile's opposite side - lines between the tiles (under
// DDrawCompat too). Inside the tile both are the same; the terrain (depth tested) tiles its textures and keeps WRAP.
internal int rd3_flat_picture(rd_device *d, rd_vb *vb, DWORD start, DWORD n) {
    if (d->rs[D3DRENDERSTATE_ZENABLE] || !d->tex[0] || n > 64) return 0;
    for (DWORD i = start; i < start + n; i++) {
        const float *uv = (const float *)(vb->mem + (size_t)i * vb->stride + 28);   // FVF 0x152's texture coordinates
        if (uv[0] < -0.001f || uv[0] > 1.001f || uv[1] < -0.001f || uv[1] > 1.001f) return 0;
    }
    return 1;
}
// the device's states into the shaders' constants and the pipeline; 0: nothing to draw into. clamp: CLAMP addressing
internal int rd3_prepare(rd_device *d, int clamp) {
    rd_surface *t = d->target;
    if (!t || !t->rtv || !rd3_init()) return 0;
    // nothing but the world matrix changed since the last draw (the game sets it for each object, most other states
    // over and over to the values they have): the states bound are this draw's - the work below cost more than the draw
    rd_surface *tx = d->tex[0] ? (d->tex[0]->top ? d->tex[0]->top : d->tex[0]) : NULL;
    if (!d->changed && g_rd3.bound && g_rd3.prep_dev == d && g_rd3.prep_clamp == clamp && g_rd3.rtv == t->rtv
        && g_rd3.dsv == (t->zbuf ? t->zbuf->dsv : NULL) && !(tx && tx->kind != RK_TARGET && tx->dirty))
        return 1;
    ID3D11DeviceContext *x = g_d3d.ctx;
    const DWORD *rs = d->rs;
    int full = !g_rd3.bound;   // (whoever unbound flushed before)
    if (full) {
        ID3D11DeviceContext_IASetInputLayout(x, g_rd3.inputs);
        ID3D11DeviceContext_VSSetShader(x, g_rd3.vs, NULL, 0);
        ID3D11DeviceContext_PSSetShader(x, g_rd3.ps, NULL, 0);
        ID3D11DeviceContext_VSSetConstantBuffers(x, 0, 1, &g_rd3.vcb);
        ID3D11DeviceContext_VSSetShaderResources(x, 1, 1, &g_rd3.wsrv);
        ID3D11DeviceContext_PSSetConstantBuffers(x, 1, 1, &g_rd3.pcb);
        UINT stride = sizeof(rb_vertex), off = 0;
        ID3D11DeviceContext_IASetVertexBuffers(x, 0, 1, &g_rd3.vb, &stride, &off);
        ID3D11DeviceContext_IASetIndexBuffer(x, g_rd3.ib, DXGI_FORMAT_R32_UINT, 0);
        g_rd3.bound = 1;
    }
    // each state: the batch so far drawn with the old one before it changes
    ID3D11DepthStencilView *dsv = t->zbuf ? t->zbuf->dsv : NULL;
    if (full || g_rd3.rtv != t->rtv || g_rd3.dsv != dsv) {
        rd3_flush();
        ID3D11DeviceContext_OMSetRenderTargets(x, 1, &t->rtv, dsv);
        g_rd3.rtv = t->rtv; g_rd3.dsv = dsv;
    }
    D3D11_VIEWPORT vp = { (float)d->viewport.dwX, (float)d->viewport.dwY, (float)d->viewport.dwWidth,
                          (float)d->viewport.dwHeight, d->viewport.dvMinZ, d->viewport.dvMaxZ };
    if (full || memcmp(&vp, &g_rd3.vport, sizeof vp)) { rd3_flush(); ID3D11DeviceContext_RSSetViewports(x, 1, &vp); g_rd3.vport = vp; }
    uint32_t bk = rs[D3DRENDERSTATE_ALPHABLENDENABLE] ? 1 | (rs[D3DRENDERSTATE_SRCBLEND] & 15) << 1 | (rs[D3DRENDERSTATE_DESTBLEND] & 15) << 5 : 0;
    void *o = rd3_cache(g_rd3.blends, &g_rd3.n_blend, 32, bk, rd3_mk_blend);
    if (full || o != g_rd3.blend) { rd3_flush(); ID3D11DeviceContext_OMSetBlendState(x, o, NULL, 0xFFFFFFFF); g_rd3.blend = o; }
    uint32_t dk = (rs[D3DRENDERSTATE_ZENABLE] ? 1 : 0) | (rs[D3DRENDERSTATE_ZWRITEENABLE] ? 2 : 0) | (rs[D3DRENDERSTATE_ZFUNC] & 15) << 2;
    o = rd3_cache(g_rd3.depths, &g_rd3.n_depth, 32, dk, rd3_mk_depth);
    if (full || o != g_rd3.depth) { rd3_flush(); ID3D11DeviceContext_OMSetDepthStencilState(x, o, 0); g_rd3.depth = o; }
    o = rd3_cache(g_rd3.rasters, &g_rd3.n_raster, 8, rs[D3DRENDERSTATE_CULLMODE] & 3, rd3_mk_raster);
    if (full || o != g_rd3.raster) { rd3_flush(); ID3D11DeviceContext_RSSetState(x, o); g_rd3.raster = o; }

    // texture stage 0 (stage 1 is always off in the game)
    const DWORD *ts = d->tss[0];
    rd_surface *tex = d->tex[0];
    int ckey = rs[D3DRENDERSTATE_COLORKEYENABLE] && tex && (tex->top ? tex->top : tex)->has_key;
    ID3D11ShaderResourceView *srv = tex ? rd_gpu_texture(tex, ckey) : NULL;
    if (full || srv != g_rd3.srv) { rd3_flush(); ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &srv); g_rd3.srv = srv; }
    int bias = (int)(rd3_float(ts[D3DTSS_MIPMAPLODBIAS]) * 16.0f);
    uint32_t au = clamp ? D3DTADDRESS_CLAMP : ts[D3DTSS_ADDRESSU], av = clamp ? D3DTADDRESS_CLAMP : ts[D3DTSS_ADDRESSV];
    uint32_t sk = (au & 7) | (av & 7) << 3 | (ts[D3DTSS_MAGFILTER] & 3) << 6 |
                  (ts[D3DTSS_MINFILTER] & 3) << 8 | (ts[D3DTSS_MIPFILTER] & 3) << 10 | (uint32_t)(bias & 255) << 12;
    o = rd3_cache(g_rd3.samps, &g_rd3.n_samp, 32, sk, rd3_mk_samp);
    if (full || o != g_rd3.samp) { rd3_flush(); ID3D11DeviceContext_PSSetSamplers(x, 0, 1, (ID3D11SamplerState **)&o); g_rd3.samp = o; }

    // the vertex shader's constants - not when only a texture or its stage changed (most often, between two batches)
    if (full || g_rd3.prep_dev != d || (d->changed & (RD_CH_VC | RD_CH_RS))) {
        rd3_vcb vc;
        memset(&vc, 0, sizeof vc);
        rd3_mul(vc.vpm, &d->view._11, &d->proj._11);
        memcpy(vc.view, &d->view._11, sizeof vc.view);
        if (rs[D3DRENDERSTATE_ZENABLE] && (d->view._41 != 0 || d->view._42 != 0 || d->view._43 != 0)) {
            // the map's camera (a view away from the origin - the interface's 3D pieces have none): the overlay's
            memcpy(g_rd3.scene_vp, vc.vpm, sizeof vc.vpm);
            g_rd3.scene_port = vp;
            g_rd3.scene_seen = 1;
        }
        int lights = 0;
        if (rs[D3DRENDERSTATE_LIGHTING]) {
            for (int i = 0; i < 8; i++) {
                const D3DLIGHT7 *l = &d->light[i];
                if (!d->light_on[i] || l->dltType != D3DLIGHT_DIRECTIONAL) continue;   // the game's: one directional
                float len = sqrtf(l->dvDirection.x * l->dvDirection.x + l->dvDirection.y * l->dvDirection.y + l->dvDirection.z * l->dvDirection.z);
                if (len <= 0) continue;
                vc.ldir[lights][0] = l->dvDirection.x / len; vc.ldir[lights][1] = l->dvDirection.y / len;
                vc.ldir[lights][2] = l->dvDirection.z / len;
                rd3_value(vc.ldiff[lights], &l->dcvDiffuse);
                rd3_value(vc.lamb[lights], &l->dcvAmbient);
                lights++;
            }
            rd3_value(vc.mdiff, &d->material.diffuse);
            rd3_value(vc.mamb, &d->material.ambient);
            rd3_value(vc.memi, &d->material.emissive);
            rd3_color(vc.gamb, rs[D3DRENDERSTATE_AMBIENT]);
            int cv = rs[D3DRENDERSTATE_COLORVERTEX] != 0;   // the vertex has a diffuse colour (COLOR1), no specular (COLOR2)
            vc.src[0] = cv && rs[D3DRENDERSTATE_DIFFUSEMATERIALSOURCE] == D3DMCS_COLOR1;
            vc.src[1] = cv && rs[D3DRENDERSTATE_AMBIENTMATERIALSOURCE] == D3DMCS_COLOR1;
            vc.src[2] = cv && rs[D3DRENDERSTATE_EMISSIVEMATERIALSOURCE] == D3DMCS_COLOR1;
        }
        vc.lit[0] = rs[D3DRENDERSTATE_LIGHTING] != 0;
        vc.lit[1] = lights;
        vc.lit[2] = rs[D3DRENDERSTATE_NORMALIZENORMALS] != 0;
        if (rs[D3DRENDERSTATE_FOGENABLE]) {   // vertex fog (the game's); a table fog mode computed per vertex the same way
            DWORD m = rs[D3DRENDERSTATE_FOGTABLEMODE] != D3DFOG_NONE ? rs[D3DRENDERSTATE_FOGTABLEMODE] : rs[D3DRENDERSTATE_FOGVERTEXMODE];
            vc.fog[0] = (float)(m == D3DFOG_LINEAR ? 3 : m == D3DFOG_EXP2 ? 2 : m == D3DFOG_EXP ? 1 : 0);
            vc.fog[1] = rd3_float(rs[D3DRENDERSTATE_FOGSTART]);
            vc.fog[2] = rd3_float(rs[D3DRENDERSTATE_FOGEND]);
            vc.fog[3] = rd3_float(rs[D3DRENDERSTATE_FOGDENSITY]);
            if (vc.fog[0] == 3 && vc.fog[2] == vc.fog[1]) vc.fog[2] += 1e-6f;
        }
        // Direct3D 7's pixel centres are at whole coordinates, Direct3D 11's at .5: a vertex at x lands on pixel x's centre
        // shifted half a pixel right and down (in clip space: + x, - y; the picture compare found 1 pixel up-left without)
        vc.vp[0] = d->viewport.dwWidth ? 1.0f / (float)d->viewport.dwWidth : 0;
        vc.vp[1] = d->viewport.dwHeight ? -1.0f / (float)d->viewport.dwHeight : 0;
        vc.vp[2] = rs[D3DRENDERSTATE_RANGEFOGENABLE] ? 1.0f : 0.0f;
        if (full || memcmp(&vc, &g_rd3.vc, sizeof vc)) {
            rd3_flush();
            ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_rd3.vcb, 0, NULL, &vc, 0, 0);
            g_rd3.vc = vc;
        }
    }
    int fog = g_rd3.vc.fog[0] != 0;   // (0 while fog is off)

    // the pixel shader's
    rd3_pcb pc;
    memset(&pc, 0, sizeof pc);
    pc.op[0] = (int32_t)ts[D3DTSS_COLOROP]; pc.op[1] = (int32_t)ts[D3DTSS_COLORARG1]; pc.op[2] = (int32_t)ts[D3DTSS_COLORARG2];
    pc.op[3] = srv != NULL;
    pc.aop[0] = (int32_t)ts[D3DTSS_ALPHAOP]; pc.aop[1] = (int32_t)ts[D3DTSS_ALPHAARG1]; pc.aop[2] = (int32_t)ts[D3DTSS_ALPHAARG2];
    pc.aop[3] = rs[D3DRENDERSTATE_ALPHATESTENABLE] ? (int32_t)rs[D3DRENDERSTATE_ALPHAFUNC] : 0;
    rd3_color(pc.fogc, rs[D3DRENDERSTATE_FOGCOLOR]);
    rd3_color(pc.tf, rs[D3DRENDERSTATE_TEXTUREFACTOR]);
    pc.misc[0] = (float)(rs[D3DRENDERSTATE_ALPHAREF] & 255);
    pc.misc[1] = fog ? 1.0f : 0.0f;
    pc.misc[2] = ckey ? 1.0f : 0.0f;
    if (full || memcmp(&pc, &g_rd3.pc, sizeof pc)) {
        rd3_flush();
        ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_rd3.pcb, 0, NULL, &pc, 0, 0);
        g_rd3.pc = pc;
    }
    d->changed = 0;
    g_rd3.prep_dev = d; g_rd3.prep_clamp = clamp;
    return 1;
}

// n bytes into an upload buffer at a multiple of align; the offset, or -1
internal long rd3_upload(ID3D11Buffer *b, UINT cap, UINT *pos, const void *src, UINT n, UINT align) {
    if (n > cap) return -1;
    UINT at = (*pos + align - 1) / align * align;
    D3D11_MAP how = D3D11_MAP_WRITE_NO_OVERWRITE;
    if (at + n > cap) { at = 0; how = D3D11_MAP_WRITE_DISCARD; }
    D3D11_MAPPED_SUBRESOURCE ms;
    if (FAILED(ID3D11DeviceContext_Map(g_d3d.ctx, (ID3D11Resource *)b, 0, how, 0, &ms))) return -1;
    memcpy((uint8_t *)ms.pData + at, src, n);
    ID3D11DeviceContext_Unmap(g_d3d.ctx, (ID3D11Resource *)b, 0);
    *pos = at + n;
    return (long)at;
}
internal D3D11_PRIMITIVE_TOPOLOGY rd3_topology(D3DPRIMITIVETYPE t) {
    return t == D3DPT_TRIANGLELIST ? D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST : t == D3DPT_LINELIST ? D3D11_PRIMITIVE_TOPOLOGY_LINELIST
         : t == D3DPT_TRIANGLESTRIP ? D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP : t == D3DPT_LINESTRIP ? D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP
         : t == D3DPT_POINTLIST ? D3D11_PRIMITIVE_TOPOLOGY_POINTLIST : D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
}
// what the pipeline can't draw, told once: a vertex format other than the game's 0x152, a triangle fan
internal int rd3_drawable(rd_vb *vb, D3DPRIMITIVETYPE type) {
    static volatile LONG told_fvf, told_type;
    if (vb->fvf != 0x152) {
        if (!InterlockedExchange(&told_fvf, 1)) log_locked("# render: vertex format %08lx not implemented\n", (unsigned long)vb->fvf);
        return 0;
    }
    if (rd3_topology(type) == D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED) {
        if (!InterlockedExchange(&told_type, 1)) log_locked("# render: primitive type %d not implemented\n", (int)type);
        return 0;
    }
    return 1;
}

// the batch into the upload buffers and drawn with the states bound
internal void rd3_flush(void) {
    if (!g_rb.ni) return;
    ID3D11DeviceContext *x = g_d3d.ctx;
    long wa = rd3_upload(g_rd3.wb, RD3_WB_BYTES, &g_rd3.wb_pos, g_rb.w, (UINT)g_rb.nw * sizeof *g_rb.w, sizeof *g_rb.w);
    if (wa > 0) for (int k = 0; k < g_rb.nv; k++) g_rb.v[k].w += (uint32_t)wa / sizeof *g_rb.w;   // the batch's matrices' place
    long at = wa >= 0 ? rd3_upload(g_rd3.vb, RD3_VB_BYTES, &g_rd3.vb_pos, g_rb.v, (UINT)g_rb.nv * sizeof *g_rb.v, sizeof *g_rb.v) : -1;
    long ia = at >= 0 ? rd3_upload(g_rd3.ib, RD3_IB_BYTES, &g_rd3.ib_pos, g_rb.i, (UINT)g_rb.ni * 4, 4) : -1;
    if (ia >= 0) {
        ID3D11DeviceContext_IASetPrimitiveTopology(x, g_rb.topo);
        ID3D11DeviceContext_DrawIndexed(x, (UINT)g_rb.ni, (UINT)ia / 4, (INT)((UINT)at / sizeof *g_rb.v));
        g_rb.batches++;
    }
    g_rb.nv = g_rb.ni = g_rb.nw = 0;
}
// a draw into the batch (rd3_prepare bound its states): nv vertices from start, ni indices (NULL: 0..nv-1) of type
internal void rd3_append(rd_device *d, D3DPRIMITIVETYPE type, rd_vb *vb, DWORD start, DWORD nv, const WORD *idx, DWORD ni) {
    int line = type == D3DPT_LINELIST || type == D3DPT_LINESTRIP, strip = type == D3DPT_TRIANGLESTRIP || type == D3DPT_LINESTRIP;
    D3D11_PRIMITIVE_TOPOLOGY topo = line ? D3D11_PRIMITIVE_TOPOLOGY_LINELIST : type == D3DPT_POINTLIST
                                  ? D3D11_PRIMITIVE_TOPOLOGY_POINTLIST : D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    DWORD n = idx ? ni : nv;   // the primitives' vertices as given
    DWORD out = !strip ? n : line ? (n >= 2 ? (n - 1) * 2 : 0) : (n >= 3 ? (n - 2) * 3 : 0);
    if (!out || nv > RB_VERTS || out > RB_INDICES) return;
    if (g_rb.ni && (g_rb.topo != topo || g_rb.nv + (int)nv > RB_VERTS || g_rb.ni + (int)out > RB_INDICES || g_rb.nw >= RB_WORLDS))
        rd3_flush();
    if (!g_rb.nw || memcmp(g_rb.w[g_rb.nw - 1], &d->world._11, 64)) memcpy(g_rb.w[g_rb.nw++], &d->world._11, 64);
    g_rb.topo = topo;
    uint32_t w = (uint32_t)g_rb.nw - 1, base = (uint32_t)g_rb.nv;
    for (DWORD k = 0; k < nv; k++) {
        rb_vertex *v = &g_rb.v[g_rb.nv++];
        memcpy(v->fvf, vb->mem + (size_t)(start + k) * vb->stride, sizeof v->fvf);
        v->w = w;
    }
    uint32_t *o = g_rb.i + g_rb.ni;
    #define RB_AT(k) (base + (idx ? idx[k] : (k)))
    if (!strip) for (DWORD k = 0; k < n; k++) *o++ = RB_AT(k);
    else if (line) for (DWORD k = 0; k + 1 < n; k++) { *o++ = RB_AT(k); *o++ = RB_AT(k + 1); }
    else for (DWORD k = 0; k + 2 < n; k++) {   // every second triangle of a strip turns the other way
        *o++ = RB_AT(k + (k & 1)); *o++ = RB_AT(k + 1 - (k & 1)); *o++ = RB_AT(k + 2);
    }
    #undef RB_AT
    g_rb.ni += (int)out;
}

#include "render_overlay.c"   // the debug overlay: the AI's plans on the map

internal HRESULT __stdcall rd_dev_qi(rd_device *d, const GUID *iid, void **out) { (void)d; (void)iid; *out = NULL; return E_NOINTERFACE; }
internal ULONG __stdcall rd_dev_addref(rd_device *d) { return (ULONG)InterlockedIncrement(&d->refs); }
internal ULONG __stdcall rd_dev_release(rd_device *d) {
    LONG n = InterlockedDecrement(&d->refs);
    if (n > 0) return (ULONG)n;
    if (d->target) rd_sf_release(d->target);
    free(d);
    return 0;
}

// what the device can do: the T&L HAL of a DirectX 7 card that has everything the game asks for
internal void rd_device_desc(D3DDEVICEDESC7 *c, int tnl) {
    memset(c, 0, sizeof *c);
    c->dwDevCaps = D3DDEVCAPS_FLOATTLVERTEX | D3DDEVCAPS_EXECUTESYSTEMMEMORY | D3DDEVCAPS_TLVERTEXSYSTEMMEMORY |
        D3DDEVCAPS_TEXTUREVIDEOMEMORY | D3DDEVCAPS_DRAWPRIMTLVERTEX | D3DDEVCAPS_CANRENDERAFTERFLIP |
        D3DDEVCAPS_DRAWPRIMITIVES2 | D3DDEVCAPS_DRAWPRIMITIVES2EX | D3DDEVCAPS_HWRASTERIZATION |
        D3DDEVCAPS_TEXTURENONLOCALVIDMEM | D3DDEVCAPS_CANBLTSYSTONONLOCAL | (tnl ? D3DDEVCAPS_HWTRANSFORMANDLIGHT : 0);
    D3DPRIMCAPS p = { 0 };
    p.dwSize = sizeof p;
    p.dwMiscCaps = D3DPMISCCAPS_MASKZ | D3DPMISCCAPS_CULLNONE | D3DPMISCCAPS_CULLCW | D3DPMISCCAPS_CULLCCW;
    p.dwRasterCaps = D3DPRASTERCAPS_DITHER | D3DPRASTERCAPS_ZTEST | D3DPRASTERCAPS_FOGVERTEX | D3DPRASTERCAPS_FOGRANGE |
        D3DPRASTERCAPS_MIPMAPLODBIAS | D3DPRASTERCAPS_ZBIAS | D3DPRASTERCAPS_ANISOTROPY | D3DPRASTERCAPS_WFOG |
        D3DPRASTERCAPS_ZFOG | D3DPRASTERCAPS_FOGTABLE;
    p.dwZCmpCaps = p.dwAlphaCmpCaps = 0xFF;   // every comparison
    p.dwSrcBlendCaps = p.dwDestBlendCaps = 0x1FFF;
    p.dwShadeCaps = D3DPSHADECAPS_COLORGOURAUDRGB | D3DPSHADECAPS_SPECULARGOURAUDRGB | D3DPSHADECAPS_ALPHAGOURAUDBLEND |
        D3DPSHADECAPS_FOGGOURAUD;
    p.dwTextureCaps = D3DPTEXTURECAPS_PERSPECTIVE | D3DPTEXTURECAPS_POW2 | D3DPTEXTURECAPS_ALPHA |
        D3DPTEXTURECAPS_TRANSPARENCY | D3DPTEXTURECAPS_TEXREPEATNOTSCALEDBYSIZE;
    p.dwTextureFilterCaps = D3DPTFILTERCAPS_NEAREST | D3DPTFILTERCAPS_LINEAR | D3DPTFILTERCAPS_MIPNEAREST |
        D3DPTFILTERCAPS_MIPLINEAR | D3DPTFILTERCAPS_LINEARMIPNEAREST | D3DPTFILTERCAPS_LINEARMIPLINEAR |
        D3DPTFILTERCAPS_MINFPOINT | D3DPTFILTERCAPS_MINFLINEAR | D3DPTFILTERCAPS_MIPFPOINT | D3DPTFILTERCAPS_MIPFLINEAR |
        D3DPTFILTERCAPS_MAGFPOINT | D3DPTFILTERCAPS_MAGFLINEAR;
    p.dwTextureBlendCaps = D3DPTBLENDCAPS_MODULATE | D3DPTBLENDCAPS_MODULATEALPHA | D3DPTBLENDCAPS_DECAL |
        D3DPTBLENDCAPS_DECALALPHA | D3DPTBLENDCAPS_COPY | D3DPTBLENDCAPS_ADD;
    p.dwTextureAddressCaps = D3DPTADDRESSCAPS_WRAP | D3DPTADDRESSCAPS_CLAMP | D3DPTADDRESSCAPS_MIRROR |
        D3DPTADDRESSCAPS_INDEPENDENTUV;
    c->dpcLineCaps = c->dpcTriCaps = p;
    c->dwDeviceRenderBitDepth = DDBD_16 | DDBD_32;
    c->dwDeviceZBufferBitDepth = DDBD_16 | DDBD_24 | DDBD_32;
    c->dwMinTextureWidth = c->dwMinTextureHeight = 1;
    c->dwMaxTextureWidth = c->dwMaxTextureHeight = 4096;
    c->dwMaxTextureRepeat = 8192;
    c->dwMaxTextureAspectRatio = 4096;
    c->dwMaxAnisotropy = 16;
    c->dvGuardBandLeft = c->dvGuardBandTop = -8192;
    c->dvGuardBandRight = c->dvGuardBandBottom = 8192;
    c->dwFVFCaps = 8;   // texture coordinate sets
    c->dwTextureOpCaps = 0x00FFFFFF;
    c->wMaxTextureBlendStages = c->wMaxSimultaneousTextures = 8;
    c->dwMaxActiveLights = 8;
    c->dvMaxVertexW = 1e10f;
    c->deviceGUID = tnl ? IID_IDirect3DTnLHalDevice : IID_IDirect3DHALDevice;
    c->wMaxUserClipPlanes = 6;
    c->wMaxVertexBlendMatrices = 4;
    c->dwVertexProcessingCaps = D3DVTXPCAPS_TEXGEN | D3DVTXPCAPS_MATERIALSOURCE7 | D3DVTXPCAPS_VERTEXFOG |
        D3DVTXPCAPS_DIRECTIONALLIGHTS | D3DVTXPCAPS_POSITIONALLIGHTS | D3DVTXPCAPS_LOCALVIEWER;
}
internal HRESULT __stdcall rd_dev_get_caps(rd_device *d, D3DDEVICEDESC7 *c) { (void)d; rd_device_desc(c, 1); return D3D_OK; }
internal HRESULT __stdcall rd_dev_enum_tex(rd_device *d, HRESULT (CALLBACK *cb)(DDPIXELFORMAT *, void *), void *ctx) {
    (void)d;
    static const int f[] = { RF_ARGB8888, RF_XRGB8888, RF_RGB565, RF_XRGB1555, RF_ARGB1555, RF_ARGB4444 };
    for (int i = 0; i < 6; i++) {
        DDPIXELFORMAT pf;
        rf_pixel_format(f[i], &pf);
        if (cb(&pf, ctx) == D3DENUMRET_CANCEL) break;
    }
    return D3D_OK;
}
// THE WORLD'S LOCK (Low-Level Engine's TSReadWriteLock, wrapped through the exe's import slots): the picture reads the
// game's objects under a read lock, the simulation thread writes them under the write lock for its steps (and the AI's
// work in them) - while it holds it, the frame waits. Measured for the minute's line: the render thread's (the one
// ending the scenes) time in ReadLock, and how long the lock it waited on was held for writing.
typedef void (__thiscall *rd_rwl_fn)(void *lock);
internal rd_rwl_fn g_rwl_read, g_rwl_write, g_rwl_wunlock;
internal DWORD g_rd_thread;
internal void *g_rwl_world;   // the lock the render thread waited on
internal LONGLONG g_rwl_wait_qpc, g_rwl_held_qpc, g_rwl_write_at;
// The minute's averages hide what a player sees: a frame that waits 30-80 ms a few times a second while the average
// stays at 120 fps (camtrace in the player's game, 2026-10-28) - the camera stands, then jumps. So also the long ones:
// the frames whose wait was RWL_LONG_MS or more, and the write holds that long by where the game took the lock (the
// return address of its call through the import slot; the writers are serialized by the lock itself).
#define RWL_LONG_MS 8
enum { RWL_SITES = 16 };
internal struct { uintptr_t ret; LONG n; LONGLONG sum, max; } g_rwl_long[RWL_SITES];
internal uintptr_t g_rwl_write_ret;
internal LONG g_rwl_waits_long;
internal LONGLONG g_rwl_wait_max;
internal void __thiscall rd_hook_read_lock(void *lock) {
    if (GetCurrentThreadId() != g_rd_thread) { g_rwl_read(lock); return; }
    LONGLONG t0 = rd_qpc();
    g_rwl_read(lock);
    LONGLONG w = rd_qpc() - t0;
    g_rwl_wait_qpc += w;
    if (w >= rd_qpc_ms(RWL_LONG_MS)) g_rwl_waits_long++;
    if (w > g_rwl_wait_max) g_rwl_wait_max = w;
    if (w > 0 && !g_rwl_world) g_rwl_world = lock;
}
internal void __thiscall rd_hook_write_lock(void *lock) {
    g_rwl_write(lock);
    if (lock == g_rwl_world) {
        g_rwl_write_at = rd_qpc();
        g_rwl_write_ret = (uintptr_t)__builtin_return_address(0);
        g_sim_thread = GetCurrentThreadId();
    }
}
internal void __thiscall rd_hook_write_unlock(void *lock) {
    if (lock == g_rwl_world && g_rwl_write_at) {
        LONGLONG h = rd_qpc() - g_rwl_write_at;
        g_rwl_held_qpc += h;
        g_rwl_write_at = 0;
        if (h >= rd_qpc_ms(RWL_LONG_MS)) {
            int i = 0;
            while (i < RWL_SITES && g_rwl_long[i].ret && g_rwl_long[i].ret != g_rwl_write_ret) i++;
            if (i < RWL_SITES) {
                g_rwl_long[i].ret = g_rwl_write_ret;
                g_rwl_long[i].n++;
                g_rwl_long[i].sum += h;
                if (h > g_rwl_long[i].max) g_rwl_long[i].max = h;
            }
        }
    }
    g_rwl_wunlock(lock);
}
internal int rd_world_lock_install(void) {
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll"), exe = GetModuleHandleA(NULL);
    if (!lle) return 0;
    g_rwl_read = (rd_rwl_fn)(void *)GetProcAddress(lle, "?ReadLock@TSReadWriteLock@@QAEXXZ");
    g_rwl_write = (rd_rwl_fn)(void *)GetProcAddress(lle, "?WriteLock@TSReadWriteLock@@QAEXXZ");
    g_rwl_wunlock = (rd_rwl_fn)(void *)GetProcAddress(lle, "?WriteUnlock@TSReadWriteLock@@QAEXXZ");
    if (!g_rwl_read || !g_rwl_write || !g_rwl_wunlock) return 0;
    return patch_iat(exe, "Low-Level Engine.dll", "?ReadLock@TSReadWriteLock@@QAEXXZ", (void *)rd_hook_read_lock) +
           patch_iat(exe, "Low-Level Engine.dll", "?WriteLock@TSReadWriteLock@@QAEXXZ", (void *)rd_hook_write_lock) +
           patch_iat(exe, "Low-Level Engine.dll", "?WriteUnlock@TSReadWriteLock@@QAEXXZ", (void *)rd_hook_write_unlock);
}

// THE GAME'S DRAWING, BY ITS PARTS: RTSGraphics draws a frame through its render objects (spec engine_display.cs) -
// each one's draw timed for the minute's line, the game's own work in it and the renderer's under it together
enum { RO_TERRAIN, RO_SHADOWS, RO_UNITS, RO_WATER, RO_UI, RO_N };
internal LONGLONG g_ro_qpc[RO_N];
internal void __thiscall rd_hook_ro_terrain(ee_ro_terrain *o) { LONGLONG t = rd_qpc(); ro_terrain_draw(o); g_ro_qpc[RO_TERRAIN] += rd_qpc() - t; }
internal void __thiscall rd_hook_ro_shadows(ee_ro_shadows *o) { LONGLONG t = rd_qpc(); ro_shadows_draw(o); g_ro_qpc[RO_SHADOWS] += rd_qpc() - t; }
internal void __thiscall rd_hook_ro_units(ee_ro_units *o) { LONGLONG t = rd_qpc(); ro_units_draw(o); g_ro_qpc[RO_UNITS] += rd_qpc() - t; }
internal void __thiscall rd_hook_ro_water(ee_ro_water *o) { LONGLONG t = rd_qpc(); ro_water_draw(o); g_ro_qpc[RO_WATER] += rd_qpc() - t; }
internal void __thiscall rd_hook_ro_ui(ee_ro_ui *o) { LONGLONG t = rd_qpc(); ro_ui_draw(o); g_ro_qpc[RO_UI] += rd_qpc() - t; }
internal int rd_render_objects_install(void) {
    return patch_ro_terrain_draw(rd_hook_ro_terrain) + patch_ro_shadows_draw(rd_hook_ro_shadows) +
           patch_ro_units_draw(rd_hook_ro_units) + patch_ro_water_draw(rd_hook_ro_water) + patch_ro_ui_draw(rd_hook_ro_ui);
}

internal void rd_frame_tick(void) {
    g_rd_thread = GetCurrentThreadId();
    InterlockedIncrement(&g_rd_frames);
    DWORD now = GetTickCount();
    if (now - g_rd_minute_tick < 60000) return;
    static LONG frames0, draws0, batches0;
    static LONGLONG present0, wait0, held0;
    static DWORD tick0;
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    LONG fr = g_rd_frames - frames0, dr = g_rd_draws - draws0, ba = g_rb.batches - batches0;
    double k = fr > 0 ? 1000.0 / (double)f.QuadPart / fr : 0, secs = tick0 ? (double)(now - tick0) / 1000.0 : 0;
    log_locked("# render: frames %ld, draws %ld (the last minute a frame: %ld draws in %ld batches, %.2f ms in them, %.2f ms in Present,"
               " %.2f ms waiting for the simulation's lock; it held it %.0f ms a second; %ld frames)\n", (long)g_rd_frames,
               (long)g_rd_draws, fr > 0 ? (long)(dr / fr) : 0L, fr > 0 ? (long)(ba / fr) : 0L,
               (double)(g_rd_draw_qpc - g_rd_draw_qpc_told) * k, (double)(g_d3d_present_qpc - present0) * k,
               (double)(g_rwl_wait_qpc - wait0) * k,
               secs > 0 ? (double)(g_rwl_held_qpc - held0) * 1000.0 / (double)f.QuadPart / secs : 0, (long)fr);
    static LONGLONG ro0[RO_N];
    double ro[RO_N];
    for (int i = 0; i < RO_N; i++) { ro[i] = (double)(g_ro_qpc[i] - ro0[i]) * k; ro0[i] = g_ro_qpc[i]; }
    log_locked("# render: the game's drawing a frame (ms, the renderer's draws inside): terrain %.2f, shadows %.2f, units %.2f,"
               " water %.2f, interface %.2f\n", ro[RO_TERRAIN], ro[RO_SHADOWS], ro[RO_UNITS], ro[RO_WATER], ro[RO_UI]);
    // the long waits and holds of the minute, the sites by their time held long
    double ms = 1000.0 / (double)f.QuadPart;
    char sites[512];
    int len = 0;
    for (int shown = 0; shown < 5; shown++) {
        int best = -1;
        for (int i = 0; i < RWL_SITES; i++)
            if (g_rwl_long[i].n && (best < 0 || g_rwl_long[i].sum > g_rwl_long[best].sum)) best = i;
        if (best < 0) break;
        len += snprintf(sites + len, sizeof sites - (size_t)len, "%s%08x %ldx %.0f ms (longest %.0f)", shown ? ", " : "",
                        (unsigned)g_rwl_long[best].ret, (long)g_rwl_long[best].n, (double)g_rwl_long[best].sum * ms,
                        (double)g_rwl_long[best].max * ms);
        g_rwl_long[best].n = 0;
    }
    log_locked("# render: frames waiting %d ms or more for the lock %ld (longest %.0f ms); its holds that long by where the"
               " game took it: %s\n", RWL_LONG_MS, (long)g_rwl_waits_long, (double)g_rwl_wait_max * ms, len ? sites : "none");
    memset(g_rwl_long, 0, sizeof g_rwl_long);
    g_rwl_waits_long = 0;
    g_rwl_wait_max = 0;
    g_rd_minute_tick = tick0 = now;
    frames0 = g_rd_frames; draws0 = g_rd_draws; batches0 = g_rb.batches;
    g_rd_draw_qpc_told = g_rd_draw_qpc; present0 = g_d3d_present_qpc; wait0 = g_rwl_wait_qpc; held0 = g_rwl_held_qpc;
}
internal HRESULT __stdcall rd_dev_begin(rd_device *d) { d->in_scene = 1; return D3D_OK; }
internal HRESULT __stdcall rd_dev_end(rd_device *d) {
    d->in_scene = 0;
    RD_LOCK();
    rd3_flush();
    rd3_overlay_frame_end(d);
    RD_UNLOCK();
    rd_frame_tick();
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_set_target(rd_device *d, rd_surface *t, DWORD flags) {
    (void)flags;
    if (!rd_is_surface(t) || t->kind != RK_TARGET) return DDERR_INVALIDPARAMS;
    rd_sf_addref(t);
    if (d->target) rd_sf_release(d->target);
    if (d->target != t) d->changed |= RD_CH_TARGET;
    d->target = t;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_target(rd_device *d, rd_surface **t) {
    *t = d->target;
    if (d->target) rd_sf_addref(d->target);
    return D3D_OK;
}
// Clear: the target's colour and its depth buffer - whole surfaces when the rects cover them (the game's: its viewport)
internal HRESULT __stdcall rd_dev_clear(rd_device *d, DWORD n, D3DRECT *rects, DWORD flags, D3DCOLOR col, D3DVALUE z, DWORD st) {
    (void)st;
    rd_surface *t = d->target;
    if (!t) return D3D_OK;
    RD_LOCK();
    rd3_flush();
    for (DWORD i = 0; i < (n ? n : 1); i++) {
        RECT r = { 0, 0, (LONG)t->desc.dwWidth, (LONG)t->desc.dwHeight };
        if (n && rects) { r.left = rects[i].x1; r.top = rects[i].y1; r.right = rects[i].x2; r.bottom = rects[i].y2; }
        int whole = r.left <= 0 && r.top <= 0 && r.right >= (LONG)t->desc.dwWidth && r.bottom >= (LONG)t->desc.dwHeight;
        if ((flags & D3DCLEAR_TARGET) && t->rtv) {
            float c[4] = { (float)(col >> 16 & 255) / 255.0f, (float)(col >> 8 & 255) / 255.0f, (float)(col & 255) / 255.0f, 1 };
            if (whole) ID3D11DeviceContext_ClearRenderTargetView(g_d3d.ctx, t->rtv, c);
            else rd2_rect(t, &r, NULL, 0, 0, NULL, c, 0);
        }
        if ((flags & D3DCLEAR_ZBUFFER) && t->zbuf && t->zbuf->dsv) {
            if (whole) ID3D11DeviceContext_ClearDepthStencilView(g_d3d.ctx, t->zbuf->dsv, D3D11_CLEAR_DEPTH, z, 0);
            else rd2_depth_rect(t, &r, z);
        }
    }
    RD_UNLOCK();
    return D3D_OK;
}
internal D3DMATRIX *rd_transform(rd_device *d, D3DTRANSFORMSTATETYPE t) {
    return t == D3DTRANSFORMSTATE_WORLD ? &d->world : t == D3DTRANSFORMSTATE_VIEW ? &d->view
         : t == D3DTRANSFORMSTATE_PROJECTION ? &d->proj : NULL;
}
internal HRESULT __stdcall rd_dev_set_transform(rd_device *d, D3DTRANSFORMSTATETYPE t, D3DMATRIX *m) {
    D3DMATRIX *x = rd_transform(d, t);
    if (!x) return D3D_OK;
    if (x != &d->world && memcmp(x, m, sizeof *m)) d->changed |= RD_CH_VC;   // the world is the vertices' own (the batch)
    *x = *m;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_transform(rd_device *d, D3DTRANSFORMSTATETYPE t, D3DMATRIX *m) {
    D3DMATRIX *x = rd_transform(d, t);
    if (x) *m = *x;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_set_viewport(rd_device *d, D3DVIEWPORT7 *v) {
    if (memcmp(&d->viewport, v, sizeof *v) != 0) d->changed |= RD_CH_VC;
    d->viewport = *v;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_viewport(rd_device *d, D3DVIEWPORT7 *v) { *v = d->viewport; return D3D_OK; }
internal HRESULT __stdcall rd_dev_set_material(rd_device *d, D3DMATERIAL7 *m) {
    if (memcmp(&d->material, m, sizeof *m) != 0) d->changed |= RD_CH_VC;
    d->material = *m;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_material(rd_device *d, D3DMATERIAL7 *m) { *m = d->material; return D3D_OK; }
internal HRESULT __stdcall rd_dev_set_light(rd_device *d, DWORD i, D3DLIGHT7 *l) {
    if (i >= 8) return DDERR_INVALIDPARAMS;
    if (memcmp(&d->light[i], l, sizeof *l) != 0) d->changed |= RD_CH_VC;
    d->light[i] = *l;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_light(rd_device *d, DWORD i, D3DLIGHT7 *l) {
    if (i >= 8) return DDERR_INVALIDPARAMS;
    *l = d->light[i];
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_light_enable(rd_device *d, DWORD i, BOOL on) {
    if (i >= 8) return DDERR_INVALIDPARAMS;
    if (d->light_on[i] != (on != 0)) d->changed |= RD_CH_VC;
    d->light_on[i] = on != 0;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_light_enable(rd_device *d, DWORD i, BOOL *on) {
    if (i >= 8) return DDERR_INVALIDPARAMS;
    *on = d->light_on[i];
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_set_rs(rd_device *d, D3DRENDERSTATETYPE s, DWORD v) {
    if ((unsigned)s < RD_RS && d->rs[s] != v) { d->rs[s] = v; d->changed |= RD_CH_RS; }
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_rs(rd_device *d, D3DRENDERSTATETYPE s, DWORD *v) {
    *v = (unsigned)s < RD_RS ? d->rs[s] : 0;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_set_texture(rd_device *d, DWORD stage, rd_surface *t) {
    rd_surface *s = rd_is_surface(t) ? t : NULL;
    if (stage < 2 && d->tex[stage] != s) { d->tex[stage] = s; d->changed |= RD_CH_TEX; }
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_texture(rd_device *d, DWORD stage, rd_surface **t) {
    *t = stage < 2 ? d->tex[stage] : NULL;
    if (*t) rd_sf_addref(*t);
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_set_tss(rd_device *d, DWORD stage, D3DTEXTURESTAGESTATETYPE t, DWORD v) {
    if (stage >= 2 || (unsigned)t >= RD_TSS) return D3D_OK;
    DWORD *ts = d->tss[stage];
    if (t == D3DTSS_ADDRESS) {   // the game's: both at once
        if (ts[D3DTSS_ADDRESSU] != v || ts[D3DTSS_ADDRESSV] != v) d->changed |= RD_CH_TEX;
        ts[D3DTSS_ADDRESSU] = ts[D3DTSS_ADDRESSV] = v;
    }
    if (ts[t] != v) d->changed |= RD_CH_TEX;
    ts[t] = v;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_tss(rd_device *d, DWORD stage, D3DTEXTURESTAGESTATETYPE t, DWORD *v) {
    *v = stage < 2 && (unsigned)t < RD_TSS ? d->tss[stage][t] : 0;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_validate(rd_device *d, DWORD *passes) { (void)d; *passes = 1; return D3D_OK; }
internal HRESULT __stdcall rd_dev_draw_vb(rd_device *d, D3DPRIMITIVETYPE type, rd_vb *vb, DWORD start, DWORD n, DWORD flags) {
    (void)flags;
    InterlockedIncrement(&g_rd_draws);
    if (!n || start + n > vb->n || !rd3_drawable(vb, type)) return D3D_OK;
    LONGLONG t0 = rd_qpc();
    RD_LOCK();
    rd3_overlay_before(d);
    if (rd3_prepare(d, rd3_flat_picture(d, vb, start, n))) rd3_append(d, type, vb, start, n, NULL, 0);
    g_rd_draw_qpc += rd_qpc() - t0;
    RD_UNLOCK();
    return D3D_OK;
}
// the indices count from start (Direct3D 7 has no base vertex of its own)
internal HRESULT __stdcall rd_dev_draw_indexed_vb(rd_device *d, D3DPRIMITIVETYPE type, rd_vb *vb, DWORD start, DWORD nv,
                                                  WORD *idx, DWORD ni, DWORD flags) {
    (void)flags;
    InterlockedIncrement(&g_rd_draws);
    if (!nv || !ni || !idx || start + nv > vb->n || !rd3_drawable(vb, type)) return D3D_OK;
    LONGLONG t0 = rd_qpc();
    RD_LOCK();
    rd3_overlay_before(d);
    if (rd3_prepare(d, rd3_flat_picture(d, vb, start, nv))) rd3_append(d, type, vb, start, nv, idx, ni);
    g_rd_draw_qpc += rd_qpc() - t0;
    RD_UNLOCK();
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_direct3d(rd_device *d, void **out);

// the device's defaults (Direct3D 7's)
internal void rd_device_defaults(rd_device *d) {
    static const D3DMATRIX id = { ._11 = 1, ._22 = 1, ._33 = 1, ._44 = 1 };
    d->world = d->view = d->proj = id;
    d->changed = RD_CH_VC | RD_CH_RS | RD_CH_TEX | RD_CH_TARGET;
    d->rs[D3DRENDERSTATE_ZENABLE] = 1; d->rs[D3DRENDERSTATE_ZWRITEENABLE] = 1; d->rs[D3DRENDERSTATE_ZFUNC] = D3DCMP_LESSEQUAL;
    d->rs[D3DRENDERSTATE_SRCBLEND] = D3DBLEND_ONE; d->rs[D3DRENDERSTATE_DESTBLEND] = D3DBLEND_ZERO;
    d->rs[D3DRENDERSTATE_CULLMODE] = D3DCULL_CCW; d->rs[D3DRENDERSTATE_ALPHAFUNC] = D3DCMP_ALWAYS;
    d->rs[D3DRENDERSTATE_SHADEMODE] = D3DSHADE_GOURAUD; d->rs[D3DRENDERSTATE_FILLMODE] = D3DFILL_SOLID;
    d->rs[D3DRENDERSTATE_LIGHTING] = 1; d->rs[D3DRENDERSTATE_COLORVERTEX] = 1; d->rs[D3DRENDERSTATE_CLIPPING] = 1;
    d->rs[D3DRENDERSTATE_DIFFUSEMATERIALSOURCE] = D3DMCS_COLOR1; d->rs[D3DRENDERSTATE_SPECULARMATERIALSOURCE] = D3DMCS_COLOR2;
    d->rs[D3DRENDERSTATE_FOGVERTEXMODE] = D3DFOG_NONE; d->rs[D3DRENDERSTATE_FOGEND] = 0x3F800000;   // 1.0f
    d->rs[D3DRENDERSTATE_FOGDENSITY] = 0x3F800000;
    for (int s = 0; s < 2; s++) {
        d->tss[s][D3DTSS_COLOROP] = s ? D3DTOP_DISABLE : D3DTOP_MODULATE;
        d->tss[s][D3DTSS_ALPHAOP] = s ? D3DTOP_DISABLE : D3DTOP_SELECTARG1;
        d->tss[s][D3DTSS_COLORARG1] = d->tss[s][D3DTSS_ALPHAARG1] = D3DTA_TEXTURE;
        d->tss[s][D3DTSS_COLORARG2] = d->tss[s][D3DTSS_ALPHAARG2] = D3DTA_CURRENT;
        d->tss[s][D3DTSS_ADDRESSU] = d->tss[s][D3DTSS_ADDRESSV] = D3DTADDRESS_WRAP;
        d->tss[s][D3DTSS_MAGFILTER] = d->tss[s][D3DTSS_MINFILTER] = D3DTFG_POINT;
        d->tss[s][D3DTSS_MIPFILTER] = D3DTFP_NONE;
        d->tss[s][D3DTSS_TEXCOORDINDEX] = (DWORD)s;
    }
}

// ---- Direct3D7 ----
typedef struct { void **vt; LONG refs; } rd_d3d;
internal void *rd_d3d_new(void) {
    rd_d3d *o = calloc(1, sizeof *o);
    if (!o) return NULL;
    o->vt = g_rd_vt[RDI_D3D];
    o->refs = 1;
    InterlockedIncrement(&g_rd_dd.refs);   // it holds its DirectDraw
    return o;
}
internal HRESULT __stdcall rd_dev_get_direct3d(rd_device *d, void **out) { (void)d; *out = rd_d3d_new(); return *out ? D3D_OK : E_OUTOFMEMORY; }
internal HRESULT __stdcall rd_d3d_qi(rd_d3d *o, const GUID *iid, void **out) {
    if (IsEqualGUID(iid, &IID_IDirect3D7) || IsEqualGUID(iid, &IID_IUnknown)) { InterlockedIncrement(&o->refs); *out = o; return S_OK; }
    if (IsEqualGUID(iid, &IID_IDirectDraw7)) { InterlockedIncrement(&g_rd_dd.refs); *out = &g_rd_dd; return S_OK; }
    *out = NULL;
    return E_NOINTERFACE;
}
internal ULONG __stdcall rd_d3d_addref(rd_d3d *o) { return (ULONG)InterlockedIncrement(&o->refs); }
internal ULONG __stdcall rd_d3d_release(rd_d3d *o) {
    LONG n = InterlockedDecrement(&o->refs);
    if (n > 0) return (ULONG)n;
    InterlockedDecrement(&g_rd_dd.refs);
    free(o);
    return 0;
}
internal HRESULT __stdcall rd_d3d_enum_devices(rd_d3d *o, HRESULT (CALLBACK *cb)(LPSTR, LPSTR, D3DDEVICEDESC7 *, void *), void *ctx) {
    (void)o;
    D3DDEVICEDESC7 c;
    rd_device_desc(&c, 0);
    if (cb("Direct3D HAL", "Direct3D 11 (Empire Earth: Age of AI)", &c, ctx) == D3DENUMRET_CANCEL) return D3D_OK;
    rd_device_desc(&c, 1);
    cb("Direct3D T&L HAL", "Direct3D 11 T&L (Empire Earth: Age of AI)", &c, ctx);
    return D3D_OK;
}
internal HRESULT __stdcall rd_d3d_create_device(rd_d3d *o, const GUID *cls, rd_surface *target, rd_device **out) {
    (void)o; (void)cls;
    *out = NULL;
    if (!rd_is_surface(target) || target->kind != RK_TARGET) return DDERR_INVALIDPARAMS;
    rd_device *d = calloc(1, sizeof *d);
    if (!d) return DDERR_OUTOFMEMORY;
    d->vt = g_rd_vt[RDI_DEV];
    d->refs = 1;
    rd_sf_addref(target);
    d->target = target;
    d->viewport.dwWidth = target->desc.dwWidth;
    d->viewport.dwHeight = target->desc.dwHeight;
    d->viewport.dvMaxZ = 1;
    rd_device_defaults(d);
    *out = d;
    return D3D_OK;
}
internal HRESULT __stdcall rd_d3d_create_vb(rd_d3d *o, D3DVERTEXBUFFERDESC *desc, rd_vb **out, DWORD flags) {
    (void)o; (void)flags;
    *out = NULL;
    rd_vb *b = calloc(1, sizeof *b);
    if (!b) return DDERR_OUTOFMEMORY;
    b->vt = g_rd_vt[RDI_VB];
    b->refs = 1;
    b->caps = desc->dwCaps; b->fvf = desc->dwFVF; b->n = desc->dwNumVertices;
    b->stride = rd_fvf_stride(desc->dwFVF);
    b->mem = calloc(b->n ? b->n : 1, b->stride ? b->stride : 4);
    if (!b->mem) { free(b); return DDERR_OUTOFMEMORY; }
    *out = b;
    return D3D_OK;
}
internal HRESULT __stdcall rd_d3d_enum_z(rd_d3d *o, const GUID *cls, HRESULT (CALLBACK *cb)(DDPIXELFORMAT *, void *), void *ctx) {
    (void)o; (void)cls;
    static const int f[] = { RF_Z16, RF_Z24, RF_Z32 };
    for (int i = 0; i < 3; i++) {
        DDPIXELFORMAT pf;
        rf_pixel_format(f[i], &pf);
        if (cb(&pf, ctx) == D3DENUMRET_CANCEL) break;
    }
    return D3D_OK;
}
internal HRESULT __stdcall rd_d3d_evict(rd_d3d *o) { (void)o; return D3D_OK; }

internal void rd3_vtables(void) {
    void **v = g_rd_vt[RDI_D3D];
    v[0] = (void *)rd_d3d_qi; v[1] = (void *)rd_d3d_addref; v[2] = (void *)rd_d3d_release; v[3] = (void *)rd_d3d_enum_devices;
    v[4] = (void *)rd_d3d_create_device; v[5] = (void *)rd_d3d_create_vb; v[6] = (void *)rd_d3d_enum_z; v[7] = (void *)rd_d3d_evict;
    v = g_rd_vt[RDI_DEV];
    v[0] = (void *)rd_dev_qi; v[1] = (void *)rd_dev_addref; v[2] = (void *)rd_dev_release; v[3] = (void *)rd_dev_get_caps;
    v[4] = (void *)rd_dev_enum_tex; v[5] = (void *)rd_dev_begin; v[6] = (void *)rd_dev_end; v[7] = (void *)rd_dev_get_direct3d;
    v[8] = (void *)rd_dev_set_target; v[9] = (void *)rd_dev_get_target; v[10] = (void *)rd_dev_clear;
    v[11] = (void *)rd_dev_set_transform; v[12] = (void *)rd_dev_get_transform; v[13] = (void *)rd_dev_set_viewport;
    v[15] = (void *)rd_dev_get_viewport; v[16] = (void *)rd_dev_set_material; v[17] = (void *)rd_dev_get_material;
    v[18] = (void *)rd_dev_set_light; v[19] = (void *)rd_dev_get_light; v[20] = (void *)rd_dev_set_rs; v[21] = (void *)rd_dev_get_rs;
    v[31] = (void *)rd_dev_draw_vb; v[32] = (void *)rd_dev_draw_indexed_vb; v[34] = (void *)rd_dev_get_texture;
    v[35] = (void *)rd_dev_set_texture; v[36] = (void *)rd_dev_get_tss; v[37] = (void *)rd_dev_set_tss;
    v[38] = (void *)rd_dev_validate; v[44] = (void *)rd_dev_light_enable; v[45] = (void *)rd_dev_get_light_enable;
    v = g_rd_vt[RDI_VB];
    v[0] = (void *)rd_vb_qi; v[1] = (void *)rd_vb_addref; v[2] = (void *)rd_vb_release; v[3] = (void *)rd_vb_lock;
    v[4] = (void *)rd_vb_unlock; v[6] = (void *)rd_vb_get_desc; v[7] = (void *)rd_vb_optimize;
}
