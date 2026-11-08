// THE TERRAIN COLOUR DIALOG (`terrain_palette`, [editor]): a colour type of the map's own made or changed. The editor's
// colour list ends in an item of the mod's own, a strip of the hues round: selected (a click), the dialog opens for a
// new type, the list's selection back on the item before; a double click on one of the map's own types opens it for
// that type. The dialog: its kind (one colour, a ramp between two, each slot its own; a ramp's blend: sRGB as the
// game's own, linear light or OKLab - tp_ramp), its slots (1 .. 16; a ramp 2 at least), its colours as a strip - a click on a slot chooses it for the picker under the strip (a ramp: the end
// nearer the click, the slots between follow) -, the slots free for it. The picker is part of the dialog, not a window of its own (the
// theme has one background at a time), and the strip follows it as the mouse moves: a square of saturation and value, a bar of
// the hues, the colour as numbers, what it was (a click there brings it back), the colours used last, kept while the game runs; OK makes the type (terrain_palette.c
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
#define TD_SQUARE 112
#define TD_HUE 113
#define TD_SWATCH 114
#define TD_RECENT 115
#define TD_RECENT_LABEL 116
#define TD_R 117          // TD_G, TD_B after it, then TD_HEX; their labels in that order
#define TD_G 118
#define TD_B 119
#define TD_HEX 120
#define TD_R_LABEL 121
#define TD_G_LABEL 122
#define TD_B_LABEL 123
#define TD_HEX_LABEL 124
#define TD_RECENTS 16
#define TD_POINTS 9       // its font's size at the game's picture 1:1 (the dialog's parts follow the font) ...
#define TD_PIXELS 500     // ... and about its height then, with the title bar
internal struct {
    int type;                          // the type changed, -1 a new one
    int mode, blend;
    unsigned n, avail;                 // its slots; the slots it may have (free ones and its own)
    uint8_t cell[TP_TYPE_SLOTS][3];    // the slots' colours as shown
    int result;                        // IDOK, IDCANCEL, TD_DELETE
    unsigned sel;                      // the slot the picker works on
    float hue, sat, val;               // its place in the picker (0 .. 1): the colour's, with what a grey or black doesn't tell
    uint8_t was[3];                    // its colour when it was chosen
    int drag;                          // the mouse held on: TD_SQUARE, TD_HUE
    int edited, filling;               // changed since it was chosen; the fields being set from here (their EN_CHANGE ignored)
} g_td;
internal uint8_t g_td_recent[TD_RECENTS][3];   // the colours used last, the newest first
internal unsigned g_td_recents;
internal int g_td_prev = -1;           // the colour list's selection before the "new" item's

// a ramp's slots between its ends anew, a single colour's one
internal void td_derive(void) {
    if (g_td.mode == TP_FLAT) g_td.n = 1;
    if (g_td.mode == TP_RAMP && g_td.n < 2) g_td.n = 2;
    if (g_td.mode == TP_RAMP) tp_ramp(g_td.cell[0], g_td.cell[g_td.n - 1], g_td.n, g_td.blend, g_td.cell);
}

// hue 0 .. 1 round, saturation and value 0 .. 1
internal void td_hsv_rgb(float h, float s, float v, uint8_t out[3]) {
    float f = h * 6.0f;
    int i = (int)f;
    f -= (float)i;
    float p = v * (1 - s), q = v * (1 - s * f), t = v * (1 - s * (1 - f));
    const float c[6][3] = { { v, t, p }, { q, v, p }, { p, v, t }, { p, q, v }, { t, p, v }, { v, p, q } };
    for (int j = 0; j < 3; j++) out[j] = (uint8_t)(c[i % 6][j] * 255.0f + 0.5f);
}

// the picker's place from a colour: a grey keeps the hue it had, black its saturation too (the marks stay where they were)
internal void td_rgb_hsv(const uint8_t c[3]) {
    float r = c[0] / 255.0f, g = c[1] / 255.0f, b = c[2] / 255.0f;
    float top = r > g ? (r > b ? r : b) : (g > b ? g : b), low = r < g ? (r < b ? r : b) : (g < b ? g : b), d = top - low;
    g_td.val = top;
    if (top > 0) g_td.sat = d / top;
    if (d <= 0) return;
    float h = top == r ? (g - b) / d : top == g ? 2 + (b - r) / d : 4 + (r - g) / d;
    g_td.hue = (h < 0 ? h + 6 : h) / 6;
}

// the slot's colour into the recent colours' front, if it was changed since it was chosen
internal void td_remember(void) {
    if (!g_td.edited) return;
    g_td.edited = 0;
    uint8_t c[3];
    memcpy(c, g_td.cell[g_td.sel], 3);
    unsigned at = 0;
    while (at < g_td_recents && memcmp(g_td_recent[at], c, 3)) at++;
    if (at == g_td_recents && g_td_recents < TD_RECENTS) g_td_recents++;
    if (at == g_td_recents) at--;
    memmove(g_td_recent[1], g_td_recent[0], at * 3);
    memcpy(g_td_recent[0], c, 3);
}

// the fields from the chosen slot's colour, but the one being typed in
internal void td_fields(HWND d, int skip) {
    const uint8_t *c = g_td.cell[g_td.sel];
    wchar_t t[8];
    g_td.filling = 1;
    for (int j = 0; j < 3; j++)
        if (TD_R + j != skip) SetDlgItemInt(d, TD_R + j, c[j], FALSE);
    if (skip != TD_HEX) {
        wsprintfW(t, L"%02X%02X%02X", c[0], c[1], c[2]);
        SetDlgItemTextW(d, TD_HEX, t);
    }
    g_td.filling = 0;
}

internal void td_redraw(HWND d) {
    static const int drawn[] = { TD_STRIP, TD_SQUARE, TD_HUE, TD_SWATCH, TD_RECENT };
    for (unsigned k = 0; k < sizeof drawn / sizeof *drawn; k++) InvalidateRect(GetDlgItem(d, drawn[k]), NULL, FALSE);
}

// the slot the picker works on
internal void td_choose(HWND d, unsigned k) {
    td_remember();
    g_td.sel = k;
    memcpy(g_td.was, g_td.cell[k], 3);
    td_rgb_hsv(g_td.cell[k]);
    td_fields(d, 0);
    td_redraw(d);
}

// the chosen slot's colour changed (skip: the field it was typed in)
internal void td_changed(HWND d, int skip) {
    g_td.edited = 1;
    td_derive();
    td_fields(d, skip);
    td_redraw(d);
}

// a colour for the chosen slot (the "was" half, a recent one, the fields)
internal void td_set(HWND d, const uint8_t c[3], int skip) {
    memmove(g_td.cell[g_td.sel], c, 3);
    td_rgb_hsv(c);
    td_changed(d, skip);
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
    if (g_td.n <= g_td.avail) wsprintfW(t, L" %u of %u free slots taken", g_td.n, g_td.avail);
    else wsprintfW(t, L" only %u slots free", g_td.avail);
    SetDlgItemTextW(d, TD_FREE, t);
    SetDlgItemTextW(d, TD_HINT, g_td.mode == TP_RAMP ? L" Click near an end of the strip to choose which end to change." :
                                g_td.mode == TP_EACH ? L" Click a slot to choose which to change." : L" Its colour: choose it below.");
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
        if (g_td.sel) g_td.sel = n - 1;   // the picker stays on the last end
    }
}

// the mode changed: a ramp from the first and last colour, one colour the first
internal void td_mode(int mode) {
    if (mode == g_td.mode) return;
    if (mode == TP_RAMP && g_td.n < 2) td_count(8);
    g_td.mode = mode;
}

internal void td_fill(HDC dc, const RECT *r, const uint8_t c[3]) {
    HBRUSH b = CreateSolidBrush(RGB(c[0], c[1], c[2]));
    FillRect(dc, r, b);
    DeleteObject(b);
}

// a drawn control's inside: its bevel round it, as a field's (none in the plain look, its place kept: the mouse maps to it)
internal RECT td_sunken(const DRAWITEMSTRUCT *di) {
    RECT r = di->rcItem;
    if (g_gl.on) theme_bevel(di->hDC, &r, 1);
    InflateRect(&r, -theme_px(2), -theme_px(2));
    return r;
}

// a frame t pixels thick just inside c, c then inside it
internal void td_frame(HDC dc, RECT *c, HBRUSH b, int t) {
    for (int i = 0; i < t; i++) {
        FrameRect(dc, c, b);
        InflateRect(c, -1, -1);
    }
}

internal void td_draw(const DRAWITEMSTRUCT *di) {
    RECT r = td_sunken(di);
    LONG w = r.right - r.left;
    int t = theme_px(1);
    HBRUSH black = (HBRUSH)GetStockObject(BLACK_BRUSH), white = (HBRUSH)GetStockObject(WHITE_BRUSH), mark = CreateSolidBrush(RGB(254, 247, 32));
    for (unsigned k = 0; k < g_td.n; k++) {
        RECT c = { r.left + (LONG)k * w / (LONG)g_td.n, r.top, r.left + (LONG)(k + 1) * w / (LONG)g_td.n, r.bottom };
        td_fill(di->hDC, &c, g_td.cell[k]);
        if (g_td.mode != TP_RAMP || k == 0 || k + 1 == g_td.n) {   // what a click chooses: framed, the chosen one in the look's yellow
            td_frame(di->hDC, &c, black, t);
            td_frame(di->hDC, &c, k == g_td.sel ? mark : white, k == g_td.sel ? 2 * t : t);
            if (k == g_td.sel) td_frame(di->hDC, &c, black, t);
        }
    }
    DeleteObject(mark);
}

// the picker's square (across saturation, up value, of the hue chosen) or its bar of the hues, and the mark on it
internal void td_gradient(const DRAWITEMSTRUCT *di, int bar) {
    RECT r = td_sunken(di);
    int w = r.right - r.left, h = r.bottom - r.top;
    if (w < 2 || h < 2) return;
    BITMAPINFO bi = { .bmiHeader = { .biSize = sizeof bi.bmiHeader, .biWidth = w, .biHeight = -h, .biPlanes = 1, .biBitCount = 32 } };
    uint8_t *bits;
    HBITMAP bm = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
    if (!bm) return;
    uint8_t *o = bits, c[3];
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++, o += 4) {
            if (bar) td_hsv_rgb((float)y / (float)(h - 1), 1, 1, c);
            else td_hsv_rgb(g_td.hue, (float)x / (float)(w - 1), 1 - (float)y / (float)(h - 1), c);
            o[0] = c[2], o[1] = c[1], o[2] = c[0];
        }
    GdiFlush();
    HDC mem = CreateCompatibleDC(di->hDC);
    HGDIOBJ old = SelectObject(mem, bm);
    BitBlt(di->hDC, r.left, r.top, w, h, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteDC(mem);
    DeleteObject(bm);
    int saved = SaveDC(di->hDC);
    IntersectClipRect(di->hDC, r.left, r.top, r.right, r.bottom);
    int mx = r.left + (int)(g_td.sat * (float)(w - 1) + 0.5f), my = r.top + (int)((1 - g_td.val) * (float)(h - 1) + 0.5f);
    int a = theme_px(5), t = theme_px(1);
    RECT m = { mx - a, my - a, mx + a + 1, my + a + 1 };
    if (bar) {
        my = r.top + (int)(g_td.hue * (float)(h - 1) + 0.5f);
        a = theme_px(3);
        m = (RECT){ r.left, my - a, r.right, my + a + 1 };
    }
    td_frame(di->hDC, &m, (HBRUSH)GetStockObject(BLACK_BRUSH), t);
    td_frame(di->hDC, &m, (HBRUSH)GetStockObject(WHITE_BRUSH), t);
    RestoreDC(di->hDC, saved);
}

// the chosen slot's colour over what it was when it was chosen (a click on that half brings it back)
internal void td_swatch(const DRAWITEMSTRUCT *di) {
    RECT r = td_sunken(di), a = r, b = r;
    a.bottom = b.top = (r.top + r.bottom) / 2;
    td_fill(di->hDC, &a, g_td.cell[g_td.sel]);
    td_fill(di->hDC, &b, g_td.was);
}

internal void td_recents(const DRAWITEMSTRUCT *di) {
    RECT r = td_sunken(di);
    LONG w = r.right - r.left;
    for (unsigned k = 0; k < TD_RECENTS; k++) {
        RECT c = { r.left + (LONG)k * w / TD_RECENTS, r.top, r.left + (LONG)(k + 1) * w / TD_RECENTS, r.bottom };
        if (k < g_td_recents) td_fill(di->hDC, &c, g_td_recent[k]);
        else FillRect(di->hDC, &c, g_pal.b_field);
        td_frame(di->hDC, &c, (HBRUSH)GetStockObject(BLACK_BRUSH), theme_px(1));
    }
}

// a control's rectangles in the dialog: whole (returned: the point on it), inside its bevel
internal int td_hit(HWND d, int id, POINT p, RECT *in) {
    GetWindowRect(GetDlgItem(d, id), in);
    MapWindowPoints(NULL, d, (POINT *)in, 2);
    int on = PtInRect(in, p);
    InflateRect(in, -theme_px(2), -theme_px(2));
    return on;
}

// the mouse on the square or the bar: its place is the colour, kept on it outside
internal void td_drag(HWND d, POINT p) {
    RECT r;
    td_hit(d, g_td.drag, p, &r);
    float x = (float)(p.x - r.left) / (float)(r.right - r.left - 1), y = (float)(p.y - r.top) / (float)(r.bottom - r.top - 1);
    x = x < 0 ? 0 : x > 1 ? 1 : x, y = y < 0 ? 0 : y > 1 ? 1 : y;
    if (g_td.drag == TD_HUE) g_td.hue = y;
    else g_td.sat = x, g_td.val = 1 - y;
    td_hsv_rgb(g_td.hue, g_td.sat, g_td.val, g_td.cell[g_td.sel]);
    td_changed(d, 0);
}

internal void td_click(HWND d, POINT p) {
    RECT r;
    if (td_hit(d, TD_SQUARE, p, &r) || td_hit(d, TD_HUE, p, &r)) {
        g_td.drag = td_hit(d, TD_HUE, p, &r) ? TD_HUE : TD_SQUARE;
        SetCapture(d);
        td_drag(d, p);
    } else if (td_hit(d, TD_STRIP, p, &r)) {   // a slot to work on (a ramp: the end nearer the click)
        LONG w = r.right - r.left;
        unsigned k = w > 0 && p.x > r.left ? (unsigned)((p.x - r.left) * (LONG)g_td.n / w) : 0;
        if (k >= g_td.n) k = g_td.n - 1;
        if (g_td.mode == TP_FLAT) k = 0;
        if (g_td.mode == TP_RAMP) k = k * 2 < g_td.n ? 0 : g_td.n - 1;
        td_choose(d, k);
    } else if (td_hit(d, TD_SWATCH, p, &r)) {
        if (p.y >= (r.top + r.bottom) / 2) td_set(d, g_td.was, 0);
    } else if (td_hit(d, TD_RECENT, p, &r) && r.right > r.left && p.x >= r.left) {
        unsigned k = (unsigned)((p.x - r.left) * TD_RECENTS / (r.right - r.left));
        if (k < g_td_recents) {
            uint8_t c[3];
            memcpy(c, g_td_recent[k], 3);
            td_set(d, c, 0);
        }
    }
}

// a field typed in: its value the slot's colour once it is one (a part typed is left as it is)
internal void td_typed(HWND d, int id) {
    uint8_t c[3];
    memcpy(c, g_td.cell[g_td.sel], 3);
    if (id == TD_HEX) {
        wchar_t t[8];
        if (GetDlgItemTextW(d, id, t, 8) != 6) return;
        unsigned v = 0;
        for (int i = 0; i < 6; i++) {
            unsigned x = t[i] >= L'0' && t[i] <= L'9' ? t[i] - L'0' : t[i] >= L'A' && t[i] <= L'F' ? t[i] - L'A' + 10u : 16;
            if (x > 15) return;
            v = v * 16 + x;
        }
        c[0] = (uint8_t)(v >> 16), c[1] = (uint8_t)(v >> 8), c[2] = (uint8_t)v;
    } else {
        BOOL ok;
        UINT v = GetDlgItemInt(d, id, &ok, FALSE);
        if (!ok) return;
        if (v > 255) {
            v = 255;
            g_td.filling = 1;
            SetDlgItemInt(d, id, v, FALSE);
            SendDlgItemMessageW(d, id, EM_SETSEL, 3, 3);
            g_td.filling = 0;
        }
        c[id - TD_R] = (uint8_t)v;
    }
    td_set(d, c, id);
}

// the labels and radio buttons the dropdowns' height (a cell of the game's in its look, theme_control), on a
// dropdown's row those beside it, the others round their middle: their boxes and texts in line with the fields'
internal void td_rows(HWND d) {
    static const int rows[][2] = { { TD_FLAT, 0 }, { TD_RAMP, TD_BLEND }, { TD_EACH, 0 }, { TD_BLEND_LABEL, TD_BLEND },
                                   { TD_SLOTS_LABEL, TD_COUNT }, { TD_FREE, TD_COUNT }, { TD_HINT, 0 },
                                   { TD_R, 0 }, { TD_G, 0 }, { TD_B, 0 }, { TD_HEX, 0 }, { TD_R_LABEL, TD_R }, { TD_G_LABEL, TD_G },
                                   { TD_B_LABEL, TD_B }, { TD_HEX_LABEL, TD_HEX }, { TD_RECENT_LABEL, 0 }, { TD_RECENT, TD_RECENT_LABEL } };
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
    RECT a, b;   // the picker's square, bar and swatch from the first field's top to the last's bottom
    GetWindowRect(GetDlgItem(d, TD_R), &a);
    GetWindowRect(GetDlgItem(d, TD_HEX), &b);
    MapWindowPoints(NULL, d, (POINT *)&a, 2);
    MapWindowPoints(NULL, d, (POINT *)&b, 2);
    static const int tall[] = { TD_SQUARE, TD_HUE, TD_SWATCH };
    for (unsigned k = 0; k < sizeof tall / sizeof *tall; k++) {
        HWND c = GetDlgItem(d, tall[k]);
        RECT r;
        GetWindowRect(c, &r);
        MapWindowPoints(NULL, d, (POINT *)&r, 2);
        SetWindowPos(c, NULL, r.left, a.top, r.right - r.left, b.bottom - a.top, SWP_NOZORDER | SWP_NOACTIVATE);
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
        static const wchar_t *const blends[] = { L"sRGB (the game's)", L"linear light", L"OKLab (even)" };   // tp_blend's order
        for (unsigned k = 0; k < sizeof blends / sizeof *blends; k++) SendDlgItemMessageW(d, TD_BLEND, CB_ADDSTRING, 0, (LPARAM)blends[k]);
        if (g_td.type < 0) ShowWindow(GetDlgItem(d, TD_DELETE), SW_HIDE);
        SetWindowTextW(d, g_td.type < 0 ? L"New terrain colour" : L"Terrain colour");
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
        for (int id = TD_R; id <= TD_HEX; id++) {
            theme_control(GetDlgItem(d, id), WC_EDITA);
            SendDlgItemMessageW(d, id, EM_LIMITTEXT, id == TD_HEX ? 6 : 3, 0);
        }
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
    case WM_CTLCOLORSTATIC: case WM_CTLCOLOREDIT: case WM_CTLCOLORLISTBOX: case WM_CTLCOLORBTN: case WM_CTLCOLORDLG:
        return (INT_PTR)theme_ctlcolor(msg, w, l);   // a dialog procedure answers these with the brush itself
    case WM_DRAWITEM:
        if (w == TD_STRIP) { td_draw((const DRAWITEMSTRUCT *)l); return TRUE; }
        if (w == TD_SQUARE || w == TD_HUE) { td_gradient((const DRAWITEMSTRUCT *)l, w == TD_HUE); return TRUE; }
        if (w == TD_SWATCH) { td_swatch((const DRAWITEMSTRUCT *)l); return TRUE; }
        if (w == TD_RECENT) { td_recents((const DRAWITEMSTRUCT *)l); return TRUE; }
        if (theme_draw_item((const DRAWITEMSTRUCT *)l)) return TRUE;   // the dropdowns' lists
        break;
    case WM_LBUTTONDOWN:   // the drawn controls are statics: the mouse goes through them to here
        td_click(d, (POINT){ (short)LOWORD(l), (short)HIWORD(l) });
        return TRUE;
    case WM_MOUSEMOVE:
        if (g_td.drag) td_drag(d, (POINT){ (short)LOWORD(l), (short)HIWORD(l) });
        return TRUE;
    case WM_LBUTTONUP:
        if (g_td.drag) g_td.drag = 0, ReleaseCapture();
        return TRUE;
    case WM_CAPTURECHANGED:
        g_td.drag = 0;
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
        case TD_R: case TD_G: case TD_B: case TD_HEX:
            if (HIWORD(w) == EN_CHANGE && !g_td.filling) td_typed(d, LOWORD(w));
            return TRUE;
        case IDOK: case IDCANCEL: case TD_DELETE:
            if (LOWORD(w) == IDOK) td_remember();
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
    *it = (DLGITEMTEMPLATE){ .style = style | WS_CHILD | WS_VISIBLE, .dwExtendedStyle = cls == 0x81 ? WS_EX_CLIENTEDGE : 0,   // an edit: the theme draws that frame
                             .x = x, .y = y, .cx = cx, .cy = cy, .id = id };
    p = (WORD *)(it + 1);
    *p++ = 0xFFFF, *p++ = cls;   // a class by its atom: 0x80 button, 0x81 edit, 0x82 static, 0x85 combo box
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
    p = td_str(p, L"");          // the title: set at its start
    *p++ = (WORD)points;
    p = td_str(p, L"Segoe UI");  // ageofai_config.exe's (Windows' message font)
    // the buttons first: the dialog gives its first tab stop the focus when it is activated, and an automatic radio
    // button given the focus is clicked (the kind was one colour whenever the dialog opened). A radio button as tall
    // as the game's picture of one (20 pixels)
    p = td_item(p, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, 146, 218, 50, 15, IDOK, 0x80, L"OK");
    p = td_item(p, BS_PUSHBUTTON | WS_TABSTOP, 202, 218, 50, 15, IDCANCEL, 0x80, L"Cancel");
    p = td_item(p, BS_PUSHBUTTON | WS_TABSTOP, 8, 218, 50, 15, TD_DELETE, 0x80, L"Delete");
    p = td_item(p, BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP, 8, 6, 124, 12, TD_FLAT, 0x80, L"One colour");
    p = td_item(p, BS_AUTORADIOBUTTON, 8, 20, 124, 12, TD_RAMP, 0x80, L"A ramp between two colours");
    p = td_item(p, BS_AUTORADIOBUTTON, 8, 34, 124, 12, TD_EACH, 0x80, L"Each slot a colour of its own");
    p = td_item(p, SS_LEFT | SS_CENTERIMAGE, 140, 20, 30, 12, TD_BLEND_LABEL, 0x82, L" Blend:");   // the ramp's, beside it; the labels' texts a space in: off their boxes' edge, as a radio button's
    p = td_item(p, CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_TABSTOP | WS_GROUP, 172, 20, 80, 80, TD_BLEND, 0x85, L"");
    p = td_item(p, SS_LEFT | SS_CENTERIMAGE, 8, 54, 30, 12, TD_SLOTS_LABEL, 0x82, L" Slots:");
    p = td_item(p, CBS_DROPDOWNLIST | CBS_OWNERDRAWFIXED | CBS_HASSTRINGS | WS_VSCROLL | WS_TABSTOP | WS_GROUP, 40, 54, 36, 160,
                TD_COUNT, 0x85, L"");
    p = td_item(p, SS_LEFT | SS_CENTERIMAGE, 86, 54, 166, 12, TD_FREE, 0x82, L"");
    p = td_item(p, SS_OWNERDRAW, 8, 72, 244, 24, TD_STRIP, 0x82, L"");
    p = td_item(p, SS_LEFT | SS_CENTERIMAGE, 8, 101, 244, 12, TD_HINT, 0x82, L"");
    p = td_item(p, SS_OWNERDRAW, 8, 118, 128, 75, TD_SQUARE, 0x82, L"");   // the picker (td_rows: these three as tall as the fields)
    p = td_item(p, SS_OWNERDRAW, 140, 118, 14, 75, TD_HUE, 0x82, L"");
    p = td_item(p, SS_OWNERDRAW, 218, 118, 34, 75, TD_SWATCH, 0x82, L"");
    static const wchar_t *const fields[] = { L" R:", L" G:", L" B:", L" Hex:" };
    for (int k = 0; k < 4; k++) {
        p = td_item(p, SS_LEFT | SS_CENTERIMAGE, 158, (short)(118 + 21 * k), 22, 12, (WORD)(TD_R_LABEL + k), 0x82, fields[k]);
        p = td_item(p, WS_TABSTOP | ES_AUTOHSCROLL | (k < 3 ? ES_NUMBER : ES_UPPERCASE) | (k ? 0 : WS_GROUP), 182, (short)(118 + 21 * k), 32, 12, (WORD)(TD_R + k), 0x81, L"");
    }
    p = td_item(p, SS_LEFT | SS_CENTERIMAGE, 8, 198, 34, 12, TD_RECENT_LABEL, 0x82, L" Recent:");
    p = td_item(p, SS_OWNERDRAW, 44, 198, 208, 12, TD_RECENT, 0x82, L"");
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
