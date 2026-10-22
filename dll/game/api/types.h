// THE GAME'S OBJECTS, TYPED: a pointer type of its own for each kind of object (raw/fields.h's kinds), for
// the module's accessors, wrappers and hook types (one place: the objects point at each other). The types are never defined, only pointed to: a unit passed where a
// player is wanted, or the arguments swapped, doesn't build (a void * still passes - code not moved over yet). FIELD
// reads them as before; dll/build.sh's -fno-strict-aliasing keeps the types out of the optimizer's aliasing.

typedef struct ee_player ee_player;             // PL: a player (EEPlayer, EEComputerPlayer)
typedef struct ee_unit ee_unit;                 // UNIT: a unit, building or resource on the map
typedef struct ee_unit_type ee_unit_type;       // TD: a unit type's data (EEUnitClass)
typedef struct ee_unit_stats ee_unit_stats;     // USTAT: a unit type's values for one player, a unit's +0x34
typedef struct ee_goal ee_goal;                 // GOAL: a unit's goal (EEUnitGoal)
typedef struct ee_civ_mgr ee_civ_mgr;           // CM: the AI's civilization manager, player +0xB90
typedef struct ee_resource_mgr ee_resource_mgr; // RM: the AI's resource manager, CM +0x38
typedef struct ee_build_mgr ee_build_mgr;       // BM: the AI's build manager, CM +0x3C
typedef struct ee_research_mgr ee_research_mgr; // RS: the AI's research manager, CM +0x40
typedef struct ee_unit_mgr ee_unit_mgr;         // UM: the AI's unit manager, CM +0x44
typedef struct ee_tech_tree ee_tech_tree;       // TT: a player's tech tree, player +0x9D4
typedef struct ee_tech ee_tech;                 // TECH, TECHNODE: a tech tree node (EETechTreeNode)
typedef struct ee_tech_data ee_tech_data;       // TECHDATA: a tech's data, TECH +0x8
typedef struct ee_zone ee_zone;                 // ZONE: an AI town or resource zone
typedef struct ee_mouse ee_mouse;               // MOUSE: the game cursor, [G_MOUSE] (RTSMouse)
typedef struct ee_combat_mgr ee_combat_mgr;     // AM: the AI's combat manager, player +0xB8C
typedef struct ee_attack_cmd ee_attack_cmd;     // AMCMD: its attack commander, AM +0x48
typedef struct ee_attack_group ee_attack_group; // AG: an attack group
typedef struct ee_attack_subgroup ee_attack_subgroup; // ASG: an attack subgroup
typedef struct ee_asg_unit ee_asg_unit;         // ASGU: a subgroup's unit record
typedef struct ee_map ee_map;                   // MAP: the map, [G_MAP] (EEMap)
typedef struct ee_tile ee_tile;                 // TILE: a map tile
typedef struct ee_grid ee_grid;                 // GRID: the map's unit grid, MAP +0x181724
typedef struct ee_grid_cell ee_grid_cell;       // GRIDCELL: a cell of the unit grid (16 bytes)
typedef struct ee_unit_vis ee_unit_vis;         // SITEOWN: a unit's visibility record, its +0x14
typedef struct ee_asgu_info ee_asgu_info;       // ASGUINFO: what FN_ASGU_SCORE scores
typedef struct ee_strategy ee_strategy;         // STRATEGY: an attack strategy (EECPCombatStrategy)
typedef struct ee_task ee_task;                 // TASK: an attack task (EECPAGTask)
typedef struct ee_route_planner ee_route_planner; // ROUTEPLANNER: the land attack route planner
typedef struct ee_route_node ee_route_node;     // ROUTENODE: a route planner's node
typedef struct ee_am4c ee_am4c;                 // AM4C: the object at the combat manager's +0x4C
typedef struct ee_plan_node ee_plan_node;       // CMLIST: a node of the civilization manager's build plan
typedef struct ee_tt_epoch ee_tt_epoch;         // TTREC: the tech tree's record of an epoch
typedef struct ee_epoch_techs ee_epoch_techs;   // EPOCHTECHS: an epoch's techs, G_EPOCH_TECHS
typedef struct ee_effect ee_effect;             // EFFECT: a tech effect, G_EFFECTS
typedef struct ee_player_stats ee_player_stats; // PLSTATS: a player's statistics, player +0x4A8
typedef struct ee_player_b98 ee_player_b98;     // PLB98: the object at a player's +0xB98
typedef struct ee_dmg_model ee_dmg_model;       // DMGMODEL: the object of FN_DAMAGE_POINTS
typedef struct ee_physics ee_physics;           // PHYS: the simulation, [G_WORLD_CLOCK] (EEPhysics)
typedef struct ee_world ee_world;               // WORLD: the world, [G_WORLD] (EEWorld)
typedef struct ee_server ee_server;             // SERVER: the lockstep server or client, [G_SERVER]
typedef struct ee_msg ee_msg;                   // MSG: a message players send (EEMessage: tribute, chat, select ...)
typedef struct ee_game_data ee_game_data;       // GAMEDATA: the game data, [G_GAME_DATA]
typedef struct ee_setup ee_setup;               // SETUP: the game setup
typedef struct ee_setup_slot ee_setup_slot;     // SETUPSLOT: a player slot of the setup
typedef struct ee_scen_mgr ee_scen_mgr;         // SCEN: the scenario manager (EEScenarioManager)
typedef struct ee_trigger ee_trigger;           // TRIGGER: a scenario trigger (EESMTrigger)
typedef struct ee_trig_cond ee_trig_cond;       // TRIGCOND: a trigger's condition reference
typedef struct ee_trig_effect ee_trig_effect;   // TRIGEFFECT: a trigger's effect reference
typedef struct ee_scen_cond ee_scen_cond;       // SCENCOND: a scenario condition
typedef struct ee_scen_effect ee_scen_effect;   // SCENEFFECT: a scenario effect
typedef struct ee_draw_rec ee_draw_rec;         // DRAWREC: a draw list record
typedef struct ee_camera ee_camera;             // CAMERA: the camera, G_CAMERA (EECameraControl)
typedef struct ee_scroll ee_scroll;             // SCROLL: the scroll action (EEAScroll)
typedef struct ee_native_chat ee_native_chat;   // NATIVECHAT: the object of FN_NATIVE_CHAT
typedef struct ee_ui_form ee_ui_form;           // UIFORM: the UI manager's current form
typedef struct ee_ui_screen ee_ui_screen;       // UISCREEN: a single-player menu screen
// the engine DLLs' objects (E_)
typedef struct ee_rasterizer ee_rasterizer;     // E_RAS: the rasterizer (DX7Rasterizer), [G_RASTERIZER]
typedef struct ee_screen_state ee_screen_state; // E_SCRSTATE: its per-screen state
typedef struct ee_screen ee_screen;             // E_SCREEN: a screen (DX7Screen)
typedef struct ee_surface ee_surface;           // E_SURF: a surface (DX7Surface)
typedef struct ee_draw_state ee_draw_state;     // E_DSTATE: the rasterizer's draw state
typedef struct ee_window ee_window;             // E_WINDOW: the game window (GEWindow)
typedef struct ee_input_mouse ee_input_mouse;   // E_MOUSE: the DirectInput mouse (ISMouse)
typedef struct ee_text ee_text;                 // E_TEXT: a text (GEText)
typedef struct ee_font ee_font;                 // E_FONT: a font (GEFont)
typedef struct ee_sound_mgr ee_sound_mgr;       // E_SOUNDMGR: the sound manager (SESoundManager)
typedef struct ee_sound_cat ee_sound_cat;       // E_SNDCAT: its categories
typedef struct ee_sound_node ee_sound_node;     // E_SNDNODE: a playing sound (SESMInstanceNode)
typedef struct ee_sample ee_sample;             // E_SAMPLE: a sample (SESample)
typedef struct ee_mixer ee_mixer;               // E_MIXER: the mixer plugin's functions (SEMixer)

// the game's UString (passed by value to some of its functions)
struct ustring { uint8_t raw[16]; };

// Objects with no fields read yet (the wall manager, the UI manager, a behaviour ...) are void *; uint8_t * is a byte
// (a flag's address) or a runtime-library vector's elements stepped by their size.
