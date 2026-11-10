// THE UNITS' COLLECTION, THE MOD'S OWN (the pipe: picture collect own | game; the game's by default). The units' update
// (EEROUnits::vf0 0x503B83, AoC 0x50EABE) fills the draw lists the frame draws from: after its setup (the lists
// emptied, the selection, the unit under the mouse) it walks the visible tiles inside the picker's lock - every
// object on a tile and the parts it is made of, each the view's test asks for (EEROUnits.drawn) into a record of the
// objects' list or the trees', sorted by its opacity; then on a tile out of the player's sight the buildings and trees
// it remembers there (doppelgangers) - and after it the placement ghosts' records and the music measured. collect_walk
// is that walk, of a copy of the world (collect_copy.c: the simulation's, or the walk's own), its fills the game's or
// the mod's ports below, which read the copy, never the world (docs/renderer.md "The units' collection"). The update's
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

#include "collect_copy.c"   // step 3: the world's copy the walk reads

// STEP 4: THE MOVING BETWEEN STEPS (the pipe: picture collect smooth; collect_copy.c cp_pose_swap has the poses). A
// moving object's record drawn between its pose in the last copy but one and its last, g_uc_alpha of the way (the time
// since the last copy over the time between the two) - a step behind the simulation, so that a frame always has both.
// 1 (a pause, or the time up) is the object as the game has it. Not across a jump (a unit set down or teleported:
// further than UC_SMOOTH_FAR tiles), and a turn of more than about 25 degrees only moved, not turned: the matrices are
// blended by their numbers, and a big turn would shrink the model on the way.
#define UC_SMOOTH_FAR 16.0f   // tiles, squared
internal float g_uc_alpha = 1.0f;
internal ee_unit *g_cs_u;     // the probe: the moving object it follows, its last drawn place
internal float g_cs_x, g_cs_y, g_cs_now_x, g_cs_now_y;
internal int g_cs_have, g_cs_seen, g_cs_moving, g_cs_still;
internal uint32_t g_cs_n, g_cs_zero;
internal double g_cs_sum, g_cs_sq;
// STEP 4: THE UNITS' UPDATE DURING A STEP (the pipe: picture collect during on | off). A frame that comes while the
// simulation runs a step ran no units' update (render_3d.c rd_hook_ro_units_update, g_rwl_pass): the units stood. With
// this on it runs from the steps' newest copy: the game's update with what reads the live world left out - the
// ground's pass (the region is the last one's, the ground isn't updated either) and the two picks (uc_hook_pick_*: they read
// through the picker's objects and the mouse's takes a reference), the walk without the picker (a running step may free an object of
// the copy, whose pointer the picker's add would reference: the picker stays the last full update's, what a click picks
// from), an object drawn once by its own stamp in the copy, no ambient sounds, the unit under the mouse the last full
// update's (only compared), the selection's group the copy's. It needs all the fills ours (the game's read the world).
// Only beside the simulation's own step: any other writer (the interface's, a load, which may free the map) counts
// itself before it takes the lock and waits for such an update to end, and the update doesn't start while one is
// counted (g_rwl_nonstep, g_uc_in_during: one of the two sees the other).
internal int g_uc_during;                  // this update is one
internal uc_copy *g_uc_during_c;           // ... of this copy
internal LONGLONG g_uc_walked_at;          // the copy of the last walk, its alpha: nothing to do if the same and at rest
internal float g_uc_walked_alpha = 1.0f;
internal ee_unit *g_uc_last_picked;        // the unit under the mouse of the last full update (only compared)
internal uint32_t g_uc_walk_gen;
internal LONG g_uc_during_walks;
internal inline int uc_in_group(const uc_obj *o, ee_player *me, ee_unit *cu) {
    return g_uc_during ? o->in_group : group_contains(player_get_ui_group(me), cu);
}
internal inline void uc_pick_add(ee_unit *u, void *t, float opacity) {
    if (!g_uc_during) pick_list_add(engine_display_pick_list_at(), u, t, opacity);
}
internal void __thiscall uc_hook_ground_pass(ee_ro_units *ro) { if (!g_uc_during) ro_units_ground_pass(ro); }
internal void __thiscall uc_hook_pick_rect(ee_pick_list *l, void *rect, ee_player *pl, int32_t type, void *into) {
    if (!g_uc_during) pick_list_pick_units(l, rect, pl, type, into);
}
internal ee_unit *__thiscall uc_hook_pick_mouse(ee_pick_list *l, void *point, int32_t prefer) {
    return g_uc_during ? NULL : pick_list_pick_unit(l, point, prefer);
}
internal void uc_smooth(ee_unit *u, float *t, const uc_obj *o) {
    int on = o->has_was && g_uc_alpha < 1.0f && g_cp_smooth == 1;
    float dx = t[3] - o->was[3], dy = t[7] - o->was[7], dz = t[11] - o->was[11];
    int moved = o->has_was && (dx != 0.0f || dy != 0.0f || dz != 0.0f);
    if (on && dx * dx + dy * dy + dz * dz <= UC_SMOOTH_FAR) {
        float a = g_uc_alpha;
        float len = t[0] * t[0] + t[4] * t[4] + t[8] * t[8], dot = t[0] * o->was[0] + t[4] * o->was[4] + t[8] * o->was[8];
        if (dot >= 0.9f * len)
            for (int i = 0; i < 12; i++) { if (t[i] != o->was[i]) t[i] = o->was[i] + (t[i] - o->was[i]) * a; }   // (an unchanged -0 stays -0: a still unit byte for byte)
        else
            for (int i = 3; i < 12; i += 4) { if (t[i] != o->was[i]) t[i] = o->was[i] + (t[i] - o->was[i]) * a; }
    }
    if (!g_cp_smooth) return;
    if (!g_cs_u && moved) g_cs_u = u, g_cs_have = 0, g_cs_still = 0;
    if (u == g_cs_u) g_cs_seen = 1, g_cs_now_x = t[3], g_cs_now_y = t[7], g_cs_moving = moved;
}
// THE ANIMATION BETWEEN STEPS (the pipe: picture collect anim on | off; with units_smooth): the simulation moves a model's
// animation on once a step, a walking soldier about 3 frames of its 15 - four poses a pace, held for a dozen frames of the
// picture each while the soldier itself glided on (the player, 2026-11-09: "the animations are very stuttery now"). The
// animation's place in the last copy but one and in the last, frame + fraction, blended by the same alpha as the pose, the
// model blends the two frames around it itself (CopyAndSmoothStdVertices). Only the same animation, running on by at most
// half of its length (more is no telling which way; a start from the beginning, or the fast speeds). An animation is a
// stretch of its model's frames (anim_first .. anim_last): past its last frame it goes on at its first, as the game's own
// next frame does.
internal int g_uc_anim_smooth = 1;
internal int collect_anim_mode(int on) { g_uc_anim_smooth = on; return 0; }
// An animation may hold each pose for several of its model's frames (a dying artillery piece for 2: frames 7 and 8 share one
// vertex array, 9 and 10 the next; next_frame = frame + 2): the fraction is then the way from one pose to the next while the
// frame runs through both of a pose's numbers, so frame + fraction jumped by 1.28 and 0.28 frames in turn (the player,
// 2026-11-10: "the death animation of attilery ... stutters"), and the frames between two numbers of one pose are no movement
// at all ("the first half smotly complets the full step, the next one just hold still"). So the place is counted in poses.
internal int32_t uc_anim_stride(int32_t frame, int32_t next, int32_t n) {
    int32_t d = next - frame;
    if (d < 0) d += n;   // past the last frame it goes on at the first
    return d < 1 ? 1 : d;
}
internal void uc_anim_between(const uc_obj *o, int32_t *frame, int32_t *next, float *fraction) {
    if (!g_uc_anim_smooth || !o->has_was || g_cp_smooth != 1 || o->was_anim != o->anim) return;
    ee_model *m = ((ee_model **)dmg_model_anim_model_at(o->gfx))[o->anim];
    int32_t first = ((const int32_t *)dmg_model_anim_first_at(o->gfx))[o->anim], last = ((const int32_t *)dmg_model_anim_last_at(o->gfx))[o->anim];
    int32_t n = last - first + 1;
    if (!m || n < 2 || first < 0 || (uint32_t)last >= model_get_frames(m) || o->anim_frame < first || o->anim_frame > last
        || o->was_frame < first || o->was_frame > last) return;
    // the larger of the two: at the animation's end the game's next frame stops at the last, a smaller step than the pose's
    int32_t s = uc_anim_stride(o->anim_frame, o->next_frame, n), sw = uc_anim_stride(o->was_frame, o->was_next, n);
    if (sw > s) s = sw;
    int32_t poses = (n + s - 1) / s, pose = (o->anim_frame - first) / s;
    float at = (float)((o->was_frame - first) / s) + o->was_fraction, by = (float)pose + o->fraction - at;
    if (by < 0.0f) by += (float)poses;
    if (by <= 0.0f || by > (float)poses * 0.5f) return;
    float p = at + by * g_uc_alpha;
    if (p >= (float)poses) p -= (float)poses;
    int32_t k = (int32_t)p, f = first + k * s;
    *fraction = p - (float)k;
    *frame = f;
    // past the last frame: as the game's own next frame there says - the first in a loop, itself where the animation ends and
    // stays (a death's: blended toward its first frame it went back to its beginning for a moment and came back)
    *next = f + s <= last ? f + s : k == pose ? o->next_frame : first;
}
// THE FLYING PIECES BETWEEN STEPS (with the animation's: picture collect anim): what a dying mech throws - a Minotaur's torso,
// the shards - is no part of its model but an effect (EEE3DParticle, moved in the step), so it stood for a step's time and
// jumped (the player, 2026-11-10: "the death animation of a minotaurus cyborg, its still in single frames"). Its draw reads
// three plain things, the place, the angles and the fade: drawn as they were a step ago, blended on to the last step's by the
// time since it came, a step behind as the units are. A piece isn't in any copy of the world: seen new at the draw.
#define PT_SLOTS 4096
typedef struct { ee_particle3d *o; LONGLONG seen, at, period; float was[7], now[7]; } pt_e;
internal pt_e g_pt[PT_SLOTS];
internal uint32_t __thiscall pt_hook_draw(ee_particle3d *o) {
    if (!g_uc_anim_smooth || g_cp_smooth != 1) return eee3_d_particle_draw(o);
    static LONGLONG hz;
    if (!hz) { LARGE_INTEGER f; QueryPerformanceFrequency(&f); hz = f.QuadPart; }
    float *place = particle3d_place_at(o), *fade = particle3d_fade_at(o), *angles = particle3d_angles_at(o), cur[7];
    memcpy(cur, place, 12); memcpy(cur + 3, angles, 12); cur[6] = *fade;
    pt_e *e = &g_pt[((uint32_t)(uintptr_t)o >> 4) * 2654435761u >> 20];
    LONGLONG t = rd_qpc();
    if (e->o != o || t - e->seen > hz / 4) {   // new here, or one before it in this memory
        e->o = o, e->at = t, e->period = 0;
        memcpy(e->was, cur, sizeof cur); memcpy(e->now, cur, sizeof cur);
    }
    e->seen = t;
    if (memcmp(cur, e->now, sizeof cur)) {
        e->period = t - e->at, e->at = t;
        memcpy(e->was, e->now, sizeof cur); memcpy(e->now, cur, sizeof cur);
    }
    if (!e->period || e->period > hz / 2 || t - e->at >= e->period) return eee3_d_particle_draw(o);
    float a = (float)((double)(t - e->at) / (double)e->period), dx = e->now[0] - e->was[0], dy = e->now[1] - e->was[1], b[7];
    if (dx * dx + dy * dy > 16.0f) return eee3_d_particle_draw(o);   // not the same flight
    for (int i = 0; i < 7; i++) b[i] = e->was[i] + (e->now[i] - e->was[i]) * a;
    for (int i = 3; i < 6; i++) if (fabsf(e->now[i] - e->was[i]) > 1.6f) b[i] = e->now[i];   // an angle come round
    memcpy(place, b, 12); memcpy(angles, b + 3, 12); *fade = b[6];
    uint32_t r = eee3_d_particle_draw(o);
    memcpy(place, cur, 12); memcpy(angles, cur + 3, 12); *fade = cur[6];
    return r;
}
// after the walk: the followed object's step on the screen, while it moves
internal void uc_smooth_probe(void) {
    if (!g_cp_smooth || !g_cs_u) return;
    if (!g_cs_seen || (!g_cs_moving && ++g_cs_still > 120)) { g_cs_u = NULL; return; }
    g_cs_seen = 0;
    if (g_cs_moving) {
        g_cs_still = 0;
        if (g_cs_have) {
            double dx = g_cs_now_x - g_cs_x, dy = g_cs_now_y - g_cs_y, d = sqrt(dx * dx + dy * dy);
            g_cs_n++, g_cs_sum += d, g_cs_sq += d * d;
            if (d == 0.0) g_cs_zero++;
        }
    }
    g_cs_x = g_cs_now_x, g_cs_y = g_cs_now_y, g_cs_have = 1;
}

// STEP 2, THE FILLS OURS: THE OPACITY (EEROUnits.opacity, the AoC decompilation's GetUnitOpacity: EE 0x5044D7, AoC
// 0x50F412 with its cloak part in a function of its own, 0x63CEE2). An object's opacity into *out, and whether it is
// drawn see-through (its record into the see-through sort map): "show hidden units" (1: an obstacle or a moved unit
// hidden behind something, 2: every obstacle) at hidden_opacity; a dying unit fading as it rots; a cloakable unit by its
// own opacity, dimmed to an enemy; and a land building faded as it comes near the camera's front plane. Ported from
// the disassembly, its floats as the FPU has them: long double between the stores the game's code makes (a float
// there), in the same order - the game's precision whatever the FPU is set to on the render thread. Of the object's
// copy (collect_copy.c: what the game reads of the unit and its complex unit), the camera the frame's.
internal int uc_cloak(const uc_obj *o, int see, float *out, const opacity_consts *c) {
    if (o->rotting) {
        *out = o->rot_t ? (float)((long double)o->rot_ms / o->rot_t) : 0.0f;
        return see;
    }
    if (!o->cloakable || *game_free_camera_at()) {
        *out = 1.0f;
        return see;
    }
    long double x = o->cu_opacity;
    *out = (float)x;
    if (!(x >= c->one)) see = 1;
    if (o->cloak_friend) return see;   // its owner the player, an ally, or the player out of the game
    long double y = (x - c->cloaked) / c->range * c->cloaked;
    *out = (float)y;
    if (!(y >= c->zero)) *out = 0.0f;
    else if (y > c->cloaked) *out = c->cloaked;
    return see;
}
internal uint32_t g_uc_op_land, g_uc_op_near;   // the check's: land buildings the near fade tested, faded
// The model the near fade measures: its graphics' for the animation. It can be empty in the walk of a copy: the graphics'
// last user gone (the release 0x4D343B frees the models and zeroes the table - the last wall piece of a type shot down)
// while the copy from before still lists the object, which the game's own walk under the world's lock never meets (the
// player's game 2026-11-09, units_smooth: read at 0 in the fade). The game's draw reads the same table: such a building
// is not listed (uc_drawn), the first few logged.
internal uint32_t g_uc_no_model;
internal ee_model *uc_fade_model(const uc_obj *o, int anim) {
    ee_model *m = ((ee_model **)dmg_model_anim_model_at(o->gfx))[anim];
    if (!m && g_uc_no_model++ < 8)
        log_locked("# collect: no model for the near fade: type %d at %d,%d, animation %d, graphics %p (users %d), complex unit %d, "
                   "foundation %d, finished %d, dead %d\n", unit_type_get_id(o->ty), o->x, o->y, anim, (void *)o->gfx,
                   dmg_model_get_users(o->gfx), o->has_cu, o->foundation, o->finished, o->dead);
    return m;
}
internal int uc_opacity(uc_obj *o, float *out) {
    opacity_consts c = opacity_consts_now();
    ee_unit_type *ty = o->ty;
    int see = 0, mode = show_hidden_mode();
    if (mode == 1 && sil_on() && (unit_type_is_land_building(ty) || o->tree)) mode = 0;   // what lies behind a building or a tree is seen through it, it stays whole (render_silhouette.c)
    *out = 1.0f;
    if ((mode == 1 && (unit_type_get_is_obstacle(ty) || o->moved) && o->hidden) ||
        (mode == 2 && unit_type_get_is_obstacle(ty))) {
        see = 1;
        *out = c.hidden;
    }
    // AoC: a see-through one by its cloak too while it is cloaked
    if (o->has_cu && (!see || (build_is("aoc") && o->cloakable && o->cloaked))) see = uc_cloak(o, see, out, &c);
    if (!unit_type_is_land_building(ty) || *game_cinematic_at() || *game_camera_fixed_at()) return see;
    void *vp = opacity_viewport();
    long double r = c.one / ((long double)viewport_far(vp) - viewport_near(vp));
    float range = (float)r, limit = (float)(c.front * r), center[3] = { 0, 0, 0 }, view[3] = { 0, 0, 0 }, scale;
    ee_model *m;
    int frame;
    if (o->has_cu) {
        if (!(m = uc_fade_model(o, o->anim))) return see;
        scale = o->cu_height;   // its physical model's scale (the AoC decompilation's GetScale)
        frame = o->anim_frame;
        transform_apply((float *)o->pm, (const float *)model_center_at(m), center);
    } else {   // on its tile's middle, the ground's height there
        __attribute__((aligned(8))) float t[16];
        if (!(m = uc_fade_model(o, 0))) return see;
        g_uc_transform_ctor(t);
        t[3] = (float)((long double)o->x + c.half);
        t[7] = (float)((long double)o->y + c.half);
        t[11] = mesh_z(map_mesh_at(game_map()), t[3], t[7]);
        scale = o->height;
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
// several tiles (the set met: the update's), and its model against the view - a complex unit's own physical model (the
// copy's); one without (a remembered building), or a tree (shifted off its tile's middle by its id), a physical model of
// its own at the tile's middle turned to the camera's yaw. The view's three calls go to what their sites call now (the
// shadows' casters off the screen). A path point is never drawn (the game asks a `return false`), a flare neither, a
// foundation only for its owner. dry: the met set only looked in, not added to (the check: the game's call comes after).
internal int uc_view_at(const uc_obj *o, int call, float x, float y) {
    __attribute__((aligned(8))) uint8_t pm[0x60];
    __attribute__((aligned(8))) float t[16];
    physical_model_new(pm);
    float z = mesh_z(map_mesh_at(game_map()), x, y);
    transform_orient(t, camera_yaw(), 0.0f, 0.0f);
    t[3] = x, t[7] = y, t[11] = z;
    t[12] = t[13] = t[14] = 0.0f, t[15] = 1.0f;
    physical_model_set_model(pm, ((void **)dmg_model_anim_model_at(o->gfx))[0]);
    physical_model_set_frame(pm, 0);
    float scale = o->height;
    transform_assign(pm, t);
    physical_model_set_scale(pm, scale);
    int r = units_view_call(call)(pm, opacity_viewport(), *game_camera_fixed_at()) != 0;
    physical_model_free(pm);
    return r;
}
internal int uc_drawn(uc_obj *o, const uc_ctile *v, void *met, int dry) {
    opacity_consts c = opacity_consts_now();
    ee_unit_type *ty = o->ty;
    ee_player *me = game_players()[local_player_index()];
    if (!v->seen_me && !o->food && !*game_free_camera_at()) return 0;
    float sc = o->height;
    if (!(sc < c.zero || sc > c.zero)) return 0;   // fcomp, je: 0 or not a number
    int size = unit_type_get_size(ty);
    if (o->tree) {
        if (*draw_small_count_at() >= drawn_small_room()) return 0;
        int id = o->serial, jx = (id % 3 - 1) * size, jy = (id / 3 % 3 - 1) * size;
        long double half = (long double)size * c.half;
        float x = (float)((long double)jx * tree_shift() + o->x + half);
        float y = (float)((long double)jy * tree_shift() + o->y + half);
        return uc_view_at(o, 2, x, y);
    }
    if (type_is_path_point(ty) || *draw_count_at() >= drawn_room() || (o->has_cu && o->dead)) return 0;
    if (unit_type_is_land_building(ty) && !uc_fade_model(o, o->has_cu ? o->anim : 0)) return 0;
    if (size > 1) {
        void *found[2], *key = o->u;
        if (*(void **)doppelganger_set_find(met, found, (ee_doppelganger **)&key) != *(void **)((uint8_t *)met + 4)) return 0;
        if (!dry) doppelganger_set_insert(met, found, (ee_doppelganger **)&key);
    }
    if (!o->has_cu) {
        long double half = (long double)size * c.half;
        if (!uc_view_at(o, 1, (float)((long double)o->x + half), (float)((long double)o->y + half))) return 0;
        return !o->marker || o->owner == me;
    }
    ee_unit_type *cty = o->cty;
    if (unit_type_get_class_id(cty) != CLS_GATE && !units_view_call(0)(o->pm, opacity_viewport(), *game_camera_fixed_at()))
        return 0;
    if (*game_free_camera_at()) return 1;
    if (type_is_path_point(cty)) return o->owner == me && o->pp_shown;
    if (o->foundation) return o->owner == me;
    return !unit_type_get_is_flare(cty);
}
// the walk's view test: the game's, the game's held against ours (ours first, its met set only looked in) or ours
// (the pipe: picture collect drawn game | check | own)
internal volatile int g_uc_drawn;
internal uint32_t g_uc_dr_n, g_uc_dr_diff, g_uc_dr_yes;
internal int uc_drawn_of(ee_ro_units *ro, uc_obj *o, const uc_ctile *v, void *met) {
    if (g_uc_drawn == 2) return uc_drawn(o, v, met, 0);
    int mine = g_uc_drawn == 1 ? uc_drawn(o, v, met, 1) : 0;
    int r = ro_units_drawn(ro, o->u, (ee_unit_vis *)v->t, met) != 0;
    if (g_uc_drawn == 1) {
        g_uc_dr_n++;
        g_uc_dr_yes += r;
        if (mine != r && g_uc_dr_diff++ < 8)
            log_locked("# collect: drawn differs: type %d at %d,%d: the game's %d, ours %d\n", unit_type_get_id(o->ty), o->x, o->y, r, mine);
    }
    return r;
}

// STEP 2: THE FILL (EEROUnits' FillUnit: EE 0x502622, AoC 0x50D3AD - the same code but for addresses and offsets and
// four AoC blocks). An object's draw record from its copy: its type's, owner's and complex unit's values, the health
// bars (its own, then the units inside's for its owner and allies), its graphics' parts and two lists of the complex
// unit, the name under the mouse, its pose (the complex unit's transformation, frame, next frame, fraction; one
// without - a remembered building, a path marker on its target - at its tile turned to the camera), a building site
// scaled up by its hit points, the ground's height under it; then into the picker and the sort map by its graphics'
// key (one with a complex unit also into the third map). Its game calls - the vectors' resize, clear and push, the
// strings' operator=, the picker's add, the maps' insert, the ambient sounds' turn - are the game's own. info: the
// update's (the unit under the mouse +0, the camera's yaw +4, the fighting units +8, the ambient sounds' turn +0x10).
typedef struct { ee_unit *picked; float yaw, fighting; uint32_t frame; uint8_t ambient; } uc_info;
internal void uc_bars(uint8_t *rec, const uc_copy *c, const uc_obj *o) {
    if (o->bars_friend) {
        vector_resize(draw_record_hp_bars_at(rec), o->n_inside + 1, 0);
        for (uint32_t i = 0; i < o->n_inside; i++) memcpy((float *)vector_first(draw_record_hp_bars_at(rec)) + i + 1, c->pool + o->inside_at + i, 4);
    }
    ((float *)vector_first(draw_record_hp_bars_at(rec)))[0] = o->own_bar;
    draw_record_set_power_bar(rec, o->power_bar);
}
internal int uc_in_bar_ids(int32_t serial) {
    const uint8_t *v = engine_display_bar_ids_at();
    const uint32_t *p = vector_first(v), *e = *(const uint32_t *const *)(v + 8);
    for (; p != e; p++) if (*p == (uint32_t)serial) return 1;
    return 0;
}
internal void uc_fill(ee_ro_units *ro, const uc_copy *c, uc_obj *o, uint8_t *rec, void *sort_map, uc_info *info) {
    sil_tag(rec, g_uc_walk_gen, o->has_cu && o->cloakable && !o->rotting && o->cu_opacity < 1.0f && !*game_free_camera_at() ? SIL_CLOAKED :
            unit_type_is_land_building(o->ty) ? SIL_BUILDING : o->has_cu && !o->tree && o->owner_index ? SIL_UNIT : 0);
    opacity_consts oc = opacity_consts_now();
    fill_consts fc = fill_consts_now();
    ee_map *map = game_map();
    const void *mesh = map_mesh_at(map);
    ee_unit *u = o->u, *cu = o->cu, *second = cu;   // second: the object of the third map's insert
    int second_foundation = o->foundation;
    ee_player *me = game_players()[local_player_index()], *owner = o->owner;
    ee_unit_type *ty = o->ty;
    int ux = o->x, uy = o->y, free_cam = *game_free_camera_at() != 0;
    float pos[3] = { 0, 0, 0 }, ground[3] = { 0, 0, 0 }, up[3] = { 0, 1, 0 }, tilt[3] = { 0, 0, 0 };
    int food = o->food;
    if (cu) { if (info->ambient) ambient_add(engine_display_ambient_sounds_at(), cu); }
    else if (info->ambient) ambient_add_object(engine_display_ambient_sounds_at(), u);
    draw_record_set_type(rec, ty);
    draw_record_set_size(rec, unit_type_get_size(ty));
    memcpy(draw_record_color_at(rec), player_color_at(owner), 12);
    draw_record_set_class_id(rec, unit_type_get_class_id(ty));
    draw_record_set_type_id(rec, unit_type_get_id(ty));
    draw_record_set_family(rec, unit_type_get_family(ty));
    draw_record_set_bar_x(rec, o->hp > 0 ? unit_type_get_bar_x(ty) : 0.0f);
    draw_record_set_bar_y(rec, o->hp > 0 ? unit_type_get_bar_y(ty) : 0.0f);
    draw_record_set_circle(rec, unit_type_get_mark_size(ty));
    draw_record_set_player(rec, o->owner_index);
    draw_record_set_producing(rec, (int)producing_none());
    draw_record_set_serial(rec, o->serial);
    draw_record_set_gfx_built(rec, NULL);
    draw_record_set_tinted(rec, 0);
    draw_record_set_gated(rec, 0);
    uint8_t *label = draw_record_label_at(rec);   // the string emptied: its first character, its length and room
    **(uint8_t **)(label + 4) = 0;
    *(uint32_t *)(label + 0xC) = 0;
    *(uint32_t *)(label + 8) = 0;
    draw_record_set_goal_value(rec, 0);
    draw_record_set_fogged(rec, !free_cam && food ? !o->own_seen : 0);
    draw_record_set_gfx(rec, o->gfx);
    graphics_acquire(draw_record_get_gfx(rec));
    if (cu) {
        draw_record_set_scale(rec, o->cu_height);
        draw_record_set_tinted(rec, o->tint_on);
        if (draw_record_get_tinted(rec)) {
            memcpy(draw_record_tint_at(rec), o->tint, 12);
            draw_record_set_tint_alpha(rec, fc.tint_alpha);
        }
        ee_unit_type *cty = o->cty;
        if (o->gate && o->gate_shown) {
            draw_record_set_gate_open(rec, o->gate_open);
            draw_record_set_gated(rec, 1);
        }
        if (*engine_display_music_counts_at() && o->cu_hp && (o->goal_state == 2 || o->goal_state == 3 || o->goal_state == 0xB))
            info->fighting = (float)((long double)info->fighting + oc.one);
        if (build_is("aoc") && unit_type_get_id(cty) == 0x2F0 && o->finished &&
            uc_in_group(o, me, cu))   // AoC's: its ground circle at its range
            ground_circle(ro, (float *)o->pm, o->range, 2);
        if (!o->foundation) {
            uint32_t n = o->n_parts;
            vector_resize(draw_record_part_values_at(rec), n, 0);
            vector_resize(draw_record_part_models_at(rec), n, 0);
            for (uint32_t i = 0; i < n; i++) {
                memcpy((float *)vector_first(draw_record_part_values_at(rec)) + i, c->pool + o->parts_at + i, 4);
                uint8_t *part = *type_stats_game((uint8_t *)draw_record_get_gfx(rec) + 4, (int)i);
                ((uint32_t *)vector_first(draw_record_part_models_at(rec)))[i] = *(uint32_t *)(part + 0x4C);
            }
            vector_resize(draw_record_list_a_c_at(rec), o->n_a, 0);
            vector_resize(draw_record_list_a_4_at(rec), o->n_a, 0);
            for (uint32_t i = 0; i < o->n_a; i++) {
                ((uint32_t *)vector_first(draw_record_list_a_c_at(rec)))[i] = c->pool[o->a_at + 2 * i];
                ((uint32_t *)vector_first(draw_record_list_a_4_at(rec)))[i] = c->pool[o->a_at + 2 * i + 1];
            }
            vector_resize(draw_record_list_b_c_at(rec), o->n_b, 0);
            vector_resize(draw_record_list_b_4_at(rec), o->n_b, 0);
            for (uint32_t i = 0; i < o->n_b; i++) {
                ((uint32_t *)vector_first(draw_record_list_b_c_at(rec)))[i] = c->pool[o->b_at + 2 * i];
                ((uint32_t *)vector_first(draw_record_list_b_4_at(rec)))[i] = c->pool[o->b_at + 2 * i + 1];
            }
        }
        int bars = !free_cam ? uc_in_group(o, me, cu) || player_get_ui_unit(me) == u : o->selected;
        if (!bars) bars = uc_in_bar_ids(o->serial);
        if (bars) {
            if (!unit_type_is_land_building(ty) || o->hp) {
                int mode = *(const int *)engine_display_circle_mode_at();
                if (mode == 0) draw_record_set_circle(rec, unit_type_get_mark_size(ty));
                else if (mode == 1 || mode == 2) {
                    float r = mode == 2 ? o->range : build_is("aoc") ? o->los : o->sight;
                    draw_record_set_circle(rec, r + r);
                }
                uc_bars(rec, c, o);
                void *r = rec;
                vector_push(engine_display_bar_records_at(), &r);
            }
        } else if (info->picked == u) {
            wstring_assign(label, unit_stats_name_at(o->cst));
            uc_bars(rec, c, o);
            *(uint8_t **)engine_display_ui_record_at() = rec;
            if (owner != game_players()[0] && owner != me) {
                *(uint8_t **)engine_display_hover_record_at() = rec;
                int named = player_is_human(owner) || player_get_shows_name(owner);
                wstring_assign(engine_display_hover_name_at(), named ? (void *)player_name_at(owner) : (void *)player_civ_name_at(owner));
            }
        } else if (*(const uint32_t *)engine_display_show_labels_at())
            wstring_assign(label, unit_stats_name_at(o->cst));
        if ((uint32_t)o->producing != producing_none() && o->produce_left > 0) draw_record_set_producing(rec, o->producing);
        if (!o->foundation) {
            int32_t frame = o->anim_frame, next = o->next_frame;
            float fraction = o->fraction;
            uc_anim_between(o, &frame, &next, &fraction);
            draw_record_set_frame(rec, frame);
            draw_record_set_next_frame(rec, next);
            draw_record_set_fraction(rec, fraction);
        } else {
            draw_record_set_frame(rec, 0);
            draw_record_set_next_frame(rec, 0);
            draw_record_set_fraction(rec, 0.0f);
        }
        transform_assign(rec, o->pm);
        uc_smooth(u, draw_record_transform_at(rec), o);
        if (unit_type_get_faces_camera(cty)) {
            transform_apply_vector(rec, up, tilt);
            float pitch = (float)(arc_cos(-tilt[2]) - fc.tilt_offset);
            transform_orient(rec, info->yaw, pitch, 0.0f);
        }
        draw_record_set_state(rec, o->ai_state);
        draw_record_set_anim(rec, o->anim);
        draw_record_set_goal_value(rec, o->goal_value);
        if (!o->finished && !o->foundation) {   // a building site: scaled up by its hit points
            ee_unit_type *bt = unit_type_get_built_type(draw_record_get_type(rec));
            if (bt) draw_record_set_gfx_built(rec, unit_stats_get_gfx(player_stats_of(game_players()[draw_record_get_player(rec)], unit_type_get_id(bt))));
            if (draw_record_get_gfx_built(rec)) graphics_acquire(draw_record_get_gfx_built(rec));
            float *t = draw_record_transform_at(rec);
            float z = t[11];
            t[11] = 0.0f;
            transform_scale(rec, 1.0f, 1.0f, (float)((long double)o->hp / o->st_hp));
            t[11] = z;
        } else if (build_is("aoc") && o->foundation) {   // AoC's: two kinds' sites sunk
            float *t = draw_record_transform_at(rec);
            if (unit_type_get_id(cty) == 0x302) t[11] = (float)((long double)t[11] - fc.sink_302);
            else if (unit_type_get_id(cty) == 0x31F) t[11] = (float)((long double)t[11] - fc.sink_31f);
        }
    } else {
        draw_record_set_scale(rec, o->height);
        int bars = u == player_get_ui_unit(me) || (free_cam && o->selected);
        if (!bars) bars = uc_in_bar_ids(o->serial);
        if (bars) {
            draw_record_set_power_bar(rec, fc.bar_full);
            vector_clear(draw_record_hp_bars_at(rec));
            void *r = rec;
            vector_push(engine_display_bar_records_at(), &r);
        }
        long double half = (long double)draw_record_get_size(rec) * oc.half;
        pos[0] = (float)(half + ux);
        pos[1] = (float)((long double)draw_record_get_size(rec) * oc.half + uy);
        pos[2] = mesh_z(mesh, pos[0], pos[1]);
        if (o->marker && o->tcu) {   // a path marker on its target (world_unit by its hit points)
            second = o->tcu;
            second_foundation = o->t_foundation;
            if (o->marker_ok) {
                pos[0] = o->at[0], pos[1] = o->at[1], pos[2] = o->at[2];
                if (mesh_has_tile(mesh, (int)floorf(pos[0]), (int)floorf(pos[1]))) pos[2] = mesh_z(mesh, pos[0], pos[1]);
                uint32_t id = o->target_id, k[3];
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
        if (o->marker && !(o->tile_z >= fc.marker_min_z)) pos[2] = fc.marker_min_z;
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
    if (o->lit && !(t[11] < oc.zero)) draw_record_set_ground_z(rec, 0.0f);
    else {
        float p[3] = { t[3], t[7], t[11] };
        ground_point(map, p, ground);
        draw_record_set_ground_z(rec, ground[2]);
    }
    uc_pick_add(u, rec, draw_record_get_opacity(rec));
    void *found[2], *pair[2] = { (void *)(uintptr_t)graphics_anim_key(draw_record_get_gfx(rec), draw_record_get_anim(rec)), rec };
    sort_map_insert(sort_map, found, pair);
    if (second && !second_foundation) {
        pair[0] = (void *)(uintptr_t)dmg_model_get_shadow_key(draw_record_get_gfx(rec));
        pair[1] = rec;
        sort_map_insert(engine_display_sort_maps_at() + 0x18, found, pair);
    }
}

// THE TREES' FILL (EEROUnits' FillObject: EE 0x501DFF, AoC 0x50CBA3 - the same code but for addresses and offsets, and
// EE hands only a wood tree to the ambient sounds). A tree's small record: at its tile's middle shifted by its id, the
// ground's height there, turned to the camera's yaw; its hit points; its scale varied by its id; its type's two bar
// values when a diagonal neighbour tile has no tree (the copy's open); its selection circle and the trees' health bars'
// list while it is the player's selected unit; its sway in the wind by the clock and its id (a model of more than one
// frame); then into the sort map by its graphics' key, the picker (its transformation) and the sixth sort map. (The
// game makes a physical model on its stack it never uses: left out.)
internal void uc_fill_tree(uc_obj *o, uint8_t *rec, void *sort_map, uc_info *info, int dry) {
    opacity_consts oc = opacity_consts_now();
    tree_consts tc = tree_consts_now();
    fill_consts fc = fill_consts_now();
    ee_player *me = game_players()[local_player_index()];
    ee_map *map = game_map();
    ee_unit_type *ty = o->ty;
    int id = o->serial, size = unit_type_get_size(ty);
    float *pos = (float *)draw_small_record_pos_at(rec);
    pos[0] = (float)((long double)((id % 3 - 1) * size) * tree_shift() + (long double)size * oc.half + o->x);
    pos[1] = (float)((long double)((id / 3 % 3 - 1) * size) * tree_shift() + (long double)unit_type_get_size(ty) * oc.half + o->y);
    pos[2] = mesh_z(map_mesh_at(map), pos[0], pos[1]);
    __attribute__((aligned(8))) float t[16];
    transform_orient(t, info->yaw, 0.0f, 0.0f);
    transform_set_position(t, pos[0], pos[1], pos[2]);
    t[12] = t[13] = t[14] = 0.0f, t[15] = 1.0f;
    if (!dry && info->ambient && tree_has_ambient(ty)) ambient_add_tree(engine_display_ambient_sounds_at(), o->u);
    draw_small_record_set_hp(rec, (float)((long double)o->hp / o->st_hp));
    draw_small_record_set_flag(rec, 0);
    draw_small_record_set_gfx(rec, o->gfx);
    if (!dry) graphics_acquire(draw_small_record_get_gfx(rec));
    draw_small_record_set_scale(rec, o->height);
    draw_small_record_set_scale(rec, (float)(((long double)((uint32_t)id % 11) * tc.scale_step + oc.one) * draw_small_record_get_scale(rec)));
    draw_small_record_set_bar_x(rec, o->open ? unit_type_get_bar_x(ty) : 0.0f);
    draw_small_record_set_bar_y(rec, o->open ? unit_type_get_bar_y(ty) : 0.0f);
    if (o->u == player_get_selected_unit(me) || (*game_free_camera_at() && (o->selected || uc_in_bar_ids(o->serial)))) {
        draw_small_record_set_circle(rec, unit_type_get_mark_size(ty));
        void *r = rec;
        if (!dry) vector_push_small(engine_display_tree_bar_records_at(), &r);
    }
    draw_small_record_set_anim(rec, 0);
    ee_model *m = ((ee_model **)dmg_model_anim_model_at(draw_small_record_get_gfx(rec)))[draw_small_record_get_anim(rec)];
    draw_small_record_set_frame(rec, 0);
    draw_small_record_set_next_frame(rec, draw_small_record_get_frame(rec));
    if (model_get_frames(m) > 1) {
        draw_small_record_set_frame(rec, draw_small_record_get_frame(rec) + 1);
        int a = (int)((tc.clock + (uint32_t)id * 8) % 360) - 180;
        float arg = (float)((long double)fc.tilt_offset - (long double)a * tc.sway_step);
        long double s = (game_cos_fpu(arg) + oc.one) * oc.half;
        if (!(s >= oc.zero)) s = 0;
        else if (s > oc.one) s = 1;
        draw_small_record_set_sway(rec, (float)s);
    } else draw_small_record_set_sway(rec, 0.0f);
    if (dry) return;
    void *found[2], *pair[2] = { (void *)(uintptr_t)graphics_anim_key(draw_small_record_get_gfx(rec), draw_small_record_get_anim(rec)), rec };
    sort_map_insert(sort_map, found, pair);
    uc_pick_add(o->u, t, draw_small_record_get_opacity(rec));
    pair[0] = (void *)(uintptr_t)dmg_model_get_shadow_key(draw_small_record_get_gfx(rec));
    pair[1] = rec;
    sort_map_insert(engine_display_sort_maps_at() + 0x3C, found, pair);
}

// the walk's trees' fill: the game's, or the game's held against ours (fill check: the record as it was before the
// game's fill, then ours into that copy without the calls that change the game's lists - the record's every byte
// compared, the sway's clock the same)
internal uint32_t g_uc_tr_n, g_uc_tr_diff;
internal volatile int g_uc_fill;
internal void uc_fill_tree_of(ee_ro_units *ro, uc_obj *o, uint8_t *rec, void *map, void *info) {
    uint8_t was[DRAW_SMALL_REC];
    if (g_uc_fill == 1) memcpy(was, rec, sizeof was);
    ro_units_fill_object(ro, o->u, rec, map, info);
    if (g_uc_fill != 1) return;
    uc_fill_tree(o, was, map, (uc_info *)info, 1);
    g_uc_tr_n++;
    if (memcmp(was, rec, sizeof was)) {
        int k = 0;
        while (k < DRAW_SMALL_REC / 4 && !memcmp(was + k * 4, rec + k * 4, 4)) k++;
        if (g_uc_tr_diff++ < 8)
            log_locked("# collect: tree's record differs: type %d at %d,%d: from +0x%X, the game's %08x, ours %08x\n",
                       unit_type_get_id(o->ty), o->x, o->y, k * 4, *(uint32_t *)(rec + k * 4), *(uint32_t *)(was + k * 4));
    }
}

// THE DOPPELGANGERS' FILLS (EEROUnits' FillDoppelgangerUnit: EE 0x5035AD, AoC 0x50E3C8; FillDoppelgangerObject: EE
// 0x502224, AoC 0x50CFBC - the same code but for addresses and offsets, AoC's graphics acquired through their vtable,
// its tile's light byte by 7 bits, every remembered tree handed to the ambient sounds). A building or tree as the player
// remembers it on a tile out of sight, from what the doppelganger answers (its virtual functions, the copy's: a plain
// one from the object's type and owner, a complex one what it was when last seen), its record at *count when it is in
// the view and room is left (counted then): a building at its own pose or its tile's middle (turned to the camera's
// yaw by a type that faces it), see-through never, fogged; a building site scaled up by its hit points with its
// finished building's graphics from the record's player as it was (the fill doesn't set it: the last record there's);
// the ground's height under it (0 on a lit tile at or above it); then the picker, the sort map by its graphics' first
// key (not its animation's) and the third map. A tree as FillObject makes it, but at the walk's tile x, y and by the
// type's unknown_42c for its scale, its hit points the doppelganger's; the remembered tree's own transformation (the
// picker's) never its record's. dry: only the record (the check), no game call that changes the game's lists.
internal int uc_dopp_view(const uc_dopp *p, void *pm, void *t, float scale, int call) {
    physical_model_set_model(pm, ((void **)dmg_model_anim_model_at(p->gfx))[p->anim]);
    physical_model_set_frame(pm, 0);
    transform_assign(pm, t);
    physical_model_set_scale(pm, scale);
    return units_view_call(call)(pm, opacity_viewport(), *game_camera_fixed_at()) != 0;
}
internal int uc_fill_dopp(const uc_dopp *p, uint8_t *rec, uint32_t *count, void *sort_map, uint32_t room, int x, int y,
                          uc_info *info, int dry) {
    opacity_consts oc = opacity_consts_now();
    ee_map *map = game_map();
    ee_unit_type *ty = p->ty;
    __attribute__((aligned(8))) uint8_t pm[0x60];
    __attribute__((aligned(8))) float t[16];
    float pos[3] = { 0, 0, 0 }, ground[3] = { 0, 0, 0 };
    physical_model_new(pm);
    float scale = p->scale;
    if (p->has_t) memcpy(t, p->t, sizeof t);
    else {
        transform_identity(t);
        pos[0] = (float)((long double)unit_type_get_size(ty) * oc.half + x);
        pos[1] = (float)((long double)unit_type_get_size(ty) * oc.half + y);
        pos[2] = mesh_z(map_mesh_at(map), pos[0], pos[1]);
        transform_set_position(t, pos[0], pos[1], pos[2]);
    }
    // (the game turns it twice)
    if (unit_type_get_faces_camera(ty) == 1) transform_orient(t, info->yaw, 0.0f, 0.0f);
    if (unit_type_get_faces_camera(ty) == 1) transform_orient(t, info->yaw, 0.0f, 0.0f);
    int drawn = *count < room && uc_dopp_view(p, pm, t, scale, 4);
    if (drawn) {
        draw_record_set_type(rec, ty);
        draw_record_set_class_id(rec, unit_type_get_class_id(ty));
        draw_record_set_type_id(rec, unit_type_get_id(ty));
        draw_record_set_family(rec, unit_type_get_family(ty));
        draw_record_set_bar_x(rec, p->dead ? 0.0f : unit_type_get_bar_x(ty));
        draw_record_set_bar_y(rec, unit_type_get_bar_y(ty));
        uint8_t *label = draw_record_label_at(rec);   // the string emptied: its first character, its length and room
        **(uint8_t **)(label + 4) = 0;
        *(uint32_t *)(label + 0xC) = 0;
        *(uint32_t *)(label + 8) = 0;
        draw_record_set_gfx_built(rec, NULL);
        memcpy(draw_record_color_at(rec), player_color_at(p->uowner), 12);
        draw_record_set_goal_value(rec, 0);
        draw_record_set_scale(rec, scale);
        draw_record_set_opacity(rec, 1.0f);
        draw_record_set_size(rec, unit_type_get_size(ty));
        draw_record_set_fogged(rec, 1);
        draw_record_set_tinted(rec, 0);
        draw_record_set_gated(rec, 0);
        draw_record_set_gfx(rec, p->gfx);
        if (!p->finished && p->hp > 1) {
            ee_unit_type *bt = unit_type_get_built_type(draw_record_get_type(rec));
            if (bt) draw_record_set_gfx_built(rec, unit_stats_get_gfx(player_stats_of(game_players()[draw_record_get_player(rec)], unit_type_get_id(bt))));
            if (draw_record_get_gfx_built(rec) && !dry) graphics_acquire(draw_record_get_gfx_built(rec));
        }
        if (!dry) graphics_acquire(draw_record_get_gfx(rec));
        transform_assign(rec, t);
        float *rt = draw_record_transform_at(rec);
        if (map_tile_lit(map, (int)floorf(t[3]), (int)floorf(t[7])) && rt[11] >= oc.zero) draw_record_set_ground_z(rec, 0.0f);
        else {
            pos[0] = rt[3], pos[1] = rt[7], pos[2] = rt[11];
            ground_point(map, pos, ground);
            draw_record_set_ground_z(rec, ground[2]);
        }
        if (!p->finished && p->hp != 1)   // a building site: scaled up by its hit points
            transform_scale(rec, 1.0f, 1.0f, (float)((long double)p->hp / p->full));
        draw_record_set_frame(rec, p->frame);
        draw_record_set_next_frame(rec, p->frame);
        draw_record_set_fraction(rec, 0.0f);
        draw_record_set_anim(rec, p->anim);
        draw_record_set_state(rec, 0);
        if (!dry) {
            uc_pick_add(p->u, rec, draw_record_get_opacity(rec));
            void *found[2], *pair[2] = { (void *)(uintptr_t)graphics_anim_key(draw_record_get_gfx(rec), 0), rec };
            sort_map_insert(sort_map, found, pair);
            pair[0] = (void *)(uintptr_t)dmg_model_get_shadow_key(draw_record_get_gfx(rec));
            pair[1] = rec;
            sort_map_insert(engine_display_sort_maps_at() + 0x18, found, pair);
            ++*count;
        }
    }
    physical_model_free(pm);
    return drawn;
}
internal int uc_fill_dopp_tree(const uc_dopp *p, uint8_t *rec, uint32_t *count, void *sort_map, uint32_t room, int x,
                               int y, uc_info *info, int dry) {
    opacity_consts oc = opacity_consts_now();
    tree_consts tc = tree_consts_now();
    fill_consts fc = fill_consts_now();
    ee_map *map = game_map();
    ee_unit_type *ty = p->ty;
    __attribute__((aligned(8))) uint8_t pm[0x60];
    __attribute__((aligned(8))) float t[16];
    physical_model_new(pm);
    float h = unit_type_get_unknown_42c(ty);
    int id = p->serial, size = unit_type_get_size(ty);
    float *pos = (float *)draw_small_record_pos_at(rec);
    pos[0] = (float)((long double)((id % 3 - 1) * size) * tree_shift() + (long double)size * oc.half + x);
    pos[1] = (float)((long double)((id / 3 % 3 - 1) * size) * tree_shift() + (long double)unit_type_get_size(ty) * oc.half + y);
    pos[2] = mesh_z(map_mesh_at(map), pos[0], pos[1]);
    transform_orient(t, info->yaw, 0.0f, 0.0f);
    t[3] = pos[0], t[7] = pos[1], t[11] = pos[2];
    t[12] = t[13] = t[14] = 0.0f, t[15] = 1.0f;
    int drawn = *count < room && uc_dopp_view(p, pm, t, h, 3);
    if (drawn) {
        draw_small_record_set_scale(rec, h);
        draw_small_record_set_flag(rec, 1);
        draw_small_record_set_hp(rec, (float)((long double)p->hp / p->full));
        draw_small_record_set_opacity(rec, 1.0f);
        draw_small_record_set_scale(rec, (float)(((long double)((uint32_t)id % 11) * tc.scale_step + oc.one) * h));
        draw_small_record_set_gfx(rec, p->gfx);
        if (!dry) graphics_acquire(draw_small_record_get_gfx(rec));
        draw_small_record_set_bar_x(rec, p->open ? unit_type_get_bar_x(ty) : 0.0f);
        draw_small_record_set_bar_y(rec, p->open ? unit_type_get_bar_y(ty) : 0.0f);
        draw_small_record_set_anim(rec, p->anim);
        int f = p->frame;
        draw_small_record_set_frame(rec, f);
        draw_small_record_set_next_frame(rec, f);
        ee_model *m = ((ee_model **)dmg_model_anim_model_at(draw_small_record_get_gfx(rec)))[draw_small_record_get_anim(rec)];
        if ((uint32_t)(f + 1) < (uint32_t)model_get_frames(m)) {
            draw_small_record_set_frame(rec, f + 1);
            int a = (int)((tc.clock + (uint32_t)id * 8) % 360) - 180;
            float arg = (float)((long double)fc.tilt_offset - (long double)a * tc.sway_step);
            long double s = (game_cos_fpu(arg) + oc.one) * oc.half;
            if (!(s >= oc.zero)) s = 0;
            else if (s > oc.one) s = 1;
            draw_small_record_set_sway(rec, (float)s);
        } else draw_small_record_set_sway(rec, 0.0f);
        if (!dry) {
            if (info->ambient && tree_has_ambient(ty)) ambient_add_tree(engine_display_ambient_sounds_at(), p->u);
            void *found[2], *pair[2] = { (void *)(uintptr_t)graphics_anim_key(draw_small_record_get_gfx(rec), 0), rec };
            sort_map_insert(sort_map, found, pair);
            uc_pick_add(p->u, t, draw_small_record_get_opacity(rec));
            pair[0] = (void *)(uintptr_t)dmg_model_get_shadow_key(draw_small_record_get_gfx(rec));
            pair[1] = rec;
            sort_map_insert(engine_display_sort_maps_at() + 0x3C, found, pair);
            ++*count;
        }
    }
    physical_model_free(pm);
    return drawn;
}

// the walk's doppelgangers' fills: the game's, or the game's held against ours (fill check: the record as it was before
// the game's fill, then ours dry into that copy - every byte compared, and whether both counted it) or ours
internal uint32_t g_uc_dp_n, g_uc_dp_diff, g_uc_dp_yes;
internal void uc_fill_dopp_of(ee_ro_units *ro, const uc_dopp *p, uint8_t *rec, uint32_t *count, void *map, uint32_t room,
                              int x, int y, void *info, int tree) {
    if (g_uc_fill == 2) {
        if (tree) uc_fill_dopp_tree(p, rec, count, map, room, x, y, (uc_info *)info, 0);
        else uc_fill_dopp(p, rec, count, map, room, x, y, (uc_info *)info, 0);
        return;
    }
    uint8_t was[DRAW_REC];
    size_t sz = tree ? DRAW_SMALL_REC : DRAW_REC;
    uint32_t n0 = *count;
    if (g_uc_fill == 1) memcpy(was, rec, sz);
    if (tree) ro_units_fill_doppelganger_object(ro, p->d, rec, count, map, room, x, y, info);
    else ro_units_fill_doppelganger(ro, p->d, rec, count, map, room, x, y, info);
    if (g_uc_fill != 1) return;
    // ours as the game's saw it: the count before, its record's room the copy
    uint32_t n1 = n0;
    int ours = tree ? uc_fill_dopp_tree(p, was, &n1, map, room, x, y, (uc_info *)info, 1)
                    : uc_fill_dopp(p, was, &n1, map, room, x, y, (uc_info *)info, 1);
    g_uc_dp_n++;
    int game_drew = *count != n0;
    g_uc_dp_yes += (uint32_t)game_drew;
    // (a record left as it was can match by chance - the same doppelganger in the same place the frame before -, so
    // whether each drew it is compared too)
    if (ours != game_drew || memcmp(was, rec, sz)) {
        int k = 0;
        while (k < (int)sz / 4 && !memcmp(was + k * 4, rec + k * 4, 4)) k++;
        if (k == (int)sz / 4) k = 0;
        if (g_uc_dp_diff++ < 8)
            log_locked("# collect: doppelganger's record differs: %s type %d at %d,%d (drawn: the game's %d, ours %d): from +0x%X, the game's %08x, ours %08x\n",
                       tree ? "tree" : "building", unit_type_get_id(p->ty), x, y, game_drew, ours, k * 4, *(uint32_t *)(rec + k * 4),
                       *(uint32_t *)(was + k * 4));
    }
}

// THE GHOSTS' FILL (EEROUnits.fill_ghosts, the AoC decompilation's FillGhostRecords: EE 0x501C29, AoC 0x50C952 - the
// same code but for addresses and offsets, AoC's graphics acquired through their vtable and its types 0x302 and 0x31F
// lifted by their heights). After the walk, when the placement ghosts changed (the preview moved them), a record for
// each - the building being placed under the mouse, an airport's runways, a granary's farms: its model and scale (its
// player's stats of its type), frame, animation, whether it can be placed, its type's and player's values, its tile,
// its footprint's cells and corner heights, its transformation lifted a little over the ground - into the ghost
// records' vector (the game's push_back copies it), all of it inside the ghosts' lock. One record on the stack for them
// all, as the game has it (a field one ghost doesn't set keeps the one before's). dry: no push and no acquire - each
// record held against the game's (want: its records of this call, n of them) in the dwords the record's constructor
// or the fill sets (uc_ghost_set), the differences counted.
internal uint32_t g_uc_gh_n, g_uc_gh_diff, g_uc_gh_calls;
// a ghost record's dword the record's constructor or the fill sets (the check's: the rest - its circle, bars, ground's
// height, opacity, state, serial, player ... - is the stack's as it was, in the game's fill too; the placement's
// drawing sets the opacity and reads none of the rest; the vectors and label are allocations of the copy)
internal int uc_ghost_set(uint32_t o) {
    return o < 0x58 || (o >= 0xDC && o < 0xEC) || o == 0x104 || o == 0x110 || (o >= 0x124 && o < 0x134) || o >= 0x144;
}
internal void uc_fill_ghosts(int dry, uint8_t *want, uint32_t n) {
    __attribute__((aligned(8))) uint8_t rec[0x440];
    fill_consts fc = fill_consts_now();
    uint32_t size = ghost_record_size(), k = 0;
    if (size > sizeof rec) return;
    draw_record_init(rec);
    g_uc_enter(ghost_lock());
    if (ghosts_changed()) {
        void *v = ghost_units();
        for (uint32_t i = 0; i < vector_size(v); i++) {
            ee_ghost_unit *g = *(ee_ghost_unit **)type_stats_game(v, (int)i);
            draw_record_set_gfx(rec, ghost_unit_model(g));
            if (!dry) graphics_acquire(draw_record_get_gfx(rec));
            draw_record_set_fraction(rec, 0.0f);
            draw_record_set_frame(rec, ghost_unit_get_frame(g));
            draw_record_set_next_frame(rec, ghost_unit_get_frame(g));
            draw_record_set_anim(rec, ghost_unit_get_anim(g));
            draw_record_set_gfx_built(rec, NULL);
            draw_record_set_fogged(rec, 0);
            draw_record_set_scale(rec, ghost_unit_scale(g));
            draw_record_set_can_place(rec, ghost_unit_get_can_place(g));
            ee_unit_type *ty = ghost_unit_get_type_data(g);
            draw_record_set_class_id(rec, unit_type_get_class_id(ty));
            draw_record_set_size(rec, unit_type_get_size(ty));
            memcpy(draw_record_color_at(rec), player_color_at(ghost_unit_get_player(g)), 12);
            draw_record_set_type(rec, ty);
            draw_record_set_tinted(rec, 0);
            draw_record_set_gated(rec, 0);
            ghost_record_tile(rec)[0] = ghost_unit_get_tile_x(g);
            ghost_record_tile(rec)[1] = ghost_unit_get_tile_y(g);
            memcpy(ghost_record_cells(rec), ghost_unit_cells_at(g), 0x90);
            memcpy(ghost_record_heights(rec), ghost_unit_heights_at(g), 0x240);
            transform_assign(rec, ghost_unit_transform_at(g));
            float *t = draw_record_transform_at(rec);
            t[11] = (float)((long double)t[11] + ghost_lift());
            if (build_is("aoc") && unit_type_get_id(ty) == 0x302) t[11] = (float)((long double)t[11] + fc.sink_302);
            else if (build_is("aoc") && unit_type_get_id(ty) == 0x31F) t[11] = (float)((long double)t[11] + fc.sink_31f);
            if (!dry) ghost_record_push(ghost_records(), rec);
            else {
                g_uc_gh_n++;
                const uint8_t *w = k < n ? want + (size_t)k * size : NULL;
                k++;
                uint32_t j = 0;
                while (w && j < size / 4 && (!uc_ghost_set(j * 4) || !memcmp(rec + j * 4, w + j * 4, 4))) j++;
                if (!w || j < size / 4) {
                    if (g_uc_gh_diff++ < 8)
                        log_locked("# collect: ghost's record differs: type %d, the game's %u records: from +0x%X, the game's %08x, ours %08x\n",
                                   unit_type_get_id(ty), n, j * 4, w && j < size / 4 ? *(const uint32_t *)(w + j * 4) : 0,
                                   j < size / 4 ? *(const uint32_t *)(rec + j * 4) : 0);
                }
            }
        }
    }
    g_uc_leave(ghost_lock());
    if (dry && k != n && g_uc_gh_diff++ < 8) log_locked("# collect: ghosts' records: the game's %u, ours %u\n", n, k);
    draw_record_destroy(rec);
}
// at the update's call of the ghosts' fill: the game's, the game's held against ours (fill check: its records of this
// call, then ours dry) or ours
internal void __thiscall uc_hook_fill_ghosts(ee_ro_units *ro) {
    if (g_uc_fill == 2) { uc_fill_ghosts(0, NULL, 0); return; }
    uint8_t *v = ghost_records();
    uint32_t size = ghost_record_size(), n0 = (uint32_t)(*(uint8_t **)(v + 8) - *(uint8_t **)(v + 4)) / size;
    ro_units_fill_ghosts(ro);
    if (g_uc_fill != 1) return;
    uint32_t n1 = (uint32_t)(*(uint8_t **)(v + 8) - *(uint8_t **)(v + 4)) / size;
    g_uc_gh_calls++;
    uc_fill_ghosts(1, *(uint8_t **)(v + 4) + (size_t)n0 * size, n1 - n0);
}
// the walk's fill: the game's or ours (the pipe: picture collect fill game | own). walls.c's hook of its site calls
// collect_fill for the fill in the game's walk (ours then of an object copied there and then) and shears its farms
// after it (walls_after_fill, which ours calls too)
internal void walls_after_fill(ee_unit_type *td, int has_physics, ee_unit_stats *st, float *rec);   // walls.c
internal uc_copy g_cp_one;   // the pool of an object copied for the game's walk
internal void __thiscall collect_fill(void *c, ee_unit *u, float *rec, void *p3, void *p4) {
    if (g_uc_fill != 2) { draw_record_fill(c, u, rec, p3, p4); return; }
    uc_obj o;
    g_cp_one.n_pool = 0, g_cp_one.ok = 1;
    cp_obj(&g_cp_one, &o, u, game_players()[local_player_index()], game_map(), show_hidden_mode());
    uc_fill((ee_ro_units *)c, &g_cp_one, &o, (uint8_t *)rec, p3, (uc_info *)p4);
}

// the walk's opacity: the game's, the game's held against ours (each object: both, the game's kept, the differences
// counted and the first logged) or ours (the pipe: picture collect opacity game | check | own)
internal volatile int g_uc_opacity;
internal uint32_t g_uc_op_n, g_uc_op_diff;
internal int uc_opacity_of(ee_ro_units *ro, uc_obj *o, float *out) {
    if (g_uc_opacity == 2) return uc_opacity(o, out);
    int see = ro_units_opacity(ro, o->u, out);
    if (g_uc_opacity == 1) {
        float mine;
        int s = uc_opacity(o, &mine);
        g_uc_op_n++;
        if ((s != 0) != (see != 0) || memcmp(&mine, out, sizeof mine)) {
            if (g_uc_op_diff++ < 8)
                log_locked("# collect: opacity differs: type %d at %d,%d: the game's %d %.9g, ours %d %.9g\n",
                           unit_type_get_id(o->ty), o->x, o->y, see != 0, *out, s, mine);
        }
    }
    return see;
}

// THE WALK, OF THE COPY: the view's tiles (the terrain mesh's visible rows) as the game walks them, from the copy of
// them (cp_take: the simulation's, or made now) - an explored tile's objects in their order, each the view test lets
// through and the picker hasn't got into a record of the objects' list or the trees'; on a tile out of sight the
// buildings and trees the player remembers there
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
    unsigned long nrows = w.rows(mesh);
    long bx0 = LONG_MAX, by0 = LONG_MAX, bx1 = LONG_MIN, by1 = LONG_MIN;
    for (unsigned long r = 0; r < nrows; r++) {
        long y, x0, x1;
        w.row(mesh, r, &y, &x0, &x1);
        if (x0 > x1) continue;
        if (y < by0) by0 = y;
        if (y > by1) by1 = y;
        if (x0 < bx0) bx0 = x0;
        if (x1 > bx1) bx1 = x1;
    }
    int during = g_uc_during;
    g_uc_walk_gen++;   // every walk: the records are filled anew, and what they were tagged in the last one is over (sil_tag)
    if (during) {
        ((uc_info *)info)->ambient = 0;
        ((uc_info *)info)->picked = g_uc_last_picked;
    } else {
        g_uc_last_picked = ((uc_info *)info)->picked;
        g_uc_enter(lock);
        pick_list_reset(picker);
    }
    uint32_t dopp = 0;
    uc_copy *c = during ? (by0 <= by1 && cp_fits(g_uc_during_c, map, pl, (int)bx0, (int)by0, (int)bx1, (int)by1) ? g_uc_during_c : NULL)
                        : by0 <= by1 ? cp_take(map, pl, (int)bx0, (int)by0, (int)bx1, (int)by1) : NULL;
    g_uc_alpha = 1.0f;
    if (c && g_cp_smooth == 1 && c->dt > 0) {
        double a = (double)(rd_qpc() - c->at) / (double)c->dt;
        g_uc_alpha = a < 1.0 ? (float)a : 1.0f;
    }
    for (unsigned long r = 0; c && r < nrows; r++) {
        long y, x0, x1;
        w.row(mesh, r, &y, &x0, &x1);
        for (long x = x0; x <= x1; x++) {
            uc_ctile *ct = &c->tiles[(uint32_t)((y - c->y0) * c->w + (x - c->x0))];
            if (!ct->t || (!ct->explored && !free_cam)) continue;
            for (uint32_t k = 0; k < ct->n_obj; k++) {
                uc_obj *o = &c->objs[c->seq[ct->obj_at + k]];
                if (uc_drawn_of(ro, o, ct, w.met) && (during ? o->walked != g_uc_walk_gen : !w.listed(picker, o->u))) {
                    o->walked = g_uc_walk_gen;
                    if (!o->tree) {
                        if (o->has_cu) *w.complex_n += 1.0f;
                        uint8_t *rec = recs + (size_t)*n * DRAW_REC;
                        int see = uc_opacity_of(ro, o, draw_record_opacity(rec));
                        if (g_uc_fill == 2) {
                            uc_fill(ro, c, o, rec, maps + (see ? 0xC : 0), (uc_info *)info);
                            if (w.fill_hooked) walls_after_fill(o->ty, o->has_cu, o->st, (float *)rec);
                        } else w.fill(ro, o->u, (float *)rec, maps + (see ? 0xC : 0), info);
                        ++*n;
                    } else {
                        uint8_t *rec = small + (size_t)*sn * DRAW_SMALL_REC;
                        int see = uc_opacity_of(ro, o, draw_small_record_opacity(rec));
                        if (g_uc_fill == 2) uc_fill_tree(o, rec, maps + (see ? 0x30 : 0x24), (uc_info *)info, 0);
                        else uc_fill_tree_of(ro, o, rec, maps + (see ? 0x30 : 0x24), info);
                        ++*sn;
                    }
                }
            }
            if (ct->seen || free_cam) continue;
            for (uint32_t k = 0; k < ct->n_dopp; k++) {
                uc_dopp *p = &c->dopps[ct->dopp_at + k];
                if (p->skip) continue;   // a building under construction on the map: only its owner's
                if (unit_type_get_size(p->ty) > 1) {   // on several tiles: once
                    void *found[2];   // the insert's answer is a pair: the node, and whether it was new
                    ee_doppelganger *d = p->d;
                    if (*(void **)doppelganger_set_find(dset, found, &d) != dset_end) continue;
                    doppelganger_set_insert(dset, found, &d);
                }
                dopp++;
                if (!unit_type_get_is_tree(p->ty))
                    uc_fill_dopp_of(ro, p, recs + (size_t)*n * DRAW_REC, n, maps + (unit_type_get_is_obstacle(p->ty) ? 0xC : 0), room,
                                    (int)x, (int)y, info, 0);
                else
                    uc_fill_dopp_of(ro, p, small + (size_t)*sn * DRAW_SMALL_REC, sn, maps + 0x24, small_room, (int)x, (int)y, info, 1);
            }
        }
    }
    if (c && g_cp_on == 2 && !during) cp_check(c);
    uc_smooth_probe();
    g_uc_walked_at = c ? c->at : 0, g_uc_walked_alpha = g_uc_alpha;
    g_uc_alpha = 1.0f;
    if (during) ((uc_info *)info)->picked = NULL;   // (the update's end would release the reference a pick took)
    else {
        cp_release();
        g_uc_leave(lock);
    }
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

// a frame during a step (render_3d.c): 1 the units' update is to run, from the steps' newest copy (held till the end)
internal int collect_during_begin(void) {
    if (!g_uc_during_on || !g_uc_own || g_uc_fill != 2 || g_uc_drawn != 2 || g_uc_opacity != 2 || !g_cp_on || !g_cp_view[4] ||
        *game_free_camera_at())
        return 0;
    InterlockedExchange(&g_uc_in_during, 1);
    if (g_rwl_nonstep > 0) { g_uc_in_during = 0; return 0; }
    ee_map *map = game_map();
    ee_player *pl = map ? game_players()[local_player_index()] : NULL;
    uc_copy *c = NULL;
    EnterCriticalSection(&g_cp_cs);
    if (map && g_cp_new >= 0 && cp_fits(&g_cp[g_cp_new], map, pl, g_cp_view[0], g_cp_view[1], g_cp_view[2], g_cp_view[3]) &&
        (g_cp[g_cp_new].at != g_uc_walked_at || (g_cp_smooth == 1 && g_uc_walked_alpha < 1.0f)))
        c = &g_cp[g_cp_read = g_cp_new];
    LeaveCriticalSection(&g_cp_cs);
    if (!c) { g_uc_in_during = 0; return 0; }
    g_uc_during_c = c;
    g_uc_during = 1;
    return 1;
}
internal void collect_during_end(void) {
    g_uc_during = 0;
    cp_release();
    g_uc_during_walks++;
    g_uc_in_during = 0;
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

internal int g_uc_installed, g_uc_during_ok;
internal void collect_install(void) {
    HMODULE lle = GetModuleHandleA("Low-Level Engine.dll");
    g_uc_enter = lle ? (uc_lock_fn)(void *)GetProcAddress(lle, "?Enter@TSCriticalSection@@QAEXXZ") : NULL;
    g_uc_leave = lle ? (uc_lock_fn)(void *)GetProcAddress(lle, "?Leave@TSCriticalSection@@QAEXXZ") : NULL;
    g_uc_transform_ctor = lle ? (transform_ctor_fn)(void *)GetProcAddress(lle, "??0GETransformation@@QAE@XZ") : NULL;
    InitializeCriticalSection(&g_cp_cs);
    g_cp_cs_ok = 1;
    g_uc_lock_at = (uintptr_t)engine_display_picker_lock_at();
    g_uc_end = (uintptr_t)engine_display_units_walk_end_at();
    uint8_t want[6] = { 0x8B, 0x0D };   // mov ecx, [picker_lock]
    memcpy(want + 2, &g_uc_lock_at, 4);
    g_uc_installed = g_uc_enter && g_uc_leave && patch_insn6_call((uintptr_t)engine_display_units_walk_at(), want, collect_thunk);
    int ghosts = g_uc_installed && patch_fill_ghosts(uc_hook_fill_ghosts), fx_of = 0;
    if (!patch_particle3d_draw(pt_hook_draw) && g_log) fprintf(g_log, "# collect: the flying pieces' draw NOT hooked\n");
    if (g_uc_installed) patch_scene_release(fx_hook_scene_release, &fx_of);
    g_uc_during_ok = g_uc_installed && patch_units_ground_pass(uc_hook_ground_pass) && patch_units_pick_rect(uc_hook_pick_rect) &&
                     patch_units_pick_mouse(uc_hook_pick_mouse);
    if (g_uc_installed && g_set.units_smooth) {   // [display] units_smooth: the walk, its fills and the steps' copy the mod's, the moving on, and during the steps
        g_uc_own = 1, g_uc_opacity = 2, g_uc_drawn = 2, g_uc_fill = 2, g_cp_on = 1, g_cp_smooth = 1;
        if (g_uc_during_ok) g_uc_during_on = 1, g_rwl_during_handoff = 250;
        g_fx_on = 1;
    }
    if (g_log) fprintf(g_log, "# collect: the units' walk %s (the game's until the pipe's picture collect own), the ghosts' fill %s\n",
                       g_uc_installed ? "hooked" : "NOT hooked (exe differs)", ghosts ? "hooked" : "NOT hooked");
}

// the pipe's picture collect own | game: 0 done, -1 not installed
internal int collect_own(int on) {
    if (!g_uc_installed) return -1;
    g_uc_own = on;
    return 0;
}
// the pipe's picture collect ghost <type> <x> <y>: a placement ghost of the type for the local player at the tile, as
// the build menu makes one and the mouse moves it (the game's Create and preview) - for the fill's check in a game
// nobody plays (type -1: only moved there)
internal int collect_ghost(int type, float x, float y) {
    if (!g_uc_installed) return -1;
    float loc[2] = { x, y };
    if (type >= 0) ghost_unit_create(loc, type, game_players()[local_player_index()], 0.0f, 0);
    ghost_unit_preview(loc);
    return 0;
}
// picture collect copy off | on | check (0, 1, 2): the walk's copy of the world - its own of its view (off), the
// simulation's (on), the simulation's held against one made in the frame (check: the copy check's counts begin again)
internal int collect_copy_mode(int how) {
    if (!g_uc_installed) return -1;
    g_cp_chk_n = g_cp_chk_diff = g_cp_chk_objs = 0;
    g_cp_by_step = g_cp_by_frame = g_cp_stales = g_cp_used = 0;
    g_cp_step_qpc = g_cp_frame_qpc = 0;
    g_cp_stale = 1;
    g_cp_on = how;
    return 0;
}
// picture collect effects off | on | check (0, 1, 2): the effects' queues made by the simulation for the frames during its steps
internal int collect_effects_mode(int how) {
    if (!g_uc_installed) return -1;
    g_fx_builds = g_fx_takes = g_fx_queued = g_fx_chk_n = g_fx_chk_diff = g_fx_dead = 0;
    g_fx_qpc = 0;
    g_fx_on = how;
    return 0;
}
// picture collect during off | on [ms] (ms: the hand-off's wait while such updates run, render_3d.c)
internal int collect_during_mode(int on, int ms) {
    if (!g_uc_installed || !g_uc_during_ok) return -1;
    g_uc_during_walks = 0;
    g_uc_during_on = on;
    g_rwl_during_handoff = on ? ms : 0;
    return 0;
}
// picture collect smooth off | on | probe (0, 1, 2): the moving between steps; the probe's counts begin again
internal int collect_smooth_mode(int how) {
    if (!g_uc_installed) return -1;
    g_cs_u = NULL, g_cs_n = g_cs_zero = 0, g_cs_sum = g_cs_sq = 0;
    g_cp_smooth = how;
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
// picture collect fill game | check | own (0, 1, 2): the fills the walk calls - check holds the trees' and the
// doppelgangers' against the game's as they are filled; the objects' fill has none (it changes the game's lists before
// its record is done: the lists' print in a paused game holds the two against each other)
internal int collect_fill_mode(int how) {
    if (!g_uc_installed) return -1;
    g_uc_tr_n = g_uc_tr_diff = g_uc_dp_n = g_uc_dp_diff = g_uc_dp_yes = g_uc_gh_n = g_uc_gh_diff = g_uc_gh_calls = 0;
    g_uc_fill = how;
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
    if (g_uc_fill == 1)
        log_locked("# collect: trees' fill held against the game's: %u trees, %u differ; doppelgangers' %u (%u drawn), %u differ; ghosts' %u (%u fills), %u differ\n", g_uc_tr_n, g_uc_tr_diff,
                   g_uc_dp_n, g_uc_dp_yes, g_uc_dp_diff, g_uc_gh_n, g_uc_gh_calls, g_uc_gh_diff);
    LONG steps = g_cp_by_step, frames = g_cp_by_frame;
    log_locked("# collect: the copy (%s): %ld by the steps (%.2f ms each, %ld objects the last), %ld by the frame (%.2f ms each), %ld writes making it stale, %ld walks of the steps'; checked %ld (%ld objects), %ld differ\n",
               g_cp_on == 2 ? "check" : g_cp_on ? "on" : "off", (long)steps, steps ? (double)g_cp_step_qpc / rd_qpc_ms(1) / steps : 0.0,
               (long)g_cp_objs, (long)frames, frames ? (double)g_cp_frame_qpc / rd_qpc_ms(1) / frames : 0.0, (long)g_cp_stales,
               (long)g_cp_used, (long)g_cp_chk_n, (long)g_cp_chk_objs, (long)g_cp_chk_diff);
    if (g_uc_during_on) log_locked("# collect: the units' update during a step: %ld so far\n", (long)g_uc_during_walks);
    if (g_fx_on)
        log_locked("# collect: the effects' queues (%s): %ld made by the steps (%.3f ms each, %.1f effects, %ld finished ones passed over), %ld taken by frames during a step; checked %ld, %ld differ\n",
                   g_fx_on == 2 ? "check" : "on", (long)g_fx_builds, g_fx_builds ? (double)g_fx_qpc / rd_qpc_ms(1) / g_fx_builds : 0.0,
                   g_fx_builds ? (double)g_fx_queued / g_fx_builds : 0.0, (long)g_fx_dead, (long)g_fx_takes, (long)g_fx_chk_n, (long)g_fx_chk_diff);
    if (g_cp_smooth)
        log_locked("# collect: the moving between steps (%s): %u frames of a moving object, its step on the screen %.4f tiles on average, rms %.4f (%.2f times the average: 1 is even), %u frames it did not move\n",
                   g_cp_smooth == 1 ? "on" : "probe", g_cs_n, g_cs_n ? g_cs_sum / g_cs_n : 0.0, g_cs_n ? sqrt(g_cs_sq / g_cs_n) : 0.0,
                   g_cs_sum > 0 ? sqrt(g_cs_sq / g_cs_n) / (g_cs_sum / g_cs_n) : 0.0, g_cs_zero);
    if (g_uc_drawn == 1)
        log_locked("# collect: view test held against the game's: %u objects (%u drawn), %u differ\n", g_uc_dr_n, g_uc_dr_yes, g_uc_dr_diff);
}
