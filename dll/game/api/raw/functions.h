// THE ADDRESSES OF THE GAME'S FUNCTIONS THE MOD CALLS (functions.c wraps them): their rows of tables.c's tables, in
// the same columns (one value per exe build, k_exe_builds' order). F(name, return type, calling convention, (parameters),
// value per build) a function, typed by its row; D(name, value per build) an address or number. A function the mod also
// hooks is here too; where it is hooked (its call sites, vtables, slots) is in hooks.h.

#define ADDRS_FUNCTIONS(F, D) \
    F(FN_TECH_LOOKUP, uint8_t *, __thiscall, (void *, int), 0x004019A1, 0x004018A4) \
    /* thiscall (tile x, tile y): set target + queue (only if tile valid) */ \
    F(FN_CAMACT_SET, void, __thiscall, (void *, int, int), 0x00401F3E, 0x00401EE0) \
    F(FN_EXPLORED, char, __thiscall, (void *, void *), 0x00404A8A, 0x004042E2) /* thiscall(site+0x14 object, player) -> bool */ \
    F(FN_SAVE_ACTION_CTOR, void *, __thiscall, (void *, void *, int, struct ustring), 0x00404FA1, 0x00404613) \
    F(FN_SCROLL_EXECUTE, void, __thiscall, (void *), 0x004050FC, 0x0040478E) \
    /* point from int x, y (the task location's point) */ \
    F(FN_POINT_SET, void, __thiscall, (void *, int *), 0x0040D29D, 0x0044D28B) \
    F(FN_ROUTE_COST, int, __thiscall, (ee_route_planner *, int *, ee_route_node *), 0x004141E8, 0x00410074) /* its slot 1: int thiscall(edge*, node*) */ \
    /* the area object at a tile (thiscall on the map areas G_MAP_AREAS) */ \
    F(FN_AREA_AT, uint8_t *, __thiscall, (void *, int *), 0x00425F16, 0x00422AD9) \
    F(FN_REGION, unsigned short, __thiscall, (void *, int *, int), 0x00425F9F, 0x00422B86) /* thiscall(0x917BC8, int pos[2], 2) -> short region id */ \
    /* where an idle transport lands (thiscall(transport manager, transport)) */ \
    F(FN_SHORE_TO_UNLOAD, int, __thiscall, (void *, void *), 0x0042C49A, 0x00422658) \
    F(FN_UNITS_INSIDE, int, __thiscall, (void *), 0x0042E347, 0x0040D5C5)     /* units aboard a transport or inside a building */ \
    F(FN_CP_ADD_RESOURCE, void, __thiscall, (void *, int, int), 0x0042F783, 0x0042D1A5) /* EEComputerPlayer::vf18(type, amount) */ \
    /* thiscall(player), called at 0x4311E5, 0x43FA9B, 0x474AF0 */ \
    F(FN_POP_CAP, void, __thiscall, (void *), 0x0042F7A5, 0x0042D1C7) \
    /* thiscall(player, on player, 1): calls off its attack groups on that player */ \
    F(FN_CALL_OFF_ATTACK, void, __thiscall, (void *, void *, int), 0x0043033E, 0x0042DCA6) \
    F(FN_DELETE_FOR_ROOM, int, __thiscall, (void *, void *, int), 0x00430D40, 0x0042E851) \
    /* EEComputerPlayer::AdjustCost (vf27): a price with the cheat */ \
    F(FN_ADJUST_COST, int, __thiscall, (void *, int), 0x00430FBD, 0x0042EACE) \
    F(FN_EPOCH_CATCH_UP, void, __thiscall, (void *, void *), 0x00431533, 0x0042F064) /* thiscall(computer player, human): the attempt redrawn soon when the human is ahead */ \
    /* thiscall(player, now): the next attack time (+0xED8) */ \
    F(FN_SCHEDULE_ATTACK, void, __thiscall, (void *, uint32_t), 0x0043DD2F, 0x0043D4CD) \
    /* EEComputerPlayer::vf45(cost object, ?, ?): pays a unit/building/tech */ \
    F(FN_CP_PAY, void, __thiscall, (void *, void *, int, int), 0x0043F53D, 0x0043ED98) \
    F(FN_CP_UPDATE, void, __thiscall, (ee_player *, uint32_t), 0x0043F91A, 0x0043F175) \
    F(FN_GIVE_GOAL, void, __thiscall, (void *, void *, int), 0x004412D4, 0x0043D416) /* thiscall(unit, goal, slot) */ \
    F(FN_TASK_CTOR, void, __thiscall, (void *), 0x00441FB3, 0x0044162F)       /* an attack task */ \
    F(FN_TASK_SET_TARGET, void, __thiscall, (void *, void *), 0x00442246, 0x00441912) \
    F(FN_TASK_DTOR, void, __thiscall, (void *), 0x00442283, 0x004417F5) \
    F(FN_ASG_AIR_GIVE_GOAL, char, __thiscall, (void *, void *, void *, int, int), 0x00442AD8, 0x00442220) \
    F(FN_ASG_AIR_UPDATE, void, __thiscall, (void *, int), 0x00442DBA, 0x0045019D) /* EECPASGAir slot 90: update(now) */ \
    F(FN_ASG_LAND_STATS, void, __thiscall, (void *), 0x004472A9, 0x004466DD) \
    F(FN_ASG_UPDATE, void, __thiscall, (void *, int), 0x004491DD, 0x004488CE) /* EECPASGLand slot 90: update(now) */ \
    F(FN_ASG_ROUTE, void, __thiscall, (ee_attack_subgroup *, int *, int *), 0x0044BC57, 0x0044B3C7) /* its slot 94: void thiscall(int start[2], int dest[2]) */ \
    F(FN_ASG_ROUTE_DONE, void, __thiscall, (ee_attack_subgroup *), 0x0044BDBF, 0x0044B53D) /* its slot 97: route arrived, waypoints in the deque at +0x60 */ \
    F(FN_ASG_SEA_UPDATE, void, __thiscall, (void *, int), 0x0044ED11, 0x0044F70E) /* EECPASGSea slot 90: update(now) */ \
    /* EECPASGSeaTransport slot 90: update(now) */ \
    F(FN_ASG_SEA_TRANSPORT_UPDATE, void, __thiscall, (void *, int), 0x0044FB92, 0x0044E9BB) \
    F(FN_GROUP_STAT, void, __thiscall, (void *, void *), 0x00450554, 0x00451239) \
    F(FN_UNIT_TO_GROUP, char, __thiscall, (void *, void *), 0x00450E07, 0x00451A8A) \
    /* thiscall(attack manager, task, strategy, unit ids) -> the attack group */ \
    F(FN_ATTACK_GROUP_NEW, uint8_t *, __thiscall, (void *, void *, void *, void *), 0x0045106C, 0x00451D8E) \
    F(FN_ASG_STATS, void, __thiscall, (void *), 0x0045AD2F, 0x0045C925) \
    F(FN_ASG_GIVE_GOAL, char, __thiscall, (void *, void *, void *, int, int), 0x0045CBEA, 0x0045E7F9) \
    F(FN_ASGU_SCORE, float, __thiscall, (ee_asg_unit *, ee_asgu_info *), 0x00461854, 0x00463494) /* float thiscall(info *) */ \
    F(FN_BM_QUEUED, char, __thiscall, (void *, int), 0x00465640, 0x004672FA)  /* thiscall(build manager, type): queued */ \
    F(FN_PLACE, char, __thiscall, (void *, int, int, int), 0x0046580A, 0x004674C4) \
    F(FN_ENEMY_NEAR, char, __thiscall, (void *, int, int), 0x004660B9, 0x00467E10) /* thiscall(build manager, x, y) -> bool */ \
    /* thiscall(build manager, type, 1, 0): one building requested */ \
    F(FN_BM_REQUEST, void, __thiscall, (void *, int, int, int), 0x0046802D, 0x0046A9C1) \
    F(FN_BQ_ADD, void, __thiscall, (void *, int *), 0x00468C37, 0x0046B798)   /* an entry into a build manager epoch queue */ \
    F(FN_FISH_TYPE, int, __thiscall, (void *), 0x0046D209, 0x0046FDD3) \
    F(FN_HUMAN_EPOCH, int, __cdecl, (void), 0x0046D436, 0x0047005A)           /* the highest epoch of the players SLOT_PLAYER_HUMAN says are human, 0 without */ \
    F(FN_SET_EPOCH_TIME, void, __thiscall, (void *, uint32_t, char), 0x0046DE8C, 0x00470B04) /* SetNextEpochAdvanceTime */ \
    F(FN_REVENGE, void, __thiscall, (void *, void *), 0x0046FFB7, 0x00472EC3) \
    /* thiscall(cm, task, strategy, unit ids) */ \
    F(FN_ATTACK_PLAN_OK, char, __thiscall, (void *, void *, void *, void *), 0x0047067A, 0x0047360A) \
    F(FN_CM_UNIT_FREE, char, __thiscall, (void *, void *), 0x004707D8, 0x0047377E) /* thiscall(cm, unit state): in no group */ \
    F(FN_STRATEGY_CTOR, void, __thiscall, (void *), 0x00473BC4, 0x00477084) \
    F(FN_STRATEGY_DTOR, void, __thiscall, (void *), 0x00473C99, 0x0047716D) \
    F(FN_STRATEGY_INIT, void, __thiscall, (void *), 0x00473CA7, 0x0047717B) \
    F(FN_LOCATION_CTOR, void, __thiscall, (void *), 0x0047C4E8, 0x00480873) \
    F(FN_TASK_SET_LOCATION, void, __thiscall, (void *, void *), 0x0047C8B7, 0x00480C42) \
    F(FN_NATIVE_CHAT, char, __thiscall, (void *, int, int), 0x00484EB3, 0x0048966C) \
    F(FN_RS_SCHEDULE, void, __thiscall, (void *, int), 0x00485F57, 0x0048A79F) \
    /* thiscall(research manager cm+0x40, count): the AI's request for an epoch's required techs */ \
    F(FN_REQUEST_TECHS, void, __thiscall, (void *, int), 0x004860D6, 0x0048A91E) \
    /* thiscall(zone, type) -> first site of that type with amount > 0 */ \
    F(FN_ZONE_SITE, uint8_t *, __thiscall, (void *, int), 0x004886F2, 0x0048D065) \
    F(FN_ASSIGN_CITIZENS, char, __thiscall, (void *), 0x0048A554, 0x0048EECF) \
    F(FN_HUNT, void *, __thiscall, (void *, uintptr_t, uintptr_t, uintptr_t), 0x0048BA3B, 0x00490458) /* (count, flag, out list) */ \
    F(FN_FIND_FOOD, void *, __thiscall, (void *, uintptr_t, uintptr_t, uintptr_t), 0x0048BBA8, 0x00490602) /* (char needDropsite, char nearBase, unit) -> site */ \
    F(FN_UNBUILT_FARMS, void *, __thiscall, (void *, uintptr_t, uintptr_t), 0x0048BF4F, 0x004909D7) \
    F(FN_FARMS, void *, __thiscall, (void *, uintptr_t, uintptr_t), 0x0048BFD0, 0x00490A63) /* (count, out list) */ \
    F(FN_FIND_WOOD, void *, __thiscall, (void *, uintptr_t, uintptr_t), 0x0048C060, 0x00490AF3) /* (flag, unit) -> target */ \
    F(FN_FIND_MINE, void *, __thiscall, (void *, uintptr_t, uintptr_t, uintptr_t), 0x0048C204, 0x00490CB5) /* (type, unit, reverse) -> site */ \
    F(FN_ASSIGN_TO_SITE, void, __thiscall, (void *, void *, void *, int), 0x0048C81C, 0x004912BE) /* (unit, target, 0) */ \
    F(FN_COMPUTE_TARGETS, char, __thiscall, (void *), 0x0048D0A2, 0x00491B78) \
    F(FN_RM_UPDATE, void, __thiscall, (void *, uint32_t), 0x0048D263, 0x00491D34) \
    F(FN_TABLE_VALUE, unsigned, __thiscall, (void *, int), 0x0048E817, 0x004CD4B3) \
    F(FN_UM_PENDING, char, __thiscall, (void *, int), 0x0048E9D8, 0x00493696) /* thiscall(unit manager, type) */ \
    F(FN_UM_IN_PRODUCTION, char, __thiscall, (void *, int), 0x0048EA2E, 0x004936EC) /* thiscall(unit manager, type) */ \
    F(FN_UM_CITIZENS_WANTED, int, __thiscall, (void *), 0x0048EDDB, 0x00493AEE) /* citizen share x population cap */ \
    F(FN_TRAIN_CITIZENS, uintptr_t, __thiscall, (void *, uintptr_t), 0x0048EE84, 0x00493B97) \
    /* citizens of both sexes and the epoch's fishing boat type */ \
    F(FN_UM_CITIZENS, int, __thiscall, (void *), 0x0048F819, 0x00494572) \
    /* thiscall(unit manager, type): its units and buildings of that type */ \
    F(FN_UM_TYPE_LIST, uint8_t *, __thiscall, (void *, int), 0x0048F8AF, 0x00494608) \
    F(FN_TRAIN_UNITS, uintptr_t, __thiscall, (void *, int *, uintptr_t), 0x0049014F, 0x00494F42) \
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
    F(FN_SEL_PACK, unsigned, __cdecl, (void *, uint32_t, uint8_t *), 0x004BD15A, 0x004C5E67) \
    F(FN_DAMAGE_POINTS, int, __thiscall, (void *, int, unsigned), 0x004D291F, 0x004D84EC) \
    /* thiscall(G_BONUS_TABLE, weapon kind, target type data, attacker, target player) */ \
    F(FN_ATTACK_BONUS, float, __thiscall, (void *, int, void *, int, int), 0x004D5F2C, 0x004DBE44) \
    F(FN_MOUSE_CLAMP, void, __thiscall, (void *), 0x00510DAD, 0x0051B31C) \
    F(FN_MOUSE_UPDATE, void, __thiscall, (void *), 0x00510E5D, 0x0051B3CC) \
    /* thiscall on G_CAMERA (float dx, float dy) */ \
    F(FN_CAMERA_MOVE, void, __thiscall, (void *, float, float), 0x0051E2E6, 0x00528EA5) \
    /* thiscall([G_CHEAT_PARSER], UWideString *, int *code, int *arg) */ \
    F(FN_CHEAT_PARSE, void, __thiscall, (void *, void *, int *, int *), 0x00520A17, 0x0052B5D6) \
    F(FN_LOAD_SAVED, int, __cdecl, (void *), 0x00521F94, 0x0052CB90) \
    F(FN_PARSE_CMDLINE, char, __cdecl, (const char *), 0x00522A36, 0x0052D703) \
    F(FN_GAME_START, char, __thiscall, (void *, ee_setup *, void *), 0x0052599B, 0x005306F2) \
    F(FN_COLOR_RGB, void *, __cdecl, (unsigned), 0x00526B85, 0x005318BD) \
    F(FN_SETTINGS_SAVE, void, __thiscall, (void *), 0x0052AA81, 0x00535943)   /* every setting into the registry */ \
    F(FN_SET_STANCE, void, __thiscall, (void *, int, int, int), 0x0052D5D7, 0x005384DD) \
    /* thiscall(G_GAME_STATE, char won, index) */ \
    F(FN_GAME_PLAYER_OUT, void, __thiscall, (void *, int, int), 0x0052E2B7, 0x005391F2) \
    F(FN_SITE_GATHERERS, unsigned, __thiscall, (void *, void *), 0x0052E5F7, 0x00539532) \
    F(FN_GAME_PLAYER, void *, __thiscall, (void *, int), 0x0052E6EF, 0x00539670) /* thiscall(G_GAME_STATE, index) */ \
    F(FN_SAVE_GAME, int, __cdecl, (void *, int, int, struct ustring), 0x00540ACA, 0x0054D066) \
    F(FN_PHYS_STEP, void, __thiscall, (void *), 0x00546C63, 0x00553082) \
    /* thiscall(player, type, 0x20, 0): can train or build one now */ \
    F(FN_CAN_BUILD, char, __thiscall, (void *, int, int, int), 0x00555710, 0x005639E8) \
    F(FN_POPULATION, int, __thiscall, (void *, int, int), 0x00556E11, 0x005651E0) /* thiscall(player, 1, 0) */ \
    /* EEPlayer::vf31 thiscall(player, 0): the game's unit limit for this player */ \
    F(FN_GAME_LIMIT, int, __thiscall, (void *, int), 0x00556EBC, 0x0056528B) \
    F(FN_SELECT_UNITS, void, __thiscall, (void *, void *), 0x0055B3FB, 0x0056A658) /* EEMSelectUnits carried out on the player */ \
    F(FN_SET_DIPLOMACY, void, __thiscall, (void *, void *, int, int), 0x0055B8E1, 0x0056AB41) \
    F(FN_PLAYER_RESIGN, void, __thiscall, (void *, int), 0x0055BCEA, 0x0056AF5C) /* thiscall(player, 2) as EEMResign */ \
    F(FN_STAT_ADD, void, __thiscall, (void *, int, int), 0x0055DBCF, 0x0056D507) \
    F(FN_CAMACT_CTOR, void *, __thiscall, (void *), 0x00560179, 0x0056FAD4)   /* EEACenterCamera ctor (0x1C bytes) */ \
    F(FN_EDITOR_FORM, void *, __thiscall, (void *, void *), 0x0058D3E1, 0x0059D615) \
    F(SM_RUN_EFFECT, void, __thiscall, (void *, int), 0x005AD29B, 0x005BD5EB) \
    F(FN_EPOCH_NEXT, void, __thiscall, (void *), 0x005BC15D, 0x005CCB34)    /* thiscall(tech tree): the next epoch (to the game's end epoch) - 0x5BC893 sets it, sends the epoch event */ \
    D(FN_EPOCH_UPGRADES, 0x005BDDBD, 0x005CE8C5)                              /* thiscall(node, category), AoC's (+ only new): econ.c */ \
    F(FN_TECH_GRANT, int, __thiscall, (void *), 0x005BF335, 0x005CFA53)       /* AoC's takes an argument: called in the 2017 update only */ \
    F(FN_CHEAT_MSG_CTOR, void *, __thiscall, (void *, int, int), 0x005C29FB, 0x005D3162) /* new 0x18; (code, arg) */ \
    F(FN_CHAT_MSG_CTOR, void *, __thiscall, (void *, void *), 0x005C2A2F, 0x005D3196) /* new 0x24; (UWideString *) */ \
    F(FN_RESIGN_SEND, void, __cdecl, (void), 0x005D05CC, 0x005E0E7F) \
    F(FN_EXIT_CONFIRM, void, __cdecl, (void), 0x005D0B4F, 0x005E145B) \
    F(FN_UNIT_RELEASE, void, __thiscall, (void *), 0x005ED71E, 0x005FB7FB) \
    /* thiscall(unit, 0x1F51) - as the assignment does after the order */ \
    F(FN_UNIT_SAY, void, __thiscall, (void *, int), 0x005EFCF0, 0x005FDFA5) \
    F(FN_SET_GOAL, void, __thiscall, (void *, void *, char, int), 0x005F05BD, 0x005FE863) /* thiscall(unit, goal, 0, 0) */ \
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
    F(FN_GOALSET_ASK, int, __thiscall, (void *, void *, int *), 0x00619FF5, 0x0062CD8B) \
    F(FN_VECTOR_FREE, void, __thiscall, (void *), 0x0061ABEC, 0x006002C7)     /* a vector's dtor (the game's delete) */ \
    F(FN_FIRE, void, __thiscall, (void *, void *), 0x0061E11C, 0x00631312) /* thiscall(attacker class, attacker) */ \
    F(FN_UNIT_UNLINK, void, __thiscall, (void *), 0x00628E29, 0x0063DBDA) \
    F(FN_BALLISTIC, char, __cdecl, (ee_unit *, ee_unit_stats *, float *, float *, float *), 0x0062CBB3, 0x00641A6A) \
    F(FN_UNIT_DISTANCE, float, __cdecl, (void *, void *), 0x0062D5A4, 0x00642489) /* cdecl(unit, other) */ \
    /* cdecl char(player, type, x, y, w, h, 0) */ \
    F(FN_CAN_PLACE, char, __cdecl, (void *, int, int, int, int, int, int), 0x0062E940, 0x00643CE6) \
    /* cdecl(player, type, x, y): a building placed (a foundation) */ \
    F(FN_PLACE_BUILDING, void *, __cdecl, (void *, int, int, int), 0x0062EF8A, 0x00644C83) \
    /* cdecl(unit, player index, new type data) */ \
    F(FN_UNIT_CONVERT, int, __cdecl, (void *, int, uint32_t), 0x0062F192, 0x00644E95) \
    F(FN_AIM, void, __cdecl, (ee_unit *, ee_unit **, float *, void *), 0x00633413, 0x0064939D) \
    F(FN_RANGE_SORT, void, __cdecl, (void **, void **, void *), 0x0063A355, 0x006509AF) \
    /* EEMHackANewUnit: thiscall(int *type, float xy[2], char player) */ \
    F(FN_NEW_UNIT_MSG_CTOR, void, __thiscall, (void *, int *, float *, char), 0x00650B3C, 0x00662C24) \
    F(FN_SET_RESOLUTION, void, __stdcall, (int, int, int, int), 0x0065F314, 0x006768FE) \
    F(FN_QUEUE_ACTION, void, __thiscall, (void *, void *), 0x0066192F, 0x00678F79) \
    /* thiscall(ui [G_UI], colour, name, line, ms, 0): a chat or system line */ \
    F(FN_UI_LINE, void, __thiscall, (void *, void *, void *, void *, int, int), 0x00661E91, 0x006794C6) \
    F(FN_NEW, void *, __cdecl, (size_t), 0x00683718, 0x0069D178)              /* the game's operator new */ \
    D(IAT_RESOURCE_STRING, 0x0082319C, 0x00837198)                            /* the game's string resource (a language entry) */ \
    D(IAT_WSTRING_CTOR, 0x008231A4, 0x008371A0)                               /* UWideString::UWideString() */ \
    D(IAT_RELEASE, 0x008231AC, 0x008371A8)                                    /* imported refcount release, thiscall on the action */ \
    D(IAT_USTRING_CTOR, 0x008231E4, 0x008371E0)                               /* UString::UString() */ \
    D(IAT_WSTRING_ASSIGN, 0x00823224, 0x00837220)                             /* UWideString::operator=(const char *) */ \
    D(IAT_USTRING_ASSIGN, 0x00823228, 0x00837224)                             /* UString::operator=(const char *) */ \
    D(IAT_WSTRING_CTOR_C, 0x008232C4, 0x008372C0)                             /* UWideString::UWideString(const char *) */ \
    D(IAT_WSTRING_FORMAT, 0x008232C8, 0x008372C4)                             /* UWideString::Format(const char *, ...) */ \
    D(IAT_COS, 0x00823304, 0x00837340)                                        /* cos (the game's) */ \
    D(IAT_EDIT_GET_TEXT, 0x00823328, 0x00837EE0)                              /* UIEditBox::GetText */ \
    D(IMP_IS_MOUSE, 0x008235CC, 0x00837570)                                   /* import: &ISMouse* in Low-Level Engine.dll */ \
    D(IAT_WSTRING_RESERVE, 0x008235D0, 0x00837574) \
    D(IAT_WSTRING_DTOR, 0x00823AEC, 0x00837194)                               /* UWideString::~UWideString() */ \
    D(IMP_IS_KEYBOARD, 0x00823AF8, 0x00837AE8)                                /* import: &ISKeyboard* in Low-Level Engine.dll */ \
    D(IAT_UIM_FORM, 0x00823D60, 0x00837B44)                                   /* the UI manager's current form */ \
    D(IAT_USTRING_DTOR, 0x008245BC, 0x008385B0)                               /* UString::~UString() */ \
    D(IAT_RAND, 0x00824648, 0x0083863C)                                       /* the game's rand */ \
    D(VT_PAUSE_ACTION, 0x00830E94, 0x00846790)                                /* EEAUserPauseGame vtable (0xC bytes: vtable, refcount, byte) */ \
    D(VT_GOAL_ATTACK, 0x00831968, 0x00847380) \
    D(VT_GOAL_BUILD, 0x008319F0, 0x0084740C) \
    /* EEUGLoadTransport: the holder's goal and each unit's part (0x614833), the holder at +0x50 */ \
    D(VT_GOAL_LOAD, 0x00831DC0, 0x008477F8) \
    D(VT_GOAL_LOCATION, 0x00831E64, 0x008478A0) \
    D(VT_GOAL_UNLOAD, 0x00831F5C, 0x008479A0)                                 /* EEUGUnloadTransport (new 0x40, ctor 0x615A1F(transport, x, y, flag)) */ \


#define ADDR_NUMBERS_FUNCTIONS(D) \
    D(SLOT_TECH_AVAILABLE, 1, 1)        /* EETechTreeNode: char thiscall(tech) */ \
    D(SLOT_GOAL_PART, 4, 4)             /* EEUGLoadTransport: a unit's part of the load goal */ \
    D(SLOT_TYPE_ASSIGN, 29, 29)         /* EEUnitClass: thiscall(type data, unit), as the game's goal assignment */ \
    D(SLOT_PLAYER_HUMAN, 29, 29)        /* EEPlayer: char thiscall(player) - human (EEHumanPlayer's returns 1; FN_HUMAN_EPOCH's test) */ \
    D(SLOT_UNIT_LIMIT, 31, 31)          /* EEPlayer: int thiscall(player, 0), the unit limit (FN_GAME_LIMIT) */ \
    D(RVA_MIX_CHANNELS, 0x52AC, 0x52AC)       /* Miles Sound System Mixer.dll: its channels, a vector of 8 bytes each */ \
    D(RVA_MIX_MAX_CHANNELS, 0x5320, 0x5320)   /* int: the most channels it gives */ \
    D(SLOT_GROUP_COUNT, 6, 6)           /* the selected group (player +0x9BC): unsigned thiscall(group); AoC not checked */ \
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
    D(SIZE_GOAL_LOAD, 0x68, 0x68)       /* FN_GOAL_LOAD_CTOR */ \


#define ADDR_LISTS_FUNCTIONS(L) \


