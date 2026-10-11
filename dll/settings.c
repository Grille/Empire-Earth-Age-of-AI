// User settings per AI difficulty (aimod.ini next to the DLL). Every key is looked up in the difficulty's
// own section ([easy], [medium], [hard]), then in [all], then falls back to the built-in default (for keys an
// older file doesn't have yet; the file itself is required, see config_missing in eeaimod.c). The autotest ini can override
// any key for all difficulties (A/B tests). Included by eeaimod.c before sites.c/econ.c.
//
// Difficulty = player+0xBA4 (0 easy, 1 medium, 2 hard).
// Cost: AdjustCost (vtable 0x8264C0 slot 27, 0x430FBD) multiplies prices by pct/100 while the player's
//   Cheat toggle (+0xEFB) is on; pct is +0xD48 (easy, int), +0xBB4 (medium, float), +0xD4C (hard, int).
// Wait for the player: toggle "Ignorieren" (+0xF02) = 0 -> AttemptEpochAdvance (0x46C966) postpones while
//   the AI would get ahead of the highest human epoch, and the epoch timer is x2 ahead / /2 behind.
// Epoch timer: SetNextEpochAdvanceTime (0x46DE8C) draws player+0xD58..+0xD5C minutes (x difficulty factor).
// Attacks (copied from the AI settings table by 0x431697): first attack min/max minutes +0xD2C/+0xD28,
//   minutes between attacks min/max +0xD00/+0xCFC (next attack time +0xED8, scheduled by 0x43DD2F; +0xEF5 =
//   first attack done), attack group size percent easy/medium/hard +0xD30/+0xD34/+0xD38 (0x43103F),
//   immediate-attack chances in percent: on contact with an enemy +0xCD8, to take resources +0xCDC,
//   revenge +0xCE0.

struct ai_cfg {
    int copper_plan, budget, safe_sites;
    float mil_start_share, mil_income_share, mil_income_share_farms;   // income share before / from the Copper Age (farms)
    int cost_percent;       // 0 = leave the game's Cheat setting alone
    int pacing_fast;        // 1 = advance as soon as affordable (all epochs)
    int epoch_min, epoch_max;
    int wait_for_player;    // -1 = leave, 0 = ignore humans, 1 = never get ahead of them
    int first_attack_min, first_attack_max, attack_min, attack_max;   // minutes, 0 = leave
    int attack_group_percent;                                         // 0 = leave
    int attack_on_contact, attack_for_resources, attack_revenge;      // percent, -1 = leave
    float citizen_share;    // share of the population cap for citizens (+0xBDC, the rest is military), 0 = leave
    float home_guard_share; // < 0 = [all] home_guard_share (defence.c)
};
static struct ai_cfg g_cfg[3];
static const char *const k_diff_sections[3] = { "easy", "medium", "hard" };

// Per player: the difficulty's settings with a personality's [style] on top (persona.c, only with ai_style=1).
static struct ai_cfg g_cfg_pl[16];
static uint8_t g_cfg_pl_on[16];

static const struct ai_cfg *cfg_of(void *pl) {
    if (pl) {
        unsigned i = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
        if (g_cfg_pl_on[i]) return &g_cfg_pl[i];
    }
    int d = pl ? FIELD(pl, PL_DIFFICULTY, int) : 1;
    return &g_cfg[d < 0 ? 0 : d > 2 ? 2 : d];
}

// The settings a personality's [style] may change - the same keys as in aimod.ini. Not the structural ones
// (copper_plan, budget, safe_sites, epoch_pacing) nor the ones that would cheat or bind to the human
// (cost_percent, wait_for_player).
static const struct { const char *key; size_t off; char is_float; } k_style_keys[] = {
    { "military_start_share", offsetof(struct ai_cfg, mil_start_share), 1 },
    { "military_income_share", offsetof(struct ai_cfg, mil_income_share), 1 },
    { "military_income_share_farms", offsetof(struct ai_cfg, mil_income_share_farms), 1 },
    { "epoch_min", offsetof(struct ai_cfg, epoch_min), 0 },
    { "epoch_max", offsetof(struct ai_cfg, epoch_max), 0 },
    { "first_attack_min", offsetof(struct ai_cfg, first_attack_min), 0 },
    { "first_attack_max", offsetof(struct ai_cfg, first_attack_max), 0 },
    { "attack_interval_min", offsetof(struct ai_cfg, attack_min), 0 },
    { "attack_interval_max", offsetof(struct ai_cfg, attack_max), 0 },
    { "attack_group_percent", offsetof(struct ai_cfg, attack_group_percent), 0 },
    { "attack_on_contact", offsetof(struct ai_cfg, attack_on_contact), 0 },
    { "attack_for_resources", offsetof(struct ai_cfg, attack_for_resources), 0 },
    { "attack_revenge", offsetof(struct ai_cfg, attack_revenge), 0 },
    { "citizen_share", offsetof(struct ai_cfg, citizen_share), 1 },
    { "home_guard_share", offsetof(struct ai_cfg, home_guard_share), 1 },
};
// one key=value onto c; 0 = unknown key
static int cfg_style_set(struct ai_cfg *c, const char *key, const char *val) {
    for (size_t k = 0; k < sizeof k_style_keys / sizeof *k_style_keys; k++) {
        if (lstrcmpiA(key, k_style_keys[k].key)) continue;
        if (k_style_keys[k].is_float) *(float *)((uint8_t *)c + k_style_keys[k].off) = (float)atof(val);
        else *(int *)((uint8_t *)c + k_style_keys[k].off) = atoi(val);
        return 1;
    }
    return 0;
}
static void cfg_style_fix(struct ai_cfg *c) {   // a max below its min follows the min
    if (c->epoch_max < c->epoch_min) c->epoch_max = c->epoch_min;
    if (c->first_attack_max < c->first_attack_min) c->first_attack_max = c->first_attack_min;
    if (c->attack_max < c->attack_min) c->attack_max = c->attack_min;
}

static int cfg_int(const char *ini, const char *test_ini, int d, const char *key, int def) {
    int v = GetPrivateProfileIntA("all", key, def, ini);
    v = GetPrivateProfileIntA(k_diff_sections[d], key, v, ini);
    if (test_ini) v = GetPrivateProfileIntA("autotest", key, v, test_ini);
    return v;
}
static void cfg_str(const char *ini, const char *test_ini, int d, const char *key, const char *def, char *out, int cap) {
    char tmp[64];
    GetPrivateProfileStringA("all", key, def, tmp, sizeof tmp, ini);
    GetPrivateProfileStringA(k_diff_sections[d], key, tmp, out, cap, ini);
    if (test_ini) { lstrcpynA(tmp, out, sizeof tmp); GetPrivateProfileStringA("autotest", key, tmp, out, cap, test_ini); }
}
static float cfg_float(const char *ini, const char *test_ini, int d, const char *key, const char *def) {
    char buf[32];
    cfg_str(ini, test_ini, d, key, def, buf, sizeof buf);
    return (float)atof(buf);
}

static void settings_load(const char *ini, const char *test_ini) {
    for (int d = 0; d < 3; d++) {
        struct ai_cfg *c = &g_cfg[d];
        c->copper_plan = cfg_int(ini, test_ini, d, "copper_plan", 0);
        c->budget = cfg_int(ini, test_ini, d, "budget", 0) || c->copper_plan;   // the plan works through the budget
        c->safe_sites = cfg_int(ini, test_ini, d, "safe_sites", 0);
        c->mil_start_share = cfg_float(ini, test_ini, d, "military_start_share", "0.30");
        c->mil_income_share = cfg_float(ini, test_ini, d, "military_income_share", "0.40");
        c->mil_income_share_farms = cfg_float(ini, test_ini, d, "military_income_share_farms", "0.65");
        c->cost_percent = cfg_int(ini, test_ini, d, "cost_percent", 0);
        char pacing[16];
        cfg_str(ini, test_ini, d, "epoch_pacing", "timer", pacing, sizeof pacing);
        c->pacing_fast = !lstrcmpiA(pacing, "fast");
        c->epoch_min = cfg_int(ini, test_ini, d, "epoch_min", 0);
        c->epoch_max = cfg_int(ini, test_ini, d, "epoch_max", c->epoch_min);
        if (c->epoch_max < c->epoch_min) c->epoch_max = c->epoch_min;
        c->wait_for_player = cfg_int(ini, test_ini, d, "wait_for_player", -1);
        c->first_attack_min = cfg_int(ini, test_ini, d, "first_attack_min", 0);
        c->first_attack_max = cfg_int(ini, test_ini, d, "first_attack_max", c->first_attack_min);
        if (c->first_attack_max < c->first_attack_min) c->first_attack_max = c->first_attack_min;
        c->attack_min = cfg_int(ini, test_ini, d, "attack_interval_min", 0);
        c->attack_max = cfg_int(ini, test_ini, d, "attack_interval_max", c->attack_min);
        if (c->attack_max < c->attack_min) c->attack_max = c->attack_min;
        c->attack_group_percent = cfg_int(ini, test_ini, d, "attack_group_percent", 0);
        c->attack_on_contact = cfg_int(ini, test_ini, d, "attack_on_contact", -1);
        c->attack_for_resources = cfg_int(ini, test_ini, d, "attack_for_resources", -1);
        c->attack_revenge = cfg_int(ini, test_ini, d, "attack_revenge", -1);
        c->citizen_share = cfg_float(ini, test_ini, d, "citizen_share", "0");
        c->home_guard_share = cfg_float(ini, test_ini, d, "home_guard_share", "-1");
        if (g_log)
            fprintf(g_log, "# settings %-6s copper_plan=%d budget=%d (military %.2f/%.2f/%.2f) safe_sites=%d cost=%d%% "
                    "pacing=%s epoch=%d-%d wait_for_player=%d attacks first %d-%d every %d-%d group %d%% "
                    "contact %d resources %d revenge %d citizens %.2f\n", k_diff_sections[d], c->copper_plan, c->budget,
                    c->mil_start_share, c->mil_income_share, c->mil_income_share_farms, c->safe_sites, c->cost_percent,
                    c->pacing_fast ? "fast" : "timer", c->epoch_min, c->epoch_max, c->wait_for_player,
                    c->first_attack_min, c->first_attack_max, c->attack_min, c->attack_max, c->attack_group_percent,
                    c->attack_on_contact, c->attack_for_resources, c->attack_revenge, c->citizen_share);
    }
}

// Once per AI player per game (first AI update).
static void settings_apply(void *pl) {
    const struct ai_cfg *c = cfg_of(pl);
    int d = FIELD(pl, PL_DIFFICULTY, int);
    if (c->cost_percent > 0) {
        FIELD(pl, PL_CHEAT, uint8_t) = c->cost_percent != 100;
        if (d == 0) FIELD(pl, 0xD48, int) = c->cost_percent;
        else if (d == 2) FIELD(pl, 0xD4C, int) = c->cost_percent;
        else FIELD(pl, 0xBB4, float) = (float)c->cost_percent;
    }
    if (c->wait_for_player >= 0) FIELD(pl, 0xF02, uint8_t) = !c->wait_for_player;
    if (c->epoch_min > 0) {
        FIELD(pl, 0xD58, int) = c->epoch_min;
        FIELD(pl, 0xD5C, int) = c->epoch_max;
        // the first attempt was drawn before this runs: redraw it if it lies outside the new range
        uint8_t *cm = FIELD(pl, 0xB90, uint8_t *);
        uint32_t now = game_time();
        if (cm) {
            uint32_t t = FIELD(cm, 0x58, uint32_t);
            uint32_t lo = now + (uint32_t)c->epoch_min * 60000u, hi = now + (uint32_t)c->epoch_max * 60000u;
            if (t < lo || t > hi) {
                int span = c->epoch_max - c->epoch_min + 1, idx = FIELD(pl, PL_INDEX, int);
                FIELD(cm, 0x58, uint32_t) = lo + (uint32_t)((idx * 7) % span) * 60000u;
            }
        }
    }
    if (c->first_attack_min > 0) {
        FIELD(pl, 0xD2C, int) = c->first_attack_min;
        FIELD(pl, 0xD28, int) = c->first_attack_max;
        if (!FIELD(pl, 0xEF5, uint8_t)) {   // first attack still pending: redraw it with the new range
            int span = c->first_attack_max - c->first_attack_min + 1, idx = FIELD(pl, PL_INDEX, int);
            FIELD(pl, 0xED8, uint32_t) = game_time() + (uint32_t)(c->first_attack_min + (idx * 5) % span) * 60000u;
        }
    }
    if (c->attack_min > 0) { FIELD(pl, 0xD00, int) = c->attack_min; FIELD(pl, 0xCFC, int) = c->attack_max; }
    if (c->attack_group_percent > 0) FIELD(pl, d == 0 ? 0xD30 : d == 2 ? 0xD38 : 0xD34, int) = c->attack_group_percent;
    if (c->attack_on_contact >= 0) FIELD(pl, 0xCD8, int) = c->attack_on_contact;
    if (c->attack_for_resources >= 0) FIELD(pl, 0xCDC, int) = c->attack_for_resources;
    if (c->attack_revenge >= 0) FIELD(pl, 0xCE0, int) = c->attack_revenge;
    if (c->citizen_share > 0.0f && c->citizen_share < 1.0f) FIELD(pl, 0xBDC, float) = c->citizen_share;
    if (g_log) fprintf(g_log, "# settings applied: P%d difficulty %d\n", FIELD(pl, PL_INDEX, int), d);
}
