// A UNIT (ee_unit: EEUnit, EEComplexUnit - a unit, building, resource site or projectile on the map): its fields, the
// game's functions on it, its hooks.

// FIELDS
internal inline int unit_get_refs(const ee_unit *u) { return FIELD(u, UNIT_REFS, int); }                 // +0x4 references (release 0x4FCAD3)
internal inline void *unit_get_list_node(const ee_unit *u) { return FIELD(u, UNIT_LIST_NODE, void *); }       // +0x8 its node in the unit list {next, prev}
internal inline ee_unit_vis *unit_get_vis(const ee_unit *u) { return FIELD(u, UNIT_VIS, ee_unit_vis *); }      // +0x14 visibility: +0xC a uint16 per player, seen
internal inline ee_player *unit_get_owner(const ee_unit *u) { return FIELD(u, UNIT_OWNER, ee_player *); } // +0x18
internal inline int unit_get_x(const ee_unit *u) { return FIELD(u, UNIT_X, int); }                   // +0x1C tile x (-10000: off the map)
internal inline int unit_get_y(const ee_unit *u) { return FIELD(u, UNIT_Y, int); }                   // +0x20 tile y
// +0x24: not known - the test opponent keeps it beside the pointer, to tell a reused unit object from the one before
internal inline int unit_get_serial(const ee_unit *u) { return FIELD(u, UNIT_SERIAL, int); }
internal inline uint8_t unit_get_released(const ee_unit *u) { return FIELD(u, UNIT_RELEASED, uint8_t); }    // +0x2A back in the pool (vf12 sets it)
internal inline uint8_t unit_get_valid(const ee_unit *u) { return FIELD(u, UNIT_VALID, uint8_t); }       // +0x2B mIsValid
internal inline ee_unit_type *unit_get_type_data(const ee_unit *u) { return FIELD(u, UNIT_TYPE_DATA, ee_unit_type *); } // +0x2C its type's data (TD)
internal inline ee_unit *unit_get_physics(const ee_unit *u) { return FIELD(u, UNIT_PHYSICS, ee_unit *); }  // +0x30 (the game's dump: physics) - a unit too
internal inline ee_unit_stats *unit_get_stats(const ee_unit *u) { return FIELD(u, UNIT_STATS, ee_unit_stats *); }    // +0x34 its player's stats of the type (USTAT)
internal inline int unit_get_resource(const ee_unit *u) { return FIELD(u, UNIT_RESOURCE, int); }            // +0x38 a resource site's resource
internal inline int unit_get_hp(const ee_unit *u) { return FIELD(u, UNIT_HP, int); }                  // +0x3C hit points (> 0: alive)
internal inline int unit_get_amount(const ee_unit *u) { return FIELD(u, UNIT_AMOUNT, int); }              // +0x40 a resource site's amount left
internal inline int *unit_get_queue_begin(const ee_unit *u) { return FIELD(u, UNIT_QUEUE, int *); }      // +0x4C a building's queue
internal inline int *unit_get_queue_end(const ee_unit *u) { return FIELD(u, UNIT_QUEUE + 0x4, int *); }        // +0x50
internal inline int unit_get_id(const ee_unit *u) { return FIELD(u, UNIT_ID, int); }                  // +0x60 (lookup 0x4111F6)
// +0x68: not known - the AI's gatherer checks (econ.c, food.c) pass over a citizen where it is set
internal inline int unit_get_busy(const ee_unit *u) { return FIELD(u, UNIT_BUSY, int); }
// +0x130 its matrix (GETransformation): +0x134 / +0x144 / +0x154 the local y axis (facing), +0x13C / +0x14C / +0x15C
// the position in tiles
internal inline float unit_get_facing_x(const ee_unit *u) { return FIELD(u, UNIT_MATRIX + 0x4, float); }
internal inline float unit_get_facing_y(const ee_unit *u) { return FIELD(u, UNIT_MATRIX + 0x14, float); }
internal inline float unit_get_facing_z(const ee_unit *u) { return FIELD(u, UNIT_MATRIX + 0x24, float); }
internal inline float unit_get_pos_x(const ee_unit *u) { return FIELD(u, UNIT_MATRIX + 0xC, float); }
internal inline float unit_get_pos_y(const ee_unit *u) { return FIELD(u, UNIT_MATRIX + 0x1C, float); }
internal inline float unit_get_pos_z(const ee_unit *u) { return FIELD(u, UNIT_MATRIX + 0x2C, float); }
internal inline ee_unit_type *unit_get_attacker_type(const ee_unit *u) { return FIELD(u, UNIT_ATTACKER_TYPE, ee_unit_type *); } // +0x1C0 a projectile's: the attack's type data
// +0x1CC: the next of a chain from +0x30 - its last is what a BuildRepair goal holds (0x60BAD0)
internal inline ee_unit *unit_get_chain(const ee_unit *u) { return FIELD(u, UNIT_CHAIN, ee_unit *); }
internal inline ee_goal *unit_get_goal(const ee_unit *u) { return FIELD(u, UNIT_GOAL, ee_goal *); }   // +0x1EC its goal (GOAL)
internal inline ee_goal *unit_get_goal_object(const ee_unit *u) { return FIELD(u, UNIT_GOAL_OBJECT, ee_goal *); } // +0x1F0 the goal object (0x5ED24B casts it)
internal inline void *unit_get_behaviour(const ee_unit *u) { return FIELD(u, UNIT_BEHAVIOUR, void *); }    // +0x208 its behaviour (behaviour_table())
internal inline void unit_set_behaviour(ee_unit *u, void *b) { FIELD(u, UNIT_BEHAVIOUR, void *) = b; }
internal inline float unit_get_speed_min(const ee_unit *u) { return FIELD(u, UNIT_SPEED_MIN, float); }      // +0x21C the speed, at least the type's minimum
internal inline float unit_get_speed(const ee_unit *u) { return FIELD(u, UNIT_SPEED, float); }          // +0x238 tiles/s (0 standing)
internal inline int unit_get_producing(const ee_unit *u) { return FIELD(u, UNIT_PRODUCING, int); }          // +0x25C what a building makes now
internal inline int unit_get_flight_ms(const ee_unit *u) { return FIELD(u, UNIT_FLIGHT_MS, int); }          // +0x28C a missile's climb left (ms)
internal inline int unit_get_reload(const ee_unit *u) { return FIELD(u, UNIT_RELOAD, int); }            // +0x2A0 its own reload (ms, 0: its class's)
internal inline uint8_t unit_get_frozen(const ee_unit *u) { return FIELD(u, UNIT_FROZEN, uint8_t); }     // +0x33B frozen (the scenario effect)
internal inline uint8_t unit_get_finished(const ee_unit *u) { return FIELD(u, UNIT_FINISHED, uint8_t); }   // +0x340 on a building's +0x30: built
// +0x341: held by the AI (EEComputerPlayer vf19 sets it; cleared when the AI takes the unit for an attack)
internal inline uint8_t unit_get_ai_held(const ee_unit *u) { return FIELD(u, UNIT_AI_HELD, uint8_t); }

// FUNCTIONS
// the citizens working site (all players)
internal inline unsigned site_gatherers(ee_unit *site) { return GF(FN_SITE_GATHERERS)((void *)GA(G_GAME_STATE), site); }
// a shot's ballistic path from -> to for unit u (type: its live stats): reachable (pitch out); the aim at target
internal inline char ballistic(ee_unit *u, ee_unit_stats *type, float *from, float *to, float *pitch) {
    return ((char (__cdecl *)(ee_unit *, ee_unit_stats *, float *, float *, float *))GA(FN_BALLISTIC))(u, type, from, to, pitch);
}
internal inline void unit_aim(ee_unit *u, ee_unit **target, float *aim, void *zero) {
    ((void (__cdecl *)(ee_unit *, ee_unit **, float *, void *))GA(FN_AIM))(u, target, aim, zero);
}
// is the map object explored by pl (a site's +0x14: its map object)
internal inline char explored_by(ee_unit *site, ee_player *pl) { return GF(FN_EXPLORED)(FIELD(site, UNIT_VIS, void *), pl); }

// the unit says sound (an answer to an order: 0x1F51 "building")
internal inline void unit_say(ee_unit *u, int sound) { GF(FN_UNIT_SAY)(u, sound); }
// the unit's goal set (the unit takes it over); append: after its current one, prio: the order's priority
internal inline void unit_set_goal(ee_unit *u, ee_goal *goal, int append, int prio) {
    ((void (__thiscall *)(void *, void *, char, int))GA(FN_SET_GOAL))(u, goal, (char)append, prio);
}
// the goal given into one of the unit's goal slots (the original of landing.c's unload retry)
internal inline void unit_give_goal(ee_unit *u, ee_goal *goal, int slot) { GF(FN_GIVE_GOAL)(u, goal, slot); }
// the units aboard a transport or inside a building
internal inline int units_inside(ee_unit *u) { return GF(FN_UNITS_INSIDE)(u); }
// the distance between two units (tiles)
internal inline float unit_distance(ee_unit *a, ee_unit *b) { return GF(FN_UNIT_DISTANCE)(a, b); }
// the unit changed in place into the type of new_td, owner player index pi (defence.c: a line upgrade)
internal inline int unit_convert(ee_unit *u, int pi, ee_unit_type *new_td) { return GF(FN_UNIT_CONVERT)(u, pi, (uint32_t)new_td); }
// the unit taken out of the map's unit list, given back to the game's pool (the originals of attack.c's guards)
internal inline void unit_unlink(ee_unit *u) { GF(FN_UNIT_UNLINK)(u); }
internal inline void unit_release(ee_unit *u) { GF(FN_UNIT_RELEASE)(u); }

// HOOKS
// shots (aim.c): the aim of a unit's and a missile's shot
typedef void (__cdecl *aim_hook)(ee_unit *u, ee_unit **target, float *aim, void *zero);
internal inline int patch_aim(aim_hook h) { return patch_call(GA(CALL_AIM), GA(FN_AIM), (void *)h); }
internal inline int patch_missile_aim(aim_hook h) { return patch_call(GA(CALL_MISSILE_AIM), GA(FN_AIM), (void *)h); }

typedef void (__thiscall *set_goal_hook)(ee_unit *u, ee_goal *goal, int append, int prio);
// the game's repair orders (the handing-over 0x46DA88: attack.c refuses them under attack)
internal inline int patch_repair_goal(set_goal_hook h) { return patch_call(GA(CALL_REPAIR_GOAL), GA(FN_SET_GOAL), (void *)h); }
typedef void (__thiscall *give_goal_hook)(ee_unit *u, ee_goal *goal, int slot);
internal inline int patch_unload_retry_give(give_goal_hook h) { return patch_call(GA(CALL_UNLOAD_RETRY_GIVE), GA(FN_GIVE_GOAL), (void *)h); }
typedef void (__thiscall *unit_release_hook)(ee_unit *u);
internal inline int patch_unit_release(unit_release_hook h) { return patch_vtable(GA(VT_COMPLEX_UNIT), GA(SLOT_UNIT_RELEASE), GA(FN_UNIT_RELEASE), (void *)h); }
// the unit list's node deletions (operator delete of a node, the unit in ESI): the thunk ends in game_op_delete
internal inline int patch_unit_node_free(thunk_fn thunk, int *of) {
    const uint32_t sites[2] = { GA(CALL_NODE_DEL_1), GA(CALL_NODE_DEL_2) };
    return patch_calls(sites, 2, GA(FN_OP_DELETE), (void *)thunk, of);
}
internal inline uintptr_t game_op_delete(void) { return GA(FN_OP_DELETE); }
