// THE LOG (aimod_econ.csv, or a test slot's): g_log, under g_lock for the threads other than the simulation's
static FILE *g_log;
static CRITICAL_SECTION g_lock;
static int g_autotest;   // an autotest run (aimod_autotest.ini): the parts' test-only log lines

// a log line from a thread other than the simulation's (render, window, sound): under g_lock, flushed - as the pipe
// thread (ctl_log), the crash handler and the autotest/quit exits that close the log hold it
static void log_locked(const char *fmt, ...) {
    if (!g_log) return;
    EnterCriticalSection(&g_lock);
    if (g_log) {
        va_list ap;
        va_start(ap, fmt);
        vfprintf(g_log, fmt, ap);
        va_end(ap);
        fflush(g_log);
    }
    LeaveCriticalSection(&g_lock);
}

static void log_line(const char *fmt, const char *a, int b) {
    if (!g_log) return;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, fmt, a, b);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}
