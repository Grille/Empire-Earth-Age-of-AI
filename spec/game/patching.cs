/// patching's data of the game, places in its code and numbers
static unsafe class patching
{
    /// an unload call (the resource manager's ferry)
    [address((ee)0x0048CC5D, (aoc)0x00491733)]
    static code call_unload_rm;

    [address((ee)0x0044AF88, (aoc)0x0044A6A3)]
    static code ret_asg_land_handoff_give;

}
