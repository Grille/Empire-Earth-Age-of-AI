/// the object of damage_points
[c_type("void")]
unsafe struct ee_damage_model
{
    [offset(0x14C), found(s)]
    block map;
}

/// the game data, [game.game_data]
unsafe struct ee_game_data
{
    /// the code reading [game.game_data]
    [offset(0x2CC), found(c)]
    int32 line_ms;

    /// the code reading [game.game_data] (the setup copy 0x524B8F)
    [offset(0x2E8), found(c)]
    int32 unit_limit;

    /// the code reading [game.game_data] (the setup copy 0x524B8F)
    [offset(0x2EC), found(c)]
    int32 unit_limit_max;
}

/// the simulation, [api.world_clock] (EEPhysics)
unsafe struct EEPhysics
{
    /// the game generator
    [offset(0x18), found(c)]
    uint32 seed;

    /// steps done
    [offset(0x80), found(c)]
    uint32 steps;

    /// a step's length (s)
    [offset(0x8C), found(c)]
    float32 step_s;

    /// steps it may still run
    [offset(0xB8), found(c)]
    uint32 steps_granted;

    /// game time (ms)
    [offset(0xD0), found(c)]
    uint32 time;

    [address((ee)0x00546C63, (aoc)0x00553082)]
    extern void step();
}

/// the world, game.world - a static object: its players at +0x74 (game.players), its map at +0xBC (EEWorld)
[c_type("ee_world"), vtable((ee)0x0082C84C, (aoc)0x00841FEC)]
unsafe struct EEWorld
{
    /// thiscall(game.game_state, index)
    [address((ee)0x0052E6EF, (aoc)0x00539670)]
    extern void* player(int32 index);

    /// thiscall(game.game_state, char won, index)
    [address((ee)0x0052E2B7, (aoc)0x005391F2)]
    extern void player_out(int32 won, int32 index);

    [address((ee)0x0052599B, (aoc)0x005306F2)]
    [call_site("game_start", (ee)0x004CEC14, (aoc)0x004D4443)]
    extern char8 start(ee_setup* setup, void* p2);

    [address((ee)0x0052D5D7, (aoc)0x005384DD)]
    extern void set_stance(int32 from, int32 to, int32 stance);

    /// thiscall(game.game_state, id): a unit's handle by its id (the player in bits 24..31), its +0x30 the unit; NULL
    /// unknown
    [address((ee)0x0053B889, (aoc)0x0054804F)]
    extern EEUnit* unit_by_id(uint32 id);

    [address((ee)0x0052E5F7, (aoc)0x00539532)]
    extern uint32 site_gatherers(EEUnit* site);
}

/// the game's tables of values, game.table_values (EEWorldTable)
[c_type("ee_world_table"), vtable((ee)0x0082AB14, (aoc)0x0083F810)]
unsafe struct EEWorldTable
{
    [address((ee)0x0048E817, (aoc)0x004CD4B3)]
    extern uint32 value(int32 key);
}
