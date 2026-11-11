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

// A TREE'S SHADOW CARD (render_shadows.c rd3_shadow_card made one from the vertices at every draw, and it took a batch of its
// own: a state change at each tree). It is the same for every tree of a model's frame - they all have the camera's turn, the
// card is about the camera and the sun, and each tree's size is only a factor (the matrix holds it: 1.65 to 1.88 among one
// kind) - so it is kept at size 1, made anew when those have moved. A card is linear in its vertices, so
// the blend of two frames' is the card of their blend: the two go into the animation's pool like any two frames, and are
// drawn at the tree's place as instances. Queued, so that a run of trees is one batch of trees and one of cards.
#define IN_CARDS 4096
typedef struct { const float *src; const WORD *idx; uint32_t n, ni; float r[9], axes[4]; float *v; float lo[2], hi[2]; } in_card;
typedef struct { in_card *a, *b; float frac, size, at[3]; const WORD *idx; DWORD ni; } in_card_q;
internal struct {
    int n, nq, flushing;
    float gen;              // a card remade is another run for the pool: its first vertex differs by this
    rd_device *d;
    rd_surface *tex;        // the queue's texture
    in_card card[IN_CARDS];
    in_card_q q[RB_WORLDS];
} g_ic;
internal int in_card_axes(const rd_device *d, float *axes) {
    float ax = d->view._11, ay = d->view._21, al = sqrtf(ax * ax + ay * ay), sx = g_sh.sun[1], sy = -g_sh.sun[0], sl = sqrtf(sx * sx + sy * sy);
    if (al < 1e-4f || sl < 1e-4f) return 0;
    axes[0] = ax / al; axes[1] = ay / al; axes[2] = sx / sl; axes[3] = sy / sl;
    if (axes[0] * axes[2] + axes[1] * axes[3] < 0) { axes[2] = -axes[2]; axes[3] = -axes[3]; }
    return 1;
}
// the card of a frame's run at size 1 under these axes and this turn (r: the world matrix's three rows, its size taken out); NULL: none
internal in_card *in_card_of(const float *src, uint32_t n, const WORD *idx, DWORD ni, const float *r, const float *axes) {
    uint32_t h = (uint32_t)(((uintptr_t)src >> 5) * 2654435761u ^ ((uintptr_t)idx >> 1) * 40503u) & (IN_CARDS - 1);
    in_card *c;
    for (;; h = (h + 1) & (IN_CARDS - 1)) {
        c = &g_ic.card[h];
        if (!c->src || (c->src == src && c->idx == idx && c->ni == ni && c->n == n)) break;
    }
    if (!c->src && g_ic.n >= IN_CARDS * 3 / 4) return NULL;
    int same = c->src != NULL;
    for (int i = 0; same && i < 9; i++) same = fabsf(c->r[i] - r[i]) < 1e-4f;   // (the camera's turn: it stands still; the sun creeps)
    for (int i = 0; same && i < 4; i++) same = fabsf(c->axes[i] - axes[i]) < 0.005f;
    if (same) return c;
    if (!c->src) {
        if (!(c->v = malloc((size_t)n * 32))) return NULL;
        c->src = src; c->idx = idx; c->n = n; c->ni = ni;
        g_ic.n++;
    }
    memcpy(c->r, r, sizeof c->r);
    memcpy(c->axes, axes, 16);
    for (uint32_t k = 0; k < n; k++) { g_card.up[k] = (int)k; g_card.n[k] = 0; g_card.cx[k] = g_card.cy[k] = 0; }
    for (DWORD t = 0; t + 2 < ni; t += 3) {
        if (idx[t] >= n || idx[t + 1] >= n || idx[t + 2] >= n) continue;
        int a = card_piece(idx[t]), b = card_piece(idx[t + 1]);
        g_card.up[b] = a;
        g_card.up[card_piece(idx[t + 2])] = a;
    }
    g_ic.gen += 1;
    g_in_cards++;
    for (uint32_t k = 0; k < n; k++) {
        const float *s = src + k * 8;
        float *p = c->v + k * 8;
        p[0] = s[0] * r[0] + s[1] * r[1] + s[2] * r[2];
        p[1] = s[0] * r[3] + s[1] * r[4] + s[2] * r[5];
        p[2] = s[0] * r[6] + s[1] * r[7] + s[2] * r[8];
        p[3] = g_ic.gen; p[4] = p[5] = 0; p[6] = s[6]; p[7] = s[7];
        int q = card_piece((int)k);
        g_card.cx[q] += p[0]; g_card.cy[q] += p[1]; g_card.n[q]++;
    }
    c->lo[0] = c->lo[1] = 1e30f; c->hi[0] = c->hi[1] = -1e30f;
    for (uint32_t k = 0; k < n; k++) {
        float *p = c->v + k * 8;
        int q = card_piece((int)k);
        float cx = g_card.cx[q] / (float)g_card.n[q], cy = g_card.cy[q] / (float)g_card.n[q];
        float l = (p[0] - cx) * axes[0] + (p[1] - cy) * axes[1];
        p[0] = cx + axes[2] * l; p[1] = cy + axes[3] * l;
        for (int a = 0; a < 2; a++) { if (p[a] < c->lo[a]) c->lo[a] = p[a]; if (p[a] > c->hi[a]) c->hi[a] = p[a]; }
    }
    return c;
}
// the queue drawn as one batch: the states of the trees it came with, the cast's role, no part in the silhouettes
internal void in_cards_flush(rd_device *d) {
    if (!g_ic.nq || g_ic.flushing) return;
    g_ic.flushing = 1;
    rd_surface *tex = d->tex[0];
    int role = d->sh_role, in = g_sl.in;
    D3DMATRIX keep = d->world;
    d->tex[0] = g_ic.tex; d->sh_role = SH_CARD; d->changed |= RD_CH_VC | RD_CH_TEX;
    g_sl.in = 0;
    if (rd3_prepare(d, 0))
        for (int i = 0; i < g_ic.nq; i++) {
            const in_card_q *e = &g_ic.q[i];
            mo_run *a = mo_run_of(e->a->v, e->a->n), *b = e->b == e->a ? a : mo_run_of(e->b->v, e->b->n);
            if (a && b != a) a = mo_run_of(e->a->v, e->a->n);
            if (!a || !b) continue;
            memset(&d->world, 0, sizeof d->world);
            d->world._11 = d->world._22 = d->world._33 = e->size;
            d->world._44 = 1;
            d->world._41 = e->at[0]; d->world._42 = e->at[1]; d->world._43 = e->at[2];
            in_append(d, a, b, e->frac, e->idx, e->ni);
        }
    g_sl.in = in;
    d->tex[0] = tex; d->sh_role = role; d->world = keep; d->changed |= RD_CH_VC | RD_CH_TEX;
    g_ic.nq = 0;
    g_ic.flushing = 0;
}
// the units' render object's end (RO_DRAW)
internal void in_cards_end(void) { if (g_ic.nq && g_ic.d) in_cards_flush(g_ic.d); }
internal void in_card_queue(rd_device *d, const float *pa, const float *pb, float frac, uint32_t n, const WORD *idx, DWORD ni) {
    float axes[4];
    if (!in_card_axes(d, axes)) return;
    const D3DMATRIX *w = &d->world;
    float size = sqrtf(w->_11 * w->_11 + w->_21 * w->_21 + w->_31 * w->_31);
    if (size < 1e-4f) return;
    float r[9] = { w->_11 / size, w->_21 / size, w->_31 / size, w->_12 / size, w->_22 / size, w->_32 / size, w->_13 / size, w->_23 / size, w->_33 / size };
    in_card *a = in_card_of(pa, n, idx, ni, r, axes), *b = pb == pa ? a : in_card_of(pb, n, idx, ni, r, axes);
    if (!a || !b) return;
    if (g_ic.nq >= RB_WORLDS) in_cards_flush(d);
    in_card_q *e = &g_ic.q[g_ic.nq++];
    e->a = a; e->b = b; e->frac = frac; e->size = size; e->idx = idx; e->ni = ni;
    e->at[0] = d->world._41; e->at[1] = d->world._42; e->at[2] = d->world._43;
    g_ic.tex = d->tex[0];
    g_ic.d = d;
    for (int k = 0; k < 2; k++) {
        float lo = fminf(a->lo[k], b->lo[k]) * size + e->at[k], hi = fmaxf(a->hi[k], b->hi[k]) * size + e->at[k];
        if (lo < g_sh.lo[k]) g_sh.lo[k] = lo;
        if (hi > g_sh.hi[k]) g_sh.hi[k] = hi;
    }
}

typedef struct { void *ras, *view; rd_device *d; unsigned long screen; float bias; } in_ctx;
// A SHIP'S UNDERSIDE (the player, 2026-11-11: "that the ships have no underside is very noticable now, you think the mesh
// undersides could be filled? a simple face betwen oposidte lowest edges or something like that"). The hulls are open
// below - on the water nobody saw it; sinking by their floats (collect.c us_*) they dive and roll over, and the inside
// showed. A sailing ship's open bottom is one flat loop at its hull material's lowest point (read from data.ssa: the
// frigate's 14 edges at z -0.03, the galley's 10 at -0.05, the Victory's 14; the sails' own open loops lie higher). For a
// ship's record (render_silhouette.c g_sl.ship) a material's list gets its flat lowest loops filled - once, by triangles
// between the loop's own vertices (cut off one ear at a time in the plane), wound as the triangles beside them, so they
// are the hull's texture, light and animation - and that list goes into the pool in its place: the shadows and the
// mirror draw from the same lists. A list's edges are the vertices' places' (a texture's seam is no edge), an open one is
// on one triangle; the loop runs against the way its triangles do. Lists without such a loop are kept as they are
// (marked), and so are the units drawn by the display DLL (not instanced: [display] unit_instancing off).
#define IN_CAPS 2048
#define IN_CAP_LOOP 256
typedef struct { const WORD *src; uint32_t n; WORD first[3], last[3]; WORD *out; uint32_t nout; } in_cap;
internal in_cap g_in_cap[IN_CAPS];
internal uint32_t g_in_caps, g_in_capped, g_in_cap_full;   // lists looked at, with a cap, the table full
typedef struct { uint32_t lo, hi; WORD from, to; } in_edge;   // an edge by its places (lo < hi), as one triangle has it
internal int in_edge_cmp(const void *a, const void *b) {
    const in_edge *x = a, *y = b;
    return x->lo != y->lo ? (x->lo < y->lo ? -1 : 1) : x->hi != y->hi ? (x->hi < y->hi ? -1 : 1) : 0;
}
// the caps of a list of ni indices into nv vertices (8 floats each: the place first) as triangles into out (room for
// max); their count of indices
internal uint32_t in_cap_make(const WORD *idx, uint32_t ni, const float *v, uint32_t nv, WORD *out, uint32_t max) {
    uint32_t *place = malloc(nv * sizeof *place), ne = 0, made = 0;
    in_edge *e = malloc(ni * sizeof *e);
    int32_t *next = malloc(nv * sizeof *next);
    WORD *real = malloc(nv * sizeof *real);
    uint8_t *done = calloc(nv, 1);
    if (!place || !e || !next || !real || !done) goto end;
    for (uint32_t i = 0; i < nv; i++) {   // one place, one vertex number (the first there)
        place[i] = i;
        for (uint32_t j = 0; j < i; j++)
            if (place[j] == j && fabsf(v[i * 8] - v[j * 8]) < 1e-4f && fabsf(v[i * 8 + 1] - v[j * 8 + 1]) < 1e-4f && fabsf(v[i * 8 + 2] - v[j * 8 + 2]) < 1e-4f) { place[i] = j; break; }
        next[i] = -1;
    }
    float lo[3] = { 1e30f, 1e30f, 1e30f }, hi[3] = { -1e30f, -1e30f, -1e30f };
    for (uint32_t k = 0; k + 2 < ni; k += 3)
        for (int c = 0; c < 3; c++) {
            WORD a = idx[k + c], b = idx[k + (c + 1) % 3];
            if (a >= nv || b >= nv) goto end;
            for (int j = 0; j < 3; j++) lo[j] = fminf(lo[j], v[a * 8 + j]), hi[j] = fmaxf(hi[j], v[a * 8 + j]);
            uint32_t pa = place[a], pb = place[b];
            if (pa == pb) continue;
            e[ne].lo = pa < pb ? pa : pb, e[ne].hi = pa < pb ? pb : pa, e[ne].from = a, e[ne].to = b, ne++;
        }
    qsort(e, ne, sizeof *e, in_edge_cmp);
    for (uint32_t i = 0; i < ne;) {   // an open edge: the loop runs against it (its "to" on to its "from")
        uint32_t j = i + 1;
        while (j < ne && !in_edge_cmp(&e[i], &e[j])) j++;
        if (j == i + 1) {
            uint32_t pt = place[e[i].to];
            next[pt] = (int32_t)place[e[i].from];
            real[pt] = e[i].to, real[place[e[i].from]] = e[i].from;
        }
        i = j;
    }
    // the bottom: of the closed loops the lowest (by their mean height) that runs at least half the hull's length and
    // across its middle - not an oar bank on one side, a sail, a hatch (the hulls' own low loops slope a little: the
    // carrack's from -0.06 to -0.03, and their oars hang lower than their floor; it is filled in the plane seen from below)
    int along = hi[1] - lo[1] >= hi[0] - lo[0], across = !along;
    float mid = 0.5f * (lo[across] + hi[across]), length = hi[along] - lo[along], best_z = 1e30f;
    uint32_t loop[IN_CAP_LOOP], n = 0;
    for (uint32_t s = 0; s < nv; s++) {
        if (next[s] < 0 || done[s]) continue;
        uint32_t l[IN_CAP_LOOP], ln = 0, p = s;
        while (ln < IN_CAP_LOOP && next[p] >= 0 && !done[p]) { done[p] = 1; l[ln++] = p; p = (uint32_t)next[p]; }
        if (p != s || ln < 4) continue;   // not closed
        float llo[2] = { 1e30f, 1e30f }, lhi[2] = { -1e30f, -1e30f }, z = 0.0f;
        for (uint32_t i = 0; i < ln; i++) {
            const float *q = v + real[l[i]] * 8;
            for (int j = 0; j < 2; j++) llo[j] = fminf(llo[j], q[j]), lhi[j] = fmaxf(lhi[j], q[j]);
            z += q[2] / (float)ln;
        }
        if (llo[across] < mid && lhi[across] > mid && lhi[along] - llo[along] >= 0.5f * length && z < best_z)
            best_z = z, n = ln, memcpy(loop, l, ln * sizeof *l);
    }
    if (n) {
        float area = 0.0f;
        for (uint32_t i = 0; i < n; i++) {
            const float *a = v + real[loop[i]] * 8, *b = v + real[loop[(i + 1) % n]] * 8;
            area += a[0] * b[1] - b[0] * a[1];
        }
        while (n >= 3 && made + 3 <= max) {   // the ears: a corner turning as the loop does, nothing of it inside
            uint32_t at = n;
            for (uint32_t i = 0; i < n && at == n; i++) {
                const float *a = v + real[loop[(i + n - 1) % n]] * 8, *b = v + real[loop[i]] * 8, *c = v + real[loop[(i + 1) % n]] * 8;
                float turn = (b[0] - a[0]) * (c[1] - b[1]) - (b[1] - a[1]) * (c[0] - b[0]);
                if (turn * area <= 0.0f && n > 3) continue;
                int inside = 0;
                for (uint32_t j = 0; j < n && !inside; j++) {
                    if (j == i || j == (i + 1) % n || j == (i + n - 1) % n) continue;
                    const float *q = v + real[loop[j]] * 8;
                    float d1 = (b[0] - a[0]) * (q[1] - a[1]) - (b[1] - a[1]) * (q[0] - a[0]);
                    float d2 = (c[0] - b[0]) * (q[1] - b[1]) - (c[1] - b[1]) * (q[0] - b[0]);
                    float d3 = (a[0] - c[0]) * (q[1] - c[1]) - (a[1] - c[1]) * (q[0] - c[0]);
                    inside = (d1 > 0 && d2 > 0 && d3 > 0) || (d1 < 0 && d2 < 0 && d3 < 0);
                }
                if (!inside) at = i;
            }
            if (at == n) break;   // (no ear: a loop that crosses itself - what is filled stays)
            out[made++] = real[loop[(at + n - 1) % n]], out[made++] = real[loop[at]], out[made++] = real[loop[(at + 1) % n]];
            for (uint32_t i = at; i + 1 < n; i++) loop[i] = loop[i + 1];
            n--;
        }
    }
end:
    free(place), free(e), free(next), free(real), free(done);
    return made;
}
// the list to draw for a ship's material: its own with its caps (made once), or itself
internal const WORD *in_capped(const WORD *idx, DWORD ni, const float *v, uint32_t nv, DWORD *nout) {
    *nout = ni;
    if (ni < 3) return idx;
    uint32_t h = (uint32_t)(((uintptr_t)idx >> 1) * 2654435761u ^ ni * 40503u) & (IN_CAPS - 1);
    for (int probe = 0; probe < 16; probe++, h = (h + 1) & (IN_CAPS - 1)) {
        in_cap *c = &g_in_cap[h];
        if (c->src == idx && c->n == ni && !memcmp(c->first, idx, 6) && !memcmp(c->last, idx + ni - 3, 6)) {
            if (c->out) *nout = c->nout;
            return c->out ? c->out : idx;
        }
        if (c->src && c->src == idx) free(c->out), c->src = NULL;   // its memory holds another list now
        if (c->src) continue;
        uint32_t room = ni / 2 + 3 * IN_CAP_LOOP;
        WORD *out = malloc((ni + room) * sizeof *out);
        uint32_t made = out ? in_cap_make(idx, ni, v, nv, out + ni, room) : 0;
        g_in_caps++;
        c->src = idx, c->n = ni, c->out = NULL, c->nout = ni;
        memcpy(c->first, idx, 6), memcpy(c->last, idx + ni - 3, 6);
        if (made && ni + made <= IN_LIST_MAX) {
            memcpy(out, idx, ni * sizeof *out);
            c->out = out, c->nout = ni + made, *nout = c->nout;
            if (g_in_capped++ < 64) log_locked("# instancing: a ship's list of %lu indices: its open bottom filled by %u triangles\n", (unsigned long)ni, made / 3);
            return out;
        }
        free(out);
        return idx;
    }
    g_in_cap_full++;
    return idx;
}
// what a model's parts are drawn with: its frames, the fraction between them, its opacity - the shared model's, as the
// game's record draw set them before DrawModelHierarchy, or a record's own (render_units.c: the mod's record draw,
// which leaves the model alone)
typedef struct { uint32_t frame, next; float frac, alpha; } in_pose;
// a record the mod draws itself (render_units.c): what the game's draw would have set on its model - the root's
// transformation over the frame's (outer) and its size, every part's pose - and the record's texture of frames
// (EEGraphicObject anim_keys: each material's frame of it, by the build's rule)
typedef struct { const float *outer; float scale; in_pose pose; ee_texture *tex; uint32_t root_mats; } in_rec;
internal int g_in_capture;   // render_units.c's comparison: the parts noted (1 only noted, 2 noted and drawn)
internal LONG g_in_walks;   // ... and the walks while it notes
internal void ud_note(ee_model *m, ee_material *mat, void *tex, const in_pose *p, const D3DMATRIX *world, const WORD *idx, DWORD ni);
// a material of a model seen: what the DLL does for it, without the device's calls
internal void in_part(const in_ctx *c, ee_model *m, ee_material *mat, void *tex, const in_pose *p, const D3DMATRIX *world, const WORD *idx, DWORD ni) {
    if (g_in_capture) {
        ud_note(m, mat, tex, p, world, idx, ni);
        if (g_in_capture == 1) return;
    }
    uint32_t n = material_get_vertex_count(mat), first = material_get_vertex_first(mat);
    ee_vertices **fr = model_get_vertices(m);
    float frac = p->frac;
    const float *pa = (const float *)vertices_get_data(fr[p->frame]) + first * 8;
    const float *pb = frac != 0 ? (const float *)vertices_get_data(fr[p->next]) + first * 8 : pa;
    rd_device *d = c->d;
    g_in.settex(c->ras, tex, c->screen);
    vec3 col = material_get_colour(mat);
    D3DMATERIAL7 mt;
    memset(&mt, 0, sizeof mt);
    mt.diffuse.r = mt.ambient.r = col.x; mt.diffuse.g = mt.ambient.g = col.y; mt.diffuse.b = mt.ambient.b = col.z;
    mt.diffuse.a = p->alpha * material_get_alpha(mat);
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
        if (g_ic.nq && d->tex[0] != g_ic.tex) in_cards_flush(d);
        g_mo_box = box;
        rd3_shadow_classify(d, &none, 0, n);
        g_mo_box = NULL;
        if (d->sh_role != SH_BLOB && rd3_prepare(d, 0)) {
            DWORD cn = ni;
            const WORD *ci = g_sl.ship ? in_capped(idx, ni, pa, n, &cn) : idx;   // a ship's: its open bottom filled
            in_append(d, a, b, frac, ci, cn);
        }
        if (d->sh_card && n <= SH_CARD_MAX) in_card_queue(d, pa, pb, frac, n, idx, ni);
        InterlockedIncrement(&g_in_parts);
    }
    g_rd_draw_qpc += rd_qpc() - t0;
    RD_UNLOCK();
}
// a x b (GETransformation::Apply: 4 x 4 by row, the place in the fourth column, the last row b's)
internal void in_mul(const float *a, const float *b, float *out) {
    for (int r = 0; r < 3; r++) {
        const float *ar = a + r * 4;
        for (int c = 0; c < 4; c++) out[r * 4 + c] = b[c] * ar[0] + b[4 + c] * ar[1] + b[8 + c] * ar[2] + (c == 3 ? ar[3] : 0.0f);
    }
    memcpy(out + 12, b + 12, 4 * sizeof(float));
}
internal const float *mt_frame_used(ee_model *m, uint32_t f, uint32_t nx, float fr, float *x);   // collect.c
// a model's transformation as PrepareTransforms makes it for a pose (outer x (the frame's x own, its 3 x 3 times
// scale)), without the model set to it
internal void in_local(ee_model *m, const float *outer, float scale, const in_pose *p, float *out) {
    float x[16], y[16];
    in_mul(mt_frame_used(m, p->frame, p->next, p->frac, x), model_own_at(m), y);
    for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) y[r * 4 + c] *= scale;
    y[12] = y[13] = y[14] = 0.0f, y[15] = 1.0f;
    in_mul(outer, y, out);
}
// a material's texture for a record: the game's record draw gives the root's first material and every other part's
// the record's texture, the root's others its frame by their texture_index (frames[0] past the count or for a texture of
// one frame) - and AoC's the root's children's materials too, when the root has more than one, by the index alone
internal void *in_tex(const in_rec *r, ee_material *mat, uint32_t i, int depth) {
    ee_texture *t = r->tex;
    if (!t || (depth == 0 && i == 0) || depth > 1 || (depth == 1 && (!build_is("aoc") || r->root_mats < 2))) return t;
    uint32_t k = material_get_texture_index(mat), n = texture_get_frame_count(t);
    ee_texture **fr = texture_get_frames(t);
    if (depth == 0) return k < n && !texture_get_one_frame(t) ? fr[k] : fr[0];
    return k < n ? (texture_get_one_frame(t) ? fr[0] : fr[k]) : t;
}
// a model and what hangs on it (parent: its parent's transformation, pscale its size; r: a record the mod draws, NULL
// for the game's draw, which set the model)
internal void in_node(const in_ctx *c, ee_model *m, const float *parent, float pscale, const in_rec *r, int depth) {
    float t[16], s = r && !parent ? r->scale : model_get_scale(m);
    in_pose own = { model_get_frame(m), model_get_next_frame(m), model_get_fraction(m), model_get_alpha(m) };
    const in_pose *p = r ? &r->pose : &own;
    if (r) {
        float l[16];
        in_local(m, parent ? model_outer_at(m) : r->outer, s, p, l);
        if (parent) { s *= pscale; in_mul(parent, l, t); }
        else memcpy(t, l, sizeof t);
    } else if (parent) { s *= pscale; g_in.apply(parent, model_transform_at(m), t); }
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
            if (tris) in_part(c, m, mat, r ? in_tex(r, mat, i, depth) : material_get_textures(mat)[c->screen], p, &world,
                              indices + material_get_polygon_first(mat)[lod] * 3, tris * 3);
        }
    }
    ee_model **ch = model_get_children(m);
    for (uint32_t i = 0, nc = model_get_child_count(m); i < nc; i++) in_node(c, ch[i], t, s, r, depth + 1);
}
// the whole hierarchy can be drawn this way, in this pose (own: by the mod's record draw - every part's frame
// transformations needed)
internal int in_model_ok(ee_model *m, const in_pose *p, int own) {
    in_pose mp = { model_get_frame(m), model_get_next_frame(m), model_get_fraction(m), 0 };
    if (!p) p = &mp;
    ee_vertices **fr = model_get_vertices(m);
    if (!fr || !fr[p->frame] || vertices_get_type(fr[0])) return 0;
    if (p->frac != 0 && !fr[p->next]) return 0;
    if (own && !model_get_frame_transforms(m)) return 0;
    ee_material **mats = model_get_materials(m);
    for (uint32_t i = 0, nm = model_get_material_count(m); i < nm; i++) {
        uint32_t n = material_get_vertex_count(mats[i]);
        if (!n || n > MO_MAX_VERTS || material_get_polygon_count(mats[i])[0] * 3 > IN_LIST_MAX) return 0;
    }
    ee_model **ch = model_get_children(m);
    for (uint32_t i = 0, nc = model_get_child_count(m); i < nc; i++) if (!in_model_ok(ch[i], own ? p : NULL, own)) return 0;
    return 1;
}
// the instanced draw can take a model now (but for the model's own test): a unit's record in the units' render
// object, the animation on the graphics card, no transformation pushed by someone else
internal int in_can(uint8_t ortho) {
    const uint32_t *st = g_in.stack;
    return in_on() && g_sl.in && !ortho && g_ro_cur == RO_UNITS && !g_mo_off && g_mo.on && g_rp_state != 2 && g_mo_smooth && g_shr_visible &&
           g_in.orig && !(st[1] && (st[2] - st[1]) & ~7u);
}
internal const in_rec *g_in_rec;   // in_draw_rec's record, for the walk its Draw comes to
internal int g_in_rec_fell;        // ... which couldn't draw it
internal rd_device *in_device(void *ras, unsigned long screen) {
    rd_device *d = draw_state_get_device(rasterizer_get_dstates(ras)[screen]);
    return d && d->rs[D3DRENDERSTATE_ZENABLE] ? d : NULL;
}
internal long __thiscall in_hook(void *ras, ee_model *m, unsigned long screen, float bias, uint8_t ortho) {
    const in_rec *r = g_in_rec;   // (in_draw_rec tested all of this)
    g_in_rec = NULL;
    if (g_in_capture) g_in_walks++;
    rd_device *d = r || (in_can(ortho) && in_model_ok(m, NULL, 0) && in_init()) ? in_device(ras, screen) : NULL;
    if (r && (!d || !in_can(ortho))) { g_in_rec_fell = 1; return 0; }   // (bent for this draw: walls.c) - the game's draw is to draw it
    if (!d) { InterlockedIncrement(&g_in_fell); return g_in.orig(ras, m, screen, bias, ortho); }
    in_ctx c = { ras, rasterizer_get_screen_state(ras, screen), d, screen, bias };
    in_node(&c, m, NULL, 0, r, 0);
    return 0;
}
// a record's model drawn by the mod's own record draw (render_units.c), the game's states for it set: through the
// display DLL's Draw as the game's draw does (its lock, its 3D mode), whose walk takes the record's values instead of
// the model's (the caller tested in_can, in_model_ok and in_init). 0: not drawn - the draw's hooks bent the model for it
// (walls.c bl_bend: a copy of its vertices, the DLL's to draw), the game's draw is to draw it
internal int in_draw_rec(ee_model *m, const in_rec *r, float bias) {
    g_in_rec = r;
    g_in_rec_fell = 0;
    rasterizer_draw_model(rasterizer(), m, (uint32_t)screen_index(), bias, (int8_t)*game_camera_fixed_at());
    g_in_rec = NULL;
    return !g_in_rec_fell;
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
