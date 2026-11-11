// THE UNITS' RECORD DRAW, THE MOD'S OWN (the pipe: picture draw game | own | check; on with [display] unit_instancing).
// Fully zoomed out the units' draw was the game's CPU work per record (the player, 2026-11-11: "lets make the drawing code
// ours too, its the bigest gain even ifits work"; docs/renderer.md "The units' draw, the mod's own"): its record draw
// (EEROUnits.draw_record, EE 0x50B4AC, AoC 0x515CC7) sets everything a record looks like on its graphics' model, which
// all records of that graphics share - the opacity, the texture and each material's frame of it, the frames and the
// fraction, size and place, every part down the hierarchy through the engine's functions, the texture's references
// counted at each set - then the model's transformations (PrepareTransforms) and the draw. Here the record's values go
// to the walk (render_instance.c in_node: an in_rec) and the model's are left as they are: the transformations made by
// the walk (in_local), the textures picked there (in_tex). What it still writes into the model the game's draw writes
// too and the walk reads from there: the player's colour into its materials (the game's own function), a part's turn
// into its own transformation (the engine's SetOrientationYPR), a gate's leaf, one type's tint. The states the game's
// draw sets are set the same (the alpha test's reference, the depth's use), and the model goes through the display DLL's
// Draw as before (its lock, its 3D mode). Left to the game's draw: what the instancing can't take (render_instance.c
// in_can), a record with what its gfx_built adds on (a few), a model the walk can't draw.
// picture draw check compares, record by record, the parts this draw would hand over (noted, not drawn) with those the
// game's draw then hands over and draws: the same model, material, texture, pose, indices, the place within float
// rounding (the turns' order of operations differs).
#define UD_NOTES 256
enum { UD_PARTS, UD_MODEL, UD_TEX, UD_POSE, UD_PLACE, UD_INDICES, UD_STATES, UD_KINDS };
internal const char *const k_ud_kind[UD_KINDS] = { "parts", "model or material", "texture", "pose", "place", "indices", "states" };
enum { UD_BUILT, UD_BENT, UD_CANT, UD_MODEL_NOT, UD_REASONS };
internal const char *const k_ud_reason[UD_REASONS] = { "something on it (gfx_built)", "bent onto the ground", "the instancing can't now", "the model can't be" };
typedef struct { ee_model *m; ee_material *mat; void *tex; in_pose p; D3DMATRIX w; const WORD *idx; DWORD ni; uint32_t st; } ud_part;
internal struct {
    int force;               // the pipe's: 0 the game's draw, 1 the mod's, 2 compared; -1: the mod's
    LONG own, game, reason[UD_REASONS], trees_own, trees_game;
    // the comparison: the parts each side noted
    int side, n[2], over;
    ud_part e[2][UD_NOTES];
    LONG recs, differ, kind[UD_KINDS], logged, walks, fell;   // (walks, fell: the game's side's, by in_hook)
    int bent;                // the record is a bent object's (its parts not compared: the game's side goes to the DLL)
    LONG bents;
    float worst;
} g_ud = { .force = -1 };
internal int ud_mode(void) { return g_ud.force >= 0 ? g_ud.force : 1; }
// the device's states a part is drawn under (all but the world matrix and the material, which the walk sets from the
// part, and the texture bound, which it binds - the part's own is compared) and its material's colour, as a hash (compared
// as the parts are)
internal uint32_t ud_states(const rd_device *d, const ee_material *mat) {
    vec3 col = material_get_colour(mat);
    const void *parts[] = { &d->view, &d->proj, &d->viewport, d->light, d->light_on, d->rs, d->tss, &col };
    const size_t sizes[] = { sizeof d->view, sizeof d->proj, sizeof d->viewport, sizeof d->light, sizeof d->light_on, sizeof d->rs, sizeof d->tss, sizeof col };
    uint32_t h = 2166136261u;
    for (int k = 0; k < 8; k++) for (size_t i = 0; i < sizes[k]; i++) h = (h ^ ((const uint8_t *)parts[k])[i]) * 16777619u;
    return h;
}
internal void ud_note(ee_model *m, ee_material *mat, void *tex, const in_pose *p, const D3DMATRIX *world, const WORD *idx, DWORD ni, const rd_device *d) {
    int s = g_ud.side;
    if (g_ud.n[s] >= UD_NOTES) { g_ud.over = 1; return; }
    ud_part *e = &g_ud.e[s][g_ud.n[s]++];
    e->m = m; e->mat = mat; e->tex = tex; e->p = *p; e->w = *world; e->idx = idx; e->ni = ni; e->st = ud_states(d, mat);
}
internal int bl_noted(const void *gfx), bl_noted_decal(const ee_model *m), bl_bend_at(ee_model *m, uint32_t f, const float *t, float *out);   // walls.c
internal int ud_fell(int why) { InterlockedIncrement(&g_ud.reason[why]); return 0; }
// GETransformation::SetOrientationYPR's turn (its 3 x 3; engine 0x472D3) about z by y or about x by p (the other angles
// 0): its formula with its cosines - the game's (UMCos, a table), those of 0 kept - without the four calls of them that
// give the same each time (the engine's call was 4.6 % of the render thread zoomed out)
internal void ud_turn(float *t, float y, float p) {
    static float c0, s0;
    if (!s0 && !c0) c0 = game_cos(0.0f), s0 = game_cos(1.5707964f);
    float c1 = y != 0.0f ? game_cos(y) : c0, s1 = y != 0.0f ? game_cos(1.5707964f - y) : s0;
    float c2 = p != 0.0f ? game_cos(p) : c0, s2 = p != 0.0f ? game_cos(1.5707964f - p) : s0, c3 = c0, s3 = s0;
    float a = -s2 * -s1, b = -s2 * c1;
    t[0] = c3 * c1 + a * s3; t[1] = c2 * -s1; t[2] = -s3 * c1 + a * c3;
    t[4] = c3 * s1 + b * s3; t[5] = c2 * c1; t[6] = -s3 * s1 + b * c3;
    t[8] = s3 * c2; t[9] = s2; t[10] = c3 * c2;
}
// the record's three lists of parts turned (the complex unit's part_values with part_models, its list_a's and list_b's
// values with their models): each model's own transformation the turn alone - about its z by the first two, its x by
// the third - with no place (the game's draw makes it in a transformation it constructed: the identity)
internal void ud_turns(void *rec) {
    uint8_t *lists[3][2] = { { draw_record_part_values_at(rec), draw_record_part_models_at(rec) },
                             { draw_record_list_a_c_at(rec), draw_record_list_a_4_at(rec) },
                             { draw_record_list_b_c_at(rec), draw_record_list_b_4_at(rec) } };
    for (int l = 0; l < 3; l++) {
        const float *a = vector_first(lists[l][0]), *end = vector_end(lists[l][0]);
        ee_model *const *mm = vector_first(lists[l][1]);
        if (!a) continue;
        for (uint32_t i = 0, n = (uint32_t)(end - a); i < n; i++) {
            float t[16] = { 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1 };
            if (l < 2) ud_turn(t, a[i], 0.0f);
            else ud_turn(t, 0.0f, a[i]);
            memcpy(model_own_at(mm[i]), t, sizeof t);
        }
    }
}
// a gate's leaf: the model's first part placed at its first frame's place times the opening (the game's draw keeps the
// part's turn over its frame, outer, as it is)
internal void ud_gate(ee_model *m, float open) {
    if (!model_get_child_count(m)) return;
    ee_model *leaf = model_get_children(m)[0];
    const float *f0 = model_get_frame_transforms(leaf), k = *(const float *)engine_display_bar_full_at();
    float *o = model_outer_at(leaf);
    o[3] = f0[3] * open * k; o[7] = f0[7] * open * k; o[11] = f0[11] * open * k;
}
// BENT OBJECTS KEPT: walls.c bends a resource's or an ambient object's model onto the ground under it at every draw
// (bl_bend: five heights of the ground a vertex, into the shared frame and back), and that frame went to the display
// DLL one draw at a time - 37 % of the records zoomed out in the test. They stand still: here an object's bent frame is
// made once by the same code (bl_bend_at into a copy) and kept by model, frame and place, and drawn instanced like the
// rest. Kept while the map and the game are the same (a load frees them, and the GPU's pool of runs forgets their memory); only while
// a game runs (the scenario editor changes the ground under them: the game's draw there), only a frame's own (not one
// blended toward the next: bl_bend bends the frame alone).
#define UB_SLOTS 8192
typedef struct { ee_model *m; uint32_t f; float t[12]; float *v; int decal; } ub_entry;   // v NULL: drawn as it is
internal struct { ub_entry e[UB_SLOTS]; int n; void *map; LONG made, full, stale, starts; } g_ub;
internal void mo_forget(void);   // render_morph.c
internal void ub_clear(void *map) {
    for (int i = 0; i < UB_SLOTS; i++) free(g_ub.e[i].v);
    if (g_ub.n) mo_forget();
    memset(g_ub.e, 0, sizeof g_ub.e);
    g_ub.n = 0;
    g_ub.map = map;
}
// an object's bent frame f under the root's transformation t (its 3 x 4): kept, made now if new; NULL when the table is full
internal const ub_entry *ub_get(ee_model *m, uint32_t f, const float *t) {
    void *map = game_map();
    if (map != g_ub.map || g_ub.starts != g_game_starts) ub_clear(map), g_ub.starts = g_game_starts;
    uint32_t key[4];
    memcpy(key, t + 3, 4); memcpy(key + 1, t + 7, 4); memcpy(key + 2, t + 11, 4); memcpy(key + 3, t, 4);
    uint32_t h = (((uint32_t)(uintptr_t)m >> 4) * 2654435761u ^ f * 40503u ^ key[0] * 2246822519u ^ key[1] * 3266489917u ^ key[2] * 668265263u ^ key[3]) & (UB_SLOTS - 1);
    for (;; h = (h + 1) & (UB_SLOTS - 1)) {
        ub_entry *e = &g_ub.e[h];
        if (!e->m) break;
        if (e->m == m && e->f == f && !memcmp(e->t, t, sizeof e->t)) return e;
    }
    if (g_ub.n >= UB_SLOTS * 3 / 4) { InterlockedIncrement(&g_ub.full); return NULL; }
    ub_entry *e = &g_ub.e[h];
    int decal = bl_noted_decal(m);
    uint32_t nv = vertices_get_count(model_get_vertices(m)[f]);
    float *v = decal >= 0 && nv ? malloc((size_t)nv * 8 * sizeof(float)) : NULL;
    if (v && !bl_bend_at(m, f, t, v)) { free(v); v = NULL; }   // (not standing on the ground: drawn as it is)
    e->m = m; e->f = f; memcpy(e->t, t, sizeof e->t); e->v = v; e->decal = decal > 0;
    g_ub.n++;
    InterlockedIncrement(&g_ub.made);
    return e;
}
// a record of the units' lists drawn by the mod: 1 drawn (or nothing to draw, as the game's draw would), 0 the game's
// draw is to draw it
internal int ud_draw(void *rec) {
    ee_dmg_model *g = draw_record_get_gfx(rec);
    if (!g) return 0;
    if (!*dmg_model_drawable_at(g) || !(draw_record_get_scale(rec) > *(const float *)engine_display_float_zero_at())) return 1;
    ee_dmg_model *gb = draw_record_get_gfx_built(rec);
    if (gb && *dmg_model_drawable_at(gb) && dmg_model_get_built_model(gb)) return ud_fell(UD_BUILT);
    int bent = bl_noted(g);
    int anim = draw_record_get_anim(rec);
    ee_model *m = graphics_anim_model(g, anim);
    float op = draw_record_get_opacity(rec);
    in_rec r = { draw_record_transform_at(rec), draw_record_get_scale(rec),
                 { (uint32_t)draw_record_get_frame(rec), (uint32_t)draw_record_get_next_frame(rec),
                   *engine_display_frame_blending_at() ? draw_record_get_fraction(rec) : 0.0f, op, NULL },
                 graphics_anim_texture(g, anim), m ? model_get_material_count(m) : 0, 0, 0 };
    if (bent && (!game_time() || r.pose.frac != 0)) return ud_fell(UD_BENT);
    if (!in_can(*game_camera_fixed_at()) || !in_init()) return ud_fell(UD_CANT);
    if (!m || !in_model_ok(m, &r.pose, 1)) return ud_fell(UD_MODEL_NOT);
    if (bent) {   // its bent frame kept (the root's transformation as the walk makes it)
        float t[16];
        in_local(m, r.outer, r.scale, &r.pose, t);
        const ub_entry *e = ub_get(m, r.pose.frame, t);
        if (!e) return ud_fell(UD_BENT);
        r.pose.va = e->v;
        r.decal = e->decal;
        g_ud.bent = 1;
        if (g_in_capture && e->v) {   // compared: the kept copy against one bent now
            uint32_t nv = vertices_get_count(model_get_vertices(m)[r.pose.frame]);
            float *now = malloc((size_t)nv * 8 * sizeof(float));
            if (now && bl_bend_at(m, r.pose.frame, t, now) && memcmp(now, e->v, (size_t)nv * 8 * sizeof(float))) InterlockedIncrement(&g_ub.stale);
            free(now);
        }
    }
    ee_rasterizer *ras = rasterizer();
    uint32_t screen = (uint32_t)screen_index();
    float ref = dmg_model_get_alpha_ref(g);
    rasterizer_set_alpha_ref(ras, ref > op ? *(const float *)engine_display_alpha_ref_low_at() : ref, screen);
    rasterizer_set_z_buffer(ras, op > *(const float *)engine_display_depth_write_opacity_at() ? 3 : 2, screen);
    if (!in_device(ras, screen)) return ud_fell(UD_CANT);
    dmg_model_set_player_colour(g, draw_record_color_at(rec), anim);
    ud_turns(rec);
    if (draw_record_get_gated(rec)) ud_gate(m, draw_record_get_gate_open(rec));
    if (draw_record_tints_model(rec) && model_get_material_count(m)) material_set_colour(model_get_materials(m)[0], draw_record_get_tint(rec));
    int mode = *(const int32_t *)engine_display_detail_mode_at();
    float bias = dmg_model_get_detail_bias(g);
    if (mode == 1) bias *= *(const float *)engine_display_float_half_at();
    else if (mode) bias = 1.0f;
    return in_draw_rec(m, &r, bias) ? 1 : ud_fell(UD_BENT);
}
internal float ud_off(const float *a, const float *b, int n) {   // the largest difference, relative above 1
    float w = 0.0f;
    for (int i = 0; i < n; i++) w = fmaxf(w, fabsf(a[i] - b[i]) / fmaxf(1.0f, fabsf(b[i])));
    return w;
}
// the comparison of a record's two sides (0 the mod's, 1 the game's); tree: a tree's record
internal void ud_compare(void *rec, int tree) {
    InterlockedIncrement(&g_ud.recs);
    int bad[UD_KINDS] = { 0 }, n = g_ud.n[0] < g_ud.n[1] ? g_ud.n[0] : g_ud.n[1], first = -1;
    float worst = 0.0f;
    bad[UD_PARTS] = g_ud.n[0] != g_ud.n[1];
    for (int i = 0; i < n; i++) {
        const ud_part *a = &g_ud.e[0][i], *b = &g_ud.e[1][i];
        int k0 = bad[UD_MODEL] + bad[UD_TEX] + bad[UD_POSE] + bad[UD_PLACE] + bad[UD_INDICES] + bad[UD_STATES];
        bad[UD_MODEL] |= a->m != b->m || a->mat != b->mat;
        bad[UD_TEX] |= a->tex != b->tex;
        bad[UD_POSE] |= a->p.frame != b->p.frame || a->p.next != b->p.next || ud_off(&a->p.frac, &b->p.frac, 2) > 1e-6f;
        bad[UD_INDICES] |= a->idx != b->idx || a->ni != b->ni;
        bad[UD_STATES] |= a->st != b->st;
        float w = ud_off(&a->w._11, &b->w._11, 16);
        worst = fmaxf(worst, w);
        bad[UD_PLACE] |= w > 1e-4f;
        if (first < 0 && bad[UD_MODEL] + bad[UD_TEX] + bad[UD_POSE] + bad[UD_PLACE] + bad[UD_INDICES] + bad[UD_STATES] > k0) first = i;
    }
    if (worst > g_ud.worst) g_ud.worst = worst;
    int any = 0;
    for (int k = 0; k < UD_KINDS; k++) if (bad[k]) { any = 1; InterlockedIncrement(&g_ud.kind[k]); }
    if (!any) return;
    InterlockedIncrement(&g_ud.differ);
    if (g_ud.logged++ >= 12 || !g_log) return;
    if (tree) log_locked("# units' draw: tree %p (anim %d) differs: parts %d / %d (mod / game)", rec, draw_small_record_get_anim(rec), g_ud.n[0], g_ud.n[1]);
    else log_locked("# units' draw: record %p (type %d, class %d, anim %d) differs: parts %d / %d (mod / game)", rec, draw_record_get_type_id(rec),
               draw_record_get_class_id(rec), draw_record_get_anim(rec), g_ud.n[0], g_ud.n[1]);
    log_locked(" (the game's walks %ld, to the DLL %ld)", (long)g_ud.walks, (long)g_ud.fell);
    for (int k = 1; k < UD_KINDS; k++) if (bad[k]) log_locked(", %s", k_ud_kind[k]);
    if (first >= 0) {
        const ud_part *a = &g_ud.e[0][first], *b = &g_ud.e[1][first];
        log_locked(" - part %d: tex %p / %p, frame %u %u %.4f a %.3f / %u %u %.4f a %.3f, at %.3f %.3f %.3f / %.3f %.3f %.3f, x axis %.4f %.4f %.4f / %.4f %.4f %.4f",
                   first, a->tex, b->tex, a->p.frame, a->p.next, (double)a->p.frac, (double)a->p.alpha, b->p.frame, b->p.next,
                   (double)b->p.frac, (double)b->p.alpha, (double)a->w._41, (double)a->w._42, (double)a->w._43, (double)b->w._41,
                   (double)b->w._42, (double)b->w._43, (double)a->w._11, (double)a->w._12, (double)a->w._13, (double)b->w._11,
                   (double)b->w._12, (double)b->w._13);
    }
    log_locked("\n");
}
// render_silhouette.c's hook of the game's record draw: 1 when it is drawn here
internal int ud_record(ee_ro_units *ro, void *rec) {
    int mode = ud_mode();
    if (!mode) return 0;
    if (mode == 1) {
        if (!ud_draw(rec)) { InterlockedIncrement(&g_ud.game); return 0; }
        InterlockedIncrement(&g_ud.own);
        return 1;
    }
    // compared: this draw's parts noted, the game's noted and drawn
    g_ud.side = 0; g_ud.n[0] = g_ud.n[1] = 0; g_ud.over = 0; g_ud.bent = 0;
    g_in_capture = 1;
    int mine = ud_draw(rec);
    g_ud.side = 1;
    g_in_capture = 2;
    LONG fell = g_in_fell;
    LONG walks = g_in_walks;
    ro_units_draw_record(ro, rec);
    g_ud.fell = g_in_fell - fell, g_ud.walks = g_in_walks - walks;
    g_in_capture = 0;
    if (mine && g_ud.bent) InterlockedIncrement(&g_ud.bents);
    else if (mine && !g_ud.over) ud_compare(rec, 0);
    return 1;
}
// A TREE'S RECORD DRAW, THE MOD'S OWN (with the records': picture draw game | own | check). The game's (draw_tree, EE
// 0x50AE59, AoC 0x515674) sets on its graphics' model the opacity, the texture of its animation (GEModel::SetTexture),
// its frame and next frame (1 and 0 for a model that sways, else 0 and 0), its sway as the fraction between them and its
// size, writes its place into the turn its caller made once for all the trees (at) and makes that the model's outer
// transformation, then PrepareTransforms and Draw with the graphics' detail bias - 2600 trees a frame in the test. Here
// they go to the walk as a record's do; the alpha test's reference and the depth's use are set as it sets them, its place
// written into the turn as the game writes it.
internal int ut_draw(float *at, void *rec) {
    ee_dmg_model *g = draw_small_record_get_gfx(rec);
    if (!g) return 0;
    float scale = draw_small_record_get_scale(rec);
    if (!(scale > *(const float *)engine_display_float_zero_at())) return 1;
    if (bl_noted(g)) return ud_fell(UD_BENT);   // (walls.c bends it in the game's draw)
    int anim = draw_small_record_get_anim(rec);
    ee_model *m = graphics_anim_model(g, anim);
    float op = draw_small_record_get_opacity(rec);
    vec3 pos = draw_small_record_get_pos(rec);
    in_rec r = { at, scale, { (uint32_t)draw_small_record_get_frame(rec), (uint32_t)draw_small_record_get_next_frame(rec),
                              draw_small_record_get_sway(rec), op, NULL },
                 graphics_anim_texture(g, anim), m ? model_get_material_count(m) : 0, 0, 0 };
    if (!in_can(*game_camera_fixed_at()) || !in_init()) return ud_fell(UD_CANT);
    if (!m || !in_model_ok(m, &r.pose, 1)) return ud_fell(UD_MODEL_NOT);
    ee_rasterizer *ras = rasterizer();
    uint32_t screen = (uint32_t)screen_index();
    float ref = dmg_model_get_alpha_ref(g);
    rasterizer_set_alpha_ref(ras, ref > op ? *(const float *)engine_display_alpha_ref_low_at() : ref, screen);
    rasterizer_set_z_buffer(ras, op > *(const float *)engine_display_depth_write_opacity_at() ? 3 : 2, screen);
    if (!in_device(ras, screen)) return ud_fell(UD_CANT);
    at[3] = pos.x, at[7] = pos.y, at[11] = pos.z;
    return in_draw_rec(m, &r, dmg_model_get_detail_bias(g)) ? 1 : ud_fell(UD_BENT);
}
// render_silhouette.c's hook of the game's tree draw: 1 when it is drawn here
internal int ut_record(ee_ro_units *ro, float *at, void *rec) {
    int mode = ud_mode();
    if (!mode) return 0;
    if (mode == 1) {
        if (!ut_draw(at, rec)) { InterlockedIncrement(&g_ud.trees_game); return 0; }
        InterlockedIncrement(&g_ud.trees_own);
        return 1;
    }
    g_ud.side = 0; g_ud.n[0] = g_ud.n[1] = 0; g_ud.over = 0; g_ud.bent = 0;
    g_in_capture = 1;
    int mine = ut_draw(at, rec);
    g_ud.side = 1;
    g_in_capture = 2;
    LONG fell = g_in_fell;
    LONG walks = g_in_walks;
    ro_units_draw_tree(ro, at, rec);
    g_ud.fell = g_in_fell - fell, g_ud.walks = g_in_walks - walks;
    g_in_capture = 0;
    if (mine && !g_ud.over) ud_compare(rec, 1);
    return 1;
}
// the pipe: picture draw [game | own | check]
internal int um_pipe(char *out, const char *arg);   // collect_maps.c
internal int pd_pipe(char *out, const char *arg);   // render_effects.c
internal int ud_pipe(char *out, const char *mode, const char *arg) {
    if (!strcmp(mode, "maps")) return um_pipe(out, arg);
    if (!strcmp(mode, "pieces")) return pd_pipe(out, arg);
    if (strcmp(mode, "draw")) return 0;
    if (!strcmp(arg, "game") || !strcmp(arg, "own") || !strcmp(arg, "check")) {
        g_ud.force = arg[0] == 'g' ? 0 : arg[0] == 'o' ? 1 : 2;
        if (g_ud.force == 2) {
            g_ud.recs = g_ud.differ = g_ud.logged = g_ud.bents = g_ub.stale = 0;
            g_ud.worst = 0.0f;
            memset(g_ud.kind, 0, sizeof g_ud.kind);
        }
        REPLY("the units' records drawn by %s\n", arg[0] == 'g' ? "the game" : arg[0] == 'o' ? "the mod" : "both, compared");
        return 1;
    }
    if (arg[0]) { REPLY("error: usage: picture draw [game | own | check]\n"); return 1; }
    REPLY("the units' records: %s; by the mod %ld, left to the game %ld; trees by the mod %ld, left to the game %ld (", ud_mode() == 0 ? "the game's draw" : ud_mode() == 1 ? "the mod's draw" : "compared",
          (long)g_ud.own, (long)g_ud.game, (long)g_ud.trees_own, (long)g_ud.trees_game);
    for (int k = 0; k < UD_REASONS; k++) REPLY("%s%s %ld", k ? ", " : "", k_ud_reason[k], (long)g_ud.reason[k]);
    REPLY("); compared %ld, differ %ld (", (long)g_ud.recs, (long)g_ud.differ);
    for (int k = 0; k < UD_KINDS; k++) REPLY("%s%s %ld", k ? ", " : "", k_ud_kind[k], (long)g_ud.kind[k]);
    REPLY("), the places' largest difference %.2g; bent objects compared %ld, their kept copies %d (made %ld, the table full %ld times), stale %ld\n",
          (double)g_ud.worst, (long)g_ud.bents, g_ub.n, (long)g_ub.made, (long)g_ub.full, (long)g_ub.stale);
    return 1;
}
