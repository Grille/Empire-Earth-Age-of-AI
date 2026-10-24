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


internal int g_editor_w, g_editor_h, g_editor_game;   // 0 = leave
internal int g_editor_layout;                         // a resolution set: the form's layout moved before it is built

internal void editor_size(int *w, int *h) {
    int nw = g_editor_game ? game_res_w() : g_editor_w;
    int nh = g_editor_game ? game_res_h() : g_editor_h;
    if (nw >= 800 && nh >= 600) { *w = nw; *h = nh; }
}

// Minimap picture: EEUIMiniMap (0x65DF30..) sizes it as screen width * 1/800 and height * 1/600 (constants
// 0x8358A8 / 0x82CBCC, also used to map clicks on it) - built for frames that scale with the screen like the
// in-game one. The editor's frame has AoC's fixed size (1.28 x the 800x600 one), so at other resolutions the
// picture overshot it (only its upper left part visible). The 6 operands read our factors instead: in the
// editor 1.28 / width and 1.28 / height (= the originals at 1024x768), elsewhere the originals.
internal float g_mm_fx = 1.0f / 800.0f, g_mm_fy = 1.0f / 600.0f;
internal int redirect_minimap_scale(void) { return patch_minimap_scale(&g_mm_fx, &g_mm_fy); }

internal void __stdcall hook_editor_resolution(int w, int h, int force, int flag) {
    memset(g_players, 0, sizeof g_players);   // into the editor (also back from a test run): the game's players are gone
    editor_size(&w, &h);
    int lay = apply_editor_layout(w, h);
    g_mm_fx = 1.28f / (float)w;
    g_mm_fy = 1.28f / (float)h;
    if (g_log) { log_locked("# editor: resolution %dx%d, layout patches %d/%d\n", w, h, lay, (int)N_LAYOUT + 6); }
    set_resolution(w, h, force, flag);
}

// every other resolution switch (game, menus, test play from the editor): the original minimap factors
internal void __stdcall hook_other_resolution(int w, int h, int force, int flag) {
    g_mm_fx = 1.0f / 800.0f;
    g_mm_fy = 1.0f / 600.0f;
    set_resolution(w, h, force, flag);
}

// The editor's main form (constructor 0x58D3E1) is built once, on first use (0x65B9F5) - before the resolution
// switch - and keeps its layout; its sub-pages are built later. The layout is computed right before it.
// The arrow keys: the game's form registers them (four EEUIEKeyboardScroll hotkeys, 0x64D0AB), the editor's
// never did (in neither build: it scrolls only at the screen edge and by dragging) - given to it the same way.
internal void *__thiscall hook_editor_form(void *form, void *owner) {
    if (g_editor_layout) {
        int w = 800, h = 600;
        editor_size(&w, &h);
        int lay = apply_editor_layout(w, h);
        if (g_log) { log_locked("# editor: building the editor for %dx%d, layout patches %d/%d\n", w, h, lay, (int)N_LAYOUT + 6); }
    }
    void *r = editor_form(form, owner);
    static const unsigned long keys[4] = { 0xC8, 0xD0, 0xCB, 0xCD };   // DIK_UP, DOWN, LEFT, RIGHT: directions 0..3
    int n = 0;
    for (int i = 0; i < 4; i++) {
        void *ev = key_scroll_new(i);
        if (ev) { uiform_add_hotkey(form, keys[i], ev); n++; }
    }
    if (g_log) { log_locked("# editor: arrow keys scroll %d/4\n", n); }
    return r;
}

// Windowed mode: the engine still has the windowed path SSSI used during development - with the flag byte
// 0x91945a = 0 the game creates a normal titled window (DX7Screen::CreateAWindow mode 1), takes DirectDraw in
// DDSCL_NORMAL instead of exclusive mode (Capture), never changes the display mode (SetMode checks the screen's
// fullscreen flag), presents by blitting into the client area instead of flipping (ShowScene), and
// minimizes/restores with ShowWindow. The byte is already 1 when the DLLs load and nothing writes it later, so
// clearing it here switches every reader (including dreXmod's hook on the loading screen at 0x534433).
// aimod.ini [display] windowed = 0 | 1
internal uint8_t g_fullscreen = 1;
internal int g_present_every;   // autotest present_every (diagnostics)
internal volatile DWORD g_render_thread;   // thread that calls ShowScene (profile render)
internal int g_diag;   // diagnostic sampling (aimod.ini [display] diag=1 or autotest diag_hooks=1)

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
internal struct { const char *dll; uintptr_t lo, hi; ShowSceneFn show; SetModeFn set_mode; } g_show[2] = {
    { "DX7HRDisplay.dll", 0, 0, NULL, NULL }, { "DX7HRTnLDisplay.dll", 0, 0, NULL, NULL },
};
#define DISPLAY_DLL(obj) (*(uintptr_t *)(obj) >= g_show[0].lo && *(uintptr_t *)(obj) < g_show[0].hi ? 0 : 1)
internal volatile LONG g_present_frames, g_present_fail;

// Borderless fullscreen and DPI. aimod.ini [display] windowed = borderless: a frameless window over the whole
// monitor; Alt+Enter switches between that and a normal window. The game keeps rendering at its resolution
// and the image is scaled into the window (aspect kept, black bars). dpi_aware = 1 (default): the window is
// created DPI-aware (CreateWindowExA in the display DLLs, per-thread context), so Windows doesn't stretch it
// (blurry at 125/150 % display scaling) and its pixels are real screen pixels.
// Headless tests (autotest headless=1): the window is created off-screen, never activates and ignores focus
// changes; the game cursor and clicks come from the control pipe (vmouse/vclick) and screenshots are taken from
// the presented frame (shot) - nothing on the user's screen, mouse or keyboard is touched.
internal int g_headless;
internal volatile LONG g_shot_req, g_shot_done;
internal char g_shot_path[MAX_PATH];
internal int write_desc_bmp(const DDSURFACEDESC2 *d, const char *path);
internal int g_borderless;   // current mode: 1 borderless, 0 normal window
internal int g_smooth;       // [display] smooth = 1: filtered scaling (HALFTONE) instead of nearest pixel
internal int g_dpi_windows;
internal void window_hook(HWND hwnd);
typedef void *(WINAPI *SetThreadDpiFn)(void *);
typedef void *(WINAPI *GetWindowDpiFn)(HWND);
internal SetThreadDpiFn g_set_thread_dpi;
internal GetWindowDpiFn g_get_window_dpi;
// window geometry from the game's (DPI-unaware) threads in the window's own DPI context, else a DPI-aware
// window is reported scaled down
internal void *dpi_enter(HWND h) { return g_set_thread_dpi && g_get_window_dpi ? g_set_thread_dpi(g_get_window_dpi(h)) : NULL; }
internal void dpi_leave(void *prev) { if (prev) g_set_thread_dpi(prev); }

typedef HWND (WINAPI *CreateWindowExAFn)(DWORD, LPCSTR, LPCSTR, DWORD, int, int, int, int, HWND, HMENU, HINSTANCE, LPVOID);
internal HWND WINAPI hook_create_window(DWORD ex, LPCSTR cls, LPCSTR name, DWORD style, int x, int y, int w, int h,
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

internal int patch_iat(HMODULE m, const char *dll, const char *fn, void *hook) {
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
internal RECT view_rect(const RECT *cr, int w, int h) {
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
internal struct {
    const uint8_t *src; int sw, sh, spitch;
    uint32_t *dst; int dw, dh;
    int *xi; uint32_t *xw;   // per destination column: source column, weight 0..256
} g_scale;
internal HANDLE g_scale_go[8], g_scale_done[8];
internal SRWLOCK g_scale_lock = SRWLOCK_INIT;   // held by the caller from scale_frame until its buffer is copied
internal int g_scale_threads;

internal void scale_rows(int y0, int y1) {
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

internal DWORD WINAPI scale_worker(void *arg) {
    int i = (int)(intptr_t)arg, n = g_scale_threads + 1;
    for (;;) {
        WaitForSingleObject(g_scale_go[i], INFINITE);
        scale_rows(g_scale.dh * (i + 1) / n, g_scale.dh * (i + 2) / n);
        SetEvent(g_scale_done[i]);
    }
}

// scales the locked frame (32 bpp) to dw x dh into our buffer; returns it, or NULL (then GDI scales)
internal uint32_t *scale_frame(const DDSURFACEDESC2 *d, int dw, int dh) {
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
internal volatile LONG64 g_t_lock, g_t_blit, g_t_frame;
internal volatile LONG g_t_count;
internal LONGLONG qpc(void) { LARGE_INTEGER t; QueryPerformanceCounter(&t); return t.QuadPart; }

#include "present_d3d.c"

// present = auto (default): with DDrawCompat as ddraw.dll the frame goes onto the primary surface - DDrawCompat
// shows the primary over the game window itself (a frame drawn into the window directly stays hidden: black
// window), and unlike dgVoodoo's pass-through its windowed primary is desktop sized, so the Blt is safe. A GPU
// copy (back buffer -> primary at the window's client area, scaled, bars filled black, clipped to the window);
// the back buffer is only locked for a "shot". Otherwise Direct3D 11 (present_d3d.c), or gdi when set.
internal int g_present_mode;   // 0 auto, 1 d3d, 2 gdi, 3 primary
internal int ddrawcompat_loaded(void) {
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

internal volatile LONG g_primary_frames, g_primary_fail;
internal HRESULT g_primary_hr;
internal int present_primary(ee_rasterizer *ras, unsigned screen, ee_screen *scr, ee_surface *back, IDirectDrawSurface7 *b, HWND hwnd) {
    (void)ras; (void)screen;
    if (g_present_mode != 3 && !(g_present_mode == 0 && ddrawcompat_loaded())) return 0;
    ee_screen_state *st = ras_get_screen_state(ras, screen);
    ee_surface *front = scrstate_get_front(st);
    IDirectDrawSurface7 *f = front ? surf_get_dds(front) : NULL;
    IDirectDraw7 *dd = screen_get_ddraw(scr);
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
    IDirectDrawClipper *c = NULL;
    int has_clipper = f->lpVtbl->GetClipper(f, &c) == DD_OK;   // a primary made anew (device lost, mode change) has none
    if (has_clipper && c) c->lpVtbl->Release(c);
    if (clipped != hwnd || !has_clipper) {   // clip the primary to the game window (other windows stay untouched)
        c = NULL;
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

internal int present_to_window(ee_rasterizer *ras, unsigned screen) {
    ee_screen_state *st = ras_get_screen_state(ras, screen);
    ee_screen *scr = scrstate_get_screen(st); ee_surface *back = scrstate_get_back(st); ee_window *win = scrstate_get_window(st);
    if (!scr || screen_get_fullscreen(scr) || !back || !win) return 0;
    IDirectDrawSurface7 *s = surf_get_dds(back);
    HWND hwnd = window_get_hwnd(win);
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
internal volatile LONG g_lit_samples, g_lit_frames, g_lock_fail;
internal void sample_back(ee_rasterizer *ras, unsigned screen) {
    ee_screen_state *st = ras_get_screen_state(ras, screen);
    ee_surface *back = scrstate_get_back(st);
    IDirectDrawSurface7 *s = back ? surf_get_dds(back) : NULL;
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
internal TMGetTimeFn g_tm_get_time;
internal void frame_stats(ee_rasterizer *ras, unsigned screen) {
    ee_screen_state *st = ras_get_screen_state(ras, screen);
    if (!st || ++SS_U(st, SS_FRAMES) != 12) return;
    if (!g_tm_get_time) g_tm_get_time = (TMGetTimeFn)(void *)GetProcAddress(GetModuleHandleA("Low-Level Engine.dll"), "?TMGetTime@@YAKXZ");
    if (!g_tm_get_time) return;
    float sum = SS_F(st, SS_SUM);
    unsigned a = SS_U(st, SS_A), frames = SS_U(st, SS_FRAMES), b = SS_U(st, SS_B), c = SS_U(st, SS_C);
    SS_F(st, SS_SUM_AVG) = sum / 12.0f;
    SS_F(st, SS_A_AVG) = (float)a / 12.0f;
    SS_F(st, SS_B_AVG) = (float)b / 12.0f;
    SS_F(st, SS_C_AVG) = (float)c / 12.0f;
    DWORD now = g_tm_get_time(), dt = now - SS_U(st, SS_LAST);
    float k = dt ? 1000.0f / (float)dt : 0.0f;
    SS_F(st, SS_SUM_RATE) = sum * k;
    SS_F(st, SS_A_RATE) = (float)a * k;
    SS_F(st, SS_FPS) = (float)frames * k;
    SS_F(st, SS_B_RATE) = (float)b * k;
    SS_U(st, SS_LAST) = now;
    SS_F(st, SS_SUM) = 0.0f;
    SS_U(st, SS_A) = SS_U(st, SS_FRAMES) = SS_U(st, SS_B) = SS_U(st, SS_C) = 0;
}

#include "display_diag.c"

internal int __thiscall hook_show_scene(void *ras, unsigned screen) {
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

internal int __thiscall hook_set_mode(void *scr, unsigned mode) {
    if (screen_get_fullscreen(scr)) return g_show[DISPLAY_DLL(scr)].set_mode(scr, mode);
    screen_set_mode(scr, mode);
    return 0;
}

// GetCurrentMode / GetCurrentModeIndex: the index the constructor takes from the mode enumeration can be out of
// range windowed (the game reads the current mode before its first SetMode - crash at 0x535958); clamp it.
// Modes: vector of 0x14-byte GEVideoMode at +0x10 (begin) / +0x14 (end).
internal unsigned screen_mode_index(ee_screen *scr) {
    unsigned n = (unsigned)(screen_get_modes_end(scr) - screen_get_modes_begin(scr)) / 0x14;
    if (!screen_get_fullscreen(scr) && screen_get_mode(scr) >= n) screen_set_mode(scr, 0);
    return screen_get_mode(scr);
}
internal void *__thiscall hook_get_current_mode(ee_screen *scr) {
    return screen_get_modes_begin(scr) + screen_mode_index(scr) * 0x14;
}
internal unsigned __thiscall hook_get_current_mode_index(ee_screen *scr) {
    return screen_mode_index(scr);
}

internal int replace_rdata_ptr(HMODULE m, void *fn, void *hook) {
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

#include "display_text.c"

internal void *const k_fc_hooks[FC_N] = { (void *)hook_fc_text, (void *)hook_fc_text_rect, (void *)hook_fc_lock,
                                        (void *)hook_fc_text_lock, (void *)hook_fc_text_unlock };

internal int install_windowed_present(void) {
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
        // diagnostics (ddinfo counters; autotest diag_skip leaves terrain/water/fog out): only with [display] diag=1
        // or autotest diag_hooks / diag_skip - for players they were per-draw hooks with nothing to show
        int diag = g_diag || g_diag_skip;
        void *dt = diag ? (void *)GetProcAddress(m, "?Draw@DX7Rasterizer@@UAEJAAVGETerrainMesh@@K_N@Z") : NULL;
        if (dt) { g_draw_terrain[i] = (DrawTerrainFn)dt; n += replace_rdata_ptr(m, dt, (void *)hook_draw_terrain); }
        void *dw = diag ? (void *)GetProcAddress(m, "?DrawWater@DX7Rasterizer@@UAEJAAVGETerrainMesh@@K_N@Z") : NULL;
        if (dw) { g_draw_water[i] = (DrawTerrainFn)dw; n += replace_rdata_ptr(m, dw, (void *)hook_draw_water); }
        g_get_native_error = (GetErrFn)GetProcAddress(GetModuleHandleA("Low-Level Engine.dll"), "?ESGetNativeError@@YAKXZ");
        static const char *cnt_exports[3] = {
            "?CopyTo@DX7Surface@@UAEJPAVGESurface@@AAVGERectangle@@1_N@Z",
            "?CopyWithFilter@DX7Surface@@UAEJPAVGESurface@@AAVGERectangle@@1@Z",
            "?CreateSurface@DX7Screen@@UAEJAAUGESurfaceDescription@@PAPAVGESurface@@@Z",
        };
        void *sf = diag ? (void *)GetProcAddress(m, "?SetFog@DX7Rasterizer@@UAEXKK@Z") : NULL;
        void *sft = diag ? (void *)GetProcAddress(m, "?SetFogType@DX7Rasterizer@@UAEXKK@Z") : NULL;
        void *sfr = diag ? (void *)GetProcAddress(m, "?SetFogRange@DX7Rasterizer@@UAEXMMK@Z") : NULL;
        if (sf) { g_set_fog[i] = (SetFogFn)sf; n += replace_rdata_ptr(m, sf, (void *)hook_set_fog); }
        if (sft) { g_set_fog_type[i] = (SetFogFn)sft; n += replace_rdata_ptr(m, sft, (void *)hook_set_fog_type); }
        if (sfr) { g_set_fog_range[i] = (SetFogRangeFn)sfr; n += replace_rdata_ptr(m, sfr, (void *)hook_set_fog_range); }
        void *cl = diag ? (void *)GetProcAddress(m, "?Clear@DX7Rasterizer@@UAEJABVGEColor@@ABVGERectangle@@K@Z") : NULL;
        void *clz = diag ? (void *)GetProcAddress(m, "?ClearZBuffer@DX7Rasterizer@@UAEJMABVGERectangle@@K@Z") : NULL;
        if (cl) { g_clear[i] = (ClearFn)cl; n += replace_rdata_ptr(m, cl, (void *)hook_clear); }
        if (clz) { g_clearz[i] = (ClearZFn)clz; n += replace_rdata_ptr(m, clz, (void *)hook_clearz); }
        void *srt = diag ? (void *)GetProcAddress(m, "?SetRenderTarget@DX7Rasterizer@@MAEXPAVGESurface@@K@Z") : NULL;
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
            if ((k == 1 && !diag) || (k == 2 && !diag && !g_frame_watch)) continue;   // CopyTo: the front-copy fix
            void *f = (void *)GetProcAddress(m, cnt_exports[k]);
            if (f) { g_cnt[k].orig[i] = f; n += replace_rdata_ptr(m, f, cnt_hooks[k]); }
        }
    }
    return n;
}

// Windowed input. Fullscreen-era behaviour that breaks a window:
// - the mouse is a DirectInput device in exclusive mode (ISMouse::Resume, Low-Level Engine.dll RVA_LLE_MOUSE_COOP
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
typedef void (__thiscall *ThisFn)(void *);
internal ThisFn g_recompute_buttons;   // ISMouse::RecomputeCurrentButtonState
internal int g_lock_mouse = 2;   // 0 never, 1 always, 2 auto (borderless only)
internal int mouse_locked(void) { return g_lock_mouse == 1 || (g_lock_mouse == 2 && g_borderless); }
internal WNDPROC g_game_wndproc;
internal HWND g_subclassed;
internal volatile LONG g_mouse_updates, g_mouse_unacq, g_mouse_reacq;

internal void os_cursor_visible(void) {   // display count back to 0 for this thread
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
internal GetDpiFn g_get_dpi;
internal AdjustForDpiFn g_adjust_for_dpi;
internal RECT frame_for_dpi(UINT dpi) {   // WS_OVERLAPPEDWINDOW frame around an empty client
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
internal UINT window_dpi(HWND h) { frame_for_dpi(96); return g_get_dpi ? g_get_dpi(h) : 96; }

// a normal window with client cw x ch (the game resolution), in the window's DPI context. It always fits the
// monitor's work area: a resolution as big as the screen would make the window larger than the desktop, which
// Windows cuts and dgVoodoo then crashes on (DDRAW.dll+2B133) - the client is scaled down (aspect kept), the
// image is scaled into it. Position: kept (centered when coming from borderless), moved inside the work area.
internal RECT normal_window_rect(HWND h, int cw, int ch) {
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

internal RECT window_rect_for(HWND h) {
    if (g_borderless) {
        MONITORINFO mi = { .cbSize = sizeof mi };
        GetMonitorInfoA(MonitorFromWindow(h, MONITOR_DEFAULTTONEAREST), &mi);
        return mi.rcMonitor;
    }
    ee_window *win = game_window();
    return normal_window_rect(h, win ? window_get_width(win) : 1024, win ? window_get_height(win) : 768);
}

internal void apply_window_mode(HWND h) {
    if (IsIconic(h)) ShowWindow(h, SW_RESTORE);
    if (IsZoomed(h)) ShowWindow(h, SW_RESTORE);
    RECT r = window_rect_for(h);   // before the style change (reads where it comes from)
    LONG st = GetWindowLongA(h, GWL_STYLE);
    st = g_borderless ? (st & ~WS_OVERLAPPEDWINDOW) | WS_POPUP : (st & ~WS_POPUP) | WS_OVERLAPPEDWINDOW;
    SetWindowLongA(h, GWL_STYLE, st);
    SetWindowPos(h, HWND_TOP, r.left, r.top, r.right - r.left, r.bottom - r.top, SWP_FRAMECHANGED | SWP_NOOWNERZORDER | SWP_SHOWWINDOW);
    ClipCursor(NULL);   // new rectangle, maybe no lock any more (lock_mouse=auto); the next frame clips again if locked
    if (g_log) { log_locked("# display: %s %ldx%ld, mouse %s\n", g_borderless ? "borderless" : "window", r.right - r.left,
                         r.bottom - r.top, mouse_locked() ? "locked" : "free"); }
}

internal void modern_frame(HWND h);
#define WM_AIMOD_FRAME (WM_APP + 0x2E46)
internal LRESULT CALLBACK window_proc(HWND h, UINT msg, WPARAM w, LPARAM l) {
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
internal void modern_frame(HWND h) {
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
        log_locked("# display: modern frame - set DWM policy hr %08lx, frame rendering now %d, ex style %08lx, window region %d (%ld,%ld-%ld,%ld), DwmSetWindowAttribute %s bytes %02x %02x %02x %02x %02x\n",
                (unsigned long)hr, nc, (unsigned long)GetWindowLongA(h, GWL_EXSTYLE), rk, rb.left, rb.top, rb.right, rb.bottom, at,
                pb ? pb[0] : 0, pb ? pb[1] : 0, pb ? pb[2] : 0, pb ? pb[3] : 0, pb ? pb[4] : 0);
    }
}

internal void window_hook(HWND hwnd) {   // subclass once per window (present and mouse threads both call this)
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
internal BOOL WINAPI hook_set_window_pos(HWND h, HWND after, int x, int y, int cx, int cy, UINT flags) {
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

internal ee_input_mouse *is_mouse(void) { return input_mouse(); }

// clicks outside the client area must not reach the game; Acquire again re-reads the button state
internal void mouse_acquire(int want) {
    static int unacquired, up_frames;
    ee_input_mouse *ms = is_mouse();
    if (!ms) return;
    void **dev = imouse_get_device(ms);
    if (!dev || imouse_get_suspended(ms)) { unacquired = 0; return; }   // +4: suspended by the game (window inactive)
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
internal volatile LONG g_input_resumes;
internal void resume_if_suspended(void *d, DWORD *last) {   // the mouse or the keyboard (an ISDevice: its +4 the same)
    if (!d || !imouse_get_suspended(d) || GetTickCount() - *last < 250) return;   // +4: suspended (ISDevice)
    *last = GetTickCount();
    ((ThisFn)(*(void ***)d)[4])(d);   // Resume (vtable +0x10)
    InterlockedIncrement(&g_input_resumes);
}

// virtual mouse (headless): position in game coordinates from the control pipe; a click increments the button's
// transition counter (ISMouse +0x18 + button: odd = down; JustPressed compares it with last frame's copy +0x1C,
// which Refresh takes just before this hook) - down now, up 3 updates later. The device is never acquired
// (the window is never in front), so no real input interferes.
internal volatile LONG g_vm_x = -1, g_vm_y = -1, g_vm_button = -1, g_vm_step, g_vm_clicks;
internal void virtual_mouse(ee_mouse *m, int frozen) {
    if (g_vm_x >= 0 && !frozen) {
        mouse_set_x(m, (float)g_vm_x);
        mouse_set_y(m, (float)g_vm_y);
        mouse_clamp(m);
    }
    ee_input_mouse *ms = is_mouse();
    LONG b = g_vm_button;
    if (!ms || b < 0 || b > 3) return;
    uint8_t *st = imouse_buttons_at(ms) + b;
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

internal void window_mouse_in(ee_mouse *m, int frozen, ee_window *win, HWND hwnd);
internal void window_mouse(ee_mouse *m, int frozen) {
    ee_window *win = game_window();
    HWND hwnd = win ? window_get_hwnd(win) : NULL;
    if (!hwnd) return;
    window_hook(hwnd);
    if (g_headless) { virtual_mouse(m, frozen); return; }
    void *dpi = dpi_enter(hwnd);
    window_mouse_in(m, frozen, win, hwnd);
    dpi_leave(dpi);
}

internal void window_mouse_in(ee_mouse *m, int frozen, ee_window *win, HWND hwnd) {
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
        resume_if_suspended(input_mouse(), &last_mouse);
        resume_if_suspended(input_keyboard(), &last_keyboard);
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
    unsigned gw = (unsigned)window_get_width(win), gh = (unsigned)window_get_height(win);
    if (gw < 2 || gh < 2) return;
    RECT v = view_rect(&rc, (int)gw, (int)gh);   // borderless: the image may have bars
    if (v.right - v.left < 2 || v.bottom - v.top < 2) return;
    mouse_set_x(m, (float)(c.x - v.left) * (float)gw / (float)(v.right - v.left - 1));
    mouse_set_y(m, (float)(c.y - v.top) * (float)gh / (float)(v.bottom - v.top - 1));
    mouse_clamp(m);
    InterlockedIncrement(&g_mouse_updates);
}

internal void __thiscall hook_mouse_update(ee_mouse *m) {
    uint8_t frozen = mouse_get_frozen(m);
    mouse_set_frozen(m, 1);   // the original then only reads the wheel
    mouse_update(m);
    mouse_set_frozen(m, frozen);
    window_mouse(m, frozen);
}


internal int install_windowed_input(void) {
    int n = 0;
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
    if (lle) {
        n += patch_input_coop(lle);   // push 5 -> push 6 before SetCooperativeLevel
        g_recompute_buttons = (ThisFn)(void *)GetProcAddress(lle, "?RecomputeCurrentButtonState@ISMouse@@AAEXXZ");
    }
    n += patch_mouse_update(hook_mouse_update, NULL);
    n += patch_no_minimize();
    return n;
}

internal void player_name_install(void);
internal void display_install(const char *autotest_ini) {
    player_name_install();   // (defined at the end of this file)
    int dmode = g_set.display_mode;   // [display] mode: native 0, windowed 1, borderless 2 (rows.h)
    if (autotest_ini) g_headless = ini_int("autotest", "headless", 0, autotest_ini);
    if (g_headless) { dmode = 1; headless_windows(); }   // a normal (off-screen) window, never activated
    g_borderless = dmode == 2;
    int windowed = dmode != 0;
    if (autotest_ini) g_present_every = ini_int("autotest", "present_every", 0, autotest_ini);
    if (autotest_ini) g_diag_skip = ini_int("autotest", "diag_skip", 0, autotest_ini);
    g_diag = (autotest_ini && ini_int("autotest", "diag_hooks", 0, autotest_ini)) || g_set.display_diag;
    if (windowed) {
        uint8_t was = *fullscreen_var();
        if (was == 1) *fullscreen_var() = g_fullscreen = 0;
        g_text_dc = g_set.display_text_dc;
        g_text_bitmap = g_set.display_text_bitmap;
        if (autotest_ini) g_atlas_dump = ini_int("autotest", "atlas_dump", 0, autotest_ini);
        if (g_text_bitmap) {
            g_text_bitmap = text_bitmap_install();
            if (g_log) fprintf(g_log, "# text bitmap: %s\n", g_text_bitmap ? "on (umlaut patch 2/2)" : "engine functions not found - off");
        }
        g_frame_watch = g_set.display_frame_watch;
        g_fw_test = g_set_from[SETTING_display_frame_watch] == 2 ? g_frame_watch : 0;   // the test ini's own
        int hooks = install_windowed_present();
        g_lock_mouse = g_set.display_lock_mouse;   // 0, 1, auto 2 (rows.h)
        int input = install_windowed_input();
        g_smooth = g_set.display_smooth;
        g_present_mode = g_set.display_present;   // auto 0, d3d 1, gdi 2, primary 3 (rows.h)
        g_present_d3d = g_present_mode <= 1;
        g_vsync = g_set.display_vsync;
        int dpi = 0;
        if (g_set.display_dpi_aware) {
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
        int nosplash = 0, splash = patch_splash_stop(&nosplash);
        if (g_log && g_headless) fprintf(g_log, "# display: headless (off-screen window, virtual mouse, %s)\n", nosplash ? "no splash" : "splash NOT prevented - started before the DLL loads");
        if (g_log) fprintf(g_log, "# display: windowed %s (%s), present hooks %d, input patches %d/7, dpi hooks %d/2, window fit %s, splash stop %s\n",
                           was == 1 ? "on" : "NOT set (unexpected fullscreen flag)", g_borderless ? "borderless" : "window",
                           hooks, input, dpi, fit ? "ok" : "FAILED", splash ? "ok" : "FAILED");
    } else if (g_diag) {
        int hooks = install_windowed_present();   // all pass through in fullscreen; for the ddinfo counters
        if (g_log) fprintf(g_log, "# display: fullscreen, diagnostic hooks %d\n", hooks);
    }

    int form = patch_editor_form(hook_editor_form);   // the arrow keys, whatever the resolution
    if (g_log) fprintf(g_log, "# editor: form hook %d/1\n", form);
    const char *mode = g_set.editor_resolution;
    if (!lstrcmpiA(mode, "ee")) return;
    if (!lstrcmpiA(mode, "aoc")) { g_editor_w = 1024; g_editor_h = 768; }
    else if (!lstrcmpiA(mode, "game")) g_editor_game = 1;
    else if (sscanf(mode, "%dx%d", &g_editor_w, &g_editor_h) != 2 || g_editor_w < 800 || g_editor_h < 600) {
        if (g_log) fprintf(g_log, "# editor: bad resolution '%s' in aimod.ini, left at 800x600\n", mode);
        return;
    }
    g_editor_layout = 1;
    int ok = patch_editor_resolution(hook_editor_resolution);
    int other = patch_other_resolution(hook_other_resolution, NULL);
    int mm = redirect_minimap_scale();
    if (g_log) fprintf(g_log, "# editor: minimap scale operands %d/6, other resolution switches %d/7\n", mm, other);
    if (g_log) fprintf(g_log, "# editor: resolution=%s hooks %d/2\n", mode, ok);
}

// ddinfo [blit]: state of the DirectDraw presentation (for the windowed mode work). Rasterizer [0x919794],
// per-screen state +0x20[screen [0x919428]]: +0x2A8 DX7Screen (+0x30 IDirectDraw7, +0x3C attached to desktop,
// +0x3D fullscreen), +0x2AC front GESurface, +0x2B0 back GESurface (DX7Surface +0x48 IDirectDrawSurface7),
// +0x2BC GEWindow (+0 HWND, +4/+8 client origin floats, +0xC/+0x10 size). "blit" repeats ShowScene's Blt.
internal int dump_surface(IDirectDrawSurface7 *s, const char *path) {
    DDSURFACEDESC2 d = { .dwSize = sizeof d };
    if (s->lpVtbl->Lock(s, NULL, &d, DDLOCK_WAIT | DDLOCK_READONLY, NULL) != DD_OK) return 0;
    int r = write_desc_bmp(&d, path);
    s->lpVtbl->Unlock(s, NULL);
    return r;
}

internal int write_desc_bmp(const DDSURFACEDESC2 *dp, const char *path) {   // locked surface -> 24-bit BMP
    const DDSURFACEDESC2 d = *dp;
    int w = (int)d.dwWidth, h = (int)d.dwHeight, stride = (w * 3 + 3) & ~3, sbpp = (int)d.ddpfPixelFormat.dwRGBBitCount;
    if (w <= 0 || h <= 0 || !d.lpSurface || (sbpp != 16 && sbpp != 24 && sbpp != 32)) return 0;
    BITMAPINFOHEADER bi = { sizeof bi, w, h, 1, 24, BI_RGB, (DWORD)(stride * h), 0, 0, 0, 0 };
    struct { BITMAPINFOHEADER h; DWORD masks[3]; } src = {
        { sizeof src.h, d.lPitch / (sbpp / 8), -h, 1, (WORD)sbpp, sbpp == 16 ? BI_BITFIELDS : BI_RGB, 0, 0, 0, 0, 0 },
        { d.ddpfPixelFormat.dwRBitMask, d.ddpfPixelFormat.dwGBitMask, d.ddpfPixelFormat.dwBBitMask },
    };
    void *bits = NULL;
    HDC mdc = CreateCompatibleDC(NULL);
    HBITMAP bmp = mdc ? CreateDIBSection(mdc, (BITMAPINFO *)&bi, DIB_RGB_COLORS, &bits, NULL, 0) : NULL;
    if (!bmp || !bits) {   // no memory for the picture (a huge surface)
        if (bmp) DeleteObject(bmp);
        if (mdc) DeleteDC(mdc);
        return 0;
    }
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
internal int is_game_code(uintptr_t a) {
    HMODULE m = NULL;
    if (a < 0x400000 || !GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)a, &m)) return 0;
    return m == GetModuleHandleA(NULL) || m == GetModuleHandleA("Low-Level Engine.dll") || m == GetModuleHandleA("DX7HRDisplay.dll")
        || m == GetModuleHandleA("DX7HRTnLDisplay.dll");
}
// a stack value that is a return address: the bytes before it are a CALL (E8 rel32, FF /2 with register, [reg],
// [reg+disp8], [reg+disp32] or [disp32] operand, with or without SIB). Filters the data values on the stack
// that only happen to point into a module.
internal int is_own_code(uintptr_t a) {   // this DLL (its present path)
    HMODULE m = NULL, self = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)&is_own_code, &self);
    return GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)a, &m) &&
           m == self;
}
internal int is_return_address(uintptr_t a) {
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
internal int is_system_module(HMODULE m) {
    char path[MAX_PATH];
    if (!m || !GetModuleFileNameA(m, path, sizeof path)) return 1;
    const char *b = strrchr(path, 0x5C);
    b = b ? b + 1 : path;
    static const char *const sys[] = { "ntdll.dll", "kernel32.dll", "KERNELBASE.dll", "user32.dll", "win32u.dll",
                                       "gdi32.dll", "gdi32full.dll", "msvcrt.dll", "ucrtbase.dll" };
    for (size_t i = 0; i < sizeof sys / sizeof *sys; i++) if (!lstrcmpiA(b, sys[i])) return 1;
    return 0;
}

internal void cmd_profile_render(int dur, char *out) {
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

internal void cmd_profile(const char *args, char *out) {
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
internal void cmd_headless(const char *verb, const char *args, char *out) {
    if (!strcmp(verb, "shot")) {
        char name[64] = "shot";
        sscanf(args, "%63s", name);
        CreateDirectoryA("aimod_dumps", NULL);
        char dir[MAX_PATH];
        DWORD dn = GetFullPathNameA("aimod_dumps", sizeof dir, dir, NULL);
        if (!dn || dn >= sizeof dir || snprintf(g_shot_path, sizeof g_shot_path, "%s\\%s.bmp", dir, name) >= (int)sizeof g_shot_path) {
            REPLY("error: the picture's path is too long\n");   // (a deep game folder)
            return;
        }
        LONG before = g_shot_done;
        g_shot_req = 1;
        for (int i = 0; i < 300 && g_shot_done == before; i++) Sleep(10);
        if (g_shot_done != before) REPLY("ok: %s\n", g_shot_path);
        else { g_shot_req = 0; REPLY("error: no frame presented within 3 s\n"); }
        return;
    }
    if (!strncmp(args, "off", 3)) { g_vm_x = g_vm_y = -1; REPLY("ok: real mouse\n"); return; }
    if (!strcmp(verb, "wtype")) {   // wtype TEXT: the characters as WM_CHAR to the game window (edit boxes; tests)
        ee_window *win = game_window();
        HWND hwnd = win ? window_get_hwnd(win) : NULL;
        if (!hwnd) { REPLY("error: no window\n"); return; }
        int n = 0;
        for (const char *c = args; *c; c++, n++) {
            PostMessageA(hwnd, WM_CHAR, (WPARAM)(unsigned char)*c, 1);
            Sleep(30);
        }
        REPLY("ok: posted %d characters\n", n);
        return;
    }
    int x = -1, y = -1;
    char which[16] = "";
    if (sscanf(args, "%d %d %15s", &x, &y, which) < 2 || x < 0 || y < 0) { REPLY("usage: %s x y%s\n", verb, *verb == 'v' && verb[1] == 'c' ? " [right|mid]" : ""); return; }
    if (!strcmp(verb, "wclick")) {   // window messages (WONAPI screens such as the civ builder read those, not the game cursor)
        ee_window *win = game_window();
        HWND hwnd = win ? window_get_hwnd(win) : NULL;
        if (!hwnd) { REPLY("error: no window\n"); return; }
        RECT cr;
        GetClientRect(hwnd, &cr);
        RECT v = view_rect(&cr, window_get_width(win), window_get_height(win));
        int cx = v.left + x * (v.right - v.left) / window_get_width(win), cy = v.top + y * (v.bottom - v.top) / window_get_height(win);
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

internal void dd_surface(char *out, const char *name, ee_surface *ges) {
    IDirectDrawSurface7 *s = ges ? surf_get_dds(ges) : NULL;
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
internal void lock_unlock(char *out, const char *name, IDirectDrawSurface7 *s) {
    DDSURFACEDESC2 d = { .dwSize = sizeof d };
    HRESULT hr = s ? s->lpVtbl->Lock(s, NULL, &d, DDLOCK_WAIT | DDLOCK_READONLY, NULL) : E_POINTER;
    if (hr == DD_OK) s->lpVtbl->Unlock(s, NULL);
    REPLY("%s lock/unlock hr=%08lx\n", name, hr);
}
internal void cmd_ddpoke(char *out, const char *args) {
    ee_rasterizer *ras = rasterizer();
    ee_screen_state *st = ras ? ras_get_screen_state(ras, screen_index()) : NULL;
    if (!st) { REPLY("error: no rasterizer\n"); return; }
    ee_screen *scr = scrstate_get_screen(st); ee_surface *front = scrstate_get_front(st), *back = scrstate_get_back(st);
    IDirectDrawSurface7 *b7 = back ? surf_get_dds(back) : NULL;
    if (!strcmp(args, "z") && b7) {
        DDSCAPS2 zc = { .dwCaps = DDSCAPS_ZBUFFER };
        IDirectDrawSurface7 *z = NULL;
        if (b7->lpVtbl->GetAttachedSurface(b7, &zc, &z) == DD_OK) { lock_unlock(out, "z-buffer", z); z->lpVtbl->Release(z); }
    } else if (!strcmp(args, "back")) lock_unlock(out, "back", b7);
    else if (!strcmp(args, "front")) lock_unlock(out, "front", front ? surf_get_dds(front) : NULL);
    else if (!strcmp(args, "mode")) {
        IDirectDraw7 *dd = screen_get_ddraw(scr);
        DDSURFACEDESC2 m = { .dwSize = sizeof m };
        REPLY("GetDisplayMode hr=%08lx\n", dd->lpVtbl->GetDisplayMode(dd, &m));
    } else if (!strncmp(args, "cursor", 6)) {   // cursor <client x> <client y>: place the Windows cursor (game's coordinates)
        ee_window *win = game_window();
        POINT p = { 0, 0 }, q;
        RECT rc = { 0 };
        sscanf(args + 6, "%ld %ld", &p.x, &p.y);
        if (win) { GetClientRect(window_get_hwnd(win), &rc); ClientToScreen(window_get_hwnd(win), &p); }
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
internal void cmd_topbar(char *out, const char *args) {
    void *uim = ui_manager();
    ee_ui_form *form = uim ? ui_current_form(uim) : NULL;
    const char *rn = form ? rtti_name(form) : "";
    if (!form || !strstr(rn, "EEUIFMainGame")) { REPLY("error: the current form is %s, not the game screen\n", form ? rn : "none"); return; }
    int v = -1;
    if (sscanf(args, "%d", &v) == 1 && v >= 0 && v <= 2) uiform_set_top_bar(form, v);
    REPLY("in-game form %p: top bar mode %d (0 age, 1 clock, 2 FPS)\n", (void *)form, uiform_get_top_bar(form));
}

internal void cmd_ddinfo(char *out, const char *args) {
    ee_rasterizer *ras = rasterizer();
    int idx = screen_index();
    ee_screen_state *st = ras ? ras_get_screen_state(ras, idx) : NULL;
    if (!st) { REPLY("error: no rasterizer\n"); return; }
    ee_screen *scr = scrstate_get_screen(st); ee_window *win = scrstate_get_window(st);
    REPLY("screen %p fullscreen=%d desktop=%d flag=%d renderer %s\n", (void *)scr, screen_get_fullscreen(scr), screen_get_desktop(scr), g_fullscreen,
          g_show[DISPLAY_DLL(ras)].dll);
    {
        void **svt = *(void ***)scr;
        char d32 = ((char (__thiscall *)(void *))svt[3])(scr), t32 = ((char (__thiscall *)(void *))svt[4])(scr);
        REPLY("32-bit device support %d, 32-bit texture support %d; screen bytes 0x30..0x40:", d32, t32);
        for (int k = 0x30; k < 0x40; k++) REPLY(" %02x", ((uint8_t *)scr)[k]);
        REPLY("\n");
    }
    HWND hwnd = win ? window_get_hwnd(win) : NULL;
    RECT cr = {0};
    POINT o = {0, 0};
    if (hwnd) { GetClientRect(hwnd, &cr); ClientToScreen(hwnd, &o); }
    REPLY("window %p hwnd %p origin %.0f,%.0f size %ux%u; real client %ldx%ld at %ld,%ld\n", (void *)win, (void *)hwnd,
          win ? window_get_4(win) : 0, win ? window_get_8(win) : 0, win ? (unsigned)window_get_width(win) : 0,
          win ? (unsigned)window_get_height(win) : 0, cr.right, cr.bottom, o.x, o.y);
    ee_surface *front = scrstate_get_front(st), *back = scrstate_get_back(st);
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
    IDirectDraw7 *dd = screen_get_ddraw(scr);
    if (dd) {
        DDSURFACEDESC2 m = { .dwSize = sizeof m };
        HRESULT hr = dd->lpVtbl->GetDisplayMode(dd, &m);
        DEVMODEA dm = { .dmSize = sizeof dm };
        EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &dm);
        REPLY("ddraw display mode: hr=%08lx %lux%lu %lu bpp; real desktop %lux%lu %lu bpp; mode index %u of %u\n", hr,
              m.dwWidth, m.dwHeight, m.ddpfPixelFormat.dwRGBBitCount, dm.dmPelsWidth, dm.dmPelsHeight, dm.dmBitsPerPel,
              screen_get_mode(scr), (unsigned)((unsigned)(screen_get_modes_end(scr) - screen_get_modes_begin(scr)) / 0x14));
        uint32_t *md = (uint32_t *)(screen_get_modes_begin(scr) + screen_get_mode(scr) * 0x14);
        REPLY("current GEVideoMode: %u %u %u %u %u\n", md[0], md[1], md[2], md[3], md[4]);
    }
    REPLY("windowed present: %ld frames, %ld failed; back buffer before ShowScene: %ld samples, %ld lit of 1024 avg, lock failed %ld\n",
          g_present_frames, g_present_fail, g_lit_frames, g_lit_frames ? g_lit_samples / g_lit_frames : 0, g_lock_fail);
    ee_mouse *gm = game_mouse(); ee_input_mouse *ms = is_mouse();
    if (gm) REPLY("mouse: game cursor %.1f,%.1f of x %.0f..%.0f y %.0f..%.0f, updates %ld, unacquired %ld, reacquired %ld, "
                  "device %s, resumed by us %ld, subclassed %p\n", mouse_get_x(gm), mouse_get_y(gm), mouse_get_x_min(gm),
                  mouse_get_x_max(gm), mouse_get_y_min(gm), mouse_get_y_max(gm), g_mouse_updates, g_mouse_unacq,
                  g_mouse_reacq, ms ? (imouse_get_suspended(ms) ? "suspended" : "active") : "-", g_input_resumes, (void *)g_subclassed);
    {
        LARGE_INTEGER f;
        QueryPerformanceFrequency(&f);
        LONG n = InterlockedExchange(&g_t_count, 0);
        double tick_ms = 1000.0 / (double)f.QuadPart;
        LONG64 tl = InterlockedExchange64(&g_t_lock, 0), tb = InterlockedExchange64(&g_t_blit, 0), tf = InterlockedExchange64(&g_t_frame, 0);
        if (n) REPLY("present timing over %ld frames: frame %.2f ms, readback (Lock) %.2f ms, to the window %.2f ms, rest (game) %.2f ms\n",
                     n, tf * tick_ms / n, tl * tick_ms / n, tb * tick_ms / n, (tf - tl - tb) * tick_ms / n);
    }
    REPLY("frame stats (FPS display): %.1f frames/s, %.1f / %.1f per s\n", SS_F(st, SS_FPS), SS_F(st, SS_A_RATE), SS_F(st, SS_B_RATE));
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
        IDirectDrawSurface7 *f = surf_get_dds(front), *b = surf_get_dds(back);
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
        ee_surface *ges[2] = { front, back };
        const char *names[2] = { "front", "back" };
        for (int i = 0; i < 2; i++) {
            IDirectDrawSurface7 *s = ges[i] ? surf_get_dds(ges[i]) : NULL;
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
            FILE *f = bits ? fopen(path, "wb") : NULL;   // no DIB (no memory, an empty client area): nothing to write
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

internal void cmd_textstate(char *out, const char *args) {
    InterlockedExchange(&g_text_state, atoi(args));
    REPLY("textstate %ld\n", g_text_state);
}
internal void cmd_textlog(char *out, const char *args) {
    if (args[0] == '?') {
        REPLY("textlog: %ld lines to go, %ld seen\n", g_textlog, g_tl_n);
        return;
    }
    int n = atoi(args);
    InterlockedExchange(&g_tl_n, 0);
    InterlockedExchange(&g_textlog, n > 0 ? n : 100);
    REPLY("logging the next %ld text draws (fonts cached %d, dropped %d)\n", g_textlog, g_fbm_n, g_fbm_dropped);
}
