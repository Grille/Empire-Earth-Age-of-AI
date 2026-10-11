// Hardware write-watchpoint (debug register DR0) for reverse engineering: logs every instruction that
// writes the watched address, with the value and a short EBP-chain backtrace. Enabled from the autotest
// ini ("watch=<hexaddr>", optional "watch_len=1|2|4") so it covers loading and game start. A helper
// thread re-applies the debug registers to every thread of the process every 20 ms, which also catches
// threads created later. Included by eeaimod.c.

static uintptr_t g_watch_addr;
static int g_watch_len = 1;
static volatile LONG g_watch_hits;
#define WATCH_MAX_HITS 20000

static LONG CALLBACK watch_handler(EXCEPTION_POINTERS *ep) {
    if (ep->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP) return EXCEPTION_CONTINUE_SEARCH;
    CONTEXT *c = ep->ContextRecord;
    if (!(c->Dr6 & 1)) return EXCEPTION_CONTINUE_SEARCH;
    c->Dr6 = 0;
    if (InterlockedIncrement(&g_watch_hits) <= WATCH_MAX_HITS && g_log) {
        uint32_t v = 0;
        memcpy(&v, (void *)g_watch_addr, (size_t)g_watch_len);
        char line[400];
        int n = wsprintfA(line, "# watch %08lx: after eip=%08lx value=%lu thread=%lu stack:", (unsigned long)g_watch_addr,
                          (unsigned long)c->Eip, (unsigned long)v, GetCurrentThreadId());
        uintptr_t *fp = (uintptr_t *)c->Ebp;
        for (int d = 0; d < 8 && fp; d++) {
            uintptr_t pair[2];
            SIZE_T got;
            if (!ReadProcessMemory(GetCurrentProcess(), fp, pair, sizeof pair, &got)) break;
            n += wsprintfA(line + n, " %08lx", (unsigned long)pair[1]);
            if (pair[0] <= (uintptr_t)fp) break;
            fp = (uintptr_t *)pair[0];
        }
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "%s\n", line);
        fflush(g_log);
        LeaveCriticalSection(&g_lock);
    }
    return EXCEPTION_CONTINUE_EXECUTION;
}

static void watch_apply_all(DWORD self) {
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    if (snap == INVALID_HANDLE_VALUE) return;
    THREADENTRY32 te = { .dwSize = sizeof te };
    DWORD pid = GetCurrentProcessId();
    DWORD len_bits = g_watch_len == 4 ? 3 : g_watch_len == 2 ? 1 : 0;
    DWORD dr7 = 1 | (1u << 16) | (len_bits << 18);   // L0, RW0 = 01 (write), LEN0
    for (BOOL ok = Thread32First(snap, &te); ok; ok = Thread32Next(snap, &te)) {
        if (te.th32OwnerProcessID != pid || te.th32ThreadID == self) continue;
        HANDLE t = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
        if (!t) continue;
        CONTEXT c = { .ContextFlags = CONTEXT_DEBUG_REGISTERS };
        if (SuspendThread(t) != (DWORD)-1) {
            if (GetThreadContext(t, &c) && (c.Dr0 != g_watch_addr || (c.Dr7 & 0xF0003) != dr7)) {
                c.Dr0 = g_watch_addr;
                c.Dr7 = (c.Dr7 & ~0xF0003u) | dr7;
                SetThreadContext(t, &c);
            }
            ResumeThread(t);
        }
        CloseHandle(t);
    }
    CloseHandle(snap);
}

static DWORD WINAPI watch_thread(LPVOID unused) {
    (void)unused;
    DWORD self = GetCurrentThreadId();
    for (;;) {
        watch_apply_all(self);
        Sleep(20);
    }
}

// Start or move the watchpoint at runtime (control command "watch <hexaddr> [len]").
static void watch_set(uintptr_t addr, int len) {
    static int started;
    g_watch_len = (len == 2 || len == 4) ? len : 1;
    g_watch_addr = addr & ~(uintptr_t)(g_watch_len - 1);
    g_watch_hits = 0;
    if (!started) {
        started = 1;
        AddVectoredExceptionHandler(1, watch_handler);
        CloseHandle(CreateThread(NULL, 0, watch_thread, NULL, 0, NULL));
    }
    if (g_log) fprintf(g_log, "# watch: write watchpoint on %08lx len %d\n", (unsigned long)g_watch_addr, g_watch_len);
}

// Experimental byte patches from the autotest ini: patch1..patch16 = <hexaddr>:<original hex>:<new hex>.
// Applied only when the original bytes match, so a wrong address can't corrupt code.
static void apply_ini_patches(const char *ini) {
    for (int i = 1; i <= 16; i++) {
        char key[16], val[160];
        wsprintfA(key, "patch%d", i);
        GetPrivateProfileStringA("autotest", key, "", val, sizeof val, ini);
        if (!val[0]) continue;
        char *c1 = strchr(val, ':'), *c2 = c1 ? strchr(c1 + 1, ':') : NULL;
        uint8_t orig[64], repl[64];
        unsigned no = 0, nr = 0, b;
        int ok = c1 && c2;
        if (ok) {
            for (char *p = c1 + 1; p + 1 < c2 && no < sizeof orig && sscanf(p, "%2x", &b) == 1; p += 2) orig[no++] = (uint8_t)b;
            for (char *p = c2 + 1; p[0] && p[1] && nr < sizeof repl && sscanf(p, "%2x", &b) == 1; p += 2) repl[nr++] = (uint8_t)b;
        }
        uint8_t *at = (uint8_t *)(uintptr_t)strtoul(val, NULL, 16);
        ok = ok && no && no == nr && !memcmp(at, orig, no);
        if (ok) {
            DWORD old;
            VirtualProtect(at, nr, PAGE_EXECUTE_READWRITE, &old);
            memcpy(at, repl, nr);
            VirtualProtect(at, nr, old, &old);
            FlushInstructionCache(GetCurrentProcess(), at, nr);
        }
        if (g_log) fprintf(g_log, "# patch%d %s: %s\n", i, val, ok ? "applied" : "REJECTED (bytes differ or bad syntax)");
    }
}

static void watch_start(const char *ini) {
    apply_ini_patches(ini);
    char buf[32];
    GetPrivateProfileStringA("autotest", "watch", "", buf, sizeof buf, ini);
    if (!buf[0]) return;
    g_watch_addr = (uintptr_t)strtoul(buf, NULL, 16);
    g_watch_len = GetPrivateProfileIntA("autotest", "watch_len", 1, ini);
    if (g_watch_len != 2 && g_watch_len != 4) g_watch_len = 1;
    g_watch_addr &= ~(uintptr_t)(g_watch_len - 1);   // debug registers need aligned addresses
    AddVectoredExceptionHandler(1, watch_handler);
    CloseHandle(CreateThread(NULL, 0, watch_thread, NULL, 0, NULL));
    if (g_log) fprintf(g_log, "# watch: write watchpoint on %08lx len %d\n", (unsigned long)g_watch_addr, g_watch_len);
}

// Crash log: the first access violations / illegal instructions with module+offset and an EBP-chain
// backtrace. First-chance, so exceptions the game handles itself show up too (hence the cap).
static volatile LONG g_crash_logged;

static void crash_addr(char *buf, size_t cap, uintptr_t a) {
    HMODULE hm = NULL;
    char path[MAX_PATH];
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)a, &hm) && GetModuleFileNameA(hm, path, sizeof path)) {
        const char *b = strrchr(path, 0x5C);
        snprintf(buf, cap, "%08lx(%s+%lx)", (unsigned long)a, b ? b + 1 : path, (unsigned long)(a - (uintptr_t)hm));
    } else snprintf(buf, cap, "%08lx", (unsigned long)a);
}

static LONG CALLBACK crash_handler(EXCEPTION_POINTERS *ep) {
    DWORD code = ep->ExceptionRecord->ExceptionCode;
    if (code != EXCEPTION_ACCESS_VIOLATION && code != EXCEPTION_ILLEGAL_INSTRUCTION && code != EXCEPTION_PRIV_INSTRUCTION)
        return EXCEPTION_CONTINUE_SEARCH;
    if (InterlockedIncrement(&g_crash_logged) > 8 || !g_log) return EXCEPTION_CONTINUE_SEARCH;
    CONTEXT *c = ep->ContextRecord;
    char at[160], line[1200];
    crash_addr(at, sizeof at, c->Eip);
    int n = snprintf(line, sizeof line, "# exception %08lx at %s thread %lu, %s %08lx; eax=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx; stack:",
                     code, at, GetCurrentThreadId(), ep->ExceptionRecord->ExceptionInformation[0] ? "write" : "read",
                     (unsigned long)ep->ExceptionRecord->ExceptionInformation[1], c->Eax, c->Ecx, c->Edx, c->Esi, c->Edi);
    uintptr_t *fp = (uintptr_t *)c->Ebp;
    for (int d = 0; d < 10 && fp && n < (int)sizeof line - 80; d++) {
        uintptr_t pair[2];
        SIZE_T got;
        if (!ReadProcessMemory(GetCurrentProcess(), fp, pair, sizeof pair, &got)) break;
        crash_addr(at, sizeof at, pair[1]);
        n += snprintf(line + n, sizeof line - n, " %s", at);
        if (pair[0] <= (uintptr_t)fp) break;
        fp = (uintptr_t *)pair[0];
    }
    // code without frame pointers (DDRAW.dll, drivers) breaks the chain: also list stack words that point into
    // a module's code, the likely return addresses
    n += snprintf(line + n, sizeof line - n, "; scan:");
    uintptr_t words[256];
    SIZE_T got = 0;
    ReadProcessMemory(GetCurrentProcess(), (void *)c->Esp, words, sizeof words, &got);
    for (size_t i = 0, shown = 0; i < got / sizeof *words && shown < 12 && n < (int)sizeof line - 80; i++) {
        HMODULE hm;
        MEMORY_BASIC_INFORMATION mbi;
        if (words[i] < 0x10000 || !VirtualQuery((void *)words[i], &mbi, sizeof mbi)
            || !(mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY | PAGE_EXECUTE))
            || !GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                   (LPCSTR)words[i], &hm)) continue;
        crash_addr(at, sizeof at, words[i]);
        n += snprintf(line + n, sizeof line - n, " [%u]%s", (unsigned)i, at);
        shown++;
    }
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "%s\n", line);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
    return EXCEPTION_CONTINUE_SEARCH;
}
