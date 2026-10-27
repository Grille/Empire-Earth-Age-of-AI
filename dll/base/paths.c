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
