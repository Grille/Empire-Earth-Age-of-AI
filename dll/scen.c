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

#define SM_MGR (*(uint8_t **)0x00918E24)
#define SM_RUN_EFFECT 0x005AD29B

static const char *const k_sm_cond[8] = { "always", "object", "player-attribute", "player-tech", "trigger",
    "diplomacy", "game", "chat" };
static const char *const k_sm_obj[13] = { "create", "freeze", "unfreeze", "kill", "remove", "task-object",
    "task-area", "stance", "unit-attribute", "owner", "class-name", "face", "class-attribute" };
static const char *const k_sm_player[12] = { "victory", "script-camera", "defeat", "diplomacy", "attribute",
    "tech-tree", "civ-points", "fade-in", "fade-out", "visibility-on", "visibility-off", "minimap-flare" };
static const char *const k_sm_media[4] = { "sound", "change-text", "dialogue", "chat" };
static const char *const k_sm_game[7] = { "daylight-hour", "day-night-cycle", "calamity", "graphic-effect",
    "start-cinematic", "end-cinematic", "weather" };
static const char *const k_sm_ai[7] = { "ai-target", "ai-group", "ai-build", "ai-maintain", "ai-toggle",
    "ai-variable", "ai-favourites" };
static const char *const k_sm_trig[4] = { "on", "off", "fired", "not-fired" };
static const char *const k_sm_op[5] = { "<", "<=", "=", ">=", ">" };

static int sm_count(uint8_t *vec_begin) {
    uint8_t **b = FIELD(vec_begin, 0, uint8_t **), **e = FIELD(vec_begin, 4, uint8_t **);
    return b && e > b ? (int)(e - b) : 0;
}
static uint8_t *sm_at(uint8_t *vec_begin, int i) {
    return i >= 0 && i < sm_count(vec_begin) ? FIELD(vec_begin, 0, uint8_t **)[i] : NULL;
}

// UWideString text (8-bit chars despite the name, pointer at +4)
static const char *sm_text(uint8_t *ws, char *buf, int n) {
    const char *w = FIELD(ws, 4, const char *);
    int k = 0;
    for (; w && w[k] && k < n - 1; k++) buf[k] = w[k];
    buf[k] = 0;
    return buf;
}

// a trigger by index or (case-insensitive) name
static uint8_t *sm_trigger(const char *arg, int *idx) {
    uint8_t *m = SM_MGR;
    if (!m) return NULL;
    char *end, name[96];
    long i = strtol(arg, &end, 10);
    if (end != arg && (*end == 0 || *end == ' ')) { *idx = (int)i; return sm_at(m + 0xE8, (int)i); }
    for (int k = 0; k < sm_count(m + 0xE8); k++) {
        uint8_t *t = sm_at(m + 0xE8, k);
        if (t && !_stricmp(sm_text(t + 4, name, sizeof name), arg)) { *idx = k; return t; }
    }
    return NULL;
}

static void sm_list(char *out, uint8_t *m, int detail) {
    char name[96];
    int nt = sm_count(m + 0xE8);
    REPLY("%d triggers, %d conditions, %d effects, %d objects, %d areas; scenario time %.1f s\n", nt,
          sm_count(m + 0xF8), sm_count(m + 0x108), sm_count(m + 0x118), sm_count(m + 0x128), FIELD(m, 0x210, float) / 1000.0f);
    for (int k = 0; k < nt; k++) {
        uint8_t *t = sm_at(m + 0xE8, k);
        if (!t) continue;
        REPLY("%3d \"%s\" %s%s%s true-for %ds:", k, sm_text(t + 4, name, sizeof name), FIELD(t, 0x58, char) ? "on" : "off",
              FIELD(t, 0x59, char) ? " fired" : "", FIELD(t, 0x5A, char) ? " loop" : "", FIELD(t, 0x4C, int));
        for (int c = 0; c < sm_count(t + 0x30); c++) {
            uint8_t *w = sm_at(t + 0x30, c), *cd = w ? sm_at(m + 0xF8, FIELD(w, 4, int)) : NULL;
            if (!w) continue;
            int ty = cd ? FIELD(cd, 0x40, int) : -1, op = cd ? FIELD(cd, 0x44, int) : 0;
            REPLY(" %sc%d %s", FIELD(w, 0xC, char) ? "not " : "", FIELD(w, 4, int), ty >= 0 && ty < 8 ? k_sm_cond[ty] : "?");
            if (ty == 7) REPLY(" \"%s\"", sm_text(cd + 0x2C, name, sizeof name));
            else if (ty == 2) REPLY("(p%d attr %d %s %d)", FIELD(cd, 0x64, int), FIELD(cd, 0x50, int), op >= 0 && op < 5 ? k_sm_op[op] : "?", FIELD(cd, 0x5C, int));
            else if (ty == 3) REPLY("(p%d tech %d)", FIELD(cd, 0x64, int), FIELD(cd, 0x58, int));
            else if (ty == 4) REPLY("(t%d%s%s)", FIELD(cd, 0x78, int), FIELD(cd, 0x80, char) ? " on" : "", FIELD(cd, 0x81, char) ? " fired" : "");
            else if (ty == 1) REPLY("(o%d)", FIELD(cd, 0x60, int));
            else if (ty == 6 && FIELD(cd, 0x83, char)) REPLY("(time %d..%d s)", FIELD(cd, 0x6C, int), FIELD(cd, 0x70, int));
            REPLY("%s", FIELD(w, 8, int) == 1 ? " and" : FIELD(w, 8, int) == 2 ? " or" : "");
        }
        REPLY(" ->");
        for (int e = 0; e < sm_count(t + 0x40); e++) {
            uint8_t *w = sm_at(t + 0x40, e), *ef = w ? sm_at(m + 0x108, FIELD(w, 0xC, int)) : NULL;
            if (!w) continue;
            if (FIELD(w, 4, int) == 2) { REPLY(" |"); continue; }
            // category +0x100: 1 object (+0x104), 2 player (+0xF8), 3 trigger (+0xFC), 4 media (+0xF0), 5 game (+0x13C),
            // 6 AI (+0xE4)
            int cat = ef ? FIELD(ef, 0x100, int) : -1, sub = -1;
            static const int k_sub_off[7] = { 0, 0x104, 0xF8, 0xFC, 0xF0, 0x13C, 0xE4 };
            static const char *const *const k_sub_names[7] = { NULL, k_sm_obj, k_sm_player, k_sm_trig, k_sm_media, k_sm_game, k_sm_ai };
            static const int k_sub_n[7] = { 0, 13, 12, 4, 4, 7, 7 };
            if (cat >= 1 && cat <= 6) sub = FIELD(ef, k_sub_off[cat], int);
            if (cat >= 1 && cat <= 6 && sub >= 0 && sub < k_sub_n[cat]) {
                REPLY(" e%d %s", FIELD(w, 0xC, int), k_sub_names[cat][sub]);
                if (cat == 1) REPLY("(o%d)", FIELD(ef, 0x158, int));
                else if (cat == 3) REPLY("(t%d)", FIELD(ef, 0x180, int));
            }
            else REPLY(" e%d(cat %d/%d)", FIELD(w, 0xC, int), cat, sub);
            if (FIELD(w, 8, int)) REPLY("+%ds", FIELD(w, 8, int));
        }
        REPLY("\n");
        if (detail && FIELD(t, 0x18 + 4, void *)) REPLY("     %s\n", sm_text(t + 0x18, name, sizeof name));
    }
}

// trig [list|desc] | trig on|off|reset T | trig fire T | trig effect N  (T = index or name)
static void cmd_trig(char *out, const char *args) {
    uint8_t *m = SM_MGR;
    char verb[16] = "list";
    int n = 0, idx = -1;
    sscanf(args, "%15s %n", verb, &n);
    const char *arg = args + n;
    if (!m) { REPLY("error: no scenario manager\n"); return; }
    if (!strcmp(verb, "list") || !strcmp(verb, "desc")) { sm_list(out, m, verb[0] == 'd'); return; }
    if (!strcmp(verb, "effect")) {
        int e = atoi(arg);
        uint8_t *ef = sm_at(m + 0x108, e);
        if (!ef) { REPLY("error: no effect %d\n", e); return; }
        ((void (__thiscall *)(void *, int))SM_RUN_EFFECT)(ef, 0);
        REPLY("ok: effect %d (category %d) ran\n", e, FIELD(ef, 0x100, int));
        return;
    }
    uint8_t *t = sm_trigger(arg, &idx);
    if (!t) { REPLY("error: no trigger \"%s\"\n", arg); return; }
    if (!strcmp(verb, "on")) FIELD(t, 0x58, char) = 1;
    else if (!strcmp(verb, "off")) FIELD(t, 0x58, char) = 0;
    else if (!strcmp(verb, "reset")) { FIELD(t, 0x58, char) = 1; FIELD(t, 0x59, char) = 0; FIELD(t, 0x50, int) = 0; }
    else if (!strcmp(verb, "fire")) {
        // every effect now, delays and random groups ignored
        int ran = 0;
        for (int e = 0; e < sm_count(t + 0x40); e++) {
            uint8_t *w = sm_at(t + 0x40, e), *ef = w && FIELD(w, 4, int) != 2 ? sm_at(m + 0x108, FIELD(w, 0xC, int)) : NULL;
            if (ef) { ((void (__thiscall *)(void *, int))SM_RUN_EFFECT)(ef, 0); ran++; }
        }
        FIELD(t, 0x59, char) = 1;
        REPLY("ok: trigger %d fired, %d effects ran\n", idx, ran);
        return;
    }
    else { REPLY("error: trig [list|desc] | on|off|reset|fire T | effect N\n"); return; }
    REPLY("ok: trigger %d %s\n", idx, verb);
}
