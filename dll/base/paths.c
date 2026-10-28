// WHERE THINGS ARE: the game folder, the settings files, the personalities, the dumps - and a test slot's own folder.
internal char g_ini[MAX_PATH], g_aimod_ini[MAX_PATH];   // ageofai_autotest.ini, ageofai.ini (paths set in DllMain)
// <game>\ and the personalities' folder, <game>\ageofai\personalities\ (with the backslash). Under autotest the config and the
// personality folder come from ageofai_autotest.ini [autotest] config / ai_dir (run_test.ps1: the repository's config\ by
// default), so tests measure the shipped files, not the player's own ageofai\ageofai.ini; normal play: ageofai.ini in
// <game>\ageofai\, the personalities from its [general] ai_dir (a developer's game reading the repository's files) or
// its folder personalities.
internal char g_game_dir[MAX_PATH], g_ai_dir[MAX_PATH];
internal int g_slot;   // a test slot (TEST SLOTS, at slot_dir), 0: none
internal char g_dump_dir[MAX_PATH];   // also the folder of the control pipe's shot / ddinfo dump pictures

// TEST SLOTS: parallel test games (run_test.ps1 -Slot n; docs/testing.md, Parallel runs). The runner starts slot
// n (1..30) with EEAIMOD_SLOT=n and EEAIMOD_SLOT_DIR=<its folder>: the test ini, the log and the dumps are there instead
// of the game folder, the control pipe is \\.\pipe\eeaimod<n>, the engine's network ports are moved by 2n
// (slot_ports) - and the runner patches the game's single-instance check (docs/engine-notes.md, Exe builds). 0: none.
// Without a slot they are in the mod's folder in the game folder, <game>\ageofai\ (with the settings and the
// personalities - the game folder itself holds only the two DLLs and the settings program).
internal void slot_dir(char *dir) {   // dir: the game folder (with its \) -> where this game's log and test files are
    char v[MAX_PATH];
    g_slot = GetEnvironmentVariableA("EEAIMOD_SLOT", v, sizeof v) ? atoi(v) : 0;
    if (g_slot < 0 || g_slot > 30) g_slot = 0;
    if (g_slot && GetEnvironmentVariableA("EEAIMOD_SLOT_DIR", v, sizeof v) && v[0]) {
        size_t n = strlen(v);
        snprintf(dir, MAX_PATH, "%s%s", v, v[n - 1] == '\\' || v[n - 1] == '/' ? "" : "\\");
    } else {
        lstrcatA(dir, "ageofai\\");
        CreateDirectoryA(dir, NULL);   // a log line before the folder was ever made (exe.c: another build)
    }
}

// FOLDER LINKS: a folder with a file folder.link in it is another folder - the one on the link's first line, absolute
// or relative to the link's own folder ("..\..\..\Empire Earth\ageofai\personalities"); its subfolders are the
// target's, and they may be links of their own. So an AoC game reads EE's personalities, a developer's game the
// repository's, a mod shares only mod\ - without a copy, and without the junctions Windows hides (a removed worktree
// deleted the files behind one, 2026-10-26). Strict: a linked folder holds nothing but its link (which copy is read
// is never a question), the target must be a folder, at most LINK_DEPTH links in a row (a circle). The personality
// folder is the first to use it (persona.c ai_folder).
#define LINK_FILE "folder.link"
#define LINK_DEPTH 8
// dir (a folder with its \, MAX_PATH) -> the folder it stands for, through its links; 0: a mistake, told in why
internal int link_follow(char *dir, char *why, int cap) {
    for (int depth = 0;; depth++) {
        char path[MAX_PATH], line[MAX_PATH] = "";
        snprintf(path, sizeof path, "%s" LINK_FILE, dir);
        HANDLE h = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (h == INVALID_HANDLE_VALUE) return 1;   // no link: the folder is itself
        if (depth == LINK_DEPTH) { CloseHandle(h); snprintf(why, cap, "%s: more than %d links in a row (a circle?)", path, LINK_DEPTH); return 0; }
        DWORD got = 0;
        BOOL read = ReadFile(h, line, sizeof line - 1, &got, NULL);
        CloseHandle(h);
        line[read ? got : 0] = 0;
        char *s = line;
        if ((uint8_t)s[0] == 0xEF && (uint8_t)s[1] == 0xBB && (uint8_t)s[2] == 0xBF) s += 3;   // UTF-8 as Notepad saves it
        s[strcspn(s, "\r\n")] = 0;
        while (*s == ' ' || *s == '\t') s++;
        size_t n = strlen(s);
        while (n && (s[n - 1] == ' ' || s[n - 1] == '\t' || s[n - 1] == '\\' || s[n - 1] == '/')) s[--n] = 0;
        if (!n) { snprintf(why, cap, "%s: no folder on its first line", path); return 0; }
        // nothing beside the link
        char pat[MAX_PATH];
        snprintf(pat, sizeof pat, "%s*", dir);
        WIN32_FIND_DATAA fd;
        HANDLE f = FindFirstFileA(pat, &fd);
        if (f != INVALID_HANDLE_VALUE) {
            int other = 0;
            do other = lstrcmpA(fd.cFileName, ".") && lstrcmpA(fd.cFileName, "..") && lstrcmpiA(fd.cFileName, LINK_FILE);
            while (!other && FindNextFileA(f, &fd));
            FindClose(f);
            if (other) { snprintf(why, cap, "%s: %s beside it - a linked folder holds nothing but its link", path, fd.cFileName); return 0; }
        }
        char to[MAX_PATH * 2];
        if (s[1] == ':' || (s[0] == '\\' && s[1] == '\\') || (s[0] == '/' && s[1] == '/')) lstrcpynA(to, s, sizeof to);
        else snprintf(to, sizeof to, "%s%s", dir, s);
        for (char *c = to; *c; c++) if (*c == '/') *c = '\\';
        char full[MAX_PATH];
        DWORD fn = GetFullPathNameA(to, MAX_PATH - 1, full, NULL);   // the ..\ out
        DWORD a = fn && fn < MAX_PATH - 1 ? GetFileAttributesA(full) : INVALID_FILE_ATTRIBUTES;
        if (a == INVALID_FILE_ATTRIBUTES || !(a & FILE_ATTRIBUTE_DIRECTORY)) { snprintf(why, cap, "%s: %s is no folder", path, fn ? full : to); return 0; }
        if (fn && full[fn - 1] != '\\') lstrcatA(full, "\\");
        lstrcpyA(dir, full);
    }
}
// root (a folder with its \) + sub ("" or "mod\\" - folders only, each with its \) -> out (MAX_PATH), every folder on
// the way through its links; 0: a mistake, told in why
internal int link_folder(const char *root, const char *sub, char *out, char *why, int cap) {
    lstrcpynA(out, root, MAX_PATH);
    if (!link_follow(out, why, cap)) return 0;
    for (const char *p = sub; *p;) {
        size_t n = strcspn(p, "\\/");
        if (!p[n]) { snprintf(why, cap, "%s%s: a folder must end with \\", out, p); return 0; }
        if (strlen(out) + n + 2 > MAX_PATH) { snprintf(why, cap, "%s%.*s: too long", out, (int)n, p); return 0; }
        size_t o = strlen(out);
        memcpy(out + o, p, n);
        out[o + n] = '\\';
        out[o + n + 1] = 0;
        if (!link_follow(out, why, cap)) return 0;
        p += n + 1;
    }
    return 1;
}
