// THE TERRAIN COLOUR DIALOG (`terrain_palette`, [editor]): a colour type of the map's own made or changed. The editor's
// colour list ends in an item of the mod's own, a strip of the hues round: selected (a click), the dialog opens for a
// new type, the list's selection back on the item before; a double click on one of the map's own types opens it for
// that type. The dialog: its kind (one colour, a ramp between two, each slot its own; a ramp's blend: sRGB as the
// game's own, linear light or OKLab - tp_ramp), its slots (1 .. 16; a ramp 2 at least), its colours as a strip - a click on a slot chooses it for the picker under the strip (a ramp: the end
// nearer the click, the slots between follow) -, the slots free for it. The picker (ui/color_picker.c) is part of the dialog, not a
// window of its own (the theme has one background at a time), and the strip follows it as the mouse moves; OK makes the type (terrain_palette.c
// tp_type_put), Delete takes it out. A modal Win32 dialog over the game's window, on the game's thread: the game
// waits meanwhile; the same program's window, so no app switch reaches the game (it would minimize itself). In the
// game's look (ui/theme.c, ageofai_config.exe's controls in the editor's style: the parchment of its pages under it,
// yellow text on its darker boxes, the game's radio buttons, dropdown and buttons; plain dark without the game's
// pictures), and at the scale the game's picture is shown at (td_open): at a lower resolution, enlarged to the window, it
// is as much larger as the game's own controls.
#define TD_FLAT 101
#define TD_RAMP 102
#define TD_EACH 103
#define TD_COUNT 104
#define TD_STRIP 105
#define TD_FREE 106
#define TD_DELETE 107
#define TD_HINT 108
#define TD_BLEND 109
#define TD_BLEND_LABEL 110
#define TD_SLOTS_LABEL 111
#define TD_POINTS 9       // its font's size at the game's picture 1:1 (the dialog's parts follow the font) ...
#define TD_PIXELS 500     // ... and about its height then, with the title bar
internal struct {
    int type;                          // the type changed, -1 a new one
    int mode, blend;
    unsigned n, avail;                 // its slots; the slots it may have (free ones and its own)
    uint8_t cell[TP_TYPE_SLOTS][3];    // the slots' colours as shown
    int result;                        // IDOK, IDCANCEL, TD_DELETE
    unsigned sel;                      // the slot the picker works on (g_cpk.c: its colour)
} g_td;
internal int g_td_prev = -1;           // the colour list's selection before the "new" item's

// a ramp's slots between its ends anew, a single colour's one
internal void td_derive(void) {
    if (g_td.mode == TP_FLAT) g_td.n = 1;
    if (g_td.mode == TP_RAMP && g_td.n < 2) g_td.n = 2;
    if (g_td.mode == TP_RAMP) tp_ramp(g_td.cell[0], g_td.cell[g_td.n - 1], g_td.n, g_td.blend, g_td.cell);
}

// a label's word, a space in: off its box's edge (the caller copies it before the next)
internal const wchar_t *td_lead(const char *key) {
    static wchar_t t[120];
    t[0] = L' ';
    lstrcpynW(t + 1, word_w(key), 119);
    return t;
}
internal void td_redraw(HWND d) {
    InvalidateRect(GetDlgItem(d, TD_STRIP), NULL, FALSE);
    cpk_redraw(d);
}

// the slot the picker works on
internal void td_choose(HWND d, unsigned k) {
    g_td.sel = k;
    cpk_begin(d, g_td.cell[k]);
    InvalidateRect(GetDlgItem(d, TD_STRIP), NULL, FALSE);
}

// the picker changed the chosen slot's colour (g_cpk.changed): a ramp's slots between its ends follow
internal void td_picked(HWND d) {
    td_derive();
    InvalidateRect(GetDlgItem(d, TD_STRIP), NULL, FALSE);
}

internal void td_update(HWND d) {
    td_derive();
    unsigned k = g_td.mode == TP_FLAT ? 0 : g_td.mode == TP_RAMP ? (g_td.sel * 2 < g_td.n ? 0 : g_td.n - 1) : g_td.sel < g_td.n ? g_td.sel : g_td.n - 1;
    if (k != g_td.sel) td_choose(d, k);
    CheckRadioButton(d, TD_FLAT, TD_EACH, TD_FLAT + g_td.mode);
    SendDlgItemMessageW(d, TD_COUNT, CB_SETCURSEL, g_td.n - 1, 0);
    EnableWindow(GetDlgItem(d, TD_COUNT), g_td.mode != TP_FLAT);
    SendDlgItemMessageW(d, TD_BLEND, CB_SETCURSEL, (WPARAM)g_td.blend, 0);
    EnableWindow(GetDlgItem(d, TD_BLEND), g_td.mode == TP_RAMP);
    wchar_t t[96];
    t[0] = L' ';
    if (g_td.n <= g_td.avail) word_fill_w(t + 1, 95, "slots_taken", g_td.n, g_td.avail);
    else word_fill_w(t + 1, 95, "slots_short", g_td.avail, 0);
    SetDlgItemTextW(d, TD_FREE, t);
    SetDlgItemTextW(d, TD_HINT, td_lead(g_td.mode == TP_RAMP ? "hint_ramp" : g_td.mode == TP_EACH ? "hint_each" : "hint_flat"));
    EnableWindow(GetDlgItem(d, IDOK), g_td.n <= g_td.avail);
    td_redraw(d);
}

// the count changed: new slots like the last one (a ramp keeps its last end)
internal void td_count(unsigned n) {
    unsigned had = g_td.n;
    uint8_t last[3];
    memcpy(last, g_td.cell[had - 1], 3);
    for (unsigned k = had; k < n; k++) memcpy(g_td.cell[k], last, 3);
    g_td.n = n;
    if (g_td.mode == TP_RAMP) {
        memcpy(g_td.cell[n - 1], last, 3);
        if (g_td.sel) g_td.sel = n - 1, g_cpk.c = g_td.cell[n - 1];   // the picker stays on the last end
    }
}

// the mode changed: a ramp from the first and last colour, one colour the first
internal void td_mode(int mode) {
    if (mode == g_td.mode) return;
    if (mode == TP_RAMP && g_td.n < 2) td_count(8);
    g_td.mode = mode;
}

internal void td_draw(const DRAWITEMSTRUCT *di) {
    RECT r = cpk_sunken(di);
    LONG w = r.right - r.left;
    int t = theme_px(1);
    HBRUSH black = (HBRUSH)GetStockObject(BLACK_BRUSH), white = (HBRUSH)GetStockObject(WHITE_BRUSH), mark = CreateSolidBrush(RGB(254, 247, 32));
    for (unsigned k = 0; k < g_td.n; k++) {
        RECT c = { r.left + (LONG)k * w / (LONG)g_td.n, r.top, r.left + (LONG)(k + 1) * w / (LONG)g_td.n, r.bottom };
        cpk_fill(di->hDC, &c, g_td.cell[k]);
        if (g_td.mode != TP_RAMP || k == 0 || k + 1 == g_td.n) {   // what a click chooses: framed, the chosen one in the look's yellow
            cpk_frame(di->hDC, &c, black, t);
            cpk_frame(di->hDC, &c, k == g_td.sel ? mark : white, k == g_td.sel ? 2 * t : t);
            if (k == g_td.sel) cpk_frame(di->hDC, &c, black, t);
        }
    }
    DeleteObject(mark);
}

internal void td_click(HWND d, POINT p) {
    RECT r;
    if (cpk_click(d, p)) return;
    if (cpk_hit(d, TD_STRIP, p, &r)) {   // a slot to work on (a ramp: the end nearer the click)
        LONG w = r.right - r.left;
        unsigned k = w > 0 && p.x > r.left ? (unsigned)((p.x - r.left) * (LONG)g_td.n / w) : 0;
        if (k >= g_td.n) k = g_td.n - 1;
        if (g_td.mode == TP_FLAT) k = 0;
        if (g_td.mode == TP_RAMP) k = k * 2 < g_td.n ? 0 : g_td.n - 1;
        td_choose(d, k);
    }
}

// the labels and radio buttons the dropdowns' height (a cell of the game's in its look, theme_control), on a
// dropdown's row those beside it, the others round their middle: their boxes and texts in line with the fields'
internal void td_rows(HWND d) {
    static const int rows[][2] = { { TD_FLAT, 0 }, { TD_RAMP, TD_BLEND }, { TD_EACH, 0 }, { TD_BLEND_LABEL, TD_BLEND },
                                   { TD_SLOTS_LABEL, TD_COUNT }, { TD_FREE, TD_COUNT }, { TD_HINT, 0 },
                                   { CPK_R, 0 }, { CPK_G, 0 }, { CPK_B, 0 }, { CPK_HEX, 0 }, { CPK_R_LABEL, CPK_R }, { CPK_G_LABEL, CPK_G },
                                   { CPK_B_LABEL, CPK_B }, { CPK_HEX_LABEL, CPK_HEX }, { CPK_RECENT_LABEL, 0 }, { CPK_RECENT, CPK_RECENT_LABEL } };
    RECT f;
    GetWindowRect(GetDlgItem(d, TD_BLEND), &f);
    int h = f.bottom - f.top;
    static const int fields[][2] = { { TD_BLEND, TD_RAMP }, { TD_COUNT, TD_FREE } };   // each dropdown centred on its row: taller than it at a large font
    for (unsigned k = 0; k < sizeof fields / sizeof *fields; k++) {
        RECT r;
        HWND c = GetDlgItem(d, fields[k][0]);
        GetWindowRect(c, &r);
        GetWindowRect(GetDlgItem(d, fields[k][1]), &f);
        MapWindowPoints(NULL, d, (POINT *)&r, 2);
        MapWindowPoints(NULL, d, (POINT *)&f, 2);
        SetWindowPos(c, NULL, r.left, (f.top + f.bottom - h) / 2, r.right - r.left, r.bottom - r.top, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    for (unsigned k = 0; k < sizeof rows / sizeof *rows; k++) {
        HWND c = GetDlgItem(d, rows[k][0]);
        RECT r;
        GetWindowRect(rows[k][1] ? GetDlgItem(d, rows[k][1]) : c, &f);
        GetWindowRect(c, &r);
        MapWindowPoints(NULL, d, (POINT *)&r, 2);
        MapWindowPoints(NULL, d, (POINT *)&f, 2);
        int top = rows[k][1] ? f.top : (f.top + f.bottom - h) / 2;
        SetWindowPos(c, NULL, r.left, top, r.right - r.left, h, SWP_NOZORDER | SWP_NOACTIVATE);
    }
    cpk_fit(d);   // the picker's square, bar and swatch from the first field's top to the last's bottom
}

internal INT_PTR CALLBACK td_proc(HWND d, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_INITDIALOG: {
        for (unsigned k = 1; k <= TP_TYPE_SLOTS; k++) {
            wchar_t t[8];
            wsprintfW(t, L"%u", k);
            SendDlgItemMessageW(d, TD_COUNT, CB_ADDSTRING, 0, (LPARAM)t);
        }
        static const char *const blends[] = { "blend_srgb", "blend_linear", "blend_oklab" };   // tp_blend's order
        for (unsigned k = 0; k < sizeof blends / sizeof *blends; k++) SendDlgItemMessageW(d, TD_BLEND, CB_ADDSTRING, 0, (LPARAM)word_w(blends[k]));
        if (g_td.type < 0) ShowWindow(GetDlgItem(d, TD_DELETE), SW_HIDE);
        SetWindowTextW(d, word_w(g_td.type < 0 ? "colour_new" : "colour_edit"));
        HWND o = GetWindow(d, GW_OWNER);   // over the game's window (headless: off the screen with it)
        RECT ro, rd;
        if (o && GetWindowRect(o, &ro) && GetWindowRect(d, &rd))
            SetWindowPos(d, HWND_TOP, (ro.left + ro.right - (rd.right - rd.left)) / 2, (ro.top + ro.bottom - (rd.bottom - rd.top)) / 2,
                         0, 0, SWP_NOSIZE);
        theme_window(d);
        theme_size(d);   // the painting under it (the labels on its darkened copy)
        static const int buttons[] = { IDOK, IDCANCEL, TD_DELETE, TD_FLAT, TD_RAMP, TD_EACH };   // not the strip: drawn here
        for (unsigned k = 0; k < sizeof buttons / sizeof *buttons; k++) theme_control(GetDlgItem(d, buttons[k]), WC_BUTTONA);
        theme_control(GetDlgItem(d, TD_COUNT), WC_COMBOBOXA);
        theme_control(GetDlgItem(d, TD_BLEND), WC_COMBOBOXA);
        for (int id = CPK_R; id <= CPK_HEX; id++) theme_control(GetDlgItem(d, id), WC_EDITA);
        td_rows(d);
        td_update(d);
        td_choose(d, 0);
        SetFocus(GetDlgItem(d, IDOK));   // not the first radio button: an automatic one given the focus is clicked too
        return FALSE;
    }
    case WM_ERASEBKGND:
        if (!theme_erase(d, (HDC)w)) break;
        SetWindowLongPtrW(d, DWLP_MSGRESULT, 1);
        return TRUE;
    case WM_CTLCOLORSTATIC: case WM_CTLCOLOREDIT: case WM_CTLCOLORLISTBOX: case WM_CTLCOLORBTN: case WM_CTLCOLORDLG: {
        LRESULT brush = theme_ctlcolor(msg, w, l);   // a dialog procedure answers these with the brush itself
        if (msg == WM_CTLCOLOREDIT && cpk_bad(GetDlgCtrlID((HWND)l))) SetTextColor((HDC)w, CPK_BAD_TEXT);
        return (INT_PTR)brush;
    }
    case WM_DRAWITEM:
        if (w == TD_STRIP) { td_draw((const DRAWITEMSTRUCT *)l); return TRUE; }
        if (cpk_draw_item((const DRAWITEMSTRUCT *)l)) return TRUE;
        if (theme_draw_item((const DRAWITEMSTRUCT *)l)) return TRUE;   // the dropdowns' lists
        break;
    case WM_LBUTTONDOWN:   // the drawn controls are statics: the mouse goes through them to here
        td_click(d, (POINT){ (short)LOWORD(l), (short)HIWORD(l) });
        return TRUE;
    case WM_MOUSEMOVE: case WM_LBUTTONUP: case WM_CAPTURECHANGED:
        cpk_mouse(d, msg, l);
        return TRUE;
    case WM_COMMAND:
        switch (LOWORD(w)) {
        case TD_FLAT: case TD_RAMP: case TD_EACH:
            if (HIWORD(w) == BN_CLICKED) td_mode(LOWORD(w) - TD_FLAT), td_update(d);
            return TRUE;
        case TD_COUNT:
            if (HIWORD(w) == CBN_SELCHANGE) {
                LRESULT i = SendDlgItemMessageW(d, TD_COUNT, CB_GETCURSEL, 0, 0);
                if (i >= 0) td_count((unsigned)i + 1), td_update(d);
            }
            return TRUE;
        case TD_BLEND:
            if (HIWORD(w) == CBN_SELCHANGE) {
                LRESULT i = SendDlgItemMessageW(d, TD_BLEND, CB_GETCURSEL, 0, 0);
                if (i >= TP_SRGB && i <= TP_OKLAB) g_td.blend = (int)i, td_update(d);
            }
            return TRUE;
        case CPK_R: case CPK_G: case CPK_B: case CPK_HEX:
            return cpk_command(d, LOWORD(w), HIWORD(w));
        case IDOK: case IDCANCEL: case TD_DELETE:
            if (LOWORD(w) == IDOK) cpk_remember();
            g_cpk.c = NULL;
            g_td.result = LOWORD(w);
            EndDialog(d, 0);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// the dialog for type i (-1: a new one, its colours from the list's item sel); after it the type made, changed or
// taken out, the list's selection on it
internal void td_open(int i, int sel) {
    void *mesh = tp_mesh();
    if (!mesh || !g_tp_panel) return;
    memset(&g_td, 0, sizeof g_td);
    memset(&g_cpk, 0, sizeof g_cpk);
    g_cpk.changed = td_picked;
    g_td.type = i;
    g_td.result = IDCANCEL;
    unsigned avail = 0;
    for (unsigned s = 0; s < TP_SLOTS; s++) avail += g_tp_free[s] && !g_tp_own[s];
    if (i >= 0) {
        const struct tp_type *t = &g_tp_type[i];
        g_td.mode = t->mode, g_td.blend = t->blend, g_td.n = t->n, g_td.avail = avail + t->n;
        for (unsigned k = 0; k < t->n; k++) memcpy(g_td.cell[k], g_tp_rgb[t->slot[k]], 3);
    } else {   // a ramp of 8 from the first and last colour of the item selected before
        g_td.mode = TP_RAMP, g_td.n = 8, g_td.avail = avail;
        uint8_t a[3] = { 96, 96, 96 }, b[3] = { 255, 255, 255 };
        ee_color_type *ct = sel >= 0 ? se_terrain_color_type_at(g_tp_panel, (unsigned)sel) : NULL;
        int own = sel >= (int)g_tp_list_base && (unsigned)(sel - (int)g_tp_list_base) < g_tp_types;
        unsigned first = own ? g_tp_type[sel - g_tp_list_base].slot[0] : ct ? color_type_get_first(ct) : TP_SLOTS;
        unsigned last = own ? g_tp_type[sel - g_tp_list_base].slot[g_tp_type[sel - g_tp_list_base].n - 1] : ct ? color_type_get_last(ct) : TP_SLOTS;
        for (int j = 0; j < 3 && first < TP_SLOTS && last < TP_SLOTS; j++)
            a[j] = (uint8_t)(mesh_color(mesh, first)[j] * 255.0f + 0.5f), b[j] = (uint8_t)(mesh_color(mesh, last)[j] * 255.0f + 0.5f);
        memcpy(g_td.cell[0], a, 3), memcpy(g_td.cell[7], b, 3);
    }
    td_derive();
    ee_window *gw = game_window();
    HWND owner = gw ? window_get_hwnd(gw) : NULL;
    // the scale the game's picture is shown at (windowed or borderless it is enlarged to the window, the bars left out),
    // as the font's size - the dialog units follow it, what is drawn in pixels g_theme_zoom; it must fit in the window
    int points = TD_POINTS;
    RECT cr;
    if (owner && GetClientRect(owner, &cr) && window_get_height(gw) > 0) {
        RECT v = view_rect(&cr, window_get_width(gw), window_get_height(gw));
        points = (TD_POINTS * (int)(v.bottom - v.top) + window_get_height(gw) / 2) / window_get_height(gw);
        if (points > TD_POINTS * cr.bottom / TD_PIXELS) points = TD_POINTS * cr.bottom / TD_PIXELS;
        if (points < 7) points = 7;
    }
    g_theme_zoom = points * 100 / TD_POINTS;
    static WORD tpl[1024];
    memset(tpl, 0, sizeof tpl);
    DLGTEMPLATE *dt = (DLGTEMPLATE *)tpl;
    *dt = (DLGTEMPLATE){ .style = DS_MODALFRAME | DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU, .cdit = 26, .cx = 260, .cy = 241 };
    WORD *p = (WORD *)(dt + 1);
    *p++ = 0, *p++ = 0;          // no menu, the dialog class
    p = cpk_tpl_str(p, L"");          // the title: set at its start
    *p++ = (WORD)points;
    p = cpk_tpl_str(p, L"Segoe UI");  // ageofai_config.exe's (Windows' message font)
    // the buttons first: the dialog gives its first tab stop the focus when it is activated, and an automatic radio
    // button given the focus is clicked (the kind was one colour whenever the dialog opened). A radio button as tall
    // as the game's picture of one (20 pixels)
    p = cpk_tpl_item(p, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, 146, 218, 50, 15, IDOK, 0x80, word_w("ok"));
    p = cpk_tpl_item(p, BS_PUSHBUTTON | WS_TABSTOP, 202, 218, 50, 15, IDCANCEL, 0x80, word_w("cancel"));
    p = cpk_tpl_item(p, BS_PUSHBUTTON | WS_TABSTOP, 8, 218, 50, 15, TD_DELETE, 0x80, word_w("delete"));
    p = cpk_tpl_item(p, BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 8, 6, 124, 12, TD_FLAT, 0x80, word_w("colour_flat"));
    p = cpk_tpl_item(p, BS_AUTORADIOBUTTON, 8, 20, 124, 12, TD_RAMP, 0x80, word_w("colour_ramp"));
    p = cpk_tpl_item(p, BS_AUTORADIOBUTTON, 8, 34, 124, 12, TD_EACH, 0x80, word_w("colour_each"));
    p = cpk_tpl_item(p, SS_LEFT | SS_CENTERIMAGE, 140, 20, 30, 12, TD_BLEND_LABEL, 0x82, td_lead("blend"));   // the ramp's, beside it; the labels' texts a space in: off their boxes' edge, as a radio button's
    p = cpk_tpl_item(p, CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_TABSTOP | WS_GROUP, 172, 20, 80, 80, TD_BLEND, 0x85, L"");
    p = cpk_tpl_item(p, SS_LEFT | SS_CENTERIMAGE, 8, 54, 30, 12, TD_SLOTS_LABEL, 0x82, td_lead("slots"));
    p = cpk_tpl_item(p, CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP | WS_GROUP, 40, 54, 36, 160,
                TD_COUNT, 0x85, L"");
    p = cpk_tpl_item(p, SS_LEFT | SS_CENTERIMAGE, 86, 54, 166, 12, TD_FREE, 0x82, L"");
    p = cpk_tpl_item(p, SS_OWNERDRAW, 8, 72, 244, 24, TD_STRIP, 0x82, L"");
    p = cpk_tpl_item(p, SS_LEFT | SS_CENTERIMAGE, 8, 101, 244, 12, TD_HINT, 0x82, L"");
    p = cpk_items(p, 8, 118, td_lead("recent"));   // the picker (td_rows: the square, bar and swatch as tall as the fields)
    RECT clip;
    int clipped = GetClipCursor(&clip);
    ClipCursor(NULL);
    int before = ShowCursor(TRUE) - 1, shown = before + 1;   // Windows' cursor over the dialog, its count as it was after
    while (shown < 0) shown = ShowCursor(TRUE);
    static int looked;
    if (!looked) {   // the game's pictures read at the first open (from the game's folder, else the original game's: AoC)
        looked = 1;
        char ini[MAX_PATH + 32];
        snprintf(ini, sizeof ini, "%sageofai\\ageofai.ini", g_game_dir);   // its eesettings.ini beside it: the original's folder
        theme_look(ini, theme_style_editor());
        theme_app_mode(1);
    }
    ULONG_PTR cookie;
    int ctx = theme_context_enter(&cookie);
    INT_PTR r = DialogBoxIndirectParamW(GetModuleHandleW(NULL), dt, owner, td_proc, 0);
    if (ctx) theme_context_leave(cookie);
    while (shown > before) shown = ShowCursor(FALSE);
    if (clipped) ClipCursor(&clip);
    if (owner) SetForegroundWindow(owner);
    if (g_log) log_locked("# terrain palette: the colour dialog for %s %d (%s, %s, %d points): %s%s\n", i < 0 ? "a new type, from item" : "type", i < 0 ? sel : i,
                          g_gl.on ? "the game's look" : "plain, no game pictures", ctx ? "common controls 6" : "the old controls", points,
                          r < 0 ? "FAILED to open" : g_td.result == IDOK ? "OK" : g_td.result == TD_DELETE ? "delete" : "cancel",
                          g_td.result != IDOK ? "" : g_td.mode == TP_FLAT ? ", one colour" : g_td.mode == TP_EACH ? ", each slot" :
                          (const char *[]){ ", a ramp (sRGB)", ", a ramp (linear light)", ", a ramp (OKLab)" }[g_td.blend]);
    ee_se_color_list *list = se_terrain_get_color_list(g_tp_panel);
    if (g_td.result == IDOK) {
        unsigned k = i < 0 ? g_tp_types : (unsigned)i;
        uint8_t rgb[TP_TYPE_SLOTS][3];
        memcpy(rgb, g_td.cell, sizeof rgb);
        if (g_td.mode == TP_RAMP) memcpy(rgb[1], g_td.cell[g_td.n - 1], 3);   // tp_type_put's ramp: its two ends
        if (tp_type_put(k, g_td.mode, g_td.blend, g_td.n, (const uint8_t (*)[3])rgb)) se_color_list_select(list, g_tp_list_base + k, 1, 1);
    } else if (g_td.result == TD_DELETE && i >= 0 && tp_type_delete((unsigned)i))
        se_color_list_select(list, sel >= 0 && (unsigned)sel < g_tp_list_base ? (unsigned)sel : 0, 1, 1);
}

// the colour list's selection: the last item (the mod's "new" one) opens the dialog for a new type
internal uint32_t __thiscall hook_td_change(eeuie_terrain_color_change *ev, ee_ui_form *form, uint32_t arg) {
    uint32_t r = eeuie_terrain_color_change_execute(ev, form, arg);
    ee_se_color_list *list = g_tp_panel ? se_terrain_get_color_list(g_tp_panel) : NULL;
    if (!list) return r;
    int sel = se_color_list_get_selected(list);
    if (sel != (int)(g_tp_list_base + g_tp_types)) { g_td_prev = sel; return r; }
    int prev = g_td_prev >= 0 && g_td_prev < sel ? g_td_prev : 0;
    se_color_list_select(list, (unsigned)prev, 1, 0);
    td_open(-1, prev);
    return r;
}

// a double click on one of the map's own types opens the dialog for it
internal int __thiscall hook_td_double(ee_se_color_list *list) {
    int r = se_color_list_mouse_l_button_double_pressed(list);
    int sel = se_color_list_get_selected(list);
    if (g_tp_panel && list == se_terrain_get_color_list(g_tp_panel) && sel >= (int)g_tp_list_base && sel < (int)(g_tp_list_base + g_tp_types))
        td_open(sel - (int)g_tp_list_base, sel);
    return r;
}

internal int terrain_dialog_install(void) {
    return patch_terrain_color_change(hook_td_change) + patch_se_color_list_double_click(hook_td_double);
}
