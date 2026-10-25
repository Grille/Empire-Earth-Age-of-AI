/// api's data of the game, places in its code and numbers
static unsafe class api
{
    /// ptr to EEPhysics; +0xD0 = game time in ms
    [address((ee)0x00919774, (aoc)0x009318F0)]
    static block world_clock;
}

/// the game's functions api.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class api_functions
{
}
