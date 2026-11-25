// A DIALOG OF THE MOD'S OVER THE GAME: a modal Win32 dialog over the game's window, on the game's thread - the game
// waits meanwhile; the same program's window, so no app switch reaches the game (it would minimize itself). In the
// game's look (ui/theme.c, ageofai_config.exe's controls in the editor's style: the parchment of its pages under it,
// yellow text on its darker boxes, the game's radio buttons, dropdowns and buttons; plain dark without the game's
// pictures), and at the scale the game's picture is shown at: at a lower resolution, enlarged to the window, it is as
// much larger as the game's own controls. The terrain colour dialog (terrain_dialog.c) and the map options
// (mapgen_dialog.c).

// the font's size for a dialog whose font is `points` at the game's picture 1:1 and which is about `pixels` high then
// (with its title bar): the scale the game's picture is shown at (windowed or borderless it is enlarged to the window,
// the bars left out) - the dialog units follow the font, what is drawn in pixels g_theme_zoom (set here); it must fit
// in the window
internal int game_dialog_points(int points0, int pixels) {
    ee_window *gw = game_window();
    HWND owner = gw ? window_get_hwnd(gw) : NULL;
    int points = points0;
    RECT cr;
    if (owner && GetClientRect(owner, &cr) && window_get_height(gw) > 0) {
        RECT v = view_rect(&cr, window_get_width(gw), window_get_height(gw));
        points = (points0 * (int)(v.bottom - v.top) + window_get_height(gw) / 2) / window_get_height(gw);
        if (points > points0 * cr.bottom / pixels) points = points0 * cr.bottom / pixels;
        if (points < 7) points = 7;
    }
    g_theme_zoom = points * 100 / points0;
    return points;
}

// a dialog's WM_INITDIALOG first: centred over the game's window (headless: off the screen with it), in the game's look
// with the painting under it (the labels on its darkened copy)
internal void game_dialog_init(HWND d) {
    HWND o = GetWindow(d, GW_OWNER);
    RECT ro, rd;
    if (o && GetWindowRect(o, &ro) && GetWindowRect(d, &rd))
        SetWindowPos(d, HWND_TOP, (ro.left + ro.right - (rd.right - rd.left)) / 2, (ro.top + ro.bottom - (rd.bottom - rd.top)) / 2, 0, 0, SWP_NOSIZE);
    theme_window(d);
    theme_size(d);
}

// the dialog shown over the game's window, Windows' cursor over it (the game's own hidden, the clip let go meanwhile),
// the game's pictures read at the first one (from the game's folder, else the original game's: AoC); *ctx: shown with
// the common controls 6 -> DialogBoxIndirectParamW's result (below 0: it could not open)
internal INT_PTR game_dialog_show(DLGTEMPLATE *dt, DLGPROC proc, int *ctx) {
    ee_window *gw = game_window();
    HWND owner = gw ? window_get_hwnd(gw) : NULL;
    RECT clip;
    int clipped = GetClipCursor(&clip);
    ClipCursor(NULL);
    int before = ShowCursor(TRUE) - 1, shown = before + 1;   // Windows' cursor over the dialog, its count as it was after
    while (shown < 0) shown = ShowCursor(TRUE);
    static int looked;
    if (!looked) {
        looked = 1;
        char ini[MAX_PATH + 32];
        snprintf(ini, sizeof ini, "%sageofai\\ageofai.ini", g_game_dir);   // its eesettings.ini beside it: the original's folder
        theme_look(ini, theme_style_editor());
        theme_app_mode(1);
    }
    ULONG_PTR cookie;
    *ctx = theme_context_enter(&cookie);
    INT_PTR r = DialogBoxIndirectParamW(GetModuleHandleW(NULL), dt, owner, proc, 0);
    if (*ctx) theme_context_leave(cookie);
    while (shown > before) shown = ShowCursor(FALSE);
    if (clipped) ClipCursor(&clip);
    if (owner) SetForegroundWindow(owner);
    return r;
}
