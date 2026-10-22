// SCENARIO TRIGGERS: read and drive the scenario's triggers from the test pipe (trig). Included by eeaimod.c.
//
// The runtime (docs/engine-notes.md "Scenario triggers"): EEScenarioManager [0x918E24] keeps vectors (begin, end) of
// triggers +0xE8, conditions +0xF8, effects +0x108, objects +0x118 and areas +0x128; triggers refer to conditions and
// effects by index. Every 500 ms of game time 0x5A1ED2 updates each trigger (0x5B93B0) and runs the queued effects
// (+0x168, 12-byte entries {effect, time, cinematic, done}) whose time has come; +0x210 scenario time (ms).
// EESMTrigger: +4 name, +0x18 description (UWideString, text at +4 of each), +0x30 condition wrappers, +0x40 effect
// wrappers, +0x4C conditions true for (s), +0x50 since, +0x54 an int it saves (not its index), +0x58 on, +0x59 fired, +0x5A looping,
// +0x5B active in cinematics. Condition wrapper: +4 condition, +8 joins the next (0 end, 1 and, 2 or), +0xC not.
// Effect wrapper: +4 kind (2 = "or" between random groups), +8 delay (s), +0xC effect.
// An effect runs with 0x5AD29B thiscall(effect, bool cinematic) - on the game thread.

#define SM_MGR (scenario_manager())

internal const char *const k_sm_cond[8] = { "always", "object", "player-attribute", "player-tech", "trigger",
    "diplomacy", "game", "chat" };
internal const char *const k_sm_obj[13] = { "create", "freeze", "unfreeze", "kill", "remove", "task-object",
    "task-area", "stance", "unit-attribute", "owner", "class-name", "face", "class-attribute" };
internal const char *const k_sm_player[12] = { "victory", "script-camera", "defeat", "diplomacy", "attribute",
    "tech-tree", "civ-points", "fade-in", "fade-out", "visibility-on", "visibility-off", "minimap-flare" };
internal const char *const k_sm_media[4] = { "sound", "change-text", "dialogue", "chat" };
internal const char *const k_sm_game[7] = { "daylight-hour", "day-night-cycle", "calamity", "graphic-effect",
    "start-cinematic", "end-cinematic", "weather" };
internal const char *const k_sm_ai[7] = { "ai-target", "ai-group", "ai-build", "ai-maintain", "ai-toggle",
    "ai-variable", "ai-favourites" };
internal const char *const k_sm_trig[4] = { "on", "off", "fired", "not-fired" };
internal const char *const k_sm_op[5] = { "<", "<=", "=", ">=", ">" };

internal int sm_count(uint8_t *vec_begin) {
    uint8_t **b = FIELD(vec_begin, 0, uint8_t **), **e = FIELD(vec_begin, 4, uint8_t **);
    return b && e > b ? (int)(e - b) : 0;
}
internal void *sm_at(uint8_t *vec_begin, int i) {   // an element of a vector of pointers
    return i >= 0 && i < sm_count(vec_begin) ? FIELD(vec_begin, 0, uint8_t **)[i] : NULL;
}

// UWideString text (8-bit chars despite the name, pointer at +4)
internal const char *sm_text(uint8_t *ws, char *buf, int n) {
    const char *w = FIELD(ws, 4, const char *);
    int k = 0;
    for (; w && w[k] && k < n - 1; k++) buf[k] = w[k];
    buf[k] = 0;
    return buf;
}

// a trigger by index or (case-insensitive) name
internal ee_trigger *sm_find_trigger(const char *arg, int *idx) {
    ee_scen_mgr *m = SM_MGR;
    if (!m) return NULL;
    char *end, name[96];
    long i = strtol(arg, &end, 10);
    if (end != arg && (*end == 0 || *end == ' ')) { *idx = (int)i; return sm_at(scen_triggers_at(m), (int)i); }
    for (int k = 0; k < sm_count(scen_triggers_at(m)); k++) {
        ee_trigger *t = sm_at(scen_triggers_at(m), k);
        if (t && !_stricmp(sm_text(trigger_name_at(t), name, sizeof name), arg)) { *idx = k; return t; }
    }
    return NULL;
}

internal void sm_list(char *out, ee_scen_mgr *m, int detail) {
    char name[96];
    int nt = sm_count(scen_triggers_at(m));
    REPLY("%d triggers, %d conditions, %d effects, %d objects, %d areas; scenario time %.1f s\n", nt,
          sm_count(scen_conds_at(m)), sm_count(scen_effects_at(m)), sm_count(scen_objects_at(m)), sm_count(scen_areas_at(m)), scen_get_time(m) / 1000.0f);
    for (int k = 0; k < nt; k++) {
        ee_trigger *t = sm_at(scen_triggers_at(m), k);
        if (!t) continue;
        REPLY("%3d \"%s\" %s%s%s true-for %ds:", k, sm_text(trigger_name_at(t), name, sizeof name), trigger_get_on(t) ? "on" : "off",
              trigger_get_fired(t) ? " fired" : "", trigger_get_loop(t) ? " loop" : "", trigger_get_true_for(t));
        for (int c = 0; c < sm_count(trigger_conds_at(t)); c++) {
            ee_trig_cond *w = sm_at(trigger_conds_at(t), c); ee_scen_cond *cd = w ? sm_at(scen_conds_at(m), trigger_cond_get_index(w)) : NULL;
            if (!w) continue;
            int ty = cd ? scen_cond_get_type(cd) : -1, op = cd ? scen_cond_get_op(cd) : 0;
            REPLY(" %sc%d %s", trigger_cond_get_not(w) ? "not " : "", trigger_cond_get_index(w), ty >= 0 && ty < 8 ? k_sm_cond[ty] : "?");
            if (ty == 7) REPLY(" \"%s\"", sm_text(scen_cond_text_at(cd), name, sizeof name));
            else if (ty == 2) REPLY("(p%d attr %d %s %d)", scen_cond_get_player(cd), scen_cond_get_attr(cd), op >= 0 && op < 5 ? k_sm_op[op] : "?", scen_cond_get_value(cd));
            else if (ty == 3) REPLY("(p%d tech %d)", scen_cond_get_player(cd), scen_cond_get_tech(cd));
            else if (ty == 4) REPLY("(t%d%s%s)", scen_cond_get_trigger(cd), scen_cond_get_trigger_on(cd) ? " on" : "", scen_cond_get_trigger_fired(cd) ? " fired" : "");
            else if (ty == 1) REPLY("(o%d)", scen_cond_get_object(cd));
            else if (ty == 6 && scen_cond_get_timed(cd)) REPLY("(time %d..%d s)", scen_cond_get_time_from(cd), scen_cond_get_time_to(cd));
            REPLY("%s", trigger_cond_get_join(w) == 1 ? " and" : trigger_cond_get_join(w) == 2 ? " or" : "");
        }
        REPLY(" ->");
        for (int e = 0; e < sm_count(trigger_effects_at(t)); e++) {
            ee_trig_effect *w = sm_at(trigger_effects_at(t), e); ee_scen_effect *ef = w ? sm_at(scen_effects_at(m), trigger_effect_get_index(w)) : NULL;
            if (!w) continue;
            if (trigger_effect_get_kind(w) == 2) { REPLY(" |"); continue; }
            int cat = ef ? scen_effect_get_category(ef) : -1, sub = -1;
            static const char *const *const k_sub_names[7] = { NULL, k_sm_obj, k_sm_player, k_sm_trig, k_sm_media, k_sm_game, k_sm_ai };
            static const int k_sub_n[7] = { 0, 13, 12, 4, 4, 7, 7 };
            if (cat >= 1 && cat <= 6) sub = scen_effect_get_kind(ef, cat);
            if (cat >= 1 && cat <= 6 && sub >= 0 && sub < k_sub_n[cat]) {
                REPLY(" e%d %s", trigger_effect_get_index(w), k_sub_names[cat][sub]);
                if (cat == 1) REPLY("(o%d)", scen_effect_get_object(ef));
                else if (cat == 3) REPLY("(t%d)", scen_effect_get_trigger(ef));
            }
            else REPLY(" e%d(cat %d/%d)", trigger_effect_get_index(w), cat, sub);
            if (trigger_effect_get_delay(w)) REPLY("+%ds", trigger_effect_get_delay(w));
        }
        REPLY("\n");
        if (detail && FIELD(trigger_desc_at(t), 4, void *)) REPLY("     %s\n", sm_text(trigger_desc_at(t), name, sizeof name));
    }
}

// trig objects|effects FROM lists SM_PAGE of them from FROM (a campaign scenario has hundreds: more than one reply holds)
#define SM_PAGE 100
// an object's class filter as the editor's lists have it (the ranges by build): "epoch 3", "family 36" ...
internal const char *sm_class_kind(enum sm_class c, int *n) {
    if (c == SM_CLASS_ANY) { *n = 0; return "any "; }
    if (c >= sm_class_group(0)) { *n = (int)(c - sm_class_group(0)); return "group "; }
    if (c >= sm_class_rps(0)) { *n = (int)(c - sm_class_rps(0)); return "rps "; }
    if (c >= sm_class_family(0)) { *n = (int)(c - sm_class_family(0)); return "family "; }
    *n = (int)c;
    return "epoch ";
}
// trig objects: each object's filter and what the game's own evaluation picks now (it may lock the object)
internal void sm_objects(char *out, ee_scen_mgr *m, int from) {
    char name[96];
    for (int k = from; k < sm_count(scen_objects_at(m)) && k < from + SM_PAGE; k++) {
        ee_scen_object *o = sm_object(m, k);
        if (!o) continue;
        int cn = 0;
        const char *ck = sm_class_kind(scen_obj_get_class(o), &cn);
        REPLY("o%d \"%s\" players %d class %s%d (%d) type %d count %d..%d area %d(%d) rel o%d kind %d(%d) parent o%d attr %d %d..%d(%d%s) state %d(%d) seen %d(%d) sel(%d)%s%s",
              k, sm_text(scen_obj_name_at(o), name, sizeof name), scen_obj_get_players(o), ck, cn, scen_obj_get_class(o), scen_obj_get_type(o),
              scen_obj_get_min(o), scen_obj_get_max(o), scen_obj_get_area(o), scen_obj_get_area_mode(o), scen_obj_get_rel_object(o),
              scen_obj_get_rel_kind(o), scen_obj_get_rel_mode(o), scen_obj_get_parent(o), scen_obj_get_attr(o), scen_obj_get_attr_min(o),
              scen_obj_get_attr_max(o), scen_obj_get_attr_mode(o), scen_obj_get_attr_percent(o) ? " %" : "", scen_obj_get_state(o),
              scen_obj_get_state_mode(o), scen_obj_get_seen_players(o), scen_obj_get_seen_mode(o), scen_obj_get_selected_mode(o),
              scen_obj_get_locked(o) ? " locked" : "", scen_obj_get_dynamic(o) ? " dynamic" : "");
        int holds = sm_object_holds(o, 0), n = scen_obj_unit_count(o);
        REPLY(" -> %s, %d units", holds ? "holds" : "no", n);
        for (int i = 0; i < n && i < 6; i++) REPLY(" %d", scen_obj_unit_id(o, i));
        if (n && scen_obj_get_type(o)) REPLY(" (type %d passes the class test: %d)", scen_obj_get_type(o), sm_class_test(o, scen_obj_get_type(o)));
        REPLY("\n");
    }
}
// trig areas: each area, and how many units on the map it contains by the game's own test
internal void sm_areas(char *out, ee_scen_mgr *m) {
    char name[96];
    for (int k = 0; k < sm_count(scen_areas_at(m)); k++) {
        ee_scen_area *a = sm_area(m, k);
        if (!a) continue;
        ivec2 p = scen_area_get_point(a);
        int in = 0;
        for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) in += sm_area_contains(a, it.u);
        REPLY("a%d \"%s\" type %d at %d,%d ai-area %d: %d units in it\n", k, sm_text(scen_area_name_at(a), name, sizeof name),
              scen_area_get_type(a), p.x, p.y, scen_area_get_ai_area(a), in);
    }
}
// trig effects: each effect's category, kind and the values that category reads
internal void sm_effects(char *out, ee_scen_mgr *m, int from) {
    char name[96];
    for (int k = from; k < sm_count(scen_effects_at(m)) && k < from + SM_PAGE; k++) {
        ee_scen_effect *ef = sm_effect(m, k);
        if (!ef) continue;
        int cat = scen_effect_get_category(ef), sub = scen_effect_get_kind(ef, cat);
        REPLY("e%d cat %d kind %d:", k, cat, sub);
        if (cat == 1) REPLY(" o%d owner %d attr %d op %d value %d%s task %d area %d target o%d stance %d", scen_effect_get_object(ef),
                            scen_effect_get_owner_players(ef), scen_effect_get_unit_attr(ef), scen_effect_get_op(ef), scen_effect_get_value(ef),
                            scen_effect_get_percent(ef) ? " %" : "", scen_effect_get_task(ef), scen_effect_get_task_area(ef),
                            scen_effect_get_task_object(ef), scen_effect_get_obj_stance(ef));
        else if (cat == 2) REPLY(" players %d to %d stance %d attr %d mode %d amount %d tech %d%s civ %d flare a%d/o%d%s", scen_effect_get_players(ef),
                                 scen_effect_get_target_players(ef), scen_effect_get_stance(ef), scen_effect_get_player_attr(ef),
                                 scen_effect_get_attr_mode(ef), scen_effect_get_amount(ef), scen_effect_get_tech(ef),
                                 scen_effect_get_tech_add(ef) ? " add" : "", scen_effect_get_civ_points(ef), scen_effect_get_cam_area(ef),
                                 scen_effect_get_cam_object(ef), scen_effect_get_flare_area(ef) ? " by area" : "");
        else if (cat == 3) REPLY(" t%d", scen_effect_get_trigger(ef));
        else if (cat == 4) REPLY(" to %d text %d%s \"%s\" for %ds", scen_effect_get_msg_players(ef), scen_effect_get_text_kind(ef),
                                 scen_effect_get_clear(ef) ? " clear" : "", sm_text(scen_effect_text_at(ef), name, sizeof name), scen_effect_get_duration(ef));
        else if (cat == 5) REPLY(" hour %d cycle %d calamity %d a%d o%d%s", scen_effect_get_hour(ef), scen_effect_get_day_cycle(ef),
                                 scen_effect_get_calamity(ef), scen_effect_get_cal_area(ef), scen_effect_get_cal_object(ef),
                                 scen_effect_get_cal_use_area(ef) ? " by area" : "");
        else if (cat == 6) REPLY(" players %d area a%d target o%d%s%s%s%s toggle %d%s var %d %d %d %.2f", scen_effect_get_players(ef),
                                 scen_effect_get_area(ef), scen_effect_get_target(ef), scen_effect_get_use_area(ef) ? " by area" : "",
                                 scen_effect_get_attack(ef) ? " attack" : " defend", scen_effect_get_add(ef) ? " add" : "",
                                 scen_effect_get_favorites(ef) ? " favourites" : "", scen_effect_get_ai_toggle(ef), scen_effect_get_on(ef) ? " on" : "",
                                 scen_effect_get_ai_var(ef), scen_effect_get_int1(ef), scen_effect_get_int2(ef), scen_effect_get_float(ef));
        REPLY("\n");
    }
}
// trig eval: each condition by the game's own evaluation now (objects it tests may lock), with its key values
internal void sm_eval(char *out, ee_scen_mgr *m) {
    for (int k = 0; k < sm_count(scen_conds_at(m)); k++) {
        ee_scen_cond *c = sm_cond(m, k);
        if (!c) continue;
        int ty = scen_cond_get_type(c);
        REPLY("c%d %s: %s", k, ty >= 0 && ty < 8 ? k_sm_cond[ty] : "?", sm_cond_holds(c) ? "true" : "false");
        if (ty == 2) {
            int p = sm_pick_player(m, scen_cond_get_player(c));
            REPLY(" (p%d attr %d = %d%s, vs p%d attr %d)", p, scen_cond_get_attr(c), p >= 0 ? sm_player_attr(c, p, scen_cond_get_attr(c)) : 0,
                  scen_cond_get_vs_player(c) ? "" : " - against the constant", scen_cond_get_player2(c), scen_cond_get_attr2(c));
        } else if (ty == 5) REPLY(" (p%d to p%d stance %d)", scen_cond_get_player(c), scen_cond_get_player2(c), scen_cond_get_stance(c));
        else if (ty == 6) REPLY(" (%s%s%s difficulty %d hour %d)", scen_cond_get_timed(c) ? "time " : "", scen_cond_get_test_hour(c) ? "hour " : "",
                                scen_cond_get_test_difficulty(c) ? "difficulty" : "", scen_cond_get_difficulty(c), scen_cond_get_hour(c));
        REPLY("\n");
    }
}
// trig check: the game's getters against the lists, the step, what is queued, the chat line
internal void sm_check(char *out, ee_scen_mgr *m) {
    int bad = 0;
#define SM_CHECK(c, ...) do { if (c) { bad++; REPLY(__VA_ARGS__); } } while (0)
    for (int k = 0; k < sm_count(scen_triggers_at(m)); k++) {
        SM_CHECK(sm_trigger(m, k) != sm_at(scen_triggers_at(m), k), "trigger %d: the getter's %p, the list's %p\n", k, (void *)sm_trigger(m, k), sm_at(scen_triggers_at(m), k));
    }
    for (int k = 0; k < sm_count(scen_conds_at(m)); k++) SM_CHECK(sm_cond(m, k) != sm_at(scen_conds_at(m), k), "condition %d\n", k);
    for (int k = 0; k < sm_count(scen_effects_at(m)); k++) SM_CHECK(sm_effect(m, k) != sm_at(scen_effects_at(m), k), "effect %d\n", k);
    for (int k = 0; k < sm_count(scen_objects_at(m)); k++) SM_CHECK(sm_object(m, k) != sm_at(scen_objects_at(m), k), "object %d\n", k);
    for (int k = 0; k < sm_count(scen_areas_at(m)); k++) SM_CHECK(sm_area(m, k) != sm_at(scen_areas_at(m), k), "area %d\n", k);
    SM_CHECK(sm_trigger(m, sm_count(scen_triggers_at(m))) != NULL, "trigger %d (out of range) not NULL\n", sm_count(scen_triggers_at(m)));
#undef SM_CHECK
    REPLY("%s: %d mismatches; trigger 0's +0x54 %d; step %.0f ms, queued %d bytes, chat \"%s\"\n", bad ? "error" : "ok", bad,
          sm_count(scen_triggers_at(m)) ? trigger_get_0x54(sm_at(scen_triggers_at(m), 0)) : -1, scen_step(),
          (int)(FIELD(scen_queue_at(m), 4, uint8_t *) - FIELD(scen_queue_at(m), 0, uint8_t *)), FIELD(scen_chat_at(m), 4, const char *) ? FIELD(scen_chat_at(m), 4, const char *) : "");
}

// trig [list|desc|areas|eval|check] | trig objects|effects [FROM] | trig attr P A | trig on|off|reset T | trig fire T | trig effect N
// (T = index or name)
internal void cmd_trig(char *out, const char *args) {
    ee_scen_mgr *m = SM_MGR;
    char verb[16] = "list";
    int n = 0, idx = -1;
    sscanf(args, "%15s %n", verb, &n);
    const char *arg = args + n;
    if (!m) { REPLY("error: no scenario manager\n"); return; }
    if (!strcmp(verb, "list") || !strcmp(verb, "desc")) { sm_list(out, m, verb[0] == 'd'); return; }
    if (!strcmp(verb, "objects")) { REPLY("ok: objects from %d of %d\n", atoi(arg), sm_count(scen_objects_at(m))); sm_objects(out, m, atoi(arg)); return; }
    if (!strcmp(verb, "areas")) { sm_areas(out, m); return; }
    if (!strcmp(verb, "effects")) { REPLY("ok: effects from %d of %d\n", atoi(arg), sm_count(scen_effects_at(m))); sm_effects(out, m, atoi(arg)); return; }
    if (!strcmp(verb, "eval")) { sm_eval(out, m); return; }
    if (!strcmp(verb, "check")) { sm_check(out, m); return; }
    if (!strcmp(verb, "attr")) {   // a player's attribute as conditions read it (the first condition as `this`: only attribute 0 uses it)
        int p = 0, a = 0;
        sscanf(arg, "%d %d", &p, &a);
        ee_scen_cond *c = sm_cond(m, 0);
        if (!c) { REPLY("error: the scenario has no condition\n"); return; }
        REPLY("ok: p%d attribute %d = %d\n", p, a, sm_player_attr(c, p, a));
        return;
    }
    if (!strcmp(verb, "effect")) {
        int e = atoi(arg);
        ee_scen_effect *ef = sm_at(scen_effects_at(m), e);
        if (!ef) { REPLY("error: no effect %d\n", e); return; }
        sm_run_effect(ef, 0);
        REPLY("ok: effect %d (category %d) ran\n", e, scen_effect_get_category(ef));
        return;
    }
    ee_trigger *t = sm_find_trigger(arg, &idx);
    if (!t) { REPLY("error: no trigger \"%s\"\n", arg); return; }
    if (!strcmp(verb, "on")) trigger_set_on(t, 1);
    else if (!strcmp(verb, "off")) trigger_set_on(t, 0);
    else if (!strcmp(verb, "reset")) { trigger_set_on(t, 1); trigger_set_fired(t, 0); trigger_set_since(t, 0); }
    else if (!strcmp(verb, "fire")) {
        // every effect now, delays and random groups ignored
        int ran = 0;
        for (int e = 0; e < sm_count(trigger_effects_at(t)); e++) {
            ee_trig_effect *w = sm_at(trigger_effects_at(t), e); ee_scen_effect *ef = w && trigger_effect_get_kind(w) != 2 ? sm_at(scen_effects_at(m), trigger_effect_get_index(w)) : NULL;
            if (ef) { sm_run_effect(ef, 0); ran++; }
        }
        trigger_set_fired(t, 1);
        REPLY("ok: trigger %d fired, %d effects ran\n", idx, ran);
        return;
    }
    else { REPLY("error: trig [list|desc|areas|eval|check] | objects|effects [FROM] | attr P A | on|off|reset|fire T | effect N\n"); return; }
    REPLY("ok: trigger %d %s\n", idx, verb);
}

