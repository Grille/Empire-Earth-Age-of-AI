// THE COLOUR PICKER: a square of saturation and value (of the hue chosen), a bar of the hues, the colour over what it was
// (a click on that half brings it back), the colour as numbers - R, G, B and hex, typed in: what is a value is the colour
// at once, what is not is shown in red and changes nothing -, the colours used last (kept while the program runs). The
// controls of the dialog that holds them, on the colour it gives (cpk_begin): the lower half of the scenario editor's
// terrain colour dialog (engine/terrain_dialog.c: its slots' colours), and a dialog of its own (cpk_dialog:
// ageofai_config.exe's colour settings). One at a time - a modal dialog's -, its state g_cpk. Its controls' ids
// CPK_SQUARE .. CPK_HEX_LABEL (the host's own outside them); the host passes its messages on (cpk_draw_item, cpk_click,
// cpk_mouse, cpk_command, cpk_bad) and is told of a change (g_cpk.changed). Words on it are the host's (the DLL's in the
// game's language, ageofai_config.exe's English). In the theme's look (ui/theme.c).
#define CPK_SQUARE 112
#define CPK_HUE 113
#define CPK_SWATCH 114
#define CPK_RECENT 115
#define CPK_RECENT_LABEL 116
#define CPK_R 117          // CPK_G, CPK_B after it, then CPK_HEX; their labels in that order
#define CPK_G 118
#define CPK_B 119
#define CPK_HEX 120
#define CPK_R_LABEL 121
#define CPK_G_LABEL 122
#define CPK_B_LABEL 123
#define CPK_HEX_LABEL 124
#define CPK_ITEMS 13       // the controls cpk_items puts into a dialog's template
#define CPK_RECENTS 16
#define CPK_BAD_TEXT RGB(255, 0, 0)
internal struct {
    uint8_t *c;                        // the colour worked on (the host's)
    uint8_t was[3];                    // what it was when it was given
    float hue, sat, val;               // its place in the picker (0 .. 1): the colour's, with what a grey or black doesn't tell
    int drag;                          // the mouse held on: CPK_SQUARE, CPK_HUE
    int edited, filling;               // changed since it was given; the fields being set from here (their EN_CHANGE ignored)
    unsigned bad;                      // the fields whose text is no value (a bit each from CPK_R on): shown in red
    void (*changed)(HWND d);           // the host's, after the colour changed (before the picker's own are drawn again)
} g_cpk;
internal uint8_t g_cpk_recent[CPK_RECENTS][3];   // the colours used last, the newest first
internal unsigned g_cpk_recents;

// hue 0 .. 1 round, saturation and value 0 .. 1
internal void cpk_hsv_rgb(float h, float s, float v, uint8_t out[3]) {
    float f = h * 6.0f;
    int i = (int)f;
    f -= (float)i;
    float p = v * (1 - s), q = v * (1 - s * f), t = v * (1 - s * (1 - f));
    const float c[6][3] = { { v, t, p }, { q, v, p }, { p, v, t }, { p, q, v }, { t, p, v }, { v, p, q } };
    for (int j = 0; j < 3; j++) out[j] = (uint8_t)(c[i % 6][j] * 255.0f + 0.5f);
}

// the picker's place from a colour: a grey keeps the hue it had, black its saturation too (the marks stay where they were)
internal void cpk_rgb_hsv(const uint8_t c[3]) {
    float r = c[0] / 255.0f, g = c[1] / 255.0f, b = c[2] / 255.0f;
    float top = r > g ? (r > b ? r : b) : (g > b ? g : b), low = r < g ? (r < b ? r : b) : (g < b ? g : b), d = top - low;
    g_cpk.val = top;
    if (top > 0) g_cpk.sat = d / top;
    if (d <= 0) return;
    float h = top == r ? (g - b) / d : top == g ? 2 + (b - r) / d : 4 + (r - g) / d;
    g_cpk.hue = (h < 0 ? h + 6 : h) / 6;
}

// the colour into the recent colours' front, if it was changed since it was given
internal void cpk_remember(void) {
    if (!g_cpk.edited || !g_cpk.c) return;
    g_cpk.edited = 0;
    uint8_t c[3];
    memcpy(c, g_cpk.c, 3);
    unsigned at = 0;
    while (at < g_cpk_recents && memcmp(g_cpk_recent[at], c, 3)) at++;
    if (at == g_cpk_recents && g_cpk_recents < CPK_RECENTS) g_cpk_recents++;
    if (at == g_cpk_recents) at--;
    memmove(g_cpk_recent[1], g_cpk_recent[0], at * 3);
    memcpy(g_cpk_recent[0], c, 3);
}

internal void cpk_mark(HWND d, int id, int bad) {
    unsigned bit = 1u << (id - CPK_R);
    if (!(g_cpk.bad & bit) == !bad) return;
    g_cpk.bad ^= bit;
    InvalidateRect(GetDlgItem(d, id), NULL, TRUE);
}

// the fields from the colour, but the one being typed in
internal void cpk_fields(HWND d, int skip) {
    const uint8_t *c = g_cpk.c;
    wchar_t t[8];
    g_cpk.filling = 1;
    for (int j = 0; j < 3; j++)
        if (CPK_R + j != skip) SetDlgItemInt(d, CPK_R + j, c[j], FALSE), cpk_mark(d, CPK_R + j, 0);
    if (skip != CPK_HEX) {
        wsprintfW(t, L"%02X%02X%02X", c[0], c[1], c[2]);
        SetDlgItemTextW(d, CPK_HEX, t);
        cpk_mark(d, CPK_HEX, 0);
    }
    g_cpk.filling = 0;
}

internal void cpk_redraw(HWND d) {
    static const int drawn[] = { CPK_SQUARE, CPK_HUE, CPK_SWATCH, CPK_RECENT };
    for (unsigned k = 0; k < sizeof drawn / sizeof *drawn; k++) InvalidateRect(GetDlgItem(d, drawn[k]), NULL, FALSE);
}

// the colour the picker works on from now (the one before into the recent ones, if changed)
internal void cpk_begin(HWND d, uint8_t c[3]) {
    cpk_remember();
    g_cpk.c = c;
    memcpy(g_cpk.was, c, 3);
    cpk_rgb_hsv(c);
    cpk_fields(d, 0);
    cpk_redraw(d);
}

// the colour changed (skip: the field it was typed in)
internal void cpk_changed(HWND d, int skip) {
    g_cpk.edited = 1;
    if (g_cpk.changed) g_cpk.changed(d);
    cpk_fields(d, skip);
    cpk_redraw(d);
}

// a colour for it (the "was" half, a recent one, the fields)
internal void cpk_set(HWND d, const uint8_t c[3], int skip) {
    memmove(g_cpk.c, c, 3);
    cpk_rgb_hsv(c);
    cpk_changed(d, skip);
}

internal void cpk_fill(HDC dc, const RECT *r, const uint8_t c[3]) {
    HBRUSH b = CreateSolidBrush(RGB(c[0], c[1], c[2]));
    FillRect(dc, r, b);
    DeleteObject(b);
}

// a drawn control's inside: its bevel round it, as a field's (none in the plain look, its place kept: the mouse maps to it)
internal RECT cpk_sunken(const DRAWITEMSTRUCT *di) {
    RECT r = di->rcItem;
    if (g_gl.on) theme_bevel(di->hDC, &r, 1);
    InflateRect(&r, -theme_px(2), -theme_px(2));
    return r;
}

// a frame t pixels thick just inside c, c then inside it
internal void cpk_frame(HDC dc, RECT *c, HBRUSH b, int t) {
    for (int i = 0; i < t; i++) {
        FrameRect(dc, c, b);
        InflateRect(c, -1, -1);
    }
}

// the square (across saturation, up value, of the hue chosen) or the bar of the hues, and the mark on it
internal void cpk_gradient(const DRAWITEMSTRUCT *di, int bar) {
    RECT r = cpk_sunken(di);
    int w = r.right - r.left, h = r.bottom - r.top;
    if (w < 2 || h < 2) return;
    BITMAPINFO bi = { .bmiHeader = { .biSize = sizeof bi.bmiHeader, .biWidth = w, .biHeight = -h, .biPlanes = 1, .biBitCount = 32 } };
    uint8_t *bits;
    HBITMAP bm = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
    if (!bm) return;
    uint8_t *o = bits, c[3];
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++, o += 4) {
            if (bar) cpk_hsv_rgb((float)y / (float)(h - 1), 1, 1, c);
            else cpk_hsv_rgb(g_cpk.hue, (float)x / (float)(w - 1), 1 - (float)y / (float)(h - 1), c);
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
    int mx = r.left + (int)(g_cpk.sat * (float)(w - 1) + 0.5f), my = r.top + (int)((1 - g_cpk.val) * (float)(h - 1) + 0.5f);
    int a = theme_px(5), t = theme_px(1);
    RECT m = { mx - a, my - a, mx + a + 1, my + a + 1 };
    if (bar) {
        my = r.top + (int)(g_cpk.hue * (float)(h - 1) + 0.5f);
        a = theme_px(3);
        m = (RECT){ r.left, my - a, r.right, my + a + 1 };
    }
    cpk_frame(di->hDC, &m, (HBRUSH)GetStockObject(BLACK_BRUSH), t);
    cpk_frame(di->hDC, &m, (HBRUSH)GetStockObject(WHITE_BRUSH), t);
    RestoreDC(di->hDC, saved);
}

// the colour over what it was when it was given (a click on that half brings it back)
internal void cpk_swatch(const DRAWITEMSTRUCT *di) {
    RECT r = cpk_sunken(di), a = r, b = r;
    a.bottom = b.top = (r.top + r.bottom) / 2;
    cpk_fill(di->hDC, &a, g_cpk.c);
    cpk_fill(di->hDC, &b, g_cpk.was);
}

internal void cpk_recents(const DRAWITEMSTRUCT *di) {
    RECT r = cpk_sunken(di);
    LONG w = r.right - r.left;
    for (unsigned k = 0; k < CPK_RECENTS; k++) {
        RECT c = { r.left + (LONG)k * w / CPK_RECENTS, r.top, r.left + (LONG)(k + 1) * w / CPK_RECENTS, r.bottom };
        if (k < g_cpk_recents) cpk_fill(di->hDC, &c, g_cpk_recent[k]);
        else FillRect(di->hDC, &c, g_pal.b_field);
        cpk_frame(di->hDC, &c, (HBRUSH)GetStockObject(BLACK_BRUSH), theme_px(1));
    }
}

// WM_DRAWITEM of one of the picker's drawn controls: 1 drawn
internal int cpk_draw_item(const DRAWITEMSTRUCT *di) {
    if (di->CtlID == CPK_SQUARE || di->CtlID == CPK_HUE) { cpk_gradient(di, di->CtlID == CPK_HUE); return 1; }
    if (di->CtlID == CPK_SWATCH) { cpk_swatch(di); return 1; }
    if (di->CtlID == CPK_RECENT) { cpk_recents(di); return 1; }
    return 0;
}

// a control's rectangles in the dialog: whole (returned: the point on it), inside its bevel
internal int cpk_hit(HWND d, int id, POINT p, RECT *in) {
    GetWindowRect(GetDlgItem(d, id), in);
    MapWindowPoints(NULL, d, (POINT *)in, 2);
    int on = PtInRect(in, p);
    InflateRect(in, -theme_px(2), -theme_px(2));
    return on;
}

// the mouse on the square or the bar: its place is the colour, kept on it outside
internal void cpk_drag(HWND d, POINT p) {
    RECT r;
    cpk_hit(d, g_cpk.drag, p, &r);
    float x = (float)(p.x - r.left) / (float)(r.right - r.left - 1), y = (float)(p.y - r.top) / (float)(r.bottom - r.top - 1);
    x = x < 0 ? 0 : x > 1 ? 1 : x, y = y < 0 ? 0 : y > 1 ? 1 : y;
    if (g_cpk.drag == CPK_HUE) g_cpk.hue = y;
    else g_cpk.sat = x, g_cpk.val = 1 - y;
    cpk_hsv_rgb(g_cpk.hue, g_cpk.sat, g_cpk.val, g_cpk.c);
    cpk_changed(d, 0);
}

// WM_LBUTTONDOWN at p (the drawn controls are statics: the mouse goes through them to the dialog): 1 on the picker
internal int cpk_click(HWND d, POINT p) {
    RECT r;
    if (cpk_hit(d, CPK_SQUARE, p, &r) || cpk_hit(d, CPK_HUE, p, &r)) {
        g_cpk.drag = cpk_hit(d, CPK_HUE, p, &r) ? CPK_HUE : CPK_SQUARE;
        SetCapture(d);
        cpk_drag(d, p);
    } else if (cpk_hit(d, CPK_SWATCH, p, &r)) {
        if (p.y >= (r.top + r.bottom) / 2) cpk_set(d, g_cpk.was, 0);
    } else if (cpk_hit(d, CPK_RECENT, p, &r)) {
        if (r.right > r.left && p.x >= r.left) {
            unsigned k = (unsigned)((p.x - r.left) * CPK_RECENTS / (r.right - r.left));
            if (k < g_cpk_recents) {
                uint8_t c[3];
                memcpy(c, g_cpk_recent[k], 3);
                cpk_set(d, c, 0);
            }
        }
    } else return 0;
    return 1;
}

// the mouse's other messages while a drag may be on
internal void cpk_mouse(HWND d, UINT msg, LPARAM l) {
    if (msg == WM_MOUSEMOVE && g_cpk.drag) cpk_drag(d, (POINT){ (short)LOWORD(l), (short)HIWORD(l) });
    if (msg == WM_LBUTTONUP && g_cpk.drag) g_cpk.drag = 0, ReleaseCapture();
    if (msg == WM_CAPTURECHANGED) g_cpk.drag = 0;
}

// a field's text as its value, 1 parsed: 0 .. 255, or six hex digits of either case after an optional #; spaces round it
// count for nothing
internal int cpk_parse(const wchar_t *t, int hex, unsigned *out) {
    while (*t == L' ') t++;
    if (hex && *t == L'#') t++;
    unsigned v = 0, n = 0;
    for (; *t && *t != L' '; t++, n++) {
        unsigned x = *t >= L'0' && *t <= L'9' ? *t - L'0' : hex && (*t | 32) >= L'a' && (*t | 32) <= L'f' ? (*t | 32) - L'a' + 10u : 16;
        if (x == 16) return 0;
        v = v * (hex ? 16 : 10) + x;
        if (!hex && v > 255) return 0;
    }
    while (*t == L' ') t++;
    if (*t || (hex ? n != 6 : !n)) return 0;
    *out = v;
    return 1;
}

// a field typed in: anything may be typed; what is a value is the colour at once, what is not is shown in red and
// changes nothing
internal void cpk_typed(HWND d, int id) {
    wchar_t t[64];
    unsigned v;
    GetDlgItemTextW(d, id, t, 64);
    int ok = cpk_parse(t, id == CPK_HEX, &v);
    cpk_mark(d, id, !ok);
    if (!ok) return;
    uint8_t c[3];
    memcpy(c, g_cpk.c, 3);
    if (id == CPK_HEX) c[0] = (uint8_t)(v >> 16), c[1] = (uint8_t)(v >> 8), c[2] = (uint8_t)v;
    else c[id - CPK_R] = (uint8_t)v;
    cpk_set(d, c, id);
}

// WM_COMMAND of a field (id, the notification code): 1 one of the picker's
internal int cpk_command(HWND d, int id, int code) {
    if (id < CPK_R || id > CPK_HEX) return 0;
    if (code == EN_CHANGE && !g_cpk.filling) cpk_typed(d, id);
    return 1;
}

// a field of the picker whose text is no value (WM_CTLCOLOREDIT: its text red)
internal int cpk_bad(int id) { return id >= CPK_R && id <= CPK_HEX && g_cpk.bad >> (id - CPK_R) & 1; }

// THE TEMPLATE: an in-memory dialog template, the dialog, then its items (dialog units)
internal WORD *cpk_tpl_align(WORD *p) { return (WORD *)(((uintptr_t)p + 3) & ~(uintptr_t)3); }
internal WORD *cpk_tpl_str(WORD *p, const wchar_t *s) {
    for (int n = 0; *s && n < 60; n++) *p++ = *s++;   // the words are the player's to change, a template's size isn't
    *p++ = 0;
    return p;
}
internal WORD *cpk_tpl_item(WORD *p, DWORD style, short x, short y, short cx, short cy, WORD id, WORD cls, const wchar_t *text) {
    p = cpk_tpl_align(p);
    DLGITEMTEMPLATE *it = (DLGITEMTEMPLATE *)p;
    *it = (DLGITEMTEMPLATE){ .style = style | WS_CHILD | WS_VISIBLE, .dwExtendedStyle = cls == 0x81 ? WS_EX_CLIENTEDGE : 0,   // an edit: the theme draws that frame
                             .x = x, .y = y, .cx = cx, .cy = cy, .id = id };
    p = (WORD *)(it + 1);
    *p++ = 0xFFFF, *p++ = cls;   // a class by its atom: 0x80 button, 0x81 edit, 0x82 static, 0x85 combo box
    p = cpk_tpl_str(p, text);
    *p++ = 0;                    // no creation data
    return p;
}
// the picker's CPK_ITEMS controls with their top left at x, y: 244 x 92 dialog units - the square, the bar, the fields
// and the swatch 75 high, the recent colours under them (recent: their label, a space in: off its box's edge)
internal WORD *cpk_items(WORD *p, short x, short y, const wchar_t *recent) {
    p = cpk_tpl_item(p, SS_OWNERDRAW, x, y, 128, 75, CPK_SQUARE, 0x82, L"");   // (a host may size these three as tall as the fields)
    p = cpk_tpl_item(p, SS_OWNERDRAW, (short)(x + 132), y, 14, 75, CPK_HUE, 0x82, L"");
    p = cpk_tpl_item(p, SS_OWNERDRAW, (short)(x + 210), y, 34, 75, CPK_SWATCH, 0x82, L"");
    static const wchar_t *const fields[] = { L" R:", L" G:", L" B:", L" Hex:" };
    for (int k = 0; k < 4; k++) {
        p = cpk_tpl_item(p, SS_LEFT | SS_CENTERIMAGE, (short)(x + 150), (short)(y + 21 * k), 22, 12, (WORD)(CPK_R_LABEL + k), 0x82, fields[k]);
        p = cpk_tpl_item(p, WS_TABSTOP | ES_AUTOHSCROLL | (k ? 0 : WS_GROUP), (short)(x + 174), (short)(y + 21 * k), 32, 12, (WORD)(CPK_R + k), 0x81, L"");
    }
    p = cpk_tpl_item(p, SS_LEFT | SS_CENTERIMAGE, x, (short)(y + 80), 34, 12, CPK_RECENT_LABEL, 0x82, recent);
    p = cpk_tpl_item(p, SS_OWNERDRAW, (short)(x + 36), (short)(y + 80), 208, 12, CPK_RECENT, 0x82, L"");
    return p;
}
// the square, the bar and the swatch from the first field's top to the last's bottom (the fields' height may differ
// from the template's: theme_control)
internal void cpk_fit(HWND d) {
    RECT a, b;
    GetWindowRect(GetDlgItem(d, CPK_R), &a);
    GetWindowRect(GetDlgItem(d, CPK_HEX), &b);
    MapWindowPoints(NULL, d, (POINT *)&a, 2);
    MapWindowPoints(NULL, d, (POINT *)&b, 2);
    static const int tall[] = { CPK_SQUARE, CPK_HUE, CPK_SWATCH };
    for (unsigned k = 0; k < sizeof tall / sizeof *tall; k++) {
        HWND c = GetDlgItem(d, tall[k]);
        RECT r;
        GetWindowRect(c, &r);
        MapWindowPoints(NULL, d, (POINT *)&r, 2);
        SetWindowPos(c, NULL, r.left, a.top, r.right - r.left, b.bottom - a.top, SWP_NOZORDER | SWP_NOACTIVATE);
    }
}

// THE DIALOG of its own (cpk_dialog): the picker and OK / Cancel, over its owner, modal; its words the caller's
struct cpk_words { const wchar_t *title, *ok, *cancel, *recent; };
internal struct { uint8_t c[3]; int ok; const struct cpk_words *w; } g_cpk_dlg;
internal INT_PTR CALLBACK cpk_proc(HWND d, UINT msg, WPARAM w, LPARAM l) {
    switch (msg) {
    case WM_INITDIALOG: {
        SetWindowTextW(d, g_cpk_dlg.w->title);
        HWND o = GetWindow(d, GW_OWNER);   // over its owner's middle
        RECT ro, rd;
        if (o && GetWindowRect(o, &ro) && GetWindowRect(d, &rd))
            SetWindowPos(d, HWND_TOP, (ro.left + ro.right - (rd.right - rd.left)) / 2, (ro.top + ro.bottom - (rd.bottom - rd.top)) / 2,
                         0, 0, SWP_NOSIZE);
        theme_window(d);
        theme_size(d);
        theme_control(GetDlgItem(d, IDOK), WC_BUTTONA);
        theme_control(GetDlgItem(d, IDCANCEL), WC_BUTTONA);
        for (int id = CPK_R; id <= CPK_HEX; id++) theme_control(GetDlgItem(d, id), WC_EDITA);
        cpk_fit(d);
        cpk_begin(d, g_cpk_dlg.c);
        SetFocus(GetDlgItem(d, CPK_HEX));   // the colour's text first: typed or pasted at once
        SendDlgItemMessageW(d, CPK_HEX, EM_SETSEL, 0, -1);
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
        if (cpk_draw_item((const DRAWITEMSTRUCT *)l)) return TRUE;
        if (theme_draw_item((const DRAWITEMSTRUCT *)l)) return TRUE;
        break;
    case WM_LBUTTONDOWN:
        cpk_click(d, (POINT){ (short)LOWORD(l), (short)HIWORD(l) });
        return TRUE;
    case WM_MOUSEMOVE: case WM_LBUTTONUP: case WM_CAPTURECHANGED:
        cpk_mouse(d, msg, l);
        return TRUE;
    case WM_COMMAND:
        if (cpk_command(d, LOWORD(w), HIWORD(w))) return TRUE;
        if (LOWORD(w) == IDOK || LOWORD(w) == IDCANCEL) {
            g_cpk_dlg.ok = LOWORD(w) == IDOK;
            if (g_cpk_dlg.ok) cpk_remember();
            g_cpk.c = NULL;
            EndDialog(d, 0);
            return TRUE;
        }
        break;
    }
    return FALSE;
}
// the picker on c (red, green, blue) over owner; 1: OK, c the colour chosen - else c as it was. points: the font's
// size (the dialog's parts follow it); the theme's look as its caller has set it up
internal int cpk_dialog(HWND owner, uint8_t c[3], const struct cpk_words *words, int points) {
    memset(&g_cpk, 0, sizeof g_cpk);
    memcpy(g_cpk_dlg.c, c, 3);
    g_cpk_dlg.ok = 0;
    g_cpk_dlg.w = words;
    static WORD tpl[512];
    memset(tpl, 0, sizeof tpl);
    DLGTEMPLATE *dt = (DLGTEMPLATE *)tpl;
    *dt = (DLGTEMPLATE){ .style = DS_MODALFRAME | DS_SETFONT | WS_POPUP | WS_CAPTION | WS_SYSMENU, .cdit = CPK_ITEMS + 2, .cx = 260, .cy = 129 };
    WORD *p = (WORD *)(dt + 1);
    *p++ = 0, *p++ = 0;              // no menu, the dialog class
    p = cpk_tpl_str(p, L"");         // the title: set at its start
    *p++ = (WORD)points;
    p = cpk_tpl_str(p, L"Segoe UI");
    p = cpk_items(p, 8, 8, words->recent);
    p = cpk_tpl_item(p, BS_DEFPUSHBUTTON | WS_TABSTOP | WS_GROUP, 146, 106, 50, 15, IDOK, 0x80, words->ok);
    cpk_tpl_item(p, BS_PUSHBUTTON | WS_TABSTOP, 202, 106, 50, 15, IDCANCEL, 0x80, words->cancel);
    if (DialogBoxIndirectParamW(GetModuleHandleW(NULL), dt, owner, cpk_proc, 0) < 0) return 0;
    if (g_cpk_dlg.ok) memcpy(c, g_cpk_dlg.c, 3);
    return g_cpk_dlg.ok;
}
