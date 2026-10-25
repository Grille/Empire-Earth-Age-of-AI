// THE SCENARIO MANAGER AND ITS TRIGGERS (EEScenarioManager): fields, functions.

// The editor's lists (sm_category, sm_player_spec ...): scenario_lists.c.

// FIELDS (scenario_manager(); test/scen.c, engine-notes.md
// "Scenario triggers"): the lists are vectors (begin, end) - the address of each list's begin
// the last chat line received (UWideString; the 500 ms check empties it - one line a check), the queued effects (a
// vector of {effect id, time ms, cinematic} 12 bytes each: an effect's delay, sm_queue_effect)
internal inline float scen_step(void) { return *(const float *)data_scenario_sm_step; }                       // 500 ms
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
// a condition holds now, as the 500 ms check asks it (an object it tests may lock onto its units)
internal inline int sm_cond_holds(ee_scen_cond *c) { return fn_sm_cond_eval(c) != 0; }
// an object picks its units (or filters its locked ones): any left (dead: dying units count too)
internal inline int sm_object_holds(ee_scen_object *o, int dead) { return fn_sm_object_eval(o, (char)dead) != 0; }
internal inline int sm_area_contains(ee_scen_area *a, ee_unit *u) { return fn_sm_area_contains_game(a, u) != 0; }
internal inline int sm_class_test(ee_scen_object *o, int type) { return fn_sm_class_test_game(o, type) != 0; }
// one player of a player spec, as an effect picks it (-1: none; a random one of "any ...")
internal inline int sm_pick_player(ee_scen_mgr *m, enum sm_player_spec spec) { return fn_sm_players(m, spec, NULL, 1); }
// a player's attribute as conditions read it (resources, population, statistics ...), as effects set it
// a trigger's check at scenario time now (ms): conditions, true-for, its effects queued
// an effect queued to run at scenario time at (ms), as a trigger's delayed effects
// a scenario effect run now (its trigger's conditions and delay not asked)
