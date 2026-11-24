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

// A UNIT TYPE'S IMPROVEMENTS (EEUCUpgradeTable, unit_type_get_upgrades): per attribute (IMP_*) the price factor and the
// step - 0: not offered; a level costs the type's own price x the level x the factor
internal inline float upgrade_table_price(const ee_upgrade_table *t, int attr) { return ((const float *)upgrade_table_factors_at(t))[attr * 2]; }
internal inline float upgrade_table_step(const ee_upgrade_table *t, int attr) { return ((const float *)upgrade_table_factors_at(t))[attr * 2 + 1]; }
// the improvement points one level costs (0x626F9A): an armour 1, the rest 2
internal inline int improvement_points(int attr) { return attr >= IMP_ARMOR_ARROW && attr <= IMP_ARMOR_SHOCK ? 1 : 2; }

// FUNCTIONS
// the damage bonus of an attacker type (data atd) against a target type (data ttd), the players' indices
internal inline float attack_bonus(ee_unit_type *atd, ee_unit_type *ttd, int api, int tpi) {
    return fn_to_hit_table_attack_bonus((void *)data_unit_type_bonus_table, FIELD(atd, of_unit_type_weapon, int), ttd, api, tpi);
}
// a projectile class's: the projectile down (1: on the ground or the target), a missile launched, a unit's shot fired
// a unit type's projectile class (type data +0x3FC) by its vtable: a plain projectile (arrows, shells), a missile
internal inline int is_projectile_class(const void *pc) { return *(const uintptr_t *)pc == vt_eeuc_projectile; }
internal inline int is_missile_class(const void *pc) { return *(const uintptr_t *)pc == vt_eeuc_missile; }
// the unit's type told that the unit got a goal (EEUnitClass's vtable, as the game's own goal assignment does)
// may the transport (of this type) carry the unit: its family against the unit type's board flags (EEUCTransport only)
internal inline char unit_type_can_carry(ee_unit_type *td, ee_unit *transport, ee_unit *u) {
    return ((char (__thiscall *)(void *, void *, void *))(*(void ***)td)[value_unit_type_slot_type_can_carry])(td, transport, u);
}
// may the priest convert target now: the game's own test (EEUCBasicPriest.can_convert - at war, a type that can be
// converted, the techs a priest or a building needs, the priest's power, no University's protection), which the
// game asks only before its own orders; *university 1 when a University's protection refused it. The 2017 update's is
// the priest classes' slot (all of them keep the base's: called directly), AoC's a plain function; 1 in a build
// without either
internal inline int priest_can_convert(ee_unit *priest, ee_unit *target, int *university) {
    char im = 0;
    int ok = fn_eeuc_basic_priest_can_convert ? eeuc_basic_priest_can_convert(unit_get_type_data(priest), priest, target, &im) != 0
           : fn_unit_type_priest_can_convert ? unit_type_priest_can_convert(priest, target, &im) != 0 : 1;
    if (university) *university = im != 0;
    return ok;
}
// the way an Airport's runways run from its hangar at x, y (its square's low corner): 0 +y, 1 -y, 2 +x, 3 -x - the
// game's own rule (unit_type.airport_facing; AoC's towards the middle of a planet on Small Planets); -1 in a build
// without either
internal inline int airport_facing(int x, int y) {
    return fn_unit_type_airport_facing ? unit_type_airport_facing(x, y) : fn_unit_type_airport_facing_aoc ? unit_type_airport_facing_aoc(x, y) : -1;
}
internal inline void unit_type_goal_assigned(ee_unit_type *td, ee_unit *u) { ((void (__thiscall *)(void *, void *))(*(void ***)td)[value_unit_type_slot_type_assign])(td, u); }
