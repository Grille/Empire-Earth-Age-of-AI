/// a cluster of a group's units (group_sector_route's units)
[c_type("void")]
unsafe struct ee_unit_list
{
    /// group_sector_route's reads (both builds): a vector of ee_unit * (begin, end +4)
    [offset(0x4), found(c)]
    block units;
}

/// path's data of the game, places in its code and numbers
static unsafe class path
{
    /// EEActiveGroup::vf16's call of group_sector_route, for each cluster of the group
    [address((ee)0x005DB5C8, (aoc)0x005EBF0C)]
    static code call_group_sector_route;
}

/// the game's functions path.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class path_functions
{
    /// thiscall(group, int from[2], int to[2], units): a sector route for a cluster of a group's units, each given
    /// its chain of auto-waypoint goals
    [address((ee)0x005F670A, (aoc)0x00606581), thiscall]
    static extern void group_sector_route(void* group, int32* from, int32* to, void* units);

    /// the path manager's route request: thiscall(mgr, unit, from tile, to tile, sync, node limit, use sectors, ctx),
    /// ret 0x1C (AoC: an 8th argument, ret 0x20 - its continent routing; port_addrs 69%) - PrepareToMove's
    /// (0x6224D0), a sector route's first leg, the re-path of all
    [address((ee)0x004A1AFF, (aoc)0x004A6E88), thiscall]
    static extern void path_request_game(void* p1, void* p2, int32* p3, int32* p4, int32 p5, int32 p6, int32 p7, void* p8);
}
