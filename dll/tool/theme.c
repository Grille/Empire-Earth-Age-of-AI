// THE LOOK: the game's own when the player's Data\data.ssa has its pictures (game_art.c: the settings screen's
// painting under the window, the game's check boxes and dropdown knots, buttons and fields drawn as its menus draw
// them), else plain: the same colours without the pictures - always dark, as the game's menus are (a light look of
// its own, after Windows' app mode, went 2026-11-05: the player - "we have the game's style now"; one palette to keep
// the list's meanings readable in, not two). The colours the program draws with are g_pal's, never GetSysColor's.
// Windows' own dark themes where its controls have one (DarkMode_Explorer: buttons, the list, scrollbars;
// DarkMode_CFD: edit fields and dropdowns - the names Explorer and the file dialog use), the rest by the WM_CTLCOLOR*
// messages; without the
// game's pictures a check box is drawn classic, the one control whose themed text Windows keeps black. The calls that
// switch the app's menus and scrollbars to dark (uxtheme's ordinals 133 and 135) have no names: looked up by number,
// left out where the Windows has none.
#include <uxtheme.h>
#include <dwmapi.h>
#include "tool/game_art.c"

struct palette {
    COLORREF back, field, text, grey, game, game_grey, tint_warn, tint_mistake, text_warn, text_mistake, focus,
             overruled, forced;   // a master's member: its value set but not played, off but played on
    HBRUSH b_back, b_field;
};
internal struct palette g_pal;

// the game's look: on when both pictures were read. ui: uicontrols.sst, 20x20 cells drawn whole and 1:1 (at 96 dpi)
// as the game draws them, in pairs - row 0 checked, empty, a radio on; row 1 a radio off, the knot up, down; row 2
// left, right, the scrollbar's ball; which of a pair the game shows unpressed measured against its setup screen (the
// pixels the same): a check box's the left one, the dropdown knot's the right one. paint: the settings screen's painting (gamesettings, 1024x768); bg: it
// at the window's size (theme_size), b_bg a brush of it; dim: it darkened to under half, b_dim its brush for a label
// drawn transparent over it (the list and the panel cover the rest: the painting itself as bright as the game's)
struct game_look { int on; HWND main; HDC ui_dc, paint_dc, bg_dc; HBITMAP ui, paint, bg, dim; HBRUSH b_bg, b_dim; int pw, ph; };
internal struct game_look g_gl;
#define GL_CELL_CHECKED 0, 0   // + 1 pressed
#define GL_CELL_EMPTY 0, 2   // + 1 pressed
#define GL_CELL_KNOT_DOWN 1, 5   // - 1 pressed
#define GL_CELL 20

// the original game's folder (the 2017 update's): Art of Conquest's archive holds only the art it adds, the rest it
// reads from there - found as AoC finds it, HKCU\Software\SSSI\Empire Earth's "Installed From Volume" and "Directory",
// in the registry or in the settings file the engine reads when the mod is loaded with it (ageofai\eesettings.ini
// beside the ini: the registry's values quoted, a backslash doubled)
internal int theme_original_dir(const char *ini_path, char *dir, int cap) {
    char vol[16] = "", sub[MAX_PATH] = "", file[MAX_PATH], *s;
    DWORD n = sizeof vol, m = sizeof sub;
    const char *key = "Software\\SSSI\\Empire Earth";
    if (RegGetValueA(HKEY_CURRENT_USER, key, "Installed From Volume", RRF_RT_REG_SZ, NULL, vol, &n) != ERROR_SUCCESS ||
        RegGetValueA(HKEY_CURRENT_USER, key, "Installed From Directory", RRF_RT_REG_SZ, NULL, sub, &m) != ERROR_SUCCESS) {
        lstrcpynA(file, ini_path, sizeof file);
        if ((s = strrchr(file, '\\'))) s[1] = 0; else file[0] = 0;
        char path[MAX_PATH + 16];
        snprintf(path, sizeof path, "%seesettings.ini", file);
        GetPrivateProfileStringA("HKCU\\Software\\SSSI\\Empire Earth", "Installed From Volume", "", vol, sizeof vol, path);
        GetPrivateProfileStringA("HKCU\\Software\\SSSI\\Empire Earth", "Installed From Directory", "", sub, sizeof sub, path);
        for (char *r = sub, *w = sub; ; ) {
            if (r[0] == '\\' && r[1] == '\\') r++;
            if (!(*w++ = *r++)) break;
        }
    }
    if (!vol[0] || !sub[0]) return 0;
    snprintf(dir, cap, "%s%s", vol, sub);
    size_t len = strlen(dir);
    if (len && dir[len - 1] == '\\') dir[len - 1] = 0;
    return 1;
}

internal int theme_game_from(const char *dir) {   // both pictures from <dir>\Data\data.ssa: 1, else none (0)
    int w, h, ok = 0;
    if (art_open(dir)) {
        uint32_t *px = art_texture("uicontrols", &w, &h);
        if (px && w == 128 && h == 128) g_gl.ui = art_bitmap(px, w, h);
        free(px);
        if (g_gl.ui) g_gl.paint = art_picture("gamesettings", 3, 4, &g_gl.pw, &g_gl.ph);
        ok = g_gl.ui && g_gl.paint;
        if (!ok && g_gl.ui) { DeleteObject(g_gl.ui); g_gl.ui = NULL; }
    }
    art_close();
    return ok;
}

// the game's pictures from the folder the program is in (the game folder, where it is installed), else the one the
// ini's folder is in (<game>\ageofai\ageofai.ini), else the original game's (Art of Conquest's folder: its art isn't
// all there)
internal void theme_game_open(const char *ini_path) {
    char dir[MAX_PATH], *s;
    GetModuleFileNameA(NULL, dir, sizeof dir);
    if ((s = strrchr(dir, '\\'))) *s = 0;
    if (!theme_game_from(dir)) {
        lstrcpynA(dir, ini_path, sizeof dir);
        for (int up = 0; up < 2; up++) if ((s = strrchr(dir, '\\'))) *s = 0;
        if (!theme_game_from(dir) && !(theme_original_dir(ini_path, dir, sizeof dir) && theme_game_from(dir))) return;
    }
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
    // black fields, light text
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
    p->focus = RGB(255, 210, 60);   // the control with the keyboard: its text gold, as the game's menus mark theirs
    p->overruled = RGB(196, 140, 255);   // purple and teal, not red and green: told apart without red-green vision
    p->forced = RGB(70, 205, 185);
    HMODULE ux = LoadLibraryA("uxtheme.dll");
    typedef int (WINAPI *app_mode_fn)(int);   // SetPreferredAppMode: 2 always dark
    app_mode_fn mode = ux ? (app_mode_fn)(void *)GetProcAddress(ux, MAKEINTRESOURCEA(135)) : NULL;
    if (mode) mode(2);
    p->b_back = CreateSolidBrush(p->back);
    p->b_field = CreateSolidBrush(p->field);
}

// the window's background at its size (WM_SIZE): the painting over the whole of it, cut to its shape (no stretch
// out of proportion), and a copy of it darkened to under half, under the labels that lie on it
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
    g_gl.b_bg = CreatePatternBrush(bm);
    uint8_t *dim;
    HBITMAP dm = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, (void **)&dim, NULL, 0);
    if (g_gl.b_dim) DeleteObject(g_gl.b_dim);
    if (g_gl.dim) DeleteObject(g_gl.dim);
    g_gl.dim = dm;
    g_gl.b_dim = NULL;
    if (!dm) return;
    for (size_t i = 0; i < (size_t)cw * ch * 4; i++) dim[i] = (uint8_t)(bits[i] * 45 / 100);
    g_gl.b_dim = CreatePatternBrush(dm);
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

// a field's own colours (a dropdown drawn here, an edit): the program's, 1 when it sets them - ageofai_config's
// controls over a line of its list take the line's look; else the palette's
internal int (*theme_field_look)(HWND c, COLORREF *text, COLORREF *bk);

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
// the text white; a check box: the game's box, its text beside it. The keyboard's focus: the text gold (no dotted frame)
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
        theme_text(dc, c, &t, text, off ? g_pal.grey : focus ? g_pal.focus : g_pal.text, DT_LEFT);
        return;
    }
    theme_shade(dc, &r, down ? 200 : hot ? 105 : 160);
    theme_bevel(dc, &r, down);
    RECT t = r;
    if (down) OffsetRect(&t, 1, 1);
    theme_text(dc, c, &t, text, off ? g_pal.grey : focus ? g_pal.focus : g_pal.text, DT_CENTER);
}

// a dropdown as the game's: a black field in the sunken bevel, the text, and right after it the knot, a square of the
// whole height
internal void theme_paint_combo(HWND c, HDC dc) {
    RECT r;
    char text[256] = "";
    GetClientRect(c, &r);
    int sel = (int)SendMessageA(c, CB_GETCURSEL, 0, 0);
    if (sel >= 0 && SendMessageA(c, CB_GETLBTEXTLEN, sel, 0) < (LRESULT)sizeof text) SendMessageA(c, CB_GETLBTEXT, sel, (LPARAM)text);
    COLORREF color = g_pal.text, bk = g_pal.field;
    if (theme_field_look) theme_field_look(c, &color, &bk);
    int size = r.bottom;
    RECT f = { r.left, r.top, r.right - size, r.bottom };
    HBRUSH b = CreateSolidBrush(bk);
    FillRect(dc, &f, b);
    DeleteObject(b);
    theme_bevel(dc, &f, 1);
    theme_cell(dc, r.right - size, 0, size, GL_CELL_KNOT_DOWN - (SendMessageA(c, CB_GETDROPPEDSTATE, 0, 0) != 0));
    RECT t = { f.left + 4, f.top + 2, f.right - 2, f.bottom - 2 };   // ageofai_config's draw_field draws its faces' text here too
    int focus = GetFocus() == c && !SendMessageA(c, CB_GETDROPPEDSTATE, 0, 0);
    theme_text(dc, c, &t, text, !IsWindowEnabled(c) ? g_pal.grey : focus ? g_pal.focus : color, DT_LEFT);
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
// share shown, as the game's isn't). It is a window of its own lying over Windows' bar, the field drawing nothing
// under it (WS_CLIPSIBLINGS): Windows draws its bar at once whenever the scrolling changes (SetScrollInfo, a wheel's
// smooth scroll many times a turn), past any message a subclass could paint over it after - so it never reaches the
// screen. The knots and the track repeat while held, the ball dragged scrolls the field by lines (a list by its rows'
// height, an edit by EM_LINESCROLL); the wheel over the bar goes to the field.
#define GL_CELL_KNOT_UP 1, 2   // + 1 pressed (the brighter one unpressed, as the down knot: measured against the game)
#define GL_CELL_BALL 2, 4
#define SB_PROP "ageofai_sb"   // a field's bar window
enum { SB_PART_NONE, SB_PART_UP, SB_PART_DOWN, SB_PART_PAGE_UP, SB_PART_PAGE_DOWN, SB_PART_BALL };
internal int g_sb_part;   // the part the mouse holds (of the bar that has the capture)

internal int theme_sb_width(HWND c) {
    HDC dc = GetDC(c);
    int w = MulDiv(GL_CELL, GetDeviceCaps(dc, LOGPIXELSY), 96);
    ReleaseDC(c, dc);
    return w;
}

// the bar's place in the field's parent, 0 none shown: right of the client area, its height
internal int theme_sb_place(HWND c, RECT *bar) {
    if (!(GetWindowLongA(c, GWL_STYLE) & WS_VSCROLL) || !IsWindowVisible(c)) return 0;
    RECT cl;
    GetClientRect(c, &cl);
    MapWindowPoints(c, GetParent(c), (POINT *)&cl, 2);
    int width = theme_sb_width(c);
    SetRect(bar, cl.right, cl.top, cl.right + width, cl.bottom);
    return cl.bottom - cl.top >= 2 * width;
}

// the parts' places in a bar h high, w wide: the track's ends, the ball's top (-1: nothing to scroll)
internal void theme_sb_parts(HWND c, int w, int h, int *t0, int *t1, int *ball) {
    *t0 = w;
    *t1 = h - w;
    SCROLLINFO si = { sizeof si, SIF_ALL, 0, 0, 0, 0, 0 };
    GetScrollInfo(c, SB_VERT, &si);
    int range = si.nMax - si.nMin + 1 - (int)si.nPage, room = *t1 - *t0 - w;
    *ball = range > 0 ? *t0 + (int)((long long)(si.nPos - si.nMin) * (room > 0 ? room : 0) / range) : -1;
}

internal int theme_sb_part(HWND bar, HWND c, int y) {   // which part of the bar a point (its y in the bar) is on
    RECT r;
    int t0, t1, ball;
    GetClientRect(bar, &r);
    theme_sb_parts(c, r.right, r.bottom, &t0, &t1, &ball);
    if (y < t0) return SB_PART_UP;
    if (y >= t1) return SB_PART_DOWN;
    if (ball < 0) return SB_PART_NONE;
    if (y < ball) return SB_PART_PAGE_UP;
    return y >= ball + r.right ? SB_PART_PAGE_DOWN : SB_PART_BALL;
}

internal void theme_sb_lines(HWND c, int lines) {   // the field scrolled by lines (the ball dragged)
    char cls[32] = "";
    GetClassNameA(c, cls, sizeof cls);
    if (!lstrcmpiA(cls, WC_LISTVIEWA)) {
        RECT r = { LVIR_BOUNDS, 0, 0, 0 };
        int row = SendMessageA(c, LVM_GETITEMRECT, 0, (LPARAM)&r) ? r.bottom - r.top : 16;
        SendMessageA(c, LVM_SCROLL, 0, lines * row);
    } else SendMessageA(c, EM_LINESCROLL, 0, lines);
}

internal void theme_sb_draw(HWND bar, HDC dc) {
    HWND c = (HWND)GetWindowLongPtrA(bar, GWLP_USERDATA);
    RECT r;
    int t0, t1, ball, held = GetCapture() == bar ? g_sb_part : SB_PART_NONE;
    GetClientRect(bar, &r);
    int w = r.right, h = r.bottom;
    theme_sb_parts(c, w, h, &t0, &t1, &ball);
    HDC mem = CreateCompatibleDC(dc);
    HBITMAP bm = CreateCompatibleBitmap(dc, w, h), old = (HBITMAP)SelectObject(mem, bm);
    RECT track = { 0, t0, w, t1 };
    FillRect(mem, &track, g_pal.b_field);
    theme_bevel(mem, &track, 1);
    theme_cell(mem, 0, 0, w, GL_CELL_KNOT_UP + (held == SB_PART_UP));
    theme_cell(mem, 0, h - w, w, GL_CELL_KNOT_DOWN - (held == SB_PART_DOWN));
    if (ball >= 0) theme_cell(mem, 0, ball, w, GL_CELL_BALL);
    BitBlt(dc, 0, 0, w, h, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteObject(bm);
    DeleteDC(mem);
}

// the mouse held on the bar until it is let go: the knots and the track act at once and repeat (after 400 ms, then
// every 50) while the mouse stays on them, the ball follows the mouse
internal void theme_sb_track(HWND bar, HWND c, int part, int y0) {
    RECT r;
    int t0, t1, ball;
    GetClientRect(bar, &r);
    theme_sb_parts(c, r.right, r.bottom, &t0, &t1, &ball);
    int grab = y0 - ball, width = r.right;
    g_sb_part = part;
    SetCapture(bar);
    DWORD next = GetTickCount();
    for (int first = 1; GetCapture() == bar; first = 0) {
        POINT pt;
        GetCursorPos(&pt);
        ScreenToClient(bar, &pt);
        if (part == SB_PART_BALL) {
            SCROLLINFO si = { sizeof si, SIF_ALL, 0, 0, 0, 0, 0 };
            GetScrollInfo(c, SB_VERT, &si);
            int range = si.nMax - si.nMin + 1 - (int)si.nPage, room = t1 - t0 - width;
            if (range > 0 && room > 0) {
                int want = si.nMin + (int)(((long long)(pt.y - grab - t0) * range + room / 2) / room);
                if (want < si.nMin) want = si.nMin;
                if (want > si.nMin + range) want = si.nMin + range;
                if (want != si.nPos) theme_sb_lines(c, want - si.nPos);
            }
        } else if (GetTickCount() >= next) {
            int code = part == SB_PART_UP ? SB_LINEUP : part == SB_PART_DOWN ? SB_LINEDOWN : part == SB_PART_PAGE_UP ? SB_PAGEUP : SB_PAGEDOWN;
            if (first || (PtInRect(&r, pt) && theme_sb_part(bar, c, pt.y) == part))   // a page: on that side of the ball
                SendMessageA(c, WM_VSCROLL, MAKEWPARAM(code, 0), 0);
            next = GetTickCount() + (first ? 400 : 50);
        }
        InvalidateRect(bar, NULL, FALSE);
        UpdateWindow(bar);
        MSG m;
        while (PeekMessageA(&m, NULL, 0, 0, PM_REMOVE)) {
            if (m.message == WM_LBUTTONUP || (m.message == WM_KEYDOWN && m.wParam == VK_ESCAPE)) { ReleaseCapture(); break; }
            if (m.message == WM_MOUSEMOVE || m.message == WM_LBUTTONDOWN) continue;
            TranslateMessage(&m);
            DispatchMessageA(&m);
        }
        if (GetCapture() == bar) MsgWaitForMultipleObjects(0, NULL, FALSE, 15, QS_ALLINPUT);
    }
    if (GetCapture() == bar) ReleaseCapture();
    SendMessageA(c, WM_VSCROLL, MAKEWPARAM(SB_ENDSCROLL, 0), 0);
    InvalidateRect(bar, NULL, FALSE);
}

internal LRESULT CALLBACK theme_sb_proc(HWND bar, UINT msg, WPARAM wp, LPARAM lp) {
    HWND c = (HWND)GetWindowLongPtrA(bar, GWLP_USERDATA);
    switch (msg) {
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(bar, &ps);
        if (c) theme_sb_draw(bar, dc);
        EndPaint(bar, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: {
        int y = (short)HIWORD(lp), part = c ? theme_sb_part(bar, c, y) : SB_PART_NONE;
        if (part != SB_PART_NONE) theme_sb_track(bar, c, part, y);
        return 0;
    }
    case WM_MOUSEWHEEL: if (c) return SendMessageA(c, msg, wp, lp); break;
    case WM_MOUSEACTIVATE: return MA_NOACTIVATE;
    }
    return DefWindowProcA(bar, msg, wp, lp);
}

// the field's bar window at its place and painted anew, or hidden; made the first time
internal void theme_sb_sync(HWND c) {
    HWND bar = (HWND)GetPropA(c, SB_PROP);
    RECT want, now;
    if (!bar) {
        static int registered;
        if (!registered) {
            WNDCLASSA wc = { 0 };
            wc.style = CS_DBLCLKS;
            wc.lpfnWndProc = theme_sb_proc;
            wc.hInstance = GetModuleHandleA(NULL);
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.lpszClassName = "AgeOfAIScrollBar";
            RegisterClassA(&wc);
            registered = 1;
        }
        bar = CreateWindowExA(WS_EX_NOPARENTNOTIFY, "AgeOfAIScrollBar", "", WS_CHILD, 0, 0, 0, 0, GetParent(c), NULL,
                              GetModuleHandleA(NULL), NULL);
        if (!bar) return;
        SetWindowLongPtrA(bar, GWLP_USERDATA, (LONG_PTR)c);
        SetPropA(c, SB_PROP, bar);
    }
    if (!theme_sb_place(c, &want)) {
        if (IsWindowVisible(bar)) ShowWindow(bar, SW_HIDE);
        return;
    }
    GetWindowRect(bar, &now);
    MapWindowPoints(NULL, GetParent(c), (POINT *)&now, 2);
    if (!EqualRect(&want, &now) || !IsWindowVisible(bar))
        SetWindowPos(bar, HWND_TOP, want.left, want.top, want.right - want.left, want.bottom - want.top, SWP_NOACTIVATE | SWP_SHOWWINDOW);
    InvalidateRect(bar, NULL, FALSE);
    UpdateWindow(bar);
}

// a field's messages for its bar: the client area narrower for the wider bar (1: handled, *res the result)
internal int theme_sb_before(HWND c, UINT msg, WPARAM wp, LPARAM lp, LRESULT *res) {
    if (!g_gl.on || msg != WM_NCCALCSIZE || !(GetWindowLongA(c, GWL_STYLE) & WS_VSCROLL)) return 0;
    *res = DefSubclassProc(c, msg, wp, lp);
    RECT *r = (RECT *)lp;
    int more = theme_sb_width(c) - GetSystemMetrics(SM_CXVSCROLL);
    if (more > 0 && r->right - more > r->left) r->right -= more;
    return 1;
}

// the message through; the bar after it at its place and painted when the scrolling, the size or the showing changed
internal LRESULT theme_sb_after(HWND c, UINT msg, WPARAM wp, LPARAM lp) {
    if (!g_gl.on) return DefSubclassProc(c, msg, wp, lp);
    if (msg == WM_NCDESTROY) {
        HWND bar = (HWND)RemovePropA(c, SB_PROP);
        if (bar) DestroyWindow(bar);
        return DefSubclassProc(c, msg, wp, lp);
    }
    SCROLLINFO a = { sizeof a, SIF_ALL, 0, 0, 0, 0, 0 }, b = a;
    LONG style = GetWindowLongA(c, GWL_STYLE);
    GetScrollInfo(c, SB_VERT, &a);
    LRESULT res = DefSubclassProc(c, msg, wp, lp);
    GetScrollInfo(c, SB_VERT, &b);
    if (msg == WM_WINDOWPOSCHANGED || msg == WM_SIZE || a.nPos != b.nPos || a.nPage != b.nPage || a.nMax != b.nMax ||
        a.nMin != b.nMin || style != GetWindowLongA(c, GWL_STYLE)) theme_sb_sync(c);
    return res;
}

// THE TEXT PANEL (the game's look): text to read - the help, the game page's report - not in a field, which looks like
// one to type in, but as the game's menus frame a group: the painting darkened under it, a frame of three lines (dark,
// the light of the bevel, its dark) and the text over it. Scrolled by lines when it is longer than the panel - the
// wheel, the game's scrollbar (WS_VSCROLL set by SetScrollInfo only then: none while the text fits). Its first
// characters can be a lead in a colour of their own (PANEL_LEAD before the text: the editors' warnings and mistakes over
// the help), a blank line between it and the rest.
#define PANEL_CLASS "AgeOfAIText"
#define PANEL_LEAD (WM_APP + 1)   // wParam the lead's colour, lParam its length (0: none); another class ignores it
struct text_panel { HFONT font; COLORREF lead_color; int lead, top, line, lines, page, busy; };

internal struct text_panel *theme_panel_of(HWND w) { return (struct text_panel *)GetWindowLongPtrA(w, GWLP_USERDATA); }

internal void theme_panel_inner(HWND w, RECT *r) {   // where the text goes: inside the frame and a margin
    GetClientRect(w, r);
    InflateRect(r, -8, -6);
}

// the lead and the rest from at's top down (calc: only measured), the height they take
internal int theme_panel_draw(struct text_panel *t, HDC dc, const char *text, RECT at, int calc) {
    UINT how = DT_WORDBREAK | DT_EDITCONTROL | DT_NOPREFIX | DT_EXPANDTABS | (calc ? DT_CALCRECT : 0);
    int n = (int)strlen(text), lead = t->lead < n ? t->lead : n, y = at.top;
    const char *rest = text + lead;
    while (*rest == '\r' || *rest == '\n') rest++;
    if (lead) {
        RECT r = at;
        SetTextColor(dc, t->lead_color);
        DrawTextA(dc, text, lead, &r, how | DT_CALCRECT);
        if (!calc) { r.right = at.right; DrawTextA(dc, text, lead, &r, how); }
        y = r.bottom + (*rest ? t->line : 0);
    }
    if (*rest) {
        RECT r = at;
        r.top = y;
        SetTextColor(dc, g_pal.text);
        DrawTextA(dc, rest, -1, &r, how | DT_CALCRECT);
        if (!calc) { r.right = at.right; DrawTextA(dc, rest, -1, &r, how); }
        y = r.bottom;
    }
    return y - at.top;
}

// the text's lines at the panel's width, the page, the scrolling; again when the bar comes or goes (the width changes)
internal void theme_panel_layout(HWND w) {
    struct text_panel *t = theme_panel_of(w);
    if (!t || t->busy) return;
    t->busy = 1;
    for (int pass = 0; pass < 2; pass++) {
        RECT in;
        theme_panel_inner(w, &in);
        int n = GetWindowTextLengthA(w);
        char *text = malloc(n + 1);
        if (!text) break;
        GetWindowTextA(w, text, n + 1);
        HDC dc = GetDC(w);
        HFONT old = (HFONT)SelectObject(dc, t->font ? t->font : GetStockObject(DEFAULT_GUI_FONT));
        TEXTMETRICA tm;
        GetTextMetricsA(dc, &tm);
        t->line = tm.tmHeight > 0 ? tm.tmHeight : 16;
        RECT r = { 0, 0, in.right - in.left > 1 ? in.right - in.left : 1, 0 };
        int height = theme_panel_draw(t, dc, text, r, 1);
        SelectObject(dc, old);
        ReleaseDC(w, dc);
        free(text);
        t->lines = (height + t->line - 1) / t->line;
        t->page = (in.bottom - in.top) / t->line;
        if (t->page < 1) t->page = 1;
        int last = t->lines - t->page;
        if (t->top > last) t->top = last;
        if (t->top < 0) t->top = 0;
        SCROLLINFO si = { sizeof si, SIF_RANGE | SIF_PAGE | SIF_POS, 0, t->lines - 1, (UINT)t->page, t->top, 0 };
        LONG before = GetWindowLongA(w, GWL_STYLE) & WS_VSCROLL;
        SetScrollInfo(w, SB_VERT, &si, TRUE);
        if ((GetWindowLongA(w, GWL_STYLE) & WS_VSCROLL) == before) break;
    }
    t->busy = 0;
    InvalidateRect(w, NULL, FALSE);
}

internal void theme_panel_scroll(HWND w, int to) {   // the first line shown
    struct text_panel *t = theme_panel_of(w);
    int last = t->lines - t->page;
    if (to > last) to = last;
    if (to < 0) to = 0;
    if (to == t->top) return;
    t->top = to;
    SetScrollPos(w, SB_VERT, to, TRUE);
    InvalidateRect(w, NULL, FALSE);
}

internal void theme_panel_paint(HWND w, HDC dc) {
    struct text_panel *t = theme_panel_of(w);
    RECT r, in;
    GetClientRect(w, &r);
    theme_under(dc, w, &r);
    theme_shade(dc, &r, 185);
    HBRUSH dark = CreateSolidBrush(RGB(20, 20, 20)), light = CreateSolidBrush(RGB(176, 194, 193)), mid = CreateSolidBrush(RGB(89, 101, 100));
    RECT f = r;
    FrameRect(dc, &f, dark);
    InflateRect(&f, -1, -1);
    FrameRect(dc, &f, light);
    InflateRect(&f, -1, -1);
    FrameRect(dc, &f, mid);
    DeleteObject(dark);
    DeleteObject(light);
    DeleteObject(mid);
    int n = GetWindowTextLengthA(w);
    char *text = malloc(n + 1);
    if (!text) return;
    GetWindowTextA(w, text, n + 1);
    theme_panel_inner(w, &in);
    HRGN clip = CreateRectRgnIndirect(&in);
    SelectClipRgn(dc, clip);
    RECT tr = in;
    tr.top -= t->top * t->line;
    tr.bottom = 0x3FFFFFFF;
    HFONT old = (HFONT)SelectObject(dc, t->font ? t->font : GetStockObject(DEFAULT_GUI_FONT));
    SetBkMode(dc, TRANSPARENT);
    theme_panel_draw(t, dc, text, tr, 0);
    SelectObject(dc, old);
    SelectClipRgn(dc, NULL);
    DeleteObject(clip);
    free(text);
}

internal LRESULT CALLBACK theme_panel_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp) {
    struct text_panel *t = theme_panel_of(w);
    switch (msg) {
    case WM_NCCREATE:
        SetWindowLongPtrA(w, GWLP_USERDATA, (LONG_PTR)calloc(1, sizeof(struct text_panel)));
        break;
    case WM_NCDESTROY: free(t); SetWindowLongPtrA(w, GWLP_USERDATA, 0); break;
    case WM_SETFONT: if (t) { t->font = (HFONT)wp; theme_panel_layout(w); } return 0;
    case WM_GETFONT: return t ? (LRESULT)t->font : 0;
    case PANEL_LEAD: if (t) { t->lead_color = (COLORREF)wp; t->lead = (int)lp; } return 0;
    case WM_SETTEXT: {
        LRESULT res = DefWindowProcA(w, msg, wp, lp);
        if (t) { t->top = 0; theme_panel_layout(w); }
        return res;
    }
    case WM_SIZE: theme_panel_layout(w); return 0;
    case WM_ERASEBKGND: return 1;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        RECT r;
        HDC dc = BeginPaint(w, &ps);
        GetClientRect(w, &r);
        HDC mem = CreateCompatibleDC(dc);
        HBITMAP bm = CreateCompatibleBitmap(dc, r.right, r.bottom), old = (HBITMAP)SelectObject(mem, bm);
        if (t) theme_panel_paint(w, mem);
        BitBlt(dc, 0, 0, r.right, r.bottom, mem, 0, 0, SRCCOPY);
        SelectObject(mem, old);
        DeleteObject(bm);
        DeleteDC(mem);
        EndPaint(w, &ps);
        return 0;
    }
    case WM_VSCROLL: {
        if (!t) return 0;
        int to = t->top;
        switch (LOWORD(wp)) {
        case SB_LINEUP: to--; break;
        case SB_LINEDOWN: to++; break;
        case SB_PAGEUP: to -= t->page; break;
        case SB_PAGEDOWN: to += t->page; break;
        case SB_TOP: to = 0; break;
        case SB_BOTTOM: to = t->lines; break;
        case SB_THUMBTRACK: case SB_THUMBPOSITION: to = HIWORD(wp); break;
        }
        theme_panel_scroll(w, to);
        return 0;
    }
    case EM_LINESCROLL: if (t) theme_panel_scroll(w, t->top + (int)lp); return TRUE;   // the game's scrollbar's ball
    case WM_MOUSEWHEEL: {
        UINT n = 3;
        SystemParametersInfoA(SPI_GETWHEELSCROLLLINES, 0, &n, 0);
        if (t) theme_panel_scroll(w, t->top - (short)HIWORD(wp) * (int)n / WHEEL_DELTA);
        return 0;
    }
    }
    return DefWindowProcA(w, msg, wp, lp);
}

// the class a text to read is made of: the panel in the game's look, else a read-only field
internal const char *theme_text_class(void) {
    if (!g_gl.on) return WC_EDITA;
    static int registered;
    if (!registered) {
        WNDCLASSA wc = { 0 };
        wc.lpfnWndProc = theme_panel_proc;
        wc.hInstance = GetModuleHandleA(NULL);
        wc.hCursor = LoadCursor(NULL, IDC_ARROW);
        wc.lpszClassName = PANEL_CLASS;
        RegisterClassA(&wc);
        registered = 1;
    }
    return PANEL_CLASS;
}

internal LRESULT CALLBACK theme_scroll_proc(HWND c, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR data) {
    (void)id; (void)data;   // the panel's bar: as a field's, without a field's frame
    LRESULT res;
    if (theme_sb_before(c, msg, wp, lp, &res)) return res;
    return theme_sb_after(c, msg, wp, lp);
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

internal void theme_control(HWND c, const char *cls) {   // a child: its dark theme
    if (!lstrcmpA(cls, PANEL_CLASS)) {   // the game's look's text panel: its bar
        SetWindowSubclass(c, theme_scroll_proc, 1, 0);
        SetWindowLongA(c, GWL_STYLE, GetWindowLongA(c, GWL_STYLE) | WS_CLIPSIBLINGS);
        return;
    }
    if (!lstrcmpA(cls, WC_LISTVIEWA)) {
        HWND h = ListView_GetHeader(c);
        theme_allow_dark(c);
        SetWindowTheme(c, L"DarkMode_Explorer", NULL);
        theme_allow_dark(h);
        SetWindowTheme(h, L"ItemsView", NULL);
        SetWindowSubclass(c, theme_list_proc, 1, 0);
        if (g_gl.on) SetWindowLongA(c, GWL_STYLE, GetWindowLongA(c, GWL_STYLE) | WS_CLIPSIBLINGS);   // nothing drawn under its bar (theme_sb_sync)
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
        SetWindowLongA(c, GWL_STYLE, GetWindowLongA(c, GWL_STYLE) | WS_CLIPSIBLINGS);   // nothing drawn under its bar (theme_sb_sync)
        SetWindowPos(c, NULL, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_FRAMECHANGED);
    }
}

// the WM_CTLCOLOR* messages: the field's black for edits, lists and a read-only edit (it asks as a static);
// the rest the window's grey, or in the game's look the darkened painting under it (the brush's origin at the child's place:
// the child fills with its part of it); an edit with a look of its own (theme_field_look) that
internal LRESULT theme_ctlcolor(UINT msg, WPARAM wp, LPARAM lp) {
    static HBRUSH own;   // the brush of the last look of its own
    static COLORREF own_bk;
    COLORREF text, bk;
    if (msg == WM_CTLCOLOREDIT && theme_field_look && theme_field_look((HWND)lp, &text, &bk)) {
        if (!own || own_bk != bk) {
            if (own) DeleteObject(own);
            own = CreateSolidBrush(bk);
            own_bk = bk;
        }
        SetTextColor((HDC)wp, text);
        SetBkColor((HDC)wp, bk);
        return (LRESULT)own;
    }
    char cls[16] = "";
    GetClassNameA((HWND)lp, cls, sizeof cls);
    int field = msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORLISTBOX || !lstrcmpiA(cls, "Edit");
    SetTextColor((HDC)wp, g_pal.text);
    SetBkColor((HDC)wp, field ? g_pal.field : g_pal.back);
    if (field) return (LRESULT)g_pal.b_field;
    if (g_gl.on && g_gl.b_dim && GetParent((HWND)lp) == g_gl.main) {
        POINT pt = theme_at((HWND)lp);
        SetBkMode((HDC)wp, TRANSPARENT);
        SetBrushOrgEx((HDC)wp, -pt.x, -pt.y, NULL);
        return (LRESULT)g_gl.b_dim;
    }
    return (LRESULT)g_pal.b_back;
}
