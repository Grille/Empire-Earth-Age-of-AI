// THE AGE OF AI MAP'S OPTIONS (the player 2026-11-25: the mod's map type one entry, "Age of AI", its options in a
// dialog, with "a preview ... to give a rough idea what the setting will produce"): [random_map]'s keys, a dropdown
// each - their values those of the row (rows.h), each shown by its word map_v_<name> (language.eeai), the label its
// key's word - and beside them the map they make (mapgen_preview), made anew at each change. The scenario editor's
// Create Map with that type asks for them first (editor_undo.c's hook of EESEACreateRandomMap.run): its size and the
// seed are chosen by then, so the preview is the map OK makes (mapgen_preview: how alike) - for
// as many players as the editor has. New seed draws another (the editor's own, as its panel's Random does; the made
// map takes it). OK makes the map with them, Cancel no map and the options as they were. What OK takes is kept -
// g_set for the maps to come, and the file: the player's ageofai.ini, a test's own ini ([autotest] overrides a row by
// its name; the test's ageofai.ini is the repository's shipped one, never written). A modal dialog over the game in
// its look (game_dialog.c). Next the setup screens' button (the plan the player agreed to).
#define MD_FIRST SETTING_map_layout
#define MD_ROWS (SETTING_map_mines - SETTING_map_layout + 1)
#define MD_COMBO 200    // + the row: its dropdown
#define MD_LABEL 230    // + the row: its label
#define MD_HINT 260
#define MD_PREVIEW 261
#define MD_RESEED 262
#define MD_SEED 263
#define MD_POINTS 9     // its font's size at the game's picture 1:1 ...
#define MD_PIXELS 460   // ... and about its height then, with the title bar
internal struct {
    int result;
    int n, np;          // the map's tiles a side and players
    uint32_t seed;      // the map's seed (the editor's, or a new one drawn here)
    int was[MD_ROWS];   // the options as they were: Cancel puts them back
    uint32_t *px;       // the preview's picture, side x side
    int side;
} g_md;

// a label's word, a space in: off its box's edge (the caller copies it before the next)
internal const wchar_t *md_lead(const char *key) {
    static wchar_t t[120];
    t[0] = L' ';
    lstrcpynW(t + 1, word_w(key), 119);
    return t;
}
internal int *md_value(int row) { return (int *)((uint8_t *)&g_set + k_settings[MD_FIRST + row].off); }

// the labels as tall as their dropdowns and beside them (a dropdown in the game's look is taller than the template's
// at a large font: theme_control); the seed's label as tall as New seed
internal void md_rows(HWND d) {
    for (int i = 0; i <= MD_ROWS; i++) {
        RECT f, r;
        HWND c = GetDlgItem(d, i < MD_ROWS ? MD_LABEL + i : MD_SEED);
        GetWindowRect(GetDlgItem(d, i < MD_ROWS ? MD_COMBO + i : MD_RESEED), &f);
        GetWindowRect(c, &r);
        MapWindowPoints(NULL, d, (POINT *)&f, 2);
        MapWindowPoints(NULL, d, (POINT *)&r, 2);
        SetWindowPos(c, NULL, r.left, f.top, r.right - r.left, f.bottom - f.top, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

// the preview made anew for the dropdowns' choices (into g_set: mapgen_preview reads them) - as large as its box
internal void md_render(HWND d) {
    for (int i = 0; i < MD_ROWS; i++) {
        LRESULT k = SendDlgItemMessageW(d, MD_COMBO + i, CB_GETCURSEL, 0, 0);
        if (k >= 0) *md_value(i) = (int)k;
    }
    RECT r;
    GetClientRect(GetDlgItem(d, MD_PREVIEW), &r);
    int side = (r.right < r.bottom ? r.right : r.bottom) - 2 * theme_px(2);
    if (side < 16) return;
    if (side != g_md.side || !g_md.px) {
        free(g_md.px);
        g_md.px = malloc((size_t)side * (size_t)side * sizeof *g_md.px);
        g_md.side = g_md.px ? side : 0;
    }
    char what[200] = "";
    HCURSOR was = SetCursor(LoadCursorW(NULL, (LPCWSTR)IDC_WAIT));
    LARGE_INTEGER f, t0, t1;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t0);
    int ok = g_md.px && mapgen_preview(g_md.n, g_md.np, g_md.seed, g_md.px, g_md.side, what, sizeof what);
    QueryPerformanceCounter(&t1);
    SetCursor(was);
    if (!ok) { free(g_md.px); g_md.px = NULL; g_md.side = 0; }
    wchar_t t[64];
    t[0] = L' ';
    word_fill_w(t + 1, 63, "map_seed", g_md.seed, 0);
    SetDlgItemTextW(d, MD_SEED, t);
    InvalidateRect(GetDlgItem(d, MD_PREVIEW), NULL, FALSE);
    if (g_log) log_locked("# mapgen: preview - %d tiles a side, %d players, seed %u: %s (%.0f ms)\n", g_md.n, g_md.np, g_md.seed,
                          ok ? what : "no memory", (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)f.QuadPart);
}

// the preview's box: the picture in its middle in a sunken frame, the parchment round it
internal void md_draw(const DRAWITEMSTRUCT *di) {
    RECT r = di->rcItem;
    theme_under(di->hDC, di->hwndItem, &r);
    if (!g_md.px) return;
    int s = g_md.side, b = theme_px(2), x = (r.right - r.left - s) / 2, y = (r.bottom - r.top - s) / 2;
    RECT f = { x - b, y - b, x + s + b, y + s + b };
    theme_bevel(di->hDC, &f, 1);
    BITMAPINFOHEADER h = { sizeof h, s, -s, 1, 32, BI_RGB, 0, 0, 0, 0, 0 };
    SetDIBitsToDevice(di->hDC, x, y, (DWORD)s, (DWORD)s, 0, 0, 0, (UINT)s, g_md.px, (BITMAPINFO *)&h, DIB_RGB_COLORS);
}

internal INT_PTR CALLBACK md_proc(HWND d, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_INITDIALOG: {
        SetWindowTextW(d, word_w("map_options"));
        game_dialog_init(d);
        for (int i = 0; i < MD_ROWS; i++) {
            const struct setting_row *r = &k_settings[MD_FIRST + i];
            HWND c = GetDlgItem(d, MD_COMBO + i);
            for (int k = 0; ; k++) {
                char name[40], key[48];
                setting_choice_name(r->choices, k, name, sizeof name);
                if (!name[0]) break;
                snprintf(key, sizeof key, "map_v_%s", name);
                SendMessageW(c, CB_ADDSTRING, 0, (LPARAM)word_w(key));
            }
            SendMessageW(c, CB_SETCURSEL, (WPARAM)*md_value(i), 0);
            theme_control(c, WC_COMBOBOXA);
        }
        static const int buttons[] = { IDOK, IDCANCEL, MD_RESEED };
        for (unsigned k = 0; k < sizeof buttons / sizeof *buttons; k++) theme_control(GetDlgItem(d, buttons[k]), WC_BUTTONA);
        md_rows(d);
        md_render(d);
        SetFocus(GetDlgItem(d, IDOK));
        return FALSE;
    }
    case WM_ERASEBKGND:
        if (!theme_erase(d, (HDC)w)) break;
        SetWindowLongPtrW(d, DWLP_MSGRESULT, 1);
        return TRUE;
    case WM_CTLCOLORSTATIC: case WM_CTLCOLOREDIT: case WM_CTLCOLORLISTBOX: case WM_CTLCOLORBTN: case WM_CTLCOLORDLG:
        return (INT_PTR)theme_ctlcolor(msg, w, l);   // a dialog procedure answers these with the brush itself
    case WM_DRAWITEM:
        if (w == MD_PREVIEW) { md_draw((const DRAWITEMSTRUCT *)l); return TRUE; }
        if (theme_draw_item((const DRAWITEMSTRUCT *)l)) return TRUE;   // the dropdowns' lists
        break;
    case WM_COMMAND:
        if (LOWORD(w) >= MD_COMBO && LOWORD(w) < MD_COMBO + MD_ROWS && HIWORD(w) == CBN_SELCHANGE) { md_render(d); return TRUE; }
        if (LOWORD(w) == MD_RESEED && HIWORD(w) == BN_CLICKED) {   // as the panel's Random: 0 .. 999999
            uint32_t s = g_md.seed ^ GetTickCount() ^ 0x9E3779B9u;
            s ^= s << 13; s ^= s >> 17; s ^= s << 5;
            g_md.seed = s % 1000000u;
            md_render(d);
            return TRUE;
        }
        if (LOWORD(w) == IDOK) md_render(d);   // the choices into g_set (as the preview has them)
        if (LOWORD(w) == IDOK || LOWORD(w) == IDCANCEL) {
            g_md.result = LOWORD(w);
            EndDialog(d, 0);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// the choices into the file: the player's ageofai.ini [random_map], a test's own ini [autotest]
internal void md_keep(void) {
    for (int i = 0; i < MD_ROWS; i++) {
        const struct setting_row *r = &k_settings[MD_FIRST + i];
        char name[40];
        setting_choice_name(r->choices, *md_value(i), name, sizeof name);
        WritePrivateProfileStringA(g_autotest ? "autotest" : r->sec, r->key, name, g_autotest ? g_ini : g_aimod_ini);
    }
}

// the dialog for a map of n tiles a side and np players from *seed (a new one drawn: there): 1 OK (the choices in
// g_set and the file), 0 cancelled or not opened
internal int md_open(int n, int np, uint32_t *seed) {
    int points = game_dialog_points(MD_POINTS, MD_PIXELS), ctx;
    static WORD tpl[2048];
    memset(tpl, 0, sizeof tpl);
    DLGTEMPLATE *dt = (DLGTEMPLATE *)tpl;
    *dt = (DLGTEMPLATE){ .style = DS_MODALFRAME | DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU, .cdit = 2 * MD_ROWS + 6, .cx = 412, .cy = 221 };
    WORD *p = (WORD *)(dt + 1);
    *p++ = 0, *p++ = 0;               // no menu, the dialog class
    p = cpk_tpl_str(p, L"");          // the title: set at its start
    *p++ = (WORD)points;
    p = cpk_tpl_str(p, L"Segoe UI");  // ageofai_config.exe's (Windows' message font)
    p = cpk_tpl_item(p, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, 298, 199, 50, 15, IDOK, 0x80, word_w("ok"));
    p = cpk_tpl_item(p, BS_PUSHBUTTON | WS_TABSTOP, 354, 199, 50, 15, IDCANCEL, 0x80, word_w("cancel"));
    for (int i = 0; i < MD_ROWS; i++) {
        short y = (short)(8 + 17 * i);
        p = cpk_tpl_item(p, SS_LEFT | SS_CENTERIMAGE, 8, y, 62, 12, (WORD)(MD_LABEL + i), 0x82, md_lead(k_settings[MD_FIRST + i].key));
        p = cpk_tpl_item(p, CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP | WS_GROUP, 72, y, 120, 100,
                         (WORD)(MD_COMBO + i), 0x85, L"");
    }
    p = cpk_tpl_item(p, SS_LEFT | SS_CENTERIMAGE, 8, 180, 184, 12, MD_HINT, 0x82, md_lead("map_starts_keep"));
    p = cpk_tpl_item(p, SS_OWNERDRAW, 204, 8, 200, 184, MD_PREVIEW, 0x82, L"");
    p = cpk_tpl_item(p, BS_PUSHBUTTON | WS_TABSTOP, 8, 199, 72, 15, MD_RESEED, 0x80, word_w("map_new_seed"));
    p = cpk_tpl_item(p, SS_LEFT | SS_CENTERIMAGE, 84, 199, 108, 12, MD_SEED, 0x82, L"");
    g_md.result = IDCANCEL;
    g_md.n = n; g_md.np = np; g_md.seed = *seed;
    for (int i = 0; i < MD_ROWS; i++) g_md.was[i] = *md_value(i);
    INT_PTR r = game_dialog_show(dt, md_proc, &ctx);
    free(g_md.px);
    g_md.px = NULL; g_md.side = 0;
    if (g_md.result == IDOK) { md_keep(); *seed = g_md.seed; }
    else for (int i = 0; i < MD_ROWS; i++) *md_value(i) = g_md.was[i];
    if (g_log) {
        char line[300];
        int at = snprintf(line, sizeof line, "# mapgen: the options' dialog (%s, %d points): %s", ctx ? "common controls 6" : "the old controls", points,
                          r < 0 ? "FAILED to open" : g_md.result == IDOK ? "OK -" : "cancel");
        for (int i = 0; g_md.result == IDOK && i < MD_ROWS && at > 0 && at < (int)sizeof line - 40; i++) {
            char name[40];
            setting_choice_name(k_settings[MD_FIRST + i].choices, *md_value(i), name, sizeof name);
            at += snprintf(line + at, sizeof line - (size_t)at, " %s %s", k_settings[MD_FIRST + i].key + 4, name);
        }
        if (g_md.result == IDOK && at > 0 && at < (int)sizeof line - 20) snprintf(line + at, sizeof line - (size_t)at, ", seed %u", g_md.seed);
        log_locked("%s\n", line);
    }
    return g_md.result == IDOK;
}

// the editor's Create Map (editor_undo.c's hook, before the map is made): with the mod's type chosen the options
// first - its size the action's, the players the scenario's (its Players screen's count: before run the game's 17
// slots all look alike), the seed the editor's (a new one drawn in
// the dialog becomes the editor's) - 0: cancelled, no map
internal int mapgen_editor_ask(ee_se_random_map *a) {
    int i = se_random_map_get_map_type(a), np = editor_get_world_player_count() - 1;   // the world is one of them: not a start
    if (i < 0 || (unsigned)i >= map_types_count() || !mapgen_is_ours(map_type_name_text(map_types_at((unsigned)i)))) return 1;
    ee_ui *ui = game_get_ui();
    uint32_t seed = ui ? ui_get_seed(ui) : 0;
    if (!md_open(map_scale_tiles(se_random_map_get_map_scale(a)), np > 0 ? np : 1, &seed)) return 0;
    if (ui) ui_set_seed(ui, seed);
    return 1;
}

// edmap [size [type]]: the editor's Create Map as its button makes it - the map size's index in its list (default 1,
// small) and a map type by its name (default the mod's) - for a test's way to the dialog without the panel's clicks.
// Queued by its constructor: run with the editor's next actions, the seed the one the editor's last map had
internal void cmd_edmap(char *out, const char *args) {
    int size = 1;
    char name[64] = MG_NAME;
    sscanf(args, "%d %63[^\n]", &size, name);
    unsigned i = 0, n = map_types_count();
    while (i < n && lstrcmpiA(map_type_name_text(map_types_at(i)), name)) i++;
    if (i == n) { REPLY("error: no map type \"%s\" (%u types)\n", name, n); return; }
    if (!se_random_map_new((uint32_t)size, i)) { REPLY("error: no memory\n"); return; }
    REPLY("ok: Create Map queued - size %d, type %u \"%s\"\n", size, i, name);
}
