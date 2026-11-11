// A TEST'S ORDERS STAY IN THE TEST (the player, 2026-11-11: "when giving an move command in a test game, exit back to
// the editor, and run test again, the units start to move having still the same command, adn if i save in editor this
// behavior gets saved too, grand battle is alredy poisend by this"; then: "having the orders is a nice thing ... the
// problems was that it spilled over from the test game"). The editor's Test writes the scenario to
// data\Scenarios\ScenDefault.scn (a saved game: the units' orders in it) and plays the editor's own world; back from it,
// the form's open puts the units where they stood but leaves them the orders they ended the test with, and the editor's
// next save wrote those into the scenario ("grand battle" started with 486 of its 1012 units marching off). So the file
// is read back over the world once the form is open again (the quick load's reader, scenario_read): the units as they
// were before the test, the orders given in the editor with them. The file is the game folder's, shared by every game
// that runs there (a test's of another session too): its copy in this game's own folder is read, made when the save of
// the test is done (persist.c's save hook). Without one, the units' orders are dropped (as the player's Halt does).
internal char g_eo_copy[MAX_PATH];
internal int g_eo_have;

// the save of a game done (persist.c): the test's file copied
internal void editor_test_saved(const char *folder, const char *file) {
    if (lstrcmpiA(file, "ScenDefault.scn")) return;
    char from[MAX_PATH * 2], dir[MAX_PATH];
    lstrcpynA(dir, g_game_dir, MAX_PATH);
    slot_dir(dir);
    snprintf(from, sizeof from, "%s%s%s", g_game_dir, folder, file);
    snprintf(g_eo_copy, sizeof g_eo_copy, "%seditor_test.scn", dir);
    g_eo_have = CopyFileA(from, g_eo_copy, FALSE) != 0;
    if (g_log) log_locked("# editor: the test's scenario %s to %s\n", g_eo_have ? "copied" : "NOT copied", g_eo_copy);
}

// how many units hold orders, their orders dropped with halt (those on the map that move, not carried or garrisoned)
internal int eo_orders(int halt) {
    int n = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit_stats *st = unit_get_stats(it.u);
        if (!st || unit_stats_get_speed(st) <= 0 || unit_orders(it.u) <= 0) continue;   // (a tree, a mine: no goal slots)
        if (halt) unit_halt(it.u);
        n++;
    }
    return n;
}

internal void __thiscall hook_eo_open(ee_editor_form *f) {
    int back = editor_form_get_testing(f) != 0;
    editor_form_open(f);
    if (!back) return;
    groupmove_new_game();   // the test's marches over: their linger would send the units back to the test's spots
    int n = eo_orders(1), left = eo_orders(0);   // (the read gives a unit the file's orders, but leaves it its own when the file has none)
    __attribute__((aligned(8))) uint8_t spec[0x100] = { 0 };
    int read = -1;
    if (g_eo_have && fs_spec_init(spec, g_eo_copy)) {
        read = hook_eu_read(spec, 1, 1, 1);   // (the renderer kept off the world while it is made, the undo's steps gone)
        fs_spec_free(spec);
    }
    g_eo_have = 0;
    if (g_log) log_locked("# editor: back from a test, %d units' orders of it dropped (%d left), %s, %d units hold orders\n", n, left,
                          read == 0 ? "the scenario read as it was before it" : "the scenario NOT read again: the editor's orders lost", eo_orders(0));
}

internal void editor_orders_install(void) {
    int n = patch_editor_open(hook_eo_open);
    if (g_log) fprintf(g_log, "# editor: back from a test as before it %s\n", n ? "hooked" : "FAILED");
}
