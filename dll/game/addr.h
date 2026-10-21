// THE GAME'S ADDRESSES: every address the mod uses in Empire Earth.exe, by name, with its value in each exe build the
// mod knows - one column per k_exe_builds entry, in that order (0: the build hasn't got it). game_build (game/exe.c)
// picks the column when the DLL loads (addr_select): GA(NAME) is the address, GF(NAME) the function there, typed by
// its row, GL(NAME) a list of addresses (its count: sizeof GL(NAME) / sizeof *GL(NAME)). Another build - Art of
// Conquest, another release, a patched exe - is one more hash and one more value per row. The names: FN_ a function,
// CALL_ a call (patched), SITE_/CODE_ an instruction or operand, G_ data, VT_ a vtable, VTS_ a vtable slot's address,
// SLOT_ a vtable slot's index (a class may have gained a virtual function), SIZE_ an object's size, IAT_ an import
// slot, RVA_ a place in an engine DLL. Tables of their own for one build: editor_layout.h (generated for the 2017
// update: apply_editor_layout only there). The objects' fields: fields.h, the same columns.
// Art of Conquest's column: scripts/port_addrs.py (the two exes' code paired; its HAND table the few found by hand,
// each with its reason). Not ready: nothing ran there yet - the mod stays off there.
#define ADDR_BUILDS 2
static const struct exe_build { uint32_t hash; const char *name; int ready; } k_exe_builds[ADDR_BUILDS] = {
    { 0x4FB6A479, "ee", 1 },    // the 2017 update (GOG, Steam, empireearth.eu Vanilla)
    { 0x0644BA54, "aoc", 0 },   // The Art of Conquest, EE-AOC.exe (GOG, Steam)
};

// F(name, return type, calling convention, (parameters), value per build): a function; D(name, value per build)
#define ADDRS(F, D) \
    F(FN_TECH_LOOKUP, uint8_t *, __thiscall, (void *, int), 0x004019A1, 0x004018A4) \
    /* thiscall (tile x, tile y): set target + queue (only if tile valid) */ \
    F(FN_CAMACT_SET, void, __thiscall, (void *, int, int), 0x00401F3E, 0x00401EE0) \
    F(FN_EXPLORED, char, __thiscall, (void *, void *), 0x00404A8A, 0x004042E2) /* thiscall(site+0x14 object, player) -> bool */ \
    F(FN_SAVE_ACTION_CTOR, void *, __thiscall, (void *, void *, int, struct ustring), 0x00404FA1, 0x00404613) \
    D(SITE_SCROLL_TIMER, 0x004050C2, 0x00404734)                              /* operand of push 0x19 (timer period in ms) */ \
    F(FN_SCROLL_EXECUTE, void, __thiscall, (void *), 0x004050FC, 0x0040478E) \
    /* point from int x, y (the task location's point) */ \
    F(FN_POINT_SET, void, __thiscall, (void *, int *), 0x0040D29D, 0x0044D28B) \
    D(FN_ROUTE_COST, 0x004141E8, 0x00410074)                                  /* its slot 1: int thiscall(edge*, node*) */ \
    /* the area object at a tile (thiscall on the map areas G_MAP_AREAS) */ \
    F(FN_AREA_AT, uint8_t *, __thiscall, (void *, int *), 0x00425F16, 0x00422AD9) \
    D(FN_REGION, 0x00425F9F, 0x00422B86)                                      /* thiscall(0x917BC8, int pos[2], 2) -> short region id */ \
    /* where an idle transport lands (thiscall(transport manager, transport)) */ \
    F(FN_SHORE_TO_UNLOAD, int, __thiscall, (void *, void *), 0x0042C49A, 0x00422658) \
    F(FN_UNITS_INSIDE, int, __thiscall, (void *), 0x0042E347, 0x0040D5C5)     /* units aboard a transport or inside a building */ \
    D(CALL_CP_UPDATE, 0x0042F3CD, 0x0042CDD1) \
    F(FN_CP_ADD_RESOURCE, void, __thiscall, (void *, int, int), 0x0042F783, 0x0042D1A5) /* EEComputerPlayer::vf18(type, amount) */ \
    /* thiscall(player), called at 0x4311E5, 0x43FA9B, 0x474AF0 */ \
    F(FN_POP_CAP, void, __thiscall, (void *), 0x0042F7A5, 0x0042D1C7) \
    /* thiscall(player, on player, 1): calls off its attack groups on that player */ \
    F(FN_CALL_OFF_ATTACK, void, __thiscall, (void *, void *, int), 0x0043033E, 0x0042DCA6) \
    D(CALL_TRAIN_CITIZENS, 0x00430740, 0x0042E23C)                            /* a call of FN_TRAIN_CITIZENS */ \
    F(FN_DELETE_FOR_ROOM, int, __thiscall, (void *, void *, int), 0x00430D40, 0x0042E851) \
    /* EEComputerPlayer::AdjustCost (vf27): a price with the cheat */ \
    F(FN_ADJUST_COST, int, __thiscall, (void *, int), 0x00430FBD, 0x0042EACE) \
    D(CALL_POP_CAP_1, 0x004311E5, 0x0042ECF6)                                 /* the callers of FN_POP_CAP */ \
    D(CALL_EPOCH_UPGRADES, 0x0043D365, 0x0043C9FC)                            /* the call of FN_EPOCH_UPGRADES */ \
    /* thiscall(player, now): the next attack time (+0xED8) */ \
    F(FN_SCHEDULE_ATTACK, void, __thiscall, (void *, uint32_t), 0x0043DD2F, 0x0043D4CD) \
    /* EEComputerPlayer::vf45(cost object, ?, ?): pays a unit/building/tech */ \
    F(FN_CP_PAY, void, __thiscall, (void *, void *, int, int), 0x0043F53D, 0x0043ED98) \
    D(FN_CP_UPDATE, 0x0043F91A, 0x0043F175) \
    D(CALL_POP_CAP_2, 0x0043FA9B, 0x0043F2F6) \
    F(FN_GIVE_GOAL, void, __thiscall, (void *, void *, int), 0x004412D4, 0x0043D416) /* thiscall(unit, goal, slot) */ \
    D(FN_ADD_MOVIE, 0x00441432, 0x00440E57)                                   /* __thiscall, ret 4 */ \
    F(FN_TASK_CTOR, void, __thiscall, (void *), 0x00441FB3, 0x0044162F)       /* an attack task */ \
    F(FN_TASK_SET_TARGET, void, __thiscall, (void *, void *), 0x00442246, 0x00441912) \
    F(FN_TASK_DTOR, void, __thiscall, (void *), 0x00442283, 0x004417F5) \
    F(FN_ASG_AIR_GIVE_GOAL, char, __thiscall, (void *, void *, void *, int, int), 0x00442AD8, 0x00442220) \
    F(FN_ASG_AIR_UPDATE, void, __thiscall, (void *, int), 0x00442DBA, 0x0045019D) /* EECPASGAir slot 90: update(now) */ \
    F(FN_ASG_LAND_STATS, void, __thiscall, (void *), 0x004472A9, 0x004466DD) \
    F(FN_ASG_UPDATE, void, __thiscall, (void *, int), 0x004491DD, 0x004488CE) /* EECPASGLand slot 90: update(now) */ \
    D(RET_ASG_LAND_HANDOFF_GIVE, 0x0044AF88, 0x0044A6A3) \
    D(FN_ASG_ROUTE, 0x0044BC57, 0x0044B3C7)                                   /* its slot 94: void thiscall(int start[2], int dest[2]) */ \
    D(FN_ASG_ROUTE_DONE, 0x0044BDBF, 0x0044B53D)                              /* its slot 97: route arrived, waypoints in the deque at +0x60 */ \
    F(FN_ASG_SEA_UPDATE, void, __thiscall, (void *, int), 0x0044ED11, 0x0044F70E) /* EECPASGSea slot 90: update(now) */ \
    /* EECPASGSeaTransport slot 90: update(now) */ \
    F(FN_ASG_SEA_TRANSPORT_UPDATE, void, __thiscall, (void *, int), 0x0044FB92, 0x0044E9BB) \
    D(CALL_GROUP_STAT, 0x00450546, 0x0045122B) \
    F(FN_GROUP_STAT, void, __thiscall, (void *, void *), 0x00450554, 0x00451239) \
    F(FN_UNIT_TO_GROUP, char, __thiscall, (void *, void *), 0x00450E07, 0x00451A8A) \
    /* thiscall(attack manager, task, strategy, unit ids) -> the attack group */ \
    F(FN_ATTACK_GROUP_NEW, uint8_t *, __thiscall, (void *, void *, void *, void *), 0x0045106C, 0x00451D8E) \
    F(FN_ASG_STATS, void, __thiscall, (void *), 0x0045AD2F, 0x0045C925) \
    F(FN_ASG_GIVE_GOAL, char, __thiscall, (void *, void *, void *, int, int), 0x0045CBEA, 0x0045E7F9) \
    D(FN_ASGU_SCORE, 0x00461854, 0x00463494)                                  /* float thiscall(info *) */ \
    F(FN_BM_QUEUED, char, __thiscall, (void *, int), 0x00465640, 0x004672FA)  /* thiscall(build manager, type): queued */ \
    F(FN_PLACE, char, __thiscall, (void *, int, int, int), 0x0046580A, 0x004674C4) \
    F(FN_ENEMY_NEAR, char, __thiscall, (void *, int, int), 0x004660B9, 0x00467E10) /* thiscall(build manager, x, y) -> bool */ \
    /* thiscall(build manager, type, 1, 0): one building requested */ \
    F(FN_BM_REQUEST, void, __thiscall, (void *, int, int, int), 0x0046802D, 0x0046A9C1) \
    D(CALL_PLACE, 0x00468629, 0x0046B044) \
    F(FN_BQ_ADD, void, __thiscall, (void *, int *), 0x00468C37, 0x0046B798)   /* an entry into a build manager epoch queue */ \
    F(FN_FISH_TYPE, int, __thiscall, (void *), 0x0046D209, 0x0046FDD3) \
    F(FN_HUMAN_EPOCH, int, __cdecl, (void), 0x0046D436, 0x0047005A)           /* the highest epoch of the players SLOT_PLAYER_HUMAN says are human, 0 without */ \
    F(FN_EPOCH_CATCH_UP, void, __thiscall, (void *, void *), 0x00431533, 0x0042F064) /* thiscall(computer player, human): the attempt redrawn soon when the human is ahead */ \
    D(CALL_EPOCH_CATCH_UP, 0x0055A5B5, 0x00569633)                            /* its one call: EEPlayer::vf41, a human reached an epoch */ \
    D(CALL_REPAIR_GOAL, 0x0046DA88, 0x004706DF) \
    F(FN_SET_EPOCH_TIME, void, __thiscall, (void *, uint32_t, char), 0x0046DE8C, 0x00470B04) /* SetNextEpochAdvanceTime */ \
    D(CALL_RM_UPDATE, 0x0046E3C6, 0x00471051)                                 /* call EECPResourceManager::Update(now)              (0x48D263) */ \
    F(FN_REVENGE, void, __thiscall, (void *, void *), 0x0046FFB7, 0x00472EC3) \
    /* thiscall(cm, task, strategy, unit ids) */ \
    F(FN_ATTACK_PLAN_OK, char, __thiscall, (void *, void *, void *, void *), 0x0047067A, 0x0047360A) \
    F(FN_CM_UNIT_FREE, char, __thiscall, (void *, void *), 0x004707D8, 0x0047377E) /* thiscall(cm, unit state): in no group */ \
    F(FN_STRATEGY_CTOR, void, __thiscall, (void *), 0x00473BC4, 0x00477084) \
    F(FN_STRATEGY_DTOR, void, __thiscall, (void *), 0x00473C99, 0x0047716D) \
    F(FN_STRATEGY_INIT, void, __thiscall, (void *), 0x00473CA7, 0x0047717B) \
    D(CALL_POP_CAP_3, 0x00474AF0, 0x004781C9) \
    D(CALL_NV_MAINT_DELETE, 0x00477B17, 0x0047B635) \
    F(FN_LOCATION_CTOR, void, __thiscall, (void *), 0x0047C4E8, 0x00480873) \
    F(FN_TASK_SET_LOCATION, void, __thiscall, (void *, void *), 0x0047C8B7, 0x00480C42) \
    D(CALL_UNIT_TO_GROUP, 0x004824BF, 0x004869F3) \
    D(CALL_SHORE_TO_UNLOAD, 0x00483E24, 0x00488490)                           /* the idle transports' call of FN_SHORE_TO_UNLOAD */ \
    F(FN_NATIVE_CHAT, char, __thiscall, (void *, int, int), 0x00484EB3, 0x0048966C) \
    F(FN_RS_SCHEDULE, void, __thiscall, (void *, int), 0x00485F57, 0x0048A79F) \
    /* thiscall(research manager cm+0x40, count): the AI's request for an epoch's required techs */ \
    F(FN_REQUEST_TECHS, void, __thiscall, (void *, int), 0x004860D6, 0x0048A91E) \
    /* thiscall(zone, type) -> first site of that type with amount > 0 */ \
    F(FN_ZONE_SITE, uint8_t *, __thiscall, (void *, int), 0x004886F2, 0x0048D065) \
    D(LEFTOVER_SITE_END, 0x0048A2B9, 0x0048EC34)                              /* call sites below this are in the leftover pass 0x48A1AE */ \
    D(FN_ASSIGN_CITIZENS, 0x0048A554, 0x0048EECF) \
    D(CALL_FARMS, 0x0048AC54, 0x0048F643)                                     /* the call of FN_FARMS */ \
    D(CALL_HUNT_1, 0x0048ACE4, 0x0048F6D2)                                    /* the calls of FN_HUNT */ \
    D(CALL_HUNT_2, 0x0048AFC4, 0x0048F9B3) \
    D(CALL_UNBUILT_FARMS, 0x0048B0FD, 0x0048FAD4) \
    D(FN_HUNT, 0x0048BA3B, 0x00490458)                                        /* (count, flag, out list) */ \
    D(FN_FIND_FOOD, 0x0048BBA8, 0x00490602)                                   /* (char needDropsite, char nearBase, unit) -> site */ \
    D(FN_UNBUILT_FARMS, 0x0048BF4F, 0x004909D7) \
    D(FN_FARMS, 0x0048BFD0, 0x00490A63)                                       /* (count, out list) */ \
    D(FN_FIND_WOOD, 0x0048C060, 0x00490AF3)                                   /* (flag, unit) -> target */ \
    D(FN_FIND_MINE, 0x0048C204, 0x00490CB5)                                   /* (type, unit, reverse) -> site */ \
    F(FN_ASSIGN_TO_SITE, void, __thiscall, (void *, void *, void *, int), 0x0048C81C, 0x004912BE) /* (unit, target, 0) */ \
    D(CALL_UNLOAD_RM, 0x0048CC5D, 0x00491733)                                 /* an unload call (the resource manager's ferry) */ \
    D(CALL_UNLOAD_RETRY_GIVE, 0x0048D03C, 0x00491B12) \
    D(FN_COMPUTE_TARGETS, 0x0048D0A2, 0x00491B78) \
    D(FN_RM_UPDATE, 0x0048D263, 0x00491D34) \
    D(CALL_COMPUTE_TARGETS, 0x0048D347, 0x00491E18)                           /* call EECPResourceManager::ComputeGathererTargets (0x48D0A2) */ \
    D(CALL_ASSIGN_CITIZENS, 0x0048D36A, 0x00491E3B)                           /* call EECPResourceManager::AssignCitizens        (0x48A554) */ \
    F(FN_TABLE_VALUE, unsigned, __thiscall, (void *, int), 0x0048E817, 0x004CD4B3) \
    D(FN_UM_PENDING, 0x0048E9D8, 0x00493696)                                  /* thiscall(unit manager, type) */ \
    D(FN_UM_IN_PRODUCTION, 0x0048EA2E, 0x004936EC)                            /* thiscall(unit manager, type) */ \
    F(FN_UM_CITIZENS_WANTED, int, __thiscall, (void *), 0x0048EDDB, 0x00493AEE) /* citizen share x population cap */ \
    F(FN_TRAIN_CITIZENS, uintptr_t, __thiscall, (void *, uintptr_t), 0x0048EE84, 0x00493B97) \
    /* citizens of both sexes and the epoch's fishing boat type */ \
    F(FN_UM_CITIZENS, int, __thiscall, (void *), 0x0048F819, 0x00494572) \
    D(CALL_FISH_TYPE_1, 0x0048F823, 0x0049457C)                               /* the calls of FN_FISH_TYPE */ \
    /* thiscall(unit manager, type): its units and buildings of that type */ \
    F(FN_UM_TYPE_LIST, uint8_t *, __thiscall, (void *, int), 0x0048F8AF, 0x00494608) \
    D(CALL_FISH_TYPE_2, 0x0048FB16, 0x0049488C) \
    F(FN_TRAIN_UNITS, uintptr_t, __thiscall, (void *, int *, uintptr_t), 0x0049014F, 0x00494F42) \
    D(CALL_CITIZENS_WANTED, 0x00490BC2, 0x00495A15)                           /* a call of FN_UM_CITIZENS_WANTED */ \
    /* thiscall(player +0x874, type): the per-player stats record */ \
    F(FN_TYPE_STATS, uint8_t **, __thiscall, (void *, int), 0x00492969, 0x0051E2A5) \
    /* thiscall(wall manager, int from[2], int to[2], type, char) */ \
    F(FN_BUILD_WALL, void, __thiscall, (void *, int *, int *, int, char), 0x00497C3F, 0x0049D05F) \
    F(FN_VECTOR_SIZE, unsigned, __thiscall, (void *), 0x004A670A, 0x0047C428) \
    F(FN_INGAME_CHAT, void, __thiscall, (void *), 0x004B78C7, 0x004BFDD2) \
    F(FN_PAY_TRIBUTE, void, __thiscall, (void *), 0x004BA6BE, 0x004C315B) \
    F(FN_RESIGN, void, __thiscall, (void *), 0x004BB906, 0x004C44BB) \
    /* EEMSelectUnits: thiscall(msg, rect, type, 0x4E) */ \
    F(FN_SELECT_MSG_INIT, void, __thiscall, (void *, void *, int, int), 0x004BD072, 0x004C5D7F) \
    D(CALL_SEL_PACK, 0x004BD133, 0x004C5E40) \
    F(FN_SEL_PACK, unsigned, __cdecl, (void *, uint32_t, uint8_t *), 0x004BD15A, 0x004C5E67) \
    D(START_EXEC, 0x004BEE57, 0x004C7CDC) \
    D(CALL_GAME_START, 0x004CEC14, 0x004D4443) \
    F(FN_DAMAGE_POINTS, int, __thiscall, (void *, int, unsigned), 0x004D291F, 0x004D84EC) \
    /* thiscall(G_BONUS_TABLE, weapon kind, target type data, attacker, target player) */ \
    F(FN_ATTACK_BONUS, float, __thiscall, (void *, int, void *, int, int), 0x004D5F2C, 0x004DBE44) \
    D(CALL_DAMAGE_POINTS, 0x004F80A4, 0x0050232D) \
    F(FN_DRAW_REC_CTOR, void, __thiscall, (void *), 0x00506442, 0x005118E9)   /* a draw list record */ \
    D(FN_MOUSE_CLAMP, 0x00510DAD, 0x0051B31C) \
    D(FN_MOUSE_UPDATE, 0x00510E5D, 0x0051B3CC) \
    D(FN_ATTACK_ALERT, 0x00519BC1, 0x00524497) \
    /* thiscall on G_CAMERA (float dx, float dy) */ \
    F(FN_CAMERA_MOVE, void, __thiscall, (void *, float, float), 0x0051E2E6, 0x00528EA5) \
    /* thiscall([G_CHEAT_PARSER], UWideString *, int *code, int *arg) */ \
    F(FN_CHEAT_PARSE, void, __thiscall, (void *, void *, int *, int *), 0x00520A17, 0x0052B5D6) \
    D(FN_LOAD_SAVED, 0x00521F94, 0x0052CB90) \
    D(FN_PARSE_CMDLINE, 0x00522A36, 0x0052D703) \
    D(CALL_LOAD_SAVED, 0x00522BF8, 0x0052D8F9)                                /* call LoadSavedGameFromCmdline(UString *) 0x521F94 */ \
    D(FN_GAME_START, 0x0052599B, 0x005306F2) \
    D(CALL_PLAYER_COLOR, 0x005262E4, 0x00531057) \
    F(FN_COLOR_RGB, void *, __cdecl, (unsigned), 0x00526B85, 0x005318BD) \
    F(FN_SETTINGS_SAVE, void, __thiscall, (void *), 0x0052AA81, 0x00535943)   /* every setting into the registry */ \
    F(FN_SET_STANCE, void, __thiscall, (void *, int, int, int), 0x0052D5D7, 0x005384DD) \
    /* thiscall(G_GAME_STATE, char won, index) */ \
    F(FN_GAME_PLAYER_OUT, void, __thiscall, (void *, int, int), 0x0052E2B7, 0x005391F2) \
    F(FN_SITE_GATHERERS, unsigned, __thiscall, (void *, void *), 0x0052E5F7, 0x00539532) \
    F(FN_GAME_PLAYER, void *, __thiscall, (void *, int), 0x0052E6EF, 0x00539670) /* thiscall(G_GAME_STATE, index) */ \
    D(CODE_SPLASH_START_JNE, 0x00534400, 0x0053F2FE)                          /* the loading splash's start (jne) */ \
    D(CODE_SPLASH_STOP_JNE, 0x0053443D, 0x0053F33B)                           /* the loading splash's stop (jne) */ \
    D(CALL_PARSE_CMDLINE, 0x00535EF2, 0x005410C8)                             /* call ParseCommandLine(const char *) 0x522A36 */ \
    F(FN_SAVE_GAME, int, __cdecl, (void *, int, int, struct ustring), 0x00540ACA, 0x0054D066) \
    F(FN_PHYS_STEP, void, __thiscall, (void *), 0x00546C63, 0x00553082) \
    D(CALL_MISSILE_AIM, 0x0054E850, 0x0055B1C3) \
    D(SITE_SIM_RECEIVE, 0x00553348, 0x00561459) \
    D(SITE_SIM_WAIT, 0x005533C5, 0x005614D6) \
    /* thiscall(player, type, 0x20, 0): can train or build one now */ \
    F(FN_CAN_BUILD, char, __thiscall, (void *, int, int, int), 0x00555710, 0x005639E8) \
    F(FN_POPULATION, int, __thiscall, (void *, int, int), 0x00556E11, 0x005651E0) /* thiscall(player, 1, 0) */ \
    /* EEPlayer::vf31 thiscall(player, 0): the game's unit limit for this player */ \
    F(FN_GAME_LIMIT, int, __thiscall, (void *, int), 0x00556EBC, 0x0056528B) \
    F(FN_SELECT_UNITS, void, __thiscall, (void *, void *), 0x0055B3FB, 0x0056A658) /* EEMSelectUnits carried out on the player */ \
    D(FN_SET_DIPLOMACY, 0x0055B8E1, 0x0056AB41) \
    F(FN_PLAYER_RESIGN, void, __thiscall, (void *, int), 0x0055BCEA, 0x0056AF5C) /* thiscall(player, 2) as EEMResign */ \
    F(FN_STAT_ADD, void, __thiscall, (void *, int, int), 0x0055DBCF, 0x0056D507) \
    F(FN_CAMACT_CTOR, void *, __thiscall, (void *), 0x00560179, 0x0056FAD4)   /* EEACenterCamera ctor (0x1C bytes) */ \
    F(FN_EDITOR_FORM, void *, __thiscall, (void *, void *), 0x0058D3E1, 0x0059D615) \
    F(SM_RUN_EFFECT, void, __thiscall, (void *, int), 0x005AD29B, 0x005BD5EB) \
    D(FN_EPOCH_UPGRADES, 0x005BDDBD, 0x005CE8C5)                              /* thiscall(node, category), AoC's (+ only new): econ.c */ \
    F(FN_EPOCH_NEXT, void, __thiscall, (void *), 0x005BC15D, 0x005CCB34)    /* thiscall(tech tree): the next epoch (to the game's end epoch) - 0x5BC893 sets it, sends the epoch event */ \
    F(FN_TECH_GRANT, int, __thiscall, (void *), 0x005BF335, 0x005CFA53)       /* AoC's takes an argument: called in the 2017 update only */ \
    F(FN_CHEAT_MSG_CTOR, void *, __thiscall, (void *, int, int), 0x005C29FB, 0x005D3162) /* new 0x18; (code, arg) */ \
    F(FN_CHAT_MSG_CTOR, void *, __thiscall, (void *, void *), 0x005C2A2F, 0x005D3196) /* new 0x24; (UWideString *) */ \
    F(FN_RESIGN_SEND, void, __cdecl, (void), 0x005D05CC, 0x005E0E7F) \
    D(EXIT_CONFIRM_AT, 0x005D0B2C, 0x005E1438) \
    F(FN_EXIT_CONFIRM, void, __cdecl, (void), 0x005D0B4F, 0x005E145B) \
    F(FN_UNIT_RELEASE, void, __thiscall, (void *), 0x005ED71E, 0x005FB7FB) \
    D(CALL_GOAL_FBA_CHECK, 0x005ED9BF, 0x005FBA9C)                            /* in 0x5ED24B: FN_GOAL_FBA_CHECK on a dynamic_cast, unchecked */ \
    /* thiscall(unit, 0x1F51) - as the assignment does after the order */ \
    F(FN_UNIT_SAY, void, __thiscall, (void *, int), 0x005EFCF0, 0x005FDFA5) \
    D(FN_SET_GOAL, 0x005F05BD, 0x005FE863)                                    /* thiscall(unit, goal, 0, 0) */ \
    D(AT_GOAL_SET_READ, 0x005F0634, 0x005FE8DA) \
    D(CALL_GOALSET_ASK, 0x005F0FFA, 0x005FF364) \
    D(CALL_GOALSET_AGAIN, 0x005F1029, 0x005FF393) \
    /* EEUCMissile vf42, thiscall(class, missile, aim) - ret 8, the aim unused */ \
    F(FN_MISSILE_LAUNCH, int, __thiscall, (void *, void *, float *), 0x00606109, 0x0061799B) \
    F(FN_PROJ_DOWN, char, __thiscall, (void *, void *), 0x006078F9, 0x00619143) \
    D(FN_GOAL_ATTACK_CTOR, 0x0060AD45, 0x0061D337)                            /* EEUGAttack(unit, target, 1); AoC + a byte: attack.c */ \
    /* EEUGBuildRepair constructor: thiscall(goal, player, building, 1) */ \
    F(FN_BUILD_GOAL, void *, __thiscall, (void *, void *, void *, int), 0x0060BAD0, 0x0061E164) \
    /* thiscall(goal 0xFBA object, unit) (0x5ED24B) */ \
    F(FN_GOAL_FBA_CHECK, char, __thiscall, (void *, void *), 0x0060FE7E, 0x006226CF) \
    /* EEUGLoadTransport: new 0x68 */ \
    F(FN_GOAL_LOAD_CTOR, void *, __thiscall, (void *, void *, void *, int), 0x0061487F, 0x00627286) \
    /* new 0x38 */ \
    F(FN_GOAL_MOVE_CTOR, void *, __thiscall, (void *, float *, int, char, char, float, char, char), 0x00614CE6, 0x006276E1) \
    /* EEUGUnloadTransport: new 0x40 */ \
    F(FN_GOAL_UNLOAD_CTOR, void *, __thiscall, (void *, void *, int, int, int), 0x00615A1F, 0x006283FF) \
    D(CALL_KILL_STAT, 0x00617780, 0x0062A2DF) \
    D(CALL_ATTACK_ALERT, 0x006177CF, 0x0062A366) \
    F(FN_GOALSET_ASK, int, __thiscall, (void *, void *, int *), 0x00619FF5, 0x0062CD8B) \
    F(FN_VECTOR_FREE, void, __thiscall, (void *), 0x0061ABEC, 0x006002C7)     /* a vector's dtor (the game's delete) */ \
    F(FN_FIRE, void, __thiscall, (void *, void *), 0x0061E11C, 0x00631312) /* thiscall(attacker class, attacker) */ \
    D(CALL_AIM, 0x0061E1D7, 0x006313D6) \
    D(CALL_NODE_DEL_2, 0x00628B7F, 0x0063D930) \
    D(CALL_NODE_DEL_1, 0x00628BE7, 0x0063D998) \
    F(FN_UNIT_UNLINK, void, __thiscall, (void *), 0x00628E29, 0x0063DBDA) \
    D(FN_BALLISTIC, 0x0062CBB3, 0x00641A6A) \
    F(FN_UNIT_DISTANCE, float, __cdecl, (void *, void *), 0x0062D5A4, 0x00642489) /* cdecl(unit, other) */ \
    /* cdecl char(player, type, x, y, w, h, 0) */ \
    F(FN_CAN_PLACE, char, __cdecl, (void *, int, int, int, int, int, int), 0x0062E940, 0x00643CE6) \
    /* cdecl(player, type, x, y): a building placed (a foundation) */ \
    F(FN_PLACE_BUILDING, void *, __cdecl, (void *, int, int, int), 0x0062EF8A, 0x00644C83) \
    /* cdecl(unit, player index, new type data) */ \
    F(FN_UNIT_CONVERT, int, __cdecl, (void *, int, uint32_t), 0x0062F192, 0x00644E95) \
    D(CALL_RANGE_SORT, 0x0063052E, 0x0064631B) \
    D(FN_AIM, 0x00633413, 0x0064939D) \
    F(FN_RANGE_SORT, void, __cdecl, (void **, void **, void *), 0x0063A355, 0x006509AF) \
    /* EEMHackANewUnit: thiscall(int *type, float xy[2], char player) */ \
    F(FN_NEW_UNIT_MSG_CTOR, void, __thiscall, (void *, int *, float *, char), 0x00650B3C, 0x00662C24) \
    D(SITE_EDITOR_FORM, 0x0065BA13, 0x00673E9B) \
    D(FN_SET_RESOLUTION, 0x0065F314, 0x006768FE) \
    D(SITE_EDITOR_RES, 0x0065FD6D, 0x00677361) \
    D(SITE_EDITOR_MENU, 0x006601F1, 0x006777E5) \
    D(FN_QUEUE_ACTION, 0x0066192F, 0x00678F79) \
    /* thiscall(ui [G_UI], colour, name, line, ms, 0): a chat or system line */ \
    F(FN_UI_LINE, void, __thiscall, (void *, void *, void *, void *, int, int), 0x00661E91, 0x006794C6) \
    D(SITE_CLOSE_WINDOW_JE, 0x00662666, 0x00679C57)                           /* je over the CloseWindow call on WM_ACTIVATEAPP(0) */ \
    D(FN_OP_DELETE, 0x006836F0, 0x0069D150) \
    F(FN_NEW, void *, __cdecl, (size_t), 0x00683718, 0x0069D178)              /* the game's operator new */ \
    D(IAT_RESOURCE_STRING, 0x0082319C, 0x00837198)                            /* the game's string resource (a language entry) */ \
    D(IAT_WSTRING_CTOR, 0x008231A4, 0x008371A0)                               /* UWideString::UWideString() */ \
    D(IAT_RELEASE, 0x008231AC, 0x008371A8)                                    /* imported refcount release, thiscall on the action */ \
    D(IAT_USTRING_CTOR, 0x008231E4, 0x008371E0)                               /* UString::UString() */ \
    D(IAT_WSTRING_ASSIGN, 0x00823224, 0x00837220)                             /* UWideString::operator=(const char *) */ \
    D(IAT_USTRING_ASSIGN, 0x00823228, 0x00837224)                             /* UString::operator=(const char *) */ \
    D(IAT_WSTRING_CTOR_C, 0x008232C4, 0x008372C0)                             /* UWideString::UWideString(const char *) */ \
    D(IAT_WSTRING_FORMAT, 0x008232C8, 0x008372C4)                             /* UWideString::Format(const char *, ...) */ \
    D(IAT_SND_PLAY, 0x008232D0, 0x008372CC)                                   /* SESoundManager::Play(id, vol, pan, pitch, bool queue, char *) -> id or -1 */ \
    D(IAT_COS, 0x00823304, 0x00837340)                                        /* cos (the game's) */ \
    D(IAT_EDIT_GET_TEXT, 0x00823328, 0x00837EE0)                              /* UIEditBox::GetText */ \
    D(IAT_SND_PLAYREF, 0x008233EC, 0x00837380)                                /* SESoundManager::PlayAndReturnReferencedNode(same) -> node or NULL */ \
    D(IMP_IS_MOUSE, 0x008235CC, 0x00837570)                                   /* import: &ISMouse* in Low-Level Engine.dll */ \
    D(IAT_WSTRING_RESERVE, 0x008235D0, 0x00837574) \
    D(IAT_TS_WAIT, 0x00823688, 0x00837634) \
    D(IAT_TS_RECEIVE, 0x008236B0, 0x0083765C) \
    D(IAT_SND_SETMAXCH, 0x00823AD4, 0x00837AC4) \
    D(IAT_WSTRING_DTOR, 0x00823AEC, 0x00837194)                               /* UWideString::~UWideString() */ \
    D(IMP_IS_KEYBOARD, 0x00823AF8, 0x00837AE8)                                /* import: &ISKeyboard* in Low-Level Engine.dll */ \
    D(IAT_UIM_FORM, 0x00823D60, 0x00837B44)                                   /* the UI manager's current form */ \
    D(IAT_USTRING_DTOR, 0x008245BC, 0x008385B0)                               /* UString::~UString() */ \
    D(IAT_SRAND, 0x00824644, 0x00838638) \
    D(IAT_RAND, 0x00824648, 0x0083863C)                                       /* the game's rand */ \
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
    D(VT_INGAME_CHAT, 0x0082936C, 0x0083E0BC) \
    D(VTBL_PHYS_STEP, 0x0082CB7C, 0x00842324)                                 /* EEPhysics vtable slot 8: one simulation step */ \
    D(G_FLOAT_1_600, 0x0082CBCC, 0x00842374)                                  /* the float 1/600 (the minimap's y scale operand) */ \
    D(VTS_TECH_RESEARCHED, 0x00830874, 0x00846168)                            /* EETechTreeNode::vf6's vtable slot */ \
    D(VT_PAUSE_ACTION, 0x00830E94, 0x00846790)                                /* EEAUserPauseGame vtable (0xC bytes: vtable, refcount, byte) */ \
    D(VT_COMPLEX_UNIT, 0x00831494, 0x00846DDC) \
    D(VT_GOAL_ATTACK, 0x00831968, 0x00847380) \
    D(VT_GOAL_BUILD, 0x008319F0, 0x0084740C) \
    /* EEUGLoadTransport: the holder's goal and each unit's part (0x614833), the holder at +0x50 */ \
    D(VT_GOAL_LOAD, 0x00831DC0, 0x008477F8) \
    D(VT_GOAL_LOCATION, 0x00831E64, 0x008478A0) \
    D(VT_GOAL_UNLOAD, 0x00831F5C, 0x008479A0)                                 /* EEUGUnloadTransport (new 0x40, ctor 0x615A1F(transport, x, y, flag)) */ \
    D(VT_MISSILE, 0x00832E90, 0x00848CF8) \
    D(VT_PROJECTILE, 0x00833060, 0x00848ED0)                                  /* EEUCProjectile */ \
    D(G_FLOAT_1_800, 0x008358A8, 0x0084B940)                                  /* the float 1/800 (the minimap's x scale operand) */ \
    /* the scenario editor's file buttons: 6 of {id, x, y, u0, v0, u1, v1, tooltip} */ \
    D(G_EDITOR_FILE_BUTTONS, 0x008AE0A0, 0x008C47B0) \
    D(G_COLORS_BEGIN, 0x008CFA74, 0x008E610C)                                 /* the player colour table (vector begin, end) */ \
    D(G_COLORS_END, 0x008CFA78, 0x008E6110) \
    D(G_HIT_TABLE_A, 0x008D2008, 0x008E870C)                                  /* the hit table's variants by game mode */ \
    D(G_HIT_TABLE, 0x008D200C, 0x008E8708)                                    /* [target category][weapon kind]: 0 = it cannot hit */ \
    D(G_HIT_TABLE_B, 0x008D2010, 0x008E8710) \
    D(G_BEHAVIOUR_TABLE, 0x008D2044, 0x008E8744)                              /* the unit behaviours (stances) */ \
    D(G_DAMAGE_TABLE, 0x008D2050, 0x008E8750)                                 /* [attacker class + target class x 6] */ \
    D(G_SELECT_RECT, 0x008D2778, 0x008E8F18)                                  /* a default selection rectangle */ \
    D(DRAW_OLD, 0x008D2D30, 0x008E94D0) \
    D(G_DRAW_N, 0x00912538, 0x00928CD8)                                       /* draw list counts: objects, small objects */ \
    D(G_DRAW_SMALL_N, 0x0091253C, 0x00928CDC) \
    D(G_TABLE_VALUES, 0x00917BB0, 0x0092FCA0)                                 /* FN_TABLE_VALUE's table (0x16: the population limit) */ \
    D(G_GAME_DATA, 0x00917BB8, 0x0092FCA8)                                    /* game data loaded (unit limit +0x2EC, chat time +0x2CC) */ \
    D(G_MAP_AREAS, 0x00917BC8, 0x00930BF0)                                    /* the map's areas (regions, continents) */ \
    D(G_BONUS_TABLE, 0x00917DE0, 0x009318E8)                                  /* FN_ATTACK_BONUS's object */ \
    D(G_GAME_STATE, 0x00918300, 0x00930D40)                                   /* object; vtable slot 11 called after a speed change */ \
    D(G_PLAYERS, 0x00918374, 0x00930DB4) \
    D(G_MAP, 0x009183BC, 0x00930DFC)                                          /* ptr; +0x1816FC = map size in tiles */ \
    D(G_GAME_SPEED, 0x009183C0, 0x00930E00)                                   /* float; set like EEMGameSpeed::Apply (0x4B6BD4) */ \
    D(G_PLAYERS_N, 0x009183CC, 0x00930E0C)                                    /* the number of players */ \
    D(G_POP_LIMIT, 0x009183D4, 0x00930E14)                                    /* the population limit game setting */ \
    D(G_END_EPOCH, 0x009187F4, 0x00931234)                                    /* the game's end epoch */ \
    D(G_GAME_MODE, 0x00918808, 0x00931248) \
    D(G_SCENARIO_FLAG, 0x00918819, 0x00931259)                                /* byte: game is a scenario/campaign (settings 0x918410 + 0x409) */ \
    D(G_GAME_TYPE, 0x00918BF4, 0x00931634)                                    /* 0 random map, 1 scenario, 2 saved game, 3/4 other */ \
    D(G_DIFFICULTY, 0x00918C14, 0x00931654)                                   /* the AIs' difficulty (the setup's) */ \
    D(G_CAMERA, 0x00918CD0, 0x00931710)                                       /* EECameraControl; +0x78/+0x7C/+0x80 look-at x/y/z (tiles), +0xE0 zoom */ \
    D(G_SCROLL_SPEED, 0x00918DB0, 0x009317F0)                                 /* float */ \
    D(G_SM_MGR, 0x00918E24, 0x00931864)                                       /* the scenario manager */ \
    D(G_LOCAL_PLAYER, 0x00918E84, 0x009318C4)                                 /* int: index of the player on this PC (the human in single player) */ \
    D(G_CHEATS_ON, 0x00918E9A, 0x009318DA) \
    D(G_TEAMS_LOCKED, 0x00918E9F, 0x009318DF) \
    D(G_PAUSED, 0x00918EA2, 0x009318E2)                                       /* byte, toggled by EEAUserPauseGame::Execute (0x4064F1) */ \
    D(G_SETTINGS, 0x00918EA8, 0x00930308)                                     /* the game settings object */ \
    D(G_PLAYER_NAME, 0x00919308, 0x00930768)                                  /* the player name (UWideString) */ \
    D(G_SCROLL_SETTING, 0x009193A8, 0x00930828)                               /* float, settings slider x 0.01 */ \
    D(G_SOUND_PERCENT, 0x009193B8, 0x00930838) \
    D(G_GAME_RES_H, 0x009193F8, 0x00930878) \
    D(G_GAME_RES_W, 0x009193FC, 0x0093087C) \
    D(G_SCREEN, 0x00919428, 0x009308A8)                                       /* the current screen index */ \
    D(G_FULLSCREEN, 0x0091945A, 0x009308DB) \
    D(G_RESOURCES, 0x00919770, 0x009318EC)                                    /* the string resources */ \
    D(G_WORLD_CLOCK, 0x00919774, 0x009318F0)                                  /* ptr to EEPhysics; +0xD0 = game time in ms */ \
    D(G_WORLD, 0x0091977C, 0x009318F8)                                        /* action queue owner: QueueAction 0x66192F (thiscall, takes action ptr) */ \
    D(G_RASTERIZER, 0x00919794, 0x00931910) \
    D(G_UI_MANAGER, 0x009197A0, 0x0093191C) \
    D(G_GAME_WINDOW, 0x009197A4, 0x00931920)                                  /* GEWindow* (+0 HWND) */ \
    D(G_MOUSE, 0x009197A8, 0x00931924)                                        /* RTSMouse */ \
    D(G_SOUND_MANAGER, 0x00919848, 0x009319C4) \
    /* EEClient/EEServer (lockstep server, slot 6 takes a message); UI checks [+0x10] != 0 and vtable+0x10 before */ \
    /* pausing */ \
    D(G_SERVER, 0x0091984C, 0x009319C8) \
    D(G_CHEAT_PARSER, 0x00919858, 0x009319D4) \
    D(G_INPUT_MSGS_DONE, 0x00919B68, 0x00931CF0)                              /* the user-input messages processed so far */ \
    D(G_EFFECTS, 0x00919E70, 0x00931FF8)                                      /* tech effects, 0x44 each */ \
    D(G_EPOCH_TECHS, 0x0093B1B8, 0x0095CF80)                                  /* per epoch 0x10: the tech lists */ \
    D(G_TYPES, 0x009418A0, 0x009636AC) \


// numbers that aren't addresses: SLOT_ a virtual function's index in its class's vtable (patch_vtable, a call through
// the vtable; slot 0, the deleting destructor in both builds, stays a plain 0), SIZE_ the size of a game object the
// mod makes itself (FN_NEW, then its constructor), RVA_ a place in an engine DLL relative to its base - each build
// ships its own DLLs (scripts/port_dll.py); what the DLLs export is taken by name instead. A patch there checks the
// bytes first: a DLL from elsewhere is left alone.
#define ADDR_NUMBERS(D) \
    D(RVA_LLE_MOUSE_COOP, 0x4C838, 0x3BA05)   /* Low-Level Engine, ISMouse::Resume: the 5 of push 5 (DISCL_ flags) */ \
    D(RVA_LLE_INPUT_COOP, 0x4A76F, 0x39607)   /* ISInitialize's, setting up the device */ \
    D(RVA_LLE_FONT_MOVSX_1, 0x27A46, 0x15C01) /* GEFontBitmap::Draw (both overloads): movsx of a character */ \
    D(RVA_LLE_FONT_MOVSX_2, 0x27C9A, 0x15E55) \
    D(RVA_LLE_SEND_WAIT, 0x55DF5, 0x49DDA)    /* NESynchronousServer::Send: call [WaitForMultipleObjects] (turbo) */ \
    D(RVA_LLE_IAT_WFMO, 0x940EC, 0x970EC)     /* its import slot */ \
    D(RVA_MIX_CHANNELS, 0x52AC, 0x52AC)       /* Miles Sound System Mixer.dll: its channels, a vector of 8 bytes each */ \
    D(RVA_MIX_MAX_CHANNELS, 0x5320, 0x5320)   /* int: the most channels it gives */ \
    D(SLOT_ROUTE_COST, 1, 1)            /* VT_ROUTE_PLANNER: FN_ROUTE_COST */ \
    D(SLOT_SCROLL_EXECUTE, 1, 1)        /* VT_SCROLL: Execute */ \
    D(SLOT_SP_BUTTON, 1, 1)             /* SP_VTABLES: a button's handler */ \
    D(SLOT_TECH_AVAILABLE, 1, 1)        /* EETechTreeNode: char thiscall(tech) */ \
    D(SLOT_ASGU_SCORE, 2, 2)            /* ASGU_VTABLES */ \
    D(SLOT_GOAL_PART, 4, 4)             /* EEUGLoadTransport: a unit's part of the load goal */ \
    D(SLOT_SERVER_SEND, 6, 6)           /* [G_SERVER] (EEServer, EEClient): thiscall(net, message, 0xFF) */ \
    D(SLOT_GROUP_COUNT, 6, 6)           /* the selected group (player +0x9BC): unsigned thiscall(group); AoC not checked */ \
    D(SLOT_MSG_EXECUTE, 10, 10)         /* VT_PAY_TRIBUTE, VT_INGAME_CHAT, VT_RESIGN: the message's Execute */ \
    D(SLOT_UNIT_RELEASE, 13, 13)        /* VT_COMPLEX_UNIT */ \
    D(SLOT_ASG_STATS, 20, 20)           /* the attack subgroups: FN_ASG_STATS, VT_ASG_LAND's FN_ASG_LAND_STATS */ \
    D(SLOT_TYPE_ASSIGN, 29, 29)         /* EEUnitClass: thiscall(type data, unit), as the game's goal assignment */ \
    D(SLOT_PLAYER_HUMAN, 29, 29)        /* EEPlayer: char thiscall(player) - human (EEHumanPlayer's returns 1; FN_HUMAN_EPOCH's test) */ \
    D(SLOT_UNIT_LIMIT, 31, 31)          /* EEPlayer: int thiscall(player, 0), the unit limit (FN_GAME_LIMIT) */ \
    D(SLOT_MISSILE_LAUNCH, 42, 43)      /* VT_MISSILE */ \
    D(SLOT_PROJ_DOWN, 55, 56)           /* VT_PROJECTILE */ \
    D(SLOT_ASG_GIVE_GOAL, 57, 57)       /* the attack subgroups */ \
    D(SLOT_ASG_UPDATE, 90, 90)          /* the attack subgroups: update(now) */ \
    D(SLOT_ASG_ROUTE, 94, 94)           /* VT_ASG_LAND */ \
    D(SLOT_ASG_ROUTE_DONE, 97, 97)      /* VT_ASG_LAND */ \
    D(SIZE_PAUSE_ACTION, 0xC, 0xC)      /* EEAUserPauseGame: vtable, refcount, byte */ \
    D(SIZE_CHEAT_MSG, 0x18, 0x18)       /* FN_CHEAT_MSG_CTOR */ \
    D(SIZE_CAMERA_ACTION, 0x1C, 0x1C)   /* FN_CAMACT_CTOR */ \
    D(SIZE_NEW_UNIT_MSG, 0x20, 0x20)    /* FN_NEW_UNIT_MSG_CTOR */ \
    D(SIZE_MSG_CHAT, 0x24, 0x24)        /* FN_CHAT_MSG_CTOR */ \
    D(SIZE_SAVE_ACTION, 0x30, 0x30)     /* FN_SAVE_ACTION_CTOR */ \
    D(SIZE_GOAL_MOVE, 0x38, 0x38)       /* FN_GOAL_MOVE_CTOR */ \
    D(SIZE_GOAL_ATTACK, 0x40, 0x44)     /* FN_GOAL_ATTACK_CTOR */ \
    D(SIZE_GOAL_UNLOAD, 0x40, 0x40)     /* FN_GOAL_UNLOAD_CTOR */ \
    D(SIZE_GOAL_BUILD, 0x5C, 0x5C)      /* FN_BUILD_GOAL */ \
    D(SIZE_GOAL_LOAD, 0x68, 0x68)       /* FN_GOAL_LOAD_CTOR */

// lists: L(name, count, (values of build 1), (values of build 2) ...)
#define ADDR_LISTS(L) \
    /* TCP Protocol.dll, relative to its base: the network ports' immediates (UDP 33334, TCP 33335), moved for a test slot */ \
    L(RVA_TCP_PORTS, 8, (0x17E2, 0x196F, 0x19EB, 0x1EB8, 0x1FBD, 0x2148, 0x279C, 0x2F2C), \
        (0x1684, 0x185C, 0x18D8, 0x1DA5, 0x1EAA, 0x2035, 0x2689, 0x2E1C)) \
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
    /* the calls of FN_ASSIGN_TO_SITE (trace.c) */ \
    L(GATHER_SITES, 11, (0x0048A2A6, 0x0048A372, 0x0048A9B8, 0x0048AB53, 0x0048ABF3, 0x0048ACB7, 0x0048ADC6, 0x0048AE7A, \
        0x0048AF60, 0x0048B0A6, 0x0048B261), \
        (0x0048EC21, 0x0048ECED, 0x0048F32C, 0x0048F540, 0x0048F5E2, 0x0048F6A6, 0x0048F7B3, 0x0048F869, 0x0048F94F, 0x0048FA80, 0x0048FC37)) \
    /* the calls of FN_GOAL_UNLOAD_CTOR (landing.c; the last, 0x615AE6, the unload goal's copy) */ \
    L(UNLOAD_SITES, 8, (0x00445AAD, 0x00483E4D, 0x0048CC5D, 0x0048D022, 0x004BF82A, 0x005B3E23, 0x005DC81B, 0x00615AE6), \
        (0x00444CCD, 0x004884B9, 0x00491733, 0x00491AF8, 0x004C86E9, 0x005C4E31, 0x005ED156, 0x006284E2)) \
    /* the calls of FN_FIRE (aim.c) */ \
    L(FIRE_CALLS, 4, (0x00606067, 0x00607D79, 0x0060ABB0, 0x0060B33C), \
        (0x00617838, 0x00619604, 0x0061D174, 0x0061D997)) \
    /* the land unit records' vtables of the attack subgroups: base, hero, priests and prophets */ \
    L(ASGU_VTABLES, 6, (0x00827844, 0x008253C0, 0x00825708, 0x008257F0, 0x00825458, 0x00825540), \
        (0x0083BF4C, 0x0083936C, 0x008396BC, 0x008397A8, 0x00839404, 0x008394F0)) \
    /* the attack subgroups' vtables */ \
    L(ASG_VTABLES, 6, (0x00827620, 0x00826610, 0x00826818, 0x00826F10, 0x008270F8, 0x00827318), \
        (0x0083BD20, 0x0083A6D8, 0x0083A8E0, 0x0083AFEC, 0x0083B1D8, 0x0083B9F0)) \
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
    /* the calls of FN_NATIVE_CHAT (the AI's own lines) */ \
    L(NATIVE_CHAT_SITES, 5, (0x00470040, 0x004715F8, 0x0047166E, 0x0047280E, 0x00472B9D), \
        (0x00472F4C, 0x00474682, 0x00474705, 0x004758D5, 0x00475C65)) \
    /* call AddMovie(path) in PlayIntro 0x538867 */ \
    L(INTRO_CALLS, 3, (0x00538909, 0x0053892A, 0x00538945), \
        (0x00543A57, 0x00543A78, 0x00543A93)) \
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


#define ADDR_ENUM_F(n, r, cc, a, ...) A_##n,
#define ADDR_ENUM_D(n, ...) A_##n,
enum addr_name { ADDRS(ADDR_ENUM_F, ADDR_ENUM_D) ADDR_NUMBERS(ADDR_ENUM_D) A_N };
#define ADDR_TYPE_F(n, r, cc, a, ...) typedef r (cc *n##_fn) a;
#define ADDR_TYPE_D(n, ...)
ADDRS(ADDR_TYPE_F, ADDR_TYPE_D)
#define ADDR_ROW_F(n, r, cc, a, ...) { __VA_ARGS__ },
#define ADDR_ROW_D(n, ...) { __VA_ARGS__ },
static const uint32_t k_addr[A_N][ADDR_BUILDS] = { ADDRS(ADDR_ROW_F, ADDR_ROW_D) ADDR_NUMBERS(ADDR_ROW_D) };
#define ADDR_NAME_F(n, r, cc, a, ...) #n,
#define ADDR_NAME_D(n, ...) #n,
static const char *const k_addr_name[A_N] = { ADDRS(ADDR_NAME_F, ADDR_NAME_D) ADDR_NUMBERS(ADDR_NAME_D) };
#define ADDR_UNPAREN(...) { __VA_ARGS__ }
#define ADDR_LIST_COL(v) ADDR_UNPAREN v,
#define ADDR_LIST(n, k, ...) static const uint32_t k_addr_list_##n[ADDR_BUILDS][k] = { ADDR_LIST_EACH(__VA_ARGS__) };
#define ADDR_LIST_EACH(v1, v2) ADDR_LIST_COL(v1) ADDR_LIST_COL(v2)   // one per build
ADDR_LISTS(ADDR_LIST)

static int g_exe_build = -1;           // the column of the running exe (-1: none known - the mod stays off)
// the running build by its name ("ee", "aoc"): for code that differs by more than an address (AoC's rewrites)
static int build_is(const char *name) { return g_exe_build >= 0 && !lstrcmpA(k_exe_builds[g_exe_build].name, name); }
static uintptr_t g_addr[A_N];
#define GA(n) (g_addr[A_##n])
#define GF(n) ((n##_fn)g_addr[A_##n])
#define GL(n) (k_addr_list_##n[g_exe_build])
static int g_addr_missing;               // names the running build hasn't got (logged at the start: addr_log)

// the running exe's column into g_addr; the names it hasn't got (0) are counted - the caller logs them
static int addr_select(int build) {
    int missing = 0;
    g_exe_build = build;
    for (int i = 0; i < A_N; i++) missing += !(g_addr[i] = k_addr[i][build]);
    image_ranges();
    return missing;
}
static void addr_log(FILE *log) {   // the names the running build hasn't got: what uses them doesn't work there
    if (!log || !g_addr_missing) return;
    fprintf(log, "# addresses: build %d hasn't got %d of %d:", g_exe_build, g_addr_missing, A_N);
    for (int i = 0; i < A_N; i++) if (!g_addr[i]) fprintf(log, " %s", k_addr_name[i]);
    fputc(10, log);
}
