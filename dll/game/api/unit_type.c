// A UNIT TYPE (ee_unit_type: EEUnitClass; ee_unit_stats: its values for one player) and its projectile class: fields,
// functions, hooks.

// FIELDS
// A UNIT TYPE'S DATA (EEUnitClass, type_data(t), a unit's +0x2C): the same for every unit of the type
// +0x42C, +0x430: not known - aim.c's shot-interval report prints them beside +0x438
// +0x470 a collision sphere (dreadnought 4, cruiser 3): half of it is about a ship's hull radius in tiles (a dreadnought
// is stopped 1.7 tiles from the coast)
// +0x5D6 a byte per terrain kind (tile_get_kind): may the type enter it (0 land, 1 water - a guess; AoC asks a
// unit's stats instead, its own copy of the path search's tile test)
internal inline int unit_type_terrain_ok(const ee_unit_type *td, int kind) { return FIELD_AT(td, of_unit_type_terrain_ok)[kind] != 0; }

// A UNIT TYPE'S VALUES FOR ONE PLAYER (a unit's +0x34, type_stats(pl, t)): the type's data with the player's upgrades
// +0xE0 float[6] the armour against each damage class
internal inline float unit_stats_get_armor(const ee_unit_stats *st, int c) { return FIELD(st, of_unit_stats_armor + c * 4, float); }

// FUNCTIONS
// the damage bonus of an attacker type (data atd) against a target type (data ttd), the players' indices
internal inline float attack_bonus(ee_unit_type *atd, ee_unit_type *ttd, int api, int tpi) {
    return fn_attack_bonus_game((void *)data_unit_type_bonus_table, FIELD(atd, of_unit_type_weapon, int), ttd, api, tpi);
}
// a projectile class's: the projectile down (1: on the ground or the target), a missile launched, a unit's shot fired
// a unit type's projectile class (type data +0x3FC) by its vtable: a plain projectile (arrows, shells), a missile
internal inline int is_projectile_class(const void *pc) { return *(const uintptr_t *)pc == data_unit_type_vt_projectile; }
internal inline int is_missile_class(const void *pc) { return *(const uintptr_t *)pc == data_unit_type_vt_missile; }
// the unit's type told that the unit got a goal (EEUnitClass's vtable, as the game's own goal assignment does)
// may the transport (of this type) carry the unit: its family against the unit type's board flags (EEUCTransport only)
internal inline char unit_type_can_carry(ee_unit_type *td, ee_unit *transport, ee_unit *u) {
    return ((char (__thiscall *)(void *, void *, void *))(*(void ***)td)[value_unit_type_slot_type_can_carry])(td, transport, u);
}
internal inline void unit_type_goal_assigned(ee_unit_type *td, ee_unit *u) { ((void (__thiscall *)(void *, void *))(*(void ***)td)[value_unit_type_slot_type_assign])(td, u); }

// HOOKS
// shots (aim.c): a projectile down, a missile launched, a shot fired
typedef char (__thiscall *proj_down_hook)(void *cls, ee_unit *projectile);
internal inline int patch_proj_down(proj_down_hook h) { return patch_vtable(data_unit_type_vt_projectile, value_unit_type_slot_proj_down, (uintptr_t)fn_proj_down, (void *)h); }
typedef int (__thiscall *missile_launch_hook)(uint8_t *cls, ee_unit *m, float *aim);
internal inline int patch_missile_launch(missile_launch_hook h) { return patch_vtable(data_unit_type_vt_missile, value_unit_type_slot_missile_launch, (uintptr_t)fn_missile_launch, (void *)h); }
