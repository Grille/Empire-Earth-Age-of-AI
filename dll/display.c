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
    if (g_log) fprintf(g_log, "# editor: resolution=%s hooks %d/2\n", mode, ok);
}
