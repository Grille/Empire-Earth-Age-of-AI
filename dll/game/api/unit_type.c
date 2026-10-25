// A UNIT TYPE (ee_unit_type: EEUnitClass; ee_unit_stats: its values for one player) and its projectile class: fields,
// functions, hooks.

// FIELDS
// A UNIT TYPE'S DATA (EEUnitClass, type_data(t), a unit's +0x2C): the same for every unit of the type
// +0x42C, +0x430: not known - aim.c's shot-interval report prints them beside +0x438
// +0x470 a collision sphere (dreadnought 4, cruiser 3): half of it is about a ship's hull radius in tiles (a dreadnought
// is stopped 1.7 tiles from the coast)
// +0x5D6 a byte per terrain kind (tile_get_kind): may the type enter it (0 land, 1 water - a guess; AoC asks a
// unit's stats instead, its own copy of the path search's tile test)
internal inline int type_terrain_ok(const ee_unit_type *td, int kind) { return FIELD_AT(td, of_type_terrain_ok)[kind] != 0; }

// A UNIT TYPE'S VALUES FOR ONE PLAYER (a unit's +0x34, type_stats(pl, t)): the type's data with the player's upgrades
// +0xE0 float[6] the armour against each damage class
internal inline float stat_get_armor(const ee_unit_stats *st, int c) { return FIELD(st, of_stat_armor + c * 4, float); }

// FUNCTIONS
// the damage bonus of an attacker type (data atd) against a target type (data ttd), the players' indices
internal inline float attack_bonus(ee_unit_type *atd, ee_unit_type *ttd, int api, int tpi) {
    return GF(FN_ATTACK_BONUS)((void *)GA(G_BONUS_TABLE), FIELD(atd, of_type_weapon, int), ttd, api, tpi);
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
