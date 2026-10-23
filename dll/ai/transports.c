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
            units_inside(u));
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
                    (now - t->since) / 1000.0, n - left, n, c[TWP_IDLE], c[TWP_NOGOAL], c[TWP_OTHER], c[TWP_GONE], left, units_inside(u));
        }
        return;
    }
    if (t->st == TW_IDLE && goal_is_unload(g) && units_inside(u) > 0 && now - t->since >= 20000 && t->kicks < 3) {
        t->kicks++; g_tw_kicked++;
        if (g_log && (g_tw_log || g_tw_kicked <= 20)) {
            tw_head(t, u, now, "watchdog:");
            fprintf(g_log, " Idle %.0f s with its unload to %d,%d and %d aboard - PrepareToMove (%d)\n", (now - t->since) / 1000.0,
                    goal_get_unload_x(g), goal_get_unload_y(g), units_inside(u), t->kicks);
        }
        unit_set_state(u, TW_PREPARE_TO_MOVE);
    }
}

struct tw_scan { uint32_t now; };
internal void tw_unit(ee_unit *u, void *ctx) {
    struct tw_scan *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    if (!td || !is_transport(type_get_id(td)) || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) return;
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
    if (!unit_get_goal(u) && units_inside(u) <= 0) { t->since = s->now; return; }   // idle and empty: not a stall
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
internal void tw_new_game(void) { memset(g_tw, 0, sizeof g_tw); g_tw_n = 0; g_tw_last = 0; g_tw_dropped = g_tw_kicked = 0; }
internal void tw_install(const char *aimod_ini, const char *autotest_ini) {
    g_tw_log = autotest_ini ? cfg_int(aimod_ini, autotest_ini, 1, "transport_log", 0) : 0;
    g_tw_dog = cfg_int(aimod_ini, autotest_ini, 1, "transport_watchdog", 0);
    g_v_tw_dog = autotest_ini ? ini_int("autotest", "v_transport_watchdog", -1, autotest_ini) : -1;
    if (g_log) fprintf(g_log, "# tw: watchdog %s (variant players %d)%s\n", g_tw_dog ? "on" : "off", g_v_tw_dog, g_tw_log ? ", transport states logged" : "");
}
