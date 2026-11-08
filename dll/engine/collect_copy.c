// STEP 3: THE WORLD'S COPY THE COLLECTION READS (the pipe: picture collect copy on | check | off; off by default -
// the walk then copies the tiles it needs itself, at its start). Everything the walk and its fills (the opacity, the
// view test, the objects', trees' and doppelgangers' fills) read of the world, copied: per tile of a region its sight
// for the local player, the objects on it in the walk's order (an object made of parts from its first part on) and the
// buildings and trees it remembers there; per object the values the fills read of it, its complex unit, its stats and
// the units inside it (docs/renderer.md "The units' collection"). The rest the fills read is the game's data (types,
// graphics, the players' colours and names, the ground) or the frame's (the camera, the selection, the unit under the
// mouse, the interface's settings).
// Made by the simulation when its steps let go of the world's write lock (render_3d.c rd_hook_write_unlock, the loop's
// lock: spec engine_display.step_write_return), for the frame's last view and a margin round it; any other writer (the
// interface's, the lobby's, a load) only makes it stale - the walk then copies the world itself (it holds the read
// lock), as it does when the copy isn't for its view, its player, its free camera or show-hidden mode. So whenever the
// walk reads a copy the world is as it was copied, and the objects' pointers in it (u, cu, the doppelgangers) are as
// good as the game's own walk's - the picker, the met sets, the ambient sounds and the selection's groups take them,
// the fills never read through them. Three copies: the newest, the one the frame reads, the one being made.
// update_hidden (the opacity's "show hidden units" 1) is the one call that writes to a unit: made for an obstacle or a
// moved unit of the region when the copy is made, where the game asks it for the ones it draws.
#define UC_MARGIN 4   // tiles round the frame's last view the simulation's copy takes
enum { UC_PM = 0x4C, UC_COPIES = 3 };
typedef struct {
    ee_unit *u, *cu, *tcu;      // the objects as they were (never read through: the identities the game's lists take)
    ee_unit_type *ty, *cty;     // the game's data
    ee_unit_stats *st, *cst;
    ee_dmg_model *gfx;
    ee_player *owner, *cowner;
    int32_t x, y, serial, hp, st_hp, cu_hp, anim, anim_frame, next_frame, ai_state, producing, produce_left, goal_state,
            rot_ms, rot_t, owner_index;
    uint32_t goal_value, n_parts, parts_at, n_a, a_at, n_b, b_at, n_inside, inside_at, target_id;
    float height, cu_height, fraction, cu_opacity, power_bar, own_bar, gate_open, range, sight, los, tile_z;
    float tint[3], at[3];
    uint8_t tree, has_cu, food, moved, hidden, rotting, cloakable, cloaked, cloak_friend, marker, pp_shown, dead,
            foundation, finished, tint_on, selected, gate, gate_shown, own_seen, lit, bars_friend, marker_ok, t_foundation,
            open;
    __attribute__((aligned(8))) uint8_t pm[UC_PM];   // its complex unit's physical model (its pose, model, scale, frame)
} uc_obj;
typedef struct {
    ee_doppelganger *d;   // as it was (the met set's)
    ee_unit *u;           // its object (the picker's)
    ee_unit_type *ty;
    ee_player *uowner;
    ee_dmg_model *gfx;
    int32_t anim, frame, hp, full, serial;
    float scale;
    float t[16];
    uint8_t skip, has_t, finished, dead, open;
} uc_dopp;
typedef struct {
    ee_tile *t;           // the world's (NULL off the map; the game's own functions in the checks take it)
    uint32_t obj_at, n_obj, dopp_at, n_dopp;
    uint8_t explored, seen, seen_me;   // seen: by the walk's player; seen_me: by the local one (the view test's)
} uc_ctile;
typedef struct {
    int x0, y0, w, h;     // the region, in tiles
    ee_map *map;
    ee_player *pl;
    int free_cam, hidden_mode, made_by;   // made_by: 1 the simulation, 2 the frame
    uc_ctile *tiles;
    uint32_t tiles_cap;
    uc_obj *objs;
    uint32_t n_objs, objs_cap;
    uint32_t *seq;        // the tiles' objects, indices into objs in the walk's order
    uint32_t n_seq, seq_cap;
    uc_dopp *dopps;
    uint32_t n_dopps, dopps_cap;
    uint32_t *pool;       // the objects' lists: part values, the two lists' pairs, the units inside's bars
    uint32_t n_pool, pool_cap;
    int ok;               // the arrays held everything
} uc_copy;

internal int cp_grow(void **p, uint32_t *cap, uint32_t need, size_t elem) {
    if (need <= *cap) return 1;
    uint32_t n = *cap ? *cap : 256;
    while (n < need) n *= 2;
    void *q = realloc(*p, (size_t)n * elem);
    if (!q) return 0;
    *p = q;
    *cap = n;
    return 1;
}
internal uint32_t cp_pool_add(uc_copy *c, const void *v, uint32_t words) {
    if (!cp_grow((void **)&c->pool, &c->pool_cap, c->n_pool + words, 4)) { c->ok = 0; return 0; }
    uint32_t at = c->n_pool;
    memcpy(c->pool + at, v, (size_t)words * 4);
    c->n_pool += words;
    return at;
}

// the objects met in this copy: their index by their pointer (an object on several tiles is copied once)
typedef struct { ee_unit *u; uint32_t i, gen; } cp_seen_e;
internal cp_seen_e *g_cp_seen;
internal uint32_t g_cp_seen_cap, g_cp_gen;
internal uint32_t cp_seen_hash(ee_unit *u) { return ((uint32_t)(uintptr_t)u >> 3) * 2654435761u & (g_cp_seen_cap - 1); }
// room for n at half full: a bigger table, this copy's entries moved into it
internal int cp_seen_grow(uint32_t n) {
    if (n * 2 <= g_cp_seen_cap) return 1;
    uint32_t cap = g_cp_seen_cap ? g_cp_seen_cap : 4096, old_cap = g_cp_seen_cap;
    while (cap < n * 2) cap *= 2;
    void *q = calloc(cap, sizeof *g_cp_seen);
    if (!q) return 0;
    cp_seen_e *o = g_cp_seen;
    g_cp_seen = q;
    g_cp_seen_cap = cap;
    uint32_t gen = g_cp_gen ? g_cp_gen : 1;
    for (uint32_t k = 0; k < old_cap; k++)
        if (o[k].gen == g_cp_gen && g_cp_gen) {
            uint32_t h = cp_seen_hash(o[k].u);
            while (g_cp_seen[h].gen == gen) h = (h + 1) & (cap - 1);
            g_cp_seen[h].u = o[k].u, g_cp_seen[h].i = o[k].i, g_cp_seen[h].gen = gen;
        }
    g_cp_gen = gen;
    free(o);
    return 1;
}
internal uint32_t *cp_seen_slot(ee_unit *u, int *found) {
    uint32_t h = cp_seen_hash(u);
    while (g_cp_seen[h].gen == g_cp_gen && g_cp_seen[h].u != u) h = (h + 1) & (g_cp_seen_cap - 1);
    *found = g_cp_seen[h].gen == g_cp_gen;
    g_cp_seen[h].u = u;
    g_cp_seen[h].gen = g_cp_gen;
    return &g_cp_seen[h].i;
}

internal int uc_shows_bars(ee_unit *cu, ee_player *me) {
    return unit_count_inside(cu) && (unit_get_owner(cu) == me || player_stance_to(unit_get_owner(cu), player_get_index(me)) == 0);
}
internal ee_tile *uc_tile(ee_map *map, int x, int y) {
    uint32_t n = map_get_size(map);
    return (uint32_t)x < n && (uint32_t)y < n ? map_get_tiles(map)[(uint32_t)y * n + (uint32_t)x] : NULL;
}

// an object's values as the fills read them (the expressions theirs, in their order where a call may write)
internal void cp_obj(uc_copy *c, uc_obj *o, ee_unit *u, ee_player *me, ee_map *map, int mode) {
    memset(o, 0, sizeof *o);
    ee_unit *cu = unit_get_physics(u);
    ee_unit_type *ty = unit_get_type_data(u);
    ee_unit_stats *st = unit_get_stats(u);
    o->u = u, o->cu = cu, o->ty = ty, o->st = st;
    o->gfx = unit_graphics(u);
    o->owner = unit_get_owner(u);
    o->x = unit_get_x(u), o->y = unit_get_y(u), o->serial = unit_get_serial(u), o->hp = unit_get_hp(u);
    o->st_hp = unit_stats_get_hp(st);
    o->height = unit_stats_get_height(st);
    o->owner_index = player_get_index(o->owner);
    o->tree = unit_type_get_is_tree(ty) != 0;
    o->has_cu = cu != NULL;
    o->food = unit_holds_food(u) && cu && unit_type_get_class_id(ty) != CLS_LAND_CITIZEN &&
              unit_type_get_class_id(ty) != CLS_NAVAL_CITIZEN && o->owner_index == 0;
    o->moved = (uint8_t)unit_moved(u);
    if (mode == 1 && (unit_type_get_is_obstacle(ty) || o->moved)) o->hidden = unit_update_hidden(u) != 0;
    o->selected = unit_get_selected(u) != 0;
    if (o->tree) {
        o->open = !tile_has_tree(uc_tile(map, o->x - 1, o->y - 1)) || !tile_has_tree(uc_tile(map, o->x + 1, o->y + 1));
        return;
    }
    o->marker = unit_is_path_marker(u) != 0;
    o->pp_shown = unit_path_point_shown(u) != 0;
    if (o->food) o->own_seen = unit_vis_seen_by(unit_get_vis(u), me) != 0;
    o->lit = (uint8_t)map_tile_lit(map, o->x, o->y);
    o->own_bar = (float)((long double)o->hp / o->st_hp);
    if (!cu) {
        o->power_bar = fill_consts_now().bar_full;
        if (o->marker) {
            o->tile_z = tile_get_height((ee_tile *)unit_get_vis(u));
            ee_unit *target = world_unit((uint32_t)o->hp), *tcu;
            if (target && (tcu = unit_get_physics(world_unit((uint32_t)o->hp)))) {
                o->tcu = tcu;
                o->t_foundation = unit_is_foundation(tcu) != 0;
                o->target_id = (uint32_t)unit_get_serial(tcu) & 0xFFFFFF;
                float *at;
                if (marker_place(unit_get_physics(target), o->x, o->y, &at)) {
                    o->marker_ok = 1;
                    o->at[0] = at[0], o->at[1] = at[1], o->at[2] = at[2];
                }
            }
        }
        return;
    }
    ee_unit_type *cty = unit_get_type_data(cu);
    o->cty = cty;
    o->cst = unit_get_stats(cu);
    o->cowner = unit_get_owner(cu);
    memcpy(o->pm, unit_matrix_at(cu), physical_model_size() < UC_PM ? physical_model_size() : UC_PM);
    o->cu_hp = unit_get_hp(cu);
    o->cu_height = unit_get_height(cu);
    o->anim = unit_get_anim(cu);
    o->anim_frame = unit_get_anim_frame(cu);
    o->next_frame = unit_get_next_frame(cu);
    o->fraction = unit_get_fraction(cu);
    o->dead = (uint8_t)unit_dead(cu);
    o->foundation = unit_is_foundation(cu) != 0;
    o->finished = unit_get_finished(cu) != 0;
    o->rotting = (uint8_t)unit_rotting(cu);
    if (o->rotting) {   // EE: the object's type and its complex unit's rot; AoC: the complex unit's type and its complex unit's
        ee_unit *r = build_is("aoc") ? cu : u;
        o->rot_t = unit_type_get_rot_time(unit_get_type_data(r));
        o->rot_ms = unit_get_rot_ms(unit_get_physics(r));
    }
    o->cloakable = unit_get_cloakable(cu) != 0;
    o->cloaked = build_is("aoc") ? unit_get_cloaked(cu) != 0 : 0;
    o->cu_opacity = unit_get_opacity(cu);
    o->cloak_friend = me == o->cowner || player_stance_to(o->cowner, player_get_index(me)) == 0 || player_get_out(me);
    o->tint_on = unit_get_tint_on(cu) != 0;
    memcpy(o->tint, unit_tint_at(cu), 12);
    if (unit_type_get_class_id(cty) == CLS_GATE) {
        void *gate = gate_class_of(cty);
        o->gate = 1;
        o->gate_shown = unit_get_hp(cu) > 0 && ((!unit_get_chained(cu) && unit_get_chain(cu)) || gate_drawn_open(gate, cu));
        o->gate_open = unit_get_gate_open(cu);
    }
    ee_goal *g = unit_get_goal(cu);
    o->goal_state = g ? goal_get_state(g) : -1;
    o->range = unit_stats_get_range(st);
    o->sight = unit_stats_get_sight(st);
    if (build_is("aoc")) o->los = line_of_sight(cu);
    if (!o->foundation) {
        uint8_t *gc = (uint8_t *)unit_graphics(cu);
        o->n_parts = vector_size(gc + 4);
        o->parts_at = cp_pool_add(c, unit_get_part_values(cu), o->n_parts);
        o->n_a = unit_get_list_a_n(cu);
        o->a_at = c->n_pool;
        for (uint32_t i = 0; i < o->n_a; i++) {
            uint8_t *e = unit_get_list_a(cu)[i];
            uint32_t pair[2] = { *(uint32_t *)(e + 0xC), *(uint32_t *)(e + 4) };
            cp_pool_add(c, pair, 2);
        }
        o->n_b = unit_get_list_b_n(cu);
        o->b_at = c->n_pool;
        for (uint32_t i = 0; i < o->n_b; i++) {
            uint8_t *e = unit_get_list_b(cu)[i];
            uint32_t pair[2] = { *(uint32_t *)(e + 0xC), *(uint32_t *)(e + 4) };
            cp_pool_add(c, pair, 2);
        }
    }
    o->bars_friend = (uint8_t)uc_shows_bars(cu, me);
    if (o->bars_friend) {
        o->n_inside = (uint32_t)unit_count_inside(cu);
        o->inside_at = c->n_pool;
        for (uint32_t i = 0; i < o->n_inside; i++) {
            ee_unit *in = ((ee_unit **)vector_first(unit_inside_at(cu) - 4))[i];
            float v = (float)((long double)unit_get_hp(in) / unit_stats_get_hp(unit_get_stats(in)));
            cp_pool_add(c, &v, 1);
        }
    }
    int power = build_is("aoc") ? unit_stats_get_power(st) != 0 : unit_type_get_has_power(ty) != 0;
    o->power_bar = power ? (float)((long double)unit_get_power(cu) / unit_stats_get_power(st)) : fill_consts_now().bar_full;
    o->producing = unit_get_producing(cu);
    o->produce_left = unit_get_produce_left(cu);
    ee_ai_state *s = unit_get_ai_state(cu);
    o->ai_state = s ? (int32_t)ai_state_get_id(s) : 0;
    for (void **p = unit_get_goals_first(cu); p != unit_get_goals_end(cu); p++) {
        if (o->hp <= 0) continue;
        uint8_t *h = *p;
        int kind = *(int *)(*(uint8_t **)(h + 0x10) + 0x20);
        if (kind == 5 || kind == 6 || kind == 2) {
            o->goal_value = ((uint32_t (__thiscall *)(void *))(*(void ***)h)[6])(h);
            break;
        }
    }
}
// a remembered building or tree: its doppelganger's answers (the fills' calls, in their order) at the walk's tile x, y
internal void cp_dopp(uc_dopp *o, ee_doppelganger *d, ee_player *pl, ee_map *map, int x, int y) {
    memset(o, 0, sizeof *o);
    ee_unit *u = doppelganger_get_unit(d);
    o->d = d, o->u = u;
    o->serial = unit_get_serial(u);
    o->skip = unit_get_list_node(u) && unit_get_physics(u) && unit_is_foundation(unit_get_physics(u)) && unit_get_owner(u) != pl;
    if (o->skip) return;
    o->ty = unit_get_type_data(u);
    o->uowner = unit_get_owner(u);
    o->scale = doppelganger_scale(d);
    o->has_t = doppelganger_has_transform(d) == 1;
    if (o->has_t) doppelganger_transform(d, o->t);
    o->anim = doppelganger_anim(d);
    o->gfx = doppelganger_graphics(d);
    o->dead = doppelganger_dead(d) != 0;
    o->finished = doppelganger_finished(d) != 0;
    o->hp = doppelganger_hp(d);
    o->frame = doppelganger_frame(d);
    o->full = unit_stats_get_hp(player_stats_of(o->uowner, unit_type_get_id(o->ty)));
    if (unit_type_get_is_tree(o->ty))
        o->open = !tile_has_tree(uc_tile(map, x - 1, y - 1)) || !tile_has_tree(uc_tile(map, x + 1, y + 1));
}

// the copy of the region [x0, x1] x [y0, y1] for player pl; 0 when out of memory
internal int cp_make(uc_copy *c, ee_map *map, ee_player *pl, int x0, int y0, int x1, int y1, int made_by) {
    int mode = show_hidden_mode(), free_cam = *game_free_camera_at() != 0;
    c->x0 = x0, c->y0 = y0, c->w = x1 - x0 + 1, c->h = y1 - y0 + 1;
    c->map = map, c->pl = pl, c->free_cam = free_cam, c->hidden_mode = mode, c->made_by = made_by;
    c->n_objs = c->n_seq = c->n_dopps = c->n_pool = 0;
    c->ok = 1;
    uint32_t nt = (uint32_t)(c->w * c->h);
    if (c->w <= 0 || c->h <= 0 || !cp_grow((void **)&c->tiles, &c->tiles_cap, nt, sizeof *c->tiles) ||
        !cp_seen_grow(c->objs_cap > 2048 ? c->objs_cap : 2048))
        return c->ok = 0;
    if (++g_cp_gen == 0) { memset(g_cp_seen, 0, (size_t)g_cp_seen_cap * sizeof *g_cp_seen); g_cp_gen = 1; }
    ee_player *me = game_players()[local_player_index()];
    for (int y = y0; y <= y1 && c->ok; y++)
        for (int x = x0; x <= x1 && c->ok; x++) {
            uc_ctile *ct = &c->tiles[(uint32_t)((y - y0) * c->w + (x - x0))];
            ee_tile *t = uc_tile(map, x, y);
            ee_unit_vis *v = (ee_unit_vis *)t;
            memset(ct, 0, sizeof *ct);
            ct->t = t;
            ct->obj_at = c->n_seq, ct->dopp_at = c->n_dopps;
            if (!t) continue;
            ct->explored = unit_vis_explored(v, pl) != 0;
            ct->seen = unit_vis_seen_by(v, pl) != 0;
            ct->seen_me = pl == me ? ct->seen : unit_vis_seen_by(v, me) != 0;
            if (ct->explored || free_cam)
                for (void *node = tile_get_objects(t); node && c->ok; node = tile_node_get_next(node)) {
                    // an object made of parts (a ship and its sails ...): from its first part on
                    ee_unit *o = tile_node_get_object(node);
                    if (unit_get_physics(o)) {
                        o = unit_get_physics(o);
                        while (unit_get_chain(o)) o = unit_get_chain(o);
                    }
                    do {
                        if (!cp_seen_grow(c->n_objs + 1) || !cp_grow((void **)&c->seq, &c->seq_cap, c->n_seq + 1, 4)) { c->ok = 0; break; }
                        int found;
                        uint32_t *slot = cp_seen_slot(o, &found);
                        if (!found) {
                            if (!cp_grow((void **)&c->objs, &c->objs_cap, c->n_objs + 1, sizeof *c->objs)) { c->ok = 0; break; }
                            *slot = c->n_objs;
                            cp_obj(c, &c->objs[c->n_objs++], o, me, map, mode);
                        }
                        c->seq[c->n_seq++] = *slot;
                        ct->n_obj++;
                        o = unit_get_physics(o) ? unit_get_chained(unit_get_physics(o)) : NULL;
                    } while (o);
                }
            if (ct->seen || free_cam) continue;
            for (uint32_t i = 0; i < unit_vis_doppelganger_count(v) && c->ok; i++) {
                ee_doppelganger *d = tile_get_doppelgangers(t)[i];
                if (doppelganger_get_player(d) != pl) continue;
                if (!cp_grow((void **)&c->dopps, &c->dopps_cap, c->n_dopps + 1, sizeof *c->dopps)) { c->ok = 0; break; }
                cp_dopp(&c->dopps[c->n_dopps++], d, pl, map, x, y);
                ct->n_dopp++;
            }
        }
    return c->ok;
}

// THE THREE COPIES: the newest (g_cp_new), the one the frame reads (g_cp_read) and one for the next - made by the
// simulation under the world's write lock or by the frame under its read lock, never both at once (the lock)
internal uc_copy g_cp[UC_COPIES], g_cp_chk;
internal CRITICAL_SECTION g_cp_cs;
internal int g_cp_cs_ok, g_cp_new = -1, g_cp_read = -1;
internal volatile int g_cp_on;           // the pipe's picture collect copy: 0 off (the walk copies its view itself), 1 on, 2 check
internal volatile LONG g_cp_stale = 1;   // a writer other than the steps let go since the newest copy
internal volatile LONG g_cp_view[5];     // the frame's last view x0, y0, x1, y1 (the walk's), and 1 once it has one
internal ee_map *volatile g_cp_view_map;
internal LONG g_cp_by_step, g_cp_by_frame, g_cp_stales, g_cp_fails, g_cp_used, g_cp_objs;
internal LONGLONG g_cp_step_qpc, g_cp_frame_qpc;   // the simulation's time and the frame's making them
internal int cp_spare(void) {   // inside g_cp_cs
    for (int i = 0; i < UC_COPIES; i++)
        if (i != g_cp_new && i != g_cp_read) return i;
    return -1;
}
// at the end of a write to the world (render_3d.c rd_hook_write_unlock, its holder still in; ret: where it took the
// lock): the steps' copy of the frame's last view and UC_MARGIN round it, else the copy stale
internal void collect_copy_written(uintptr_t ret) {
    if (!g_cp_on || !g_cp_cs_ok) return;
    ee_map *map = game_map();
    if (ret != (uintptr_t)engine_display_step_write_return_at() || !g_cp_view[4] || !map || map != g_cp_view_map) {
        InterlockedExchange(&g_cp_stale, 1);
        g_cp_stales++;
        return;
    }
    EnterCriticalSection(&g_cp_cs);
    int i = cp_spare();
    LeaveCriticalSection(&g_cp_cs);
    LONGLONG t0 = rd_qpc();
    int ok = cp_make(&g_cp[i], map, game_players()[local_player_index()], g_cp_view[0] - UC_MARGIN, g_cp_view[1] - UC_MARGIN,
                     g_cp_view[2] + UC_MARGIN, g_cp_view[3] + UC_MARGIN, 1);
    g_cp_step_qpc += rd_qpc() - t0;
    EnterCriticalSection(&g_cp_cs);
    if (ok) g_cp_new = i, g_cp_stale = 0, g_cp_by_step++, g_cp_objs = (LONG)g_cp[i].n_objs;
    else g_cp_stale = 1, g_cp_fails++;
    LeaveCriticalSection(&g_cp_cs);
}
internal int cp_fits(const uc_copy *c, ee_map *map, ee_player *pl, int x0, int y0, int x1, int y1) {
    return c->ok && c->map == map && c->pl == pl && c->free_cam == (*game_free_camera_at() != 0) &&
           c->hidden_mode == show_hidden_mode() && x0 >= c->x0 && y0 >= c->y0 && x1 < c->x0 + c->w && y1 < c->y0 + c->h;
}
// the walk's copy of its view [x0, x1] x [y0, y1]: the newest when it fits (its view, player, free camera, show-hidden
// mode) and no other writer came since, else made now (the walk holds the read lock); NULL out of memory
internal uc_copy *cp_take(ee_map *map, ee_player *pl, int x0, int y0, int x1, int y1) {
    g_cp_view[0] = x0, g_cp_view[1] = y0, g_cp_view[2] = x1, g_cp_view[3] = y1, g_cp_view_map = map, g_cp_view[4] = 1;
    uc_copy *c = NULL;
    EnterCriticalSection(&g_cp_cs);
    if (g_cp_on && g_cp_new >= 0 && !g_cp_stale && cp_fits(&g_cp[g_cp_new], map, pl, x0, y0, x1, y1)) c = &g_cp[g_cp_read = g_cp_new];
    int i = c ? -1 : (g_cp_read = cp_spare());
    LeaveCriticalSection(&g_cp_cs);
    if (c) { g_cp_used++; return c; }
    LONGLONG t0 = rd_qpc();
    int m = g_cp_on ? UC_MARGIN : 0, ok = cp_make(&g_cp[i], map, pl, x0 - m, y0 - m, x1 + m, y1 + m, 2);
    g_cp_frame_qpc += rd_qpc() - t0;
    g_cp_by_frame++;
    EnterCriticalSection(&g_cp_cs);
    if (ok) g_cp_new = i, g_cp_stale = 0;
    else g_cp_fails++;
    LeaveCriticalSection(&g_cp_cs);
    return ok ? &g_cp[i] : NULL;
}
internal void cp_release(void) {
    EnterCriticalSection(&g_cp_cs);
    g_cp_read = -1;
    LeaveCriticalSection(&g_cp_cs);
}

// copy check: the simulation's copy against one made now from the world (the walk holds the read lock: it is as the
// copy left it) - every tile, object, doppelganger and list, byte for byte, but update_hidden's state: the copy's own
// call clears a moved unit's bit after reading it (the game's opacity too reads it before its call), and asked again it
// may answer otherwise (its 3 s up)
internal LONG g_cp_chk_n, g_cp_chk_diff, g_cp_chk_objs;
internal void cp_check(const uc_copy *c) {
    if (c->made_by != 1 || !cp_make(&g_cp_chk, c->map, c->pl, c->x0, c->y0, c->x0 + c->w - 1, c->y0 + c->h - 1, 2)) return;
    const uc_copy *f = &g_cp_chk;
    g_cp_chk_n++;
    g_cp_chk_objs += (LONG)c->n_objs;
    const char *what = NULL;
    uint32_t at = 0, off = 0;
    if (f->n_objs != c->n_objs || f->n_seq != c->n_seq || f->n_dopps != c->n_dopps || f->n_pool != c->n_pool) what = "counts";
    for (uint32_t i = 0; !what && i < (uint32_t)(c->w * c->h); i++)
        if (memcmp(&c->tiles[i], &f->tiles[i], sizeof *c->tiles)) what = "tile", at = i;
    if (!what && memcmp(c->seq, f->seq, (size_t)c->n_seq * 4)) what = "tiles' objects";
    for (uint32_t i = 0; !what && i < c->n_objs; i++) {
        uc_obj a = c->objs[i], b = f->objs[i];
        a.hidden = b.hidden, a.moved = b.moved;   // update_hidden's: the copy's call cleared its moved bit
        if (memcmp(&a, &b, sizeof a)) {
            what = "object", at = i;
            while (off < sizeof a && !memcmp((uint8_t *)&a + off, (uint8_t *)&b + off, 1)) off++;
        }
    }
    for (uint32_t i = 0; !what && i < c->n_dopps; i++)
        if (memcmp(&c->dopps[i], &f->dopps[i], sizeof *c->dopps)) what = "doppelganger", at = i;
    if (!what && memcmp(c->pool, f->pool, (size_t)c->n_pool * 4)) what = "lists";
    if (what && g_cp_chk_diff++ < 8)
        log_locked("# collect: the copy differs from the world: %s %u (+%u)%s%d; objects %u / %u, tiles' %u / %u, doppelgangers %u / %u\n",
                   what, at, off, !strcmp(what, "object") ? ", type " : "", !strcmp(what, "object") ? unit_type_get_id(c->objs[at].ty) : 0,
                   c->n_objs, f->n_objs, c->n_seq, f->n_seq, c->n_dopps, f->n_dopps);
}
