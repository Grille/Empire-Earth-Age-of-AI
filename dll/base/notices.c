// NOTICES: a fault the mod caught and got round - a kept part freed under the drawing, a run too big for the animation
// pool - is a bug somewhere that a player's game hit, and its log line alone went unseen among thousands (the player,
// 2026-11-22: "it unlikly i notice them if they just land soewhere in the logs"). notice(kind, ...) tells it three
// ways: the log (# notice:), ageofai_notices.txt beside it (only these: the date and time, the game minute, the kind
// and what happened - the first NOTICE_FILE of a kind in a session), and a line on the game's screen the first time a
// kind comes up in a session (notices_show, on the simulation's thread: voice/chat.c). Any thread may call it.
#define NOTICE_KINDS 32
#define NOTICE_FILE 5           // a kind's notices written to the file in a session (the log has them all)
#define NOTICE_FILE_MAX (1 << 20)   // the file over this at the session's first notice: it becomes ageofai_notices.old.txt
internal char g_notice_path[MAX_PATH];   // set with the log's path (eeaimod.c)
internal struct {
    const char *kind[NOTICE_KINDS];
    LONG count[NOTICE_KINDS];
    int nkinds, rotated;
    char screen[4][240];   // the lines for the screen not shown yet
    int nscreen;
    uint32_t (*minute)(void);   // the game's time (ms), 0 outside a game: set by whoever knows it (eeaimod.c)
} g_notice;

internal PRINTF_LIKE(2, 3) void notice(const char *kind, const char *fmt, ...) {
    char msg[400];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof msg, fmt, ap);
    va_end(ap);
    if (!g_log) return;   // (the log's lock is made with it)
    uint32_t ms = g_notice.minute ? g_notice.minute() : 0;
    EnterCriticalSection(&g_lock);
    int k = 0;
    while (k < g_notice.nkinds && strcmp(g_notice.kind[k], kind)) k++;
    if (k == g_notice.nkinds && k < NOTICE_KINDS) g_notice.kind[g_notice.nkinds++] = kind;
    LONG n = k < NOTICE_KINDS ? ++g_notice.count[k] : 2;
    fprintf(g_log, "# notice: %s (%ld this session): %s\n", kind, (long)n, msg);
    fflush(g_log);
    if (n <= NOTICE_FILE && g_notice_path[0]) {
        if (!g_notice.rotated) {
            g_notice.rotated = 1;
            WIN32_FILE_ATTRIBUTE_DATA fa;
            if (GetFileAttributesExA(g_notice_path, GetFileExInfoStandard, &fa) && fa.nFileSizeLow > NOTICE_FILE_MAX) {
                char old[MAX_PATH];
                lstrcpynA(old, g_notice_path, MAX_PATH - 8);
                char *dot = strrchr(old, '.');
                if (dot) lstrcpyA(dot, ".old.txt");
                MoveFileExA(g_notice_path, old, MOVEFILE_REPLACE_EXISTING);
            }
        }
        FILE *f = fopen(g_notice_path, "a");
        if (f) {
            SYSTEMTIME st;
            GetLocalTime(&st);
            fprintf(f, "%04d-%02d-%02d %02d:%02d:%02d  game %u:%02u  %s%s: %s\n", st.wYear, st.wMonth, st.wDay, st.wHour,
                    st.wMinute, st.wSecond, ms / 60000, ms / 1000 % 60, kind, n == NOTICE_FILE ? " (more of it only in the log)" : "", msg);
            fclose(f);
        }
    }
    if (n == 1 && g_notice.nscreen < 4)
        snprintf(g_notice.screen[g_notice.nscreen++], sizeof g_notice.screen[0], "Age of AI caught a fault and went on: %s (ageofai_notices.txt)", kind);
    LeaveCriticalSection(&g_lock);
}
// the next line for the screen into line (taken off the queue); 0 when none
internal int notice_next(char *line, size_t cap) {
    if (!g_notice.nscreen || !g_log) return 0;
    EnterCriticalSection(&g_lock);
    int got = g_notice.nscreen > 0;
    if (got) {
        lstrcpynA(line, g_notice.screen[0], (int)cap);
        memmove(g_notice.screen[0], g_notice.screen[1], sizeof g_notice.screen[0] * (size_t)(--g_notice.nscreen));
    }
    LeaveCriticalSection(&g_lock);
    return got;
}
