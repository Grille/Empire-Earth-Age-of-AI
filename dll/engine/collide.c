// SHIP COLLISION (`ship_collide`; docs/engine-notes.md "Collision"): the game keeps ships apart only by a contact
// list rebuilt on a tile change, from the tiles within the ship's own size, and tests each contact as two lines along
// the headings without width: ships side by side or at a shallow angle pass through each other, a small one next to
// a big one never sees it; a hit halves the speed of the smaller (per contact, every step) and pushes it back in one
// jump after its step. Here, in the ships' collision step (slot 18: after the step, before the push is applied),
// every ship is a hull - a capsule along its heading, the game's size (+0x244) its length, UC_WIDTH of it its width -
// and each ship near (the unit grid, not the contact list) whose hull overlaps is pushed off along the line between
// their nearest points: UC_RELAX of the overlap a step, shared by size when the other moves too (it does its own part
// in its own step), all of it against one standing. The push goes through the game's own (SLOT_PM_PUSH: terrain and
// buildings tested, no faster than the top speed); the speed is left alone, so a ship pressing on another slides
// along it. Submarines too (their model's slot 18 is the same): the game lets them meet only submarines - its test
// (EEPMSubmergedNaval::vf14) takes the classes 10 and 27 - so they pass under ships, and here as well.
// Only + - * / and sqrtf: the same on every PC.

#define UC_WIDTH 0.35f   // a hull's width, a share of its length
#define UC_RELAX 0.5f    // the share of an overlap undone a step
#define UC_SIDESTEP 0.5f   // heading into another: the push turned this much (x how straight) to the side, starboard unless the other lies clearly to port
#define UC_REACH 2       // tiles looked beyond the sum of the two half lengths for the largest ship (a carrier 3.2)

internal int g_uc_on;
internal unsigned g_uc_steps, g_uc_hits;   // collision steps taken over, overlaps pushed off
internal float g_uc_deepest;               // the deepest overlap met (tiles)

struct uc_hull { vec2 a, b, dir; float r, len; };   // a capsule: the segment a-b, its radius; the heading, the ship's length

internal struct uc_hull uc_hull_of(ee_unit *u) {
    struct uc_hull h;
    vec3 p = unit_get_pos(u), f = unit_get_facing(u);
    vec2 c = v2(p.x, p.y), d = v2(f.x, f.y);
    float n = vec2_len(d);
    d = n > 0.0001f ? vec2_scale(d, 1.0f / n) : v2(0, 1);
    h.dir = d;
    h.len = unit_get_size(u);
    h.r = h.len * UC_WIDTH * 0.5f;
    float half = h.len * 0.5f - h.r;
    if (half < 0) half = 0;
    h.a = vec2_sub(c, vec2_scale(d, half));
    h.b = vec2_add(c, vec2_scale(d, half));
    return h;
}

internal float uc_clamp01(float v) { return v < 0 ? 0 : v > 1 ? 1 : v; }

// the nearest points of segments p1-q1 and p2-q2 (Ericson, Real-Time Collision Detection 5.1.9): into *c1, *c2
internal void uc_nearest(vec2 p1, vec2 q1, vec2 p2, vec2 q2, vec2 *c1, vec2 *c2) {
    vec2 d1 = vec2_sub(q1, p1), d2 = vec2_sub(q2, p2), r = vec2_sub(p1, p2);
    float a = vec2_dot(d1, d1), e = vec2_dot(d2, d2), f = vec2_dot(d2, r), s = 0, t = 0;
    if (a <= 1e-6f && e <= 1e-6f) s = t = 0;
    else if (a <= 1e-6f) t = uc_clamp01(f / e);
    else {
        float c = vec2_dot(d1, r);
        if (e <= 1e-6f) s = uc_clamp01(-c / a);
        else {
            float b = vec2_dot(d1, d2), den = a * e - b * b;
            s = den > 1e-9f ? uc_clamp01((b * f - c * e) / den) : 0;
            t = (b * s + f) / e;
            if (t < 0) { t = 0; s = uc_clamp01(-c / a); }
            else if (t > 1) { t = 1; s = uc_clamp01((b - c) / a); }
        }
    }
    *c1 = vec2_add(p1, vec2_scale(d1, s));
    *c2 = vec2_add(p2, vec2_scale(d2, t));
}

internal int uc_is_sub(ee_unit_type *td) { int c = type_get_family(td); return c == 10 || c == 27; }   // the classes the submarines' test takes

// a hull u meets: a ship afloat, a submarine only if u is one
internal int uc_is_hull(ee_unit *c, int sub) {
    ee_unit_type *td = unit_get_type_data(c);
    return td && unit_get_valid(c) && unit_get_hp(c) > 0 && !unit_get_holder(c) && uc_is_sub(td) == sub && sr_is_ship(c);
}

internal int uc_moves(ee_unit *u) { return unit_get_speed(u) > 0.05f || unit_get_turn_rate(u) != 0; }

// the push off every overlapping hull near u, added to its push velocity; a standing ship of u's owner gets its part
// at once (its own collision step doesn't run while it stands)
internal void uc_separate(void *model, ee_unit *u) {
    ee_physics *ph = game_physics();
    float dt = ph ? phys_get_step_s(ph) * (float)phys_substeps() : 0;
    if (dt <= 0) return;
    struct uc_hull h = uc_hull_of(u);
    float mu = h.len * h.len;
    vec2 corr = v2(0, 0);
    vec3 p = unit_get_pos(u);
    int sub = uc_is_sub(unit_get_type_data(u));
    int reach = (int)(h.len * 0.5f) + UC_REACH + 1;
    for (struct grid_it it = grid_it_box(unit_get_x(u), unit_get_y(u), reach, 1); grid_it_next(&it); ) {
        ee_unit *c = it.u;
        if (c == u || !uc_is_hull(c, sub)) continue;
        vec3 q = unit_get_pos(c);
        float cl = unit_get_size(c), lim = (h.len + cl) * 0.5f;
        if (fabsf(q.x - p.x) > lim || fabsf(q.y - p.y) > lim) continue;
        struct uc_hull o = uc_hull_of(c);
        vec2 n1, n2;
        uc_nearest(h.a, h.b, o.a, o.b, &n1, &n2);
        vec2 d = vec2_sub(n1, n2);
        float d2 = vec2_len2(d), rr = h.r + o.r;
        if (d2 >= rr * rr) continue;
        float dist = sqrtf(d2);
        if (dist > 0.0001f) d = vec2_scale(d, 1.0f / dist);
        else {   // the axes cross: off sideways from the other's heading, to the side u's centre is on
            vec2 side = v2(o.b.y - o.a.y, o.a.x - o.b.x), rel = v2(p.x - q.x, p.y - q.y);
            float sl = vec2_len(side);
            if (sl <= 0.0001f) side = v2(1, 0), sl = 1;
            d = vec2_scale(side, (vec2_dot(side, rel) < 0 ? -1.0f : 1.0f) / sl);
        }
        float k = -vec2_dot(h.dir, d);
        if (k > 0 && unit_get_speed(u) > 0.05f) {   // u heads into it: off to a side as well, so it slides past
            vec2 right = v2(h.dir.y, -h.dir.x);
            float side = vec2_dot(d, right) < -0.05f ? -1.0f : 1.0f;
            d = vec2_add(d, vec2_scale(right, side * k * UC_SIDESTEP));
            d = vec2_scale(d, 1.0f / vec2_len(d));
        }
        float over = rr - dist, oc = o.len * o.len, share = oc / (mu + oc);
        int moves = uc_moves(c), nudge = !moves && unit_get_owner(c) == unit_get_owner(u) && !unit_get_frozen(c);
        if (!moves && !nudge) share = 1.0f;
        corr = vec2_add(corr, vec2_scale(d, over * share * UC_RELAX));
        if (nudge) {   // the standing one steps aside by its part
            unit_set_push(c, vec2_add(unit_get_push(c), vec2_scale(d, -over * (1.0f - share) * UC_RELAX / dt)));
            model_push(model, c);
        }
        g_uc_hits++;
        if (over > g_uc_deepest) g_uc_deepest = over;
    }
    if (corr.x == 0 && corr.y == 0) return;
    unit_set_push(u, vec2_add(unit_get_push(u), vec2_scale(corr, 1.0f / dt)));
}

internal void __thiscall hook_ship_collide(void *model, ee_unit *u) {
    if (!u || !unit_get_moving(u)) { model_collide(model, u); return; }
    vec2 push = unit_get_push(u);
    if (unit_get_speed(u) == 0 && unit_get_turn_rate(u) == 0 && fabsf(push.x) < 0.001f && fabsf(push.y) < 0.001f) { model_collide(model, u); return; }
    g_uc_steps++;
    uc_separate(model, u);
    model_push(model, u);
}

internal void collide_new_game(void) { g_uc_steps = g_uc_hits = 0; g_uc_deepest = 0; }

internal void collide_install(void) {
    g_uc_on = g_set.ship_collide;
    int n = g_uc_on ? patch_ship_collide(hook_ship_collide) : 0;
    if (g_log) fprintf(g_log, "# shipcollide: ship_collide=%d (hooks %d/2: ships, submarines)\n", g_uc_on, n);
}
