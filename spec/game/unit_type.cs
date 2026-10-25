/// a unit type's data, game.types[type] and a unit's +0x2C (EEUnitClass, EEUC*)
[c_type("ee_unit_type"), c_prefix("type")]
unsafe struct EEUnitClass
{
    [offset(0x0), found(c)]
    uint8* vtable;

    /// its data name
    [offset(0x1C), found(v)]
    char8* name;

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

    /// its selection circle's size (tiles)
    /// EEE3DCommandConfirmation vf12's read (0x4E28E9 / AoC 0x4E9F49) - a circle's scale for the unit (float)
    [offset((ee)0x464, (aoc)0x168), found(c)]
    float32 mark_size;

    /// a collision sphere (float; dreadnought 4, cruiser 3): AoC the shift of +0x474 (0x178, EEUnitClass::vf34's
    /// arrival test)
    [offset((ee)0x470, (aoc)0x174), found(c)]
    float32 collision;

    /// 2 building, 29 fortress, 5/6 units ...
    [offset((ee)0x4B4, (aoc)0x1B8), found(c)]
    int32 family;

    /// the type (UT(...))
    [offset((ee)0x4B8, (aoc)0x1BC), found(c)]
    int32 id;

    /// the damage table's class
    /// the type data's constructors
    [offset((ee)0x4C0, (aoc)0x1C4), found(c)]
    int32 damage_class;

    /// as a target (the hit table's row)
    /// the type data's constructors
    [offset((ee)0x4C4, (aoc)0x1C8), found(c)]
    int32 category;

    /// ammo type (the hit table's weapon kind)
    /// the type data's constructors
    [offset((ee)0x4C8, (aoc)0x1CC), found(c)]
    int32 weapon;

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

    /// a byte per terrain kind (TILE_KIND): may the type enter it (the path search's tile test 0x41D36A; AoC 0x41990C
    /// asks 0x634AD8, which reads it only without a unit - with one a bit mask in its stats +0x160)
    [offset((ee)0x5D6, (aoc)0x2D2), found(c)]
    block terrain_ok;

    /// takes a behaviour (stances)
    /// the type data's constructors
    [offset((ee)0x621, (aoc)0x31F), found(c)]
    uint8 behaviours;
}

/// a unit type's per-player values, a unit's +0x34 (range, attack ...)
[c_prefix("stat")]
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

    /// tiles/s (0: cannot move)
    /// read through a unit's +0x34
    [offset(0xAC), found(c)]
    float32 speed;

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

/// unit_type's data of the game, places in its code and numbers
static unsafe class unit_type
{
    /// the minimap's operands of hooks.float_1_800
    [address((ee)0x00606067, (aoc)0x00617838)]
    [address((ee)0x00607D79, (aoc)0x00619604)]
    [address((ee)0x0060ABB0, (aoc)0x0061D174)]
    [address((ee)0x0060B33C, (aoc)0x0061D997)]
    static code fire_calls;

    /// attack_bonus_game's object
    [address((ee)0x00917DE0, (aoc)0x009318E8)]
    static block bonus_table;

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
    static block vt_missile;

    /// EEUCProjectile
    [address((ee)0x00833060, (aoc)0x00848ED0)]
    static block vt_projectile;
}

/// the game's functions unit_type.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class unit_type_functions
{
    /// thiscall(unit_type.bonus_table, weapon kind, target type data, attacker, target player)
    [address((ee)0x004D5F2C, (aoc)0x004DBE44), thiscall]
    static extern float32 attack_bonus_game(void* p1, int32 p2, void* p3, int32 p4, int32 p5);

    /// thiscall(attacker class, attacker)
    [address((ee)0x0061E11C, (aoc)0x00631312), thiscall]
    static extern void unit_fire(void* cls, EEUnit* u);

    /// EEUCMissile vf42, thiscall(class, missile, aim) - ret 8, the aim unused
    [address((ee)0x00606109, (aoc)0x0061799B), thiscall]
    static extern int32 missile_launch(void* cls, EEUnit* m, float32* aim);

    [address((ee)0x006078F9, (aoc)0x00619143), thiscall]
    static extern char8 proj_down(void* cls, void* projectile);
}
