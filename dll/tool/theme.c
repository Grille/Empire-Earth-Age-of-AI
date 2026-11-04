// THE LOOK: light or dark as Windows' apps are (Settings - Personalization - Colors: "Choose your default app mode"),
// read once at the start. The colours the program draws with are g_pal's, never GetSysColor's, so both modes keep
// the list's meanings (grey the default, blue the game's own, the warning and mistake tints) readable - a dark
// colour loses its hue on white, a light one keeps it on black. Dark: Windows' own dark themes where its controls
// have one (DarkMode_Explorer: buttons, the list, scrollbars; DarkMode_CFD: edit fields and dropdowns - the names
// Explorer and the file dialog use), the rest by the WM_CTLCOLOR* messages; a check box is drawn classic, the one
// control whose themed text Windows keeps black. The calls that switch the app's menus and scrollbars to dark
// (uxtheme's ordinals 133 and 135) have no names: looked up by number, left out where the Windows has none.
#include <uxtheme.h>
#include <dwmapi.h>

struct palette {
    int dark;
    COLORREF back, field, text, grey, game, game_grey, tint_warn, tint_mistake, text_warn, text_mistake;
    HBRUSH b_back, b_field;
};
internal struct palette g_pal;

internal int theme_windows_dark(void) {   // Windows' app mode: 1 dark
    DWORD light = 1, n = sizeof light;
    if (RegGetValueA(HKEY_CURRENT_USER, "Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
                     "AppsUseLightTheme", RRF_RT_REG_DWORD, NULL, &light, &n) != ERROR_SUCCESS) return 0;
    return light == 0;
}

internal void theme_init(void) {
    struct palette *p = &g_pal;
    p->dark = theme_windows_dark();
    if (p->dark) {   // the game's own look: black fields, light text
        p->back = RGB(32, 32, 32);
        p->field = RGB(18, 18, 18);
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

// a header's text in the palette's colour: the dark header theme draws it black (the list's subclass)
internal LRESULT CALLBACK theme_list_proc(HWND w, UINT msg, WPARAM wp, LPARAM lp, UINT_PTR id, DWORD_PTR data) {
    (void)id; (void)data;
    if (msg == WM_NOTIFY && ((NMHDR *)lp)->code == NM_CUSTOMDRAW && ((NMHDR *)lp)->hwndFrom == ListView_GetHeader(w)) {
        NMCUSTOMDRAW *cd = (NMCUSTOMDRAW *)lp;
        if (cd->dwDrawStage == CDDS_PREPAINT) return CDRF_NOTIFYITEMDRAW;
        if (cd->dwDrawStage == CDDS_ITEMPREPAINT) { SetTextColor(cd->hdc, g_pal.text); return CDRF_DODEFAULT; }
    }
    return DefSubclassProc(w, msg, wp, lp);
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
        ListView_SetBkColor(c, g_pal.field);
        ListView_SetTextBkColor(c, g_pal.field);
        ListView_SetTextColor(c, g_pal.text);
        return;
    }
    if (!lstrcmpA(cls, WC_BUTTONA) && (GetWindowLongA(c, GWL_STYLE) & BS_TYPEMASK) == BS_AUTOCHECKBOX) {
        SetWindowTheme(c, L" ", L" ");   // classic: its text in WM_CTLCOLORSTATIC's colour
        return;
    }
    theme_allow_dark(c);
    SetWindowTheme(c, !lstrcmpA(cls, WC_COMBOBOXA) || !lstrcmpA(cls, WC_EDITA) ? L"DarkMode_CFD" : L"DarkMode_Explorer", NULL);
}

// the WM_CTLCOLOR* messages in dark: the field's black for edits, lists and a read-only edit (it asks as a static),
// the window's grey for the rest; 0 in light (Windows' own)
internal LRESULT theme_ctlcolor(UINT msg, WPARAM wp, LPARAM lp) {
    if (!g_pal.dark) return 0;
    char cls[16] = "";
    GetClassNameA((HWND)lp, cls, sizeof cls);
    int field = msg == WM_CTLCOLOREDIT || msg == WM_CTLCOLORLISTBOX || !lstrcmpiA(cls, "Edit");
    SetTextColor((HDC)wp, g_pal.text);
    SetBkColor((HDC)wp, field ? g_pal.field : g_pal.back);
    return (LRESULT)(field ? g_pal.b_field : g_pal.b_back);
}
