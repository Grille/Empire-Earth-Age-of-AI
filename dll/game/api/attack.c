// THE AI'S ATTACKS (ee_combat_mgr, ee_attack_cmd, ee_attack_group, ee_attack_subgroup, ee_asg_unit; tasks,
// strategies, the route planner): fields, functions, hooks.

// FIELDS
// THE COMBAT MANAGER (EECPCombatManager, a computer player's +0xB8C) and its attack commander, which keeps the attack
// groups: a group has its subgroups (one per kind: land, sea, sea transport, air), a subgroup its unit records
// the attention's activities 6 ([+0x4C] +0x48; the object at +0x4C not known) and 8 (+0x62)
internal inline uint8_t combat_mgr_get_att_4c(const ee_combat_mgr *am) {
    uint8_t *o = FIELD(am, of_combat_mgr_0x4c, uint8_t *);
    return o ? o[of_combat_mgr_4c_0x48] : 0;
}
// +0x4 the groups, a deque of ee_attack_group * (deque_at); +0x14 its count
internal inline int attack_cmd_get_group_count(const ee_attack_cmd *ac) { return FIELD(ac, of_attack_cmd_groups + 0x10, int); }

// AN ATTACK GROUP (EECPAttackGroup)
internal inline ee_attack_subgroup **attack_group_get_subgroups_end(const ee_attack_group *g) { return FIELD(g, of_attack_group_subgroups_begin + 0x4, ee_attack_subgroup **); }

// AN ATTACK SUBGROUP (EECPAttackSubGroup; EECPASGLand, attack_subgroup_is_land, has the fields from +0x60 on)
internal inline ee_asg_unit **attack_subgroup_get_units_end(const ee_attack_subgroup *sg) { return FIELD(sg, of_attack_subgroup_units_begin + 0x4, ee_asg_unit **); }
internal inline void attack_subgroup_set_units_end(ee_attack_subgroup *sg, ee_asg_unit **e) { FIELD(sg, of_attack_subgroup_units_begin + 0x4, ee_asg_unit **) = e; }
// +0x60 the route, a deque of {x, y, ?} (attack.c route_waypoint); +0x70 its waypoints
// +0x100 the target: +4 x, +8 y, +0xC the unit's id, +0x10 its kind (1: a unit)
// its target record: +4 / +8 the place, +0xC the unit's id, +0x10 the kind (1 a unit)
internal inline int asg_target_get_id(const ee_asg_target *tg) { return FIELD(tg, 0xC, int); }
internal inline int asg_target_get_kind(const ee_asg_target *tg) { return FIELD(tg, 0x10, int); }
internal inline void asg_target_set_unit(ee_asg_target *tg, int x, int y, int id) {
    FIELD(tg, 4, int) = x; FIELD(tg, 8, int) = y;
    FIELD(tg, 0xC, int) = id;
    FIELD(tg, 0x10, int) = 1;
}

// A SUBGROUP'S UNIT RECORD (EECPAttackSubGroupUnit)
// what asg_unit_score scores: +0x4 the enemy

// AN ATTACK TASK (EECPAGTask; test/testcmd.c aigroup makes one): +0x20 the player it attacks, +0x28 / +0x2C / +0x30
// as the scenario effect "Create AI Military Group" sets them (7, 2, 1 - not known)
internal inline void task_set_unknown_28(ee_task *task, int v) { FIELD(task, of_task_0x28, int) = v; }
internal inline void task_set_unknown_2c(ee_task *task, int v) { FIELD(task, of_task_0x2c, int) = v; }
internal inline void task_set_unknown_30(ee_task *task, uint8_t v) { FIELD(task, of_task_0x30, uint8_t) = v; }
// the route planner (its own player's index +0x48) and its nodes (tile +0x3C, +0x40)
// an attack strategy (test/testcmd.c aigroup: as the scenario effect sets it, 4 and 0 - not known)
internal inline void strategy_set_unknown_3c(ee_strategy *s, int v) { FIELD(s, of_strategy_0x3c, int) = v; }
internal inline void strategy_set_unknown_40(ee_strategy *s, uint8_t v) { FIELD(s, of_strategy_0x40, uint8_t) = v; }

// FUNCTIONS
internal inline unsigned group_count(void *grp) { return ((unsigned (__thiscall *)(void *))(*(void ***)grp)[value_attack_slot_group_count])(grp); }
internal inline ee_unit *group_get(void *grp, unsigned i) { return ((ee_unit *(__thiscall *)(void *, unsigned))(*(void ***)grp)[value_attack_slot_group_get])(grp, i); }
// the combat manager (player +0xB8C) answers an attack by target (the game's revenge attack)

// a land attack subgroup (EECPASGLand, by its vtable: the subgroups of the other kinds have none of its fields)
internal inline int attack_subgroup_is_land(const ee_attack_subgroup *sg) { return *(const uintptr_t *)sg == data_attack_vt_asg_land; }

// AN ATTACK GROUP OF THE GAME'S OWN (test/testcmd.c aigroup: as the scenario effect "Create AI Military Group"):
// a task (0x34) with its location (0x18, the point at +4) and target, a strategy (0x44), the unit ids (a vector<int>
// in the game's heap) - the combat manager asked, then its command (cm +0x48) makes the group

// ATTACK GROUPS AND SUBGROUPS (the originals of attack.c's and landing.c's hooks)
// the units b..e sorted by distance to unit (the targets' range sort)
// a land subgroup's route from start to dest (int[2] each), its arrival (waypoints at +0x60)
// a subgroup member's score for an enemy (info: what it scores)
// the route planner's cost of an edge into node
internal inline int route_cost(ee_route_planner *planner, int *edge, ee_route_node *node) {
    return fn_route_cost_game(planner, edge, node);
}

// HOOKS
// ATTACK GROUPS AND SUBGROUPS (attack.c, forces.c, landing.c)
typedef void (__thiscall *asg_hook)(ee_attack_subgroup *sg);
// the subgroups' stats: the land subgroups' own, the others' (data_attack_asg_vtables)
internal inline int patch_asg_land_stats(asg_hook h) { return patch_vtable(data_attack_vt_asg_land, value_attack_slot_asg_stats, (uintptr_t)fn_attack_subgroup_land_stats, (void *)h); }
internal inline int patch_asg_stats(asg_hook h, int *of) { return patch_vtables(PATCH_LIST(data_attack_asg_vtables), value_attack_slot_asg_stats, (uintptr_t)fn_attack_subgroup_stats, (void *)h, of); }
typedef void (__thiscall *asg_update_hook)(ee_attack_subgroup *sg, int now);
internal inline int patch_asg_update(asg_update_hook h) { return patch_vtable(data_attack_vt_asg_land, value_attack_slot_asg_update, (uintptr_t)fn_attack_subgroup_update, (void *)h); }
internal inline int patch_asg_sea_update(asg_update_hook h) { return patch_vtable(data_attack_vt_asg_sea, value_attack_slot_asg_update, (uintptr_t)fn_attack_subgroup_sea_update, (void *)h); }
internal inline int patch_asg_sea_transport_update(asg_update_hook h) {
    return patch_vtable(data_attack_vt_asg_sea_transport, value_attack_slot_asg_update, (uintptr_t)fn_attack_subgroup_sea_transport_update, (void *)h);
}
internal inline int patch_asg_air_update(asg_update_hook h) { return patch_vtable(data_attack_vt_asg_air, value_attack_slot_asg_update, (uintptr_t)fn_attack_subgroup_air_update, (void *)h); }
// the subgroups' orders to a unit (vf57): sea transport, sea, land through h, air through h_air - of 4
typedef char (__thiscall *asg_give_goal_hook)(ee_attack_subgroup *sg, ee_unit *u, ee_goal *goal, int a, int prio);
internal inline int patch_asg_give_goal(asg_give_goal_hook h, asg_give_goal_hook h_air) {
    return patch_vtable(data_attack_vt_asg_sea_transport, value_attack_slot_asg_give_goal, (uintptr_t)fn_attack_subgroup_give_goal, (void *)h) +
           patch_vtable(data_attack_vt_asg_sea, value_attack_slot_asg_give_goal, (uintptr_t)fn_attack_subgroup_give_goal, (void *)h) +
           patch_vtable(data_attack_vt_asg_land, value_attack_slot_asg_give_goal, (uintptr_t)fn_attack_subgroup_give_goal, (void *)h) +
           patch_vtable(data_attack_vt_asg_air, value_attack_slot_asg_give_goal, (uintptr_t)fn_attack_subgroup_air_give_goal, (void *)h_air);
}
// a land subgroup's route and its arrival, the route planner's edge cost
typedef void (__thiscall *asg_route_hook)(ee_attack_subgroup *sg, int *start, int *dest);
internal inline int patch_asg_route(asg_route_hook h) { return patch_vtable(data_attack_vt_asg_land, value_attack_slot_asg_route, (uintptr_t)fn_asg_route, (void *)h); }
internal inline int patch_asg_route_done(asg_hook h) { return patch_vtable(data_attack_vt_asg_land, value_attack_slot_asg_route_done, (uintptr_t)fn_attack_subgroup_route_done, (void *)h); }
typedef int (__thiscall *route_cost_hook)(ee_route_planner *planner, int *edge, ee_route_node *node);
internal inline int patch_route_cost(route_cost_hook h) { return patch_vtable(data_attack_vt_route_planner, value_attack_slot_route_cost, (uintptr_t)fn_route_cost_game, (void *)h); }
// the land members' score of an enemy (base, hero, priests and prophets: data_attack_asgu_vtables)
typedef float (__thiscall *asgu_score_hook)(ee_asg_unit *member, ee_asgu_info *info);
internal inline int patch_asgu_score(asgu_score_hook h, int *of) {
    return patch_vtables(PATCH_LIST(data_attack_asgu_vtables), value_attack_slot_asgu_score, (uintptr_t)fn_asg_unit_score, (void *)h, of);
}
// the attack subgroups' pop of a waiting unit (asg_queue.c: a thunk, it calls game_asg_queue_pop) - the land and the air ones
internal inline int patch_asg_queue_pop(thunk_fn thunk, int *of) {
    const uint32_t sites[2] = { data_attack_call_asg_queue_pop, data_attack_call_asg_air_queue_pop };
    return patch_calls(sites, 2, data_attack_fn_asg_queue_pop, (void *)thunk, of);
}
internal inline uintptr_t game_asg_queue_pop(void) { return data_attack_fn_asg_queue_pop; }
