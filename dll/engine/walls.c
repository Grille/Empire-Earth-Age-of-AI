// WALLS ON SLOPES (`wall_slopes`): the wall's layout (unit_functions.wall_piece 0x638628, for every piece it lays or
// joins anew) turns a piece and picks its model by its place in the wall - straight (anim 9), diagonal (10), the
// round post at an end or a corner (11) - and leaves it flat at the height of its tile's corner x, y: on a hill one
// end floats, the other sinks, and neighbours at other heights stick out of each other in steps. Its sloped pieces
// (6 straight, 3 diagonal) have one slope built in (0.42 over a piece) and come only where the ground between two
// tiles rises 0.4 or more - on Highlands 90% of the neighbouring corners differ by less than 0.27; the corners it
// counts for them (0x517C10) it never reads. Here a piece leans onto the ground instead: its matrix (unit +0x130, the
// engine's GETransformation - drawn, saved and loaded as written, damage leaves it, 2026-11-03) gets a shear along its
// length, z rising with the ground from one end to the other (its sides stay upright), its middle on that line and
// set into the ground by the ground's fall below the line across its thickness and a little more, taller by as much
// (its top stays). A sloped piece becomes the plain one first. A post stands on the lowest corner of its tile, its
// top where it was. The layout's last step (wall_joints) makes a piece where walls meet a post anew, flat: the wall's
// pieces are fitted again after it. Pieces laid before (a loaded game) are fitted at the game's first step. A piece's collapse when
// destroyed turns it by its sheared length (EEPMBuilding::vf12) - it tips over as it sinks.
#define WS_SLOPE_DIAG 3     // the layout's animations (0x63842B): a diagonal piece with its slope built in
#define WS_SLOPE 6          //   a straight one with its slope
#define WS_STRAIGHT 9
#define WS_DIAG 10
#define WS_POST 11
#define WS_SINK 0.06f       // into the ground beyond what the ground's fall across the piece needs
#define WS_SINK_MAX 0.5f    // a piece on a cliff's edge: no deeper than this
#define WS_HALF_THICK 0.25f // a piece's sides from its middle line (the meshes: 0.21 .. 0.31)
#define WS_HEIGHT 1.05f     // a piece's mesh (Copper 0.98 .. Nano 1.21): taller by the sink over this, its top stays
#define WS_POST_HEIGHT 1.28f

internal int g_ws_on, g_ws_fitted_game;
internal uint32_t g_ws_t0;   // the game time first seen: a loaded game's while it loads, before its units are there
internal unsigned g_ws_pieces, g_ws_posts, g_ws_unsloped, g_ws_skipped;

// the ground's lowest point below the line from a to b (heights ha, hb) across the piece's thickness (side sx, sy):
// at both ends and the middle, on the line and to either side - how deep the piece must stand to touch it everywhere
internal float ws_fall(const ee_map *map, float ax, float ay, float bx, float by, float ha, float hb, float sx, float sy) {
    float fall = 0;
    for (int k = 0; k <= 2; k++) {
        float t = (float)k * 0.5f, x = ax + (bx - ax) * t, y = ay + (by - ay) * t, line = ha + (hb - ha) * t;
        for (int s = -1; s <= 1; s++) {
            float g = ground_at(map, x + (float)s * WS_HALF_THICK * sx, y + (float)s * WS_HALF_THICK * sy);
            if (g == g && line - g > fall) fall = line - g;
        }
    }
    return fall;
}

// one piece onto the ground; 0 when it isn't a piece of the layout's or its ground is off the map
internal int ws_fit(ee_unit *u) {
    ee_map *map = game_map();
    if (!map || !u) return 0;
    int anim = unit_get_anim(u);
    if (anim == WS_SLOPE) { unit_start_anim(u, WS_STRAIGHT, -1, 0); g_ws_unsloped++; }
    else if (anim == WS_SLOPE_DIAG) { unit_start_anim(u, WS_DIAG, -1, 0); g_ws_unsloped++; }
    else if (anim != WS_STRAIGHT && anim != WS_DIAG && anim != WS_POST) return 0;
    float px = unit_get_matrix(u, 3), py = unit_get_matrix(u, 7);
    if (anim == WS_POST) {
        float mid = ground_at(map, px, py), low = mid;
        for (int k = 0; k < 4; k++) {
            float g = ground_at(map, px + (k & 1 ? 0.5f : -0.5f), py + (k & 2 ? 0.5f : -0.5f));
            if (g < low) low = g;
        }
        if (mid != mid || low != low) { g_ws_skipped++; return 0; }
        // its scale from its turn (x, y), so fitting it again gives the same: as tall above mid as it was
        float z = low - WS_SINK, s = sqrtf(unit_get_matrix(u, 0) * unit_get_matrix(u, 0) + unit_get_matrix(u, 4) * unit_get_matrix(u, 4));
        unit_set_matrix(u, 8, 0);
        unit_set_matrix(u, 9, 0);
        unit_set_matrix(u, 10, s * (1 + (mid - z) / WS_POST_HEIGHT));
        unit_set_matrix(u, 11, z);
        g_ws_posts++;
        return 1;
    }
    // its length: the local y axis (its scale, 1 for walls, kept); its ends half a tile out along it - an edge's middle
    // (straight) or a corner (diagonal), where the ground's corners are exact
    float fx = unit_get_matrix(u, 1), fy = unit_get_matrix(u, 5), len = sqrtf(fx * fx + fy * fy);
    if (len < 0.01f) { g_ws_skipped++; return 0; }
    fx /= len; fy /= len;
    float d = 0.5f * (fabsf(fx) + fabsf(fy));
    float ax = px - d * fx, ay = py - d * fy, bx = px + d * fx, by = py + d * fy;
    float ha = ground_at(map, ax, ay), hb = ground_at(map, bx, by);
    if (ha != ha || hb != hb) { g_ws_skipped++; return 0; }
    float sink = WS_SINK + ws_fall(map, ax, ay, bx, by, ha, hb, fy, -fx);
    if (sink > WS_SINK_MAX) sink = WS_SINK_MAX;
    const float m[12] = {
        fy * len,  fx * len,                        0, px,
        -fx * len, fy * len,                        0, py,
        0,         (hb - ha) / (2 * d) * len,       len * (1 + sink / WS_HEIGHT), (ha + hb) * 0.5f - sink,
    };
    for (int i = 0; i < 12; i++) unit_set_matrix(u, i, m[i]);
    g_ws_pieces++;
    return 1;
}

internal void __cdecl hook_wall_piece(ee_unit *u, int code, int raised) {
    wall_piece(u, code, raised);
    if (g_ws_on) ws_fit(u);
}

// where walls meet or cross, the layout's last step (unit_functions.wall_joints 0x62E73B) makes a piece with more than
// two neighbours a post, its matrix anew and flat: the wall's pieces fitted again (a fit twice gives the same)
internal void __cdecl hook_wall_joints(ee_unit_vec *pieces, void *tiles, ee_player *pl, ee_map *map) {
    wall_joints(pieces, tiles, pl, map);
    if (!g_ws_on || !pieces) return;
    for (ee_unit **p = unit_vec_get_begin(pieces), **end = unit_vec_get_end(pieces); p && p < end; p++)
        if (*p && unit_get_hp(*p) > 0) ws_fit(*p);
}

// the walls a game starts with (a loaded game, a scenario's): once its time has moved on from the first seen - a
// loaded game has its time while it loads, its units come later - after the same step on every PC
internal void walls_tick(void) {
    uint32_t now = game_time();
    if (!g_ws_on || g_ws_fitted_game || !now || !game_map()) return;
    if (!g_ws_t0) g_ws_t0 = now;
    if (now == g_ws_t0) return;
    g_ws_fitted_game = 1;
    unsigned before = g_ws_pieces + g_ws_posts, n = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        int t = unit_type_get_id(it.td);
        if (is_wall(t) && !ut_role(t, UTR_GATE) && unit_get_hp(it.u) > 0) n += (unsigned)ws_fit(it.u);
    }
    if (g_log) fprintf(g_log, "# walls: %u pieces of the game's start fitted to the ground (%u in all so far)\n", n, before + n);
}

internal void walls_new_game(void) { g_ws_fitted_game = 0; g_ws_t0 = 0; g_ws_pieces = g_ws_posts = g_ws_unsloped = g_ws_skipped = 0; }

internal void walls_install(void) {
    g_ws_on = g_set.wall_slopes;
    int of = 0, n = g_ws_on ? patch_wall_piece(hook_wall_piece, &of) + patch_wall_joints(hook_wall_joints, &of) : 0;
    if (g_log) fprintf(g_log, "# walls: wall_slopes=%d (%s)\n", g_ws_on, !g_ws_on ? "-" : n == of ? "the layout's calls hooked" : n ? "SOME of the layout's calls hooked" : "FAILED");
}
