// WHERE THINGS ARE: the game folder, the settings files, the personalities, the dumps - and a test slot's own folder.
internal char g_ini[MAX_PATH], g_aimod_ini[MAX_PATH];   // aimod_autotest.ini, aimod.ini (paths set in DllMain)
// <game>\ and <game>\aimod_ai\ (with the backslash). Under autotest the config and the personality folder come from
// aimod_autotest.ini [autotest] config / ai_dir (run_test.ps1: the repository's config\ by default), so tests measure
// the shipped files, not the player's own ..\aimod.ini; normal play: aimod.ini next to the game, the personalities
// from its [general] ai_dir (a developer's game reading the repository's files) or next to the game.
internal char g_game_dir[MAX_PATH], g_ai_dir[MAX_PATH];
internal int g_slot;   // a test slot (TEST SLOTS, at slot_dir), 0: none
internal char g_dump_dir[MAX_PATH];   // also the folder of the control pipe's shot / ddinfo dump pictures

// TEST SLOTS: parallel test games (run_test.ps1 -Slot n; docs/testing.md, Parallel runs). The runner starts slot
// n (1..30) with EEAIMOD_SLOT=n and EEAIMOD_SLOT_DIR=<its folder>: the test ini, the log and the dumps are there instead
// of the game folder, the control pipe is \\.\pipe\eeaimod<n>, the engine's network ports are moved by 2n
// (slot_ports) - and the runner patches the game's single-instance check (docs/engine-notes.md, Exe builds). 0: none.
internal void slot_dir(char *dir) {   // dir: the game folder (with its \) -> where this game's test files are
    char v[MAX_PATH];
    g_slot = GetEnvironmentVariableA("EEAIMOD_SLOT", v, sizeof v) ? atoi(v) : 0;
    if (g_slot < 0 || g_slot > 30) g_slot = 0;
    if (g_slot && GetEnvironmentVariableA("EEAIMOD_SLOT_DIR", v, sizeof v) && v[0]) {
        size_t n = strlen(v);
        snprintf(dir, MAX_PATH, "%s%s", v, v[n - 1] == '\\' || v[n - 1] == '/' ? "" : "\\");
    }
}
