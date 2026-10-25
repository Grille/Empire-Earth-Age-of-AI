/// the game's global state, its imports and the places in its code the mod changes in place
static unsafe class game
{
    // DATA

    /// the players by index, player_count of them
    [address((ee)0x00918374, (aoc)0x00930DB4), was("G_PLAYERS")]
    static EEPlayer** players;
    /// the number of players
    [address((ee)0x009183CC, (aoc)0x00930E0C), was("G_PLAYERS_N")]
    static uint32 player_count;
    /// the game speed; EEMGameSpeed::Apply (0x4B6BD4) sets it
    [address((ee)0x009183C0, (aoc)0x00930E00), was("G_GAME_SPEED")]
    static float32 speed;
    /// the float 1/800 (the minimap's x scale operand)
    [address((ee)0x008358A8, (aoc)0x0084B940), was("G_FLOAT_1_800")]
    static float32 float_1_800;

    // FUNCTIONS

    /// operator delete; the unit list's node deletions call it with the unit in ESI
    [address((ee)0x006836F0, (aoc)0x0069D150), cdecl, was("FN_OP_DELETE")]
    [call_site("unit_node_free", (ee)0x00628BE7, (aoc)0x0063D998)]
    [call_site("unit_node_free", (ee)0x00628B7F, (aoc)0x0063D930)]
    static extern void op_delete(void* p);
    /// UString::UString()
    [import((ee)0x008231E4, (aoc)0x008371E0), was("IAT_USTRING_CTOR")]
    static extern void ustring_init(void* s);

    // CODE

    /// the scroll action's timer period in ms: the operand of a push 0x19
    [address((ee)0x004050C2, (aoc)0x00404734), expect(-1, 0x6A, 0x19), was("SITE_SCROLL_TIMER")]
    static uint8 scroll_timer;
    /// the minimap's operands of float_1_800
    [address((ee)0x0065E01D, (aoc)0x00675618)]
    [address((ee)0x0065E8A3, (aoc)0x00675E95)]
    [address((ee)0x0065EB3E, (aoc)0x00676130)]
    [expect(nameof(float_1_800)), was("MINIMAP_FX_OPS")]
    static float32* minimap_fx_ops;
}

/// Miles Sound System Mixer.dll
[dll("Miles Sound System Mixer.dll")]
static unsafe class mixer
{
    /// its channels, 8 bytes each
    [address(0x52AC), was("RVA_MIX_CHANNELS")]
    static vector channels;
    /// the most channels it gives
    [address(0x5320), was("RVA_MIX_MAX_CHANNELS")]
    static int32 max_channels;
}
