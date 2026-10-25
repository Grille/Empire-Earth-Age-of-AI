// FORMATIONS BY SIZE (`formations`; docs/engine-notes.md "Group moves"): a group move puts each unit on a slot of the
// group's formation, and the game spaces every row and column of it by the largest unit's collision size (type +0x470)
// - one tank stretches the grid of 170 soldiers - in as few as 5 columns, the rows stacked behind the clicked point,
// and pairs units with slots by distance alone (a tank may get a slot among infantry): a 174-unit army sent 20 tiles
// ended with half of it more than 28 tiles from the click (tests/probes/army_move.eeai). Here every formation is laid
// out again, for any group, from its units' own sizes and around the clicked point, keeping its kind's shape:
//   lines (short, long, loose, staggered): rows of FO_ASPECT[kind] times as wide as deep, each unit as wide as its
//     size x FO_PAD[kind], a row as deep as its largest unit, a staggered line's every other row shifted half a unit;
//   wedges (deep, shallow): rows from a tip, each as wide as the wedge's opening allows at its depth;
//   circles (inward, outward): rings from the middle out, the largest units innermost, as wide as their units.
// Units of like size are put in bands, each in rows of its own (a row is as deep as its largest unit), the bands and the
// rows in them front to back by how far ahead their units stand now along the move, the units in a row left to right by
// where they stand across it, so no one walks through the group. The game then pairs units with these slots as before
// (that block also prepares the group's route: skipping it sent units on detours), and each unit's goal is given its own
// planned spot instead of its paired slot's (hook_slot_goal: the slot found by its point, the unit by the slot). The
// collision sorts out the rest. Only + - * / and sqrtf (the circles' sines from a polynomial): the same on every PC.

#define FO_MAX 2048        // units laid out; a larger group keeps the game's slots
#define FO_MIN_SIZE 0.3f   // a unit's size counted at least this (a citizen 0.3)
#define FO_BAND 0.85f      // units this much of a band's largest or more share its rows
internal const float FO_PAD[8] = { 1.4f, 1.4f, 2.0f, 1.4f, 1.25f, 1.25f, 1.2f, 1.2f };   // by FORMATION_* kind
internal const float FO_ASPECT[4] = { 2.0f, 6.0f, 2.0f, 3.0f };                          // a line's width : depth
internal const float FO_WEDGE_TAN[2] = { 0.4142f, 0.8391f };   // tan of half the opening: deep 45, shallow 80 degrees

internal int g_fo_on;
internal unsigned g_fo_moves;   // group moves laid out

struct fo_unit { ee_unit *u; vec2 p; float w, along, across; vec2 at; int band; uint32_t facing; };   // at: its spot
internal struct fo_unit g_fo[FO_MAX];
internal int g_fo_order[FO_MAX];

// order idx[0..n) by key descending (insertion sort: stable, and n is a group)
internal void fo_sort(int *idx, int n, const float *key) {
    for (int i = 1; i < n; i++) {
        int v = idx[i], j = i;
        while (j > 0 && key[idx[j - 1]] < key[v]) { idx[j] = idx[j - 1]; j--; }
        idx[j] = v;
    }
}
internal float g_fo_key[FO_MAX];
internal void fo_sort_by(int *idx, int n, int what) {   // 0 along (front first), 1 across (left first), 2 size (largest first)
    for (int i = 0; i < n; i++) {
        struct fo_unit *f = &g_fo[idx[i]];
        g_fo_key[idx[i]] = what == 0 ? f->along : what == 1 ? -f->across : f->w;
    }
    fo_sort(idx, n, g_fo_key);
}

// sine and cosine of a in [0, 2 pi): reduced to a quarter, then Taylor to x^9 (error under 1e-6)
internal float fo_sin_q(float x) { float x2 = x * x; return x * (1 - x2 / 6 * (1 - x2 / 20 * (1 - x2 / 42 * (1 - x2 / 72)))); }
internal vec2 fo_dir(float a) {
    const float PI = 3.14159265f;
    float s = 1, c = 1;
    if (a > PI) { a -= PI; s = -1; c = -1; }
    if (a > PI / 2) { a = PI - a; c = -c; }
    return v2(c * fo_sin_q(PI / 2 - a), s * fo_sin_q(a));
}

// a direction's angle as a number in [0, 4) that orders like the angle (counter-clockwise from +x)
internal float fo_pseudo_angle(vec2 d) {
    float s = fabsf(d.x) + fabsf(d.y);
    if (s < 1e-6f) return 0;
    float a = d.y / s;
    return d.x < 0 ? 2 - a : d.y < 0 ? 4 + a : a;
}

// a row of units (idx[0..k)) placed left to right at depth y around x 0, shifted by dx
internal void fo_place_row(int *idx, int k, float y, float dx) {
    fo_sort_by(idx, k, 1);
    float wsum = 0;
    for (int i = 0; i < k; i++) wsum += g_fo[idx[i]].w;
    float x = -wsum * 0.5f + dx;
    for (int i = 0; i < k; i++) {
        struct fo_unit *f = &g_fo[idx[i]];
        f->at = v2(x + f->w * 0.5f, y);
        x += f->w;
    }
}

// lines and wedges: rows front to back (y 0, then negative), each filled to its width
internal void fo_rows(int *idx, int n, int kind) {
    fo_sort_by(idx, n, 0);
    // bands of like size (within FO_BAND of the band's largest), each in rows of its own - a row is as deep as its
    // largest unit, so tanks among infantry spaced every row out; the bands front to back by where they stand on
    // average, the units within each by where they stand (the sorts are stable)
    static int by_size[FO_MAX];
    static float band_along[FO_MAX];
    memcpy(by_size, idx, sizeof *idx * (size_t)n);
    fo_sort_by(by_size, n, 2);
    for (int i = 0, b = -1, first = 0; i < n; i++) {
        struct fo_unit *f = &g_fo[by_size[i]];
        if (b < 0 || f->w < g_fo[by_size[first]].w * FO_BAND) {
            if (b >= 0) band_along[b] /= (float)(i - first);
            b++; first = i; band_along[b] = 0;
        }
        f->band = b;
        band_along[b] += f->along;
        if (i == n - 1) band_along[b] /= (float)(i + 1 - first);
    }
    for (int i = 0; i < n; i++) g_fo_key[idx[i]] = band_along[g_fo[idx[i]].band];
    fo_sort(idx, n, g_fo_key);
    float area = 0, wmax = 0, wmean = 0;
    for (int i = 0; i < n; i++) { float w = g_fo[idx[i]].w; area += w * w; wmean += w; if (w > wmax) wmax = w; }
    wmean /= (float)n;
    int wedge = kind == FORMATION_DEEP_WEDGE || kind == FORMATION_SHALLOW_WEDGE;
    float width = wedge ? 0 : sqrtf(area * FO_ASPECT[kind]);
    if (width < wmax) width = wmax;
    float y = 0, prev = 0;
    for (int i = 0, row = 0; i < n; row++) {
        // the row's units: as many as its width takes (at least one), its depth their largest
        float cap = wedge ? g_fo[idx[i]].w + 2 * (-y) * FO_WEDGE_TAN[kind - FORMATION_DEEP_WEDGE] : width, sum = 0, deep = 0;
        int k = 0;
        while (i + k < n && (k == 0 || (sum + g_fo[idx[i + k]].w <= cap + 0.001f && g_fo[idx[i + k]].band == g_fo[idx[i]].band))) {
            float w = g_fo[idx[i + k]].w;
            sum += w;
            if (w > deep) deep = w;
            k++;
        }
        if (row) y -= (prev + deep) * 0.5f;
        fo_place_row(idx + i, k, y, kind == FORMATION_STAGGERED_LINE && (row & 1) ? wmean * 0.5f : 0);
        prev = deep;
        i += k;
    }
}

// circles: rings from the middle out, the largest units innermost; on a ring by the angle they stand at now
internal void fo_rings(int *idx, int n) {
    const float TWO_PI = 6.28318531f;
    fo_sort_by(idx, n, 2);
    float r = 0, prev = 0;
    for (int i = 0, ring = 0; i < n; ring++) {
        float deep = g_fo[idx[i]].w;   // sorted: the ring's first is its largest
        r = ring ? r + (prev + deep) * 0.5f : deep * 0.75f;
        float sum = 0;
        int k = 0;
        while (i + k < n && (k == 0 || sum + g_fo[idx[i + k]].w <= TWO_PI * r)) sum += g_fo[idx[i + k++]].w;
        if (i + k == n && k == 1 && ring == 0) { g_fo[idx[i]].at = v2(0, 0); break; }   // one unit: the middle
        int *ix = idx + i;
        for (int a = 0; a < k; a++) g_fo_key[ix[a]] = -fo_pseudo_angle(g_fo[ix[a]].p);
        fo_sort(ix, k, g_fo_key);   // counter-clockwise from +x, as the ring is laid
        float at = 0;
        for (int a = 0; a < k; a++) {
            float w = g_fo[ix[a]].w;
            g_fo[ix[a]].at = vec2_scale(fo_dir(TWO_PI * (at + w * 0.5f) / sum), r);
            at += w;
        }
        prev = deep;
        i += k;
    }
}

internal void *g_fo_slots;   // the slots of the group move being dispatched, laid out (NULL: the game's own)
internal int g_fo_n;

internal void __thiscall hook_formation_slots(void *f, void *group, void *slots) {
    formation_slots(f, group, slots);
    g_fo_slots = NULL;
    int n = formation_slot_count(slots), kind = g_set.formation_kind >= 0 ? g_set.formation_kind : formation_get_kind(f);   // (tests: a kind's layout without its button)
    if (!g_fo_on || n < 2 || n > FO_MAX || kind < 0 || kind > FORMATION_OUTWARD_CIRCLE) return;
    vec3 c3 = formation_get_center(f);
    vec2 c = v2(c3.x, c3.y), mean = v2(0, 0);
    for (int i = 0; i < n; i++) {
        struct fo_unit *u = &g_fo[i];
        u->u = fslot_get_unit(formation_slot(slots, i));
        if (!u->u) return;
        vec3 p = unit_get_pos(u->u);
        u->p = v2(p.x, p.y);
        float s = unit_get_size(u->u);
        u->w = (s < FO_MIN_SIZE ? FO_MIN_SIZE : s) * FO_PAD[kind];
        mean = vec2_add(mean, u->p);
        g_fo_order[i] = i;
    }
    mean = vec2_scale(mean, 1.0f / (float)n);
    vec2 fwd = vec2_sub(c, mean);
    float len = vec2_len(fwd);
    fwd = len > 0.5f ? vec2_scale(fwd, 1.0f / len) : v2(0, 1);
    vec2 right = v2(fwd.y, -fwd.x);
    for (int i = 0; i < n; i++) {
        vec2 d = vec2_sub(g_fo[i].p, mean);
        g_fo[i].along = vec2_dot(d, fwd);
        g_fo[i].across = vec2_dot(d, right);
        g_fo[i].p = vec2_sub(g_fo[i].p, c);   // the circles' angles: around the point
    }
    int circle = kind >= FORMATION_INWARD_CIRCLE;
    if (circle) fo_rings(g_fo_order, n); else fo_rows(g_fo_order, n, kind);
    // the shape's middle on the clicked point (a circle's already is), turned to face the move
    vec2 mid = v2(0, 0);
    if (!circle) {
        for (int i = 0; i < n; i++) mid = vec2_add(mid, g_fo[i].at);
        mid = vec2_scale(mid, 1.0f / (float)n);
    }
    // the game's slots as they were: a circle's facings are taken from its nearest one
    static vec2 game_at[FO_MAX];
    static uint32_t game_facing[FO_MAX];
    for (int i = 0; i < n; i++) {
        void *s = formation_slot(slots, i);
        vec3 p = fslot_get_pos(s);
        game_at[i] = v2(p.x, p.y);
        game_facing[i] = fslot_get_facing(s);
    }
    // each unit's spot, and the slots moved there (the game pairs units with them by these points)
    for (int i = 0; i < n; i++) {
        struct fo_unit *u = &g_fo[i];
        vec2 a = vec2_sub(u->at, mid);
        u->at = circle ? vec2_add(c, a) : vec2_add(c, vec2_add(vec2_scale(right, a.x), vec2_scale(fwd, a.y)));
        int nearest = 0;
        if (circle)
            for (int k = 1; k < n; k++) if (vec2_dist2(game_at[k], u->at) < vec2_dist2(game_at[nearest], u->at)) nearest = k;
        u->facing = game_facing[nearest];
        void *s = formation_slot(slots, i);
        vec3 p = fslot_get_pos(s);
        p.x = u->at.x;
        p.y = u->at.y;
        fslot_set_pos(s, p);
        fslot_set_facing(s, u->facing);
    }
    g_fo_slots = slots;
    g_fo_n = n;
    g_fo_moves++;
}

// a unit's location goal in the dispatch: its own spot (and facing) instead of the slot it was paired with - the slot
// found by the point the dispatch copied from it, the unit it was paired with in the slot
internal void *__thiscall hook_slot_goal(ee_goal *g, float *pt, uint32_t a, uint32_t b, uint32_t c, uint32_t facing, uint32_t e, uint32_t f) {
    if (g_fo_slots && pt && formation_slot_count(g_fo_slots) == g_fo_n)
        for (int i = 0; i < g_fo_n; i++) {
            void *s = formation_slot(g_fo_slots, i);
            vec3 p = fslot_get_pos(s);
            if (p.x != pt[0] || p.y != pt[1]) continue;
            ee_unit *u = fslot_get_unit(s);
            for (int k = 0; k < g_fo_n; k++)
                if (g_fo[k].u == u) { pt[0] = g_fo[k].at.x; pt[1] = g_fo[k].at.y; facing = g_fo[k].facing; break; }
            break;
        }
    return goal_move_at(g, pt, a, b, c, facing, e, f);
}

internal void groupmove_install(void) {
    g_fo_on = g_set.formations;
    int n = g_fo_on ? patch_formation_slots(hook_formation_slots) : 0, goals = n ? patch_slot_goal(hook_slot_goal) : 0;
    if (g_log) fprintf(g_log, "# groupmove: formations=%d (%d formation vtables, goals hooked %d)\n", g_fo_on, n, goals);
}
