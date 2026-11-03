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
// (its top stays). And it spans its tile exactly, into a post beside it: the meshes are longer than the pieces'
// spacing (straight 1.00 .. 1.06 on 1, diagonal 1.58 .. 1.71 on 1.41), so neighbours overlapped - the faces in one
// plane flickered, and pieces leaning apart stuck out of each other at every bend (the player, 2026-11-03: "the
// overlap becomes even more noticeable now, and it flickers ... instead make the overlap targeted like extending into
// the round connector pieces"). A sloped piece becomes the plain one first. A post stands on the lowest corner of its
// tile, its top where it was. The layout's last step (wall_joints) makes a piece where walls meet a post anew, flat:
// the wall's pieces are fitted again after it, with all their neighbours there. Pieces laid before (a loaded game)
// are fitted at the game's first step. A piece's collapse when
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
#define WS_INTO_POST 0.3f   // a piece's end into a post beside it (the posts' sides 0.49 .. 0.66 from their middle)

internal int g_ws_on, g_ws_fitted_game;
internal uint32_t g_ws_t0;   // the game time first seen: a loaded game's while it loads, before its units are there
internal unsigned g_ws_pieces, g_ws_posts, g_ws_unsloped, g_ws_skipped, g_ws_no_model;
#define WS_GFX_MAX 32
internal ee_dmg_model *g_ws_gfx[WS_GFX_MAX];   // the graphics the fit took a user of this game (one each, kept)
internal int g_ws_gfx_n;

// a user of the wall's graphics, once a game: their models loaded and kept while the game runs
internal void ws_acquire(ee_dmg_model *gfx) {
    for (int i = 0; i < g_ws_gfx_n; i++) if (g_ws_gfx[i] == gfx) return;
    if (g_ws_gfx_n >= WS_GFX_MAX) return;
    g_ws_gfx[g_ws_gfx_n++] = gfx;
    int r = dmg_model_acquire(gfx);
    if (g_log) fprintf(g_log, "# walls: the graphics %p loaded for the fit (%d)\n", (void *)gfx, r);
}

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

#define WS_LAID_MAX 512
internal ee_unit *g_ws_laid[WS_LAID_MAX];   // the pieces of the layout under way, fitted again at its last step
internal int g_ws_laid_n;

// a wall's piece (not a gate's) on tile x, y: its round post. The layout's own pieces from its list - they come onto
// the map (the unit grid, a tile's objects) only after the layout, at the next step; an older wall's by the grid
internal int ws_post_at(int x, int y) {
    for (int i = 0; i < g_ws_laid_n; i++) {
        ee_unit *u = g_ws_laid[i];
        if (u && unit_get_x(u) == x && unit_get_y(u) == y && unit_get_anim(u) == WS_POST) return 1;
    }
    for (struct grid_it it = grid_it_box(x, y, 0, 1); grid_it_next(&it); ) {
        int t = unit_type_get_id(it.td);
        if (unit_get_x(it.u) == x && unit_get_y(it.u) == y && is_wall(t) && !ut_role(t, UTR_GATE) && unit_get_anim(it.u) == WS_POST)
            return 1;
    }
    return 0;
}

// one piece onto the ground; 0 when it isn't a piece of the layout's or its ground is off the map
internal int ws_fit(ee_unit *u) {
    ee_map *map = game_map();
    if (!map || !u) return 0;
    int anim = unit_get_anim(u);
    if (anim == WS_SLOPE) { unit_start_anim(u, WS_STRAIGHT, -1, 0); g_ws_unsloped++; }
    else if (anim == WS_SLOPE_DIAG) { unit_start_anim(u, WS_DIAG, -1, 0); g_ws_unsloped++; }
    else if (anim != WS_STRAIGHT && anim != WS_DIAG && anim != WS_POST) return 0;
    // the middle of its tile, not the matrix's: a piece's matrix is moved below, a fit again gives the same
    int tx = unit_get_x(u), ty = unit_get_y(u);
    float px = (float)tx + 0.5f, py = (float)ty + 0.5f;
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
    // its length: the local y axis, its turn (x, y; the local x axis keeps its scale, 1 for walls); its ends half a
    // tile out along it - an edge's middle (straight) or a corner (diagonal), where the ground's corners are exact
    float fx = unit_get_matrix(u, 1), fy = unit_get_matrix(u, 5), f = sqrtf(fx * fx + fy * fy);
    float len = sqrtf(unit_get_matrix(u, 0) * unit_get_matrix(u, 0) + unit_get_matrix(u, 4) * unit_get_matrix(u, 4));
    if (f < 0.01f || len < 0.01f) { g_ws_skipped++; return 0; }
    fx /= f; fy /= f;
    float d = 0.5f * (fabsf(fx) + fabsf(fy));
    float ax = px - d * fx, ay = py - d * fy, bx = px + d * fx, by = py + d * fy;
    float ha = ground_at(map, ax, ay), hb = ground_at(map, bx, by);
    if (ha != ha || hb != hb) { g_ws_skipped++; return 0; }
    float sink = WS_SINK + ws_fall(map, ax, ay, bx, by, ha, hb, fy, -fx);
    if (sink > WS_SINK_MAX) sink = WS_SINK_MAX;
    // its mesh (the model's first frame, its y from ylo to yhi - each era's its own length, 1.00 .. 1.71, and off its
    // middle by up to 0.025) stretched onto its tile's span exactly: the meshes were made longer than the spacing, and
    // neighbours overlapped - the faces flickered and, leaning apart, stuck out at every bend. Into a post at an end
    // (it hides it; a diagonal piece's end at the corner would stop short of one). The animation's model from its
    // graphics, not the one it is drawn with: a piece laid out is a building site and draws the site's until built
    // The game loads a graphics' models when a first user needs them, and a doppelganger (the fog of war's last-seen
    // copy) is one - on each PC by what its player saw: a wall's are often not there yet when its first pieces are
    // laid out (none in the first game minutes). The matrix is the game's state (the unit's position, its points),
    // so the fit takes a user of its own, the same step on every PC, and has the same model everywhere
    ee_unit_stats *st = unit_get_stats(u);
    ee_dmg_model *gfx = st ? unit_stats_get_gfx(st) : NULL;
    int ma = anim == WS_SLOPE ? WS_STRAIGHT : anim == WS_SLOPE_DIAG ? WS_DIAG : anim;
    ee_model *mdl = gfx ? ((ee_model **)dmg_model_anim_model_at(gfx))[ma] : NULL;
    if (gfx && !mdl) {
        ws_acquire(gfx);
        mdl = ((ee_model **)dmg_model_anim_model_at(gfx))[ma];
    }
    ee_box *box = mdl && model_get_frames(mdl) ? model_get_boxes(mdl) : NULL;
    float ylo = box ? box_get_min(box).y : -d, yhi = box ? box_get_max(box).y : d;
    if (yhi - ylo < 0.1f) { ylo = -d; yhi = d; g_ws_no_model++; }
    int sx = fx > 0.3f ? 1 : fx < -0.3f ? -1 : 0, sy = fy > 0.3f ? 1 : fy < -0.3f ? -1 : 0;
    float ea = ws_post_at(tx - sx, ty - sy) ? WS_INTO_POST : 0, eb = ws_post_at(tx + sx, ty + sy) ? WS_INTO_POST : 0;
    float k = (2 * d + ea + eb) / (yhi - ylo), c = -d - ea - k * ylo, slope = (hb - ha) / (2 * d);   // its y -> k y + c along f
    const float m[12] = {
        fy * len,  fx * k,    0, px + fx * c,
        -fx * len, fy * k,    0, py + fy * c,
        0,         slope * k, len * (1 + sink / WS_HEIGHT), (ha + hb) * 0.5f - sink + slope * c,
    };
    for (int i = 0; i < 12; i++) unit_set_matrix(u, i, m[i]);
    g_ws_pieces++;
    return 1;
}

internal void __cdecl hook_wall_piece(ee_unit *u, int code, int raised) {
    wall_piece(u, code, raised);
    if (!g_ws_on) return;
    ws_fit(u);
    if (u && g_ws_laid_n < WS_LAID_MAX) g_ws_laid[g_ws_laid_n++] = u;
}

// where walls meet or cross, the layout's last step (unit_functions.wall_joints 0x62E73B) makes a piece with more than
// two neighbours a post, its matrix anew and flat: the wall's pieces fitted again (a fit twice gives the same), and
// the walls' pieces around them - a piece of an older wall beside a new post reaches into it now
internal void __cdecl hook_wall_joints(ee_unit_vec *pieces, void *tiles, ee_player *pl, ee_map *map) {
    wall_joints(pieces, tiles, pl, map);
    if (!g_ws_on) { g_ws_laid_n = 0; return; }
    // the pieces laid out, all of them there now (when each was laid its neighbours weren't yet), and the joints' own
    // (an older wall's, made a post); then the older walls' pieces around both (by the unit grid: the new pieces come
    // onto it only at the next step - ws_post_at finds them in the list)
    ee_unit **pb = pieces ? unit_vec_get_begin(pieces) : NULL, **pe = pieces ? unit_vec_get_end(pieces) : NULL;
    int nv = pb && pe > pb ? (int)(pe - pb) : 0;
    for (int pass = 0; pass < 2; pass++)
        for (int i = 0; i < g_ws_laid_n + nv; i++) {
            ee_unit *u = i < g_ws_laid_n ? g_ws_laid[i] : pb[i - g_ws_laid_n];
            if (!u || unit_get_hp(u) <= 0) continue;
            if (!pass) { ws_fit(u); continue; }
            int x = unit_get_x(u), y = unit_get_y(u);
            for (struct grid_it it = grid_it_box(x, y, 1, 1); grid_it_next(&it); ) {
                int t = unit_type_get_id(it.td);
                if (it.u != u && abs(unit_get_x(it.u) - x) <= 1 && abs(unit_get_y(it.u) - y) <= 1 && is_wall(t) &&
                    !ut_role(t, UTR_GATE) && unit_get_hp(it.u) > 0)
                    ws_fit(it.u);
            }
        }
    g_ws_laid_n = 0;
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
    if (g_log) fprintf(g_log, "# walls: %u pieces of the game's start fitted to the ground (%u in all so far, %u without their model's length)\n",
                       n, before + n, g_ws_no_model);
}

internal void walls_new_game(void) {
    g_ws_fitted_game = 0; g_ws_t0 = 0; g_ws_gfx_n = 0; g_ws_laid_n = 0;
    g_ws_pieces = g_ws_posts = g_ws_unsloped = g_ws_skipped = g_ws_no_model = 0;
}

internal void walls_install(void) {
    g_ws_on = g_set.wall_slopes;
    int of = 0, n = g_ws_on ? patch_wall_piece(hook_wall_piece, &of) + patch_wall_joints(hook_wall_joints, &of) : 0;
    if (g_log) fprintf(g_log, "# walls: wall_slopes=%d (%s)\n", g_ws_on, !g_ws_on ? "-" : n == of ? "the layout's calls hooked" : n ? "SOME of the layout's calls hooked" : "FAILED");
}
