// THE SCENARIO MANAGER AND ITS TRIGGERS (EEScenarioManager): fields, functions.

// FIELDS (scenario_manager(); test/scen.c, engine-notes.md
// "Scenario triggers"): the lists are vectors (begin, end) - the address of each list's begin
internal inline uint8_t *scen_triggers_at(const uint8_t *m) { return (uint8_t *)FIELD_AT(m, SCEN_TRIGGERS); }
internal inline uint8_t *scen_conds_at(const uint8_t *m) { return (uint8_t *)FIELD_AT(m, SCEN_CONDS); }
internal inline uint8_t *scen_effects_at(const uint8_t *m) { return (uint8_t *)FIELD_AT(m, SCEN_EFFECTS); }
internal inline uint8_t *scen_objects_at(const uint8_t *m) { return (uint8_t *)FIELD_AT(m, SCEN_OBJECTS); }
internal inline uint8_t *scen_areas_at(const uint8_t *m) { return (uint8_t *)FIELD_AT(m, SCEN_AREAS); }
internal inline float scen_get_time(const uint8_t *m) { return FIELD(m, SCEN_TIME, float); }            // +0x210 ms
// a trigger (EESMTrigger): its name and description (UWideString), the lists of its condition and effect references
internal inline uint8_t *trigger_name_at(const uint8_t *t) { return (uint8_t *)FIELD_AT(t, TRIGGER_NAME); }
internal inline uint8_t *trigger_desc_at(const uint8_t *t) { return (uint8_t *)FIELD_AT(t, TRIGGER_DESC); }
internal inline uint8_t *trigger_conds_at(const uint8_t *t) { return (uint8_t *)FIELD_AT(t, TRIGGER_CONDS); }
internal inline uint8_t *trigger_effects_at(const uint8_t *t) { return (uint8_t *)FIELD_AT(t, TRIGGER_EFFECTS); }
internal inline int trigger_get_true_for(const uint8_t *t) { return FIELD(t, TRIGGER_TRUE_FOR, int); }          // +0x4C s the conditions held
internal inline void trigger_set_since(uint8_t *t, int v) { FIELD(t, TRIGGER_SINCE, int) = v; }
internal inline char trigger_get_on(const uint8_t *t) { return FIELD(t, TRIGGER_ON, char); }
internal inline void trigger_set_on(uint8_t *t, char v) { FIELD(t, TRIGGER_ON, char) = v; }
internal inline char trigger_get_fired(const uint8_t *t) { return FIELD(t, TRIGGER_FIRED, char); }
internal inline void trigger_set_fired(uint8_t *t, char v) { FIELD(t, TRIGGER_FIRED, char) = v; }
internal inline char trigger_get_loop(const uint8_t *t) { return FIELD(t, TRIGGER_LOOP, char); }
// a trigger's condition reference: the condition's index, the join to the next (0 end, 1 and, 2 or), not
internal inline int trigger_cond_get_index(const uint8_t *w) { return FIELD(w, TRIGCOND_INDEX, int); }
internal inline int trigger_cond_get_join(const uint8_t *w) { return FIELD(w, TRIGCOND_JOIN, int); }
internal inline char trigger_cond_get_not(const uint8_t *w) { return FIELD(w, TRIGCOND_NOT, char); }
// a trigger's effect reference: its kind (2: "or" between random groups), delay (s), the effect's index
internal inline int trigger_effect_get_kind(const uint8_t *w) { return FIELD(w, TRIGEFFECT_KIND, int); }
internal inline int trigger_effect_get_delay(const uint8_t *w) { return FIELD(w, TRIGEFFECT_DELAY, int); }
internal inline int trigger_effect_get_index(const uint8_t *w) { return FIELD(w, TRIGEFFECT_INDEX, int); }
// a condition: its type (0 always .. 7 chat) and each type's values
internal inline uint8_t *scen_cond_text_at(const uint8_t *c) { return (uint8_t *)FIELD_AT(c, SCENCOND_TEXT); } // chat: the text
internal inline int scen_cond_get_type(const uint8_t *c) { return FIELD(c, SCENCOND_TYPE, int); }
internal inline int scen_cond_get_op(const uint8_t *c) { return FIELD(c, SCENCOND_OP, int); }
internal inline int scen_cond_get_attr(const uint8_t *c) { return FIELD(c, SCENCOND_ATTR, int); }
internal inline int scen_cond_get_tech(const uint8_t *c) { return FIELD(c, SCENCOND_TECH, int); }
internal inline int scen_cond_get_value(const uint8_t *c) { return FIELD(c, SCENCOND_VALUE, int); }
internal inline int scen_cond_get_object(const uint8_t *c) { return FIELD(c, SCENCOND_OBJECT, int); }
internal inline int scen_cond_get_player(const uint8_t *c) { return FIELD(c, SCENCOND_PLAYER, int); }
internal inline int scen_cond_get_time_from(const uint8_t *c) { return FIELD(c, SCENCOND_TIME_FROM, int); }
internal inline int scen_cond_get_time_to(const uint8_t *c) { return FIELD(c, SCENCOND_TIME_TO, int); }
internal inline int scen_cond_get_trigger(const uint8_t *c) { return FIELD(c, SCENCOND_TRIGGER, int); }
internal inline char scen_cond_get_trigger_on(const uint8_t *c) { return FIELD(c, SCENCOND_TRIGGER_ON, char); }
internal inline char scen_cond_get_trigger_fired(const uint8_t *c) { return FIELD(c, SCENCOND_TRIGGER_FIRED, char); }
internal inline char scen_cond_get_timed(const uint8_t *c) { return FIELD(c, SCENCOND_TIMED, char); }
// an effect: its category (1 object, 2 player, 3 trigger, 4 media, 5 game, 6 AI) and its kind within the category
internal inline int scen_effect_get_category(const uint8_t *ef) { return FIELD(ef, SCENEFFECT_CATEGORY, int); }
internal inline int scen_effect_get_kind(const uint8_t *ef, int cat) {
    const uint32_t off[7] = { 0, SCENEFFECT_OBJECT_KIND, SCENEFFECT_PLAYER_KIND, SCENEFFECT_TRIGGER_KIND,
                              SCENEFFECT_MEDIA_KIND, SCENEFFECT_GAME_KIND, SCENEFFECT_AI_KIND };
    return cat >= 1 && cat <= 6 ? FIELD(ef, off[cat], int) : -1;
}
internal inline int scen_effect_get_object(const uint8_t *ef) { return FIELD(ef, SCENEFFECT_OBJECT, int); }     // object effects
internal inline int scen_effect_get_trigger(const uint8_t *ef) { return FIELD(ef, SCENEFFECT_TRIGGER, int); }    // trigger effects

// FUNCTIONS
// the game's objects: the scenario manager, the string resources, the cheat parser, the sound manager, the colour
// table (its rgb records, begin and end)
internal inline uint8_t *scenario_manager(void) { return *(uint8_t **)GA(G_SM_MGR); }
// a scenario effect run now (its trigger's conditions and delay not asked)
internal inline void sm_run_effect(void *effect, int flag) { GF(SM_RUN_EFFECT)(effect, flag); }
