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
internal inline uint8_t *unit_get_type_data(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x2C), uint8_t *); } // +0x2C its type's data (TD)
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
internal inline uint8_t *unit_get_attacker_type(const ee_unit *u) { return FIELD(u, OF(UNIT, 0x1C0), uint8_t *); } // +0x1C0 a projectile's: the attack's type data
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
