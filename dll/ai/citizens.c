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
internal struct cz_st {
    uint32_t t, min;           // last sample, its minute
    struct cz_min m;           // this minute's producer-seconds
    unsigned calls, blocked, b_food, b_reserve, b_epoch, trained;   // citizen requests through the budget (this minute)
    int wanted, have, room, cap;
} g_cz[16];
internal int g_cz_tech[2] = { -1, -1 }, g_cz_ptypes[8], g_cz_np = -1;
internal struct { uint32_t t; int goal, game, feed, home, slots, extra; } g_czp[16];   // citizen_plan's last pass (cz_targets)

internal void cz_new_game(void) { memset(g_cz, 0, sizeof g_cz); memset(g_czp, 0, sizeof g_czp); g_cz_np = -1; }

// a building is finished at [unit +0x30] +0x340 (unit +0x340 is something else)
internal int cz_finished(ee_unit *b) { ee_unit *bs = unit_get_physics(b); return bs && unit_get_finished(bs); }

// the producer types of citizens (once a game: the citizen tech's building list)
internal void cz_types(ee_player *pl) {
    if (g_cz_np >= 0) return;
    ee_tech_tree *tt = FIELD(pl, PL_TECHTREE, ee_tech_tree *);
    if (!tt) return;
    g_cz_np = 0;
    for (int k = 0; k < 2; k++) {
        ee_unit_type *td = type_data(k ? UT(FEMALE_CITIZEN) : UT(CITIZEN));
        g_cz_tech[k] = td ? type_get_tech(td) : -1;
        ee_tech *t = td ? tech_lookup(tt, g_cz_tech[k]) : NULL;
        uint8_t *d = t ? FIELD(t, OF(TECH, 0x8), uint8_t *) : NULL;
        int *b = d ? FIELD(d, OF(TECHDATA, 0x10), int *) : NULL, *e = d ? FIELD(d, OF(TECHDATA, 0x14), int *) : NULL;
        for (int *p = b; b && e > b && e - b < 16 && p < e; p++) {
            int j;
            for (j = 0; j < g_cz_np && g_cz_ptypes[j] != *p; j++) ;
            if (j == g_cz_np && g_cz_np < 8) g_cz_ptypes[g_cz_np++] = *p;
        }
    }
    if (!g_cz_np) { g_cz_ptypes[0] = UT(B_TOWN_CENTER); g_cz_ptypes[1] = UT(B_CAPITOL); g_cz_np = 2; }
}
internal int cz_is_cit_tech(int t) { return t >= 0 && (t == g_cz_tech[0] || t == g_cz_tech[1]); }

// a citizen request through the budget (econ.c hook_train_units): refused, and why
internal void cz_budget_note(ee_player *pl, unsigned idx, const int *req, const int *stock, const int *visibles, int blocked, int afford) {
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
internal void cz_sample(ee_player *pl, unsigned idx) {
    struct cz_st *z = &g_cz[idx];
    uint8_t *cm = FIELD(pl, OF(PL, 0xB90), uint8_t *);
    ee_unit_mgr *um = cm ? FIELD(cm, OF(CM, 0x44), ee_unit_mgr *) : NULL;
    if (!um) return;
    cz_types(pl);
    int idle = 0;
    for (int k = 0; k < g_cz_np; k++) {
        uint8_t *q = um_type_list(um, g_cz_ptypes[k]);
        for (unsigned j = 0, f = q ? FIELD(q, 0xC, unsigned) : 0, n = q ? FIELD(q, 0x10, unsigned) : 0; j < n && j < 64; j++) {
            ee_unit *b = deque_at(q, f + j);
            if (!b || unit_get_hp(b) <= 0 || !cz_finished(b)) continue;
            z->m.prod++;
            int cur = unit_get_producing(b), *qb = unit_get_queue_begin(b), *qe = unit_get_queue_end(b), cit = cz_is_cit_tech(cur);
            int busy = cur > 0 && cur != 100000;
            for (int *p = qb; qb && qe > qb && qe - qb < 64 && p < qe; p++) { busy = 1; if (cz_is_cit_tech(*p)) cit = 1; }
            if (cit) z->m.cit++;
            else if (busy) z->m.other++;
            else { z->m.idle++; idle++; }
        }
    }
    int cap = unit_limit(pl);
    int room = cap - FIELD(um, OF(UM, 0x13C), int) - population(pl, 1, 0);
    int wanted = um_citizens_wanted(um), have = um_citizens(um);
    z->wanted = wanted; z->have = have; z->room = room; z->cap = cap;
    if (!idle) return;
    unsigned *r;
    if (room <= 0) r = &z->m.cap;
    else if (wanted - have <= 0) r = &z->m.target;
    else if ((ut_has(UT(CITIZEN)) && um_pending(um, UT(CITIZEN))) || (ut_has(UT(FEMALE_CITIZEN)) && um_pending(um, UT(FEMALE_CITIZEN))))   // the game asks vf32 (can pay one) before 0x49014F: food short, else the budget
        r = FIELD(pl, PL_STOCK + 8, int) < unit_cost(pl, UT(CITIZEN), 2) ? &z->m.pfood : &z->m.pbud;
    else if ((ut_has(UT(CITIZEN)) && um_in_production(um, UT(CITIZEN))) || (ut_has(UT(FEMALE_CITIZEN)) && um_in_production(um, UT(FEMALE_CITIZEN)))) r = &z->m.batch;
    else if (!FIELD(um, OF(UM, 0x140), uint8_t)) r = &z->m.att;
    else r = &z->m.rest;
    *r += (unsigned)idle;
}

internal void wf_log(ee_player *pl, unsigned idx, unsigned min);   // below: workforce_plan's # wf line
internal void cz_tick(ee_player *pl, uint32_t now) {
    if (!g_autotest || !g_log || !pl) return;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    struct cz_st *z = &g_cz[idx];
    if (now < z->t) memset(z, 0, sizeof *z);
    if (z->t && now - z->t < 1000) return;
    z->t = now ? now : 1;
    if (now / 60000 != z->min) {
        if (z->m.prod || z->calls) {
            uint8_t *cm = FIELD(pl, OF(PL, 0xB90), uint8_t *);
            ee_resource_mgr *rm = cm ? FIELD(cm, OF(CM, 0x38), ee_resource_mgr *) : NULL;
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
        if (z->t > 60000) wf_log(pl, idx, z->min);   // workforce_plan's line (below)
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
internal int g_cit_plan = 3, g_v_cit_plan = -1, g_cit_train_ms = 28000;
internal float g_cit_army = 0.40f;   // [all] citizen_army_share
#define CIT_PLAN(idx) PV(idx, g_v_cit_plan, g_cit_plan)
internal int expand_cit_need(ee_player *pl, unsigned idx);   // expand.c: the citizens an outpost still needs (EXTRA)
internal int wf_cap_wanted(unsigned idx, int n);         // below: workforce_plan CAP

// finished producers of citizens (Town Center, Capitol)
internal int cz_producers(ee_player *pl) {
    uint8_t *cm = FIELD(pl, OF(PL, 0xB90), uint8_t *);
    ee_unit_mgr *um = cm ? FIELD(cm, OF(CM, 0x44), ee_unit_mgr *) : NULL;
    int n = 0;
    if (!um) return 0;
    cz_types(pl);
    for (int k = 0; k < g_cz_np; k++) {
        uint8_t *q = um_type_list(um, g_cz_ptypes[k]);
        for (unsigned j = 0, f = q ? FIELD(q, 0xC, unsigned) : 0, c = q ? FIELD(q, 0x10, unsigned) : 0; j < c && j < 64; j++) {
            ee_unit *b = deque_at(q, f + j);
            if (b && unit_get_hp(b) > 0 && cz_finished(b)) n++;
        }
    }
    return n;
}
// food gatherers that pay the producers the missing citizens need: one producer per 10 citizens still missing (a
// producer makes ~10 in 5 minutes), all of them at most
internal int cz_feed(ee_player *pl, unsigned idx, int missing) {
    if (missing <= 0) return 0;
    float pw = g_rate[idx].per_worker[2];
    if (pw < 5.0f) pw = 5.0f;
    int np = cz_producers(pl), need = (missing + 9) / 10;
    if (np > need) np = need;
    float per_min = (float)np * (float)unit_cost(pl, UT(CITIZEN), 2) * 60000.0f / (float)g_cit_train_ms;
    return (int)(per_min / pw + 0.99f);
}
// one worker from the largest of the targets in [lo, hi] that stays above its floor; -1: none
internal int cz_take(uint32_t *target, const int *floor_, int lo, int hi) {
    int from = -1;
    for (int r = lo; r <= hi; r++)
        if ((int)target[r] > floor_[r] && (from < 0 || (int)target[r] - floor_[r] > (int)target[from] - floor_[from])) from = r;
    if (from >= 0) target[from]--;
    return from;
}
// after the worker targets (hook_compute_targets, after mine_fill): the goal, FEED and the mines' slots
internal void cz_targets(ee_resource_mgr *rm) {
    ee_player *pl = rm_player(rm);
    if (!pl) return;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    int plan = CIT_PLAN(idx);
    uint32_t now = game_time();
    if (now < g_czp[idx].t) memset(&g_czp[idx], 0, sizeof g_czp[idx]);
    g_czp[idx].t = now ? now : 1;
    g_czp[idx].goal = g_czp[idx].feed = g_czp[idx].extra = 0;
    if (!(plan & 3) || current_epoch(pl) < FARM_EPOCH) return;
    uint8_t *cm = FIELD(pl, OF(PL, 0xB90), uint8_t *);
    ee_unit_mgr *um = cm ? FIELD(cm, OF(CM, 0x44), ee_unit_mgr *) : NULL;
    if (!um) return;
    uint32_t *target = (uint32_t *)FIELD_AT(rm, RM_TARGET);
    int wf = workforce(pl), game = um_citizens_wanted(um);
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
    int cap = unit_limit(pl);
    int goal = slots + rm_count(rm, 2) + rm_count(rm, 3) + rm_count(rm, 0) + 4 + expand_cit_need(pl, idx);   // expand.c EXTRA
    int lim = cap - (int)((float)cap * g_cit_army), hl = home + rm_count(rm, 2) / 2 + 4;
    if (hl > cap * 4 / 5) hl = cap * 4 / 5;
    if (lim < hl) lim = hl;
    if (goal > lim) goal = lim;
    if (goal < game) goal = game;
    g_czp[idx].goal = plan & 1 ? goal : 0;
    g_czp[idx].home = home; g_czp[idx].slots = slots;
    // floors: food keeps FEED's gatherers and its fields + 2 (farmers), wood its late target (late_targets bit 1),
    // a metal its gatherers (nobody is pulled off a mine for food)
    void *fq = type_list(um, UT(B_FARM));
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
internal int __fastcall hook_cit_wanted(ee_unit_mgr *um, void *edx) {
    (void)edx;
    int n = um_citizens_wanted(um);
    ee_player *pl = FIELD(um, OF(UM, 0x134), void *);
    if (!pl || is_vanilla(pl)) return n;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    int fresh = game_time() >= g_czp[idx].t && game_time() - g_czp[idx].t <= 15000;
    int r = (CIT_PLAN(idx) & 1) && fresh && g_czp[idx].goal > n ? g_czp[idx].goal : n;
    return wf_cap_wanted(idx, r);   // workforce_plan CAP: no more than the economy uses
}
// FIRST (train_budget, a citizen's food): the stock without the epoch reserve while the income pays both
internal int cz_visible_food(ee_player *pl, unsigned idx, const int *stock, int visible) {
    if (!(CIT_PLAN(idx) & 8) || current_epoch(pl) < 1) return visible;
    int goal = g_czp[idx].goal ? g_czp[idx].goal : g_czp[idx].game;
    if (goal && workforce(pl) >= goal) return visible;
    float spend = (float)cz_producers(pl) * (float)unit_cost(pl, UT(CITIZEN), 2) * 60000.0f / (float)g_cit_train_ms;
    if (g_rate[idx].rate[2] < 2.0f * spend) return visible;
    return stock[2] > visible ? stock[2] : visible;
}
// ---- workforce_plan ([all], bits; [autotest] v_workforce_plan for the variant players; docs/economy.md "Workforce
// into army") ----
// The playtester: citizens are needed to fill every mine, a lack of them should not be the limit - nor should an
// oversized, wood-heavy workforce starve the army; workers on wood beyond what is spent are waste.
// Measured first (the shipped build against the base AI at hard, unit limit 1500 = a cap of 505 each, 75 min, Small
// Islands Large / Tournament Islands Huge / Large Islands Huge, 40 games a cell; scripts/workforce.py): at minute 60
// the mod's AI had 170-227 citizens (the base AI 122), 31-63 of them idle (3-14), 68-107 on wood (61-68) with
// 8400-14900 wood in stock (3400-7600) - 22000-29000 wood came in over minutes 30-60, 14000-17000 were spent - and
// 35-62 soldiers (70-81); it had trained 116-148 soldiers by then (238-279), with 3.5-4.5 land production buildings
// (10). Why so many citizens: citizen_plan's goal counts the citizens there - the woodcutters and the idle - so
// whoever is trained and finds no mine raises it, up to the cap less citizen_army_share (303 of 505); the game's
// own 45% of the cap (227) is a floor on top. Why so many on wood: LATE WOOD's target closes the game's wood
// "demand" (refused requests stay on its books, ~80000) - the 2000 it counts at most ask for 40% of the workers. The
// island's mines near drop sites were full (the slots elsewhere are other islands', out of a walking citizen's reach),
// its forest cut back: the extra citizens stood idle or cut wood nobody spent. Why so few soldiers: every soldier
// costs gold or iron (Middle Ages: Long Sword / Pikeman 30 iron, Long Bow 30 gold, Knight 60 gold, the warships
// 125-250) - the gold and iron stock stayed at ~200 the whole game, the army's orders refused 10000-12000 times
// from minute 20 for gold and iron while food and wood piled up.
// 1 CAP: from the Copper Age the citizens wanted (the game's share of the cap and citizen_plan's goal alike) are at
//   most what the economy uses: the mines' slots near drop sites (or the miners, if more), the food gatherers (at
//   least the fields + 2 and FEED's), the wood workers the wood actually spent needs (wood spent over the last 5
//   minutes - income less the stock's growth - at the measured rate per worker, + 25% and 3), the outpost's
//   (expand.c EXTRA), 4 builders and a margin of cap / 40 (at least 4); never the idle or woodcutters beyond the
//   need. After 5 minutes of samples, never under 30.
// 2 WOOD: while the wood stock holds 3 minutes of its spending + 1000, the wood target is at most that need: the rest
//   goes to the metals' targets up to their slots near drop sites, then to food up to its fields + 2
//   (econ_rebalance moves the gatherers); what has nowhere to go stays on wood.
// 4 ARMY: army_plan's FILL and BUILD (army.c) on every map, the islands too, from a population cap of
//   workforce_army_cap (250): soldiers into the room the capped goal leaves while the account and the stock pay, one
//   more land production building when they train 70%+ of the time (up to 2 + workers / 15).
// 8 THRIFT: from the Copper Age, while food or wood lie at 800+, a soldier refused for gold or iron is replaced at once
//   (no saving first, econ.c substitute_wait) by the soldier with the most score (hit points x attack) per gold +
//   iron that the budget pays: Long Bows (30 gold, 40 wood) and Pikemen (30 iron, 30 food) for the Knights (60 gold)
//   the game asks for - the metal is the army's limit, the food and wood are there.
internal int g_wf_plan = 0, g_v_wf_plan = -1, g_wf_army_cap = 250;
#define WF_PLAN(idx) PV(idx, g_v_wf_plan, g_wf_plan)
#define WF_N 31   // 10-s samples: 5 minutes
internal struct wf_st {
    uint32_t t, tc;                   // last sample, CAP's last pass
    int inc[WF_N], stk[WF_N], k, n;   // wood income so far (g_bud) and the wood stock, oldest at k once full
    int spent, need;                  // wood spent a minute (5-minute mean), the wood workers it needs
    int cap, metal, food;             // CAP's goal and its parts
    int asked, given;                 // the last citizens wanted: asked (the game's / citizen_plan's), given
    int rs0;                          // # wf: the techs' gold + iron at the first line (the start epoch's)
    unsigned held, moved, thrift;     // this minute: wanted calls held back, wood targets moved, THRIFT's soldiers
} g_wf[16];
internal void wf_new_game(void) { memset(g_wf, 0, sizeof g_wf); }

// every 10 s (the mod's AIs): the wood spent over the last 5 minutes and the woodcutters it needs
internal void wf_tick(ee_player *pl, uint32_t now) {
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    struct wf_st *w = &g_wf[idx];
    if (now < w->t) memset(w, 0, sizeof *w);
    if (!WF_PLAN(idx) && !g_autotest) return;
    if (w->t && now - w->t < 10000) return;
    w->t = now ? now : 1;
    w->inc[w->k] = g_bud[idx].income[3];
    w->stk[w->k] = FIELD(pl, PL_STOCK + 12, int);
    w->k = (w->k + 1) % WF_N;
    if (w->n < WF_N) w->n++;
    if (w->n < WF_N) return;
    int nw = (w->k + WF_N - 1) % WF_N, sp = (w->inc[nw] - w->inc[w->k]) - (w->stk[nw] - w->stk[w->k]);
    w->spent = sp > 0 ? sp / 5 : 0;
    float pw = g_rate[idx].per_worker[3];
    if (pw < 5.0f) pw = 5.0f;
    w->need = (int)((float)w->spent * 1.25f / pw + 0.99f) + 3;
}
// after the worker targets (hook_compute_targets, after cz_targets): CAP's goal, WOOD's targets
internal void wf_targets(ee_resource_mgr *rm) {
    ee_player *pl = rm_player(rm);
    if (!pl) return;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    struct wf_st *w = &g_wf[idx];
    int plan = WF_PLAN(idx);
    w->cap = 0;
    if (!(plan & 3) || current_epoch(pl) < FARM_EPOCH || w->n < WF_N) return;
    uint8_t *cm = FIELD(pl, OF(PL, 0xB90), uint8_t *);
    ee_unit_mgr *um = cm ? FIELD(cm, OF(CM, 0x44), ee_unit_mgr *) : NULL;
    if (!um) return;
    uint32_t now = game_time();
    uint32_t *target = (uint32_t *)FIELD_AT(rm, RM_TARGET);
    struct mf_st *m = &g_mf[idx];
    mf_scan(pl, idx, now);
    void *fq = type_list(um, UT(B_FARM));
    int fields = fq ? FIELD(fq, 0x10, int) : 0, metal = 0, food = rm_count(rm, 2);
    for (int r = 4; r < 7; r++) metal += m->slots[r] > rm_count(rm, r) ? m->slots[r] : rm_count(rm, r);
    if (food < fields + 2) food = fields + 2;
    if (food < g_czp[idx].feed) food = g_czp[idx].feed;
    if (plan & 1) {
        int cap = unit_limit(pl), margin = cap / 40 > 4 ? cap / 40 : 4;
        int c = metal + food + w->need + 4 + margin + expand_cit_need(pl, idx);
        w->cap = c > 30 ? c : 30;
        w->metal = metal; w->food = food; w->tc = now ? now : 1;
        if (g_czp[idx].goal > w->cap) g_czp[idx].goal = w->cap;   // army_plan FILL's room counts the capped goal
    }
    if ((plan & 2) && FIELD(pl, PL_STOCK + 12, int) >= 3 * w->spent + 1000 && (int)target[3] > w->need) {
        int extra = (int)target[3] - w->need;
        for (int r = 4; r < 7; r++)
            for (; extra > 0 && (int)target[r] < m->slots[r]; extra--) { target[r]++; target[3]--; w->moved++; }
        for (; extra > 0 && (int)target[2] < fields + 2; extra--) { target[2]++; target[3]--; w->moved++; }
    }
}
// the citizens wanted at the request (hook_cit_wanted): CAP's goal at most
internal int wf_cap_wanted(unsigned idx, int n) {
    struct wf_st *w = &g_wf[idx & 15];
    uint32_t now = game_time();
    w->asked = n;
    if ((WF_PLAN(idx) & 1) && w->cap && now >= w->tc && now - w->tc <= 60000 && n > w->cap) { w->held++; n = w->cap; }
    w->given = n;
    return n;
}
// ARMY: army_plan's FILL + BUILD on every map at a high population cap (army.c army_plan_tick)
internal int wf_army(ee_player *pl, unsigned idx) {
    if (!(WF_PLAN(idx) & 4) || current_epoch(pl) < FARM_EPOCH) return 0;
    int cap = unit_limit(pl);
    return cap >= g_wf_army_cap ? 6 : 0;
}
// THRIFT (econ.c train_budget, a refused soldier): food or wood pile and the soldier lacks gold or iron
internal int wf_thrift(ee_player *pl, unsigned idx, int type, const int *vis) {
    if (!(WF_PLAN(idx) & 8) || current_epoch(pl) < FARM_EPOCH) return 0;
    if (FIELD(pl, PL_STOCK + 8, int) < 800 && FIELD(pl, PL_STOCK + 12, int) < 800) return 0;
    for (int r = 5; r < 7; r++) if (vis[r] < unit_cost(pl, type, r)) return 1;
    return 0;
}
internal void wf_thrift_note(unsigned idx, int type, int n) { (void)type; g_wf[idx & 15].thrift += (unsigned)(n > 0 ? n : 0); }
// "# wf:" once a minute (autotest, both sides): the wood spent a minute and the woodcutters it needs, wood workers,
// target, stock; CAP's goal and parts, the citizens wanted asked / given, calls held, wood targets moved; the idle; the
// gold + iron paid for the techs researched so far (their prices, all techs with the researched flag)
internal void wf_log(ee_player *pl, unsigned idx, unsigned min) {
    struct wf_st *w = &g_wf[idx & 15];
    uint8_t *cm = FIELD(pl, OF(PL, 0xB90), uint8_t *);
    ee_tech_tree *tt = FIELD(pl, PL_TECHTREE, ee_tech_tree *);
    ee_resource_mgr *rm = cm ? FIELD(cm, OF(CM, 0x38), ee_resource_mgr *) : NULL;
    if (!rm) return;
    int rs = 0;
    for (int id = 1000; tt && id < 15000; id++) {
        ee_tech *t = tech_lookup(tt, id);
        uint8_t *d = t ? FIELD(t, OF(TECH, 0x8), uint8_t *) : NULL;
        if (d && FIELD(t, OF(TECH, 0x4), uint8_t)) rs += FIELD(d, OF(TECHDATA, 0x30), int) + FIELD(d, OF(TECHDATA, 0x34), int);
    }
    if (!w->rs0) w->rs0 = rs ? rs : 1;
    rs -= w->rs0;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# wf: P%u at %u min: spent %d need %d have %d target %d stock %d | cap %d metal %d food %d asked %d given %d held %u "
            "moved %u thrift %u | idle %d | rsch %d\n", idx, min, w->spent, w->need, rm_count(rm, 3), FIELD(rm, RM_TARGET + 12, int),
            FIELD(pl, PL_STOCK + 12, int), w->cap, w->metal, w->food, w->asked, w->given, w->held, w->moved, w->thrift, rm_count(rm, 0), rs);
    LeaveCriticalSection(&g_lock);
    w->held = w->moved = w->thrift = 0;
}

internal void cz_install(const char *aimod_ini, const char *autotest_ini) {
    g_cit_plan = cfg_int(aimod_ini, autotest_ini, 1, "citizen_plan", 3);
    g_cit_army = cfg_share(aimod_ini, autotest_ini, 1, "citizen_army_share", 0.40, 0, 0.9);
    g_cit_train_ms = cfg_int(aimod_ini, autotest_ini, 1, "citizen_train_ms", 28000);
    if (g_cit_train_ms < 5000) g_cit_train_ms = 5000;
    if (autotest_ini) g_v_cit_plan = ini_int("autotest", "v_citizen_plan", -1, autotest_ini);
    g_wf_plan = cfg_int(aimod_ini, autotest_ini, 1, "workforce_plan", 0);
    g_wf_army_cap = cfg_int(aimod_ini, autotest_ini, 1, "workforce_army_cap", 250);
    if (autotest_ini) g_v_wf_plan = ini_int("autotest", "v_workforce_plan", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# workforce plan: %d (variant %d), army from a cap of %d\n", g_wf_plan, g_v_wf_plan, g_wf_army_cap);
    BOOL ok = patch_citizens_wanted(hook_cit_wanted);
    if (g_log) fprintf(g_log, "# citizens: citizen_plan %d (variant %d), army share %.0f%%, hook %s\n", g_cit_plan, g_v_cit_plan,
                       g_cit_army * 100.0f, ok ? "ok" : "FAILED");
}
