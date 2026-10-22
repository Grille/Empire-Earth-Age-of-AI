// THE ADDRESSES THE MOD PATCHES TO HOOK THE GAME (hooks.h's installers): the calls and vtable slots of the hooked
// functions, their vtables and lists of call sites - their rows of addr.h's tables, in the same columns (one value per
// exe build, k_exe_builds' order). A hooked function the mod also calls is in functions_addr.h.

#define ADDRS_HOOKS(F, D) \
    D(CALL_CP_UPDATE, 0x0042F3CD, 0x0042CDD1) \
    D(CALL_POP_CAP_1, 0x004311E5, 0x0042ECF6)                                 /* the callers of FN_POP_CAP */ \
    D(CALL_EPOCH_UPGRADES, 0x0043D365, 0x0043C9FC)                            /* the call of FN_EPOCH_UPGRADES */ \
    D(CALL_POP_CAP_2, 0x0043FA9B, 0x0043F2F6) \
    D(CALL_GROUP_STAT, 0x00450546, 0x0045122B) \
    D(CALL_PLACE, 0x00468629, 0x0046B044) \
    D(CALL_REPAIR_GOAL, 0x0046DA88, 0x004706DF) \
    D(CALL_RM_UPDATE, 0x0046E3C6, 0x00471051)                                 /* call EECPResourceManager::Update(now)              (0x48D263) */ \
    D(CALL_POP_CAP_3, 0x00474AF0, 0x004781C9) \
    D(CALL_NV_MAINT_DELETE, 0x00477B17, 0x0047B635) \
    D(CALL_UNIT_TO_GROUP, 0x004824BF, 0x004869F3) \
    D(CALL_SHORE_TO_UNLOAD, 0x00483E24, 0x00488490)                           /* the idle transports' call of FN_SHORE_TO_UNLOAD */ \
    D(CALL_FARMS, 0x0048AC54, 0x0048F643)                                     /* the call of FN_FARMS */ \
    D(CALL_HUNT_1, 0x0048ACE4, 0x0048F6D2)                                    /* the calls of FN_HUNT */ \
    D(CALL_HUNT_2, 0x0048AFC4, 0x0048F9B3) \
    D(CALL_UNBUILT_FARMS, 0x0048B0FD, 0x0048FAD4) \
    D(CALL_UNLOAD_RM, 0x0048CC5D, 0x00491733)                                 /* an unload call (the resource manager's ferry) */ \
    D(CALL_UNLOAD_RETRY_GIVE, 0x0048D03C, 0x00491B12) \
    D(CALL_COMPUTE_TARGETS, 0x0048D347, 0x00491E18)                           /* call EECPResourceManager::ComputeGathererTargets (0x48D0A2) */ \
    D(CALL_ASSIGN_CITIZENS, 0x0048D36A, 0x00491E3B)                           /* call EECPResourceManager::AssignCitizens        (0x48A554) */ \
    D(CALL_FISH_TYPE_1, 0x0048F823, 0x0049457C)                               /* the calls of FN_FISH_TYPE */ \
    D(CALL_FISH_TYPE_2, 0x0048FB16, 0x0049488C) \
    D(CALL_CITIZENS_WANTED, 0x00490BC2, 0x00495A15)                           /* a call of FN_UM_CITIZENS_WANTED */ \
    D(CALL_GAME_START, 0x004CEC14, 0x004D4443) \
    D(CALL_DAMAGE_POINTS, 0x004F80A4, 0x0050232D) \
    D(CALL_LOAD_SAVED, 0x00522BF8, 0x0052D8F9)                                /* call LoadSavedGameFromCmdline(UString *) 0x521F94 */ \
    D(CALL_PARSE_CMDLINE, 0x00535EF2, 0x005410C8)                             /* call ParseCommandLine(const char *) 0x522A36 */ \
    D(CALL_EPOCH_CATCH_UP, 0x0055A5B5, 0x00569633)                            /* its one call: EEPlayer::vf41, a human reached an epoch */ \
    D(EXIT_CONFIRM_AT, 0x005D0B2C, 0x005E1438) \
    D(CALL_GOAL_FBA_CHECK, 0x005ED9BF, 0x005FBA9C)                            /* in 0x5ED24B: FN_GOAL_FBA_CHECK on a dynamic_cast, unchecked */ \
    D(AT_GOAL_SET_READ, 0x005F0634, 0x005FE8DA) \
    D(CALL_GOALSET_ASK, 0x005F0FFA, 0x005FF364) \
    D(CALL_GOALSET_AGAIN, 0x005F1029, 0x005FF393) \
    D(CALL_KILL_STAT, 0x00617780, 0x0062A2DF) \
    D(CALL_NODE_DEL_2, 0x00628B7F, 0x0063D930) \
    D(CALL_NODE_DEL_1, 0x00628BE7, 0x0063D998) \
    D(CALL_RANGE_SORT, 0x0063052E, 0x0064631B) \
    D(FN_OP_DELETE, 0x006836F0, 0x0069D150) \
    D(VT_ROUTE_PLANNER, 0x00825CC4, 0x00839C70)                               /* EEAIGPGModifierLandAttackSubGroupRoutePlanner */ \
    D(VT_COMPUTER_PLAYER, 0x008264C0, 0x0083A57C) \
    D(VT_ASG_AIR, 0x00826610, 0x0083A6D8) \
    D(VT_ASG_LAND, 0x00826CA8, 0x0083AD7C)                                    /* EECPASGLand */ \
    D(VT_ASG_SEA, 0x00826F10, 0x0083AFEC) \
    D(VT_ASG_SEA_TRANSPORT, 0x008270F8, 0x0083B1D8) \
    D(VT_RESIGN, 0x00828820, 0x0083D4E8) \
    D(VT_PAY_TRIBUTE, 0x008289C0, 0x0083D688) \
    D(VT_INGAME_CHAT, 0x0082936C, 0x0083E0BC) \
    D(VTS_TECH_RESEARCHED, 0x00830874, 0x00846168)                            /* EETechTreeNode::vf6's vtable slot */ \
    D(VT_COMPLEX_UNIT, 0x00831494, 0x00846DDC) \


#define ADDR_NUMBERS_HOOKS(D) \
    D(SLOT_ROUTE_COST, 1, 1)            /* VT_ROUTE_PLANNER: FN_ROUTE_COST */ \
    D(SLOT_ASGU_SCORE, 2, 2)            /* ASGU_VTABLES */ \
    D(SLOT_MSG_EXECUTE, 10, 10)         /* VT_PAY_TRIBUTE, VT_INGAME_CHAT, VT_RESIGN: the message's Execute */ \
    D(SLOT_UNIT_RELEASE, 13, 13)        /* VT_COMPLEX_UNIT */ \
    D(SLOT_ASG_STATS, 20, 20)           /* the attack subgroups: FN_ASG_STATS, VT_ASG_LAND's FN_ASG_LAND_STATS */ \
    D(SLOT_CP_ADD_RESOURCE, 18, 18)     /* VT_COMPUTER_PLAYER: FN_CP_ADD_RESOURCE (econ.c's hook; AoC as before) */ \
    D(SLOT_CP_PAY, 45, 45)              /* VT_COMPUTER_PLAYER: FN_CP_PAY (econ.c's hook; AoC as before) */ \
    D(SLOT_ASG_GIVE_GOAL, 57, 57)       /* the attack subgroups */ \
    D(SLOT_ASG_UPDATE, 90, 90)          /* the attack subgroups: update(now) */ \
    D(SLOT_ASG_ROUTE, 94, 94)           /* VT_ASG_LAND */ \
    D(SLOT_ASG_ROUTE_DONE, 97, 97)      /* VT_ASG_LAND */ \


#define ADDR_LISTS_HOOKS(L) \
    /* the calls of FN_TRAIN_UNITS (econ.c: the game's unit orders) */ \
    L(TRAIN_SITES, 5, (0x0046CCE6, 0x00490CAC, 0x00490E12, 0x00490F1F, 0x00490FA9), \
        (0x0046F786, 0x00495AFF, 0x00495C65, 0x00495D72, 0x00495DFC)) \
    /* the calls of FN_FIND_MINE */ \
    L(MINE_SITES, 6, (0x0048A247, 0x0048A25F, 0x0048A277, 0x0048A952, 0x0048A9F4, 0x0048AA2B), \
        (0x0048EBC2, 0x0048EBDA, 0x0048EBF2, 0x0048F2C6, 0x0048F369, 0x0048F3A4)) \
    /* the calls of FN_FIND_FOOD */ \
    L(FOOD_SITES, 6, (0x0048AB8D, 0x0048AC2B, 0x0048AE14, 0x0048AEB4, 0x0048AEFA, 0x0048AF99), \
        (0x0048F57C, 0x0048F61A, 0x0048F803, 0x0048F8A3, 0x0048F8E9, 0x0048F988)) \
    /* the calls of FN_FIND_WOOD */ \
    L(WOOD_SITES, 2, (0x0048A28E, 0x0048AAF6), \
        (0x0048EC09, 0x0048F4E1)) \
    /* the calls of FN_GOAL_UNLOAD_CTOR (landing.c; the last, 0x615AE6, the unload goal's copy) */ \
    L(UNLOAD_SITES, 8, (0x00445AAD, 0x00483E4D, 0x0048CC5D, 0x0048D022, 0x004BF82A, 0x005B3E23, 0x005DC81B, 0x00615AE6), \
        (0x00444CCD, 0x004884B9, 0x00491733, 0x00491AF8, 0x004C86E9, 0x005C4E31, 0x005ED156, 0x006284E2)) \
    /* the land unit records' vtables of the attack subgroups: base, hero, priests and prophets */ \
    L(ASGU_VTABLES, 6, (0x00827844, 0x008253C0, 0x00825708, 0x008257F0, 0x00825458, 0x00825540), \
        (0x0083BF4C, 0x0083936C, 0x008396BC, 0x008397A8, 0x00839404, 0x008394F0)) \
    /* the attack subgroups' vtables */ \
    L(ASG_VTABLES, 6, (0x00827620, 0x00826610, 0x00826818, 0x00826F10, 0x008270F8, 0x00827318), \
        (0x0083BD20, 0x0083A6D8, 0x0083A8E0, 0x0083AFEC, 0x0083B1D8, 0x0083B9F0)) \

