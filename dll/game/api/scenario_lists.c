// THE SCENARIO EDITOR'S LISTS (scenario.c's objects hold them): the enums before the spec's C (dll/game/api/gen/),
// which types its fields by them - included by api.c ahead of gen/gen.c.

// THE EDITOR'S LISTS: the values the fields hold and the functions take, named as the editor shows them (the strings
// of Language.dll, number in brackets; docs/triggers.md has what each does). A list's value is its place in the editor's
// list unless a value is given.
// an effect's category (29321) and each category's actions (scen_effect_get_kind)
enum sm_category { SM_CAT_OBJECT = 1, SM_CAT_PLAYER, SM_CAT_TRIGGER, SM_CAT_MEDIA, SM_CAT_GAME, SM_CAT_AI };
enum sm_object_action {   // 29350
    SM_OBJ_CREATE, SM_OBJ_FREEZE, SM_OBJ_UNFREEZE, SM_OBJ_KILL, SM_OBJ_REMOVE, SM_OBJ_TASK_OBJECT, SM_OBJ_TASK_AREA,
    SM_OBJ_STANCE, SM_OBJ_UNIT_ATTRIBUTE, SM_OBJ_OWNER, SM_OBJ_CLASS_NAME, SM_OBJ_FACE_OBJECT, SM_OBJ_CLASS_ATTRIBUTE
};
enum sm_player_action {   // 29452
    SM_PL_VICTORY, SM_PL_SCRIPT_CAMERA, SM_PL_DEFEAT, SM_PL_DIPLOMACY, SM_PL_SET_ATTRIBUTE_LEVEL, SM_PL_MODIFY_TECH_TREE,
    SM_PL_AWARD_CIV_POINTS, SM_PL_FADE_IN, SM_PL_FADE_OUT, SM_PL_VISIBILITY_ON, SM_PL_VISIBILITY_OFF, SM_PL_MINIMAP_FLARE
};
enum sm_trigger_action { SM_TRIG_ON, SM_TRIG_OFF, SM_TRIG_FIRED, SM_TRIG_NOT_FIRED };                                    // 29650
enum sm_media_action { SM_MEDIA_PLAY_SOUND, SM_MEDIA_CHANGE_TEXT, SM_MEDIA_SEND_DIALOGUE, SM_MEDIA_SEND_CHAT_MESSAGE }; // 29525
enum sm_game_action {     // 29575
    SM_GAME_DAYLIGHT_HOUR, SM_GAME_DAY_NIGHT_CYCLE_TIME, SM_GAME_CALAMITY, SM_GAME_GRAPHIC_EFFECT, SM_GAME_START_CINEMATIC_MODE,
    SM_GAME_END_CINEMATIC_MODE, SM_GAME_AMBIENT_WEATHER
};
enum sm_ai_action {       // 29690; Maintain Object does nothing
    SM_AI_MODIFY_AI_MILITARY_TARGET, SM_AI_CREATE_AI_MILITARY_GROUP, SM_AI_BUILD_OBJECT, SM_AI_MAINTAIN_OBJECT,
    SM_AI_SET_AI_TOGGLE, SM_AI_SET_AI_VARIABLE, SM_AI_SET_FAVORITE_UNITS
};
// a condition's type (29300 from Player-Attribute; 0 always true, 1 an object), its compare (29310: unsigned)
enum sm_cond_type {
    SM_COND_TRUE, SM_COND_OBJECT, SM_COND_PLAYER_ATTRIBUTE, SM_COND_PLAYER_TECHNOLOGY, SM_COND_TRIGGER_ATTRIBUTE,
    SM_COND_PLAYER_DIPLOMACY, SM_COND_GAME_ATTRIBUTE, SM_COND_CHAT_MESSAGE_CONTAINS
};
enum sm_compare { SM_LESS, SM_LESS_EQUAL, SM_EQUAL, SM_GREATER_EQUAL, SM_GREATER };
// a player spec (29050): a group, or one player - SM_PLAYER(n). In conditions "any" and "all" are swapped
enum sm_player_spec {
    SM_WORLD, SM_ANY_HUMAN, SM_ANY_COMPUTER, SM_ALL_HUMAN, SM_ALL_COMPUTER, SM_ANY_PLAYER, SM_ANY_EXCLUDE_WORLD, SM_PLAYER_0
};
#define SM_PLAYER(n) ((enum sm_player_spec)(SM_PLAYER_0 + (n) - 1))   // "Player n", 1-based as the editor counts
// a player attribute (29670; the editor offers up to Difficulty Level, the rest work too): the variables, the
// tribute sent / received by resource, then statistics series n as SM_PA_STATS + n (AoC: only 1..30, then its mining
// restriction - 29681, player +0xAE4, read and set)
enum sm_player_attr {
    SM_PA_ANY_RESOURCE, SM_PA_ALL_RESOURCES, SM_PA_STONE, SM_PA_GOLD, SM_PA_WOOD, SM_PA_FOOD, SM_PA_IRON, SM_PA_CURRENT_POP,
    SM_PA_POP_CAP, SM_PA_UNIT_COUNT, SM_PA_DIFFICULTY_LEVEL, SM_PA_VARIABLE0, SM_PA_FOOD_SENT = 21, SM_PA_FOOD_RECEIVED,
    SM_PA_WOOD_SENT, SM_PA_WOOD_RECEIVED, SM_PA_STONE_SENT, SM_PA_STONE_RECEIVED, SM_PA_GOLD_SENT, SM_PA_GOLD_RECEIVED,
    SM_PA_IRON_SENT, SM_PA_IRON_RECEIVED, SM_PA_STATS = 30, SM_PA_MINING_RESTRICTION = 61
};
enum sm_attr_mode { SM_TO_EXACTLY, SM_TO_INCREASE_BY, SM_TO_DECREASE_BY };   // 29500: Set-Attribute Level
// a unit attribute (29100; Unit-Attribute sets them), from SM_UA_SHOCK_ARMOR the class's (Class-Attribute), from
// SM_UA_TOTAL_SHOCK_ARMOR the totals only an object tests; the operation (29590; a class attribute only increases or
// decreases)
enum sm_unit_attr {
    SM_UA_HITPOINTS, SM_UA_ANY_RESOURCE, SM_UA_STONE, SM_UA_GOLD, SM_UA_WOOD, SM_UA_FOOD, SM_UA_IRON, SM_UA_FLIGHT_TIME,
    SM_UA_CARGO_COUNT, SM_UA_GARRISON_LEVEL, SM_UA_AMMUNITION, SM_UA_ANIMATION_IS_PAUSED, SM_UA_RELOAD_TIME,
    SM_UA_GRAPHIC_SCALE, SM_UA_POWER, SM_UA_ROTTING_TIME, SM_UA_CONSTRUCTION_COMPLETE, SM_UA_UNIT_VARIABLE1,
    SM_UA_UNIT_VARIABLE2, SM_UA_UNIT_VARIABLE3,
    SM_UA_SHOCK_ARMOR, SM_UA_ARROW_ARMOR, SM_UA_PIERCE_ARMOR, SM_UA_GUN_ARMOR, SM_UA_LASER_ARMOR, SM_UA_MISSILE_ARMOR,
    SM_UA_RELOAD_TIME_CLASS, SM_UA_ATTACK, SM_UA_WORK_RATE, SM_UA_FLIGHT_TIME_CLASS, SM_UA_LEADERSHIP,
    SM_UA_MAX_OBJECTS_CARRIED, SM_UA_REPAIR, SM_UA_HITPOINTS_CLASS, SM_UA_CARRY_AMOUNT, SM_UA_CONTAINER_HEALING,
    SM_UA_RANGE, SM_UA_LINE_OF_SIGHT, SM_UA_MAX_VELOCITY, SM_UA_POWER_CLASS, SM_UA_ANIMAL_AI_ACTIVE, SM_UA_CAMERA_FOCUS,
    SM_UA_TOTAL_SHOCK_ARMOR, SM_UA_TOTAL_ARROW_ARMOR, SM_UA_TOTAL_PIERCE_ARMOR, SM_UA_TOTAL_GUN_ARMOR, SM_UA_TOTAL_LASER_ARMOR,
    SM_UA_TOTAL_MISSILE_ARMOR, SM_UA_TOTAL_RELOAD_TIME, SM_UA_TOTAL_ATTACK, SM_UA_TOTAL_WORK_RATE, SM_UA_TOTAL_FLIGHT_TIME,
    SM_UA_TOTAL_LEADERSHIP, SM_UA_TOTAL_MAX_OBJECTS_CARRIED, SM_UA_TOTAL_REPAIR, SM_UA_TOTAL_HITPOINTS, SM_UA_TOTAL_CARRY_AMOUNT,
    SM_UA_TOTAL_HEALING, SM_UA_TOTAL_RANGE, SM_UA_TOTAL_LINE_OF_SIGHT, SM_UA_TOTAL_MAX_VELOCITY, SM_UA_TOTAL_POWER,
    SM_UA_TOTAL_PLACEHOLDER2, SM_UA_TOTAL_PLACEHOLDER3, SM_UA_TOTAL_TRANSPORT_WEIGHT, SM_UA_TOTAL_WEIGHT, SM_UA_TOTAL_TILE_SIZE,
    SM_UA_TOTAL_CRUISE_ALTITUDE, SM_UA_TOTAL_MINIMUM_RANGE, SM_UA_TOTAL_CAN_ENTER_AIR_TRANSPORTS,
    SM_UA_TOTAL_CAN_ENTER_LAND_TRANSPORTS, SM_UA_TOTAL_CAN_ENTER_SEA_TRANSPORTS, SM_UA_TOTAL_PLACE_HOLDER1,
    SM_UA_TOTAL_PLACE_HOLDER2, SM_UA_TOTAL_PLACE_HOLDER3
};
enum sm_unit_op { SM_INCREASE, SM_DECREASE, SM_EXACTLY };
// Task-Object / Task-Area (29375), then a power cast: SM_TASK_POWER + its calamity (sm_calamity); Stance (29475)
enum sm_task { SM_TASK_NORMAL, SM_TASK_PATROL, SM_TASK_ATTACK_MOVE, SM_TASK_LOAD, SM_TASK_UNLOAD, SM_TASK_GARRISON, SM_TASK_POWER };
enum sm_stance {
    SM_STANCE_EXPLORE, SM_STANCE_IDLE, SM_STANCE_LOCKED, SM_STANCE_UNLOCKED, SM_STANCE_TOWN_PATROL, SM_STANCE_SCOUT,
    SM_STANCE_STAND_GROUND, SM_STANCE_GUARD, SM_STANCE_HARASS, SM_STANCE_CAUTIOUS, SM_STANCE_AGGRESSIVE
};
enum sm_diplomacy { SM_ALLIED, SM_ENEMY };                                  // 29510: Diplomacy, Player-Diplomacy
enum sm_text_kind { SM_TEXT_HINTS, SM_TEXT_INSTRUCTIONS, SM_TEXT_HISTORY };  // 29550: Change Text
// a calamity: the records of db\dbcalamity.dat - the editor offers the prophets' (29600, Any: one of them at random),
// the powers after them work too (campaigns use Time Warp and Battlecry); AoC's file has 13 more, from Flaming Arrows
enum sm_calamity {
    SM_CAL_ANY, SM_CAL_GLOBAL_WARMING, SM_CAL_ICE_AGE, SM_CAL_EARTHQUAKE, SM_CAL_HURRICANE, SM_CAL_SLAVE_REVOLT,
    SM_CAL_REBELLION, SM_CAL_PLAGUE, SM_CAL_MALARIA, SM_CAL_VOLCANO, SM_CAL_FIRE, SM_CAL_ASTEROID, SM_CAL_FLOOD,
    SM_CAL_SHIELD_BATTERY, SM_CAL_ION_PULSE_CANNON, SM_CAL_TIME_WARP, SM_CAL_TELEPORT, SM_CAL_RESONATOR,
    SM_CAL_ANTI_MATTER_STORM, SM_CAL_MECH_VIRUS, SM_CAL_BATTLECRY_7, SM_CAL_BATTLECRY_8, SM_CAL_BATTLECRY_9,
    SM_CAL_FLAMING_ARROWS, SM_CAL_FA_FIRE, SM_CAL_PILUM_ATTACK, SM_CAL_MIND_CONTROL, SM_CAL_CONSCRIPTION, SM_CAL_LOGIC_BOMB,
    SM_CAL_PLANTED_EXPLOSIVES, SM_CAL_CAPITAL_SHIP_SPECIAL, SM_CAL_CLOAKING, SM_CAL_METEOR_STORM, SM_CAL_LINE_OF_SIGHT,
    SM_CAL_BATTLECRY_CENTURION, SM_CAL_SPECIAL_LOCATION
};
enum sm_ai_toggle {       // 29625: Set AI Toggle
    SM_AIT_ACTIVE, SM_AIT_CHEAT, SM_AIT_ATTACK, SM_AIT_DEFEND, SM_AIT_BUILD_REPAIR, SM_AIT_EXPLORE, SM_AIT_IGNORE, SM_AIT_GATHER
};
enum sm_difficulty { SM_EASY, SM_MEDIUM, SM_HARD };                         // 29665: Game-Attribute, Difficulty Level
// an object's tests: each one's mode, the relation's kind (29001); its class filter: any, an epoch (1..14, AoC 1..15),
// then a family, an RPS class, a unit group - their ranges move with AoC's 15th epoch: sm_class_family(f) ...
enum sm_test_mode { SM_IGNORE, SM_MUST, SM_MUST_NOT };
enum sm_relation { SM_REL_LOS, SM_REL_RANGE, SM_REL_NEAR };
enum sm_class { SM_CLASS_ANY };
internal inline enum sm_class sm_class_epoch(int epoch) { return (enum sm_class)epoch; }
internal inline enum sm_class sm_class_family(int family) { return (enum sm_class)((int)GA(SM_CLASS_FAMILY) + family); }
internal inline enum sm_class sm_class_rps(int rps) { return (enum sm_class)((int)GA(SM_CLASS_RPS) + rps); }
internal inline enum sm_class sm_class_group(int group) { return (enum sm_class)((int)GA(SM_CLASS_GROUP) + group); }
// an area's type: its rectangle, the land or sea its point is on, a player's shape round its point
enum sm_area_type { SM_AREA_RECTANGLE, SM_AREA_REGION, SM_AREA_PLAYER_SHAPE };
_Static_assert(SM_UA_CAMERA_FOCUS == 41 && SM_UA_TOTAL_PLACE_HOLDER3 == 74, "the unit attributes are strings 29100..29174");
_Static_assert(SM_CAL_BATTLECRY_9 == 22 && SM_CAL_SPECIAL_LOCATION == 35, "dbcalamity.dat has 23 records, AoC's 36");
