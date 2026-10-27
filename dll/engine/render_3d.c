// The own renderer's Direct3D 7 (render.c, docs/renderer.md): Direct3D7 (devices, vertex buffers, depth formats),
// the device (its states as the game sets them, Clear) and vertex buffers (memory the game writes, 36-byte vertices of
// FVF 0x152 in the game). Drawing the 3D on Direct3D 11 is the next step: the draws are counted ("# render: frame"
// once a minute) and not drawn yet.

// ---- vertex buffers ----
typedef struct { void **vt; LONG refs; DWORD caps, fvf, n, stride; uint8_t *mem; } rd_vb;
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
} rd_device;
internal volatile LONG g_rd_draws, g_rd_frames;
internal DWORD g_rd_minute_tick;

internal void rd3_dirty(void) {}   // the 3D pipeline sets its states again (next step: tracked here)

internal HRESULT __stdcall rd_dev_qi(rd_device *d, const GUID *iid, void **out) { (void)d; (void)iid; *out = NULL; return E_NOINTERFACE; }
internal ULONG __stdcall rd_dev_addref(rd_device *d) { return (ULONG)InterlockedIncrement(&d->refs); }
internal ULONG __stdcall rd_dev_release(rd_device *d) {
    LONG n = InterlockedDecrement(&d->refs);
    if (n > 0) return (ULONG)n;
    if (d->target) rd_sf_release(d->target);
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
internal void rd_frame_tick(void) {
    InterlockedIncrement(&g_rd_frames);
    DWORD now = GetTickCount();
    if (now - g_rd_minute_tick < 60000) return;
    g_rd_minute_tick = now;
    log_locked("# render: frames %ld, draws %ld (3D not drawn yet)\n", (long)g_rd_frames, (long)g_rd_draws);
}
internal HRESULT __stdcall rd_dev_begin(rd_device *d) { d->in_scene = 1; return D3D_OK; }
internal HRESULT __stdcall rd_dev_end(rd_device *d) { d->in_scene = 0; rd_frame_tick(); return D3D_OK; }
internal HRESULT __stdcall rd_dev_set_target(rd_device *d, rd_surface *t, DWORD flags) {
    (void)flags;
    if (!rd_is_surface(t) || t->kind != RK_TARGET) return DDERR_INVALIDPARAMS;
    rd_sf_addref(t);
    if (d->target) rd_sf_release(d->target);
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
    RD_LOCK();
    for (DWORD i = 0; i < (n ? n : 1); i++) {
        RECT r = { 0, 0, (LONG)t->desc.dwWidth, (LONG)t->desc.dwHeight };
        if (n && rects) { r.left = rects[i].x1; r.top = rects[i].y1; r.right = rects[i].x2; r.bottom = rects[i].y2; }
        int whole = r.left <= 0 && r.top <= 0 && r.right >= (LONG)t->desc.dwWidth && r.bottom >= (LONG)t->desc.dwHeight;
        if ((flags & D3DCLEAR_TARGET) && t->rtv) {
            float c[4] = { (float)(col >> 16 & 255) / 255.0f, (float)(col >> 8 & 255) / 255.0f, (float)(col & 255) / 255.0f, 1 };
            if (whole) ID3D11DeviceContext_ClearRenderTargetView(g_d3d.ctx, t->rtv, c);
            else rd2_rect(t, &r, NULL, 0, 0, NULL, c, 0);
        }
        if ((flags & D3DCLEAR_ZBUFFER) && t->zbuf && t->zbuf->dsv)
            ID3D11DeviceContext_ClearDepthStencilView(g_d3d.ctx, t->zbuf->dsv, D3D11_CLEAR_DEPTH, z, 0);   // the next step: rects
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
    if (x) *x = *m;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_transform(rd_device *d, D3DTRANSFORMSTATETYPE t, D3DMATRIX *m) {
    D3DMATRIX *x = rd_transform(d, t);
    if (x) *m = *x;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_set_viewport(rd_device *d, D3DVIEWPORT7 *v) { d->viewport = *v; return D3D_OK; }
internal HRESULT __stdcall rd_dev_get_viewport(rd_device *d, D3DVIEWPORT7 *v) { *v = d->viewport; return D3D_OK; }
internal HRESULT __stdcall rd_dev_set_material(rd_device *d, D3DMATERIAL7 *m) { d->material = *m; return D3D_OK; }
internal HRESULT __stdcall rd_dev_get_material(rd_device *d, D3DMATERIAL7 *m) { *m = d->material; return D3D_OK; }
internal HRESULT __stdcall rd_dev_set_light(rd_device *d, DWORD i, D3DLIGHT7 *l) {
    if (i >= 8) return DDERR_INVALIDPARAMS;
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
    d->light_on[i] = on != 0;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_light_enable(rd_device *d, DWORD i, BOOL *on) {
    if (i >= 8) return DDERR_INVALIDPARAMS;
    *on = d->light_on[i];
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_set_rs(rd_device *d, D3DRENDERSTATETYPE s, DWORD v) {
    if ((unsigned)s < RD_RS) d->rs[s] = v;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_rs(rd_device *d, D3DRENDERSTATETYPE s, DWORD *v) {
    *v = (unsigned)s < RD_RS ? d->rs[s] : 0;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_set_texture(rd_device *d, DWORD stage, rd_surface *t) {
    if (stage < 2) d->tex[stage] = rd_is_surface(t) ? t : NULL;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_texture(rd_device *d, DWORD stage, rd_surface **t) {
    *t = stage < 2 ? d->tex[stage] : NULL;
    if (*t) rd_sf_addref(*t);
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_set_tss(rd_device *d, DWORD stage, D3DTEXTURESTAGESTATETYPE t, DWORD v) {
    if (stage < 2 && (unsigned)t < RD_TSS) d->tss[stage][t] = v;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_tss(rd_device *d, DWORD stage, D3DTEXTURESTAGESTATETYPE t, DWORD *v) {
    *v = stage < 2 && (unsigned)t < RD_TSS ? d->tss[stage][t] : 0;
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_validate(rd_device *d, DWORD *passes) { (void)d; *passes = 1; return D3D_OK; }
internal HRESULT __stdcall rd_dev_draw_vb(rd_device *d, D3DPRIMITIVETYPE type, rd_vb *vb, DWORD start, DWORD n, DWORD flags) {
    (void)d; (void)type; (void)vb; (void)start; (void)n; (void)flags;
    InterlockedIncrement(&g_rd_draws);
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_draw_indexed_vb(rd_device *d, D3DPRIMITIVETYPE type, rd_vb *vb, DWORD start, DWORD nv,
                                                  WORD *idx, DWORD ni, DWORD flags) {
    (void)d; (void)type; (void)vb; (void)start; (void)nv; (void)idx; (void)ni; (void)flags;
    InterlockedIncrement(&g_rd_draws);
    return D3D_OK;
}
internal HRESULT __stdcall rd_dev_get_direct3d(rd_device *d, void **out);

// the device's defaults (Direct3D 7's)
internal void rd_device_defaults(rd_device *d) {
    static const D3DMATRIX id = { ._11 = 1, ._22 = 1, ._33 = 1, ._44 = 1 };
    d->world = d->view = d->proj = id;
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
    b->mem = calloc(b->n ? b->n : 1, b->stride ? b->stride : 4);
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
