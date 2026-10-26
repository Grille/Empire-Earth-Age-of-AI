// FORMATIONS BY SIZE (`formations`; docs/engine-notes.md "Group moves"): a group move puts each unit on a slot of the
// group's formation, and the game spaces every row and column of it by the largest unit's collision size (type +0x470)
// - one tank stretches the grid of 170 soldiers - in as few as 5 columns, the rows stacked behind the clicked point,
// and pairs units with slots by distance alone (a tank may get a slot among infantry): a 174-unit army sent 20 tiles
// ended with half of it more than 28 tiles from the click (tests/probes/army_move.eeai). Here every formation is laid
// out again, for any group, from its units' own sizes, its front row on the clicked point and facing the way the game's
// own slots face (a right-button drag turns them), keeping its kind's shape:
//   lines (short, long, loose, staggered): rows of FO_ASPECT[kind] times as wide as deep, each unit as wide as its
//     size x FO_PAD[kind], a row as deep as its largest unit, a staggered line's every other row shifted half a unit;
//   wedges (deep, shallow): rows from a single tip, each FO_WEDGE_GROW[kind] units wider than the one before;
//   circles (inward, outward): rings from the middle out, the largest units innermost, as wide as their units.
// Each unit has a rank (fo_rank: the close fighters in front, then the ranged soldiers and the healing heroes, siege,
// and who doesn't fight at the back; a circle has them inside). Units of one rank and like size are put in bands, each
// in rows of its own (a row is as deep as its largest unit), the ranks front to back, a rank's bands and the rows in
// them by how far ahead their units stand now along the move, the units in a row left to right by where they stand
// across it, so no one walks through the group more than the ranks ask. The game then pairs units with these slots as before
// (that block also prepares the group's route: skipping it sent units on detours), and each unit's goal is given its own
// planned spot instead of its paired slot's (hook_slot_goal: the slot found by its point, the unit by the slot). The
// collision sorts out the rest. Only + - * / and sqrtf (the circles' sines from a polynomial): the same on every PC.

#define FO_MAX 2048        // units laid out; a larger group keeps the game's slots
#define FO_MIN_SIZE 0.3f   // a unit's size counted at least this (a citizen 0.3)
#define FO_BAND 0.85f      // units this much of a band's largest or more share its rows
internal const float FO_PAD[8] = { 1.4f, 1.4f, 2.0f, 1.4f, 1.25f, 1.25f, 1.2f, 1.2f };   // by FORMATION_* kind
internal const float FO_ASPECT[4] = { 2.0f, 6.0f, 2.0f, 3.0f };                          // a line's width : depth
// a wedge's row this many units wider than the one before (deep 1, 2, 3 ..., shallow 1, 3, 5 ...) - a row's width
// from its depth gave the deep wedge a tip of two units in a line (its second row too narrow for two) and the shallow
// one a lone tip before 2, 4 ...
internal const float FO_WEDGE_GROW[2] = { 1.0f, 2.0f };

internal int g_fo_on;
internal unsigned g_fo_moves;   // group moves laid out

struct fo_unit { ee_unit *u; vec2 p, from; float w, along, across; vec2 at; int band, rank; uint32_t facing; };   // at: its spot, from: where it sets out
internal struct fo_unit g_fo[FO_MAX];
internal int g_fo_order[FO_MAX];

// A unit's rank, front to back (a circle's outside in): 0 who fights up close or takes the hits - melee range, the tanks
// and mechs, the military heroes (class 23: Gilgamesh .. Molotov, the fighters); 1 the ranged soldiers and the
// strategist heroes (class 13 of family hero: they heal those around them); 2 siege and rams; 3 who doesn't fight -
// citizens (the diplomat and engineer too), priests and prophets, and whatever has no attack (the medics)
#define FO_RANGED 2.0f   // a range of this many tiles or more: a ranged soldier (melee 0.5 .. 1.3, a javelin 3)
internal int fo_rank(ee_unit *u) {
    ee_unit_type *td = unit_get_type_data(u);
    if (!td) return 1;
    int cls = unit_type_get_class_id(td), kind = unit_type_get_family(td), type = unit_type_get_id(td);
    ee_unit_stats *st = unit_get_stats(u);
    float range = st ? unit_stats_get_range(st) : unit_type_get_range(td);
    float attack = st ? unit_stats_get_attack(st) : (float)unit_type_get_attack(td);
    if (cls == CLS_LAND_CITIZEN || is_citizen_type(type) || attack <= 0) return 3;
    if (cls == CLS_LAND_MILITARY_HERO) return 0;
    if (cls == CLS_LAND_PRIEST) return kind == KIND_HERO ? 1 : 3;
    if (is_siege(type) || is_ram(type)) return 2;
    if (kind == KIND_TANK || kind == KIND_TANK_STRONG || (kind >= KIND_MECH_MINOTAUR && kind <= KIND_MECH_CYCLOPS)
        || kind == KIND_MECH_APOLLO) return 0;
    return range >= FO_RANGED ? 1 : 0;
}

// order idx[0..n) by key descending (insertion sort: stable, and n is a group)
internal void fo_sort(int *idx, int n, const float *key) {
    for (int i = 1; i < n; i++) {
        int v = idx[i], j = i;
        while (j > 0 && key[idx[j - 1]] < key[v]) { idx[j] = idx[j - 1]; j--; }
        idx[j] = v;
    }
}
internal float g_fo_key[FO_MAX];
// 0 along (front first), 1 across (left first), 2 size (largest first), 3 rank (front first), 4 rank (back first)
internal void fo_sort_by(int *idx, int n, int what) {
    for (int i = 0; i < n; i++) {
        struct fo_unit *f = &g_fo[idx[i]];
        g_fo_key[idx[i]] = what == 0 ? f->along : what == 1 ? -f->across : what == 2 ? f->w : what == 3 ? (float)-f->rank : (float)f->rank;
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

// the direction share s (0..1) of the turn from a to b, the shorter way round (exactly opposite: counter-clockwise),
// by halving the turn (to 1/1024)
internal vec2 fo_half(vec2 u, vec2 v) {
    vec2 m = vec2_add(u, v);
    float l = vec2_len(m);
    return l > 1e-4f ? vec2_scale(m, 1.0f / l) : v2(-u.y, u.x);
}
internal vec2 fo_turn(vec2 a, vec2 b, float s) {
    float lo = 0, hi = 1;
    for (int i = 0; i < 10; i++) {
        vec2 m = fo_half(a, b);
        float mid = (lo + hi) / 2;
        if (s < mid) { b = m; hi = mid; } else { a = m; lo = mid; }
    }
    return fo_half(a, b);
}

// atan2(y, x) from + - * / alone (the same on every PC): the octant reduced to [0, 1], then a polynomial (error
// under 1e-5 radians)
internal float fo_atan2(float y, float x) {
    const float PI = 3.14159265f;
    float ax = fabsf(x), ay = fabsf(y), mx = ax > ay ? ax : ay, mn = ax > ay ? ay : ax;
    if (mx < 1e-12f) return 0;
    float t = mn / mx, t2 = t * t;
    float r = t * (0.99997726f + t2 * (-0.33262347f + t2 * (0.19354346f + t2 * (-0.11643287f + t2 * (0.05265332f + t2 * -0.01172120f)))));
    if (ay > ax) r = PI / 2 - r;
    if (x < 0) r = PI - r;
    return y < 0 ? -r : r;
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

// lines and wedges: rows front to back (y 0, then negative), each filled to its width - a line's at least as wide as
// the game's own front row of `front` units (its 5 columns: 3 tanks side by side, not 2 and 1 behind)
internal void fo_rows(int *idx, int n, int kind, int front) {
    fo_sort_by(idx, n, 0);
    // bands of one rank and like size (within FO_BAND of the band's largest), each in rows of its own - a row is as
    // deep as its largest unit, so tanks among infantry spaced every row out; the ranks front to back, a rank's bands
    // by where they stand on average, the units within each by where they stand (the sorts are stable)
    static int by_size[FO_MAX];
    static float band_along[FO_MAX];
    static int band_rank[FO_MAX];
    memcpy(by_size, idx, sizeof *idx * (size_t)n);
    fo_sort_by(by_size, n, 2);
    fo_sort_by(by_size, n, 3);
    for (int i = 0, b = -1, first = 0; i < n; i++) {
        struct fo_unit *f = &g_fo[by_size[i]];
        if (b < 0 || f->rank != band_rank[b] || f->w < g_fo[by_size[first]].w * FO_BAND) {
            if (b >= 0) band_along[b] /= (float)(i - first);
            b++; first = i; band_along[b] = 0; band_rank[b] = f->rank;
        }
        f->band = b;
        band_along[b] += f->along;
        if (i == n - 1) band_along[b] /= (float)(i + 1 - first);
    }
    for (int i = 0; i < n; i++) g_fo_key[idx[i]] = band_along[g_fo[idx[i]].band];
    fo_sort(idx, n, g_fo_key);
    fo_sort_by(idx, n, 3);
    float area = 0, wmax = 0, wmean = 0;
    for (int i = 0; i < n; i++) { float w = g_fo[idx[i]].w; area += w * w; wmean += w; if (w > wmax) wmax = w; }
    wmean /= (float)n;
    int wedge = kind == FORMATION_DEEP_WEDGE || kind == FORMATION_SHALLOW_WEDGE;
    float width = wedge ? 0 : sqrtf(area * FO_ASPECT[kind]);
    if (!wedge && width < wmean * (float)front) width = wmean * (float)front;
    if (width < wmax) width = wmax;
    float y = 0, prev = 0;
    for (int i = 0, row = 0; i < n; row++) {
        // the row's units: as many as its width takes (at least one), its depth their largest
        float cap = wedge ? g_fo[idx[i]].w * (1 + FO_WEDGE_GROW[kind - FORMATION_DEEP_WEDGE] * (float)row) : width, sum = 0, deep = 0;
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

// circles: rings from the middle out, the ranks back first (who doesn't fight in the middle, the fighters outside), a
// rank's largest units innermost; on a ring by the angle they stand at now
internal void fo_rings(int *idx, int n) {
    const float TWO_PI = 6.28318531f;
    fo_sort_by(idx, n, 2);
    fo_sort_by(idx, n, 4);
    float r = 0, prev = 0;
    for (int i = 0, ring = 0; i < n; ring++) {
        // the ring's radius from its largest unit (no longer always its first: the ranks come first) - its units taken
        // at a radius from the first, then the radius from their largest and the units again
        float deep = g_fo[idx[i]].w, sum = 0, r0 = r, rest = 0;
        int k = 0;
        for (int pass = 0; pass < 2; pass++) {
            r = ring ? r0 + (prev + deep) * 0.5f : deep * 0.75f;
            sum = 0; k = 0;
            while (i + k < n && (k == 0 || sum + g_fo[idx[i + k]].w <= TWO_PI * r)) sum += g_fo[idx[i + k++]].w;
            for (int a = 0; a < k; a++) if (g_fo[idx[i + a]].w > deep) deep = g_fo[idx[i + a]].w;
        }
        // the rest all on this ring when it holds them at no more than the next ring's radius - else 8 submarines were 4
        // on a small ring and 4 on the next at the same angles: a cross
        for (int a = i; a < n; a++) rest += g_fo[idx[a]].w;
        if (rest / TWO_PI <= r + deep) {
            for (int a = i + k; a < n; a++) if (g_fo[idx[a]].w > deep) deep = g_fo[idx[a]].w;
            if (r < rest / TWO_PI) r = rest / TWO_PI;
            k = n - i; sum = rest;
        }
        if (i + k == n && k == 1 && ring == 0) { g_fo[idx[i]].at = v2(0, 0); break; }   // one unit: the middle
        int *ix = idx + i;
        for (int a = 0; a < k; a++) g_fo_key[ix[a]] = -fo_pseudo_angle(g_fo[ix[a]].p);
        fo_sort(ix, k, g_fo_key);   // counter-clockwise from +x, as the ring is laid
        float at = ring & 1 ? sum * 0.5f / (float)k : 0;   // every other ring turned half a unit: no spokes
        for (int a = 0; a < k; a++) {
            float w = g_fo[ix[a]].w;
            float ang = TWO_PI * (at + w * 0.5f) / sum;
            g_fo[ix[a]].at = vec2_scale(fo_dir(ang >= TWO_PI ? ang - TWO_PI : ang), r);
            at += w;
        }
        prev = deep;
        i += k;
    }
}

internal void *g_fo_slots;   // the slots of the group move being dispatched, laid out (NULL: the game's own)
internal int g_fo_n;
internal ee_active_group *g_fo_group;   // its group, till its march is set (formation_march)
internal unsigned g_fo_marches;         // groups set to march at their slowest unit's speed
#define FO_CLICK 0.985f                 // cos 10 degrees: a queued move's facing this near the way from where they stand
internal unsigned g_fo_refaced;         // queued moves turned to the way from the last order's points
internal int g_fo_queued;               // the move being carried out is queued after the units' others (shift)
internal vec2 g_fo_march[FO_MAX];       // the march's own layout (formation_follow: the way it goes off the final facing)
internal int g_fo_has_march;
internal int g_fo_naval;                // the layout's units are ships (formation_follow: they turn together)
internal unsigned g_fo_queued_n;        // group moves laid out from the units' last queued points

// the move (EEMPathTo's Execute, the formation's slots laid out within it): whether it is queued - then the formation is
// laid out from where each unit's last order leaves it, not where it stands
internal void __thiscall hook_path_execute(ee_path_msg *msg) {
    g_fo_queued = path_msg_get_queued(msg);
    path_msg_execute(msg);
    g_fo_queued = 0;
}

// `formation_march`: the group marches at its slowest unit's speed. The group's together flag set makes every member's
// speed getter return the group's speed, acceleration and turns, which its vf19 works out from the members' - again
// whenever members come or go (one arrives and leaves), so that is hooked: the speed then the slowest member's instead
// of their average. Set when the units get their goals, not with the move's own together flag: that one also skips
// the dispatch's pairing of units and slots for small fast units, and with it the group's route preparation (detours).
// Only the groups marched here (the last FO_MARCHED): the game's own together moves keep the game's speed
#define FO_MARCHED 64
internal ee_active_group *g_fo_marched[FO_MARCHED];
internal unsigned g_fo_marched_n;
internal void __thiscall hook_group_speeds(ee_active_group *grp) {
    active_group_speeds(grp);
    if (!active_group_get_together(grp)) return;
    for (int i = 0; i < FO_MARCHED; i++)
        if (g_fo_marched[i] == grp) { active_group_set_speed(grp, active_group_get_speed_min(grp)); return; }
}
internal void fo_march(ee_active_group *grp) {
    g_fo_marched[g_fo_marched_n++ % FO_MARCHED] = grp;
    active_group_set_together(grp, 1);
    hook_group_speeds(grp);
    g_fo_marches++;
}

// FOLLOWING (`formation_follow`, with formations; step 2 of moving in formation): a group laid out here marches in its
// formation instead of each unit running its own route to its spot. It hangs on an anchor that marches like a ship
// (ff_leader): set out from the leader - the unit whose spot is nearest the formation's middle -, heading for the
// leader's route FF_LOOK ahead, its heading turned no faster than an arc that leaves the innermost unit FF_INNER inside
// it (every unit walks forward through a turn instead of the formation wheeling round a leader on the route), at the
// slowest member's top speed, slower in a turn so the outermost unit keeps up. Every land unit, the leader too, steers
// each tick at its point (its spot's offset from the leader's, from the anchor, turned with the formation's facing: the
// anchor's heading - its front rank leads - and over the last FF_SETTLE seconds turned to the final facing, longer when
// its outermost units need the time to swing round, at most FF_SETTLE_MAX; with a move queued after it, the heading all
// along) plus FF_AHEAD along the heading, through the turn (shiproute.c's hook of the land models' slot 7), and its speed
// (the land models' speed step) is the anchor's plus FF_GAIN for each tile it is behind its point, while behind up to
// formation_catchup (200%: near its point, or a few seconds after it was) of its own top speed. Its own route stays: whenever its point is further than FF_FAR, on a tile it may not
// enter or not in a clear line, it takes its route for that tick; while it follows, the waypoints it passed (or that
// are within FF_DROP in a clear line) are dropped so the route is where it stands. Once the anchor is within FF_END of
// the leader's spot, or the leader is gone or given another order, the march ends: each unit goes on to its own spot.
// The members' speeds are their own then (formation_march's group speed would hold the ones behind back). A queued move
// (shift) is laid out from the units' last order's points and its march sets out when the leader takes it up: the
// march before it goes on till then; turning less than 45 degrees, the same formation goes on - the anchor runs through
// the corner, turning into the next leg FF_LOOK before it, the leader's leg ended there. Units far apart (two groups
// sent to the point between them) march as blocks, each behind a leader of its own (ff_start). All from the game's
// state in the simulation: the same on every PC
#define FF_GROUPS 64
#define FF_UNITS 4096          // the followers' table (open, by unit id)
#define FF_FAR 24.0f           // tiles
#define FF_END 3.0f            // tiles
#define FF_GAIN 0.5f           // tiles/s for each tile behind
#define FF_AHEAD 1.5f          // tiles: the point steered at lies this far ahead of the follower's own (no circling on it)
#define FF_LOOK 3.0f           // tiles: the anchor heads for the leader's route this far ahead of it
#define FF_INNER 1.0f          // tiles: the anchor's tightest arc leaves the innermost unit this far inside it
#define FF_PACE 0.8f           // the formation goes at most this share of each member's speed now (its top speed and its
                               // catch-up share): one fallen behind at its own top speed holds it to 80% of that, and all
                               // with 125% or more (catch-up near their places) let it go at the march's speed
#define FF_DROP 2.0f           // tiles
#define FF_FALLEN 4.0f         // x its size: a follower this far from its point and further has no catch-up speed
#define FF_KEEP 10             // steps: a unit steered at its point this recently keeps the formation's speed - a ship
                               // isn't steered every tick, and the game's step in between braked it: its speed a
                               // sawtooth, and every speed-up threw out a bow wave (the player: far too dense)
#define FF_GRACE 5.0f          // s: ... unless it was in its place (or its march began: a re-form) this long ago or less
#define FF_FRESH 20            // steps: a unit whose catch-up share was reckoned this recently comes from a march, else from rest
#define FF_LINK 4.0f           // tiles: units this close where they set out (chained) march as one block - the gap
                               // between their spots' edges (their sizes padded as laid out: a fixed 4 tiles between
                               // the centres left ships of a fleet out, each on its own route - the player)
#define FF_SETTLE 5.0f         // s: the formation turns from the way to its final facing this long before the leader arrives,
#define FF_SETTLE_MAX 15.0f    // up to this long when its outermost units need it to swing round at its speed
#define FF_SWING 1.0f          // its facing turns at most this much x its speed / its reach a second (its outermost units
                               // swing round at about its speed)
#define FF_ARC 0.7071f         // a queued move turning less than this (cos 45 degrees) goes on in the same formation
#define FF_EASE 2.0f           // s: formation_collision fades in over this long from the march's start (and out after the
#define FF_LINGER 10.0f        // linger): the formation lingers this long after it arrived, all of it still in effect, and
                               // an idle unit pushed off its spot meanwhile walks back to it (ff_linger_tick) -
#define FF_HOME_SLOP 0.5f      // x its size (at least FF_HOME_MIN): further off than this,
#define FF_HOME_MIN 0.25f      // tiles
#define FF_HOME_FAR 3.0f       // tiles: and no further (one further off didn't stray by a push: it never got there, blocked)
#define FF_HOME_TRIES 3        // walks back at most in a linger (a spot it can't reach - behind a wall - isn't tried for ever)
#define FF_HOME_EVERY 10       // steps: the idle units looked at this often
// fwd: the formation's final facing, way: where it goes, face: its facing now, reach: its outermost unit's distance from
// the leader, goal: the leader's goal of this move (NULL: not known - its group
// tells), live: on its way this step (else queued behind the leader's current order), lead_off: the leader's spot from
// the formation's point in its frame, through: a queued march goes on from it in the same formation (it doesn't end
// short of the leader's spot), onward: then the next leg's way, passed: the anchor past the leader's spot then; anchor:
// the point the formation hangs on (ff_leader), way its heading, vel its speed, turning its share of the tightest turn
// last step, started: set out (prev: the march it went on from, else -1), t0 the step then, linger the step it arrived
// (0: not yet, or not arrived - given another order)
struct ff_group { uint32_t leader; ee_group *grp; ee_goal *goal; vec2 fwd, way, face, end, lead_off, onward, anchor, way_from, face_from, way_rot, face_rot; float speed, reach, turn_r, acc, pace, pace_next, vel, vel_from, turning, dt; uint32_t step, t0, linger, at_ms; float sp, front, rear, fw; int n, cols; int on, live, through, passed, started, prev, reform, arc, pivot, naval; };   // sp: its members' mean spacing, front: the front rank's offset, fw: the formation's width, n: its units, cols: narrowed to this many abreast (0: as laid out)   // arc: it goes on from prev in the same formation (else it re-forms on the move: pivot - its anchor turns onto the new leg sharply); naval: ships
internal struct ff_group g_ff[FF_GROUPS];
internal unsigned g_ff_next;
internal int g_ff_first, g_ff_count;   // the marches started by the move being dispatched, for their leaders' goals
// step: the last it followed; pend (1-based, into g_ff_p): the first of the queued moves' marches it joins, each when it
// sets out; home its spot at the goal
struct ff_unit { uint32_t id; int g, pend; vec2 off, home; uint32_t step; float along, with, off_by, share, grace_cap, want; vec2 lag_pos; uint32_t in_at, share_at, yaw, lag_at, rerouted_at; int tries, own, out, order; };   // order: its place front to back, left to right in its formation (ff_off); out: dropped out of its march (lag_at, lag_pos: since when it lags, where it stood then); own: why it took its own route last (1 its point too far, 2 the point steered at on a tile it may not enter, 3 not in a clear line); in_at: the step it was last in its place (or its march began); share its catch-up share at share_at, grace_cap the most of it the grace gives; yaw its facing there (the slot's float, 0: none), tries its walks back
internal struct ff_unit g_ff_u[FF_UNITS];
// a queued move's march a unit joins once it sets out (its offset, its spot and facing at the goal, its goal in that move),
// after (1-based) the one queued behind it - a unit had one, so a third waypoint's took the second's place and from the
// second on each unit went its own way (a tank at its own top speed)
#define FF_PEND 4096
struct ff_pend { uint32_t id; int g, after, order; vec2 off, home; uint32_t yaw; ee_goal *goal; };
internal struct ff_pend g_ff_p[FF_PEND];
internal unsigned g_ff_p_next;
internal int g_ff_on;
internal unsigned g_ff_flows;   // queued re-forms that took the march before's anchor over
internal unsigned g_ff_centre;  // follower ticks steered along the formation's line, their points out of reach
internal unsigned g_ff_out;     // units dropped out of their marches (held up, they would hold the formation back)
internal unsigned g_ff_marches, g_ff_split, g_ff_arcs, g_ff_finished, g_ff_steered, g_ff_own[4];   // marches started, moves split into blocks; follower ticks steered, left to the own route (by e->own)

internal uint32_t ff_step(void);

// a march begins for e: its grace no more than the catch-up share it had in the march before at that moment (a straggler
// with none got the full boost from the re-form on, all its way back), none from rest (a group standing about in a jumble
// takes its time to assemble)
internal void ff_grace_from(struct ff_unit *e) {
    uint32_t step = ff_step();
    e->in_at = 0;
    e->grace_cap = e->share_at && step - e->share_at <= FF_FRESH ? e->share : 0;
}

// e's queued march i (1-based), NULL for none or one overwritten since (the table is a ring)
internal struct ff_pend *ff_pend_of(struct ff_unit *e, int i) {
    struct ff_pend *p = i > 0 && i <= FF_PEND ? &g_ff_p[i - 1] : NULL;
    return p && p->id == e->id ? p : NULL;
}

// e's last queued march, NULL for none
internal struct ff_pend *ff_pend_last(struct ff_unit *e) {
    struct ff_pend *p = ff_pend_of(e, e->pend), *q;
    for (int k = 0; p && k < FF_GROUPS && (q = ff_pend_of(e, p->after)); k++) p = q;
    return p;
}

internal struct ff_unit *ff_find(uint32_t id, int make) {
    if (!id) return NULL;
    uint32_t h = (id * 2654435761u) % FF_UNITS;
    for (int k = 0; k < 16; k++) {
        struct ff_unit *e = &g_ff_u[(h + (uint32_t)k) % FF_UNITS];
        if (e->id == id) return e;
        if (!e->id) { if (!make) return NULL; memset(e, 0, sizeof *e); e->id = id; return e; }
    }
    if (!make) return NULL;
    struct ff_unit *e = &g_ff_u[h];   // full around it: the slot taken over
    memset(e, 0, sizeof *e);
    e->id = id;
    return e;
}

// one block's march (idx[0..k) of g_fo, their spots .at around the point c) at the formation's speed, facing fwd;
// queued: it sets out once its leader takes up this move, the units' marches till then go on; from (>= 0): the march it
// goes on from in the same formation (its leader, its facing to turn from)
internal void ff_start_block(void *group, const int *idx, int k, vec2 c, vec2 fwd, vec2 right, int queued, float slow, int from) {
    vec2 mid = v2(0, 0);
    for (int j = 0; j < k; j++) mid = vec2_add(mid, g_fo[idx[j]].at);
    mid = vec2_scale(mid, 1.0f / (float)k);
    int lead = idx[0];
    for (int j = 1; j < k; j++) if (vec2_dist2(g_fo[idx[j]].at, mid) < vec2_dist2(g_fo[lead].at, mid)) lead = idx[j];
    int before = -1;   // queued, re-forming: the march its units are on or queued for last
    for (int j = 0; queued && from < 0 && j < k && before < 0; j++) {
        struct ff_unit *e = ff_find((uint32_t)unit_get_serial(g_fo[idx[j]].u), 0);
        struct ff_pend *p = e ? ff_pend_last(e) : NULL;
        int b = p ? p->g : e ? e->g : -1;
        if (b >= 0 && g_ff[b].on && !g_ff[b].through) before = b;
    }
    // the leader that march's: this one sets out when that leader's leg ends (another's leg still ran to its spot)
    int keep = from >= 0 ? from : before;
    if (keep >= 0)
        for (int j = 0; j < k; j++) if ((uint32_t)unit_get_serial(g_fo[idx[j]].u) == g_ff[keep].leader) lead = idx[j];
    int gi = (int)(g_ff_next++ % FF_GROUPS);
    struct ff_group *gr = &g_ff[gi];
    gr->leader = (uint32_t)unit_get_serial(g_fo[lead].u);
    gr->grp = (ee_group *)group;
    gr->goal = NULL;
    gr->fwd = fwd;
    vec2 to = vec2_sub(g_fo[lead].at, g_fo[lead].from);
    float tl = vec2_len(to);
    gr->way = gr->face = tl > 0.01f ? vec2_scale(to, 1.0f / tl) : fwd;
    gr->naval = g_fo_naval;
    gr->end = g_fo[lead].at;
    vec2 lc = vec2_sub(gr->end, c);
    gr->lead_off = v2(vec2_dot(lc, right), vec2_dot(lc, fwd));
    gr->through = gr->passed = gr->started = gr->pivot = 0;
    gr->arc = from >= 0;
    gr->reform = from < 0 && g_fo_has_march;
    gr->prev = from;
    gr->vel = gr->vel_from = gr->turning = gr->dt = 0;
    gr->way_rot = gr->face_rot = v2(1, 0);
    gr->onward = v2(0, 0);
    if (from >= 0) {
        vec2 on = vec2_sub(gr->end, g_ff[from].end);
        float ol = vec2_len(on);
        g_ff[from].onward = ol > 0.01f ? vec2_scale(on, 1.0f / ol) : gr->way;
        gr->face = g_ff[from].face;
        g_ff[from].through = 1;
        g_ff_arcs++;
    }
    gr->reach = gr->turn_r = 0;
    gr->acc = 1e9f;
    for (int j = 0; j < k; j++) {   // its least agile member's turning circle at its top speed (a ship's: it can't turn on the
        float w = unit_max_turn(g_fo[idx[j]].u), tr = w > 0 ? unit_max_speed(g_fo[idx[j]].u) / w : 0, a = unit_accel(g_fo[idx[j]].u);   // spot), its slowest acceleration
        if (tr > gr->turn_r) gr->turn_r = tr;
        if (a < gr->acc) gr->acc = a;
    }
    gr->speed = slow;
    gr->step = ~0u;
    gr->t0 = gr->linger = 0;
    gr->on = 1;
    gr->live = 0;
    gr->pace = gr->pace_next = -1;
    // the offsets, and each unit's place front to back, left to right (a narrowed formation fills its rows in that order)
    static vec2 offs[FO_MAX];
    static int ord[FO_MAX], rank_of[FO_MAX];
    float xl = 0, xr = 0;
    gr->sp = 0;
    gr->front = -1e9f;
    gr->rear = 0;
    for (int j = 0; j < k; j++) {
        int i = idx[j];
        vec2 d = vec2_sub(g_fo[i].at, g_fo[lead].at), off = v2(vec2_dot(d, right), vec2_dot(d, fwd));
        if (gr->reform) off = vec2_sub(g_fo_march[i], g_fo_march[lead]);   // the march's own layout (its frame: x right, y ahead)
        offs[j] = off;
        ord[j] = j;
        if (off.x < xl) xl = off.x;
        if (off.x > xr) xr = off.x;
        if (off.y > gr->front) gr->front = off.y;
        if (off.y < gr->rear) gr->rear = off.y;
        gr->sp += g_fo[i].w / (float)k;   // the mean spacing (at the widest a mixed army's narrowed column ran out long)
    }
    for (int a = 1; a < k; a++)   // insertion sort: front (larger y) first, then left
        for (int b = a; b > 0; b--) {
            vec2 p = offs[ord[b - 1]], q = offs[ord[b]];
            if (p.y > q.y + 0.25f || (p.y > q.y - 0.25f && p.x <= q.x)) break;
            int t = ord[b]; ord[b] = ord[b - 1]; ord[b - 1] = t;
        }
    for (int a = 0; a < k; a++) rank_of[ord[a]] = a;
    gr->fw = xr - xl + gr->sp;   // about
    gr->n = k;
    gr->cols = 0;
    for (int j = 0; j < k; j++) {
        int i = idx[j];
        struct ff_unit *e = ff_find((uint32_t)unit_get_serial(g_fo[i].u), 1);
        vec2 off = offs[j];
        if (vec2_len(off) > gr->reach) gr->reach = vec2_len(off);
        if (queued && ((e->g >= 0 && e->g != gi && g_ff[e->g].on) || ff_pend_of(e, e->pend))) {   // behind the ones it has
            int pi = (int)(g_ff_p_next++ % FF_PEND);
            struct ff_pend *p = &g_ff_p[pi], *last = ff_pend_last(e);
            p->id = e->id; p->g = gi; p->after = 0; p->off = off; p->order = rank_of[j]; p->home = g_fo[i].at; p->yaw = g_fo[i].facing; p->goal = NULL;
            if (last) last->after = pi + 1;
            else e->pend = pi + 1;
            continue;
        }
        e->g = gi;
        e->pend = 0;
        e->out = 0;
        e->lag_at = 0;
        e->off = off;
        e->order = rank_of[j];
        e->home = g_fo[i].at;
        e->step = ~0u;
        ff_grace_from(e);
        e->yaw = g_fo[i].facing;
        e->tries = 0;
    }
    // a queued re-form (45 degrees or more) after a march its units are on: that one runs through its leader's spot as an
    // arc's does and this one takes its anchor over - the units re-form round it on the move, the pace slowing it while they
    // do; it ended FF_END short and the units went on to their spots on their own routes (a tank rushing ahead), and
    // holding it at its end stopped the formation at every waypoint
    if (from < 0 && before >= 0 && before != gi && g_ff[before].on && !g_ff[before].through) {
        struct ff_group *pv = &g_ff[before];
        vec2 on = vec2_sub(gr->end, pv->end);
        float ol = vec2_len(on);
        pv->onward = ol > 0.01f ? vec2_scale(on, 1.0f / ol) : gr->way;
        pv->through = 1;
        gr->prev = before;
        g_ff_flows++;
    }
    if (g_ff_count++ == 0) g_ff_first = gi;
    g_ff_marches++;
}

// the marches of the units just laid out (g_fo[0..n)): a block each - the units within FF_LINK of each other where they
// set out, chained (two blocks sent to the point between them can't follow one leader: each goes to its part of the
// formation behind a leader of its own); a unit on its own takes its own route. All at the slowest member's top speed
internal int g_ff_root[FO_MAX], g_ff_idx[FO_MAX];
internal int ff_root(int i) { while (g_ff_root[i] != i) i = g_ff_root[i] = g_ff_root[g_ff_root[i]]; return i; }
internal void ff_start(void *group, int n, vec2 c, vec2 fwd, vec2 right, int queued, int arc) {
    g_ff_count = 0;
    float slow = 1e9f;
    int naval = -1;   // land units or ships and submarines, not both: they can't keep one formation
    for (int i = 0; i < n; i++) {
        ee_unit_type *td = unit_get_type_data(g_fo[i].u);
        int kind = td ? model_kind(unit_type_get_model(td)) : 0, nv = kind == MODEL_SHIPS || kind == MODEL_SUBS;
        if (!kind || (naval >= 0 && nv != naval)) return;
        naval = nv;
        float v = unit_max_speed(g_fo[i].u);
        if (v < slow) slow = v;
    }
    if (slow <= 0) return;
    if (arc >= 0) {   // the same formation on: one block, its leader
        for (int i = 0; i < n; i++) g_ff_idx[i] = i;
        ff_start_block(group, g_ff_idx, n, c, fwd, right, queued, slow, arc);
        return;
    }
    for (int i = 0; i < n; i++) g_ff_root[i] = i;
    for (int i = 1; i < n; i++)
        for (int j = 0; j < i; j++) {
            float lk = FF_LINK + (g_fo[i].w + g_fo[j].w) * 0.5f;
            if (vec2_dist2(g_fo[i].from, g_fo[j].from) < lk * lk) {
                int a = ff_root(i), b = ff_root(j);
                if (a != b) g_ff_root[a > b ? a : b] = a < b ? a : b;
            }
        }
    for (int r = 0; r < n; r++) {
        if (ff_root(r) != r) continue;
        int k = 0;
        for (int i = r; i < n; i++) if (ff_root(i) == r) g_ff_idx[k++] = i;
        if (k >= 2) { ff_start_block(group, g_ff_idx, k, c, fwd, right, queued, slow, -1); continue; }
        struct ff_unit *e = queued ? NULL : ff_find((uint32_t)unit_get_serial(g_fo[r].u), 0);   // on its own: out of any march
        if (e) { e->g = -1; e->pend = 0; }
    }
    if (g_ff_count > 1) g_ff_split++;
}

// a queued move turning less than 45 degrees from the last march its units are on or queued for (all of them, all on
// it): the same formation goes on - each unit's spot its offset from the leader in that march, turned to the new facing
// around the leader's spot (as far from the point c as before) - instead of a new layout; the march's index, else -1
internal vec2 g_fo_arc_off[FO_MAX];
internal int ff_arc_layout(int n, vec2 c, vec2 fwd, vec2 right) {
    struct ff_unit *e0 = ff_find((uint32_t)unit_get_serial(g_fo[0].u), 0);
    struct ff_pend *p0 = e0 ? ff_pend_last(e0) : NULL;
    int og = p0 ? p0->g : e0 ? e0->g : -1;
    if (og < 0 || !g_ff[og].on) return -1;
    struct ff_group *old = &g_ff[og];
    vec2 mean = v2(0, 0);
    for (int i = 0; i < n; i++) {
        struct ff_unit *e = ff_find((uint32_t)unit_get_serial(g_fo[i].u), 0);
        struct ff_pend *p = e ? ff_pend_last(e) : NULL;
        if (!e || (p ? p->g : e->g) != og) return -1;
        g_fo_arc_off[i] = p ? p->off : e->off;
        mean = vec2_add(mean, g_fo[i].from);
    }
    vec2 leg = vec2_sub(c, vec2_scale(mean, 1.0f / (float)n));
    float ll = vec2_len(leg);
    if (ll < 0.5f || vec2_dot(old->way, vec2_scale(leg, 1.0f / ll)) <= FF_ARC) return -1;
    vec2 lead = vec2_add(c, vec2_add(vec2_scale(right, old->lead_off.x), vec2_scale(fwd, old->lead_off.y)));
    for (int i = 0; i < n; i++) g_fo[i].at = vec2_add(lead, vec2_add(vec2_scale(right, g_fo_arc_off[i].x), vec2_scale(fwd, g_fo_arc_off[i].y)));
    return og;
}

// u's current order ended where it stands (its waypoints down to one, that one under it: the game's arrival takes a
// leg's end there and starts the next order) - a follower that swung through the corner with its formation never comes
// to its spot of the leg before
internal void ff_finish_leg(ee_unit *u) {
    while (unit_get_waypoints_left(u) > 1 && !unit_pop_waypoint(u)) {}
    vec3 p = unit_get_pos(u);
    if (unit_get_waypoints_left(u) == 1 && !unit_set_waypoint_at(u, 0, p.x, p.y)) g_ff_finished++;
}

// is the leader on the march's move now (1), still before it in its orders (0, queued), or neither (-1: given another
// order, or the move done)
internal int ff_on_move(struct ff_group *gr, ee_unit *l) {
    if (!gr->goal) return unit_get_group(l) == gr->grp && unit_get_goal(l) && goal_is_location(unit_get_goal(l)) ? 1 : -1;
    int n = unit_orders(l);
    for (int i = 0; i < n; i++) if (unit_order_at(l, i) == gr->goal) return i == 0;
    return -1;
}

// where the anchor heads: the leader's route FF_LOOK ahead of it (its waypoints, round what blocks; only those ahead),
// near the leader's spot on along the next leg when a march goes on from this one (the turn begins before the corner),
// past it along the next leg
internal vec2 ff_pursue(struct ff_group *gr, ee_unit *l) {
    vec2 a = gr->anchor;
    if (gr->passed) return vec2_add(a, vec2_scale(gr->onward, FF_LOOK));
    float de = vec2_len(vec2_sub(gr->end, a));
    if (de < FF_LOOK) return gr->through ? vec2_add(gr->end, vec2_scale(gr->onward, FF_LOOK - de)) : gr->end;
    int n = unit_get_waypoints_left(l);
    for (int i = 0; i < n && i < 32; i++) {
        float w[3];
        if (unit_waypoint_at(l, i, w)) break;
        vec2 q = v2(w[0], w[1]), d = vec2_sub(q, a);
        if (vec2_dot(d, gr->way) > 0 && vec2_dot(d, d) >= FF_LOOK * FF_LOOK) return q;
    }
    return gr->end;
}

// the turn from unit vector a to b as one (cos, sin), and s of it (0..1: the chord's share, near enough the angle's for
// the small turns of a step) - a vector turned by it, ff_rot
internal vec2 ff_rot_between(vec2 a, vec2 b) { return v2(vec2_dot(a, b), a.x * b.y - a.y * b.x); }
internal vec2 ff_rot_part(vec2 r, float s) {
    vec2 m = v2(1 - s + s * r.x, s * r.y);
    float ml = vec2_len(m);
    return ml > 0.0001f ? vec2_scale(m, 1.0f / ml) : v2(1, 0);
}
internal vec2 ff_rot(vec2 v, vec2 r) { return v2(v.x * r.x - v.y * r.y, v.x * r.y + v.y * r.x); }

// the anchor now, its heading, facing and speed (*way, *face, *vel): the formation is decided at a step of the physics
// (15 at a time, about 2.5 s of game time; a unit ticks every few hundred ms) and goes there over the next one - its
// heading and facing turned from where they were to the step's, its speed from what it was to the step's, the anchor
// carried on along them; after that span (up to 5 s) on at the step's. Each step takes the anchor from here, so it never
// jumps. Moved at the step itself, the points leapt ahead and turned in a jump, and the formation's speed changed at
// once: a ship sped up within a tick and threw out a bow wave every 2.5 s (the player: far too dense; the steps clearly
// visible, the rotation not interpolated)
internal vec2 ff_anchor_now(const struct ff_group *gr, vec2 *way, vec2 *face, float *vel) {
    uint32_t ms = game_time() - gr->at_ms;
    float t = ms > 5000 ? 5.0f : (float)ms / 1000.0f;
    if (gr->dt <= 0) { *way = gr->way; *face = gr->face; *vel = gr->vel; return vec2_add(gr->anchor, vec2_scale(gr->way, gr->vel * t)); }
    float s = t < gr->dt ? t / gr->dt : 1, t1 = t < gr->dt ? t : gr->dt;
    *way = ff_rot(gr->way_from, ff_rot_part(gr->way_rot, s));
    *face = ff_rot(gr->face_from, ff_rot_part(gr->face_rot, s));
    *vel = gr->vel_from + (gr->vel - gr->vel_from) * s;
    vec2 mid = ff_rot(gr->way_from, ff_rot_part(gr->way_rot, s * 0.5f)), a = vec2_add(gr->anchor, vec2_scale(mid, (gr->vel_from + *vel) * 0.5f * t1));
    return t > gr->dt ? vec2_add(a, vec2_scale(gr->way, gr->vel * (t - gr->dt))) : a;
}

// NARROWING (the player: a ray from the formation's anchor for the room there is, and where it sees room for one or two
// abreast a fitting formation for the while): each step the room across the way is measured at every tile from the
// formation's rear to FF_NARROW_LOOK beyond its front - the ground across its width the leader may enter - and where
// the narrowest is less than FF_NARROW_AT of the formation's width it fills its rows anew, front to back and left to right in its order, as many abreast as
// fit at its members' mean spacing; it widens again once that room is there along all of it. The jam it had made
// at a gap crawled through: its units couldn't all fit, the formation waited for them
#define FF_NARROW_LOOK 6.0f    // tiles beyond its front
#define FF_NARROW_AT 0.4f
#define FF_WIDEN_AT 0.7f
internal unsigned g_ff_narrowed;   // steps a formation went narrowed
internal vec2 ff_off(const struct ff_group *gr, const struct ff_unit *e) {
    if (!gr->cols || gr->n <= 0) return e->off;
    int row = e->order / gr->cols, col = e->order % gr->cols, in_row = gr->n - row * gr->cols;
    if (in_row > gr->cols) in_row = gr->cols;
    return v2(((float)col - (float)(in_row - 1) * 0.5f) * gr->sp, gr->front - (float)row * gr->sp);
}
// the room across the way at p, lim to each side: the ground there it may enter (0: p itself blocked) - not to the
// nearest thing that blocks: a tree or a house beside the way narrowed a formation in the open, and it came apart
internal float ff_room(ee_map *map, ee_unit *u, ee_unit_type *td, int size, vec2 p, vec2 across, float lim) {
    if (!sr_pass(map, u, td, size, (int)floorf(p.x), (int)floorf(p.y))) return 0;
    float room = 0;
    for (float d = -lim; d <= lim; d += 0.5f) {
        vec2 q = vec2_add(p, vec2_scale(across, d));
        if (sr_pass(map, u, td, size, (int)floorf(q.x), (int)floorf(q.y))) room += 0.5f;
    }
    return room;
}
internal void ff_narrow(struct ff_group *gr, ee_unit *l) {
    ee_map *map = game_map();
    ee_unit_type *td = l ? unit_get_type_data(l) : NULL;
    int size = map ? (int)map_get_size(map) : 0;
    if (gr->n < 2 || gr->sp <= 0 || !td || size <= 0 || !sr_alloc((uint32_t)(size * size))) return;
    st_stamp_step();
    int full = (int)(gr->fw / gr->sp);   // abreast as laid out (about)
    float back = gr->rear;   // its rear: the laid out one's, or the narrowed one's
    if (gr->cols) {
        float rear = gr->front - (float)((gr->n - 1) / gr->cols) * gr->sp;
        if (rear < back) back = rear;
    }
    vec2 across = v2(gr->way.y, -gr->way.x);
    float room = 1e9f, lim = gr->fw * 0.5f + 1;
    for (float t = back; t <= gr->front + FF_NARROW_LOOK; t += 1.0f) {
        float r = ff_room(map, l, td, size, vec2_add(gr->anchor, vec2_scale(gr->way, t)), across, lim);
        if (r > 0 && r < room) room = r;   // 0: the line itself on something (a bend) - not a measure of the gap
    }
    // narrowed below FF_NARROW_AT of its width, narrower still where the room shrinks, as laid out again once FF_WIDEN_AT
    // of it is there along all of it (the whole width: a few trees about kept a formation narrowed for good) - each change
    // fills the rows anew (it went 4, 5, 4 abreast from step to step: every unit to a new place)
    int narrow = room < gr->fw * (gr->cols ? FF_WIDEN_AT : FF_NARROW_AT), cols = narrow ? (int)(room / gr->sp) : 0;
    if (narrow && cols < 1) cols = 1;
    if (cols >= full) cols = 0;
    if (cols && gr->cols && cols > gr->cols) cols = gr->cols;
    if (cols != gr->cols && g_log)   // for the player's games: when and where a formation narrowed or spread out again
        fprintf(g_log, "# formation: %d units at %.1f,%.1f %s %d abreast (room %.1f of its %.1f tiles)\n", gr->n, gr->anchor.x, gr->anchor.y,
                cols ? "narrowed to" : "as laid out again, about", cols ? cols : (int)(gr->fw / gr->sp), room, gr->fw);
    gr->cols = cols;
    if (cols) g_ff_narrowed++;
}

// once a step: is the march on its way (its leader there, on this move, not done), the anchor moved on; NULL while it
// is queued too (gr->on stays). The anchor - the point the formation hangs on, every unit's point (the leader's too) its
// offset from it turned with the facing - heads for ff_pursue's point like a ship: its heading turns no faster than an
// arc of radius reach + FF_INNER (the innermost unit still walks forward through the turn instead of the formation
// wheeling round a leader on the route), and it slows in the turn so the outermost unit keeps up within formation_catchup
internal ee_unit *ff_leader(struct ff_group *gr, uint32_t step) {
    ee_unit *l = gr->on ? unit_by_id(gr->leader) : NULL;
    if (gr->step == step) return gr->live ? l : NULL;
    uint32_t steps = gr->step == ~0u || step - gr->step > 20 ? 1 : step - gr->step;   // since the last (no unit of it may have moved in between)
    gr->step = step;
    gr->live = 0;
    vec3 p = l ? unit_get_pos(l) : v3(0, 0, 0);
    int on = l && unit_get_hp(l) > 0 ? ff_on_move(gr, l) : -1;
    if (on < 0) {   // the leader's move done (it idles - or died: the others still arrive), else it was given another order
        gr->on = 0;
        if (gr->started && !gr->linger && (!l || unit_get_hp(l) <= 0 || !unit_orders(l))) gr->linger = step ? step : 1;
        return NULL;
    }
    if (!on) return NULL;
    gr->live = 1;
    // the slowest member's speed now last step (fo_follow_aim; -1: none told)
    gr->pace = gr->pace_next;
    gr->pace_next = -1;
    ee_physics *ph = game_physics();
    float dt = (ph ? physics_get_step_s(ph) : 0.05f) * (float)steps;
    float vn = 0;   // its speed now
    vec2 w, f;
    if (!gr->started) {   // the anchor sets out from the leader - or where the march before left it, turning on from there
        struct ff_group *pv = gr->prev >= 0 ? &g_ff[gr->prev] : NULL;
        gr->started = 1;
        gr->t0 = step;
        if (pv && pv->started && pv->through) {
            gr->anchor = ff_anchor_now(pv, &w, &f, &vn);
            gr->way = w;
            if (gr->arc) gr->face = f; else gr->pivot = 1;
        }
        else gr->anchor = v2(p.x, p.y);
    } else {   // where the last step sent it
        gr->anchor = ff_anchor_now(gr, &w, &f, &vn);
        gr->way = w;
        gr->face = f;
    }
    vec2 to = vec2_sub(ff_pursue(gr, l), gr->anchor);
    float tl = vec2_len(to);
    vec2 dir = tl > 0.01f ? vec2_scale(to, 1.0f / tl) : gr->way;
    // its speed: the march's, no more than FF_PACE of its slowest member's speed now (a unit fallen behind has only its
    // top speed: the formation slows for it), less in a turn
    float base = gr->pace >= 0 && FF_PACE * gr->pace < gr->speed ? FF_PACE * gr->pace : gr->speed;
    // re-forming on the move (pivot) it turns onto the new leg at FF_INNER, not at the formation's reach: the units take new
    // places anyway, and the wide swing at a sharp corner put their points on ground they couldn't reach
    // ships turn together: the anchor turns as one ship does (its least agile member's circle at its top speed) and the
    // formation's facing with it, every ship on its own circle to its new place, the pace slowing the fleet while they get
    // there - the whole fleet wheeling round its innermost ship's circle swung far out (the player: too large turns are
    // impractical in the game)
    float r = gr->naval ? (gr->turn_r > FF_INNER ? gr->turn_r : FF_INNER) : gr->pivot ? FF_INNER : gr->reach + FF_INNER;
    float k = gr->pivot || gr->naval ? 1 : g_set.formation_catchup * 0.85f * r / (r + gr->reach);
    if (k > 1) k = 1;
    float v = base * (1 - gr->turning * (1 - k));
    // a fleet's speed changes no faster than its slowest-accelerating ship can: the pace rocked it - the ships fell behind,
    // the fleet slowed, they closed in, it sped up, they fell behind again (a ship's speed a sawtooth, a bow wave at every
    // speed-up)
    float dv = gr->acc * dt;
    if (gr->naval && gr->acc < 1e8f) v = v > vn + dv ? vn + dv : v < vn - dv ? vn - dv : v;
    // its heading towards dir, no faster than the arc (the chord for the angle: small steps)
    float most = v * dt / r, ch = vec2_len(vec2_sub(dir, gr->way));
    gr->turning = ch > most ? 1 : most > 0 ? ch / most : 0;
    vec2 way0 = gr->way, face0 = gr->face;
    gr->way = ch <= most ? dir : fo_turn(gr->way, dir, most / ch);
    if (ch <= most) gr->pivot = 0;   // on the new leg: its turns at its reach again
    gr->at_ms = game_time();
    gr->vel_from = vn;
    gr->vel = v;
    // the anchor at the leader's spot (where this step takes it): the march done, each unit on to its own; going on to a
    // queued march, the anchor goes on and the leader's leg ends there (its arrival starts the next order, which takes the
    // anchor over)
    float de = vec2_len(vec2_sub(gr->end, vec2_add(gr->anchor, vec2_scale(fo_half(way0, gr->way), (vn + v) * 0.5f * dt))));
    if (de < FF_END && !gr->through) { gr->on = gr->live = 0; gr->linger = step ? step : 1; return NULL; }
    if (de < FF_END && gr->through && !gr->passed) { gr->passed = 1; ff_finish_leg(l); }
    // the facing: the heading, turned to the final facing over the last span - its share of the turn the time left on it
    // (the turn's arc for the outermost unit: at most the chord x pi / 2) - no faster than its outermost units swing round
    float spd = gr->speed > 0.05f ? gr->speed : 0.05f, left = de / spd;
    float span = gr->reach * vec2_len(vec2_sub(gr->fwd, gr->way)) * 1.5708f / spd;
    span = span < FF_SETTLE ? FF_SETTLE : span > FF_SETTLE_MAX ? FF_SETTLE_MAX : span;
    vec2 want = left >= span || gr->through || gr->reform ? gr->way : fo_turn(gr->way, gr->fwd, 1 - left / span);
    float swing = FF_SWING * spd / (gr->reach > 1 ? gr->reach : 1) * dt;
    ch = vec2_len(vec2_sub(want, gr->face));
    gr->face = ch <= swing || gr->naval ? want : fo_turn(gr->face, want, swing / ch);
    ff_narrow(gr, l);
    gr->way_from = way0;
    gr->face_from = face0;
    gr->way_rot = ff_rot_between(way0, gr->way);
    gr->face_rot = ff_rot_between(face0, gr->face);
    gr->dt = dt;
    return l;
}

internal uint32_t ff_step(void) { ee_physics *ph = game_physics(); return ph ? physics_get_steps(ph) : 0; }

// seconds since step (the formation's steps)
internal float ff_since(uint32_t step) {
    ee_physics *ph = game_physics();
    return (float)(ff_step() - step) * (ph ? physics_get_step_s(ph) : 0.05f);
}

// collide.c: the share of their radii at which a and b collide - formation_collision for two units of one formation,
// bound to the formation: faded in over FF_EASE from its march's start, all of it while it marches and FF_LINGER after
// it arrived, then faded out over FF_EASE (a switch on arrival shoved the units that had slipped into each other apart
// at once, off their spots); 1 for any other pair
internal float ff_collision(ee_unit *a, ee_unit *b) {
    if (!g_ff_on || g_set.formation_collision >= 1) return 1;
    struct ff_unit *ea = ff_find((uint32_t)unit_get_serial(a), 0), *eb = ea && ea->g >= 0 ? ff_find((uint32_t)unit_get_serial(b), 0) : NULL;
    if (!eb || eb->g != ea->g) return 1;
    struct ff_group *gr = &g_ff[ea->g];
    float t;
    if (gr->on && gr->live && gr->started) t = ff_since(gr->t0) / FF_EASE;
    else if (!gr->on && gr->linger) t = (FF_LINGER + FF_EASE - ff_since(gr->linger)) / FF_EASE;
    else return 1;
    t = t < 0 ? 0 : t > 1 ? 1 : t;
    return 1 - (1 - g_set.formation_collision) * t;
}

// u's spot at its formation move's goal (test/fleetmove.c), 1 - else 0
internal int ff_spot(ee_unit *u, vec2 *spot) {
    struct ff_unit *e = g_ff_on ? ff_find((uint32_t)unit_get_serial(u), 0) : NULL;
    struct ff_pend *p = e ? ff_pend_of(e, e->pend) : NULL;
    if (!e || e->g < 0) return 0;
    *spot = p ? p->home : e->home;
    return 1;
}

// u's march as `fmove watch` logs it: -1 none, 0 its march done, 1 on but not on its way (queued), 2 on its way and u
// steered at its point the last step, 3 .. 5 on its way and u on its own route (3 the point steered at on a tile it may
// not enter, 4 not in a clear line, 5 its point too far, 6 dropped out); + 10 x its queued marches
internal int ff_watch_state(ee_unit *u) {
    struct ff_unit *e = g_ff_on ? ff_find((uint32_t)unit_get_serial(u), 0) : NULL;
    if (!e) return -1;
    int q = 0;
    for (struct ff_pend *p = ff_pend_of(e, e->pend); p && q < 9; p = ff_pend_of(e, p->after)) q++;
    struct ff_group *gr = e->g >= 0 ? &g_ff[e->g] : NULL;
    int s = !gr ? -1 : !gr->on ? 0 : !gr->live ? 1 : e->out ? 6 : e->step + 1 >= ff_step() ? 2 : e->own == 1 ? 5 : e->own == 2 ? 3 : 4;
    return s + 10 * q;
}

// u's distance from its point in its marching formation this step (test/fleetmove.c), else -1
internal float ff_point_off(ee_unit *u) {
    struct ff_unit *e = g_ff_on ? ff_find((uint32_t)unit_get_serial(u), 0) : NULL;
    return e && e->g >= 0 && g_ff[e->g].on && g_ff[e->g].live && g_ff[e->g].step == ff_step() ? e->off_by : -1;
}

// the safe point between simulation steps, every FF_HOME_EVERY: an idle unit of a formation that arrived less than
// FF_LINGER ago, pushed off its spot (further than FF_HOME_SLOP x its size, no further than FF_HOME_FAR), walks back to
// it - a move order of its own, turning to its formation's facing there: it turns, walks and turns again (a push slid it
// back sideways). The game's own goal, as the AI gives one: the same on every PC
internal unsigned g_ff_walks;   // walks back given
internal void ff_linger_tick(void) {
    uint32_t step = ff_step();
    if (!g_ff_on || step % FF_HOME_EVERY) return;
    int any = 0;
    for (int k = 0; k < FF_GROUPS && !any; k++) any = !g_ff[k].on && g_ff[k].linger && ff_since(g_ff[k].linger) < FF_LINGER;
    if (!any) return;
    for (int k = 0; k < FF_UNITS; k++) {
        struct ff_unit *e = &g_ff_u[k];
        if (!e->id || e->g < 0 || e->pend || e->tries >= FF_HOME_TRIES) continue;
        struct ff_group *gr = &g_ff[e->g];
        if (gr->on || !gr->linger || ff_since(gr->linger) >= FF_LINGER) continue;
        ee_unit *u = unit_by_id(e->id);
        ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
        if (!td || unit_get_hp(u) <= 0 || unit_get_holder(u) || unit_get_frozen(u) || unit_orders(u) || unit_get_speed(u) > 0.05f) continue;
        vec3 p = unit_get_pos(u);
        float d = vec2_len(vec2_sub(e->home, v2(p.x, p.y))), slop = unit_get_size(u) * FF_HOME_SLOP;
        if (d <= (slop > FF_HOME_MIN ? slop : FF_HOME_MIN) || d > FF_HOME_FAR) continue;
        // straight there only: a spot behind a wall would send it round it
        ee_map *map = game_map();
        int size = map ? (int)map_get_size(map) : 0;
        if (size <= 0 || !sr_alloc((uint32_t)(size * size))) return;
        st_stamp_step();
        if (!sr_pass(map, u, td, size, (int)floorf(e->home.x), (int)floorf(e->home.y)) || !sr_clear(map, u, td, size, p.x, p.y, e->home.x, e->home.y)) { e->tries = FF_HOME_TRIES; continue; }
        void *mem = goal_move_alloc();
        if (!mem) return;
        float xy[2] = { e->home.x, e->home.y }, yaw;
        ee_goal *g = goal_move_init(mem, xy);
        memcpy(&yaw, &e->yaw, sizeof yaw);
        if (e->yaw == 0) yaw = fo_atan2(-gr->fwd.x, gr->fwd.y);   // a click: the formation's facing
        location_goal_set_yaw((ee_location_goal *)g, yaw);
        location_goal_set_face((ee_location_goal *)g, 1);
        unit_assign_goal(u, g, 0, 0);
        unit_set_state(u, 0x1F40);
        unit_type_goal_assigned(td, u);
        e->tries++;
        g_ff_walks++;
    }
}

// u's march on its way this step and its leader (*l), else NULL: a queued move's march taken up once it sets out;
// u in its leader's group (both moved on together: a queued move's dispatch puts them in a group of its own), or on its
// next queued move already (its leg done before the leader's: it holds its point till the march it joins sets out)
internal struct ff_group *ff_march(struct ff_unit *e, ee_unit *u, uint32_t step, ee_unit **l) {
    struct ff_pend *p = e->pend ? ff_pend_of(e, e->pend) : NULL;
    if (e->pend && !p) e->pend = 0;   // overwritten: its own route
    if (p) {
        struct ff_group *n = &g_ff[p->g];
        if (ff_leader(n, step)) {   // its leg before ended where it stands, if it hasn't reached it
            if (p->goal && unit_orders(u) >= 2 && unit_order_at(u, 0) != p->goal && unit_order_at(u, 1) == p->goal) ff_finish_leg(u);
            e->g = p->g; e->off = p->off; e->order = p->order; e->home = p->home; e->yaw = p->yaw; e->tries = 0; e->step = ~0u; e->pend = p->after; e->out = 0; e->lag_at = 0; ff_grace_from(e);
        }
        else if (!n->on) e->pend = p->after;   // given up: on to the one behind it
    }
    if (e->g < 0) { *l = NULL; return NULL; }
    struct ff_group *gr = &g_ff[e->g];
    *l = gr->on ? ff_leader(gr, step) : NULL;
    struct ff_pend *q = ff_pend_of(e, e->pend);
    return *l && (e->id == gr->leader || unit_get_group(u) == unit_get_group(*l) || (q && q->goal && unit_get_goal(u) == q->goal)) ? gr : NULL;
}

// u's catch-up share (of formation_catchup over its top speed): only near its place - all of it within its size, none
// from FF_FALLEN x its size on (a unit fallen behind doesn't run for long stretches at it: it walks, and the formation
// slows for it) - or for FF_GRACE seconds after it was last in its place (a re-form, the march's start: none is in its
// place yet), falling off, no more than the share it had when the march began (ff_grace_from); the larger counts.
// e->off_by of this step
internal float ff_share(struct ff_unit *e, ee_unit *u, uint32_t step) {
    float sz = unit_get_size(u), place = sz < FO_MIN_SIZE ? FO_MIN_SIZE : sz;
    float share = e->off_by <= place ? 1 : e->off_by >= FF_FALLEN * place ? 0 : (FF_FALLEN * place - e->off_by) / ((FF_FALLEN - 1) * place);
    ee_physics *ph = game_physics();
    float sstep = ph ? physics_get_step_s(ph) : 0.05f;
    if (!e->in_at || e->off_by <= place) e->in_at = step ? step : 1;
    if (e->off_by <= place) e->grace_cap = 1;   // in its place: the grace's whole again
    float since = (float)(step - e->in_at) * sstep, grace = since >= FF_GRACE ? 0 : 1 - since / FF_GRACE;
    if (grace > e->grace_cap) grace = e->grace_cap;
    e->share = grace > share ? grace : share;
    e->share_at = step ? step : 1;
    return e->share;
}

// u's route planned anew from where it stands to the end of its waypoints (the mod's route search, with ship_routes /
// land_routes; else, or none found, the one it has) - the one planned at the order lies where it stood then: a unit the
// formation had carried off it turned round and walked back along it, the long way (the player)
internal unsigned g_ff_rerouted;
internal void ff_reroute(ee_unit *u) {
    int n = unit_get_waypoints_left(u);
    static float keep[64][3];
    if (!sr_wants(u) || n <= 0 || n > 64) return;
    for (int i = 0; i < n; i++) if (unit_waypoint_at(u, i, keep[i])) return;
    while (unit_get_waypoints_left(u) > 0 && !unit_pop_waypoint(u)) {}
    int k = n - unit_get_waypoints_left(u);   // popped
    vec3 p = unit_get_pos(u);
    int from[2] = { (int)floorf(p.x), (int)floorf(p.y) }, to[2] = { (int)floorf(keep[n - 1][0]), (int)floorf(keep[n - 1][1]) };
    if (unit_get_waypoints_left(u) == 0 && sr_route(u, from, to, NULL, 0)) { g_ff_rerouted++; return; }
    for (int i = n - k; i < n; i++) unit_add_waypoint(u, keep[i], 1);   // none: back as they were
}

// u drops out of its march: on its own route from here, planned anew
#define FF_OUT_S 10.0f         // s
#define FF_OUT_MOVE 1.0f       // tiles: moving less than this in FF_OUT_S a unit held up is stuck
#define FF_REROUTE_S 3.0f      // s: a unit's own route planned anew at most this often
#define FF_JAM 0.5f            // x its top speed: a unit fallen behind counts for the pace at its speed, at least this
internal void ff_drop_out(struct ff_unit *e, ee_unit *u) {
    e->out = 1;
    g_ff_out++;
    ff_reroute(u);
}

// the land turn (shiproute.c's hook): u's point to steer at this tick, 1 - else 0, its own route
internal int fo_follow_aim(ee_unit *u, const float *pos, float *aim) {
    if (!g_ff_on || !u) return 0;
    struct ff_unit *e = ff_find((uint32_t)unit_get_serial(u), 0);
    if (!e) return 0;
    uint32_t step = ff_step();
    ee_unit *l;
    struct ff_group *gr = ff_march(e, u, step, &l);
    if (!gr || e->out) return 0;
    vec2 way, face;
    float vel;
    vec2 anchor = ff_anchor_now(gr, &way, &face, &vel), right = v2(face.y, -face.x);
    vec2 off = ff_off(gr, e), pt = vec2_add(anchor, vec2_add(vec2_scale(right, off.x), vec2_scale(face, off.y)));
    ee_map *map = game_map();
    ee_unit_type *td = unit_get_type_data(u);
    int size = map ? (int)map_get_size(map) : 0;
    // its point on ground it may not enter (a formation wider than the channel it passes): drawn in across towards the
    // formation's line, its rank kept - the formation narrows there (the outer ships went their own way, away from it)
    if (td && size > 0 && sr_alloc((uint32_t)(size * size))) {
        st_stamp_step();
        for (int k = 3; k >= 0 && !sr_pass(map, u, td, size, (int)floorf(pt.x), (int)floorf(pt.y)); k--)
            pt = vec2_add(anchor, vec2_add(vec2_scale(right, off.x * (float)k * 0.25f), vec2_scale(face, off.y)));
    }
    vec2 d = vec2_sub(pt, v2(pos[0], pos[1]));
    float w = unit_max_turn(u), ahead = gr->naval && w > 0 ? 2 * unit_max_speed(u) / w : 0;   // a ship: two turning radii ahead (it closes on its place in a curve)
    if (ahead < FF_AHEAD) ahead = FF_AHEAD;
    // the point steered at: on ground it may enter, in a clear line - a ship's, two turning radii ahead, nearer its point
    // till it is (in a channel it lay on the shore: 2598 ticks of 16 ships on their own routes, two ships the whole
    // second leg - the player: some ships decide they aren't part of the group)
    // A ship ahead of its point steers on from abreast of itself (it slows for its point instead of turning back to it:
    // one with its point behind it went round in a full circle), one beside it at least twice that far ahead (it merges
    // at a shallow angle: it turned 90 degrees to its point and back - the player: some turn the wrong way at the start,
    // some do a full 360 with the target in front of them, or 90 and back)
    vec2 base = pt, rel = vec2_sub(v2(pos[0], pos[1]), pt);
    if (gr->naval) {
        float lon = vec2_dot(rel, way), lat = fabsf(vec2_dot(rel, v2(way.y, -way.x)));
        if (lon > 0) base = vec2_add(pt, vec2_scale(way, lon));
        if (ahead < 2 * lat) ahead = 2 * lat < FF_FAR ? 2 * lat : FF_FAR;
    }
    int ok = 0, why = 1;
    vec2 at = vec2_add(base, vec2_scale(way, ahead));
    if (vec2_len(d) <= FF_FAR && td && size > 0 && sr_alloc((uint32_t)(size * size))) {
        st_stamp_step();
        for (float ah = ahead; !ok; ah *= 0.5f) {
            if (ah < FF_AHEAD) ah = FF_AHEAD;
            vec2 c = vec2_add(base, vec2_scale(way, ah));
            why = !sr_pass(map, u, td, size, (int)floorf(c.x), (int)floorf(c.y)) ? 2 : !sr_clear(map, u, td, size, pos[0], pos[1], c.x, c.y) ? 3 : 0;
            if (!why || ah == ahead) at = c;
            ok = !why;
            if (ah <= FF_AHEAD) break;
        }
        // neither: the formation's line ahead of the anchor - it runs along the leader's route, through the gap the
        // formation is too wide for; its own route was planned from where it stood at the order, and the formation had
        // carried it off that since (a land unit at a choke point turned round and went the long way)
        vec2 c = vec2_add(anchor, vec2_scale(way, FF_AHEAD));
        if (!ok && sr_pass(map, u, td, size, (int)floorf(c.x), (int)floorf(c.y)) && sr_clear(map, u, td, size, pos[0], pos[1], c.x, c.y)) {
            at = c;
            ok = 1;
            g_ff_centre++;
        }
    }
    // behind and ahead by the way it steers (at its point): a follower with its point on the far side, against the way
    // (two blocks sent to the point between them: the leader's block walks to the other), walks back to it - by the way
    // it stood waiting for a point that came at the leader's crawl, and the leader for the formation
    vec2 to = vec2_sub(at, v2(pos[0], pos[1]));
    float tl = vec2_len(to);
    vec2 a = tl > 0.01f ? vec2_scale(to, 1.0f / tl) : way;
    e->along = vec2_dot(d, a);
    e->off_by = vec2_len(d);
    // its point's speed its way: the anchor's, and in a turn its offset swung round with the facing (the outer ones of a
    // wheeling fleet go faster, the inner slower - else the outer ones fell behind in every arc)
    float spin = gr->dt > 0 ? gr->face_rot.y / gr->dt : 0;
    vec2 r = vec2_sub(pt, anchor), vp = vec2_add(vec2_scale(way, vel), vec2_scale(v2(-r.y, r.x), spin));
    e->with = vec2_dot(vp, a);
    // fallen behind (no catch-up share left, its point ahead) and asking for its top speed (ff_speed) it counts for the
    // pace with the speed it actually goes - in a jam at a gap the formation waits at the jam's speed (the player: its limited throughput, and then they don't run
    // spread thin into enemy territory behind the choke, is the point); at its top speed it held the formation at 80% of
    // that while the jam fell further behind. Held up - so, or its point out of reach - and moving less than FF_OUT_MOVE
    // in FF_OUT_S, it is stuck: it drops out of the march (the formation no longer waits for it) and takes a route of its
    // own from where it stands (the player: a unit can't hold the formation back - it drops out and follows the path
    // alone, from its position to its next waypoint)
    float sz = unit_get_size(u), place = sz < FO_MIN_SIZE ? FO_MIN_SIZE : sz;
    int behind = e->along > 0 && e->off_by > FF_FALLEN * place;
    vec2 here = v2(pos[0], pos[1]);
    if (!ok || behind) {
        if (!e->lag_at) { e->lag_at = step ? step : 1; e->lag_pos = here; }
        else if (ff_since(e->lag_at) >= FF_OUT_S) {
            if (vec2_dist2(here, e->lag_pos) < FF_OUT_MOVE * FF_OUT_MOVE) { ff_drop_out(e, u); return 0; }
            e->lag_at = step ? step : 1;
            e->lag_pos = here;
        }
    } else e->lag_at = 0;
    float now = unit_max_speed(u) * (1 + (g_set.formation_catchup - 1) * ff_share(e, u, step));   // its speed now: the formation's pace
    // (one closing in at less goes the formation's speed and a bit: counted, it slowed the formation, itself with it, and
    // so on down - to FF_JAM in open ground)
    if (behind && e->want >= unit_max_speed(u) * 0.99f) {   // no less than FF_JAM of its top speed: one slowed in a turn
                    // or by a push stopped the formation, and the others' speed, the formation's, with it
        float v = unit_get_speed(u), lo = FF_JAM * unit_max_speed(u);
        if (v < lo) v = lo;
        if (v < now) now = v;
    }
    if (gr->pace_next < 0 || now < gr->pace_next) gr->pace_next = now;
    if (!ok) {   // its own route for now: from where it stands, when it steered at its point till now (at most every FF_REROUTE_S)
        if (e->step != ~0u && step - e->step <= 3 && (!e->rerouted_at || ff_since(e->rerouted_at) >= FF_REROUTE_S)) {
            // the march goes on through its leg's end and that lies behind: the leg ends here (its next order takes it
            // on from where it stands) - a citizen carried round the corner walked back to its first leg's spot
            float end[3];
            int n = unit_get_waypoints_left(u);
            if (gr->through && unit_orders(u) >= 2 && n > 0 && !unit_waypoint_at(u, n - 1, end) && vec2_dot(vec2_sub(v2(end[0], end[1]), here), way) < 0)
                ff_finish_leg(u);
            else ff_reroute(u);
            e->rerouted_at = step ? step : 1;
        }
        g_ff_own[e->own = why]++;
        return 0;
    }
    st_drop_passed(u, map, td, size, pos[0], pos[1], FF_DROP);
    aim[0] = at.x; aim[1] = at.y; aim[2] = pos[2];
    e->step = step;
    g_ff_steered++;
    return 1;
}

// the land models' speed step: the leader at the march's speed (the slowest member's top speed), less while a member
// lags (FF_PACE); a follower that steered this step at its point's speed (the leader's) its way plus
// FF_GAIN for each tile behind it - while behind up to formation_catchup of its own top speed (the slowest ones could never catch up at their top speed,
// and the formation as a whole still goes no faster than its slowest unit) - towards it by its own acceleration a tick;
// the game's own step while it turns more than 45 degrees
// (ships: shiproute.c's speed step calls it with the game's step s)
internal float ff_speed(ee_unit *u, float s, float angle) {
    struct ff_unit *e = g_ff_on && u ? ff_find((uint32_t)unit_get_serial(u), 0) : NULL;
    if (!e || fabsf(angle) > 0.7853982f) return s;
    uint32_t step = ff_step();
    ee_unit *l;
    struct ff_group *gr = ff_march(e, u, step, &l);
    if (!gr) return s;
    float top = unit_max_speed(u), want, cap = top;
    if (e->step == ~0u || step - e->step > FF_KEEP) return s;   // steered at its point lately (a ship isn't every tick)
    want = e->with + FF_GAIN * e->along;   // its point's speed its way, and closing
    if (e->along > 0) cap = top * (1 + (g_set.formation_catchup - 1) * ff_share(e, u, step));   // near its place only
    if (want > cap) want = cap;
    if (want < 0) want = 0;
    e->want = want;
    ee_physics *ph = game_physics();
    float v = unit_get_speed(u), acc = ph ? physics_get_step_s(ph) * unit_accel(u) : 0.01f;
    return v < want ? (v + acc < want ? v + acc : want) : (v - acc > want ? v - acc : want);
}
// a ship's turn at its point (shiproute.c, after the game's turn towards it: *left the angle off its bow then): at a
// rate of the angle left over FF_TURN_T, within its top turn rate and changed by its turn acceleration a tick - the
// game's turn sped up, braked and landed on the point each time: chasing one that moves, it turned a step every third
// tick and stood between (the player: jerky small rotations every 0.5 s instead of turning continuously); in a steady
// arc the rate holds now
#define FF_TURN_T 1.0f   // s
internal void ff_turn(ee_unit *u, const float *left, float *rate, float *heading) {
    ee_physics *ph = game_physics();
    float dt = ph ? physics_get_step_s(ph) : 0;
    vec3 f = unit_get_facing(u);
    if (dt <= 0 || (f.x == 0 && f.y == 0)) return;
    const float PI = 3.14159265f;
    float h0 = fo_atan2(-f.x, f.y), w = unit_max_turn(u), acc = unit_turn_accel(u) * dt, r0 = unit_get_turn_rate(u);
    float want = *left / FF_TURN_T;
    want = want > w ? w : want < -w ? -w : want;
    float r = want > r0 + acc ? r0 + acc : want < r0 - acc ? r0 - acc : want;
    if ((r > 0 && *left > 0 && r * dt > *left) || (r < 0 && *left < 0 && r * dt < *left)) r = *left / dt;   // not past it
    float h = h0 + r * dt;
    *rate = r;
    *heading = h > PI ? h - 2 * PI : h < -PI ? h + 2 * PI : h;
}

internal float __thiscall hook_ff_speed(void *model, ee_unit *u, float dist, float angle) { return ff_speed(u, physics_model_speed_step(model, u, dist, angle), angle); }

internal void groupmove_new_game(void) {
    memset(g_ff, 0, sizeof g_ff);
    memset(g_ff_u, 0, sizeof g_ff_u);
    memset(g_ff_p, 0, sizeof g_ff_p);
    g_ff_p_next = 0;
    memset(g_fo_marched, 0, sizeof g_fo_marched);
    g_ff_next = g_fo_marched_n = 0;
    g_ff_count = 0;
}

internal void __thiscall hook_formation_slots(void *f, void *group, void *slots) {
    formation_slots(f, group, slots);
    g_fo_slots = NULL;
    int n = formation_slot_count(slots), kind = g_set.formation_kind >= 0 ? g_set.formation_kind : formation_get_kind(f);   // (tests: a kind's layout without its button)
    if (!g_fo_on || n < 2 || n > FO_MAX || kind < 0 || kind > FORMATION_OUTWARD_CIRCLE) return;
    vec3 c3 = formation_get_center(f);
    vec2 c = v2(c3.x, c3.y), mean = v2(0, 0), now = v2(0, 0);   // now: where they stand
    int from_last = 0;
    for (int i = 0; i < n; i++) {
        struct fo_unit *u = &g_fo[i];
        u->u = fslot_get_unit(formation_slot(slots, i));
        if (!u->u) return;
        vec3 p = unit_get_pos(u->u);
        now = vec2_add(now, v2(p.x, p.y));
        int orders = g_fo_queued ? unit_orders(u->u) : 0;
        ee_goal *last = orders > 0 ? unit_order_at(u->u, orders - 1) : NULL;
        if (last && goal_is_location(last)) { p = goal_target(last, u->u); from_last++; }   // queued: from its last order's point
        u->p = u->from = v2(p.x, p.y);
        float s = unit_get_size(u->u);
        u->w = (s < FO_MIN_SIZE ? FO_MIN_SIZE : s) * FO_PAD[kind];
        u->rank = fo_rank(u->u);
        mean = vec2_add(mean, u->p);
        g_fo_order[i] = i;
    }
    mean = vec2_scale(mean, 1.0f / (float)n);
    now = vec2_scale(now, 1.0f / (float)n);
    // the game's slots as they were: its front row on the clicked point and the rest behind; a circle's facings are
    // taken from its nearest one
    static vec2 game_at[FO_MAX];
    static uint32_t game_facing[FO_MAX];
    for (int i = 0; i < n; i++) {
        void *s = formation_slot(slots, i);
        vec3 p = fslot_get_pos(s);
        game_at[i] = v2(p.x, p.y);
        game_facing[i] = fslot_get_facing(s);
    }
    // The way to face: the slots' facing, the angle of the right-button drag (a float, atan2(-dx, dy): 0 faces +y) - not
    // the clicked point less the slots' middle, which the long line's 10 columns put half a column aside (its row was
    // turned across the move). A click gives a drag of no length, 0: then the way from the group to the point
    float facing;
    memcpy(&facing, &game_facing[0], sizeof facing);
    const float PI = 3.14159265f;
    float ang = facing + PI / 2;   // the facing's direction (-sin, cos) as an angle from +x
    while (ang < 0) ang += 2 * PI;
    while (ang >= 2 * PI) ang -= 2 * PI;
    vec2 to = vec2_sub(c, mean), fwd = facing == 0 ? v2(0, 1) : fo_dir(ang);
    // a click in the game's view comes with a facing too - the way from the selection where it stands to the point
    // (the player's games: every move a "drag"): for a queued move that is the way from the wrong place, so a facing
    // within FO_CLICK of it counts as a click, laid out from the last order's points and turned to on arrival
    vec2 cur = vec2_sub(c, now);
    float cl = vec2_len(cur);
    int reface = g_fo_queued && facing != 0 && from_last && cl > 0.5f && vec2_dot(fwd, vec2_scale(cur, 1.0f / cl)) > FO_CLICK;
    if (reface) facing = 0;
    // the game's own front row (lines): its slots ahead along the way it laid them out, before a click's direction
    int front = 0;
    float most = -1e30f;
    for (int i = 0; i < n; i++) { float d = vec2_dot(game_at[i], fwd); if (d > most) most = d; }
    for (int i = 0; i < n; i++) if (vec2_dot(game_at[i], fwd) > most - 0.3f) front++;
    float tl = vec2_len(to);
    if (facing == 0) fwd = tl > 0.5f ? vec2_scale(to, 1.0f / tl) : v2(0, 1);
    if (reface) {   // the units' facing on arrival: the drag's angle of the new way, atan2(-x, y)
        float a = fo_atan2(-fwd.x, fwd.y);
        uint32_t bits;
        memcpy(&bits, &a, sizeof bits);
        for (int i = 0; i < n; i++) game_facing[i] = bits;
        g_fo_refaced++;
    }
    vec2 right = v2(fwd.y, -fwd.x);
    int circle = kind >= FORMATION_INWARD_CIRCLE;
    // following, with the final facing 45 degrees or more off the way it goes (FF_ARC, as at a waypoint): the march
    // forms for its way first - the same rows, the units given their places by where they stand along that way (a wedge
    // sent behind itself facing back: before, each unit crossed to the far side of the formation to walk with its final
    // front first, and the formation swung round at the goal) - and each unit goes on to its final place at the goal
    vec2 go = vec2_sub(c, mean);
    float gl = vec2_len(go);
    g_fo_has_march = 0;
    g_fo_naval = 1;
    for (int i = 0; i < n; i++) {
        ee_unit_type *td = unit_get_type_data(g_fo[i].u);
        int mk = td ? model_kind(unit_type_get_model(td)) : 0;
        if (mk != MODEL_SHIPS && mk != MODEL_SUBS) g_fo_naval = 0;
    }
    if (g_ff_on && !circle && gl > 0.5f && vec2_dot(vec2_scale(go, 1.0f / gl), fwd) < FF_ARC) {
        vec2 wf = vec2_scale(go, 1.0f / gl), wr = v2(wf.y, -wf.x);
        for (int i = 0; i < n; i++) {
            vec2 d = vec2_sub(g_fo[i].p, mean);
            g_fo[i].along = vec2_dot(d, wf);
            g_fo[i].across = vec2_dot(d, wr);
        }
        fo_rows(g_fo_order, n, kind, front);
        for (int i = 0; i < n; i++) { g_fo_march[i] = g_fo[i].at; g_fo_order[i] = i; }
        g_fo_has_march = 1;
    }
    // the places: by where the units stand along the facing - following under 45 degrees off it, along the way they
    // go (the march is this layout turned to the way, and it turns into it at the goal: units standing for the way they
    // go - a formation on the move - keep their places)
    vec2 pf = fwd, pr = right;
    if (g_ff_on && !circle && gl > 0.5f && !g_fo_has_march) { pf = vec2_scale(go, 1.0f / gl); pr = v2(pf.y, -pf.x); }
    for (int i = 0; i < n; i++) {
        vec2 d = vec2_sub(g_fo[i].p, mean);
        g_fo[i].along = vec2_dot(d, pf);
        g_fo[i].across = vec2_dot(d, pr);
        g_fo[i].p = vec2_sub(g_fo[i].p, c);   // the circles' angles: around the point
    }
    if (circle) fo_rings(g_fo_order, n); else fo_rows(g_fo_order, n, kind, front);
    // the front row (a wedge's tip) on the clicked point, as the game's: the mouse gives the front line; a circle around it
    vec2 mid = v2(0, 0);
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
    int arc = g_ff_on && g_fo_queued && !circle ? ff_arc_layout(n, c, fwd, right) : -1;   // a gentle turn: the formation as it marches
    for (int i = 0; arc >= 0 && i < n; i++) {
        void *s = formation_slot(slots, i);
        vec3 p = fslot_get_pos(s);
        p.x = g_fo[i].at.x;
        p.y = g_fo[i].at.y;
        fslot_set_pos(s, p);
    }
    g_fo_slots = slots;
    g_fo_group = (ee_active_group *)group;
    g_fo_n = n;
    g_fo_moves++;
    if (g_fo_queued) g_fo_queued_n++;
    g_ff_count = 0;
    if (g_ff_on) ff_start(group, n, c, fwd, right, g_fo_queued, arc);
    // a line a move (the player's games: what a reported layout or march did)
    if (g_log) fprintf(g_log, "# formation: %d units, kind %d, %s%d from their last order%s, from %.1f,%.1f to %.1f,%.1f facing %.2f,%.2f (%s), %d marches\n",
                       n, kind, g_fo_queued ? "queued, " : "", from_last, reface ? " (the click's facing turned to their way)" : "", mean.x, mean.y, c.x, c.y, fwd.x, fwd.y, facing == 0 ? "click" : "drag", g_ff_on ? g_ff_count : 0);
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
            struct ff_unit *fe = u && g_ff_count ? ff_find((uint32_t)unit_get_serial(u), 0) : NULL;
            struct ff_pend *fp = fe ? ff_pend_last(fe) : NULL;
            if (fp && (unsigned)(fp->g - g_ff_first + FF_GROUPS) % FF_GROUPS < (unsigned)g_ff_count) fp->goal = g;   // the goal of the queued march it joins (this move's)
            for (int m = 0; m < g_ff_count && u; m++) {   // a march goes by its leader's goal
                struct ff_group *gr = &g_ff[(g_ff_first + m) % FF_GROUPS];
                if ((uint32_t)unit_get_serial(u) == gr->leader) gr->goal = g;
            }
            for (int k = 0; k < g_fo_n; k++)
                if (g_fo[k].u == u) { pt[0] = g_fo[k].at.x; pt[1] = g_fo[k].at.y; facing = g_fo[k].facing; break; }
            break;
        }
    if (g_fo_group && g_set.formation_march && !g_ff_on) fo_march(g_fo_group);
    g_fo_group = NULL;
    return goal_move_at(g, pt, a, b, c, facing, e, f);
}

internal void groupmove_install(void) {
    g_fo_on = g_set.formations;
    int n = g_fo_on ? patch_formation_slots(hook_formation_slots) : 0, goals = n ? patch_slot_goal(hook_slot_goal) : 0;
    int markers = g_set.goal_markers ? patch_order_marks_all() : 0;   // every unit's circle shows where it goes
    int queued = goals ? patch_path_msg_execute(hook_path_execute) : 0;
    int march = goals && g_set.formation_march ? patch_group_speeds(hook_group_speeds) : 0;
    g_ff_on = goals && g_set.formation_follow ? patch_land_speed_step(hook_ff_speed) == 2 : 0;   // the turn: shiproute.c's hook
    if (g_log) fprintf(g_log, "# groupmove: formations=%d (%d formation vtables, goals hooked %d), goal markers for every unit: %d, queued moves laid out from the last order: %d, march at the slowest speed: %d, follow a leader: %d\n",
                       g_fo_on, n, goals, markers, queued, march, g_ff_on);
}
