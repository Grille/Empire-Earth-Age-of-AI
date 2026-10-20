// CITIZEN PRODUCTION (docs/economy.md "Citizen production"). Included by eeaimod.c after mines.c.
// The playtester: "Filling the mines is a task after ensuring enough food income that citizens can continuously be
// trained; not enough citizens should never be a real limit to filling a mine after the early game or in very low pop
// games."
// How the game trains citizens (unit manager update 0x490AD6, at 0x490B60): a request for citizens (type 0x20) only
// while the room (cap vf31 - in production um +0x13C - population 0x556E11) is above 0, the attention flag um +0x140 is
// set (0x42F912: per period only player +0xE20 + 1 of 9 activities), no citizen request waits in the 14 request lists
// (0x48E9D8) and no producer has a citizen in production (0x48EA2E: building +0x25C the tech in production, +0x4C/+0x50
// the queued techs) - and only for the citizens wanted (0x48EDDB: player +0xBDC x cap, 0.45) minus those there
// (0x48F819: citizens of both sexes and the epoch's type), at most 10 (5 on water maps). 0x49014F (hook_train_units)
// hands the request round-robin to every finished producer of the type (the citizen tech's data +0x10..+0x14 list of
// building types: Town Center, Capitol).
// "# cit:" once a minute (autotest): per producer and second of the minute whether it trained a citizen, something
// else or nothing, and for the idle seconds the reason, in the order the game checks: cap (no room), target (the
// citizens wanted are there), a request waits - pfood (the game's own check, vf32, refuses: under 50 food in the
// stock) or pbud (the mod's budget refused it: req / blocked count those, by food short, the epoch reserve, another
// reserve) -, batch (another producer still trains the last batch), att (um +0x140 off), rest. Then the stock's food,
// food gatherers / target, all workers and citizen_plan's goal.
struct cz_min { unsigned prod, cit, other, idle, cap, target, pfood, pbud, batch, att, rest; };
static struct cz_st {
    uint32_t t, min;           // last sample, its minute
    struct cz_min m;           // this minute's producer-seconds
    unsigned calls, blocked, b_food, b_reserve, b_epoch, trained;   // citizen requests through the budget (this minute)
    int wanted, have, room, cap;
} g_cz[16];
static int g_cz_tech[2] = { -1, -1 }, g_cz_ptypes[8], g_cz_np = -1;
static struct { uint32_t t; int goal, game, feed, home, slots, extra; } g_czp[16];   // citizen_plan's last pass (cz_targets)

static void cz_new_game(void) { memset(g_cz, 0, sizeof g_cz); memset(g_czp, 0, sizeof g_czp); g_cz_np = -1; }

// the producer types of citizens (once a game: the citizen tech's building list)
static void cz_types(void *pl) {
    if (g_cz_np >= 0) return;
    uint8_t *tt = FIELD(pl, PL_TECHTREE, uint8_t *);
    if (!tt) return;
    g_cz_np = 0;
    for (int k = 0; k < 2; k++) {
        uint8_t *td = type_data(k ? 480 : 32);
        g_cz_tech[k] = td ? FIELD(td, 0x4E0, int) : -1;
        uint8_t *t = td ? ((uint8_t *(__thiscall *)(void *, int))0x004019A1)(tt, g_cz_tech[k]) : NULL, *d = t ? FIELD(t, 8, uint8_t *) : NULL;
        int *b = d ? FIELD(d, 0x10, int *) : NULL, *e = d ? FIELD(d, 0x14, int *) : NULL;
        for (int *p = b; b && e > b && e - b < 16 && p < e; p++) {
            int j;
            for (j = 0; j < g_cz_np && g_cz_ptypes[j] != *p; j++) ;
            if (j == g_cz_np && g_cz_np < 8) g_cz_ptypes[g_cz_np++] = *p;
        }
    }
    if (!g_cz_np) { g_cz_ptypes[0] = 35; g_cz_ptypes[1] = 41; g_cz_np = 2; }   // Town Center, Capitol
}
static int cz_is_cit_tech(int t) { return t >= 0 && (t == g_cz_tech[0] || t == g_cz_tech[1]); }

// a citizen request through the budget (econ.c hook_train_units): refused, and why
static void cz_budget_note(void *pl, unsigned idx, const int *req, const int *stock, const int *visibles, int blocked, int afford) {
    struct cz_st *z = &g_cz[idx & 15];
    z->calls++;
    if (!blocked) { z->trained += (unsigned)(afford > 0 ? afford : 0); return; }
    z->blocked++;
    int c = unit_cost(pl, req[0], 2);
    if (stock[2] < c) { z->b_food++; return; }
    int er[7] = { 0 };
    spend_reserve(pl, idx, er, NULL);
    if (stock[2] - er[2] < c) z->b_epoch++;
    else z->b_reserve++;
    (void)visibles;
}

// once a game second: every producer of the player
static void cz_sample(void *pl, unsigned idx) {
    struct cz_st *z = &g_cz[idx];
    uint8_t *cm = FIELD(pl, 0xB90, uint8_t *), *um = cm ? FIELD(cm, 0x44, uint8_t *) : NULL;
    if (!um) return;
    cz_types(pl);
    int idle = 0;
    for (int k = 0; k < g_cz_np; k++) {
        uint8_t *q = ((uint8_t *(__thiscall *)(void *, int))0x0048F8AF)(um, g_cz_ptypes[k]);
        for (unsigned j = 0, f = q ? FIELD(q, 0xC, unsigned) : 0, n = q ? FIELD(q, 0x10, unsigned) : 0; j < n && j < 64; j++) {
            uint8_t *b = deque_at(q, f + j);
            if (!b || FIELD(b, 0x3C, int) <= 0 || !b[0x340]) continue;
            z->m.prod++;
            int cur = FIELD(b, 0x25C, int), *qb = FIELD(b, 0x4C, int *), *qe = FIELD(b, 0x50, int *), cit = cz_is_cit_tech(cur);
            int busy = cur > 0 && cur != 100000;
            for (int *p = qb; qb && qe > qb && qe - qb < 64 && p < qe; p++) { busy = 1; if (cz_is_cit_tech(*p)) cit = 1; }
            if (cit) z->m.cit++;
            else if (busy) z->m.other++;
            else { z->m.idle++; idle++; }
        }
    }
    int cap = ((int (__thiscall *)(void *, int))(*(void ***)pl)[31])(pl, 0);
    int room = cap - FIELD(um, 0x13C, int) - ((int (__thiscall *)(void *, int, int))0x00556E11)(pl, 1, 0);
    int wanted = ((int (__thiscall *)(void *))0x0048EDDB)(um), have = ((int (__thiscall *)(void *))0x0048F819)(um);
    z->wanted = wanted; z->have = have; z->room = room; z->cap = cap;
    if (!idle) return;
    char (__thiscall *pend)(void *, int) = (void *)0x0048E9D8, (__thiscall *inprod)(void *, int) = (void *)0x0048EA2E;
    unsigned *r;
    if (room <= 0) r = &z->m.cap;
    else if (wanted - have <= 0) r = &z->m.target;
    else if (pend(um, 0x20) || pend(um, 0x1E0))   // the game asks vf32 (can pay one) before 0x49014F: food short, else the budget
        r = FIELD(pl, PL_STOCK + 8, int) < unit_cost(pl, 32, 2) ? &z->m.pfood : &z->m.pbud;
    else if (inprod(um, 0x20) || inprod(um, 0x1E0)) r = &z->m.batch;
    else if (!um[0x140]) r = &z->m.att;
    else r = &z->m.rest;
    *r += (unsigned)idle;
}

static void cz_tick(void *pl, uint32_t now) {
    if (!g_auto.enabled || !g_log || !pl) return;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    struct cz_st *z = &g_cz[idx];
    if (now < z->t) memset(z, 0, sizeof *z);
    if (z->t && now - z->t < 1000) return;
    z->t = now ? now : 1;
    if (now / 60000 != z->min) {
        if (z->m.prod || z->calls) {
            uint8_t *cm = FIELD(pl, 0xB90, uint8_t *), *rm = cm ? FIELD(cm, 0x38, uint8_t *) : NULL;
            int fw = rm ? rm_count(rm, 2) : 0, ft = rm ? FIELD(rm, RM_TARGET + 8, int) : 0;
            EnterCriticalSection(&g_lock);
            fprintf(g_log, "# cit: P%u at %u min: prod %u cit %u other %u idle %u | cap %u target %u pfood %u pbud %u batch %u att %u rest %u | "
                    "req %u blocked %u (food %u epoch %u reserve %u) trained %u | wanted %d have %d room %d cap %d | food %d fw %d ft %d wf %d "
                    "goal %d feed %d slots %d home %d extra %d\n",
                    idx, z->min, z->m.prod, z->m.cit, z->m.other, z->m.idle, z->m.cap, z->m.target, z->m.pfood, z->m.pbud, z->m.batch,
                    z->m.att, z->m.rest, z->calls, z->blocked, z->b_food, z->b_epoch, z->b_reserve, z->trained, z->wanted, z->have, z->room,
                    z->cap, FIELD(pl, PL_STOCK + 8, int), fw, ft, workforce(pl), g_czp[idx].goal, g_czp[idx].feed, g_czp[idx].slots,
                    g_czp[idx].home, g_czp[idx].extra);
            LeaveCriticalSection(&g_lock);
        }
        memset(&z->m, 0, sizeof z->m);
        z->calls = z->blocked = z->b_food = z->b_reserve = z->b_epoch = z->trained = 0;
        z->min = now / 60000;
    }
    cz_sample(pl, idx);
}

// ---- citizen_plan ([all], bits; [autotest] v_citizen_plan for the variant players; docs/economy.md) ----
// Measured first (# cit, the base AI at hard, 60 min, 3 maps x cheat x 20 seeds x both seats): from minute 30 the
// producers stood idle 77-85% of the time, mostly because the game's citizens wanted (+0xBDC x cap: 75 of a cap of
// 166) were there ("target", 42-58% of their time; the base AI's own share is the same, but with the cheat it has
// twice the workers by then); minutes 10-30 on Continental 31-37% the mod's budget (all of it the epoch reserve of
// copper_plan), on the islands 13-39% too little food in the stock (pfood; with the cheat 42-62% from minute 30).
// 1 GOAL: from the Copper Age the citizens wanted at the request (0x490BC2: the call of 0x48EDDB) are the plan's goal
//   when more: every mine slot within 12 tiles of a drop site (the home continent's and wherever the AI holds one;
//   mf_scan, mines.c) or the metal gatherers if more, the food and wood gatherers, the idle and 4 builders - within the
//   population cap less citizen_army_share (40%) for the army; the home mines' slots always fit (at most 80% of the
//   cap). Then the metals' targets go up to their slots near drop sites, from food beyond its floor (FEED's gatherers,
//   its fields + 2) and wood beyond its late target (late_targets bit 1) - never from a mine.
// 2 FEED: from the Copper Age food has at least the gatherers that pay the producers the missing citizens need - one
//   producer (Town Center, Capitol) per 10 citizens missing, ~28 s and 50 food a citizen (measured: producer-seconds
//   per citizen trained) - over the food per worker-minute (rates_update); taken from wood beyond its target and from
//   metal targets above their gatherers. The first try fed every producer nonstop and took from the largest target
//   (the mines): +19 workers on Continental, all on food, the mines no fuller.
// 4 DROP: mines.c's DROP (a Settlement at the home continent's unworked mine) with the plan.
// 8 FIRST: citizens below the goal pay their food before the epoch reserve while the food income (per minute) is at
//   least twice what the producers spend nonstop - the reserve still fills.
// Measured (paired with the build without, 40 games a cell): GOAL + FEED (3) workers at 60 min +15 to +35 (Small
// Islands with the cheat +8), mine gatherers +2 to +8, stone+gold+iron income +700 to +2000 an hour (Small Islands
// with the cheat -100: its mines' 30 slots were full), the base AI's wins with the cheat 45 -> 31 of 120; soldiers
// -8 on Continental without the cheat, kills - losses within the noise but Large Islands with the cheat -16 (7). 7 (+ DROP) 6-9 more on Continental's mines but 13 soldiers fewer; FIRST
// changed nothing. Head-to-head: workers +24, razed +6; lowpop (cap 30) workers +10, kills - losses +23. Default 3.
static int g_cit_plan = 3, g_v_cit_plan = -1, g_cit_train_ms = 28000;
static float g_cit_army = 0.40f;   // [all] citizen_army_share
#define CIT_PLAN(idx) PV(idx, g_v_cit_plan, g_cit_plan)

// finished producers of citizens (Town Center, Capitol)
static int cz_producers(void *pl) {
    uint8_t *cm = FIELD(pl, 0xB90, uint8_t *), *um = cm ? FIELD(cm, 0x44, uint8_t *) : NULL;
    int n = 0;
    if (!um) return 0;
    cz_types(pl);
    for (int k = 0; k < g_cz_np; k++) {
        uint8_t *q = ((uint8_t *(__thiscall *)(void *, int))0x0048F8AF)(um, g_cz_ptypes[k]);
        for (unsigned j = 0, f = q ? FIELD(q, 0xC, unsigned) : 0, c = q ? FIELD(q, 0x10, unsigned) : 0; j < c && j < 64; j++) {
            uint8_t *b = deque_at(q, f + j);
            if (b && FIELD(b, 0x3C, int) > 0 && b[0x340]) n++;
        }
    }
    return n;
}
// food gatherers that pay the producers the missing citizens need: one producer per 10 citizens still missing (a
// producer makes ~10 in 5 minutes), all of them at most
static int cz_feed(void *pl, unsigned idx, int missing) {
    if (missing <= 0) return 0;
    float pw = g_rate[idx].per_worker[2];
    if (pw < 5.0f) pw = 5.0f;
    int np = cz_producers(pl), need = (missing + 9) / 10;
    if (np > need) np = need;
    float per_min = (float)np * (float)unit_cost(pl, 32, 2) * 60000.0f / (float)g_cit_train_ms;
    return (int)(per_min / pw + 0.99f);
}
// one worker from the largest of the targets in [lo, hi] that stays above its floor; -1: none
static int cz_take(uint32_t *target, const int *floor_, int lo, int hi) {
    int from = -1;
    for (int r = lo; r <= hi; r++)
        if ((int)target[r] > floor_[r] && (from < 0 || (int)target[r] - floor_[r] > (int)target[from] - floor_[from])) from = r;
    if (from >= 0) target[from]--;
    return from;
}
// after the worker targets (hook_compute_targets, after mine_fill): the goal, FEED and the mines' slots
static void cz_targets(uint8_t *rm) {
    void *pl = rm_player(rm);
    if (!pl) return;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    int plan = CIT_PLAN(idx);
    uint32_t now = game_time();
    if (now < g_czp[idx].t) memset(&g_czp[idx], 0, sizeof g_czp[idx]);
    g_czp[idx].t = now ? now : 1;
    g_czp[idx].goal = g_czp[idx].feed = g_czp[idx].extra = 0;
    if (!(plan & 3) || current_epoch(pl) < FARM_EPOCH) return;
    uint8_t *cm = FIELD(pl, 0xB90, uint8_t *), *um = cm ? FIELD(cm, 0x44, uint8_t *) : NULL;
    if (!um) return;
    uint32_t *target = (uint32_t *)(rm + RM_TARGET);
    int wf = workforce(pl), game = ((int (__thiscall *)(void *))0x0048EDDB)(um);
    g_czp[idx].game = game;
    struct mf_st *m = &g_mf[idx];
    mf_scan(pl, idx, now);
    int slots = 0, home = 0;
    for (int r = 4; r < 7; r++) {
        int g = rm_count(rm, r);
        slots += m->slots[r] > g ? m->slots[r] : g;
        home += m->home_slots[r];
    }
    // the goal: the mines' slots (or their gatherers, if more), food, wood, the idle and 4 builders - within the cap
    // less the army's share, the home mines always in
    int cap = ((int (__thiscall *)(void *, int))(*(void ***)pl)[31])(pl, 0);
    int goal = slots + rm_count(rm, 2) + rm_count(rm, 3) + rm_count(rm, 0) + 4;
    int lim = cap - (int)((float)cap * g_cit_army), hl = home + rm_count(rm, 2) / 2 + 4;
    if (hl > cap * 4 / 5) hl = cap * 4 / 5;
    if (lim < hl) lim = hl;
    if (goal > lim) goal = lim;
    if (goal < game) goal = game;
    g_czp[idx].goal = plan & 1 ? goal : 0;
    g_czp[idx].home = home; g_czp[idx].slots = slots;
    // floors: food keeps FEED's gatherers and its fields + 2 (farmers), wood its late target (late_targets bit 1),
    // a metal its gatherers (nobody is pulled off a mine for food)
    void *fq = ((void *(__thiscall *)(void *, int))0x0048F8AF)(um, 0x2A);
    int fields = fq ? FIELD(fq, 0x10, int) : 0;
    int feed = plan & 2 ? cz_feed(pl, idx, goal - wf) : 0;
    g_czp[idx].feed = feed;
    int fl[7] = { 0, 0, 0, g_wood_want[idx] > 4 ? g_wood_want[idx] : 4, 0, 0, 0 };
    for (int r = 4; r < 7; r++) fl[r] = rm_count(rm, r) > 2 ? rm_count(rm, r) : 2;
    while ((int)target[2] < feed && cz_take(target, fl, 3, 6) >= 0) target[2]++;   // FEED: the citizens' food first
    if (!(plan & 1)) return;
    // the mines' slots near drop sites: from food beyond its floor and wood beyond its need
    fl[2] = feed > fields + 2 ? feed : fields + 2;
    fl[4] = fl[5] = fl[6] = 0x7FFFFFFF;
    int moved = 0;
    for (int r = 4; r < 7; r++)
        while ((int)target[r] < m->slots[r] && cz_take(target, fl, 2, 3) >= 0) { target[r]++; moved++; }
    g_czp[idx].extra = moved;
}
// the citizens wanted at the request (0x490BC2: call 0x48EDDB thiscall(um))
static int __fastcall hook_cit_wanted(void *um, void *edx) {
    (void)edx;
    int n = ((int (__thiscall *)(void *))0x0048EDDB)(um);
    void *pl = FIELD(um, 0x134, void *);
    if (!pl || is_vanilla(pl)) return n;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!(CIT_PLAN(idx) & 1) || game_time() < g_czp[idx].t || game_time() - g_czp[idx].t > 15000) return n;
    return g_czp[idx].goal > n ? g_czp[idx].goal : n;
}
// FIRST (train_budget, a citizen's food): the stock without the epoch reserve while the income pays both
static int cz_visible_food(void *pl, unsigned idx, const int *stock, int visible) {
    if (!(CIT_PLAN(idx) & 8) || current_epoch(pl) < 1) return visible;
    int goal = g_czp[idx].goal ? g_czp[idx].goal : g_czp[idx].game;
    if (goal && workforce(pl) >= goal) return visible;
    float spend = (float)cz_producers(pl) * (float)unit_cost(pl, 32, 2) * 60000.0f / (float)g_cit_train_ms;
    if (g_rate[idx].rate[2] < 2.0f * spend) return visible;
    return stock[2] > visible ? stock[2] : visible;
}
static void cz_install(const char *aimod_ini, const char *autotest_ini) {
    g_cit_plan = cfg_int(aimod_ini, autotest_ini, 1, "citizen_plan", 3);
    g_cit_army = cfg_share(aimod_ini, autotest_ini, 1, "citizen_army_share", 0.40, 0, 0.9);
    g_cit_train_ms = cfg_int(aimod_ini, autotest_ini, 1, "citizen_train_ms", 28000);
    if (g_cit_train_ms < 5000) g_cit_train_ms = 5000;
    if (autotest_ini) g_v_cit_plan = ini_int("autotest", "v_citizen_plan", -1, autotest_ini);
    BOOL ok = patch_call(0x00490BC2, 0x0048EDDB, hook_cit_wanted);
    if (g_log) fprintf(g_log, "# citizens: citizen_plan %d (variant %d), army share %.0f%%, hook %s\n", g_cit_plan, g_v_cit_plan,
                       g_cit_army * 100.0f, ok ? "ok" : "FAILED");
}
