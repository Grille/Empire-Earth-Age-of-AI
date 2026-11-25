// THE AGE OF AI MAP'S OPTIONS (the player 2026-11-25: the mod's map type one entry, "Age of AI", its options in a
// dialog): [random_map]'s keys, a dropdown each - their values those of the row (rows.h), each shown by its word
// map_v_<name> (language.eeai), the label its key's word. The scenario editor's Create Map with that type asks for
// them first (editor_undo.c's hook of EESEACreateRandomMap.run): OK makes the map with them, Cancel no map. What OK
// takes is kept - g_set for the maps to come, and the file: the player's ageofai.ini, a test's own ini ([autotest]
// overrides a row by its name; the test's ageofai.ini is the repository's shipped one, never written). A modal dialog
// over the game in its look (game_dialog.c). Later the preview and the setup screens' button (the plan the player
// agreed to: step 1 the dialog in the editor).
#define MD_FIRST SETTING_map_layout
#define MD_ROWS (SETTING_map_mines - SETTING_map_layout + 1)
#define MD_COMBO 200    // + the row: its dropdown
#define MD_LABEL 230    // + the row: its label
#define MD_HINT 260
#define MD_POINTS 9     // its font's size at the game's picture 1:1 ...
#define MD_PIXELS 460   // ... and about its height then, with the title bar
internal int g_md_result;

// a label's word, a space in: off its box's edge (the caller copies it before the next)
internal const wchar_t *md_lead(const char *key) {
    static wchar_t t[120];
    t[0] = L' ';
    lstrcpynW(t + 1, word_w(key), 119);
    return t;
}
internal int *md_value(int row) { return (int *)((uint8_t *)&g_set + k_settings[MD_FIRST + row].off); }

// the labels as tall as their dropdowns and beside them (a dropdown in the game's look is taller than the template's
// at a large font: theme_control)
internal void md_rows(HWND d) {
    for (int i = 0; i < MD_ROWS; i++) {
        RECT f, r;
        HWND c = GetDlgItem(d, MD_LABEL + i);
        GetWindowRect(GetDlgItem(d, MD_COMBO + i), &f);
        GetWindowRect(c, &r);
        MapWindowPoints(NULL, d, (POINT *)&f, 2);
        MapWindowPoints(NULL, d, (POINT *)&r, 2);
        SetWindowPos(c, NULL, r.left, f.top, r.right - r.left, f.bottom - f.top, SWP_NOZORDER | SWP_NOACTIVATE);
    }
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
        theme_control(GetDlgItem(d, IDOK), WC_BUTTONA);
        theme_control(GetDlgItem(d, IDCANCEL), WC_BUTTONA);
        md_rows(d);
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
        if (theme_draw_item((const DRAWITEMSTRUCT *)l)) return TRUE;   // the dropdowns' lists
        break;
    case WM_COMMAND:
        if (LOWORD(w) == IDOK)
            for (int i = 0; i < MD_ROWS; i++) {
                LRESULT k = SendDlgItemMessageW(d, MD_COMBO + i, CB_GETCURSEL, 0, 0);
                if (k >= 0) *md_value(i) = (int)k;
            }
        if (LOWORD(w) == IDOK || LOWORD(w) == IDCANCEL) {
            g_md_result = LOWORD(w);
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

// the dialog: 1 OK (the choices in g_set and the file), 0 cancelled or not opened
internal int md_open(void) {
    int points = game_dialog_points(MD_POINTS, MD_PIXELS), ctx;
    static WORD tpl[2048];
    memset(tpl, 0, sizeof tpl);
    DLGTEMPLATE *dt = (DLGTEMPLATE *)tpl;
    *dt = (DLGTEMPLATE){ .style = DS_MODALFRAME | DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU, .cdit = 2 * MD_ROWS + 3, .cx = 200, .cy = 221 };
    WORD *p = (WORD *)(dt + 1);
    *p++ = 0, *p++ = 0;               // no menu, the dialog class
    p = cpk_tpl_str(p, L"");          // the title: set at its start
    *p++ = (WORD)points;
    p = cpk_tpl_str(p, L"Segoe UI");  // ageofai_config.exe's (Windows' message font)
    p = cpk_tpl_item(p, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, 86, 199, 50, 15, IDOK, 0x80, word_w("ok"));
    p = cpk_tpl_item(p, BS_PUSHBUTTON | WS_TABSTOP, 142, 199, 50, 15, IDCANCEL, 0x80, word_w("cancel"));
    for (int i = 0; i < MD_ROWS; i++) {
        short y = (short)(8 + 17 * i);
        p = cpk_tpl_item(p, SS_LEFT | SS_CENTERIMAGE, 8, y, 62, 12, (WORD)(MD_LABEL + i), 0x82, md_lead(k_settings[MD_FIRST + i].key));
        p = cpk_tpl_item(p, CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP | WS_GROUP, 72, y, 120, 100,
                         (WORD)(MD_COMBO + i), 0x85, L"");
    }
    p = cpk_tpl_item(p, SS_LEFT | SS_CENTERIMAGE, 8, 180, 184, 12, MD_HINT, 0x82, md_lead("map_starts_keep"));
    g_md_result = IDCANCEL;
    INT_PTR r = game_dialog_show(dt, md_proc, &ctx);
    if (g_md_result == IDOK) md_keep();
    if (g_log) {
        char line[300];
        int at = snprintf(line, sizeof line, "# mapgen: the options' dialog (%s, %d points): %s", ctx ? "common controls 6" : "the old controls", points,
                          r < 0 ? "FAILED to open" : g_md_result == IDOK ? "OK -" : "cancel");
        for (int i = 0; g_md_result == IDOK && i < MD_ROWS && at > 0 && at < (int)sizeof line - 40; i++) {
            char name[40];
            setting_choice_name(k_settings[MD_FIRST + i].choices, *md_value(i), name, sizeof name);
            at += snprintf(line + at, sizeof line - (size_t)at, " %s %s", k_settings[MD_FIRST + i].key + 4, name);
        }
        log_locked("%s\n", line);
    }
    return g_md_result == IDOK;
}

// the editor's Create Map (editor_undo.c's hook, before the map is made): with the mod's type chosen the options
// first - 0: cancelled, no map
internal int mapgen_editor_ask(ee_se_random_map *a) {
    int i = se_random_map_get_map_type(a);
    if (i < 0 || (unsigned)i >= map_types_count() || !mapgen_is_ours(map_type_name_text(map_types_at((unsigned)i)))) return 1;
    return md_open();
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
