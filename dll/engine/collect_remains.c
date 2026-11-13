// THE REMAINS (collect.c includes it where the fills need them): the dead kept after the game lets them go, sunk,
// faded and drawn by the walk's end; a ship's and an aircraft's wreck by its floats, their fall in
// collect_remains_physics.c
// THE DEAD SINKING ([display] remains_sink, remains_lie_seconds, remains_sink_seconds; the player, 2026-11-11: "dead units
// next, how about letting them slowly sink into the ground instead of the fafing out?", then "the games remains fade out
// times are weirdly mixed, lets try uniform times: 30s where they just lay, 2m where they linelary sink into the ground").
// The game fades a corpse over its type's rot time - rot_ms set to it at the death, a step's ms taken off each step (the
// rotting state's updates), the object removed at 0: a Clubman's 40 s - and the opacity's check tells the rotting apart
// from the game's then. Here every corpse lies as it fell for remains_lie_seconds of game time (the death's animation
// plays then), goes down at an even pace over remains_sink_seconds by UC_SINK_DEPTH of its model's radius, and fades over
// the last UC_FADE_MS (what a slope or a tall wreck leaves out of the ground); whole until then. The radius is the
// standing unit's (a Clubman's 0.70 tiles with its scale), and one lying dead shows hardly 0.06 over the ground: sunk 0.1
// of it, it is under shortly before the end (by twice the radius over 55 % of its 40 s it was gone in 2 s). Its age at the
// walk's game time: its type's rot time less rot_ms is its age at the copy's clock, the time from there to the walk's added
// (the walk shows the game a step and a half behind: it goes down between steps too).
#define UC_SINK_DEPTH 0.1
#define UC_FADE_MS 5000.0
internal double uc_lie_ms(void) { return g_set.remains_lie_seconds * 1000.0; }
internal double uc_end_ms(void) { return (g_set.remains_lie_seconds + g_set.remains_sink_seconds) * 1000.0; }
internal double uc_dead_ms(const uc_obj *o) {   // a rotting object's age at the walk's game time
    double d = (double)o->rot_t - o->rot_ms;
    if (g_uc_now >= 0.0 && g_uc_clock >= 0.0) d += g_uc_now - g_uc_clock;
    return d < 0.0 ? 0.0 : d;
}
internal float uc_dead_opacity(double d) {   // how much of a corpse is seen at its age
    double end = uc_end_ms();
    return d >= end ? 0.0f : d <= end - UC_FADE_MS ? 1.0f : (float)((end - d) / UC_FADE_MS);
}
// a kind the game moves itself as it dies - a ship goes down, a spaceship falls - and fades over its own rot time: left
// to the game unless it is a wreck of ours (remains_physics), not kept, sunk or faded here (the player, 2026-11-12:
// "ships and spaceships should be collected like the game does when our physic arent enabled, elsewise they just get
// stuck halfway" - kept at the pose where the game took it off, halfway down, and lying on for remains_lie_seconds)
internal int uc_game_sinks(const ee_unit_type *ty) {
    int k = unit_type_get_class_id(ty);
    return class_is_ship(k) || k == CLS_SUBMERGED_NAVAL || k == CLS_SPACESHIP || k == CLS_SPACE_TRANSPORT || k == CLS_PLANET_SPACESHIP;
}
// a corpse's pose t lowered as far as its age has taken it
internal void uc_sink_at(float *t, ee_dmg_model *gfx, int32_t anim, int32_t frame, float scale, double d) {
    double q = (d - uc_lie_ms()) / (g_set.remains_sink_seconds * 1000.0);
    if (q <= 0.0 || !gfx) return;
    ee_model *m = ((ee_model **)dmg_model_anim_model_at(gfx))[anim];
    if (!m) return;
    if (q > 1.0) q = 1.0;
    t[11] = (float)(t[11] - UC_SINK_DEPTH * model_get_radii(m)[frame] * scale * q);
}
// THE CORPSES KEPT: one the game removes before its time is up (most: a Clubman's after 40 s) is kept here - its last
// record's pose, graphics, animation and parts - and drawn by the walk on the explored tiles, the way the remembered
// buildings are (uc_corpses_walk), until it has sunk; the game's graphics kept by a user of ours meanwhile (the last
// user's release frees the models: every Clubman gone, the Clubman's models with them). An entry is made when the walk
// meets a rotting object (on the screen, in the player's sight); one not met by a walk - removed, out of the view or
// in the fog - is kept the same way (THE REMAINS SEEN ONCE STAY). Met again (the game's still), it is the game's again.
// Emptied at the scene's release (a game's end, a load: fx_hook_scene_release).
// Its parts' turns are held from its first sight on, the game's as well as the kept: the record draw (0x50B4AC) turns a
// part's model by its part value (a yaw) and the two lists' models by theirs (a yaw, a pitch), and the simulation goes on
// turning a dead unit's - a dead helicopter's rotors spun on the ground (the player, 2026-11-11: "is it possible to kill all
// animations for corpses? the rotors are still rotating here"). Its frames go on: the death's animation plays out.
// UC_NO_CORPSE: a kind whose rot time is 0 leaves none - it blows up and the game removes it at once (an Apache's, a Sea
// King's; logged 2026-11-11: Clubman, horseman, knight, artillery, howitzer 40 s, Minotaur 10, tanks 7), and kept it
// stood whole on the ground for the whole time, the helicopter of the player's picture: neither kept nor drawn, as the
// game has it.
#define UC_CORPSES 4096
#define UC_KC_PARTS 8
typedef struct {
    ee_unit *u;              // the object as it was (only compared)
    ee_dmg_model *gfx;       // its graphics: a user of ours
    ee_unit_type *ty;
    ee_player *owner;
    float t[16];             // its record's pose, not sunk
    float scale;
    uint32_t part[UC_KC_PARTS];                          // its parts' turns (floats), held
    uint32_t la[UC_KC_PARTS][2], lb[UC_KC_PARTS][2];     // its lists' turns and models, held
    double died;             // the game time of its death
    int32_t anim, frame, rot, n_parts, n_la, n_lb;
    uint32_t gen;            // the walk that met it last
    uint8_t gone;            // removed by the game: drawn by the walk
    uint8_t held;            // its turns taken
    uint8_t ship;            // a ship going down by its floats (uc_ship_*)
    float sp[9][3], sq[9][3];   // its floats - bow, stern, port, starboard - and its ballast: where they are and were a step before
                             // (world); an airplane's tailplane, fin and nose weight after them (US_PTS_AIR)
    float body[9][3];        // ... in the hull's own space times its scale, the floats at its waterline
    int32_t n;               // its points: US_PTS, an airplane's US_PTS_AIR
    uint8_t heli;            // a helicopter (US_PTS_HELI)
    float hit;               // an aircraft's ground collider over its floats' middle, along its up (tiles: US_HIT_HIGH)
    float rotor[3];          // ... its rotor's hold (0 .. 1) and its disc's tilt (sines, along and across) - us_rotor
    float spin_t;            // ... how long its rotors have turned since its death (s, in the air; held when it hits)
    float turn_rate[3][UC_KC_PARTS];   // ... its turns' rates (radians/s: its parts', its lists' a and b first values),
    double turn_ms;          //     measured while the game turns them, at its last sight (game ms; 0 none yet)
    float turn_at;           //     and its spin_t then: on from there by their rates (us_turn)
    float tilt[4];           // an airplane's surfaces' angles (their sines): its port and starboard wings' (front up), its
                             // tailplane's (nose up), its fin's (nose right) - us_wings, from the controls at its death
    float along[3], across[3];  // the hull's own axes bow-ward and port-ward
    float leak[4][4];        // each float's leak: its buoyancy in the end (g's, fully under), from when, over how long (s),
                             // its buoyancy before (its kind's, a seesaw's sides more)
    float dig[9];            // how deep each point can go into the seabed (tiles; the floats' and the ballast's)
    float shift;             // how far its ballast has slid to the low side (0 .. 1), once it heeled past US_SHIFT
    int32_t shift_to;        // ... that side's float (2 port, 3 starboard; 0 not yet)
    float zw, range, keel;   // the water's level; a float's depth from dry to fully under; the keel's under the floats
    float lift;              // the floats' buoyancy before the leaks (g's, fully under): US_FLOAT, a submarine's US_SUB_FLOAT,
                             // a helicopter's none
    float wet_at;            // when it first touched the water (s after its death; < 0 not yet): the leaks' clock
    int32_t serial;          // its id: the simulation's thread finds it by it (us_hook_effects_step)
    uint8_t air;             // an aircraft: it splashes into the water and blows up on the ground (US_FX_*)
    uint8_t fx_done;         // ... the effects it had: US_FX_SPLASH, US_FX_CRASH
    uint8_t fx_due;          // ... the ones still to be made on the simulation's thread, at fx_at
    uint8_t stuck;           // an aircraft whose middle hit the ground (or the seabed): it stays as it hit
    uint8_t burnt;           // ... an aircraft's hit on land: drawn burnt out from then on (us_burnt)
    uint8_t shatters;        // ... a flier that lies shattered (us_shatter): its death animation from its hit on land
    int32_t fly_anim, fly_frame;   // ... its animation and frame at its death, drawn until it shatters
    float shatter_t;         // ... when it hit land (s after its death; < 0 not)
    uint8_t in_game;         // the game still has its unit (the effects' step, by its id): not met by a walk, it is out of
                             // the view by the game's copy, not removed (us_alive_known)
    float fx_at[3][3];       // where: the splash's (on the water), the crash's, a spaceship's piece's blast (US_FX_BOOM)
    double sim;              // the game time it is simulated to (ms)
    float snap_t;            // a ship that breaks in two (us_snap): when (s after its death; < 0 never)
    float cut, ends[2];      // ... where, along its length (k->along, its own space times its scale), and its hull's ends
                             //     there (the stern's, the bow's)
    float deck;              // ... its hull's top in its model's own space: what reaches under it is the hull (US_HULL_TOP)
    uint8_t half;            // ... broken: 1 the bow's half, 2 the stern's (each a model of its own: us_clip_tag); 0 whole
    uint8_t twin;            // ... the stern's half: an entry of its own, never the game's unit's
    float cut2;              // ... the second cut of three pieces (half 3 the middle's between them); cut when two
    uint8_t space;           // a spaceship's wreck (A SPACESHIP'S WRECK): it drifts, burnt, and blows up at boom_t
    uint8_t blown;           // ... it has: its entry let go once its blast is made
    float boom_t;            // ... when (s after its death; < 0 never)
    uint8_t floor_snap;      // ... one that breaks on the seabed instead (US_SNAP_FLOOR), once it hit it (floor_hit)
    uint8_t floor_hit;       // ... and the steps since (it breaks US_FLOOR_STEPS on)
    float hit_v;             // ... how fast it went down then (tiles/s)
} uc_corpse;
internal uc_corpse g_kc[UC_CORPSES];
internal uint32_t g_kc_n, g_kc_full, g_kc_drawn;
internal CRITICAL_SECTION g_kc_cs;
internal int g_kc_cs_ok;
internal int32_t uc_kc_n(uint32_t n) { return (int32_t)(n < UC_KC_PARTS ? n : UC_KC_PARTS); }
#include "collect_remains_physics.c"   // the wrecks' floats: a ship's sinking, an aircraft's fall
// a rotting object the walk met (or a dead ship, rotting or not yet), its record rec filled and its pose t not yet sunk:
// its entry made or brought up to date, the record's turns its held ones, and a ship's pose its floats'
internal void uc_corpse_saw(const uc_copy *c, const uc_obj *o, uint8_t *rec, float *t, double d, int ship) {
    if (!g_kc_cs_ok || g_uc_now < 0.0 || !o->gfx) return;
    EnterCriticalSection(&g_kc_cs);
    uc_corpse *k = NULL;
    // by its object and id: one kept long after the game freed it must not be taken for a new one at the same address
    for (uint32_t i = 0; i < g_kc_n && !k; i++) if (g_kc[i].u == o->u && g_kc[i].serial == o->serial && !g_kc[i].twin) k = &g_kc[i];
    if (!k && g_kc_n < UC_CORPSES) {
        k = &g_kc[g_kc_n++];
        memset(k, 0, sizeof *k);
        k->u = o->u, k->serial = o->serial;
    } else if (!k) g_kc_full++;
    if (k) {
        if (k->gfx != o->gfx) {
            graphics_acquire(o->gfx);
            if (k->gfx) graphics_release(k->gfx);
            k->gfx = o->gfx;
        }
        k->ty = o->ty, k->owner = o->owner;
        memcpy(k->t, t, sizeof k->t);
        k->scale = o->cu_height, k->anim = o->anim, k->frame = o->anim_frame, k->rot = o->rot_ms;
        int32_t np = o->foundation ? 0 : uc_kc_n(o->n_parts), na = o->foundation ? 0 : uc_kc_n(o->n_a),
                nb = o->foundation ? 0 : uc_kc_n(o->n_b);
        if (!k->held || k->n_parts != np || k->n_la != na || k->n_lb != nb) {
            k->n_parts = np, k->n_la = na, k->n_lb = nb, k->held = 1;
            for (int32_t i = 0; i < np; i++) k->part[i] = c->pool[o->parts_at + i];
            for (int32_t i = 0; i < na; i++) k->la[i][0] = c->pool[o->a_at + 2 * i], k->la[i][1] = c->pool[o->a_at + 2 * i + 1];
            for (int32_t i = 0; i < nb; i++) k->lb[i][0] = c->pool[o->b_at + 2 * i], k->lb[i][1] = c->pool[o->b_at + 2 * i + 1];
        }
        if (k->ship && k->heli && !k->stuck && k->wet_at < 0.0f) {   // falling dry: its turns the game's, their rates measured
            // (by the copy's game time: frames between two steps see the same turns, the same time)
            double dms = k->turn_ms > 0.0 ? g_uc_clock - k->turn_ms : -1.0;
            for (int32_t i = 0; i < np; i++) us_turn_seen(&k->turn_rate[0][i], &k->part[i], c->pool[o->parts_at + i], dms);
            for (int32_t i = 0; i < na; i++) us_turn_seen(&k->turn_rate[1][i], &k->la[i][0], c->pool[o->a_at + 2 * i], dms);
            for (int32_t i = 0; i < nb; i++) us_turn_seen(&k->turn_rate[2][i], &k->lb[i][0], c->pool[o->b_at + 2 * i], dms);
            if (dms != 0.0) k->turn_ms = g_uc_clock, k->turn_at = k->spin_t;
        }
        for (int32_t i = 0; i < np; i++) ((uint32_t *)vector_first(draw_record_part_values_at(rec)))[i] = us_turn(k, 0, i, k->part[i]);
        for (int32_t i = 0; i < na; i++) ((uint32_t *)vector_first(draw_record_list_a_c_at(rec)))[i] = us_turn(k, 1, i, k->la[i][0]);
        for (int32_t i = 0; i < nb; i++) ((uint32_t *)vector_first(draw_record_list_b_c_at(rec)))[i] = us_turn(k, 2, i, k->lb[i][0]);
        if (ship && !k->ship) k->died = g_uc_now, us_start(k, o, t);   // a ship's age from its first sight dead (its rot is its last 2 s)
        else if (!k->ship) k->died = g_uc_now - d;
        if (k->ship) {   // its floats' pose; never fainter than its own age has it - the game fades a helicopter it is
            // taking off to nothing in its last steps (opacity 0 for three frames before the walk no longer met it, then
            // whole again as a kept one: the player, 2026-11-12, "when an helicopter is falling there is a single fram when
            // the object gets deleted where the falling model flickers"; logged)
            us_run(k), us_pose(k, t), us_clip_tag(rec, k, t);
            draw_record_set_opacity(rec, fmaxf(draw_record_get_opacity(rec), uc_dead_opacity(g_uc_now - k->died)));
            if (k->shatters) {
                int32_t an = draw_record_get_anim(rec), fr = draw_record_get_frame(rec), nx = draw_record_get_next_frame(rec);
                float fc = draw_record_get_fraction(rec);
                us_shatter(k, t, &an, &fr, &nx, &fc);
                draw_record_set_anim(rec, an), draw_record_set_frame(rec, fr), draw_record_set_next_frame(rec, nx), draw_record_set_fraction(rec, fc);
            }
        }
        if (us_burnt(k)) sil_tag(rec, g_uc_walk_gen, SIL_WRECK);
        k->gen = g_uc_walk_gen, k->gone = 0;
    }
    LeaveCriticalSection(&g_kc_cs);
}
// A CRASHED HELICOPTER BURNT OUT (the player, 2026-11-12: "could you generate wreck models for the helicopters? swaping
// the textures for the games generc bunrt out texture and removing the rotor completly"). The game has no burnt
// helicopter - its graphics give every animation the same texture - but a tank's death animation (5) is drawn with the
// game's generic death texture, gen_deathT, its graphics' name for it (one texture for every graphics that names it).
// A helicopter's wreck that hit land is tagged from its hit on (SIL_WRECK): the mod's record draw (render_units.c
// ud_draw) draws it with that texture and its rotors shrunk to nothing; the game's own draw (without unit_instancing)
// as it was. The texture is the first unit type's whose graphics name it (their models loaded for it once, held until
// the scene's release); none found, nothing changes.
// An airplane's too since the same day (the player: "do the burnt out plane wrecks next"): its list a is its
// propellers (logged: a Sopwith's and a P-51's 1, a B-17's 4, the jets' none), gone with the rotors' shrinking; one
// that came down in the water keeps its paint, as a helicopter there does.
#define US_ANIMS 14   // a graphics' animations (acquire's loop over them)
internal ee_dmg_model *g_us_dgfx;
internal int g_us_dlooked;
internal ee_texture *us_death_texture(ee_player *pl) {
    if (g_ud_death_tex || g_us_dlooked || !pl) return g_ud_death_tex;
    g_us_dlooked = 1;
    for (unsigned t = 0, n = type_stats_count(pl); t < n; t++) {
        ee_unit_stats **sp = type_stats(pl, (int)t), *st = sp ? *sp : NULL;
        ee_dmg_model *g = st ? unit_stats_get_gfx(st) : NULL;
        if (!g) continue;
        ee_ustring *const *names = (ee_ustring *const *)dmg_model_anim_texture_names_at(g);
        for (int a = 0; a < US_ANIMS; a++) {
            const char *s = names[a] ? ustring_get_text(names[a]) : NULL, *f = s ? strrchr(s, '\\') : NULL;
            f = f ? f + 1 : s;   // (its folder before it, textures)
            if (!f || _strnicmp(f, "gen_death", 9)) continue;
            graphics_acquire(g);   // (its textures loaded by it)
            g_us_dgfx = g, g_ud_death_tex = graphics_anim_texture(g, a);
            if (g_log) log_locked("# wrecks: the death texture from type %u's animation %d (%s): %p\n", t, a, s, (void *)g_ud_death_tex);
            return g_ud_death_tex;
        }
    }
    if (g_log) log_locked("# wrecks: no graphics names the death texture - helicopters' wrecks drawn as they are\n");
    return NULL;
}
// its record drawn burnt out (a crashed aircraft on land, once the texture is had)
internal int us_burnt(const uc_corpse *k) { return k->ship && k->burnt && us_death_texture(k->owner); }
internal void uc_corpses_clear(void) {
    if (!g_kc_cs_ok) return;
    EnterCriticalSection(&g_kc_cs);
    for (uint32_t i = 0; i < g_kc_n; i++) graphics_release(g_kc[i].gfx);
    g_kc_n = 0;
    if (g_us_dgfx) graphics_release(g_us_dgfx);
    g_us_dgfx = NULL, g_ud_death_tex = NULL, g_us_dlooked = 0;
    LeaveCriticalSection(&g_kc_cs);
}
// a kept corpse into the record rec at its age d, as uc_fill_dopp fills a remembered building: whole or see-through by
// its fade, fogged out of the player's sight, never picked, no silhouette, its parts as last seen; 0 when outside the view
internal int uc_fill_corpse(uc_corpse *k, uint8_t *rec, uint8_t *maps, double d, int fogged) {
    __attribute__((aligned(8))) float pose[16];   // where it lies: its last pose, a ship's by its floats on
    memcpy(pose, k->t, sizeof pose);
    if (k->ship) us_run(k), us_pose(k, pose);
    int32_t anim = k->anim, frame = k->frame, next = k->frame;
    float fraction = 0.0f;
    us_shatter(k, pose, &anim, &frame, &next, &fraction);
    ee_model *m = ((ee_model **)dmg_model_anim_model_at(k->gfx))[anim];
    if (!m) return 0;
    __attribute__((aligned(8))) uint8_t pm[0x60];
    physical_model_new(pm);
    physical_model_set_model(pm, m);
    physical_model_set_frame(pm, frame);
    transform_assign(pm, pose);
    physical_model_set_scale(pm, k->scale);
    int in = units_view_call(4)(pm, opacity_viewport(), *game_camera_fixed_at()) != 0;
    physical_model_free(pm);
    if (!in) return 0;
    opacity_consts oc = opacity_consts_now();
    ee_map *map = game_map();
    ee_unit_type *ty = k->ty;
    float op = uc_dead_opacity(d), pos[3], ground[3] = { 0, 0, 0 };
    sil_tag(rec, g_uc_walk_gen, us_burnt(k) ? SIL_WRECK : 0);
    us_clip_tag(rec, k, pose);
    draw_record_set_type(rec, ty);
    draw_record_set_size(rec, unit_type_get_size(ty));
    memcpy(draw_record_color_at(rec), player_color_at(k->owner), 12);
    draw_record_set_class_id(rec, unit_type_get_class_id(ty));
    draw_record_set_type_id(rec, unit_type_get_id(ty));
    draw_record_set_family(rec, unit_type_get_family(ty));
    draw_record_set_bar_x(rec, 0.0f);
    draw_record_set_bar_y(rec, 0.0f);
    draw_record_set_circle(rec, unit_type_get_mark_size(ty));
    draw_record_set_player(rec, player_get_index(k->owner));
    draw_record_set_producing(rec, (int)producing_none());
    draw_record_set_serial(rec, 0);
    draw_record_set_gfx_built(rec, NULL);
    draw_record_set_tinted(rec, 0);
    draw_record_set_gated(rec, 0);
    uint8_t *label = draw_record_label_at(rec);   // the string emptied: its first character, its length and room
    **(uint8_t **)(label + 4) = 0;
    *(uint32_t *)(label + 0xC) = 0;
    *(uint32_t *)(label + 8) = 0;
    draw_record_set_goal_value(rec, 0);
    draw_record_set_fogged(rec, fogged);
    draw_record_set_opacity(rec, op);
    draw_record_set_scale(rec, k->scale);
    draw_record_set_gfx(rec, k->gfx);
    graphics_acquire(k->gfx);   // the record's own, given back by the units' update as every record's
    vector_resize(draw_record_part_values_at(rec), (uint32_t)k->n_parts, 0);
    vector_resize(draw_record_part_models_at(rec), (uint32_t)k->n_parts, 0);
    for (int32_t i = 0; i < k->n_parts; i++) {
        uint32_t pv = us_turn(k, 0, i, k->part[i]);
        memcpy((float *)vector_first(draw_record_part_values_at(rec)) + i, &pv, 4);
        uint8_t *part = *type_stats_game((uint8_t *)k->gfx + 4, (int)i);
        ((uint32_t *)vector_first(draw_record_part_models_at(rec)))[i] = *(uint32_t *)(part + 0x4C);
    }
    vector_resize(draw_record_list_a_c_at(rec), (uint32_t)k->n_la, 0);
    vector_resize(draw_record_list_a_4_at(rec), (uint32_t)k->n_la, 0);
    for (int32_t i = 0; i < k->n_la; i++) {
        ((uint32_t *)vector_first(draw_record_list_a_c_at(rec)))[i] = us_turn(k, 1, i, k->la[i][0]);
        ((uint32_t *)vector_first(draw_record_list_a_4_at(rec)))[i] = k->la[i][1];
    }
    vector_resize(draw_record_list_b_c_at(rec), (uint32_t)k->n_lb, 0);
    vector_resize(draw_record_list_b_4_at(rec), (uint32_t)k->n_lb, 0);
    for (int32_t i = 0; i < k->n_lb; i++) {
        ((uint32_t *)vector_first(draw_record_list_b_c_at(rec)))[i] = us_turn(k, 2, i, k->lb[i][0]);
        ((uint32_t *)vector_first(draw_record_list_b_4_at(rec)))[i] = k->lb[i][1];
    }
    draw_record_set_frame(rec, frame);
    draw_record_set_next_frame(rec, next);
    draw_record_set_fraction(rec, fraction);
    draw_record_set_state(rec, 0);
    draw_record_set_anim(rec, anim);
    transform_assign(rec, pose);
    float *t = draw_record_transform_at(rec);
    if (map_tile_lit(map, (int)floorf(t[3]), (int)floorf(t[7])) && t[11] >= oc.zero) draw_record_set_ground_z(rec, 0.0f);
    else {
        pos[0] = t[3], pos[1] = t[7], pos[2] = t[11];
        ground_point(map, pos, ground);
        draw_record_set_ground_z(rec, ground[2]);
    }
    if (!k->ship) uc_sink_at(t, k->gfx, k->anim, k->frame, k->scale, d);
    void *pair[2] = { (void *)(uintptr_t)graphics_anim_key(k->gfx, anim), rec };
    uc_map_insert(maps + (op < 1.0f ? 0xC : 0), pair);
    pair[0] = (void *)(uintptr_t)dmg_model_get_shadow_key(k->gfx);
    pair[1] = rec;
    uc_map_insert(engine_display_sort_maps_at() + 0x18, pair);
    return 1;
}
// A WRECK OUT OF THE VIEW BY THE GAME'S COPY (the player, the same day: "the crashing planes sometime vanish, when zooming
// the camera further out they apear again, i asume the draw bounds check is still the games?"): the walk meets a dying
// unit on the game's tile and lets it through by the game's pose, and the game moves its copy itself - a plane glides
// down its own way, a helicopter straight down. Out of the view by that, the wreck in it wasn't drawn, and not met it was
// taken for out of the view and let go, made anew from the game's pose when met again. So whether the game still has
// it is asked by its id on the simulation's thread (the effects' step: k->in_game), and one it has that the walk didn't
// meet is drawn as a kept one, by its own place and pose
internal int us_effects_on(void);
internal volatile int g_us_alive_known;   // the effects' step has looked the wrecks' units up (its hook in, its loop run)
internal int us_alive_known(void) { return g_us_alive_known && us_effects_on(); }
// THE REMAINS SEEN ONCE STAY (the player, 2026-11-12: "maybe we should make all remains visible in fog of war? (as long as
// they where seen once ofcourse)"; first "i just had a few ship wreck disaper" - a dead ship sees nothing, so one lost
// where nothing else of the player's looked went into the fog as it died, and the mod's wreck floats on long after the
// game lets its unit go). The game draws a dead unit only on a tile in the player's sight, so one the fog took, or the
// camera left, wasn't met by the walk: it was let go when its rot time was not nearly over (out of the view, made anew
// from the game's when met again). Now every corpse the walk met once - on the screen in the player's sight - is kept
// when not met until its time is up, and drawn on any explored tile of the copy as a remembered building is: fogged
// out of the player's sight; met again it is the game's again. A wreck the game still has that the walk didn't meet
// (out of the view by the game's copy) is drawn the same way; the game's own corpses stay as they were.
// The walk's end: those not met kept, those whose time is up let go, the kept ones drawn - a wreck's by its floats' middle
internal void uc_corpses_walk(const uc_copy *c, uint8_t *recs, uint32_t *n, uint32_t room, uint8_t *maps, int free_cam) {
    if (!g_kc_cs_ok || g_uc_now < 0.0) return;
    EnterCriticalSection(&g_kc_cs);
    double end = uc_end_ms();
    int known = us_alive_known();
    uint32_t j = 0;
    for (uint32_t i = 0; i < g_kc_n; i++) {
        uc_corpse *k = &g_kc[i];
        int unmet = k->gen != g_uc_walk_gen;
        if (unmet && !k->gone) k->gone = k->ship && known && k->in_game ? 0 : 1;
        double d = g_uc_now - k->died;
        if ((k->gone && d >= end) || (k->blown && !k->fx_due)) {
            graphics_release(k->gfx);
            continue;
        }
        if ((k->gone || unmet) && !k->blown && g_set.remains_sink && *n < room) {
            float at[2] = { k->t[3], k->t[7] };
            if (k->ship) at[0] = 0.25f * (k->sp[0][0] + k->sp[1][0] + k->sp[2][0] + k->sp[3][0]),
                         at[1] = 0.25f * (k->sp[0][1] + k->sp[1][1] + k->sp[2][1] + k->sp[3][1]);
            int x = (int)floorf(at[0]) - c->x0, y = (int)floorf(at[1]) - c->y0;
            const uc_ctile *ct = x >= 0 && y >= 0 && x < c->w && y < c->h ? &c->tiles[y * c->w + x] : NULL;
            if (ct && ct->t && (ct->explored || free_cam) && uc_fill_corpse(k, recs + (size_t)*n * DRAW_REC, maps, d, !free_cam && !ct->seen))
                ++*n, g_kc_drawn++;
        }
        if (j != i) g_kc[j] = *k;
        j++;
    }
    g_kc_n = j;
    LeaveCriticalSection(&g_kc_cs);
}
// THE EFFECTS OF ITS FALL, on the simulation's thread (above us_fx): the wrecks are the mod's while its fills are on and
// the corpses go down by their floats
internal int us_effects_on(void) { return g_uc_own && g_uc_fill == 2 && g_set.remains_sink && g_set.remains_physics; }
// the crash steps' death effect and splash: let out while the wrecks are the mod's (it makes its own)
internal int __thiscall us_hook_crash_effect(ee_effect_mgr *m, ee_unit *unit, void *effect, int *handle, int keeps, int tag, int goal) {
    return us_effects_on() ? 0 : effect_mgr_add_on_unit(m, unit, effect, handle, keeps, tag, goal);
}
internal int __thiscall us_hook_crash_splash(ee_effect_mgr *m, float *pos, void *effect, int *handle) {
    return us_effects_on() ? 0 : effect_mgr_add_at(m, pos, effect, handle);
}
// the same death effect as the game takes a unit off the map (the player, the same day: "helicopters now have 2
// explosions on impact, on where they landed, and one on the ground over hwere they originaly died"): a helicopter's
// copy falls straight down at its own pace and is taken off where it lands - let out for a unit whose wreck is the mod's
internal int __thiscall us_hook_remove_effect(ee_effect_mgr *m, ee_unit *unit, void *effect, int *handle, int keeps, int tag, int goal) {
    int ours = 0;
    if (g_kc_cs_ok && us_effects_on()) {
        int32_t s = unit_get_serial(unit);
        EnterCriticalSection(&g_kc_cs);
        for (uint32_t i = 0; i < g_kc_n && !ours; i++) ours = g_kc[i].ship && g_kc[i].serial == s;
        LeaveCriticalSection(&g_kc_cs);
    }
    return ours ? 0 : effect_mgr_add_on_unit(m, unit, effect, handle, keeps, tag, goal);
}
internal float g_us_kept[UC_CORPSES][12];   // the game's matrices of the wrecks' units during the effects' step
internal float *g_us_put[UC_CORPSES];
internal uint32_t g_us_fx_made;
// the effects' step: each wreck's unit has the wreck's pose for its matrix meanwhile (the game's put back after), the
// splashes and crashes the fall noted made first: an airplane's crash on its unit while the game still has it (it
// lies there for 30 s, the wreck's pose its matrix in every step), else at the place (a kind with no placed version,
// the game's explosion, is then left out); a helicopter's always at the place (its pair, kind 0x3b, is placed). On the
// unit too until 2026-11-12 (the player: "the second explosion at the base game crash also still happens sometimes"):
// the game takes a helicopter's unit off where its copy lands, and an effect on it goes on by the unit's own matrix -
// one made by a wreck that hit first (2 of 9 helicopters, logged) at the game's copy
internal void __thiscall us_hook_effects_step(ee_effect_mgr *m) {
    uint32_t n = 0;
    if (g_kc_cs_ok && us_effects_on()) {
        EnterCriticalSection(&g_kc_cs);
        for (uint32_t i = 0; i < g_kc_n && n < UC_CORPSES; i++) {
            uc_corpse *k = &g_kc[i];
            if (!k->ship) continue;
            ee_unit *cu = k->gone ? NULL : unit_by_id((unsigned)k->serial);
            k->in_game = cu != NULL;
            if (cu) {
                float *mt = unit_matrix_at(cu), pose[16];
                memcpy(g_us_kept[n], mt, sizeof g_us_kept[n]);
                memcpy(pose, mt, sizeof g_us_kept[n]);
                us_pose(k, pose);
                memcpy(mt, pose, sizeof g_us_kept[n]);
                g_us_put[n++] = mt;
            }
            if (k->fx_due & US_FX_SPLASH) effect_mgr_add_at(m, k->fx_at[0], unit_type_get_ground_gfx(k->ty), NULL), g_us_fx_made++;
            if (k->fx_due & US_FX_BOOM) effect_mgr_add_at(m, k->fx_at[2], unit_type_get_impact_gfx(k->ty), NULL), g_us_fx_made++;
            if (k->fx_due & US_FX_CRASH) {
                void *fx = unit_type_get_impact_gfx(k->ty);
                if (cu && !k->heli) effect_mgr_add_on_unit(m, cu, fx, NULL, 0, 0, 0);
                else effect_mgr_add_at(m, k->fx_at[1], fx, NULL);
                g_us_fx_made++;
            }
            k->fx_due = 0;
        }
        g_us_alive_known = 1;
        LeaveCriticalSection(&g_kc_cs);
    }
    effect_mgr_step(m);
    for (uint32_t i = 0; i < n; i++) memcpy(g_us_put[i], g_us_kept[i], sizeof g_us_kept[i]);
}
