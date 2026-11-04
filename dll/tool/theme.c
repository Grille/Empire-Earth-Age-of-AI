// THE LOOK: the game's own when the player's Data\data.ssa has its pictures (game_art.c: the settings screen's
// painting under the window, the game's check boxes and dropdown knots, buttons and fields drawn as its menus draw
// them - always dark: its menus are), else light or dark as Windows' apps are (Settings - Personalization - Colors:
// "Choose your default app mode"), read once at the start. The colours the program draws with are g_pal's, never
// GetSysColor's, so every look keeps the list's meanings (grey the default, blue the game's own, the warning and
// mistake tints) readable - a dark colour loses its hue on white, a light one keeps it on black. Dark: Windows' own
// dark themes where its controls have one (DarkMode_Explorer: buttons, the list, scrollbars; DarkMode_CFD: edit fields
// and dropdowns - the names Explorer and the file dialog use), the rest by the WM_CTLCOLOR* messages; without the
// game's pictures a check box is drawn classic, the one control whose themed text Windows keeps black. The calls that
// switch the app's menus and scrollbars to dark (uxtheme's ordinals 133 and 135) have no names: looked up by number,
// left out where the Windows has none.
#include <uxtheme.h>
#include <dwmapi.h>
#include "tool/game_art.c"

struct palette {
    int dark;
    COLORREF back, field, text, grey, game, game_grey, tint_warn, tint_mistake, text_warn, text_mistake;
    HBRUSH b_back, b_field;
};
internal struct palette g_pal;

// the game's look: on when both pictures were read. ui: uicontrols.sst, 20x20 cells drawn whole and 1:1 (at 96 dpi)
// as the game draws them, in pairs - row 0 checked, empty, a radio on; row 1 a radio off, the knot up, down; row 2
// left, right, the scrollbar's ball; which of a pair the game shows unpressed measured against its setup screen (the
// pixels the same): a check box's the left one, the dropdown knot's the right one. paint: the settings screen's painting (gamesettings, 1024x768); bg: it
// darkened at the window's size (theme_size), b_bg a brush of it for what draws transparent over it
struct game_look { int on; HWND main; HDC ui_dc, paint_dc, bg_dc; HBITMAP ui, paint, bg; HBRUSH b_bg; int pw, ph; };
internal struct game_look g_gl;
#define GL_CELL_CHECKED 0, 0   // + 1 pressed
#define GL_CELL_EMPTY 0, 2   // + 1 pressed
#define GL_CELL_KNOT_DOWN 1, 5   // - 1 pressed
#define GL_CELL 20

internal int theme_windows_dark(void) {   // Windows' app mode: 1 dark
    DWORD light = 1, n = sizeof light;
    if (RegGetValueA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                     "AppsUseLightTheme", RRF_RT_REG_DWORD, NULL, &light, &n) != ERROR_SUCCESS) return 0;
    return light == 0;
}

// the game's pictures from the folder the program is in (the game folder, where it is installed), else the one the
// ini's folder is in (<game>\ageofai\ageofai.ini)
internal void theme_game_open(const char *ini_path) {
    char dir[MAX_PATH], *s;
    GetModuleFileNameA(NULL, dir, sizeof dir);
    if ((s = strrchr(dir, '\\'))) *s = 0;
    if (!art_open(dir)) {
        lstrcpynA(dir, ini_path, sizeof dir);
        for (int up = 0; up < 2; up++) if ((s = strrchr(dir, '\\'))) *s = 0;
        if (!art_open(dir)) return;
    }
    int w, h;
    uint32_t *px = art_texture("uicontrols", &w, &h);
    if (px && w == 128 && h == 128) g_gl.ui = art_bitmap(px, w, h);
    free(px);
    g_gl.paint = art_picture("gamesettings", 3, 4, &g_gl.pw, &g_gl.ph);
    if (!g_gl.ui || !g_gl.paint) return;
    g_gl.ui_dc = CreateCompatibleDC(NULL);
    SelectObject(g_gl.ui_dc, g_gl.ui);
    g_gl.paint_dc = CreateCompatibleDC(NULL);
    SelectObject(g_gl.paint_dc, g_gl.paint);
    g_gl.bg_dc = CreateCompatibleDC(NULL);
    g_gl.on = 1;
}

internal void theme_init(const char *ini_path) {
    struct palette *p = &g_pal;
    theme_game_open(ini_path);
    p->dark = g_gl.on || theme_windows_dark();
    if (p->dark) {   // the game's own look: black fields, light text
        p->back = RGB(32, 32, 32);
        p->field = g_gl.on ? RGB(8, 8, 8) : RGB(18, 18, 18);
        p->text = RGB(228, 228, 228);
        p->grey = RGB(130, 130, 130);
        p->game = RGB(110, 170, 255);
        p->game_grey = RGB(72, 108, 158);
        p->tint_warn = RGB(84, 66, 12);
        p->tint_mistake = RGB(98, 30, 30);
        p->text_warn = RGB(240, 190, 80);
        p->text_mistake = RGB(255, 115, 115);
        HMODULE ux = LoadLibraryA("uxtheme.dll");
        typedef int (WINAPI *app_mode_fn)(int);   // SetPreferredAppMode: 2 always dark
        app_mode_fn mode = ux ? (app_mode_fn)(void *)GetProcAddress(ux, MAKEINTRESOURCEA(135)) : NULL;
        if (mode) mode(2);
    } else {
        p->back = GetSysColor(COLOR_BTNFACE);
        p->field = GetSysColor(COLOR_WINDOW);
        p->text = GetSysColor(COLOR_WINDOWTEXT);
        p->grey = GetSysColor(COLOR_GRAYTEXT);
        p->game = RGB(0, 70, 190);
        p->game_grey = RGB(120, 150, 205);
        p->tint_warn = RGB(255, 240, 180);
        p->tint_mistake = RGB(255, 205, 205);
        p->text_warn = RGB(150, 100, 0);
        p->text_mistake = RGB(190, 0, 0);
    }
    p->b_back = CreateSolidBrush(p->back);
    p->b_field = CreateSolidBrush(p->field);
}

// the window's background at its size (WM_SIZE): the painting over the whole of it, cut to its shape (no stretch
// out of proportion), darkened to under half so text reads over it
internal void theme_size(HWND w) {
    if (!g_gl.on) return;
    g_gl.main = w;   // the window the painting is under (the help's window has none)
    RECT rc;
    GetClientRect(w, &rc);
    int cw = rc.right > 1 ? rc.right : 1, ch = rc.bottom > 1 ? rc.bottom : 1;
    BITMAPINFO bi = { 0 };
    bi.bmiHeader.biSize = sizeof bi.bmiHeader;
    bi.bmiHeader.biWidth = cw;
    bi.bmiHeader.biHeight = -ch;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 32;
    uint8_t *bits;
    HBITMAP bm = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void **)&bits, NULL, 0);
    if (!bm) return;
    SelectObject(g_gl.bg_dc, bm);
    if (g_gl.bg) DeleteObject(g_gl.bg);
    if (g_gl.b_bg) DeleteObject(g_gl.b_bg);
    g_gl.bg = bm;
    int sw = g_gl.pw, sh = g_gl.pw * ch / cw;   // the painting's part with the window's shape, centred
    if (sh > g_gl.ph) { sh = g_gl.ph; sw = g_gl.ph * cw / ch; }
    SetStretchBltMode(g_gl.bg_dc, HALFTONE);
    SetBrushOrgEx(g_gl.bg_dc, 0, 0, NULL);
    StretchBlt(g_gl.bg_dc, 0, 0, cw, ch, g_gl.paint_dc, (g_gl.pw - sw) / 2, (g_gl.ph - sh) / 2, sw, sh, SRCCOPY);
    GdiFlush();
    for (size_t i = 0; i < (size_t)cw * ch * 4; i++) bits[i] = (uint8_t)(bits[i] * 45 / 100);
    g_gl.b_bg = CreatePatternBrush(bm);
}

internal int theme_erase(HWND w, HDC dc) {   // the window's background: 1 drawn
    if (!g_gl.on) return 0;
    RECT r;
    GetClientRect(w, &r);
    BitBlt(dc, 0, 0, r.right, r.bottom, g_gl.bg_dc, 0, 0, SRCCOPY);
    return 1;
}

internal POINT theme_at(HWND c) {   // a child's place in its parent's client area
    POINT pt = { 0, 0 };
    MapWindowPoints(c, GetParent(c), &pt, 1);
    return pt;
}

internal void theme_under(HDC dc, HWND c, const RECT *r) {   // what lies under a child: the background's part, else back
    if (g_gl.on && GetParent(c) && GetParent(c) == g_gl.main) {
        POINT pt = theme_at(c);
        BitBlt(dc, r->left, r->top, r->right - r->left, r->bottom - r->top, g_gl.bg_dc, pt.x + r->left, pt.y + r->top, SRCCOPY);
    } else FillRect(dc, r, g_pal.b_back);
}

internal void theme_shade(HDC dc, const RECT *r, int alpha) {   // black over r, alpha of 255
    static HDC black;
    if (!black) {
        black = CreateCompatibleDC(NULL);
        SelectObject(black, CreateCompatibleBitmap(GetDC(NULL), 1, 1));   // 1x1, black
        PatBlt(black, 0, 0, 1, 1, BLACKNESS);
    }
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, (BYTE)alpha, 0 };
    AlphaBlend(dc, r->left, r->top, r->right - r->left, r->bottom - r->top, black, 0, 0, 1, 1, bf);
}

// the game's bevel, 2 pixels round r (measured in its 1024x768 menus): light (176,194,193) at the top and left, dark
// (89,101,100) at the bottom and right - a button raised so; in: the other way round, a field and a button pressed
internal void theme_bevel(HDC dc, const RECT *r, int in) {
    HBRUSH hi = CreateSolidBrush(RGB(176, 194, 193)), lo = CreateSolidBrush(RGB(89, 101, 100)), a = in ? lo : hi, b = in ? hi : lo;
    RECT top = { r->left, r->top, r->right, r->top + 2 }, left = { r->left, r->top, r->left + 2, r->bottom };
    RECT bottom = { r->left + 1, r->bottom - 2, r->right, r->bottom }, right = { r->right - 2, r->top + 1, r->right, r->bottom };
    FillRect(dc, &top, a);
    FillRect(dc, &left, a);
    FillRect(dc, &bottom, b);
    FillRect(dc, &right, b);
    DeleteObject(hi);
    DeleteObject(lo);
}

internal void theme_cell(HDC dc, int x, int y, int size, int row, int col) {   // a picture of uicontrols.sst
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };
    AlphaBlend(dc, x, y, size, size, g_gl.ui_dc, col * GL_CELL, row * GL_CELL, GL_CELL, GL_CELL, bf);
}

internal void theme_text(HDC dc, HWND c, RECT *r, const char *text, COLORREF color, UINT how) {
    HFONT old = (HFONT)SelectObject(dc, (HFONT)SendMessageA(c, WM_GETFONT, 0, 0));
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    DrawTextA(dc, text, -1, r, how | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX | DT_END_ELLIPSIS);
    SelectObject(dc, old);
}

// a push button as the game's menus draw theirs: the background darkened under it (less under the mouse), the bevel,
// the text white; a check box: the game's box, its text beside it
internal void theme_paint_button(HWND c, HDC dc) {
    RECT r;
    char text[128];
    GetClientRect(c, &r);
    GetWindowTextA(c, text, sizeof text);
    LRESULT st = SendMessageA(c, BM_GETSTATE, 0, 0);
    int down = (st & BST_PUSHED) != 0, hot = (st & BST_HOT) != 0, off = !IsWindowEnabled(c), focus = GetFocus() == c;
    theme_under(dc, c, &r);
    if ((GetWindowLongA(c, GWL_STYLE) & BS_TYPEMASK) == BS_AUTOCHECKBOX) {
        int size = MulDiv(GL_CELL, GetDeviceCaps(dc, LOGPIXELSY), 96), y = (r.bottom - size) / 2;
        int on = SendMessageA(c, BM_GETCHECK, 0, 0) == BST_CHECKED;
        if (on) theme_cell(dc, 0, y, size, GL_CELL_CHECKED + (down || hot));
        else theme_cell(dc, 0, y, size, GL_CELL_EMPTY + (down || hot));
        RECT t = { size + size / 4, 0, r.right, r.bottom };
        theme_text(dc, c, &t, text, off ? g_pal.grey : g_pal.text, DT_LEFT);
        if (focus) { t.right = t.left + 2 + LOWORD(GetTabbedTextExtentA(dc, text, lstrlenA(text), 0, NULL)); DrawFocusRect(dc, &t); }
        return;
    }
    theme_shade(dc, &r, down ? 200 : hot ? 105 : 160);
    theme_bevel(dc, &r, down);
    RECT t = r;
    if (down) OffsetRect(&t, 1, 1);
    theme_text(dc, c, &t, text, off ? g_pal.grey : g_pal.text, DT_CENTER);
    if (focus) { InflateRect(&r, -4, -4); DrawFocusRect(dc, &r); }
}

// a dropdown as the game's: a black field in the sunken bevel, the text, and right after it the knot, a square of the
// whole height
internal void theme_paint_combo(HWND c, HDC dc) {
    RECT r;
    char text[256] = "";
    GetClientRect(c, &r);
    int sel = (int)SendMessageA(c, CB_GETCURSEL, 0, 0);
    if (sel >= 0 && SendMessageA(c, CB_GETLBTEXTLEN, sel, 0) < (LRESULT)sizeof text) SendMessageA(c, CB_GETLBTEXT, sel, (LPARAM)text);
    int size = r.bottom;
    RECT f = { r.left, r.top, r.right - size, r.bottom };
    FillRect(dc, &f, g_pal.b_field);
    theme_bevel(dc, &f, 1);
    theme_cell(dc, r.right - size, 0, size, GL_CELL_KNOT_DOWN - (SendMessageA(c, CB_GETDROPPEDSTATE, 0, 0) != 0));
    RECT t = { f.left + 4, f.top + 2, f.right - 2, f.bottom - 2 };
    theme_text(dc, c, &t, text, IsWindowEnabled(c) ? g_pal.text : g_pal.grey, DT_LEFT);
    if (GetFocus() == c && !SendMessageA(c, CB_GETDROPPEDSTATE, 0, 0)) { InflateRect(&t, 2, -1); DrawFocusRect(dc, &t); }
}

// a field's frame (an edit, the list: their client edge, 2 pixels) the game's sunken bevel in place of Windows' line
internal void theme_frame(HWND c) {
    HDC dc = GetWindowDC(c);
    RECT r;
    GetWindowRect(c, &r);
    OffsetRect(&r, -r.left, -r.top);
    theme_bevel(dc, &r, 1);
    ReleaseDC(c, dc);
}

// what the controls drawn here look like now: a change (the mouse over it, a press, a check, the focus, a choice, the
// dropdown open) paints them again - the control's own code draws some of them at once, past WM_PAINT
internal LRESULT theme_state(HWND c, int combo) {
    if (combo) return SendMessageA(c, CB_GETCURSEL, 0, 0) << 2 | (SendMessageA(c, CB_GETDROPPEDSTATE, 0, 0) != 0) << 1 | (GetFocus() == c);
    return SendMessageA(c, BM_GETSTATE, 0, 0) << 2 | SendMessageA(c, BM_GETCHECK, 0, 0);
}

internal LRESULT CALLBACK theme_drawn_proc(HWND c, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR combo) {
    (void)id;
    if (msg == BM_GETSTATE || msg == BM_GETCHECK || msg == CB_GETCURSEL || msg == CB_GETDROPPEDSTATE)
        return DefSubclassProc(c, msg, wp, lp);   // theme_state's own questions: straight through
    if (msg == WM_ERASEBKGND) return 1;
    if (msg == WM_PAINT || msg == WM_PRINTCLIENT) {
        PAINTSTRUCT ps;
        HDC dc = msg == WM_PAINT ? BeginPaint(c, &ps) : (HDC)wp;
        RECT r;
        GetClientRect(c, &r);
        HDC mem = CreateCompatibleDC(dc);   // drawn off screen: no flicker
        HBITMAP bm = CreateCompatibleBitmap(dc, r.right, r.bottom), old = (HBITMAP)SelectObject(mem, bm);
        if (combo) theme_paint_combo(c, mem); else theme_paint_button(c, mem);
        BitBlt(dc, 0, 0, r.right, r.bottom, mem, 0, 0, SRCCOPY);
        SelectObject(mem, old);
        DeleteObject(bm);
        DeleteDC(mem);
        if (msg == WM_PAINT) EndPaint(c, &ps);
        return 0;
    }
    LRESULT before = theme_state(c, (int)combo);
    LRESULT res = DefSubclassProc(c, msg, wp, lp);
    if (theme_state(c, (int)combo) != before || msg == WM_SETTEXT || msg == WM_ENABLE || msg == WM_SETFOCUS ||
        msg == WM_KILLFOCUS || msg == WM_MOUSELEAVE || msg == WM_LBUTTONUP || msg == WM_KEYUP) InvalidateRect(c, NULL, FALSE);
    return res;
}

// THE SCROLLBAR (a field's vertical one: the list, the help) as the game's: its knot up at the top, down at the bottom,
// whole cells 1:1 - the bar 20 pixels wide (at 96 dpi), Windows' 17 and the rest taken from the client area
// (WM_NCCALCSIZE) - the track between them in the field's sunken bevel, the silver ball on it (not stretched to the
// share shown, as the game's isn't). Windows' bar is never shown: painted over after every message that may have
// drawn it (SetScrollInfo draws at once, past WM_NCPAINT), its mouse handled here - the knots and the track repeat
// while held, the ball dragged scrolls the control by lines (a list by its rows' height, an edit by EM_LINESCROLL).
#define GL_CELL_KNOT_UP 1, 2   // + 1 pressed (the brighter one unpressed, as the down knot: measured against the game)
#define GL_CELL_BALL 2, 4
enum { SB_PART_NONE, SB_PART_UP, SB_PART_DOWN, SB_PART_PAGE_UP, SB_PART_PAGE_DOWN, SB_PART_BALL };
internal HWND g_sb_held;   // the control whose bar the mouse holds, and the part
internal int g_sb_part;

internal int theme_sb_width(HWND c) {
    HDC dc = GetDC(c);
    int w = MulDiv(GL_CELL, GetDeviceCaps(dc, LOGPIXELSY), 96);
    ReleaseDC(c, dc);
    return w;
}

// the bar in the window's coordinates (GetWindowDC's), 0 none shown; *ball: the ball's top and the track's ends
internal int theme_sb_rect(HWND c, RECT *bar, int *track0, int *track1, int *ball) {
    if (!(GetWindowLongA(c, GWL_STYLE) & WS_VSCROLL)) return 0;
    RECT w, cl;
    GetWindowRect(c, &w);
    GetClientRect(c, &cl);
    MapWindowPoints(c, NULL, (POINT *)&cl, 2);
    int width = theme_sb_width(c);
    bar->left = cl.right - w.left;
    bar->right = bar->left + width;
    bar->top = cl.top - w.top;
    bar->bottom = cl.bottom - w.top;
    if (bar->bottom - bar->top < 2 * width) return 0;
    *track0 = bar->top + width;
    *track1 = bar->bottom - width;
    SCROLLINFO si = { sizeof si, SIF_ALL, 0, 0, 0, 0, 0 };
    GetScrollInfo(c, SB_VERT, &si);
    int range = si.nMax - si.nMin + 1 - (int)si.nPage, room = *track1 - *track0 - width;
    *ball = range > 0 ? *track0 + (int)((long long)(si.nPos - si.nMin) * (room > 0 ? room : 0) / range) : -1;
    return 1;
}

internal void theme_sb_paint(HWND c) {
    RECT bar;
    int t0, t1, ball;
    if (!g_gl.on || !theme_sb_rect(c, &bar, &t0, &t1, &ball)) return;
    int w = bar.right - bar.left, h = bar.bottom - bar.top, held = g_sb_held == c ? g_sb_part : SB_PART_NONE;
    HDC dc = GetWindowDC(c), mem = CreateCompatibleDC(dc);
    HBITMAP bm = CreateCompatibleBitmap(dc, w, h), old = (HBITMAP)SelectObject(mem, bm);
    RECT track = { 0, t0 - bar.top, w, t1 - bar.top };
    FillRect(mem, &track, g_pal.b_field);
    theme_bevel(mem, &track, 1);
    theme_cell(mem, 0, 0, w, GL_CELL_KNOT_UP + (held == SB_PART_UP));
    theme_cell(mem, 0, h - w, w, GL_CELL_KNOT_DOWN - (held == SB_PART_DOWN));
    if (ball >= 0) theme_cell(mem, 0, ball - bar.top, w, GL_CELL_BALL);
    BitBlt(dc, bar.left, bar.top, w, h, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteObject(bm);
    DeleteDC(mem);
    ReleaseDC(c, dc);
}

internal int theme_sb_part(HWND c, POINT screen) {   // which part of the bar a point is on
    RECT bar, w;
    int t0, t1, ball;
    if (!theme_sb_rect(c, &bar, &t0, &t1, &ball)) return SB_PART_NONE;
    GetWindowRect(c, &w);
    int x = screen.x - w.left, y = screen.y - w.top;
    if (x < bar.left || x >= bar.right || y < bar.top || y >= bar.bottom) return SB_PART_NONE;
    if (y < t0) return SB_PART_UP;
    if (y >= t1) return SB_PART_DOWN;
    if (ball < 0) return SB_PART_NONE;
    if (y < ball) return SB_PART_PAGE_UP;
    return y >= ball + (bar.right - bar.left) ? SB_PART_PAGE_DOWN : SB_PART_BALL;
}

internal void theme_sb_lines(HWND c, int lines) {   // the control scrolled by lines (the ball dragged)
    char cls[32] = "";
    GetClassNameA(c, cls, sizeof cls);
    if (!lstrcmpiA(cls, WC_LISTVIEWA)) {
        RECT r = { LVIR_BOUNDS, 0, 0, 0 };
        int row = SendMessageA(c, LVM_GETITEMRECT, 0, (LPARAM)&r) ? r.bottom - r.top : 16;
        SendMessageA(c, LVM_SCROLL, 0, lines * row);
    } else SendMessageA(c, EM_LINESCROLL, 0, lines);
}

// the mouse held on the bar until it is let go: the knots and the track repeat (after 400 ms, then every 50), the ball
// follows the mouse
internal void theme_sb_track(HWND c, int part, POINT at) {
    RECT bar;
    int t0, t1, ball;
    if (!theme_sb_rect(c, &bar, &t0, &t1, &ball)) return;
    RECT win;
    GetWindowRect(c, &win);
    int grab = at.y - win.top - ball, width = bar.right - bar.left;
    g_sb_held = c;
    g_sb_part = part;
    SetCapture(c);
    DWORD next = GetTickCount();
    for (int first = 1; GetCapture() == c; first = 0) {
        POINT pt;
        GetCursorPos(&pt);
        if (part == SB_PART_BALL) {
            SCROLLINFO si = { sizeof si, SIF_ALL, 0, 0, 0, 0, 0 };
            GetScrollInfo(c, SB_VERT, &si);
            int range = si.nMax - si.nMin + 1 - (int)si.nPage, room = t1 - t0 - width;
            if (range > 0 && room > 0) {
                int y = pt.y - win.top - grab - t0, want = si.nMin + (int)(((long long)y * range + room / 2) / room);
                if (want < si.nMin) want = si.nMin;
                if (want > si.nMin + range) want = si.nMin + range;
                if (want != si.nPos) theme_sb_lines(c, want - si.nPos);
            }
        } else if (GetTickCount() >= next) {
            int now = theme_sb_part(c, pt), code = part == SB_PART_UP ? SB_LINEUP : part == SB_PART_DOWN ? SB_LINEDOWN :
                      part == SB_PART_PAGE_UP ? SB_PAGEUP : SB_PAGEDOWN;
            if (first || now == part)   // held: while the mouse is still on it (a page: on that side of the ball)
                SendMessageA(c, WM_VSCROLL, MAKEWPARAM(code, 0), 0);
            next = GetTickCount() + (first ? 400 : 50);
        }
        theme_sb_paint(c);
        MSG m;
        while (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) {
            if (m.message == WM_LBUTTONUP || m.message == WM_NCLBUTTONUP || (m.message == WM_KEYDOWN && m.wParam == VK_ESCAPE)) { ReleaseCapture(); break; }
            if (m.message == WM_MOUSEMOVE || m.message == WM_NCMOUSEMOVE || m.message == WM_LBUTTONDOWN) continue;
            TranslateMessage(&m);
            DispatchMessageA(&m);
        }
        if (GetCapture() == c) MsgWaitForMultipleObjects(0, NULL, FALSE, 15, QS_ALLINPUT);
    }
    if (GetCapture() == c) ReleaseCapture();
    SendMessageA(c, WM_VSCROLL, MAKEWPARAM(SB_ENDSCROLL, 0), 0);
    g_sb_held = NULL;
    theme_sb_paint(c);
}

// a field's messages for its bar: 1 handled (*res the result); after any other the bar painted again when the
// control's scrolling changed (or WM_NCPAINT drew Windows' bar)
internal int theme_sb_before(HWND c, UINT msg, WPARAM wp, LPARAM lp, LRESULT *res) {
    if (!g_gl.on) return 0;
    if (msg == WM_NCCALCSIZE && (GetWindowLongA(c, GWL_STYLE) & WS_VSCROLL)) {   // the bar wider by taking from the client
        *res = DefSubclassProc(c, msg, wp, lp);
        RECT *r = (RECT *)lp;
        int more = theme_sb_width(c) - GetSystemMetrics(SM_CXVSCROLL);
        if (more > 0 && r->right - more > r->left) r->right -= more;
        return 1;
    }
    POINT pt = { (short)LOWORD(lp), (short)HIWORD(lp) };
    if (msg == WM_NCHITTEST && theme_sb_part(c, pt) != SB_PART_NONE) { *res = HTVSCROLL; return 1; }
    if ((msg == WM_NCLBUTTONDOWN || msg == WM_NCLBUTTONDBLCLK) && wp == HTVSCROLL) {
        int part = theme_sb_part(c, pt);
        if (part != SB_PART_NONE) theme_sb_track(c, part, pt);
        *res = 0;
        return 1;
    }
    if ((msg == WM_NCMOUSEMOVE || msg == WM_NCMOUSELEAVE) && wp == HTVSCROLL) { *res = 0; return 1; }   // no hot look
    return 0;
}

internal LRESULT theme_sb_after(HWND c, UINT msg, WPARAM wp, LPARAM lp) {   // the message through, the bar kept the game's
    if (!g_gl.on) return DefSubclassProc(c, msg, wp, lp);
    SCROLLINFO a = { sizeof a, SIF_ALL, 0, 0, 0, 0, 0 }, b = a;
    LONG style = GetWindowLongA(c, GWL_STYLE);
    GetScrollInfo(c, SB_VERT, &a);
    LRESULT res = DefSubclassProc(c, msg, wp, lp);
    GetScrollInfo(c, SB_VERT, &b);
    if (msg == WM_NCPAINT || msg == WM_SIZE || a.nPos != b.nPos || a.nPage != b.nPage || a.nMax != b.nMax ||
        style != GetWindowLongA(c, GWL_STYLE)) theme_sb_paint(c);
    return res;
}

internal LRESULT CALLBACK theme_frame_proc(HWND c, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR data) {
    (void)id; (void)data;
    LRESULT res;
    if (theme_sb_before(c, msg, wp, lp, &res)) return res;
    res = theme_sb_after(c, msg, wp, lp);
    if (msg == WM_NCPAINT || msg == WM_SETFOCUS || msg == WM_KILLFOCUS || msg == WM_MOUSEMOVE) theme_frame(c);
    return res;
}

internal void theme_allow_dark(HWND w) {   // AllowDarkModeForWindow: the window's own theme parts dark too
    static int (WINAPI *allow)(HWND, int);
    static int looked;
    if (!looked) {
        HMODULE ux = LoadLibraryA("uxtheme.dll");
        allow = ux ? (int (WINAPI *)(HWND, int))(void *)GetProcAddress(ux, MAKEINTRESOURCEA(133)) : NULL;
        looked = 1;
    }
    if (allow) allow(w, 1);
}

internal void theme_window(HWND w) {   // a top-level window: its title bar dark (DWMWA_USE_IMMERSIVE_DARK_MODE)
    if (!g_pal.dark) return;
    BOOL on = TRUE;
    DwmSetWindowAttribute(w, 20, &on, sizeof on);
}

// a header's text in the palette's colour: the dark header theme draws it black (the list's subclass); the list's
// frame the game's
internal LRESULT CALLBACK theme_list_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR data) {
    (void)id; (void)data;
    if (msg == WM_NOTIFY && ((NMHDR *)lp)->code == NM_CUSTOMDRAW && ((NMHDR *)lp)->hwndFrom == ListView_GetHeader(w)) {
        NMCUSTOMDRAW *cd = (NMCUSTOMDRAW *)lp;
        if (cd->dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
        if (cd->dwDrawStage == CDDS_ITEMPREPAINT) { SetTextColor(cd->hdc, g_pal.text); return CDRF_DODEFAULT; }
    }
    LRESULT res;
    if (theme_sb_before(w, msg, wp, lp, &res)) return res;
    res = theme_sb_after(w, msg, wp, lp);
    if (msg == WM_NCPAINT && g_gl.on) theme_frame(w);
    return res;
}

internal void theme_control(HWND c, const char *cls) {   // a child: its theme for the mode
    if (!g_pal.dark) return;
    if (!lstrcmpA(cls, WC_LISTVIEWA)) {
        HWND h = ListView_GetHeader(c);
        theme_allow_dark(c);
        SetWindowTheme(c, L"DarkMode_Explorer", NULL);
        theme_allow_dark(h);
        SetWindowTheme(h, L"ItemsView", NULL);
        SetWindowSubclass(c, theme_list_proc, 1, 0);
        SetWindowPos(c, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);   // the bar's width (theme_sb_before)
        ListView_SetBkColor(c, g_pal.field);
        ListView_SetTextBkColor(c, g_pal.field);
        ListView_SetTextColor(c, g_pal.text);
        return;
    }
    int check = !lstrcmpA(cls, WC_BUTTONA) && (GetWindowLongA(c, GWL_STYLE) & BS_TYPEMASK) == BS_AUTOCHECKBOX;
    if (check && !g_gl.on) {
        SetWindowTheme(c, L" ", L" ");   // classic: its text in WM_CTLCOLORSTATIC's colour
        return;
    }
    theme_allow_dark(c);
    SetWindowTheme(c, !lstrcmpA(cls, WC_COMBOBOXA) || !lstrcmpA(cls, WC_EDITA) ? L"DarkMode_CFD" : L"DarkMode_Explorer", NULL);
    if (!g_gl.on) return;
    if (!lstrcmpA(cls, WC_BUTTONA)) SetWindowSubclass(c, theme_drawn_proc, 1, 0);
    else if (!lstrcmpA(cls, WC_COMBOBOXA)) {   // the height of the knot: it is drawn 1:1, a square of the whole height
        SetWindowSubclass(c, theme_drawn_proc, 1, 1);
        HDC dc = GetDC(c);
        int want = MulDiv(GL_CELL, GetDeviceCaps(dc, LOGPIXELSY), 96);
        ReleaseDC(c, dc);
        for (int i = 0; i < 2; i++) {   // the selection field's height sets the whole: the rest found by measuring
            RECT r;
            GetWindowRect(c, &r);
            int item = (int)SendMessageA(c, CB_GETITEMHEIGHT, (WPARAM)-1, 0) + want - (r.bottom - r.top);
            SendMessageA(c, CB_SETITEMHEIGHT, (WPARAM)-1, item > 1 ? item : 1);
        }
    }
    else if (!lstrcmpA(cls, WC_EDITA) && (GetWindowLongA(c, GWL_EXSTYLE) & WS_EX_CLIENTEDGE)) {
        SetWindowSubclass(c, theme_frame_proc, 1, 0);
        SetWindowPos(c, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

// the WM_CTLCOLOR* messages in dark: the field's black for edits, lists and a read-only edit (it asks as a static);
// the rest the window's grey, or in the game's look the painting under it (the brush's origin at the child's place:
// the child fills with its part of it); 0 in light (Windows' own)
internal LRESULT theme_ctlcolor(UINT msg, WPARAM wp, LPARAM lp) {
    if (!g_pal.dark) return 0;
    char cls[16] = "";
    GetClassNameA((HWND)lp, cls, sizeof cls);
    int field = msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORLISTBOX || !lstrcmpiA(cls, "Edit");
    SetTextColor((HDC)wp, g_pal.text);
    SetBkColor((HDC)wp, field ? g_pal.field : g_pal.back);
    if (field) return (LRESULT)g_pal.b_field;
    if (g_gl.on && g_gl.b_bg && GetParent((HWND)lp) == g_gl.main) {
        POINT pt = theme_at((HWND)lp);
        SetBkMode((HDC)wp, TRANSPARENT);
        SetBrushOrgEx((HDC)wp, -pt.x, -pt.y, NULL);
        return (LRESULT)g_gl.b_bg;
    }
    return (LRESULT)g_pal.b_back;
}
