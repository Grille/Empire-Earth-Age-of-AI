// TRANSPORTS STUCK IN THEIR SCRIPT (docs/engine-notes.md "Unit AI scripts (.tai)"). Included by eeaimod.c after
// landing.c (landing_has_unit) and expand.c (expand_ship).
// From the code two ways a transport hangs in its .tai script: in WaitForPassengers for ever while a pending passenger
// will never come (NoMoreLoadableUnitsToPickUp is only true when none is left; a passenger leaves the set by boarding,
// by CannotReachContainer, or by the goal's vf19 when it takes another order - not always, as the log showed), and in
// Idle with its unload goal and passengers aboard after RepathAroundObstacle false(Idle) (no Idle line in the
// transport scripts, and with a goal it isn't registered as idle). Seen in island games (2026-10-23): passengers
// standing Idle without any goal, or gone off on an attack, kept a transport waiting until it sank.
// Every transport on the map once a second - the game's own crossings, the landing's and the outpost ferries' alike: a
// stall is the same state on the same tile for 20 s while it has a goal or passengers aboard.
// transport_watchdog=1 ([all], default 0 until measured; [autotest] v_transport_watchdog for the variant players): a transport waiting 30 s in WaitForPassengers lets go of the
// pending passengers that aren't coming - dead, inside something else, without a goal, with another order, or idle
// with their board goal - through the goal's own vf19 (the game's step for a passenger with another order); one in
// Idle 20 s with its unload goal and passengers aboard is set to PrepareToMove (what every order does), 3 times a stall.
// transport_log=1 ([autotest]): "# tw: P<n> at <m> min: <landing|ferry|game> transport <id> stalls in <state> <s> s at
// x,y goal <class> aboard <n>", for a load goal "pending <n> (board <coming>, idle <idle with its board goal>, no goal
// <n>, other <another order>, gone <dead or inside something>): <id> <state> <goal class> <its board goal is this
// transport's: 1> <tiles away>; ...", for an unload goal "unload x,y <tiles>"; "still ..." every 2 minutes, then
// "... moves on after <s> s: <state> -> <state>" (or "gone after" when it sank). The watchdog: "# tw: ... watchdog:
// ..." for each step. <id>: the unit object's address (the same within a game).

internal int g_tw_log = 0;   // [autotest] transport_log
internal int g_tw_dog = 0;   // [all] transport_watchdog
internal int g_v_tw_dog = -1;   // head-to-head (autotest): [autotest] v_transport_watchdog for the variant_players (econ.c PV)

#define TW_N 128
#define TW_IDLE 5000u
#define TW_PREPARE_TO_MOVE 8000u
#define TW_WAIT_FOR_PASSENGERS 12010u
internal struct tw { ee_unit *u; int pi, kicks; unsigned st; ivec2 at; uint32_t since, said, seen; int stalled; } g_tw[TW_N];
internal int g_tw_n;
internal uint32_t g_tw_last;
internal unsigned g_tw_dropped, g_tw_kicked;   // the watchdog's steps this game

// the states a transport and its passengers pass through (the game's table 0x618E11: 162 names); others by number
internal const char *tw_state(unsigned id) {
    static char buf[4][12];
    static int k;
    switch (id) {
    case 0: return "none";
    case 2012: return "FaceEnemyUnit";
    case 2013: return "CheckRange";
    case 2015: return "LookForEnemies";
    case 2016: return "InitialAttackState";
    case 2021: return "WaitForReload";
    case 5000: return "Idle";
    case 5001: return "ReacquireGoal";
    case 6000: return "Dying";
    case 8000: return "PrepareToMove";
    case 8021: return "GetNextAutoWaypoint";
    case 9000: return "GetNextMoveWaypoint";
    case 9001: return "WaitForObstacleToClear";
    case 9002: return "RepathAroundObstacle";
    case 9006: return "TurnToUnitFacing";
    case 10003: return "Advance";
    case 11002: return "FindUnexploredArea";
    case 12000: return "LoadIntoContainer";
    case 12001: return "UnloadTransport";
    case 12002: return "MoveToRendezvous";
    case 12003: return "PickedUpLoadableUnits";
    case 12004: return "GetContainerProgress";
    case 12006: return "BumpExtraLoadableUnits";
    case 12010: return "WaitForPassengers";
    case 12011: return "WaitForTransport";
    case 12012: return "CannotReachContainer";
    }
    k = (k + 1) & 3;
    snprintf(buf[k], sizeof buf[k], "%u", id);
    return buf[k];
}
internal const char *tw_who(ee_unit *u) {
    ee_player *o = unit_get_owner(u);
    if (o && landing_has_unit(o, u)) return "landing";
    if (expand_ship(u)) return "ferry";
    return "game";
}
internal unsigned tw_id(ee_unit *u) { return (unsigned)(uintptr_t)u & 0xFFFFFF; }
internal float tw_dist(ee_unit *a, ee_unit *b) {
    float dx = (float)(unit_get_x(a) - unit_get_x(b)), dy = (float)(unit_get_y(a) - unit_get_y(b));
    return sqrtf(dx * dx + dy * dy);
}
// the goal's class without "EEUG" and "@@"
internal int tw_goal_len(const char *n) { return (int)strcspn(n, "@"); }
internal const char *tw_goal_name(ee_goal *g) {
    const char *n = rtti_name(g);
    return strncmp(n, "EEUG", 4) ? n : n + 4;
}
// a pending passenger of tr's load goal: 0 coming (its board goal, not idle), 1 idle with its board goal, 2 no goal,
// 3 another order, 4 gone (dead, or inside something - off the map)
enum { TWP_BOARD, TWP_IDLE, TWP_NOGOAL, TWP_OTHER, TWP_GONE };
internal int tw_passenger(ee_unit *tr, ee_unit *p) {
    if (!p || unit_get_hp(p) <= 0 || !unit_get_type_data(p) || unit_get_x(p) < 0) return TWP_GONE;
    ee_goal *g = unit_get_goal(p);
    if (!g) return TWP_NOGOAL;
    if (!goal_is_load(g) || goal_get_holder(g) != tr) return TWP_OTHER;
    return unit_ai_state_id(p) == TW_IDLE ? TWP_IDLE : TWP_BOARD;
}
internal void tw_head(struct tw *t, ee_unit *u, uint32_t now, const char *what) {
    fprintf(g_log, "# tw: P%d at %.2f min: %s transport %06x %s", t->pi, now / 60000.0, tw_who(u), tw_id(u), what);
}
internal void tw_say(struct tw *t, ee_unit *u, uint32_t now, const char *what) {
    ee_goal *g = unit_get_goal(u);
    const char *gn = tw_goal_name(g);
    tw_head(t, u, now, what);
    fprintf(g_log, " in %s %.0f s at %d,%d goal %.*s aboard %d", tw_state(t->st), (now - t->since) / 1000.0, t->at.x, t->at.y, tw_goal_len(gn), gn,
            unit_count_inside(u));
    if (g && goal_is_load(g)) {
        ee_unit **b = goal_get_pending_begin(g), **e = goal_get_pending_end(g);
        int n = b && e > b ? (int)(e - b) : 0, c[5] = { 0 };
        for (int k = 0; k < n; k++) c[tw_passenger(u, b[k])]++;
        fprintf(g_log, " pending %d (board %d, idle %d, no goal %d, other %d, gone %d):", n, c[TWP_BOARD], c[TWP_IDLE], c[TWP_NOGOAL], c[TWP_OTHER], c[TWP_GONE]);
        for (int k = 0; k < n && k < 12; k++) {
            ee_unit *p = b[k];
            if (tw_passenger(u, p) == TWP_GONE) { fprintf(g_log, " %06x gone;", tw_id(p)); continue; }
            ee_goal *pg = unit_get_goal(p);
            const char *pn = tw_goal_name(pg);
            fprintf(g_log, " %06x %s %.*s %d %.0f;", tw_id(p), tw_state(unit_ai_state_id(p)), tw_goal_len(pn), pn,
                    pg && goal_is_load(pg) && goal_get_holder(pg) == u, tw_dist(p, u));
        }
    } else if (g && goal_is_unload(g)) {
        float dx = (float)(goal_get_unload_x(g) - unit_get_x(u)), dy = (float)(goal_get_unload_y(g) - unit_get_y(u));
        fprintf(g_log, " unload %d,%d %.0f", goal_get_unload_x(g), goal_get_unload_y(g), sqrtf(dx * dx + dy * dy));
    }
    fputs("\n", g_log);
}

// the watchdog's two steps (transport_watchdog), on a transport stalled in its state since t->since
internal void tw_dog(struct tw *t, ee_unit *u, uint32_t now) {
    ee_goal *g = unit_get_goal(u);
    if (!g) return;
    if (t->st == TW_WAIT_FOR_PASSENGERS && goal_is_load(g) && now - t->since >= 30000) {
        ee_unit **b = goal_get_pending_begin(g), **e = goal_get_pending_end(g);
        ee_unit *drop[64];
        int n = b && e > b ? (int)(e - b) : 0, nd = 0, c[5] = { 0 };
        for (int k = 0; k < n && nd < 64; k++) {   // a copy first: vf19 erases from the vector
            int w = tw_passenger(u, b[k]);
            c[w]++;
            if (w != TWP_BOARD) drop[nd++] = b[k];
        }
        if (!nd) return;
        for (int k = 0; k < nd; k++) goal_drop_passenger(g, drop[k]);
        b = goal_get_pending_begin(g); e = goal_get_pending_end(g);
        int left = b && e > b ? (int)(e - b) : 0;
        g_tw_dropped += (unsigned)(n - left);
        if (g_log && (g_tw_log || g_tw_dropped <= 20)) {
            tw_head(t, u, now, "watchdog:");
            fprintf(g_log, " after %.0f s in WaitForPassengers %d of %d pending let go (idle %d, no goal %d, other %d, gone %d), %d left, %d aboard\n",
                    (now - t->since) / 1000.0, n - left, n, c[TWP_IDLE], c[TWP_NOGOAL], c[TWP_OTHER], c[TWP_GONE], left, unit_count_inside(u));
        }
        return;
    }
    if (t->st == TW_IDLE && goal_is_unload(g) && unit_count_inside(u) > 0 && now - t->since >= 20000 && t->kicks < 3) {
        t->kicks++; g_tw_kicked++;
        if (g_log && (g_tw_log || g_tw_kicked <= 20)) {
            tw_head(t, u, now, "watchdog:");
            fprintf(g_log, " Idle %.0f s with its unload to %d,%d and %d aboard - PrepareToMove (%d)\n", (now - t->since) / 1000.0,
                    goal_get_unload_x(g), goal_get_unload_y(g), unit_count_inside(u), t->kicks);
        }
        unit_set_state(u, TW_PREPARE_TO_MOVE);
    }
}

struct tw_scan { uint32_t now; };
internal void tw_unit(ee_unit *u, void *ctx) {
    struct tw_scan *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    if (!td || !is_transport(unit_type_get_id(td)) || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) return;
    struct tw *t = NULL;
    for (int k = 0; k < g_tw_n && !t; k++) if (g_tw[k].u == u) t = &g_tw[k];
    unsigned st = unit_ai_state_id(u);
    ivec2 at = unit_get_tile(u);
    ee_player *o = unit_get_owner(u);
    if (!t) {
        if (g_tw_n >= TW_N) return;
        t = &g_tw[g_tw_n++];
        memset(t, 0, sizeof *t);
        t->u = u; t->st = st; t->at = at; t->since = s->now;
    }
    t->pi = o ? player_get_index(o) : -1;   // a transport converted by a priest: its new owner
    t->seen = s->now;
    if (st != t->st || at.x != t->at.x || at.y != t->at.y) {
        if (t->stalled && g_tw_log && g_log) {
            tw_head(t, u, s->now, "moves on");
            fprintf(g_log, " after %.0f s: %s -> %s\n", (s->now - t->since) / 1000.0, tw_state(t->st), tw_state(st));
        }
        t->st = st; t->at = at; t->since = s->now; t->stalled = 0; t->kicks = 0;
        return;
    }
    if (!unit_get_goal(u) && unit_count_inside(u) <= 0) { t->since = s->now; return; }   // idle and empty: not a stall
    if (g_tw_log && g_log) {
        if (!t->stalled && s->now - t->since >= 20000) { t->stalled = 1; t->said = s->now; tw_say(t, u, s->now, "stalls"); }
        else if (t->stalled && s->now - t->said >= 120000) { t->said = s->now; tw_say(t, u, s->now, "still"); }
    }
    if (o && PV(player_get_index(o), g_v_tw_dog, g_tw_dog) && !player_is_human(o)) tw_dog(t, u, s->now);   // the AIs' transports only: a human's are left to them
}
internal void tw_tick(ee_player *pl, uint32_t now) {
    (void)pl;
    if (!g_tw_log && !g_tw_dog && g_v_tw_dog <= 0) return;
    if (now < g_tw_last) tw_new_game();   // time went backwards: another game
    if (now - g_tw_last < 1000) return;
    g_tw_last = now;
    struct tw_scan s = { now };
    grid_each(tw_unit, &s);
    int n = 0;
    for (int k = 0; k < g_tw_n; k++) {   // not seen: sunk, or its object back in the unit pool
        struct tw *t = &g_tw[k];
        if (t->seen != now) {
            if (t->stalled && g_tw_log && g_log)
                fprintf(g_log, "# tw: P%d at %.2f min: transport %06x gone after %.0f s in %s\n", t->pi, now / 60000.0, tw_id(t->u), (now - t->since) / 1000.0,
                        tw_state(t->st));
            continue;
        }
        g_tw[n++] = *t;
    }
    g_tw_n = n;
}
// THE PLAYER'S LOAD AND UNLOAD COMMANDS ([general] transport_split, transport_unload_spread, default 1). The game's
// load command (EEMLoadTransport::Execute 0x4B9446: units right-clicked onto a transport) sends every selected unit to
// the clicked transport alone - 10 board, the rest are turned back at its side (BumpExtraLoadableUnits), the other
// selected transports get nothing to carry. Its unload (EEMUnloadTransport 0x4BF756) gives all selected transports one
// unload at the clicked tile - the first puts its units there, the next finds no room and ends its unload with its
// units aboard (the landing's finding, ld_run_in). Both run in the simulation on every PC (the selection is the
// game's, kept in step by the select messages), so the mod's split decides the same everywhere - in multiplayer only
// while every PC has the same settings, like the rest of the mod.
// The transports: every class that carries (the data's "unit type", TD_FAMILY) - the ships (18), the helicopters
// (17), on land the cargo truck, the staff car, the siege towers and the Trojan Horse (16), and the fortress (29: the
// same command, only the room part matters there); not the aircraft carriers (planes) nor the airport.
// Load: the units right-clicked onto a transport are shared out over it and the other selected transports - the
// transports nearest the units first, each takes its nearest units that it can carry (the game's own test, its
// type's vf60) while its room lasts (its room left +0x2A4, less what its load under way still awaits; a unit's size
// its type's +0x558); each gets a load goal of its own (one under way gets the units added, as the game does), with a
// meeting point of its own where the game's fails (a ship's, tw_load_split). Units that find no room keep what they
// did - with one transport too (the game sent them all, those beyond its room were turned back at its side).
// Unload: the game's command first (every unit's order as it gives them), then each loaded transport its own tile
// near the clicked one - on that land, free, 4 tiles from the others' (3 when there is little room), a ship's at the
// sea it sails - the nearest transport the nearest tile.
// Measured (2026-10-23, tests/probes/transport_split.eeai and two variants: Small Islands map seed 1, P1's island, 3 copper
// transports 2-4 tiles off a beach, 25 macemen inland, through the pipe's `transports`): aboard after 2.5 minutes 75 of
// 75 in 3 runs (the game's meeting points, transport_meeting=0: 60 - the transport 4 tiles off took none), off the
// east coast 90 of 90 (81); unloading the 25 at one tile of another island, all ashore 0.4 minutes after the order
// (one tile for all, transport_unload_spread=0: 0.6-0.8).
internal int g_tw_split = 1, g_tw_spread = 1, g_tw_meet = 1;   // g_tw_meet: [autotest] transport_meeting=0 keeps the game's meeting points
internal int g_tw_click_mark, g_tw_click_on = -1;   // this PC's last click circle: its effect, the unit's id (hook_click_mark)
internal ivec2 g_tw_click_at;
internal int ld_tile_free(ivec2 p);                       // landing.c
internal unsigned ocean_near(ivec2 p, int r, ivec2 *w);   // landing.c

enum { TW_SEA, TW_LAND, TW_AIR, TW_FORT };
internal int tw_kind(ee_unit *u) {   // a transport's: by its class (TW_SEA ...; a fortress too), -1 none
    ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
    int c = td ? unit_type_get_class_id(td) : 0;
    return c == CLS_NAVAL_TRANSPORT ? TW_SEA : c == CLS_LAND_TRANSPORT ? TW_LAND : c == CLS_HELICOPTER_TRANSPORT ? TW_AIR : c == CLS_FORTRESS ? TW_FORT : -1;
}
internal int tw_carrier(ee_player *pl, ee_unit *u) {   // pl's transport on the map
    return tw_kind(u) >= 0 && unit_get_owner(u) == pl && unit_get_hp(u) > 0 && unit_get_x(u) >= 0;
}
internal int tw_dist2(ee_unit *a, ee_unit *b) { return ivec2_dist2(unit_get_tile(a), unit_get_tile(b)); }
internal int tw_size(ee_unit *u) { int s = unit_type_get_transport_size(unit_get_type_data(u)); return s > 0 ? s : 1; }
// How far two transports' spots stay apart, by their size (the user: ships need half the old 4 tiles, trucks far
// less): the type's selection circle (TD_MARK_SIZE, tiles: copper transport 1.2, atomic 2.0, Chinook 1.0, Pegasus
// 1.3, cargo truck 0.7) - the two's mean rounded up, a tile less where there is little room, 1 at least (another
// tile); spots q and at[j] (sizes sz[j]) apart in x or y by that many.
internal float tw_space(ee_unit *t) { float r = unit_type_get_mark_size(unit_get_type_data(t)); return r > 0.0f && r < 8.0f ? r : 1.0f; }
internal int tw_apart(ivec2 q, float r, const ivec2 *at, const float *sz, int n, int tight) {
    for (int j = 0; j < n; j++) {
        int g = (int)ceilf((r + sz[j]) * 0.5f) - tight;
        if (g < 1) g = 1;
        if (abs(q.x - at[j].x) < g && abs(q.y - at[j].y) < g) return 0;
    }
    return 1;
}

// an unload tile near p for transport k of kind and size r: on land cont, free, apart from the others' at[0..k-1]
// (sizes sz), a ship's on the beach of its sea ocean; the nearest p within reach; 0 none
internal int tw_beach(ivec2 p, unsigned cont, int kind, unsigned ocean, float r, const ivec2 *at, const float *sz, int k, int reach, ivec2 *o) {
    for (int tight = 0; tight <= 1; tight++)
        for (int d = 0; d <= reach; d++)
            for (int dx = -d; dx <= d; dx++)
                for (int dy = -d; dy <= d; dy += (abs(dx) == d || !d ? 1 : 2 * d)) {
                    ivec2 q = iv2(p.x + dx, p.y + dy);
                    int ok = area_id(q, 2) == cont && !area_id(q, 1) && (kind != TW_SEA || ocean_near(q, 2, NULL) == ocean) && ld_tile_free(q) &&
                             tw_apart(q, r, at, sz, k, tight);
                    if (ok) { *o = q; return 1; }
                }
    return 0;
}

// nothing on tile q but pl's own soldiers and citizens (they move: the passengers themselves stand on the beach)
internal int tw_tile_clear(ee_player *pl, ivec2 q) {
    for (struct grid_it it = grid_it_box(q.x, q.y, 2, 1); grid_it_next(&it); ) {
        if (unit_get_x(it.u) != q.x || unit_get_y(it.u) != q.y) continue;
        if (unit_get_owner(it.u) == pl && (is_soldier(it.td) || is_citizen_type(unit_type_get_id(it.td)))) continue;
        return 0;
    }
    return 1;
}
// the tile of land cont beside p (p itself first), none: -1, -1
internal ivec2 tw_land_by(ivec2 p, unsigned cont) {
    for (int dx = 0; dx <= 2; dx++)
        for (int dy = 0; dy <= 2; dy++) {
            ivec2 q = iv2(p.x + (dx == 2 ? -1 : dx), p.y + (dy == 2 ? -1 : dy));
            if (area_id(q, 2) == cont && !area_id(q, 1)) return q;
        }
    return iv2(-1, -1);
}
// a meeting point for transport t and its units list[n]: the beach tile (on their land, clear, apart from the others'
// meet[nmeet], sizes msz) with a tile of t's sea beside it, the nearest the units and the transport - the units' walk
// weighing double; 0 none (or the transport more than 60 tiles off: the game's point stays)
internal int tw_meeting(ee_unit *t, ee_unit **list, int n, const ivec2 *meet, const float *msz, int nmeet, ivec2 *beach, ivec2 *water) {
    ee_player *pl = unit_get_owner(t);
    int cx = 0, cy = 0;
    for (int i = 0; i < n; i++) { cx += unit_get_x(list[i]); cy += unit_get_y(list[i]); }
    ivec2 c = iv2(cx / n, cy / n), s = unit_get_tile(t);
    unsigned cont = area_id(unit_get_tile(list[0]), 2), ocean = area_id(s, 1);
    int x0 = (c.x < s.x ? c.x : s.x) - 8, x1 = (c.x > s.x ? c.x : s.x) + 8, y0 = (c.y < s.y ? c.y : s.y) - 8, y1 = (c.y > s.y ? c.y : s.y) + 8;
    if (x1 - x0 > 60 || y1 - y0 > 60) return 0;   // the transport far off: the game's own
    for (int tight = 0; tight <= 1; tight++) {
        int best = -1;
        for (int x = x0; x <= x1; x++)
            for (int y = y0; y <= y1; y++) {
                ivec2 q = iv2(x, y), v, w = iv2(0, 0);
                if (area_id(q, 2) != cont || area_id(q, 1)) continue;
                int ok = tw_apart(q, tw_space(t), meet, msz, nmeet, tight), wd = -1;
                for (int k = 0; k < 8 && ok; k++) {   // the water beside it nearest the transport
                    static const signed char k_dx[8] = { 1, -1, 0, 0, 1, 1, -1, -1 }, k_dy[8] = { 0, 0, 1, -1, 1, -1, 1, -1 };
                    v = iv2(q.x + k_dx[k], q.y + k_dy[k]);
                    if (area_id(v, 1) == ocean && (wd < 0 || ivec2_dist2(v, s) < wd)) { w = v; wd = ivec2_dist2(v, s); }
                }
                if (!ok || wd < 0 || !tw_tile_clear(pl, q)) continue;
                int score = 4 * ivec2_dist2(q, c) + wd;   // the units' walk weighs double (squared: 4)
                if (best < 0 || score < best) { best = score; *beach = q; *water = w; }
            }
        if (best >= 0) return 1;
    }
    return 0;
}

// The slots (the units' groups of a load, the tiles of an unload) handed to the transports so that the way all sail
// together is the shortest: transport i takes slot slot[i]; two swap theirs while that shortens their two ways - two
// straight ways that cross are always longer than the two swapped (the triangle inequality), so none cross after.
// cost[i][s]: transport i's way to slot s (TW_NO: it can't take it); the tiles the ways got shorter by.
#define TW_NO 1e30f
internal float tw_uncross(int n, float cost[16][16], int *slot) {
    float saved = 0.0f;
    for (int round = 0, changed = 1; changed && round < 64; round++) {
        changed = 0;
        for (int i = 0; i < n; i++)
            for (int j = i + 1; j < n; j++) {
                float now = cost[i][slot[i]] + cost[j][slot[j]], swapped = cost[i][slot[j]] + cost[j][slot[i]];
                if (swapped < TW_NO && swapped < now - 0.01f) { int t = slot[i]; slot[i] = slot[j]; slot[j] = t; changed = 1; saved += now - swapped; }
            }
    }
    return saved;
}
internal float tw_way(ivec2 a, ivec2 b) { return sqrtf((float)ivec2_dist2(a, b)); }
// two transports may trade slots: the same kind, ships on the same sea
internal int tw_alike(ee_unit *a, ee_unit *b) {
    return tw_kind(a) == tw_kind(b) && (tw_kind(a) != TW_SEA || area_id(unit_get_tile(a), 1) == area_id(unit_get_tile(b), 1));
}

// the units us[nu] shared out over pl's transports tr[nt] (sorted here: nearest the units first): *used the transports
// given units; the units given. A ship's new load a meeting point of its own - the game's (EEUCNavalTransport's
// vf56) is the transport's own tile when it finds the units' region the same as its own (a copper
// transport 2 tiles off the beach: its 10 units walked to its tile, could not reach it and dropped out - 0 aboard, the
// same with one transport alone) and one beside the other's for units of one group (2 transports: 9 and 6 of 10 aboard)
internal int tw_load_split(ee_player *pl, ee_unit **tr, int nt, ee_unit **us, int nu, int *used) {
    int cx = 0, cy = 0;
    for (int i = 0; i < nu; i++) { cx += unit_get_x(us[i]); cy += unit_get_y(us[i]); }
    ivec2 c = iv2(cx / nu, cy / nu);
    for (int i = 1; i < nt; i++)   // ties: the selection's order
        for (int j = i; j > 0 && ivec2_dist2(unit_get_tile(tr[j]), c) < ivec2_dist2(unit_get_tile(tr[j - 1]), c); j--) { ee_unit *t = tr[j]; tr[j] = tr[j - 1]; tr[j - 1] = t; }
    static signed char owner[1024];   // the transport each unit goes to, -1 none
    memset(owner, -1, (size_t)nu);
    int m[16] = { 0 }, room0[16], busy[16], given = 0;
    for (int k = 0; k < nt; k++) {   // first who goes where
        ee_unit *t = tr[k];
        ee_unit_type *ttd = unit_get_type_data(t);
        ee_goal *g = unit_get_goal(t);
        int room = unit_get_room(t);
        busy[k] = g && goal_is_load(g) && goal_get_holder(g) == t;
        if (busy[k])   // a load under way: what it still awaits
            for (ee_unit **p = goal_get_pending_begin(g); p && p < goal_get_pending_end(g); p++)
                if (*p && unit_get_hp(*p) > 0) room -= tw_size(*p);
        room0[k] = room;
        while (room > 0) {   // its nearest unit that it can carry and that fits
            int best = -1, bd = 0;
            for (int i = 0; i < nu; i++) {
                if (owner[i] >= 0 || tw_size(us[i]) > room || !unit_type_can_carry(ttd, t, us[i])) continue;
                int d = tw_dist2(us[i], t);
                if (best < 0 || d < bd) { best = i; bd = d; }
            }
            if (best < 0) break;
            owner[best] = (signed char)k; room -= tw_size(us[best]); m[k]++;
        }
        if (g_log) fprintf(g_log, "# tw: P%d load: transport %06x type %d (%s, size %.2f) at %d,%d - room %d, %d given, %d aboard\n", player_get_index(pl),
                           tw_id(t), unit_type_get_id(ttd), rtti_name(ttd), (double)tw_space(t), unit_get_x(t), unit_get_y(t), unit_get_room(t), m[k], unit_count_inside(t));
    }
    // then which transport takes which group (slot s: the units transport s found): the shortest ways, none crossing -
    // a transport may take another's group it has room for and can carry; one with a load under way keeps its own
    int slot[16], size[16] = { 0 }, swapped = 0;
    ivec2 ctr[16];
    float cost[16][16];
    for (int s = 0; s < nt; s++) {
        int sx = 0, sy = 0;
        for (int i = 0; i < nu; i++) if (owner[i] == s) { sx += unit_get_x(us[i]); sy += unit_get_y(us[i]); size[s] += tw_size(us[i]); }
        ctr[s] = m[s] ? iv2(sx / m[s], sy / m[s]) : unit_get_tile(tr[s]);
        slot[s] = s;
    }
    for (int k = 0; k < nt; k++)
        for (int s = 0; s < nt; s++) {
            int ok = s == k || (!busy[k] && !busy[s] && tw_alike(tr[k], tr[s]) && size[s] <= room0[k]);
            for (int i = 0; i < nu && ok && s != k; i++) ok = owner[i] != s || unit_type_can_carry(unit_get_type_data(tr[k]), tr[k], us[i]);
            cost[k][s] = !ok ? TW_NO : m[s] ? tw_way(unit_get_tile(tr[k]), ctr[s]) : 0.0f;   // an empty group: it stays
        }
    float saved = tw_uncross(nt, cost, slot);
    for (int k = 0; k < nt; k++) swapped += slot[k] != k;
    static ee_unit *list[1024];
    ivec2 meet[16];
    float msz[16];
    int nmeet = 0, moved = 0;
    *used = 0;
    for (int k = 0; k < nt; k++) {   // then the goals
        if (!m[slot[k]]) continue;
        ee_unit *t = tr[k];
        int n = 0;
        for (int i = 0; i < nu; i++) if (owner[i] == slot[k]) list[n++] = us[i];
        ee_goal *g = unit_get_goal(t);
        if (g && goal_is_load(g) && goal_get_holder(g) == t) {   // under way: the units added, as the game does
            for (int i = 0; i < n; i++) goal_add_passenger(g, list[i]);
        } else {
            uint8_t *vec[4] = { 0, (uint8_t *)list, (uint8_t *)(list + n), (uint8_t *)(list + n) };
            void *mem = goal_load_alloc();
            if (!mem || !(g = goal_load_init(mem, t, vec))) continue;
            if (g_tw_meet && tw_kind(t) == TW_SEA) {   // a ship: the game's meeting point while its units' tile is at their beach and apart from the others', else the mod's
                ivec2 beach, water, at = tw_land_by(iv2(goal_get_meeting_x(g), goal_get_meeting_y(g)), area_id(unit_get_tile(list[0]), 2));
                int ok = at.x >= 0 && tw_apart(at, tw_space(t), meet, msz, nmeet, 0);
                int found = !ok && tw_meeting(t, list, n, meet, msz, nmeet, &beach, &water);
                if (g_log) {
                    fprintf(g_log, "# tw: P%d transport %06x at %d,%d: the game's meeting point %d,%d / %d,%d %s", player_get_index(pl), tw_id(t), unit_get_x(t),
                            unit_get_y(t), goal_get_meeting_ship_x(g), goal_get_meeting_ship_y(g), goal_get_meeting_x(g), goal_get_meeting_y(g),
                            ok ? "kept" : found ? "- the mod's" : "- none better found");
                    if (found) fprintf(g_log, " %d,%d / %d,%d", water.x, water.y, beach.x, beach.y);
                    fputs("\n", g_log);
                }
                if (found) {
                    goal_set_meeting(g, water.x, water.y, beach.x, beach.y);
                    at = beach; moved++;
                }
                msz[nmeet] = tw_space(t);
                meet[nmeet++] = at;
            }
            if (!unit_goal(t, g, 0, TW_PREPARE_TO_MOVE)) continue;   // refused: deleted, no parts from it
        }
        gfx_mark(GFX_ORDER_AT, unit_get_type_data(t), goal_get_meeting_ship_x(g), goal_get_meeting_ship_y(g), player_get_index(pl));   // where it goes, sized as the game's for the transport
        for (int i = 0; i < n; i++) given += unit_goal(list[i], goal_load_part(g), 0, TW_PREPARE_TO_MOVE);
        (*used)++;
    }
    if (g_log) fprintf(g_log, "# tw: P%d at %.2f min: load - %d units over %d of %d transports (%d meeting points moved apart, %d groups traded, the ways %.0f tiles shorter), %d without room\n",
                       player_get_index(pl), game_time() / 60000.0, given, *used, nt, moved, swapped, (double)saved, nu - given);
    return given;
}
// The mouse click's confirmation (this PC's, drawn at the click, before the command's message runs; GFX_CLICK,
// GFX_CLICK_ON): the last one, its handle, the unit clicked's id (-1: the ground) and the tile. The load split and the
// unload spread take it away - the yellow circles where each transport goes say it (this PC's player's command).
internal int __thiscall hook_click_mark(ee_effect_mgr *fx, void *on, float *pos, void *kind, int *handle, int arg) {
    if (handle) *handle = 0;   // (its result isn't a success flag: the handle says whether it was drawn)
    int r = effect_mgr_add(fx, on, pos, kind, handle, arg);
    g_tw_click_mark = handle && *handle && gfx_alive(*handle) ? *handle : 0;
    g_tw_click_on = g_tw_click_mark && on ? click_mark_on_id(on) : -1;
    g_tw_click_at = pos ? iv2((int)pos[0], (int)pos[1]) : iv2(-1, -1);
    return r;
}
internal void tw_click_mark_off(ee_player *pl, int on, ivec2 at) {
    if (!g_tw_click_mark || g_tw_click_on != on || player_get_index(pl) != (int)local_player_index()) return;
    if (on < 0 && (g_tw_click_at.x != at.x || g_tw_click_at.y != at.y)) return;
    gfx_remove(g_tw_click_mark);
    g_tw_click_mark = 0;
}
// The click's message (0x403ADE): with transports selected it takes the selection's first transport of one class
// (helicopters, else land transports, else ships, else a fortress); the one clicked is the transport (unit -1) when it
// is that one or of its type, else the message loads the clicked unit into that one - a transport of another type
// clicked (a Pegasus beside a Chinook, a ship beside a helicopter) became a transport to be loaded into another, and
// failed. Here the clicked transport is the one loaded onto, whatever its type.
internal void __thiscall hook_msg_load(ee_load_msg *msg) {
    ee_player *pl = g_tw_split ? game_player(load_msg_get_from(msg)) : NULL;
    ee_unit *clicked = pl ? unit_by_id(load_msg_get_transport(msg)) : NULL;
    if (pl && load_msg_get_unit(msg) != -1) {   // a transport clicked: that one; another unit (into the selected transport): the game's
        ee_unit *u = unit_by_id((unsigned)load_msg_get_unit(msg));
        clicked = tw_carrier(pl, u) && tw_carrier(pl, clicked) ? u : NULL;
    }
    void *grp = pl ? player_get_selected_group(pl) : NULL;
    if (!grp || !tw_carrier(pl, clicked)) { load_msg_execute(msg); return; }
    static ee_unit *us[1024];
    ee_unit *tr[16];
    int nt = 0, nu = 0, used;
    tr[nt++] = clicked;
    for (unsigned i = 0, n = group_count(grp); i < n; i++) {
        ee_unit *u = group_get(grp, i);
        if (!u || u == clicked) continue;
        if (tw_carrier(pl, u)) { if (nt < 16) tr[nt++] = u; continue; }
        if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || unit_get_holder(u) || nu >= 1024) continue;
        us[nu++] = u;
    }
    if (!nu) { load_msg_execute(msg); return; }
    if (tw_load_split(pl, tr, nt, us, nu, &used)) tw_click_mark_off(pl, (int)load_msg_get_transport(msg), iv2(-1, -1));
}

// pl's transports tr[nt] (with an unload goal each, units aboard) each to its own tile near p (sorted here: nearest p
// first), apart from the taken tiles at[0..taken-1] too (another group's of the same command); their tiles at[taken
// ..]; the transports sent elsewhere than p
internal int tw_unload_spread(ee_player *pl, ee_unit **tr, int nt, ivec2 p, ivec2 *at, float *sz, int taken) {
    unsigned cont = area_id(p, 2);
    if (!cont) return 0;   // not a tile on land: as the game gave it
    for (int i = 1; i < nt; i++)
        for (int j = i; j > 0 && ivec2_dist2(unit_get_tile(tr[j]), p) < ivec2_dist2(unit_get_tile(tr[j - 1]), p); j--) { ee_unit *t = tr[j]; tr[j] = tr[j - 1]; tr[j - 1] = t; }
    int spread = 0, slot[16], swapped = 0;
    ivec2 tile[16];
    float cost[16][16];
    for (int k = 0; k < nt; k++) {   // the tiles first (nearest p first)
        ivec2 *o = &at[taken + k];
        sz[taken + k] = tw_space(tr[k]);
        if (!tw_beach(p, cont, tw_kind(tr[k]), area_id(unit_get_tile(tr[k]), 1), sz[taken + k], at, sz, taken + k, 8 + 2 * (taken + nt), o)) *o = p;
        tile[k] = *o;
        slot[k] = k;
    }
    for (int k = 0; k < nt; k++)   // then who takes which: the shortest ways, none crossing (a ship's tile is at its sea)
        for (int s = 0; s < nt; s++) cost[k][s] = s == k || tw_alike(tr[k], tr[s]) ? tw_way(unit_get_tile(tr[k]), tile[s]) : TW_NO;
    float saved = tw_uncross(nt, cost, slot);
    for (int k = 0; k < nt; k++) {
        ivec2 o = tile[slot[k]];
        at[taken + k] = o;
        swapped += slot[k] != k;
        if (o.x == p.x && o.y == p.y) continue;   // its tile is the clicked one: the game's goal stays
        ee_goal *g = goal_unload(tr[k], o.x, o.y, 0);
        spread += g && unit_goal(tr[k], g, 0, TW_PREPARE_TO_MOVE);
    }
    if (g_log) fprintf(g_log, "# tw: P%d at %.2f min: unload at %d,%d - %d transports, %d sent to their own tile (%d tiles traded, the ways %.0f tiles shorter)\n", player_get_index(pl),
                       game_time() / 60000.0, p.x, p.y, nt, spread, swapped, (double)saved);
    return spread;
}
// The unload command: the Execute gives the selected group its unload goal at the clicked tile, the group hands it to
// its transports (0x5DB641) and draws a yellow circle at each one's goal (order_marks, data_ui_call_unload_marks) - the
// spread comes in between, so each circle is at its transport's own tile. The Execute makes a group of each kind of
// the selection (helicopters, trucks, ships ...: one hand-out each), so the tiles taken are kept for the command.
// Should no circles' call come, the spread follows the Execute (no circles then).
internal ee_unload_msg *g_tw_unload_msg;   // the unload command being executed
internal int g_tw_unload_groups;    // ... its groups spread
internal ivec2 g_tw_unload_at[64];  // ... the tiles taken
internal float g_tw_unload_sz[64];  // ... the sizes of their transports
internal int g_tw_unload_taken;
internal void tw_unload_group(ee_unload_msg *msg, void *grp) {
    ee_player *pl = game_player(unload_msg_get_from(msg));
    if (!pl || !grp) return;
    ee_unit *tr[16];
    ivec2 p = iv2(unload_msg_get_x(msg), unload_msg_get_y(msg));
    int nt = 0, taken = g_tw_unload_taken;
    for (unsigned i = 0, n = group_count(grp); i < n && nt < 16 && taken + nt < 64; i++) {
        ee_unit *t = group_get(grp, i);
        ee_goal *g = tw_carrier(pl, t) ? unit_get_goal(t) : NULL;
        if (g && goal_is_unload(g) && unit_count_inside(t) > 0) tr[nt++] = t;
    }
    g_tw_unload_groups++;
    if (!nt) return;
    g_tw_unload_taken += nt;
    if (nt == 1 && !taken) { g_tw_unload_at[0] = p; g_tw_unload_sz[0] = tw_space(tr[0]); return; }   // one transport (so far): the game's tile, kept for the next groups
    if (tw_unload_spread(pl, tr, nt, p, g_tw_unload_at, g_tw_unload_sz, taken)) tw_click_mark_off(pl, -1, p);   // the click's confirmation: the circles say it
}
internal void __cdecl hook_unload_marks(void *grp, void *on, int owner, int how, unsigned goal) {
    if (g_tw_unload_msg) tw_unload_group(g_tw_unload_msg, grp);
    order_marks(grp, on, owner, how, goal);
}
internal void __thiscall hook_msg_unload(ee_unload_msg *msg) {
    g_tw_unload_msg = msg;
    g_tw_unload_groups = g_tw_unload_taken = 0;
    unload_msg_execute(msg);
    g_tw_unload_msg = NULL;
    ee_player *pl = g_tw_unload_groups ? NULL : game_player(unload_msg_get_from(msg));
    if (pl) tw_unload_group(msg, player_get_selected_group(pl));
}

// pipe `transports load P x y [r]`: P's transports and land units within r tiles (default 15) of x, y shared out as the
// load command does (the transports and units of a selection); `transports unload P x y [r]`: P's loaded transports
// within r of x, y unload at x, y as the game's command gives it (one tile), then each its own; `transports id P`:
// each of P's transports, its +0x24 as an id found back (the id a load message names). `transports order load|unload
// P x y [r]`: the same units selected (the select command's own step) and the command's message executed as a
// right-click's is (the hooks, the game's Execute: the circles too) - load onto the transport nearest x, y.
internal void cmd_transports(char *out, const char *args) {
    char what[16] = "";
    int p = 0, x = 0, y = 0, r = 15, order = !strncmp(args, "order ", 6);
    if (order) args += 6;
    if (sscanf(args, "%15s %d %d %d %d", what, &p, &x, &y, &r) < 2) { REPLY("transports load|unload P x y [r] | id P\n"); return; }
    ee_player *pl = p > 0 && p < 16 ? game_players()[p] : NULL;
    ee_map *map = game_map();
    if (!pl || !map) { REPLY("no player %d or no map\n", p); return; }
    static ee_unit *us[1024];
    ee_unit *tr[16];
    ivec2 at[16], c = iv2(x, y);
    float asz[16];
    int nt = 0, nu = 0, used = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
        int close = ivec2_dist2(unit_get_tile(u), c) <= r * r;
        if (!strcmp(what, "id") && tw_carrier(pl, u)) {
            ee_unit *back = unit_by_id((unsigned)unit_get_serial(u));
            const char *gn = tw_goal_name(unit_get_goal(u));
            REPLY("transport %06x (%s) at %d,%d id %08x -> %06x%s, aboard %d, room %d, %s, goal %.*s\n", tw_id(u), rtti_name(unit_get_type_data(u)), unit_get_x(u), unit_get_y(u),
                  (unsigned)unit_get_serial(u), tw_id(back), back == u ? " (the same)" : "", unit_count_inside(u), unit_get_room(u), tw_state(unit_ai_state_id(u)),
                  tw_goal_len(gn), gn);
            ee_goal *g = unit_get_goal(u);
            if (g && goal_is_load(g)) {
                REPLY("    meeting %d,%d / %d,%d, pending:", goal_get_meeting_ship_x(g), goal_get_meeting_ship_y(g), goal_get_meeting_x(g),
                      goal_get_meeting_y(g));
                int k = 0;
                for (ee_unit **pp = goal_get_pending_begin(g); pp && pp < goal_get_pending_end(g) && k < 6; pp++, k++) {
                    const char *pn = tw_goal_name(unit_get_goal(*pp));
                    REPLY(" %d,%d %s %.*s;", unit_get_x(*pp), unit_get_y(*pp), tw_state(unit_ai_state_id(*pp)), tw_goal_len(pn), pn);
                }
                REPLY("\n");
            }
            continue;
        }
        if (!close) continue;
        if (tw_carrier(pl, u)) { if (nt < 16) tr[nt++] = u; continue; }
        if (!strcmp(what, "load") && (is_soldier(it.td) || is_citizen_type(unit_type_get_id(it.td))) && !unit_get_holder(u) && nu < 1024) us[nu++] = u;
    }
    if (!strcmp(what, "id")) return;
    if (order && nt && (nu || strcmp(what, "load"))) {
        static uint32_t ids[1040];
        static __attribute__((aligned(8))) uint8_t mem[0x40];
        unsigned n = 0;
        ee_unit *click = NULL, *sel1 = NULL;   // the transport clicked (the nearest x, y), the selection's first of the click's class
        for (int k = 0; k < nt; k++) {
            if (!click || ivec2_dist2(unit_get_tile(tr[k]), c) < ivec2_dist2(unit_get_tile(click), c)) click = tr[k];
            if (tw_kind(tr[k]) == TW_FORT || (strcmp(what, "load") && unit_count_inside(tr[k]) <= 0)) continue;   // a building isn't selected with units
            ids[n++] = (uint32_t)unit_get_serial(tr[k]);
        }
        for (int want = TW_AIR, j = 0; want >= 0 && !sel1; want = want == TW_AIR ? TW_LAND : want == TW_LAND ? TW_SEA : -1)   // 0x403ADE's order
            for (j = 0; j < nt && !sel1; j++)
                if (tw_kind(tr[j]) == want && tw_kind(tr[j]) != TW_FORT && (!strcmp(what, "load") ? 1 : unit_count_inside(tr[j]) > 0)) sel1 = tr[j];
        for (int i = 0; i < nu; i++) ids[n++] = (uint32_t)unit_get_serial(us[i]);
        uint32_t *sel[3] = { NULL, ids, ids + n };   // a select message's selection: its ids at +4 .. +8
        player_select(pl, sel);
        void *grp = player_get_selected_group(pl);
        memset(mem, 0, sizeof mem);
        if (!strcmp(what, "load")) {   // as the click makes it: the clicked one, or the selection's first transport and the clicked one as its unit
            int same = !sel1 || sel1 == click || unit_get_type_data(sel1) == unit_get_type_data(click);
            ee_load_msg *msg = (ee_load_msg *)mem;
            load_msg_make(msg, player_get_msg_id(pl), (unsigned)unit_get_serial(same ? click : sel1), same ? -1 : unit_get_serial(click));
            hook_msg_load(msg);
        }
        else { ee_unload_msg *msg = (ee_unload_msg *)mem; unload_msg_make(msg, player_get_msg_id(pl), x, y); hook_msg_unload(msg); }
        REPLY("%s by P%d (this PC's P%u): %u selected, %u in the group;", what, p, local_player_index(), n, grp ? group_count(grp) : 0);
        for (int k = 0; k < nt; k++) {
            const char *gn = tw_goal_name(unit_get_goal(tr[k]));
            REPLY(" %06x %.*s;", tw_id(tr[k]), tw_goal_len(gn), gn);
        }
        REPLY("\n");
        return;
    }
    if (!strcmp(what, "load")) {
        if (!nt || !nu) { REPLY("%d transports, %d units near %d,%d\n", nt, nu, x, y); return; }
        int given = tw_load_split(pl, tr, nt, us, nu, &used);
        REPLY("%d of %d units over %d of %d transports:", given, nu, used, nt);
        for (int k = 0; k < nt; k++) {
            ee_goal *g = unit_get_goal(tr[k]);
            int n = g && goal_is_load(g) ? (int)(goal_get_pending_end(g) - goal_get_pending_begin(g)) : 0;
            REPLY(" %06x room %d pending %d;", tw_id(tr[k]), unit_get_room(tr[k]), n);
        }
        REPLY("\n");
        return;
    }
    if (!strcmp(what, "unload")) {
        int n = 0;
        for (int k = 0; k < nt; k++) {   // the game's command: one unload at x, y for all
            if (unit_count_inside(tr[k]) <= 0) continue;
            ee_goal *g = goal_unload(tr[k], x, y, 0);
            if (g && unit_goal(tr[k], g, 0, TW_PREPARE_TO_MOVE)) tr[n++] = tr[k];
        }
        int s = n >= 2 && g_tw_spread ? tw_unload_spread(pl, tr, n, c, at, asz, 0) : 0;
        REPLY("%d loaded transports, %d sent to their own tile:", n, s);
        for (int k = 0; k < n && n >= 2; k++) REPLY(" %06x %d aboard -> %d,%d;", tw_id(tr[k]), unit_count_inside(tr[k]), at[k].x, at[k].y);
        REPLY("\n");
        return;
    }
    REPLY("transports load|unload P x y [r] | id P\n");
}

internal void tw_new_game(void) { memset(g_tw, 0, sizeof g_tw); g_tw_n = 0; g_tw_last = 0; g_tw_dropped = g_tw_kicked = 0; }
internal void tw_install(const char *autotest_ini) {
    g_tw_log = g_set.transport_log;
    g_tw_dog = g_set_ai[1].transport_watchdog;
    g_v_tw_dog = autotest_ini ? ini_int("autotest", "v_transport_watchdog", -1, autotest_ini) : -1;
    if (g_log) fprintf(g_log, "# tw: watchdog %s (variant players %d)%s\n", g_tw_dog ? "on" : "off", g_v_tw_dog, g_tw_log ? ", transport states logged" : "");
    g_tw_split = g_set.transport_split;
    g_tw_spread = g_set.transport_unload_spread;
    if (autotest_ini) g_tw_meet = ini_int("autotest", "transport_meeting", 1, autotest_ini);
    int hc = g_tw_split || g_tw_spread ? patch_click_mark(hook_click_mark) : 0;
    int hl = g_tw_split ? patch_msg_load(hook_msg_load) + hc : 0, hu = g_tw_spread ? patch_msg_unload(hook_msg_unload) + patch_unload_marks(hook_unload_marks) + hc : 0;
    if (g_log) fprintf(g_log, "# tw: the player's load split %d (hooks %s), unload spread %d (hooks %s)\n", g_tw_split, g_tw_split ? (hl == 2 ? "ok" : "FAILED") : "-",
                       g_tw_spread, g_tw_spread ? (hu == 3 ? "ok" : "FAILED") : "-");
}
