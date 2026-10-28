// Camera scrolling: Empire Earth's constant speed or Art of Conquest's acceleration, in either build.
// ageofai.ini [camera] scroll = game | ee | aoc (game, the default: the running build's own - EE's in Empire Earth,
// AoC's in AoC).
//
// Scrolling (mouse at the screen edge / keys) is the action EEAScroll (data_ui_vt_scroll, slot 1 Execute scroll_execute),
// queued by a timer (its ctor: `push 0x19` - 25 ms - at data_ui_site_scroll_timer; AoC's `push 0xA`, 10 ms). The fields are
// the same in both: +0x40 elapsed timer ticks, +0x44 left, +0x45 right, +0x46 down, +0x47 up. EE moves the camera at a
// constant speed:
//   step = (speed data_game_scroll_speed + 0.5) * (scroll setting data_game_scroll_setting + 0.5) * ticks * 0.25
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
// 4-13 ms: per frame 0 or 1 steps of the same size whatever the frame time - visible jitter; and a busy game thread
// (heavy simulation steps late in a big game) held the camera until it got round to the next step. Now the render
// thread moves the camera, every frame by speed x the frame's time; the game's steps (the timer every 1 ms, run only
// while a direction is held - camtrace: none at rest) only tell which directions are held. When they stop coming,
// that is either the player letting go or the game thread busy: the render thread goes on only while the input says
// so itself - the arrow key down, or the cursor at that edge of the game's foreground window (windowed and
// borderless; native fullscreen's DirectInput mouse isn't the OS cursor) - up to CAM_HOLD_MS. Same speeds as
// before: EE (speed + 0.5) * (setting + 0.5) * 0.01 tiles per ms; AoC ramps its velocity from 10% to that by 1%
// of the top speed per 10 ms. The game's camera move (camera_move) sets the scene's camera node, which is what
// the render thread draws from - between its frames it has the node to itself.
#define CAM_FRESH_MS 40    // the game's last step this recent: its directions stand
#define CAM_HOLD_MS 500    // a late step's directions kept this long while the input agrees
#define CAM_EDGE_PX 8      // the cursor this near a window edge counts as at it
internal int g_cam_smooth = 1;
internal volatile LONG g_cam_keys;     // the directions of the game's last step: bit k = SCROLL_KEYS + k (left, right, down, up)
internal volatile DWORD g_cam_keys_at; // GetTickCount of that step
internal LARGE_INTEGER g_cam_freq, g_cam_frame;

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

// the camera moved by ms of scrolling in the directions k (bits as g_cam_keys)
internal void scroll_move(float ms, int k) {
    int l = k & 1, r = k >> 1 & 1, dn = k >> 2 & 1, up = k >> 3 & 1;
    float speed = scroll_speed() + 0.5f, maxv = scroll_setting() + 0.5f;
    float dx = 0, dy = 0;
    if (!g_scroll_aoc) {   // EE: constant speed, as 0x4050FC
        float step = speed * maxv * ms * 0.01f;
        dx = r ? step : l ? -step : 0;
        dy = dn ? step : up ? -step : 0;
    } else {               // AoC: velocity ramp
        DWORD t = GetTickCount();
        if (t - g_scroll_last > 100) g_vel_x = g_vel_y = 0;   // scrolling had stopped: start slow again
        g_scroll_last = t;
        float minv = maxv * 0.1f, scale = speed * ms * 0.01f, ramp = ms * 0.001f * maxv;
        if (l || r) { scroll_axis(&g_vel_x, l, r, minv, maxv, ramp); dx = g_vel_x * scale; }
        if (up || dn) { scroll_axis(&g_vel_y, up, dn, minv, maxv, ramp); dy = g_vel_y * scale; }
    }
    if (dx != 0 || dy != 0) camera_pan(dx, dy);
}

internal void __thiscall hook_scroll_execute(ee_scroll *self) {
    InterlockedIncrement(&g_scroll_calls);
    if (g_scroll_native && !g_cam_smooth) { scroll_execute(self); return; }
    int k = 0;
    for (int i = 0; i < 4; i++) if (scroll_get_key(self, i)) k |= 1 << i;
    if (g_cam_smooth) {   // the render thread moves it (camera_frame)
        g_cam_keys_at = GetTickCount();
        InterlockedExchange(&g_cam_keys, k);
        return;
    }
    scroll_move((float)scroll_get_ticks(self) * 10.0f, k);   // the timer's ticks (10 ms AoC)
}

// The arrow keys aren't EEAScroll's: each is a hotkey of the game's form whose event (EEUIEKeyboardScroll) the form's
// input poll runs on the game thread while the key is down, and every run moved the camera by a fixed 0.05 x (speed +
// 0.5) x (setting + 0.5). An idle game thread polls about every 10 ms (camtrace, paused, 2026-10-28: 0.8 runs per
// 8.3 ms frame); a busy one late in a big game every 30 ms or more - the camera stood, then jumped, while the frames
// went on at 120 fps. Smoothed like the edge: the event only tells which arrows are held, the render thread moves
// the camera at the idle poll's speed - 0.05 per 10 ms, half the edge's EE speed - by each frame's time.
internal volatile LONG g_arrow_keys;     // bit = the event's direction: 0 up, 1 down, 2 left, 3 right
internal volatile DWORD g_arrow_at[4];   // GetTickCount of each direction's last event

internal uint32_t __thiscall hook_key_scroll_execute(ee_key_scroll *self, uint32_t a, uint32_t b) {
    int d = key_scroll_get_direction(self);
    if (!g_cam_smooth || d < 0 || d > 3) return eeuie_keyboard_scroll_execute(self, a, b);
    g_arrow_at[d] = GetTickCount();
    InterlockedOr(&g_arrow_keys, 1 << d);
    return 0;
}

internal void arrow_frame(float ms) {
    static const int vk[4] = { VK_UP, VK_DOWN, VK_LEFT, VK_RIGHT };
    int k = (int)g_arrow_keys, held = 0;
    if (!k) return;
    DWORD now = GetTickCount();
    for (int d = 0; d < 4; d++) {
        if (!(k >> d & 1)) continue;
        DWORD age = now - g_arrow_at[d];
        // the form's poll is late or the key was let go: the key's own state decides, up to CAM_HOLD_MS
        if (age <= CAM_FRESH_MS || (age <= CAM_HOLD_MS && GetAsyncKeyState(vk[d]) < 0)) held |= 1 << d;
        else InterlockedAnd(&g_arrow_keys, ~(1 << d));
    }
    if (!held) return;
    float step = (scroll_speed() + 0.5f) * (scroll_setting() + 0.5f) * 0.005f * ms;   // 0.05 per 10 ms
    float dx = (held & 8 ? step : 0) - (held & 4 ? step : 0), dy = (held & 1 ? step : 0) - (held & 2 ? step : 0);
    if (dx != 0 || dy != 0) camera_pan(dx, dy);
}

// the directions of k the player still holds by the input itself: an arrow key, the cursor at a window edge
internal int scroll_input_agrees(int k) {
    int held = (GetAsyncKeyState(VK_LEFT) < 0) | (GetAsyncKeyState(VK_RIGHT) < 0) << 1
             | (GetAsyncKeyState(VK_DOWN) < 0) << 2 | (GetAsyncKeyState(VK_UP) < 0) << 3;
    HWND w = GetForegroundWindow();
    DWORD pid = 0;
    POINT p;
    RECT rc;
    if (w && GetWindowThreadProcessId(w, &pid) && pid == GetCurrentProcessId() && GetCursorPos(&p)
        && ScreenToClient(w, &p) && GetClientRect(w, &rc))
        held |= (p.x <= rc.left + CAM_EDGE_PX) | (p.x >= rc.right - 1 - CAM_EDGE_PX) << 1
              | (p.y >= rc.bottom - 1 - CAM_EDGE_PX) << 2 | (p.y <= rc.top + CAM_EDGE_PX) << 3;
    return k & held;
}

internal void camera_frame(void) {   // the render thread, every presented frame
    if (!g_cam_smooth) return;
    LARGE_INTEGER now;
    if (!g_cam_freq.QuadPart) QueryPerformanceFrequency(&g_cam_freq);
    QueryPerformanceCounter(&now);
    double ms = g_cam_frame.QuadPart ? (double)(now.QuadPart - g_cam_frame.QuadPart) * 1000.0 / (double)g_cam_freq.QuadPart : 0;
    g_cam_frame = now;
    if (game_camera()) arrow_frame(ms > 100.0 ? 100.0f : (float)ms);
    int k = (int)g_cam_keys;
    if (!k || !game_camera()) return;
    DWORD age = GetTickCount() - g_cam_keys_at;
    if (age > CAM_HOLD_MS) k = 0;
    else if (age > CAM_FRESH_MS) k = scroll_input_agrees(k);
    if (!k) return;
    scroll_move(ms > 100.0 ? 100.0f : (float)ms, k);   // a frame that long (a window drag): no leap
}

internal void camera_install(void) {
    int build_aoc = build_is("aoc");
    enum { SCROLL_GAME, SCROLL_EE, SCROLL_AOC };   // [camera] scroll's names (rows.h)
    g_scroll_aoc = g_set.camera_scroll == SCROLL_GAME ? build_aoc : g_set.camera_scroll == SCROLL_AOC;
    g_scroll_native = g_scroll_aoc == build_aoc;
    g_cam_smooth = g_set.camera_smooth;
    if (g_scroll_native && !g_cam_smooth) return;   // the build's own way without smoothing: the game untouched
    int ok = patch_scroll_execute(hook_scroll_execute);
    ok += patch_scroll_timer(build_aoc ? 0x0A : 0x19, g_cam_smooth ? 0x01 : 0x0A);   // push 25 (AoC 10) -> 10 ms ticks like AoC, 1 ms fine
    if (g_cam_smooth) ok += patch_key_scroll_execute(hook_key_scroll_execute);
    if (g_log) fprintf(g_log, "# camera: scroll=%s (%s), fine steps %d, patches %d/%d\n", g_scroll_aoc ? "aoc" : "ee",
                       g_scroll_native ? "the game's own" : "the other build's", g_cam_smooth, ok, g_cam_smooth ? 3 : 2);
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
    ee_camera *cam = game_camera();
    float x = camera_get_x(cam), y = camera_get_y(cam);
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
    uint8_t mem[0x50] = { 0 };
    ee_scroll *obj = (ee_scroll *)mem;
    scroll_action_init(obj);
    scroll_set_ticks(obj, 1);
    scroll_set_key(obj, 0, dir[0] == 'l'); scroll_set_key(obj, 1, dir[0] == 'r'); scroll_set_key(obj, 2, dir[0] == 'd');
    scroll_set_key(obj, 3, dir[0] == 'u');
    ee_camera *cam = game_camera();
    float x0 = camera_get_x(cam), y0 = camera_get_y(cam);
    REPLY("mode %s, start %.2f,%.2f\n", g_scroll_aoc ? "aoc" : "ee", x0, y0);
    for (int i = 0; i < n && i < 200; i++) {
        float x = camera_get_x(cam), y = camera_get_y(cam);
        hook_scroll_execute(obj);
        if (i < 12 || i == n - 1)
            REPLY("  call %2d: moved %.3f,%.3f (velocity %.3f,%.3f)\n", i, camera_get_x(cam) - x,
                  camera_get_y(cam) - y, g_vel_x, g_vel_y);
    }
}
