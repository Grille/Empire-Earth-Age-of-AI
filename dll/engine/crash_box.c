// THE CRASH BOX ([general] crash_dialog; the player, 2026-11-14: "right now the game exits when crashing, makes sence for
// nomral play. but for us it might be usefull if the process stays open and show an exception dialog?"). An exception
// nothing handles ends at the top-level filter (kernel32's UnhandledExceptionFilter: the C runtime's frame round the
// main thread and every thread's start ask it), and the process ended without a word. The mod's filter comes first
// (on by default since the player, 2026-11-14: "the game just closing has no value to anybody"): a box with the
// exception - where (module + offset), what was read or written, the call chain by frame pointers - and two buttons
// ("it than should have clerly explained options: a "dump and close" and "close" button"): Dump and close writes a
// memory dump of the process at the crash (ageofai_dumps\crash_<date>_<time>.dmp, the crashing thread's context in it -
// Visual Studio or WinDbg open it at the faulting instruction), Close doesn't; a line in the log either way. The
// process stays as it is while the box is open - its other threads run on, a debugger can attach. Then: with a debugger
// attached the faulting instruction runs again and the debugger stops at it; without one the process ends with the
// exception's code, as a crash ended before. Not through the filter there was: Miles Sound's 3D module (Mssrsx.m3d) sets one at a
// game start, and called after the box it hung the game (a test game: killed at its time limit instead of ending). The
// work is done on a thread of its own (a stack overflow leaves the crashing thread a few KB), the crashing one waits for
// it; a second crash while the box is open waits for good. Ours is put in front again at each game start (Miles's set
// since; the exe's import of SetUnhandledExceptionFilter, where a build has one, is hooked too). Never in test runs
// (headless: nobody to close the box - the test server would take the stopped clock for a hang).
#include <dbghelp.h>

internal LPTOP_LEVEL_EXCEPTION_FILTER g_cb_next;   // the filter ours replaced or kept out (returned to a SetUnhandledExceptionFilter call; never called)
internal volatile LONG g_cb_busy;
internal EXCEPTION_POINTERS *g_cb_ep;
internal DWORD g_cb_thread;
internal int g_cb_debugger;   // a debugger attached when the box was closed
internal char g_cb_text[3000], g_cb_dump[MAX_PATH], g_cb_why[96];   // why: no dump, why not

internal const char *cb_code_name(DWORD code) {
    switch (code) {
    case 0xC0000005: return "access violation";
    case 0xC00000FD: return "stack overflow";
    case 0xC000001D: return "illegal instruction";
    case 0xC0000094: return "integer division by zero";
    case 0xC0000095: return "integer overflow";
    case 0xC000008E: case 0xC0000090: case 0xC0000091: return "floating point error";
    case 0xC0000096: return "privileged instruction";
    case 0xC0000006: return "in-page error";
    case 0xC0000409: return "stack buffer overrun";
    case 0x80000003: return "breakpoint";
    default: return "exception";
    }
}
// the dump at the crash: the threads, their stacks, the modules' data, what the stacks point at
internal int cb_write_dump(void) {
    typedef BOOL (WINAPI *mdw_fn)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION,
                                  PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);
    HMODULE dh = LoadLibraryA("dbghelp.dll");
    mdw_fn mdw = dh ? (mdw_fn)(void *)GetProcAddress(dh, "MiniDumpWriteDump") : NULL;
    if (!mdw || !g_dump_dir[0]) { snprintf(g_cb_why, sizeof g_cb_why, !mdw ? "no dbghelp.dll" : "no dump folder"); return 0; }
    SYSTEMTIME t;
    GetLocalTime(&t);
    CreateDirectoryA(g_dump_dir, NULL);
    size_t dl = strlen(g_dump_dir);
    snprintf(g_cb_dump, sizeof g_cb_dump, "%s%scrash_%04u%02u%02u_%02u%02u%02u.dmp", g_dump_dir,
             g_dump_dir[dl - 1] == '\\' || g_dump_dir[dl - 1] == '/' ? "" : "\\", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    // from inside the process the fuller kinds can fail on memory they can't read (8007012b, a test game's crash): each
    // kind in turn down to the plain one (the threads' stacks and the modules)
    static const DWORD kinds[3] = { MiniDumpWithDataSegs | MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithUnloadedModules | MiniDumpWithThreadInfo,
                                    MiniDumpWithDataSegs | MiniDumpWithUnloadedModules, MiniDumpNormal };
    MINIDUMP_EXCEPTION_INFORMATION mei = { g_cb_thread, g_cb_ep, FALSE };
    DWORD err = 0;
    for (int k = 0; k < 3; k++) {
        HANDLE f = CreateFileA(g_cb_dump, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (f == INVALID_HANDLE_VALUE) { snprintf(g_cb_why, sizeof g_cb_why, "the file not made (error %lu)", GetLastError()); return 0; }
        BOOL ok = mdw(GetCurrentProcess(), GetCurrentProcessId(), f, (MINIDUMP_TYPE)kinds[k], &mei, NULL, NULL);
        if (!ok) err = GetLastError();
        CloseHandle(f);
        if (ok) { snprintf(g_cb_why, sizeof g_cb_why, "%s", k == 0 ? "full" : k == 1 ? "without the memory the stacks point at" : "stacks only"); return 1; }
        DeleteFileA(g_cb_dump);
    }
    snprintf(g_cb_why, sizeof g_cb_why, "MiniDumpWriteDump failed (error %08lx)", err);
    return 0;
}
// the box's buttons (the message box's Yes and No) named for what they do and wide enough for it, as it opens
internal HHOOK g_cb_hook;
internal LRESULT CALLBACK cb_cbt(int code, WPARAM wp, LPARAM lp) {
    if (code == HCBT_ACTIVATE) {
        HWND w = (HWND)wp, yes = GetDlgItem(w, IDYES), no = GetDlgItem(w, IDNO);
        if (yes && no) {
            static const char dump_text[] = "Dump and close";
            SetWindowTextA(yes, dump_text);
            SetWindowTextA(no, "Close");
            RECT ry, rn, cr;
            GetWindowRect(yes, &ry); GetWindowRect(no, &rn);
            MapWindowPoints(NULL, w, (POINT *)&ry, 2); MapWindowPoints(NULL, w, (POINT *)&rn, 2);
            HDC dc = GetDC(yes);
            HFONT f = (HFONT)SendMessageA(yes, WM_GETFONT, 0, 0);
            HGDIOBJ of = f ? SelectObject(dc, f) : NULL;
            SIZE sz = { 0, 0 };
            GetTextExtentPoint32A(dc, dump_text, (int)sizeof dump_text - 1, &sz);
            if (of) SelectObject(dc, of);
            ReleaseDC(yes, dc);
            int bw = sz.cx + 28, bh = ry.bottom - ry.top, gap = rn.left - ry.right, margin = 12;
            if (bw < ry.right - ry.left) bw = ry.right - ry.left;
            if (gap < 6) gap = 6;
            GetClientRect(w, &cr);
            int need = 2 * bw + gap + 2 * margin;
            if (need > cr.right) {   // the box wider for them
                RECT wr;
                GetWindowRect(w, &wr);
                SetWindowPos(w, NULL, 0, 0, wr.right - wr.left + need - cr.right, wr.bottom - wr.top, SWP_NOMOVE | SWP_NOZORDER);
                GetClientRect(w, &cr);
            }
            int x = cr.right - margin - (2 * bw + gap);   // at the right, as the box's own
            MoveWindow(yes, x, ry.top, bw, bh, TRUE);
            MoveWindow(no, x + bw + gap, rn.top, bw, bh, TRUE);
        }
    }
    return CallNextHookEx(NULL, code, wp, lp);
}
internal DWORD WINAPI cb_box_thread(void *arg) {
    (void)arg;
    EXCEPTION_RECORD *er = g_cb_ep->ExceptionRecord;
    CONTEXT *c = g_cb_ep->ContextRecord;
    char at[160];
    crash_addr(at, sizeof at, c->Eip);
    int n = snprintf(g_cb_text, sizeof g_cb_text, "The game crashed: %s (%08lx) at %s, thread %lu", cb_code_name(er->ExceptionCode),
                     er->ExceptionCode, at, g_cb_thread);
    if (er->ExceptionCode == 0xC0000005 && er->NumberParameters >= 2)
        n += snprintf(g_cb_text + n, sizeof g_cb_text - n, " - %s %08lx", er->ExceptionInformation[0] == 8 ? "executing"
                      : er->ExceptionInformation[0] ? "writing" : "reading", (unsigned long)er->ExceptionInformation[1]);
    n += snprintf(g_cb_text + n, sizeof g_cb_text - n, ".\n\neax %08lx ebx %08lx ecx %08lx edx %08lx\nesi %08lx edi %08lx ebp %08lx esp %08lx\n\nCalled from:",
                  c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp);
    uintptr_t *fp = (uintptr_t *)c->Ebp;
    for (int d = 0; d < 12 && fp && n < (int)sizeof g_cb_text - 400; d++) {   // the frame pointers' chain (code without them breaks it)
        uintptr_t pair[2];
        SIZE_T got;
        if (!ReadProcessMemory(GetCurrentProcess(), fp, pair, sizeof pair, &got)) break;
        crash_addr(at, sizeof at, pair[1]);
        n += snprintf(g_cb_text + n, sizeof g_cb_text - n, "\n  %s", at);
        if (pair[0] <= (uintptr_t)fp) break;
        fp = (uintptr_t *)pair[0];
    }
    snprintf(g_cb_text + n, sizeof g_cb_text - n, "\n\nThe log (ageofai\\ageofai_log.csv) has this with more of the stack.");
    // the choice first, the details under it
    static char box[4000];
    snprintf(box, sizeof box,
             "Empire Earth has crashed.\n\n"
             "Dump and close: writes a memory dump of the game as it crashed (a few hundred MB, in %s) - with it the cause can "
             "be found - then closes the game.\n"
             "Close: closes the game.\n\n"
             "Until you choose, the game stays as it is: a debugger can attach now (Visual Studio: Debug > Attach to Process; "
             "x32dbg) and stops at the crash after either button.\n\n"
             "----------\n%s", g_dump_dir[0] ? g_dump_dir : "ageofai\\ageofai_dumps", g_cb_text);
    g_cb_hook = SetWindowsHookExA(WH_CBT, cb_cbt, NULL, GetCurrentThreadId());
    int dump = MessageBoxA(NULL, box, "Empire Earth: Age of AI - the game crashed",
                           MB_YESNO | MB_DEFBUTTON1 | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND) == IDYES;
    if (g_cb_hook) { UnhookWindowsHookEx(g_cb_hook); g_cb_hook = NULL; }
    int dumped = dump && cb_write_dump();
    if (g_log) {
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# crash box: %s %08lx at eip %08lx thread %lu - %s%s%s%s%s\n", cb_code_name(er->ExceptionCode), er->ExceptionCode,
                (unsigned long)c->Eip, g_cb_thread, !dump ? "closed without a dump" : dumped ? "dump " : "no dump: ", dumped ? g_cb_dump : "",
                dumped ? " (" : "", dump ? g_cb_why : "", dumped ? ")" : "");
        fflush(g_log);
        LeaveCriticalSection(&g_lock);
    }
    if (dump && !dumped) {
        snprintf(box, sizeof box, "The memory dump could not be written: %s.\n\nThe game closes now.", g_cb_why);
        MessageBoxA(NULL, box, "Empire Earth: Age of AI - the game crashed", MB_OK | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND);
    }
    g_cb_debugger = IsDebuggerPresent();
    return 0;
}
internal LONG WINAPI cb_filter(EXCEPTION_POINTERS *ep) {
    if (InterlockedExchange(&g_cb_busy, 1)) { for (;;) Sleep(INFINITE); }   // a second crash while the box is open
    g_cb_ep = ep;
    g_cb_thread = GetCurrentThreadId();
    HANDLE h = CreateThread(NULL, 256 * 1024, cb_box_thread, NULL, 0, NULL);
    if (h) { WaitForSingleObject(h, INFINITE); CloseHandle(h); }
    if (g_cb_debugger) return EXCEPTION_CONTINUE_EXECUTION;   // the faulting instruction again: the debugger stops at it
    TerminateProcess(GetCurrentProcess(), ep->ExceptionRecord->ExceptionCode);   // as a crash ended (not g_cb_next: above)
    return EXCEPTION_EXECUTE_HANDLER;
}
// the exe's SetUnhandledExceptionFilter: its filter goes behind ours (the one it replaces is returned as the system would)
internal LPTOP_LEVEL_EXCEPTION_FILTER WINAPI cb_hook_set(LPTOP_LEVEL_EXCEPTION_FILTER f) {
    LPTOP_LEVEL_EXCEPTION_FILTER was = g_cb_next;
    g_cb_next = f == cb_filter ? NULL : f;
    return was;
}
internal int g_cb_on;
internal void crash_box_install(void) {
    if (!g_set.crash_dialog || g_autotest) return;
    g_cb_on = 1;
    LPTOP_LEVEL_EXCEPTION_FILTER was = SetUnhandledExceptionFilter(cb_filter);
    g_cb_next = was == cb_filter ? NULL : was;
    int hooked = patch_iat(GetModuleHandleA(NULL), "KERNEL32.dll", "SetUnhandledExceptionFilter", (void *)cb_hook_set);
    if (g_log) fprintf(g_log, "# crash box: on (the exe's import of the filter call %s, a filter before ours %s)\n", hooked ? "hooked" : "not there", was ? "replaced" : "none");
}
// at each game start ours in front again: a filter set since by a call the import hook doesn't see (the exe has no such
// import in the 2017 update - found by name at run time, or another module's) goes behind it
internal void crash_box_again(void) {
    if (!g_cb_on) return;
    LPTOP_LEVEL_EXCEPTION_FILTER was = SetUnhandledExceptionFilter(cb_filter);
    if (was == cb_filter) return;
    g_cb_next = was;
    char at[160];
    crash_addr(at, sizeof at, (uintptr_t)was);
    if (g_log) fprintf(g_log, "# crash box: a filter set since (%s) replaced by ours again\n", at);
}
