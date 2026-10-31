// THE PATH MANAGER (0x917D00, ticked every simulation step): the units' route requests - functions, hooks.
// A request (path_mgr_request) runs a tile A* (8 directions) at once or in slices of about 32 nodes a step, long ones
// through the map's sector graph as a chain of auto-waypoint goals (a new request whenever few waypoints are left);
// the result is written to the unit's waypoints (unit_add_waypoint). docs/engine-notes.md "Routes".

// HOOKS
// the route request (all its calls: PrepareToMove, a sector route's first leg, the re-path of all) - the hook calls
// path_mgr_request for what it doesn't route itself. AoC's takes an 8th argument (ret 0x20): with it set, a unit whose
// +0x8C is 7 or more skips the request's first try (0x4A78C1) - its own hook, which hands it on unread.
typedef void (__thiscall *path_request_hook)(void *mgr, ee_unit *u, int *from, int *to, int sync, int limit, int sectors, void *ctx);
typedef void (__thiscall *path_request_aoc_hook)(void *mgr, ee_unit *u, int *from, int *to, int sync, int limit, int sectors, void *ctx, int skip_try);
internal inline void path_mgr_request_aoc(void *mgr, ee_unit *u, int *from, int *to, int sync, int limit, int sectors, void *ctx, int skip_try) {
    ((path_request_aoc_hook)(void *)fn_path_mgr_request)(mgr, u, from, to, sync, limit, sectors, ctx, skip_try);
}
internal inline int patch_path_request(path_request_hook h, path_request_aoc_hook h_aoc) {
    return patch_all_calls((uintptr_t)fn_path_mgr_request, build_is("aoc") ? (void *)h_aoc : (void *)h);
}
// a group's move far away (EEActiveGroup::vf16): for each cluster of its units a sector route, each unit given its
// chain of auto-waypoint goals (the sector centres) - the hook calls group_sector_route for what it leaves to it
// the units of such a cluster: begin .. end
internal inline ee_unit **unitlist_get_begin(const void *units) { return FIELD(units, of_unit_list_units, ee_unit **); }
internal inline ee_unit **unitlist_get_end(const void *units) { return FIELD(units, of_unit_list_units + 0x4, ee_unit **); }
