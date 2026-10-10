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
    DDSURFACEDESC2 d = { .dwSize = sizeof d };
    HDC sdc;
    if (s->lpVtbl->GetSurfaceDesc(s, &d) != DD_OK || s->lpVtbl->GetDC(s, &sdc) != DD_OK) {
        InterlockedIncrement(&g_present_fail);
        return 0;
    }
    HDC wdc = GetDC(hwnd);
    RECT cr;
    GetClientRect(hwnd, &cr);
    if (cr.right == (LONG)d.dwWidth && cr.bottom == (LONG)d.dwHeight)
        BitBlt(wdc, 0, 0, cr.right, cr.bottom, sdc, 0, 0, SRCCOPY);
    else {
        SetStretchBltMode(wdc, COLORONCOLOR);
        StretchBlt(wdc, 0, 0, cr.right, cr.bottom, sdc, 0, 0, d.dwWidth, d.dwHeight, SRCCOPY);
    }
    ReleaseDC(hwnd, wdc);
    s->lpVtbl->ReleaseDC(s, sdc);
    InterlockedIncrement(&g_present_frames);
    return 1;
}

static int __fastcall hook_show_scene(void *ras, void *edx, unsigned screen) {
    (void)edx;
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
    }
    return n;
}

static void display_install(const char *aimod_ini, const char *autotest_ini) {
    char mode[32], tmp[32];
    int windowed = GetPrivateProfileIntA("display", "windowed", 0, aimod_ini);
    if (autotest_ini) windowed = GetPrivateProfileIntA("autotest", "windowed", windowed, autotest_ini);
    if (windowed) {
        uint8_t was = *(uint8_t *)G_FULLSCREEN;
        if (was == 1) *(uint8_t *)G_FULLSCREEN = g_fullscreen = 0;
        int hooks = install_windowed_present();
        if (g_log) fprintf(g_log, "# display: windowed %s, present hooks %d\n",
                           was == 1 ? "on" : "NOT set (unexpected fullscreen flag)", hooks);
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
static void dd_surface(char *out, const char *name, uint8_t *ges) {
    IDirectDrawSurface7 *s = ges ? FIELD(ges, 0x48, IDirectDrawSurface7 *) : NULL;
    if (!s) { REPLY("%s: none\n", name); return; }
    DDSURFACEDESC2 d = { .dwSize = sizeof d };
    HRESULT hr = s->lpVtbl->GetSurfaceDesc(s, &d);
    REPLY("%s: %p hr=%08lx %lux%lu caps=%08lx bpp=%lu masks=%06lx/%06lx/%06lx lost=%08lx\n", name, (void *)s, hr,
          d.dwWidth, d.dwHeight, d.ddsCaps.dwCaps, d.ddpfPixelFormat.dwRGBBitCount, d.ddpfPixelFormat.dwRBitMask,
          d.ddpfPixelFormat.dwGBitMask, d.ddpfPixelFormat.dwBBitMask, s->lpVtbl->IsLost(s));
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
    REPLY("screen %p fullscreen=%d desktop=%d flag=%d\n", (void *)scr, scr[0x3D], scr[0x3C], g_fullscreen);
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
    typedef DWORD (*GetErrFn)(void);
    GetErrFn ge = (GetErrFn)GetProcAddress(GetModuleHandleA("Low-Level Engine.dll"), "?ESGetNativeError@@YAKXZ");
    if (ge) REPLY("last native error %08lx\n", ge());
    REPLY("windowed present: %ld frames, %ld failed\n", g_present_frames, g_present_fail);
    if (!strncmp(args, "blit", 4) && front && back) {
        IDirectDrawSurface7 *f = FIELD(front, 0x48, IDirectDrawSurface7 *), *b = FIELD(back, 0x48, IDirectDrawSurface7 *);
        RECT dst = { o.x, o.y, o.x + cr.right, o.y + cr.bottom };
        HRESULT hr = f->lpVtbl->Blt(f, &dst, b, NULL, DDBLT_WAIT, NULL);
        REPLY("blit to %ld,%ld-%ld,%ld: hr=%08lx\n", dst.left, dst.top, dst.right, dst.bottom, hr);
    }
}
