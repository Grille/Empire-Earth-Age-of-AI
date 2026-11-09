// The own renderer's Direct3D 7 (render.c, docs/renderer.md): Direct3D7 (devices, vertex buffers, depth formats),
// the device (its states as the game sets them, Clear) and vertex buffers (memory the game writes, 36-byte vertices of
// FVF 0x152 in the game). The draws go to Direct3D 11 through one vertex and one pixel shader that do the fixed
// function the game uses: world-view-projection, directional lights with the material sources, vertex fog, texture
// stage 0's operations, alpha test and the colour key; the depth / blend / raster states and samplers are made once
// per combination, the vertices and indices of each draw copied into one upload buffer.

// ---- vertex buffers ----
// The display DLLs' buffer is 2442 vertices (DX7Rasterizer::LockVB starts it over when fewer are left), and a ground
// material's draw writes all of its vertices from the start whatever their count: with the ground's margin
// (render_frame.c) the big ones - the grass's - went past it, written beyond the memory and their draws dropped. So
// each buffer has room for RD_VB_ROOM (an 88-tile region of one material: 47 000) and the draws are checked by it; and
// LockVB starts it over by that room (render.c render_display_loaded: by its own, a place past 2442 never started over).
#define RD_VB_ROOM 65536
typedef struct { void **vt; LONG refs; DWORD caps, fvf, n, stride, room; uint8_t *mem; } rd_vb;
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
    int sh_role;     // the draw's part in the dynamic shadows (render_shadows.c SH_), sh_b its size for the units' tint
    float sh_b;
    int sh_card;     // a tree: casts its card (render_shadows.c rd3_shadow_card), not itself
    int changed;   // states other than the world matrix set to other values since rd3_prepare last ran (RD_CH_)
} rd_device;
// the game's render objects (spec engine_display.cs), the one drawing (its draw hooked: rd_render_objects_install)
enum { RO_TERRAIN, RO_SHADOWS, RO_UNITS, RO_WATER, RO_UI, RO_N };
internal const char *const k_ro_names[RO_N] = { "terrain", "shadows", "units", "water", "interface" };
internal int g_ro_cur = -1;
enum { RD_CH_VC = 1, RD_CH_RS = 2, RD_CH_TEX = 4, RD_CH_TARGET = 8 };   // transforms, viewport, lights, material: the vertex constants
internal volatile LONG g_rd_draws, g_rd_frames;
internal DWORD g_rd_minute_tick;
internal LONGLONG g_rd_draw_qpc, g_rd_draw_qpc_told;   // time inside the draw calls (the game's thread), for the minute's line
internal LONGLONG rd_qpc(void) { LARGE_INTEGER t; QueryPerformanceCounter(&t); return t.QuadPart; }
// the draws of more vertices than the display DLL's own buffer holds (display_vb_vertices: a real Direct3D 7 one
// overrun; LockVB unpatched left its place past it for good - render.c render_display_loaded), by render object
// (RO_N: none), the most in one; and those past our room (dropped) - the minute's line
internal volatile LONG g_rd_vb_over[RO_N + 1], g_rd_vb_dropped;
internal DWORD g_rd_vb_most;
internal int rd_vb_fits(const rd_vb *vb, DWORD start, DWORD n) {
    if (n > display_vb_vertices()) {
        g_rd_vb_over[g_ro_cur >= 0 ? g_ro_cur : RO_N]++;
        if (n > g_rd_vb_most) g_rd_vb_most = n;
    }
    if (start + n <= vb->room) return 1;
    g_rd_vb_dropped++;
    return 0;
}
internal LONGLONG rd_qpc_ms(int ms) {
    static LONGLONG per_ms;
    if (!per_ms) { LARGE_INTEGER f; QueryPerformanceFrequency(&f); per_ms = f.QuadPart / 1000; }
    return per_ms * ms;
}

// ---- the 3D pipeline ----
// the vertex shader's constants (D3D7's row vectors: v * world * view * projection, row_major in HLSL); the world
// matrix and the material are the vertex's own (its draw's entry in the batch's table, rb_world), so draws that differ
// only in them batch
typedef struct {
    float vpm[16], view[16];   // view x projection, view
    float ldir[8][4], ldiff[8][4], lamb[8][4];
    float gamb[4];
    int32_t lit[4];   // lighting, lights, normalize normals
    int32_t src[4];   // diffuse, ambient, emissive from the vertex colour (1) or the material (0)
    float fog[4];     // mode (0 none, 1 exp, 2 exp2, 3 linear), start, end, density
    float vp[4];      // half a pixel in clip space (x, y), range fog
    float shm[16];    // the dynamic shadows (render_shadows.c): world -> the shadow map's u, v and depth
    float shk[4];     // the colour a shadow multiplies with (1 = none), w the strength
    float shp[4];     // the draw's role, the map's size, on (a map drawn), 1 / the map's depth range
    float shx[4], shy[4];   // the draw's x and y in the world, as shz (the hills' map)
    float shz[4];     // the draw's height in the world: its placed vertex (camera space for the ground) dotted with it
    float shl[4];     // the water's level, 1 / the depth a shadow fades out by under it, on (water seen this frame), the camera's height (the fog under the water: render_water.c)
    float shs[4];     // the game's light in the draw's space, w on: the ground's shadow weighed by how much it faces it
    float sha[4], shf[4];   // the units' ambient and the sun's colour: a shadow leaves a face its ambient part
} rd3_vcb;
// the pixel shader's: stage 0's colour op and arguments, texture bound; alpha op and arguments, alpha function
typedef struct {
    int32_t op[4], aop[4];
    float fogc[4], tf[4];
    float misc[4];   // alpha reference, fog on, colour key
    float shk[4];    // the dynamic shadows (the ground's, a decal's): their colour; the draw's role, a texel, on, the bias;
    float shd[4];
    float sht[4];    // the water's share of a shadow, shadows all on, a caster's bias
    float shh[4];    // the hills' map: the world's x, y -> u, v; the soft edge, the bias (tiles); on
    float wat[4], sea[4], wop[4], wfc[4];   // the water by its depth, the seabed's detail, the camera and 1 / the way through it that hides all, the colour under the water and the projection's depth term (render_water.c)
    float rfa[4], rfb[4];   // the reflections: strength, wobble, on, 1 / the depth of full strength; the picture's place on the target (render_water.c)
} rd3_pcb;
// a draw's entry in the batch's table (the shaders' wt, 8 rows): its world matrix (_14: a tinted caster's size -
// render_shadows.c), its material's diffuse, ambient and emissive, and a blended draw's runs (render_morph.c). The
// units' draws alternate between two materials - the white of a model's parts and the player's colour of its banner
// parts -, and as a constant of the batch's the material ended it at every other draw: 12 000 batches a frame over the
// 8000-unit save's army (16 500 draws in 24 states, 21 changes of a state other than the material)
typedef struct { float m[16], diff[4], amb[4], emi[4], mo[4]; } rb_world;
// a blended draw's vertex (render_morph.c): its index has the top bit, its entry (mo: the two runs' places in the pool,
// the fraction) and the vertex in the run - the vertex buffer isn't read for it (the input assembler's read past the
// buffer gives zeros)
#define RD3_MORPH_HLSL \
    "Buffer<float4> mp : register(t5);\n" \
    "void morph(inout I i, uint id) { if (id < 0x80000000) return; uint s = (id >> 12) & 0x7FFFF, k = id & 0xFFF; if (s == 0x7FFFF) s = i.inst;" \
    " float4 m = wt.Load(s * 8 + 7); uint a = ((uint)m.x + k) * 2, b = ((uint)m.y + k) * 2;" \
    " float4 a0 = mp.Load(a), a1 = mp.Load(a + 1), b0 = mp.Load(b), b1 = mp.Load(b + 1);" \
    " i.p = lerp(a0.xyz, b0.xyz, m.z); i.n = lerp(float3(a0.w, a1.xy), float3(b0.w, b1.xy), m.z); i.t = a1.zw;" \
    " i.c = float4(1, 1, 1, 1); i.w = s; }\n"
internal const char k_rd3_shader[] =
    "cbuffer v : register(b0) { row_major float4x4 vpm; row_major float4x4 view;"
    " float4 ldir[8]; float4 ldiff[8]; float4 lamb[8]; float4 gamb; int4 lit; int4 src; float4 fog;"
    " float4 vp; row_major float4x4 shm; float4 shk; float4 shp; float4 shx; float4 shy; float4 shz; float4 shl; float4 shs; float4 sha; float4 shf; };\n"
    "cbuffer p : register(b1) { int4 op; int4 aop; float4 fogc; float4 tf; float4 misc; float4 pshk; float4 shd; float4 sht;"
    " float4 shh; float4 wat; float4 sea; float4 wop; float4 wfc; float4 rfa; float4 rfb; };\n"
    "cbuffer g : register(b3) { float4 gnd; float4 gla; float4 gld[8]; float4 glc[8]; };\n"   // the ground from the map (render_ground.c): on, the wobble's phase and strength, all seen; the game's lights: ambient and how many, toward each, each's colour
    "Texture2D<float> gf : register(t7);\n"   // its cells' sight: 0, a half, all
    "cbuffer e : register(b2) { float4 edp; };\n"   // which part of a texture this draw is (render_frame.c rf_edges)
    "Buffer<float4> wt : register(t1);\n"   // the draws' entries, 8 rows each (rb_world)
    "Texture2D t : register(t0); SamplerState s : register(s0);\n"
    "Texture2D<float> sm : register(t2); SamplerComparisonState sc : register(s1);\n"   // the dynamic shadows' maps:
    "Texture2D rfl : register(t6); Texture2D wsc : register(t8); Texture2D<float> wsd : register(t9);\n"
    "Texture2D<float2> hm : register(t4); SamplerState hs : register(s2);\n"   // the casters', the hills' (render_shadows.c)
    "struct I { float3 p : POSITION; float3 n : NORMAL; float4 c : COLOR; float2 t : TEXCOORD; uint w : WORLD; uint inst : INST; };\n"
    RD3_MORPH_HLSL
    "struct O { float4 p : SV_Position; float4 c : COLOR; float2 t : TEXCOORD0; float f : FOG; float4 q : TEXCOORD1;"
    " float3 k : TEXCOORD2; float3 w : TEXCOORD3; };\n"
    // a unit's tint: how many of four texels around its origin's place in the map are nearer the sun by more than b
    "float shade_at(float3 q, float b) { if (q.x < 0 || q.y < 0 || q.x > 1 || q.y > 1) return 0;"
    " int n = (int)shp.y; int2 c = int2(q.xy * shp.y); float k = 0;"
    " for (int j = 0; j < 4; j++) { int2 a = clamp(c + int2(j & 1, j >> 1) * 4 - 2, 0, n - 1);"
    "  k += sm.Load(int3(a, 0)) < q.z - b ? 1 : 0; } return k / 4; }\n"
    "float fog_at(float z) { if (fog.x == 3) return (fog.z - z) / (fog.z - fog.y); if (fog.x == 1) return exp(-fog.w * z);"
    " if (fog.x == 2) return exp(-(fog.w * z) * (fog.w * z)); return 1; }\n"
    // a normal through a world matrix that doesn't keep right angles (a 1:2 wall's piece is sheared across its length,
    // walls.c: taken as a direction a face's normal followed the sheared side, and pieces of one run were lit as a
    // straight piece or a diagonal one - the player, 2026-11-08, "why is the selected wall dark?"): the matrix's cofactors,
    // the inverse's transpose but for the determinant, as Direct3D's fixed function does. Its length as before, for a
    // draw without NORMALIZENORMALS; a flat matrix has none of its own. A rotation and a scale give the same direction
    "float3 world_normal(float3 n, float3x3 w) { float3 a = cross(w[1], w[2]), o = mul(n, w);"
    " float3 m = n.x * a + n.y * cross(w[2], w[0]) + n.z * cross(w[0], w[1]); float l = dot(m, m);"
    " return l > 1e-12 ? m * sign(dot(w[0], a)) * sqrt(dot(o, o) / l) : o; }\n"
    // the ground from the map: its seabed moved as TransformToCamera does, its brightness the fog of war (GETerrainMesh::UpdateFogOfWar),
    // the game's lights on the table's colour, clamped before it as DX7Rasterizer::Light does (a warm stone washes out otherwise);
    // the water (gnd.x 2): the height it holds is the ground's under it, the surface moves about the level, over land half as much and below it
    "void ground(inout I i) { int X = (int)i.p.x, Y = (int)i.p.y; float wv = sin(i.p.x * i.p.y + gnd.y); if (gnd.x > 1.5) i.p.z = i.p.z < 0 ? wv * gnd.z : (wv - 1) * gnd.z * 0.5;"
    " else if (i.p.z < -0.5) i.p.z -= wv * gnd.z; float l = 1;"
    " if (gnd.w == 0) { l = 0; for (int j = 0; j < 4; j++) { int2 a = int2(X - (j & 1), Y - (j >> 1)); if (a.x >= 0 && a.y >= 0) l += round(gf.Load(int3(a, 0)) * 2) * 0.125; } }"
    " float3 L = gla.rgb; for (int k = 0; k < (int)gla.w; k++) L += glc[k].rgb * max(0, dot(i.n, gld[k].xyz)); i.c.rgb *= saturate(L).bgr * l; i.n = 0; }\n"
    "O vs(I i, uint id : SV_VertexID) { O o; morph(i, id); if (gnd.x > 0) ground(i); uint r = i.w * 8; float4 r0 = wt.Load(r); float b = r0.w; r0.w = 0;"   // _14: the draw's size (SH_TINT)
    " float4x4 world = gnd.x > 0 ? float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1) : float4x4(r0, wt.Load(r + 1), wt.Load(r + 2), wt.Load(r + 3));"
    " float4 p = mul(float4(i.p, 1), world); o.p = mul(p, vpm); o.q.xyz = mul(p, shm).xyz;"
    // under water the shadow fades out with the depth (the sun's light scattered in the water)
    " o.w = float3(dot(p, shx), dot(p, shy), dot(p, shz));"
    " o.q.w = shl.z > 0 ? saturate(1 - (shl.x - o.w.z) * shl.y) : 1;"
    " o.p.xy += vp.xy * o.p.w;"
    " float4 vc = i.c.zyxw, c = vc; o.k = shk.rgb;"   // D3DCOLOR's bytes are B G R A
    // the ground in a shadow keeps its ambient part of the light it faces: a slope turned from the game's light (its
    // baked shading already dark) loses nothing - the shadows' sun, never lower than 25 degrees, isn't the shading's
    " if (shs.w > 0 && dot(i.n, i.n) > 0) {"
    "  float3 lt = sha.rgb + shf.rgb * max(0, dot(normalize(i.n), -shs.xyz));"
    "  o.k = 1 - shk.w * (1 - saturate(sha.rgb / max(lt, 0.001))); }"
    " if (lit.x) { float4 d = src.x ? vc : wt.Load(r + 4), a = src.y ? vc : wt.Load(r + 5), e = src.z ? vc : wt.Load(r + 6);"
    "  float3 n = world_normal(i.n, (float3x3)world); if (lit.z) n = normalize(n);"
    "  float3 am = gamb.rgb, df = 0;"
    "  for (int k = 0; k < lit.y; k++) { am += lamb[k].rgb; df += ldiff[k].rgb * max(0, dot(n, -ldir[k].xyz)); }"
    "  if (shp.x == 3 && shp.z > 0) df *= 1 - shk.w * shade_at(mul(float4(world[3].xyz, 1), shm).xyz, b * shp.w);"
    "  c = float4(saturate(e.rgb + a.rgb * am + d.rgb * df), d.a);"
    // a caster in a shadow (shadows all on): its colour without the lights' diffuse part, as much as the strength takes
    "  o.k = saturate(e.rgb + a.rgb * am + d.rgb * df * (1 - shk.w)) / max(c.rgb, 0.001); }"
    " o.c = c; o.t = i.t;"
    " float3 q = mul(p, view).xyz; float z = vp.z > 0.5 ? length(q) : q.z, f = fog_at(z);"
    // what lies under the water takes the fog of the place the line of sight enters it (render_water.c) - the game's
    // own again toward the fog's end, where the ground's mesh stops
    " if (shp.x > 0 && shl.w > shl.x && o.w.z < shl.x) { float e = fog_at(z * (shl.w - shl.x) / (shl.w - o.w.z));"
    "  f = lerp(f, e, saturate(e / 0.7)); }"
    " o.f = f; return o; }\n"   // clamped per pixel, as the driver under a wrapper does (the picture compare)
    // D3DTA_: 0 diffuse, 1 current (the diffuse at stage 0), 2 texture, 3 texture factor; D3DTOP_ 1..8
    "float4 arg(int a, float4 x, float4 d) { a = a & 15; return a == 2 ? x : a == 3 ? tf : d; }\n"
    "float4 top(int o, float4 a, float4 b) { return o == 2 ? a : o == 3 ? b : o == 4 ? a * b : o == 5 ? saturate(a * b * 2)"
    " : o == 6 ? saturate(a * b * 4) : o == 7 ? saturate(a + b) : o == 8 ? saturate(a + b - 0.5) : a * b; }\n"
    "float4 ps(O i) : SV_Target { float4 x = op.w ? t.Sample(s, i.t) : float4(1, 1, 1, 1), d = i.c, c = d;"
    " if (misc.z > 0.5 && x.a == 0) discard;"
    // the seabed loses its texture's detail with the depth, keeping its colour: toward the texture's last mip level
    " if (sea.x > 0 && shd.x == 1 && i.w.z < sea.z) x.rgb = lerp(x.rgb, t.SampleLevel(s, i.t, 8).rgb,"
    "  sea.x * (1 - exp(-(sea.z - i.w.z) * sea.y)));"
    " if (op.x != 1) { c.rgb = top(op.x, arg(op.y, x, d), arg(op.z, x, d)).rgb;"
    "  if (aop.x != 1) c.a = top(aop.x, arg(aop.y, x, d), arg(aop.z, x, d)).a; }"
    " int A = (int)round(c.a * 255), R = (int)misc.x, f = aop.w;"
    " if (f && !(f == 8 || (f == 2 && A < R) || (f == 3 && A == R) || (f == 4 && A <= R) || (f == 5 && A > R)"
    "  || (f == 6 && A != R) || (f == 7 && A >= R))) discard;"
    " if (edp.x > 0 ? x.a < edp.x : edp.x < 0 && x.a >= -edp.x) discard;"
    // the ground, a decal or field on it, the water: 3x3 comparisons with the casters' map, its colour in a shadow as
    // the vertex shader worked it out; with shadows all on the units, buildings and trees too, their colour without
    // the sun's part. The hills' map: in their shadow below its height (a soft edge); the darker of the two shadows
    // the water by its depth (render_water.c): the depth is the hills' map's ground (its
    // ceiling less how far the ground lies under it) under the surface's own height
    " if (wat.x > 0 && shd.x == 6 && shh.w > 0) { float2 wh = hm.SampleLevel(hs, i.w.xy * shh.x, 0);"
    "  float wd = max(0, i.w.z - (wh.x - wh.y));"
    "  c.a = saturate(c.a * lerp(wat.y, wat.z, 1 - exp(-wd * wat.x))) * saturate(wd * wat.w); }"
    " bool g = shd.x == 1 || shd.x == 4 || shd.x == 6, rc = sht.y > 0 && (shd.x == 2 || shd.x == 3 || shd.x == 9);"
    " if ((g || rc) && (shd.z > 0 || shh.w > 0)) { float z = min(i.q.z, 0.9999), l = 9, b = g ? shd.w : sht.z, hl = 0;"
    "  if (shd.z > 0) { l = 0; for (int y = -1; y <= 1; y++) for (int u = -1; u <= 1; u++)"
    "   l += sm.SampleCmpLevelZero(sc, i.q.xy + float2(u, y) * shd.y, z - b); }"
    // The ground and what lies on it read how far the map's own ground is under the ceiling (its second number, 0 where
    // the sun reaches): no comparison of the map's texels with the ground's triangles, which part on a slope, so the edge
    // can be sharp everywhere - where a hill's shadow lands and a unit on top of the hill must meet it (a fade from the
    // true edge to 0.75 tiles inside it left the two apart, the player 2026-11-08). A unit or the water stands above the
    // ground: its own height against the ceiling, as soft as ever
    "  if (shh.w > 0) { float2 hh = hm.SampleLevel(hs, i.w.xy * shh.x, 0); bool og = shd.x == 1 || shd.x == 4;"
    "   hl = saturate(((og ? hh.y : hh.x - i.w.z) - (og ? 0.01 : shh.z)) / (og ? min(0.12, shh.y) : shh.y)); }"
    "  c.rgb *= lerp(1, i.k, max(1 - l / 9, hl) * i.q.w * (shd.x == 6 ? sht.x : 1)); }"
    // the mirrored picture on the surface (render_water.c), moved a little by the water's own ripples, none of it at
    // the shore, all from the depth of full strength on; in that picture's own pass what lies under the level is left out and what
    // the fog has taken goes with it (its alpha the object's own fog)
    " if (rfa.z > 0 && shd.x == 6) { float2 ru = (i.p.xy - rfb.xy) * rfb.zw + (x.rg - 0.5) * rfa.y;"
    "  float rk = rfa.x; if (shh.w > 0) { float2 rh = hm.SampleLevel(hs, i.w.xy * shh.x, 0);"
    "   rk *= smoothstep(0, 1, (i.w.z - (rh.x - rh.y)) * rfa.w); }"
    "  float4 rc = rfl.SampleLevel(hs, ru, 0); c.rgb = lerp(c.rgb, rc.rgb, rc.a * rk); c.a = lerp(c.a, 1, rc.a * rk); }"
    " if (sea.w > 0) { if (i.w.z < sea.z) discard; c.a = misc.y > 0.5 ? saturate(i.f) : 1; }"
    " if (misc.y > 0.5) c.rgb = lerp(fogc.rgb, c.rgb, saturate(i.f));"
    " return c; }\n"
    // the water's step over the picture (render_water.c rd3_water_pass): a pixel of the surface, what was drawn behind it
    // from the copy toward the water's colour by the way from the surface to it (views' depths: the ratio of the two along
    // the same line of sight); nothing drawn behind it is the farthest there is. The surface's vertices bring the light
    // and the game's fade to black at what isn't explored, its fog the distance's
    "float4 wfog(O i) : SV_Target { int3 a = int3(i.p.xy, 0); float d = wsd.Load(a), wp = 1;"
    " if (d < 0.99999) wp = saturate(length(i.w - wop.xyz) * max((i.p.z - wfc.w) / (d - wfc.w) - 1, 0) * wop.w);"
    " float3 fc = wfc.rgb * i.c.rgb; if (misc.y > 0.5) fc = lerp(fogc.rgb, fc, saturate(i.f));"
    " return float4(lerp(wsc.Load(a).rgb, fc, wp * wp * (3 - 2 * wp)), 1); }\n"
    // a unit seen through a building (render_silhouette.c): vs's own, its entry's fourth number the player's colour; the
    // colour lit as the unit is - the brightest channel of its vertex colour, which is the light on a white part and on a
    // part in the player's colour alike - where a building's surface is what the pixel shows and the unit lies behind it
    "O vsil(I i, uint id : SV_VertexID) { uint r = id >= 0x80000000 ? (id >> 12) & 0x7FFFF : (uint)i.w; if (r == 0x7FFFF) r = i.inst; r *= 8; O o = vs(i, id);"
    " uint q = (uint)wt.Load(r + 7).w; o.k = float3((q >> 16) & 255, (q >> 8) & 255, q & 255) / 255; return o; }\n"
    "Texture2D<float> slb : register(t10); Texture2D<float> slz : register(t11);\n"
    "float4 psil(O i) : SV_Target { int3 a = int3(i.p.xy, 0); float bz = slb.Load(a);"
    " if (i.p.z <= bz + 1e-6 || slz.Load(a) + 2e-6 < bz) discard;"
    " float e = 1; if (op.w) { e = t.Sample(s, i.t).a; if (e < 0.5) discard; }"
    " float3 c = i.k * max(i.c.r, max(i.c.g, i.c.b)); if (misc.y > 0.5) c = lerp(fogc.rgb, c, saturate(i.f));"
    " return float4(c, e); }\n"
    // the silhouettes' finished picture over the scene, see-through as one
    "Texture2D slc : register(t12);\n"
    "float4 pslc(float4 p : SV_Position, float2 uv : TEXCOORD0) : SV_Target { float4 c = slc.Load(int3(p.xy, 0)); if (c.a == 0) discard;"
    " return float4(c.rgb, c.a * 0.55); }\n";

// The upload buffers, rings the frame's list draws from (render_frame.c): a busy base's frame about 4 MB of vertices.
// A ring full while the list still holds draws draws the list first (rd3_upload) - the frame then in parts, and a
// part's ground gets the shadows of only its own casters: over the 8000-unit save's archers (3 million vertices a
// frame) 2 parts more a frame, the shadows of the rows of units after the cut gone, a cut that moved every frame
// (the player: "the shadows repeatedly load in like scanlines"). Such a ring is made twice as big then, up to 8
// times its first size, so the next frames fit in one.
#define RD3_VB_BYTES (48u << 20)
#define RD3_IB_BYTES (16u << 20)
#define RD3_WB_BYTES (4u << 20)   // the batches' entries (rb_world), appended like the vertices (no discard a batch)
#define RD3_GROW_MAX 8
// THE BATCH: the game draws an object at a time (5000 draws a frame in a busy base, each a few dozen triangles), and
// a draw call costs its time on the game's thread whatever its size. Draws in a row whose states match - the same
// texture, blend, lights ... - differing only in their world matrix and material are collected and drawn as one: their
// vertices (with their entry's index in the batch's table, rb_world) and indices into the upload buffers, then one
// DrawIndexed. Strips
// become lists (one batch takes many). A state that changes (rd3_prepare) ends the batch (rd3_flush): into the frame's
// list (render_frame.c), drawn when the frame is done or something else needs the context (rd3_submit).
#define RB_VERTS 65536       // a draw's: RD_VB_ROOM's (the ground's materials with the margin, render_frame.c)
#define RB_INDICES (RB_VERTS * 3)
#define RB_WORLDS 4096       // entries: a blended draw has one of its own (its runs)
typedef struct { uint8_t fvf[36]; uint32_t w; } rb_vertex;   // FVF 0x152 and its entry's index
internal struct {
    rb_vertex *v;
    uint32_t *i;
    rb_world *w;
    int nv, ni, nw;
    D3D11_PRIMITIVE_TOPOLOGY topo;
    int morph;      // the batch's draws are blended ones (render_morph.c): indices only, no vertices; 2: instances (render_instance.c)
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
    UINT vb_cap, ib_cap, wb_cap;   // their sizes (bytes): RD3_*_BYTES, grown (rd3_upload)
    rd3_vcb vc;
    rd3_pcb pc;
    // the states the batch is collected under (the frame's list draws with them)
    ID3D11RenderTargetView *rtv;
    ID3D11DepthStencilView *dsv;
    void *blend, *depth, *raster, *samp;
    void *depth_soft;   // the depth state as it is without writing (rf_edges); NULL: it writes none anyway
    int sil;            // the batch's part in the silhouettes (render_silhouette.c SIL_)
    int blended;
    ID3D11ShaderResourceView *srv;
    rd_surface *srv_of;   // the surface srv shows (its chain's top) and whether keyed: render_frame.c's kept interface
    int srv_keyed;
    D3D11_VIEWPORT vport;
    rd3_cached blends[32], depths[32], rasters[8], samps[32];
    int n_blend, n_depth, n_raster, n_samp;
    // the map's camera this frame (render_overlay.c draws with it): view x projection, the viewport; the overlay drawn
    float scene_vp[16], scene_view[16];   // and its view (render_frame.c rf_shift: the camera's axes)
    D3D11_VIEWPORT scene_port;
    int scene_seen, overlay_done;
    rd_device *prep_dev;   // whose states are bound (rd3_prepare), with which clamp
    int prep_clamp;
} g_rd3;

// an upload ring of `bytes` into *b (bind: its use - the world matrices' with their view, g_rd3.wsrv); the old one
// released only once the new is made
internal HRESULT rd3_make_upload(ID3D11Buffer **b, UINT bytes, UINT bind) {
    D3D11_BUFFER_DESC ud = { bytes, D3D11_USAGE_DYNAMIC, bind, D3D11_CPU_ACCESS_WRITE, 0, 0 };
    ID3D11Buffer *nb = NULL;
    ID3D11ShaderResourceView *nv = NULL;
    HRESULT hr = ID3D11Device_CreateBuffer(g_d3d.dev, &ud, NULL, &nb);
    if (SUCCEEDED(hr) && bind == D3D11_BIND_SHADER_RESOURCE) {
        D3D11_SHADER_RESOURCE_VIEW_DESC sd = { .Format = DXGI_FORMAT_R32G32B32A32_FLOAT, .ViewDimension = D3D11_SRV_DIMENSION_BUFFER };
        sd.Buffer.NumElements = bytes / 16;
        hr = ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)nb, &sd, &nv);
        if (FAILED(hr)) D3D_RELEASE(nb);
    }
    if (FAILED(hr)) return hr;
    D3D_RELEASE(*b);
    *b = nb;
    if (nv) { D3D_RELEASE(g_rd3.wsrv); g_rd3.wsrv = nv; }
    return hr;
}
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
        { "WORLD", 0, DXGI_FORMAT_R32_UINT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },     // rb_vertex
        { "INST", 0, DXGI_FORMAT_R32_UINT, 1, 0, D3D11_INPUT_PER_INSTANCE_DATA, 1 } };   // render_instance.c
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateInputLayout(g_d3d.dev, el, 6, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), &g_rd3.inputs);
    D3D_RELEASE(code);
    code = SUCCEEDED(hr) ? d3d_compile(k_rd3_shader, sizeof k_rd3_shader - 1, "ps", "ps_4_0") : NULL;
    hr = code ? ID3D11Device_CreatePixelShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_rd3.ps) : E_FAIL;
    D3D_RELEASE(code);
    D3D11_BUFFER_DESC bd = { sizeof(rd3_vcb), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_rd3.vcb);
    bd.ByteWidth = sizeof(rd3_pcb);
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_rd3.pcb);
    if (SUCCEEDED(hr)) hr = rd3_make_upload(&g_rd3.vb, g_rd3.vb_cap = RD3_VB_BYTES, D3D11_BIND_VERTEX_BUFFER);
    if (SUCCEEDED(hr)) hr = rd3_make_upload(&g_rd3.ib, g_rd3.ib_cap = RD3_IB_BYTES, D3D11_BIND_INDEX_BUFFER);
    if (SUCCEEDED(hr)) hr = rd3_make_upload(&g_rd3.wb, g_rd3.wb_cap = RD3_WB_BYTES, D3D11_BIND_SHADER_RESOURCE);
    if (FAILED(hr)) return rd_fail("the 3D pipeline", hr), 0;
    g_rb.v = malloc(RB_VERTS * sizeof *g_rb.v);
    g_rb.i = malloc(RB_INDICES * sizeof *g_rb.i);
    g_rb.w = malloc(RB_WORLDS * sizeof *g_rb.w);
    if (!g_rb.v || !g_rb.i || !g_rb.w) return rd_fail("the 3D pipeline's batch", E_OUTOFMEMORY), 0;
    g_rd3.vb_pos = g_rd3.vb_cap;   // the first write discards
    g_rd3.ib_pos = g_rd3.ib_cap;
    g_rd3.wb_pos = g_rd3.wb_cap;
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
// linear), the mip bias in 1/16, the anisotropy (bits 20..24)
internal void *rd3_mk_samp(uint32_t key) {
    D3D11_SAMPLER_DESC sd = { 0 };
    DWORD u = key & 7, v = key >> 3 & 7, mag = key >> 6 & 3, min = key >> 8 & 3, mip = key >> 10 & 3;
    sd.AddressU = u >= 1 && u <= 4 ? (D3D11_TEXTURE_ADDRESS_MODE)u : D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressV = v >= 1 && v <= 4 ? (D3D11_TEXTURE_ADDRESS_MODE)v : D3D11_TEXTURE_ADDRESS_WRAP;
    sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
    sd.Filter = (D3D11_FILTER)((min >= 2 ? 0x10 : 0) | (mag >= 2 ? 0x4 : 0) | (mip == 3 ? 0x1 : 0));
    sd.MipLODBias = (float)(int8_t)(key >> 12 & 255) / 16.0f;
    // the game's linear minifying with mip levels as anisotropic: the ground at a slant kept sharp ([display] anisotropy)
    DWORD an = key >> 20 & 31;
    sd.MaxAnisotropy = an > 1 && min >= 2 && mip >= 2 ? an : 1;
    if (sd.MaxAnisotropy > 1) sd.Filter = D3D11_FILTER_ANISOTROPIC;
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

internal volatile LONG g_rp_state;   // the probe (below)
internal void rec_count(int role, DWORD nv);   // render_record.c: a draw's role counted for the frame
internal ID3D11Buffer *g_in_ib, *g_in_ids;   // the units' instances (render_instance.c): the triangles' pool, the instance stream
internal int g_in_pool, g_in_force = -1;      // the pool is the index buffer bound now; the pipe's (-1: the ini's)
internal LONG g_in_parts, g_in_fell;          // the pipe's: parts drawn that way, models left to the display DLL
internal void in_flush(void);
internal ID3D11ShaderResourceView *g_mo_srv;   // the animation's pool (render_morph.c), bound with the list
internal volatile LONG g_rp_state;   // the pipe's rprobe (below)
#include "render_frame.c"     // the frame recorded and drawn at once
#include "render_shadows.c"   // the dynamic shadows ([display] shadows = dynamic)
#include "render_silhouette.c"   // units seen through the buildings in front of them ([display] unit_silhouettes)
#include "render_water.c"     // the water by its depth, its reflections ([display] water_look, water_reflections)
#include "render_morph.c"     // the units' animation blended on the graphics card ([display] gpu_animation)
#include "render_ground.c"    // the ground from the map itself, on the GPU (the own renderer's only ground)
#include "render_fxaa.c"      // the picture's sharpness ([display] antialias, anisotropy)

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
// the device's states into the shaders' constants and the states the batch is collected under; 0: nothing to draw
// into. clamp: CLAMP addressing
internal int rd3_prepare(rd_device *d, int clamp) {
    rd_surface *t = d->target;
    if (!t || !t->rtv || !rd3_init()) return 0;
    // nothing but the world matrix changed since the last draw (the game sets it for each object, most other states
    // over and over to the values they have): the states bound are this draw's - the work below cost more than the draw
    rd_surface *tx = d->tex[0] ? (d->tex[0]->top ? d->tex[0]->top : d->tex[0]) : NULL;
    if (!d->changed && g_rd3.bound && g_rd3.prep_dev == d && g_rd3.prep_clamp == clamp && g_rd3.rtv == t->rtv
        && g_rd3.dsv == (t->zbuf ? t->zbuf->dsv : NULL) && !(tx && tx->kind != RK_TARGET && tx->dirty))
        return 1;
    const DWORD *rs = d->rs;
    int full = !g_rd3.bound;
    g_rd3.bound = 1;
    // each state: the batch so far ended with the old one before it changes
    ID3D11DepthStencilView *dsv = t->zbuf ? t->zbuf->dsv : NULL;
    if (full || g_rd3.rtv != t->rtv || g_rd3.dsv != dsv) { rd3_flush(); g_rd3.rtv = t->rtv; g_rd3.dsv = dsv; }
    D3D11_VIEWPORT vp = { (float)d->viewport.dwX, (float)d->viewport.dwY, (float)d->viewport.dwWidth,
                          (float)d->viewport.dwHeight, d->viewport.dvMinZ, d->viewport.dvMaxZ };
    if (full || memcmp(&vp, &g_rd3.vport, sizeof vp)) { rd3_flush(); g_rd3.vport = vp; }
    uint32_t bk = rs[D3DRENDERSTATE_ALPHABLENDENABLE] ? 1 | (rs[D3DRENDERSTATE_SRCBLEND] & 15) << 1 | (rs[D3DRENDERSTATE_DESTBLEND] & 15) << 5 : 0;
    void *o = rd3_cache(g_rd3.blends, &g_rd3.n_blend, 32, bk, rd3_mk_blend);
    if (full || o != g_rd3.blend) { rd3_flush(); g_rd3.blend = o; g_rd3.blended = bk & 1; }
    uint32_t dk = (rs[D3DRENDERSTATE_ZENABLE] ? 1 : 0) | (rs[D3DRENDERSTATE_ZWRITEENABLE] ? 2 : 0) | (rs[D3DRENDERSTATE_ZFUNC] & 15) << 2;
    o = rd3_cache(g_rd3.depths, &g_rd3.n_depth, 32, dk, rd3_mk_depth);
    if (full || o != g_rd3.depth) { rd3_flush(); g_rd3.depth = o; g_rd3.depth_soft = dk & 2 ? rd3_cache(g_rd3.depths, &g_rd3.n_depth, 32, dk & ~2u, rd3_mk_depth) : NULL; }
    o = rd3_cache(g_rd3.rasters, &g_rd3.n_raster, 8, rs[D3DRENDERSTATE_CULLMODE] & 3, rd3_mk_raster);
    if (full || o != g_rd3.raster) { rd3_flush(); g_rd3.raster = o; }

    // texture stage 0 (stage 1 is always off in the game)
    const DWORD *ts = d->tss[0];
    rd_surface *tex = d->tex[0];
    int ckey = rs[D3DRENDERSTATE_COLORKEYENABLE] && tex && (tex->top ? tex->top : tex)->has_key;
    ID3D11ShaderResourceView *srv = tex ? rd_gpu_texture(tex, ckey) : NULL;
    if (tex) (tex->top ? tex->top : tex)->rf_used = g_rd_epoch;   // in the list: its pixels kept until it is drawn
    if (full || srv != g_rd3.srv) { rd3_flush(); g_rd3.srv = srv; }
    g_rd3.srv_of = tex ? (tex->top ? tex->top : tex) : NULL;
    g_rd3.srv_keyed = ckey;
    int bias = (int)(rd3_float(ts[D3DTSS_MIPMAPLODBIAS]) * 16.0f);
    uint32_t au = clamp ? D3DTADDRESS_CLAMP : ts[D3DTSS_ADDRESSU], av = clamp ? D3DTADDRESS_CLAMP : ts[D3DTSS_ADDRESSV];
    uint32_t sk = (au & 7) | (av & 7) << 3 | (ts[D3DTSS_MAGFILTER] & 3) << 6 |
                  (ts[D3DTSS_MINFILTER] & 3) << 8 | (ts[D3DTSS_MIPFILTER] & 3) << 10 | (uint32_t)(bias & 255) << 12 |
                  (uint32_t)(g_fx.aniso & 31) << 20;
    o = rd3_cache(g_rd3.samps, &g_rd3.n_samp, 32, sk, rd3_mk_samp);
    if (full || o != g_rd3.samp) { rd3_flush(); g_rd3.samp = o; }

    // the vertex shader's constants - not when only a texture or its stage changed (most often, between two batches)
    if (full || g_rd3.prep_dev != d || (d->changed & (RD_CH_VC | RD_CH_RS))) {
        rd3_vcb vc;
        memset(&vc, 0, sizeof vc);
        rd3_mul(vc.vpm, &d->view._11, &d->proj._11);
        memcpy(vc.view, &d->view._11, sizeof vc.view);
        if (rs[D3DRENDERSTATE_ZENABLE] && (d->view._41 != 0 || d->view._42 != 0 || d->view._43 != 0)) {
            // the map's camera (a view away from the origin - the interface's 3D pieces have none): the overlay's
            memcpy(g_rd3.scene_vp, vc.vpm, sizeof vc.vpm);
            memcpy(g_rd3.scene_view, vc.view, sizeof vc.view);
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
        rd3_shadow_constants(d, &vc);
        if (full || memcmp(&vc, &g_rd3.vc, sizeof vc)) { rd3_flush(); g_rd3.vc = vc; g_rf.vc_new = 1; }
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
    rd3_shadow_pconstants(d, &pc);
    if (full || memcmp(&pc, &g_rd3.pc, sizeof pc)) { rd3_flush(); g_rd3.pc = pc; g_rf.pc_new = 1; }
    d->changed = 0;
    g_rd3.prep_dev = d; g_rd3.prep_clamp = clamp;
    return 1;
}

// n bytes into an upload ring (*b, *cap bytes; bind its use) at a multiple of align; the offset, or -1
internal long rd3_upload(ID3D11Buffer **b, UINT *cap, UINT bind, UINT *pos, const void *src, UINT n, UINT align) {
    if (n > *cap) return -1;
    UINT at = (*pos + align - 1) / align * align;
    D3D11_MAP how = D3D11_MAP_WRITE_NO_OVERWRITE;
    if (at + n > *cap) {   // the ring full: what the list draws from drawn first (render_frame.c), then the ring bigger
        if (g_rf.n) {
            g_rf.wraps++;
            rf_replay();
            UINT first = bind == D3D11_BIND_VERTEX_BUFFER ? RD3_VB_BYTES : bind == D3D11_BIND_INDEX_BUFFER ? RD3_IB_BYTES : RD3_WB_BYTES;
            if (*cap < first * RD3_GROW_MAX && SUCCEEDED(rd3_make_upload(b, *cap * 2, bind))) {
                *cap *= 2;
                log_locked("# render: the %s' upload ring full inside a frame (the frame drawn in parts, its shadows cut):"
                           " made %u MB\n", bind == D3D11_BIND_VERTEX_BUFFER ? "vertices" : bind == D3D11_BIND_INDEX_BUFFER
                           ? "indices" : "world matrices", *cap >> 20);
            }
        }
        at = 0;
        how = D3D11_MAP_WRITE_DISCARD;
    }
    D3D11_MAPPED_SUBRESOURCE ms;
    if (FAILED(ID3D11DeviceContext_Map(g_d3d.ctx, (ID3D11Resource *)*b, 0, how, 0, &ms))) return -1;
    memcpy((uint8_t *)ms.pData + at, src, n);
    ID3D11DeviceContext_Unmap(g_d3d.ctx, (ID3D11Resource *)*b, 0);
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

// the batch into the upload buffers and the frame's list (render_frame.c), with the states it was collected under
internal void rd3_flush(void) {
    if (!g_rb.ni) return;
    if (g_rb.morph == 2) { in_flush(); return; }
    rf_keep_ground();
    long wa = rd3_upload(&g_rd3.wb, &g_rd3.wb_cap, D3D11_BIND_SHADER_RESOURCE, &g_rd3.wb_pos, g_rb.w, (UINT)g_rb.nw * sizeof *g_rb.w, sizeof *g_rb.w);
    if (wa >= 0 && g_rb.morph) {   // indices only: their entries' slots moved to the matrices' place
        uint32_t add = (uint32_t)wa / sizeof *g_rb.w << 12;
        if (add) for (int k = 0; k < g_rb.ni; k++) g_rb.i[k] += add;
        long ia = rd3_upload(&g_rd3.ib, &g_rd3.ib_cap, D3D11_BIND_INDEX_BUFFER, &g_rd3.ib_pos, g_rb.i, (UINT)g_rb.ni * 4, 4);
        if (ia >= 0) rf_record((UINT)ia / 4, (UINT)g_rb.ni, 0);
        g_rb.nv = g_rb.ni = g_rb.nw = 0;
        return;
    }
    if (wa > 0) for (int k = 0; k < g_rb.nv; k++) g_rb.v[k].w += (uint32_t)wa / sizeof *g_rb.w;   // the batch's matrices' place
    long at = wa >= 0 ? rd3_upload(&g_rd3.vb, &g_rd3.vb_cap, D3D11_BIND_VERTEX_BUFFER, &g_rd3.vb_pos, g_rb.v, (UINT)g_rb.nv * sizeof *g_rb.v, sizeof *g_rb.v) : -1;
    long ia = at >= 0 ? rd3_upload(&g_rd3.ib, &g_rd3.ib_cap, D3D11_BIND_INDEX_BUFFER, &g_rd3.ib_pos, g_rb.i, (UINT)g_rb.ni * 4, 4) : -1;
    if (ia >= 0) rf_record((UINT)ia / 4, (UINT)g_rb.ni, (INT)((UINT)at / sizeof *g_rb.v));
    g_rb.nv = g_rb.ni = g_rb.nw = 0;
}
// a draw's entry: its world matrix (_14, 0 in the game's matrices: a tinted caster's size for the shader -
// render_shadows.c) and material
internal void rd3_entry(const rd_device *d, rb_world *e) {
    memset(e, 0, sizeof *e);
    memcpy(e->m, &d->world._11, 64);
    if (d->sh_role == SH_TINT) e->m[3] = d->sh_b;
    rd3_value(e->diff, &d->material.diffuse);
    rd3_value(e->amb, &d->material.ambient);
    rd3_value(e->emi, &d->material.emissive);
    e->mo[3] = sil_colour();
}
// a draw into the batch (rd3_prepare bound its states): nv vertices from start, ni indices (NULL: 0..nv-1) of type
internal void rd3_append(rd_device *d, D3DPRIMITIVETYPE type, rd_vb *vb, DWORD start, DWORD nv, const WORD *idx, DWORD ni) {
    int line = type == D3DPT_LINELIST || type == D3DPT_LINESTRIP, strip = type == D3DPT_TRIANGLESTRIP || type == D3DPT_LINESTRIP;
    D3D11_PRIMITIVE_TOPOLOGY topo = line ? D3D11_PRIMITIVE_TOPOLOGY_LINELIST : type == D3DPT_POINTLIST
                                  ? D3D11_PRIMITIVE_TOPOLOGY_POINTLIST : D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    DWORD n = idx ? ni : nv;   // the primitives' vertices as given
    DWORD out = !strip ? n : line ? (n >= 2 ? (n - 1) * 2 : 0) : (n >= 3 ? (n - 2) * 3 : 0);
    if (!out || nv > RB_VERTS || out > RB_INDICES) return;
    sil_batch();
    if (g_rb.ni && (g_rb.morph || g_rb.topo != topo || g_rb.nv + (int)nv > RB_VERTS || g_rb.ni + (int)out > RB_INDICES || g_rb.nw >= RB_WORLDS))
        rd3_flush();
    g_rb.morph = 0;
    rb_world e;
    rd3_entry(d, &e);
    if (!g_rb.nw || memcmp(&g_rb.w[g_rb.nw - 1], &e, sizeof e)) g_rb.w[g_rb.nw++] = e;
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
// The interface's lines on a larger target ([editor] editor_uiscale). The engine draws a border N of its pixels wide as
// N lines one of its pixels apart (UIControl::DrawBorder), a line is one pixel of the picture thick whatever the scale,
// so the rows of the picture between them stayed open. Such a line is drawn as what it covers at the game's own size,
// a strip one of its pixels wide, stopping short of its end as a line does. The strip starts at the picture's pixel its
// start falls in, where a colour fill's edge is rounded down to (this path puts an edge half a pixel later, and the
// fill would show outside the border). Only for a picture larger than the game's size: smaller, the strips could fall
// between its rows.
#define RD3_UI_LINES 64
// the coordinate whose edge falls on the start of the picture's pixel v falls in (a little over: v is the engine's)
internal float rd3_ui_snap(float v, float s) { return (floorf(v * s + 0.01f) - 0.5f) / s; }
internal int rd3_ui_line(const rd_device *d, D3DPRIMITIVETYPE type, const rd_vb *vb) {
    const rd_surface *t = d->target;
    return type == D3DPT_LINELIST && t && t->sx > 1 && t->sy > 1 && vb->stride == 36
        && fabsf(d->proj._11 * (float)t->desc.dwWidth - 2) < 0.001f && fabsf(d->proj._22 * (float)t->desc.dwHeight + 2) < 0.001f;
}
internal void rd3_append_ui_lines(rd_device *d, rd_vb *vb, DWORD start, DWORD nv) {
    static uint8_t v[RD3_UI_LINES * 4 * 36];   // under the renderer's lock
    static WORD idx[RD3_UI_LINES * 6];
    DWORD cull = d->rs[D3DRENDERSTATE_CULLMODE];
    float scale[2] = { d->target->sx, d->target->sy };
    for (DWORD at = 0; at + 2 <= nv; ) {
        DWORD lines = (nv - at) / 2 < RD3_UI_LINES ? (nv - at) / 2 : RD3_UI_LINES;
        for (DWORD k = 0; k < lines; k++) {
            const uint8_t *a = vb->mem + (size_t)(start + at + k * 2) * 36;
            uint8_t *q = v + (size_t)k * 4 * 36;
            memcpy(q, a, 36); memcpy(q + 36, a + 36, 36); memcpy(q + 72, a, 36); memcpy(q + 108, a + 36, 36);
            float *p0 = (float *)q, *p1 = (float *)(q + 36), *p2 = (float *)(q + 72), *p3 = (float *)(q + 108);
            p0[0] -= 0.001f, p0[1] -= 0.001f, p1[0] -= 0.001f, p1[1] -= 0.001f;   // the engine's nudge into the pixel
            p2[0] = p0[0], p2[1] = p0[1], p3[0] = p1[0], p3[1] = p1[1];
            float dx = p1[0] - p0[0], dy = p1[1] - p0[1];
            int wide = fabsf(dx) >= fabsf(dy), along = wide ? 0 : 1, across = 1 - along;
            p2[across] += 1, p3[across] += 1;
            p0[across] = rd3_ui_snap(p0[across], scale[across]), p1[across] = rd3_ui_snap(p1[across], scale[across]);
            if (dx != 0 || dy != 0) {
                float *back = p0[along] <= p1[along] ? p0 : p1, *back2 = back == p0 ? p2 : p3;
                back[along] = back2[along] = rd3_ui_snap(back[along], scale[along]);
            }
            // the screen's y goes down: positive is clockwise, the side Direct3D's default keeps
            float turn = wide ? dx : -dy;
            int flip = cull == D3DCULL_CW ? turn > 0 : turn < 0;
            WORD b = (WORD)(k * 4), *o = idx + k * 6;
            o[0] = b; o[1] = (WORD)(b + (flip ? 2 : 1)); o[2] = (WORD)(b + (flip ? 1 : 2));
            o[3] = (WORD)(b + 2); o[4] = (WORD)(b + (flip ? 3 : 1)); o[5] = (WORD)(b + (flip ? 1 : 3));
        }
        rd_vb quads = { NULL, 0, vb->caps, vb->fvf, lines * 4, 36, lines * 4, v };
        rd3_append(d, D3DPT_TRIANGLELIST, &quads, 0, lines * 4, idx, lines * 6);
        at += lines * 2;
    }
}
// a blended draw into the batch (render_morph.c): its entry - with the runs' places and the fraction - and its
// triangles' indices to the entry and the vertex in the run
internal void rd3_append_morph(rd_device *d, const mo_run *a, const mo_run *b, float f, const WORD *idx, DWORD ni) {
    sil_batch();
    if (g_rb.ni && (g_rb.morph != 1 || g_rb.ni + (int)ni > RB_INDICES || g_rb.nw >= RB_WORLDS)) rd3_flush();
    uint32_t s = (uint32_t)g_rb.nw++;
    rb_world *e = &g_rb.w[s];
    rd3_entry(d, e);
    e->mo[0] = (float)a->at; e->mo[1] = (float)b->at; e->mo[2] = f;
    g_rb.morph = 1;
    g_rb.topo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    uint32_t *o = g_rb.i + g_rb.ni, top = 0x80000000u | s << 12;
    for (DWORD k = 0; k < ni; k++) *o++ = top | (idx[k] & 0xFFF);
    g_rb.ni += (int)ni;
    g_mo.draws++;
}

#include "render_instance.c"  // the units drawn as instances ([display] unit_instancing)
#include "render_overlay.c"   // the debug overlay: the AI's plans on the map
#include "render_record.c"    // the pipe's record: frames one after another, to find a flicker

internal HRESULT __stdcall rd_dev_qi(rd_device *d, const GUID *iid, void **out) { (void)d; (void)iid; *out = NULL; return E_NOINTERFACE; }
internal ULONG __stdcall rd_dev_addref(rd_device *d) { return (ULONG)InterlockedIncrement(&d->refs); }
internal ULONG __stdcall rd_dev_release(rd_device *d) {
    LONG n = InterlockedDecrement(&d->refs);
    if (n > 0) return (ULONG)n;
    if (d->target) rd_sf_release(d->target);
    if (g_rd3.prep_dev == d) g_rd3.prep_dev = NULL;   // the ground from the map draws on it (a new display: the editor's test play)
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
internal LONGLONG g_rwl_read_at, g_rwl_read_qpc;   // the render thread's read holds (the frame's time the simulation waits)
internal LONG g_rwl_reads;
internal uintptr_t g_rwl_read_ret;   // where the game took it
internal int g_rwl_read_depth;
internal rd_rwl_fn g_rwl_runlock;
internal int g_rwl_pass;   // a frame during a step: the world's lock passed over (rd_hook_graphics_update)
internal volatile LONG g_rwl_writers, g_rwl_claim;   // the frame's decision against the writers (rd_hook_write_lock)
internal void __thiscall rd_hook_read_unlock(void *lock) {
    if (g_rwl_pass && lock == g_rwl_world && GetCurrentThreadId() == g_rd_thread) return;
    if (GetCurrentThreadId() == g_rd_thread && g_rwl_read_depth > 0 && --g_rwl_read_depth == 0)
        g_rwl_read_qpc += rd_qpc() - g_rwl_read_at;
    g_rwl_runlock(lock);
}
internal void __thiscall rd_hook_read_lock(void *lock) {
    if (GetCurrentThreadId() != g_rd_thread) { g_rwl_read(lock); return; }
    if (g_rwl_pass && lock == g_rwl_world) return;
    LONGLONG t0 = rd_qpc();
    g_rwl_read(lock);
    if (lock == g_rwl_world) g_rwl_claim = 0;   // the frame's read is in (the writers' wait, below)
    g_rwl_read_ret = (uintptr_t)__builtin_return_address(0);
    LONGLONG t1 = rd_qpc();
    if (g_rwl_read_depth++ == 0) g_rwl_read_at = t1;
    g_rwl_reads++;
    LONGLONG w = t1 - t0;
    g_rwl_wait_qpc += w;
    if (w >= rd_qpc_ms(RWL_LONG_MS)) g_rwl_waits_long++;
    if (w > g_rwl_wait_max) g_rwl_wait_max = w;
    if (w > 0 && !g_rwl_world) g_rwl_world = lock;
}
// The hand-off (the frame during a step, below): a frame that found a step under way leaves g_rwl_want, and the
// simulation's next write lock waits for the update that reads the step's result (g_rwl_taken, set after it) - at
// most RWL_HANDOFF_MS, once (a picture that stopped, minimized, holds nothing up)
#define RWL_HANDOFF_MS 50
// ... and only once the picture has gone g_rwl_handoff_after ms ([display] step_handoff_ms) without an update: asked
// after every step a frame came during, it held the simulation 1000-1500 times a minute in the player's 450-unit
// game (2026-11-07, their log: 4.5-10.8 s of every minute, 8-18% of its time - the frames come every few ms, nearly
// every step had one) - the frames between the steps update by themselves unless the steps come back to back
internal int g_rwl_handoff_after = 50;
internal LONGLONG g_rwl_update_at;   // the last frame's update with the world (rd_hook_graphics_update)
internal float g_rwl_update_cam[16];   // ... its camera
internal volatile LONG g_rwl_want;
internal HANDLE g_rwl_taken;
internal LONG g_rwl_handoffs;
internal LONGLONG g_rwl_handoff_qpc;   // the simulation's time waiting for them
// The frame's decision (rd_hook_graphics_update) against the writers: it chose to read the world when no write was held
// - and the step's write lock came between that and its read, which then waited for the whole step (the player, a step
// of 1 s: "the mouse sidescroll lags for one second once the sim takes control again"). Now a writer counts itself
// before it takes the lock (g_rwl_writers: the threads in or waiting for it, the frame's own not) and a frame claims its
// read before it looks (g_rwl_claim, till its read lock is in): one of the two sees the other - the frame is drawn as
// during a step, or the writer waits for its read (a few ms; RWL_CLAIM_MS at most).
#define RWL_CLAIM_MS 100
internal int g_rwl_counted;   // the holder of the write lock counted itself (only the holder writes it)
// what the other threads (the simulation, the main thread's input) wait for the world's write lock - a frame's read
// in, its claim - the hand-off not: the minute's line (what the picture costs the simulation besides the hand-off)
internal LONGLONG g_rwl_wwait_qpc;
internal LONG g_rwl_wlocks;
// The units' update during a step (collect.c collect_during_begin) reads the map and the game's data beside the
// simulation's step: every other writer - which may free them, a load - counts itself here and waits for such an update
// to end, and the update doesn't begin while one is counted (from before its wait to its unlock)
internal volatile LONG g_rwl_nonstep, g_uc_in_during;
internal int g_rwl_nonstep_held;   // the holder of the write lock is one (only the holder writes it)
internal int g_rwl_during_handoff; // while such updates run: the picture goes this long (ms) without a full update before a hand-off (0: they don't run)
internal void __thiscall rd_hook_write_lock(void *lock) {
    int other = lock == g_rwl_world && GetCurrentThreadId() != g_rd_thread;
    int nonstep = other && (uintptr_t)__builtin_return_address(0) != (uintptr_t)engine_display_step_write_return_at();
    if (nonstep) {
        InterlockedIncrement(&g_rwl_nonstep);
        for (DWORD t0 = GetTickCount(); (g_uc_in_during || g_rwl_pass) && GetTickCount() - t0 < RWL_CLAIM_MS;) Sleep(0);
    }
    if (other && g_rwl_want && g_rwl_taken) {
        LONGLONG t0 = rd_qpc();
        WaitForSingleObject(g_rwl_taken, RWL_HANDOFF_MS);
        g_rwl_want = 0;
        g_rwl_handoffs++;
        g_rwl_handoff_qpc += rd_qpc() - t0;
    }
    LONGLONG w0 = other ? rd_qpc() : 0;
    if (other) {
        InterlockedIncrement(&g_rwl_writers);
        for (DWORD t0 = GetTickCount(); g_rwl_claim && GetTickCount() - t0 < RWL_CLAIM_MS;) Sleep(0);
    }
    g_rwl_write(lock);
    if (other) { g_rwl_wwait_qpc += rd_qpc() - w0; g_rwl_wlocks++; }   // the holder alone writes them
    if (lock == g_rwl_world) {
        g_rwl_counted = other;
        g_rwl_nonstep_held = nonstep;
        if (!other) g_rwl_claim = 0;   // the frame's own (update_objects' write mode)
        g_rwl_write_at = rd_qpc();
        g_rwl_write_ret = (uintptr_t)__builtin_return_address(0);
        g_sim_thread = GetCurrentThreadId();
    }
}
internal void collect_copy_written(uintptr_t ret);   // collect_copy.c: the units' copy of the world at a write's end
internal void __thiscall rd_hook_write_unlock(void *lock) {
    if (lock == g_rwl_world && g_rwl_counted) collect_copy_written(g_rwl_write_ret);
    if (lock == g_rwl_world && g_rwl_counted) {
        g_rwl_counted = 0;
        InterlockedDecrement(&g_rwl_writers);
        if (g_rwl_nonstep_held) { g_rwl_nonstep_held = 0; InterlockedDecrement(&g_rwl_nonstep); }
    }
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
// THE FRAME DURING A STEP ([display] draw_during_steps; the pipe: picture steps on | off): the frame reads the world
// only in the render objects' update (EEGraphics.update_objects, under the read lock); draw_objects draws from what the
// last update left. While the simulation holds the lock for a step the update is skipped and the frame drawn from the
// last one's - the camera (moved on this thread: camera.c) and the cursor go on, the units stand till the step is done -
// where it waited for the step before (the camera stood, then jumped). Not before an update has run at all.
// Only the updates that read the world's units are skipped - the ground's (its patches and the units' visible region
// with them), the units', their shadows' and the effects' (it crashed reading a unit, run during a step); the game's update runs with the lock passed over, the interface's with it
// (UIManager::CopyData and RedrawAll under the interface's own lock: the messages and buttons over the 3D view were
// missing from such frames - they flickered while moving) and the water's animation. The ground's draw draws nothing
// without its update (the clear's sky blue where it was): the last frame's ground is drawn again (render_frame.c "the
// ground kept").
// A game whose simulation is behind runs its steps back to back, the lock free for a moment between them that no frame
// hit (10844 units at speed 2, the player 2026-11-06: "the 3d view doesnt change anymore" - every frame drawn during a
// step, minutes long): a frame drawn during a step hands off (g_rwl_want above) - the simulation waits before its next
// step until a frame has updated from this one's result, so every step's result is drawn and no frame waits for a step.
// The interface's lock (UIManager.lock): the update holds it around the world's read lock, and the game's main thread
// takes the world's write lock inside it for the player's input - a click, the mouse at the screen's edge - so during a
// step the main thread held it waiting for the step, and the frame waited for the main thread: the picture stood till
// the step's end (the player: "one mouse click and it freezes once the next simstep comes"; the threads' stacks: the
// frame in TSCriticalSection::Enter from EEGraphics.update_objects, the main thread in WriteLock). So the frame takes
// it first without waiting: while another thread holds it and a writer is in or waiting, the frame goes without the
// game's update at all (drawn as during a step, the interface's last updated draws again: render_frame.c g_ku - g_rwl_bare).
internal int g_rwl_free = 1;
internal LONG g_rwl_skips, g_rwl_updates, g_rwl_bare;
internal ee_render_object *g_ro_obj[RO_N];   // the render objects, as their draws are called
internal int rec_step(void);   // render_record.c: the pipe's record steps - every second frame drawn as during a step (1) or bare (2)
internal void __thiscall rd_hook_graphics_update(ee_graphics *g) {
    int free = g_rwl_updates && g_rwl_free && g_rwl_world, ui = -1;
    if (free)
        while ((ui = ui_lock_try()) == 0 && !g_rwl_writers) Sleep(0);   // its holder at the interface, not waiting
    if (free && ui != 0) InterlockedExchange(&g_rwl_claim, 1);
    // a writer that isn't the simulation's step (a load takes the interface apart under it): waited for, as the game does
    int held = free && (ui == 0 || (g_rwl_writers > 0 && g_rwl_nonstep <= 0));
    if (held) g_rwl_claim = 0;
    int rs = g_rwl_updates ? rec_step() : 0, skip = held || rs;
    g_rwl_bare_now = ui == 0 || rs == 2;
    int after = g_rwl_handoff_after;
    if (held && g_rwl_during_handoff > after && !memcmp(g_rwl_update_cam, camera_transformation_at(game_camera()), sizeof g_rwl_update_cam))
        after = g_rwl_during_handoff;   // the units come from the copy: only the ground, shadows and effects wait - not while the camera moves
    if (held && g_rwl_taken && rd_qpc() - g_rwl_update_at >= rd_qpc_ms(after)) {
        ResetEvent(g_rwl_taken);
        g_rwl_want = 1;
    }
    rf_ground_frame(skip);
    if (skip) g_rwl_skips++;
    else g_rwl_updates++;
    g_rwl_pass = skip;
    if (skip && g_rp_state == 3) g_rp_state = 2;   // the pipe's rprobe step
    if (!g_rwl_bare_now) ee_graphics_update_objects(g);
    else g_rwl_bare++;
    g_rwl_pass = 0;
    g_rwl_claim = 0;
    if (ui == 1) ui_lock_leave();
    if (!skip) {
        g_rwl_update_at = rd_qpc();
        memcpy(g_rwl_update_cam, camera_transformation_at(game_camera()), sizeof g_rwl_update_cam);
    }
    if (!skip && g_rwl_taken) SetEvent(g_rwl_taken);
}
internal void __thiscall rd_hook_ro_terrain_update(ee_ro_terrain *o) {
    if (g_rwl_pass) return;
    shadow_hills_editor();   // render_shadows.c: here, under the read lock - a new map's writer frees what it reads
    rf_pull_begin();   // a zoom out under way: the ground made for the camera pulled back (render_frame.c)
    ro_terrain_update(o);
    rf_pull_end();
}
internal void collect_print(void);   // collect.c: the pipe's drawcount hash
internal int collect_during_begin(void);   // collect.c: a frame during a step's units' update from the steps' copy
internal void collect_during_end(void);
internal void __thiscall rd_hook_ro_units_update(ee_ro_units *o) {
    if (g_rwl_pass) {
        if (collect_during_begin()) {
            ro_units_update(o);
            collect_during_end();
        }
        return;
    }
    ro_units_update(o);
    collect_print();
}
internal void __thiscall rd_hook_ro_shadows_update(ee_ro_shadows *o) { if (!g_rwl_pass) ro_shadows_update(o); }
internal void collect_fx_take(void), collect_fx_check(void);   // collect_copy.c: the effects' queues the simulation makes
internal void __thiscall rd_hook_ro_effects_update(ee_ro_effects *o) {
    if (g_rwl_pass) { collect_fx_take(); return; }
    ro_effects_update(o);
    collect_fx_check();
}
internal int rd_world_lock_install(void) {
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll"), exe = GetModuleHandleA(NULL);
    if (!lle) return 0;
    g_rwl_read = (rd_rwl_fn)(void *)GetProcAddress(lle, "?ReadLock@TSReadWriteLock@@QAEXXZ");
    g_rwl_runlock = (rd_rwl_fn)(void *)GetProcAddress(lle, "?ReadUnlock@TSReadWriteLock@@QAEXXZ");
    g_rwl_write = (rd_rwl_fn)(void *)GetProcAddress(lle, "?WriteLock@TSReadWriteLock@@QAEXXZ");
    g_rwl_wunlock = (rd_rwl_fn)(void *)GetProcAddress(lle, "?WriteUnlock@TSReadWriteLock@@QAEXXZ");
    if (!g_rwl_read || !g_rwl_runlock || !g_rwl_write || !g_rwl_wunlock) return 0;
    g_rwl_taken = CreateEventA(NULL, TRUE, FALSE, NULL);
    return patch_iat(exe, "Low-Level Engine.dll", "?ReadLock@TSReadWriteLock@@QAEXXZ", (void *)rd_hook_read_lock) +
           patch_iat(exe, "Low-Level Engine.dll", "?ReadUnlock@TSReadWriteLock@@QAEXXZ", (void *)rd_hook_read_unlock) +
           patch_iat(exe, "Low-Level Engine.dll", "?WriteLock@TSReadWriteLock@@QAEXXZ", (void *)rd_hook_write_lock) +
           patch_iat(exe, "Low-Level Engine.dll", "?WriteUnlock@TSReadWriteLock@@QAEXXZ", (void *)rd_hook_write_unlock) +
           patch_graphics_update(rd_hook_graphics_update) + patch_ro_terrain_update(rd_hook_ro_terrain_update) +
           patch_ro_units_update(rd_hook_ro_units_update) + patch_ro_shadows_update(rd_hook_ro_shadows_update) +
           patch_ro_effects_update(rd_hook_ro_effects_update) + patch_terrain_visible(rf_hook_terrain_visible);
}

// THE GAME'S DRAWING, BY ITS PARTS: RTSGraphics draws a frame through its render objects (spec engine_display.cs) -
// each one's draw timed for the minute's line, the game's own work in it and the renderer's under it together
internal LONGLONG g_ro_qpc[RO_N];
internal int g_rp_skip;       // bit per render object: its draw skipped (the pipe's rprobe skip - the picture without it)
// Each one's batch flushed at its end: a batch takes the render object current when it is flushed (rd3_flush, at the
// next draw with other states), so the ground's last went in with the units' first and the water's last with the
// interface's - left out of the kept ground and water, and the water's not moved with the camera during a step (drawn
// where the camera had been: pieces of water over the map's edge, squares of the seabed on the water - AoC, 2026-11-05)
#define RO_DRAW(i, call) do { g_ro_obj[i] = (ee_render_object *)o; if (g_rp_skip & 1 << (i)) return; int was_ = g_ro_cur; g_ro_cur = (i); \
    LONGLONG t_ = rd_qpc(); call; RD_LOCK(); rd3_flush(); RD_UNLOCK(); g_ro_qpc[i] += rd_qpc() - t_; g_ro_cur = was_; } while (0)
// during a step the last real frame's ground instead of the game's draw, which in Art of Conquest draws its last lists
// again (rprobe step: the same 73 draws) - the ground twice, its see-through layers twice as strong (render_frame.c
// rf_ground_again)
internal void __thiscall rd_hook_ro_terrain(ee_ro_terrain *o) {
    RO_DRAW(RO_TERRAIN, if (g_kg_skip && (g_kg.nb || rg_ready())) { rf_ground_again(&g_kg); rg_again(0, g_rwl_nonstep > 0); rf_ground_lines(); } else ro_terrain_draw(o));
}
// during a step the last real frame's fields (the game's blob shadows aren't drawn: render_shadows.c SH_BLOB)
internal void __thiscall rd_hook_ro_shadows(ee_ro_shadows *o) {
    RO_DRAW(RO_SHADOWS, if (g_kg_skip && g_ks.nb) rf_ground_again(&g_ks); else ro_shadows_draw(o));
}
internal void __thiscall rd_hook_ro_units(ee_ro_units *o) { RO_DRAW(RO_UNITS, ro_units_draw(o)); }
// during a step the last real frame's water, as its ground (render_frame.c rf_ground_again)
internal void __thiscall rd_hook_ro_water(ee_ro_water *o) {
    RO_DRAW(RO_WATER, if (g_kg_skip && (g_kw.nb || rg_ready())) { rf_ground_again(&g_kw); rg_again(1, g_rwl_nonstep > 0); } else ro_water_draw(o));
}
internal void __thiscall rd_hook_ro_ui(ee_ro_ui *o) { RO_DRAW(RO_UI, if (g_rwl_bare_now && g_ku.nb) rf_ground_again(&g_ku); else ro_ui_draw(o)); }
internal int rd_render_objects_install(void) {
    return patch_ro_terrain_draw(rd_hook_ro_terrain) + patch_ro_shadows_draw(rd_hook_ro_shadows) +
           patch_ro_units_draw(rd_hook_ro_units) + patch_ro_water_draw(rd_hook_ro_water) + patch_ro_ui_draw(rd_hook_ro_ui);
}

// THE PROBE (the pipe's rprobe): the next whole frame's draws logged, each with its render object, states, light,
// material, texture, world and projection matrices and its vertices' extent (from their least x, y, z) and flatness (the largest distance from the plane of its
// first triangle, in the model's own units) - what the game draws where, for the shadows' plan (docs/renderer.md)
internal volatile LONG g_rp_state;   // 1 asked, 2 logging this frame, 3 asked for a frame drawn during a step
internal void rd3_probe(rd_device *d, D3DPRIMITIVETYPE type, rd_vb *vb, DWORD start, DWORD nv, WORD *idx, DWORD ni) {
    if (g_rp_state != 2 || vb->fvf != 0x152) return;
    const uint8_t *m = vb->mem + start * vb->stride;
    float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f };
    for (DWORD i = 0; i < nv; i++) {
        const float *p = (const float *)(m + i * vb->stride);
        for (int k = 0; k < 3; k++) { if (p[k] < lo[k]) lo[k] = p[k]; if (p[k] > hi[k]) hi[k] = p[k]; }
    }
    float flat = -1;
    DWORD a = idx && ni >= 3 ? idx[0] : 0, b = idx && ni >= 3 ? idx[1] : 1, c = idx && ni >= 3 ? idx[2] : 2;
    if (nv >= 3 && a < nv && b < nv && c < nv) {
        const float *p0 = (const float *)(m + a * vb->stride), *p1 = (const float *)(m + b * vb->stride),
                    *p2 = (const float *)(m + c * vb->stride);
        float u[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] }, v[3] = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };
        float n[3] = { u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0] };
        float len = sqrtf(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        if (len > 1e-9f) {
            flat = 0;
            for (DWORD i = 0; i < nv; i++) {
                const float *p = (const float *)(m + i * vb->stride);
                float dd = fabsf((p[0] - p0[0]) * n[0] + (p[1] - p0[1]) * n[1] + (p[2] - p0[2]) * n[2]) / len;
                if (dd > flat) flat = dd;
            }
        }
    }
    // the heights in the world: through the draw's world and view and the units' camera back (the ground's and the
    // water's vertices are in the camera's space - render_shadows.c)
    float iv[16], wv[16], wm[16], zl = 1e30f, zh = -1e30f;
    rd3_rigid_inverse(iv, g_sh.view);
    rd3_mul(wv, &d->world._11, &d->view._11);
    rd3_mul(wm, wv, iv);
    for (DWORD i = 0; i < nv; i++) {
        const float *p = (const float *)(m + i * vb->stride);
        float z = p[0] * wm[2] + p[1] * wm[6] + p[2] * wm[10] + wm[14];
        if (z < zl) zl = z;
        if (z > zh) zh = z;
    }
    const float *n0 = (const float *)(m + 12);
    DWORD col0 = *(const DWORD *)(m + 24);
    const DWORD *rs = d->rs, *ts = d->tss[0];
    const D3DLIGHT7 *l = &d->light[0];
    const D3DMATRIX *w = &d->world;
    rd_surface *t = d->tex[0];
    log_locked("# rprobe %s: prim %d verts %lu idx %lu | lit %lu amb %08lx light0 on %d type %d dir %.2f %.2f %.2f diff %.2f %.2f %.2f"
               " amb %.2f %.2f %.2f | mat d %.2f %.2f %.2f %.2f a %.2f %.2f %.2f src %lu %lu | blend %lu %lu-%lu z %lu/%lu"
               " atest %lu/%lu cull %lu | cop %lu %lu %lu aop %lu %lu %lu tf %08lx | tex %p %lux%lu | ext %.2f %.2f %.2f from %.2f %.2f %.5f flat %.3f"
               " n0 %.2f %.2f %.2f col0 %08lx | world %.2f %.2f %.2f / %.2f %.2f %.2f / %.2f %.2f %.2f / %.1f %.1f %.2f"
               " | viewport %lu %lu %lux%lu | view at %.1f %.1f %.1f | world z %.2f - %.2f | proj %.4g %.4g %.4g %.4g / %.4g %.4g %.4g %.4g"
               " / %.4g %.4g %.4g %.4g / %.4g %.4g %.4g %.4g\n",
               g_ro_cur >= 0 ? k_ro_names[g_ro_cur] : "other", (int)type, (unsigned long)nv, (unsigned long)ni,
               (unsigned long)rs[D3DRENDERSTATE_LIGHTING], (unsigned long)rs[D3DRENDERSTATE_AMBIENT], d->light_on[0],
               (int)l->dltType, l->dvDirection.x, l->dvDirection.y, l->dvDirection.z, l->dcvDiffuse.r, l->dcvDiffuse.g,
               l->dcvDiffuse.b, l->dcvAmbient.r, l->dcvAmbient.g, l->dcvAmbient.b, d->material.diffuse.r,
               d->material.diffuse.g, d->material.diffuse.b, d->material.diffuse.a, d->material.ambient.r,
               d->material.ambient.g, d->material.ambient.b, (unsigned long)rs[D3DRENDERSTATE_DIFFUSEMATERIALSOURCE],
               (unsigned long)rs[D3DRENDERSTATE_AMBIENTMATERIALSOURCE], (unsigned long)rs[D3DRENDERSTATE_ALPHABLENDENABLE],
               (unsigned long)rs[D3DRENDERSTATE_SRCBLEND], (unsigned long)rs[D3DRENDERSTATE_DESTBLEND],
               (unsigned long)rs[D3DRENDERSTATE_ZENABLE], (unsigned long)rs[D3DRENDERSTATE_ZWRITEENABLE],
               (unsigned long)rs[D3DRENDERSTATE_ALPHATESTENABLE], (unsigned long)rs[D3DRENDERSTATE_ALPHAREF],
               (unsigned long)rs[D3DRENDERSTATE_CULLMODE], (unsigned long)ts[D3DTSS_COLOROP],
               (unsigned long)ts[D3DTSS_COLORARG1], (unsigned long)ts[D3DTSS_COLORARG2], (unsigned long)ts[D3DTSS_ALPHAOP],
               (unsigned long)ts[D3DTSS_ALPHAARG1], (unsigned long)ts[D3DTSS_ALPHAARG2],
               (unsigned long)rs[D3DRENDERSTATE_TEXTUREFACTOR], (void *)t, t ? (unsigned long)t->desc.dwWidth : 0UL,
               t ? (unsigned long)t->desc.dwHeight : 0UL, hi[0] - lo[0], hi[1] - lo[1], hi[2] - lo[2], lo[0], lo[1], lo[2], flat, n0[0], n0[1],
               n0[2], (unsigned long)col0, w->_11, w->_12, w->_13, w->_21, w->_22, w->_23, w->_31, w->_32, w->_33, w->_41,
               w->_42, w->_43, (unsigned long)d->viewport.dwX, (unsigned long)d->viewport.dwY,
               (unsigned long)d->viewport.dwWidth, (unsigned long)d->viewport.dwHeight, d->view._41, d->view._42,
               d->view._43, zl, zh, d->proj._11, d->proj._12, d->proj._13, d->proj._14, d->proj._21, d->proj._22,
               d->proj._23, d->proj._24, d->proj._31, d->proj._32, d->proj._33, d->proj._34, d->proj._41, d->proj._42,
               d->proj._43, d->proj._44);
}

// rprobe [step | skip <parts | none>]: the own renderer's next frame (step: the next drawn during a step) logged draw
// by draw ("# rprobe", render_3d.c rd3_probe);
// skip leaves render objects out of the picture (terrain, shadows, units, water, interface - a comma list) until none
internal void cmd_rprobe(char *out, const char *args) {
    while (*args == ' ') args++;
    if (!strncmp(args, "skip", 4)) {
        int skip = 0;
        for (int i = 0; i < RO_N; i++)
            if (strstr(args + 4, k_ro_names[i])) skip |= 1 << i;
        g_rp_skip = skip;
        REPLY("rprobe: skipping");
        for (int i = 0; i < RO_N; i++) if (skip & 1 << i) REPLY(" %s", k_ro_names[i]);
        REPLY("%s\n", skip ? "" : " none");
        return;
    }
    g_rp_state = strncmp(args, "step", 4) ? 1 : 3;
    REPLY("rprobe: the next frame's%s draws go to the log (# rprobe)\n", g_rp_state == 3 ? " drawn during a step" : "");
}

internal void rd_frame_tick(void) {
    if (g_rp_state == 2) { g_rp_state = 0; log_locked("# rprobe: frame end\n"); }
    else if (g_rp_state == 1) g_rp_state = 2;
    g_rd_thread = GetCurrentThreadId();
    InterlockedIncrement(&g_rd_frames);
    DWORD now = GetTickCount();
    if (now - g_rd_minute_tick < 60000) return;
    static LONG frames0, draws0, batches0, replays0, forced0, wraps0;
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
    static LONG mo_draws0, mo_up0, mo_copied0, why0[4];
    log_locked("# render: the units' animation on the graphics card %s: %.0f draws a frame blended there, %.1f runs put into"
               " its pool a frame (%u vertices in it, started over %ld times), %.1f copies a frame made on the CPU after all"
               " (in the minute: not the GPU's %ld, not the draw's %ld, a card %ld, the probe %ld)\n",
               !g_mo.on ? "off" : g_mo_smooth ? "on" : "NOT hooked", fr > 0 ? (double)(g_mo.draws - mo_draws0) / fr : 0,
               fr > 0 ? (double)(g_mo.uploads - mo_up0) / fr : 0, g_mo.used, (long)g_mo.resets,
               fr > 0 ? (double)(g_mo.copied - mo_copied0) / fr : 0, (long)(g_mo.why[0] - why0[0]),
               (long)(g_mo.why[1] - why0[1]), (long)(g_mo.why[2] - why0[2]), (long)(g_mo.why[3] - why0[3]));
    mo_draws0 = g_mo.draws; mo_up0 = g_mo.uploads; mo_copied0 = g_mo.copied;
    memcpy(why0, g_mo.why, sizeof why0);
    log_locked("# render: the frame's list drawn %.2f times a frame, %.2f of them inside the scene (a texture, a read back,"
               " the 2D path), the ring full %ld times\n", fr > 0 ? (double)(g_rf.replays - replays0) / fr : 0,
               fr > 0 ? (double)(g_rf.forced - forced0) / fr : 0, (long)(g_rf.wraps - wraps0));
    replays0 = g_rf.replays; forced0 = g_rf.forced; wraps0 = g_rf.wraps;
    static LONGLONG ro0[RO_N];
    double ro[RO_N];
    for (int i = 0; i < RO_N; i++) { ro[i] = (double)(g_ro_qpc[i] - ro0[i]) * k; ro0[i] = g_ro_qpc[i]; }
    log_locked("# render: the game's drawing a frame (ms, the renderer's draws inside): terrain %.2f, shadows %.2f, units %.2f,"
               " water %.2f, interface %.2f\n", ro[RO_TERRAIN], ro[RO_SHADOWS], ro[RO_UNITS], ro[RO_WATER], ro[RO_UI]);
    // the frame's time by its parts: what none of them holds is the game's own work around them (its loop, the
    // interface's update, the sound ...) or a wait of the thread's; the reads back from the GPU (a lock of the target -
    // the GDI text of the interface -, wherever they are) wait for it to finish the frame so far
    static LONGLONG rf0, rh0, rr0;
    static LONG rn0;
    double rsum = 0, fms = fr > 0 ? secs * 1000.0 / fr : 0, lst = (double)(g_rf.qpc - rf0) * k,
           hld = (double)(g_rwl_read_qpc - rh0) * k, prs = (double)(g_d3d_present_qpc - present0) * k;
    for (int i = 0; i < RO_N; i++) rsum += ro[i];
    if (secs > 0)
        log_locked("# render: a frame %.2f ms: the render objects' draws %.2f, the update's read holds of the world %.2f, the"
                   " lists to the GPU %.2f, Present %.2f, the rest (the game's own around them, waits) %.2f; reads back from the"
                   " GPU %.2f a frame, %.2f ms\n", fms, rsum, hld, lst, prs, fms - rsum - hld - lst - prs,
                   fr > 0 ? (double)(g_rd_reads - rn0) / fr : 0, (double)(g_rd_read_qpc - rr0) * k);
    rf0 = g_rf.qpc; rh0 = g_rwl_read_qpc; rr0 = g_rd_read_qpc; rn0 = g_rd_reads;
    log_locked("# render: draws of more than the display DLL's %u vertices (terrain, shadows, units, water, interface, none) %ld %ld"
               " %ld %ld %ld %ld, the most %lu; past our room, dropped %ld\n", display_vb_vertices(), (long)g_rd_vb_over[RO_TERRAIN],
               (long)g_rd_vb_over[RO_SHADOWS], (long)g_rd_vb_over[RO_UNITS], (long)g_rd_vb_over[RO_WATER],
               (long)g_rd_vb_over[RO_UI], (long)g_rd_vb_over[RO_N], (unsigned long)g_rd_vb_most, (long)g_rd_vb_dropped);
    log_locked("# render: the ground's polygons, the most in a region: the ground %lu, the water %lu (the mesh's own room %u and"
               " %u; the mod's pools %s)\n", g_gm_polys_max[0], g_gm_polys_max[1], terrain_ground_polygons_room(),
               terrain_water_polygons_room(), g_gm_pools == 2 ? "on" : "OFF - no ground margin");
    g_gm_polys_max[0] = g_gm_polys_max[1] = 0;
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
    static LONG reads0, skips0, handoffs0, bare0, wlocks0;
    static LONGLONG read0, handoff0, wwait0;
    log_locked("# render: the frame's read holds of the world %.2f ms a frame (%.2f holds; the game took the last at %08x);"
               " frames drawn during a step %ld (draw_during_steps %s; %ld without the game's update - the interface's lock"
               " held by a thread waiting to write), the simulation waiting for their update %ld times, %.0f ms; the other"
               " threads' write locks %ld, %.0f ms waiting for them\n",
               (double)(g_rwl_read_qpc - read0) * k, fr > 0 ? (double)(g_rwl_reads - reads0) / fr : 0,
               (unsigned)g_rwl_read_ret, (long)(g_rwl_skips - skips0), g_rwl_free ? "on" : "off", (long)(g_rwl_bare - bare0),
               (long)(g_rwl_handoffs - handoffs0), (double)(g_rwl_handoff_qpc - handoff0) * ms,
               (long)(g_rwl_wlocks - wlocks0), (double)(g_rwl_wwait_qpc - wwait0) * ms);
    reads0 = g_rwl_reads; read0 = g_rwl_read_qpc; skips0 = g_rwl_skips; bare0 = g_rwl_bare;
    handoffs0 = g_rwl_handoffs; handoff0 = g_rwl_handoff_qpc; wlocks0 = g_rwl_wlocks; wwait0 = g_rwl_wwait_qpc;
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
    if (!g_kg_skip) rf_camera_axes();
    rd3_overlay_frame_end(d);
    rf_scene_end();
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
    if (g_rp_state == 2)   // the probe: the frame's clears too
        log_locked("# rprobe clear (%s): flags %lx, z %.3f, %lu rects, first %ld %ld - %ld %ld, viewport %lu %lu %lux%lu\n",
                   g_ro_cur >= 0 ? k_ro_names[g_ro_cur] : "other", (unsigned long)flags, (double)z, (unsigned long)n,
                   n && rects ? (long)rects[0].x1 : -1L, n && rects ? (long)rects[0].y1 : -1L,
                   n && rects ? (long)rects[0].x2 : -1L, n && rects ? (long)rects[0].y2 : -1L,
                   (unsigned long)d->viewport.dwX, (unsigned long)d->viewport.dwY, (unsigned long)d->viewport.dwWidth,
                   (unsigned long)d->viewport.dwHeight);
    RD_LOCK();
    rd3_submit();
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
internal HRESULT __stdcall rd_dev_set_material(rd_device *d, D3DMATERIAL7 *m) {   // the draw's own, as the world (rb_world)
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
    if (!n || !rd_vb_fits(vb, start, n) || !rd3_drawable(vb, type)) return D3D_OK;
    LONGLONG t0 = rd_qpc();
    RD_LOCK();
    rd3_overlay_before(d);
    rd3_probe(d, type, vb, start, n, NULL, 0);
    rd3_shadow_classify(d, vb, start, n);
    if (d->sh_role != SH_BLOB && rd3_prepare(d, rd3_flat_picture(d, vb, start, n))) {
        if (rd3_ui_line(d, type, vb)) rd3_append_ui_lines(d, vb, start, n);
        else rd3_append(d, type, vb, start, n, NULL, 0);
    }
    if (d->sh_card) rd3_shadow_card(d, type, vb, start, n, NULL, 0);
    g_rd_draw_qpc += rd_qpc() - t0;
    RD_UNLOCK();
    return D3D_OK;
}
// the indices count from start (Direct3D 7 has no base vertex of its own)
internal HRESULT __stdcall rd_dev_draw_indexed_vb(rd_device *d, D3DPRIMITIVETYPE type, rd_vb *vb, DWORD start, DWORD nv,
                                                  WORD *idx, DWORD ni, DWORD flags) {
    (void)flags;
    InterlockedIncrement(&g_rd_draws);
    if (!nv || !ni || !idx || !rd_vb_fits(vb, start, nv) || !rd3_drawable(vb, type)) { mo_copy_now(); return D3D_OK; }
    LONGLONG t0 = rd_qpc();
    RD_LOCK();
    rd3_overlay_before(d);
    mo_run *ma = NULL, *mb = NULL;
    float box[6];
    int morph = mo_take(d, type, vb, start, nv, &ma, &mb, box);   // its vertices not in the buffer: the GPU blends them
    float f = g_mo_note.f;
    g_mo_box = morph ? box : NULL;
    rd3_probe(d, type, vb, start, nv, idx, ni);
    rd3_shadow_classify(d, vb, start, nv);
    g_mo_box = NULL;
    if (morph && d->sh_card) { g_mo.why[2]++; mo_copy_now(); }   // a tree's card is made of its vertices (the tree the GPU's)
    g_mo_note.dst = NULL;
    if (morph) { if (d->sh_role != SH_BLOB && rd3_prepare(d, 0)) rd3_append_morph(d, ma, mb, f, idx, ni); }
    else if (d->sh_role != SH_BLOB && rd3_prepare(d, rd3_flat_picture(d, vb, start, nv))) rd3_append(d, type, vb, start, nv, idx, ni);
    if (d->sh_card) rd3_shadow_card(d, type, vb, start, nv, idx, ni);
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
    b->room = b->n > RD_VB_ROOM ? b->n : RD_VB_ROOM;
    b->mem = calloc(b->room, b->stride ? b->stride : 4);
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
