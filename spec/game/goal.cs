/// a unit's goal, its +0x1EC (EEUnitGoal, EEUG*)
[c_type("ee_goal")]
unsafe struct EEUnitGoal
{
    /// (aim.c: 2 while it attacks)
    [offset(0x5), found(b)]
    uint8 state;

    /// 0 from the base constructor 0x6283AB: +1 per path request for the goal (EEUnitClass::vf52 0x6222A8), +8 per
    /// detour waypoint pair (vf53 0x6225E7)
    [offset(0x18), found(c), guess]
    int32 path_tries;

    /// 1 for a goal from an order (commands, the AI), 0 for the unit's own (its target search); the guard return skips
    /// an ordered one
    [offset(0x1C), found(c)]
    uint8 ordered;

    /// 0 from the base constructor; the group dispatch 0x5DB641 (and 0x5E94C1, 0x5EF6A7) sets 1 on each member's goal:
    /// vf52 asks EEPathManager::request for a sector route only without it
    [offset(0x1D), found(c), guess]
    uint8 no_sectors;

    [offset(0x20), found(c)]
    float32 dest_x;

    /// the destination's y
    [offset(0x24), found(c)]
    float32 dest_y;

    /// the destination's z
    [offset(0x28), found(c)]
    float32 dest_z;

    /// a kind 6 goal's target
    /// a kind 6 goal's target - the contact test (EEPhysicsModel::vf14, AoC vf15) lets two units at work on kind 6 meet
    /// only when it is the same
    [offset(0x38), found(c)]
    void* _0x38;

    /// a kind 4 goal's target
    /// a kind 4 goal's target - the same for kind 4
    [offset(0x3C), found(c)]
    void* _0x3c;

    [offset(0x50), found(c)]
    EEUnit* holder;

    /// thiscall(goal 0xFBA object, unit) (0x5ED24B)
    [address((ee)0x0060FE7E, (aoc)0x006226CF)]
    // in 0x5ED24B: goal_fits on a dynamic_cast, unchecked
    [call_site("goal_fits", (ee)0x005ED9BF, (aoc)0x005FBA9C)]
    extern char8 fits(void* unit);

    /// the same goal's other constructor: thiscall(goal, float pt[3], 6 dwords - a float among them), the group's
    /// dispatch gives each unit its slot with it
    [address((ee)0x00614D47, (aoc)0x00627742)]
    // the group's dispatch (0x5DB641): each unit's location goal at its slot, goal_move_at_game
    [call_site("slot_goal", (ee)0x005DC902, (aoc)0x005ED231)]
    extern void* move_at_game(float32* p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8);

    /// EEUGUnloadTransport: new 0x40
    [address((ee)0x00615A1F, (aoc)0x006283FF)]
    // the calls of goal_unload_init (landing.c; the last, 0x615AE6, the unload goal's copy)
    [call_site("unload_ctor", (ee)0x00445AAD, (aoc)0x00444CCD)]
    [call_site("unload_ctor", (ee)0x00483E4D, (aoc)0x004884B9)]
    [call_site("unload_ctor", (ee)0x0048CC5D, (aoc)0x00491733)]
    [call_site("unload_ctor", (ee)0x0048D022, (aoc)0x00491AF8)]
    [call_site("unload_ctor", (ee)0x004BF82A, (aoc)0x004C86E9)]
    [call_site("unload_ctor", (ee)0x005B3E23, (aoc)0x005C4E31)]
    [call_site("unload_ctor", (ee)0x005DC81B, (aoc)0x005ED156)]
    [call_site("unload_ctor", (ee)0x00615AE6, (aoc)0x006284E2)]
    extern EEUnitGoal* unload_init(EEUnit* holder, int32 x, int32 y, int32 flag);

    /// thiscall(load goal, unit): the unit into its pending passengers (if not there), as EEMLoadTransport adds to a
    /// load under way
    [address((ee)0x0061321D, (aoc)0x00625AB4)]
    extern void add_passenger(EEUnit* u);
}

/// the build-or-repair goal (EEUGBuildRepair)
[@base(typeof(EEUnitGoal)), c_type("ee_build_goal"), vtable((ee)0x008319F0, (aoc)0x0084740C)]
unsafe struct EEUGBuildRepair
{
    /// its dump's Target Center
    [offset(0x2C), found(c)]
    vec3 center;

    /// the building's type data (its dump's Target Class)
    [offset(0x40), found(c)]
    EEUnitClass* target_class;

    /// the player's tech tree node of the type's tech
    [offset(0x44), found(c)]
    EETechTreeNode* tech_node;

    /// the type's tech (type +0x4E0; 100000 none)
    [offset(0x48), found(c)]
    int32 tech_id;

    /// the building's type (its dump's Target Type)
    [offset(0x4C), found(c)]
    int32 type_id;

    /// EEUGBuildRepair constructor: thiscall(goal, player, building, 1)
    [address((ee)0x0060BAD0, (aoc)0x0061E164)]
    extern EEUnitGoal* init(EEPlayer* pl, EEUnit* building, int32 flag);
}

/// the load-into-a-transport goal (EEUGLoadTransport)
[@base(typeof(EEUnitGoal)), c_type("ee_load_goal"), vtable((ee)0x00831DC0, (aoc)0x008477F8)]
unsafe struct EEUGLoadTransport
{
    /// EEUGLoadTransport: new 0x68
    [address((ee)0x0061487F, (aoc)0x00627286)]
    extern void* init(void* p2, void* p3, int32 p4);
}

/// the go-to-a-location goal (EEUGLocation)
[@base(typeof(EEUnitGoal)), c_type("ee_location_goal"), vtable((ee)0x00831E64, (aoc)0x008478A0)]
unsafe struct EEUGLocation
{
    /// the heading to face on arrival (its dump's Yaw)
    [offset(0x2C), found(c)]
    float32 yaw;

    /// a forced move; 0 an attack-move (vf8's AttackMove, vf21 zeroes the unit's scan_ms)
    [offset(0x30), found(c)]
    uint8 forced;

    /// Should Face: on arrival it turns to yaw (state 0x232E) instead of idling (5000)
    [offset(0x31), found(c)]
    uint8 face;

    /// Is Auto Waypoint: assign_goal doesn't make it the current goal
    [offset(0x32), found(c)]
    uint8 auto_waypoint;

    /// Use Group Movement (the copy vf4 carries it)
    [offset(0x33), found(c)]
    uint8 group_move;

    /// its dump's Group Path State
    [offset(0x34), found(c)]
    uint8 group_path_state;

    /// new 0x38
    [address((ee)0x00614CE6, (aoc)0x006276E1)]
    extern void* init(float32* p2, int32 p3, char8 p4, char8 p5, float32 p6, char8 p7, char8 p8);
}

/// goal's data of the game, places in its code and numbers
static unsafe class goal
{
    [address((ee)0x005F0634, (aoc)0x005FE8DA)]
    static code at_goal_set_read;

    /// EEUGAttack(unit, target, 1); AoC + a byte: attack.c
    [address((ee)0x0060AD45, (aoc)0x0061D337)]
    static code fn_goal_attack_ctor;

    /// goal.fn_goal_attack_ctor
    [value((ee)64, (aoc)68)]
    static uint32 size_goal_attack;

    /// goal_build_init
    [value(92)]
    static uint32 size_goal_build;

    /// goal_load_ctor
    [value(0x68)]
    static uint32 size_goal_load;

    /// goal_move_ctor
    [value(56)]
    static uint32 size_goal_move;

    /// goal_unload_init
    [value(64)]
    static uint32 size_goal_unload;

    /// EEUGLoadContainer: thiscall(goal, unit) - the unit out of its holder's load goal's pending passengers
    /// (0x613992; AoC 0x626229, the same code)
    [value(19)]
    static uint32 slot_goal_drop;

    /// EEUGLoadTransport: a unit's part of the load goal
    [value(4)]
    static uint32 slot_goal_part;

    /// EEUGoal: thiscall(goal, unit, float out[3]) - where the goal sends the unit (PrepareToMove 0x6222A8)
    [value(2)]
    static uint32 slot_goal_target;

    [address((ee)0x00831968, (aoc)0x00847380)]
    static vftable vt_goal_attack;

    [address((ee)0x008319F0, (aoc)0x0084740C)]
    static vftable vt_goal_build;

    /// EEUGLoadTransport: the holder's goal and each unit's part (0x614833), the holder at +0x50
    [address((ee)0x00831DC0, (aoc)0x008477F8)]
    static vftable vt_goal_load;

    [address((ee)0x00831E64, (aoc)0x008478A0)]
    static vftable vt_goal_location;

    /// EEUGUnloadTransport (new 0x40, ctor 0x615A1F(transport, x, y, flag))
    [address((ee)0x00831F5C, (aoc)0x008479A0)]
    static vftable vt_goal_unload;
}

/// the attack-a-unit goal (EEUGAttackUnit, 0x40 bytes; its constructor 0x60AD45 sets the attacker +0x2C, the target
/// +0x30 and the target's tile +0x34 / +0x38)
[@base(typeof(EEUnitGoal)), vtable((ee)0x00831968, (aoc)0x00847380)]
unsafe struct EEUGAttackUnit
{
    /// the unit it attacks, referenced (the constructor 0x60AD45); its readers take the target's hp, type and stats
    /// (0x5F1214, EEUCMissile / EEUCProjectile::vf39, 0x61B677)
    [offset(0x30), found(c)]
    EEUnit* target;
}

/// the unload-a-transport goal (EEUGUnloadTransport, new 0x40; its constructor 0x615A1F(transport, x, y, flag) -
/// goal.vt_goal_unload; its out-of-synch dump 0x615AF3 names its fields)
[@base(typeof(EEUnitGoal)), vtable((ee)0x00831F5C, (aoc)0x008479A0)]
unsafe struct EEUGUnloadTransport
{
    /// its dump's "First Cargo Tile" (0x615AF3); unload_init 0x615A1F zeroes it
    [offset(0x2C), found(c)]
    ivec2 first_cargo_tile;

    /// its dump's "Unload Tile", x (y at +0x38: the base's _0x38): unload_init stores x, y, 0x483D97 tries shore
    /// tiles into it, 0x48CE8F copies it into a new unload goal
    [offset(0x34), found(c)]
    int32 unload_tile_x;

    /// its dump's "Unload Range Squared": unload_init sets the transport type's +0x4AC squared (AoC type +0x1B0)
    [offset(0x3C), found(c)]
    float32 unload_range_sq;
}

/// the explore goal (EEUGExplore; its dump vf5 0x60ECF6)
[@base(typeof(EEUnitGoal)), vtable((ee)0x00831AE8, (aoc)0x0084750C)]
unsafe struct EEUGExplore
{
    /// its dump's "Tile Location X" (vf5 0x60ECF6): vf31 0x60F318 returns it + 0.5; the dispatch 0x5DD463 sets the
    /// unit's x
    [offset(0x3C), found(c)]
    int32 tile_x;

    /// its dump's "Tile Location Y" (the dispatch 0x5DD463 after a cast to EEUGExplore)
    [offset(0x40), found(c)]
    int32 tile_y;
}

/// the gather goal (EEUGGather)
[@base(typeof(EEUnitGoal)), vtable((ee)0x00831B80, (aoc)0x008475A8)]
unsafe struct EEUGGather
{
    /// 1 once it has found a drop-off and heads there (state 0x1F48, 0x6106F9), cleared at 0x610A3D, 0x610BB6 and in
    /// vf20: vf18 0x611065 takes the drop-off's reach test while it is set; the script condition 0x138F returns it
    [offset(0x40), found(c), guess]
    uint8 returning;

    /// 0x610BB6 sets 1 (returning 0) to walk to its spot at dest_x / dest_y: vf18 then tests the distance to it instead
    /// of the reach; the script condition 0x139B returns it
    [offset(0x41), found(c), guess]
    uint8 to_spot;
}
