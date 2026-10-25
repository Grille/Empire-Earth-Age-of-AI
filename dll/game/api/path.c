// THE PATH MANAGER (0x917D00, ticked every simulation step): the units' route requests - functions, hooks.
// A request (FN_PATH_REQUEST) runs a tile A* (8 directions) at once or in slices of about 32 nodes a step, long ones
// through the map's sector graph as a chain of auto-waypoint goals (a new request whenever few waypoints are left);
// the result is written to the unit's waypoints (unit_add_waypoint). docs/engine-notes.md "Routes".

// HOOKS
// the route request (all its calls: PrepareToMove, a sector route's first leg, the re-path of all) - the hook calls
// path_request for what it doesn't route itself. Only the 2017 update: AoC's takes an 8th argument.
typedef void (__thiscall *path_request_hook)(void *mgr, ee_unit *u, int *from, int *to, int sync, int limit, int sectors, void *ctx);
internal inline void path_request(void *mgr, ee_unit *u, int *from, int *to, int sync, int limit, int sectors, void *ctx) {
    GF(FN_PATH_REQUEST)(mgr, u, from, to, sync, limit, sectors, ctx);
}
internal inline int patch_path_request(path_request_hook h) { return build_is("aoc") ? 0 : patch_all_calls(GA(FN_PATH_REQUEST), (void *)h); }
// a group's move far away (EEActiveGroup::vf16): for each cluster of its units a sector route, each unit given its
// chain of auto-waypoint goals (the sector centres) - the hook calls group_sector_route for what it leaves to it
typedef void (__thiscall *group_sector_route_hook)(void *group, int *from, int *to, void *units);
internal inline void group_sector_route(void *group, int *from, int *to, void *units) { GF(FN_GROUP_SECTOR_ROUTE)(group, from, to, units); }
internal inline int patch_group_sector_route(group_sector_route_hook h) { return patch_call(GA(CALL_GROUP_SECTOR_ROUTE), GA(FN_GROUP_SECTOR_ROUTE), (void *)h); }
// the units of such a cluster: begin .. end
internal inline ee_unit **unitlist_get_begin(const void *units) { return FIELD(units, of_unit_list_units, ee_unit **); }
internal inline ee_unit **unitlist_get_end(const void *units) { return FIELD(units, of_unit_list_units + 0x4, ee_unit **); }
