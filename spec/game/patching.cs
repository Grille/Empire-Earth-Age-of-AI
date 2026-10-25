/// patching's data of the game, places in its code and numbers
static unsafe class patching
{
    /// an unload call (the resource manager's ferry)
    [address((ee)0x0048CC5D, (aoc)0x00491733)]
    static code call_unload_rm;

    [address((ee)0x0044AF88, (aoc)0x0044A6A3)]
    static code ret_asg_land_handoff_give;

    /// the calls of goal_unload_init (landing.c; the last, 0x615AE6, the unload goal's copy)
    [address((ee)0x00445AAD, (aoc)0x00444CCD)]
    [address((ee)0x00483E4D, (aoc)0x004884B9)]
    [address((ee)0x0048CC5D, (aoc)0x00491733)]
    [address((ee)0x0048D022, (aoc)0x00491AF8)]
    [address((ee)0x004BF82A, (aoc)0x004C86E9)]
    [address((ee)0x005B3E23, (aoc)0x005C4E31)]
    [address((ee)0x005DC81B, (aoc)0x005ED156)]
    [address((ee)0x00615AE6, (aoc)0x006284E2)]
    static code unload_sites;
}
