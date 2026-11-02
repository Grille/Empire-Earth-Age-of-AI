// ageofai_check.exe: the .eeai files checked without the game - a personality folder as the game reads it at its start
// (every file, mod/ with it: their records, names, lines, fields, handlers, the must-say situations), a script
// (tests/scenarios, tests/probes) as a test run reads it before it runs. Each mistake one line, path:line: what - the
// form VS Code's problem matchers and most editors read (.vscode/tasks.json); exit 1 when there is one.
//
//   ageofai_check <personality folder>      every file of it (the folder with mod\ in it)
//   ageofai_check <file.eeai> ...           a personality file: its folder; any other: a script, with the
//                                           personalities of --ai <folder> or else config\personalities above it
//   ageofai_check <folder> ...              another folder: each of its .eeai files a script
//
// One personality folder a run (the game reads one; its state isn't made to be read twice), scripts as many as given.
// The DLL's own code, compiled into a program of its own: the same checks, not a copy of them. Nothing of the game is
// there - its code is never called (the DLL's install isn't), and the game's addresses aren't this program's memory
// (no build known: g_exe_build -1; its image above them): a read of the game fails at once instead of reading this
// program. What only the game's data knows is left for the game: unit.<identifier> names (counted, said at the end).
#include "eeaimod.c"

internal char g_chk_dir[MAX_PATH];   // the folder of the files checked now (the mistakes name them relative to it)
internal char g_chk_ai[MAX_PATH];    // the personality folder read ("" none yet)
internal int g_chk_n;
internal void chk_told(const char *msg) {   // a mistake: its file's name made a path, "file:line: " in front
    char file[MAX_PATH], path[MAX_PATH];
    size_t n = strcspn(msg, ": ");
    if (n && n < 64 && msg[n] == ':') {   // "fuchs.eeai:12: ...", "fuchs.eeai: ...", "fuchs: include ..." (a file's name)
        lstrcpynA(file, msg, (int)n + 1);
        if (!strstr(file, ".eeai")) lstrcatA(file, ".eeai");
        snprintf(path, sizeof path, "%s%s", g_chk_dir, file);
        for (char *s = path; *s; s++) if (*s == '/') *s = '\\';
        if (GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES) {
            const char *rest = msg + n + 1;
            int line = 1;   // a mistake of the whole file: its first line
            if (isdigit((unsigned char)*rest)) { line = atoi(rest); rest += strspn(rest, "0123456789"); if (*rest == ':') rest++; }
            while (*rest == ' ') rest++;
            printf("%s:%d: %s\n", path, line, rest);
            g_chk_n++;
            return;
        }
    }
    printf("%s: %s\n", g_chk_dir[0] ? g_chk_dir : "ageofai_check", msg);   // not of one file (the folder's)
    g_chk_n++;
}
internal int chk_is_folder(const char *dir) {   // a personality folder: mod\computer.eeai in it
    char p[MAX_PATH];
    snprintf(p, sizeof p, "%s\\mod\\computer.eeai", dir);
    return GetFileAttributesA(p) != INVALID_FILE_ATTRIBUTES;
}
internal void chk_folder(const char *dir) {   // the personality folder dir as the game reads it - once a run
    char d[MAX_PATH];
    snprintf(d, sizeof d, "%s\\", dir);
    if (g_chk_ai[0]) {
        if (lstrcmpiA(g_chk_ai, d)) fprintf(stderr, "ageofai_check: %s left out - one personality folder a run (%s read)\n", dir, g_chk_ai);
        return;
    }
    lstrcpyA(g_chk_ai, d);
    lstrcpyA(g_chk_dir, d);
    lstrcpyA(g_ai_dir, d);
    lstrcpyA(g_lang, "en");
    personas_load();
    texts_load();
    if (g_texts_missing) chk_told("mod\\computer.eeai, mod\\prompt.eeai, mod\\units.eeai: not all there - not a personality folder");
}
internal void chk_script(const char *path, const char *ai) {   // a script, after the personalities it may name (persona.fuchs)
    char dir[MAX_PATH], p[MAX_PATH] = "";
    lstrcpyA(dir, path);
    char *slash = strrchr(dir, '\\');
    if (slash) *slash = 0;
    if (ai) GetFullPathNameA(ai, MAX_PATH, p, NULL);
    else for (char up[MAX_PATH] = ""; !g_chk_ai[0];) {   // config\personalities in a folder above it
        if (!up[0]) lstrcpyA(up, dir);
        snprintf(p, sizeof p, "%s\\config\\personalities", up);
        if (chk_is_folder(p)) break;
        p[0] = 0;
        char *s = strrchr(up, '\\');
        if (!s) break;
        *s = 0;
    }
    if (p[0]) chk_folder(p);
    snprintf(g_chk_dir, sizeof g_chk_dir, "%s\\", dir);   // a script's mistakes name it without its folder
    char *text;
    struct xunit u;
    const char *why;
    int line;
    if (!script_load(path, &text, &u, &why, &line)) { printf("%s:%d: %s\n", path, line > 0 ? line : 1, why); g_chk_n++; }
}
int main(int argc, char **argv) {
    const char *ai = NULL;
    int n = 0;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--ai") && i + 1 < argc) ai = argv[++i];
        else n++;
    }
    if (!n) { fprintf(stderr, "usage: ageofai_check <personality folder | file.eeai> ... [--ai <personality folder>]\n"); return 2; }
    InitializeCriticalSection(&g_x_lock);
    x_host_install();
    g_x_told = chk_told;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--ai")) { i++; continue; }
        char full[MAX_PATH], dir[MAX_PATH], up[MAX_PATH];
        DWORD at = GetFullPathNameA(argv[i], MAX_PATH, full, NULL) ? GetFileAttributesA(full) : INVALID_FILE_ATTRIBUTES;
        if (at == INVALID_FILE_ATTRIBUTES) { printf("ageofai_check: %s: no such file or folder\n", argv[i]); g_chk_n++; continue; }
        if ((at & FILE_ATTRIBUTE_DIRECTORY) && chk_is_folder(full)) { chk_folder(full); continue; }
        if (at & FILE_ATTRIBUTE_DIRECTORY) {   // a folder of scripts (tests\scenarios): each of its .eeai files
            char pat[MAX_PATH], path[MAX_PATH];
            snprintf(pat, sizeof pat, "%s\\*.eeai", full);
            WIN32_FIND_DATAA fd;
            HANDLE h = FindFirstFileA(pat, &fd);
            if (h == INVALID_HANDLE_VALUE) continue;
            do {
                snprintf(path, sizeof path, "%s\\%s", full, fd.cFileName);
                chk_script(path, ai);
            } while (FindNextFileA(h, &fd));
            FindClose(h);
            continue;
        }
        lstrcpyA(dir, full);
        char *slash = strrchr(dir, '\\');
        if (slash) *slash = 0;
        lstrcpyA(up, dir);   // a file of mod\: the folder above it
        slash = strrchr(up, '\\');
        if (slash && !lstrcmpiA(slash + 1, "mod")) *slash = 0;
        if (chk_is_folder(dir)) chk_folder(dir);
        else if (chk_is_folder(up)) chk_folder(up);
        else chk_script(full, ai);
    }
    if (g_unit_nlater) printf("ageofai_check: %d unit.<identifier> name(s) not checked - the game's data checks them at its start\n", g_unit_nlater);
    printf("ageofai_check: %d mistake(s)\n", g_chk_n);
    return g_chk_n ? 1 : 0;
}
