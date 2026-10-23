// THE TYPES THE AI CODE NAMES: by their identifiers (docs/units.md), each one's number looked up in the data the game
// has loaded (unitnames.c unit_types_load) - so a data mod that numbers its types otherwise still gets the type it
// names. UT(B_FARM) is the farm's number; a type the data hasn't got is UT_NONE (logged once at the start): it never
// equals a type, and a place that builds or trains a type checks for it (ut_has). The name is the identifier in
// capitals: the docs and the code find each other.
#define UT_TYPES(X) \
    X(CITIZEN, "citizen") X(FEMALE_CITIZEN, "female_citizen") X(PRIEST, "priest") X(PROPHET, "prophet") \
    X(INF01_CLUBMAN, "inf01_clubman") \
    X(B_TOWN_CENTER, "b_town_center") X(B_CAPITOL, "b_capitol") X(B_SETTLEMENT, "b_settlement") \
    X(X_SINGLE_PLAYER_SETTLEMENT, "x_single_player_settlement") \
    X(B_FARM, "b_farm") X(B_GRANARY, "b_granary") X(B_GRANARY_IMPROVED, "b_granary_improved") \
    X(RES_FORAGE_BUSH, "res_forage_bush") X(RES_FISH, "res_fish") X(RES_TIGER, "res_tiger") X(RES_WOLF, "res_wolf") \
    X(B_BARRACKS, "b_barracks") X(B_ARCHERY_RANGE, "b_archery_range") X(B_STABLE, "b_stable") \
    X(B_SIEGE_FACTORY, "b_siege_factory") X(B_DOCK, "b_dock") X(B_NAVY_YARD, "b_navy_yard") X(B_AIRPORT, "b_airport") \
    X(B_TANK_FACTORY, "b_tank_factory") X(B_CYBER_FACTORY, "b_cyber_factory") X(B_FORTRESS, "b_fortress") \
    X(B_HOSPITAL, "b_hospital") \
    X(B_GUARD_TOWER_PALEO, "b_guard_tower_paleo") X(B_GUARD_TOWER_COPPER, "b_guard_tower_copper") \
    X(B_GUARD_TOWER_BRONZE, "b_guard_tower_bronze") X(B_GUARD_TOWER_MIDDLE, "b_guard_tower_middle") \
    X(B_GUARD_TOWER_IMPERIAL, "b_guard_tower_imperial") X(B_GUARD_TOWER_WW1, "b_guard_tower_ww1") \
    X(B_GUARD_TOWER_DIGITAL, "b_guard_tower_digital") X(B_TANK_OBSTACLE, "b_tank_obstacle") \
    X(B_WALL_COPPER, "b_wall_copper") X(B_WALL_BRONZE, "b_wall_bronze") X(B_WALL_MIDDLE, "b_wall_middle") \
    X(B_WALL_IMPERIAL, "b_wall_imperial") X(B_WALL_WW1, "b_wall_ww1") X(B_WALL_DIGITAL, "b_wall_digital") \
    X(B_GATE_COPPER, "b_gate_copper") X(B_GATE_BRONZE, "b_gate_bronze") X(B_GATE_MIDDLE, "b_gate_middle") \
    X(B_GATE_IMPERIAL, "b_gate_imperial") X(B_GATE_WW1, "b_gate_ww1") X(B_GATE_DIGITAL, "b_gate_digital") \
    X(B_AA10_GUN_88MM, "b_aa10_gun_88mm") X(B_AA13_MISSILE_TOWER, "b_aa13_missile_tower") \
    X(GUN_SIEGE04_CATAPULT, "gun_siege04_catapult") X(GUN_SIEGE06_TREBUCHET, "gun_siege06_trebuchet") \
    X(GUN_SIEGE07_BASILISK_CANNON, "gun_siege07_basilisk_cannon") X(GUN_SIEGE09_SERPENTINE_CANNON, "gun_siege09_serpentine_cannon") \
    X(GUN_SIEGE10_HOWITZER_CANNON, "gun_siege10_howitzer_cannon") X(GUN_SIEGE13_PALADIN_CANNON, "gun_siege13_paladin_cannon") \
    X(AA10_STINGER_SOLDIER, "aa10_stinger_soldier") X(AA10_TANK, "10_aa_tank") X(AA13_TANK_SKYWATCHER, "13_aa_tank_skywatcher") \
    X(S02_STONE_TRANSPORT, "s02_stone_transport") X(S03_COPPER_TRANSPORT, "s03_copper_transport") \
    X(S04_BRONZE_TRANSPORT, "s04_bronze_transport") X(S08_IMPERIAL_TRANSPORT, "s08_imperial_transport") \
    X(S10_ATOMIC_AGE_TRANSPORT, "s10_atomic_age_transport") X(S13_GARGANTUA_TRANSPORT, "s13_gargantua_transport") \
    X(X09_AGINCOURT_BATTLESHIP, "x09_agincourt_battleship") X(X07_GOLDEN_HIND, "x07_golden_hind") \
    X(S03_COPPER_BATTLESHIP_GALLEY, "s03_copper_battleship_galley") X(S04_BRONZE_BATTLESHIP_PENTAKONTER, "s04_bronze_battleship_pentakonter") \
    X(S04_BRONZE_CATAPULT_SHIP, "s04_bronze_catapult_ship") X(S05_BYZANTINE_BATTLESHIP_SEPTRIREME, "s05_byzantine_battleship_septrireme") \
    X(S06_MIDDLE_AGE_CATAPULT_SHIP, "s06_middle_age_catapult_ship") X(S06_MIDDLE_AGE_FRIGATE, "s06_middle_age_frigate") \
    X(S06_MIDDLE_AGE_GALLEY, "s06_middle_age_galley") X(S06_MIDDLE_AGES_BATTLESHIP_DECEREME, "s06_middle_ages_battleship_decereme") \
    X(S10_GOOD_HOPE_FRIGATE, "s10_good_hope_frigate") X(S10_CRUISER, "s10_cruiser") \
    X(S10_DREADNOUGHT_BATTLESHIP, "s10_dreadnought_battleship") X(S10_U_BOAT, "s10_u_boat") \
    X(INF10_MACHINE_GUN, "inf10_machine_gun") X(INF10_DOUGHBOY, "inf10_doughboy") X(INF10_TRENCH_MORTAR, "inf10_trench_mortar") \
    X(GUN_ARTILLERY10_ARTILLERY, "gun_artillery10_artillery") X(GUN_AT10_57MM_AT_GUN, "gun_at10_57mm_at_gun") \
    X(TANK10_A7V, "tank10_a7v") X(A_STRAFE_FIGHTER10_FOKKER_DR_1, "a_strafe_fighter10_fokker_dr_1") \
    X(A_BOMBER_10_GOTHA, "a_bomber_10_gotha") X(A_FIGHTER10_SOPWITH_CAMEL_F_1, "a_fighter10_sopwith_camel_f_1") \
    X(A_HELICOPTERGUNSHIP12_GUN_SHIP, "a_helicoptergunship12_gun_ship")

// the sets the code asks about (UT_IN(t, UT_TOWNS) ...), each written once - what a type is by its family or class
// (a tower, a wall, siege ...) is a role below. The tower line, oldest first (its upgrades: defence.c k_upg_names) -
// the walls', the gates'
#define UT_TOWER_LINE UT_B_GUARD_TOWER_PALEO, UT_B_GUARD_TOWER_COPPER, UT_B_GUARD_TOWER_BRONZE, UT_B_GUARD_TOWER_MIDDLE, \
    UT_B_GUARD_TOWER_IMPERIAL, UT_B_GUARD_TOWER_WW1, UT_B_GUARD_TOWER_DIGITAL
#define UT_WALL_LINE UT_B_WALL_COPPER, UT_B_WALL_BRONZE, UT_B_WALL_MIDDLE, UT_B_WALL_IMPERIAL, UT_B_WALL_WW1, UT_B_WALL_DIGITAL
#define UT_GATE_LINE UT_B_GATE_COPPER, UT_B_GATE_BRONZE, UT_B_GATE_MIDDLE, UT_B_GATE_IMPERIAL, UT_B_GATE_WW1, UT_B_GATE_DIGITAL
#define UT_TOWNS UT_B_TOWN_CENTER, UT_B_CAPITOL, UT_B_SETTLEMENT   // the drop sites that are towns
#define UT_ENUM(n, s) UT_##n,
enum ut_name { UT_TYPES(UT_ENUM) UT_N };
#undef UT_ENUM
#define UT_NONE (-0x10000)   // a type the loaded data hasn't got (and before it is loaded): equal to no type, no -1
#define UT(n) (g_ut[UT_##n])
internal int g_ut[UT_N] = { [0 ... UT_N - 1] = UT_NONE };   // unit_types_load
internal int ut_has(int t) { return t >= 0; }   // a type to build or train: the data has it
// the unit manager's list of a type (0x48F8AF: its units and buildings of that type); NULL: no manager, or a type the
// data hasn't got (UT_NONE never reaches the game)
internal uint8_t *type_list(void *um, int type) {
    return um && type >= 0 ? um_type_list(um, type) : NULL;
}
internal int ut_in(int t, const enum ut_name *set, int n) {   // t one of the named types (a set of UT_ names)
    for (int i = 0; i < n; i++) if (g_ut[set[i]] == t) return 1;
    return 0;
}
#define UT_IN(t, ...) ut_in((t), (const enum ut_name[]){ __VA_ARGS__ }, (int)(sizeof (const enum ut_name[]){ __VA_ARGS__ } / sizeof(enum ut_name)))

// WHAT A TYPE IS, BY THE DATA: its family (+0x4C4, kind.<kind>: the armour and bonus class) and its class (+0x4B4: how
// the engine runs it - 5 land unit, 6 citizen, 18 transport ship ...), so a data mod's new tower, wall, ram or
// transport counts without a name here. Set per type at the game start (unitnames.c ut_roles_load). Which type of a
// line the AI builds is another matter: the game's own test (0x555710, the pipe's canbuild) says yes to campaign and
// scenario objects too - the Missile Base and castle walls, the Sampson (a siege-family melee unit) - so towers, walls
// and gates are still built from their named lines; only transports and fishing boats, one per epoch by their tech
// record's test (landing.c ld_type_avail), are chosen by their class.
enum { CLS_LAND = 5, CLS_CITIZEN = 6, CLS_LAND_TRANSPORT = 16, CLS_AIR_TRANSPORT = 17, CLS_TRANSPORT = 18, CLS_FISHING_BOAT = 22, CLS_GATE = 25 };
enum {
    UTR_TOWER = 1,          // family towers (the guard towers, the Missile Base)
    UTR_WALL = 2,           // family walls: walls, gates, barbed wire, the castle pieces
    UTR_GATE = 4,           // class gate (a wall the owner's units pass)
    UTR_SIEGE = 8,          // a land unit of family siege or machines that shoots from 3 tiles on (catapult .. Paladin
                            // and the Culverin and Bronze Cannon; not the Sampson, melee, nor the staff car)
    UTR_RAM = 16,           // a land unit of family ram (not the siege towers and the Trojan Horse: class 16, they carry)
    UTR_CITIZEN = 32,       // class and family citizen (not the Engineer, a citizen-class human, nor the diplomat)
    UTR_TRANSPORT = 64,     // class transport ship
    UTR_FISHING_BOAT = 128, // class fishing boat
};
internal unsigned char g_ut_role[G_TYPES_N];
internal int ut_role(int t, int r) { return t >= 0 && t < G_TYPES_N && (g_ut_role[t] & r); }
