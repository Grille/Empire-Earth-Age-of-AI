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
// stone, gold, iron: hardly any civilian use besides the epoch cost (reserved anyway) and a few techs - with the
// 3000 cap gold piled up (1000 in stock) while every soldier failed for gold; aimod.ini hoard_cap_mineral
static int g_hoard_cap_mineral = 600;
static struct { uint32_t last_t; int init; float mil[7]; int income[7], mil_spent[7], cit_blocked, mil_limited; } g_bud[16];

// Spending by caller (diagnostic): what EEComputerPlayer::vf45 (pay) took, summed per player, resource and return
// address, with the income so far; logged as "# spend:" every 5 minutes.
static struct { uintptr_t site; int type; unsigned sum, n; } g_spend[16][24];
static void spend_note(unsigned idx, int type, int amount, uintptr_t site) {
    int k = 0;
    while (k < 24 && g_spend[idx][k].n && (g_spend[idx][k].site != site || g_spend[idx][k].type != type)) k++;
    if (k == 24) return;
    g_spend[idx][k].site = site; g_spend[idx][k].type = type;
    g_spend[idx][k].sum += (unsigned)amount; g_spend[idx][k].n++;
}
static void spend_log(unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_log || now / 300000 == last[idx] / 300000) return;
    last[idx] = now;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# spend: P%u at %u min:", idx, now / 60000);
    for (int k = 0; k < 24 && g_spend[idx][k].n; k++)
        fprintf(g_log, " %06x/%c=%u(%u)", (unsigned)g_spend[idx][k].site, "..FWSGI"[g_spend[idx][k].type],
                g_spend[idx][k].sum, g_spend[idx][k].n);
    fprintf(g_log, "; income F%d W%d S%d G%d I%d\n", g_bud[idx].income[2], g_bud[idx].income[3], g_bud[idx].income[4],
            g_bud[idx].income[5], g_bud[idx].income[6]);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}

// BUILD QUEUE FIX: the build manager (0x468540, every 3 s) takes the first queue (one per epoch) whose front entry it
// can afford, places it with 0x46580A(type, x, y) - one call site, 0x468629 - and stops for this round; an entry that
// can't be placed goes back into its queue. The random-map build list starts with a "Single Player Settlement"
// (type 395) that can never be placed, alone in the epoch-0 queue: it failed ~1000 times a game and took every round,
// so nothing from the later queues was ever built - no Granary, Archery Range, Stable, Siege Factory, University...
// build_fix (aimod.ini [all], default on): 395 is placed as a normal Settlement (43, a drop site), and an entry that
// fails 3 times in a row is dropped (reported as placed; the planner 0x465A8E re-queues missing buildings every
// 2 minutes). Attempts and failures per player and type are logged as "# place:" every 5 minutes.
#define CALL_PLACE 0x00468629
#define FN_PLACE   0x0046580A
static int g_build_fix = 1;
static struct { int type; unsigned tries, fails, dropped, streak; } g_place[16][64];   // every type the build lists hold
static char __fastcall hook_place(uint8_t *bm, void *edx, int type, int x, int y) {
    (void)edx;
    int place_as = g_build_fix && type == 395 ? 43 : type;
    char ok = ((char (__thiscall *)(void *, int, int, int))FN_PLACE)(bm, place_as, x, y);
    void *pl = FIELD(bm, 0x138, void *);
    unsigned idx = pl ? (unsigned)FIELD(pl, PL_INDEX, int) & 15 : 0;
    int k = 0;
    while (k < 64 && g_place[idx][k].tries && g_place[idx][k].type != type) k++;
    if (k == 64) return ok;
    g_place[idx][k].type = type;
    g_place[idx][k].tries++;
    if (ok) { g_place[idx][k].streak = 0; return ok; }
    g_place[idx][k].fails++;
    if (g_build_fix && ++g_place[idx][k].streak >= 3) {
        g_place[idx][k].streak = 0;
        g_place[idx][k].dropped++;
        return 1;
    }
    return ok;
}
static void place_log(unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_log || now / 300000 == last[idx] / 300000) return;
    last[idx] = now;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# place: P%u at %u min (type tries/failed/dropped):", idx, now / 60000);
    for (int k = 0; k < 64 && g_place[idx][k].tries; k++)
        fprintf(g_log, " %d %u/%u/%u", g_place[idx][k].type, g_place[idx][k].tries, g_place[idx][k].fails, g_place[idx][k].dropped);
    fputc('\n', g_log);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}

// drops P: the player's drop sites (x,y; g = granary, food only)
static void cmd_drops(char *out, const char *args) {
    int p = atoi(args);
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    if (!pl) { REPLY("error: no AI player %d\n", p); return; }
    static int d[64][3];
    int nd = drop_sites(pl, d, 64);
    REPLY("P%d drop sites:", p);
    for (int i = 0; i < nd; i++) REPLY(" %d,%d%s", d[i][0], d[i][1], d[i][2] ? "g" : "");
    REPLY("\n");
}

// Gathering (diagnostic), every 5 minutes: per resource the workers, the mean distance from their gather site to the
// nearest drop site, and the income per worker-minute since the last line.
static void gather_log(void *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    static int last_income[16][7];
    if (!g_log || now / 300000 == last[idx] / 300000) return;
    last[idx] = now;
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *rm = cm ? FIELD(cm, 0x38, uint8_t *) : NULL;
    if (!rm) return;
    static int d[64][3];
    int nd = drop_sites(pl, d, 64), ndrop = 0;
    for (int i = 0; i < nd; i++) ndrop += !d[i][2];
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# gather: P%u at %u min, drop sites %d (+%d granaries):", idx, now / 60000, ndrop, nd - ndrop);
    for (int r = 2; r < 7; r++) {
        uint8_t *dq = rm + RM_LISTS + r * 0x14;
        unsigned first = FIELD(dq, 0xC, unsigned), n = FIELD(dq, 0x10, unsigned), m = 0;
        int sum = 0;
        for (unsigned k = 0; k < n; k++) {
            uint8_t *u = deque_at(dq, first + k), *goal = u ? FIELD(u, 0x1EC, uint8_t *) : NULL;
            uint8_t *site = goal && !strcmp(rtti_name(goal), "EEUGGather@@") ? FIELD(goal, 0x38, uint8_t *) : NULL;
            if (!site) continue;
            sum += drop_dist(d, nd, FIELD(site, 0x1C, int), FIELD(site, 0x20, int), r == 2);
            m++;
        }
        int inc = g_bud[idx].income[r] - last_income[idx][r];
        last_income[idx][r] = g_bud[idx].income[r];
        fprintf(g_log, " %c %u/%u dist %.0f rate %.1f;", "..FWSGI"[r], m, n, m ? (float)sum / m : 0.0f, n ? inc / (5.0f * n) : 0.0f);
    }
    fputc('\n', g_log);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}

#define VT_COMPUTER_PLAYER 0x008264C0
#define FN_CP_PAY 0x0043F53D   // EEComputerPlayer::vf45(cost object, ?, ?): pays a unit/building/tech
static void __fastcall hook_pay(void *pl, void *edx, void *what, int b, int c) {
    (void)edx;
    int *stock = (int *)((uint8_t *)pl + PL_STOCK), before[7];
    memcpy(before, stock, sizeof before);
    ((void (__thiscall *)(void *, void *, int, int))FN_CP_PAY)(pl, what, b, c);
    uintptr_t site = (uintptr_t)__builtin_return_address(0);
    for (int r = 2; r < 7; r++)
        if (before[r] > stock[r]) spend_note((unsigned)FIELD(pl, PL_INDEX, int) & 15, r, before[r] - stock[r], site);
}
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
    spend_log(ai, now);
    gather_log(pl, ai, now);
    place_log(ai, now);
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

// WORKER REBALANCE (after every AssignCitizens pass, 10 s): AssignCitizens only hands out idle citizens and never
// takes a gatherer back, so workers stay wherever an old target sent them - 21 on iron with a target of 9 and
// 1300 iron unused, while gold (target 17) had 5 and every soldier failed for gold. Move gathering citizens from
// the resource with the largest surplus to a stone/gold/iron mine that is short, with the game's own site search
// (0x48C204: free slot, explored, no enemy near, the citizen's continent) and assignment (0x48C81C: new gather
// target, moves the citizen between the resource lists). Hysteresis: the targets jump with demand every 5 s.
#define FN_ASSIGN_TO_SITE 0x0048C81C
static int g_rebalance = 1;   // aimod.ini [all] rebalance_workers
static unsigned g_moved[16][7];
static int rm_count(uint8_t *rm, int t) { return FIELD(rm, RM_LISTS + t * 0x14 + 0x10, int); }
static uint8_t *movable_gatherer(uint8_t *rm, int t) {
    uint8_t *dq = rm + RM_LISTS + t * 0x14;
    unsigned first = FIELD(dq, 0xC, unsigned), n = FIELD(dq, 0x10, unsigned);
    for (unsigned k = n; k-- > 0; ) {   // the newest first
        uint8_t *u = deque_at(dq, first + k);
        if (!u || FIELD(u, 0x3C, int) <= 0 || FIELD(u, 0x33B, uint8_t) || FIELD(u, 0x341, uint8_t) || FIELD(u, 0x68, int))
            continue;
        void *goal = FIELD(u, 0x1EC, void *);
        if (goal && !strcmp(rtti_name(goal), "EEUGGather@@")) return u;
    }
    return NULL;
}
static void econ_rebalance(uint8_t *rm) {
    void *pl = rm_player(rm);
    if (!g_rebalance || !pl) return;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    uint32_t *target = (uint32_t *)(rm + RM_TARGET);
    int moves = 0, dry = 0;   // dry: donor lists without a movable citizen
    for (int r = 4; r < 7 && moves < 2; r++) {
        while (moves < 2 && (int)target[r] - rm_count(rm, r) >= 2) {
            int d = -1, best = 2;
            for (int t = 2; t < 7; t++) {
                int s = rm_count(rm, t) - (int)target[t];
                if (t != r && !(dry >> t & 1) && s > best) { best = s; d = t; }
            }
            if (d < 0) break;
            uint8_t *u = movable_gatherer(rm, d);
            if (!u) { dry |= 1 << d; continue; }
            void *site = hook_find_mine(rm, NULL, r, u, 1);   // with mine_choice: nearest to a drop site
            if (!site) break;   // no free, safe mine of this resource
            int before = rm_count(rm, r);
            ((void (__thiscall *)(void *, void *, void *, int))FN_ASSIGN_TO_SITE)(rm, u, site, 0);
            if (rm_count(rm, r) == before) { dry |= 1 << d; continue; }
            g_moved[idx][r]++;
            moves++;
        }
    }
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

static int unit_cost(void *pl, int type, int r);

// Military request statistics, logged every 5 minutes: how often the AI asks for soldiers, how many it gets
// and which resource was short when a request failed (unit cost = object data 0x9418A0[type] +0x24..).
static struct { unsigned req, trained, short_of[7]; uint32_t last_log; int ftype[4]; unsigned fcount[4]; } g_mil[16];
static void mil_note(void *pl, unsigned idx, const int *req, const int *visible, int spent, uintptr_t site) {
    g_mil[idx].req++;
    if (spent) { g_mil[idx].trained += req[1] > 0 ? (unsigned)req[1] : 1; return; }
    int k = 0;   // most requested failing types (4 slots, the rarest one is replaced)
    int key = (int)((site & 0xFFFFF) << 11) | (req[0] & 0x7FF);   // call site + unit type
    while (k < 4 && g_mil[idx].fcount[k] && g_mil[idx].ftype[k] != key) k++;
    if (k == 4) { k = 0; for (int j = 1; j < 4; j++) if (g_mil[idx].fcount[j] < g_mil[idx].fcount[k]) k = j; g_mil[idx].fcount[k] = 0; }
    g_mil[idx].ftype[k] = key;
    g_mil[idx].fcount[k]++;
    for (int r = 2; r < 7; r++)
        if (visible[r] < unit_cost(pl, req[0], r)) g_mil[idx].short_of[r]++;
}

// Price of a unit for this player: the unit's tech (object data 0x9418A0[type] +0x4E0 = tech id, looked up with
// 0x4019A1 on the player's tech tree +0x9D4; tech +8 = data, costs +0x24.. food..iron, like the epoch techs)
// x the AdjustCost percentage (vtable slot 27 0x430FBD: only while the Cheat toggle is on; easy +0xD48 int,
// medium +0xBB4 float, hard +0xD4C int).
#define FN_TECH_LOOKUP 0x004019A1
static int unit_cost(void *pl, int type, int r) {
    uint8_t *obj = (type >= 0 && type < 2048) ? ((uint8_t **)0x009418A0)[type] : NULL;
    uint8_t *tt = FIELD(pl, PL_TECHTREE, uint8_t *);
    if (!obj || !tt || r < 2 || r > 6) return 0;
    uint8_t *tech = ((uint8_t *(__thiscall *)(void *, int))FN_TECH_LOOKUP)(tt, FIELD(obj, 0x4E0, int));
    uint8_t *data = tech ? FIELD(tech, 8, uint8_t *) : NULL;
    if (!data) return 0;
    float c = (float)FIELD(data, 0x24 + (r - 2) * 4, int);
    if (FIELD(pl, PL_CHEAT, uint8_t)) {
        int d = FIELD(pl, PL_DIFFICULTY, int);
        float pct = d == 0 ? (float)FIELD(pl, 0xD48, int) : d == 2 ? (float)FIELD(pl, 0xD4C, int) : FIELD(pl, 0xBB4, float);
        c = c * pct * 0.01f;
    }
    return (int)c;
}

// The budget decides before the game does: a request it doesn't allow is refused here and the original is not
// called. (Hiding stock during the call leaked: 0x49014F counts the unit as "in production" (unit manager
// +0x13C) even when the building queue 0x5E8908 then refuses it for lack of resources, and that counter is
// subtracted from the population cap - after a while the AI could train nothing at all.)
static int g_priority_mil;   // set by the siege controller around its own requests
static uintptr_t __fastcall hook_train_units(void *um, void *edx, int *req, uintptr_t b) {
    (void)edx;
    void *pl = um_owner(um);
    int *stock = pl ? (int *)((uint8_t *)pl + PL_STOCK) : NULL;
    int before[7] = { 0 }, reserve[7] = { 0 }, visibles[7] = { 0 }, blocked = 0, afford = 0;
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
            if (military && !g_priority_mil) {   // priority requests (siege) spend the stock above the reserve
                int allowed = (int)g_bud[idx].mil[r];
                int excess = stock[r] - reserve[r] - (r >= 4 ? g_hoard_cap_mineral : g_hoard_cap);   // never hoard beyond the cap
                if (excess > allowed) allowed = excess;
                if (visible > allowed) visible = allowed;
            }
            if (no_work) visible = 0;
            if (visible < 0) visible = 0;
            if (visible > stock[r]) visible = stock[r];
            visibles[r] = visible;
        }
        if (no_work) g_bud[idx].cit_blocked++;
        // as many of the requested units as the budget pays for (citizens come in batches of 10)
        int want = req[1] > 0 ? req[1] : 1;
        afford = want;
        for (int r = 2; r < 7; r++) {
            int c = unit_cost(pl, req[0], r);
            if (c > 0 && visibles[r] / c < afford) afford = visibles[r] / c;
        }
        blocked = afford <= 0;
    } else if (stock) memcpy(visibles, stock, sizeof visibles);
    uintptr_t res = 0;
    if (blocked) { if (b) *(unsigned *)b = 0; }
    else {
        int saved_count = req[1];
        if (afford > 0 && afford < req[1]) req[1] = afford;   // the caller reads back how many via *b
        res = ((uintptr_t (__thiscall *)(void *, int *, uintptr_t))FN_TRAIN_UNITS)(um, req, b);
        req[1] = saved_count;
    }
    if (stock) {
        int any = 0;
        for (int r = 2; r < 7; r++) {
            int spent = before[r] - stock[r];
            any |= spent > 0;
            if (cfg->budget && military && spent > 0) { g_bud[idx].mil[r] -= spent; g_bud[idx].mil_spent[r] += spent; }
        }
        if (military) mil_note(pl, idx, req, visibles, any, (uintptr_t)__builtin_return_address(0) - 5);
        trace_train(pl, req, res, before, reserve);
        static uint32_t last_log[16];
        uint32_t now = game_time();
        if (cfg->budget && g_log && (now < last_log[idx] || now - last_log[idx] >= 60000)) {
            last_log[idx] = now;
            EnterCriticalSection(&g_lock);
            fprintf(g_log, "# B %u P%u food=%d reserve=%d mil_account=%.0f income=%d mil_spent=%d idle=%d cit_blocked=%d moved S%u G%u I%u\n",
                    now, idx, stock[2], reserve[2], g_bud[idx].mil[2], g_bud[idx].income[2], g_bud[idx].mil_spent[2],
                    idle_citizens(pl), g_bud[idx].cit_blocked, g_moved[idx][4], g_moved[idx][5], g_moved[idx][6]);
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
    g_hoard_cap_mineral = cfg_int(aimod_ini, autotest_ini, 1, "hoard_cap_mineral", 600);
    g_min_workforce = cfg_int(aimod_ini, autotest_ini, 1, "min_workforce", 20);
    g_wild_share = cfg_float(aimod_ini, autotest_ini, 1, "wild_food_share", "0.6");
    g_reserve_window_ms = cfg_int(aimod_ini, autotest_ini, 1, "reserve_window_min", 5) * 60000;
    g_rebalance = cfg_int(aimod_ini, autotest_ini, 1, "rebalance_workers", 1);
    g_build_fix = cfg_int(aimod_ini, autotest_ini, 1, "build_fix", 1);
    int ok = 0;
    void **sp = (void **)(VT_COMPUTER_PLAYER + 45 * 4);
    if (*sp == (void *)FN_CP_PAY) {
        DWORD old;
        VirtualProtect(sp, 4, PAGE_READWRITE, &old);
        *sp = (void *)hook_pay;
        VirtualProtect(sp, 4, old, &old);
    }
    void **slot = (void **)(VT_COMPUTER_PLAYER + 18 * 4);
    if (*slot == (void *)FN_CP_ADD_RESOURCE) {
        DWORD old;
        VirtualProtect(slot, 4, PAGE_READWRITE, &old);
        *slot = (void *)hook_add_resource;
        VirtualProtect(slot, 4, old, &old);
        ok++;
    }
    patch_call(CALL_PLACE, FN_PLACE, hook_place);
    for (size_t i = 0; i < sizeof k_train_sites / sizeof *k_train_sites; i++)
        ok += patch_call(k_train_sites[i], FN_TRAIN_UNITS, hook_train_units);
    if (g_log) fprintf(g_log, "# economy hooks: %d/%d\n", ok, (int)(sizeof k_train_sites / sizeof *k_train_sites) + 1);
}
