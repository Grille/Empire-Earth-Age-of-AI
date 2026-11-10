internal void rd_world_rebuild(int on);   // render_3d.c
// THE EDITOR'S UNDO (the player, 2026-11-10: "if you can make the undo actually work for all editor actions that would be
// great"). The game's Undo and Redo take the last action back by its vf2, empty in every editor action. Here an undo is
// the scenario as it was: before an action that changes it the scenario is written to a file of this game's folder
// undo\, as the quick save writes it, and undo loads that file through the game's own quick load (its call of the load
// is given the file instead of EEQuickSave.scn: the sound, the pause, the clock and the panel as it does them). That holds
// whatever the scenario holds, also what no action changes (a trigger written between two actions comes and goes with the
// state it was in; the loading text's action too: the panels fire it at every refresh, a load's as well, changed or not).
// A stroke of the paint tools is one state: the game makes an action per mouse move.
#define EU_STATES 64         // steps back: the oldest file goes
#define EU_STROKE_MS 500     // a paint action this soon after one of its kind goes on the same stroke
internal char g_eu_dir[MAX_PATH], g_eu_file[MAX_PATH];   // the files' folder; the file the next quick load reads instead
// the files by number: first .. top - 1 are there; cur is the state the scenario is in (cur == top: not written yet)
internal int g_eu_first, g_eu_cur, g_eu_top;
internal int g_eu_depth;             // an action inside another (a blank map paints its ground): the outer one's state holds
internal int g_eu_loading;           // the world is being put back: not a new game
internal void *g_eu_stroke_vt;       // the paint action's kind of the stroke, its last action's time
internal DWORD g_eu_stroke_at;
internal void *g_eu_ev_vt[2][UI_EVENT_SLOTS];   // the hotkeys' events' vtables

internal void eu_path(char *p, int i) { snprintf(p, MAX_PATH, "%s%d.scn", g_eu_dir, i); }
internal void eu_drop(int from, int to) {
    char p[MAX_PATH];
    for (int i = from; i < to; i++) { eu_path(p, i); DeleteFileA(p); }
}
// the folder made, a game's leftovers gone (a game closed with the editor open leaves its files)
internal void eu_folder(void) {
    if (g_eu_dir[0]) return;
    char d[MAX_PATH], p[MAX_PATH];
    lstrcpynA(d, g_game_dir, sizeof d);
    slot_dir(d);
    GetFullPathNameA(d, sizeof g_eu_dir, g_eu_dir, NULL);
    lstrcatA(g_eu_dir, "undo\\");
    CreateDirectoryA(g_eu_dir, NULL);
    snprintf(p, sizeof p, "%s*.scn", g_eu_dir);
    WIN32_FIND_DATAA f;
    HANDLE h = FindFirstFileA(p, &f);
    if (h == INVALID_HANDLE_VALUE) return;
    do { snprintf(p, sizeof p, "%s%s", g_eu_dir, f.cFileName); DeleteFileA(p); } while (FindNextFileA(h, &f));
    FindClose(h);
}
// the two files the same
internal int eu_same(int a, int b) {
    char p[MAX_PATH];
    int same = 0;
    eu_path(p, a);
    HANDLE fa = CreateFileA(p, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    eu_path(p, b);
    HANDLE fb = CreateFileA(p, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    DWORD na = fa != INVALID_HANDLE_VALUE ? GetFileSize(fa, NULL) : 0, nb = fb != INVALID_HANDLE_VALUE ? GetFileSize(fb, NULL) : 1;
    if (na == nb && na && na != INVALID_FILE_SIZE) {
        uint8_t *m = malloc((size_t)na * 2);
        DWORD ra = 0, rb = 0;
        if (m && ReadFile(fa, m, na, &ra, NULL) && ReadFile(fb, m + na, nb, &rb, NULL) && ra == na && rb == nb) same = !memcmp(m, m + na, na);
        free(m);
    }
    if (fa != INVALID_HANDLE_VALUE) CloseHandle(fa);
    if (fb != INVALID_HANDLE_VALUE) CloseHandle(fb);
    return same;
}
internal int eu_write(int i) {
    char p[MAX_PATH];
    DWORD t = GetTickCount();
    eu_folder();
    eu_path(p, i);
    int ok = scenario_save_to(p);
    if (g_log) log_locked("# editor undo: state %d %s in %lu ms\n", i, ok ? "kept" : "NOT KEPT", GetTickCount() - t);
    return ok;
}
// the game's quick load, of the file; the quick load's action does the rest
internal int eu_read(int i) {
    DWORD t = GetTickCount();
    ee_se_quick_load *a = se_quick_load_new();
    if (!a) return 0;
    eu_path(g_eu_file, i);
    g_eu_loading = 1;
    eesea_quick_save_load_run(a);
    g_eu_loading = 0;
    g_eu_file[0] = 0;
    ((void (__thiscall *)(void *, int))(*(void ***)a)[0])(a, 1);
    if (g_ep_panel && se_unit_get_placing(g_ep_panel)) se_unit_reselect(g_ep_panel);   // the ghosts went with the world
    g_eu_stroke_vt = NULL;
    if (g_log) log_locked("# editor undo: state %d loaded in %lu ms\n", i, GetTickCount() - t);
    return 1;
}
// what the quick load's call of the load does (and any quick load: no frame through the step while the world is rebuilt)
internal int __cdecl hook_eu_read(void *spec, int a, int b, uint8_t c) {
    rd_world_rebuild(1);
    int r = g_eu_file[0] ? scenario_read_from(g_eu_file, a, b, c) : scenario_read(spec, a, b, c);
    rd_world_rebuild(0);
    return r;
}

// before an action that changes the scenario: the state kept. stroke: a paint tool's
internal void eu_before(void *vt, int stroke) {
    if (g_eu_depth++ || g_eu_loading) return;
    DWORD now = GetTickCount();
    int same = stroke && vt == g_eu_stroke_vt && now - g_eu_stroke_at < EU_STROKE_MS;
    g_eu_stroke_vt = stroke ? vt : NULL;
    g_eu_stroke_at = now;
    if (same || !eu_write(g_eu_cur)) return;
    // nothing changed since the state before (a tool's action that did nothing, a panel's): one state, not an undo that shows nothing
    if (g_eu_cur > g_eu_first && eu_same(g_eu_cur - 1, g_eu_cur)) { if (g_eu_top == g_eu_cur) eu_drop(g_eu_cur, g_eu_cur + 1); return; }
    eu_drop(g_eu_cur + 1, g_eu_top);   // what was undone can't be redone after this
    g_eu_top = ++g_eu_cur;
    if (g_eu_cur - g_eu_first > EU_STATES) eu_drop(g_eu_first, g_eu_first + 1), g_eu_first++;
}
internal void eu_after(void) { g_eu_depth--; }

internal void __thiscall hook_eu_undo(ee_se_undo *a) {
    (void)a;
    if (g_eu_cur <= g_eu_first) { if (g_log) log_locked("# editor undo: nothing to undo\n"); return; }
    if (g_eu_top == g_eu_cur) { if (!eu_write(g_eu_cur)) return; g_eu_top = g_eu_cur + 1; }   // for the redo
    if (eu_read(g_eu_cur - 1)) g_eu_cur--;
}
internal void __thiscall hook_eu_redo(ee_se_redo *a) {
    (void)a;
    if (g_eu_cur + 1 >= g_eu_top) { if (g_log) log_locked("# editor undo: nothing to redo\n"); return; }
    if (eu_read(g_eu_cur + 1)) g_eu_cur++;
}

// a new game or another scenario: the states were another's
internal void editor_undo_new_game(void) {
    if (g_eu_loading) return;
    if (g_eu_dir[0]) eu_drop(g_eu_first, g_eu_top);
    g_eu_first = g_eu_cur = g_eu_top = 0;
    g_eu_stroke_vt = NULL;
}

internal uint32_t __thiscall eu_key_undo(void *ev, void *form, uint32_t arg) { (void)ev, (void)form, (void)arg; se_undo_queue(0); return 0; }
internal uint32_t __thiscall eu_key_redo(void *ev, void *form, uint32_t arg) { (void)ev, (void)form, (void)arg; se_undo_queue(1); return 0; }
internal void editor_undo_hotkeys(ee_editor_form *form) {
    // (the form is made once: a game's second editor has its hotkeys)
    void *undo = ui_event_own(g_eu_ev_vt[0], (void *)eu_key_undo), *redo = ui_event_own(g_eu_ev_vt[1], (void *)eu_key_redo);
    if (undo) ui_form_add_ctrl_hotkey(form, 0x2C, undo);   // DIK_Z
    if (redo) ui_form_add_ctrl_hotkey(form, 0x15, redo);   // DIK_Y
}

#define EU_HOOK(site, klass, stroke) \
    internal void __thiscall hook_eu_##site(ee_##site *a) { eu_before(*(void **)a, stroke); eesea_##klass##_run(a); eu_after(); }
EU_HOOK(se_place_unit, place_unit, 0)
EU_HOOK(se_place_wall, place_wall, 0)
EU_HOOK(se_place_gate, place_gate, 0)
EU_HOOK(se_delete, delete_unit, 0)
EU_HOOK(se_move, move_unit, 0)
EU_HOOK(se_rotate, rotate_units, 0)
EU_HOOK(se_player_count, change_world_player_count, 0)
EU_HOOK(se_blank_map, create_blank_map, 0)
EU_HOOK(se_random_map, create_random_map, 0)
EU_HOOK(se_tech_tree, tech_tree, 0)
EU_HOOK(se_paint_area, paint_area, 1)
EU_HOOK(se_paint_elevation, paint_elevation, 1)
EU_HOOK(se_paint_mountains, paint_moutains, 1)
EU_HOOK(se_paint_terrain, paint_terrain, 1)

internal void editor_undo_install(void) {
    int n = patch_se_place_unit(hook_eu_se_place_unit) + patch_se_place_wall(hook_eu_se_place_wall) + patch_se_place_gate(hook_eu_se_place_gate)
            + patch_se_delete(hook_eu_se_delete) + patch_se_move(hook_eu_se_move) + patch_se_rotate(hook_eu_se_rotate)
            + patch_se_player_count(hook_eu_se_player_count)
            + patch_se_blank_map(hook_eu_se_blank_map) + patch_se_random_map(hook_eu_se_random_map) + patch_se_tech_tree(hook_eu_se_tech_tree)
            + patch_se_paint_area(hook_eu_se_paint_area) + patch_se_paint_elevation(hook_eu_se_paint_elevation)
            + patch_se_paint_mountains(hook_eu_se_paint_mountains) + patch_se_paint_terrain(hook_eu_se_paint_terrain)
            + patch_se_undo(hook_eu_undo) + patch_se_redo(hook_eu_redo);
    int read = patch_se_quick_read(hook_eu_read);
    if (g_log) fprintf(g_log, "# editor: undo %s\n", n == 16 && read ? "hooked" : "FAILED");
}
