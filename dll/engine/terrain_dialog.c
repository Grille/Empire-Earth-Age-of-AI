// THE TERRAIN COLOUR DIALOG (`terrain_palette`, [editor]): a colour type of the map's own made or changed. The editor's
// colour list ends in an item of the mod's own, a strip of the hues round: selected (a click), the dialog opens for a
// new type, the list's selection back on the item before; a double click on one of the map's own types opens it for
// that type. The dialog: its kind (one colour, a ramp between two, each slot its own), its slots (1 .. 16; a ramp 2
// at least), its colours as a strip - a click on a slot picks its colour with Windows' colour dialog (a ramp: the end
// nearer the click, the slots between follow) -, the slots free for it; OK makes the type (terrain_palette.c
// tp_type_put), Delete takes it out. A modal Win32 dialog over the game's window, on the game's thread: the game
// waits meanwhile; the same program's window, so no app switch reaches the game (it would minimize itself).
#define TD_FLAT 101
#define TD_RAMP 102
#define TD_EACH 103
#define TD_COUNT 104
#define TD_STRIP 105
#define TD_FREE 106
#define TD_DELETE 107
#define TD_HINT 108
internal struct {
    int type;                          // the type changed, -1 a new one
    int mode;
    unsigned n, avail;                 // its slots; the slots it may have (free ones and its own)
    uint8_t cell[TP_TYPE_SLOTS][3];    // the slots' colours as shown
    int result;                        // IDOK, IDCANCEL, TD_DELETE
} g_td;
internal COLORREF g_td_custom[16];    // Windows' colour dialog's custom colours, kept between its opens
internal BOOL (WINAPI *g_td_choose)(CHOOSECOLORW *);
internal int g_td_prev = -1;           // the colour list's selection before the "new" item's

// a ramp's slots between its ends anew, a single colour's one
internal void td_derive(void) {
    if (g_td.mode == TP_FLAT) g_td.n = 1;
    if (g_td.mode == TP_RAMP && g_td.n < 2) g_td.n = 2;
    if (g_td.mode != TP_RAMP) return;
    unsigned n = g_td.n;
    for (unsigned k = 1; k + 1 < n; k++)
        for (int j = 0; j < 3; j++)
            g_td.cell[k][j] = (uint8_t)((g_td.cell[0][j] * (int)(n - 1 - k) + g_td.cell[n - 1][j] * (int)k + (int)(n - 1) / 2) / (int)(n - 1));
}

internal void td_update(HWND d) {
    td_derive();
    CheckRadioButton(d, TD_FLAT, TD_EACH, TD_FLAT + g_td.mode);
    SendDlgItemMessageW(d, TD_COUNT, CB_SETCURSEL, g_td.n - 1, 0);
    EnableWindow(GetDlgItem(d, TD_COUNT), g_td.mode != TP_FLAT);
    wchar_t t[96];
    if (g_td.n <= g_td.avail) wsprintfW(t, L"%u of %u free slots taken", g_td.n, g_td.avail);
    else wsprintfW(t, L"only %u slots free", g_td.avail);
    SetDlgItemTextW(d, TD_FREE, t);
    SetDlgItemTextW(d, TD_HINT, g_td.mode == TP_RAMP ? L"Click near an end of the strip to pick that end's colour." :
                                g_td.mode == TP_EACH ? L"Click a slot to pick its colour." : L"Click the strip to pick the colour.");
    EnableWindow(GetDlgItem(d, IDOK), g_td.n <= g_td.avail);
    InvalidateRect(GetDlgItem(d, TD_STRIP), NULL, TRUE);
}

// the count changed: new slots like the last one (a ramp keeps its last end)
internal void td_count(unsigned n) {
    unsigned had = g_td.n;
    uint8_t last[3];
    memcpy(last, g_td.cell[had - 1], 3);
    for (unsigned k = had; k < n; k++) memcpy(g_td.cell[k], last, 3);
    g_td.n = n;
    if (g_td.mode == TP_RAMP) memcpy(g_td.cell[n - 1], last, 3);
}

// the mode changed: a ramp from the first and last colour, one colour the first
internal void td_mode(int mode) {
    if (mode == g_td.mode) return;
    if (mode == TP_RAMP && g_td.n < 2) td_count(8);
    g_td.mode = mode;
}

internal void td_pick(HWND d) {
    HWND s = GetDlgItem(d, TD_STRIP);
    POINT p;
    RECT r;
    GetCursorPos(&p);
    ScreenToClient(s, &p);
    GetClientRect(s, &r);
    unsigned k = r.right > 0 && p.x > 0 ? (unsigned)(p.x * (LONG)g_td.n / r.right) : 0;
    if (k >= g_td.n) k = g_td.n - 1;
    if (g_td.mode == TP_FLAT) k = 0;
    if (g_td.mode == TP_RAMP) k = k * 2 < g_td.n ? 0 : g_td.n - 1;
    if (!g_td_choose) {
        HMODULE m = LoadLibraryA("comdlg32.dll");
        g_td_choose = m ? (BOOL (WINAPI *)(CHOOSECOLORW *))(void *)GetProcAddress(m, "ChooseColorW") : NULL;
        if (!g_td_choose) return;
    }
    uint8_t *c = g_td.cell[k];
    CHOOSECOLORW cc = { .lStructSize = sizeof cc, .hwndOwner = d, .rgbResult = RGB(c[0], c[1], c[2]),
                        .lpCustColors = g_td_custom, .Flags = CC_FULLOPEN | CC_RGBINIT };
    if (!g_td_choose(&cc)) return;
    c[0] = GetRValue(cc.rgbResult), c[1] = GetGValue(cc.rgbResult), c[2] = GetBValue(cc.rgbResult);
    td_update(d);
}

internal void td_draw(const DRAWITEMSTRUCT *di) {
    RECT r = di->rcItem;
    LONG w = r.right - r.left;
    for (unsigned k = 0; k < g_td.n; k++) {
        RECT c = { r.left + (LONG)k * w / (LONG)g_td.n, r.top, r.left + (LONG)(k + 1) * w / (LONG)g_td.n, r.bottom };
        HBRUSH b = CreateSolidBrush(RGB(g_td.cell[k][0], g_td.cell[k][1], g_td.cell[k][2]));
        FillRect(di->hDC, &c, b);
        DeleteObject(b);
        if (g_td.mode != TP_RAMP || k == 0 || k + 1 == g_td.n) {   // what a click picks: framed
            FrameRect(di->hDC, &c, (HBRUSH)GetStockObject(BLACK_BRUSH));
            InflateRect(&c, -1, -1);
            FrameRect(di->hDC, &c, (HBRUSH)GetStockObject(WHITE_BRUSH));
        }
    }
}

internal INT_PTR CALLBACK td_proc(HWND d, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_INITDIALOG: {
        for (unsigned k = 1; k <= TP_TYPE_SLOTS; k++) {
            wchar_t t[8];
            wsprintfW(t, L"%u", k);
            SendDlgItemMessageW(d, TD_COUNT, CB_ADDSTRING, 0, (LPARAM)t);
        }
        if (g_td.type < 0) ShowWindow(GetDlgItem(d, TD_DELETE), SW_HIDE);
        SetWindowTextW(d, g_td.type < 0 ? L"New terrain colour" : L"Terrain colour");
        HWND o = GetWindow(d, GW_OWNER);   // over the game's window (headless: off the screen with it)
        RECT ro, rd;
        if (o && GetWindowRect(o, &ro) && GetWindowRect(d, &rd))
            SetWindowPos(d, HWND_TOP, (ro.left + ro.right - (rd.right - rd.left)) / 2, (ro.top + ro.bottom - (rd.bottom - rd.top)) / 2,
                         0, 0, SWP_NOSIZE);
        td_update(d);
        SetFocus(GetDlgItem(d, IDOK));   // not the first radio button: an automatic one given the focus is clicked too
        return FALSE;
    }
    case WM_DRAWITEM:
        if (w == TD_STRIP) { td_draw((const DRAWITEMSTRUCT *)l); return TRUE; }
        break;
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
        case TD_STRIP:
            if (HIWORD(w) == BN_CLICKED) td_pick(d);
            return TRUE;
        case IDOK: case IDCANCEL: case TD_DELETE:
            g_td.result = LOWORD(w);
            EndDialog(d, 0);
            return TRUE;
        }
        break;
    }
    return FALSE;
}

// an in-memory dialog template: the dialog, then its items (dialog units)
internal WORD *td_align(WORD *p) { return (WORD *)(((uintptr_t)p + 3) & ~(uintptr_t)3); }
internal WORD *td_str(WORD *p, const wchar_t *s) {
    do *p++ = *s; while (*s++);
    return p;
}
internal WORD *td_item(WORD *p, DWORD style, short x, short y, short cx, short cy, WORD id, WORD cls, const wchar_t *text) {
    p = td_align(p);
    DLGITEMTEMPLATE *it = (DLGITEMTEMPLATE *)p;
    *it = (DLGITEMTEMPLATE){ .style = style | WS_CHILD | WS_VISIBLE, .x = x, .y = y, .cx = cx, .cy = cy, .id = id };
    p = (WORD *)(it + 1);
    *p++ = 0xFFFF, *p++ = cls;   // a class by its atom: 0x80 button, 0x82 static, 0x85 combo box
    p = td_str(p, text);
    *p++ = 0;                    // no creation data
    return p;
}

// the dialog for type i (-1: a new one, its colours from the list's item sel); after it the type made, changed or
// taken out, the list's selection on it
internal void td_open(int i, int sel) {
    void *mesh = tp_mesh();
    if (!mesh || !g_tp_panel) return;
    memset(&g_td, 0, sizeof g_td);
    g_td.type = i;
    g_td.result = IDCANCEL;
    unsigned avail = 0;
    for (unsigned s = 0; s < TP_SLOTS; s++) avail += g_tp_free[s] && !g_tp_own[s];
    if (i >= 0) {
        const struct tp_type *t = &g_tp_type[i];
        g_td.mode = t->mode, g_td.n = t->n, g_td.avail = avail + t->n;
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
    for (unsigned k = 0; k < 16 && k < g_td.n; k++) g_td_custom[k] = RGB(g_td.cell[k][0], g_td.cell[k][1], g_td.cell[k][2]);
    static WORD tpl[1024];
    memset(tpl, 0, sizeof tpl);
    DLGTEMPLATE *dt = (DLGTEMPLATE *)tpl;
    *dt = (DLGTEMPLATE){ .style = DS_MODALFRAME | DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU, .cdit = 13, .cx = 260, .cy = 142 };
    WORD *p = (WORD *)(dt + 1);
    *p++ = 0, *p++ = 0;          // no menu, the dialog class
    p = td_str(p, L"");          // the title: set at its start
    *p++ = 9;
    p = td_str(p, L"MS Shell Dlg");
    // the buttons first: the dialog gives its first tab stop the focus when it is activated, and an automatic radio
    // button given the focus is clicked (the kind was one colour whenever the dialog opened)
    p = td_item(p, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, 146, 123, 50, 14, IDOK, 0x80, L"OK");
    p = td_item(p, BS_PUSHBUTTON | WS_TABSTOP, 202, 123, 50, 14, IDCANCEL, 0x80, L"Cancel");
    p = td_item(p, BS_PUSHBUTTON | WS_TABSTOP, 8, 123, 50, 14, TD_DELETE, 0x80, L"Delete");
    p = td_item(p, BS_GROUPBOX | WS_GROUP, 6, 4, 248, 52, 0xFFFF, 0x80, L"Kind");
    p = td_item(p, BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 14, 16, 230, 10, TD_FLAT, 0x80, L"One colour");
    p = td_item(p, BS_AUTORADIOBUTTON, 14, 28, 230, 10, TD_RAMP, 0x80, L"A ramp between two colours");
    p = td_item(p, BS_AUTORADIOBUTTON, 14, 40, 230, 10, TD_EACH, 0x80, L"Each slot a colour of its own");
    p = td_item(p, SS_LEFT, 8, 64, 30, 10, 0xFFFF, 0x82, L"Slots:");
    p = td_item(p, CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP | WS_GROUP, 40, 62, 36, 160, TD_COUNT, 0x85, L"");
    p = td_item(p, SS_LEFT, 86, 64, 168, 10, TD_FREE, 0x82, L"");
    p = td_item(p, BS_OWNERDRAW | WS_TABSTOP, 8, 80, 244, 22, TD_STRIP, 0x80, L"");
    p = td_item(p, SS_LEFT, 8, 105, 244, 10, TD_HINT, 0x82, L"");
    p = td_item(p, SS_ETCHEDHORZ, 6, 118, 248, 1, 0xFFFF, 0x82, L"");
    ee_window *gw = game_window();
    HWND owner = gw ? window_get_hwnd(gw) : NULL;
    RECT clip;
    int clipped = GetClipCursor(&clip);
    ClipCursor(NULL);
    int before = ShowCursor(TRUE) - 1, shown = before + 1;   // Windows' cursor over the dialog, its count as it was after
    while (shown < 0) shown = ShowCursor(TRUE);
    INT_PTR r = DialogBoxIndirectParamW(GetModuleHandleW(NULL), dt, owner, td_proc, 0);
    while (shown > before) shown = ShowCursor(FALSE);
    if (clipped) ClipCursor(&clip);
    if (owner) SetForegroundWindow(owner);
    if (g_log) log_locked("# terrain palette: the colour dialog for %s %d: %s%s\n", i < 0 ? "a new type, from item" : "type", i < 0 ? sel : i,
                          r < 0 ? "FAILED to open" : g_td.result == IDOK ? "OK" : g_td.result == TD_DELETE ? "delete" : "cancel",
                          g_td.result == IDOK ? (g_td.mode == TP_FLAT ? ", one colour" : g_td.mode == TP_RAMP ? ", a ramp" : ", each slot") : "");
    ee_se_color_list *list = se_terrain_get_color_list(g_tp_panel);
    if (g_td.result == IDOK) {
        unsigned k = i < 0 ? g_tp_types : (unsigned)i;
        uint8_t rgb[TP_TYPE_SLOTS][3];
        memcpy(rgb, g_td.cell, sizeof rgb);
        if (g_td.mode == TP_RAMP) memcpy(rgb[1], g_td.cell[g_td.n - 1], 3);   // tp_type_put's ramp: its two ends
        if (tp_type_put(k, g_td.mode, g_td.n, (const uint8_t (*)[3])rgb)) se_color_list_select(list, g_tp_list_base + k, 1, 1);
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
