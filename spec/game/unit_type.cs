/// a unit type's data, game.types[type] and a unit's +0x2C (EEUnitClass, EEUC*)
[c_type("ee_unit_type")]
unsafe struct EEUnitClass
{
    [offset(0x0), found(c)]
    uint8* vtable;

    /// its data name
    [offset(0x1C), found(v)]
    char8* name;

    /// the graphic effect a missile or projectile plays on itself when it arrives
    [offset((ee)0x378, (aoc)0x68), found(c)]
    void* impact_gfx;

    /// the graphic effect on the attacker when it fires
    [offset((ee)0x38C, (aoc)0x7C), found(c)]
    void* fire_gfx;

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

    /// a collision sphere (float; dreadnought 4, cruiser 3): AoC the shift of +0x474 (0x178, EEUnitClass::vf34's
    /// arrival test)
    [offset((ee)0x470, (aoc)0x174), found(c)]
    float32 collision;

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

    /// missile delay time
    /// the type data's constructors
    [offset((ee)0x588, (aoc)0x288), found(c)]
    int32 missile_delay;

    /// its footprint (tiles a side)
    [offset((ee)0x58C, (aoc)0x28C), found(c)]
    int32 size;

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

    /// its model tilts with its speed
    [offset((ee)0x5F3, (aoc)0x2EF), found(c)]
    uint8 pitches;

    /// its model rolls in turns (bank_angle)
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
}

/// a unit type's per-player values, a unit's +0x34 (range, attack ...)
unsafe struct ee_unit_stats
{
    /// between fields read through a unit's +0x34, unchanged
    [offset(0x60), found(b)]
    EEUnitClass* type_data;

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

    /// between fields read through a unit's +0x34, unchanged
    [offset(0xE0), found(b)]
    block armor;

    /// ms between shots
    /// read through a unit's +0x34
    [offset(0x138), found(c)]
    int32 reload;

    /// its full hit points
    /// read through a unit's +0x34
    [offset(0x144), found(c)]
    int32 hp;
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

/// unit_type's data of the game, places in its code and numbers
static unsafe class unit_type
{
    /// attack_bonus_game's object
    [address((ee)0x00917DE0, (aoc)0x009318E8)]
    static EEToHitTable bonus_table;

    /// unit_type.vt_missile
    [value((ee)42, (aoc)43)]
    static uint32 slot_missile_launch;

    /// unit_type.vt_projectile
    [value((ee)55, (aoc)56)]
    static uint32 slot_proj_down;

    /// EEUnitClass: thiscall(type data, unit), as the game's goal assignment
    [value(29)]
    static uint32 slot_type_assign;

    /// EEUCTransport: char thiscall(type data, transport, unit) - its family against the unit type's board flags
    /// (0x609095; AoC put a stub at 60)
    [value((ee)60, (aoc)61)]
    static uint32 slot_type_can_carry;

    [address((ee)0x00832E90, (aoc)0x00848CF8)]
    static vftable vt_missile;

    /// EEUCProjectile
    [address((ee)0x00833060, (aoc)0x00848ED0)]
    static vftable vt_projectile;
}

/// the game's functions unit_type.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class unit_type_functions
{
    /// EEUCMissile vf42, thiscall(class, missile, aim) - ret 8, the aim unused
    [address((ee)0x00606109, (aoc)0x0061799B), thiscall]
    static extern int32 missile_launch(void* cls, EEUnit* m, float32* aim);

    [address((ee)0x006078F9, (aoc)0x00619143), thiscall]
    static extern char8 proj_down(void* cls, void* projectile);
}
