// DISPLAY TEXT: text as textures ([display] text_bitmap - the engine's bitmap fonts) and the own text DC
// ([display] text_dc - the font cache leak of DDrawCompat). Included by display.c (at this place in it).

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
            if (g_log) log_locked("# text bitmap: font %p destroyed - its entry dropped\n", (void *)font);
            break;
        }
    LeaveCriticalSection(&g_fbm_cs);
    return g_font_delete(font, flags);
}
// textlog N (pipe): the next N distinct text draws into the log (each combination once) - texture text for the back
// buffer (font, HFONT, the bitmap font used, the colour GEText +0x14: floats r g b and the size, the string +0x34)
// and GDI text (ExtTextOutA / TextOutW: on our text DC or another one, its surface, the colour) - for a screen
// whose text is missing
static volatile LONG g_textlog;
// The device state around our bitmap text: the engine's sprite text uses whatever the 3D scene left. In a big late
// game (the player's 70-minute save) the scene left fog on: every character came out in the fog colour - white text
// looked as always, black text (save screen title and labels, the overwrite and resign dialogs) vanished on the
// beige background, in 2 of 4 openings. So while it draws: fog off, alpha blending src alpha / inverse src alpha
// (restored after). textstate N (pipe, tests): 0 leave the state as it is, 1 that (default), 2 also texture stage 0
// = texture x diffuse (colour and alpha).
static volatile LONG g_text_state = 1;
static uint32_t g_tl_seen[1024];
static volatile LONG g_tl_n;
static uint32_t tl_hash(const void *p, size_t n, uint32_t h) {
    for (size_t i = 0; i < n; i++) h = (h ^ ((const uint8_t *)p)[i]) * 16777619u;
    return h;
}
static int textlog_new(uint32_t h) {   // 1 the first time (and still lines to log)
    if (g_textlog <= 0) return 0;
    for (LONG i = 0; i < g_tl_n && i < 1024; i++) if (g_tl_seen[i] == h) return 0;
    LONG k = InterlockedIncrement(&g_tl_n) - 1;
    if (k < 1024) g_tl_seen[k] = h;
    return InterlockedDecrement(&g_textlog) >= 0;
}
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
        if (g_log) log_locked("# text bitmap: back buffer %p %lux%lu - %s\n", surf, d.dwWidth, d.dwHeight,
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
                log_locked("# text bitmap: STALE font %p (HFONT %p, made for %p)\n", (void *)font, hf_now, g_fbm[i].hfont);
            LeaveCriticalSection(&g_fbm_cs);
            return r;
        }
    if (g_fbm_n >= 64) {
        static int full_logged;
        LeaveCriticalSection(&g_fbm_cs);
        if (g_log && !full_logged++) log_locked("# text bitmap: FULL (64 fonts) - GDI text for new fonts\n");
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
    if (g_textlog > 0 && g_log && text) {
        uint8_t *f = FIELD(text, 0x44, uint8_t *);
        const char *str = FIELD(text, 0x34, const char *);
        uint32_t *c = (uint32_t *)((uint8_t *)text + 0x14);
        int ok = str && mem_readable(str, 1);
        uint32_t h = tl_hash(&f, sizeof f, 2166136261u);
        h = tl_hash(&bmp, sizeof bmp, h);
        h = tl_hash(c, 12, h);
        h = tl_hash(&surf, sizeof surf, h);
        if (ok) h = tl_hash(str, strlen(str), h);
        if (textlog_new(h)) {
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# textlog: surf %p font %p hfont %p own %p bmp %p +14: %08x %08x %08x %08x \"%.60s\"\n", surf, (void *)f,
                f ? FIELD(f, 0x18, void *) : NULL, f ? FIELD(f, 0x24, void *) : NULL, bmp, c[0], c[1], c[2], c[3],
                ok ? str : "?");
        LeaveCriticalSection(&g_lock);
        }
    }
    // the character textures with the device's wrap addressing: filtering at a glyph's bottom edge picked up its
    // top row (a line under the "t" of "Vorzeit") - clamped while our bitmap text draws. Device: (ras +0x6C)[0]
    // +0x1C (IDirect3DDevice7; vtable 36 GetTextureStageState, 37 SetTextureStageState; 13/14 ADDRESSU/V, 3 clamp)
    uint8_t **dstate = bmp ? FIELD(ras, 0x6C, uint8_t **) : NULL;
    void *dev = dstate && dstate[0] ? FIELD(dstate[0], 0x1C, void *) : NULL;
    DWORD addr[2] = { 0, 0 };
    if (bmp) FIELD(FIELD(text, 0x44, uint8_t *), 0x24, void *) = bmp;
    // render states (IDirect3DDevice7 vtable 20 SetRenderState, 21 GetRenderState): 27 ALPHABLENDENABLE, 19 SRCBLEND,
    // 20 DESTBLEND, 28 FOGENABLE, 15 ALPHATESTENABLE, 60 TEXTUREFACTOR; stage 0: 1 COLOROP, 2/3 COLORARG1/2, 4 ALPHAOP,
    // 5/6 ALPHAARG1/2
    static const DWORD rs_id[6] = { 27, 19, 20, 28, 15, 60 }, ts_id[6] = { 1, 2, 3, 4, 5, 6 };
    static const DWORD rs_val[6] = { 1, 5, 6, 0, 0, 0 }, ts_val[6] = { 4, 2, 0, 4, 2, 0 };
    DWORD rs_old[6], ts_old[6];
    int st = (int)g_text_state;
    if (dev) {
        void **vt = *(void ***)dev;
        for (int k = 0; k < 2; k++) {
            ((HRESULT (__stdcall *)(void *, DWORD, DWORD, DWORD *))vt[36])(dev, 0, 13 + k, &addr[k]);
            ((HRESULT (__stdcall *)(void *, DWORD, DWORD, DWORD))vt[37])(dev, 0, 13 + k, 3);
        }
        for (int k = 0; k < 6; k++) {
            ((HRESULT (__stdcall *)(void *, DWORD, DWORD *))vt[21])(dev, rs_id[k], &rs_old[k]);
            ((HRESULT (__stdcall *)(void *, DWORD, DWORD, DWORD *))vt[36])(dev, 0, ts_id[k], &ts_old[k]);
        }
        if (g_textlog > 0 && g_log && textlog_new(tl_hash(rs_old, sizeof rs_old, tl_hash(ts_old, sizeof ts_old, 77)))) {
            EnterCriticalSection(&g_lock);
            fprintf(g_log, "# textlog state: blend %lu src %lu dst %lu fog %lu alphatest %lu tfactor %08lx; stage0 colorop %lu args %lu %lu "
                    "alphaop %lu args %lu %lu (textstate %d)\n", rs_old[0], rs_old[1], rs_old[2], rs_old[3], rs_old[4], rs_old[5],
                    ts_old[0], ts_old[1], ts_old[2], ts_old[3], ts_old[4], ts_old[5], st);
            LeaveCriticalSection(&g_lock);
        }
        for (int k = 0; st >= 1 && k < 4; k++) ((HRESULT (__stdcall *)(void *, DWORD, DWORD))vt[20])(dev, rs_id[k], rs_val[k]);
        for (int k = 0; st >= 2 && k < 6; k++) ((HRESULT (__stdcall *)(void *, DWORD, DWORD, DWORD))vt[37])(dev, 0, ts_id[k], ts_val[k]);
    }
    FC_TIMED(FC_TEXT_RECT, ((long (__thiscall *)(void *, void *, void *, void *))g_fc[FC_TEXT_RECT].orig[DISPLAY_DLL(ras)])(ras, text, rect, surf));
    if (dev) {
        void **vt = *(void ***)dev;
        for (int k = 0; k < 2; k++) ((HRESULT (__stdcall *)(void *, DWORD, DWORD, DWORD))vt[37])(dev, 0, 13 + k, addr[k]);
        for (int k = 0; st >= 1 && k < 4; k++) ((HRESULT (__stdcall *)(void *, DWORD, DWORD))vt[20])(dev, rs_id[k], rs_old[k]);
        for (int k = 0; st >= 2 && k < 6; k++) ((HRESULT (__stdcall *)(void *, DWORD, DWORD, DWORD))vt[37])(dev, 0, ts_id[k], ts_old[k]);
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
// Six DIBs (surface sizes / formats); a seventh size (window resized again and again) replaces the one used longest
// ago - they were kept for the session, and from the seventh size on all text went the leaking way. Surfaces up to
// 8192x8192 (the tile map's size only: 64 KB, its loops go as far as the surface; it was 2560x1600, and 4K or
// ultrawide windows got the game's own GDI text and its leak).
static struct { int w, h, bpp; DWORD masks[3]; HDC dc; HBITMAP bmp; void *bits; int pitch; unsigned used; } g_tdc[6];
static struct { void *surf; int slot; DDSURFACEDESC2 d; } g_tdc_active[8];
enum { TDC_TILE = 32, TDC_MAX = 8192, TDC_TW = TDC_MAX / TDC_TILE + 2, TDC_TH = TDC_MAX / TDC_TILE + 2 };
static uint8_t g_tdc_tiles[TDC_TH][TDC_TW];   // loaded in this text phase (one phase at a time)
static unsigned g_tdc_clock;
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
            g_tdc[i].masks[0] == d->ddpfPixelFormat.dwRBitMask && g_tdc[i].masks[1] == d->ddpfPixelFormat.dwGBitMask) {
            g_tdc[i].used = ++g_tdc_clock;
            return i;
        }
        if (!g_tdc[i].dc && free_ < 0) free_ = i;
    }
    for (int i = 0; i < 6 && free_ < 0; i++) {   // all in use: the one used longest ago and not drawn on now
        int busy = 0;
        for (int k = 0; k < 8; k++) busy |= g_tdc_active[k].surf && g_tdc_active[k].slot == i;
        if (!busy && (free_ < 0 || g_tdc[i].used < g_tdc[free_].used)) free_ = i;
    }
    if (free_ < 0) return -1;
    if (g_tdc[free_].dc) {
        if (g_log) log_locked("# text dc: %dx%d %d bit replaces the DIB for %dx%d %d bit\n", w, h, bpp, g_tdc[free_].w,
                              g_tdc[free_].h, g_tdc[free_].bpp);
        DeleteDC(g_tdc[free_].dc);   // the DIB was selected into it: deleted after the DC
        DeleteObject(g_tdc[free_].bmp);
        memset(&g_tdc[free_], 0, sizeof g_tdc[free_]);
    }
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
    g_tdc[free_].used = ++g_tdc_clock;
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
    int tx0 = TDC_TW, ty0 = TDC_TH, tx1 = -1, ty1 = -1, tw = (w + TDC_TILE - 1) / TDC_TILE, th = (h + TDC_TILE - 1) / TDC_TILE;
    uint8_t want = to_surface ? 1 : 2;   // 2 = to load now, 1 = loaded (written back at the end)
    for (int ty = 0; ty < th && ty < TDC_TH; ty++)
        for (int tx = 0; tx < tw && tx < TDC_TW; tx++)
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

static void textlog_gdi(HDC dc, int a, const char *str, int n) {
    if (g_textlog <= 0 || g_fbm_making || !g_log) return;
    COLORREF col = GetTextColor(dc);
    int w = a >= 0 ? g_tdc[g_tdc_active[a].slot].w : 0, hh = a >= 0 ? g_tdc[g_tdc_active[a].slot].h : 0;
    void *surf = a >= 0 ? g_tdc_active[a].surf : NULL;
    uint32_t h = tl_hash(&col, sizeof col, 2166136261u ^ 0x5A);
    h = tl_hash(&w, sizeof w, h);
    h = tl_hash(str, (size_t)n, h);
    if (!textlog_new(h)) return;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# textlog gdi: %s dc %p surface %p %dx%d colour %06lx bkmode %d \"%.*s\"\n", a >= 0 ? "our" : "game", (void *)dc,
            surf, w, hh, (unsigned long)col, GetBkMode(dc), n > 60 ? 60 : n, str);
    LeaveCriticalSection(&g_lock);
}
static BOOL WINAPI hook_ext_text_out(HDC dc, int x, int y, UINT opts, const RECT *rc, LPCSTR str, UINT n, const INT *dx) {
    int a = tdc_active_of(dc);
    if (str && n) textlog_gdi(dc, a, str, (int)n);
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
    if (g_textlog > 0 && str && n > 0) {
        char t[128];
        int m = WideCharToMultiByte(CP_ACP, 0, str, n < 120 ? n : 120, t, sizeof t, NULL, NULL);
        if (m > 0) textlog_gdi(dc, a, t, m);
    }
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
    static int big_logged;
    if ((int)d->dwWidth > TDC_MAX || (int)d->dwHeight > TDC_MAX) {
        if (!big_logged++ && g_log)
            log_locked("# text dc: surface %lux%lu over %dx%d - the game's own text DC for it\n", d->dwWidth, d->dwHeight, TDC_MAX, TDC_MAX);
        return text_dc_fallback(surf, 0);
    }
    int slot = text_dc_slot(d);
    if (slot < 0) return text_dc_fallback(surf, 0);
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
