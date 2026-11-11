// NO ORDERS IN A SCENARIO (the player, 2026-11-11: "when giving an move command in a test game, exit back to the editor,
// and run test again, the units start to move having still the same command, adn if i save in editor this behavior gets
// saved too, grand battle is alredy poisend by this"). Units come back to the editor from a test with the orders they
// were given there, and the editor writes a unit's goals as a saved game does: his "grand battle" started with 486 of its
// 1012 units marching off. The editor gives no orders of its own, so every unit's orders are dropped (as the player's
// Halt drops them, unit_halt) before the editor saves the scenario, quick-saves it or starts a test - a scenario saved
// once is clean again. The units on the map that move (the grid), not those carried or garrisoned.

internal void eo_halt_all(const char *why) {
    int n = 0, units = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit_stats *st = unit_get_stats(it.u);
        if (!st || unit_stats_get_speed(st) <= 0) continue;   // (a tree, a mine: no goal slots)
        units++;
        if (unit_orders(it.u) <= 0) continue;
        unit_halt(it.u);
        n++;
    }
    if (g_log && n) log_locked("# editor: %d of %d units' orders dropped before the %s\n", n, units, why);
}

internal void __thiscall hook_eo_save(ee_se_save *a) { eo_halt_all("save"); eesea_save_scenario_run(a); }
internal void __thiscall hook_eo_quick_save(ee_se_quick_save *a) { eo_halt_all("quick save"); eesea_quick_save_scenario_run(a); }
internal void __thiscall hook_eo_test(ee_se_test *a) { eo_halt_all("test"); eesea_test_scenario_run(a); }

internal void editor_orders_install(void) {
    int n = patch_se_save(hook_eo_save) + patch_se_quick_save(hook_eo_quick_save) + patch_se_test(hook_eo_test);
    if (g_log) fprintf(g_log, "# editor: orders dropped before a save or a test %s\n", n == 3 ? "hooked" : "FAILED");
}
