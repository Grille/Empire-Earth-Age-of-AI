// THE ADDRESSES THE MOD PATCHES TO HOOK THE GAME (hooks.c's installers): the calls and vtable slots of the hooked
// functions, their vtables and lists of call sites - their rows of tables.c's tables, in the same columns (one value per
// exe build, k_exe_builds' order). A hooked function the mod also calls is in functions.h.

#define ADDRS_HOOKS(F, D) \
    D(CALL_CLICK_MARK, 0x00404A26, 0x0040416B)                                /* EEAMouseClick::vf3's FN_GFX_ADD: the command's circle */ \
    D(SITE_SCROLL_TIMER, 0x004050C2, 0x00404734)                              /* operand of push 0x19 (timer period in ms) */ \
    D(CALL_CP_UPDATE, 0x0042F3CD, 0x0042CDD1) \
    D(CALL_TRAIN_CITIZENS, 0x00430740, 0x0042E23C)                            /* a call of FN_TRAIN_CITIZENS */ \
    D(CALL_POP_CAP_1, 0x004311E5, 0x0042ECF6)                                 /* the callers of FN_POP_CAP */ \
    D(CALL_EPOCH_UPGRADES, 0x0043D365, 0x0043C9FC)                            /* the call of FN_EPOCH_UPGRADES */ \
    D(CALL_POP_CAP_2, 0x0043FA9B, 0x0043F2F6) \
    D(FN_ADD_MOVIE, 0x00441432, 0x00440E57)                                   /* __thiscall, ret 4 */ \
    D(CALL_ASG_AIR_QUEUE_POP, 0x00442718, 0x00441E02)                        /* EECPASGAir vf85: the waiting units' pop (asg_queue.c) */ \
    D(RET_ASG_LAND_HANDOFF_GIVE, 0x0044AF88, 0x0044A6A3) \
    D(CALL_GROUP_STAT, 0x00450546, 0x0045122B) \
    D(CALL_ASG_QUEUE_POP, 0x0045BDA2, 0x0045D974)                            /* the other subgroups' vf85 (0x45BD56): the waiting units' pop */ \
    D(CALL_PLACE, 0x00468629, 0x0046B044) \
    D(CALL_REPAIR_GOAL, 0x0046DA88, 0x004706DF) \
    D(CALL_RM_UPDATE, 0x0046E3C6, 0x00471051)                                 /* call EECPResourceManager::Update(now)              (0x48D263) */ \
    D(CALL_POP_CAP_3, 0x00474AF0, 0x004781C9) \
    D(CALL_NV_MAINT_DELETE, 0x00477B17, 0x0047B635) \
    D(FN_ASG_QUEUE_POP, 0x00482028, 0x0048655C)                              /* thiscall(queue): releases the front unit, removes it */ \
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
    F(FN_PATH_REQUEST, void, __thiscall, (void *, void *, int *, int *, int, int, int, void *), 0x004A1AFF, 0x004A6E88) /* the path manager's route request: thiscall(mgr, unit, from tile, to tile, sync, node limit, use sectors, ctx), ret 0x1C (AoC: an 8th argument, ret 0x20 - its continent routing; port_addrs 69%) - PrepareToMove's (0x6224D0), a sector route's first leg, the re-path of all */ \
    D(CALL_SEL_PACK, 0x004BD133, 0x004C5E40) \
    F(START_EXEC, void, __thiscall, (ee_msg *), 0x004BEE57, 0x004C7CDC) \
    D(CALL_GAME_START, 0x004CEC14, 0x004D4443) \
    D(CALL_DAMAGE_POINTS, 0x004F80A4, 0x0050232D) \
    F(FN_DRAW_REC_CTOR, void, __thiscall, (void *), 0x00506442, 0x005118E9)   /* a draw list record */ \
    D(FN_ATTACK_ALERT, 0x00519BC1, 0x00524497) \
    D(CALL_LOAD_SAVED, 0x00522BF8, 0x0052D8F9)                                /* call LoadSavedGameFromCmdline(UString *) 0x521F94 */ \
    D(CALL_PLAYER_COLOR, 0x005262E4, 0x00531057) \
    D(CODE_SPLASH_START_JNE, 0x00534400, 0x0053F2FE)                          /* the loading splash's start (jne) */ \
    D(CODE_SPLASH_STOP_JNE, 0x0053443D, 0x0053F33B)                           /* the loading splash's stop (jne) */ \
    D(CALL_PARSE_CMDLINE, 0x00535EF2, 0x005410C8)                             /* call ParseCommandLine(const char *) 0x522A36 */ \
    F(FN_SPEED_STEP, float, __thiscall, (void *, void *, float, float), 0x0054858E, 0x00554BDE) /* EEPhysicsModel::vf8 thiscall(model, unit, distance to the waypoint, angle off the bow): the new speed from FN_SPEED_DECIDE (given the same) - on the last waypoint faster no further than the distance */ \
    F(FN_SPEED_DECIDE, int, __thiscall, (void *, void *, float, float), 0x00548645, 0x00554CB2) /* EEPhysicsModel::vf9 thiscall(model, unit, distance to the waypoint, angle off the bow): 2 hold, 3 faster, 4 slower - more than 45 degrees off, slower */ \
    F(FN_TURN_TOWARDS, void, __thiscall, (void *, void *, float *, float *, float *, float *, float *, char), 0x005484D9, 0x00554A25) /* EEPhysicsModel::vf7 thiscall(model, unit, float pos[3], float target[3], out angle left, out turn rate, out new heading, 0 / 1 to the unit's facing): the tick's turn towards a point (EEPMNaval::vf1 0x54EB00 gives the front waypoint); ret 0x1C */ \
    F(FN_TURN_DECIDE, int, __thiscall, (void *, void *, float, float), 0x0054880E, 0x00554F56) /* EEPhysicsModel slot 11 (AoC 12: its vtables have a slot more before; AoC rewrote it - the signs, a step ahead -, so not patched there) thiscall(model, unit, turn rate now (unit +0x17C, radians/s), angle left to turn): 3 turn faster towards it, 4 slower - from a stopping angle of the type's +0xC8 x the settings' +0x88 */ \
    D(CALL_MISSILE_AIM, 0x0054E850, 0x0055B1C3) \
    D(SITE_SIM_RECEIVE, 0x00553348, 0x00561459) \
    D(SITE_SIM_WAIT, 0x005533C5, 0x005614D6) \
    D(CALL_EPOCH_CATCH_UP, 0x0055A5B5, 0x00569633)                            /* its one call: EEPlayer::vf41, a human reached an epoch */ \
    D(EXIT_CONFIRM_AT, 0x005D0B2C, 0x005E1438) \
    D(CALL_GROUP_SECTOR_ROUTE, 0x005DB5C8, 0x005EBF0C)                        /* EEActiveGroup::vf16's call of FN_GROUP_SECTOR_ROUTE, for each cluster of the group */ \
    D(CALL_UNLOAD_MARKS, 0x005DCDBD, 0x005ED592)                              /* the group's unload order (0x5DB641): FN_ORDER_MARKS */ \
    D(CALL_GOAL_FBA_CHECK, 0x005ED9BF, 0x005FBA9C)                            /* in 0x5ED24B: FN_GOAL_FBA_CHECK on a dynamic_cast, unchecked */ \
    D(AT_GOAL_SET_READ, 0x005F0634, 0x005FE8DA) \
    D(CALL_GOALSET_ASK, 0x005F0FFA, 0x005FF364) \
    D(CALL_GOALSET_AGAIN, 0x005F1029, 0x005FF393) \
    F(FN_GROUP_SECTOR_ROUTE, void, __thiscall, (void *, int *, int *, void *), 0x005F670A, 0x00606581) /* thiscall(group, int from[2], int to[2], units): a sector route for a cluster of a group's units, each given its chain of auto-waypoint goals */ \
    D(CALL_KILL_STAT, 0x00617780, 0x0062A2DF) \
    D(CALL_ATTACK_ALERT, 0x006177CF, 0x0062A366) \
    D(CALL_AIM, 0x0061E1D7, 0x006313D6) \
    D(CALL_NODE_DEL_2, 0x00628B7F, 0x0063D930) \
    D(CALL_NODE_DEL_1, 0x00628BE7, 0x0063D998) \
    D(CALL_RANGE_SORT, 0x0063052E, 0x0064631B) \
    D(SITE_EDITOR_FORM, 0x0065BA13, 0x00673E9B) \
    D(SITE_EDITOR_RES, 0x0065FD6D, 0x00677361) \
    D(SITE_EDITOR_MENU, 0x006601F1, 0x006777E5) \
    D(SITE_CLOSE_WINDOW_JE, 0x00662666, 0x00679C57)                           /* je over the CloseWindow call on WM_ACTIVATEAPP(0) */ \
    D(FN_OP_DELETE, 0x006836F0, 0x0069D150) \
    D(IAT_SND_PLAY, 0x008232D0, 0x008372CC)                                   /* SESoundManager::Play(id, vol, pan, pitch, bool queue, char *) -> id or -1 */ \
    D(IAT_SND_PLAYREF, 0x008233EC, 0x00837380)                                /* SESoundManager::PlayAndReturnReferencedNode(same) -> node or NULL */ \
    D(IAT_TS_WAIT, 0x00823688, 0x00837634) \
    D(IAT_TS_RECEIVE, 0x008236B0, 0x0083765C) \
    D(IAT_SND_SETMAXCH, 0x00823AD4, 0x00837AC4) \
    D(IAT_SRAND, 0x00824644, 0x00838638) \
    D(VT_MSG_LOAD, 0x00824E00, 0x00838DDC)                                   /* EEMLoadTransport */ \
    D(VT_SCROLL, 0x00825098, 0x0083907C) \
    D(VT_ROUTE_PLANNER, 0x00825CC4, 0x00839C70)                               /* EEAIGPGModifierLandAttackSubGroupRoutePlanner */ \
    D(VT_COMPUTER_PLAYER, 0x008264C0, 0x0083A57C) \
    D(VT_ASG_AIR, 0x00826610, 0x0083A6D8) \
    D(VT_ASG_LAND, 0x00826CA8, 0x0083AD7C)                                    /* EECPASGLand */ \
    D(VT_ASG_SEA, 0x00826F10, 0x0083AFEC) \
    D(VT_ASG_SEA_TRANSPORT, 0x008270F8, 0x0083B1D8) \
    D(START_VTBL_EXEC, 0x008285D8, 0x0083D2A0) \
    D(VT_RESIGN, 0x00828820, 0x0083D4E8) \
    D(VT_PAY_TRIBUTE, 0x008289C0, 0x0083D688) \
    D(VT_MSG_UNLOAD, 0x00828AC4, 0x0083D78C)                                 /* EEMUnloadTransport */ \
    D(VT_INGAME_CHAT, 0x0082936C, 0x0083E0BC) \
    D(VTBL_PHYS_STEP, 0x0082CB7C, 0x00842324)                                 /* EEPhysics vtable slot 8: one simulation step */ \
    D(VT_PM_SUBMERGED, 0x0082CDD4, 0x008425F8)                                /* EEPMSubmergedNaval, the submarines' movement */ \
    D(VT_PM_NAVAL, 0x0082CE30, 0x00842658)                                    /* EEPMNaval, the ships' movement */ \
    D(VTS_TECH_RESEARCHED, 0x00830874, 0x00846168)                            /* EETechTreeNode::vf6's vtable slot */ \
    D(VT_COMPLEX_UNIT, 0x00831494, 0x00846DDC) \
    D(VT_MISSILE, 0x00832E90, 0x00848CF8) \
    D(VT_PROJECTILE, 0x00833060, 0x00848ED0)                                  /* EEUCProjectile */ \
    D(DRAW_OLD, 0x008D2D30, 0x008E94D0) \


#define ADDR_NUMBERS_HOOKS(D) \
    D(SLOT_PM_TURN_TOWARDS, 7, 7)       /* EEPhysicsModel: FN_TURN_TOWARDS (the tick's +0x1C) */ \
    D(SLOT_PM_SPEED_STEP, 8, 9)         /* EEPhysicsModel: FN_SPEED_STEP (the tick's +0x20) */ \
    D(SLOT_PM_SPEED_DECIDE, 9, 10)      /* EEPhysicsModel: FN_SPEED_DECIDE (AoC: its vf8 0x554BDE calls +0x28) */ \
    D(SLOT_PM_TURN_DECIDE, 11, 12)      /* EEPhysicsModel: FN_TURN_DECIDE (called from the turn step 0x548266 / AoC 0x554738: +0x2C / +0x30) */ \
    D(SLOT_SERVER_SEND, 6, 6)           /* [G_SERVER] (EEServer, EEClient): thiscall(net, message, 0xFF) */ \
    D(SLOT_ROUTE_COST, 1, 1)            /* VT_ROUTE_PLANNER: FN_ROUTE_COST */ \
    D(SLOT_ASGU_SCORE, 2, 2)            /* ASGU_VTABLES */ \
    D(SLOT_MSG_EXECUTE, 10, 10)         /* VT_PAY_TRIBUTE, VT_INGAME_CHAT, VT_RESIGN, VT_MSG_LOAD, VT_MSG_UNLOAD: the message's Execute */ \
    D(SLOT_UNIT_RELEASE, 13, 13)        /* VT_COMPLEX_UNIT */ \
    D(SLOT_ASG_STATS, 20, 20)           /* the attack subgroups: FN_ASG_STATS, VT_ASG_LAND's FN_ASG_LAND_STATS */ \
    D(SLOT_CP_ADD_RESOURCE, 18, 18)     /* VT_COMPUTER_PLAYER: FN_CP_ADD_RESOURCE (econ.c's hook; AoC as before) */ \
    D(SLOT_CP_PAY, 45, 45)              /* VT_COMPUTER_PLAYER: FN_CP_PAY (econ.c's hook; AoC as before) */ \
    D(SLOT_ASG_GIVE_GOAL, 57, 57)       /* the attack subgroups */ \
    D(SLOT_ASG_UPDATE, 90, 90)          /* the attack subgroups: update(now) */ \
    D(SLOT_ASG_ROUTE, 94, 94)           /* VT_ASG_LAND */ \
    D(SLOT_ASG_ROUTE_DONE, 97, 97)      /* VT_ASG_LAND */ \
    D(RVA_LLE_SEND_WAIT, 0x55DF5, 0x49DDA)    /* NESynchronousServer::Send: call [WaitForMultipleObjects] (turbo) */ \
    D(RVA_LLE_IAT_WFMO, 0x940EC, 0x970EC)     /* its import slot */ \
    D(RVA_LLE_MOUSE_COOP, 0x4C838, 0x3BA05)   /* Low-Level Engine, ISMouse::Resume: the 5 of push 5 (DISCL_ flags) */ \
    D(RVA_LLE_INPUT_COOP, 0x4A76F, 0x39607)   /* ISInitialize's, setting up the device */ \
    D(RVA_LLE_FONT_MOVSX_1, 0x27A46, 0x15C01) /* GEFontBitmap::Draw (both overloads): movsx of a character */ \
    D(RVA_LLE_FONT_MOVSX_2, 0x27C9A, 0x15E55) \
    D(SLOT_SCROLL_EXECUTE, 1, 1)        /* VT_SCROLL: Execute */ \
    D(SLOT_SP_BUTTON, 1, 1)             /* SP_VTABLES: a button's handler */ \
    D(SLOT_MISSILE_LAUNCH, 42, 43)      /* VT_MISSILE */ \
    D(SLOT_PROJ_DOWN, 55, 56)           /* VT_PROJECTILE */ \


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
    /* the calls of FN_NATIVE_CHAT (the AI's own lines) */ \
    L(NATIVE_CHAT_SITES, 5, (0x00470040, 0x004715F8, 0x0047166E, 0x0047280E, 0x00472B9D), \
        (0x00472F4C, 0x00474682, 0x00474705, 0x004758D5, 0x00475C65)) \
    /* TCP Protocol.dll, relative to its base: the network ports' immediates (UDP 33334, TCP 33335), moved for a test slot */ \
    L(RVA_TCP_PORTS, 8, (0x17E2, 0x196F, 0x19EB, 0x1EB8, 0x1FBD, 0x2148, 0x279C, 0x2F2C), \
        (0x1684, 0x185C, 0x18D8, 0x1DA5, 0x1EAA, 0x2035, 0x2689, 0x2E1C)) \
    /* the calls of FN_ASSIGN_TO_SITE (trace.c) */ \
    L(GATHER_SITES, 11, (0x0048A2A6, 0x0048A372, 0x0048A9B8, 0x0048AB53, 0x0048ABF3, 0x0048ACB7, 0x0048ADC6, 0x0048AE7A, \
        0x0048AF60, 0x0048B0A6, 0x0048B261), \
        (0x0048EC21, 0x0048ECED, 0x0048F32C, 0x0048F540, 0x0048F5E2, 0x0048F6A6, 0x0048F7B3, 0x0048F869, 0x0048F94F, 0x0048FA80, 0x0048FC37)) \
    /* call AddMovie(path) in PlayIntro 0x538867 */ \
    L(INTRO_CALLS, 3, (0x00538909, 0x0053892A, 0x00538945), \
        (0x00543A57, 0x00543A78, 0x00543A93)) \
    /* the calls of FN_FIRE (aim.c) */ \
    L(FIRE_CALLS, 4, (0x00606067, 0x00607D79, 0x0060ABB0, 0x0060B33C), \
        (0x00617838, 0x00619604, 0x0061D174, 0x0061D997)) \
    /* the minimap's operands of G_FLOAT_1_800 */ \
    L(MINIMAP_FX_OPS, 3, (0x0065E01D, 0x0065E8A3, 0x0065EB3E), \
        (0x00675618, 0x00675E95, 0x00676130)) \
    /* the minimap's operands of G_FLOAT_1_600 */ \
    L(MINIMAP_FY_OPS, 3, (0x0065E032, 0x0065E8CE, 0x0065EB68), \
        (0x0067562D, 0x00675EC0, 0x0067615A)) \
    /* the other calls of FN_SET_RESOLUTION (display.c) */ \
    L(OTHER_RESOLUTION_SITES, 7, (0x0065F30A, 0x0065F61B, 0x0065F945, 0x0065FAD4, 0x00660CD3, 0x00661329, 0x00661363), \
        (0x006768F4, 0x00676C0A, 0x00676F39, 0x006770C8, 0x0067831D, 0x00678973, 0x006789AD)) \
    /* the calls of FN_MOUSE_UPDATE */ \
    L(MOUSE_UPDATE_SITES, 4, (0x004B8ABB, 0x0050FEC8, 0x005111CF, 0x006618B8), \
        (0x004C11FE, 0x0051A487, 0x0051B73E, 0x00678F02)) \
    /* the single player screens' vtables (playername.c) */ \
    L(SP_VTABLES, 6, (0x00835740, 0x00835754, 0x00835768, 0x00835560, 0x0083577C, 0x00835790), \
        (0x0084B814, 0x0084B828, 0x0084B83C, 0x0084B65C, 0x0084B850, 0x0084B864)) \
    /* their slot 1 */ \
    L(SP_FNS, 6, (0x005D80D5, 0x005D812B, 0x00628E89, 0x005D8156, 0x005D88C0, 0x005D817A), \
        (0x005E8A41, 0x005E8B10, 0x0067CF38, 0x005E8B44, 0x005E8B68, 0x005E8B86)) \
    /* the draw list's operands to move (drawlist.c, in its table's order) */ \
    L(DRAW_SITES, 9, (0x0050099C, 0x00504158, 0x005007B2, 0x005007CC, 0x00503C69, 0x00503ED0, 0x00503FCC, 0x005043B5, \
        0x0050DC5A), \
        (0x0050B497, 0x0050F093, 0x0050B2AD, 0x0050B2C7, 0x0050EBA4, 0x0050EE0B, 0x0050EF07, 0x0050F2F0, 0x0051854F)) \

