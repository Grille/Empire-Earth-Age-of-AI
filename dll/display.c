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
// Minimap picture: EEUIMiniMap (0x65DF30..) sizes it as screen width * 1/800 and height * 1/600 (constants
// 0x8358A8 / 0x82CBCC, also used to map clicks on it) - built for frames that scale with the screen like the
// in-game one. The editor's frame has AoC's fixed size (1.28 x the 800x600 one), so at other resolutions the
// picture overshot it (only its upper left part visible). The 6 operands read our factors instead: in the
// editor 1.28 / width and 1.28 / height (= the originals at 1024x768), elsewhere the originals.
static float g_mm_fx = 1.0f / 800.0f, g_mm_fy = 1.0f / 600.0f;
static const uintptr_t k_mm_fx_ops[] = { 0x0065E01D, 0x0065E8A3, 0x0065EB3E };
static const uintptr_t k_mm_fy_ops[] = { 0x0065E032, 0x0065E8CE, 0x0065EB68 };
static int redirect_minimap_scale(void) {
    int n = 0;
    for (int i = 0; i < 3; i++) {
        if (*(uint32_t *)k_mm_fx_ops[i] == 0x008358A8) { write_code32((uint32_t *)k_mm_fx_ops[i], (uint32_t)(uintptr_t)&g_mm_fx); n++; }
        if (*(uint32_t *)k_mm_fy_ops[i] == 0x0082CBCC) { write_code32((uint32_t *)k_mm_fy_ops[i], (uint32_t)(uintptr_t)&g_mm_fy); n++; }
    }
    return n;
}

static void __stdcall hook_editor_resolution(int w, int h, int force, int flag) {
    editor_size(&w, &h);
    int lay = apply_editor_layout(w, h);
    g_mm_fx = 1.28f / (float)w;
    g_mm_fy = 1.28f / (float)h;
    if (g_log) { fprintf(g_log, "# editor: resolution %dx%d, layout patches %d/%d\n", w, h, lay, (int)N_LAYOUT + 6); fflush(g_log); }
    ((SetResolutionFn)FN_SET_RESOLUTION)(w, h, force, flag);
}

// every other resolution switch (game, menus, test play from the editor): the original minimap factors
static void __stdcall hook_other_resolution(int w, int h, int force, int flag) {
    g_mm_fx = 1.0f / 800.0f;
    g_mm_fy = 1.0f / 600.0f;
    ((SetResolutionFn)FN_SET_RESOLUTION)(w, h, force, flag);
}
static const uintptr_t k_other_resolution_sites[] = { 0x0065F30A, 0x0065F61B, 0x0065F945, 0x0065FAD4, 0x00660CD3, 0x00661329, 0x00661363 };

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
    // headless: every top-level window off-screen, never activated - the game window and the display DLL's
    // 16x16 "TestWindowClass" probe window, which otherwise takes the foreground for a moment
    if (g_headless && !parent) {
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

// timing of the windowed present (ddinfo): readback (Lock), copy to the window (GDI, Direct3D or primary Blt), frame interval
static volatile LONG64 g_t_lock, g_t_blit, g_t_frame;
static volatile LONG g_t_count;
static LONGLONG qpc(void) { LARGE_INTEGER t; QueryPerformanceCounter(&t); return t.QuadPart; }

#include "present_d3d.c"

// present = auto (default): with DDrawCompat as ddraw.dll the frame goes onto the primary surface - DDrawCompat
// shows the primary over the game window itself (a frame drawn into the window directly stays hidden: black
// window), and unlike dgVoodoo's pass-through its windowed primary is desktop sized, so the Blt is safe. A GPU
// copy (back buffer -> primary at the window's client area, scaled, bars filled black, clipped to the window);
// the back buffer is only locked for a "shot". Otherwise Direct3D 11 (present_d3d.c), or gdi when set.
static int g_present_mode;   // 0 auto, 1 d3d, 2 gdi, 3 primary
static int ddrawcompat_loaded(void) {
    static int known = -1;
    if (known >= 0) return known;
    known = 0;
    HMODULE m = GetModuleHandleA("ddraw.dll");
    if (!m) return 0;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)((uint8_t *)m + ((IMAGE_DOS_HEADER *)m)->e_lfanew);
    const uint8_t *p = (const uint8_t *)m, *end = p + nt->OptionalHeader.SizeOfImage - 16;
    MEMORY_BASIC_INFORMATION mbi;
    for (const uint8_t *q = p; q < end; ) {   // readable pages only
        if (!VirtualQuery(q, &mbi, sizeof mbi)) break;
        const uint8_t *re = (const uint8_t *)mbi.BaseAddress + mbi.RegionSize;
        if (mbi.State == MEM_COMMIT && !(mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)))
            for (; q < re && q < end; q++)
                if (*q == 'D' && !memcmp(q, "DDrawCompat version", 19)) { known = 1; return 1; }
        q = re;
    }
    return 0;
}

static volatile LONG g_primary_frames, g_primary_fail;
static HRESULT g_primary_hr;
static int present_primary(uint8_t *ras, unsigned screen, uint8_t *scr, uint8_t *back, IDirectDrawSurface7 *b, HWND hwnd) {
    (void)ras; (void)screen;
    if (g_present_mode != 3 && !(g_present_mode == 0 && ddrawcompat_loaded())) return 0;
    uint8_t *st = FIELD(ras, 0x20, uint8_t **)[screen];
    uint8_t *front = FIELD(st, 0x2AC, uint8_t *);
    IDirectDrawSurface7 *f = front ? FIELD(front, 0x48, IDirectDrawSurface7 *) : NULL;
    IDirectDraw7 *dd = FIELD(scr, 0x30, IDirectDraw7 *);
    if (!f || !dd) return 0;
    (void)back;
    window_hook(hwnd);
    static LONGLONG t_last;
    LONGLONG t0 = qpc();
    DDSURFACEDESC2 d = { .dwSize = sizeof d };
    if (g_shot_req && b->lpVtbl->Lock(b, NULL, &d, DDLOCK_WAIT | DDLOCK_READONLY, NULL) == DD_OK) {
        write_desc_bmp(&d, g_shot_path);
        b->lpVtbl->Unlock(b, NULL);
        g_shot_req = 0;
        InterlockedIncrement(&g_shot_done);
    }
    d.dwFlags = 0;
    b->lpVtbl->GetSurfaceDesc(b, &d);
    static HWND clipped;
    if (clipped != hwnd) {   // clip the primary to the game window (other windows stay untouched)
        IDirectDrawClipper *c = NULL;
        if (dd->lpVtbl->CreateClipper(dd, 0, &c, NULL) == DD_OK) {
            c->lpVtbl->SetHWnd(c, 0, hwnd);
            f->lpVtbl->SetClipper(f, c);
            c->lpVtbl->Release(c);
        }
        clipped = hwnd;
    }
    void *dpi = dpi_enter(hwnd);
    RECT cr;
    GetClientRect(hwnd, &cr);
    RECT v = view_rect(&cr, (int)d.dwWidth, (int)d.dwHeight);
    POINT o = { 0, 0 };
    ClientToScreen(hwnd, &o);
    dpi_leave(dpi);
    if (v.left > 0 || v.top > 0) {   // bars
        DDBLTFX fx = { .dwSize = sizeof fx, .dwFillColor = 0 };
        RECT b1 = { o.x, o.y, o.x + (v.left > 0 ? v.left : cr.right), o.y + (v.left > 0 ? cr.bottom : v.top) };
        RECT b2 = { o.x + (v.left > 0 ? v.right : 0), o.y + (v.left > 0 ? 0 : v.bottom), o.x + cr.right, o.y + cr.bottom };
        f->lpVtbl->Blt(f, &b1, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
        f->lpVtbl->Blt(f, &b2, NULL, NULL, DDBLT_COLORFILL | DDBLT_WAIT, &fx);
    }
    // the part of the image that lies on the primary (a window partly off the screen would fail the whole Blt
    // with DDERR_INVALIDRECT); the source rect is cut to match
    RECT dst = { o.x + v.left, o.y + v.top, o.x + v.right, o.y + v.bottom };
    DDSURFACEDESC2 fd = { .dwSize = sizeof fd };
    f->lpVtbl->GetSurfaceDesc(f, &fd);
    RECT on = { 0, 0, (LONG)fd.dwWidth, (LONG)fd.dwHeight }, dc;
    HRESULT hr = DD_OK;
    if (IntersectRect(&dc, &dst, &on)) {
        double sx = (double)d.dwWidth / (dst.right - dst.left), sy = (double)d.dwHeight / (dst.bottom - dst.top);
        RECT src = { (LONG)((dc.left - dst.left) * sx), (LONG)((dc.top - dst.top) * sy),
                     (LONG)((dc.right - dst.left) * sx + 0.5), (LONG)((dc.bottom - dst.top) * sy + 0.5) };
        if (src.right > (LONG)d.dwWidth) src.right = (LONG)d.dwWidth;
        if (src.bottom > (LONG)d.dwHeight) src.bottom = (LONG)d.dwHeight;
        hr = f->lpVtbl->Blt(f, &dc, b, &src, DDBLT_WAIT, NULL);
    }
    g_primary_hr = hr;
    if (hr != DD_OK) InterlockedIncrement(&g_primary_fail);
    LONGLONG t1 = qpc();
    if (t_last) {
        InterlockedAdd64(&g_t_blit, t1 - t0);
        InterlockedAdd64(&g_t_frame, t0 - t_last);
        InterlockedIncrement(&g_t_count);
    }
    t_last = t0;
    ValidateRect(hwnd, NULL);
    InterlockedIncrement(&g_primary_frames);
    InterlockedIncrement(&g_present_frames);
    return 1;
}

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
    if (present_primary(ras, screen, scr, back, s, hwnd)) return 1;
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

// FRAME WATCH (aimod.ini [display] frame_watch = frames per second, default 35; 0 = off). Low frame rates late in
// long games (25 fps, 64% of the render thread in GDI, 2026-10-12) don't show in the test games, so the game watches
// itself: while a game runs (the game clock moves), every minute a "# frames:" line with the average and the worst
// second and the process's GDI / USER object counts (a leak would grow them); when 10 seconds in a row stay under
// the threshold, the render-thread profile (profile 3000 render) runs on its own thread and goes into the log -
// at most 5 a session, 5 minutes apart.
static void cmd_profile_render(int dur, char *out);
static void frame_costs_log(unsigned frames);
static int is_return_address(uintptr_t a);
static int g_frame_watch = 35;
static int g_fw_test;   // autotest frame_watch given: also headless (tests)
static struct {
    DWORD sec_start, min_ms, last_profile;
    unsigned sec_frames, min_frames, slow_secs, min_slow, profiles;
    uint32_t last_gt;
    float worst, trigger_fps;
} g_fw;
static HANDLE g_fw_event;
static DWORD WINAPI frame_profile_thread(LPVOID unused) {
    (void)unused;
    static char out[sizeof g_req.out];
    for (;;) {
        WaitForSingleObject(g_fw_event, INFINITE);
        out[0] = 0;
        cmd_profile_render(3000, out);
        if (!g_log) continue;
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# frame watch: under %d fps for 10 s at %.1f min (last second %.0f fps), render thread profile:\n",
                g_frame_watch, game_time() / 60000.0, g_fw.trigger_fps);
        for (char *l = out, *e; *l; l = e) {   // every line a comment line of the log
            e = strchr(l, '\n');
            int n = e ? (int)(e - l) : (int)strlen(l);
            fprintf(g_log, "# | %.*s\n", n, l);
            e = e ? e + 1 : l + n;
        }
        fflush(g_log);
        LeaveCriticalSection(&g_lock);
    }
    return 0;
}
static void frame_watch(void) {
    if (!g_frame_watch || (g_headless && !g_fw_test)) return;
    DWORD now = GetTickCount();
    if (!g_fw.sec_start) { g_fw.sec_start = now; g_fw.worst = 1e9f; return; }
    g_fw.sec_frames++;
    DWORD dt = now - g_fw.sec_start;
    if (dt < 1000) return;
    float fps = (float)g_fw.sec_frames * 1000.0f / (float)dt;
    uint32_t gt = game_time();
    int running = gt && gt != g_fw.last_gt;   // paused, menus, loading: not counted
    g_fw.last_gt = gt;
    g_fw.sec_start = now;
    unsigned frames = g_fw.sec_frames;
    g_fw.sec_frames = 0;
    if (!running) { g_fw.slow_secs = 0; return; }
    g_fw.min_frames += frames;
    g_fw.min_ms += dt;
    if (fps < g_fw.worst) g_fw.worst = fps;
    if (fps < (float)g_frame_watch) { g_fw.slow_secs++; g_fw.min_slow++; } else g_fw.slow_secs = 0;
    if (g_fw.slow_secs >= 10 && g_fw.profiles < 5 && (!g_fw.last_profile || now - g_fw.last_profile >= 300000)) {
        if (!g_fw_event) {
            g_fw_event = CreateEventA(NULL, FALSE, FALSE, NULL);
            HANDLE t = g_fw_event ? CreateThread(NULL, 0, frame_profile_thread, NULL, 0, NULL) : NULL;
            if (t) CloseHandle(t);
        }
        g_fw.slow_secs = 0;
        g_fw.last_profile = now;
        g_fw.profiles++;
        g_fw.trigger_fps = fps;
        if (g_fw_event) SetEvent(g_fw_event);
    }
    if (g_fw.min_ms >= 60000) {
        if (g_log) {
            HANDLE pr = GetCurrentProcess();
            EnterCriticalSection(&g_lock);
            fprintf(g_log, "# frames: at %.1f min: %.0f fps, worst second %.0f, %u s under %d fps; GDI objects %lu, USER objects %lu\n",
                    gt / 60000.0, g_fw.min_frames * 1000.0 / g_fw.min_ms, g_fw.worst, g_fw.min_slow, g_frame_watch,
                    GetGuiResources(pr, GR_GDIOBJECTS), GetGuiResources(pr, GR_USEROBJECTS));
            fflush(g_log);
            LeaveCriticalSection(&g_lock);
        }
        frame_costs_log(g_fw.min_frames);
        g_fw.min_frames = g_fw.min_ms = g_fw.min_slow = 0;
        g_fw.worst = 1e9f;
    }
}

static int __fastcall hook_show_scene(void *ras, void *edx, unsigned screen) {
    (void)edx;
    g_render_thread = GetCurrentThreadId();
    frame_watch();
    camera_frame();
    camtrace_frame();
    // headless turbo: nobody watches, so only a few frames a second (CPU for the simulation); a shot still
    // gets the next frame
    if (g_headless && g_turbo && !g_shot_req)
        for (int i = 0; i < 20 && !g_shot_req; i++) Sleep(10);
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
static void *g_text_lock_orig[2], *g_text_unlock_orig[2];   // DX7Surface::TextLock / TextUnlock (own text DC)
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

// FRAME COSTS (with the frame watch): the profile of a slow game (2026-10-12, 15-25 fps late in the game) had 73%
// of the render thread in GDI - a quarter each under DX7Rasterizer::Draw(GEText) (GetTextExtentPoint32, text on
// surfaces), DX7Surface::Lock and the exe's WONAPI code. These display DLL functions are timed (calls and time per
// frame, and their 3 most frequent callers) and logged with the minute line of the frame watch.
enum { FC_TEXT, FC_TEXT_RECT, FC_LOCK, FC_TEXT_LOCK, FC_TEXT_UNLOCK, FC_N };
static struct {
    const char *name, *export;
    void *orig[2];
    volatile LONG calls;
    volatile LONG64 ticks;
    struct { uintptr_t a; volatile LONG n; } by[8];
} g_fc[FC_N] = {
    { "text", "?Draw@DX7Rasterizer@@UAEJPAVGEText@@PAVGESurface@@@Z", { 0 }, 0, 0, { { 0, 0 } } },
    { "text in rect", "?Draw@DX7Rasterizer@@UAEJPAVGEText@@ABVGERectangle@@PAVGESurface@@@Z", { 0 }, 0, 0, { { 0, 0 } } },
    { "lock", "?Lock@DX7Surface@@UAEJXZ", { 0 }, 0, 0, { { 0, 0 } } },
    { "text lock", "?TextLock@DX7Surface@@UAEJXZ", { 0 }, 0, 0, { { 0, 0 } } },
    { "text unlock", "?TextUnlock@DX7Surface@@UAEJXZ", { 0 }, 0, 0, { { 0, 0 } } },
};
static void fc_note(int k, uintptr_t ret, LONGLONG t) {
    InterlockedIncrement(&g_fc[k].calls);
    InterlockedExchangeAdd64(&g_fc[k].ticks, t);
    int j = 0, low = 0;
    for (; j < 8 && g_fc[k].by[j].a && g_fc[k].by[j].a != ret; j++)
        if (g_fc[k].by[j].n < g_fc[k].by[low].n) low = j;
    if (j == 8) { j = low; g_fc[k].by[j].a = ret; g_fc[k].by[j].n = 0; }
    g_fc[k].by[j].a = ret;
    InterlockedIncrement(&g_fc[k].by[j].n);
}
#define FC_TIMED(k, expr) do { LARGE_INTEGER t0, t1; QueryPerformanceCounter(&t0); r = (expr); QueryPerformanceCounter(&t1); \
                               fc_note(k, (uintptr_t)__builtin_return_address(0), t1.QuadPart - t0.QuadPart); } while (0)
static long __fastcall hook_fc_text(void *ras, void *edx, void *text, void *surf) {
    (void)edx; long r;
    FC_TIMED(FC_TEXT, ((long (__thiscall *)(void *, void *, void *))g_fc[FC_TEXT].orig[DISPLAY_DLL(ras)])(ras, text, surf));
    return r;
}
// TEXT AS TEXTURES ([display] text_bitmap): DX7Rasterizer::Draw(GEText*, rect, GESurface*) draws with GDI on a
// lock of the surface - on the 3D back buffer DDrawCompat then waits for the GPU (~2-4 ms a frame, half of a frame
// at 100 fps) - unless the text's font (+0x44) has a GEFontBitmap at +0x24: then the Low-Level Engine draws each
// character as a sprite of a texture prerendered with GDI once (alpha channel, tmFirstChar..tmLastChar-1, one
// texture per character; GEFontBitmap::GEFontBitmap(GERasterizer*, GEFont*, screen) 0x2724E, 0x24 bytes). The game
// gives only a few fonts one. That path draws into the current render target whatever the surface argument is, so
// a bitmap font of ours is set on the font only while text goes to the back buffer (screen 0 +0x2B0) and taken off
// again after the call. It reads characters as signed bytes (�/�/� negative, no glyph): both Draw functions'
// movsx (LLE +0x27A46, +0x27C9A) become movzx.
static int g_text_bitmap = 1;
static struct { void *font, *bmp, *hfont; } g_fbm[64];
// A font the game destroys (dialogs and screens make theirs anew each time) leaves its entry behind: the next font
// often gets the same address and, since ~GEFont (0x1002721B) DeleteObject's its HFONT, the same handle - the
// cache then gave it the dead font's characters and the text drew nothing (the save screen's labels and its
// overwrite dialog the second time it opened - suspected, not seen headless). The deleting destructor (GEFont
// vtable slot 0) is hooked: the entry goes. Its bitmap font is left alone (freeing engine objects whose textures may
// have gone with a screen is not worth the risk; a few KB per destroyed font, and fonts are rarely destroyed).
static CRITICAL_SECTION g_fbm_cs;
static int g_fbm_n;   // (defined with g_fbm_logged below)
static int g_fbm_dropped;
static void *(__thiscall *g_font_delete)(void *, unsigned);
static void *__fastcall hook_font_delete(uint8_t *font, void *edx, unsigned flags) {
    (void)edx;
    EnterCriticalSection(&g_fbm_cs);
    for (int i = 0; i < g_fbm_n; i++)
        if (g_fbm[i].font == font) {
            g_fbm[i] = g_fbm[--g_fbm_n];
            g_fbm_dropped++;
            if (g_log) fprintf(g_log, "# text bitmap: font %p destroyed - its entry dropped\n", (void *)font);
            break;
        }
    LeaveCriticalSection(&g_fbm_cs);
    return g_font_delete(font, flags);
}
// textlog N (pipe): the next N text draws for the back buffer into the log - font, HFONT, the bitmap font used,
// the string (GEText +0x34) and the 16 bytes at +0x14 (colour?) - for a screen whose text is missing
static volatile LONG g_textlog;
static int g_fbm_n, g_fbm_logged;
static volatile int g_fbm_making;
static int g_atlas_dump;   // autotest atlas_dump=1: each new bitmap font's last atlas -> aimod_dumps/atlas<n>.bmp
static void atlas_dump(int n);   // inside our GEFontBitmap constructor call (the atlas is drawn)
static void *(__thiscall *g_fbm_ctor)(void *, void *, void *, unsigned);
static void *text_bitmap_for(void *ras, void *text, void *surf) {
    if (!g_text_bitmap || !g_fbm_ctor || !text) return NULL;
    uint8_t **screens = FIELD(ras, 0x20, uint8_t **);
    uint8_t *st = screens ? screens[0] : NULL;
    if (!st || FIELD(st, 0x2B0, void *) != surf) return NULL;   // only text for the back buffer
    // Only the game's own image (wider than the menus' 1024x768: the menus gain nothing and are scaled up), and
    // only with bitmap fonts made for this back buffer: leaving a game switches the display mode, the character
    // textures are gone and the old bitmap fonts drew nothing (menus without text). A new back buffer starts a
    // new set (the old objects are left alone: their textures went with the old screen).
    static void *back;
    static int back_ok;
    if (surf != back) {
        back = surf;
        g_fbm_n = 0;
        IDirectDrawSurface7 *dds = FIELD((uint8_t *)surf, 0x48, IDirectDrawSurface7 *);
        DDSURFACEDESC2 d = { .dwSize = sizeof d };
        back_ok = dds && dds->lpVtbl->GetSurfaceDesc(dds, &d) == DD_OK && d.dwWidth > 1024;
        if (g_log) fprintf(g_log, "# text bitmap: back buffer %p %lux%lu - %s\n", surf, d.dwWidth, d.dwHeight,
                           back_ok ? "text as textures" : "GDI text (menus)");
    }
    if (!back_ok) return NULL;
    uint8_t *font = FIELD(text, 0x44, uint8_t *);
    if (!font || FIELD(font, 0x24, void *)) return NULL;          // the game's own bitmap font
    void *hf_now = FIELD(font, 0x18, void *);
    EnterCriticalSection(&g_fbm_cs);
    for (int i = 0; i < g_fbm_n; i++)
        if (g_fbm[i].font == font) {
            void *r = g_fbm[i].bmp;
            if (g_fbm[i].hfont != hf_now && g_log)
                fprintf(g_log, "# text bitmap: STALE font %p (HFONT %p, made for %p)\n", (void *)font, hf_now, g_fbm[i].hfont);
            LeaveCriticalSection(&g_fbm_cs);
            return r;
        }
    if (g_fbm_n >= 64) {
        static int full_logged;
        LeaveCriticalSection(&g_fbm_cs);
        if (g_log && !full_logged++) fprintf(g_log, "# text bitmap: FULL (64 fonts) - GDI text for new fonts\n");
        return NULL;
    }
    void *obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x40);
    if (!obj) { LeaveCriticalSection(&g_fbm_cs); return NULL; }
    LARGE_INTEGER t0, t1, f;
    QueryPerformanceCounter(&t0);
    g_fbm_making = 1;
    g_fbm_ctor(obj, ras, font, 0);
    g_fbm_making = 0;
    if (g_atlas_dump) atlas_dump(g_fbm_n);
    QueryPerformanceCounter(&t1);
    QueryPerformanceFrequency(&f);
    FIELD(font, 0x24, void *) = NULL;   // in case the constructor registers itself: only during our calls
    g_fbm[g_fbm_n].font = font;
    g_fbm[g_fbm_n].bmp = obj;
    g_fbm[g_fbm_n].hfont = hf_now;
    g_fbm_n++;
    int made = g_fbm_n, dropped = g_fbm_dropped;
    LeaveCriticalSection(&g_fbm_cs);
    if (g_log && g_fbm_logged++ < 200) {
        HFONT hf = FIELD(font, 0x18, HFONT);
        LOGFONTA lf = { 0 };
        if (hf) GetObjectA(hf, sizeof lf, &lf);
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# text bitmap: font %d (%d dropped) %p \"%s\" %ld px: bitmap font made in %.1f ms\n", made, dropped, (void *)font, lf.lfFaceName,
                lf.lfHeight, (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)f.QuadPart);
        LeaveCriticalSection(&g_lock);
    }
    return obj;
}
static long __fastcall hook_fc_text_rect(void *ras, void *edx, void *text, void *rect, void *surf) {
    (void)edx; long r;
    void *bmp = text_bitmap_for(ras, text, surf);
    if (g_textlog > 0 && InterlockedDecrement(&g_textlog) >= 0 && g_log && text) {
        uint8_t *f = FIELD(text, 0x44, uint8_t *);
        const char *str = FIELD(text, 0x34, const char *);
        uint32_t *c = (uint32_t *)((uint8_t *)text + 0x14);
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# textlog: surf %p font %p hfont %p own %p bmp %p +14: %08x %08x %08x %08x \"%.60s\"\n", surf, (void *)f,
                f ? FIELD(f, 0x18, void *) : NULL, f ? FIELD(f, 0x24, void *) : NULL, bmp, c[0], c[1], c[2], c[3],
                str && !IsBadReadPtr(str, 1) ? str : "?");
        LeaveCriticalSection(&g_lock);
    }
    // the character textures with the device's wrap addressing: filtering at a glyph's bottom edge picked up its
    // top row (a line under the "t" of "Vorzeit") - clamped while our bitmap text draws. Device: (ras +0x6C)[0]
    // +0x1C (IDirect3DDevice7; vtable 36 GetTextureStageState, 37 SetTextureStageState; 13/14 ADDRESSU/V, 3 clamp)
    uint8_t **dstate = bmp ? FIELD(ras, 0x6C, uint8_t **) : NULL;
    void *dev = dstate && dstate[0] ? FIELD(dstate[0], 0x1C, void *) : NULL;
    DWORD addr[2] = { 0, 0 };
    if (bmp) FIELD(FIELD(text, 0x44, uint8_t *), 0x24, void *) = bmp;
    if (dev) {
        void **vt = *(void ***)dev;
        for (int k = 0; k < 2; k++) {
            ((HRESULT (__stdcall *)(void *, DWORD, DWORD, DWORD *))vt[36])(dev, 0, 13 + k, &addr[k]);
            ((HRESULT (__stdcall *)(void *, DWORD, DWORD, DWORD))vt[37])(dev, 0, 13 + k, 3);
        }
    }
    FC_TIMED(FC_TEXT_RECT, ((long (__thiscall *)(void *, void *, void *, void *))g_fc[FC_TEXT_RECT].orig[DISPLAY_DLL(ras)])(ras, text, rect, surf));
    if (dev) {
        void **vt = *(void ***)dev;
        for (int k = 0; k < 2; k++) ((HRESULT (__stdcall *)(void *, DWORD, DWORD, DWORD))vt[37])(dev, 0, 13 + k, addr[k]);
    }
    if (bmp) FIELD(FIELD(text, 0x44, uint8_t *), 0x24, void *) = NULL;
    return r;
}
static int text_bitmap_install(void) {   // constructor and the two movsx -> movzx of the Low-Level Engine
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
    g_fbm_ctor = lle ? (void *(__thiscall *)(void *, void *, void *, unsigned))(void *)GetProcAddress(lle,
                 "??0GEFontBitmap@@QAE@PAVGERasterizer@@PAVGEFont@@K@Z") : NULL;
    if (!g_fbm_ctor) return 0;
    InitializeCriticalSection(&g_fbm_cs);
    void **font_vt = (void **)(void *)GetProcAddress(lle, "??_7GEFont@@6B@");
    if (!font_vt) return 0;   // without the hook the cache can't tell a new font from a dead one
    g_font_delete = (void *(__thiscall *)(void *, unsigned))font_vt[0];
    if (!patch_vtable((uintptr_t)font_vt, 0, (uintptr_t)font_vt[0], (void *)hook_font_delete)) return 0;
    static const uint32_t sites[2] = { 0x27A46, 0x27C9A };
    int ok = 0;
    for (int i = 0; i < 2; i++) {
        uint8_t *p = (uint8_t *)lle + sites[i];
        if (p[0] != 0x0F || (p[1] != 0xBE && p[1] != 0xB6)) continue;
        DWORD old;
        if (!VirtualProtect(p, 2, PAGE_EXECUTE_READWRITE, &old)) continue;
        p[1] = 0xB6;
        VirtualProtect(p, 2, old, &old);
        ok++;
    }
    FlushInstructionCache(GetCurrentProcess(), NULL, 0);
    return ok == 2;
}
#define FC_SURFACE_HOOK(fn, k) static long __fastcall fn(void *surf, void *edx) { \
    (void)edx; long r; FC_TIMED(k, ((long (__thiscall *)(void *))g_fc[k].orig[DISPLAY_DLL(surf)])(surf)); return r; }
FC_SURFACE_HOOK(hook_fc_lock, FC_LOCK)
FC_SURFACE_HOOK(hook_fc_text_lock, FC_TEXT_LOCK)
FC_SURFACE_HOOK(hook_fc_text_unlock, FC_TEXT_UNLOCK)
// OWN TEXT DC ([display] text_dc, default 1): the game draws text with GDI on a DC of the DirectDraw surface
// (DX7Surface::TextLock = IDirectDrawSurface7::GetDC into +0x54, flag +0x5D; TextUnlock = ReleaseDC). Through
// DDrawCompat (v0.7.1) every ExtTextOutA on such a DC makes gdi32 add a font realization to its width cache
// (LpkUseGDIWidthCache -> GdiRealizationInfo, 1472-byte blocks) that is never freed: ~4 a frame, 13-22 MB a minute,
// a list every text output walks - 111 fps at the start of a game, 13 after 100 minutes (dgVoodoo: no growth).
// Instead the game gets a memory DC of ours that lives for the session (one per surface size and pixel format,
// a DIB section in the surface's format). The text goes onto the 3D back buffer (2560x1440, video memory): a lock of
// the whole surface reads all 14.7 MB back from the graphics card (~4-5 ms a frame), so nothing is locked at
// TextLock; each ExtTextOutA / TextOutW on our DC (import hooks of the display DLLs and the exe) locks only the
// rectangle of the 32x32 tiles its text box touches that aren't loaded yet and copies them into the DIB;
// TextUnlock locks the rectangle of the loaded tiles, writes them back, and selects the stock font again (the
// game's fonts must not stay selected in a DC it never sees again).
static int g_text_dc = 1;
static struct { int w, h, bpp; DWORD masks[3]; HDC dc; HBITMAP bmp; void *bits; int pitch; } g_tdc[6];
static struct { void *surf; int slot; DDSURFACEDESC2 d; } g_tdc_active[8];
enum { TDC_TILE = 32, TDC_TW = 2560 / TDC_TILE + 2, TDC_TH = 1600 / TDC_TILE + 2 };
static uint8_t g_tdc_tiles[TDC_TH][TDC_TW];   // loaded in this text phase (one phase at a time)
static int g_tdc_any;
typedef BOOL (WINAPI *ExtTextOutAFn)(HDC, int, int, UINT, const RECT *, LPCSTR, UINT, const INT *);
typedef BOOL (WINAPI *TextOutWFn)(HDC, int, int, LPCWSTR, int);
static ExtTextOutAFn g_ext_text_out;
static TextOutWFn g_text_out_w;
static int g_tdc_logged;
static HFONT g_tdc_stock;
static long text_dc_fallback(void *surf, int unlock) {   // the original TextLock / TextUnlock
    return ((long (__thiscall *)(void *))(unlock ? g_text_unlock_orig : g_text_lock_orig)[DISPLAY_DLL(surf)])(surf);
}
static int text_dc_slot(const DDSURFACEDESC2 *d) {
    int bpp = (int)d->ddpfPixelFormat.dwRGBBitCount, w = (int)d->dwWidth, h = (int)d->dwHeight, free_ = -1;
    if (bpp != 16 && bpp != 32) return -1;
    for (int i = 0; i < 6; i++) {
        if (g_tdc[i].dc && g_tdc[i].w == w && g_tdc[i].h == h && g_tdc[i].bpp == bpp &&
            g_tdc[i].masks[0] == d->ddpfPixelFormat.dwRBitMask && g_tdc[i].masks[1] == d->ddpfPixelFormat.dwGBitMask)
            return i;
        if (!g_tdc[i].dc && free_ < 0) free_ = i;
    }
    if (free_ < 0) return -1;
    struct { BITMAPINFOHEADER h; DWORD masks[3]; } bi = {
        { sizeof bi.h, w, -h, 1, (WORD)bpp, BI_BITFIELDS, 0, 0, 0, 0, 0 },
        { d->ddpfPixelFormat.dwRBitMask, d->ddpfPixelFormat.dwGBitMask, d->ddpfPixelFormat.dwBBitMask },
    };
    void *bits = NULL;
    HDC dc = CreateCompatibleDC(NULL);
    HBITMAP bmp = dc ? CreateDIBSection(dc, (BITMAPINFO *)&bi, DIB_RGB_COLORS, &bits, NULL, 0) : NULL;
    if (!bmp) { if (dc) DeleteDC(dc); return -1; }
    SelectObject(dc, bmp);
    if (!g_tdc_stock) g_tdc_stock = (HFONT)GetStockObject(SYSTEM_FONT);
    g_tdc[free_].w = w; g_tdc[free_].h = h; g_tdc[free_].bpp = bpp;
    g_tdc[free_].masks[0] = bi.masks[0]; g_tdc[free_].masks[1] = bi.masks[1]; g_tdc[free_].masks[2] = bi.masks[2];
    g_tdc[free_].dc = dc; g_tdc[free_].bmp = bmp; g_tdc[free_].bits = bits; g_tdc[free_].pitch = (w * bpp / 8 + 3) & ~3;
    return free_;
}
static int tdc_active_of(HDC dc) {   // the active text phase drawing on this DC, or -1
    for (int i = 0; i < 8; i++)
        if (g_tdc_active[i].surf && g_tdc[g_tdc_active[i].slot].dc == dc) return i;
    return -1;
}
// lock rectangle r of the phase's surface (DDLOCK_WAIT); lpSurface then points at r's top left
static int tdc_lock(int a, RECT *r, DDSURFACEDESC2 *d) {
    IDirectDrawSurface7 *dds = FIELD((uint8_t *)g_tdc_active[a].surf, 0x48, IDirectDrawSurface7 *);
    memset(d, 0, sizeof *d);
    d->dwSize = sizeof *d;
    return dds && dds->lpVtbl->Lock(dds, r, d, DDLOCK_WAIT, NULL) == DD_OK;
}
static void tdc_unlock(int a, RECT *r) {
    IDirectDrawSurface7 *dds = FIELD((uint8_t *)g_tdc_active[a].surf, 0x48, IDirectDrawSurface7 *);
    if (dds) dds->lpVtbl->Unlock(dds, r);
}
// copy the tiles of `want` (1 = to load) between the DIB and the surface, through one lock of their bounding box
static void tdc_copy_tiles(int a, int to_surface) {
    int slot = g_tdc_active[a].slot, w = g_tdc[slot].w, h = g_tdc[slot].h, bpp = g_tdc[slot].bpp / 8;
    int tx0 = TDC_TW, ty0 = TDC_TH, tx1 = -1, ty1 = -1;
    uint8_t want = to_surface ? 1 : 2;   // 2 = to load now, 1 = loaded (written back at the end)
    for (int ty = 0; ty < TDC_TH; ty++)
        for (int tx = 0; tx < TDC_TW; tx++)
            if (g_tdc_tiles[ty][tx] == want) {
                if (tx < tx0) tx0 = tx;
                if (tx > tx1) tx1 = tx;
                if (ty < ty0) ty0 = ty;
                if (ty > ty1) ty1 = ty;
            }
    if (tx1 < 0) return;
    RECT r = { tx0 * TDC_TILE, ty0 * TDC_TILE, (tx1 + 1) * TDC_TILE, (ty1 + 1) * TDC_TILE };
    if (r.right > w) r.right = w;
    if (r.bottom > h) r.bottom = h;
    DDSURFACEDESC2 d;
    if (!tdc_lock(a, &r, &d)) {
        for (int ty = ty0; ty <= ty1; ty++) for (int tx = tx0; tx <= tx1; tx++) if (g_tdc_tiles[ty][tx] == want) g_tdc_tiles[ty][tx] = 0;
        return;
    }
    for (int ty = ty0; ty <= ty1; ty++)
        for (int tx = tx0; tx <= tx1; tx++) {
            if (g_tdc_tiles[ty][tx] != want) continue;
            g_tdc_tiles[ty][tx] = to_surface ? 0 : 1;
            int px = tx * TDC_TILE, n = (px + TDC_TILE > w ? w - px : TDC_TILE) * bpp;
            for (int y = ty * TDC_TILE; y < ty * TDC_TILE + TDC_TILE && y < h; y++) {
                uint8_t *sp = (uint8_t *)d.lpSurface + (y - r.top) * d.lPitch + (px - r.left) * bpp;
                uint8_t *bp = (uint8_t *)g_tdc[slot].bits + y * g_tdc[slot].pitch + px * bpp;
                if (to_surface) memcpy(sp, bp, n); else memcpy(bp, sp, n);
            }
        }
    tdc_unlock(a, &r);
}
// load the tiles of rectangle x0,y0-x1,y1 (exclusive) from the surface into the DIB, once a phase
static void tdc_load(int a, int x0, int y0, int x1, int y1) {
    int slot = g_tdc_active[a].slot, w = g_tdc[slot].w, h = g_tdc[slot].h, any = 0;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > w) x1 = w;
    if (y1 > h) y1 = h;
    for (int ty = y0 / TDC_TILE; ty * TDC_TILE < y1 && ty < TDC_TH; ty++)
        for (int tx = x0 / TDC_TILE; tx * TDC_TILE < x1 && tx < TDC_TW; tx++)
            if (!g_tdc_tiles[ty][tx]) { g_tdc_tiles[ty][tx] = 2; any = 1; }
    if (any) { g_tdc_any = 1; tdc_copy_tiles(a, 0); }
}
static void tdc_store(int a) {   // the loaded tiles back into the surface
    if (g_tdc_any) tdc_copy_tiles(a, 1);
    g_tdc_any = 0;
}
// the box a text output can touch: the text's extent placed by the DC's alignment, the clip/opaque rectangle,
// and a margin for overhangs
static void tdc_text_box(HDC dc, int x, int y, SIZE sz, UINT opts, const RECT *rc, RECT *box) {
    UINT al = GetTextAlign(dc);
    TEXTMETRICA tm;
    if (!GetTextMetricsA(dc, &tm)) memset(&tm, 0, sizeof tm);
    if (al & TA_UPDATECP) { POINT cp; GetCurrentPositionEx(dc, &cp); x = cp.x; y = cp.y; }
    int x0 = (al & TA_CENTER) == TA_CENTER ? x - sz.cx / 2 : (al & TA_RIGHT) ? x - sz.cx : x;
    int y0 = (al & TA_BASELINE) == TA_BASELINE ? y - tm.tmAscent : (al & TA_BOTTOM) ? y - sz.cy : y;
    box->left = x0 - 8 - tm.tmOverhang; box->right = x0 + sz.cx + 8 + tm.tmOverhang;
    box->top = y0 - 4; box->bottom = y0 + sz.cy + 4;
    if (rc && (opts & (ETO_OPAQUE | ETO_CLIPPED))) {
        if (opts & ETO_CLIPPED) {   // nothing outside the clip rectangle
            if (box->left < rc->left) box->left = rc->left;
            if (box->top < rc->top) box->top = rc->top;
            if (box->right > rc->right) box->right = rc->right;
            if (box->bottom > rc->bottom) box->bottom = rc->bottom;
        }
        if (opts & ETO_OPAQUE) {    // the rectangle is filled
            if (rc->left < box->left) box->left = rc->left;
            if (rc->top < box->top) box->top = rc->top;
            if (rc->right > box->right) box->right = rc->right;
            if (rc->bottom > box->bottom) box->bottom = rc->bottom;
        }
    }
    POINT org;
    if (GetViewportOrgEx(dc, &org)) OffsetRect(box, org.x, org.y);
}
// A character of a bitmap font's atlas (while the mod builds one): the engine draws white on black into a 16-bit
// surface (ARGB4444) - no font smoothing there - and cuts each character out by its advance width; its green
// channel becomes the alpha. Characters with a negative left bearing spilled into the neighbouring cell (a stroke
// under every "i"), and the sprites sample a texel beyond their cell (white dots left of "l" from the "k" before
// it). Instead the character comes from GetGlyphOutline (GGO_GRAY8_BITMAP: 65 coverage levels at every size -
// Tahoma's own table forbids Windows' smoothing for small sizes) and is written into its cell only, the cell's
// last column left empty.
static uint8_t g_glyph_buf[256 * 256];
static uint32_t pack_gray(int slot, int g) {   // gray level g (0-255) in the atlas DIB's format
    uint32_t v = 0;
    for (int c = 0; c < 3; c++) {
        DWORD m = g_tdc[slot].masks[c];
        if (!m) continue;
        int shift = 0, bits = 0;
        while (!(m >> shift & 1)) shift++;
        while (m >> (shift + bits) & 1) bits++;
        v |= ((uint32_t)(g * ((1 << bits) - 1) + 127) / 255) << shift;
    }
    return v;
}
static int atlas_glyph(int a, HDC dc, int x, int y, char ch, SIZE sz) {
    int slot = g_tdc_active[a].slot;
    if (sz.cx <= 1 || sz.cy <= 0 || g_tdc[slot].bpp != 16) return 0;
    static const MAT2 ident = { { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };
    GLYPHMETRICS gm;
    TEXTMETRICA tm;
    if (!GetTextMetricsA(dc, &tm)) return 0;
    UINT c = (uint8_t)ch;
    DWORD need = GetGlyphOutlineA(dc, c, GGO_GRAY8_BITMAP, &gm, 0, NULL, &ident);
    if (need == GDI_ERROR || need > sizeof g_glyph_buf) return 0;
    if (need && GetGlyphOutlineA(dc, c, GGO_GRAY8_BITMAP, &gm, need, g_glyph_buf, &ident) == GDI_ERROR) return 0;
    int w = g_tdc[slot].w, h = g_tdc[slot].h, cw = sz.cx - 1;   // the cell, its last column left empty
    for (int row = 0; row < sz.cy; row++)                         // the cell black first
        if (y + row >= 0 && y + row < h)
            for (int col = 0; col < cw; col++)
                if (x + col >= 0 && x + col < w)
                    ((uint16_t *)((uint8_t *)g_tdc[slot].bits + (y + row) * g_tdc[slot].pitch))[x + col] = (uint16_t)pack_gray(slot, 0);
    if (!need) return 1;   // a blank (space)
    int pitch = ((int)gm.gmBlackBoxX + 3) & ~3;
    int gx = x + gm.gmptGlyphOrigin.x, gy = y + tm.tmAscent - gm.gmptGlyphOrigin.y;
    for (int row = 0; row < (int)gm.gmBlackBoxY; row++) {
        int py = gy + row;
        if (py < y || py >= y + sz.cy || py < 0 || py >= h) continue;
        uint16_t *dst = (uint16_t *)((uint8_t *)g_tdc[slot].bits + py * g_tdc[slot].pitch);
        for (int col = 0; col < (int)gm.gmBlackBoxX; col++) {
            int px = gx + col;
            if (px < x || px >= x + cw || px < 0 || px >= w) continue;
            int g = g_glyph_buf[row * pitch + col] * 255 / 64;
            dst[px] = (uint16_t)pack_gray(slot, g > 255 ? 255 : g);
        }
    }
    return 1;
}

static void atlas_dump(int n) {   // tests: the 256x256 DIB holds the last atlas a bitmap font drew
    for (int i = 0; i < 6; i++) {
        if (!g_tdc[i].dc || g_tdc[i].w != 256 || g_tdc[i].h != 256) continue;
        DDSURFACEDESC2 dd = { .dwSize = sizeof dd };
        dd.dwWidth = 256; dd.dwHeight = 256; dd.lPitch = g_tdc[i].pitch; dd.lpSurface = g_tdc[i].bits;
        dd.ddpfPixelFormat.dwRGBBitCount = (DWORD)g_tdc[i].bpp;
        dd.ddpfPixelFormat.dwRBitMask = g_tdc[i].masks[0];
        dd.ddpfPixelFormat.dwGBitMask = g_tdc[i].masks[1];
        dd.ddpfPixelFormat.dwBBitMask = g_tdc[i].masks[2];
        char path[MAX_PATH];
        wsprintfA(path, "aimod_dumps\\atlas%d.bmp", n);
        write_desc_bmp(&dd, path);
        if (g_log) fprintf(g_log, "# text bitmap: %s, %d bit, masks %04lx %04lx %04lx\n", path, g_tdc[i].bpp,
                           g_tdc[i].masks[0], g_tdc[i].masks[1], g_tdc[i].masks[2]);
    }
}

static BOOL WINAPI hook_ext_text_out(HDC dc, int x, int y, UINT opts, const RECT *rc, LPCSTR str, UINT n, const INT *dx) {
    int a = tdc_active_of(dc);
    if (a >= 0) {
        SIZE sz = { 0, 0 };
        if (dx) { for (UINT i = 0; i < n; i++) sz.cx += dx[i]; TEXTMETRICA tm; if (GetTextMetricsA(dc, &tm)) sz.cy = tm.tmHeight; }
        else GetTextExtentPoint32A(dc, str ? str : "", (int)n, &sz);
        RECT box;
        tdc_text_box(dc, x, y, sz, opts, rc, &box);
        tdc_load(a, box.left, box.top, box.right, box.bottom);
        if (g_fbm_making && !rc && n == 1 && str &&
            (GetTextAlign(dc) & (TA_CENTER | TA_BOTTOM | TA_BASELINE | TA_UPDATECP)) == 0 && atlas_glyph(a, dc, x, y, str[0], sz))
            return TRUE;
    }
    return g_ext_text_out(dc, x, y, opts, rc, str, n, dx);
}
static BOOL WINAPI hook_text_out_w(HDC dc, int x, int y, LPCWSTR str, int n) {
    int a = tdc_active_of(dc);
    if (a >= 0) {
        SIZE sz = { 0, 0 };
        GetTextExtentPoint32W(dc, str ? str : L"", n, &sz);
        RECT box;
        tdc_text_box(dc, x, y, sz, 0, NULL, &box);
        tdc_load(a, box.left, box.top, box.right, box.bottom);
    }
    return g_text_out_w(dc, x, y, str, n);
}
static long __fastcall hook_text_lock(uint8_t *surf, void *edx) {
    (void)edx;
    if (surf[0x5D] == 1) return 2;
    IDirectDrawSurface7 *dds = FIELD(surf, 0x48, IDirectDrawSurface7 *);
    int a = -1;
    for (int i = 0; i < 8; i++) if (!g_tdc_active[i].surf) { a = i; break; }
    if (!dds || a < 0 || !g_ext_text_out) return text_dc_fallback(surf, 0);
    DDSURFACEDESC2 *d = &g_tdc_active[a].d;
    memset(d, 0, sizeof *d);
    d->dwSize = sizeof *d;
    if (dds->lpVtbl->GetSurfaceDesc(dds, d) != DD_OK) return text_dc_fallback(surf, 0);
    int slot = text_dc_slot(d);
    if (slot < 0 || (int)d->dwWidth > (TDC_TW - 2) * TDC_TILE || (int)d->dwHeight > (TDC_TH - 2) * TDC_TILE)
        return text_dc_fallback(surf, 0);
    if (g_tdc_logged < 4 && g_log) {
        g_tdc_logged++;
        DDSCAPS2 caps = { 0 };
        dds->lpVtbl->GetCaps(dds, &caps);
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# text dc: surface %p %lux%lu, %lu bit, caps %08lx (%s), slot %d\n", (void *)surf, d->dwWidth, d->dwHeight,
                d->ddpfPixelFormat.dwRGBBitCount, caps.dwCaps, caps.dwCaps & DDSCAPS_VIDEOMEMORY ? "video memory" : "system memory", slot);
        LeaveCriticalSection(&g_lock);
    }
    g_tdc_active[a].surf = surf;
    g_tdc_active[a].slot = slot;
    FIELD(surf, 0x54, HDC) = g_tdc[slot].dc;
    surf[0x5D] = 1;
    return 0;
}
static long __fastcall hook_text_unlock(uint8_t *surf, void *edx) {
    (void)edx;
    int a = -1;
    for (int i = 0; i < 8; i++) if (g_tdc_active[i].surf == surf) { a = i; break; }
    if (a < 0) return text_dc_fallback(surf, 1);   // locked by the original (fallback) or not at all
    int slot = g_tdc_active[a].slot;
    GdiFlush();
    tdc_store(a);
    SelectObject(g_tdc[slot].dc, g_tdc_stock);
    g_tdc_active[a].surf = NULL;
    surf[0x5D] = 0;
    return 0;
}

static void *const k_fc_hooks[FC_N] = { (void *)hook_fc_text, (void *)hook_fc_text_rect, (void *)hook_fc_lock,
                                        (void *)hook_fc_text_lock, (void *)hook_fc_text_unlock };
// What a session accumulates: a slow game (13 fps at minute 103, the process at 620-690 MB) ran at 148 fps after
// a restart and loading its own autosave (350 MB). Counted for the minute line, since the session started: surfaces
// made by DX7Screen::CreateSurface and DX7Surface objects deleted (vtable slot 0, the deleting destructor), texture
// handles created / destroyed (DX7Rasterizer, virtual), with the process's private memory and handle count.
static volatile LONG g_surf_deleted, g_tex_created, g_tex_destroyed;
static void *g_surf_dtor[2], *g_tex_create[2], *g_tex_destroy[2];
static void *__fastcall hook_surf_dtor(void *surf, void *edx, unsigned flags) {
    (void)edx;
    InterlockedIncrement(&g_surf_deleted);
    return ((void *(__thiscall *)(void *, unsigned))g_surf_dtor[DISPLAY_DLL(surf)])(surf, flags);
}
static long __fastcall hook_tex_create(void *ras, void *edx, void *tex) {
    (void)edx;
    InterlockedIncrement(&g_tex_created);
    return ((long (__thiscall *)(void *, void *))g_tex_create[DISPLAY_DLL(ras)])(ras, tex);
}
static void __fastcall hook_tex_destroy(void *ras, void *edx, void *tex) {
    (void)edx;
    InterlockedIncrement(&g_tex_destroyed);
    ((void (__thiscall *)(void *, void *))g_tex_destroy[DISPLAY_DLL(ras)])(ras, tex);
}
static void session_counts_log(void) {
    typedef BOOL (WINAPI *MemInfoFn)(HANDLE, PROCESS_MEMORY_COUNTERS *, DWORD);
    static MemInfoFn mem_info;
    if (!mem_info) mem_info = (MemInfoFn)(void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "K32GetProcessMemoryInfo");
    PROCESS_MEMORY_COUNTERS_EX pm = { .cb = sizeof pm };
    DWORD handles = 0;
    GetProcessHandleCount(GetCurrentProcess(), &handles);
    if (!mem_info || !mem_info(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS *)&pm, sizeof pm)) pm.PrivateUsage = 0;
    if (!g_log) return;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# session: memory %lu MB, handles %lu; surfaces created %ld, deleted %ld; textures created %ld, destroyed %ld\n",
            (unsigned long)(pm.PrivateUsage >> 20), handles, g_cnt[2].calls, g_surf_deleted, g_tex_created, g_tex_destroyed);
    LeaveCriticalSection(&g_lock);
}

// ALLOCATION TRACE (tests only: autotest alloc_trace=<bytes>): which code makes the heap blocks a session piles up
// (DDrawCompat: ~1500-byte blocks, linked, each with the same DC and font handle, about one per text output - none
// with dgVoodoo). ntdll!RtlAllocateHeap is hot-patched (mov edi,edi + 5 bytes of padding); for requests within 80
// bytes below the given size the hook only copies 4 KB of the raw stack into a static buffer (no calls, no
// allocations inside the allocator); the minute line resolves the return addresses on them and logs the chains.
enum { AT_MAX = 24, AT_STACK = 1024 };
static int g_at_size;
static volatile LONG g_at_n, g_at_logged;
static uintptr_t g_at_stack[AT_MAX][AT_STACK];
static SIZE_T g_at_req[AT_MAX];
static int g_at_depth[AT_MAX];
static void *(WINAPI *g_rtl_alloc)(HANDLE, ULONG, SIZE_T);
static void *WINAPI hook_rtl_alloc(HANDLE heap, ULONG flags, SIZE_T size) {
    if ((int)size <= g_at_size && (int)size > g_at_size - 80 && g_at_n < AT_MAX && game_time() > 60000) {
        LONG k = InterlockedIncrement(&g_at_n) - 1;
        if (k < AT_MAX) {
            volatile uintptr_t *sp = (volatile uintptr_t *)__builtin_frame_address(0);
            volatile uintptr_t *top = (volatile uintptr_t *)(uintptr_t)__readfsdword(4);   // TEB StackBase
            int n = 0;
            for (; n < AT_STACK && sp + n < top; n++) g_at_stack[k][n] = sp[n];
            g_at_depth[k] = n;
            g_at_req[k] = size;
        }
    }
    return g_rtl_alloc(heap, flags, size);
}
static int alloc_trace_install(int size) {
    uint8_t *f = (uint8_t *)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlAllocateHeap");
    if (!f || f[0] != 0x8B || f[1] != 0xFF || f[-5] != 0xCC || f[-1] != 0xCC) return 0;
    DWORD old;
    if (!VirtualProtect(f - 5, 7, PAGE_EXECUTE_READWRITE, &old)) return 0;
    g_at_size = size;
    g_rtl_alloc = (void *(WINAPI *)(HANDLE, ULONG, SIZE_T))(void *)(f + 2);
    f[-5] = 0xE9;
    *(int32_t *)(f - 4) = (int32_t)((uintptr_t)hook_rtl_alloc - (uintptr_t)f);
    *(volatile uint16_t *)f = 0xF9EB;   // jmp short -7
    VirtualProtect(f - 5, 7, old, &old);
    FlushInstructionCache(GetCurrentProcess(), f - 5, 7);
    return 1;
}
static void alloc_trace_log(void) {
    LONG n = g_at_n < AT_MAX ? g_at_n : AT_MAX;
    if (!g_log || g_at_logged >= n) return;
    HMODULE nt = GetModuleHandleA("ntdll.dll");
    for (LONG k = g_at_logged; k < n; k++) {
        char line[1600];
        int o = snprintf(line, sizeof line, "# alloc trace %ld: %lu bytes:", k, (unsigned long)g_at_req[k]);
        int found = 0;
        if (k == 0) {   // raw check of the first copy
            int depth = 0;
            depth = g_at_depth[k];
            EnterCriticalSection(&g_lock);
            fprintf(g_log, "# alloc trace raw: %d dwords:", depth);
            for (int i = 0; i < 24 && i < depth; i++) fprintf(g_log, " %08lx", (unsigned long)g_at_stack[k][i]);
            fputc('\n', g_log);
            LeaveCriticalSection(&g_lock);
        }
        for (int i = 0; i < g_at_depth[k] && found < 14; i++) {
            uintptr_t a = g_at_stack[k][i];
            HMODULE m = NULL;
            if (!is_return_address(a) || !GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)a, &m) || m == nt) continue;
            char at[160];
            crash_addr(at, sizeof at, a);
            o += snprintf(line + o, sizeof line - o, " %s", at);
            found++;
        }
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "%s\n", line);
        LeaveCriticalSection(&g_lock);
    }
    g_at_logged = n;
}

static void frame_costs_log(unsigned frames) {
    session_counts_log();
    alloc_trace_log();
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    for (int k = 0; k < FC_N && g_log && frames; k++) {
        LONG calls = InterlockedExchange(&g_fc[k].calls, 0);
        LONG64 ticks = InterlockedExchange64(&g_fc[k].ticks, 0);
        if (!calls) continue;
        char by[400] = "";
        for (int r = 0; r < 3; r++) {   // the 3 most frequent callers
            int best = -1;
            for (int j = 0; j < 8; j++) if (g_fc[k].by[j].a && g_fc[k].by[j].n > 0 && (best < 0 || g_fc[k].by[j].n > g_fc[k].by[best].n)) best = j;
            if (best < 0) break;
            char at[160];
            crash_addr(at, sizeof at, g_fc[k].by[best].a);
            int o = lstrlenA(by);
            snprintf(by + o, sizeof by - o, "%s%s %.0f%%", r ? ", " : "", at, g_fc[k].by[best].n * 100.0 / calls);
            g_fc[k].by[best].n = -g_fc[k].by[best].n;
        }
        memset(g_fc[k].by, 0, sizeof g_fc[k].by);
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# frame costs: %-12s %6.1f calls, %5.2f ms per frame; from %s\n", g_fc[k].name, (double)calls / frames,
                (double)ticks * 1000.0 / (double)f.QuadPart / frames, by);
        LeaveCriticalSection(&g_lock);
    }
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
        if (g_frame_watch) {
            void **svt = (void **)GetProcAddress(m, "??_7DX7Surface@@6B@");
            void *tc = (void *)GetProcAddress(m, "?CreateTextureHandle@DX7Rasterizer@@MAEJPAVGETexture@@@Z");
            void *td = (void *)GetProcAddress(m, "?DestroyTextureHandle@DX7Rasterizer@@MAEXPAVGETexture@@@Z");
            if (svt && svt[0]) { g_surf_dtor[i] = svt[0]; n += patch_vtable((uintptr_t)svt, 0, (uintptr_t)svt[0], (void *)hook_surf_dtor); }
            if (tc) { g_tex_create[i] = tc; n += replace_rdata_ptr(m, tc, (void *)hook_tex_create); }
            if (td) { g_tex_destroy[i] = td; n += replace_rdata_ptr(m, td, (void *)hook_tex_destroy); }
        }
        g_text_lock_orig[i] = (void *)GetProcAddress(m, "?TextLock@DX7Surface@@UAEJXZ");
        g_text_unlock_orig[i] = (void *)GetProcAddress(m, "?TextUnlock@DX7Surface@@UAEJXZ");
        for (int k = 0; (g_frame_watch || (g_text_bitmap && k == FC_TEXT_RECT)) && k < FC_N; k++) {
            if (!g_frame_watch && k != FC_TEXT_RECT) continue;
            void *f = (void *)GetProcAddress(m, g_fc[k].export);
            if (f) { g_fc[k].orig[i] = f; n += replace_rdata_ptr(m, f, k_fc_hooks[k]); }
        }
        if (g_text_dc && !g_ext_text_out) {   // the real functions (DDrawCompat's hooks sit inside GDI itself)
            g_ext_text_out = (ExtTextOutAFn)(void *)GetProcAddress(GetModuleHandleA("gdi32.dll"), "ExtTextOutA");
            g_text_out_w = (TextOutWFn)(void *)GetProcAddress(GetModuleHandleA("gdi32.dll"), "TextOutW");
        }
        if (g_text_dc && g_ext_text_out) {
            int iat = patch_iat(m, "GDI32.dll", "ExtTextOutA", (void *)hook_ext_text_out);
            if (!iat) g_text_lock_orig[i] = NULL;   // no hook on the text output: keep the original DC
            n += iat;
            static int exe_done;
            if (!exe_done && g_text_out_w) {   // the exe's own TextOutW (IME window) may draw on the text DC too
                exe_done = 1;
                int w = patch_iat(GetModuleHandleA(NULL), "gdi32.dll", "TextOutW", (void *)hook_text_out_w);
                if (g_log) fprintf(g_log, "# text dc: exe TextOutW hook %d\n", w);
            }
        }
        if (g_text_dc && g_text_lock_orig[i] && g_text_unlock_orig[i]) {
            if (g_frame_watch) {   // the frame-cost hooks stay in front and call ours
                g_fc[FC_TEXT_LOCK].orig[i] = (void *)hook_text_lock;
                g_fc[FC_TEXT_UNLOCK].orig[i] = (void *)hook_text_unlock;
                n += 2;
            } else
                n += replace_rdata_ptr(m, g_text_lock_orig[i], (void *)hook_text_lock) +
                     replace_rdata_ptr(m, g_text_unlock_orig[i], (void *)hook_text_unlock);
        }
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
// aimod.ini [display] lock_mouse = 1 keeps the cursor inside the window while the game is in front; auto (default)
// only in borderless (a full-screen picture: edge scrolling must not slide onto another monitor), not in a window
// (Alt+Enter switches both). Native fullscreen needs nothing: the exclusive DirectInput mouse never leaves.
#define FN_MOUSE_UPDATE  0x00510E5D
#define FN_MOUSE_CLAMP   0x00510DAD
#define G_GAME_WINDOW    0x009197A4   // GEWindow* (+0 HWND)
#define IMP_IS_MOUSE     0x008235CC   // import: &ISMouse* in Low-Level Engine.dll
#define SITE_CLOSE_WINDOW_JE 0x00662666   // je over the CloseWindow call on WM_ACTIVATEAPP(0)
typedef void (__thiscall *ThisFn)(void *);
static ThisFn g_recompute_buttons;   // ISMouse::RecomputeCurrentButtonState
static int g_lock_mouse = 2;   // 0 never, 1 always, 2 auto (borderless only)
static int mouse_locked(void) { return g_lock_mouse == 1 || (g_lock_mouse == 2 && g_borderless); }
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
    ClipCursor(NULL);   // new rectangle, maybe no lock any more (lock_mouse=auto); the next frame clips again if locked
    if (g_log) { fprintf(g_log, "# display: %s %ldx%ld, mouse %s\n", g_borderless ? "borderless" : "window", r.right - r.left,
                         r.bottom - r.top, mouse_locked() ? "locked" : "free"); fflush(g_log); }
}

static void modern_frame(HWND h);
#define WM_AIMOD_FRAME (WM_APP + 0x2E46)
static LRESULT CALLBACK window_proc(HWND h, UINT msg, WPARAM w, LPARAM l) {
    if (g_headless) {   // the game never learns about focus: no suspend, no pause, no activation
        if (msg == WM_ACTIVATEAPP || msg == WM_ACTIVATE || msg == WM_SETCURSOR) return msg == WM_SETCURSOR;
        if (msg == WM_MOUSEACTIVATE) return MA_NOACTIVATE;
    }
    switch (msg) {
    case WM_AIMOD_FRAME:
        modern_frame(h);
        return 0;
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

// DDrawCompat switches the modern window frame off for the game (DWM non-client rendering and/or visual styles),
// leaving the Windows 98 look; switched on again for the game window (posted to the window's thread).
static void modern_frame(HWND h) {
    typedef DWORD (WINAPI *GetThemeAppPropsFn)(void);
    typedef void (WINAPI *SetThemeAppPropsFn)(DWORD);
    typedef HRESULT (WINAPI *SetWindowThemeFn)(HWND, LPCWSTR, LPCWSTR);
    typedef HRESULT (WINAPI *DwmSetAttrFn)(HWND, DWORD, const void *, DWORD);
    HMODULE ux = LoadLibraryA("uxtheme.dll"), dw = LoadLibraryA("dwmapi.dll");
    GetThemeAppPropsFn gp = ux ? (GetThemeAppPropsFn)(void *)GetProcAddress(ux, "GetThemeAppProperties") : NULL;
    SetThemeAppPropsFn sp = ux ? (SetThemeAppPropsFn)(void *)GetProcAddress(ux, "SetThemeAppProperties") : NULL;
    SetWindowThemeFn swt = ux ? (SetWindowThemeFn)(void *)GetProcAddress(ux, "SetWindowTheme") : NULL;
    DwmSetAttrFn sa = dw ? (DwmSetAttrFn)(void *)GetProcAddress(dw, "DwmSetWindowAttribute") : NULL;
    if (gp && sp && !(gp() & 1)) sp(gp() | 1 | 2 | 4);   // STAP_ALLOW_NONCLIENT | CONTROLS | WEBCONTENT
    if (swt) swt(h, NULL, NULL);                          // the window's default theme
    SetWindowRgn(h, NULL, FALSE);                         // DDrawCompat's window region: DWM draws no modern frame with one
    DWORD policy = 2;                                     // DWMNCRP_ENABLED
    HRESULT hr = sa ? sa(h, 2 /* DWMWA_NCRENDERING_POLICY */, &policy, sizeof policy) : E_NOTIMPL;
    SetWindowPos(h, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    typedef HRESULT (WINAPI *DwmGetAttrFn)(HWND, DWORD, void *, DWORD);
    DwmGetAttrFn ga = dw ? (DwmGetAttrFn)(void *)GetProcAddress(dw, "DwmGetWindowAttribute") : NULL;
    BOOL nc = -1;
    if (ga) ga(h, 1, &nc, sizeof nc);
    if (g_log) {
        char at[160] = "-";
        if (sa) crash_addr(at, sizeof at, (uintptr_t)sa);
        const uint8_t *pb = (const uint8_t *)sa;
        HRGN rgn = CreateRectRgn(0, 0, 0, 0);
        int rk = GetWindowRgn(h, rgn);
        RECT rb = { 0 };
        GetRgnBox(rgn, &rb);
        DeleteObject(rgn);
        fprintf(g_log, "# display: modern frame - set DWM policy hr %08lx, frame rendering now %d, ex style %08lx, window region %d (%ld,%ld-%ld,%ld), DwmSetWindowAttribute %s bytes %02x %02x %02x %02x %02x\n",
                (unsigned long)hr, nc, (unsigned long)GetWindowLongA(h, GWL_EXSTYLE), rk, rb.left, rb.top, rb.right, rb.bottom, at,
                pb ? pb[0] : 0, pb ? pb[1] : 0, pb ? pb[2] : 0, pb ? pb[3] : 0, pb ? pb[4] : 0);
        fflush(g_log);
    }
}

static void window_hook(HWND hwnd) {   // subclass once per window (present and mouse threads both call this)
    static volatile LONG busy;
    if (g_subclassed == hwnd || InterlockedExchange(&busy, 1)) return;
    if (g_subclassed != hwnd) {
        g_game_wndproc = (WNDPROC)GetWindowLongA(hwnd, GWL_WNDPROC);
        SetWindowLongA(hwnd, GWL_WNDPROC, (LONG)(uintptr_t)window_proc);
        g_subclassed = hwnd;
        if (g_borderless) PostMessageA(hwnd, WM_AIMOD_MODE, 0, 0);   // initial style/size, on the window's thread
        // (modern_frame: no effect - DDrawCompat covers the whole window, frame included, with its presentation window)
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
    if (mouse_locked() && fg && inside) {
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
        g_text_dc = GetPrivateProfileIntA("display", "text_dc", 1, aimod_ini);
        if (autotest_ini) g_text_dc = GetPrivateProfileIntA("autotest", "text_dc", g_text_dc, autotest_ini);
        g_text_bitmap = GetPrivateProfileIntA("display", "text_bitmap", 1, aimod_ini);
        if (autotest_ini) g_atlas_dump = GetPrivateProfileIntA("autotest", "atlas_dump", 0, autotest_ini);
        if (autotest_ini) g_text_bitmap = GetPrivateProfileIntA("autotest", "text_bitmap", g_text_bitmap, autotest_ini);
        if (g_text_bitmap) {
            g_text_bitmap = text_bitmap_install();
            if (g_log) fprintf(g_log, "# text bitmap: %s\n", g_text_bitmap ? "on (umlaut patch 2/2)" : "engine functions not found - off");
        }
        g_frame_watch = GetPrivateProfileIntA("display", "frame_watch", 35, aimod_ini);
        if (autotest_ini && (g_fw_test = GetPrivateProfileIntA("autotest", "frame_watch", -1, autotest_ini)) >= 0) g_frame_watch = g_fw_test;
        else g_fw_test = 0;
        int at = autotest_ini ? GetPrivateProfileIntA("autotest", "alloc_trace", 0, autotest_ini) : 0;
        if (at > 0 && g_log) fprintf(g_log, "# alloc trace: requests of %d-%d bytes, hook %d\n", at - 79, at, alloc_trace_install(at));
        int hooks = install_windowed_present();
        GetPrivateProfileStringA("display", "lock_mouse", "auto", tmp, sizeof tmp, aimod_ini);
        g_lock_mouse = !lstrcmpiA(tmp, "auto") ? 2 : atoi(tmp) ? 1 : 0;
        int input = install_windowed_input();
        g_smooth = GetPrivateProfileIntA("display", "smooth", 0, aimod_ini);
        GetPrivateProfileStringA("display", "present", "auto", tmp, sizeof tmp, aimod_ini);
        if (autotest_ini) { char at[32]; lstrcpynA(at, tmp, sizeof at); GetPrivateProfileStringA("autotest", "present", at, tmp, sizeof tmp, autotest_ini); }
        g_present_mode = !lstrcmpiA(tmp, "d3d") ? 1 : !lstrcmpiA(tmp, "gdi") ? 2 : !lstrcmpiA(tmp, "primary") ? 3 : 0;
        g_present_d3d = g_present_mode <= 1;
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
        // loading splash (thread 0x5344FB, a popup centred on screen): started (0x5343F8, jne at 0x534400) while the
        // flag is still 1 - before the engine loads the DLLs - but its stop (0x534433) checks the flag again and
        // would leave it on screen; stop it in windowed mode too (jne at 0x53443D -> nop). Headless test runs
        // (run_test.ps1) start the game suspended and patch the start to jmp, so there is no splash at all, and
        // then the stop must be skipped as well (it waits for the splash thread without a time limit).
        uint8_t *jne = (uint8_t *)0x0053443D, *jstart = (uint8_t *)0x00534400;
        int splash = 0, nosplash = jstart[0] == 0xEB;
        if (jne[0] == 0x75 && jne[1] == 0x19) {
            DWORD old;
            VirtualProtect(jne, 2, PAGE_EXECUTE_READWRITE, &old);
            if (nosplash) jne[0] = 0xEB; else jne[0] = jne[1] = 0x90;
            VirtualProtect(jne, 2, old, &old);
            FlushInstructionCache(GetCurrentProcess(), jne, 2);
            splash = 1;
        }
        if (g_log && g_headless) fprintf(g_log, "# display: headless (off-screen window, virtual mouse, %s)\n", nosplash ? "no splash" : "splash NOT prevented - started before the DLL loads");
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
    int other = 0;
    for (size_t i = 0; i < sizeof k_other_resolution_sites / sizeof *k_other_resolution_sites; i++)
        other += patch_call(k_other_resolution_sites[i], FN_SET_RESOLUTION, hook_other_resolution);
    int mm = redirect_minimap_scale();
    if (g_log) fprintf(g_log, "# editor: minimap scale operands %d/6, other resolution switches %d/7\n", mm, other);
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
// a stack value that is a return address: the bytes before it are a CALL (E8 rel32, FF /2 with register, [reg],
// [reg+disp8], [reg+disp32] or [disp32] operand, with or without SIB). Filters the data values on the stack
// that only happen to point into a module.
static int is_own_code(uintptr_t a) {   // this DLL (its present path)
    HMODULE m = NULL, self = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&is_own_code, &self);
    return GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)a, &m) &&
           m == self;
}
static int is_return_address(uintptr_t a) {
    HMODULE m = NULL;
    if (a < 0x10000 || !GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)a, &m))
        return 0;
    if (a - (uintptr_t)m < 0x1000) return 0;   // headers, not code (and the 7 bytes before must lie in the module)
    const uint8_t *p = (const uint8_t *)a;
    if (p[-5] == 0xE8) return 1;
    for (int len = 2; len <= 7; len++) {
        if (p[-len] != 0xFF) continue;
        uint8_t modrm = p[-len + 1];
        if ((modrm & 0x38) != 0x10) continue;   // /2 = call
        int mod = modrm >> 6, rm = modrm & 7, need = 2;
        if (mod != 3 && rm == 4) need++;        // SIB
        if (mod == 1) need += 1;
        else if (mod == 2 || (mod == 0 && rm == 5)) need += 4;
        if (need == len) return 1;
    }
    return 0;
}
static int is_system_module(HMODULE m) {
    char path[MAX_PATH];
    if (!m || !GetModuleFileNameA(m, path, sizeof path)) return 1;
    const char *b = strrchr(path, 0x5C);
    b = b ? b + 1 : path;
    static const char *const sys[] = { "ntdll.dll", "kernel32.dll", "KERNELBASE.dll", "user32.dll", "win32u.dll",
                                       "gdi32.dll", "gdi32full.dll", "msvcrt.dll", "ucrtbase.dll" };
    for (size_t i = 0; i < sizeof sys / sizeof *sys; i++) if (!lstrcmpiA(b, sys[i])) return 1;
    return 0;
}

static void cmd_profile_render(int dur, char *out) {
    DWORD tid = g_render_thread;
    HANDLE h = tid ? OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT, FALSE, tid) : NULL;
    if (!h) { REPLY("error: render thread unknown (windowed mode only)\n"); return; }
    enum { HB = 4096 };
    static struct { uintptr_t a; int n; } callers[HB], gdi_callers[HB], gdi_game[HB];
    struct { HMODULE m; int n; } tops[24];
    int nt = 0, samples = 0, gdi_samples = 0;
    memset(callers, 0, sizeof callers);
    memset(gdi_callers, 0, sizeof gdi_callers);
    memset(gdi_game, 0, sizeof gdi_game);
    DWORD end = GetTickCount() + (DWORD)dur;
    timeBeginPeriod(1);
    while ((LONG)(end - GetTickCount()) > 0) {
        if (SuspendThread(h) != (DWORD)-1) {
            CONTEXT c = { .ContextFlags = CONTEXT_CONTROL };
            static uintptr_t stack[2048];   // 8 KB: DDrawCompat's call chains are deep
            SIZE_T got = 0;
            BOOL ok = GetThreadContext(h, &c);
            if (ok && !ReadProcessMemory(GetCurrentProcess(), (void *)(uintptr_t)c.Esp, stack, sizeof stack, &got))
                ReadProcessMemory(GetCurrentProcess(), (void *)(uintptr_t)c.Esp, stack, 2048, &got);   // near the stack base
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
                for (size_t i = 0; !caller && i < got / sizeof *stack; i++)
                    if (is_game_code(stack[i]) && is_return_address(stack[i])) caller = stack[i];
                uintptr_t a = caller & ~(uintptr_t)15;
                unsigned k = (unsigned)(a >> 4) * 2654435761u % HB;
                for (int p = 0; p < HB; p++, k = (k + 1) % HB)
                    if (!callers[k].a || callers[k].a == a) { callers[k].a = a; callers[k].n++; break; }
                if (m && (m == GetModuleHandleA("gdi32full.dll") || m == GetModuleHandleA("gdi32.dll") || m == GetModuleHandleA("win32u.dll"))) {
                    gdi_samples++;   // who called into GDI: the first return address outside Windows' own DLLs
                    uintptr_t g = 0;
                    for (size_t i = 0; !g && i < got / sizeof *stack; i++) {
                        HMODULE sm = NULL;
                        if (is_return_address(stack[i]) &&
                            GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)stack[i], &sm) &&
                            !is_system_module(sm)) g = stack[i];
                    }
                    unsigned q = (unsigned)(g >> 2) * 2654435761u % HB;
                    for (int p = 0; p < HB; p++, q = (q + 1) % HB)
                        if (!gdi_callers[q].a || gdi_callers[q].a == g) { gdi_callers[q].a = g; gdi_callers[q].n++; break; }
                    uintptr_t gg = 0;   // and the game code that led there
                    for (size_t i = 0; !gg && i < got / sizeof *stack; i++)
                        if ((is_game_code(stack[i]) || is_own_code(stack[i])) && is_return_address(stack[i])) gg = stack[i];
                    q = (unsigned)(gg >> 2) * 2654435761u % HB;
                    for (int p = 0; p < HB; p++, q = (q + 1) % HB)
                        if (!gdi_game[q].a || gdi_game[q].a == gg) { gdi_game[q].a = gg; gdi_game[q].n++; break; }
                }
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
    if (!gdi_samples) return;
    REPLY("  in GDI %d samples (%d%%), called from (exact return addresses):\n", gdi_samples, samples ? gdi_samples * 100 / samples : 0);
    for (int r = 0; r < 12; r++) {
        int best = -1;
        for (int k = 0; k < HB; k++) if (gdi_callers[k].n > 0 && (best < 0 || gdi_callers[k].n > gdi_callers[best].n)) best = k;
        if (best < 0) break;
        char at[160];
        if (gdi_callers[best].a) crash_addr(at, sizeof at, gdi_callers[best].a); else lstrcpyA(at, "(only Windows code on the stack)");
        REPLY("    %5.1f%%  %s\n", gdi_samples ? gdi_callers[best].n * 100.0 / gdi_samples : 0.0, at);
        gdi_callers[best].n = -gdi_callers[best].n;
    }
    REPLY("  the game code that led into GDI (innermost game return address):\n");
    for (int r = 0; r < 12; r++) {
        int best = -1;
        for (int k = 0; k < HB; k++) if (gdi_game[k].n > 0 && (best < 0 || gdi_game[k].n > gdi_game[best].n)) best = k;
        if (best < 0) break;
        char at[160];
        if (gdi_game[best].a) crash_addr(at, sizeof at, gdi_game[best].a); else lstrcpyA(at, "(no game code on the stack)");
        REPLY("    %5.1f%%  %s\n", gdi_samples ? gdi_game[best].n * 100.0 / gdi_samples : 0.0, at);
        gdi_game[best].n = -gdi_game[best].n;
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

// topbar [0|1|2]: the in-game top bar, drawn by FUN_0064FD4E of EEUIFMainGame (the current form of the UI manager
// [0x9197A0] while playing, UIManager::GetCurrentForm import 0x823D60): +0x5D8 = 0 the age, 1 the game clock,
// 2 "FPS: n" (relabelled every update - each new text is drawn with GDI)
static void cmd_topbar(char *out, const char *args) {
    void *uim = *(void **)0x009197A0;
    uint8_t *form = uim ? ((uint8_t *(__thiscall *)(void *))*(void **)0x00823D60)(uim) : NULL;
    const char *rn = form ? rtti_name(form) : "";
    if (!form || !strstr(rn, "EEUIFMainGame")) { REPLY("error: the current form is %s, not the game screen\n", form ? rn : "none"); return; }
    int v = -1;
    if (sscanf(args, "%d", &v) == 1 && v >= 0 && v <= 2) FIELD(form, 0x5D8, int) = v;
    REPLY("in-game form %p: top bar mode %d (0 age, 1 clock, 2 FPS)\n", (void *)form, FIELD(form, 0x5D8, int));
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
    if (g_subclassed) {   // window frame state (DDrawCompat switches the modern frame off)
        typedef DWORD (WINAPI *GetThemeAppPropsFn)(void);
        typedef BOOL (WINAPI *IsThemeFn)(void);
        typedef HANDLE (WINAPI *GetWindowThemeFn)(HWND);
        typedef HRESULT (WINAPI *DwmGetAttrFn)(HWND, DWORD, void *, DWORD);
        HMODULE ux = LoadLibraryA("uxtheme.dll"), dw = LoadLibraryA("dwmapi.dll");
        GetThemeAppPropsFn props = ux ? (GetThemeAppPropsFn)(void *)GetProcAddress(ux, "GetThemeAppProperties") : NULL;
        IsThemeFn active = ux ? (IsThemeFn)(void *)GetProcAddress(ux, "IsThemeActive") : NULL;
        IsThemeFn app = ux ? (IsThemeFn)(void *)GetProcAddress(ux, "IsAppThemed") : NULL;
        GetWindowThemeFn wt = ux ? (GetWindowThemeFn)(void *)GetProcAddress(ux, "GetWindowTheme") : NULL;
        DwmGetAttrFn ga = dw ? (DwmGetAttrFn)(void *)GetProcAddress(dw, "DwmGetWindowAttribute") : NULL;
        BOOL nc = -1;
        if (ga) ga(g_subclassed, 1 /* DWMWA_NCRENDERING_ENABLED */, &nc, sizeof nc);
        HRGN rgn = CreateRectRgn(0, 0, 0, 0);
        int rk = GetWindowRgn(g_subclassed, rgn);
        DeleteObject(rgn);
        REPLY("frame: theme app properties %lx, theme active %d, app themed %d, window theme %p, DWM frame rendering %d, window region %d\n",
              props ? props() : 0xFFFF, active ? active() : -1, app ? app() : -1, wt ? (void *)wt(g_subclassed) : NULL, nc, rk);
    }
    REPLY("present path: %s (DDrawCompat %s); primary Blt %ld frames, %ld failed, last hr %08lx\n",
          g_present_mode == 3 || (g_present_mode == 0 && ddrawcompat_loaded()) ? "primary Blt"
          : g_present_d3d && !g_d3d.failed ? "Direct3D 11" : "GDI", ddrawcompat_loaded() ? "loaded" : "not loaded",
          g_primary_frames, g_primary_fail, (unsigned long)g_primary_hr);
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

static void cmd_textlog(char *out, const char *args) {
    int n = atoi(args);
    InterlockedExchange(&g_textlog, n > 0 ? n : 100);
    REPLY("logging the next %ld text draws (fonts cached %d, dropped %d)\n", g_textlog, g_fbm_n, g_fbm_dropped);
}
