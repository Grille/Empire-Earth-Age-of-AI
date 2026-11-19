// THE FLYING PIECES' DRAW, THE MOD'S OWN (the pipe: picture pieces game | own | check; on with [display]
// unit_instancing). After a big fight zoomed out the effects took the frame (the player, 2026-11-11: "yes do A"; the test:
// 3000 Minotaurs fought out, ~9000 pieces and 9000 flames and smoke left, the water's render object 12-17 ms of 31-36): the
// pieces a dying mech or machine throws (EEE3DParticle) are drawn by their draw (vf6, EE 0x4EB951, AoC 0x4F556B) one at a
// time, each setting about eight of the device's states (the alpha test, the depth, the colour source, the blending, the
// light, the ambient colour, its material's colour and texture) and putting three back (the colour, the opacity, the
// fog) - so the renderer prepared every piece's states anew and drew it alone, 1.4 us a piece. The pieces lie in long
// runs of one kind, model and glow (7600 in 73 runs): here a run's states are set once, as the piece's draw sets them,
// and each piece goes to the instanced walk (render_instance.c) with its own place, turn, size and opacity - its
// transformation as the draw makes it (GETransformation::SetOrientationYPR's formula with the game's cosines, the place,
// the size from its kind's start to its end by its fade), the model left alone until the run ends, then as the draw
// leaves it: the last piece's place and size prepared, white, opaque, the fog as it was. A run ends before anything
// else of the queue draws - so EEROWater's draw (EE 0x50E1F4, AoC 0x518AE9: the water's surface, then the fifth queue's
// effects node by node, each node a sprites' batch flushed at its end) is drawn here the same, with the run ended
// before each node's sprites are flushed and before every effect that isn't such a piece. Left to the game's draw: a
// piece that glows (its ambient light faded by the fog over its place - set per piece), a model of more than one part
// or none, what the instancing can't draw now.
// picture pieces check: each piece's parts as this draw would hand them over (noted, not drawn), its run begun anew,
// against those of the game's draw (through the instanced walk, noted and drawn) - model, material, texture, pose,
// indices, the place within float rounding and the device's states and the material's colour by a hash.
enum { PD_GLOWS, PD_MODEL, PD_CANT, PD_FELL, PD_REASONS };
internal const char *const k_pd_reason[PD_REASONS] = { "it glows", "its model", "the instancing can't now", "the walk fell" };
enum { PD_KIND_PARTS, PD_KIND_MODEL, PD_KIND_TEX, PD_KIND_POSE, PD_KIND_PLACE, PD_KIND_INDICES, PD_KIND_STATES, PD_KINDS };
internal const char *const k_pd_kind[PD_KINDS] = { "parts", "model or material", "texture", "pose", "place", "indices", "states" };
typedef float (__thiscall *pd_get_ref_fn)(void *ras, uint32_t screen);
typedef uint32_t (__thiscall *pd_get_fog_fn)(void *ras, uint32_t screen);
typedef void (__thiscall *pd_opacity_fn)(ee_model *m, float a);
typedef void (__thiscall *pd_settex_fn)(ee_material *mat, void *tex, uint32_t screen);
internal struct {
    int force;               // the pipe's: 0 the game's draw, 1 the mod's, 2 compared; -1: the mod's (with the instancing)
    int ready;               // the engine's functions found (1), not (-1)
    pd_get_ref_fn get_ref;
    pd_get_fog_fn get_fog;
    pd_opacity_fn opacity;
    pd_settex_fn settex;
    // the run: its pieces' kind, graphics, glow and model, the fog to put back, its last piece's transformation and size
    int open;
    ee_graphic_effect *ge;
    ee_dmg_model *g;
    float glow;
    ee_model *m;
    uint32_t fog;
    float last[16], last_scale;
    LONG own, game, runs, reason[PD_REASONS];
    LONG pieces, differ, kind[PD_KINDS], logged;
    float worst;
} g_pd = { .force = -1 };
internal int pd_mode(void) { return g_pd.force >= 0 ? g_pd.force : in_on() ? 1 : 0; }
internal int pd_init(void) {
    if (g_pd.ready) return g_pd.ready > 0;
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
    if (!lle) return 0;   // (not yet: asked again)
    *(void **)&g_pd.get_ref = (void *)GetProcAddress(lle, "?GetAlphaTestingRefValue@GERasterizer@@QBEMK@Z");
    *(void **)&g_pd.get_fog = (void *)GetProcAddress(lle, "?GetFog@GERasterizer@@QBEKK@Z");
    *(void **)&g_pd.opacity = (void *)GetProcAddress(lle, "?SetOpacity@GEModel@@QAEXM@Z");
    *(void **)&g_pd.settex = (void *)GetProcAddress(lle, "?SetTexture@GEMaterial@@QAEXPAVGETexture@@K@Z");
    g_pd.ready = g_pd.get_ref && g_pd.get_fog && g_pd.opacity && g_pd.settex ? 1 : -1;
    if (g_pd.ready < 0) log_locked("# effects: the engine's functions for the flying pieces' draw not found - the game's draw\n");
    return g_pd.ready > 0;
}
internal int pd_fell(int why) { InterlockedIncrement(&g_pd.reason[why]); return 0; }
// the run ended as the piece's draw ends: the model with the last piece's place and size prepared, its first material
// white, opaque, the fog put back
internal void pd_close(void) {
    if (!g_pd.open) return;
    g_pd.open = 0;
    ee_model *m = g_pd.m;
    memcpy(model_outer_at(m), g_pd.last, sizeof g_pd.last);
    model_set_scale(m, g_pd.last_scale);
    model_prepare(m);
    material_set_colour(model_get_materials(m)[0], (vec3){ 1.0f, 1.0f, 1.0f });
    g_pd.opacity(m, 1.0f);
    rasterizer_set_fog(rasterizer(), g_pd.fog, (uint32_t)screen_index());
}
// a run begun: the states the piece's draw sets, in its order (its opacity is each piece's own, in its pose)
internal void pd_open(ee_graphic_effect *ge, ee_dmg_model *g, float glow, ee_model *m) {
    ee_rasterizer *ras = rasterizer();
    uint32_t sc = (uint32_t)screen_index();
    ee_dmg_model *ag = graphic_effect_get_alpha_graphics(ge);
    rasterizer_set_alpha_ref(ras, ag ? dmg_model_get_alpha_ref(ag) : 0.0f, sc);
    g_pd.fog = g_pd.get_fog(ras, sc);
    rasterizer_set_z_buffer(ras, 2, sc);
    rasterizer_set_shading_color_source(ras, 0, sc);
    rasterizer_set_blending(ras, 4, 5, sc);
    effects_lit();
    float amb[3] = { glow, glow, glow };
    rasterizer_set_ambient_color(ras, amb, sc);
    ee_material *m0 = model_get_materials(m)[0];
    material_set_colour(m0, graphic_effect_get_colour(ge));
    g_pd.settex(m0, graphics_anim_texture(g, 0), sc);
    g_pd.open = 1;
    g_pd.ge = ge, g_pd.g = g, g_pd.glow = glow, g_pd.m = m;
    g_pd.runs++;
}
// GETransformation::SetOrientationYPR (engine 0x472D3): its 3 x 3 by yaw, pitch and roll, its formula with the game's
// cosines (UMCos, a table)
internal void pd_ypr(float *t, float y, float p, float r) {
    float c1 = game_cos(y), s1 = game_cos(1.5707964f - y), c2 = game_cos(p), s2 = game_cos(1.5707964f - p);
    float c3 = game_cos(r), s3 = game_cos(1.5707964f - r), a = -s2 * -s1, b = -s2 * c1;
    t[0] = c3 * c1 + a * s3; t[1] = c2 * -s1; t[2] = -s3 * c1 + a * c3;
    t[4] = c3 * s1 + b * s3; t[5] = c2 * c1; t[6] = -s3 * s1 + b * c3;
    t[8] = s3 * c2; t[9] = s2; t[10] = c3 * c2;
}
internal int pt_blend(void *o, float ms, float *vals, int n, const float *cur);   // collect.c: a piece between its moves
// a flying piece drawn here: 1 drawn, 0 the game's draw is to draw it (no run open then)
internal int pd_piece(ee_particle3d *o) {
    ee_graphic_effect *ge = particle3d_get_graphic_effect(o);
    ee_dmg_model *g = particle3d_get_graphics(o);
    if (!ge || !g) { pd_close(); return pd_fell(PD_MODEL); }
    if (graphic_effect_get_glows(ge)) { pd_close(); return pd_fell(PD_GLOWS); }
    ee_model *m = dmg_model_get_built_model(g);
    if (!m || model_get_child_count(m) || !model_get_material_count(m)) { pd_close(); return pd_fell(PD_MODEL); }
    // its place, turn and fade as the game's draw gets them (collect.c pt_hook_draw: between its moves)
    float cur[7], b[7];
    memcpy(cur, particle3d_place_at(o), 3 * sizeof(float));
    memcpy(cur + 3, particle3d_angles_at(o), 3 * sizeof(float));
    cur[6] = *particle3d_fade_at(o);
    const float *v = pt_blend(o, particle3d_get_moved_ms(o), b, 7, cur) ? b : cur;
    float t[16] = { 0 }, fade = v[6], s0 = graphic_effect_get_size_start(ge);
    pd_ypr(t, v[3], v[4], v[5]);
    t[3] = v[0], t[7] = v[1], t[11] = v[2], t[15] = 1.0f;
    in_rec r = { t, (graphic_effect_get_size_end(ge) - s0) * fade + s0,
                 { model_get_frame(m), model_get_next_frame(m), model_get_fraction(m), (1.0f - fade) * graphic_effect_get_opacity(ge), NULL },
                 NULL, model_get_material_count(m), 0, 1 };
    ee_rasterizer *ras = rasterizer();
    g_pd_in = 1;
    if (!in_can(*game_camera_fixed_at()) || !in_init() || !in_model_ok(m, &r.pose, 1) || !in_device(ras, (uint32_t)screen_index())) {
        g_pd_in = 0;
        pd_close();
        return pd_fell(PD_CANT);
    }
    float glow = particle3d_get_glow(o);
    if (!g_pd.open || ge != g_pd.ge || g != g_pd.g || glow != g_pd.glow || m != g_pd.m) { pd_close(); pd_open(ge, g, glow, m); }
    int ok = in_draw_rec(m, &r, 1.0f);
    g_pd_in = 0;
    memcpy(g_pd.last, t, sizeof t);
    g_pd.last_scale = r.scale;
    if (!ok) { pd_close(); return pd_fell(PD_FELL); }
    return 1;
}
internal float pd_off(const float *a, const float *b, int n) {   // the largest difference, relative above 1
    float w = 0.0f;
    for (int i = 0; i < n; i++) w = fmaxf(w, fabsf(a[i] - b[i]) / fmaxf(1.0f, fabsf(b[i])));
    return w;
}
// the comparison of a piece's two sides (render_units.c's notes: 0 the mod's, 1 the game's)
internal void pd_compare(const void *o) {
    InterlockedIncrement(&g_pd.pieces);
    int bad[PD_KINDS] = { 0 }, n = g_ud.n[0] < g_ud.n[1] ? g_ud.n[0] : g_ud.n[1];
    float worst = 0.0f;
    bad[PD_KIND_PARTS] = g_ud.n[0] != g_ud.n[1];
    for (int i = 0; i < n; i++) {
        const ud_part *a = &g_ud.e[0][i], *b = &g_ud.e[1][i];
        bad[PD_KIND_MODEL] |= a->m != b->m || a->mat != b->mat;
        bad[PD_KIND_TEX] |= a->tex != b->tex;
        bad[PD_KIND_POSE] |= a->p.frame != b->p.frame || a->p.next != b->p.next || pd_off(&a->p.frac, &b->p.frac, 2) > 1e-6f;
        bad[PD_KIND_INDICES] |= a->idx != b->idx || a->ni != b->ni;
        bad[PD_KIND_STATES] |= a->st != b->st;
        float w = pd_off(&a->w._11, &b->w._11, 16);
        worst = fmaxf(worst, w);
        bad[PD_KIND_PLACE] |= w > 1e-4f;
    }
    if (worst > g_pd.worst) g_pd.worst = worst;
    int any = 0;
    for (int k = 0; k < PD_KINDS; k++) if (bad[k]) { any = 1; InterlockedIncrement(&g_pd.kind[k]); }
    if (!any) return;
    InterlockedIncrement(&g_pd.differ);
    if (g_pd.logged++ >= 12 || !g_log) return;
    log_locked("# effects: flying piece %p differs: parts %d / %d (mod / game)", o, g_ud.n[0], g_ud.n[1]);
    for (int k = 1; k < PD_KINDS; k++) if (bad[k]) log_locked(", %s", k_pd_kind[k]);
    if (n) {
        const ud_part *a = &g_ud.e[0][0], *b = &g_ud.e[1][0];
        log_locked(" - tex %p / %p, frame %u %u %.4f a %.4f / %u %u %.4f a %.4f, at %.3f %.3f %.3f / %.3f %.3f %.3f, x axis %.4f %.4f %.4f / %.4f %.4f %.4f, states %08x / %08x",
                   a->tex, b->tex, a->p.frame, a->p.next, (double)a->p.frac, (double)a->p.alpha, b->p.frame, b->p.next,
                   (double)b->p.frac, (double)b->p.alpha, (double)a->w._41, (double)a->w._42, (double)a->w._43, (double)b->w._41,
                   (double)b->w._42, (double)b->w._43, (double)a->w._11, (double)a->w._12, (double)a->w._13, (double)b->w._11,
                   (double)b->w._12, (double)b->w._13, a->st, b->st);
    }
    log_locked("\n");
}
// compared: the piece's parts noted by this draw (its run begun anew), then the game's noted and drawn
internal void pd_check(ee_particle3d *o) {
    g_ud.side = 0; g_ud.n[0] = g_ud.n[1] = 0; g_ud.over = 0;
    g_in_capture = 1;
    int mine = pd_piece(o);
    pd_close();
    g_ud.side = 1;
    g_in_capture = 2;
    g_pd_in = 1;   // (the game's draw through the instanced walk, which notes it)
    effect_draw_water(o);
    g_pd_in = 0;
    g_in_capture = 0;
    if (mine && !g_ud.over) pd_compare(o);
}
// an effect of the fifth queue: a flying piece by this draw, the rest by their own (the run ended first)
internal void pd_effect(void *e) {
    if (effect_is_particle3d(e)) {
        int mode = pd_mode();
        if (mode == 2) { pd_check(e); return; }
        if (pd_piece(e)) { InterlockedIncrement(&g_pd.own); return; }
        InterlockedIncrement(&g_pd.game);
    }
    pd_close();
    effect_draw_water(e);
}
// EEROWater's draw (render_3d.c rd_hook_ro_water): the game's, or the same here with the flying pieces by runs
internal void pd_water(ee_ro_water *o) {
    ee_map *map = game_map();
    if (!pd_mode() || !pd_init() || !map) { ro_water_draw(o); return; }
    ee_rasterizer *ras = rasterizer();
    uint32_t sc = (uint32_t)screen_index();
    float ref = g_pd.get_ref(ras, sc);
    rasterizer_set_alpha_ref(ras, 0.0f, sc);
    rasterizer_set_blending(ras, 4, 5, sc);
    rasterizer_set_z_buffer(ras, 2, sc);
    rasterizer_set_z_compare(ras, 2, sc);
    rasterizer_set_shading_color_source(ras, 1, sc);
    rasterizer_set_culling(ras, 0);
    effects_unlit();
    effects_fog();
    rasterizer_draw_water(ras, map_mesh_at(map), sc, (int8_t)*game_camera_fixed_at());
    uint8_t *queue = effect_queue_of(effect_queues_at(), 4);
    for (uint8_t *n = effect_queue_first(queue); n != effect_queue_head(queue); n = effect_node_next(n)) {
        sprites_begin((ee_texture *)(uintptr_t)effect_node_key(n));
        for (uint32_t k = 0; k < effect_node_count(n); k++) pd_effect(effect_node_at(n, k));
        pd_close();
        sprites_flush();
        sprites_end();
    }
    rasterizer_set_alpha_ref(ras, ref, sc);
}
// the pipe: picture pieces [game | own | check]
internal int pd_pipe(char *out, const char *arg) {
    if (!strcmp(arg, "game") || !strcmp(arg, "own") || !strcmp(arg, "check")) {
        g_pd.force = arg[0] == 'g' ? 0 : arg[0] == 'o' ? 1 : 2;
        if (g_pd.force == 2) {
            g_pd.pieces = g_pd.differ = g_pd.logged = 0;
            g_pd.worst = 0.0f;
            memset(g_pd.kind, 0, sizeof g_pd.kind);
        }
        REPLY("the flying pieces drawn by %s\n", arg[0] == 'g' ? "the game" : arg[0] == 'o' ? "the mod" : "both, compared");
        return 1;
    }
    if (arg[0]) { REPLY("error: usage: picture pieces [game | own | check]\n"); return 1; }
    REPLY("the flying pieces: %s; by the mod %ld in %ld runs, left to the game %ld (", pd_mode() == 0 ? "the game's draw" : pd_mode() == 1 ? "the mod's draw" : "compared",
          (long)g_pd.own, (long)g_pd.runs, (long)g_pd.game);
    for (int k = 0; k < PD_REASONS; k++) REPLY("%s%s %ld", k ? ", " : "", k_pd_reason[k], (long)g_pd.reason[k]);
    REPLY("); compared %ld, differ %ld (", (long)g_pd.pieces, (long)g_pd.differ);
    for (int k = 0; k < PD_KINDS; k++) REPLY("%s%s %ld", k ? ", " : "", k_pd_kind[k], (long)g_pd.kind[k]);
    REPLY("), the places' largest difference %.2g\n", (double)g_pd.worst);
    return 1;
}
