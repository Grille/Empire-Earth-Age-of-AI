// THE ADDRESSES OF THE GAME'S DATA (the module's accessors read them: game_map(), game_players() ...): their rows of tables.c's
// tables, in the same columns (one value per exe build, k_exe_builds' order).

#define ADDRS_DATA(F, D) \
    D(G_FLOAT_1_600, 0x0082CBCC, 0x00842374)                                  /* the float 1/600 (the minimap's y scale operand) */ \
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

