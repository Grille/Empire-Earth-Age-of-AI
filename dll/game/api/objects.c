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
internal inline uint8_t *unit_get_stats(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x34), uint8_t *); }    // +0x34 its player's stats of the type (USTAT)
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
internal inline uint8_t *unit_get_goal(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x1EC), uint8_t *); }   // +0x1EC its goal (GOAL)
internal inline uint8_t *unit_get_goal_object(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x1F0), uint8_t *); } // +0x1F0 the goal object (0x5ED24B casts it)
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

// A PLAYER (EEPlayer, EEComputerPlayer; engine-notes.md "Players", the AI settings table's fields: ai/settings.c)
internal inline int player_get_unknown_4(const ee_player *pl) { return FIELD(pl, OF(PL, 0x4), int); }        // +0x4 not known (a log prints it)
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
internal inline void **player_get_zones_begin(const ee_player *pl) { return FIELD(pl, OF(PL, 0xB50), void **); } // +0xB50 the resource zones (map-wide)
internal inline void **player_get_zones_end(const ee_player *pl) { return FIELD(pl, OF(PL, 0xB54), void **); }
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
