// THE GAME'S OBJECTS, TYPED: a pointer type of its own for each kind of object (raw/fields.h's kinds), for
// functions.c's wrappers and hooks.c's hook types. The types are never defined, only pointed to: a unit passed where a
// player is wanted, or the arguments swapped, doesn't build (a void * still passes - code not moved over yet). FIELD
// reads them as before; dll/build.sh's -fno-strict-aliasing keeps the types out of the optimizer's aliasing.

typedef struct ee_player ee_player;             // PL: a player (EEPlayer, EEComputerPlayer)
typedef struct ee_unit ee_unit;                 // UNIT: a unit, building or resource on the map
typedef struct ee_unit_type ee_unit_type;       // TD: a unit type's data (EEUnitClass)
typedef struct ee_goal ee_goal;                 // GOAL: a unit's goal (EEUnitGoal)
typedef struct ee_civ_mgr ee_civ_mgr;           // CM: the AI's civilization manager, player +0xB90
typedef struct ee_resource_mgr ee_resource_mgr; // RM: the AI's resource manager, CM +0x38
typedef struct ee_build_mgr ee_build_mgr;       // BM: the AI's build manager, CM +0x3C
typedef struct ee_research_mgr ee_research_mgr; // RS: the AI's research manager, CM +0x40
typedef struct ee_unit_mgr ee_unit_mgr;         // UM: the AI's unit manager, CM +0x44
typedef struct ee_tech_tree ee_tech_tree;       // TT: a player's tech tree, player +0x9D4
typedef struct ee_tech ee_tech;                 // TECH, TECHNODE, TECHCHILD: a tech tree node (EETechTreeNode)
typedef struct ee_tech_data ee_tech_data;       // TECHDATA: a tech's data, TECH +0x8
typedef struct ee_zone ee_zone;                 // ZONE: an AI town or resource zone
typedef struct ee_mouse ee_mouse;               // MOUSE: the game cursor, [G_MOUSE] (RTSMouse)
typedef struct ee_combat_mgr ee_combat_mgr;     // AM: the AI's combat manager, player +0xB8C
typedef struct ee_attack_cmd ee_attack_cmd;     // AMCMD: its attack commander, AM +0x48
typedef struct ee_attack_group ee_attack_group; // AG: an attack group
typedef struct ee_attack_subgroup ee_attack_subgroup; // ASG: an attack subgroup
typedef struct ee_asg_unit ee_asg_unit;         // ASGU: a subgroup's unit record

// the game's UString (passed by value to some of its functions)
struct ustring { uint8_t raw[16]; };

// Kinds not typed yet (goals, messages, the game state's tables) are void *.
