// THE UNITS' COLLECTION, THE MOD'S OWN (the pipe: picture collect own | game; the game's by default). The units' update
// (EEROUnits::vf0 0x503B83, AoC 0x50EABE) fills the draw lists the frame draws from: after its setup (the lists
// emptied, the selection, the unit under the mouse) it walks the visible tiles inside the picker's lock - every
// object on a tile and the parts it is made of, each the view's test asks for (EEROUnits.drawn) into a record of the
// objects' list or the trees', sorted by its opacity; then on a tile out of the player's sight the buildings and trees
// it remembers there (doppelgangers) - and after it the records are finished and the music measured. collect_walk is
// that walk, the game's fills still making each record: the first step to a collection of the mod's own that reads
// a copy of the world the simulation makes, never the world (docs/renderer.md "The units' collection"). The update's
// first instruction of the walk (engine_display.units_walk, mov ecx, [picker_lock]) calls collect_thunk: with the walk
// the game's it does what the instruction did, else the walk is ours and the update goes on after the game's
// (units_walk_end). Its locals are the update's (its frame: walk_this ...). The calls the walk makes at the game's call
// sites go to what those sites call now - a hook there (walls.c's record fill, drawlist.c's search of the picker, the
// shadows' rows) is called as from the game's walk.
typedef void (__thiscall *uc_lock_fn)(void *cs);
internal uc_lock_fn g_uc_enter, g_uc_leave;
internal LONG g_uc_walks;   // walks of ours (the pipe's reply)
internal uint32_t g_uc_dopp;  // the doppelgangers our last walk filled (the lists' print)
__attribute__((used)) uintptr_t g_uc_lock_at, g_uc_end;   // the picker's lock, where the update goes on (collect_install)
__attribute__((used)) volatile int g_uc_own;              // the walk ours (the pipe)
internal transform_ctor_fn g_uc_transform_ctor;            // GETransformation's constructor (collect_install)

// STEP 2, THE FILLS OURS: THE OPACITY (EEROUnits.opacity, the AoC decompilation's GetUnitOpacity: EE 0x5044D7, AoC
// 0x50F412 with its cloak part in a function of its own, 0x63CEE2). An object's opacity into *out, and whether it is
// drawn see-through (its record into the see-through sort map): "show hidden units" (1: an obstacle or a moved unit
// hidden behind something, 2: every obstacle) at hidden_opacity; a dying unit fading as it rots; a cloakable unit by its
// own opacity, dimmed to an enemy; and a land building faded as it comes near the camera's front plane. Ported from
// the disassembly, its floats as the FPU has them: long double between the stores the game's code makes (a float
// there), in the same order - the game's precision whatever the FPU is set to on the render thread.
internal int uc_cloak(ee_unit *u, ee_unit *cu, int see, float *out, const opacity_consts *c) {
    if (unit_rotting(cu)) {
        // EE: the object's type and its complex unit's rot; AoC: the complex unit's type and its complex unit's
        int t = unit_type_get_rot_time(unit_get_type_data(build_is("aoc") ? cu : u));
        *out = t ? (float)((long double)unit_get_rot_ms(unit_get_physics(build_is("aoc") ? cu : u)) / t) : 0.0f;
        return see;
    }
    if (!unit_get_cloakable(cu) || *game_free_camera_at()) {
        *out = 1.0f;
        return see;
    }
    long double x = unit_get_opacity(cu);
    *out = (float)x;
    if (!(x >= c->one)) see = 1;
    ee_player *me = game_players()[local_player_index()], *owner = unit_get_owner(cu);
    if (me == owner || player_stance_to(owner, player_get_index(me)) == 0 || player_get_out(me)) return see;
    long double y = (x - c->cloaked) / c->range * c->cloaked;
    *out = (float)y;
    if (!(y >= c->zero)) *out = 0.0f;
    else if (y > c->cloaked) *out = c->cloaked;
    return see;
}
internal uint32_t g_uc_op_land, g_uc_op_near;   // the check's: land buildings the near fade tested, faded
internal int uc_opacity(ee_unit *u, float *out) {
    opacity_consts c = opacity_consts_now();
    ee_unit *cu = unit_get_physics(u);
    ee_unit_type *ty = unit_get_type_data(u);
    int see = 0, mode = show_hidden_mode();
    *out = 1.0f;
    if ((mode == 1 && (unit_type_get_is_obstacle(ty) || unit_moved(u)) && unit_update_hidden(u)) ||
        (mode == 2 && unit_type_get_is_obstacle(ty))) {
        see = 1;
        *out = c.hidden;
    }
    // AoC: a see-through one by its cloak too while it is cloaked
    if (cu && (!see || (build_is("aoc") && unit_get_cloakable(cu) && unit_get_cloaked(cu)))) see = uc_cloak(u, cu, see, out, &c);
    if (!unit_type_is_land_building(ty) || *game_cinematic_at() || *game_camera_fixed_at()) return see;
    void *vp = opacity_viewport();
    long double r = c.one / ((long double)viewport_far(vp) - viewport_near(vp));
    float range = (float)r, limit = (float)(c.front * r), center[3] = { 0, 0, 0 }, view[3] = { 0, 0, 0 }, scale;
    ee_model *m;
    int frame;
    if (cu) {
        m = ((ee_model **)dmg_model_anim_model_at(unit_graphics(u)))[unit_get_anim(cu)];
        scale = unit_get_height(cu);   // its physical model's scale (the AoC decompilation's GetScale)
        frame = unit_get_anim_frame(cu);
        transform_apply(unit_matrix_at(cu), (const float *)model_center_at(m), center);
    } else {   // on its tile's middle, the ground's height there
        __attribute__((aligned(8))) float t[16];
        g_uc_transform_ctor(t);
        t[3] = (float)((long double)unit_get_x(u) + c.half);
        t[7] = (float)((long double)unit_get_y(u) + c.half);
        t[11] = mesh_z(map_mesh_at(game_map()), t[3], t[7]);
        m = ((ee_model **)dmg_model_anim_model_at(unit_graphics(u)))[0];
        scale = unit_stats_get_height(unit_get_stats(u));
        frame = 0;
        transform_apply(t, (const float *)model_center_at(m), center);
    }
    transform_apply(opacity_view_inverse(), center, view);
    g_uc_op_land++;
    long double d = ((long double)view[1] - (long double)model_get_radii(m)[frame] * scale - viewport_near(vp)) * range;
    if (d >= limit) return see;
    d /= limit;
    g_uc_op_near++;
    if (!(d >= c.zero)) d = 0;
    if (!(d >= *out)) *out = (float)d;
    return 1;
}
// STEP 2: THE VIEW TEST (EEROUnits.drawn, the AoC decompilation's IsUnitDrawn: EE 0x5008B8, AoC 0x50B3B3, the same
// code but for addresses and offsets). Whether the walk lists an object: on a tile in the player's sight (or a gaia
// animal or bush with food left, or the free camera), of a scale, room left in its list, not dead, met once when on
// several tiles (the set met: the update's), and its model against the view - a complex unit's own physical model; one
// without (a remembered building), or a tree (shifted off its tile's middle by its id), a physical model of its own
// at the tile's middle turned to the camera's yaw. The view's three calls go to what their sites call now (the shadows'
// casters off the screen). A path point is never drawn (the game asks a `return false`), a flare neither, a foundation
// only for its owner. dry: the met set only looked in, not added to (the check: the game's call comes after).
internal int uc_view_at(ee_unit *u, int call, float x, float y) {
    __attribute__((aligned(8))) uint8_t pm[0x60];
    __attribute__((aligned(8))) float t[16];
    physical_model_new(pm);
    float z = mesh_z(map_mesh_at(game_map()), x, y);
    transform_orient(t, camera_yaw(), 0.0f, 0.0f);
    t[3] = x, t[7] = y, t[11] = z;
    t[12] = t[13] = t[14] = 0.0f, t[15] = 1.0f;
    physical_model_set_model(pm, ((void **)dmg_model_anim_model_at(unit_graphics(u)))[0]);
    physical_model_set_frame(pm, 0);
    float scale = unit_stats_get_height(unit_get_stats(u));
    transform_assign(pm, t);
    physical_model_set_scale(pm, scale);
    int r = units_view_call(call)(pm, opacity_viewport(), *game_camera_fixed_at()) != 0;
    physical_model_free(pm);
    return r;
}
internal int uc_drawn(ee_unit *u, ee_unit_vis *v, void *met, int dry) {
    opacity_consts c = opacity_consts_now();
    ee_unit *cu = unit_get_physics(u);
    ee_unit_type *ty = unit_get_type_data(u);
    ee_player *me = game_players()[local_player_index()];
    int food = unit_holds_food(u) && cu && unit_type_get_class_id(ty) != CLS_LAND_CITIZEN &&
               unit_type_get_class_id(ty) != CLS_NAVAL_CITIZEN && player_get_index(unit_get_owner(u)) == 0;
    if (!unit_vis_seen_by(v, me) && !food && !*game_free_camera_at()) return 0;
    float sc = unit_stats_get_height(unit_get_stats(u));
    if (!(sc < c.zero || sc > c.zero)) return 0;   // fcomp, je: 0 or not a number
    int size = unit_type_get_size(ty);
    if (unit_type_get_is_tree(ty)) {
        if (*draw_small_count_at() >= drawn_small_room()) return 0;
        int id = unit_get_serial(u), jx = (id % 3 - 1) * size, jy = (id / 3 % 3 - 1) * size;
        long double half = (long double)size * c.half;
        float x = (float)((long double)jx * tree_shift() + unit_get_x(u) + half);
        float y = (float)((long double)jy * tree_shift() + unit_get_y(u) + half);
        return uc_view_at(u, 2, x, y);
    }
    if (type_is_path_point(ty) || *draw_count_at() >= drawn_room() || (cu && unit_dead(cu))) return 0;
    if (size > 1) {
        void *found[2], *key = u;
        if (*(void **)doppelganger_set_find(met, found, (ee_doppelganger **)&key) != *(void **)((uint8_t *)met + 4)) return 0;
        if (!dry) doppelganger_set_insert(met, found, (ee_doppelganger **)&key);
    }
    if (!cu) {
        long double half = (long double)size * c.half;
        if (!uc_view_at(u, 1, (float)((long double)unit_get_x(u) + half), (float)((long double)unit_get_y(u) + half))) return 0;
        return !unit_is_path_marker(u) || unit_get_owner(u) == me;
    }
    ee_unit_type *cty = unit_get_type_data(cu);
    if (unit_type_get_class_id(cty) != CLS_GATE && !units_view_call(0)(unit_matrix_at(cu), opacity_viewport(), *game_camera_fixed_at()))
        return 0;
    if (*game_free_camera_at()) return 1;
    if (type_is_path_point(cty)) return unit_get_owner(u) == me && unit_path_point_shown(u);
    if (unit_is_foundation(cu)) return unit_get_owner(u) == me;
    return !unit_type_get_is_flare(cty);
}
// the walk's view test: the game's, the game's held against ours (ours first, its met set only looked in) or ours
// (the pipe: picture collect drawn game | check | own)
internal volatile int g_uc_drawn;
internal uint32_t g_uc_dr_n, g_uc_dr_diff, g_uc_dr_yes;
internal int uc_drawn_of(ee_ro_units *ro, ee_unit *o, ee_unit_vis *v, void *met) {
    if (g_uc_drawn == 2) return uc_drawn(o, v, met, 0);
    int mine = g_uc_drawn == 1 ? uc_drawn(o, v, met, 1) : 0;
    int r = ro_units_drawn(ro, o, v, met) != 0;
    if (g_uc_drawn == 1) {
        g_uc_dr_n++;
        g_uc_dr_yes += r;
        if (mine != r && g_uc_dr_diff++ < 8)
            log_locked("# collect: drawn differs: type %d at %d,%d: the game's %d, ours %d\n",
                       unit_type_get_id(unit_get_type_data(o)), unit_get_x(o), unit_get_y(o), r, mine);
    }
    return r;
}

// STEP 2: THE FILL (EEROUnits' FillUnit: EE 0x502622, AoC 0x50D3AD - the same code but for addresses and offsets and
// four AoC blocks). An object's draw record from the world: its type's, owner's and complex unit's values, the health
// bars (its own, then the units inside's for its owner and allies), its graphics' parts and two lists of the complex
// unit, the name under the mouse, its pose (the complex unit's transformation, frame, next frame, fraction; one
// without - a remembered building, a path marker on its target - at its tile turned to the camera), a building site
// scaled up by its hit points, the ground's height under it; then into the picker and the sort map by its graphics'
// key (one with a complex unit also into the third map). Its game calls - the vectors' resize, clear and push, the
// strings' operator=, the picker's add, the maps' insert, the ambient sounds' turn - are the game's own. info: the
// update's (the unit under the mouse +0, the camera's yaw +4, the fighting units +8, the ambient sounds' turn +0x10).
typedef struct { ee_unit *picked; float yaw, fighting; uint32_t frame; uint8_t ambient; } uc_info;
internal int uc_shows_bars(ee_unit *cu, ee_player *me) {
    return unit_count_inside(cu) && (unit_get_owner(cu) == me || player_stance_to(unit_get_owner(cu), player_get_index(me)) == 0);
}
internal void uc_bars(uint8_t *rec, ee_unit *u, ee_unit *cu, ee_player *me) {
    ee_unit_stats *st = unit_get_stats(u);
    if (uc_shows_bars(cu, me)) {
        vector_resize(draw_record_hp_bars_at(rec), (uint32_t)unit_count_inside(cu) + 1, 0);
        for (uint32_t i = 0; i < (uint32_t)unit_count_inside(cu); i++) {
            ee_unit *in = ((ee_unit **)vector_first(unit_inside_at(cu) - 4))[i];
            float v = (float)((long double)unit_get_hp(in) / unit_stats_get_hp(unit_get_stats(in)));
            ((float *)vector_first(draw_record_hp_bars_at(rec)))[i + 1] = v;
        }
    }
    ((float *)vector_first(draw_record_hp_bars_at(rec)))[0] = (float)((long double)unit_get_hp(u) / unit_stats_get_hp(st));
    int power = build_is("aoc") ? unit_stats_get_power(st) != 0 : unit_type_get_has_power(unit_get_type_data(u)) != 0;
    draw_record_set_power_bar(rec, power ? (float)((long double)unit_get_power(cu) / unit_stats_get_power(st)) : fill_consts_now().bar_full);
}
internal int uc_in_bar_ids(ee_unit *u) {
    const uint8_t *v = engine_display_bar_ids_at();
    const uint32_t *p = vector_first(v), *e = *(const uint32_t *const *)(v + 8);
    for (; p != e; p++) if (*p == (uint32_t)unit_get_serial(u)) return 1;
    return 0;
}
internal void uc_fill(ee_ro_units *ro, ee_unit *u, uint8_t *rec, void *sort_map, uc_info *info) {
    opacity_consts oc = opacity_consts_now();
    fill_consts fc = fill_consts_now();
    ee_map *map = game_map();
    const void *mesh = map_mesh_at(map);
    ee_tile *tile = (ee_tile *)unit_get_vis(u);
    ee_unit *cu = unit_get_physics(u), *second = cu;   // second: the object of the third map's insert
    ee_unit_stats *st = unit_get_stats(u);
    ee_player *me = game_players()[local_player_index()], *owner = unit_get_owner(u);
    ee_unit_type *ty = unit_get_type_data(u);
    int ux = unit_get_x(u), uy = unit_get_y(u), free_cam = *game_free_camera_at() != 0;
    float pos[3] = { 0, 0, 0 }, ground[3] = { 0, 0, 0 }, up[3] = { 0, 1, 0 }, tilt[3] = { 0, 0, 0 };
    int food = unit_holds_food(u) && cu && unit_type_get_class_id(ty) != CLS_LAND_CITIZEN &&
               unit_type_get_class_id(ty) != CLS_NAVAL_CITIZEN && player_get_index(owner) == 0;
    if (cu) { if (info->ambient) ambient_add(engine_display_ambient_sounds_at(), cu); }
    else if (info->ambient) ambient_add_object(engine_display_ambient_sounds_at(), u);
    draw_record_set_type(rec, ty);
    draw_record_set_size(rec, unit_type_get_size(ty));
    memcpy(draw_record_color_at(rec), player_color_at(owner), 12);
    draw_record_set_class_id(rec, unit_type_get_class_id(ty));
    draw_record_set_type_id(rec, unit_type_get_id(ty));
    draw_record_set_family(rec, unit_type_get_family(ty));
    draw_record_set_bar_x(rec, unit_get_hp(u) > 0 ? unit_type_get_bar_x(ty) : 0.0f);
    draw_record_set_bar_y(rec, unit_get_hp(u) > 0 ? unit_type_get_bar_y(ty) : 0.0f);
    draw_record_set_circle(rec, unit_type_get_mark_size(ty));
    draw_record_set_player(rec, player_get_index(owner));
    draw_record_set_producing(rec, (int)producing_none());
    draw_record_set_serial(rec, unit_get_serial(u));
    draw_record_set_gfx_built(rec, NULL);
    draw_record_set_tinted(rec, 0);
    draw_record_set_gated(rec, 0);
    uint8_t *label = draw_record_label_at(rec);   // the string emptied: its first character, its length and room
    **(uint8_t **)(label + 4) = 0;
    *(uint32_t *)(label + 0xC) = 0;
    *(uint32_t *)(label + 8) = 0;
    draw_record_set_goal_value(rec, 0);
    draw_record_set_fogged(rec, !free_cam && food ? !unit_vis_seen_by((ee_unit_vis *)tile, me) : 0);
    draw_record_set_gfx(rec, unit_graphics(u));
    graphics_acquire(draw_record_get_gfx(rec));
    if (cu) {
        draw_record_set_scale(rec, unit_get_height(cu));
        draw_record_set_tinted(rec, unit_get_tint_on(cu));
        if (draw_record_get_tinted(rec)) {
            memcpy(draw_record_tint_at(rec), unit_tint_at(cu), 12);
            draw_record_set_tint_alpha(rec, fc.tint_alpha);
        }
        ee_unit_type *cty = unit_get_type_data(cu);
        if (unit_type_get_class_id(cty) == CLS_GATE) {
            void *gate = gate_class_of(cty);
            if (unit_get_hp(cu) > 0 && ((!unit_get_chained(cu) && unit_get_chain(cu)) || gate_drawn_open(gate, cu))) {
                draw_record_set_gate_open(rec, unit_get_gate_open(cu));
                draw_record_set_gated(rec, 1);
            }
        }
        ee_goal *g = unit_get_goal(cu);
        if (*engine_display_music_counts_at() && unit_get_hp(cu) && g &&
            (goal_get_state(g) == 2 || goal_get_state(g) == 3 || goal_get_state(g) == 0xB))
            info->fighting = (float)((long double)info->fighting + oc.one);
        if (build_is("aoc") && unit_type_get_id(cty) == 0x2F0 && unit_get_finished(cu) &&
            group_contains(player_get_ui_group(me), cu))   // AoC's: its ground circle at its range
            ground_circle(ro, unit_matrix_at(cu), unit_stats_get_range(st), 2);
        if (!unit_is_foundation(cu)) {
            uint8_t *gc = (uint8_t *)unit_graphics(cu);
            uint32_t n = vector_size(gc + 4);
            vector_resize(draw_record_part_values_at(rec), n, 0);
            vector_resize(draw_record_part_models_at(rec), n, 0);
            for (uint32_t i = 0; i < n; i++) {
                ((float *)vector_first(draw_record_part_values_at(rec)))[i] = unit_get_part_values(cu)[i];
                uint8_t *part = *type_stats_game((uint8_t *)draw_record_get_gfx(rec) + 4, (int)i);
                ((uint32_t *)vector_first(draw_record_part_models_at(rec)))[i] = *(uint32_t *)(part + 0x4C);
            }
            uint32_t na = unit_get_list_a_n(cu);
            vector_resize(draw_record_list_a_c_at(rec), na, 0);
            vector_resize(draw_record_list_a_4_at(rec), na, 0);
            for (uint32_t i = 0; i < na; i++) {
                uint8_t *e = unit_get_list_a(cu)[i];
                ((uint32_t *)vector_first(draw_record_list_a_c_at(rec)))[i] = *(uint32_t *)(e + 0xC);
                ((uint32_t *)vector_first(draw_record_list_a_4_at(rec)))[i] = *(uint32_t *)(e + 4);
            }
            uint32_t nb = unit_get_list_b_n(cu);
            vector_resize(draw_record_list_b_c_at(rec), nb, 0);
            vector_resize(draw_record_list_b_4_at(rec), nb, 0);
            for (uint32_t i = 0; i < nb; i++) {
                uint8_t *e = unit_get_list_b(cu)[i];
                ((uint32_t *)vector_first(draw_record_list_b_c_at(rec)))[i] = *(uint32_t *)(e + 0xC);
                ((uint32_t *)vector_first(draw_record_list_b_4_at(rec)))[i] = *(uint32_t *)(e + 4);
            }
        }
        int bars = !free_cam ? group_contains(player_get_ui_group(me), cu) || player_get_ui_unit(me) == u : unit_get_selected(u) != 0;
        if (!bars) bars = uc_in_bar_ids(u);
        if (bars) {
            if (!unit_type_is_land_building(ty) || unit_get_hp(u)) {
                int mode = *(const int *)engine_display_circle_mode_at();
                if (mode == 0) draw_record_set_circle(rec, unit_type_get_mark_size(ty));
                else if (mode == 1 || mode == 2) {
                    float r = mode == 2 ? unit_stats_get_range(st) : build_is("aoc") ? line_of_sight(cu) : unit_stats_get_sight(st);
                    draw_record_set_circle(rec, r + r);
                }
                uc_bars(rec, u, cu, me);
                void *r = rec;
                vector_push(engine_display_bar_records_at(), &r);
            }
        } else if (info->picked == u) {
            wstring_assign(label, unit_stats_name_at(unit_get_stats(cu)));
            uc_bars(rec, u, cu, me);
            *(uint8_t **)engine_display_ui_record_at() = rec;
            if (owner != game_players()[0] && owner != me) {
                *(uint8_t **)engine_display_hover_record_at() = rec;
                int named = player_is_human(owner) || player_get_shows_name(owner);
                wstring_assign(engine_display_hover_name_at(), named ? (void *)player_name_at(owner) : (void *)player_civ_name_at(owner));
            }
        } else if (*(const uint32_t *)engine_display_show_labels_at())
            wstring_assign(label, unit_stats_name_at(unit_get_stats(cu)));
        if ((uint32_t)unit_get_producing(cu) != producing_none() && unit_get_produce_left(cu) > 0)
            draw_record_set_producing(rec, unit_get_producing(cu));
        if (!unit_is_foundation(cu)) {
            draw_record_set_frame(rec, unit_get_anim_frame(cu));
            draw_record_set_next_frame(rec, unit_get_next_frame(cu));
            draw_record_set_fraction(rec, unit_get_fraction(cu));
        } else {
            draw_record_set_frame(rec, 0);
            draw_record_set_next_frame(rec, 0);
            draw_record_set_fraction(rec, 0.0f);
        }
        transform_assign(rec, unit_matrix_at(cu));
        if (unit_type_get_faces_camera(cty)) {
            transform_apply_vector(rec, up, tilt);
            float pitch = (float)(arc_cos(-tilt[2]) - fc.tilt_offset);
            transform_orient(rec, info->yaw, pitch, 0.0f);
        }
        ee_ai_state *s = unit_get_ai_state(cu);
        draw_record_set_state(rec, s ? ai_state_get_id(s) : 0);
        draw_record_set_anim(rec, unit_get_anim(cu));
        for (void **p = unit_get_goals_first(cu); p != unit_get_goals_end(cu); p++) {
            if (unit_get_hp(u) <= 0) continue;
            uint8_t *h = *p;
            int kind = *(int *)(*(uint8_t **)(h + 0x10) + 0x20);
            if (kind == 5 || kind == 6 || kind == 2) {
                draw_record_set_goal_value(rec, ((uint32_t (__thiscall *)(void *))(*(void ***)h)[6])(h));
                break;
            }
        }
        if (!unit_get_finished(cu) && !unit_is_foundation(cu)) {   // a building site: scaled up by its hit points
            ee_unit_type *bt = unit_type_get_built_type(draw_record_get_type(rec));
            if (bt) draw_record_set_gfx_built(rec, unit_stats_get_gfx(player_stats_of(game_players()[draw_record_get_player(rec)], unit_type_get_id(bt))));
            if (draw_record_get_gfx_built(rec)) graphics_acquire(draw_record_get_gfx_built(rec));
            float *t = draw_record_transform_at(rec);
            float z = t[11];
            t[11] = 0.0f;
            transform_scale(rec, 1.0f, 1.0f, (float)((long double)unit_get_hp(u) / unit_stats_get_hp(st)));
            t[11] = z;
        } else if (build_is("aoc") && unit_is_foundation(cu)) {   // AoC's: two kinds' sites sunk
            float *t = draw_record_transform_at(rec);
            if (unit_type_get_id(cty) == 0x302) t[11] = (float)((long double)t[11] - fc.sink_302);
            else if (unit_type_get_id(cty) == 0x31F) t[11] = (float)((long double)t[11] - fc.sink_31f);
        }
    } else {
        draw_record_set_scale(rec, unit_stats_get_height(st));
        int bars = u == player_get_ui_unit(me) || (free_cam && unit_get_selected(u));
        if (!bars) bars = uc_in_bar_ids(u);
        if (bars) {
            draw_record_set_power_bar(rec, fc.bar_full);
            vector_clear(draw_record_hp_bars_at(rec));
            void *r = rec;
            vector_push(engine_display_bar_records_at(), &r);
        }
        long double half = (long double)draw_record_get_size(rec) * oc.half;
        pos[0] = (float)(half + unit_get_x(u));
        pos[1] = (float)((long double)draw_record_get_size(rec) * oc.half + unit_get_y(u));
        pos[2] = mesh_z(mesh, pos[0], pos[1]);
        ee_unit *target, *tcu;
        float *at;
        if (unit_is_path_marker(u) && world_unit((uint32_t)unit_get_hp(u)) && (tcu = unit_get_physics(world_unit((uint32_t)unit_get_hp(u))))) {
            second = tcu;
            target = world_unit((uint32_t)unit_get_hp(u));
            if (marker_place(unit_get_physics(target), ux, uy, &at)) {
                pos[0] = at[0], pos[1] = at[1], pos[2] = at[2];
                if (mesh_has_tile(mesh, (int)floorf(pos[0]), (int)floorf(pos[1]))) pos[2] = mesh_z(mesh, pos[0], pos[1]);
                uint32_t id = (uint32_t)unit_get_serial(tcu) & 0xFFFFFF, k[3];
                switch (id % 3) {
                case 0: k[0] = id % 6, k[1] = id + 1, k[2] = id + 2; break;
                case 1: k[0] = id + 1, k[1] = id % 6, k[2] = id + 2; break;
                default: k[0] = id + 2, k[1] = id + 1, k[2] = id % 6; break;
                }
                float *tint = (float *)draw_record_tint_at(rec);
                for (int i = 0; i < 3; i++) tint[i] = (float)((long double)k[i] * fc.marker_step);
            }
        }
        unit_type_place_drawn(ty, pos);
        if (unit_is_path_marker(u) && !(tile_get_height(tile) >= fc.marker_min_z)) pos[2] = fc.marker_min_z;
        transform_orient(rec, unit_type_get_faces_camera(ty) ? info->yaw : 0.0f, 0.0f, 0.0f);
        float *t = draw_record_transform_at(rec);
        t[3] = pos[0], t[7] = pos[1], t[11] = pos[2];
        draw_record_set_frame(rec, 0);
        draw_record_set_next_frame(rec, 0);
        draw_record_set_fraction(rec, 0.0f);
        draw_record_set_state(rec, 0);
        draw_record_set_anim(rec, 0);
    }
    float *t = draw_record_transform_at(rec);
    if (map_tile_lit(map, ux, uy) && !(t[11] < oc.zero)) draw_record_set_ground_z(rec, 0.0f);
    else {
        float p[3] = { t[3], t[7], t[11] };
        ground_point(map, p, ground);
        draw_record_set_ground_z(rec, ground[2]);
    }
    pick_list_add(engine_display_pick_list_at(), u, rec, draw_record_get_opacity(rec));
    void *found[2], *pair[2] = { (void *)(uintptr_t)graphics_anim_key(draw_record_get_gfx(rec), draw_record_get_anim(rec)), rec };
    sort_map_insert(sort_map, found, pair);
    if (second && !unit_is_foundation(second)) {
        pair[0] = (void *)(uintptr_t)dmg_model_get_shadow_key(draw_record_get_gfx(rec));
        pair[1] = rec;
        sort_map_insert(engine_display_sort_maps_at() + 0x18, found, pair);
    }
}

// the walk's fill: the game's or ours (the pipe: picture collect fill game | own) - also for walls.c's hook of its site,
// which calls collect_fill for the fill and then shears its farms
internal volatile int g_uc_fill;
internal void __thiscall collect_fill(void *c, ee_unit *u, float *rec, void *p3, void *p4) {
    if (g_uc_fill == 2) uc_fill((ee_ro_units *)c, u, (uint8_t *)rec, p3, (uc_info *)p4);
    else draw_record_fill(c, u, rec, p3, p4);
}

// the walk's opacity: the game's, the game's held against ours (each object: both, the game's kept, the differences
// counted and the first logged) or ours (the pipe: picture collect opacity game | check | own)
internal volatile int g_uc_opacity;
internal uint32_t g_uc_op_n, g_uc_op_diff;
internal int uc_opacity_of(ee_ro_units *ro, ee_unit *o, float *out) {
    if (g_uc_opacity == 2) return uc_opacity(o, out);
    int see = ro_units_opacity(ro, o, out);
    if (g_uc_opacity == 1) {
        float mine;
        int s = uc_opacity(o, &mine);
        g_uc_op_n++;
        if ((s != 0) != (see != 0) || memcmp(&mine, out, sizeof mine)) {
            if (g_uc_op_diff++ < 8)
                log_locked("# collect: opacity differs: type %d at %d,%d: the game's %d %.9g, ours %d %.9g\n",
                           unit_type_get_id(unit_get_type_data(o)), unit_get_x(o), unit_get_y(o), see != 0, *out, s, mine);
        }
    }
    return see;
}

__attribute__((used)) void __cdecl collect_walk(uint8_t *f) {
    units_walk w = units_walk_of(f);
    ee_ro_units *ro = w.ro;
    ee_player *pl = w.pl;
    ee_map *map = w.map;
    void *info = w.info, *dset = w.doppelgangers;
    void *dset_end = *(void **)((uint8_t *)dset + 4);   // the set's head: its find's answer when not met
    void *lock = *(void **)engine_display_picker_lock_at();
    ee_pick_list *picker = (ee_pick_list *)engine_display_pick_list_at();
    uint8_t *maps = engine_display_sort_maps_at();
    int free_cam = *game_free_camera_at() != 0;
    uint8_t *recs = draw_records(), *small = draw_small_records();
    uint32_t room = draw_room(), small_room = draw_small_room(), *n = draw_count_at(), *sn = draw_small_count_at();
    const void *mesh = map_mesh_at(map);
    g_uc_enter(lock);
    pick_list_reset(picker);
    uint32_t dopp = 0;
    unsigned long nrows = w.rows(mesh);
    for (unsigned long r = 0; r < nrows; r++) {
        long y, x0, x1;
        w.row(mesh, r, &y, &x0, &x1);
        for (long x = x0; x <= x1; x++) {
            uint32_t size = map_get_size(map);
            ee_tile *t = (uint32_t)x < size && (uint32_t)y < size ? map_get_tiles(map)[(uint32_t)y * size + (uint32_t)x] : NULL;
            ee_unit_vis *v = (ee_unit_vis *)t;
            if (!t || (!unit_vis_explored(v, pl) && !free_cam)) continue;
            for (void *node = tile_get_objects(t); node; node = tile_node_get_next(node)) {
                // an object made of parts (a ship and its sails ...): from its first part on
                ee_unit *o = tile_node_get_object(node);
                if (unit_get_physics(o)) {
                    o = unit_get_physics(o);
                    while (unit_get_chain(o)) o = unit_get_chain(o);
                }
                do {
                    if (uc_drawn_of(ro, o, v, w.met) && !w.listed(picker, o)) {
                        if (!unit_type_get_is_tree(unit_get_type_data(o))) {
                            if (unit_get_physics(o)) *w.complex_n += 1.0f;
                            uint8_t *rec = recs + (size_t)*n * DRAW_REC;
                            int see = uc_opacity_of(ro, o, draw_record_opacity(rec));
                            if (g_uc_fill == 2 && !w.fill_hooked) uc_fill(ro, o, rec, maps + (see ? 0xC : 0), (uc_info *)info);
                            else w.fill(ro, o, (float *)rec, maps + (see ? 0xC : 0), info);
                            ++*n;
                        } else {
                            uint8_t *rec = small + (size_t)*sn * DRAW_SMALL_REC;
                            int see = uc_opacity_of(ro, o, draw_small_record_opacity(rec));
                            ro_units_fill_object(ro, o, rec, maps + (see ? 0x30 : 0x24), info);
                            ++*sn;
                        }
                    }
                    o = unit_get_physics(o) ? unit_get_chained(unit_get_physics(o)) : NULL;
                } while (o);
            }
            if (unit_vis_seen_by(v, pl) || free_cam) continue;
            for (uint32_t i = 0; i < unit_vis_doppelganger_count(v); i++) {
                ee_doppelganger *d = tile_get_doppelgangers(t)[i];
                if (doppelganger_get_player(d) != pl) continue;
                ee_unit *u = doppelganger_get_unit(d);
                // a building under construction on the map: only its owner's
                if (unit_get_list_node(u) && unit_get_physics(u) && unit_is_foundation(unit_get_physics(u)) && unit_get_owner(u) != pl)
                    continue;
                ee_unit_type *ty = unit_get_type_data(u);
                if (unit_type_get_size(ty) > 1) {   // on several tiles: once
                    void *found[2];   // the insert's answer is a pair: the node, and whether it was new
                    if (*(void **)doppelganger_set_find(dset, found, &d) != dset_end) continue;
                    doppelganger_set_insert(dset, found, &d);
                }
                dopp++;
                if (!unit_type_get_is_tree(ty))
                    ro_units_fill_doppelganger(ro, d, recs + (size_t)*n * DRAW_REC, n,
                                               maps + (unit_type_get_is_obstacle(ty) ? 0xC : 0), room, (int)x, (int)y, info);
                else
                    ro_units_fill_doppelganger_object(ro, d, small + (size_t)*sn * DRAW_SMALL_REC, sn, maps + 0x24, small_room,
                                                      (int)x, (int)y, info);
            }
        }
    }
    g_uc_leave(lock);
    g_uc_dopp = dopp;
    g_uc_walks++;
}

// THE PICKER'S ROOM: the list the mouse picks from (EEUnitPicker at engine_display.pick_list; the walk adds every
// object it draws, IsUnitDrawn) has room for 0x600 - its constructor's; its add drops any more, so with more objects on
// the screen the rest couldn't be clicked (the 8000 save: 7198 drawn, 1536 in it; the player: "a known limitation").
// It gets room for all the draw lists hold (objects and trees, drawlist.c's limits): an array of its own, swapped in
// once inside its lock with the list emptied, before the first walk (game/api pick_list_grow).
internal int g_uc_grown;   // 1 grown, -1 not (once)
internal void picker_grow(void) {
    ee_pick_list *l = (ee_pick_list *)engine_display_pick_list_at();
    uint32_t was = pick_list_get_room(l), room = draw_room() + draw_small_room();
    void *lock = *(void **)engine_display_picker_lock_at();
    g_uc_grown = -1;
    if (room <= was || !lock || !g_uc_transform_ctor) return;
    g_uc_enter(lock);
    pick_list_reset(l);
    if (pick_list_grow(l, room, g_uc_transform_ctor)) g_uc_grown = 1;
    g_uc_leave(lock);
    log_locked("# collect: the picker's room %u -> %u%s\n", was, room, g_uc_grown > 0 ? "" : " FAILED (no memory)");
}

// the thunk's C, on the render thread at the walk's start: 1 the walk done (ours), 0 the game's to do
__attribute__((used)) int __cdecl collect_at(uint8_t *f) {
    if (!g_uc_grown) picker_grow();
    if (!g_uc_own) return 0;
    collect_walk(f);
    return 1;
}

// at the update's mov ecx, [picker_lock] (a call now; eax, ecx, edx free there): the game's walk goes on with ecx as it
// was to be; after ours the update goes on past the game's walk (the stack as the game's left it: the call's return
// dropped)
__attribute__((naked, used)) void collect_thunk(void) {
    __asm__("pushl %ebp\n\t"
            "call _collect_at\n\t"
            "addl $4, %esp\n\t"
            "testl %eax, %eax\n\t"
            "jne 1f\n\t"
            "movl _g_uc_lock_at, %ecx\n\t"
            "movl (%ecx), %ecx\n\t"
            "ret\n"
            "1:\n\t"
            "addl $4, %esp\n\t"
            "jmp *_g_uc_end");
}

internal int g_uc_installed;
internal void collect_install(void) {
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
    g_uc_enter = lle ? (uc_lock_fn)(void *)GetProcAddress(lle, "?Enter@TSCriticalSection@@QAEXXZ") : NULL;
    g_uc_leave = lle ? (uc_lock_fn)(void *)GetProcAddress(lle, "?Leave@TSCriticalSection@@QAEXXZ") : NULL;
    g_uc_transform_ctor = lle ? (transform_ctor_fn)(void *)GetProcAddress(lle, "??0GETransformation@@QAE@XZ") : NULL;
    g_uc_lock_at = (uintptr_t)engine_display_picker_lock_at();
    g_uc_end = (uintptr_t)engine_display_units_walk_end_at();
    uint8_t want[6] = { 0x8B, 0x0D };   // mov ecx, [picker_lock]
    memcpy(want + 2, &g_uc_lock_at, 4);
    g_uc_installed = g_uc_enter && g_uc_leave && patch_insn6_call((uintptr_t)engine_display_units_walk_at(), want, collect_thunk);
    if (g_log) fprintf(g_log, "# collect: the units' walk %s (the game's until the pipe's picture collect own)\n",
                       g_uc_installed ? "hooked" : "NOT hooked (exe differs)");
}

// the pipe's picture collect own | game: 0 done, -1 not installed
internal int collect_own(int on) {
    if (!g_uc_installed) return -1;
    g_uc_own = on;
    return 0;
}
// picture collect opacity game | check | own (0, 1, 2; the walk's - collect own): the counts begin again
internal int collect_opacity(int how) {
    if (!g_uc_installed) return -1;
    g_uc_op_n = g_uc_op_diff = g_uc_op_land = g_uc_op_near = 0;
    g_uc_opacity = how;
    return 0;
}
// picture collect drawn game | check | own: the view test the same way
internal int collect_drawn(int how) {
    if (!g_uc_installed) return -1;
    g_uc_dr_n = g_uc_dr_diff = g_uc_dr_yes = 0;
    g_uc_drawn = how;
    return 0;
}
// picture collect fill game | own (0, 2): the fill the walk calls (no check: the fill changes the game's lists - the
// lists' print in a paused game holds the two against each other)
internal int collect_fill_mode(int how) {
    if (!g_uc_installed) return -1;
    g_uc_fill = how ? 2 : 0;
    return 0;
}

// THE LISTS' PRINT (the pipe's drawcount hash): what the last update collected, to hold the mod's walk against the
// game's in a paused game - the counts, the picker's, the sort maps' sizes and a hash of the records' contents but
// their vectors (+0x58 .. +0xDC: allocations of their own); taken on the render thread right after the units' update
internal void collect_print(void) {
    if (!g_uc_print_ask) return;
    g_uc_print_ask = 0;
    uint32_t n = *draw_count_at(), sn = *draw_small_count_at(), h = 2166136261u, hs = 2166136261u;
    const uint8_t *recs = draw_records(), *small = draw_small_records();
    for (uint32_t i = 0; i < n; i++) {
        const uint8_t *r = recs + (size_t)i * DRAW_REC;
        for (int k = 0; k < DRAW_REC; k++)
            if (k < 0x58 || k >= 0xDC) h = (h ^ r[k]) * 16777619u;
    }
    // the trees' by their dwords too: some of them sway with the clock (the paused game's trees move)
    uint32_t col[DRAW_SMALL_REC / 4];
    for (int k = 0; k < DRAW_SMALL_REC / 4; k++) col[k] = 2166136261u;
    for (uint32_t i = 0; i < sn * DRAW_SMALL_REC; i++) {
        hs = (hs ^ small[i]) * 16777619u;
        col[i % DRAW_SMALL_REC / 4] = (col[i % DRAW_SMALL_REC / 4] ^ small[i]) * 16777619u;
    }
    char cols[DRAW_SMALL_REC / 4 * 9 + 1];
    for (int k = 0; k < DRAW_SMALL_REC / 4; k++) snprintf(cols + k * 9, 10, " %08x", col[k]);
    log_locked("# collect: the trees' records by their dwords:%s\n", cols);
    // the objects' records' vectors and label by their contents (their element counts too), the health bars' list
    uint32_t hv = 2166136261u, nv = 0;
    for (uint32_t i = 0; i < n; i++) {
        uint8_t *r = (uint8_t *)recs + (size_t)i * DRAW_REC;
        for (int k = 0; k < 7; k++) {
            const uint8_t *v = draw_record_hp_bars_at(r) + k * 0x10;
            const uint8_t *b = vector_first(v), *e = *(const uint8_t *const *)(v + 8);
            uint32_t cnt = b ? (uint32_t)(e - b) / 4 : 0;
            nv += cnt;
            hv = (hv ^ cnt) * 16777619u;
            for (const uint8_t *p = b; p && p < e; p++) hv = (hv ^ *p) * 16777619u;
        }
        const uint8_t *lb = draw_record_label_at(r);
        const uint16_t *ch = *(const uint16_t *const *)(lb + 4);
        for (uint32_t k = 0; ch && ch[k] && k < 256; k++) hv = (hv ^ ch[k]) * 16777619u;
    }
    const uint8_t *bv = engine_display_bar_records_at();
    log_locked("# collect: the records' vectors and labels: hash %08x (%u elements); health bars' list %u\n", hv, nv,
               (uint32_t)(*(const uint8_t *const *)(bv + 8) - *(const uint8_t *const *)(bv + 4)) / 4);
    const uint8_t *maps = engine_display_sort_maps_at();
    log_locked("# collect: the walk %s - objects %u (hash %08x), trees %u (hash %08x), picker %u, sort maps %u %u %u %u %u %u; doppelgangers %u (the mod's last walk)\n",
               g_uc_own ? "the mod's" : "the game's", n, h, sn, hs, pick_list_get_count((ee_pick_list *)engine_display_pick_list_at()),
               *(const uint32_t *)(maps + 8), *(const uint32_t *)(maps + 0x14), *(const uint32_t *)(maps + 0x20),
               *(const uint32_t *)(maps + 0x2C), *(const uint32_t *)(maps + 0x38), *(const uint32_t *)(maps + 0x44), g_uc_dopp);
    if (g_uc_opacity == 1)
        log_locked("# collect: opacity held against the game's: %u objects, %u differ (land buildings tested by the camera's front plane %u, faded %u)\n",
                   g_uc_op_n, g_uc_op_diff, g_uc_op_land, g_uc_op_near);
    if (g_uc_drawn == 1)
        log_locked("# collect: view test held against the game's: %u objects (%u drawn), %u differ\n", g_uc_dr_n, g_uc_dr_yes, g_uc_dr_diff);
}
