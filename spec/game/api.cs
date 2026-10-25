/// api's data of the game, places in its code and numbers (raw/'s address rows)
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
    /// thiscall(site+0x14 object, player) -> bool
    [address((ee)0x00404A8A, (aoc)0x004042E2), thiscall]
    static extern char8 explored_game(void* p1, void* p2);
}
