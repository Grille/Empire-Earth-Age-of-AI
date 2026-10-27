// DirectX 7 probe (autotest dx_probe=1; docs/testing.md): what the game asks of DirectDraw 7 and Direct3D 7 - the
// measure for a renderer of the mod's own that implements only the calls the game makes. The display DLLs
// (DX7HRDisplay.dll, DX7HRTnLDisplay.dll) are the only modules importing ddraw.dll (DirectDrawCreateEx): its object,
// and every object made from it (surfaces, Direct3D, the device, vertex buffers), gets its vtable wrapped - each
// method by a stub that counts the call by its caller's module (display DLL, engine, the mod, the wrapper, other), the
// methods a replacement must reproduce also by their arguments (render and texture stage states with their values,
// vertex formats, primitive types, surface kinds and pixel formats, lock / blit / flip flags). Every 5 s the whole
// table goes to dxprobe.txt beside the test ini (a crash keeps the last one). Test only: the stubs cost a call each.

enum { DXI_DD, DXI_SURF, DXI_D3D, DXI_DEV, DXI_VB, DXI_N };
internal const char *const k_dx_iface[DXI_N] = { "DirectDraw7", "Surface7", "Direct3D7", "Device7", "VertexBuffer7" };
internal const char *const k_dx_dd[] = { "QueryInterface", "AddRef", "Release", "Compact", "CreateClipper",
    "CreatePalette", "CreateSurface", "DuplicateSurface", "EnumDisplayModes", "EnumSurfaces", "FlipToGDISurface",
    "GetCaps", "GetDisplayMode", "GetFourCCCodes", "GetGDISurface", "GetMonitorFrequency", "GetScanLine",
    "GetVerticalBlankStatus", "Initialize", "RestoreDisplayMode", "SetCooperativeLevel", "SetDisplayMode",
    "WaitForVerticalBlank", "GetAvailableVidMem", "GetSurfaceFromDC", "RestoreAllSurfaces", "TestCooperativeLevel",
    "GetDeviceIdentifier", "StartModeTest", "EvaluateMode" };
internal const char *const k_dx_surf[] = { "QueryInterface", "AddRef", "Release", "AddAttachedSurface",
    "AddOverlayDirtyRect", "Blt", "BltBatch", "BltFast", "DeleteAttachedSurface", "EnumAttachedSurfaces",
    "EnumOverlayZOrders", "Flip", "GetAttachedSurface", "GetBltStatus", "GetCaps", "GetClipper", "GetColorKey", "GetDC",
    "GetFlipStatus", "GetOverlayPosition", "GetPalette", "GetPixelFormat", "GetSurfaceDesc", "Initialize", "IsLost",
    "Lock", "ReleaseDC", "Restore", "SetClipper", "SetColorKey", "SetOverlayPosition", "SetPalette", "Unlock",
    "UpdateOverlay", "UpdateOverlayDisplay", "UpdateOverlayZOrder", "GetDDInterface", "PageLock", "PageUnlock",
    "SetSurfaceDesc", "SetPrivateData", "GetPrivateData", "FreePrivateData", "GetUniquenessValue",
    "ChangeUniquenessValue", "SetPriority", "GetPriority", "SetLOD", "GetLOD" };
internal const char *const k_dx_d3d[] = { "QueryInterface", "AddRef", "Release", "EnumDevices", "CreateDevice",
    "CreateVertexBuffer", "EnumZBufferFormats", "EvictManagedTextures" };
internal const char *const k_dx_dev[] = { "QueryInterface", "AddRef", "Release", "GetCaps", "EnumTextureFormats",
    "BeginScene", "EndScene", "GetDirect3D", "SetRenderTarget", "GetRenderTarget", "Clear", "SetTransform",
    "GetTransform", "SetViewport", "MultiplyTransform", "GetViewport", "SetMaterial", "GetMaterial", "SetLight",
    "GetLight", "SetRenderState", "GetRenderState", "BeginStateBlock", "EndStateBlock", "PreLoad", "DrawPrimitive",
    "DrawIndexedPrimitive", "SetClipStatus", "GetClipStatus", "DrawPrimitiveStrided", "DrawIndexedPrimitiveStrided",
    "DrawPrimitiveVB", "DrawIndexedPrimitiveVB", "ComputeSphereVisibility", "GetTexture", "SetTexture",
    "GetTextureStageState", "SetTextureStageState", "ValidateDevice", "ApplyStateBlock", "CaptureStateBlock",
    "DeleteStateBlock", "CreateStateBlock", "Load", "LightEnable", "GetLightEnable", "SetClipPlane", "GetClipPlane",
    "GetInfo" };
internal const char *const k_dx_vb[] = { "QueryInterface", "AddRef", "Release", "Lock", "Unlock", "ProcessVertices",
    "GetVertexBufferDesc", "Optimize", "ProcessVerticesStrided" };
internal const char *const *const k_dx_names[DXI_N] = { k_dx_dd, k_dx_surf, k_dx_d3d, k_dx_dev, k_dx_vb };
internal const int k_dx_slots[DXI_N] = { 30, 49, 8, 49, 9 };
#define DX_ID(iface, slot) ((iface) * 64 + (slot))

// the methods with an argument record (their vtable slots)
enum { DD_QI = 0, DD_CREATE_SURFACE = 6, DD_SET_COOP = 20, DD_SET_MODE = 21 };
enum { SF_QI = 0, SF_BLT = 5, SF_BLTFAST = 7, SF_FLIP = 11, SF_GET_ATTACHED = 12, SF_LOCK = 25, SF_SET_COLORKEY = 29 };
enum { D3_CREATE_DEVICE = 4, D3_CREATE_VB = 5 };
enum { DV_END_SCENE = 6, DV_CLEAR = 10, DV_SET_TRANSFORM = 11, DV_SET_LIGHT = 18, DV_SET_RS = 20, DV_DRAW = 25,
       DV_DRAW_INDEXED = 26, DV_DRAW_VB = 31, DV_DRAW_INDEXED_VB = 32, DV_SET_TEXTURE = 35, DV_SET_TSS = 37,
       DV_LOAD = 43, DV_LIGHT_ENABLE = 44 };
enum { VB_LOCK = 3, VB_PROCESS = 5, VB_GET_DESC = 6 };

// callers: 0 the display DLLs, 1 the engine (exe, Low-Level Engine.dll), 2 the mod, 3 the wrapper (ddraw.dll), 4 other
#define DX_CALLERS 5
internal volatile LONG g_dx_calls[DXI_N * 64][DX_CALLERS];
internal struct { uintptr_t lo, hi; int cls; } g_dx_mods[6];
internal void dx_mod_range(int i, HMODULE m, int cls) {
    if (!m) return;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)((uint8_t *)m + ((IMAGE_DOS_HEADER *)m)->e_lfanew);
    g_dx_mods[i].lo = (uintptr_t)m;
    g_dx_mods[i].hi = (uintptr_t)m + nt->OptionalHeader.SizeOfImage;
    g_dx_mods[i].cls = cls;
}
internal int dx_caller(uintptr_t ret) {
    if (!g_dx_mods[0].lo || !g_dx_mods[1].lo) {   // the display DLLs load after the mod
        HMODULE self = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)(void *)dx_caller, &self);
        dx_mod_range(0, GetModuleHandleA("DX7HRDisplay.dll"), 0);
        dx_mod_range(1, GetModuleHandleA("DX7HRTnLDisplay.dll"), 0);
        dx_mod_range(2, GetModuleHandleA(NULL), 1);
        dx_mod_range(3, GetModuleHandleA("Low-Level Engine.dll"), 1);
        dx_mod_range(4, self, 2);
        dx_mod_range(5, GetModuleHandleA("ddraw.dll"), 3);
    }
    for (int i = 0; i < 6; i++)
        if (ret >= g_dx_mods[i].lo && ret < g_dx_mods[i].hi) return g_dx_mods[i].cls;
    return 4;
}
// the stubs' call (cdecl, eax / ecx / edx are free at a COM method's entry: everything is on the stack)
internal void __cdecl dx_count(uint32_t id, uintptr_t ret) {
    InterlockedIncrement(&g_dx_calls[id][dx_caller(ret)]);
}

// argument records: (key, a, b, c) -> count. Values that vary without end (fog distances, alpha reference, colours)
// stop at 32 different ones per state: the rest counts as "more"
#define DX_RECS 8192
internal struct { uint32_t k, a, b, c; LONG n; } g_dx_rec[DX_RECS];
internal int g_dx_nrec, g_dx_full;
internal uint8_t g_dx_rs_distinct[256], g_dx_tss_distinct[8][32];
internal CRITICAL_SECTION g_dx_cs;
#define DX_MORE 0xFFFFFFFEu
internal void dx_rec(uint32_t k, uint32_t a, uint32_t b, uint32_t c) {
    EnterCriticalSection(&g_dx_cs);
    uint32_t h = (k * 2654435761u ^ a * 40503u ^ b * 2246822519u ^ c * 3266489917u) % DX_RECS;
    for (int i = 0; i < DX_RECS; i++, h = (h + 1) % DX_RECS) {
        if (g_dx_rec[h].n && g_dx_rec[h].k == k && g_dx_rec[h].a == a && g_dx_rec[h].b == b && g_dx_rec[h].c == c) {
            g_dx_rec[h].n++;
            break;
        }
        if (!g_dx_rec[h].n) {
            if (g_dx_nrec >= DX_RECS * 3 / 4) { g_dx_full++; break; }
            g_dx_rec[h].k = k, g_dx_rec[h].a = a, g_dx_rec[h].b = b, g_dx_rec[h].c = c, g_dx_rec[h].n = 1;
            g_dx_nrec++;
            break;
        }
    }
    LeaveCriticalSection(&g_dx_cs);
}
internal int dx_rec_has(uint32_t k, uint32_t a, uint32_t b, uint32_t c) {
    uint32_t h = (k * 2654435761u ^ a * 40503u ^ b * 2246822519u ^ c * 3266489917u) % DX_RECS;
    for (int i = 0; i < DX_RECS && g_dx_rec[h].n; i++, h = (h + 1) % DX_RECS)
        if (g_dx_rec[h].k == k && g_dx_rec[h].a == a && g_dx_rec[h].b == b && g_dx_rec[h].c == c) return 1;
    return 0;
}

// a wrapped vtable: its original methods (a wrapper may have a vtable per kind of object - each its own record)
typedef struct { void **vt; int iface; void *orig[64]; } dx_vt;
#define DX_VTS 32
internal dx_vt g_dx_vts[DX_VTS];
internal int g_dx_nvts;
internal uint8_t *g_dx_stubs;
internal int g_dx_nstubs;
internal void *dx_orig(void *obj, int iface, int slot) {
    void **vt = *(void ***)obj;
    for (int i = 0; i < g_dx_nvts; i++) if (g_dx_vts[i].vt == vt) return g_dx_vts[i].orig[slot];
    for (int i = 0; i < g_dx_nvts; i++) if (g_dx_vts[i].iface == iface) return g_dx_vts[i].orig[slot];   // a copied vtable
    return NULL;
}
#define DX_ORIG(type, obj, iface, slot) ((type)dx_orig(obj, iface, slot))
#define DX_COUNT(iface, slot) dx_count(DX_ID(iface, slot), (uintptr_t)__builtin_return_address(0))

internal void *const *dx_typed(int iface);
internal void dx_wrap(void *obj, int iface) {
    if (!obj) return;
    void **vt = *(void ***)obj;
    EnterCriticalSection(&g_dx_cs);
    int known = 0;
    for (int i = 0; i < g_dx_nvts; i++) known |= g_dx_vts[i].vt == vt;
    if (known || g_dx_nvts == DX_VTS) { LeaveCriticalSection(&g_dx_cs); return; }
    if (!g_dx_stubs) g_dx_stubs = VirtualAlloc(NULL, 0x10000, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    dx_vt *r = &g_dx_vts[g_dx_nvts++];
    r->vt = vt;
    r->iface = iface;
    int n = k_dx_slots[iface];
    void *const *typed = dx_typed(iface);
    for (int s = 0; s < n; s++) r->orig[s] = vt[s];
    DWORD old;
    VirtualProtect(vt, n * 4, PAGE_READWRITE, &old);
    for (int s = 0; s < n; s++) {
        if (typed[s]) { vt[s] = typed[s]; continue; }
        if (!g_dx_stubs || g_dx_nstubs >= 0x10000 / 24) continue;
        // push [esp] (the return address); push id; call dx_count; add esp, 8; jmp [orig]
        uint8_t *p = g_dx_stubs + 24 * g_dx_nstubs++;
        p[0] = 0xFF, p[1] = 0x34, p[2] = 0x24;
        p[3] = 0x68; *(uint32_t *)(p + 4) = DX_ID(iface, s);
        p[8] = 0xE8; *(int32_t *)(p + 9) = (int32_t)((uintptr_t)dx_count - (uintptr_t)(p + 13));
        p[13] = 0x83, p[14] = 0xC4, p[15] = 0x08;
        p[16] = 0xFF, p[17] = 0x25; *(void ***)(p + 18) = &r->orig[s];
        vt[s] = p;
    }
    VirtualProtect(vt, n * 4, old, &old);
    LeaveCriticalSection(&g_dx_cs);
    FlushInstructionCache(GetCurrentProcess(), g_dx_stubs, 0x10000);
}

internal uint32_t dx_guid1(const void *g) { return g ? *(const uint32_t *)g : 0; }
internal uint32_t dx_bits(uint32_t m) { return (uint32_t)__builtin_popcount(m); }
internal uint32_t dx_bucket(uint32_t n) { uint32_t b = 1; while (b < n) b <<= 1; return b; }   // a power of two at least n

// DirectDraw7
typedef HRESULT (__stdcall *dx_fn2)(void *, uint32_t);
typedef HRESULT (__stdcall *dx_fn3)(void *, uint32_t, uint32_t);
typedef HRESULT (__stdcall *dx_fn4)(void *, uint32_t, uint32_t, uint32_t);
typedef HRESULT (__stdcall *dx_fn5)(void *, uint32_t, uint32_t, uint32_t, uint32_t);
typedef HRESULT (__stdcall *dx_fn6)(void *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
typedef HRESULT (__stdcall *dx_fn8)(void *, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
// the interfaces their QueryInterface hands out that the probe follows (the first dword of the IID)
#define IID1_D3D7 0xF5049E77u
#define IID1_DD7 0x15E65EC0u
#define IID1_SURF7 0x06675A80u
internal void dx_follow(uint32_t iid1, void *out) {
    if (iid1 == IID1_D3D7) dx_wrap(out, DXI_D3D);
    else if (iid1 == IID1_DD7) dx_wrap(out, DXI_DD);
    else if (iid1 == IID1_SURF7) dx_wrap(out, DXI_SURF);
}
internal HRESULT __stdcall dx_dd_qi(void *o, const GUID *iid, void **out) {
    DX_COUNT(DXI_DD, DD_QI);
    dx_rec(DX_ID(DXI_DD, DD_QI), dx_guid1(iid), 0, 0);
    HRESULT r = DX_ORIG(dx_fn3, o, DXI_DD, DD_QI)(o, (uint32_t)iid, (uint32_t)out);
    if (r == 0 && out) dx_follow(dx_guid1(iid), *out);
    return r;
}
internal HRESULT __stdcall dx_dd_create_surface(void *o, DDSURFACEDESC2 *d, void **out, void *outer) {
    DX_COUNT(DXI_DD, DD_CREATE_SURFACE);
    if (d) {
        uint32_t k = DX_ID(DXI_DD, DD_CREATE_SURFACE);
        dx_rec(k, d->dwFlags, d->ddsCaps.dwCaps, d->ddsCaps.dwCaps2);
        if (d->dwFlags & DDSD_PIXELFORMAT) {
            DDPIXELFORMAT *pf = &d->ddpfPixelFormat;
            dx_rec(k | 1 << 12, pf->dwFlags, pf->dwRGBBitCount,
                   dx_bits(pf->dwRBitMask) << 24 | dx_bits(pf->dwGBitMask) << 16 | dx_bits(pf->dwBBitMask) << 8 |
                   dx_bits(pf->dwRGBAlphaBitMask));
        }
        dx_rec(k | 2 << 12, d->dwFlags & DDSD_WIDTH ? d->dwWidth : 0, d->dwFlags & DDSD_HEIGHT ? d->dwHeight : 0,
               d->dwFlags & DDSD_MIPMAPCOUNT ? d->dwMipMapCount : 0);
    }
    HRESULT r = DX_ORIG(dx_fn4, o, DXI_DD, DD_CREATE_SURFACE)(o, (uint32_t)d, (uint32_t)out, (uint32_t)outer);
    if (r == 0 && out) dx_wrap(*out, DXI_SURF);
    return r;
}
internal HRESULT __stdcall dx_dd_set_coop(void *o, HWND h, uint32_t flags) {
    DX_COUNT(DXI_DD, DD_SET_COOP);
    dx_rec(DX_ID(DXI_DD, DD_SET_COOP), flags, 0, 0);
    return DX_ORIG(dx_fn3, o, DXI_DD, DD_SET_COOP)(o, (uint32_t)h, flags);
}
internal HRESULT __stdcall dx_dd_set_mode(void *o, uint32_t w, uint32_t h, uint32_t bpp, uint32_t hz, uint32_t flags) {
    DX_COUNT(DXI_DD, DD_SET_MODE);
    dx_rec(DX_ID(DXI_DD, DD_SET_MODE), w, h, bpp);
    return DX_ORIG(dx_fn6, o, DXI_DD, DD_SET_MODE)(o, w, h, bpp, hz, flags);
}

// Surface7
internal HRESULT __stdcall dx_sf_qi(void *o, const GUID *iid, void **out) {
    DX_COUNT(DXI_SURF, SF_QI);
    dx_rec(DX_ID(DXI_SURF, SF_QI), dx_guid1(iid), 0, 0);
    HRESULT r = DX_ORIG(dx_fn3, o, DXI_SURF, SF_QI)(o, (uint32_t)iid, (uint32_t)out);
    if (r == 0 && out) dx_follow(dx_guid1(iid), *out);
    return r;
}
internal HRESULT __stdcall dx_sf_blt(void *o, RECT *dst, void *src, RECT *srect, uint32_t flags, DDBLTFX *fx) {
    DX_COUNT(DXI_SURF, SF_BLT);
    dx_rec(DX_ID(DXI_SURF, SF_BLT), flags, (src ? 1 : 0) | (dst ? 2 : 0) | (srect ? 4 : 0),
           fx && (flags & DDBLT_ROP) ? fx->dwROP : 0);
    return DX_ORIG(dx_fn6, o, DXI_SURF, SF_BLT)(o, (uint32_t)dst, (uint32_t)src, (uint32_t)srect, flags, (uint32_t)fx);
}
internal HRESULT __stdcall dx_sf_bltfast(void *o, uint32_t x, uint32_t y, void *src, RECT *r, uint32_t trans) {
    DX_COUNT(DXI_SURF, SF_BLTFAST);
    dx_rec(DX_ID(DXI_SURF, SF_BLTFAST), trans, r ? 1 : 0, 0);
    return DX_ORIG(dx_fn6, o, DXI_SURF, SF_BLTFAST)(o, x, y, (uint32_t)src, (uint32_t)r, trans);
}
internal HRESULT __stdcall dx_sf_flip(void *o, void *target, uint32_t flags) {
    DX_COUNT(DXI_SURF, SF_FLIP);
    dx_rec(DX_ID(DXI_SURF, SF_FLIP), flags, target ? 1 : 0, 0);
    return DX_ORIG(dx_fn3, o, DXI_SURF, SF_FLIP)(o, (uint32_t)target, flags);
}
internal HRESULT __stdcall dx_sf_get_attached(void *o, DDSCAPS2 *caps, void **out) {
    DX_COUNT(DXI_SURF, SF_GET_ATTACHED);
    dx_rec(DX_ID(DXI_SURF, SF_GET_ATTACHED), caps ? caps->dwCaps : 0, caps ? caps->dwCaps2 : 0, 0);
    HRESULT r = DX_ORIG(dx_fn3, o, DXI_SURF, SF_GET_ATTACHED)(o, (uint32_t)caps, (uint32_t)out);
    if (r == 0 && out) dx_wrap(*out, DXI_SURF);
    return r;
}
internal HRESULT __stdcall dx_sf_lock(void *o, RECT *rect, DDSURFACEDESC2 *d, uint32_t flags, HANDLE ev) {
    DX_COUNT(DXI_SURF, SF_LOCK);
    HRESULT r = DX_ORIG(dx_fn5, o, DXI_SURF, SF_LOCK)(o, (uint32_t)rect, (uint32_t)d, flags, (uint32_t)ev);
    // what is locked: the surface's caps (the description the lock filled in)
    dx_rec(DX_ID(DXI_SURF, SF_LOCK), flags, rect ? 1 : 0, r == 0 && d ? d->ddsCaps.dwCaps : 0xFFFFFFFFu);
    return r;
}
internal HRESULT __stdcall dx_sf_set_colorkey(void *o, uint32_t flags, DDCOLORKEY *key) {
    DX_COUNT(DXI_SURF, SF_SET_COLORKEY);
    dx_rec(DX_ID(DXI_SURF, SF_SET_COLORKEY), flags, key ? key->dwColorSpaceLowValue : 0,
           key ? key->dwColorSpaceHighValue : 0);
    return DX_ORIG(dx_fn3, o, DXI_SURF, SF_SET_COLORKEY)(o, flags, (uint32_t)key);
}

// Direct3D7 (D3DVERTEXBUFFERDESC: size, caps, FVF, vertex count)
internal HRESULT __stdcall dx_d3_create_device(void *o, const GUID *cls, void *surf, void **out) {
    DX_COUNT(DXI_D3D, D3_CREATE_DEVICE);
    dx_rec(DX_ID(DXI_D3D, D3_CREATE_DEVICE), dx_guid1(cls), 0, 0);
    HRESULT r = DX_ORIG(dx_fn4, o, DXI_D3D, D3_CREATE_DEVICE)(o, (uint32_t)cls, (uint32_t)surf, (uint32_t)out);
    if (r == 0 && out) dx_wrap(*out, DXI_DEV);
    return r;
}
internal HRESULT __stdcall dx_d3_create_vb(void *o, uint32_t *desc, void **out, uint32_t flags) {
    DX_COUNT(DXI_D3D, D3_CREATE_VB);
    if (desc) dx_rec(DX_ID(DXI_D3D, D3_CREATE_VB), desc[1], desc[2], dx_bucket(desc[3]));
    HRESULT r = DX_ORIG(dx_fn4, o, DXI_D3D, D3_CREATE_VB)(o, (uint32_t)desc, (uint32_t)out, flags);
    if (r == 0 && out) dx_wrap(*out, DXI_VB);
    return r;
}

// Device7: the draws' totals (vertices, indices) beside their records
internal volatile LONG64 g_dx_verts, g_dx_indices;
internal uint32_t dx_vb_fvf(void *vb) {
    uint32_t desc[4] = { 16, 0, 0, 0 };
    dx_fn2 get = vb ? (dx_fn2)dx_orig(vb, DXI_VB, VB_GET_DESC) : NULL;
    return get && get(vb, (uint32_t)desc) == 0 ? desc[2] : 0xFFFFFFFFu;
}
internal HRESULT __stdcall dx_dv_clear(void *o, uint32_t n, void *rects, uint32_t flags, uint32_t col, float z, uint32_t st) {
    DX_COUNT(DXI_DEV, DV_CLEAR);
    dx_rec(DX_ID(DXI_DEV, DV_CLEAR), flags, n ? 1 : 0, 0);
    typedef HRESULT (__stdcall *fn)(void *, uint32_t, void *, uint32_t, uint32_t, float, uint32_t);
    return DX_ORIG(fn, o, DXI_DEV, DV_CLEAR)(o, n, rects, flags, col, z, st);
}
internal HRESULT __stdcall dx_dv_set_transform(void *o, uint32_t type, void *m) {
    DX_COUNT(DXI_DEV, DV_SET_TRANSFORM);
    dx_rec(DX_ID(DXI_DEV, DV_SET_TRANSFORM), type, 0, 0);
    return DX_ORIG(dx_fn3, o, DXI_DEV, DV_SET_TRANSFORM)(o, type, (uint32_t)m);
}
internal HRESULT __stdcall dx_dv_set_light(void *o, uint32_t i, uint32_t *light) {
    DX_COUNT(DXI_DEV, DV_SET_LIGHT);
    dx_rec(DX_ID(DXI_DEV, DV_SET_LIGHT), i, light ? light[0] : 0, 0);
    return DX_ORIG(dx_fn3, o, DXI_DEV, DV_SET_LIGHT)(o, i, (uint32_t)light);
}
internal HRESULT __stdcall dx_dv_set_rs(void *o, uint32_t state, uint32_t value) {
    DX_COUNT(DXI_DEV, DV_SET_RS);
    uint32_t k = DX_ID(DXI_DEV, DV_SET_RS), v = value;
    if (state < 256 && !dx_rec_has(k, state, value, 0)) {
        if (g_dx_rs_distinct[state] >= 32) v = DX_MORE;
        else g_dx_rs_distinct[state]++;
    }
    dx_rec(k, state, v, 0);
    return DX_ORIG(dx_fn3, o, DXI_DEV, DV_SET_RS)(o, state, value);
}
internal HRESULT __stdcall dx_dv_draw(void *o, uint32_t type, uint32_t fvf, void *v, uint32_t n, uint32_t flags) {
    DX_COUNT(DXI_DEV, DV_DRAW);
    dx_rec(DX_ID(DXI_DEV, DV_DRAW), type, fvf, flags);
    InterlockedAdd64(&g_dx_verts, n);
    return DX_ORIG(dx_fn6, o, DXI_DEV, DV_DRAW)(o, type, fvf, (uint32_t)v, n, flags);
}
internal HRESULT __stdcall dx_dv_draw_indexed(void *o, uint32_t type, uint32_t fvf, void *v, uint32_t nv, void *idx,
                                             uint32_t ni, uint32_t flags) {
    DX_COUNT(DXI_DEV, DV_DRAW_INDEXED);
    dx_rec(DX_ID(DXI_DEV, DV_DRAW_INDEXED), type, fvf, flags);
    InterlockedAdd64(&g_dx_verts, nv);
    InterlockedAdd64(&g_dx_indices, ni);
    return DX_ORIG(dx_fn8, o, DXI_DEV, DV_DRAW_INDEXED)(o, type, fvf, (uint32_t)v, nv, (uint32_t)idx, ni, flags);
}
internal HRESULT __stdcall dx_dv_draw_vb(void *o, uint32_t type, void *vb, uint32_t start, uint32_t n, uint32_t flags) {
    DX_COUNT(DXI_DEV, DV_DRAW_VB);
    dx_rec(DX_ID(DXI_DEV, DV_DRAW_VB), type, dx_vb_fvf(vb), flags);
    InterlockedAdd64(&g_dx_verts, n);
    return DX_ORIG(dx_fn6, o, DXI_DEV, DV_DRAW_VB)(o, type, (uint32_t)vb, start, n, flags);
}
internal HRESULT __stdcall dx_dv_draw_indexed_vb(void *o, uint32_t type, void *vb, uint32_t start, uint32_t nv,
                                                void *idx, uint32_t ni, uint32_t flags) {
    DX_COUNT(DXI_DEV, DV_DRAW_INDEXED_VB);
    dx_rec(DX_ID(DXI_DEV, DV_DRAW_INDEXED_VB), type, dx_vb_fvf(vb), flags);
    InterlockedAdd64(&g_dx_verts, nv);
    InterlockedAdd64(&g_dx_indices, ni);
    return DX_ORIG(dx_fn8, o, DXI_DEV, DV_DRAW_INDEXED_VB)(o, type, (uint32_t)vb, start, nv, (uint32_t)idx, ni, flags);
}
internal HRESULT __stdcall dx_dv_set_texture(void *o, uint32_t stage, void *tex) {
    DX_COUNT(DXI_DEV, DV_SET_TEXTURE);
    dx_rec(DX_ID(DXI_DEV, DV_SET_TEXTURE), stage, tex ? 1 : 0, 0);
    return DX_ORIG(dx_fn3, o, DXI_DEV, DV_SET_TEXTURE)(o, stage, (uint32_t)tex);
}
internal HRESULT __stdcall dx_dv_set_tss(void *o, uint32_t stage, uint32_t type, uint32_t value) {
    DX_COUNT(DXI_DEV, DV_SET_TSS);
    uint32_t k = DX_ID(DXI_DEV, DV_SET_TSS), v = value;
    if (stage < 8 && type < 32 && !dx_rec_has(k, stage, type, value)) {
        if (g_dx_tss_distinct[stage][type] >= 32) v = DX_MORE;
        else g_dx_tss_distinct[stage][type]++;
    }
    dx_rec(k, stage, type, v);
    return DX_ORIG(dx_fn4, o, DXI_DEV, DV_SET_TSS)(o, stage, type, value);
}
internal HRESULT __stdcall dx_dv_load(void *o, void *dst, POINT *pt, void *src, RECT *r, uint32_t flags) {
    DX_COUNT(DXI_DEV, DV_LOAD);
    dx_rec(DX_ID(DXI_DEV, DV_LOAD), flags, (pt ? 1 : 0) | (r ? 2 : 0), 0);
    return DX_ORIG(dx_fn6, o, DXI_DEV, DV_LOAD)(o, (uint32_t)dst, (uint32_t)pt, (uint32_t)src, (uint32_t)r, flags);
}
internal HRESULT __stdcall dx_dv_light_enable(void *o, uint32_t i, uint32_t on) {
    DX_COUNT(DXI_DEV, DV_LIGHT_ENABLE);
    dx_rec(DX_ID(DXI_DEV, DV_LIGHT_ENABLE), i, on, 0);
    return DX_ORIG(dx_fn3, o, DXI_DEV, DV_LIGHT_ENABLE)(o, i, on);
}

// VertexBuffer7
internal HRESULT __stdcall dx_vb_lock(void *o, uint32_t flags, void **p, uint32_t *size) {
    DX_COUNT(DXI_VB, VB_LOCK);
    dx_rec(DX_ID(DXI_VB, VB_LOCK), flags, 0, 0);
    return DX_ORIG(dx_fn4, o, DXI_VB, VB_LOCK)(o, flags, (uint32_t)p, (uint32_t)size);
}
internal HRESULT __stdcall dx_vb_process(void *o, uint32_t op, uint32_t di, uint32_t n, void *src, uint32_t si,
                                        void *dev, uint32_t flags) {
    DX_COUNT(DXI_VB, VB_PROCESS);
    dx_rec(DX_ID(DXI_VB, VB_PROCESS), op, flags, dx_vb_fvf(src));
    return DX_ORIG(dx_fn8, o, DXI_VB, VB_PROCESS)(o, op, di, n, (uint32_t)src, si, (uint32_t)dev, flags);
}

internal void *const *dx_typed(int iface) {
    static void *t[DXI_N][64];
    static int made;
    if (!made) {
        made = 1;
        t[DXI_DD][DD_QI] = (void *)dx_dd_qi;
        t[DXI_DD][DD_CREATE_SURFACE] = (void *)dx_dd_create_surface;
        t[DXI_DD][DD_SET_COOP] = (void *)dx_dd_set_coop;
        t[DXI_DD][DD_SET_MODE] = (void *)dx_dd_set_mode;
        t[DXI_SURF][SF_QI] = (void *)dx_sf_qi;
        t[DXI_SURF][SF_BLT] = (void *)dx_sf_blt;
        t[DXI_SURF][SF_BLTFAST] = (void *)dx_sf_bltfast;
        t[DXI_SURF][SF_FLIP] = (void *)dx_sf_flip;
        t[DXI_SURF][SF_GET_ATTACHED] = (void *)dx_sf_get_attached;
        t[DXI_SURF][SF_LOCK] = (void *)dx_sf_lock;
        t[DXI_SURF][SF_SET_COLORKEY] = (void *)dx_sf_set_colorkey;
        t[DXI_D3D][D3_CREATE_DEVICE] = (void *)dx_d3_create_device;
        t[DXI_D3D][D3_CREATE_VB] = (void *)dx_d3_create_vb;
        t[DXI_DEV][DV_CLEAR] = (void *)dx_dv_clear;
        t[DXI_DEV][DV_SET_TRANSFORM] = (void *)dx_dv_set_transform;
        t[DXI_DEV][DV_SET_LIGHT] = (void *)dx_dv_set_light;
        t[DXI_DEV][DV_SET_RS] = (void *)dx_dv_set_rs;
        t[DXI_DEV][DV_DRAW] = (void *)dx_dv_draw;
        t[DXI_DEV][DV_DRAW_INDEXED] = (void *)dx_dv_draw_indexed;
        t[DXI_DEV][DV_DRAW_VB] = (void *)dx_dv_draw_vb;
        t[DXI_DEV][DV_DRAW_INDEXED_VB] = (void *)dx_dv_draw_indexed_vb;
        t[DXI_DEV][DV_SET_TEXTURE] = (void *)dx_dv_set_texture;
        t[DXI_DEV][DV_SET_TSS] = (void *)dx_dv_set_tss;
        t[DXI_DEV][DV_LOAD] = (void *)dx_dv_load;
        t[DXI_DEV][DV_LIGHT_ENABLE] = (void *)dx_dv_light_enable;
        t[DXI_VB][VB_LOCK] = (void *)dx_vb_lock;
        t[DXI_VB][VB_PROCESS] = (void *)dx_vb_process;
    }
    return t[iface];
}

typedef HRESULT (WINAPI *dd_create_ex_fn)(GUID *, void **, REFIID, IUnknown *);
internal dd_create_ex_fn g_dx_create_ex;
internal HRESULT WINAPI dx_create_ex(GUID *guid, void **out, REFIID iid, IUnknown *outer) {
    dx_count(DX_ID(DXI_DD, 63), (uintptr_t)__builtin_return_address(0));   // slot 63: DirectDrawCreateEx itself
    HRESULT r = g_dx_create_ex(guid, out, iid, outer);
    if (r == 0 && out) dx_wrap(*out, DXI_DD);
    return r;
}

// the names the table prints with: render states and texture stage states (D3D7's numbers)
internal const char *dx_rs_name(uint32_t s) {
    switch (s) {
    case 2: return "ANTIALIAS"; case 4: return "TEXTUREPERSPECTIVE"; case 7: return "ZENABLE"; case 8: return "FILLMODE";
    case 9: return "SHADEMODE"; case 10: return "LINEPATTERN"; case 14: return "ZWRITEENABLE";
    case 15: return "ALPHATESTENABLE"; case 16: return "LASTPIXEL"; case 19: return "SRCBLEND"; case 20: return "DESTBLEND";
    case 22: return "CULLMODE"; case 23: return "ZFUNC"; case 24: return "ALPHAREF"; case 25: return "ALPHAFUNC";
    case 26: return "DITHERENABLE"; case 27: return "ALPHABLENDENABLE"; case 28: return "FOGENABLE";
    case 29: return "SPECULARENABLE"; case 30: return "ZVISIBLE"; case 33: return "STIPPLEDALPHA"; case 34: return "FOGCOLOR";
    case 35: return "FOGTABLEMODE"; case 36: return "FOGSTART"; case 37: return "FOGEND"; case 38: return "FOGDENSITY";
    case 40: return "EDGEANTIALIAS"; case 41: return "COLORKEYENABLE"; case 47: return "ZBIAS"; case 48: return "RANGEFOGENABLE";
    case 52: return "STENCILENABLE"; case 53: return "STENCILFAIL"; case 54: return "STENCILZFAIL"; case 55: return "STENCILPASS";
    case 56: return "STENCILFUNC"; case 57: return "STENCILREF"; case 58: return "STENCILMASK"; case 59: return "STENCILWRITEMASK";
    case 60: return "TEXTUREFACTOR"; case 136: return "CLIPPING"; case 137: return "LIGHTING"; case 138: return "EXTENTS";
    case 139: return "AMBIENT"; case 140: return "FOGVERTEXMODE"; case 141: return "COLORVERTEX"; case 142: return "LOCALVIEWER";
    case 143: return "NORMALIZENORMALS"; case 144: return "COLORKEYBLENDENABLE"; case 145: return "DIFFUSEMATERIALSOURCE";
    case 146: return "SPECULARMATERIALSOURCE"; case 147: return "AMBIENTMATERIALSOURCE"; case 148: return "EMISSIVEMATERIALSOURCE";
    case 151: return "VERTEXBLEND"; case 152: return "CLIPPLANEENABLE";
    default: return s >= 128 && s <= 135 ? "WRAPn" : "?";
    }
}
internal const char *dx_tss_name(uint32_t t) {
    static const char *const n[] = { "?", "COLOROP", "COLORARG1", "COLORARG2", "ALPHAOP", "ALPHAARG1", "ALPHAARG2",
        "BUMPENVMAT00", "BUMPENVMAT01", "BUMPENVMAT10", "BUMPENVMAT11", "TEXCOORDINDEX", "ADDRESS", "ADDRESSU",
        "ADDRESSV", "BORDERCOLOR", "MAGFILTER", "MINFILTER", "MIPFILTER", "MIPMAPLODBIAS", "MAXMIPLEVEL",
        "MAXANISOTROPY", "BUMPENVLSCALE", "BUMPENVLOFFSET", "TEXTURETRANSFORMFLAGS" };
    return t < sizeof n / sizeof *n ? n[t] : "?";
}
internal int dx_rec_cmp(const void *x, const void *y) {
    const uint32_t *a = x, *b = y;   // k, a, b, c
    for (int i = 0; i < 4; i++) if (a[i] != b[i]) return a[i] < b[i] ? -1 : 1;
    return 0;
}
internal float dx_float(uint32_t v) { float f; memcpy(&f, &v, 4); return f; }

internal char g_dx_path[MAX_PATH];
internal DWORD g_dx_t0;
internal void dx_write(void) {
    FILE *f = fopen(g_dx_path, "w");
    if (!f) return;
    LONG frames = g_dx_calls[DX_ID(DXI_DEV, DV_END_SCENE)][0];
    double per = frames > 0 ? 1.0 / frames : 0;
    fprintf(f, "# dxprobe: %lu s, %ld frames (EndScene by the display DLL), %lld vertices, %lld indices drawn; "
               "%d argument records%s\n", (unsigned long)((GetTickCount() - g_dx_t0) / 1000), frames,
            (long long)g_dx_verts, (long long)g_dx_indices, g_dx_nrec, g_dx_full ? " (TABLE FULL - some left out)" : "");
    fprintf(f, "## calls: method, total, per frame, by caller - display DLL / engine / mod / wrapper / other\n");
    for (int i = 0; i < DXI_N; i++)
        for (int s = 0; s < 64; s++) {
            volatile LONG *c = g_dx_calls[DX_ID(i, s)];
            long t = c[0] + c[1] + c[2] + c[3] + c[4];
            if (!t) continue;
            fprintf(f, "%s.%s %ld %.2f | %ld %ld %ld %ld %ld\n", k_dx_iface[i],
                    s == 63 ? "DirectDrawCreateEx" : s < k_dx_slots[i] ? k_dx_names[i][s] : "?", t, t * per,
                    (long)c[0], (long)c[1], (long)c[2], (long)c[3], (long)c[4]);
        }
    fprintf(f, "## arguments: method (record) values - count\n");
    EnterCriticalSection(&g_dx_cs);
    static struct { uint32_t k, a, b, c; LONG n; } rows[DX_RECS];
    int n = 0;
    for (int i = 0; i < DX_RECS; i++) if (g_dx_rec[i].n) memcpy(&rows[n++], &g_dx_rec[i], sizeof rows[0]);
    LeaveCriticalSection(&g_dx_cs);
    qsort(rows, n, sizeof rows[0], dx_rec_cmp);
    for (int i = 0; i < n; i++) {
        uint32_t k = rows[i].k, id = k & 0xFFF, sub = k >> 12, a = rows[i].a, b = rows[i].b, c = rows[i].c;
        int iface = id / 64, s = id % 64;
        fprintf(f, "%s.%s", k_dx_iface[iface], s < k_dx_slots[iface] ? k_dx_names[iface][s] : "?");
        if (id == DX_ID(DXI_DEV, DV_SET_RS)) {
            if (b == DX_MORE) fprintf(f, " %s(%u) = more values", dx_rs_name(a), a);
            else fprintf(f, " %s(%u) = 0x%x (%u, %g)", dx_rs_name(a), a, b, b, dx_float(b));
        } else if (id == DX_ID(DXI_DEV, DV_SET_TSS)) {
            if (c == DX_MORE) fprintf(f, " stage %u %s(%u) = more values", a, dx_tss_name(b), b);
            else fprintf(f, " stage %u %s(%u) = 0x%x (%u, %g)", a, dx_tss_name(b), b, c, c, dx_float(c));
        } else if (id == DX_ID(DXI_DD, DD_CREATE_SURFACE))
            fprintf(f, sub == 1 ? " pixel format flags 0x%x, %u bits, rgba bits 0x%08x"
                     : sub == 2 ? " size %u x %u, mipmaps %u" : " flags 0x%x caps 0x%x caps2 0x%x", a, b, c);
        else if (id == DX_ID(DXI_D3D, D3_CREATE_VB)) fprintf(f, " caps 0x%x fvf 0x%x vertices <= %u", a, b, c);
        else if (id == DX_ID(DXI_DEV, DV_DRAW_VB) || id == DX_ID(DXI_DEV, DV_DRAW_INDEXED_VB) ||
                 id == DX_ID(DXI_DEV, DV_DRAW) || id == DX_ID(DXI_DEV, DV_DRAW_INDEXED))
            fprintf(f, " primitive %u fvf 0x%x flags 0x%x", a, b, c);
        else if (id == DX_ID(DXI_SURF, SF_LOCK)) fprintf(f, " flags 0x%x rect %u, surface caps 0x%x", a, b, c);
        else if (id == DX_ID(DXI_SURF, SF_BLT)) fprintf(f, " flags 0x%x src/dst/srcrect %u rop 0x%x", a, b, c);
        else fprintf(f, " 0x%x 0x%x 0x%x", a, b, c);
        fprintf(f, " - %ld\n", (long)rows[i].n);
    }
    fclose(f);
}
internal DWORD WINAPI dx_writer(void *arg) {
    (void)arg;
    for (;;) { Sleep(5000); dx_write(); }
    return 0;
}

internal void dx_display_loaded(uintptr_t base) {
    HMODULE m = (HMODULE)base;
    if (!g_dx_create_ex) {
        HMODULE dd = GetModuleHandleA("ddraw.dll");
        g_dx_create_ex = dd ? (dd_create_ex_fn)(void *)GetProcAddress(dd, "DirectDrawCreateEx") : NULL;
    }
    int ok = g_dx_create_ex && patch_iat(m, "ddraw.dll", "DirectDrawCreateEx", (void *)dx_create_ex);
    log_locked("# dxprobe: %s's DirectDrawCreateEx %s\n", m == GetModuleHandleA("DX7HRDisplay.dll") ? "DX7HRDisplay.dll"
               : "DX7HRTnLDisplay.dll", ok ? "hooked" : "NOT hooked");
}
internal void dx_probe_start(const char *ini) {
    if (!ini_int("autotest", "dx_probe", 0, ini)) return;
    InitializeCriticalSection(&g_dx_cs);
    g_dx_t0 = GetTickCount();
    snprintf(g_dx_path, sizeof g_dx_path, "%s", ini);
    char *slash = strrchr(g_dx_path, '\\');
    snprintf(slash ? slash + 1 : g_dx_path, sizeof g_dx_path - (slash ? (size_t)(slash + 1 - g_dx_path) : 0), "dxprobe.txt");
    int w = on_module_load("DX7HRDisplay.dll", dx_display_loaded) + on_module_load("DX7HRTnLDisplay.dll", dx_display_loaded);
    CloseHandle(CreateThread(NULL, 0, dx_writer, NULL, 0, NULL));
    if (g_log) fprintf(g_log, "# dxprobe: on, %d display DLLs watched, table into %s\n", w, g_dx_path);
}
