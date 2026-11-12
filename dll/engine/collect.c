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

// THE DEAD SINKING ([display] corpses_sink, corpses_lie_seconds, corpses_sink_seconds; the player, 2026-11-11: "dead units
// next, how about letting them slowly sink into the ground instead of the fafing out?", then "the games remains fade out
// times are weirdly mixed, lets try uniform times: 30s where they just lay, 2m where they linelary sink into the ground").
// The game fades a corpse over its type's rot time - rot_ms set to it at the death, a step's ms taken off each step (the
// rotting state's updates), the object removed at 0: a Clubman's 40 s - and the opacity's check tells the rotting apart
// from the game's then. Here every corpse lies as it fell for corpses_lie_seconds of game time (the death's animation
// plays then), goes down at an even pace over corpses_sink_seconds by UC_SINK_DEPTH of its model's radius, and fades over
// the last UC_FADE_MS (what a slope or a tall wreck leaves out of the ground); whole until then. The radius is the
// standing unit's (a Clubman's 0.70 tiles with its scale), and one lying dead shows hardly 0.06 over the ground: sunk 0.1
// of it, it is under shortly before the end (by twice the radius over 55 % of its 40 s it was gone in 2 s). Its age at the
// walk's game time: its type's rot time less rot_ms is its age at the copy's clock, the time from there to the walk's added
// (the walk shows the game a step and a half behind: it goes down between steps too).
#define UC_SINK_DEPTH 0.1
#define UC_FADE_MS 5000.0
internal double g_uc_clock = -1.0, g_uc_now = -1.0;   // the walk's copy's game time and its own (-1: no walk of a copy)
internal double uc_lie_ms(void) { return g_set.corpses_lie_seconds * 1000.0; }
internal double uc_end_ms(void) { return (g_set.corpses_lie_seconds + g_set.corpses_sink_seconds) * 1000.0; }
internal double uc_dead_ms(const uc_obj *o) {   // a rotting object's age at the walk's game time
    double d = (double)o->rot_t - o->rot_ms;
    if (g_uc_now >= 0.0 && g_uc_clock >= 0.0) d += g_uc_now - g_uc_clock;
    return d < 0.0 ? 0.0 : d;
}
internal float uc_dead_opacity(double d) {   // how much of a corpse is seen at its age
    double end = uc_end_ms();
    return d >= end ? 0.0f : d <= end - UC_FADE_MS ? 1.0f : (float)((end - d) / UC_FADE_MS);
}
// a corpse's pose t lowered as far as its age has taken it
internal void uc_sink_at(float *t, ee_dmg_model *gfx, int32_t anim, int32_t frame, float scale, double d) {
    double q = (d - uc_lie_ms()) / (g_set.corpses_sink_seconds * 1000.0);
    if (q <= 0.0 || !gfx) return;
    ee_model *m = ((ee_model **)dmg_model_anim_model_at(gfx))[anim];
    if (!m) return;
    if (q > 1.0) q = 1.0;
    t[11] = (float)(t[11] - UC_SINK_DEPTH * model_get_radii(m)[frame] * scale * q);
}
// THE CORPSES KEPT: one the game removes before its time is up (most: a Clubman's after 40 s) is kept here - its last
// record's pose, graphics, animation and parts - and drawn by the walk where the player sees, the way the remembered
// buildings are (uc_corpses_walk), until it has sunk; the game's graphics kept by a user of ours meanwhile (the last
// user's release frees the models: every Clubman gone, the Clubman's models with them). An entry is made when the walk
// meets a rotting object; one not met by a walk was removed when its rot_ms was nearly done at its last sight
// (UC_GONE_ROT: more than a step's ms), else it went out of the view (let go: made anew when met again). Met again (the
// game's still), it is the game's again. Emptied at the scene's release (a game's end, a load: fx_hook_scene_release).
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
#define UC_GONE_ROT 2000
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
                             // tailplane's (nose up), its fin's (a yaw) - us_wings
    float along[3], across[3];  // the hull's own axes bow-ward and port-ward
    float leak[4][3];        // each float's leak: its buoyancy in the end (g's, fully under), from when, over how long (s)
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
    uint8_t in_game;         // the game still has its unit (the effects' step, by its id): not met by a walk, it is out of
                             // the view by the game's copy, not removed (us_alive_known)
    float fx_at[2][3];       // where: the splash's (on the water), the crash's
    double sim;              // the game time it is simulated to (ms)
} uc_corpse;
internal uc_corpse g_kc[UC_CORPSES];
internal uint32_t g_kc_n, g_kc_full, g_kc_drawn;
internal CRITICAL_SECTION g_kc_cs;
internal int g_kc_cs_ok;
internal int32_t uc_kc_n(uint32_t n) { return (int32_t)(n < UC_KC_PARTS ? n : UC_KC_PARTS); }
// THE SHIPS SINKING ([display] ship_sinking, with corpses_sink; the player, 2026-11-11: "i wonderd if we could make sinking
// ships more intretsing. maybe 4 points (one front, one back, and 2 in the middle to keep it upright) - each point has a
// buoyancy, 1 to 3 point get a negative buoyancy ... ships that sink similar to now, ships that fall over etc", the
// outcomes weighted, a wreck half out of the water in the shallows welcome). The game sinks every ship alike - it moves
// the unit itself, its animation still: turned about one axis to 36 degrees in 2 s, down at 0.28 tiles a second to
// 1.3 tiles under, 2 s still rotting, gone (logged 2026-11-11, a frigate: state 6000, then the rotting 6002 with a rot
// time of 2 s). Here a dead ship (its hit points at 0) is drawn by four floats of its own from its first sight on: bow and
// stern 0.8 of the way to its hull's ends along its longer side (the frame's bounding box, GEModel boxes), port and
// starboard as far to its sides, at its waterline; a rigid frame (their six distances held) under gravity, each float
// lifted by its buoyancy times how deep it lies (0 dry, 1 fully under over range: half under at rest), slowed by the
// water's drag, and stopped by the seabed under it at its keel's depth. Their buoyancy leaks away: all of them slowly to
// less than their weight (it goes down in the end), and by the plan of its serial (the same on every PC, the picture only)
// one or two fast to below nothing - a dive by the bow or the stern half the time (now and then with a list), a roll onto
// one side a quarter (the other side holding on longest), level a quarter. Stepped by the game time (stands while paused),
// US_DT a step; kept after the game removes it as the other corpses, its time theirs from its death. Each float goes into
// the seabed by a depth of its own, up to US_DIG of the beam by the same plan (the player, the same day: "each point
// should have a random value how deep it can sink into the ground"): a wreck comes to rest unevenly buried, tilted.
// It goes on at the speed it had when it died (the player: "also let the unit keep its velocity on death"): the floats
// start with its last step's move along the water (us_velocity), the drag slows it, the seabed stops it.
// A fifth point, its ballast (the player: "a thried point in the middle that a bit lower than the others with a heigher
// wieght, to simulate a real ships balance better"): under its middle at US_LOW of the keel, US_BALLAST times a float's
// weight, no buoyancy of its own - the floats carry it (their lift times all the weight over theirs: half under at rest
// as before, the leaks' shares of the weight as before). Its weight below the floats rights a heeling hull; the frame's
// distances are held by the points' weights (the heavy one moves the least). Heeled past US_SHIFT the ballast breaks
// loose and slides to the low side over US_SHIFT_S, as a cargo shifts, and the hull stays over: held in the middle it
// rights every rolled ship as it fills (tried: all ten of a test came to rest upright, their sails out of the water).
#define US_PTS 5        // the four floats and the ballast (the last)
#define US_LOW 0.6f     // the ballast under the waterline: a share of the keel's depth
#define US_BALLAST 3.0f // its weight, a float's 1
#define US_SHIFT 0.8f   // the heel (its sine: 53 degrees) past which the ballast slides
#define US_SHIFT_S 1.5f // ... over this long (s), 0.8 of the way to the low side
// The blow that kills it (the player, the same day: "on death an units should get a bit of random rotation for a start.
// even if it levels out again, the kick is suposed to make clearer something happend"): it starts turning about its
// own axes, half to all of these (radians/s) by its serial, either way - the buoyancy and the drag level it out again
#define US_KICK_ROLL 0.8f    // about its length: a heel of up to about 25 degrees
#define US_KICK_PITCH 0.35f  // about its beam
#define US_KICK_YAW 0.3f     // about its up: slewed round
// an aircraft's (the player: "helicopters get a strong yaw kick, and airplanes an roll kick"): nothing in the air holds it
// back, the turn goes on until it hits - a helicopter spins down, an airplane rolls over
#define US_KICK_HELI_YAW 1.0f    // a start: its spin builds up as it falls (US_HELI_TWIST; 4 until 2026-11-12)
#define US_KICK_PLANE_ROLL 2.5f
#define US_KICK_AIR_OTHER 0.2f   // its other two axes
#define US_G 3.0f       // gravity (tiles/s^2)
// an aircraft's (the player, the same day: "reduce grafithy to 1 tile/s"): it falls slower, its spin or roll and its
// planing have longer to show before it hits; its float in the water scaled alike (buoyancy is lift x gravity)
#define US_G_AIR 1.0f
#define US_DT 0.01      // the step (s of game time)
// The water's drag by the hull's own axes (the player, the same day: "what do you think of adding a lift plane? ... real
// ships glide when they sink right?"): a hull slips along its length and is held across it - sideways, and through its
// deck and keel - so one going down by the bow or the stern glides on along its line instead of dropping (the Titanic's
// bow came to rest about 600 m from its stern), one that sinks level goes straight down, and its way at its death
// carries it on where its bow points (a slew of the kick doesn't drift it sideways). Until then by the world's axes:
// 0.5 along the water, 1.6 up and down. The air's alike every way.
#define US_DRAG 1.6f    // the water's across the hull (1/s), along its length, the air's
#define US_ALONG 0.35f  // a frigate at 0.8 tiles/s at its death coasts about 2.3 tiles
// Its turning held harder still (the player: "the plane should heavely discurage quick rotation underwater right?"):
// a turning hull sweeps its whole flanks through the water, its ends the most - its points' way about their middle
// (by their weights; the way of the whole kept) slowed by US_TURN times how much of it is under. With the drag
// alone a leaking bow pivoted a frigate on end in about a second, and a diver at a slant now stays slanted longer.
#define US_TURN 8.0f    // 1/s, wholly under (4 hardly slowed the pivots)
// A hull under way lifts its leading end (the player: "how about an slight upwards rotation scaled by speed?"): as a
// boat planes, the water pushes the end it moves toward up and the other down - a pair, nothing lifted as a whole -
// by its way along its length, the bow going ahead, the stern going astern, as far as each end is under. A ship
// killed under way rears a little and settles as it slows; a dive's glide is flattened a little.
#define US_RISE 3.0f    // the push (tiles/s^2) per tile/s: about 5 degrees at a frigate's 0.8 tiles/s
#define US_RISE_TOP 2.0f   // tiles/s: no more than this counted (a ship of 170 rears, it doesn't flip)
// AN AIRPLANE'S WINGS AND FINS in the air (the player, the same day: "lets disable the plaing for planes in the air,
// underwater it could make for a nice dive"; then "keep the middle point as a coliider only, no lift. add 4 new points,
// two for each horizontal wing and one for the yaw wing. they are towards the back of the plane. the 4 point is a pure
// weight placed in the front", and "2 extra point for the main wings? along the center of weight? the ideas is that the
// wing points can be disabled or weakend randomly, to simulate failing wings"). Each surface point cancels its own way
// through its surface (its up: length x span; the fin's: its span) as far as it is dry - each point on its own, so the
// surfaces turn it: a dart, its weight ahead of its fins, turns its nose into its way; it falls as a thing thrown, its
// nose following the curve. The wings are its side floats (at the middle of its length: no pitch of their own), so a
// sinking plane is held up and noses over slowly - a glide that steepens; a weak wing drops, it rolls toward it and
// spirals in; both whole hold a roll back (the kick's levels out). No planing in the air (in the water as a ship's).
#define US_PTS_AIR 9    // the floats, the ballast; the tailplane's port and starboard points, the fin's, the nose weight
#define US_WING 16.0f   // a wing point's (1/s, wholly in the air)
#define US_TAILPLANE 12.0f
#define US_FIN 12.0f
// Its surfaces meet the air at angles of their own (the player, the same day: "no plane started rotating, are you sure
// the brocken wings work?" - a surface that only cancels the way across it pushes nothing once the tail has turned the
// nose into the way, so a weak wing made no difference after its first second; then "maybe disabeling/softening wings is
// the wrong lever, your (8 degrees nose-up) ... add a random number ... the same for the tailfins, instead of just not
// doing their job when failing they stear the plane. pitch the plane up and stall it, yaw it into a trun"): each
// pushes by the speed along it, across its tilted surface. Each wing between 8 degrees front up and 6 front down (by
// its serial, us_wings): two alike glide, two apart roll it; the tailplane pitches its nose up (a stall: it rears and
// sinks, slowed by its wings across the way) or down, the fin yaws it into a turn
#define US_WING_UP 8.0f     // degrees, the most front up
#define US_WING_DOWN 6.0f   // ... front down
#define US_TAIL_TURN 20.0f  // the tailplane's, the most either way (8 hardly lifted a nose: they die at about 1 tile/s)
#define US_FIN_TURN 15.0f
// A HELICOPTER'S FALL (the player, 2026-11-12: "now lets improve the helicopters, bougancy, some other ideas?", then
// "yes do 1- 4": it floats a little and rolls over, its rotor holds it, its spin builds up, its rotors turn until it
// hits). Six points: the floats, its ballast high - its engine and rotor over its cabin: top-heavy, a helicopter in
// the water rolls over and floats on its back before it goes, as ditched ones do - and its rotor's hub at its top.
// Its floats carry it briefly (US_HELI_FLOAT, leaking at US_HELI_LEAK of a ship's times). The hub's way through the
// rotor's disc is slowed by US_ROTOR times its hold, 0 .. 1 by its serial (autorotating down slowly, or dropping), and
// a little every way (held at its top it falls upright, a shuttlecock); the disc tilted up to US_DISC_TILT any way
// drifts it off sideways as it sinks.
// Its main rotor's torque, no tail rotor to hold it, turns it about its up the more the longer it falls (US_HELI_TWIST,
// the same way for all: their rotors turn alike), and its rotors turn on until it hits or is wet - all of it in the air
// only. Its rotors are its graphics' turned
// models, an Apache's the first values of its list a (logged: the main rotor's +7 radians/s, the tail rotor's -3.5,
// no parts): falling dry, they are the game's own while it is met, each one's rate measured, and on by those rates
// while it isn't (out of the view, or taken off where the game's copy lands); held from its hit or its first wet on
#define US_PTS_HELI 6       // the floats, the ballast (high), the rotor's hub
#define US_HELI_FLOAT 1.6f  // buoyancy, fully under (g's)
#define US_HELI_LEAK 0.3f   // its leaks' start and length: a share of a ship's (afloat a few seconds)
#define US_HELI_HIGH 0.6f   // the ballast at this share of its height over its floats
#define US_ROTOR 12.0f      // 1/s, a whole hold
#define US_ROTOR_SIDE 3.0f  // 1/s, the hub's every way, a whole hold
#define US_ROTOR_W 2.0f     // the hub's weight, a float's 1
#define US_DISC_TILT 12.0f  // degrees, the most
#define US_HELI_TWIST 1.5f  // radians/s^2: about 4.5 radians/s after 3 s
#define US_TAIL_SPAN 0.35f   // the tailplane's half span: a share of the wings' (a P-51's: 4 m to 11 m)
#define US_FIN_W 0.5f   // the tail's points' weights, a float's 1
#define US_NOSE 2.0f    // the nose weight's
#define US_AIR 0.3f
#define US_FLOAT 2.0f   // a float's buoyancy, fully under (g's): half under at rest
#define US_SUB_FLOAT 1.0f   // a submarine's: it hangs at its depth (all under) until the leaks take it down
#define US_DIG 0.6f     // the deepest a float goes into the seabed: a share of the beam
#define US_DIG_AIR 0.1f // ... an aircraft's into the ground (a flat thing: 0.6 buried it whole)
// an aircraft's ground collider up in its body (us_step's end; the player, 2026-11-12: "the ground collider should be
// placed a bit heigher, so that upright crashes sink in a bit and upside ones dont completly vanish"): at the floats -
// its model's foot - an upright one stopped on its skids or belly, untouched, and one on its back went in to its
// floats, its whole body buried
#define US_HIT_HIGH 0.35f   // of its model's height over its foot
#define US_FAST 1.5f    // a move between its poses faster than this times its top speed (at least US_JUMP tiles/s)
#define US_JUMP 4.0f    // was no sailing but a jump: a ship of 170 tiles/s stopped dead at a fixed 4 (the player)
// the ships on the water and the submarines (the player, the same day: "next add subs to the system"): a submarine's
// floats at its hull's middle height, neutral (US_SUB_FLOAT) - it hangs at its depth until the leaks take it down
internal int uc_is_aircraft(int k) {
    return k == CLS_AIRPLANE || k == CLS_HELICOPTER || k == CLS_HELICOPTER_TRANSPORT || k == CLS_HELICOPTER_PRIEST;
}
// ... and the aircraft (the player: "i want helicopters and airplanes to use the same system, with the difrence betwen
// water and air being just the drag. aiplanes mayb could also have boyancy that only drains once they hit water, so
// they raise up a bit before sinking"). The game glides a plane shot down to the ground over about 3.5 s (its dying
// state 6000, on forward), then it lies as a wreck for 30 s; a helicopter it lets down at about a tile a second and
// removes at the ground (rot time 0; logged 2026-11-11, P-51, B-17, Apache). Here it falls from where it was hit
// with its way and the kick, through the air's light drag, onto the ground or into the water: an airplane floats there
// as a ship does, its leaks' clock from its first touch of the water (it comes up before it goes down), a helicopter
// has no buoyancy. Its length is its model's y (the way it faces; a fighter is wider than long), a tilt is no sign of
// its going down (it banks in flight), and it digs into the ground by US_DIG_AIR of its span at most.
internal int uc_is_ship(const ee_unit_type *ty) {
    int k = unit_type_get_class_id(ty);
    return class_is_ship(k) || k == CLS_SUBMERGED_NAVAL || uc_is_aircraft(k);
}
internal float us_rnd(uint32_t *s) { *s = *s * 1664525u + 1013904223u; return (float)(*s >> 8) / 16777216.0f; }
// a point's weight (a float's 1): the ballast, a helicopter's hub, an airplane's tail points and nose weight
internal float us_w(const uc_corpse *k, int i) {
    return i < 4 ? 1.0f : i == 4 ? US_BALLAST : k->heli ? US_ROTOR_W : i == 8 ? US_NOSE : US_FIN_W;
}
// a helicopter's rotor by its serial (a stream of its own, hashed): its hold even 0 .. 1, its disc's tilt any way
internal void us_rotor(uc_corpse *k, int32_t serial) {
    uint32_t s = (uint32_t)serial ^ 0x27D4EB2Fu;
    s ^= s >> 16, s *= 0x7FEB352Du, s ^= s >> 15, s *= 0x846CA68Bu, s ^= s >> 16;
    k->rotor[0] = us_rnd(&s);
    float dir = 6.2831853f * us_rnd(&s), a = sinf(US_DISC_TILT * us_rnd(&s) * 3.14159265f / 180.0f);
    k->rotor[1] = a * cosf(dir), k->rotor[2] = a * sinf(dir);
}
// a held turn for the record (kind: 0 a part's, 1 list a's, 2 list b's first value): a falling helicopter's on by its rate
internal uint32_t us_turn(const uc_corpse *k, int kind, int32_t i, uint32_t held) {
    if (!k->ship || !k->heli || k->turn_ms <= 0.0) return held;
    float f;
    uint32_t r;
    memcpy(&f, &held, 4);
    f = fmodf(f + k->turn_rate[kind][i] * (k->spin_t - k->turn_at), 6.2831853f);
    memcpy(&r, &f, 4);
    return r;
}
// ... the game's turn now into held, its rate from the one held dms ms before (the shorter way round)
internal void us_turn_seen(float *rate, uint32_t *held, uint32_t now, double dms) {
    float a, b;
    memcpy(&a, held, 4), memcpy(&b, &now, 4);
    if (dms > 0.0 && dms < 500.0) {
        float d = b - a;
        d -= 6.2831853f * floorf(d / 6.2831853f + 0.5f);
        *rate = d / (float)(dms / 1000.0);
    }
    *held = now;
}
// an airplane's surfaces' angles by its serial (a stream of their own, its seed hashed), even over their ranges
internal void us_wings(uc_corpse *k, int32_t serial) {
    uint32_t s = (uint32_t)serial ^ 0x165667B1u;   // mixed whole: ids one apart drew nearly alike by a product alone
    s ^= s >> 16, s *= 0x7FEB352Du, s ^= s >> 15, s *= 0x846CA68Bu, s ^= s >> 16;
    const float rad = 3.14159265f / 180.0f;
    for (int i = 0; i < 2; i++) k->tilt[i] = sinf((-US_WING_DOWN + (US_WING_UP + US_WING_DOWN) * us_rnd(&s)) * rad);
    k->tilt[2] = sinf(US_TAIL_TURN * (2.0f * us_rnd(&s) - 1.0f) * rad);
    k->tilt[3] = sinf(US_FIN_TURN * (2.0f * us_rnd(&s) - 1.0f) * rad);
}
// its leaks by the plan of its serial; late: met already sinking (come into the view): at their end at once
internal void us_leaks(uc_corpse *k, int32_t serial, int late) {
    uint32_t s = (uint32_t)serial * 2654435761u + 0x9E3779B9u;
    us_rnd(&s);
    float plan = us_rnd(&s);
    for (int i = 0; i < 4; i++)   // the slow leaks: each float to less than its weight
        k->leak[i][0] = 0.55f + 0.3f * us_rnd(&s), k->leak[i][1] = 1.0f + 2.0f * us_rnd(&s), k->leak[i][2] = 6.0f + 6.0f * us_rnd(&s);
    int fast = plan < 0.25f ? 0 : plan < 0.5f ? 1 : plan < 0.75f ? 2 + (us_rnd(&s) < 0.5f) : -1;   // bow, stern, a side, level
    if (fast >= 0) {
        k->leak[fast][0] = -0.6f - 0.6f * us_rnd(&s), k->leak[fast][1] = 0.2f * us_rnd(&s), k->leak[fast][2] = 1.5f + 2.0f * us_rnd(&s);
        if (fast >= 2) {   // the other side holds on longest: it rolls over before it goes
            int o = fast ^ 1;
            k->leak[o][0] = 0.8f + 0.15f * us_rnd(&s), k->leak[o][1] = 4.0f + 2.0f * us_rnd(&s), k->leak[o][2] = 10.0f + 8.0f * us_rnd(&s);
        } else if (us_rnd(&s) < 0.4f) {   // a dive with a list to one side
            int o = 2 + (us_rnd(&s) < 0.5f);
            k->leak[o][0] = 0.1f + 0.4f * us_rnd(&s), k->leak[o][1] = 0.5f + us_rnd(&s), k->leak[o][2] = 3.0f + 3.0f * us_rnd(&s);
        }
    }
    for (int i = 0; i < US_PTS; i++) k->dig[i] = us_rnd(&s);   // its share of US_DIG (drawn last: the leaks stay as they were)
    if (late) for (int i = 0; i < 4; i++) k->leak[i][1] = -1000.0f, k->leak[i][2] = 0.01f;
}
// its velocity along the water at its death (tiles/s) into v: its newest move between the poses the collection kept of
// it (the game may stop it in the step it dies: then the one before), none if it is met already sinking
internal void us_velocity(const uc_obj *o, float *v) {
    v[0] = v[1] = 0.0f;
    const float *now = (const float *)o->pm;
    float top = fmaxf(o->st ? unit_stats_get_speed(o->st) : 0.0f, o->cst ? unit_stats_get_speed(o->cst) : 0.0f);
    float fast = fmaxf(US_FAST * top, US_JUMP);
    for (int i = 0; i < 2; i++) {
        const float *a = i ? o->was2 : o->was, *b = i ? o->was : now;
        float dt = (o->ms[i] - o->ms[i + 1]) / 1000.0f;
        if (!(i ? o->has_was2 : o->has_was) || dt <= 0.0f || dt > 1.0f) return;
        float dx = (b[3] - a[3]) / dt, dy = (b[7] - a[7]) / dt, s = dx * dx + dy * dy;
        if (s > fast * fast) return;
        if (s > 1e-4f) { v[0] = dx, v[1] = dy; return; }
    }
}
// the kick at its death: each point's way a step behind by the turn about its own axes (the frame's as it starts,
// about its points' middle by their weights), random by its serial - a stream of its own, the leaks' plans as before
internal void us_kick(uc_corpse *k, int32_t serial) {
    uint32_t s = (uint32_t)serial * 2246822519u + 0x27D4EB2Fu;
    us_rnd(&s);
    float w[3], m[3] = { 0, 0, 0 }, al[3], ac[3], up[3], la = 0.0f, lc = 0.0f, lu = 0.0f, sum = 0.0f;
    static const float ship[3] = { US_KICK_ROLL, US_KICK_PITCH, US_KICK_YAW },
                       plane[3] = { US_KICK_PLANE_ROLL, US_KICK_AIR_OTHER, US_KICK_AIR_OTHER },
                       heli[3] = { US_KICK_AIR_OTHER, US_KICK_AIR_OTHER, US_KICK_HELI_YAW };
    const float *rate = !k->air ? ship : k->heli ? heli : plane;
    for (int a = 0; a < 3; a++) w[a] = rate[a] * (0.5f + 0.5f * us_rnd(&s)) * (us_rnd(&s) < 0.5f ? -1.0f : 1.0f);
    for (int j = 0; j < 3; j++) al[j] = k->sp[0][j] - k->sp[1][j], ac[j] = k->sp[2][j] - k->sp[3][j], la += al[j] * al[j], lc += ac[j] * ac[j];
    la = sqrtf(la), lc = sqrtf(lc);
    if (la < 1e-6f || lc < 1e-6f) return;
    for (int j = 0; j < 3; j++) al[j] /= la, ac[j] /= lc;
    up[0] = al[1] * ac[2] - al[2] * ac[1], up[1] = al[2] * ac[0] - al[0] * ac[2], up[2] = al[0] * ac[1] - al[1] * ac[0];
    for (int j = 0; j < 3; j++) lu += up[j] * up[j];
    lu = sqrtf(lu);
    if (lu < 1e-6f) return;
    float om[3];   // the turn in the world: about its length, its beam, its up
    for (int j = 0; j < 3; j++) om[j] = w[0] * al[j] + w[1] * ac[j] + w[2] * up[j] / lu;
    for (int i = 0; i < k->n; i++) {
        float wt = us_w(k, i);
        for (int j = 0; j < 3; j++) m[j] += wt * k->sp[i][j];
        sum += wt;
    }
    for (int i = 0; i < k->n; i++) {
        float r[3], dt = (float)US_DT;
        for (int j = 0; j < 3; j++) r[j] = k->sp[i][j] - m[j] / sum;
        k->sq[i][0] -= (om[1] * r[2] - om[2] * r[1]) * dt;
        k->sq[i][1] -= (om[2] * r[0] - om[0] * r[2]) * dt;
        k->sq[i][2] -= (om[0] * r[1] - om[1] * r[0]) * dt;
    }
}
// its floats set up from the game's pose t (rows; the model's scale apart) and its model's frame
internal void us_start(uc_corpse *k, const uc_obj *o, const float *t) {
    ee_model *m = ((ee_model **)dmg_model_anim_model_at(o->gfx))[o->anim];
    ee_box *box = m ? model_box(m, (uint32_t)o->anim_frame) : NULL;
    float r = m ? model_get_radii(m)[o->anim_frame] : 1.0f, s = o->cu_height;
    float lo[3] = { -0.9f * r, -0.3f * r, -0.2f * r }, hi[3] = { 0.9f * r, 0.3f * r, 0.5f * r };
    if (box) {
        vec3 a = box_get_min(box), b = box_get_max(box);
        if (b.x > a.x && b.y > a.y) lo[0] = a.x, lo[1] = a.y, lo[2] = a.z, hi[0] = b.x, hi[1] = b.y, hi[2] = b.z;
    }
    // a ship's floats at its waterline (its model's 0), a submarine's at its hull's middle height: its model stands on
    // its keel (0 .. 0.5 of its scale, logged), the game holds it 0.4 tiles under the water, wholly under
    int cls = unit_type_get_class_id(o->ty), sub = cls == CLS_SUBMERGED_NAVAL, air = uc_is_aircraft(cls);
    int x_long = !air && hi[0] - lo[0] >= hi[1] - lo[1];
    float fz = sub ? 0.5f * (lo[2] + hi[2]) * s : 0.0f;
    float c[3] = { 0.5f * (lo[0] + hi[0]) * s, 0.5f * (lo[1] + hi[1]) * s, fz };
    float half_long = 0.4f * (x_long ? hi[0] - lo[0] : hi[1] - lo[1]) * s, half_across = 0.4f * (x_long ? hi[1] - lo[1] : hi[0] - lo[0]) * s;
    float al[3] = { x_long ? 1.0f : 0.0f, x_long ? 0.0f : 1.0f, 0.0f }, ac[3] = { x_long ? 0.0f : -1.0f, x_long ? 1.0f : 0.0f, 0.0f };   // across: up x along
    memcpy(k->along, al, sizeof al), memcpy(k->across, ac, sizeof ac);
    for (int j = 0; j < 3; j++) {
        k->body[0][j] = c[j] + half_long * al[j], k->body[1][j] = c[j] - half_long * al[j];
        k->body[2][j] = c[j] + half_across * ac[j], k->body[3][j] = c[j] - half_across * ac[j];
    }
    k->keel = fminf(fmaxf(fz - lo[2] * s, 0.05f), 1.0f);
    k->body[4][0] = c[0], k->body[4][1] = c[1], k->body[4][2] = fz - US_LOW * k->keel;
    k->heli = (uint8_t)(air && cls != CLS_AIRPLANE), k->spin_t = 0.0f;
    k->n = cls == CLS_AIRPLANE ? US_PTS_AIR : k->heli ? US_PTS_HELI : US_PTS;
    if (k->heli) {   // its ballast high over its floats (its model stands on them), its hub at its top
        float top = fmaxf(hi[2] * s, 0.1f);
        k->body[4][2] = fz + US_HELI_HIGH * top;
        k->body[5][0] = c[0], k->body[5][1] = c[1], k->body[5][2] = fz + 0.95f * top;
        us_rotor(k, o->serial);
    }
    if (k->n == US_PTS_AIR) {   // the tail at the stern float, the fin over it, the nose weight at the bow float's height
        float tail = US_TAIL_SPAN * half_across, fin = fmaxf(0.6f * hi[2] * s, 0.2f * half_across);
        for (int j = 0; j < 3; j++) {
            k->body[5][j] = c[j] - half_long * al[j] + tail * ac[j], k->body[6][j] = c[j] - half_long * al[j] - tail * ac[j];
            k->body[7][j] = c[j] - half_long * al[j], k->body[8][j] = c[j] + 0.9f * half_long * al[j];
        }
        k->body[7][2] += fin;
        // its ballast at its wings' height: under them it hung from their lift as a pendulum and rolled a plane with a
        // broken wing level again (logged: banked to 40 degrees in a second, back to 25 by its crash)
        k->body[4][2] = fz;
        us_wings(k, o->serial);
    }
    k->lift = sub ? US_SUB_FLOAT : k->heli ? US_HELI_FLOAT : US_FLOAT;
    k->wet_at = air ? -1.0f : 0.0f;
    k->serial = o->serial, k->air = (uint8_t)air, k->fx_done = k->fx_due = k->stuck = 0, k->in_game = 1;
    k->hit = air ? US_HIT_HIGH * fmaxf(hi[2] * s, 0.0f) : 0.0f;
    k->shift = 0.0f, k->shift_to = 0;
    k->range = fminf(fmaxf(1.2f * half_across, 0.1f), 0.5f);
    k->zw = g_sh.water_on ? g_sh.level : t[11];
    for (int i = 0; i < k->n; i++)
        for (int j = 0; j < 3; j++) {
            const float *b = k->body[i];
            k->sp[i][j] = k->sq[i][j] = t[j * 4 + 3] + t[j * 4] * b[0] + t[j * 4 + 1] * b[1] + t[j * 4 + 2] * b[2];
        }
    int late = o->rotting || (!air && (t[10] < 0.98f || (!sub && t[11] < k->zw - 0.05f)));   // (a submarine is under anyway)
    memset(k->dig, 0, sizeof k->dig);
    us_leaks(k, o->serial, late);
    for (int i = 0; i < 4 && k->heli; i++) k->leak[i][1] *= US_HELI_LEAK, k->leak[i][2] *= US_HELI_LEAK;
    for (int i = 0; i < US_PTS; i++) k->dig[i] *= (air ? US_DIG_AIR : US_DIG) * 2.5f * half_across;   // the beam: half_across is 0.4 of it
    // never pushed up out of the bottom where the game had it - a submarine in the shallows (0.4 under the water) stood
    // in the sand and came up with its nose out: a point below its stop at the start digs in as deep as it was
    const void *mesh = game_map() ? map_mesh_at(game_map()) : NULL;
    for (int i = 0; i < US_PTS && mesh; i++) {
        const float *p = k->sp[i];
        if (!mesh_has_tile(mesh, (int)floorf(p[0]), (int)floorf(p[1]))) continue;
        float stop = mesh_z(mesh, p[0], p[1]) + (i == 4 ? 1.0f - US_LOW : 1.0f) * k->keel;   // upright: the keel under it
        if (p[2] < stop - k->dig[i]) k->dig[i] = stop - p[2];
    }
    float v[2] = { 0.0f, 0.0f };
    if (!late) us_velocity(o, v);
    for (int i = 0; i < k->n; i++) k->sq[i][0] -= v[0] * (float)US_DT, k->sq[i][1] -= v[1] * (float)US_DT;   // Verlet: a step's way behind
    if (!late) us_kick(k, o->serial);
    k->sim = k->died;
    k->ship = 1;
}
// THE EFFECTS OF ITS FALL (the player, the same day: "the explosion and water splash still get triggerd by the old system,
// smoke also dosnt follow the demaged unit"). The game crashes an aircraft by moving it itself (EEPMAirplane's and
// EEPMHelicopter's crash steps): it makes the splash where it comes down through the water and the death effect where it
// reaches the ground - at its own place and moment, while the wreck drawn here fell elsewhere and sooner; and every
// effect on a unit (the damage smoke's puffs, the fire, the trails, a ship's death effect) reads the unit's matrix at
// each of its steps. So the game's two calls in the crash steps are let out (us_hook_crash_*) while the wrecks are the
// mod's, the fall here makes them where it splashes (a float going down through the water over a tile under it) and
// where it hits the ground going down faster than US_CRASH_V (an aircraft killed standing on its airfield none), and
// around the effects' step (us_hook_effects_step) each wreck's unit has the wreck's pose for its matrix - the game's put
// back right after, so the simulation never sees it. The effects are the picture only (made or left out by each PC's
// detail setting, their own random numbers). Both made on the simulation's thread: the fall runs on the frame's, so it
// only notes them (fx_due).
#define US_FX_SPLASH 1
#define US_FX_CRASH 2
#define US_CRASH_V 0.5f   // tiles/s down
internal void us_fx(uc_corpse *k, uint8_t f, float x, float y, float z) {
    float *at = k->fx_at[f == US_FX_SPLASH ? 0 : 1];
    at[0] = x, at[1] = y, at[2] = z;
    k->fx_done |= f, k->fx_due |= f;
}
// AN AIRCRAFT'S CRASH (the player, the same day: "the settling on the floor like sunken ships after an crash is silly ...
// airplanes and helicopters have only ground collison on a single point in the middle, once that point hits ground its
// stuck. no air drag, only planing"): in the air nothing slows it, only the planing turns a helicopter (US_RISE: the end it
// moves toward up) and an airplane's wings and fins hold it (US_PTS_AIR); the ground holds only its middle (its floats' mean), and once that touches the ground or the seabed it stays
// as it hit - nose in, a wing down - no more steps. In the water it floats and is slowed as a ship is.
// one step at t s after its death: each float moved by its forces (Verlet: its velocity its way since the step before),
// then the frame's distances and the seabed held, four times round
internal void us_step(uc_corpse *k, float t, const void *mesh) {
    if (k->stuck) return;
    const float dt = (float)US_DT;
    float carry = 0.0f;   // a float's lift: its share of all the weight
    for (int i = 0; i < k->n; i++) carry += 0.25f * us_w(k, i);
    // the hull's length as it lies (from the stern to the bow): the way along it the water's drag spares
    float ln[3], ll = 0.0f;
    for (int j = 0; j < 3; j++) ln[j] = k->sp[0][j] - k->sp[1][j], ll += ln[j] * ln[j];
    ll = sqrtf(ll);
    for (int j = 0; j < 3; j++) ln[j] = ll > 1e-6f ? ln[j] / ll : 0.0f;
    const float f_across = expf(-US_DRAG * dt), f_along = expf(-US_ALONG * dt), f_air = k->air ? 1.0f : expf(-US_AIR * dt);
    float wet = 0.0f;   // how much of it is under: its floats' mean
    // its way along its length (the points' by their weights; + bow-ward), as far as it is counted
    float way = 0.0f, ws = 0.0f;
    for (int i = 0; i < k->n; i++) {
        float w = us_w(k, i);
        for (int j = 0; j < 3; j++) way += w * (k->sp[i][j] - k->sq[i][j]) * ln[j];
        ws += w;
    }
    way = fminf(fmaxf(way / (ws * dt), -US_RISE_TOP), US_RISE_TOP);
    // an airplane's surfaces' ups: its wings' and tailplane's (length x span), its fin's (its span)
    float sn[3], un[3], ls = 0.0f, lu = 0.0f;
    for (int j = 0; j < 3; j++) sn[j] = k->sp[2][j] - k->sp[3][j], ls += sn[j] * sn[j];
    ls = sqrtf(ls);
    for (int j = 0; j < 3; j++) sn[j] = ls > 1e-6f ? sn[j] / ls : 0.0f;
    un[0] = ln[1] * sn[2] - ln[2] * sn[1], un[1] = ln[2] * sn[0] - ln[0] * sn[2], un[2] = ln[0] * sn[1] - ln[1] * sn[0];
    for (int j = 0; j < 3; j++) lu += un[j] * un[j];
    lu = sqrtf(lu);
    for (int j = 0; j < 3; j++) un[j] = lu > 1e-6f ? un[j] / lu : 0.0f;
    // each surface's up tilted by its angle: a wing's front up (back), the tailplane's to pitch the nose up (forward: it
    // pushes the tail down), the fin's to yaw it
    float nw[4][3], cm[3] = { 0, 0, 0 }, cw = 0.0f, hub = 0.0f;   // hub: the rotor's push this step (tiles/step, along its up)
    if (k->heli) {   // its middle by the weights (the spin's axis through it)
        for (int i = 0; i < k->n; i++) {
            for (int j = 0; j < 3; j++) cm[j] += us_w(k, i) * k->sp[i][j];
            cw += us_w(k, i);
        }
        for (int j = 0; j < 3; j++) cm[j] /= cw;
    }
    if (k->n == US_PTS_AIR)
        for (int s = 0; s < 4; s++) {
            const float *base = s == 3 ? sn : un;
            float st = k->tilt[s] * (s == 2 ? 1.0f : -1.0f), ct = sqrtf(1.0f - k->tilt[s] * k->tilt[s]);
            for (int j = 0; j < 3; j++) nw[s][j] = ct * base[j] + st * ln[j];
        }
    for (int i = 0; i < k->n; i++) {
        float *p = k->sp[i], *q = k->sq[i], b = 0.0f, under = 0.5f + (k->zw - p[2]) / k->range;
        under = under < 0.0f ? 0.0f : under > 1.0f ? 1.0f : under;
        if (i < 4) wet += 0.25f * under;
        if (i < 4 && under > 0.0f && k->wet_at < 0.0f) k->wet_at = t;   // the leaks' clock starts
        if (i < 4 && k->lift > 0.0f) {
            float u = k->wet_at < 0.0f ? -1.0f : (t - k->wet_at - k->leak[i][1]) / k->leak[i][2];
            u = u < 0.0f ? 0.0f : u > 1.0f ? 1.0f : u * u * (3.0f - 2.0f * u);
            b = (k->lift + (k->leak[i][0] - k->lift) * u) * carry;
        }
        float v[3], a = 0.0f;
        for (int j = 0; j < 3; j++) v[j] = p[j] - q[j], a += v[j] * ln[j];
        // in the water its way along the hull kept by the light drag, the rest across it by the heavy one; in the air the
        // air's every way - between them by how wet the point is: wholly in the water from the waterline down (the hull
        // under it), wholly in the air half its range up (one wholly in the water as soon as it was wet, until 2026-11-11)
        float wd = fminf(2.0f * under, 1.0f);
        for (int j = 0; j < 3; j++) {
            float in_water = a * ln[j] * f_along + (v[j] - a * ln[j]) * f_across, in_air = v[j] * f_air;
            v[j] = in_air + (in_water - in_air) * wd;
        }
        float surf = k->heli ? (i == 5 ? US_ROTOR * k->rotor[0] : 0.0f)
                   : k->n != US_PTS_AIR ? 0.0f : i == 2 || i == 3 ? US_WING : i == 5 || i == 6 ? US_TAILPLANE : i == 7 ? US_FIN : 0.0f;
        if (surf > 0.0f && k->heli) {   // the rotor: its hub's way along its up slowed, a little every way (it hangs
            // under its rotor as under a shuttlecock's feathers: upright); the disc's tilt only drifts the whole (below) -
            // pushed by a tilted disc at its top it tumbled in the air (logged: on its side in 2 s)
            float c = 0.0f, f = expf(-surf * (1.0f - wd) * dt), fs = expf(-US_ROTOR_SIDE * k->rotor[0] * (1.0f - wd) * dt);
            for (int j = 0; j < 3; j++) c += v[j] * un[j];
            for (int j = 0; j < 3; j++) v[j] = (v[j] - (1.0f - f) * c * un[j]) * fs;
            hub = -(1.0f - f) * c;
        } else if (surf > 0.0f) {   // its way through its surface cancelled, as far as it is dry
            const float *nv = nw[i == 2 ? 0 : i == 3 ? 1 : i == 7 ? 3 : 2];
            float c = 0.0f, f = expf(-surf * (1.0f - wd) * dt);
            for (int j = 0; j < 3; j++) c += v[j] * nv[j];
            for (int j = 0; j < 3; j++) v[j] -= (1.0f - f) * c * nv[j];
        }
        float g = k->air ? US_G_AIR : US_G;
        v[2] += (-g + b * g * under) * dt * dt;
        if (k->heli) {   // the main rotor's torque about its up through its middle, as far as it is dry
            float r[3], tw = US_HELI_TWIST * (1.0f - wd) * dt * dt;
            for (int j = 0; j < 3; j++) r[j] = p[j] - cm[j];
            v[0] += tw * (un[1] * r[2] - un[2] * r[1]), v[1] += tw * (un[2] * r[0] - un[0] * r[2]), v[2] += tw * (un[0] * r[1] - un[1] * r[0]);
        }
        if (i < 2)   // the leading end up, the other down: in the water only (a helicopter's in the air too until 2026-11-12:
                     // spinning, its drift's way along it turned to and fro and rocked it over)
            v[2] += (i == 0 ? 1.0f : -1.0f) * US_RISE * way * under * dt * dt;
        for (int j = 0; j < 3; j++) q[j] = p[j], p[j] += v[j];
        if (k->air && i < 4 && !(k->fx_done & US_FX_SPLASH) && q[2] > k->zw && p[2] <= k->zw &&   // into the water
            mesh_has_tile(mesh, (int)floorf(p[0]), (int)floorf(p[1])) && mesh_z(mesh, p[0], p[1]) < k->zw)
            us_fx(k, US_FX_SPLASH, p[0], p[1], k->zw);
    }
    if (k->heli) {   // its rotors turn while it is dry; the disc's tilt drifts the whole by the rotor's push, sideways
        k->spin_t += dt * (1.0f - fminf(2.0f * wet, 1.0f));
        float dr = hub * US_ROTOR_W / cw;
        for (int i = 0; i < k->n; i++)
            for (int j = 0; j < 3; j++) k->sp[i][j] += dr * (k->rotor[1] * ln[j] + k->rotor[2] * sn[j]);
    }
    // its turning held: each point's way about the points' middle (by their weights) slowed, the whole's way kept
    float vm[3] = { 0, 0, 0 }, wsum = 0.0f, f_turn = expf(-US_TURN * wet * dt);
    for (int i = 0; i < k->n; i++) {
        float w = us_w(k, i);
        for (int j = 0; j < 3; j++) vm[j] += w * (k->sp[i][j] - k->sq[i][j]);
        wsum += w;
    }
    for (int i = 0; i < k->n; i++)
        for (int j = 0; j < 3; j++) {
            float m = vm[j] / wsum;
            k->sp[i][j] = k->sq[i][j] + m + (k->sp[i][j] - k->sq[i][j] - m) * f_turn;
        }
    // how far the hull reaches under each float as it lies now: the keel as far as it is upright, and under the bow and
    // the stern half its beam as far as it lies on its side (the side floats are at its sides) - a ship rolled over
    // comes to rest on its side; held at the keel alone, all four came to rest level and it stood up again on the bottom
    float al[3], ac[3], lal = 0.0f, lac = 0.0f, uz, below[US_PTS];
    for (int j = 0; j < 3; j++) al[j] = k->sp[0][j] - k->sp[1][j], ac[j] = k->sp[2][j] - k->sp[3][j], lal += al[j] * al[j], lac += ac[j] * ac[j];
    lal = sqrtf(lal), lac = sqrtf(lac);
    uz = lal > 1e-6f && lac > 1e-6f ? (al[0] * ac[1] - al[1] * ac[0]) / (lal * lac) : 1.0f;   // the hull's up, its height
    float side = lac > 1e-6f ? fabsf(ac[2]) / lac : 0.0f, beam = 0.5f * lac;
    below[0] = below[1] = k->keel * fmaxf(uz, 0.0f) + beam * side;
    below[2] = below[3] = k->keel * fmaxf(uz, 0.0f);
    if (!k->air && !k->shift_to && side > US_SHIFT) k->shift_to = k->sp[2][2] < k->sp[3][2] ? 2 : 3;
    if (k->shift_to && k->shift < 1.0f) {   // sliding: its place in the hull from the middle toward the low side's float
        k->shift = fminf(k->shift + dt / US_SHIFT_S, 1.0f);
        const float *a = k->body[2], *b = k->body[3], *to = k->body[k->shift_to];
        for (int j = 0; j < 2; j++) {
            float mid = 0.5f * (a[j] + b[j]);
            k->body[4][j] = mid + 0.8f * k->shift * (to[j] - mid);
        }
    }
    // the ballast: above the keel on the middle line, on its side half the beam up - less as it slid to the low side
    below[4] = (1.0f - US_LOW) * k->keel * fmaxf(uz, 0.0f) + beam * side * (1.0f - 0.8f * k->shift);
    for (int round = 0; round < 4; round++) {
        for (int i = 0; i < k->n; i++)
            for (int j = i + 1; j < k->n; j++) {
                float d[3], e[3], len = 0.0f, rest = 0.0f;
                for (int c = 0; c < 3; c++) d[c] = k->sp[j][c] - k->sp[i][c], e[c] = k->body[j][c] - k->body[i][c], len += d[c] * d[c], rest += e[c] * e[c];
                len = sqrtf(len), rest = sqrtf(rest);
                if (len < 1e-6f) continue;
                // by their weights: the lighter moves the more (a float 1, the ballast US_BALLAST ...: us_w)
                float wi = 1.0f / us_w(k, i), wj = 1.0f / us_w(k, j), w = (len - rest) / (len * (wi + wj));
                for (int c = 0; c < 3; c++) k->sp[i][c] += d[c] * w * wi, k->sp[j][c] -= d[c] * w * wj;
            }
        for (int i = 0; i < US_PTS && !k->air; i++) {   // (an aircraft's ground: its middle, below)
            float *p = k->sp[i], *q = k->sq[i];
            int x = (int)floorf(p[0]), y = (int)floorf(p[1]);
            if (!mesh_has_tile(mesh, x, y)) continue;
            float bottom = mesh_z(mesh, p[0], p[1]) + below[i] - k->dig[i];
            if (p[2] < bottom) {   // on the bottom: held up, and dragged along it
                p[2] = bottom;
                if (q[2] < p[2]) q[2] = p[2];
                q[0] = p[0] - (p[0] - q[0]) * 0.8f, q[1] = p[1] - (p[1] - q[1]) * 0.8f;
            }
        }
    }
    if (k->air) {   // its middle on the ground or the seabed: it stays as it hit (a crash on land going down fast)
        float m[3] = { 0, 0, 0 }, mq = 0.0f, a[3], b[3], u[3], lu2 = 0.0f;
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 3; j++) m[j] += 0.25f * k->sp[i][j];
            mq += 0.25f * k->sq[i][2];
        }
        for (int j = 0; j < 3; j++) a[j] = k->sp[0][j] - k->sp[1][j], b[j] = k->sp[2][j] - k->sp[3][j];
        u[0] = a[1] * b[2] - a[2] * b[1], u[1] = a[2] * b[0] - a[0] * b[2], u[2] = a[0] * b[1] - a[1] * b[0];
        for (int j = 0; j < 3; j++) lu2 += u[j] * u[j];
        if (lu2 > 1e-12f) {   // the collider up its body, by US_HIT_HIGH (the way down its own as well)
            float h = k->hit / sqrtf(lu2);
            for (int j = 0; j < 3; j++) m[j] += h * u[j];
            mq += h * u[2];
        }
        if (mesh_has_tile(mesh, (int)floorf(m[0]), (int)floorf(m[1]))) {
            float ground = mesh_z(mesh, m[0], m[1]);
            if (m[2] <= ground) {
                if (ground >= k->zw && !(k->fx_done & US_FX_CRASH) && mq - m[2] > US_CRASH_V * dt) us_fx(k, US_FX_CRASH, m[0], m[1], ground);
                k->stuck = 1;
            }
        }
    }
}
// simulated on to the walk's game time (at most 3 s at once: one long out of the view jumps there)
internal void us_run(uc_corpse *k) {
    if (g_uc_now < 0.0) return;
    if (g_uc_now - k->sim > 3000.0) k->sim = g_uc_now - 3000.0;
    const void *mesh = map_mesh_at(game_map());
    while (k->sim + US_DT * 1000.0 <= g_uc_now) {
        k->sim += US_DT * 1000.0;
        us_step(k, (float)((k->sim - k->died) / 1000.0), mesh);
    }
}
// its pose into t (rows, the place in the fourth column) from its floats: bow-ward from the stern to the bow, port-ward
// from starboard to port square to it, up square to both - the hull's own axes there
internal void us_pose(const uc_corpse *k, float *t) {
    float a[3], b[3], u[3], la = 0.0f, lb = 0.0f, ab = 0.0f, mean[3] = { 0, 0, 0 }, body[3] = { 0, 0, 0 };
    for (int j = 0; j < 3; j++) a[j] = k->sp[0][j] - k->sp[1][j], b[j] = k->sp[2][j] - k->sp[3][j], la += a[j] * a[j];
    la = sqrtf(la);
    if (la < 1e-6f) return;
    for (int j = 0; j < 3; j++) a[j] /= la, ab += a[j] * b[j];
    for (int j = 0; j < 3; j++) b[j] -= ab * a[j], lb += b[j] * b[j];
    lb = sqrtf(lb);
    if (lb < 1e-6f) return;
    for (int j = 0; j < 3; j++) b[j] /= lb;
    u[0] = a[1] * b[2] - a[2] * b[1], u[1] = a[2] * b[0] - a[0] * b[2], u[2] = a[0] * b[1] - a[1] * b[0];
    const float up[3] = { 0.0f, 0.0f, 1.0f };
    for (int r = 0; r < 3; r++)   // the world's axes over the hull's: [a b u] times [along across up] turned
        for (int c = 0; c < 3; c++) t[r * 4 + c] = a[r] * k->along[c] + b[r] * k->across[c] + u[r] * up[c];
    for (int i = 0; i < 4; i++)
        for (int j = 0; j < 3; j++) mean[j] += 0.25f * k->sp[i][j], body[j] += 0.25f * k->body[i][j];
    for (int r = 0; r < 3; r++) t[r * 4 + 3] = mean[r] - (t[r * 4] * body[0] + t[r * 4 + 1] * body[1] + t[r * 4 + 2] * body[2]);
}
// a rotting object the walk met (or a dead ship, rotting or not yet), its record rec filled and its pose t not yet sunk:
// its entry made or brought up to date, the record's turns its held ones, and a ship's pose its floats'
internal void uc_corpse_saw(const uc_copy *c, const uc_obj *o, uint8_t *rec, float *t, double d, int ship) {
    if (!g_kc_cs_ok || g_uc_now < 0.0 || !o->gfx) return;
    EnterCriticalSection(&g_kc_cs);
    uc_corpse *k = NULL;
    for (uint32_t i = 0; i < g_kc_n && !k; i++) if (g_kc[i].u == o->u) k = &g_kc[i];
    if (!k && g_kc_n < UC_CORPSES) {
        k = &g_kc[g_kc_n++];
        memset(k, 0, sizeof *k);
        k->u = o->u;
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
        if (k->ship) us_run(k), us_pose(k, t);
        k->gen = g_uc_walk_gen, k->gone = 0;
    }
    LeaveCriticalSection(&g_kc_cs);
}
internal void uc_corpses_clear(void) {
    if (!g_kc_cs_ok) return;
    EnterCriticalSection(&g_kc_cs);
    for (uint32_t i = 0; i < g_kc_n; i++) graphics_release(g_kc[i].gfx);
    g_kc_n = 0;
    LeaveCriticalSection(&g_kc_cs);
}
// a kept corpse into the record rec at its age d, as uc_fill_dopp fills a remembered building: whole or see-through by
// its fade, not fogged, never picked, no silhouette, its parts as last seen; 0 when outside the view
internal int uc_fill_corpse(uc_corpse *k, uint8_t *rec, uint8_t *maps, double d) {
    ee_model *m = ((ee_model **)dmg_model_anim_model_at(k->gfx))[k->anim];
    if (!m) return 0;
    __attribute__((aligned(8))) float pose[16];   // where it lies: its last pose, a ship's by its floats on
    memcpy(pose, k->t, sizeof pose);
    if (k->ship) us_run(k), us_pose(k, pose);
    __attribute__((aligned(8))) uint8_t pm[0x60];
    physical_model_new(pm);
    physical_model_set_model(pm, m);
    physical_model_set_frame(pm, k->frame);
    transform_assign(pm, pose);
    physical_model_set_scale(pm, k->scale);
    int in = units_view_call(4)(pm, opacity_viewport(), *game_camera_fixed_at()) != 0;
    physical_model_free(pm);
    if (!in) return 0;
    opacity_consts oc = opacity_consts_now();
    ee_map *map = game_map();
    ee_unit_type *ty = k->ty;
    float op = uc_dead_opacity(d), pos[3], ground[3] = { 0, 0, 0 };
    sil_tag(rec, g_uc_walk_gen, 0);
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
    draw_record_set_fogged(rec, 0);
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
    draw_record_set_frame(rec, k->frame);
    draw_record_set_next_frame(rec, k->frame);
    draw_record_set_fraction(rec, 0.0f);
    draw_record_set_state(rec, 0);
    draw_record_set_anim(rec, k->anim);
    transform_assign(rec, pose);
    float *t = draw_record_transform_at(rec);
    if (map_tile_lit(map, (int)floorf(t[3]), (int)floorf(t[7])) && t[11] >= oc.zero) draw_record_set_ground_z(rec, 0.0f);
    else {
        pos[0] = t[3], pos[1] = t[7], pos[2] = t[11];
        ground_point(map, pos, ground);
        draw_record_set_ground_z(rec, ground[2]);
    }
    if (!k->ship) uc_sink_at(t, k->gfx, k->anim, k->frame, k->scale, d);
    void *pair[2] = { (void *)(uintptr_t)graphics_anim_key(k->gfx, k->anim), rec };
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
// the walk's end: those it didn't meet removed (kept) or out of the view (let go; a wreck the game still has: drawn), the
// kept ones drawn on the tiles of the copy the player sees - a wreck's by its floats' middle -, those whose time is up let go
internal void uc_corpses_walk(const uc_copy *c, uint8_t *recs, uint32_t *n, uint32_t room, uint8_t *maps, int free_cam) {
    if (!g_kc_cs_ok || g_uc_now < 0.0) return;
    EnterCriticalSection(&g_kc_cs);
    double end = uc_end_ms();
    int known = us_alive_known();
    uint32_t j = 0;
    for (uint32_t i = 0; i < g_kc_n; i++) {
        uc_corpse *k = &g_kc[i];
        int unmet = k->gen != g_uc_walk_gen;
        if (unmet && !k->gone) k->gone = k->ship && known ? (k->in_game ? 0 : 1) : k->rot <= UC_GONE_ROT ? 1 : 2;
        double d = g_uc_now - k->died;
        if (k->gone == 2 || (k->gone && d >= end)) {
            graphics_release(k->gfx);
            continue;
        }
        if ((k->gone || unmet) && g_set.corpses_sink && *n < room) {
            float at[2] = { k->t[3], k->t[7] };
            if (k->ship) at[0] = 0.25f * (k->sp[0][0] + k->sp[1][0] + k->sp[2][0] + k->sp[3][0]),
                         at[1] = 0.25f * (k->sp[0][1] + k->sp[1][1] + k->sp[2][1] + k->sp[3][1]);
            int x = (int)floorf(at[0]) - c->x0, y = (int)floorf(at[1]) - c->y0;
            const uc_ctile *ct = x >= 0 && y >= 0 && x < c->w && y < c->h ? &c->tiles[y * c->w + x] : NULL;
            if (ct && ct->t && (ct->seen || free_cam) && uc_fill_corpse(k, recs + (size_t)*n * DRAW_REC, maps, d)) ++*n, g_kc_drawn++;
        }
        if (j != i) g_kc[j] = *k;
        j++;
    }
    g_kc_n = j;
    LeaveCriticalSection(&g_kc_cs);
}
// THE EFFECTS OF ITS FALL, on the simulation's thread (above us_fx): the wrecks are the mod's while its fills are on and
// the corpses go down by their floats
internal int us_effects_on(void) { return g_uc_own && g_uc_fill == 2 && g_set.corpses_sink && g_set.ship_sinking; }
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
// splashes and crashes the fall noted made first (on the unit when the game still has it, else at the place: a kind
// with no placed version, the game's explosion, is then left out)
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
            if (k->fx_due & US_FX_CRASH) {
                void *fx = unit_type_get_impact_gfx(k->ty);
                if (cu) effect_mgr_add_on_unit(m, cu, fx, NULL, 0, 0, 0);
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

// STEP 2, THE FILLS OURS: THE OPACITY (EEROUnits.opacity, the AoC decompilation's GetUnitOpacity: EE 0x5044D7, AoC
// 0x50F412 with its cloak part in a function of its own, 0x63CEE2). An object's opacity into *out, and whether it is
// drawn see-through (its record into the see-through sort map): "show hidden units" (1: an obstacle or a moved unit
// hidden behind something, 2: every obstacle) at hidden_opacity; a dying unit fading as it rots; a cloakable unit by its
// own opacity, dimmed to an enemy; and a land building faded as it comes near the camera's front plane. Ported from
// the disassembly, its floats as the FPU has them: long double between the stores the game's code makes (a float
// there), in the same order - the game's precision whatever the FPU is set to on the render thread. Of the object's
// copy (collect_copy.c: what the game reads of the unit and its complex unit), the camera the frame's.
internal int uc_cloak(const uc_obj *o, int see, float *out, const opacity_consts *c) {
    if (o->rotting && g_set.corpses_sink) {   // whole while it sinks, then faded out; a kind without a rot time none (UC_NO_CORPSE)
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
    int ship = cu && g_set.corpses_sink && g_set.ship_sinking && o->cu_hp <= 0 && uc_is_ship(o->ty);   // (uc_ship_*)
    if (cu && (o->rotting || ship) && g_set.corpses_sink) {   // after the ground's height under it: that stays the ground's
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
    pos[2] = uc_mesh_z(map_mesh_at(map), pos[0], pos[1]);
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
            void *pair[2] = { (void *)(uintptr_t)graphics_anim_key(draw_small_record_get_gfx(rec), 0), rec };
            uc_map_insert(sort_map, pair);
            uc_pick_add(p->u, t, draw_small_record_get_opacity(rec));
            pair[0] = (void *)(uintptr_t)dmg_model_get_shadow_key(draw_small_record_get_gfx(rec));
            pair[1] = rec;
            uc_map_insert(engine_display_sort_maps_at() + 0x3C, pair);
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
    g_tl.want = 1;
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
    if (g_tl.want) { g_tl.normal++; return -2; }
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
                        if (tl == -2 && !see) g_tl.anchor = rec, g_tl.want = 0;
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
