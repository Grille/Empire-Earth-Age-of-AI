// THE GAME'S OBJECTS' FIELDS: every offset the mod reads in the game's objects, one row each: its name - the kind of
// object and what the field is (PL_TECH_TREE), or its offset in the 2017 update while nobody knows (PL_0xB10) - and
// its value in each exe build (k_exe_builds' columns, in that order, the 2017 update's first; tables.c makes the
// table of these rows). The name is the running build's offset: FIELD(pl, PL_TECH_TREE, void *); a read of a field
// not in the table doesn't build. A structure inside an object (a vector, a matrix, an array) is one row, its parts
// plain numbers from there in objects.c (PL_ZONES + 4 its end): they move with it. A kind has one row per offset
// (scripts/addr_sort.py --check). The kinds are below; the runtime library's layouts (std::deque, vector, map nodes,
// UString) and the mod's own structures stay plain numbers.
// Art of Conquest's column: scripts/port_fields.py (in the two exes' paired code, [this + X] read as [this + Y] in a
// class's methods; and every read's offsets for objects without methods), for the engine DLLs' objects (E_: AoC ships
// its own builds of them) scripts/port_dll.py; scripts/port_check.py checks a row in its object's own code (its source
// file, the pointer that reaches it, its constructors). How each value was found, the last column:
//   c  its class's code          b  between two fields of the kind with the same shift
//   v  the code's reads agree with a neighbouring field's shift     s  kept: no read of this object says otherwise
//   h  by hand, with its reason                                     -  not known yet (NA)
// The kinds:
//   PL           a player (EEPlayer, EEComputerPlayer)
//   UNIT         a unit, building or resource on the map (EEUnit, EEComplexUnit); its +0x30 is a unit too
//   TD           a unit type's data, G_TYPES[type] and a unit's +0x2C (EEUnitClass, EEUC*)
//   USTAT        a unit type's per-player values, a unit's +0x34 (range, attack ...)
//   GOAL         a unit's goal, its +0x1EC (EEUnitGoal, EEUG*)
//   CM           the AI's civilization manager, player +0xB90 (EECPCivilizationManager)
//   RM           the AI's resource manager (EECPResourceManager)
//   BM           the AI's build manager, CM +0x3C
//   UM           the AI's unit manager, CM +0x44
//   RS           the AI's research manager, CM +0x40 (EECPResearchManager)
//   AM           the AI's combat manager, player +0xB8C (EECPCombatManager)
//   AM4C         the object at the combat manager's +0x4C
//   AMCMD        the combat manager's commands, AM +0x48
//   AG           an attack group (EECPAttackGroup)
//   ASG          an attack subgroup (EECPAttackSubGroup, EECPASG*)
//   ASGU         an attack subgroup's unit record (EECPAttackSubGroupUnit, EECPASGU*)
//   ASGUINFO     what FN_ASGU_SCORE scores
//   STRATEGY     an attack strategy (EECPCombatStrategy)
//   TASK         an attack task (EECPAGTask)
//   ROUTEPLANNER the land attack route planner (EEAIGPGModifierLandAttackSubGroupRoutePlanner)
//   ROUTENODE    a route planner's node
//   TT           a player's tech tree, player +0x9D4 (EETechTree)
//   TECH         a tech, FN_TECH_LOOKUP
//   TECHNODE     a tech tree node (EETechTreeNode)
//   TECHDATA     a tech's data, TECH +8 (costs, its building ...)
//   TTREC        the tech tree's epoch records, TT + 8 + epoch * 0x54
//   EPOCHTECHS   G_EPOCH_TECHS' records
//   EFFECT       G_EFFECTS' records
//   ZONE         an AI town or resource zone
//   SITEOWN      a site's +0x14
//   CMLIST       the civilization manager's list nodes
//   MAP          the map, [G_MAP] (EEMap)
//   TILE         a map tile
//   GRID         the map's unit grid, MAP +0x181724
//   GRIDCELL     a cell of the unit grid
//   DMGMODEL     the object of FN_DAMAGE_POINTS
//   PLSTATS      a player's statistics, player +0x4A8 (EEPlayerStatistics)
//   PLB98        the object at a player's +0xB98
//   PHYS         the simulation, [G_WORLD_CLOCK] (EEPhysics)
//   WORLD        the world, [G_WORLD] (EEWorld)
//   SERVER       the lockstep server or client, [G_SERVER] (EEServer, EEClient)
//   GAMEDATA     the game data, [G_GAME_DATA]
//   SETUP        the game setup
//   SETUPSLOT    the game setup's player slots, 0x38 each
//   SCEN         the scenario manager (EEScenarioManager; test/scen.c)
//   TRIGGER      a scenario trigger (EESMTrigger)
//   TRIGCOND     a trigger's condition reference
//   TRIGEFFECT   a trigger's effect reference
//   SCENCOND     a scenario condition
//   SCENEFFECT   a scenario effect
//   DRAWREC      the draw list's records, DRAW_OLD (0x148 each)
//   CAMERA       the camera, G_CAMERA (EECameraControl)
//   SCROLL       the scroll action (EEAScroll)
//   MSG_TRIBUTE  the tribute message (EEMPayTribute)
//   MSG_CHAT     the in-game chat message (EEMInGameChat)
//   MSG_SELECT   the select message (EEMSelectUnits)
//   MSG_START    the game start message (EEMStartLoading)
//   NATIVECHAT   the object of FN_NATIVE_CHAT
//   UIFORM       the UI manager's current form
//   UISCREEN     a single-player menu screen (EEUIESP*)
//   MOUSE        the game cursor, [G_MOUSE] (RTSMouse) - not the DirectInput device (E_MOUSE)
//   E_*          the engine DLLs' objects: E_RAS the rasterizer (DX7Rasterizer), E_SCRSTATE its per-screen state,
//                E_SCREEN (DX7Screen), E_SURF (DX7Surface), E_DSTATE the rasterizer's +0x6C[0]; Low-Level Engine's
//                E_WINDOW (GEWindow), E_MOUSE (ISMouse), E_TEXT (GEText), E_FONT (GEFont), E_SOUNDMGR
//                (SESoundManager), E_SNDCAT its categories, E_SNDNODE (SESMInstanceNode), E_SAMPLE (SESample),
//                E_MIXER (SEMixer: the mixer plugin's functions)
#define NA 0xFFFFFFFFu   // a build hasn't got it, or it isn't known yet

// O(KIND_NAME, value per build)
#define FIELDS(O) \
    O(AG_SUBGROUPS, 0x8, 0x8) /* c */ \
    O(AG_TARGET_UNIT, 0x44, 0x44) /* b */ \
    O(AG_TARGET, 0x48, 0x48) /* b */ \
    O(AM_COMMANDER, 0x48, 0x48) /* c */ \
    O(AM_0x4C, 0x4C, 0x4C) /* b */ \
    O(AM_0x62, 0x62, 0x62) /* v */ \
    O(AM4C_0x48, 0x48, 0x48) /* s */ \
    O(AMCMD_GROUPS, 0x4, 0x4) /* s: the groups' deque (its count +0x10) */ \
    O(ASG_UNITS, 0x54, 0x54) /* c */ \
    O(ASG_ROUTE, 0x60, 0x60) /* c */ \
    O(ASG_WAYPOINTS, 0x70, 0x70) /* c */ \
    O(ASG_TARGET_REC, 0x100, 0x100) /* c */ \
    O(ASG_GOAL_X, 0x104, 0x104) /* c */ \
    O(ASG_GOAL_Y, 0x108, 0x108) /* c */ \
    O(ASG_TARGET_UNIT, 0x118, 0x118) /* c */ \
    O(ASG_TARGET, 0x11C, 0x11C) /* b */ \
    O(ASG_MISSION, 0x124, 0x124) /* c */ \
    O(ASG_PLAYER, 0x190, 0x194) /* c */ \
    O(ASG_STATE, 0x1D0, 0x1D4) /* c */ \
    O(ASG_X, 0x234, 0x238) /* c */ \
    O(ASG_Y, 0x238, 0x23C) /* c */ \
    O(ASGU_UNIT, 0x44, 0x44) /* c */ \
    O(ASGU_ENGAGED, 0x48, 0x48) /* c */ \
    O(ASGU_SUBGROUP, 0x4C, 0x4C) /* c */ \
    O(ASGUINFO_ENEMY, 0x4, 0x4) /* s */ \
    O(BM_QUEUES, 0x4, 0x4) /* c: the per-epoch build lists, 0x14 each - FN_BM_REQUEST 0x46802D / 0x46A9C1 */ \
    O(BM_BASE_X, 0x12C, 0x150) /* c: its file's code (+0x24, 5 votes) */ \
    O(BM_BASE_Y, 0x130, 0x154) /* c: its file's code (+0x24) */ \
    O(BM_OWNER, 0x134, 0x158) /* c: its file's code (+0x24, 18 votes) */ \
    O(BM_PLAYER, 0x138, 0x15C) /* c: its file's code (+0x24, 33 votes) */ \
    O(BM_INITIALIZED, 0x148, 0x16C) /* c: its file's code (+0x24) */ \
    O(CAMERA_POS, 0x78, 0x78) /* s */ \
    O(CAMERA_ZOOM, 0xE0, 0xE0) /* s */ \
    O(CM_PLAN, 0x8, 0x8) /* v */ \
    O(CM_RESOURCE_MGR, 0x38, 0x38) /* c */ \
    O(CM_BUILD_MGR, 0x3C, 0x3C) /* b */ \
    O(CM_RESEARCH_MGR, 0x40, 0x40) /* b */ \
    O(CM_UNIT_MGR, 0x44, 0x44) /* c */ \
    O(CM_WALL_MGR, 0x48, 0x48) /* c */ \
    O(CM_PLAYER, 0x4C, 0x4C) /* b: between +0x48 and +0x50, unchanged (0x46D209 reads it: the player) */ \
    O(CM_PROJECT, 0x50, 0x50) /* c */ \
    O(CM_TIMERS, 0x58, 0x58) /* v */ \
    O(CM_FLAG, 0x74, 0x78) /* c: its file's code (+4 from 0x74) */ \
    O(CM_SETUP_DONE, 0x75, 0x79) /* c: its file's code (+4 from 0x74) */ \
    O(CM_REQUEST_OUT, 0x76, 0x7A) /* c: its file's code (+4 from 0x74) */ \
    O(CM_0x78, 0x78, 0x7C) /* c: its file's code (+4 from 0x74) */ \
    O(CM_ATT_EPOCH_TECHS, 0x7A, 0x7E) /* c: its file's code (+4 from 0x74) */ \
    O(CM_ATT_BUILD, 0x7B, 0x7F) /* c: its file's code (+4 from 0x74) */ \
    O(CM_0x7C, 0x7C, 0x80) /* c: its file's code (+4 from 0x74) */ \
    O(CMLIST_NEXT, 0x0, 0x0) /* s */ \
    O(CMLIST_0xC, 0xC, 0xC) /* s */ \
    O(CMLIST_0x10, 0x10, 0x10) /* s */ \
    O(DMGMODEL_MAP, 0x14C, 0x14C) /* s */ \
    O(DRAWREC_0x58, 0x58, 0x58) /* h: the same in AoC: its draw list sites read DRAW_OLD + 0x58 and + 0xFC (0x50B2AD, 0x50EF07) */ \
    O(DRAWREC_0xFC, 0xFC, 0xFC) /* h: the same in AoC: its draw list sites read DRAW_OLD + 0x58 and + 0xFC (0x50B2AD, 0x50EF07) */ \
    O(EFFECT_TYPE, 0x14, 0x14) /* v: the tech tree code's reads (unchanged) */ \
    O(EFFECT_NEW_TYPE, 0x18, 0x18) /* v: the tech tree code's reads (unchanged) */ \
    O(EFFECT_KIND, 0x20, 0x20) /* v: the tech tree code's reads (unchanged) */ \
    O(EFFECT_ATTR, 0x2C, 0x2C) /* v: the tech tree code's reads (unchanged) */ \
    O(EPOCHTECHS_IDS, 0x4, 0x4) /* v: the tech tree code's reads (unchanged) */ \
    O(EPOCHTECHS_COUNT, 0x8, 0x8) /* v: the tech tree code's reads (unchanged) */ \
    O(E_DSTATE_DEVICE, 0x1C, 0x1C) /* c */ \
    O(E_FONT_HFONT, 0x18, 0x18) /* c */ \
    O(E_FONT_BITMAP, 0x24, 0x24) /* c */ \
    O(E_MIXER_SPIM, 0xB0, 0xB0) /* c */ \
    O(E_MOUSE_SUSPENDED, 0x4, 0x4) /* c */ \
    O(E_MOUSE_BUTTONS, 0x18, 0x18) /* c */ \
    O(E_MOUSE_DEVICE, 0x44, 0x44) /* c */ \
    O(E_RAS_SCREEN_STATES, 0x20, 0x20) /* c */ \
    O(E_RAS_DSTATES, 0x6C, 0x6C) /* c */ \
    O(E_SAMPLE_CHANNEL, 0x24, 0x24) /* c */ \
    O(E_SCREEN_MODES, 0x10, 0x10) /* c */ \
    O(E_SCREEN_DDRAW, 0x30, 0x30) /* c */ \
    O(E_SCREEN_MODE, 0x38, 0x38) /* c */ \
    O(E_SCREEN_DESKTOP, 0x3C, 0x3C) /* c */ \
    O(E_SCREEN_FULLSCREEN, 0x3D, 0x3D) /* c */ \
    O(E_SCRSTATE_SCREEN, 0x2A8, 0x2A8) /* c */ \
    O(E_SCRSTATE_FRONT, 0x2AC, 0x2AC) /* c */ \
    O(E_SCRSTATE_BACK, 0x2B0, 0x2B0) /* c */ \
    O(E_SCRSTATE_WINDOW, 0x2BC, 0x2BC) /* c */ \
    O(E_SCRSTATE_STATS, 0x2C4, 0x2C4) /* c */ \
    O(E_SNDCAT_SHARE, 0x0, 0x0) /* c */ \
    O(E_SNDCAT_MAX, 0x4, 0x4) /* c */ \
    O(E_SNDCAT_USED, 0x8, 0x8) /* c */ \
    O(E_SNDCAT_PRIORITY, 0xC, 0xC) /* c */ \
    O(E_SNDNODE_ID, 0xC, 0xC) /* c */ \
    O(E_SNDNODE_CHANNEL, 0x1C, 0x1C) /* c */ \
    O(E_SNDNODE_SAMPLE, 0x20, 0x20) /* c */ \
    O(E_SOUNDMGR_SLOTS, 0x24, 0x24) /* c */ \
    O(E_SOUNDMGR_PAUSED, 0x34, 0x34) /* c */ \
    O(E_SOUNDMGR_CATS, 0x54, 0x54) /* c */ \
    O(E_SOUNDMGR_MIXER, 0x70, 0x70) /* c */ \
    O(E_SOUNDMGR_CHANNELS, 0x88, 0x88) /* c */ \
    O(E_SURF_DDS, 0x48, 0x48) /* c */ \
    O(E_SURF_DC, 0x54, 0x54) /* c */ \
    O(E_SURF_DC_OUT, 0x5D, 0x5D) /* c */ \
    O(E_TEXT_STRING, 0x34, 0x34) /* c */ \
    O(E_TEXT_FONT, 0x44, 0x44) /* c */ \
    O(E_WINDOW_HWND, 0x0, 0x0) /* c */ \
    O(E_WINDOW_0x4, 0x4, 0x4) /* c */ \
    O(E_WINDOW_0x8, 0x8, 0x8) /* c */ \
    O(E_WINDOW_WIDTH, 0xC, 0xC) /* c */ \
    O(E_WINDOW_HEIGHT, 0x10, 0x10) /* c */ \
    O(GAMEDATA_LINE_MS, 0x2CC, 0x2CC) /* c: the code reading [G_GAME_DATA] */ \
    O(GAMEDATA_UNIT_LIMIT, 0x2E8, 0x2E8) /* c: the code reading [G_GAME_DATA] (the setup copy 0x524B8F) */ \
    O(GAMEDATA_UNIT_LIMIT_MAX, 0x2EC, 0x2EC) /* c: the code reading [G_GAME_DATA] (the setup copy 0x524B8F) */ \
    O(GOAL_STATE, 0x5, 0x5) /* b */ \
    O(GOAL_ARGS, 0x20, 0x20) /* c */ \
    O(GOAL_HOLDER, 0x50, 0x50) /* c */ \
    O(GRID_CELLS, 0x4, 0x4) /* s */ \
    O(GRID_CELL_SIZE, 0x8, 0x8) /* s */ \
    O(GRID_NX, 0x10, 0x10) /* s */ \
    O(GRID_NY, 0x14, 0x14) /* s */ \
    O(GRIDCELL_UNITS, 0x4, 0x4) /* s */ \
    O(MAP_TILES, 0x1816D4, 0x1955F0) /* c */ \
    O(MAP_SIZE, 0x1816FC, 0x195618) /* c */ \
    O(MAP_GRID, 0x181724, 0x195640) /* c */ \
    O(MAP_FLAGS, 0x181730, 0x19564C) /* c */ \
    O(MOUSE_X, 0x4, 0x4) /* c */ \
    O(MOUSE_Y, 0x8, 0x8) /* c */ \
    O(MOUSE_Y_MAX, 0x14, 0x14) /* s: as the 2017 update (were rows of RM, which this is not: not checked in AoC) */ \
    O(MOUSE_X_MIN, 0x18, 0x18) /* s: as the 2017 update (were rows of RM, which this is not: not checked in AoC) */ \
    O(MOUSE_X_MAX, 0x1C, 0x1C) /* s: as the 2017 update (were rows of RM, which this is not: not checked in AoC) */ \
    O(MOUSE_Y_MIN, 0x20, 0x20) /* s: as the 2017 update (were rows of RM, which this is not: not checked in AoC) */ \
    O(MOUSE_FROZEN, 0xA4, 0xA4) /* c */ \
    O(MSG_CHAT_FROM, 0xA, 0xA) /* b */ \
    O(MSG_CHAT_TEXT, 0x10, 0x10) /* c */ \
    O(MSG_SELECT_SELECTION, 0x10, 0x10) /* b */ \
    O(MSG_SELECT_IDS, 0x14, 0x14) /* b */ \
    O(MSG_SELECT_FLAG, 0x134, 0x134) /* c */ \
    O(MSG_START_SEED, 0x14, 0x14) /* c */ \
    O(MSG_TRIBUTE_FOOD, 0x10, 0x10) /* c */ \
    O(MSG_TRIBUTE_GOLD, 0x14, 0x14) /* c */ \
    O(MSG_TRIBUTE_IRON, 0x18, 0x18) /* c */ \
    O(MSG_TRIBUTE_STONE, 0x1C, 0x1C) /* c */ \
    O(MSG_TRIBUTE_WOOD, 0x20, 0x20) /* c */ \
    O(MSG_TRIBUTE_TO, 0x24, 0x24) /* v */ \
    O(NATIVECHAT_PLAYER, 0x20, 0x20) /* s */ \
    O(PHYS_SEED, 0x18, 0x18) /* c */ \
    O(PHYS_STEP, 0x80, 0x80) /* c */ \
    O(PHYS_STEP_S, 0x8C, 0x8C) /* c */ \
    O(PHYS_STEPS_GRANTED, 0xB8, 0xB8) /* c */ \
    O(PHYS_TIME, 0xD0, 0xD0) /* c */ \
    O(PL_LIST, 0x18, 0x18) /* c */ \
    O(PL_NAME, 0x24, 0x24) /* c */ \
    O(PL_COLOR, 0x438, 0x438) /* c */ \
    O(PL_INDEX, 0x45C, 0x45C) /* c */ \
    O(PL_MSG_ID, 0x464, 0x464) /* b */ \
    O(PL_EXPLORE_ALL, 0x465, 0x465) /* c */ \
    O(PL_OUT, 0x467, 0x467) /* c */ \
    O(PL_CIV_NAME, 0x488, 0x488) /* b */ \
    O(PL_STATS, 0x4A8, 0x4A8) /* c */ \
    O(PL_TYPE_STATS, 0x874, 0x884) /* b */ \
    O(PL_SELECTED_GROUP, 0x9BC, 0x9AC) /* c */ \
    O(PL_TECH_TREE, 0x9D4, 0x9CC) /* c */ \
    O(PL_CHAT_TO, 0x9E0, 0x9D8) /* c */ \
    O(PL_STANCE, 0x9E4, 0x9DC) /* b */ \
    O(PL_STRATEGY, 0xA28, 0xA20) /* c */ \
    O(PL_RESULT, 0xA30, 0xA28) /* c */ \
    O(PL_STOCK, 0xAF0, 0xAF4) /* c */ \
    O(PL_0xB10, 0xB10, 0xB14) /* c */ \
    O(PL_OBJECTS, 0xB14, 0xB18) /* c */ \
    O(PL_0xB1C, 0xB1C, 0xB20) /* c */ \
    O(PL_0xB20, 0xB20, 0xB24) /* c */ \
    O(PL_CHAT_TO_PLAYER, 0xB2C, 0xB30) /* b */ \
    O(PL_CIV_CHOSEN, 0xB46, 0xB4A) /* v */ \
    O(PL_ZONES, 0xB50, 0xB84) /* c */ \
    O(PL_COMBAT_MGR, 0xB8C, 0xBC0) /* c */ \
    O(PL_CIV_MGR, 0xB90, 0xBC4) /* c */ \
    O(PL_0xB98, 0xB98, 0xBCC) /* c */ \
    O(PL_DIFFICULTY, 0xBA4, 0xBD8) /* c */ \
    O(PL_COST_MEDIUM, 0xBB4, 0xBEC) /* c */ \
    O(PL_CITIZEN_SHARE, 0xBDC, 0xC14) /* b */ \
    O(PL_POP_CAP, 0xCCC, 0xD04) /* b */ \
    O(PL_ATTACK_ON_CONTACT, 0xCD8, 0xD10) /* b */ \
    O(PL_ATTACK_FOR_RESOURCES, 0xCDC, 0xD14) /* b */ \
    O(PL_ATTACK_REVENGE, 0xCE0, 0xD18) /* b */ \
    O(PL_ATTACK_MAX, 0xCFC, 0xD34) /* c */ \
    O(PL_ATTACK_MIN, 0xD00, 0xD38) /* c */ \
    O(PL_FIRST_ATTACK_MAX, 0xD28, 0xD60) /* b */ \
    O(PL_FIRST_ATTACK_MIN, 0xD2C, 0xD64) /* b */ \
    O(PL_COST_EASY, 0xD48, 0xD80) /* c */ \
    O(PL_COST_HARD, 0xD4C, 0xD84) /* c */ \
    O(PL_EPOCH_MIN, 0xD58, 0xD90) /* b */ \
    O(PL_EPOCH_MAX, 0xD5C, 0xD94) /* b */ \
    O(PL_DROPOFF_DISTANCE, 0xE10, 0xE4C) /* c */ \
    O(PL_ATTENTION, 0xE20, 0xE5C) /* v */ \
    O(PL_NEXT_ATTACK, 0xED8, 0xF28) /* c */ \
    O(PL_TOGGLES, 0xEF4, 0xF44) /* b */ \
    O(PL_FIRST_ATTACK_DONE, 0xEF5, 0xF45) /* c */ \
    O(PL_BOATS_FOR_FOOD, 0xEFA, 0xF4A) /* c */ \
    O(PL_CHEAT, 0xEFB, 0xF4B) /* c */ \
    O(PL_BUILD, 0xEFC, 0xF4C) /* b */ \
    O(PL_IGNORE, 0xF02, 0xF52) /* c */ \
    O(PLB98_ATT, 0x2D, 0x2D) /* c: read through a player's +0xB98 */ \
    O(PLSTATS_LISTS, 0x1C, 0x1C) /* b */ \
    O(RM_HUNT, 0x88, 0x88) /* c: its file's code (the resource manager) */ \
    O(RM_FISH, 0x98, 0x98) /* c: its file's code (the resource manager) */ \
    O(RM_LISTS, 0xA4, 0xA4) /* c: its file's code (the resource manager) */ \
    O(RM_OWNER, 0x130, 0x130) /* c: its file's code (the resource manager) */ \
    O(RM_DEMAND, 0x134, 0x134) /* c: its file's code (the resource manager) */ \
    O(RM_TARGET, 0x150, 0x150) /* c: its file's code (the resource manager) */ \
    O(RM_TIMERS, 0x16C, 0x16C) /* c: its file's code (the resource manager) */ \
    O(RM_INITIALIZED, 0x187, 0x187) /* c: its file's code (the resource manager) */ \
    O(RM_0x188, 0x188, 0x188) /* c: its file's code (the resource manager) */ \
    O(ROUTENODE_X, 0x3C, 0x3C) /* s */ \
    O(ROUTENODE_Y, 0x40, 0x40) /* s */ \
    O(ROUTEPLANNER_PLAYER_INDEX, 0x48, 0x48) /* c */ \
    O(RS_POOL, 0x8, 0x8) /* h: kept: the object's start, before its first known shift (0xF4 -> 0x104) */ \
    O(RS_EPOCH, 0x18, 0x18) /* h: kept: the object's start, before its first known shift (0xF4 -> 0x104) */ \
    O(RS_CURRENT, 0x114, 0x124) /* v */ \
    O(RS_INITIALIZED, 0x118, 0x128) /* v */ \
    O(RS_ATT_RANDOM, 0x11A, 0x12A) /* v */ \
    O(SCEN_TRIGGERS, 0xE8, 0xE8) /* c: its file's code (the scenario manager) */ \
    O(SCEN_CONDS, 0xF8, 0xF8) /* c: its file's code (the scenario manager) */ \
    O(SCEN_EFFECTS, 0x108, 0x108) /* c: its file's code (the scenario manager) */ \
    O(SCEN_OBJECTS, 0x118, 0x118) /* c: its file's code (the scenario manager) */ \
    O(SCEN_AREAS, 0x128, 0x128) /* c: its file's code (the scenario manager) */ \
    O(SCEN_TIME, 0x210, 0x210) /* c: its file's code (the scenario manager) */ \
    O(SCENCOND_TEXT, 0x2C, 0x2C) /* c: its file's code (the scenario manager) */ \
    O(SCENCOND_TYPE, 0x40, 0x40) /* c: its file's code (the scenario manager) */ \
    O(SCENCOND_OP, 0x44, 0x44) /* c: its file's code (the scenario manager) */ \
    O(SCENCOND_ATTR, 0x50, 0x50) /* c: its file's code (the scenario manager) */ \
    O(SCENCOND_TECH, 0x58, 0x58) /* c: its file's code (the scenario manager) */ \
    O(SCENCOND_VALUE, 0x5C, 0x5C) /* c: its file's code (the scenario manager) */ \
    O(SCENCOND_OBJECT, 0x60, 0x60) /* c: its file's code (the scenario manager) */ \
    O(SCENCOND_PLAYER, 0x64, 0x64) /* c: its file's code (the scenario manager) */ \
    O(SCENCOND_TIME_FROM, 0x6C, 0x6C) /* c: its file's code (the scenario manager) */ \
    O(SCENCOND_TIME_TO, 0x70, 0x70) /* c: its file's code (the scenario manager) */ \
    O(SCENCOND_TRIGGER, 0x78, 0x78) /* c: its file's code (the scenario manager) */ \
    O(SCENCOND_TRIGGER_ON, 0x80, 0x80) /* c: its file's code (the scenario manager) */ \
    O(SCENCOND_TRIGGER_FIRED, 0x81, 0x81) /* c: its file's code (the scenario manager) */ \
    O(SCENCOND_TIMED, 0x83, 0x83) /* c: its file's code (the scenario manager) */ \
    O(SCENEFFECT_AI_KIND, 0xE4, 0xE4) /* c: its file's code (the scenario manager) */ \
    O(SCENEFFECT_MEDIA_KIND, 0xF0, 0xF0) /* c: its file's code (the scenario manager) */ \
    O(SCENEFFECT_PLAYER_KIND, 0xF8, 0xF8) /* c: its file's code (the scenario manager) */ \
    O(SCENEFFECT_TRIGGER_KIND, 0xFC, 0xFC) /* c: its file's code (the scenario manager) */ \
    O(SCENEFFECT_CATEGORY, 0x100, 0x100) /* c: its file's code (the scenario manager) */ \
    O(SCENEFFECT_OBJECT_KIND, 0x104, 0x104) /* c: its file's code (the scenario manager) */ \
    O(SCENEFFECT_GAME_KIND, 0x13C, 0x13C) /* c: its file's code (the scenario manager) */ \
    O(SCENEFFECT_OBJECT, 0x158, 0x158) /* c: its file's code (the scenario manager) */ \
    O(SCENEFFECT_TRIGGER, 0x180, 0x180) /* h: kept, unsure (98 uses keep it, 49 say +4; only the test command scen reads it) */ \
    O(SCROLL_TICKS, 0x40, 0x40) /* v */ \
    O(SCROLL_KEYS, 0x44, 0x44) /* v */ \
    O(SERVER_SENT, 0x10, 0x10) /* b */ \
    O(SERVER_WAITING, 0x94, 0x94) /* c: the next tick's messages, NESynchronousServer (port_dll.py) */ \
    O(SERVER_DONE, 0x190, 0x190) /* b */ \
    O(SERVER_RATE_MAX, 0x1B4, 0x1B4) /* c: NESynchronousServer::TuneForSlowCPU (port_dll.py) */ \
    O(SERVER_RATE_MIN, 0x1B8, 0x1B8) /* c: NESynchronousServer::TuneForSlowCPU (port_dll.py) */ \
    O(SERVER_RATE, 0x1FB, 0x1FB) /* c: NESynchronousServer::TuneForSlowCPU (port_dll.py) */ \
    O(SETUP_MAP_NAME, 0x3A8, 0x3A8) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP_0x3D4, 0x3D4, 0x3D4) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP_MODE, 0x3D8, 0x3D8) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP_MAP_SCALE, 0x3DC, 0x3DC) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP_START_EPOCH, 0x3E0, 0x3E0) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP_END_EPOCH, 0x3E4, 0x3E4) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP_MAP_SIZE, 0x3E8, 0x3E8) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP_0x3EC, 0x3EC, 0x3EC) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP_UNIT_LIMIT, 0x3F0, 0x3F0) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP_0x3F4, 0x3F4, 0x3F4) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP_DIFFICULTY, 0x3F8, 0x3F8) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP_0x3FC, 0x3FC, 0x3FC) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP_FLAGS, 0x400, 0x400) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUPSLOT_TYPE, 0x0, 0x0) /* c: the setup code */ \
    O(SETUPSLOT_0x4, 0x4, 0x4) /* s: as the slot's others (engine-notes.md "Game start") */ \
    O(SETUPSLOT_NAME, 0x8, 0x8) /* s: the player's name, as the slot's others */ \
    O(SETUPSLOT_0x1C, 0x1C, 0x1C) /* s: as the slot's others (engine-notes.md "Game start") */ \
    O(SETUPSLOT_INDEX, 0x20, 0x20) /* s: as the slot's others (engine-notes.md "Game start") */ \
    O(SETUPSLOT_COLOR, 0x24, 0x24) /* c: the setup code */ \
    O(SETUPSLOT_TEAM, 0x28, 0x28) /* s: as the slot's others (engine-notes.md "Game start") */ \
    O(SETUPSLOT_CITIZENS, 0x2C, 0x2C) /* s: as the slot's others (engine-notes.md "Game start") */ \
    O(SETUPSLOT_0x30, 0x30, 0x30) /* s: as the slot's others (engine-notes.md "Game start") */ \
    O(SETUPSLOT_FLAGS, 0x34, 0x34) /* s: as the slot's others (engine-notes.md "Game start") */ \
    O(SETUPSLOT_SIZE, 0x38, 0x38) /* s: a slot's size, the next slot */ \
    O(SITEOWN_FLAG, 0x4, 0x4) /* s */ \
    O(STRATEGY_0x3C, 0x3C, 0x3C) /* s */ \
    O(STRATEGY_0x40, 0x40, 0x40) /* s */ \
    O(TASK_TARGET_PLAYER, 0x20, 0x20) /* c */ \
    O(TASK_0x28, 0x28, 0x28) /* c */ \
    O(TASK_0x2C, 0x2C, 0x2C) /* c */ \
    O(TASK_0x30, 0x30, 0x30) /* c */ \
    O(TD_VTABLE, 0x0, 0x0) /* c */ \
    O(TD_NAME, 0x1C, 0x1C) /* v */ \
    O(TD_PROJECTILE, 0x3FC, 0xF8) /* c */ \
    O(TD_MIN_RANGE, 0x41C, 0x120) /* c: the type data's constructors */ \
    O(TD_RANGE, 0x420, 0x124) /* c: the type data's constructors */ \
    O(TD_AREA, 0x424, 0x128) /* c: the type data's constructors */ \
    O(TD_0x42C, 0x42C, 0x130) /* c: the type data's constructors */ \
    O(TD_0x430, 0x430, 0x134) /* c: the type data's constructors */ \
    O(TD_ACCEL, 0x438, 0x13C) /* c: the type data's constructors */ \
    O(TD_FAMILY, 0x4B4, 0x1B8) /* c */ \
    O(TD_ID, 0x4B8, 0x1BC) /* c */ \
    O(TD_DAMAGE_CLASS, 0x4C0, 0x1C4) /* c: the type data's constructors */ \
    O(TD_CATEGORY, 0x4C4, 0x1C8) /* c: the type data's constructors */ \
    O(TD_WEAPON, 0x4C8, 0x1CC) /* c: the type data's constructors */ \
    O(TD_TECH, 0x4E0, 0x1E4) /* c: the type data's constructors */ \
    O(TD_RELOAD, 0x50C, 0x210) /* c: the type data's constructors */ \
    O(TD_ATTACK, 0x510, 0x214) /* c: the type data's constructors */ \
    O(TD_TRANSPORT_CAPACITY, 0x54C, 0x24C) /* c: the type data's constructors */ \
    O(TD_GARRISON_CAPACITY, 0x550, 0x250) /* c: the type data's constructors */ \
    O(TD_HP, 0x55C, 0x25C) /* c: the type data's constructors */ \
    O(TD_MISSILE_DELAY, 0x588, 0x288) /* c: the type data's constructors */ \
    O(TD_SIZE, 0x58C, 0x28C) /* c */ \
    O(TD_BEHAVIOURS, 0x621, 0x31F) /* c: the type data's constructors */ \
    O(TECH_RESEARCHED, 0x4, 0x4) /* c: researched - EETechTreeNode::vf6 sets it (0x5BF335 / 0x5CFA53) */ \
    O(TECH_AVAILABLE, 0x6, 0x6) /* c: available - vf1 and vf6 test it */ \
    O(TECH_DATA, 0x8, 0x8) /* v: the tech tree code's reads (unchanged) */ \
    O(TECH_EPOCH, 0x14, 0x14) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA_MAKERS, 0x10, 0x10) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA_COSTS, 0x24, 0x24) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA_CATEGORY, 0x48, 0x48) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA_ID, 0x4C, 0x4C) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA_EFFECTS, 0x50, 0x50) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA_UNIT_TYPE, 0x6C, 0x6C) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA_COUNTS, 0x7D, 0x7D) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA_AOC_NEW, 0x81, 0x81) /* h: AoC's epoch upgrades (standard only) grant only a tech with it set: dbtechtree byte 0xF7, clear on civ power and special unit upgrades (0 on all of the 2017 update's) - read in AoC only */ \
    O(TECHNODE_TREE, 0xC, 0xC) /* c */ \
    O(TECHNODE_KIDS, 0x24, 0x24) /* v */ \
    O(TILE_HEIGHT, 0x10, 0x10) /* c */ \
    O(TRIGCOND_INDEX, 0x4, 0x4) /* c: its file's code (the scenario manager) */ \
    O(TRIGCOND_JOIN, 0x8, 0x8) /* c: its file's code (the scenario manager) */ \
    O(TRIGCOND_NOT, 0xC, 0xC) /* c: its file's code (the scenario manager) */ \
    O(TRIGEFFECT_KIND, 0x4, 0x4) /* c: its file's code (the scenario manager) */ \
    O(TRIGEFFECT_DELAY, 0x8, 0x8) /* c: its file's code (the scenario manager) */ \
    O(TRIGEFFECT_INDEX, 0xC, 0xC) /* c: its file's code (the scenario manager) */ \
    O(TRIGGER_NAME, 0x4, 0x4) /* c: its file's code (the scenario manager) */ \
    O(TRIGGER_DESC, 0x18, 0x18) /* c: its file's code (the scenario manager) */ \
    O(TRIGGER_CONDS, 0x30, 0x30) /* s: a trigger's condition references, as its +0x40 */ \
    O(TRIGGER_EFFECTS, 0x40, 0x40) /* c: its file's code (the scenario manager) */ \
    O(TRIGGER_TRUE_FOR, 0x4C, 0x4C) /* c: its file's code (the scenario manager) */ \
    O(TRIGGER_SINCE, 0x50, 0x50) /* c: its file's code (the scenario manager) */ \
    O(TRIGGER_ON, 0x58, 0x58) /* c: its file's code (the scenario manager) */ \
    O(TRIGGER_FIRED, 0x59, 0x59) /* c: its file's code (the scenario manager) */ \
    O(TRIGGER_LOOP, 0x5A, 0x5A) /* c: its file's code (the scenario manager) */ \
    O(TT_PLAYER_INDEX, 0x4, 0x4) /* v */ \
    O(TT_EPOCH_RECS, 0x8, 0x8) /* s: the epoch records, 0x54 apart (AoC: one more, its 0x4E4 one record on) */ \
    O(TT_EPOCH, 0x4E4, 0x538) /* v */ \
    O(TTREC_TECH_DATA, 0x8, 0x8) /* v: the tech tree code's reads (unchanged) */ \
    O(TTREC_NEEDED, 0x40, 0x40) /* v: the tech tree code's reads (unchanged) */ \
    O(TTREC_DONE, 0x48, 0x48) /* v: the tech tree code's reads (unchanged) */ \
    O(UIFORM_TOP_BAR, 0x5D8, 0x5E8) /* h: votes 6/7 */ \
    O(UISCREEN_CONTROLS, 0xF8, 0xF8) /* v */ \
    O(UM_REQUESTS, 0xC, 0xC) /* c: its file's code (the unit manager) */ \
    O(UM_PLAYER, 0x134, 0x148) /* c: its file's code (+0x14, 31 votes) */ \
    O(UM_IN_PRODUCTION, 0x13C, 0x150) /* c: its file's code (+0x14) */ \
    O(UM_0x140, 0x140, 0x154) /* c: its file's code (+0x14) */ \
    O(UNIT_REFS, 0x4, 0x4) /* c */ \
    O(UNIT_LIST_NODE, 0x8, 0x8) /* c */ \
    O(UNIT_VIS, 0x14, 0x14) /* c */ \
    O(UNIT_OWNER, 0x18, 0x18) /* c */ \
    O(UNIT_X, 0x1C, 0x1C) /* c */ \
    O(UNIT_Y, 0x20, 0x20) /* c */ \
    O(UNIT_SERIAL, 0x24, 0x24) /* c */ \
    O(UNIT_RELEASED, 0x2A, 0x2A) /* c */ \
    O(UNIT_VALID, 0x2B, 0x2B) /* c */ \
    O(UNIT_TYPE_DATA, 0x2C, 0x2C) /* c */ \
    O(UNIT_PHYSICS, 0x30, 0x30) /* c */ \
    O(UNIT_STATS, 0x34, 0x34) /* c */ \
    O(UNIT_RESOURCE, 0x38, 0x38) /* c */ \
    O(UNIT_HP, 0x3C, 0x3C) /* c */ \
    O(UNIT_AMOUNT, 0x40, 0x40) /* c */ \
    O(UNIT_QUEUE, 0x4C, 0x4C) /* c */ \
    O(UNIT_ID, 0x60, 0x60) /* c */ \
    O(UNIT_BUSY, 0x68, 0x68) /* c */ \
    O(UNIT_MATRIX, 0x130, 0x130) /* b: the unit's matrix (GETransformation), its +0x4 .. +0x2C between fields of the same shift */ \
    O(UNIT_ATTACKER_TYPE, 0x1C0, 0x1C0) /* c */ \
    O(UNIT_CHAIN, 0x1CC, 0x1CC) /* c */ \
    O(UNIT_GOAL, 0x1EC, 0x1EC) /* c */ \
    O(UNIT_GOAL_OBJECT, 0x1F0, 0x1F0) /* c */ \
    O(UNIT_BEHAVIOUR, 0x208, 0x208) /* c */ \
    O(UNIT_SPEED_MIN, 0x21C, 0x21C) /* c */ \
    O(UNIT_SPEED, 0x238, 0x238) /* c */ \
    O(UNIT_PRODUCING, 0x25C, 0x260) /* c */ \
    O(UNIT_FLIGHT_MS, 0x28C, 0x290) /* c */ \
    O(UNIT_RELOAD, 0x2A0, 0x2A4) /* c */ \
    O(UNIT_FROZEN, 0x33B, 0x347) /* c */ \
    O(UNIT_FINISHED, 0x340, 0x34C) /* c */ \
    O(UNIT_AI_HELD, 0x341, 0x34D) /* c */ \
    O(USTAT_TYPE_DATA, 0x60, 0x60) /* b: between fields read through a unit's +0x34, unchanged */ \
    O(USTAT_HEIGHT, 0x98, 0x98) /* c: read through a unit's +0x34 */ \
    O(USTAT_RANGE, 0x9C, 0x9C) /* c: read through a unit's +0x34 */ \
    O(USTAT_AREA, 0xA0, 0xA0) /* c: read through a unit's +0x34 */ \
    O(USTAT_SPEED, 0xAC, 0xAC) /* c: read through a unit's +0x34 */ \
    O(USTAT_SHOT_SPEED, 0xC0, 0xC0) /* b: between fields read through a unit's +0x34, unchanged */ \
    O(USTAT_ATTACK, 0xD8, 0xD8) /* c: read through a unit's +0x34 */ \
    O(USTAT_ARMOR, 0xE0, 0xE0) /* b: between fields read through a unit's +0x34, unchanged */ \
    O(USTAT_RELOAD, 0x138, 0x138) /* c: read through a unit's +0x34 */ \
    O(USTAT_HP, 0x144, 0x144) /* c: read through a unit's +0x34 */ \
    O(WORLD_SEED, 0x54, 0x54) /* b */ \
    O(ZONE_SITES, 0x8, 0x8) /* s */ \
    O(ZONE_DROP, 0x14, 0x14) /* s */ \
    O(ZONE_ACTIVE, 0x18, 0x18) /* s */ \

