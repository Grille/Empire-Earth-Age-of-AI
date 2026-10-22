// SCENARIO TRIGGERS: read and drive the scenario's triggers from the test pipe (trig). Included by eeaimod.c.
//
// The runtime (docs/engine-notes.md "Scenario triggers"): EEScenarioManager [0x918E24] keeps vectors (begin, end) of
// triggers +0xE8, conditions +0xF8, effects +0x108, objects +0x118 and areas +0x128; triggers refer to conditions and
// effects by index. Every 500 ms of game time 0x5A1ED2 updates each trigger (0x5B93B0) and runs the queued effects
// (+0x168, 12-byte entries {effect, time, cinematic, done}) whose time has come; +0x210 scenario time (ms).
// EESMTrigger: +4 name, +0x18 description (UWideString, text at +4 of each), +0x30 condition wrappers, +0x40 effect
// wrappers, +0x4C conditions true for (s), +0x50 since, +0x54 id, +0x58 on, +0x59 fired, +0x5A looping,
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
internal ee_trigger *sm_trigger(const char *arg, int *idx) {
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

// trig [list|desc] | trig on|off|reset T | trig fire T | trig effect N  (T = index or name)
internal void cmd_trig(char *out, const char *args) {
    ee_scen_mgr *m = SM_MGR;
    char verb[16] = "list";
    int n = 0, idx = -1;
    sscanf(args, "%15s %n", verb, &n);
    const char *arg = args + n;
    if (!m) { REPLY("error: no scenario manager\n"); return; }
    if (!strcmp(verb, "list") || !strcmp(verb, "desc")) { sm_list(out, m, verb[0] == 'd'); return; }
    if (!strcmp(verb, "effect")) {
        int e = atoi(arg);
        ee_scen_effect *ef = sm_at(scen_effects_at(m), e);
        if (!ef) { REPLY("error: no effect %d\n", e); return; }
        sm_run_effect(ef, 0);
        REPLY("ok: effect %d (category %d) ran\n", e, scen_effect_get_category(ef));
        return;
    }
    ee_trigger *t = sm_trigger(arg, &idx);
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
    else { REPLY("error: trig [list|desc] | on|off|reset|fire T | effect N\n"); return; }
    REPLY("ok: trigger %d %s\n", idx, verb);
}

