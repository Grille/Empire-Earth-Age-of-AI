// Camera scrolling: the original Empire Earth behaviour or Art of Conquest's smooth scrolling, backported.
// aimod.ini [camera] scroll = ee | aoc (default ee: nothing is changed).
//
// Scrolling (mouse at the screen edge / keys) is the action EEAScroll (vtable 0x825098, slot 1 Execute 0x4050FC),
// queued by a timer every 25 ms (ctor 0x40509D: `push 0x19` at 0x4050C2). Fields: +0x40 elapsed timer ticks,
// +0x44 left, +0x45 right, +0x46 down, +0x47 up. EE moves the camera at a constant speed:
//   step = (speed 0x918DB0 + 0.5) * (scroll setting 0x9193A8 + 0.5) * ticks * 0.25
// AoC (EE-AOC.exe 0x40478E) ticks every 10 ms and keeps a velocity per axis that starts at 10% and ramps up to
// full speed (+1% of the maximum per elapsed tick); reversing an axis starts slow again and the velocity is reset
// when scrolling stops. The top speed is the same as EE's: per second 100 x 0.1 vs 40 x 0.25.

#define VT_SCROLL          0x00825098
#define FN_SCROLL_EXECUTE  0x004050FC
#define SITE_SCROLL_TIMER  0x004050C2   // operand of push 0x19 (timer period in ms)
#define G_SCROLL_SPEED     0x00918DB0   // float
#define G_SCROLL_SETTING   0x009193A8   // float, settings slider x 0.01
#define FN_CAMERA_MOVE     0x0051E2E6   // thiscall on G_CAMERA (float dx, float dy)

static int g_scroll_aoc;
static float g_vel_x, g_vel_y;
static DWORD g_scroll_last;

static void scroll_axis(float *vel, int neg, int pos, float minv, float maxv, float step) {
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

static void __fastcall hook_scroll_execute(uint8_t *self, void *edx) {
    (void)edx;
    if (!g_scroll_aoc) { ((void (__thiscall *)(void *))FN_SCROLL_EXECUTE)(self); return; }
    DWORD t = GetTickCount();
    if (t - g_scroll_last > 100) g_vel_x = g_vel_y = 0;   // scrolling had stopped: start slow again
    g_scroll_last = t;
    float ticks = (float)FIELD(self, 0x40, unsigned);
    float maxv = *(float *)G_SCROLL_SETTING + 0.5f, minv = maxv * 0.1f;
    float scale = (*(float *)G_SCROLL_SPEED + 0.5f) * ticks * 0.1f;
    float step = ticks * 0.01f * maxv;
    float dx = 0, dy = 0;
    if (self[0x44] || self[0x45]) { scroll_axis(&g_vel_x, self[0x44], self[0x45], minv, maxv, step); dx = g_vel_x * scale; }
    if (self[0x47] || self[0x46]) { scroll_axis(&g_vel_y, self[0x47], self[0x46], minv, maxv, step); dy = g_vel_y * scale; }
    if (dx != 0 || dy != 0) ((void (__thiscall *)(void *, float, float))FN_CAMERA_MOVE)((void *)G_CAMERA, dx, dy);
}

static void camera_install(const char *aimod_ini, const char *autotest_ini) {
    char mode[16], tmp[16];
    GetPrivateProfileStringA("camera", "scroll", "ee", mode, sizeof mode, aimod_ini);
    if (autotest_ini) {   // tests: [autotest] camera_scroll overrides
        lstrcpynA(tmp, mode, sizeof tmp);
        GetPrivateProfileStringA("autotest", "camera_scroll", tmp, mode, sizeof mode, autotest_ini);
    }
    g_scroll_aoc = !lstrcmpiA(mode, "aoc");
    if (!g_scroll_aoc) return;   // ee: leave the game untouched
    int ok = patch_vtable(VT_SCROLL, 1, FN_SCROLL_EXECUTE, hook_scroll_execute);
    uint8_t *p = (uint8_t *)SITE_SCROLL_TIMER;
    if (p[-1] == 0x6A && p[0] == 0x19) {   // push 0x19 -> push 0x0A (10 ms ticks like AoC)
        DWORD old;
        VirtualProtect(p, 1, PAGE_EXECUTE_READWRITE, &old);
        *p = 0x0A;
        VirtualProtect(p, 1, old, &old);
        FlushInstructionCache(GetCurrentProcess(), p, 1);
        ok++;
    }
    if (g_log) fprintf(g_log, "# camera: scroll=aoc (smooth), patches %d/2\n", ok);
}

// scrolltest <left|right|up|down> <calls>: run the scroll action on a fake object, report camera movement
static void cmd_scrolltest(char *out, const char *args) {
    char dir[16] = "right";
    int n = 10;
    sscanf(args, "%15s %d", dir, &n);
    uint8_t obj[0x50] = { 0 };
    *(uint32_t *)obj = VT_SCROLL;
    *(unsigned *)(obj + 0x40) = 1;
    obj[0x44] = dir[0] == 'l'; obj[0x45] = dir[0] == 'r'; obj[0x46] = dir[0] == 'd'; obj[0x47] = dir[0] == 'u';
    uint8_t *cam = (uint8_t *)G_CAMERA;
    float x0 = FIELD(cam, 0x78, float), y0 = FIELD(cam, 0x7C, float);
    REPLY("mode %s, start %.2f,%.2f\n", g_scroll_aoc ? "aoc" : "ee", x0, y0);
    for (int i = 0; i < n && i < 200; i++) {
        float x = FIELD(cam, 0x78, float), y = FIELD(cam, 0x7C, float);
        hook_scroll_execute(obj, NULL);
        if (i < 12 || i == n - 1)
            REPLY("  call %2d: moved %.3f,%.3f (velocity %.3f,%.3f)\n", i, FIELD(cam, 0x78, float) - x,
                  FIELD(cam, 0x7C, float) - y, g_vel_x, g_vel_y);
    }
}
