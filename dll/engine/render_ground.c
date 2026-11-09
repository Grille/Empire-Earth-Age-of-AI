// THE GROUND AS THE MAP HOLDS IT (the own renderer's ground, no choice - the player, 2026-11-09: a fundamental part made
// optional makes everything after it twice the work; the pipe's picture ground on | off compares it with the game's until
// that goes; the player, 2026-11-09: "raw game map data straight to gpu"). The game makes the ground anew at every update for what the camera sees, in arrays of 101 x
// 101 tiles built into the engine and both display DLLs - that is the limit on how far the camera can go back, and what a
// frame during a step has to do without. Here the whole map lies on the GPU in chunks of RG_CHUNK tiles, made once from the
// mesh's own public functions (no offsets of its), each frame drawing the chunks in the view with the camera as it is now.
// What a tile is (GETerrainMesh::UpdateVisibleTerrain, the 2017 update's 0x1003CD4D): a quad between four points, its
// corners belonging to the four cells around them - D its own (x, y), B (x - 1, y), A (x - 1, y - 1), C (x, y - 1). Each
// cell has a texture set, and the texture is the set's at the quad's x, y. The lowest of the four textures by address is
// the base, drawn whole; each other is drawn over it, a corner's alpha 1 where that corner's cell has it (a cell outside
// the map, or of the same set, counts as the quad's own). Two triangles, A B D and A D C; along the map's diamond edges one.
// Materials are drawn by texture address, each its base triangles and then its blended ones.
// The colour of a point is the map's colour table's times the game's lights (DX7Rasterizer::Light) - the lights are the
// vertex shader's, they change in play and the chunks are made once; its brightness the fog of war, four cells' sight (GETerrainMesh::UpdateFogOfWar: at least 2 units 0.25 each, 1 unit 0.125) - a texture
// of the cells read by the vertex shader; the seabed's wobble (DX7Rasterizer::TransformToCamera: a point deeper than -0.5
// moves by sin(x y + phase) times the strength) the shader's too, so it goes on in a frame drawn during a step.
// The water's surface the same way (UpdateVisibleTerrain's end, DX7Rasterizer::DrawWater): a tile the map calls water is a quad
// at the level with one whole texture of the animation, the frame its place in a table of 33 x 33 says, half see-through and
// clear at a corner the map's bits say is shore; one colour (the map's for the water times the lights on level ground), the
// fog of war the ground's, and the movement sin(x y + phase) times the surface's strength - over land half of (sin - 1).
// Round the map the black the game puts there (Generate*EdgePolygons): from each point of the boundary out and down to the
// level within 5 tiles, then on to the far distance, flat; a wedge of the same at each of the four corners. The game makes it
// for the camera each update, as far as the view reaches; here once, as far as any view can.
#define RG_CHUNK 32
#define RG_MATERIALS 1024
#define RG_LAYERS 4     // a quad's textures at most
#define RG_SKIRT_NEAR 5.0f      // tiles out and down to the level, as the game's
#define RG_SKIRT_FAR 1000.0f    // and on from there, beyond any view (the game's corners too)
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
} rg_part;
typedef struct {
    rg_part p[2];        // the ground, the water
    int wn, wshore;      // the water's vertices, those over land (the water's level for the shadows)
} rg_chunk;
// the game's 3D states for the ground or the water, as the last frame that drew it had them
typedef struct {
    int ok;
    ID3D11RenderTargetView *rtv;
    ID3D11DepthStencilView *dsv;
    void *blend, *depth, *raster, *samp;
    D3D11_VIEWPORT vport;
    rd3_vcb vc;
    rd3_pcb pc;
    float proj[16];
} rg_states;
internal struct {
    int on, api, built, dll, flag;
    rg_states st[2];
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
    ID3D11Buffer *cb, *cbw, *skirt;
    int nskirt;
    uint32_t wcol;
    double phase;
    LONGLONG at;
    long drawn, tris, frames;
    rg_lights lights;
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
    rg_draw_fn draw[2], water[2];
    rg_bind_fn bind[2];
    uint8_t (__thiscall *is_space)(const void *mesh);   // Art of Conquest's only
    uint8_t (__thiscall *is_water)(const void *mesh, unsigned long x, unsigned long y);
    uint8_t (__thiscall *water_info)(const void *mesh, unsigned long x, unsigned long y);
    const float *(__thiscall *water_colour)(const void *mesh, unsigned long x, unsigned long y);
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
        && RG_GET(tex, "?GetTexture@GETextureSet@@QAEPAVGETexture@@KK@Z") && RG_GET(uv, "?GetTextureCoordinate@GETextureSet@@QAEXKKAAVGETextureCoordinate@@@Z")
        && RG_GET(is_water, "?GetIsWaterTile@GETerrainMesh@@QBE_NKK@Z") && RG_GET(water_info, "?GetWaterInfo@GETerrainMesh@@QBEEKK@Z")
        && RG_GET(water_colour, "?GetWaterColor@GETerrainMesh@@QBEABVGEColor@@KK@Z");
    if (ok) *(FARPROC *)&g_rg.is_space = GetProcAddress(lle, "?GetIsSpaceMap@GETerrainMesh@@QBE_NXZ");
    g_rg.api = ok ? 1 : -1;
    log_locked("# ground: the mesh's functions %s\n", ok ? "found" : "NOT found - the game's ground");
    return ok;
}
internal void rg_free(void) {
    if (g_rg.c) {
        for (int i = 0; i < g_rg.across * g_rg.across; i++) for (int w = 0; w < 2; w++) { D3D_RELEASE(g_rg.c[i].p[w].vb); free(g_rg.c[i].p[w].r); }
        free(g_rg.c);
        g_rg.c = NULL;
    }
    D3D_RELEASE(g_rg.skirt);
    g_rg.nskirt = 0;
    D3D_RELEASE(g_rg.fow_srv);
    D3D_RELEASE(g_rg.fow);
    free(g_rg.fow_was); free(g_rg.fow_now);
    g_rg.fow_was = g_rg.fow_now = NULL;
    g_rg.built = g_rg.nmat = 0;
}
// a colour of the game's for a vertex (bytes B G R, alpha the corner's)
internal uint32_t rg_bytes(const float *c, int a) {
    uint32_t out = (uint32_t)a << 24;
    for (int i = 0; i < 3; i++) {
        float v = c[i] * 255.0f + 0.5f;
        out |= (v <= 0 ? 0u : v >= 255 ? 255u : (uint32_t)v) << (16 - i * 8);
    }
    return out;
}
internal void rg_vertex(rb_vertex *o, unsigned long x, unsigned long y, const float *uv, int a) {
    float *f = (float *)o->fvf;
    f[0] = (float)x; f[1] = (float)y; f[2] = g_rg.z(g_rg.mesh, x, y);
    memcpy(f + 3, g_rg.normal(g_rg.mesh, x, y), 12);
    *(uint32_t *)(f + 6) = rg_bytes(g_rg.colour(g_rg.mesh, x, y), a);
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
internal void rg_water_vertex(rg_chunk *ch, rb_vertex *o, unsigned long x, unsigned long y, float u, float v, int a) {
    float *f = (float *)o->fvf;
    f[0] = (float)x; f[1] = (float)y; f[2] = g_rg.z(g_rg.mesh, x, y);   // the ground's height: the vertex shader's choice of the movement
    f[3] = f[4] = 0; f[5] = 1;   // level: the lights as DX7Rasterizer::Light's end has them for the water
    *(uint32_t *)(f + 6) = g_rg.wcol | (uint32_t)a << 24;
    f[7] = u; f[8] = v;
    o->w = 0;
    ch->wn++;
    if (f[2] >= 0) ch->wshore++;
}
// a water tile's triangles, as the ground's along the map's edges; the tile's frame of the animation in place of a texture
internal int rg_water_quad(rg_chunk *ch, rg_tri *out, unsigned long x, unsigned long y, unsigned groups) {
    if (!g_rg.is_water(g_rg.mesh, x, y)) return 0;
    unsigned m = mesh_water_patch(g_rg.mesh, (unsigned)x, (unsigned)y);
    if (m >= groups || m >= RG_MATERIALS) return 0;
    unsigned info = g_rg.water_info(g_rg.mesh, x, y);
    int aa = info & 2 ? 0 : 128, ab = info & 4 ? 0 : 128, ad = info & 8 ? 0 : 128, ac = info & 0x10 ? 0 : 128;
    int left = g_rg.left(g_rg.mesh, (long)x, (long)y), right = g_rg.right(g_rg.mesh, (long)x, (long)y);
    int topq = g_rg.top(g_rg.mesh, (long)x, (long)y), bottom = g_rg.bottom(g_rg.mesh, (long)x, (long)y);
    int n = 0;
    #define RG_A 0, x, y, 0.0f, 1.0f, aa
    #define RG_B 1, x, y + 1, 0.0f, 0.0f, ab
    #define RG_C 2, x + 1, y, 1.0f, 1.0f, ac
    #define RG_D 3, x + 1, y + 1, 1.0f, 0.0f, ad
    #define RG_W(k, i_, X, Y, U, V, A) rg_water_vertex(ch, &out[n].v[k], X, Y, U, V, A)
    #define RG_W2(k, ...) RG_W(k, __VA_ARGS__)
    #define RG_TRI(P0, P1, P2) do { out[n].tex = (void *)(uintptr_t)m; out[n].layer = 0; RG_W2(0, P0); RG_W2(1, P1); RG_W2(2, P2); n++; } while (0)
    if (right) RG_TRI(RG_A, RG_B, RG_C);
    else if (left) RG_TRI(RG_B, RG_D, RG_C);
    else {
        if (!topq) RG_TRI(RG_A, RG_B, RG_D);
        if (!bottom) RG_TRI(RG_A, RG_D, RG_C);
    }
    #undef RG_TRI
    #undef RG_W2
    #undef RG_W
    #undef RG_D
    #undef RG_C
    #undef RG_B
    #undef RG_A
    return n;
}
// the triangles of a chunk's part onto the GPU, by texture; the water's material is its frame's number
internal void rg_part_make(rg_part *c, rg_tri *tris, int n, int water) {
    float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f };
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
        int m = water ? (int)(uintptr_t)tris[i].tex : rg_material(tris[i].tex);
        if (m < 0) continue;
        if (nr && r[nr - 1].m == m && !(i && tris[i].layer != tris[i - 1].layer)) r[nr - 1].n += 3;
        else { r[nr].m = m; r[nr].first = i * 3; r[nr].n = 3; nr++; }
    }
    D3D11_BUFFER_DESC bd = { (UINT)n * 3 * sizeof *v, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA sd = { v, 0, 0 };
    if (SUCCEEDED(ID3D11Device_CreateBuffer(g_d3d.dev, &bd, &sd, &c->vb))) { c->r = r; c->nr = nr; g_rg.tris += n; }
    else free(r);
    free(v);
    if (water) lo[2] = hi[2] = 0;   // their z is the ground's under them, the surface lies at the level
    memcpy(c->lo, lo, sizeof lo);
    memcpy(c->hi, hi, sizeof hi);
}
internal void rg_chunk_build(int cx, int cy, rg_tri *tris) {
    rg_chunk *c = &g_rg.c[cy * g_rg.across + cx];
    unsigned groups = mesh_water_groups(g_rg.mesh);
    for (int w = 0; w < 2; w++) {
        int n = 0;
        for (int y = cy * RG_CHUNK; y < (cy + 1) * RG_CHUNK && y < g_rg.size; y++)
            for (int x = cx * RG_CHUNK; x < (cx + 1) * RG_CHUNK && x < g_rg.size; x++)
                if (g_rg.tile(g_rg.mesh, (unsigned long)x, (unsigned long)y))
                    n += w ? rg_water_quad(c, tris + n, (unsigned long)x, (unsigned long)y, groups) : rg_quad(tris + n, (unsigned long)x, (unsigned long)y);
        rg_part_make(&c->p[w], tris, n, w);
    }
}
// THE BLACK ROUND THE MAP. Its triangles both ways round: which way the game's face (four lists by hand) doesn't matter when both are there
internal void rg_black(rb_vertex *o, const float *p) {
    float *f = (float *)o->fvf;
    memset(f, 0, 36);
    f[0] = p[0]; f[1] = p[1]; f[2] = p[2];
    *(uint32_t *)(f + 6) = 0xFF000000u;
    o->w = 0;
}
internal int rg_black_tri(rb_vertex *v, int n, const float *a, const float *b, const float *c) {
    rg_black(&v[n], a); rg_black(&v[n + 1], b); rg_black(&v[n + 2], c);
    rg_black(&v[n + 3], a); rg_black(&v[n + 4], c); rg_black(&v[n + 5], b);
    return n + 6;
}
// a point, moved out along (dx, dy) by k tiles, at the level
internal void rg_out(float *o, const float *p, float dx, float dy, float k) { o[0] = p[0] + dx * k; o[1] = p[1] + dy * k; o[2] = 0; }
internal int rg_black_side(rb_vertex *v, int n, const float *p, const float *q, float dx, float dy) {
    float p1[3], q1[3], p2[3], q2[3];
    rg_out(p1, p, dx, dy, RG_SKIRT_NEAR); rg_out(q1, q, dx, dy, RG_SKIRT_NEAR);
    rg_out(p2, p, dx, dy, RG_SKIRT_FAR); rg_out(q2, q, dx, dy, RG_SKIRT_FAR);
    n = rg_black_tri(v, n, p, q, q1); n = rg_black_tri(v, n, p, q1, p1);
    n = rg_black_tri(v, n, p1, q1, q2);
    return rg_black_tri(v, n, p1, q2, p2);
}
internal void rg_skirt_build(void) {
    static const float dirs[4][2] = { { 1, -1 }, { 1, 1 }, { -1, 1 }, { -1, -1 } };   // the game's back, right, front, left
    size_t cap = (size_t)g_rg.size * 4 * 4 * 6 + 4 * 3 * 6;
    rb_vertex *v = malloc(cap * sizeof *v);
    if (!v) return;
    int n = 0, have = 0;
    float corner[4][3];   // the boundary's point furthest along +x, +y, -x, -y: where one side turns into the next
    #define RG_POINT(P, X, Y) float P[3] = { (float)(X), (float)(Y), g_rg.z(g_rg.mesh, (unsigned long)(X), (unsigned long)(Y)) }
    #define RG_SIDE(P, Q, D) do { n = rg_black_side(v, n, P, Q, dirs[D][0], dirs[D][1]); \
        const float *e_[2] = { P, Q }; for (int k_ = 0; k_ < 2; k_++) { \
            float s_[4] = { e_[k_][0], e_[k_][1], -e_[k_][0], -e_[k_][1] }; \
            for (int c_ = 0; c_ < 4; c_++) { float t_[4] = { corner[c_][0], corner[c_][1], -corner[c_][0], -corner[c_][1] }; \
                if (!have || s_[c_] > t_[c_]) memcpy(corner[c_], e_[k_], sizeof corner[c_]); } have = 1; } } while (0)
    for (int y = 0; y < g_rg.size; y++) for (int x = 0; x < g_rg.size; x++) {
        if (!g_rg.tile(g_rg.mesh, (unsigned long)x, (unsigned long)y)) continue;
        RG_POINT(a, x, y); RG_POINT(b, x, y + 1); RG_POINT(c, x + 1, y); RG_POINT(d, x + 1, y + 1);
        if (g_rg.right(g_rg.mesh, x, y)) RG_SIDE(b, c, 1);
        else if (g_rg.left(g_rg.mesh, x, y)) RG_SIDE(b, c, 3);
        else {
            if (g_rg.top(g_rg.mesh, x, y)) RG_SIDE(a, d, 2);
            if (g_rg.bottom(g_rg.mesh, x, y)) RG_SIDE(a, d, 0);
        }
        if ((size_t)n + 6 * 8 > cap - 4 * 3 * 6) break;
    }
    #undef RG_SIDE
    #undef RG_POINT
    if (have) for (int c = 0; c < 4; c++) {   // +x lies between back and right, +y right and front, -x front and left, -y left and back
        const float *from = dirs[c], *to = dirs[(c + 1) % 4];
        float a1[3], b1[3], a2[3], b2[3];
        rg_out(a1, corner[c], from[0], from[1], RG_SKIRT_NEAR); rg_out(b1, corner[c], to[0], to[1], RG_SKIRT_NEAR);
        rg_out(a2, corner[c], from[0], from[1], RG_SKIRT_FAR); rg_out(b2, corner[c], to[0], to[1], RG_SKIRT_FAR);
        n = rg_black_tri(v, n, corner[c], a1, b1);
        n = rg_black_tri(v, n, a1, a2, b2);
        n = rg_black_tri(v, n, a1, b2, b1);
    }
    D3D11_BUFFER_DESC bd = { (UINT)n * sizeof *v, D3D11_USAGE_DEFAULT, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
    D3D11_SUBRESOURCE_DATA sd = { v, 0, 0 };
    if (n && SUCCEEDED(ID3D11Device_CreateBuffer(g_d3d.dev, &bd, &sd, &g_rg.skirt))) g_rg.nskirt = n;
    free(v);
}
internal int rg_build(void) {
    ee_map *map = game_map();
    if (!map || !rg_api()) return 0;
    if (g_rg.built && g_rg.map == map) return 1;
    rg_free();
    g_rg.map = map;
    g_rg.size = (int)map_get_size(map);
    g_rg.across = (g_rg.size + RG_CHUNK - 1) / RG_CHUNK;
    g_rg.tris = 0;
    g_rg.c = calloc((size_t)g_rg.across * (size_t)g_rg.across, sizeof *g_rg.c);
    rg_tri *tris = malloc(sizeof *tris * (size_t)RG_CHUNK * RG_CHUNK * 2 * RG_LAYERS);
    if (!g_rg.c || !tris) { free(tris); rg_free(); return 0; }
    LONGLONG t0 = rd_qpc();
    g_rg.wcol = rg_bytes(g_rg.water_colour(g_rg.mesh, 0, 0), 0);
    for (int cy = 0; cy < g_rg.across; cy++) for (int cx = 0; cx < g_rg.across; cx++) rg_chunk_build(cx, cy, tris);
    free(tris);
    rg_skirt_build();
    D3D11_TEXTURE2D_DESC td = { (UINT)g_rg.size, (UINT)g_rg.size, 1, 1, DXGI_FORMAT_R8_UNORM, { 1, 0 }, D3D11_USAGE_DYNAMIC, D3D11_BIND_SHADER_RESOURCE, D3D11_CPU_ACCESS_WRITE, 0 };
    g_rg.fow_was = calloc((size_t)g_rg.size, (size_t)g_rg.size + 1);
    g_rg.fow_now = calloc((size_t)g_rg.size, (size_t)g_rg.size);
    if (!g_rg.fow_was || !g_rg.fow_now || FAILED(ID3D11Device_CreateTexture2D(g_d3d.dev, &td, NULL, &g_rg.fow))
        || FAILED(ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)g_rg.fow, NULL, &g_rg.fow_srv))) { rg_free(); return 0; }
    if (!g_rg.cb) {
        D3D11_BUFFER_DESC bd = { 72 * sizeof(float), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
        if (FAILED(ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_rg.cb)) || FAILED(ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_rg.cbw))) { rg_free(); return 0; }
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
internal int rg_visible(const rg_part *c, const float *m) {
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
// the ground (water 0) or the water of this frame into the list, on the game's states for it
internal void rg_draw_now(int water, int again) {
    rd_device *d = g_rd3.prep_dev;
    if (!g_rg.ras || !rg_build()) return;
    int i = g_rg.dll;
    rg_states *st = &g_rg.st[water];
    if (again && !st->ok) return;
    rd3_flush();
    // the projection the game's draw would set: the viewport's (+0x1F0, a GETransformation by rows; +0x1B0 the fixed camera's)
    const float *gp = (const float *)(*(const uint8_t **)(*(const uintptr_t *)((const uint8_t *)g_rg.ras + 0x20) + g_rg.screen * 4) + (g_rg.flag ? 0x1B0 : 0x1F0));
    // (in a frame during a step the current viewport is the interface's: the projection of the last frame that drew the ground)
    float proj[16], vpm[16];
    if (again) memcpy(proj, st->proj, sizeof proj);
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
        const rg_part *c = &g_rg.c[k].p[water];
        vis[k] = c->vb && rg_visible(c, vpm);
        if (vis[k]) for (int r = 0; r < c->nr; r++) want[c->r[r].m] = 1;
    }
    int nmat = water ? (int)mesh_water_groups(g_rg.mesh) : g_rg.nmat;
    if (nmat > RG_MATERIALS) nmat = RG_MATERIALS;
    if (!water) want[0] = 1;   // the black round the map: any texture, its colour is 0
    for (int m = 0; m < nmat; m++) {
        srv[m] = NULL;
        if (!want[m]) continue;
        g_rg.bind[i](g_rg.ras, water ? mesh_water_texture(g_rg.mesh, (unsigned)m) : g_rg.mat[m], g_rg.screen);
        rd_surface *t = d->tex[0];
        if (!t) continue;
        srv[m] = rd_gpu_texture(t, 0);
        (t->top ? t->top : t)->rf_used = g_rd_epoch;
    }
    // a frame during a step: the game has set no 3D states for it (its draw doesn't run): those of the last frame that did
    if (again) {
        g_rd3.rtv = st->rtv; g_rd3.dsv = st->dsv; g_rd3.blend = st->blend; g_rd3.depth = st->depth;
        g_rd3.raster = st->raster; g_rd3.samp = st->samp; g_rd3.vport = st->vport;
        g_rd3.vc = st->vc; g_rd3.pc = st->pc;
    } else {
        d->changed = -1;
        if (!rd3_prepare(d, 0)) return;
    }
    memcpy(g_rd3.vc.vpm, vpm, sizeof vpm);
    memcpy(g_rd3.vc.view, g_cam_view_now, sizeof g_cam_view_now);
    // its part in the shadows, the vertex shader's and the pixel shader's: the map's ground takes them as the game's does
    g_rd3.vc.shp[0] = g_rd3.pc.shd[0] = water ? SH_WATER : SH_RECEIVE;
    g_rf.vc_new = g_rf.pc_new = 1;
    if (!again) {
        st->rtv = g_rd3.rtv; st->dsv = g_rd3.dsv; st->blend = g_rd3.blend; st->depth = g_rd3.depth;
        st->raster = g_rd3.raster; st->samp = g_rd3.samp; st->vport = g_rd3.vport;
        st->vc = g_rd3.vc; st->pc = g_rd3.pc;
        memcpy(st->proj, proj, sizeof proj);
        st->ok = 1;
    }
    g_rb.topo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    const float *ws = (const float *)((const uint8_t *)g_rg.ras + 0x40);   // the movement's speed, the surface's strength, the seabed's
    if (!water) {
        rg_fow();
        LONGLONG now = rd_qpc();
        if (g_rg.at) g_rg.phase = fmod(g_rg.phase + (double)ws[0] * (double)(now - g_rg.at) / (double)rd_qpc_ms(1000), 6.283185307179586 * 1024.0);
        g_rg.at = now;
    }
    float cb[72] = { water ? 2.0f : 1.0f, (float)g_rg.phase, ws[water ? 1 : 2], rg_revealed() ? 1.0f : 0.0f };
    memcpy(cb + 4, g_rg.lights.amb, sizeof g_rg.lights.amb);
    cb[7] = (float)g_rg.lights.n;
    for (int l = 0; l < g_rg.lights.n; l++) { memcpy(cb + 8 + l * 4, g_rg.lights.dir[l], 12); memcpy(cb + 40 + l * 4, g_rg.lights.col[l], 12); }
    ID3D11DeviceContext_UpdateSubresource(g_d3d.ctx, (ID3D11Resource *)(water ? g_rg.cbw : g_rg.cb), 0, NULL, cb, 0, 0);
    long drawn = 0;
    for (int k = 0; k < nc; k++) {
        if (!vis[k]) continue;
        const rg_part *c = &g_rg.c[k].p[water];
        if (water) { g_sh.water_z -= g_rg.c[k].wshore * ws[1] * 0.5; g_sh.water_n += g_rg.c[k].wn; }   // the level, as the game's vertices give it (its mean: the sines cancel)
        for (int r = 0; r < c->nr; r++) {
            if (!srv[c->r[r].m]) continue;
            g_rd3.srv = srv[c->r[r].m];
            rf_record((UINT)c->r[r].first, (UINT)c->r[r].n, 0);
            if (g_rf.n) g_rf.cmd[g_rf.n - 1].gvb = c->vb;
            drawn++;
        }
    }
    if (!water && g_rg.skirt && srv[0]) {
        g_rd3.srv = srv[0];
        rf_record(0, (UINT)g_rg.nskirt, 0);
        if (g_rf.n) g_rf.cmd[g_rf.n - 1].gvb = g_rg.skirt;
    }
    if (!water) { g_rg.drawn = drawn; g_rg.frames++; }
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
    ID3D11DeviceContext_VSSetConstantBuffers(x, 3, 1, c->ro == RO_WATER ? &g_rg.cbw : &g_rg.cb);
    ID3D11DeviceContext_VSSetShaderResources(x, 7, 1, &g_rg.fow_srv);
    ID3D11DeviceContext_Draw(x, c->n, c->at);
    ID3D11DeviceContext_IASetVertexBuffers(x, 0, 1, &g_rd3.vb, &st, &off);
    ID3D11DeviceContext_VSSetConstantBuffers(x, 3, 1, &none);
    ID3D11DeviceContext_VSSetShaderResources(x, 7, 1, &nos);
}
internal int rg_hook(int i, int water, void *ras, void *mesh, unsigned screen, int flag) {
    // Art of Conquest's maps with space tiles (GETerrainMesh::SetIsSpace) are the game's still: the mesh draws those its own way, not yet done here
    if (!rg_ready() || !rg_api() || (g_rg.is_space && g_rg.is_space(mesh))) return (water ? g_rg.water : g_rg.draw)[i](ras, mesh, screen, flag);
    g_rg.ras = ras; g_rg.mesh = mesh; g_rg.screen = screen; g_rg.dll = i; g_rg.flag = flag & 0xFF;
    RD_LOCK(); rg_draw_now(water, 0); RD_UNLOCK();
    return 0;
}
internal int __thiscall rg_hook0(void *ras, void *mesh, unsigned screen, int flag) { return rg_hook(0, 0, ras, mesh, screen, flag); }
internal int __thiscall rg_hook1(void *ras, void *mesh, unsigned screen, int flag) { return rg_hook(1, 0, ras, mesh, screen, flag); }
internal int __thiscall rg_water_hook0(void *ras, void *mesh, unsigned screen, int flag) { return rg_hook(0, 1, ras, mesh, screen, flag); }
internal int __thiscall rg_water_hook1(void *ras, void *mesh, unsigned screen, int flag) { return rg_hook(1, 1, ras, mesh, screen, flag); }
// a frame during a step: the game's draw doesn't run, the ground is drawn from the map all the same - what was made from
// this map: not while another writer (a load) has the world, nor after one until a real frame has made it anew
internal int rg_again(int water, int writer) {
    if (writer || !rg_ready() || !g_rg.ras || !g_rg.built || g_rg.map != game_map()) return 0;
    RD_LOCK(); rg_draw_now(water, 1); RD_UNLOCK();
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
    void *wa = (void *)GetProcAddress(m, "?DrawWater@DX7Rasterizer@@UAEJAAVGETerrainMesh@@K_N@Z");
    if (!wa) return 0;
    g_rg.water[i] = (rg_draw_fn)wa;
    return (replace_rdata_ptr(m, li, i ? (void *)rg_light_hook1 : (void *)rg_light_hook0) > 0) + (replace_rdata_ptr(m, fn, i ? (void *)rg_hook1 : (void *)rg_hook0) > 0)
         + (replace_rdata_ptr(m, wa, i ? (void *)rg_water_hook1 : (void *)rg_water_hook0) > 0);
}
