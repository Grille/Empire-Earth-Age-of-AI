// THE TREES KEPT ON THE GRAPHICS CARD, BY CHUNK (the pipe: picture tchunks on | off | remake | (nothing: its counts); with
// the trees' own list, the kept trees and the instances - render_units.c). The player, 2026-11-23, after the zoomed-out
// planets' speed-ups: "yes, keep the trees on the GPU per chunk". Each frame every listed tree went through its kept parts
// on the CPU - its entry found, its parts' states set and prepared, its animation's runs and its card looked up, an entry
// made and uploaded for each part - though none of it changes while the trees, the camera and the light stand still. Now
// the list's trees are taken by chunk of TC_SIDE x TC_SIDE tiles (and by the anchor they are drawn after: in sight, or
// remembered under the fog). A chunk is made once: its trees drawn as before, but their parts' entries and their cards'
// taken into the chunk (render_instance.c g_tc_cap), sorted into groups of one state and one list, and put into a buffer
// of its own on the graphics card. Every frame it is drawn from there, a draw a group (render_frame.c rf_cmd wsrv). Its
// sway is the shader's: a kept tree's entry holds its place in the sway's circle (its id's), and the walk's clock picks
// the game's own sway for it from a table made with the fills' formula (UMCos; render_3d.c RD3_MORPH_HLSL).
// What a chunk holds doesn't follow the camera: its trees are drawn on the screen or off it alike (render_instance.c: the
// graphics card clips them; the shadows' "off the screen" role would change at every move), so a camera that moved
// only has each tree's detail looked at again (kt_lod). It is made again when anything it was made of changed: its trees'
// records (but their sway), a tree's detail, the cards' axes (the sun and the camera's turn, as far as a card is made
// again for: in_card_of), the pools its entries point into (the animation's runs, the lists), the kept entries' graphics,
// the shadows' switches. What its making added to the shadows' sums (the casters' box, the tallest) is added again each
// frame. A tree that can't be kept (kt_draw's own fallbacks) is drawn as before, every frame.
#define TC_SIDE 64       // a chunk's side, in tiles
#define TC_STATES 128    // the states the groups are drawn in (all chunks'; full: made anew, every chunk with it)
#define TC_GENS 5
typedef struct { rd_surface *tex; DWORD rs[RD_RS]; DWORD tss[RD_TSS]; int role, sil; float b; } tc_state;
typedef struct { uint32_t state, at, n, first, count; } tc_group;   // a draw: its state, its list's place, its entries
typedef struct { uint32_t state, at, n; rb_world e; } tc_ent;       // an entry taken while a chunk is made
typedef struct { in_card *a, *b; float size, at[3]; const WORD *idx; DWORD ni; uint32_t state; rb_world e; } tc_card;
typedef struct {
    int built;
    uint32_t gen[TC_GENS], view, cam;   // what it was made with (tc_gens, kt_view_state, g_tc.cam_state)
    uint32_t n, cap;               // its trees: their records with the sway out, in the list's order
    uint8_t *recs, *loose;         // ... which of them are drawn as before
    uint32_t *kt;                  // ... and each kept one's entry (its number in g_kt.t) and detail
    uint8_t *lod;
    uint32_t kt_cap, loose_cap, lod_cap;
    float axes[4];                 // the cards' axes it was made with (in_card_axes); cards: it has some
    int cards;
    tc_group *g;
    uint32_t ng, g_cap;
    ID3D11Buffer *buf;             // its entries, the groups' one after the other
    ID3D11ShaderResourceView *srv;
    uint32_t buf_cap;
    float lo[3], hi[3], tall;      // what its making added to the shadows' sums
} tc_chunk;
internal struct {
    int force;                     // the pipe's: 0 off, 1 on; -1: on
    tc_chunk *c;                   // [fog][chunk]
    uint32_t across, nc;           // chunks across the map, of one anchor
    ee_map *map;
    LONG starts;
    void *dev;
    tc_state st[TC_STATES];
    uint32_t nst, st_gen, last[2];
    tc_ent *ent;
    uint32_t nent, ent_cap;
    tc_card *card;
    uint32_t ncard, card_cap;
    rb_world *up;                  // a chunk's entries in its groups' order, for the upload
    uint32_t up_cap;
    uint32_t *cnt, *at, *ix, *ci;  // the frame's trees by chunk: counts, starts, the list's places, each's chunk
    uint32_t cnt_cap, at_cap, ix_cap, ci_cap;
    int q;                         // the tree being taken's place in the sway's circle; -1 none
    int fail;                      // the chunk being made can't be kept
    float cam[32];                 // the device's view and projection the camera's state was counted for
    uint32_t cam_state;            // ... its count
    float sw[360 + 4];             // the sway's table and the clock's place (the shader's b6)
    uint32_t sw_bits[3], sw_ok, sw_clock;
    LONG made, kept, looked, trees, loose, failed;
    LONG why[TC_GENS + 4];         // made for: none kept, another count of trees, a gen (tc_gens' order), records, details
} g_tc = { .force = -1 };
internal int tc_on(void) { return (g_tc.force < 0 ? 1 : g_tc.force) && ud_mode() == 1 && kt_on() && tl_on() && in_on(); }
internal int tc_grow(void **p, uint32_t *cap, uint32_t need, size_t size) {
    if (need <= *cap) return 1;
    uint32_t c = *cap ? *cap : 256;
    while (c < need) c *= 2;
    void *q = realloc(*p, (size_t)c * size);
    if (!q) return 0;
    *p = q, *cap = c;
    return 1;
}
internal void tc_gens(uint32_t *g) {
    g[0] = (uint32_t)g_mo.resets, g[1] = (uint32_t)g_in_resets;
    g[2] = (uint32_t)(g_kt.reloads + g_kt.changed + g_kt.starts_over);
    g[3] = (uint32_t)(g_sh.on != 0) | (uint32_t)(g_sh.tint != 0) << 1 | (uint32_t)(g_sh.nocards != 0) << 2;
    g[4] = g_tc.st_gen;
}
// the chunks given up (a new map or game, another device)
internal void tc_free(void) {
    for (uint32_t i = 0; g_tc.c && i < 2 * g_tc.nc; i++) {
        tc_chunk *ch = &g_tc.c[i];
        D3D_RELEASE(ch->srv); D3D_RELEASE(ch->buf);
        free(ch->recs); free(ch->loose); free(ch->kt); free(ch->lod); free(ch->g);
    }
    free(g_tc.c);
    g_tc.c = NULL, g_tc.nc = g_tc.across = 0;
    g_tc.nst = 0, g_tc.st_gen++;
}
internal int tc_chunks(ee_map *map) {
    uint32_t size = map_get_size(map), across = (size + TC_SIDE - 1) / TC_SIDE;
    if (g_tc.c && map == g_tc.map && g_tc.starts == g_game_starts && g_tc.dev == (void *)g_d3d.dev && across == g_tc.across) return 1;
    tc_free();
    if (!across) return 0;
    g_tc.c = calloc((size_t)2 * across * across, sizeof *g_tc.c);
    if (!g_tc.c) return 0;
    g_tc.across = across, g_tc.nc = across * across;
    g_tc.map = map, g_tc.starts = g_game_starts, g_tc.dev = (void *)g_d3d.dev;
    return 1;
}
// the sway's table: the fills' (collect.c uc_fill_tree) for each degree of the clock's circle, made anew when its constants
// change; the clock's place each walk. Into the shader's b6 (render_frame.c g_tc_swb, bound with the list's shaders)
internal int tc_sway(void) {
    fill_consts fc = fill_consts_now();
    tree_consts tc = tree_consts_now();
    opacity_consts oc = opacity_consts_now();
    uint32_t bits[3];
    float one = (float)oc.one;
    memcpy(&bits[0], &fc.tilt_offset, 4), memcpy(&bits[1], &tc.sway_step, 4), memcpy(&bits[2], &one, 4);
    int remake = !g_tc.sw_ok || memcmp(bits, g_tc.sw_bits, sizeof bits);
    if (remake) {
        for (int a = 0; a < 360; a++) {
            float arg = (float)((long double)fc.tilt_offset - (long double)(a - 180) * tc.sway_step);
            long double s = (game_cos_fpu(arg) + oc.one) * oc.half;
            if (!(s >= oc.zero)) s = 0;
            else if (s > oc.one) s = 1;
            g_tc.sw[a] = (float)s;
        }
        memcpy(g_tc.sw_bits, bits, sizeof bits);
    }
    uint32_t clock = g_tl.clock % 360;
    if (!g_tc_swb) {
        D3D11_BUFFER_DESC bd = { sizeof g_tc.sw, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
        if (FAILED(ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_tc_swb))) return 0;
        remake = 1;
    }
    if (remake || !g_tc.sw_ok || clock != g_tc.sw_clock) {
        memcpy(&g_tc.sw[360], &clock, 4);
        ID3D11DeviceContext_UpdateSubresource(g_d3d.ctx, (ID3D11Resource *)g_tc_swb, 0, NULL, g_tc.sw, 0, 0);
        ID3D11DeviceContext_VSSetConstantBuffers(g_d3d.ctx, 6, 1, &g_tc_swb);
        g_tc.sw_clock = clock, g_tc.sw_ok = 1;
    }
    return 1;
}
// the states a part is drawn in (those kt_draw and in_draw set: the texture, the render and stage states, the shadows'
// role), a role and a part in the silhouettes of its own: their place in the table. A tree's parts and their cards come
// by turns, each a run of one state: the last of each kind looked at first
internal uint32_t tc_state_of(const rd_device *d, int role, int sil) {
    tc_state s;
    memset(&s, 0, sizeof s);
    s.tex = d->tex[0];
    memcpy(s.rs, d->rs, sizeof s.rs);
    memcpy(s.tss, d->tss[0], sizeof s.tss);
    s.role = role, s.sil = sil;   // (sh_b, a part's size, goes into its entry: rd3_entry)
    uint32_t *last = &g_tc.last[role == SH_CARD];
    if (*last < g_tc.nst && !memcmp(&g_tc.st[*last], &s, sizeof s)) return *last;
    for (uint32_t i = 0; i < g_tc.nst; i++) if (!memcmp(&g_tc.st[i], &s, sizeof s)) return *last = i;
    if (g_tc.nst >= TC_STATES) { g_tc.fail = 1; return 0; }
    g_tc.st[g_tc.nst] = s;
    return *last = g_tc.nst++;
}
// render_instance.c in_append while a chunk is made: the part's entry into it
internal void tc_take(rd_device *d, const mo_run *a, const mo_run *b, float f, const WORD *idx, DWORD ni, DWORD cap_at) {
    const in_list *l = in_list_of(idx, ni, cap_at);
    if (!l || !tc_grow((void **)&g_tc.ent, &g_tc.ent_cap, g_tc.nent + 1, sizeof *g_tc.ent)) { g_tc.fail = 1; return; }
    tc_ent *t = &g_tc.ent[g_tc.nent++];
    rd3_entry(d, &t->e);
    t->e.mo[0] = (float)a->at, t->e.mo[1] = (float)b->at;
    t->e.mo[2] = g_tc.q >= 0 ? -1.0f - (float)g_tc.q : f;
    t->state = tc_state_of(d, d->sh_role, g_sl.in ? g_sl.tag : 0);
    t->at = l->at, t->n = l->n;
}
// render_instance.c in_card_queue while a chunk is made: the card into it, drawn as in_cards_flush draws a queue's - the
// tree's states, the cast's role, no part in the silhouettes
internal void tc_card_take(rd_device *d, in_card *a, in_card *b, float frac, float size, const float *at, const WORD *idx, DWORD ni) {
    if (!tc_grow((void **)&g_tc.card, &g_tc.card_cap, g_tc.ncard + 1, sizeof *g_tc.card)) { g_tc.fail = 1; return; }
    tc_card *c = &g_tc.card[g_tc.ncard++];
    c->a = a, c->b = b, c->size = size, c->idx = idx, c->ni = ni;
    memcpy(c->at, at, sizeof c->at);
    c->state = tc_state_of(d, SH_CARD, 0);
    rd3_entry(d, &c->e);
    c->e.mo[2] = g_tc.q >= 0 ? -1.0f - (float)g_tc.q : frac;
    c->e.mo[3] = 0;
}
internal int tc_ent_cmp(const void *x, const void *y) {
    const tc_ent *a = x, *b = y;
    if (a->state != b->state) return a->state < b->state ? -1 : 1;
    if (a->at != b->at) return a->at < b->at ? -1 : 1;
    return a->n < b->n ? -1 : a->n > b->n;
}
// a chunk made of the list's trees ix[0..n-1]: drawn as before but into it, its groups and buffer made. 1 kept; 0 not -
// the trees it took (loose 0) not drawn; -1 not even begun, none drawn
internal int tc_make(tc_chunk *ch, const uint32_t *ix, uint32_t n, ee_ro_units *ro, float *at, rd_device *d) {
    ch->built = 0;
    if (!tc_grow((void **)&ch->recs, &ch->cap, n, DRAW_SMALL_REC) || !tc_grow((void **)&ch->kt, &ch->kt_cap, n, sizeof *ch->kt)
        || !tc_grow((void **)&ch->loose, &ch->loose_cap, n, 1) || !tc_grow((void **)&ch->lod, &ch->lod_cap, n, 1)) return -1;
    ch->n = n;
    g_tc.nent = g_tc.ncard = 0;
    g_tc.fail = 0;
    // the shadows' sums of the frame put aside: the chunk's own taken, added at its draw as a kept one's are
    float lo[3], hi[3], tall = g_sh.tall_frame;
    memcpy(lo, g_sh.lo, sizeof lo), memcpy(hi, g_sh.hi, sizeof hi);
    for (int a = 0; a < 3; a++) g_sh.lo[a] = 1e30f, g_sh.hi[a] = -1e30f;
    g_sh.tall_frame = 0;
    LONG mo0 = g_mo.resets, in0 = g_in_resets;
    g_tc_taking = 1;
    for (uint32_t i = 0; i < n; i++) {
        uint8_t *r = g_tl.rec + (size_t)ix[i] * DRAW_SMALL_REC, *k = ch->recs + (size_t)i * DRAW_SMALL_REC;
        memcpy(k, r, DRAW_SMALL_REC);
        draw_small_record_set_sway(k, 0.0f);
        g_tc.q = g_tl.swq[ix[i]] == TL_NO_SWAY ? -1 : g_tl.swq[ix[i]];
        g_tc_took = 0;
        int drawn = ut_record(ro, at, r);
        ch->loose[i] = !g_tc_took;
        ch->kt[i] = g_tc_took ? (uint32_t)(g_tc_kt - g_kt.t) : 0;
        ch->lod[i] = g_tc_took ? (uint8_t)g_tc_lod : 0;
        if (!g_tc_took && !drawn) ro_units_draw_tree(ro, at, r);
    }
    g_tc_taking = 0, g_tc_cap = 0;
    ch->cards = g_tc.ncard > 0;
    if (!ch->cards || !in_card_axes(d, ch->axes)) memset(ch->axes, 0, sizeof ch->axes);
    // the cards: their runs in the animation's pool (as in_cards_flush puts them), at the tree's place and size
    for (uint32_t i = 0; i < g_tc.ncard && !g_tc.fail; i++) {
        tc_card *c = &g_tc.card[i];
        mo_run *ra = mo_run_of(c->a->v, c->a->n), *rb = c->b == c->a ? ra : mo_run_of(c->b->v, c->b->n);
        if (ra && rb != ra) ra = mo_run_of(c->a->v, c->a->n);
        if (!ra || !rb) continue;
        const in_list *l = in_list_of(c->idx, c->ni, c->ni);
        if (!l || !tc_grow((void **)&g_tc.ent, &g_tc.ent_cap, g_tc.nent + 1, sizeof *g_tc.ent)) { g_tc.fail = 1; break; }
        tc_ent *t = &g_tc.ent[g_tc.nent++];
        t->e = c->e;
        memset(t->e.m, 0, sizeof t->e.m);
        t->e.m[0] = t->e.m[5] = t->e.m[10] = c->size, t->e.m[15] = 1;
        t->e.m[12] = c->at[0], t->e.m[13] = c->at[1], t->e.m[14] = c->at[2];
        t->e.mo[0] = (float)ra->at, t->e.mo[1] = (float)rb->at;
        t->state = c->state, t->at = l->at, t->n = l->n;
    }
    memcpy(ch->lo, g_sh.lo, sizeof ch->lo), memcpy(ch->hi, g_sh.hi, sizeof ch->hi);
    ch->tall = g_sh.tall_frame;
    memcpy(g_sh.lo, lo, sizeof lo), memcpy(g_sh.hi, hi, sizeof hi);
    g_sh.tall_frame = tall;
    // a pool started over meanwhile: the entries taken before point at places it gave away
    if (g_mo.resets != mo0 || g_in_resets != in0) g_tc.fail = 1;
    if (g_tc.fail) {
        if (g_tc.nst >= TC_STATES) g_tc.nst = 0, g_tc.st_gen++;   // (the states' table full: made anew, every chunk with it)
        InterlockedIncrement(&g_tc.failed);
        return 0;
    }
    // the groups: the entries by state and list, one after the other in the buffer
    qsort(g_tc.ent, g_tc.nent, sizeof *g_tc.ent, tc_ent_cmp);
    if (!tc_grow((void **)&g_tc.up, &g_tc.up_cap, g_tc.nent ? g_tc.nent : 1, sizeof *g_tc.up)) return 0;
    ch->ng = 0;
    for (uint32_t i = 0; i < g_tc.nent; i++) {
        const tc_ent *t = &g_tc.ent[i];
        if (!ch->ng || ch->g[ch->ng - 1].state != t->state || ch->g[ch->ng - 1].at != t->at || ch->g[ch->ng - 1].n != t->n) {
            if (!tc_grow((void **)&ch->g, &ch->g_cap, ch->ng + 1, sizeof *ch->g)) return 0;
            tc_group *g = &ch->g[ch->ng++];
            g->state = t->state, g->at = t->at, g->n = t->n, g->first = i, g->count = 0;
        }
        ch->g[ch->ng - 1].count++;
        g_tc.up[i] = t->e;
    }
    if (g_tc.nent) {
        if (g_tc.nent > ch->buf_cap) {
            uint32_t bcap = ch->buf_cap ? ch->buf_cap : 64;
            while (bcap < g_tc.nent) bcap *= 2;
            D3D_RELEASE(ch->srv); D3D_RELEASE(ch->buf);
            ch->buf_cap = 0;
            D3D11_BUFFER_DESC bd = { bcap * (UINT)sizeof(rb_world), D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, 0 };
            HRESULT hr = ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &ch->buf);
            if (SUCCEEDED(hr)) {
                D3D11_SHADER_RESOURCE_VIEW_DESC sd = { .Format = DXGI_FORMAT_R32G32B32A32_FLOAT, .ViewDimension = D3D11_SRV_DIMENSION_BUFFER };
                sd.Buffer.NumElements = bcap * (UINT)sizeof(rb_world) / 16;
                hr = ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)ch->buf, &sd, &ch->srv);
            }
            if (FAILED(hr)) { D3D_RELEASE(ch->srv); D3D_RELEASE(ch->buf); InterlockedIncrement(&g_tc.failed); return 0; }
            ch->buf_cap = bcap;
        }
        D3D11_BOX box = { 0, 0, 0, g_tc.nent * (UINT)sizeof(rb_world), 1, 1 };
        ID3D11DeviceContext_UpdateSubresource(g_d3d.ctx, (ID3D11Resource *)ch->buf, 0, &box, g_tc.up, 0, 0);
    }
    tc_gens(ch->gen);
    ch->built = 1;
    InterlockedIncrement(&g_tc.made);
    return 1;
}
// a kept chunk still right for a camera that moved: each kept tree's detail the one it holds (kt_lod: the engine's size on
// the screen, kept by the viewport's state), and the cards' axes within the step a card is made again for (in_card_of)
internal int tc_still(tc_chunk *ch, const in_ctx *c0, rd_device *d) {
    if (ch->cards) {
        float axes[4];
        if (!in_card_axes(d, axes)) return 0;
        for (int i = 0; i < 4; i++) if (!(fabsf(axes[i] - ch->axes[i]) < 0.005f)) return 0;
    }
    InterlockedIncrement(&g_tc.looked);
    for (uint32_t i = 0; i < ch->n; i++) {
        if (ch->loose[i]) continue;
        kt_tree *k = &g_kt.t[ch->kt[i]];
        in_ctx c = *c0;
        c.bias = dmg_model_get_detail_bias(k->g);
        if (kt_lod(&c, k, k->m, k->scale) != ch->lod[i]) return 0;
    }
    return 1;
}
// a kept chunk drawn: its sums into the shadows', its groups as draws of its buffer's entries
internal void tc_emit(tc_chunk *ch, rd_device *d) {
    for (int a = 0; a < 3; a++) {
        if (ch->lo[a] < g_sh.lo[a]) g_sh.lo[a] = ch->lo[a];
        if (ch->hi[a] > g_sh.hi[a]) g_sh.hi[a] = ch->hi[a];
    }
    if (ch->tall > g_sh.tall_frame) g_sh.tall_frame = ch->tall;
    if (!ch->ng || !ch->srv) return;
    for (uint32_t i = 0; i < ch->ng; i++) {
        const tc_group *g = &ch->g[i];
        const tc_state *s = &g_tc.st[g->state];
        d->tex[0] = s->tex;
        memcpy(d->rs, s->rs, sizeof d->rs);
        memcpy(d->tss[0], s->tss, sizeof d->tss[0]);
        d->sh_role = s->role, d->sh_b = s->b, d->sh_card = 0;
        d->changed |= RD_CH_VC | RD_CH_RS | RD_CH_TEX;
        g_sl.in = s->sil != 0, g_sl.tag = s->sil;
        if (!rd3_prepare(d, 0)) continue;
        sil_batch();
        rd3_flush();
        rf_record_inst_w(g->at, g->n, g->count, g->first, ch->srv);
        InterlockedExchangeAdd(&g_rd_draws, (LONG)g->count);   // (the minute's line counts a part's draw)
    }
}
// render_units.c tl_after: the list's trees of this anchor (fog: the remembered ones') drawn by chunk; 0 not done here
internal int tc_draw(ee_ro_units *ro, float *at, int fog) {
    if (!tc_on() || !g_tl.swq) return 0;
    if (!shr_pass() || !in_can(*game_camera_fixed_at()) || !in_init() || !g_in.mode3d || !g_in.cs_enter || !g_in.cs_leave) return 0;
    ee_map *map = game_map();
    ee_rasterizer *ras = rasterizer();
    uint32_t screen = (uint32_t)screen_index();
    rd_device *d = map ? in_device(ras, screen) : NULL;
    if (!d || !tc_chunks(map) || !tc_sway()) return 0;
    void *cs = rasterizer_get_lock(ras);
    if (cs) g_in.cs_enter(cs);
    g_in.mode3d(ras, screen);   // (the camera's view and projection, as kt_draw takes them)
    // the camera's state: the viewport's (the details) and the device's view and projection (the cards' axes): a chunk
    // made for another looks at its trees' details again (tc_still)
    float cam[32];
    memcpy(cam, &d->view._11, 64), memcpy(cam + 16, &d->proj._11, 64);
    if (memcmp(cam, g_tc.cam, sizeof cam)) memcpy(g_tc.cam, cam, sizeof cam), g_tc.cam_state++;
    in_ctx c = { ras, rasterizer_get_screen_state(ras, screen), d, screen, 0 };
    uint32_t view = kt_view_state(c.view), gen[TC_GENS];
    if (cs) g_in.cs_leave(cs);
    tc_gens(gen);
    // the anchor's trees by chunk, each chunk's in the list's order
    uint32_t n = g_tl.n, nc = g_tc.nc, m = 0;
    if (!tc_grow((void **)&g_tc.cnt, &g_tc.cnt_cap, nc + 1, sizeof *g_tc.cnt) || !tc_grow((void **)&g_tc.at, &g_tc.at_cap, nc + 1, sizeof *g_tc.at)
        || !tc_grow((void **)&g_tc.ix, &g_tc.ix_cap, n + 1, sizeof *g_tc.ix) || !tc_grow((void **)&g_tc.ci, &g_tc.ci_cap, n + 1, sizeof *g_tc.ci))
        return 0;
    memset(g_tc.cnt, 0, (size_t)(nc + 1) * sizeof *g_tc.cnt);
    for (uint32_t i = 0; i < n; i++) {
        if (!tl_of(i, fog)) { g_tc.ci[i] = nc; continue; }
        vec3 p = draw_small_record_get_pos(g_tl.rec + (size_t)i * DRAW_SMALL_REC);
        int cx = (int)p.x / TC_SIDE, cy = (int)p.y / TC_SIDE;
        if (cx < 0) cx = 0;
        if (cy < 0) cy = 0;
        if ((uint32_t)cx >= g_tc.across) cx = (int)g_tc.across - 1;
        if ((uint32_t)cy >= g_tc.across) cy = (int)g_tc.across - 1;
        g_tc.ci[i] = (uint32_t)cy * g_tc.across + (uint32_t)cx;
        g_tc.cnt[g_tc.ci[i]]++;
        m++;
    }
    for (uint32_t k = 0, sum = 0; k < nc; k++) g_tc.at[k] = sum, sum += g_tc.cnt[k];
    for (uint32_t i = 0; i < n; i++) if (g_tc.ci[i] < nc) g_tc.ix[g_tc.at[g_tc.ci[i]]++] = i;
    for (uint32_t k = 0; k < nc; k++) g_tc.at[k] -= g_tc.cnt[k];   // (back to each's start)
    int in = g_sl.in, tag = g_sl.tag;
    DWORD rs[RD_RS], tss[RD_TSS];
    uint8_t rk[DRAW_SMALL_REC];
    for (uint32_t k = 0; k < nc; k++) {
        uint32_t cn = g_tc.cnt[k];
        if (!cn) continue;
        const uint32_t *ix = g_tc.ix + g_tc.at[k];
        tc_chunk *ch = &g_tc.c[(uint32_t)fog * nc + k];
        int same = ch->built && ch->n == cn && !memcmp(ch->gen, gen, sizeof gen), why = !ch->built ? 0 : ch->n != cn ? 1 : -1;
        for (int g = 0; why < 0 && g < TC_GENS; g++) if (ch->gen[g] != gen[g]) why = 2 + g;
        for (uint32_t i = 0; same && i < cn; i++) {
            memcpy(rk, g_tl.rec + (size_t)ix[i] * DRAW_SMALL_REC, sizeof rk);
            draw_small_record_set_sway(rk, 0.0f);
            same = !memcmp(rk, ch->recs + (size_t)i * DRAW_SMALL_REC, sizeof rk);
            if (!same) why = 2 + TC_GENS;
        }
        if (same && (ch->view != view || ch->cam != g_tc.cam_state)) {
            if (cs) g_in.cs_enter(cs);
            same = tc_still(ch, &c, d);
            if (cs) g_in.cs_leave(cs);
            if (same) ch->view = view, ch->cam = g_tc.cam_state;
            else why = 3 + TC_GENS;
        }
        if (!same && why >= 0) InterlockedIncrement(&g_tc.why[why]);
        int made = 0;
        if (!same) {
            g_sl.in = in, g_sl.tag = tag;
            made = tc_make(ch, ix, cn, ro, at, d);
            if (made <= 0) {   // not kept: its trees as before - those it took (or all, not begun) drawn now
                for (uint32_t i = 0; i < cn; i++) {
                    if (made == 0 && ch->loose[i]) continue;
                    uint8_t *r = g_tl.rec + (size_t)ix[i] * DRAW_SMALL_REC;
                    if (!ut_record(ro, at, r)) ro_units_draw_tree(ro, at, r);
                }
                ch->built = 0;
                continue;
            }
            ch->view = view, ch->cam = g_tc.cam_state;
        } else InterlockedIncrement(&g_tc.kept);
        // The groups' states are set on the device directly, the display DLL not asked: put back after them as they were
        // just before, which is what the DLL holds it set (it leaves out a state it believes set already). Put back to the
        // list's start instead, after a chunk was made by drawing its trees through the DLL, its trees' later states
        // weren't set - the DLL believed them set - and they came out dark (2026-11-24)
        if (cs) g_in.cs_enter(cs);
        rd_surface *tex = d->tex[0];
        memcpy(rs, d->rs, sizeof rs), memcpy(tss, d->tss[0], sizeof tss);
        int role = d->sh_role, card = d->sh_card;
        float b = d->sh_b;
        tc_emit(ch, d);
        d->tex[0] = tex;
        memcpy(d->rs, rs, sizeof rs), memcpy(d->tss[0], tss, sizeof tss);
        d->sh_role = role, d->sh_card = card, d->sh_b = b;
        d->changed |= RD_CH_VC | RD_CH_RS | RD_CH_TEX;
        g_sl.in = in, g_sl.tag = tag;
        memcpy(g_sh.view, &d->view._11, sizeof g_sh.view);
        if (cs) g_in.cs_leave(cs);
        InterlockedExchangeAdd(&g_tc.trees, (LONG)cn);
        // those it couldn't keep, as before (tc_make drew them already)
        for (uint32_t i = 0; !made && i < cn; i++) {
            if (!ch->loose[i]) continue;
            uint8_t *r = g_tl.rec + (size_t)ix[i] * DRAW_SMALL_REC;
            if (!ut_record(ro, at, r)) ro_units_draw_tree(ro, at, r);
            InterlockedIncrement(&g_tc.loose);
        }
    }
    InterlockedExchangeAdd(&g_tl.drawn, (LONG)m);
    return 1;
}
internal int tc_pipe(char *out, const char *arg) {
    if (!strcmp(arg, "on") || !strcmp(arg, "off")) {
        g_tc.force = arg[1] == 'n';
        REPLY("the trees kept by chunk %s\n", g_tc.force ? "on" : "off");
        return 1;
    }
    if (!strcmp(arg, "remake")) {   // every chunk made again at its next draw
        g_tc.st_gen++;
        REPLY("the trees kept by chunk: all to be made again\n");
        return 1;
    }
    if (arg[0]) { REPLY("error: usage: picture tchunks [on | off | remake]\n"); return 1; }
    uint32_t built = 0, ents = 0;
    for (uint32_t i = 0; g_tc.c && i < 2 * g_tc.nc; i++)
        if (g_tc.c[i].built) { built++; for (uint32_t j = 0; j < g_tc.c[i].ng; j++) ents += g_tc.c[i].g[j].count; }
    REPLY("the trees kept by chunk: %s; %u chunks of %u tiles kept now (%u entries, %u states); made %ld, drawn kept %ld (after a camera's move looked at %ld), trees drawn by chunk %ld, of them as before %ld; failed %ld\n",
          tc_on() ? "on" : "off", built, TC_SIDE, ents, g_tc.nst, (long)g_tc.made, (long)g_tc.kept, (long)g_tc.looked, (long)g_tc.trees, (long)g_tc.loose, (long)g_tc.failed);
    REPLY("  made for: none kept %ld, another count %ld, the animation pool %ld, the instance pool %ld, kept trees taken again %ld, the shadows' switches %ld, the states' table %ld, records %ld, details %ld\n",
          (long)g_tc.why[0], (long)g_tc.why[1], (long)g_tc.why[2], (long)g_tc.why[3], (long)g_tc.why[4], (long)g_tc.why[5], (long)g_tc.why[6],
          (long)g_tc.why[7], (long)g_tc.why[8]);
    return 1;
}
