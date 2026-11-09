// THE GROUND AS THE MAP HOLDS IT ([display] own_ground, the pipe's picture ground on | off; the player, 2026-11-09: "raw game
// map data straight to gpu"). The game makes the ground anew at every update for what the camera sees, in arrays of 101 x
// 101 tiles built into the engine and both display DLLs - that is the limit on how far the camera can go back, and what a
// frame during a step has to do without. Here the whole map lies on the GPU in chunks of RG_CHUNK tiles, made once from the
// mesh's own public functions (no offsets of its), each frame drawing the chunks in the view with the camera as it is now.
// What a tile is (GETerrainMesh::UpdateVisibleTerrain, the 2017 update's 0x1003CD4D): a quad between four points, its
// corners belonging to the four cells around them - D its own (x, y), B (x - 1, y), A (x - 1, y - 1), C (x, y - 1). Each
// cell has a texture set, and the texture is the set's at the quad's x, y. The lowest of the four textures by address is
// the base, drawn whole; each other is drawn over it, a corner's alpha 1 where that corner's cell has it (a cell outside
// the map, or of the same set, counts as the quad's own). Two triangles, A B D and A D C; along the map's diamond edges one.
// Materials are drawn by texture address, each its base triangles and then its blended ones.
// The colour of a point is the map's colour table's times the game's lights (DX7Rasterizer::Light), once; its brightness
// the fog of war, four cells' sight (GETerrainMesh::UpdateFogOfWar: at least 2 units 0.25 each, 1 unit 0.125) - a texture
// of the cells read by the vertex shader; the seabed's wobble (DX7Rasterizer::TransformToCamera: a point deeper than -0.5
// moves by sin(x y + phase) times the strength) the shader's too, so it goes on in a frame drawn during a step.
#define RG_CHUNK 32
#define RG_MATERIALS 1024
#define RG_LAYERS 4     // a quad's textures at most
typedef float (__thiscall *rg_z_fn)(const void *mesh, unsigned long x, unsigned long y);
typedef const float *(__thiscall *rg_colour_fn)(const void *mesh, unsigned long x, unsigned long y);
typedef unsigned long (__thiscall *rg_set_fn)(const void *mesh, unsigned long x, unsigned long y);
typedef void *(__thiscall *rg_registered_fn)(const void *mesh, unsigned long id);
typedef uint8_t (__thiscall *rg_valid_fn)(const void *mesh, unsigned long x, unsigned long y);
typedef uint8_t (__thiscall *rg_edge_fn)(const void *mesh, long x, long y);
typedef void *(__thiscall *rg_texture_fn)(void *set, unsigned long x, unsigned long y);
typedef void (__thiscall *rg_coord_fn)(void *set, unsigned long x, unsigned long y, float *uv);
typedef int (__thiscall *rg_draw_fn)(void *ras, void *mesh, unsigned screen, int flag);
typedef void (__thiscall *rg_bind_fn)(void *ras, void *tex, unsigned screen);
typedef void (__thiscall *rg_lit_fn)(void *ras, void *mesh, unsigned screen);
// the game's lights for the ground, as they are while its DX7Rasterizer::Light runs: the device's are off by the time it draws
typedef struct { float amb[3]; int n; float dir[8][3], col[8][3]; } rg_lights;
typedef struct { int m, first, n; } rg_range;
typedef struct {
    ID3D11Buffer *vb;
    rg_range *r;
    int nr;
    float lo[3], hi[3];
} rg_chunk;
internal struct {
    int on, api, built, dll, flag;
    // the game's 3D states for the ground, as the last frame that drew it had them
    int state_ok;
    ID3D11RenderTargetView *st_rtv;
    ID3D11DepthStencilView *st_dsv;
    void *st_blend, *st_depth, *st_raster, *st_samp;
    D3D11_VIEWPORT st_vport;
    rd3_vcb st_vc;
    rd3_pcb st_pc;
    float st_proj[16];
    int size, across;
    ee_map *map;
    void *mesh, *ras;
    unsigned screen;
    rg_chunk *c;
    void *mat[RG_MATERIALS];
    int nmat;
    ID3D11Texture2D *fow;
    ID3D11ShaderResourceView *fow_srv;
    uint8_t *fow_was, *fow_now;
    ID3D11Buffer *cb;
    double phase;
    LONGLONG at;
    long drawn, tris, frames;
    rg_lights lights, lights_built;
    int lit;
    rg_lit_fn light[2];
    rg_z_fn z;
    rg_colour_fn colour;
    const float *(__thiscall *normal)(const void *mesh, unsigned long x, unsigned long y);
    rg_set_fn set;
    rg_registered_fn reg;
    rg_valid_fn tile;
    rg_edge_fn left, right, top, bottom;
    rg_texture_fn tex;
    rg_coord_fn uv;
    rg_draw_fn draw[2];
    rg_bind_fn bind[2];
} g_rg;
typedef struct { void *tex; int layer; rb_vertex v[3]; } rg_tri;

#define RG_GET(field, name) (*(FARPROC *)&g_rg.field = GetProcAddress(lle, name), g_rg.field != NULL)
internal int rg_api(void) {
    if (g_rg.api) return g_rg.api > 0;
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
    int ok = lle != NULL;
    ok = ok && RG_GET(z, "?GetZ@GETerrainMesh@@QBEMKK@Z") && RG_GET(colour, "?GetVertexColor@GETerrainMesh@@QBEABVGEColor@@KK@Z")
        && RG_GET(normal, "?GetVertexNormal@GETerrainMesh@@QBEPAVGEVector@@KK@Z") && RG_GET(set, "?GetTextureSetID@GETerrainMesh@@QBEKKK@Z")
        && RG_GET(reg, "?GetRegisteredTextureSet@GETerrainMesh@@QBEPAVGETextureSet@@K@Z") && RG_GET(tile, "?GetIsValidTile@GETerrainMesh@@QBE_NKK@Z")
        && RG_GET(left, "?GetIsLeftEdgeTile@GETerrainMesh@@QBE_NJJ@Z") && RG_GET(right, "?GetIsRightEdgeTile@GETerrainMesh@@QBE_NJJ@Z")
        && RG_GET(top, "?GetIsTopEdgeTile@GETerrainMesh@@QBE_NJJ@Z") && RG_GET(bottom, "?GetIsBottomEdgeTile@GETerrainMesh@@QBE_NJJ@Z")
        && RG_GET(tex, "?GetTexture@GETextureSet@@QAEPAVGETexture@@KK@Z") && RG_GET(uv, "?GetTextureCoordinate@GETextureSet@@QAEXKKAAVGETextureCoordinate@@@Z");
    g_rg.api = ok ? 1 : -1;
    log_locked("# ground: the mesh's functions %s\n", ok ? "found" : "NOT found - the game's ground");
    return ok;
}
internal void rg_free(void) {
    if (g_rg.c) {
        for (int i = 0; i < g_rg.across * g_rg.across; i++) { D3D_RELEASE(g_rg.c[i].vb); free(g_rg.c[i].r); }
        free(g_rg.c);
        g_rg.c = NULL;
    }
    D3D_RELEASE(g_rg.fow_srv);
    D3D_RELEASE(g_rg.fow);
    free(g_rg.fow_was); free(g_rg.fow_now);
    g_rg.fow_was = g_rg.fow_now = NULL;
    g_rg.built = g_rg.nmat = 0;
}
// a point's colour for the vertex: the table's times the game's lights as they were for its Light (bytes B G R A, alpha the corner's)
internal uint32_t rg_light(unsigned long x, unsigned long y, int a) {
    const float *c = g_rg.colour(g_rg.mesh, x, y), *n = g_rg.normal(g_rg.mesh, x, y);
    float l[3] = { g_rg.lights.amb[0], g_rg.lights.amb[1], g_rg.lights.amb[2] };
    for (int i = 0; i < g_rg.lights.n; i++) {
        float k = n[0] * g_rg.lights.dir[i][0] + n[1] * g_rg.lights.dir[i][1] + n[2] * g_rg.lights.dir[i][2];
        if (k <= 0) continue;
        l[0] += g_rg.lights.col[i][0] * k; l[1] += g_rg.lights.col[i][1] * k; l[2] += g_rg.lights.col[i][2] * k;
    }
    uint32_t out = (uint32_t)a << 24;
    for (int i = 0; i < 3; i++) {
        float v = c[i] * (l[i] > 1.0f ? 1.0f : l[i]) * 255.0f + 0.5f;   // the light clamped before the colour, as Light does: else a warm stone washes out
        out |= (v <= 0 ? 0u : v >= 255 ? 255u : (uint32_t)v) << (16 - i * 8);
    }
    return out;
}
internal void rg_vertex(rb_vertex *o, unsigned long x, unsigned long y, const float *uv, int a) {
    float *f = (float *)o->fvf;
    f[0] = (float)x; f[1] = (float)y; f[2] = g_rg.z(g_rg.mesh, x, y);
    f[3] = f[4] = f[5] = 0;
    *(uint32_t *)(f + 6) = rg_light(x, y, a);
    f[7] = uv[0]; f[8] = uv[1];
    o->w = 0;
}
internal int rg_tri_order(const void *pa, const void *pb) {
    const rg_tri *a = pa, *b = pb;
    if (a->tex != b->tex) return (uintptr_t)a->tex < (uintptr_t)b->tex ? -1 : 1;
    return a->layer - b->layer;
}
internal int rg_material(void *tex) {
    static int last;
    if (last < g_rg.nmat && g_rg.mat[last] == tex) return last;
    for (int i = 0; i < g_rg.nmat; i++) if (g_rg.mat[i] == tex) return last = i;
    if (g_rg.nmat >= RG_MATERIALS) return -1;
    g_rg.mat[g_rg.nmat] = tex;
    return last = g_rg.nmat++;
}
// the texture of a cell for the quad at x, y: the quad's own for a cell outside the map or of the same set
internal void *rg_cell(long cx, long cy, unsigned long x, unsigned long y, unsigned long own, void *own_tex, void **set) {
    if (cx < 0 || cy < 0 || !g_rg.tile(g_rg.mesh, (unsigned long)cx, (unsigned long)cy)) return own_tex;
    unsigned long id = g_rg.set(g_rg.mesh, (unsigned long)cx, (unsigned long)cy);
    if (id == own) return own_tex;
    *set = g_rg.reg(g_rg.mesh, id);
    return g_rg.tex(*set, x, y);
}
internal int rg_quad(rg_tri *out, unsigned long x, unsigned long y) {
    unsigned long own = g_rg.set(g_rg.mesh, x, y);
    void *own_set = g_rg.reg(g_rg.mesh, own), *own_tex = g_rg.tex(own_set, x, y);
    void *sets[4] = { own_set, own_set, own_set, own_set };   // D B A C
    void *t[4];
    t[0] = own_tex;
    t[1] = rg_cell((long)x - 1, (long)y, x, y, own, own_tex, &sets[1]);
    t[2] = rg_cell((long)x - 1, (long)y - 1, x, y, own, own_tex, &sets[2]);
    t[3] = rg_cell((long)x, (long)y - 1, x, y, own, own_tex, &sets[3]);
    int base = 0;
    for (int i = 1; i < 4; i++) if ((uintptr_t)t[i] < (uintptr_t)t[base]) base = i;
    int left = g_rg.left(g_rg.mesh, (long)x, (long)y), right = g_rg.right(g_rg.mesh, (long)x, (long)y);
    int topq = g_rg.top(g_rg.mesh, (long)x, (long)y), bottom = g_rg.bottom(g_rg.mesh, (long)x, (long)y);
    int n = 0;
    for (int k = -1; k < 4; k++) {   // the base first (-1), then each other texture once
        if (k >= 0 && (t[k] == t[base] || (k > 0 && t[k] == t[0]) || (k > 1 && t[k] == t[1]) || (k > 2 && t[k] == t[2]))) continue;
        void *tex = k < 0 ? t[base] : t[k], *set = k < 0 ? sets[base] : sets[k];
        float a[2], b[2], c[2], e[2];   // the texture's place at A, B, C, D
        float xy[2], x1y1[2];
        g_rg.uv(set, x, y, xy);
        g_rg.uv(set, x + 1, y + 1, x1y1);
        float u1 = x1y1[0] == 0 ? 1.0f : x1y1[0], v1 = x1y1[1] == 0 ? 1.0f : x1y1[1];
        a[0] = xy[0]; a[1] = xy[1];
        b[0] = xy[0]; b[1] = v1;
        c[0] = u1; c[1] = xy[1];
        e[0] = u1; e[1] = v1;
        // a corner's alpha: the whole base; a layer where the corner's cell has its texture
        int aa = k < 0 || t[2] == tex ? 255 : 0, ab = k < 0 || t[1] == tex ? 255 : 0, ac = k < 0 || t[3] == tex ? 255 : 0, ad = k < 0 || t[0] == tex ? 255 : 0;
        int layer = k < 0 ? 0 : 1;
        #define RG_V(i, X, Y, UV, AL) rg_vertex(&out[n].v[i], X, Y, UV, AL)
        #define RG_TRI(X0, Y0, U0, A0, X1, Y1, U1_, A1, X2, Y2, U2, A2) do { out[n].tex = tex; out[n].layer = layer; \
            RG_V(0, X0, Y0, U0, A0); RG_V(1, X1, Y1, U1_, A1); RG_V(2, X2, Y2, U2, A2); n++; } while (0)
        if (right) RG_TRI(x, y, a, aa, x, y + 1, b, ab, x + 1, y, c, ac);
        else if (left) RG_TRI(x, y + 1, b, ab, x + 1, y + 1, e, ad, x + 1, y, c, ac);
        else {
            if (!topq) RG_TRI(x, y, a, aa, x, y + 1, b, ab, x + 1, y + 1, e, ad);
            if (!bottom) RG_TRI(x, y, a, aa, x + 1, y + 1, e, ad, x + 1, y, c, ac);
        }
        #undef RG_TRI
        #undef RG_V
    }
    return n;
}
internal void rg_chunk_build(int cx, int cy, rg_tri *tris) {
    rg_chunk *c = &g_rg.c[cy * g_rg.across + cx];
    int n = 0;
    float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f };
    for (int y = cy * RG_CHUNK; y < (cy + 1) * RG_CHUNK && y < g_rg.size; y++)
        for (int x = cx * RG_CHUNK; x < (cx + 1) * RG_CHUNK && x < g_rg.size; x++)
            if (g_rg.tile(g_rg.mesh, (unsigned long)x, (unsigned long)y)) n += rg_quad(tris + n, (unsigned long)x, (unsigned long)y);
    if (!n) return;
    qsort(tris, (size_t)n, sizeof *tris, rg_tri_order);
    rb_vertex *v = malloc((size_t)n * 3 * sizeof *v);
    rg_range *r = malloc((size_t)n * sizeof *r);
    if (!v || !r) { free(v); free(r); return; }
    int nr = 0;
    for (int i = 0; i < n; i++) {
        memcpy(&v[i * 3], tris[i].v, sizeof tris[i].v);
        for (int k = 0; k < 3; k++) for (int a = 0; a < 3; a++) {
            float p = ((const float *)tris[i].v[k].fvf)[a];
            if (p < lo[a]) lo[a] = p;
            if (p > hi[a]) hi[a] = p;
        }
        int m = rg_material(tris[i].tex);
        if (m < 0) continue;
        if (nr && r[nr - 1].m == m && !(i && tris[i].layer != tris[i - 1].layer)) r[nr - 1].n += 3;
        else { r[nr].m = m; r[nr].first = i * 3; r[nr].n = 3; nr++; }
    }
    D3D11_BUFFER_DESC bd = { (UINT)n * 3 * sizeof *v, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA sd = { v, 0, 0 };
    if (SUCCEEDED(ID3D11Device_CreateBuffer(g_d3d.dev, &bd, &sd, &c->vb))) { c->r = r; c->nr = nr; g_rg.tris += n; }
    else free(r);
    free(v);
    memcpy(c->lo, lo, sizeof lo);
    memcpy(c->hi, hi, sizeof hi);
}
internal int rg_build(void) {
    ee_map *map = game_map();
    if (!map || !rg_api()) return 0;
    if (g_rg.built && g_rg.map == map && !memcmp(&g_rg.lights, &g_rg.lights_built, sizeof g_rg.lights)) return 1;
    rg_free();
    g_rg.map = map;
    g_rg.size = (int)map_get_size(map);
    g_rg.across = (g_rg.size + RG_CHUNK - 1) / RG_CHUNK;
    g_rg.tris = 0;
    g_rg.c = calloc((size_t)g_rg.across * (size_t)g_rg.across, sizeof *g_rg.c);
    rg_tri *tris = malloc(sizeof *tris * (size_t)RG_CHUNK * RG_CHUNK * 2 * RG_LAYERS);
    if (!g_rg.c || !tris) { free(tris); rg_free(); return 0; }
    LONGLONG t0 = rd_qpc();
    for (int cy = 0; cy < g_rg.across; cy++) for (int cx = 0; cx < g_rg.across; cx++) rg_chunk_build(cx, cy, tris);
    g_rg.lights_built = g_rg.lights;
    free(tris);
    D3D11_TEXTURE2D_DESC td = { (UINT)g_rg.size, (UINT)g_rg.size, 1, 1, DXGI_FORMAT_R8_UNORM, { 1, 0 }, D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_WRITE, 0 };
    g_rg.fow_was = calloc((size_t)g_rg.size, (size_t)g_rg.size + 1);
    g_rg.fow_now = calloc((size_t)g_rg.size, (size_t)g_rg.size);
    if (!g_rg.fow_was || !g_rg.fow_now || FAILED(ID3D11Device_CreateTexture2D(g_d3d.dev, &td, NULL, &g_rg.fow))
        || FAILED(ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)g_rg.fow, NULL, &g_rg.fow_srv))) { rg_free(); return 0; }
    if (!g_rg.cb) {
        D3D11_BUFFER_DESC bd = { 32, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
        if (FAILED(ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_rg.cb))) { rg_free(); return 0; }
    }
    g_rg.fow_was[(size_t)g_rg.size * g_rg.size] = 1;   // differs: the first frame sends it
    g_rg.built = 1;
    log_locked("# ground: the map's %d x %d tiles in %d chunks, %ld triangles, %d textures, made in %.1f ms\n", g_rg.size, g_rg.size,
               g_rg.across * g_rg.across, g_rg.tris, g_rg.nmat, (double)(rd_qpc() - t0) / (double)rd_qpc_ms(1));
    return 1;
}
// the cells' sight into the texture when it changed: 0, 1 or 2 and more as 0, a half, all
internal void rg_fow(void) {
    ee_map *map = g_rg.map;
    uint8_t *o = g_rg.fow_now;
    memset(o, 0, (size_t)g_rg.size * g_rg.size);
    for (int y = 0; y < g_rg.size; y++) {
        const int *row = map_view_row(map, (unsigned)y);
        int first = row[0], count = row[1];
        const uint8_t *p = (const uint8_t *)(uintptr_t)(unsigned)row[2];
        if (!p) continue;
        for (int k = 0; k < count; k++) {
            int x = first + k;
            if (x < 0 || x >= g_rg.size) continue;
            unsigned s = *(const uint16_t *)(p + (size_t)k * 4);   // as UpdateFogOfWar reads it: the number as stored
            o[y * g_rg.size + x] = s >= 2 ? 255 : s == 1 ? 128 : 0;
        }
    }
    int was = g_rg.fow_was[(size_t)g_rg.size * g_rg.size];
    if (!was && !memcmp(o, g_rg.fow_was, (size_t)g_rg.size * g_rg.size)) return;
    D3D11_MAPPED_SUBRESOURCE ms;
    if (FAILED(ID3D11DeviceContext_Map(g_d3d.ctx, (ID3D11Resource *)g_rg.fow, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) return;
    for (int y = 0; y < g_rg.size; y++) memcpy((uint8_t *)ms.pData + (size_t)y * ms.RowPitch, o + (size_t)y * g_rg.size, (size_t)g_rg.size);
    ID3D11DeviceContext_Unmap(g_d3d.ctx, (ID3D11Resource *)g_rg.fow, 0);
    memcpy(g_rg.fow_was, o, (size_t)g_rg.size * g_rg.size);
    g_rg.fow_was[(size_t)g_rg.size * g_rg.size] = 0;
}
// a chunk wholly outside one plane of the view (a little beyond it: the wobble, and the camera moved at the list's replay)
internal int rg_visible(const rg_chunk *c, const float *m) {
    int all[6] = { 1, 1, 1, 1, 1, 1 };
    for (int i = 0; i < 8; i++) {
        float p[3] = { i & 1 ? c->hi[0] + 2 : c->lo[0] - 2, i & 2 ? c->hi[1] + 2 : c->lo[1] - 2, i & 4 ? c->hi[2] + 1 : c->lo[2] - 1 };
        float cl[4];
        for (int a = 0; a < 4; a++) cl[a] = p[0] * m[a] + p[1] * m[4 + a] + p[2] * m[8 + a] + m[12 + a];
        float w = cl[3] * 1.2f + 1.0f;
        all[0] &= cl[0] < -w; all[1] &= cl[0] > w; all[2] &= cl[1] < -w; all[3] &= cl[1] > w; all[4] &= cl[3] < 0; all[5] &= cl[2] > w;
    }
    for (int i = 0; i < 6; i++) if (all[i]) return 0;
    return 1;
}
// no fog of war on the ground (the game's call of UpdateFogOfWar, 0x503A7F's caller): the player sees all, the free camera
internal int rg_revealed(void) {
    ee_player *p = game_players()[local_player_index()];
    return *game_free_camera_at() || (p && player_get_explore_all(p));
}
internal int rg_ready(void) { return g_rg.on && g_rg.lit && g_rg.api >= 0 && g_rd3.scene_view[15] == 1 && g_rd3.prep_dev && g_cam_now_ok && game_map(); }   // the units have drawn once: the projection, from the frame before
// the ground of this frame into the list, on the game's states for it
internal void rg_draw_now(int again) {
    rd_device *d = g_rd3.prep_dev;
    if (!g_rg.ras || !rg_build()) return;
    int i = g_rg.dll;
    rd3_flush();
    // the projection the game's draw would set: the viewport's (+0x1F0, a GETransformation by rows; +0x1B0 the fixed camera's)
    const float *gp = (const float *)(*(const uint8_t **)(*(const uintptr_t *)((const uint8_t *)g_rg.ras + 0x20) + g_rg.screen * 4) + (g_rg.flag ? 0x1B0 : 0x1F0));
    // (in a frame during a step the current viewport is the interface's: the projection of the last frame that drew the ground)
    float proj[16], vpm[16];
    if (again && g_rg.state_ok) memcpy(proj, g_rg.st_proj, sizeof proj);
    else for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) proj[r * 4 + c] = gp[c * 4 + r];
    rd3_mul(vpm, g_cam_view_now, proj);
    static ID3D11ShaderResourceView *srv[RG_MATERIALS];
    static uint8_t want[RG_MATERIALS];
    memset(want, 0, sizeof want);
    int nc = g_rg.across * g_rg.across;
    static uint8_t *vis;
    static int cap;
    if (cap < nc) { free(vis); vis = malloc((size_t)nc); cap = vis ? nc : 0; }
    if (!vis) return;
    for (int k = 0; k < nc; k++) {
        const rg_chunk *c = &g_rg.c[k];
        vis[k] = c->vb && rg_visible(c, vpm);
        if (vis[k]) for (int r = 0; r < c->nr; r++) want[c->r[r].m] = 1;
    }
    for (int m = 0; m < g_rg.nmat; m++) {
        srv[m] = NULL;
        if (!want[m]) continue;
        g_rg.bind[i](g_rg.ras, g_rg.mat[m], g_rg.screen);
        rd_surface *t = d->tex[0];
        if (!t) continue;
        srv[m] = rd_gpu_texture(t, 0);
        (t->top ? t->top : t)->rf_used = g_rd_epoch;
    }
    // a frame during a step: the game has set no 3D states for it (its draw doesn't run): those of the last frame that did
    if (again && g_rg.state_ok) {
        g_rd3.rtv = g_rg.st_rtv; g_rd3.dsv = g_rg.st_dsv; g_rd3.blend = g_rg.st_blend; g_rd3.depth = g_rg.st_depth;
        g_rd3.raster = g_rg.st_raster; g_rd3.samp = g_rg.st_samp; g_rd3.vport = g_rg.st_vport;
        g_rd3.vc = g_rg.st_vc; g_rd3.pc = g_rg.st_pc;
        g_rf.pc_new = 1;
    } else {
        d->changed = -1;
        if (!rd3_prepare(d, 0)) return;
    }
    memcpy(g_rd3.vc.vpm, vpm, sizeof vpm);
    memcpy(g_rd3.vc.view, g_cam_view_now, sizeof g_cam_view_now);
    g_rd3.vc.shp[0] = SH_GROUND;
    g_rf.vc_new = 1;
    if (!again) {
        g_rg.st_rtv = g_rd3.rtv; g_rg.st_dsv = g_rd3.dsv; g_rg.st_blend = g_rd3.blend; g_rg.st_depth = g_rd3.depth;
        g_rg.st_raster = g_rd3.raster; g_rg.st_samp = g_rd3.samp; g_rg.st_vport = g_rd3.vport;
        g_rg.st_vc = g_rd3.vc; g_rg.st_pc = g_rd3.pc;
        memcpy(g_rg.st_proj, proj, sizeof proj);
        g_rg.state_ok = 1;
    }
    g_rb.topo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    rg_fow();
    LONGLONG now = rd_qpc();
    const float *ws = (const float *)((const uint8_t *)g_rg.ras + 0x40);
    if (g_rg.at) g_rg.phase = fmod(g_rg.phase + (double)ws[0] * (double)(now - g_rg.at) / (double)rd_qpc_ms(1000), 6.283185307179586 * 1024.0);
    g_rg.at = now;
    float cb[8] = { 1, (float)g_rg.phase, ws[2], rg_revealed() ? 1.0f : 0.0f, 0, 0, 0, 0 };
    ID3D11DeviceContext_UpdateSubresource(g_d3d.ctx, (ID3D11Resource *)g_rg.cb, 0, NULL, cb, 0, 0);
    long drawn = 0;
    for (int k = 0; k < nc; k++) {
        if (!vis[k]) continue;
        const rg_chunk *c = &g_rg.c[k];
        for (int r = 0; r < c->nr; r++) {
            if (!srv[c->r[r].m]) continue;
            g_rd3.srv = srv[c->r[r].m];
            rf_record((UINT)c->r[r].first, (UINT)c->r[r].n, 0);
            if (g_rf.n) g_rf.cmd[g_rf.n - 1].gvb = c->vb;
            drawn++;
        }
    }
    g_rg.drawn = drawn;
    g_rg.frames++;
    // as the game's draw leaves it: the display's state "3D drawn" - Set2DMode puts the interface's matrices back only then
    (*(uint8_t **)(*(uintptr_t *)((uint8_t *)g_rg.ras + 0x6c) + g_rg.screen * 4))[0x6d] = 1;
    g_rd3.bound = 0;
    d->changed = -1;
}
// a list's ground draw (render_frame.c rf_draw): its own vertex buffer, the fog of war and the wobble for it
internal void rg_replay(ID3D11DeviceContext *x, const rf_cmd *c) {
    UINT st = sizeof(rb_vertex), off = 0;
    ID3D11Buffer *none = NULL;
    ID3D11ShaderResourceView *nos = NULL;
    ID3D11DeviceContext_IASetVertexBuffers(x, 0, 1, &c->gvb, &st, &off);
    ID3D11DeviceContext_VSSetConstantBuffers(x, 3, 1, &g_rg.cb);
    ID3D11DeviceContext_VSSetShaderResources(x, 7, 1, &g_rg.fow_srv);
    ID3D11DeviceContext_Draw(x, c->n, c->at);
    ID3D11DeviceContext_IASetVertexBuffers(x, 0, 1, &g_rd3.vb, &st, &off);
    ID3D11DeviceContext_VSSetConstantBuffers(x, 3, 1, &none);
    ID3D11DeviceContext_VSSetShaderResources(x, 7, 1, &nos);
}
internal int rg_hook(int i, void *ras, void *mesh, unsigned screen, int flag) {
    if (!rg_ready()) return g_rg.draw[i](ras, mesh, screen, flag);
    g_rg.ras = ras; g_rg.mesh = mesh; g_rg.screen = screen; g_rg.dll = i; g_rg.flag = flag & 0xFF;
    RD_LOCK(); rg_draw_now(0); RD_UNLOCK();
    return 0;
}
internal int __thiscall rg_hook0(void *ras, void *mesh, unsigned screen, int flag) { return rg_hook(0, ras, mesh, screen, flag); }
internal int __thiscall rg_hook1(void *ras, void *mesh, unsigned screen, int flag) { return rg_hook(1, ras, mesh, screen, flag); }
// a frame during a step: the game's draw doesn't run, the ground is drawn from the map all the same
internal int rg_again(void) {
    if (!rg_ready() || !g_rg.ras) return 0;
    RD_LOCK(); rg_draw_now(1); RD_UNLOCK();
    return 1;
}
internal void rg_lights_take(void *ras) {
    rd_device *d = g_rd3.prep_dev;
    if (!d) return;
    rg_lights l;
    memset(&l, 0, sizeof l);
    memcpy(l.amb, (const uint8_t *)ras + 0x10, sizeof l.amb);
    for (int i = 0; i < 8; i++) {
        const D3DLIGHT7 *lt = &d->light[i];
        if (!d->light_on[i] || lt->dltType != D3DLIGHT_DIRECTIONAL) continue;
        l.dir[l.n][0] = -lt->dvDirection.x; l.dir[l.n][1] = -lt->dvDirection.y; l.dir[l.n][2] = -lt->dvDirection.z;
        l.col[l.n][0] = lt->dcvDiffuse.r; l.col[l.n][1] = lt->dcvDiffuse.g; l.col[l.n][2] = lt->dcvDiffuse.b;
        l.n++;
    }
    g_rg.lights = l;
    g_rg.lit = 1;
}
internal void __thiscall rg_light_hook0(void *ras, void *mesh, unsigned screen) { rg_lights_take(ras); g_rg.light[0](ras, mesh, screen); }
internal void __thiscall rg_light_hook1(void *ras, void *mesh, unsigned screen) { rg_lights_take(ras); g_rg.light[1](ras, mesh, screen); }
internal int rg_display_loaded(HMODULE m, const char *name) {
    int i = strstr(name, "TnL") ? 1 : 0;
    void *fn = (void *)GetProcAddress(m, "?Draw@DX7Rasterizer@@UAEJAAVGETerrainMesh@@K_N@Z");
    *(FARPROC *)&g_rg.bind[i] = GetProcAddress(m, "?SetTexture@DX7Rasterizer@@UAEXPAVGETexture@@K@Z");
    if (!fn || !g_rg.bind[i]) return 0;
    g_rg.draw[i] = (rg_draw_fn)fn;
    void *li = (void *)GetProcAddress(m, "?Light@DX7Rasterizer@@UAEXAAVGETerrainMesh@@K@Z");
    if (!li) return 0;
    g_rg.light[i] = (rg_lit_fn)li;
    return (replace_rdata_ptr(m, li, i ? (void *)rg_light_hook1 : (void *)rg_light_hook0) > 0) + (replace_rdata_ptr(m, fn, i ? (void *)rg_hook1 : (void *)rg_hook0) > 0);
}
