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
    uint32 step;

    /// a step's length (s)
    [offset(0x8C), found(c)]
    float32 step_s;

    /// steps it may still run
    [offset(0xB8), found(c)]
    uint32 steps_granted;

    /// game time (ms)
    [offset(0xD0), found(c)]
    uint32 time;
}

/// the world, [game.world] (EEWorld)
unsafe struct EEWorld
{
    /// its thread (TSThread::Post)
    /// EEWorld's +0x1C and +0x5C the same in AoC
    [offset(0x40), found(b)]
    void* thread;

    /// the map's seed
    [offset(0x54), found(b)]
    uint32 seed;
}
