// DISPLAY DIAGNOSTICS: the frame watch ([display] frame_watch), the diagnostic hooks ([display] diag / autotest
// diag_hooks: terrain, surface copies and creation, render targets, clears, fog), the frame costs and the session
// counts of the minute line. Included by display.c (at this place in it - it uses what display.c defines above).

// FRAME WATCH (aimod.ini [display] frame_watch = frames per second; default 0 = off - a profiler suspending the
// render thread, for diagnosing, not for every player). Low frame rates late in
// long games (25 fps, 64% of the render thread in GDI, 2026-10-12) don't show in the test games, so the game watches
// itself: while a game runs (the game clock moves), every minute a "# frames:" line with the average and the worst
// second and the process's GDI / USER object counts (a leak would grow them); when 10 seconds in a row stay under
// the threshold, the render-thread profile (profile 3000 render) runs on its own thread and goes into the log -
// at most 5 a session, 5 minutes apart.
internal void cmd_profile_render(int dur, char *out);
internal void frame_costs_log(unsigned frames);
internal int is_return_address(uintptr_t a);
internal int g_frame_watch;
internal int g_fw_test;   // autotest frame_watch given: also headless (tests)
internal struct {
    DWORD sec_start, min_ms, last_profile, last_frame;
    unsigned sec_frames, min_frames, slow_secs, min_slow, profiles;
    uint32_t last_gt;
    float worst, trigger_fps;
} g_fw;
// STALLS: a frame 2+ s after the last one - the per-second rate above can't see it (a 10-s freeze and then 900
// frames is a fast second). Logged with whether the language model was answering then (it shares the GPU: a model
// loaded into video memory on its first request froze the game for seconds).
internal int llm_busy(void);   // llm.c: requests queued or running
internal HANDLE g_fw_event;
internal DWORD WINAPI frame_profile_thread(LPVOID unused) {
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
internal void frame_watch(void) {
    if (!g_frame_watch || (g_headless && !g_fw_test)) return;
    DWORD now = GetTickCount();
    DWORD gap = g_fw.last_frame ? now - g_fw.last_frame : 0;
    g_fw.last_frame = now;
    if (gap >= 2000 && game_time() && g_log) {
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# frames: stall of %.1f s at %.1f min (model requests open: %d)\n", gap / 1000.0, game_time() / 60000.0,
                llm_busy());
        fflush(g_log);
        LeaveCriticalSection(&g_lock);
    }
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

// diagnostics: terrain draw results (DX7Rasterizer::Draw(GETerrainMesh&, screen, bool))
typedef int (__thiscall *DrawTerrainFn)(void *ras, void *mesh, unsigned screen, int flag);
internal DrawTerrainFn g_draw_terrain[2];
internal volatile LONG g_terrain_calls, g_terrain_fail, g_terrain_last_err, g_terrain_rows;
typedef DWORD (*GetErrFn)(void);
internal GetErrFn g_get_native_error;
internal int g_diag_skip;   // autotest diag_skip: 1 = no terrain, 2 = no water (diagnostics)
internal DrawTerrainFn g_draw_water[2];
internal int __fastcall hook_draw_water(void *ras, void *edx, void *mesh, unsigned screen, int flag) {
    (void)edx;
    return g_diag_skip == 2 ? 0 : g_draw_water[DISPLAY_DLL(ras)](ras, mesh, screen, flag);
}
internal int __fastcall hook_draw_terrain(void *ras, void *edx, void *mesh, unsigned screen, int flag) {
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
internal void *g_text_lock_orig[2], *g_text_unlock_orig[2];   // DX7Surface::TextLock / TextUnlock (own text DC)
internal struct { const char *name; volatile LONG calls, fail, last_err, last_ret; void *orig[2]; } g_cnt[3] = {
    { "CopyTo", 0, 0, 0, 0, { NULL, NULL } }, { "CopyWithFilter", 0, 0, 0, 0, { NULL, NULL } },
    { "CreateSurface", 0, 0, 0, 0, { NULL, NULL } },
};
internal void cnt_note(int k, int r) {
    InterlockedIncrement(&g_cnt[k].calls);
    if (r) {
        InterlockedIncrement(&g_cnt[k].fail);
        g_cnt[k].last_ret = r;
        if (g_get_native_error) g_cnt[k].last_err = (LONG)g_get_native_error();
    }
}
// alpha left in copy destinations that have an alpha channel (sampled, first 400 such copies)
internal volatile LONG g_alpha_copies, g_alpha_samples, g_alpha_zero, g_alpha_src_zero;
internal char g_alpha_last[200];
internal void sample_alpha(uint8_t *src, uint8_t *dst) {
    IDirectDrawSurface7 *d7 = dst ? FIELD(dst, OF(E_SURF, 0x48), IDirectDrawSurface7 *) : NULL;
    IDirectDrawSurface7 *s7 = src ? FIELD(src, OF(E_SURF, 0x48), IDirectDrawSurface7 *) : NULL;
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
internal volatile LONG g_front_copies_skipped;
internal int __fastcall hook_copy_to(void *s, void *edx, void *dst, void *r1, void *r2, int f) {
    (void)edx;
    // windowed: the UI's "copy dirty rectangles" restore regions from the front surface - after a page flip the
    // back buffer holds an older frame. Windowed the front is the emulated primary that never gets our frames
    // (garbage/scanlines in the editor where the mouse moved) and the back buffer is never flipped, so it still
    // holds the last frame: copies from or to the front are skipped.
    if (!g_fullscreen) {
        uint8_t *ras = *(uint8_t **)GA(G_RASTERIZER);
        uint8_t *st = ras ? FIELD(ras, OF(E_RAS, 0x20), uint8_t **)[*(int *)GA(G_SCREEN)] : NULL;
        uint8_t *front = st ? FIELD(st, OF(E_SCRSTATE, 0x2AC), uint8_t *) : NULL;
        if (front && (s == front || dst == front)) { InterlockedIncrement(&g_front_copies_skipped); return 0; }
    }
    int r =((int (__thiscall *)(void *, void *, void *, void *, int))g_cnt[0].orig[DISPLAY_DLL(s)])(s, dst, r1, r2, f);
    cnt_note(0, r);
    if (g_diag && !r) sample_alpha(s, dst);
    return r;
}
internal int __fastcall hook_copy_filter(void *s, void *edx, void *dst, void *r1, void *r2) {
    (void)edx;
    int r = ((int (__thiscall *)(void *, void *, void *, void *))g_cnt[1].orig[DISPLAY_DLL(s)])(s, dst, r1, r2);
    cnt_note(1, r);
    return r;
}
// histogram of created surfaces by GESurfaceDescription type (+0) and pixel format index (+4) - after the call
// the format is the resolved one; also the DirectDraw pixel format of the last surface per bucket
internal struct { unsigned type, fmt, count, w, h, bpp, amask, flags; } g_surf_kinds[24];
internal uintptr_t g_surf16_stack[8];
internal int __fastcall hook_create_surface(void *s, void *edx, void *desc, void **out) {
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
        IDirectDrawSurface7 *dds = FIELD(*out, OF(E_SURF, 0x48), IDirectDrawSurface7 *);
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
internal SetRTFn g_set_rt[2];
internal volatile LONG g_rt_calls, g_rt_back;
internal char g_rt_last[160];
internal void __fastcall hook_set_rt(void *ras, void *edx, uint8_t *surf, unsigned screen) {
    (void)edx;
    g_set_rt[DISPLAY_DLL(ras)](ras, surf, screen);
    InterlockedIncrement(&g_rt_calls);
    uint8_t *st = FIELD(ras, OF(E_RAS, 0x20), uint8_t **)[screen];
    if (surf == FIELD(st, OF(E_SCRSTATE, 0x2B0), uint8_t *)) { InterlockedIncrement(&g_rt_back); return; }
    IDirectDrawSurface7 *s = surf ? FIELD(surf, OF(E_SURF, 0x48), IDirectDrawSurface7 *) : NULL;
    DDSURFACEDESC2 d = { .dwSize = sizeof d };
    if (s && s->lpVtbl->GetSurfaceDesc(s, &d) == DD_OK)
        snprintf(g_rt_last, sizeof g_rt_last, "%lux%lu caps=%08lx bpp=%lu", d.dwWidth, d.dwHeight, d.ddsCaps.dwCaps,
                 d.ddpfPixelFormat.dwRGBBitCount);
}

// diagnostics: Clear(color, rect, screen) / ClearZBuffer(depth, rect, screen) results and the last rect
typedef int (__thiscall *ClearFn)(void *ras, void *a, float *rect, unsigned screen);
typedef int (__thiscall *ClearZFn)(void *ras, float depth, float *rect, unsigned screen);
internal ClearFn g_clear[2];
internal ClearZFn g_clearz[2];
internal volatile LONG g_clear_calls, g_clear_fail, g_clearz_calls, g_clearz_fail;
internal float g_clear_rect[4], g_clearz_rect[4], g_clearz_depth;
internal int __fastcall hook_clear(void *ras, void *edx, void *color, float *rect, unsigned screen) {
    (void)edx;
    int r = g_clear[DISPLAY_DLL(ras)](ras, color, rect, screen);
    InterlockedIncrement(&g_clear_calls);
    if (r) InterlockedIncrement(&g_clear_fail);
    memcpy(g_clear_rect, rect, sizeof g_clear_rect);
    return r;
}
internal int __fastcall hook_clearz(void *ras, void *edx, float depth, float *rect, unsigned screen) {
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
internal SetFogFn g_set_fog[2], g_set_fog_type[2];
internal SetFogRangeFn g_set_fog_range[2];
internal volatile LONG g_fog_on = -1, g_fog_type = -1, g_fog_calls;
internal float g_fog_start, g_fog_end;
internal void __fastcall hook_set_fog(void *ras, void *edx, unsigned on, unsigned screen) {
    (void)edx;
    g_fog_on = (LONG)on;
    InterlockedIncrement(&g_fog_calls);
    g_set_fog[DISPLAY_DLL(ras)](ras, g_diag_skip == 3 ? 0 : on, screen);
}
internal void __fastcall hook_set_fog_type(void *ras, void *edx, unsigned t, unsigned screen) {
    (void)edx;
    g_fog_type = (LONG)t;
    g_set_fog_type[DISPLAY_DLL(ras)](ras, t, screen);
}
internal void __fastcall hook_set_fog_range(void *ras, void *edx, float a, float b, unsigned screen) {
    (void)edx;
    g_fog_start = a;
    g_fog_end = b;
    g_set_fog_range[DISPLAY_DLL(ras)](ras, a, b, screen);
}

// FRAME COSTS (with the frame watch): the profile of a slow game (2026-10-12, 15-25 fps late in the game) had 73%
// of the render thread in GDI - a quarter each under DX7Rasterizer::Draw(GEText) (GetTextExtentPoint32, text on
// surfaces), DX7Surface::Lock and the exe's WONAPI code. These display DLL functions are timed (calls and time per
// frame, and their 3 most frequent callers) and logged with the minute line of the frame watch.
enum { FC_TEXT, FC_TEXT_RECT, FC_LOCK, FC_TEXT_LOCK, FC_TEXT_UNLOCK, FC_N };
internal struct {
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
internal void fc_note(int k, uintptr_t ret, LONGLONG t) {
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
internal long __fastcall hook_fc_text(void *ras, void *edx, void *text, void *surf) {
    (void)edx; long r;
    FC_TIMED(FC_TEXT, ((long (__thiscall *)(void *, void *, void *))g_fc[FC_TEXT].orig[DISPLAY_DLL(ras)])(ras, text, surf));
    return r;
}

// What a session accumulates: a slow game (13 fps at minute 103, the process at 620-690 MB) ran at 148 fps after
// a restart and loading its own autosave (350 MB). Counted for the minute line, since the session started: surfaces
// made by DX7Screen::CreateSurface and DX7Surface objects deleted (vtable slot 0, the deleting destructor), texture
// handles created / destroyed (DX7Rasterizer, virtual), with the process's private memory and handle count.
internal volatile LONG g_surf_deleted, g_tex_created, g_tex_destroyed;
internal void *g_surf_dtor[2], *g_tex_create[2], *g_tex_destroy[2];
internal void *__fastcall hook_surf_dtor(void *surf, void *edx, unsigned flags) {
    (void)edx;
    InterlockedIncrement(&g_surf_deleted);
    return ((void *(__thiscall *)(void *, unsigned))g_surf_dtor[DISPLAY_DLL(surf)])(surf, flags);
}
internal long __fastcall hook_tex_create(void *ras, void *edx, void *tex) {
    (void)edx;
    InterlockedIncrement(&g_tex_created);
    return ((long (__thiscall *)(void *, void *))g_tex_create[DISPLAY_DLL(ras)])(ras, tex);
}
internal void __fastcall hook_tex_destroy(void *ras, void *edx, void *tex) {
    (void)edx;
    InterlockedIncrement(&g_tex_destroyed);
    ((void (__thiscall *)(void *, void *))g_tex_destroy[DISPLAY_DLL(ras)])(ras, tex);
}
internal void session_counts_log(void) {
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

internal void frame_costs_log(unsigned frames) {
    session_counts_log();
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
