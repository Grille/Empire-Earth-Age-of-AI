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

// the same owner's object on tile x, y, as wall_joins sees it (wj_at below): its kind, and a piece's axis
#define WJ_NONE 0
#define WJ_PIECE 1
#define WJ_POST 2
#define WJ_HUB 3   // a tower or a gate: a wall ends into it
struct wj_at { int kind, sx, sy; ee_unit *u; };
internal struct wj_at wj_at(const ee_map *map, int x, int y, const ee_player *pl);
internal int g_wj_on;

// a wall's round post on tile x, y, of pl's - or with wall_joins a tower or a gate of pl's: a piece's end reaches into
// it. The layout's own pieces from its list - they come onto the map (the unit grid, a tile's objects) only after the
// layout, at the next step; an older wall's by the tile's objects
internal int ws_post_at(const ee_map *map, int x, int y, const ee_player *pl) {
    for (int i = 0; i < g_ws_laid_n; i++) {
        ee_unit *u = g_ws_laid[i];
        if (u && unit_get_x(u) == x && unit_get_y(u) == y && unit_get_anim(u) == WS_POST) return 1;
    }
    int k = wj_at(map, x, y, pl).kind;
    return k == WJ_POST || (g_wj_on && k == WJ_HUB);
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
    ee_player *pl = unit_get_owner(u);
    float ea = ws_post_at(map, tx - sx, ty - sy, pl) ? WS_INTO_POST : 0, eb = ws_post_at(map, tx + sx, ty + sy, pl) ? WS_INTO_POST : 0;
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

// WALL JOINS (`wall_joins`, with wall_slopes): which tile of a wall is a round post. The layout makes one at each end
// of the stretch it lays, where the stretch turns, and (its sloped pieces) where the ground steps 0.4 or more; its
// last step (wall_joints) one of every piece with more than two of the owner's walls or towers among its 8
// neighbours. So a wall drawn on from the end of another has two posts side by side, one drawn up to another's side
// or across it three or four in a cluster (the diagonal neighbours of the joint count), and a post stands between a
// wall and the tower or gate it runs into (the player, 2026-11-03: "the round connectors appearing in many places they
// aren't supposed to ... walls should properly connect to gates and towers"). Here every tile of the owner's walls is
// looked at by what is around it, every WJ_EVERY of game time at a step's end: a piece points at the two tiles along
// its axis; a post becomes a piece where two pieces point at it from either side (a post in a straight run), or one
// does and the tile on the other side holds a wall, a tower or a gate (a wall's end against another's post or side, a
// tower, a gate: it reaches into it); a piece becomes a post where a piece points at it from aside (a wall ending on
// its side: the joint) or one of its ends has nothing of the owner's and the other a piece (a piece destroyed beside
// it, a gap the layout left: the wall's end; with a post, a tower or a gate at the other it stays a stub out of it);
// a post no piece points at, beside one joint, tower or gate with nothing on its other side, becomes a stub into it.
// From the tiles alone, so a loaded game and every PC come to the same; the pieces fitted again after it, a piece's
// end into a tower or a gate as into a post. And (wall_slopes alone too) a gate laid into a wall (build_gate) turns its
// five pieces into the gate's: its ends get a matrix of their own, its middle three keep the piece's - fitted, the
// gate's middle was drawn shortened (a diagonal one to 0.63) and sheared -, put back to the game's own (upright, its
// tile's corner x, y)
#define WJ_EVERY 500u       // ms of game time
#define WJ_ROUNDS 4         // looks a pass: a post turned a piece makes the side it reaches a post at the next
#define WJ_MAX 16384        // wall pieces a pass looks at

internal uint32_t g_wj_last;
internal unsigned g_wj_to_piece, g_wj_to_post, g_wj_gates;
internal ee_unit *g_wj_units[WJ_MAX];
internal struct wj_at g_wj_want[WJ_MAX];

internal int wj_alive(ee_unit *o) {   // a site laid out has no hit points yet; a finished piece without is collapsing
    ee_unit *bs = unit_get_physics(o);
    return unit_get_hp(o) > 0 || (bs && !unit_get_finished(bs));
}
internal int wj_piece_anim(int anim) { return anim == WS_STRAIGHT || anim == WS_DIAG || anim == WS_SLOPE || anim == WS_SLOPE_DIAG; }

// a piece's axis, a step between tiles: its facing (the local y axis), straight along x or y, diagonal both
internal void wj_axis(const ee_unit *u, int *sx, int *sy) {
    float fx = unit_get_matrix(u, 1), fy = unit_get_matrix(u, 5), f = sqrtf(fx * fx + fy * fy);
    if (f > 0.01f) { fx /= f; fy /= f; }
    *sx = fx > 0.3f ? 1 : fx < -0.3f ? -1 : 0;
    *sy = fy > 0.3f ? 1 : fy < -0.3f ? -1 : 0;
}

// the tile's objects (a building on every tile it covers), pl's: its wall's piece or post, else its tower or gate
internal struct wj_at wj_at(const ee_map *map, int x, int y, const ee_player *pl) {
    struct wj_at a = { WJ_NONE, 0, 0, NULL };
    int n = map ? (int)map_get_size(map) : 0;
    ee_tile *tile = x >= 0 && y >= 0 && x < n && y < n ? map_get_tiles(map)[y * n + x] : NULL;
    for (void *node = tile ? tile_get_objects(tile) : NULL; node; node = tilenode_get_next(node)) {
        ee_unit *o = tilenode_get_object(node);
        ee_unit_type *td = o ? unit_get_type_data(o) : NULL;
        if (!td || unit_get_owner(o) != pl || !wj_alive(o)) continue;
        int t = unit_type_get_id(td), anim = unit_get_anim(o);
        if (ut_role(t, UTR_GATE) || ut_role(t, UTR_TOWER)) { a.kind = WJ_HUB; a.u = o; continue; }
        if (!is_wall(t)) continue;
        if (anim == WS_POST) { a.kind = WJ_POST; a.u = o; return a; }
        if (wj_piece_anim(anim)) { a.kind = WJ_PIECE; a.u = o; wj_axis(o, &a.sx, &a.sy); return a; }
    }
    return a;
}

internal const int k_wj_dir[8][2] = { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 } };

// whether a piece of pl's among the 8 around x, y points at it
internal int wj_pointed(const ee_map *map, int x, int y, const ee_player *pl) {
    for (int k = 0; k < 8; k++) {
        int dx = k_wj_dir[k][0], dy = k_wj_dir[k][1];
        struct wj_at nb = wj_at(map, x + dx, y + dy, pl);
        if (nb.kind == WJ_PIECE && ((nb.sx == dx && nb.sy == dy) || (nb.sx == -dx && nb.sy == -dy))) return 1;
    }
    return 0;
}

// what wall tile x, y (now self) should be by its 8 neighbours
internal struct wj_at wj_want(const ee_map *map, int x, int y, struct wj_at self, const ee_player *pl) {
    const int (*dir)[2] = k_wj_dir;
    int pt[8], np = 0;   // the neighbours whose axis points at x, y
    for (int k = 0; k < 8; k++) {
        struct wj_at nb = wj_at(map, x + dir[k][0], y + dir[k][1], pl);
        if (nb.kind == WJ_PIECE && ((nb.sx == dir[k][0] && nb.sy == dir[k][1]) || (nb.sx == -dir[k][0] && nb.sy == -dir[k][1])))
            pt[np++] = k;
    }
    struct wj_at w = self;
    if (self.kind == WJ_PIECE) {
        for (int i = 0; i < np; i++) {
            int dx = dir[pt[i]][0], dy = dir[pt[i]][1];
            if (!((dx == self.sx && dy == self.sy) || (dx == -self.sx && dy == -self.sy))) { w.kind = WJ_POST; return w; }
        }
        // an end with nothing: the wall's end, unless the other end goes into a post, a tower or a gate - a stub out
        // of it (the game leaves the steepest tiles out of a wall: a gap beside a joint), no second post beside it
        int a = wj_at(map, x + self.sx, y + self.sy, pl).kind, b = wj_at(map, x - self.sx, y - self.sy, pl).kind;
        if ((a == WJ_NONE && b != WJ_POST && b != WJ_HUB) || (b == WJ_NONE && a != WJ_POST && a != WJ_HUB))
            w.kind = WJ_POST;
        return w;
    }
    // a post: the new piece along the pointing one's axis, turned as it is
    int k = pt[0];
    if (np == 2 ? pt[1] - pt[0] == 4 : np == 1 && wj_at(map, x - dir[k][0], y - dir[k][1], pl).kind != WJ_NONE) {
        struct wj_at nb = wj_at(map, x + dir[k][0], y + dir[k][1], pl);
        w.kind = WJ_PIECE; w.sx = nb.sx; w.sy = nb.sy;
    }
    // none points at it: beside one joint (a post pieces point at), tower or gate with nothing on its other side, a stub
    // into it - a wall's end made a post at a gap the layout left, before a wall came to meet it there
    if (!np) {
        int cand = -1, nc = 0;
        for (int j = 0; j < 8; j++) {
            struct wj_at nb = wj_at(map, x + dir[j][0], y + dir[j][1], pl);
            if ((nb.kind == WJ_HUB || (nb.kind == WJ_POST && wj_pointed(map, x + dir[j][0], y + dir[j][1], pl))) &&
                wj_at(map, x - dir[j][0], y - dir[j][1], pl).kind == WJ_NONE) { cand = j; nc++; }
        }
        if (nc == 1) { w.kind = WJ_PIECE; w.sx = dir[cand][0]; w.sy = dir[cand][1]; }
    }
    return w;
}

// u turned into a piece along sx, sy or a post: its animation, upright on its tile as the layout leaves one (the fit
// after puts it onto the ground)
internal void wj_turn(const ee_map *map, ee_unit *u, struct wj_at w) {
    int tx = unit_get_x(u), ty = unit_get_y(u);
    float l = w.sx && w.sy ? sqrtf(0.5f) : 1.0f, fx = w.kind == WJ_PIECE ? (float)w.sx * l : 0, fy = w.kind == WJ_PIECE ? (float)w.sy * l : 1;
    float z = ground_corner(map, tx, ty);
    unit_start_anim(u, w.kind == WJ_POST ? WS_POST : w.sx && w.sy ? WS_DIAG : WS_STRAIGHT, -1, 0);
    const float m[12] = { fy, fx, 0, (float)tx + 0.5f,  -fx, fy, 0, (float)ty + 0.5f,  0, 0, 1, z == z ? z : unit_get_matrix(u, 11) };
    for (int i = 0; i < 12; i++) unit_set_matrix(u, i, m[i]);
    if (w.kind == WJ_POST) g_wj_to_post++; else g_wj_to_piece++;
}

// a gate's unit with a fitted piece's matrix (stretched, sheared, taller) put back to the game's: upright, its tile's
// corner x, y
internal void wj_gate(const ee_map *map, ee_unit *u) {
    float fx = unit_get_matrix(u, 1), fy = unit_get_matrix(u, 5), f = sqrtf(fx * fx + fy * fy);
    if (f < 0.01f || (fabsf(f - 1) < 0.001f && unit_get_matrix(u, 9) == 0 && fabsf(unit_get_matrix(u, 10) - 1) < 0.001f)) return;
    int tx = unit_get_x(u), ty = unit_get_y(u);
    float z = ground_corner(map, tx, ty);
    fx /= f; fy /= f;
    const float m[12] = { fy, fx, 0, (float)tx + 0.5f,  -fx, fy, 0, (float)ty + 0.5f,  0, 0, 1, z == z ? z : unit_get_matrix(u, 11) };
    for (int i = 0; i < 12; i++) unit_set_matrix(u, i, m[i]);
    g_wj_gates++;
}

internal void wj_pass(void) {
    ee_map *map = game_map();
    if (!map) return;
    int n = 0;
    unsigned piece0 = g_wj_to_piece, post0 = g_wj_to_post, gates0 = g_wj_gates;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        int t = unit_type_get_id(it.td);
        if (!is_wall(t) || !wj_alive(it.u)) continue;
        if (ut_role(t, UTR_GATE)) wj_gate(map, it.u);
        else if (g_wj_on && n < WJ_MAX) g_wj_units[n++] = it.u;
    }
    // each look from the tiles as they are, then the turns
    for (int r = 0; r < WJ_ROUNDS; r++) {
        int turned = 0;
        for (int i = 0; i < n; i++) {
            ee_unit *u = g_wj_units[i];
            ee_player *pl = unit_get_owner(u);
            struct wj_at self = wj_at(map, unit_get_x(u), unit_get_y(u), pl);
            struct wj_at w = self.u == u ? wj_want(map, unit_get_x(u), unit_get_y(u), self, pl) : self;
            w.u = self.u == u && (w.kind != self.kind || w.sx != self.sx || w.sy != self.sy) ? u : NULL;
            g_wj_want[i] = w;
        }
        for (int i = 0; i < n; i++)
            if (g_wj_want[i].u) { wj_turn(map, g_wj_units[i], g_wj_want[i]); turned++; }
        if (!turned) break;
    }
    for (int i = 0; i < n; i++) ws_fit(g_wj_units[i]);
    if (g_log && (g_wj_to_piece != piece0 || g_wj_to_post != post0 || g_wj_gates != gates0))
        fprintf(g_log, "# walls: joins at %.2f min: %u posts made pieces, %u pieces posts, %u gate units upright (%d wall pieces)\n",
                (double)game_time() / 60000.0, g_wj_to_piece - piece0, g_wj_to_post - post0, g_wj_gates - gates0, n);
}

// at a step's end (the same step on every PC): the walls a game starts with (a loaded game, a scenario's) at its first,
// the gates (wall_slopes) and the joins (wall_joins) every WJ_EVERY
internal void __thiscall hook_step_end(ee_physics *ph) {
    physics_step_end(ph);
    uint32_t now = game_time();
    if (!g_ws_on || !game_map()) return;
    if (!g_ws_fitted_game) {
        g_ws_fitted_game = 1;
        unsigned before = g_ws_pieces + g_ws_posts, n = 0;
        for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
            int t = unit_type_get_id(it.td);
            if (is_wall(t) && !ut_role(t, UTR_GATE) && unit_get_hp(it.u) > 0) n += (unsigned)ws_fit(it.u);
        }
        if (g_log) fprintf(g_log, "# walls: %u pieces of the game's start fitted to the ground (%u in all so far, %u without their model's length)\n",
                           n, before + n, g_ws_no_model);
    }
    if (!g_wj_last || now / WJ_EVERY != g_wj_last / WJ_EVERY) {
        g_wj_last = now ? now : 1;
        wj_pass();
    }
}

internal void walls_new_game(void) {
    g_ws_fitted_game = 0; g_ws_gfx_n = 0; g_ws_laid_n = 0; g_wj_last = 0;
    g_ws_pieces = g_ws_posts = g_ws_unsloped = g_ws_skipped = g_ws_no_model = 0;
    g_wj_to_piece = g_wj_to_post = g_wj_gates = 0;
}

internal void walls_install(void) {
    g_ws_on = g_set.wall_slopes;
    g_wj_on = g_ws_on && g_set.wall_joins;
    int of = 0, n = g_ws_on ? patch_wall_piece(hook_wall_piece, &of) + patch_wall_joints(hook_wall_joints, &of) : 0;
    int step = g_ws_on ? patch_step_end(hook_step_end) : 0;
    if (g_log) fprintf(g_log, "# walls: wall_slopes=%d wall_joins=%d (%s%s)\n", g_ws_on, g_wj_on,
                       !g_ws_on ? "-" : n == of ? "the layout's calls hooked" : n ? "SOME of the layout's calls hooked" : "FAILED",
                       !g_ws_on ? "" : step ? ", the step's end" : ", the step's end FAILED");
}
