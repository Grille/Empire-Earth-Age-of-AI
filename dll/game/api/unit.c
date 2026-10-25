// A UNIT (ee_unit: EEUnit, EEComplexUnit - a unit, building, resource site or projectile on the map): its fields, the
// game's functions on it, its hooks. What spec/game/unit.cs describes is gen/unit.c's (unit_get_owner, unit_get_speed,
// unit_max_speed, unit_set_goal, patch_aim ...); here the rows not moved yet and the code of our own on top.

// FIELDS
internal inline ivec2 unit_get_tile(const ee_unit *u) { return iv2(unit_get_x(u), unit_get_y(u)); }
// +0x24 its id: the player in bits 24..31 (a load message names its transport by it, unit_by_id finds it back) - the
// test opponent keeps it beside the pointer, to tell a reused unit object from the one before
internal inline int *unit_get_queue_end(const ee_unit *u) { return FIELD(u, of_unit_queue_begin + 0x4, int *); }        // +0x50
// +0x68: not known - the AI's gatherer checks (econ.c, food.c) pass over a citizen where it is set
// its facing (the local y axis) and position in tiles: a column of its matrix each, its parts not side by side,
// gathered into one value
internal inline vec3 unit_get_facing(const ee_unit *u) { return v3(unit_get_matrix(u, 1), unit_get_matrix(u, 5), unit_get_matrix(u, 9)); }
internal inline vec3 unit_get_pos(const ee_unit *u) { return v3(unit_get_matrix(u, 3), unit_get_matrix(u, 7), unit_get_matrix(u, 11)); }
// +0x1CC: the next of a chain from +0x30 - its last is what a BuildRepair goal holds (0x60BAD0)
// the state's id (the .tai state names' numbers: Idle 5000, WaitForPassengers 12010 ... - engine-notes.md), 0 without one
internal inline unsigned unit_ai_state_id(const ee_unit *u) { ee_ai_state *s = unit_get_ai_state(u); return s ? FIELD(s, of_ai_state_id, unsigned) : 0; }
// +0x2E0 a ship's mode against a tile's tile_get_sea_class (0, 3 ... - a guess: its draught)
// its waypoint i (0 the current) into pt[3] - the deque's element as the game's own accessor (0x5E80BE) finds it: 0,
// else it has no such
internal inline int unit_waypoint_at(ee_unit *u, int i, float *pt) {
    if (i < 0 || i >= unit_get_waypoints_left(u)) return 7;
    uint8_t *dq = unit_waypoints_at(u);
    uint32_t pos = *(uint32_t *)(dq + 0xC) + (uint32_t)i, blocks = *(uint32_t *)(dq + 8), blk = pos / 0xCC;
    if (blk >= blocks) blk -= blocks;
    float *e = (float *)((*(uint8_t ***)(dq + 4))[blk] + (pos % 0xCC) * 20);
    pt[0] = e[0]; pt[1] = e[1]; pt[2] = e[2];
    return 0;
}
// moves its waypoint i to x, y (its z and the route lengths stay): 0, else it has no such
internal inline int unit_set_waypoint_at(ee_unit *u, int i, float x, float y) {
    if (i < 0 || i >= unit_get_waypoints_left(u)) return 7;
    uint8_t *dq = unit_waypoints_at(u);
    uint32_t pos = *(uint32_t *)(dq + 0xC) + (uint32_t)i, blocks = *(uint32_t *)(dq + 8), blk = pos / 0xCC;
    if (blk >= blocks) blk -= blocks;
    float *e = (float *)((*(uint8_t ***)(dq + 4))[blk] + (pos % 0xCC) * 20);
    e[0] = x; e[1] = y;
    return 0;
}
// +0x341: held by the AI (EEComputerPlayer vf19 sets it; cleared when the AI takes the unit for an attack)

// FUNCTIONS
// its acceleration and top turn rate (radians/s) as its movement uses them (its group's with the group speed on:
// engine-notes "top speed"; its top speed: unit_max_speed)
internal inline float unit_accel(ee_unit *u) { return GF(FN_UNIT_ACCEL)(u); }
internal inline float unit_max_turn(ee_unit *u) { return GF(FN_UNIT_MAX_TURN)(u); }
internal inline float unit_turn_accel(ee_unit *u) { return GF(FN_UNIT_TURN_ACCEL)(u); }   // radians/s per s
// its orders (goal slot 0's queue: the front the current goal, shift-clicked moves after it): how many, and order i
internal inline int unit_orders(ee_unit *u) {
    uint8_t *q = (uint8_t *)GF(FN_UNIT_GOAL_QUEUE)(u, 0);
    return q ? *(int *)(q + 0x10) : 0;
}
internal inline ee_goal *unit_order_at(ee_unit *u, int i) {
    uint8_t *q = (uint8_t *)GF(FN_UNIT_GOAL_QUEUE)(u, 0);
    if (!q || i < 0 || i >= *(int *)(q + 0x10)) return NULL;
    uint32_t pos = *(uint32_t *)(q + 0xC) + (uint32_t)i, blocks = *(uint32_t *)(q + 8), blk = pos >> 10;
    if (blk >= blocks) blk -= blocks;
    return ((ee_goal ***)*(void **)(q + 4))[blk][pos & 0x3FF];
}
// drops its current waypoint, the next the current (as GetNextMoveWaypoint does on arrival): 0, else it had none
internal inline int unit_pop_waypoint(ee_unit *u) { return GF(FN_POP_WAYPOINT)(u); }
// the citizens working site (all players)
internal inline unsigned site_gatherers(ee_unit *site) { return GF(FN_SITE_GATHERERS)((void *)GA(G_GAME_STATE), site); }
// a shot's ballistic path from -> to for unit u (type: its live stats): reachable (pitch out); the aim at target
internal inline char ballistic(ee_unit *u, ee_unit_stats *type, float *from, float *to, float *pitch) {
    return GF(FN_BALLISTIC)(u, type, from, to, pitch);
}
// is the map object explored by pl (a site's +0x14: its map object)
internal inline char explored_by(ee_unit *site, ee_player *pl) { return GF(FN_EXPLORED)(FIELD(site, of_unit_vis, void *), pl); }

// the unit's script state set (SetState: its .tai state by id - engine-notes.md "Unit AI scripts"; entered, it runs once):
// 8000 PrepareToMove after every order, 5000 Idle, 0x1F51 MoveToRepairSite (the game's builder order)
internal inline void unit_set_state(ee_unit *u, unsigned state) { GF(FN_UNIT_SET_STATE)(u, (int)state); }
// the units aboard a transport or inside a building
internal inline int units_inside(ee_unit *u) { return GF(FN_UNITS_INSIDE)(u); }
// the distance between two units (tiles)
internal inline float unit_distance(ee_unit *a, ee_unit *b) { return GF(FN_UNIT_DISTANCE)(a, b); }
// the unit changed in place into the type of new_td, owner player index pi (defence.c: a line upgrade)
internal inline int unit_convert(ee_unit *u, int pi, ee_unit_type *new_td) { return GF(FN_UNIT_CONVERT)(u, pi, (uint32_t)new_td); }
// the unit taken out of the map's unit list (the original of attack.c's guards; given back to the pool:
// ee_complex_unit_release)
internal inline void unit_unlink(ee_unit *u) { GF(FN_UNIT_UNLINK)(u); }

// HOOKS
// (the spec's in gen/unit.c: patch_aim, patch_missile_aim, patch_repair_goal, patch_unload_retry_give, patch_unit_release)
// the unit list's node deletions (operator delete of a node, the unit in ESI): the thunk ends in game_op_delete
internal inline int patch_unit_node_free(thunk_fn thunk, int *of) {
    const uint32_t sites[2] = { GA(CALL_NODE_DEL_1), GA(CALL_NODE_DEL_2) };
    return patch_calls(sites, 2, GA(FN_OP_DELETE), (void *)thunk, of);
}
internal inline uintptr_t game_op_delete(void) { return GA(FN_OP_DELETE); }
