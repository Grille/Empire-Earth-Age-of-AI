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
            open, has_was, has_was2, in_group;   // in_group: only for an update during a step (uc_in_group)
    uint32_t walked;            // the frame's: the walk during a step that drew it last
    float was[12];              // the moving between steps: its pose at the game time before its last (has_was)
    int32_t was_anim, was_frame, was_next;   // ... and its animation there
    float was_fraction;
    float was2[12];             // ... and the one before that (has_was2)
    int32_t was2_anim, was2_frame, was2_next;
    float was2_fraction;
    float ms[3];                // the game times of its pose now, was and was2
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
    LONGLONG at, dt;      // the simulation's copy: when it was made, and the time since the one before (0: none to move from)
    float clock;          // the game time it shows
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

// STEP 4, THE MOVING BETWEEN STEPS (the pipe: picture collect smooth off | on | probe; off by default): the frame draws
// a moving object between its pose in the steps' last copy but one and its last, by the time since that copy (collect.c
// uc_smooth), a step behind the simulation. Each object's pose in the last copy is kept here by its pointer, the
// simulation's alone (a frame's own copy of the world neither reads nor writes it). An entry not met in the last copy
// is of an object no longer there, or a new one at its address: no pose to come from. probe: the poses kept and the
// picture's steps counted, nothing moved.
internal volatile int g_cp_smooth;
internal volatile int g_uc_during_on;   // the pipe's picture collect during: the units' update run during a step (collect.c)
typedef struct { float t[12]; int32_t anim, frame, next; float fraction, ms; } cp_pose_p;   // a pose and its game time
typedef struct { ee_unit *u; uint32_t gen; int n; cp_pose_p p[3]; } cp_pose_e;   // its last n poses, the newest first
internal cp_pose_e *g_cp_pose;
internal uint32_t g_cp_pose_cap, g_cp_pose_n, g_cp_pose_gen;
// a new copy's turn: the table at most half full, those of older copies than the last dropped when it is rebuilt
internal void cp_pose_turn(uint32_t want) {
    g_cp_pose_gen++;
    if (g_cp_pose_cap && want * 2 <= g_cp_pose_cap && g_cp_pose_n * 2 <= g_cp_pose_cap) return;
    uint32_t cap = g_cp_pose_cap ? g_cp_pose_cap : 16384;
    while (cap < want * 2) cap *= 2;
    cp_pose_e *o = g_cp_pose, *q = calloc(cap, sizeof *q);
    if (!q) return;
    uint32_t old = g_cp_pose_cap, n = 0;
    for (uint32_t i = 0; i < old; i++)
        if (o[i].u && o[i].gen + 1 == g_cp_pose_gen) {
            uint32_t h = ((uint32_t)(uintptr_t)o[i].u >> 3) * 2654435761u & (cap - 1);
            while (q[h].u) h = (h + 1) & (cap - 1);
            q[h] = o[i];
            n++;
        }
    free(o);
    g_cp_pose = q, g_cp_pose_cap = cap, g_cp_pose_n = n;
}
// the object's poses before its own of the game time now_ms: the two newest of the entry's older than it
internal void cp_pose_give(uc_obj *o, const cp_pose_e *e, float now_ms) {
    o->ms[0] = now_ms;
    for (int i = 0, k = 0; i < e->n && k < 2; i++) {
        const cp_pose_p *p = &e->p[i];
        if (p->ms >= now_ms) continue;
        if (!k) memcpy(o->was, p->t, sizeof p->t), o->was_anim = p->anim, o->was_frame = p->frame, o->was_next = p->next, o->was_fraction = p->fraction, o->has_was = 1;
        else memcpy(o->was2, p->t, sizeof p->t), o->was2_anim = p->anim, o->was2_frame = p->frame, o->was2_next = p->next, o->was2_fraction = p->fraction, o->has_was2 = 1;
        o->ms[++k] = p->ms;
    }
}
internal cp_pose_e *cp_pose_find(ee_unit *u) {
    uint32_t h = ((uint32_t)(uintptr_t)u >> 3) * 2654435761u & (g_cp_pose_cap - 1);
    while (g_cp_pose[h].u && g_cp_pose[h].u != u) h = (h + 1) & (g_cp_pose_cap - 1);
    return &g_cp_pose[h];
}
// the simulation's copy (game time ms): the object's pose and animation now into its entry, the poses before out. A copy
// of the same game time as the last (Art of Conquest writes the world with the step's return now and then without a step)
// replaces the newest instead of pushing the others out.
internal void cp_pose_swap(ee_unit *u, uc_obj *o, float ms) {
    if (!g_cp_pose_cap || g_cp_pose_n * 4 >= g_cp_pose_cap * 3) return;
    cp_pose_e *e = cp_pose_find(u);
    if (!e->u) e->u = u, g_cp_pose_n++;
    if (e->gen + 1 != g_cp_pose_gen) e->n = 0;   // not in the last copy: no pose to come from
    cp_pose_p now;
    memcpy(now.t, o->pm, sizeof now.t);
    now.anim = o->anim, now.frame = o->anim_frame, now.next = o->next_frame, now.fraction = o->fraction, now.ms = ms;
    if (e->n && e->p[0].ms == ms) e->p[0] = now;
    else {
        memmove(e->p + 1, e->p, 2 * sizeof *e->p);
        e->p[0] = now;
        if (e->n < 3) e->n++;
    }
    e->gen = g_cp_pose_gen;
    cp_pose_give(o, e, ms);
}
// a frame's own copy (a writer other than the steps came): the poses of the steps' last copy, read only - else such
// frames drew the units as they stood, a step and a half ahead of the others
internal void cp_pose_peek(ee_unit *u, uc_obj *o, float ms) {
    if (!g_cp_pose_cap) return;
    cp_pose_e *e = cp_pose_find(u);
    if (e->u == u && e->gen == g_cp_pose_gen) cp_pose_give(o, e, ms);
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
    if (g_cp_smooth && c->made_by == 1) cp_pose_swap(u, o, c->clock);
    else if (g_cp_smooth) cp_pose_peek(u, o, c->clock);
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
    if (g_uc_during_on && o->cowner == me) o->in_group = group_contains(player_get_ui_group(me), cu) != 0;   // a linear search: the player's own
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
    c->at = c->dt = 0;
    c->clock = physics_get_time_ms(game_physics());
    if (g_cp_smooth && made_by == 1) cp_pose_turn(c->objs_cap);
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
#define UC_SMOOTH_MAX_MS 1000   // steps further apart than this (a pause) aren't moved between
// the game time as the steps' writes leave it (the flying pieces' clock, collect.c pt_clock), under g_cp_cs: the newest,
// when it came, a step's game time (averaged), and the game time a real ms (over the steps of about the last half second,
// not across a pause). Art of Conquest writes the world with the step's return now and then without the time going on.
internal float g_cp_clock, g_cp_step_ms, g_cp_rate;
internal LONGLONG g_cp_clock_at;
internal void cp_clock_moved(LONGLONG now) {
    static double win_ms, win_rt;
    float clock = physics_get_time_ms(game_physics());
    EnterCriticalSection(&g_cp_cs);
    if (clock != g_cp_clock) {
        float step = clock - g_cp_clock;
        LONGLONG rt = now - g_cp_clock_at;
        if (g_cp_clock_at && step > 0.0f && rt < rd_qpc_ms(UC_SMOOTH_MAX_MS)) {
            g_cp_step_ms = g_cp_step_ms > 0.0f ? g_cp_step_ms * 0.9f + step * 0.1f : step;
            win_ms += step, win_rt += (double)rt;
            if (win_rt >= (double)rd_qpc_ms(500)) g_cp_rate = (float)(win_ms * (double)rd_qpc_ms(1) / win_rt), win_ms = win_rt = 0.0;
        } else win_ms = win_rt = 0.0;   // a pause, or the time gone back (another game)
        g_cp_clock = clock, g_cp_clock_at = now;
    }
    LeaveCriticalSection(&g_cp_cs);
}
internal LONGLONG g_cp_last_at;                    // when the simulation's last copy was made
internal int cp_spare(void) {   // inside g_cp_cs
    for (int i = 0; i < UC_COPIES; i++)
        if (i != g_cp_new && i != g_cp_read) return i;
    return -1;
}
// STEP 4, THE EFFECTS (the pipe: picture collect effects off | on | check; on with [display] units_smooth). The frame
// draws the effects from five queues (the game's: engine_display.effect_queues) its full update remakes; a frame during
// a step didn't, the effects stood: the update reads the units they belong to and its Queue copies each one's state
// for the draw - it crashed when run beside a step. So the simulation makes them, at a step's end inside the world's
// lock, as the game's update does (the view's rows, each effect's PrepareQueue and Queue), into queues of its own, and a
// frame during a step takes them over. References are plain counts: only ever taken and given back inside the lock -
// the queues a frame replaces are retired, for the simulation to release at its next step's end (or the game's own
// release, when a full update finds them in its place: it holds the read lock).
enum { FX_EMPTY, FX_BUILDING, FX_READY };
internal uint8_t g_fx_p[EFFECT_QUEUES_SIZE], g_fx_r[EFFECT_QUEUES_SIZE];   // the next, the retired
internal int g_fx_made;
internal volatile int g_fx_on;                 // the pipe: 0 off, 1 on, 2 check
internal int g_fx_sims;                        // the queues in use are the simulation's (collect_fx_sims)
internal volatile LONG g_fx_state, g_fx_retired;
internal ee_map *g_fx_map;                     // the map g_fx_p is of
internal float g_fx_cam[16];                   // the camera it was made for (the check's)
internal long *g_fx_lo, *g_fx_hi;              // by region row: the first and last region under the view's rows
internal uint32_t g_fx_rows;
internal LONG g_fx_builds, g_fx_takes, g_fx_queued, g_fx_chk_n, g_fx_chk_diff, g_fx_dead;
internal LONGLONG g_fx_qpc;
internal int fx_release_one(void *ctx, int q, uint32_t key, void *e) { (void)ctx, (void)q, (void)key; object_release(e); return 1; }
internal void fx_empty(uint8_t *q, int release) {
    if (release) effect_queues_each(q, fx_release_one, NULL);
    for (int i = 0; i < 5; i++) effect_queue_clear(q + i * 12);
}
// WHAT THE QUEUES IN USE HOLD. A finished effect (its Step says so: the manager gives back its two references) leaves
// its map region only when its last reference goes. The game's update releases the old queues before it makes the new
// ones, so one that only the queues held is gone by then; here the frame still draws the old ones while the new are
// made, and such an effect, asked again and referenced again, never died - a muzzle flash stood on its last frame for
// good (the player's picture, 2026-11-08). So an effect that nothing but the queues in use holds isn't queued: the
// set of those (the frame's, the retired). A reference is a call of its Queue, one per map region the effect lies in: a line
// over a region's border (an airport's line to its target) is in two and queued twice, and counted as one such a line that
// had been taken away stayed on the screen for good (the player's game, 2026-11-09: 4 references, 2 regions, 2 entries in the
// game's queues, gone from the manager). One call may fill several of the five queues, so what a set holds is the most
// entries the effect has in any one of them.
typedef struct { void *e; uint32_t gen; uint16_t n[2][5]; } fx_hold_e;
internal fx_hold_e *g_fx_hold;
internal uint32_t g_fx_hold_cap, g_fx_hold_gen;
internal fx_hold_e *fx_hold_at(const void *e) {
    uint32_t h = ((uint32_t)(uintptr_t)e >> 3) * 2654435761u & (g_fx_hold_cap - 1);
    while (g_fx_hold[h].gen == g_fx_hold_gen && g_fx_hold[h].e != e) h = (h + 1) & (g_fx_hold_cap - 1);
    return &g_fx_hold[h];
}
internal int fx_count_one(void *ctx, int q, uint32_t key, void *e) { (void)q, (void)key, (void)e; ++*(uint32_t *)ctx; return 1; }
internal int fx_hold_one(void *ctx, int q, uint32_t key, void *e) {
    (void)key;
    fx_hold_e *h = fx_hold_at(e);
    if (h->gen != g_fx_hold_gen) h->e = e, h->gen = g_fx_hold_gen, memset(h->n, 0, sizeof h->n);
    h->n[(uintptr_t)ctx - 1][q]++;
    return 1;
}
// a table of what the queues hold; 0 out of memory
internal int fx_hold_make(uint8_t *a, uint8_t *b) {
    uint32_t n = 0;
    effect_queues_each(a, fx_count_one, &n);
    if (b) effect_queues_each(b, fx_count_one, &n);
    if ((n + 8) * 2 > g_fx_hold_cap) {
        uint32_t cap = g_fx_hold_cap ? g_fx_hold_cap : 4096;
        while (cap < (n + 8) * 2) cap *= 2;
        fx_hold_e *q = calloc(cap, sizeof *q);
        if (!q) return 0;
        free(g_fx_hold);
        g_fx_hold = q, g_fx_hold_cap = cap, g_fx_hold_gen = 0;
    }
    if (++g_fx_hold_gen == 0) { memset(g_fx_hold, 0, (size_t)g_fx_hold_cap * sizeof *g_fx_hold); g_fx_hold_gen = 1; }
    effect_queues_each(a, fx_hold_one, (void *)1);
    if (b) effect_queues_each(b, fx_hold_one, (void *)2);
    return 1;
}
// whether something besides the queues in use holds the effect (the game's own queues would be gone by now)
internal int fx_alive(const void *e) {
    fx_hold_e *h = fx_hold_at(e);
    uint32_t held = 0;
    if (h->gen == g_fx_hold_gen)
        for (int s = 0; s < 2; s++) {
            uint32_t most = 0;
            for (int q = 0; q < 5; q++) if (h->n[s][q] > most) most = h->n[s][q];
            held += most;
        }
    return effect_references(e) > held;
}
// the effects the view shows into g_fx_p: the game's QueueEffects' rows and order
internal void fx_build(ee_map *map) {
    const uint8_t *tab = map_effect_regions(map);
    int size = tab ? effect_regions_size(tab) : 0;
    uint32_t h = tab ? effect_regions_height(tab) : 0;
    if (size <= 0 || !h) return;
    if (h > g_fx_rows) {
        long *a = realloc(g_fx_lo, h * sizeof *a), *b = realloc(g_fx_hi, h * sizeof *b);
        if (a) g_fx_lo = a;
        if (b) g_fx_hi = b;
        if (!a || !b) return;
        g_fx_rows = h;
    }
    for (uint32_t i = 0; i < h; i++) g_fx_lo[i] = LONG_MAX, g_fx_hi[i] = LONG_MIN;
    const void *mesh = map_mesh_at(map);
    unsigned long n = vr_rows(mesh);
    for (unsigned long r = 0; r < n; r++) {
        long y, x0, x1;
        vr_row(mesh, r, &y, &x0, &x1);
        long key = y / size;
        if (key < 0 || (uint32_t)key >= h) continue;
        if (x0 / size < g_fx_lo[key]) g_fx_lo[key] = x0 / size;
        if (x1 / size > g_fx_hi[key]) g_fx_hi[key] = x1 / size;
    }
    for (uint32_t y = 0; y < h; y++) {
        if (g_fx_lo[y] > g_fx_hi[y]) continue;
        for (long x = g_fx_lo[y]; x <= g_fx_hi[y]; x++) {
            uint32_t k;
            void **e = effect_region(tab, (uint32_t)x, y, &k);
            for (uint32_t i = 0; i < k; i++) {
                if (!effect_shown(e[i])) continue;
                if (!fx_alive(e[i])) { g_fx_dead++; continue; }
                effect_prepare(e[i]);
                if (!effect_queue(e[i], g_fx_p)) { effect_reference(e[i]); g_fx_queued++; }
            }
        }
    }
}
// at a step's end, inside the lock (collect_copy_written): the retired queues released, the next made
internal void collect_fx_step(ee_map *map) {
    if (!g_fx_on) return;
    if (!g_fx_made) {
        memset(g_fx_p, 0, sizeof g_fx_p), memset(g_fx_r, 0, sizeof g_fx_r);
        for (int i = 0; i < 5; i++) effect_queue_new(g_fx_p + i * 12), effect_queue_new(g_fx_r + i * 12);
        g_fx_made = 1;
    }
    LONGLONG t0 = rd_qpc();
    if (g_fx_retired) {
        fx_empty(g_fx_r, g_fx_map == map);   // of another map: its effects are gone with it - the references stay unpaid
        g_fx_retired = 0;
    }
    EnterCriticalSection(&g_cp_cs);
    int had = g_fx_state == FX_READY;
    g_fx_state = FX_BUILDING;
    LeaveCriticalSection(&g_cp_cs);
    if (had) fx_empty(g_fx_p, g_fx_map == map);
    g_fx_map = map;
    if (fx_hold_make(effect_queues_at(), g_fx_retired ? g_fx_r : NULL)) fx_build(map);   // (no take now: not ready)
    memcpy(g_fx_cam, camera_transformation_at(game_camera()), sizeof g_fx_cam);
    g_fx_builds++;
    g_fx_qpc += rd_qpc() - t0;
    EnterCriticalSection(&g_cp_cs);
    g_fx_state = FX_READY;
    LeaveCriticalSection(&g_cp_cs);
}
// a frame during a step: the newest queues in place of those it drew, those retired (nothing else of them is read: the
// frame's draws come after)
internal void collect_fx_take(void) {
    if (!g_fx_on || !g_fx_made) return;
    uint8_t *g = effect_queues_at();
    EnterCriticalSection(&g_cp_cs);
    if (g_fx_state == FX_READY && !g_fx_retired && g_fx_map == game_map()) {
        uint8_t t[EFFECT_QUEUES_SIZE];
        memcpy(t, g_fx_r, sizeof t);
        memcpy(g_fx_r, g, sizeof t);
        memcpy(g, g_fx_p, sizeof t);
        memcpy(g_fx_p, t, sizeof t);
        g_fx_retired = 1;
        g_fx_state = FX_EMPTY;
        g_fx_takes++;
        g_fx_sims = 1;
    }
    LeaveCriticalSection(&g_cp_cs);
}
// the queues in use are the simulation's - they hold their effects, which a step can't free: a frame during a step may
// draw them anew (render_3d.c rd_hook_ro_water). A real frame's update (collect_fx_check after it) makes the game's again.
internal int collect_fx_sims(void) { return g_fx_sims; }
// the check, after a full update made the game's queues (its read lock held: the simulation's aren't being made): the
// same effects in the same order, when no other writer came since they were made and the camera is as it was
// the game's scene released (a game's end, a load: render-thread hook of EEReleaseScene): what the simulation's queues
// hold given back while the effects are there, inside the read lock as the game's own release; the corpses collect.c keeps
// let go (their graphics' users given back while the game's graphics are there)
internal void uc_corpses_clear(void);   // collect.c
internal void __cdecl fx_hook_scene_release(void) {
    uc_corpses_clear();
    if (g_fx_made && g_rwl_world) {
        g_rwl_read(g_rwl_world);
        EnterCriticalSection(&g_cp_cs);
        if (g_fx_retired) fx_empty(g_fx_r, 1), g_fx_retired = 0;
        if (g_fx_state == FX_READY) fx_empty(g_fx_p, 1);
        g_fx_state = FX_EMPTY;
        g_fx_map = NULL;
        LeaveCriticalSection(&g_cp_cs);
        g_rwl_runlock(g_rwl_world);
    }
    scene_release();
}
typedef struct { uint32_t n, hash; } fx_sum;
internal int fx_sum_one(void *ctx, int q, uint32_t key, void *e) {
    fx_sum *s = ctx;
    s->n++;
    s->hash = (s->hash ^ (uint32_t)q ^ key * 31 ^ (uint32_t)(uintptr_t)e) * 16777619u;
    return 1;
}
internal void collect_fx_check(void) {
    g_fx_sims = 0;
    if (g_fx_on != 2 || !g_fx_made || g_fx_state != FX_READY || g_cp_stale || g_fx_map != game_map() ||
        memcmp(g_fx_cam, camera_transformation_at(game_camera()), sizeof g_fx_cam))
        return;
    fx_sum a = { 0, 2166136261u }, b = { 0, 2166136261u };
    effect_queues_each(effect_queues_at(), fx_sum_one, &a);
    effect_queues_each(g_fx_p, fx_sum_one, &b);
    g_fx_chk_n++;
    if ((a.n != b.n || a.hash != b.hash) && g_fx_chk_diff++ < 8)
        log_locked("# collect: the effects' queues differ from the game's: %u effects, the simulation's %u\n", a.n, b.n);
}

// at the end of a write to the world (render_3d.c rd_hook_write_unlock, its holder still in; ret: where it took the
// lock): the steps' copy of the frame's last view and UC_MARGIN round it, else the copy stale
internal void collect_copy_written(uintptr_t ret) {
    if (!g_cp_on || !g_cp_cs_ok) return;
    if (ret == (uintptr_t)engine_display_step_write_return_at()) cp_clock_moved(rd_qpc());
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
    LONGLONG t1 = rd_qpc();
    g_cp_step_qpc += t1 - t0;
    g_cp[i].at = t1;
    g_cp[i].dt = g_cp_last_at && t1 - g_cp_last_at < rd_qpc_ms(UC_SMOOTH_MAX_MS) ? t1 - g_cp_last_at : 0;   // none after a pause
    g_cp_last_at = t1;
    collect_fx_step(map);
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
        a.has_was = b.has_was, memcpy(a.was, b.was, sizeof a.was);   // the poses before: the simulation's copy's
        a.was_anim = b.was_anim, a.was_frame = b.was_frame, a.was_next = b.was_next, a.was_fraction = b.was_fraction;
        a.has_was2 = b.has_was2, memcpy(a.was2, b.was2, sizeof a.was2), memcpy(a.ms, b.ms, sizeof a.ms);
        a.was2_anim = b.was2_anim, a.was2_frame = b.was2_frame, a.was2_next = b.was2_next, a.was2_fraction = b.was2_fraction;
        a.walked = b.walked;   // the frame's
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
