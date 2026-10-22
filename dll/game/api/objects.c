// THE GAME'S OBJECTS' FIELDS, BY NAME: kind_get_x(obj) reads a field, kind_set_x(obj, v) writes one (only those the
// mod writes) - types.h's pointer types, the offsets per build from raw/fields.h (the 2017 update's offset beside each,
// as engine-notes.md and the decompile name them). Inlined: the machine code is FIELD's. Where a field's meaning
// isn't known, its comment says how the code uses it.

// A UNIT (EEUnit, EEComplexUnit): a unit, building, resource site or projectile on the map
internal inline int unit_get_refs(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x4), int); }                 // +0x4 references (release 0x4FCAD3)
internal inline void *unit_get_list_node(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x8), void *); }       // +0x8 its node in the unit list {next, prev}
internal inline uint8_t *unit_get_vis(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x14), uint8_t *); }      // +0x14 visibility: +0xC a uint16 per player, seen
internal inline ee_player *unit_get_owner(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x18), ee_player *); } // +0x18
internal inline int unit_get_x(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x1C), int); }                   // +0x1C tile x (-10000: off the map)
internal inline int unit_get_y(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x20), int); }                   // +0x20 tile y
// +0x24: not known - the test opponent keeps it beside the pointer, to tell a reused unit object from the one before
internal inline int unit_get_serial(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x24), int); }
internal inline uint8_t unit_get_released(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x2A), uint8_t); }    // +0x2A back in the pool (vf12 sets it)
internal inline uint8_t unit_get_valid(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x2B), uint8_t); }       // +0x2B mIsValid
internal inline ee_unit_type *unit_get_type_data(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x2C), ee_unit_type *); } // +0x2C its type's data (TD)
internal inline ee_unit *unit_get_physics(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x30), ee_unit *); }  // +0x30 (the game's dump: physics) - a unit too
internal inline ee_unit_stats *unit_get_stats(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x34), ee_unit_stats *); }    // +0x34 its player's stats of the type (USTAT)
internal inline int unit_get_resource(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x38), int); }            // +0x38 a resource site's resource
internal inline int unit_get_hp(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x3C), int); }                  // +0x3C hit points (> 0: alive)
internal inline int unit_get_amount(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x40), int); }              // +0x40 a resource site's amount left
internal inline int *unit_get_queue_begin(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x4C), int *); }      // +0x4C a building's queue
internal inline int *unit_get_queue_end(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x50), int *); }        // +0x50
internal inline int unit_get_id(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x60), int); }                  // +0x60 (lookup 0x4111F6)
// +0x68: not known - the AI's gatherer checks (econ.c, food.c) pass over a citizen where it is set
internal inline int unit_get_busy(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x68), int); }
// +0x130 its matrix (GETransformation): +0x134 / +0x144 / +0x154 the local y axis (facing), +0x13C / +0x14C / +0x15C
// the position in tiles
internal inline float unit_get_facing_x(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x134), float); }
internal inline float unit_get_facing_y(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x144), float); }
internal inline float unit_get_facing_z(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x154), float); }
internal inline float unit_get_pos_x(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x13C), float); }
internal inline float unit_get_pos_y(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x14C), float); }
internal inline float unit_get_pos_z(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x15C), float); }
internal inline ee_unit_type *unit_get_attacker_type(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x1C0), ee_unit_type *); } // +0x1C0 a projectile's: the attack's type data
// +0x1CC: the next of a chain from +0x30 - its last is what a BuildRepair goal holds (0x60BAD0)
internal inline ee_unit *unit_get_chain(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x1CC), ee_unit *); }
internal inline ee_goal *unit_get_goal(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x1EC), ee_goal *); }   // +0x1EC its goal (GOAL)
internal inline ee_goal *unit_get_goal_object(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x1F0), ee_goal *); } // +0x1F0 the goal object (0x5ED24B casts it)
internal inline void *unit_get_behaviour(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x208), void *); }    // +0x208 its behaviour (behaviour_table())
internal inline void unit_set_behaviour(ee_unit *u, void *b) { FIELD(u, OF(UNIT, 0x208), void *) = b; }
internal inline float unit_get_speed_min(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x21C), float); }      // +0x21C the speed, at least the type's minimum
internal inline float unit_get_speed(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x238), float); }          // +0x238 tiles/s (0 standing)
internal inline int unit_get_producing(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x25C), int); }          // +0x25C what a building makes now
internal inline int unit_get_flight_ms(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x28C), int); }          // +0x28C a missile's climb left (ms)
internal inline int unit_get_reload(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x2A0), int); }            // +0x2A0 its own reload (ms, 0: its class's)
internal inline uint8_t unit_get_frozen(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x33B), uint8_t); }     // +0x33B frozen (the scenario effect)
internal inline uint8_t unit_get_finished(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x340), uint8_t); }   // +0x340 on a building's +0x30: built
// +0x341: held by the AI (EEComputerPlayer vf19 sets it; cleared when the AI takes the unit for an attack)
internal inline uint8_t unit_get_ai_held(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x341), uint8_t); }

// A MAP TILE (the map's tile table, MAP +0x1816D4): its +0x10 is UNIT's
internal inline float tile_get_height(const void *tile) { return FIELD(tile, OF(UNIT, 0x10), float); }

// A UNIT'S GOAL (EEUnitGoal, a unit's +0x1EC; the kind by its vtable: goal_is_attack(g) ...): the fields of each kind
internal inline uint8_t goal_get_state(const ee_goal *g) { return FIELD(g, OF(GOAL, 0x5), uint8_t); }        // +0x5 (aim.c: 2 while it attacks)
// a move goal (goal_is_location): where to
internal inline float goal_get_dest_x(const ee_goal *g) { return FIELD(g, OF(GOAL, 0x20), float); }         // +0x20
internal inline float goal_get_dest_y(const ee_goal *g) { return FIELD(g, OF(GOAL, 0x24), float); }         // +0x24
// an attack goal (goal_is_attack): the unit it attacks
internal inline ee_unit *goal_get_target(const ee_goal *g) { return FIELD(g, OF(GOAL, 0x30), ee_unit *); }   // +0x30
// a gather goal (EEUGGather): the resource site
internal inline ee_unit *goal_get_site(const ee_goal *g) { return FIELD(g, OF(GOAL, 0x38), ee_unit *); }     // +0x38
// a build or repair goal (EEUGBuildRepair, goal_is_build): the building
internal inline ee_unit *goal_get_building(const ee_goal *g) { return FIELD(g, OF(GOAL, 0x3C), ee_unit *); } // +0x3C
// a load goal (goal_is_load): the transport or building to board
internal inline ee_unit *goal_get_holder(const ee_goal *g) { return FIELD(g, OF(GOAL, 0x50), ee_unit *); }   // +0x50
// an unload goal (goal_is_unload): the water tiles +0x24 / +0x2C, the point to unload at +0x34, the reach (squared)
internal inline int goal_get_water_x(const ee_goal *g) { return FIELD(g, OF(GOAL, 0x24), int); }
internal inline int goal_get_water_y(const ee_goal *g) { return FIELD(g, OF(GOAL, 0x28), int); }
internal inline int goal_get_water2_x(const ee_goal *g) { return FIELD(g, OF(GOAL, 0x2C), int); }
internal inline int goal_get_water2_y(const ee_goal *g) { return FIELD(g, OF(GOAL, 0x30), int); }
internal inline int goal_get_unload_x(const ee_goal *g) { return FIELD(g, OF(GOAL, 0x34), int); }
internal inline int goal_get_unload_y(const ee_goal *g) { return FIELD(g, OF(GOAL, 0x38), int); }
internal inline float goal_get_reach_sq(const ee_goal *g) { return FIELD(g, OF(GOAL, 0x3C), float); }

// A UNIT TYPE'S DATA (EEUnitClass, type_data(t), a unit's +0x2C): the same for every unit of the type
internal inline uint8_t *type_get_vtable(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x0), uint8_t *); }
internal inline const char *type_get_name(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x1C), const char *); } // +0x1C its data name
internal inline ee_unit_type *type_get_projectile(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x3FC), ee_unit_type *); } // +0x3FC what it fires
internal inline float type_get_min_range(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x41C), float); }  // +0x41C
internal inline float type_get_range(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x420), float); }      // +0x420 base range
internal inline float type_get_area(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x424), float); }       // +0x424 area effect
// +0x42C, +0x430: not known - aim.c's shot-interval report prints them beside +0x438
internal inline float type_get_unknown_42c(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x42C), float); }
internal inline float type_get_unknown_430(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x430), float); }
internal inline float type_get_accel(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x438), float); }      // +0x438 base accel (a projectile's: its fall)
internal inline int type_get_family(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x4B4), int); }         // +0x4B4 2 building, 29 fortress, 5/6 units ...
internal inline int type_get_id(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x4B8), int); }             // +0x4B8 the type (UT(...))
internal inline int type_get_damage_class(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x4C0), int); }   // +0x4C0 the damage table's class
internal inline int type_get_category(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x4C4), int); }       // +0x4C4 as a target (the hit table's row)
internal inline int type_get_weapon(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x4C8), int); }         // +0x4C8 ammo type (the hit table's weapon kind)
internal inline int type_get_tech(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x4E0), int); }           // +0x4E0 the tech that makes it (its price)
internal inline int type_get_reload(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x50C), int); }         // +0x50C base reload time (ms)
internal inline int type_get_attack(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x510), int); }         // +0x510 base attack
internal inline int type_get_transport_capacity(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x54C), int); } // +0x54C (0 for soldiers)
internal inline int type_get_garrison_capacity(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x550), int); }  // +0x550
internal inline int type_get_hp(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x55C), int); }             // +0x55C base hit points
internal inline int type_get_missile_delay(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x588), int); }  // +0x588 missile delay time
internal inline int type_get_size(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x58C), int); }          // +0x58C its footprint (tiles a side)
internal inline uint8_t type_get_behaviours(const ee_unit_type *td) { return FIELD(td, OF(TD, 0x621), uint8_t); } // +0x621 takes a behaviour (stances)

// A UNIT TYPE'S VALUES FOR ONE PLAYER (a unit's +0x34, type_stats(pl, t)): the type's data with the player's upgrades
internal inline ee_unit_type *stat_get_type_data(const ee_unit_stats *st) { return FIELD(st, OF(USTAT, 0x60), ee_unit_type *); } // +0x60
internal inline float stat_get_height(const ee_unit_stats *st) { return FIELD(st, OF(USTAT, 0x98), float); }     // +0x98
internal inline float stat_get_range(const ee_unit_stats *st) { return FIELD(st, OF(USTAT, 0x9C), float); }      // +0x9C tiles
internal inline float stat_get_area(const ee_unit_stats *st) { return FIELD(st, OF(USTAT, 0xA0), float); }       // +0xA0 the area its shots hit
internal inline float stat_get_speed(const ee_unit_stats *st) { return FIELD(st, OF(USTAT, 0xAC), float); }      // +0xAC tiles/s (0: cannot move)
internal inline float stat_get_shot_speed(const ee_unit_stats *st) { return FIELD(st, OF(USTAT, 0xC0), float); } // +0xC0 its projectile's launch speed
internal inline float stat_get_attack(const ee_unit_stats *st) { return FIELD(st, OF(USTAT, 0xD8), float); }     // +0xD8
// +0xE0 float[6] the armour against each damage class
internal inline float stat_get_armor(const ee_unit_stats *st, int c) { return FIELD(st, OF(USTAT, 0xE0) + c * 4, float); }
internal inline int stat_get_reload(const ee_unit_stats *st) { return FIELD(st, OF(USTAT, 0x138), int); }        // +0x138 ms between shots
internal inline int stat_get_hp(const ee_unit_stats *st) { return FIELD(st, OF(USTAT, 0x144), int); }            // +0x144 its full hit points

// A PLAYER (EEPlayer, EEComputerPlayer; engine-notes.md "Players", the AI settings table's fields: ai/settings.c)
// +0x18 / +0x1C: a list of 4-byte entries - not known; the population check's log counts it
internal inline uint8_t *player_get_list_begin(const ee_player *pl) { return FIELD(pl, OF(PL, 0x18), uint8_t *); }
internal inline uint8_t *player_get_list_end(const ee_player *pl) { return FIELD(pl, OF(PL, 0x1C), uint8_t *); }
internal inline void *player_name_at(const ee_player *pl) { return (void *)FIELD_AT(pl, OF(PL, 0x24)); }        // +0x24 its name (UWideString)
internal inline const char *player_get_name(const ee_player *pl) { return FIELD(pl, OF(PL, 0x28), const char *); } // +0x28 the name's text
internal inline const void *player_color_at(const ee_player *pl) { return FIELD_AT(pl, OF(PL, 0x438)); }      // +0x438 its colour (RGB)
internal inline int player_get_index(const ee_player *pl) { return FIELD(pl, OF(PL, 0x45C), int); }          // +0x45C
internal inline int8_t player_get_msg_id(const ee_player *pl) { return FIELD(pl, OF(PL, 0x464), int8_t); }  // +0x464 its id in a chat message (msg +0xA)
internal inline uint8_t player_get_explore_all(const ee_player *pl) { return FIELD(pl, OF(PL, 0x465), uint8_t); } // +0x465 no fog
internal inline void player_set_explore_all(ee_player *pl, uint8_t v) { FIELD(pl, OF(PL, 0x465), uint8_t) = v; }
internal inline uint8_t player_get_out(const ee_player *pl) { return FIELD(pl, OF(PL, 0x467), uint8_t); }    // +0x467 out of the game (resigned, defeated)
internal inline void *player_civ_name_at(const ee_player *pl) { return (void *)FIELD_AT(pl, OF(PL, 0x488)); }  // +0x488 its civilization's name (UWideString)
internal inline uint8_t *player_stats_at(const ee_player *pl) { return (uint8_t *)FIELD_AT(pl, OF(PL, 0x4A8)); } // +0x4A8 EEPlayerStatistics (PLSTATS)
internal inline int *player_get_selected_group(const ee_player *pl) { return FIELD(pl, OF(PL, 0x9BC), int *); } // +0x9BC
internal inline ee_tech_tree *player_get_tech_tree(const ee_player *pl) { return FIELD(pl, OF(PL, 0x9D4), ee_tech_tree *); } // +0x9D4
// +0x9E0 whom its chat reaches: 0 everyone, 1 allies, 2 enemies, 3 the players marked at +0xB2C
internal inline int player_get_chat_to(const ee_player *pl) { return FIELD(pl, OF(PL, 0x9E0), int); }
internal inline void player_set_chat_to(ee_player *pl, int v) { FIELD(pl, OF(PL, 0x9E0), int) = v; }
internal inline int player_get_stance(const ee_player *pl, int i) { return FIELD(pl, OF(PL, 0x9E4) + i * 4, int); } // +0x9E4 [16] towards player i: 0 allied, 1 enemy
internal inline int player_get_strategy(const ee_player *pl) { return FIELD(pl, OF(PL, 0xA28), int); }        // +0xA28 orders the attention's activities
internal inline int player_get_result(const ee_player *pl) { return FIELD(pl, OF(PL, 0xA30), int); }         // +0xA30 1 won, 2 lost
internal inline int *player_stock_at(const ee_player *pl) { return (int *)FIELD_AT(pl, OF(PL, 0xAF0)); }     // +0xAF0 int[7]: 2 food, 3 wood, 4 stone, 5 gold, 6 iron
internal inline int player_get_stock(const ee_player *pl, int r) { return FIELD(pl, OF(PL, 0xAF0) + r * 4, int); }
// +0xB10 / +0xB1C / +0xB20: population counts - +0xB1C and +0xB20 in production (0x556E11), +0xB10 not known
internal inline int player_get_pop_b10(const ee_player *pl) { return FIELD(pl, OF(PL, 0xB10), int); }
internal inline int player_get_objects(const ee_player *pl) { return FIELD(pl, OF(PL, 0xB14), int); }        // +0xB14 its objects (< 1: out)
internal inline int player_get_pop_b1c(const ee_player *pl) { return FIELD(pl, OF(PL, 0xB1C), int); }
internal inline int player_get_pop_b20(const ee_player *pl) { return FIELD(pl, OF(PL, 0xB20), int); }
internal inline uint8_t player_get_chat_to_player(const ee_player *pl, int i) { return FIELD(pl, OF(PL, 0xB2C) + i, uint8_t); } // +0xB2C [16]
internal inline void player_set_chat_to_player(ee_player *pl, int i, uint8_t v) { FIELD(pl, OF(PL, 0xB2C) + i, uint8_t) = v; }
internal inline void player_set_civ_chosen(ee_player *pl, uint8_t v) { FIELD(pl, OF(PL, 0xB46), uint8_t) = v; } // +0xB46 (0: the civ dialog opens)
internal inline ee_zone **player_get_zones_begin(const ee_player *pl) { return FIELD(pl, OF(PL, 0xB50), ee_zone **); } // +0xB50 the resource zones (map-wide)
internal inline ee_zone **player_get_zones_end(const ee_player *pl) { return FIELD(pl, OF(PL, 0xB54), ee_zone **); }
internal inline ee_combat_mgr *player_get_combat_mgr(const ee_player *pl) { return FIELD(pl, OF(PL, 0xB8C), ee_combat_mgr *); } // +0xB8C
internal inline ee_civ_mgr *player_get_civ_mgr(const ee_player *pl) { return FIELD(pl, OF(PL, 0xB90), ee_civ_mgr *); } // +0xB90 a computer player's
internal inline uint8_t *player_get_b98(const ee_player *pl) { return FIELD(pl, OF(PL, 0xB98), uint8_t *); }    // +0xB98 (PLB98: not known)
internal inline int player_get_difficulty(const ee_player *pl) { return FIELD(pl, OF(PL, 0xBA4), int); }      // +0xBA4 0 easy, 1 medium, 2 hard
internal inline void player_set_difficulty(ee_player *pl, int v) { FIELD(pl, OF(PL, 0xBA4), int) = v; }
// the price percent with Cheat (AdjustCost 0x430FBD): easy +0xD48 (int), medium +0xBB4 (float), hard +0xD4C (int)
internal inline float player_get_cost_medium(const ee_player *pl) { return FIELD(pl, OF(PL, 0xBB4), float); }
internal inline void player_set_cost_medium(ee_player *pl, float v) { FIELD(pl, OF(PL, 0xBB4), float) = v; }
internal inline int player_get_cost_easy(const ee_player *pl) { return FIELD(pl, OF(PL, 0xD48), int); }
internal inline void player_set_cost_easy(ee_player *pl, int v) { FIELD(pl, OF(PL, 0xD48), int) = v; }
internal inline int player_get_cost_hard(const ee_player *pl) { return FIELD(pl, OF(PL, 0xD4C), int); }
internal inline void player_set_cost_hard(ee_player *pl, int v) { FIELD(pl, OF(PL, 0xD4C), int) = v; }
internal inline float player_get_citizen_share(const ee_player *pl) { return FIELD(pl, OF(PL, 0xBDC), float); } // +0xBDC of the cap (0.45)
internal inline void player_set_citizen_share(ee_player *pl, float v) { FIELD(pl, OF(PL, 0xBDC), float) = v; }
internal inline int player_get_pop_cap(const ee_player *pl) { return FIELD(pl, OF(PL, 0xCCC), int); }         // +0xCCC its own cap (vf31)
internal inline void player_set_pop_cap(ee_player *pl, int v) { FIELD(pl, OF(PL, 0xCCC), int) = v; }
// attacks (the AI settings table, 0x431697): chances in percent, minutes, the next attack's game time
internal inline void player_set_attack_on_contact(ee_player *pl, int v) { FIELD(pl, OF(PL, 0xCD8), int) = v; }   // +0xCD8
internal inline void player_set_attack_for_resources(ee_player *pl, int v) { FIELD(pl, OF(PL, 0xCDC), int) = v; } // +0xCDC
internal inline void player_set_attack_revenge(ee_player *pl, int v) { FIELD(pl, OF(PL, 0xCE0), int) = v; }      // +0xCE0
internal inline void player_set_attack_max(ee_player *pl, int v) { FIELD(pl, OF(PL, 0xCFC), int) = v; }          // +0xCFC between attacks
internal inline void player_set_attack_min(ee_player *pl, int v) { FIELD(pl, OF(PL, 0xD00), int) = v; }          // +0xD00
internal inline void player_set_first_attack_max(ee_player *pl, int v) { FIELD(pl, OF(PL, 0xD28), int) = v; }    // +0xD28
internal inline void player_set_first_attack_min(ee_player *pl, int v) { FIELD(pl, OF(PL, 0xD2C), int) = v; }    // +0xD2C
internal inline void player_set_epoch_min(ee_player *pl, int v) { FIELD(pl, OF(PL, 0xD58), int) = v; }           // +0xD58 the epoch timer's minutes
internal inline void player_set_epoch_max(ee_player *pl, int v) { FIELD(pl, OF(PL, 0xD5C), int) = v; }           // +0xD5C
internal inline int player_get_dropoff_distance(const ee_player *pl) { return FIELD(pl, OF(PL, 0xE10), int); }   // +0xE10
internal inline int player_get_attention(const ee_player *pl) { return FIELD(pl, OF(PL, 0xE20), int); }          // +0xE20 activities a period (+1)
internal inline uint32_t player_get_next_attack(const ee_player *pl) { return FIELD(pl, OF(PL, 0xED8), uint32_t); } // +0xED8 game time
internal inline void player_set_next_attack(ee_player *pl, uint32_t v) { FIELD(pl, OF(PL, 0xED8), uint32_t) = v; }
// +0xEF4.. the AI toggles: Angreifen EF4, Aktiv EF9, Cheat EFB, Bauen EFC, Erkunden EFD, Sammeln EFF, Verteidigen F00,
// Ignorieren F02 - and +0xEF5 first attack done, +0xEFA boats instead of fields (a water map with fish)
internal inline uint8_t player_get_toggle(const ee_player *pl, int i) { return FIELD(pl, OF(PL, 0xEF4) + i, uint8_t); }
internal inline uint8_t player_get_first_attack_done(const ee_player *pl) { return FIELD(pl, OF(PL, 0xEF5), uint8_t); }
internal inline uint8_t player_get_boats_for_food(const ee_player *pl) { return FIELD(pl, OF(PL, 0xEFA), uint8_t); }
internal inline uint8_t player_get_cheat(const ee_player *pl) { return FIELD(pl, OF(PL, 0xEFB), uint8_t); }
internal inline void player_set_cheat(ee_player *pl, uint8_t v) { FIELD(pl, OF(PL, 0xEFB), uint8_t) = v; }
internal inline uint8_t player_get_build(const ee_player *pl) { return FIELD(pl, OF(PL, 0xEFC), uint8_t); }
internal inline void player_set_ignore(ee_player *pl, uint8_t v) { FIELD(pl, OF(PL, 0xF02), uint8_t) = v; }

// A PLAYER'S TECH TREE (EETechTree, player_get_tech_tree(pl))
internal inline int tt_get_player_index(const ee_tech_tree *tt) { return FIELD(tt, OF(TT, 0x4), int); }     // +0x4
internal inline int tt_get_epoch(const ee_tech_tree *tt) { return FIELD(tt, OF(TT, 0x4E4), int); }          // +0x4E4 the current epoch (0 = Prehistoric)
// +0x8 the epoch records, 0x54 apart (Art of Conquest's epoch field lies one record further: 0x538)
internal inline uint8_t *tt_epoch_rec_at(const ee_tech_tree *tt, int e) { return (uint8_t *)FIELD_AT(tt, OF(TT, 0x8) + e * 0x54); }
// an epoch record: +0x8 the epoch's tech's data, the new things the epoch needs +0x40 and has +0x48 (0x5BD681)
internal inline ee_tech_data *ttrec_get_tech_data(const uint8_t *rec) { return FIELD(rec, OF(TTREC, 0x8), ee_tech_data *); }
internal inline int ttrec_get_needed(const uint8_t *rec) { return FIELD(rec, OF(TTREC, 0x40), int); }
internal inline int ttrec_get_done(const uint8_t *rec) { return FIELD(rec, OF(TTREC, 0x48), int); }

// A TECH (EETechTreeNode, tech_lookup(tt, id); an epoch's node, its children: game/tech.c)
internal inline uint8_t tech_get_researched(const ee_tech *t) { return FIELD(t, OF(TECH, 0x4), uint8_t); }   // +0x4 (vf6 sets it)
internal inline uint8_t *tech_researched_at(const ee_tech *t) { return (uint8_t *)FIELD_AT(t, OF(TECH, 0x4)); }
internal inline uint8_t tech_get_available(const ee_tech *t) { return FIELD(t, OF(TECH, 0x6), uint8_t); }    // +0x6 (vf1, vf6 test it)
internal inline ee_tech_data *tech_get_data(const ee_tech *t) { return FIELD(t, OF(TECH, 0x8), ee_tech_data *); } // +0x8
internal inline ee_tech_tree *tech_get_tree(const ee_tech *t) { return FIELD(t, OF(TECHNODE, 0xC), ee_tech_tree *); } // +0xC its tree
internal inline int tech_get_epoch(const ee_tech *t) { return FIELD(t, OF(TECH, 0x14), int); }               // +0x14
// an epoch node's children (game/tech.c): the 2017 update's vector +0x24 .. +0x28, Art of Conquest's map head +0x28
internal inline uint8_t *tech_get_kids_begin(const ee_tech *t) { return FIELD(t, OF(TECHNODE, 0x24), uint8_t *); }
internal inline uint8_t *tech_get_kids_end(const ee_tech *t) { return FIELD(t, OF(TECHNODE, 0x28), uint8_t *); }

// A TECH'S DATA (tech_get_data(t): the same for every player)
// +0x10 .. +0x14 the unit types that make it (its buildings)
internal inline int *techdata_get_makers_begin(const ee_tech_data *d) { return FIELD(d, OF(TECHDATA, 0x10), int *); }
internal inline int *techdata_get_makers_end(const ee_tech_data *d) { return FIELD(d, OF(TECHDATA, 0x14), int *); }
// +0x24 its cost by resource r (2 food .. 6 iron)
internal inline int techdata_get_cost(const ee_tech_data *d, int r) { return FIELD(d, OF(TECHDATA, 0x24) + (r - 2) * 4, int); }
internal inline int techdata_get_category(const ee_tech_data *d) { return FIELD(d, OF(TECHDATA, 0x48), int); } // +0x48 (7: a line upgrade)
internal inline int techdata_get_id(const ee_tech_data *d) { return FIELD(d, OF(TECHDATA, 0x4C), int); }      // +0x4C
internal inline int techdata_get_effects(const ee_tech_data *d) { return FIELD(d, OF(TECHDATA, 0x50), int); } // +0x50 its effect list (effect_list(i))
internal inline int techdata_get_unit_type(const ee_tech_data *d) { return FIELD(d, OF(TECHDATA, 0x6C), int); } // +0x6C the unit type it makes (< 0: a tech)
// +0x7D it counts as one of an epoch's new things (EETechTree::vf4)
internal inline uint8_t techdata_get_counts(const ee_tech_data *d) { return FIELD(d, OF(TECHDATA, 0x7D), uint8_t); }
internal inline void techdata_set_counts(ee_tech_data *d, uint8_t v) { FIELD(d, OF(TECHDATA, 0x7D), uint8_t) = v; }
// +0x81 Art of Conquest: granted by its epoch's line upgrades (only_new)
internal inline uint8_t techdata_get_aoc_new(const ee_tech_data *d) { return FIELD(d, OF(TECHDATA, 0x81), uint8_t); }

// AN EFFECT LIST (G_EPOCH_TECHS[i], a tech's techdata_get_effects): +0x4 the effect ids, +0x8 their count
internal inline unsigned *effects_get_ids(const uint8_t *list) { return FIELD(list, OF(EPOCHTECHS, 0x4), unsigned *); }
internal inline unsigned effects_get_count(const uint8_t *list) { return FIELD(list, OF(EPOCHTECHS, 0x8), unsigned); }

// A CIVILIZATION MANAGER (EECPCivilizationManager, a computer player's +0xB90; Update 0x46E2C7): the AI's managers
internal inline uint8_t *cm_get_plan(const ee_civ_mgr *cm) { return FIELD(cm, OF(CM, 0x8), uint8_t *); }     // +0x8 the build plan's list
internal inline ee_resource_mgr *cm_get_resource_mgr(const ee_civ_mgr *cm) { return FIELD(cm, OF(CM, 0x38), ee_resource_mgr *); } // +0x38
internal inline ee_build_mgr *cm_get_build_mgr(const ee_civ_mgr *cm) { return FIELD(cm, OF(CM, 0x3C), ee_build_mgr *); }          // +0x3C
internal inline ee_research_mgr *cm_get_research_mgr(const ee_civ_mgr *cm) { return FIELD(cm, OF(CM, 0x40), ee_research_mgr *); } // +0x40
internal inline ee_unit_mgr *cm_get_unit_mgr(const ee_civ_mgr *cm) { return FIELD(cm, OF(CM, 0x44), ee_unit_mgr *); }             // +0x44
internal inline void *cm_get_wall_mgr(const ee_civ_mgr *cm) { return FIELD(cm, OF(CM, 0x48), void *); }       // +0x48 EECPWallManager
internal inline ee_player *cm_get_player(const ee_civ_mgr *cm) { return FIELD(cm, OF(CM, 0x4C), ee_player *); } // +0x4C its player
internal inline void *cm_get_project(const ee_civ_mgr *cm) { return FIELD(cm, OF(CM, 0x50), void *); }        // +0x50 the construction project (NULL: a new one may start)
// +0x58 uint32_t[CM_TIMERS_N] game times: 0 next epoch, 1 building count check, 2 military buildings (31 s), 3 build
// (0x46E51E), 4 (0x46E817), 5 (59 s)
internal inline uint32_t *cm_timers_at(const ee_civ_mgr *cm) { return (uint32_t *)FIELD_AT(cm, OF(CM, 0x58)); }
internal inline uint32_t cm_get_timer(const ee_civ_mgr *cm, int i) { return FIELD(cm, OF(CM, 0x58) + i * 4, uint32_t); }
internal inline void cm_set_timer(ee_civ_mgr *cm, int i, uint32_t v) { FIELD(cm, OF(CM, 0x58) + i * 4, uint32_t) = v; }
// +0x74.. flag bytes (CM_FLAGS_LEN): +0x75 the one-time setup done, +0x76 a request for the epoch's techs is out, the
// attention's activities +0x7A (0: request the epoch's techs), +0x7B (4: the build manager's pass), +0x7C (5); +0x78 not
// known
internal inline uint8_t cm_get_flag(const ee_civ_mgr *cm, int i) { return FIELD(cm, OF(CM, 0x74) + i, uint8_t); }
internal inline uint8_t *cm_setup_done_at(const ee_civ_mgr *cm) { return (uint8_t *)FIELD_AT(cm, OF(CM, 0x75)); }
internal inline uint8_t cm_get_request_out(const ee_civ_mgr *cm) { return FIELD(cm, OF(CM, 0x76), uint8_t); }
internal inline void cm_set_request_out(ee_civ_mgr *cm, uint8_t v) { FIELD(cm, OF(CM, 0x76), uint8_t) = v; }
internal inline uint8_t cm_get_unknown_78(const ee_civ_mgr *cm) { return FIELD(cm, OF(CM, 0x78), uint8_t); }
internal inline uint8_t cm_get_att_epoch_techs(const ee_civ_mgr *cm) { return FIELD(cm, OF(CM, 0x7A), uint8_t); }
internal inline uint8_t cm_get_att_build(const ee_civ_mgr *cm) { return FIELD(cm, OF(CM, 0x7B), uint8_t); }
internal inline uint8_t cm_get_att_7c(const ee_civ_mgr *cm) { return FIELD(cm, OF(CM, 0x7C), uint8_t); }

// A RESOURCE MANAGER (EECPResourceManager, CM +0x38; constructor/serializer 0x489F76 / 0x48C5B2)
internal inline void **rm_get_hunt_begin(const ee_resource_mgr *rm) { return FIELD(rm, OF(RM, 0x88), void **); } // +0x88 the animals to hunt
internal inline void **rm_get_hunt_end(const ee_resource_mgr *rm) { return FIELD(rm, OF(RM, 0x8C), void **); }
internal inline void **rm_get_fish_begin(const ee_resource_mgr *rm) { return FIELD(rm, OF(RM, 0x98), void **); } // +0x98 the fish
internal inline void **rm_get_fish_end(const ee_resource_mgr *rm) { return FIELD(rm, OF(RM, 0x9C), void **); }
// +0xA4 7 unit lists by resource (2 food .. 6 iron), deques 0x14 apart: their gatherers
internal inline uint8_t *rm_list_at(const ee_resource_mgr *rm, int r) { return (uint8_t *)FIELD_AT(rm, OF(RM, 0xA4) + r * 0x14); }
internal inline ee_civ_mgr *rm_get_owner(const ee_resource_mgr *rm) { return FIELD(rm, OF(RM, 0x130), ee_civ_mgr *); } // +0x130
internal inline int *rm_demand_at(const ee_resource_mgr *rm) { return (int *)FIELD_AT(rm, OF(RM, 0x134)); }   // +0x134 int[7] still needed
internal inline int rm_get_demand(const ee_resource_mgr *rm, int r) { return FIELD(rm, OF(RM, 0x134) + r * 4, int); }
internal inline uint32_t *rm_target_at(const ee_resource_mgr *rm) { return (uint32_t *)FIELD_AT(rm, OF(RM, 0x150)); } // +0x150 uint[7] gatherers wanted
internal inline uint32_t rm_get_target(const ee_resource_mgr *rm, int r) { return FIELD(rm, OF(RM, 0x150) + r * 4, uint32_t); }
internal inline uint32_t rm_get_timer(const ee_resource_mgr *rm, int i) { return FIELD(rm, OF(RM, 0x16C) + i * 4, uint32_t); } // +0x16C uint[5] next runs (game time)
internal inline uint8_t rm_get_initialized(const ee_resource_mgr *rm) { return FIELD(rm, OF(RM, 0x187), uint8_t); } // +0x187 (the timers reset while 0)
internal inline uint8_t rm_get_att_188(const ee_resource_mgr *rm) { return FIELD(rm, OF(RM, 0x188), uint8_t); }  // +0x188 the attention's activity 1

// A BUILD MANAGER (EECPBuildManager, CM +0x3C; Update 0x468540)
// +0x4 14 queues by epoch, deques 0x14 apart of {type, x, y}; the placement area follows at +0x11C
internal inline uint8_t *bm_queue_at(const ee_build_mgr *bm, int e) { return (uint8_t *)FIELD_AT(bm, OF(BM, 0x4) + e * 0x14); }
internal inline int bm_get_base_x(const ee_build_mgr *bm) { return FIELD(bm, OF(BM, 0x12C), int); }         // +0x12C its base (-10000: none yet)
internal inline int bm_get_base_y(const ee_build_mgr *bm) { return FIELD(bm, OF(BM, 0x130), int); }         // +0x130
internal inline ee_civ_mgr *bm_get_owner(const ee_build_mgr *bm) { return FIELD(bm, OF(BM, 0x134), ee_civ_mgr *); } // +0x134
internal inline ee_player *bm_get_player(const ee_build_mgr *bm) { return FIELD(bm, OF(BM, 0x138), ee_player *); }  // +0x138
internal inline uint8_t *bm_initialized_at(const ee_build_mgr *bm) { return (uint8_t *)FIELD_AT(bm, OF(BM, 0x148)); } // +0x148 the build lists set up (0x465E4F while 0)

// A UNIT MANAGER (EECPUnitManager, CM +0x44; Update 0x490AD6)
// +0xC 14 request lists, deques 0x14 apart of 16-byte entries {type, count, ..}
internal inline uint8_t *um_requests_at(const ee_unit_mgr *um, int i) { return (uint8_t *)FIELD_AT(um, OF(UM, 0xC) + i * 0x14); }
internal inline ee_player *um_get_player(const ee_unit_mgr *um) { return FIELD(um, OF(UM, 0x134), ee_player *); } // +0x134
internal inline int um_get_in_production(const ee_unit_mgr *um) { return FIELD(um, OF(UM, 0x13C), int); }   // +0x13C units in production (0x49014F adds)
internal inline uint8_t um_get_att_140(const ee_unit_mgr *um) { return FIELD(um, OF(UM, 0x140), uint8_t); }  // +0x140 the attention's activity 3

// A RESEARCH MANAGER (EECPResearchManager, CM +0x40)
internal inline int *rs_get_pool_begin(const ee_research_mgr *rs) { return FIELD(rs, OF(RS, 0x8), int *); }  // +0x8 the techs it may research (ids)
internal inline int *rs_get_pool_end(const ee_research_mgr *rs) { return FIELD(rs, OF(RS, 0xC), int *); }
internal inline void rs_set_pool_end(ee_research_mgr *rs, int *e) { FIELD(rs, OF(RS, 0xC), int *) = e; }
// +0x18 14 lists by epoch, 0x10 apart: {begin, end} of tech ids
internal inline int *rs_get_epoch_begin(const ee_research_mgr *rs, int e) { return FIELD(rs, OF(RS, 0x18) + e * 0x10, int *); }
internal inline int *rs_get_epoch_end(const ee_research_mgr *rs, int e) { return FIELD(rs, OF(RS, 0x1C) + e * 0x10, int *); }
internal inline int rs_get_current(const ee_research_mgr *rs) { return FIELD(rs, OF(RS, 0x114), int); }      // +0x114 what it researches now
internal inline uint8_t *rs_initialized_at(const ee_research_mgr *rs) { return (uint8_t *)FIELD_AT(rs, OF(RS, 0x118)); } // +0x118 the schedule set up (while 0)
internal inline uint8_t rs_get_att_random(const ee_research_mgr *rs) { return FIELD(rs, OF(RS, 0x11A), uint8_t); } // +0x11A the attention's activity 2 (random research)

// THE COMBAT MANAGER (EECPCombatManager, a computer player's +0xB8C) and its attack commander, which keeps the attack
// groups: a group has its subgroups (one per kind: land, sea, sea transport, air), a subgroup its unit records
internal inline ee_attack_cmd *am_get_commander(const ee_combat_mgr *am) { return FIELD(am, OF(AM, 0x48), ee_attack_cmd *); } // +0x48
// the attention's activities 6 ([+0x4C] +0x48; the object at +0x4C not known) and 8 (+0x62)
internal inline uint8_t am_get_att_4c(const ee_combat_mgr *am) {
    uint8_t *o = FIELD(am, OF(AM, 0x4C), uint8_t *);
    return o ? o[OF(AM4C, 0x48)] : 0;
}
internal inline uint8_t am_get_att_62(const ee_combat_mgr *am) { return FIELD(am, OF(AM, 0x62), uint8_t); }
// +0x4 the groups, a deque of ee_attack_group * (deque_at); +0x14 its count
internal inline uint8_t *ac_groups_at(const ee_attack_cmd *ac) { return (uint8_t *)FIELD_AT(ac, OF(AMCMD, 0x4)); }
internal inline int ac_get_group_count(const ee_attack_cmd *ac) { return FIELD(ac, OF(AMCMD, 0x14), int); }

// AN ATTACK GROUP (EECPAttackGroup)
internal inline ee_attack_subgroup **ag_get_subgroups_begin(const ee_attack_group *g) { return FIELD(g, OF(AG, 0x8), ee_attack_subgroup **); } // +0x8
internal inline ee_attack_subgroup **ag_get_subgroups_end(const ee_attack_group *g) { return FIELD(g, OF(AG, 0xC), ee_attack_subgroup **); }
internal inline ee_unit *ag_get_target_unit(const ee_attack_group *g) { return FIELD(g, OF(AG, 0x44), ee_unit *); } // +0x44 (its task +0x28's +0x1C)
internal inline ee_player *ag_get_target(const ee_attack_group *g) { return FIELD(g, OF(AG, 0x48), ee_player *); }  // +0x48 the player it attacks

// AN ATTACK SUBGROUP (EECPAttackSubGroup; EECPASGLand, asg_is_land, has the fields from +0x60 on)
internal inline ee_asg_unit **asg_get_units_begin(const ee_attack_subgroup *sg) { return FIELD(sg, OF(ASG, 0x54), ee_asg_unit **); } // +0x54 its members
internal inline ee_asg_unit **asg_get_units_end(const ee_attack_subgroup *sg) { return FIELD(sg, OF(ASG, 0x58), ee_asg_unit **); }
internal inline void asg_set_units_end(ee_attack_subgroup *sg, ee_asg_unit **e) { FIELD(sg, OF(ASG, 0x58), ee_asg_unit **) = e; }
// +0x60 the route, a deque of {x, y, ?} (attack.c route_waypoint); +0x70 its waypoints
internal inline uint8_t *asg_route_at(const ee_attack_subgroup *sg) { return (uint8_t *)FIELD_AT(sg, OF(ASG, 0x60)); }
internal inline unsigned asg_get_waypoints(const ee_attack_subgroup *sg) { return FIELD(sg, OF(ASG, 0x70), unsigned); }
// +0x100 the target: +4 x, +8 y, +0xC the unit's id, +0x10 its kind (1: a unit)
internal inline uint8_t *asg_target_at(const ee_attack_subgroup *sg) { return (uint8_t *)FIELD_AT(sg, OF(ASG, 0x100)); }
internal inline int asg_get_goal_x(const ee_attack_subgroup *sg) { return FIELD(sg, OF(ASG, 0x104), int); }
internal inline int asg_get_goal_y(const ee_attack_subgroup *sg) { return FIELD(sg, OF(ASG, 0x108), int); }
internal inline ee_unit *asg_get_target_unit(const ee_attack_subgroup *sg) { return FIELD(sg, OF(ASG, 0x118), ee_unit *); } // +0x118
internal inline ee_player *asg_get_target(const ee_attack_subgroup *sg) { return FIELD(sg, OF(ASG, 0x11C), ee_player *); } // +0x11C the player it attacks
internal inline int asg_get_mission(const ee_attack_subgroup *sg) { return FIELD(sg, OF(ASG, 0x124), int); }      // +0x124
internal inline ee_player *asg_get_player(const ee_attack_subgroup *sg) { return FIELD(sg, OF(ASG, 0x190), ee_player *); } // +0x190 its own
internal inline int asg_get_state(const ee_attack_subgroup *sg) { return FIELD(sg, OF(ASG, 0x1D0), int); }        // +0x1D0
internal inline int asg_get_x(const ee_attack_subgroup *sg) { return FIELD(sg, OF(ASG, 0x234), int); }            // +0x234 where it is
internal inline int asg_get_y(const ee_attack_subgroup *sg) { return FIELD(sg, OF(ASG, 0x238), int); }

// A SUBGROUP'S UNIT RECORD (EECPAttackSubGroupUnit)
internal inline ee_unit *asgu_get_unit(const ee_asg_unit *r) { return FIELD(r, OF(ASGU, 0x44), ee_unit *); }     // +0x44
internal inline void asgu_set_unit(ee_asg_unit *r, ee_unit *u) { FIELD(r, OF(ASGU, 0x44), ee_unit *) = u; }
internal inline void *asgu_get_engaged(const ee_asg_unit *r) { return FIELD(r, OF(ASGU, 0x48), void *); }       // +0x48 set while it fights
internal inline ee_attack_subgroup *asgu_get_subgroup(const ee_asg_unit *r) { return FIELD(r, OF(ASGU, 0x4C), ee_attack_subgroup *); } // +0x4C
// what FN_ASGU_SCORE scores: +0x4 the enemy
internal inline ee_unit *score_info_get_enemy(const void *info) { return FIELD(info, OF(ASGUINFO, 0x4), ee_unit *); }

// AN ATTACK TASK (EECPAGTask; test/testcmd.c aigroup makes one): +0x20 the player it attacks, +0x28 / +0x2C / +0x30
// as the scenario effect "Create AI Military Group" sets them (7, 2, 1 - not known)
internal inline void task_set_target_player(void *task, ee_player *pl) { FIELD(task, OF(TASK, 0x20), ee_player *) = pl; }
internal inline void task_set_unknown_28(void *task, int v) { FIELD(task, OF(TASK, 0x28), int) = v; }
internal inline void task_set_unknown_2c(void *task, int v) { FIELD(task, OF(TASK, 0x2C), int) = v; }
internal inline void task_set_unknown_30(void *task, uint8_t v) { FIELD(task, OF(TASK, 0x30), uint8_t) = v; }

// AN AI ZONE (a town or resource zone, player_get_zones_begin(pl)): its resource sites, whether it has a drop site, active
internal inline void **zone_get_sites_begin(const ee_zone *z) { return FIELD(z, OF(ZONE, 0x8), void **); }   // +0x8 .. +0xC its sites (ee_unit *)
internal inline void **zone_get_sites_end(const ee_zone *z) { return FIELD(z, OF(ZONE, 0xC), void **); }
internal inline int zone_get_drop(const ee_zone *z) { return FIELD(z, OF(ZONE, 0x14), int); }                 // +0x14 a drop site serves it
internal inline uint8_t zone_get_active(const ee_zone *z) { return FIELD(z, OF(ZONE, 0x18), uint8_t); }       // +0x18

// THE SCENARIO MANAGER AND ITS TRIGGERS (EEScenarioManager, scenario_manager(); test/scen.c, engine-notes.md
// "Scenario triggers"): the lists are vectors (begin, end) - the address of each list's begin
internal inline uint8_t *scen_triggers_at(const uint8_t *m) { return (uint8_t *)FIELD_AT(m, OF(SCEN, 0xE8)); }
internal inline uint8_t *scen_conds_at(const uint8_t *m) { return (uint8_t *)FIELD_AT(m, OF(SCEN, 0xF8)); }
internal inline uint8_t *scen_effects_at(const uint8_t *m) { return (uint8_t *)FIELD_AT(m, OF(SCEN, 0x108)); }
internal inline uint8_t *scen_objects_at(const uint8_t *m) { return (uint8_t *)FIELD_AT(m, OF(SCEN, 0x118)); }
internal inline uint8_t *scen_areas_at(const uint8_t *m) { return (uint8_t *)FIELD_AT(m, OF(SCEN, 0x128)); }
internal inline float scen_get_time(const uint8_t *m) { return FIELD(m, OF(SCEN, 0x210), float); }            // +0x210 ms
// a trigger (EESMTrigger): its name and description (UWideString), the lists of its condition and effect references
internal inline uint8_t *trigger_name_at(const uint8_t *t) { return (uint8_t *)FIELD_AT(t, OF(SCEN, 0x4)); }
internal inline uint8_t *trigger_desc_at(const uint8_t *t) { return (uint8_t *)FIELD_AT(t, OF(SCEN, 0x18)); }
internal inline uint8_t *trigger_conds_at(const uint8_t *t) { return (uint8_t *)FIELD_AT(t, OF(SCEN, 0x30)); }
internal inline uint8_t *trigger_effects_at(const uint8_t *t) { return (uint8_t *)FIELD_AT(t, OF(SCEN, 0x40)); }
internal inline int trigger_get_true_for(const uint8_t *t) { return FIELD(t, OF(SCEN, 0x4C), int); }          // +0x4C s the conditions held
internal inline void trigger_set_since(uint8_t *t, int v) { FIELD(t, OF(SCEN, 0x50), int) = v; }
internal inline char trigger_get_on(const uint8_t *t) { return FIELD(t, OF(SCEN, 0x58), char); }
internal inline void trigger_set_on(uint8_t *t, char v) { FIELD(t, OF(SCEN, 0x58), char) = v; }
internal inline char trigger_get_fired(const uint8_t *t) { return FIELD(t, OF(SCEN, 0x59), char); }
internal inline void trigger_set_fired(uint8_t *t, char v) { FIELD(t, OF(SCEN, 0x59), char) = v; }
internal inline char trigger_get_loop(const uint8_t *t) { return FIELD(t, OF(SCEN, 0x5A), char); }
// a trigger's condition reference: the condition's index, the join to the next (0 end, 1 and, 2 or), not
internal inline int trigger_cond_get_index(const uint8_t *w) { return FIELD(w, OF(SCEN, 0x4), int); }
internal inline int trigger_cond_get_join(const uint8_t *w) { return FIELD(w, OF(SCEN, 0x8), int); }
internal inline char trigger_cond_get_not(const uint8_t *w) { return FIELD(w, OF(SCEN, 0xC), char); }
// a trigger's effect reference: its kind (2: "or" between random groups), delay (s), the effect's index
internal inline int trigger_effect_get_kind(const uint8_t *w) { return FIELD(w, OF(SCEN, 0x4), int); }
internal inline int trigger_effect_get_delay(const uint8_t *w) { return FIELD(w, OF(SCEN, 0x8), int); }
internal inline int trigger_effect_get_index(const uint8_t *w) { return FIELD(w, OF(SCEN, 0xC), int); }
// a condition: its type (0 always .. 7 chat) and each type's values
internal inline uint8_t *scen_cond_text_at(const uint8_t *c) { return (uint8_t *)FIELD_AT(c, OF(SCEN, 0x2C)); } // chat: the text
internal inline int scen_cond_get_type(const uint8_t *c) { return FIELD(c, OF(SCEN, 0x40), int); }
internal inline int scen_cond_get_op(const uint8_t *c) { return FIELD(c, OF(SCEN, 0x44), int); }
internal inline int scen_cond_get_attr(const uint8_t *c) { return FIELD(c, OF(SCEN, 0x50), int); }
internal inline int scen_cond_get_tech(const uint8_t *c) { return FIELD(c, OF(SCEN, 0x58), int); }
internal inline int scen_cond_get_value(const uint8_t *c) { return FIELD(c, OF(SCEN, 0x5C), int); }
internal inline int scen_cond_get_object(const uint8_t *c) { return FIELD(c, OF(SCEN, 0x60), int); }
internal inline int scen_cond_get_player(const uint8_t *c) { return FIELD(c, OF(SCEN, 0x64), int); }
internal inline int scen_cond_get_time_from(const uint8_t *c) { return FIELD(c, OF(SCEN, 0x6C), int); }
internal inline int scen_cond_get_time_to(const uint8_t *c) { return FIELD(c, OF(SCEN, 0x70), int); }
internal inline int scen_cond_get_trigger(const uint8_t *c) { return FIELD(c, OF(SCEN, 0x78), int); }
internal inline char scen_cond_get_trigger_on(const uint8_t *c) { return FIELD(c, OF(SCEN, 0x80), char); }
internal inline char scen_cond_get_trigger_fired(const uint8_t *c) { return FIELD(c, OF(SCEN, 0x81), char); }
internal inline char scen_cond_get_timed(const uint8_t *c) { return FIELD(c, OF(SCEN, 0x83), char); }
// an effect: its category (1 object, 2 player, 3 trigger, 4 media, 5 game, 6 AI) and its kind within the category
internal inline int scen_effect_get_category(const uint8_t *ef) { return FIELD(ef, OF(SCEN, 0x100), int); }
internal inline int scen_effect_get_kind(const uint8_t *ef, int cat) {
    const uint32_t off[7] = { 0, OF(SCEN, 0x104), OF(SCEN, 0xF8), OF(SCEN, 0xFC), OF(SCEN, 0xF0), OF(SCEN, 0x13C), OF(SCEN, 0xE4) };
    return cat >= 1 && cat <= 6 ? FIELD(ef, off[cat], int) : -1;
}
internal inline int scen_effect_get_object(const uint8_t *ef) { return FIELD(ef, OF(SCEN, 0x158), int); }     // object effects
internal inline int scen_effect_get_trigger(const uint8_t *ef) { return FIELD(ef, OF(SCEN, 0x180), int); }    // trigger effects

// THE MAP ([G_MAP], game_map(): EEMap)
internal inline uint8_t **map_get_tiles(const void *map) { return FIELD(map, OF(MAP, 0x1816D4), uint8_t **); }   // [y * size + x]
internal inline uint32_t map_get_size(const void *map) { return FIELD(map, OF(MAP, 0x1816FC), uint32_t); }     // tiles a side
internal inline uint32_t map_get_flags(const void *map) { return FIELD(map, OF(MAP, 0x181730), uint32_t); }    // bit 0: water
// the unit grid (map +0x181724, unit_grid()): cells of cs tiles, nx x ny of them; a cell's units begin .. end
internal inline uint8_t *grid_get_cells(const void *grid) { return FIELD(grid, OF(GRID, 0x4), uint8_t *); }
internal inline int grid_get_cell_size(const void *grid) { return FIELD(grid, OF(GRID, 0x8), int); }
internal inline int grid_get_nx(const void *grid) { return FIELD(grid, OF(GRID, 0x10), int); }
internal inline int grid_get_ny(const void *grid) { return FIELD(grid, OF(GRID, 0x14), int); }
internal inline ee_unit **cell_get_units_begin(const void *cell) { return FIELD(cell, OF(GRIDCELL, 0x4), ee_unit **); }
internal inline ee_unit **cell_get_units_end(const void *cell) { return FIELD(cell, OF(GRIDCELL, 0x8), ee_unit **); }
// a resource site's ownership record (unit_get_vis(site)): +4 1 while the site is flagged (taken)
internal inline int siteown_get_flag(const void *so) { return FIELD(so, OF(SITEOWN, 0x4), int); }

// THE SIMULATION ([G_WORLD_CLOCK], game_physics(): EEPhysics), THE WORLD ([G_WORLD]), THE SERVER ([G_SERVER])
internal inline unsigned phys_get_seed(const void *ph) { return FIELD(ph, OF(PHYS, 0x18), unsigned); }       // +0x18 the game generator
internal inline unsigned phys_get_step(const void *ph) { return FIELD(ph, OF(PHYS, 0x80), unsigned); }       // +0x80 steps done
internal inline float phys_get_step_s(const void *ph) { return FIELD(ph, OF(PHYS, 0x8C), float); }          // +0x8C a step's length (s)
internal inline unsigned phys_get_steps_granted(const void *ph) { return FIELD(ph, OF(PHYS, 0xB8), unsigned); } // +0xB8 steps it may still run
internal inline uint32_t phys_get_time(const void *ph) { return FIELD(ph, OF(PHYS, 0xD0), uint32_t); }      // +0xD0 game time (ms)
internal inline unsigned world_get_seed(const void *w) { return FIELD(w, OF(WORLD, 0x54), unsigned); }        // +0x54 the map's seed
internal inline void world_set_seed(void *w, unsigned v) { FIELD(w, OF(WORLD, 0x54), unsigned) = v; }
internal inline unsigned server_get_sent(const void *srv) { return FIELD(srv, OF(SERVER, 0x10), unsigned); }  // +0x10 / +0x190 sent / done
internal inline unsigned server_get_done(const void *srv) { return FIELD(srv, OF(SERVER, 0x190), unsigned); }
internal inline LONG server_get_waiting(const void *srv) { return FIELD(srv, OF(SERVER, 0x94), LONG); }     // +0x94 messages waiting
// +0x1B4 / +0x1B8 the steps a second at most / at least, +0x1FB its current pace
internal inline unsigned server_get_rate_max(const void *srv) { return FIELD(srv, OF(SERVER, 0x1B4), unsigned); }
internal inline void server_set_rate_max(void *srv, unsigned v) { FIELD(srv, OF(SERVER, 0x1B4), unsigned) = v; }
internal inline unsigned server_get_rate_min(const void *srv) { return FIELD(srv, OF(SERVER, 0x1B8), unsigned); }
internal inline void server_set_rate_min(void *srv, unsigned v) { FIELD(srv, OF(SERVER, 0x1B8), unsigned) = v; }
internal inline uint8_t server_get_rate(const void *srv) { return FIELD(srv, OF(SERVER, 0x1FB), uint8_t); }

// THE GAME DATA ([G_GAME_DATA], game_data()): a chat line's time on screen (ms), the unit limits (default, most)
internal inline int gamedata_get_line_ms(const void *gd) { return FIELD(gd, OF(GAMEDATA, 0x2CC), int); }
internal inline int gamedata_get_unit_limit(const void *gd) { return FIELD(gd, OF(GAMEDATA, 0x2E8), int); }
internal inline int gamedata_get_unit_limit_max(const void *gd) { return FIELD(gd, OF(GAMEDATA, 0x2EC), int); }

// THE GAME SETUP (game start 0x52599B(state, setup)): 16 player slots of 0x38, then the game's settings
internal inline uint8_t *setup_slot_at(const void *setup, int i) { return (uint8_t *)FIELD_AT(setup, i * OF(SETUPSLOT, 0x38)); }
internal inline uint8_t *setup_map_name_at(const void *setup) { return (uint8_t *)FIELD_AT(setup, OF(SETUP, 0x3A8)); } // a UString
internal inline int setup_get_mode(const void *setup) { return FIELD(setup, OF(SETUP, 0x3D8), int); } // 0 random map, 1 scenario, 2 saved game, 3 editor test, 4 campaign
internal inline void setup_set_mode(void *setup, int v) { FIELD(setup, OF(SETUP, 0x3D8), int) = v; }
internal inline void setup_set_unknown_3d4(void *setup, int v) { FIELD(setup, OF(SETUP, 0x3D4), int) = v; }
internal inline void setup_set_map_scale(void *setup, int v) { FIELD(setup, OF(SETUP, 0x3DC), int) = v; }   // 1 Small ..
internal inline void setup_set_start_epoch(void *setup, int v) { FIELD(setup, OF(SETUP, 0x3E0), int) = v; }
internal inline void setup_set_end_epoch(void *setup, int v) { FIELD(setup, OF(SETUP, 0x3E4), int) = v; }
internal inline void setup_set_map_size(void *setup, int v) { FIELD(setup, OF(SETUP, 0x3E8), int) = v; }
internal inline void setup_set_unknown_3ec(void *setup, int v) { FIELD(setup, OF(SETUP, 0x3EC), int) = v; }
internal inline void setup_set_unit_limit(void *setup, int v) { FIELD(setup, OF(SETUP, 0x3F0), int) = v; }
internal inline void setup_set_unknown_3f4(void *setup, int v) { FIELD(setup, OF(SETUP, 0x3F4), int) = v; }
internal inline void setup_set_difficulty(void *setup, int v) { FIELD(setup, OF(SETUP, 0x3F8), int) = v; }
internal inline void setup_set_unknown_3fc(void *setup, int v) { FIELD(setup, OF(SETUP, 0x3FC), int) = v; }
// +0x400 .. +0x403 four flag bytes (the random map setup's 1, 0, 0, 1)
internal inline void setup_set_flag(void *setup, int i, uint8_t v) { FIELD(setup, OF(SETUP, 0x400) + i, uint8_t) = v; }
// a slot: +0 its type (1 human, 0 computer, 4 closed), +0x20 the player's index, +0x24 its colour, +0x28 team,
// +0x2C citizens, +0x34 flags; +0x4 (3), +0x1C, +0x30 (0) not known
internal inline int slot_get_type(const uint8_t *slot) { return FIELD(slot, OF(SETUPSLOT, 0x0), int); }
internal inline void slot_set_type(uint8_t *slot, int v) { FIELD(slot, OF(SETUPSLOT, 0x0), int) = v; }
internal inline void slot_set_unknown_4(uint8_t *slot, int v) { FIELD(slot, OF(SETUPSLOT, 0x4), int) = v; }
internal inline uint8_t *slot_name_at(const uint8_t *slot) { return (uint8_t *)FIELD_AT(slot, OF(SETUPSLOT, 0x8)); } // +0x8 the player's name (a UString)
internal inline void slot_set_unknown_1c(uint8_t *slot, int v) { FIELD(slot, OF(SETUPSLOT, 0x1C), int) = v; }
internal inline void slot_set_index(uint8_t *slot, int v) { FIELD(slot, OF(SETUPSLOT, 0x20), int) = v; }
internal inline int slot_get_color(const uint8_t *slot) { return FIELD(slot, OF(SETUPSLOT, 0x24), int); }
internal inline void slot_set_color(uint8_t *slot, int v) { FIELD(slot, OF(SETUPSLOT, 0x24), int) = v; }
internal inline void slot_set_team(uint8_t *slot, int v) { FIELD(slot, OF(SETUPSLOT, 0x28), int) = v; }
internal inline void slot_set_citizens(uint8_t *slot, int v) { FIELD(slot, OF(SETUPSLOT, 0x2C), int) = v; }
internal inline void slot_set_unknown_30(uint8_t *slot, int v) { FIELD(slot, OF(SETUPSLOT, 0x30), int) = v; }
internal inline void slot_set_flags(uint8_t *slot, uint32_t v) { FIELD(slot, OF(SETUPSLOT, 0x34), uint32_t) = v; }

// MESSAGES (the commands players send each other through the server): the sender's message id at +0xA (every
// message's, the chat message's row)
internal inline int8_t msg_get_from(const void *msg) { return (int8_t)FIELD(msg, OF(MSG_CHAT, 0xA), uint8_t); }
// a tribute (EEMPayTribute): food +0x10, gold +0x14, iron +0x18, stone +0x1C, wood +0x20; to player +0x24
internal inline int tribute_get_food(const void *msg) { return FIELD(msg, OF(MSG_TRIBUTE, 0x10), int); }
internal inline int tribute_get_gold(const void *msg) { return FIELD(msg, OF(MSG_TRIBUTE, 0x14), int); }
internal inline int tribute_get_iron(const void *msg) { return FIELD(msg, OF(MSG_TRIBUTE, 0x18), int); }
internal inline int tribute_get_stone(const void *msg) { return FIELD(msg, OF(MSG_TRIBUTE, 0x1C), int); }
internal inline int tribute_get_wood(const void *msg) { return FIELD(msg, OF(MSG_TRIBUTE, 0x20), int); }
internal inline uint8_t tribute_get_to(const void *msg) { return FIELD(msg, OF(MSG_TRIBUTE, 0x24), uint8_t); }
// a chat line (EEMInGameChat): its text, a UString at +0x10 (the characters at its +4)
internal inline const char *chat_msg_get_text(const void *msg) { return FIELD(FIELD_AT(msg, OF(MSG_CHAT, 0x10)), 4, const char *); }
// a selection (EEMSelectUnits): the selection +0x10 (player_select), its unit ids +0x14 .. +0x18, +0x134 a flag
internal inline uint8_t *select_msg_selection_at(const void *msg) { return (uint8_t *)FIELD_AT(msg, OF(MSG_SELECT, 0x10)); }
internal inline uint32_t *select_msg_get_ids_begin(const void *msg) { return FIELD(msg, OF(MSG_SELECT, 0x14), uint32_t *); }
internal inline uint32_t *select_msg_get_ids_end(const void *msg) { return FIELD(msg, OF(MSG_SELECT, 0x18), uint32_t *); }
internal inline uint8_t select_msg_get_flag(const void *msg) { return FIELD(msg, OF(MSG_SELECT, 0x134), uint8_t); }
// the game start (EEMStartLoading): +0x14 the game's seed
internal inline unsigned start_msg_get_seed(const void *msg) { return FIELD(msg, OF(MSG_START, 0x14), unsigned); }
internal inline void start_msg_set_seed(void *msg, unsigned v) { FIELD(msg, OF(MSG_START, 0x14), unsigned) = v; }

// THE CAMERA ([G_CAMERA], game_camera()) AND THE SCROLL ACTION (EEAScroll)
internal inline float *camera_look_at(const void *cam) { return (float *)FIELD_AT(cam, OF(CAMERA, 0x78)); }   // +0x78 x, y, z
internal inline float camera_get_x(const void *cam) { return FIELD(cam, OF(CAMERA, 0x78), float); }
internal inline float camera_get_y(const void *cam) { return FIELD(cam, OF(CAMERA, 0x7C), float); }
internal inline float camera_get_zoom(const void *cam) { return FIELD(cam, OF(CAMERA, 0xE0), float); }
internal inline unsigned scroll_get_ticks(const void *s) { return FIELD(s, OF(SCROLL, 0x40), unsigned); }    // +0x40 elapsed (10 ms)
internal inline void scroll_set_ticks(void *s, unsigned v) { FIELD(s, OF(SCROLL, 0x40), unsigned) = v; }
// +0x44 .. +0x47 the keys held: left, right, down, up
internal inline uint8_t scroll_get_key(const void *s, int k) { return FIELD(s, OF(SCROLL, 0x44) + k, uint8_t); }
internal inline void scroll_set_key(void *s, int k, uint8_t v) { FIELD(s, OF(SCROLL, 0x44) + k, uint8_t) = v; }

// AND THE REST
// a tech effect (tech_effect(id)): +0x20 its kind (2 an attribute, 0x13 a line upgrade), +0x14 the unit type it is on
// (-1 all; a line upgrade's old type), +0x18 a line upgrade's new type, +0x2C the attribute
internal inline int effect_get_type(const void *ef) { return FIELD(ef, OF(EFFECT, 0x14), int); }
internal inline int effect_get_new_type(const void *ef) { return FIELD(ef, OF(EFFECT, 0x18), int); }
internal inline int effect_get_kind(const void *ef) { return FIELD(ef, OF(EFFECT, 0x20), int); }
internal inline int effect_get_attr(const void *ef) { return FIELD(ef, OF(EFFECT, 0x2C), int); }
// the build plan's list nodes (cm_get_plan): +0 the next, +0xC / +0x10 the entry's values
internal inline uint8_t *plan_node_get_next(const uint8_t *nd) { return FIELD(nd, OF(CMLIST, 0x0), uint8_t *); }
internal inline int plan_node_get_unknown_c(const uint8_t *nd) { return FIELD(nd, OF(CMLIST, 0xC), int); }
internal inline int plan_node_get_unknown_10(const uint8_t *nd) { return FIELD(nd, OF(CMLIST, 0x10), int); }
// a player's statistics (player_stats_at): series k {?, begin, end, ..} of 8-byte samples
internal inline uint8_t *plstats_get_begin(const void *st, int k) { return FIELD(st, OF(PLSTATS, 0x1C) + k * 0x18, uint8_t *); }
internal inline uint8_t *plstats_get_end(const void *st, int k) { return FIELD(st, OF(PLSTATS, 0x20) + k * 0x18, uint8_t *); }
// the object at a player's +0xB98 (not known): its +0x2D the attention's activity 7
internal inline uint8_t b98_get_att(const uint8_t *b98) { return b98[OF(PLB98, 0x2D)]; }
// the route planner (its own player's index +0x48) and its nodes (tile +0x3C, +0x40)
internal inline int planner_get_player_index(const void *rp) { return FIELD(rp, OF(ROUTEPLANNER, 0x48), int); }
internal inline int route_node_get_x(const void *n) { return FIELD(n, OF(ROUTENODE, 0x3C), int); }
internal inline int route_node_get_y(const void *n) { return FIELD(n, OF(ROUTENODE, 0x40), int); }
// an attack strategy (test/testcmd.c aigroup: as the scenario effect sets it, 4 and 0 - not known)
internal inline void strategy_set_unknown_3c(void *s, int v) { FIELD(s, OF(STRATEGY, 0x3C), int) = v; }
internal inline void strategy_set_unknown_40(void *s, uint8_t v) { FIELD(s, OF(STRATEGY, 0x40), uint8_t) = v; }
// a damage model's map by state (FN_DAMAGE_POINTS' object)
internal inline uint8_t *dmgmodel_map_at(const void *model) { return (uint8_t *)FIELD_AT(model, OF(DMGMODEL, 0x14C)); }
// the UI: the in-game form's top bar mode (0 age, 1 clock, 2 FPS), a single-player screen's controls, the native chat's player
internal inline int uiform_get_top_bar(const void *form) { return FIELD(form, OF(UIFORM, 0x5D8), int); }
internal inline void uiform_set_top_bar(void *form, int v) { FIELD(form, OF(UIFORM, 0x5D8), int) = v; }
internal inline uint8_t **uiscreen_get_controls(const void *screen) { return FIELD(screen, OF(UISCREEN, 0xF8), uint8_t **); }
internal inline ee_player *nativechat_get_player(const void *nc) { return FIELD(nc, OF(NATIVECHAT, 0x20), ee_player *); }

// THE GAME'S MOUSE (game_mouse(): the cursor in the game's coordinates)
internal inline float mouse_get_x(const ee_mouse *m) { return FIELD(m, OF(MOUSE, 0x4), float); }            // +0x4
internal inline void mouse_set_x(ee_mouse *m, float v) { FIELD(m, OF(MOUSE, 0x4), float) = v; }
internal inline float mouse_get_y(const ee_mouse *m) { return FIELD(m, OF(MOUSE, 0x8), float); }            // +0x8
internal inline void mouse_set_y(ee_mouse *m, float v) { FIELD(m, OF(MOUSE, 0x8), float) = v; }
internal inline float mouse_get_y_max(const ee_mouse *m) { return FIELD(m, OF(MOUSE, 0x14), float); }       // +0x14
internal inline float mouse_get_x_min(const ee_mouse *m) { return FIELD(m, OF(MOUSE, 0x18), float); }       // +0x18
internal inline float mouse_get_x_max(const ee_mouse *m) { return FIELD(m, OF(MOUSE, 0x1C), float); }       // +0x1C
internal inline float mouse_get_y_min(const ee_mouse *m) { return FIELD(m, OF(MOUSE, 0x20), float); }       // +0x20
// +0xA4 frozen: the update then only reads the wheel
internal inline uint8_t mouse_get_frozen(const ee_mouse *m) { return FIELD(m, OF(MOUSE, 0xA4), uint8_t); }
internal inline void mouse_set_frozen(ee_mouse *m, uint8_t v) { FIELD(m, OF(MOUSE, 0xA4), uint8_t) = v; }
