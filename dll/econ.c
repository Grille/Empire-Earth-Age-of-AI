// Economy fixes. Included by eeaimod.c (after control.c, so g_players/um_player-style helpers exist).
//
// BUDGET: the unit manager trains units through one function, 0x49014F(request{type, count, ...}), called
// from 5 places. It spends whatever is in the stockpile, so in the Prehistoric the AI turned ~70% of its
// food into Rock Throwers/Clubmen (army target = share of the pop cap, 60 from minute 0) and could never
// afford the next epoch (Stone Age: 850 food). We hide a reserve from the stockpile for the duration of
// the call - the AI only sees and spends the surplus - and put it back afterwards.
// Reserve = cost of the next epoch's tech (tech tree +8 + epoch*0x54, +8 -> data, costs at +0x24..+0x34).
//
// On top of the reserve, spending is split by purpose:
// - Military units may only spend from a military account: a share of the stock at the first purchase,
//   plus a share of every income (EEComputerPlayer::AddResource, vtable 0x8264C0 slot 18). This holds in
//   every epoch - farms don't make burning food on soldiers free.
// - Citizens are only trained while the existing ones have work: no new citizens while the idle ones plus
//   those beyond their resource's target (e.g. 36 on wood with a target of 3) exceed a small slack.

#define FN_TRAIN_UNITS 0x0049014F
static const uintptr_t k_train_sites[] = { 0x46cce6, 0x490cac, 0x490e12, 0x490f1f, 0x490fa9 };
// per-difficulty switches and military shares: settings.c (cfg_of)
static int g_citizen_slack = 9999;   // idle-citizen check: off by default (stalled growth in tests)
static int g_hoard_cap = 3000;   // stock above reserve + this is free for military too, aimod.ini [budget] hoard_cap
static struct { uint32_t last_t; int init; float mil[7]; int income[7], mil_spent[7], cit_blocked, mil_limited; } g_bud[16];

#define VT_COMPUTER_PLAYER 0x008264C0
#define FN_CP_ADD_RESOURCE 0x0042F783   // EEComputerPlayer::vf18(type, amount)
static void __fastcall hook_add_resource(void *pl, void *edx, int type, int amount) {
    (void)edx;
    ((void (__thiscall *)(void *, int, int))FN_CP_ADD_RESOURCE)(pl, type, amount);
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (amount > 0 && type >= 2 && type < 7 && g_bud[idx].init) {
        g_bud[idx].income[type] += amount;
        g_bud[idx].mil[type] += amount * cfg_of(pl)->mil_income_share;
    }
}

static int is_citizen_type(int type) { return type == 32 || type == 480; }   // Citizen, Female Citizen

// Citizens without useful work: unassigned ones plus those beyond their resource's target.
static int idle_citizens(void *pl) {
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *rm = cm ? FIELD(cm, 0x38, uint8_t *) : NULL;
    if (!rm) return 0;
    int idle = FIELD(rm, RM_LISTS + 0x10, int);   // list 0: not assigned to a resource
    for (int t = 2; t < 7; t++) {
        int n = FIELD(rm, RM_LISTS + t * 0x14 + 0x10, int), want = ((int *)(rm + RM_TARGET))[t];
        if (n > want) idle += n - want;
    }
    return idle;
}

static void *um_owner(void *um) {
    for (int i = 0; i < 16; i++) {
        void *pl = g_players[i];
        void *cm = pl ? FIELD(pl, 0xB90, void *) : NULL;
        if (cm && FIELD(cm, 0x44, void *) == um) return pl;
    }
    return NULL;
}

// Resources the AI must not spend on units: the next epoch's cost (types 2..6 = food..iron), faded in over
// the last g_reserve_window_ms before the AI's scheduled epoch attempt (civ manager +0x58) and held in
// full once the attempt is due. Before that window nothing is held back - early growth matters most.
// COPPER PLAN (aimod.ini copper_plan, per difficulty - see settings.c): before farms (Farm/Granary come with epoch 2, Copper
// Age) food is a finite stock of berries and game, so the AI aims for the Copper Age: it reserves the full
// remaining cost of getting there (Stone 850 food; Copper 750 food, 400 gold, 400 iron) against military
// spending, and advances as soon as it can pay for the next epoch instead of waiting for its 50-70 minute
// timer. From the Copper Age on the normal timer and the faded-in reserve apply.
#define FARM_EPOCH 2
static int g_min_workforce = 20;   // economy first: below this, citizens ignore the reserve and no early advance

static int workforce(void *pl) {
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *rm = cm ? FIELD(cm, 0x38, uint8_t *) : NULL;
    int n = 0;
    for (int t = 0; rm && t < 7; t++) n += FIELD(rm, RM_LISTS + t * 0x14 + 0x10, int);
    return n;
}

static uint8_t *epoch_tech(void *pl, int e) {
    uint8_t *tt = FIELD(pl, PL_TECHTREE, uint8_t *);
    if (!tt || e > 13 || e > *(int *)0x009187F4) return NULL;
    return FIELD(tt + 8 + e * 0x54, 8, uint8_t *);
}
static int epoch_cost(void *pl, int e, int r) {
    uint8_t *d = epoch_tech(pl, e);
    int c = d ? FIELD(d, 0x24 + (r - 2) * 4, int) : 0;
    return c > 0 ? c : 0;
}
static int current_epoch(void *pl) {
    uint8_t *tt = FIELD(pl, PL_TECHTREE, uint8_t *);
    return tt ? FIELD(tt, TT_EPOCH, int) : 0;
}

// Per AI tick (from hook_rm_update): pull the epoch attempt forward in the pre-farm epochs once affordable.
static void econ_tick(void *pl, uint32_t now) {
    const struct ai_cfg *c = cfg_of(pl);
    // attack log: the next-attack time (+0xED8) is redrawn whenever an attack starts
    static uint32_t last_attack_time[16];
    unsigned ai = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    uint32_t t_attack = FIELD(pl, 0xED8, uint32_t);
    if (t_attack != last_attack_time[ai]) {
        if (last_attack_time[ai] && g_log)
            fprintf(g_log, "# attack: P%u at %.1f min, next at %.1f min\n", ai, now / 60000.0, t_attack / 60000.0);
        last_attack_time[ai] = t_attack;
    }
    int ep = current_epoch(pl);
    // pull the attempt forward: before farms with the copper plan, in every epoch with epoch_pacing=fast
    if (!(c->copper_plan && ep < FARM_EPOCH) && !c->pacing_fast) return;
    uint8_t *cm = FIELD(pl, 0xB90, uint8_t *);
    // AttemptEpochAdvance (0x46C966) asks the research manager once for the techs the next epoch requires
    // (record +0x40 needed, +0x48 done) and then waits while cm+0x76 is set; re-arm it after 5 minutes.
    static uint32_t pending_since[16];
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (cm && cm[0x76]) {
        if (!pending_since[pi] || now < pending_since[pi]) pending_since[pi] = now;
        else if (now - pending_since[pi] > 5 * 60000) {
            cm[0x76] = 0;
            pending_since[pi] = 0;
            if (g_log) fprintf(g_log, "# copper plan: P%u epoch requirement still pending after 5 min, re-requesting\n", pi);
        }
    } else pending_since[pi] = 0;
    if (!cm || !epoch_tech(pl, ep + 1)) return;
    int *stock = (int *)((uint8_t *)pl + PL_STOCK);
    for (int r = 2; r < 7; r++)
        if (stock[r] < epoch_cost(pl, ep + 1, r)) return;
    uint32_t *t_epoch = (uint32_t *)(cm + 0x58);
    if (*t_epoch > now + 5000) {
        if (g_log) fprintf(g_log, "# epoch plan: P%d can pay epoch %d at %.1f min, attempt moved up from %.1f min\n",
                           FIELD(pl, PL_INDEX, int), ep + 1, now / 60000.0, *t_epoch / 60000.0);
        *t_epoch = now;
    }
}

// The pre-farm food budget is what the map offers: wild food (forage sites) the AI has explored within safe
// reach of its base (g_site_max_dist), like a player counting the berry patches around his town. Only part
// of it will actually be gathered (competition, losses), hence g_wild_share.
static float g_wild_share = 0.6f;   // aimod.ini [budget] wild_food_share
static int wild_food(void *pl) {
    uint8_t *cm = FIELD(pl, 0xB90, uint8_t *);
    void *bm = cm ? FIELD(cm, 0x3C, void *) : NULL;
    if (!bm) return 0;
    int bx = FIELD(bm, 0x12C, int), by = FIELD(bm, 0x130, int);
    if (bx == -10000) return 0;
    int lim = g_site_max_dist * g_site_max_dist, total = 0;
    void **zb = FIELD(pl, 0xB50, void **), **ze = FIELD(pl, 0xB54, void **);
    for (int z = 0; zb && zb + z < ze; z++) {
        uint8_t *s = zb[z] ? ((uint8_t *(__thiscall *)(void *, int))FN_ZONE_SITE)(zb[z], 2) : NULL;
        if (!s || FIELD(FIELD(s, 0x2C, uint8_t *), 0x4B8, int) != 0x37) continue;
        int dx = FIELD(s, 0x1C, int) - bx, dy = FIELD(s, 0x20, int) - by;
        if (dx * dx + dy * dy >= lim) continue;
        if (!((char (__thiscall *)(void *, void *))FN_EXPLORED)(FIELD(s, 0x14, void *), pl)) continue;
        total += FIELD(s, 0x40, int);
    }
    return total;
}

// Worker targets (after ComputeTargets), pre-farm epochs only: the original targets follow each resource's
// share of outstanding demand - which our budget inflates for food (blocked military requests stay on the
// books), so workers piled onto food while stone/gold starved and the epoch requirement techs never got
// paid. Redistribute the same number of workers by actual shortfall: food only up to the epoch reserve,
// others by demand beyond stock, stone/gold/iron at least what the Copper Age still costs.
static void budget_reserve(void *pl, int reserve[7]);
static void econ_targets(uint8_t *rm) {
    void *pl = rm_player(rm);
    if (!pl || !cfg_of(pl)->copper_plan || current_epoch(pl) >= FARM_EPOCH) return;
    int *stock = (int *)((uint8_t *)pl + PL_STOCK);
    int *demand = (int *)(rm + RM_DEMAND);
    uint32_t *target = (uint32_t *)(rm + RM_TARGET);
    int reserve[7];
    budget_reserve(pl, reserve);
    int total = 0, need[7] = { 0 }, sum = 0;
    for (int r = 2; r < 7; r++) total += (int)target[r];
    need[2] = reserve[2] + 300 - stock[2];
    for (int r = 3; r < 7; r++) {
        need[r] = demand[r] - stock[r];
        if (r >= 4 && reserve[r] - stock[r] > need[r]) need[r] = reserve[r] - stock[r];
    }
    for (int r = 2; r < 7; r++) { if (need[r] < 0) need[r] = 0; sum += need[r]; }
    if (sum <= 0 || total < 6) return;
    int floor_[7] = { 0, 0, 2, 2, 0, 0, 0 };   // keep a few on food (growth) and wood (building)
    int rest = total - 4, given = 0;
    for (int r = 2; r < 7; r++) {
        int t = floor_[r] + (int)((long long)rest * need[r] / sum);
        target[r] = (uint32_t)t;
        given += t;
    }
    for (int r = 2; given < total; r = r == 6 ? 2 : r + 1)   // rounding leftovers to the largest needs
        if (need[r] > 0) { target[r]++; given++; }
}

static int g_reserve_window_ms = 5 * 60000;   // aimod.ini [budget] reserve_window_min
static void budget_reserve(void *pl, int reserve[7]) {
    memset(reserve, 0, 7 * sizeof *reserve);
    uint8_t *tt = FIELD(pl, PL_TECHTREE, uint8_t *);
    uint8_t *cm = FIELD(pl, 0xB90, uint8_t *);
    if (!tt || !cm) return;
    int next = FIELD(tt, TT_EPOCH, int) + 1;
    if (next > 13 || next > *(int *)0x009187F4) return;   // end epoch of this game
    uint8_t *data = FIELD(tt + 8 + next * 0x54, 8, uint8_t *);
    if (!data) return;
    if (cfg_of(pl)->copper_plan && next <= FARM_EPOCH) {
        for (int r = 2; r < 7; r++)
            for (int e = next; e <= FARM_EPOCH; e++) reserve[r] += epoch_cost(pl, e, r);
        return;
    }
    int remaining = (int)(FIELD(cm, 0x58, uint32_t) - game_time());
    float f = remaining <= 0 ? 1.0f : remaining >= g_reserve_window_ms ? 0.0f
            : 1.0f - (float)remaining / (float)g_reserve_window_ms;
    for (int r = 2; r < 7; r++) {
        int c = FIELD(data, 0x24 + (r - 2) * 4, int);
        reserve[r] = c > 0 ? (int)(c * f) : 0;
    }
}

static void trace_train(void *pl, int *req, uintptr_t r, const int *before, const int *reserve);

static uintptr_t __fastcall hook_train_units(void *um, void *edx, int *req, uintptr_t b) {
    (void)edx;
    void *pl = um_owner(um);
    int *stock = pl ? (int *)((uint8_t *)pl + PL_STOCK) : NULL;
    int before[7] = { 0 }, reserve[7] = { 0 }, hidden[7] = { 0 };
    if (stock) memcpy(before, stock, sizeof before);
    unsigned idx = pl ? (unsigned)FIELD(pl, PL_INDEX, int) & 15 : 0;
    int military = !is_citizen_type(req[0]);
    const struct ai_cfg *cfg = pl ? cfg_of(pl) : NULL;
    if (stock && cfg->budget) {
        uint32_t now = game_time();
        if (now < g_bud[idx].last_t) g_bud[idx].init = 0;   // new game
        g_bud[idx].last_t = now;
        budget_reserve(pl, reserve);
        if (!g_bud[idx].init) {
            memset(&g_bud[idx], 0, sizeof g_bud[idx]);
            g_bud[idx].init = 1;
            g_bud[idx].last_t = now;
            for (int r = 2; r < 7; r++)
                g_bud[idx].mil[r] = stock[r] > reserve[r] ? (stock[r] - reserve[r]) * cfg->mil_start_share : 0;
        }
        int no_work = 0;
        if (!military) {
            void *cm = FIELD(pl, 0xB90, void *);
            uint8_t *rm = cm ? FIELD(cm, 0x38, uint8_t *) : NULL;
            int total = 0;
            for (int t = 0; rm && t < 7; t++) total += FIELD(rm, RM_LISTS + t * 0x14 + 0x10, int);
            int idle = idle_citizens(pl);
            // never starve a small population; the first assignment pass runs only every 10 s
            // only matters while food is scarce: idle citizens are cheap (and builders) when it is plentiful
            int scarce = stock[2] < 2 * reserve[2] + 500;
            no_work = scarce && total >= 10 && idle > g_citizen_slack && idle * 100 > total * 15;
        }
        for (int r = 2; r < 7; r++) {
            int visible = stock[r] - reserve[r];
            // citizens before farms: they pay only food, and only what the food budget (stock + reachable wild
            // food) doesn't need for the epochs; a tiny workforce always grows
            if (!military && cfg->copper_plan && current_epoch(pl) < FARM_EPOCH) {
                int wf = workforce(pl);
                if (r != 2 || wf < 10) visible = stock[r];
                else if (wf < g_min_workforce && stock[2] + (int)(wild_food(pl) * g_wild_share) >= reserve[2] + 500)
                    visible = stock[2];   // enough food on the map: grow the economy first
                // else: workforce built (or the map is poor) -> save: the full reserve applies
            }
            if (military) {
                int allowed = (int)g_bud[idx].mil[r];
                int excess = stock[r] - reserve[r] - g_hoard_cap;   // never hoard beyond the cap
                if (excess > allowed) allowed = excess;
                if (visible > allowed) visible = allowed;
            }
            if (no_work) visible = 0;
            if (visible < 0) visible = 0;
            if (visible > stock[r]) visible = stock[r];
            hidden[r] = stock[r] - visible;
            stock[r] = visible;
        }
        if (no_work) g_bud[idx].cit_blocked++;
    }
    uintptr_t res = ((uintptr_t (__thiscall *)(void *, int *, uintptr_t))FN_TRAIN_UNITS)(um, req, b);
    if (stock) {
        for (int r = 2; r < 7; r++) {
            stock[r] += hidden[r];
            int spent = before[r] - stock[r];
            if (cfg->budget && military && spent > 0) { g_bud[idx].mil[r] -= spent; g_bud[idx].mil_spent[r] += spent; }
        }
        trace_train(pl, req, res, before, reserve);
        static uint32_t last_log[16];
        uint32_t now = game_time();
        if (cfg->budget && g_log && (now < last_log[idx] || now - last_log[idx] >= 60000)) {
            last_log[idx] = now;
            EnterCriticalSection(&g_lock);
            fprintf(g_log, "# B %u P%u food=%d reserve=%d mil_account=%.0f income=%d mil_spent=%d idle=%d cit_blocked=%d\n",
                    now, idx, stock[2], reserve[2], g_bud[idx].mil[2], g_bud[idx].income[2], g_bud[idx].mil_spent[2],
                    idle_citizens(pl), g_bud[idx].cit_blocked);
            fflush(g_log);
            LeaveCriticalSection(&g_lock);
        }
    }
    return res;
}

static void econ_install(const char *aimod_ini, const char *autotest_ini) {
    settings_load(aimod_ini, autotest_ini);
    sites_install(aimod_ini, autotest_ini);
    // finer tuning, same for all difficulties ([all] section; autotest may override)
    g_citizen_slack = cfg_int(aimod_ini, autotest_ini, 1, "idle_citizen_slack", 9999);
    g_hoard_cap = cfg_int(aimod_ini, autotest_ini, 1, "hoard_cap", 3000);
    g_min_workforce = cfg_int(aimod_ini, autotest_ini, 1, "min_workforce", 20);
    g_wild_share = cfg_float(aimod_ini, autotest_ini, 1, "wild_food_share", "0.6");
    g_reserve_window_ms = cfg_int(aimod_ini, autotest_ini, 1, "reserve_window_min", 5) * 60000;
    int ok = 0;
    void **slot = (void **)(VT_COMPUTER_PLAYER + 18 * 4);
    if (*slot == (void *)FN_CP_ADD_RESOURCE) {
        DWORD old;
        VirtualProtect(slot, 4, PAGE_READWRITE, &old);
        *slot = (void *)hook_add_resource;
        VirtualProtect(slot, 4, old, &old);
        ok++;
    }
    for (size_t i = 0; i < sizeof k_train_sites / sizeof *k_train_sites; i++)
        ok += patch_call(k_train_sites[i], FN_TRAIN_UNITS, hook_train_units);
    if (g_log) fprintf(g_log, "# economy hooks: %d/%d\n", ok, (int)(sizeof k_train_sites / sizeof *k_train_sites) + 1);
}
