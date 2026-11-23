// AoC'S DATA ORDER (`aoc_data_order`, Art of Conquest only): AoC's data.ssa holds only what AoC changed - the rest
// comes from the 2017 update's folder, so its search path ([data_game_file_path], game/api/files.c) has two roots: its
// own folder and the 2017 update's (the registry's), a directory Data\ and an archive Data\data.ssa each. The engine's
// FindFile looks in every directory before any archive, so a loose file in the 2017 update's Data\ beat AoC's own
// archive: the player's Data\DB\dbworld.dat (84 rows, the 2017 update's table with a unit limit of its own) gave AoC
// the 2017 update's world table, whose rows end before AoC's 95 - the reload's random factor (rows +0x984 / +0x988:
// 0.8 and 1.2 in AoC's file) was read past its end, from the heap: 0 or a pointer's tiny float, a reload of nothing
// (2026-11-05: a howitzer fired every half second, the hunters never landed a blow). On: the main path's lookups go
// through two paths of their own - AoC's directory and archive, then the 2017 update's - so AoC's files come first,
// loose or packed, and the 2017 update's loose ones still beat its archive.
internal void *g_do_main, *g_do_paths[2];
internal volatile LONG g_do_state;   // 0 not built, 1 building, 2 in use, -1 the main path isn't the two roots expected
internal int g_do_found;             // the 2017 update's install found again and added to the path's end (original_find)

// THE 2017 UPDATE'S INSTALL FOUND AGAIN (a player's crash, 2026-11-23: AoC stopped at every start, its music's folder
// missing - engine/sound.c THE MUSIC'S FOLDER). AoC takes the 2017 update's folder from that game's registry values
// (game.cs original_volume / original_directory) - its second root of the data files (the archive and Data\ that hold
// everything AoC didn't change), the music, the check for the original game. That PC's registry still named the folder
// the game had been installed to before it was moved, and AoC ran on its own archive alone. ageofai.dll loads after
// the search path is set up (the engine's DLL scan, PIMInitialize 0x5412D5, comes after 0x53FE97) and before the
// first data file is looked up through it: so here, when that folder has no Data\data.ssa, the 2017 update's folder
// beside AoC's (..\Empire Earth\) takes its place - the two values set to it (the music, everything after) and its
// Data\ and archive added to the path's end, after the stale ones, which find nothing. A notice tells the player, the
// registry stays as it is.
internal int original_has_data(const char *dir) {
    char f[MAX_PATH * 2];
    snprintf(f, sizeof f, "%sData\\data.ssa", dir);
    DWORD a = GetFileAttributesA(f);
    return a != INVALID_FILE_ATTRIBUTES && !(a & FILE_ATTRIBUTE_DIRECTORY);
}
internal void original_find(void) {
    void *vs = original_volume(), *ds = original_directory(), *main = fs_main_path();
    if (!vs || !ds || !main || !g_game_dir[0]) return;
    const char *v = wstring_text(vs), *d = wstring_text(ds);
    char was[MAX_PATH * 2], at[MAX_PATH * 2], full[MAX_PATH];
    snprintf(was, sizeof was, "%s%s", v ? v : "", d ? d : "");
    if (was[0] && original_has_data(was)) return;
    snprintf(at, sizeof at, "%s..\\Empire Earth\\", g_game_dir);   // g_game_dir ends in a backslash
    DWORD n = GetFullPathNameA(at, sizeof full, full, NULL);
    if (!n || n >= sizeof full || !original_has_data(full) || full[1] != ':') {
        notice("the original game's folder isn't where its registry says", "%s has no Data\\data.ssa and %s neither - "
               "Art of Conquest runs without the original game's data (HKCU\\Software\\SSSI\\Empire Earth: Installed From Volume / Directory)",
               was[0] ? was : "(none)", at);
        return;
    }
    CharUpperA(full);   // as AoC's settings keep them
    char vol[3] = { full[0], ':', 0 };
    ustring_set(vs, vol);
    ustring_set(ds, full + 2);
    char data[MAX_PATH + 16], arc[MAX_PATH + 32];
    snprintf(data, sizeof data, "%sDATA\\", full);
    snprintf(arc, sizeof arc, "%sDATA\\DATA.SSA", full);
    int added = fs_path_add(main, data, arc);
    g_do_found = added == 2;
    notice("the original game's folder isn't where its registry says", "%s has no Data\\data.ssa - the one beside Art of Conquest "
           "taken: %s (its data %s; the registry stays: HKCU\\Software\\SSSI\\Empire Earth, Installed From Volume / Directory)",
           was[0] ? was : "(none)", full, added == 2 ? "added" : "NOT added");
}

internal long __thiscall hook_fs_find(void *path, const void *name, void **found, void *spec) {
    void *main = fs_main_path();
    if (!main || path != main) return g_fs_find(path, name, found, spec);
    if (!g_do_state && !InterlockedCompareExchange(&g_do_state, 1, 0)) {   // the first lookup on the main path: built
        int dirs = fs_dir_count(main), archives = fs_archive_count(main);
        if (dirs == 2 + g_do_found && archives >= 2 && archives <= 2 + g_do_found) {   // the 2017 update's the last: found again, the stale ones before it
            g_do_paths[0] = fs_path_of(main, 0, 0);
            g_do_paths[1] = fs_path_of(main, dirs - 1, archives - 1);
        }
        g_do_main = main;
        InterlockedExchange(&g_do_state, g_do_paths[0] && g_do_paths[1] ? 2 : -1);
        if (g_log) log_locked("# data order: %s (the search path: %d directories, %d archives)\n", g_do_state == 2
                              ? "AoC's folder first, loose and packed, then the 2017 update's" : "left as the game has it", dirs, archives);
    }
    if (g_do_state != 2 || path != g_do_main) return g_fs_find(path, name, found, spec);
    for (int i = 0; i < 2; i++)
        if (!g_fs_find(g_do_paths[i], name, found, spec)) return 0;
    return g_fs_find(path, name, found, spec);   // in neither: the game's own answer (not found)
}

internal void data_order_install(void) {
    if (build_is("aoc")) original_find();
    if (!build_is("aoc") || !g_set.aoc_data_order) return;
    int ok = patch_fs_find(hook_fs_find);
    if (g_log) fprintf(g_log, "# data order: aoc_data_order=1 (%s)\n", ok ? "FindFile hooked" : "FAILED: FindFile not as expected");
}

