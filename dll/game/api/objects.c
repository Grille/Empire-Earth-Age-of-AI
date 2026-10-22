// THE GAME'S OBJECTS' FIELDS, BY NAME: kind_get_x(obj) reads a field, kind_set_x(obj, v) writes one (only those the
// mod writes) - types.h's pointer types, the offsets per build from raw/fields.h (the 2017 update's offset beside each,
// as engine-notes.md and the decompile name them). Inlined: the machine code is FIELD's. Where a field's meaning
// isn't known, its comment says how the code uses it.

// A UNIT (EEUnit, EEComplexUnit): a unit, building, resource site or projectile on the map
internal inline int unit_get_refs(const ee_unit *u) { return FIELD(u, UNIT_REFS, int); }                 // +0x4 references (release 0x4FCAD3)
internal inline void *unit_get_list_node(const ee_unit *u) { return FIELD(u, UNIT_LIST_NODE, void *); }       // +0x8 its node in the unit list {next, prev}
internal inline uint8_t *unit_get_vis(const ee_unit *u) { return FIELD(u, UNIT_VIS, uint8_t *); }      // +0x14 visibility: +0xC a uint16 per player, seen
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

// A MAP TILE (the map's tile table, MAP +0x1816D4): its +0x10 is UNIT's
internal inline float tile_get_height(const void *tile) { return FIELD(tile, TILE_HEIGHT, float); }

// A UNIT'S GOAL (EEUnitGoal, a unit's +0x1EC; the kind by its vtable: goal_is_attack(g) ...): the fields of each kind,
// from +0x20 on (GOAL_ARGS) its own
internal inline uint8_t goal_get_state(const ee_goal *g) { return FIELD(g, GOAL_STATE, uint8_t); }        // +0x5 (aim.c: 2 while it attacks)
// a move goal (goal_is_location): where to
internal inline float goal_get_dest_x(const ee_goal *g) { return FIELD(g, GOAL_ARGS, float); }         // +0x20
internal inline float goal_get_dest_y(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x4, float); }         // +0x24
// an attack goal (goal_is_attack): the unit it attacks
internal inline ee_unit *goal_get_target(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x10, ee_unit *); }   // +0x30
// a gather goal (EEUGGather): the resource site
internal inline ee_unit *goal_get_site(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x18, ee_unit *); }     // +0x38
// a build or repair goal (EEUGBuildRepair, goal_is_build): the building
internal inline ee_unit *goal_get_building(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x1C, ee_unit *); } // +0x3C
// a load goal (goal_is_load): the transport or building to board
internal inline ee_unit *goal_get_holder(const ee_goal *g) { return FIELD(g, GOAL_HOLDER, ee_unit *); }   // +0x50
// an unload goal (goal_is_unload): the water tiles +0x24 / +0x2C, the point to unload at +0x34, the reach (squared)
internal inline int goal_get_water_x(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x4, int); }
internal inline int goal_get_water_y(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x8, int); }
internal inline int goal_get_water2_x(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0xC, int); }
internal inline int goal_get_water2_y(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x10, int); }
internal inline int goal_get_unload_x(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x14, int); }
internal inline int goal_get_unload_y(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x18, int); }
internal inline float goal_get_reach_sq(const ee_goal *g) { return FIELD(g, GOAL_ARGS + 0x1C, float); }

// A UNIT TYPE'S DATA (EEUnitClass, type_data(t), a unit's +0x2C): the same for every unit of the type
internal inline uint8_t *type_get_vtable(const ee_unit_type *td) { return FIELD(td, TD_VTABLE, uint8_t *); }
internal inline const char *type_get_name(const ee_unit_type *td) { return FIELD(td, TD_NAME, const char *); } // +0x1C its data name
internal inline ee_unit_type *type_get_projectile(const ee_unit_type *td) { return FIELD(td, TD_PROJECTILE, ee_unit_type *); } // +0x3FC what it fires
internal inline float type_get_min_range(const ee_unit_type *td) { return FIELD(td, TD_MIN_RANGE, float); }  // +0x41C
internal inline float type_get_range(const ee_unit_type *td) { return FIELD(td, TD_RANGE, float); }      // +0x420 base range
internal inline float type_get_area(const ee_unit_type *td) { return FIELD(td, TD_AREA, float); }       // +0x424 area effect
// +0x42C, +0x430: not known - aim.c's shot-interval report prints them beside +0x438
internal inline float type_get_unknown_42c(const ee_unit_type *td) { return FIELD(td, TD_0x42C, float); }
internal inline float type_get_unknown_430(const ee_unit_type *td) { return FIELD(td, TD_0x430, float); }
internal inline float type_get_accel(const ee_unit_type *td) { return FIELD(td, TD_ACCEL, float); }      // +0x438 base accel (a projectile's: its fall)
internal inline int type_get_family(const ee_unit_type *td) { return FIELD(td, TD_FAMILY, int); }         // +0x4B4 2 building, 29 fortress, 5/6 units ...
internal inline int type_get_id(const ee_unit_type *td) { return FIELD(td, TD_ID, int); }             // +0x4B8 the type (UT(...))
internal inline int type_get_damage_class(const ee_unit_type *td) { return FIELD(td, TD_DAMAGE_CLASS, int); }   // +0x4C0 the damage table's class
internal inline int type_get_category(const ee_unit_type *td) { return FIELD(td, TD_CATEGORY, int); }       // +0x4C4 as a target (the hit table's row)
internal inline int type_get_weapon(const ee_unit_type *td) { return FIELD(td, TD_WEAPON, int); }         // +0x4C8 ammo type (the hit table's weapon kind)
internal inline int type_get_tech(const ee_unit_type *td) { return FIELD(td, TD_TECH, int); }           // +0x4E0 the tech that makes it (its price)
internal inline int type_get_reload(const ee_unit_type *td) { return FIELD(td, TD_RELOAD, int); }         // +0x50C base reload time (ms)
internal inline int type_get_attack(const ee_unit_type *td) { return FIELD(td, TD_ATTACK, int); }         // +0x510 base attack
internal inline int type_get_transport_capacity(const ee_unit_type *td) { return FIELD(td, TD_TRANSPORT_CAPACITY, int); } // +0x54C (0 for soldiers)
internal inline int type_get_garrison_capacity(const ee_unit_type *td) { return FIELD(td, TD_GARRISON_CAPACITY, int); }  // +0x550
internal inline int type_get_hp(const ee_unit_type *td) { return FIELD(td, TD_HP, int); }             // +0x55C base hit points
internal inline int type_get_missile_delay(const ee_unit_type *td) { return FIELD(td, TD_MISSILE_DELAY, int); }  // +0x588 missile delay time
internal inline int type_get_size(const ee_unit_type *td) { return FIELD(td, TD_SIZE, int); }          // +0x58C its footprint (tiles a side)
internal inline uint8_t type_get_behaviours(const ee_unit_type *td) { return FIELD(td, TD_BEHAVIOURS, uint8_t); } // +0x621 takes a behaviour (stances)

// A UNIT TYPE'S VALUES FOR ONE PLAYER (a unit's +0x34, type_stats(pl, t)): the type's data with the player's upgrades
internal inline ee_unit_type *stat_get_type_data(const ee_unit_stats *st) { return FIELD(st, USTAT_TYPE_DATA, ee_unit_type *); } // +0x60
internal inline float stat_get_height(const ee_unit_stats *st) { return FIELD(st, USTAT_HEIGHT, float); }     // +0x98
internal inline float stat_get_range(const ee_unit_stats *st) { return FIELD(st, USTAT_RANGE, float); }      // +0x9C tiles
internal inline float stat_get_area(const ee_unit_stats *st) { return FIELD(st, USTAT_AREA, float); }       // +0xA0 the area its shots hit
internal inline float stat_get_speed(const ee_unit_stats *st) { return FIELD(st, USTAT_SPEED, float); }      // +0xAC tiles/s (0: cannot move)
internal inline float stat_get_shot_speed(const ee_unit_stats *st) { return FIELD(st, USTAT_SHOT_SPEED, float); } // +0xC0 its projectile's launch speed
internal inline float stat_get_attack(const ee_unit_stats *st) { return FIELD(st, USTAT_ATTACK, float); }     // +0xD8
// +0xE0 float[6] the armour against each damage class
internal inline float stat_get_armor(const ee_unit_stats *st, int c) { return FIELD(st, USTAT_ARMOR + c * 4, float); }
internal inline int stat_get_reload(const ee_unit_stats *st) { return FIELD(st, USTAT_RELOAD, int); }        // +0x138 ms between shots
internal inline int stat_get_hp(const ee_unit_stats *st) { return FIELD(st, USTAT_HP, int); }            // +0x144 its full hit points

// A PLAYER (EEPlayer, EEComputerPlayer; engine-notes.md "Players", the AI settings table's fields: ai/settings.c)
// +0x18 / +0x1C: a list of 4-byte entries - not known; the population check's log counts it
internal inline uint8_t *player_get_list_begin(const ee_player *pl) { return FIELD(pl, PL_LIST, uint8_t *); }
internal inline uint8_t *player_get_list_end(const ee_player *pl) { return FIELD(pl, PL_LIST + 0x4, uint8_t *); }
internal inline void *player_name_at(const ee_player *pl) { return (void *)FIELD_AT(pl, PL_NAME); }        // +0x24 its name (UWideString)
internal inline const char *player_get_name(const ee_player *pl) { return FIELD(pl, PL_NAME + 0x4, const char *); } // +0x28 the name's text
internal inline const void *player_color_at(const ee_player *pl) { return FIELD_AT(pl, PL_COLOR); }      // +0x438 its colour (RGB)
internal inline int player_get_index(const ee_player *pl) { return FIELD(pl, PL_INDEX, int); }          // +0x45C
internal inline int8_t player_get_msg_id(const ee_player *pl) { return FIELD(pl, PL_MSG_ID, int8_t); }  // +0x464 its id in a chat message (msg +0xA)
internal inline uint8_t player_get_explore_all(const ee_player *pl) { return FIELD(pl, PL_EXPLORE_ALL, uint8_t); } // +0x465 no fog
internal inline void player_set_explore_all(ee_player *pl, uint8_t v) { FIELD(pl, PL_EXPLORE_ALL, uint8_t) = v; }
internal inline uint8_t player_get_out(const ee_player *pl) { return FIELD(pl, PL_OUT, uint8_t); }    // +0x467 out of the game (resigned, defeated)
internal inline void *player_civ_name_at(const ee_player *pl) { return (void *)FIELD_AT(pl, PL_CIV_NAME); }  // +0x488 its civilization's name (UWideString)
internal inline uint8_t *player_stats_at(const ee_player *pl) { return (uint8_t *)FIELD_AT(pl, PL_STATS); } // +0x4A8 EEPlayerStatistics (PLSTATS)
internal inline int *player_get_selected_group(const ee_player *pl) { return FIELD(pl, PL_SELECTED_GROUP, int *); } // +0x9BC
internal inline ee_tech_tree *player_get_tech_tree(const ee_player *pl) { return FIELD(pl, PL_TECH_TREE, ee_tech_tree *); } // +0x9D4
// +0x9E0 whom its chat reaches: 0 everyone, 1 allies, 2 enemies, 3 the players marked at +0xB2C
internal inline int player_get_chat_to(const ee_player *pl) { return FIELD(pl, PL_CHAT_TO, int); }
internal inline void player_set_chat_to(ee_player *pl, int v) { FIELD(pl, PL_CHAT_TO, int) = v; }
internal inline int player_get_stance(const ee_player *pl, int i) { return FIELD(pl, PL_STANCE + i * 4, int); } // +0x9E4 [16] towards player i: 0 allied, 1 enemy
internal inline int player_get_strategy(const ee_player *pl) { return FIELD(pl, PL_STRATEGY, int); }        // +0xA28 orders the attention's activities
internal inline int player_get_result(const ee_player *pl) { return FIELD(pl, PL_RESULT, int); }         // +0xA30 1 won, 2 lost
internal inline int *player_stock_at(const ee_player *pl) { return (int *)FIELD_AT(pl, PL_STOCK); }     // +0xAF0 int[7]: 2 food, 3 wood, 4 stone, 5 gold, 6 iron
internal inline int player_get_stock(const ee_player *pl, int r) { return FIELD(pl, PL_STOCK + r * 4, int); }
// +0xB10 / +0xB1C / +0xB20: population counts - +0xB1C and +0xB20 in production (0x556E11), +0xB10 not known
internal inline int player_get_pop_b10(const ee_player *pl) { return FIELD(pl, PL_0xB10, int); }
internal inline int player_get_objects(const ee_player *pl) { return FIELD(pl, PL_OBJECTS, int); }        // +0xB14 its objects (< 1: out)
internal inline int player_get_pop_b1c(const ee_player *pl) { return FIELD(pl, PL_0xB1C, int); }
internal inline int player_get_pop_b20(const ee_player *pl) { return FIELD(pl, PL_0xB20, int); }
internal inline uint8_t player_get_chat_to_player(const ee_player *pl, int i) { return FIELD(pl, PL_CHAT_TO_PLAYER + i, uint8_t); } // +0xB2C [16]
internal inline void player_set_chat_to_player(ee_player *pl, int i, uint8_t v) { FIELD(pl, PL_CHAT_TO_PLAYER + i, uint8_t) = v; }
internal inline void player_set_civ_chosen(ee_player *pl, uint8_t v) { FIELD(pl, PL_CIV_CHOSEN, uint8_t) = v; } // +0xB46 (0: the civ dialog opens)
internal inline ee_zone **player_get_zones_begin(const ee_player *pl) { return FIELD(pl, PL_ZONES, ee_zone **); } // +0xB50 the resource zones (map-wide)
internal inline ee_zone **player_get_zones_end(const ee_player *pl) { return FIELD(pl, PL_ZONES + 0x4, ee_zone **); }
internal inline ee_combat_mgr *player_get_combat_mgr(const ee_player *pl) { return FIELD(pl, PL_COMBAT_MGR, ee_combat_mgr *); } // +0xB8C
internal inline ee_civ_mgr *player_get_civ_mgr(const ee_player *pl) { return FIELD(pl, PL_CIV_MGR, ee_civ_mgr *); } // +0xB90 a computer player's
internal inline uint8_t *player_get_b98(const ee_player *pl) { return FIELD(pl, PL_0xB98, uint8_t *); }    // +0xB98 (PLB98: not known)
internal inline int player_get_difficulty(const ee_player *pl) { return FIELD(pl, PL_DIFFICULTY, int); }      // +0xBA4 0 easy, 1 medium, 2 hard
internal inline void player_set_difficulty(ee_player *pl, int v) { FIELD(pl, PL_DIFFICULTY, int) = v; }
// the price percent with Cheat (AdjustCost 0x430FBD): easy +0xD48 (int), medium +0xBB4 (float), hard +0xD4C (int)
internal inline float player_get_cost_medium(const ee_player *pl) { return FIELD(pl, PL_COST_MEDIUM, float); }
internal inline void player_set_cost_medium(ee_player *pl, float v) { FIELD(pl, PL_COST_MEDIUM, float) = v; }
internal inline int player_get_cost_easy(const ee_player *pl) { return FIELD(pl, PL_COST_EASY, int); }
internal inline void player_set_cost_easy(ee_player *pl, int v) { FIELD(pl, PL_COST_EASY, int) = v; }
internal inline int player_get_cost_hard(const ee_player *pl) { return FIELD(pl, PL_COST_HARD, int); }
internal inline void player_set_cost_hard(ee_player *pl, int v) { FIELD(pl, PL_COST_HARD, int) = v; }
internal inline float player_get_citizen_share(const ee_player *pl) { return FIELD(pl, PL_CITIZEN_SHARE, float); } // +0xBDC of the cap (0.45)
internal inline void player_set_citizen_share(ee_player *pl, float v) { FIELD(pl, PL_CITIZEN_SHARE, float) = v; }
internal inline int player_get_pop_cap(const ee_player *pl) { return FIELD(pl, PL_POP_CAP, int); }         // +0xCCC its own cap (vf31)
internal inline void player_set_pop_cap(ee_player *pl, int v) { FIELD(pl, PL_POP_CAP, int) = v; }
// attacks (the AI settings table, 0x431697): chances in percent, minutes, the next attack's game time
internal inline void player_set_attack_on_contact(ee_player *pl, int v) { FIELD(pl, PL_ATTACK_ON_CONTACT, int) = v; }   // +0xCD8
internal inline void player_set_attack_for_resources(ee_player *pl, int v) { FIELD(pl, PL_ATTACK_FOR_RESOURCES, int) = v; } // +0xCDC
internal inline void player_set_attack_revenge(ee_player *pl, int v) { FIELD(pl, PL_ATTACK_REVENGE, int) = v; }      // +0xCE0
internal inline void player_set_attack_max(ee_player *pl, int v) { FIELD(pl, PL_ATTACK_MAX, int) = v; }          // +0xCFC between attacks
internal inline void player_set_attack_min(ee_player *pl, int v) { FIELD(pl, PL_ATTACK_MIN, int) = v; }          // +0xD00
internal inline void player_set_first_attack_max(ee_player *pl, int v) { FIELD(pl, PL_FIRST_ATTACK_MAX, int) = v; }    // +0xD28
internal inline void player_set_first_attack_min(ee_player *pl, int v) { FIELD(pl, PL_FIRST_ATTACK_MIN, int) = v; }    // +0xD2C
internal inline void player_set_epoch_min(ee_player *pl, int v) { FIELD(pl, PL_EPOCH_MIN, int) = v; }           // +0xD58 the epoch timer's minutes
internal inline void player_set_epoch_max(ee_player *pl, int v) { FIELD(pl, PL_EPOCH_MAX, int) = v; }           // +0xD5C
internal inline int player_get_dropoff_distance(const ee_player *pl) { return FIELD(pl, PL_DROPOFF_DISTANCE, int); }   // +0xE10
internal inline int player_get_attention(const ee_player *pl) { return FIELD(pl, PL_ATTENTION, int); }          // +0xE20 activities a period (+1)
internal inline uint32_t player_get_next_attack(const ee_player *pl) { return FIELD(pl, PL_NEXT_ATTACK, uint32_t); } // +0xED8 game time
internal inline void player_set_next_attack(ee_player *pl, uint32_t v) { FIELD(pl, PL_NEXT_ATTACK, uint32_t) = v; }
// +0xEF4.. the AI toggles: Angreifen EF4, Aktiv EF9, Cheat EFB, Bauen EFC, Erkunden EFD, Sammeln EFF, Verteidigen F00,
// Ignorieren F02 - and +0xEF5 first attack done, +0xEFA boats instead of fields (a water map with fish)
internal inline uint8_t player_get_toggle(const ee_player *pl, int i) { return FIELD(pl, PL_TOGGLES + i, uint8_t); }
internal inline uint8_t player_get_first_attack_done(const ee_player *pl) { return FIELD(pl, PL_FIRST_ATTACK_DONE, uint8_t); }
internal inline uint8_t player_get_boats_for_food(const ee_player *pl) { return FIELD(pl, PL_BOATS_FOR_FOOD, uint8_t); }
internal inline uint8_t player_get_cheat(const ee_player *pl) { return FIELD(pl, PL_CHEAT, uint8_t); }
internal inline void player_set_cheat(ee_player *pl, uint8_t v) { FIELD(pl, PL_CHEAT, uint8_t) = v; }
internal inline uint8_t player_get_build(const ee_player *pl) { return FIELD(pl, PL_BUILD, uint8_t); }
internal inline void player_set_ignore(ee_player *pl, uint8_t v) { FIELD(pl, PL_IGNORE, uint8_t) = v; }

// A PLAYER'S TECH TREE (EETechTree, player_get_tech_tree(pl))
internal inline int tt_get_player_index(const ee_tech_tree *tt) { return FIELD(tt, TT_PLAYER_INDEX, int); }     // +0x4
internal inline int tt_get_epoch(const ee_tech_tree *tt) { return FIELD(tt, TT_EPOCH, int); }          // +0x4E4 the current epoch (0 = Prehistoric)
// +0x8 the epoch records, 0x54 apart (Art of Conquest's epoch field lies one record further: 0x538)
internal inline uint8_t *tt_epoch_rec_at(const ee_tech_tree *tt, int e) { return (uint8_t *)FIELD_AT(tt, TT_EPOCH_RECS + e * 0x54); }
// an epoch record: +0x8 the epoch's tech's data, the new things the epoch needs +0x40 and has +0x48 (0x5BD681)
internal inline ee_tech_data *ttrec_get_tech_data(const uint8_t *rec) { return FIELD(rec, TTREC_TECH_DATA, ee_tech_data *); }
internal inline int ttrec_get_needed(const uint8_t *rec) { return FIELD(rec, TTREC_NEEDED, int); }
internal inline int ttrec_get_done(const uint8_t *rec) { return FIELD(rec, TTREC_DONE, int); }

// A TECH (EETechTreeNode, tech_lookup(tt, id); an epoch's node, its children: game/tech.c)
internal inline uint8_t tech_get_researched(const ee_tech *t) { return FIELD(t, TECH_RESEARCHED, uint8_t); }   // +0x4 (vf6 sets it)
internal inline uint8_t *tech_researched_at(const ee_tech *t) { return (uint8_t *)FIELD_AT(t, TECH_RESEARCHED); }
internal inline uint8_t tech_get_available(const ee_tech *t) { return FIELD(t, TECH_AVAILABLE, uint8_t); }    // +0x6 (vf1, vf6 test it)
internal inline ee_tech_data *tech_get_data(const ee_tech *t) { return FIELD(t, TECH_DATA, ee_tech_data *); } // +0x8
internal inline ee_tech_tree *tech_get_tree(const ee_tech *t) { return FIELD(t, TECHNODE_TREE, ee_tech_tree *); } // +0xC its tree
internal inline int tech_get_epoch(const ee_tech *t) { return FIELD(t, TECH_EPOCH, int); }               // +0x14
// an epoch node's children (game/tech.c): the 2017 update's vector +0x24 .. +0x28, Art of Conquest's map head +0x28
internal inline uint8_t *tech_get_kids_begin(const ee_tech *t) { return FIELD(t, TECHNODE_KIDS, uint8_t *); }
internal inline uint8_t *tech_get_kids_end(const ee_tech *t) { return FIELD(t, TECHNODE_KIDS + 0x4, uint8_t *); }

// A TECH'S DATA (tech_get_data(t): the same for every player)
// +0x10 .. +0x14 the unit types that make it (its buildings)
internal inline int *techdata_get_makers_begin(const ee_tech_data *d) { return FIELD(d, TECHDATA_MAKERS, int *); }
internal inline int *techdata_get_makers_end(const ee_tech_data *d) { return FIELD(d, TECHDATA_MAKERS + 0x4, int *); }
// +0x24 its cost by resource r (2 food .. 6 iron)
internal inline int techdata_get_cost(const ee_tech_data *d, int r) { return FIELD(d, TECHDATA_COSTS + (r - 2) * 4, int); }
internal inline int techdata_get_category(const ee_tech_data *d) { return FIELD(d, TECHDATA_CATEGORY, int); } // +0x48 (7: a line upgrade)
internal inline int techdata_get_id(const ee_tech_data *d) { return FIELD(d, TECHDATA_ID, int); }      // +0x4C
internal inline int techdata_get_effects(const ee_tech_data *d) { return FIELD(d, TECHDATA_EFFECTS, int); } // +0x50 its effect list (effect_list(i))
internal inline int techdata_get_unit_type(const ee_tech_data *d) { return FIELD(d, TECHDATA_UNIT_TYPE, int); } // +0x6C the unit type it makes (< 0: a tech)
// +0x7D it counts as one of an epoch's new things (EETechTree::vf4)
internal inline uint8_t techdata_get_counts(const ee_tech_data *d) { return FIELD(d, TECHDATA_COUNTS, uint8_t); }
internal inline void techdata_set_counts(ee_tech_data *d, uint8_t v) { FIELD(d, TECHDATA_COUNTS, uint8_t) = v; }
// +0x81 Art of Conquest: granted by its epoch's line upgrades (only_new)
internal inline uint8_t techdata_get_aoc_new(const ee_tech_data *d) { return FIELD(d, TECHDATA_AOC_NEW, uint8_t); }

// AN EFFECT LIST (G_EPOCH_TECHS[i], a tech's techdata_get_effects): +0x4 the effect ids, +0x8 their count
internal inline unsigned *effects_get_ids(const uint8_t *list) { return FIELD(list, EPOCHTECHS_IDS, unsigned *); }
internal inline unsigned effects_get_count(const uint8_t *list) { return FIELD(list, EPOCHTECHS_COUNT, unsigned); }

// A CIVILIZATION MANAGER (EECPCivilizationManager, a computer player's +0xB90; Update 0x46E2C7): the AI's managers
internal inline uint8_t *cm_get_plan(const ee_civ_mgr *cm) { return FIELD(cm, CM_PLAN, uint8_t *); }     // +0x8 the build plan's list
internal inline ee_resource_mgr *cm_get_resource_mgr(const ee_civ_mgr *cm) { return FIELD(cm, CM_RESOURCE_MGR, ee_resource_mgr *); } // +0x38
internal inline ee_build_mgr *cm_get_build_mgr(const ee_civ_mgr *cm) { return FIELD(cm, CM_BUILD_MGR, ee_build_mgr *); }          // +0x3C
internal inline ee_research_mgr *cm_get_research_mgr(const ee_civ_mgr *cm) { return FIELD(cm, CM_RESEARCH_MGR, ee_research_mgr *); } // +0x40
internal inline ee_unit_mgr *cm_get_unit_mgr(const ee_civ_mgr *cm) { return FIELD(cm, CM_UNIT_MGR, ee_unit_mgr *); }             // +0x44
internal inline void *cm_get_wall_mgr(const ee_civ_mgr *cm) { return FIELD(cm, CM_WALL_MGR, void *); }       // +0x48 EECPWallManager
internal inline ee_player *cm_get_player(const ee_civ_mgr *cm) { return FIELD(cm, CM_PLAYER, ee_player *); } // +0x4C its player
internal inline void *cm_get_project(const ee_civ_mgr *cm) { return FIELD(cm, CM_PROJECT, void *); }        // +0x50 the construction project (NULL: a new one may start)
// +0x58 uint32_t[CM_TIMERS_N] game times: 0 next epoch, 1 building count check, 2 military buildings (31 s), 3 build
// (0x46E51E), 4 (0x46E817), 5 (59 s)
internal inline uint32_t *cm_timers_at(const ee_civ_mgr *cm) { return (uint32_t *)FIELD_AT(cm, CM_TIMERS); }
internal inline uint32_t cm_get_timer(const ee_civ_mgr *cm, int i) { return FIELD(cm, CM_TIMERS + i * 4, uint32_t); }
internal inline void cm_set_timer(ee_civ_mgr *cm, int i, uint32_t v) { FIELD(cm, CM_TIMERS + i * 4, uint32_t) = v; }
// +0x74.. flag bytes (CM_FLAGS_LEN): +0x75 the one-time setup done, +0x76 a request for the epoch's techs is out, the
// attention's activities +0x7A (0: request the epoch's techs), +0x7B (4: the build manager's pass), +0x7C (5); +0x78 not
// known
internal inline uint8_t cm_get_flag(const ee_civ_mgr *cm, int i) { return FIELD(cm, CM_FLAG + i, uint8_t); }
internal inline uint8_t *cm_setup_done_at(const ee_civ_mgr *cm) { return (uint8_t *)FIELD_AT(cm, CM_SETUP_DONE); }
internal inline uint8_t cm_get_request_out(const ee_civ_mgr *cm) { return FIELD(cm, CM_REQUEST_OUT, uint8_t); }
internal inline void cm_set_request_out(ee_civ_mgr *cm, uint8_t v) { FIELD(cm, CM_REQUEST_OUT, uint8_t) = v; }
internal inline uint8_t cm_get_unknown_78(const ee_civ_mgr *cm) { return FIELD(cm, CM_0x78, uint8_t); }
internal inline uint8_t cm_get_att_epoch_techs(const ee_civ_mgr *cm) { return FIELD(cm, CM_ATT_EPOCH_TECHS, uint8_t); }
internal inline uint8_t cm_get_att_build(const ee_civ_mgr *cm) { return FIELD(cm, CM_ATT_BUILD, uint8_t); }
internal inline uint8_t cm_get_att_7c(const ee_civ_mgr *cm) { return FIELD(cm, CM_0x7C, uint8_t); }

// A RESOURCE MANAGER (EECPResourceManager, CM +0x38; constructor/serializer 0x489F76 / 0x48C5B2)
internal inline void **rm_get_hunt_begin(const ee_resource_mgr *rm) { return FIELD(rm, RM_HUNT, void **); } // +0x88 the animals to hunt
internal inline void **rm_get_hunt_end(const ee_resource_mgr *rm) { return FIELD(rm, RM_HUNT + 0x4, void **); }
internal inline void **rm_get_fish_begin(const ee_resource_mgr *rm) { return FIELD(rm, RM_FISH, void **); } // +0x98 the fish
internal inline void **rm_get_fish_end(const ee_resource_mgr *rm) { return FIELD(rm, RM_FISH + 0x4, void **); }
// +0xA4 7 unit lists by resource (2 food .. 6 iron), deques 0x14 apart: their gatherers
internal inline uint8_t *rm_list_at(const ee_resource_mgr *rm, int r) { return (uint8_t *)FIELD_AT(rm, RM_LISTS + r * 0x14); }
internal inline ee_civ_mgr *rm_get_owner(const ee_resource_mgr *rm) { return FIELD(rm, RM_OWNER, ee_civ_mgr *); } // +0x130
internal inline int *rm_demand_at(const ee_resource_mgr *rm) { return (int *)FIELD_AT(rm, RM_DEMAND); }   // +0x134 int[7] still needed
internal inline int rm_get_demand(const ee_resource_mgr *rm, int r) { return FIELD(rm, RM_DEMAND + r * 4, int); }
internal inline uint32_t *rm_target_at(const ee_resource_mgr *rm) { return (uint32_t *)FIELD_AT(rm, RM_TARGET); } // +0x150 uint[7] gatherers wanted
internal inline uint32_t rm_get_target(const ee_resource_mgr *rm, int r) { return FIELD(rm, RM_TARGET + r * 4, uint32_t); }
internal inline uint32_t rm_get_timer(const ee_resource_mgr *rm, int i) { return FIELD(rm, RM_TIMERS + i * 4, uint32_t); } // +0x16C uint[5] next runs (game time)
internal inline uint8_t rm_get_initialized(const ee_resource_mgr *rm) { return FIELD(rm, RM_INITIALIZED, uint8_t); } // +0x187 (the timers reset while 0)
internal inline uint8_t rm_get_att_188(const ee_resource_mgr *rm) { return FIELD(rm, RM_0x188, uint8_t); }  // +0x188 the attention's activity 1

// A BUILD MANAGER (EECPBuildManager, CM +0x3C; Update 0x468540)
// +0x4 14 queues by epoch, deques 0x14 apart of {type, x, y}; the placement area follows at +0x11C
internal inline uint8_t *bm_queue_at(const ee_build_mgr *bm, int e) { return (uint8_t *)FIELD_AT(bm, BM_QUEUES + e * 0x14); }
internal inline int bm_get_base_x(const ee_build_mgr *bm) { return FIELD(bm, BM_BASE_X, int); }         // +0x12C its base (-10000: none yet)
internal inline int bm_get_base_y(const ee_build_mgr *bm) { return FIELD(bm, BM_BASE_Y, int); }         // +0x130
internal inline ee_civ_mgr *bm_get_owner(const ee_build_mgr *bm) { return FIELD(bm, BM_OWNER, ee_civ_mgr *); } // +0x134
internal inline ee_player *bm_get_player(const ee_build_mgr *bm) { return FIELD(bm, BM_PLAYER, ee_player *); }  // +0x138
internal inline uint8_t *bm_initialized_at(const ee_build_mgr *bm) { return (uint8_t *)FIELD_AT(bm, BM_INITIALIZED); } // +0x148 the build lists set up (0x465E4F while 0)

// A UNIT MANAGER (EECPUnitManager, CM +0x44; Update 0x490AD6)
// +0xC 14 request lists, deques 0x14 apart of 16-byte entries {type, count, ..}
internal inline uint8_t *um_requests_at(const ee_unit_mgr *um, int i) { return (uint8_t *)FIELD_AT(um, UM_REQUESTS + i * 0x14); }
internal inline ee_player *um_get_player(const ee_unit_mgr *um) { return FIELD(um, UM_PLAYER, ee_player *); } // +0x134
internal inline int um_get_in_production(const ee_unit_mgr *um) { return FIELD(um, UM_IN_PRODUCTION, int); }   // +0x13C units in production (0x49014F adds)
internal inline uint8_t um_get_att_140(const ee_unit_mgr *um) { return FIELD(um, UM_0x140, uint8_t); }  // +0x140 the attention's activity 3

// A RESEARCH MANAGER (EECPResearchManager, CM +0x40)
internal inline int *rs_get_pool_begin(const ee_research_mgr *rs) { return FIELD(rs, RS_POOL, int *); }  // +0x8 the techs it may research (ids)
internal inline int *rs_get_pool_end(const ee_research_mgr *rs) { return FIELD(rs, RS_POOL + 0x4, int *); }
internal inline void rs_set_pool_end(ee_research_mgr *rs, int *e) { FIELD(rs, RS_POOL + 0x4, int *) = e; }
// +0x18 14 lists by epoch, 0x10 apart: {begin, end} of tech ids
internal inline int *rs_get_epoch_begin(const ee_research_mgr *rs, int e) { return FIELD(rs, RS_EPOCH + e * 0x10, int *); }
internal inline int *rs_get_epoch_end(const ee_research_mgr *rs, int e) { return FIELD(rs, RS_EPOCH + 0x4 + e * 0x10, int *); }
internal inline int rs_get_current(const ee_research_mgr *rs) { return FIELD(rs, RS_CURRENT, int); }      // +0x114 what it researches now
internal inline uint8_t *rs_initialized_at(const ee_research_mgr *rs) { return (uint8_t *)FIELD_AT(rs, RS_INITIALIZED); } // +0x118 the schedule set up (while 0)
internal inline uint8_t rs_get_att_random(const ee_research_mgr *rs) { return FIELD(rs, RS_ATT_RANDOM, uint8_t); } // +0x11A the attention's activity 2 (random research)

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
internal inline ee_unit *score_info_get_enemy(const void *info) { return FIELD(info, ASGUINFO_ENEMY, ee_unit *); }

// AN ATTACK TASK (EECPAGTask; test/testcmd.c aigroup makes one): +0x20 the player it attacks, +0x28 / +0x2C / +0x30
// as the scenario effect "Create AI Military Group" sets them (7, 2, 1 - not known)
internal inline void task_set_target_player(void *task, ee_player *pl) { FIELD(task, TASK_TARGET_PLAYER, ee_player *) = pl; }
internal inline void task_set_unknown_28(void *task, int v) { FIELD(task, TASK_0x28, int) = v; }
internal inline void task_set_unknown_2c(void *task, int v) { FIELD(task, TASK_0x2C, int) = v; }
internal inline void task_set_unknown_30(void *task, uint8_t v) { FIELD(task, TASK_0x30, uint8_t) = v; }

// AN AI ZONE (a town or resource zone, player_get_zones_begin(pl)): its resource sites, whether it has a drop site, active
internal inline void **zone_get_sites_begin(const ee_zone *z) { return FIELD(z, ZONE_SITES, void **); }   // +0x8 .. +0xC its sites (ee_unit *)
internal inline void **zone_get_sites_end(const ee_zone *z) { return FIELD(z, ZONE_SITES + 0x4, void **); }
internal inline int zone_get_drop(const ee_zone *z) { return FIELD(z, ZONE_DROP, int); }                 // +0x14 a drop site serves it
internal inline uint8_t zone_get_active(const ee_zone *z) { return FIELD(z, ZONE_ACTIVE, uint8_t); }       // +0x18

// THE SCENARIO MANAGER AND ITS TRIGGERS (EEScenarioManager, scenario_manager(); test/scen.c, engine-notes.md
// "Scenario triggers"): the lists are vectors (begin, end) - the address of each list's begin
internal inline uint8_t *scen_triggers_at(const uint8_t *m) { return (uint8_t *)FIELD_AT(m, SCEN_TRIGGERS); }
internal inline uint8_t *scen_conds_at(const uint8_t *m) { return (uint8_t *)FIELD_AT(m, SCEN_CONDS); }
internal inline uint8_t *scen_effects_at(const uint8_t *m) { return (uint8_t *)FIELD_AT(m, SCEN_EFFECTS); }
internal inline uint8_t *scen_objects_at(const uint8_t *m) { return (uint8_t *)FIELD_AT(m, SCEN_OBJECTS); }
internal inline uint8_t *scen_areas_at(const uint8_t *m) { return (uint8_t *)FIELD_AT(m, SCEN_AREAS); }
internal inline float scen_get_time(const uint8_t *m) { return FIELD(m, SCEN_TIME, float); }            // +0x210 ms
// a trigger (EESMTrigger): its name and description (UWideString), the lists of its condition and effect references
internal inline uint8_t *trigger_name_at(const uint8_t *t) { return (uint8_t *)FIELD_AT(t, TRIGGER_NAME); }
internal inline uint8_t *trigger_desc_at(const uint8_t *t) { return (uint8_t *)FIELD_AT(t, TRIGGER_DESC); }
internal inline uint8_t *trigger_conds_at(const uint8_t *t) { return (uint8_t *)FIELD_AT(t, TRIGGER_CONDS); }
internal inline uint8_t *trigger_effects_at(const uint8_t *t) { return (uint8_t *)FIELD_AT(t, TRIGGER_EFFECTS); }
internal inline int trigger_get_true_for(const uint8_t *t) { return FIELD(t, TRIGGER_TRUE_FOR, int); }          // +0x4C s the conditions held
internal inline void trigger_set_since(uint8_t *t, int v) { FIELD(t, TRIGGER_SINCE, int) = v; }
internal inline char trigger_get_on(const uint8_t *t) { return FIELD(t, TRIGGER_ON, char); }
internal inline void trigger_set_on(uint8_t *t, char v) { FIELD(t, TRIGGER_ON, char) = v; }
internal inline char trigger_get_fired(const uint8_t *t) { return FIELD(t, TRIGGER_FIRED, char); }
internal inline void trigger_set_fired(uint8_t *t, char v) { FIELD(t, TRIGGER_FIRED, char) = v; }
internal inline char trigger_get_loop(const uint8_t *t) { return FIELD(t, TRIGGER_LOOP, char); }
// a trigger's condition reference: the condition's index, the join to the next (0 end, 1 and, 2 or), not
internal inline int trigger_cond_get_index(const uint8_t *w) { return FIELD(w, TRIGCOND_INDEX, int); }
internal inline int trigger_cond_get_join(const uint8_t *w) { return FIELD(w, TRIGCOND_JOIN, int); }
internal inline char trigger_cond_get_not(const uint8_t *w) { return FIELD(w, TRIGCOND_NOT, char); }
// a trigger's effect reference: its kind (2: "or" between random groups), delay (s), the effect's index
internal inline int trigger_effect_get_kind(const uint8_t *w) { return FIELD(w, TRIGEFFECT_KIND, int); }
internal inline int trigger_effect_get_delay(const uint8_t *w) { return FIELD(w, TRIGEFFECT_DELAY, int); }
internal inline int trigger_effect_get_index(const uint8_t *w) { return FIELD(w, TRIGEFFECT_INDEX, int); }
// a condition: its type (0 always .. 7 chat) and each type's values
internal inline uint8_t *scen_cond_text_at(const uint8_t *c) { return (uint8_t *)FIELD_AT(c, SCENCOND_TEXT); } // chat: the text
internal inline int scen_cond_get_type(const uint8_t *c) { return FIELD(c, SCENCOND_TYPE, int); }
internal inline int scen_cond_get_op(const uint8_t *c) { return FIELD(c, SCENCOND_OP, int); }
internal inline int scen_cond_get_attr(const uint8_t *c) { return FIELD(c, SCENCOND_ATTR, int); }
internal inline int scen_cond_get_tech(const uint8_t *c) { return FIELD(c, SCENCOND_TECH, int); }
internal inline int scen_cond_get_value(const uint8_t *c) { return FIELD(c, SCENCOND_VALUE, int); }
internal inline int scen_cond_get_object(const uint8_t *c) { return FIELD(c, SCENCOND_OBJECT, int); }
internal inline int scen_cond_get_player(const uint8_t *c) { return FIELD(c, SCENCOND_PLAYER, int); }
internal inline int scen_cond_get_time_from(const uint8_t *c) { return FIELD(c, SCENCOND_TIME_FROM, int); }
internal inline int scen_cond_get_time_to(const uint8_t *c) { return FIELD(c, SCENCOND_TIME_TO, int); }
internal inline int scen_cond_get_trigger(const uint8_t *c) { return FIELD(c, SCENCOND_TRIGGER, int); }
internal inline char scen_cond_get_trigger_on(const uint8_t *c) { return FIELD(c, SCENCOND_TRIGGER_ON, char); }
internal inline char scen_cond_get_trigger_fired(const uint8_t *c) { return FIELD(c, SCENCOND_TRIGGER_FIRED, char); }
internal inline char scen_cond_get_timed(const uint8_t *c) { return FIELD(c, SCENCOND_TIMED, char); }
// an effect: its category (1 object, 2 player, 3 trigger, 4 media, 5 game, 6 AI) and its kind within the category
internal inline int scen_effect_get_category(const uint8_t *ef) { return FIELD(ef, SCENEFFECT_CATEGORY, int); }
internal inline int scen_effect_get_kind(const uint8_t *ef, int cat) {
    const uint32_t off[7] = { 0, SCENEFFECT_OBJECT_KIND, SCENEFFECT_PLAYER_KIND, SCENEFFECT_TRIGGER_KIND,
                              SCENEFFECT_MEDIA_KIND, SCENEFFECT_GAME_KIND, SCENEFFECT_AI_KIND };
    return cat >= 1 && cat <= 6 ? FIELD(ef, off[cat], int) : -1;
}
internal inline int scen_effect_get_object(const uint8_t *ef) { return FIELD(ef, SCENEFFECT_OBJECT, int); }     // object effects
internal inline int scen_effect_get_trigger(const uint8_t *ef) { return FIELD(ef, SCENEFFECT_TRIGGER, int); }    // trigger effects

// THE MAP ([G_MAP], game_map(): EEMap)
internal inline uint8_t **map_get_tiles(const void *map) { return FIELD(map, MAP_TILES, uint8_t **); }   // [y * size + x]
internal inline uint32_t map_get_size(const void *map) { return FIELD(map, MAP_SIZE, uint32_t); }     // tiles a side
internal inline uint32_t map_get_flags(const void *map) { return FIELD(map, MAP_FLAGS, uint32_t); }    // bit 0: water
internal inline uint8_t *map_get_grid(const void *map) { return FIELD(map, MAP_GRID, uint8_t *); }    // the unit grid (game/grid.c)
// the unit grid (map +0x181724, unit_grid()): cells of cs tiles, nx x ny of them; a cell's units begin .. end
internal inline uint8_t *grid_get_cells(const void *grid) { return FIELD(grid, GRID_CELLS, uint8_t *); }
internal inline int grid_get_cell_size(const void *grid) { return FIELD(grid, GRID_CELL_SIZE, int); }
internal inline int grid_get_nx(const void *grid) { return FIELD(grid, GRID_NX, int); }
internal inline int grid_get_ny(const void *grid) { return FIELD(grid, GRID_NY, int); }
internal inline ee_unit **cell_get_units_begin(const void *cell) { return FIELD(cell, GRIDCELL_UNITS, ee_unit **); }
internal inline ee_unit **cell_get_units_end(const void *cell) { return FIELD(cell, GRIDCELL_UNITS + 0x4, ee_unit **); }
// a resource site's ownership record (unit_get_vis(site)): +4 1 while the site is flagged (taken)
internal inline int siteown_get_flag(const void *so) { return FIELD(so, SITEOWN_FLAG, int); }

// THE SIMULATION ([G_WORLD_CLOCK], game_physics(): EEPhysics), THE WORLD ([G_WORLD]), THE SERVER ([G_SERVER])
internal inline unsigned phys_get_seed(const void *ph) { return FIELD(ph, PHYS_SEED, unsigned); }       // +0x18 the game generator
internal inline unsigned phys_get_step(const void *ph) { return FIELD(ph, PHYS_STEP, unsigned); }       // +0x80 steps done
internal inline float phys_get_step_s(const void *ph) { return FIELD(ph, PHYS_STEP_S, float); }          // +0x8C a step's length (s)
internal inline unsigned phys_get_steps_granted(const void *ph) { return FIELD(ph, PHYS_STEPS_GRANTED, unsigned); } // +0xB8 steps it may still run
internal inline uint32_t phys_get_time(const void *ph) { return FIELD(ph, PHYS_TIME, uint32_t); }      // +0xD0 game time (ms)
internal inline unsigned world_get_seed(const void *w) { return FIELD(w, WORLD_SEED, unsigned); }        // +0x54 the map's seed
internal inline void world_set_seed(void *w, unsigned v) { FIELD(w, WORLD_SEED, unsigned) = v; }
internal inline unsigned server_get_sent(const void *srv) { return FIELD(srv, SERVER_SENT, unsigned); }  // +0x10 / +0x190 sent / done
internal inline unsigned server_get_done(const void *srv) { return FIELD(srv, SERVER_DONE, unsigned); }
internal inline LONG server_get_waiting(const void *srv) { return FIELD(srv, SERVER_WAITING, LONG); }     // +0x94 messages waiting
// +0x1B4 / +0x1B8 the steps a second at most / at least, +0x1FB its current pace
internal inline unsigned server_get_rate_max(const void *srv) { return FIELD(srv, SERVER_RATE_MAX, unsigned); }
internal inline void server_set_rate_max(void *srv, unsigned v) { FIELD(srv, SERVER_RATE_MAX, unsigned) = v; }
internal inline unsigned server_get_rate_min(const void *srv) { return FIELD(srv, SERVER_RATE_MIN, unsigned); }
internal inline void server_set_rate_min(void *srv, unsigned v) { FIELD(srv, SERVER_RATE_MIN, unsigned) = v; }
internal inline uint8_t server_get_rate(const void *srv) { return FIELD(srv, SERVER_RATE, uint8_t); }

// THE GAME DATA ([G_GAME_DATA], game_data()): a chat line's time on screen (ms), the unit limits (default, most)
internal inline int gamedata_get_line_ms(const void *gd) { return FIELD(gd, GAMEDATA_LINE_MS, int); }
internal inline int gamedata_get_unit_limit(const void *gd) { return FIELD(gd, GAMEDATA_UNIT_LIMIT, int); }
internal inline int gamedata_get_unit_limit_max(const void *gd) { return FIELD(gd, GAMEDATA_UNIT_LIMIT_MAX, int); }

// THE GAME SETUP (game start 0x52599B(state, setup)): 16 player slots of 0x38, then the game's settings
internal inline uint8_t *setup_slot_at(const void *setup, int i) { return (uint8_t *)FIELD_AT(setup, i * SETUPSLOT_SIZE); }
internal inline uint8_t *setup_map_name_at(const void *setup) { return (uint8_t *)FIELD_AT(setup, SETUP_MAP_NAME); } // a UString
internal inline int setup_get_mode(const void *setup) { return FIELD(setup, SETUP_MODE, int); } // 0 random map, 1 scenario, 2 saved game, 3 editor test, 4 campaign
internal inline void setup_set_mode(void *setup, int v) { FIELD(setup, SETUP_MODE, int) = v; }
internal inline void setup_set_unknown_3d4(void *setup, int v) { FIELD(setup, SETUP_0x3D4, int) = v; }
internal inline void setup_set_map_scale(void *setup, int v) { FIELD(setup, SETUP_MAP_SCALE, int) = v; }   // 1 Small ..
internal inline void setup_set_start_epoch(void *setup, int v) { FIELD(setup, SETUP_START_EPOCH, int) = v; }
internal inline void setup_set_end_epoch(void *setup, int v) { FIELD(setup, SETUP_END_EPOCH, int) = v; }
internal inline void setup_set_map_size(void *setup, int v) { FIELD(setup, SETUP_MAP_SIZE, int) = v; }
internal inline void setup_set_unknown_3ec(void *setup, int v) { FIELD(setup, SETUP_0x3EC, int) = v; }
internal inline void setup_set_unit_limit(void *setup, int v) { FIELD(setup, SETUP_UNIT_LIMIT, int) = v; }
internal inline void setup_set_unknown_3f4(void *setup, int v) { FIELD(setup, SETUP_0x3F4, int) = v; }
internal inline void setup_set_difficulty(void *setup, int v) { FIELD(setup, SETUP_DIFFICULTY, int) = v; }
internal inline void setup_set_unknown_3fc(void *setup, int v) { FIELD(setup, SETUP_0x3FC, int) = v; }
// +0x400 .. +0x403 four flag bytes (the random map setup's 1, 0, 0, 1)
internal inline void setup_set_flag(void *setup, int i, uint8_t v) { FIELD(setup, SETUP_FLAGS + i, uint8_t) = v; }
// a slot: +0 its type (1 human, 0 computer, 4 closed), +0x20 the player's index, +0x24 its colour, +0x28 team,
// +0x2C citizens, +0x34 flags; +0x4 (3), +0x1C, +0x30 (0) not known
internal inline int slot_get_type(const uint8_t *slot) { return FIELD(slot, SETUPSLOT_TYPE, int); }
internal inline void slot_set_type(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_TYPE, int) = v; }
internal inline void slot_set_unknown_4(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_0x4, int) = v; }
internal inline uint8_t *slot_name_at(const uint8_t *slot) { return (uint8_t *)FIELD_AT(slot, SETUPSLOT_NAME); } // +0x8 the player's name (a UString)
internal inline void slot_set_unknown_1c(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_0x1C, int) = v; }
internal inline void slot_set_index(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_INDEX, int) = v; }
internal inline int slot_get_color(const uint8_t *slot) { return FIELD(slot, SETUPSLOT_COLOR, int); }
internal inline void slot_set_color(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_COLOR, int) = v; }
internal inline void slot_set_team(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_TEAM, int) = v; }
internal inline void slot_set_citizens(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_CITIZENS, int) = v; }
internal inline void slot_set_unknown_30(uint8_t *slot, int v) { FIELD(slot, SETUPSLOT_0x30, int) = v; }
internal inline void slot_set_flags(uint8_t *slot, uint32_t v) { FIELD(slot, SETUPSLOT_FLAGS, uint32_t) = v; }

// MESSAGES (the commands players send each other through the server): the sender's message id at +0xA (every
// message's, the chat message's row)
internal inline int8_t msg_get_from(const void *msg) { return (int8_t)FIELD(msg, MSG_CHAT_FROM, uint8_t); }
// a tribute (EEMPayTribute): food +0x10, gold +0x14, iron +0x18, stone +0x1C, wood +0x20; to player +0x24
internal inline int tribute_get_food(const void *msg) { return FIELD(msg, MSG_TRIBUTE_FOOD, int); }
internal inline int tribute_get_gold(const void *msg) { return FIELD(msg, MSG_TRIBUTE_GOLD, int); }
internal inline int tribute_get_iron(const void *msg) { return FIELD(msg, MSG_TRIBUTE_IRON, int); }
internal inline int tribute_get_stone(const void *msg) { return FIELD(msg, MSG_TRIBUTE_STONE, int); }
internal inline int tribute_get_wood(const void *msg) { return FIELD(msg, MSG_TRIBUTE_WOOD, int); }
internal inline uint8_t tribute_get_to(const void *msg) { return FIELD(msg, MSG_TRIBUTE_TO, uint8_t); }
// a chat line (EEMInGameChat): its text, a UString at +0x10 (the characters at its +4)
internal inline const char *chat_msg_get_text(const void *msg) { return FIELD(FIELD_AT(msg, MSG_CHAT_TEXT), 4, const char *); }
// a selection (EEMSelectUnits): the selection +0x10 (player_select), its unit ids +0x14 .. +0x18, +0x134 a flag
internal inline uint8_t *select_msg_selection_at(const void *msg) { return (uint8_t *)FIELD_AT(msg, MSG_SELECT_SELECTION); }
internal inline uint32_t *select_msg_get_ids_begin(const void *msg) { return FIELD(msg, MSG_SELECT_IDS, uint32_t *); }
internal inline uint32_t *select_msg_get_ids_end(const void *msg) { return FIELD(msg, MSG_SELECT_IDS + 0x4, uint32_t *); }
internal inline uint8_t select_msg_get_flag(const void *msg) { return FIELD(msg, MSG_SELECT_FLAG, uint8_t); }
// the game start (EEMStartLoading): +0x14 the game's seed
internal inline unsigned start_msg_get_seed(const void *msg) { return FIELD(msg, MSG_START_SEED, unsigned); }
internal inline void start_msg_set_seed(void *msg, unsigned v) { FIELD(msg, MSG_START_SEED, unsigned) = v; }

// THE CAMERA ([G_CAMERA], game_camera()) AND THE SCROLL ACTION (EEAScroll)
internal inline float *camera_look_at(const void *cam) { return (float *)FIELD_AT(cam, CAMERA_POS); }   // +0x78 x, y, z
internal inline float camera_get_x(const void *cam) { return FIELD(cam, CAMERA_POS, float); }
internal inline float camera_get_y(const void *cam) { return FIELD(cam, CAMERA_POS + 0x4, float); }
internal inline float camera_get_zoom(const void *cam) { return FIELD(cam, CAMERA_ZOOM, float); }
internal inline unsigned scroll_get_ticks(const void *s) { return FIELD(s, SCROLL_TICKS, unsigned); }    // +0x40 elapsed (10 ms)
internal inline void scroll_set_ticks(void *s, unsigned v) { FIELD(s, SCROLL_TICKS, unsigned) = v; }
// +0x44 .. +0x47 the keys held: left, right, down, up
internal inline uint8_t scroll_get_key(const void *s, int k) { return FIELD(s, SCROLL_KEYS + k, uint8_t); }
internal inline void scroll_set_key(void *s, int k, uint8_t v) { FIELD(s, SCROLL_KEYS + k, uint8_t) = v; }

// AND THE REST
// a tech effect (tech_effect(id)): +0x20 its kind (2 an attribute, 0x13 a line upgrade), +0x14 the unit type it is on
// (-1 all; a line upgrade's old type), +0x18 a line upgrade's new type, +0x2C the attribute
internal inline int effect_get_type(const void *ef) { return FIELD(ef, EFFECT_TYPE, int); }
internal inline int effect_get_new_type(const void *ef) { return FIELD(ef, EFFECT_NEW_TYPE, int); }
internal inline int effect_get_kind(const void *ef) { return FIELD(ef, EFFECT_KIND, int); }
internal inline int effect_get_attr(const void *ef) { return FIELD(ef, EFFECT_ATTR, int); }
// the build plan's list nodes (cm_get_plan): +0 the next, +0xC / +0x10 the entry's values
internal inline uint8_t *plan_node_get_next(const uint8_t *nd) { return FIELD(nd, CMLIST_NEXT, uint8_t *); }
internal inline int plan_node_get_unknown_c(const uint8_t *nd) { return FIELD(nd, CMLIST_0xC, int); }
internal inline int plan_node_get_unknown_10(const uint8_t *nd) { return FIELD(nd, CMLIST_0x10, int); }
// a player's statistics (player_stats_at): series k {?, begin, end, ..} of 8-byte samples
internal inline uint8_t *plstats_get_begin(const void *st, int k) { return FIELD(st, PLSTATS_LISTS + k * 0x18, uint8_t *); }
internal inline uint8_t *plstats_get_end(const void *st, int k) { return FIELD(st, PLSTATS_LISTS + 0x4 + k * 0x18, uint8_t *); }
// the object at a player's +0xB98 (not known): its +0x2D the attention's activity 7
internal inline uint8_t b98_get_att(const uint8_t *b98) { return b98[PLB98_ATT]; }
// the route planner (its own player's index +0x48) and its nodes (tile +0x3C, +0x40)
internal inline int planner_get_player_index(const void *rp) { return FIELD(rp, ROUTEPLANNER_PLAYER_INDEX, int); }
internal inline int route_node_get_x(const void *n) { return FIELD(n, ROUTENODE_X, int); }
internal inline int route_node_get_y(const void *n) { return FIELD(n, ROUTENODE_Y, int); }
// an attack strategy (test/testcmd.c aigroup: as the scenario effect sets it, 4 and 0 - not known)
internal inline void strategy_set_unknown_3c(void *s, int v) { FIELD(s, STRATEGY_0x3C, int) = v; }
internal inline void strategy_set_unknown_40(void *s, uint8_t v) { FIELD(s, STRATEGY_0x40, uint8_t) = v; }
// a damage model's map by state (FN_DAMAGE_POINTS' object)
internal inline uint8_t *dmgmodel_map_at(const void *model) { return (uint8_t *)FIELD_AT(model, DMGMODEL_MAP); }
// the UI: the in-game form's top bar mode (0 age, 1 clock, 2 FPS), a single-player screen's controls, the native chat's player
internal inline int uiform_get_top_bar(const void *form) { return FIELD(form, UIFORM_TOP_BAR, int); }
internal inline void uiform_set_top_bar(void *form, int v) { FIELD(form, UIFORM_TOP_BAR, int) = v; }
internal inline uint8_t **uiscreen_get_controls(const void *screen) { return FIELD(screen, UISCREEN_CONTROLS, uint8_t **); }
internal inline ee_player *nativechat_get_player(const void *nc) { return FIELD(nc, NATIVECHAT_PLAYER, ee_player *); }

// THE ENGINE DLLS' OBJECTS (engine/: AoC ships its own builds of them - scripts/port_dll.py)
// the rasterizer (DX7Rasterizer): its per-screen states +0x20[screen], +0x6C[0] the device state (its +0x1C the device)
internal inline uint8_t *ras_get_screen_state(const void *ras, unsigned screen) { return FIELD(ras, E_RAS_SCREEN_STATES, uint8_t **)[screen]; }
internal inline uint8_t **ras_get_screen_states(const void *ras) { return FIELD(ras, E_RAS_SCREEN_STATES, uint8_t **); }
internal inline uint8_t **ras_get_dstates(const void *ras) { return FIELD(ras, E_RAS_DSTATES, uint8_t **); }
internal inline void *dstate_get_device(const void *ds) { return FIELD(ds, E_DSTATE_DEVICE, void *); }
// a screen's state: its screen, front and back surfaces, window
internal inline uint8_t *scrstate_get_screen(const void *st) { return FIELD(st, E_SCRSTATE_SCREEN, uint8_t *); }
internal inline uint8_t *scrstate_get_front(const void *st) { return FIELD(st, E_SCRSTATE_FRONT, uint8_t *); }
internal inline uint8_t *scrstate_get_back(const void *st) { return FIELD(st, E_SCRSTATE_BACK, uint8_t *); }
internal inline uint8_t *scrstate_get_window(const void *st) { return FIELD(st, E_SCRSTATE_WINDOW, uint8_t *); }
// the frame statistics (display.c frame_stats): +0x2C4 a float sum, +0x328 / +0x330 / +0x338 counters, +0x32C frames,
// +0x334 the last time (ms); the averages over 12 frames +0x2C8 +0x2D0 +0x2DC +0x2E4, the rates per second +0x2CC
// +0x2D4 +0x2D8 (frames) +0x2E0
enum scrstate_stat { SS_SUM, SS_SUM_AVG, SS_SUM_RATE, SS_A_AVG, SS_A_RATE, SS_FPS, SS_B_AVG, SS_B_RATE, SS_C_AVG, SS_A,
                     SS_FRAMES, SS_B, SS_LAST, SS_C };
internal inline void *scrstate_stat_at(void *st, enum scrstate_stat k) {
    const uint8_t off[] = { 0x0, 0x4, 0x8, 0xC, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x64, 0x68, 0x6C, 0x70, 0x74 };   // from +0x2C4
    return FIELD_AT(st, E_SCRSTATE_STATS + off[k]);
}
#define SS_F(st, k) (*(float *)scrstate_stat_at(st, k))      // the floats (sums, averages, rates)
#define SS_U(st, k) (*(unsigned *)scrstate_stat_at(st, k))   // the counters, the last time (ms)
// a screen (DX7Screen): its modes +0x10 .. +0x14 (0x14 each), the mode +0x38, the desktop's +0x3C, fullscreen +0x3D,
// its DirectDraw +0x30
internal inline uint8_t *screen_get_modes_begin(const void *scr) { return FIELD(scr, E_SCREEN_MODES, uint8_t *); }
internal inline uint8_t *screen_get_modes_end(const void *scr) { return FIELD(scr, E_SCREEN_MODES + 0x4, uint8_t *); }
internal inline void *screen_get_ddraw(const void *scr) { return FIELD(scr, E_SCREEN_DDRAW, void *); }   // IDirectDraw7 *
internal inline unsigned screen_get_mode(const void *scr) { return FIELD(scr, E_SCREEN_MODE, unsigned); }
internal inline void screen_set_mode(void *scr, unsigned v) { FIELD(scr, E_SCREEN_MODE, unsigned) = v; }
internal inline uint8_t screen_get_desktop(const void *scr) { return FIELD(scr, E_SCREEN_DESKTOP, uint8_t); }
internal inline uint8_t screen_get_fullscreen(const void *scr) { return FIELD(scr, E_SCREEN_FULLSCREEN, uint8_t); }
// a surface (DX7Surface): its DirectDraw surface +0x48, the DC handed out +0x54, +0x5D 1 while that DC is out
internal inline void *surf_get_dds(const void *s) { return FIELD(s, E_SURF_DDS, void *); }   // IDirectDrawSurface7 *
internal inline void surf_set_dc(void *s, HDC dc) { FIELD(s, E_SURF_DC, HDC) = dc; }
internal inline uint8_t surf_get_dc_out(const void *s) { return FIELD(s, E_SURF_DC_OUT, uint8_t); }
internal inline void surf_set_dc_out(void *s, uint8_t v) { FIELD(s, E_SURF_DC_OUT, uint8_t) = v; }
// the game window (GEWindow): its HWND, +0x4 / +0x8 (floats), the game's width and height
internal inline HWND window_get_hwnd(const void *w) { return FIELD(w, E_WINDOW_HWND, HWND); }
internal inline float window_get_4(const void *w) { return FIELD(w, E_WINDOW_0x4, float); }
internal inline float window_get_8(const void *w) { return FIELD(w, E_WINDOW_0x8, float); }
internal inline int window_get_width(const void *w) { return FIELD(w, E_WINDOW_WIDTH, int); }
internal inline int window_get_height(const void *w) { return FIELD(w, E_WINDOW_HEIGHT, int); }
// the DirectInput mouse (ISMouse): +0x4 suspended (window inactive), +0x18 the buttons' bytes, +0x44 the device
internal inline uint8_t imouse_get_suspended(const void *ms) { return FIELD(ms, E_MOUSE_SUSPENDED, uint8_t); }
internal inline uint8_t *imouse_buttons_at(void *ms) { return (uint8_t *)FIELD_AT(ms, E_MOUSE_BUTTONS); }
internal inline void **imouse_get_device(const void *ms) { return FIELD(ms, E_MOUSE_DEVICE, void **); }
// a text (GEText): its string +0x34, font +0x44; a font (GEFont): its HFONT +0x18, the bitmap font +0x24
internal inline const char *text_get_string(const void *t) { return FIELD(t, E_TEXT_STRING, const char *); }
internal inline uint8_t *text_get_font(const void *t) { return FIELD(t, E_TEXT_FONT, uint8_t *); }
internal inline HFONT font_get_hfont(const void *f) { return FIELD(f, E_FONT_HFONT, HFONT); }
internal inline void *font_get_bitmap(const void *f) { return FIELD(f, E_FONT_BITMAP, void *); }
internal inline void font_set_bitmap(void *f, void *b) { FIELD(f, E_FONT_BITMAP, void *) = b; }
// the sound manager (SESoundManager): its channel slots +0x24 (nodes), the paused +0x34 .. +0x38, the categories +0x54
// .. +0x58, the mixer +0x70, the channel count +0x88
internal inline uint8_t **sndmgr_get_slots(const void *sm) { return FIELD(sm, E_SOUNDMGR_SLOTS, uint8_t **); }
internal inline uint8_t *sndmgr_get_paused_begin(const void *sm) { return FIELD(sm, E_SOUNDMGR_PAUSED, uint8_t *); }
internal inline uint8_t *sndmgr_get_paused_end(const void *sm) { return FIELD(sm, E_SOUNDMGR_PAUSED + 0x4, uint8_t *); }
internal inline uint8_t **sndmgr_get_cats_begin(const void *sm) { return FIELD(sm, E_SOUNDMGR_CATS, uint8_t **); }
internal inline uint8_t **sndmgr_get_cats_end(const void *sm) { return FIELD(sm, E_SOUNDMGR_CATS + 0x4, uint8_t **); }
internal inline uint8_t *sndmgr_get_mixer(const void *sm) { return FIELD(sm, E_SOUNDMGR_MIXER, uint8_t *); }
internal inline uint32_t sndmgr_get_channels(const void *sm) { return FIELD(sm, E_SOUNDMGR_CHANNELS, uint32_t); }
// a category: its share of the channels, its most channels, in use, priority
internal inline float sndcat_get_share(const void *c) { return FIELD(c, E_SNDCAT_SHARE, float); }
internal inline int sndcat_get_max(const void *c) { return FIELD(c, E_SNDCAT_MAX, int); }
internal inline int sndcat_get_used(const void *c) { return FIELD(c, E_SNDCAT_USED, int); }
internal inline int sndcat_get_priority(const void *c) { return FIELD(c, E_SNDCAT_PRIORITY, int); }
// a playing sound (SESMInstanceNode): its id +0xC, channel +0x1C, sample +0x20; a sample (SESample): its channel +0x24
internal inline int sndnode_get_id(const void *n) { return FIELD(n, E_SNDNODE_ID, int); }
internal inline int sndnode_get_channel(const void *n) { return FIELD(n, E_SNDNODE_CHANNEL, int); }
internal inline uint8_t *sndnode_get_sample(const void *n) { return FIELD(n, E_SNDNODE_SAMPLE, uint8_t *); }
internal inline int sample_get_channel(const void *s) { return FIELD(s, E_SAMPLE_CHANNEL, int); }
// the mixer (SEMixer): the plugin's SPIM functions as its fields - the mod reads and replaces some
enum mixer_fn { MIXER_GET_MAX_CHANNELS, MIXER_PLAY_CHANNEL, MIXER_SET_CHANNEL_SAMPLE, MIXER_SET_SAMPLE_VOLUME };
// SPIMGetMaximumChannelCount, int (cdecl *)(int channel), int (cdecl *)(int channel, SESample *), SPIMSetSampleVolume
internal inline void **mixer_fn_at(void *mx, enum mixer_fn k) {
    const uint8_t off[] = { 0x0, 0x20, 0x28, 0x40 };   // from +0xB0
    return (void **)FIELD_AT(mx, E_MIXER_SPIM + off[k]);
}

// THE GAME'S MOUSE (game_mouse(): the cursor in the game's coordinates)
internal inline float mouse_get_x(const ee_mouse *m) { return FIELD(m, MOUSE_X, float); }            // +0x4
internal inline void mouse_set_x(ee_mouse *m, float v) { FIELD(m, MOUSE_X, float) = v; }
internal inline float mouse_get_y(const ee_mouse *m) { return FIELD(m, MOUSE_Y, float); }            // +0x8
internal inline void mouse_set_y(ee_mouse *m, float v) { FIELD(m, MOUSE_Y, float) = v; }
internal inline float mouse_get_y_max(const ee_mouse *m) { return FIELD(m, MOUSE_Y_MAX, float); }       // +0x14
internal inline float mouse_get_x_min(const ee_mouse *m) { return FIELD(m, MOUSE_X_MIN, float); }       // +0x18
internal inline float mouse_get_x_max(const ee_mouse *m) { return FIELD(m, MOUSE_X_MAX, float); }       // +0x1C
internal inline float mouse_get_y_min(const ee_mouse *m) { return FIELD(m, MOUSE_Y_MIN, float); }       // +0x20
// +0xA4 frozen: the update then only reads the wheel
internal inline uint8_t mouse_get_frozen(const ee_mouse *m) { return FIELD(m, MOUSE_FROZEN, uint8_t); }
internal inline void mouse_set_frozen(ee_mouse *m, uint8_t v) { FIELD(m, MOUSE_FROZEN, uint8_t) = v; }
