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
#define WS_INTO_POST 0.3f   // a piece's end into a post beside it (the posts' sides 0.49 .. 0.66 from their middle)

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
internal ee_model *ws_model(ee_unit *u, int anim) {
    ee_unit_stats *st = unit_get_stats(u);
    ee_dmg_model *gfx = st ? unit_stats_get_gfx(st) : NULL;
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
// point, WS_SKIRT_EDGE) goes on downwards by a quad WS_SKIRT deep, its two lower corners copies of the edge's
// (normal, texture coordinates: the bottom row of the texture drawn down), in the triangle's material and winding
// - into the ground where it is level, filling the gap where it falls away. A model is the engine's (GEModel, one
// for every piece of that kind and era): each material a run of its vertices and of each level of detail's
// triangles, indices counted from the run's first (spec GEMaterial) - its arrays made anew larger by the display
// DLL (DX7Rasterizer.create_vertex_array, create_polygon_array), each material's run followed by its skirt. The
// renderer copies a material's vertices into its vertex buffer every time it draws it (CopyStdVertices), so the new
// arrays are drawn from the next frame on. The graphics thread draws the models while the simulation runs: the fit
// (simulation thread) asks for the skirt (ws_skirt_want), the model's draw (DrawModelHierarchy, hooked in both
// display DLLs) makes it before drawing it, on the one thread that reads a model's arrays. The old arrays stay (other
// code may hold them; a few KB a model). Only the picture: the pieces' places and the game state are the fit's.
#define WS_SKIRT 1.0f            // how far below its bottom a model reaches
#define WS_SKIRT_EDGE 0.02f      // a vertex this close above the mesh's lowest point is on its bottom
#define WS_SKIRT_MODELS 64
struct ws_skirt { ee_model *m; ee_vertices *va; volatile LONG state; };   // state 0 asked, 1 made, 2 can't be
internal struct ws_skirt g_ws_sk[WS_SKIRT_MODELS];
internal volatile LONG g_ws_sk_n, g_ws_sk_asked;
internal rasterizer_draw_model_hierarchy_fn g_ws_draw[2];
internal int g_ws_draw_hooks;

// the fit's model: its skirt made at its next draw (the simulation thread; the draw reads g_ws_sk[0 .. g_ws_sk_n))
internal void ws_skirt_want(ee_model *m) {
    if (!m || !g_ws_draw_hooks) return;
    LONG n = g_ws_sk_n;
    for (LONG i = 0; i < n; i++) {
        struct ws_skirt *s = &g_ws_sk[i];
        if (s->m != m) continue;
        // a model loaded anew where one was (the graphics freed and loaded again): not the arrays it was given
        if (s->state && model_get_frames(m) && model_get_vertices(m) && model_get_vertices(m)[0] != s->va) {
            s->state = 0;
            InterlockedIncrement(&g_ws_sk_asked);
        }
        return;
    }
    if (n >= WS_SKIRT_MODELS) return;
    g_ws_sk[n] = (struct ws_skirt){ m, NULL, 0 };
    InterlockedIncrement(&g_ws_sk_asked);
    InterlockedExchange(&g_ws_sk_n, n + 1);
}

internal int ws_cmp_first(const void *a, const void *b) {
    uint32_t x = material_get_vertex_first(*(ee_material *const *)a), y = material_get_vertex_first(*(ee_material *const *)b);
    return x < y ? -1 : x > y;
}

// the skirt of m (the graphics thread, before its draw): 1 made, 0 none (no bottom edge), -1 a model it can't take
// (another vertex kind, materials not in runs one after another, a material beyond the vertex buffer) - left as it is
internal int ws_skirt_make(ee_rasterizer *ras, ee_model *m, unsigned *edges_out) {
    uint32_t nf = model_get_frames(m), nl = model_get_lods(m), nm = model_get_material_count(m), nv = model_get_vertex_count(m);
    ee_vertices **va = model_get_vertices(m);
    ee_polygons **pa = model_get_polygons(m);
    ee_material **mat = model_get_materials(m);
    if (!nf || !nl || !nm || !nv || nf > 256 || nl > 16 || nm > 256 || !va || !pa || !mat) return -1;
    for (uint32_t f = 0; f < nf; f++)
        if (!va[f] || vertices_get_type(va[f]) != 0 || vertices_get_count(va[f]) < nv) return -1;
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
    ee_vertex *v0 = vertices_get_data(va[0]);
    float zmin = 1e9f;
    for (uint32_t i = 0; i < nv; i++) { float z = vertex_point_at(vertex_at(v0, i))->z; if (z < zmin) zmin = z; }
    // per material: its bottom vertices that get a copy below (by index in its run: the copy's index, 0 none), and per
    // lod the skirt's triangles; the edges counted at lod 0
    uint32_t *copy = calloc(nv, sizeof *copy);
    uint16_t *tri = NULL;   // per lod and material: the skirt's triangles (3 indices each), appended
    uint32_t tri_n = 0, tri_cap = 0, add_v = 0, edges = 0;
    uint32_t *tri_at = calloc((size_t)nl * nm + 1, sizeof *tri_at);   // [l * nm + k]: its first in tri, then the end
    uint32_t *extra = calloc(nm, sizeof *extra);                       // [k]: its copies
    int ok = copy && tri_at && extra;
    for (uint32_t l = 0; ok && l < nl; l++)
        for (uint32_t k = 0; ok && k < nm; k++) {
            tri_at[l * nm + k] = tri_n;
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
                // the edge a -> b in the triangle's order; below it (b, a, a') and (b, a', b'), the same way round
                int j = low[0] && low[1] ? 0 : low[1] && low[2] ? 1 : 2;
                uint16_t a = c[j], b = c[(j + 1) % 3];
                for (int e = 0; e < 2; e++) {
                    uint16_t w = e ? b : a;
                    if (!copy[vf + w]) { copy[vf + w] = vc + ++extra[k]; add_v++; }
                }
                if (tri_n + 6 > tri_cap) {
                    tri_cap = tri_cap ? tri_cap * 2 : 192;
                    uint16_t *t2 = realloc(tri, tri_cap * sizeof *tri);
                    if (!t2) { ok = 0; break; }
                    tri = t2;
                }
                uint16_t a2 = (uint16_t)(copy[vf + a] - 1), b2 = (uint16_t)(copy[vf + b] - 1);
                uint16_t q[6] = { b, a, a2, b, a2, b2 };
                memcpy(tri + tri_n, q, sizeof q);
                tri_n += 6;
                if (!l) edges++;
            }
        }
    if (ok) tri_at[nl * nm] = tri_n;
    for (uint32_t k = 0; ok && k < nm; k++)
        if (material_get_vertex_count(ord[k]) + extra[k] > display_vb_vertices() || material_get_vertex_count(ord[k]) + extra[k] > 0xFFFF) ok = 0;
    int r = !ok ? -1 : !edges ? 0 : 1;
    if (r == 1) {
        // the new arrays: each frame's (one for the frames that shared one), each lod's
        uint32_t nv2 = nv + add_v;
        ee_vertices *nva[256] = { 0 };
        ee_polygons *npa[16] = { 0 };
        for (uint32_t f = 0; r == 1 && f < nf; f++) {
            uint32_t g = 0;
            while (g < f && va[g] != va[f]) g++;
            if (g < f) { nva[f] = nva[g]; continue; }
            if (!(nva[f] = rasterizer_create_vertex_array(ras, 0, nv2))) { r = -1; break; }
            ee_vertex *src = vertices_get_data(va[f]), *dst = vertices_get_data(nva[f]);
            uint32_t to = 0;
            for (uint32_t k = 0; k < nm; k++) {
                uint32_t vf = material_get_vertex_first(ord[k]), vc = material_get_vertex_count(ord[k]);
                memcpy(vertex_at(dst, to), vertex_at(src, vf), (size_t)vc * vertex_size());
                for (uint32_t i = 0; i < vc; i++)
                    if (copy[vf + i]) {
                        ee_vertex *d = vertex_at(dst, to + copy[vf + i] - 1);
                        memcpy(d, vertex_at(src, vf + i), vertex_size());
                        vertex_point_at(d)->z -= WS_SKIRT;
                    }
                to += vc + extra[k];
            }
        }
        for (uint32_t l = 0; r == 1 && l < nl; l++) {
            if (!pa[l]) continue;
            uint32_t n2 = polygons_get_count(pa[l]) + (tri_at[l * nm + nm] - tri_at[l * nm]) / 3;
            if (!(npa[l] = rasterizer_create_polygon_array(ras, n2))) { r = -1; break; }
            const uint16_t *src = polygons_get_indices(pa[l]);
            uint16_t *dst = polygons_get_indices(npa[l]);
            uint32_t to = 0;
            for (uint32_t k = 0; k < nm; k++) {
                uint32_t pf = material_get_polygon_first(ord[k])[l], pc = material_get_polygon_count(ord[k])[l];
                uint32_t s0 = tri_at[l * nm + k], s1 = tri_at[l * nm + k + 1];
                memcpy(dst + to * 3, src + pf * 3, (size_t)pc * 3 * sizeof *dst);
                memcpy(dst + to * 3 + pc * 3, tri + s0, (size_t)(s1 - s0) * sizeof *dst);
                to += pc + (s1 - s0) / 3;
            }
            // a lod's triangles beyond its materials' (none seen) stay at its end
            uint32_t own = 0;
            for (uint32_t k = 0; k < nm; k++) own += material_get_polygon_count(ord[k])[l];
            memcpy(dst + to * 3, src + own * 3, (size_t)(polygons_get_count(pa[l]) - own) * 3 * sizeof *dst);
        }
        if (r == 1) {   // the model onto them: the runs moved by the skirts before them
            for (uint32_t f = 0; f < nf; f++) va[f] = nva[f];
            for (uint32_t l = 0; l < nl; l++) if (npa[l]) pa[l] = npa[l];
            uint32_t vto = 0, pto[16] = { 0 };
            for (uint32_t k = 0; k < nm; k++) {
                material_set_vertex_first(ord[k], vto);
                material_set_vertex_count(ord[k], material_get_vertex_count(ord[k]) + extra[k]);
                vto += material_get_vertex_count(ord[k]);
                for (uint32_t l = 0; l < nl; l++) {
                    uint32_t *pf = material_get_polygon_first(ord[k]), *pc = material_get_polygon_count(ord[k]);
                    if (!pc[l]) continue;
                    pf[l] = pto[l];
                    pc[l] += (tri_at[l * nm + k + 1] - tri_at[l * nm + k]) / 3;
                    pto[l] += pc[l];
                }
            }
            model_set_vertex_count(m, nv2);
            if (npa[0]) model_set_polygon_count(m, polygons_get_count(npa[0]));
        }
    }
    free(copy); free(tri); free(tri_at); free(extra);
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
        int r = ws_skirt_make(ras, m, &edges);
        s->va = model_get_frames(m) && model_get_vertices(m) ? model_get_vertices(m)[0] : NULL;
        s->state = r >= 0 ? 1 : 2;
        InterlockedDecrement(&g_ws_sk_asked);
        log_locked("# walls: model %p %s (%u bottom edges, %u vertices, %u materials, %u lods)\n", (void *)m,
                   r > 0 ? "skirted" : r == 0 ? "has no bottom edge" : "CAN'T be skirted", edges,
                   model_get_vertex_count(m), model_get_material_count(m), model_get_lods(m));
        return;
    }
}
internal int __thiscall ws_hook_draw0(ee_rasterizer *ras, void *m, uint32_t screen, float p3, int8_t p4) {
    if (g_ws_sk_asked > 0) ws_skirt_drawn(ras, m);
    return g_ws_draw[0](ras, m, screen, p3, p4);
}
internal int __thiscall ws_hook_draw1(ee_rasterizer *ras, void *m, uint32_t screen, float p3, int8_t p4) {
    if (g_ws_sk_asked > 0) ws_skirt_drawn(ras, m);
    return g_ws_draw[1](ras, m, screen, p3, p4);
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
struct wj_at { int kind, sx, sy; ee_unit *u; };
internal struct wj_at wj_at(const ee_map *map, int x, int y, const ee_player *pl);
internal int g_wj_on;
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

// one piece onto the ground; 0 when it isn't a piece of the layout's or its ground is off the map
internal int ws_fit(ee_unit *u) {
    ee_map *map = game_map();
    if (!map || !u) return 0;
    int anim = unit_get_anim(u);
    if (anim == WS_SLOPE) { unit_start_anim(u, WS_STRAIGHT, -1, 0); g_ws_unsloped++; }
    else if (anim == WS_SLOPE_DIAG) { unit_start_anim(u, WS_DIAG, -1, 0); g_ws_unsloped++; }
    else if (anim != WS_STRAIGHT && anim != WS_DIAG && anim != WS_POST) return 0;
    // the middle of its tile, not the matrix's: a piece's matrix is moved below, a fit again gives the same
    int tx = unit_get_x(u), ty = unit_get_y(u);
    float px = (float)tx + 0.5f, py = (float)ty + 0.5f;
    ee_model *mdl = ws_model(u, anim);
    ws_skirt_want(mdl);
    if (anim == WS_POST) {
        float mid = ground_at(map, px, py);
        if (mid != mid) { g_ws_skipped++; return 0; }
        // its scale from its turn (x, y), so fitting it again gives the same; its skirt down to its lower corners
        float s = sqrtf(unit_get_matrix(u, 0) * unit_get_matrix(u, 0) + unit_get_matrix(u, 4) * unit_get_matrix(u, 4));
        unit_set_matrix(u, 8, 0);
        unit_set_matrix(u, 9, 0);
        unit_set_matrix(u, 10, s);
        unit_set_matrix(u, 11, mid);
        g_ws_posts++;
        return 1;
    }
    // its length: the local y axis, its turn (x, y; the local x axis keeps its scale, 1 for walls); its ends half a
    // tile out along it - an edge's middle (straight) or a corner (diagonal), where the ground's corners are exact
    float fx = unit_get_matrix(u, 1), fy = unit_get_matrix(u, 5), f = sqrtf(fx * fx + fy * fy);
    float len = sqrtf(unit_get_matrix(u, 0) * unit_get_matrix(u, 0) + unit_get_matrix(u, 4) * unit_get_matrix(u, 4));
    if (f < 0.01f || len < 0.01f) { g_ws_skipped++; return 0; }
    fx /= f; fy /= f;
    float d = 0.5f * (fabsf(fx) + fabsf(fy));
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
    else if (UT_IN(unit_type_get_id(unit_get_type_data(u)), UT_B_WALL_COPPER, UT_B_BARBED_WIRE)) {
        float mid = (ylo + yhi) * 0.5f;
        ylo = mid - d; yhi = mid + d;
    }
    int sx = fx > 0.3f ? 1 : fx < -0.3f ? -1 : 0, sy = fy > 0.3f ? 1 : fy < -0.3f ? -1 : 0;
    ee_player *pl = unit_get_owner(u);
    float ea = ws_post_at(map, tx - sx, ty - sy, pl) ? WS_INTO_POST : 0, eb = ws_post_at(map, tx + sx, ty + sy, pl) ? WS_INTO_POST : 0;
    float k = (2 * d + ea + eb) / (yhi - ylo), c = -d - ea - k * ylo, slope = (hb - ha) / (2 * d);   // its y -> k y + c along f
    const float m[12] = {
        fy * len,  fx * k,    0, px + fx * c,
        -fx * len, fy * k,    0, py + fy * c,
        0,         slope * k, len,      (ha + hb) * 0.5f + slope * c,
    };
    for (int i = 0; i < 12; i++) unit_set_matrix(u, i, m[i]);
    g_ws_pieces++;
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
#define WJ_EV_MAX 64        // areas noted, waiting for the next step's end
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
internal void wj_axis(const ee_unit *u, int *sx, int *sy) {
    float fx = unit_get_matrix(u, 1), fy = unit_get_matrix(u, 5), f = sqrtf(fx * fx + fy * fy);
    if (f > 0.01f) { fx /= f; fy /= f; }
    *sx = fx > 0.3f ? 1 : fx < -0.3f ? -1 : 0;
    *sy = fy > 0.3f ? 1 : fy < -0.3f ? -1 : 0;
}

// the tile's objects (a building on every tile it covers), pl's: its wall's piece or post, else its tower or gate
internal struct wj_at wj_at(const ee_map *map, int x, int y, const ee_player *pl) {
    struct wj_at a = { WJ_NONE, 0, 0, NULL };
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
        if (wj_piece_anim(anim)) { a.kind = WJ_PIECE; a.u = o; wj_axis(o, &a.sx, &a.sy); return a; }
    }
    return a;
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
        if (j < 4 && nb[j].kind == WJ_POST && nb[j + 4].kind == WJ_POST) { line = j; nl++; }
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

// u upright on its tile as the layout leaves a piece: facing fx, fy (a unit vector), on its tile's corner x, y
internal void wj_upright(const ee_map *map, ee_unit *u, float fx, float fy) {
    int tx = unit_get_x(u), ty = unit_get_y(u);
    float z = ground_corner(map, tx, ty);
    const float m[12] = { fy, fx, 0, (float)tx + 0.5f,  -fx, fy, 0, (float)ty + 0.5f,  0, 0, 1, z == z ? z : unit_get_matrix(u, 11) };
    for (int i = 0; i < 12; i++) unit_set_matrix(u, i, m[i]);
}

// u turned into a piece along sx, sy or a post (the fit after puts it onto the ground)
internal void wj_turn(const ee_map *map, ee_unit *u, struct wj_at w) {
    float l = w.sx && w.sy ? sqrtf(0.5f) : 1.0f;
    unit_start_anim(u, w.kind == WJ_POST ? WS_POST : w.sx && w.sy ? WS_DIAG : WS_STRAIGHT, -1, 0);
    if (w.kind == WJ_PIECE) wj_upright(map, u, (float)w.sx * l, (float)w.sy * l);
    else wj_upright(map, u, 0, 1);
    if (w.kind == WJ_POST) g_wj_to_post++; else g_wj_to_piece++;
}

// a fitted matrix (stretched, sheared, taller) put back upright - a piece about to become a gate's, a gate's unit of a
// game saved before gates were (the gate's middle three kept the fitted piece's: a diagonal one drawn at 0.63 of its
// length, sheared)
internal int wj_unfit(const ee_map *map, ee_unit *u) {
    float fx = unit_get_matrix(u, 1), fy = unit_get_matrix(u, 5), f = sqrtf(fx * fx + fy * fy);
    if (f < 0.01f || (fabsf(f - 1) < 0.001f && unit_get_matrix(u, 9) == 0 && fabsf(unit_get_matrix(u, 10) - 1) < 0.001f)) return 0;
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

// a tower placed (place_foundation's calls place_building: the player's, the AI's build manager's): its tiles noted - a
// wall's end post in line before it becomes a piece into it. A tower beside a straight wall leaves its pieces be: a
// piece turns a post only where another piece points at it from aside, and a tower points at nothing (the game's
// wall_joints counts towers among a piece's neighbours - a wall laid past a tower got a post beside it, a piece again
// at its joins: pointed at from either side)
internal ee_unit *__cdecl hook_place_building(ee_player *pl, int type, int x, int y) {
    ee_unit *b = place_foundation(pl, type, x, y);
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
    if (!g_ws_on || !game_map()) return;
    if (game_time()) { g_ws_ed_seen = 0; g_ws_ed_base = 0; }   // a game runs (a test play): not the editor - its time stays 0
    if (!g_ws_fitted_game) {
        g_ws_fitted_game = 1;
        if (g_ws_start) wj_area(-1, -1, -1, -1, "the game's start");
    }
    if (!g_wj_ev_n) return;
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
    if (!g_ws_on || !seen || !map) return;
    DWORD now = GetTickCount();
    if ((LONG)(now - (DWORD)seen) > WS_ED_GONE) { g_ws_ed_seen = 0; g_ws_ed_base = 0; return; }   // seen (| 1) may be 1 ahead
    if (now - last < WS_ED_EVERY) return;
    last = now;
    g_ws_fitted_game = 1;
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
    int more = g_ws_on ? patch_step_end(hook_step_end) + patch_gate_convert(hook_gate_convert) : 0;
    int pof = 0, pn = g_wj_on ? patch_place_building(hook_place_building, &pof) : 0;
    int ed = g_ws_on ? patch_editor_frame(hook_editor_frame) : 0;
    if (g_ws_on && (!on_module_load("DX7HRDisplay.dll", ws_display_loaded0) || !on_module_load("DX7HRTnLDisplay.dll", ws_display_loaded1)) && g_log)
        fprintf(g_log, "# walls: skirts - the display DLLs' loads not watchable\n");
    if (g_log) fprintf(g_log, "# walls: wall_slopes=%d wall_joins=%d wall_start_fit=%d (%s%s%s%s)\n", g_ws_on, g_wj_on, g_ws_start,
                       !g_ws_on ? "-" : n == of ? "the layout's calls hooked" : n ? "SOME of the layout's calls hooked" : "FAILED",
                       !g_ws_on ? "" : more == 2 ? ", the step's end, the gate's" : ", the step's end or the gate's FAILED",
                       !g_wj_on ? "" : pn == pof ? ", the buildings' placing" : ", the buildings' placing FAILED",
                       !g_ws_on ? "" : ed ? ", the editor's" : ", the editor's FAILED");
}
