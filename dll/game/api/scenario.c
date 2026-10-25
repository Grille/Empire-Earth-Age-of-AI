// THE SCENARIO MANAGER AND ITS TRIGGERS (EEScenarioManager): fields, functions.

// The editor's lists (sm_category, sm_player_spec ...): scenario_lists.c.

// FIELDS (scenario_manager(); test/scen.c, engine-notes.md
// "Scenario triggers"): the lists are vectors (begin, end) - the address of each list's begin
// the last chat line received (UWideString; the 500 ms check empties it - one line a check), the queued effects (a
// vector of {effect id, time ms, cinematic} 12 bytes each: an effect's delay, SM_QUEUE_EFFECT)
internal inline float scen_step(void) { return *(const float *)GA(G_SM_STEP); }                       // 500 ms
// a trigger (EESMTrigger): its name and description (UWideString), the lists of its condition and effect references
// a trigger's condition reference: the condition's index, the join to the next (0 end, 1 and, 2 or), not
// a trigger's effect reference: its kind (2: "or" between random groups), delay (s), the effect's index
// a condition: its type (0 always .. 7 chat) and each type's values
// an effect: its category (1 object, 2 player, 3 trigger, 4 media, 5 game, 6 AI) and its kind within the category
internal inline int scen_effect_get_kind(const ee_scen_effect *ef, int cat) {
    const uint32_t off[7] = { 0, of_scen_effect_object_kind, of_scen_effect_player_kind, of_scen_effect_trigger_kind,
                              of_scen_effect_media_kind, of_scen_effect_game_kind, of_scen_effect_ai_kind };
    return cat >= 1 && cat <= 6 ? FIELD(ef, off[cat], int) : -1;
}
// an effect's values by category (docs/triggers.md "effects"): the player specs (each a player spec as conditions
// have), the object, area and trigger ids (indices), the texts (UWideString / CString: their addresses)
// an object (EESMObject): the units picked (their ids: begin, end), by owner spec, class / type, count, area, a relation
// to another object, a parent's units, an attribute range, a state, who sees / selects them - each test's mode 0
// ignore, 1 must, 2 must not. Locked: the set picked for good (a non-dynamic object's first success)
internal inline int scen_obj_unit_count(const ee_scen_object *o) {
    const int *b = FIELD(o, of_scen_obj_units, const int *), *e = FIELD(o, of_scen_obj_units + 4, const int *);
    return b && e > b ? (int)(e - b) : 0;
}
internal inline int scen_obj_unit_id(const ee_scen_object *o, int i) { return FIELD(o, of_scen_obj_units, const int *)[i]; }
// an area (EESMArea): its name, its point (-10000: off the map; < 0: a unit's previous place), its rectangle (type
// 0), its type (0 rectangle, 1 the land or sea of its point, 2 a player's shape round it), the AI area it names
internal inline ivec2 scen_area_get_point(const ee_scen_area *a) {
    return iv2(FIELD(a, of_scen_area_point, int), FIELD(a, of_scen_area_point + 4, int));
}

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
