// THE LONG HOLDS OF THE WORLD'S LOCK (the player, 2026-11-08: "the 50s bother me"). His big save loads in 3 s in a test, in
// 6-15 s in his game and once took 50.7 s, all of it one hold of the world's lock (the game's load, taken at 0052e821) -
// and only his game does it (from the menu, a real window), so it has to be measured there, as it happens. A thread of
// its own looks 10 times a second whether the lock has been held for HP_START_MS: then it samples the holder and the
// render thread (a load may be waiting for it) until the hold ends. A hold of HP_MIN_MS or more is told in the log:
//   # long hold: where the game took the lock, how long, the holder's CPU time (little of it: it was waiting), what the
//     process read meanwhile; on top: the modules the threads were in
//   # long hold: <thread> frames: return addresses on the stack by the share of the samples they were on it, with the
//     time of the hold they were seen first and last ("0052e9c4 62% 1.2-8.1"): the load's stages, in order
// While the thread is suspended nothing is called but GetThreadContext and ReadProcessMemory: it may hold the heap's
// lock. Nothing happens below HP_START_MS but one read of the lock's time, so the game plays as it does without this.
// EEAIMOD_LONG_HOLD_MS (test runs) sets HP_MIN_MS, a test game's loads being short.
// A stop of the frames is told the same way ("# long hold: no frame for", the main thread sampled first): the way out of a
// game to the menu (the player, 2026-11-08) holds the world's lock for 150 ms of it, the rest is the main thread's alone.
// From 400 ms on, the clock's time in the line, 40 of each kind a game: the hiccups of a playing game (the player, 2026-11-08:
// "a 0.5 to 1 second hickup where noting moves"), which no test showed, the player's camera and clicks being his own.
enum { HP_START_MS = 500, HP_MIN_MS = 400, HP_KEYS = 1024, HP_SHOW = 40, HP_TOPS = 16, HP_REPORTS = 40 };
typedef struct { uintptr_t a; int n, first, last; } hp_key;
typedef struct {
    const char *name;
    DWORD tid;
    HANDLE h;
    hp_key keys[HP_KEYS];
    struct { HMODULE m; int n; } tops[HP_TOPS];
    int nt, samples;
} hp_thread;
internal hp_thread g_hp[2];
internal int g_hp_min_ms = HP_MIN_MS;

internal LONGLONG hp_lock_at(void) {   // the time the write hold began; two reads of a 64-bit value on a 32-bit program
    LONGLONG a;
    do a = g_rwl_write_at; while (a != g_rwl_write_at);
    return a;
}
internal int hp_is_frame(uintptr_t a) { return (is_game_code(a) || is_own_code(a)) && is_return_address(a); }
internal void hp_count(hp_thread *t, uintptr_t a, int ms) {
    unsigned k = (unsigned)(a >> 2) * 2654435761u % HP_KEYS;
    for (int p = 0; p < HP_KEYS; p++, k = (k + 1) % HP_KEYS) {
        if (t->keys[k].a && t->keys[k].a != a) continue;
        if (!t->keys[k].a) { t->keys[k].a = a; t->keys[k].first = ms; }
        t->keys[k].n++;
        t->keys[k].last = ms;
        return;
    }
}
internal void hp_sample(hp_thread *t, int ms) {
    static uintptr_t stack[4096];   // 16 KB: the load's calls are deep
    if (!t->h || SuspendThread(t->h) == (DWORD)-1) return;
    CONTEXT c = { .ContextFlags = CONTEXT_CONTROL };
    SIZE_T got = 0;
    BOOL ok = GetThreadContext(t->h, &c);
    if (ok && !ReadProcessMemory(GetCurrentProcess(), (void *)(uintptr_t)c.Esp, stack, sizeof stack, &got))
        ReadProcessMemory(GetCurrentProcess(), (void *)(uintptr_t)c.Esp, stack, 4096, &got);   // near the stack's base
    ResumeThread(t->h);
    if (!ok) return;
    t->samples++;
    HMODULE m = NULL;
    GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)(uintptr_t)c.Eip, &m);
    int j = 0;
    while (j < t->nt && t->tops[j].m != m) j++;
    if (j == t->nt && t->nt < HP_TOPS) { t->tops[t->nt].m = m; t->tops[t->nt].n = 0; t->nt++; }
    if (j < t->nt) t->tops[j].n++;
    uintptr_t seen[96];
    int ns = 0;
    for (size_t i = 0; i < got / sizeof *stack && ns < 96; i++) {
        if (!hp_is_frame(stack[i])) continue;
        int dup = 0;
        for (int q = 0; q < ns && !dup; q++) dup = seen[q] == stack[i];
        if (dup) continue;
        seen[ns++] = stack[i];
        hp_count(t, stack[i], ms);
    }
}
internal const char *hp_module(HMODULE m, char *buf, DWORD cap) {
    if (!m || !GetModuleFileNameA(m, buf, cap)) return "?";
    const char *b = strrchr(buf, 0x5C);
    return b ? b + 1 : buf;
}
internal void hp_report(hp_thread *t) {
    char line[1800], path[MAX_PATH];
    int n = snprintf(line, sizeof line, "# long hold: %s: %d samples, on top", t->name, t->samples);
    for (int j = 0; j < t->nt && n < 400; j++)
        if (t->tops[j].n * 50 >= t->samples)
            n += snprintf(line + n, sizeof line - n, " %s %d%%", hp_module(t->tops[j].m, path, sizeof path), t->tops[j].n * 100 / t->samples);
    log_locked("%s\n", line);
    n = snprintf(line, sizeof line, "# long hold: %s frames:", t->name);
    for (int r = 0; r < HP_SHOW; r++) {
        int best = -1;
        for (int k = 0; k < HP_KEYS; k++) if (t->keys[k].n > 0 && (best < 0 || t->keys[k].n > t->keys[best].n)) best = k;
        if (best < 0 || t->keys[best].n * 100 < t->samples) break;
        hp_key *e = &t->keys[best];
        HMODULE m = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCSTR)e->a, &m);
        char at[64];
        if (m == GetModuleHandleA(NULL)) snprintf(at, sizeof at, "%08lx", (unsigned long)e->a);
        else snprintf(at, sizeof at, "%.24s+%lx", hp_module(m, path, sizeof path), (unsigned long)(e->a - (uintptr_t)m));
        n += snprintf(line + n, sizeof line - n, " %s %d%% %.1f-%.1f", at, e->n * 100 / t->samples, e->first / 1000.0, e->last / 1000.0);
        e->n = 0;
        if (n > (int)sizeof line - 96) { log_locked("%s\n", line); n = snprintf(line, sizeof line, "# long hold: %s frames:", t->name); }
    }
    log_locked("%s\n", line);
}
internal ULONGLONG hp_cpu(HANDLE h, ULONGLONG *kernel) {   // the thread's CPU time so far, 100 ns
    FILETIME c, x, k, u;
    if (!h || !GetThreadTimes(h, &c, &x, &k, &u)) return *kernel = 0;
    *kernel = (ULONGLONG)k.dwHighDateTime << 32 | k.dwLowDateTime;
    return *kernel + ((ULONGLONG)u.dwHighDateTime << 32 | u.dwLowDateTime);
}
internal DWORD WINAPI hold_profile_thread(void *unused) {
    (void)unused;
    int reports[2] = { 0 };   // the world's lock, the frames'
    int start_ms = g_hp_min_ms / 2 < HP_START_MS ? g_hp_min_ms / 2 : HP_START_MS;
    for (;;) {
        Sleep(100);
        LONGLONG at = hp_lock_at();
        DWORD shown = g_show_tick;
        int hold = at && rd_qpc() - at >= rd_qpc_ms(start_ms);
        int stall = !hold && shown && GetTickCount() - shown >= (DWORD)start_ms;
        if ((!hold && !stall) || reports[stall] >= HP_REPORTS) continue;
        uintptr_t ret = g_rwl_write_ret;
        memset(g_hp, 0, sizeof g_hp);
        DWORD first = hold ? g_sim_thread : g_render_thread, other = hold ? g_render_thread : g_sim_thread;
        g_hp[0].name = hold ? "holder" : "main", g_hp[0].tid = first;
        g_hp[1].name = hold ? "render" : "simulation", g_hp[1].tid = other != first ? other : 0;
        for (int i = 0; i < 2; i++)
            g_hp[i].h = g_hp[i].tid ? OpenThread(THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION, FALSE, g_hp[i].tid) : NULL;
        ULONGLONG k0, k1, cpu0 = hp_cpu(g_hp[0].h, &k0);
        IO_COUNTERS io0 = { 0 }, io1 = { 0 };
        GetProcessIoCounters(GetCurrentProcess(), &io0);
        int held = 0;
        while (hold ? hp_lock_at() == at : g_show_tick == shown) {
            held = hold ? (int)((rd_qpc() - at) / rd_qpc_ms(1)) : (int)(GetTickCount() - shown);
            for (int i = 0; i < 2; i++) hp_sample(&g_hp[i], held);
            Sleep(10);
        }
        ULONGLONG cpu1 = hp_cpu(g_hp[0].h, &k1);
        GetProcessIoCounters(GetCurrentProcess(), &io1);
        if (held >= g_hp_min_ms) {
            reports[stall]++;
            char what[32];
            if (hold) snprintf(what, sizeof what, "%08lx", (unsigned long)ret); else snprintf(what, sizeof what, "no frame");
            SYSTEMTIME now;
            GetLocalTime(&now);
            log_locked("# long hold: %s for %d ms at %.1f min (%02d:%02d:%02d): the %s's CPU %.1f s (kernel %.1f), read %.1f MB in %lu calls (sampled from %d ms on)\n",
                       what, held, game_time() / 60000.0, now.wHour, now.wMinute, now.wSecond, g_hp[0].name, (double)(cpu1 - cpu0) / 1e7, (double)(k1 - k0) / 1e7,
                       (double)(io1.ReadTransferCount - io0.ReadTransferCount) / 1e6, (unsigned long)(io1.ReadOperationCount - io0.ReadOperationCount), start_ms);
            for (int i = 0; i < 2; i++) if (g_hp[i].samples) hp_report(&g_hp[i]);
        }
        for (int i = 0; i < 2; i++) if (g_hp[i].h) CloseHandle(g_hp[i].h);
    }
    return 0;
}
internal void hold_profile_install(void) {
    if (!g_rd_on) return;   // the lock's hooks are the own renderer's
    char ms[16];
    if (GetEnvironmentVariableA("EEAIMOD_LONG_HOLD_MS", ms, sizeof ms) && atoi(ms) > 0) g_hp_min_ms = atoi(ms);
    HANDLE t = CreateThread(NULL, 0, hold_profile_thread, NULL, 0, NULL);
    if (t) CloseHandle(t);
}
