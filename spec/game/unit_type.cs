/// a unit type's data, game.types[type] and a unit's +0x2C (EEUnitClass, EEUC*)
[c_type("ee_unit_type")]
unsafe struct EEUnitClass
{
    [offset(0x0), found(c)]
    uint8* vtable;

    /// its data name
    [offset(0x1C), found(v)]
    char8* name;

    /// the data's column 0x344 (the community editor: "Plane refuel location") - 1 the airport planes, 2 the carriers'
    /// (F-14, Corsair, Avenger); the air models (0x54A5AE, 0x54AA2B, 0x54B540, 0x54BC24) treat 2 apart, AoC also 4
    /// (the AoC decompilation: mAirTrafficType, an airport's air traffic slot - 1)
    [offset(0x28), found(c), guess]
    int32 refuel_at;

    /// the graphic effect a missile or projectile plays on itself when it arrives; a unit's death effect (the AoC
    /// decompilation: mDeathEffect) - a crashing aircraft's on reaching the ground (EEEffectManager.add_on_unit's
    /// air_crash_effect), a ship's and a submarine's as they start to die (EEUCNaval::vf29 0x606427, EEUCSubmergedNaval
    /// 0x608901)
    [offset((ee)0x378, (aoc)0x68), found(c)]
    void* impact_gfx;

    /// the graphic effect on the attacker when it fires
    [offset((ee)0x38C, (aoc)0x7C), found(c)]
    void* fire_gfx;

    /// the effect at the shot's point (z 0) instead of impact_gfx when a missile or projectile ends below 0 over a tile
    /// below 0 (water?) without attacking a unit (EEUCMissile::vf29 0x605E9B, EEUCProjectile::vf29 0x607B11)
    /// (the AoC decompilation: mSplashEffect, the splash on the water) - the water's: a crashing aircraft's splash where
    /// it comes down through z 0 over a tile under the water (EEPMAirplane's and EEPMHelicopter's crash steps, the
    /// call air_crash_splash: z 0 and the tile's height below 0 its test)
    [offset((ee)0x394, (aoc)0x84), found(c)]
    void* ground_gfx;

    /// the shot's effect: a beam from the attacker to the target, or at the aim point without one
    [offset((ee)0x3C4, (aoc)0xB4), found(c)]
    void* shot_gfx;

    /// what it fires
    [offset((ee)0x3FC, (aoc)0xF8), found(c)]
    EEUnitClass* projectile;

    /// its movement model (movement.c model_kind)
    /// its movement model (EEPhysicsModel: movement.vt_pm_land, movement.vt_pm_naval ...), shared by the type's units - EEUCNaval::vf45
    /// 0x60686F calls the movement step on it
    [offset((ee)0x40C, (aoc)0x110), found(c)]
    void* model;

    /// its improvements' table (db\dbupgrade.dat's row by the object data's +0x238, unit_type.upgrade_tables; the
    /// type's constructor 0x61C35B, AoC 0x62F317) - every type of a running game holds one (2026-11-15: buildings a
    /// row with max_level 0)
    [offset((ee)0x410, (aoc)0x114), found(c)]
    EEUCUpgradeTable* upgrades;

    /// the data's column 0x1D4 (the editor: "Flight altitude"); below it the air models scale the height by
    /// stats.height x (z / it x 0.5 + 0.5) (the AoC decompilation: mCruiseAltitude)
    [offset((ee)0x418, (aoc)0x11C), found(c), guess]
    float32 flight_altitude;

    /// the type data's constructors
    [offset((ee)0x41C, (aoc)0x120), found(c)]
    float32 min_range;

    /// base range
    /// the type data's constructors
    [offset((ee)0x420, (aoc)0x124), found(c)]
    float32 range;

    /// area effect
    /// the type data's constructors
    [offset((ee)0x424, (aoc)0x128), found(c)]
    float32 area;

    /// the type data's constructors
    [offset((ee)0x42C, (aoc)0x130), found(c)]
    float32 unknown_42c;

    /// the type data's constructors
    [offset((ee)0x430, (aoc)0x134), found(c)]
    float32 unknown_430;

    /// base accel (a projectile's: its fall)
    /// the type data's constructors
    [offset((ee)0x438, (aoc)0x13C), found(c)]
    float32 accel;

    /// the most it rolls in a turn (radians), with banks
    [offset((ee)0x454, (aoc)0x158), found(c)]
    float32 bank_angle;

    /// radians added to its heading in the turn step
    [offset((ee)0x458, (aoc)0x15C), found(c)]
    float32 heading_offset;

    /// its selection circle's size (tiles)
    /// EEE3DCommandConfirmation vf12's read (0x4E28E9 / AoC 0x4E9F49) - a circle's scale for the unit (float)
    [offset((ee)0x464, (aoc)0x168), found(c)]
    float32 mark_size;

    /// two floats the units' fill (EEROUnits' FillUnit) gives a record with hit points (ee_draw_record.bar_x / bar_y)
    /// [guess: the health bar's place]
    [offset((ee)0x468, (aoc)0x16C), found(c), guess]
    float32 bar_x;

    [offset((ee)0x46C, (aoc)0x170), found(c), guess]
    /// the record's bar_y (beside bar_x): the fill copies it while the unit has hit points
    float32 bar_y;

    /// the units' fill turns its record to the camera's yaw (an object without a complex unit), or its complex unit's
    /// up tilted toward it [guess]
    [offset((ee)0x5FA, (aoc)0x2F6), found(c), guess]
    uint8 faces_camera;

    /// another type whose stats' graphics the units' fill gives the record too (ee_draw_record.gfx_built) - a
    /// building site's finished building [guess]
    [offset((ee)0x400, (aoc)0xFC), found(c), guess]
    EEUnitClass* built_type;

    /// EE's: a power bar shown (the record's power_bar from the complex unit's power over its stats'); AoC tests the
    /// stats' power itself
    [offset((ee)0x57C, (aoc)none), found(c), guess]
    int32 has_power;

    /// its room in a formation (dreadnought 4, cruiser 3; the data's column 0x200, the community editor's "Size in
    /// formation", copied by the constructor 0x61C35B): the formations lay their slots out by the members' largest
    /// (EEFCircle / EEFLine / EEFWedge::vf2 0x4D7235 / 0x4D79D9 / 0x4D8079, EECPAttackFormation::vf15 0x45257A); the
    /// debug message EEMChangeSelectedUnitCollisionSphere (vf10 0x4B4AF6) edits it as its second sphere (mode 10).
    /// The spec called it collision until 2026-10-25. AoC +0x174 (the shift of +0x474)
    [offset((ee)0x470, (aoc)0x174), found(c)]
    float32 formation_size;

    /// its collision sphere (dreadnought 3, cruiser 1.95; the data's column 0x204, the editor's "Size of collider"):
    /// the debug message EEMChangeSelectedUnitCollisionSphere (vf10 0x4B4AF6, mode 9) edits it and copies it into the
    /// unit's size (+0x244); the movement's waypoint and arrival radius (EEUnitClass::vf34; x unit +0x240 at 0x5F0000)
    [offset((ee)0x474, (aoc)0x178), found(c)]
    float32 collider_size;

    /// the game data's "Unit type" (dbobjects column 36): the class of the type data - EEUCBuilding, EEUCNaval ... (the
    /// spec called it family until 2026-10-25, and the family category)
    [offset((ee)0x4B4, (aoc)0x1B8), found(c)]
    unit_class class_id;

    /// the type (UT(...))
    [offset((ee)0x4B8, (aoc)0x1BC), found(c)]
    int32 id;

    /// the damage table's class
    /// the type data's constructors
    [offset((ee)0x4C0, (aoc)0x1C4), found(c)]
    int32 damage_class;

    /// the game data's "Family ID" (the armour and bonus class, unit_names.h's KIND_ - kind.towers; the spec's category
    /// until 2026-10-25): as a target the hit table's row
    /// the type data's constructors
    [offset((ee)0x4C4, (aoc)0x1C8), found(c)]
    int32 family;

    /// ammo type (the hit table's weapon kind)
    /// the type data's constructors
    [offset((ee)0x4C8, (aoc)0x1CC), found(c)]
    int32 weapon;

    /// its RPS class (the scenario's object class 72 + it); the attack scoring tests 3, 10, 20
    [offset((ee)0x4CC, (aoc)0x1D0), found(c)]
    int32 rps_class;

    /// the RPS classes that counter it, the first the best (0: none; Protryon's mCounterKind1-3): the unit selector's
    /// counter vote (0x49352A -> 0x495B40) gives 3, 2 and 1 to these classes for each type of its target it knows of
    [offset((ee)0x4D0, (aoc)0x1D4), found(c)]
    int32 counter_1;

    /// the second class that counters it (counter_1)
    [offset((ee)0x4D4, (aoc)0x1D8), found(c)]
    int32 counter_2;

    /// the third class that counters it (counter_1)
    [offset((ee)0x4D8, (aoc)0x1DC), found(c)]
    int32 counter_3;

    /// the tech that makes it (its price)
    /// the type data's constructors
    [offset((ee)0x4E0, (aoc)0x1E4), found(c)]
    int32 tech;

    /// base reload time (ms)
    /// the type data's constructors
    [offset((ee)0x50C, (aoc)0x210), found(c)]
    int32 reload;

    /// base attack
    /// the type data's constructors
    [offset((ee)0x510, (aoc)0x214), found(c)]
    int32 attack;

    /// the influence table counts its units only while this is set (the world's vf10 0x5313D6 / 0x6171E3 test it
    /// before EEInfluenceTable.add; AoC 0x53C4CE) - what it holds is not read [guess: the name from that use]
    [offset((ee)0x540, (aoc)0x244), found(c), guess]
    int32 influence;

    /// a corpse's rotting time (ms): caps the unit's +0x2A8
    [offset((ee)0x548, (aoc)0x248), found(c)]
    int32 rot_time;

    /// (0 for soldiers)
    /// the type data's constructors
    [offset((ee)0x54C, (aoc)0x24C), found(c)]
    int32 transport_capacity;

    /// the type data's constructors
    [offset((ee)0x550, (aoc)0x250), found(c)]
    int32 garrison_capacity;

    /// the room it takes aboard (the load goal's vf33 sums it)
    /// 2 votes, the shift of its neighbours
    [offset((ee)0x558, (aoc)0x258), found(c)]
    int32 transport_size;

    /// base hit points
    /// the type data's constructors
    [offset((ee)0x55C, (aoc)0x25C), found(c)]
    int32 hp;

    /// its name's string in the language file (the scenario editor's unit list shows it, 0x5899BC)
    [offset((ee)0x560, (aoc)0x260), found(c)]
    int32 name_text;

    /// the data's column 0x198 "Resource type": lets_pass (0x6167C7) and 0x48xxxx test 3 (wood: trees) with the other
    /// type's +0x617 (the data's "Can walk through trees")
    [offset((ee)0x568, (aoc)0x268), found(c)]
    int32 resource_type;

    /// missile delay time
    /// the type data's constructors
    [offset((ee)0x588, (aoc)0x288), found(c)]
    int32 missile_delay;

    /// its footprint (tiles a side)
    [offset((ee)0x58C, (aoc)0x28C), found(c)]
    int32 size;

    /// a new aircraft's ammo (the scenario's Create 0x5ACD49 gives it this; unit attribute Ammunition's 100%)
    [offset((ee)0x5A0, (aoc)0x29C), found(c)]
    int32 ammo;

    /// bit 0 an aircraft, bit 2 a ship (1 and 3 tested too)
    [offset((ee)0x5D4, (aoc)0x2D0), found(c)]
    uint8 kind_flags;

    /// a byte per terrain kind (TILE_KIND): may the type enter it (the path search's tile test 0x41D36A; AoC 0x41990C
    /// asks 0x634AD8, which reads it only without a unit - with one a bit mask in its stats +0x160)
    [offset((ee)0x5D6, (aoc)0x2D2), found(c)]
    block terrain_ok;

    /// each shot takes one of the unit's ammo
    [offset((ee)0x5E8, (aoc)0x2E4), found(c)]
    uint8 uses_ammo;

    /// its shots take the high ballistic solution
    [offset((ee)0x5E9, (aoc)0x2E5), found(c)]
    uint8 high_arc;

    /// its model pitches with the ground (0x51216A: the ground 0.65 ahead against 0.65 behind) where it is put
    /// (EEUnit.stand) and on its land movement's steps (EEPMLand::vf20) - the AA guns' legs on a hill
    [offset((ee)0x5F3, (aoc)0x2EF), found(c)]
    uint8 pitches;

    /// its model rolls with the ground across it (0x512257, as pitches sideways)
    [offset((ee)0x5F4, (aoc)0x2F0), found(c)]
    uint8 banks;

    /// its heading turned by pi (the model faces backwards)
    [offset((ee)0x5F5, (aoc)0x2F1), found(c)]
    uint8 faces_back;

    /// 1: its units let no one pass (buildings), 0 ships and land units
    [offset((ee)0x600, (aoc)0x2FC), found(c)]
    uint8 blocks;

    /// may ride a helicopter transport (CLS_HELICOPTER_TRANSPORT)
    [offset((ee)0x601, (aoc)0x2FD), found(c)]
    uint8 boards_helicopter;

    /// may ride a land transport (CLS_LAND_TRANSPORT)
    [offset((ee)0x602, (aoc)0x2FE), found(c)]
    uint8 boards_land_transport;

    /// may board a transport ship (CLS_NAVAL_TRANSPORT)
    [offset((ee)0x603, (aoc)0x2FF), found(c)]
    uint8 boards_ship;

    /// may go into a fortress (CLS_FORTRESS; AoC's +0x300 its space transports)
    [offset((ee)0x604, (aoc)0x301), found(c)]
    uint8 boards_fortress;

    /// the data's column 0x245 (the editor: Can be owned by "World"; resources, scaffolding, ambient): the subgroup's
    /// target filter 0x446124 skips such objects unless they are armed and the stance is 2
    /// (the AoC decompilation: mIsGaiaPlaceable)
    [offset((ee)0x609, (aoc)0x306), found(c), guess]
    uint8 world_ownable;

    /// a wall: set after the load (0x62188D) as membership of unit set 9 (0x4A7618(type, 9)), dbunitset.dat's "Walls 1"
    /// - the wall pieces of every epoch (467..472) and the castle tower (176), continued in set 10 "Walls 2" (barbed
    /// wire 178, the castle wall 40); with is_guard_tower in freeze (0x5AF0BB), death and state (0x63862x, 0x60D36C),
    /// the attack scoring and the boarding; build_gate (0x6022E3) converts the objects with it on its tiles, the wall
    /// manager (0x498017) lays a gate where a finished object has it and not +0x61E, the path planners take it for a
    /// wall unless a gate (the AoC decompilation: mIsWall, kAITileWall)
    [offset((ee)0x612, (aoc)0x310), found(c)]
    uint8 is_wall;

    /// the data's column 0x248 (the editor: "You can kill this object with Delete key"; 0 for 24 types): a unit's
    /// +0x334 starts as it (0x5E6024); freeze clears that and unfreeze restores it from here (0x5AF0BB, 0x5B32CB)
    /// (the AoC decompilation: mIsDeletable)
    [offset((ee)0x613, (aoc)0x311), found(c), guess]
    uint8 deletable;

    /// a remembered building with it goes into the see-through sort map (the units' collection's walk, 0x50414D; the
    /// AoC decompilation: mIsObstacle)
    [offset((ee)0x618, (aoc)0x316), found(c), guess]
    uint8 is_obstacle;

    /// drawn as a tree: the units' collection gives it the small record (EEROUnits.fill_object, hooks.draw_small_old)
    /// instead of the object's (the AoC decompilation: mIsTree)
    [offset((ee)0x619, (aoc)0x317), found(c)]
    uint8 is_tree;

    /// a flare (the AoC decompilation's mIsFlare): the units' view test (EEROUnits.drawn) draws none
    [offset((ee)0x626, (aoc)0x324), found(c)]
    uint8 is_flare;

    /// the data's column 0x3B0 (the editor: "Melee attack"): EECPAttackSubGroupUnit::score 0x461854 picks its table 0
    /// by it (the AoC decompilation: mIsMelee)
    [offset((ee)0x61A, (aoc)0x318), found(c), guess]
    uint8 melee;

    /// the constructor sets 1 only for ids 47 and 461..466, the guard towers (AoC also 0x2FD, 0x302, 0x312, 0x322);
    /// attackers of rps_class 20 score a bonus against it
    [offset((ee)0x61E, (aoc)0x31C), found(c)]
    uint8 is_guard_tower;

    /// takes a behaviour (stances)
    /// the type data's constructors
    [offset((ee)0x621, (aoc)0x31F), found(c)]
    uint8 behaviours;

    /// thiscall(attacker class, attacker)
    [address((ee)0x0061E11C, (aoc)0x00631312)]
    // the calls of unit_type_fire (aim.c)
    [call_site("fire", (ee)0x00606067, (aoc)0x00617838)]
    [call_site("fire", (ee)0x00607D79, (aoc)0x00619604)]
    [call_site("fire", (ee)0x0060ABB0, (aoc)0x0061D174)]
    [call_site("fire", (ee)0x0060B33C, (aoc)0x0061D997)]
    extern void fire(EEUnit* u);

    /// vf42 (AoC 43): a missile launched, thiscall(class, missile, aim) - ret 8; the base's returns 0 (0x475908, a
    /// stub the linker shares), EEUCMissile's (and EEUCTorpedo's) launches
    [slot((ee)42, (aoc)43)]
    extern int32 launch(EEUnit* m, float32* aim);

    /// vf12: a new unit of this type for a player on tile (x, y), thiscall(type, x, y, player, 0, 0) - ret 0x14; what
    /// the scenario's Create (0x5ACD49) and the new-unit cheat's buildings (EEMHackANewUnit::vf10 0x4B7044) make one
    /// with. Without an id yet and not placed: the Create goes on with EEComplexUnit.set_built, the player's adopt,
    /// the world's settle and the map's place (game/api/unit.c unit_create)
    [slot(12)]
    extern EEUnit* create(int32 x, int32 y, int32 player, int32 p4, int32 p5);

    /// vf13: a new unit of this type for a player at matrix m (a GETransformation), thiscall(type, m, player, 0, 0) - placed,
    /// what EESEAPlaceUnit's vf1 makes each with
    [slot(13)]
    extern EEUnit* place_at(float32* m, int32 player, int32 p3, int32 p4);

    /// vf14: a new unit of this type for a player on tile (x, y) of a map, turned by angle (radians), thiscall(type,
    /// player, x, y, map, angle) - what the random map lays its resources with (0x66F0D5, for the world player; AoC
    /// 0x687636 the same slot): into the map it is given, which needn't be the running game's yet
    [slot(14)]
    extern EEUnit* create_on(EEPlayer* pl, int32 x, int32 y, EEMap* map, float32 angle);

    /// vf15: the same at a point (tiles, a tile's middle x + 0.5) - the random map's start units (0x66B921: the
    /// script's PlaceUnit lines round each start position; AoC 0x684FD9 the same slot), which then get
    /// EEComplexUnit.set_built (1, 0, 0) and paid_for on their physics. NULL: none made
    [slot(15)]
    extern EEUnit* create_at(EEPlayer* pl, float32 x, float32 y, EEMap* map, float32 angle);

    /// vf1: thiscall(type, float pos[3]) - the units' fill moves an object without a complex unit's place by it (its
    /// tile's middle and the ground's height in) [guess: what it changes]
    [slot(1)]
    extern void place_drawn(float32* pos);

    /// vf21: a building on land (the AoC decompilation's IsLandBuilding) - the units' opacity (EEROUnits.opacity)
    /// fades one that comes near the camera's front plane
    [slot(21)]
    extern uint8 is_land_building();
}

/// a missile's type data (EEUCMissile; EEUCTorpedo derives from it and keeps its launch)
[@base(typeof(EEUnitClass)), c_type("ee_unit_type"), vtable((ee)0x00832E90, (aoc)0x00848CF8)]
unsafe struct EEUCMissile
{
    /// its launch (the aim unused)
    [address((ee)0x00606109, (aoc)0x0061799B), overrides(typeof(EEUnitClass))]
    [vtable_site("missile_launch")]
    extern int32 launch(EEUnit* m, float32* aim);
}

/// a projectile's type data (EEUCProjectile; EEUCBomb derives from it and keeps its down)
[@base(typeof(EEUnitClass)), c_type("ee_unit_type"), vtable((ee)0x00833060, (aoc)0x00848ED0)]
unsafe struct EEUCProjectile
{
    /// vf55 (AoC 56), a slot of its own (EEUnitClass has 55): the projectile has come down - thiscall(class,
    /// projectile)
    [slot((ee)55, (aoc)56), address((ee)0x006078F9, (aoc)0x00619143)]
    [vtable_site("proj_down")]
    extern char8 down(EEUnit* projectile);
}

/// a priest's type data (EEUCBasicPriest; EEUCLandPriest, EEUCHelicopterPriest and EEUCNavalPriest derive from it and
/// keep its can_convert)
[@base(typeof(EEUnitClass)), c_type("ee_unit_type"), vtable((ee)0x0083177C, (aoc)0x00847144)]
unsafe struct EEUCBasicPriest
{
    /// vf55, a slot of its own (EEUnitClass has 55): whether priest may convert target now, thiscall(priest's class,
    /// priest, target, &immune) - ret 0xC. Not its own player's, one it is at war with, the priest's conversion above 0,
    /// the target alive and of a type that can be converted (stats +0x88 below 1), the to-hit table's entry for the
    /// pair, a priest only with the stats' +0x15F and a building only with +0x15E (the techs that allow them), the
    /// priest's power at least the conversion's cost (0x5FFEEA), and the target not in a University's area: its area
    /// effect of kind 4 (EEAEConversionImmunity; *immune 1 then) unless either type has +0x622 set. The game asks it
    /// in the player's cursor (0x64F...), the units' own targeting (0x5DDC8C) and the answer to an attacker (the
    /// scripts' AttackerIsLegalConversionTarget) - not when a conversion goal is given or completes (0x6000C6), so a
    /// goal given directly converts in a University's area. AoC made it a plain function: priest_can_convert
    [slot((ee)55, (aoc)none), address((ee)0x00600381, (aoc)none)]
    extern char8 can_convert(EEUnit* priest, EEUnit* target, char8* immune);
}

/// a unit type's per-player values, a unit's +0x34 (range, attack ...): EEUnitClassUpgrade, the type data's values
/// with the player's improvements - the per-type upgrades bought at a building (attack, hit points, range, speed, the
/// armours ...; not the tech tree's). Made by 0x627765(type) (AoC 0x63B6AE); +0x4 holds an EEUnitClassImprovement
/// (its levels and points). A buy: the button (EEUIBUpgrade) queues EEAUpgradeImprovement, which sends
/// EEMImproveUnit; its execute pays and queues it (EEPlayer.queue_improvement), the player's update counts the queue's
/// front down by the research time (dbworld.dat record 0x39 + attribute: 24-90 s the first, a quarter more later)
/// and applies it (EEPlayer.apply_improvement -> change_level): the level up by one, the stat by the table's step x
/// the type's base value (at least 1, the area and speed's 0.1 / 0.05), the player's live units of the type rescaled.
/// The price: the type's own price x the level (counting the queue) x the table's price factor. Limits: the table's
/// max_level buys of an attribute, ee_game_data.improvement_points points (an armour 1, the rest 2) a type, and
/// game.improvements_allowed. A line upgrade carries the levels over to the new type (copy_levels_to). The game's
/// computer player buys them only as an attack group forms (EECPAttackGroup.buy_improvements: paid and applied at
/// once, no research time). Read in both builds' code 2026-11-15; the layout checked in a running game
[c_type("ee_unit_stats"), vtable((ee)0x00833A60, (aoc)0x00849AD8)]
unsafe struct EEUnitClassUpgrade
{
    /// its improvements' levels by attribute (enum improvement), 1 at the start: each level above 1 one bought
    /// (EEUnitClassImprovement +0x4)
    [offset(0x8), found(c)]
    fixed int32 levels[16];

    /// the improvement points it has spent (EEUnitClassImprovement +0x44): with the queue's at most
    /// ee_game_data.improvement_points. Reapplying after a tech tree reload (0x55A912, AoC 0x569990) adds the points
    /// again without clearing them first [read, not tested]
    [offset(0x48), found(c)]
    int32 points;

    /// thiscall(stats, other): other's levels raised by this one's above 1, the stats with them - a line upgrade's
    /// carry-over (0x5BA59D: the old type's stats to the new type's)
    [slot(1), address((ee)0x00627855, (aoc)0x0063B7A4)]
    extern void copy_levels_to(EEUnitClassUpgrade* other);

    /// an improvement raised (+1) or lowered (-1) now: EEUnitClassImprovement.change_level on its +0x4
    [address((ee)0x0062792B, (aoc)0x0063B87A)]
    extern void change_level(int32 attr, int32 delta);

    /// the values refilled from the type data, then the levels applied again (EEUnitClassImprovement.reapply)
    [address((ee)0x00627942, (aoc)0x0063B891)]
    extern void reset();

    /// its name (UWideString): the units' fill shows it under the unit under the mouse
    [offset(0x4C), found(c)]
    wstring name;

    /// between fields read through a unit's +0x34, unchanged
    [offset(0x60), found(b)]
    EEUnitClass* type_data;

    /// the unit's graphics (EEGraphicObject: RTTI in a running game, every unit's): EEUnitClass::vf29 0x61AD41 reads
    /// its +0x318 + animation x 4 (AoC +0x328), a duration per animation (anim_ms's random start)
    [offset(0x68), found(c)]
    EEGraphicObject* gfx;

    /// read through a unit's +0x34
    [offset(0x98), found(c)]
    float32 height;

    /// tiles
    /// read through a unit's +0x34
    [offset(0x9C), found(c)]
    float32 range;

    /// the area its shots hit
    /// read through a unit's +0x34
    [offset(0xA0), found(c)]
    float32 area;

    /// line of sight (tiles)
    [offset(0xA4), found(c)]
    float32 sight;

    /// acceleration (EEUnit::accel)
    [offset(0xA8), found(c)]
    float32 accel;

    /// tiles/s (0: cannot move)
    /// read through a unit's +0x34
    [offset(0xAC), found(c)]
    float32 speed;

    /// turn acceleration (EEUnit::turn_accel)
    [offset(0xB0), found(c)]
    float32 turn_accel;

    /// the top turn rate, radians/s (EEUnit::max_turn)
    [offset(0xB4), found(c)]
    float32 turn_rate;

    /// its projectile's launch speed
    /// between fields read through a unit's +0x34, unchanged
    [offset(0xC0), found(b)]
    float32 shot_speed;

    /// read through a unit's +0x34
    [offset(0xD8), found(c)]
    float32 attack;

    /// leadership (the type's +0x538; improvement leadership)
    [offset(0xDC), found(c)]
    float32 leadership;

    /// the armour against each damage class, float[6]: [0] shock, [1] arrow, [2] pierce, [3] gun, [4] laser, [5]
    /// missile (the improvements' change_level writes each of them)
    [offset(0xE0), found(c)]
    block armor;

    /// an aircraft's flight time, ms (the scenario's Create gives a new one this)
    [offset(0x130), found(c)]
    int32 flight_ms;

    /// ms between shots
    /// read through a unit's +0x34
    [offset(0x138), found(c)]
    int32 reload;

    /// its full hit points
    /// read through a unit's +0x34
    [offset(0x144), found(c)]
    int32 hp;

    /// a new unit's power (a priest's, a prophet's; the scenario's Create gives it this when more than 0)
    [offset(0x148), found(c)]
    int32 power;

    /// what it carries (the type's +0x570; improvement transport)
    [offset(0x14C), found(c)]
    int32 transport;

    /// it may lob over a wall: a shot of its that hits a wall in the way sets its unit's alt_attack, and with this the
    /// unit then fires the high arc (ballistic_game) at 1.5 x its reload, its shots passing walls (the flight's check
    /// 0x61F6A4 skips group 0x2A objects only for a shooter with this and alt_attack); the AI's target choice skips
    /// the line of fire test (line_of_fire_game) for it as for a high_arc type. Longbows have it (the player: they lob
    /// after their first shot hits a wall; tests/probes/wall_archers.eeai). AoC: its target choice reads +0x164 beside
    /// the type's high_arc +0x2E5, ballistic_game the same
    [offset((ee)0x15C, (aoc)0x164), found(c)]
    uint8 lob;
}

/// the attack bonuses by weapon and target, unit_type.bonus_table - a static object (EEToHitTable)
[c_type("ee_to_hit_table"), vtable((ee)0x0082C920, (aoc)0x008420C0)]
unsafe struct EEToHitTable
{
    /// thiscall(unit_type.bonus_table, weapon kind, target type data, attacker, target player)
    [address((ee)0x004D5F2C, (aoc)0x004DBE44)]
    extern float32 attack_bonus(int32 weapon, void* target_type, int32 attacker, int32 target_player);
}

/// a unit type's class_id, the game data's "Unit type": the class of its type data, one each (EEUC + the name: 4
/// EEUCNaval) - every type's read in a running game of both builds 2026-10-25
[c_prefix("CLS")]
enum unit_class : int32
{
    ambient = 0,
    resource = 1,
    building = 2,
    airplane = 3,
    /// the warships (and the fish)
    naval = 4,
    /// the land units, siege included
    land = 5,
    land_citizen = 6,
    helicopter = 7,
    projectile = 9,
    /// the submarines
    submerged_naval = 10,
    bomb = 11,
    missile = 12,
    land_priest = 13,
    land_transport = 16,
    helicopter_transport = 17,
    /// the transport ships
    naval_transport = 18,
    torpedo = 20,
    /// the aircraft carriers
    naval_building = 21,
    /// the fishing boats
    naval_citizen = 22,
    land_military_hero = 23,
    gate = 25,
    helicopter_priest = 26,
    sea_animal = 27,
    air_animal = 28,
    fortress = 29,
    /// AoC's own classes
    building_priest = 30,
    spaceship = 31,
    space_building = 32,
    space_transport = 33,
    planet_spaceship = 34,
}

/// a unit type's improvement levels for one player (EEUnitClassImprovement, 0x48 bytes), held at EEUnitClassUpgrade
/// +0x4 (its levels and points there)
[c_type("ee_improvements"), vtable((ee)0x00833A54, (aoc)0x00849ACC)]
unsafe struct EEUnitClassImprovement
{
    /// thiscall(impr, type, stats, attr, delta): levels[attr] and the points moved by delta, the stat by delta x the
    /// step (the table's step x the type's base value, at least the attribute's least step)
    [address((ee)0x00627078, (aoc)0x0063AFC1)]
    extern void change_level(EEUnitClass* type, EEUnitClassUpgrade* stats, int32 attr, int32 delta);

    /// thiscall(impr, type, stats): each level set back to 1 and applied again (level - 1 steps) to freshly filled
    /// stats - the points are added again
    [address((ee)0x00627412, (aoc)0x0063B35B)]
    extern void reapply(EEUnitClass* type, EEUnitClassUpgrade* stats);
}

/// a unit type's improvements, db\dbupgrade.dat's row (EEUCUpgradeTable, 0x88 bytes): the file a count (2017 update
/// 69, AoC 76) and records of 0xF0 - the name, the steps, the price factors, max_level - that the loader (0x4D67CE,
/// AoC 0x4DC6F0) puts into factors[attr]; the constructor (0x4D67A9, AoC 0x4DC6CB) sets every price factor 1, every
/// step 0. Spearman: attack, hit points 0.15 x 2, pierce and shock armour 0.15 x 0.75, speed 0.1 x 2, max 2; Tower:
/// range 0.15 x 1, max 1; Building: max 0
[c_type("ee_upgrade_table"), size(0x88), vtable((ee)0x0082AB04, (aoc)0x0083F800)]
unsafe struct EEUCUpgradeTable
{
    /// float[2] per attribute (enum improvement): +0 the price factor, +4 the step (a share of the type's base value
    /// per level; 0: not offered)
    [offset(0x4), found(c)]
    block factors;

    /// buys per attribute: a level (from 1, the queue counted) may be bought while it is at most this
    [offset(0x84), found(c)]
    int32 max_level;
}

/// an improvement's attribute (EEUnitClassUpgrade.levels, EEUIBUpgrade.attr, EEMImproveUnit.attr): the switch of
/// EEUnitClassImprovement.change_level and the names 0x626E74 (strings 30000+)
[c_prefix("IMP")]
enum improvement : int32
{
    /// stats.area (the type's +0x424), steps of at least 0.1
    area = 0,
    /// stats.attack (+0x510)
    attack = 1,
    armor_arrow = 2,
    armor_gun = 3,
    armor_laser = 4,
    armor_missile = 5,
    armor_pierce = 6,
    armor_shock = 7,
    /// stats.flight_ms when the type has a flight time (its +0x534 above 0), else stats.power
    flight_or_power = 8,
    /// stats.hp (+0x55C), the live units' hit points rescaled
    hp = 9,
    leadership = 10,
    /// stats.range (+0x420) and the sight with it
    range = 11,
    /// the type's +0x554 to stats +0x140 (what it does not read); no table has a step for it
    repair = 12,
    /// stats.speed (+0x430), steps of at least 0.05
    speed = 14,
    transport = 15,
}

/// unit_type's data of the game, places in its code and numbers
static unsafe class unit_type
{
    /// the improvements' tables by their dbupgrade.dat index (EEUCUpgradeTable* vector: its begin, the end after it)
    [address((ee)0x008D202C, (aoc)0x008E872C)]
    static block upgrade_tables;

    /// attack_bonus_game's object
    [address((ee)0x00917DE0, (aoc)0x009318E8)]
    static EEToHitTable bonus_table;

    /// EEUnitClass: thiscall(type data, unit), as the game's goal assignment
    [value(29)]
    static uint32 slot_type_assign;

    /// EEUCTransport: char thiscall(type data, transport, unit) - its family against the unit type's board flags
    /// (0x609095; AoC put a stub at 60)
    [value((ee)60, (aoc)61)]
    static uint32 slot_type_can_carry;

    /// AoC's EEUCBasicPriest.can_convert, a plain function there: cdecl(priest, target, &immune), the same tests and
    /// a few of its own (a priest of class 0x1E only within its range); called where the 2017 update calls the
    /// slot (the cursor 0x665E89, the answer to an attacker EEUCBasicPriest::vf40, eight callers)
    [address((ee)none, (aoc)0x0060DA86), cdecl]
    static extern char8 priest_can_convert(EEUnit* priest, EEUnit* target, char8* immune);

    /// the way an Airport's three runways run from its hangar with its square's low corner at tile x, y: 0 +y, 1 -y,
    /// 2 +x, 3 -x - towards the map's middle (2 at the middle itself), by the angle of the way there: within 45
    /// degrees of an axis that axis, a tie with +y to +y, one with -y to the x axis. The player's preview (ghost_unit.preview, its ghosts: the
    /// hangar's tile + the runway's size x 1, 2, 3 that way), the game's laying of the runways (EEUCBuilding::vf17
    /// 0x601041, its footprint 0x630xxx) and its computer player's search for a spot (0x466xxx, the whole strip at
    /// once) all ask it. Stdcall here (ret 8); AoC's is airport_facing_aoc
    [address((ee)0x00600E64, (aoc)none), stdcall]
    static extern int32 airport_facing(int32 x, int32 y);

    /// AoC's airport_facing, cdecl: on a map of areas (map +0x18F4A0, Small Planets' planets) towards the middle of
    /// the area x, y is in (its +0x44, +0x48) instead of the map's - the 2017 update's rule taken for it pointed the
    /// AI's test at another side of the hangar than the one its runways were laid on (2026-11-24)
    [address((ee)none, (aoc)0x0060EBDA), cdecl]
    static extern int32 airport_facing_aoc(int32 x, int32 y);

    /// the Airport's type id, as the game's code names it (the ghosts' create 0x5F4608 and preview, vf17, the
    /// facing's callers; AoC the same): its hangar, the piece the player and the AI lay
    [value(0x42)]
    static uint32 type_airport;

    /// the runway piece's type id (b_airport_runway), three of them beside an Airport's hangar
    [value(0xF0)]
    static uint32 type_runway;
}
