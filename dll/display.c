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

// The editor layout for any resolution from 800x600 up: Art of Conquest's 1024x768 values (generated from a
// comparison of both exes, scripts/layout_diff.py + gen_editor_layout.py), each moved with the screen size the way
// AoC moved it from EE's 800x600: anchored right/bottom, stretched, or centred. Computed when the editor is entered.
#include "editor_layout.h"
#define N_LAYOUT (sizeof k_editor_layout / sizeof *k_editor_layout)
static uint32_t g_layout_consts[N_LAYOUT];
static int8_t g_layout_state[N_LAYOUT];   // 0 unchecked, 1 patched, -1 didn't match (skipped)
static int g_buttons_state;

static void write_code32(uint32_t *p, uint32_t v) {
    DWORD old;
    VirtualProtect(p, 4, PAGE_EXECUTE_READWRITE, &old);
    *p = v;
    VirtualProtect(p, 4, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 4);
}

static int apply_editor_layout(int w, int h) {
    int ok = 0, bad = 0;
    for (size_t i = 0; i < N_LAYOUT; i++) {
        uint32_t *p = (uint32_t *)k_editor_layout[i].addr;
        if (!g_layout_state[i]) {   // first time: the original value must be there
            uint32_t *c = k_editor_layout[i].kind ? (uint32_t *)*p : p;
            int match = k_editor_layout[i].kind
                ? (uintptr_t)c >= 0x401000 && (uintptr_t)c < 0xA00000 && *c == k_editor_layout[i].old
                : *p == k_editor_layout[i].old;
            g_layout_state[i] = match ? 1 : -1;
            if (match && k_editor_layout[i].kind) write_code32(p, (uint32_t)(uintptr_t)&g_layout_consts[i]);
        }
        if (g_layout_state[i] < 0) { bad++; continue; }
        float v = k_editor_layout[i].aoc + k_editor_layout[i].fx * 0.5f * (float)(w - 1024)
                + k_editor_layout[i].fy * 0.5f * (float)(h - 768);
        uint32_t bits;
        if (k_editor_layout[i].isf) memcpy(&bits, &v, 4);
        else bits = (uint32_t)(int32_t)lroundf(v);
        if (k_editor_layout[i].kind) g_layout_consts[i] = bits;
        else if (*p != bits) write_code32(p, bits);
        ok++;
    }
    // the file buttons (load, save, test, exit ...) come from a data table, not code: 0x8AE0A0, 6 entries of
    // {id, x, y, texture u0, v0, u1, v1, tooltip}; AoC (0x8C47B0) has them at x 647.. (step 35), y 736 for 1024x768
    int32_t *t = (int32_t *)0x008AE0A0;
    if (!g_buttons_state) g_buttons_state = t[1] == 415 && t[2] == 567 ? 1 : -1;
    if (g_buttons_state > 0)
        for (int i = 0; i < 6; i++, t += 8) {
            t[1] = 647 + 35 * i + (w - 1024);
            t[2] = 736 + (h - 768);
            ok++;
        }
    else bad++;
    if (g_log && bad) fprintf(g_log, "# editor: %d layout patch sites did not match and were skipped\n", bad);
    return ok;
}

static void editor_size(int *w, int *h) {
    int nw = g_editor_game ? *(int *)G_GAME_RES_W : g_editor_w;
    int nh = g_editor_game ? *(int *)G_GAME_RES_H : g_editor_h;
    if (nw >= 800 && nh >= 600) { *w = nw; *h = nh; }
}

typedef void (__stdcall *SetResolutionFn)(int w, int h, int force, int flag);
static void __stdcall hook_editor_resolution(int w, int h, int force, int flag) {
    editor_size(&w, &h);
    int lay = apply_editor_layout(w, h);
    if (g_log) { fprintf(g_log, "# editor: resolution %dx%d, layout patches %d/%d\n", w, h, lay, (int)N_LAYOUT + 6); fflush(g_log); }
    ((SetResolutionFn)FN_SET_RESOLUTION)(w, h, force, flag);
}

// The editor's main form (constructor 0x58D3E1) is built once, on first use (0x65B9F5) - before the resolution
// switch - and keeps its layout; its sub-pages are built later. The layout is computed right before it.
#define SITE_EDITOR_FORM 0x0065BA13
#define FN_EDITOR_FORM   0x0058D3E1
static void *__fastcall hook_editor_form(void *form, void *edx, void *owner) {
    (void)edx;
    int w = 800, h = 600;
    editor_size(&w, &h);
    int lay = apply_editor_layout(w, h);
    if (g_log) { fprintf(g_log, "# editor: building the editor for %dx%d, layout patches %d/%d\n", w, h, lay, (int)N_LAYOUT + 6); fflush(g_log); }
    return ((void *(__thiscall *)(void *, void *))FN_EDITOR_FORM)(form, owner);
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
static int g_present_every;   // autotest present_every (diagnostics)
static volatile DWORD g_render_thread;   // thread that calls ShowScene (profile render)
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

// Borderless fullscreen and DPI. aimod.ini [display] windowed = borderless: a frameless window over the whole
// monitor; Alt+Enter switches between that and a normal window. The game keeps rendering at its resolution
// and the image is scaled into the window (aspect kept, black bars). dpi_aware = 1 (default): the window is
// created DPI-aware (CreateWindowExA in the display DLLs, per-thread context), so Windows doesn't stretch it
// (blurry at 125/150 % display scaling) and its pixels are real screen pixels.
// Headless tests (autotest headless=1): the window is created off-screen, never activates and ignores focus
// changes; the game cursor and clicks come from the control pipe (vmouse/vclick) and screenshots are taken from
// the presented frame (shot) - nothing on the user's screen, mouse or keyboard is touched.
static int g_headless;
static volatile LONG g_shot_req, g_shot_done;
static char g_shot_path[MAX_PATH];
static int write_desc_bmp(const DDSURFACEDESC2 *d, const char *path);
static int g_borderless;   // current mode: 1 borderless, 0 normal window
static int g_smooth;       // [display] smooth = 1: filtered scaling (HALFTONE) instead of nearest pixel
static int g_dpi_windows;
static void window_hook(HWND hwnd);
typedef void *(WINAPI *SetThreadDpiFn)(void *);
typedef void *(WINAPI *GetWindowDpiFn)(HWND);
static SetThreadDpiFn g_set_thread_dpi;
static GetWindowDpiFn g_get_window_dpi;
// window geometry from the game's (DPI-unaware) threads in the window's own DPI context, else a DPI-aware
// window is reported scaled down
static void *dpi_enter(HWND h) { return g_set_thread_dpi && g_get_window_dpi ? g_set_thread_dpi(g_get_window_dpi(h)) : NULL; }
static void dpi_leave(void *prev) { if (prev) g_set_thread_dpi(prev); }

typedef HWND (WINAPI *CreateWindowExAFn)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
static HWND WINAPI hook_create_window(DWORD ex, LPCSTR cls, LPCSTR name, DWORD style, int x, int y, int w, int h,
                                      HWND parent, HMENU menu, HINSTANCE inst, LPVOID param) {
    if (g_headless && (style & WS_CAPTION)) {   // the game window: off-screen, never activated
        ex |= WS_EX_NOACTIVATE;
        x = GetSystemMetrics(SM_XVIRTUALSCREEN) - 8000;
        y = 0;
    }
    void *prev = g_set_thread_dpi ? g_set_thread_dpi((void *)(intptr_t)-4) : NULL;   // PER_MONITOR_AWARE_V2
    if (!prev && g_set_thread_dpi) prev = g_set_thread_dpi((void *)(intptr_t)-3);     // PER_MONITOR_AWARE
    HWND r = CreateWindowExA(ex, cls, name, style, x, y, w, h, parent, menu, inst, param);
    if (prev) { g_set_thread_dpi(prev); InterlockedIncrement((volatile LONG *)&g_dpi_windows); }
    return r;
}

static int patch_iat(HMODULE m, const char *dll, const char *fn, void *hook) {
    uint8_t *b = (uint8_t *)m;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(b + ((IMAGE_DOS_HEADER *)b)->e_lfanew);
    IMAGE_DATA_DIRECTORY dd = nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (!dd.VirtualAddress) return 0;
    for (IMAGE_IMPORT_DESCRIPTOR *d = (IMAGE_IMPORT_DESCRIPTOR *)(b + dd.VirtualAddress); d->Name; d++) {
        if (lstrcmpiA((char *)(b + d->Name), dll) || !d->OriginalFirstThunk) continue;
        IMAGE_THUNK_DATA *names = (IMAGE_THUNK_DATA *)(b + d->OriginalFirstThunk), *iat = (IMAGE_THUNK_DATA *)(b + d->FirstThunk);
        for (; names->u1.AddressOfData; names++, iat++) {
            if (IMAGE_SNAP_BY_ORDINAL(names->u1.Ordinal)) continue;
            if (strcmp((char *)((IMAGE_IMPORT_BY_NAME *)(b + names->u1.AddressOfData))->Name, fn)) continue;
            DWORD old;
            if (!VirtualProtect(&iat->u1.Function, sizeof iat->u1.Function, PAGE_READWRITE, &old)) return 0;
            iat->u1.Function = (uintptr_t)hook;
            VirtualProtect(&iat->u1.Function, sizeof iat->u1.Function, old, &old);
            return 1;
        }
    }
    return 0;
}

// where the game image goes in the client area: scaled to fit, aspect kept
static RECT view_rect(const RECT *cr, int w, int h) {
    RECT r = *cr;
    if (w <= 0 || h <= 0) return r;
    if ((long long)cr->right * h > (long long)cr->bottom * w) {   // client wider: bars left and right
        int vw = (int)((long long)cr->bottom * w / h);
        r.left = (cr->right - vw) / 2;
        r.right = r.left + vw;
    } else {
        int vh = (int)((long long)cr->right * h / w);
        r.top = (cr->bottom - vh) / 2;
        r.bottom = r.top + vh;
    }
    return r;
}

// smooth = 1: bilinear scaling of the 32-bit frame on the CPU, rows split over worker threads, then a 1:1 copy.
// GDI's HALFTONE costs ~33 ms per frame at 2560x1440 -> 2405x1353 (measured with ddpoke bench), this a few ms.
static struct {
    const uint8_t *src; int sw, sh, spitch;
    uint32_t *dst; int dw, dh;
    int *xi; uint32_t *xw;   // per destination column: source column, weight 0..256
} g_scale;
static HANDLE g_scale_go[8], g_scale_done[8];
static SRWLOCK g_scale_lock = SRWLOCK_INIT;   // held by the caller from scale_frame until its buffer is copied
static int g_scale_threads;

static void scale_rows(int y0, int y1) {
    const int sw = g_scale.sw, sh = g_scale.sh, dw = g_scale.dw;
    for (int y = y0; y < y1; y++) {
        int fy = (int)(((long long)y * (sh - 1) << 8) / (g_scale.dh > 1 ? g_scale.dh - 1 : 1));
        int sy = fy >> 8, wy = fy & 255;
        if (sy >= sh - 1) { sy = sh - 2 < 0 ? 0 : sh - 2; wy = sh > 1 ? 256 : 0; }
        const uint32_t *r0 = (const uint32_t *)(g_scale.src + (size_t)sy * g_scale.spitch);
        const uint32_t *r1 = sh > 1 ? (const uint32_t *)(g_scale.src + (size_t)(sy + 1) * g_scale.spitch) : r0;
        uint32_t *out = g_scale.dst + (size_t)y * dw;
        for (int x = 0; x < dw; x++) {
            int sx = g_scale.xi[x];
            uint32_t wx = g_scale.xw[x], a = r0[sx], b = r0[sx + 1 < sw ? sx + 1 : sx], c = r1[sx], e = r1[sx + 1 < sw ? sx + 1 : sx];
            uint32_t trb = ((a & 0xFF00FF) * (256 - wx) + (b & 0xFF00FF) * wx) >> 8 & 0xFF00FF;
            uint32_t tg = ((a & 0xFF00) * (256 - wx) + (b & 0xFF00) * wx) >> 8 & 0xFF00;
            uint32_t brb = ((c & 0xFF00FF) * (256 - wx) + (e & 0xFF00FF) * wx) >> 8 & 0xFF00FF;
            uint32_t bg = ((c & 0xFF00) * (256 - wx) + (e & 0xFF00) * wx) >> 8 & 0xFF00;
            uint32_t rb = ((trb * (256 - wy) + brb * wy) >> 8) & 0xFF00FF;
            uint32_t g = ((tg * (256 - wy) + bg * wy) >> 8) & 0xFF00;
            out[x] = rb | g;
        }
    }
}

static DWORD WINAPI scale_worker(void *arg) {
    int i = (int)(intptr_t)arg, n = g_scale_threads + 1;
    for (;;) {
        WaitForSingleObject(g_scale_go[i], INFINITE);
        scale_rows(g_scale.dh * (i + 1) / n, g_scale.dh * (i + 2) / n);
        SetEvent(g_scale_done[i]);
    }
}

// scales the locked frame (32 bpp) to dw x dh into our buffer; returns it, or NULL (then GDI scales)
static uint32_t *scale_frame(const DDSURFACEDESC2 *d, int dw, int dh) {
    static uint32_t *buf;
    static int bw, bh, bsw;
    if (d->ddpfPixelFormat.dwRGBBitCount != 32 || dw < 2 || dh < 2) return NULL;
    if (!g_scale_threads) {
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        int n = (int)si.dwNumberOfProcessors - 1;
        g_scale_threads = n < 1 ? -1 : n > 7 ? 7 : n;   // -1: no workers
        for (int i = 0; i < g_scale_threads; i++) {
            g_scale_go[i] = CreateEventA(NULL, FALSE, FALSE, NULL);
            g_scale_done[i] = CreateEventA(NULL, FALSE, FALSE, NULL);
            CloseHandle(CreateThread(NULL, 0, scale_worker, (void *)(intptr_t)i, 0, NULL));
        }
    }
    int sw = (int)d->dwWidth, sh = (int)d->dwHeight;
    if (dw != bw || dh != bh || sw != bsw) {
        free(buf); free(g_scale.xi); free(g_scale.xw);
        buf = malloc((size_t)dw * dh * 4);
        g_scale.xi = malloc(sizeof(int) * dw);
        g_scale.xw = malloc(sizeof(uint32_t) * dw);
        if (!buf || !g_scale.xi || !g_scale.xw) { bw = 0; return NULL; }
        for (int x = 0; x < dw; x++) {
            int fx = (int)(((long long)x * (sw - 1) << 8) / (dw - 1));
            g_scale.xi[x] = fx >> 8;
            g_scale.xw[x] = (uint32_t)(fx & 255);
        }
        bw = dw; bh = dh; bsw = sw;
    }
    g_scale.src = d->lpSurface; g_scale.sw = sw; g_scale.sh = sh; g_scale.spitch = (int)d->lPitch;
    g_scale.dst = buf; g_scale.dw = dw; g_scale.dh = dh;
    int n = g_scale_threads > 0 ? g_scale_threads : 0;
    for (int i = 0; i < n; i++) SetEvent(g_scale_go[i]);
    scale_rows(0, dh / (n + 1));
    if (n) WaitForMultipleObjects((DWORD)n, g_scale_done, TRUE, INFINITE);
    return buf;
}

#include "present_d3d.c"

// timing of the windowed present (ddinfo): readback (Lock), copy to the window (GDI or Direct3D), frame interval
static volatile LONG64 g_t_lock, g_t_blit, g_t_frame;
static volatile LONG g_t_count;
static LONGLONG qpc(void) { LARGE_INTEGER t; QueryPerformanceCounter(&t); return t.QuadPart; }

static int present_to_window(uint8_t *ras, unsigned screen) {
    uint8_t *st = FIELD(ras, 0x20, uint8_t **)[screen];
    uint8_t *scr = FIELD(st, 0x2A8, uint8_t *), *back = FIELD(st, 0x2B0, uint8_t *), *win = FIELD(st, 0x2BC, uint8_t *);
    if (!scr || scr[0x3D] || !back || !win) return 0;
    IDirectDrawSurface7 *s = FIELD(back, 0x48, IDirectDrawSurface7 *);
    HWND hwnd = FIELD(win, 0, HWND);
    if (!s || !hwnd) return 0;
    // the z-buffer of a new windowed back buffer starts out unusable for the terrain (it fails the depth test while
    // models draw); one Lock/Unlock syncs it and it stays fine - found by elimination (ddpoke z)
    static IDirectDrawSurface7 *z_synced_for;
    if (z_synced_for != s) {
        DDSCAPS2 zc = { .dwCaps = DDSCAPS_ZBUFFER };
        IDirectDrawSurface7 *z = NULL;
        if (s->lpVtbl->GetAttachedSurface(s, &zc, &z) == DD_OK && z) {
            DDSURFACEDESC2 zd = { .dwSize = sizeof zd };
            if (z->lpVtbl->Lock(z, NULL, &zd, DDLOCK_WAIT | DDLOCK_READONLY, NULL) == DD_OK) z->lpVtbl->Unlock(z, NULL);
            z->lpVtbl->Release(z);
        }
        z_synced_for = s;
    }
    // Lock, not GetDC: the GDI view of the 3D render target reads back black under current Windows' DirectDraw
    DDSURFACEDESC2 d = { .dwSize = sizeof d };
    static LONGLONG t_last;
    LONGLONG t0 = qpc();
    if (s->lpVtbl->Lock(s, NULL, &d, DDLOCK_WAIT | DDLOCK_READONLY, NULL) != DD_OK) {
        InterlockedIncrement(&g_present_fail);
        return 0;
    }
    LONGLONG t1 = qpc();
    if (g_shot_req) {   // control pipe "shot": the finished frame as BMP
        write_desc_bmp(&d, g_shot_path);
        g_shot_req = 0;
        InterlockedIncrement(&g_shot_done);
    }
    int bpp = (int)d.ddpfPixelFormat.dwRGBBitCount;
    struct { BITMAPINFOHEADER h; DWORD masks[3]; } bi = {
        { sizeof bi.h, d.lPitch / (bpp / 8), -(LONG)d.dwHeight, 1, (WORD)bpp, bpp == 16 ? BI_BITFIELDS : BI_RGB, 0, 0, 0, 0, 0 },
        { d.ddpfPixelFormat.dwRBitMask, d.ddpfPixelFormat.dwGBitMask, d.ddpfPixelFormat.dwBBitMask },
    };
    static HWND erase_off;
    if (erase_off != hwnd) {   // no background erase between our frames (flicker)
        SetClassLongA(hwnd, GCL_HBRBACKGROUND, 0);
        erase_off = hwnd;
    }
    window_hook(hwnd);
    void *dpi = dpi_enter(hwnd);
    RECT cr;
    GetClientRect(hwnd, &cr);
    RECT v = view_rect(&cr, (int)d.dwWidth, (int)d.dwHeight);
    if (present_d3d(&d, hwnd, &cr, &v)) {
        LONGLONG t2 = qpc();
        if (t_last) {
            InterlockedAdd64(&g_t_lock, t1 - t0);
            InterlockedAdd64(&g_t_blit, t2 - t1);
            InterlockedAdd64(&g_t_frame, t0 - t_last);
            InterlockedIncrement(&g_t_count);
        }
        t_last = t0;
        dpi_leave(dpi);
        ValidateRect(hwnd, NULL);
        s->lpVtbl->Unlock(s, NULL);
        InterlockedIncrement(&g_present_frames);
        return 1;
    }
    HDC wdc = GetDC(hwnd);
    if (v.left > 0 || v.top > 0) {   // bars
        HBRUSH black = (HBRUSH)GetStockObject(BLACK_BRUSH);
        RECT b1 = { 0, 0, v.left > 0 ? v.left : cr.right, v.left > 0 ? cr.bottom : v.top };
        RECT b2 = { v.left > 0 ? v.right : 0, v.left > 0 ? 0 : v.bottom, cr.right, cr.bottom };
        FillRect(wdc, &b1, black);
        FillRect(wdc, &b2, black);
    }
    int vw = v.right - v.left, vh = v.bottom - v.top;
    int want_scale = g_smooth && (vw != (int)d.dwWidth || vh != (int)d.dwHeight);
    if (want_scale) AcquireSRWLockExclusive(&g_scale_lock);
    uint32_t *scaled = want_scale ? scale_frame(&d, vw, vh) : NULL;
    if (scaled) {
        BITMAPINFOHEADER sb = { sizeof sb, vw, -vh, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
        SetDIBitsToDevice(wdc, v.left, v.top, (DWORD)vw, (DWORD)vh, 0, 0, 0, (UINT)vh, scaled, (BITMAPINFO *)&sb, DIB_RGB_COLORS);
    }
    if (want_scale) ReleaseSRWLockExclusive(&g_scale_lock);
    if (!scaled) {
        SetStretchBltMode(wdc, COLORONCOLOR);
        StretchDIBits(wdc, v.left, v.top, vw, vh, 0, 0, (int)d.dwWidth, (int)d.dwHeight, d.lpSurface,
                      (BITMAPINFO *)&bi, DIB_RGB_COLORS, SRCCOPY);
    }
    LONGLONG t2 = qpc();
    if (t_last) {
        InterlockedAdd64(&g_t_lock, t1 - t0);
        InterlockedAdd64(&g_t_blit, t2 - t1);
        InterlockedAdd64(&g_t_frame, t0 - t_last);
        InterlockedIncrement(&g_t_count);
    }
    t_last = t0;
    ReleaseDC(hwnd, wdc);
    dpi_leave(dpi);
    ValidateRect(hwnd, NULL);   // the frame is drawn - no WM_PAINT/erase for it
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

// The frame statistics the original ShowScene keeps before presenting (DX7HRDisplay +0xBADD; the TnL variant is
// the same class), read by the FPS display. Per-screen state: +0x32C frames, +0x328/+0x330/+0x338 counters,
// +0x2C4 float sum; every 12 frames the averages (+0x2C8 +0x2D0 +0x2DC +0x2E4) and the rates per second over
// TMGetTime (ms): +0x2CC, +0x2D4, +0x2D8 = frames per second, +0x2E0; then all counters restart.
typedef DWORD (__cdecl *TMGetTimeFn)(void);
static TMGetTimeFn g_tm_get_time;
static void frame_stats(uint8_t *ras, unsigned screen) {
    uint8_t *st = FIELD(ras, 0x20, uint8_t **)[screen];
    if (!st || ++FIELD(st, 0x32C, unsigned) != 12) return;
    if (!g_tm_get_time) g_tm_get_time = (TMGetTimeFn)(void *)GetProcAddress(GetModuleHandleA("Low-Level Engine.dll"), "?TMGetTime@@YAKXZ");
    if (!g_tm_get_time) return;
    float sum = FIELD(st, 0x2C4, float);
    unsigned a = FIELD(st, 0x328, unsigned), frames = FIELD(st, 0x32C, unsigned), b = FIELD(st, 0x330, unsigned), c = FIELD(st, 0x338, unsigned);
    FIELD(st, 0x2C8, float) = sum / 12.0f;
    FIELD(st, 0x2D0, float) = (float)a / 12.0f;
    FIELD(st, 0x2DC, float) = (float)b / 12.0f;
    FIELD(st, 0x2E4, float) = (float)c / 12.0f;
    DWORD now = g_tm_get_time(), dt = now - FIELD(st, 0x334, DWORD);
    float k = dt ? 1000.0f / (float)dt : 0.0f;
    FIELD(st, 0x2CC, float) = sum * k;
    FIELD(st, 0x2D4, float) = (float)a * k;
    FIELD(st, 0x2D8, float) = (float)frames * k;
    FIELD(st, 0x2E0, float) = (float)b * k;
    FIELD(st, 0x334, DWORD) = now;
    FIELD(st, 0x2C4, float) = 0.0f;
    FIELD(st, 0x328, unsigned) = FIELD(st, 0x32C, unsigned) = FIELD(st, 0x330, unsigned) = FIELD(st, 0x338, unsigned) = 0;
}

static int __fastcall hook_show_scene(void *ras, void *edx, unsigned screen) {
    (void)edx;
    g_render_thread = GetCurrentThreadId();
    if (g_fullscreen) return g_show[DISPLAY_DLL(ras)].show(ras, screen);
    // windowed: the original is not called. Its Blt to the emulated primary at the window's screen position is
    // rejected while the back buffer is larger than that 1024x768 surface, but at 1024x768 (scenario editor) the
    // rect only overhangs by the title bar height and the copy runs past the end of the surface (crash in DDRAW.dll).
    // Its frame statistics (FPS display) are kept here instead.
    frame_stats(ras, screen);
    static unsigned skip_n;   // autotest present_every=N: diagnostics, present only every Nth frame
    if (g_present_every > 1 && ++skip_n % (unsigned)g_present_every) return 0;
    if (g_diag && (g_present_frames & 63) == 0) sample_back(ras, screen);
    return present_to_window(ras, screen) ? 0 : 1;
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
static volatile LONG g_front_copies_skipped;
static int __fastcall hook_copy_to(void *s, void *edx, void *dst, void *r1, void *r2, int f) {
    (void)edx;
    // windowed: the UI's "copy dirty rectangles" restore regions from the front surface - after a page flip the
    // back buffer holds an older frame. Windowed the front is the emulated primary that never gets our frames
    // (garbage/scanlines in the editor where the mouse moved) and the back buffer is never flipped, so it still
    // holds the last frame: copies from or to the front are skipped.
    if (!g_fullscreen) {
        uint8_t *ras = *(uint8_t **)0x00919794;
        uint8_t *st = ras ? FIELD(ras, 0x20, uint8_t **)[*(int *)0x00919428] : NULL;
        uint8_t *front = st ? FIELD(st, 0x2AC, uint8_t *) : NULL;
        if (front && (s == front || dst == front)) { InterlockedIncrement(&g_front_copies_skipped); return 0; }
    }
    int r =((int (__thiscall *)(void *, void *, void *, void *, int))g_cnt[0].orig[DISPLAY_DLL(s)])(s, dst, r1, r2, f);
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

// Windowed input. Fullscreen-era behaviour that breaks a window:
// - the mouse is a DirectInput device in exclusive mode (ISMouse::Resume, Low-Level Engine.dll +0x4C837
//   SetCooperativeLevel(hwnd, 5 = EXCLUSIVE|FOREGROUND)): Windows hides its cursor and gets no mouse messages,
//   so the title bar can't be clicked. Windowed we take it non-exclusive (6), like the keyboard already is.
// - the game cursor (RTSMouse [0x9197A8], +4/+8 position floats, x range +0x18..+0x1C, y +0x20..+0x14) moves
//   by the raw DirectInput deltas, scaled by the desktop size and divided by the smallest delta seen so far
//   (0x510E5D, "MinXValue") - not the Windows pointer speed/acceleration. Windowed it follows the real cursor:
//   client position mapped to the game's range (the window may be scaled), wheel still from the original.
// - the window procedure 0x6622DD hides the cursor on every WM_SETCURSOR (also over the title bar and when the
//   window is in the background) and minimizes the window (CloseWindow, 0x662670) when the app loses focus.
// - non-exclusive, the device sees every click while the window is in front, also on the title bar or other
//   windows: the mouse is unacquired while the cursor is outside the client area (and no button is held).
// aimod.ini [display] lock_mouse = 1 keeps the cursor inside the window while the game is in front.
#define FN_MOUSE_UPDATE  0x00510E5D
#define FN_MOUSE_CLAMP   0x00510DAD
#define G_GAME_WINDOW    0x009197A4   // GEWindow* (+0 HWND)
#define IMP_IS_MOUSE     0x008235CC   // import: &ISMouse* in Low-Level Engine.dll
#define SITE_CLOSE_WINDOW_JE 0x00662666   // je over the CloseWindow call on WM_ACTIVATEAPP(0)
typedef void (__thiscall *ThisFn)(void *);
static ThisFn g_recompute_buttons;   // ISMouse::RecomputeCurrentButtonState
static int g_lock_mouse;
static WNDPROC g_game_wndproc;
static HWND g_subclassed;
static volatile LONG g_mouse_updates, g_mouse_unacq, g_mouse_reacq;

static void os_cursor_visible(void) {   // display count back to 0 for this thread
    int c = ShowCursor(TRUE);
    while (c < 0) c = ShowCursor(TRUE);
    while (c > 0) c = ShowCursor(FALSE);
}

// The window's size: borderless = the monitor, set here on every change (WM_WINDOWPOSCHANGING) except while
// minimized; normal = client of the game resolution, which the game sets itself (GEWindow::Resize:
// AdjustWindowRect + SetWindowPos; it stores the new size at +0xC/+0x10 only after the call, so it can't be
// read during the change). Resize runs on DPI-unaware threads, so for a DPI-aware window its SetWindowPos is
// wrapped (hook_set_window_pos): the requested client size is kept in real pixels.
#define WM_AIMOD_MODE (WM_APP + 0x2E45)
typedef UINT (WINAPI *GetDpiFn)(HWND);
typedef BOOL (WINAPI *AdjustForDpiFn)(RECT *, DWORD, BOOL, DWORD, UINT);
static GetDpiFn g_get_dpi;
static AdjustForDpiFn g_adjust_for_dpi;
static RECT frame_for_dpi(UINT dpi) {   // WS_OVERLAPPEDWINDOW frame around an empty client
    RECT f = { 0 };
    if (!g_get_dpi) {
        HMODULE u = GetModuleHandleA("user32.dll");
        g_get_dpi = (GetDpiFn)(void *)GetProcAddress(u, "GetDpiForWindow");
        g_adjust_for_dpi = (AdjustForDpiFn)(void *)GetProcAddress(u, "AdjustWindowRectExForDpi");
    }
    if (g_adjust_for_dpi) g_adjust_for_dpi(&f, WS_OVERLAPPEDWINDOW, FALSE, 0, dpi);
    else AdjustWindowRect(&f, WS_OVERLAPPEDWINDOW, FALSE);
    return f;
}
static UINT window_dpi(HWND h) { frame_for_dpi(96); return g_get_dpi ? g_get_dpi(h) : 96; }

// a normal window with client cw x ch (the game resolution), in the window's DPI context. It always fits the
// monitor's work area: a resolution as big as the screen would make the window larger than the desktop, which
// Windows cuts and dgVoodoo then crashes on (DDRAW.dll+2B133) - the client is scaled down (aspect kept), the
// image is scaled into it. Position: kept (centered when coming from borderless), moved inside the work area.
static RECT normal_window_rect(HWND h, int cw, int ch) {
    MONITORINFO mi = { .cbSize = sizeof mi };
    GetMonitorInfoA(MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST), &mi);
    RECT f = frame_for_dpi(window_dpi(h)), wr, wa = mi.rcWork;
    int fw = f.right - f.left, fh = f.bottom - f.top;
    int maxw = wa.right - wa.left - fw, maxh = wa.bottom - wa.top - fh;
    if (cw > maxw || ch > maxh) {
        if ((long long)cw * maxh > (long long)ch * maxw) { ch = (int)((long long)ch * maxw / cw); cw = maxw; }
        else { cw = (int)((long long)cw * maxh / ch); ch = maxh; }
    }
    int w = cw + fw, hh = ch + fh;
    GetWindowRect(h, &wr);
    int x = wr.left, y = wr.top;
    if (GetWindowLongA(h, GWL_STYLE) & WS_POPUP) {   // coming from borderless: centered
        x = wa.left + (wa.right - wa.left - w) / 2;
        y = wa.top + (wa.bottom - wa.top - hh) / 2;
    }
    if (x + w > wa.right) x = wa.right - w;
    if (y + hh > wa.bottom) y = wa.bottom - hh;
    if (x < wa.left) x = wa.left;
    if (y < wa.top) y = wa.top;   // title bar reachable
    return (RECT){ x, y, x + w, y + hh };
}

static RECT window_rect_for(HWND h) {
    if (g_borderless) {
        MONITORINFO mi = { .cbSize = sizeof mi };
        GetMonitorInfoA(MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST), &mi);
        return mi.rcMonitor;
    }
    uint8_t *win = *(uint8_t **)G_GAME_WINDOW;
    return normal_window_rect(h, win ? FIELD(win, 0xC, int) : 1024, win ? FIELD(win, 0x10, int) : 768);
}

static void apply_window_mode(HWND h) {
    if (IsIconic(h)) ShowWindow(h, SW_RESTORE);
    if (IsZoomed(h)) ShowWindow(h, SW_RESTORE);
    RECT r = window_rect_for(h);   // before the style change (reads where it comes from)
    LONG st = GetWindowLongA(h, GWL_STYLE);
    st = g_borderless ? (st & ~WS_OVERLAPPEDWINDOW) | WS_POPUP : (st & ~WS_POPUP) | WS_OVERLAPPEDWINDOW;
    SetWindowLongA(h, GWL_STYLE, st);
    SetWindowPos(h, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
    if (g_log) { fprintf(g_log, "# display: %s %ldx%ld\n", g_borderless ? "borderless" : "window", r.right - r.left, r.bottom - r.top); fflush(g_log); }
}

static LRESULT CALLBACK window_proc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    if (g_headless) {   // the game never learns about focus: no suspend, no pause, no activation
        if (msg == WM_ACTIVATEAPP || msg == WM_ACTIVATE || msg == WM_SETCURSOR) return msg == WM_SETCURSOR;
        if (msg == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    }
    switch (msg) {
    case WM_AIMOD_MODE:
        apply_window_mode(h);
        return 0;
    case WM_SYSKEYDOWN:   // Alt+Enter: borderless <-> window
        if (w == VK_RETURN && (l & (1 << 29))) {
            if (!(l & (1 << 30))) { g_borderless = !g_borderless; apply_window_mode(h); }
            return 0;
        }
        break;
    case WM_SYSCHAR:
        if (w == '\r') return 0;   // no beep
        break;
    case WM_WINDOWPOSCHANGING: {
        WINDOWPOS *p = (WINDOWPOS *)l;
        LONG st = GetWindowLongA(h, GWL_STYLE);
        if (!g_borderless || (st & WS_MINIMIZE) || p->x <= -32000) break;
        if ((p->flags & SWP_NOSIZE) && (p->flags & SWP_NOMOVE)) break;
        RECT r = window_rect_for(h);
        p->x = r.left;
        p->y = r.top;
        p->flags &= ~SWP_NOMOVE;
        p->cx = r.right - r.left;
        p->cy = r.bottom - r.top;
        p->flags &= ~SWP_NOSIZE;
        break;
    }
    case WM_SETCURSOR:   // game cursor in the client area while in front, Windows' cursor everywhere else
        if (LOWORD(l) == HTCLIENT && GetForegroundWindow() == h) { SetCursor(NULL); return TRUE; }
        os_cursor_visible();
        if (LOWORD(l) == HTCLIENT) { SetCursor(LoadCursorA(NULL, (LPCSTR)IDC_ARROW)); return TRUE; }
        return DefWindowProcA(h, msg, w, l);
    case WM_ACTIVATE: {   // the original returns without DefWindowProc, which would set the keyboard focus
        LRESULT r = CallWindowProcA(g_game_wndproc, h, msg, w, l);
        if (LOWORD(w) != WA_INACTIVE && GetFocus() != h) SetFocus(h);
        return r;
    }
    case WM_ACTIVATEAPP:
        if (!w) ClipCursor(NULL);
        break;
    }
    return CallWindowProcA(g_game_wndproc, h, msg, w, l);
}

static void window_hook(HWND hwnd) {   // subclass once per window (present and mouse threads both call this)
    static volatile LONG busy;
    if (g_subclassed == hwnd || InterlockedExchange(&busy, 1)) return;
    if (g_subclassed != hwnd) {
        g_game_wndproc = (WNDPROC)GetWindowLongA(hwnd, GWL_WNDPROC);
        SetWindowLongA(hwnd, GWL_WNDPROC, (LONG)(uintptr_t)window_proc);
        g_subclassed = hwnd;
        if (g_borderless) PostMessageA(hwnd, WM_AIMOD_MODE, 0, 0);   // initial style/size, on the window's thread
    }
    busy = 0;
}

// Low-Level Engine's SetWindowPos (only GEWindow::Resize): its size comes from AdjustWindowRect at 96 DPI on an
// unaware thread; for the DPI-aware game window the client it means is rebuilt at the window's DPI and set in
// the window's context, so Windows doesn't scale it
static BOOL WINAPI hook_set_window_pos(HWND h, HWND after, int x, int y, int cx, int cy, UINT flags) {
    if ((flags & SWP_NOSIZE) || (GetWindowLongA(h, GWL_STYLE) & WS_POPUP) || IsIconic(h) || IsZoomed(h))
        return SetWindowPos(h, after, x, y, cx, cy, flags);
    RECT f96 = frame_for_dpi(96);   // the game's AdjustWindowRect ran on an unaware thread: 96 DPI frame
    int cw = cx - (f96.right - f96.left), ch = cy - (f96.bottom - f96.top);
    void *prev = dpi_enter(h);
    RECT r = normal_window_rect(h, cw, ch);
    if (g_headless) flags |= SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER;   // fitted size, but stays off-screen
    else flags &= ~SWP_NOMOVE;
    BOOL ok = SetWindowPos(h, after, r.left, r.top, r.right - r.left, r.bottom - r.top, flags);
    dpi_leave(prev);
    return ok;
}

static uint8_t *is_mouse(void) {
    uint8_t **p = *(uint8_t ***)IMP_IS_MOUSE;
    return p ? *p : NULL;
}

// clicks outside the client area must not reach the game; Acquire again re-reads the button state
static void mouse_acquire(int want) {
    static int unacquired, up_frames;
    uint8_t *ms = is_mouse();
    if (!ms) return;
    void **dev = FIELD(ms, 0x44, void **);
    if (!dev || ms[4]) { unacquired = 0; return; }   // +4: suspended by the game (window inactive)
    typedef HRESULT (__stdcall *DevFn)(void *);
    if (want) {
        up_frames = 0;
        if (unacquired) {
            ((DevFn)(*(void ***)dev)[7])(dev);   // Acquire
            if (g_recompute_buttons) g_recompute_buttons(ms);
            unacquired = 0;
            InterlockedIncrement(&g_mouse_reacq);
        }
    } else if (!unacquired && ++up_frames > 2) {   // a release just before leaving is still read first
        ((DevFn)(*(void ***)dev)[8])(dev);   // Unacquire
        unacquired = 1;
        InterlockedIncrement(&g_mouse_unacq);
    }
}

// The devices are resumed on WM_ACTIVATEAPP/WM_ACTIVATE, but Acquire (DISCL_FOREGROUND) fails while Windows hasn't
// made the window foreground yet - a click into the window then leaves mouse and keyboard dead until it's minimized
// and restored. The intro sequence (0x538xxx) also ends with Suspend. In front, a suspended device is resumed.
#define IMP_IS_KEYBOARD  0x00823AF8   // import: &ISKeyboard* in Low-Level Engine.dll
static volatile LONG g_input_resumes;
static void resume_if_suspended(uintptr_t imp, DWORD *last) {
    uint8_t **p = *(uint8_t ***)imp, *d = p ? *p : NULL;
    if (!d || !d[4] || GetTickCount() - *last < 250) return;   // +4: suspended
    *last = GetTickCount();
    ((ThisFn)(*(void ***)d)[4])(d);   // Resume (vtable +0x10)
    InterlockedIncrement(&g_input_resumes);
}

// virtual mouse (headless): position in game coordinates from the control pipe; a click increments the button's
// transition counter (ISMouse +0x18 + button: odd = down; JustPressed compares it with last frame's copy +0x1C,
// which Refresh takes just before this hook) - down now, up 3 updates later. The device is never acquired
// (the window is never in front), so no real input interferes.
static volatile LONG g_vm_x = -1, g_vm_y = -1, g_vm_button = -1, g_vm_step, g_vm_clicks;
static void virtual_mouse(uint8_t *m, int frozen) {
    if (g_vm_x >= 0 && !frozen) {
        FIELD(m, 4, float) = (float)g_vm_x;
        FIELD(m, 8, float) = (float)g_vm_y;
        ((ThisFn)FN_MOUSE_CLAMP)(m);
    }
    uint8_t *ms = is_mouse();
    LONG b = g_vm_button;
    if (!ms || b < 0 || b > 3) return;
    uint8_t *st = ms + 0x18 + b;
    static DWORD down_at;
    LONG step = g_vm_step++;
    if (step == 0) {                                // down, held ~150 ms like a real click
        if (!(*st & 1)) (*st)++;
        down_at = GetTickCount();
    } else if (GetTickCount() - down_at >= 150) {
        if (*st & 1) (*st)++;                      // up
        g_vm_button = -1;
        InterlockedIncrement(&g_vm_clicks);
    }
}

static void window_mouse_in(uint8_t *m, int frozen, uint8_t *win, HWND hwnd);
static void window_mouse(uint8_t *m, int frozen) {
    uint8_t *win = *(uint8_t **)G_GAME_WINDOW;
    HWND hwnd = win ? FIELD(win, 0, HWND) : NULL;
    if (!hwnd) return;
    window_hook(hwnd);
    if (g_headless) { virtual_mouse(m, frozen); return; }
    void *dpi = dpi_enter(hwnd);
    window_mouse_in(m, frozen, win, hwnd);
    dpi_leave(dpi);
}

static void window_mouse_in(uint8_t *m, int frozen, uint8_t *win, HWND hwnd) {
    POINT pt, c;
    RECT rc;
    if (!GetCursorPos(&pt) || !GetClientRect(hwnd, &rc) || rc.right < 2 || rc.bottom < 2) return;
    c = pt;
    ScreenToClient(hwnd, &c);
    int fg = GetForegroundWindow() == hwnd;
    int inside = PtInRect(&rc, c);
    int held = ((GetAsyncKeyState(VK_LBUTTON) | GetAsyncKeyState(VK_RBUTTON) | GetAsyncKeyState(VK_MBUTTON)) & 0x8000) != 0;
    if (fg && !IsIconic(hwnd)) {
        static DWORD last_mouse, last_keyboard;
        resume_if_suspended(IMP_IS_MOUSE, &last_mouse);
        resume_if_suspended(IMP_IS_KEYBOARD, &last_keyboard);
    }
    mouse_acquire(inside || held);
    if (g_lock_mouse && fg && inside) {
        RECT want = rc, cur;
        MapWindowPoints(hwnd, NULL, (POINT *)&want, 2);
        if (!GetClipCursor(&cur) || !EqualRect(&cur, &want)) ClipCursor(&want);
    }
    if (frozen || !fg) return;
    if (!inside && !held && WindowFromPoint(pt) == hwnd) return;   // on the title bar or frame: leave it
    // client position -> game resolution (GEWindow +0xC/+0x10; the window may be scaled). The cursor range
    // (+0x18..) is only the clamp: while dragging in the 3D view the game narrows it to the map area.
    // Outside the window (or dragging out of it) the position clamps to the edge: edge scrolling works by
    // pushing past the border; the last pixel maps to the full width (clamped to end - 1, like fullscreen)
    unsigned gw = FIELD(win, 0xC, unsigned), gh = FIELD(win, 0x10, unsigned);
    if (gw < 2 || gh < 2) return;
    RECT v = view_rect(&rc, (int)gw, (int)gh);   // borderless: the image may have bars
    if (v.right - v.left < 2 || v.bottom - v.top < 2) return;
    FIELD(m, 4, float) = (float)(c.x - v.left) * (float)gw / (float)(v.right - v.left - 1);
    FIELD(m, 8, float) = (float)(c.y - v.top) * (float)gh / (float)(v.bottom - v.top - 1);
    ((ThisFn)FN_MOUSE_CLAMP)(m);
    InterlockedIncrement(&g_mouse_updates);
}

static void __fastcall hook_mouse_update(uint8_t *m, void *edx) {
    (void)edx;
    uint8_t frozen = m[0xA4];
    m[0xA4] = 1;   // the original then only reads the wheel
    ((ThisFn)FN_MOUSE_UPDATE)(m);
    m[0xA4] = frozen;
    window_mouse(m, frozen);
}

static int patch_byte(uintptr_t at, uint8_t expected, uint8_t val) {
    uint8_t *p = (uint8_t *)at;
    DWORD old;
    if (*p != expected || !VirtualProtect(p, 1, PAGE_EXECUTE_READWRITE, &old)) return 0;
    *p = val;
    VirtualProtect(p, 1, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 1);
    return 1;
}

static int install_windowed_input(void) {
    int n = 0;
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
    if (lle) {
        uint8_t *b = (uint8_t *)lle;
        // push 5 -> push 6 before SetCooperativeLevel: ISMouse::Resume and the device setup in ISInitialize
        if (b[0x4C837] == 0x6A) n += patch_byte((uintptr_t)b + 0x4C838, 5, 6);
        if (b[0x4A76E] == 0x6A) n += patch_byte((uintptr_t)b + 0x4A76F, 5, 6);
        g_recompute_buttons = (ThisFn)(void *)GetProcAddress(lle, "?RecomputeCurrentButtonState@ISMouse@@AAEXXZ");
    }
    static const uintptr_t sites[] = { 0x004B8ABB, 0x0050FEC8, 0x005111CF, 0x006618B8 };
    for (size_t i = 0; i < sizeof sites / sizeof *sites; i++) n += patch_call(sites[i], FN_MOUSE_UPDATE, hook_mouse_update);
    n += patch_byte(SITE_CLOSE_WINDOW_JE, 0x74, 0xEB);   // no minimize on focus loss
    return n;
}

static void display_install(const char *aimod_ini, const char *autotest_ini) {
    char mode[32], tmp[32];
    // [display] mode = native | windowed | borderless (or 0 | 1 | 2); the older key windowed = 0 | 1 | borderless
    // is read when mode is missing; autotest: mode (or windowed)
    char wmode[32];
    GetPrivateProfileStringA("display", "windowed", "0", tmp, sizeof tmp, aimod_ini);
    GetPrivateProfileStringA("display", "mode", tmp, wmode, sizeof wmode, aimod_ini);
    if (autotest_ini) {
        lstrcpynA(tmp, wmode, sizeof tmp);
        GetPrivateProfileStringA("autotest", "windowed", tmp, wmode, sizeof wmode, autotest_ini);
        lstrcpynA(tmp, wmode, sizeof tmp);
        GetPrivateProfileStringA("autotest", "mode", tmp, wmode, sizeof wmode, autotest_ini);
    }
    int dmode = !lstrcmpiA(wmode, "borderless") ? 2 : !lstrcmpiA(wmode, "windowed") || !lstrcmpiA(wmode, "window") ? 1
              : !lstrcmpiA(wmode, "native") || !lstrcmpiA(wmode, "fullscreen") ? 0 : atoi(wmode);
    if (dmode < 0 || dmode > 2) {
        if (g_log) fprintf(g_log, "# display: unknown mode '%s' in aimod.ini, using native\n", wmode);
        dmode = 0;
    }
    if (autotest_ini) g_headless = GetPrivateProfileIntA("autotest", "headless", 0, autotest_ini);
    if (g_headless) dmode = 1;   // a normal (off-screen) window
    g_borderless = dmode == 2;
    int windowed = dmode != 0;
    if (autotest_ini) g_present_every = GetPrivateProfileIntA("autotest", "present_every", 0, autotest_ini);
    if (autotest_ini) g_diag_skip = GetPrivateProfileIntA("autotest", "diag_skip", 0, autotest_ini);
    g_diag = (autotest_ini && GetPrivateProfileIntA("autotest", "diag_hooks", 0, autotest_ini))
             || GetPrivateProfileIntA("display", "diag", 0, aimod_ini);
    if (windowed) {
        uint8_t was = *(uint8_t *)G_FULLSCREEN;
        if (was == 1) *(uint8_t *)G_FULLSCREEN = g_fullscreen = 0;
        int hooks = install_windowed_present();
        g_lock_mouse = GetPrivateProfileIntA("display", "lock_mouse", 0, aimod_ini);
        int input = install_windowed_input();
        g_smooth = GetPrivateProfileIntA("display", "smooth", 0, aimod_ini);
        GetPrivateProfileStringA("display", "present", "d3d", tmp, sizeof tmp, aimod_ini);
        if (autotest_ini) { char at[32]; lstrcpynA(at, tmp, sizeof at); GetPrivateProfileStringA("autotest", "present", at, tmp, sizeof tmp, autotest_ini); }
        g_present_d3d = lstrcmpiA(tmp, "gdi") != 0;
        g_vsync = GetPrivateProfileIntA("display", "vsync", 0, aimod_ini);
        int dpi = 0;
        if (GetPrivateProfileIntA("display", "dpi_aware", 1, aimod_ini)) {
            HMODULE u = GetModuleHandleA("user32.dll");
            g_set_thread_dpi = (SetThreadDpiFn)(void *)GetProcAddress(u, "SetThreadDpiAwarenessContext");
            g_get_window_dpi = (GetWindowDpiFn)(void *)GetProcAddress(u, "GetWindowDpiAwarenessContext");
            if (!g_set_thread_dpi || !g_get_window_dpi) g_set_thread_dpi = NULL, g_get_window_dpi = NULL;
        }
        for (int i = 0; i < 2 && (g_set_thread_dpi || g_headless); i++) {
            HMODULE m = GetModuleHandleA(g_show[i].dll);
            if (m) dpi += patch_iat(m, "USER32.dll", "CreateWindowExA", (void *)hook_create_window);
        }
        HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
        int fit = lle ? patch_iat(lle, "USER32.dll", "SetWindowPos", (void *)hook_set_window_pos) : 0;
        // loading splash: started (0x5343F8) while the flag is still 1 - before the DLLs load - but its stop
        // (0x534433) checks the flag again and would leave it on screen; stop it in windowed mode too
        uint8_t *jne = (uint8_t *)0x0053443D;
        int splash = 0;
        if (jne[0] == 0x75 && jne[1] == 0x19) {
            DWORD old;
            VirtualProtect(jne, 2, PAGE_EXECUTE_READWRITE, &old);
            jne[0] = jne[1] = 0x90;
            VirtualProtect(jne, 2, old, &old);
            FlushInstructionCache(GetCurrentProcess(), jne, 2);
            splash = 1;
        }
        if (g_log && g_headless) fprintf(g_log, "# display: headless (off-screen window, virtual mouse)\n");
        if (g_log) fprintf(g_log, "# display: windowed %s (%s), present hooks %d, input patches %d/7, dpi hooks %d/2, window fit %s, splash stop %s\n",
                           was == 1 ? "on" : "NOT set (unexpected fullscreen flag)", g_borderless ? "borderless" : "window",
                           hooks, input, dpi, fit ? "ok" : "FAILED", splash ? "ok" : "FAILED");
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
    else if (sscanf(mode, "%dx%d", &g_editor_w, &g_editor_h) != 2 || g_editor_w < 800 || g_editor_h < 600) {
        if (g_log) fprintf(g_log, "# editor: bad resolution '%s' in aimod.ini, left at 800x600\n", mode);
        return;
    }
    int ok = patch_call(SITE_EDITOR_MENU, FN_SET_RESOLUTION, hook_editor_resolution)
           + patch_call(SITE_EDITOR_RES, FN_SET_RESOLUTION, hook_editor_resolution)
           + patch_call(SITE_EDITOR_FORM, FN_EDITOR_FORM, hook_editor_form);
    if (g_log) fprintf(g_log, "# editor: resolution=%s hooks %d/3\n", mode, ok);
}

// ddinfo [blit]: state of the DirectDraw presentation (for the windowed mode work). Rasterizer [0x919794],
// per-screen state +0x20[screen [0x919428]]: +0x2A8 DX7Screen (+0x30 IDirectDraw7, +0x3C attached to desktop,
// +0x3D fullscreen), +0x2AC front GESurface, +0x2B0 back GESurface (DX7Surface +0x48 IDirectDrawSurface7),
// +0x2BC GEWindow (+0 HWND, +4/+8 client origin floats, +0xC/+0x10 size). "blit" repeats ShowScene's Blt.
static int dump_surface(IDirectDrawSurface7 *s, const char *path) {
    DDSURFACEDESC2 d = { .dwSize = sizeof d };
    if (s->lpVtbl->Lock(s, NULL, &d, DDLOCK_WAIT | DDLOCK_READONLY, NULL) != DD_OK) return 0;
    int r = write_desc_bmp(&d, path);
    s->lpVtbl->Unlock(s, NULL);
    return r;
}

static int write_desc_bmp(const DDSURFACEDESC2 *dp, const char *path) {   // locked surface -> 24-bit BMP
    const DDSURFACEDESC2 d = *dp;
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

// profile [ms]: sampling profiler (pipe thread). Every ~1 ms each other thread of the game is suspended for a moment
// and its EIP recorded; reports the busiest threads and code addresses (module+offset, 64-byte buckets).
// profile <ms> render: only the thread that calls ShowScene, waits included; each sample is put on the innermost
// return address on its stack that lies in the game's own code (exe, Low-Level Engine, DX7 display DLLs) - the game
// function that waits or works - and the top frame's module is counted (where the time actually goes)
static int is_game_code(uintptr_t a) {
    HMODULE m = NULL;
    if (a < 0x400000 || !GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)a, &m)) return 0;
    return m == GetModuleHandleA(NULL) || m == GetModuleHandleA("Low-Level Engine.dll") || m == GetModuleHandleA("DX7HRDisplay.dll")
        || m == GetModuleHandleA("DX7HRTnLDisplay.dll");
}
static void cmd_profile_render(int dur, char *out) {
    DWORD tid = g_render_thread;
    HANDLE h = tid ? OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, tid) : NULL;
    if (!h) { REPLY("error: render thread unknown (windowed mode only)\n"); return; }
    enum { HB = 4096 };
    static struct { uintptr_t a; int n; } callers[HB];
    struct { HMODULE m; int n; } tops[24];
    int nt = 0, samples = 0;
    memset(callers, 0, sizeof callers);
    DWORD end = GetTickCount() + (DWORD)dur;
    timeBeginPeriod(1);
    while ((LONG)(end - GetTickCount()) > 0) {
        if (SuspendThread(h) != (DWORD)-1) {
            CONTEXT c = { .ContextFlags = CONTEXT_CONTROL };
            uintptr_t stack[512];
            SIZE_T got = 0;
            BOOL ok = GetThreadContext(h, &c);
            if (ok) ReadProcessMemory(GetCurrentProcess(), (void *)(uintptr_t)c.Esp, stack, sizeof stack, &got);
            ResumeThread(h);
            if (ok) {
                samples++;
                HMODULE m = NULL;
                GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)(uintptr_t)c.Eip, &m);
                int j = 0;
                while (j < nt && tops[j].m != m) j++;
                if (j == nt && nt < 24) { tops[nt].m = m; tops[nt].n = 0; nt++; }
                if (j < nt) tops[j].n++;
                uintptr_t caller = is_game_code(c.Eip) ? c.Eip : 0;
                for (size_t i = 0; !caller && i < got / sizeof *stack; i++) if (is_game_code(stack[i])) caller = stack[i];
                uintptr_t a = caller & ~(uintptr_t)15;
                unsigned k = (unsigned)(a >> 4) * 2654435761u % HB;
                for (int p = 0; p < HB; p++, k = (k + 1) % HB)
                    if (!callers[k].a || callers[k].a == a) { callers[k].a = a; callers[k].n++; break; }
            }
        }
        Sleep(1);
    }
    timeEndPeriod(1);
    CloseHandle(h);
    REPLY("render thread %lu: %d samples over %d ms\n  where the time is (top frame):\n", tid, samples, dur);
    for (int j = 0; j < nt; j++) {
        char path[MAX_PATH] = "?";
        if (tops[j].m) GetModuleFileNameA(tops[j].m, path, sizeof path);
        const char *b = strrchr(path, 0x5C);
        if (tops[j].n * 50 >= samples) REPLY("    %-28s %3d%%\n", b ? b + 1 : path, samples ? tops[j].n * 100 / samples : 0);
    }
    REPLY("  game code it is in or called from (16-byte buckets):\n");
    for (int r = 0; r < 25; r++) {
        int best = -1;
        for (int k = 0; k < HB; k++) if (callers[k].n > 0 && (best < 0 || callers[k].n > callers[best].n)) best = k;
        if (best < 0) break;
        char at[160];
        if (callers[best].a) crash_addr(at, sizeof at, callers[best].a); else lstrcpyA(at, "(no game code on the stack)");
        REPLY("    %5.1f%%  %s\n", samples ? callers[best].n * 100.0 / samples : 0.0, at);
        callers[best].n = -callers[best].n;
    }
}

static void cmd_profile(const char *args, char *out) {
    int dur = atoi(args);
    if (dur <= 0) dur = 2000;
    if (strstr(args, "render")) { cmd_profile_render(dur, out); return; }
    enum { MAXT = 64, HB = 8192 };
    struct { DWORD id; HANDLE h; int samples, busy; } th[MAXT];
    int nt = 0;
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    THREADENTRY32 te = { .dwSize = sizeof te };
    DWORD pid = GetCurrentProcessId(), self = GetCurrentThreadId();
    if (snap != INVALID_HANDLE_VALUE && Thread32First(snap, &te))
        do {
            if (te.th32OwnerProcessID != pid || te.th32ThreadID == self || nt >= MAXT) continue;
            HANDLE h = OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, te.th32ThreadID);
            if (h) { th[nt].id = te.th32ThreadID; th[nt].h = h; th[nt].samples = th[nt].busy = 0; nt++; }
        } while (Thread32Next(snap, &te));
    if (snap != INVALID_HANDLE_VALUE) CloseHandle(snap);
    static struct { uintptr_t a; int n, t; } hist[HB];
    memset(hist, 0, sizeof hist);
    HMODULE ntdll = GetModuleHandleA("ntdll.dll"), win32u = GetModuleHandleA("win32u.dll");
    DWORD end = GetTickCount() + (DWORD)dur;
    int rounds = 0;
    timeBeginPeriod(1);
    while ((LONG)(end - GetTickCount()) > 0) {
        for (int i = 0; i < nt; i++) {
            if (SuspendThread(th[i].h) == (DWORD)-1) continue;
            CONTEXT c = { .ContextFlags = CONTEXT_CONTROL };
            BOOL ok = GetThreadContext(th[i].h, &c);
            ResumeThread(th[i].h);
            if (!ok) continue;
            th[i].samples++;
            HMODULE m = NULL;
            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)(uintptr_t)c.Eip, &m);
            if (m == ntdll || m == win32u) continue;   // waiting (mostly)
            th[i].busy++;
            uintptr_t a = c.Eip & ~(uintptr_t)63;
            unsigned k = (unsigned)(a >> 6) * 2654435761u % HB;
            for (int p = 0; p < HB; p++, k = (k + 1) % HB)
                if (!hist[k].a || hist[k].a == a) { hist[k].a = a; hist[k].n++; hist[k].t = i; break; }
        }
        rounds++;
        Sleep(1);
    }
    timeEndPeriod(1);
    REPLY("profile: %d rounds over %d ms, %d threads (busy = not in ntdll/win32u)\n", rounds, dur, nt);
    for (int i = 0; i < nt; i++)
        if (th[i].busy * 20 >= rounds)
            REPLY("  thread %5lu: busy %3d%%\n", th[i].id, rounds ? th[i].busy * 100 / rounds : 0);
    // module totals and top buckets
    struct { HMODULE m; int n; } mods[32];
    int nm = 0, total = 0;
    for (int k = 0; k < HB; k++) {
        if (!hist[k].a) continue;
        total += hist[k].n;
        HMODULE m = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)hist[k].a, &m);
        int j = 0;
        while (j < nm && mods[j].m != m) j++;
        if (j == nm && nm < 32) { mods[nm].m = m; mods[nm].n = 0; nm++; }
        if (j < nm) mods[j].n += hist[k].n;
    }
    for (int j = 0; j < nm; j++) {
        char path[MAX_PATH] = "?";
        if (mods[j].m) GetModuleFileNameA(mods[j].m, path, sizeof path);
        const char *b = strrchr(path, 0x5C);
        if (mods[j].n * 50 >= total) REPLY("  module %-28s %3d%%\n", b ? b + 1 : path, total ? mods[j].n * 100 / total : 0);
    }
    for (int r = 0; r < 30; r++) {
        int best = -1;
        for (int k = 0; k < HB; k++) if (hist[k].a && hist[k].n > 0 && (best < 0 || hist[k].n > hist[best].n)) best = k;
        if (best < 0) break;
        char at[160];
        crash_addr(at, sizeof at, hist[best].a);
        REPLY("  %5.1f%%  %s  (thread %lu)\n", total ? hist[best].n * 100.0 / total : 0.0, at, th[hist[best].t].id);
        hist[best].n = -hist[best].n;
    }
    for (int i = 0; i < nt; i++) CloseHandle(th[i].h);
}

// headless test commands (pipe thread, also in the menus and the editor):
//   vmouse <x> <y>               game cursor to x, y (game coordinates; "vmouse off" = back to the real mouse)
//   vclick <x> <y> [right|mid]   move there and click
//   shot <name>                  next presented frame -> aimod_dumps/<name>.bmp
static void cmd_headless(const char *verb, const char *args, char *out) {
    if (!strcmp(verb, "shot")) {
        char name[64] = "shot";
        sscanf(args, "%63s", name);
        CreateDirectoryA("aimod_dumps", NULL);
        GetFullPathNameA("aimod_dumps", sizeof g_shot_path, g_shot_path, NULL);
        lstrcatA(g_shot_path, "\\");
        lstrcatA(g_shot_path, name);
        lstrcatA(g_shot_path, ".bmp");
        LONG before = g_shot_done;
        g_shot_req = 1;
        for (int i = 0; i < 300 && g_shot_done == before; i++) Sleep(10);
        if (g_shot_done != before) REPLY("ok: %s\n", g_shot_path);
        else { g_shot_req = 0; REPLY("error: no frame presented within 3 s\n"); }
        return;
    }
    if (!strncmp(args, "off", 3)) { g_vm_x = g_vm_y = -1; REPLY("ok: real mouse\n"); return; }
    int x = -1, y = -1;
    char which[16] = "";
    if (sscanf(args, "%d %d %15s", &x, &y, which) < 2 || x < 0 || y < 0) { REPLY("usage: %s x y%s\n", verb, *verb == 'v' && verb[1] == 'c' ? " [right|mid]" : ""); return; }
    if (!strcmp(verb, "wclick")) {   // window messages (WONAPI screens such as the civ builder read those, not the game cursor)
        uint8_t *win = *(uint8_t **)G_GAME_WINDOW;
        HWND hwnd = win ? FIELD(win, 0, HWND) : NULL;
        if (!hwnd) { REPLY("error: no window\n"); return; }
        RECT cr;
        GetClientRect(hwnd, &cr);
        RECT v = view_rect(&cr, FIELD(win, 0xC, int), FIELD(win, 0x10, int));
        int cx = v.left + x * (v.right - v.left) / FIELD(win, 0xC, int), cy = v.top + y * (v.bottom - v.top) / FIELD(win, 0x10, int);
        LPARAM lp = MAKELPARAM(cx, cy);
        PostMessageA(hwnd, WM_MOUSEMOVE, 0, lp);
        Sleep(100);
        PostMessageA(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lp);
        Sleep(150);
        PostMessageA(hwnd, WM_LBUTTONUP, 0, lp);
        REPLY("ok: posted click at client %d,%d\n", cx, cy);
        return;
    }
    g_vm_x = x;
    g_vm_y = y;
    if (!strcmp(verb, "vclick")) {
        Sleep(50);   // a frame at the new position first (hover)
        LONG before = g_vm_clicks;
        g_vm_step = 0;
        g_vm_button = !strcmp(which, "right") ? 1 : !strcmp(which, "mid") ? 2 : 0;
        for (int i = 0; i < 300 && g_vm_clicks == before; i++) Sleep(10);
        REPLY(g_vm_clicks != before ? "ok: clicked %d,%d\n" : "error: click not taken within 3 s (%d,%d)\n", x, y);
        return;
    }
    REPLY("ok: cursor %d,%d\n", x, y);
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

// ddpoke z|back|front|mode: one of the side effects ddinfo has, alone (which one makes the windowed terrain appear?)
static void lock_unlock(char *out, const char *name, IDirectDrawSurface7 *s) {
    DDSURFACEDESC2 d = { .dwSize = sizeof d };
    HRESULT hr = s ? s->lpVtbl->Lock(s, NULL, &d, DDLOCK_WAIT | DDLOCK_READONLY, NULL) : E_POINTER;
    if (hr == DD_OK) s->lpVtbl->Unlock(s, NULL);
    REPLY("%s lock/unlock hr=%08lx\n", name, hr);
}
static void cmd_ddpoke(char *out, const char *args) {
    uint8_t *ras = *(uint8_t **)0x00919794;
    uint8_t *st = ras ? FIELD(ras, 0x20, uint8_t **)[*(int *)0x00919428] : NULL;
    if (!st) { REPLY("error: no rasterizer\n"); return; }
    uint8_t *scr = FIELD(st, 0x2A8, uint8_t *), *front = FIELD(st, 0x2AC, uint8_t *), *back = FIELD(st, 0x2B0, uint8_t *);
    IDirectDrawSurface7 *b7 = back ? FIELD(back, 0x48, IDirectDrawSurface7 *) : NULL;
    if (!strcmp(args, "z") && b7) {
        DDSCAPS2 zc = { .dwCaps = DDSCAPS_ZBUFFER };
        IDirectDrawSurface7 *z = NULL;
        if (b7->lpVtbl->GetAttachedSurface(b7, &zc, &z) == DD_OK) { lock_unlock(out, "z-buffer", z); z->lpVtbl->Release(z); }
    } else if (!strcmp(args, "back")) lock_unlock(out, "back", b7);
    else if (!strcmp(args, "front")) lock_unlock(out, "front", front ? FIELD(front, 0x48, IDirectDrawSurface7 *) : NULL);
    else if (!strcmp(args, "mode")) {
        IDirectDraw7 *dd = FIELD(scr, 0x30, IDirectDraw7 *);
        DDSURFACEDESC2 m = { .dwSize = sizeof m };
        REPLY("GetDisplayMode hr=%08lx\n", dd->lpVtbl->GetDisplayMode(dd, &m));
    } else if (!strncmp(args, "cursor", 6)) {   // cursor <client x> <client y>: place the Windows cursor (game's coordinates)
        uint8_t *win = *(uint8_t **)G_GAME_WINDOW;
        POINT p = { 0, 0 }, q;
        RECT rc = { 0 };
        sscanf(args + 6, "%ld %ld", &p.x, &p.y);
        if (win) { GetClientRect(FIELD(win, 0, HWND), &rc); ClientToScreen(FIELD(win, 0, HWND), &p); }
        SetCursorPos(p.x, p.y);
        GetCursorPos(&q);
        REPLY("client %ldx%ld, cursor set to screen %ld,%ld, now at %ld,%ld\n", rc.right, rc.bottom, p.x, p.y, q.x, q.y);
    } else if (!strncmp(args, "bench", 5) && b7) {   // bench [w h]: CPU cost of the GDI copy modes into a w x h bitmap
        int w = 2405, h = 1353;
        sscanf(args + 5, "%d %d", &w, &h);
        DDSURFACEDESC2 d = { .dwSize = sizeof d };
        if (b7->lpVtbl->Lock(b7, NULL, &d, DDLOCK_WAIT | DDLOCK_READONLY, NULL) != DD_OK) { REPLY("error: lock\n"); return; }
        int bpp = (int)d.ddpfPixelFormat.dwRGBBitCount;
        struct { BITMAPINFOHEADER h; DWORD masks[3]; } bi = {
            { sizeof bi.h, d.lPitch / (bpp / 8), -(LONG)d.dwHeight, 1, (WORD)bpp, bpp == 16 ? BI_BITFIELDS : BI_RGB, 0, 0, 0, 0, 0 },
            { d.ddpfPixelFormat.dwRBitMask, d.ddpfPixelFormat.dwGBitMask, d.ddpfPixelFormat.dwBBitMask },
        };
        BITMAPINFOHEADER db = { sizeof db, w, -h, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
        void *bits;
        HDC mdc = CreateCompatibleDC(NULL);
        HBITMAP bmp = CreateDIBSection(mdc, (BITMAPINFO *)&db, DIB_RGB_COLORS, &bits, NULL, 0);
        HGDIOBJ prev = SelectObject(mdc, bmp);
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        const char *names[] = { "HALFTONE scaled", "COLORONCOLOR scaled", "1:1 (no scaling)" };
        for (int m = 0; m < 3; m++) {
            SetStretchBltMode(mdc, m == 0 ? HALFTONE : COLORONCOLOR);
            SetBrushOrgEx(mdc, 0, 0, NULL);
            LONGLONG t = qpc();
            for (int i = 0; i < 5; i++)
                StretchDIBits(mdc, 0, 0, m == 2 ? (int)d.dwWidth : w, m == 2 ? (int)d.dwHeight : h, 0, 0, (int)d.dwWidth, (int)d.dwHeight,
                              d.lpSurface, (BITMAPINFO *)&bi, DIB_RGB_COLORS, SRCCOPY);
            REPLY("%-20s %.2f ms per frame\n", names[m], (double)(qpc() - t) * 1000.0 / (double)f.QuadPart / 5);
        }
        LONGLONG t = qpc();
        for (int i = 0; i < 5; i++) {
            AcquireSRWLockExclusive(&g_scale_lock);
            uint32_t *sc = scale_frame(&d, w, h);
            BITMAPINFOHEADER sb = { sizeof sb, w, -h, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
            if (sc) SetDIBitsToDevice(mdc, 0, 0, (DWORD)w, (DWORD)h, 0, 0, 0, (UINT)h, sc, (BITMAPINFO *)&sb, DIB_RGB_COLORS);
            if (sc && i == 4) {   // the scaled frame for a look: aimod_dumps/bench_scaled.bmp
                DDSURFACEDESC2 sd = d;
                sd.dwWidth = (DWORD)w; sd.dwHeight = (DWORD)h; sd.lPitch = w * 4; sd.lpSurface = sc;
                CreateDirectoryA("aimod_dumps", NULL);
                write_desc_bmp(&sd, "aimod_dumps\\bench_scaled.bmp");
            }
            ReleaseSRWLockExclusive(&g_scale_lock);
        }
        REPLY("%-20s %.2f ms per frame (%d worker threads + this one)\n", "bilinear (mod)",
              (double)(qpc() - t) * 1000.0 / (double)f.QuadPart / 5, g_scale_threads > 0 ? g_scale_threads : 0);
        b7->lpVtbl->Unlock(b7, NULL);
        SelectObject(mdc, prev);
        DeleteObject(bmp);
        DeleteDC(mdc);
    } else REPLY("usage: ddpoke z|back|front|mode|cursor x y|bench [w h]\n");
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
    REPLY("copies from/to the front surface skipped (windowed): %ld\n", g_front_copies_skipped);
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
        REPLY("ddraw display mode: hr=%08lx %lux%lu %lu bpp; real desktop %lux%lu %lu bpp; mode index %u of %u\n", hr,
              m.dwWidth, m.dwHeight, m.ddpfPixelFormat.dwRGBBitCount, dm.dmPelsWidth, dm.dmPelsHeight, dm.dmBitsPerPel,
              FIELD(scr, 0x38, unsigned), (unsigned)((FIELD(scr, 0x14, uintptr_t) - FIELD(scr, 0x10, uintptr_t)) / 0x14));
        uint32_t *md = (uint32_t *)(FIELD(scr, 0x10, uint8_t *) + FIELD(scr, 0x38, unsigned) * 0x14);
        REPLY("current GEVideoMode: %u %u %u %u %u\n", md[0], md[1], md[2], md[3], md[4]);
    }
    REPLY("windowed present: %ld frames, %ld failed; back buffer before ShowScene: %ld samples, %ld lit of 1024 avg, lock failed %ld\n",
          g_present_frames, g_present_fail, g_lit_frames, g_lit_frames ? g_lit_samples / g_lit_frames : 0, g_lock_fail);
    uint8_t *rm = *(uint8_t **)0x009197A8, *ms = is_mouse();
    if (rm) REPLY("mouse: game cursor %.1f,%.1f of x %.0f..%.0f y %.0f..%.0f, updates %ld, unacquired %ld, reacquired %ld, "
                  "device %s, resumed by us %ld, subclassed %p\n", FIELD(rm, 4, float), FIELD(rm, 8, float), FIELD(rm, 0x18, float),
                  FIELD(rm, 0x1C, float), FIELD(rm, 0x20, float), FIELD(rm, 0x14, float), g_mouse_updates, g_mouse_unacq,
                  g_mouse_reacq, ms ? (ms[4] ? "suspended" : "active") : "-", g_input_resumes, (void *)g_subclassed);
    {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        LONG n = InterlockedExchange(&g_t_count, 0);
        double ms = 1000.0 / (double)f.QuadPart;
        LONG64 tl = InterlockedExchange64(&g_t_lock, 0), tb = InterlockedExchange64(&g_t_blit, 0), tf = InterlockedExchange64(&g_t_frame, 0);
        if (n) REPLY("present timing over %ld frames: frame %.2f ms, readback (Lock) %.2f ms, to the window %.2f ms, rest (game) %.2f ms\n",
                     n, tf * ms / n, tl * ms / n, tb * ms / n, (tf - tl - tb) * ms / n);
    }
    REPLY("frame stats (FPS display): %.1f frames/s, %.1f / %.1f per s\n", FIELD(st, 0x2D8, float), FIELD(st, 0x2D4, float), FIELD(st, 0x2E0, float));
    if (g_subclassed) {
        typedef UINT (WINAPI *GetDpiFn)(HWND);
        GetDpiFn get_dpi = (GetDpiFn)(void *)GetProcAddress(GetModuleHandleA("user32.dll"), "GetDpiForWindow");
        void *dpi = dpi_enter(g_subclassed);
        RECT wr, cr2;
        GetWindowRect(g_subclassed, &wr);
        GetClientRect(g_subclassed, &cr2);
        dpi_leave(dpi);
        REPLY("window: %s, style %08lx, window %ld,%ld %ldx%ld, client %ldx%ld, dpi %u, created dpi-aware %d, smooth %d\n",
              g_borderless ? "borderless" : "normal", (unsigned long)GetWindowLongA(g_subclassed, GWL_STYLE), wr.left, wr.top,
              wr.right - wr.left, wr.bottom - wr.top, cr2.right, cr2.bottom, get_dpi ? get_dpi(g_subclassed) : 0, g_dpi_windows, g_smooth);
    }
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
