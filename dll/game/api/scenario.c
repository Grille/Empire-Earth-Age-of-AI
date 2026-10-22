// THE SCENARIO MANAGER AND ITS TRIGGERS (EEScenarioManager): fields, functions.

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
// tribute sent / received by resource, then statistics series n as SM_PA_STATS + n
enum sm_player_attr {
    SM_PA_ANY_RESOURCE, SM_PA_ALL_RESOURCES, SM_PA_STONE, SM_PA_GOLD, SM_PA_WOOD, SM_PA_FOOD, SM_PA_IRON, SM_PA_CURRENT_POP,
    SM_PA_POP_CAP, SM_PA_UNIT_COUNT, SM_PA_DIFFICULTY_LEVEL, SM_PA_VARIABLE0, SM_PA_FOOD_SENT = 21, SM_PA_FOOD_RECEIVED,
    SM_PA_WOOD_SENT, SM_PA_WOOD_RECEIVED, SM_PA_STONE_SENT, SM_PA_STONE_RECEIVED, SM_PA_GOLD_SENT, SM_PA_GOLD_RECEIVED,
    SM_PA_IRON_SENT, SM_PA_IRON_RECEIVED, SM_PA_STATS = 30
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
// the powers after them work too (campaigns use Time Warp and Battlecry)
enum sm_calamity {
    SM_CAL_ANY, SM_CAL_GLOBAL_WARMING, SM_CAL_ICE_AGE, SM_CAL_EARTHQUAKE, SM_CAL_HURRICANE, SM_CAL_SLAVE_REVOLT,
    SM_CAL_REBELLION, SM_CAL_PLAGUE, SM_CAL_MALARIA, SM_CAL_VOLCANO, SM_CAL_FIRE, SM_CAL_ASTEROID, SM_CAL_FLOOD,
    SM_CAL_SHIELD_BATTERY, SM_CAL_ION_PULSE_CANNON, SM_CAL_TIME_WARP, SM_CAL_TELEPORT, SM_CAL_RESONATOR,
    SM_CAL_ANTI_MATTER_STORM, SM_CAL_MECH_VIRUS, SM_CAL_BATTLECRY_7, SM_CAL_BATTLECRY_8, SM_CAL_BATTLECRY_9
};
enum sm_ai_toggle {       // 29625: Set AI Toggle
    SM_AIT_ACTIVE, SM_AIT_CHEAT, SM_AIT_ATTACK, SM_AIT_DEFEND, SM_AIT_BUILD_REPAIR, SM_AIT_EXPLORE, SM_AIT_IGNORE, SM_AIT_GATHER
};
enum sm_difficulty { SM_EASY, SM_MEDIUM, SM_HARD };                         // 29665: Game-Attribute, Difficulty Level
// an object's tests: each one's mode, the relation's kind (29001), the class filter's ranges (SM_CLASS_EPOCH + epoch
// 1..14, SM_CLASS_FAMILY + family, SM_CLASS_RPS + RPS class, SM_CLASS_GROUP + unit group)
enum sm_test_mode { SM_IGNORE, SM_MUST, SM_MUST_NOT };
enum sm_relation { SM_REL_LOS, SM_REL_RANGE, SM_REL_NEAR };
enum sm_class { SM_CLASS_ANY, SM_CLASS_EPOCH = 0, SM_CLASS_FAMILY = 15, SM_CLASS_RPS = 72, SM_CLASS_GROUP = 113 };
// an area's type: its rectangle, the land or sea its point is on, a player's shape round its point
enum sm_area_type { SM_AREA_RECTANGLE, SM_AREA_REGION, SM_AREA_PLAYER_SHAPE };
_Static_assert(SM_UA_CAMERA_FOCUS == 41 && SM_UA_TOTAL_PLACE_HOLDER3 == 74, "the unit attributes are strings 29100..29174");
_Static_assert(SM_CAL_BATTLECRY_9 == 22, "dbcalamity.dat has 23 records");

// FIELDS (scenario_manager(); test/scen.c, engine-notes.md
// "Scenario triggers"): the lists are vectors (begin, end) - the address of each list's begin
internal inline uint8_t *scen_triggers_at(const ee_scen_mgr *m) { return (uint8_t *)FIELD_AT(m, SCEN_TRIGGERS); }
internal inline uint8_t *scen_conds_at(const ee_scen_mgr *m) { return (uint8_t *)FIELD_AT(m, SCEN_CONDS); }
internal inline uint8_t *scen_effects_at(const ee_scen_mgr *m) { return (uint8_t *)FIELD_AT(m, SCEN_EFFECTS); }
internal inline uint8_t *scen_objects_at(const ee_scen_mgr *m) { return (uint8_t *)FIELD_AT(m, SCEN_OBJECTS); }
internal inline uint8_t *scen_areas_at(const ee_scen_mgr *m) { return (uint8_t *)FIELD_AT(m, SCEN_AREAS); }
internal inline float scen_get_time(const ee_scen_mgr *m) { return FIELD(m, SCEN_TIME, float); }            // +0x210 ms
// the last chat line received (UWideString; the 500 ms check empties it - one line a check), the queued effects (a
// vector of {effect id, time ms, cinematic} 12 bytes each: an effect's delay, SM_QUEUE_EFFECT)
internal inline uint8_t *scen_chat_at(const ee_scen_mgr *m) { return (uint8_t *)FIELD_AT(m, SCEN_CHAT); }
internal inline uint8_t *scen_queue_at(const ee_scen_mgr *m) { return (uint8_t *)FIELD_AT(m, SCEN_QUEUE); }
internal inline float scen_step(void) { return *(const float *)GA(G_SM_STEP); }                       // 500 ms
// a trigger (EESMTrigger): its name and description (UWideString), the lists of its condition and effect references
internal inline uint8_t *trigger_name_at(const ee_trigger *t) { return (uint8_t *)FIELD_AT(t, TRIGGER_NAME); }
internal inline uint8_t *trigger_desc_at(const ee_trigger *t) { return (uint8_t *)FIELD_AT(t, TRIGGER_DESC); }
internal inline uint8_t *trigger_conds_at(const ee_trigger *t) { return (uint8_t *)FIELD_AT(t, TRIGGER_CONDS); }
internal inline uint8_t *trigger_effects_at(const ee_trigger *t) { return (uint8_t *)FIELD_AT(t, TRIGGER_EFFECTS); }
internal inline int trigger_get_true_for(const ee_trigger *t) { return FIELD(t, TRIGGER_TRUE_FOR, int); }          // +0x4C s the conditions held
internal inline void trigger_set_since(ee_trigger *t, int v) { FIELD(t, TRIGGER_SINCE, int) = v; }
internal inline char trigger_get_on(const ee_trigger *t) { return FIELD(t, TRIGGER_ON, char); }
internal inline void trigger_set_on(ee_trigger *t, char v) { FIELD(t, TRIGGER_ON, char) = v; }
internal inline char trigger_get_fired(const ee_trigger *t) { return FIELD(t, TRIGGER_FIRED, char); }
internal inline void trigger_set_fired(ee_trigger *t, char v) { FIELD(t, TRIGGER_FIRED, char) = v; }
internal inline char trigger_get_loop(const ee_trigger *t) { return FIELD(t, TRIGGER_LOOP, char); }
internal inline int trigger_get_0x54(const ee_trigger *t) { return FIELD(t, TRIGGER_0x54, int); }                // loaded and saved, not its index
internal inline char trigger_get_cinematic(const ee_trigger *t) { return FIELD(t, TRIGGER_CINEMATIC, char); }     // checked during cinematics too
// a trigger's condition reference: the condition's index, the join to the next (0 end, 1 and, 2 or), not
internal inline int trigger_cond_get_index(const ee_trig_cond *w) { return FIELD(w, TRIGCOND_INDEX, int); }
internal inline int trigger_cond_get_join(const ee_trig_cond *w) { return FIELD(w, TRIGCOND_JOIN, int); }
internal inline char trigger_cond_get_not(const ee_trig_cond *w) { return FIELD(w, TRIGCOND_NOT, char); }
// a trigger's effect reference: its kind (2: "or" between random groups), delay (s), the effect's index
internal inline int trigger_effect_get_kind(const ee_trig_effect *w) { return FIELD(w, TRIGEFFECT_KIND, int); }
internal inline int trigger_effect_get_delay(const ee_trig_effect *w) { return FIELD(w, TRIGEFFECT_DELAY, int); }
internal inline int trigger_effect_get_index(const ee_trig_effect *w) { return FIELD(w, TRIGEFFECT_INDEX, int); }
// a condition: its type (0 always .. 7 chat) and each type's values
internal inline uint8_t *scen_cond_text_at(const ee_scen_cond *c) { return (uint8_t *)FIELD_AT(c, SCENCOND_TEXT); } // chat: the text
internal inline enum sm_cond_type scen_cond_get_type(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TYPE, enum sm_cond_type); }
internal inline enum sm_compare scen_cond_get_op(const ee_scen_cond *c) { return FIELD(c, SCENCOND_OP, enum sm_compare); }
internal inline enum sm_player_attr scen_cond_get_attr(const ee_scen_cond *c) { return FIELD(c, SCENCOND_ATTR, enum sm_player_attr); }
internal inline int scen_cond_get_tech(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TECH, int); }
internal inline int scen_cond_get_value(const ee_scen_cond *c) { return FIELD(c, SCENCOND_VALUE, int); }
internal inline int scen_cond_get_object(const ee_scen_cond *c) { return FIELD(c, SCENCOND_OBJECT, int); }
internal inline enum sm_player_spec scen_cond_get_player(const ee_scen_cond *c) { return FIELD(c, SCENCOND_PLAYER, enum sm_player_spec); }
internal inline int scen_cond_get_time_from(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TIME_FROM, int); }
internal inline int scen_cond_get_time_to(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TIME_TO, int); }
internal inline int scen_cond_get_trigger(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TRIGGER, int); }
internal inline char scen_cond_get_trigger_on(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TRIGGER_ON, char); }
internal inline char scen_cond_get_trigger_fired(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TRIGGER_FIRED, char); }
internal inline char scen_cond_get_timed(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TIMED, char); }
internal inline uint8_t *scen_cond_name_at(const ee_scen_cond *c) { return (uint8_t *)FIELD_AT(c, SCENCOND_NAME); }
internal inline enum sm_difficulty scen_cond_get_difficulty(const ee_scen_cond *c) { return FIELD(c, SCENCOND_DIFFICULTY, enum sm_difficulty); } // game: the difficulty
internal inline enum sm_diplomacy scen_cond_get_stance(const ee_scen_cond *c) { return FIELD(c, SCENCOND_STANCE, enum sm_diplomacy); }         // diplomacy: 0 allied, 1 enemy
internal inline enum sm_player_attr scen_cond_get_attr2(const ee_scen_cond *c) { return FIELD(c, SCENCOND_ATTR2, enum sm_player_attr); }           // attribute: the other player's
internal inline enum sm_player_spec scen_cond_get_player2(const ee_scen_cond *c) { return FIELD(c, SCENCOND_PLAYER2, enum sm_player_spec); }       // diplomacy, attribute: the other spec
internal inline int scen_cond_get_hour(const ee_scen_cond *c) { return FIELD(c, SCENCOND_HOUR, int); }             // game: at or after that hour
internal inline char scen_cond_get_test_difficulty(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TEST_DIFFICULTY, char); }
internal inline char scen_cond_get_test_hour(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TEST_HOUR, char); }
internal inline char scen_cond_get_vs_player(const ee_scen_cond *c) { return FIELD(c, SCENCOND_VS_PLAYER, char); } // attribute against attribute
// an effect: its category (1 object, 2 player, 3 trigger, 4 media, 5 game, 6 AI) and its kind within the category
internal inline enum sm_category scen_effect_get_category(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_CATEGORY, enum sm_category); }
internal inline int scen_effect_get_kind(const ee_scen_effect *ef, int cat) {
    const uint32_t off[7] = { 0, SCENEFFECT_OBJECT_KIND, SCENEFFECT_PLAYER_KIND, SCENEFFECT_TRIGGER_KIND,
                              SCENEFFECT_MEDIA_KIND, SCENEFFECT_GAME_KIND, SCENEFFECT_AI_KIND };
    return cat >= 1 && cat <= 6 ? FIELD(ef, off[cat], int) : -1;
}
internal inline int scen_effect_get_object(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_OBJECT, int); }     // object effects
internal inline int scen_effect_get_trigger(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TRIGGER, int); }    // trigger effects
// an effect's values by category (docs/triggers.md "effects"): the player specs (each a player spec as conditions
// have), the object, area and trigger ids (indices), the texts (UWideString / CString: their addresses)
internal inline uint8_t *scen_effect_group_at(const ee_scen_effect *ef) { return (uint8_t *)FIELD_AT(ef, SCENEFFECT_GROUP); }  // AI group: vector of object ids
internal inline uint8_t *scen_effect_text_at(const ee_scen_effect *ef) { return (uint8_t *)FIELD_AT(ef, SCENEFFECT_TEXT); }    // dialogue, chat: the message
internal inline uint8_t *scen_effect_class_name_at(const ee_scen_effect *ef) { return (uint8_t *)FIELD_AT(ef, SCENEFFECT_CLASS_NAME); }
internal inline uint8_t *scen_effect_sound_at(const ee_scen_effect *ef) { return (uint8_t *)FIELD_AT(ef, SCENEFFECT_SOUND); }  // the file under Sounds\.
internal inline uint8_t *scen_effect_line_at(const ee_scen_effect *ef) { return (uint8_t *)FIELD_AT(ef, SCENEFFECT_LINE); }    // change text: the line added
internal inline enum sm_player_attr scen_effect_get_player_attr(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_PLAYER_ATTR, enum sm_player_attr); } // set attribute
internal inline enum sm_diplomacy scen_effect_get_stance(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_STANCE, enum sm_diplomacy); }             // diplomacy
internal inline enum sm_player_spec scen_effect_get_msg_players(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_MSG_PLAYERS, enum sm_player_spec); }   // dialogue, chat
internal inline enum sm_player_spec scen_effect_get_owner_players(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_OWNER_PLAYERS, enum sm_player_spec); } // object owner
internal inline enum sm_player_spec scen_effect_get_players(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_PLAYERS, enum sm_player_spec); }           // player, AI effects
internal inline enum sm_player_spec scen_effect_get_target_players(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TARGET_PLAYERS, enum sm_player_spec); } // diplomacy
internal inline int scen_effect_get_ai_var(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_AI_VAR, int); }             // set AI variable: the record
internal inline enum sm_ai_toggle scen_effect_get_ai_toggle(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_AI_TOGGLE, enum sm_ai_toggle); }
internal inline enum sm_unit_op scen_effect_get_op(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_OP, enum sm_unit_op); }                     // unit / class attribute
internal inline int scen_effect_get_tech(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TECH, int); }
internal inline float scen_effect_get_float(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_FLOAT, float); }           // AI variable
internal inline int scen_effect_get_int1(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_INT1, int); }                 // zoom, AI variable, land favourite
internal inline int scen_effect_get_int2(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_INT2, int); }                 // scroll speed, AI variable, air favourite
internal inline int scen_effect_get_area(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_AREA, int); }                 // AI target / group, camera
internal inline int scen_effect_get_target(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TARGET, int); }             // its object: AI target / group, camera
internal inline enum sm_calamity scen_effect_get_calamity(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_CALAMITY, enum sm_calamity); }         // calamity, graphic effect
internal inline int scen_effect_get_cal_area(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_CAL_AREA, int); }
internal inline int scen_effect_get_cal_object(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_CAL_OBJECT, int); }
internal inline int scen_effect_get_hour(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_HOUR, int); }                 // daylight, cinematic speed
internal inline int scen_effect_get_day_cycle(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_DAY_CYCLE, int); }
internal inline int scen_effect_get_duration(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_DURATION, int); }         // s
internal inline enum sm_text_kind scen_effect_get_text_kind(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TEXT_KIND, enum sm_text_kind); }       // 0 hints, 1 instructions, 2 history
internal inline enum sm_unit_attr scen_effect_get_unit_attr(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_UNIT_ATTR, enum sm_unit_attr); }       // < 20 unit, else class
internal inline int scen_effect_get_value(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_VALUE, int); }               // attribute value, facing (degrees)
internal inline enum sm_stance scen_effect_get_obj_stance(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_OBJ_STANCE, enum sm_stance); }
internal inline enum sm_task scen_effect_get_task(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TASK, enum sm_task); }                 // 0 normal .. 5 garrison, 7+ a power
internal inline int scen_effect_get_task_area(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TASK_AREA, int); }
internal inline int scen_effect_get_task_object(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TASK_OBJECT, int); }
internal inline int scen_effect_get_civ_points(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_CIV_POINTS, int); }
internal inline enum sm_attr_mode scen_effect_get_attr_mode(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_ATTR_MODE, enum sm_attr_mode); }
internal inline int scen_effect_get_amount(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_AMOUNT, int); }
internal inline int scen_effect_get_cam_area(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_CAM_AREA, int); }         // camera, flare
internal inline int scen_effect_get_cam_object(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_CAM_OBJECT, int); }
internal inline char scen_effect_get_on(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_ON, char); }                   // AI toggle, weather
internal inline char scen_effect_get_use_area(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_USE_AREA, char); }       // AI target / group, camera
internal inline char scen_effect_get_attack(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_ATTACK, char); }           // AI: attack (1) / defend (0)
internal inline char scen_effect_get_favorites(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_FAVORITES, char); }
internal inline char scen_effect_get_add(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_ADD, char); }                 // AI target: add (1) / remove (0)
internal inline char scen_effect_get_clear(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_CLEAR, char); }             // change text: clear it
internal inline char scen_effect_get_flare_area(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_FLARE_AREA, char); }   // flare, camera: the area
internal inline char scen_effect_get_percent(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_PERCENT, char); }         // attribute in %, face at once
internal inline char scen_effect_get_tech_add(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TECH_ADD, char); }       // tech tree: add (1) / remove (0)
internal inline char scen_effect_get_cal_use_area(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_CAL_USE_AREA, char); }
// an object (EESMObject): the units picked (their ids: begin, end), by owner spec, class / type, count, area, a relation
// to another object, a parent's units, an attribute range, a state, who sees / selects them - each test's mode 0
// ignore, 1 must, 2 must not. Locked: the set picked for good (a non-dynamic object's first success)
internal inline uint8_t *scen_obj_name_at(const ee_scen_object *o) { return (uint8_t *)FIELD_AT(o, SCENOBJ_NAME); }
internal inline int scen_obj_unit_count(const ee_scen_object *o) {
    const int *b = FIELD(o, SCENOBJ_UNITS, const int *), *e = FIELD(o, SCENOBJ_UNITS + 4, const int *);
    return b && e > b ? (int)(e - b) : 0;
}
internal inline int scen_obj_unit_id(const ee_scen_object *o, int i) { return FIELD(o, SCENOBJ_UNITS, const int *)[i]; }
internal inline enum sm_player_spec scen_obj_get_players(const ee_scen_object *o) { return FIELD(o, SCENOBJ_PLAYERS, enum sm_player_spec); }
internal inline enum sm_class scen_obj_get_class(const ee_scen_object *o) { return FIELD(o, SCENOBJ_CLASS, enum sm_class); }   // 0 any, 1..14 epoch, 15..71 family, 72..112 RPS, 113+ group
internal inline int scen_obj_get_type(const ee_scen_object *o) { return FIELD(o, SCENOBJ_TYPE, int); }     // 0: by class
internal inline int scen_obj_get_min(const ee_scen_object *o) { return FIELD(o, SCENOBJ_MIN, int); }
internal inline int scen_obj_get_max(const ee_scen_object *o) { return FIELD(o, SCENOBJ_MAX, int); }
internal inline int scen_obj_get_area(const ee_scen_object *o) { return FIELD(o, SCENOBJ_AREA, int); }
internal inline enum sm_relation scen_obj_get_rel_kind(const ee_scen_object *o) { return FIELD(o, SCENOBJ_REL_KIND, enum sm_relation); }   // 0 sight, 1 range, 2 near
internal inline int scen_obj_get_rel_object(const ee_scen_object *o) { return FIELD(o, SCENOBJ_REL_OBJECT, int); }
internal inline int scen_obj_get_parent(const ee_scen_object *o) { return FIELD(o, SCENOBJ_PARENT, int); }
internal inline enum sm_unit_attr scen_obj_get_attr(const ee_scen_object *o) { return FIELD(o, SCENOBJ_ATTR, enum sm_unit_attr); }
internal inline int scen_obj_get_attr_min(const ee_scen_object *o) { return FIELD(o, SCENOBJ_ATTR_MIN, int); }
internal inline int scen_obj_get_attr_max(const ee_scen_object *o) { return FIELD(o, SCENOBJ_ATTR_MAX, int); }
internal inline int scen_obj_get_state(const ee_scen_object *o) { return FIELD(o, SCENOBJ_STATE, int); }
internal inline enum sm_player_spec scen_obj_get_seen_players(const ee_scen_object *o) { return FIELD(o, SCENOBJ_SEEN_PLAYERS, enum sm_player_spec); }
internal inline enum sm_test_mode scen_obj_get_area_mode(const ee_scen_object *o) { return FIELD(o, SCENOBJ_AREA_MODE, enum sm_test_mode); }
internal inline enum sm_test_mode scen_obj_get_rel_mode(const ee_scen_object *o) { return FIELD(o, SCENOBJ_REL_MODE, enum sm_test_mode); }
internal inline enum sm_test_mode scen_obj_get_attr_mode(const ee_scen_object *o) { return FIELD(o, SCENOBJ_ATTR_MODE, enum sm_test_mode); }
internal inline enum sm_test_mode scen_obj_get_seen_mode(const ee_scen_object *o) { return FIELD(o, SCENOBJ_SEEN_MODE, enum sm_test_mode); }
internal inline enum sm_test_mode scen_obj_get_state_mode(const ee_scen_object *o) { return FIELD(o, SCENOBJ_STATE_MODE, enum sm_test_mode); }
internal inline enum sm_test_mode scen_obj_get_selected_mode(const ee_scen_object *o) { return FIELD(o, SCENOBJ_SELECTED_MODE, enum sm_test_mode); }
internal inline char scen_obj_get_locked(const ee_scen_object *o) { return FIELD(o, SCENOBJ_LOCKED, char); }
internal inline char scen_obj_get_dynamic(const ee_scen_object *o) { return FIELD(o, SCENOBJ_DYNAMIC, char); }
internal inline char scen_obj_get_attr_percent(const ee_scen_object *o) { return FIELD(o, SCENOBJ_ATTR_PERCENT, char); }
// an area (EESMArea): its name, its point (-10000: off the map; < 0: a unit's previous place), its rectangle (type
// 0), its type (0 rectangle, 1 the land or sea of its point, 2 a player's shape round it), the AI area it names
internal inline uint8_t *scen_area_name_at(const ee_scen_area *a) { return (uint8_t *)FIELD_AT(a, SCENAREA_NAME); }
internal inline ivec2 scen_area_get_point(const ee_scen_area *a) {
    return iv2(FIELD(a, SCENAREA_POINT, int), FIELD(a, SCENAREA_POINT + 4, int));
}
internal inline uint8_t *scen_area_rect_at(const ee_scen_area *a) { return (uint8_t *)FIELD_AT(a, SCENAREA_RECT); }   // GERectangle
internal inline enum sm_area_type scen_area_get_type(const ee_scen_area *a) { return FIELD(a, SCENAREA_TYPE, enum sm_area_type); }
internal inline int scen_area_get_ai_area(const ee_scen_area *a) { return FIELD(a, SCENAREA_AI_AREA, int); }

// FUNCTIONS
// the lists' entries by id (their index; NULL out of range)
internal inline ee_trigger *sm_trigger(ee_scen_mgr *m, int id) { return GF(SM_GET_TRIGGER)(m, id); }
internal inline ee_scen_cond *sm_cond(ee_scen_mgr *m, int id) { return GF(SM_GET_COND)(m, id); }
internal inline ee_scen_effect *sm_effect(ee_scen_mgr *m, int id) { return GF(SM_GET_EFFECT)(m, id); }
internal inline ee_scen_object *sm_object(ee_scen_mgr *m, int id) { return GF(SM_GET_OBJECT)(m, id); }
internal inline ee_scen_area *sm_area(ee_scen_mgr *m, int id) { return GF(SM_GET_AREA)(m, id); }
// a condition holds now, as the 500 ms check asks it (an object it tests may lock onto its units)
internal inline int sm_cond_holds(ee_scen_cond *c) { return GF(SM_COND_EVAL)(c) != 0; }
// an object picks its units (or filters its locked ones): any left (dead: dying units count too)
internal inline int sm_object_holds(ee_scen_object *o, int dead) { return GF(SM_OBJECT_EVAL)(o, (char)dead) != 0; }
internal inline int sm_area_contains(ee_scen_area *a, ee_unit *u) { return GF(SM_AREA_CONTAINS)(a, u) != 0; }
internal inline int sm_class_test(ee_scen_object *o, int type) { return GF(SM_CLASS_TEST)(o, type) != 0; }
// one player of a player spec, as an effect picks it (-1: none; a random one of "any ...")
internal inline int sm_pick_player(ee_scen_mgr *m, enum sm_player_spec spec) { return GF(SM_PLAYERS)(m, spec, NULL, 1); }
// a player's attribute as conditions read it (resources, population, statistics ...), as effects set it
internal inline int sm_player_attr(ee_scen_cond *c, int player, enum sm_player_attr attr) { return GF(SM_PLAYER_ATTR)(c, player, attr); }
internal inline void sm_set_player_attr(ee_scen_effect *ef, int player, enum sm_player_attr attr, enum sm_attr_mode mode, int v) { GF(SM_SET_PLAYER_ATTR)(ef, player, attr, mode, v); }
// a trigger's check at scenario time now (ms): conditions, true-for, its effects queued
internal inline void sm_trigger_update(ee_trigger *t, float now) { GF(SM_TRIGGER_UPDATE)(t, now); }
// an effect queued to run at scenario time at (ms), as a trigger's delayed effects
internal inline void sm_queue_effect(ee_scen_mgr *m, int effect, int at, int cinematic) { GF(SM_QUEUE_EFFECT)(m, effect, at, (char)cinematic); }
// a scenario effect run now (its trigger's conditions and delay not asked)
internal inline void sm_run_effect(ee_scen_effect *effect, int flag) { GF(SM_RUN_EFFECT)(effect, flag); }
