// Camera scrolling: Empire Earth's constant speed or Art of Conquest's acceleration, in either build.
// aimod.ini [camera] scroll = game | ee | aoc (game, the default: the running build's own - EE's in Empire Earth,
// AoC's in AoC).
//
// Scrolling (mouse at the screen edge / keys) is the action EEAScroll (VT_SCROLL, slot 1 Execute FN_SCROLL_EXECUTE),
// queued by a timer (its ctor: `push 0x19` - 25 ms - at SITE_SCROLL_TIMER; AoC's `push 0xA`, 10 ms). The fields are
// the same in both: +0x40 elapsed timer ticks, +0x44 left, +0x45 right, +0x46 down, +0x47 up. EE moves the camera at a
// constant speed:
//   step = (speed G_SCROLL_SPEED + 0.5) * (scroll setting G_SCROLL_SETTING + 0.5) * ticks * 0.25
// AoC (EE-AOC.exe 0x40478E) keeps a velocity per axis (globals 0x8E5BE4 x, 0x8E5BE0 y) that starts at 10% and ramps
// up to full speed (+1% of the maximum per elapsed tick; with its +0x48 byte clear - the ctor sets it - at once);
// reversing an axis starts slow again and the velocity is reset when scrolling stops. The top speed is the same as
// EE's: per second 100 x 0.1 vs 40 x 0.25.


internal int g_scroll_aoc, g_scroll_native;   // the AoC ramp; the build's own way (its Execute when not smooth)
internal float g_vel_x, g_vel_y;
internal DWORD g_scroll_last;
internal volatile LONG g_scroll_calls;   // scroll steps run (for camtrace)

// Fine scrolling ([camera] smooth, default 1, both builds): the scroll steps come from a timer (10 ms AoC, 25 ms EE) on the
// game's thread, and each moved the camera by a fixed distance, while the frames are drawn on another thread every
// 4-13 ms: per frame 0 or 1 steps of the same size whatever the frame time (camtrace: about every fifth frame
// without a step) - visible jitter. Now the timer runs every 1 ms (the game thread then makes one step per frame)
// and each step moves by speed x the time of the frames drawn since the last step: the render thread adds each
// frame's duration to a budget, a step takes all of it - the camera moves in proportion to the frame times and
// over all exactly as far as the time passed (the step's own time if no frames are counted). Same speeds as
// before: EE (speed + 0.5) * (setting + 0.5) * 0.01 tiles per ms; AoC ramps its velocity from 10% to that by 1%
// of the top speed per 10 ms.
internal int g_cam_smooth = 1;
internal LARGE_INTEGER g_cam_last, g_cam_freq, g_cam_frame;
internal volatile LONG64 g_cam_budget;   // QPC ticks of frames not yet used by a scroll step
internal void camera_frame(void) {       // the render thread, every presented frame
    LARGE_INTEGER now;
    QueryPerformanceCounter(&now);
    if (g_cam_frame.QuadPart) InterlockedExchangeAdd64(&g_cam_budget, now.QuadPart - g_cam_frame.QuadPart);
    g_cam_frame = now;
}

internal void scroll_axis(float *vel, int neg, int pos, float minv, float maxv, float step) {
    if (neg) {
        if (-minv < *vel) *vel = -minv;
        *vel -= step;
        if (*vel < -maxv) *vel = -maxv;
    } else if (pos) {
        if (*vel < minv) *vel = minv;
        *vel += step;
        if (*vel > maxv) *vel = maxv;
    }
}

internal void __fastcall hook_scroll_execute(uint8_t *self, void *edx) {
    (void)edx;
    InterlockedIncrement(&g_scroll_calls);
    if (g_scroll_native && !g_cam_smooth) { GF(FN_SCROLL_EXECUTE)(self); return; }
    // elapsed time: real time with fine scrolling, else the timer's ticks (10 ms AoC)
    float ms = (float)FIELD(self, OF(SCROLL, 0x40), unsigned) * 10.0f;
    if (g_cam_smooth) {
        LARGE_INTEGER now;
        if (!g_cam_freq.QuadPart) QueryPerformanceFrequency(&g_cam_freq);
        QueryPerformanceCounter(&now);
        double since = g_cam_last.QuadPart ? (double)(now.QuadPart - g_cam_last.QuadPart) * 1000.0 / (double)g_cam_freq.QuadPart : 1e9;
        g_cam_last = now;
        LONG64 budget = InterlockedExchange64(&g_cam_budget, 0);
        double d = g_cam_frame.QuadPart ? (double)budget * 1000.0 / (double)g_cam_freq.QuadPart : since;
        if (since > 30.0) d = 1.0;   // the first step after a pause: one timer period (the budget is old frames)
        ms = d > 50.0 ? 50.0f : (float)d;
    }
    float speed = *(float *)GA(G_SCROLL_SPEED) + 0.5f, maxv = *(float *)GA(G_SCROLL_SETTING) + 0.5f;
    float dx = 0, dy = 0;
    if (!g_scroll_aoc) {   // EE: constant speed, as 0x4050FC
        float step = speed * maxv * ms * 0.01f;
        dx = self[OF(SCROLL, 0x45)] ? step : self[OF(SCROLL, 0x44)] ? -step : 0;
        dy = self[OF(SCROLL, 0x46)] ? step : self[OF(SCROLL, 0x47)] ? -step : 0;
    } else {               // AoC: velocity ramp
        DWORD t = GetTickCount();
        if (t - g_scroll_last > 100) g_vel_x = g_vel_y = 0;   // scrolling had stopped: start slow again
        g_scroll_last = t;
        float minv = maxv * 0.1f, scale = speed * ms * 0.01f, ramp = ms * 0.001f * maxv;
        if (self[OF(SCROLL, 0x44)] || self[OF(SCROLL, 0x45)]) { scroll_axis(&g_vel_x, self[OF(SCROLL, 0x44)], self[OF(SCROLL, 0x45)], minv, maxv, ramp); dx = g_vel_x * scale; }
        if (self[OF(SCROLL, 0x47)] || self[OF(SCROLL, 0x46)]) { scroll_axis(&g_vel_y, self[OF(SCROLL, 0x47)], self[OF(SCROLL, 0x46)], minv, maxv, ramp); dy = g_vel_y * scale; }
    }
    if (dx != 0 || dy != 0) GF(FN_CAMERA_MOVE)((void *)GA(G_CAMERA), dx, dy);
}

internal void camera_install(const char *aimod_ini, const char *autotest_ini) {
    char mode[16], tmp[16];
    int build_aoc = build_is("aoc");
    GetPrivateProfileStringA("camera", "scroll", "game", mode, sizeof mode, aimod_ini);
    if (autotest_ini) {   // tests: [autotest] camera_scroll overrides
        lstrcpynA(tmp, mode, sizeof tmp);
        GetPrivateProfileStringA("autotest", "camera_scroll", tmp, mode, sizeof mode, autotest_ini);
    }
    g_scroll_aoc = !lstrcmpiA(mode, "game") ? build_aoc : !lstrcmpiA(mode, "aoc");
    g_scroll_native = g_scroll_aoc == build_aoc;
    g_cam_smooth = ini_int("camera", "smooth", 1, aimod_ini);
    if (autotest_ini) g_cam_smooth = ini_int("autotest", "camera_smooth", g_cam_smooth, autotest_ini);
    if (g_scroll_native && !g_cam_smooth) return;   // the build's own way without smoothing: the game untouched
    int ok = patch_vtable(GA(VT_SCROLL), GA(SLOT_SCROLL_EXECUTE), GA(FN_SCROLL_EXECUTE), hook_scroll_execute);
    uint8_t *p = (uint8_t *)GA(SITE_SCROLL_TIMER);
    if (p[-1] == 0x6A && p[0] == (build_aoc ? 0x0A : 0x19)) {   // push 25 (AoC 10) -> 10 ms ticks like AoC, 1 ms fine
        DWORD old;
        VirtualProtect(p, 1, PAGE_EXECUTE_READWRITE, &old);
        *p = g_cam_smooth ? 0x01 : 0x0A;
        VirtualProtect(p, 1, old, &old);
        FlushInstructionCache(GetCurrentProcess(), p, 1);
        ok++;
    }
    if (g_log) fprintf(g_log, "# camera: scroll=%s (%s), fine steps %d, patches %d/2\n", g_scroll_aoc ? "aoc" : "ee",
                       g_scroll_native ? "the game's own" : "the other build's", g_cam_smooth, ok);
}

// camtrace N (tests): for the next N presented frames the time since the previous frame, the camera's movement
// and the scroll steps run in between - then logged as "# camtrace:" lines
enum { CT_MAX = 400 };
internal volatile LONG g_ct_left;
internal int g_ct_n;
internal struct { float dt, dx, dy; int steps; } g_ct[CT_MAX];
internal LARGE_INTEGER g_ct_last;
internal float g_ct_x, g_ct_y;
internal void camtrace_frame(void) {   // the render thread, every presented frame
    if (!g_ct_left) return;
    LARGE_INTEGER now, f;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&f);
    uint8_t *cam = (uint8_t *)GA(G_CAMERA);
    float x = FIELD(cam, OF(CAMERA, 0x78), float), y = FIELD(cam, OF(CAMERA, 0x7C), float);
    if (g_ct_n < CT_MAX && g_ct_last.QuadPart) {
        g_ct[g_ct_n].dt = (float)((double)(now.QuadPart - g_ct_last.QuadPart) * 1000.0 / (double)f.QuadPart);
        g_ct[g_ct_n].dx = x - g_ct_x;
        g_ct[g_ct_n].dy = y - g_ct_y;
        g_ct[g_ct_n].steps = (int)InterlockedExchange(&g_scroll_calls, 0);
        g_ct_n++;
    } else InterlockedExchange(&g_scroll_calls, 0);
    g_ct_last = now; g_ct_x = x; g_ct_y = y;
    if (InterlockedDecrement(&g_ct_left) == 0 && g_log) {
        EnterCriticalSection(&g_lock);
        for (int i = 0; i < g_ct_n; i++)
            fprintf(g_log, "# camtrace: %3d dt %5.1f ms, moved %7.4f %7.4f, scroll steps %d\n", i, g_ct[i].dt, g_ct[i].dx, g_ct[i].dy, g_ct[i].steps);
        fflush(g_log);
        LeaveCriticalSection(&g_lock);
    }
}
internal void cmd_camtrace(char *out, const char *args) {
    int n = atoi(args);
    if (n <= 0 || n > CT_MAX) n = 120;
    g_ct_n = 0; g_ct_last.QuadPart = 0;
    g_ct_left = n + 1;
    REPLY("tracing %d frames\n", n);
}

// scrolltest <left|right|up|down> <calls>: run the scroll action on a fake object, report camera movement
internal void cmd_scrolltest(char *out, const char *args) {
    char dir[16] = "right";
    int n = 10;
    sscanf(args, "%15s %d", dir, &n);
    uint8_t obj[0x50] = { 0 };
    *(uint32_t *)obj = GA(VT_SCROLL);
    *(unsigned *)(obj + OF(SCROLL, 0x40)) = 1;
    obj[OF(SCROLL, 0x44)] = dir[0] == 'l'; obj[OF(SCROLL, 0x45)] = dir[0] == 'r'; obj[OF(SCROLL, 0x46)] = dir[0] == 'd';
    obj[OF(SCROLL, 0x47)] = dir[0] == 'u';
    uint8_t *cam = (uint8_t *)GA(G_CAMERA);
    float x0 = FIELD(cam, OF(CAMERA, 0x78), float), y0 = FIELD(cam, OF(CAMERA, 0x7C), float);
    REPLY("mode %s, start %.2f,%.2f\n", g_scroll_aoc ? "aoc" : "ee", x0, y0);
    for (int i = 0; i < n && i < 200; i++) {
        float x = FIELD(cam, OF(CAMERA, 0x78), float), y = FIELD(cam, OF(CAMERA, 0x7C), float);
        hook_scroll_execute(obj, NULL);
        if (i < 12 || i == n - 1)
            REPLY("  call %2d: moved %.3f,%.3f (velocity %.3f,%.3f)\n", i, FIELD(cam, OF(CAMERA, 0x78), float) - x,
                  FIELD(cam, OF(CAMERA, 0x7C), float) - y, g_vel_x, g_vel_y);
    }
}
