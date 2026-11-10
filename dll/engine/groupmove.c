// FORMATIONS BY SIZE (`formations`; docs/engine-notes.md "Group moves"): a group move puts each unit on a slot of the
// group's formation, and the game spaces every row and column of it by the largest unit's collision size (type +0x470)
// - one tank stretches the grid of 170 soldiers - in as few as 5 columns, the rows stacked behind the clicked point,
// and pairs units with slots by distance alone (a tank may get a slot among infantry): a 174-unit army sent 20 tiles
// ended with half of it more than 28 tiles from the click (tests/probes/army_move.eeai). Here every formation is laid
// out again, for any group, from its units' own sizes, its front row on the clicked point and facing the way the game's
// own slots face (a right-button drag turns them), keeping its kind's shape:
//   lines (short, long, loose, staggered): rows of FO_ASPECT[kind] times as wide as deep, each unit as wide as its
//     size x FO_PAD[kind] (the loose and the staggered line's twice its size), a row as deep as its largest unit, a
//     staggered line's every other row shifted half a unit;
//   wedges (deep, shallow): rows from a single tip, each FO_WEDGE_GROW[kind] units wider than the one before;
//   circles (inward, outward): one ring of every unit round the point, as the game's own.
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
internal const float FO_PAD[8] = { 1.4f, 1.4f, 2.0f, 2.0f, 1.25f, 1.25f, 1.0f, 1.0f };   // by formation_kind: the loose line and the staggered line are the loose ones, a circle packed tight (the player, 2026-11-03)
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
        fo_place_row(idx + i, k, y, kind == FORMATION_LOOSE_STAGGERED_LINE && (row & 1) ? wmean * 0.5f : 0);
        prev = deep;
        i += k;
    }
}

// circles: rings from the middle out, the ranks back first (who doesn't fight in the middle, the fighters outside), a
// rank's largest units innermost; on a ring by the angle they stand at now
internal void fo_rings(int *idx, int n) {
    // one ring of every unit round the point, as the game's own circle (the player, 2026-11-03: "in the base game the
    // circle is a one deep circle line around the center" - the rings from the middle out, the largest innermost, filled
    // it): its circumference the units' widths, packed tight (FO_PAD 1: a unit's own size), the units counter-clockwise
    // from +x by where they stand; one unit in the middle. Neighbours stand on the chord, shorter than the arc: the
    // radius widened by the arc's share over the chord (x / sin x, x = pi / n: two units 1.57, three 1.21, eight 1.03,
    // many 1) - by the largest unit's width over the chord before, one large unit among 109 put the ring at 17 times
    // its width (the player, 2026-11-03: "a single large unit shouldn't blow the whole formation over half the map")
    const float TWO_PI = 6.28318531f, PI = 3.14159265f;
    if (n == 1) { g_fo[idx[0]].at = v2(0, 0); return; }
    float sum = 0, x = PI / (float)n;
    for (int a = 0; a < n; a++) sum += g_fo[idx[a]].w;
    float r = sum / TWO_PI * x / fo_sin_q(x);
    for (int a = 0; a < n; a++) g_fo_key[idx[a]] = -fo_pseudo_angle(g_fo[idx[a]].p);
    fo_sort(idx, n, g_fo_key);
    float at = 0;
    for (int a = 0; a < n; a++) {
        float w = g_fo[idx[a]].w, ang = TWO_PI * (at + w * 0.5f) / sum;
        g_fo[idx[a]].at = vec2_scale(fo_dir(ang >= TWO_PI ? ang - TWO_PI : ang), r);
        at += w;
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
internal int g_fo_kind;                 // the layout's formation kind (the button): a march remembers it - another one lays out anew
internal vec2 g_fo_way;                 // the way the march being laid out sets out along: its route's first leg (sr_first_leg), else the way to the point
internal unsigned g_fo_queued_n;        // group moves laid out from the units' last queued points

// the move (EEMPathTo's Execute, the formation's slots laid out within it): whether it is queued - then the formation is
// laid out from where each unit's last order leaves it, not where it stands
internal int g_fo_forced = 1;   // the move's forced flag (0: an attack-move) - for the log line
internal void __thiscall hook_path_execute(ee_path_msg *msg) {
    g_fo_queued = path_msg_get_queued(msg);
    g_fo_forced = path_msg_get_goal_30(msg);
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
// formation_catchup of its own top speed near its point (falling off to none at formation_catchup_falloff x its size),
// or formation_reform for a few seconds after a march began or it was last in its place. Its own route stays: whenever its point is further than FF_FAR, on a tile it may not
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
#define FF_FOES 32             // the enemies a holding formation's units close in on, at most
#define FF_ROOM_TILES 48       // the points of the formation's line measured for room at most (one every tile of its depth and the look beyond it)
#define FF_TRAIL 128           // the anchor's trail: its place at this many leader steps back (a column of 109 two abreast is 65 tiles deep, a step about a tile)
#define FF_UNITS 4096          // the followers' table (open, by unit id)
#define FF_FAR 24.0f           // tiles
#define FF_END 3.0f            // tiles
#define FF_END_PLACE 0.75f     // tiles: a formation moved as it stands (in_place 2) marches to this near its end - its units are at their offsets already
#define FF_GAIN 0.5f           // tiles/s for each tile behind
#define FF_CREEP 6.0f          // tiles: a column's unit ahead of its place with no narrow ahead of it goes at its point's speed less a share per tile ahead, none this far ahead
#define FF_AHEAD 1.5f          // tiles: the point steered at lies this far ahead of the follower's own (no circling on it)
#define FF_LOOK 3.0f           // tiles: the anchor heads for the leader's route this far ahead of it
#define FF_INNER 1.0f          // tiles: the anchor's tightest arc leaves the innermost unit this far inside it
#define FF_DROP 2.0f           // tiles
// (formation_catchup_falloff, 4 x its size: a follower this far from its point and further has no catch-up speed)
#define FF_KEEP 10             // steps: a unit steered at its point this recently keeps the formation's speed - a ship
                               // isn't steered every tick, and the game's step in between braked it: its speed a
                               // sawtooth, and every speed-up threw out a bow wave (the player: far too dense)
#define FF_GRACE 5.0f          // s: ... unless it was in its place (or its march began: a re-form) this long ago or less
#define FF_FRESH 20            // steps: a unit whose catch-up share was reckoned this recently comes from a march, else from rest
#define FF_WAIT_GAIN 0.5f      // tiles: a straggler the formation waits for must gain this on its point within FF_OUT_S, else it goes on without waiting
#define FF_LINK 4.0f           // tiles: units this close where they set out (chained) march as one block - the gap
                               // between their spots' edges (their sizes padded as laid out: a fixed 4 tiles between
                               // the centres left ships of a fleet out, each on its own route - the player)
#define FF_SETTLE 5.0f         // s: the formation turns from the way to its final facing this long before the leader arrives,
#define FF_SETTLE_MAX 15.0f    // up to this long when its outermost units need it to swing round at its speed
#define FF_SWING 1.0f          // its facing turns at most this much x its speed / its reach a second (its outermost units
                               // swing round at about its speed)
#define FF_TURN_EASE 0.33f     // the turn slowdown follows the turning share eased by this much a leader step (a route corner's at once: the speed jumped)
#define FF_LINGER 10.0f        // s: the formation lingers this long after the last of its units came to rest, and an
                               // idle unit pushed off its spot meanwhile walks back to it (ff_linger_tick) -
#define FF_LINGER_MAX 60.0f    // s: at most this long after the march ended (a unit that never comes to rest)
#define FF_HOME_SLOP 0.5f      // x its size (at least FF_HOME_MIN): further off than this,
#define FF_HOME_MIN 0.25f      // tiles
#define FF_HOME_FAR 3.0f       // tiles: and no further (one further off didn't stray by a push: it never got there, blocked)
#define FF_HOME_LOST 30.0f     // tiles: one further off than FF_HOME_FAR, up to this, standing with no order left, the game gave up on short of its spot - its route cut by its detour round a blocker (RepathAroundObstacle keeps a few waypoints and drops the rest; 8 of the siphon's 71 stood 3-9 tiles short in the crowd at the goal): it walks back by a route
#define FF_HOME_TRIES 3        // walks back at most in a linger (a spot it can't reach - behind a wall - isn't tried for ever)
#define FF_HOME_EVERY 10       // steps: the idle units looked at this often
// fwd: the formation's final facing, way: where it goes, face: its facing now, reach: its outermost unit's distance from
// the leader, goal: the leader's goal of this move (NULL: not known - its group
// tells), live: on its way this step (else queued behind the leader's current order), lead_off: the leader's spot from
// the formation's point in its frame, through: a queued march goes on from it in the same formation (it doesn't end
// short of the leader's spot), onward: then the next leg's way, passed: the anchor past the leader's spot then; anchor:
// the point the formation hangs on (ff_leader), way its heading, vel its speed, turning its share of the tightest turn
// (eased over the leader steps: FF_TURN_EASE), started: set out (prev: the march it went on from, else -1), t0 the step then, linger the step it arrived
// (0: not yet, or not arrived - given another order)
struct ff_group { uint32_t leader; ee_group *grp; ee_goal *goal; vec2 fwd, way, face, end, lead_off, onward, anchor, way_from, face_from, way_rot, face_rot; float speed, reach, turn_r, acc, pace, pace_next, jam, jam_next, vel, vel_from, turning, dt, side; uint32_t step, t0, linger, at_ms; float sp, front, rear, fw, col_w, col_rear; int n, cols; int on, live, through, passed, started, prev, reform, arc, pivot, naval, in_place, kind; uint32_t settled; vec2 trail[FF_TRAIL]; int trail_n, trail_at; uint32_t drain, rel_at; int stopped, attack, fight; float narrow_to, line_t[FF_ROOM_TILES + 1], line_room[FF_ROOM_TILES + 1]; int line_n; vec2 line_at; float conv, line_l[FF_ROOM_TILES + 1], line_r[FF_ROOM_TILES + 1]; vec2 foe[FF_FOES]; int foe_n; };   // foe: where the enemies its fighters attack stand (ff_fighting)   // in_place: a re-form within the formation's own size - 1 no march, each unit on its own route, the formation sorted out at the end as after a march; 2 a formation moved as it stands: a march straight to its end, facing its final facing from the start, to FF_END_PLACE of its end; settled: the step its last unit came to rest after it arrived (0: not yet)   // sp: its members' mean spacing, front: the front rank's offset, fw: the formation's width, n: its units, cols: narrowed (ff_fill_column: col_w the width its column is packed to, col_rear its last rank's offset; 0: as laid out)   // arc: it goes on from prev in the same formation (else it re-forms on the move: pivot - its anchor turns onto the new leg sharply); naval: ships   // side: how far right of the leader's route the formation's line runs (ff_side: off an edge the route hugs)   // stopped: the march ended with no unit on its move - the player's stop (the linger puts back only the pushed-off); attack: an attack-move (its units take on what they meet, the formation holds while they fight; fight: how many fought at the last step)   // drain: the step the anchor reached its end with the column still in a narrow (0: not draining), rel_at the step a unit was last let go, narrow_to how far along the line (ff_progress's measure) the narrow ends (ff_narrow); line_t, line_room: the line's points as last measured (how far along, the room across: -1 none), line_n of them
internal struct ff_group g_ff[FF_GROUPS];
internal unsigned g_ff_next;
internal int g_ff_first, g_ff_count;   // the marches started by the move being dispatched, for their leaders' goals
// step: the last it followed; pend (1-based, into g_ff_p): the first of the queued moves' marches it joins, each when it
// sets out; home its spot at the goal
struct ff_unit { uint32_t id; int g, pend; vec2 off, home, col; float w, cw, gap, prog; uint32_t ahead, prog_at; int rel, resent; uint32_t step; float along, with, off_by, share, grace_cap, want; vec2 lag_pos; uint32_t in_at, share_at, yaw, lag_at, rerouted_at; int tries, own, out, order, waited, merge, halt, close; uint32_t wait_at, jam_at; float wait_along, debt; };   // halt: held standing by the merge (ff_stand), debt: how far its place has moved on meanwhile   // jam_at: the step it was first held up (behind its point, asking its top speed, going slower), 0: not now   // waited: 1 a straggler the formation waits for (formation_wait), till it is within its catch-up of its point, -1 one it gave up on till then (wait_at, wait_along: the step it last gained on its point, and how far behind it was then)   // order: its place front to back, left to right in its formation; col: its place in the narrowed column (ff_off), w its width as laid out, cw its width in a column (ff_fill_column), ahead the first unit of the rank before its (0: the front rank) and gap the spacing to that rank, prog how far along the line it was at its last tick (ff_progress), at step prog_at; rel: let go to its spot as the column drained; out: dropped out of its march (lag_at, lag_pos: since when it lags, where it stood then); own: why it took its own route last (1 its point too far, 2 the point steered at on a tile it may not enter, 3 not in a clear line); in_at: the step it was last in its place (or its march began); share its catch-up share at share_at, grace_cap the most of it the grace gives; yaw its facing there (the slot's float, 0: none), tries its walks back
internal struct ff_unit g_ff_u[FF_UNITS];
internal unsigned g_ff_in_place;    // re-forms in place: no march
internal unsigned g_ff_wait_gave_up;   // stragglers the formation stopped waiting for (formation_wait): not gaining on their points
internal int g_ff_watch;   // `fmove watch 1` (test): every march's anchor logged every leader step (# ffwatch: its speed, the slowdowns)
// a queued move's march a unit joins once it sets out (its offset, its spot and facing at the goal, its goal in that move),
// after (1-based) the one queued behind it - a unit had one, so a third waypoint's took the second's place and from the
// second on each unit went its own way (a tank at its own top speed)
#define FF_PEND 4096
struct ff_pend { uint32_t id; int g, after, order; vec2 off, home; float w; uint32_t yaw; ee_goal *goal; };
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
    gr->way = gr->face = g_fo_has_march && from < 0 ? g_fo_way : tl > 0.01f ? vec2_scale(to, 1.0f / tl) : fwd;   // laid out for the way: it sets out along it (the route's first leg)
    gr->naval = g_fo_naval;
    gr->kind = g_fo_kind;
    gr->end = g_fo[lead].at;
    vec2 lc = vec2_sub(gr->end, c);
    gr->lead_off = v2(vec2_dot(lc, right), vec2_dot(lc, fwd));
    gr->through = gr->passed = gr->started = gr->pivot = gr->in_place = 0;
    gr->settled = 0;
    gr->drain = 0;
    gr->stopped = gr->fight = 0;
    gr->attack = !g_fo_forced;   // the move message's flag (the slot goals are made forced and get the flag after)
    gr->narrow_to = -1e9f;
    gr->conv = 0;
    gr->arc = from >= 0;
    gr->reform = from < 0 && g_fo_has_march;
    gr->prev = from;
    gr->vel = gr->vel_from = gr->turning = gr->dt = gr->side = 0;
    gr->trail_n = gr->trail_at = 0;
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
    gr->pace = gr->pace_next = gr->jam = gr->jam_next = -1;
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
            p->id = e->id; p->g = gi; p->after = 0; p->off = off; p->order = rank_of[j]; p->w = g_fo[i].w; p->home = g_fo[i].at; p->yaw = g_fo[i].facing; p->goal = NULL;
            if (last) last->after = pi + 1;
            else e->pend = pi + 1;
            continue;
        }
        e->g = gi;
        e->pend = 0;
        e->out = 0;
        e->rel = 0;
        e->prog_at = 0;
        e->waited = 0;
        e->lag_at = 0;
        e->off = off;
        e->order = rank_of[j];
        e->w = g_fo[i].w;
        e->home = g_fo[i].at;
        e->step = ~0u;
        ff_grace_from(e);
        e->yaw = g_fo[i].facing;
        e->tries = e->resent = 0;
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

// a re-form in place (the marches just started by ff_start): no march - its units take their own routes to their spots
// as the game sends them, and the formation is sorted out at the end as after a march (ff_linger_tick: a unit pushed
// off its spot walks back; the player, 2026-11-02: without it "the formation always seems to end up messy"). A
// formation moved as it stands (ff_keep_layout) marches (in_place 2): the anchor straight from the leader to its spot,
// the formation facing its final facing from the start, every unit at its offset - they hold the formation on the way
// (the player, 2026-11-02: on their own routes "the moving units don't hold the formation like they do for a drag")
internal void ff_in_place(int march) {
    g_ff_in_place++;
    for (int i = 0; i < g_ff_count; i++) {
        struct ff_group *gr = &g_ff[(g_ff_next - (unsigned)g_ff_count + (unsigned)i) % FF_GROUPS];
        gr->in_place = march ? 2 : 1;
        if (march) gr->face = gr->fwd; else gr->started = 1;
    }
}
// the marches of the units just laid out (g_fo[0..n)): a block each - the units within FF_LINK of each other where they
// set out, chained (two blocks sent to the point between them can't follow one leader: each goes to its part of the
// formation behind a leader of its own); a unit on its own takes its own route. All at the slowest member's top speed
internal int g_ff_root[FO_MAX], g_ff_idx[FO_MAX];
internal int ff_root(int i) { while (g_ff_root[i] != i) i = g_ff_root[i] = g_ff_root[g_ff_root[i]]; return i; }
internal char g_ff_no_march[96];   // why the last layout got no march (the log line): empty when it did
internal void ff_start(void *group, int n, vec2 c, vec2 fwd, vec2 right, int queued, int arc) {
    g_ff_count = 0;
    g_ff_no_march[0] = 0;
    float slow = 1e9f;
    int naval = -1;   // land units or ships and submarines, not both: they can't keep one formation
    for (int i = 0; i < n; i++) {
        ee_unit_type *td = unit_get_type_data(g_fo[i].u);
        int kind = td ? model_kind(unit_type_get_model(td)) : 0, nv = kind == MODEL_SHIPS || kind == MODEL_SUBS;
        if (!kind || (naval >= 0 && nv != naval)) {
            snprintf(g_ff_no_march, sizeof g_ff_no_march, !kind ? ", no march: type %d moves by no land or sea model" : ", no march: ships and land units together (type %d)", td ? unit_type_get_id(td) : -1);
            return;
        }
        naval = nv;
        float v = unit_max_speed(g_fo[i].u);
        if (v < slow) slow = v;
    }
    if (slow <= 0) { snprintf(g_ff_no_march, sizeof g_ff_no_march, ", no march: a unit's top speed is 0"); return; }
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

// a queued move turning so little from the last march its units are on or queued for (all of them, all on it) that
// the formation wheels (ff_wheels): the same formation goes on - each unit's spot its offset from the leader in that march, turned to the new facing
// around the leader's spot (as far from the point c as before) - instead of a new layout; the march's index, else -1
internal vec2 g_fo_arc_off[FO_MAX];
// a formation of this reach (its outermost unit's distance from its middle) turning by the angle of this cosine keeps
// its layout - turns as it stands, each unit on the chord to its place - when that moves its outermost unit at most
// formation_wheel tiles (the chord 2 x reach x sin(angle/2)); turned further it forms anew, each unit to the nearest
// place. Not an angle alone (45 degrees before - the player, 2026-11-03: "a bit naive: for small groups even 180
// would not be a real problem, while large ones already break before 45"): a squad of six, reach 1.5, wheels right
// round for 3 tiles, the save's wedge of 109, reach 5.9, breaks up at 30 degrees
internal int ff_wheels(float reach, float cosang) {
    float w = (float)g_set.formation_wheel;
    return reach <= w * 0.5f || cosang >= 1 - w * w / (2 * reach * reach);
}
// the formation the units just laid out (g_fo[0..n)) stand in: one march, finished or not, all of its units among them
// (and units of no march: a straggler that was out of reach when it began) - its index, else -1 (a re-form in place of
// a formation moves it as it is: ff_keep_layout)
internal int ff_same_group(int n) {
    int og = -1, in = 0;
    for (int i = 0; i < n; i++) {
        struct ff_unit *e = ff_find((uint32_t)unit_get_serial(g_fo[i].u), 0);
        if (!e || e->g < 0) continue;
        if (og >= 0 && e->g != og) return -1;
        og = e->g;
        in++;
    }
    return og >= 0 && g_ff[og].n == in ? og : -1;
}
// the places of a re-form in place of the formation og: each unit's offset from the leader in it (one of no march:
// where it stands, from the leader's spot), the leader's spot as far from the point c as before - in the layout's frame
// (x right, y ahead), as fo_rows lays them; the formation moves and turns as it is, no unit changes place (the player,
// 2026-11-02: the wedge of the save "slow" ordered 3 tiles ahead and a bit aside - laid out anew, the units given
// their places by where they stand, neighbours swapped places and crossed each other's way: "they look a bit messy")
internal void ff_keep_layout(int og, int n) {
    struct ff_group *old = &g_ff[og];
    vec2 f = old->fwd, r = v2(f.y, -f.x);
    for (int i = 0; i < n; i++) {
        struct ff_unit *e = ff_find((uint32_t)unit_get_serial(g_fo[i].u), 0);
        vec2 off;
        if (e && e->g == og) off = e->off;
        else { vec2 d = vec2_sub(g_fo[i].from, old->end); off = v2(vec2_dot(d, r), vec2_dot(d, f)); }
        g_fo[i].at = v2(off.x + old->lead_off.x, off.y + old->lead_off.y);
    }
}
internal int ff_arc_layout(int n, vec2 c, vec2 fwd, vec2 right) {
    struct ff_unit *e0 = ff_find((uint32_t)unit_get_serial(g_fo[0].u), 0);
    struct ff_pend *p0 = e0 ? ff_pend_last(e0) : NULL;
    int og = p0 ? p0->g : e0 ? e0->g : -1;
    if (og < 0 || !g_ff[og].on || g_ff[og].kind != g_fo_kind) return -1;
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
    if (ll < 0.5f || !ff_wheels(old->reach, vec2_dot(old->way, vec2_scale(leg, 1.0f / ll)))) return -1;
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
    for (int i = 0; i < n; i++) {
        if (unit_order_at(l, i) != gr->goal) continue;
        for (int k = 0; k < i; k++) if (!goal_is_attack(unit_order_at(l, k))) return 0;   // behind another move: queued
        return 1;   // current - or behind the attacks of an attack-move: on it, fighting (the formation holds, ff_leader)
    }
    return -1;
}

// where the anchor heads: the leader's route FF_LOOK ahead of it (its waypoints, round what blocks; only those ahead -
// by their distance along the way: the anchor runs beside the route by its side, a waypoint abreast is not ahead),
// near the leader's spot on along the next leg when a march goes on from this one (the turn begins before the corner),
// past it along the next leg. *route: 1 a point of the route (ff_side moves the formation's line off it), 0 the end;
// *leg the route's direction on from that point (to its next waypoint, else the end; the way when there is none), *qi
// the point's index among the leader's waypoints (-1: not one of them)
internal vec2 ff_pursue(struct ff_group *gr, ee_unit *l, int *route, vec2 *leg, int *qi) {
    vec2 a = gr->anchor;
    *route = 0;
    *qi = -1;
    *leg = gr->way;
    if (gr->passed) return vec2_add(a, vec2_scale(gr->onward, FF_LOOK));
    float de = vec2_len(vec2_sub(gr->end, a));
    if (de < FF_LOOK) return gr->through ? vec2_add(gr->end, vec2_scale(gr->onward, FF_LOOK - de)) : gr->end;
    int n = unit_get_waypoints_left(l);
    for (int i = 0; i < n && i < 32; i++) {
        float w[3];
        if (unit_waypoint_at(l, i, w)) break;
        vec2 q = v2(w[0], w[1]), d = vec2_sub(q, a);
        if (vec2_dot(d, gr->way) >= FF_LOOK) {
            vec2 next = gr->end;
            if (i + 1 < n && !unit_waypoint_at(l, i + 1, w)) next = v2(w[0], w[1]);
            vec2 on = vec2_sub(next, q);
            float ol = vec2_len(on);
            if (ol > 0.5f) *leg = vec2_scale(on, 1.0f / ol);
            *route = 1;
            *qi = i;
            return q;
        }
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

// THE TRAIL (the player's "siphon", 2026-11-03: 71 units through a channel 3 tiles wide and 20 long, "the chokepoint
// handling is very bad, many units get stuck / are separated from the main formation"): a land march's units behind the
// anchor take their points on the anchor's trail - the path it came, at their offset back along it, their offset across
// from the path's heading there - so a column snakes along the route and a corner is turned by each rank where the
// anchor turned it. Every unit's point a fixed offset from the anchor turned with the facing (as a fleet's still are,
// and a formation moved as it stands), the body swung round the anchor at a corner, its depth too: the anchor turned
// into the channel on the formation's turning circle of 7 tiles, 2-3 tiles outside it inside the wall, and the rear
// ranks swept through the wall - a quarter of the followers' steps on their own routes, the last unit at 135 s. With
// the rear on the trail the anchor turns on the formation's half-width (narrowed: the column's) plus FF_INNER instead
// of its reach. The trail holds the anchor's place at each leader step (ff_trail_push); before it began, straight back
// along its oldest piece (the way, from rest). A queued march takes the march before's trail on with its anchor
internal void ff_trail_push(struct ff_group *gr) {
    if (gr->trail_n) {
        vec2 last = gr->trail[(gr->trail_at - 1 + FF_TRAIL) % FF_TRAIL];
        if (vec2_dist2(last, gr->anchor) < 0.05f * 0.05f) return;   // standing: no piece
    }
    gr->trail[gr->trail_at] = gr->anchor;
    gr->trail_at = (gr->trail_at + 1) % FF_TRAIL;
    if (gr->trail_n < FF_TRAIL) gr->trail_n++;
}
// the point `back` tiles behind the anchor along its trail, *dir the trail's heading there
internal vec2 ff_trail_at(const struct ff_group *gr, vec2 anchor, vec2 way, float back, vec2 *dir) {
    vec2 p = anchor, d = way;
    for (int k = 0; k < gr->trail_n; k++) {
        vec2 q = gr->trail[(gr->trail_at - 1 - k + 2 * FF_TRAIL) % FF_TRAIL], s = vec2_sub(p, q);
        float sl = vec2_len(s);
        if (sl < 0.01f) continue;
        d = vec2_scale(s, 1.0f / sl);
        if (back <= sl) { *dir = d; return vec2_sub(p, vec2_scale(d, back)); }
        back -= sl;
        p = q;
    }
    *dir = d;
    return vec2_sub(p, vec2_scale(d, back));
}
// e's point (its offset off, the across part scaled by `across`: drawn in), *dir the heading of the formation there: a
// land march's rear on the trail, the rest its offset from the anchor turned with the facing
internal vec2 ff_point(const struct ff_group *gr, vec2 anchor, vec2 way, vec2 face, vec2 off, float across, vec2 *dir) {
    if (!gr->naval && gr->in_place != 2 && off.y < 0) {
        vec2 p = ff_trail_at(gr, anchor, way, -off.y, dir);
        return vec2_add(p, vec2_scale(v2(dir->y, -dir->x), off.x * across));
    }
    *dir = face;
    return vec2_add(anchor, vec2_add(vec2_scale(v2(face.y, -face.x), off.x * across), vec2_scale(face, off.y)));
}

// NARROWING (the player: a ray from the formation's anchor for the room there is, and where it sees room for one or two
// abreast a fitting formation for the while): each step the room across the formation's line is measured at every tile
// from its rear (along the anchor's trail) to FF_NARROW_LOOK beyond its front (along the leader's route: ff_line) - the
// ground across its width the leader may enter, unbroken from the line out (ff_rooms) - and where
// the narrowest is less than FF_NARROW_AT of the formation's width it fills its rows anew, front to back and left to right in its order, as many abreast as
// fit at its members' mean spacing; it widens again once that room is there along all of it. The jam it had made
// at a gap crawled through: its units couldn't all fit, the formation waited for them
#define FF_NARROW_LOOK 6.0f    // tiles beyond its front
#define FF_NARROW_AT ((float)g_set.formation_narrow)   // of the formation's width (formation_narrow, 40%)
#define FF_WIDEN_AT (FF_NARROW_AT + (1 - FF_NARROW_AT) * 0.5f)   // 70% at 40%
#define FF_TIGHT_AT 0.4f       // of the formation's width: with less room the column is packed tight (FF_COL_GAP), with more its units keep the formation's own spacing
internal unsigned g_ff_narrowed;   // steps a formation went narrowed
internal vec2 ff_off(const struct ff_group *gr, const struct ff_unit *e) { return gr->cols ? e->col : e->off; }
// how far along the formation's line (the anchor's trail behind it, the way ahead, straight back before the trail
// began) p lies: the arc length of the line's point nearest it, 0 at the anchor, negative behind; *across (if asked)
// how far right of the line it stands there
internal float ff_progress(const struct ff_group *gr, vec2 anchor, vec2 p, float *across) {
    vec2 a = anchor, dir = gr->way, at = gr->way, nearest;
    float best_d2 = 1e30f, best = 0, gone = 0;
    float s = vec2_dot(vec2_sub(p, a), gr->way);
    if (s < 0) s = 0;
    nearest = vec2_add(a, vec2_scale(gr->way, s));
    best_d2 = vec2_dist2(p, nearest);
    best = s;
    for (int k = 0; k < gr->trail_n; k++) {
        vec2 q = gr->trail[(gr->trail_at - 1 - k + 2 * FF_TRAIL) % FF_TRAIL], seg = vec2_sub(q, a);
        float sl = vec2_len(seg);
        if (sl < 0.01f) { a = q; continue; }
        float t = vec2_dot(vec2_sub(p, a), seg) / (sl * sl);
        t = t < 0 ? 0 : t > 1 ? 1 : t;
        vec2 on = vec2_add(a, vec2_scale(seg, t));
        float d2 = vec2_dist2(p, on);
        dir = vec2_scale(seg, -1.0f / sl);
        if (d2 < best_d2) { best_d2 = d2; best = -(gone + t * sl); at = dir; nearest = on; }
        gone += sl;
        a = q;
    }
    s = -vec2_dot(vec2_sub(p, a), dir);
    if (s > 0 && vec2_dist2(p, vec2_sub(a, vec2_scale(dir, s))) < best_d2) { best = -(gone + s); at = dir; nearest = vec2_sub(a, vec2_scale(dir, s)); }
    if (across) *across = vec2_dot(vec2_sub(p, nearest), v2(at.y, -at.x));   // from the line's nearest point (from the anchor, past a bend of the trail the measures of two rank-mates didn't compare)
    return best;
}
// the least room across the line from `from` tiles along it (ff_progress's measure) on, as last measured (ff_narrow);
// no more than the formation's width - a rank past the narrow spreads to that, no wider
internal float ff_room_ahead(const struct ff_group *gr, float from) {
    float room = gr->fw;
    for (int i = 0; i < gr->line_n; i++) if (gr->line_room[i] >= 0 && gr->line_t[i] >= from - 1.0f && gr->line_room[i] < room) room = gr->line_room[i];
    return room;
}
// where the next narrow begins from `from` tiles along the line on (ff_progress's measure, as measured at the last
// leader step: ff_narrow) - the first point with too little room for the whole formation, its index; -1: none ahead
internal int ff_mouth_ahead(const struct ff_group *gr, float from) {
    for (int i = 0; i < gr->line_n; i++)
        if (gr->line_room[i] >= 0 && gr->line_t[i] >= from && gr->line_room[i] < gr->fw * FF_NARROW_AT) return i;
    return -1;
}
// THE COLUMN (the player's "siphon", 2026-11-03: "a good chunk of the soldiers still get siphoned off by the wall, only
// one soldier after another trickling into the 2 tile wide chokepoint is very inefficient; the tanks look silly being
// a mass that pushes through the corridor, they should move in order through it"): the narrowed formation's places,
// filled anew whenever it narrows. The ranks go to the units by how far along the line they are, the furthest first -
// filled in the formation's order before, a unit that stood at the back of the line got a rank 40 tiles behind the
// anchor and waited in the bowl while one from the front got a rank beyond the wall and walked round the wall's end
// to it (its point across the wall: its own route, along the wall) - and each rank holds as many as fit the room by
// their own widths, as deep as its widest (ranks of the members' mean spacing put two tanks where two soldiers fit:
// they pushed through as a mass). The front rank at the formation's front, the last rank's offset col_rear. A unit
// whose place lies behind it (the wide body before the gap: the rear ranks' places 30 tiles back along the trail)
// merges (fo_follow_aim THE MERGE; the player, 2026-11-10: "the formations get really messy when going through tight
// spots"): standing for its place it looked sluggish (68 of 71 standing at 12 s), queued up close behind the rank
// ahead and creeping on (FF_CREEP, 2026-11-03) the whole pile pressed into the channel as one mass - 71 units in 11
// tiles of a channel their column is 39 deep in, tanks three abreast where two fit, 26 pairs overlapping at any
// time. Filled anew every leader step: each rank as
// wide as the least room from where its first unit stands on (ff_room_ahead: the head spreads out past a gap while
// the tail is still in it - packed to the narrowest point of the whole line, a 2.5-tile gap behind the rear put the
// tanks single file through the channel ahead, where two fit), the ranks' order kept once given (by their progress
// alone two units side by side swapped ranks as either edged ahead), each rank's places left to right by where its
// units stand (not by their progress: that swapped their lanes). In the drain the places stay as
// last filled and move on (gr->conv)
#define FF_COL_GAP 0.15f       // tiles: a column's units stand this clear of each other, collider to collider (a tank 1.15 apart, a soldier 0.65: two tanks or three soldiers abreast in a channel of 3 tiles) - closer than the formation's own padding (FO_PAD; 1.4 x the size for a line: through a channel 20 tiles long the column of 71 was 40 deep, and it drains as fast as it is short). At the game's own formation sizes (a tank's 1.5) the tanks went single file through a channel two of them fit (the player, 2026-11-03: "the one line tank row makes sense for a one width channel, for a two wide it's a bit wasteful")
#define FF_MERGE_MIN 0.5f      // tiles: a column's unit this far ahead of its place or further merges (below: its place's own point)
#define FF_MERGE_EASE 2.0f     // tiles: a unit in the narrow this far ahead of its place (beyond FF_MERGE_MIN) stands for it, nearer it eases
#define FF_LAG_LINE 2.5f       // tiles: a column's unit this far behind its place walks along the line in its file (its place may be round a bend)
#define FF_MOUTH_IN 1.0f       // tiles: a merging unit is within the mouth's opening this far before the mouth
#define FF_FUNNEL 0.75f        // tiles aside of its file a merging unit may stay for each tile it is ahead of its place
#define FF_COL_MARGIN 0.3f     // tiles: a column keeps this clear of what bounds its room on each side
// fresh: the formation just narrowed (the ranks by progress), else the ranks' order stays, progress between equals
internal void ff_fill_column(struct ff_group *gr, float room, int fresh) {
    static int idx[FF_UNITS];
    static float key[FF_UNITS], prog[FF_UNITS], side[FF_UNITS];
    int gi = (int)(gr - g_ff), n = 0;
    for (int i = 0; i < FF_UNITS; i++) {
        struct ff_unit *e = &g_ff_u[i];
        if (!e->id || e->g != gi || e->out || e->rel) continue;
        ee_unit *u = unit_by_id(e->id);
        if (!u || unit_get_hp(u) <= 0) continue;
        vec3 p = unit_get_pos(u);
        float sz = unit_get_size(u);
        e->cw = (sz < FO_MIN_SIZE ? FO_MIN_SIZE : sz) + FF_COL_GAP;
        if (room >= gr->fw * FF_TIGHT_AT && e->w > e->cw) e->cw = e->w;
        idx[n] = i;
        prog[n] = ff_progress(gr, gr->anchor, v2(p.x, p.y), &side[n]);
        key[n] = fresh ? prog[n] : e->col.y * 1000.0f + prog[n];
        n++;
    }
    for (int a = 1; a < n; a++)   // insertion sort: the front first
        for (int b = a; b > 0 && key[b] > key[b - 1]; b--) {
            int t = idx[b]; idx[b] = idx[b - 1]; idx[b - 1] = t;
            float kt = key[b]; key[b] = key[b - 1]; key[b - 1] = kt;
            kt = prog[b]; prog[b] = prog[b - 1]; prog[b - 1] = kt;
            kt = side[b]; side[b] = side[b - 1]; side[b - 1] = kt;
        }
    gr->col_w = room;
    gr->col_rear = gr->front;
    float y = gr->front, last_deep = 0;
    uint32_t ahead = 0;
    for (int at = 0; at < n;) {
        int end = at;   // the rank: at .. end-1
        float wide = 0, deep = 0, fit = ff_room_ahead(gr, prog[at]) - 2 * FF_COL_MARGIN;
        while (end < n && (end == at || wide + g_ff_u[idx[end]].cw <= fit)) { float cw = g_ff_u[idx[end]].cw; wide += cw; if (cw > deep) deep = cw; end++; }
        uint32_t lead = g_ff_u[idx[at]].id;   // the rank's front-most: the rank behind queues up behind it
        for (int a = at + 1; a < end; a++)   // its places left to right by where they stand
            for (int b = a; b > at && side[b] < side[b - 1]; b--) {
                int t = idx[b]; idx[b] = idx[b - 1]; idx[b - 1] = t;
                float st = side[b]; side[b] = side[b - 1]; side[b - 1] = st;
            }
        float gap = (last_deep + deep) * 0.5f;
        if (at > 0) y -= gap;
        float x = -wide * 0.5f;
        for (int j = at; j < end; j++) { struct ff_unit *e = &g_ff_u[idx[j]]; e->col = v2(x + e->cw * 0.5f, y); e->ahead = ahead; e->gap = gap; x += e->cw; }
        ahead = lead;
        last_deep = deep;
        gr->col_rear = y - deep * 0.5f;
        at = end;
    }
}
// the room from p outward along dir, to lim: how far the ground it may enter goes on unbroken. The passable ground
// counted along the whole ray before: a wall a tile thick with the open behind it counted as room, and the formation
// marched with a flank in the wall - and into a channel 3 tiles wide between two walls (the player's "siphon") 71 units
// 11 wide never narrowed. To the blocked tile's edge: sampled every half tile alone it read 2.5 for a channel of 3
// (the edge lay between the last sample passed and the first blocked), and two tanks 1.15 apart didn't fit it less the
// margins
// *wall: what blocks goes on along the ray for FF_WALL_RUN tiles or more (a wall across the way, the ray along it; a
// tree or a house ends sooner)
#define FF_WALL_RUN 3.0f
internal float ff_room_side(ee_map *map, ee_unit *u, ee_unit_type *td, int size, vec2 p, vec2 dir, float lim, int *wall) {
    float room = 0;
    *wall = 0;
    for (float d = 0.5f; d <= lim; d += 0.5f) {
        vec2 q = vec2_add(p, vec2_scale(dir, d));
        int tx = (int)floorf(q.x), ty = (int)floorf(q.y);
        if (!sr_pass(map, u, td, size, tx, ty)) {   // where the ray enters the tile: the further of its two entering sides
            float ex = dir.x > 1e-6f ? ((float)tx - p.x) / dir.x : dir.x < -1e-6f ? ((float)tx + 1 - p.x) / dir.x : -1e9f;
            float ey = dir.y > 1e-6f ? ((float)ty - p.y) / dir.y : dir.y < -1e-6f ? ((float)ty + 1 - p.y) / dir.y : -1e9f;
            float e = ex > ey ? ex : ey;
            if (e > room && e < d) room = e;
            *wall = 1;
            for (float b = d + 0.5f; b <= d + FF_WALL_RUN && *wall; b += 0.5f) {
                vec2 w = vec2_add(p, vec2_scale(dir, b));
                if (sr_pass(map, u, td, size, (int)floorf(w.x), (int)floorf(w.y))) *wall = 0;
            }
            break;
        }
        room = d;
    }
    return room;
}
// the room left and right of a line of n points (pts, the line's heading at each in dirs): rooms[i][0] left, [1] right
// - a point's the most of its own and its neighbours' along the line (a tree or a house beside the way is passed, a
// wall or a shore runs on: the nearest thing that blocks narrowed a formation in the open, and it came apart), -1 where
// the line's own tile is blocked (a bend: no measure). A wall across the way (ff_room_side: what blocks runs on along
// the ray) is its own point's measure whatever its neighbours': a wall a tile thick with a gap in it was passed like
// a tree, and a formation twice the gap's width never narrowed for it (2026-11-10)
internal void ff_rooms(ee_map *map, ee_unit *u, ee_unit_type *td, int size, const vec2 *pts, const vec2 *dirs, int n, float lim, float rooms[][2], int walls) {
    float raw[FF_ROOM_TILES + 1][2];
    int wall[FF_ROOM_TILES + 1][2];
    for (int i = 0; i < n; i++) {
        vec2 q = pts[i], right = v2(dirs[i].y, -dirs[i].x), left = v2(-dirs[i].y, dirs[i].x);
        raw[i][0] = raw[i][1] = -1;
        wall[i][0] = wall[i][1] = 0;
        if (sr_pass(map, u, td, size, (int)floorf(q.x), (int)floorf(q.y))) { raw[i][0] = ff_room_side(map, u, td, size, q, left, lim, &wall[i][0]); raw[i][1] = ff_room_side(map, u, td, size, q, right, lim, &wall[i][1]); }
    }
    for (int i = 0; i < n; i++) {
        rooms[i][0] = rooms[i][1] = -1;
        for (int j = i - 1; j <= i + 1; j++) {
            if (j < 0 || j >= n) continue;
            if (raw[j][0] > rooms[i][0]) rooms[i][0] = raw[j][0];
            if (raw[j][1] > rooms[i][1]) rooms[i][1] = raw[j][1];
        }
        if (walls && wall[i][0]) rooms[i][0] = raw[i][0];
        if (walls && wall[i][1]) rooms[i][1] = raw[i][1];
    }
}
// the formation's line as it runs, as points a tile apart (at most cap, further apart then) from `back` tiles behind
// the anchor (negative: along its trail) to `front` ahead (along the leader's route: the point pursued q - its index
// among the waypoints qi, -1 none - and the waypoints on from it, straight on past the last), the line's heading at
// each in dirs, how far along the line each lies (ff_progress's measure) in ts (NULL: not wanted). The room is measured across it. A straight line along the way from the anchor ran into the wall at
// the channel's corner: 0.5 tiles of room there, and 71 units went through one abreast, a column 85 tiles long
internal int ff_line(const struct ff_group *gr, ee_unit *l, vec2 q, int qi, float back, float front, vec2 *pts, vec2 *dirs, float *ts, int cap) {
    // no further back than the trail goes: the march's start (behind it the line is fiction - 36 tiles of it ran to
    // the map's edge, "room 0.0", and the siphon's column went one abreast)
    float len = 0;
    vec2 last = gr->anchor;
    for (int k = 0; k < gr->trail_n; k++) {
        vec2 t_k = gr->trail[(gr->trail_at - 1 - k + 2 * FF_TRAIL) % FF_TRAIL];
        len += vec2_len(vec2_sub(last, t_k));
        last = t_k;
    }
    if (back < -len) back = -len;
    float span = front - back, step = span > (float)(cap - 1) ? span / (float)(cap - 1) : 1.0f, t = back;
    int n = 0;
    for (; t < 0 && n < cap; t += step) { pts[n] = ff_trail_at(gr, gr->anchor, gr->way, -t, &dirs[n]); if (ts) ts[n] = t; n++; }
    vec2 a = gr->anchor, b = q, dir = gr->way;
    float gone = 0, sl = vec2_len(vec2_sub(b, a));
    int i = qi, nw = l ? unit_get_waypoints_left(l) : 0;
    for (; t <= front && n < cap; t += step) {
        while (gone + sl < t) {   // onto the next piece: the next waypoint, else straight on
            float w[3];
            gone += sl;
            a = b;
            if (i >= 0 && i + 1 < nw && i + 1 < 32 && !unit_waypoint_at(l, i + 1, w)) { i++; b = v2(w[0], w[1]); }
            else { i = -1; b = vec2_add(a, vec2_scale(dir, 1000.0f)); }
            sl = vec2_len(vec2_sub(b, a));
        }
        if (sl > 0.01f) dir = vec2_scale(vec2_sub(b, a), 1.0f / sl);
        pts[n] = vec2_add(a, vec2_scale(dir, t - gone));
        dirs[n] = dir;
        if (ts) ts[n] = t;
        n++;
    }
    return n;
}
internal uint32_t ff_step(void) { ee_physics *ph = game_physics(); return ph ? physics_get_steps(ph) : 0; }

// seconds since step (the formation's steps)
internal float ff_since(uint32_t step) {
    ee_physics *ph = game_physics();
    return (float)(ff_step() - step) * (ph ? physics_get_step_s(ph) : 0.05f);
}
// TRADING PLACES (the player, 2026-11-11: "when the group had to narrow on its march they break into chaos on arrival
// at their destination, could it be that they reform into the original layout then? never updating the destination
// with the renegotiated position in the formation?" - so it was: each unit kept the spot of the order's layout, and
// the column had changed who is in front): when a narrowed formation spreads out again, or reaches its end narrowed,
// its places are given out anew among the units of one type - the type's places front to back, row by row, to its
// units as far along the line as they are now, a row's left to right as they stand - so the layout stays and nobody
// walks through the others to an old place. A unit with a new place gets its offset in the march, its spot at the
// goal and a move there in place of its own (one with orders queued after this move keeps its place)
internal unsigned g_ff_traded;   // units given another unit's place
internal int ff_on_this(const struct ff_unit *e, ee_unit *u);
internal void ff_trade(struct ff_group *gr) {
    static int idx[FF_UNITS], slot[FF_UNITS];
    static float prog[FF_UNITS], side[FF_UNITS];
    static struct { vec2 off, home; uint32_t yaw; int order; float w; } was[FF_UNITS];
    int gi = (int)(gr - g_ff), n = 0;
    if (gr->naval || gr->in_place) return;
    for (int i = 0; i < FF_UNITS; i++) {
        struct ff_unit *e = &g_ff_u[i];
        if (!e->id || e->g != gi || e->out || e->rel) continue;
        ee_unit *u = unit_by_id(e->id);
        if (!u || unit_get_hp(u) <= 0 || unit_orders(u) != 1 || !ff_on_this(e, u)) continue;
        vec3 p = unit_get_pos(u);
        idx[n] = slot[n] = i;
        prog[i] = ff_progress(gr, gr->anchor, v2(p.x, p.y), &side[i]);
        was[i].off = e->off; was[i].home = e->home; was[i].yaw = e->yaw; was[i].order = e->order; was[i].w = e->w;
        n++;
    }
    // by type; a type's units the furthest along first, its places the foremost first (and left to right)
    #define FF_TYPE_OF(i) unit_type_get_id(unit_get_type_data(unit_by_id(g_ff_u[i].id)))
    for (int a = 1; a < n; a++) {
        int v = idx[a], b = a, tv = FF_TYPE_OF(v);
        for (; b > 0; b--) { int w = idx[b - 1], tw = FF_TYPE_OF(w); if (tw < tv || (tw == tv && prog[w] >= prog[v])) break; idx[b] = w; }
        idx[b] = v;
    }
    for (int a = 1; a < n; a++) {
        int v = slot[a], b = a, tv = FF_TYPE_OF(v);
        for (; b > 0; b--) {
            int w = slot[b - 1], tw = FF_TYPE_OF(w);
            if (tw < tv || (tw == tv && (was[w].off.y > was[v].off.y + 0.05f || (was[w].off.y >= was[v].off.y - 0.05f && was[w].off.x <= was[v].off.x)))) break;
            slot[b] = w;
        }
        slot[b] = v;
    }
    for (int at = 0; at < n;) {   // a row of one type's places: at .. end-1, its units by where they stand across
        int end = at + 1, t = FF_TYPE_OF(slot[at]);
        while (end < n && FF_TYPE_OF(slot[end]) == t && was[slot[end]].off.y > was[slot[at]].off.y - 0.05f) end++;
        for (int a = at + 1; a < end; a++) {
            int v = idx[a], b = a;
            for (; b > at && side[idx[b - 1]] > side[v]; b--) idx[b] = idx[b - 1];
            idx[b] = v;
        }
        at = end;
    }
    #undef FF_TYPE_OF
    for (int k = 0; k < n; k++) {
        if (idx[k] == slot[k]) continue;
        struct ff_unit *e = &g_ff_u[idx[k]];
        ee_unit *u = unit_by_id(e->id);
        void *mem = goal_move_alloc();
        if (!mem) return;
        e->off = was[slot[k]].off; e->home = was[slot[k]].home; e->yaw = was[slot[k]].yaw; e->order = was[slot[k]].order; e->w = was[slot[k]].w;
        float xy[2] = { e->home.x, e->home.y }, yaw;
        ee_goal *g = gr->attack ? goal_attack_move_init(mem, xy) : goal_move_init(mem, xy);
        memcpy(&yaw, &e->yaw, sizeof yaw);
        if (e->yaw == 0) yaw = fo_atan2(-gr->fwd.x, gr->fwd.y);
        location_goal_set_yaw((ee_location_goal *)g, yaw);
        location_goal_set_face((ee_location_goal *)g, 1);
        unit_assign_goal(u, g, 0, 0);
        unit_set_state(u, 0x1F40);
        unit_type_goal_assigned(unit_get_type_data(u), u);
        if (e->id == gr->leader) gr->goal = unit_get_goal(u);
        g_ff_traded++;
    }
    if (g_log) fprintf(g_log, "# formation: %d units at %.1f,%.1f: places given out anew%c", gr->n, gr->anchor.x, gr->anchor.y, 10);
}
internal void ff_narrow(struct ff_group *gr, ee_unit *l, vec2 q, int qi) {
    ee_map *map = game_map();
    ee_unit_type *td = l ? unit_get_type_data(l) : NULL;
    int size = map ? (int)map_get_size(map) : 0;
    if (gr->n < 2 || gr->sp <= 0 || !td || size <= 0 || !sr_alloc((uint32_t)(size * size))) return;
    if (gr->in_place == 2) return;   // a formation moved as it stands: it goes across its facing, the room across its way is no measure of it (form_turn's 104 turned 2.7 tiles away narrowed to a column and drained for 54 s)
    st_stamp_step();
    int full = (int)(gr->fw / gr->sp);   // abreast as laid out (about)
    float back = gr->rear;   // its rear: the laid out one's, or the narrowed one's
    if (gr->cols && gr->col_rear < back) back = gr->col_rear;
    if (gr->cols) {   // and the column's hindmost unit: the queue holds the tail far behind its places once the head spreads out past a gap, and the line measured to the places' rear no longer reached the gap - the formation widened with its tail still in it, and the march ended with the tail at the entry on its own routes
        int gi = (int)(gr - g_ff);
        for (int i = 0; i < FF_UNITS; i++) {   // measured now: a unit standing in the queue ticks no follow, its own measure is from when it last moved
            const struct ff_unit *e = &g_ff_u[i];
            if (!e->id || e->g != gi || e->out || e->rel) continue;
            ee_unit *u = unit_by_id(e->id);
            if (!u || unit_get_hp(u) <= 0 || !unit_orders(u)) continue;
            vec3 p = unit_get_pos(u);
            float pr = ff_progress(gr, gr->anchor, v2(p.x, p.y), NULL);
            if (pr - 1 < back) back = pr - 1;
        }
    }
    float room = 1e9f, lim = gr->fw * 0.5f + 1, rooms[FF_ROOM_TILES + 1][2], ts[FF_ROOM_TILES + 1];
    vec2 pts[FF_ROOM_TILES + 1], dirs[FF_ROOM_TILES + 1];
    int nr = ff_line(gr, l, q, qi, back, gr->front + FF_NARROW_LOOK, pts, dirs, ts, FF_ROOM_TILES + 1);
    ff_rooms(map, l, td, size, pts, dirs, nr, lim, rooms, FF_NARROW_AT > FF_TIGHT_AT);   // walls across the way only with formation_narrow raised: at 40% march_gap's 3-tile gap read 2.0 so (3.0 by its neighbours), one abreast, the last unit 28 s later
    float narrow_to = -1e9f;   // where the narrow ends: the last point along the line with too little room for the whole formation (the drain lets a unit go past it)
    gr->line_n = nr;
    for (int i = 0; i < nr; i++) {
        gr->line_t[i] = ts[i];
        gr->line_room[i] = -1;
        gr->line_l[i] = rooms[i][0]; gr->line_r[i] = rooms[i][1];
        if (rooms[i][0] < 0 || rooms[i][1] < 0) continue;
        float r = rooms[i][0] + rooms[i][1];
        gr->line_room[i] = r;
        if (r < room) room = r;
        if (r < gr->fw * FF_NARROW_AT && ts[i] > narrow_to) narrow_to = ts[i];   // the narrow as it narrows a formation: by the widening's room it ran 4 tiles past the channel's end, and the head waited there in a clump
    }
    gr->narrow_to = narrow_to;
    gr->line_at = gr->anchor;
    // narrowed below FF_NARROW_AT of its width, narrower still where the room shrinks, as laid out again once FF_WIDEN_AT
    // of it is there along all of it (the whole width: a few trees about kept a formation narrowed for good) - each change
    // fills the rows anew (it went 4, 5, 4 abreast from step to step: every unit to a new place)
    int narrow = room < gr->fw * (gr->cols ? FF_WIDEN_AT : FF_NARROW_AT), cols = narrow ? (int)(room / gr->sp) : 0;
    if (narrow && cols < 1) cols = 1;
    if (cols >= full) cols = 0;
    if (cols && gr->cols && cols > gr->cols) cols = gr->cols;
    if (cols) ff_fill_column(gr, room, !gr->cols);
    if (cols != gr->cols && g_log)   // for the player's games: when and where a formation narrowed or spread out again
        fprintf(g_log, "# formation: %d units at %.1f,%.1f %s (room %.1f of its %.1f tiles%s)\n", gr->n, gr->anchor.x, gr->anchor.y,
                cols ? "narrowed to a column" : "as laid out again", room, gr->fw, cols ? "" : "");
    if (cols && g_log && cols != gr->cols) fprintf(g_log, "# formation: the column %.1f wide, %.1f tiles deep\n", gr->col_w, gr->front - gr->col_rear);
    if (!cols && gr->cols) ff_trade(gr);
    gr->cols = cols;
    if (cols) g_ff_narrowed++;
}

// CENTRING (the player, 2026-11-03: "the formations still drag along edges"): the leader's route is found for one unit
// and runs as close by what it rounds as that unit fits, so along a shore or a wood the followers on that side had their
// points in the water or the trees and went by their own routes, strung out behind. Each step the room to each side of
// the route ahead is measured (ff_line, ff_rooms: from the anchor along the route to FF_NARROW_LOOK beyond the front;
// the body behind is where it is; the route's own tile blocked - a bend - not a measure), and the
// formation's line is moved off the route to where every tile has the formation's half-width (narrowed: the column's)
// to each side - each tile allows a range of moves, the line takes the one nearest the route that all allow, and where
// none does (a corridor narrower than the formation, a bend) the middle of what the tightest tiles ask from each side -,
// at most FF_SIDE_RATE of the anchor's step a step (its heading turns gently: an abrupt move across tripped the turn
// slowdown) and back onto the route where the ground opens. A tile's room is the most of its own and its neighbours'
// along the way: a tree or a house beside the way is passed, a wall or a shore runs on. Not within the formation's reach
// of the end: the route's end is the leader's spot, every unit's spot its offset from it. Returns what it wants this
// step (the log). The move that fixes the tile with the least room was tried first: it flipped between the front's
// shore and the rear's cliff of a diagonal corridor from step to step, and the zigzag's turn slowdown halved the march
#define FF_SIDE_RATE 0.5f
internal float g_ff_side_log[3];   // `fmove watch` (# ffwatch): the tiles measured (-1: none fit, -2: no route point), the least room left and right
internal float ff_side(struct ff_group *gr, ee_unit *l, vec2 q, int qi, int route, float base, float dt) {
    ee_map *map = game_map();
    ee_unit_type *td = l ? unit_get_type_data(l) : NULL;
    int size = map ? (int)map_get_size(map) : 0;
    float s = 0, hw = (gr->cols ? gr->col_w : gr->fw) * 0.5f;
    float de = vec2_len(vec2_sub(gr->end, gr->anchor));
    if (route && !gr->naval && gr->n >= 2 && hw > 0.5f && td && size > 0 && de > gr->reach + FF_LOOK * 2 && sr_alloc((uint32_t)(size * size))) {
        st_stamp_step();
        float lim = hw * 2, rooms[FF_ROOM_TILES + 1][2];
        vec2 pts[FF_ROOM_TILES + 1], dirs[FF_ROOM_TILES + 1];
        int n = ff_line(gr, l, q, qi, 0, gr->front + FF_NARROW_LOOK, pts, dirs, NULL, FF_ROOM_TILES + 1);   // ahead: the body behind is where it is
        ff_rooms(map, l, td, size, pts, dirs, n, lim, rooms, gr->cols != 0);   // a wall across the way only for a formation narrowed for it: for any, the end of a wood by the way moved the line (three units: one 43 s late)
        // each tile allows the line moved right by hw - left room up to right room - hw (a move that leaves it the
        // half-width to each side; none when its corridor is narrower than the formation)
        float lo = -1e9f, hi = 1e9f, min_l = lim, min_r = lim;
        int seen = 0;
        for (int i = 0; i < n; i++) {
            float l_i = rooms[i][0], r_i = rooms[i][1];
            if (l_i < 0 || r_i < 0) continue;
            if (hw - l_i > lo) lo = hw - l_i;
            if (r_i - hw < hi) hi = r_i - hw;
            if (l_i < min_l) min_l = l_i;
            if (r_i < min_r) min_r = r_i;
            seen++;
        }
        if (seen) s = lo > hi ? (lo + hi) * 0.5f : lo > 0 ? lo : hi < 0 ? hi : 0;
        g_ff_side_log[0] = (float)seen; g_ff_side_log[1] = min_l; g_ff_side_log[2] = min_r;
    }
    else g_ff_side_log[0] = route ? -1 : -2;
    float most = base * dt * FF_SIDE_RATE, d = s - gr->side;
    gr->side += d > most ? most : d < -most ? -most : d;
    return s;
}

// THE DRAIN (the player, 2026-11-03: "it did not take long for it to break down completely again, many units just
// giving up and standing in the channel"): the march ended when the anchor came within FF_END of its end, and every
// unit went on to its spot on its own route - with the column 41 tiles deep on a march of 38, half of it was still in
// the channel then: on the game's routes they went single file along the wall, each blocked by the one ahead, and the
// game's detour round a blocker (RepathAroundObstacle keeps a few waypoints and drops the rest) left eight of them at
// a last waypoint in the channel, their orders done, standing for good (the test's "done" counted them: no orders
// left). Now a narrowed land formation's anchor stands at its end and the march goes on - the queue drains the column
// through the narrow - and each unit is let go to its spot on its own route once it is FF_REL_PAST tiles past the
// narrow's end (ff_narrow: the last point of the line with too little room for the whole formation, as measured at
// the last leader step; the units behind still queue up behind it, its progress measured on); the march ends when
// none is held any more, or after FF_DRAIN_MAX
#define FF_DRAIN_MAX 120.0f    // s
#define FF_REL_PAST 1.5f       // tiles
internal unsigned g_ff_released;   // units let go to their spots as their column drained
internal unsigned g_ff_handed;     // marches whose lead was handed on (ff_hand_over)
#define FF_DRAIN_STALL 3.0f    // s: a drain that let nobody go this long lets its front rank go
#define FF_CLOSE_RANGE 0.85f   // of its own range: a unit closing in on the fight comes this near the enemy,
#define FF_CLOSE_MIN 0.75f     // tiles: at least this near (melee)
internal unsigned g_ff_closing;   // ticks a unit closed in on its formation's fight
#define FF_FIGHT_LEASH 4.0f    // tiles: a fighter further than the formation's reach plus this from the anchor is chasing off - the formation moves on without it
#define FF_DRAIN_IDLE 15.0f    // s: a drain that let nobody go this long ends (the goal inside the narrow: the column's front stands at its place short of the release line)
// u is on this march's move: its order a move to its spot (a member sent elsewhere alone keeps e->g, as does a member of
// an earlier march in the same slot - the group test told them apart, and the drain can't use it: the leader's group
// may be gone)
internal int ff_on_this(const struct ff_unit *e, ee_unit *u) {
    ee_goal *g = unit_get_goal(u);
    if (!g || !goal_is_location(g)) return 0;
    vec3 to = goal_target(g, u);
    return vec2_dist2(v2(to.x, to.y), e->home) < 1.5f * 1.5f;
}
// a unit of gr still held in its column: alive, on this march's move (one idle - arrived, or given up on by the game -
// ticks no more and could never be let go), not dropped out, not let go
internal int ff_held(const struct ff_group *gr) {
    int gi = (int)(gr - g_ff);
    for (int i = 0; i < FF_UNITS; i++) {
        const struct ff_unit *e = &g_ff_u[i];
        if (!e->id || e->g != gi || e->out || e->rel) continue;
        ee_unit *u = unit_by_id(e->id);
        if (u && unit_get_hp(u) > 0 && unit_orders(u) && ff_on_this(e, u)) return 1;
    }
    return 0;
}
// most of gr's units idle further than FF_HOME_FAR from their spots: a march that ends with nobody on the move was
// stopped by the player if so (at its natural end only the few the game gave up on short of their spots stand that
// far; the units let go as the column drained are still walking, on their own orders)
internal int ff_most_far(const struct ff_group *gr) {
    int gi = (int)(gr - g_ff), all = 0, away = 0;
    for (int i = 0; i < FF_UNITS; i++) {
        const struct ff_unit *e = &g_ff_u[i];
        if (!e->id || e->g != gi) continue;
        ee_unit *u = unit_by_id(e->id);
        if (!u || unit_get_hp(u) <= 0) continue;
        vec3 p = unit_get_pos(u);
        all++;
        if (!unit_orders(u) && vec2_dist2(e->home, v2(p.x, p.y)) > FF_HOME_FAR * FF_HOME_FAR) away++;
    }
    return away * 2 > all;
}
// the drain over - none held, nobody let go for FF_DRAIN_IDLE, or FF_DRAIN_MAX passed -: the march ends and the
// formation lingers (ff_linger_tick walks the units given up on back to their spots), 1; else 0. Asked at each leader
// step and between steps (ff_linger_tick): with every unit idle no tick asks ff_leader, and the drain never ended -
// no linger, nobody walked back
internal int ff_drain_done(struct ff_group *gr, uint32_t step) {
    int held = ff_held(gr);
    if (held && ff_since(gr->drain) < FF_DRAIN_MAX && ff_since(gr->rel_at) < FF_DRAIN_IDLE) return 0;
    if (!held) gr->stopped = ff_most_far(gr);   // nobody on the move: the player's stop if most stand short
    if (g_log) fprintf(g_log, "# formation: %d units at %.1f,%.1f: the column drained in %.0f s%s\n", gr->n, gr->anchor.x, gr->anchor.y, ff_since(gr->drain), held ? " (given up on the rest)" : "");
    gr->on = gr->live = 0;
    gr->linger = step ? step : 1;
    return 1;
}

// the leader's move dropped by the game (idle with no order left, short of its spot: blocked in the press at a gap's
// mouth for a few seconds - the player's "enclosure", 2026-11-03: the march ended there, and the rest went on by the
// game's own routes, "a large part of the formation stuck at the entry") or the leader dead: the unit furthest along
// still on this march's move leads on - its route serves the anchor from here (the anchor's end and every offset stay
// the old leader's: its spot is still where the anchor heads), the march's goal its own. NULL when nobody is on the
// move any more: the player's stop
// gr's units fighting now near it: their current order an attack (an attack-move's: the game put it on top of the
// move when the unit spotted an enemy; a plain move takes no enemies on). One chasing off beyond the formation's reach
// plus FF_FIGHT_LEASH drops out: the formation moves on, it comes after on its own route
internal int ff_fighting(struct ff_group *gr) {
    int gi = (int)(gr - g_ff), n = 0;
    gr->foe_n = 0;
    for (int i = 0; i < FF_UNITS; i++) {
        struct ff_unit *e = &g_ff_u[i];
        if (!e->id || e->g != gi || e->out) continue;
        ee_unit *u = unit_by_id(e->id);
        ee_goal *g = u && unit_get_hp(u) > 0 ? unit_get_goal(u) : NULL;
        if (!g || !goal_is_attack(g)) continue;
        vec3 p = unit_get_pos(u);
        if (vec2_len(vec2_sub(v2(p.x, p.y), gr->anchor)) > gr->reach + FF_FIGHT_LEASH) { e->out = 1; g_ff_out++; continue; }
        n++;
        ee_unit *t = attack_goal_get_target((ee_attack_goal *)g);
        if (t && unit_get_hp(t) > 0 && gr->foe_n < FF_FOES) {
            vec3 tp = unit_get_pos(t);
            int have = 0;
            for (int k = 0; k < gr->foe_n && !have; k++) have = vec2_dist2(gr->foe[k], v2(tp.x, tp.y)) < 0.25f;
            if (!have) gr->foe[gr->foe_n++] = v2(tp.x, tp.y);
        }
    }
    return n;
}
internal ee_unit *ff_hand_over(struct ff_group *gr, const char *why) {
    int gi = (int)(gr - g_ff);
    struct ff_unit *best = NULL;
    ee_unit *bu = NULL;
    for (int i = 0; i < FF_UNITS; i++) {
        struct ff_unit *e = &g_ff_u[i];
        if (!e->id || e->g != gi || e->out || e->rel || e->id == gr->leader) continue;
        ee_unit *u = unit_by_id(e->id);
        if (!u || unit_get_hp(u) <= 0 || !unit_orders(u) || !ff_on_this(e, u)) continue;
        if (!best || (e->prog_at && (!best->prog_at || e->prog > best->prog))) { best = e; bu = u; }
    }
    if (!best) return NULL;
    if (g_log) fprintf(g_log, "# formation: %d units at %.1f,%.1f: the leader %06x %s, %06x leads on\n", gr->n, gr->anchor.x, gr->anchor.y, gr->leader & 0xFFFFFF, why, best->id & 0xFFFFFF);
    gr->leader = best->id;
    gr->goal = unit_get_goal(bu);
    g_ff_handed++;
    return bu;
}

// how fast the draining column's places move on, as a share of the march's speed: all of it while its units keep up,
// less by how far the held ones are behind their places on average (beyond half a tile; the ones let go ahead of them
// slow in the crowd at their spots, and places that ran on round the bend were out of the followers' clear line -
// they took their own routes), FF_CONV_MIN at least: the places lead, a column waiting for places that wait for it
// would stand
#define FF_CONV_MIN 0.2f
internal float ff_conv_share(const struct ff_group *gr) {
    int gi = (int)(gr - g_ff), n = 0;
    float lag = 0;
    uint32_t step = ff_step();
    for (int i = 0; i < FF_UNITS; i++) {
        const struct ff_unit *e = &g_ff_u[i];
        if (!e->id || e->g != gi || e->out || e->rel || !e->prog_at || step - e->prog_at > 20) continue;
        float d = e->col.y + gr->conv - e->prog;
        if (d > 0) lag += d;
        n++;
    }
    float share = n ? 1.5f - lag / (float)n : 1;
    return share > 1 ? 1 : share < FF_CONV_MIN ? FF_CONV_MIN : share;
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
    if (gr->in_place == 1) {   // no march: nobody steered; the leader's move done, the formation lingers (ff_linger_tick looks too)
        if (!l || unit_get_hp(l) <= 0 || !unit_orders(l)) { gr->on = 0; gr->linger = step ? step : 1; }
        return NULL;
    }
    if (l && !gr->drain && gr->started && unit_get_goal(l) && goal_is_attack(unit_get_goal(l))) {   // the leader fights: the anchor would follow its chase - another leads on (the fighter back to its place after, as any)
        ee_unit *nl = ff_hand_over(gr, "fighting");
        if (nl) l = nl;
    }
    if (l && gr->goal && gr->started && !gr->drain && unit_get_hp(l) > 0 && ff_on_move(gr, l) < 0) {   // the leader sent on again to its spot (ff_linger_tick, its move dropped) before this step saw it idle: it leads on by its new goal (else "another order", and the march ended for good)
        struct ff_unit *le = ff_find(gr->leader, 0);
        if (le && ff_on_this(le, l)) gr->goal = unit_get_goal(l);
    }
    vec3 p = l ? unit_get_pos(l) : v3(0, 0, 0);
    int on = l && unit_get_hp(l) > 0 ? ff_on_move(gr, l) : -1;
    if (gr->drain && on < 0 && l && unit_get_hp(l) > 0 && !unit_orders(l)) on = 1;   // draining: the leader's own move is done, the column still drains (another order ends it)
    if (on < 0 && !gr->drain && gr->started && (!l || unit_get_hp(l) <= 0 || !unit_orders(l))) {   // its move dropped, or dead: another leads on (ff_hand_over); nobody on the move: stopped
        ee_unit *nl = ff_hand_over(gr, "idle, its move dropped short of its spot");
        if (nl) { l = nl; on = 1; }
        else gr->stopped = ff_most_far(gr);
    }
    if (on < 0) {   // the leader's move done (it idles - or died: the others still arrive), else it was given another order
        gr->on = 0;
        if (gr->started && !gr->linger && (!l || unit_get_hp(l) <= 0 || !unit_orders(l))) gr->linger = step ? step : 1;
        return NULL;
    }
    if (!on) return NULL;
    gr->live = 1;
    // the slowest member's speed now last step (fo_follow_aim; -1: none told)
    gr->pace = gr->pace_next;
    gr->jam = gr->jam_next;
    gr->pace_next = gr->jam_next = -1;
    ee_physics *ph = game_physics();
    float dt = (ph ? physics_get_step_s(ph) : 0.05f) * (float)steps;
    if (gr->drain) {   // the anchor stands at its end while the column drains (above)
        if (ff_drain_done(gr, step)) return NULL;
        gr->pace = gr->jam = gr->pace_next = gr->jam_next = -1;
        gr->vel_from = gr->vel = 0;
        gr->way_from = gr->way;
        gr->face_from = gr->face;
        gr->way_rot = gr->face_rot = v2(1, 0);
        gr->dt = dt;
        gr->at_ms = game_time();
        gr->conv += gr->speed * dt * ff_conv_share(gr);   // the column's places move on at the march's speed: the column walks out through the narrow in its order
        return l;
    }
    // ATTACK-MOVE (the player, 2026-11-03: "attack move isn't working in formation" - the units formed up and marched,
    // but one that spotted an enemy was steered on to its spot all the same, its attack on top of its move never let
    // it close): a fighter is the game's own (ff_march), and the formation holds while any fight - the anchor stands,
    // the rest keep their places about it - and moves on when the fight is over, the fighters back to their spots by
    // the catch-up
    int fighting = ff_fighting(gr);   // any: with the march going on through its fighters (a tenth of it fighting held it, fewer not) the rest pushed them along, and a reload never finished - 50 doughboys died for 4 of yellow's 48, where the game's own attack-move killed all 48 for 39
    if (g_log && fighting && !gr->fight) fprintf(g_log, "# formation: %d units at %.1f,%.1f: holds, %d fighting\n", gr->n, gr->anchor.x, gr->anchor.y, fighting);
    if (g_log && !fighting && gr->fight) fprintf(g_log, "# formation: %d units at %.1f,%.1f: moves on, the fight over\n", gr->n, gr->anchor.x, gr->anchor.y);
    gr->fight = fighting;
    if (fighting) {
        gr->pace = gr->jam = gr->pace_next = gr->jam_next = -1;
        gr->vel_from = gr->vel = 0;
        gr->way_from = gr->way;
        gr->face_from = gr->face;
        gr->way_rot = gr->face_rot = v2(1, 0);
        gr->dt = dt;
        gr->at_ms = game_time();
        return l;
    }
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
            memcpy(gr->trail, pv->trail, sizeof gr->trail);
            gr->trail_n = pv->trail_n;
            gr->trail_at = pv->trail_at;
        }
        else {
            // from rest: the formation laid over the group where it fits best along the way - the anchor at the leader,
            // moved by the mean of how far ahead of their points the land units stand there (the leader is the unit
            // whose spot is nearest the formation's middle, standing anywhere in the group: form_place's stood at the
            // group's rear edge, every unit's place 7 tiles behind it). Those standing behind their places close on
            // them, those ahead wait for them (fo_follow_aim: abreast), the slowest trail by at most half the spread.
            // Set ahead by the largest lag before (da60ba3: no unit's point behind it), the save's wedge marching 41
            // degrees off its facing had its anchor 7 tiles ahead of its middle, the rear ran at its top speed for
            // 35-50 s closing and the slowest, with no speed to spare, trailed the whole way (the player, 2026-11-02:
            // "still feels weird"); at the leader alone, half of form_place stood 10-20 s. Never past the end (a few
            // units standing far ahead - stragglers - put the anchor beyond the leader's spot once, and it came round
            // on the formation's turning circle: "they run in a large arc for a minute"). Ships keep their start:
            // they turn as one on their circles
            gr->anchor = v2(p.x, p.y);
            if (!gr->naval && gr->in_place != 2) {   // moved as it stands: every unit at its offset already
                // the way: the leader's route's first waypoint from where it stands - its route is there now, nobody
                // has moved yet, and the layout's estimate of it (sr_first_leg) was 16-33 degrees off where the
                // route really went on the player's "siphon": the anchor swung onto the route over the first 10 s
                // ("they still rotate after the first command")
                int route = 0, qi = -1;
                vec2 leg, q0 = ff_pursue(gr, l, &route, &leg, &qi), d0 = vec2_sub(q0, gr->anchor);
                float dl = vec2_len(d0);
                if (route && dl > 0.5f) gr->way = gr->face = vec2_scale(d0, 1.0f / dl);
                int gi = (int)(gr - g_ff), cnt = 0;
                float lag = 0, most = vec2_dot(vec2_sub(gr->end, gr->anchor), gr->way) - FF_END;
                vec2 right = v2(gr->way.y, -gr->way.x);
                for (int i = 0; i < FF_UNITS; i++) {
                    struct ff_unit *e = &g_ff_u[i];
                    if (!e->id || e->g != gi || e->out) continue;
                    ee_unit *u = unit_by_id(e->id);
                    if (!u || unit_get_hp(u) <= 0) continue;
                    vec3 q = unit_get_pos(u);
                    vec2 pt = vec2_add(gr->anchor, vec2_add(vec2_scale(right, e->off.x), vec2_scale(gr->way, e->off.y)));
                    lag += vec2_dot(vec2_sub(v2(q.x, q.y), pt), gr->way);   // how far ahead of its point it stands
                    cnt++;
                }
                if (cnt) lag /= (float)cnt;
                if (lag > most) lag = most;
                gr->anchor = vec2_add(gr->anchor, vec2_scale(gr->way, lag));
            }
        }
    } else {   // where the last step sent it
        gr->anchor = ff_anchor_now(gr, &w, &f, &vn);
        gr->way = w;
        gr->face = f;
    }
    ff_trail_push(gr);
    // its speed: the march's, no more than formation_wait (80%) of its slowest member's speed now (its top speed and its
    // catch-up share: a unit fallen behind has only its top speed, the formation slows for it to 80% of that; members in
    // their places or ahead of their points set no pace - all in place, it goes at the march's speed; formation_wait=0
    // stands while one is behind), less in a turn
    float wait = g_set.formation_wait;
    float base = gr->pace >= 0 && wait * gr->pace < gr->speed ? wait * gr->pace : gr->speed;
    if (gr->jam >= 0 && gr->jam < base) base = gr->jam;   // a member held up goes as it can: the formation no faster, whatever formation_wait (a jam inside it can't catch up until it moves)
    // re-forming on the move (pivot) it turns onto the new leg at FF_INNER, not at the formation's reach: the units take new
    // places anyway, and the wide swing at a sharp corner put their points on ground they couldn't reach
    // ships turn together: the anchor turns as one ship does (its least agile member's circle at its top speed) and the
    // formation's facing with it, every ship on its own circle to its new place, the pace slowing the fleet while they get
    // there - the whole fleet wheeling round its innermost ship's circle swung far out (the player: too large turns are
    // impractical in the game)
    // a land march turns on its half-width (its rear follows the trail: the depth doesn't swing); a formation moved as it
    // stands on its reach (its body turned whole)
    float hw = gr->in_place == 2 ? gr->reach : (gr->cols ? gr->col_w : gr->fw) * 0.5f;
    float r = gr->naval ? (gr->turn_r > FF_INNER ? gr->turn_r : FF_INNER) : gr->pivot ? FF_INNER : hw + FF_INNER;
    float k = gr->pivot || gr->naval ? 1 : g_set.formation_catchup * 0.85f * r / (r + hw);
    if (k > 1) k = 1;
    // where it heads: the route's point ahead, moved across to the side with room (ff_side)
    int route = 0, qi = -1;
    vec2 leg, q = ff_pursue(gr, l, &route, &leg, &qi);
    float want_side = ff_side(gr, l, q, qi, route, base, dt);
    if (route) q = vec2_add(q, vec2_scale(v2(leg.y, -leg.x), gr->side));
    vec2 to = vec2_sub(q, gr->anchor);
    float tl = vec2_len(to);
    vec2 dir = tl > 0.01f ? vec2_scale(to, 1.0f / tl) : gr->way;
    // the turning share: this step's heading change against the tightest arc's at the base speed, eased by FF_TURN_EASE
    // a step - the slowdown of a corner spread over about three leader steps, a sustained turn's whole after them (the
    // outermost units lag a little through a corner and catch up after it). Before, the share of the step before, whole:
    // every corner of the leader's route (the tightest arc turns 8 degrees a leader step of 2.5 s at a wedge's 7 tiles)
    // dropped the speed to k, 57% for the save's wedge, at 12 of 33 leader steps of a 41-tile march (the player,
    // 2026-11-02: "the turn slowdowns are the main reason it doesn't feel smooth")
    float ch = vec2_len(vec2_sub(dir, gr->way)), tight = base * dt / r;
    float turning = tight > 0 ? (ch > tight ? 1 : ch / tight) : 0;
    gr->turning += (turning - gr->turning) * FF_TURN_EASE;
    float v = base * (1 - gr->turning * (1 - k));
    // a fleet's speed changes no faster than its slowest-accelerating ship can: the pace rocked it - the ships fell behind,
    // the fleet slowed, they closed in, it sped up, they fell behind again (a ship's speed a sawtooth, a bow wave at every
    // speed-up)
    float dv = gr->acc * dt;
    if (gr->naval && gr->acc < 1e8f) v = v > vn + dv ? vn + dv : v < vn - dv ? vn - dv : v;
    // its heading towards dir, no faster than the arc (the chord for the angle: small steps)
    float most = v * dt / r;
    vec2 way0 = gr->way, face0 = gr->face;
    gr->way = ch <= most ? dir : fo_turn(gr->way, dir, most / ch);
    if (ch <= most) gr->pivot = 0;   // on the new leg: its turns at its reach again
    gr->at_ms = game_time();
    gr->vel_from = vn;
    gr->vel = v;
    if (g_ff_watch && g_log)
        fprintf(g_log, "# ffwatch %u %d anchor %.3f %.3f way %.3f %.3f vel %.3f base %.3f march %.3f turning %.2f k %.2f pace %.2f jam %.2f side %.2f want %.2f seen %.0f room %.1f %.1f\n", gr->at_ms, (int)(gr - g_ff), gr->anchor.x, gr->anchor.y, gr->way.x, gr->way.y, v, base, gr->speed, gr->turning, k, gr->pace, gr->jam, gr->side, want_side, g_ff_side_log[0], g_ff_side_log[1], g_ff_side_log[2]);
    // the anchor at the leader's spot (where this step takes it): the march done, each unit on to its own; going on to a
    // queued march, the anchor goes on and the leader's leg ends there (its arrival starts the next order, which takes the
    // anchor over)
    float de = vec2_len(vec2_sub(gr->end, vec2_add(gr->anchor, vec2_scale(fo_half(way0, gr->way), (vn + v) * 0.5f * dt)))), end_r = gr->in_place == 2 ? FF_END_PLACE : FF_END;
    if (de < end_r && !gr->through) {
        if (gr->cols && !gr->naval) ff_narrow(gr, l, q, qi);   // the narrow as it lies from the anchor where it stands now (as measured a step before it lay a step's travel back)
        if (gr->cols && !gr->naval && gr->col_w < gr->fw * FF_TIGHT_AT && ff_held(gr)) {   // narrowed to a tight column: the anchor stands here, the column drains (above); a loose one's units find their spots as an unnarrowed formation's do
            gr->drain = gr->rel_at = step ? step : 1;
            ff_trade(gr);
            if (g_log) fprintf(g_log, "# formation: %d units at %.1f,%.1f: at its end narrowed, the column drains past the narrow %.1f tiles back\n", gr->n, gr->anchor.x, gr->anchor.y, -gr->narrow_to);
            gr->vel_from = gr->vel = 0;
            gr->way_from = gr->way;
            gr->face_from = gr->face;
            gr->way_rot = gr->face_rot = v2(1, 0);
            gr->dt = dt;
            return l;
        }
        gr->on = gr->live = 0;
        gr->linger = step ? step : 1;
        return NULL;
    }
    if (de < end_r && gr->through && !gr->passed) { gr->passed = 1; ff_finish_leg(l); }
    // the facing: the heading, turned to the final facing over the last span - its share of the turn the time left on it
    // (the turn's arc for the outermost unit: at most the chord x pi / 2) - no faster than its outermost units swing round
    float spd = gr->speed > 0.05f ? gr->speed : 0.05f, left = de / spd;
    float span = gr->reach * vec2_len(vec2_sub(gr->fwd, gr->way)) * 1.5708f / spd;
    span = span < FF_SETTLE ? FF_SETTLE : span > FF_SETTLE_MAX ? FF_SETTLE_MAX : span;
    vec2 want = gr->in_place == 2 ? gr->fwd : left >= span || gr->through || gr->reform ? gr->way : fo_turn(gr->way, gr->fwd, 1 - left / span);
    if (!gr->naval && gr->in_place != 2) want = gr->way;   // a land march faces its way to the end: its rear is on the trail, a wheel into the final facing would swing the front alone - each unit turns on its spot
    float swing = FF_SWING * spd / (gr->reach > 1 ? gr->reach : 1) * dt;
    ch = vec2_len(vec2_sub(want, gr->face));
    gr->face = ch <= swing || gr->naval ? want : fo_turn(gr->face, want, swing / ch);
    ff_narrow(gr, l, q, qi);
    gr->way_from = way0;
    gr->face_from = face0;
    gr->way_rot = ff_rot_between(way0, gr->way);
    gr->face_rot = ff_rot_between(face0, gr->face);
    gr->dt = dt;
    return l;
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
// not enter, 4 not in a clear line, 5 its point too far, 6 dropped out, 7 let go to its spot as the column drained, 8 fighting); + 10 x its queued marches
internal int ff_watch_state(ee_unit *u) {
    struct ff_unit *e = g_ff_on ? ff_find((uint32_t)unit_get_serial(u), 0) : NULL;
    if (!e) return -1;
    int q = 0;
    for (struct ff_pend *p = ff_pend_of(e, e->pend); p && q < 9; p = ff_pend_of(e, p->after)) q++;
    struct ff_group *gr = e->g >= 0 ? &g_ff[e->g] : NULL;
    ee_goal *cur = unit_get_goal(u);
    int s = !gr ? -1 : !gr->on ? 0 : !gr->live ? 1 : e->out ? 6 : e->rel ? 7 : cur && goal_is_attack(cur) ? 8 : e->step + 1 >= ff_step() ? 2 : e->own == 1 ? 5 : e->own == 2 ? 3 : 4;
    return s + 10 * q;
}

// u's distance from its point in its marching formation this step (test/fleetmove.c), else -1
internal float ff_point_off(ee_unit *u) {
    struct ff_unit *e = g_ff_on ? ff_find((uint32_t)unit_get_serial(u), 0) : NULL;
    return e && e->g >= 0 && g_ff[e->g].on && g_ff[e->g].live && g_ff[e->g].step == ff_step() ? e->off_by : -1;
}

// the safe point between simulation steps, every FF_HOME_EVERY: an idle unit of a formation that arrived, pushed off its
// spot (further than FF_HOME_SLOP x its size, no further than FF_HOME_FAR), walks back to it - a move order of its own,
// and so does one the game gave up on short of its spot (idle, no order left, further off, within FF_HOME_LOST) -
// turning to its formation's facing there: it turns, walks and turns again (a push slid it back sideways). The game's
// own goal, as the AI gives one: the same on every PC. The formation lingers till FF_LINGER after the last of its units
// came to rest (the player, 2026-11-02: "at the end there are supposed to be ~10 seconds for the formation to sort
// itself out" - counted from the march's end, the anchor at the leader's spot, the 10 s were over before a big
// formation's last units had walked to their spots, and the ones they pushed stayed off), FF_LINGER_MAX at most
internal unsigned g_ff_walks, g_ff_walks_lost, g_ff_resent;   // walks back given, of them to units the game gave up on short of their spots; units sent on again while their march went on
// an attack-move's steered units look for enemies (the player, 2026-11-03, yellow's 48 doughboys: the formation
// marched in and died without a shot - 50 for 4, where the game's own attack-move killed all 48 for 39): the unit AI
// script looks only at each waypoint (generic movement.tai: GetNextMoveWaypoint -> LookForEnemies), and a unit steered
// at its point never arrives at one, so it stays in Advance through the enemy. Every FF_HOME_EVERY steps a steered unit
// in Advance is put into LookForEnemies with its scan clock zeroed: an enemy spotted takes it to CheckRange and its
// attack (the game's own, on top of its move), none back to Advance
#define FF_STATE_ADVANCE 10003
#define FF_STATE_LOOK 2015
internal unsigned g_ff_looks;   // such looks given
internal void ff_scan_tick(struct ff_group *gr, uint32_t step) {
    int gi = (int)(gr - g_ff);
    for (int i = 0; i < FF_UNITS; i++) {
        const struct ff_unit *e = &g_ff_u[i];
        if (!e->id || e->g != gi || e->out || e->rel || e->step == ~0u || step - e->step > 2) continue;   // steered this step or the last
        ee_unit *u = unit_by_id(e->id);
        if (!u || unit_get_hp(u) <= 0 || unit_ai_state_id(u) != FF_STATE_ADVANCE) continue;
        unit_set_scan_ms(u, 0);
        if (!find_target(u)) continue;   // the game's own search first: the look state itself stops the unit a tick, every half second it made the march stutter (164 s for 146)
        unit_set_state(u, FF_STATE_LOOK);
        g_ff_looks++;
    }
}
// a land march on its way (the units sent on again while it goes - ships keep their own routes)
internal int ff_marching(const struct ff_group *gr) { return gr->on && gr->started && !gr->naval && gr->in_place != 1 && !gr->drain; }   // not in the drain: a unit sent on again there is on its own goal, and the leader re-sent ended the march (the given up on walk back once it drained)
internal int ff_lingering(const struct ff_group *gr) {
    return !gr->on && gr->linger && ff_since(gr->linger) < FF_LINGER_MAX && (!gr->settled || ff_since(gr->settled) < FF_LINGER);
}
internal void ff_linger_tick(void) {
    uint32_t step = ff_step();
    if (!g_ff_on || step % FF_HOME_EVERY) return;
    static int moving[FF_GROUPS];
    int any = 0;
    for (int k = 0; k < FF_GROUPS; k++) {
        struct ff_group *gr = &g_ff[k];
        if (gr->on && gr->drain) ff_drain_done(gr, step);   // every unit idle: no tick asks ff_leader
        if (gr->on && gr->live && gr->attack && !gr->naval) ff_scan_tick(gr, step);
        if (gr->on && gr->in_place == 1) {   // no march, nobody steering: its leader's move done, it lingers
            ee_unit *l = unit_by_id(gr->leader);
            if (!l || unit_get_hp(l) <= 0 || !unit_orders(l)) { gr->on = 0; gr->linger = step ? step : 1; }
        }
        moving[k] = 0;
        any |= ff_lingering(gr) || ff_marching(gr);
    }
    if (!any) return;
    for (int k = 0; k < FF_UNITS; k++) {   // its units still on their way
        struct ff_unit *e = &g_ff_u[k];
        if (!e->id || e->g < 0 || e->pend || !ff_lingering(&g_ff[e->g])) continue;
        ee_unit *u = unit_by_id(e->id);
        if (u && unit_get_hp(u) > 0 && unit_orders(u)) moving[e->g] = 1;
    }
    for (int k = 0; k < FF_GROUPS; k++) {
        struct ff_group *gr = &g_ff[k];
        if (!ff_lingering(gr)) continue;
        if (moving[k]) gr->settled = 0; else if (!gr->settled) gr->settled = step ? step : 1;
    }
    for (int k = 0; k < FF_UNITS; k++) {
        struct ff_unit *e = &g_ff_u[k];
        if (!e->id || e->g < 0 || e->pend) continue;
        struct ff_group *gr = &g_ff[e->g];
        int marching = ff_marching(gr);   // on its way: a unit the game dropped the move of is sent on at once (it stood till the march's end before: 10-16 of the enclosure's 50 at a gap's mouth for 100 s)
        if (!ff_lingering(gr) && !marching) continue;
        if ((marching ? e->resent : e->tries) >= FF_HOME_TRIES) continue;   // each at most FF_HOME_TRIES, the march's re-sends apart from the linger's walks back (a unit re-sent thrice into the press at the goal had none left for the linger)
        ee_unit *u = unit_by_id(e->id);
        ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
        if (!td || unit_get_hp(u) <= 0 || unit_get_holder(u) || unit_get_frozen(u) || unit_orders(u) || unit_get_speed(u) > 0.05f) continue;
        vec3 p = unit_get_pos(u);
        float d = vec2_len(vec2_sub(e->home, v2(p.x, p.y))), slop = unit_get_size(u) * FF_HOME_SLOP;
        if (d <= (slop > FF_HOME_MIN ? slop : FF_HOME_MIN) || (!marching && d > (gr->stopped ? FF_HOME_FAR : FF_HOME_LOST))) continue;   // stopped by the player: its units stay where they stand, only one pushed off its spot steps back
        // pushed off its spot: straight there only (a spot behind a wall would send it round it); given up on further
        // off: by a route (the goal's own path request, the mod's route with land_routes)
        ee_map *map = game_map();
        int size = map ? (int)map_get_size(map) : 0;
        if (size <= 0 || !sr_alloc((uint32_t)(size * size))) return;
        st_stamp_step();
        if (!sr_pass(map, u, td, size, (int)floorf(e->home.x), (int)floorf(e->home.y)) || (d <= FF_HOME_FAR && !sr_clear(map, u, td, size, p.x, p.y, e->home.x, e->home.y))) { if (marching) e->resent = FF_HOME_TRIES; else e->tries = FF_HOME_TRIES; continue; }
        if (marching) { g_ff_resent++; e->resent++; }
        else if (d > FF_HOME_FAR) g_ff_walks_lost++;
        void *mem = goal_move_alloc();
        if (!mem) return;
        float xy[2] = { e->home.x, e->home.y }, yaw;
        ee_goal *g = gr->attack ? goal_attack_move_init(mem, xy) : goal_move_init(mem, xy);   // an attack-move's unit goes on taking on what it meets
        memcpy(&yaw, &e->yaw, sizeof yaw);
        if (e->yaw == 0) yaw = fo_atan2(-gr->fwd.x, gr->fwd.y);   // a click: the formation's facing
        location_goal_set_yaw((ee_location_goal *)g, yaw);
        location_goal_set_face((ee_location_goal *)g, 1);
        unit_assign_goal(u, g, 0, 0);
        unit_set_state(u, 0x1F40);
        unit_type_goal_assigned(td, u);
        if (!marching) e->tries++;
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
            e->g = p->g; e->off = p->off; e->order = p->order; e->w = p->w; e->home = p->home; e->yaw = p->yaw; e->tries = e->resent = 0; e->step = ~0u; e->pend = p->after; e->out = 0; e->rel = 0; e->prog_at = 0; e->lag_at = 0; ff_grace_from(e);
        }
        else if (!n->on) e->pend = p->after;   // given up: on to the one behind it
    }
    if (e->g < 0) { *l = NULL; return NULL; }
    struct ff_group *gr = &g_ff[e->g];
    *l = gr->on ? ff_leader(gr, step) : NULL;
    ee_goal *cur = unit_get_goal(u);
    if (cur && goal_is_attack(cur)) { e->lag_at = 0; *l = NULL; return NULL; }   // fighting (an attack-move's target on top of its move): the game's own movement, steered to its spot again once the fight is over (not stuck meanwhile: the drop-out's clock reset)
    struct ff_pend *q = ff_pend_of(e, e->pend);
    return *l && (e->id == gr->leader || unit_get_group(u) == unit_get_group(*l) || (q && q->goal && unit_get_goal(u) == q->goal)
                  || ff_on_this(e, u)) ? gr : NULL;   // or on its move by a goal of its own: draining (the leader's move is done and its group may be gone), or sent on again after the game dropped its move (ff_linger_tick)
}

// u's boost over its top speed (the factor less 1, times its share; e->off_by of this step): the catch-up
// (formation_catchup), only near its place - all of it within its size, none from formation_catchup_falloff x its size on
// (a unit fallen behind doesn't run for long stretches at it: it walks, and the formation slows for it) - or the re-form
// boost (formation_reform) for FF_GRACE seconds after it was last in its place (a re-form, the march's start: none is in
// its place yet), falling off, no more than the share it had when the march began (ff_grace_from); the larger boost
// counts (the player, 2026-11-02: the two apart - keeping the place and taking it are different manoeuvres). e->share:
// the larger share, the next march's grace cap
internal float ff_boost(struct ff_unit *e, ee_unit *u, uint32_t step) {
    float sz = unit_get_size(u), place = sz < FO_MIN_SIZE ? FO_MIN_SIZE : sz, fallen = (float)g_set.formation_catchup_falloff;
    float share = e->off_by <= place ? 1 : e->off_by >= fallen * place ? 0 : (fallen * place - e->off_by) / ((fallen - 1) * place);
    ee_physics *ph = game_physics();
    float sstep = ph ? physics_get_step_s(ph) : 0.05f;
    if (!e->in_at || e->off_by <= place) e->in_at = step ? step : 1;
    if (e->off_by <= place) e->grace_cap = 1;   // in its place: the grace's whole again
    float since = (float)(step - e->in_at) * sstep, grace = since >= FF_GRACE ? 0 : 1 - since / FF_GRACE;
    if (grace > e->grace_cap) grace = e->grace_cap;
    e->share = grace > share ? grace : share;
    e->share_at = step ? step : 1;
    float boost = (g_set.formation_catchup - 1) * share, reform = (g_set.formation_reform - 1) * grace;
    return reform > boost ? reform : boost;
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
#define FF_JAM_S 2.0f          // s: held up this long before it counts as a jam (a bump in the crowd braked the whole formation for a leader step)
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
    if (!gr || e->out) { if (g_ff_watch && g_log) fprintf(g_log, "# ffaim %u %06x %s\n", game_time(), (unsigned)unit_get_serial(u) & 0xFFFFFF, !gr ? "nomarch" : "out"); return 0; }
    vec2 way, face;
    float vel;
    vec2 anchor = ff_anchor_now(gr, &way, &face, &vel), dir;
    // CLOSING IN (the player, 2026-11-11: "once any unit gets into attack range the whole formation stops, only the
    // units that initially saw the enemy fight, the others are only drawn in once these units died or the enemy
    // approaches them"): while the formation holds for a fight, a unit of it that can fight and has no target walks
    // at the nearest enemy its fighters attack, at its top speed, to within FF_CLOSE_RANGE of its own range of it
    // (the look for enemies, ff_scan_tick, gives it its target on the way: the game's own attack from there) - no
    // further from the anchor than the fighters' leash. The ones that don't fight keep their places
    e->close = 0;
    if (gr->fight && gr->attack && gr->foe_n && !gr->naval && fo_rank(u) < 3) {
        vec2 here = v2(pos[0], pos[1]), f = gr->foe[0];
        for (int k = 1; k < gr->foe_n; k++) if (vec2_dist2(gr->foe[k], here) < vec2_dist2(f, here)) f = gr->foe[k];
        ee_unit_stats *st = unit_get_stats(u);
        float d = vec2_len(vec2_sub(f, here)), range = st ? unit_stats_get_range(st) : 0, stop = range * FF_CLOSE_RANGE;
        if (stop < FF_CLOSE_MIN) stop = FF_CLOSE_MIN;
        if (d > 0.01f && vec2_len(vec2_sub(f, gr->anchor)) - stop <= gr->reach + FF_FIGHT_LEASH) {   // where it would stand is within the leash
            vec2 dirf = vec2_scale(vec2_sub(f, here), 1.0f / d);
            float go = d - stop;
            e->close = go > 0 ? 1 : 2;
            if (go > FF_AHEAD || go <= 0) go = FF_AHEAD;
            aim[0] = here.x + dirf.x * go; aim[1] = here.y + dirf.y * go; aim[2] = pos[2];
            e->step = step;
            e->lag_at = 0;
            g_ff_closing++;
            return 1;
        }
    }
    float across_now = 0;
    e->prog = ff_progress(gr, anchor, v2(pos[0], pos[1]), &across_now);
    e->prog_at = step ? step : 1;
    e->merge = 0;
    if (gr->drain) vel = gr->speed;   // the anchor stands at its end, the column's places move on (gr->conv)
    if (gr->drain) {   // the column drains (ff_leader): past the narrow a unit is let go to its spot on its own route, the ones behind queue up behind it still
        if (!e->rel && (e->prog > gr->narrow_to + FF_REL_PAST || e->prog > gr->front - 1.0f || (!e->ahead && ff_since(gr->rel_at) >= FF_DRAIN_STALL))) { e->rel = 1; gr->rel_at = step ? step : 1; g_ff_released++; ff_reroute(u); }   // or at the column's front: no further to go (the goal inside the narrow); or the front rank of a drain that let nobody go for FF_DRAIN_STALL: the goal inside an enclosure, the line narrow to its end, the front rank standing short of a place in the far wall (the player, 2026-11-03: the last units waited till the others had reached their spots)
        if (e->rel) return 0;
    }
    vec2 off = ff_off(gr, e);
    if (gr->cols) off.y += gr->conv;   // draining: the places have moved on this far (ff_leader)
    vec2 pt = ff_point(gr, anchor, way, face, off, 1, &dir);
    ee_map *map = game_map();
    ee_unit_type *td = unit_get_type_data(u);
    int size = map ? (int)map_get_size(map) : 0;
    // its point on ground it may not enter (a formation wider than the channel it passes): drawn in across towards the
    // formation's line, its rank kept - the formation narrows there (the outer ships went their own way, away from it)
    if (td && size > 0 && sr_alloc((uint32_t)(size * size))) {
        st_stamp_step();
        for (int k = 3; k >= 0 && !sr_pass(map, u, td, size, (int)floorf(pt.x), (int)floorf(pt.y)); k--)
            pt = ff_point(gr, anchor, way, face, off, (float)k * 0.25f, &dir);
    }
    vec2 d = vec2_sub(pt, v2(pos[0], pos[1]));
    float w = unit_max_turn(u), ahead = gr->naval && w > 0 ? 2 * unit_max_speed(u) / w : 0;   // a ship: two turning radii ahead (it closes on its place in a curve)
    if (ahead < FF_AHEAD) ahead = FF_AHEAD;
    // the point steered at: on ground it may enter, in a clear line - a ship's, two turning radii ahead, nearer its point
    // till it is (in a channel it lay on the shore: 2598 ticks of 16 ships on their own routes, two ships the whole
    // second leg - the player: some ships decide they aren't part of the group)
    // A unit ahead of its point steers on from abreast of itself: it faces the way and slows for its point (ff_speed:
    // standing when the point is FF_AHEAD / FF_GAIN behind it) instead of walking back to it - a ship with its point
    // behind it went round in a full circle; a land unit walked back through the formation (the player, 2026-11-02: a
    // wedge clicked behind itself). Before, the march from rest was set ahead by the largest such lag so that no land
    // unit's point lay behind it: the save's wedge marching 41 degrees off its facing had its anchor 7 tiles ahead
    // of its middle, and the rear ran at its top speed for 35-50 s closing while the slowest could never (they
    // have no speed to spare) - strung out most of the way, and the slow set out late; the game's own move keeps
    // every unit at the group's speed from the first step (the player: "still feels weird"). A ship beside its point
    // steers at least twice that far ahead (it merges at a shallow angle: it turned 90 degrees to its point and back -
    // the player: some turn the wrong way at the start, some do a full 360 with the target in front of them)
    // Ahead of its point only its
    // distance across counts as far: by the whole distance the column's waiting rear, its points 30 tiles back along
    // the trail, went on its own routes (FF_FAR: 10400 ticks of the siphon). Waiting where it stands instead of abreast
    // (tried): it no longer sidestepped into its lane while it waited, and form_turn's drag took 33.8 s (24.5)
    // (fd: the way at its point - the trail's heading for a point on the trail, else the way the formation moves: a
    // formation moved as it stands moves across its facing, and along the facing a unit beside its point counted as
    // ahead of it, form_turn's drag 35.8 s instead of 24.5)
    vec2 fd = !gr->naval && gr->in_place != 2 && off.y < 0 ? dir : way;
    vec2 base = pt, rel = vec2_sub(v2(pos[0], pos[1]), pt);
    float lon = vec2_dot(rel, fd), away = vec2_len(d);
    if (lon > 0) {
        base = vec2_add(pt, vec2_scale(fd, lon));
        away = vec2_len(vec2_sub(rel, vec2_scale(fd, lon)));
    }
    // THE MERGE: a column's unit ahead of its place (the wide formation's body before a gap: its place in the column
    // lies far back along the line) walks on along the line where it is, drawn in towards its file as its place
    // comes nearer (FF_FUNNEL tiles aside for each tile ahead of its place: the body before the mouth a funnel), at
    // the speed that brings it to the mouth as its place gets there (ff_speed) - nobody stands, nobody overtakes
    // A column's unit far behind its place (held up: its place gone on round a bend, out of its clear line - it took
    // its own route, out of the column's order) walks along the line in its file the same way, at its catch-up speed
    float lead = e->prog - off.y, merge_d = 0, merge_aside = 0;
    if (gr->cols && !gr->naval && gr->in_place != 2 && (lead > FF_MERGE_MIN || lead < -FF_LAG_LINE)) {
        float lat = across_now - off.x, most = lead > 0 ? (lead - FF_MERGE_MIN) * FF_FUNNEL : 0, t = e->prog;
        merge_aside = fabsf(lat);
        lat = off.x + (lat > most ? most : lat < -most ? -most : lat);
        // and into the mouth's opening as it nears the mouth (a tile aside of it for each tile before it), not past
        // the mouth while it stands outside the opening: walking on along the line where it was, the body's wing went
        // on beside the channel outside its wall (the player, 2026-11-10: "units still get siphoned off" - 12 of 71)
        float adv = vec2_len(vec2_sub(gr->anchor, gr->line_at));   // the line was measured this far back
        int mi = lead > 0 ? ff_mouth_ahead(gr, e->prog + adv - 0.5f) : -1;
        if (mi >= 0) {
            float half = unit_get_size(u) * 0.5f + FF_COL_MARGIN, lo = half - gr->line_l[mi], hi = gr->line_r[mi] - half;
            if (lo > hi) lo = hi = (lo + hi) * 0.5f;
            merge_d = gr->line_t[mi] - adv - e->prog;
            float slack = merge_d > FF_MOUTH_IN ? merge_d - FF_MOUTH_IN : 0;
            lat = lat < lo - slack ? lo - slack : lat > hi + slack ? hi + slack : lat;
            if ((across_now < lo - 0.25f || across_now > hi + 0.25f) && t > gr->line_t[mi] - adv - FF_MOUTH_IN - FF_AHEAD) t = gr->line_t[mi] - adv - FF_MOUTH_IN - FF_AHEAD;
        }
        // within the room where it is (the line's point nearest its progress, as last measured): a unit fallen behind
        // a front rank's place - the head spread out past the gap, 5 tiles aside - steered at the wall beside it and
        // stood in the channel's mouth till the drain gave up (the way back north, 2026-11-11)
        {
            int ni = -1;
            for (int i = 0; i < gr->line_n; i++) if (gr->line_room[i] >= 0 && (ni < 0 || fabsf(gr->line_t[i] - adv - t) < fabsf(gr->line_t[ni] - adv - t))) ni = i;
            if (ni >= 0 && fabsf(gr->line_t[ni] - adv - t) < 2.0f) {
                float half = unit_get_size(u) * 0.5f + FF_COL_MARGIN, lo = half - gr->line_l[ni], hi = gr->line_r[ni] - half;
                if (lo > hi) lo = hi = (lo + hi) * 0.5f;
                lat = lat < lo ? lo : lat > hi ? hi : lat;
            }
        }
        vec2 ld = way, lp = t < 0 ? ff_trail_at(gr, anchor, way, -t, &ld) : vec2_add(anchor, vec2_scale(way, t));
        base = vec2_add(lp, vec2_scale(v2(ld.y, -ld.x), lat));
        fd = ld;
        away = fabsf(across_now - lat);
        if (lead > 0) {
            if (mi < 0) merge_d = 1e9f;
            e->merge = 1;
        }
    }
    if (gr->naval) {
        float lat = fabsf(vec2_dot(rel, v2(fd.y, -fd.x)));
        if (ahead < 2 * lat) ahead = 2 * lat < FF_FAR ? 2 * lat : FF_FAR;
    }
    int ok = 0, why = 1;
    vec2 at = vec2_add(base, vec2_scale(fd, ahead));
    if (away <= FF_FAR && td && size > 0 && sr_alloc((uint32_t)(size * size))) {
        st_stamp_step();
        for (float ah = ahead; !ok; ah *= 0.5f) {
            if (ah < FF_AHEAD) ah = FF_AHEAD;
            vec2 c = vec2_add(base, vec2_scale(fd, ah));
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
    if (g_ff_watch && g_log && gr->cols) fprintf(g_log, "# ffcol %u %06x prog %.2f off %.2f %.2f lead %.2f merge %d d %.1f pt %.1f,%.1f at %.1f,%.1f ok %d conv %.1f anchor %.1f,%.1f%c", game_time(), (unsigned)unit_get_serial(u) & 0xFFFFFF, e->prog, off.x, off.y, lead, e->merge, merge_d, pt.x, pt.y, at.x, at.y, ok, gr->conv, anchor.x, anchor.y, 10);
    vec2 to = vec2_sub(at, v2(pos[0], pos[1]));
    float tl = vec2_len(to);
    vec2 a = tl > 0.01f ? vec2_scale(to, 1.0f / tl) : way;
    e->along = vec2_dot(d, a);
    e->off_by = vec2_len(d);
    // its point's speed its way: the anchor's, and in a turn its offset swung round with the facing (the outer ones of a
    // wheeling fleet go faster, the inner slower - else the outer ones fell behind in every arc)
    float spin = gr->dt > 0 ? gr->face_rot.y / gr->dt : 0;
    vec2 r = vec2_sub(pt, anchor), vp = vec2_add(vec2_scale(way, vel), vec2_scale(v2(-r.y, r.x), spin));
    if (!gr->naval && gr->in_place != 2 && off.y < 0) vp = vec2_scale(dir, vel);   // on the trail: along it
    e->with = vec2_dot(vp, a);
    if (e->merge) {   // to the mouth as its place gets there; past it (or none ahead), slower the further ahead of its place it is
        // (its way aside counts half: it walks in at an angle; and no slower than a unit in the narrow that far ahead
        // of its place - at the mouth it eased in instead of standing for its place and lurching after it)
        float over = lead - FF_MERGE_MIN, in = 1 - over / FF_MERGE_EASE;
        float share = merge_d < 1e8f ? (merge_d > 0 ? (merge_d + 0.5f * merge_aside) / (merge_d + over) : 0) : 1 - lead / FF_CREEP;
        if (merge_d < 1e8f && in > share) share = in;
        e->with = vel * (share > 1 ? 1 : share > 0 ? share : 0);
    }
    // fallen behind (no catch-up share left, its point ahead) and asking for its top speed (ff_speed) it counts for the
    // pace with the speed it actually goes - in a jam at a gap the formation waits at the jam's speed (the player: its limited throughput, and then they don't run
    // spread thin into enemy territory behind the choke, is the point); at its top speed it held the formation at 80% of
    // that while the jam fell further behind. Held up - so, or its point out of reach - and moving less than FF_OUT_MOVE
    // in FF_OUT_S, it is stuck: it drops out of the march (the formation no longer waits for it) and takes a route of its
    // own from where it stands (the player: a unit can't hold the formation back - it drops out and follows the path
    // alone, from its position to its next waypoint)
    float sz = unit_get_size(u), place = sz < FO_MIN_SIZE ? FO_MIN_SIZE : sz;
    int behind = e->along > 0 && e->off_by > (float)g_set.formation_catchup_falloff * place;
    vec2 here = v2(pos[0], pos[1]);
    if ((!ok || behind) && !gr->fight) {   // not while the formation holds for a fight: standing in it is no jam (22 of 108 dropped out so)
        if (!e->lag_at) { e->lag_at = step ? step : 1; e->lag_pos = here; }
        else if (ff_since(e->lag_at) >= FF_OUT_S) {
            if (vec2_dist2(here, e->lag_pos) < FF_OUT_MOVE * FF_OUT_MOVE) { ff_drop_out(e, u); return 0; }
            e->lag_at = step ? step : 1;
            e->lag_pos = here;
        }
    } else e->lag_at = 0;
    float now = unit_max_speed(u) * (1 + ff_boost(e, u, step));   // its speed now
    // THE PACE (formation_wait): a straggler - fallen behind the formation's rear line beyond its catch-up,
    // formation_catchup_falloff x its size - sets it with its speed now (its top speed: no catch-up left) until it is
    // within its catch-up of its point again, and the formation goes at most formation_wait of that; a member within the
    // formation's body sets none - it closes on its own, and the formation stops for nobody then, whatever
    // formation_wait (the player, 2026-11-02: formation_wait=0 to test "stand until the stragglers have caught up" - the
    // formation never set out, every member's speed now times 0; counting any member out of its place it stood for
    // ever with them a tile beside their points, counting those behind their points beyond the catch-up it stood from
    // rest with the back of a wedge sent behind itself behind its places by the formation's depth: the closing rule
    // gives a unit speed for the distance behind its point only, and only the moving formation untangles a crowd).
    // THE JAM: a member behind its point beyond its catch-up, asking its top speed and going slower (held up: one
    // slowed in a turn or by a push stopped the formation, and the others' speed, the formation's, with it - no less
    // than FF_JAM of its top speed) sets the formation's speed to what it goes - not scaled by formation_wait, and a
    // straggler held up sets no pace meanwhile: in a jam at a gap the formation waits at the jam's speed (the player:
    // its limited throughput, and then they don't run spread thin into enemy territory behind the choke, is the
    // point), and one held up by the formation itself (riders from rest behind the standing citizens, their places in
    // front) can't catch up until it moves
    float top = unit_max_speed(u);
    // Held up FF_JAM_S first: one of a hundred bumped in the crowd for a moment (the march of the save "slow", straight
    // over open ground: a unit or two under 90% of its top speed at most leader steps) braked the whole formation to
    // FF_JAM of its top speed for a leader step, 2.5 s, every other step - stop-and-go (the player, 2026-11-02: the
    // formation doesn't feel smooth)
    int jammed = 0;
    if (behind && e->want >= top * 0.99f) {
        float v = unit_get_speed(u), lo = FF_JAM * top;
        if (v < top * 0.9f) { if (!e->jam_at) e->jam_at = step ? step : 1; jammed = ff_since(e->jam_at) >= FF_JAM_S; }
        else e->jam_at = 0;
        if (jammed) { if (v < lo) v = lo; if (v < now) now = v; }
        if (gr->jam_next < 0 || now < gr->jam_next) gr->jam_next = now;
    } else e->jam_at = 0;
    float fallen = (float)g_set.formation_catchup_falloff * place;
    int straggling = e->along - (e->off.y - gr->rear) > fallen;   // behind the rear line beyond its catch-up
    if (e->off_by < fallen) e->waited = 0;   // within its catch-up of its point: the slate clean
    else if (straggling) {
        if (!e->waited) { e->waited = 1; e->wait_at = step ? step : 1; e->wait_along = e->along; }
        else if (e->waited > 0 && e->along < e->wait_along - FF_WAIT_GAIN) { e->wait_at = step ? step : 1; e->wait_along = e->along; }   // gaining on its point
        if (e->waited > 0 && ff_since(e->wait_at) >= FF_OUT_S) { e->waited = -1; g_ff_wait_gave_up++; }   // not gaining in FF_OUT_S (jostled in the crowd, blocked): given up till it is near its point - the formation goes on, and the moving formation untangles it (the clock runs on through a swing beside or ahead of its point: a jostled unit flipped in and out of straggling every few seconds, and a fresh clock each time never ran out)
    }
    if (e->waited > 0 && straggling && !jammed && (gr->pace_next < 0 || now < gr->pace_next)) gr->pace_next = now;
    if (!ok && g_ff_watch && g_log) fprintf(g_log, "# ffaim %u %06x own why %d at %.1f,%.1f pt %.1f,%.1f\n", game_time(), (unsigned)unit_get_serial(u) & 0xFFFFFF, why, at.x, at.y, pt.x, pt.y);
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

// STAND OR WALK (the player, 2026-11-10: "the formation can slow down to silly looking speeds, the result is soldiers
// walking on the same spot, or gliding slowly towards the chokepoint" - their rule: slower than a quarter of its
// speed, and its target not yet twice its size away, it stops; and standing it shows its standing animation): a
// merging unit whose speed (want) would be under FF_STAND_AT of its top speed stands - at rest in the game's idle
// animation (7; a move's walk, 0xD, runs on whatever the speed) - till the way it would have crept adds up to
// FF_STAND_STEP times its size, then walks that off at FF_WALK_MIN of its top speed or more, and stands again. Only
// the two animations are changed, and only for a unit this held (halt 2: its walk put back when it goes on, leaves
// the merge or the march)
#define FF_STAND_AT 0.25f
#define FF_STAND_STEP 2.0f
#define FF_WALK_MIN 0.5f
#define FF_ANIM_IDLE 7
#define FF_ANIM_IDLE2 8        // the game's other idle animation: a unit standing in 7 goes on to it by itself
#define FF_ANIM_WALK 0xD
internal unsigned g_ff_stands;   // times a merging unit was stood
internal void ff_walk_on(struct ff_unit *e, ee_unit *u) {
    if (e->halt == 2 && (unit_get_anim(u) == FF_ANIM_IDLE || unit_get_anim(u) == FF_ANIM_IDLE2) && unit_orders(u)) unit_start_anim(u, FF_ANIM_WALK, -1, 0);
    e->halt = 0;
    e->debt = 0;
}
internal float ff_stand(struct ff_unit *e, ee_unit *u, float want, float top, float dt) {
    float sz = unit_get_size(u), step_len = FF_STAND_STEP * (sz < FO_MIN_SIZE ? FO_MIN_SIZE : sz), v = unit_get_speed(u);
    if (!e->halt) {
        if (want >= FF_STAND_AT * top && e->debt <= 0) return want;
        if (e->debt > 0) {   // walking a stand's way off
            e->debt += (want - v) * dt;
            return want > FF_WALK_MIN * top ? want : FF_WALK_MIN * top;
        }
        e->halt = 1;
        g_ff_stands++;
    }
    e->debt += want * dt;
    if (e->debt >= step_len || want >= FF_WALK_MIN * top) {   // its place far enough on, or the column flowing again
        ff_walk_on(e, u);
        e->debt = step_len < 0.01f ? 0.01f : e->debt > 0.01f ? e->debt : 0.01f;
        return want > FF_WALK_MIN * top ? want : FF_WALK_MIN * top;
    }
    if (e->halt == 1 && v < 0.01f && unit_get_anim(u) == FF_ANIM_WALK) { unit_start_anim(u, FF_ANIM_IDLE, -1, 0); e->halt = 2; }
    return 0;
}

// the land models' speed step: the leader at the march's speed (the slowest member's top speed), less while a member
// lags (formation_wait); a follower that steered this step at its point's speed (the leader's) its way plus
// FF_GAIN for each tile behind it - while behind up to formation_catchup (or formation_reform) of its own top speed (the slowest ones could never catch up at their top speed,
// and the formation as a whole still goes no faster than its slowest unit) - towards it by its own acceleration a tick;
// the game's own step while it turns more than 45 degrees
// (ships: shiproute.c's speed step calls it with the game's step s)
internal float ff_speed_step(ee_unit *u, float s, float angle) {
    struct ff_unit *e = g_ff_on && u ? ff_find((uint32_t)unit_get_serial(u), 0) : NULL;
    if (!e) return s;
    uint32_t step = ff_step();
    // turning more than 45 degrees to the game's own route point: the game's step - unless the unit is steered at its
    // point by the follow (this tick or the last): the game's angle is to its own waypoint then, behind a unit waiting
    // abreast for its point, and the game's step turned it round and walked it back, ours turned it forward again - a
    // twitch every second while it waited (the save's 27-tile march: a unit 0 -> 0.34 -> 0.60 -> 0.26 -> 0, five times)
    if (fabsf(angle) > 0.7853982f && (e->step == ~0u || step - e->step > 1)) { if (g_ff_watch && g_log) fprintf(g_log, "# ffspd %u %06x game s %.3f ang %.2f steered %d own %d\n", game_time(), (unsigned)unit_get_serial(u) & 0xFFFFFF, s, angle, e->step == ~0u ? -1 : (int)(ff_step() - e->step), e->own); ff_walk_on(e, u); return s; }
    ee_unit *l;
    struct ff_group *gr = ff_march(e, u, step, &l);
    if (!gr) { if (g_ff_watch && g_log) fprintf(g_log, "# ffspd %u %06x nomarch s %.3f\n", game_time(), (unsigned)unit_get_serial(u) & 0xFFFFFF, s); ff_walk_on(e, u); return s; }
    float top = unit_max_speed(u), want, cap = top;
    if ((e->halt || e->debt > 0) && step - e->step > 1) ff_walk_on(e, u);   // no longer steered (its own route, let go): it walks
    if (e->step == ~0u || step - e->step > FF_KEEP) { if (g_ff_watch && g_log) fprintf(g_log, "# ffspd %u %06x own s %.3f since %u\n", game_time(), (unsigned)unit_get_serial(u) & 0xFFFFFF, s, e->step == ~0u ? 0 : step - e->step); ff_walk_on(e, u); return s; }   // steered at its point lately (a ship isn't every tick)
    want = e->with + FF_GAIN * e->along;   // its point's speed its way, and closing
    if (e->merge) want = e->with;   // a column's unit ahead of its place: the merge's speed (fo_follow_aim)
    if (e->close) want = e->close == 1 ? top : 0;   // closing in on the fight: at its top speed, standing once in its range
    if (e->along > 0) cap = top * (1 + ff_boost(e, u, step));   // near its place only, or in the re-form's grace
    if (want > cap) want = cap;
    if (want < 0) want = 0;
    ee_physics *ph = game_physics();
    if (!gr->naval && gr->started && !gr->fight) want = ff_stand(e, u, want, top, ph ? physics_get_step_s(ph) : 0.05f);   // any steered land unit: a formation slowed to a crawl (a jam ahead, a draining column) goes on in steps too
    else if (e->halt || e->debt > 0) ff_walk_on(e, u);
    e->want = want;
    // by its own acceleration: capped (0.5 tiles/s/s against a halberdier's 1.3) the units arrived late - one stopped
    // by a push near its spot got going too slowly (form_place's last at 104 s, 64 uncapped; form_back 82, 68); and
    // the march slowed into its goal (over the last 2-4 tiles, to 40-50%) doubled the pushes at rest and left a unit
    // shuffling for minutes - the units' own stop at their spots, as the game's, is the clean one (2026-11-02)
    float v = unit_get_speed(u), acc = ph ? physics_get_step_s(ph) * unit_accel(u) : 0.01f;
    if (g_ff_watch && g_log) fprintf(g_log, "# ffspd %u %06x ff s %.3f ang %.2f want %.3f v %.3f along %.2f with %.3f acc %.3f\n", game_time(), (unsigned)unit_get_serial(u) & 0xFFFFFF, s, angle, want, v, e->along, e->with, acc);
    return v < want ? (v + acc < want ? v + acc : want) : (v - acc > want ? v - acc : want);
}
// (and no land unit of a march goes on in a standing animation: one stood by ff_stand whose entry was given to a new
// march, or that stood on in the game's second idle animation, glided - 4% of the moving steps of the player's centre
// group; without ff_stand none moves so)
internal float ff_speed(ee_unit *u, float s, float angle) {
    float r = ff_speed_step(u, s, angle);
    struct ff_unit *e = g_ff_on && u && r > 0.02f ? ff_find((uint32_t)unit_get_serial(u), 0) : NULL;
    if (e && e->g >= 0 && !g_ff[e->g].naval && (unit_get_anim(u) == FF_ANIM_IDLE || unit_get_anim(u) == FF_ANIM_IDLE2)) {
        ee_goal *g = unit_get_goal(u);
        if (g && goal_is_location(g)) unit_start_anim(u, FF_ANIM_WALK, -1, 0);
    }
    return r;
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
    // following, with the final facing so far off the way it goes that the turn at the goal would move the outermost
    // unit further than formation_wheel (ff_wheels, as at a waypoint): the march
    // forms for its way first - the same rows, the units given their places by where they stand along that way (a wedge
    // sent behind itself facing back: before, each unit crossed to the far side of the formation to walk with its final
    // front first, and the formation swung round at the goal) - and each unit goes on to its final place at the goal
    vec2 go = vec2_sub(c, mean);
    float gl = vec2_len(go), spread = 0;   // spread: the formation's radius as laid out here (its furthest place from their middle)
    if (!circle) {   // the rows in the facing's frame, as a re-form in place lays them (the places are laid out again below)
        vec2 m = v2(0, 0);
        for (int i = 0; i < n; i++) { vec2 d = vec2_sub(g_fo[i].p, mean); g_fo[i].along = vec2_dot(d, fwd); g_fo[i].across = vec2_dot(d, right); }
        fo_rows(g_fo_order, n, kind, front);
        for (int i = 0; i < n; i++) { m = vec2_add(m, g_fo[i].at); g_fo_order[i] = i; }
        m = vec2_scale(m, 1.0f / (float)n);
        for (int i = 0; i < n; i++) { float d = vec2_len(vec2_sub(g_fo[i].at, m)); if (d > spread) spread = d; }
    }
    // A RE-FORM IN PLACE: the point within the formation's own diameter of the group's middle (the player, 2026-11-02: a
    // right-button drag where the formation stands, to turn it - laid out as a move to the point 2 tiles aside, the
    // march pointed the wedge along that way, and its anchor swung round on the formation's turning circle, 11 tiles:
    // 97 units walked 45 tiles each for 2.5 minutes and never formed): no march, each unit walks to its spot by its own
    // route and faces the drag's way there, as the game does without the mod. The diameter, not the radius: at the
    // radius (the drag on a wedge's tip) a march and the own routes were level, 12 tiles and further the march is the
    // game's own walk (tests/probes/form_turn.eeai). The formation's size as laid out here, not the group's spread where
    // it stands (a few stragglers 15 tiles ahead made a 35-tile move "in place") nor the game's own slots (its grid is
    // twice as wide). A circle never marches (below)
    int in_place = gl < 2 * spread;
    // the formation they stand in, moved as it is (ff_keep_layout) when the turn moves its outermost unit no further
    // than formation_wheel (ff_wheels, as a queued move goes on in the same formation) - turned further it is laid out
    // anew where it stands, each unit given its place by where it stands (the player, 2026-11-02: a wedge placed on the
    // one standing, facing back, "they try to rotate into it instead of reforming, which causes a mess": every unit
    // kept its offset and crossed the formation to the far side); with formation_keep_facing a click keeps its facing too, instead of the game's turn to
    // face the way to the point (a click in the game's view comes as a drag facing that way: one within FO_CLICK of it
    // is a click here; the player, 2026-11-02: the turn is the game's and intended, the keeping "worth saving as a
    // setting")
    int same = in_place && !g_fo_queued && !circle && g_ff_on ? ff_same_group(n) : -1;
    if (same >= 0 && g_ff[same].kind != kind) same = -1;   // another button: its layout (the player, 2026-11-03: "selecting the formation buttons no longer works, they are stuck in the one they loaded with" - the offsets kept were the old kind's)
    if (same >= 0 && g_set.formation_keep_facing && (facing == 0 || (cl > 0.5f && vec2_dot(fwd, vec2_scale(cur, 1.0f / cl)) > FO_CLICK))) {
        fwd = g_ff[same].fwd;
        right = v2(fwd.y, -fwd.x);
        float a = fo_atan2(-fwd.x, fwd.y);
        uint32_t bits;
        memcpy(&bits, &a, sizeof bits);
        for (int i = 0; i < n; i++) game_facing[i] = bits;
    }
    if (same >= 0 && !ff_wheels(spread, vec2_dot(fwd, g_ff[same].fwd))) same = -1;
    g_fo_has_march = 0;
    g_fo_kind = kind;
    g_fo_naval = 1;
    for (int i = 0; i < n; i++) {
        ee_unit_type *td = unit_get_type_data(g_fo[i].u);
        int mk = td ? model_kind(unit_type_get_model(td)) : 0;
        if (mk != MODEL_SHIPS && mk != MODEL_SUBS) g_fo_naval = 0;
    }
    // the way the march sets out: its route's first leg where the route doesn't start straight for the point (the
    // player, 2026-11-03, "siphon": laid out facing the point south, a group whose way out of its walled bowl ran east
    // turned on the spot first, "very slow to start moving") - the route as the planner finds it for the unit nearest
    // the group's middle (the leader, near enough), from where it stands (the leader's route starts there: from the
    // group's middle it ran 20 degrees off the leader's, and the formation swung onto the route over its first 10 s -
    // the player: "they still rotate after the first command"); a queued move sets out from the move before's end
    vec2 wf = gl > 0.5f ? vec2_scale(go, 1.0f / gl) : fwd;
    if (g_ff_on && !circle && !in_place && !g_fo_queued && !g_fo_naval && gl > 0.5f) {
        int nr = 0;
        for (int i = 1; i < n; i++) if (vec2_dist2(g_fo[i].p, mean) < vec2_dist2(g_fo[nr].p, mean)) nr = i;
        vec2 leg;
        if (sr_first_leg(g_fo[nr].u, g_fo[nr].p, c, spread + FF_LOOK, &leg)) wf = leg;
    }
    g_fo_way = wf;
    if (g_ff_on && !circle && !in_place && gl > 0.5f && !ff_wheels(spread, vec2_dot(wf, fwd))) {
        vec2 wr = v2(wf.y, -wf.x);
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
    if (g_ff_on && !circle && !in_place && gl > 0.5f && !g_fo_has_march) { pf = wf; pr = v2(pf.y, -pf.x); }
    for (int i = 0; i < n; i++) {
        vec2 d = vec2_sub(g_fo[i].p, mean);
        g_fo[i].along = vec2_dot(d, pf);
        g_fo[i].across = vec2_dot(d, pr);
        g_fo[i].p = vec2_sub(g_fo[i].p, c);   // the circles' angles: around the point
    }
    if (circle) fo_rings(g_fo_order, n); else fo_rows(g_fo_order, n, kind, front);
    if (same >= 0) ff_keep_layout(same, n);
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
    int arc = g_ff_on && g_fo_queued && !circle && !in_place ? ff_arc_layout(n, c, fwd, right) : -1;   // a gentle turn: the formation as it marches
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
    // a circle takes its places by the units' own routes, as the game's own (one ring round the point: 109 units a
    // ring of 19 tiles, marched as a formation 38 tiles wide it was narrowed for the terrain and still forming at 150 s)
    if (g_ff_on) { ff_start(group, n, c, fwd, right, g_fo_queued, arc); if (in_place || circle) ff_in_place(same >= 0); }
    // a line a move (the player's games: what a reported layout or march did)
    if (g_log) fprintf(g_log, "# formation: %d units, kind %d, %s%d from their last order%s, from %.1f,%.1f to %.1f,%.1f facing %.2f,%.2f (%s), %.1f tiles (the formation's radius %.1f), %s%d marches%s%s\n",
                       n, kind, g_fo_queued ? "queued, " : "", from_last, reface ? " (the click's facing turned to their way)" : "", mean.x, mean.y, c.x, c.y, fwd.x, fwd.y, facing == 0 ? "click" : "drag", gl, spread, same >= 0 ? "in place as it stands, " : in_place ? "in place, " : "", g_ff_on ? g_ff_count : 0, g_fo_forced ? "" : ", an attack-move", g_ff_on ? g_ff_no_march : "");
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

// the ripple timer's 1 / top speed (unit_inv_speed at the timer's call): its product with the unit's speed held at 1.
// A ship sends a ripple every max - (max - min) x speed / top ms (a frigate 4000 - 3700 x ...): above its top speed -
// the catch-up of formation_follow, up to formation_catchup or formation_reform - the time runs out at 108% and below 0 above, a ripple
// every frame, the stern a solid white plume (the player: the water effect); now as at its top speed. Only the picture
internal float __thiscall hook_ripple_speed(ee_unit *u) {
    float inv = unit_inv_speed(u), v = unit_get_speed(u);
    return v * inv > 1 ? 1 / v : inv;
}

internal void groupmove_install(void) {
    g_fo_on = g_set.formations;
    int n = g_fo_on ? patch_formation_slots(hook_formation_slots) : 0, goals = n ? patch_slot_goal(hook_slot_goal) : 0;
    int markers = g_set.goal_markers ? patch_order_marks_all() : 0;   // every unit's circle shows where it goes
    int queued = goals ? patch_path_msg_execute(hook_path_execute) : 0;
    int march = goals && g_set.formation_march ? patch_group_speeds(hook_group_speeds) : 0;
    g_ff_on = goals && g_set.formation_follow ? patch_land_speed_step(hook_ff_speed) == 2 : 0;   // the turn: shiproute.c's hook
    int ripples = g_ff_on ? patch_ripple_speed(hook_ripple_speed) : 0;
    if (g_log) fprintf(g_log, "# groupmove: formations=%d (%d formation vtables, goals hooked %d), goal markers for every unit: %d, queued moves laid out from the last order: %d, march at the slowest speed: %d, follow a leader: %d (ripples at most at the top speed's rate: %d)\n",
                       g_fo_on, n, goals, markers, queued, march, g_ff_on, ripples);
}
