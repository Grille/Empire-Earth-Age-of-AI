// THE UNITS DRAWN AS INSTANCES ([display] unit_instancing; the pipe: picture inst on | off | (nothing: its counts)). The
// animation's vertices are on the GPU already (render_morph.c), but a model still went through the display DLL a part at a
// time (DrawModelHierarchy): the world matrix, the material, a lock of the vertex buffer, a copy noted and not made, the
// draw - 1.8 ms a frame for 1300 objects, of which the game's own work on a record was next to nothing (the player,
// 2026-11-09: "are the soilders meshes properly retained and instanced etc?"). For a unit's record that function is
// replaced: the same walk of the hierarchy, the same test of what is seen and choice of the detail by the engine's own
// functions, and each material goes straight into the batch. A model's triangles go once into a pool on the GPU, and the
// draws of one list in a batch become one DrawIndexedInstanced: its index says "my entry is the instance's" (the shader's
// morph) and the instance stream counts up from where the batch's entries lie, so they are uploaded list by list.
// What the replacement can't do is left to the DLL as a whole model: anything but a unit's record, a model bent for this
// draw, vertices with colours of their own, a transformation pushed by someone else, the probe.
#define IN_INDICES (1u << 23)    // the pool: 32 MB; full, it starts over (the list drawn first)
#define IN_SLOTS 16384           // the lists known, open addressing; three quarters full it starts over too
#define IN_LIST_MAX (1u << 18)   // a list's indices
#define IN_IDS (1u << 19)        // the instance stream: as far as an entry's number goes (the index's 19 bits)
#define IN_GROUPS 1024           // lists in one batch
#define IN_HASH 4096
#define IN_THIS_ENTRY 0xFFFFF000u   // the index's top bits: RD3_MORPH_HLSL reads the entry from the instance
typedef long (__thiscall *in_model_fn)(void *ras, ee_model *m, unsigned long screen, float bias, uint8_t ortho);
typedef struct { const WORD *src; uint32_t n, at; WORD first[3], last[3]; } in_list;
typedef struct { const WORD *src; uint32_t n, at; int head, tail, count; } in_group;
internal struct {
    int ready;
    uint32_t used;
    int n;
    in_list list[IN_SLOTS];
    uint32_t tmp[IN_LIST_MAX];
    // the batch's lists, each's entries chained
    int ng;
    in_group g[IN_GROUPS];
    uint32_t stamp;
    uint32_t hstamp[IN_HASH];
    uint16_t hg[IN_HASH];
    int16_t next[RB_WORLDS];
    rb_world sorted[RB_WORLDS];
    // the display's and the engine's
    in_model_fn orig;
    void *(__thiscall *settex)(void *ras, void *tex, unsigned long screen);
    float (__thiscall *pixels)(void *ras, void *model, void *viewport, void *at, float scale, uint8_t ortho);
    void (__thiscall *apply)(const void *parent, const void *local, void *out);
    const uint32_t *stack;   // sTransformStack (a vector: its first +4, its last +8)
} g_in;
internal int in_on(void) { return g_in_force < 0 ? g_set.unit_instancing : g_in_force; }
internal int in_init(void) {
    if (g_in.ready) return g_in.ready > 0;
    g_in.ready = -1;
    if (!mo_init()) return 0;
    D3D11_BUFFER_DESC bd = { IN_INDICES * 4, D3D11_USAGE_DEFAULT, D3D11_BIND_INDEX_BUFFER, 0, 0, 0 };
    HRESULT hr = ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_in_ib);
    uint32_t *ids = malloc(IN_IDS * 4);
    if (!ids) hr = E_OUTOFMEMORY;
    if (SUCCEEDED(hr)) {
        for (uint32_t k = 0; k < IN_IDS; k++) ids[k] = k;
        D3D11_BUFFER_DESC vd = { IN_IDS * 4, D3D11_USAGE_IMMUTABLE, D3D11_BIND_VERTEX_BUFFER, 0, 0, 0 };
        D3D11_SUBRESOURCE_DATA sd = { ids, 0, 0 };
        hr = ID3D11Device_CreateBuffer(g_d3d.dev, &vd, &sd, &g_in_ids);
    }
    free(ids);
    if (FAILED(hr)) return rd_fail("the units' instances", hr), 0;
    g_in.stamp = 1;   // (the table's slots start at 0: free)
    g_in.ready = 1;
    return 1;
}
// the list's place in the pool, put there if new (or its memory holds another now); NULL when it can't be
internal const in_list *in_list_of(const WORD *idx, uint32_t ni) {
    uint32_t h = (uint32_t)(((uintptr_t)idx >> 1) * 2654435761u ^ ni * 40503u) & (IN_SLOTS - 1);
    in_list *r;
    for (;; h = (h + 1) & (IN_SLOTS - 1)) {
        r = &g_in.list[h];
        if (!r->src) break;
        if (r->src == idx && r->n == ni) {
            if (!memcmp(r->first, idx, 6) && !memcmp(r->last, idx + ni - 3, 6)) return r;
            break;
        }
    }
    if (ni > IN_LIST_MAX || ni > IN_INDICES) return NULL;
    if (g_in.used + ni > IN_INDICES || (!r->src && g_in.n >= IN_SLOTS * 3 / 4)) {
        rd3_submit();   // the list draws from the pool as it is
        memset(g_in.list, 0, sizeof g_in.list);
        g_in.used = 0;
        g_in.n = 0;
        return in_list_of(idx, ni);
    }
    if (!r->src) g_in.n++;
    r->src = idx;
    r->n = ni;
    r->at = g_in.used;
    memcpy(r->first, idx, 6);
    memcpy(r->last, idx + ni - 3, 6);
    for (uint32_t k = 0; k < ni; k++) g_in.tmp[k] = IN_THIS_ENTRY | (idx[k] & 0xFFF);
    D3D11_BOX box = { r->at * 4, 0, 0, (r->at + ni) * 4, 1, 1 };
    ID3D11DeviceContext_UpdateSubresource(g_d3d.ctx, (ID3D11Resource *)g_in_ib, 0, &box, g_in.tmp, 0, 0);
    g_in.used += ni;
    return r;
}
internal in_group *in_group_of(const in_list *l) {
    uint32_t h = (uint32_t)(((uintptr_t)l->src >> 1) * 2654435761u ^ l->n * 40503u) & (IN_HASH - 1);
    for (;; h = (h + 1) & (IN_HASH - 1)) {
        if (g_in.hstamp[h] != g_in.stamp) {
            in_group *g = &g_in.g[g_in.ng];
            g->src = l->src; g->n = l->n; g->at = l->at; g->head = g->tail = -1; g->count = 0;
            g_in.hstamp[h] = g_in.stamp;
            g_in.hg[h] = (uint16_t)g_in.ng++;
            return g;
        }
        in_group *g = &g_in.g[g_in.hg[h]];
        if (g->src == l->src && g->n == l->n) return g;
    }
}
// a part into the batch (rd3_prepare bound its states): its entry - the matrix, the material, the runs - to its list
internal void in_append(rd_device *d, const mo_run *a, const mo_run *b, float f, const WORD *idx, DWORD ni) {
    const in_list *l = in_list_of(idx, ni);   // (may draw the list so far)
    if (!l) return;
    sil_batch();
    if (g_rb.ni && (g_rb.morph != 2 || g_rb.nw >= RB_WORLDS || g_in.ng >= IN_GROUPS)) rd3_flush();
    in_group *g = in_group_of(l);
    int s = g_rb.nw++;
    rb_world *e = &g_rb.w[s];
    rd3_entry(d, e);
    e->mo[0] = (float)a->at; e->mo[1] = (float)b->at; e->mo[2] = f;
    g_in.next[s] = -1;
    if (g->tail >= 0) g_in.next[g->tail] = (int16_t)s; else g->head = s;
    g->tail = s;
    g->count++;
    g_rb.morph = 2;
    g_rb.topo = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    g_rb.ni += (int)ni;   // (only that the batch isn't empty)
    g_mo.draws++;
}
// the batch into the list (rd3_flush): its entries list by list, a draw of each list's
internal void in_flush(void) {
    uint32_t at = 0;
    for (int i = 0; i < g_in.ng; i++)
        for (int e = g_in.g[i].head; e >= 0; e = g_in.next[e]) g_in.sorted[at++] = g_rb.w[e];
    long wa = rd3_upload(&g_rd3.wb, &g_rd3.wb_cap, D3D11_BIND_SHADER_RESOURCE, &g_rd3.wb_pos, g_in.sorted, at * sizeof *g_in.sorted, sizeof *g_in.sorted);
    if (wa >= 0) {
        uint32_t first = (uint32_t)wa / sizeof *g_in.sorted;
        for (int i = 0; i < g_in.ng; i++) {
            rf_record_inst(g_in.g[i].at, g_in.g[i].n, (UINT)g_in.g[i].count, first);
            first += (uint32_t)g_in.g[i].count;
        }
    }
    g_rb.nv = g_rb.ni = g_rb.nw = 0;
    g_in.ng = 0;
    g_in.stamp++;
}

typedef struct { void *ras, *view; rd_device *d; unsigned long screen; float bias; } in_ctx;
// a material of a model seen: what the DLL does for it, without the device's calls
internal void in_part(const in_ctx *c, ee_model *m, ee_material *mat, const D3DMATRIX *world, const WORD *idx, DWORD ni) {
    uint32_t n = material_get_vertex_count(mat), first = material_get_vertex_first(mat);
    ee_vertices **fr = model_get_vertices(m);
    float frac = model_get_fraction(m);
    const float *pa = (const float *)vertices_get_data(fr[model_get_frame(m)]) + first * 8;
    const float *pb = frac != 0 ? (const float *)vertices_get_data(fr[model_get_next_frame(m)]) + first * 8 : pa;
    rd_device *d = c->d;
    g_in.settex(c->ras, material_get_textures(mat)[c->screen], c->screen);
    vec3 col = material_get_colour(mat);
    D3DMATERIAL7 mt;
    memset(&mt, 0, sizeof mt);
    mt.diffuse.r = mt.ambient.r = col.x; mt.diffuse.g = mt.ambient.g = col.y; mt.diffuse.b = mt.ambient.b = col.z;
    mt.diffuse.a = model_get_alpha(m) * material_get_alpha(mat);
    mt.ambient.a = 1;
    InterlockedIncrement(&g_rd_draws);
    LONGLONG t0 = rd_qpc();
    RD_LOCK();
    d->material = mt;
    d->world = *world;
    mo_run *a = mo_run_of(pa, n), *b = pb == pa ? a : mo_run_of(pb, n);
    if (a && b != a) a = mo_run_of(pa, n);   // the second's put in may have started the pool over
    if (a && b) {
        float box[6];
        for (int k = 0; k < 3; k++) { box[k] = fminf(a->lo[k], b->lo[k]); box[3 + k] = fmaxf(a->hi[k], b->hi[k]); }
        rd_vb none = { NULL, 0, 0, 0x152, n, 36, n, NULL };   // (the box stands for its vertices)
        g_mo_box = box;
        rd3_shadow_classify(d, &none, 0, n);
        g_mo_box = NULL;
        if (d->sh_role != SH_BLOB && rd3_prepare(d, 0)) in_append(d, a, b, frac, idx, ni);
        if (d->sh_card && n <= SH_CARD_MAX) {   // a tree's card is made of its vertices
            static uint8_t v[SH_CARD_MAX * 36];
            (frac != 0 ? g_mo_smooth : g_mo_copy)(c->ras, m, mat, v);
            rd_vb vb = { NULL, 0, 0, 0x152, n, 36, n, v };
            rd3_shadow_card(d, D3DPT_TRIANGLELIST, &vb, 0, n, idx, ni);
        }
        InterlockedIncrement(&g_in_parts);
    }
    g_rd_draw_qpc += rd_qpc() - t0;
    RD_UNLOCK();
}
// a model and what hangs on it (parent: its parent's transformation, pscale its size)
internal void in_node(const in_ctx *c, ee_model *m, const float *parent, float pscale) {
    float t[16], s = model_get_scale(m);
    if (parent) { s *= pscale; g_in.apply(parent, model_transform_at(m), t); }
    else memcpy(t, model_transform_at(m), sizeof t);
    uint8_t seen = 0, clip = 0;
    shr_hook_visible(c->ras, m, s, c->view, t, &seen, &clip);
    if (seen) {
        uint32_t lod = 0, lods = model_get_lods(m);
        if (!parent) {
            float w = screen_state_get_view_right(c->view) - screen_state_get_view_left(c->view);
            long double f = (long double)g_in.pixels(c->ras, m, c->view, t, s, 0);
            f = ((long double)800 / (long double)w * f) / (long double)c->bias;
            if (f > 1) f = 1;
            float k = (float)((1 - f) * (long double)(int)lods);
            lod = (uint32_t)lrintf(k);
            if (lod >= lods - 1) lod = lods - 1;
        }
        D3DMATRIX world = { ._11 = t[0], ._12 = t[4], ._13 = t[8], ._14 = t[12], ._21 = t[1], ._22 = t[5], ._23 = t[9], ._24 = t[13],
                            ._31 = t[2], ._32 = t[6], ._33 = t[10], ._34 = t[14], ._41 = t[3], ._42 = t[7], ._43 = t[11], ._44 = t[15] };
        const WORD *indices = polygons_get_indices(model_get_polygons(m)[lod]);
        ee_material **mats = model_get_materials(m);
        for (uint32_t i = 0, nm = model_get_material_count(m); i < nm; i++) {
            ee_material *mat = mats[i];
            uint32_t tris = material_get_polygon_count(mat)[lod];
            if (tris) in_part(c, m, mat, &world, indices + material_get_polygon_first(mat)[lod] * 3, tris * 3);
        }
    }
    ee_model **ch = model_get_children(m);
    for (uint32_t i = 0, nc = model_get_child_count(m); i < nc; i++) in_node(c, ch[i], t, s);
}
// the whole hierarchy can be drawn this way
internal int in_model_ok(ee_model *m) {
    ee_vertices **fr = model_get_vertices(m);
    if (!fr || !fr[model_get_frame(m)] || vertices_get_type(fr[0])) return 0;
    if (model_get_fraction(m) != 0 && !fr[model_get_next_frame(m)]) return 0;
    ee_material **mats = model_get_materials(m);
    for (uint32_t i = 0, nm = model_get_material_count(m); i < nm; i++) {
        uint32_t n = material_get_vertex_count(mats[i]);
        if (!n || n > MO_MAX_VERTS || material_get_polygon_count(mats[i])[0] * 3 > IN_LIST_MAX) return 0;
    }
    ee_model **ch = model_get_children(m);
    for (uint32_t i = 0, nc = model_get_child_count(m); i < nc; i++) if (!in_model_ok(ch[i])) return 0;
    return 1;
}
internal long __thiscall in_hook(void *ras, ee_model *m, unsigned long screen, float bias, uint8_t ortho) {
    const uint32_t *st = g_in.stack;
    rd_device *d = NULL;
    if (in_on() && g_sl.in && !ortho && g_ro_cur == RO_UNITS && !g_mo_off && g_mo.on && g_rp_state != 2 && g_mo_smooth && g_shr_visible &&
        !(st[1] && (st[2] - st[1]) & ~7u) && in_model_ok(m) && in_init())
        d = draw_state_get_device(rasterizer_get_dstates(ras)[screen]);
    if (!d || !d->rs[D3DRENDERSTATE_ZENABLE]) { InterlockedIncrement(&g_in_fell); return g_in.orig(ras, m, screen, bias, ortho); }
    in_ctx c = { ras, rasterizer_get_screen_state(ras, screen), d, screen, bias };
    in_node(&c, m, NULL, 0);
    return 0;
}
// the display DLL loaded (render.c render_display_loaded): the model draw hooked, 1; the T&L DLL's only (render_morph.c)
internal int in_display_loaded(HMODULE m, const char *name) {
    if (!g_set.unit_instancing || _stricmp(name, "DX7HRTnLDisplay.dll") || g_in.orig) return 0;
    static const uint8_t ee[9] = { 0x55, 0x8B, 0xEC, 0x81, 0xEC, 0xF4, 0x00, 0x00, 0x00 };    // push ebp; mov ebp, esp; sub esp, 0xF4
    static const uint8_t aoc[9] = { 0x55, 0x8B, 0xEC, 0x81, 0xEC, 0xF0, 0x00, 0x00, 0x00 };   // the same, 0xF0
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
    uintptr_t f = (uintptr_t)GetProcAddress(m, "?DrawModelHierarchy@DX7Rasterizer@@MAEJPAVGEModel@@KM_N@Z");
    *(void **)&g_in.settex = (void *)GetProcAddress(m, "?SetTexture@DX7Rasterizer@@UAEXPAVGETexture@@K@Z");
    if (!lle || !f || !g_in.settex) return 0;
    *(void **)&g_in.pixels = (void *)GetProcAddress(lle, "?GetPixelSize@GERasterizer@@QAEMPAVGEModel@@PAVGEViewport@@AAVGETransformation@@M_N@Z");
    *(void **)&g_in.apply = (void *)GetProcAddress(lle, "?Apply@GETransformation@@QBEXABV1@AAV1@@Z");
    g_in.stack = (const uint32_t *)(void *)GetProcAddress(lle, "?sTransformStack@GERasterizer@@1V?$vector@UTransformStack@GERasterizer@@V?$allocator@UTransformStack@GERasterizer@@@std@@@std@@A");
    if (!g_in.pixels || !g_in.apply || !g_in.stack) return 0;
    void *o = NULL;
    if (!patch_entry(f, build_is("aoc") ? aoc : ee, 9, (void *)in_hook, &o)) return 0;
    g_in.orig = (in_model_fn)o;
    return 1;
}
