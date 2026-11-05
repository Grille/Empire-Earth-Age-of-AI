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

internal long __thiscall hook_fs_find(void *path, const void *name, void **found, void *spec) {
    void *main = fs_main_path();
    if (!main || path != main) return g_fs_find(path, name, found, spec);
    if (!g_do_state && !InterlockedCompareExchange(&g_do_state, 1, 0)) {   // the first lookup on the main path: built
        int dirs = fs_dir_count(main), archives = fs_archive_count(main);
        if (dirs == 2 && archives == 2) {
            g_do_paths[0] = fs_path_of(main, 0, 0);
            g_do_paths[1] = fs_path_of(main, 1, 1);
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
    if (!build_is("aoc") || !g_set.aoc_data_order) return;
    int ok = patch_fs_find(hook_fs_find);
    if (g_log) fprintf(g_log, "# data order: aoc_data_order=1 (%s)\n", ok ? "FindFile hooked" : "FAILED: FindFile not as expected");
}

