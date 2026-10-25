/// a unit's goal, its +0x1EC (EEUnitGoal, EEUG*)
[c_type("ee_goal")]
unsafe struct EEUnitGoal
{
    /// (aim.c: 2 while it attacks)
    [offset(0x5), found(b)]
    uint8 state;

    [offset(0x20), found(c)]
    float32 dest_x;

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
}

/// goal's data of the game, places in its code and numbers
static unsafe class goal
{
    [address((ee)0x005F0634, (aoc)0x005FE8DA)]
    static code at_goal_set_read;

    [address((ee)0x005F1029, (aoc)0x005FF393)]
    static code call_goalset_again;

    [address((ee)0x005F0FFA, (aoc)0x005FF364)]
    static code call_goalset_ask;

    /// in 0x5ED24B: goal_fits on a dynamic_cast, unchecked
    [address((ee)0x005ED9BF, (aoc)0x005FBA9C)]
    static code call_goal_fba_check;

    /// the group's dispatch (0x5DB641): each unit's location goal at its slot, goal_move_at_game
    [address((ee)0x005DC902, (aoc)0x005ED231)]
    static code call_slot_goal;

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

/// the game's functions goal.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class goal_functions
{
    /// EEUGBuildRepair constructor: thiscall(goal, player, building, 1)
    [address((ee)0x0060BAD0, (aoc)0x0061E164), thiscall]
    static extern EEUnitGoal* goal_build_init(void* mem, EEPlayer* pl, EEUnit* building, int32 flag);

    [address((ee)0x00619FF5, (aoc)0x0062CD8B), thiscall]
    static extern int32 goalset_ask(void* set, EEUnit* u, int32* @out);

    /// thiscall(goal 0xFBA object, unit) (0x5ED24B)
    [address((ee)0x0060FE7E, (aoc)0x006226CF), thiscall]
    static extern char8 goal_fits(EEUnitGoal* goal, void* unit);

    /// EEUGLoadTransport: new 0x68
    [address((ee)0x0061487F, (aoc)0x00627286), thiscall]
    static extern void* goal_load_ctor(void* p1, void* p2, void* p3, int32 p4);

    /// the same goal's other constructor: thiscall(goal, float pt[3], 6 dwords - a float among them), the group's
    /// dispatch gives each unit its slot with it
    [address((ee)0x00614D47, (aoc)0x00627742), thiscall]
    static extern void* goal_move_at_game(void* p1, float32* p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8);

    /// new 0x38
    [address((ee)0x00614CE6, (aoc)0x006276E1), thiscall]
    static extern void* goal_move_ctor(void* p1, float32* p2, int32 p3, char8 p4, char8 p5, float32 p6, char8 p7, char8 p8);

    /// EEUGUnloadTransport: new 0x40
    [address((ee)0x00615A1F, (aoc)0x006283FF), thiscall]
    static extern EEUnitGoal* goal_unload_init(void* mem, EEUnit* holder, int32 x, int32 y, int32 flag);

    /// thiscall(load goal, unit): the unit into its pending passengers (if not there), as EEMLoadTransport adds to a
    /// load under way
    [address((ee)0x0061321D, (aoc)0x00625AB4), thiscall]
    static extern void goal_add_passenger(EEUnitGoal* goal, EEUnit* u);
}
