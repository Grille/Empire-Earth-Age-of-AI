// User settings per AI difficulty (aimod.ini next to the DLL), applied to the game's player fields. The values come
// from the rows (dll/settings/: g_set_ai[d], each key from the difficulty's own section - [ai_easy], [ai_hard]; medium none -,
// then [ai_base], then the built-in default, the test ini's [autotest] over all of them); the built-in defaults are the
// shipped config/aimod.ini's values - the AI at its strongest without cheating, [ai_easy] eased off (SETTINGS_EASY).
// Included by eeaimod.c before sites.c/econ.c.
//
// Difficulty = player+0xBA4 (0 easy, 1 medium, 2 hard).
// Cost: AdjustCost (vtable 0x8264C0 slot 27, 0x430FBD) multiplies prices by pct/100 while the player's
//   Cheat toggle (+0xEFB) is on; pct is +0xD48 (easy, int), +0xBB4 (medium, float), +0xD4C (hard, int).
// Epoch pacing (epoch_pacing): classic = the toggle "Ignorieren" (+0xF02) = 0 - AttemptEpochAdvance (0x46C966)
//   postpones by a minute while the AI's epoch is above the highest human epoch (0x46D436: one epoch ahead at most),
//   and the epoch timer is x2 while ahead / /2 while behind (Art of Conquest: x0.25 more when 3+ behind); the toggle
//   also gives the game's population cap that follows the humans (pop_follow_human) and lets the AI delete own units
//   for room (0x430D40). Every other mode: the toggle set (1). Whatever the toggle, a human reaching an epoch redraws
//   the attempt of every computer player behind it to a few minutes later (0x431533, the catch-up). timer: the timer
//   alone, without the catch-up (econ.c hook_epoch_catch_up); rush and follow are econ.c's (the attempt moved up,
//   follow's hold, both with the catch-up). independent: meant to weigh when the next epoch pays off - for now the
//   timer with the catch-up (the default: what timer was before it lost the catch-up).
// Epoch timer: SetNextEpochAdvanceTime (0x46DE8C) draws player+0xD5C (min)..+0xD58 (max) minutes (x difficulty factor).
// Attacks (copied from the AI settings table by 0x431697): first attack min/max minutes +0xD2C/+0xD28,
//   minutes between attacks min/max +0xD00/+0xCFC (next attack time +0xED8, scheduled by 0x43DD2F; +0xF05 =
//   first attack done: set when it draws from the interval), attack group size percent easy/medium/hard +0xD30/+0xD34/+0xD38 (0x43103F),
//   immediate-attack chances in percent: on contact with an enemy +0xCD8, to take resources +0xCDC,
//   revenge +0xCE0.

struct ai_cfg {
    int copper_plan, budget;
    float mil_start_share, mil_income_share, mil_income_share_farms;   // income share before / from the Copper Age (farms)
    int cost_percent;       // 0 = leave the game's Cheat setting alone
    int pacing;             // PACE_ (epoch_pacing)
    int epoch_min, epoch_max;
    int first_attack_min, first_attack_max, attack_min, attack_max;   // minutes, 0 = leave
    int attack_group_percent;                                         // 0 = leave
    int attack_on_contact, attack_for_resources, attack_revenge;      // percent, -1 = leave
    float citizen_share;    // share of the population cap for citizens (+0xBDC, the rest is military), 0 = leave
    float home_guard_share; // of the military room (defence.c)
    int pop_follow;         // pop_follow_human: 1 = the game's cap that follows the humans, 0 = the game limit (econ.c)
};
enum { PACE_CLASSIC, PACE_TIMER, PACE_RUSH, PACE_FOLLOW, PACE_INDEPENDENT, PACE_N };
internal const char *const k_pacing[PACE_N] = { "classic", "timer", "rush", "follow", "independent" };
internal struct ai_cfg g_cfg[3];
internal const char *const k_diff_sections[3] = { "easy", "medium", "hard" };

// Per player: the difficulty's settings with a personality's style on top (persona.c, only with ai_style=1).
internal struct ai_cfg g_cfg_pl[16];
internal uint8_t g_cfg_pl_on[16];

internal const struct ai_cfg *cfg_of(ee_player *pl) {
    if (pl) {
        unsigned i = (unsigned)player_get_index(pl) & 15;
        if (g_cfg_pl_on[i]) return &g_cfg_pl[i];
    }
    int d = pl ? player_get_difficulty(pl) : 1;
    return &g_cfg[d < 0 ? 0 : d > 2 ? 2 : d];
}

// The settings a personality's style may change - the same keys as in aimod.ini. Not the structural ones
// (copper_plan, budget, epoch_pacing) nor the ones that would cheat or bind to the human
// (cost_percent).
// In a personality the values are expressions (persona.c, worked out at the game start), numbers in the language's
// standard: a share or a chance is 0% to 100% (home_guard_share = 35%, attack_on_contact = 20%), a group's size a share
// of the normal one (attack_group_percent = 150%) - aimod.ini's whole percents (20, 150) are those times 100; minutes
// and epochs are plain numbers. A value outside its range is a mistake (attack_on_contact = 20 would be 2000%).
enum { SK_NUM, SK_SHARE, SK_CHANCE, SK_SIZE };
internal const struct { const char *key; size_t off; char kind; } k_style_keys[] = {
    { "military_start_share", offsetof(struct ai_cfg, mil_start_share), SK_SHARE },
    { "military_income_share", offsetof(struct ai_cfg, mil_income_share), SK_SHARE },
    { "military_income_share_farms", offsetof(struct ai_cfg, mil_income_share_farms), SK_SHARE },
    { "epoch_min", offsetof(struct ai_cfg, epoch_min), SK_NUM },
    { "epoch_max", offsetof(struct ai_cfg, epoch_max), SK_NUM },
    { "first_attack_min", offsetof(struct ai_cfg, first_attack_min), SK_NUM },
    { "first_attack_max", offsetof(struct ai_cfg, first_attack_max), SK_NUM },
    { "attack_interval_min", offsetof(struct ai_cfg, attack_min), SK_NUM },
    { "attack_interval_max", offsetof(struct ai_cfg, attack_max), SK_NUM },
    { "attack_group_percent", offsetof(struct ai_cfg, attack_group_percent), SK_SIZE },
    { "attack_on_contact", offsetof(struct ai_cfg, attack_on_contact), SK_CHANCE },
    { "attack_for_resources", offsetof(struct ai_cfg, attack_for_resources), SK_CHANCE },
    { "attack_revenge", offsetof(struct ai_cfg, attack_revenge), SK_CHANCE },
    { "citizen_share", offsetof(struct ai_cfg, citizen_share), SK_SHARE },
    { "home_guard_share", offsetof(struct ai_cfg, home_guard_share), SK_SHARE },
};
internal int style_key(const char *key) {   // its index in k_style_keys, -1: no style key
    for (size_t k = 0; k < sizeof k_style_keys / sizeof *k_style_keys; k++) if (!lstrcmpiA(key, k_style_keys[k].key)) return (int)k;
    return -1;
}
// style key k = v onto c; 0: v out of its range (*why)
internal int cfg_style_set(struct ai_cfg *c, int k, double v, const char **why) {
    void *at = (uint8_t *)c + k_style_keys[k].off;
    switch (k_style_keys[k].kind) {
    case SK_SHARE:
        if (!(v >= 0 && v <= 1)) { *why = "a share 0% to 100% (home_guard_share = 35%)"; return 0; }
        *(float *)at = (float)v;
        return 1;
    case SK_CHANCE:
        if (!(v >= 0 && v <= 1)) { *why = "a chance 0% to 100% (attack_on_contact = 20%)"; return 0; }
        *(int *)at = (int)(v * 100 + 0.5);
        return 1;
    case SK_SIZE:
        if (!(v >= 0.1 && v <= 10)) { *why = "a size 10% to 1000% of the normal group (attack_group_percent = 150%)"; return 0; }
        *(int *)at = (int)(v * 100 + 0.5);
        return 1;
    default:
        if (!(v >= 0 && v <= 1000) || v != (int)v) { *why = "a whole number 0 to 1000 (first_attack_min = 20)"; return 0; }
        *(int *)at = (int)v;
        return 1;
    }
}
internal void cfg_style_fix(struct ai_cfg *c) {   // a max below its min follows the min
    if (c->epoch_max < c->epoch_min) c->epoch_max = c->epoch_min;
    if (c->first_attack_max < c->first_attack_min) c->first_attack_max = c->first_attack_min;
    if (c->attack_max < c->attack_min) c->attack_max = c->attack_min;
}

// g_cfg from the rows (dll/settings/: g_set_ai, each difficulty's own), logged
internal void settings_load(void) {
    for (int d = 0; d < 3; d++) {
        struct ai_cfg *c = &g_cfg[d];
        const struct settings_ai *v = &g_set_ai[d];
        c->copper_plan = v->copper_plan;
        c->budget = v->budget || c->copper_plan;   // the plan works through the budget
        c->mil_start_share = v->military_start_share;
        c->mil_income_share = v->military_income_share;
        c->mil_income_share_farms = v->military_income_share_farms;
        c->cost_percent = v->cost_percent;
        c->pacing = v->epoch_pacing;   // the names in PACE_ order (rows.h)
        c->epoch_min = v->epoch_min;
        c->epoch_max = v->epoch_max;
        if (c->epoch_max < c->epoch_min) c->epoch_max = c->epoch_min;
        c->first_attack_min = v->first_attack_min;
        c->first_attack_max = v->first_attack_max;
        if (c->first_attack_max < c->first_attack_min) c->first_attack_max = c->first_attack_min;
        c->attack_min = v->attack_interval_min;
        c->attack_max = v->attack_interval_max;
        if (c->attack_max < c->attack_min) c->attack_max = c->attack_min;
        c->attack_group_percent = v->attack_group_percent;
        c->attack_on_contact = v->attack_on_contact;
        c->attack_for_resources = v->attack_for_resources;
        c->attack_revenge = v->attack_revenge;
        c->citizen_share = v->citizen_share;
        c->home_guard_share = v->home_guard_share;
        c->pop_follow = v->pop_follow_human;
        if (g_log)
            fprintf(g_log, "# settings %-6s copper_plan=%d budget=%d (military %.2f/%.2f/%.2f) cost=%d%% "
                    "pacing=%s epoch=%d-%d attacks first %d-%d every %d-%d group %d%% "
                    "contact %d resources %d revenge %d citizens %.2f\n", k_diff_sections[d], c->copper_plan, c->budget,
                    c->mil_start_share, c->mil_income_share, c->mil_income_share_farms, c->cost_percent,
                    k_pacing[c->pacing], c->epoch_min, c->epoch_max,
                    c->first_attack_min, c->first_attack_max, c->attack_min, c->attack_max, c->attack_group_percent,
                    c->attack_on_contact, c->attack_for_resources, c->attack_revenge, c->citizen_share);
    }
}

// Once per AI player per game (first AI update).
internal void settings_apply(ee_player *pl) {
    const struct ai_cfg *c = cfg_of(pl);
    int d = player_get_difficulty(pl);
    if (c->cost_percent > 0) {
        player_set_cheat(pl, c->cost_percent != 100);
        if (d == 0) player_set_cost_easy(pl, c->cost_percent);
        else if (d == 2) player_set_cost_hard(pl, c->cost_percent);
        else player_set_cost_medium(pl, (float)c->cost_percent);
    }
    player_set_ignore(pl, c->pacing != PACE_CLASSIC);
    if (c->epoch_min > 0) {
        player_set_epoch_min(pl, c->epoch_min);
        player_set_epoch_max(pl, c->epoch_max);
        // the first attempt was drawn before this runs: redraw it if it lies outside the new range
        ee_civ_mgr *cm = player_get_civ_mgr(pl);
        uint32_t now = game_time();
        if (cm) {
            uint32_t t = civ_mgr_get_timer(cm, 0);
            uint32_t lo = now + (uint32_t)c->epoch_min * 60000u, hi = now + (uint32_t)c->epoch_max * 60000u;
            if (t < lo || t > hi) {
                int span = c->epoch_max - c->epoch_min + 1, idx = player_get_index(pl);
                civ_mgr_set_timer(cm, 0, lo + (uint32_t)((idx * 7) % span) * 60000u);
            }
        }
    }
    if (c->first_attack_min > 0) {
        player_set_first_attack_min(pl, c->first_attack_min);
        player_set_first_attack_max(pl, c->first_attack_max);
        if (!player_get_first_attack_done(pl)) {   // first attack still pending: redraw it with the new range
            int span = c->first_attack_max - c->first_attack_min + 1, idx = player_get_index(pl);
            player_set_next_attack(pl, game_time() + (uint32_t)(c->first_attack_min + (idx * 5) % span) * 60000u);
        }
    }
    if (c->attack_min > 0) { player_set_attack_min(pl, c->attack_min); player_set_attack_max(pl, c->attack_max); }
    if (c->attack_group_percent > 0) {   // by the spec: AoC's are 0x38 later (the 2017 offsets were its attack_floor ...)
        if (d == 0) player_set_group_pct_easy(pl, c->attack_group_percent);
        else if (d == 2) player_set_group_pct_hard(pl, c->attack_group_percent);
        else player_set_group_pct_medium(pl, c->attack_group_percent);
    }
    if (c->attack_on_contact >= 0) player_set_attack_on_contact(pl, c->attack_on_contact);
    if (c->attack_for_resources >= 0) player_set_attack_for_resources(pl, c->attack_for_resources);
    if (c->attack_revenge >= 0) player_set_attack_revenge(pl, c->attack_revenge);
    if (c->citizen_share > 0.0f && c->citizen_share < 1.0f) player_set_citizen_share(pl, c->citizen_share);
    if (g_log) fprintf(g_log, "# settings applied: P%d difficulty %d\n", player_get_index(pl), d);
}
