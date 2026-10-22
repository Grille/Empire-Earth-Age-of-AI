// THE AI'S ATTACKS (ee_combat_mgr, ee_attack_cmd, ee_attack_group, ee_attack_subgroup, ee_asg_unit; tasks,
// strategies, the route planner): fields, functions, hooks.

// FIELDS
// THE COMBAT MANAGER (EECPCombatManager, a computer player's +0xB8C) and its attack commander, which keeps the attack
// groups: a group has its subgroups (one per kind: land, sea, sea transport, air), a subgroup its unit records
internal inline ee_attack_cmd *am_get_commander(const ee_combat_mgr *am) { return FIELD(am, AM_COMMANDER, ee_attack_cmd *); } // +0x48
// the attention's activities 6 ([+0x4C] +0x48; the object at +0x4C not known) and 8 (+0x62)
internal inline uint8_t am_get_att_4c(const ee_combat_mgr *am) {
    uint8_t *o = FIELD(am, AM_0x4C, uint8_t *);
    return o ? o[AM4C_0x48] : 0;
}
internal inline uint8_t am_get_att_62(const ee_combat_mgr *am) { return FIELD(am, AM_0x62, uint8_t); }
// +0x4 the groups, a deque of ee_attack_group * (deque_at); +0x14 its count
internal inline uint8_t *ac_groups_at(const ee_attack_cmd *ac) { return (uint8_t *)FIELD_AT(ac, AMCMD_GROUPS); }
internal inline int ac_get_group_count(const ee_attack_cmd *ac) { return FIELD(ac, AMCMD_GROUPS + 0x10, int); }

// AN ATTACK GROUP (EECPAttackGroup)
internal inline ee_attack_subgroup **ag_get_subgroups_begin(const ee_attack_group *g) { return FIELD(g, AG_SUBGROUPS, ee_attack_subgroup **); } // +0x8
internal inline ee_attack_subgroup **ag_get_subgroups_end(const ee_attack_group *g) { return FIELD(g, AG_SUBGROUPS + 0x4, ee_attack_subgroup **); }
internal inline ee_unit *ag_get_target_unit(const ee_attack_group *g) { return FIELD(g, AG_TARGET_UNIT, ee_unit *); } // +0x44 (its task +0x28's +0x1C)
internal inline ee_player *ag_get_target(const ee_attack_group *g) { return FIELD(g, AG_TARGET, ee_player *); }  // +0x48 the player it attacks

// AN ATTACK SUBGROUP (EECPAttackSubGroup; EECPASGLand, asg_is_land, has the fields from +0x60 on)
internal inline ee_asg_unit **asg_get_units_begin(const ee_attack_subgroup *sg) { return FIELD(sg, ASG_UNITS, ee_asg_unit **); } // +0x54 its members
internal inline ee_asg_unit **asg_get_units_end(const ee_attack_subgroup *sg) { return FIELD(sg, ASG_UNITS + 0x4, ee_asg_unit **); }
internal inline void asg_set_units_end(ee_attack_subgroup *sg, ee_asg_unit **e) { FIELD(sg, ASG_UNITS + 0x4, ee_asg_unit **) = e; }
// +0x60 the route, a deque of {x, y, ?} (attack.c route_waypoint); +0x70 its waypoints
internal inline uint8_t *asg_route_at(const ee_attack_subgroup *sg) { return (uint8_t *)FIELD_AT(sg, ASG_ROUTE); }
internal inline unsigned asg_get_waypoints(const ee_attack_subgroup *sg) { return FIELD(sg, ASG_WAYPOINTS, unsigned); }
// +0x100 the target: +4 x, +8 y, +0xC the unit's id, +0x10 its kind (1: a unit)
internal inline uint8_t *asg_target_at(const ee_attack_subgroup *sg) { return (uint8_t *)FIELD_AT(sg, ASG_TARGET_REC); }
internal inline int asg_get_goal_x(const ee_attack_subgroup *sg) { return FIELD(sg, ASG_GOAL_X, int); }
internal inline int asg_get_goal_y(const ee_attack_subgroup *sg) { return FIELD(sg, ASG_GOAL_Y, int); }
internal inline ee_unit *asg_get_target_unit(const ee_attack_subgroup *sg) { return FIELD(sg, ASG_TARGET_UNIT, ee_unit *); } // +0x118
internal inline ee_player *asg_get_target(const ee_attack_subgroup *sg) { return FIELD(sg, ASG_TARGET, ee_player *); } // +0x11C the player it attacks
internal inline int asg_get_mission(const ee_attack_subgroup *sg) { return FIELD(sg, ASG_MISSION, int); }      // +0x124
internal inline ee_player *asg_get_player(const ee_attack_subgroup *sg) { return FIELD(sg, ASG_PLAYER, ee_player *); } // +0x190 its own
internal inline int asg_get_state(const ee_attack_subgroup *sg) { return FIELD(sg, ASG_STATE, int); }        // +0x1D0
internal inline int asg_get_x(const ee_attack_subgroup *sg) { return FIELD(sg, ASG_X, int); }            // +0x234 where it is
internal inline int asg_get_y(const ee_attack_subgroup *sg) { return FIELD(sg, ASG_Y, int); }

// A SUBGROUP'S UNIT RECORD (EECPAttackSubGroupUnit)
internal inline ee_unit *asgu_get_unit(const ee_asg_unit *r) { return FIELD(r, ASGU_UNIT, ee_unit *); }     // +0x44
internal inline void asgu_set_unit(ee_asg_unit *r, ee_unit *u) { FIELD(r, ASGU_UNIT, ee_unit *) = u; }
internal inline void *asgu_get_engaged(const ee_asg_unit *r) { return FIELD(r, ASGU_ENGAGED, void *); }       // +0x48 set while it fights
internal inline ee_attack_subgroup *asgu_get_subgroup(const ee_asg_unit *r) { return FIELD(r, ASGU_SUBGROUP, ee_attack_subgroup *); } // +0x4C
// what FN_ASGU_SCORE scores: +0x4 the enemy
internal inline ee_unit *score_info_get_enemy(const ee_asgu_info *info) { return FIELD(info, ASGUINFO_ENEMY, ee_unit *); }

// AN ATTACK TASK (EECPAGTask; test/testcmd.c aigroup makes one): +0x20 the player it attacks, +0x28 / +0x2C / +0x30
// as the scenario effect "Create AI Military Group" sets them (7, 2, 1 - not known)
internal inline void task_set_target_player(ee_task *task, ee_player *pl) { FIELD(task, TASK_TARGET_PLAYER, ee_player *) = pl; }
internal inline void task_set_unknown_28(ee_task *task, int v) { FIELD(task, TASK_0x28, int) = v; }
internal inline void task_set_unknown_2c(ee_task *task, int v) { FIELD(task, TASK_0x2C, int) = v; }
internal inline void task_set_unknown_30(ee_task *task, uint8_t v) { FIELD(task, TASK_0x30, uint8_t) = v; }
// the route planner (its own player's index +0x48) and its nodes (tile +0x3C, +0x40)
internal inline int planner_get_player_index(const ee_route_planner *rp) { return FIELD(rp, ROUTEPLANNER_PLAYER_INDEX, int); }
internal inline int route_node_get_x(const ee_route_node *n) { return FIELD(n, ROUTENODE_X, int); }
internal inline int route_node_get_y(const ee_route_node *n) { return FIELD(n, ROUTENODE_Y, int); }
// an attack strategy (test/testcmd.c aigroup: as the scenario effect sets it, 4 and 0 - not known)
internal inline void strategy_set_unknown_3c(ee_strategy *s, int v) { FIELD(s, STRATEGY_0x3C, int) = v; }
internal inline void strategy_set_unknown_40(ee_strategy *s, uint8_t v) { FIELD(s, STRATEGY_0x40, uint8_t) = v; }

// FUNCTIONS
internal inline unsigned group_count(void *grp) { return ((unsigned (__thiscall *)(void *))(*(void ***)grp)[GA(SLOT_GROUP_COUNT)])(grp); }
// the combat manager (player +0xB8C) answers an attack by target (the game's revenge attack)
internal inline void revenge(ee_combat_mgr *combat_mgr, ee_player *target) { GF(FN_REVENGE)(combat_mgr, target); }

// a land attack subgroup (EECPASGLand, by its vtable: the subgroups of the other kinds have none of its fields)
internal inline int asg_is_land(const ee_attack_subgroup *sg) { return *(const uintptr_t *)sg == GA(VT_ASG_LAND); }

// AN ATTACK GROUP OF THE GAME'S OWN (test/testcmd.c aigroup: as the scenario effect "Create AI Military Group"):
// a task (0x34) with its location (0x18, the point at +4) and target, a strategy (0x44), the unit ids (a vector<int>
// in the game's heap) - the combat manager asked, then its command (cm +0x48) makes the group
internal inline void task_init(void *task) { GF(FN_TASK_CTOR)(task); }
internal inline void task_free(void *task) { GF(FN_TASK_DTOR)(task); }
internal inline void task_set_target(void *task, ee_unit *t) { GF(FN_TASK_SET_TARGET)(task, t); }
internal inline void task_set_location(void *task_loc, void *loc) { GF(FN_TASK_SET_LOCATION)(task_loc, loc); }
internal inline void location_init(void *loc) { GF(FN_LOCATION_CTOR)(loc); }
internal inline void point_set(void *point, int *pt) { GF(FN_POINT_SET)(point, pt); }
internal inline void strategy_init(void *s) { GF(FN_STRATEGY_CTOR)(s); }
internal inline void strategy_reset(void *s) { GF(FN_STRATEGY_INIT)(s); }
internal inline void strategy_free(void *s) { GF(FN_STRATEGY_DTOR)(s); }
internal inline char attack_plan_ok(ee_combat_mgr *cm, void *task, void *strategy, void *ids) { return GF(FN_ATTACK_PLAN_OK)(cm, task, strategy, ids); }
internal inline ee_attack_group *attack_group_new(ee_attack_cmd *cmd, void *task, void *strategy, void *ids) { return (ee_attack_group *)GF(FN_ATTACK_GROUP_NEW)(cmd, task, strategy, ids); }

// ATTACK GROUPS AND SUBGROUPS (the originals of attack.c's and landing.c's hooks)
internal inline char unit_to_group(void *cmd, ee_unit *u) { return GF(FN_UNIT_TO_GROUP)(cmd, u); }
internal inline void group_stat(void *stats, ee_unit *u) { GF(FN_GROUP_STAT)(stats, u); }
// the units b..e sorted by distance to unit (the targets' range sort)
internal inline void range_sort(void **b, void **e, ee_unit *unit) { GF(FN_RANGE_SORT)(b, e, unit); }
internal inline void asg_update(ee_attack_subgroup *sg, int now) { GF(FN_ASG_UPDATE)(sg, now); }
internal inline void asg_sea_update(ee_attack_subgroup *sg, int now) { GF(FN_ASG_SEA_UPDATE)(sg, now); }
internal inline void asg_sea_transport_update(ee_attack_subgroup *sg, int now) { GF(FN_ASG_SEA_TRANSPORT_UPDATE)(sg, now); }
internal inline void asg_air_update(ee_attack_subgroup *sg, int now) { GF(FN_ASG_AIR_UPDATE)(sg, now); }
internal inline void asg_stats(ee_attack_subgroup *sg) { GF(FN_ASG_STATS)(sg); }
internal inline void asg_land_stats(ee_attack_subgroup *sg) { GF(FN_ASG_LAND_STATS)(sg); }
internal inline char asg_give_goal(ee_attack_subgroup *sg, ee_unit *u, ee_goal *goal, int a, int prio) { return GF(FN_ASG_GIVE_GOAL)(sg, u, goal, a, prio); }
internal inline char asg_air_give_goal(ee_attack_subgroup *sg, ee_unit *u, ee_goal *goal, int a, int prio) { return GF(FN_ASG_AIR_GIVE_GOAL)(sg, u, goal, a, prio); }
// a land subgroup's route from start to dest (int[2] each), its arrival (waypoints at +0x60)
internal inline void asg_route(ee_attack_subgroup *sg, int *start, int *dest) { ((void (__thiscall *)(ee_attack_subgroup *, int *, int *))GA(FN_ASG_ROUTE))(sg, start, dest); }
internal inline void asg_route_done(ee_attack_subgroup *sg) { ((void (__thiscall *)(ee_attack_subgroup *))GA(FN_ASG_ROUTE_DONE))(sg); }
// a subgroup member's score for an enemy (info: what it scores)
internal inline float asgu_score(ee_asg_unit *member, ee_asgu_info *info) { return ((float (__thiscall *)(ee_asg_unit *, ee_asgu_info *))GA(FN_ASGU_SCORE))(member, info); }
// the route planner's cost of an edge into node
internal inline int route_cost(ee_route_planner *planner, int *edge, ee_route_node *node) {
    return ((int (__thiscall *)(ee_route_planner *, int *, ee_route_node *))GA(FN_ROUTE_COST))(planner, edge, node);
}

// HOOKS
// ATTACK GROUPS AND SUBGROUPS (attack.c, forces.c, landing.c)
typedef char (__thiscall *unit_to_group_hook)(void *cmd, ee_unit *u);
internal inline int patch_unit_to_group(unit_to_group_hook h) { return patch_call(GA(CALL_UNIT_TO_GROUP), GA(FN_UNIT_TO_GROUP), (void *)h); }
typedef void (__thiscall *group_stat_hook)(void *stats, ee_unit *u);
internal inline int patch_group_stat(group_stat_hook h) { return patch_call(GA(CALL_GROUP_STAT), GA(FN_GROUP_STAT), (void *)h); }
typedef void (__cdecl *range_sort_hook)(void **b, void **e, ee_unit *unit);
internal inline int patch_range_sort(range_sort_hook h) { return patch_call(GA(CALL_RANGE_SORT), GA(FN_RANGE_SORT), (void *)h); }
typedef void (__thiscall *asg_hook)(ee_attack_subgroup *sg);
// the subgroups' stats: the land subgroups' own, the others' (ASG_VTABLES)
internal inline int patch_asg_land_stats(asg_hook h) { return patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_STATS), GA(FN_ASG_LAND_STATS), (void *)h); }
internal inline int patch_asg_stats(asg_hook h, int *of) { return patch_vtables(PATCH_LIST(ASG_VTABLES), GA(SLOT_ASG_STATS), GA(FN_ASG_STATS), (void *)h, of); }
typedef void (__thiscall *asg_update_hook)(ee_attack_subgroup *sg, int now);
internal inline int patch_asg_update(asg_update_hook h) { return patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_UPDATE), GA(FN_ASG_UPDATE), (void *)h); }
internal inline int patch_asg_sea_update(asg_update_hook h) { return patch_vtable(GA(VT_ASG_SEA), GA(SLOT_ASG_UPDATE), GA(FN_ASG_SEA_UPDATE), (void *)h); }
internal inline int patch_asg_sea_transport_update(asg_update_hook h) {
    return patch_vtable(GA(VT_ASG_SEA_TRANSPORT), GA(SLOT_ASG_UPDATE), GA(FN_ASG_SEA_TRANSPORT_UPDATE), (void *)h);
}
internal inline int patch_asg_air_update(asg_update_hook h) { return patch_vtable(GA(VT_ASG_AIR), GA(SLOT_ASG_UPDATE), GA(FN_ASG_AIR_UPDATE), (void *)h); }
// the subgroups' orders to a unit (vf57): sea transport, sea, land through h, air through h_air - of 4
typedef char (__thiscall *asg_give_goal_hook)(ee_attack_subgroup *sg, ee_unit *u, ee_goal *goal, int a, int prio);
internal inline int patch_asg_give_goal(asg_give_goal_hook h, asg_give_goal_hook h_air) {
    return patch_vtable(GA(VT_ASG_SEA_TRANSPORT), GA(SLOT_ASG_GIVE_GOAL), GA(FN_ASG_GIVE_GOAL), (void *)h) +
           patch_vtable(GA(VT_ASG_SEA), GA(SLOT_ASG_GIVE_GOAL), GA(FN_ASG_GIVE_GOAL), (void *)h) +
           patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_GIVE_GOAL), GA(FN_ASG_GIVE_GOAL), (void *)h) +
           patch_vtable(GA(VT_ASG_AIR), GA(SLOT_ASG_GIVE_GOAL), GA(FN_ASG_AIR_GIVE_GOAL), (void *)h_air);
}
// a land subgroup's route and its arrival, the route planner's edge cost
typedef void (__thiscall *asg_route_hook)(ee_attack_subgroup *sg, int *start, int *dest);
internal inline int patch_asg_route(asg_route_hook h) { return patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_ROUTE), GA(FN_ASG_ROUTE), (void *)h); }
internal inline int patch_asg_route_done(asg_hook h) { return patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_ROUTE_DONE), GA(FN_ASG_ROUTE_DONE), (void *)h); }
typedef int (__thiscall *route_cost_hook)(ee_route_planner *planner, int *edge, ee_route_node *node);
internal inline int patch_route_cost(route_cost_hook h) { return patch_vtable(GA(VT_ROUTE_PLANNER), GA(SLOT_ROUTE_COST), GA(FN_ROUTE_COST), (void *)h); }
// the land members' score of an enemy (base, hero, priests and prophets: ASGU_VTABLES)
typedef float (__thiscall *asgu_score_hook)(ee_asg_unit *member, ee_asgu_info *info);
internal inline int patch_asgu_score(asgu_score_hook h, int *of) {
    return patch_vtables(PATCH_LIST(ASGU_VTABLES), GA(SLOT_ASGU_SCORE), GA(FN_ASGU_SCORE), (void *)h, of);
}
// the attack subgroups' pop of a waiting unit (asg_queue.c: a thunk, it calls game_asg_queue_pop) - the land and the air ones
internal inline int patch_asg_queue_pop(thunk_fn thunk, int *of) {
    const uint32_t sites[2] = { GA(CALL_ASG_QUEUE_POP), GA(CALL_ASG_AIR_QUEUE_POP) };
    return patch_calls(sites, 2, GA(FN_ASG_QUEUE_POP), (void *)thunk, of);
}
internal inline uintptr_t game_asg_queue_pop(void) { return GA(FN_ASG_QUEUE_POP); }
