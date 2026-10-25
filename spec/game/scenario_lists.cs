/// scenario_lists's data of the game, places in its code and numbers (raw/'s address rows)
static unsafe class scenario_lists
{
    /// SM_CLASS_TEST: a scenario object's class from here is a family (AoC: 15 epochs before)
    [value((ee)15, (aoc)16)]
    static uint32 sm_class_family;

    /// from here a unit group
    [value((ee)0x71, (aoc)0x75)]
    static uint32 sm_class_group;

    /// from here an RPS class
    [value((ee)72, (aoc)76)]
    static uint32 sm_class_rps;
}
