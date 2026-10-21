// THE GAME'S ADDRESSES: every address the mod uses in Empire Earth.exe, by name, with its value in each exe build the
// mod knows - one column per k_exe_builds entry, in that order (0: the build hasn't got it). game_build (game/exe.c)
// picks the column when the DLL loads (addr_select): GA(NAME) is the address, GF(NAME) the function there, typed by
// its row, GL(NAME) a list of addresses (its count: sizeof GL(NAME) / sizeof *GL(NAME)). Another build - Art of
// Conquest, another release, a patched exe - is one more hash and one more value per row. The names: FN_ a function,
// CALL_ a call (patched), SITE_/CODE_ an instruction or operand, G_ data, VT_ a vtable, VTS_ a vtable slot, IAT_ an
// import slot. Tables of their own for one build: editor_layout.h (generated for the 2017 update: apply_editor_layout
// only there). Struct offsets (FIELD(pl, 0x45C ...)) are not here yet: Art of Conquest's objects have layouts of their own.
#define ADDR_BUILDS 1
static const uint32_t k_exe_builds[ADDR_BUILDS] = { 0x4FB6A479 };   // the 2017 update (GOG, Steam, empireearth.eu Vanilla)

// F(name, return type, calling convention, (parameters), value per build): a function; D(name, value per build)
#define ADDRS(F, D) \
    F(FN_TECH_LOOKUP, uint8_t *, __thiscall, (void *, int), 0x004019A1) \
    /* thiscall (tile x, tile y): set target + queue (only if tile valid) */ \
    F(FN_CAMACT_SET, void, __thiscall, (void *, int, int), 0x00401F3E) \
    F(FN_EXPLORED, char, __thiscall, (void *, void *), 0x00404A8A) /* thiscall(site+0x14 object, player) -> bool */ \
    F(FN_SAVE_ACTION_CTOR, void *, __thiscall, (void *, void *, int, struct ustring), 0x00404FA1) \
    D(SITE_SCROLL_TIMER, 0x004050C2)                              /* operand of push 0x19 (timer period in ms) */ \
    F(FN_SCROLL_EXECUTE, void, __thiscall, (void *), 0x004050FC) \
    /* point from int x, y (the task location's point) */ \
    F(FN_POINT_SET, void, __thiscall, (void *, int *), 0x0040D29D) \
    D(FN_ROUTE_COST, 0x004141E8)                                  /* its slot 1: int thiscall(edge*, node*) */ \
    /* the area object at a tile (thiscall on the map areas G_MAP_AREAS) */ \
    F(FN_AREA_AT, uint8_t *, __thiscall, (void *, int *), 0x00425F16) \
    D(FN_REGION, 0x00425F9F)                                      /* thiscall(0x917BC8, int pos[2], 2) -> short region id */ \
    /* where an idle transport lands (thiscall(transport manager, transport)) */ \
    F(FN_SHORE_TO_UNLOAD, int, __thiscall, (void *, void *), 0x0042C49A) \
    F(FN_UNITS_INSIDE, int, __thiscall, (void *), 0x0042E347)     /* units aboard a transport or inside a building */ \
    D(CALL_CP_UPDATE, 0x0042F3CD) \
    F(FN_CP_ADD_RESOURCE, void, __thiscall, (void *, int, int), 0x0042F783) /* EEComputerPlayer::vf18(type, amount) */ \
    /* thiscall(player), called at 0x4311E5, 0x43FA9B, 0x474AF0 */ \
    F(FN_POP_CAP, void, __thiscall, (void *), 0x0042F7A5) \
    /* thiscall(player, on player, 1): calls off its attack groups on that player */ \
    F(FN_CALL_OFF_ATTACK, void, __thiscall, (void *, void *, int), 0x0043033E) \
    D(CALL_TRAIN_CITIZENS, 0x00430740)                            /* a call of FN_TRAIN_CITIZENS */ \
    F(FN_DELETE_FOR_ROOM, int, __thiscall, (void *, void *, int), 0x00430D40) \
    /* EEComputerPlayer::AdjustCost (vf27): a price with the cheat */ \
    F(FN_ADJUST_COST, int, __thiscall, (void *, int), 0x00430FBD) \
    D(CALL_POP_CAP_1, 0x004311E5)                                 /* the callers of FN_POP_CAP */ \
    D(CALL_EPOCH_UPGRADES, 0x0043D365)                            /* the call of FN_EPOCH_UPGRADES */ \
    /* thiscall(player, now): the next attack time (+0xED8) */ \
    F(FN_SCHEDULE_ATTACK, void, __thiscall, (void *, uint32_t), 0x0043DD2F) \
    /* EEComputerPlayer::vf45(cost object, ?, ?): pays a unit/building/tech */ \
    F(FN_CP_PAY, void, __thiscall, (void *, void *, int, int), 0x0043F53D) \
    D(FN_CP_UPDATE, 0x0043F91A) \
    D(CALL_POP_CAP_2, 0x0043FA9B) \
    F(FN_GIVE_GOAL, void, __thiscall, (void *, void *, int), 0x004412D4) /* thiscall(unit, goal, slot) */ \
    D(FN_ADD_MOVIE, 0x00441432)                                   /* __thiscall, ret 4 */ \
    F(FN_TASK_CTOR, void, __thiscall, (void *), 0x00441FB3)       /* an attack task */ \
    F(FN_TASK_SET_TARGET, void, __thiscall, (void *, void *), 0x00442246) \
    F(FN_TASK_DTOR, void, __thiscall, (void *), 0x00442283) \
    F(FN_ASG_AIR_GIVE_GOAL, char, __thiscall, (void *, void *, void *, int, int), 0x00442AD8) \
    F(FN_ASG_AIR_UPDATE, void, __thiscall, (void *, int), 0x00442DBA) /* EECPASGAir slot 90: update(now) */ \
    F(FN_ASG_LAND_STATS, void, __thiscall, (void *), 0x004472A9) \
    F(FN_ASG_UPDATE, void, __thiscall, (void *, int), 0x004491DD) /* EECPASGLand slot 90: update(now) */ \
    D(RET_ASG_LAND_HANDOFF_GIVE, 0x0044AF88) \
    D(FN_ASG_ROUTE, 0x0044BC57)                                   /* its slot 94: void thiscall(int start[2], int dest[2]) */ \
    D(FN_ASG_ROUTE_DONE, 0x0044BDBF)                              /* its slot 97: route arrived, waypoints in the deque at +0x60 */ \
    F(FN_ASG_SEA_UPDATE, void, __thiscall, (void *, int), 0x0044ED11) /* EECPASGSea slot 90: update(now) */ \
    /* EECPASGSeaTransport slot 90: update(now) */ \
    F(FN_ASG_SEA_TRANSPORT_UPDATE, void, __thiscall, (void *, int), 0x0044FB92) \
    D(CALL_GROUP_STAT, 0x00450546) \
    F(FN_GROUP_STAT, void, __thiscall, (void *, void *), 0x00450554) \
    F(FN_UNIT_TO_GROUP, char, __thiscall, (void *, void *), 0x00450E07) \
    /* thiscall(attack manager, task, strategy, unit ids) -> the attack group */ \
    F(FN_ATTACK_GROUP_NEW, uint8_t *, __thiscall, (void *, void *, void *, void *), 0x0045106C) \
    F(FN_ASG_STATS, void, __thiscall, (void *), 0x0045AD2F) \
    F(FN_ASG_GIVE_GOAL, char, __thiscall, (void *, void *, void *, int, int), 0x0045CBEA) \
    D(FN_ASGU_SCORE, 0x00461854)                                  /* float thiscall(info *) */ \
    F(FN_BM_QUEUED, char, __thiscall, (void *, int), 0x00465640)  /* thiscall(build manager, type): queued */ \
    F(FN_PLACE, char, __thiscall, (void *, int, int, int), 0x0046580A) \
    F(FN_ENEMY_NEAR, char, __thiscall, (void *, int, int), 0x004660B9) /* thiscall(build manager, x, y) -> bool */ \
    /* thiscall(build manager, type, 1, 0): one building requested */ \
    F(FN_BM_REQUEST, void, __thiscall, (void *, int, int, int), 0x0046802D) \
    D(CALL_PLACE, 0x00468629) \
    F(FN_BQ_ADD, void, __thiscall, (void *, int *), 0x00468C37)   /* an entry into a build manager epoch queue */ \
    F(FN_FISH_TYPE, int, __thiscall, (void *), 0x0046D209) \
    D(CALL_REPAIR_GOAL, 0x0046DA88) \
    F(FN_SET_EPOCH_TIME, void, __thiscall, (void *, uint32_t, char), 0x0046DE8C) /* SetNextEpochAdvanceTime */ \
    D(CALL_RM_UPDATE, 0x0046E3C6)                                 /* call EECPResourceManager::Update(now)              (0x48D263) */ \
    F(FN_REVENGE, void, __thiscall, (void *, void *), 0x0046FFB7) \
    /* thiscall(cm, task, strategy, unit ids) */ \
    F(FN_ATTACK_PLAN_OK, char, __thiscall, (void *, void *, void *, void *), 0x0047067A) \
    F(FN_CM_UNIT_FREE, char, __thiscall, (void *, void *), 0x004707D8) /* thiscall(cm, unit state): in no group */ \
    F(FN_STRATEGY_CTOR, void, __thiscall, (void *), 0x00473BC4) \
    F(FN_STRATEGY_DTOR, void, __thiscall, (void *), 0x00473C99) \
    F(FN_STRATEGY_INIT, void, __thiscall, (void *), 0x00473CA7) \
    D(CALL_POP_CAP_3, 0x00474AF0) \
    D(CALL_NV_MAINT_DELETE, 0x00477B17) \
    F(FN_LOCATION_CTOR, void, __thiscall, (void *), 0x0047C4E8) \
    F(FN_TASK_SET_LOCATION, void, __thiscall, (void *, void *), 0x0047C8B7) \
    D(CALL_UNIT_TO_GROUP, 0x004824BF) \
    D(CALL_SHORE_TO_UNLOAD, 0x00483E24)                           /* the idle transports' call of FN_SHORE_TO_UNLOAD */ \
    F(FN_NATIVE_CHAT, char, __thiscall, (void *, int, int), 0x00484EB3) \
    F(FN_RS_SCHEDULE, void, __thiscall, (void *, int), 0x00485F57) \
    /* thiscall(research manager cm+0x40, count): the AI's request for an epoch's required techs */ \
    F(FN_REQUEST_TECHS, void, __thiscall, (void *, int), 0x004860D6) \
    /* thiscall(zone, type) -> first site of that type with amount > 0 */ \
    F(FN_ZONE_SITE, uint8_t *, __thiscall, (void *, int), 0x004886F2) \
    D(LEFTOVER_SITE_END, 0x0048A2B9)                              /* call sites below this are in the leftover pass 0x48A1AE */ \
    D(FN_ASSIGN_CITIZENS, 0x0048A554) \
    D(CALL_FARMS, 0x0048AC54)                                     /* the call of FN_FARMS */ \
    D(CALL_HUNT_1, 0x0048ACE4)                                    /* the calls of FN_HUNT */ \
    D(CALL_HUNT_2, 0x0048AFC4) \
    D(CALL_UNBUILT_FARMS, 0x0048B0FD) \
    D(FN_HUNT, 0x0048BA3B)                                        /* (count, flag, out list) */ \
    D(FN_FIND_FOOD, 0x0048BBA8)                                   /* (char needDropsite, char nearBase, unit) -> site */ \
    D(FN_UNBUILT_FARMS, 0x0048BF4F) \
    D(FN_FARMS, 0x0048BFD0)                                       /* (count, out list) */ \
    D(FN_FIND_WOOD, 0x0048C060)                                   /* (flag, unit) -> target */ \
    D(FN_FIND_MINE, 0x0048C204)                                   /* (type, unit, reverse) -> site */ \
    F(FN_ASSIGN_TO_SITE, void, __thiscall, (void *, void *, void *, int), 0x0048C81C) /* (unit, target, 0) */ \
    D(CALL_UNLOAD_RM, 0x0048CC5D)                                 /* an unload call (the resource manager's ferry) */ \
    D(CALL_UNLOAD_RETRY_GIVE, 0x0048D03C) \
    D(FN_COMPUTE_TARGETS, 0x0048D0A2) \
    D(FN_RM_UPDATE, 0x0048D263) \
    D(CALL_COMPUTE_TARGETS, 0x0048D347)                           /* call EECPResourceManager::ComputeGathererTargets (0x48D0A2) */ \
    D(CALL_ASSIGN_CITIZENS, 0x0048D36A)                           /* call EECPResourceManager::AssignCitizens        (0x48A554) */ \
    F(FN_TABLE_VALUE, unsigned, __thiscall, (void *, int), 0x0048E817) \
    D(FN_UM_PENDING, 0x0048E9D8)                                  /* thiscall(unit manager, type) */ \
    D(FN_UM_IN_PRODUCTION, 0x0048EA2E)                            /* thiscall(unit manager, type) */ \
    F(FN_UM_CITIZENS_WANTED, int, __thiscall, (void *), 0x0048EDDB) /* citizen share x population cap */ \
    F(FN_TRAIN_CITIZENS, uintptr_t, __thiscall, (void *, uintptr_t), 0x0048EE84) \
    /* citizens of both sexes and the epoch's fishing boat type */ \
    F(FN_UM_CITIZENS, int, __thiscall, (void *), 0x0048F819) \
    D(CALL_FISH_TYPE_1, 0x0048F823)                               /* the calls of FN_FISH_TYPE */ \
    /* thiscall(unit manager, type): its units and buildings of that type */ \
    F(FN_UM_TYPE_LIST, uint8_t *, __thiscall, (void *, int), 0x0048F8AF) \
    D(CALL_FISH_TYPE_2, 0x0048FB16) \
    F(FN_TRAIN_UNITS, uintptr_t, __thiscall, (void *, int *, uintptr_t), 0x0049014F) \
    D(CALL_CITIZENS_WANTED, 0x00490BC2)                           /* a call of FN_UM_CITIZENS_WANTED */ \
    /* thiscall(player +0x874, type): the per-player stats record */ \
    F(FN_TYPE_STATS, uint8_t **, __thiscall, (void *, int), 0x00492969) \
    /* thiscall(wall manager, int from[2], int to[2], type, char) */ \
    F(FN_BUILD_WALL, void, __thiscall, (void *, int *, int *, int, char), 0x00497C3F) \
    F(FN_VECTOR_SIZE, unsigned, __thiscall, (void *), 0x004A670A) \
    F(FN_INGAME_CHAT, void, __thiscall, (void *), 0x004B78C7) \
    F(FN_PAY_TRIBUTE, void, __thiscall, (void *), 0x004BA6BE) \
    F(FN_RESIGN, void, __thiscall, (void *), 0x004BB906) \
    /* EEMSelectUnits: thiscall(msg, rect, type, 0x4E) */ \
    F(FN_SELECT_MSG_INIT, void, __thiscall, (void *, void *, int, int), 0x004BD072) \
    D(CALL_SEL_PACK, 0x004BD133) \
    F(FN_SEL_PACK, unsigned, __cdecl, (void *, uint32_t, uint8_t *), 0x004BD15A) \
    D(START_EXEC, 0x004BEE57) \
    D(CALL_GAME_START, 0x004CEC14) \
    F(FN_DAMAGE_POINTS, int, __thiscall, (void *, int, unsigned), 0x004D291F) \
    /* thiscall(G_BONUS_TABLE, weapon kind, target type data, attacker, target player) */ \
    F(FN_ATTACK_BONUS, float, __thiscall, (void *, int, void *, int, int), 0x004D5F2C) \
    D(CALL_DAMAGE_POINTS, 0x004F80A4) \
    F(FN_DRAW_REC_CTOR, void, __thiscall, (void *), 0x00506442)   /* a draw list record */ \
    D(FN_MOUSE_CLAMP, 0x00510DAD) \
    D(FN_MOUSE_UPDATE, 0x00510E5D) \
    D(FN_ATTACK_ALERT, 0x00519BC1) \
    /* thiscall on G_CAMERA (float dx, float dy) */ \
    F(FN_CAMERA_MOVE, void, __thiscall, (void *, float, float), 0x0051E2E6) \
    /* thiscall([G_CHEAT_PARSER], UWideString *, int *code, int *arg) */ \
    F(FN_CHEAT_PARSE, void, __thiscall, (void *, void *, int *, int *), 0x00520A17) \
    D(FN_LOAD_SAVED, 0x00521F94) \
    D(FN_PARSE_CMDLINE, 0x00522A36) \
    D(CALL_LOAD_SAVED, 0x00522BF8)                                /* call LoadSavedGameFromCmdline(UString *) 0x521F94 */ \
    D(FN_GAME_START, 0x0052599B) \
    D(CALL_PLAYER_COLOR, 0x005262E4) \
    F(FN_COLOR_RGB, void *, __cdecl, (unsigned), 0x00526B85) \
    F(FN_SETTINGS_SAVE, void, __thiscall, (void *), 0x0052AA81)   /* every setting into the registry */ \
    F(FN_SET_STANCE, void, __thiscall, (void *, int, int, int), 0x0052D5D7) \
    /* thiscall(G_GAME_STATE, char won, index) */ \
    F(FN_GAME_PLAYER_OUT, void, __thiscall, (void *, int, int), 0x0052E2B7) \
    F(FN_SITE_GATHERERS, unsigned, __thiscall, (void *, void *), 0x0052E5F7) \
    F(FN_GAME_PLAYER, void *, __thiscall, (void *, int), 0x0052E6EF) /* thiscall(G_GAME_STATE, index) */ \
    D(CODE_SPLASH_START_JNE, 0x00534400)                          /* the loading splash's start (jne) */ \
    D(CODE_SPLASH_STOP_JNE, 0x0053443D)                           /* the loading splash's stop (jne) */ \
    D(CALL_PARSE_CMDLINE, 0x00535EF2)                             /* call ParseCommandLine(const char *) 0x522A36 */ \
    F(FN_SAVE_GAME, int, __cdecl, (void *, int, int, struct ustring), 0x00540ACA) \
    F(FN_PHYS_STEP, void, __thiscall, (void *), 0x00546C63) \
    D(CALL_MISSILE_AIM, 0x0054E850) \
    D(SITE_SIM_RECEIVE, 0x00553348) \
    D(SITE_SIM_WAIT, 0x005533C5) \
    /* thiscall(player, type, 0x20, 0): can train or build one now */ \
    F(FN_CAN_BUILD, char, __thiscall, (void *, int, int, int), 0x00555710) \
    F(FN_POPULATION, int, __thiscall, (void *, int, int), 0x00556E11) /* thiscall(player, 1, 0) */ \
    /* EEPlayer::vf31 thiscall(player, 0): the game's unit limit for this player */ \
    F(FN_GAME_LIMIT, int, __thiscall, (void *, int), 0x00556EBC) \
    F(FN_SELECT_UNITS, void, __thiscall, (void *, void *), 0x0055B3FB) /* EEMSelectUnits carried out on the player */ \
    D(FN_SET_DIPLOMACY, 0x0055B8E1) \
    F(FN_PLAYER_RESIGN, void, __thiscall, (void *, int), 0x0055BCEA) /* thiscall(player, 2) as EEMResign */ \
    F(FN_STAT_ADD, void, __thiscall, (void *, int, int), 0x0055DBCF) \
    F(FN_CAMACT_CTOR, void *, __thiscall, (void *), 0x00560179)   /* EEACenterCamera ctor (0x1C bytes) */ \
    F(FN_EDITOR_FORM, void *, __thiscall, (void *, void *), 0x0058D3E1) \
    F(SM_RUN_EFFECT, void, __thiscall, (void *, int), 0x005AD29B) \
    F(FN_EPOCH_UPGRADES, void, __thiscall, (void *, int), 0x005BDDBD) /* tech tree node, category */ \
    F(FN_TECH_GRANT, int, __thiscall, (void *), 0x005BF335) \
    F(FN_CHEAT_MSG_CTOR, void *, __thiscall, (void *, int, int), 0x005C29FB) /* new 0x18; (code, arg) */ \
    F(FN_CHAT_MSG_CTOR, void *, __thiscall, (void *, void *), 0x005C2A2F) /* new 0x24; (UWideString *) */ \
    F(FN_RESIGN_SEND, void, __cdecl, (void), 0x005D05CC) \
    D(EXIT_CONFIRM_AT, 0x005D0B2C) \
    F(FN_EXIT_CONFIRM, void, __cdecl, (void), 0x005D0B4F) \
    F(FN_UNIT_RELEASE, void, __thiscall, (void *), 0x005ED71E) \
    D(CALL_GOAL_FBA_CHECK, 0x005ED9BF)                            /* in 0x5ED24B: FN_GOAL_FBA_CHECK on a dynamic_cast, unchecked */ \
    /* thiscall(unit, 0x1F51) - as the assignment does after the order */ \
    F(FN_UNIT_SAY, void, __thiscall, (void *, int), 0x005EFCF0) \
    D(FN_SET_GOAL, 0x005F05BD)                                    /* thiscall(unit, goal, 0, 0) */ \
    D(AT_GOAL_SET_READ, 0x005F0634) \
    D(CALL_GOALSET_ASK, 0x005F0FFA) \
    D(CALL_GOALSET_AGAIN, 0x005F1029) \
    /* EEUCMissile vf42, thiscall(class, missile, aim) - ret 8, the aim unused */ \
    F(FN_MISSILE_LAUNCH, int, __thiscall, (void *, uint8_t *, float *), 0x00606109) \
    F(FN_PROJ_DOWN, char, __thiscall, (void *, uint8_t *), 0x006078F9) \
    F(FN_GOAL_ATTACK_CTOR, void *, __thiscall, (void *, void *, void *, int), 0x0060AD45) /* EEUGAttack: new 0x40 */ \
    /* EEUGBuildRepair constructor: thiscall(goal, player, building, 1) */ \
    F(FN_BUILD_GOAL, void *, __thiscall, (void *, void *, void *, int), 0x0060BAD0) \
    /* thiscall(goal 0xFBA object, unit) (0x5ED24B) */ \
    F(FN_GOAL_FBA_CHECK, char, __thiscall, (void *, void *), 0x0060FE7E) \
    /* EEUGLoadTransport: new 0x68 */ \
    F(FN_GOAL_LOAD_CTOR, void *, __thiscall, (void *, void *, void *, int), 0x0061487F) \
    /* new 0x38 */ \
    F(FN_GOAL_MOVE_CTOR, void *, __thiscall, (void *, float *, int, char, char, float, char, char), 0x00614CE6) \
    /* EEUGUnloadTransport: new 0x40 */ \
    F(FN_GOAL_UNLOAD_CTOR, void *, __thiscall, (void *, void *, int, int, int), 0x00615A1F) \
    D(CALL_KILL_STAT, 0x00617780) \
    D(CALL_ATTACK_ALERT, 0x006177CF) \
    F(FN_GOALSET_ASK, int, __thiscall, (void *, void *, int *), 0x00619FF5) \
    F(FN_VECTOR_FREE, void, __thiscall, (void *), 0x0061ABEC)     /* a vector's dtor (the game's delete) */ \
    F(FN_FIRE, void, __thiscall, (void *, uint8_t *), 0x0061E11C) /* thiscall(attacker class, attacker) */ \
    D(CALL_AIM, 0x0061E1D7) \
    D(CALL_NODE_DEL_2, 0x00628B7F) \
    D(CALL_NODE_DEL_1, 0x00628BE7) \
    F(FN_UNIT_UNLINK, void, __thiscall, (void *), 0x00628E29) \
    D(FN_BALLISTIC, 0x0062CBB3) \
    F(FN_UNIT_DISTANCE, float, __cdecl, (void *, void *), 0x0062D5A4) /* cdecl(unit, other) */ \
    /* cdecl char(player, type, x, y, w, h, 0) */ \
    F(FN_CAN_PLACE, char, __cdecl, (void *, int, int, int, int, int, int), 0x0062E940) \
    /* cdecl(player, type, x, y): a building placed (a foundation) */ \
    F(FN_PLACE_BUILDING, void *, __cdecl, (void *, int, int, int), 0x0062EF8A) \
    /* cdecl(unit, player index, new type data) */ \
    F(FN_UNIT_CONVERT, int, __cdecl, (void *, int, uint32_t), 0x0062F192) \
    D(CALL_RANGE_SORT, 0x0063052E) \
    D(FN_AIM, 0x00633413) \
    F(FN_RANGE_SORT, void, __cdecl, (void **, void **, void *), 0x0063A355) \
    /* EEMHackANewUnit: thiscall(int *type, float xy[2], char player) */ \
    F(FN_NEW_UNIT_MSG_CTOR, void, __thiscall, (void *, int *, float *, char), 0x00650B3C) \
    D(SITE_EDITOR_FORM, 0x0065BA13) \
    D(FN_SET_RESOLUTION, 0x0065F314) \
    D(SITE_EDITOR_RES, 0x0065FD6D) \
    D(SITE_EDITOR_MENU, 0x006601F1) \
    D(FN_QUEUE_ACTION, 0x0066192F) \
    /* thiscall(ui [G_UI], colour, name, line, ms, 0): a chat or system line */ \
    F(FN_UI_LINE, void, __thiscall, (void *, void *, void *, void *, int, int), 0x00661E91) \
    D(SITE_CLOSE_WINDOW_JE, 0x00662666)                           /* je over the CloseWindow call on WM_ACTIVATEAPP(0) */ \
    D(FN_OP_DELETE, 0x006836F0) \
    F(FN_NEW, void *, __cdecl, (size_t), 0x00683718)              /* the game's operator new */ \
    D(IAT_RESOURCE_STRING, 0x0082319C)                            /* the game's string resource (a language entry) */ \
    D(IAT_WSTRING_CTOR, 0x008231A4)                               /* UWideString::UWideString() */ \
    D(IAT_RELEASE, 0x008231AC)                                    /* imported refcount release, thiscall on the action */ \
    D(IAT_USTRING_CTOR, 0x008231E4)                               /* UString::UString() */ \
    D(IAT_WSTRING_ASSIGN, 0x00823224)                             /* UWideString::operator=(const char *) */ \
    D(IAT_USTRING_ASSIGN, 0x00823228)                             /* UString::operator=(const char *) */ \
    D(IAT_WSTRING_CTOR_C, 0x008232C4)                             /* UWideString::UWideString(const char *) */ \
    D(IAT_WSTRING_FORMAT, 0x008232C8)                             /* UWideString::Format(const char *, ...) */ \
    D(IAT_SND_PLAY, 0x008232D0)                                   /* SESoundManager::Play(id, vol, pan, pitch, bool queue, char *) -> id or -1 */ \
    D(IAT_COS, 0x00823304)                                        /* cos (the game's) */ \
    D(IAT_EDIT_GET_TEXT, 0x00823328)                              /* UIEditBox::GetText */ \
    D(IAT_SND_PLAYREF, 0x008233EC)                                /* SESoundManager::PlayAndReturnReferencedNode(same) -> node or NULL */ \
    D(IMP_IS_MOUSE, 0x008235CC)                                   /* import: &ISMouse* in Low-Level Engine.dll */ \
    D(IAT_WSTRING_RESERVE, 0x008235D0) \
    D(IAT_TS_WAIT, 0x00823688) \
    D(IAT_TS_RECEIVE, 0x008236B0) \
    D(IAT_SND_SETMAXCH, 0x00823AD4) \
    D(IAT_WSTRING_DTOR, 0x00823AEC)                               /* UWideString::~UWideString() */ \
    D(IMP_IS_KEYBOARD, 0x00823AF8)                                /* import: &ISKeyboard* in Low-Level Engine.dll */ \
    D(IAT_UIM_FORM, 0x00823D60)                                   /* the UI manager's current form */ \
    D(IAT_USTRING_DTOR, 0x008245BC)                               /* UString::~UString() */ \
    D(IAT_SRAND, 0x00824644) \
    D(IAT_RAND, 0x00824648)                                       /* the game's rand */ \
    D(VT_SCROLL, 0x00825098) \
    D(VT_ROUTE_PLANNER, 0x00825CC4)                               /* EEAIGPGModifierLandAttackSubGroupRoutePlanner */ \
    D(VT_COMPUTER_PLAYER, 0x008264C0) \
    D(VT_ASG_AIR, 0x00826610) \
    D(VT_ASG_LAND, 0x00826CA8)                                    /* EECPASGLand */ \
    D(VT_ASG_SEA, 0x00826F10) \
    D(VT_ASG_SEA_TRANSPORT, 0x008270F8) \
    D(START_VTBL_EXEC, 0x008285D8) \
    D(VT_RESIGN, 0x00828820) \
    D(VT_PAY_TRIBUTE, 0x008289C0) \
    D(VT_INGAME_CHAT, 0x0082936C) \
    D(VTBL_PHYS_STEP, 0x0082CB7C)                                 /* EEPhysics vtable slot 8: one simulation step */ \
    D(G_FLOAT_1_600, 0x0082CBCC)                                  /* the float 1/600 (the minimap's y scale operand) */ \
    D(VTS_TECH_RESEARCHED, 0x00830874)                            /* EETechTreeNode::vf6's vtable slot */ \
    D(VT_PAUSE_ACTION, 0x00830E94)                                /* EEAUserPauseGame vtable (0xC bytes: vtable, refcount, byte) */ \
    D(VT_COMPLEX_UNIT, 0x00831494) \
    D(VT_GOAL_ATTACK, 0x00831968) \
    D(VT_GOAL_BUILD, 0x008319F0) \
    /* EEUGLoadTransport: the holder's goal and each unit's part (0x614833), the holder at +0x50 */ \
    D(VT_GOAL_LOAD, 0x00831DC0) \
    D(VT_GOAL_LOCATION, 0x00831E64) \
    D(VT_GOAL_UNLOAD, 0x00831F5C)                                 /* EEUGUnloadTransport (new 0x40, ctor 0x615A1F(transport, x, y, flag)) */ \
    D(VT_MISSILE, 0x00832E90) \
    D(VT_PROJECTILE, 0x00833060)                                  /* EEUCProjectile */ \
    D(G_FLOAT_1_800, 0x008358A8)                                  /* the float 1/800 (the minimap's x scale operand) */ \
    /* the scenario editor's file buttons: 6 of {id, x, y, u0, v0, u1, v1, tooltip} */ \
    D(G_EDITOR_FILE_BUTTONS, 0x008AE0A0) \
    D(G_COLORS_BEGIN, 0x008CFA74)                                 /* the player colour table (vector begin, end) */ \
    D(G_COLORS_END, 0x008CFA78) \
    D(G_HIT_TABLE_A, 0x008D2008)                                  /* the hit table's variants by game mode */ \
    D(G_HIT_TABLE, 0x008D200C)                                    /* [target category][weapon kind]: 0 = it cannot hit */ \
    D(G_HIT_TABLE_B, 0x008D2010) \
    D(G_BEHAVIOUR_TABLE, 0x008D2044)                              /* the unit behaviours (stances) */ \
    D(G_DAMAGE_TABLE, 0x008D2050)                                 /* [attacker class + target class x 6] */ \
    D(G_SELECT_RECT, 0x008D2778)                                  /* a default selection rectangle */ \
    D(DRAW_OLD, 0x008D2D30) \
    D(G_DRAW_N, 0x00912538)                                       /* draw list counts: objects, small objects */ \
    D(G_DRAW_SMALL_N, 0x0091253C) \
    D(G_TABLE_VALUES, 0x00917BB0)                                 /* FN_TABLE_VALUE's table (0x16: the population limit) */ \
    D(G_GAME_DATA, 0x00917BB8)                                    /* game data loaded (unit limit +0x2EC, chat time +0x2CC) */ \
    D(G_MAP_AREAS, 0x00917BC8)                                    /* the map's areas (regions, continents) */ \
    D(G_BONUS_TABLE, 0x00917DE0)                                  /* FN_ATTACK_BONUS's object */ \
    D(G_GAME_STATE, 0x00918300)                                   /* object; vtable slot 11 called after a speed change */ \
    D(G_PLAYERS, 0x00918374) \
    D(G_MAP, 0x009183BC)                                          /* ptr; +0x1816FC = map size in tiles */ \
    D(G_GAME_SPEED, 0x009183C0)                                   /* float; set like EEMGameSpeed::Apply (0x4B6BD4) */ \
    D(G_PLAYERS_N, 0x009183CC)                                    /* the number of players */ \
    D(G_POP_LIMIT, 0x009183D4)                                    /* the population limit game setting */ \
    D(G_END_EPOCH, 0x009187F4)                                    /* the game's end epoch */ \
    D(G_GAME_MODE, 0x00918808) \
    D(G_SCENARIO_FLAG, 0x00918819)                                /* byte: game is a scenario/campaign (settings 0x918410 + 0x409) */ \
    D(G_GAME_TYPE, 0x00918BF4)                                    /* 0 random map, 1 scenario, 2 saved game, 3/4 other */ \
    D(G_DIFFICULTY, 0x00918C14)                                   /* the AIs' difficulty (the setup's) */ \
    D(G_CAMERA, 0x00918CD0)                                       /* EECameraControl; +0x78/+0x7C/+0x80 look-at x/y/z (tiles), +0xE0 zoom */ \
    D(G_SCROLL_SPEED, 0x00918DB0)                                 /* float */ \
    D(G_SM_MGR, 0x00918E24)                                       /* the scenario manager */ \
    D(G_LOCAL_PLAYER, 0x00918E84)                                 /* int: index of the player on this PC (the human in single player) */ \
    D(G_CHEATS_ON, 0x00918E9A) \
    D(G_TEAMS_LOCKED, 0x00918E9F) \
    D(G_PAUSED, 0x00918EA2)                                       /* byte, toggled by EEAUserPauseGame::Execute (0x4064F1) */ \
    D(G_SETTINGS, 0x00918EA8)                                     /* the game settings object */ \
    D(G_PLAYER_NAME, 0x00919308)                                  /* the player name (UWideString) */ \
    D(G_SCROLL_SETTING, 0x009193A8)                               /* float, settings slider x 0.01 */ \
    D(G_SOUND_PERCENT, 0x009193B8) \
    D(G_GAME_RES_H, 0x009193F8) \
    D(G_GAME_RES_W, 0x009193FC) \
    D(G_SCREEN, 0x00919428)                                       /* the current screen index */ \
    D(G_FULLSCREEN, 0x0091945A) \
    D(G_RESOURCES, 0x00919770)                                    /* the string resources */ \
    D(G_WORLD_CLOCK, 0x00919774)                                  /* ptr to EEPhysics; +0xD0 = game time in ms */ \
    D(G_WORLD, 0x0091977C)                                        /* action queue owner: QueueAction 0x66192F (thiscall, takes action ptr) */ \
    D(G_RASTERIZER, 0x00919794) \
    D(G_UI_MANAGER, 0x009197A0) \
    D(G_GAME_WINDOW, 0x009197A4)                                  /* GEWindow* (+0 HWND) */ \
    D(G_MOUSE, 0x009197A8)                                        /* RTSMouse */ \
    D(G_SOUND_MANAGER, 0x00919848) \
    /* EEClient/EEServer (lockstep server, slot 6 takes a message); UI checks [+0x10] != 0 and vtable+0x10 before */ \
    /* pausing */ \
    D(G_SERVER, 0x0091984C) \
    D(G_CHEAT_PARSER, 0x00919858) \
    D(G_INPUT_MSGS_DONE, 0x00919B68)                              /* the user-input messages processed so far */ \
    D(G_EFFECTS, 0x00919E70)                                      /* tech effects, 0x44 each */ \
    D(G_EPOCH_TECHS, 0x0093B1B8)                                  /* per epoch 0x10: the tech lists */ \
    D(G_TYPES, 0x009418A0) \


// lists: L(name, count, (values of build 1), (values of build 2) ...)
#define ADDR_LISTS(L) \
    /* the calls of FN_TRAIN_UNITS (econ.c: the game's unit orders) */ \
    L(TRAIN_SITES, 5, (0x0046CCE6, 0x00490CAC, 0x00490E12, 0x00490F1F, 0x00490FA9)) \
    /* the calls of FN_FIND_MINE */ \
    L(MINE_SITES, 6, (0x0048A247, 0x0048A25F, 0x0048A277, 0x0048A952, 0x0048A9F4, 0x0048AA2B)) \
    /* the calls of FN_FIND_FOOD */ \
    L(FOOD_SITES, 6, (0x0048AB8D, 0x0048AC2B, 0x0048AE14, 0x0048AEB4, 0x0048AEFA, 0x0048AF99)) \
    /* the calls of FN_FIND_WOOD */ \
    L(WOOD_SITES, 2, (0x0048A28E, 0x0048AAF6)) \
    /* the calls of FN_ASSIGN_TO_SITE (trace.c) */ \
    L(GATHER_SITES, 11, (0x0048A2A6, 0x0048A372, 0x0048A9B8, 0x0048AB53, 0x0048ABF3, 0x0048ACB7, 0x0048ADC6, 0x0048AE7A, \
        0x0048AF60, 0x0048B0A6, 0x0048B261)) \
    /* the calls of FN_GOAL_UNLOAD_CTOR (landing.c; the last, 0x615AE6, the unload goal's copy) */ \
    L(UNLOAD_SITES, 8, (0x00445AAD, 0x00483E4D, 0x0048CC5D, 0x0048D022, 0x004BF82A, 0x005B3E23, 0x005DC81B, 0x00615AE6)) \
    /* the calls of FN_FIRE (aim.c) */ \
    L(FIRE_CALLS, 4, (0x00606067, 0x00607D79, 0x0060ABB0, 0x0060B33C)) \
    /* the land unit records' vtables of the attack subgroups: base, hero, priests and prophets */ \
    L(ASGU_VTABLES, 6, (0x00827844, 0x008253C0, 0x00825708, 0x008257F0, 0x00825458, 0x00825540)) \
    /* the attack subgroups' vtables */ \
    L(ASG_VTABLES, 6, (0x00827620, 0x00826610, 0x00826818, 0x00826F10, 0x008270F8, 0x00827318)) \
    /* the minimap's operands of G_FLOAT_1_800 */ \
    L(MINIMAP_FX_OPS, 3, (0x0065E01D, 0x0065E8A3, 0x0065EB3E)) \
    /* the minimap's operands of G_FLOAT_1_600 */ \
    L(MINIMAP_FY_OPS, 3, (0x0065E032, 0x0065E8CE, 0x0065EB68)) \
    /* the other calls of FN_SET_RESOLUTION (display.c) */ \
    L(OTHER_RESOLUTION_SITES, 7, (0x0065F30A, 0x0065F61B, 0x0065F945, 0x0065FAD4, 0x00660CD3, 0x00661329, 0x00661363)) \
    /* the calls of FN_MOUSE_UPDATE */ \
    L(MOUSE_UPDATE_SITES, 4, (0x004B8ABB, 0x0050FEC8, 0x005111CF, 0x006618B8)) \
    /* the calls of FN_NATIVE_CHAT (the AI's own lines) */ \
    L(NATIVE_CHAT_SITES, 5, (0x00470040, 0x004715F8, 0x0047166E, 0x0047280E, 0x00472B9D)) \
    /* call AddMovie(path) in PlayIntro 0x538867 */ \
    L(INTRO_CALLS, 3, (0x00538909, 0x0053892A, 0x00538945)) \
    /* the single player screens' vtables (playername.c) */ \
    L(SP_VTABLES, 6, (0x00835740, 0x00835754, 0x00835768, 0x00835560, 0x0083577C, 0x00835790)) \
    /* their slot 1 */ \
    L(SP_FNS, 6, (0x005D80D5, 0x005D812B, 0x00628E89, 0x005D8156, 0x005D88C0, 0x005D817A)) \
    /* the draw list's operands to move (drawlist.c, in its table's order) */ \
    L(DRAW_SITES, 9, (0x0050099C, 0x00504158, 0x005007B2, 0x005007CC, 0x00503C69, 0x00503ED0, 0x00503FCC, 0x005043B5, \
        0x0050DC5A)) \


#define ADDR_ENUM_F(n, r, cc, a, ...) A_##n,
#define ADDR_ENUM_D(n, ...) A_##n,
enum addr_name { ADDRS(ADDR_ENUM_F, ADDR_ENUM_D) A_N };
#define ADDR_TYPE_F(n, r, cc, a, ...) typedef r (cc *n##_fn) a;
#define ADDR_TYPE_D(n, ...)
ADDRS(ADDR_TYPE_F, ADDR_TYPE_D)
#define ADDR_ROW_F(n, r, cc, a, ...) { __VA_ARGS__ },
#define ADDR_ROW_D(n, ...) { __VA_ARGS__ },
static const uint32_t k_addr[A_N][ADDR_BUILDS] = { ADDRS(ADDR_ROW_F, ADDR_ROW_D) };
#define ADDR_NAME_F(n, r, cc, a, ...) #n,
#define ADDR_NAME_D(n, ...) #n,
static const char *const k_addr_name[A_N] = { ADDRS(ADDR_NAME_F, ADDR_NAME_D) };
#define ADDR_UNPAREN(...) { __VA_ARGS__ }
#define ADDR_LIST_COL(v) ADDR_UNPAREN v,
#define ADDR_LIST(n, k, ...) static const uint32_t k_addr_list_##n[ADDR_BUILDS][k] = { ADDR_LIST_EACH(__VA_ARGS__) };
#define ADDR_LIST_EACH(v) ADDR_LIST_COL(v)   // one build; with two: ADDR_LIST_COL(v1) ADDR_LIST_COL(v2)
ADDR_LISTS(ADDR_LIST)

static int g_exe_build = -1;           // the column of the running exe (-1: none known - the mod stays off)
static uintptr_t g_addr[A_N];
#define GA(n) (g_addr[A_##n])
#define GF(n) ((n##_fn)g_addr[A_##n])
#define GL(n) (k_addr_list_##n[g_exe_build])
static uintptr_t g_code_lo, g_code_hi;   // the exe's code section and its whole image (its PE header: the same test
static uintptr_t g_image_lo, g_image_hi; // for every build)
static int g_addr_missing;               // names the running build hasn't got (logged at the start: addr_log)

// the running exe's column into g_addr; the names it hasn't got (0) are counted - the caller logs them
static int addr_select(int build) {
    int missing = 0;
    g_exe_build = build;
    for (int i = 0; i < A_N; i++) missing += !(g_addr[i] = k_addr[i][build]);
    uint8_t *b = (uint8_t *)GetModuleHandleA(NULL);
    IMAGE_NT_HEADERS32 *nt = (IMAGE_NT_HEADERS32 *)(b + *(uint32_t *)(b + 0x3C));
    g_image_lo = (uintptr_t)b;
    g_image_hi = g_image_lo + nt->OptionalHeader.SizeOfImage;
    IMAGE_SECTION_HEADER *s = IMAGE_FIRST_SECTION(nt);
    for (int i = 0; i < nt->FileHeader.NumberOfSections; i++, s++)
        if (s->Characteristics & IMAGE_SCN_CNT_CODE) {
            g_code_lo = (uintptr_t)b + s->VirtualAddress;
            g_code_hi = g_code_lo + s->Misc.VirtualSize;
            break;
        }
    return missing;
}
static void addr_log(FILE *log) {   // the names the running build hasn't got: what uses them doesn't work there
    if (!log || !g_addr_missing) return;
    fprintf(log, "# addresses: build %d hasn't got %d of %d:", g_exe_build, g_addr_missing, A_N);
    for (int i = 0; i < A_N; i++) if (!g_addr[i]) fprintf(log, " %s", k_addr_name[i]);
    fputc(10, log);
}
