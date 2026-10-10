// Display: resolution of the scenario editor.
// aimod.ini [editor] resolution = ee | aoc | game | <width>x<height>   (default ee: nothing is changed)
//   ee   = 800 x 600 (original Empire Earth)
//   aoc  = 1024 x 768 (what The Art of Conquest uses)
//   game = the resolution chosen for games in the settings
//
// The editor is entered from the main menu (0x6601D9) and on return from test play (0x65FD21, ScenDefault);
// both switch the screen with SetResolution 0x65F314(width, height, force, flag) - stdcall (ret 0x10; ECX
// unused) - with a fixed 800 x 600 (call sites 0x6601F1 and 0x65FD6D). AoC uses 1024 x 768 in both places;
// games use [0x9193FC] x [0x9193F8].

#include <ddraw.h>

#define SITE_EDITOR_MENU  0x006601F1
#define SITE_EDITOR_RES   0x0065FD6D
#define FN_SET_RESOLUTION 0x0065F314
#define G_GAME_RES_W      0x009193FC
#define G_GAME_RES_H      0x009193F8

static int g_editor_w, g_editor_h, g_editor_game;   // 0 = leave

// aoc: the editor layout for 1024x768 (map view 1024x606, panels moved down 168 px, right side 224 px), as
// Art of Conquest has it - generated from a comparison of both exes (scripts/layout_diff.py, gen_editor_layout.py).
#include "editor_layout.h"
#define N_LAYOUT (sizeof k_editor_layout / sizeof *k_editor_layout)
static uint32_t g_layout_consts[N_LAYOUT];

static int apply_editor_layout(void) {
    int ok = 0, bad = 0;
    for (size_t i = 0; i < N_LAYOUT; i++) {
        uint32_t *p = (uint32_t *)k_editor_layout[i].addr;
        DWORD old;
        if (k_editor_layout[i].kind == 0) {
            if (*p != k_editor_layout[i].old) { bad++; continue; }
            VirtualProtect(p, 4, PAGE_EXECUTE_READWRITE, &old);
            *p = k_editor_layout[i].val;
        } else {   // redirect the absolute operand to our own constant
            uint32_t *c = (uint32_t *)*p;
            if ((uintptr_t)c < 0x401000 || (uintptr_t)c >= 0xA00000 || *c != k_editor_layout[i].old) { bad++; continue; }
            g_layout_consts[i] = k_editor_layout[i].val;
            VirtualProtect(p, 4, PAGE_EXECUTE_READWRITE, &old);
            *p = (uint32_t)(uintptr_t)&g_layout_consts[i];
        }
        VirtualProtect(p, 4, old, &old);
        FlushInstructionCache(GetCurrentProcess(), p, 4);
        ok++;
    }
    // the file buttons (load, save, test, exit ...) come from a data table, not code: 0x8AE0A0, 6 entries of
    // {id, x, y, texture u0, v0, u1, v1, tooltip}; AoC (0x8C47B0) has them at x 647.. (step 35), y 736
    int32_t *t = (int32_t *)0x008AE0A0;
    for (int i = 0; i < 6; i++, t += 8) {
        if (t[1] != 415 + 35 * i || t[2] != 567) { bad++; continue; }
        t[1] = 647 + 35 * i;
        t[2] = 736;
        ok++;
    }
    if (g_log && bad) fprintf(g_log, "# editor: %d layout patch sites did not match and were skipped\n", bad);
    return ok;
}

typedef void (__stdcall *SetResolutionFn)(int w, int h, int force, int flag);
static void __stdcall hook_editor_resolution(int w, int h, int force, int flag) {
    int nw = g_editor_game ? *(int *)G_GAME_RES_W : g_editor_w;
    int nh = g_editor_game ? *(int *)G_GAME_RES_H : g_editor_h;
    if (nw >= 640 && nh >= 480) { w = nw; h = nh; }
    if (g_log) { fprintf(g_log, "# editor: resolution %dx%d\n", w, h); fflush(g_log); }
    ((SetResolutionFn)FN_SET_RESOLUTION)(w, h, force, flag);
}

// Windowed mode: the engine still has the windowed path SSSI used during development - with the flag byte
// 0x91945a = 0 the game creates a normal titled window (DX7Screen::CreateAWindow mode 1), takes DirectDraw in
// DDSCL_NORMAL instead of exclusive mode (Capture), never changes the display mode (SetMode checks the screen's
// fullscreen flag), presents by blitting into the client area instead of flipping (ShowScene), and
// minimizes/restores with ShowWindow. The byte is already 1 when the DLLs load and nothing writes it later, so
// clearing it here switches every reader (including dreXmod's hook on the loading screen at 0x534433).
// aimod.ini [display] windowed = 0 | 1
#define G_FULLSCREEN 0x0091945A
static uint8_t g_fullscreen = 1;
static int g_diag;   // diagnostic sampling (aimod.ini [display] diag=1 or autotest diag_hooks=1)

// Windowed present: ShowScene (DX7Rasterizer vtable slot 66, in DX7HRDisplay.dll and DX7HRTnLDisplay.dll)
// blits the back buffer to the primary surface at the window's screen position. On current Windows the primary a
// DDSCL_NORMAL app gets is an emulated 1024x768 one, so that Blt fails (DDERR_INVALIDRECT) and the window stays
// black. After the original we copy the back buffer into the window ourselves with GDI (stretched if the client
// size differs). Per-screen state: rasterizer +0x20[screen] -> +0x2A8 DX7Screen (+0x3D fullscreen),
// +0x2B0 back GESurface (DX7Surface +0x48 IDirectDrawSurface7), +0x2BC GEWindow (+0 HWND).
// The live objects are subclasses with their own vtables (the inherited slots still point to the DX7 base
// functions), so every pointer to a hooked function in the module's read-only data is replaced.
//
// SetMode (DX7Screen) skips everything when not fullscreen - also recording the mode index (+0x38), so in
// windowed mode the screen's current mode stays uninitialised and ActivateScreen builds the primary surface
// description from a garbage pixel format index (intermittent crash in CreateSurface). Windowed, we only record it.
typedef int (__thiscall *ShowSceneFn)(void *ras, unsigned screen);
typedef int (__thiscall *SetModeFn)(void *scr, unsigned mode);
static struct { const char *dll; uintptr_t lo, hi; ShowSceneFn show; SetModeFn set_mode; } g_show[2] = {
    { "DX7HRDisplay.dll", 0, 0, NULL, NULL }, { "DX7HRTnLDisplay.dll", 0, 0, NULL, NULL },
};
#define DISPLAY_DLL(obj) (*(uintptr_t *)(obj) >= g_show[0].lo && *(uintptr_t *)(obj) < g_show[0].hi ? 0 : 1)
static volatile LONG g_present_frames, g_present_fail;

static int present_to_window(uint8_t *ras, unsigned screen) {
    uint8_t *st = FIELD(ras, 0x20, uint8_t **)[screen];
    uint8_t *scr = FIELD(st, 0x2A8, uint8_t *), *back = FIELD(st, 0x2B0, uint8_t *), *win = FIELD(st, 0x2BC, uint8_t *);
    if (!scr || scr[0x3D] || !back || !win) return 0;
    IDirectDrawSurface7 *s = FIELD(back, 0x48, IDirectDrawSurface7 *);
    HWND hwnd = FIELD(win, 0, HWND);
    if (!s || !hwnd) return 0;
    // Lock, not GetDC: the GDI view of the 3D render target reads back black under current Windows' DirectDraw
    DDSURFACEDESC2 d = { .dwSize = sizeof d };
    if (s->lpVtbl->Lock(s, NULL, &d, DDLOCK_WAIT | DDLOCK_READONLY, NULL) != DD_OK) {
        InterlockedIncrement(&g_present_fail);
        return 0;
    }
    int bpp = (int)d.ddpfPixelFormat.dwRGBBitCount;
    struct { BITMAPINFOHEADER h; DWORD masks[3]; } bi = {
        { sizeof bi.h, d.lPitch / (bpp / 8), -(LONG)d.dwHeight, 1, (WORD)bpp, bpp == 16 ? BI_BITFIELDS : BI_RGB, 0, 0, 0, 0, 0 },
        { d.ddpfPixelFormat.dwRBitMask, d.ddpfPixelFormat.dwGBitMask, d.ddpfPixelFormat.dwBBitMask },
    };
    HDC wdc = GetDC(hwnd);
    RECT cr;
    GetClientRect(hwnd, &cr);
    SetStretchBltMode(wdc, COLORONCOLOR);
    StretchDIBits(wdc, 0, 0, cr.right, cr.bottom, 0, 0, (int)d.dwWidth, (int)d.dwHeight, d.lpSurface,
                  (BITMAPINFO *)&bi, DIB_RGB_COLORS, SRCCOPY);
    ReleaseDC(hwnd, wdc);
    s->lpVtbl->Unlock(s, NULL);
    InterlockedIncrement(&g_present_frames);
    return 1;
}

// diagnostics: share of non-black pixels in the back buffer (sampled) before the original ShowScene
static volatile LONG g_lit_samples, g_lit_frames, g_lock_fail;
static void sample_back(uint8_t *ras, unsigned screen) {
    uint8_t *st = FIELD(ras, 0x20, uint8_t **)[screen];
    uint8_t *back = FIELD(st, 0x2B0, uint8_t *);
    IDirectDrawSurface7 *s = back ? FIELD(back, 0x48, IDirectDrawSurface7 *) : NULL;
    if (!s) return;
    DDSURFACEDESC2 d = { .dwSize = sizeof d };
    if (s->lpVtbl->Lock(s, NULL, &d, DDLOCK_WAIT | DDLOCK_READONLY, NULL) != DD_OK) { InterlockedIncrement(&g_lock_fail); return; }
    int lit = 0, bpp = (int)d.ddpfPixelFormat.dwRGBBitCount / 8;
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 32; x++) {
            uint8_t *p = (uint8_t *)d.lpSurface + (size_t)(y * (int)d.dwHeight / 32) * d.lPitch + (size_t)(x * (int)d.dwWidth / 32) * bpp;
            if (bpp == 4 ? (*(uint32_t *)p & 0xFFFFFF) != 0 : *(uint16_t *)p != 0) lit++;
        }
    s->lpVtbl->Unlock(s, NULL);
    InterlockedExchangeAdd(&g_lit_samples, lit);
    InterlockedIncrement(&g_lit_frames);
}

static int __fastcall hook_show_scene(void *ras, void *edx, unsigned screen) {
    (void)edx;
    if (g_diag && !g_fullscreen && (g_present_frames & 63) == 0) sample_back(ras, screen);
    int r = g_show[DISPLAY_DLL(ras)].show(ras, screen);   // windowed: its Blt to the emulated primary fails, harmless
    if (!g_fullscreen && present_to_window(ras, screen)) r = 0;
    return r;
}

static int __fastcall hook_set_mode(void *scr, void *edx, unsigned mode) {
    (void)edx;
    if (((uint8_t *)scr)[0x3D]) return g_show[DISPLAY_DLL(scr)].set_mode(scr, mode);
    FIELD(scr, 0x38, unsigned) = mode;
    return 0;
}

// GetCurrentMode / GetCurrentModeIndex: the index the constructor takes from the mode enumeration can be out of
// range windowed (the game reads the current mode before its first SetMode - crash at 0x535958); clamp it.
// Modes: vector of 0x14-byte GEVideoMode at +0x10 (begin) / +0x14 (end).
static unsigned screen_mode_index(uint8_t *scr) {
    unsigned n = (FIELD(scr, 0x14, uintptr_t) - FIELD(scr, 0x10, uintptr_t)) / 0x14;
    if (!scr[0x3D] && FIELD(scr, 0x38, unsigned) >= n) FIELD(scr, 0x38, unsigned) = 0;
    return FIELD(scr, 0x38, unsigned);
}
static void *__fastcall hook_get_current_mode(uint8_t *scr) {
    return FIELD(scr, 0x10, uint8_t *) + screen_mode_index(scr) * 0x14;
}
static unsigned __fastcall hook_get_current_mode_index(uint8_t *scr) {
    return screen_mode_index(scr);
}

// diagnostics: terrain draw results (DX7Rasterizer::Draw(GETerrainMesh&, screen, bool))
typedef int (__thiscall *DrawTerrainFn)(void *ras, void *mesh, unsigned screen, int flag);
static DrawTerrainFn g_draw_terrain[2];
static volatile LONG g_terrain_calls, g_terrain_fail, g_terrain_last_err, g_terrain_rows;
typedef DWORD (*GetErrFn)(void);
static GetErrFn g_get_native_error;
static int g_diag_skip;   // autotest diag_skip: 1 = no terrain, 2 = no water (diagnostics)
static DrawTerrainFn g_draw_water[2];
static int __fastcall hook_draw_water(void *ras, void *edx, void *mesh, unsigned screen, int flag) {
    (void)edx;
    return g_diag_skip == 2 ? 0 : g_draw_water[DISPLAY_DLL(ras)](ras, mesh, screen, flag);
}
static int __fastcall hook_draw_terrain(void *ras, void *edx, void *mesh, unsigned screen, int flag) {
    (void)edx;
    if (g_diag_skip == 1) return 0;
    static unsigned (__thiscall *rows_fn)(void *);
    if (!rows_fn) rows_fn = (unsigned (__thiscall *)(void *))GetProcAddress(GetModuleHandleA("Low-Level Engine.dll"),
                                                                             "?GetNumVisibleTileRows@GETerrainMesh@@QBEKXZ");
    if (rows_fn) g_terrain_rows = rows_fn(mesh);
    int r = g_draw_terrain[DISPLAY_DLL(ras)](ras, mesh, screen, flag);
    InterlockedIncrement(&g_terrain_calls);
    if (r) {
        InterlockedIncrement(&g_terrain_fail);
        if (g_get_native_error) g_terrain_last_err = (LONG)g_get_native_error();
    }
    return r;
}

// diagnostics: surface copies / creation (DX7Surface::CopyTo, ::CopyWithFilter, DX7Screen::CreateSurface)
static struct { const char *name; volatile LONG calls, fail, last_err, last_ret; void *orig[2]; } g_cnt[3] = {
    { "CopyTo", 0, 0, 0, 0, { NULL, NULL } }, { "CopyWithFilter", 0, 0, 0, 0, { NULL, NULL } },
    { "CreateSurface", 0, 0, 0, 0, { NULL, NULL } },
};
static void cnt_note(int k, int r) {
    InterlockedIncrement(&g_cnt[k].calls);
    if (r) {
        InterlockedIncrement(&g_cnt[k].fail);
        g_cnt[k].last_ret = r;
        if (g_get_native_error) g_cnt[k].last_err = (LONG)g_get_native_error();
    }
}
// alpha left in copy destinations that have an alpha channel (sampled, first 400 such copies)
static volatile LONG g_alpha_copies, g_alpha_samples, g_alpha_zero, g_alpha_src_zero;
static char g_alpha_last[200];
static void sample_alpha(uint8_t *src, uint8_t *dst) {
    IDirectDrawSurface7 *d7 = dst ? FIELD(dst, 0x48, IDirectDrawSurface7 *) : NULL;
    IDirectDrawSurface7 *s7 = src ? FIELD(src, 0x48, IDirectDrawSurface7 *) : NULL;
    if (!d7 || g_alpha_copies >= 400) return;
    DDSURFACEDESC2 d = { .dwSize = sizeof d }, sd = { .dwSize = sizeof sd };
    if (d7->lpVtbl->Lock(d7, NULL, &d, DDLOCK_WAIT | DDLOCK_READONLY, NULL) != DD_OK) return;
    DWORD am = d.ddpfPixelFormat.dwRGBAlphaBitMask;
    int bpp = (int)d.ddpfPixelFormat.dwRGBBitCount / 8;
    if (am && (bpp == 2 || bpp == 4)) {
        InterlockedIncrement(&g_alpha_copies);
        for (int y = 0; y < 4; y++)
            for (int x = 0; x < 4; x++) {
                uint8_t *p = (uint8_t *)d.lpSurface + (size_t)((y * 2 + 1) * (int)d.dwHeight / 8) * d.lPitch + (size_t)((x * 2 + 1) * (int)d.dwWidth / 8) * bpp;
                DWORD v = bpp == 4 ? *(uint32_t *)p : *(uint16_t *)p;
                InterlockedIncrement(&g_alpha_samples);
                if (!(v & am)) InterlockedIncrement(&g_alpha_zero);
            }
        DDSURFACEDESC2 sl = { .dwSize = sizeof sl };   // the same sample points in the source
        if (s7 && s7->lpVtbl->Lock(s7, NULL, &sl, DDLOCK_WAIT | DDLOCK_READONLY, NULL) == DD_OK) {
            DWORD sam = sl.ddpfPixelFormat.dwRGBAlphaBitMask;
            int sb = (int)sl.ddpfPixelFormat.dwRGBBitCount / 8;
            if (sam && (sb == 2 || sb == 4))
                for (int y = 0; y < 4; y++)
                    for (int x = 0; x < 4; x++) {
                        uint8_t *p = (uint8_t *)sl.lpSurface + (size_t)((y * 2 + 1) * (int)sl.dwHeight / 8) * sl.lPitch + (size_t)((x * 2 + 1) * (int)sl.dwWidth / 8) * sb;
                        DWORD v = sb == 4 ? *(uint32_t *)p : *(uint16_t *)p;
                        if (!(v & sam)) InterlockedIncrement(&g_alpha_src_zero);
                    }
            s7->lpVtbl->Unlock(s7, NULL);
        }
        if (s7) s7->lpVtbl->GetSurfaceDesc(s7, &sd);
        snprintf(g_alpha_last, sizeof g_alpha_last, "src %lux%lu %lubpp amask %08lx caps %08lx -> dst %lux%lu %lubpp amask %08lx caps %08lx",
                 sd.dwWidth, sd.dwHeight, sd.ddpfPixelFormat.dwRGBBitCount, sd.ddpfPixelFormat.dwRGBAlphaBitMask, sd.ddsCaps.dwCaps,
                 d.dwWidth, d.dwHeight, d.ddpfPixelFormat.dwRGBBitCount, am, d.ddsCaps.dwCaps);
    }
    d7->lpVtbl->Unlock(d7, NULL);
}
static int __fastcall hook_copy_to(void *s, void *edx, void *dst, void *r1, void *r2, int f) {
    (void)edx;
    int r = ((int (__thiscall *)(void *, void *, void *, void *, int))g_cnt[0].orig[DISPLAY_DLL(s)])(s, dst, r1, r2, f);
    cnt_note(0, r);
    if (g_diag && !r) sample_alpha(s, dst);
    return r;
}
static int __fastcall hook_copy_filter(void *s, void *edx, void *dst, void *r1, void *r2) {
    (void)edx;
    int r = ((int (__thiscall *)(void *, void *, void *, void *))g_cnt[1].orig[DISPLAY_DLL(s)])(s, dst, r1, r2);
    cnt_note(1, r);
    return r;
}
// histogram of created surfaces by GESurfaceDescription type (+0) and pixel format index (+4) - after the call
// the format is the resolved one; also the DirectDraw pixel format of the last surface per bucket
static struct { unsigned type, fmt, count, w, h, bpp, amask, flags; } g_surf_kinds[24];
static uintptr_t g_surf16_stack[8];
static int __fastcall hook_create_surface(void *s, void *edx, void *desc, void **out) {
    (void)edx;
    unsigned type = ((unsigned *)desc)[0], fmt_req = ((unsigned *)desc)[1];
    if (g_diag && (type == 5 || type == 6) && (fmt_req == 2 || fmt_req == 7) && !g_surf16_stack[0]) {   // who asks for 16-bit textures
        uintptr_t *fp = (uintptr_t *)__builtin_frame_address(0);
        for (int d = 0; d < 8 && fp; d++) {
            g_surf16_stack[d] = fp[1];
            if ((uintptr_t *)fp[0] <= fp) break;
            fp = (uintptr_t *)fp[0];
        }
    }
    int r = ((int (__thiscall *)(void *, void *, void **))g_cnt[2].orig[DISPLAY_DLL(s)])(s, desc, out);
    cnt_note(2, r);
    if (g_diag && !r && out && *out) {
        IDirectDrawSurface7 *dds = FIELD(*out, 0x48, IDirectDrawSurface7 *);
        DDSURFACEDESC2 d = { .dwSize = sizeof d };
        if (dds) dds->lpVtbl->GetSurfaceDesc(dds, &d);
        for (int k = 0; k < 24; k++) {
            if (g_surf_kinds[k].count && (g_surf_kinds[k].type != type || g_surf_kinds[k].fmt != fmt_req)) continue;
            g_surf_kinds[k].type = type;
            g_surf_kinds[k].fmt = fmt_req;
            g_surf_kinds[k].count++;
            g_surf_kinds[k].w = d.dwWidth;
            g_surf_kinds[k].h = d.dwHeight;
            g_surf_kinds[k].bpp = d.ddpfPixelFormat.dwRGBBitCount;
            g_surf_kinds[k].amask = d.ddpfPixelFormat.dwRGBAlphaBitMask;
            g_surf_kinds[k].flags = d.ddpfPixelFormat.dwFlags;
            break;
        }
    }
    return r;
}

// diagnostics: render target switches (DX7Rasterizer::SetRenderTarget(GESurface*, screen), void)
typedef void (__thiscall *SetRTFn)(void *ras, void *surf, unsigned screen);
static SetRTFn g_set_rt[2];
static volatile LONG g_rt_calls, g_rt_back;
static char g_rt_last[160];
static void __fastcall hook_set_rt(void *ras, void *edx, uint8_t *surf, unsigned screen) {
    (void)edx;
    g_set_rt[DISPLAY_DLL(ras)](ras, surf, screen);
    InterlockedIncrement(&g_rt_calls);
    uint8_t *st = FIELD(ras, 0x20, uint8_t **)[screen];
    if (surf == FIELD(st, 0x2B0, uint8_t *)) { InterlockedIncrement(&g_rt_back); return; }
    IDirectDrawSurface7 *s = surf ? FIELD(surf, 0x48, IDirectDrawSurface7 *) : NULL;
    DDSURFACEDESC2 d = { .dwSize = sizeof d };
    if (s && s->lpVtbl->GetSurfaceDesc(s, &d) == DD_OK)
        snprintf(g_rt_last, sizeof g_rt_last, "%lux%lu caps=%08lx bpp=%lu", d.dwWidth, d.dwHeight, d.ddsCaps.dwCaps,
                 d.ddpfPixelFormat.dwRGBBitCount);
}

// diagnostics: Clear(color, rect, screen) / ClearZBuffer(depth, rect, screen) results and the last rect
typedef int (__thiscall *ClearFn)(void *ras, void *a, float *rect, unsigned screen);
typedef int (__thiscall *ClearZFn)(void *ras, float depth, float *rect, unsigned screen);
static ClearFn g_clear[2];
static ClearZFn g_clearz[2];
static volatile LONG g_clear_calls, g_clear_fail, g_clearz_calls, g_clearz_fail;
static float g_clear_rect[4], g_clearz_rect[4], g_clearz_depth;
static int __fastcall hook_clear(void *ras, void *edx, void *color, float *rect, unsigned screen) {
    (void)edx;
    int r = g_clear[DISPLAY_DLL(ras)](ras, color, rect, screen);
    InterlockedIncrement(&g_clear_calls);
    if (r) InterlockedIncrement(&g_clear_fail);
    memcpy(g_clear_rect, rect, sizeof g_clear_rect);
    return r;
}
static int __fastcall hook_clearz(void *ras, void *edx, float depth, float *rect, unsigned screen) {
    (void)edx;
    int r = g_clearz[DISPLAY_DLL(ras)](ras, depth, rect, screen);
    InterlockedIncrement(&g_clearz_calls);
    if (r) InterlockedIncrement(&g_clearz_fail);
    memcpy(g_clearz_rect, rect, sizeof g_clearz_rect);
    g_clearz_depth = depth;
    return r;
}

// diagnostics: fog settings (SetFog(on, screen), SetFogRange(start, end, screen), SetFogType(type, screen))
typedef void (__thiscall *SetFogFn)(void *ras, unsigned v, unsigned screen);
typedef void (__thiscall *SetFogRangeFn)(void *ras, float a, float b, unsigned screen);
static SetFogFn g_set_fog[2], g_set_fog_type[2];
static SetFogRangeFn g_set_fog_range[2];
static volatile LONG g_fog_on = -1, g_fog_type = -1, g_fog_calls;
static float g_fog_start, g_fog_end;
static void __fastcall hook_set_fog(void *ras, void *edx, unsigned on, unsigned screen) {
    (void)edx;
    g_fog_on = (LONG)on;
    InterlockedIncrement(&g_fog_calls);
    g_set_fog[DISPLAY_DLL(ras)](ras, g_diag_skip == 3 ? 0 : on, screen);
}
static void __fastcall hook_set_fog_type(void *ras, void *edx, unsigned t, unsigned screen) {
    (void)edx;
    g_fog_type = (LONG)t;
    g_set_fog_type[DISPLAY_DLL(ras)](ras, t, screen);
}
static void __fastcall hook_set_fog_range(void *ras, void *edx, float a, float b, unsigned screen) {
    (void)edx;
    g_fog_start = a;
    g_fog_end = b;
    g_set_fog_range[DISPLAY_DLL(ras)](ras, a, b, screen);
}

static int replace_rdata_ptr(HMODULE m, void *fn, void *hook) {
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)((uint8_t *)m + ((IMAGE_DOS_HEADER *)m)->e_lfanew);
    IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
    int n = 0;
    for (int k = 0; k < nt->FileHeader.NumberOfSections; k++, sec++) {
        if (sec->Characteristics & (IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_WRITE)) continue;   // .rdata only
        void **p = (void **)((uint8_t *)m + sec->VirtualAddress);
        for (DWORD j = 0; j < sec->Misc.VirtualSize / 4; j++) {
            if (p[j] != fn) continue;
            DWORD old;
            VirtualProtect(&p[j], 4, PAGE_READWRITE, &old);
            p[j] = hook;
            VirtualProtect(&p[j], 4, old, &old);
            n++;
        }
    }
    return n;
}

// GetDisplayMode: windowed (DDSCL_NORMAL), DirectDraw - Windows' emulation as well as dgVoodoo - reports a 16-bit
// display, and the engine's texture loading then produces textures without alpha (the land terrain disappears;
// fullscreen reports the 32-bit mode it set). Windowed we report the real desktop format. Hooked on every
// IDirectDraw7 the display DLLs create (DirectDrawCreateEx import), vtable slot 12.
typedef HRESULT (WINAPI *GetDisplayModeFn)(IDirectDraw7 *, DDSURFACEDESC2 *);
typedef HRESULT (WINAPI *DDCreateExFn)(GUID *, void **, REFIID, IUnknown *);
static GetDisplayModeFn g_get_display_mode;
static DDCreateExFn g_dd_create_ex;
static volatile LONG g_display_mode_fixed;

static HRESULT WINAPI hook_get_display_mode(IDirectDraw7 *dd, DDSURFACEDESC2 *d) {
    HRESULT hr = g_get_display_mode(dd, d);
    if (hr == DD_OK && !g_fullscreen && d && d->ddpfPixelFormat.dwRGBBitCount != 32) {
        HDC dc = GetDC(NULL);
        int bpp = GetDeviceCaps(dc, BITSPIXEL);
        ReleaseDC(NULL, dc);
        if (bpp == 32) {
            d->ddpfPixelFormat.dwFlags = DDPF_RGB;
            d->ddpfPixelFormat.dwRGBBitCount = 32;
            d->ddpfPixelFormat.dwRBitMask = 0xFF0000;
            d->ddpfPixelFormat.dwGBitMask = 0x00FF00;
            d->ddpfPixelFormat.dwBBitMask = 0x0000FF;
            d->ddpfPixelFormat.dwRGBAlphaBitMask = 0;
            d->lPitch = (LONG)d->dwWidth * 4;
            InterlockedIncrement(&g_display_mode_fixed);
        }
    }
    return hr;
}

static void hook_dd_vtable(void *dd) {
    void **vt = *(void ***)dd;
    if (vt[12] == (void *)hook_get_display_mode) return;
    g_get_display_mode = (GetDisplayModeFn)vt[12];
    DWORD old;
    VirtualProtect(&vt[12], 4, PAGE_EXECUTE_READWRITE, &old);
    vt[12] = (void *)hook_get_display_mode;
    VirtualProtect(&vt[12], 4, old, &old);
}

static HRESULT WINAPI hook_dd_create_ex(GUID *g, void **out, REFIID iid, IUnknown *outer) {
    HRESULT hr = g_dd_create_ex(g, out, iid, outer);
    if (hr == DD_OK && out && *out) hook_dd_vtable(*out);
    return hr;
}

// replace an import of module m (by DLL name and function name) - returns 1 when found
static int patch_import(HMODULE m, const char *dll, const char *fn, void *hook, void **orig) {
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)((uint8_t *)m + ((IMAGE_DOS_HEADER *)m)->e_lfanew);
    IMAGE_DATA_DIRECTORY dir = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dir.VirtualAddress) return 0;
    for (IMAGE_IMPORT_DESCRIPTOR *imp = (IMAGE_IMPORT_DESCRIPTOR *)((uint8_t *)m + dir.VirtualAddress); imp->Name; imp++) {
        if (lstrcmpiA((char *)m + imp->Name, dll)) continue;
        IMAGE_THUNK_DATA *names = (IMAGE_THUNK_DATA *)((uint8_t *)m + (imp->OriginalFirstThunk ? imp->OriginalFirstThunk : imp->FirstThunk));
        IMAGE_THUNK_DATA *addrs = (IMAGE_THUNK_DATA *)((uint8_t *)m + imp->FirstThunk);
        for (; names->u1.AddressOfData; names++, addrs++) {
            if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal)) continue;
            IMAGE_IMPORT_BY_NAME *ibn = (IMAGE_IMPORT_BY_NAME *)((uint8_t *)m + names->u1.AddressOfData);
            if (strcmp((char *)ibn->Name, fn)) continue;
            DWORD old;
            if (!*orig) *orig = (void *)addrs->u1.Function;
            VirtualProtect(&addrs->u1.Function, 4, PAGE_READWRITE, &old);
            addrs->u1.Function = (uintptr_t)hook;
            VirtualProtect(&addrs->u1.Function, 4, old, &old);
            return 1;
        }
    }
    return 0;
}

static int install_windowed_present(void) {
    int n = 0;
    for (int i = 0; i < 2; i++) {
        HMODULE m = GetModuleHandleA(g_show[i].dll);
        void *show = m ? (void *)GetProcAddress(m, "?ShowScene@DX7Rasterizer@@UAEJK@Z") : NULL;
        void *set_mode = m ? (void *)GetProcAddress(m, "?SetMode@DX7Screen@@UAEJK@Z") : NULL;
        if (!show || !set_mode) continue;
        IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)((uint8_t *)m + ((IMAGE_DOS_HEADER *)m)->e_lfanew);
        g_show[i].lo = (uintptr_t)m;
        g_show[i].hi = (uintptr_t)m + nt->OptionalHeader.SizeOfImage;
        g_show[i].show = (ShowSceneFn)show;
        g_show[i].set_mode = (SetModeFn)set_mode;
        n += replace_rdata_ptr(m, show, (void *)hook_show_scene) + replace_rdata_ptr(m, set_mode, (void *)hook_set_mode);
        n += patch_import(m, "DDRAW.dll", "DirectDrawCreateEx", (void *)hook_dd_create_ex, (void **)&g_dd_create_ex);
        void *gcm = (void *)GetProcAddress(m, "?GetCurrentMode@DX7Screen@@UBEABUGEVideoMode@@XZ");
        void *gcmi = (void *)GetProcAddress(m, "?GetCurrentModeIndex@DX7Screen@@UBEKXZ");
        if (gcm) n += replace_rdata_ptr(m, gcm, (void *)hook_get_current_mode);
        if (gcmi) n += replace_rdata_ptr(m, gcmi, (void *)hook_get_current_mode_index);
        void *dt = (void *)GetProcAddress(m, "?Draw@DX7Rasterizer@@UAEJAAVGETerrainMesh@@K_N@Z");
        if (dt) { g_draw_terrain[i] = (DrawTerrainFn)dt; n += replace_rdata_ptr(m, dt, (void *)hook_draw_terrain); }
        void *dw = (void *)GetProcAddress(m, "?DrawWater@DX7Rasterizer@@UAEJAAVGETerrainMesh@@K_N@Z");
        if (dw) { g_draw_water[i] = (DrawTerrainFn)dw; n += replace_rdata_ptr(m, dw, (void *)hook_draw_water); }
        g_get_native_error = (GetErrFn)GetProcAddress(GetModuleHandleA("Low-Level Engine.dll"), "?ESGetNativeError@@YAKXZ");
        static const char *cnt_exports[3] = {
            "?CopyTo@DX7Surface@@UAEJPAVGESurface@@AAVGERectangle@@1_N@Z",
            "?CopyWithFilter@DX7Surface@@UAEJPAVGESurface@@AAVGERectangle@@1@Z",
            "?CreateSurface@DX7Screen@@UAEJAAUGESurfaceDescription@@PAPAVGESurface@@@Z",
        };
        void *sf = (void *)GetProcAddress(m, "?SetFog@DX7Rasterizer@@UAEXKK@Z");
        void *sft = (void *)GetProcAddress(m, "?SetFogType@DX7Rasterizer@@UAEXKK@Z");
        void *sfr = (void *)GetProcAddress(m, "?SetFogRange@DX7Rasterizer@@UAEXMMK@Z");
        if (sf) { g_set_fog[i] = (SetFogFn)sf; n += replace_rdata_ptr(m, sf, (void *)hook_set_fog); }
        if (sft) { g_set_fog_type[i] = (SetFogFn)sft; n += replace_rdata_ptr(m, sft, (void *)hook_set_fog_type); }
        if (sfr) { g_set_fog_range[i] = (SetFogRangeFn)sfr; n += replace_rdata_ptr(m, sfr, (void *)hook_set_fog_range); }
        void *cl = (void *)GetProcAddress(m, "?Clear@DX7Rasterizer@@UAEJABVGEColor@@ABVGERectangle@@K@Z");
        void *clz = (void *)GetProcAddress(m, "?ClearZBuffer@DX7Rasterizer@@UAEJMABVGERectangle@@K@Z");
        if (cl) { g_clear[i] = (ClearFn)cl; n += replace_rdata_ptr(m, cl, (void *)hook_clear); }
        if (clz) { g_clearz[i] = (ClearZFn)clz; n += replace_rdata_ptr(m, clz, (void *)hook_clearz); }
        void *srt = (void *)GetProcAddress(m, "?SetRenderTarget@DX7Rasterizer@@MAEXPAVGESurface@@K@Z");
        if (srt) { g_set_rt[i] = (SetRTFn)srt; n += replace_rdata_ptr(m, srt, (void *)hook_set_rt); }
        void *cnt_hooks[3] = { (void *)hook_copy_to, (void *)hook_copy_filter, (void *)hook_create_surface };
        for (int k = 0; k < 3; k++) {
            void *f = (void *)GetProcAddress(m, cnt_exports[k]);
            if (f) { g_cnt[k].orig[i] = f; n += replace_rdata_ptr(m, f, cnt_hooks[k]); }
        }
    }
    return n;
}

static void display_install(const char *aimod_ini, const char *autotest_ini) {
    char mode[32], tmp[32];
    int windowed = GetPrivateProfileIntA("display", "windowed", 0, aimod_ini);
    if (autotest_ini) windowed = GetPrivateProfileIntA("autotest", "windowed", windowed, autotest_ini);
    if (autotest_ini) g_diag_skip = GetPrivateProfileIntA("autotest", "diag_skip", 0, autotest_ini);
    g_diag = (autotest_ini && GetPrivateProfileIntA("autotest", "diag_hooks", 0, autotest_ini))
             || GetPrivateProfileIntA("display", "diag", 0, aimod_ini);
    if (windowed) {
        uint8_t was = *(uint8_t *)G_FULLSCREEN;
        if (was == 1) *(uint8_t *)G_FULLSCREEN = g_fullscreen = 0;
        int hooks = install_windowed_present();
        if (g_log) fprintf(g_log, "# display: windowed %s, present hooks %d\n",
                           was == 1 ? "on" : "NOT set (unexpected fullscreen flag)", hooks);
    } else if (g_diag) {
        int hooks = install_windowed_present();   // all pass through in fullscreen; for the ddinfo counters
        if (g_log) fprintf(g_log, "# display: fullscreen, diagnostic hooks %d\n", hooks);
    }

    GetPrivateProfileStringA("editor", "resolution", "ee", mode, sizeof mode, aimod_ini);
    if (autotest_ini) {
        lstrcpynA(tmp, mode, sizeof tmp);
        GetPrivateProfileStringA("autotest", "editor_resolution", tmp, mode, sizeof mode, autotest_ini);
    }
    if (!lstrcmpiA(mode, "ee")) return;
    if (!lstrcmpiA(mode, "aoc")) { g_editor_w = 1024; g_editor_h = 768; }
    else if (!lstrcmpiA(mode, "game")) g_editor_game = 1;
    else if (sscanf(mode, "%dx%d", &g_editor_w, &g_editor_h) != 2 || g_editor_w < 640 || g_editor_h < 480) {
        if (g_log) fprintf(g_log, "# editor: bad resolution '%s' in aimod.ini, left at 800x600\n", mode);
        return;
    }
    int ok = patch_call(SITE_EDITOR_MENU, FN_SET_RESOLUTION, hook_editor_resolution)
           + patch_call(SITE_EDITOR_RES, FN_SET_RESOLUTION, hook_editor_resolution);
    int lay = g_editor_w == 1024 && g_editor_h == 768 ? apply_editor_layout() : 0;
    if (g_log) fprintf(g_log, "# editor: resolution=%s hooks %d/2, layout patches %d/%d\n", mode, ok, lay, (int)N_LAYOUT + 6);
}

// ddinfo [blit]: state of the DirectDraw presentation (for the windowed mode work). Rasterizer [0x919794],
// per-screen state +0x20[screen [0x919428]]: +0x2A8 DX7Screen (+0x30 IDirectDraw7, +0x3C attached to desktop,
// +0x3D fullscreen), +0x2AC front GESurface, +0x2B0 back GESurface (DX7Surface +0x48 IDirectDrawSurface7),
// +0x2BC GEWindow (+0 HWND, +4/+8 client origin floats, +0xC/+0x10 size). "blit" repeats ShowScene's Blt.
static int dump_surface(IDirectDrawSurface7 *s, const char *path) {
    DDSURFACEDESC2 d = { .dwSize = sizeof d };
    if (s->lpVtbl->Lock(s, NULL, &d, DDLOCK_WAIT | DDLOCK_READONLY, NULL) != DD_OK) return 0;
    int w = (int)d.dwWidth, h = (int)d.dwHeight, stride = (w * 3 + 3) & ~3, sbpp = (int)d.ddpfPixelFormat.dwRGBBitCount;
    BITMAPINFOHEADER bi = { sizeof bi, w, h, 1, 24, BI_RGB, (DWORD)(stride * h), 0, 0, 0, 0 };
    struct { BITMAPINFOHEADER h; DWORD masks[3]; } src = {
        { sizeof src.h, d.lPitch / (sbpp / 8), -h, 1, (WORD)sbpp, sbpp == 16 ? BI_BITFIELDS : BI_RGB, 0, 0, 0, 0, 0 },
        { d.ddpfPixelFormat.dwRBitMask, d.ddpfPixelFormat.dwGBitMask, d.ddpfPixelFormat.dwBBitMask },
    };
    void *bits = NULL;
    HDC mdc = CreateCompatibleDC(NULL);
    HBITMAP bmp = CreateDIBSection(mdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS, &bits, NULL, 0);
    HGDIOBJ prev = SelectObject(mdc, bmp);
    StretchDIBits(mdc, 0, 0, w, h, 0, 0, w, h, d.lpSurface, (BITMAPINFO *)&src, DIB_RGB_COLORS, SRCCOPY);
    s->lpVtbl->Unlock(s, NULL);
    FILE *f = fopen(path, "wb");
    if (f) {
        BITMAPFILEHEADER fh = { 0x4D42, (DWORD)(sizeof fh + sizeof bi + bi.biSizeImage), 0, 0, sizeof fh + sizeof bi };
        fwrite(&fh, sizeof fh, 1, f);
        fwrite(&bi, sizeof bi, 1, f);
        fwrite(bits, bi.biSizeImage, 1, f);
        fclose(f);
    }
    SelectObject(mdc, prev);
    DeleteObject(bmp);
    DeleteDC(mdc);
    return f != NULL;
}

static void dd_surface(char *out, const char *name, uint8_t *ges) {
    IDirectDrawSurface7 *s = ges ? FIELD(ges, 0x48, IDirectDrawSurface7 *) : NULL;
    if (!s) { REPLY("%s: none\n", name); return; }
    DDSURFACEDESC2 d = { .dwSize = sizeof d };
    HRESULT hr = s->lpVtbl->GetSurfaceDesc(s, &d);
    REPLY("%s: %p hr=%08lx %lux%lu caps=%08lx bpp=%lu masks=%06lx/%06lx/%06lx lost=%08lx\n", name, (void *)s, hr,
          d.dwWidth, d.dwHeight, d.ddsCaps.dwCaps, d.ddpfPixelFormat.dwRGBBitCount, d.ddpfPixelFormat.dwRBitMask,
          d.ddpfPixelFormat.dwGBitMask, d.ddpfPixelFormat.dwBBitMask, s->lpVtbl->IsLost(s));
    DDSCAPS2 zc = { .dwCaps = DDSCAPS_ZBUFFER };
    IDirectDrawSurface7 *z = NULL;
    hr = s->lpVtbl->GetAttachedSurface(s, &zc, &z);
    if (hr == DD_OK && z) {
        DDSURFACEDESC2 zd = { .dwSize = sizeof zd };
        z->lpVtbl->GetSurfaceDesc(z, &zd);
        REPLY("  z-buffer %p %lux%lu depth %lu bits caps=%08lx\n", (void *)z, zd.dwWidth, zd.dwHeight,
              zd.ddpfPixelFormat.dwZBufferBitDepth, zd.ddsCaps.dwCaps);
        DDSURFACEDESC2 ld = { .dwSize = sizeof ld };
        HRESULT lhr = z->lpVtbl->Lock(z, NULL, &ld, DDLOCK_WAIT | DDLOCK_READONLY, NULL);
        if (lhr == DD_OK) {   // sample a 5x5 grid of raw depth values
            REPLY("  z samples:");
            for (int y = 0; y < 5; y++)
                for (int x = 0; x < 5; x++) {
                    uint8_t *p = (uint8_t *)ld.lpSurface + (size_t)((y * 2 + 1) * (int)ld.dwHeight / 10) * ld.lPitch
                                 + (size_t)((x * 2 + 1) * (int)ld.dwWidth / 10) * (ld.ddpfPixelFormat.dwZBufferBitDepth / 8);
                    REPLY(" %08lx", ld.ddpfPixelFormat.dwZBufferBitDepth == 16 ? (unsigned long)*(uint16_t *)p : (unsigned long)*(uint32_t *)p);
                }
            REPLY("\n");
            z->lpVtbl->Unlock(z, NULL);
        } else REPLY("  z-buffer lock failed hr=%08lx\n", lhr);
        z->lpVtbl->Release(z);
    } else REPLY("  no z-buffer attached (hr=%08lx)\n", hr);
    IDirectDrawClipper *c = NULL;
    hr = s->lpVtbl->GetClipper(s, &c);
    if (hr == DD_OK && c) {
        HWND h = NULL;
        c->lpVtbl->GetHWnd(c, &h);
        REPLY("  clipper %p hwnd %p\n", (void *)c, (void *)h);
        c->lpVtbl->Release(c);
    } else REPLY("  no clipper (hr=%08lx)\n", hr);
}

static void cmd_ddinfo(char *out, const char *args) {
    uint8_t *ras = *(uint8_t **)0x00919794;
    int idx = *(int *)0x00919428;
    uint8_t *st = ras ? FIELD(ras, 0x20, uint8_t **)[idx] : NULL;
    if (!st) { REPLY("error: no rasterizer\n"); return; }
    uint8_t *scr = FIELD(st, 0x2A8, uint8_t *), *win = FIELD(st, 0x2BC, uint8_t *);
    REPLY("screen %p fullscreen=%d desktop=%d flag=%d renderer %s\n", (void *)scr, scr[0x3D], scr[0x3C], g_fullscreen,
          g_show[DISPLAY_DLL(ras)].dll);
    {
        void **svt = *(void ***)scr;
        char d32 = ((char (__thiscall *)(void *))svt[3])(scr), t32 = ((char (__thiscall *)(void *))svt[4])(scr);
        REPLY("32-bit device support %d, 32-bit texture support %d; screen bytes 0x30..0x40:", d32, t32);
        for (int k = 0x30; k < 0x40; k++) REPLY(" %02x", scr[k]);
        REPLY("\n");
    }
    HWND hwnd = win ? FIELD(win, 0, HWND) : NULL;
    RECT cr = {0};
    POINT o = {0, 0};
    if (hwnd) { GetClientRect(hwnd, &cr); ClientToScreen(hwnd, &o); }
    REPLY("window %p hwnd %p origin %.0f,%.0f size %ux%u; real client %ldx%ld at %ld,%ld\n", (void *)win, (void *)hwnd,
          win ? FIELD(win, 4, float) : 0, win ? FIELD(win, 8, float) : 0, win ? FIELD(win, 0xC, unsigned) : 0,
          win ? FIELD(win, 0x10, unsigned) : 0, cr.right, cr.bottom, o.x, o.y);
    uint8_t *front = FIELD(st, 0x2AC, uint8_t *), *back = FIELD(st, 0x2B0, uint8_t *);
    dd_surface(out, "front", front);
    dd_surface(out, "back ", back);
    GetErrFn ge = (GetErrFn)GetProcAddress(GetModuleHandleA("Low-Level Engine.dll"), "?ESGetNativeError@@YAKXZ");
    if (ge) REPLY("last native error %08lx\n", ge());
    REPLY("terrain draws: %ld, failed %ld, last error %08lx, visible tile rows %ld\n", g_terrain_calls, g_terrain_fail,
          g_terrain_last_err, g_terrain_rows);
    REPLY("Clear: %ld calls, %ld failed, rect %.0f %.0f %.0f %.0f; ClearZBuffer: %ld calls, %ld failed, depth %g rect %.0f %.0f %.0f %.0f\n",
          g_clear_calls, g_clear_fail, g_clear_rect[0], g_clear_rect[1], g_clear_rect[2], g_clear_rect[3],
          g_clearz_calls, g_clearz_fail, g_clearz_depth, g_clearz_rect[0], g_clearz_rect[1], g_clearz_rect[2], g_clearz_rect[3]);
    for (int k = 0; k < 24 && g_surf_kinds[k].count; k++)
        REPLY("surfaces type %u fmt %u: %u created, last %ux%u %u bpp alpha mask %08x pf flags %08x\n", g_surf_kinds[k].type,
              g_surf_kinds[k].fmt, g_surf_kinds[k].count, g_surf_kinds[k].w, g_surf_kinds[k].h, g_surf_kinds[k].bpp,
              g_surf_kinds[k].amask, g_surf_kinds[k].flags);
    REPLY("copies into alpha surfaces: %ld, alpha samples %ld, zero alpha %ld (in the sources %ld); last: %s\n", g_alpha_copies,
          g_alpha_samples, g_alpha_zero, g_alpha_src_zero, g_alpha_last);
    if (g_surf16_stack[0]) {
        REPLY("first 16-bit texture requested from:");
        for (int d = 0; d < 8 && g_surf16_stack[d]; d++) {
            HMODULE hm = NULL;
            char mp[MAX_PATH] = "?";
            if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   (LPCSTR)g_surf16_stack[d], &hm))
                GetModuleFileNameA(hm, mp, sizeof mp);
            const char *b = strrchr(mp, 0x5C);
            REPLY(" %s+%lx", b ? b + 1 : mp, (unsigned long)(g_surf16_stack[d] - (uintptr_t)hm));
        }
        REPLY("\n");
    }
    REPLY("fog: on=%ld type=%ld range %g..%g (%ld SetFog calls)\n", g_fog_on, g_fog_type, g_fog_start, g_fog_end, g_fog_calls);
    REPLY("SetRenderTarget: %ld calls, %ld to the back buffer, last other target: %s\n", g_rt_calls, g_rt_back, g_rt_last);
    for (int k = 0; k < 3; k++)
        REPLY("%s: %ld calls, %ld failed, last ret %ld err %08lx\n", g_cnt[k].name, g_cnt[k].calls, g_cnt[k].fail,
              g_cnt[k].last_ret, g_cnt[k].last_err);
    IDirectDraw7 *dd = FIELD(scr, 0x30, IDirectDraw7 *);
    if (dd) {
        DDSURFACEDESC2 m = { .dwSize = sizeof m };
        HRESULT hr = dd->lpVtbl->GetDisplayMode(dd, &m);
        DEVMODEA dm = { .dmSize = sizeof dm };
        EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
        REPLY("display mode reported as 32-bit %ld times (hook %s)\n", g_display_mode_fixed,
              (*(void ***)dd)[12] == (void *)hook_get_display_mode ? "on" : "OFF");
        REPLY("ddraw display mode: hr=%08lx %lux%lu %lu bpp; real desktop %lux%lu %lu bpp; mode index %u of %u\n", hr,
              m.dwWidth, m.dwHeight, m.ddpfPixelFormat.dwRGBBitCount, dm.dmPelsWidth, dm.dmPelsHeight, dm.dmBitsPerPel,
              FIELD(scr, 0x38, unsigned), (unsigned)((FIELD(scr, 0x14, uintptr_t) - FIELD(scr, 0x10, uintptr_t)) / 0x14));
        uint32_t *md = (uint32_t *)(FIELD(scr, 0x10, uint8_t *) + FIELD(scr, 0x38, unsigned) * 0x14);
        REPLY("current GEVideoMode: %u %u %u %u %u\n", md[0], md[1], md[2], md[3], md[4]);
    }
    REPLY("windowed present: %ld frames, %ld failed; back buffer before ShowScene: %ld samples, %ld lit of 1024 avg, lock failed %ld\n",
          g_present_frames, g_present_fail, g_lit_frames, g_lit_frames ? g_lit_samples / g_lit_frames : 0, g_lock_fail);
    if (!strncmp(args, "blit", 4) && front && back) {
        IDirectDrawSurface7 *f = FIELD(front, 0x48, IDirectDrawSurface7 *), *b = FIELD(back, 0x48, IDirectDrawSurface7 *);
        RECT dst = { o.x, o.y, o.x + cr.right, o.y + cr.bottom };
        HRESULT hr = f->lpVtbl->Blt(f, &dst, b, NULL, DDBLT_WAIT, NULL);
        REPLY("blit to %ld,%ld-%ld,%ld: hr=%08lx\n", dst.left, dst.top, dst.right, dst.bottom, hr);
    }
    if (!strncmp(args, "notex", 5)) {   // engine debug switch: draw without textures
        void (__thiscall *fn)(void *, int, unsigned) = (void (__thiscall *)(void *, int, unsigned))
            GetProcAddress(GetModuleHandleA(g_show[DISPLAY_DLL(ras)].dll), "?SetDisplayNoTextureMode@DX7Rasterizer@@UAEX_NK@Z");
        if (fn) fn(ras, args[5] != '0', (unsigned)idx);
        REPLY("no-texture mode %s\n", fn ? (args[5] != '0' ? "on" : "off") : "not found");
    }
    if (!strncmp(args, "dump", 4)) {   // surfaces to aimod_dumps/dd_<name>.bmp
        uint8_t *ges[2] = { front, back };
        const char *names[2] = { "front", "back" };
        for (int i = 0; i < 2; i++) {
            IDirectDrawSurface7 *s = ges[i] ? FIELD(ges[i], 0x48, IDirectDrawSurface7 *) : NULL;
            char path[MAX_PATH];
            snprintf(path, sizeof path, "%s/dd_%s.bmp", g_dump_dir, names[i]);
            REPLY("dump %s: %s\n", names[i], s && dump_surface(s, path) ? path : "failed");
        }
        if (hwnd) {   // what the window shows (its DWM redirection surface, readable even when covered)
            char path[MAX_PATH];
            snprintf(path, sizeof path, "%s/dd_window.bmp", g_dump_dir);
            int w = cr.right, h = cr.bottom, stride = (w * 3 + 3) & ~3;
            BITMAPINFOHEADER bi = { sizeof bi, w, h, 1, 24, BI_RGB, (DWORD)(stride * h), 0, 0, 0, 0 };
            void *bits = NULL;
            HDC wdc = GetDC(hwnd), mdc = CreateCompatibleDC(wdc);
            HBITMAP bmp = CreateDIBSection(mdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS, &bits, NULL, 0);
            HGDIOBJ prev = SelectObject(mdc, bmp);
            BitBlt(mdc, 0, 0, w, h, wdc, 0, 0, SRCCOPY);
            FILE *f = fopen(path, "wb");
            if (f) {
                BITMAPFILEHEADER fh = { 0x4D42, (DWORD)(sizeof fh + sizeof bi + bi.biSizeImage), 0, 0, sizeof fh + sizeof bi };
                fwrite(&fh, sizeof fh, 1, f);
                fwrite(&bi, sizeof bi, 1, f);
                fwrite(bits, bi.biSizeImage, 1, f);
                fclose(f);
            }
            SelectObject(mdc, prev);
            DeleteObject(bmp);
            DeleteDC(mdc);
            ReleaseDC(hwnd, wdc);
            REPLY("dump window: %s\n", f ? path : "failed");
        }
    }
}
