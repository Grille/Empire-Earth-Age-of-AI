// ART OF CONQUEST'S ENERGY FIELDS: the field between a Wall - Space's posts is an effect the game hangs on each piece
// (unit effect, EEE3DEnergyWall) where a piece is finished or laid in a game - never by the scenario editor's placing
// (it skips them there), and a scenario's walls come from its file without one: in a scenario, the editor's test play
// too, they stood bare (the player 2026-11-26: "in my current game the energi fields dont render at all" - 53 of 54
// pieces placed in the editor without one). And the game's fire uses the same slot: a damaged building's fire ends by
// deleting the effect in it, so a space wall repaired lost its field. Each second of game time (at a step's end, the
// same step on every PC; the editor's time stays 0) every piece that should have one by the game's own test and has
// none gets it as the game makes it (energy_wall_wanted / energy_wall_make). A saved game's walls keep theirs.
//
// WHERE A FIELD REACHES (the player's "space walls" scenario, 2026-11-26: "it diagonals thje game dosnt nativly
// supports that get messy"): the game links a piece to the owner's pieces on the tiles left, up-left, up and up-right
// of its own and draws a quad from its post to each - but to its own place plus that tile step, taking only the
// neighbour's height. The posts don't stand on that grid: a wall at a slope is a staircase of pieces at their own
// places (the game's layout: 59.5, 47.25 beside 60.5, 47.75; the mod's fit), so each quad ended beside the next post,
// in the air, and overlapped its neighbours' - a sawtooth along every sloped wall. Here the link stores the
// neighbour's place too (energy_wall_link_store: the call after the game's own store), so a quad runs from post to
// post. And a piece linked to a diagonal neighbour it also reaches round the corner - through the piece beside both,
// where a staircase steps both ways or walls meet - made a triangle of fields, layers crossing: such a diagonal link
// is dropped, the way round the corner stays.
internal uint32_t g_ew_next;   // game time of the next look
internal int g_ew_linked;      // the link's store patched
// the owners' Wall - Space pieces by tile (owner index + 1, 0 none; a side g_ew_n), made with each look: two kept
// and swapped whole, the link (whichever thread updates the effects) reading the one made last
internal uint8_t *g_ew_grids[2];
internal uint8_t *volatile g_ew_grid;
internal int g_ew_n;

internal int ew_has(const uint8_t *g, int x, int y, int owner) {
    return x >= 0 && y >= 0 && x < g_ew_n && y < g_ew_n && g[y * g_ew_n + x] == owner + 1;
}
// after the game's store of neighbour n's height into point k of field f (pz that height): the point onto n's place;
// a diagonal (2, 4; a gate's second end 6, 8) dropped where the pieces meet round the corner
__attribute__((used)) void __stdcall ew_link(ee_energy_wall *f, ee_unit *n, float *pz) {
    int k = (int)(pz - energy_wall_points_at(f)) / 3 + 1;
    if (k < 1 || k > 8) return;
    vec3 p = unit_get_pos(n);
    pz[-2] = p.x;
    pz[-1] = p.y;
    const uint8_t *g = g_ew_grid;
    ee_player *pl = unit_get_owner(n);
    if (!g || (k & 1) || !pl) return;
    const float *o = k <= 4 ? energy_wall_at_at(f) : energy_wall_gate_end_at(f);
    int x = (int)floorf(o[0]), y = (int)floorf(o[1]), own = player_get_index(pl);
    int side = k == 2 || k == 6 ? -1 : 1;   // up-left, else up-right
    if (ew_has(g, x + side, y, own) || ew_has(g, x, y - 1, own)) energy_wall_set_links(f, energy_wall_get_links(f) & ~(1u << k));
}
// at the store (a call over its 7 bytes): the store itself, then ew_link(field esi, neighbour edi, the height's place
// eax) with every register kept - the loop goes on at the next instruction
__attribute__((naked, used)) void ew_link_thunk(void) {
    __asm__ volatile (
        "movl -8(%ebp), %eax\n\t"
        "faddp %st, %st(1)\n\t"
        "fstps (%eax)\n\t"
        "pushal\n\t"
        "pushl %eax\n\t"
        "pushl %edi\n\t"
        "pushl %esi\n\t"
        "call _ew_link@12\n\t"
        "popal\n\t"
        "ret\n\t");
}

internal void energy_walls_step(void) {
    uint32_t now = game_time();
    if (!build_is("aoc") || (now < g_ew_next && g_ew_next - now <= 1000)) return;   // further off: a new game's time
    g_ew_next = now + 1000;
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0;
    uint8_t *g = NULL;
    if (size > 0 && size <= 4096) {
        if (size != g_ew_n) {   // a map of another size: the link reads none meanwhile
            g_ew_grid = NULL;
            for (int b = 0; b < 2; b++) { free(g_ew_grids[b]); g_ew_grids[b] = calloc((size_t)size * size, 1); }
            g_ew_n = g_ew_grids[0] && g_ew_grids[1] ? size : 0;
        }
        if (g_ew_n) { g = g_ew_grids[g_ew_grid == g_ew_grids[0]]; memset(g, 0, (size_t)size * size); }
    }
    int n = 0;
    for (unsigned i = 0; i < game_player_count() && i < 16; i++) {
        ee_player *pl = game_players()[i];
        for (ee_unit *u = pl ? player_get_units(pl) : NULL; u; u = unit_get_next(u)) {
            if (energy_wall_wanted(u)) { energy_wall_make(u); n++; }
            int x = unit_get_x(u), y = unit_get_y(u);
            if (g && x >= 0 && y >= 0 && x < g_ew_n && y < g_ew_n && energy_wall_piece(u)) g[y * g_ew_n + x] = (uint8_t)(i + 1);
        }
    }
    if (g) g_ew_grid = g;
    if (n) log_line("# energy fields: %d space wall pieces given theirs (a scenario's, or a fire's end took it)\n", n);
}
internal void energy_walls_install(void) {
    if (!build_is("aoc")) return;
    g_ew_linked = patch_energy_wall_link((void *)ew_link_thunk);
    if (g_log) fprintf(g_log, "# energy fields: %s\n", g_ew_linked ? "their links reach the neighbours' posts" : "FAILED: the link's store not as expected - the fields end beside the posts");
}
