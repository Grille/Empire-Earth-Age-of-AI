// THE EDITOR'S FORMATIONS (the player, 2026-11-10: "its alredy possible to select groups of units, with a right drag
// click they could be placed into formation like they would in the actual game"; then "maybe an dropbox with available
// formations? with rotate being one of them"). The editor's right-button drag (its command control,
// EESEUICCommandUnits) queues a turn of the selected units to the drag's facing at every move of the mouse
// (EESEARotateUnits: each where it stands). With a formation chosen in the units panel's list (editor_place.c; Rotate
// is the game's turn) the drag lays them out instead, as a group move puts them with the mod's own layout (groupmove.c
// fo_rows and fo_rings: by the units' sizes, the close fighters in front, a line at least the game's 5 wide): a line's
// front row or a wedge's tip on the point pressed facing the drag's way, a circle round it, its units facing out or in -
// placed at once, and again at every move of the mouse from where they stood when the drag began, so they follow the
// drag. Only units that move (their stats' speed: buildings, walls and trees stay), at most FO_MAX. A unit whose spot is
// off the map or on ground its type can't stand on stays where it stood. It runs inside the turn's action
// (editor_undo.c's hook), so the undo takes a drag as one step. g_fo is the group moves' table: the editor makes none.
#define EF_FRONT 5            // the game's short line: 5 columns
#define EF_GROUND 0.05f       // a unit this near its ground stands on it (else it floats or flies: its height kept)
struct ef_unit { ee_unit *u; float m[16], w; int rank; };   // m: its matrix when the drag began
internal struct ef_unit g_ef[FO_MAX];
internal int g_ef_n, g_ef_kind;
internal int g_ef_mode;              // the drag going on: 0 not decided yet, 1 a turn (the game's), 2 a formation
internal vec3 g_ef_at;               // where the drag began: the formation's front, a circle's middle
internal eeseuic_command_units *g_ef_ctl;
internal ee_editor_form *g_ef_form;  // the editor's form (made once)

internal void editor_formation_form(ee_editor_form *form) { g_ef_form = form; }

// the drag, at its first turn: Rotate chosen a turn, else a formation of the selected units that move
internal int ef_decide(int *selected) {
    g_ef_n = 0;
    *selected = 0;
    g_ef_kind = ep_form_kind();
    if (!g_ef_ctl || !g_ef_form) return 1;
    int n = *selected = editor_selection_n(g_ef_form);
    if (g_ef_kind < 0) return 1;
    for (int i = 0; i < n && g_ef_n < FO_MAX; i++) {
        ee_unit *s = editor_selection_unit(g_ef_form, i), *u = s ? unit_get_physics(s) : NULL;
        ee_unit_stats *st = u ? unit_get_stats(u) : NULL;
        if (!st || unit_stats_get_speed(st) <= 0) continue;
        struct ef_unit *e = &g_ef[g_ef_n++];
        float size = unit_get_size(u);
        e->u = u;
        memcpy(e->m, unit_matrix_at(u), sizeof e->m);
        e->w = (size < FO_MIN_SIZE ? FO_MIN_SIZE : size) * FO_PAD[g_ef_kind];
        e->rank = fo_rank(u);
    }
    g_ef_at = eeseuic_command_units_get_press_at(g_ef_ctl);
    return g_ef_n ? 2 : 1;
}

// may u stand at p: within the map's diamond, on a tile its type may stand on
internal int ef_may_stand(ee_map *map, ee_unit *u, vec2 p) {
    float xy[2] = { p.x, p.y };
    int size = (int)map_get_size(map), x = (int)p.x, y = (int)p.y;
    if (p.x < 0 || p.y < 0 || x >= size || y >= size || !spawn_on_map(map, xy)) return 0;
    ee_tile *t = map_get_tiles(map)[y * size + x];
    ee_unit_type *td = unit_get_type_data(u);
    return t && td && unit_type_terrain_ok(td, tile_get_kind(t));
}

// the units laid out facing angle (a heading: atan2(-dx, dy), (-sin, cos) ahead), each put at its spot turned to it (a
// circle's to face out or in)
internal void ef_lay(float angle) {
    ee_map *map = game_map();
    int n = g_ef_n, circle = g_ef_kind >= FORMATION_INWARD_CIRCLE;
    if (!map || n < 1) return;
    vec2 fwd = v2(-sinf(angle), cosf(angle)), right = v2(fwd.y, -fwd.x), c = v2(g_ef_at.x, g_ef_at.y), mean = v2(0, 0);
    for (int i = 0; i < n; i++) mean = vec2_add(mean, v2(g_ef[i].m[3], g_ef[i].m[7]));
    mean = vec2_scale(mean, 1.0f / (float)n);
    for (int i = 0; i < n; i++) {   // their places by where they stood along and across the facing (a circle's round the point)
        struct fo_unit *f = &g_fo[i];
        vec2 d = vec2_sub(v2(g_ef[i].m[3], g_ef[i].m[7]), mean), r = vec2_sub(v2(g_ef[i].m[3], g_ef[i].m[7]), c);
        memset(f, 0, sizeof *f);
        f->u = g_ef[i].u;
        f->w = g_ef[i].w;
        f->rank = g_ef[i].rank;
        f->along = vec2_dot(d, fwd);
        f->across = vec2_dot(d, right);
        f->p = v2(vec2_dot(r, right), vec2_dot(r, fwd));
        g_fo_order[i] = i;
    }
    if (circle) fo_rings(g_fo_order, n); else fo_rows(g_fo_order, n, g_ef_kind, EF_FRONT);
    for (int i = 0; i < n; i++) {
        const struct ef_unit *e = &g_ef[i];
        vec2 a = g_fo[i].at, off = vec2_add(vec2_scale(right, a.x), vec2_scale(fwd, a.y)), s = vec2_add(c, off);
        float m[16], face = angle;
        if (circle && vec2_len(off) > 1e-3f) face = g_ef_kind == FORMATION_OUTWARD_CIRCLE ? atan2f(-off.x, off.y) : atan2f(off.x, -off.y);
        memcpy(m, e->m, sizeof m);
        if (ef_may_stand(map, e->u, s)) {
            float g0 = ground_at(map, e->m[3], e->m[7]), g1 = ground_at(map, s.x, s.y), z = e->m[11];
            if (fabsf(z - g0) <= EF_GROUND) z = g1;
            else z += (g1 > 0 ? g1 : 0) - (g0 > 0 ? g0 : 0);   // a ship, a flier: as high over the water or the ground as it was
            transform_orient(m, face, 0, 0);
            m[3] = s.x, m[7] = s.y, m[11] = z;
        }
        unit_place_matrix(e->u, m, 1);
    }
}

// the turn's action (editor_undo.c's hook): 1 when the drag is a formation's (laid out here), 0 the game's turn runs
internal int editor_formation_turn(ee_se_rotate *a) {
    if (!g_ef_mode) {
        int selected = 0;
        g_ef_mode = ef_decide(&selected);
        if (g_log) {
            vec2 from = v2(0, 0);
            for (int i = 0; i < g_ef_n; i++) from = vec2_add(from, v2(g_ef[i].m[3], g_ef[i].m[7]));
            if (g_ef_n) from = vec2_scale(from, 1.0f / (float)g_ef_n);
            if (g_ef_mode == 2) log_locked("# editor formation: %d of %d selected, kind %d, from about %.1f,%.1f laid out at %.1f,%.1f\n", g_ef_n, selected, g_ef_kind, (double)from.x, (double)from.y, (double)g_ef_at.x, (double)g_ef_at.y);
            else log_locked("# editor formation: none, the turn (%d selected, %s)\n", selected, g_ef_kind < 0 ? "Rotate chosen" : "none of them moves");
        }
    }
    if (g_ef_mode != 2) return 0;
    ef_lay(se_rotate_get_angle(a));
    return 1;
}

// the control's mouse move while the right button is down: the press clears its drag, so a drag decides afresh
internal int __thiscall hook_ef_move(eeseuic_command_units *c, void *at) {
    g_ef_ctl = c;
    if (!eeseuic_command_units_get_dragging(c)) g_ef_mode = 0;
    return eeseuic_command_units_mouse_move(c, at);
}

internal void editor_formation_install(void) {
    int n = patch_se_command_move(hook_ef_move);
    if (g_log) fprintf(g_log, "# editor: formations %s\n", n ? "hooked" : "FAILED");
}
