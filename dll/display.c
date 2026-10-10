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

static void display_install(const char *aimod_ini, const char *autotest_ini) {
    char mode[32], tmp[32];
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
