// THE CRASH BOX ([general] crash_dialog; the player, 2026-11-14: "right now the game exits when crashing, makes sence for
// nomral play. but for us it might be usefull if the process stays open and show an exception dialog?"). An exception
// nothing handles ends at the top-level filter (kernel32's UnhandledExceptionFilter: the C runtime's frame round the
// main thread and every thread's start ask it), and the process ended without a word. The mod's filter comes first
// (on by default since the player, 2026-11-14: "the game just closing has no value to anybody"): a box with the
// exception - where (module + offset), what was read or written, the call chain by frame pointers - and two buttons
// ("it than should have clerly explained options: a "dump and close" and "close" button"), asking for a report at the
// project's issues (THE REPORT below): Dump and close writes the crash report - a zip of a memory dump at the crash (the
// crashing thread's context in it: Visual Studio or WinDbg open it at the faulting instruction) and the log's end - and
// opens its folder and the issue page; Close doesn't; a line in the log either way. The
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
internal char g_cb_log[MAX_PATH];   // the log's path (crash_box_install: the report takes its end)

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
// THE REPORT (the player, 2026-11-14: "this is now also meant as a user facing dialog, with the invitation to post the
// crash as a issue with the dump file aswell a small description"): crash_<date>_<time>.zip in ageofai_dumps with the
// dump (crash.dmp) and the log's last CB_LOG_TAIL bytes (ageofai_log_end.csv: the exception's line, what led to it). The
// dump is the small kind for a report - the threads, their stacks and the memory they point at, not the modules' data:
// 0.8 MB in a test game against 172 MB with it (and from inside the process the kind with the data failed on memory it
// couldn't read, 8007012b); GitHub takes files up to 25 MB, a .zip among them, not a .dmp. Stored, not compressed (no
// deflate here; the size is small enough as it is). Then the folder with the zip selected and the project's issue page
// open in their programs.
#define CB_ISSUES "https://github.com/Grille/Empire-Earth-Age-of-AI/issues"
#define CB_LOG_TAIL (1024 * 1024)
internal uint32_t g_cb_crc[256];
internal uint32_t cb_crc32(uint32_t c, const uint8_t *p, size_t n) {
    if (!g_cb_crc[1])
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t v = i;
            for (int k = 0; k < 8; k++) v = v & 1 ? 0xEDB88320u ^ (v >> 1) : v >> 1;
            g_cb_crc[i] = v;
        }
    c = ~c;
    while (n--) c = g_cb_crc[(c ^ *p++) & 255] ^ (c >> 8);
    return ~c;
}
// a file's bytes from offset on (malloc'd, *n its size); NULL: none
internal uint8_t *cb_read(const char *path, long tail, DWORD *n) {
    HANDLE f = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, 0, NULL);
    if (f == INVALID_HANDLE_VALUE) return NULL;
    DWORD size = GetFileSize(f, NULL), from = tail > 0 && size > (DWORD)tail ? size - (DWORD)tail : 0, got = 0;
    uint8_t *b = size != INVALID_FILE_SIZE ? malloc(size - from + 1) : NULL;
    if (b) { SetFilePointer(f, (LONG)from, NULL, FILE_BEGIN); if (!ReadFile(f, b, size - from, &got, NULL)) got = 0; }
    CloseHandle(f);
    *n = got;
    return b;
}
// a zip of the files, stored; 1 written
internal int cb_zip(const char *path, const char *const *names, uint8_t *const *data, const DWORD *sizes, int n, const SYSTEMTIME *t) {
    HANDLE f = CreateFileA(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (f == INVALID_HANDLE_VALUE) return 0;
    uint16_t dtime = (uint16_t)(t->wHour << 11 | t->wMinute << 5 | t->wSecond / 2), ddate = (uint16_t)((t->wYear - 1980) << 9 | t->wMonth << 5 | t->wDay);
    uint32_t off[8], crc[8], at = 0;
    DWORD w;
    int ok = 1;
    #define CB_PUT(p, k) do { ok &= WriteFile(f, (p), (DWORD)(k), &w, NULL) && w == (DWORD)(k); } while (0)
    for (int i = 0; i < n && i < 8; i++) {
        uint16_t nl = (uint16_t)strlen(names[i]);
        crc[i] = cb_crc32(0, data[i], sizes[i]);
        off[i] = at;
        uint8_t h[30];
        memset(h, 0, sizeof h);
        memcpy(h, "PK\3\4", 4); h[4] = 20;
        memcpy(h + 10, &dtime, 2); memcpy(h + 12, &ddate, 2); memcpy(h + 14, &crc[i], 4);
        memcpy(h + 18, &sizes[i], 4); memcpy(h + 22, &sizes[i], 4); memcpy(h + 26, &nl, 2);
        CB_PUT(h, 30); CB_PUT(names[i], nl); CB_PUT(data[i], sizes[i]);
        at += 30 + nl + sizes[i];
    }
    uint32_t cd = at;
    for (int i = 0; i < n && i < 8; i++) {
        uint16_t nl = (uint16_t)strlen(names[i]);
        uint8_t h[46];
        memset(h, 0, sizeof h);
        memcpy(h, "PK\1\2", 4); h[4] = 20; h[6] = 20;
        memcpy(h + 12, &dtime, 2); memcpy(h + 14, &ddate, 2); memcpy(h + 16, &crc[i], 4);
        memcpy(h + 20, &sizes[i], 4); memcpy(h + 24, &sizes[i], 4); memcpy(h + 28, &nl, 2); memcpy(h + 42, &off[i], 4);
        CB_PUT(h, 46); CB_PUT(names[i], nl);
        at += 46 + nl;
    }
    uint8_t e[22];
    memset(e, 0, sizeof e);
    uint16_t cnt = (uint16_t)(n < 8 ? n : 8);
    uint32_t cds = at - cd;
    memcpy(e, "PK\5\6", 4); memcpy(e + 8, &cnt, 2); memcpy(e + 10, &cnt, 2); memcpy(e + 12, &cds, 4); memcpy(e + 16, &cd, 4);
    CB_PUT(e, 22);
    #undef CB_PUT
    CloseHandle(f);
    if (!ok) DeleteFileA(path);
    return ok;
}
// the report: the dump, then the zip of it and the log's end (g_cb_dump: the zip's path); 1 written
internal int cb_write_report(void) {
    typedef BOOL (WINAPI *mdw_fn)(HANDLE, DWORD, HANDLE, MINIDUMP_TYPE, PMINIDUMP_EXCEPTION_INFORMATION,
                                  PMINIDUMP_USER_STREAM_INFORMATION, PMINIDUMP_CALLBACK_INFORMATION);
    HMODULE dh = LoadLibraryA("dbghelp.dll");
    mdw_fn mdw = dh ? (mdw_fn)(void *)GetProcAddress(dh, "MiniDumpWriteDump") : NULL;
    if (!mdw || !g_dump_dir[0]) { snprintf(g_cb_why, sizeof g_cb_why, !mdw ? "no dbghelp.dll" : "no dump folder"); return 0; }
    SYSTEMTIME t;
    GetLocalTime(&t);
    CreateDirectoryA(g_dump_dir, NULL);
    size_t dl = strlen(g_dump_dir);
    char dmp[MAX_PATH];
    snprintf(g_cb_dump, sizeof g_cb_dump, "%s%scrash_%04u%02u%02u_%02u%02u%02u.zip", g_dump_dir,
             g_dump_dir[dl - 1] == '\\' || g_dump_dir[dl - 1] == '/' ? "" : "\\", t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    snprintf(dmp, sizeof dmp, "%.*s.dmp", (int)strlen(g_cb_dump) - 4, g_cb_dump);
    static const DWORD kinds[3] = { MiniDumpWithIndirectlyReferencedMemory | MiniDumpWithUnloadedModules | MiniDumpWithThreadInfo,
                                    MiniDumpWithUnloadedModules | MiniDumpWithThreadInfo, MiniDumpNormal };   // each in turn till one is written
    MINIDUMP_EXCEPTION_INFORMATION mei = { g_cb_thread, g_cb_ep, FALSE };
    DWORD err = 0;
    int ok = 0;
    for (int k = 0; k < 3 && !ok; k++) {
        HANDLE f = CreateFileA(dmp, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        if (f == INVALID_HANDLE_VALUE) { snprintf(g_cb_why, sizeof g_cb_why, "the file not made (error %lu)", GetLastError()); return 0; }
        ok = mdw(GetCurrentProcess(), GetCurrentProcessId(), f, (MINIDUMP_TYPE)kinds[k], &mei, NULL, NULL);
        if (!ok) err = GetLastError();
        CloseHandle(f);
    }
    if (!ok) { DeleteFileA(dmp); snprintf(g_cb_why, sizeof g_cb_why, "MiniDumpWriteDump failed (error %08lx)", err); return 0; }
    if (g_log) { EnterCriticalSection(&g_lock); fflush(g_log); LeaveCriticalSection(&g_lock); }
    DWORD sizes[2] = { 0, 0 };
    uint8_t *data[2] = { cb_read(dmp, 0, &sizes[0]), cb_read(g_cb_log, CB_LOG_TAIL, &sizes[1]) };
    static const char *const names[2] = { "crash.dmp", "ageofai_log_end.csv" };
    int zipped = data[0] && cb_zip(g_cb_dump, names, data, sizes, data[1] ? 2 : 1, &t);
    free(data[0]); free(data[1]);
    if (zipped) DeleteFileA(dmp);
    else snprintf(g_cb_dump, sizeof g_cb_dump, "%s", dmp);   // the dump as it is
    snprintf(g_cb_why, sizeof g_cb_why, "%s", zipped ? "the dump and the log's end" : "the dump alone, the zip not made");
    return 1;
}
// the folder with the report selected, the issue page: in their own programs
internal void cb_open_report(void) {
    typedef HINSTANCE (WINAPI *shex_fn)(HWND, LPCSTR, LPCSTR, LPCSTR, LPCSTR, INT);
    HMODULE sh = LoadLibraryA("shell32.dll");
    shex_fn shex = sh ? (shex_fn)(void *)GetProcAddress(sh, "ShellExecuteA") : NULL;
    if (!shex) return;
    char arg[MAX_PATH + 16];
    snprintf(arg, sizeof arg, "/select,\"%s\"", g_cb_dump);
    shex(NULL, "open", "explorer.exe", arg, NULL, SW_SHOWNORMAL);
    shex(NULL, "open", CB_ISSUES, NULL, NULL, SW_SHOWNORMAL);
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
             "Empire Earth has crashed - sorry!\n\n"
             "Please help to fix it: report it at\n%s\n"
             "Say in a few words what you were doing when it crashed, and attach the crash report.\n\n"
             "Dump and close: saves the crash report (a zip of a few MB: the game's state at the crash and the end of the log) "
             "and closes the game. The folder with the report and the page to report it then open.\n"
             "Close: just closes the game.\n\n"
             "For developers: until you choose, the game stays as it is - a debugger can attach now and stops at the crash "
             "after either button.\n\n"
             "----------\n%s", CB_ISSUES, g_cb_text);
    g_cb_hook = SetWindowsHookExA(WH_CBT, cb_cbt, NULL, GetCurrentThreadId());
    int dump = MessageBoxA(NULL, box, "Empire Earth: Age of AI - the game crashed",
                           MB_YESNO | MB_DEFBUTTON1 | MB_ICONERROR | MB_TOPMOST | MB_SETFOREGROUND) == IDYES;
    if (g_cb_hook) { UnhookWindowsHookEx(g_cb_hook); g_cb_hook = NULL; }
    if (g_log) {   // before the report: its log's end has it
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# crash box: %s %08lx at eip %08lx thread %lu - %s\n", cb_code_name(er->ExceptionCode), er->ExceptionCode,
                (unsigned long)c->Eip, g_cb_thread, dump ? "the crash report asked for" : "closed without a report");
        fflush(g_log);
        LeaveCriticalSection(&g_lock);
    }
    int dumped = dump && cb_write_report();
    if (dump && g_log) {
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# crash box: %s%s (%s)\n", dumped ? "the report " : "no report: ", dumped ? g_cb_dump : "", g_cb_why);
        fflush(g_log);
        LeaveCriticalSection(&g_lock);
    }
    if (dumped) cb_open_report();
    else if (dump) {
        snprintf(box, sizeof box, "The crash report could not be written: %s.\n\nThe game closes now. You can still report the crash at\n%s\n"
                 "and attach ageofai\\ageofai_log.csv.", g_cb_why, CB_ISSUES);
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
internal void crash_box_install(const char *log_path) {
    lstrcpynA(g_cb_log, log_path ? log_path : "", sizeof g_cb_log);
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
