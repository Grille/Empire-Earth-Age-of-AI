/// a cluster of a group's units (group_sector_route's units)
[c_type("void")]
unsafe struct ee_unit_list
{
    /// group_sector_route's reads (both builds): a vector of ee_unit * (begin, end +4)
    [offset(0x4), found(c)]
    vector units;
}

/// a group of units moving together (EEGroup; EEActiveGroup ...)
[c_type("ee_group"), vtable((ee)0x008315AC, (aoc)0x00846F20)]
unsafe struct EEGroup
{
    /// thiscall(group, int from[2], int to[2], units): a sector route for a cluster of a group's units, each given
    /// its chain of auto-waypoint goals
    [address((ee)0x005F670A, (aoc)0x00606581)]
    // EEActiveGroup::vf16's call of group_sector_route, for each cluster of the group
    [call_site("group_sector_route", (ee)0x005DB5C8, (aoc)0x005EBF0C)]
    extern void sector_route(int32* from, int32* to, void* units);

    /// the group's speed, acceleration and turns from its members' (EEActiveGroup's)
    [slot(19)]
    extern void speeds();
}

/// a group move's group (EEActiveGroup): one per kind of a move order (EEMPathTo::vf10), a unit's +0x1C8 while it is in
/// one; the formation's slots are laid for it (formation_slots' group)
[@base(typeof(EEGroup)), c_type("ee_active_group"), vtable((ee)0x0083141C, (aoc)0x00846CF8)]
unsafe struct EEActiveGroup
{
    /// a map member unit id -> vec3 (head node +0x40, count +0x44, its allocator bytes +0x3C / +0x3D): 0x5DF33D stores
    /// where each member is sent, 0x5DF185 / 0x5DF23D test whether a point is taken, vf10 0x5DF0FC erases a leaving
    /// member
    [offset(0x40), found(c), guess]
    block member_targets;

    /// built by the constructor 0x5DB37C (allocator byte +0x48), cleared only by the regroup 0x5DD18B and the
    /// destructor 0x5DB409; no reader
    [offset(0x4C), found(c), guess]
    vector _0x4c;

    /// the same (allocator byte +0x58): built, cleared and freed only
    [offset(0x5C), found(c), guess]
    vector _0x5c;

    /// the group's goal, copied to each member: 0x5DD436 sets it, 0x4C00C2 deletes it, the dispatch 0x5DB641 / 0x5DD463
    /// calls its slot 10
    [offset(0x68), found(c)]
    EEUnitGoal* goal;

    /// the group's action as a unit state: 5000 none (the constructor, 0x5DD18B), 8000 move, 2001 attack, 11002
    /// explore; the dispatch 0x5DD463 switches on it
    [offset(0x6C), found(c), guess]
    int32 action;

    /// the members' top speed, tiles/s - the average, at most the slowest x the settings table's 0x31 (speeds); every
    /// member's top speed while together is set (0x46123F through 0x5DD355)
    [offset(0x70), found(c)]
    float32 speed;

    /// speeds 0x5DCE49: the fastest member's speed
    [offset(0x74), found(c)]
    float32 speed_max;

    /// the slowest member's top speed (speeds)
    [offset(0x78), found(c)]
    float32 speed_min;

    /// speeds: the members' average acceleration, at most accel_min x speed_cap; 0x5DD373 returns it for a member's
    /// acceleration while together is set
    [offset(0x7C), found(c)]
    float32 accel;

    /// speeds: the largest acceleration
    [offset(0x80), found(c)]
    float32 accel_max;

    /// speeds: the smallest acceleration
    [offset(0x84), found(c)]
    float32 accel_min;

    /// speeds: the average turn rate (stats +0xB4), capped; 0x5DD391 returns it for a member's max turn
    [offset(0x88), found(c)]
    float32 turn_rate;

    /// speeds: the largest turn rate
    [offset(0x8C), found(c)]
    float32 turn_rate_max;

    /// speeds: the smallest turn rate
    [offset(0x90), found(c)]
    float32 turn_rate_min;

    /// speeds: the average turn acceleration (stats +0xB0), capped; 0x5DD3B2 returns it for a member's
    [offset(0x94), found(c)]
    float32 turn_accel;

    /// speeds: the largest turn acceleration
    [offset(0x98), found(c)]
    float32 turn_accel_max;

    /// speeds: the smallest turn acceleration
    [offset(0x9C), found(c)]
    float32 turn_accel_min;

    /// speeds: 1 / speed (0 unless above 0); 0x5DD3D3 returns it for stats +0xD0
    [offset(0xA0), found(c)]
    float32 speed_inv;

    /// speeds: 1 / accel; 0x5DD3F4 returns it for stats +0xC4
    [offset(0xA4), found(c)]
    float32 accel_inv;

    /// speeds: 1 / turn_rate
    [offset(0xA8), found(c)]
    float32 turn_rate_inv;

    /// speeds: 1 / turn_accel; 0x5DD415 returns it for stats +0xC8
    [offset(0xAC), found(c)]
    float32 turn_accel_inv;

    /// speeds: the settings table's 0x31 - each average at most it x the matching least
    [offset(0xB0), found(c)]
    float32 speed_cap;

    /// 0x5DD2AA deletes the goal, puts the group into its player's +0x854 list of groups to drop (0x5596xx releases
    /// them), adds one to +4 and sets this; the constructor clears it, no reader
    [offset(0xB4), found(c), guess]
    uint8 retired;

    /// 0x5DD436's second argument, the order's queued (0x4B0000.c): with it the dispatch (0x5DB641, 0x5DD463) gives a
    /// new goal only to members without a goal
    [offset(0xB5), found(c)]
    uint8 queued;

    /// set: the members move with the group's speed, acceleration and turns (+0x70 / +0x7C / +0x88 / +0x94) instead
    /// of their own - the group's hand-out 0x5DD436 sets it from the move goal's "together" (EEMPathTo +0x1F)
    [offset(0xB6), found(c)]
    uint8 together;

    /// vf19: the group's speed, acceleration and turns from its members' (+0x70 .. +0xAC: average, fastest, slowest
    /// of each) - again whenever members come or go
    [address((ee)0x005DCE49, (aoc)0x005ED692), overrides(typeof(EEGroup))]
    [vtable_site("group_speeds")]
    extern void speeds();
}

/// the path manager, a static object at 0x917D00 (AoC not looked up) (EEPathManager)
[c_type("ee_path_mgr"), vtable((ee)0x00827C44, (aoc)0x0083C438)]
unsafe struct EEPathManager
{
    /// the path manager's route request: thiscall(mgr, unit, from tile, to tile, sync, node limit, use sectors, ctx),
    /// ret 0x1C (AoC: an 8th argument, ret 0x20 - its continent routing; port_addrs 69%) - PrepareToMove's
    /// (0x6224D0), a sector route's first leg, the re-path of all
    [address((ee)0x004A1AFF, (aoc)0x004A6E88)]
    extern void request(EEUnit* u, int32* from, int32* to, int32 sync, int32 limit, int32 sectors, void* ctx);
}

/// path's data of the game, places in its code and numbers
static unsafe class path
{
}

/// the game's functions path.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class path_functions
{
}
