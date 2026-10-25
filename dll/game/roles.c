// WHAT A TYPE IS, as the code asks it: the roles of unit_ids.h (by the game's family and class) and the army families.
// Walls and gates are no weak spot (no economy behind them, only a detour): the family walls (barbed wire too).
internal int is_wall(int type) { return ut_role(type, UTR_WALL); }
// a unit family of the army (soldiers, siege, ships, aircraft ... - not buildings, walls, towers, gates, resources):
// the families as in the statistics code 0x5EB000/0x55DD66
internal int is_army_family(int fam) { return !(fam == CLS_BUILDING || fam == CLS_LAND_CITIZEN || fam == 0x13 || fam == CLS_NAVAL_CITIZEN || fam == 0x18 || fam == CLS_GATE || fam == CLS_FORTRESS || fam == CLS_AMBIENT || fam == CLS_RESOURCE); }
internal int is_ram(int type) { return ut_role(type, UTR_RAM); }
internal int is_siege(int type) { return ut_role(type, UTR_SIEGE); }
internal int is_tower_type(int t) { return ut_role(t, UTR_TOWER) || t == UT(B_TANK_OBSTACLE); }   // the tank barrier holds a front too
internal int is_citizen_type(int type) { return ut_role(type, UTR_CITIZEN); }
internal int is_transport(int t) { return ut_role(t, UTR_TRANSPORT); }   // a transport ship (its class)
internal int is_fishing_boat(int t) { return ut_role(t, UTR_FISHING_BOAT); }
// a soldier: of an army family, no siege weapon, ram or citizen
internal int is_soldier(ee_unit_type *td) {
    int fam = unit_type_get_class_id(td), type = unit_type_get_id(td);
    if (!is_army_family(fam)) return 0;
    return !is_siege(type) && !is_ram(type) && !is_citizen_type(type);
}
