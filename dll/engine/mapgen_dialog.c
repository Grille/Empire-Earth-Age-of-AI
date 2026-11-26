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
// its look (game_dialog.c). The game setup screen's options button opens it too (mapgen_button.c md_open_setup).
#define MD_FIRST SETTING_map_layout
#define MD_ROWS (SETTING_map_mines - SETTING_map_layout + 1)
#define MD_COMBO 200    // + the row: its dropdown
#define MD_LABEL 230    // + the row: its label
#define MD_HINT 260
#define MD_PREVIEW 261
#define MD_RESEED 262
#define MD_SEED 263
#define MD_KEEP 264
#define MD_POINTS 9     // its font's size at the game's picture 1:1 ...
#define MD_PIXELS 528   // ... and about its height then, with the title bar
#define MD_DONE (WM_APP + 1)   // the preview's worker is done
// a preview made by a worker (md_render): the map's whole steps, 0.75 s on a Titanic map - on the dialog's thread
// it froze the dialog at each choice
struct md_job {
    HWND d;
    int n, np, side, ok;
    uint32_t seed, *px;
    char what[200];
    double ms;
};
internal struct {
    int result;
    int n, np;          // the map's tiles a side and players
    uint32_t seed;      // the map's seed (the editor's, or a new one drawn here)
    int was[MD_ROWS];   // the options as they were: Cancel puts them back
    uint32_t *px;       // the preview's picture, side x side
    int side;
    int setup;          // asked on the game setup screen (mapgen_button.c): the seed an example, "play this map" offered
    int keep;           // ... and ticked
    HANDLE worker;      // the preview being made (job), NULL none
    int dirty;          // a choice changed meanwhile: made anew once it is done
    struct md_job job;
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

// the dropdowns' choices into g_set (mapgen_preview reads them) - never while a worker makes a preview of them
internal void md_read(HWND d) {
    for (int i = 0; i < MD_ROWS; i++) {
        LRESULT k = SendDlgItemMessageW(d, MD_COMBO + i, CB_GETCURSEL, 0, 0);
        if (k >= 0) *md_value(i) = (int)k;
    }
}
internal DWORD WINAPI md_work(void *arg) {
    struct md_job *j = arg;
    LARGE_INTEGER f, t0, t1;
    QueryPerformanceFrequency(&f);
    QueryPerformanceCounter(&t0);
    j->ok = j->px && mapgen_preview(j->n, j->np, j->seed, j->px, j->side, j->what, sizeof j->what);
    QueryPerformanceCounter(&t1);
    j->ms = (double)(t1.QuadPart - t0.QuadPart) * 1000.0 / (double)f.QuadPart;
    PostMessageW(j->d, MD_DONE, 0, 0);
    return 0;
}
// the preview made anew for the dropdowns' choices and the seed - as large as its box - by a worker: the dialog
// answers meanwhile, the box black till the new one is done (md_draw, MD_DONE: md_done). A change while one is being
// made only marks it dirty: the next is made of the choices then, once it is done - none made for each step of a
// scroll through a list. The game waits in the dialog's loop meanwhile: its data (mapgen_preview's types and
// textures) holds still
internal void md_render(HWND d) {
    if (g_md.worker || g_md.job.px) { g_md.dirty = 1; return; }   // (one made, not taken yet: its MD_DONE is coming)
    md_read(d);
    wchar_t t[64];
    t[0] = L' ';
    word_fill_w(t + 1, 63, "map_seed", g_md.seed, 0);
    SetDlgItemTextW(d, MD_SEED, t);
    RECT r;
    GetClientRect(GetDlgItem(d, MD_PREVIEW), &r);
    int side = (r.right < r.bottom ? r.right : r.bottom) - 2 * theme_px(2);
    if (side < 16) return;
    struct md_job *j = &g_md.job;
    memset(j, 0, sizeof *j);
    j->d = d; j->n = g_md.n; j->np = g_md.np; j->seed = g_md.seed; j->side = side;
    j->px = malloc((size_t)side * (size_t)side * sizeof *j->px);
    // (the generator's steps want more stack than a thread's default 1 MB might leave: as the game's main thread's)
    g_md.worker = CreateThread(NULL, 8u << 20, md_work, j, STACK_SIZE_PARAM_IS_A_RESERVATION, NULL);
    if (!g_md.worker) md_work(j);   // (no thread: made here, as before; its MD_DONE comes next)
    InvalidateRect(GetDlgItem(d, MD_PREVIEW), NULL, FALSE);   // (black till it is done: md_draw)
}
// a worker's preview taken: its picture shown, the next made if a choice changed meanwhile. show 0: only waited for
// (OK, Cancel: the dialog closes)
internal void md_done(HWND d, int show) {
    if (g_md.worker) { WaitForSingleObject(g_md.worker, INFINITE); CloseHandle(g_md.worker); g_md.worker = NULL; }
    struct md_job *j = &g_md.job;
    if (!j->px && !j->ok) return;   // (none made, or taken already)
    if (g_log) log_locked("# mapgen: preview - %d tiles a side, %d players, seed %u: %s (%.0f ms)\n", j->n, j->np, j->seed,
                          j->ok ? j->what : "no memory", j->ms);
    if (show && j->ok) { free(g_md.px); g_md.px = j->px; g_md.side = j->side; }
    else free(j->px);
    j->px = NULL; j->ok = 0;
    if (!show) return;
    InvalidateRect(GetDlgItem(d, MD_PREVIEW), NULL, FALSE);
    if (g_md.dirty) { g_md.dirty = 0; md_render(d); }
}

// the preview's box: the picture in its middle in a sunken frame, the parchment round it; black while one is being
// made, so an old picture isn't taken for the choices' map
internal void md_draw(const DRAWITEMSTRUCT *di) {
    RECT r = di->rcItem;
    theme_under(di->hDC, di->hwndItem, &r);
    int busy = g_md.worker || g_md.job.px;
    if (!busy && !g_md.px) return;
    int s = busy ? g_md.job.side : g_md.side, b = theme_px(2), x = (r.right - r.left - s) / 2, y = (r.bottom - r.top - s) / 2;
    RECT f = { x - b, y - b, x + s + b, y + s + b };
    theme_bevel(di->hDC, &f, 1);
    if (busy) {
        RECT in = { x, y, x + s, y + s };
        FillRect(di->hDC, &in, (HBRUSH)GetStockObject(BLACK_BRUSH));
        return;
    }
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
        static const int buttons[] = { IDOK, IDCANCEL, MD_RESEED, MD_KEEP };
        for (unsigned k = 0; k < sizeof buttons / sizeof *buttons; k++) theme_control(GetDlgItem(d, buttons[k]), WC_BUTTONA);
        if (!g_md.setup) ShowWindow(GetDlgItem(d, MD_KEEP), SW_HIDE);
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
    case MD_DONE:
        md_done(d, 1);
        return TRUE;
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
        if (LOWORD(w) == IDOK || LOWORD(w) == IDCANCEL) {
            md_done(d, 0);   // (a preview being made: waited for, its picture let go)
            if (LOWORD(w) == IDOK) md_read(d), g_md.keep = IsDlgButtonChecked(d, MD_KEEP) == BST_CHECKED;   // the choices into g_set
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
// g_set and the file), 0 cancelled or not opened. setup: the game setup screen's - "play this map" offered
internal int md_open(int n, int np, uint32_t *seed, int setup) {
    int points = game_dialog_points(MD_POINTS, MD_PIXELS), ctx;
    static WORD tpl[2048];
    memset(tpl, 0, sizeof tpl);
    DLGTEMPLATE *dt = (DLGTEMPLATE *)tpl;
    // a row 17 units high, the hint under them, the buttons under it (12 rows: 254 units high)
    short hint = (short)(9 + 17 * MD_ROWS), by = (short)(hint + 19);
    *dt = (DLGTEMPLATE){ .style = DS_MODALFRAME | DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU, .cdit = 2 * MD_ROWS + 7, .cx = 412, .cy = (short)(by + 22) };
    WORD *p = (WORD *)(dt + 1);
    *p++ = 0, *p++ = 0;               // no menu, the dialog class
    p = cpk_tpl_str(p, L"");          // the title: set at its start
    *p++ = (WORD)points;
    p = cpk_tpl_str(p, L"Segoe UI");  // ageofai_config.exe's (Windows' message font)
    p = cpk_tpl_item(p, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, 298, by, 50, 15, IDOK, 0x80, word_w("ok"));
    p = cpk_tpl_item(p, BS_PUSHBUTTON | WS_TABSTOP, 354, by, 50, 15, IDCANCEL, 0x80, word_w("cancel"));
    for (int i = 0; i < MD_ROWS; i++) {
        short y = (short)(8 + 17 * i);
        p = cpk_tpl_item(p, SS_LEFT | SS_CENTERIMAGE, 8, y, 62, 12, (WORD)(MD_LABEL + i), 0x82, md_lead(k_settings[MD_FIRST + i].key));
        p = cpk_tpl_item(p, CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP | WS_GROUP, 72, y, 120, 100,
                         (WORD)(MD_COMBO + i), 0x85, L"");
    }
    p = cpk_tpl_item(p, SS_LEFT | SS_CENTERIMAGE, 8, hint, 184, 12, MD_HINT, 0x82, md_lead("map_starts_keep"));
    p = cpk_tpl_item(p, SS_OWNERDRAW, 204, 8, 200, (short)(by - 15), MD_PREVIEW, 0x82, L"");
    p = cpk_tpl_item(p, BS_PUSHBUTTON | WS_TABSTOP, 8, by, 72, 15, MD_RESEED, 0x80, word_w("map_new_seed"));
    p = cpk_tpl_item(p, SS_LEFT | SS_CENTERIMAGE, 84, by, 108, 12, MD_SEED, 0x82, L"");
    p = cpk_tpl_item(p, BS_AUTOCHECKBOX | WS_TABSTOP, 204, (short)(by + 1), 90, 12, MD_KEEP, 0x80, word_w("map_keep_seed"));
    g_md.result = IDCANCEL;
    g_md.setup = setup; g_md.keep = 0;
    g_md.n = n; g_md.np = np; g_md.seed = *seed;
    for (int i = 0; i < MD_ROWS; i++) g_md.was[i] = *md_value(i);
    INT_PTR r = game_dialog_show(dt, md_proc, &ctx);
    md_done(NULL, 0);   // (closed some other way with a preview being made)
    g_md.dirty = 0;
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
        if (g_md.result == IDOK && at > 0 && at < (int)sizeof line - 40)
            snprintf(line + at, sizeof line - (size_t)at, ", seed %u%s", g_md.seed, !setup ? "" : g_md.keep ? " (to play)" : " (an example)");
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
    if (!md_open(map_scale_tiles(se_random_map_get_map_scale(a)), np > 0 ? np : 1, &seed, 0)) return 0;
    if (ui) ui_set_seed(ui, seed);
    return 1;
}

// the game setup screen's options button (mapgen_button.c): the dialog for its map, seed an example - "play this map"
// keeps it for the next single-player game's map (g_mg_keep)
internal void md_open_setup(int n, int np, uint32_t seed) {
    if (md_open(n, np, &seed, 1) && g_md.keep) { g_mg_keep.on = 1; g_mg_keep.seed = seed; }
    else if (g_md.result == IDOK) g_mg_keep.on = 0;
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
