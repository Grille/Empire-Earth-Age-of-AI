internal void rd_world_rebuild(int on);   // render_3d.c
// THE EDITOR'S UNDO (the player, 2026-11-10: "we need a propper undo system, only saving the units etc, even if itzs lossy").
// The game's Undo and Redo take the last action back by its vf2, empty in every editor action. Here an action's change is
// worked out: before it a list of the units on the map (ids, types, owners, poses, hit points) and, for a paint tool, the
// ground's data; after it the same again and what differs is kept. Nothing is written or loaded, the camera doesn't move.
// Undo takes away what the action made (the Delete action's own vf1), makes again what it took (the type's own make, as
// Place does) and puts back what it moved, redo the other way round. A unit made again has another id: the steps' lists
// are told. Lossy on purpose: a unit comes back without its cargo, orders or what else it held.
// A stroke of the paint tools is one step: the game makes an action per mouse move, the step stays open between them and
// is closed by the next thing done. A move is one step from the units' pick-up (off the map) to their putting down.
// Painting has to stay smooth (the player, 2026-11-10: "painting terrain now still has a noticable delay"), so a stroke
// reads no more than it touched: one copy of the whole ground runs along (made at the first stroke, a map's only full
// read), the tools tell the minimap what they changed and only that box is compared and brought up to date; the units are
// listed for a stroke only if its terrain scatters objects.
#define EU_STEPS 200
#define EU_STROKE_MS 500     // a paint action this soon after one of its kind goes on the same stroke
struct eu_unit { int id, type, player, x, y, hp, anim; float m[16]; };
struct eu_move { struct eu_unit was, now; };
struct eu_ground { float h; uint8_t kind, tex, color; };
struct eu_cell { uint16_t x, y; float h[2]; uint8_t kind[2], tex[2], color[2]; };   // [0] before the action, [1] after
struct eu_step {
    struct eu_unit *made, *gone;
    struct eu_move *moved;
    struct eu_cell *cells;
    int nmade, ngone, nmoved, ncells;
    int x0, x1, y0, y1;        // the cells' box
};
internal struct eu_step g_eu_steps[EU_STEPS];
internal int g_eu_n, g_eu_cur;      // the steps kept; those below cur are done (cur < n: the rest undone, for a redo)
internal struct eu_unit *g_eu_was;  // the open step: the units before it, by id
internal int g_eu_was_n, g_eu_open;
internal struct eu_ground *g_eu_ground;   // the ground as of the last step, (size + 1)^2: what a stroke is compared with
internal int g_eu_ground_size;
internal int g_eu_stroke_open, g_eu_dirty, g_eu_dx0, g_eu_dx1, g_eu_dy0, g_eu_dy1;   // a paint tool's step open, the box it changed
internal int g_eu_depth;            // an action inside another (a paint's own): the outer's step holds
internal int g_eu_busy;             // the undo itself is at work
internal void *g_eu_stroke_vt;
internal DWORD g_eu_stroke_at;
internal void *g_eu_ev_vt[2][UI_EVENT_SLOTS];   // the hotkeys' events' vtables
internal void eu_settle(void);
// units made again and the id they had: a new unit has its id only after the world's step, so the steps' lists are told later
struct eu_late { int was; ee_unit *u; };
internal struct eu_late *g_eu_late;
internal int g_eu_late_n;

internal int eu_by_id(const void *a, const void *b) {
    int x = ((const struct eu_unit *)a)->id, y = ((const struct eu_unit *)b)->id;
    return x < y ? -1 : x > y;
}
// the units on the map (the grid: not those in a transport), by id
internal struct eu_unit *eu_units(int *n) {
    int cap = 4096, k = 0;
    struct eu_unit *a = malloc((size_t)cap * sizeof *a);
    for (struct grid_it it = grid_it_all(1); a && grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *pl = unit_get_owner(u);
        if (!it.td || !pl) continue;
        if (k == cap) {
            struct eu_unit *b = realloc(a, (size_t)(cap *= 2) * sizeof *a);
            if (!b) { free(a); a = NULL; break; }
            a = b;
        }
        struct eu_unit *r = &a[k++];
        r->id = unit_get_serial(u);   // what the world's unit_by_id and the editor's own actions go by
        r->type = unit_type_get_id(it.td);
        r->player = player_get_index(pl);
        r->x = unit_get_x(u);
        r->y = unit_get_y(u);
        r->hp = unit_get_hp(u);
        r->anim = unit_get_anim(u);
        memcpy(r->m, unit_matrix_at(u), 12 * sizeof(float));
        r->m[12] = r->m[13] = r->m[14] = 0.0f;
        r->m[15] = 1.0f;
    }
    if (a) qsort(a, (size_t)k, sizeof *a, eu_by_id);
    *n = a ? k : 0;
    return a;
}
internal double eu_ms(LONGLONG since) {
    LARGE_INTEGER now, f;
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&f);
    return (double)(now.QuadPart - since) * 1000.0 / (double)f.QuadPart;
}
internal LONGLONG eu_now(void) { LARGE_INTEGER t; QueryPerformanceCounter(&t); return t.QuadPart; }
// the ground's data the undo keeps of the corners and tiles of a box, row by row
internal void eu_ground_read(const ee_map *map, unsigned size, unsigned x0, unsigned x1, unsigned y0, unsigned y1, struct eu_ground *g) {
    ee_tile *const *tiles = map_get_tiles(map);
    for (unsigned y = y0; y <= y1; y++)
        for (unsigned x = x0; x <= x1; x++, g++) {
            const ee_tile *t = x < size && y < size ? tiles[y * size + x] : NULL;
            const ee_corner *c = map_corner(map, (int)x, (int)y);
            int tex = t ? map_tile_texture(map, (int)x, (int)y) : -1;
            g->h = c ? corner_get_height(c) : 0.0f;
            g->kind = t ? (uint8_t)tile_get_kind(t) : 0xFF;
            g->tex = tex < 0 ? 0xFF : (uint8_t)tex;
            g->color = c ? corner_get_color(c) : 0;
        }
}
internal void eu_step_free(struct eu_step *s) {
    free(s->made), free(s->gone), free(s->moved), free(s->cells);
    memset(s, 0, sizeof *s);
}
internal void eu_clear(void) {
    for (int i = 0; i < g_eu_n; i++) eu_step_free(&g_eu_steps[i]);
    g_eu_n = g_eu_cur = 0;
    free(g_eu_was), free(g_eu_ground);
    g_eu_was = NULL, g_eu_ground = NULL;
    g_eu_ground_size = 0;
    g_eu_stroke_open = 0;
    g_eu_open = g_eu_depth = 0;
    g_eu_stroke_vt = NULL;
    free(g_eu_late);
    g_eu_late = NULL;
    g_eu_late_n = 0;
}
// the units' change into a step
internal void eu_diff_units(struct eu_step *s, struct eu_unit *now, int n) {
    struct eu_unit *was = g_eu_was;
    int nw = g_eu_was_n, a = 0, b = 0, made = 0, gone = 0, moved = 0;
    for (a = b = 0; a < nw || b < n; ) {
        if (b >= n || (a < nw && was[a].id < now[b].id)) { gone++; a++; }
        else if (a >= nw || now[b].id < was[a].id) { made++; b++; }
        else { moved += memcmp(&was[a], &now[b], sizeof *was) != 0; a++; b++; }
    }
    if (made) s->made = malloc((size_t)made * sizeof *s->made);
    if (gone) s->gone = malloc((size_t)gone * sizeof *s->gone);
    if (moved) s->moved = malloc((size_t)moved * sizeof *s->moved);
    if ((made && !s->made) || (gone && !s->gone) || (moved && !s->moved)) return;
    for (a = b = 0; a < nw || b < n; ) {
        if (b >= n || (a < nw && was[a].id < now[b].id)) s->gone[s->ngone++] = was[a++];
        else if (a >= nw || now[b].id < was[a].id) s->made[s->nmade++] = now[b++];
        else {
            if (memcmp(&was[a], &now[b], sizeof *was)) { s->moved[s->nmoved].was = was[a]; s->moved[s->nmoved++].now = now[b]; }
            a++; b++;
        }
    }
}
#define EU_MARGIN 4           // round what a tool says it changed: the cliff pass and the corners beside its tiles
// the running copy ready for this map (the one full read)
internal int eu_ground_ready(const ee_map *map) {
    unsigned size = map_get_size(map);
    if (g_eu_ground && (unsigned)g_eu_ground_size == size) return 1;
    free(g_eu_ground);
    g_eu_ground = malloc((size_t)(size + 1) * (size + 1) * sizeof *g_eu_ground);
    g_eu_ground_size = g_eu_ground ? (int)size : 0;
    if (g_eu_ground) eu_ground_read(map, size, 0, size, 0, size, g_eu_ground);
    return g_eu_ground != NULL;
}
internal void eu_clamp_box(int *x0, int *x1, int *y0, int *y1, int size) {
    *x0 = *x0 - EU_MARGIN < 0 ? 0 : *x0 - EU_MARGIN;
    *y0 = *y0 - EU_MARGIN < 0 ? 0 : *y0 - EU_MARGIN;
    *x1 = *x1 + EU_MARGIN > size ? size : *x1 + EU_MARGIN;
    *y1 = *y1 + EU_MARGIN > size ? size : *y1 + EU_MARGIN;
}
// the stroke's box against the running copy: what differs into the step, the copy brought up to date
internal void eu_diff_ground(struct eu_step *s, const ee_map *map) {
    if (!g_eu_dirty || !g_eu_ground || !map || (int)map_get_size(map) != g_eu_ground_size) return;
    int size = g_eu_ground_size, bx0 = g_eu_dx0, bx1 = g_eu_dx1, by0 = g_eu_dy0, by1 = g_eu_dy1;
    eu_clamp_box(&bx0, &bx1, &by0, &by1, size);
    if (bx0 > bx1 || by0 > by1) return;
    int w = bx1 - bx0 + 1, h = by1 - by0 + 1, n = 0;
    struct eu_ground *now = malloc((size_t)w * (size_t)h * sizeof *now);
    if (!now) return;
    eu_ground_read(map, (unsigned)size, (unsigned)bx0, (unsigned)bx1, (unsigned)by0, (unsigned)by1, now);
    for (int y = 0; y < h; y++) {
        const struct eu_ground *a = &g_eu_ground[(size_t)(y + by0) * (size_t)(size + 1) + (size_t)bx0], *b = &now[(size_t)y * (size_t)w];
        for (int x = 0; x < w; x++) n += memcmp(&a[x], &b[x], sizeof *a) != 0;
    }
    if (n && (s->cells = malloc((size_t)n * sizeof *s->cells)) != NULL) {
        s->x0 = s->y0 = size, s->x1 = s->y1 = 0;
        for (int y = 0; y < h; y++) {
            const struct eu_ground *a = &g_eu_ground[(size_t)(y + by0) * (size_t)(size + 1) + (size_t)bx0], *b = &now[(size_t)y * (size_t)w];
            for (int x = 0; x < w; x++) {
                if (!memcmp(&a[x], &b[x], sizeof *a)) continue;
                struct eu_cell c = { (uint16_t)(x + bx0), (uint16_t)(y + by0), { a[x].h, b[x].h }, { a[x].kind, b[x].kind }, { a[x].tex, b[x].tex }, { a[x].color, b[x].color } };
                s->cells[s->ncells++] = c;
                if (x + bx0 < s->x0) s->x0 = x + bx0;
                if (x + bx0 > s->x1) s->x1 = x + bx0;
                if (y + by0 < s->y0) s->y0 = y + by0;
                if (y + by0 > s->y1) s->y1 = y + by0;
            }
        }
    }
    for (int y = 0; y < h; y++)
        memcpy(&g_eu_ground[(size_t)(y + by0) * (size_t)(size + 1) + (size_t)bx0], &now[(size_t)y * (size_t)w], (size_t)w * sizeof *now);
    free(now);
}
// the running copy after an undo or redo of a step's box
internal void eu_ground_sync(const struct eu_step *s) {
    ee_map *map = game_map();
    if (!s->ncells || !g_eu_ground || !map || (int)map_get_size(map) != g_eu_ground_size) return;
    int size = g_eu_ground_size, bx0 = s->x0, bx1 = s->x1, by0 = s->y0, by1 = s->y1;
    eu_clamp_box(&bx0, &bx1, &by0, &by1, size);
    for (int y = by0; y <= by1; y++)
        eu_ground_read(map, (unsigned)size, (unsigned)bx0, (unsigned)bx1, (unsigned)y, (unsigned)y, &g_eu_ground[(size_t)y * (size_t)(size + 1) + (size_t)bx0]);
}
// the open step finished: what it changed kept, if anything
internal void eu_close(void) {
    if (!g_eu_open) return;
    LONGLONG t0 = eu_now();
    g_eu_open = 0;
    g_eu_stroke_vt = NULL;
    struct eu_step s;
    memset(&s, 0, sizeof s);
    if (g_eu_was) {
        int n = 0;
        struct eu_unit *now = eu_units(&n);
        if (now) eu_diff_units(&s, now, n);
        free(now);
    }
    if (g_eu_stroke_open) eu_diff_ground(&s, game_map());
    g_eu_stroke_open = 0;
    free(g_eu_was);
    g_eu_was = NULL;
    if (!s.nmade && !s.ngone && !s.nmoved && !s.ncells) { eu_step_free(&s); return; }
    while (g_eu_n > g_eu_cur) eu_step_free(&g_eu_steps[--g_eu_n]);   // what was undone can't be redone after this
    if (g_eu_n == EU_STEPS) {
        eu_step_free(&g_eu_steps[0]);
        memmove(g_eu_steps, g_eu_steps + 1, (EU_STEPS - 1) * sizeof *g_eu_steps);
        memset(&g_eu_steps[EU_STEPS - 1], 0, sizeof *g_eu_steps);
        g_eu_n--;
    }
    g_eu_steps[g_eu_n++] = s;
    g_eu_cur = g_eu_n;
    if (g_log) log_locked("# editor undo: step %d: %d made, %d gone, %d changed, %d of the ground, %.1f ms\n", g_eu_n, s.nmade, s.ngone, s.nmoved, s.ncells, eu_ms(t0));
}
// before an action that changes the scenario. stroke: 1 a paint tool's, its ground watched; 2 a move, open from the pick-up;
// units: the units' list wanted
internal void eu_before(void *vt, int stroke, int units) {
    if (g_eu_depth++ || g_eu_busy) return;
    eu_settle();
    DWORD now = GetTickCount();
    // (the button held: a pause in the stroke doesn't end it)
    int same = stroke && vt == g_eu_stroke_vt && g_eu_open && (stroke == 2 || now - g_eu_stroke_at < EU_STROKE_MS || GetAsyncKeyState(VK_LBUTTON) < 0);
    g_eu_stroke_at = now;
    if (same) return;
    eu_close();
    LONGLONG t0 = eu_now();
    g_eu_was = NULL, g_eu_was_n = 0;
    if (units) g_eu_was = eu_units(&g_eu_was_n);
    ee_map *map = game_map();
    if (stroke == 1 && map && eu_ground_ready(map)) { g_eu_stroke_open = 1; g_eu_dirty = 0; }
    g_eu_open = 1;
    g_eu_stroke_vt = stroke ? vt : NULL;
    if (g_log) log_locked("# editor undo: opened (%d units listed) in %.1f ms\n", g_eu_was_n, eu_ms(t0));
}
// (a step is closed by the next thing done or by the undo, not here: a new unit is on the map's grid only after the world's step)
internal void eu_after(void) { g_eu_depth--; }

struct eu_pair { int was, now; };
internal int eu_pair_cmp(const void *a, const void *b) {
    int x = ((const struct eu_pair *)a)->was, y = ((const struct eu_pair *)b)->was;
    return x < y ? -1 : x > y;
}
internal void eu_rename(int *id, const struct eu_pair *p, int n) {
    struct eu_pair key = { *id, 0 };
    const struct eu_pair *f = bsearch(&key, p, (size_t)n, sizeof *p, eu_pair_cmp);
    if (f) *id = f->now;
}
// the units made again have their ids by now (a frame has passed): every step's lists told
internal void eu_settle(void) {
    if (!g_eu_late_n) return;
    struct eu_pair *p = malloc((size_t)g_eu_late_n * sizeof *p);
    int np = 0;
    for (int i = 0; p && i < g_eu_late_n; i++) {
        int now = unit_get_serial(g_eu_late[i].u);
        if (now) { p[np].was = g_eu_late[i].was; p[np++].now = now; }
    }
    if (np) {
        qsort(p, (size_t)np, sizeof *p, eu_pair_cmp);
        for (int k = 0; k < g_eu_n; k++) {
            struct eu_step *q = &g_eu_steps[k];
            for (int i = 0; i < q->nmade; i++) eu_rename(&q->made[i].id, p, np);
            for (int i = 0; i < q->ngone; i++) eu_rename(&q->gone[i].id, p, np);
            for (int i = 0; i < q->nmoved; i++) { eu_rename(&q->moved[i].was.id, p, np); eu_rename(&q->moved[i].now.id, p, np); }
        }
    }
    free(p);
    free(g_eu_late);
    g_eu_late = NULL;
    g_eu_late_n = 0;
}
internal void eu_make(const struct eu_unit *r) {
    ee_unit_type *td = type_data(r->type);
    if (!td || r->player < 0 || !game_players()[r->player]) return;
    float m[16];
    memcpy(m, r->m, sizeof m);
    ee_unit *u = se_unit_make(td, m, r->player);
    if (!u) return;
    if (unit_type_is_land_building(td)) unit_set_built(u, 1, 0, 1);
    unit_start_anim(u, r->anim, -1, 0);
    unit_set_hp(u, r->hp);
    struct eu_late *b = realloc(g_eu_late, (size_t)(g_eu_late_n + 1) * sizeof *b);
    if (!b) return;
    g_eu_late = b;
    g_eu_late[g_eu_late_n].was = r->id;
    g_eu_late[g_eu_late_n++].u = u;
}
internal void eu_pose(const struct eu_unit *r) {
    ee_unit *u = unit_by_id((unsigned)r->id);
    if (!u) return;
    float m[16];
    memcpy(m, r->m, sizeof m);
    unit_place_matrix(u, m, 1);
    if (unit_get_hp(u) != r->hp) unit_set_hp(u, r->hp);
    if (unit_get_anim(u) != r->anim) unit_start_anim(u, r->anim, -1, 0);
}
// the ground back to side (0 before the action, 1 after): the heights first, what the game makes of them, then the tiles' own data
internal void eu_ground_set(const struct eu_step *s, int side) {
    ee_map *map = game_map();
    if (!map || !s->ncells) return;
    void *mesh = map_mesh_at(map);
    int heights = 0;
    for (int i = 0; i < s->ncells; i++) {
        const struct eu_cell *c = &s->cells[i];
        if (c->h[0] == c->h[1]) continue;
        mesh_set_z(mesh, c->x, c->y, c->h[side]);
        map_refresh_corner(map, c->x, c->y);
        heights = 1;
    }
    if (heights) {
        // what the elevation brush does at its end: the corners' normals (the ground's shading) and the shores, one round the box
        for (int y = s->y0 - 1; y <= s->y1 + 1; y++)
            for (int x = s->x0 - 1; x <= s->x1 + 1; x++)
                if (map_corner(map, x, y)) mesh_compute_normal(mesh, (unsigned long)x, (unsigned long)y);
        for (int y = s->y0 - 1; y <= s->y1 + 1; y++)
            for (int x = s->x0 - 1; x <= s->x1 + 1; x++)
                if (map_tile_texture(map, x, y) >= 0) mesh_compute_shorelines(mesh, (unsigned long)x, (unsigned long)y);
        map_refresh_tiles(map, (uint32_t)s->x0, (uint32_t)s->x1 + 1, (uint32_t)s->y0, (uint32_t)s->y1 + 1);
        map_cliff_pass(map, (uint32_t)(s->x0 > 2 ? s->x0 - 2 : 0), (uint32_t)s->x1 + 3, s->y0 - 2, s->y1 + 3);
    }
    for (int i = 0; i < s->ncells; i++) {
        const struct eu_cell *c = &s->cells[i];
        ee_tile *t = c->x < map_get_size(map) && c->y < map_get_size(map) ? map_get_tiles(map)[c->y * map_get_size(map) + c->x] : NULL;
        if (t && c->tex[side] != 0xFF && map_tile_texture(map, c->x, c->y) != c->tex[side]) map_set_texture(map, c->x, c->y, c->tex[side], 1);
        if (t && c->kind[side] != 0xFF && tile_get_kind(t) != c->kind[side]) {
            int pos[2] = { c->x, c->y };
            tile_change_kind(t, c->kind[side], pos, map);
        }
        const ee_corner *k = map_corner(map, c->x, c->y);
        if (k && corner_get_color(k) != c->color[side]) mesh_set_color_index(mesh, c->x, c->y, c->color[side]);
    }
    // (the box may hold the last corners, x or y = size: the minimap's tables go by tiles)
    int last = (int)map_get_size(map) - 1;
    ground_tell(s->x0, s->x1 < last ? s->x1 : last, s->y1 < last ? s->y1 : last, s->y0);
}
// the step taken back (undo) or done again: units first gone, then those brought back, then the moved, then the ground
internal void eu_apply(struct eu_step *s, int undo) {
    DWORD t0 = GetTickCount();
    struct eu_unit *take = undo ? s->made : s->gone, *bring = undo ? s->gone : s->made;
    int nt = undo ? s->nmade : s->ngone, nb = undo ? s->ngone : s->nmade;
    g_eu_busy = 1;
    for (int i = 0; i < nt; i++)
        if (unit_by_id((unsigned)take[i].id)) se_unit_delete_by_id(take[i].id, take[i].player);
    for (int i = 0; i < nb; i++) eu_make(&bring[i]);
    for (int i = 0; i < s->nmoved; i++) eu_pose(undo ? &s->moved[i].was : &s->moved[i].now);
    eu_ground_set(s, !undo);
    eu_ground_sync(s);
    g_eu_busy = 0;
    if (g_log) log_locked("# editor undo: %s: %d taken, %d brought back, %d moved, %d of the ground, %lu ms\n", undo ? "undo" : "redo", nt, nb, s->nmoved, s->ncells, GetTickCount() - t0);
}
internal void __thiscall hook_eu_undo(ee_se_undo *a) {
    (void)a;
    eu_settle();
    eu_close();
    if (g_eu_cur <= 0) { if (g_log) log_locked("# editor undo: nothing to undo\n"); return; }
    eu_apply(&g_eu_steps[--g_eu_cur], 1);
}
internal void __thiscall hook_eu_redo(ee_se_redo *a) {
    (void)a;
    eu_settle();
    eu_close();
    if (g_eu_cur >= g_eu_n) { if (g_log) log_locked("# editor undo: nothing to redo\n"); return; }
    eu_apply(&g_eu_steps[g_eu_cur++], 0);
}

// another game or scenario: the steps were another's
internal void editor_undo_new_game(void) { eu_clear(); }
// the game's quick load (back from a test of the scenario): no frame while the world is rebuilt, the steps gone with it
internal int __cdecl hook_eu_read(void *spec, int a, int b, uint8_t c) {
    rd_world_rebuild(1);
    int r = scenario_read(spec, a, b, c);
    rd_world_rebuild(0);
    eu_clear();
    return r;
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
    internal void __thiscall hook_eu_##site(ee_##site *a) { eu_before(*(void **)a, stroke, stroke != 1); eesea_##klass##_run(a); eu_after(); }
EU_HOOK(se_place_unit, place_unit, 0)
EU_HOOK(se_place_wall, place_wall, 0)
EU_HOOK(se_place_gate, place_gate, 0)
EU_HOOK(se_delete, delete_unit, 0)
EU_HOOK(se_rotate, rotate_units, 0)
EU_HOOK(se_paint_area, paint_area, 1)
EU_HOOK(se_paint_elevation, paint_elevation, 1)
EU_HOOK(se_paint_mountains, paint_moutains, 1)
// the terrain paint: its units listed only if its terrain scatters objects
internal void __thiscall hook_eu_se_paint_terrain(ee_se_paint_terrain *a) {
    const ee_terrain *t = terrain_at((uint32_t)se_paint_terrain_get_terrain(a));
    eu_before(*(void **)a, 1, t && terrain_get_objects_n(t) > 0);
    eesea_paint_terrain_run(a);
    eu_after();
}
// the paint tools tell the minimap what they changed: the open stroke's box
internal void __thiscall hook_eu_minimap(ee_minimap *m, int x_lo, int x_hi, int y_hi, int y_lo) {
    if (g_eu_stroke_open && !g_eu_busy) {
        if (!g_eu_dirty) { g_eu_dirty = 1; g_eu_dx0 = x_lo, g_eu_dx1 = x_hi, g_eu_dy0 = y_lo, g_eu_dy1 = y_hi; }
        else {
            if (x_lo < g_eu_dx0) g_eu_dx0 = x_lo;
            if (x_hi > g_eu_dx1) g_eu_dx1 = x_hi;
            if (y_lo < g_eu_dy0) g_eu_dy0 = y_lo;
            if (y_hi > g_eu_dy1) g_eu_dy1 = y_hi;
        }
    }
    ee_mini_map_redraw_tiles(m, x_lo, x_hi, y_hi, y_lo);
}
// a move: the pick-up opens the step, the putting down closes it
internal void __thiscall hook_eu_se_move(ee_se_move *a) {
    int same = g_eu_stroke_vt == *(void **)a && g_eu_open && !se_move_get_lift(a);
    if (same) { eesea_move_unit_run(a); return; }
    eu_before(*(void **)a, 2, 1);
    eesea_move_unit_run(a);
    eu_after();
}
// a new map: nothing of before stands
internal void __thiscall hook_eu_se_blank_map(ee_se_blank_map *a) { eu_clear(); g_eu_depth++; eesea_create_blank_map_run(a); g_eu_depth--; }
internal void __thiscall hook_eu_se_random_map(ee_se_random_map *a) { eu_clear(); g_eu_depth++; eesea_create_random_map_run(a); g_eu_depth--; }

internal void editor_undo_install(void) {
    int n = patch_se_place_unit(hook_eu_se_place_unit) + patch_se_place_wall(hook_eu_se_place_wall) + patch_se_place_gate(hook_eu_se_place_gate)
            + patch_se_delete(hook_eu_se_delete) + patch_se_move(hook_eu_se_move) + patch_se_rotate(hook_eu_se_rotate)
            + patch_se_blank_map(hook_eu_se_blank_map) + patch_se_random_map(hook_eu_se_random_map)
            + patch_se_paint_area(hook_eu_se_paint_area) + patch_se_paint_elevation(hook_eu_se_paint_elevation)
            + patch_se_paint_mountains(hook_eu_se_paint_mountains) + patch_se_paint_terrain(hook_eu_se_paint_terrain)
            + patch_se_undo(hook_eu_undo) + patch_se_redo(hook_eu_redo) + patch_minimap_redraw(hook_eu_minimap);
    int read = patch_se_quick_read(hook_eu_read);
    if (g_log) fprintf(g_log, "# editor: undo %s\n", n == 15 && read ? "hooked" : "FAILED");
}
