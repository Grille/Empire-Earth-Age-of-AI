// BUILD RETRY (`build_retry`): a human player's citizen that ends its walk to a building site one tile off gets its
// build order again instead of standing there. The citizen script (citizen.tai) checks at the walk's end
// (CheckRepairSiteAccessibility) whether the citizen stands on a tile of the building's ring - RepairSiteInRange, the
// build goal's condition 0xFAE: its tile exactly one of those its type lists - and goes idle on a no, without a second
// try. Citizens sent together all walk to the ring tile nearest them: after a move order five stand close, choose the
// same tile and push each other off it, and the ones pushed off stop next to the site for good (the player: five
// citizens given a move order and then a build order all walk to the foundation, only one or two begin to build;
// tests/probes/build_after_move.eeai) - or a way round another citizen in that crowd isn't found and the movement
// script gives up. Here the build goal's condition test (EEUGBuildRepair vf25, the movement's conditions too) notes such a
// citizen, and at the next safe point - the goal is not changed under the script - the game's own placing call
// (unit_build_next: its goals dropped, the building first in its build list) sends it again: the script picks the ring
// tile nearest to where it now stands. At most BR_TRIES times per citizen and building, and only while the building
// still needs work, the citizen stands idle and the player gave it no other order.

#define BR_MAX 64
#define BR_TRIES 3
#define BR_FORGET_MS 60000   // a citizen's tries counted afresh after that long
#define COND_REPAIR_SITE_IN_RANGE 0xFAE
#define STATE_REPAIR_SITE_ACCESS 4017   // CheckRepairSiteAccessibility
#define COND_MOVE_PREPARED 8000         // MovePreparationComplete
#define STATE_REPATH 9002               // RepathAroundObstacle
#define STATE_IDLE 5000

internal int g_br_on;
internal struct br_entry { uint32_t unit, bld, at; int tries, due; } g_br[BR_MAX];
internal int g_br_n;
internal unsigned g_br_retried, g_br_gave_up;   // tests: `sitecheck`'s line

internal int br_needs_work(ee_unit *bld) {
    ee_unit_type *td = bld ? unit_get_type_data(bld) : NULL;
    return td && unit_get_hp(bld) > 0 && unit_get_hp(bld) < unit_type_get_hp(td);
}

internal char __thiscall hook_build_condition(ee_build_goal *g, ee_unit *u, uint32_t cond) {
    char r = build_goal_condition(g, u, cond);
    // the two ways a build order ends next to the site: off the ring at the walk's end, or the way round a unit in
    // the crowd not found (generic movement.tai: RepathAroundObstacle on a false MovePreparationComplete -> Idle)
    unsigned st = u && !r ? unit_ai_state_id(u) : 0;
    if (!((cond == COND_REPAIR_SITE_IN_RANGE && st == STATE_REPAIR_SITE_ACCESS) || (cond == COND_MOVE_PREPARED && st == STATE_REPATH))) return r;
    ee_player *pl = unit_get_owner(u);
    ee_unit *bld = goal_get_building((ee_goal *)g);
    if (!pl || !player_is_human(pl) || !br_needs_work(bld)) return r;
    uint32_t id = (uint32_t)unit_get_serial(u), bid = (uint32_t)unit_get_serial(bld), now = game_time();
    struct br_entry *e = NULL;
    for (int i = 0; i < g_br_n && !e; i++) if (g_br[i].unit == id) e = &g_br[i];
    if (e && (e->bld != bid || now - e->at > BR_FORGET_MS)) e->tries = 0;
    if (!e && g_br_n < BR_MAX) { e = &g_br[g_br_n++]; e->tries = 0; }
    if (!e) return r;
    e->unit = id; e->bld = bid; e->at = now;
    if (e->tries >= BR_TRIES) { if (!e->due) g_br_gave_up++; e->due = 0; return r; }
    e->due = 1;
    return r;
}

// the safe point (control.c's hook_sim_receive): the noted citizens sent again
internal void build_retry_tick(void) {
    if (!g_br_on || !g_br_n) return;
    uint32_t now = game_time();
    for (int i = 0; i < g_br_n; ) {
        struct br_entry *e = &g_br[i];
        ee_unit *u = unit_by_id(e->unit), *bld = unit_by_id(e->bld);
        if (!u || !bld || unit_get_hp(u) <= 0 || now - e->at > BR_FORGET_MS) { g_br[i] = g_br[--g_br_n]; continue; }
        if (e->due && (now - e->at > 2000 || !br_needs_work(bld))) e->due = 0;   // it never went idle, or no work left
        else if (e->due && unit_ai_state_id(u) == STATE_IDLE && !unit_get_goal(u)) {
            e->due = 0;
            e->tries++;
            g_br_retried++;
            unit_build_next(u, bld);
        }
        i++;
    }
}

internal void build_retry_new_game(void) { g_br_n = 0; g_br_retried = g_br_gave_up = 0; }

// A GATE'S BUILDERS (always): a citizen's work on a building of the walls family (0x60C0F5) asks for the piece that
// replaces it as the pieces join up (0x6311A8) and hands it to the build goal's replacement (EEUGBuildRepair vf27,
// 0x60D759), which reads it without a check - for a gate there is none: the game crashed the moment citizens worked on a
// gate (read at 0x18, 0x60D77B; the mod's straight walls lay one in their middle - the game's own AI never sends its
// citizens to one). No replacement: nothing replaced, the goal keeps its building.
internal unsigned g_gate_nulls;
internal void __thiscall hook_build_replaced(ee_build_goal *g, ee_unit *old, void *repl, int32_t arg) {
    if (!repl) { g_gate_nulls++; return; }
    build_goal_replaced(g, old, repl, arg);
}

internal void build_retry_install(void) {
    g_br_on = g_set.build_retry;
    int ok = g_br_on ? patch_build_condition(hook_build_condition) : 0;
    if (g_log) fprintf(g_log, "# build_retry: %d (%s)\n", g_br_on, !g_br_on ? "-" : ok ? "hooked" : "FAILED");
    if (!patch_build_replaced(hook_build_replaced) && g_log) fprintf(g_log, "# build: the build goal's replacement not hooked - gates crash their builders\n");
}
