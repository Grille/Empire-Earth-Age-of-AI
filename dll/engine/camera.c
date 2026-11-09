// Camera scrolling: Empire Earth's constant speed or Art of Conquest's acceleration, in either build.
// ageofai.ini [camera] scroll = game | ee | aoc (game, the default: the running build's own - EE's in Empire Earth,
// AoC's in AoC).
//
// Scrolling (mouse at the screen edge / keys) is the action EEAScroll (data_ui_vt_scroll, slot 1 Execute scroll_execute),
// queued by a timer (its ctor: `push 0x19` - 25 ms - at data_ui_site_scroll_timer; AoC's `push 0xA`, 10 ms). The fields are
// the same in both: +0x40 elapsed timer ticks, +0x44 left, +0x45 right, +0x46 top, +0x47 bottom (the edge check 0x4051A0
// sets +0x46 for a cursor y at or above its upper bound, +0x47 at or below its lower - the game's mouse y counts down
// the screen, window_mouse_to; read the other way round until 2026-11-07). The top moves the camera +y, as the up
// arrow does (EEUIEKeyboardScroll). EE moves the camera at a constant speed:
//   step = (speed data_game_scroll_speed + 0.5) * (scroll setting data_game_scroll_setting + 0.5) * ticks * 0.25
// AoC (EE-AOC.exe 0x40478E) keeps a velocity per axis (globals 0x8E5BE4 x, 0x8E5BE0 y) that starts at 10% and ramps
// up to full speed (+1% of the maximum per elapsed tick; with its +0x48 byte clear - the ctor sets it - at once);
// reversing an axis starts slow again and the velocity is reset when scrolling stops. The top speed is the same as
// EE's: per second 100 x 0.1 vs 40 x 0.25.


internal int g_scroll_aoc, g_scroll_native;   // the AoC ramp; the build's own way (its Execute when not smooth)
internal float g_vel_x, g_vel_y;
internal DWORD g_scroll_last;
internal volatile LONG g_scroll_calls;   // scroll steps run (for camtrace)

// Fine scrolling ([camera] smooth_scroll, default 1, both builds): the scroll steps come from a timer (10 ms AoC, 25 ms EE) on the
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
internal volatile LONG g_cam_keys;     // the directions of the game's last step: bit k = SCROLL_KEYS + k (left, right, top, bottom)
internal volatile DWORD g_cam_keys_at; // GetTickCount of that step
internal LARGE_INTEGER g_cam_freq, g_cam_frame;
internal int g_rd_on, g_headless;   // display.c: the own renderer draws; a test run without a window in front

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
    int l = k & 1, r = k >> 1 & 1, top = k >> 2 & 1, bottom = k >> 3 & 1;
    float speed = scroll_speed() + 0.5f, maxv = scroll_setting() + 0.5f;
    float dx = 0, dy = 0;
    if (!g_scroll_aoc) {   // EE: constant speed, as 0x4050FC
        float step = speed * maxv * ms * 0.01f;
        dx = r ? step : l ? -step : 0;
        dy = top ? step : bottom ? -step : 0;
    } else {               // AoC: velocity ramp
        DWORD t = GetTickCount();
        if (t - g_scroll_last > 100) g_vel_x = g_vel_y = 0;   // scrolling had stopped: start slow again
        g_scroll_last = t;
        float minv = maxv * 0.1f, scale = speed * ms * 0.01f, ramp = ms * 0.001f * maxv;
        if (l || r) { scroll_axis(&g_vel_x, l, r, minv, maxv, ramp); dx = g_vel_x * scale; }
        if (top || bottom) { scroll_axis(&g_vel_y, bottom, top, minv, maxv, ramp); dy = g_vel_y * scale; }
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
// While an edit box has the keyboard the arrow keys are its own (they move its cursor), but its form runs its hotkeys
// all the same: the camera moved with the cursor (the editor's unit search, 2026-11-04). The form keeps the control
// that has the keyboard (UIForm +0x108; the game's own F-keys wait for it to be none, 0x65C99D): while the current
// form has one the arrows' events do nothing. (Timed by the box's keyboard updates instead, the camera still went in
// steps: while an arrow is held the form runs the hotkey and gives the box no update.)
internal volatile LONG g_arrow_keys;     // bit = the event's direction: 0 up, 1 down, 2 left, 3 right
internal volatile DWORD g_arrow_at[4];   // GetTickCount of each direction's last event
internal ui_control_update_keyboard_fn g_edit_keyboard;

internal int keyboard_captured(void) {
    void *uim = ui_manager();
    ee_ui_form *f = uim ? ui_current_form(uim) : NULL;
    return f && ui_form_get_keyboard_captured(f);
}

// An edit box kept the keyboard after a click beside it that hit no control (the 3D view, the panels' background):
// only a control clicked takes it. Every edit box's UpdateKeyboard - the form calls it for the box that has the
// keyboard - goes through one import of the exe (ui.iat_edit_keyboard): there a left press with the mouse outside the
// box lets the keyboard go. (By the form's mouse and the box's rectangle: the form's control under the mouse is
// found again only after the click, while a control has the keyboard. The press by the button's state at the box's
// last update: the mouse's own frame copy is taken again before the box's update sees it.)
internal int g_edit_lbutton;   // the left button down at an edit box's last keyboard update
internal int __thiscall hook_edit_keyboard(ee_ui_control *box) {
    int r = g_edit_keyboard(box);
    ee_ui_form *f = ui_control_get_form(box);
    ee_input_mouse *ms = input_mouse();
    int down = ms && (input_mouse_buttons_at(ms)[0] & 1), pressed = down && !g_edit_lbutton;
    g_edit_lbutton = down;
    if (pressed && f && ui_form_get_keyboard_captured(f) == box && !ui_control_under_mouse(box, f))
        ui_control_release_captured_keyboard(box);
    return r;
}

internal uint32_t __thiscall hook_key_scroll_execute(ee_key_scroll *self, uint32_t a, uint32_t b) {
    if (keyboard_captured()) {
        InterlockedExchange(&g_arrow_keys, 0);
        return 0;
    }
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

// the game's own window in front - not another window of the program (the editor's terrain colour dialog: the player
// 2026-11-08, "sidescrolling uses the new dialog window" - its edges scrolled the map while the game waited in it)
internal HWND game_in_front(void) {
    ee_window *gw = game_window();
    HWND w = GetForegroundWindow();
    return w && gw && w == window_get_hwnd(gw) ? w : NULL;
}

// the directions (bits as g_cam_keys: left, right, top, bottom) the input itself holds: the cursor at a window edge,
// the arrow keys (up as the top: both move the camera +y)
internal int scroll_edges(void) {
    HWND w = game_in_front();
    POINT p;
    RECT rc;
    if (!w || !GetCursorPos(&p) || !ScreenToClient(w, &p) || !GetClientRect(w, &rc))
        return 0;
    return (p.x <= rc.left + CAM_EDGE_PX) | (p.x >= rc.right - 1 - CAM_EDGE_PX) << 1
         | (p.y <= rc.top + CAM_EDGE_PX) << 2 | (p.y >= rc.bottom - 1 - CAM_EDGE_PX) << 3;
}
internal int scroll_arrows(void) {
    return (GetAsyncKeyState(VK_LEFT) < 0) | (GetAsyncKeyState(VK_RIGHT) < 0) << 1
         | (GetAsyncKeyState(VK_UP) < 0) << 2 | (GetAsyncKeyState(VK_DOWN) < 0) << 3;
}
// the directions of k the player still holds by the input itself
internal int scroll_input_agrees(int k) { return k & (scroll_edges() | scroll_arrows()); }

// The camera's shape ([camera] camera_distance, camera_distance_max, camera_view_distance, camera_fov): what dreXmod's
// camera set (docs/engine-notes.md), the game's own values - the eye's distance at zoom 1 and the far depth, two floats in
// .rdata, and the field of view, a global the camera's step gives the viewport each time. Distances times 0.99 as
// dreXmod's: its 25 and 50 are the picture the player knows.
// Further out: the game's zoom ends at 1, and past 1 its pitch (0x51DA2D: pow(1 - zoom, ..)) goes NaN and the engine's
// UMCos never returns. So the game's zoom stays at 1, the angle with it, and the eye's distance at 1 is what grows:
// g_cam_v, the zoom past 1, is that distance / camera_distance; zoom_to(1) makes the camera's move to it as for any zoom.
// The far depth grows by 0.9 of the distance gained: the ground at the top of the screen lies about 1.6 times the distance
// away, and GetTileRowsInVolume's region is the hull of the frustum's far corners, so a far depth no longer than that
// keeps it under the mesh's 101 x 101 tiles, as far as 42 tiles back: the most without the own renderer, whose ground is the
// whole map (render_ground.c) and which keeps that region within the arrays (render_frame.c rf_region_fit).
#define CAM_AS_DREXMOD 0.99f
#define CAM_FAR_PER_TILE 0.9f
#define CAM_FAR_PER_TILE_OWN 1.6f   // the own renderer: to the top of the screen, the ground is the whole map's
#define CAM_DIST_GAME 20.5f
#define CAM_VIEW_GAME 35.0f
#define CAM_TILES_GAME 42.0f     // camera_distance_max without the own renderer
internal float g_cam_dist, g_cam_view, g_cam_fov_set, g_cam_v_max = 1.0f;
internal float g_cam_v = 1.0f;   // the zoom past the game's 1 (1: none)
internal int g_cam_shape;
internal ee_map *g_cam_v_map;
internal int g_rd_on;   // display.c: the own renderer draws
// The eye gets to a new distance by the camera's ride, over a second or more, and the far depth takes effect at once: going
// in it would cut the ground off behind the eye still far out (the sky's colour over the picture while the wheel turned fast;
// out it only sees further than yet needed), so going in it waits for the ride's end (camera_shape_frame). g_cam_far_v: the zoom it is made for.
internal float g_cam_far_v = 1.0f;
internal void camera_far_write(void) {
    *camera_far_at() = g_cam_view + g_cam_dist * (g_cam_far_v - 1.0f) * (g_rd_on ? CAM_FAR_PER_TILE_OWN : CAM_FAR_PER_TILE);
}
internal void camera_shape_write(float v) {
    if (!g_cam_shape) return;
    *camera_eye_far_at() = -g_cam_dist * v;
    if (v > g_cam_far_v) g_cam_far_v = v;
    camera_far_write();
}
internal float camera_zoom_max(void) {
    if (!g_cam_shape) return 1.0f;
    return g_rd_on ? g_cam_v_max : fmaxf(1.0f, fminf(g_cam_v_max, CAM_TILES_GAME / g_cam_dist));
}
// the zoom: the game's below 1, ours from there
internal float camera_zoom_now(void) {
    float z = scroll_speed();
    return z < 1.0f ? z : g_cam_v;
}
// The game's ride goes 50 tiles a second, a ride of a few tiles in its shortest time, 0.12 s (0x51E79C): a notch far out is
// a hundred tiles, seconds of it (the player 2026-11-09: "the camera still zooms with the old speed, only when moving it snaps
// to the real zoom" - a move ends the ride). Every zoom a ride of that shortest time.
#define CAM_ZOOM_RATE (2.5f / 0.3f)
internal void camera_zoom_ride(ee_camera *cam, float zoom) {
    camera_zoom_to(cam, zoom);
    if (camera_get_blend_rate(cam) < CAM_ZOOM_RATE) camera_set_blend_rate(cam, CAM_ZOOM_RATE);
}
internal void camera_zoom_set(float z) {
    ee_camera *cam = game_camera();
    float top = camera_zoom_max();
    if (z < 0) z = 0;
    if (z > top) z = top;
    g_cam_v = z > 1.0f ? z : 1.0f;
    camera_shape_write(g_cam_v);
    if (cam) camera_zoom_ride(cam, z > 1.0f ? 1.0f : z);
}
// The wheel's zoom after n notches in (out: below 0): the game's 0.05 a notch up to its 1, and from there a step that grows
// with the distance, the same 0.05 at 1 and towards a tenth of the distance far out (the player 2026-11-09: "zooming in and
// out should be stronger the further the camera is"; 0.05 all the way was 780 notches to 1000 tiles back). A factor on
// z - 0.5, so a notch in undoes a notch out.
#define CAM_NOTCH 0.05f
#define CAM_NOTCH_FAR 1.1f
internal float camera_zoom_notches(int n) {
    float z = camera_zoom_now();
    for (int i = n < 0 ? -n : n; i > 0; i--) {
        if (n < 0) z = z >= 1.0f ? 0.5f + (z - 0.5f) * CAM_NOTCH_FAR : z + CAM_NOTCH;
        else z = z > 1.0f ? 0.5f + (z - 0.5f) / CAM_NOTCH_FAR : z - CAM_NOTCH;
    }
    return z;
}
// The wheel's zoom keeps the ground under the mouse where it is ([camera] camera_zoom_at_mouse; the player 2026-11-09: "the
// mouse alsways remains on the same world position"), past the game's zoom 1 only ("horizon view and anchor dont mix": below
// it the pitch turns toward the horizon). A notch takes the point of the terrain under the cursor in the picture drawn now, and
// keeps it through further notches while the cursor stays; every frame of the ride the same ray again, and the difference
// taken off the look-at, at the ride's start and its end together, so the ride goes on (the player's way: world position,
// zoom, world position again, remove the difference; one look-at worked out per notch for the ride's end strayed by 70 tiles
// when notches overlapped). The camera's own move, which ends a ride, runs once at the end: the look-at's height there and
// the map's borders. No point under the cursor (above the horizon): at the middle of the screen.
#define CAM_RAY_STEP 0.5f     // tiles along the ray
#define CAM_RAY_MOST 20000
internal int rf_screen_ray(const float *m, float x, float y, float *eye, float *d);
internal int camera_ground_point(const float *eye, const float *d, float low, float *at) {
    float len = sqrtf(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    if (!(d[2] < 0) || !(len > 0)) return 0;
    float p[3] = { eye[0], eye[1], eye[2] }, k = CAM_RAY_STEP / len;
    for (int i = 0; i < CAM_RAY_MOST; i++) {
        float h;
        ivec2 t = { (int)floorf(p[0]), (int)floorf(p[1]) };
        if (tile_height(t, &h) ? p[2] <= h : p[2] <= low) { memcpy(at, p, sizeof p); return 1; }
        for (int a = 0; a < 3; a++) p[a] += d[a] * k;
    }
    return 0;
}
internal float g_cam_now[16];   // render_frame.c: the camera's transformation this frame, its ride's
internal int g_cam_now_ok;
internal struct { int on; float sx, sy, mx, my, at[3]; } g_cam_anchor;
internal void camera_zoom_at_mouse(float z) {
    ee_camera *cam = game_camera();
    ee_mouse *m = game_mouse();
    ee_window *w = game_window();
    int ok = g_set.camera_zoom_at_mouse && z >= 1.0f && camera_zoom_now() >= 1.0f && g_cam_now_ok && cam && m && w && game_map() && !mouse_get_frozen(m);
    if (!ok && g_cam_anchor.on) {   // into the game's range: the ride ended where it is now, as at its end below
        g_cam_anchor.on = 0;
        if (cam) camera_pan(0.001f, 0);
    }
    if (ok && (!g_cam_anchor.on || mouse_get_x(m) != g_cam_anchor.mx || mouse_get_y(m) != g_cam_anchor.my)) {
        float eye[3], d[3];
        g_cam_anchor.mx = mouse_get_x(m);
        g_cam_anchor.my = mouse_get_y(m);
        g_cam_anchor.sx = g_cam_anchor.mx / (float)window_get_width(w);
        g_cam_anchor.sy = g_cam_anchor.my / (float)window_get_height(w);
        g_cam_anchor.on = rf_screen_ray(g_cam_now, g_cam_anchor.sx, g_cam_anchor.sy, eye, d)
                       && camera_ground_point(eye, d, camera_look_at(cam)[2], g_cam_anchor.at);
    }
    camera_zoom_set(z);
}
// render_frame.c rf_camera_frame: the camera's transformation now (t: the ride's place in it), put right
internal void camera_anchor_frame(ee_camera *cam, float *now, float t) {
    float eye[3], d[3];
    ee_map *map = game_map();
    if (!g_cam_anchor.on) return;
    if (!map) { g_cam_anchor.on = 0; return; }
    if (rf_screen_ray(now, g_cam_anchor.sx, g_cam_anchor.sy, eye, d) && d[2] < 0) {
        float k = (g_cam_anchor.at[2] - eye[2]) / d[2], n = (float)map_get_size(map), x = camera_get_x(cam), y = camera_get_y(cam);
        float dx = fmaxf(0, fminf(n, x + g_cam_anchor.at[0] - (eye[0] + d[0] * k))) - x;
        float dy = fmaxf(0, fminf(n, y + g_cam_anchor.at[1] - (eye[1] + d[1] * k))) - y;
        camera_set_x(cam, x + dx);
        camera_set_y(cam, y + dy);
        camera_set_last_x(cam, camera_get_last_x(cam) + dx);
        camera_set_last_y(cam, camera_get_last_y(cam) + dy);
        now[3] += dx;
        now[7] += dy;
    }
    if (t >= 1.0f && !(camera_get_blend(cam) < 1)) {
        g_cam_anchor.on = 0;
        camera_pan(0.001f, 0);
    }
}
// every frame, the render thread: a float the game's step reads; a zoom past 1 ends with a new map or a zoom the game made
internal void camera_shape_frame(void) {
    if (!g_cam_shape) return;
    if (g_cam_fov_set > 0) *camera_fov_at() = g_cam_fov_set;
    ee_map *map = game_map();
    if (g_cam_v != 1.0f && game_camera() && (map != g_cam_v_map || scroll_speed() < 1.0f)) {
        g_cam_v = 1.0f;
        camera_shape_write(1.0f);
    }
    if (g_cam_far_v > g_cam_v && (map != g_cam_v_map || !game_camera() || !(camera_get_blend(game_camera()) < 1))) {
        g_cam_far_v = g_cam_v;
        camera_far_write();
    }
    g_cam_v_map = map;
}
internal void camera_shape_install(void) {
    g_cam_shape = camera_shape_open();
    if (!g_cam_shape) return;
    g_cam_dist = g_set.camera_distance > 0 ? (float)g_set.camera_distance * CAM_AS_DREXMOD : CAM_DIST_GAME;
    g_cam_view = g_set.camera_view_distance > 0 ? (float)g_set.camera_view_distance * CAM_AS_DREXMOD : CAM_VIEW_GAME;
    g_cam_fov_set = (float)g_set.camera_fov * 3.14159265f / 180.0f;
    if ((float)g_set.camera_distance_max > g_cam_dist) g_cam_v_max = (float)g_set.camera_distance_max / g_cam_dist;
    camera_shape_write(1.0f);
    int tilt = g_set.camera_fov > 0 ? patch_camera_tilt() : 0;
    if (g_log) fprintf(g_log, "# camera: %.2f tiles back at the game's furthest, to %.2f; far %.2f; field of view %.3f (the fixed tilt %s)\n",
                       (double)g_cam_dist, (double)(g_cam_dist * g_cam_v_max), (double)g_cam_view, (double)g_cam_fov_set, tilt ? "made the pitch" : "as it was");
}

// The wheel's zoom with the own renderer: the game's (EEUIEWheelZoom, the game's form's wheel event) runs in the form's
// input on the game thread, which waits for the world's lock through a step - the zoom came with the step's end (the
// player 2026-11-07: "mouse scroll/zoom is still bound to the lookstep"). So a low-level mouse hook on a thread of its
// own counts the wheel while the game's window is in front (the game thread may be stuck, its messages with it) and
// the render thread zooms by it as the event does: scroll_speed - 0.05 a notch, 0 .. 1, zoom_to - its ride then the
// render thread's too (render_frame.c rf_camera_frame); the camera's transformation the ride starts from (0x51DDD7
// copies +0x84, made only in the camera's step) set to the one drawn now first, else a second notch within a step
// started back at the step's start; and the ride begun anew there (its blend is 0 already through a step - a second
// notch was not seen as a new ride and jumped to its end). The game's event does nothing meanwhile (hook_wheel_zoom). The game's form only
// (EEUIFMainGame): the scenario editor's lists take the wheel themselves - its zoom stays the game's. Not headless: a
// low-level hook is called for every mouse event of the machine - the player's mouse through 30 test slots.
internal volatile LONG g_wheel;   // the wheel's delta not zoomed yet (WHEEL_DELTA a notch: the axis' granularity)
internal HHOOK g_wheel_hook;   // in place: the wheel and the middle button are the render thread's (grab_frame)
internal LONG g_wheel_zooms;
internal float g_cam_blend_seen, g_cam_t;
internal LRESULT CALLBACK wheel_hook(int code, WPARAM w, LPARAM l) {
    if (code == HC_ACTION && w == WM_MOUSEWHEEL) {
        if (game_in_front())
            InterlockedExchangeAdd(&g_wheel, (short)HIWORD(((const MSLLHOOKSTRUCT *)l)->mouseData));
    }
    return CallNextHookEx(g_wheel_hook, code, w, l);
}
internal DWORD WINAPI wheel_thread(LPVOID unused) {
    (void)unused;
    g_wheel_hook = SetWindowsHookExA(WH_MOUSE_LL, wheel_hook, GetModuleHandleA(NULL), 0);
    if (g_log) fprintf(g_log, "# camera: the wheel's zoom on the render thread%s\n", g_wheel_hook ? "" : " - no mouse hook (the game's own)");
    MSG m;
    while (g_wheel_hook && GetMessageA(&m, NULL, 0, 0) > 0) {}
    return 0;
}
// the wheel is the render thread's now: the game's form, its zoom's own conditions (EEUIEWheelZoom)
internal int wheel_ours(const ee_ui_form *f) {
    return g_rd_on && ui_form_is_game(f) && wheel_zoom_free();
}
internal void wheel_frame(const ee_ui_form *f) {
    LONG n = g_wheel / WHEEL_DELTA;   // whole notches, the rest kept (the game's axis drops it)
    if (!n) return;
    InterlockedExchangeAdd(&g_wheel, -n * WHEEL_DELTA);
    ee_camera *cam = game_camera();
    if (!cam || !wheel_ours(f)) return;
    float z = camera_zoom_notches((int)n);
    if (g_cam_now_ok && g_cam_t > camera_get_blend(cam)) camera_set_blend(cam, g_cam_t);
    camera_zoom_at_mouse(z);
    g_cam_t = 0;
    g_cam_blend_seen = -1;   // the next frame takes the camera's blend as a ride's start
    g_wheel_zooms++;
}
// The zoom keys: the game's own up to its 1, and from there on ours (a step of the game's 0.0035 .. 0.01 by its curve,
// growing with the distance as the wheel's: the factor is the wheel's step at g_cam_v over its 0.05 at 1)
#define CAM_KEY_STEP 0.0065f
internal uint32_t __thiscall hook_hot_key_zoom(eeuie_hot_key_zoom *self, ee_ui_form *form, uint32_t arg) {
    if (scroll_speed() >= 1.0f && camera_zoom_max() > 1.0f && wheel_zoom_free()) {
        int out = eeuie_hot_key_zoom_get_zooms_out(self) != 0;
        if (out || g_cam_v > 1.0f) {
            float step = CAM_KEY_STEP * (2.0f * g_cam_v - 1.0f);
            camera_zoom_set(g_cam_v + (out ? step : -step));
            return 0;
        }
    }
    return eeuie_hot_key_zoom_execute(self, form, arg);
}
internal uint32_t __thiscall hook_wheel_zoom(eeuie_wheel_zoom *self, ee_ui_form *form, uint32_t arg) {
    void *uim = ui_manager();
    if (g_wheel_hook && wheel_ours(uim ? ui_current_form(uim) : NULL)) return 0;
    // the editor's wheel (its lists keep theirs): on past the game's 1, as the game's form's
    if (editor_is_on() && camera_zoom_max() > 1.0f && wheel_zoom_free()) {
        int n = mouse_wheel_notches();
        if (n) camera_zoom_at_mouse(camera_zoom_notches(n));
        return 0;
    }
    return eeuie_wheel_zoom_execute(self, form, arg);
}

// The middle button's drag the same way (the player 2026-11-07: "middle mouse scroll doesn't work yet"): the game's
// (EEUIEGrabScroll, the form's middle-button event) freezes the game's mouse and moves the camera by the mouse's
// counts in the form's input on the game thread - through a step not at all. With the mouse hook in place (the own
// renderer, not headless) the render thread does it on the game's screen: a middle press in the game's window freezes
// the game's mouse as the event does (its update, on this thread, leaves the cursor: display.c hook_mouse_update), the
// Windows cursor is put back where the press was every frame and the camera moved by how far it went, at the event's
// speed (a pixel for a count: (grab_setting + 0.5) x (scroll_speed + 0.5) x 0.07, y the other way); let go, the window
// left or another form: the mouse free again. The game's event does nothing meanwhile (hook_grab_scroll).
internal int g_grab, g_grab_down;   // the drag held; the middle button down at the last frame
internal POINT g_grab_at;
internal void grab_frame(const ee_ui_form *f, ee_mouse *m) {
    int down = GetAsyncKeyState(VK_MBUTTON) < 0, pressed = down && !g_grab_down;
    g_grab_down = down;
    if (!g_wheel_hook || !m) return;
    HWND w = game_in_front();
    int ours = w && ui_form_is_game(f);
    POINT p;
    if (!GetCursorPos(&p)) return;
    if (!g_grab) {
        RECT rc;
        POINT c = p;
        if (!pressed || !ours || !ScreenToClient(w, &c) || !GetClientRect(w, &rc) || !PtInRect(&rc, c)) return;
        g_grab = 1;
        g_grab_at = p;
        mouse_set_frozen(m, 1);
        return;
    }
    if (!down || !ours) {
        g_grab = 0;
        mouse_set_frozen(m, 0);
        return;
    }
    int dx = p.x - g_grab_at.x, dy = p.y - g_grab_at.y;
    if (!dx && !dy) return;
    SetCursorPos(g_grab_at.x, g_grab_at.y);
    float k = (grab_setting() + 0.5f) * (scroll_speed() + 0.5f) * 0.07f;
    camera_pan(k * (float)dx, -k * (float)dy);
}
// The minimap the same way (the player 2026-11-07: "the last open point is moving the camera via clicking and dragging
// on the minimap"): its control's mouse (EEUIMiniMap's button up and move, the game thread) queues its action
// (EEAMiniMap: the camera centred on the minimap's point) for the action loop, which runs it under the world's write
// lock - both wait for a step. With the mouse hook in place the render thread does it on the game's screen: a left
// press (the physical button, as the game's DirectInput mouse) with the game's cursor - moved on this thread - over
// the minimap control (its pointer from its copy_data, which the render objects' update runs every frame) starts it,
// and while the button is held every new point is run at once through an action of its own (the same code: the
// point onto the map, kept on it, the camera centred, its zoom, snap); let go, another form, the mouse frozen (the
// middle button's drag) or a flare being placed (the mouse's mode 0x16: the game's) end it. The game's minimap
// control queues none of its own meanwhile (hook_minimap_queue); the scenario editor's minimap stays the game's.
internal eeui_mini_map *g_minimap;   // the minimap control of the form its copy_data ran for last
internal int g_mm_drag, g_mm_down;
internal float g_mm_at[2];
internal uint32_t g_mm_action[8];    // the render thread's own EEAMiniMap (0x1C bytes)
internal int __thiscall hook_minimap_copy_data(eeui_mini_map *c) {
    g_minimap = c;
    return eeui_mini_map_copy_data(c);
}
// the pipe's vmini: the frames of a drag left (1: a click), from g_mm_test_at[0..1] to [2..3] (the form's pixels) in
// g_mm_test_n frames, a point a frame
internal volatile LONG g_mm_test;
internal LONG g_mm_test_n;
internal float g_mm_test_at[4];
internal LONG g_mm_runs;
internal void minimap_run(eeui_mini_map *c, float x, float y) {
    float p[2];
    minimap_point(c, x, y, p);
    if (p[0] == g_mm_at[0] && p[1] == g_mm_at[1]) return;
    g_mm_at[0] = p[0];
    g_mm_at[1] = p[1];
    eea_mini_map_execute(minimap_action_init(g_mm_action, p));
    g_mm_runs++;
}
internal void minimap_frame(const ee_ui_form *f, ee_mouse *m) {
    int down = GetAsyncKeyState(VK_LBUTTON) < 0, pressed = down && !g_mm_down;
    g_mm_down = down;
    eeui_mini_map *c = g_minimap;
    LONG left = g_mm_test;
    if (left > 0 && c && ui_form_is_game(f)) {
        float k = g_mm_test_n > 1 ? (float)(g_mm_test_n - left) / (float)(g_mm_test_n - 1) : 0;
        float x = g_mm_test_at[0] + (g_mm_test_at[2] - g_mm_test_at[0]) * k, y = g_mm_test_at[1] + (g_mm_test_at[3] - g_mm_test_at[1]) * k;
        if (left == g_mm_test_n) g_mm_at[0] = g_mm_at[1] = -1e9f;
        if (ui_control_contains((const ee_ui_control *)c, x, y)) minimap_run(c, x, y);
        g_mm_test = left - 1;
    } else g_mm_test = 0;
    if (!g_wheel_hook || !c || !m) return;
    int ours = game_in_front() && ui_form_is_game(f)
            && ui_control_get_form((const ee_ui_control *)c) == f && !mouse_get_frozen(m) && mouse_get_mode(m) != 0x16;
    float x = mouse_get_x(m), y = mouse_get_y(m);
    if (!g_mm_drag) {
        if (!pressed || !ours || !ui_control_contains((const ee_ui_control *)c, x, y)) return;
        g_mm_drag = 1;
        g_mm_at[0] = g_mm_at[1] = -1e9f;
    } else if (!down || !ours) {
        g_mm_drag = 0;
        return;
    }
    minimap_run(c, x, y);
}
// The game's control queues nothing meanwhile: not its actions skipped as they come (the player 2026-11-07: "dragging
// on the minimap still has weird effects, the fps drop sharply and numbers flicker in and out") - it queued one for
// every mouse move while the button was held, and the action loop took the interface's lock and the world's write
// lock for each, skipped or not, as the edge's timer had (camera_install).
internal void __thiscall hook_minimap_queue(eeui_mini_map *c) {
    void *uim = ui_manager();
    if (g_wheel_hook && ui_form_is_game(uim ? ui_current_form(uim) : NULL)) return;
    eeui_mini_map_queue_point(c);
}
internal uint32_t __thiscall hook_grab_scroll(eeuie_grab_scroll *self, ee_ui_form *form, uint32_t arg) {
    void *uim = ui_manager();
    if (g_wheel_hook && ui_form_is_game(uim ? ui_current_form(uim) : NULL)) return 0;
    return eeuie_grab_scroll_execute(self, form, arg);
}

// With the own renderer the input alone scrolls, every frame (the player 2026-11-07: "why leave the camera at all to
// the simthread now?" - a step held seconds, the pipe's stepload, stopped it after CAM_HOLD_MS). The own renderer
// draws only windowed or borderless, so the Windows cursor is the game's (native fullscreen's DirectInput mouse isn't:
// there, and with the game's renderers, the game thread's reports as above). The game's own conditions: a form with
// the 3D view is the current one (not the options, the diplomacy, a menu), the game's mouse not frozen (the edge
// check 0x4051A0 tests it first), the game's window in front; the arrows not while an edit box has the keyboard. The
// cursor at an edge at the edge's speed, the arrows at theirs. The game thread's scroll events still come and move
// nothing (hook_scroll_execute, hook_key_scroll_execute).
internal void input_frame(float ms) {
    void *uim = ui_manager();
    ee_ui_form *f = uim ? ui_current_form(uim) : NULL;
    wheel_frame(f);   // a frozen mouse still zooms: the game's update then reads only the wheel
    ee_mouse *m = game_mouse();
    grab_frame(f, m);
    minimap_frame(f, m);
    if (!ui_form_has_view(f) || !m || mouse_get_frozen(m) || !game_in_front()) return;
    scroll_move(ms, scroll_edges());
    int a = ui_form_get_keyboard_captured(f) ? 0 : scroll_arrows();
    float step = (scroll_speed() + 0.5f) * (scroll_setting() + 0.5f) * 0.005f * ms;   // arrow_frame's speed
    float dx = (a & 2 ? step : 0) - (a & 1 ? step : 0), dy = (a & 4 ? step : 0) - (a & 8 ? step : 0);   // up +y, as arrow_frame
    if (dx != 0 || dy != 0) camera_pan(dx, dy);
}

internal void camera_frame(int own) {   // the render thread, every presented frame; own: the own renderer draws
    camera_shape_frame();
    if (!g_cam_smooth) return;
    LARGE_INTEGER now;
    if (!g_cam_freq.QuadPart) QueryPerformanceFrequency(&g_cam_freq);
    QueryPerformanceCounter(&now);
    double ms = g_cam_frame.QuadPart ? (double)(now.QuadPart - g_cam_frame.QuadPart) * 1000.0 / (double)g_cam_freq.QuadPart : 0;
    g_cam_frame = now;
    if (own) {
        if (game_camera()) input_frame(ms > 100.0 ? 100.0f : (float)ms);
        return;
    }
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
    g_scroll_aoc = g_set.scroll == SCROLL_GAME ? build_aoc : g_set.scroll == SCROLL_AOC;
    g_scroll_native = g_scroll_aoc == build_aoc;
    g_cam_smooth = g_set.smooth_scroll;
    camera_shape_install();
    patch_hot_key_zoom_execute(hook_hot_key_zoom);
    // the arrow keys still while an edit box has the keyboard, which a click beside it lets go: always
    int ok = patch_edit_box_keyboard(hook_edit_keyboard, &g_edit_keyboard) + patch_key_scroll_execute(hook_key_scroll_execute);
    if (g_scroll_native && !g_cam_smooth) {   // the build's own way without smoothing: the game untouched but for that
        if (g_log) fprintf(g_log, "# camera: edit boxes and the arrow keys, patches %d/2\n", ok);
        return;
    }
    ok += patch_scroll_execute(hook_scroll_execute);
    // push 25 (AoC 10) -> 10 ms ticks like AoC, 1 ms fine. With the own renderer (display_install before this) the
    // input alone scrolls (input_frame) and the timer's actions only cost: the game's action loop (0x6608D7) runs them
    // under the interface's lock and the world's write lock, and at 1 ms the cursor at an edge took both up to a
    // thousand times a second - the frames went without the game's update or waited for the hand-off, the picture
    // jerked in a normal game and not in a long step, where those writes waited for the step (the player 2026-11-07:
    // "the edge scroll is smooth with high stepload, but shows lag in a normal running game"). There push -1: the
    // period never comes round (TMPoll::Poll: the time gone / the period), the edge check (0x4051A0) queues one action
    // as the cursor reaches the edge and starts the timer, then none till it comes back.
    ok += patch_scroll_timer(build_aoc ? 0x0A : 0x19, g_cam_smooth && g_rd_on ? 0xFF : g_cam_smooth ? 0x01 : 0x0A);
    if (g_cam_smooth && g_rd_on && patch_wheel_zoom_execute(hook_wheel_zoom) + patch_grab_scroll_execute(hook_grab_scroll)
        + patch_minimap_copy_data(hook_minimap_copy_data) + patch_minimap_queue(hook_minimap_queue, NULL) == 5 && !g_headless) {
        HANDLE t = CreateThread(NULL, 0, wheel_thread, NULL, 0, NULL);
        if (t) CloseHandle(t);
    }
    if (g_log) fprintf(g_log, "# camera: scroll=%s (%s), fine steps %d, patches %d/4\n", g_scroll_aoc ? "aoc" : "ee",
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
