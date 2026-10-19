// HEADLESS TEST WINDOWS: nothing a test game creates may take the focus or show on the taskbar. Measured (a watcher
// on the foreground and the shell's window events): every start activated windows ~22 times in ~7 s - the display
// DLLs' 16x16 "TestWindowClass" probes (6 of them before the engine loads eeaimod.dll, 15 after) and the game window,
// each also with a taskbar button for a moment; WS_EX_NOACTIVATE alone doesn't stop a window created visible from
// being activated. A thread CBT hook sees every window of the thread before it shows: no taskbar button
// (WS_EX_TOOLWINDOW), off-screen (layered windows - DDrawCompat's presentation window - stay where they are), and
// every activation refused. Installed by dinput.dll (eesettings.c) at the process start when run_test.ps1 sets
// EEAIMOD_HEADLESS=1 - before the display DLLs load - and by eeaimod.dll with headless=1 (display.c), for a game
// without the mod's dinput.dll. Included by both.
static HHOOK g_hl_cbt;
static LRESULT CALLBACK hl_cbt(int code, WPARAM w, LPARAM l) {
    if (code == HCBT_CREATEWND) {
        CREATESTRUCTA *cs = ((CBT_CREATEWNDA *)l)->lpcs;
        if (!cs->hwndParent) {   // top-level (a child, an owned window or a message-only one has a parent here)
            LONG ex = (GetWindowLongA((HWND)w, GWL_EXSTYLE) | WS_EX_NOACTIVATE | WS_EX_TOOLWINDOW) & ~WS_EX_APPWINDOW;
            SetWindowLongA((HWND)w, GWL_EXSTYLE, ex);
            cs->dwExStyle = ex;
            if (!(ex & WS_EX_LAYERED)) { cs->x = GetSystemMetrics(SM_XVIRTUALSCREEN) - 8000; cs->y = 0; }
        }
    } else if (code == HCBT_ACTIVATE) return 1;   // refused
    return CallNextHookEx(g_hl_cbt, code, w, l);
}
static void headless_windows(void) {   // this thread's windows (the engine's: it loads the DLLs and creates the windows)
    if (!g_hl_cbt) g_hl_cbt = SetWindowsHookExA(WH_CBT, hl_cbt, NULL, GetCurrentThreadId());
}
