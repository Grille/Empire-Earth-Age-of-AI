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

// THE ORDER MODE (the player, 2026-11-11: "beside placment we should have an order mode ... order prefill the units
// with actions (attack moves, build orders etc) order modes should also have an clear action button"). In the units
// panel's Orders mode (editor_place.c) the right button on the map gives the selected units orders as the game's
// right-click gives a player's (EEAMouseClick::vf3 0x403CF1: its messages, carried out at once), and the scenario keeps
// them as a saved game keeps a unit's goals. The editor's selection holds any player's units: each owner's are made
// that player's selection (player_select) and the message carried out for it (msg_as_begin, as an AI army's) - its
// citizens apart where they get another order. What the order is, by what was under the cursor at a click (the
// editor's own pick, the unit its Placement mode deletes):
// - nothing, or a unit of the order's: a move to the point pressed, as a group move with the mod's formations (a drag:
//   facing the drag's way; a click: the way from the units' middle to the point, as the game's), Ctrl an attack-move;
// - a resource with some left (a mine, a tree, a forage, an animal - the world player's): the citizens gather it, the
//   others attack an animal and move to the rest;
// - a unit of a player the owner isn't allied with: an attack;
// - the owner's own or an ally's: the citizens repair it while it is damaged; the owner's own transport or (built)
//   fortress: all go in (EEMLoadTransport, as the game's click: the mod's load hook sees it); else a move to it.
// Shift queues the order after the units' others. Only units that move (buildings set no rally point here).

// whether u gathers and repairs: a citizen, on land or a fishing boat
internal int eo_citizen(ee_unit *u) {
    ee_unit_type *td = unit_get_type_data(u);
    int cls = td ? unit_type_get_class_id(td) : -1;
    return cls == CLS_LAND_CITIZEN || cls == CLS_NAVAL_CITIZEN;
}
// the order a unit of owner pl (a citizen or not) gets on target t: 0 a move to the point, 1 gather, 2 attack, 3
// repair, 4 go inside, 5 a move to t
internal int eo_kind(ee_player *pl, int citizen, ee_unit *t) {
    if (!t) return 0;
    ee_player *to = unit_get_owner(t);
    ee_unit_type *td = unit_get_type_data(t);
    int cls = td ? unit_type_get_class_id(td) : -1;
    if (to && player_get_index(to) == 0 && unit_get_amount(t) > 0)   // the world's: a resource
        return citizen ? 1 : cls == CLS_RESOURCE ? 5 : 2;
    if (!to || unit_get_hp(t) <= 0) return 5;
    if (to != pl && player_get_stance(pl, player_get_index(to)) != 0) return 2;
    ee_unit_stats *st = player_stats_of(to, unit_type_get_id(td));
    if (citizen && st && unit_get_hp(t) < unit_stats_get_hp(st)) return 3;
    int holds = cls == CLS_LAND_TRANSPORT || cls == CLS_HELICOPTER_TRANSPORT || cls == CLS_NAVAL_TRANSPORT || cls == CLS_FORTRESS;   // (the load's own test, 0x404B37)
    if (to == pl && holds && unit_get_finished(t)) return 4;   // (the right-click's: a fortress built)
    return 5;
}
// the units ids[n] (pl's) given order kind on target t (its id) or to the point at (facing face); queued, attack-move
internal void eo_send(ee_player *pl, uint32_t *ids, int n, int kind, ee_unit *t, vec2 at, float face, int queued, int attack) {
    static __attribute__((aligned(8))) uint8_t mem[0x40];
    uint32_t *sel[3] = { NULL, ids, ids + n };   // a select message's selection: its ids at +4 .. +8
    player_select(pl, sel);
    memset(mem, 0, sizeof mem);
    int from = msg_as_begin(pl);
    unsigned id = t ? (unsigned)unit_get_serial(t) : 0;
    if (kind == 1) gather_msg_make((ee_gather_msg *)mem, from, id, queued);
    else if (kind == 2) attack_msg_make((ee_attack_msg *)mem, from, id, queued);
    else if (kind == 3) repair_msg_make((ee_repair_msg *)mem, from, id, queued);
    else if (kind == 4) { load_msg_make((ee_load_msg *)mem, from, id, -1); load_msg_set_vtable((ee_load_msg *)mem); }
    else {
        if (kind == 5) { vec3 q = unit_get_pos(t); at = v2(q.x, q.y); }
        path_msg_make((ee_path_msg *)mem, from, at.x, at.y, 0, queued);
        path_msg_set_facing((ee_path_msg *)mem, face);
        if (attack) path_msg_set_forced((ee_path_msg *)mem, 0);
    }
    msg_execute((ee_msg *)mem);
    msg_as_end(pl);
}
// the editor's selected units given the order at the point at (a drag's facing drag, else none: NAN), on the unit of
// id target (UNIT_NONE none); returns how many got one
internal int eo_order(vec2 at, float drag, unsigned target, int queued, int attack) {
    ee_editor_form *f = g_ef_form;
    int n = f ? editor_selection_n(f) : 0;
    if (n < 1) return 0;
    uint32_t *ids = (uint32_t *)malloc((size_t)n * sizeof *ids);
    ee_unit **us = (ee_unit **)malloc((size_t)n * sizeof *us);
    int m = 0;
    vec2 mid = v2(0, 0);
    ee_unit *t = target != UNIT_NONE ? unit_by_id(target) : NULL;
    for (int i = 0; ids && us && i < n; i++) {   // the units that move, alive and on the map
        ee_unit *s = editor_selection_unit(f, i), *u = s ? unit_get_physics(s) : NULL;
        ee_unit_stats *st = u ? unit_get_stats(u) : NULL;
        if (!st || unit_stats_get_speed(st) <= 0 || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !unit_get_owner(u)) continue;
        vec3 q = unit_get_pos(u);
        mid = vec2_add(mid, v2(q.x, q.y));
        us[m++] = u;
    }
    for (int i = 0; t && i < m; i++) if (unit_get_serial(us[i]) == unit_get_serial(t)) t = NULL;   // one of its own clicked: the point
    int given = 0;
    if (m) {
        mid = vec2_scale(mid, 1.0f / (float)m);
        float face = isnan(drag) ? atan2f(-(at.x - mid.x), at.y - mid.y) : drag;
        static const char *const what[6] = { "move to", "gather at", "attack at", "repair at", "go inside at", "move to the unit at" };
        vec2 to = at;
        if (t) { vec3 q = unit_get_pos(t); to = v2(q.x, q.y); }
        for (int f0 = 0; f0 < m; f0++) {   // each owner's units (from its first): one order, or one for its citizens and one for the rest
            ee_player *pl = unit_get_owner(us[f0]);
            int seen = 0, p = player_get_index(pl);
            for (int j = 0; j < f0 && !seen; j++) seen = unit_get_owner(us[j]) == pl;
            if (seen) continue;
            int kc = eo_kind(pl, 1, t), kn = eo_kind(pl, 0, t), apart = kc != kn;
            for (int c = 0; c <= apart; c++) {   // c 1: the citizens
                int k = 0, kind = apart && c ? kc : kn;
                for (int i = 0; i < m; i++)
                    if (unit_get_owner(us[i]) == pl && (!apart || eo_citizen(us[i]) == c)) ids[k++] = (uint32_t)unit_get_serial(us[i]);
                if (!k) continue;
                eo_send(pl, ids, k, kind, t, at, face, queued, attack);
                given += k;
                if (g_log) log_locked("# editor order: P%d's %d units%s %s %.1f,%.1f%s%s\n", p, k, apart ? (c ? " (citizens)" : " (not citizens)") : "",
                                      what[kind], (double)(kind ? to.x : at.x), (double)(kind ? to.y : at.y), queued ? ", queued" : "", attack && !kind ? ", an attack-move" : "");
            }
        }
    }
    free(ids);
    free(us);
    return given;
}

// the right button's release on the map: in Orders mode the order, and not the editor's delete of the unit under the
// cursor (its drag flag set for the release: the editor deletes at a click only)
internal int __thiscall hook_eo_up(eeseuic_command_units *c) {
    if (!ep_orders() || !g_ef_form) return eeseuic_command_units_mouse_r_button_up(c);
    int drag = eeseuic_command_units_get_dragging(c) != 0;
    vec3 at = eeseuic_command_units_get_press_at(c);
    unsigned target = drag ? UNIT_NONE : editor_unit_under_mouse();
    eo_order(v2(at.x, at.y), drag ? eeseuic_command_units_get_drag_angle(c) : NAN, target, (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0,
             (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0);
    eeseuic_command_units_set_dragging(c, 1);
    int r = eeseuic_command_units_mouse_r_button_up(c);
    eeseuic_command_units_set_dragging(c, (uint8_t)drag);
    return r;
}

// the Clear orders box (editor_place.c): the selected units' orders dropped as the player's Halt drops them, every
// unit's when none is selected
internal void editor_orders_clear(void) {
    ee_editor_form *f = g_ef_form;
    int n = f ? editor_selection_n(f) : 0, k = 0, left = 0;
    for (int i = 0; i < n; i++) {
        ee_unit *s = editor_selection_unit(f, i), *u = s ? unit_get_physics(s) : NULL;
        ee_unit_stats *st = u ? unit_get_stats(u) : NULL;
        if (!st || unit_stats_get_speed(st) <= 0 || unit_orders(u) <= 0) continue;
        unit_halt(u);
        k++;
        left += unit_orders(u) > 0;
    }
    if (!n) k = eo_orders(1), left = eo_orders(0);
    if (g_log) log_locked("# editor: orders cleared, %d units' (%s), %d still hold some\n", k, n ? "the selected" : "none selected: every unit's", left);
}

internal void editor_orders_install(void) {
    int n = patch_editor_open(hook_eo_open);
    if (g_log) fprintf(g_log, "# editor: back from a test as before it %s\n", n ? "hooked" : "FAILED");
    n = patch_se_command_up(hook_eo_up);
    if (g_log) fprintf(g_log, "# editor: the order mode %s\n", n ? "hooked" : "FAILED");
}
