// A UNIT TYPE (ee_unit_type: EEUnitClass; ee_unit_stats: its values for one player) and its projectile class: fields,
// functions, hooks.

// FIELDS
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
internal inline void *type_get_model(const ee_unit_type *td) { return FIELD(td, TD_MODEL, void *); }        // +0x40C its movement model (movement.c model_kind)
internal inline int type_get_id(const ee_unit_type *td) { return FIELD(td, TD_ID, int); }             // +0x4B8 the type (UT(...))
// +0x470 a collision sphere (dreadnought 4, cruiser 3): half of it is about a ship's hull radius in tiles (a dreadnought
// is stopped 1.7 tiles from the coast)
internal inline float type_get_collision(const ee_unit_type *td) { return FIELD(td, TD_COLLISION, float); }
// +0x5D6 a byte per terrain kind (tile_get_kind): may the type enter it (0 land, 1 water - a guess; AoC asks a
// unit's stats instead, its own copy of the path search's tile test)
internal inline int type_terrain_ok(const ee_unit_type *td, int kind) { return FIELD_AT(td, TD_TERRAIN_OK)[kind] != 0; }
internal inline int type_get_damage_class(const ee_unit_type *td) { return FIELD(td, TD_DAMAGE_CLASS, int); }   // +0x4C0 the damage table's class
internal inline int type_get_category(const ee_unit_type *td) { return FIELD(td, TD_CATEGORY, int); }       // +0x4C4 as a target (the hit table's row)
internal inline int type_get_weapon(const ee_unit_type *td) { return FIELD(td, TD_WEAPON, int); }         // +0x4C8 ammo type (the hit table's weapon kind)
internal inline int type_get_tech(const ee_unit_type *td) { return FIELD(td, TD_TECH, int); }           // +0x4E0 the tech that makes it (its price)
internal inline int type_get_reload(const ee_unit_type *td) { return FIELD(td, TD_RELOAD, int); }         // +0x50C base reload time (ms)
internal inline int type_get_attack(const ee_unit_type *td) { return FIELD(td, TD_ATTACK, int); }         // +0x510 base attack
internal inline int type_get_transport_capacity(const ee_unit_type *td) { return FIELD(td, TD_TRANSPORT_CAPACITY, int); } // +0x54C (0 for soldiers)
internal inline float type_get_mark_size(const ee_unit_type *td) { return FIELD(td, TD_MARK_SIZE, float); }      // +0x464 its selection circle's size (tiles)
internal inline int type_get_transport_size(const ee_unit_type *td) { return FIELD(td, TD_TRANSPORT_SIZE, int); }   // +0x558 the room it takes aboard (the load goal's vf33 sums it)
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

// FUNCTIONS
// the damage bonus of an attacker type (data atd) against a target type (data ttd), the players' indices
internal inline float attack_bonus(ee_unit_type *atd, ee_unit_type *ttd, int api, int tpi) {
    return GF(FN_ATTACK_BONUS)((void *)GA(G_BONUS_TABLE), FIELD(atd, TD_WEAPON, int), ttd, api, tpi);
}
// a projectile class's: the projectile down (1: on the ground or the target), a missile launched, a unit's shot fired
internal inline char proj_down(void *cls, void *projectile) { return GF(FN_PROJ_DOWN)(cls, projectile); }
internal inline int missile_launch(void *cls, ee_unit *m, float *aim) { return GF(FN_MISSILE_LAUNCH)(cls, m, aim); }
internal inline void unit_fire(void *cls, ee_unit *u) { GF(FN_FIRE)(cls, u); }
// a unit type's projectile class (type data +0x3FC) by its vtable: a plain projectile (arrows, shells), a missile
internal inline int is_projectile_class(const void *pc) { return *(const uintptr_t *)pc == GA(VT_PROJECTILE); }
internal inline int is_missile_class(const void *pc) { return *(const uintptr_t *)pc == GA(VT_MISSILE); }
// the unit's type told that the unit got a goal (EEUnitClass's vtable, as the game's own goal assignment does)
// may the transport (of this type) carry the unit: its family against the unit type's board flags (EEUCTransport only)
internal inline char type_can_carry(ee_unit_type *td, ee_unit *transport, ee_unit *u) {
    return ((char (__thiscall *)(void *, void *, void *))(*(void ***)td)[GA(SLOT_TYPE_CAN_CARRY)])(td, transport, u);
}
internal inline void type_goal_assigned(ee_unit_type *td, ee_unit *u) { ((void (__thiscall *)(void *, void *))(*(void ***)td)[GA(SLOT_TYPE_ASSIGN)])(td, u); }

// HOOKS
// shots (aim.c): a projectile down, a missile launched, a shot fired
typedef char (__thiscall *proj_down_hook)(void *cls, ee_unit *projectile);
internal inline int patch_proj_down(proj_down_hook h) { return patch_vtable(GA(VT_PROJECTILE), GA(SLOT_PROJ_DOWN), GA(FN_PROJ_DOWN), (void *)h); }
typedef int (__thiscall *missile_launch_hook)(uint8_t *cls, ee_unit *m, float *aim);
internal inline int patch_missile_launch(missile_launch_hook h) { return patch_vtable(GA(VT_MISSILE), GA(SLOT_MISSILE_LAUNCH), GA(FN_MISSILE_LAUNCH), (void *)h); }
typedef void (__thiscall *fire_hook)(ee_unit_type *cls, ee_unit *u);
internal inline int patch_fire(fire_hook h, int *of) { return patch_calls(PATCH_LIST(FIRE_CALLS), GA(FN_FIRE), (void *)h, of); }
