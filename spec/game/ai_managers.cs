/// the AI's build manager, CM +0x3C
[c_prefix("bm")]
unsafe struct ee_build_mgr
{
    /// the per-epoch build lists, 0x14 each - FN_BM_REQUEST 0x46802D / 0x46A9C1
    [offset(0x4), found(c)]
    block queues;

    /// its base (-10000: none yet)
    /// its file's code (+0x24, 5 votes)
    [offset((ee)0x12C, (aoc)0x150), found(c)]
    int32 base_x;

    /// its file's code (+0x24)
    [offset((ee)0x130, (aoc)0x154), found(c)]
    int32 base_y;

    /// its file's code (+0x24, 18 votes)
    [offset((ee)0x134, (aoc)0x158), found(c)]
    EECPCivilizationManager* owner;

    /// its file's code (+0x24, 33 votes)
    [offset((ee)0x138, (aoc)0x15C), found(c)]
    EEPlayer* player;

    /// its file's code (+0x24)
    [offset((ee)0x148, (aoc)0x16C), found(c)]
    block initialized;
}

/// the AI's civilization manager, player +0xB90 (EECPCivilizationManager)
[c_type("ee_civ_mgr"), c_prefix("cm")]
unsafe struct EECPCivilizationManager
{
    /// the build plan's list
    [offset(0x8), found(v)]
    ee_plan_node* plan;

    [offset(0x38), found(c)]
    EECPResourceManager* resource_mgr;

    [offset(0x3C), found(b)]
    ee_build_mgr* build_mgr;

    [offset(0x40), found(b)]
    EECPResearchManager* research_mgr;

    [offset(0x44), found(c)]
    ee_unit_mgr* unit_mgr;

    /// EECPWallManager
    [offset(0x48), found(c)]
    void* wall_mgr;

    /// its player
    /// between +0x48 and +0x50, unchanged (0x46D209 reads it: the player)
    [offset(0x4C), found(b)]
    EEPlayer* player;

    /// the construction project (NULL: a new one may start)
    [offset(0x50), found(c)]
    void* project;

    [offset(0x58), found(v)]
    fixed uint32 timers[6];

    /// its file's code (+4 from 0x74)
    [offset((ee)0x74, (aoc)0x78), found(c)]
    block flag;

    /// its file's code (+4 from 0x74)
    [offset((ee)0x75, (aoc)0x79), found(c)]
    block setup_done;

    /// its file's code (+4 from 0x74)
    [offset((ee)0x76, (aoc)0x7A), found(c)]
    uint8 request_out;

    /// its file's code (+4 from 0x74)
    [offset((ee)0x78, (aoc)0x7C), found(c)]
    uint8 unknown_78;

    /// its file's code (+4 from 0x74)
    [offset((ee)0x7A, (aoc)0x7E), found(c)]
    uint8 att_epoch_techs;

    /// its file's code (+4 from 0x74)
    [offset((ee)0x7B, (aoc)0x7F), found(c)]
    uint8 att_build;

    /// its file's code (+4 from 0x74)
    [offset((ee)0x7C, (aoc)0x80), found(c)]
    uint8 att_7c;
}

/// the civilization manager's list nodes
unsafe struct ee_plan_node
{
    [offset(0x0), found(s)]
    ee_plan_node* next;

    [offset(0xC), found(s)]
    int32 unknown_c;

    [offset(0x10), found(s)]
    int32 unknown_10;
}

/// the AI's resource manager (EECPResourceManager)
[c_type("ee_resource_mgr"), c_prefix("rm")]
unsafe struct EECPResourceManager
{
    /// the animals to hunt
    /// its file's code (the resource manager)
    [offset(0x88), found(c)]
    void** hunt_begin;

    /// the fish
    /// its file's code (the resource manager)
    [offset(0x98), found(c)]
    void** fish_begin;

    /// its file's code (the resource manager)
    [offset(0xA4), found(c)]
    block lists;

    /// its file's code (the resource manager)
    [offset(0x130), found(c)]
    EECPCivilizationManager* owner;

    /// its file's code (the resource manager)
    /// int[7] still needed
    [offset(0x134), found(c)]
    fixed int32 demand[7];

    /// its file's code (the resource manager)
    /// uint[7] gatherers wanted
    [offset(0x150), found(c)]
    fixed uint32 target[7];

    /// its file's code (the resource manager)
    [offset(0x16C), found(c)]
    block timers;

    /// (the timers reset while 0)
    /// its file's code (the resource manager)
    [offset(0x187), found(c)]
    uint8 initialized;

    /// the attention's activity 1
    /// its file's code (the resource manager)
    [offset(0x188), found(c)]
    uint8 att_188;
}

/// the AI's research manager, CM +0x40 (EECPResearchManager)
[c_type("ee_research_mgr"), c_prefix("rs")]
unsafe struct EECPResearchManager
{
    /// the techs it may research (ids)
    /// kept: the object's start, before its first known shift (0xF4 -> 0x104)
    [offset(0x8), found(h, "kept: the object's start, before its first known shift (0xF4 -> 0x104)")]
    int32* pool_begin;

    /// kept: the object's start, before its first known shift (0xF4 -> 0x104)
    [offset(0x18), found(h, "kept: the object's start, before its first known shift (0xF4 -> 0x104)")]
    block epoch;

    /// what it researches now
    [offset((ee)0x114, (aoc)0x124), found(v)]
    int32 current;

    [offset((ee)0x118, (aoc)0x128), found(v)]
    block initialized;

    /// the attention's activity 2 (random research)
    [offset((ee)0x11A, (aoc)0x12A), found(v)]
    uint8 att_random;
}

/// the AI's unit manager, CM +0x44
[c_prefix("um")]
unsafe struct ee_unit_mgr
{
    /// its file's code (the unit manager)
    [offset(0xC), found(c)]
    block requests;

    /// its file's code (+0x14, 31 votes)
    [offset((ee)0x134, (aoc)0x148), found(c)]
    EEPlayer* player;

    /// units in production (0x49014F adds)
    /// its file's code (+0x14)
    [offset((ee)0x13C, (aoc)0x150), found(c)]
    int32 in_production;

    /// the attention's activity 3
    /// its file's code (+0x14)
    [offset((ee)0x140, (aoc)0x154), found(c)]
    uint8 att_140;
}
