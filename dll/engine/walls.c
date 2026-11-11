// WALLS ON SLOPES (`wall_slopes`): the wall's layout (unit_functions.wall_piece 0x638628, for every piece it lays or
// joins anew) turns a piece and picks its model by its place in the wall - straight (anim 9), diagonal (10), the
// round post at an end or a corner (11) - and leaves it flat at the height of its tile's corner x, y: on a hill one
// end floats, the other sinks, and neighbours at other heights stick out of each other in steps. Its sloped pieces
// (6 straight, 3 diagonal) have one slope built in (0.42 over a piece) and come only where the ground between two
// tiles rises 0.4 or more - on Highlands 90% of the neighbouring corners differ by less than 0.27; the corners it
// counts for them (0x517C10) it never reads. Here a piece leans onto the ground instead: its matrix (unit +0x130, the
// engine's GETransformation - drawn, saved and loaded as written, damage leaves it, 2026-11-03) gets a shear along its
// length, z rising with the ground from one end to the other (its sides stay upright), its bottom on that line. And
// it spans its tile exactly, into a post beside it: the meshes are longer than the pieces' spacing (straight
// 1.00 .. 1.06 on 1, diagonal 1.58 .. 1.71 on 1.41), so neighbours overlapped - the faces in one plane flickered, and
// pieces leaning apart stuck out of each other at every bend (the player, 2026-11-03: "the overlap becomes even more
// noticeable now, and it flickers ... instead make the overlap targeted like extending into the round connector
// pieces"). A sloped piece becomes the plain one first. A post stands on the ground at its tile's middle. Where the
// ground falls away below a piece (across its thickness, a bump between its ends, a post's lower corners) its skirt
// fills the gap: the wall's models are drawn reaching WS_SKIRT further down (below). The layout's last step
// (wall_joints) makes a piece where walls meet a post anew, flat:
// the wall's pieces are fitted again after it, with all their neighbours there. Pieces laid before (a loaded game, a
// scenario's) are fitted at the game's first step (wall_start_fit: else they stay as made). A gate laid into a fitted wall takes its pieces upright, as the game's (its
// middle kept a fitted piece's matrix: drawn shortened and sheared). A piece's collapse when destroyed turns it by its
// sheared length (EEPMBuilding::vf12) - it tips over as it sinks.
#define WS_SLOPE_DIAG 3     // the layout's animations (0x63842B): a diagonal piece with its slope built in
#define WS_SLOPE 6          //   a straight one with its slope
#define WS_STRAIGHT 9
#define WS_DIAG 10
#define WS_POST 11
#define WS_OVERLAP 0.01f    // every other overlapping piece this much thicker and taller (below)
#define WS_INTO_POST 0.3f   // a piece's end into a post beside it (the posts' sides 0.49 .. 0.66 from their middle)
#define WS_ACROSS 0.4f      // barbed wire's lean across: the ground this far to either side of its middle
#define WS_JOG_HALF 0.559017f   // half a jog's length (below, wj_jog): from the middle of one link to the next, sqrt(5) / 4
#define WS_JOG_OVER 0.12f   // a jog's end past a neighbour it meets at an angle: closes the wedge on the outside of the bend

internal int g_ws_on, g_ws_start, g_ws_fitted_game;
internal unsigned g_ws_pieces, g_ws_posts, g_ws_unsloped, g_ws_skipped, g_ws_no_model;
#define WS_GFX_MAX 32
internal ee_dmg_model *g_ws_gfx[WS_GFX_MAX];   // the graphics the fit took a user of this game (one each, kept)
internal int g_ws_gfx_n;

// a user of the wall's graphics, once a game: their models loaded and kept while the game runs
internal void ws_acquire(ee_dmg_model *gfx) {
    for (int i = 0; i < g_ws_gfx_n; i++) if (g_ws_gfx[i] == gfx) return;
    if (g_ws_gfx_n >= WS_GFX_MAX) return;
    g_ws_gfx[g_ws_gfx_n++] = gfx;
    int r = dmg_model_acquire(gfx);
    if (g_log) fprintf(g_log, "# walls: the graphics %p loaded for the fit (%d)\n", (void *)gfx, r);
}

// a piece's model by its animation (a sloped one's: the plain one's). Its graphics', not the one it is drawn with:
// a piece laid out is a building site and draws the site's until built. The game loads a graphics' models when a
// first user needs them, and a doppelganger (the fog of war's last-seen copy) is one - on each PC by what its player
// saw: a wall's are often not there yet when its first pieces are laid out (none in the first game minutes). The
// matrix is the game's state (the unit's position, its points), so the fit takes a user of its own, the same step on
// every PC, and has the same model everywhere
internal ee_model *ws_model(ee_dmg_model *gfx, int anim) {
    int ma = anim == WS_SLOPE ? WS_STRAIGHT : anim == WS_SLOPE_DIAG ? WS_DIAG : anim;
    ee_model *mdl = gfx ? ((ee_model **)dmg_model_anim_model_at(gfx))[ma] : NULL;
    if (gfx && !mdl) {
        ws_acquire(gfx);
        mdl = ((ee_model **)dmg_model_anim_model_at(gfx))[ma];
    }
    return mdl;
}

// WALL SKIRTS: a piece set into the ground deep enough to touch it everywhere hid its lower part - a side slope took
// a whole band of it, the player's colour with it (the player, 2026-11-04: "it hides the player color for some wall
// pieces ... instead of sinking the mesh its possible to extrude from the wall pieces downwards?"). So a piece stands
// on the ground and its model reaches below it: every triangle with two corners on the mesh's bottom (its lowest
// point, WS_SKIRT_EDGE) goes on downwards WS_SKIRT deep, in the triangle's winding - into the ground where it is
// level, filling the gap where it falls away. Its face is the foundation of the tower of the wall's era (the player:
// "the streched down skirt dosnt look great yet, and is fully broken on copper age wall because of alpha ... reusing
// the towers foundation texture with the propper uv mapping"): the towers reach 0.47 .. 0.84 below their base, their
// foundation a patch of the same texture sheet the era's walls draw from (the game gives one sheet to an era's walls,
// posts and tower - the model files' texture names are not the ones drawn), so the skirt is laid in the wall's
// material with the tower's texture, the patch of the tower's largest foundation face (its corners' texture
// coordinates, which way runs along and which down) tiled over it at the tower's scale: a tile the face's width
// along the edge, its height down. A wall with no such material (none seen) or no tower (barbed wire, no skirt)
// draws its own bottom row down instead. A model is the engine's (GEModel, one for every piece of that kind and
// era): each material a run of its vertices and of each level of detail's triangles, indices counted from the run's
// first (spec GEMaterial) - its arrays made anew larger by the display DLL (DX7Rasterizer.create_vertex_array,
// create_polygon_array), each material's run followed by its skirt. The renderer copies a material's vertices into
// its vertex buffer every time it draws it (CopyStdVertices), so the new arrays are drawn from the next frame on.
// The graphics thread draws the models while the simulation runs: the fit (simulation thread) asks for the skirt
// (ws_skirt_want, with the tower's model), the model's draw (DrawModelHierarchy, hooked in both display DLLs) makes
// it before drawing it, on the one thread that reads a model's arrays. The old arrays stay (other code may hold them;
// a few KB a model). Only the picture: the pieces' places and the game state are the fit's.
#define WS_SKIRT 1.0f            // how far below its bottom a model reaches
#define WS_SKIRT_EDGE 0.02f      // a vertex this close above the mesh's lowest point is on its bottom
#define WS_SKIRT_MODELS 64
#define WS_TOWER_ANIMS 16        // a tower's graphics: its first model among these animations (its idle ones)
#define WS_UV_MARGIN 0.008f      // a bottom edge's texture coordinates at least this far inside the sheet (2 texels of 256)
struct ws_skirt { ee_model *m, *tower; ee_vertices *va; volatile LONG state; };   // state 0 asked, 1 made, 2 can't be
internal struct ws_skirt g_ws_sk[WS_SKIRT_MODELS];
internal volatile LONG g_ws_sk_n, g_ws_sk_asked;
internal rasterizer_draw_model_hierarchy_fn g_ws_draw[2];
internal int g_ws_draw_hooks;

// the tower of a wall's era (UT_WALL_LINE's i-th is UT_TOWER_LINE's i+1-th: the towers start in the Stone Age), from
// the wall's owner's graphics of it - loaded for the fit as the wall's are; NULL for barbed wire and any wall of no line
internal ee_model *ws_tower_model(int t, ee_player *pl) {
    static const enum ut_name walls[] = { UT_WALL_LINE }, towers[] = { UT_TOWER_LINE };
    for (int i = 0; i < (int)(sizeof walls / sizeof *walls); i++) {
        if (g_ut[walls[i]] != t) continue;
        int tt = g_ut[towers[i + 1]];
        if (!pl || tt < 0 || (unsigned)tt >= type_stats_count(pl)) return NULL;
        ee_unit_stats **sp = type_stats(pl, tt), *st = sp ? *sp : NULL;
        ee_dmg_model *gfx = st ? unit_stats_get_gfx(st) : NULL;
        if (!gfx) return NULL;
        ee_model **am = (ee_model **)dmg_model_anim_model_at(gfx);
        for (int k = 0; k < 2; k++) {
            for (int a = 0; a < WS_TOWER_ANIMS; a++) if (am[a]) return am[a];
            if (!k) ws_acquire(gfx);
        }
        return NULL;
    }
    return NULL;
}

// the fit's model: its skirt made at its next draw (the simulation thread; the draw reads g_ws_sk[0 .. g_ws_sk_n))
internal void ws_skirt_want(ee_model *m, ee_model *tower) {
    if (!m || !tower || !g_ws_draw_hooks) return;
    LONG n = g_ws_sk_n;
    for (LONG i = 0; i < n; i++) {
        struct ws_skirt *s = &g_ws_sk[i];
        if (s->m != m) continue;
        // a model loaded anew where one was (the graphics freed and loaded again): not the arrays it was given
        if (s->state && model_get_frames(m) && model_get_vertices(m) && model_get_vertices(m)[0] != s->va) {
            s->tower = tower;
            s->state = 0;
            InterlockedIncrement(&g_ws_sk_asked);
        }
        return;
    }
    if (n >= WS_SKIRT_MODELS) return;
    g_ws_sk[n] = (struct ws_skirt){ m, tower, NULL, 0 };
    InterlockedIncrement(&g_ws_sk_asked);
    InterlockedExchange(&g_ws_sk_n, n + 1);
}

// a texture coordinate on (or a hair over) the sheet's border drawn inside it: the sheets wrap, so the filtering at
// the border blends in the opposite one - the Copper Age sheet's bottom row (its walls' bottom edges, v 0.99 .. 1.00)
// with its top rows, transparent: a pale seam where the wall meets its skirt (the player, 2026-11-04: "the alpha
// blending bleeding in, betwen the wall and the skirt"; below the ground in the game's own walls)
internal float ws_uv_in(float x) {
    return x > 1 - WS_UV_MARGIN && x < 1 + WS_UV_MARGIN ? 1 - WS_UV_MARGIN : x > -WS_UV_MARGIN && x < WS_UV_MARGIN ? WS_UV_MARGIN : x;
}

internal int ws_cmp_first(const void *a, const void *b) {
    uint32_t x = material_get_vertex_first(*(ee_material *const *)a), y = material_get_vertex_first(*(ee_material *const *)b);
    return x < y ? -1 : x > y;
}

// a model's arrays as the skirt reads them: standard vertices in every frame, a triangle array for lod 0
internal int ws_model_ok(ee_model *m) {
    uint32_t nf = model_get_frames(m), nl = model_get_lods(m), nm = model_get_material_count(m), nv = model_get_vertex_count(m);
    ee_vertices **va = model_get_vertices(m);
    ee_polygons **pa = model_get_polygons(m);
    if (!nf || !nl || !nm || !nv || nf > 256 || nl > 16 || nm > 256 || !va || !pa || !pa[0] || !model_get_materials(m)) return 0;
    for (uint32_t f = 0; f < nf; f++)
        if (!va[f] || vertices_get_type(va[f]) != 0 || vertices_get_count(va[f]) < nv) return 0;
    return 1;
}
internal float ws_lowest(ee_model *m) {
    ee_vertex *v0 = vertices_get_data(model_get_vertices(m)[0]);
    float zmin = 1e9f;
    for (uint32_t i = 0; i < model_get_vertex_count(m); i++) { float z = vertex_point_at(vertex_at(v0, i))->z; if (z < zmin) zmin = z; }
    return zmin;
}

// a tower's foundation: the face of the largest triangle below its base reaching its lowest point - its material and
// texture, its width along (w) and height down (h), the texture coordinates of its top corner (u0, v0) and their step
// across the face along (su, sv) and down it (tu, tv)
struct ws_found { int mat; float w, h, u0, v0, su, sv, tu, tv; };
internal int ws_found_of(ee_model *t, struct ws_found *f) {
    if (!ws_model_ok(t)) return 0;
    ee_vertex *v0 = vertices_get_data(model_get_vertices(t)[0]);
    ee_material **mat = model_get_materials(t);
    const uint16_t *ix = polygons_get_indices(model_get_polygons(t)[0]);
    float zmin = ws_lowest(t), best = 0;
    for (uint32_t k = 0; k < model_get_material_count(t); k++) {
        if (!mat[k]) continue;
        uint32_t vf = material_get_vertex_first(mat[k]), vc = material_get_vertex_count(mat[k]);
        uint32_t pf = material_get_polygon_first(mat[k])[0], pc = material_get_polygon_count(mat[k])[0];
        for (uint32_t i = pf; i < pf + pc; i++) {
            ee_vertex *p[3];
            float z[3];
            int ok = 1;
            for (int j = 0; j < 3; j++) {
                if (ix[i * 3 + j] >= vc) { ok = 0; break; }
                p[j] = vertex_at(v0, vf + ix[i * 3 + j]);
                z[j] = vertex_point_at(p[j])->z;
                if (z[j] > 0.01f) ok = 0;
            }
            if (!ok || fminf(z[0], fminf(z[1], z[2])) > zmin + 0.01f) continue;
            // two corners at one height (its edge along), the third above or below one of them
            int a = -1;
            for (int j = 0; j < 3; j++) if (fabsf(z[j] - z[(j + 1) % 3]) < 0.01f) a = j;
            if (a < 0) continue;
            int b = (a + 1) % 3, c = (a + 2) % 3;
            vec3 pa = *vertex_point_at(p[a]), pb = *vertex_point_at(p[b]), pc3 = *vertex_point_at(p[c]);
            float w = sqrtf((pb.x - pa.x) * (pb.x - pa.x) + (pb.y - pa.y) * (pb.y - pa.y)), h = fabsf(pc3.z - pa.z);
            if (w * h <= best || h < 0.1f) continue;
            // q: the one of the pair under (or over) the third, r the other
            float da = (pc3.x - pa.x) * (pc3.x - pa.x) + (pc3.y - pa.y) * (pc3.y - pa.y);
            float db = (pc3.x - pb.x) * (pc3.x - pb.x) + (pc3.y - pb.y) * (pc3.y - pb.y);
            ee_vertex *q = da <= db ? p[a] : p[b], *r = da <= db ? p[b] : p[a], *top = p[c], *bot = q;
            if (pc3.z < pa.z) { top = q; bot = p[c]; }   // the pair on top
            best = w * h;
            *f = (struct ws_found){ (int)k, w, h, vertex_get_u(top), vertex_get_v(top),
                                    vertex_get_u(r) - vertex_get_u(q), vertex_get_v(r) - vertex_get_v(q),
                                    vertex_get_u(bot) - vertex_get_u(top), vertex_get_v(bot) - vertex_get_v(top) };
        }
    }
    return best > 0;
}

// a vertex of the skirt: at s of the way from old vertex a to b (their normals too), d below; its texture coordinates
struct ws_rv { uint32_t a, b, k, i; float s, d, u, v; };   // k, i: its material (sorted) and its place in its skirt
struct ws_tri { uint16_t l, k, i[3]; };            // a skirt triangle: its lod, its material (sorted), its corners
struct ws_edge { uint32_t a, b, first, n; };       // an edge's skirt: its vertices' first (its material's own) and count
struct ws_build { struct ws_rv *rv; struct ws_tri *tri; struct ws_edge *edge; uint32_t rv_n, rv_cap, tri_n, tri_cap, edge_n, edge_cap; };
#define WS_GROW(p, n, cap) ((n) < (cap) || ws_grow((void **)&(p), &(cap), sizeof *(p)))
internal int ws_grow(void **p, uint32_t *cap, size_t size) {
    uint32_t c = *cap ? *cap * 2 : 64;
    void *q = realloc(*p, c * size);
    if (!q) return 0;
    *p = q; *cap = c;
    return 1;
}

// the skirt of m (the graphics thread, before its draw): 2 made with the tower's foundation, 1 made with its own
// bottom row, 0 none (no bottom edge), -1 a model it can't take (another vertex kind, materials not in runs one after
// another, a material beyond the vertex buffer) - left as it is
internal int ws_skirt_make(ee_rasterizer *ras, ee_model *m, ee_model *tower, unsigned *edges_out) {
    if (!ws_model_ok(m)) return -1;
    uint32_t nf = model_get_frames(m), nl = model_get_lods(m), nm = model_get_material_count(m), nv = model_get_vertex_count(m);
    ee_vertices **va = model_get_vertices(m);
    ee_polygons **pa = model_get_polygons(m);
    ee_material **mat = model_get_materials(m);
    // the materials in the order of their vertices: one run after another from 0 to nv, and each lod's triangles too
    ee_material *ord[256];
    for (uint32_t k = 0; k < nm; k++) { ord[k] = mat[k]; if (!ord[k]) return -1; }
    qsort(ord, nm, sizeof *ord, ws_cmp_first);
    uint32_t at = 0;
    for (uint32_t k = 0; k < nm; k++) {
        if (material_get_vertex_first(ord[k]) != at) return -1;
        at += material_get_vertex_count(ord[k]);
    }
    if (at != nv) return -1;
    for (uint32_t l = 0; l < nl; l++) {
        uint32_t pat = 0;
        for (uint32_t k = 0; k < nm; k++) {
            uint32_t pc = material_get_polygon_count(ord[k])[l];
            if (!pc) continue;
            if (!pa[l] || material_get_polygon_first(ord[k])[l] != pat) return -1;
            pat += pc;
        }
        if (pa[l] && pat > polygons_get_count(pa[l])) return -1;
    }
    // the tower's foundation, and the material here with its texture (the tower's own index first: the plain and the
    // player's colour share one sheet) - told by the texture's file name: a tower none of which is on the map has its
    // names but no textures, and the skirt was the wall's own bottom row drawn down for the whole game (the player,
    // 2026-11-08: "the skirts are broken, they are pure extrude without proper uv mapping")
    struct ws_found fd = { 0 };
    int kt = -1;
    const char *fname = tower && ws_found_of(tower, &fd) ? model_texture_name(tower, (uint32_t)fd.mat) : "";
    if (*fname) {
        if ((uint32_t)fd.mat < nm && !_stricmp(model_texture_name(m, (uint32_t)fd.mat), fname)) kt = fd.mat;
        for (uint32_t k = 0; kt < 0 && k < nm; k++)
            if (!_stricmp(model_texture_name(m, k), fname)) kt = (int)k;
    }
    if (kt >= 0)
        for (uint32_t k = 0; k < nm; k++) if (ord[k] == mat[kt]) { kt = (int)k; break; }   // its place in ord
    ee_vertex *v0 = vertices_get_data(va[0]);
    float zmin = ws_lowest(m);
    uint32_t *extra = calloc(nm, sizeof *extra);   // [k]: its skirt's vertices
    struct ws_build bd = { 0 };
    unsigned edges = 0;
    int ok = extra != NULL;
    for (uint32_t l = 0; ok && l < nl; l++)
        for (uint32_t k = 0; ok && k < nm; k++) {
            uint32_t vf = material_get_vertex_first(ord[k]), vc = material_get_vertex_count(ord[k]);
            uint32_t pf = material_get_polygon_first(ord[k])[l], pc = material_get_polygon_count(ord[k])[l];
            const uint16_t *ix = pc ? polygons_get_indices(pa[l]) : NULL;
            for (uint32_t t = pf; ok && t < pf + pc; t++) {
                uint16_t c[3] = { ix[t * 3], ix[t * 3 + 1], ix[t * 3 + 2] };
                int low[3], lows = 0;
                for (int j = 0; j < 3; j++) {
                    if (c[j] >= vc) { ok = 0; break; }
                    low[j] = vertex_point_at(vertex_at(v0, vf + c[j]))->z <= zmin + WS_SKIRT_EDGE;
                    lows += low[j];
                }
                if (!ok || lows != 2) continue;
                // the edge a -> b in the triangle's order; below it each tile (b side, a side, a side down), (b side,
                // a side down, b side down), the same way round
                int j = low[0] && low[1] ? 0 : low[1] && low[2] ? 1 : 2;
                uint32_t a = vf + c[j], b = vf + c[(j + 1) % 3];
                uint32_t to = kt >= 0 ? (uint32_t)kt : k;   // the material it goes into
                struct ws_edge *e = NULL;
                for (uint32_t i = 0; i < bd.edge_n; i++) if (bd.edge[i].a == a && bd.edge[i].b == b) { e = &bd.edge[i]; break; }
                int segs = 1, rows = 1;
                float last = 1;   // the last row's share of a tile
                if (kt >= 0) {
                    vec3 pa3 = *vertex_point_at(vertex_at(v0, a)), pb3 = *vertex_point_at(vertex_at(v0, b));
                    float len = sqrtf((pb3.x - pa3.x) * (pb3.x - pa3.x) + (pb3.y - pa3.y) * (pb3.y - pa3.y) + (pb3.z - pa3.z) * (pb3.z - pa3.z));
                    segs = (int)(len / fd.w + 0.5f);
                    if (segs < 1) segs = 1;
                    if (segs > 8) segs = 8;
                    rows = (int)ceilf(WS_SKIRT / fd.h - 0.01f);
                    if (rows > 8) rows = 8;
                    last = (WS_SKIRT - (float)(rows - 1) * fd.h) / fd.h;
                }
                if (!e) {   // its vertices: four a tile (the texture starts anew in each)
                    if (!WS_GROW(bd.edge, bd.edge_n, bd.edge_cap)) { ok = 0; break; }
                    e = &bd.edge[bd.edge_n++];
                    *e = (struct ws_edge){ a, b, extra[to], (uint32_t)(segs * rows * 4) };
                    if (kt < 0) e->n = 4;
                    for (int sg = 0; sg < (kt >= 0 ? segs : 1); sg++)
                        for (int rw = 0; rw < rows; rw++)
                            for (int corner = 0; corner < 4; corner++) {
                                if (!WS_GROW(bd.rv, bd.rv_n, bd.rv_cap)) { ok = 0; break; }
                                int along = corner & 1, down = corner >> 1;
                                struct ws_rv *r = &bd.rv[bd.rv_n++];
                                r->a = a; r->b = b; r->k = to; r->i = extra[to]++;
                                r->s = ((float)sg + (float)along) / (float)segs;
                                if (kt >= 0) {
                                    float dt = rw == rows - 1 && down ? last : (float)down;
                                    r->d = ((float)rw + dt) * fd.h;
                                    if (r->d > WS_SKIRT) r->d = WS_SKIRT;
                                    r->u = fd.u0 + fd.su * (float)along + fd.tu * dt;
                                    r->v = fd.v0 + fd.sv * (float)along + fd.tv * dt;
                                } else {   // its own bottom row drawn down
                                    ee_vertex *src = vertex_at(v0, along ? b : a);
                                    r->d = down ? WS_SKIRT : 0;
                                    r->u = ws_uv_in(vertex_get_u(src)); r->v = ws_uv_in(vertex_get_v(src));
                                }
                            }
                    if (!ok) break;
                }
                // its triangles at this lod, into its material's run after its own vertices
                uint32_t base = material_get_vertex_count(ord[to]) + e->first;
                for (uint32_t cell = 0; cell < e->n / 4; cell++) {
                    uint32_t q = base + cell * 4;   // a, b side above; a, b side below
                    uint16_t tris[2][3] = { { (uint16_t)(q + 1), (uint16_t)q, (uint16_t)(q + 2) },
                                            { (uint16_t)(q + 1), (uint16_t)(q + 2), (uint16_t)(q + 3) } };
                    for (int h = 0; h < 2; h++) {
                        if (!WS_GROW(bd.tri, bd.tri_n, bd.tri_cap)) { ok = 0; break; }
                        bd.tri[bd.tri_n++] = (struct ws_tri){ (uint16_t)l, (uint16_t)to, { tris[h][0], tris[h][1], tris[h][2] } };
                    }
                }
                if (!l) edges++;
            }
        }
    for (uint32_t k = 0; ok && k < nm; k++)
        if (material_get_vertex_count(ord[k]) + extra[k] > display_vb_vertices() || material_get_vertex_count(ord[k]) + extra[k] > 0xFFFF) ok = 0;
    int r = !ok ? -1 : !edges ? 0 : kt >= 0 ? 2 : 1;
    if (r > 0) {
        // the new arrays: each frame's (one for the frames that shared one), each lod's
        uint32_t nv2 = nv + bd.rv_n;
        ee_vertices *nva[256] = { 0 };
        ee_polygons *npa[16] = { 0 };
        uint32_t start[256];   // [k]: its run's first in the new arrays
        for (uint32_t k = 0, s = 0; k < nm; k++) { start[k] = s; s += material_get_vertex_count(ord[k]) + extra[k]; }
        for (uint32_t f = 0; r > 0 && f < nf; f++) {
            uint32_t g = 0;
            while (g < f && va[g] != va[f]) g++;
            if (g < f) { nva[f] = nva[g]; continue; }
            if (!(nva[f] = rasterizer_create_vertex_array(ras, 0, nv2))) { r = -1; break; }
            ee_vertex *src = vertices_get_data(va[f]), *dst = vertices_get_data(nva[f]);
            for (uint32_t k = 0; k < nm; k++)
                memcpy(vertex_at(dst, start[k]), vertex_at(src, material_get_vertex_first(ord[k])), (size_t)material_get_vertex_count(ord[k]) * vertex_size());
            for (uint32_t i = 0; i < bd.edge_n; i++)   // the edges' own corners off the sheet's border (ws_uv_in)
                for (int e = 0; e < 2; e++) {
                    uint32_t old = e ? bd.edge[i].b : bd.edge[i].a, k = 0;
                    while (k + 1 < nm && old >= material_get_vertex_first(ord[k]) + material_get_vertex_count(ord[k])) k++;
                    ee_vertex *d = vertex_at(dst, start[k] + old - material_get_vertex_first(ord[k]));
                    vertex_set_u(d, ws_uv_in(vertex_get_u(d)));
                    vertex_set_v(d, ws_uv_in(vertex_get_v(d)));
                }
            for (uint32_t i = 0; i < bd.rv_n; i++) {
                const struct ws_rv *rv = &bd.rv[i];
                ee_vertex *d = vertex_at(dst, start[rv->k] + material_get_vertex_count(ord[rv->k]) + rv->i);
                ee_vertex *pa1 = vertex_at(src, rv->a), *pb1 = vertex_at(src, rv->b);
                memcpy(d, pa1, vertex_size());
                vec3 p = *vertex_point_at(pa1), q = *vertex_point_at(pb1), n = *vertex_normal_at(pa1), n2 = *vertex_normal_at(pb1);
                *vertex_point_at(d) = (vec3){ p.x + (q.x - p.x) * rv->s, p.y + (q.y - p.y) * rv->s, p.z + (q.z - p.z) * rv->s - rv->d };
                *vertex_normal_at(d) = (vec3){ n.x + (n2.x - n.x) * rv->s, n.y + (n2.y - n.y) * rv->s, n.z + (n2.z - n.z) * rv->s };
                vertex_set_u(d, rv->u);
                vertex_set_v(d, rv->v);
            }
        }
        for (uint32_t l = 0; r > 0 && l < nl; l++) {
            if (!pa[l]) continue;
            uint32_t add = 0;
            for (uint32_t i = 0; i < bd.tri_n; i++) add += bd.tri[i].l == l;
            uint32_t n2 = polygons_get_count(pa[l]) + add;
            if (!(npa[l] = rasterizer_create_polygon_array(ras, n2))) { r = -1; break; }
            const uint16_t *src = polygons_get_indices(pa[l]);
            uint16_t *dst = polygons_get_indices(npa[l]);
            uint32_t to = 0, own = 0;
            for (uint32_t k = 0; k < nm; k++) {
                uint32_t pf = material_get_polygon_first(ord[k])[l], pc = material_get_polygon_count(ord[k])[l];
                memcpy(dst + to * 3, src + pf * 3, (size_t)pc * 3 * sizeof *dst);
                to += pc; own += pc;
                for (uint32_t i = 0; i < bd.tri_n; i++)
                    if (bd.tri[i].l == l && bd.tri[i].k == k) { memcpy(dst + to * 3, bd.tri[i].i, 3 * sizeof *dst); to++; }
            }
            // a lod's triangles beyond its materials' (none seen) stay at its end
            memcpy(dst + to * 3, src + own * 3, (size_t)(polygons_get_count(pa[l]) - own) * 3 * sizeof *dst);
        }
        if (r > 0) {   // the model onto them: the runs moved by the skirts before them
            for (uint32_t f = 0; f < nf; f++) va[f] = nva[f];
            for (uint32_t l = 0; l < nl; l++) if (npa[l]) pa[l] = npa[l];
            uint32_t pto[16] = { 0 };
            for (uint32_t k = 0; k < nm; k++) {
                uint32_t *pf = material_get_polygon_first(ord[k]), *pc = material_get_polygon_count(ord[k]);
                for (uint32_t l = 0; l < nl; l++) {
                    uint32_t add = 0;
                    for (uint32_t i = 0; i < bd.tri_n; i++) add += bd.tri[i].l == l && bd.tri[i].k == k;
                    if (!pc[l] && !add) continue;
                    pf[l] = pto[l];
                    pc[l] += add;
                    pto[l] += pc[l];
                }
                material_set_vertex_first(ord[k], start[k]);
                material_set_vertex_count(ord[k], material_get_vertex_count(ord[k]) + extra[k]);
            }
            model_set_vertex_count(m, nv2);
            if (npa[0]) model_set_polygon_count(m, polygons_get_count(npa[0]));
        }
    }
    free(extra); free(bd.rv); free(bd.tri); free(bd.edge);
    *edges_out = edges;
    return r;
}

// a model drawn: its skirt first if the fit asked for it
internal void ws_skirt_drawn(ee_rasterizer *ras, ee_model *m) {
    LONG n = g_ws_sk_n;
    for (LONG i = 0; i < n; i++) {
        struct ws_skirt *s = &g_ws_sk[i];
        if (s->m != m || s->state) continue;
        unsigned edges = 0;
        int r = ws_skirt_make(ras, m, s->tower, &edges);
        s->va = model_get_frames(m) && model_get_vertices(m) ? model_get_vertices(m)[0] : NULL;
        s->state = r >= 0 ? 1 : 2;
        InterlockedDecrement(&g_ws_sk_asked);
        log_locked("# walls: model %p %s (%u bottom edges, %u vertices, %u materials, %u lods; tower %p)\n", (void *)m,
                   r == 2 ? "skirted with its tower's foundation" : r == 1 ? "skirted with its own bottom row (no material with its tower's texture)" :
                   r == 0 ? "has no bottom edge" : "CAN'T be skirted", edges,
                   model_get_vertex_count(m), model_get_material_count(m), model_get_lods(m), (void *)s->tower);
        return;
    }
}
// BENT OBJECTS (building_lean): an ambient object's or a resource site's model bent onto the ground where it is drawn (bl_bend below)
internal volatile LONG g_bl_n;
internal ee_vertices *bl_bend(ee_model *m, int *decal);
internal void bl_unbend(ee_vertices *a);
internal int __thiscall ws_hook_draw0(ee_rasterizer *ras, void *m, uint32_t screen, float p3, int8_t p4) {
    if (g_ws_sk_asked > 0) ws_skirt_drawn(ras, m);
    int decal = g_in_rec ? g_in_rec->decal : 0;   // (render_units.c's record: its bent copy kept, bent here no more)
    ee_vertices *b = g_bl_n && !g_in_rec ? bl_bend(m, &decal) : NULL;
    g_sh_decal = decal;
    int r = g_ws_draw[0](ras, m, screen, p3, p4);
    g_sh_decal = 0;
    if (b) bl_unbend(b);
    return r;
}
internal int __thiscall ws_hook_draw1(ee_rasterizer *ras, void *m, uint32_t screen, float p3, int8_t p4) {
    if (g_ws_sk_asked > 0) ws_skirt_drawn(ras, m);
    int decal = g_in_rec ? g_in_rec->decal : 0;   // (render_units.c's record: its bent copy kept, bent here no more)
    ee_vertices *b = g_bl_n && !g_in_rec ? bl_bend(m, &decal) : NULL;
    g_sh_decal = decal;
    g_mo_off = b != NULL;   // its frame's vertices bent for this draw: copied, not the GPU's (render_morph.c)
    int r = g_ws_draw[1](ras, m, screen, p3, p4);
    g_mo_off = 0;
    g_sh_decal = 0;
    if (b) bl_unbend(b);
    return r;
}
// DrawModelHierarchy in each display DLL's vtable (at its load: they load after the mod, patch.c on_module_load)
internal void ws_display_loaded(int i, uintptr_t base) {
    void *f = (void *)GetProcAddress((HMODULE)base, "?DrawModelHierarchy@DX7Rasterizer@@MAEJPAVGEModel@@KM_N@Z");
    if (!f || g_ws_draw[i]) return;
    g_ws_draw[i] = (rasterizer_draw_model_hierarchy_fn)f;
    int n = replace_rdata_ptr((HMODULE)base, f, i ? (void *)ws_hook_draw1 : (void *)ws_hook_draw0);
    if (n) g_ws_draw_hooks++;
    log_locked("# walls: skirts - %s's model draw %s\n", i ? "DX7HRTnLDisplay.dll" : "DX7HRDisplay.dll", n == 1 ? "hooked" : n ? "hooked (more than once)" : "NOT hooked");
}
internal void ws_display_loaded0(uintptr_t base) { ws_display_loaded(0, base); }
internal void ws_display_loaded1(uintptr_t base) { ws_display_loaded(1, base); }

#define WS_LAID_MAX 512
internal ee_unit *g_ws_laid[WS_LAID_MAX];   // the pieces of the layout under way, fitted again at its last step
internal int g_ws_laid_n;

// the same owner's object on tile x, y, as wall_joins sees it (wj_at below): its kind, and a piece's axis
#define WJ_NONE 0
#define WJ_PIECE 1
#define WJ_POST 2
#define WJ_HUB 3   // a tower or a gate: a wall ends into it
#define WJ_JOG 4   // a piece of a jog (wj_jog): along neither the tiles nor their diagonal, it points at no tile
struct wj_at { int kind, sx, sy; ee_unit *u; };
struct wj_jog_fit { float ox, oy, fx, fy, ax, ay, ea, eb; int diag; };   // off its tile's middle, its facing, across it, how far its ends reach on, its ends along a diagonal piece's
internal struct wj_at wj_at(const ee_map *map, int x, int y, const ee_player *pl);
internal int wj_jog_place(const ee_map *map, int x, int y, const ee_player *pl, struct wj_jog_fit *j);
internal void wj_upright_m(const ee_map *map, float *m, int tx, int ty, float fx, float fy);
internal int g_wj_on;
// a wall's placement preview on the tiles (THE PREVIEW below), which wj_at reads before the map - only while the thread
// that makes the preview is at it: the walls' state is the simulation's, the same on every PC, and never sees this
#define WJ_PV_MAX 1024
#define WJ_PV_SLOTS 4096    // the tiles' hash (a power of two)
struct wj_pv { int x, y; struct wj_at at; ee_ghost_unit *g; };
internal struct wj_pv g_wj_pv[WJ_PV_MAX];
internal int16_t g_wj_pv_slot[WJ_PV_SLOTS];   // an entry's index + 1, 0 none
internal int g_wj_pv_n;
internal DWORD g_wj_pv_thread;
internal const ee_player *g_wj_pv_pl;
internal volatile LONG g_ws_ed_seen;   // GetTickCount of the scenario editor's last frame, 0 none (below)
internal int g_ws_ed_base;   // the editor's walls looked at since it was entered (below)

// a wall's round post on tile x, y, of pl's - or with wall_joins a tower or a gate of pl's: a piece's end reaches into
// it. The layout's own pieces from its list - they come onto the map (the unit grid, a tile's objects) only after the
// layout, at the next step; an older wall's by the tile's objects
internal int ws_post_at(const ee_map *map, int x, int y, const ee_player *pl) {
    for (int i = 0; i < g_ws_laid_n; i++) {
        ee_unit *u = g_ws_laid[i];
        if (u && unit_get_x(u) == x && unit_get_y(u) == y && unit_get_anim(u) == WS_POST) return 1;
    }
    int k = wj_at(map, x, y, pl).kind;
    return k == WJ_POST || (g_wj_on && k == WJ_HUB);
}

// what the fit needs of a piece: a unit's on the map, a placement ghost's (THE PREVIEW below)
struct ws_piece { int tx, ty, anim, type; float scale; ee_player *pl; ee_dmg_model *gfx; };
internal int ws_plain_anim(int anim) { return anim == WS_SLOPE ? WS_STRAIGHT : anim == WS_SLOPE_DIAG ? WS_DIAG : anim; }

// one piece (anim a plain one's) onto the ground: its matrix m (3 x 4, by row); 0 when its ground is off the map
internal int ws_fit_piece(const struct ws_piece *p, float *m) {
    ee_map *map = game_map();
    if (!map) return 0;
    int anim = p->anim, tx = p->tx, ty = p->ty;
    // the middle of its tile, not the matrix's: a piece's matrix is moved below, a fit again gives the same
    float px = (float)tx + 0.5f, py = (float)ty + 0.5f;
    ee_model *mdl = ws_model(p->gfx, anim);
    ws_skirt_want(mdl, ws_tower_model(p->type, p->pl));
    if (anim == WS_POST) {
        float mid = ground_at(map, px, py);
        if (mid != mid) { g_ws_skipped++; return 0; }
        // a piece the game made a post (a wall laid onto its side) keeps what it stood with: a jog's place off its tile's
        // middle and its stretch, and this fit only sets the height - a post drawn long and slanted (the player, 2026-11-08:
        // "i have the stretched connector again")
        float ly = sqrtf(m[1] * m[1] + m[5] * m[5]);
        float lx = sqrtf(m[0] * m[0] + m[4] * m[4]);
        if (fabsf(m[3] - px) > 0.001f || fabsf(m[7] - py) > 0.001f || fabsf(lx - ly) > 0.001f) wj_upright_m(map, m, tx, ty, 0, 1);
        // its scale from its turn (x, y), so fitting it again gives the same; its skirt down to its lower corners
        float s = sqrtf(m[0] * m[0] + m[4] * m[4]);
        m[8] = 0;
        m[9] = 0;
        m[10] = s;
        m[11] = mid;
        g_ws_posts++;
        return 1;
    }
    // its length: the local y axis, its turn (x, y; the local x axis keeps its scale, 1 for walls); its ends half a
    // tile out along it - an edge's middle (straight) or a corner (diagonal), where the ground's corners are exact
    // Its scale the unit's own (1 normally), not the matrix's: that holds the last fit's WS_OVERLAP, and read back each
    // fit grew a Copper or barbed wire piece by it again - a wall laid on beside it fitted it once more, and it rose
    // into a tower of stone (the player, 2026-11-05: "wall grows infinitely when i continue to place a wall segment
    // into or beside it")
    float fx = m[1], fy = m[5], f = sqrtf(fx * fx + fy * fy);
    float len = p->scale;
    if (!(len > 0.01f && len < 100)) len = sqrtf(m[0] * m[0] + m[4] * m[4]);
    if (f < 0.01f || len < 0.01f) { g_ws_skipped++; return 0; }
    fx /= f; fy /= f;
    // a straight piece of a jog from the middle of one link to the middle of the next, off its tile's middle; one that
    // no longer is (its neighbours not on the map yet, wall_joins off) back along the tiles
    struct wj_jog_fit jf;
    int straight = anim == WS_STRAIGHT, jog = g_wj_on && straight && wj_jog_place(map, tx, ty, p->pl, &jf);
    if (jog) { px += jf.ox; py += jf.oy; fx = jf.fx; fy = jf.fy; }
    else if (straight && fabsf(fx) > 0.3f && fabsf(fy) > 0.3f) {
        if (fabsf(fx) > fabsf(fy)) { fx = fx > 0 ? 1.0f : -1.0f; fy = 0; }
        else { fy = fy > 0 ? 1.0f : -1.0f; fx = 0; }
    }
    float d = jog ? WS_JOG_HALF : 0.5f * (fabsf(fx) + fabsf(fy));
    float ax = px - d * fx, ay = py - d * fy, bx = px + d * fx, by = py + d * fy;
    float ha = ground_at(map, ax, ay), hb = ground_at(map, bx, by);
    if (ha != ha || hb != hb) { g_ws_skipped++; return 0; }
    // its mesh (the model's first frame, its y from ylo to yhi - each era's its own length, 1.00 .. 1.71, and off its
    // middle by up to 0.025) stretched onto its tile's span exactly: the meshes were made longer than the spacing, and
    // neighbours overlapped - the faces flickered and, leaning apart, stuck out at every bend. Into a post at an end
    // (it hides it; a diagonal piece's end at the corner would stop short of one)
    ee_box *box = mdl && model_get_frames(mdl) ? model_get_boxes(mdl) : NULL;
    float ylo = box ? box_get_min(box).y : -d, yhi = box ? box_get_max(box).y : d;
    if (yhi - ylo < 0.1f) { ylo = -d; yhi = d; g_ws_no_model++; }
    // drawn at their own length (only into a post longer): the Copper Age wall's stone is the span, its boards and flags
    // on top reach beyond it - shrunk to the span a seam opened at every join (the player, 2026-11-03: "the straight
    // copper age wall has a tiny gap"; its mesh 1.064, the others' 1.000 .. 1.044) - and barbed wire's coils are made
    // to overlap ("barbed wire is a bit of an edge case, it should not be shortened")
    // Their neighbours overlap then, the faces of the overlap in one plane - they flickered, as in the game (the
    // player, 2026-11-04: "z fighting betwen wall pieces (was also an issue in the base game)"): every other piece
    // along the run (by its tile across the run's direction) WS_OVERLAP thicker and taller, its faces off its
    // neighbours'
    float grow = 1, across = 0;
    int type = p->type;
    if (UT_IN(type, UT_B_WALL_COPPER, UT_B_BARBED_WIRE)) {
        float mid = (ylo + yhi) * 0.5f, own = jog ? 0.5f : d;   // a jog's the plain piece's, drawn longer: no seam
        ylo = mid - own; yhi = mid + own;
        if ((fabsf(fx) >= fabsf(fy) ? tx : ty) & 1) grow = 1 + WS_OVERLAP;
    }
    // barbed wire leans across the slope too (its local x, sheared: the stakes stay upright), its feet on the ground on
    // both sides - it has no skirt below it (the player, 2026-11-05: "the aa guns have a feature to be rotated towards
    // the ground, might be useful here?"; the game's turn of the whole model would lean the stakes off the vertical).
    // A wall's foot side to side is its skirt's
    if (type == UT(B_BARBED_WIRE)) {
        float hl = ground_at(map, px + WS_ACROSS * fy, py - WS_ACROSS * fx), hr = ground_at(map, px - WS_ACROSS * fy, py + WS_ACROSS * fx);
        if (hl == hl && hr == hr) across = (hl - hr) / (2 * WS_ACROSS);
    }
    int sx = fx > 0.3f ? 1 : fx < -0.3f ? -1 : 0, sy = fy > 0.3f ? 1 : fy < -0.3f ? -1 : 0;
    ee_player *pl = p->pl;
    float ea = jog ? jf.ea : ws_post_at(map, tx - sx, ty - sy, pl) ? WS_INTO_POST : 0, eb = jog ? jf.eb : ws_post_at(map, tx + sx, ty + sy, pl) ? WS_INTO_POST : 0;
    float k = (2 * d + ea + eb) / (yhi - ylo), c = -d - ea - k * ylo, slope = (hb - ha) / (2 * d);   // its y -> k y + c along f
    float wx = jog ? jf.ax : fy, wy = jog ? jf.ay : -fx;   // its local x axis: square to it, a jog's along its neighbour's end
    // a jog is the straight model; beside a diagonal piece as wide and as high as that one's model, another size (concrete:
    // straight 0.44 wide and 1.05 high, diagonal 0.48 and 1.02), they stepped at the join (the player, 2026-11-08: "the 1:2
    // wall is a bit too narrow and too high"). Between straight pieces exactly their width, for their ends to meet: wider, a
    // crack stood at the join, down the middle of a laser wall's post (the player, 2026-11-09: "still a few places that
    // dont connect nicely")
    float wide = 1, tall = 1;
    if (jog && jf.diag && box) {
        ee_model *dm = ws_model(p->gfx, WS_DIAG);
        ee_box *db = dm && model_get_frames(dm) ? model_get_boxes(dm) : NULL;
        float sw = box_get_max(box).x - box_get_min(box).x, sz = box_get_max(box).z;
        if (db && sw > 0.01f && sz > 0.01f) { wide = (box_get_max(db).x - box_get_min(db).x) / sw; tall = box_get_max(db).z / sz; }
    }
    const float fit[12] = {
        wx * len * grow * wide,  fx * k,    0, px + fx * c,
        wy * len * grow * wide,  fy * k,    0, py + fy * c,
        across * len * grow, slope * k, len * grow * tall, (ha + hb) * 0.5f + slope * c,
    };
    memcpy(m, fit, sizeof fit);
    g_ws_pieces++;
    return 1;
}

// one piece of the map onto the ground; 0 when it isn't a piece of the layout's or its ground is off the map
internal int ws_fit(ee_unit *u) {
    if (!game_map() || !u) return 0;
    int anim = unit_get_anim(u), plain = ws_plain_anim(anim);
    if (plain != anim) { unit_start_anim(u, plain, -1, 0); g_ws_unsloped++; }
    else if (anim != WS_STRAIGHT && anim != WS_DIAG && anim != WS_POST) return 0;
    ee_unit_stats *st = unit_get_stats(u);
    const struct ws_piece p = { unit_get_x(u), unit_get_y(u), plain, unit_type_get_id(unit_get_type_data(u)), unit_get_scale(u), unit_get_owner(u), st ? unit_stats_get_gfx(st) : NULL };
    float m[12];
    for (int i = 0; i < 12; i++) m[i] = unit_get_matrix(u, i);
    if (!ws_fit_piece(&p, m)) return 0;
    for (int i = 0; i < 12; i++) unit_set_matrix(u, i, m[i]);
    return 1;
}

// WALL JOINS (`wall_joins`, with wall_slopes): which tile of a wall is a round post. The layout makes one at each end
// of the stretch it lays, where the stretch turns, and (its sloped pieces) where the ground steps 0.4 or more; its
// last step (wall_joints) one of every piece with more than two of the owner's walls or towers among its 8
// neighbours. So a wall drawn on from the end of another has two posts side by side, one drawn up to another's side
// or across it three or four in a cluster (the diagonal neighbours of the joint count), and a post stands between a
// wall and the tower or gate it runs into (the player, 2026-11-03: "the round connectors appearing in many places they
// aren't supposed to ... walls should properly connect to gates and towers"). Decided when walls are built, as the
// game decides its posts (the player: "a runtime check every few seconds seems overkill for something that should be
// set at build time" - and the gaps of a destroyed wall stay open): a layout (at wall_joints), a gate (build_gate's
// conversions) and a tower placed note the tiles they touched, and at the end of the next step - their new pieces are on the map then -
// each wall tile there is looked at by its neighbours: a piece points at the two tiles along its axis; a post becomes
// a piece where two pieces point at it from either side (a post in a straight run), or one does and the tile on the
// other side holds a wall, a tower or a gate (a wall's end against another's post or side, a tower, a gate: it
// reaches into it); a piece becomes a post where a piece points at it from aside (a wall ending on its side: the
// joint); a post no piece points at becomes a piece where posts stand on both sides of it along one line (a run the
// game cut into posts beside a tower), else, with no post beside it, where one joint, tower or gate is beside it with
// nothing on its other side: a stub into it. A tower beside a straight wall changes nothing: it points at no tile.
// From the tiles alone, so every PC comes to the same (the walls' state - anims and matrices - is saved);
// the pieces there fitted again, a piece's end into a tower or a gate as into a post. A loaded game's walls are
// looked at once, at its first step (wall_start_fit).
// JOGS: a wall drawn at any other angle than along the tiles or their diagonal steps aside now and then - the layout
// lays it as straight stretches with a post at each end, so where it stepped two round posts touched with a gap between
// them, and at 1:2, the steps every other tile, it was nothing but posts in two rows (the player, 2026-11-08: "is it
// possible to add a connector for round pieces that touch like this? and id also like to handle 1:2 wall if possible").
// A tile holds one object, so a connector is one of the two: both tiles of a step draw the plain straight piece, each
// from the middle of its link to the tile behind to the middle of its link to the tile ahead. Those two lines are one, at
// 1:2 to the tiles, and a whole 1:2 wall is a single straight line through its links' middles. Which tiles do is told by
// the tiles alone: a tile with two neighbours and a 45 degree bend between them, beside another such tile where the wall
// bends back (a sideways step, its two bends opposite) or, over a diagonal link, bends on the same way (a corner of two
// posts, drawn round), or beside a post that stays one over a diagonal link - a junction or a wall's end: the two posts
// would touch with a gap, so the bend draws the piece into it (the player, 2026-11-08: "it's the touching connector pieces
// again"). One bend alone is the wall's corner and keeps its post. A jog is sheared across its length: its
// ends lie along the end of the plain piece it runs on from and meet it corner to corner - square to itself, it stuck
// out past that piece on one side of the bend (the player, 2026-11-08: "can you smoothen that edge when it joins with
// the normal wall again?").
#define WJ_EV_MAX 64       // areas noted, waiting for the next step's end
#define WJ_MARGIN 2         // tiles around what a layout or a gate touched
#define WJ_ROUNDS 4         // looks: a post turned a piece makes the side it reaches a post at the next
#define WJ_MAX 16384        // wall pieces looked at together

struct wj_ev { int x0, y0, x1, y1; uint32_t t; };
internal struct wj_ev g_wj_ev[WJ_EV_MAX];
internal int g_wj_ev_n;
internal unsigned g_wj_to_piece, g_wj_to_post, g_wj_gates;
internal ee_unit *g_wj_units[WJ_MAX];
internal struct wj_at g_wj_want[WJ_MAX];

internal const int k_wj_dir[8][2] = { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 } };

internal int wj_alive(ee_unit *o) {   // a site laid out has no hit points yet; a finished piece without is collapsing
    ee_unit *bs = unit_get_physics(o);
    return unit_get_hp(o) > 0 || (bs && !unit_get_finished(bs));
}
internal int wj_piece_anim(int anim) { return anim == WS_STRAIGHT || anim == WS_DIAG || anim == WS_SLOPE || anim == WS_SLOPE_DIAG; }

// a piece's axis, a step between tiles: its facing (the local y axis), straight along x or y, diagonal both
internal void wj_axis_of(float fx, float fy, int *sx, int *sy) {
    float f = sqrtf(fx * fx + fy * fy);
    if (f > 0.01f) { fx /= f; fy /= f; }
    *sx = fx > 0.3f ? 1 : fx < -0.3f ? -1 : 0;
    *sy = fy > 0.3f ? 1 : fy < -0.3f ? -1 : 0;
}
internal void wj_axis(const ee_unit *u, int *sx, int *sy) { wj_axis_of(unit_get_matrix(u, 1), unit_get_matrix(u, 5), sx, sy); }

// the preview's entry for tile x, y, NULL none
internal struct wj_pv *wj_pv_find(int x, int y) {
    for (unsigned h = ((unsigned)x * 73u + (unsigned)y * 179u) & (WJ_PV_SLOTS - 1); g_wj_pv_slot[h]; h = (h + 1) & (WJ_PV_SLOTS - 1)) {
        struct wj_pv *p = &g_wj_pv[g_wj_pv_slot[h] - 1];
        if (p->x == x && p->y == y) return p;
    }
    return NULL;
}

// the tile's objects (a building on every tile it covers), pl's: its wall's piece or post, else its tower or gate
internal struct wj_at wj_at(const ee_map *map, int x, int y, const ee_player *pl) {
    struct wj_at a = { WJ_NONE, 0, 0, NULL };
    if (g_wj_pv_n && pl == g_wj_pv_pl && GetCurrentThreadId() == g_wj_pv_thread) {
        const struct wj_pv *p = wj_pv_find(x, y);
        if (p) return p->at;
    }
    int n = map ? (int)map_get_size(map) : 0;
    ee_tile *tile = x >= 0 && y >= 0 && x < n && y < n ? map_get_tiles(map)[y * n + x] : NULL;
    for (void *node = tile ? tile_get_objects(tile) : NULL; node; node = tilenode_get_next(node)) {
        ee_unit *o = tilenode_get_object(node);
        ee_unit_type *td = o ? unit_get_type_data(o) : NULL;
        if (!td || unit_get_owner(o) != pl || !wj_alive(o)) continue;
        int t = unit_type_get_id(td), anim = unit_get_anim(o);
        if (ut_role(t, UTR_GATE) || ut_role(t, UTR_TOWER)) { a.kind = WJ_HUB; a.u = o; continue; }
        if (!is_wall(t)) continue;
        if (anim == WS_POST) { a.kind = WJ_POST; a.u = o; return a; }
        if (wj_piece_anim(anim)) {
            a.kind = WJ_PIECE; a.u = o; wj_axis(o, &a.sx, &a.sy);
            if (anim == WS_STRAIGHT && a.sx && a.sy) { a.kind = WJ_JOG; a.sx = a.sy = 0; }   // only a jog is slanted
            return a;
        }
    }
    return a;
}

// the neighbours (of any kind) among the 8 around x, y: how many, the first two's directions
internal int wj_links(const ee_map *map, int x, int y, const ee_player *pl, int *a, int *b) {
    int n = 0;
    for (int k = 0; k < 8; k++) {
        if (wj_at(map, x + k_wj_dir[k][0], y + k_wj_dir[k][1], pl).kind == WJ_NONE) continue;
        if (n == 0) *a = k; else if (n == 1) *b = k;
        n++;
    }
    return n;
}

// whether x, y is a tile of a jog (JOGS above), its two links in a, b (the 8 directions' numbers)
internal int wj_jog(const ee_map *map, int x, int y, const ee_player *pl, int *a, int *b) {
    if (wj_links(map, x, y, pl, a, b) != 2) return 0;
    int gap = (*a - *b) & 7;
    if (gap != 3 && gap != 5) return 0;
    for (int side = 0; side < 2; side++) {
        int to = side ? *b : *a, from = side ? *a : *b, in = (from + 4) & 7;   // the wall comes along in, goes on along to
        int nx = x + k_wj_dir[to][0], ny = y + k_wj_dir[to][1], na = 0, nb = 0;
        if (wj_at(map, nx, ny, pl).kind == WJ_HUB) continue;
        int links = wj_links(map, nx, ny, pl, &na, &nb);
        if (links != 2) {   // a junction or a wall's end, a post for good, touching over a diagonal link: this tile runs into it
            if (to & 1) return 1;
            continue;
        }
        int out = na == ((to + 4) & 7) ? nb : na, turn = (to - in) & 7, next = (out - to) & 7;   // 1 or 7: 45 degrees
        if (next != 1 && next != 7) continue;
        if (next != turn || (to & 1)) return 1;
    }
    return 0;
}

// a jog's facing from its link b to its link a, one way for every piece of it (y growing): a, b swapped to match
internal void wj_chord(int *a, int *b, float *fx, float *fy) {
    int cx = k_wj_dir[*a][0] - k_wj_dir[*b][0], cy = k_wj_dir[*a][1] - k_wj_dir[*b][1];
    if (cy < 0) { int t = *a; *a = *b; *b = t; cx = -cx; cy = -cy; }
    float l = 1.0f / sqrtf((float)(cx * cx + cy * cy));
    *fx = (float)cx * l; *fy = (float)cy * l;
}

// whether the tile along link m from x, y holds a plain piece running along that link
internal int wj_along(const ee_map *map, int x, int y, int m, const ee_player *pl) {
    struct wj_at nb = wj_at(map, x + k_wj_dir[m][0], y + k_wj_dir[m][1], pl);
    return nb.kind == WJ_PIECE && ((nb.sx == k_wj_dir[m][0] && nb.sy == k_wj_dir[m][1]) || (nb.sx == -k_wj_dir[m][0] && nb.sy == -k_wj_dir[m][1]));
}

// which of a jog's links its ends lie square to: its diagonal one (odd) where a plain diagonal piece runs on from it,
// else its straight one - a whole 1:2 wall the same way, so its pieces meet end to end
internal int wj_jog_axis(const ee_map *map, int x, int y, const ee_player *pl, int a, int b) {
    int diag = a & 1 ? a : b, straight = a & 1 ? b : a;
    return wj_along(map, x, y, diag, pl) ? diag : straight;
}

// how far a jog's end along link m goes on: into a post or a tower; not at all where it meets its neighbour's end along
// the same line (a plain piece its ends are square to, a piece of the same jog), whose faces would flicker; else past
// the neighbour, the wall bends there
internal float wj_jog_end(const ee_map *map, int x, int y, int m, const ee_player *pl, float fx, float fy, int axis) {
    int nx = x + k_wj_dir[m][0], ny = y + k_wj_dir[m][1], a, b;
    if (ws_post_at(map, nx, ny, pl)) return WS_INTO_POST;
    if (wj_along(map, x, y, m, pl)) return m == axis ? 0 : WS_JOG_OVER;
    if (wj_jog(map, nx, ny, pl, &a, &b)) {
        float gx, gy;
        int theirs = wj_jog_axis(map, nx, ny, pl, a, b);
        wj_chord(&a, &b, &gx, &gy);
        if (fabsf(fx * gy - fy * gx) < 0.01f && ((theirs ^ axis) & 3) == 0) return 0;
    }
    return WS_JOG_OVER;
}

// where the piece of a jog on x, y stands (ws_fit); 0 when x, y is none
internal int wj_jog_place(const ee_map *map, int x, int y, const ee_player *pl, struct wj_jog_fit *j) {
    int a, b;
    if (!wj_jog(map, x, y, pl, &a, &b)) return 0;
    wj_chord(&a, &b, &j->fx, &j->fy);
    j->ox = (float)(k_wj_dir[a][0] + k_wj_dir[b][0]) * 0.25f;
    j->oy = (float)(k_wj_dir[a][1] + k_wj_dir[b][1]) * 0.25f;
    // across it: square to its axis link, the same way round as square to itself (its faces' winding)
    int axis = wj_jog_axis(map, x, y, pl, a, b);
    float dx = (float)k_wj_dir[axis][0], dy = (float)k_wj_dir[axis][1], l = 1.0f / sqrtf(dx * dx + dy * dy);
    j->ax = dy * l; j->ay = -dx * l;
    j->diag = axis & 1;
    if (j->ax * j->fy - j->ay * j->fx < 0) { j->ax = -j->ax; j->ay = -j->ay; }
    j->ea = wj_jog_end(map, x, y, b, pl, j->fx, j->fy, axis);
    j->eb = wj_jog_end(map, x, y, a, pl, j->fx, j->fy, axis);
    return 1;
}

// whether a piece of pl's among the 8 around x, y points at it
internal int wj_pointed(const ee_map *map, int x, int y, const ee_player *pl) {
    for (int k = 0; k < 8; k++) {
        int dx = k_wj_dir[k][0], dy = k_wj_dir[k][1];
        struct wj_at nb = wj_at(map, x + dx, y + dy, pl);
        if (nb.kind == WJ_PIECE && ((nb.sx == dx && nb.sy == dy) || (nb.sx == -dx && nb.sy == -dy))) return 1;
    }
    return 0;
}

// what wall tile x, y (now self) should be by its 8 neighbours
internal struct wj_at wj_want(const ee_map *map, int x, int y, struct wj_at self, const ee_player *pl) {
    const int (*dir)[2] = k_wj_dir;
    int pt[8], np = 0;   // the neighbours whose axis points at x, y
    for (int k = 0; k < 8; k++) {
        struct wj_at nb = wj_at(map, x + dir[k][0], y + dir[k][1], pl);
        if (nb.kind == WJ_PIECE && ((nb.sx == dir[k][0] && nb.sy == dir[k][1]) || (nb.sx == -dir[k][0] && nb.sy == -dir[k][1])))
            pt[np++] = k;
    }
    struct wj_at w = self;
    int ja, jb;
    if (wj_jog(map, x, y, pl, &ja, &jb)) { w.kind = WJ_JOG; w.sx = w.sy = 0; return w; }
    if (self.kind == WJ_JOG) { w.kind = WJ_POST; return w; }   // its neighbours changed: a bend, or a wall's end
    if (self.kind == WJ_PIECE) {   // pointed at from aside: the joint
        for (int i = 0; i < np; i++) {
            int dx = dir[pt[i]][0], dy = dir[pt[i]][1];
            if (!((dx == self.sx && dy == self.sy) || (dx == -self.sx && dy == -self.sy))) { w.kind = WJ_POST; return w; }
        }
        return w;
    }
    // a post: the new piece along the pointing one's axis, turned as it is
    int k = pt[0];
    if (np == 2 ? pt[1] - pt[0] == 4 : np == 1 && wj_at(map, x - dir[k][0], y - dir[k][1], pl).kind != WJ_NONE) {
        struct wj_at nb = wj_at(map, x + dir[k][0], y + dir[k][1], pl);
        w.kind = WJ_PIECE; w.sx = nb.sx; w.sy = nb.sy;
    }
    if (np) return w;
    // none points at it, posts on both sides along one line: a run the game cut into posts - its wall_joints counts
    // a tower among a piece's neighbours, so a wall laid past a tower (a wall manager lays its whole wall again to
    // mend it) got posts all along beside it
    struct wj_at nb[8];
    for (int j = 0; j < 8; j++) nb[j] = wj_at(map, x + dir[j][0], y + dir[j][1], pl);
    int line = -1, nl = 0, posts = 0;
    for (int j = 0; j < 8; j++) {
        posts += nb[j].kind == WJ_POST;
        if (j < 4 && (nb[j].kind == WJ_POST || nb[j].kind == WJ_JOG) && (nb[j + 4].kind == WJ_POST || nb[j + 4].kind == WJ_JOG)) { line = j; nl++; }
    }
    if (nl == 1) { w.kind = WJ_PIECE; w.sx = dir[line][0]; w.sy = dir[line][1]; return w; }
    // no post beside it but one joint (a post pieces point at), or a tower or a gate, with nothing on its other side: a
    // stub into it - a wall's first tile beside a gate, a post the game made at a gap beside a joint. Never with
    // posts about it: which way the wall runs there is the line's
    int cand = -1, nc = 0;
    for (int j = 0; j < 8; j++)
        if ((nb[j].kind == WJ_HUB || (nb[j].kind == WJ_POST && wj_pointed(map, x + dir[j][0], y + dir[j][1], pl))) &&
            nb[(j + 4) % 8].kind == WJ_NONE) { cand = j; nc++; }
    if (nc == 1 && posts <= (nb[cand].kind == WJ_POST)) { w.kind = WJ_PIECE; w.sx = dir[cand][0]; w.sy = dir[cand][1]; }
    return w;
}

// a matrix upright on tile tx, ty as the layout leaves a piece: facing fx, fy (a unit vector), on its tile's corner x, y
internal void wj_upright_m(const ee_map *map, float *m, int tx, int ty, float fx, float fy) {
    float z = ground_corner(map, tx, ty);
    const float up[12] = { fy, fx, 0, (float)tx + 0.5f,  -fx, fy, 0, (float)ty + 0.5f,  0, 0, 1, z == z ? z : m[11] };
    memcpy(m, up, sizeof up);
}
internal void wj_upright(const ee_map *map, ee_unit *u, float fx, float fy) {
    float m[12];
    for (int i = 0; i < 12; i++) m[i] = unit_get_matrix(u, i);
    wj_upright_m(map, m, unit_get_x(u), unit_get_y(u), fx, fy);
    for (int i = 0; i < 12; i++) unit_set_matrix(u, i, m[i]);
}

// the piece on tile x, y turned into w: a piece along sx, sy, a post or a jog - matrix m upright (the fit after puts it
// onto the ground), its animation returned
internal int wj_turned(const ee_map *map, int x, int y, const ee_player *pl, struct wj_at w, float *m) {
    float l = w.sx && w.sy ? sqrtf(0.5f) : 1.0f;
    int a, b;
    float jx = 0, jy = 1;
    if (w.kind == WJ_JOG && wj_jog(map, x, y, pl, &a, &b)) wj_chord(&a, &b, &jx, &jy);
    if (w.kind == WJ_PIECE) wj_upright_m(map, m, x, y, (float)w.sx * l, (float)w.sy * l);
    else wj_upright_m(map, m, x, y, jx, jy);
    return w.kind == WJ_POST ? WS_POST : w.sx && w.sy ? WS_DIAG : WS_STRAIGHT;
}
internal void wj_turn(const ee_map *map, ee_unit *u, struct wj_at w) {
    float m[12];
    for (int i = 0; i < 12; i++) m[i] = unit_get_matrix(u, i);
    unit_start_anim(u, wj_turned(map, unit_get_x(u), unit_get_y(u), unit_get_owner(u), w, m), -1, 0);
    for (int i = 0; i < 12; i++) unit_set_matrix(u, i, m[i]);
    if (w.kind == WJ_POST) g_wj_to_post++; else g_wj_to_piece++;
}

// a fitted matrix (stretched, sheared, taller) put back upright - a piece about to become a gate's, a gate's unit of a
// game saved before gates were (the gate's middle three kept the fitted piece's: a diagonal one drawn at 0.63 of its
// length, sheared)
internal int wj_unfit(const ee_map *map, ee_unit *u) {
    float fx = unit_get_matrix(u, 1), fy = unit_get_matrix(u, 5), f = sqrtf(fx * fx + fy * fy);
    if (f < 0.01f || (fabsf(f - 1) < 0.001f && unit_get_matrix(u, 8) == 0 && unit_get_matrix(u, 9) == 0 && fabsf(unit_get_matrix(u, 10) - 1) < 0.001f)) return 0;
    wj_upright(map, u, fx / f, fy / f);
    return 1;
}

// the wall tiles in x0, y0 .. x1, y1 (all < 0: every wall) looked at, then fitted; what: for the log
internal void wj_area(int x0, int y0, int x1, int y1, const char *what) {
    ee_map *map = game_map();
    if (!map) return;
    int n = 0, all = x0 < 0 && y0 < 0 && x1 < 0 && y1 < 0;
    unsigned piece0 = g_wj_to_piece, post0 = g_wj_to_post, gates0 = g_wj_gates;
    struct grid_it it = all ? grid_it_all(1) : grid_it_box((x0 + x1) / 2, (y0 + y1) / 2, (x1 - x0 > y1 - y0 ? x1 - x0 : y1 - y0) / 2 + 1, 1);
    while (grid_it_next(&it)) {
        int t = unit_type_get_id(it.td), x = unit_get_x(it.u), y = unit_get_y(it.u);
        if (!is_wall(t) || !wj_alive(it.u) || (!all && (x < x0 || x > x1 || y < y0 || y > y1))) continue;
        if (ut_role(t, UTR_GATE)) { if (all) g_wj_gates += (unsigned)wj_unfit(map, it.u); }
        else if (n < WJ_MAX) g_wj_units[n++] = it.u;
    }
    // each look from the tiles as they are, then the turns
    for (int r = 0; g_wj_on && r < WJ_ROUNDS; r++) {
        int turned = 0;
        for (int i = 0; i < n; i++) {
            ee_unit *u = g_wj_units[i];
            ee_player *pl = unit_get_owner(u);
            struct wj_at self = wj_at(map, unit_get_x(u), unit_get_y(u), pl);
            struct wj_at w = self.u == u ? wj_want(map, unit_get_x(u), unit_get_y(u), self, pl) : self;
            w.u = self.u == u && (w.kind != self.kind || w.sx != self.sx || w.sy != self.sy) ? u : NULL;
            g_wj_want[i] = w;
        }
        for (int i = 0; i < n; i++)
            if (g_wj_want[i].u) { wj_turn(map, g_wj_units[i], g_wj_want[i]); turned++; }
        if (!turned) break;
    }
    unsigned fitted = 0;
    for (int i = 0; i < n; i++) fitted += (unsigned)ws_fit(g_wj_units[i]);
    if (g_log && ((all && !g_ws_ed_seen) || g_wj_to_piece != piece0 || g_wj_to_post != post0 || g_wj_gates != gates0))
        fprintf(g_log, "# walls: %s at %.2f min: %u posts made pieces, %u pieces posts, %u gate units upright, %u pieces fitted\n",
                what, (double)game_time() / 60000.0, g_wj_to_piece - piece0, g_wj_to_post - post0,
                g_wj_gates - gates0, fitted);
}

// the tiles around x0, y0 .. x1, y1 looked at at the next step's end (wj_add: also without the joins - the editor's)
internal void wj_add(int x0, int y0, int x1, int y1) {
    struct wj_ev e = { x0 - WJ_MARGIN, y0 - WJ_MARGIN, x1 + WJ_MARGIN, y1 + WJ_MARGIN, game_time() };
    struct wj_ev *last = g_wj_ev_n ? &g_wj_ev[g_wj_ev_n - 1] : NULL;
    if (last && (g_wj_ev_n == WJ_EV_MAX || (last->t == e.t && e.x0 <= last->x1 + 1 && last->x0 <= e.x1 + 1 && e.y0 <= last->y1 + 1 && last->y0 <= e.y1 + 1))) {
        if (e.x0 < last->x0) last->x0 = e.x0;
        if (e.y0 < last->y0) last->y0 = e.y0;
        if (e.x1 > last->x1) last->x1 = e.x1;
        if (e.y1 > last->y1) last->y1 = e.y1;
        last->t = e.t;
        return;
    }
    g_wj_ev[g_wj_ev_n++] = e;
}
internal void wj_note(int x0, int y0, int x1, int y1) { if (g_wj_on) wj_add(x0, y0, x1, y1); }

internal void __cdecl hook_wall_piece(ee_unit *u, int code, int raised) {
    wall_piece(u, code, raised);
    if (!g_ws_on) return;
    ws_fit(u);
    if (u && g_ws_laid_n < WS_LAID_MAX) g_ws_laid[g_ws_laid_n++] = u;
}

// where walls meet or cross, the layout's last step (unit_functions.wall_joints 0x62E73B) makes a piece with more than
// two neighbours a post, its matrix anew and flat: the wall's pieces fitted again (a fit twice gives the same), and
// the walls' pieces around them - a piece of an older wall beside a new post reaches into it now; the joins noted
internal void __cdecl hook_wall_joints(ee_unit_vec *pieces, void *tiles, ee_player *pl, ee_map *map) {
    wall_joints(pieces, tiles, pl, map);
    if (!g_ws_on) { g_ws_laid_n = 0; return; }
    // the pieces laid out, all of them there now (when each was laid its neighbours weren't yet), and the joints' own
    // (an older wall's, made a post); then the older walls' pieces around both (by the unit grid: the new pieces come
    // onto it only at the next step - ws_post_at finds them in the list)
    ee_unit **pb = pieces ? unit_vec_get_begin(pieces) : NULL, **pe = pieces ? unit_vec_get_end(pieces) : NULL;
    int nv = pb && pe > pb ? (int)(pe - pb) : 0, x0 = 1 << 30, y0 = 1 << 30, x1 = -1, y1 = -1;
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < g_ws_laid_n + nv; i++) {
            ee_unit *u = i < g_ws_laid_n ? g_ws_laid[i] : pb[i - g_ws_laid_n];
            if (!u || unit_get_hp(u) <= 0) continue;
            int x = unit_get_x(u), y = unit_get_y(u);
            if (!pass) {
                ws_fit(u);
                if (x < x0) x0 = x;
                if (y < y0) y0 = y;
                if (x > x1) x1 = x;
                if (y > y1) y1 = y;
                continue;
            }
            for (struct grid_it it = grid_it_box(x, y, 1, 1); grid_it_next(&it); ) {
                int t = unit_type_get_id(it.td);
                if (it.u != u && abs(unit_get_x(it.u) - x) <= 1 && abs(unit_get_y(it.u) - y) <= 1 && is_wall(t) &&
                    !ut_role(t, UTR_GATE) && unit_get_hp(it.u) > 0)
                    ws_fit(it.u);
            }
        }
    g_ws_laid_n = 0;
    if (x1 >= 0) wj_note(x0, y0, x1, y1);
}

// THE PREVIEW: while a wall is dragged out the game makes a placement ghost per tile (wall_ghosts), each the piece the
// layout gives its tile, flat at the ground under its tile's middle and at its model's own length - posts at every step of
// a slanted wall, none of the joins or the fit a built wall gets (the player, 2026-11-08: "can you make the building
// preview display our actual wall"). So the ghosts go through the same two: their tiles laid over the map for wj_at, wj_want
// looks at them as it does at a new wall's pieces, ws_fit_piece puts each on the ground. Only the picture: a ghost is no
// map object, and an older wall beside it stays as it is until the wall is built. The game's calls hold the ghosts' lock
// around this, so the drawing never sees a ghost half done. A tile under a tower or a gate gets no piece, as in the layout
// - its ghost left as the game made it; a tile that can't be built on (a red ghost) counts, the line drawn whole.
// Its tiles were the game's own mistake: the mouse's line is sampled a tile a step (GE2DPoint::InterpolateTo), the wall
// is laid by another line (wall_line) with its ends carried on into the same type of wall they meet (wall_extend) - a
// slanted wall was built on other tiles than it was previewed on (the player, 2026-11-08: "the line that gets previewed
// and the one that's built aren't the same"). So the ghosts are made of the built wall's tiles, from the sampled line's
// ends (the sampling ends on the mouse's own tile); those functions only read the map. The scenario editor's wall
// (EESEAPlaceWall) is laid by the same placing: its preview goes the same way
internal void wj_ghosts_fit(ee_map *map, ee_player *pl) {
    void *v = ghost_units();
    int n = 0;
    memset(g_wj_pv_slot, 0, sizeof g_wj_pv_slot);
    for (uint32_t i = 0; i < vector_size(v) && n < WJ_PV_MAX; i++) {
        ee_ghost_unit *g = *(ee_ghost_unit **)type_stats_game(v, (int)i);
        int x = ghost_unit_get_tile_x(g), y = ghost_unit_get_tile_y(g), anim = ws_plain_anim(ghost_unit_get_anim(g));
        unsigned h = ((unsigned)x * 73u + (unsigned)y * 179u) & (WJ_PV_SLOTS - 1);
        if ((anim != WS_STRAIGHT && anim != WS_DIAG && anim != WS_POST) || wj_pv_find(x, y) || wj_at(map, x, y, pl).kind == WJ_HUB) continue;
        if (anim != ghost_unit_get_anim(g)) ghost_unit_start_anim(g, anim);
        struct wj_pv *p = &g_wj_pv[n];
        *p = (struct wj_pv){ x, y, { anim == WS_POST ? WJ_POST : WJ_PIECE, 0, 0, NULL }, g };
        if (anim != WS_POST) wj_axis_of(ghost_unit_get_transform(g, 1), ghost_unit_get_transform(g, 5), &p->at.sx, &p->at.sy);
        while (g_wj_pv_slot[h]) h = (h + 1) & (WJ_PV_SLOTS - 1);
        g_wj_pv_slot[h] = (int16_t)++n;
    }
    g_wj_pv_pl = pl;
    g_wj_pv_thread = GetCurrentThreadId();
    g_wj_pv_n = n;
    for (int r = 0; g_wj_on && r < WJ_ROUNDS; r++) {
        static struct wj_at want[WJ_PV_MAX];
        int turned = 0;
        for (int i = 0; i < n; i++) want[i] = wj_want(map, g_wj_pv[i].x, g_wj_pv[i].y, g_wj_pv[i].at, pl);
        for (int i = 0; i < n; i++) {
            struct wj_pv *p = &g_wj_pv[i];
            if (want[i].kind == p->at.kind && want[i].sx == p->at.sx && want[i].sy == p->at.sy) continue;
            p->at = want[i];
            ghost_unit_start_anim(p->g, wj_turned(map, p->x, p->y, pl, want[i], ghost_unit_transform_at(p->g)));
            turned++;
        }
        if (!turned) break;
    }
    for (int i = 0; i < n; i++) {
        ee_ghost_unit *g = g_wj_pv[i].g;
        const struct ws_piece p = { g_wj_pv[i].x, g_wj_pv[i].y, ghost_unit_get_anim(g), unit_type_get_id(ghost_unit_get_type_data(g)), ghost_unit_scale(g), pl, ghost_unit_model(g) };
        ws_fit_piece(&p, ghost_unit_transform_at(g));
    }
    g_wj_pv_n = 0;
}
internal void __cdecl hook_wall_ghosts(ee_tile_vec *tiles, void *codes, uint32_t tech, ee_player *pl, float rotation) {
    ee_map *map = game_map();
    const int *tb = tile_vec_get_begin(tiles), *te = tile_vec_get_end(tiles);
    ee_tech *t = pl ? tech_tree_lookup(player_get_tech_tree(pl), (int)tech) : NULL;
    ee_unit_type *td = t ? type_data(tech_data_get_unit_type(tech_get_data(t))) : NULL;
    if (!g_ws_on || !map || !td || !tb || te <= tb) wall_ghosts(tiles, codes, tech, pl, rotation);
    else {
        float from[2] = { (float)tb[0] + 0.5f, (float)tb[1] + 0.5f }, to[2] = { (float)te[-2] + 0.5f, (float)te[-1] + 0.5f };
        void *line[4] = { NULL }, *built[4] = { NULL }, *laid[4] = { NULL };   // the runtime library's vectors, empty
        wall_line(from, to, (ee_tile_vec *)line);
        wall_extend((ee_tile_vec *)line, pl, td, (ee_tile_vec *)built);
        wall_layout((ee_tile_vec *)built, laid, map);
        wall_ghosts((ee_tile_vec *)built, laid, tech, pl, rotation);
        vector_free(line);
        vector_free(built);
        vector_free(laid);
    }
    if (g_ws_on && map) wj_ghosts_fit(map, pl);
}

// a gate laid into a wall (build_gate 0x6022E3: the gate button's, the AI's) makes each of its five pieces anew as the
// gate's (unit_convert): its two ends get a matrix of their own, its middle three keep the piece's - a fitted piece is
// put back upright first, the gate as the game makes it (wall_slopes alone too); its tiles noted for the joins
internal int __cdecl hook_gate_convert(ee_unit *u, int pi, ee_unit_type *td) {
    ee_map *map = game_map();
    if (g_ws_on && u && map) wj_unfit(map, u);
    int x = u ? unit_get_x(u) : -1, y = u ? unit_get_y(u) : -1;
    int r = unit_convert(u, pi, td);
    if (g_ws_on && x >= 0) wj_note(x, y, x, y);
    return r;
}

// a wall's line upgrade (the game's, the AI's for what it left behind: defence.c) makes each piece anew as the next era's,
// which keeps the fit of the old one - the era's mesh is another length, and its skirt was only asked for by a fit (the
// player, 2026-11-08: "after epoch wall upgrades the pieces misfit and the skirt don't appear"): looked at and fitted at
// the next step's end, as a gate's pieces are
internal void walls_converted(int type, int x, int y) { if (g_ws_on && is_wall(type)) wj_add(x, y, x, y); }
internal int __cdecl hook_upgrade_convert(ee_unit *u, int pi, ee_unit_type *td) {
    int x = u ? unit_get_x(u) : -1, y = u ? unit_get_y(u) : -1;
    int r = unit_convert(u, pi, td);
    if (x >= 0 && td) walls_converted(unit_type_get_id(td), x, y);
    return r;
}

// a tower placed (place_foundation's calls place_building: the player's, the AI's build manager's): its tiles noted - a
// wall's end post in line before it becomes a piece into it. A tower beside a straight wall leaves its pieces be: a
// piece turns a post only where another piece points at it from aside, and a tower points at nothing (the game's
// wall_joints counts towers among a piece's neighbours - a wall laid past a tower got a post beside it, a piece again
// at its joins: pointed at from either side)
// With building_lean what stands on the ground follows it: a type that leans with the ground (its type's pitches, banks:
// the AA guns) stood onto it as the map's place stands a spawned one - laid out in a game it stood level, its legs in the
// hill on one side; farms, the resources but trees (mines, forage bushes) and the ambient objects (debris, rocks, bones,
// wrecks, plants) sheared onto the ground's slope under them (the player, 2026-11-05: "could you apply the lean to debris
// too? the resources/mines could also profit from that") - their upright parts stay upright (a farm's crop rows, a
// lamp), their foot on the ground on every side; one not standing on the ground (lily pads on the water) left alone
internal int g_bl_on;

#define BL_ON_GROUND 0.1f   // a sheared object stands on the ground: its height this near the ground's at its middle

internal int bl_sheared(const ee_unit_type *td) {
    enum unit_class c = unit_type_get_class_id(td);
    if (c == CLS_AMBIENT) return 1;
    if (c == CLS_RESOURCE) return unit_type_get_resource_type(td) > 0 && unit_type_get_resource_type(td) != 3;   // 3 wood: trees; 0 scaffolding, foundations, markers
    return unit_type_get_id(td) == UT(B_FARM);
}

// m (3 x 4, by row: a unit's matrix, a draw record's) sheared onto the ground's slope over a footprint of size tiles
// (half a tile out at least): z rises by gx, gy per tile of x, y - its local x and y by their world x, y (their own
// scale and turn kept). on_ground: only when its height is the ground's at its middle (not a lily pad on the water; a
// building is left to its own height). 0: left as it was
internal int bl_shear(const ee_map *map, float *m, int size, int on_ground) {
    float g = ground_at(map, m[3], m[7]);
    if (g != g || (on_ground && fabsf(m[11] - g) > BL_ON_GROUND)) return 0;
    float r = size > 1 ? 0.5f * (float)size : 0.5f;
    float gx = (ground_at(map, m[3] + r, m[7]) - ground_at(map, m[3] - r, m[7])) / (2 * r);
    float gy = (ground_at(map, m[3], m[7] + r) - ground_at(map, m[3], m[7] - r)) / (2 * r);
    if (gx != gx || gy != gy) return 0;
    m[8] = gx * m[0] + gy * m[4];
    m[9] = gx * m[1] + gy * m[5];
    return 1;
}

// u onto the ground by the game's stand (a leaning type's, else nothing); 1 when its matrix changed. Only a unit with its
// physics part has a matrix (+0x130): an ambient object or a resource site is the plain EEUnit (0x48 bytes). The
// sheared kinds are sheared where they are drawn (hook_draw_record_fill below): the game's state stays as it was
internal int bl_ground(ee_unit *u, const ee_unit_type *td) {
    if (!unit_get_physics(u) || unit_get_x(u) < 0 || (unit_type_get_kind_flags(td) & 5)) return 0;   // off the map; an aircraft's, a ship's type
    float m[12];
    for (int i = 0; i < 12; i++) m[i] = unit_get_matrix(u, i);
    if (unit_type_get_pitches(td) || unit_type_get_banks(td)) {
        float p[3] = { m[3], m[7], m[11] };
        unit_stand(u, p);
    } else return 0;
    for (int i = 0; i < 12; i++) if (fabsf(unit_get_matrix(u, i) - m[i]) > 1e-4f) return 1;
    return 0;
}

// every unit on the map onto the ground: the scenario editor's placing and moving pitch a leaning one only, never roll
// it (an AA gun on a hill falling sideways sank in on one side and floated on the other - the player, 2026-11-05: "only
// changing the terrain forces it to place correctly"), its turning stands it level; a game's start mends what a
// scenario saved so. What: for the log, which names how many changed
internal void bl_ground_all(const char *what) {
    ee_map *map = game_map();
    int n = 0;
    for (struct grid_it it = grid_it_all(1); map && grid_it_next(&it); ) n += bl_ground(it.u, it.td);
    if (n && g_log) fprintf(g_log, "# walls: building_lean: %d units put onto the ground (%s)\n", n, what);
}

// BENT OBJECTS: a shear fits a plane - on a curved slope a big object (debris over 4 tiles between hills) still floated
// at its ends and sank in its middle (the player, 2026-11-05: "this is still a bit silly, could it follow the same
// transform the farms use to fit the ground" - a farm's ground is a mesh of its own, each corner at the ground's height,
// 0x4E4F4A), and a sheared matrix lights the model by its skew (a 2x2 debris on a steep spot drawn dark, squashed: "the
// 2x2 debris is still messy"). So an ambient object or a resource site keeps the level matrix the collection gives it,
// and its model is bent instead: shared by every object of its kind, but the renderer copies the frame's vertices into
// its vertex buffer at every draw (CopyStdVertices) at the model's transformation, which the exe sets to the object's
// before - so there each vertex goes up or down by the ground's height under it over the ground's at its middle (its
// model z by that over the transformation's z scale), and back after the draw. Over a bulge a vertex goes up by as much
// as the ground under it stands above the ground around it (BL_SPAN): the debris' base is a few big triangles (75
// vertices, drawn at twice their size for the 2x2), whose middles went under a bulge between their corners - the
// highest ground around tried first lifted everything on a slope ("the mines now seem to float a bit"). A debris draw
// is a decal to the shadows (g_sh_decal): bent, its vertices no longer lie flat. Its models: every animation's of its type's graphics
// (BL_ANIMS), noted when its record is filled (a hash of the graphics objects seen: a lookup a frame)
#define BL_MODELS 512       // models bent (the plain sheared kinds': ambient objects, resources)
#define BL_GFX 1024         // graphics objects seen (a power of two)
#define BL_ANIMS 71         // EEGraphicObject's models, one per animation (+0x234 .. +0x350)
#define BL_BEND_MAX 8192    // vertices of a frame bent at most
#define BL_SPAN 0.25f       // a bent vertex's bulge: the ground this far around it (a tile, by the model's scale)
struct bl_model { ee_model *m; int decal; };   // decal: debris - casts no shadow
internal struct bl_model g_bl_models[BL_MODELS];
internal void *g_bl_gfx[BL_GFX];
internal float g_bl_z[BL_BEND_MAX];   // the bent vertices' own z, put back after the draw (the graphics thread's)

internal void bl_mark(ee_unit_stats *st, int decal) {
    ee_dmg_model *gfx = st ? unit_stats_get_gfx(st) : NULL;
    if (!gfx) return;
    unsigned h = (unsigned)((uintptr_t)gfx >> 4) & (BL_GFX - 1);
    for (int k = 0; k < BL_GFX && g_bl_gfx[h]; k++, h = (h + 1) & (BL_GFX - 1))
        if (g_bl_gfx[h] == gfx) return;
    g_bl_gfx[h] = gfx;
    for (int a = 0; a < BL_ANIMS; a++) {
        ee_model *m = ((ee_model **)dmg_model_anim_model_at(gfx))[a];
        int n = g_bl_n, dup = 0;
        for (int i = 0; i < n && !dup; i++) dup = g_bl_models[i].m == m;
        if (!m || dup || n >= BL_MODELS) continue;
        g_bl_models[n] = (struct bl_model){ m, decal };
        InterlockedIncrement(&g_bl_n);   // after the entry: the graphics thread reads up to the count
    }
}

// the graphics' models are bent (bl_mark noted it): render_units.c leaves its records to the game's draw, as the bend
// reads the model's frame and place as that draw sets them
internal int bl_noted(const void *gfx) {
    unsigned h = (unsigned)((uintptr_t)gfx >> 4) & (BL_GFX - 1);
    for (int k = 0; k < BL_GFX && g_bl_gfx[h]; k++, h = (h + 1) & (BL_GFX - 1))
        if (g_bl_gfx[h] == gfx) return 1;
    return 0;
}

// the model's place among the noted, -1 for none
internal int bl_index(const ee_model *m) {
    int n = g_bl_n, i = 0;
    while (i < n && g_bl_models[i].m != m) i++;
    return i < n ? i : -1;
}
// a noted model's decal flag (1: casts no shadow), -1 for a model not noted
internal int bl_noted_decal(const ee_model *m) {
    int i = bl_index(m);
    return i < 0 ? -1 : g_bl_models[i].decal != 0;
}
// frame f of a model bent onto the ground under its transformation t: its vertices in place (their heights kept in
// g_bl_z, to be put back), or into out (a copy, D3DVERTEX each: render_units.c keeps a standing object's); 0 when it
// isn't bent (no such frame, too many vertices, not standing on the ground)
internal int bl_bend_at(ee_model *m, uint32_t f, const float *t, float *out) {
    ee_vertices **va = model_get_vertices(m);
    ee_map *map = game_map();
    if (!va || !map || f >= model_get_frames(m) || !va[f]) return 0;
    ee_vertices *a = va[f];
    uint32_t cnt = vertices_get_count(a);
    ee_vertex *data = vertices_get_data(a);
    if (!data || cnt > BL_BEND_MAX || fabsf(t[10]) < 1e-3f) return 0;
    // level at the ground's height at its middle, as the collection made it (one not standing there - a lily pad on the
    // water - left as it is)
    float g0 = ground_at(map, t[3], t[7]);
    if (g0 != g0 || fabsf(t[11] - g0) > BL_ON_GROUND) return 0;
    float span = BL_SPAN * sqrtf(t[0] * t[0] + t[4] * t[4]);
    for (uint32_t k = 0; k < cnt; k++) {
        float *v = (float *)vertex_at(data, k);
        float px = t[0] * v[0] + t[1] * v[1] + t[2] * v[2] + t[3], py = t[4] * v[0] + t[5] * v[1] + t[6] * v[2] + t[7];
        // and over a bulge by as much as it stands above the ground around it (BL_SPAN of the model's scale): a big
        // triangle between bent corners stays over the ground under its middle - nothing on an even slope
        float g = ground_at(map, px, py), around = 0;
        for (int q = 0; q < 4; q++) around += 0.25f * ground_at(map, px + (q & 1 ? span : -span), py + (q & 2 ? span : -span));
        float d = g + fmaxf(0, g - around) - g0;
        if (out) {
            memcpy(out + k * 8, v, 8 * sizeof(float));
            if (d == d) out[k * 8 + 2] += d / t[10];
            continue;
        }
        g_bl_z[k] = v[2];
        if (d == d) v[2] += d / t[10];
    }
    return 1;
}
// m's frame bent onto the ground at its transformation, when it is a noted model: its vertex array (to put back), else
// NULL
internal ee_vertices *bl_bend(ee_model *m, int *decal) {
    int i = bl_index(m);
    if (i < 0) return NULL;
    *decal = g_bl_models[i].decal;
    uint32_t f = model_get_frame(m);
    if (!bl_bend_at(m, f, model_transform_at(m), NULL)) return NULL;
    return model_get_vertices(m)[f];
}
internal void bl_unbend(ee_vertices *a) {
    uint32_t cnt = vertices_get_count(a);
    ee_vertex *data = vertices_get_data(a);
    for (uint32_t k = 0; k < cnt && k < BL_BEND_MAX; k++) ((float *)vertex_at(data, k))[2] = g_bl_z[k];
}

// a sheared kind's drawing onto the ground - every frame, the drawing only (the game's state untouched, the same on every
// PC whatever this one draws): a farm's record (its unit's matrix, level as laid out) sheared; an ambient object or a
// resource site (no matrix of its own: the collection turns it by its heading at the ground's height, level) its model
// noted, to be bent where it is drawn
// (after the fill: collect.c's walk of the world's copy calls it with the copy's type, complex unit and stats)
internal void walls_after_fill(ee_unit_type *td, int has_physics, ee_unit_stats *st, float *rec) {
    ee_map *map = game_map();
    if (!td || !map || !rec || !bl_sheared(td)) return;
    if (has_physics) bl_shear(map, rec, unit_type_get_size(td), unit_type_get_class_id(td) != CLS_BUILDING);
    else if (st) bl_mark(st, UT_IN(unit_type_get_id(td), UT_AMB_DEBRIS_1X, UT_AMB_DEBRIS_2X, UT_AMB_DEBRIS_3X, UT_AMB_DEBRIS_4X));
}
internal void __thiscall hook_draw_record_fill(void *c, ee_unit *u, float *rec, void *p3, void *p4) {
    collect_fill(c, u, rec, p3, p4);   // the game's fill, or collect.c's port of it
    if (u) walls_after_fill(unit_get_type_data(u), unit_get_physics(u) != NULL, unit_get_stats(u), rec);
}
internal ee_unit *__cdecl hook_place_building(ee_player *pl, int type, int x, int y) {
    ee_unit *b = place_foundation(pl, type, x, y);
    ee_map *map = game_map();
    if (b && g_bl_on && map && bl_ground(b, unit_get_type_data(b)) && g_log)
        fprintf(g_log, "# walls: building_lean: type %d at %d,%d put onto the ground (its facing's rise %.2f, its side's %.2f)\n",
                type, unit_get_x(b), unit_get_y(b), unit_get_matrix(b, 9), unit_get_matrix(b, 8));
    ee_unit_type *td = b && g_wj_on && ut_role(type, UTR_TOWER) ? type_data(type) : NULL;
    if (td) {
        int s = unit_type_get_size(td) > 0 ? unit_type_get_size(td) : 1;
        wj_note(unit_get_x(b) - s, unit_get_y(b) - s, unit_get_x(b) + s, unit_get_y(b) + s);
    }
    return b;
}

// at a step's end (the same step on every PC): the walls a game starts with (a loaded game, a scenario's) at its
// first; the areas noted at an earlier step (their new pieces on the map now)
internal void __thiscall hook_step_end(ee_physics *ph) {
    physics_step_end(ph);
    if (!game_map()) return;
    if (game_time()) { g_ws_ed_seen = 0; g_ws_ed_base = 0; }   // a game runs (a test play): not the editor - its time stays 0
    if (!g_ws_fitted_game) {
        g_ws_fitted_game = 1;
        if (g_bl_on) bl_ground_all("the game's start");
        if (g_ws_on && g_ws_start) wj_area(-1, -1, -1, -1, "the game's start");
    }
    if (!g_ws_on || !g_wj_ev_n) return;
    uint32_t now = game_time();
    int k = 0;
    for (int i = 0; i < g_wj_ev_n; i++) {
        if (g_wj_ev[i].t == now) { g_wj_ev[k++] = g_wj_ev[i]; continue; }
        wj_area(g_wj_ev[i].x0, g_wj_ev[i].y0, g_wj_ev[i].x1, g_wj_ev[i].y1, "joins");
    }
    g_wj_ev_n = k;
}

// THE SCENARIO EDITOR's time stays 0 (its steps run, its clock doesn't): what its actions note would wait for good,
// and the walls of a scenario opened there were never fitted. Its actions (placing walls, gates, units, deleting,
// moving, painting the ground, opening a map) bring their objects onto the map only after they return, and some by
// no action the mod sees (a scenario opened by the file dialog) - passes for 0.6 s after a place action missed now
// and then, and nothing else was looked at (the player, 2026-11-03: "the update in editor is still a bit hit or miss,
// needing often some adding/removing to force a wall update"). So it is watched: while its form is drawn
// (EEUIFScenarioEditor::draw_background, every frame it is on screen; WS_ED_GONE without: left) the safe point between
// the loop's passes (test/control.c hook_sim_receive) looks every WS_ED_EVERY at the walls, towers and gates on the
// map - each one's place, kind, owner, whether it is on its tile's list, the ground at its tile's corners - and where
// that changed (one appeared, went, moved, the ground under it was painted) judges and fits them: every wall
// (wall_start_fit, also at the first look), else the tiles around each change (a new map - nothing in common with
// the look before - is only looked at). A pass changes none of that (anims and matrices), so it doesn't call itself
// again; the editor has no other PC to keep in step.
#define WS_ED_EVERY 100     // ms between looks
#define WS_ED_GONE 1000     // ms without the editor's frame: it was left
#define WS_ED_MAX 16384     // walls, towers and gates looked at
struct ws_ed { ee_unit *u; uint32_t h; int x, y; };
internal struct ws_ed g_ws_ed[2][WS_ED_MAX];
internal int g_ws_ed_n[2], g_ws_ed_cur;   // the last look's (by unit)

internal int __thiscall hook_editor_frame(ee_editor_form *f, void *rect) {
    g_ws_ed_seen = (LONG)(GetTickCount() | 1);
    // The minimap's top rows hang over the 3D view, which is drawn every frame, and a control is drawn only when it is
    // dirty: they were there for the frame after a change of its picture and gone again (the player, 2026-11-09: "the
    // tope of the minimap that hangs over the 3d view in the editor flickers"). Dirty every frame, they stay.
    eeui_mini_map *mm = editor_form_get_minimap(f);
    if (mm) ui_control_set_dirty((ee_ui_control *)mm, 1);
    return editor_form_draw_background(f, rect);
}

internal int ws_ed_cmp(const void *a, const void *b) {
    uintptr_t x = (uintptr_t)((const struct ws_ed *)a)->u, y = (uintptr_t)((const struct ws_ed *)b)->u;
    return x < y ? -1 : x > y;
}

// the walls, towers and gates on the map into e (by unit), their number
internal int ws_ed_look(const ee_map *map, struct ws_ed *e) {
    int n = 0, size = (int)map_get_size(map);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it) && n < WS_ED_MAX; ) {
        int t = unit_type_get_id(it.td), x = unit_get_x(it.u), y = unit_get_y(it.u);
        if (!is_wall(t) && !ut_role(t, UTR_TOWER)) continue;
        uint32_t h = (uint32_t)t * 2654435761u ^ (uint32_t)(uintptr_t)unit_get_owner(it.u) ^ (uint32_t)wj_alive(it.u) << 31;
        ee_tile *tile = x >= 0 && y >= 0 && x < size && y < size ? map_get_tiles(map)[y * size + x] : NULL;
        for (void *node = tile ? tile_get_objects(tile) : NULL; node; node = tilenode_get_next(node))
            if (tilenode_get_object(node) == it.u) { h ^= 1u << 30; break; }
        for (int c = 0; c < 4; c++) {
            float z = ground_corner(map, x + (c & 1), y + (c >> 1));
            uint32_t zb;
            memcpy(&zb, &z, 4);
            h = h * 31 + zb;
        }
        e[n++] = (struct ws_ed){ it.u, h, x, y };
    }
    qsort(e, (size_t)n, sizeof *e, ws_ed_cmp);
    return n;
}

internal void walls_safe_point(void) {
    static DWORD last;
    LONG seen = g_ws_ed_seen;
    ee_map *map = game_map();
    if (!seen || !map) return;
    DWORD now = GetTickCount();
    if ((LONG)(now - (DWORD)seen) > WS_ED_GONE) { g_ws_ed_seen = 0; g_ws_ed_base = 0; return; }   // seen (| 1) may be 1 ahead
    if (now - last < WS_ED_EVERY) return;
    last = now;
    g_ws_fitted_game = 1;
    if (g_bl_on) bl_ground_all("the editor's");
    if (!g_ws_on) return;
    g_wj_ev_n = 0;   // the layouts' notes (time 0): the look below finds their pieces
    const struct ws_ed *o = g_ws_ed[g_ws_ed_cur];
    int on = g_ws_ed_n[g_ws_ed_cur], cur = g_ws_ed_cur ^ 1;
    struct ws_ed *e = g_ws_ed[cur];
    int n = ws_ed_look(map, e), base = !g_ws_ed_base;
    g_ws_ed_cur = cur; g_ws_ed_n[cur] = n; g_ws_ed_base = 1;
    if (base) {
        if (g_log) fprintf(g_log, "# walls: the editor watched (%d walls, towers and gates)\n", n);
        if (g_ws_start) wj_area(-1, -1, -1, -1, "the editor's");
        return;
    }
    int i = 0, j = 0, common = 0;
    while (i < on || j < n) {
        uintptr_t a = i < on ? (uintptr_t)o[i].u : UINTPTR_MAX, b = j < n ? (uintptr_t)e[j].u : UINTPTR_MAX;
        if (a < b) { wj_add(o[i].x, o[i].y, o[i].x, o[i].y); i++; continue; }        // gone
        if (b < a) { wj_add(e[j].x, e[j].y, e[j].x, e[j].y); j++; continue; }        // new
        common++;
        if (o[i].h != e[j].h || o[i].x != e[j].x || o[i].y != e[j].y) {
            wj_add(o[i].x, o[i].y, o[i].x, o[i].y);
            wj_add(e[j].x, e[j].y, e[j].x, e[j].y);
        }
        i++; j++;
    }
    if (!g_wj_ev_n) return;
    if (g_ws_start) wj_area(-1, -1, -1, -1, "the editor's");
    else if (common || !on)
        for (int k = 0; k < g_wj_ev_n; k++) wj_area(g_wj_ev[k].x0, g_wj_ev[k].y0, g_wj_ev[k].x1, g_wj_ev[k].y1, "the editor's joins");
    g_wj_ev_n = 0;
}

internal void walls_new_game(void) {
    g_ws_fitted_game = 0; g_ws_gfx_n = 0; g_ws_laid_n = 0; g_wj_ev_n = 0;
    g_ws_pieces = g_ws_posts = g_ws_unsloped = g_ws_skipped = g_ws_no_model = 0;
    g_wj_to_piece = g_wj_to_post = g_wj_gates = 0;
}

internal void walls_install(void) {
    g_ws_on = g_set.wall_slopes;
    g_ws_start = g_set.wall_start_fit;
    g_wj_on = g_ws_on && g_set.wall_joins;
    int of = 0, n = g_ws_on ? patch_wall_piece(hook_wall_piece, &of) + patch_wall_joints(hook_wall_joints, &of) : 0;
    g_bl_on = g_set.building_lean;
    int any = g_ws_on || g_bl_on;
    int more = (any ? patch_step_end(hook_step_end) : 0) + (g_ws_on ? patch_gate_convert(hook_gate_convert) : 0);
    int upg = g_ws_on ? patch_upgrade_convert(hook_upgrade_convert) : 0;
    if (g_ws_on) g_unit_converted = walls_converted;
    if (g_ws_on && !upg && g_log) fprintf(g_log, "# walls: the upgrade's conversion NOT hooked\n");
    if (g_ws_on && !patch_wall_ghosts(hook_wall_ghosts) && g_log) fprintf(g_log, "# walls: the placement preview NOT hooked\n");
    int pof = 0, pn = g_wj_on || g_bl_on ? patch_place_building(hook_place_building, &pof) : 0;
    int ed = patch_editor_frame(hook_editor_frame);   // whatever the walls say: the hills' shadows read it too (render_shadows.c)
    int df = g_bl_on ? patch_draw_record_fill(hook_draw_record_fill) : 0;
    if ((g_ws_on || g_bl_on) && (!on_module_load("DX7HRDisplay.dll", ws_display_loaded0) || !on_module_load("DX7HRTnLDisplay.dll", ws_display_loaded1)) && g_log)
        fprintf(g_log, "# walls: skirts - the display DLLs' loads not watchable\n");
    if (g_log) fprintf(g_log, "# walls: wall_slopes=%d wall_joins=%d wall_start_fit=%d building_lean=%d (%s%s%s%s%s)\n", g_ws_on, g_wj_on, g_ws_start, g_bl_on,
                       !g_ws_on ? "-" : n == of ? "the layout's calls hooked" : n ? "SOME of the layout's calls hooked" : "FAILED",
                       !any ? "" : more == 1 + g_ws_on ? ", the step's end, the gate's" : ", the step's end or the gate's FAILED",
                       !g_wj_on && !g_bl_on ? "" : pn == pof ? ", the buildings' placing" : ", the buildings' placing FAILED",
                       !any ? "" : ed ? ", the editor's" : ", the editor's FAILED",
                       !g_bl_on ? "" : df ? ", the objects' drawing" : ", the objects' drawing FAILED");
}
