// THE UNITS' ANIMATION ON THE GRAPHICS CARD ([display] gpu_animation; the pipe: picture morph on | off). A model's
// animation is a whole vertex array per frame (spec GEModel.vertices), and the display DLL draws a material of it by
// copying its run of the frame drawn into the vertex buffer - blended with the next frame's by the model's fraction
// (CopyAndSmoothStdVertices: position and normal, the texture coordinates the first's) - at every draw, every frame:
// on the 8000-unit save 4566 draws, 296 000 vertices a frame blended on the CPU and uploaded again (with the copy into
// the batch, a fifth of the drawing thread; docs/renderer.md). The frames don't change: each run goes once into a pool
// on the GPU (its 32-byte vertices as they are, two float4 each), and the draw is a batch entry - the two runs' places
// and the fraction - whose vertex shader blends them (rd3 "morph"). The copy functions are hooked: they note what they
// would copy and copy nothing; the draw that follows takes the note, or has it copied after all where its vertices are
// read on the CPU (a tree's shadow card, the probe, a draw without depth) - and so does a note no draw took.
#define MO_POOL_VERTS (1u << 21)   // 64 MB; full, it starts over (the list drawn first)
#define MO_SLOTS 16384             // the runs known, open addressing; three quarters full it starts over too
#define MO_MAX_VERTS 4096          // a draw's vertices: the index's 12 bits
typedef struct {
    const float *src;     // the run in the model's frame array
    uint32_t n, at;       // its vertices, its place in the pool (vertices)
    float first[8], last[8];   // its first and last vertex when it went in: memory reused for another model's frame
    float lo[3], hi[3];   // its box in the model's space (the shadows' role)
} mo_run;
internal struct {
    int on, ready;
    ID3D11Buffer *buf;
    ID3D11ShaderResourceView *srv;
    uint32_t used;
    int n;
    mo_run run[MO_SLOTS];
    LONG uploads, resets, draws, copied;   // the minute's line
    LONG why[4];   // the copies made: not the GPU's to blend (bent, not a unit, too big), not the draw's, its card, its probe
} g_mo = { .on = 1 };
// the copy noted, not done
typedef void (__thiscall *mo_copy_fn)(void *ras, ee_model *m, ee_material *mat, uint8_t *dst);
internal mo_copy_fn g_mo_copy, g_mo_smooth;   // the DLL's own (trampolines)
internal struct {
    uint8_t *dst;   // NULL: none pending
    void *ras;
    ee_model *m;
    ee_material *mat;
    int smooth;
    const float *a, *b;
    float f;
    uint32_t n;
} g_mo_note;
internal int g_mo_off;   // walls.c: a model bent for this draw (building_lean) - its arrays aren't its frames now

internal int mo_init(void) {
    if (g_mo.ready) return g_mo.ready > 0;
    g_mo.ready = -1;
    D3D11_BUFFER_DESC bd = { MO_POOL_VERTS * 32, D3D11_USAGE_DEFAULT, D3D11_BIND_SHADER_RESOURCE, 0, 0, 0 };
    HRESULT hr = ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_mo.buf);
    D3D11_SHADER_RESOURCE_VIEW_DESC sd = { .Format = DXGI_FORMAT_R32G32B32A32_FLOAT, .ViewDimension = D3D11_SRV_DIMENSION_BUFFER };
    sd.Buffer.NumElements = MO_POOL_VERTS * 2;
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)g_mo.buf, &sd, &g_mo.srv);
    if (FAILED(hr)) return rd_fail("the animation's pool", hr), 0;
    g_mo_srv = g_mo.srv;
    g_mo.ready = 1;
    return 1;
}
// the run's place in the pool, put there if new (or changed); NULL when it can't be
internal mo_run *mo_run_of(const float *src, uint32_t n) {
    uint32_t h = (uint32_t)(((uintptr_t)src >> 5) * 2654435761u ^ n * 40503u) & (MO_SLOTS - 1);
    mo_run *r;
    for (;; h = (h + 1) & (MO_SLOTS - 1)) {
        r = &g_mo.run[h];
        if (!r->src) break;
        if (r->src == src && r->n == n) {
            if (!memcmp(r->first, src, 32) && !memcmp(r->last, src + (n - 1) * 8, 32)) return r;
            break;   // its memory holds other vertices now: put in again, here
        }
    }
    if (g_mo.used + n > MO_POOL_VERTS || (!r->src && g_mo.n >= MO_SLOTS * 3 / 4)) {
        rd3_submit();   // the list draws from the pool as it is
        memset(g_mo.run, 0, sizeof g_mo.run);
        g_mo.used = 0;
        g_mo.n = 0;
        g_mo.resets++;
        return mo_run_of(src, n);
    }
    if (!r->src) g_mo.n++;
    r->src = src;
    r->n = n;
    r->at = g_mo.used;
    memcpy(r->first, src, 32);
    memcpy(r->last, src + (n - 1) * 8, 32);
    for (int a = 0; a < 3; a++) { r->lo[a] = 1e30f; r->hi[a] = -1e30f; }
    for (uint32_t k = 0; k < n; k++)
        for (int a = 0; a < 3; a++) {
            float v = src[k * 8 + a];
            if (v < r->lo[a]) r->lo[a] = v;
            if (v > r->hi[a]) r->hi[a] = v;
        }
    D3D11_BOX box = { r->at * 32, 0, 0, (r->at + n) * 32, 1, 1 };
    ID3D11DeviceContext_UpdateSubresource(g_d3d.ctx, (ID3D11Resource *)g_mo.buf, 0, &box, src, 0, 0);
    g_mo.used += n;
    g_mo.uploads++;
    return r;
}
// the pool forgotten: every run put in again when next drawn (render_units.c freed the bent copies it held)
internal void mo_forget(void) {
    RD_LOCK();
    rd3_submit();
    memset(g_mo.run, 0, sizeof g_mo.run);
    g_mo.used = 0;
    g_mo.n = 0;
    g_mo.resets++;
    RD_UNLOCK();
}
// the noted copy done after all
internal void mo_copy_now(void) {
    if (!g_mo_note.dst) return;
    uint8_t *dst = g_mo_note.dst;
    g_mo_note.dst = NULL;
    (g_mo_note.smooth ? g_mo_smooth : g_mo_copy)(g_mo_note.ras, g_mo_note.m, g_mo_note.mat, dst);
    g_mo.copied++;
}
internal void mo_note(void *ras, ee_model *m, ee_material *mat, uint8_t *dst, int smooth) {
    mo_copy_now();   // one before, not drawn
    uint32_t n = material_get_vertex_count(mat), first = material_get_vertex_first(mat);
    ee_vertices **fr = model_get_vertices(m);
    ee_vertices *va = fr ? fr[model_get_frame(m)] : NULL, *vb = fr && smooth ? fr[model_get_next_frame(m)] : va;
    g_mo_note.dst = dst; g_mo_note.ras = ras; g_mo_note.m = m; g_mo_note.mat = mat; g_mo_note.smooth = smooth;
    if (!g_mo.on || g_mo_off || g_ro_cur != RO_UNITS || !va || !vb || !n || n > MO_MAX_VERTS) { g_mo.why[0]++; mo_copy_now(); return; }
    g_mo_note.a = (const float *)vertices_get_data(va) + first * 8;
    g_mo_note.b = (const float *)vertices_get_data(vb) + first * 8;
    g_mo_note.f = smooth ? model_get_fraction(m) : 0;
    g_mo_note.n = n;
}
internal void __thiscall mo_hook_copy(void *ras, ee_model *m, ee_material *mat, uint8_t *dst) { mo_note(ras, m, mat, dst, 0); }
internal void __thiscall mo_hook_smooth(void *ras, ee_model *m, ee_material *mat, uint8_t *dst) { mo_note(ras, m, mat, dst, 1); }

// the draw of the noted copy's vertices: its two runs (*b the second), the box around both for the shadows' role; 0
// when it draws from the vertex buffer (the copy done now if it was noted)
internal int mo_take(rd_device *d, D3DPRIMITIVETYPE type, rd_vb *vb, DWORD start, DWORD nv, mo_run **a, mo_run **b, float *box) {
    if (!g_mo_note.dst) return 0;
    if (g_mo_note.dst != vb->mem + (size_t)start * vb->stride || g_mo_note.n != nv || type != D3DPT_TRIANGLELIST ||
        vb->stride != 36 || !d->rs[D3DRENDERSTATE_ZENABLE] || g_rp_state == 2 || !mo_init()) {
        g_mo.why[g_rp_state == 2 ? 3 : 1]++;
        mo_copy_now();
        return 0;
    }
    *a = mo_run_of(g_mo_note.a, nv);
    *b = g_mo_note.b == g_mo_note.a ? *a : mo_run_of(g_mo_note.b, nv);
    if (*a && *b != *a) *a = mo_run_of(g_mo_note.a, nv);   // the second's put in may have started the pool over
    if (!*a || !*b) { mo_copy_now(); return 0; }
    for (int k = 0; k < 3; k++) {
        box[k] = fminf((*a)->lo[k], (*b)->lo[k]);
        box[3 + k] = fmaxf((*a)->hi[k], (*b)->hi[k]);
    }
    return 1;
}

// the display DLL loaded (render.c render_display_loaded): the T&L DLL's copies hooked, how many (the software DLL
// has the same - its own, its vertices made in its own transform)
internal int mo_display_loaded(HMODULE m, const char *name) {
    if (_stricmp(name, "DX7HRTnLDisplay.dll")) return 0;
    static const uint8_t copy[10] = { 0x8B, 0x44, 0x24, 0x04, 0x8B, 0x88, 0x34, 0x01, 0x00, 0x00 };   // mov eax, [esp+4]; mov ecx, [eax+0x134]
    static const uint8_t smooth[10] = { 0x8B, 0x4C, 0x24, 0x04, 0x8B, 0x81, 0x34, 0x01, 0x00, 0x00 }; // mov ecx, [esp+4]; mov eax, [ecx+0x134]
    uintptr_t fc = (uintptr_t)GetProcAddress(m, "?CopyStdVertices@DX7Rasterizer@@IAEXPAVGEModel@@PAVGEMaterial@@PAUD3DVERTEX_STD_COLOR@@@Z");
    uintptr_t fs = (uintptr_t)GetProcAddress(m, "?CopyAndSmoothStdVertices@DX7Rasterizer@@IAEXPAVGEModel@@PAVGEMaterial@@PAUD3DVERTEX_STD_COLOR@@@Z");
    if (!fc || !fs || g_mo_copy) return 0;
    void *oc = NULL, *os = NULL;
    if (!patch_entry(fc, copy, 10, (void *)mo_hook_copy, &oc)) return 0;
    g_mo_copy = (mo_copy_fn)oc;
    if (!patch_entry(fs, smooth, 10, (void *)mo_hook_smooth, &os)) { g_mo.on = 0; return 1; }   // the copy hook copies then
    g_mo_smooth = (mo_copy_fn)os;
    return 2;
}
