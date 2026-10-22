// THE GAME'S FUNCTIONS THE MOD HOOKS: for each, the type of its hook and the installer. A hook stands in for the game's
// function at its calls (patch_call: the call instruction redirected, only while it still calls that function) or in
// its class's vtable (patch_vtable: only while the slot still holds it); it calls the original through functions.h's
// wrapper when it wants the game's own work done. The hook types are the game's real arguments - a hook declared with
// other parameters doesn't build (a thiscall hook is __fastcall with the unused edx second). An installer returns how
// many places it patched; of (may be NULL) gets the number there are. Kinds not typed yet are void *, as in functions.h.
// Assembler stubs (they read registers) are void (*)(void).

typedef void (*thunk_fn)(void);

internal inline int patch_calls(const uint32_t *sites, size_t n, uintptr_t fn, void *hook, int *of) {
    int ok = 0;
    for (size_t i = 0; i < n; i++) ok += patch_call(sites[i], fn, hook);
    if (of) *of += (int)n;
    return ok;
}
internal inline int patch_vtables(const uint32_t *vts, size_t n, int slot, uintptr_t fn, void *hook, int *of) {
    int ok = 0;
    for (size_t i = 0; i < n; i++) ok += patch_vtable(vts[i], slot, fn, hook);
    if (of) *of += (int)n;
    return ok;
}
#define PATCH_LIST(L) GL(L), sizeof GL(L) / sizeof *GL(L)

// WHICH CALL A HOOK CAME FROM: at = its return address - 5 (the call instruction); the index in the list, -1: none
internal inline int site_index(const uint32_t *sites, size_t n, uintptr_t at) {
    for (size_t i = 0; i < n; i++)
        if (sites[i] == at) return (int)i;
    return -1;
}
internal inline int train_site(uintptr_t at) { return site_index(PATCH_LIST(TRAIN_SITES), at); }    // the game's 5 unit orders
internal inline int mine_site(uintptr_t at) { return site_index(PATCH_LIST(MINE_SITES), at); }      // 5: the citizen assignment's last
internal inline int unload_site(uintptr_t at) { return site_index(PATCH_LIST(UNLOAD_SITES), at); }  // 7: the unload goal's copy
internal inline uintptr_t unload_site_at(int i) { return GL(UNLOAD_SITES)[i]; }
internal inline int unload_from_rm(uintptr_t at) { return at == GA(CALL_UNLOAD_RM); }   // the resource manager's ferry

// COMPUTER PLAYERS (econ.c)
typedef void (__fastcall *cp_pay_hook)(ee_player *pl, void *edx, void *what, int b, int c);
internal inline int patch_cp_pay(cp_pay_hook h) { return patch_vtable(GA(VT_COMPUTER_PLAYER), GA(SLOT_CP_PAY), GA(FN_CP_PAY), (void *)h); }
typedef void (__fastcall *cp_add_resource_hook)(ee_player *pl, void *edx, int type, int amount);
internal inline int patch_cp_add_resource(cp_add_resource_hook h) {
    return patch_vtable(GA(VT_COMPUTER_PLAYER), GA(SLOT_CP_ADD_RESOURCE), GA(FN_CP_ADD_RESOURCE), (void *)h);
}
typedef void (__fastcall *pop_cap_hook)(ee_player *pl, void *edx);
internal inline int patch_pop_cap(pop_cap_hook h, int *of) {
    const uint32_t sites[3] = { GA(CALL_POP_CAP_1), GA(CALL_POP_CAP_2), GA(CALL_POP_CAP_3) };
    return patch_calls(sites, 3, GA(FN_POP_CAP), (void *)h, of);
}
typedef void (__fastcall *epoch_catch_up_hook)(ee_player *ai, void *edx, ee_player *human);
internal inline int patch_epoch_catch_up(epoch_catch_up_hook h) { return patch_call(GA(CALL_EPOCH_CATCH_UP), GA(FN_EPOCH_CATCH_UP), (void *)h); }
// the epoch's upgrades: the 2017 update's (node, category), Art of Conquest's (node, category, only new)
typedef void (__fastcall *epoch_upgrades_hook)(ee_tech *node, void *edx, int cat);
typedef void (__fastcall *epoch_upgrades_aoc_hook)(ee_tech *node, void *edx, int cat, int only_new);
internal inline int patch_epoch_upgrades(epoch_upgrades_hook h, epoch_upgrades_aoc_hook h_aoc) {
    return patch_call(GA(CALL_EPOCH_UPGRADES), GA(FN_EPOCH_UPGRADES), build_is("aoc") ? (void *)h_aoc : (void *)h);
}
// a tech researched for nothing (EETechTreeNode::vf6, its vtable slot VTS_TECH_RESEARCHED)
typedef int (__fastcall *tech_grant_hook)(ee_tech *t, void *edx);
internal inline int patch_tech_grant(tech_grant_hook h) { return patch_vtable(GA(VTS_TECH_RESEARCHED), 0, GA(FN_TECH_GRANT), (void *)h); }

// THE AI'S MANAGERS
typedef char (__fastcall *place_hook)(ee_build_mgr *bm, void *edx, int type, int x, int y);
internal inline int patch_place(place_hook h) { return patch_call(GA(CALL_PLACE), GA(FN_PLACE), (void *)h); }
typedef uintptr_t (__fastcall *train_units_hook)(ee_unit_mgr *um, void *edx, int *req, uintptr_t b);
internal inline int patch_train_units(train_units_hook h, int *of) { return patch_calls(PATCH_LIST(TRAIN_SITES), GA(FN_TRAIN_UNITS), (void *)h, of); }
typedef int (__fastcall *citizens_wanted_hook)(ee_unit_mgr *um, void *edx);
internal inline int patch_citizens_wanted(citizens_wanted_hook h) { return patch_call(GA(CALL_CITIZENS_WANTED), GA(FN_UM_CITIZENS_WANTED), (void *)h); }
typedef int (__fastcall *fish_type_hook)(void *o, void *edx);
internal inline int patch_fish_type(fish_type_hook h, int *of) {
    const uint32_t sites[2] = { GA(CALL_FISH_TYPE_1), GA(CALL_FISH_TYPE_2) };
    return patch_calls(sites, 2, GA(FN_FISH_TYPE), (void *)h, of);
}
// the maintenance's deletion of units for room (the navy keeps its ships: fleet.c)
typedef int (__fastcall *delete_for_room_hook)(ee_player *pl, void *edx, void *req, int flag);
internal inline int patch_maint_delete(delete_for_room_hook h) { return patch_call(GA(CALL_NV_MAINT_DELETE), GA(FN_DELETE_FOR_ROOM), (void *)h); }

// THE RESOURCE MANAGER'S SEARCHES (sites.c)
typedef void *(__fastcall *find_mine_hook)(ee_resource_mgr *rm, void *edx, int type, ee_unit *unit, uintptr_t rev);
internal inline int patch_find_mine(find_mine_hook h, int *of) { return patch_calls(PATCH_LIST(MINE_SITES), GA(FN_FIND_MINE), (void *)h, of); }
typedef void *(__fastcall *find_food_hook)(ee_resource_mgr *rm, void *edx, uintptr_t need_drop, uintptr_t near_base, ee_unit *unit);
internal inline int patch_find_food(find_food_hook h, int *of) { return patch_calls(PATCH_LIST(FOOD_SITES), GA(FN_FIND_FOOD), (void *)h, of); }
typedef void *(__fastcall *find_wood_hook)(ee_resource_mgr *rm, void *edx, uintptr_t flag, ee_unit *unit);
internal inline int patch_find_wood(find_wood_hook h, int *of) { return patch_calls(PATCH_LIST(WOOD_SITES), GA(FN_FIND_WOOD), (void *)h, of); }
typedef void *(__fastcall *farms_hook)(ee_resource_mgr *rm, void *edx, uintptr_t n, uintptr_t out);
internal inline int patch_farms(farms_hook h, farms_hook unbuilt, int *of) {
    if (of) *of += 2;
    return patch_call(GA(CALL_FARMS), GA(FN_FARMS), (void *)h) + patch_call(GA(CALL_UNBUILT_FARMS), GA(FN_UNBUILT_FARMS), (void *)unbuilt);
}
typedef void *(__fastcall *hunt_hook)(ee_resource_mgr *rm, void *edx, uintptr_t n, uintptr_t f, uintptr_t out);
internal inline int patch_hunt(hunt_hook h, int *of) {
    const uint32_t sites[2] = { GA(CALL_HUNT_1), GA(CALL_HUNT_2) };
    return patch_calls(sites, 2, GA(FN_HUNT), (void *)h, of);
}

// UNITS AND GOALS
typedef void (__fastcall *set_goal_hook)(ee_unit *u, void *edx, void *goal, int append, int prio);
// the game's repair orders (the handing-over 0x46DA88: attack.c refuses them under attack)
internal inline int patch_repair_goal(set_goal_hook h) { return patch_call(GA(CALL_REPAIR_GOAL), GA(FN_SET_GOAL), (void *)h); }
typedef void (__fastcall *give_goal_hook)(ee_unit *u, void *edx, void *goal, int slot);
internal inline int patch_unload_retry_give(give_goal_hook h) { return patch_call(GA(CALL_UNLOAD_RETRY_GIVE), GA(FN_GIVE_GOAL), (void *)h); }
typedef void *(__fastcall *unload_ctor_hook)(void *g, void *edx, ee_unit *transport, int x, int y, int flag);
internal inline int patch_unload_ctor(unload_ctor_hook h, int *of) { return patch_calls(PATCH_LIST(UNLOAD_SITES), GA(FN_GOAL_UNLOAD_CTOR), (void *)h, of); }
typedef int (__fastcall *shore_to_unload_hook)(void *mgr, void *edx, ee_unit *transport);
internal inline int patch_shore_to_unload(shore_to_unload_hook h) { return patch_call(GA(CALL_SHORE_TO_UNLOAD), GA(FN_SHORE_TO_UNLOAD), (void *)h); }
typedef int (__fastcall *goalset_ask_hook)(void *set, void *edx, ee_unit *unit, int *out);
internal inline int patch_goalset_ask(goalset_ask_hook h, int *of) {
    const uint32_t sites[2] = { GA(CALL_GOALSET_ASK), GA(CALL_GOALSET_AGAIN) };
    return patch_calls(sites, 2, GA(FN_GOALSET_ASK), (void *)h, of);
}
// the goal set's read of a unit's goal object (6 bytes: mov ecx, [esi+0x1E8]) -> call thunk; nop
internal inline int patch_goalset_read(thunk_fn thunk) {
    static const uint8_t want[6] = { 0x8B, 0x8E, 0xE8, 0x01, 0x00, 0x00 };
    uint8_t *p = (uint8_t *)GA(AT_GOAL_SET_READ);
    DWORD old;
    if (memcmp(p, want, 6) || !VirtualProtect(p, 6, PAGE_EXECUTE_READWRITE, &old)) return 0;
    p[0] = 0xE8; *(int32_t *)(p + 1) = (int32_t)((uintptr_t)thunk - (GA(AT_GOAL_SET_READ) + 5)); p[5] = 0x90;
    VirtualProtect(p, 6, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 6);
    return 1;
}
typedef char (__fastcall *goal_fits_hook)(void *goal, void *edx, void *unit);
internal inline int patch_goal_fits(goal_fits_hook h) { return patch_call(GA(CALL_GOAL_FBA_CHECK), GA(FN_GOAL_FBA_CHECK), (void *)h); }
typedef void (__fastcall *unit_release_hook)(ee_unit *u, void *edx);
internal inline int patch_unit_release(unit_release_hook h) { return patch_vtable(GA(VT_COMPLEX_UNIT), GA(SLOT_UNIT_RELEASE), GA(FN_UNIT_RELEASE), (void *)h); }
// the unit list's node deletions (operator delete of a node, the unit in ESI): the thunk ends in game_op_delete
internal inline int patch_unit_node_free(thunk_fn thunk, int *of) {
    const uint32_t sites[2] = { GA(CALL_NODE_DEL_1), GA(CALL_NODE_DEL_2) };
    return patch_calls(sites, 2, GA(FN_OP_DELETE), (void *)thunk, of);
}
internal inline uintptr_t game_op_delete(void) { return GA(FN_OP_DELETE); }
// a model's damage points (the guard: defence.c), a kill counted (diplo.c): thunks
internal inline int patch_damage_points(thunk_fn thunk) { return patch_call(GA(CALL_DAMAGE_POINTS), GA(FN_DAMAGE_POINTS), (void *)thunk); }
internal inline int patch_kill_stat(thunk_fn thunk) { return patch_call(GA(CALL_KILL_STAT), GA(FN_STAT_ADD), (void *)thunk); }

// ATTACK GROUPS AND SUBGROUPS (attack.c, assault.c, landing.c)
typedef char (__fastcall *unit_to_group_hook)(void *cmd, void *edx, ee_unit *u);
internal inline int patch_unit_to_group(unit_to_group_hook h) { return patch_call(GA(CALL_UNIT_TO_GROUP), GA(FN_UNIT_TO_GROUP), (void *)h); }
typedef void (__fastcall *group_stat_hook)(void *stats, void *edx, ee_unit *u);
internal inline int patch_group_stat(group_stat_hook h) { return patch_call(GA(CALL_GROUP_STAT), GA(FN_GROUP_STAT), (void *)h); }
typedef void (__cdecl *range_sort_hook)(void **b, void **e, ee_unit *unit);
internal inline int patch_range_sort(range_sort_hook h) { return patch_call(GA(CALL_RANGE_SORT), GA(FN_RANGE_SORT), (void *)h); }
typedef void (__fastcall *asg_hook)(void *sg, void *edx);
// the subgroups' stats: the land subgroups' own, the others' (ASG_VTABLES)
internal inline int patch_asg_land_stats(asg_hook h) { return patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_STATS), GA(FN_ASG_LAND_STATS), (void *)h); }
internal inline int patch_asg_stats(asg_hook h, int *of) { return patch_vtables(PATCH_LIST(ASG_VTABLES), GA(SLOT_ASG_STATS), GA(FN_ASG_STATS), (void *)h, of); }
typedef void (__fastcall *asg_update_hook)(void *sg, void *edx, int now);
internal inline int patch_asg_update(asg_update_hook h) { return patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_UPDATE), GA(FN_ASG_UPDATE), (void *)h); }
internal inline int patch_asg_sea_update(asg_update_hook h) { return patch_vtable(GA(VT_ASG_SEA), GA(SLOT_ASG_UPDATE), GA(FN_ASG_SEA_UPDATE), (void *)h); }
internal inline int patch_asg_sea_transport_update(asg_update_hook h) {
    return patch_vtable(GA(VT_ASG_SEA_TRANSPORT), GA(SLOT_ASG_UPDATE), GA(FN_ASG_SEA_TRANSPORT_UPDATE), (void *)h);
}
internal inline int patch_asg_air_update(asg_update_hook h) { return patch_vtable(GA(VT_ASG_AIR), GA(SLOT_ASG_UPDATE), GA(FN_ASG_AIR_UPDATE), (void *)h); }
// the subgroups' orders to a unit (vf57): sea transport, sea, land through h, air through h_air - of 4
typedef char (__fastcall *asg_give_goal_hook)(void *sg, void *edx, ee_unit *u, void *goal, int a, int prio);
internal inline int patch_asg_give_goal(asg_give_goal_hook h, asg_give_goal_hook h_air) {
    return patch_vtable(GA(VT_ASG_SEA_TRANSPORT), GA(SLOT_ASG_GIVE_GOAL), GA(FN_ASG_GIVE_GOAL), (void *)h) +
           patch_vtable(GA(VT_ASG_SEA), GA(SLOT_ASG_GIVE_GOAL), GA(FN_ASG_GIVE_GOAL), (void *)h) +
           patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_GIVE_GOAL), GA(FN_ASG_GIVE_GOAL), (void *)h) +
           patch_vtable(GA(VT_ASG_AIR), GA(SLOT_ASG_GIVE_GOAL), GA(FN_ASG_AIR_GIVE_GOAL), (void *)h_air);
}
// a land subgroup's route and its arrival, the route planner's edge cost
typedef void (__fastcall *asg_route_hook)(void *sg, void *edx, int *start, int *dest);
internal inline int patch_asg_route(asg_route_hook h) { return patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_ROUTE), GA(FN_ASG_ROUTE), (void *)h); }
internal inline int patch_asg_route_done(asg_hook h) { return patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_ROUTE_DONE), GA(FN_ASG_ROUTE_DONE), (void *)h); }
typedef int (__fastcall *route_cost_hook)(void *planner, void *edx, int *edge, uint8_t *node);
internal inline int patch_route_cost(route_cost_hook h) { return patch_vtable(GA(VT_ROUTE_PLANNER), GA(SLOT_ROUTE_COST), GA(FN_ROUTE_COST), (void *)h); }
// the land members' score of an enemy (base, hero, priests and prophets: ASGU_VTABLES)
typedef float (__fastcall *asgu_score_hook)(void *member, void *edx, uint8_t *info);
internal inline int patch_asgu_score(asgu_score_hook h, int *of) {
    return patch_vtables(PATCH_LIST(ASGU_VTABLES), GA(SLOT_ASGU_SCORE), GA(FN_ASGU_SCORE), (void *)h, of);
}

// MESSAGES AND THE MENU (diplo.c)
typedef void (__fastcall *msg_hook)(void *msg, void *edx);
internal inline int patch_msg_pay_tribute(msg_hook h) { return patch_vtable(GA(VT_PAY_TRIBUTE), GA(SLOT_MSG_EXECUTE), GA(FN_PAY_TRIBUTE), (void *)h); }
internal inline int patch_msg_ingame_chat(msg_hook h) { return patch_vtable(GA(VT_INGAME_CHAT), GA(SLOT_MSG_EXECUTE), GA(FN_INGAME_CHAT), (void *)h); }
internal inline int patch_msg_resign(msg_hook h) { return patch_vtable(GA(VT_RESIGN), GA(SLOT_MSG_EXECUTE), GA(FN_RESIGN), (void *)h); }
// the exit dialog's confirmation (a code pointer in the menu's table)
internal inline int patch_exit_confirm(void (__cdecl *h)(void)) { return patch_code32(GA(EXIT_CONFIRM_AT), GA(FN_EXIT_CONFIRM), (uint32_t)(uintptr_t)h); }
