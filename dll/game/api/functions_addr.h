// THE ADDRESSES OF THE GAME'S FUNCTIONS THE MOD CALLS (functions.h wraps them): their rows of addr.h's tables, in
// the same columns (one value per exe build, k_exe_builds' order). F(name, return type, calling convention, (parameters),
// value per build) a function, typed by its row; D(name, value per build) an address or number. A function the mod also
// hooks is here too; where it is hooked (its call sites, vtables, slots) is in hooks_addr.h.

#define ADDRS_FUNCTIONS(F, D) \
    F(FN_TECH_LOOKUP, uint8_t *, __thiscall, (void *, int), 0x004019A1, 0x004018A4) \
    F(FN_EXPLORED, char, __thiscall, (void *, void *), 0x00404A8A, 0x004042E2) /* thiscall(site+0x14 object, player) -> bool */ \
    D(FN_ROUTE_COST, 0x004141E8, 0x00410074)                                  /* its slot 1: int thiscall(edge*, node*) */ \
    D(FN_REGION, 0x00425F9F, 0x00422B86)                                      /* thiscall(0x917BC8, int pos[2], 2) -> short region id */ \
    /* where an idle transport lands (thiscall(transport manager, transport)) */ \
    F(FN_SHORE_TO_UNLOAD, int, __thiscall, (void *, void *), 0x0042C49A, 0x00422658) \
    F(FN_UNITS_INSIDE, int, __thiscall, (void *), 0x0042E347, 0x0040D5C5)     /* units aboard a transport or inside a building */ \
    F(FN_CP_ADD_RESOURCE, void, __thiscall, (void *, int, int), 0x0042F783, 0x0042D1A5) /* EEComputerPlayer::vf18(type, amount) */ \
    /* thiscall(player), called at 0x4311E5, 0x43FA9B, 0x474AF0 */ \
    F(FN_POP_CAP, void, __thiscall, (void *), 0x0042F7A5, 0x0042D1C7) \
    /* thiscall(player, on player, 1): calls off its attack groups on that player */ \
    F(FN_CALL_OFF_ATTACK, void, __thiscall, (void *, void *, int), 0x0043033E, 0x0042DCA6) \
    F(FN_DELETE_FOR_ROOM, int, __thiscall, (void *, void *, int), 0x00430D40, 0x0042E851) \
    F(FN_EPOCH_CATCH_UP, void, __thiscall, (void *, void *), 0x00431533, 0x0042F064) /* thiscall(computer player, human): the attempt redrawn soon when the human is ahead */ \
    /* EEComputerPlayer::vf45(cost object, ?, ?): pays a unit/building/tech */ \
    F(FN_CP_PAY, void, __thiscall, (void *, void *, int, int), 0x0043F53D, 0x0043ED98) \
    D(FN_CP_UPDATE, 0x0043F91A, 0x0043F175) \
    F(FN_GIVE_GOAL, void, __thiscall, (void *, void *, int), 0x004412D4, 0x0043D416) /* thiscall(unit, goal, slot) */ \
    F(FN_ASG_AIR_GIVE_GOAL, char, __thiscall, (void *, void *, void *, int, int), 0x00442AD8, 0x00442220) \
    F(FN_ASG_AIR_UPDATE, void, __thiscall, (void *, int), 0x00442DBA, 0x0045019D) /* EECPASGAir slot 90: update(now) */ \
    F(FN_ASG_LAND_STATS, void, __thiscall, (void *), 0x004472A9, 0x004466DD) \
    F(FN_ASG_UPDATE, void, __thiscall, (void *, int), 0x004491DD, 0x004488CE) /* EECPASGLand slot 90: update(now) */ \
    D(FN_ASG_ROUTE, 0x0044BC57, 0x0044B3C7)                                   /* its slot 94: void thiscall(int start[2], int dest[2]) */ \
    D(FN_ASG_ROUTE_DONE, 0x0044BDBF, 0x0044B53D)                              /* its slot 97: route arrived, waypoints in the deque at +0x60 */ \
    F(FN_ASG_SEA_UPDATE, void, __thiscall, (void *, int), 0x0044ED11, 0x0044F70E) /* EECPASGSea slot 90: update(now) */ \
    /* EECPASGSeaTransport slot 90: update(now) */ \
    F(FN_ASG_SEA_TRANSPORT_UPDATE, void, __thiscall, (void *, int), 0x0044FB92, 0x0044E9BB) \
    F(FN_GROUP_STAT, void, __thiscall, (void *, void *), 0x00450554, 0x00451239) \
    F(FN_UNIT_TO_GROUP, char, __thiscall, (void *, void *), 0x00450E07, 0x00451A8A) \
    F(FN_ASG_STATS, void, __thiscall, (void *), 0x0045AD2F, 0x0045C925) \
    F(FN_ASG_GIVE_GOAL, char, __thiscall, (void *, void *, void *, int, int), 0x0045CBEA, 0x0045E7F9) \
    D(FN_ASGU_SCORE, 0x00461854, 0x00463494)                                  /* float thiscall(info *) */ \
    F(FN_BM_QUEUED, char, __thiscall, (void *, int), 0x00465640, 0x004672FA)  /* thiscall(build manager, type): queued */ \
    F(FN_PLACE, char, __thiscall, (void *, int, int, int), 0x0046580A, 0x004674C4) \
    F(FN_ENEMY_NEAR, char, __thiscall, (void *, int, int), 0x004660B9, 0x00467E10) /* thiscall(build manager, x, y) -> bool */ \
    /* thiscall(build manager, type, 1, 0): one building requested */ \
    F(FN_BM_REQUEST, void, __thiscall, (void *, int, int, int), 0x0046802D, 0x0046A9C1) \
    F(FN_BQ_ADD, void, __thiscall, (void *, int *), 0x00468C37, 0x0046B798)   /* an entry into a build manager epoch queue */ \
    F(FN_FISH_TYPE, int, __thiscall, (void *), 0x0046D209, 0x0046FDD3) \
    F(FN_HUMAN_EPOCH, int, __cdecl, (void), 0x0046D436, 0x0047005A)           /* the highest epoch of the players SLOT_PLAYER_HUMAN says are human, 0 without */ \
    F(FN_REVENGE, void, __thiscall, (void *, void *), 0x0046FFB7, 0x00472EC3) \
    F(FN_CM_UNIT_FREE, char, __thiscall, (void *, void *), 0x004707D8, 0x0047377E) /* thiscall(cm, unit state): in no group */ \
    F(FN_NATIVE_CHAT, char, __thiscall, (void *, int, int), 0x00484EB3, 0x0048966C) \
    F(FN_RS_SCHEDULE, void, __thiscall, (void *, int), 0x00485F57, 0x0048A79F) \
    /* thiscall(research manager cm+0x40, count): the AI's request for an epoch's required techs */ \
    F(FN_REQUEST_TECHS, void, __thiscall, (void *, int), 0x004860D6, 0x0048A91E) \
    /* thiscall(zone, type) -> first site of that type with amount > 0 */ \
    F(FN_ZONE_SITE, uint8_t *, __thiscall, (void *, int), 0x004886F2, 0x0048D065) \
    D(FN_ASSIGN_CITIZENS, 0x0048A554, 0x0048EECF) \
    D(FN_HUNT, 0x0048BA3B, 0x00490458)                                        /* (count, flag, out list) */ \
    D(FN_FIND_FOOD, 0x0048BBA8, 0x00490602)                                   /* (char needDropsite, char nearBase, unit) -> site */ \
    D(FN_UNBUILT_FARMS, 0x0048BF4F, 0x004909D7) \
    D(FN_FARMS, 0x0048BFD0, 0x00490A63)                                       /* (count, out list) */ \
    D(FN_FIND_WOOD, 0x0048C060, 0x00490AF3)                                   /* (flag, unit) -> target */ \
    D(FN_FIND_MINE, 0x0048C204, 0x00490CB5)                                   /* (type, unit, reverse) -> site */ \
    F(FN_ASSIGN_TO_SITE, void, __thiscall, (void *, void *, void *, int), 0x0048C81C, 0x004912BE) /* (unit, target, 0) */ \
    D(FN_COMPUTE_TARGETS, 0x0048D0A2, 0x00491B78) \
    D(FN_RM_UPDATE, 0x0048D263, 0x00491D34) \
    F(FN_TABLE_VALUE, unsigned, __thiscall, (void *, int), 0x0048E817, 0x004CD4B3) \
    D(FN_UM_PENDING, 0x0048E9D8, 0x00493696)                                  /* thiscall(unit manager, type) */ \
    D(FN_UM_IN_PRODUCTION, 0x0048EA2E, 0x004936EC)                            /* thiscall(unit manager, type) */ \
    F(FN_UM_CITIZENS_WANTED, int, __thiscall, (void *), 0x0048EDDB, 0x00493AEE) /* citizen share x population cap */ \
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
    F(FN_DAMAGE_POINTS, int, __thiscall, (void *, int, unsigned), 0x004D291F, 0x004D84EC) \
    /* thiscall(G_BONUS_TABLE, weapon kind, target type data, attacker, target player) */ \
    F(FN_ATTACK_BONUS, float, __thiscall, (void *, int, void *, int, int), 0x004D5F2C, 0x004DBE44) \
    /* thiscall([G_CHEAT_PARSER], UWideString *, int *code, int *arg) */ \
    F(FN_CHEAT_PARSE, void, __thiscall, (void *, void *, int *, int *), 0x00520A17, 0x0052B5D6) \
    D(FN_LOAD_SAVED, 0x00521F94, 0x0052CB90) \
    D(FN_PARSE_CMDLINE, 0x00522A36, 0x0052D703) \
    D(FN_GAME_START, 0x0052599B, 0x005306F2) \
    F(FN_COLOR_RGB, void *, __cdecl, (unsigned), 0x00526B85, 0x005318BD) \
    F(FN_SET_STANCE, void, __thiscall, (void *, int, int, int), 0x0052D5D7, 0x005384DD) \
    /* thiscall(G_GAME_STATE, char won, index) */ \
    F(FN_GAME_PLAYER_OUT, void, __thiscall, (void *, int, int), 0x0052E2B7, 0x005391F2) \
    F(FN_SITE_GATHERERS, unsigned, __thiscall, (void *, void *), 0x0052E5F7, 0x00539532) \
    F(FN_GAME_PLAYER, void *, __thiscall, (void *, int), 0x0052E6EF, 0x00539670) /* thiscall(G_GAME_STATE, index) */ \
    F(FN_SAVE_GAME, int, __cdecl, (void *, int, int, struct ustring), 0x00540ACA, 0x0054D066) \
    /* thiscall(player, type, 0x20, 0): can train or build one now */ \
    F(FN_CAN_BUILD, char, __thiscall, (void *, int, int, int), 0x00555710, 0x005639E8) \
    F(FN_POPULATION, int, __thiscall, (void *, int, int), 0x00556E11, 0x005651E0) /* thiscall(player, 1, 0) */ \
    /* EEPlayer::vf31 thiscall(player, 0): the game's unit limit for this player */ \
    F(FN_GAME_LIMIT, int, __thiscall, (void *, int), 0x00556EBC, 0x0056528B) \
    F(FN_PLAYER_RESIGN, void, __thiscall, (void *, int), 0x0055BCEA, 0x0056AF5C) /* thiscall(player, 2) as EEMResign */ \
    F(FN_STAT_ADD, void, __thiscall, (void *, int, int), 0x0055DBCF, 0x0056D507) \
    D(FN_EPOCH_UPGRADES, 0x005BDDBD, 0x005CE8C5)                              /* thiscall(node, category), AoC's (+ only new): econ.c */ \
    F(FN_TECH_GRANT, int, __thiscall, (void *), 0x005BF335, 0x005CFA53)       /* AoC's takes an argument: called in the 2017 update only */ \
    F(FN_CHEAT_MSG_CTOR, void *, __thiscall, (void *, int, int), 0x005C29FB, 0x005D3162) /* new 0x18; (code, arg) */ \
    F(FN_CHAT_MSG_CTOR, void *, __thiscall, (void *, void *), 0x005C2A2F, 0x005D3196) /* new 0x24; (UWideString *) */ \
    F(FN_RESIGN_SEND, void, __cdecl, (void), 0x005D05CC, 0x005E0E7F) \
    F(FN_EXIT_CONFIRM, void, __cdecl, (void), 0x005D0B4F, 0x005E145B) \
    F(FN_UNIT_RELEASE, void, __thiscall, (void *), 0x005ED71E, 0x005FB7FB) \
    /* thiscall(unit, 0x1F51) - as the assignment does after the order */ \
    F(FN_UNIT_SAY, void, __thiscall, (void *, int), 0x005EFCF0, 0x005FDFA5) \
    D(FN_SET_GOAL, 0x005F05BD, 0x005FE863)                                    /* thiscall(unit, goal, 0, 0) */ \
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
    F(FN_UNIT_UNLINK, void, __thiscall, (void *), 0x00628E29, 0x0063DBDA) \
    F(FN_UNIT_DISTANCE, float, __cdecl, (void *, void *), 0x0062D5A4, 0x00642489) /* cdecl(unit, other) */ \
    /* cdecl char(player, type, x, y, w, h, 0) */ \
    F(FN_CAN_PLACE, char, __cdecl, (void *, int, int, int, int, int, int), 0x0062E940, 0x00643CE6) \
    /* cdecl(player, type, x, y): a building placed (a foundation) */ \
    F(FN_PLACE_BUILDING, void *, __cdecl, (void *, int, int, int), 0x0062EF8A, 0x00644C83) \
    /* cdecl(unit, player index, new type data) */ \
    F(FN_UNIT_CONVERT, int, __cdecl, (void *, int, uint32_t), 0x0062F192, 0x00644E95) \
    F(FN_RANGE_SORT, void, __cdecl, (void **, void **, void *), 0x0063A355, 0x006509AF) \
    /* thiscall(ui [G_UI], colour, name, line, ms, 0): a chat or system line */ \
    F(FN_UI_LINE, void, __thiscall, (void *, void *, void *, void *, int, int), 0x00661E91, 0x006794C6) \
    F(FN_NEW, void *, __cdecl, (size_t), 0x00683718, 0x0069D178)              /* the game's operator new */ \
    D(IAT_RESOURCE_STRING, 0x0082319C, 0x00837198)                            /* the game's string resource (a language entry) */ \
    D(IAT_WSTRING_CTOR, 0x008231A4, 0x008371A0)                               /* UWideString::UWideString() */ \
    D(IAT_USTRING_CTOR, 0x008231E4, 0x008371E0)                               /* UString::UString() */ \
    D(IAT_WSTRING_ASSIGN, 0x00823224, 0x00837220)                             /* UWideString::operator=(const char *) */ \
    D(IAT_USTRING_ASSIGN, 0x00823228, 0x00837224)                             /* UString::operator=(const char *) */ \
    D(IAT_WSTRING_CTOR_C, 0x008232C4, 0x008372C0)                             /* UWideString::UWideString(const char *) */ \
    D(IAT_WSTRING_FORMAT, 0x008232C8, 0x008372C4)                             /* UWideString::Format(const char *, ...) */ \
    D(IAT_WSTRING_RESERVE, 0x008235D0, 0x00837574) \
    D(IAT_WSTRING_DTOR, 0x00823AEC, 0x00837194)                               /* UWideString::~UWideString() */ \
    D(IAT_USTRING_DTOR, 0x008245BC, 0x008385B0)                               /* UString::~UString() */ \
    D(IAT_RAND, 0x00824648, 0x0083863C)                                       /* the game's rand */ \


#define ADDR_NUMBERS_FUNCTIONS(D) \
    D(SLOT_TECH_AVAILABLE, 1, 1)        /* EETechTreeNode: char thiscall(tech) */ \
    D(SLOT_GOAL_PART, 4, 4)             /* EEUGLoadTransport: a unit's part of the load goal */ \
    D(SLOT_SERVER_SEND, 6, 6)           /* [G_SERVER] (EEServer, EEClient): thiscall(net, message, 0xFF) */ \
    D(SLOT_TYPE_ASSIGN, 29, 29)         /* EEUnitClass: thiscall(type data, unit), as the game's goal assignment */ \
    D(SLOT_PLAYER_HUMAN, 29, 29)        /* EEPlayer: char thiscall(player) - human (EEHumanPlayer's returns 1; FN_HUMAN_EPOCH's test) */ \
    D(SLOT_UNIT_LIMIT, 31, 31)          /* EEPlayer: int thiscall(player, 0), the unit limit (FN_GAME_LIMIT) */ \


#define ADDR_LISTS_FUNCTIONS(L) \


