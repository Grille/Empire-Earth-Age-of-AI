// COLLISION (`ship_collide`, `land_collide`; docs/engine-notes.md "Collision"): the game keeps units apart only by a
// contact list rebuilt on a tile change, from the tiles within the unit's own size, and tests each contact as two
// lines along the headings without width (ships) or as circles: ships side by side or at a shallow angle pass through
// each other, a small unit next to a big one never sees it; a hit halves the speed of the smaller (per contact, every
// step) and pushes it back in one jump after its step. Here, in the models' collision step (slot 18: after the step,
// before the push is applied), every unit is a hull - a ship a capsule along its heading, the game's size (+0x244) its
// length, UC_WIDTH of it its width; a land unit a circle of that diameter, as the game's circle test has it - and
// each unit near (the unit grid, not the contact list) of the same kind of movement (model_kind: ships, submarines -
// the game lets them meet only submarines, so they pass under ships -, land units with hovercraft) whose hull overlaps
// is pushed off along the line between their nearest points: UC_RELAX of the overlap a step, shared by size when the
// other moves too (it does its own part in its own step), all of it against one standing - but a standing one of the
// same owner steps aside by its part. A unit heading into another veers off to the right. As in the game's test a
// unit at work (gathering, building: UNIT_AT_WORK on a kind 6 or 4 goal) is passed, but by one at the same work. The
// push goes through the game's own (value_movement_slot_pm_push: terrain and buildings tested, no faster than the top speed); the
// speed is left alone, so a unit pressing on another slides along it. A unit at rest (not at work) moves its share off
// other units at rest every UC_REST_EVERY steps - else a crowd that stopped on one spot stays one blob - in movement
// mode 0 for the push, so it doesn't play its walk; overlaps under UC_REST_SLOP are left, so the crowd settles. Only + - * / and sqrtf: the same on every PC.

#define UC_WIDTH 0.35f   // a ship's width, a share of its length
#define UC_RELAX 0.5f    // the share of an overlap undone a step
#define UC_SIDESTEP 0.5f   // heading into another: the push turned this much (x how straight) to the side, starboard unless the other lies clearly to port
#define UC_REST_SLOP 0.05f // tiles: at rest a lighter overlap is left - the positions settle instead of trading shoves
#define UC_REST_EVERY 4   // a unit at rest looks for overlaps every this many steps (its share of each, the other does its own)

internal int g_uc_on[4];                  // by model_kind
internal float g_uc_biggest[4];           // the largest size met of the kind: how far to look for an overlap
internal unsigned g_uc_rest;                 // pushes of units at rest off each other
#define UC_REST_LOCK 3          // pushes at rest back against the one before, since it last moved on its own: then it
                                // stays put - units pressed between others and something the push can't move shuffled
                                // for ever; pushes on the same way are progress and don't count
#define UC_TOUCH 0.1f           // tiles: a gap this narrow counts as touching (uc_weight_against)
#define UC_BEHIND 0.5f          // a neighbour within 60 degrees of the push's direction is in its way
#define UC_MEM 16384
struct uc_rest { uint32_t id; int n; vec2 last; };   // a unit's reversals at rest and its last push
internal struct uc_rest g_uc_mem[UC_MEM];

internal struct uc_rest *uc_rest_of(ee_unit *u) {
    uint32_t id = (uint32_t)unit_get_serial(u), h = (id * 2654435761u) % UC_MEM, i = h;
    for (int k = 0; k < 8; k++, i = (h + (uint32_t)k) % UC_MEM)
        if (g_uc_mem[i].id == id || !g_uc_mem[i].id) break;
    if (g_uc_mem[i].id != id) { g_uc_mem[i].id = id; g_uc_mem[i].n = 0; g_uc_mem[i].last = v2(0, 0); }
    return &g_uc_mem[i];
}
internal unsigned g_uc_steps, g_uc_hits;   // collision steps taken over, overlaps pushed off
internal float g_uc_deepest;               // the deepest overlap met (tiles)

struct uc_hull { vec2 a, b, dir; float r, len; };   // a capsule: the segment a-b, its radius; the heading, the unit's length

internal struct uc_hull uc_hull_of(ee_unit *u, int kind) {
    struct uc_hull h;
    vec3 p = unit_get_pos(u), f = unit_get_facing(u);
    vec2 c = v2(p.x, p.y), d = v2(f.x, f.y);
    float n = vec2_len(d);
    d = n > 0.0001f ? vec2_scale(d, 1.0f / n) : v2(0, 1);
    h.dir = d;
    h.len = unit_get_size(u);
    h.r = h.len * (kind == MODEL_LAND ? 1.0f : UC_WIDTH) * 0.5f;
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

// a unit's kind of movement (model_kind), 0 for one the collision leaves alone: gone, inside a transport or building,
// a resource (fish move on the ships' model)
internal int uc_kind(ee_unit *c) {
    ee_unit_type *td = unit_get_type_data(c);
    if (!td || !unit_get_valid(c) || unit_get_hp(c) <= 0 || unit_get_holder(c)) return 0;
    int cat = unit_type_get_category(td);
    return cat == KIND_RESOURCE || cat == KIND_FISH ? 0 : model_kind(unit_type_get_model(td));
}

// u's work (a kind 6 or 4 goal it is at: gathering, building), else 0 - the flag alone stays set on an idle citizen
internal int uc_work(ee_unit *u) {
    ee_goal *g = unit_get_goal_object(u);
    int k = g && unit_get_at_work(u) ? goal_get_state(g) : 0;
    return k == 6 || k == 4 ? k : 0;
}

// c at work: u passes it, unless u is at the same work
internal int uc_passes(ee_unit *u, ee_unit *c) {
    ee_goal *gc = unit_get_goal_object(c);
    int k = uc_work(c);
    if (!k) return 0;
    ee_goal *gu = unit_get_goal_object(u);
    if (!gu || !unit_get_at_work(u) || goal_get_state(gu) != k) return 1;
    return k == 6 ? goal_get_0x38(gu) != goal_get_0x38(gc) : goal_get_0x3c(gu) != goal_get_0x3c(gc);
}

// a standing unit's push applied in movement mode 0: it moves without the pushed walk (the game's state 13)
internal void uc_push_quiet(void *model, ee_unit *u) {
    int mode = unit_get_move_mode(u);
    unit_set_move_mode(u, 0);
    model_push(model, u);
    unit_set_move_mode(u, mode);
}

internal int uc_moves(ee_unit *u) { return unit_get_speed(u) > 0.05f || unit_get_turn_rate(u) != 0; }

// a standing land unit's weight against a mover pushing it along dir: its size squared x (1 + 8^n) for the n standing
// units it touches on that side (within UC_BEHIND of dir; none: x 1) - a unit steps aside freely, but not into the
// others: they had been pressed into each other, sprang back after the mover or slid round one another. A count of
// every neighbour weighed the crowd's edge light, which then was pushed into the heavy middle; 1 + 2^n of the units
// behind still let a truck press citizens together (their size squared 0.09 against its 0.49)
internal float uc_weight_against(int kind, ee_unit *c, float len, vec2 dir) {
    vec3 p = unit_get_pos(c);
    int n = 0, reach = (int)((len + g_uc_biggest[kind]) * 0.5f + UC_TOUCH) + 1;
    for (struct grid_it it = grid_it_box(unit_get_x(c), unit_get_y(c), reach, 1); grid_it_next(&it); ) {
        ee_unit *b = it.u;
        if (b == c) continue;
        vec3 q = unit_get_pos(b);
        vec2 rel = v2(q.x - p.x, q.y - p.y);
        float lim = (len + unit_get_size(b)) * 0.5f + UC_TOUCH, d2 = vec2_len2(rel);
        if (d2 >= lim * lim || d2 < 1e-8f || vec2_dot(rel, dir) < UC_BEHIND * sqrtf(d2)) continue;
        if (uc_kind(b) != kind || uc_moves(b)) continue;
        n++;
    }
    return len * len * (n ? 1.0f + (float)(1 << (n < 4 ? 3 * n : 12)) : 1.0f);
}

// the push off every overlapping hull near u, added to its push velocity; a standing ship of u's owner gets its part
// at once (its own collision step doesn't run while it stands)
internal int uc_separate(int kind, ee_unit *u, int resting) {
    ee_physics *ph = game_physics();
    float dt = ph ? physics_get_step_s(ph) * (float)phys_substeps() : 0;
    if (dt <= 0) return 0;
    struct uc_hull h = uc_hull_of(u, kind);
    float mu = h.len * h.len;
    vec2 corr = v2(0, 0);
    vec3 p = unit_get_pos(u);
    if (h.len > g_uc_biggest[kind]) g_uc_biggest[kind] = h.len;
    int reach = (int)((h.len + g_uc_biggest[kind]) * 0.5f) + 1;   // the farthest centre an overlap can have
    for (struct grid_it it = grid_it_box(unit_get_x(u), unit_get_y(u), reach, 1); grid_it_next(&it); ) {
        ee_unit *c = it.u;
        if (c == u) continue;
        vec3 q = unit_get_pos(c);   // the near test first: most units in the box are far, and uc_kind costs
        float cl = unit_get_size(c), lim = (h.len + cl) * 0.5f;
        if (fabsf(q.x - p.x) > lim || fabsf(q.y - p.y) > lim) continue;
        if (uc_kind(c) != kind || uc_passes(u, c)) continue;
        struct uc_hull o = uc_hull_of(c, kind);
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
        int moves = uc_moves(c), nudge = !resting && !moves && unit_get_owner(c) == unit_get_owner(u) && !unit_get_frozen(c);
        float over = rr - dist, oc = nudge && kind == MODEL_LAND ? uc_weight_against(kind, c, o.len, v2(-d.x, -d.y)) : o.len * o.len;
        float share = oc / (mu + oc);
        if (resting && moves) continue;   // a moving one does it in its own step
        if (!resting && !moves && !nudge) share = 1.0f;
        if (over > g_uc_deepest) g_uc_deepest = over;
        if (resting && over < UC_REST_SLOP) continue;
        over *= UC_RELAX;
        corr = vec2_add(corr, vec2_scale(d, over * share));
        if (nudge) {   // the standing one steps aside by its part, and may part from the others again (its lock undone)
            uc_rest_of(c)->n = 0;
            unit_set_push(c, vec2_add(unit_get_push(c), vec2_scale(d, -over * (1.0f - share) / dt)));
            uc_push_quiet(unit_type_get_model(unit_get_type_data(c)), c);
        }
        g_uc_hits++;
    }
    if (corr.x == 0 && corr.y == 0) return 0;
    unit_set_push(u, vec2_add(unit_get_push(u), vec2_scale(corr, 1.0f / dt)));
    return 1;
}

internal void __thiscall hook_collide(void *model, ee_unit *u) {
    int kind = model_kind(model);
    if (!u || !g_uc_on[kind]) { model_collide(model, u); return; }
    if (!unit_get_moving(u) || (unit_get_speed(u) == 0 && unit_get_turn_rate(u) == 0)) {
        // at rest: the game's step (its blocked time, the idle states), then, every UC_REST_EVERY steps, off the other
        // units at rest it overlaps - else units that stop on one spot (a rally point, a crowd) stay one blob; a unit
        // at work keeps its place. A push the game's contact test gave it (its circle test pushes the partner too) is
        // dropped: the units around a pair that couldn't part shuffled for ever
        unit_set_push(u, v2(0, 0));
        model_collide(model, u);
        ee_physics *ph = game_physics();
        struct uc_rest *r = NULL;
        if (ph && !uc_work(u) && !unit_get_frozen(u) && unit_get_hp(u) > 0 && !unit_get_holder(u)
            && ((unsigned)unit_get_serial(u) + physics_get_step(ph)) % UC_REST_EVERY == 0
            && (r = uc_rest_of(u))->n < UC_REST_LOCK && uc_separate(kind, u, 1)) {
            vec2 p = unit_get_push(u);
            if (vec2_dot(p, r->last) < 0) r->n++;
            r->last = p;
            g_uc_rest++;
            uc_push_quiet(model, u);
        }
        return;
    }
    g_uc_steps++;
    uc_rest_of(u)->n = 0;   // it moves: free to be pushed at rest again
    uc_separate(kind, u, 0);
    model_push(model, u);
}

internal void collide_new_game(void) { g_uc_steps = g_uc_hits = g_uc_rest = 0; g_uc_deepest = 0; memset(g_uc_biggest, 0, sizeof g_uc_biggest); memset(g_uc_mem, 0, sizeof g_uc_mem); }

internal void collide_install(void) {
    g_uc_on[MODEL_SHIPS] = g_uc_on[MODEL_SUBS] = g_set.ship_collide;
    g_uc_on[MODEL_LAND] = g_set.land_collide;
    int n[4] = { 0 };
    for (int k = MODEL_SHIPS; k <= MODEL_LAND; k++) n[k] = g_uc_on[k] ? patch_collide(k, hook_collide) : 0;
    if (g_log) fprintf(g_log, "# collide: ship_collide=%d land_collide=%d (hooks: ships %d, submarines %d, land %d/2)\n", g_set.ship_collide, g_set.land_collide,
                       n[MODEL_SHIPS], n[MODEL_SUBS], n[MODEL_LAND]);
}
