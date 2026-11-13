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
// part, and the texture bound with the mipmaps' bias the DLL's SetTexture sets by it, which the walk binds - the part's
// own is compared; the noted side binds none, its second part had the bias of the record before: 2 360 of 1.9 million
// differed over the player's save) and its material's colour, as a hash (compared as the parts are)
internal uint32_t ud_states(const rd_device *d, const ee_material *mat) {
    vec3 col = material_get_colour(mat);
    DWORD tss[2][RD_TSS];
    memcpy(tss, d->tss, sizeof tss);
    tss[0][D3DTSS_MIPMAPLODBIAS] = tss[1][D3DTSS_MIPMAPLODBIAS] = 0;
    const void *parts[] = { &d->view, &d->proj, &d->viewport, d->light, d->light_on, d->rs, tss, &col };
    const size_t sizes[] = { sizeof d->view, sizeof d->proj, sizeof d->viewport, sizeof d->light, sizeof d->light_on, sizeof d->rs, sizeof tss, sizeof col };
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
internal int bl_noted_wall(const ee_model *m);   // walls.c
internal LONG ws_cut_gen(void);                  // walls.c
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
// A BURNT WRECK (collect_remains.c us_burnt: a crashed aircraft from its hit on, tagged SIL_WRECK): drawn with the game's
// generic death texture instead of its animation's, and the models of its list a - its rotors - shrunk to nothing
// (their own transformations, which ud_turns sets for each record anew)
internal ee_texture *g_ud_death_tex;   // collect.c's, while it has it
internal void ud_rotors_off(void *rec) {
    const float *a = vector_first(draw_record_list_a_c_at(rec)), *end = vector_end(draw_record_list_a_c_at(rec));
    ee_model *const *mm = vector_first(draw_record_list_a_4_at(rec));
    if (!a || !mm) return;
    for (uint32_t i = 0, n = (uint32_t)(end - a); i < n; i++) {
        const float t[16] = { 1e-4f, 0, 0, 0, 0, 1e-4f, 0, 0, 0, 0, 1e-4f, 0, 0, 0, 0, 1 };
        memcpy(model_own_at(mm[i]), t, sizeof t);
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
// blended toward the next: bl_bend bends the frame alone). A wall's piece is bent by its end cuts (walls.c END CUTS),
// which change with its neighbours under the same transformation: its copy is made again when walls.c's count of the
// cuts' changes moved on.
#define UB_SLOTS 8192
typedef struct { ee_model *m; uint32_t f; float t[12]; float *v; int decal; LONG gen; } ub_entry;   // v NULL: drawn as it is; gen: a wall's cuts' count
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
        if (e->m != m || e->f != f || memcmp(e->t, t, sizeof e->t)) continue;
        LONG gen = ws_cut_gen();
        if (e->gen != gen && bl_noted_wall(m)) {   // (its copy in place: kept parts point at it)
            uint32_t nv = vertices_get_count(model_get_vertices(m)[f]);
            if (!e->v && nv) e->v = malloc((size_t)nv * 8 * sizeof(float));
            if (e->v && !bl_bend_at(m, f, t, e->v)) {
                ee_vertex *data = vertices_get_data(model_get_vertices(m)[f]);
                for (uint32_t k = 0; k < nv; k++) memcpy(e->v + k * 8, vertex_at(data, k), 8 * sizeof(float));
            }
            e->gen = gen;
        }
        return e;
    }
    if (g_ub.n >= UB_SLOTS * 3 / 4) { InterlockedIncrement(&g_ub.full); return NULL; }
    ub_entry *e = &g_ub.e[h];
    int decal = bl_noted_decal(m);
    uint32_t nv = vertices_get_count(model_get_vertices(m)[f]);
    float *v = decal >= 0 && nv ? malloc((size_t)nv * 8 * sizeof(float)) : NULL;
    if (v && !bl_bend_at(m, f, t, v)) { free(v); v = NULL; }   // (not standing on the ground: drawn as it is)
    LONG gen = ws_cut_gen();
    e->m = m; e->f = f; memcpy(e->t, t, sizeof e->t); e->v = v; e->decal = decal > 0; e->gen = gen;
    g_ub.n++;
    InterlockedIncrement(&g_ub.made);
    return e;
}
// a record of the units' lists drawn by the mod: 1 drawn (or nothing to draw, as the game's draw would), 0 the game's
// draw is to draw it (below the trees kept: it keeps records the same way)
internal int ud_draw(void *rec);
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
// THE TREES KEPT (the pipe: picture kept on | off | (nothing: its counts); on with the mod's tree draw). A tree stands
// still: its place, its size and the camera's turn its caller made for all the trees are the same from one frame to the
// next, and only its sway (the fraction between its two frames) moves - yet every frame took the whole walk for it, the
// display DLL's Draw, the hierarchy, its transformations made (10 800 trees a frame zoomed out over the player's
// 7-player save, 5 ms). Here a tree's parts are taken once from that walk (kt_capture: what in_part would draw - model,
// material, texture, place - of every material, drawn in this detail or not) and kept by everything they come of: its
// graphics, animation, model, frames, size and the turn with its place written in. Later frames choose the detail as the
// walk does (in_lod: its size on the screen, which the camera changes) and hand the kept parts to in_part with that
// detail's indices and the record's sway and opacity. Kept only what the walk gives alike at every sway (taken twice, at
// fractions 0 and 1: the frames' own transformations may differ, and the walk blends them), and drawn so only while
// every model is let through the view's test (render_shadows.c shr_pass: the casters off the screen collected) - else
// the walk as before. The trees come in the same order every frame (their sort map's): kept in the order first drawn,
// each knows the one drawn after it, and that guess is tried before the hash (a table of 27 MB looked up by hash cost
// more than the walk saved: a cache miss a tree).
// THE RECORDS KEPT the same way (2026-11-12, the player of the static objects: "continue with the buildings"; kr_find):
// a building's, a remembered one's, a bent object's (a mine, a bush: walls.c bends it onto the ground) - what stands still
// - kept by its graphics, animation, model, pose (frames and fraction), size and transformation, taken once by the walk
// (a single pass: its fraction is in what keeps it) with its bent copy's vertices (render_units.c ub_get: looked up
// at every draw before) and drawn so later; the states its draw sets are set as ud_draw sets them (the alpha test, the
// depth's use, its player's colour into the materials). Not kept: a record with turned parts, a gate, a tint on its
// model (its draw writes them into the shared models the walk reads), a building site, and while a wall's skirt is
// asked for (walls.c: made at its model's next draw through the display DLL).
#define KT_SLOTS 65536       // the hash's (a tree's number + 1), at most half full
#define KT_TREES 32768       // trees and records kept; full, it starts over
#define KT_ALL_PARTS 65536   // their parts
#define KT_PARTS 32          // a tree's or record's parts at most
enum { KT_NEW, KT_OK, KT_NOT };
enum { KT_SWAY, KT_MANY, KT_NONE, KT_REASONS };
internal const char *const k_kt_reason[KT_REASONS] = { "moved by its sway", "of too many parts", "none drawn" };
typedef struct { ee_model *m; ee_material *mat; void *tex; D3DMATRIX w; int root; const float *va; } kt_part;
typedef struct {
    ee_dmg_model *g; ee_model *m; int anim; uint32_t frame, next; float scale, at[12];
    float frac; int rec;   // a record's: its fraction (a tree's sway isn't kept by), 1
    int decal, bent;       // a record's bent copy: a decal to the shadows (walls.c), its vertices in a part
    LONG gen;              // a wall's: walls.c's count of its end cuts' changes when it was made
    int state;
    uint32_t part, np;   // its parts in g_kt.part
    uint32_t follow;     // the tree drawn after it last (its number + 1; 0 none)
    float t[16];         // the root's transformation (its size on the screen is asked by it)
} kt_tree;
internal struct {
    int force;   // the pipe's: 0 off, 1 on; -1: on
    uint32_t *hash;
    kt_tree *t;
    kt_part *part;
    uint32_t n, nparts, last;   // last: the tree drawn last (its number + 1)
    void *map;
    int starts;
    int pass, seen, bad;        // the tree being taken (g_kt_cap): its pass, the second pass's parts so far, why it can't be kept
    LONG drawn, walked, made, starts_over, guessed, reason[KT_REASONS];
    LONG rec_drawn, rec_made, rec_not;   // the records'
} g_kt = { .force = -1 };
internal int kt_on(void) { return g_kt.force < 0 ? 1 : g_kt.force; }
// the walk's part into the tree being taken (render_instance.c in_part, while g_kt_cap): the first pass's kept, the
// second's held against them
internal void kt_capture(ee_model *m, ee_material *mat, void *tex, const in_pose *p, const D3DMATRIX *world, const WORD *idx, DWORD ni) {
    (void)idx; (void)ni;   // (the detail's: chosen at each draw)
    kt_tree *k = g_kt_cap;
    if (p->va && !k->rec) { g_kt.bad |= 1 << KT_MANY; return; }
    if (!g_kt.pass) {
        if (k->np >= KT_PARTS || g_kt.nparts >= KT_ALL_PARTS) { g_kt.bad |= 1 << KT_MANY; return; }
        kt_part *e = &g_kt.part[g_kt.nparts++];
        k->np++;
        e->m = m; e->mat = mat; e->tex = tex; e->w = *world; e->root = m == k->m; e->va = p->va;
        if (p->va) k->bent = 1;
        return;
    }
    uint32_t i = (uint32_t)g_kt.seen++;
    const kt_part *e = &g_kt.part[k->part + i];
    if (i >= k->np || e->m != m || e->mat != mat || e->tex != tex || memcmp(&e->w, world, sizeof *world)) g_kt.bad |= 1 << KT_SWAY;
}
internal inline int kt_same(const kt_tree *k, const ee_dmg_model *g, const ee_model *m, int anim, uint32_t frame, uint32_t next, float scale, const float *at, float frac, int rec) {
    return k->g == g && k->m == m && k->anim == anim && k->frame == frame && k->next == next && k->scale == scale && !memcmp(k->at, at, sizeof k->at) &&
           k->rec == rec && !memcmp(&k->frac, &frac, sizeof frac);
}
// the tree (rec 0, frac 0) or record kept for these (made, KT_NEW, if new); NULL: no room
internal kt_tree *kt_find(ee_dmg_model *g, ee_model *m, int anim, uint32_t frame, uint32_t next, float scale, const float *at, float frac, int rec) {
    void *map = game_map();
    if (!g_kt.hash) {
        g_kt.hash = calloc(KT_SLOTS, sizeof *g_kt.hash);
        g_kt.t = malloc(KT_TREES * sizeof *g_kt.t);
        g_kt.part = malloc(KT_ALL_PARTS * sizeof *g_kt.part);
        if (!g_kt.hash || !g_kt.t || !g_kt.part) { free(g_kt.hash); free(g_kt.t); free(g_kt.part); g_kt.hash = NULL; return NULL; }
    }
    if (map != g_kt.map || g_kt.starts != g_game_starts || g_kt.n >= KT_TREES || g_kt.nparts > KT_ALL_PARTS - KT_PARTS) {
        if (map == g_kt.map && g_kt.starts == g_game_starts) InterlockedIncrement(&g_kt.starts_over);
        memset(g_kt.hash, 0, KT_SLOTS * sizeof *g_kt.hash);
        g_kt.n = g_kt.nparts = g_kt.last = 0;
        g_kt.map = map; g_kt.starts = g_game_starts;
    }
    kt_tree *k = NULL;
    if (g_kt.last && g_kt.t[g_kt.last - 1].follow) {
        kt_tree *f = &g_kt.t[g_kt.t[g_kt.last - 1].follow - 1];
        if (kt_same(f, g, m, anim, frame, next, scale, at, frac, rec)) k = f, g_kt.guessed++;
    }
    if (!k) {
        uint32_t h = ((uint32_t)(uintptr_t)g >> 4) * 2654435761u ^ (uint32_t)anim * 40503u ^ frame * 2246822519u, w;
        memcpy(&w, &scale, 4); h = (h ^ w) * 16777619u;
        memcpy(&w, &frac, 4); h = (h ^ w ^ (uint32_t)rec) * 16777619u;
        for (int i = 0; i < 12; i++) { memcpy(&w, at + i, 4); h = (h ^ w) * 16777619u; }
        for (h &= KT_SLOTS - 1;; h = (h + 1) & (KT_SLOTS - 1)) {
            if (!g_kt.hash[h]) {
                k = &g_kt.t[g_kt.n++];
                memset(k, 0, sizeof *k);
                k->g = g; k->m = m; k->anim = anim; k->frame = frame; k->next = next; k->scale = scale;
                memcpy(k->at, at, sizeof k->at);
                k->frac = frac; k->rec = rec;
                k->state = KT_NEW;
                g_kt.hash[h] = g_kt.n;
                break;
            }
            kt_tree *e = &g_kt.t[g_kt.hash[h] - 1];
            if (kt_same(e, g, m, anim, frame, next, scale, at, frac, rec)) { k = e; break; }
        }
    }
    uint32_t no = (uint32_t)(k - g_kt.t) + 1;
    if (g_kt.last) g_kt.t[g_kt.last - 1].follow = no;
    g_kt.last = no;
    return k;
}
// a new tree's parts taken from the walk at fractions 0 and 1 (the states of its draw set by the caller)
internal void kt_make(kt_tree *k, const in_rec *r0, float bias) {
    k->state = KT_NOT;
    k->part = g_kt.nparts; k->np = 0;
    g_kt.seen = g_kt.bad = 0;
    in_rec r = *r0;
    if (k->rec) {   // a record's: once, at its own fraction (its bent copy's vertices in r0)
        in_local(k->m, r.outer, r.scale, &r.pose, k->t);
        k->decal = r.decal; k->bent = 0;
        g_kt_cap = k;
        g_kt.pass = 0;
        if (!in_draw_rec(k->m, &r, bias)) g_kt.bad |= 1 << KT_NONE;
        g_kt_cap = NULL;
    } else {
        float t1[16];
        r.pose.frac = 0.0f; in_local(k->m, r.outer, r.scale, &r.pose, k->t);
        r.pose.frac = 1.0f; in_local(k->m, r.outer, r.scale, &r.pose, t1);
        if (memcmp(k->t, t1, sizeof t1)) g_kt.bad |= 1 << KT_SWAY;
        g_kt_cap = k;
        g_kt.pass = 0; r.pose.frac = 0.0f;
        if (!g_kt.bad && !in_draw_rec(k->m, &r, bias)) g_kt.bad |= 1 << KT_NONE;
        g_kt.pass = 1; r.pose.frac = 1.0f;
        if (!g_kt.bad && !in_draw_rec(k->m, &r, bias)) g_kt.bad |= 1 << KT_NONE;
        g_kt_cap = NULL;
        if ((uint32_t)g_kt.seen != k->np) g_kt.bad |= 1 << KT_SWAY;   // (the second pass saw another count of parts)
    }
    if (!k->np) g_kt.bad |= 1 << KT_NONE;
    for (int i = 0; i < KT_REASONS; i++)
        if (g_kt.bad & 1 << i) { InterlockedIncrement(&g_kt.reason[i]); g_kt.nparts = k->part; k->np = 0; return; }
    k->state = KT_OK;
    InterlockedIncrement(k->rec ? &g_kt.rec_made : &g_kt.made);
}
// a tree's record drawn from its kept parts (as ut_draw, which it falls back to): 1 drawn, 0 the game's draw is to draw it
internal int kt_draw(float *at, void *rec) {
    if (!kt_on()) return ut_draw(at, rec);
    ee_dmg_model *g = draw_small_record_get_gfx(rec);
    if (!g) return 0;
    float scale = draw_small_record_get_scale(rec);
    if (!(scale > *(const float *)engine_display_float_zero_at())) return 1;
    if (bl_noted(g) || !shr_pass() || !in_can(*game_camera_fixed_at()) || !in_init() || !g_in.mode3d || !g_in.cs_enter || !g_in.cs_leave)
        return ut_draw(at, rec);
    int anim = draw_small_record_get_anim(rec);
    ee_model *m = graphics_anim_model(g, anim);
    if (!m) return ut_draw(at, rec);
    float op = draw_small_record_get_opacity(rec);
    vec3 pos = draw_small_record_get_pos(rec);
    uint32_t frame = (uint32_t)draw_small_record_get_frame(rec), next = (uint32_t)draw_small_record_get_next_frame(rec);
    at[3] = pos.x, at[7] = pos.y, at[11] = pos.z;   // (as ut_draw writes it)
    kt_tree *k = kt_find(g, m, anim, frame, next, scale, at, 0.0f, 0);
    if (!k || k->state == KT_NOT) { InterlockedIncrement(&g_kt.walked); return ut_draw(at, rec); }
    ee_rasterizer *ras = rasterizer();
    uint32_t screen = (uint32_t)screen_index();
    float ref = dmg_model_get_alpha_ref(g), bias = dmg_model_get_detail_bias(g);
    rasterizer_set_alpha_ref(ras, ref > op ? *(const float *)engine_display_alpha_ref_low_at() : ref, screen);
    rasterizer_set_z_buffer(ras, op > *(const float *)engine_display_depth_write_opacity_at() ? 3 : 2, screen);
    rd_device *d = in_device(ras, screen);
    if (!d) return ud_fell(UD_CANT);
    in_rec r = { at, scale, { frame, next, draw_small_record_get_sway(rec), op, NULL }, graphics_anim_texture(g, anim), model_get_material_count(m), 0, 0 };
    if (k->state == KT_NEW) {
        if (!in_model_ok(m, &r.pose, 1)) k->state = KT_NOT;
        else kt_make(k, &r, bias);
        if (k->state != KT_OK) { InterlockedIncrement(&g_kt.walked); return ut_draw(at, rec); }
    }
    // as the DLL's Draw: its lock, its 3D mode (the camera's view and projection, stale after its 2D drawing or a move of
    // the camera - kept trees drawn without it took the frame before's, or the 2D one, and flickered: the player,
    // 2026-11-12, "tress flicker in and out in frames"), then the parts
    void *cs = rasterizer_get_lock(ras);
    if (cs) g_in.cs_enter(cs);
    g_in.mode3d(ras, screen);
    in_ctx c = { ras, rasterizer_get_screen_state(ras, screen), d, screen, bias };
    uint32_t lod = in_lod(&c, m, k->t, scale);
    for (uint32_t i = 0; i < k->np; i++) {
        const kt_part *e = &g_kt.part[k->part + i];
        uint32_t l = e->root ? lod : 0, tris = material_get_polygon_count(e->mat)[l];
        if (tris) in_part(&c, e->m, e->mat, e->tex, &r.pose, &e->w, polygons_get_indices(model_get_polygons(e->m)[l]) + material_get_polygon_first(e->mat)[l] * 3, tris * 3);
    }
    if (cs) g_in.cs_leave(cs);
    InterlockedIncrement(&g_kt.drawn);
    return 1;
}
// a record's entry kept (the THE RECORDS KEPT above): NULL when it isn't one to keep, or no room
internal int ws_skirts_asked(void);   // walls.c
internal kt_tree *kr_find(void *rec, ee_dmg_model *g, ee_model *m, int anim, const in_rec *r) {
    if (!kt_on() || !shr_pass() || !g_in.mode3d || !g_in.cs_enter || !g_in.cs_leave || ws_skirts_asked()) return NULL;
    ee_unit_type *ty = draw_record_get_type(rec);
    if (!(ty && unit_type_is_land_building(ty)) && !bl_noted(g)) return NULL;   // (what stands still)
    // (blended between two frames: an animation moving on, another entry each frame - the table would fill and start over)
    if (r->pose.frac != 0 || draw_record_get_gated(rec) || draw_record_tints_model(rec)) return NULL;
    uint8_t *lists[3] = { draw_record_part_values_at(rec), draw_record_list_a_c_at(rec), draw_record_list_b_c_at(rec) };
    for (int l = 0; l < 3; l++) if (vector_first(lists[l]) != vector_end(lists[l])) return NULL;
    kt_tree *k = kt_find(g, m, anim, r->pose.frame, r->pose.next, r->scale, r->outer, r->pose.frac, 1);
    if (k && k->state == KT_NOT) { InterlockedIncrement(&g_kt.rec_not); return NULL; }
    if (k && bl_noted_wall(m) && k->gen != ws_cut_gen()) { k->state = KT_NEW; k->gen = ws_cut_gen(); }   // (its cuts changed: made again)
    return k;
}
// a kept record's parts drawn (its states set by the caller, as for the walk): the display DLL's lock and 3D mode as
// kt_draw takes them, the detail by its size on the screen, its bent copy a decal to the shadows as walls.c's draw says
internal int kr_parts(kt_tree *k, const in_rec *r, float bias) {
    ee_rasterizer *ras = rasterizer();
    uint32_t screen = (uint32_t)screen_index();
    rd_device *d = in_device(ras, screen);
    if (!d) return ud_fell(UD_CANT);
    void *cs = rasterizer_get_lock(ras);
    if (cs) g_in.cs_enter(cs);
    g_in.mode3d(ras, screen);
    in_ctx c = { ras, rasterizer_get_screen_state(ras, screen), d, screen, bias };
    uint32_t lod = in_lod(&c, k->m, k->t, k->scale);
    in_pose p = r->pose;
    g_sh_decal = k->decal;
    for (uint32_t i = 0; i < k->np; i++) {
        const kt_part *e = &g_kt.part[k->part + i];
        uint32_t l = e->root ? lod : 0, tris = material_get_polygon_count(e->mat)[l];
        p.va = e->va;
        if (tris) in_part(&c, e->m, e->mat, e->tex, &p, &e->w, polygons_get_indices(model_get_polygons(e->m)[l]) + material_get_polygon_first(e->mat)[l] * 3, tris * 3);
    }
    g_sh_decal = 0;
    if (cs) g_in.cs_leave(cs);
    if (k->bent) g_ud.bent = 1;   // (the check compares no bent object's parts: the game's side goes to the DLL)
    InterlockedIncrement(&g_kt.rec_drawn);
    return 1;
}
// the states a record's draw sets before its model's (the game's draw sets them the same): the alpha test's reference by
// its opacity, the depth's use, its player's colour into the materials, its parts' turns, a gate's leaf, a tint; the
// detail's bias. 0: no device
internal int ud_set(void *rec, ee_dmg_model *g, ee_model *m, int anim, float op, float *bias) {
    ee_rasterizer *ras = rasterizer();
    uint32_t screen = (uint32_t)screen_index();
    float ref = dmg_model_get_alpha_ref(g);
    rasterizer_set_alpha_ref(ras, ref > op ? *(const float *)engine_display_alpha_ref_low_at() : ref, screen);
    rasterizer_set_z_buffer(ras, op > *(const float *)engine_display_depth_write_opacity_at() ? 3 : 2, screen);
    if (!in_device(ras, screen)) return 0;
    dmg_model_set_player_colour(g, draw_record_color_at(rec), anim);
    ud_turns(rec);
    if (g_ud_death_tex && sil_of(rec) == SIL_WRECK) ud_rotors_off(rec);
    if (draw_record_get_gated(rec)) ud_gate(m, draw_record_get_gate_open(rec));
    if (draw_record_tints_model(rec) && model_get_material_count(m)) material_set_colour(model_get_materials(m)[0], draw_record_get_tint(rec));
    int mode = *(const int32_t *)engine_display_detail_mode_at();
    *bias = dmg_model_get_detail_bias(g);
    if (mode == 1) *bias *= *(const float *)engine_display_float_half_at();
    else if (mode) *bias = 1.0f;
    return 1;
}
// A BROKEN SHIP'S HALF (collect_remains_physics.c A SHIP BREAKING IN TWO). Each half is a model of its own, made once from
// the ship's when it is first drawn broken: cut in the shader before, its hole showed the model's back faces, which most
// hulls haven't on the inside and the sails have only one side of (the player, 2026-11-13: "the enabled backfaces often
// dont trigger, they mess with the sails badly, maybe generating the two halfes and filling the holes with the death
// texture is the better choice"). The physics tags the half's record by the walk's number (the silhouettes'), with the
// break's plane in the world; its first draw walks the model taking its parts (uh_part: each material's vertices of
// the frame and its full detail's triangles, the bottom filled - render_instance.c in_capped - and its place), and
// uh_make cuts them in the root model's own space, where both halves agree whenever they are made: a piece of a part (its
// triangles joined by their places) that reaches under the hull's top along much of the ship or across half its beam,
// or runs along most of it wherever it lies, is cut - the hull (a ring of it too), its deck, a stripe along it -, any other kept whole by the half its middle lies in (a mast, a sail, a turret: never cut). A triangle across
// the plane keeps its side, the new corners on its edges blended from theirs (the place, the normal, the texture's), each
// edge's corner one for both triangles beside it and moved along the edge by a noise of its place (UH_JAG): the break's
// teeth, the same in both halves. The cut's edges are chained into outlines (an open one - a hull without a deck -
// closed by its chord, two open ones that meet joined) and filled flat in the plane, with the game's death texture
// (collect_remains.c us_death_texture), facing out of the half. From then on its parts are drawn from its own lists,
// frozen at the frame of the break (instanced: the record draw's walk gives each its place, uh_part swaps the lists), a
// part not in it left out; never drawn whole. A half's model is kept while it is drawn (UH_HALVES at a time, the one
// unseen longest given up for a new one; made again from the same plane when it comes back).
#define UD_CLIPS 1024
typedef struct { const void *rec; uint32_t gen, serial; int half; float pl[4]; float deck; } ud_clip;
internal ud_clip g_ud_clip[UD_CLIPS];
internal uint32_t ud_clip_hash(const void *rec) { return ((uint32_t)(uintptr_t)rec >> 3) * 2654435761u & (UD_CLIPS - 1); }
internal void ud_clip_tag(const void *rec, uint32_t gen, uint32_t serial, int half, const float *pl, float deck) {
    uint32_t h = ud_clip_hash(rec);
    for (int n = 0; n < 16; n++, h = (h + 1) & (UD_CLIPS - 1))
        if (g_ud_clip[h].rec == rec || g_ud_clip[h].gen != gen) {
            g_ud_clip[h].rec = rec, g_ud_clip[h].gen = gen, g_ud_clip[h].serial = serial, g_ud_clip[h].half = half, g_ud_clip[h].deck = deck;
            memcpy(g_ud_clip[h].pl, pl, sizeof g_ud_clip[h].pl);
            return;
        }
}
internal const ud_clip *ud_clip_of(const void *rec) {
    uint32_t h = ud_clip_hash(rec);
    for (int n = 0; n < 16; n++, h = (h + 1) & (UD_CLIPS - 1)) {
        if (g_ud_clip[h].gen != g_sil_gen) return NULL;
        if (g_ud_clip[h].rec == rec) return &g_ud_clip[h];
    }
    return NULL;
}
// whether a half can be drawn at all: the records drawn by the mod, instanced, their frames on the GPU (else the game's
// draw has them, and drew each half whole) - collect_remains_physics.c breaks no ship otherwise
internal int ud_cuts(void) { return g_sil_hooked && ud_mode() == 1 && in_on() && g_mo.on && !g_mo_off; }
#define UH_HALVES 48
#define UH_PARTS 48        // a half's parts at most (more: left out)
#define UH_SEGS 8192       // the cut's edges
#define UH_LOOP 1024       // an outline's corners at most
#define UH_JAG 0.03f       // the break's teeth: a corner moved along the ship by up to this share of its length, either way
#define UH_KEY 2e-4f       // places closer than this (the model's units) are one
#define UH_JOIN 0.04f      // two open outlines whose ends are closer than this share of the length are one
#define UH_LONG 0.4f       // a piece cut runs along this share of the ship's length at least (an oar, a mast, a sail: shorter)
#define UH_LONG_ANY 0.6f   // ... or this share wherever it lies: over the hull's top too (the player, 2026-11-13, of an Atomic
                           // transport's stripe of its player's colour along its deck, whole on one half: "the colored strip
                           // along the cargo ship isnt broken up")
#define UH_WIDE 0.5f       // ... or under the hull's top across this share of its beam: a hull of rings, the Nautilus's (cut in
                           // one 0.04 of its length long, its halves parted at a seam, unfilled)
#define UH_CAP_SHADE 0.8f  // the filling's colour over the death texture
#define UH_CAP_UV 2.0f     // ... and its texture's repeats over the ship's length
enum { UH_NEW, UH_OK, UH_NOT };
typedef struct { ee_model *m; ee_material *mat; float *v; WORD *idx; uint32_t nv, ni, cap_at; } uh_piece;
typedef struct { ee_model *m; ee_material *mat; D3DMATRIX w; const float *v; const WORD *idx; uint32_t n, ni; } uh_taken;
typedef struct {
    uint32_t serial; int half;    // half 0: a free slot
    ee_model *root; ee_material *root_mat;
    uint32_t seen; LONG starts;   // the walk it was drawn in last; the game's
    int state, cap_drawn;
    uh_piece piece[UH_PARTS]; int np;
    float *cv; WORD *ci; uint32_t cnv, cni;   // the filling, in the root model's space
} uh_half;
internal struct {
    uh_half h[UH_HALVES];
    uh_half *cur; int take;                 // the half drawn; its parts being taken
    uh_taken t[UH_PARTS]; int nt;
    float seg[UH_SEGS][6]; int nseg;        // the cut's edges (root space), the way the triangles kept run along them
    LONG made, failed, caps, parts_cut, too_big;
} g_uh;
internal void uh_free(uh_half *h) {
    for (int i = 0; i < h->np; i++) free(h->piece[i].v), free(h->piece[i].idx);
    free(h->cv), free(h->ci);
    memset(h, 0, sizeof *h);
}
// the half's model (made when new: UH_NEW); NULL when every slot is drawn this walk
internal uh_half *uh_find(uint32_t serial, int half, ee_model *root) {
    uh_half *spare = NULL, *old = NULL;
    for (int i = 0; i < UH_HALVES; i++) {
        uh_half *h = &g_uh.h[i];
        if (h->half && h->starts != g_game_starts) uh_free(h);
        if (h->half && h->serial == serial && h->half == half && h->root == root) { h->seen = g_sil_gen; return h; }
        if (!h->half) { if (!spare) spare = h; }
        else if (h->seen != g_sil_gen && (!old || g_sil_gen - h->seen > g_sil_gen - old->seen)) old = h;
    }
    uh_half *h = spare ? spare : old;
    if (!h) return NULL;
    uh_free(h);
    h->serial = serial, h->half = half, h->root = root, h->seen = g_sil_gen, h->starts = g_game_starts, h->state = UH_NEW;
    return h;
}
// a part of the walk (render_instance.c in_part while g_uh_on): taken, or drawn from the half's own lists; 1 always
// while a half is drawn (a part it hasn't is left out)
internal int uh_part(const in_ctx *c, ee_model *m, ee_material *mat, void *tex, const in_pose *p, const D3DMATRIX *world) {
    uh_half *h = g_uh.cur;
    if (!h) return 0;
    if (g_uh.take) {
        uint32_t tris = material_get_polygon_count(mat)[0];
        if (!tris || g_uh.nt >= UH_PARTS) return 1;
        uh_taken *e = &g_uh.t[g_uh.nt++];
        ee_vertices **fr = model_get_vertices(m);
        e->m = m, e->mat = mat, e->w = *world;
        e->n = material_get_vertex_count(mat);
        e->v = (p->va ? p->va : (const float *)vertices_get_data(fr[p->frame])) + material_get_vertex_first(mat) * 8;
        e->idx = polygons_get_indices(model_get_polygons(m)[0]) + material_get_polygon_first(mat)[0] * 3;
        e->ni = tris * 3;
        return 1;
    }
    for (int i = 0; i < h->np; i++) {
        const uh_piece *e = &h->piece[i];
        if (e->m != m || e->mat != mat) continue;
        if (e->ni) in_draw(c, tex, material_get_colour(mat), p->alpha * material_get_alpha(mat), world, e->v, e->v, 0.0f, e->nv, e->idx, e->ni, e->cap_at);
        if (m == h->root && mat == h->root_mat && h->cni && !h->cap_drawn) {
            vec3 col = { UH_CAP_SHADE, UH_CAP_SHADE, UH_CAP_SHADE };
            h->cap_drawn = 1;
            in_draw(c, g_ud_death_tex ? (void *)g_ud_death_tex : tex, col, p->alpha, world, h->cv, h->cv, 0.0f, h->cnv, h->ci, h->cni, h->cni);
        }
        break;
    }
    return 1;
}
// the cut's frame in the root model's space: F(q) = n . q + d, the bow's side >= 0; up and across in the plane; the
// length along n, the beam
typedef struct { float n[3], d, up[3], across[3], lo, len, beam, deck; int bow; } uh_frame;
internal uint32_t uh_cell(float x) { return (uint32_t)(int32_t)lrintf(x / UH_KEY); }
internal uint32_t uh_hash3(const float *q) {
    return (uh_cell(q[0]) * 73856093u) ^ (uh_cell(q[1]) * 19349663u) ^ (uh_cell(q[2]) * 83492791u);
}
internal int uh_same(const float *a, const float *b) { return uh_cell(a[0]) == uh_cell(b[0]) && uh_cell(a[1]) == uh_cell(b[1]) && uh_cell(a[2]) == uh_cell(b[2]); }
internal int uh_up(int *up, int k) { while (up[k] != k) k = up[k] = up[up[k]]; return k; }
// a point of the world into the root's space (w: the root's world, inv its 3 x 3's inverse)
internal void uh_to_root(const float *w, const float *inv, const float *x, float *q) {
    float y[3] = { x[0] - w[12], x[1] - w[13], x[2] - w[14] };
    for (int c = 0; c < 3; c++) q[c] = y[0] * inv[c] + y[1] * inv[3 + c] + y[2] * inv[6 + c];
}
// a corner on the edge lo - hi (lo < hi, F of each fl, fh): its place along the edge moved by the break's teeth -
// the noise of the place where the plane crosses it, so both halves and every part on the edge agree
internal float uh_along(const uh_frame *f, const float *ql, const float *qh, float fl, float fh) {
    float t0 = -fl / (fh - fl), q[3];
    for (int c = 0; c < 3; c++) q[c] = ql[c] + t0 * (qh[c] - ql[c]);
    float u = q[0] * f->across[0] + q[1] * f->across[1] + q[2] * f->across[2], w = q[0] * f->up[0] + q[1] * f->up[1] + q[2] * f->up[2];
    uint32_t h = (uint32_t)(int32_t)lrintf(u / (0.01f * f->len)) * 2654435761u ^ (uint32_t)(int32_t)lrintf(w / (0.01f * f->len)) * 2246822519u;
    h ^= h >> 15, h *= 2246822519u, h ^= h >> 13;
    float jag = UH_JAG * f->len * ((float)(h & 0xFFFF) / 32767.5f - 1.0f), t = (jag - fl) / (fh - fl);
    return t < 0.05f ? 0.05f : t > 0.95f ? 0.95f : t;
}
// one part cut into its piece (its vertices' places in the root's space q, their F); 0: it can't be (too many)
internal int uh_cut(const uh_frame *f, const uh_taken *e, const float *q, const float *F, uh_piece *out) {
    DWORD cn = e->ni;
    RD_LOCK();
    const WORD *L = in_capped(e->idx, e->ni, e->v, e->n, &cn);   // (the bottom filled: its triangles from e->ni on)
    RD_UNLOCK();
    uint32_t n = e->n, hn = 1;
    while (hn < 2 * n) hn <<= 1;
    int *up = malloc(n * sizeof *up), *cell = malloc(hn * sizeof *cell), *mode = malloc(n * sizeof *mode);
    float *st = malloc(n * 6 * sizeof *st);   // a piece's: lowest, across lo / hi, F lo / hi / sum
    uint32_t *cnt = calloc(n, sizeof *cnt), vmax = n + cn;
    if (vmax > MO_MAX_VERTS) vmax = MO_MAX_VERTS;
    float *v = malloc((size_t)vmax * 8 * sizeof *v);
    WORD *norm = malloc(cn * 2 * sizeof *norm), *bot = malloc(cn * 2 * sizeof *bot);
    uint32_t eh = 1;
    while (eh < 2 * cn) eh <<= 1;
    uint32_t *ekey = malloc(eh * sizeof *ekey);   // an edge's corner: (lo << 16 | hi) + 1, its vertex beside
    WORD *ev = malloc(eh * sizeof *ev);
    int ok = up && cell && mode && st && cnt && v && norm && bot && ekey && ev;
    uint32_t nv = n, nn = 0, nb = 0;
    if (ok) {
        memcpy(v, e->v, (size_t)n * 8 * sizeof *v);
        memset(ekey, 0, eh * sizeof *ekey);
        for (uint32_t k = 0; k < hn; k++) cell[k] = -1;
        for (uint32_t k = 0; k < n; k++) {   // the pieces: one place, one piece; then each triangle's corners one
            up[k] = (int)k;
            for (uint32_t h = uh_hash3(q + k * 3) & (hn - 1);; h = (h + 1) & (hn - 1)) {
                if (cell[h] < 0) { cell[h] = (int)k; break; }
                if (uh_same(q + k * 3, q + cell[h] * 3)) { up[uh_up(up, (int)k)] = uh_up(up, cell[h]); break; }
            }
        }
        for (DWORD t = 0; t + 2 < cn; t += 3) {
            if (L[t] >= n || L[t + 1] >= n || L[t + 2] >= n) continue;
            int a = uh_up(up, L[t]);
            up[uh_up(up, L[t + 1])] = a, up[uh_up(up, L[t + 2])] = a;
        }
        for (uint32_t k = 0; k < n; k++) {
            int r = uh_up(up, (int)k);
            float *s = st + r * 6, a = q[k * 3] * f->across[0] + q[k * 3 + 1] * f->across[1] + q[k * 3 + 2] * f->across[2];
            float z = q[k * 3] * f->up[0] + q[k * 3 + 1] * f->up[1] + q[k * 3 + 2] * f->up[2];
            if (!cnt[r]++) s[0] = z, s[1] = s[2] = a, s[3] = s[4] = F[k], s[5] = 0.0f;
            s[0] = fminf(s[0], z), s[1] = fminf(s[1], a), s[2] = fmaxf(s[2], a), s[3] = fminf(s[3], F[k]), s[4] = fmaxf(s[4], F[k]), s[5] += F[k];
        }
        for (uint32_t k = 0; k < n; k++) {   // 1 kept, 0 left out, 2 cut
            if (!cnt[k]) continue;
            const float *s = st + k * 6;
            float along = s[4] - s[3];
            int across = s[3] < 0.0f && s[4] >= 0.0f &&
                         (along >= UH_LONG_ANY * f->len || (s[0] < f->deck && (along >= UH_LONG * f->len || s[2] - s[1] >= UH_WIDE * f->beam)));
            mode[k] = across ? 2 : (s[5] / (float)cnt[k] >= 0.0f) == f->bow;
        }
    }
    for (DWORD t = 0; ok && t + 2 < cn; t += 3) {
        WORD c3[3] = { L[t], L[t + 1], L[t + 2] };
        if (c3[0] >= n || c3[1] >= n || c3[2] >= n) continue;
        WORD *o = t < e->ni ? norm : bot;
        uint32_t *no = t < e->ni ? &nn : &nb;
        int md = mode[uh_up(up, c3[0])];
        if (md == 1) { o[(*no)++] = c3[0], o[(*no)++] = c3[1], o[(*no)++] = c3[2]; continue; }
        if (!md) continue;
        WORD poly[4];
        int np = 0, out_at = -1;
        for (int i = 0; i < 3; i++) {
            WORD a = c3[i], b = c3[(i + 1) % 3];
            int ia = (F[a] >= 0.0f) == f->bow, ib = (F[b] >= 0.0f) == f->bow;
            if (ia) poly[np++] = a;
            if (ia == ib) continue;
            WORD lo = a < b ? a : b, hi = a < b ? b : a;
            uint32_t key = ((uint32_t)lo << 16 | hi) + 1, s = (key * 2654435761u) & (eh - 1);
            while (ekey[s] && ekey[s] != key) s = (s + 1) & (eh - 1);
            if (!ekey[s]) {
                if (nv >= vmax) { ok = 0; InterlockedIncrement(&g_uh.too_big); break; }
                float u = uh_along(f, q + lo * 3, q + hi * 3, F[lo], F[hi]), *x = v + nv * 8;
                const float *pl = e->v + lo * 8, *ph = e->v + hi * 8;
                for (int j = 0; j < 8; j++) x[j] = pl[j] + u * (ph[j] - pl[j]);
                float nl = sqrtf(x[3] * x[3] + x[4] * x[4] + x[5] * x[5]);
                if (nl > 1e-6f) x[3] /= nl, x[4] /= nl, x[5] /= nl;
                ekey[s] = key, ev[s] = (WORD)nv++;
            }
            if (ia) out_at = np;   // (leaving: the cut's edge runs from this corner to the next)
            poly[np++] = ev[s];
        }
        if (!ok || np < 3) continue;
        if (out_at >= 0 && g_uh.nseg < UH_SEGS) {   // the cut's edge, in the part's own space (uh_make turns it into the root's)
            WORD ca = poly[out_at], cb = poly[(out_at + 1) % np];
            float *sg = g_uh.seg[g_uh.nseg++];
            memcpy(sg, v + ca * 8, 3 * sizeof(float));
            memcpy(sg + 3, v + cb * 8, 3 * sizeof(float));
        }
        int s0 = 0;
        while (s0 < np && poly[s0] >= n) s0++;   // (a filled bottom's triangle takes its first corner's texel: an old one)
        if (s0 == np) s0 = 0;
        for (int k2 = 1; k2 + 1 < np; k2++)
            o[(*no)++] = poly[s0], o[(*no)++] = poly[(s0 + k2) % np], o[(*no)++] = poly[(s0 + k2 + 1) % np];
    }
    if (ok) {
        out->m = e->m, out->mat = e->mat, out->nv = nv, out->ni = nn + nb, out->cap_at = nn;
        out->v = v, out->idx = malloc((nn + nb + 1) * sizeof *out->idx);
        if (out->idx) { memcpy(out->idx, norm, nn * sizeof *norm); memcpy(out->idx + nn, bot, nb * sizeof *bot); v = NULL; }
        else ok = 0;
    }
    free(up), free(cell), free(mode), free(st), free(cnt), free(v), free(norm), free(bot), free(ekey), free(ev);
    return ok;
}
// the ears of an outline in the plane (p: its corners' u, w): triangles of its corner numbers into out; their count
internal int uh_ears(const float (*p)[2], int n, int *loop, int *out, int max) {
    float area = 0.0f;
    int made = 0;
    for (int i = 0; i < n; i++) area += p[loop[i]][0] * p[loop[(i + 1) % n]][1] - p[loop[(i + 1) % n]][0] * p[loop[i]][1];
    if (fabsf(area) < 1e-9f) return 0;
    while (n >= 3 && made + 3 <= max) {
        int at = -1;
        for (int i = 0; i < n && at < 0; i++) {
            const float *a = p[loop[(i + n - 1) % n]], *b = p[loop[i]], *c = p[loop[(i + 1) % n]];
            float turn = (b[0] - a[0]) * (c[1] - b[1]) - (b[1] - a[1]) * (c[0] - b[0]);
            if (turn * area <= 0.0f && n > 3) continue;
            int inside = 0;
            for (int j = 0; j < n && !inside; j++) {
                if (j == i || j == (i + 1) % n || j == (i + n - 1) % n) continue;
                const float *x = p[loop[j]];
                float d1 = (b[0] - a[0]) * (x[1] - a[1]) - (b[1] - a[1]) * (x[0] - a[0]);
                float d2 = (c[0] - b[0]) * (x[1] - b[1]) - (c[1] - b[1]) * (x[0] - b[0]);
                float d3 = (a[0] - c[0]) * (x[1] - c[1]) - (a[1] - c[1]) * (x[0] - c[0]);
                inside = (d1 > 0 && d2 > 0 && d3 > 0) || (d1 < 0 && d2 < 0 && d3 < 0);
            }
            if (!inside) at = i;
        }
        if (at < 0) break;   // (an outline that crosses itself: what is filled stays)
        out[made++] = loop[(at + n - 1) % n], out[made++] = loop[at], out[made++] = loop[(at + 1) % n];
        for (int i = at; i + 1 < n; i++) loop[i] = loop[i + 1];
        n--;
    }
    return made;
}
// the filling of the cut's edges (g_uh.seg, root space): outlines chained by their corners' places, filled facing out
// of the half (way: the side the model's triangles face, +1 or -1, by their normals)
internal void uh_fill(uh_half *h, const uh_frame *f, float way) {
    int np = 0, nseg = g_uh.nseg;
    float (*pt)[3] = malloc((size_t)nseg * 2 * sizeof *pt), (*p2)[2] = malloc((size_t)nseg * 2 * sizeof *p2);
    int *next = malloc((size_t)nseg * 2 * sizeof *next), *has_prev = calloc((size_t)nseg * 2, sizeof *has_prev);
    int *done = calloc((size_t)nseg * 2, sizeof *done), *loop = malloc(UH_LOOP * sizeof *loop), *tri = malloc(3 * UH_LOOP * sizeof *tri);
    int *chain = malloc((size_t)nseg * 2 * sizeof *chain), *start = malloc(((size_t)nseg * 2 + 1) * sizeof *start), *open = malloc((size_t)nseg * 2 * sizeof *open);
    float *cv = malloc((size_t)nseg * 2 * 8 * sizeof *cv);
    WORD *ci = malloc((size_t)nseg * 6 * sizeof *ci);
    uint32_t cni = 0;
    if (!pt || !p2 || !next || !has_prev || !done || !loop || !tri || !chain || !start || !open || !cv || !ci || !nseg) goto end;
    for (int s = 0; s < nseg; s++) {   // the corners: one place, one corner (few: a plain search)
        int id[2];
        for (int k = 0; k < 2; k++) {
            const float *x = g_uh.seg[s] + k * 3;
            int j = 0;
            while (j < np && !uh_same(pt[j], x)) j++;
            if (j == np) memcpy(pt[np], x, sizeof pt[np]), next[np] = -1, np++;
            id[k] = j;
        }
        if (id[0] == id[1]) continue;
        if (next[id[0]] < 0) next[id[0]] = id[1], has_prev[id[1]] = 1;
    }
    int nc = 0, at = 0;   // the chains: the open ones first (from a corner nothing leads to), then the closed
    for (int pass = 0; pass < 2; pass++)
        for (int s = 0; s < np; s++) {
            if (done[s] || next[s] < 0 || (!pass && has_prev[s])) continue;
            start[nc] = at, open[nc] = !pass;
            for (int p = s; p >= 0 && !done[p]; p = next[p]) done[p] = 1, chain[at++] = p;
            nc++;
        }
    start[nc] = at;
    for (int i = 0; i < np; i++) {
        p2[i][0] = pt[i][0] * f->across[0] + pt[i][1] * f->across[1] + pt[i][2] * f->across[2];
        p2[i][1] = pt[i][0] * f->up[0] + pt[i][1] * f->up[1] + pt[i][2] * f->up[2];
    }
    float out[3] = { f->bow ? -f->n[0] : f->n[0], f->bow ? -f->n[1] : f->n[1], f->bow ? -f->n[2] : f->n[2] };
    int *used = done;   // (again: a chain joined to another)
    memset(used, 0, (size_t)nc * sizeof *used);
    for (int c = 0; c < nc; c++) {
        if (used[c]) continue;
        used[c] = 1;
        int n = 0;
        for (int k = start[c]; k < start[c + 1] && n < UH_LOOP; k++) loop[n++] = chain[k];
        while (open[c] && n < UH_LOOP) {   // an open one: the next open one whose start lies by its end
            int best = -1;
            float bd = UH_JOIN * f->len;
            for (int o = 0; o < nc; o++) {
                if (used[o] || !open[o]) continue;
                const float *a = pt[loop[n - 1]], *b = pt[chain[start[o]]];
                float d = sqrtf((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
                if (d < bd) bd = d, best = o;
            }
            if (best < 0) break;
            used[best] = 1;
            for (int k = start[best]; k < start[best + 1] && n < UH_LOOP; k++) loop[n++] = chain[k];
        }
        if (n < 3) continue;
        int made = uh_ears((const float (*)[2])p2, n, loop, tri, 3 * UH_LOOP);
        for (int k = 0; k + 2 < made; k += 3) {
            const float *a = pt[tri[k]], *b = pt[tri[k + 1]], *d = pt[tri[k + 2]];
            float e1[3] = { b[0] - a[0], b[1] - a[1], b[2] - a[2] }, e2[3] = { d[0] - a[0], d[1] - a[1], d[2] - a[2] };
            float x[3] = { e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0] };
            int flip = (x[0] * out[0] + x[1] * out[1] + x[2] * out[2]) * way < 0.0f;
            ci[cni++] = (WORD)tri[k], ci[cni++] = (WORD)tri[k + (flip ? 2 : 1)], ci[cni++] = (WORD)tri[k + (flip ? 1 : 2)];
        }
    }
    if (cni && np <= MO_MAX_VERTS) {
        for (int i = 0; i < np; i++) {
            float *x = cv + i * 8;
            memcpy(x, pt[i], 3 * sizeof(float));
            memcpy(x + 3, out, 3 * sizeof(float));
            x[6] = p2[i][0] / f->len * UH_CAP_UV, x[7] = p2[i][1] / f->len * UH_CAP_UV;
        }
        h->cv = cv, h->ci = ci, h->cnv = (uint32_t)np, h->cni = cni;
        cv = NULL, ci = NULL;
        InterlockedIncrement(&g_uh.caps);
    }
end:
    free(pt), free(p2), free(next), free(has_prev), free(done), free(loop), free(tri), free(chain), free(start), free(open), free(cv), free(ci);
}
// a half that can't be made: its parts given up, never drawn (the slot kept: not tried again at every draw)
internal void uh_fail(uh_half *h, const char *why) {
    for (int i = 0; i < h->np; i++) free(h->piece[i].v), free(h->piece[i].idx);
    h->np = 0, h->state = UH_NOT;
    if (InterlockedIncrement(&g_uh.failed) <= 16 && g_log)
        log_locked("# wrecks: a ship's half (serial %u, %s) not made: %s (%d parts taken)\n", h->serial, h->half == 1 ? "bow" : "stern", why, g_uh.nt);
}
// the half's model of the parts taken (g_uh.t), the break's plane in the world (cl): UH_OK, UH_NOT - or UH_NEW again when
// the walk gave nothing (not drawn: tried at the next)
internal void uh_make(uh_half *h, const ud_clip *cl) {
    const uh_taken *rt = NULL;
    for (int i = 0; i < g_uh.nt && !rt; i++) if (g_uh.t[i].m == h->root) rt = &g_uh.t[i];
    if (!rt) { if (g_uh.nt) uh_fail(h, "no part of its root model"); return; }
    const float *w = &rt->w._11;
    float a = w[0], b = w[1], c = w[2], d = w[4], e = w[5], f3 = w[6], g = w[8], hh = w[9], k = w[10];
    float det = a * (e * k - f3 * hh) - b * (d * k - f3 * g) + c * (d * hh - e * g);
    if (fabsf(det) < 1e-12f) { uh_fail(h, "its root's world has no inverse"); return; }
    float inv[9] = { (e * k - f3 * hh) / det, (c * hh - b * k) / det, (b * f3 - c * e) / det,
                     (f3 * g - d * k) / det, (a * k - c * g) / det, (c * d - a * f3) / det,
                     (d * hh - e * g) / det, (b * g - a * hh) / det, (a * e - b * d) / det };
    uh_frame fr;
    memset(&fr, 0, sizeof fr);
    float len = 0.0f;
    for (int j = 0; j < 3; j++) fr.n[j] = w[j * 4] * cl->pl[0] + w[j * 4 + 1] * cl->pl[1] + w[j * 4 + 2] * cl->pl[2], len += fr.n[j] * fr.n[j];
    fr.d = cl->pl[3] + w[12] * cl->pl[0] + w[13] * cl->pl[1] + w[14] * cl->pl[2];
    len = sqrtf(len);
    if (len < 1e-9f) { uh_fail(h, "no plane"); return; }
    float sg = cl->half == 1 ? 1.0f : -1.0f;   // (the plane faces the half kept: the bow's way taken for both)
    for (int j = 0; j < 3; j++) fr.n[j] *= sg / len;
    fr.d *= sg / len;
    fr.bow = cl->half == 1;
    float zu = fr.n[2], ul = 0.0f;
    for (int j = 0; j < 3; j++) fr.up[j] = (j == 2 ? 1.0f : 0.0f) - zu * fr.n[j], ul += fr.up[j] * fr.up[j];
    ul = sqrtf(ul);
    for (int j = 0; j < 3; j++) fr.up[j] /= ul > 1e-6f ? ul : 1.0f;
    fr.across[0] = fr.up[1] * fr.n[2] - fr.up[2] * fr.n[1], fr.across[1] = fr.up[2] * fr.n[0] - fr.up[0] * fr.n[2], fr.across[2] = fr.up[0] * fr.n[1] - fr.up[1] * fr.n[0];
    fr.deck = cl->deck;
    // every part's vertices into the root's space, their F; the length, the beam under the hull's top
    float *q[UH_PARTS], *F[UH_PARTS], flo = 1e30f, fhi = -1e30f, blo = 1e30f, bhi = -1e30f, way = 0.0f;
    int ok = 1;
    for (int i = 0; i < g_uh.nt; i++) {
        const uh_taken *t = &g_uh.t[i];
        q[i] = malloc((size_t)t->n * 3 * sizeof(float)), F[i] = malloc((size_t)t->n * sizeof(float));
        if (!q[i] || !F[i]) { ok = 0; continue; }
        const float *tw = &t->w._11;
        for (uint32_t j = 0; j < t->n; j++) {
            const float *s = t->v + j * 8;
            float x[3];
            for (int r = 0; r < 3; r++) x[r] = s[0] * tw[r] + s[1] * tw[4 + r] + s[2] * tw[8 + r] + tw[12 + r];
            uh_to_root(w, inv, x, q[i] + j * 3);
            const float *y = q[i] + j * 3;
            F[i][j] = fr.n[0] * y[0] + fr.n[1] * y[1] + fr.n[2] * y[2] + fr.d;
            flo = fminf(flo, F[i][j]), fhi = fmaxf(fhi, F[i][j]);
            if (y[0] * fr.up[0] + y[1] * fr.up[1] + y[2] * fr.up[2] < fr.deck) {
                float u = y[0] * fr.across[0] + y[1] * fr.across[1] + y[2] * fr.across[2];
                blo = fminf(blo, u), bhi = fmaxf(bhi, u);
            }
        }
        if (t->m == h->root)   // the way the root's triangles face their normals (the filling the same)
            for (uint32_t j = 0; j + 2 < t->ni; j += 3) {
                if (t->idx[j] >= t->n || t->idx[j + 1] >= t->n || t->idx[j + 2] >= t->n) continue;
                const float *p0 = t->v + t->idx[j] * 8, *p1 = t->v + t->idx[j + 1] * 8, *p2 = t->v + t->idx[j + 2] * 8;
                float e1[3] = { p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2] }, e2[3] = { p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2] };
                float x[3] = { e1[1] * e2[2] - e1[2] * e2[1], e1[2] * e2[0] - e1[0] * e2[2], e1[0] * e2[1] - e1[1] * e2[0] };
                way += x[0] * (p0[3] + p1[3] + p2[3]) + x[1] * (p0[4] + p1[4] + p2[4]) + x[2] * (p0[5] + p1[5] + p2[5]);
            }
    }
    fr.lo = flo, fr.len = fhi > flo ? fhi - flo : 1.0f, fr.beam = bhi > blo ? bhi - blo : 0.0f;
    g_uh.nseg = 0;
    h->np = 0, h->root_mat = rt->mat;
    for (int i = 0; ok && i < g_uh.nt; i++) {
        uh_piece *pc = &h->piece[h->np];
        memset(pc, 0, sizeof *pc);
        pc->m = g_uh.t[i].m, pc->mat = g_uh.t[i].mat;
        int seg0 = g_uh.nseg;
        if (uh_cut(&fr, &g_uh.t[i], q[i], F[i], pc)) {
            // the cut's corners into the root's space: uh_cut wrote them in the part's own (its blended vertices)
            const float *tw = &g_uh.t[i].w._11;
            for (int s = seg0; s < g_uh.nseg; s++)
                for (int k2 = 0; k2 < 2; k2++) {
                    float *y = g_uh.seg[s] + k2 * 3, x[3];
                    for (int r = 0; r < 3; r++) x[r] = y[0] * tw[r] + y[1] * tw[4 + r] + y[2] * tw[8 + r] + tw[12 + r];
                    uh_to_root(w, inv, x, y);
                }
            if (g_uh.nseg > seg0) InterlockedIncrement(&g_uh.parts_cut);
        } else memset(pc, 0, sizeof *pc), pc->m = g_uh.t[i].m, pc->mat = g_uh.t[i].mat, g_uh.nseg = seg0;   // (left out)
        h->np++;
    }
    for (int i = 0; i < g_uh.nt; i++) free(q[i]), free(F[i]);
    if (!ok) { uh_fail(h, "no memory"); return; }
    uh_fill(h, &fr, way >= 0.0f ? 1.0f : -1.0f);
    h->state = UH_OK;
    InterlockedIncrement(&g_uh.made);
    if (g_log && g_uh.made <= 16)
        log_locked("# wrecks: a ship's half (serial %u, %s) made: %d parts, %d of the cut's edges, its filling %u triangles; length %.3f, beam %.3f\n",
                   h->serial, h->half == 1 ? "bow" : "stern", h->np, g_uh.nseg, h->cni / 3, (double)fr.len, (double)fr.beam);
}
internal int ud_draw_whole(void *rec);
// a record's draw, a half's of its own model: one the mod can't draw is left out, not drawn whole by the game's draw
internal int ud_draw(void *rec) {
    const ud_clip *cl = ud_clip_of(rec);
    if (!cl) return ud_draw_whole(rec);
    ee_dmg_model *g = draw_record_get_gfx(rec);
    ee_model *m = g ? graphics_anim_model(g, draw_record_get_anim(rec)) : NULL;
    uh_half *h = m ? uh_find(cl->serial, cl->half, m) : NULL;
    if (!h) return 1;
    g_uh.cur = h, g_uh_on = 1;
    if (h->state == UH_NEW) {
        g_uh.take = g_uh_take = 1, g_uh.nt = 0;
        ud_draw_whole(rec);
        g_uh.take = g_uh_take = 0;
        uh_make(h, cl);
    }
    if (h->state == UH_OK) h->cap_drawn = 0, ud_draw_whole(rec);
    g_uh_on = 0, g_uh.cur = NULL;
    return 1;
}
internal int ud_draw_whole(void *rec) {
    ee_dmg_model *g = draw_record_get_gfx(rec);
    if (!g) return 0;
    if (!*dmg_model_drawable_at(g) || !(draw_record_get_scale(rec) > *(const float *)engine_display_float_zero_at())) return 1;
    ee_dmg_model *gb = draw_record_get_gfx_built(rec);
    if (gb && *dmg_model_drawable_at(gb) && dmg_model_get_built_model(gb)) return ud_fell(UD_BUILT);
    int bent = bl_noted(g);
    int anim = draw_record_get_anim(rec);
    ee_model *m = graphics_anim_model(g, anim);
    float op = draw_record_get_opacity(rec), bias;
    int burnt = g_ud_death_tex && sil_of(rec) == SIL_WRECK;
    in_rec r = { draw_record_transform_at(rec), draw_record_get_scale(rec),
                 { (uint32_t)draw_record_get_frame(rec), (uint32_t)draw_record_get_next_frame(rec),
                   *engine_display_frame_blending_at() ? draw_record_get_fraction(rec) : 0.0f, op, NULL },
                 burnt ? g_ud_death_tex : graphics_anim_texture(g, anim), m ? model_get_material_count(m) : 0, 0, 0 };
    if (bent && (!game_time() || r.pose.frac != 0)) return ud_fell(UD_BENT);
    if (!in_can(*game_camera_fixed_at()) || !in_init()) return ud_fell(UD_CANT);
    kt_tree *k = m ? kr_find(rec, g, m, anim, &r) : NULL;   // kept: no test of its model, no lookup of its bent copy
    if (k && k->state == KT_OK) return ud_set(rec, g, m, anim, op, &bias) ? kr_parts(k, &r, bias) : ud_fell(UD_CANT);
    if (!m || !in_model_ok(m, &r.pose, 1)) {
        if (k) k->state = KT_NOT;
        return ud_fell(UD_MODEL_NOT);
    }
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
    if (!ud_set(rec, g, m, anim, op, &bias)) return ud_fell(UD_CANT);
    if (k) {   // a new one: taken from the walk, then drawn kept
        kt_make(k, &r, bias);
        if (k->state == KT_OK) return kr_parts(k, &r, bias);
    }
    return in_draw_rec(m, &r, bias) ? 1 : ud_fell(UD_BENT);
}
internal int kt_pipe(char *out, const char *arg) {
    if (!strcmp(arg, "on") || !strcmp(arg, "off")) {
        g_kt.force = arg[1] == 'n';
        REPLY("the trees kept %s\n", g_kt.force ? "on" : "off");
        return 1;
    }
    if (arg[0]) { REPLY("error: usage: picture kept [on | off]\n"); return 1; }
    REPLY("the trees kept: %s; %lu in the table (%ld made, started over %ld times), drawn kept %ld (found by the guess %ld), by the walk %ld (",
          kt_on() ? "on" : "off", (unsigned long)g_kt.n, (long)g_kt.made, (long)g_kt.starts_over, (long)g_kt.drawn, (long)g_kt.guessed, (long)g_kt.walked);
    for (int i = 0; i < KT_REASONS; i++) REPLY("%s%s %ld", i ? ", " : "", k_kt_reason[i], (long)g_kt.reason[i]);
    REPLY("); records drawn kept %ld (made %ld), not to be kept %ld\n", (long)g_kt.rec_drawn, (long)g_kt.rec_made, (long)g_kt.rec_not);
    return 1;
}
// render_silhouette.c's hook of the game's tree draw: 1 when it is drawn here
internal int ut_record(ee_ro_units *ro, float *at, void *rec) {
    int mode = ud_mode();
    if (!mode) return 0;
    if (mode == 1) {
        if (!kt_draw(at, rec)) { InterlockedIncrement(&g_ud.trees_game); return 0; }
        InterlockedIncrement(&g_ud.trees_own);
        return 1;
    }
    g_ud.side = 0; g_ud.n[0] = g_ud.n[1] = 0; g_ud.over = 0; g_ud.bent = 0;
    g_in_capture = 1;
    int mine = kt_draw(at, rec);
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
// THE TREES' OWN LIST (the pipe: picture trees on | off | (nothing: its counts); with the kept trees). The walk of the
// units' update (collect.c collect_walk) tested every tree on the view's tiles against the view, made its record and put
// it into two of the game's sort maps (the trees' and the shadows'), and the game's tree loop (EEROUnits.draw_trees, EE
// 0x50B235, AoC 0x515A50) set the lights and the blending for each before its draw - about 1.5 + 2.3 + 1 ms a frame for
// the 10 800 trees of the player's save, on top of the draw. A tree the walk can list here instead (collect.c tl_tree:
// every model let through the view's test, the casters off the screen collected - so the test is its tile's sight and its
// size -, whole, of one tile, not selected) gets its record in this list, filled as the game's would be, with the
// picker's entry and the ambient sounds' as before but in no sort map: the game's shadows' render object draws only
// blobs from that one, which the dynamic shadows don't draw. The list holds one user of each graphics it draws (the game's
// records one each), given back at the next walk and at the scene's release. One tree of the walk stays in the game's
// list, the anchor: right after the game's loop draws it, under the states it set for it - a whole tree's lights, no
// see-through blending - the list is drawn by the draw the game's would have made of each (ut_record), in the sort map's
// order (its graphics' key, then the walk's). The trees the player remembers on tiles out of sight (doppelgangers,
// collect.c tl_dopp) too: the game's loop lights a tree by its record's flag (+0x38: EE 0x50B2A9, AoC 0x515AC4 - the
// fogged lights, all of them halved, EE 0x504D17 / AoC 0x50FB74 instead of 0x504C8B / 0x50FAE8), so they have an anchor
// of their own, the walk's first such tree, and are drawn after it under its lights; the list's others after the normal
// anchor.
#define TL_MAX 32768
#define TL_GFX 256
internal struct {
    int force;              // the pipe's: 0 off, 1 on; -1: on
    int on;                 // this walk lists trees here
    uint8_t *rec;           // the records (DRAW_SMALL_REC each)
    uint32_t *key, *order;  // each's sort key; the draw's order
    uint32_t n;
    const void *anchor[2];  // the trees in the game's list the list is drawn after: a normal one's, a remembered one's
    int want[2];            // the walk's next such tree it can take is to be the anchor
    ee_dmg_model *gfx[TL_GFX];
    int ngfx;
    LONG walks, listed, fogged, drawn, normal, full;
} g_tl = { .force = -1 };
internal int tl_on(void) { return (g_tl.force < 0 ? 1 : g_tl.force) && kt_on(); }
// a graphics drawn from the list: a user of it held until the next walk
internal int tl_hold(ee_dmg_model *g) {
    for (int i = 0; i < g_tl.ngfx; i++) if (g_tl.gfx[i] == g) return 1;
    if (g_tl.ngfx >= TL_GFX) return 0;
    graphics_acquire(g);
    g_tl.gfx[g_tl.ngfx++] = g;
    return 1;
}
// the users given back and the list emptied (a walk's start, the scene's release)
internal void tl_clear(void) {
    for (int i = 0; i < g_tl.ngfx; i++) graphics_release(g_tl.gfx[i]);
    g_tl.ngfx = 0;
    g_tl.n = 0;
    g_tl.anchor[0] = g_tl.anchor[1] = NULL;
    g_tl.want[0] = g_tl.want[1] = 0;
}
// the list's trees of an anchor drawn after it (render_silhouette.c's tree hook): the sort map's order by key - few
// keys, each's trees in the order listed
internal inline int tl_of(uint32_t i, int fog) { return (draw_small_record_get_flag(g_tl.rec + (size_t)i * DRAW_SMALL_REC) != 0) == fog; }
internal void tl_after(ee_ro_units *ro, float *at, const void *rec) {
    int fog = rec == g_tl.anchor[0] ? 0 : rec == g_tl.anchor[1] ? 1 : -1;
    if (fog < 0 || !g_tl.n) return;
    uint32_t keys[TL_GFX], at_k[TL_GFX], nk = 0, m = 0;
    for (uint32_t i = 0; i < g_tl.n; i++) {
        if (!tl_of(i, fog)) continue;
        uint32_t k = 0;
        while (k < nk && keys[k] != g_tl.key[i]) k++;
        if (k == nk) {
            if (nk == TL_GFX) { g_tl.key[i] = keys[0]; k = 0; }   // (more keys than room: drawn with the first's)
            else keys[nk] = g_tl.key[i], at_k[nk++] = 0;
        }
        at_k[k]++;
    }
    for (uint32_t a = 1; a < nk; a++)   // the keys ascending, their counts with them
        for (uint32_t b = a; b > 0 && keys[b - 1] > keys[b]; b--) {
            uint32_t t = keys[b]; keys[b] = keys[b - 1]; keys[b - 1] = t;
            t = at_k[b]; at_k[b] = at_k[b - 1]; at_k[b - 1] = t;
        }
    for (uint32_t k = 0, sum = 0; k < nk; k++) { uint32_t c = at_k[k]; at_k[k] = sum; sum += c; }
    for (uint32_t i = 0; i < g_tl.n; i++) {
        if (!tl_of(i, fog)) continue;
        uint32_t k = 0;
        while (keys[k] != g_tl.key[i]) k++;
        g_tl.order[at_k[k]++] = i;
        m++;
    }
    for (uint32_t i = 0; i < m; i++) {
        uint8_t *r = g_tl.rec + (size_t)g_tl.order[i] * DRAW_SMALL_REC;
        if (!ut_record(ro, at, r)) ro_units_draw_tree(ro, at, r);
    }
    InterlockedExchangeAdd(&g_tl.drawn, (LONG)m);
}
internal int tl_pipe(char *out, const char *arg) {
    if (!strcmp(arg, "on") || !strcmp(arg, "off")) {
        g_tl.force = arg[1] == 'n';
        REPLY("the trees' own list %s\n", g_tl.force ? "on" : "off");
        return 1;
    }
    if (arg[0]) { REPLY("error: usage: picture trees [on | off]\n"); return 1; }
    REPLY("the trees' own list: %s; %lu now, %d graphics held; walks %ld, listed %ld (remembered %ld), drawn %ld, left to the game's list %ld, full %ld\n",
          tl_on() ? "on" : "off", (unsigned long)g_tl.n, g_tl.ngfx, (long)g_tl.walks, (long)g_tl.listed, (long)g_tl.fogged, (long)g_tl.drawn,
          (long)g_tl.normal, (long)g_tl.full);
    return 1;
}
// the pipe: picture draw [game | own | check]
internal int um_pipe(char *out, const char *arg);   // collect_maps.c
internal int pd_pipe(char *out, const char *arg);   // render_effects.c
internal int ud_pipe(char *out, const char *mode, const char *arg) {
    if (!strcmp(mode, "maps")) return um_pipe(out, arg);
    if (!strcmp(mode, "pieces")) return pd_pipe(out, arg);
    if (!strcmp(mode, "kept")) return kt_pipe(out, arg);
    if (!strcmp(mode, "trees")) return tl_pipe(out, arg);
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
