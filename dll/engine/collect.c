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
#include "collect_maps.c"   // the sort maps built at once after the walk
#include "collect_ground.c"   // the ground's height under what the fills place, kept

// STEP 4: THE MOVING BETWEEN STEPS (the pipe: picture collect smooth; collect_copy.c cp_pose_swap has the poses). A
// moving object's record drawn at the game time g_uc_show - the pieces' clock (pt_clock: running on with the real time, a
// step and a half behind the newest step) - between the two of its last three poses whose game times hold it. At or past
// its newest (a pause) it is the object as the game has it. Not across a jump (a unit set down or teleported: further than
// UC_SMOOTH_FAR tiles), and a turn of more than about 25 degrees only moved, not turned: the matrices are blended by their
// numbers, and a big turn would shrink the model on the way. Blended from the last copy but one to the last by the real
// time since the last over the time between the two (until 2026-11-10) the units stood whenever a step came late, and
// when two came at once (their copies 0.2 ms apart: the way done at once) until the next, 12 frames in a recording at a
// step load of 30 ms (the player: "on heigh stepload its stuttering").
#define UC_SMOOTH_FAR 16.0f   // tiles, squared
internal double g_uc_show = -1.0;   // the walk's game time (< 0: drawn as the game has them)
typedef struct { const float *t; int32_t anim, frame, next; float fraction; } uc_pose_v;   // a pose and its animation
// the two poses of an object round g_uc_show and the way between them: 0 none to blend (drawn as it is)
internal int uc_between(const uc_obj *o, uc_pose_v *f, uc_pose_v *to, float *a) {
    double d = g_uc_show;
    if (!o->has_was || g_cp_smooth != 1 || d < 0.0 || d >= o->ms[0]) return 0;
    uc_pose_v now = { (const float *)o->pm, o->anim, o->anim_frame, o->next_frame, o->fraction },
              was = { o->was, o->was_anim, o->was_frame, o->was_next, o->was_fraction },
              was2 = { o->was2, o->was2_anim, o->was2_frame, o->was2_next, o->was2_fraction };
    if (d >= o->ms[1]) *f = was, *to = now, *a = (float)((d - o->ms[1]) / (o->ms[0] - o->ms[1]));
    else if (o->has_was2 && d >= o->ms[2]) *f = was2, *to = was, *a = (float)((d - o->ms[2]) / (o->ms[1] - o->ms[2]));
    else *f = *to = o->has_was2 ? was2 : was, *a = 0.0f;   // further back than its poses: its oldest
    return 1;
}
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
    uc_pose_v f, to;
    float a;
    int moved = o->has_was && (t[3] != o->was[3] || t[7] != o->was[7] || t[11] != o->was[11]);
    if (uc_between(o, &f, &to, &a)) {
        const float *p = f.t, *q = to.t;
        float dx = q[3] - p[3], dy = q[7] - p[7], dz = q[11] - p[11];
        if (q != t) memcpy(t, q, 12 * sizeof *t);   // a pose before its newest: from there
        if (dx * dx + dy * dy + dz * dz <= UC_SMOOTH_FAR) {
            float len = q[0] * q[0] + q[4] * q[4] + q[8] * q[8], dot = q[0] * p[0] + q[4] * p[4] + q[8] * p[8];
            if (dot >= 0.9f * len)
                for (int i = 0; i < 12; i++) { if (q[i] != p[i]) t[i] = p[i] + (q[i] - p[i]) * a; }   // (an unchanged -0 stays -0: a still unit byte for byte)
            else
                for (int i = 3; i < 12; i += 4) { if (q[i] != p[i]) t[i] = p[i] + (q[i] - p[i]) * a; }
        }
    }
    if (!g_cp_smooth) return;
    if (!g_cs_u && moved) g_cs_u = u, g_cs_have = 0, g_cs_still = 0;
    if (u == g_cs_u) g_cs_seen = 1, g_cs_now_x = t[3], g_cs_now_y = t[7], g_cs_moving = moved;
}
// FACING THE WAY ([display] units_face_travel, with units_smooth; the player, 2026-11-11: tanks marching down a walled
// lane "all point to the middle where they want to be ... more a visual issue than one that affects the game"): a unit
// steers at its place, and one pushed off it by the others moves with them while it points at it. Drawn, its model is
// turned about the upright by the angle from its heading to the way its poses moved (the newest against the oldest
// kept, up to two steps back: the push and the walk together), eased over UF_EASE_S of real time and faded in between
// UF_SPEED_MIN and UF_SPEED_FULL tiles a second - a unit pressed at rest or turning on the spot keeps its heading -, none
// past UF_OFF_MAX (backing off a push, a turn the move hasn't caught up with). The record only: the simulation's heading
// (its shots, its steering) is untouched, so every PC still plays the same game. The ease is per unit in a small table
// the walk keeps (only the walk reads or writes it).
#define UF_UNITS 4096
#define UF_EASE_S 0.35f
#define UF_SPEED_MIN 0.15f   // tiles a second
#define UF_SPEED_FULL 0.4f
#define UF_OFF_MAX 1.31f     // radians: 75 degrees
typedef struct { ee_unit *u; int32_t serial; float off; } uf_e;
internal uf_e g_uf[UF_UNITS];
internal uint32_t g_uf_gen;     // the walk the ease's time was taken for
internal float g_uf_k;          // this walk's share of the way to the target (from the real time since the last)
internal LARGE_INTEGER g_uf_at;
internal void uc_face_travel(const float *now, float *t, const uc_obj *o) {
    if (!g_set.units_face_travel || !o->has_was || o->rotting || o->dead || o->foundation) return;
    if (g_uf_gen != g_uc_walk_gen) {   // the walk's first: its share of the ease
        LARGE_INTEGER q, f;
        QueryPerformanceCounter(&q);
        QueryPerformanceFrequency(&f);
        float dt = g_uf_at.QuadPart ? (float)((double)(q.QuadPart - g_uf_at.QuadPart) / (double)f.QuadPart) : 0.0f;
        if (dt > 0.25f) dt = 0.25f;
        g_uf_k = 1.0f - expf(-dt / UF_EASE_S);
        g_uf_at = q;
        g_uf_gen = g_uc_walk_gen;
    }
    const float *old = o->has_was2 ? o->was2 : o->was;
    double ms = o->ms[0] - o->ms[o->has_was2 ? 2 : 1];
    float dx = now[3] - old[3], dy = now[7] - old[7], fx = t[1], fy = t[5], d2 = dx * dx + dy * dy, target = 0.0f;
    if (ms > 0.0 && d2 <= UC_SMOOTH_FAR && (fx != 0.0f || fy != 0.0f)) {
        float speed = sqrtf(d2) * 1000.0f / (float)ms, w = (speed - UF_SPEED_MIN) / (UF_SPEED_FULL - UF_SPEED_MIN);
        float off = atan2f(fx * dy - fy * dx, fx * dx + fy * dy);   // from the heading to the way, anticlockwise
        if (w > 0.0f && fabsf(off) <= UF_OFF_MAX) target = off * (w > 1.0f ? 1.0f : w);
    }
    uint32_t h = ((uint32_t)(uintptr_t)o->u * 2654435761u) % UF_UNITS, i = h;
    uf_e *e = NULL;
    for (int k = 0; k < 8 && !e; k++, i = (h + (uint32_t)k) % UF_UNITS)
        if (g_uf[i].u == o->u && g_uf[i].serial == o->serial) e = &g_uf[i];
    if (!e) {
        if (target == 0.0f) return;   // straight on, and not turned before
        for (int k = 0; k < 8 && !e; k++) if (!g_uf[(h + (uint32_t)k) % UF_UNITS].u) e = &g_uf[(h + (uint32_t)k) % UF_UNITS];
        if (!e) e = &g_uf[h];   // full there: the slot taken over (its unit eases from 0 again)
        e->u = o->u, e->serial = o->serial, e->off = 0.0f;
    }
    e->off += (target - e->off) * g_uf_k;
    if (target == 0.0f && fabsf(e->off) < 0.002f) { e->u = NULL; return; }
    float c = cosf(e->off), s = sinf(e->off);
    for (int j = 0; j < 3; j++) {   // turned about the world's upright: the rotation's rows x and y
        float x = t[j], y = t[4 + j];
        t[j] = c * x - s * y;
        t[4 + j] = s * x + c * y;
    }
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
    uc_pose_v from, to;
    float alpha;
    // a pose before its newest in another animation than the record's: as it is (its frames are another stretch)
    if (!g_uc_anim_smooth || !uc_between(o, &from, &to, &alpha) || to.anim != o->anim) return;
    ee_model *m = ((ee_model **)dmg_model_anim_model_at(o->gfx))[o->anim];
    int32_t first = ((const int32_t *)dmg_model_anim_first_at(o->gfx))[o->anim], last = ((const int32_t *)dmg_model_anim_last_at(o->gfx))[o->anim];
    int32_t n = last - first + 1;
    if (!m || n < 2 || first < 0 || (uint32_t)last >= model_get_frames(m) || to.frame < first || to.frame > last) return;
    *frame = to.frame, *next = to.next, *fraction = to.fraction;
    if (from.anim != to.anim || from.frame < first || from.frame > last) return;
    // the larger of the two: at the animation's end the game's next frame stops at the last, a smaller step than the pose's
    int32_t s = uc_anim_stride(to.frame, to.next, n), sw = uc_anim_stride(from.frame, from.next, n);
    if (sw > s) s = sw;
    int32_t poses = (n + s - 1) / s, pose = (to.frame - first) / s;
    float at = (float)((from.frame - first) / s) + from.fraction, by = (float)pose + to.fraction - at;
    if (by < 0.0f) by += (float)poses;
    if (by <= 0.0f || by > (float)poses * 0.5f) return;
    float p = at + by * alpha;
    if (p >= (float)poses) p -= (float)poses;
    int32_t k = (int32_t)p, f = first + k * s;
    *fraction = p - (float)k;
    *frame = f;
    // past the last frame: as the game's own next frame there says - the first in a loop, itself where the animation ends and
    // stays (a death's: blended toward its first frame it went back to its beginning for a moment and came back)
    *next = f + s <= last ? f + s : k == pose ? to.next : first;
}
// THE FLYING PIECES, THE FLAMES AND THE SMOKE BETWEEN STEPS (with the animation's: picture collect anim): what a dying mech
// throws - a Minotaur's three pieces, the shards - is no part of its model but an effect (EEE3DParticle), in no copy of
// the world, so it stood for a step and jumped (the player, 2026-11-10: "the death animation of a minotaurus cyborg, its still in single frames"). Its update
// moves it whenever the game time has gone on, by the time gone, and stamps it with that time (moved_ms): once a step. Its
// draw reads three plain things, the place, the angles and the fade: drawn as they were at a game time of their own
// (pt_clock), blended between the two of its last moves whose stamps hold it. Blended by the real time between its changes
// (39d8ee4) it kept the steps' unevenness; at the units' time (8c70134) it stood whenever a step came late and leapt when
// two came at once - at the normal speed a step is about 23 ms, the simulation runs up to two at a time, and a late one
// stood the piece for one to three frames: "the flying piece still dosnt move smoth" (the player, 2026-11-10). That clock
// runs on with the real time at the rate the game time goes (g_cp_rate) and is drawn toward a step and a half behind the
// newest game time, never past it: a step late, or two at once, only bends its pace. The units are drawn at the same
// time (uc_between). The flames and smoke (EEE2DParticle, sprites) are made and moved the same way, by their place, their
// fade (size and alpha) and their turn, and stood the same: at a quarter of the game's speed they changed every third
// frame of a recording (the player, 2026-11-10: "do Flames and smoke first"). A burning field holds hundreds at once:
// the table is looked up by the pointer over 8 slots, a slot not seen for a quarter of a second free again.
#define PT_SLOTS 16384   // a power of two
#define PT_PROBE 8
#define PT_MOVES 4
typedef struct { void *o; LONGLONG seen; int n; float ms[PT_MOVES], v[PT_MOVES][7]; } pt_e;   // its last moves, the newest last
internal pt_e g_pt[PT_SLOTS];
internal LONG g_pt_evicted;   // entries taken from a piece still drawn (the table too small)
internal double g_pt_show_ms = -1.0;   // the pieces' game time; < 0 none (drawn as the game has them)
internal LONGLONG g_pt_show_at;
internal LONG g_pt_frame = -1;
internal void pt_clock(void) {   // once a frame
    LONG f = g_rd_frames;
    if (f == g_pt_frame) return;
    g_pt_frame = f;
    EnterCriticalSection(&g_cp_cs);
    double newest = g_cp_clock, step = g_cp_step_ms, rate = g_cp_rate;
    LONGLONG at = g_cp_clock_at;
    LeaveCriticalSection(&g_cp_cs);
    LONGLONG now = rd_qpc();
    double per_ms = (double)rd_qpc_ms(1), aim = newest + (double)(now - at) / per_ms * rate - 1.5 * step, t = g_pt_show_ms;
    if (!at || !(rate > 0.0) || !(step > 0.0)) t = -1.0;
    else if (t < 0.0 || fabs(aim - t) > 6.0 * step + 100.0) t = aim;   // the first, or the time gone elsewhere (a load)
    else {
        double real = (double)(now - g_pt_show_at) / per_ms, k = real / 150.0, was = t;
        t += real * rate;
        t += (aim - t) * (k < 1.0 ? k : 1.0);
        if (t < was) t = was;
    }
    if (t > newest) t = newest;
    g_pt_show_ms = t, g_pt_show_at = now;
}
// the entry of piece o: its own, else one free (or the longest unseen of the probed), emptied
internal pt_e *pt_entry(void *o, LONGLONG now) {
    uint32_t h = ((uint32_t)(uintptr_t)o >> 4) * 2654435761u;
    pt_e *take = NULL;
    for (uint32_t i = 0; i < PT_PROBE; i++) {
        pt_e *e = &g_pt[(h + i) & (PT_SLOTS - 1)];
        if (e->o == o) return e;
        if (!take || e->seen < take->seen) take = e;
    }
    if (take->o && now - take->seen < rd_qpc_ms(250)) g_pt_evicted++;
    take->o = o, take->n = 0;
    return take;
}
// a piece's values (n of them: its place's three, its turn's, its fade last) at the pieces' time into vals: 1 when they
// are blended (the caller draws with them and puts cur back), 0 drawn as it is
internal int pt_blend(void *o, float ms, float *vals, int n, const float *cur) {
    if (!g_uc_anim_smooth || g_cp_smooth != 1 || !g_cp_cs_ok) return 0;
    pt_clock();
    LONGLONG t = g_pt_show_at;   // (the frame's: a counter read a piece was 55 ns, 14 000 pieces a frame after a battle)
    pt_e *e = pt_entry(o, t);
    // one before it in this memory: unseen for a while, its time gone back (a load) or its fade (its age: it only grows)
    // smaller - the game makes a new flame in the memory of one just gone, and blended from that one's last moves it was
    // drawn swept and grown across the gap between them for a step (a recording at a quarter of the speed)
    if (e->n && (t - e->seen > rd_qpc_ms(250) || ms < e->ms[e->n - 1] || cur[n - 1] < e->v[e->n - 1][n - 1])) e->n = 0;
    e->seen = t;
    if (!e->n || ms != e->ms[e->n - 1]) {
        if (e->n == PT_MOVES) memmove(e->ms, e->ms + 1, sizeof e->ms - sizeof e->ms[0]), memmove(e->v, e->v + 1, sizeof e->v - sizeof e->v[0]), e->n--;
        e->ms[e->n++] = ms;
    }
    memcpy(e->v[e->n - 1], cur, (size_t)n * sizeof *cur);   // (changed without a move: a load)
    double d = g_pt_show_ms;
    if (d < 0.0 || d >= e->ms[e->n - 1]) return 0;
    int i = e->n - 2;
    while (i >= 0 && e->ms[i] > d) i--;
    float *p = e->v[i < 0 ? 0 : i], *q = e->v[i < 0 ? 0 : i + 1], span = i < 0 ? 0.0f : e->ms[i + 1] - e->ms[i];
    if (span > 2000.0f) return 0;
    float a = span > 0.0f ? (float)((d - e->ms[i]) / span) : 0.0f, dx = q[0] - p[0], dy = q[1] - p[1];
    if (dx * dx + dy * dy > 16.0f) return 0;   // not the same flight
    for (int k = 0; k < n; k++) vals[k] = p[k] + (q[k] - p[k]) * a;
    for (int k = 3; k < n - 1; k++) if (fabsf(q[k] - p[k]) > 1.6f) vals[k] = q[k];   // an angle come round
    return 1;
}
internal uint32_t __thiscall pt_hook_draw(ee_particle3d *o) {
    float *place = particle3d_place_at(o), *fade = particle3d_fade_at(o), *angles = particle3d_angles_at(o), cur[7], b[7];
    memcpy(cur, place, 12); memcpy(cur + 3, angles, 12); cur[6] = *fade;
    if (!pt_blend(o, particle3d_get_moved_ms(o), b, 7, cur)) return eee3_d_particle_draw(o);
    memcpy(place, b, 12); memcpy(angles, b + 3, 12); *fade = b[6];
    uint32_t r = eee3_d_particle_draw(o);
    memcpy(place, cur, 12); memcpy(angles, cur + 3, 12); *fade = cur[6];
    return r;
}
internal uint32_t __thiscall pt_hook_draw2d(ee_particle2d *o) {
    float *place = particle2d_place_at(o), *fade = particle2d_fade_at(o), *angle = particle2d_angle_at(o), cur[5], b[5];
    memcpy(cur, place, 12); cur[3] = *angle, cur[4] = *fade;
    if (!pt_blend(o, particle2d_get_moved_ms(o), b, 5, cur)) return eee2_d_particle_draw(o);
    memcpy(place, b, 12); *angle = b[3], *fade = b[4];
    uint32_t r = eee2_d_particle_draw(o);
    memcpy(place, cur, 12); *angle = cur[3], *fade = cur[4];
    return r;
}
// THE MODELS' FRAME TRANSFORMATIONS BETWEEN FRAMES (with the animation's: picture collect anim): a model moves by its
// vertices per frame, which the engine blends by next_frame and fraction, and by a transformation per frame (GEModel
// frame_transforms), which PrepareTransforms takes from the frame alone. A part that hangs on a model flies by the
// latter: a Minotaur's top thrown off in its death rose 0.64, 1.19, 1.77 and turned half over in frames 13 to 20 - it
// stood for a frame and jumped a tile to the next, and drawn by its poses (uc_anim_between: that model's vertices hold
// each pose for two frames) for two (the player, 2026-11-10, at a quarter of the game's speed: "the pieces jump back and
// fort, still move only in large steps"; a recording at 0.25: the top a tile off in one frame, then still for 100). The
// exe calls PrepareTransforms through one import slot: here it does the same with the frame's transformation blended
// to the next one's - the turn by its quaternion, the scale and the place straight - at the place in the animation the
// frame, next frame and fraction give (forward: the fraction counted from the pose's first frame, between the two frames
// round that place; back to a loop's start: the two by the fraction). Counted from the frame itself, the game's own
// values of a frame the walk had none for (frame 14 at 0.55 where the walk draws 13 at 0.55) put the top a frame ahead
// and back: a jump there and back in two frames of the recording.
internal model_prepare_fn g_mt_orig;
internal transform_mul_fn g_mt_apply;
internal void mt_quat(const float r[9], float q[4]) {   // a turn's 3 x 3 (by row) as a quaternion (w, x, y, z)
    float t = r[0] + r[4] + r[8];
    if (t > 0.0f) { float s = sqrtf(t + 1.0f) * 2.0f; q[0] = 0.25f * s; q[1] = (r[7] - r[5]) / s; q[2] = (r[2] - r[6]) / s; q[3] = (r[3] - r[1]) / s; }
    else if (r[0] > r[4] && r[0] > r[8]) { float s = sqrtf(1.0f + r[0] - r[4] - r[8]) * 2.0f; q[0] = (r[7] - r[5]) / s; q[1] = 0.25f * s; q[2] = (r[1] + r[3]) / s; q[3] = (r[2] + r[6]) / s; }
    else if (r[4] > r[8]) { float s = sqrtf(1.0f + r[4] - r[0] - r[8]) * 2.0f; q[0] = (r[2] - r[6]) / s; q[1] = (r[1] + r[3]) / s; q[2] = 0.25f * s; q[3] = (r[5] + r[7]) / s; }
    else { float s = sqrtf(1.0f + r[8] - r[0] - r[4]) * 2.0f; q[0] = (r[3] - r[1]) / s; q[1] = (r[2] + r[6]) / s; q[2] = (r[5] + r[7]) / s; q[3] = 0.25f * s; }
}
internal void mt_blend(const float *a, const float *b, float k, float *out) {   // 4 x 4 by row, the place in the fourth column
    float sa[3], sb[3], ra[9], rb[9], qa[4], qb[4], q[4];
    for (int c = 0; c < 3; c++) {
        sa[c] = sqrtf(a[c] * a[c] + a[4 + c] * a[4 + c] + a[8 + c] * a[8 + c]);
        sb[c] = sqrtf(b[c] * b[c] + b[4 + c] * b[4 + c] + b[8 + c] * b[8 + c]);
        if (sa[c] < 1e-6f || sb[c] < 1e-6f) {   // no turn to take out: straight
            for (int i = 0; i < 16; i++) out[i] = a[i] + (b[i] - a[i]) * k;
            return;
        }
        for (int r = 0; r < 3; r++) ra[r * 3 + c] = a[r * 4 + c] / sa[c], rb[r * 3 + c] = b[r * 4 + c] / sb[c];
    }
    mt_quat(ra, qa), mt_quat(rb, qb);
    float d = qa[0] * qb[0] + qa[1] * qb[1] + qa[2] * qb[2] + qa[3] * qb[3], wa = 1.0f - k, wb = k;
    if (d < 0.0f) d = -d, wb = -wb;   // the short way round
    if (d < 0.9995f) {
        float th = acosf(d), sn = sinf(th);
        wa = sinf((1.0f - k) * th) / sn, wb = (wb < 0.0f ? -1.0f : 1.0f) * sinf(k * th) / sn;
    }
    float n = 0.0f;
    for (int i = 0; i < 4; i++) q[i] = qa[i] * wa + qb[i] * wb, n += q[i] * q[i];
    n = 1.0f / sqrtf(n);
    for (int i = 0; i < 4; i++) q[i] *= n;
    float w = q[0], x = q[1], y = q[2], z = q[3];
    float r[9] = { 1 - 2 * (y * y + z * z), 2 * (x * y - w * z), 2 * (x * z + w * y),
                   2 * (x * y + w * z), 1 - 2 * (x * x + z * z), 2 * (y * z - w * x),
                   2 * (x * z - w * y), 2 * (y * z + w * x), 1 - 2 * (x * x + y * y) };
    for (int c = 0; c < 3; c++) {
        float s = sa[c] + (sb[c] - sa[c]) * k;
        for (int rr = 0; rr < 3; rr++) out[rr * 4 + c] = r[rr * 3 + c] * s;
    }
    for (int rr = 0; rr < 3; rr++) out[rr * 4 + 3] = a[rr * 4 + 3] + (b[rr * 4 + 3] - a[rr * 4 + 3]) * k;
    for (int i = 12; i < 16; i++) out[i] = a[i];
}
// the first frame of the pose frame f is in: the frames before it that share its vertices (a model that holds each pose
// for several frames - the game's fraction is the way from that frame on)
internal uint32_t mt_pose_first(ee_model *m, uint32_t f) {
    ee_vertices **v = model_get_vertices(m);
    while (v && f > 0 && v[f] && v[f - 1] == v[f]) f--;
    return f;
}
// the frame's transformation blended toward the next one's at fraction fr: ft's own or x
internal const float *mt_frame(ee_model *m, uint32_t f, uint32_t nx, float fr, float *x) {
    const float *ft = model_get_frame_transforms(m), *src = ft + f * 16;
    if (fr > 0.0f && nx != f) {
        uint32_t a = f, b = nx;
        float k = fr;
        if (nx > f) {   // the place in the animation: from the pose's first frame (the game's frame 14 at 0.55 of a pose
                        // that holds 13 and 14 is where 13 at 0.55 is - both come: the game's own values and the walk's)
            uint32_t a0 = mt_pose_first(m, f), b0 = mt_pose_first(m, nx);
            if (b0 <= a0) a0 = f, b0 = nx;
            float t = (float)a0 + fr * (float)(b0 - a0);
            a = (uint32_t)t;
            if (a < a0) a = a0;
            if (a >= b0) a = b0 - 1;
            b = a + 1, k = t - (float)a;
        }
        if (memcmp(ft + a * 16, ft + b * 16, 16 * sizeof(float))) mt_blend(ft + a * 16, ft + b * 16, k, x), src = x;
        else src = ft + a * 16;
    }
    return src;
}
// the frame's transformation PrepareTransforms takes (through mt_hook_prepare) for frame, next frame and fraction:
// blended while that is on, else the frame's own (render_units.c: a record's, without the model set to them)
internal const float *mt_frame_used(ee_model *m, uint32_t f, uint32_t nx, float fr, float *x) {
    if (!g_uc_anim_smooth || g_cp_smooth != 1 || !g_mt_apply) return model_get_frame_transforms(m) + f * 16;
    return mt_frame(m, f, nx, fr, x);
}
internal void mt_prepare(ee_model *m) {   // PrepareTransforms (engine 0x317F1) with the frames' transformations blended
    float x[16], y[16];
    const float *src = mt_frame(m, model_get_frame(m), model_get_next_frame(m), model_get_fraction(m), x);
    g_mt_apply(src, model_own_at(m), y);
    float s = model_get_scale(m);
    for (int r = 0; r < 3; r++) for (int c = 0; c < 3; c++) y[r * 4 + c] *= s;
    y[12] = y[13] = y[14] = 0.0f, y[15] = 1.0f;
    g_mt_apply(model_outer_at(m), y, model_transform_at(m));
    ee_model **ch = model_get_children(m);
    for (uint32_t i = 0, n = model_get_child_count(m); i < n; i++) mt_prepare(ch[i]);
}
internal void __thiscall mt_hook_prepare(ee_model *m) {
    if (!g_uc_anim_smooth || g_cp_smooth != 1 || !g_mt_apply || !model_get_frame_transforms(m)) { g_mt_orig(m); return; }
    mt_prepare(m);
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

internal double g_uc_clock = -1.0, g_uc_now = -1.0;   // the walk's copy's game time and its own (-1: no walk of a copy)
#include "collect_remains.c"   // the remains kept, sunk and drawn after the game lets them go

// STEP 2, THE FILLS OURS: THE OPACITY (EEROUnits.opacity, the AoC decompilation's GetUnitOpacity: EE 0x5044D7, AoC
// 0x50F412 with its cloak part in a function of its own, 0x63CEE2). An object's opacity into *out, and whether it is
// drawn see-through (its record into the see-through sort map): "show hidden units" (1: an obstacle or a moved unit
// hidden behind something, 2: every obstacle) at hidden_opacity; a dying unit fading as it rots; a cloakable unit by its
// own opacity, dimmed to an enemy; and a land building faded as it comes near the camera's front plane. Ported from
// the disassembly, its floats as the FPU has them: long double between the stores the game's code makes (a float
// there), in the same order - the game's precision whatever the FPU is set to on the render thread. Of the object's
// copy (collect_copy.c: what the game reads of the unit and its complex unit), the camera the frame's.
internal int uc_cloak(const uc_obj *o, int see, float *out, const opacity_consts *c) {
    if (o->rotting && g_set.remains_sink && !uc_game_sinks(o->ty)) {   // whole while it sinks, then faded out; a kind without a rot time none (UC_NO_CORPSE)
        *out = o->rot_t > 0 ? uc_dead_opacity(uc_dead_ms(o)) : 0.0f;
        return see;
    }
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
        t[11] = uc_mesh_z(map_mesh_at(game_map()), t[3], t[7]);
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
    float z = uc_mesh_z(map_mesh_at(game_map()), x, y);
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
            unit_type_is_land_building(o->ty) ? SIL_BUILDING : o->has_cu && !o->tree && o->owner_index && !o->rotting ? SIL_UNIT : 0);
    // (a corpse no silhouette - the player, 2026-11-11: "dead units should no longer have silluetes")
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
        if (!unit_type_get_faces_camera(cty)) uc_face_travel((const float *)o->pm, draw_record_transform_at(rec), o);
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
        pos[2] = uc_mesh_z(mesh, pos[0], pos[1]);
        if (o->marker && o->tcu) {   // a path marker on its target (world_unit by its hit points)
            second = o->tcu;
            second_foundation = o->t_foundation;
            if (o->marker_ok) {
                pos[0] = o->at[0], pos[1] = o->at[1], pos[2] = o->at[2];
                if (mesh_has_tile(mesh, (int)floorf(pos[0]), (int)floorf(pos[1]))) pos[2] = uc_mesh_z(mesh, pos[0], pos[1]);
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
    int ship = cu && g_set.remains_sink && g_set.remains_physics && o->cu_hp <= 0 && uc_is_ship(o->ty);   // (uc_ship_*)
    if (cu && (o->rotting || ship) && g_set.remains_sink && (ship || !uc_game_sinks(o->ty))) {   // after the ground's height under it: that stays the ground's
        double d = uc_dead_ms(o);
        if (o->rot_t > 0 || ship) uc_corpse_saw(c, o, rec, t, d, ship);
        if (!ship) uc_sink_at(t, o->gfx, o->anim, o->anim_frame, o->cu_height, d);
    }
    uc_pick_add(u, rec, draw_record_get_opacity(rec));
    void *pair[2] = { (void *)(uintptr_t)graphics_anim_key(draw_record_get_gfx(rec), draw_record_get_anim(rec)), rec };
    uc_map_insert(sort_map, pair);
    if (second && !second_foundation) {
        pair[0] = (void *)(uintptr_t)dmg_model_get_shadow_key(draw_record_get_gfx(rec));
        pair[1] = rec;
        uc_map_insert(engine_display_sort_maps_at() + 0x18, pair);
    }
}

// THE TREES' FILL (EEROUnits' FillObject: EE 0x501DFF, AoC 0x50CBA3 - the same code but for addresses and offsets, and
// EE hands only a wood tree to the ambient sounds). A tree's small record: at its tile's middle shifted by its id, the
// ground's height there, turned to the camera's yaw; its hit points; its scale varied by its id; its type's two bar
// values when a diagonal neighbour tile has no tree (the copy's open); its selection circle and the trees' health bars'
// list while it is the player's selected unit; its sway in the wind by the clock and its id (a model of more than one
// frame); then into the sort map by its graphics' key, the picker (its transformation) and the sixth sort map. (The
// game makes a physical model on its stack it never uses: left out.) dry: 1 only the record (the check), 2 for the trees'
// own list (render_units.c: the ambient sounds' and the picker's entries, no user of its graphics taken - the list holds
// one -, in no sort map)
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
    pos[2] = uc_mesh_z(map_mesh_at(map), pos[0], pos[1]);
    __attribute__((aligned(8))) float t[16];
    transform_orient(t, info->yaw, 0.0f, 0.0f);
    transform_set_position(t, pos[0], pos[1], pos[2]);
    t[12] = t[13] = t[14] = 0.0f, t[15] = 1.0f;
    if (dry != 1 && info->ambient && tree_has_ambient(ty)) ambient_add_tree(engine_display_ambient_sounds_at(), o->u);
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
    if (dry == 1) return;
    if (dry == 2) { uc_pick_add(o->u, t, draw_small_record_get_opacity(rec)); return; }
    void *pair[2] = { (void *)(uintptr_t)graphics_anim_key(draw_small_record_get_gfx(rec), draw_small_record_get_anim(rec)), rec };
    uc_map_insert(sort_map, pair);
    uc_pick_add(o->u, t, draw_small_record_get_opacity(rec));
    pair[0] = (void *)(uintptr_t)dmg_model_get_shadow_key(draw_small_record_get_gfx(rec));
    pair[1] = rec;
    uc_map_insert(engine_display_sort_maps_at() + 0x3C, pair);
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
        pos[2] = uc_mesh_z(map_mesh_at(map), pos[0], pos[1]);
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
            void *pair[2] = { (void *)(uintptr_t)graphics_anim_key(draw_record_get_gfx(rec), 0), rec };
            uc_map_insert(sort_map, pair);
            pair[0] = (void *)(uintptr_t)dmg_model_get_shadow_key(draw_record_get_gfx(rec));
            pair[1] = rec;
            uc_map_insert(engine_display_sort_maps_at() + 0x18, pair);
            ++*count;
        }
    }
    physical_model_free(pm);
    return drawn;
}
internal int uc_fill_dopp_tree(const uc_dopp *p, uint8_t *rec, uint32_t *count, void *sort_map, uint32_t room, int x,
                               int y, uc_info *info, int dry) {
    // (dry 2: the trees' own list's - the picker's and the ambient sounds' entries, no sort map, no user of its
    // graphics (the list holds one), no view test: the list is made only while every model passes it - shr_hook_view)
    opacity_consts oc = opacity_consts_now();
    tree_consts tc = tree_consts_now();
    fill_consts fc = fill_consts_now();
    ee_map *map = game_map();
    ee_unit_type *ty = p->ty;
    __attribute__((aligned(8))) uint8_t pm[0x60];
    __attribute__((aligned(8))) float t[16];
    if (dry != 2) physical_model_new(pm);
    float h = unit_type_get_unknown_42c(ty);
    int id = p->serial, size = unit_type_get_size(ty);
    float *pos = (float *)draw_small_record_pos_at(rec);
    pos[0] = (float)((long double)((id % 3 - 1) * size) * tree_shift() + (long double)size * oc.half + x);
    pos[1] = (float)((long double)((id / 3 % 3 - 1) * size) * tree_shift() + (long double)unit_type_get_size(ty) * oc.half + y);
    pos[2] = uc_mesh_z(map_mesh_at(map), pos[0], pos[1]);
    transform_orient(t, info->yaw, 0.0f, 0.0f);
    t[3] = pos[0], t[7] = pos[1], t[11] = pos[2];
    t[12] = t[13] = t[14] = 0.0f, t[15] = 1.0f;
    int drawn = *count < room && (dry == 2 || uc_dopp_view(p, pm, t, h, 3));
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
        if (dry == 2) {
            if (info->ambient && tree_has_ambient(ty)) ambient_add_tree(engine_display_ambient_sounds_at(), p->u);
            uc_pick_add(p->u, t, draw_small_record_get_opacity(rec));
            ++*count;
        } else if (!dry) {
            if (info->ambient && tree_has_ambient(ty)) ambient_add_tree(engine_display_ambient_sounds_at(), p->u);
            void *pair[2] = { (void *)(uintptr_t)graphics_anim_key(draw_small_record_get_gfx(rec), 0), rec };
            uc_map_insert(sort_map, pair);
            uc_pick_add(p->u, t, draw_small_record_get_opacity(rec));
            pair[0] = (void *)(uintptr_t)dmg_model_get_shadow_key(draw_small_record_get_gfx(rec));
            pair[1] = rec;
            uc_map_insert(engine_display_sort_maps_at() + 0x3C, pair);
            ++*count;
        }
    }
    if (dry != 2) physical_model_free(pm);
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

// THE TREES' OWN LIST (render_units.c g_tl): whether this walk lists trees there - the mod's walk, test, opacity and
// fill, the trees kept, the mod's tree draw, every model let through the view's test (the shadows' casters off the screen
// collected: this walk's rows widened), no free camera - and the list emptied (its graphics' users given back)
internal void tl_begin(void) {
    tl_clear();
    g_tl.on = 0;
    if (!tl_on() || ud_mode() == 0 || g_uc_fill != 2 || g_uc_drawn != 2 || g_uc_opacity != 2 || *game_free_camera_at()) return;
    if (g_shr.n <= 0 || !g_shr.on || g_shr.off || !g_sh.on || !draw_list_moved()) return;
    if (!g_tl.rec) {
        g_tl.rec = malloc((size_t)TL_MAX * DRAW_SMALL_REC);
        g_tl.key = malloc(TL_MAX * sizeof *g_tl.key);
        g_tl.order = malloc(TL_MAX * sizeof *g_tl.order);
        if (!g_tl.rec || !g_tl.key || !g_tl.order) { free(g_tl.rec); free(g_tl.key); free(g_tl.order); g_tl.rec = NULL; return; }
    }
    g_tl.on = 1;
    g_tl.want[0] = g_tl.want[1] = 1;
    g_tl.walks++;
}
// a tree of the walk's tile ct for the list: 1 listed, 0 not drawn (as uc_drawn tests a tree with every model let
// through: the tile in the player's sight, a scale), -1 the game's list's as before, -2 that and the anchor. Of one tile
// (else the walk's set of the met), not selected nor with its health bar shown (the game's list of the trees' bars holds
// the record), not seen through (the trees' second map); its opacity before the walk's mark, which the game's path tests
internal int tl_tree(uc_obj *o, const uc_ctile *ct, uc_info *info, int during) {
    if (o->food || unit_type_get_size(o->ty) != 1) return -1;
    ee_player *me = game_players()[local_player_index()];
    if (o->u == player_get_selected_unit(me) || uc_in_bar_ids(o->serial)) return -1;
    if (!ct->seen_me) return 0;
    float sc = o->height;
    if (!(sc < 0.0f || sc > 0.0f)) return 0;
    if (during && o->walked == g_uc_walk_gen) return 0;
    float op;
    if (uc_opacity(o, &op)) return -1;
    if (g_tl.want[0]) { g_tl.normal++; return -2; }
    if (g_tl.n >= TL_MAX || !tl_hold(o->gfx)) { g_tl.full++; return -1; }
    o->walked = g_uc_walk_gen;
    uint8_t *rec = g_tl.rec + (size_t)g_tl.n * DRAW_SMALL_REC;
    memset(rec, 0, DRAW_SMALL_REC);
    draw_small_record_set_opacity(rec, op);
    uc_fill_tree(o, rec, NULL, info, 2);
    g_tl.key[g_tl.n++] = graphics_anim_key(o->gfx, 0);
    g_tl.listed++;
    return 1;
}
// a tree the player remembers on a tile out of sight, of one tile, for the list: 1 taken (listed, the remembered
// trees' anchor, or not drawn), 0 the game's list's as before. Filled as the game's (uc_fill_dopp_tree, the list's mode);
// the walk's first is the anchor, filled into the game's list
internal int tl_dopp(const uc_dopp *p, uint8_t *small, uint32_t *sn, uint32_t small_room, int x, int y, uc_info *info) {
    if (g_tl.want[1]) {
        uint8_t *rec = small + (size_t)*sn * DRAW_SMALL_REC;
        uint32_t n0 = *sn;
        uc_fill_dopp_tree(p, rec, sn, engine_display_sort_maps_at() + 0x24, small_room, x, y, info, 0);
        if (*sn != n0) g_tl.anchor[1] = rec, g_tl.want[1] = 0, g_tl.normal++;
        return 1;
    }
    if (g_tl.n >= TL_MAX || !tl_hold(p->gfx)) { g_tl.full++; return 0; }
    uint8_t *rec = g_tl.rec + (size_t)g_tl.n * DRAW_SMALL_REC;
    memset(rec, 0, DRAW_SMALL_REC);
    uint32_t n = g_tl.n;
    if (!uc_fill_dopp_tree(p, rec, &n, NULL, TL_MAX, x, y, info, 2)) return 1;
    g_tl.key[g_tl.n] = graphics_anim_key(p->gfx, 0);
    g_tl.n = n;
    g_tl.listed++, g_tl.fogged++;
    return 1;
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
    g_uc_show = -1.0;
    if (c && g_cp_smooth == 1 && g_cp_cs_ok) pt_clock(), g_uc_show = g_pt_show_ms;
    g_uc_clock = c ? c->clock : -1.0;   // the corpses' clock (uc_dead_ms)
    g_uc_now = g_uc_show >= 0.0 ? g_uc_show : g_uc_clock;
    tl_begin();   // (after the rows: whether they were widened)
    for (unsigned long r = 0; c && r < nrows; r++) {
        long y, x0, x1;
        w.row(mesh, r, &y, &x0, &x1);
        for (long x = x0; x <= x1; x++) {
            uc_ctile *ct = &c->tiles[(uint32_t)((y - c->y0) * c->w + (x - c->x0))];
            if (!ct->t || (!ct->explored && !free_cam)) continue;
            for (uint32_t k = 0; k < ct->n_obj; k++) {
                uc_obj *o = &c->objs[c->seq[ct->obj_at + k]];
                int tl = o->tree && g_tl.on ? tl_tree(o, ct, (uc_info *)info, during) : -1;   // the trees' own list
                if (tl >= 0) continue;
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
                        if (tl == -2 && !see) g_tl.anchor[0] = rec, g_tl.want[0] = 0;
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
                else if (!g_tl.on || unit_type_get_size(p->ty) != 1 || !tl_dopp(p, small, sn, small_room, (int)x, (int)y, (uc_info *)info))
                    uc_fill_dopp_of(ro, p, small + (size_t)*sn * DRAW_SMALL_REC, sn, maps + 0x24, small_room, (int)x, (int)y, info, 1);
            }
        }
    }
    if (c) uc_corpses_walk(c, recs, n, room, maps, free_cam);
    g_uc_clock = g_uc_now = -1.0;
    if (c && g_cp_on == 2 && !during) cp_check(c);
    uc_smooth_probe();
    g_uc_walked_at = c ? c->at : 0, g_uc_walked_alpha = g_uc_show >= 0.0 && g_uc_show < g_cp_clock ? 0.0f : 1.0f;   // still moving on
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
    um_walk_begin();
    collect_walk(f);
    um_walk_end();
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
    InitializeCriticalSection(&g_kc_cs);
    g_kc_cs_ok = 1;
    g_cp_cs_ok = 1;
    g_uc_lock_at = (uintptr_t)engine_display_picker_lock_at();
    g_uc_end = (uintptr_t)engine_display_units_walk_end_at();
    uint8_t want[6] = { 0x8B, 0x0D };   // mov ecx, [picker_lock]
    memcpy(want + 2, &g_uc_lock_at, 4);
    g_uc_installed = g_uc_enter && g_uc_leave && patch_insn6_call((uintptr_t)engine_display_units_walk_at(), want, collect_thunk);
    int ghosts = g_uc_installed && patch_fill_ghosts(uc_hook_fill_ghosts), fx_of = 0;
    if (!patch_particle3d_draw(pt_hook_draw) && g_log) fprintf(g_log, "# collect: the flying pieces' draw NOT hooked\n");
    if (!patch_particle2d_draw(pt_hook_draw2d) && g_log) fprintf(g_log, "# collect: the flames' and smoke's draw NOT hooked\n");
    g_mt_apply = transform_mul_export();
    if ((!g_mt_apply || !patch_prepare_transforms(mt_hook_prepare, &g_mt_orig)) && g_log)
        fprintf(g_log, "# collect: the models' frame transformations NOT blended (PrepareTransforms not hooked)\n");
    if (g_uc_installed) patch_scene_release(fx_hook_scene_release, &fx_of);
    if (g_uc_installed) collect_maps_install();
    if (g_uc_installed && !(patch_effects_step(us_hook_effects_step) && patch_air_crash_effect(us_hook_crash_effect) &&
                            patch_heli_crash_effect(us_hook_crash_effect) && patch_air_crash_splash(us_hook_crash_splash) &&
                            patch_heli_crash_splash(us_hook_crash_splash) && patch_unit_remove_effect(us_hook_remove_effect)) && g_log)
        fprintf(g_log, "# collect: the wrecks' effects NOT hooked (the game's splash and crash where it has them)\n");
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
    if (g_cp_smooth == 1 && g_uc_anim_smooth)
        log_locked("# collect: the pieces, flames and smoke between steps: %ld times a piece still drawn lost its moves (the table full)\n", (long)g_pt_evicted);
    if (g_cp_smooth)
        log_locked("# collect: the moving between steps (%s): %u frames of a moving object, its step on the screen %.4f tiles on average, rms %.4f (%.2f times the average: 1 is even), %u frames it did not move\n",
                   g_cp_smooth == 1 ? "on" : "probe", g_cs_n, g_cs_n ? g_cs_sum / g_cs_n : 0.0, g_cs_n ? sqrt(g_cs_sq / g_cs_n) : 0.0,
                   g_cs_sum > 0 ? sqrt(g_cs_sq / g_cs_n) / (g_cs_sum / g_cs_n) : 0.0, g_cs_zero);
    if (g_uc_drawn == 1)
        log_locked("# collect: view test held against the game's: %u objects (%u drawn), %u differ\n", g_uc_dr_n, g_uc_dr_yes, g_uc_dr_diff);
}
