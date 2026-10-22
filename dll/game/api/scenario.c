// THE SCENARIO MANAGER AND ITS TRIGGERS (EEScenarioManager): fields, functions.

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
internal inline int scen_cond_get_type(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TYPE, int); }
internal inline int scen_cond_get_op(const ee_scen_cond *c) { return FIELD(c, SCENCOND_OP, int); }
internal inline int scen_cond_get_attr(const ee_scen_cond *c) { return FIELD(c, SCENCOND_ATTR, int); }
internal inline int scen_cond_get_tech(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TECH, int); }
internal inline int scen_cond_get_value(const ee_scen_cond *c) { return FIELD(c, SCENCOND_VALUE, int); }
internal inline int scen_cond_get_object(const ee_scen_cond *c) { return FIELD(c, SCENCOND_OBJECT, int); }
internal inline int scen_cond_get_player(const ee_scen_cond *c) { return FIELD(c, SCENCOND_PLAYER, int); }
internal inline int scen_cond_get_time_from(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TIME_FROM, int); }
internal inline int scen_cond_get_time_to(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TIME_TO, int); }
internal inline int scen_cond_get_trigger(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TRIGGER, int); }
internal inline char scen_cond_get_trigger_on(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TRIGGER_ON, char); }
internal inline char scen_cond_get_trigger_fired(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TRIGGER_FIRED, char); }
internal inline char scen_cond_get_timed(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TIMED, char); }
internal inline uint8_t *scen_cond_name_at(const ee_scen_cond *c) { return (uint8_t *)FIELD_AT(c, SCENCOND_NAME); }
internal inline int scen_cond_get_difficulty(const ee_scen_cond *c) { return FIELD(c, SCENCOND_DIFFICULTY, int); } // game: the difficulty
internal inline int scen_cond_get_stance(const ee_scen_cond *c) { return FIELD(c, SCENCOND_STANCE, int); }         // diplomacy: 0 allied, 1 enemy
internal inline int scen_cond_get_attr2(const ee_scen_cond *c) { return FIELD(c, SCENCOND_ATTR2, int); }           // attribute: the other player's
internal inline int scen_cond_get_player2(const ee_scen_cond *c) { return FIELD(c, SCENCOND_PLAYER2, int); }       // diplomacy, attribute: the other spec
internal inline int scen_cond_get_hour(const ee_scen_cond *c) { return FIELD(c, SCENCOND_HOUR, int); }             // game: at or after that hour
internal inline char scen_cond_get_test_difficulty(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TEST_DIFFICULTY, char); }
internal inline char scen_cond_get_test_hour(const ee_scen_cond *c) { return FIELD(c, SCENCOND_TEST_HOUR, char); }
internal inline char scen_cond_get_vs_player(const ee_scen_cond *c) { return FIELD(c, SCENCOND_VS_PLAYER, char); } // attribute against attribute
// an effect: its category (1 object, 2 player, 3 trigger, 4 media, 5 game, 6 AI) and its kind within the category
internal inline int scen_effect_get_category(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_CATEGORY, int); }
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
internal inline int scen_effect_get_attr_mode(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_ATTR_MODE, int); }       // 0 set, 1 add, 2 subtract
internal inline int scen_effect_get_stance(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_STANCE, int); }             // diplomacy
internal inline int scen_effect_get_msg_players(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_MSG_PLAYERS, int); }   // dialogue, chat
internal inline int scen_effect_get_owner_players(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_OWNER_PLAYERS, int); } // object owner
internal inline int scen_effect_get_players(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_PLAYERS, int); }           // player, AI effects
internal inline int scen_effect_get_target_players(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TARGET_PLAYERS, int); } // diplomacy
internal inline int scen_effect_get_ai_var(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_AI_VAR, int); }             // set AI variable: the record
internal inline int scen_effect_get_ai_toggle(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_AI_TOGGLE, int); }
internal inline int scen_effect_get_op(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_OP, int); }                     // unit / class attribute
internal inline int scen_effect_get_tech(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TECH, int); }
internal inline float scen_effect_get_float(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_FLOAT, float); }           // AI variable
internal inline int scen_effect_get_int1(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_INT1, int); }                 // zoom, AI variable, land favourite
internal inline int scen_effect_get_int2(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_INT2, int); }                 // scroll speed, AI variable, air favourite
internal inline int scen_effect_get_area(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_AREA, int); }                 // AI target / group, camera
internal inline int scen_effect_get_target(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TARGET, int); }             // its object: AI target / group, camera
internal inline int scen_effect_get_calamity(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_CALAMITY, int); }         // calamity, graphic effect
internal inline int scen_effect_get_cal_area(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_CAL_AREA, int); }
internal inline int scen_effect_get_cal_object(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_CAL_OBJECT, int); }
internal inline int scen_effect_get_hour(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_HOUR, int); }                 // daylight, cinematic speed
internal inline int scen_effect_get_day_cycle(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_DAY_CYCLE, int); }
internal inline int scen_effect_get_duration(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_DURATION, int); }         // s
internal inline int scen_effect_get_text_kind(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TEXT_KIND, int); }       // 0 hints, 1 instructions, 2 history
internal inline int scen_effect_get_unit_attr(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_UNIT_ATTR, int); }       // < 20 unit, else class
internal inline int scen_effect_get_value(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_VALUE, int); }               // attribute value, facing (degrees)
internal inline int scen_effect_get_obj_stance(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_OBJ_STANCE, int); }
internal inline int scen_effect_get_task(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TASK, int); }                 // 0 normal .. 5 garrison, 7+ a power
internal inline int scen_effect_get_task_area(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TASK_AREA, int); }
internal inline int scen_effect_get_task_object(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_TASK_OBJECT, int); }
internal inline int scen_effect_get_civ_points(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_CIV_POINTS, int); }
internal inline int scen_effect_get_player_attr(const ee_scen_effect *ef) { return FIELD(ef, SCENEFFECT_PLAYER_ATTR, int); }
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
internal inline int scen_obj_get_players(const ee_scen_object *o) { return FIELD(o, SCENOBJ_PLAYERS, int); }
internal inline int scen_obj_get_class(const ee_scen_object *o) { return FIELD(o, SCENOBJ_CLASS, int); }   // 0 any, 1..14 epoch, 15..71 family, 72..112 RPS, 113+ group
internal inline int scen_obj_get_type(const ee_scen_object *o) { return FIELD(o, SCENOBJ_TYPE, int); }     // 0: by class
internal inline int scen_obj_get_min(const ee_scen_object *o) { return FIELD(o, SCENOBJ_MIN, int); }
internal inline int scen_obj_get_max(const ee_scen_object *o) { return FIELD(o, SCENOBJ_MAX, int); }
internal inline int scen_obj_get_area(const ee_scen_object *o) { return FIELD(o, SCENOBJ_AREA, int); }
internal inline int scen_obj_get_rel_kind(const ee_scen_object *o) { return FIELD(o, SCENOBJ_REL_KIND, int); }   // 0 sight, 1 range, 2 near
internal inline int scen_obj_get_rel_object(const ee_scen_object *o) { return FIELD(o, SCENOBJ_REL_OBJECT, int); }
internal inline int scen_obj_get_parent(const ee_scen_object *o) { return FIELD(o, SCENOBJ_PARENT, int); }
internal inline int scen_obj_get_attr(const ee_scen_object *o) { return FIELD(o, SCENOBJ_ATTR, int); }
internal inline int scen_obj_get_attr_min(const ee_scen_object *o) { return FIELD(o, SCENOBJ_ATTR_MIN, int); }
internal inline int scen_obj_get_attr_max(const ee_scen_object *o) { return FIELD(o, SCENOBJ_ATTR_MAX, int); }
internal inline int scen_obj_get_state(const ee_scen_object *o) { return FIELD(o, SCENOBJ_STATE, int); }
internal inline int scen_obj_get_seen_players(const ee_scen_object *o) { return FIELD(o, SCENOBJ_SEEN_PLAYERS, int); }
internal inline int scen_obj_get_area_mode(const ee_scen_object *o) { return FIELD(o, SCENOBJ_AREA_MODE, int); }
internal inline int scen_obj_get_rel_mode(const ee_scen_object *o) { return FIELD(o, SCENOBJ_REL_MODE, int); }
internal inline int scen_obj_get_attr_mode(const ee_scen_object *o) { return FIELD(o, SCENOBJ_ATTR_MODE, int); }
internal inline int scen_obj_get_seen_mode(const ee_scen_object *o) { return FIELD(o, SCENOBJ_SEEN_MODE, int); }
internal inline int scen_obj_get_state_mode(const ee_scen_object *o) { return FIELD(o, SCENOBJ_STATE_MODE, int); }
internal inline int scen_obj_get_selected_mode(const ee_scen_object *o) { return FIELD(o, SCENOBJ_SELECTED_MODE, int); }
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
internal inline int scen_area_get_type(const ee_scen_area *a) { return FIELD(a, SCENAREA_TYPE, int); }
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
internal inline int sm_pick_player(ee_scen_mgr *m, int spec) { return GF(SM_PLAYERS)(m, spec, NULL, 1); }
// a player's attribute as conditions read it (resources, population, statistics ...), as effects set it
internal inline int sm_player_attr(ee_scen_cond *c, int player, int attr) { return GF(SM_PLAYER_ATTR)(c, player, attr); }
internal inline void sm_set_player_attr(ee_scen_effect *ef, int player, int attr, int mode, int v) { GF(SM_SET_PLAYER_ATTR)(ef, player, attr, mode, v); }
// a trigger's check at scenario time now (ms): conditions, true-for, its effects queued
internal inline void sm_trigger_update(ee_trigger *t, float now) { GF(SM_TRIGGER_UPDATE)(t, now); }
// an effect queued to run at scenario time at (ms), as a trigger's delayed effects
internal inline void sm_queue_effect(ee_scen_mgr *m, int effect, int at, int cinematic) { GF(SM_QUEUE_EFFECT)(m, effect, at, (char)cinematic); }
// a scenario effect run now (its trigger's conditions and delay not asked)
internal inline void sm_run_effect(ee_scen_effect *effect, int flag) { GF(SM_RUN_EFFECT)(effect, flag); }
