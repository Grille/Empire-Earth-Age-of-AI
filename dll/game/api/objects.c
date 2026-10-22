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
internal inline uint8_t *player_get_combat_mgr(const ee_player *pl) { return FIELD(pl, OF(PL, 0xB8C), uint8_t *); } // +0xB8C EECPCombatManager (AM)
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
