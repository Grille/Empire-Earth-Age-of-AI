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
// When the ground changes under a running game (a building flattens it: SetZ, then the normals and the shores again) the
// chunks that touches are marked, and made again at the next frame that draws the ground - the game's writer is done by
// then, the frame holds the world's read lock. The scenario editor paints through paths of its own: what it looks at is made
// again a few times a second and the rest of the map a chunk at a time.
// Art of Conquest's maps with space tiles draw their edges and cliffs as its engine does (ART OF CONQUEST'S SPACE MAPS below).
#define RG_CHUNK 32
#define RG_MATERIALS 1024
#define RG_LAYERS 4     // a quad's textures at most
#define RG_ACROSS 64    // chunks along a side at most: the marks' table
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
    int tris;
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
    float revealed;   // no fog of war, as the last frame that could ask the game
} rg_states;
// THE OVERLAY ON THE GROUND (the player, 2026-11-13: "right now our overlay lines all fail when the terrain isnt level, could
// the be projected on the terrain (and waterline) ?"): the overlay's shapes on the map (render_overlay.c ro_ground_map) are
// drawn from above into a picture of the ground's visible chunks, RG_OV_PPT pixels a tile at most (the picture
// RG_OV_SIDE across), and the ground's and the water's pixels lay it over their colour by their own place on the map (the
// shader's gom, gov) - on every slope, on the water's moving surface, under the buildings and units. The ground under the
// water's level takes none (the line is the surface's, not the seabed's seen through it).
#define RG_OV_SIDE 4096
#define RG_OV_PPT 32.0f
internal ID3D11ShaderResourceView *g_rg_ov_srv;   // the picture (render_overlay.c); NULL none
internal struct {
    int on, api, built, dll, flag;
    int space;   // the map was made as Art of Conquest's space map (rg_space_quad)
    int ov_on;   // the overlay's picture this frame: its tiles from (ov[0], ov[1]), ov[2] x ov[3] pixels, ov_ppt a tile
    float ov[4], ov_ppt;
    float vis_t[4];   // the visible chunks' tiles this frame: x0, y0, x1, y1 (ends out), as of frame vis_at
    LONG vis_at;
    rg_states st[2];
    int size, across;
    ee_map *map;
    void *mesh, *ras;
    unsigned screen;
    rg_chunk *c;
    void *mat[RG_MATERIALS];
    int nmat;
    ID3D11ShaderResourceView *srv[2][RG_MATERIALS];   // the ground's and the water's textures as last looked up, a reference held: all a frame under another writer has of them
    uint8_t hmade[RG_MATERIALS];   // the ground's textures' heights are in the borders' array (rgb_height), once each
    float inv_n[RG_MATERIALS];     // a texture's share a tile (1 / the tiles its set repeats it over): its place at a point of the map
    int inv_dirty;
    ID3D11Texture2D *tq;           // THE BORDERS' TABLE: a tile's four corners' textures (rg_quad; R32G32B32A32_UINT), the map's size across
    ID3D11ShaderResourceView *tq_srv;
    uint32_t (*tqc)[4];            // ... the chunk's being made, from its first tile
    int tq_x0, tq_y0;
    void *msamp[2][RG_MATERIALS];                     // each material's sampler and stage-0 operations as its SetTexture set them (rg_draw_now)
    int32_t mop[2][RG_MATERIALS][6];
    ID3D11Texture2D *fow;
    ID3D11ShaderResourceView *fow_srv;
    uint8_t *fow_was, *fow_now;
    ID3D11Buffer *cb, *cbw, *skirt;
    int nskirt;
    uint32_t wcol;
    double phase;
    double wms;   // the water's time (ms) for the frames of its animation (rg_water_frame)
    unsigned wf_seen;   // the game's frame of its animation last seen, since when (rg_water_blend)
    LONGLONG wf_at;
    LONGLONG at;
    long drawn, tris, frames, remade;
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
// the chunks to make again: marked by whichever thread changes the ground
internal uint8_t g_rg_dirty[RG_ACROSS * RG_ACROSS];
internal volatile int g_rg_any;
internal volatile int g_rg_new_map;
internal mesh_set_z_fn g_rg_set_z;
internal mesh_at_fn g_rg_normal, g_rg_shores;
internal texture_set_texture_fn g_rg_set_texture;
internal volatile uint8_t g_rg_gone[RG_MATERIALS];   // a material whose texture its set let go: its chunks to make again

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
    for (int w = 0; w < 2; w++) for (int m = 0; m < RG_MATERIALS; m++) D3D_RELEASE(g_rg.srv[w][m]);
    memset(g_rg.hmade, 0, sizeof g_rg.hmade);
    D3D_RELEASE(g_rg.tq_srv);
    D3D_RELEASE(g_rg.tq);
    memset(g_rg.msamp, 0, sizeof g_rg.msamp);   // the cache's: not released
    g_rg.built = g_rg.nmat = 0;
    memset(g_rg_dirty, 0, sizeof g_rg_dirty);
    memset((void *)g_rg_gone, 0, sizeof g_rg_gone);
    g_rg_any = 0;
}
// a corner changed: the chunks of the tiles its height, colour or normal reach (a normal is its neighbours' heights: two tiles round)
internal void rg_touch(unsigned long x, unsigned long y) {
    int across = g_rg.across;
    if (!g_rg.built || across <= 0 || across > RG_ACROSS) return;
    for (int k = 0; k < 4; k++) {
        long cx = ((long)x - 2 + (k & 1) * 3) / RG_CHUNK, cy = ((long)y - 2 + (k >> 1) * 3) / RG_CHUNK;
        if (cx < 0) cx = 0; else if (cx >= across) cx = across - 1;
        if (cy < 0) cy = 0; else if (cy >= across) cy = across - 1;
        g_rg_dirty[cy * RG_ACROSS + cx] = 1;
    }
    g_rg_any = 1;
}
internal void __thiscall rg_hook_set_z(void *mesh, unsigned long x, unsigned long y, float z) { g_rg_set_z(mesh, x, y, z); rg_touch(x, y); }
internal void __thiscall rg_hook_normal(void *mesh, unsigned long x, unsigned long y) { g_rg_normal(mesh, x, y); rg_touch(x, y); }
internal void __thiscall rg_hook_shores(void *mesh, unsigned long x, unsigned long y) { g_rg_shores(mesh, x, y); rg_touch(x, y); }
// a new map is set up (each map's mesh gets its colour table once: terrain_palette.c's hook). What the renderer keeps of a
// map goes by the map's address, and that doesn't tell: the editor's Create Map frees its map and news one of the same size,
// mostly the same block, and the ground went on binding the textures of the map gone (the player, 2026-11-10: "game died in
// editor as i clicked on generate new map"). The next real frame makes all of it anew.
internal void render_new_map(void) {
    g_rg_new_map = 1;
    g_sh.hmap = NULL;
}
// a terrain type's texture let go: the type's last tile painted over sets all its set's textures to 0, and they are freed
// (the player, 2026-11-11: "game died", in the editor). The game's own draw asks the set each frame; the chunks keep the
// texture they were made with, and bound it after it was gone. Its material is dropped here (bound as none meanwhile, the
// picture kept: srv holds it) and the chunks drawn with it are made again at the next frame (rg_rebuild).
internal void __thiscall rg_hook_set_texture(void *set, unsigned long x, unsigned long y, void *tex) {
    void *old = g_rg.api > 0 ? g_rg.tex(set, x, y) : NULL;
    g_rg_set_texture(set, x, y, tex);
    if (!old || old == tex || !g_rg.built) return;
    for (int m = 0; m < g_rg.nmat; m++)
        if (g_rg.mat[m] == old) { g_rg.mat[m] = NULL; g_rg_gone[m] = 1; g_rg_any = 1; }
}
internal int rg_changes_install(void) {
    return patch_mesh_set_z(rg_hook_set_z, &g_rg_set_z) + patch_mesh_compute_normal(rg_hook_normal, &g_rg_normal) + patch_mesh_compute_shorelines(rg_hook_shores, &g_rg_shores)
         + patch_texture_set_texture(rg_hook_set_texture, &g_rg_set_texture);
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
// a point at x, y, z with the normal and colour of the map's point gx, gy
internal void rg_vertex_at(rb_vertex *o, float x, float y, float z, unsigned long gx, unsigned long gy, const float *uv, int a) {
    float *f = (float *)o->fvf;
    f[0] = x; f[1] = y; f[2] = z;
    memcpy(f + 3, g_rg.normal(g_rg.mesh, gx, gy), 12);
    *(uint32_t *)(f + 6) = rg_bytes(g_rg.colour(g_rg.mesh, gx, gy), a & 255);
    f[7] = uv[0]; f[8] = uv[1];
    o->w = (uint32_t)a >> 8;   // 1: the corner is this texture's (the vertex shader's ground: the borders' colour); 2: a space piece, as it is; bits 2-12 its material + 1, 13-23 its set's first's (rg_quad)
}
internal void rg_vertex(rb_vertex *o, unsigned long x, unsigned long y, const float *uv, int a) {
    rg_vertex_at(o, (float)x, (float)y, g_rg.z(g_rg.mesh, x, y), x, y, uv, a);
}
// layer 2, a space tile's edge (rg_space_quad), after everything else of its chunk: the faces under it show through it
internal int rg_tri_order(const void *pa, const void *pb) {
    const rg_tri *a = pa, *b = pb;
    if ((a->layer == 2) != (b->layer == 2)) return a->layer == 2 ? 1 : -1;
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
// THE SET'S FIRST: a terrain's set is several textures, each over a block of tiles (GETextureSet::GetTexture: by x and y
// over the tiles a texture covers), and the textures of a tile's corners are the set's for the tile. Two tiles across a
// block's edge took their border's heights and noise from two textures, and the border broke there in a straight line
// (the player, 2026-11-24: "ground blending can still have very noticable seams"). The noise was each texture's own and
// jumped at every block's edge. The borders' noise and which corners are one terrain go by the set's first texture; the
// heights stay those of the texture the tile shows (by the first's heights over the whole map the borders no longer
// followed the cracks of the cobblestones shown - the player, 2026-11-24: "a slight downgrade").
// m the texture's material; -1 none
internal int rg_canon(void *set, int m) {
    if (m < 0 || !set) return m;
    void *first = g_rg.tex(set, 0, 0);
    int c = first ? rg_material(first) : m;
    return c < 0 ? m : c;
}
// the texture of a cell for the quad at x, y: the quad's own for a cell outside the map or of the same set
internal void *rg_cell(long cx, long cy, unsigned long x, unsigned long y, unsigned long own, void *own_tex, void **set) {
    if (cx < 0 || cy < 0 || !g_rg.tile(g_rg.mesh, (unsigned long)cx, (unsigned long)cy)) return own_tex;
    unsigned long id = g_rg.set(g_rg.mesh, (unsigned long)cx, (unsigned long)cy);
    if (id == own) return own_tex;
    *set = g_rg.reg(g_rg.mesh, id);
    return g_rg.tex(*set, x, y);
}
// ART OF CONQUEST'S SPACE MAPS (GETerrainMesh::GetIsSpaceMap: any tile set space, SetIsSpace; AoC's engine's
// UpdateVisibleTerrain 0x1002C299 and AddCliffPolys 0x1002B8E8, the space tile's bit 0x80 of the byte GetWaterInfo reads).
// A space tile itself is nothing (the game's quad of no texture, alpha -1); one at the land's edge has the edge's pattern
// in its bits 1-4, which lays one of the space textures (mesh_space_cliff 8-12) on it with that pattern's places in the
// texture, and hangs a face (0-7, by the tile's x or y) from a line across it 8 down, its lower edge jagged by a table
// of the tile's place - not along the map's left and right edges. A land corner whose cell has such a tile beside it
// takes texture 12 in place of its set's, at the tile's own places (0 to 1 across it). No black round the map but behind
// it. The game draws no water on a space map, but the map has it all the same - a tile with a corner under the level is
// water to the simulation as anywhere (rg_water_bits).
#define RG_SPACE 0x80
#define RG_SPACE_EDGE 0x1E
#define RG_CLIFF_DROP 8.0f
// by the pattern (the bits >> 1): the texture (8 +), its places at (x + 1, y + 1), (x, y + 1), (x, y), (x + 1, y), the
// face's texture by (1 x, 2 y, 3 2x, 4 2x + 1, 5 2y, 6 2y + 1; 0 none, of 8), the line under which it hangs (x, y, x, y
// onto the tile's) - AoC's engine's table at 0x10097BF8
typedef struct { int tex; float uv[4][2]; int face; float line[4]; } rg_space_edge;
internal const rg_space_edge k_rg_space[16] = {
    { 0, { { 1, 0 }, { 0, 0 }, { 0, 1 }, { 1, 1 } }, 0, { 0, 0, 0, 0 } },
    { 0, { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } }, 5, { .4f, 0, 0, .4f } },
    { 0, { { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 } }, 3, { 0, .6f, .4f, 1 } },
    { 1, { { 1, 0 }, { 1, 1 }, { 0, 1 }, { 0, 0 } }, 2, { .4f, 0, .4f, 1 } },
    { 0, { { 1, 1 }, { 0, 1 }, { 0, 0 }, { 1, 0 } }, 4, { .6f, 1, 1, .6f } },
    { 3, { { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 } }, 2, { 0, .4f, .4f, 0 } },
    { 1, { { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 } }, 1, { 0, .5f, 1, .6f } },
    { 2, { { 0, 1 }, { 1, 1 }, { 1, 0 }, { 0, 0 } }, 4, { .4f, 0, 1, .6f } },
    { 0, { { 1, 0 }, { 0, 0 }, { 0, 1 }, { 1, 1 } }, 0, { 0, 0, 0, 0 } },
    { 1, { { 1, 0 }, { 0, 0 }, { 0, 1 }, { 1, 1 } }, 0, { 0, 0, 0, 0 } },
    { 3, { { 1, 1 }, { 0, 1 }, { 0, 0 }, { 1, 0 } }, 1, { 0, .6f, .4f, 1 } },
    { 2, { { 0, 0 }, { 1, 0 }, { 1, 1 }, { 0, 1 } }, 6, { 1, .4f, .4f, 1 } },
    { 1, { { 1, 1 }, { 1, 0 }, { 0, 0 }, { 0, 1 } }, 0, { 0, 0, 0, 0 } },
    { 2, { { 1, 0 }, { 0, 0 }, { 0, 1 }, { 1, 1 } }, 0, { 0, 0, 0, 0 } },
    { 2, { { 1, 1 }, { 0, 1 }, { 0, 0 }, { 1, 0 } }, 3, { 0, .6f, .6f, 0 } },
    { 4, { { 1, 0 }, { 0, 0 }, { 0, 1 }, { 1, 1 } }, 0, { 0, 0, 0, 0 } },
};
// the faces' lower edge, by the point's x % 16 x 16 + y % 16, times a half (0x100D9760)
internal const float k_rg_jag[256] = {
    -.997f, .127f, -.613f, .617f, .17f, -.04f, -.299f, .792f, .646f, .493f, -.652f, .718f, .421f, .027f, -.392f, -.97f,
    -.817f, -.271f, -.705f, -.668f, .977f, -.109f, -.762f, -.991f, -.982f, -.244f, .063f, .142f, .204f, .214f, -.668f, .326f,
    -.098f, -.296f, -.886f, .215f, .567f, .605f, .04f, -.396f, .752f, .453f, .912f, .851f, .079f, -.715f, -.076f, -.529f,
    .724f, -.581f, .559f, .687f, .994f, .999f, .223f, -.215f, -.468f, -.405f, .68f, -.953f, -.248f, -.815f, .354f, -.888f,
    -.982f, .838f, -.448f, -.454f, .176f, .382f, .675f, .453f, -.03f, -.589f, .487f, -.063f, -.084f, .898f, .489f, -.783f,
    .198f, -.23f, .47f, .218f, .145f, -.277f, -.697f, -.55f, -.15f, .606f, .034f, .98f, .503f, -.309f, -.662f, .315f,
    -.016f, -.873f, .4f, .01f, -.705f, .899f, -.717f, .81f, .386f, -.394f, -.147f, -.859f, .933f, .366f, -.694f, .755f,
    .643f, .164f, -.617f, -.644f, .634f, -.049f, -.689f, .008f, .464f, -.189f, -.441f, .137f, .364f, .512f, .444f, -.049f,
    -.754f, -.264f, .669f, -.93f, .034f, .326f, -.148f, -.791f, .899f, .843f, .099f, -.308f, -.057f, -.25f, .694f, -.366f,
    -.088f, -.456f, .966f, -.404f, .478f, .135f, -.608f, .523f, .679f, -.205f, .002f, .78f, -.945f, .989f, .145f, -.899f,
    .063f, -.612f, .686f, .254f, .315f, -.604f, .684f, -.753f, -.78f, .486f, -.372f, .882f, -.428f, -.327f, -.719f, .466f,
    .669f, .416f, .2f, .494f, -.495f, -.711f, -.997f, -.878f, .612f, .705f, -.579f, -.769f, .106f, -.971f, -.772f, -.091f,
    .504f, .372f, .087f, -.852f, -.127f, -.596f, .392f, -.419f, -.127f, -.535f, .156f, .065f, .257f, -.68f, .008f, .926f,
    .392f, .85f, -.62f, -.328f, -.643f, .99f, -.085f, .996f, -.805f, .25f, -.811f, -.125f, .863f, -.903f, .789f, -.42f,
    -.545f, .538f, -.179f, -.596f, .256f, .208f, -.097f, -.067f, .196f, .269f, .71f, .658f, .25f, .442f, .132f, -.25f,
    -.631f, .476f, .11f, .81f, -.514f, -.622f, .209f, .397f, .169f, -.297f, -.011f, -.839f, .481f, .224f, .241f, .382f,
};
// a tile's triangles of one texture, as the ground's (the map's edges), the places and alphas at (x, y), (x, y + 1), (x + 1, y), (x + 1, y + 1)
internal int rg_tile_tris(rg_tri *out, unsigned long x, unsigned long y, void *tex, int layer, const float uv[4][2], const int al[4]) {
    int left = g_rg.left(g_rg.mesh, (long)x, (long)y), right = g_rg.right(g_rg.mesh, (long)x, (long)y);
    int topq = g_rg.top(g_rg.mesh, (long)x, (long)y), bottom = g_rg.bottom(g_rg.mesh, (long)x, (long)y);
    static const int dx[4] = { 0, 0, 1, 1 }, dy[4] = { 0, 1, 0, 1 };
    int tri[2][3], nt = 0;
    #define RG_T(a, b, c) (tri[nt][0] = a, tri[nt][1] = b, tri[nt][2] = c, nt++)
    if (right) RG_T(0, 1, 2);
    else if (left) RG_T(1, 3, 2);
    else {
        if (!topq) RG_T(0, 1, 3);
        if (!bottom) RG_T(0, 3, 2);
    }
    #undef RG_T
    for (int i = 0; i < nt; i++) {
        out[i].tex = tex; out[i].layer = layer;
        for (int k = 0; k < 3; k++) { int c = tri[i][k]; rg_vertex(&out[i].v[k], x + dx[c], y + dy[c], uv[c], al[c]); }
    }
    return nt;
}
// the height of the tile at x, y at the point px, py on it, as its two triangles (rg_tile_tris: split from (x, y) to
// (x + 1, y + 1)) lie
internal float rg_tile_z(unsigned long x, unsigned long y, float px, float py) {
    float u = px - (float)x, v = py - (float)y;
    float z00 = g_rg.z(g_rg.mesh, x, y), z01 = g_rg.z(g_rg.mesh, x, y + 1), z10 = g_rg.z(g_rg.mesh, x + 1, y), z11 = g_rg.z(g_rg.mesh, x + 1, y + 1);
    return v >= u ? z00 + v * (z01 - z00) + u * (z11 - z01) : z00 + u * (z10 - z00) + v * (z11 - z10);
}
// which way a space tile's face looks: across its line, away from the tile's corners that touch land (one of the four
// tiles round the corner on the map and not space) - out into space, level. 0: no land corner, no line
internal int rg_space_out(const void *mesh, unsigned long x, unsigned long y, const rg_space_edge *s, float *nx, float *ny) {
    float cx = 0, cy = 0;
    int nl = 0;
    for (int c = 0; c < 4; c++) {
        long kx = (long)x + (c & 1), ky = (long)y + (c >> 1);
        for (int t = 0; t < 4; t++) {
            long tx = kx - 1 + (t & 1), ty = ky - 1 + (t >> 1);
            if (tx >= 0 && ty >= 0 && g_rg.tile(mesh, (unsigned long)tx, (unsigned long)ty) && !(g_rg.water_info(mesh, (unsigned long)tx, (unsigned long)ty) & RG_SPACE)) {
                cx += (float)kx; cy += (float)ky; nl++;
                break;
            }
        }
    }
    float ax = s->line[2] - s->line[0], ay = s->line[3] - s->line[1], l = sqrtf(ax * ax + ay * ay);
    if (!nl || l <= 0) return 0;
    float ox = ay / l, oy = -ax / l, mx = (float)x + (s->line[0] + s->line[2]) * 0.5f, my = (float)y + (s->line[1] + s->line[3]) * 0.5f;
    if (ox * (cx / (float)nl - mx) + oy * (cy / (float)nl - my) > 0) ox = -ox, oy = -oy;
    *nx = ox; *ny = oy;
    return 1;
}
// the normal at a face's end ex, ey: its own (nx, ny) and those of the faces of the tiles round it that end there too
// (within 0.2: along a straight edge the table's lines step by a tenth from tile to tile),
// together (each a tile's whole face, so a corner's turn is shared out over the two faces either side of it)
internal void rg_space_end(unsigned long x, unsigned long y, float ex, float ey, float nx, float ny, float *e) {
    float sx = nx, sy = ny;
    for (int j = -1; j <= 1; j++)
        for (int i = -1; i <= 1; i++) {
            long tx = (long)x + i, ty = (long)y + j;
            if ((!i && !j) || tx < 0 || ty < 0 || !g_rg.tile(g_rg.mesh, (unsigned long)tx, (unsigned long)ty)) continue;
            unsigned info = g_rg.water_info(g_rg.mesh, (unsigned long)tx, (unsigned long)ty);
            if (!(info & RG_SPACE)) continue;
            const rg_space_edge *o = &k_rg_space[info >> 1 & 15];
            float ox, oy;
            if (!o->face || !rg_space_out(g_rg.mesh, (unsigned long)tx, (unsigned long)ty, o, &ox, &oy)) continue;
            for (int k = 0; k < 4; k += 2)
                if (fabsf((float)tx + o->line[k] - ex) < 0.2f && fabsf((float)ty + o->line[k + 1] - ey) < 0.2f) { sx += ox; sy += oy; break; }
        }
    float l = sqrtf(sx * sx + sy * sy);
    if (l < 1e-3f) { e[0] = nx; e[1] = ny; return; }   // faces back to back: its own
    e[0] = sx / l; e[1] = sy / l;
}
internal int rg_space_quad(rg_tri *out, unsigned long x, unsigned long y) {
    const rg_space_edge *s = &k_rg_space[g_rg.water_info(g_rg.mesh, x, y) >> 1 & 15];
    if (s == k_rg_space) return 0;
    void *tex = mesh_space_cliff(g_rg.mesh, 8 + (unsigned)s->tex);
    int n = 0, al[4] = { 255 | 2 << 8, 255 | 2 << 8, 255 | 2 << 8, 255 | 2 << 8 };
    if (tex) {
        const float uv[4][2] = { { s->uv[2][0], s->uv[2][1] }, { s->uv[1][0], s->uv[1][1] }, { s->uv[3][0], s->uv[3][1] }, { s->uv[0][0], s->uv[0][1] } };
        n = rg_tile_tris(out, x, y, tex, 2, uv, al);
    }
    if (!s->face || g_rg.left(g_rg.mesh, (long)x, (long)y) || g_rg.right(g_rg.mesh, (long)x, (long)y)) return n;
    unsigned long k = s->face == 1 ? x : s->face == 2 ? y : s->face == 3 ? 2 * x : s->face == 4 ? 2 * x + 1 : s->face == 5 ? 2 * y : 2 * y + 1;
    void *face = mesh_space_cliff(g_rg.mesh, (unsigned)(k % 8));
    if (!face) return n;
    float px = (float)x + s->line[0], py = (float)y + s->line[1], qx = (float)x + s->line[2], qy = (float)y + s->line[3];
    unsigned long pi = (unsigned long)px, pj = (unsigned long)py, qi = (unsigned long)qx, qj = (unsigned long)qy;   // cut off, as the engine's _ftol
    // The top of the face: the engine takes the height of the corner its point is cut off to (less 0.02 and 0.05), and on
    // a slope the face's edge stood above or below the edge's texture laid on the tile's own corners - gaps into space
    // and faces sticking up (the player, 2026-11-23: "the cliff where terrain goes into space doesnt connect to the
    // terrain properly when there are elevation changes"). Here the height of that texture's triangles at the point -
    // and not the 0.02 / 0.05 under it: from a camera low over the edge that was a slit under the ground between the
    // edge's texture and the face, the space beyond seen through it (the player, the same day: "on flat ground the space
    // cliffs arent closing fully"; the game's own drawing has it too). At the surface the face closes it; moving the
    // line toward the land instead tore the faces apart at the corners
    float pz = rg_tile_z(x, y, px, py), qz = rg_tile_z(x, y, qx, qy);
    float pb = pz - RG_CLIFF_DROP + k_rg_jag[pi % 16 * 16 + pj % 16] * 0.5f, qb = qz - RG_CLIFF_DROP + k_rg_jag[qi % 16 * 16 + qj % 16] * 0.5f;
    static const float u00[2] = { 0, 0 }, u10[2] = { 1, 0 }, u01[2] = { 0, 1 }, u11[2] = { 1, 1 };
    int a = 255 | 2 << 8;
    out[n].tex = face; out[n].layer = 0;
    rg_vertex_at(&out[n].v[0], px, py, pz, pi, pj, u00, a); rg_vertex_at(&out[n].v[1], qx, qy, qz, qi, qj, u10, a); rg_vertex_at(&out[n].v[2], px, py, pb, pi, pj, u01, a);
    out[n + 1].tex = face; out[n + 1].layer = 0;
    rg_vertex_at(&out[n + 1].v[0], px, py, pb, pi, pj, u01, a); rg_vertex_at(&out[n + 1].v[1], qx, qy, qz, qi, qj, u10, a); rg_vertex_at(&out[n + 1].v[2], qx, qy, qb, qi, qj, u11, a);
    // The face's own normal, level and out into space (the engine gives it its top corner's, the ground's: a face lit as
    // the ground on top, as bright turned from the sun as toward it - the player, 2026-11-23: "the space cliffs dont throw
    // shadows, wich can look a bit weired"). The vertex shader tells a face by it (render_3d.c ground: a space piece whose
    // normal lies level): its light by it, and the hills' shadows taken a little out in front of it, at its own height
    // - at each end the faces meeting there together (rg_space_end): one normal a face, the light changed at once where the
    // edge turns, a hard seam down each corner (the player, the same day: "is it possible to make the edges less harsh?")
    float nx, ny, en[2][2];
    if (rg_space_out(g_rg.mesh, x, y, s, &nx, &ny)) {
        rg_space_end(x, y, px, py, nx, ny, en[0]);
        rg_space_end(x, y, qx, qy, nx, ny, en[1]);
        static const uint8_t at_q[2][3] = { { 0, 1, 0 }, { 0, 1, 1 } };   // the vertices at Q (else P)
        for (int t = 0; t < 2; t++)
            for (int c = 0; c < 3; c++) { float *f = (float *)out[n + t].v[c].fvf, *e = en[at_q[t][c]]; f[3] = e[0]; f[4] = e[1]; f[5] = 0; }
    }
    return n + 2;
}
// a land corner (D B A C) whose cell has a space tile at the edge beside it (the engine's table at 0x10097F98): texture 12, no set.
// The engine asks only for bits 1-4, but a land tile can still hold a shore's there: the editor's first space tile makes
// the map a space map, and SetIsSpace (0x1002D2A6) runs ComputeShorelines - which clears a land tile's bits on a space
// map - on the 3 x 3 round it alone, so every lake's shore further off kept its bits and was taken for space's edge (the
// player, 2026-11-23: "when having water on a map and adding space for the first time all shores become space")
internal void rg_space_corners(void **t, void **sets, unsigned long x, unsigned long y) {
    static const int cell[4][2] = { { 0, 0 }, { -1, 0 }, { -1, -1 }, { 0, -1 } };
    static const int nb[4][3][2] = { { { 0, 1 }, { 1, 1 }, { 1, 0 } }, { { 0, 1 }, { -1, 1 }, { -1, 0 } },
                                     { { 0, -1 }, { -1, -1 }, { -1, 0 } }, { { 0, -1 }, { 1, -1 }, { 1, 0 } } };
    void *edge = mesh_space_cliff(g_rg.mesh, 12);
    if (!edge) return;
    for (int k = 0; k < 4; k++) {
        long cx = (long)x + cell[k][0], cy = (long)y + cell[k][1];
        if (cx < 0 || cy < 0 || !g_rg.tile(g_rg.mesh, (unsigned long)cx, (unsigned long)cy)) continue;
        for (int j = 0; j < 3; j++) {
            long nx = (long)x + nb[k][j][0], ny = (long)y + nb[k][j][1];
            if (nx < 0 || ny < 0 || !g_rg.tile(g_rg.mesh, (unsigned long)nx, (unsigned long)ny)) continue;
            unsigned info = g_rg.water_info(g_rg.mesh, (unsigned long)nx, (unsigned long)ny);
            if ((info & RG_SPACE) && (info & RG_SPACE_EDGE)) { t[k] = edge; sets[k] = NULL; break; }
        }
    }
}
internal int rg_space_map(const void *mesh) { return g_rg.is_space && g_rg.is_space(mesh); }
// how deep the ground lies at px, py of a space map for the hills' shadows (render_shadows.c, the caller asks
// rg_space_map once): its space down the faces' drop - a space tile without an edge, and an edge tile beyond its face's
// line; everything else 0 (its surface)
internal float rg_space_depth(const void *mesh, float px, float py) {
    if (!g_rg.water_info || !g_rg.tile || !mesh || px < 0 || py < 0) return 0;
    unsigned long x = (unsigned long)px, y = (unsigned long)py;
    if (!g_rg.tile(mesh, x, y)) return 0;
    unsigned info = g_rg.water_info(mesh, x, y);
    if (!(info & RG_SPACE)) return 0;
    const rg_space_edge *s = &k_rg_space[info >> 1 & 15];
    float nx, ny;
    if (s == k_rg_space) return RG_CLIFF_DROP;
    if (!s->face || !rg_space_out(mesh, x, y, s, &nx, &ny)) return 0;
    return (px - (float)x - s->line[0]) * nx + (py - (float)y - s->line[1]) * ny > 0 ? RG_CLIFF_DROP : 0;
}
// the ground drawn as a space map: one, and its space textures there. The game hands them to the mesh in its world's
// draw (AoC 0x50E8D1: a space map with NumSpaceCliffTextures 0 - loaded once, then SetSpaceCliffTextures), at the first
// frame after the first space tile; the whole ground made anew before that had no edges and no faces anywhere, until
// a chunk was made again for something else (the player, 2026-11-23, space added in the editor: "the space cliffs
// arent closing fully"). Until they are there it is drawn as before, then made anew
internal int rg_space_drawn(const void *mesh) { return rg_space_map(mesh) && mesh_space_cliff(mesh, 12); }
// a tile of space on a space map (collect_remains_physics.c A SPACESHIP'S WRECK: nothing holds a wreck there)
internal int rg_space_tile(const void *mesh, int x, int y) {
    return g_rg.water_info && x >= 0 && y >= 0 && rg_space_map(mesh) && (g_rg.water_info(mesh, (unsigned long)x, (unsigned long)y) & RG_SPACE);
}
internal int rg_quad(rg_tri *out, unsigned long x, unsigned long y) {
    if (g_rg.space && g_rg.water_info(g_rg.mesh, x, y) & RG_SPACE) return rg_space_quad(out, x, y);
    unsigned long own = g_rg.set(g_rg.mesh, x, y);
    void *own_set = g_rg.reg(g_rg.mesh, own), *own_tex = g_rg.tex(own_set, x, y);
    void *sets[4] = { own_set, own_set, own_set, own_set };   // D B A C
    void *t[4];
    t[0] = own_tex;
    t[1] = rg_cell((long)x - 1, (long)y, x, y, own, own_tex, &sets[1]);
    t[2] = rg_cell((long)x - 1, (long)y - 1, x, y, own, own_tex, &sets[2]);
    t[3] = rg_cell((long)x, (long)y - 1, x, y, own, own_tex, &sets[3]);
    if (g_rg.space) rg_space_corners(t, sets, x, y);
    int base = 0;
    for (int i = 1; i < 4; i++) if ((uintptr_t)t[i] < (uintptr_t)t[base]) base = i;
    // the borders' table (rg_chunk_build): each corner's set's first texture's material + 1 (bits 0-10), its texture's
    // place among the quad's by address - the order they are drawn in (11-12), its texture's own material + 1 (13-23);
    // none (0) where a corner has no set (a space map's edge): its border by its own heights
    if (g_rg.tqc) {
        uint32_t *q = g_rg.tqc[(y - (unsigned long)g_rg.tq_y0) * RG_CHUNK + (x - (unsigned long)g_rg.tq_x0)];
        int cm[4], ok = 1;
        int am[4];
        for (int k = 0; k < 4; k++) { am[k] = rg_material(t[k]); cm[k] = rg_canon(sets[k], am[k]); ok = ok && cm[k] >= 0 && sets[k]; }
        for (int k = 0; k < 4 && ok; k++) {
            int rank = 0;
            for (int j = 0; j < 4; j++) {
                int seen = 0;
                for (int i = 0; i < j; i++) seen |= t[i] == t[j];
                rank += !seen && (uintptr_t)t[j] < (uintptr_t)t[k];
            }
            float uv[2];
            g_rg.uv(sets[k], 1, 1, uv);   // (1 mod n) / n
            float inv = uv[0] > 0 ? uv[0] : 1.0f;
            if (g_rg.inv_n[cm[k]] != inv || g_rg.inv_n[am[k]] != inv) { g_rg.inv_n[cm[k]] = g_rg.inv_n[am[k]] = inv; g_rg.inv_dirty = 1; }
            q[k] = (uint32_t)(cm[k] + 1) | (uint32_t)rank << 11 | (uint32_t)(am[k] + 1) << 13;
        }
        if (!ok) memset(q, 0, sizeof g_rg.tqc[0]);
    }
    int left = g_rg.left(g_rg.mesh, (long)x, (long)y), right = g_rg.right(g_rg.mesh, (long)x, (long)y);
    int topq = g_rg.top(g_rg.mesh, (long)x, (long)y), bottom = g_rg.bottom(g_rg.mesh, (long)x, (long)y);
    int n = 0;
    for (int k = -1; k < 4; k++) {   // the base first (-1), then each other texture once
        if (k >= 0 && (t[k] == t[base] || (k > 0 && t[k] == t[0]) || (k > 1 && t[k] == t[1]) || (k > 2 && t[k] == t[2]))) continue;
        void *tex = k < 0 ? t[base] : t[k], *set = k < 0 ? sets[base] : sets[k];
        float a[2], b[2], c[2], e[2];   // the texture's place at A, B, C, D
        float xy[2], x1y1[2];
        if (set) {
            g_rg.uv(set, x, y, xy);
            g_rg.uv(set, x + 1, y + 1, x1y1);
            float u1 = x1y1[0] == 0 ? 1.0f : x1y1[0], v1 = x1y1[1] == 0 ? 1.0f : x1y1[1];
            a[0] = xy[0]; a[1] = xy[1];
            b[0] = xy[0]; b[1] = v1;
            c[0] = u1; c[1] = xy[1];
            e[0] = u1; e[1] = v1;
        } else {   // a space map's edge texture on a land corner: the tile's own places, as AoC's engine gives every tile
            a[0] = 0; a[1] = 1; b[0] = 0; b[1] = 0; c[0] = 1; c[1] = 1; e[0] = 1; e[1] = 0;
        }
        // a corner's alpha: the whole base; a layer where the corner's cell has its texture. Bit 8: the cell has it (a point has one cell, so one texture and one colour)
        // Bits 10-20: the texture's material + 1 (its relief), bits 21-31 its set's first's (the borders' shader finds it
        // among the corners' in the table)
        int tm = rg_material(tex);
        int mb = (int)((uint32_t)(tm + 1) << 10 | (uint32_t)(rg_canon(set, tm) + 1) << 21);
        int aa = (k < 0 || t[2] == tex ? 255 : 0) | (t[2] == tex) << 8 | mb, ab = (k < 0 || t[1] == tex ? 255 : 0) | (t[1] == tex) << 8 | mb;
        int ac = (k < 0 || t[3] == tex ? 255 : 0) | (t[3] == tex) << 8 | mb, ad = (k < 0 || t[0] == tex ? 255 : 0) | (t[0] == tex) << 8 | mb;
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
// A tile's water byte: bit 0 water, bits 1-4 the corners at a shore (GetWaterInfo). On a space map the mesh holds none of
// it - SetIsWater does nothing there and ComputeShorelines keeps only the space bit, as bits 1-4 hold the edges' patterns
// (AoC's engine 0x10012959, 0x1002795F) - and the game draws no water at all, on planets with lakes too (the player,
// 2026-11-11: "these is no good reason why the planets cant have water on them"). So there it is made as the map makes
// it elsewhere: a tile is water when the lowest of its corners lies under the level (AoC's EEMap 0x5225FA: SetIsWater(x,
// y, that < 0)), not a space tile; its shores as ComputeShorelines puts them, each of its eight neighbours on the map that
// isn't water setting the corners toward it (the engine's table at 0x10097B98)
internal int rg_space_wet(unsigned long x, unsigned long y) {
    if (g_rg.water_info(g_rg.mesh, x, y) & RG_SPACE) return 0;
    float z = g_rg.z(g_rg.mesh, x, y), a = g_rg.z(g_rg.mesh, x + 1, y), b = g_rg.z(g_rg.mesh, x, y + 1), c = g_rg.z(g_rg.mesh, x + 1, y + 1);
    return z < 0 || a < 0 || b < 0 || c < 0;
}
internal unsigned rg_water_bits(unsigned long x, unsigned long y) {
    if (!g_rg.space) return g_rg.is_water(g_rg.mesh, x, y) ? g_rg.water_info(g_rg.mesh, x, y) : 0;
    if (!rg_space_wet(x, y)) return 0;
    static const int nb[8][3] = { { -1, 1, 4 }, { 1, 1, 8 }, { -1, -1, 2 }, { 1, -1, 16 }, { 0, 1, 12 }, { 1, 0, 24 }, { 0, -1, 18 }, { -1, 0, 6 } };
    unsigned bits = 1;
    for (int k = 0; k < 8; k++) {
        long nx = (long)x + nb[k][0], ny = (long)y + nb[k][1];
        if (nx >= 0 && ny >= 0 && g_rg.tile(g_rg.mesh, (unsigned long)nx, (unsigned long)ny) && !rg_space_wet((unsigned long)nx, (unsigned long)ny)) bits |= (unsigned)nb[k][2];
    }
    return bits;
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
    unsigned info = rg_water_bits(x, y);
    if (!info) return 0;
    unsigned m = mesh_water_patch(g_rg.mesh, (unsigned)x, (unsigned)y);
    if (m >= groups || m >= RG_MATERIALS) return 0;
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
    if (SUCCEEDED(ID3D11Device_CreateBuffer(g_d3d.dev, &bd, &sd, &c->vb))) { c->r = r; c->nr = nr; c->tris = n; g_rg.tris += n; }
    else free(r);
    free(v);
    if (water) lo[2] = hi[2] = 0;   // their z is the ground's under them, the surface lies at the level
    memcpy(c->lo, lo, sizeof lo);
    memcpy(c->hi, hi, sizeof hi);
}
internal void rg_chunk_build(int cx, int cy, rg_tri *tris) {
    rg_chunk *c = &g_rg.c[cy * g_rg.across + cx];
    unsigned groups = mesh_water_groups(g_rg.mesh);
    static uint32_t tq[RG_CHUNK * RG_CHUNK][4];
    for (int w = 0; w < 2; w++) {
        int n = 0;
        if (!w && g_rg.tq) { memset(tq, 0, sizeof tq); g_rg.tqc = tq; g_rg.tq_x0 = cx * RG_CHUNK; g_rg.tq_y0 = cy * RG_CHUNK; }
        for (int y = cy * RG_CHUNK; y < (cy + 1) * RG_CHUNK && y < g_rg.size; y++)
            for (int x = cx * RG_CHUNK; x < (cx + 1) * RG_CHUNK && x < g_rg.size; x++)
                if (g_rg.tile(g_rg.mesh, (unsigned long)x, (unsigned long)y))
                    n += w ? rg_water_quad(c, tris + n, (unsigned long)x, (unsigned long)y, groups) : rg_quad(tris + n, (unsigned long)x, (unsigned long)y);
        if (g_rg.tqc) {
            D3D11_BOX b = { (UINT)g_rg.tq_x0, (UINT)g_rg.tq_y0, 0, (UINT)(g_rg.tq_x0 + RG_CHUNK < g_rg.size ? g_rg.tq_x0 + RG_CHUNK : g_rg.size),
                            (UINT)(g_rg.tq_y0 + RG_CHUNK < g_rg.size ? g_rg.tq_y0 + RG_CHUNK : g_rg.size), 1 };
            ID3D11DeviceContext_UpdateSubresource(g_d3d.ctx, (ID3D11Resource *)g_rg.tq, 0, &b, tq, sizeof tq[0] * RG_CHUNK, 0);
            g_rg.tqc = NULL;
        }
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
internal int rg_black_wall(rb_vertex *v, int n, const float *p, const float *q) {
    float p1[3] = { p[0], p[1], p[2] - RG_CLIFF_DROP }, q1[3] = { q[0], q[1], q[2] - RG_CLIFF_DROP };
    n = rg_black_tri(v, n, p, q, q1);
    return rg_black_tri(v, n, p, q1, p1);
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
        // A space map: no black beyond its edge, the stars under and round it - its side along the game's bottom edges (the
        // front) went 1000 tiles out at the level, everything below the map black from a camera far back (the player,
        // 2026-11-23: "in space mode the top and the sides are rendered as space, while the bottom bounds are just black").
        // Land at the edge hangs a black wall the faces' drop down, the edge of the sheet it would be otherwise
        if (g_rg.space) {
            if (g_rg.water_info(g_rg.mesh, (unsigned long)x, (unsigned long)y) & RG_SPACE) continue;
            if (g_rg.right(g_rg.mesh, x, y) || g_rg.left(g_rg.mesh, x, y)) n = rg_black_wall(v, n, b, c);
            else {
                if (g_rg.top(g_rg.mesh, x, y)) n = rg_black_wall(v, n, a, d);
                if (g_rg.bottom(g_rg.mesh, x, y)) n = rg_black_wall(v, n, a, d);
            }
        }
        else if (g_rg.right(g_rg.mesh, x, y)) RG_SIDE(b, c, 1);
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
        if (g_rg.space) break;
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
// the scenario editor: what it looks at again some times a second, the rest a chunk at a time (no game's time: it is the editor)
#define RG_ED_EVERY 200
#define RG_ED_GONE 2000
internal void rg_editor(void) {
    static DWORD last;
    static int sweep;
    LONG seen = g_ws_ed_seen;
    DWORD now = GetTickCount();
    ee_camera *cam = game_camera();
    int across = g_rg.across;
    if (!seen || !cam || across <= 0 || across > RG_ACROSS || (LONG)(now - (DWORD)seen) > RG_ED_GONE || now - last < RG_ED_EVERY) return;
    last = now;
    const float *at = camera_look_at(cam);
    int cx = (int)at[0] / RG_CHUNK, cy = (int)at[1] / RG_CHUNK;
    for (int y = cy - 1; y <= cy + 1; y++) for (int x = cx - 1; x <= cx + 1; x++)
        if (x >= 0 && y >= 0 && x < across && y < across) g_rg_dirty[y * RG_ACROSS + x] = 1;
    g_rg_dirty[sweep / across * RG_ACROSS + sweep % across] = 1;
    sweep = (sweep + 1) % (across * across);
    g_rg_any = 1;
}
// the marked chunks made again, and the black round the map with them (its inner points are the ground's)
internal void rg_rebuild(void) {
    g_rg_any = 0;
    int gone = 0, again = 0;
    for (int m = 0; m < g_rg.nmat; m++) {
        if (!g_rg_gone[m]) continue;
        g_rg_gone[m] = 0;
        gone++;
        for (int k = 0; k < g_rg.across * g_rg.across; k++)
            for (int r = 0; r < g_rg.c[k].p[0].nr; r++)
                if (g_rg.c[k].p[0].r[r].m == m) { again += !g_rg_dirty[k / g_rg.across * RG_ACROSS + k % g_rg.across]; g_rg_dirty[k / g_rg.across * RG_ACROSS + k % g_rg.across] = 1; break; }
    }
    if (gone) log_locked("# ground: %d textures let go by their terrain's set, %d chunks drawn with them made again\n", gone, again);
    rg_tri *tris = malloc(sizeof *tris * (size_t)RG_CHUNK * RG_CHUNK * 2 * RG_LAYERS);
    if (!tris) return;
    int made = 0;
    for (int cy = 0; cy < g_rg.across; cy++) for (int cx = 0; cx < g_rg.across; cx++) {
        if (!g_rg_dirty[cy * RG_ACROSS + cx]) continue;
        g_rg_dirty[cy * RG_ACROSS + cx] = 0;
        rg_chunk *c = &g_rg.c[cy * g_rg.across + cx];
        for (int w = 0; w < 2; w++) { D3D_RELEASE(c->p[w].vb); free(c->p[w].r); g_rg.tris -= c->p[w].tris; }
        memset(c, 0, sizeof *c);
        rg_chunk_build(cx, cy, tris);
        made++;
    }
    free(tris);
    if (made) {
        D3D_RELEASE(g_rg.skirt);
        g_rg.nskirt = 0;
        rg_skirt_build();
        g_rg.remade += made;
    }
}
internal int rg_build(void) {
    ee_map *map = game_map();
    if (!map || !rg_api()) return 0;
    if (g_rg.built && g_rg.map == map && !g_rg_new_map) return 1;
    g_rg_new_map = 0;
    rg_free();
    g_rg.map = map;
    g_rg.space = rg_space_drawn(g_rg.mesh);
    g_rg.size = (int)map_get_size(map);
    g_rg.across = (g_rg.size + RG_CHUNK - 1) / RG_CHUNK;
    g_rg.tris = 0;
    g_rg.c = calloc((size_t)g_rg.across * (size_t)g_rg.across, sizeof *g_rg.c);
    rg_tri *tris = malloc(sizeof *tris * (size_t)RG_CHUNK * RG_CHUNK * 2 * RG_LAYERS);
    if (!g_rg.c || !tris) { free(tris); rg_free(); return 0; }
    D3D11_TEXTURE2D_DESC qd = { (UINT)g_rg.size, (UINT)g_rg.size, 1, 1, DXGI_FORMAT_R32G32B32A32_UINT, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
    if (FAILED(ID3D11Device_CreateTexture2D(g_d3d.dev, &qd, NULL, &g_rg.tq)) || FAILED(ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)g_rg.tq, NULL, &g_rg.tq_srv)))
        D3D_RELEASE(g_rg.tq);   // the borders each by its own heights
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
        D3D11_BUFFER_DESC bd = { 84 * sizeof(float), D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
        if (FAILED(ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_rg.cb)) || FAILED(ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_rg.cbw))) { rg_free(); return 0; }
    }
    g_rg.fow_was[(size_t)g_rg.size * g_rg.size] = 1;   // differs: the first frame sends it
    g_rg.built = 1;
    log_locked("# ground: the map's %d x %d tiles in %d chunks, %ld triangles, %d textures, made in %.1f ms\n", g_rg.size, g_rg.size,
               g_rg.across * g_rg.across, g_rg.tris, g_rg.nmat, (double)(rd_qpc() - t0) / (double)rd_qpc_ms(1));
    return 1;
}
// the cells' sight into the texture when it changed: 0, 1 or 2 and more as 0, a half, all. Only after a write to the world
// has ended since the last look (render_3d.c g_rwl_writes: sight changes inside one; every frame until the world's lock is
// known, before which none is counted) or the game's time has moved on (a step), or a second on: zoomed out over a 1024 map the whole map's million cells were
// read, cleared and compared every frame (the player's planets, 2026-11-23). A frame while a step writes keeps the sight
// from before it instead of a half-counted one.
#define RG_FOW_MOST_MS 1000
internal void rg_fow(void) {
    static LONG seen;
    static LONGLONG at;
    static float seen_ms;
    static int seen_ok;
    LONG writes = g_rwl_writes;
    LONGLONG now = rd_qpc();
    ee_physics *ph = game_physics();
    float clock = ph ? physics_get_time_ms(ph) : -1.0f;
    if (g_rwl_writes_on && seen_ok && writes == seen && clock == seen_ms && !g_rg.fow_was[(size_t)g_rg.size * g_rg.size] && now - at < rd_qpc_ms(RG_FOW_MOST_MS)) return;
    seen = writes, seen_ms = clock, at = now, seen_ok = 1;
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
// THE GROUND'S BORDERS ([display] ground_blend, ground_blend_noise, ground_blend_soft; the pipe's picture gb, gbnoise, gbsoft,
// gbscale; the player, 2026-11-10: a height from the colours for the blending, normalized per texture). A layer of the ground
// fades between a quad's corners, a soft gradient along the tile grid. With this its high texels come first and its low ones
// last, and noise over the map bends the border away from the grid (the textures repeat, their heights alone would follow
// it). A texture's height is the brightness of its texels, a little blurred so that it comes in clumps, and ranked: the share
// of the texture's texels darker than this one. So at any fade that share of every texture shows, bright sand and dark rock
// alike, and a texture of little contrast has the whole range too.
// All of a tile's textures are weighed together (the player, 2026-11-24, of a border of three: "is whos heightmap is used
// for blending randomly decided and not a mix of both?"): each by its share of the corners at the point and its own height
// there, and a layer covers what was drawn before it in the tile as much as it weighs against that. Each layer had gone by
// its own heights alone, over whatever lay under it: the order is the textures' addresses, so it changed from tile to tile
// where three met - the border took one texture's shape in one tile and another's in the next (straight seams along the
// grid), and two layers fading out over a third showed the third between them. Now a point's colour depends on its
// textures and their weights only, not on which is drawn first, so tiles meet. The table (g_rg.tq) gives a tile's four
// corners' materials and their order, the heights of all materials lie in one array, a slice each.
#define RGB_SIDE 256    // a slice's texels across: every texture's heights resampled to it
#define RGB_LEVELS 9    // its mip levels, down to 1 x 1
#define RGB_RELIEF 0.1f // the relief at 100%: a texture's detail from dark to bright this many tiles high (the shader's grl.x)
internal struct {
    int force, fail;     // the pipe's on / off (-1: the setting); no height could be made
    float soft, noise, scale;   // the pipe's (< 0: the settings')
    LONG made;
    LONGLONG qpc;
    float relief;                      // the pipe's (< 0: the setting's)
    ID3D11Texture2D *arr;              // the heights, a slice each material
    ID3D11ShaderResourceView *arr_srv;
    ID3D11Texture2D *nrm;              // THE RELIEF (ground_relief): the slopes, a slice each material
    ID3D11ShaderResourceView *nrm_srv;
    int cap;                           // their slices
    ID3D11Buffer *inv;                 // g_rg.inv_n for the shader
    ID3D11ShaderResourceView *inv_srv;
    ID3D11SamplerState *samp;          // the heights read repeating, at any place of the map
} g_rgb = { .force = -1, .soft = -1, .noise = -1, .scale = 1.3f, .relief = -1 };
// the array, the materials' shares and the sampler, made once; 0: they can't be (the borders off)
internal int rgb_ready(void) {
    if (g_rgb.samp) return 1;
    D3D11_BUFFER_DESC bd = { RG_MATERIALS * sizeof(float), D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, 0 };
    D3D11_SHADER_RESOURCE_VIEW_DESC vd = { .Format = DXGI_FORMAT_R32_FLOAT, .ViewDimension = D3D11_SRV_DIMENSION_BUFFER };
    vd.Buffer.FirstElement = 0;
    vd.Buffer.NumElements = RG_MATERIALS;
    D3D11_SAMPLER_DESC sd = { D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP,
                              0, 1, D3D11_COMPARISON_NEVER, { 0, 0, 0, 0 }, 0, D3D11_FLOAT32_MAX };
    if (g_rgb.inv || (SUCCEEDED(ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_rgb.inv))
                      && SUCCEEDED(ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)g_rgb.inv, &vd, &g_rgb.inv_srv))))
        if (SUCCEEDED(ID3D11Device_CreateSamplerState(g_d3d.dev, &sd, &g_rgb.samp))) { g_rg.inv_dirty = 1; return 1; }
    g_rgb.fail = 1;
    log_locked("# ground: the borders' buffers could not be made: its borders as the game's\n");
    return 0;
}
// an array of cap slices of this format, what *arr held copied over; 0: it can't be made
internal int rgb_grow(ID3D11Texture2D **arr, ID3D11ShaderResourceView **srv, DXGI_FORMAT f, int cap) {
    D3D11_TEXTURE2D_DESC td = { RGB_SIDE, RGB_SIDE, RGB_LEVELS, (UINT)cap, f, { 1, 0 }, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0 };
    ID3D11Texture2D *a = NULL;
    ID3D11ShaderResourceView *v = NULL;
    if (FAILED(ID3D11Device_CreateTexture2D(g_d3d.dev, &td, NULL, &a)) || FAILED(ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)a, NULL, &v))) {
        D3D_RELEASE(a);
        return 0;
    }
    for (int s = 0; s < g_rgb.cap; s++)
        for (int l = 0; l < RGB_LEVELS; l++)
            ID3D11DeviceContext_CopySubresourceRegion(g_d3d.ctx, (ID3D11Resource *)a, (UINT)(s * RGB_LEVELS + l), 0, 0, 0, (ID3D11Resource *)*arr, (UINT)(s * RGB_LEVELS + l), NULL);
    D3D_RELEASE(*srv);
    D3D_RELEASE(*arr);
    *arr = a;
    *srv = v;
    return 1;
}
// room for material m's slices: the arrays grow
internal int rgb_slice(int m) {
    if (m < g_rgb.cap) return 1;
    int cap = g_rgb.cap ? g_rgb.cap : 16;
    while (cap <= m) cap *= 2;
    if (cap > RG_MATERIALS) cap = RG_MATERIALS;
    if (!rgb_grow(&g_rgb.arr, &g_rgb.arr_srv, DXGI_FORMAT_R8_UNORM, cap) || !rgb_grow(&g_rgb.nrm, &g_rgb.nrm_srv, DXGI_FORMAT_R16G16_SNORM, cap)) return 0;
    g_rgb.cap = cap;
    return 1;
}
internal int rgb_on(void) { return !g_rgb.fail && (g_rgb.force < 0 ? g_set.ground_blend : g_rgb.force); }
internal float rgb_soft(void) { float v = g_rgb.soft >= 0 ? g_rgb.soft : (float)g_set.ground_blend_soft; return v < 0.02f ? 0.02f : v; }
internal float rgb_noise(void) { return g_rgb.noise >= 0 ? g_rgb.noise : (float)g_set.ground_blend_noise; }
internal float rgb_relief(void) { return g_rgb.relief >= 0 ? g_rgb.relief : (float)g_set.ground_relief; }
// the textures' heights and slopes are wanted: for the borders or the relief
internal int rgb_wanted(void) { return !g_rgb.fail && (rgb_on() || rgb_relief() > 0); }
// box blur of a w x h field round, r texels each way, along x then y (it repeats: so does this); t a field's room
internal void rgb_box(float *f, float *t, int w, int h, int r) {
    for (int y = 0; y < h; y++) {
        float s = 0;
        for (int k = -r; k <= r; k++) s += f[y * w + (k % w + w) % w];
        for (int x = 0; x < w; x++) {
            t[y * w + x] = s / (float)(2 * r + 1);
            s += f[y * w + (x + r + 1) % w] - f[y * w + ((x - r) % w + w) % w];
        }
    }
    for (int x = 0; x < w; x++) {
        float s = 0;
        for (int k = -r; k <= r; k++) s += t[((k % h + h) % h) * w + x];
        for (int y = 0; y < h; y++) {
            f[y * w + x] = s / (float)(2 * r + 1);
            s += t[((y + r + 1) % h) * w + x] - t[(((y - r) % h + h) % h) * w + x];
        }
    }
}
// THE RELIEF: a texture's slopes into material mat's slice. Its detail is its brightness less what a wide blur keeps (its
// patches of colour: a dark patch of grass is no hollow), blurred against texel noise, resampled to the slice; a
// slope per texel of the slice along u and v (x and y of the map), times 4 into the format's -1..1. The shader turns it
// into one per tile by the texture's repeat (render_3d.c, grl)
internal void rgb_slopes(const uint32_t *px, int w, int h, int mat) {
    size_t n = (size_t)w * h;
    float *f = malloc(n * sizeof *f), *lo = malloc(n * sizeof *f), *t = malloc(n * sizeof *f);
    float *r = malloc((size_t)RGB_SIDE * RGB_SIDE * sizeof *r);
    int16_t *o = malloc((size_t)RGB_SIDE * RGB_SIDE * 4), *q = malloc((size_t)RGB_SIDE * RGB_SIDE * 2);
    if (f && lo && t && r && o && q) {
        for (size_t i = 0; i < n; i++)
            lo[i] = f[i] = (float)(((px[i] >> 16) & 255) * 77 + ((px[i] >> 8) & 255) * 150 + (px[i] & 255) * 29) / (255.0f * 256.0f);
        int wr = (w > h ? w : h) / 16;
        rgb_box(lo, t, w, h, wr < 2 ? 2 : wr);
        for (size_t i = 0; i < n; i++) f[i] -= lo[i];
        rgb_box(f, t, w, h, 2);   // twice: about a bell 1.6 texels wide - a texel's noise on grass caught the evening sun as sparks
        rgb_box(f, t, w, h, 2);
        for (int y = 0; y < RGB_SIDE; y++) for (int x = 0; x < RGB_SIDE; x++) {
            float fx = ((float)x + 0.5f) * (float)w / RGB_SIDE - 0.5f, fy = ((float)y + 0.5f) * (float)h / RGB_SIDE - 0.5f;
            int x0 = (int)floorf(fx), y0 = (int)floorf(fy);
            float ax = fx - (float)x0, ay = fy - (float)y0;
            int xa = (x0 % w + w) % w, xb = (xa + 1) % w, ya = (y0 % h + h) % h, yb = (ya + 1) % h;
            r[y * RGB_SIDE + x] = (f[ya * w + xa] * (1 - ax) + f[ya * w + xb] * ax) * (1 - ay) + (f[yb * w + xa] * (1 - ax) + f[yb * w + xb] * ax) * ay;
        }
        for (int y = 0; y < RGB_SIDE; y++) for (int x = 0; x < RGB_SIDE; x++) {
            float gx = (r[y * RGB_SIDE + (x + 1) % RGB_SIDE] - r[y * RGB_SIDE + (x + RGB_SIDE - 1) % RGB_SIDE]) * 0.5f;
            float gy = (r[((y + 1) % RGB_SIDE) * RGB_SIDE + x] - r[((y + RGB_SIDE - 1) % RGB_SIDE) * RGB_SIDE + x]) * 0.5f;
            gx = gx * 4 < -1 ? -1 : gx * 4 > 1 ? 1 : gx * 4;
            gy = gy * 4 < -1 ? -1 : gy * 4 > 1 ? 1 : gy * 4;
            o[(y * RGB_SIDE + x) * 2] = (int16_t)lrintf(gx * 32767.0f);
            o[(y * RGB_SIDE + x) * 2 + 1] = (int16_t)lrintf(gy * 32767.0f);
        }
        int mw = RGB_SIDE;
        int16_t *p = o;
        for (int lv = 0; lv < RGB_LEVELS; lv++) {
            ID3D11DeviceContext_UpdateSubresource(g_d3d.ctx, (ID3D11Resource *)g_rgb.nrm, (UINT)(mat * RGB_LEVELS + lv), NULL, p, (UINT)mw * 4, 0);
            if (mw == 1) break;
            int nw = mw / 2;
            int16_t *d = p == o ? q : p + (size_t)mw * mw * 2;
            for (int y = 0; y < nw; y++) for (int x = 0; x < nw; x++) for (int c = 0; c < 2; c++)
                d[(y * nw + x) * 2 + c] = (int16_t)((p[(2 * y * mw + 2 * x) * 2 + c] + p[(2 * y * mw + 2 * x + 1) * 2 + c] + p[((2 * y + 1) * mw + 2 * x) * 2 + c]
                                                    + p[((2 * y + 1) * mw + 2 * x + 1) * 2 + c]) / 4);
            p = d; mw = nw;
        }
    }
    free(f); free(lo); free(t); free(r); free(o); free(q);
}
// a texture's heights into material mat's slice, with their mip levels; 0: they can't be made
internal int rgb_height(rd_surface *t, int mat) {
    LONGLONG t0 = rd_qpc();
    if (t->top) t = t->top;
    uint8_t *m = rd_mem(t);
    int w = (int)t->desc.dwWidth, h = (int)t->desc.dwHeight;
    size_t n = (size_t)w * h;
    uint32_t *px = malloc(n * 4);
    uint16_t *a = malloc(n * 2), *b = malloc(n * 2);
    uint8_t *r = malloc((size_t)RGB_SIDE * RGB_SIDE);
    uint8_t *o = malloc((size_t)RGB_SIDE * RGB_SIDE * 2);
    int made = 0;
    if (m && w > 0 && h > 0 && px && a && b && r && o && rgb_slice(mat)) {
        for (int y = 0; y < h; y++) rf_to_bgra(t->fmt, m + (size_t)y * t->desc.lPitch, px + (size_t)y * w, w, 0, 0);
        for (size_t i = 0; i < n; i++) a[i] = (uint16_t)((((px[i] >> 16) & 255) * 77 + ((px[i] >> 8) & 255) * 150 + (px[i] & 255) * 29));
        for (int pass = 0; pass < 2; pass++) {   // twice three texels each way, over the texture's edges: it repeats
            for (int y = 0; y < h; y++) for (int x = 0; x < w; x++)
                b[y * w + x] = (uint16_t)((a[y * w + (x + w - 1) % w] + (uint32_t)a[y * w + x] + a[y * w + (x + 1) % w]) / 3);
            for (int y = 0; y < h; y++) for (int x = 0; x < w; x++)
                a[y * w + x] = (uint16_t)((b[(y + h - 1) % h * w + x] + (uint32_t)b[y * w + x] + b[(y + 1) % h * w + x]) / 3);
        }
        static uint32_t count[4096];
        memset(count, 0, sizeof count);
        for (size_t i = 0; i < n; i++) count[a[i] >> 4]++;
        static uint8_t rank[4096];
        uint32_t below = 0;
        for (int k = 0; k < 4096; k++) { rank[k] = (uint8_t)((below + count[k] / 2) * 255ull / n); below += count[k]; }
        for (size_t i = 0; i < n; i++) b[i] = rank[a[i] >> 4];
        // to the slice's size, between the texels round (the texture repeats: so does this)
        for (int y = 0; y < RGB_SIDE; y++) for (int x = 0; x < RGB_SIDE; x++) {
            float fx = ((float)x + 0.5f) * (float)w / RGB_SIDE - 0.5f, fy = ((float)y + 0.5f) * (float)h / RGB_SIDE - 0.5f;
            int x0 = (int)floorf(fx), y0 = (int)floorf(fy);
            float ax = fx - (float)x0, ay = fy - (float)y0;
            int xa = (x0 % w + w) % w, xb = (xa + 1) % w, ya = (y0 % h + h) % h, yb = (ya + 1) % h;
            float v = ((float)b[ya * w + xa] * (1 - ax) + (float)b[ya * w + xb] * ax) * (1 - ay) + ((float)b[yb * w + xa] * (1 - ax) + (float)b[yb * w + xb] * ax) * ay;
            r[y * RGB_SIDE + x] = (uint8_t)(v + 0.5f);
        }
        int mw = RGB_SIDE;
        uint8_t *p = r;
        for (int lv = 0; lv < RGB_LEVELS; lv++) {
            ID3D11DeviceContext_UpdateSubresource(g_d3d.ctx, (ID3D11Resource *)g_rgb.arr, (UINT)(mat * RGB_LEVELS + lv), NULL, p, (UINT)mw, 0);
            if (mw == 1) break;
            int nw = mw / 2;
            uint8_t *q = p == r ? o : p + (size_t)mw * mw;
            for (int y = 0; y < nw; y++) for (int x = 0; x < nw; x++)
                q[y * nw + x] = (uint8_t)((p[2 * y * mw + 2 * x] + p[2 * y * mw + 2 * x + 1] + p[(2 * y + 1) * mw + 2 * x] + p[(2 * y + 1) * mw + 2 * x + 1] + 2) / 4);
            p = q; mw = nw;
        }
        rgb_slopes(px, w, h, mat);
        made = 1;
    }
    free(px); free(a); free(b); free(r); free(o);
    if (made) { g_rgb.made++; g_rgb.qpc += rd_qpc() - t0; }
    else { g_rgb.fail = 1; log_locked("# ground: a texture's heights could not be made (%d x %d): its borders as the game's\n", w, h); }
    return made;
}
// the pipe: picture gb on | off, gbsoft <0.02..1>, gbnoise <0..1>, gbscale <per tile>, relief <0..4> (1 is 100%)
internal int rgb_pipe(char *out, const char *mode, const char *arg) {
    if (!strcmp(mode, "relief")) {
        float e = (float)atof(arg);
        if (arg[0] && (e < 0 || e > 4)) { REPLY("error: usage: picture relief <0..4>, 1 is 100%%\n"); return 1; }
        if (arg[0]) g_rgb.relief = e;
        REPLY("the ground's relief %.0f%%\n", (double)rgb_relief() * 100.0);
        return 1;
    }
    if (!strcmp(mode, "gb")) {
        if (!strcmp(arg, "on") || !strcmp(arg, "off")) g_rgb.force = arg[1] == 'n';
        else if (arg[0]) { REPLY("error: usage: picture gb on | off\n"); return 1; }
    } else if (!strcmp(mode, "gbsoft") || !strcmp(mode, "gbnoise") || !strcmp(mode, "gbscale")) {
        float e = (float)atof(arg);
        if (!arg[0] || e < 0 || e > 16) { REPLY("error: usage: picture %s <0..16>\n", mode); return 1; }
        *(mode[3] == 'o' ? &g_rgb.soft : mode[2] == 'n' ? &g_rgb.noise : &g_rgb.scale) = e;
    } else return 0;
    REPLY("the ground's borders %s, %ld textures' heights made in %.1f ms, soft %.2f, noise %.2f at %.2f a tile\n", rgb_on() ? "on" : "off", (long)g_rgb.made,
          (double)g_rgb.qpc / (double)rd_qpc_ms(1), (double)rgb_soft(), (double)rgb_noise(), (double)g_rgb.scale);
    return 1;
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
// no fog of war on the ground (the game's call of UpdateFogOfWar, 0x503A7F's caller): the player sees all, the free camera, godmode (godmode.c god_see_all)
internal int rg_revealed(void) {
    ee_player *p = game_players()[local_player_index()];
    return god_see_all() || (p && player_get_explore_all(p));
}
// THE WATER BY THE GAME TIME ([display] water_game_time, the player 2026-11-10: "water surface and caustics should probably
// respect game time?", "add it as an option"). The game moves its water by the real time, paused or not: the wobble of the
// seabed and the surface (the exe's ground update 0x503A7F hands TransformToCamera the TMGetTime gone) and the frames of its
// animation (GETerrainMesh::UpdateWaterAnimation: TMGetTime) - paused, the game's own ground and water drawn (picture ground
// off), 37% of the water's pixels changed in 2 s. With the setting the phase of the wobble (the caustics run on it) goes by
// the game time, and so do the frames: the game's water groups hold the animation's textures in turn (group j frame
// (counter + j) % groups), so a group drawn with the texture of group j + ours - the game's counter shows frame ours + j,
// ours counted in the game's ms a frame. The game time: the real time gone at the rate the game time runs (collect_copy.c's
// measure of the steps), none while paused; without a measure in the last second the game's speed setting.
internal float g_cp_rate;          // collect_copy.c, included after this file
internal LONGLONG g_cp_clock_at;
internal double rg_water_ms(double real_ms) {
    if (!g_set.water_game_time) return real_ms;
    if (game_paused()) return 0;
    float r = g_cp_rate;
    if (!(r > 0.0f) || !g_cp_clock_at || rd_qpc() - g_cp_clock_at > rd_qpc_ms(1000)) r = *game_speed_var();
    return r > 0.0f && r < 100.0f ? real_ms * (double)r : real_ms;
}
// the group whose texture the water's group 0 is drawn with now (+ j for group j, round the groups)
internal unsigned rg_water_frame(void) {
    unsigned n = mesh_water_groups(g_rg.mesh), per = n ? mesh_water_frame_ms(g_rg.mesh) : 0;
    if (g_rw_still && n) return (n - mesh_water_frame(g_rg.mesh) % n) % n;   // the pipe's water still: group j frame j
    if (!g_set.water_game_time || !per) return 0;
    unsigned ours = (unsigned)fmod(floor(g_rg.wms / (double)per), (double)n);
    return (ours + n - mesh_water_frame(g_rg.mesh) % n) % n;
}
// THE WATER'S FRAMES BLENDED (the player, 2026-11-13: "the reflection disortion moves for half a second than stops", then
// "it abselutly moves, but only every second for half a second" - "yes, blend the games water texture"): the animation is
// a few textures shown in turn, one a frame's ms (100 by the game's time at speed 1); at speed 0.25 by the game time each
// stood a second, then the water and the reflections it wobbles (rfa.y) jumped. A recording at that speed with the
// player's ini: the water and a ship's reflection alike, a change of 0.1 a frame and 4 - 6 at each switch. A group's next
// frame is the next group's texture now (group j shows frame (counter + j) % groups), so each water draw also gets the
// next group's texture (rf_cmd gwn, t14) and the shader blends the two by how far the frame is through: by the water's own
// time with [display] water_game_time, else by the real time since the game's counter changed.
internal float rg_water_blend(void) {
    unsigned per = mesh_water_frame_ms(g_rg.mesh), n = mesh_water_groups(g_rg.mesh);
    if (!g_rw_blend || g_rw_still || !per || n < 2) return 0;
    double a;
    if (g_set.water_game_time) a = g_rg.wms / (double)per, a -= floor(a);
    else {
        unsigned f = mesh_water_frame(g_rg.mesh);
        LONGLONG now = rd_qpc();
        if (f != g_rg.wf_seen || !g_rg.wf_at) g_rg.wf_seen = f, g_rg.wf_at = now;
        a = (double)(now - g_rg.wf_at) / (double)rd_qpc_ms(1) / (double)per;
    }
    return a < 0 ? 0.0f : a > 1 ? 1.0f : (float)a;
}
// EACH TEXTURE ITS OWN STAGE (the player, 2026-11-23: "the terrain textures becomes nearest sampled instead of smooth
// when space is added"). The display DLLs' SetTexture sets stage 0 from the texture's own fields - its colour and alpha
// operations, addressing, filters (AoC's DX7Rasterizer::SetTexture 0x1000BF2D) - and the game's Draw binds each
// material before its polygons. The ground here bound them all first and took the device's states once: the last
// texture's for every one. The ground's textures share theirs, so it never showed until a space map's textures (their
// own filters) came last. Now each material's states are kept as its bind leaves them and set with its draws - in a
// frame during a step too (nothing is bound then: the last real frame's). Measured on Small Planets: 68 of the ground's
// textures filter linearly (MAG and MIN LINEAR, MIP LINEAR), the 12 space textures by points (POINT, POINT) - the game
// draws its cliffs into space blocky up close. Here a point filter is taken as linear: the faces as smooth as the ground.
internal void *rd3_sampler_of(const DWORD *ts, int clamp);
internal void rg_stage_take(const rd_device *d, int water, int m) {
    if (m < 0 || m >= RG_MATERIALS) return;
    DWORD ts[RD_TSS];
    memcpy(ts, d->tss[0], sizeof ts);
    if (ts[D3DTSS_MAGFILTER] == D3DTFG_POINT) ts[D3DTSS_MAGFILTER] = D3DTFG_LINEAR;
    if (ts[D3DTSS_MINFILTER] == D3DTFN_POINT) ts[D3DTSS_MINFILTER] = D3DTFN_LINEAR;
    if (ts[D3DTSS_MIPFILTER] == D3DTFP_POINT) ts[D3DTSS_MIPFILTER] = D3DTFP_LINEAR;
    g_rg.msamp[water][m] = rd3_sampler_of(ts, 0);
    int32_t *o = g_rg.mop[water][m];
    o[0] = (int32_t)ts[D3DTSS_COLOROP]; o[1] = (int32_t)ts[D3DTSS_COLORARG1]; o[2] = (int32_t)ts[D3DTSS_COLORARG2];
    o[3] = (int32_t)ts[D3DTSS_ALPHAOP]; o[4] = (int32_t)ts[D3DTSS_ALPHAARG1]; o[5] = (int32_t)ts[D3DTSS_ALPHAARG2];
}
internal void rg_stage_use(int water, int m) {
    if (m < 0 || m >= RG_MATERIALS || !g_rg.msamp[water][m]) return;
    g_rd3.samp = g_rg.msamp[water][m];
    const int32_t *o = g_rg.mop[water][m];
    if (memcmp(g_rd3.pc.op, o, 3 * sizeof *o) || memcmp(g_rd3.pc.aop, o + 3, 3 * sizeof *o)) {
        memcpy(g_rd3.pc.op, o, 3 * sizeof *o);
        memcpy(g_rd3.pc.aop, o + 3, 3 * sizeof *o);
        g_rf.pc_new = 1;   // the next draw's constants a snapshot of their own
    }
}
internal int rg_ready(void) { return g_rg.on && g_rg.lit && g_rg.api >= 0 && g_rd3.scene_view[15] == 1 && g_rd3.prep_dev && g_cam_now_ok && game_map(); }   // the units have drawn once: the projection, from the frame before
// a target or depth buffer freed (render_frame.c rf_target_gone): the states kept with it are none until a frame draws
// anew; how many of the two (the ground's, the water's) drew on it
internal int rg_target_gone(ID3D11RenderTargetView *rtv, ID3D11DepthStencilView *dsv) {
    int n = 0;
    for (int w = 0; w < 2; w++)
        if (g_rg.st[w].ok && ((rtv && g_rg.st[w].rtv == rtv) || (dsv && g_rg.st[w].dsv == dsv))) g_rg.st[w].ok = 0, n++;
    return n;
}
// the ground (water 0) or the water of this frame into the list, on the game's states for it. again: 1 a frame during a step,
// 2 one while another writer has the world - nothing of the game's is read then, a load is freeing it
// the display DLL's texture bind, with the renderer's lock let go meanwhile. The bind takes the display's texture lock
// and may load the texture; the game's loader thread takes that lock first and then makes surfaces, which take the
// renderer's (render.c rd_dd_create_surface) - held here across the bind, each waited for the other: the player's game
// stood for good at 38 min (2026-11-16: the render thread in the bind, the loader in CreateSurface, the simulation
// waiting for that model's load, the main thread for the world). Let go, the locks are always taken texture-first
internal void rg_bind_unlocked(int i, void *tex) {
    LONG depth = g_rd_cs.OwningThread == (HANDLE)(uintptr_t)GetCurrentThreadId() ? g_rd_cs.RecursionCount : 0;
    for (LONG k = 0; k < depth; k++) LeaveCriticalSection(&g_rd_cs);
    g_rg.bind[i](g_rg.ras, tex, g_rg.screen);
    for (LONG k = 0; k < depth; k++) EnterCriticalSection(&g_rd_cs);
}
internal void rg_draw_now(int water, int again) {
    rd_device *d = g_rd3.prep_dev;
    if (!g_rg.ras || !rg_build()) return;
    int i = g_rg.dll;
    rg_states *st = &g_rg.st[water];
    if (again && !st->ok) return;
    if (!water && !again) { rg_editor(); if (g_rg_any) rg_rebuild(); }
    rd3_flush();
    // the projection the game's draw would set: the viewport's (+0x1F0, a GETransformation by rows; +0x1B0 the fixed camera's)
    const float *gp = (const float *)(*(const uint8_t **)(*(const uintptr_t *)((const uint8_t *)g_rg.ras + 0x20) + g_rg.screen * 4) + (g_rg.flag ? 0x1B0 : 0x1F0));
    // (in a frame during a step the current viewport is the interface's: the projection of the last frame that drew the ground)
    float proj[16], vpm[16];
    if (again) memcpy(proj, st->proj, sizeof proj);
    else for (int r = 0; r < 4; r++) for (int c = 0; c < 4; c++) proj[r * 4 + c] = gp[c * 4 + r];
    rd3_mul(vpm, g_cam_view_now, proj);
    rf_depth_vpm(vpm);   // the frame's depths: a chunk between them and the step's is in the view (render_frame.c)
    ID3D11ShaderResourceView **srv = g_rg.srv[water];
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
    if (!water) {   // the overlay's picture: the visible chunks' tiles (render_overlay.c ro_ground_map draws it)
        int across = g_rg.across, x0 = across, y0 = across, x1 = -1, y1 = -1;
        for (int k = 0; k < nc; k++)
            if (vis[k]) {
                int cx = k % across, cy = k / across;
                if (cx < x0) x0 = cx;
                if (cy < y0) y0 = cy;
                if (cx > x1) x1 = cx;
                if (cy > y1) y1 = cy;
            }
        g_rg.ov_on = 0;
        if (x1 >= 0) {   // the overlay's shapes outside them are left out (render_overlay.c ro_cull)
            g_rg.vis_t[0] = (float)(x0 * RG_CHUNK); g_rg.vis_t[1] = (float)(y0 * RG_CHUNK);
            g_rg.vis_t[2] = (float)((x1 + 1) * RG_CHUNK); g_rg.vis_t[3] = (float)((y1 + 1) * RG_CHUNK);
            g_rg.vis_at = g_rd_frames;
        }
        if (g_ov.on && !g_ov.flat && x1 >= 0) {
            int tx0 = x0 * RG_CHUNK, ty0 = y0 * RG_CHUNK, tx1 = (x1 + 1) * RG_CHUNK, ty1 = (y1 + 1) * RG_CHUNK;
            if (tx1 > g_rg.size) tx1 = g_rg.size;
            if (ty1 > g_rg.size) ty1 = g_rg.size;
            int ex = tx1 - tx0, ey = ty1 - ty0, e = ex > ey ? ex : ey;
            if (e > 0) {
                float ppt = RG_OV_PPT;
                if ((float)e * ppt > (float)RG_OV_SIDE) ppt = (float)RG_OV_SIDE / (float)e;
                g_rg.ov[0] = (float)tx0; g_rg.ov[1] = (float)ty0;
                g_rg.ov[2] = floorf((float)ex * ppt); g_rg.ov[3] = floorf((float)ey * ppt);
                g_rg.ov_ppt = ppt;
                g_rg.ov_on = 1;
            }
        }
    }
    int nmat = again == 2 ? 0 : water ? (int)mesh_water_groups(g_rg.mesh) : g_rg.nmat;
    if (nmat > RG_MATERIALS) nmat = RG_MATERIALS;
    if (!water) want[0] = 1;   // the black round the map: any texture, its colour is 0
    unsigned wg = water ? mesh_water_groups(g_rg.mesh) : 0;   // the water's groups: each one's next frame is the next one's texture
    if (wg > RG_MATERIALS) wg = RG_MATERIALS;
    float wblend = wg > 1 ? rg_water_blend() : 0.0f;
    if (wblend > 0 && nmat == (int)wg) {
        static uint8_t next[RG_MATERIALS];
        memset(next, 0, sizeof next);
        for (int m = 0; m < nmat; m++) if (want[m]) next[(m + 1) % nmat] = 1;
        for (int m = 0; m < nmat; m++) want[m] |= next[m];
    }
    unsigned wf = water && nmat ? rg_water_frame() : 0, wn = water && nmat ? mesh_water_groups(g_rg.mesh) : 1;
    for (int m = 0; m < nmat; m++) {
        if (!want[m]) continue;
        rg_bind_unlocked(i, water ? mesh_water_texture(g_rg.mesh, ((unsigned)m + wf) % wn) : g_rg.mat[m]);
        rg_stage_take(d, water, m);
        rd_surface *t = d->tex[0];
        if (!t) continue;
        ID3D11ShaderResourceView *now = rd_gpu_texture(t, 0);
        (t->top ? t->top : t)->rf_used = g_rd_epoch;
        if (now && now != srv[m]) {
            ID3D11ShaderResourceView_AddRef(now);
            D3D_RELEASE(srv[m]);
            srv[m] = now;
        }
        if (!water && rgb_wanted() && rgb_ready() && !g_rg.hmade[m]) g_rg.hmade[m] = (uint8_t)rgb_height(t, m);
    }
    if (!water && g_rg.inv_dirty && g_rgb.inv) {
        ID3D11DeviceContext_UpdateSubresource(g_d3d.ctx, (ID3D11Resource *)g_rgb.inv, 0, NULL, g_rg.inv_n, 0, 0);
        g_rg.inv_dirty = 0;
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
        if (again != 2) rg_fow();
        LONGLONG now = rd_qpc();
        if (g_rg.at) {
            double ms = rg_water_ms((double)(now - g_rg.at) * 1000.0 / (double)rd_qpc_ms(1000));
            g_rg.phase = fmod(g_rg.phase + (double)ws[0] * ms / 1000.0, 6.283185307179586 * 1024.0);
            g_rg.wms += ms;
            g_rw_ctime = fmod(g_rw_ctime + (double)ws[0] * ms / 1000.0 * 0.5 * (double)g_set.water_caustics_speed, 6.283185307179586 * 1024.0);
        }
        g_rg.at = now;
    }
    if (again != 2) st->revealed = rg_revealed() ? 1.0f : 0.0f;
    // the wobble's strength: the surface's, the seabed's - none with [display] seabed_wobble off (the player, 2026-11-10:
    // "add an option to disable the terrain wobble underwater that tries to fake refraction")
    float wstr = g_rw_still ? 0.0f : ws[1];   // the surface's movement (the pipe's water still: none)
    float cb[84] = { water ? 2.0f : 1.0f, (float)g_rg.phase, water ? wstr : g_set.seabed_wobble ? ws[2] : 0.0f, st->revealed };
    int blend = !water && rgb_on();
    if (blend) { cb[72] = 1; cb[73] = rgb_soft(); cb[74] = rgb_noise(); cb[75] = g_rgb.scale; }
    // the relief (grl): a full brightness's height in tiles (RGB_RELIEF at 100%), the slices' texels across
    if (!water && rgb_wanted() && g_rgb.nrm_srv) { cb[80] = rgb_relief() * RGB_RELIEF; cb[81] = RGB_SIDE; }
    if (water) cb[73] = wblend;   // the water's: its frames' blend (gbl.y; its borders' gbl.x stays 0)
    if (g_rg.ov_on) {   // the overlay's picture (gov): the map's place -> its texture's
        cb[76] = g_rg.ov[0]; cb[77] = g_rg.ov[1];
        cb[78] = cb[79] = g_rg.ov_ppt / (float)RG_OV_SIDE;
    }
    memcpy(cb + 4, g_rg.lights.amb, sizeof g_rg.lights.amb);
    cb[7] = (float)g_rg.lights.n;
    for (int l = 0; l < g_rg.lights.n; l++) { memcpy(cb + 8 + l * 4, g_rg.lights.dir[l], 12); memcpy(cb + 40 + l * 4, g_rg.lights.col[l], 12); }
    ID3D11DeviceContext_UpdateSubresource(g_d3d.ctx, (ID3D11Resource *)(water ? g_rg.cbw : g_rg.cb), 0, NULL, cb, 0, 0);
    long drawn = 0;
    for (int k = 0; k < nc; k++) {
        if (!vis[k]) continue;
        const rg_part *c = &g_rg.c[k].p[water];
        if (water) { g_sh.water_z -= g_rg.c[k].wshore * wstr * 0.5; g_sh.water_n += g_rg.c[k].wn; }   // the level, as the game's vertices give it (its mean: the sines cancel)
        for (int r = 0; r < c->nr; r++) {
            if (!srv[c->r[r].m]) continue;
            g_rd3.srv = srv[c->r[r].m];
            rg_stage_use(water, c->r[r].m);
            rf_record((UINT)c->r[r].first, (UINT)c->r[r].n, 0);
            if (g_rf.n) {
                g_rf.cmd[g_rf.n - 1].gvb = c->vb;
                if (wblend > 0 && (unsigned)c->r[r].m < wg) {   // none bound (a frame during a step binds nothing): the frame itself, no blend
                    ID3D11ShaderResourceView *nx = srv[((unsigned)c->r[r].m + 1) % wg];
                    g_rf.cmd[g_rf.n - 1].gwn = nx ? nx : srv[c->r[r].m];
                }
            }
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
    ID3D11DeviceContext_PSSetConstantBuffers(x, 3, 1, c->ro == RO_WATER ? &g_rg.cbw : &g_rg.cb);
    ID3D11DeviceContext_PSSetShaderResources(x, 13, 1, &g_rgb.arr_srv);
    ID3D11DeviceContext_PSSetShaderResources(x, 14, 1, &c->gwn);
    ID3D11DeviceContext_PSSetShaderResources(x, 15, 1, &g_rg_ov_srv);
    ID3D11ShaderResourceView *bt[3] = { g_rg.tq_srv, g_rgb.inv_srv, g_rgb.nrm_srv }, *nbt[3] = { NULL, NULL, NULL };
    ID3D11DeviceContext_PSSetShaderResources(x, 18, 3, bt);
    ID3D11DeviceContext_PSSetSamplers(x, 3, 1, &g_rgb.samp);
    ID3D11DeviceContext_Draw(x, c->n, c->at);
    ID3D11DeviceContext_IASetVertexBuffers(x, 0, 1, &g_rd3.vb, &st, &off);
    ID3D11DeviceContext_VSSetConstantBuffers(x, 3, 1, &none);
    ID3D11DeviceContext_VSSetShaderResources(x, 7, 1, &nos);
    ID3D11DeviceContext_PSSetConstantBuffers(x, 3, 1, &none);
    ID3D11DeviceContext_PSSetShaderResources(x, 13, 1, &nos);
    ID3D11DeviceContext_PSSetShaderResources(x, 14, 1, &nos);
    ID3D11DeviceContext_PSSetShaderResources(x, 15, 1, &nos);
    ID3D11DeviceContext_PSSetShaderResources(x, 18, 3, nbt);
}
internal int g_rw_fx_only;   // a frame during a step: the water's draw for its effects alone (render_3d.c rd_hook_ro_water)
internal int rg_hook(int i, int water, void *ras, void *mesh, unsigned screen, int flag) {
    if (water && g_rw_fx_only) return 0;
    int r = 0;
    if (!rg_ready() || !rg_api()) r = (water ? g_rg.water : g_rg.draw)[i](ras, mesh, screen, flag);
    else {
        if (g_rg.built && rg_space_drawn(mesh) != g_rg.space) g_rg_new_map = 1;   // the editor's first space tile (its textures there), or its last gone: all of it anew
        g_rg.ras = ras; g_rg.mesh = mesh; g_rg.screen = screen; g_rg.dll = i; g_rg.flag = flag & 0xFF;
        RD_LOCK(); rg_draw_now(water, 0); RD_UNLOCK();
    }
    if (water && !g_kg_skip) { RD_LOCK(); rd3_flush(); g_kw_surface = g_kw.nb; RD_UNLOCK(); }   // what is kept of the surface so far
    return r;
}
internal int __thiscall rg_hook0(void *ras, void *mesh, unsigned screen, int flag) { return rg_hook(0, 0, ras, mesh, screen, flag); }
internal int __thiscall rg_hook1(void *ras, void *mesh, unsigned screen, int flag) { return rg_hook(1, 0, ras, mesh, screen, flag); }
internal int __thiscall rg_water_hook0(void *ras, void *mesh, unsigned screen, int flag) { return rg_hook(0, 1, ras, mesh, screen, flag); }
internal int __thiscall rg_water_hook1(void *ras, void *mesh, unsigned screen, int flag) { return rg_hook(1, 1, ras, mesh, screen, flag); }
// a frame during a step: the game's draw doesn't run, the ground is drawn from the map all the same - what was made from
// this map, not after another map's load until a real frame has made it anew. While another writer has the world - the
// editor's paint on every stroke, the player's clicks, a load - it is drawn too, from the buffers and the views the mod holds:
// without it such a frame had no ground at all, only the sky's colour (the player, 2026-11-09: "hold a right click down over
// the 3d view, it will flicker continusly until you let go again").
internal int rg_again(int water, int writer) {
    if (!rg_ready() || !g_rg.ras || !g_rg.built || g_rg.map != game_map() || g_rg_new_map) return 0;
    RD_LOCK(); rg_draw_now(water, writer ? 2 : 1); RD_UNLOCK();
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
