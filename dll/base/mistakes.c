// A MISTAKE in the mod's files - a text, a setting, a name the game's data hasn't got: logged (# texts:), once each, and
// with strict parsing shown (x_errors_show, x_errors_box). The language reports its own here, and so does the rest.
internal int g_x_logged;   // expression errors written to the log (at most 60)
// STRICT PARSING ([general] strict_parsing=1, for whoever writes the texts; default 0: a mistake is logged and skipped,
// the game goes on): every mistake also comes up at once - those in the files at startup in a message box, those
// found while the game runs (an unknown name, a text in arithmetic ...) as a system line in the game (x_errors_show,
// from the AIs' updates).
internal int g_strict;
internal CRITICAL_SECTION g_x_lock;   // the queue: filled on the game's thread, read by the startup box's (x_errors_box)
internal char g_x_queue[8][240];
internal int g_x_queued, g_x_total, g_x_dropped;
internal char *g_x_capture;   // texts corpus: every mistake's message appended here too (each time, not once)
internal int g_x_capture_cap;
internal PRINTF_LIKE(1, 2) void x_error(const char *fmt, ...) {
    char msg[240];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    if (g_x_capture) {
        size_t o = strlen(g_x_capture);
        snprintf(g_x_capture + o, (size_t)g_x_capture_cap - o, "%s{%s}", o ? " " : "", msg);
    }
    static uint32_t told[64];   // the same mistake again (a line said again, a placeholder in every prompt): once
    uint32_t h = 2166136261u;
    for (const char *s = msg; *s; s++) h = (h ^ (uint8_t)*s) * 16777619u;
    h |= 1;
    if (told[h % 64] == h) return;
    told[h % 64] = h;
    if (g_log && g_x_logged < 60) { g_x_logged++; fprintf(g_log, "# texts: %s\n", msg); }
    if (!g_strict) return;
    EnterCriticalSection(&g_x_lock);
    g_x_total++;
    int seen = 0;   // the same mistake again (a line tried three times): once
    for (int i = 0; i < g_x_queued; i++) seen |= !strcmp(g_x_queue[i], msg);
    if (seen) g_x_total--;
    else if (g_x_queued < 8) lstrcpynA(g_x_queue[g_x_queued++], msg, sizeof g_x_queue[0]);
    else g_x_dropped++;
    LeaveCriticalSection(&g_x_lock);
}
