/// a unit's graphics (EEGraphicObject, its stats' gfx and its vf21's; RTTI in a running game, every unit's)
[c_type("ee_dmg_model")]
unsafe struct EEGraphicObject
{
    /// its damage points by state (a map; the end node +0x150)
    [offset(0x14C), found(s)]
    block map;

    /// the damage points of a state - the impact effects place theirs by it (EEE2DImpact::vf21 0x4DA7B2,
    /// EEE2DLightGlow::vf12 0x4DB2DF; game.call_damage_points hooked by the mod's guard)
    [address((ee)0x004D291F, (aoc)0x004D84EC)]
    extern int32 damage_points(int32 index, uint32 state);
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

    /// 0x553663 sets it with +0x54 (it in ms) and +0x58 (1 / it) and sets the timer; the step 0x546C63 passes it to
    /// 0x51EE75 every step; the game speed message 0x4B9378 resets it
    [offset(0x50), found(c), guess]
    float32 tick_s;

    /// tick_s in ms (0x553663): each step adds it to sync_ms
    [offset(0x54), found(c), guess]
    uint32 tick_ms;

    /// steps done
    [offset(0x80), found(c)]
    uint32 steps;

    /// a step's length (s)
    [offset(0x8C), found(c)]
    float32 step_s;

    /// game time in ms as a float: + step_ms each step
    [offset(0x94), found(c)]
    float32 time_ms;

    /// a step's length in ms (step_s x 1000)
    [offset(0x98), found(c)]
    float32 step_ms;

    /// TMGetTime at this step's start
    [offset(0xA8), found(c)]
    uint32 step_start;

    /// TMGetTime at the last step's start
    [offset(0xAC), found(c)]
    uint32 prev_step_start;

    /// each step's draw from the game's generator (+0x18, URandom::Generate) rounded; an argument of 0x40CFC7 (the
    /// map's code)
    [offset(0xB0), found(c), guess]
    int32 step_random;

    /// steps run (a second counter: zeroed by the reset 0x54650E, saved)
    [offset(0xB4), found(c)]
    uint32 step_ticks;

    /// steps it may still run
    [offset(0xB8), found(c)]
    uint32 steps_granted;

    /// ms between step starts, summed; past 999 the average goes to +0xDC and it restarts
    [offset(0xBC), found(c)]
    uint32 gap_ms_sum;

    /// how many gap_ms_sum holds
    [offset(0xC0), found(c)]
    uint32 gap_count;

    /// ms the steps' work took, summed; past 999 the average goes to +0xD8 and it restarts
    [offset(0xC4), found(c)]
    uint32 work_ms_sum;

    /// how many work_ms_sum holds
    [offset(0xC8), found(c)]
    uint32 work_count;

    /// ms since the last server sync: the server's step adds tick_ms and past [0x91943C] zeroes it and runs 0x547676
    /// and the server's vf8 twice; 0x520DE3 zeroes it
    [offset(0xCC), found(c), guess]
    uint32 sync_ms;

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
    /// 17 EEPlayer* by index (game.players is this memory): the RTSWorld constructor 0x53B637 zeroes them, set_stance
    /// 0x52D5D7 and player_out 0x52E2B7 index them
    [offset(0x74), found(c)]
    block players;

    /// the map (game.map is this memory): set_stance 0x52D5D7 passes it to set_diplomacy
    [offset(0xBC), found(c)]
    EEMap* map;

    /// the players' count (game.players_n is this memory): player 0x52E6EF and unit_by_id 0x53B889 (id >> 24 against
    /// it) bound by it
    [offset(0xCC), found(c)]
    uint32 players_n;

    /// the game's setup, an ee_setup in it (the constructor 0x52CCC9 builds one here and a second at +0x51C):
    /// game.end_epoch is its +0x3E4, game.scenario_flag its +0x409
    [offset(0x110), found(c), guess]
    block setup;

    /// a second ee_setup the constructor 0x52CCC9 builds (its use not read yet)
    [offset(0x51C), found(c), guess]
    block setup_2;

    /// a map site -> gatherers (allocator byte, head +4, size +8): site_gatherers 0x52E5F7 finds the site (0x404CA6)
    /// against its head +0x9A8 and returns the node's +0x10
    [offset(0x9A4), found(c), guess]
    block gatherer_counts;

    /// records of 0x18 bytes, each a vector of player indices at +4: the standard victory check 0x531913 ends the game
    /// when one is left
    [offset(0x9B4), found(c), guess]
    vector alliances;

    /// the local player's index (game.local_player is this memory): player_out 0x52E2B7 runs the local defeat 0x5150F2
    /// when it is the player going out
    [offset(0xB84), found(c)]
    int32 local_player;

    /// the standard victory (the game settings' +0x408 at the start, the editor's flag): while set, 0x531913 ends the
    /// game once one alliance is left and takes out every player without units (both builds' +0xB96)
    [offset(0xB96), found(c)]
    uint8 standard_victory;

    /// thiscall(game.game_state, index)
    [address((ee)0x0052E6EF, (aoc)0x00539670)]
    extern void* player(int32 index);

    /// thiscall(game.game_state, char won, index)
    [address((ee)0x0052E2B7, (aoc)0x005391F2)]
    extern void player_out(int32 won, int32 index);

    [address((ee)0x0052D5D7, (aoc)0x005384DD)]
    extern void set_stance(int32 from, int32 to, int32 stance);

    /// thiscall(game.game_state, id): a unit's handle by its id (the player in bits 24..31), its +0x30 the unit; NULL
    /// unknown
    [address((ee)0x0053B889, (aoc)0x0054804F)]
    extern EEUnit* unit_by_id(uint32 id);

    [address((ee)0x0052E5F7, (aoc)0x00539532)]
    extern uint32 site_gatherers(EEUnit* site);
}

/// the game being started (EEGameState, EE 0x917588 - its constructor 0x53B48D, RTTI by its vtable; not the world,
/// game.game_state): its own copy of the setup at +0x8, the players, the speed and the map seed for the start
[vtable((ee)0x0082CA14, (aoc)0x00842178)]
unsafe struct EEGameState
{
    /// starts the game the setup describes (the menu's start 0x4CEC14 calls it; the spec had it as EEWorld's until
    /// 2026-10-25)
    [address((ee)0x0052599B, (aoc)0x005306F2)]
    [call_site("game_start", (ee)0x004CEC14, (aoc)0x004D4443)]
    extern char8 start(ee_setup* setup, void* p2);
}

/// the game's tables of values, game.table_values (EEWorldTable)
[c_type("ee_world_table"), vtable((ee)0x0082AB14, (aoc)0x0083F810)]
unsafe struct EEWorldTable
{
    [address((ee)0x0048E817, (aoc)0x004CD4B3)]
    extern uint32 value(int32 key);
}
