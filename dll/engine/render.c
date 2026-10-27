// The mod's own renderer ([display] renderer = own; docs/renderer.md): the DirectDraw 7 / Direct3D 7 the display DLLs
// call, on Direct3D 11, instead of the ddraw.dll in the game folder (a wrapper: DDrawCompat covers the game window
// with its own - screenshots, recording and the game's warning dialogs fail -, GOG's dgVoodoo halves the frame rate).
// Only the calls the game makes are implemented (measured by the probe, dll/test/dxprobe.c); every other method logs
// once ("# render: X.Y not implemented") and returns E_NOTIMPL, so a call nobody measured shows up at once.
//
// The objects are C structs whose first field is the vtable; a vtable is an array filled from k_rd_args (each method's
// argument count with its this pointer - a stdcall method pops them, so the stub of a method that isn't implemented
// must know them) with a logging stub per slot, then the implemented methods over it. The display DLLs' import of
// DirectDrawCreateEx (and of the enumeration) is replaced at their load. Windowed / borderless only (the mod's
// display mode): the game never presents itself there - the mod's ShowScene hook calls render_present, which draws
// the render target into the window's swap chain (present_d3d.c) without reading it back.
//
// Surfaces: CPU memory for everything the game writes (system memory surfaces, the UI's sprites, textures and their
// mip levels), uploaded to a Direct3D 11 texture when the GPU needs it and the pixels changed (B8G8R8A8: 16-bit
// formats are converted on the way); the render target and the depth buffer live on the GPU, a lock reads the target
// back (only what is locked) and writes it again at the unlock. All under one lock: the device context is single
// threaded, the game and the mod's own code call from more than one thread.
// render_3d.c: Direct3D 7, the device, vertex buffers.

// g_rd_on: display.c (its present path asks it first)
// g_rd_cs: display.c (its pipe command "profile stacks" names the owner)
#define RD_LOCK() EnterCriticalSection(&g_rd_cs)
#define RD_UNLOCK() LeaveCriticalSection(&g_rd_cs)

enum { RDI_DD, RDI_SURF, RDI_D3D, RDI_DEV, RDI_VB, RDI_N };
internal const char *const k_rd_iface[RDI_N] = { "DirectDraw7", "Surface7", "Direct3D7", "Device7", "VertexBuffer7" };
internal const char *const k_rd_dd[] = { "QueryInterface", "AddRef", "Release", "Compact", "CreateClipper",
    "CreatePalette", "CreateSurface", "DuplicateSurface", "EnumDisplayModes", "EnumSurfaces", "FlipToGDISurface",
    "GetCaps", "GetDisplayMode", "GetFourCCCodes", "GetGDISurface", "GetMonitorFrequency", "GetScanLine",
    "GetVerticalBlankStatus", "Initialize", "RestoreDisplayMode", "SetCooperativeLevel", "SetDisplayMode",
    "WaitForVerticalBlank", "GetAvailableVidMem", "GetSurfaceFromDC", "RestoreAllSurfaces", "TestCooperativeLevel",
    "GetDeviceIdentifier", "StartModeTest", "EvaluateMode" };
internal const char *const k_rd_surf[] = { "QueryInterface", "AddRef", "Release", "AddAttachedSurface",
    "AddOverlayDirtyRect", "Blt", "BltBatch", "BltFast", "DeleteAttachedSurface", "EnumAttachedSurfaces",
    "EnumOverlayZOrders", "Flip", "GetAttachedSurface", "GetBltStatus", "GetCaps", "GetClipper", "GetColorKey", "GetDC",
    "GetFlipStatus", "GetOverlayPosition", "GetPalette", "GetPixelFormat", "GetSurfaceDesc", "Initialize", "IsLost",
    "Lock", "ReleaseDC", "Restore", "SetClipper", "SetColorKey", "SetOverlayPosition", "SetPalette", "Unlock",
    "UpdateOverlay", "UpdateOverlayDisplay", "UpdateOverlayZOrder", "GetDDInterface", "PageLock", "PageUnlock",
    "SetSurfaceDesc", "SetPrivateData", "GetPrivateData", "FreePrivateData", "GetUniquenessValue",
    "ChangeUniquenessValue", "SetPriority", "GetPriority", "SetLOD", "GetLOD" };
internal const char *const k_rd_d3d[] = { "QueryInterface", "AddRef", "Release", "EnumDevices", "CreateDevice",
    "CreateVertexBuffer", "EnumZBufferFormats", "EvictManagedTextures" };
internal const char *const k_rd_dev[] = { "QueryInterface", "AddRef", "Release", "GetCaps", "EnumTextureFormats",
    "BeginScene", "EndScene", "GetDirect3D", "SetRenderTarget", "GetRenderTarget", "Clear", "SetTransform",
    "GetTransform", "SetViewport", "MultiplyTransform", "GetViewport", "SetMaterial", "GetMaterial", "SetLight",
    "GetLight", "SetRenderState", "GetRenderState", "BeginStateBlock", "EndStateBlock", "PreLoad", "DrawPrimitive",
    "DrawIndexedPrimitive", "SetClipStatus", "GetClipStatus", "DrawPrimitiveStrided", "DrawIndexedPrimitiveStrided",
    "DrawPrimitiveVB", "DrawIndexedPrimitiveVB", "ComputeSphereVisibility", "GetTexture", "SetTexture",
    "GetTextureStageState", "SetTextureStageState", "ValidateDevice", "ApplyStateBlock", "CaptureStateBlock",
    "DeleteStateBlock", "CreateStateBlock", "Load", "LightEnable", "GetLightEnable", "SetClipPlane", "GetClipPlane",
    "GetInfo" };
internal const char *const k_rd_vb[] = { "QueryInterface", "AddRef", "Release", "Lock", "Unlock", "ProcessVertices",
    "GetVertexBufferDesc", "Optimize", "ProcessVerticesStrided" };
internal const char *const *const k_rd_names[RDI_N] = { k_rd_dd, k_rd_surf, k_rd_d3d, k_rd_dev, k_rd_vb };
internal const int k_rd_slots[RDI_N] = { 30, 49, 8, 49, 9 };
// each method's arguments with this (ddraw.h, d3d.h): what its stub pops
internal const uint8_t k_rd_args[RDI_N][49] = {
    { 3, 1, 1, 1, 4, 5, 4, 3, 5, 5, 1, 3, 2, 3, 2, 2, 2, 2, 2, 1, 3, 6, 3, 4, 3, 1, 1, 3, 4, 3 },
    { 3, 1, 1, 2, 2, 6, 4, 6, 3, 3, 4, 3, 3, 2, 2, 2, 3, 2, 2, 3, 2, 2, 2, 3, 1, 5, 2, 1, 2, 3, 3, 2, 2, 6, 2, 3, 2,
      2, 2, 3, 5, 4, 2, 2, 1, 2, 2, 2, 2 },
    { 3, 1, 1, 3, 4, 4, 4, 1 },
    { 3, 1, 1, 2, 3, 1, 1, 2, 3, 2, 7, 3, 3, 2, 3, 2, 2, 2, 3, 3, 3, 3, 1, 2, 2, 6, 8, 2, 2, 6, 8, 6, 8, 6, 3, 3, 4,
      4, 2, 2, 2, 2, 3, 6, 3, 3, 3, 3, 4 },
    { 3, 1, 1, 4, 1, 8, 2, 3, 8 },
};
internal void *g_rd_vt[RDI_N][49];
internal volatile LONG g_rd_told[RDI_N][49];

// the stubs' call: logged once per method
internal void __cdecl rd_unimplemented(uint32_t id) {
    int i = (int)(id >> 8), s = (int)(id & 0xFF);
    if (i < RDI_N && s < 49 && !InterlockedExchange(&g_rd_told[i][s], 1))
        log_locked("# render: %s.%s not implemented\n", k_rd_iface[i], k_rd_names[i][s]);
}
// push id; call rd_unimplemented; add esp, 4; mov eax, E_NOTIMPL; ret 4 * args
internal int rd_vtables(void) {
    uint8_t *code = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!code) return 0;
    uint8_t *p = code;
    for (int i = 0; i < RDI_N; i++)
        for (int s = 0; s < k_rd_slots[i]; s++) {
            g_rd_vt[i][s] = p;
            p[0] = 0x68; *(uint32_t *)(p + 1) = (uint32_t)(i << 8 | s);
            p[5] = 0xE8; *(int32_t *)(p + 6) = (int32_t)((uintptr_t)rd_unimplemented - (uintptr_t)(p + 10));
            p[10] = 0x83, p[11] = 0xC4, p[12] = 0x04;
            p[13] = 0xB8; *(uint32_t *)(p + 14) = (uint32_t)E_NOTIMPL;
            p[18] = 0xC2; *(uint16_t *)(p + 19) = (uint16_t)(4 * k_rd_args[i][s]);
            p += 24;
        }
    FlushInstructionCache(GetCurrentProcess(), code, 4096);
    return 1;
}

// ---- pixel formats ----
enum { RF_NONE, RF_XRGB8888, RF_ARGB8888, RF_RGB565, RF_XRGB1555, RF_ARGB1555, RF_ARGB4444, RF_Z16, RF_Z24, RF_Z32 };
internal int rf_bytes(int f) { return f == RF_XRGB8888 || f == RF_ARGB8888 || f == RF_Z24 || f == RF_Z32 ? 4 : 2; }
internal void rf_pixel_format(int f, DDPIXELFORMAT *pf) {
    memset(pf, 0, sizeof *pf);
    pf->dwSize = sizeof *pf;
    switch (f) {
    case RF_Z16: case RF_Z24: case RF_Z32:
        pf->dwFlags = DDPF_ZBUFFER;
        pf->dwZBufferBitDepth = f == RF_Z16 ? 16 : 32;
        pf->dwZBitMask = f == RF_Z16 ? 0xFFFF : f == RF_Z24 ? 0xFFFFFF : 0xFFFFFFFF;
        if (f == RF_Z24) pf->dwZBufferBitDepth = 32;   // D24X8: 24 bits of depth in 32
        return;
    }
    pf->dwFlags = DDPF_RGB;
    pf->dwRGBBitCount = (DWORD)rf_bytes(f) * 8;
    static const DWORD masks[][4] = {   // r g b a by format
        { 0, 0, 0, 0 }, { 0xFF0000, 0xFF00, 0xFF, 0 }, { 0xFF0000, 0xFF00, 0xFF, 0xFF000000 }, { 0xF800, 0x7E0, 0x1F, 0 },
        { 0x7C00, 0x3E0, 0x1F, 0 }, { 0x7C00, 0x3E0, 0x1F, 0x8000 }, { 0xF00, 0xF0, 0xF, 0xF000 } };
    pf->dwRBitMask = masks[f][0]; pf->dwGBitMask = masks[f][1]; pf->dwBBitMask = masks[f][2];
    pf->dwRGBAlphaBitMask = masks[f][3];
    if (masks[f][3]) pf->dwFlags |= DDPF_ALPHAPIXELS;
}
// the bits a colour key compares: colour only (DirectDraw ignores the unused X bits - the cursor's key 0xFF is blue)
internal uint32_t rf_key_mask(int f) {
    return f == RF_XRGB8888 || f == RF_ARGB8888 ? 0xFFFFFFu : f == RF_RGB565 ? 0xFFFFu : f == RF_ARGB4444 ? 0x0FFFu : 0x7FFFu;
}
internal int rf_from(const DDPIXELFORMAT *pf) {
    if (pf->dwFlags & DDPF_ZBUFFER)
        return pf->dwZBufferBitDepth == 16 ? RF_Z16 : pf->dwZBitMask == 0xFFFFFF ? RF_Z24 : RF_Z32;
    if (!(pf->dwFlags & DDPF_RGB)) return RF_NONE;
    int alpha = (pf->dwFlags & DDPF_ALPHAPIXELS) && pf->dwRGBAlphaBitMask;
    if (pf->dwRGBBitCount == 32 && pf->dwRBitMask == 0xFF0000) return alpha ? RF_ARGB8888 : RF_XRGB8888;
    if (pf->dwRGBBitCount == 16 && pf->dwRBitMask == 0xF800) return RF_RGB565;
    if (pf->dwRGBBitCount == 16 && pf->dwRBitMask == 0x7C00) return alpha ? RF_ARGB1555 : RF_XRGB1555;
    if (pf->dwRGBBitCount == 16 && pf->dwRBitMask == 0xF00) return RF_ARGB4444;
    return RF_NONE;
}
// a row of n pixels in format f -> B8G8R8A8 (alpha 0 where a colour key matches the raw value)
internal void rf_to_bgra(int f, const void *src, uint32_t *dst, int n, int keyed, uint32_t key) {
    const uint32_t *s32 = src;
    const uint16_t *s16 = src;
    for (int i = 0; i < n; i++) {
        uint32_t v, raw;
        switch (f) {
        case RF_ARGB8888: raw = s32[i]; v = raw; break;
        case RF_XRGB8888: raw = s32[i]; v = raw | 0xFF000000u; break;
        case RF_RGB565: raw = s16[i];
            v = 0xFF000000u | ((raw >> 11 & 31) * 255 / 31) << 16 | ((raw >> 5 & 63) * 255 / 63) << 8 | (raw & 31) * 255 / 31;
            break;
        case RF_XRGB1555: case RF_ARGB1555: raw = s16[i];
            v = (f == RF_XRGB1555 || raw & 0x8000 ? 0xFF000000u : 0) | ((raw >> 10 & 31) * 255 / 31) << 16 |
                ((raw >> 5 & 31) * 255 / 31) << 8 | (raw & 31) * 255 / 31;
            break;
        case RF_ARGB4444: raw = s16[i];
            v = (raw >> 12 & 15) * 17u << 24 | (raw >> 8 & 15) * 17u << 16 | (raw >> 4 & 15) * 17u << 8 | (raw & 15) * 17u;
            break;
        default: raw = 0; v = 0;
        }
        if (keyed && ((raw ^ key) & rf_key_mask(f)) == 0) v &= 0x00FFFFFFu;
        dst[i] = v;
    }
}
// B8G8R8A8 -> format f (the render target read back into a 16-bit surface; 32-bit formats as they are)
internal void rf_from_bgra(int f, const uint32_t *src, void *dst, int n) {
    if (rf_bytes(f) == 4) { memcpy(dst, src, (size_t)n * 4); return; }
    uint16_t *d = dst;
    for (int i = 0; i < n; i++) {
        uint32_t v = src[i], r = v >> 16 & 255, g = v >> 8 & 255, b = v & 255;
        d[i] = f == RF_RGB565 ? (uint16_t)((r >> 3) << 11 | (g >> 2) << 5 | b >> 3)
                              : (uint16_t)(0x8000 | (r >> 3) << 10 | (g >> 3) << 5 | b >> 3);
    }
}

// ---- surfaces ----
enum { RK_PRIMARY, RK_TARGET, RK_ZBUF, RK_PLAIN, RK_TEXTURE };
typedef struct rd_surface rd_surface;
struct rd_surface {
    void **vt;
    LONG refs;
    int kind, fmt, level, levels;   // levels: the chain's length (the top's)
    DDSURFACEDESC2 desc;            // as the game sees it
    uint8_t *mem;                   // CPU pixels (desc.lPitch); a target's: what the last lock read
    rd_surface *top, *next, *zbuf;  // mip chain (the top owns the GPU texture), attached depth buffer
    int dirty;                      // CPU pixels newer than the GPU texture (on the top for a chain)
    int keyed;                      // the GPU copy has alpha 0 where the colour key matches
    DDCOLORKEY key;
    int has_key;
    int locked, lock_ro;
    RECT lock_rect;
    ID3D11Texture2D *tex;
    ID3D11ShaderResourceView *srv;
    ID3D11RenderTargetView *rtv;
    ID3D11DepthStencilView *dsv;
};
internal int rd_is_surface(const void *p) { return p && *(void *const *)p == (void *)g_rd_vt[RDI_SURF]; }

internal ULONG __stdcall rd_sf_release(rd_surface *s);
internal void rd_surface_free(rd_surface *s) {
    D3D_RELEASE(s->srv); D3D_RELEASE(s->rtv); D3D_RELEASE(s->dsv); D3D_RELEASE(s->tex);
    free(s->mem);
    if (s->zbuf) rd_sf_release(s->zbuf);
    free(s);
}

internal uint8_t *rd_mem(rd_surface *s) {
    if (!s->mem) s->mem = calloc((size_t)s->desc.lPitch, s->desc.dwHeight ? s->desc.dwHeight : 1);
    return s->mem;
}

internal HRESULT rd_fail(const char *what, HRESULT hr) {
    log_locked("# render: %s failed (hr %08lx)\n", what, (unsigned long)hr);
    return hr;
}

// the GPU side of a target / depth buffer (made with the surface)
internal HRESULT rd_gpu_target(rd_surface *s) {
    D3D11_TEXTURE2D_DESC td = { 0 };
    td.Width = s->desc.dwWidth; td.Height = s->desc.dwHeight; td.MipLevels = 1; td.ArraySize = 1;
    td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_DEFAULT;
    HRESULT hr;
    if (s->kind == RK_ZBUF) {
        td.Format = DXGI_FORMAT_D32_FLOAT; td.BindFlags = D3D11_BIND_DEPTH_STENCIL;
        hr = ID3D11Device_CreateTexture2D(g_d3d.dev, &td, NULL, &s->tex);
        if (SUCCEEDED(hr)) hr = ID3D11Device_CreateDepthStencilView(g_d3d.dev, (ID3D11Resource *)s->tex, NULL, &s->dsv);
        if (SUCCEEDED(hr)) ID3D11DeviceContext_ClearDepthStencilView(g_d3d.ctx, s->dsv, D3D11_CLEAR_DEPTH, 1.0f, 0);
        return SUCCEEDED(hr) ? hr : rd_fail("depth buffer", hr);
    }
    td.Format = DXGI_FORMAT_B8G8R8A8_UNORM; td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    hr = ID3D11Device_CreateTexture2D(g_d3d.dev, &td, NULL, &s->tex);
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateRenderTargetView(g_d3d.dev, (ID3D11Resource *)s->tex, NULL, &s->rtv);
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)s->tex, NULL, &s->srv);
    if (SUCCEEDED(hr)) {
        static const float black[4] = { 0, 0, 0, 1 };
        ID3D11DeviceContext_ClearRenderTargetView(g_d3d.ctx, s->rtv, black);
    }
    return SUCCEEDED(hr) ? hr : rd_fail("render target", hr);
}

// a CPU surface's (a chain's) GPU texture, uploaded when its pixels changed; keyed: alpha 0 at the colour key
internal ID3D11ShaderResourceView *rd_gpu_texture(rd_surface *s, int keyed) {
    if (s->kind == RK_TARGET) return s->srv;
    rd_surface *t = s->top ? s->top : s;
    keyed = keyed && t->has_key;
    if (t->srv && !t->dirty && t->keyed == keyed) return t->srv;
    if (!t->tex) {
        D3D11_TEXTURE2D_DESC td = { 0 };
        td.Width = t->desc.dwWidth; td.Height = t->desc.dwHeight; td.MipLevels = (UINT)t->levels; td.ArraySize = 1;
        td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_DEFAULT; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        HRESULT hr = ID3D11Device_CreateTexture2D(g_d3d.dev, &td, NULL, &t->tex);
        if (SUCCEEDED(hr)) hr = ID3D11Device_CreateShaderResourceView(g_d3d.dev, (ID3D11Resource *)t->tex, NULL, &t->srv);
        if (FAILED(hr)) { rd_fail("texture", hr); return NULL; }
    }
    static uint32_t *row;
    static size_t row_cap;
    for (rd_surface *l = t; l; l = l->next) {
        int w = (int)l->desc.dwWidth, h = (int)l->desc.dwHeight;
        size_t need = (size_t)w * h;
        if (need > row_cap) { free(row); row = malloc(need * 4); row_cap = row ? need : 0; }
        if (!row) return NULL;
        uint8_t *m = rd_mem(l);
        for (int y = 0; y < h; y++)
            rf_to_bgra(l->fmt, m + (size_t)y * l->desc.lPitch, row + (size_t)y * w, w, keyed, t->key.dwColorSpaceLowValue);
        ID3D11DeviceContext_UpdateSubresource(g_d3d.ctx, (ID3D11Resource *)t->tex, (UINT)l->level, NULL, row, (UINT)w * 4, 0);
    }
    t->dirty = 0;
    t->keyed = keyed;
    return t->srv;
}

// the render target's pixels in r into its memory (a staging copy of that rect)
internal int rd_read_target(rd_surface *s, const RECT *r) {
    uint8_t *m = rd_mem(s);
    if (!m || !s->tex) return 0;
    D3D11_TEXTURE2D_DESC td = { 0 };
    td.Width = (UINT)(r->right - r->left); td.Height = (UINT)(r->bottom - r->top); td.MipLevels = 1; td.ArraySize = 1;
    td.SampleDesc.Count = 1; td.Format = DXGI_FORMAT_B8G8R8A8_UNORM; td.Usage = D3D11_USAGE_STAGING;
    td.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Texture2D *st = NULL;
    if (FAILED(ID3D11Device_CreateTexture2D(g_d3d.dev, &td, NULL, &st))) return 0;
    D3D11_BOX box = { (UINT)r->left, (UINT)r->top, 0, (UINT)r->right, (UINT)r->bottom, 1 };
    ID3D11DeviceContext_CopySubresourceRegion(g_d3d.ctx, (ID3D11Resource *)st, 0, 0, 0, 0, (ID3D11Resource *)s->tex, 0, &box);
    D3D11_MAPPED_SUBRESOURCE ms;
    int ok = SUCCEEDED(ID3D11DeviceContext_Map(g_d3d.ctx, (ID3D11Resource *)st, 0, D3D11_MAP_READ, 0, &ms));
    if (ok) {
        int bpp = rf_bytes(s->fmt);
        for (int y = 0; y < (int)td.Height; y++)
            rf_from_bgra(s->fmt, (const uint32_t *)((uint8_t *)ms.pData + (size_t)y * ms.RowPitch),
                         m + (size_t)(r->top + y) * s->desc.lPitch + (size_t)r->left * bpp, (int)td.Width);
        ID3D11DeviceContext_Unmap(g_d3d.ctx, (ID3D11Resource *)st, 0);
    }
    st->lpVtbl->Release(st);
    return ok;
}
// its memory in r back to the GPU (after a lock that wrote)
internal void rd_write_target(rd_surface *s, const RECT *r) {
    int w = r->right - r->left, h = r->bottom - r->top;
    if (w <= 0 || h <= 0 || !s->tex || !s->mem) return;
    uint32_t *buf = malloc((size_t)w * h * 4);
    if (!buf) return;
    int bpp = rf_bytes(s->fmt);
    for (int y = 0; y < h; y++)
        rf_to_bgra(s->fmt, s->mem + (size_t)(r->top + y) * s->desc.lPitch + (size_t)r->left * bpp, buf + (size_t)y * w, w, 0, 0);
    D3D11_BOX box = { (UINT)r->left, (UINT)r->top, 0, (UINT)r->right, (UINT)r->bottom, 1 };
    ID3D11DeviceContext_UpdateSubresource(g_d3d.ctx, (ID3D11Resource *)s->tex, 0, &box, buf, (UINT)w * 4, 0);
    free(buf);
}

// ---- the 2D pipeline: a textured or filled rectangle into a render target ----
internal struct {
    int ready;
    ID3D11VertexShader *vs;
    ID3D11PixelShader *ps;
    ID3D11Buffer *cb;
    ID3D11BlendState *opaque;
    ID3D11BlendState *no_colour;            // a depth rect writes depth only
    ID3D11DepthStencilState *no_depth, *depth_set;
    ID3D11RasterizerState *raster;
} g_rd2;
internal const char k_rd2_shader[] =
    "cbuffer c : register(b0) { float4 dst; float4 uv; float4 col; float4 mode; };\n"   // dst: x0 y0 x1 y1 in NDC
    "Texture2D t : register(t0); SamplerState s : register(s0);\n"
    "struct V { float4 p : SV_Position; float2 uv : TEXCOORD0; };\n"
    "V vs(uint id : SV_VertexID) { V o; float2 k = float2(id & 1, id >> 1);"
    " o.p = float4(lerp(dst.xy, dst.zw, k), mode.z, 1); o.uv = lerp(uv.xy, uv.zw, k); return o; }\n"
    "float4 ps(V i) : SV_Target { if (mode.x > 0.5) return col;"   // fill
    " float4 c = t.Sample(s, i.uv); if (mode.y > 0.5 && c.a < 0.5) discard; return float4(c.rgb, 1); }\n";
internal int rd2_init(void) {
    if (g_rd2.ready) return g_rd2.ready > 0;
    g_rd2.ready = -1;
    ID3DBlob *code = d3d_compile(k_rd2_shader, sizeof k_rd2_shader - 1, "vs", "vs_4_0");
    HRESULT hr = code ? ID3D11Device_CreateVertexShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_rd2.vs) : E_FAIL;
    D3D_RELEASE(code);
    code = SUCCEEDED(hr) ? d3d_compile(k_rd2_shader, sizeof k_rd2_shader - 1, "ps", "ps_4_0") : NULL;
    hr = code ? ID3D11Device_CreatePixelShader(g_d3d.dev, ID3D10Blob_GetBufferPointer(code), ID3D10Blob_GetBufferSize(code), NULL, &g_rd2.ps) : E_FAIL;
    D3D_RELEASE(code);
    D3D11_BUFFER_DESC bd = { 64, D3D11_USAGE_DEFAULT, D3D11_BIND_CONSTANT_BUFFER, 0, 0, 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBuffer(g_d3d.dev, &bd, NULL, &g_rd2.cb);
    D3D11_BLEND_DESC bl = { 0 };
    bl.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBlendState(g_d3d.dev, &bl, &g_rd2.opaque);
    bl.RenderTarget[0].RenderTargetWriteMask = 0;
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateBlendState(g_d3d.dev, &bl, &g_rd2.no_colour);
    D3D11_DEPTH_STENCIL_DESC ds = { 0 };
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateDepthStencilState(g_d3d.dev, &ds, &g_rd2.no_depth);
    ds.DepthEnable = TRUE; ds.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL; ds.DepthFunc = D3D11_COMPARISON_ALWAYS;
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateDepthStencilState(g_d3d.dev, &ds, &g_rd2.depth_set);
    D3D11_RASTERIZER_DESC rs = { 0 };
    rs.FillMode = D3D11_FILL_SOLID; rs.CullMode = D3D11_CULL_NONE; rs.ScissorEnable = TRUE;
    if (SUCCEEDED(hr)) hr = ID3D11Device_CreateRasterizerState(g_d3d.dev, &rs, &g_rd2.raster);
    if (FAILED(hr)) return rd_fail("the 2D pipeline", hr), 0;
    g_rd2.ready = 1;
    return 1;
}
internal void rd3_dirty(void);   // render_3d.c: the 2D draw changed the pipeline's state
typedef struct { float dst[4], uv[4], col[4], mode[4]; } rd2_consts;   // mode: fill, keyed, depth
internal void rd2_draw(rd_surface *dst, const rd2_consts *c, ID3D11ShaderResourceView *src, int depth);
// src (NULL: fill with col) rect sr into dst's rect dr
internal void rd2_rect(rd_surface *dst, const RECT *dr, ID3D11ShaderResourceView *src, int sw, int sh, const RECT *sr,
                       const float col[4], int keyed) {
    if (!dst->rtv || !rd2_init()) return;
    float W = (float)dst->desc.dwWidth, H = (float)dst->desc.dwHeight;
    rd2_consts c = {
        { dr->left / W * 2 - 1, 1 - dr->top / H * 2, dr->right / W * 2 - 1, 1 - dr->bottom / H * 2 },
        { 0, 0, 1, 1 }, { 0, 0, 0, 1 }, { src ? 0.0f : 1.0f, keyed ? 1.0f : 0.0f, 0, 0 } };
    if (src && sr) {
        c.uv[0] = (float)sr->left / (float)sw; c.uv[1] = (float)sr->top / (float)sh;
        c.uv[2] = (float)sr->right / (float)sw; c.uv[3] = (float)sr->bottom / (float)sh;
    }
    if (col) memcpy(c.col, col, sizeof c.col);
    rd2_draw(dst, &c, src, 0);
}
// z into the rect r of dst's depth buffer (Clear with rects)
internal void rd2_depth_rect(rd_surface *dst, const RECT *r, float z) {
    if (!dst->rtv || !dst->zbuf || !dst->zbuf->dsv || !rd2_init()) return;
    float W = (float)dst->desc.dwWidth, H = (float)dst->desc.dwHeight;
    rd2_consts c = { { r->left / W * 2 - 1, 1 - r->top / H * 2, r->right / W * 2 - 1, 1 - r->bottom / H * 2 },
                     { 0, 0, 1, 1 }, { 0, 0, 0, 1 }, { 1, 0, z, 0 } };
    rd2_draw(dst, &c, NULL, 1);
}
internal void rd2_draw(rd_surface *dst, const rd2_consts *c, ID3D11ShaderResourceView *src, int depth) {
    float W = (float)dst->desc.dwWidth, H = (float)dst->desc.dwHeight;
    ID3D11DeviceContext *x = g_d3d.ctx;
    ID3D11DeviceContext_UpdateSubresource(x, (ID3D11Resource *)g_rd2.cb, 0, NULL, c, 0, 0);
    D3D11_VIEWPORT vp = { 0, 0, W, H, 0, 1 };
    D3D11_RECT sc = { 0, 0, (LONG)W, (LONG)H };
    ID3D11DeviceContext_OMSetRenderTargets(x, 1, &dst->rtv, depth ? dst->zbuf->dsv : NULL);
    ID3D11DeviceContext_OMSetBlendState(x, depth ? g_rd2.no_colour : g_rd2.opaque, NULL, 0xFFFFFFFF);
    ID3D11DeviceContext_OMSetDepthStencilState(x, depth ? g_rd2.depth_set : g_rd2.no_depth, 0);
    ID3D11DeviceContext_RSSetState(x, g_rd2.raster);
    ID3D11DeviceContext_RSSetViewports(x, 1, &vp);
    ID3D11DeviceContext_RSSetScissorRects(x, 1, &sc);
    ID3D11DeviceContext_IASetInputLayout(x, NULL);
    ID3D11DeviceContext_IASetPrimitiveTopology(x, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    ID3D11DeviceContext_VSSetShader(x, g_rd2.vs, NULL, 0);
    ID3D11DeviceContext_VSSetConstantBuffers(x, 0, 1, &g_rd2.cb);
    ID3D11DeviceContext_PSSetShader(x, g_rd2.ps, NULL, 0);
    ID3D11DeviceContext_PSSetConstantBuffers(x, 0, 1, &g_rd2.cb);
    ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &src);
    ID3D11DeviceContext_PSSetSamplers(x, 0, 1, &g_d3d.point);
    ID3D11DeviceContext_Draw(x, 4, 0);
    ID3D11ShaderResourceView *none = NULL;
    ID3D11DeviceContext_PSSetShaderResources(x, 0, 1, &none);
    rd3_dirty();
}

// ---- Surface7 ----
internal HRESULT __stdcall rd_sf_qi(rd_surface *s, const GUID *iid, void **out) {
    if (IsEqualGUID(iid, &IID_IDirectDrawSurface7) || IsEqualGUID(iid, &IID_IUnknown)) {
        InterlockedIncrement(&s->refs);
        *out = s;
        return S_OK;
    }
    log_locked("# render: Surface7.QueryInterface %08lx not implemented\n", (unsigned long)iid->Data1);
    *out = NULL;
    return E_NOINTERFACE;
}
internal ULONG __stdcall rd_sf_addref(rd_surface *s) { return (ULONG)InterlockedIncrement(&s->refs); }
internal ULONG __stdcall rd_sf_release(rd_surface *s) {
    rd_surface *t = s->top ? s->top : s;   // a mip level lives as long as its chain
    if (s != t) { InterlockedDecrement(&s->refs); return 1; }
    LONG n = InterlockedDecrement(&s->refs);
    if (n > 0) return (ULONG)n;
    RD_LOCK();
    for (rd_surface *l = s->next, *nx; l; l = nx) { nx = l->next; free(l->mem); free(l); }
    rd_surface_free(s);
    RD_UNLOCK();
    return 0;
}
internal HRESULT __stdcall rd_sf_add_attached(rd_surface *s, rd_surface *z) {
    if (!rd_is_surface(z) || z->kind != RK_ZBUF) return DDERR_CANNOTATTACHSURFACE;
    InterlockedIncrement(&z->refs);
    if (s->zbuf) rd_sf_release(s->zbuf);
    s->zbuf = z;
    return DD_OK;
}
internal HRESULT __stdcall rd_sf_get_attached(rd_surface *s, DDSCAPS2 *caps, rd_surface **out) {
    rd_surface *r = caps->dwCaps & DDSCAPS_MIPMAP ? s->next : caps->dwCaps & DDSCAPS_ZBUFFER ? s->zbuf : NULL;
    *out = r;
    if (!r) return DDERR_NOTFOUND;
    InterlockedIncrement(&r->refs);
    return DD_OK;
}
internal HRESULT __stdcall rd_sf_get_caps(rd_surface *s, DDSCAPS2 *caps) { *caps = s->desc.ddsCaps; return DD_OK; }
internal HRESULT __stdcall rd_sf_get_pixel_format(rd_surface *s, DDPIXELFORMAT *pf) { *pf = s->desc.ddpfPixelFormat; return DD_OK; }
internal HRESULT __stdcall rd_sf_get_desc(rd_surface *s, DDSURFACEDESC2 *d) {
    DWORD size = d->dwSize;
    *d = s->desc;
    d->dwSize = size;
    d->lpSurface = NULL;
    return DD_OK;
}
internal HRESULT __stdcall rd_sf_ok(rd_surface *s) { (void)s; return DD_OK; }   // IsLost, Restore
internal HRESULT __stdcall rd_sf_get_colorkey(rd_surface *s, DWORD flags, DDCOLORKEY *k) {
    if (!(flags & DDCKEY_SRCBLT) || !s->has_key) return DDERR_NOCOLORKEY;
    *k = s->key;
    return DD_OK;
}
internal HRESULT __stdcall rd_sf_set_colorkey(rd_surface *s, DWORD flags, DDCOLORKEY *k) {
    if (!(flags & DDCKEY_SRCBLT)) return DD_OK;   // only the source key is used by the game
    rd_surface *t = s->top ? s->top : s;
    t->has_key = k != NULL;
    if (k) t->key = *k;
    t->dirty = 1;
    return DD_OK;
}
internal HRESULT __stdcall rd_sf_lock(rd_surface *s, RECT *rect, DDSURFACEDESC2 *d, DWORD flags, HANDLE ev) {
    (void)ev;
    RECT all = { 0, 0, (LONG)s->desc.dwWidth, (LONG)s->desc.dwHeight }, r = rect ? *rect : all;
    if (r.left < 0) r.left = 0;
    if (r.top < 0) r.top = 0;
    if (r.right > all.right) r.right = all.right;
    if (r.bottom > all.bottom) r.bottom = all.bottom;
    RD_LOCK();
    uint8_t *m = rd_mem(s);
    if (m && s->kind == RK_TARGET) rd_read_target(s, &r);
    RD_UNLOCK();
    if (!m) return DDERR_OUTOFMEMORY;
    s->locked = 1;
    s->lock_ro = (flags & DDLOCK_READONLY) != 0;
    s->lock_rect = r;
    DWORD size = d->dwSize;
    *d = s->desc;
    d->dwSize = size;
    d->dwFlags |= DDSD_LPSURFACE | DDSD_PITCH;
    d->lpSurface = m + (size_t)r.top * s->desc.lPitch + (size_t)r.left * rf_bytes(s->fmt);
    return DD_OK;
}
internal HRESULT __stdcall rd_sf_unlock(rd_surface *s, RECT *rect) {
    (void)rect;
    if (!s->locked) return DDERR_NOTLOCKED;
    s->locked = 0;
    if (s->lock_ro) return DD_OK;
    RD_LOCK();
    if (s->kind == RK_TARGET) rd_write_target(s, &s->lock_rect);
    else (s->top ? s->top : s)->dirty = 1;
    RD_UNLOCK();
    return DD_OK;
}
internal float rd_unit(uint32_t v, uint32_t mask) {
    if (!mask) return 1.0f;
    unsigned shift = (unsigned)__builtin_ctz(mask);
    return (float)((v & mask) >> shift) / (float)(mask >> shift);
}
// a colour fill or a copy (stretched, keyed) - CPU to CPU in memory, anything into a target on the GPU
internal HRESULT __stdcall rd_sf_blt(rd_surface *d, RECT *dr0, rd_surface *s, RECT *sr0, DWORD flags, DDBLTFX *fx) {
    RECT dr = dr0 ? *dr0 : (RECT){ 0, 0, (LONG)d->desc.dwWidth, (LONG)d->desc.dwHeight };
    if (dr.right <= dr.left || dr.bottom <= dr.top) return DD_OK;
    RD_LOCK();
    HRESULT r = DD_OK;
    if (flags & DDBLT_COLORFILL) {
        uint32_t c = fx ? fx->dwFillColor : 0;
        const DDPIXELFORMAT *pf = &d->desc.ddpfPixelFormat;
        if (d->kind == RK_TARGET) {
            float col[4] = { rd_unit(c, pf->dwRBitMask), rd_unit(c, pf->dwGBitMask), rd_unit(c, pf->dwBBitMask), 1 };
            rd2_rect(d, &dr, NULL, 0, 0, NULL, col, 0);
        } else if (d->kind != RK_ZBUF) {
            uint8_t *m = rd_mem(d);
            int bpp = rf_bytes(d->fmt);
            for (LONG y = dr.top; m && y < dr.bottom && y < (LONG)d->desc.dwHeight; y++)
                for (LONG x = dr.left; x < dr.right && x < (LONG)d->desc.dwWidth; x++) {
                    uint8_t *p = m + (size_t)y * d->desc.lPitch + (size_t)x * bpp;
                    if (bpp == 4) *(uint32_t *)p = c; else *(uint16_t *)p = (uint16_t)c;
                }
            (d->top ? d->top : d)->dirty = 1;
        }
        RD_UNLOCK();
        return DD_OK;
    }
    if (!rd_is_surface(s)) { RD_UNLOCK(); return DDERR_INVALIDPARAMS; }
    RECT sr = sr0 ? *sr0 : (RECT){ 0, 0, (LONG)s->desc.dwWidth, (LONG)s->desc.dwHeight };
    int keyed = (flags & DDBLT_KEYSRC) != 0;
    if (d->kind == RK_TARGET) {
        ID3D11ShaderResourceView *v = rd_gpu_texture(s, keyed);
        if (v) rd2_rect(d, &dr, v, (int)s->desc.dwWidth, (int)s->desc.dwHeight, &sr, NULL, keyed);
    } else if (d->kind != RK_ZBUF && d->kind != RK_PRIMARY) {
        // CPU: nearest neighbour, a target source read back first
        if (s->kind == RK_TARGET) rd_read_target(s, &sr);
        uint8_t *dm = rd_mem(d), *sm = rd_mem(s);
        int db = rf_bytes(d->fmt), sb = rf_bytes(s->fmt), same = d->fmt == s->fmt;
        int dw = dr.right - dr.left, dh = dr.bottom - dr.top, sw = sr.right - sr.left, sh = sr.bottom - sr.top;
        uint32_t key = s->has_key ? s->key.dwColorSpaceLowValue : 0, px;
        for (int y = 0; dm && sm && y < dh; y++) {
            int dy = dr.top + y, sy = sr.top + y * sh / dh;
            if (dy < 0 || dy >= (int)d->desc.dwHeight || sy < 0 || sy >= (int)s->desc.dwHeight) continue;
            for (int x = 0; x < dw; x++) {
                int dx = dr.left + x, sx = sr.left + x * sw / dw;
                if (dx < 0 || dx >= (int)d->desc.dwWidth || sx < 0 || sx >= (int)s->desc.dwWidth) continue;
                const uint8_t *sp = sm + (size_t)sy * s->desc.lPitch + (size_t)sx * sb;
                uint8_t *dp = dm + (size_t)dy * d->desc.lPitch + (size_t)dx * db;
                uint32_t raw = sb == 4 ? *(const uint32_t *)sp : *(const uint16_t *)sp;
                if (keyed && s->has_key && ((raw ^ key) & rf_key_mask(s->fmt)) == 0) continue;
                if (same) px = raw;
                else { uint32_t bgra; rf_to_bgra(s->fmt, sp, &bgra, 1, 0, 0); rf_from_bgra(d->fmt, &bgra, &px, 1); }
                if (db == 4) *(uint32_t *)dp = px; else *(uint16_t *)dp = (uint16_t)px;
            }
        }
        (d->top ? d->top : d)->dirty = 1;
    }
    RD_UNLOCK();
    return r;
}
internal HRESULT __stdcall rd_sf_bltfast(rd_surface *d, DWORD x, DWORD y, rd_surface *s, RECT *sr0, DWORD trans) {
    if (!rd_is_surface(s)) return DDERR_INVALIDPARAMS;
    RECT sr = sr0 ? *sr0 : (RECT){ 0, 0, (LONG)s->desc.dwWidth, (LONG)s->desc.dwHeight };
    RECT dr = { (LONG)x, (LONG)y, (LONG)x + sr.right - sr.left, (LONG)y + sr.bottom - sr.top };
    return rd_sf_blt(d, &dr, s, &sr, trans & DDBLTFAST_SRCCOLORKEY ? DDBLT_KEYSRC : 0, NULL);
}
internal HRESULT __stdcall rd_sf_flip(rd_surface *s, rd_surface *target, DWORD flags) { (void)s; (void)target; (void)flags; return DD_OK; }

// a surface of the game's description: its kind, format and pitch; a mipmapped texture with its chain
internal rd_surface *rd_surface_new(const DDSURFACEDESC2 *d, int fmt, int kind, int w, int h) {
    rd_surface *s = calloc(1, sizeof *s);
    if (!s) return NULL;
    s->vt = g_rd_vt[RDI_SURF];
    s->refs = 1;
    s->kind = kind;
    s->fmt = fmt;
    s->levels = 1;
    s->desc = *d;
    s->desc.dwSize = sizeof s->desc;
    s->desc.dwFlags |= DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_CAPS;
    s->desc.dwWidth = (DWORD)w;
    s->desc.dwHeight = (DWORD)h;
    s->desc.lPitch = (LONG)((w * rf_bytes(fmt) + 3) & ~3);
    rf_pixel_format(fmt, &s->desc.ddpfPixelFormat);
    s->desc.lpSurface = NULL;
    DWORD caps = s->desc.ddsCaps.dwCaps;
    if (!(caps & DDSCAPS_SYSTEMMEMORY)) caps |= DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM;
    s->desc.ddsCaps.dwCaps = caps;
    return s;
}
internal HRESULT rd_create_surface(DDSURFACEDESC2 *d, rd_surface **out) {
    *out = NULL;
    DWORD caps = d->ddsCaps.dwCaps;
    int kind = caps & DDSCAPS_PRIMARYSURFACE ? RK_PRIMARY : caps & DDSCAPS_ZBUFFER ? RK_ZBUF
             : caps & DDSCAPS_3DDEVICE ? RK_TARGET : caps & DDSCAPS_TEXTURE ? RK_TEXTURE : RK_PLAIN;
    int w = d->dwFlags & DDSD_WIDTH ? (int)d->dwWidth : 0, h = d->dwFlags & DDSD_HEIGHT ? (int)d->dwHeight : 0;
    if (kind == RK_PRIMARY) { w = GetSystemMetrics(SM_CXSCREEN); h = GetSystemMetrics(SM_CYSCREEN); }
    int fmt = d->dwFlags & DDSD_PIXELFORMAT ? rf_from(&d->ddpfPixelFormat) : kind == RK_ZBUF ? RF_Z32 : RF_XRGB8888;
    if (fmt == RF_NONE || w <= 0 || h <= 0) {
        log_locked("# render: CreateSurface caps %08lx %lux%lu, pixel format flags %08lx %lu bits: not implemented\n",
                   (unsigned long)caps, (unsigned long)d->dwWidth, (unsigned long)d->dwHeight,
                   (unsigned long)d->ddpfPixelFormat.dwFlags, (unsigned long)d->ddpfPixelFormat.dwRGBBitCount);
        return DDERR_INVALIDPIXELFORMAT;
    }
    rd_surface *s = rd_surface_new(d, fmt, kind, w, h);
    if (!s) return DDERR_OUTOFMEMORY;
    if (d->dwFlags & DDSD_CKSRCBLT) { s->has_key = 1; s->key = d->ddckCKSrcBlt; }
    if ((kind == RK_TARGET || kind == RK_ZBUF) && FAILED(rd_gpu_target(s))) { rd_surface_free(s); return DDERR_GENERIC; }
    if (kind == RK_TEXTURE && (caps & DDSCAPS_MIPMAP)) {
        int n = d->dwFlags & DDSD_MIPMAPCOUNT ? (int)d->dwMipMapCount : 1;
        rd_surface *prev = s;
        for (int l = 1; l < n && (w > 1 || h > 1); l++) {
            w = w > 1 ? w / 2 : 1; h = h > 1 ? h / 2 : 1;
            DDSURFACEDESC2 ld = s->desc;
            ld.ddsCaps.dwCaps2 |= DDSCAPS2_MIPMAPSUBLEVEL;
            rd_surface *m = rd_surface_new(&ld, fmt, kind, w, h);
            if (!m) break;
            m->top = s;
            m->level = l;
            prev->next = m;
            prev = m;
            s->levels = l + 1;
        }
        s->desc.dwMipMapCount = (DWORD)s->levels;
    }
    *out = s;
    return DD_OK;
}

// ---- DirectDraw7 ----
typedef struct { void **vt; LONG refs; HWND hwnd; } rd_dd;
internal rd_dd g_rd_dd;   // one: the display DLLs make and release it again for each probe
internal void *rd_d3d_new(void);   // render_3d.c
internal HRESULT __stdcall rd_dd_qi(rd_dd *o, const GUID *iid, void **out) {
    if (IsEqualGUID(iid, &IID_IDirect3D7)) { *out = rd_d3d_new(); return *out ? S_OK : E_OUTOFMEMORY; }
    if (IsEqualGUID(iid, &IID_IDirectDraw7) || IsEqualGUID(iid, &IID_IUnknown)) {
        InterlockedIncrement(&o->refs);
        *out = o;
        return S_OK;
    }
    log_locked("# render: DirectDraw7.QueryInterface %08lx not implemented\n", (unsigned long)iid->Data1);
    *out = NULL;
    return E_NOINTERFACE;
}
internal ULONG __stdcall rd_dd_addref(rd_dd *o) { return (ULONG)InterlockedIncrement(&o->refs); }
internal ULONG __stdcall rd_dd_release(rd_dd *o) { LONG n = InterlockedDecrement(&o->refs); return n > 0 ? (ULONG)n : 0; }
internal HRESULT __stdcall rd_dd_create_surface(rd_dd *o, DDSURFACEDESC2 *d, rd_surface **out, IUnknown *outer) {
    (void)o; (void)outer;
    if (!d || !out) return DDERR_INVALIDPARAMS;
    RD_LOCK();
    HRESULT r = d3d_device() ? rd_create_surface(d, out) : DDERR_GENERIC;
    RD_UNLOCK();
    return r;
}
// the display's modes, each in 32 and in 16 bits (DirectDraw emulates 16-bit modes the display no longer lists, and
// the display DLL's Test32BitSupport looks for 800x600x16 - without it, it reads past its mode list)
internal void rd_mode_desc(DDSURFACEDESC2 *d, int w, int h, int bpp, int hz) {
    memset(d, 0, sizeof *d);
    d->dwSize = sizeof *d;
    d->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE;
    d->dwWidth = (DWORD)w; d->dwHeight = (DWORD)h;
    d->dwRefreshRate = (DWORD)hz;
    rf_pixel_format(bpp == 16 ? RF_RGB565 : RF_XRGB8888, &d->ddpfPixelFormat);
    d->lPitch = w * (bpp / 8);
}
internal HRESULT __stdcall rd_dd_enum_modes(rd_dd *o, DWORD flags, DDSURFACEDESC2 *filter, void *ctx,
                                            HRESULT (WINAPI *cb)(DDSURFACEDESC2 *, void *)) {
    (void)o; (void)flags; (void)filter;
    static struct { short w, h, bpp; } seen[256];
    int n = 0;
    DEVMODEA dm = { .dmSize = sizeof dm };
    for (DWORD i = 0; EnumDisplaySettingsA(NULL, i, &dm); i++) {
        if (dm.dmBitsPerPel != 32 && dm.dmBitsPerPel != 16) continue;
        for (int bpp = 32; bpp >= 16; bpp -= 16) {
            int dup = 0;
            for (int k = 0; k < n; k++) dup |= seen[k].w == (short)dm.dmPelsWidth && seen[k].h == (short)dm.dmPelsHeight && seen[k].bpp == bpp;
            if (dup || n == 256) continue;
            seen[n].w = (short)dm.dmPelsWidth; seen[n].h = (short)dm.dmPelsHeight; seen[n++].bpp = (short)bpp;
            DDSURFACEDESC2 d;
            rd_mode_desc(&d, (int)dm.dmPelsWidth, (int)dm.dmPelsHeight, bpp, 0);
            if (cb(&d, ctx) == DDENUMRET_CANCEL) return DD_OK;
        }
    }
    return DD_OK;
}
internal HRESULT __stdcall rd_dd_get_display_mode(rd_dd *o, DDSURFACEDESC2 *d) {
    (void)o;
    DEVMODEA dm = { .dmSize = sizeof dm };
    EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
    DWORD size = d->dwSize;
    rd_mode_desc(d, (int)dm.dmPelsWidth, (int)dm.dmPelsHeight, 32, (int)dm.dmDisplayFrequency);
    d->dwSize = size;
    return DD_OK;
}
#define RD_VIDMEM (512u << 20)
internal void rd_caps(DDCAPS *c) {
    DWORD size = c->dwSize;
    memset(c, 0, size);
    c->dwSize = size;
    c->dwCaps = DDCAPS_3D | DDCAPS_BLT | DDCAPS_BLTCOLORFILL | DDCAPS_BLTSTRETCH | DDCAPS_COLORKEY | DDCAPS_CANBLTSYSMEM;
    c->dwCaps2 = DDCAPS2_WIDESURFACES | DDCAPS2_CANRENDERWINDOWED | DDCAPS2_FLIPNOVSYNC;
    c->dwCKeyCaps = DDCKEYCAPS_SRCBLT;
    c->dwVidMemTotal = c->dwVidMemFree = RD_VIDMEM;
    c->ddsCaps.dwCaps = DDSCAPS_3DDEVICE | DDSCAPS_TEXTURE | DDSCAPS_ZBUFFER | DDSCAPS_MIPMAP | DDSCAPS_VIDEOMEMORY |
        DDSCAPS_LOCALVIDMEM | DDSCAPS_PRIMARYSURFACE | DDSCAPS_OFFSCREENPLAIN | DDSCAPS_FLIP | DDSCAPS_BACKBUFFER |
        DDSCAPS_COMPLEX | DDSCAPS_SYSTEMMEMORY;
}
internal HRESULT __stdcall rd_dd_get_caps(rd_dd *o, DDCAPS *drv, DDCAPS *hel) {
    (void)o;
    if (drv) rd_caps(drv);
    if (hel) rd_caps(hel);
    return DD_OK;
}
internal HRESULT __stdcall rd_dd_set_coop(rd_dd *o, HWND h, DWORD flags) { (void)flags; o->hwnd = h; return DD_OK; }
internal HRESULT __stdcall rd_dd_set_mode(rd_dd *o, DWORD w, DWORD h, DWORD bpp, DWORD hz, DWORD flags) {
    (void)o; (void)w; (void)h; (void)bpp; (void)hz; (void)flags;
    return DD_OK;   // windowed only: the desktop stays as it is
}
internal HRESULT __stdcall rd_dd_ok(rd_dd *o) { (void)o; return DD_OK; }   // RestoreDisplayMode, RestoreAllSurfaces, TestCooperativeLevel, FlipToGDISurface
internal HRESULT __stdcall rd_dd_wait_vb(rd_dd *o, DWORD flags, HANDLE ev) { (void)o; (void)flags; (void)ev; return DD_OK; }
internal HRESULT __stdcall rd_dd_vidmem(rd_dd *o, DDSCAPS2 *caps, DWORD *total, DWORD *free_) {
    (void)o; (void)caps;
    if (total) *total = RD_VIDMEM;
    if (free_) *free_ = RD_VIDMEM;
    return DD_OK;
}
internal HRESULT __stdcall rd_dd_monitor_freq(rd_dd *o, DWORD *hz) {
    (void)o;
    DEVMODEA dm = { .dmSize = sizeof dm };
    EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
    *hz = dm.dmDisplayFrequency;
    return DD_OK;
}
internal HRESULT __stdcall rd_dd_fourcc(rd_dd *o, DWORD *n, DWORD *codes) { (void)o; (void)codes; *n = 0; return DD_OK; }
internal HRESULT __stdcall rd_dd_identifier(rd_dd *o, DDDEVICEIDENTIFIER2 *id, DWORD flags) {
    (void)o; (void)flags;
    memset(id, 0, sizeof *id);
    lstrcpyA(id->szDriver, "ageofai.dll");
    lstrcpyA(id->szDescription, "Empire Earth: Age of AI renderer (Direct3D 11)");
    return DD_OK;
}

// the display DLLs' imports
typedef BOOL (WINAPI *dd_enum_cb)(GUID *, LPSTR, LPSTR, void *);
typedef BOOL (WINAPI *dd_enum_ex_cb)(GUID *, LPSTR, LPSTR, void *, HMONITOR);
internal HRESULT WINAPI rd_create_ex(GUID *guid, void **out, REFIID iid, IUnknown *outer) {
    (void)guid; (void)outer;
    if (!IsEqualGUID(iid, &IID_IDirectDraw7)) return DDERR_INVALIDPARAMS;
    InterlockedIncrement(&g_rd_dd.refs);
    *out = &g_rd_dd;
    return DD_OK;
}
internal HRESULT WINAPI rd_enumerate(dd_enum_cb cb, void *ctx) {
    cb(NULL, "Primary Display Driver", "display", ctx);
    return DD_OK;
}
internal HRESULT WINAPI rd_enumerate_ex(dd_enum_ex_cb cb, void *ctx, DWORD flags) {
    (void)flags;
    cb(NULL, "Primary Display Driver", "display", ctx, NULL);
    return DD_OK;
}

#include "render_3d.c"

// the mod's ShowScene hook, windowed: the target into the window (shot: read back for the pipe's "shot")
internal int render_present(IDirectDrawSurface7 *back, HWND hwnd) {
    rd_surface *s = (rd_surface *)back;
    if (!rd_is_surface(s) || !s->srv) return 0;
    RD_LOCK();
    if (g_shot_req) {
        RECT all = { 0, 0, (LONG)s->desc.dwWidth, (LONG)s->desc.dwHeight };
        if (rd_read_target(s, &all)) {
            DDSURFACEDESC2 d = s->desc;
            d.lpSurface = s->mem;
            write_desc_bmp(&d, g_shot_path);
        }
        g_shot_req = 0;
        InterlockedIncrement(&g_shot_done);
    }
    window_hook(hwnd);
    void *dpi = dpi_enter(hwnd);
    RECT cr;
    GetClientRect(hwnd, &cr);
    RECT v = view_rect(&cr, (int)s->desc.dwWidth, (int)s->desc.dwHeight);
    int ok = d3d_window_ready(hwnd, &cr) && d3d_draw_srv(s->srv, (int)s->desc.dwWidth, (int)s->desc.dwHeight, &cr, &v);
    rd3_dirty();
    dpi_leave(dpi);
    RD_UNLOCK();
    if (ok) {
        ValidateRect(hwnd, NULL);
        InterlockedIncrement(&g_present_frames);
    }
    return ok;
}

// [display] renderer = own: the vtables, and the display DLLs' DirectDraw imports at their load (display.c's
// show_loaded calls it for each)
internal void render_display_loaded(HMODULE m, const char *name) {
    if (!g_rd_on) return;
    int n = patch_iat(m, "ddraw.dll", "DirectDrawCreateEx", (void *)rd_create_ex) +
            patch_iat(m, "ddraw.dll", "DirectDrawEnumerateA", (void *)rd_enumerate) +
            patch_iat(m, "ddraw.dll", "DirectDrawEnumerateExA", (void *)rd_enumerate_ex);
    log_locked("# render: %s's DirectDraw imports %d/3 to the mod's renderer\n", name, n);
}
// autotest render_trace=N: the first N calls of the renderer's methods logged ("# render call", caller and thread) - a
// stub in front of each slot, as the probe's
internal volatile LONG g_rd_trace;
internal void *g_rd_traced[RDI_N][49];
internal void __cdecl rd_trace_call(uint32_t id, uintptr_t ret) {
    if (InterlockedDecrement(&g_rd_trace) < 0) return;
    int i = (int)(id >> 8), s = (int)(id & 0xFF);
    char at[160];
    crash_addr(at, sizeof at, ret);
    log_locked("# render call %s.%s from %s thread %lu\n", k_rd_iface[i], k_rd_names[i][s], at, GetCurrentThreadId());
}
internal void rd_trace_install(void) {
    uint8_t *code = VirtualAlloc(NULL, 8192, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    if (!code) return;
    uint8_t *p = code;
    for (int i = 0; i < RDI_N; i++)
        for (int s = 0; s < k_rd_slots[i]; s++) {   // push [esp]; push id; call rd_trace_call; add esp, 8; jmp [the method]
            g_rd_traced[i][s] = g_rd_vt[i][s];
            p[0] = 0xFF, p[1] = 0x34, p[2] = 0x24;
            p[3] = 0x68; *(uint32_t *)(p + 4) = (uint32_t)(i << 8 | s);
            p[8] = 0xE8; *(int32_t *)(p + 9) = (int32_t)((uintptr_t)rd_trace_call - (uintptr_t)(p + 13));
            p[13] = 0x83, p[14] = 0xC4, p[15] = 0x08;
            p[16] = 0xFF, p[17] = 0x25; *(void ***)(p + 18) = &g_rd_traced[i][s];
            g_rd_vt[i][s] = p;
            p += 24;
        }
    FlushInstructionCache(GetCurrentProcess(), code, 8192);
}
internal void render_install(const char *autotest_ini) {
    if (g_set.display_renderer != 1) return;
    if (g_fullscreen) {
        if (g_log) fprintf(g_log, "# render: own renderer needs [display] mode windowed or borderless - the game's ddraw.dll\n");
        return;
    }
    InitializeCriticalSection(&g_rd_cs);
    if (!rd_vtables()) return;
    void **v = g_rd_vt[RDI_DD];
    v[0] = (void *)rd_dd_qi; v[1] = (void *)rd_dd_addref; v[2] = (void *)rd_dd_release;
    v[6] = (void *)rd_dd_create_surface; v[8] = (void *)rd_dd_enum_modes; v[10] = (void *)rd_dd_ok;
    v[11] = (void *)rd_dd_get_caps; v[12] = (void *)rd_dd_get_display_mode; v[13] = (void *)rd_dd_fourcc;
    v[15] = (void *)rd_dd_monitor_freq; v[19] = (void *)rd_dd_ok; v[20] = (void *)rd_dd_set_coop;
    v[21] = (void *)rd_dd_set_mode; v[22] = (void *)rd_dd_wait_vb; v[23] = (void *)rd_dd_vidmem; v[25] = (void *)rd_dd_ok;
    v[26] = (void *)rd_dd_ok; v[27] = (void *)rd_dd_identifier;
    v = g_rd_vt[RDI_SURF];
    v[0] = (void *)rd_sf_qi; v[1] = (void *)rd_sf_addref; v[2] = (void *)rd_sf_release; v[3] = (void *)rd_sf_add_attached;
    v[5] = (void *)rd_sf_blt; v[7] = (void *)rd_sf_bltfast; v[11] = (void *)rd_sf_flip; v[12] = (void *)rd_sf_get_attached;
    v[14] = (void *)rd_sf_get_caps; v[16] = (void *)rd_sf_get_colorkey; v[21] = (void *)rd_sf_get_pixel_format;
    v[22] = (void *)rd_sf_get_desc; v[24] = (void *)rd_sf_ok; v[25] = (void *)rd_sf_lock; v[27] = (void *)rd_sf_ok;
    v[29] = (void *)rd_sf_set_colorkey; v[32] = (void *)rd_sf_unlock;
    rd3_vtables();
    if (autotest_ini && (g_rd_trace = ini_int("autotest", "render_trace", 0, autotest_ini)) > 0) rd_trace_install();
    g_rd_dd.vt = g_rd_vt[RDI_DD];
    g_rd_on = 1;
    if (g_log) fprintf(g_log, "# render: own renderer on (Direct3D 11) - the display DLLs' DirectDraw at their load%s\n",
                       g_rd_trace > 0 ? ", calls traced" : "");
}
