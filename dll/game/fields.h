// THE GAME'S OBJECTS' FIELDS: every offset the mod reads in the game's objects, by the kind of object and the offset
// it has in the 2017 update, with its value in each exe build (addr.h's columns, in that order). OF(PL, 0x9D4) is the
// running build's offset of the field the 2017 update has at +0x9D4 in a player: FIELD(pl, OF(PL, 0x9D4), void *). A
// read of a field not in the table doesn't build. The kinds are below; the runtime library's layouts (std::deque,
// vector, map nodes, UString) and the mod's own structures stay plain numbers.
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
//   TT           a player's tech tree, player +PL_TECHTREE (EETechTree)
//   TECH         a tech, FN_TECH_LOOKUP
//   TECHNODE     a tech tree node (EETechTreeNode)
//   TECHDATA     a tech's data, TECH +8 (costs, its building ...)
//   TECHCHILD    a tech node's children
//   TTREC        the tech tree's epoch records, TT + 8 + epoch * 0x54
//   EPOCHTECHS   G_EPOCH_TECHS' records
//   EFFECT       G_EFFECTS' records
//   ZONE         an AI town or resource zone
//   SITEOWN      a site's +0x14
//   CMLIST       the civilization manager's list nodes
//   MAP          the map, [G_MAP] (EEMap)
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
//   SCEN         the scenario manager and its triggers (test/scen.c)
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

// O(kind, offset in the 2017 update, value per build)
#define FIELDS(O) \
    O(AG, 0x8, 0x8, 0x8) /* c */ \
    O(AG, 0xC, 0xC, 0xC) /* c */ \
    O(AG, 0x20, 0x20, 0x20) /* b */ \
    O(AG, 0x24, 0x24, 0x24) /* c */ \
    O(AG, 0x30, 0x30, 0x30) /* b */ \
    O(AG, 0x44, 0x44, 0x44) /* b */ \
    O(AG, 0x48, 0x48, 0x48) /* b */ \
    O(AM, 0x48, 0x48, 0x48) /* c */ \
    O(AM, 0x4C, 0x4C, 0x4C) /* b */ \
    O(AM, 0x62, 0x62, 0x62) /* v */ \
    O(AM4C, 0x48, 0x48, 0x48) /* s */ \
    O(AMCMD, 0x14, 0x14, 0x14) /* s */ \
    O(ASG, 0x54, 0x54, 0x54) /* c */ \
    O(ASG, 0x58, 0x58, 0x58) /* c */ \
    O(ASG, 0x60, 0x60, 0x60) /* c */ \
    O(ASG, 0x70, 0x70, 0x70) /* c */ \
    O(ASG, 0x100, 0x100, 0x100) /* c */ \
    O(ASG, 0x104, 0x104, 0x104) /* c */ \
    O(ASG, 0x108, 0x108, 0x108) /* c */ \
    O(ASG, 0x118, 0x118, 0x118) /* c */ \
    O(ASG, 0x11C, 0x11C, 0x11C) /* b */ \
    O(ASG, 0x124, 0x124, 0x124) /* c */ \
    O(ASG, 0x190, 0x190, 0x194) /* c */ \
    O(ASG, 0x1D0, 0x1D0, 0x1D4) /* c */ \
    O(ASG, 0x234, 0x234, 0x238) /* c */ \
    O(ASG, 0x238, 0x238, 0x23C) /* c */ \
    O(ASGU, 0x44, 0x44, 0x44) /* c */ \
    O(ASGU, 0x48, 0x48, 0x48) /* c */ \
    O(ASGU, 0x4C, 0x4C, 0x4C) /* c */ \
    O(ASGUINFO, 0x4, 0x4, 0x4) /* s */ \
    O(BM, 0x12C, 0x12C, 0x150) /* c: its file's code (+0x24, 5 votes) */ \
    O(BM, 0x130, 0x130, 0x154) /* c: its file's code (+0x24) */ \
    O(BM, 0x134, 0x134, 0x158) /* c: its file's code (+0x24, 18 votes) */ \
    O(BM, 0x138, 0x138, 0x15C) /* c: its file's code (+0x24, 33 votes) */ \
    O(BM, 0x148, 0x148, 0x16C) /* c: its file's code (+0x24) */ \
    O(CAMERA, 0x78, 0x78, 0x78) /* s */ \
    O(CAMERA, 0x7C, 0x7C, 0x7C) /* s */ \
    O(CAMERA, 0xE0, 0xE0, 0xE0) /* s */ \
    O(CM, 0x8, 0x8, 0x8) /* v */ \
    O(CM, 0x38, 0x38, 0x38) /* c */ \
    O(CM, 0x3C, 0x3C, 0x3C) /* b */ \
    O(CM, 0x40, 0x40, 0x40) /* b */ \
    O(CM, 0x44, 0x44, 0x44) /* c */ \
    O(CM, 0x48, 0x48, 0x48) /* c */ \
    O(CM, 0x50, 0x50, 0x50) /* c */ \
    O(CM, 0x58, 0x58, 0x58) /* v */ \
    O(CM, 0x74, 0x74, 0x78) /* c: its file's code (+4 from 0x74) */ \
    O(CM, 0x75, 0x75, 0x79) /* c: its file's code (+4 from 0x74) */ \
    O(CM, 0x76, 0x76, 0x7A) /* c: its file's code (+4 from 0x74) */ \
    O(CM, 0x78, 0x78, 0x7C) /* c: its file's code (+4 from 0x74) */ \
    O(CM, 0x7A, 0x7A, 0x7E) /* c: its file's code (+4 from 0x74) */ \
    O(CM, 0x7B, 0x7B, 0x7F) /* c: its file's code (+4 from 0x74) */ \
    O(CM, 0x7C, 0x7C, 0x80) /* c: its file's code (+4 from 0x74) */ \
    O(CMLIST, 0x0, 0x0, 0x0) /* s */ \
    O(CMLIST, 0xC, 0xC, 0xC) /* s */ \
    O(CMLIST, 0x10, 0x10, 0x10) /* s */ \
    O(DMGMODEL, 0x14C, 0x14C, 0x14C) /* s */ \
    O(DRAWREC, 0x58, 0x58, 0x58) /* h: the same in AoC: its draw list sites read DRAW_OLD + 0x58 and + 0xFC (0x50B2AD, 0x50EF07) */ \
    O(DRAWREC, 0xFC, 0xFC, 0xFC) /* h: the same in AoC: its draw list sites read DRAW_OLD + 0x58 and + 0xFC (0x50B2AD, 0x50EF07) */ \
    O(EFFECT, 0x14, 0x14, 0x14) /* v: the tech tree code's reads (unchanged) */ \
    O(EFFECT, 0x18, 0x18, 0x18) /* v: the tech tree code's reads (unchanged) */ \
    O(EFFECT, 0x20, 0x20, 0x20) /* v: the tech tree code's reads (unchanged) */ \
    O(EFFECT, 0x2C, 0x2C, 0x2C) /* v: the tech tree code's reads (unchanged) */ \
    O(EPOCHTECHS, 0x4, 0x4, 0x4) /* v: the tech tree code's reads (unchanged) */ \
    O(EPOCHTECHS, 0x8, 0x8, 0x8) /* v: the tech tree code's reads (unchanged) */ \
    O(EPOCHTECHS, 0xC, 0xC, 0xC) /* v: the tech tree code's reads (unchanged) */ \
    O(EPOCHTECHS, 0x10, 0x10, 0x10) /* v: the tech tree code's reads (unchanged) */ \
    O(E_DSTATE, 0x1C, 0x1C, 0x1C) /* c */ \
    O(E_FONT, 0x18, 0x18, 0x18) /* c */ \
    O(E_FONT, 0x24, 0x24, 0x24) /* c */ \
    O(E_MIXER, 0xB0, 0xB0, 0xB0) /* c */ \
    O(E_MIXER, 0xD0, 0xD0, 0xD0) /* c */ \
    O(E_MIXER, 0xD8, 0xD8, 0xD8) /* c */ \
    O(E_MIXER, 0xF0, 0xF0, 0xF0) /* c */ \
    O(E_MOUSE, 0x4, 0x4, 0x4) /* c */ \
    O(E_MOUSE, 0x18, 0x18, 0x18) /* c */ \
    O(E_MOUSE, 0x44, 0x44, 0x44) /* c */ \
    O(E_RAS, 0x20, 0x20, 0x20) /* c */ \
    O(E_RAS, 0x6C, 0x6C, 0x6C) /* c */ \
    O(E_SAMPLE, 0x24, 0x24, 0x24) /* c */ \
    O(E_SCREEN, 0x10, 0x10, 0x10) /* c */ \
    O(E_SCREEN, 0x14, 0x14, 0x14) /* c */ \
    O(E_SCREEN, 0x30, 0x30, 0x30) /* c */ \
    O(E_SCREEN, 0x38, 0x38, 0x38) /* c */ \
    O(E_SCREEN, 0x3C, 0x3C, 0x3C) /* c */ \
    O(E_SCREEN, 0x3D, 0x3D, 0x3D) /* c */ \
    O(E_SCRSTATE, 0x2A8, 0x2A8, 0x2A8) /* c */ \
    O(E_SCRSTATE, 0x2AC, 0x2AC, 0x2AC) /* c */ \
    O(E_SCRSTATE, 0x2B0, 0x2B0, 0x2B0) /* c */ \
    O(E_SCRSTATE, 0x2BC, 0x2BC, 0x2BC) /* c */ \
    O(E_SCRSTATE, 0x2C4, 0x2C4, 0x2C4) /* c */ \
    O(E_SCRSTATE, 0x2C8, 0x2C8, 0x2C8) /* c */ \
    O(E_SCRSTATE, 0x2CC, 0x2CC, 0x2CC) /* c */ \
    O(E_SCRSTATE, 0x2D0, 0x2D0, 0x2D0) /* c */ \
    O(E_SCRSTATE, 0x2D4, 0x2D4, 0x2D4) /* c */ \
    O(E_SCRSTATE, 0x2D8, 0x2D8, 0x2D8) /* c */ \
    O(E_SCRSTATE, 0x2DC, 0x2DC, 0x2DC) /* c */ \
    O(E_SCRSTATE, 0x2E0, 0x2E0, 0x2E0) /* c */ \
    O(E_SCRSTATE, 0x2E4, 0x2E4, 0x2E4) /* c */ \
    O(E_SCRSTATE, 0x328, 0x328, 0x328) /* c */ \
    O(E_SCRSTATE, 0x32C, 0x32C, 0x32C) /* c */ \
    O(E_SCRSTATE, 0x330, 0x330, 0x330) /* c */ \
    O(E_SCRSTATE, 0x334, 0x334, 0x334) /* c */ \
    O(E_SCRSTATE, 0x338, 0x338, 0x338) /* c */ \
    O(E_SNDCAT, 0x0, 0x0, 0x0) /* c */ \
    O(E_SNDCAT, 0x4, 0x4, 0x4) /* c */ \
    O(E_SNDCAT, 0x8, 0x8, 0x8) /* c */ \
    O(E_SNDCAT, 0xC, 0xC, 0xC) /* c */ \
    O(E_SNDNODE, 0xC, 0xC, 0xC) /* c */ \
    O(E_SNDNODE, 0x1C, 0x1C, 0x1C) /* c */ \
    O(E_SNDNODE, 0x20, 0x20, 0x20) /* c */ \
    O(E_SOUNDMGR, 0x24, 0x24, 0x24) /* c */ \
    O(E_SOUNDMGR, 0x34, 0x34, 0x34) /* c */ \
    O(E_SOUNDMGR, 0x38, 0x38, 0x38) /* c */ \
    O(E_SOUNDMGR, 0x54, 0x54, 0x54) /* c */ \
    O(E_SOUNDMGR, 0x58, 0x58, 0x58) /* c */ \
    O(E_SOUNDMGR, 0x70, 0x70, 0x70) /* c */ \
    O(E_SOUNDMGR, 0x88, 0x88, 0x88) /* c */ \
    O(E_SURF, 0x48, 0x48, 0x48) /* c */ \
    O(E_SURF, 0x54, 0x54, 0x54) /* c */ \
    O(E_SURF, 0x5D, 0x5D, 0x5D) /* c */ \
    O(E_TEXT, 0x34, 0x34, 0x34) /* c */ \
    O(E_TEXT, 0x44, 0x44, 0x44) /* c */ \
    O(E_WINDOW, 0x0, 0x0, 0x0) /* c */ \
    O(E_WINDOW, 0x4, 0x4, 0x4) /* c */ \
    O(E_WINDOW, 0x8, 0x8, 0x8) /* c */ \
    O(E_WINDOW, 0xC, 0xC, 0xC) /* c */ \
    O(E_WINDOW, 0x10, 0x10, 0x10) /* c */ \
    O(GAMEDATA, 0x2CC, 0x2CC, 0x2CC) /* c: the code reading [G_GAME_DATA] */ \
    O(GAMEDATA, 0x2E8, 0x2E8, 0x2E8) /* c: the code reading [G_GAME_DATA] (the setup copy 0x524B8F) */ \
    O(GAMEDATA, 0x2EC, 0x2EC, 0x2EC) /* c: the code reading [G_GAME_DATA] (the setup copy 0x524B8F) */ \
    O(GOAL, 0x5, 0x5, 0x5) /* b */ \
    O(GOAL, 0x8, 0x8, 0x8) /* c */ \
    O(GOAL, 0xC, 0xC, 0xC) /* c */ \
    O(GOAL, 0x20, 0x20, 0x20) /* c */ \
    O(GOAL, 0x24, 0x24, 0x24) /* c */ \
    O(GOAL, 0x28, 0x28, 0x28) /* c */ \
    O(GOAL, 0x2C, 0x2C, 0x2C) /* c */ \
    O(GOAL, 0x30, 0x30, 0x30) /* c */ \
    O(GOAL, 0x34, 0x34, 0x34) /* c */ \
    O(GOAL, 0x38, 0x38, 0x38) /* c */ \
    O(GOAL, 0x3C, 0x3C, 0x3C) /* c */ \
    O(GOAL, 0x48, 0x48, 0x48) /* c */ \
    O(GOAL, 0x50, 0x50, 0x50) /* c */ \
    O(GRID, 0x4, 0x4, 0x4) /* s */ \
    O(GRID, 0x8, 0x8, 0x8) /* s */ \
    O(GRID, 0x10, 0x10, 0x10) /* s */ \
    O(GRID, 0x14, 0x14, 0x14) /* s */ \
    O(GRIDCELL, 0x4, 0x4, 0x4) /* s */ \
    O(GRIDCELL, 0x8, 0x8, 0x8) /* s */ \
    O(MAP, 0x4, 0x4, 0x4) /* b */ \
    O(MAP, 0x1816D4, 0x1816D4, 0x1955F0) /* c */ \
    O(MAP, 0x1816FC, 0x1816FC, 0x195618) /* c */ \
    O(MAP, 0x181724, 0x181724, 0x195640) /* c */ \
    O(MAP, 0x181730, 0x181730, 0x19564C) /* c */ \
    O(MOUSE, 0x4, 0x4, 0x4) /* c */ \
    O(MOUSE, 0x8, 0x8, 0x8) /* c */ \
    O(MOUSE, 0xA4, 0xA4, 0xA4) /* c */ \
    O(MSG_CHAT, 0xA, 0xA, 0xA) /* b */ \
    O(MSG_CHAT, 0x10, 0x10, 0x10) /* c */ \
    O(MSG_SELECT, 0x10, 0x10, 0x10) /* b */ \
    O(MSG_SELECT, 0x14, 0x14, 0x14) /* b */ \
    O(MSG_SELECT, 0x18, 0x18, 0x18) /* b */ \
    O(MSG_SELECT, 0x134, 0x134, 0x134) /* c */ \
    O(MSG_START, 0x14, 0x14, 0x14) /* c */ \
    O(MSG_TRIBUTE, 0xA, 0xA, 0xA) /* b */ \
    O(MSG_TRIBUTE, 0x10, 0x10, 0x10) /* c */ \
    O(MSG_TRIBUTE, 0x14, 0x14, 0x14) /* c */ \
    O(MSG_TRIBUTE, 0x18, 0x18, 0x18) /* c */ \
    O(MSG_TRIBUTE, 0x1C, 0x1C, 0x1C) /* c */ \
    O(MSG_TRIBUTE, 0x20, 0x20, 0x20) /* c */ \
    O(MSG_TRIBUTE, 0x24, 0x24, 0x24) /* v */ \
    O(NATIVECHAT, 0x20, 0x20, 0x20) /* s */ \
    O(PHYS, 0x18, 0x18, 0x18) /* c */ \
    O(PHYS, 0x80, 0x80, 0x80) /* c */ \
    O(PHYS, 0x8C, 0x8C, 0x8C) /* c */ \
    O(PHYS, 0xB8, 0xB8, 0xB8) /* c */ \
    O(PHYS, 0xD0, 0xD0, 0xD0) /* c */ \
    O(PL, 0x4, 0x4, 0x4) /* c */ \
    O(PL, 0x18, 0x18, 0x18) /* c */ \
    O(PL, 0x1C, 0x1C, 0x1C) /* c */ \
    O(PL, 0x20, 0x20, 0x20) /* b */ \
    O(PL, 0x24, 0x24, 0x24) /* c */ \
    O(PL, 0x28, 0x28, 0x28) /* b */ \
    O(PL, 0x2C, 0x2C, 0x2C) /* b */ \
    O(PL, 0x34, 0x34, 0x34) /* b */ \
    O(PL, 0x4C, 0x4C, 0x4C) /* b */ \
    O(PL, 0x438, 0x438, 0x438) /* c */ \
    O(PL, 0x45C, 0x45C, 0x45C) /* c */ \
    O(PL, 0x464, 0x464, 0x464) /* b */ \
    O(PL, 0x465, 0x465, 0x465) /* c */ \
    O(PL, 0x467, 0x467, 0x467) /* c */ \
    O(PL, 0x488, 0x488, 0x488) /* b */ \
    O(PL, 0x4A8, 0x4A8, 0x4A8) /* c */ \
    O(PL, 0x874, 0x874, 0x884) /* b */ \
    O(PL, 0x9BC, 0x9BC, 0x9AC) /* c */ \
    O(PL, 0x9D4, 0x9D4, 0x9CC) /* c */ \
    O(PL, 0x9E0, 0x9E0, 0x9D8) /* c */ \
    O(PL, 0x9E4, 0x9E4, 0x9DC) /* b */ \
    O(PL, 0xA28, 0xA28, 0xA20) /* c */ \
    O(PL, 0xA30, 0xA30, 0xA28) /* c */ \
    O(PL, 0xAF0, 0xAF0, 0xAF4) /* c */ \
    O(PL, 0xB10, 0xB10, 0xB14) /* c */ \
    O(PL, 0xB14, 0xB14, 0xB18) /* c */ \
    O(PL, 0xB1C, 0xB1C, 0xB20) /* c */ \
    O(PL, 0xB20, 0xB20, 0xB24) /* c */ \
    O(PL, 0xB2C, 0xB2C, 0xB30) /* b */ \
    O(PL, 0xB46, 0xB46, 0xB4A) /* v */ \
    O(PL, 0xB50, 0xB50, 0xB84) /* c */ \
    O(PL, 0xB54, 0xB54, 0xB88) /* c */ \
    O(PL, 0xB8C, 0xB8C, 0xBC0) /* c */ \
    O(PL, 0xB90, 0xB90, 0xBC4) /* c */ \
    O(PL, 0xB98, 0xB98, 0xBCC) /* c */ \
    O(PL, 0xBA4, 0xBA4, 0xBD8) /* c */ \
    O(PL, 0xBB4, 0xBB4, 0xBEC) /* c */ \
    O(PL, 0xBDC, 0xBDC, 0xC14) /* b */ \
    O(PL, 0xCCC, 0xCCC, 0xD04) /* b */ \
    O(PL, 0xCD8, 0xCD8, 0xD10) /* b */ \
    O(PL, 0xCDC, 0xCDC, 0xD14) /* b */ \
    O(PL, 0xCE0, 0xCE0, 0xD18) /* b */ \
    O(PL, 0xCFC, 0xCFC, 0xD34) /* c */ \
    O(PL, 0xD00, 0xD00, 0xD38) /* c */ \
    O(PL, 0xD28, 0xD28, 0xD60) /* b */ \
    O(PL, 0xD2C, 0xD2C, 0xD64) /* b */ \
    O(PL, 0xD48, 0xD48, 0xD80) /* c */ \
    O(PL, 0xD4C, 0xD4C, 0xD84) /* c */ \
    O(PL, 0xD58, 0xD58, 0xD90) /* b */ \
    O(PL, 0xD5C, 0xD5C, 0xD94) /* b */ \
    O(PL, 0xE10, 0xE10, 0xE4C) /* c */ \
    O(PL, 0xE20, 0xE20, 0xE5C) /* v */ \
    O(PL, 0xED8, 0xED8, 0xF28) /* c */ \
    O(PL, 0xEF4, 0xEF4, 0xF44) /* b */ \
    O(PL, 0xEF5, 0xEF5, 0xF45) /* c */ \
    O(PL, 0xEFA, 0xEFA, 0xF4A) /* c */ \
    O(PL, 0xEFB, 0xEFB, 0xF4B) /* c */ \
    O(PL, 0xEFC, 0xEFC, 0xF4C) /* b */ \
    O(PL, 0xF02, 0xF02, 0xF52) /* c */ \
    O(PLB98, 0x2D, 0x2D, 0x2D) /* c: read through a player's +0xB98 */ \
    O(PLSTATS, 0x1C, 0x1C, 0x1C) /* b */ \
    O(PLSTATS, 0x20, 0x20, 0x20) /* b */ \
    O(RM, 0x4, 0x4, 0x4) /* c: its file's code (the resource manager) */ \
    O(RM, 0x8, 0x8, 0x8) /* c: its file's code (the resource manager) */ \
    O(RM, 0x14, 0x14, 0x14) /* c: its file's code (the resource manager) */ \
    O(RM, 0x18, 0x18, 0x18) /* c: its file's code (the resource manager) */ \
    O(RM, 0x1C, 0x1C, 0x1C) /* c: its file's code (the resource manager) */ \
    O(RM, 0x20, 0x20, 0x20) /* c: its file's code (the resource manager) */ \
    O(RM, 0x88, 0x88, 0x88) /* c: its file's code (the resource manager) */ \
    O(RM, 0x8C, 0x8C, 0x8C) /* c: its file's code (the resource manager) */ \
    O(RM, 0x98, 0x98, 0x98) /* c: its file's code (the resource manager) */ \
    O(RM, 0x9C, 0x9C, 0x9C) /* c: its file's code (the resource manager) */ \
    O(RM, 0xA4, 0xA4, 0xA4) /* c: its file's code (the resource manager) */ \
    O(RM, 0x130, 0x130, 0x130) /* c: its file's code (the resource manager) */ \
    O(RM, 0x134, 0x134, 0x134) /* c: its file's code (the resource manager) */ \
    O(RM, 0x150, 0x150, 0x150) /* c: its file's code (the resource manager) */ \
    O(RM, 0x16C, 0x16C, 0x16C) /* c: its file's code (the resource manager) */ \
    O(RM, 0x187, 0x187, 0x187) /* c: its file's code (the resource manager) */ \
    O(RM, 0x188, 0x188, 0x188) /* c: its file's code (the resource manager) */ \
    O(ROUTENODE, 0x3C, 0x3C, 0x3C) /* s */ \
    O(ROUTENODE, 0x40, 0x40, 0x40) /* s */ \
    O(ROUTEPLANNER, 0x48, 0x48, 0x48) /* c */ \
    O(RS, 0x8, 0x8, 0x8) /* h: kept: the object's start, before its first known shift (0xF4 -> 0x104) */ \
    O(RS, 0xC, 0xC, 0xC) /* h: kept: the object's start, before its first known shift (0xF4 -> 0x104) */ \
    O(RS, 0x18, 0x18, 0x18) /* h: kept: the object's start, before its first known shift (0xF4 -> 0x104) */ \
    O(RS, 0x1C, 0x1C, 0x1C) /* h: kept: the object's start, before its first known shift (0xF4 -> 0x104) */ \
    O(RS, 0x114, 0x114, 0x124) /* v */ \
    O(RS, 0x118, 0x118, 0x128) /* v */ \
    O(RS, 0x11A, 0x11A, 0x12A) /* v */ \
    O(SCEN, 0x4, 0x4, 0x4) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x8, 0x8, 0x8) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0xC, 0xC, 0xC) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x18, 0x18, 0x18) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x2C, 0x2C, 0x2C) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x40, 0x40, 0x40) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x44, 0x44, 0x44) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x4C, 0x4C, 0x4C) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x50, 0x50, 0x50) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x58, 0x58, 0x58) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x59, 0x59, 0x59) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x5A, 0x5A, 0x5A) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x5C, 0x5C, 0x5C) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x60, 0x60, 0x60) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x64, 0x64, 0x64) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x6C, 0x6C, 0x6C) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x70, 0x70, 0x70) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x78, 0x78, 0x78) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x80, 0x80, 0x80) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x81, 0x81, 0x81) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x83, 0x83, 0x83) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0xE4, 0xE4, 0xE4) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0xE8, 0xE8, 0xE8) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0xF0, 0xF0, 0xF0) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0xF8, 0xF8, 0xF8) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0xFC, 0xFC, 0xFC) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x100, 0x100, 0x100) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x104, 0x104, 0x104) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x108, 0x108, 0x108) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x118, 0x118, 0x118) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x128, 0x128, 0x128) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x13C, 0x13C, 0x13C) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x158, 0x158, 0x158) /* c: its file's code (the scenario manager) */ \
    O(SCEN, 0x180, 0x180, 0x180) /* h: kept, unsure (98 uses keep it, 49 say +4; only the test command scen reads it) */ \
    O(SCEN, 0x210, 0x210, 0x210) /* c: its file's code (the scenario manager) */ \
    O(SCROLL, 0x40, 0x40, 0x40) /* v */ \
    O(SCROLL, 0x44, 0x44, 0x44) /* v */ \
    O(SCROLL, 0x45, 0x45, 0x45) /* v */ \
    O(SCROLL, 0x46, 0x46, 0x46) /* v */ \
    O(SCROLL, 0x47, 0x47, 0x47) /* v */ \
    O(SERVER, 0x10, 0x10, 0x10) /* b */ \
    O(SERVER, 0x190, 0x190, 0x190) /* b */ \
    O(SETUP, 0x3A8, 0x3A8, 0x3A8) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP, 0x3D4, 0x3D4, 0x3D4) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP, 0x3D8, 0x3D8, 0x3D8) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP, 0x3DC, 0x3DC, 0x3DC) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP, 0x3E0, 0x3E0, 0x3E0) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP, 0x3E4, 0x3E4, 0x3E4) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP, 0x3E8, 0x3E8, 0x3E8) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP, 0x3EC, 0x3EC, 0x3EC) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP, 0x3F0, 0x3F0, 0x3F0) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP, 0x3F4, 0x3F4, 0x3F4) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP, 0x3F8, 0x3F8, 0x3F8) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP, 0x3FC, 0x3FC, 0x3FC) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP, 0x400, 0x400, 0x400) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP, 0x401, 0x401, 0x401) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP, 0x402, 0x402, 0x402) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUP, 0x403, 0x403, 0x403) /* c: the setup copy 0x524B8F and FN_GAME_START */ \
    O(SETUPSLOT, 0x0, 0x0, 0x0) /* c: the setup code */ \
    O(SETUPSLOT, 0x24, 0x24, 0x24) /* c: the setup code */ \
    O(SITEOWN, 0x4, 0x4, 0x4) /* s */ \
    O(STRATEGY, 0x3C, 0x3C, 0x3C) /* s */ \
    O(STRATEGY, 0x40, 0x40, 0x40) /* s */ \
    O(TASK, 0x20, 0x20, 0x20) /* c */ \
    O(TASK, 0x28, 0x28, 0x28) /* c */ \
    O(TASK, 0x2C, 0x2C, 0x2C) /* c */ \
    O(TASK, 0x30, 0x30, 0x30) /* c */ \
    O(TD, 0x0, 0x0, 0x0) /* c */ \
    O(TD, 0x1C, 0x1C, 0x1C) /* v */ \
    O(TD, 0x3FC, 0x3FC, 0xF8) /* c */ \
    O(TD, 0x41C, 0x41C, 0x120) /* c: the type data's constructors */ \
    O(TD, 0x420, 0x420, 0x124) /* c: the type data's constructors */ \
    O(TD, 0x424, 0x424, 0x128) /* c: the type data's constructors */ \
    O(TD, 0x42C, 0x42C, 0x130) /* c: the type data's constructors */ \
    O(TD, 0x430, 0x430, 0x134) /* c: the type data's constructors */ \
    O(TD, 0x438, 0x438, 0x13C) /* c: the type data's constructors */ \
    O(TD, 0x4B4, 0x4B4, 0x1B8) /* c */ \
    O(TD, 0x4B8, 0x4B8, 0x1BC) /* c */ \
    O(TD, 0x4C0, 0x4C0, 0x1C4) /* c: the type data's constructors */ \
    O(TD, 0x4C4, 0x4C4, 0x1C8) /* c: the type data's constructors */ \
    O(TD, 0x4C8, 0x4C8, 0x1CC) /* c: the type data's constructors */ \
    O(TD, 0x4E0, 0x4E0, 0x1E4) /* c: the type data's constructors */ \
    O(TD, 0x50C, 0x50C, 0x210) /* c: the type data's constructors */ \
    O(TD, 0x510, 0x510, 0x214) /* c: the type data's constructors */ \
    O(TD, 0x54C, 0x54C, 0x24C) /* c: the type data's constructors */ \
    O(TD, 0x550, 0x550, 0x250) /* c: the type data's constructors */ \
    O(TD, 0x55C, 0x55C, 0x25C) /* c: the type data's constructors */ \
    O(TD, 0x588, 0x588, 0x288) /* c: the type data's constructors */ \
    O(TD, 0x58C, 0x58C, 0x28C) /* c */ \
    O(TD, 0x621, 0x621, 0x31F) /* c: the type data's constructors */ \
    O(TECH, 0x8, 0x8, 0x8) /* v: the tech tree code's reads (unchanged) */ \
    O(TECH, 0x14, 0x14, 0x14) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHCHILD, 0x8, 0x8, 0x8) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA, 0x10, 0x10, 0x10) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA, 0x14, 0x14, 0x14) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA, 0x24, 0x24, 0x24) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA, 0x28, 0x28, 0x28) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA, 0x2C, 0x2C, 0x2C) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA, 0x30, 0x30, 0x30) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA, 0x34, 0x34, 0x34) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA, 0x48, 0x48, 0x48) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA, 0x4C, 0x4C, 0x4C) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA, 0x50, 0x50, 0x50) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA, 0x6C, 0x6C, 0x6C) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA, 0x7D, 0x7D, 0x7D) /* v: the tech tree code's reads (unchanged) */ \
    O(TECHDATA, 0x81, 0x81, 0x81) /* h: AoC's epoch upgrades (only new) skip a tech with it set - read in AoC only */ \
    O(TECHNODE, 0x8, 0x8, 0x8) /* c */ \
    O(TECHNODE, 0xC, 0xC, 0xC) /* c */ \
    O(TECHNODE, 0x24, 0x24, 0x24) /* v */ \
    O(TECHNODE, 0x28, 0x28, 0x28) /* v */ \
    O(TT, 0x4, 0x4, 0x4) /* v */ \
    O(TT, 0x4E4, 0x4E4, 0x538) /* v */ \
    O(TTREC, 0x8, 0x8, 0x8) /* v: the tech tree code's reads (unchanged) */ \
    O(TTREC, 0x40, 0x40, 0x40) /* v: the tech tree code's reads (unchanged) */ \
    O(TTREC, 0x48, 0x48, 0x48) /* v: the tech tree code's reads (unchanged) */ \
    O(UIFORM, 0x5D8, 0x5D8, 0x5E8) /* h: votes 6/7 */ \
    O(UISCREEN, 0xF8, 0xF8, 0xF8) /* v */ \
    O(UM, 0xC, 0xC, 0xC) /* c: its file's code (the unit manager) */ \
    O(UM, 0x134, 0x134, 0x148) /* c: its file's code (+0x14, 31 votes) */ \
    O(UM, 0x13C, 0x13C, 0x150) /* c: its file's code (+0x14) */ \
    O(UM, 0x140, 0x140, 0x154) /* c: its file's code (+0x14) */ \
    O(UNIT, 0x4, 0x4, 0x4) /* c */ \
    O(UNIT, 0x8, 0x8, 0x8) /* c */ \
    O(UNIT, 0x10, 0x10, 0x10) /* c */ \
    O(UNIT, 0x14, 0x14, 0x14) /* c */ \
    O(UNIT, 0x18, 0x18, 0x18) /* c */ \
    O(UNIT, 0x1C, 0x1C, 0x1C) /* c */ \
    O(UNIT, 0x20, 0x20, 0x20) /* c */ \
    O(UNIT, 0x24, 0x24, 0x24) /* c */ \
    O(UNIT, 0x2A, 0x2A, 0x2A) /* c */ \
    O(UNIT, 0x2B, 0x2B, 0x2B) /* c */ \
    O(UNIT, 0x2C, 0x2C, 0x2C) /* c */ \
    O(UNIT, 0x30, 0x30, 0x30) /* c */ \
    O(UNIT, 0x34, 0x34, 0x34) /* c */ \
    O(UNIT, 0x38, 0x38, 0x38) /* c */ \
    O(UNIT, 0x3C, 0x3C, 0x3C) /* c */ \
    O(UNIT, 0x40, 0x40, 0x40) /* c */ \
    O(UNIT, 0x4C, 0x4C, 0x4C) /* c */ \
    O(UNIT, 0x50, 0x50, 0x50) /* c */ \
    O(UNIT, 0x60, 0x60, 0x60) /* c */ \
    O(UNIT, 0x68, 0x68, 0x68) /* c */ \
    O(UNIT, 0x134, 0x134, 0x134) /* b */ \
    O(UNIT, 0x13C, 0x13C, 0x13C) /* b */ \
    O(UNIT, 0x144, 0x144, 0x144) /* b */ \
    O(UNIT, 0x14C, 0x14C, 0x14C) /* b */ \
    O(UNIT, 0x154, 0x154, 0x154) /* b */ \
    O(UNIT, 0x15C, 0x15C, 0x15C) /* b */ \
    O(UNIT, 0x1C0, 0x1C0, 0x1C0) /* c */ \
    O(UNIT, 0x1CC, 0x1CC, 0x1CC) /* c */ \
    O(UNIT, 0x1EC, 0x1EC, 0x1EC) /* c */ \
    O(UNIT, 0x1F0, 0x1F0, 0x1F0) /* c */ \
    O(UNIT, 0x208, 0x208, 0x208) /* c */ \
    O(UNIT, 0x21C, 0x21C, 0x21C) /* c */ \
    O(UNIT, 0x238, 0x238, 0x238) /* c */ \
    O(UNIT, 0x25C, 0x25C, 0x260) /* c */ \
    O(UNIT, 0x28C, 0x28C, 0x290) /* c */ \
    O(UNIT, 0x2A0, 0x2A0, 0x2A4) /* c */ \
    O(UNIT, 0x33B, 0x33B, 0x347) /* c */ \
    O(UNIT, 0x340, 0x340, 0x34C) /* c */ \
    O(UNIT, 0x341, 0x341, 0x34D) /* c */ \
    O(USTAT, 0x60, 0x60, 0x60) /* b: between fields read through a unit's +0x34, unchanged */ \
    O(USTAT, 0x98, 0x98, 0x98) /* c: read through a unit's +0x34 */ \
    O(USTAT, 0x9C, 0x9C, 0x9C) /* c: read through a unit's +0x34 */ \
    O(USTAT, 0xA0, 0xA0, 0xA0) /* c: read through a unit's +0x34 */ \
    O(USTAT, 0xAC, 0xAC, 0xAC) /* c: read through a unit's +0x34 */ \
    O(USTAT, 0xC0, 0xC0, 0xC0) /* b: between fields read through a unit's +0x34, unchanged */ \
    O(USTAT, 0xD8, 0xD8, 0xD8) /* c: read through a unit's +0x34 */ \
    O(USTAT, 0xE0, 0xE0, 0xE0) /* b: between fields read through a unit's +0x34, unchanged */ \
    O(USTAT, 0x138, 0x138, 0x138) /* c: read through a unit's +0x34 */ \
    O(USTAT, 0x144, 0x144, 0x144) /* c: read through a unit's +0x34 */ \
    O(WORLD, 0x54, 0x54, 0x54) /* b */ \
    O(ZONE, 0x8, 0x8, 0x8) /* s */ \
    O(ZONE, 0xC, 0xC, 0xC) /* s */ \
    O(ZONE, 0x14, 0x14, 0x14) /* s */ \
    O(ZONE, 0x18, 0x18, 0x18) /* s */ \


#define OF_ENUM(k, x, ...) OF_##k##_##x,
enum field_name { FIELDS(OF_ENUM) OF_N };
#define OF_ROW(k, x, ...) { __VA_ARGS__ },
static const uint32_t k_of[OF_N][ADDR_BUILDS] = { FIELDS(OF_ROW) };
#define OF_NAME(k, x, ...) #k " " #x,
static const char *const k_of_name[OF_N] = { FIELDS(OF_NAME) };
static uint32_t g_of[OF_N];
#define OF(k, x) (g_of[OF_##k##_##x])
static int g_of_missing;   // fields the running build hasn't got (NA): logged at the start (fields_log)
static int fields_select(int build) {   // the running exe's column into g_of (with addr_select)
    int missing = 0;
    for (int i = 0; i < OF_N; i++) missing += (g_of[i] = k_of[i][build]) == NA;
    return missing;
}
static void fields_log(FILE *log) {
    if (!log || !g_of_missing) return;
    fprintf(log, "# fields: build %d hasn't got %d of %d:", g_exe_build, g_of_missing, OF_N);
    for (int i = 0; i < OF_N; i++) if (g_of[i] == NA) fprintf(log, " %s", k_of_name[i]);
    fputc(10, log);
}
