/// a cluster of a group's units (group_sector_route's units)
[c_type("void")]
unsafe struct ee_unit_list
{
    /// group_sector_route's reads (both builds): a vector of ee_unit * (begin, end +4)
    [offset(0x4), found(c)]
    block units;
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
