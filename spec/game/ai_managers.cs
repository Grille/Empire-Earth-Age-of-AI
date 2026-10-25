/// the AI's build manager, CM +0x3C
unsafe struct ee_build_mgr
{
    /// the per-epoch build lists, 0x14 each - bm_request_game 0x46802D / 0x46A9C1
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

    /// thiscall(build manager, type): queued
    [address((ee)0x00465640, (aoc)0x004672FA)]
    extern char8 queued(int32 type);

    /// thiscall(build manager, x, y) -> bool
    [address((ee)0x004660B9, (aoc)0x00467E10)]
    extern char8 enemy_near(int32 x, int32 y);

    [address((ee)0x0046580A, (aoc)0x004674C4)]
    [call_site("place", (ee)0x00468629, (aoc)0x0046B044)]
    extern char8 place(int32 type, int32 x, int32 y);
}

/// the AI's civilization manager, player +0xB90 (EECPCivilizationManager)
[c_type("ee_civ_mgr")]
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

    /// SetNextEpochAdvanceTime
    [address((ee)0x0046DE8C, (aoc)0x00470B04)]
    extern void set_epoch_time(uint32 now, char8 flag);
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
[c_type("ee_resource_mgr")]
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

    [address((ee)0x0048A554, (aoc)0x0048EECF)]
    // call EECPResourceManager::AssignCitizens (0x48A554)
    [call_site("rm_assign_citizens", (ee)0x0048D36A, (aoc)0x00491E3B)]
    extern char8 assign_citizens();

    [address((ee)0x0048D0A2, (aoc)0x00491B78)]
    // call EECPResourceManager::ComputeGathererTargets (0x48D0A2)
    [call_site("rm_compute_targets", (ee)0x0048D347, (aoc)0x00491E18)]
    extern char8 compute_targets();

    [address((ee)0x0048D263, (aoc)0x00491D34)]
    // call EECPResourceManager::Update(now) (0x48D263)
    [call_site("rm_update", (ee)0x0046E3C6, (aoc)0x00471051)]
    extern void update(uint32 now);
}

/// the AI's research manager, CM +0x40 (EECPResearchManager)
[c_type("ee_research_mgr")]
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

    /// thiscall(research manager cm+0x40, count): the AI's request for an epoch's required techs
    [address((ee)0x004860D6, (aoc)0x0048A91E)]
    extern void request_requirements(int32 count);

    [address((ee)0x00485F57, (aoc)0x0048A79F)]
    extern void schedule(int32 id);
}

/// the AI's unit manager, CM +0x44
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

    [address((ee)0x0048EE84, (aoc)0x00493B97)]
    // a call of unit_mgr_train_citizens
    [call_site("train_citizens", (ee)0x00430740, (aoc)0x0042E23C)]
    extern nuint train_citizens(nuint a);

    [address((ee)0x0049014F, (aoc)0x00494F42)]
    // the calls of unit_mgr_train (econ.c: the game's unit orders)
    [call_site("train_units", (ee)0x0046CCE6, (aoc)0x0046F786)]
    [call_site("train_units", (ee)0x00490CAC, (aoc)0x00495AFF)]
    [call_site("train_units", (ee)0x00490E12, (aoc)0x00495C65)]
    [call_site("train_units", (ee)0x00490F1F, (aoc)0x00495D72)]
    [call_site("train_units", (ee)0x00490FA9, (aoc)0x00495DFC)]
    extern nuint train(int32* req, nuint b);

    /// citizens of both sexes and the epoch's fishing boat type
    [address((ee)0x0048F819, (aoc)0x00494572)]
    extern int32 citizens();

    /// citizen share x population cap
    [address((ee)0x0048EDDB, (aoc)0x00493AEE)]
    // a call of unit_mgr_citizens_wanted
    [call_site("citizens_wanted", (ee)0x00490BC2, (aoc)0x00495A15)]
    extern int32 citizens_wanted();

    /// thiscall(unit manager, type)
    [address((ee)0x0048E9D8, (aoc)0x00493696)]
    extern char8 pending(int32 type);

    /// thiscall(unit manager, type): its units and buildings of that type
    [address((ee)0x0048F8AF, (aoc)0x00494608)]
    extern uint8* type_list(int32 type);
}

/// ai_managers's data of the game, places in its code and numbers
static unsafe class ai_managers
{
    /// the call of farms_game
    [address((ee)0x0048AC54, (aoc)0x0048F643)]
    static code call_farms;

    [address((ee)0x0048B0FD, (aoc)0x0048FAD4)]
    static code call_unbuilt_farms;

}

/// the game's functions ai_managers.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class ai_managers_functions
{
    /// (unit, target, 0)
    [address((ee)0x0048C81C, (aoc)0x004912BE), thiscall]
    // the calls of assign_to_site (trace.c)
    [call_site("gather", (ee)0x0048A2A6, (aoc)0x0048EC21)]
    [call_site("gather", (ee)0x0048A372, (aoc)0x0048ECED)]
    [call_site("gather", (ee)0x0048A9B8, (aoc)0x0048F32C)]
    [call_site("gather", (ee)0x0048AB53, (aoc)0x0048F540)]
    [call_site("gather", (ee)0x0048ABF3, (aoc)0x0048F5E2)]
    [call_site("gather", (ee)0x0048ACB7, (aoc)0x0048F6A6)]
    [call_site("gather", (ee)0x0048ADC6, (aoc)0x0048F7B3)]
    [call_site("gather", (ee)0x0048AE7A, (aoc)0x0048F869)]
    [call_site("gather", (ee)0x0048AF60, (aoc)0x0048F94F)]
    [call_site("gather", (ee)0x0048B0A6, (aoc)0x0048FA80)]
    [call_site("gather", (ee)0x0048B261, (aoc)0x0048FC37)]
    static extern void assign_to_site(EECPResourceManager* rm, EEUnit* u, EEUnit* site, int32 c);

    /// thiscall(build manager, type, 1, 0): one building requested
    [address((ee)0x0046802D, (aoc)0x0046A9C1), thiscall]
    static extern void bm_request_game(void* p1, int32 p2, int32 p3, int32 p4);

    /// an entry into a build manager epoch queue
    [address((ee)0x00468C37, (aoc)0x0046B798), thiscall]
    static extern void bq_add(void* p1, int32* p2);

    /// thiscall(cm, unit state): in no group
    [address((ee)0x004707D8, (aoc)0x0047377E), thiscall]
    static extern char8 cm_unit_free(void* cm, void* unit_state);

    /// (count, out list)
    [address((ee)0x0048BFD0, (aoc)0x00490A63), thiscall]
    static extern void* farms_game(void* p1, nuint p2, nuint p3);

    /// (char needDropsite, char nearBase, unit) -> site
    [address((ee)0x0048BBA8, (aoc)0x00490602), thiscall]
    // the calls of find_food_game
    [call_site("find_food", (ee)0x0048AB8D, (aoc)0x0048F57C)]
    [call_site("find_food", (ee)0x0048AC2B, (aoc)0x0048F61A)]
    [call_site("find_food", (ee)0x0048AE14, (aoc)0x0048F803)]
    [call_site("find_food", (ee)0x0048AEB4, (aoc)0x0048F8A3)]
    [call_site("find_food", (ee)0x0048AEFA, (aoc)0x0048F8E9)]
    [call_site("find_food", (ee)0x0048AF99, (aoc)0x0048F988)]
    static extern void* find_food_game(EECPResourceManager* rm, nuint need_drop, nuint near_base, EEUnit* unit);

    /// (type, unit, reverse) -> site
    [address((ee)0x0048C204, (aoc)0x00490CB5), thiscall]
    // the calls of find_mine_game
    [call_site("find_mine", (ee)0x0048A247, (aoc)0x0048EBC2)]
    [call_site("find_mine", (ee)0x0048A25F, (aoc)0x0048EBDA)]
    [call_site("find_mine", (ee)0x0048A277, (aoc)0x0048EBF2)]
    [call_site("find_mine", (ee)0x0048A952, (aoc)0x0048F2C6)]
    [call_site("find_mine", (ee)0x0048A9F4, (aoc)0x0048F369)]
    [call_site("find_mine", (ee)0x0048AA2B, (aoc)0x0048F3A4)]
    static extern void* find_mine_game(EECPResourceManager* rm, int32 type, EEUnit* unit, nuint rev);

    /// (flag, unit) -> target
    [address((ee)0x0048C060, (aoc)0x00490AF3), thiscall]
    // the calls of find_wood_game
    [call_site("find_wood", (ee)0x0048A28E, (aoc)0x0048EC09)]
    [call_site("find_wood", (ee)0x0048AAF6, (aoc)0x0048F4E1)]
    static extern void* find_wood_game(EECPResourceManager* rm, nuint flag, EEUnit* unit);

    [address((ee)0x0046D209, (aoc)0x0046FDD3), thiscall]
    // the calls of fish_type
    [call_site("fish_type", (ee)0x0048F823, (aoc)0x0049457C)]
    [call_site("fish_type", (ee)0x0048FB16, (aoc)0x0049488C)]
    static extern int32 fish_type(EECPCivilizationManager* cm);

    /// (count, flag, out list)
    [address((ee)0x0048BA3B, (aoc)0x00490458), thiscall]
    // the calls of hunt_game
    [call_site("hunt", (ee)0x0048ACE4, (aoc)0x0048F6D2)]
    [call_site("hunt", (ee)0x0048AFC4, (aoc)0x0048F9B3)]
    static extern void* hunt_game(EECPResourceManager* rm, nuint n, nuint f, nuint @out);

    /// thiscall(unit manager, type)
    [address((ee)0x0048EA2E, (aoc)0x004936EC), thiscall]
    static extern char8 um_in_production(ee_unit_mgr* um, int32 type);

    [address((ee)0x0048BF4F, (aoc)0x004909D7), thiscall]
    static extern void* unbuilt_farms(void* p1, nuint p2, nuint p3);
}
