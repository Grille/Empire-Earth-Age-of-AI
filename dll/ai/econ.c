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

// per-difficulty switches and military shares: settings.c (cfg_of)
internal int g_citizen_slack = 9999;   // idle-citizen check: off by default (stalled growth in tests)
internal int g_hoard_cap = 3000;   // stock above reserve + this is free for military too, aimod.ini [budget] hoard_cap
// stone, gold, iron: hardly any civilian use besides the epoch cost (reserved anyway) and a few techs - with the
// 3000 cap gold piled up (1000 in stock) while every soldier failed for gold; aimod.ini hoard_cap_mineral. 200 (was
// 600): ~600 iron lay in stock for the whole late game while the army failed for iron hundreds of times every 5
// minutes (antiair save: Brann's iron failures 816 -> 64; late-start A/B, 3 maps x 6 seeds: alive 78% vs 75%)
internal int g_hoard_cap_mineral = 200;
// HEAD-TO-HEAD TESTS (autotest only): [autotest] variant_players = the player numbers playing variant B (e.g. 24);
// they use v_late_targets, v_hoard_cap_mineral, v_substitute_wait where set (the others the settings). AI-vs-AI
// suites with one variant for all measure how decisive the wars are, not which variant is stronger.
internal unsigned g_var_mask;
internal int g_v_late = -1, g_v_hoard_mineral = -1, g_v_sub_wait = -1;
#define PV(idx, v, g) ((g_var_mask >> ((idx) & 15) & 1) && (v) >= 0 ? (v) : (g))
internal struct { uint32_t last_t; int init; float mil[7]; int income[7], mil_spent[7], cit_blocked, mil_limited; } g_bud[16];
// EARLY GAME (dll/ai/early.c, docs/economy.md - the playtester's opening): [all] early_towers, tower_bait, early_militia,
// early_army, early_soldiers; copper_plan=2. Head-to-head: v_early_towers ... v_copper_plan for the variant players.
internal int g_early_towers = 1, g_tower_bait = 1, g_early_militia = 1, g_early_army = 0, g_early_soldiers = 0;
internal int g_v_early_towers = -1, g_v_tower_bait = -1, g_v_early_militia = -1, g_v_early_army = -1, g_v_early_soldiers = -1;
internal int g_v_copper = -1;
internal float g_early_food_share = 0.10f;   // [all] early_army_food_share: of the food income for the army before farms
internal int g_min_workforce = 40;          // [all] min_workforce (COPPER PLAN below)
internal int g_food_floor = 18;             // [all] food_floor: copper_plan=2, food workers wanted before farms (half the workers up to this)
internal int g_cit_queue = 0;                // [all] citizen_queue: copper_plan=2, citizens paid ahead while saving (0 = any)
internal float g_epoch_share = 0.40f;        // [all] epoch_share: copper_plan=2, of the income into the epoch account
internal float g_epoch_acc[16][7];           // copper_plan=2: the next epoch's money, out of the citizens' reach
internal int g_epoch_acc_ep[16];             // the epoch it was saved in (+1; 0 = none yet)
internal int g_early_workers = 60;           // [all] early_army_workers: below, the army still spares food (substitution)
#define EARLY(idx, which) PV(idx, g_v_##which, g_##which)
internal int copper_mode(ee_player *pl);          // below: copper_plan (per difficulty) or the variant's v_copper_plan
internal void early_tick(ee_player *pl, unsigned idx, uint32_t now);   // early.c
internal int early_tower_stone(ee_player *pl);
internal void food_plan_needs(ee_player *pl, ee_resource_mgr *rm, int *need, const int *stock, const int *reserve);   // food.c
internal void food_log(ee_player *pl, unsigned idx, uint32_t now);
internal int food_fields_wait(ee_player *pl, ee_resource_mgr *rm);
internal void mine_drops(ee_player *pl, unsigned idx, uint32_t now);   // mines.c
internal void mines_log(ee_player *pl, unsigned idx, uint32_t now);
internal int g_mine_fill, g_v_mine_fill;
internal int g_cit_plan, g_v_cit_plan;   // citizens.c: citizen_plan
internal int cz_visible_food(ee_player *pl, unsigned idx, const int *stock, int visible);
internal void cz_budget_note(ee_player *pl, unsigned idx, const int *req, const int *stock, const int *visibles, int blocked, int afford);   // citizens.c
internal int early_cover(ee_player *pl, int x, int y);
internal int early_core_spot(ee_player *pl, int type, int *ox, int *oy);
internal unsigned g_early_mil_refused[16];   // defence.c militia_answer: citizens kept out of a fight (early_militia)
// income rates per AI (rates_update): per minute and per worker-minute
internal struct { uint32_t t; int last[7]; float rate[7], per_worker[7]; } g_rate[16];
internal const float k_default_pw[7] = { 0, 0, 10, 10, 5, 8, 6 };
internal void rates_update(ee_player *pl, unsigned idx, uint32_t now);
internal int g_reserve_plan = 1;   // aimod.ini [all] reserve_plan (see spend_reserve)
internal int g_mineral_margin = 300;   // aimod.ini [all] mineral_worker_margin (econ_targets; -1 = off)

// Spending by caller (diagnostic): what EEComputerPlayer::vf45 (pay) took, summed per player, resource and return
// address, with the income so far; logged as "# spend:" every 5 minutes.
internal struct { uintptr_t site; int type; unsigned sum, n; } g_spend[16][24];
internal void spend_note(unsigned idx, int type, int amount, uintptr_t site) {
    int k = 0;
    while (k < 24 && g_spend[idx][k].n && (g_spend[idx][k].site != site || g_spend[idx][k].type != type)) k++;
    if (k == 24) return;
    g_spend[idx][k].site = site; g_spend[idx][k].type = type;
    g_spend[idx][k].sum += (unsigned)amount; g_spend[idx][k].n++;
}
internal void spend_log(unsigned idx, uint32_t now) {
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
// RETRY (with build_fix): a dropped entry was gone for good - the planner never queued it again. Both AIs failed
// their AA gun (145) in the first minutes (refused with the resources there, 225 wood and 200 gold at minute 0.4)
// and had none for the rest of the game, against a player with aircraft. A dropped type (not the 395 settlement, not
// a decoration) is tried again every 2 minutes with the build manager's own placement (0x46580A, spot 0,0: the
// game's search) while the AI has none of it and can pay for it; given up after 5 failed retries.
internal int defence_spot(ee_player *pl, int type, int *ox, int *oy);   // defence.c
internal int can_place(ee_player *pl, int type, int x, int y);   // defence.c: the game's placement test, every tile
internal void defence_tick(ee_player *pl, unsigned idx, uint32_t now);
internal void anti_air(ee_player *pl, unsigned idx, uint32_t now);   // defence.c
internal void bunker_reserve(ee_player *pl, int *reserve);   // bunker.c
internal void hold_town(ee_player *pl, unsigned idx, uint32_t now);   // defence.c
internal void attack_retreat(ee_player *pl, unsigned idx, uint32_t now);   // defence.c
internal void counter_siege(ee_player *pl, unsigned idx, uint32_t now);
internal void home_guard(ee_player *pl, unsigned idx, uint32_t now);
internal void muster(ee_player *pl, unsigned idx, uint32_t now);       // defence.c: gather, then strike
internal void diplo_tick(ee_player *pl, unsigned ai, uint32_t now);
internal int g_build_fix = 1;
internal struct { int type; unsigned tries, fails, dropped, streak; } g_place[16][64];   // every type the build lists hold
internal int field_place_ok(ee_build_mgr *bm, int x, int y, int lay_out);   // below, with the farm fields
internal int upgraded_type(ee_player *pl, int type);   // defence.c: the type an upgrade put in place of this one
internal struct { int type; uint32_t next; uint8_t tries; } g_retry[16][16];   // dropped types, tried again
internal int is_decoration(int type) {   // the editor's "x ..." objects (name at object data +0x1C)
    ee_unit_type *od = type_data(type);
    const char *name = od ? type_get_name(od) : NULL;
    return name && name[0] == 'x' && name[1] == ' ';
}
internal void retry_note(unsigned idx, int type, uint32_t now) {
    if (type == UT(X_SINGLE_PLAYER_SETTLEMENT) || type == UT(B_FARM) || is_decoration(type)) return;
    int f = -1;
    for (int k = 0; k < 16; k++) {
        if (g_retry[idx][k].type == type) return;
        if (!g_retry[idx][k].type && f < 0) f = k;
    }
    if (f < 0) return;
    g_retry[idx][f].type = type;
    g_retry[idx][f].next = now + 120000;
    g_retry[idx][f].tries = 0;
}
internal char __fastcall hook_place(ee_build_mgr *bm, void *edx, int type, int x, int y) {
    (void)edx;
    ee_player *owner = bm_get_player(bm);
    // a field without foundation would crash (see there) - the guard alone for a vanilla player (nothing laid out)
    if (type == UT(B_FARM) && !field_place_ok(bm, x, y, !is_vanilla(owner))) return 1;
    if (is_vanilla(owner)) return bm_place(bm, type, x, y);
    int place_as = g_build_fix && type == UT(X_SINGLE_PLAYER_SETTLEMENT) && ut_has(UT(B_SETTLEMENT)) ? UT(B_SETTLEMENT) : type;
    place_as = upgraded_type(((void *)bm_get_player(bm)), place_as);   // queued before its upgrade: the new type
    // A tower queued with its spot (defence.c defence_tick) is laid there unchecked: with a position the placement
    // (0x46580A) only looks for a builder and makes the foundation (0x62EF8A). Another building put on that spot in
    // the meantime blocks it - the build manager's round (0x465C9F) finds the builder waiting at a foundation with
    // another unit in its footprint (0x60CDCF), deletes the foundation, refunds it and queues it again at the same
    // spot (0x468BA7): in a Small Islands Large game 127 times in 13 minutes, 175 stone each. Such a spot is given up:
    // the defence plan's next slot (defence_spot, below), else the game's own search.
    if ((x || y) && is_tower_type(type) && !can_place(owner, place_as, x, y)) {
        if (g_log) fprintf(g_log, "# place: P%d at %.1f min: tower %d at %d,%d no longer fits - another spot\n",
                           player_get_index(owner) & 15, game_time() / 60000.0, place_as, x, y);
        x = y = 0;
    }
    if (!x && !y) defence_spot(((void *)bm_get_player(bm)), type, &x, &y);   // towers: where the defence plan wants them
    char ok = bm_place(bm, place_as, x, y);
    ee_player *pl = bm_get_player(bm);
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
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
        if (type != UT(X_SINGLE_PLAYER_SETTLEMENT)) retry_note(idx, place_as, game_time());   // the single player settlement arrives as a settlement: not retried
        return 1;
    }
    return ok;
}
internal void place_log(unsigned idx, uint32_t now) {
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

// GATHERING RESEARCH (aimod.ini [all] gather_research, default on): the research manager (cm+0x40) researches what is
// scheduled in its per-epoch lists (+0x18 + e*0x10); new techs come from a pool (+8) through a random planner
// (0x4858D9) that prefers gathering upgrades - but only while its flag +0x11A is set, and the AI's "attention" pass
// (0x42F912) switches on just player+0xE20 + 1 activities per period, research rarely among them. In the Bronze
// Age the AIs still had Hafts, Hand Axe, Metal Working and Hardened Chisel (75-150 food, 50-100 wood, +15% each)
// unresearched. Every 15 s one available (tech +6), unresearched (+4) pool tech with a gathering effect is scheduled
// with the planner's 0x485F57 (requests the building if missing, adds the cost to the demand) and taken out of the
// pool. Gathering effect: type 2 (+0x20), attribute +0x2C 0x1A..0x21 (wood, gold, stone, iron, ...), applied to
// citizens (+0x14 = 32/480) or to all (-1) - the planner's own test misses the -1 ones, which most of these are.
internal int workforce(ee_player *pl);
internal int g_gather_research = 1;
internal unsigned g_researched[16];
internal int is_gather_tech(ee_tech *tech) {
    uint8_t *d = FIELD(tech, OF(TECH, 0x8), uint8_t *);
    int ei = d ? FIELD(d, OF(TECHDATA, 0x50), int) : -1;
    if (ei < 0 || ei > 0x5DC) return 0;
    uint8_t *list = epoch_tech_list(ei);
    unsigned n = FIELD(list, OF(EPOCHTECHS, 0x8), unsigned), *ids = FIELD(list, OF(EPOCHTECHS, 0x4), unsigned *);
    for (unsigned k = 0; ids && k < n && k < 64; k++) {
        if (ids[k] >= 2000) continue;
        uint8_t *ef = tech_effect(ids[k]);
        int a = FIELD(ef, OF(EFFECT, 0x2C), int), on = FIELD(ef, OF(EFFECT, 0x14), int);
        if (FIELD(ef, OF(EFFECT, 0x20), int) == 2 && (on == UT(CITIZEN) || on == UT(FEMALE_CITIZEN) || on == -1) && a >= 0x1A && a <= 0x21 && a != 0x1E) return 1;
    }
    return 0;
}
internal void gather_research(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_gather_research || (now >= last[idx] && now - last[idx] < 15000)) return;
    last[idx] = now;
    void *cm = player_get_civ_mgr(pl);
    ee_research_mgr *rs = cm ? cm_get_research_mgr(cm) : NULL;
    ee_tech_tree *tt = player_get_tech_tree(pl);
    if (!rs || !tt || workforce(pl) < 10) return;
    int *b = rs_get_pool_begin(rs), *e = rs_get_pool_end(rs);
    for (int *q = b; b && q < e && e - b < 500; q++) {
        ee_tech *t = tech_lookup(tt, *q);
        if (!t || FIELD(t, OF(TECH, 0x4), uint8_t) || !FIELD(t, OF(TECH, 0x6), uint8_t) || !is_gather_tech(t)) continue;
        int id = *q;
        memmove(q, q + 1, (size_t)(e - q - 1) * sizeof *q);   // out of the pool, as the planner does
        rs_set_pool_end(rs, e - 1);
        rs_schedule(rs, id);
        g_researched[idx]++;
        if (g_log) fprintf(g_log, "# research: P%u schedules gathering tech %d at %.1f min\n", idx, id, now / 60000.0);
        return;
    }
}

// PAID UPGRADES (aimod.ini [all] paid_upgrades, default on). When a computer player reaches an epoch,
// EEComputerPlayer::vf41 (0x43DCBF -> 0x43D330) calls 0x5BDDBD(7) on the epoch's tech node: every child tech of
// category 7 (data +0x48) is researched on the spot through EETechTreeNode::vf6 (0x5BF335) - no cost, no building,
// no time. Those are the unit and building line upgrades (effect kind 0x13: Copper Age e.g. 3080 = 40 food, 40 iron:
// type 20 -> 21; 3051 = 50 stone: watch tower 47 -> 461). A human player researches each at its building and pays;
// EEPlayer::vf41 grants nothing. Here the call (0x43D365) schedules each one with the research manager instead
// (0x485F57, as gather_research does): researched at its building and paid like a player's.
// Art of Conquest rewrote 0x5BDDBD (0x5CE8C5): thiscall(node, category, standard only) - the computer player passes 1:
// only a tech with data +0x81 set is granted (the line upgrades; clear on the civ power and special unit upgrades) -
// and its children in a map (game/tech.c); its vf6 takes an argument (0).
internal int g_paid_upgrades = 1;
internal int ap_upgrade_later(ee_player *pl, uint8_t *data);   // army.c
internal void epoch_upgrades(ee_tech *node, int cat, int only_new);
internal void __fastcall hook_epoch_upgrades(ee_tech *node, void *edx, int cat) {   // the 2017 update's
    (void)edx;
    epoch_upgrades(node, cat, 0);
}
internal void __fastcall hook_epoch_upgrades_aoc(ee_tech *node, void *edx, int cat, int only_new) {
    (void)edx;
    epoch_upgrades(node, cat, only_new & 0xFF);
}
internal void epoch_upgrades(ee_tech *node, int cat, int only_new) {
    ee_tech_tree *tt = FIELD(node, OF(TECHNODE, 0xC), ee_tech_tree *);
    int pi = tt ? FIELD(tt, OF(TT, 0x4), int) : -1;
    // the game's own player array, not g_players: back in the scenario editor after a test run the editor sets the
    // computer players' epochs again (here) while g_players still held the test run's deleted players - the game
    // crashed reading one (eeaimod.dll+5e188)
    unsigned np = game_player_count();
    ee_player *pl = pi > 0 && pi < 16 && (unsigned)pi < np ? game_players()[pi] : NULL;
    if (pl && (player_get_index(pl) & 15) != pi) pl = NULL;
    void *cm = pl ? player_get_civ_mgr(pl) : NULL;
    ee_research_mgr *rs = cm ? cm_get_research_mgr(cm) : NULL;
    if (!g_paid_upgrades || !rs || cat != 7 || is_vanilla(pl)) {
        epoch_upgrades_grant(node, cat, only_new);
        return;
    }
    char ids[256] = "";
    int n = 0;
    ee_tech *c;
    for (struct tech_kids k = tech_kids(node); (c = tech_kid_next(&k)); ) {   // as the original walks them
        uint8_t *d = FIELD(c, OF(TECHCHILD, 0x8), uint8_t *);
        if (!d || FIELD(d, OF(TECHDATA, 0x48), int) != 7 || FIELD(c, OF(TECH, 0x4), uint8_t) || (only_new && !d[OF(TECHDATA, 0x81)])) continue;
        int id = FIELD(d, OF(TECHDATA, 0x4C), int);
        if (ap_upgrade_later(pl, d)) continue;   // army.c army_plan UPGRADE: a line it doesn't use yet
        rs_schedule(rs, id);
        size_t l = strlen(ids);
        if (l < sizeof ids - 8) snprintf(ids + l, sizeof ids - l, " %d", id);
        n++;
    }
    if (g_log && n) fprintf(g_log, "# upgrades: P%d at %.1f min: %d epoch upgrades scheduled for research (not free):%s\n",
                            pi, game_time() / 60000.0, n, ids);
}

// Attention (diagnostic): 0x42F912 switches on only player+0xE20 + 1 of these activities per period (strategy
// +0xA28 orders them): 0 cm+0x7A, 1 rm+0x188, 2 rs+0x11A (random research), 3 um+0x140, 4 cm+0x7B, 5 cm+0x7C,
// 6 [b8c+0x4C]+0x48, 7 b98+0x2D, 8 b8c+0x62. Share of AI ticks each was on, logged as "# attention:" every 10 min.
internal struct { unsigned ticks, on[9]; } g_att[16];
internal void attention_note(ee_player *pl, unsigned idx, uint32_t now) {
    ee_civ_mgr *cm = player_get_civ_mgr(pl); ee_combat_mgr *b8c = player_get_combat_mgr(pl); uint8_t *b98 = player_get_b98(pl);
    if (!cm || !b8c || !b98) return;
    ee_resource_mgr *rm = cm_get_resource_mgr(cm);
    ee_research_mgr *rs = cm_get_research_mgr(cm); ee_unit_mgr *um = cm_get_unit_mgr(cm);
    uint8_t f[9] = { cm_get_att_epoch_techs(cm), rm ? rm_get_att_188(rm) : 0, rs ? rs_get_att_random(rs) : 0, um ? um_get_att_140(um) : 0,
                     cm_get_att_build(cm), cm_get_att_7c(cm), am_get_att_4c(b8c), b98[OF(PLB98, 0x2D)], am_get_att_62(b8c) };
    g_att[idx].ticks++;
    for (int i = 0; i < 9; i++) g_att[idx].on[i] += f[i] != 0;
    static uint32_t last[16];
    if (!g_log || now / 600000 == last[idx] / 600000) return;
    last[idx] = now;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# attention: P%u at %u min, E20=%d A28=%d, on %%:", idx, now / 60000, player_get_attention(pl), player_get_strategy(pl));
    for (int i = 0; i < 9; i++) fprintf(g_log, " %d:%u", i, g_att[idx].ticks ? g_att[idx].on[i] * 100 / g_att[idx].ticks : 0);
    fputc('\n', g_log);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
    memset(&g_att[idx], 0, sizeof g_att[idx]);
}

// Free research (diagnostic, with tech_log): EETechTreeNode::vf6 0x5BF335 (vtable slot 0x830874) marks a tech
// researched and applies it without any payment; logged with its caller.
internal int __fastcall hook_tech_grant(ee_tech *node, void *edx) {
    (void)edx;
    uint8_t before = FIELD(node, OF(TECH, 0x4), uint8_t);
    int r = tech_grant(node);
    if (g_log && !before && FIELD(node, OF(TECH, 0x4), uint8_t)) {
        uint8_t *d = FIELD(node, OF(TECHNODE, 0x8), uint8_t *); ee_player *owner = FIELD(node, OF(TECHNODE, 0xC), ee_player *);
        uintptr_t *fp = (uintptr_t *)__builtin_frame_address(0);
        fprintf(g_log, "# techgrant: tech %d player %d at %.2f min, caller %08x, stack",
                d ? FIELD(d, OF(TECHDATA, 0x4C), int) : -1, owner ? player_get_unknown_4(owner) : -1, game_time() / 60000.0,
                (unsigned)(uintptr_t)__builtin_return_address(0));
        for (int k = 0; k < 64; k++) {   // return addresses into the exe on the stack
            uintptr_t v = fp[k];
            if (v >= g_code_lo && v < g_code_hi) fprintf(g_log, " %06x", (unsigned)v);
        }
        fputc(10, g_log);
    }
    return r;
}

// Tech log (autotest tech_log=1): once a second per AI, techs newly researched, with the stock before (last check)
// and now - whether research is paid shows in the stock.
internal int g_tech_log;
internal void tech_log_tick(ee_player *pl, unsigned idx, uint32_t now) {
    static uint8_t seen[16][16384];
    static int last_stock[16][7];
    static uint32_t last[16];
    if (!g_tech_log || !g_log || (now >= last[idx] && now - last[idx] < 1000)) return;
    last[idx] = now;
    ee_tech_tree *tt = player_get_tech_tree(pl);
    int *stock = player_stock_at(pl);
    if (!tt) return;
    char buf[1024];
    int o = 0, n = 0, cost[7] = { 0 };
    for (int id = 0; id < 16384; id++) {
        ee_tech *t = tech_lookup(tt, id);
        int done = t && FIELD(t, OF(TECH, 0x4), uint8_t);
        if (done && !seen[idx][id]) {
            uint8_t *d = FIELD(t, OF(TECH, 0x8), uint8_t *);
            for (int r = 2; r < 7 && d; r++) cost[r] += FIELD(d, OF(TECHDATA, 0x24) + (r - 2) * 4, int);
            if (o < 900) o += snprintf(buf + o, sizeof buf - o, " %d", id);
            n++;
        }
        seen[idx][id] = (uint8_t)done;
    }
    if (n && now > 2000)
        fprintf(g_log, "# techlog: P%u at %.2f min epoch %d: %d new:%s | cost %d/%d/%d/%d/%d | stock %d/%d/%d/%d/%d -> %d/%d/%d/%d/%d\n",
                idx, now / 60000.0, FIELD(tt, TT_EPOCH, int), n, buf, cost[2], cost[3], cost[4], cost[5], cost[6],
                last_stock[idx][2], last_stock[idx][3], last_stock[idx][4], last_stock[idx][5], last_stock[idx][6],
                stock[2], stock[3], stock[4], stock[5], stock[6]);
    memcpy(last_stock[idx], stock, sizeof last_stock[idx]);
}

// Gathering (diagnostic), every 5 minutes: per resource the workers, the mean distance from their gather site to the
// nearest drop site, and the income per worker-minute since the last line.
internal int g_gather_income[16][7];   // the income at the last line (econ_new_game: income counts start over)
internal void gather_log(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    int (*last_income)[7] = g_gather_income;
    if (!g_log || now / 300000 == last[idx] / 300000) return;
    last[idx] = now;
    void *cm = player_get_civ_mgr(pl);
    ee_resource_mgr *rm = cm ? cm_get_resource_mgr(cm) : NULL;
    if (!rm) return;
    static int d[64][3];
    int nd = drop_sites(pl, d, 64), ndrop = 0;
    for (int i = 0; i < nd; i++) ndrop += !d[i][2];
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# gather: P%u at %u min, drop sites %d (+%d granaries):", idx, now / 60000, ndrop, nd - ndrop);
    for (int r = 2; r < 7; r++) {
        uint8_t *dq = rm_list_at(rm, r);
        unsigned first = FIELD(dq, 0xC, unsigned), n = FIELD(dq, 0x10, unsigned), m = 0;
        int sum = 0;
        for (unsigned k = 0; k < n; k++) {
            ee_unit *u = deque_at(dq, first + k); ee_goal *goal = u ? unit_get_goal(u) : NULL;
            ee_unit *site = goal && !strcmp(rtti_name(goal), "EEUGGather@@") ? goal_get_site(goal) : NULL;
            if (!site) continue;
            sum += drop_dist(d, nd, unit_get_x(site), unit_get_y(site), r == 2);
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

internal void mt_paid(ee_player *pl, uint8_t *what, const int *before, const int *stock);   // metal.c: what the gold and iron buy
internal void mt_income(ee_player *pl, int type, int amount);
internal void __fastcall hook_pay(ee_player *pl, void *edx, void *what, int b, int c) {
    (void)edx;
    int *stock = player_stock_at(pl), before[7];
    memcpy(before, stock, sizeof before);
    cp_pay(pl, what, b, c);
    uintptr_t site = (uintptr_t)__builtin_return_address(0);
    for (int r = 2; r < 7; r++)
        if (before[r] > stock[r]) spend_note((unsigned)player_get_index(pl) & 15, r, before[r] - stock[r], site);
    mt_paid(pl, what, before, stock);
}
// navy.c (docs/navy.md stage 1): the navy account on water maps
internal void navy_income(unsigned idx, int type, int amount);
internal int navy_pays(unsigned idx, int type);
internal int navy_economy(unsigned idx, int type, int landing_order);
internal void navy_hold(ee_player *pl, unsigned idx, int reserve[7]);
internal int navy_account(unsigned idx, int r);
internal int navy_earmarked(unsigned idx);   // navy.c (stage 4): a warship spends the account through the reserve
internal void navy_paid(unsigned idx, int type, const int *spent, int got);
internal int navy_room_reserve(ee_player *pl);
internal int navy_pick(ee_player *pl, unsigned idx, int asked);
internal void navy_pick_done(ee_player *pl, unsigned idx, int pick, int asked, const int *before, const int *stock, int blocked);
#define FARM_EPOCH 2   // farms and granaries come with epoch 2, the Copper Age
internal int current_epoch(ee_player *pl);
internal int epoch_cost(ee_player *pl, int e, int r);
internal void __fastcall hook_add_resource(ee_player *pl, void *edx, int type, int amount) {
    (void)edx;
    cp_add_resource(pl, type, amount);
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    mt_income(pl, type, amount);   // metal.c (every player)
    if (amount > 0 && type >= 2 && type < 7 && g_bud[idx].init && !is_vanilla(pl)) {
        g_bud[idx].income[type] += amount;
        // from the Copper Age (farms: food no longer runs out) a player turns most of his income into army
        const struct ai_cfg *c = cfg_of(pl);
        int ep = current_epoch(pl);
        float sh = ep >= FARM_EPOCH ? c->mil_income_share_farms : c->mil_income_share;
        // early_army: no army in the first epoch (towers and citizens; with early_soldiers a few soldiers after all),
        // then food-free ranged soldiers - the army's food share small and at most 300 held, so the citizens keep
        // their food (the account held 500-780 food the army never spent while the citizens' stock was empty)
        if (EARLY(idx, early_army) && ep < FARM_EPOCH) {
            if (ep == 0 && !EARLY(idx, early_soldiers)) sh = 0;
            else if (type == 2) sh = g_bud[idx].mil[2] >= 300 ? 0 : g_early_food_share;
        }
        g_bud[idx].mil[type] += amount * sh;
        // copper_plan=2: epoch_share of the income goes into the epoch account until it holds the next epoch's cost
        // (the citizens train with the rest and never wait for the epoch; the plan's attempt comes once it is paid)
        if (copper_mode(pl) == 2 && ep < FARM_EPOCH && workforce(pl) >= g_min_workforce) {   // the economy first, as before
            if (g_epoch_acc_ep[idx] != ep + 1) { memset(g_epoch_acc[idx], 0, sizeof g_epoch_acc[idx]); g_epoch_acc_ep[idx] = ep + 1; }
            float miss = (float)epoch_cost(pl, ep + 1, type) - g_epoch_acc[idx][type], add = (float)amount * g_epoch_share;
            if (miss > 0) g_epoch_acc[idx][type] += add < miss ? add : miss;
        }
        navy_income(idx, type, amount);   // navy.c: the navy account on water maps
    }
}

// POPULATION LIMIT (aimod.ini pop_follow_human per difficulty, default 0 = the game limit; the shipped aimod.ini: 1
// for [easy] - the playtester: the defaults should show the AI at its strongest without cheating). Every minute 0x42F7A5 sets the AI's
// cap (player +0xCCC, read through EEComputerPlayer vf31) to (2.4 x the largest human population + 0.6 x the game
// limit) / 3 (dbcpbehavior records 92/93), times the difficulty's factor (record 98: 70 / 90 / 140%) - so the AI grows
// with the human and doesn't flood him with units; Ignorieren (+0xF02) or game setting 0x9187FC = 4 give it the game
// limit. pop_follow_human=0: the game limit (EEPlayer vf31 0x556EBC, as for a player) for every AI.
internal void __fastcall hook_pop_cap(ee_player *pl, void *edx) {
    (void)edx;
    cp_update_pop_cap(pl);
    if (!cfg_of(pl)->pop_follow && !is_vanilla(pl)) player_set_pop_cap(pl, game_unit_limit(pl));
}


// Citizens without useful work: unassigned ones plus those beyond their resource's target.
internal int idle_citizens(ee_player *pl) {
    void *cm = player_get_civ_mgr(pl);
    ee_resource_mgr *rm = cm ? cm_get_resource_mgr(cm) : NULL;
    if (!rm) return 0;
    int idle = FIELD(rm_list_at(rm, 0), 0x10, int);   // list 0: not assigned to a resource
    for (int t = 2; t < 7; t++) {
        int n = FIELD(rm_list_at(rm, t), 0x10, int), want = ((int *)rm_target_at(rm))[t];
        if (n > want) idle += n - want;
    }
    return idle;
}

internal void *um_owner(void *um) {
    for (int i = 0; i < 16; i++) {
        ee_player *pl = g_players[i];
        void *cm = pl ? player_get_civ_mgr(pl) : NULL;
        if (cm && cm_get_unit_mgr(cm) == um) return pl;
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
// copper_plan=2 (docs/economy.md): the plan without its brake. Against the base game's AI the plan was the most harmful
// feature of the mod (eco_ablate: off alone, cheating hard Continental 0/12 -> 6 won / 4 lost): the 1600 food of both
// epochs held from minute 0 stopped the citizens at min_workforce (40) and left the army nothing at all (0 soldiers
// until minute 22, 9-10 on food against the base AI's 18-29). Now only the next epoch's cost is held, the citizens
// never wait for it: epoch_share (40%) of the income goes into an epoch account until it holds the next epoch's cost,
// the citizens spend the rest (with the full stock to them the Town Center trained as fast as food came in and the
// Stone Age never came: epoch 0 at minute 28), and before farms food gets at least half the workers (up to 18). The
// epoch comes as soon as it is paid (the attempt moved up, the requirements asked early).
// g_min_workforce (top): economy first - below it, citizens ignore the reserve and no early advance
internal int copper_mode(ee_player *pl) {
    return PV((unsigned)player_get_index(pl) & 15, g_v_copper, cfg_of(pl)->copper_plan);
}
// EPOCH PACING follow (settings.c): the AI never gets ahead of the highest human epoch and advances as soon as it can
// pay once a human is ahead; meanwhile it prepares (the next epoch's requirements are asked for) but holds no
// resources back for the advance - a human may stay in an epoch for an hour. In every epoch, the copper plan's too.
// Without a human in the game: the timer alone. 1 = the next epoch held now.
internal int follow_held(ee_player *pl) {
    ee_tech_tree *tt = player_get_tech_tree(pl);
    if (cfg_of(pl)->pacing != PACE_FOLLOW || !tt || !human_count()) return 0;
    return highest_human_epoch() <= FIELD(tt, TT_EPOCH, int);
}
// The game's catch-up (FN_EPOCH_CATCH_UP, settings.c): a human reaching an epoch redraws the attempt of every computer
// player behind it to a few minutes from now. epoch_pacing=timer goes without it - the timer alone counts.
internal void __fastcall hook_epoch_catch_up(ee_player *pl, void *edx, ee_player *human) {
    (void)edx;
    if (!is_vanilla(pl) && cfg_of(pl)->pacing == PACE_TIMER) {
        if (g_log) fprintf(g_log, "# epoch plan: P%u at %.1f min: no catch-up (timer)\n",
                           (unsigned)player_get_index(pl) & 15, game_time() / 60000.0);
        return;
    }
    cp_epoch_catch_up(pl, human);
}

internal int workforce(ee_player *pl) {
    void *cm = player_get_civ_mgr(pl);
    ee_resource_mgr *rm = cm ? cm_get_resource_mgr(cm) : NULL;
    int n = 0;
    for (int t = 0; rm && t < 7; t++) n += FIELD(rm_list_at(rm, t), 0x10, int);
    return n;
}

internal uint8_t *epoch_tech(ee_player *pl, int e) {
    ee_tech_tree *tt = player_get_tech_tree(pl);
    if (!tt || e > 13 || e > end_epoch()) return NULL;
    return FIELD(FIELD_AT(tt, 8 + e * 0x54), OF(TTREC, 0x8), uint8_t *);
}
internal int epoch_cost(ee_player *pl, int e, int r) {
    uint8_t *d = epoch_tech(pl, e);
    int c = d ? FIELD(d, OF(TECHDATA, 0x24) + (r - 2) * 4, int) : 0;
    return c > 0 ? c : 0;
}
internal int current_epoch(ee_player *pl) {
    ee_tech_tree *tt = player_get_tech_tree(pl);
    return tt ? FIELD(tt, TT_EPOCH, int) : 0;
}

// EPOCH REQUIREMENTS: an epoch also needs a number of new things of the current epoch (epoch record tt+8+e*0x54:
// +0x40 needed, +0x48 done - 0x5BD681; counted by EETechTree::vf4 0x5BC4B6 for techs with data +0x7D set): a unit
// type or building trained / built for the first time (data +0x7C set, type +0x6C; not Town Center, Capitol,
// Settlement, Granary) or a tech. AttemptEpochAdvance (0x46C966) asks for the missing ones (0x4860D6 thiscall(cm+0x40,
// count): buildings through the build manager 0x46802D, units through the unit manager 0x48FB70, techs 0x485F57) -
// only at the attempt, once the AI can already pay, and only while the attention pass has switched on cm+0x7A.
// Two fixes (aimod.ini [all] early_requirements, default on):
// - our budget refused those units (the military account is nearly empty before farms): a requested unit type
//   that would count for the next epoch is paid from the stock above the reserve like a priority request
//   (requirement_unit, in hook_train_units). In the suites an AI that could pay waited 4 minutes on average for
//   its Copper Age, some 10+ (0 of 2 for 7 minutes).
// - like a player, the AI asks early: before farms as soon as the epoch is reached, later within 10 minutes of the
//   planned attempt; again after 3 minutes without progress.
internal int g_early_requirements = 1;
internal int requirements_missing(ee_player *pl) {
    ee_tech_tree *tt = player_get_tech_tree(pl);
    if (!tt) return 0;
    int next = FIELD(tt, TT_EPOCH, int) + 1;
    if (next > 13 || next > end_epoch()) return 0;
    uint8_t *rec = FIELD_AT(tt, 8 + next * 0x54);
    return FIELD(rec, OF(TTREC, 0x40), int) - FIELD(rec, OF(TTREC, 0x48), int);
}
// unit type whose first training counts for the next epoch (its tech: object data 0x9418A0[type] +0x4E0)
internal int requirement_unit(ee_player *pl, int type) {
    ee_tech_tree *tt = player_get_tech_tree(pl);
    ee_unit_type *od = type_data(type);
    if (!g_early_requirements || !tt || !od || requirements_missing(pl) < 1) return 0;
    ee_tech *t = tech_lookup(tt, type_get_tech(od));
    uint8_t *d = t ? FIELD(t, OF(TECH, 0x8), uint8_t *) : NULL;
    return d && !FIELD(t, OF(TECH, 0x4), uint8_t) && d[OF(TECHDATA, 0x7D)];
}
// Decoration (the editor's "x ..." objects: statues, houses, flags, ruins - name at object data +0x1C) counts as
// an epoch requirement too. When nothing else can be made right now (0x485C8F), the request falls back to every
// candidate and picks a statue (types 400, 402 in the suites), which the AI can never place: the build fix drops it
// after 3 failures, the request stays pending, and the epoch waits until something else happens to count. Their
// techs lose the "counts" flag (data +0x7D) - with no candidate left the request credits the requirement itself
// (0x4860D6's own fallback). Once per AI and game.
internal void *g_deco_done[16];   // the tech tree fixed (a new game's can reuse the address: econ_new_game)
internal void decoration_fix(ee_player *pl, unsigned ai) {
    void **done = g_deco_done;
    ee_tech_tree *tt = player_get_tech_tree(pl);
    if (!g_early_requirements || !tt || done[ai] == tt) return;
    done[ai] = tt;
    int n = 0;
    for (int type = 0; type < 0x300; type++) {   // the table's size, as the game checks it (0x4B..: < 0x300)
        ee_unit_type *od = type_data(type);
        const char *name = od ? type_get_name(od) : NULL;
        if (!name || name[0] != 'x' || name[1] != ' ') continue;
        ee_tech *t = tech_lookup(tt, type_get_tech(od));
        uint8_t *d = t ? FIELD(t, OF(TECH, 0x8), uint8_t *) : NULL;
        if (d && d[OF(TECHDATA, 0x7D)]) { d[OF(TECHDATA, 0x7D)] = 0; n++; }
    }
    if (g_log) fprintf(g_log, "# epoch plan: P%u: %d decoration objects no longer count as epoch requirements\n", ai, n);
}

// the epoch-requirement request out (cm+0x76) since: renewed by early_requirements (3 minutes without progress) or
// else re-armed by econ_tick after 5 minutes - one timer, so the two never renew the same request twice
internal uint32_t g_epoch_req_t[16];
internal void early_requirements(ee_player *pl, unsigned ai, uint32_t now, int prefarm) {
    static uint32_t asked[16];
    decoration_fix(pl, ai);
    static int asked_missing[16];
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    ee_tech_tree *tt = player_get_tech_tree(pl);
    if (!g_early_requirements || !cm || !tt || !cm_get_research_mgr(cm)) return;
    int missing = requirements_missing(pl);
    if (missing < 1) { asked[ai] = 0; return; }
    if (cm_get_request_out(cm)) {   // a request is out: renew it after 3 minutes without progress
        if (asked[ai] && now >= asked[ai] && now - asked[ai] < 180000) return;
        if (asked[ai] && missing < asked_missing[ai]) { asked[ai] = g_epoch_req_t[ai] = now ? now : 1; asked_missing[ai] = missing; return; }
    }
    if (!prefarm && (int)(cm_get_timer(cm, 0) - now) > 10 * 60000) return;
    rs_request_requirements(cm_get_research_mgr(cm), missing);
    cm_set_request_out(cm, 1);
    asked[ai] = g_epoch_req_t[ai] = now ? now : 1;
    asked_missing[ai] = missing;
    if (g_log) fprintf(g_log, "# epoch plan: P%u at %.1f min asks for %d new things for epoch %d\n", ai, now / 60000.0,
                       missing, FIELD(tt, TT_EPOCH, int) + 1);
}

// Per AI tick (from hook_rm_update): pull the epoch attempt forward in the pre-farm epochs once affordable.
internal int unit_cost(ee_player *pl, int type, int r);   // below
internal void retry_dropped(ee_player *pl, unsigned idx, uint32_t now) {
    if (!g_build_fix) return;
    if (now < 30000) { memset(g_retry[idx], 0, sizeof g_retry[idx]); return; }   // new game
    void *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
    ee_unit_mgr *um = cm ? cm_get_unit_mgr(cm) : NULL;
    int *stock = player_stock_at(pl);
    if (!bm || !um) return;
    for (int k = 0; k < 16; k++) {
        int type = g_retry[idx][k].type;
        if (!type || now < g_retry[idx][k].next) continue;
        type = upgraded_type(pl, type);
        uint8_t *dq = um_type_list(um, type);
        if (dq && FIELD(dq, 0x10, int) > 0) { g_retry[idx][k].type = 0; continue; }   // it has one now
        int pay = 1;
        for (int r = 2; r < 7; r++) if (unit_cost(pl, type, r) > stock[r]) pay = 0;
        if (!pay) { g_retry[idx][k].next = now + 30000; continue; }
        char ok = bm_place(bm, type, 0, 0);
        int give_up = !ok && ++g_retry[idx][k].tries >= 5;
        if (g_log) fprintf(g_log, "# place: P%u at %.1f min: dropped type %d tried again - %s\n", idx, now / 60000.0, type,
                           ok ? "placed" : give_up ? "failed, given up" : "failed");
        if (ok || give_up) g_retry[idx][k].type = 0;
        else g_retry[idx][k].next = now + 120000;
    }
}
internal uint32_t g_last_attack_time[16];   // +0xED8 at the last look (defence.c attack_retreat moves it on, not a launch)
// a vanilla player (game/players.c is_vanilla): econ_tick's log lines only - spending, gathering, the game's attack times
internal void econ_vanilla_log(ee_player *pl, uint32_t now) {
    unsigned ai = (unsigned)player_get_index(pl) & 15;
    spend_log(ai, now);
    gather_log(pl, ai, now);
    uint32_t t_attack = player_get_next_attack(pl);
    if (t_attack != g_last_attack_time[ai]) {
        if (g_last_attack_time[ai] && g_log)
            fprintf(g_log, "# attack: P%u at %.1f min, next at %.1f min\n", ai, now / 60000.0, t_attack / 60000.0);
        g_last_attack_time[ai] = t_attack;
    }
}
internal void econ_tick(ee_player *pl, uint32_t now) {
    const struct ai_cfg *c = cfg_of(pl);
    // attack log: the next-attack time (+0xED8) is redrawn whenever an attack starts
    unsigned ai = (unsigned)player_get_index(pl) & 15;
    spend_log(ai, now);
    gather_log(pl, ai, now);
    place_log(ai, now);
    retry_dropped(pl, ai, now);
    gather_research(pl, ai, now);
    attention_note(pl, ai, now);
    defence_tick(pl, ai, now);
    counter_siege(pl, ai, now);
    anti_air(pl, ai, now);
    tech_log_tick(pl, ai, now);
    home_guard(pl, ai, now);
    muster(pl, ai, now);
    hold_town(pl, ai, now);
    attack_retreat(pl, ai, now);
    {   // the base the AI plans around (build manager +0x12C/+0x130): log when it moves (a lost base, a new one?)
        static int bx[16] = { -10000, -10000, -10000, -10000, -10000, -10000, -10000, -10000, -10000, -10000, -10000,
                              -10000, -10000, -10000, -10000, -10000 }, by[16];
        void *cm = player_get_civ_mgr(pl);
        ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
        int x = bm ? bm_get_base_x(bm) : -10000, y = bm ? bm_get_base_y(bm) : -10000;
        if (bm && (x != bx[ai] || y != by[ai])) {
            if (g_log) fprintf(g_log, "# base: P%u at %.1f min: %d,%d -> %d,%d\n", ai, now / 60000.0, bx[ai], by[ai], x, y);
            bx[ai] = x;
            by[ai] = y;
        }
    }
    chat_tick(pl, ai, now);
    diplo_tick(pl, ai, now);
    rates_update(pl, ai, now);
    uint32_t t_attack = player_get_next_attack(pl);
    if (t_attack != g_last_attack_time[ai]) {
        if (g_last_attack_time[ai] && g_log)
            fprintf(g_log, "# attack: P%u at %.1f min, next at %.1f min\n", ai, now / 60000.0, t_attack / 60000.0);
        if (g_last_attack_time[ai]) persona_attack_launched(ai);
        if (g_last_attack_time[ai])
            ai_say(pl, CHAT_ATTACK, "[ai] attack launched, next one in %.0f min", ((float)t_attack - (float)now) / 60000.0f);
        g_last_attack_time[ai] = t_attack;
    }
    int ep = current_epoch(pl);
    early_tick(pl, ai, now);   // early.c: towers, the bait, the first army
    food_log(pl, ai, now);   // food.c
    mine_drops(pl, ai, now);   // mines.c
    mines_log(pl, ai, now);
    early_requirements(pl, ai, now, copper_mode(pl) && ep < FARM_EPOCH);
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    // epoch_pacing=follow, no human ahead: the attempt kept a minute off (as the game's own wait does) - the
    // requirements are asked for meanwhile (early_requirements: within 10 minutes of it), the cost isn't held
    static int8_t held_at[16];   // the epoch follow held at (logged once), -1 none
    int held = follow_held(pl);
    if (c->pacing == PACE_FOLLOW && held != (held_at[ai] == ep) && g_log)
        fprintf(g_log, "# epoch plan: P%u at %.1f min, epoch %d: %s (follow, the humans' highest %d of %d humans)\n", ai,
                now / 60000.0, ep, held ? "held" : "free to advance", highest_human_epoch(), human_count());
    held_at[ai] = held ? (int8_t)ep : -1;
    if (held) {
        uint32_t *t = cm ? cm_timers_at(cm) : NULL;
        if (t && *t < now + 60000) *t = now + 60000;
        return;
    }
    // pull the attempt forward: before farms with the copper plan, in every epoch with epoch_pacing=rush, and with
    // follow once a human is ahead
    if (!(copper_mode(pl) && ep < FARM_EPOCH) && c->pacing != PACE_RUSH && !(c->pacing == PACE_FOLLOW && human_count())) return;
    // AttemptEpochAdvance (0x46C966) asks the research manager once for the techs the next epoch requires
    // (record +0x40 needed, +0x48 done) and then waits while cm+0x76 is set; re-arm it after 5 minutes.
    uint32_t *pending_since = g_epoch_req_t;   // shared with early_requirements' renewal
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    if (cm && cm_get_request_out(cm)) {
        if (!pending_since[pi] || now < pending_since[pi]) pending_since[pi] = now ? now : 1;
        else if (now - pending_since[pi] > 5 * 60000) {
            cm_set_request_out(cm, 0);
            pending_since[pi] = 0;
            if (g_log) fprintf(g_log, "# copper plan: P%u epoch requirement still pending after 5 min, re-requesting\n", pi);
        }
    } else pending_since[pi] = 0;
    if (!cm || !epoch_tech(pl, ep + 1)) return;
    int *stock = player_stock_at(pl);
    for (int r = 2; r < 7; r++)
        if (stock[r] < epoch_cost(pl, ep + 1, r)) return;
    uint32_t *t_epoch = cm_timers_at(cm);
    if (*t_epoch > now + 5000) {
        if (g_log) fprintf(g_log, "# epoch plan: P%d can pay epoch %d at %.1f min, attempt moved up from %.1f min\n",
                           player_get_index(pl), ep + 1, now / 60000.0, *t_epoch / 60000.0);
        *t_epoch = now;
    }
    // diagnostic: while it can pay but hasn't advanced, every 30 s - requirement techs needed/done (epoch record
    // +0x40/+0x48), requirement request pending (cm+0x76), epoch research scheduled (cm+0x78), attention cm+0x7A
    static uint32_t last_wait[16];
    if (g_log && (now < last_wait[pi] || now - last_wait[pi] >= 30000)) {
        last_wait[pi] = now;
        ee_tech_tree *tt = player_get_tech_tree(pl);
        uint8_t *rec = FIELD_AT(tt, 8 + (ep + 1) * 0x54);
        fprintf(g_log, "# epoch wait: P%u at %.1f min, epoch %d: requirements %d/%d, pending %d, scheduled %d, attention %d;",
                pi, now / 60000.0, ep + 1, FIELD(rec, OF(TTREC, 0x48), int), FIELD(rec, OF(TTREC, 0x40), int), cm_get_request_out(cm), cm_get_unknown_78(cm), cm_get_att_epoch_techs(cm));
        // the research manager's list of this epoch: tech id (* researched) building +0x6C, cost food..iron
        ee_research_mgr *rs = cm_get_research_mgr(cm);
        int *b = rs ? rs_get_epoch_begin(rs, ep) : NULL, *en = rs ? rs_get_epoch_end(rs, ep) : NULL;
        for (int *q = b; b && en > b && en - b < 100 && q < en; q++) {
            ee_tech *t = tech_lookup(tt, *q);
            uint8_t *d = t ? FIELD(t, OF(TECH, 0x8), uint8_t *) : NULL;
            if (d) fprintf(g_log, " %d%s@%d(%d/%d/%d/%d/%d)", *q, FIELD(t, OF(TECH, 0x4), uint8_t) ? "*" : "", FIELD(d, OF(TECHDATA, 0x6C), int), FIELD(d, OF(TECHDATA, 0x24), int),
                           FIELD(d, OF(TECHDATA, 0x28), int), FIELD(d, OF(TECHDATA, 0x2C), int), FIELD(d, OF(TECHDATA, 0x30), int), FIELD(d, OF(TECHDATA, 0x34), int));
        }
        fputc('\n', g_log);
    }
}

// The pre-farm food budget is what the map offers: wild food (forage sites) the AI has explored within safe
// reach of its base (g_site_max_dist), like a player counting the berry patches around his town. Only part
// of it will actually be gathered (competition, losses), hence g_wild_share.
internal float g_wild_share = 0.6f;   // aimod.ini [budget] wild_food_share
internal int wild_food(ee_player *pl) {
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    void *bm = cm ? cm_get_build_mgr(cm) : NULL;
    if (!bm) return 0;
    int bx = bm_get_base_x(bm), by = bm_get_base_y(bm);
    if (bx == -10000) return 0;
    int lim = g_site_max_dist * g_site_max_dist, total = 0;
    void **zb = player_get_zones_begin(pl), **ze = player_get_zones_end(pl);
    for (int z = 0; zb && zb + z < ze; z++) {
        ee_unit *s = zb[z] ? zone_site(zb[z], 2) : NULL;
        if (!s || type_get_id(unit_get_type_data(s)) != UT(RES_FORAGE_BUSH)) continue;
        int dx = unit_get_x(s) - bx, dy = unit_get_y(s) - by;
        if (dx * dx + dy * dy >= lim) continue;
        if (!explored_by(s, pl)) continue;
        total += unit_get_amount(s);
    }
    return total;
}

// Worker targets (after ComputeTargets), pre-farm epochs only: the original targets follow each resource's
// share of outstanding demand - which our budget inflates for food (blocked military requests stay on the
// books), so workers piled onto food while stone/gold starved and the epoch requirement techs never got
// paid. Redistribute the same number of workers by actual shortfall: food only up to the epoch reserve,
// others by demand beyond stock, stone/gold/iron at least what the Copper Age still costs.
// LATE WOOD ([all] late_targets, bits, default 3): from the Copper Age on the game's targets stand - except wood. Its
// food demand never falls below ~75000 (every game, from minute 0), so food takes the lion's share and wood got 1-3
// workers: 92 wood, no barracks, tank factory, airport or anti-air rebuilt, only wolves from the Town Center; and the
// rebalance (below) moved surplus workers only to mines - 38 of Livia's on gold with a target of 0, 5587 gold.
// Bit 1: wood gets at least the workers that close its gap within about 2 minutes - the game's wood demand (what it
// wants built and trained: buildings, fields, ships and other wood units on water maps) beyond the stock, counted up
// to 2000 (refused requests stay on the books), at the measured rate per worker; 2..40% of the workers, changed in
// steps of 2+; with no gap no more than 2 - wood only pays when it's needed, a pile of it buys nothing. Taken from the
// largest other targets. Bit 2: the rebalance moves surplus gatherers to wood too. Late-start A/B, Continental and
// Mediterranean (water) x 6 seeds, off / a fifth of the workers under 400 wood / by need: military buildings and
// docks per AI 6.0/5.2/7.7 and 4.0/3.5/6.8, the largest wood stock 589/1475/1138 and 475/2214/610 (the fixed fifth
// piled wood up on water without building more), survival within the noise. (Tried first and dropped: food capped at
// its fields + 2, the rest by shortfall - late-start A/B, 3 maps x 4 seeds: alive 86% vs 98%, 15 workers fewer at
// the end; a late start has no fields yet, its food is hunted, gathered and fished.)
// Bit 4: the overflow to the metals. The game wants ~200 farmers for 64 fields; the leftover pass puts the rest on
// any free mine or on wood - 73 woodcutters (target 2) and 5475 wood, 3426 gold, 16000 food while the army failed
// for gold and iron. With 8+ fields and 2000 food beyond its reserve, food's target drops to its fields + 2 and the
// rest goes to stone/gold/iron by their demand; with a wood pile of 1000+ beyond its need, the metals' targets also
// grow by the woodcutters beyond target + 5 (farms full, food short all the same). The rebalance moves them.
// antiair save, 55 min (3 / 7): Brann's wood 5475 -> 860, soldiers 48 -> 69, army orders failed for gold/iron
// 688/563 -> 343/372, the idle player beaten at 52.6 min; late-start A/B, 3 maps x 6 seeds, 50 min: alive 77% vs
// 78%. Without the food condition: alive 68% vs 78% (late starts live on hunted food).
// Bit 8: the food pile. A farmer brings ~17 food a minute, a miner 4-6 gold or iron, and the units of the late ages
// cost about half food, half metal - 70 farmers against 108 miners: 10944 food, 16000 in the army's food account,
// while 1491 orders failed for gold and 1282 for iron in 5 minutes (Empire Earth has no market). While 4000+ food lie
// beyond the epoch reserve, the food target's ceiling drops by 2 every 10 s (to 6 at least; fields stand idle) and the
// farmers go to the metals; under 2000 it rises again (6 every 10 s; under 500 at once), up to the fields + 2, down
// to half the fields. Not in the default (7): antiair save at 55 min 100 soldiers vs 70, but late-start A/B, 3
// maps x 6 seeds, 50 min: alive 70% vs 75% (with the cut to 6 farmers also 70%) - idle fields cost income the AI
// misses when citizens or the epoch need food. The fields pass sends no farmers beyond food's target.
internal int *bq_entry(ee_build_mgr *bm, int e, unsigned i);   // attack.c: the build queue of epoch e
internal int rm_count(ee_resource_mgr *rm, int t);                 // below: citizens on resource t
internal int unit_cost(ee_player *pl, int type, int r);       // below
internal int g_late_targets = 7;
internal int g_wood_want[16];   // bit 1: the last wood target (steps of 2+)
internal int g_food_cap[16];          // bit 8: the food target's ceiling now (0 = not set)
internal uint32_t g_food_cap_t[16];   // its last step
// n more workers for stone, gold and iron: by their demand beyond stock (up to 3000 each, +100 so each gets some), the
// rounding rest to gold and iron
internal void to_metals(uint32_t *target, const int *demand, const int *stock, int n) {
    float lw[7] = { 0 }, ws = 0;
    int given = 0;
    if (n <= 0) return;
    for (int r = 4; r < 7; r++) {
        int d = demand[r] - stock[r];
        lw[r] = (float)(d < 0 ? 0 : d > 3000 ? 3000 : d) + 100.0f;
        ws += lw[r];
    }
    for (int r = 4; r < 7; r++) {
        int t = (int)((float)n * lw[r] / ws);
        target[r] += (uint32_t)t;
        given += t;
    }
    for (int r = 5; given < n; r = r == 6 ? 5 : r + 1) { target[r]++; given++; }
}
// Bit 16: METAL PULL. The game splits the workers by its demand, and its food demand is always ~77000 (phantom):
// gold (1000-5000) and iron (2000-5800) got 1-6% of them - P3 with 69 farmers, 15 on gold and 10 on iron at
// minute 29, P2 with 24/22/1+6+1 for 24 minutes; every late unit costs half gold or iron, so 10-20 soldiers each.
// Every 10 s: when the army's orders failed for stone/gold/iron (5+, more than twice those for food) and the food
// stock is above its reserve, 2 more of the food target go to the metals (by their demand); when food fails more,
// or the food stock falls under the reserve + 200, 4 come back. At most 60% of the food target, and not more while
// the mines can't take those already sent (metal workers 6+ under their targets).
internal int g_pull[16];
internal uint32_t g_pull_t[16];
internal unsigned g_pull_seen[16][7];
internal unsigned mil_short(unsigned idx, int r);   // below: the army's orders refused for resource r (counter)
internal void budget_reserve(ee_player *pl, int reserve[7]);
internal void navy_mines(ee_player *pl, unsigned idx, ee_resource_mgr *rm, uint32_t *target, const int *stock, const int *reserve, const int *demand);
internal void econ_targets(ee_resource_mgr *rm) {
    ee_player *pl = rm_player(rm);
    if (!pl || !copper_mode(pl)) return;
    int late = current_epoch(pl) >= FARM_EPOCH;
    int lt = PV(player_get_index(pl), g_v_late, g_late_targets);
    if (late && !(lt & 1)) return;
    int *stock = player_stock_at(pl);
    int *demand = rm_demand_at(rm);
    uint32_t *target = rm_target_at(rm);
    int reserve[7];
    budget_reserve(pl, reserve);
    int total = 0, need[7] = { 0 }, sum = 0;
    for (int r = 2; r < 7; r++) total += (int)target[r];
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    if (late) {
        if (total < 6) return;
        // the game's wood demand, or the wood of the next 3 queued buildings up to its epoch if more: a building it
        // can't pay for is no demand at all (the queue's "can build", 0x555710, tests the cost too) - the airport
        // (425 wood) and tank factory waited with 115 wood in stock and a demand of 55, so no AA tank either
        void *cm = rm_get_owner(rm);
        ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
        int queued = 0, nq = 0;
        for (int e = 0; bm && e <= current_epoch(pl) && e < 14 && nq < 3; e++)
            for (unsigned i = 0; nq < 3; i++) {
                int *ent = bq_entry(bm, e, i);
                if (!ent) break;
                int c = unit_cost(pl, ent[0], 3);
                if (c > 0) { queued += c; nq++; }
            }
        int wood_need = (demand[3] > queued ? demand[3] : queued) - stock[3];   // filled within about 2 minutes
        wood_need = wood_need < 0 ? 0 : wood_need > 2000 ? 2000 : wood_need;
        float pw = g_rate[idx].per_worker[3];
        int want = (int)((float)wood_need / (2.0f * (pw > 2.0f ? pw : 2.0f)) + 0.99f), most = total * 2 / 5;
        want = want < 2 ? 2 : want > most ? most : want;
        if (abs(want - g_wood_want[idx]) < 2 && g_wood_want[idx]) want = g_wood_want[idx];   // steps of 2+
        g_wood_want[idx] = want;
        while ((int)target[3] < want) {
            int from = -1;
            for (int r = 2; r < 7; r++)
                if (r != 3 && (int)target[r] > 2 && (from < 0 || target[r] > target[from])) from = r;
            if (from < 0) break;
            target[from]--;
            target[3]++;
        }
        void *um = cm ? cm_get_unit_mgr(cm) : NULL;
        uint8_t *fq = type_list(um, UT(B_FARM));
        int fields = fq ? FIELD(fq, 0x10, int) : 0;
        if (lt & 4) {   // the overflow to the metals (see LATE WOOD, bit 4)
            int cap = fields + 2;
            if (fields >= 8 && (int)target[2] > cap && stock[2] >= reserve[2] + 2000) {   // food to spare
                to_metals(target, demand, stock, (int)target[2] - cap);
                target[2] = (uint32_t)cap;
            }
            // the farms full and food short all the same (Livia: 84 farmers, 122 food): the overflow still lands on
            // wood (118 woodcutters, target 2, 6855 wood). A wood pile of 1000+ beyond its need: the metals' targets
            // grow by the woodcutters beyond target + 5, and the rebalance moves them
            int pile = stock[3] - (demand[3] > queued ? demand[3] : queued), extra = rm_count(rm, 3) - (int)target[3] - 5;
            if (pile > 1000 && extra > 0) to_metals(target, demand, stock, extra);
        }
        if (lt & 8) {   // the food pile: farmers to the metals while 4000+ food lie beyond the reserve
            uint32_t now = game_time();
            if (now < g_food_cap_t[idx]) g_food_cap[idx] = 0;   // a new game
            int spare = stock[2] - reserve[2], cap = g_food_cap[idx] ? g_food_cap[idx] : (int)target[2];
            int top = fields + 2 > 6 ? fields + 2 : 6;
            // down slowly (2 every 10 s), up fast (6; under 500 all fields at once): with 2 both ways 69 farmers went
            // down to 19, and coming back as slowly the stock fell from 6000 to 18
            if (now < g_food_cap_t[idx] || now - g_food_cap_t[idx] >= 10000) {
                g_food_cap_t[idx] = now ? now : 1;
                cap += spare > 4000 ? -2 : spare < 2000 ? 6 : 0;
            }
            if (spare < 500) cap = top;
            int low = fields / 2 > 6 ? fields / 2 : 6;   // at most half the fields idle
            cap = cap < low ? low : cap > top ? top : cap;
            g_food_cap[idx] = cap;
            if ((int)target[2] > cap) {
                to_metals(target, demand, stock, (int)target[2] - cap);
                target[2] = (uint32_t)cap;
            }
        }
        if (lt & 16) {   // the metal pull
            uint32_t now = game_time();
            if (now < g_pull_t[idx]) { g_pull[idx] = 0; memset(g_pull_seen[idx], 0, sizeof g_pull_seen[idx]); }   // new game
            if (!g_pull_t[idx] || now < g_pull_t[idx] || now - g_pull_t[idx] >= 10000) {
                g_pull_t[idx] = now ? now : 1;
                unsigned d[7] = { 0 };
                for (int r = 2; r < 7; r++) {   // refusals since the last look (the counter restarts with the army log)
                    unsigned c = mil_short(idx, r);
                    d[r] = c >= g_pull_seen[idx][r] ? c - g_pull_seen[idx][r] : c;
                    g_pull_seen[idx][r] = c;
                }
                unsigned metal = d[4] + d[5] + d[6];
                int spare = stock[2] - reserve[2];
                int unplaced = rm_count(rm, 4) + rm_count(rm, 5) + rm_count(rm, 6) < (int)(target[4] + target[5] + target[6]) - 6;
                if (d[2] > metal || spare < 200) g_pull[idx] -= 4;
                else if (metal >= 5 && metal > 2 * d[2] && spare > 300 && !unplaced) g_pull[idx] += 2;
            }
            int pull_most = (int)target[2] * 3 / 5;
            g_pull[idx] = g_pull[idx] < 0 ? 0 : g_pull[idx] > pull_most ? pull_most : g_pull[idx];
            if (g_pull[idx] > 0) {
                to_metals(target, demand, stock, g_pull[idx]);
                target[2] -= (uint32_t)g_pull[idx];
            }
        }
        navy_mines(pl, idx, rm, target, stock, reserve, demand);   // navy.c: the mines on a water map (the navy's and all reachable)
        return;
    }
    need[2] = reserve[2] + 300 - stock[2];
    for (int r = 3; r < 7; r++) {
        int d = demand[r];
        // stone/gold/iron: the military's demand only up to mineral_worker_margin beyond the reserve (a human game: 12-19 citizens
        // on iron with 1000 in stock and 400 needed, while the Stone Age waited 15 minutes for food)
        if (r >= 4 && g_mineral_margin >= 0 && d > reserve[r] + g_mineral_margin) d = reserve[r] + g_mineral_margin;
        need[r] = d - stock[r];
        if (r >= 4 && reserve[r] - stock[r] > need[r]) need[r] = reserve[r] - stock[r];
    }
    need[4] += early_tower_stone(pl);   // early.c: the stone of the town's first towers
    food_plan_needs(pl, rm, need, stock, reserve);   // food.c (food_plan PLAN): the citizens' food, the real wood need
    // reserve plan: by worker-minutes (shortfall / measured rate per worker), so all arrive at about the same time
    float w[7] = { 0 }, wsum = 0;
    for (int r = 2; r < 7; r++) {
        if (need[r] < 0) need[r] = 0;
        sum += need[r];
        float pw = g_rate[idx].per_worker[r];
        w[r] = g_reserve_plan ? (float)need[r] / (pw > 1.0f ? pw : 1.0f) : (float)need[r];
        wsum += w[r];
    }
    if (sum <= 0 || wsum <= 0 || total < 6) return;
    int floor_[7] = { 0, 0, 2, 2, 0, 0, 0 };   // keep a few on food (growth) and wood (building)
    int rest = total - 4, given = 0;
    for (int r = 2; r < 7; r++) {
        int t = floor_[r] + (int)((float)rest * w[r] / wsum);
        target[r] = (uint32_t)t;
        given += t;
    }
    for (int r = 2; given < total; r = r == 6 ? 2 : r + 1)   // rounding leftovers to the largest needs
        if (need[r] > 0) { target[r]++; given++; }
    if (copper_mode(pl) == 2) {   // the citizens' own food: half the workers on food, up to 18 (the base AI: 18-29)
        int fl = total / 2 > g_food_floor ? g_food_floor : total / 2;
        while ((int)target[2] < fl) {
            int from = -1;
            for (int r = 3; r < 7; r++)
                if ((int)target[r] > 2 && (from < 0 || target[r] > target[from])) from = r;
            if (from < 0) break;
            target[from]--;
            target[2]++;
        }
    }
}

// WORKER REBALANCE (after every AssignCitizens pass, 10 s): AssignCitizens only hands out idle citizens and never
// takes a gatherer back, so workers stay wherever an old target sent them - 21 on iron with a target of 9 and
// 1300 iron unused, while gold (target 17) had 5 and every soldier failed for gold. Move gathering citizens from
// the resource with the largest surplus to a stone/gold/iron mine that is short, with the game's own site search
// (0x48C204: free slot, explored, no enemy near, the citizen's continent) and assignment (0x48C81C: new gather
// target, moves the citizen between the resource lists). Hysteresis: the targets jump with demand every 5 s.
internal int g_rebalance = 1;   // aimod.ini [all] rebalance_workers
internal unsigned g_moved[16][7];
internal int rm_count(ee_resource_mgr *rm, int t) { return FIELD(rm_list_at(rm, t), 0x10, int); }
internal ee_unit *movable_gatherer(ee_resource_mgr *rm, int t) {
    uint8_t *dq = rm_list_at(rm, t);
    unsigned first = FIELD(dq, 0xC, unsigned), n = FIELD(dq, 0x10, unsigned);
    for (unsigned k = n; k-- > 0; ) {   // the newest first
        ee_unit *u = deque_at(dq, first + k);
        if (!u || unit_get_hp(u) <= 0 || unit_get_frozen(u) || unit_get_ai_held(u) || unit_get_busy(u))
            continue;
        ee_goal *goal = unit_get_goal(u);
        if (goal && !strcmp(rtti_name(goal), "EEUGGather@@")) return u;
    }
    return NULL;
}
// FARM FIELDS (build_fields, aimod.ini [all], default on). A granary comes with 8 fields (type 0x2A) as foundations
// ([field+0x30]+0x340 = finished). The game's citizen assignment (0x48A554) sends citizens to finished fields
// (0x48BFD0) and builders to unfinished ones (0x48BF4F, orders at 0x48B118) only for a food shortfall left after
// wild food, with citizens left over - and builders only while player +0xEFA is 0: on a map with water and 10+ fish
// (0x4307A7 counts type 0x13B) the byte is set and the shortfall goes to fishing boats (0x48FB05, 0x48C424). A field
// also takes 50 wood when its construction starts (the builder stops without it, as for a player), and the AIs keep
// their wood near 0. So the fields stayed foundations - on island maps always, in late-epoch starts on land too -
// and the spare citizens went to gold (19 on gold, target 1). And nothing replaces a destroyed field: the building
// plan (cm+8, 0x46CD18) holds only houses, the build list's re-queue only buildings missing entirely.
// - unit training leaves 50 wood per unfinished field (up to 2) in the stock;
// - at each assignment (10 s) unfinished fields without a builder get one each, as far as the wood in stock pays -
//   an idle citizen, else the nearest on food, else on stone, gold or iron, never one on wood - with the game's own
//   order; the builder farms the field when it is done;
// - a finished field nobody farms gets an idle citizen, else the nearest from stone, gold or iron above its target
//   (the game's gather order 0x48C81C moves it to the food list);
// - lost fields are laid out again (fields_rebuild). Fishing goes on as before.
// None of it with enemy soldiers within 6 tiles (raiders_near, attack.c): a few bowmen next to a raided field got
// every citizen the passes sent to rebuild or farm it, one by one.
internal int g_build_fields = 1;
internal int g_fields_open[16];   // unfinished fields per player (econ_fields), for the wood reserve
internal int field_wood(unsigned idx) { return g_build_fields ? 50 * (g_fields_open[idx] < 2 ? g_fields_open[idx] : 2) : 0; }
internal ee_unit *field_site(ee_unit *f) {   // what a BuildRepair goal holds (+0x3C) for this field (0x60BAD0)
    ee_unit *p = unit_get_physics(f), *last = f;
    while (p) { last = p; p = unit_get_chain(p); }
    return last;
}
internal int citizen_free(ee_unit *u, int idle) {   // idle, or (idle=0) gathering
    if (!u || unit_get_hp(u) <= 0 || unit_get_frozen(u) || unit_get_ai_held(u) || unit_get_busy(u) ||
        unit_get_x(u) == -10000)
        return 0;
    ee_goal *goal = unit_get_goal(u);
    return idle ? !goal : goal && !strcmp(rtti_name(goal), "EEUGGather@@");
}
// plan P: the building plan's entries (cm+8: list head, node +0 next, +0xC type, +0x10 wanted count), as 0x46CD18
// walks them (0x2F = the epoch's tower)
internal void cmd_plan(char *out, const char *args) {
    int p = atoi(args), t = 0;
    if (sscanf(args, "%d retry %d", &p, &t) == 2 && p > 0 && p < 16) {   // plan P retry T: T counts as dropped, tried now
        retry_note((unsigned)p, t, game_time());
        for (int k = 0; k < 16; k++) if (g_retry[p][k].type == t) g_retry[p][k].next = game_time();
        REPLY("P%d: type %d queued for a retry\n", p, t);
        return;
    }
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    ee_civ_mgr *cm = pl ? player_get_civ_mgr(pl) : NULL; uint8_t *head = cm ? cm_get_plan(cm) : NULL;
    if (!head) { REPLY("error: no AI player %d\n", p); return; }
    REPLY("P%d plan (build flag %d, attention %d):", p, player_get_build(pl), cm_get_att_build(cm));
    int n = 0;
    for (uint8_t *nd = FIELD(head, OF(CMLIST, 0x0), uint8_t *); nd && nd != head && n < 200; nd = FIELD(nd, OF(CMLIST, 0x0), uint8_t *), n++)
        REPLY(" %d:%d", FIELD(nd, OF(CMLIST, 0xC), int), FIELD(nd, OF(CMLIST, 0x10), int));
    REPLY("\n");
}
internal int field_worked(ee_resource_mgr *rm, ee_unit *f) {   // a citizen of this AI farms it or is on its way
    // (0x52E5F7 counts only those at the field - one carrying food to the granary left it "free")
    for (int r = 0; r < 7; r++) {
        uint8_t *list = rm_list_at(rm, r);
        for (unsigned k = 0, first = FIELD(list, OF(EPOCHTECHS, 0xC), unsigned), cnt = FIELD(list, OF(EPOCHTECHS, 0x10), unsigned); k < cnt; k++) {
            ee_unit *u = deque_at(list, first + k); ee_goal *goal = u ? unit_get_goal(u) : NULL;
            if (goal && goal_get_site(goal) == f && !strcmp(rtti_name(goal), "EEUGGather@@")) return 1;
        }
    }
    return 0;
}
// Lost fields: nothing in the game replaces a destroyed field - the building plan (cm+8) holds only houses, the
// build list's re-queue (0x465A8E) only buildings missing entirely, and the granary still stands. The tiles of every
// field an AI had are kept; a field gone from its tile while a granary of the player stands within 4 tiles gets a
// new foundation there, made the way the build manager makes buildings (0x62EF8A cdecl(player, type, x, y); a field
// pays nothing there - its 50 wood go when the construction starts), if the game's placement test allows it; the
// passes above build and farm it. Again after 2 minutes if it is still missing. Not through the build queue: the
// placement's field case (0x46580A) only orders builders to an existing foundation and crashes without one
// (0x60BAD0 with a null building).
internal struct { int16_t x, y; uint32_t queued; } g_field_at[16][32];
internal int g_field_n[16];
internal int can_place(ee_player *pl, int type, int x, int y);   // defence.c
internal int raiders_near(ee_player *pl, int x, int y, int r);     // attack.c: enemy soldiers it can see
internal void fields_rebuild(ee_player *pl, unsigned idx, void *um, uint8_t *dq, uint32_t now) {
    if (now < 60000) g_field_n[idx] = 0;   // new game
    unsigned first = dq ? FIELD(dq, 0xC, unsigned) : 0, cnt = dq ? FIELD(dq, 0x10, unsigned) : 0;
    for (unsigned k = 0; k < cnt; k++) {   // remember every field's tile
        ee_unit *f = deque_at(dq, first + k);
        if (!f || unit_get_hp(f) <= 0) continue;
        int x = unit_get_x(f), y = unit_get_y(f), j = 0;
        while (j < g_field_n[idx] && (g_field_at[idx][j].x != x || g_field_at[idx][j].y != y)) j++;
        if (j == g_field_n[idx] && j < 32) { g_field_at[idx][j].x = (int16_t)x; g_field_at[idx][j].y = (int16_t)y;
                                            g_field_at[idx][j].queued = 0; g_field_n[idx]++; }
    }
    uint8_t *gq = type_list(um, UT(B_GRANARY));
    int queued = 0;
    if (!gq) return;
    for (int j = 0; j < g_field_n[idx]; j++) {
        int x = g_field_at[idx][j].x, y = g_field_at[idx][j].y, there = 0, granary = 0;
        for (unsigned k = 0; k < cnt && !there; k++) {
            ee_unit *f = deque_at(dq, first + k);
            there = f && unit_get_hp(f) > 0 && unit_get_x(f) == x && unit_get_y(f) == y;
        }
        if (there || (g_field_at[idx][j].queued && now - g_field_at[idx][j].queued < 120000)) continue;
        for (unsigned k = 0, gf = FIELD(gq, 0xC, unsigned), gc = FIELD(gq, 0x10, unsigned); k < gc && !granary; k++) {
            ee_unit *g = deque_at(gq, gf + k);
            granary = g && unit_get_hp(g) > 0 && abs(unit_get_x(g) - x) <= 4 && abs(unit_get_y(g) - y) <= 4;
        }
        if (!granary) continue;
        if (raiders_near(pl, x, y, 8)) continue;   // raided: later (builders: 8 tiles, as citizens_safety)
        g_field_at[idx][j].queued = now;
        if (!ut_has(UT(B_FARM)) || !can_place(pl, UT(B_FARM), x, y)) continue;
        place_foundation(pl, UT(B_FARM), x, y);
        queued++;
    }
    if (queued && g_log) fprintf(g_log, "# fields: P%u at %.1f min: %d lost fields laid out again\n", idx, now / 60000.0, queued);
}
// MORE GRANARIES (more_granaries, aimod.ini [all], default on): a granary feeds 8 fields, and the AI builds one -
// its build list places it once; the building plan's farm entry (0x46CD18: fields wanted > fields -> a granary) is
// not in the random-map plans. Food targets of 50-100 workers then had 17-23 on food (8 farming, the rest hunting
// far away) and the citizens the assignment couldn't place went to gold (34 on gold, target 4; 4000+ gold unused,
// food 0, no army). Once a minute: food target >= food workers + 6, every field finished and farmed, no granary
// queued (0x465640) or unfinished, fewer than 1 + target/8 granaries (at most 8) -> the build manager is asked
// for one (0x46802D(bm, Granary, 1, 0), as that plan entry does); it comes with 8 fields, which the passes above build
// and farm.
// The game's spot can be one its builders never manage to lay out: P2's main Town Center hemmed in by walls, towers,
// a tank factory and both granaries' fields - every minute a request, "placed", the builders stood at 95,97 with no
// foundation until the next request moved them, idle; 16 fields for a food target of 107 from minute 14 to the end
// (88 on wood, food 10). A request without a new granary 50 s later: the next goes to our own spot - near a Town
// Center, Capitol or Settlement, 4-12 tiles out, placeable, no enemy soldiers within 12, room for 6+ of the 8 fields
// around it (2 tiles apart, as the game lays them out).
internal int granary_spot(ee_player *pl, void *um, int *ox, int *oy) {
    static const enum ut_name centers[] = { UT_B_TOWN_CENTER, UT_B_CAPITOL, UT_B_SETTLEMENT };
    for (int c = 0; c < 3; c++) {
        uint8_t *q = type_list(um, g_ut[centers[c]]);
        for (unsigned k = 0, f = q ? FIELD(q, 0xC, unsigned) : 0, n = q ? FIELD(q, 0x10, unsigned) : 0; k < n; k++) {
            ee_unit *b = deque_at(q, f + k);
            if (!b || unit_get_hp(b) <= 0) continue;
            int cx = unit_get_x(b), cy = unit_get_y(b);
            for (int r = 4; r <= 12; r++)
                for (int ax = -r; ax <= r; ax++)
                    for (int ay = -r; ay <= r; ay++) {
                        if (abs(ax) != r && abs(ay) != r) continue;
                        int x = cx + ax, y = cy + ay, room = 0;
                        if (!can_place(pl, UT(B_GRANARY), x, y) || raiders_near(pl, x, y, 12)) continue;
                        for (int fx = -2; fx <= 2; fx += 2)
                            for (int fy = -2; fy <= 2; fy += 2)
                                if ((fx || fy) && can_place(pl, UT(B_FARM), x + fx, y + fy)) room++;
                        if (room < 6) continue;
                        *ox = x; *oy = y;
                        return 1;
                    }
        }
    }
    return 0;
}
internal int g_more_granaries = 1;
internal void more_granaries(ee_player *pl, unsigned idx, ee_resource_mgr *rm, void *cm, void *um, int open_fields, uint32_t now) {
    static uint32_t next[16];
    if (!g_more_granaries || open_fields || current_epoch(pl) < FARM_EPOCH) return;
    if (now < next[idx] && next[idx] - now <= 60000) return;
    next[idx] = now + 60000;
    int want = (int)rm_get_target(rm, 2), have = rm_count(rm, 2);
    ee_build_mgr *bm = cm_get_build_mgr(cm);
    uint8_t *gq = type_list(um, UT(B_GRANARY));
    int granaries = 0;
    for (unsigned k = 0, f = gq ? FIELD(gq, 0xC, unsigned) : 0, c = gq ? FIELD(gq, 0x10, unsigned) : 0; k < c; k++) {
        ee_unit *g = deque_at(gq, f + k), *st = g ? unit_get_physics(g) : NULL;
        if (!g || unit_get_hp(g) <= 0) continue;
        if (!st || !unit_get_finished(st)) return;   // one is being built
        granaries++;
    }
    // the first granary as soon as food lacks 2 citizens (farms are the food base, fish only a bonus - the
    // playtester: "fish is very useful but a limited resource; the boats have to go further and further out"): on
    // water maps (+0xEFA) the game's build list and plan bring none (0x46CD18 counts the fields as wanted there)
    if (want < have + (granaries ? 6 : 2)) return;
    int cap = 1 + want / 8;
    if (!bm || !ut_has(UT(B_GRANARY)) || granaries >= (cap < 8 ? cap : 8) || bm_queued(bm, UT(B_GRANARY))) return;
    static struct { uint32_t t; int n; } req[16];
    int x, y;
    if (now < req[idx].t) req[idx].t = 0;   // a new game
    if (req[idx].t && granaries <= req[idx].n && now - req[idx].t >= 50000 && granary_spot(pl, um, &x, &y)) {
        char ok = bm_place(bm, UT(B_GRANARY), x, y);
        if (g_log) fprintf(g_log, "# fields: P%u at %.1f min: the last granary request brought none - our spot %d,%d: %s\n",
                           idx, now / 60000.0, x, y, ok ? "placed" : "refused");
        req[idx].t = now ? now : 1;
        req[idx].n = granaries;
        if (ok) return;
    }
    req[idx].t = now ? now : 1;
    req[idx].n = granaries;
    bm_request(bm, UT(B_GRANARY));
    if (g_log) fprintf(g_log, "# fields: P%u at %.1f min: food target %d, %d on food, %d granaries - one more requested\n",
                       idx, now / 60000.0, want, have, granaries);
}
// CRASH GUARD: the build list can hold a field (0x2A) too - its re-queue asks for a destroyed one again - and the
// placement's field case (0x46580A) only orders builders to the foundation on that tile: with none there it makes
// EEUGBuildRepair for a null building and crashes (0x60BB19, seen once fields got built and then lost in battle).
// hook_place lets a field through only with a foundation on its tile - laid out first if the tile is free - and
// reports the others as placed (dropped).
internal int field_place_ok(ee_build_mgr *bm, int x, int y, int lay_out) {
    ee_player *pl = bm_get_player(bm);
    void *cm = bm_get_owner(bm);
    void *um = cm ? cm_get_unit_mgr(cm) : NULL;
    if (!pl || !um || (!x && !y)) return 0;
    uint8_t *dq = type_list(um, UT(B_FARM));
    for (unsigned k = 0, first = dq ? FIELD(dq, 0xC, unsigned) : 0, cnt = dq ? FIELD(dq, 0x10, unsigned) : 0; k < cnt; k++) {
        ee_unit *f = deque_at(dq, first + k);
        if (f && unit_get_hp(f) > 0 && unit_get_x(f) == x && unit_get_y(f) == y) return 1;
    }
    if (!lay_out || !ut_has(UT(B_FARM)) || !can_place(pl, UT(B_FARM), x, y)) return 0;
    place_foundation(pl, UT(B_FARM), x, y);
    if (g_log) fprintf(g_log, "# fields: P%d: build list field at %d,%d - foundation laid out first\n", player_get_index(pl), x, y);
    return 1;
}
internal int site_danger(ee_player *pl, ee_unit *site, int food);   // attack.c: known enemy towers on the way
internal void econ_fields(ee_resource_mgr *rm) {   // before the citizen assignment
    ee_player *pl = rm_player(rm);
    if (!g_build_fields || !pl) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    static uint32_t next[16];
    uint32_t now = game_time();
    if (now < next[idx] && next[idx] - now <= 5000) return;
    next[idx] = now + 5000;
    void *cm = rm_get_owner(rm);
    void *um = cm ? cm_get_unit_mgr(cm) : NULL;   // unit manager: per-type lists (0x48F8AF)
    uint8_t *dq = type_list(um, UT(B_FARM));
    if (um) fields_rebuild(pl, idx, um, dq, now);
    if (!dq) {   // no field yet (no list of fields): only the granary pass - it brings the first one (water maps)
        g_fields_open[idx] = 0;
        if (um && cm) more_granaries(pl, idx, rm, cm, um, 0, now);
        return;
    }
    ee_unit *todo[16], *done[16];
    int nt = 0, nd = 0, ordered = 0, farmers = 0;
    for (unsigned k = 0, first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned); k < cnt && nt < 16; k++) {
        ee_unit *f = deque_at(dq, first + k), *st = f ? unit_get_physics(f) : NULL;
        if (!st || unit_get_hp(f) <= 0) continue;
        if (site_danger(pl, f, 1)) continue;   // under known enemy towers: no builders, no farmers (attack.c)
        // raided: a player doesn't send his citizens one by one into the raiders - the same distances as
        // citizens_safety (attack.c: 8 for builders, 6 for farmers): with 6 here a builder went every 10 s to a
        // field with bowmen 7 tiles away, was sent back 4 s later, and the next one went (the player's army shot
        // them on the way)
        if (raiders_near(pl, unit_get_x(f), unit_get_y(f), unit_get_finished(st) ? 6 : 8))
            continue;
        if (!unit_get_finished(st)) todo[nt++] = f;
        else if (nd < 16 && !field_worked(rm, f)) done[nd++] = f;
    }
    g_fields_open[idx] = nt;
    more_granaries(pl, idx, rm, cm, um, nt + nd, now);
    int wood = player_get_stock(pl, 3);
    int wild = nt && food_fields_wait(pl, rm);   // food.c (food_plan WILD): wild food first
    for (int i = 0; i < nt && wood >= 50 && !wild; i++) {
        ee_unit *site = field_site(todo[i]), *best = NULL;
        int fx = unit_get_x(todo[i]), fy = unit_get_y(todo[i]), bd = 0x7FFFFFFF, busy = 0;
        for (int r = 0; r < 7 && !busy; r++) {   // a builder already on it?
            uint8_t *list = rm_list_at(rm, r);
            for (unsigned k = 0, first = FIELD(list, OF(EPOCHTECHS, 0xC), unsigned), cnt = FIELD(list, OF(EPOCHTECHS, 0x10), unsigned); k < cnt; k++) {
                ee_unit *u = deque_at(list, first + k); ee_goal *goal = u ? unit_get_goal(u) : NULL;
                if (goal && goal_get_building(goal) == site && !strcmp(rtti_name(goal), "EEUGBuildRepair@@")) { busy = 1; break; }
            }
        }
        if (busy) continue;
        for (int pass = 0; pass < 3 && !best; pass++)   // idle, on food, on stone/gold/iron - never on wood
            for (int r = 0; r < 7; r++) {
                if (pass == 1 ? r != 2 : pass == 2 ? r < 4 : 0) continue;
                uint8_t *list = rm_list_at(rm, r);
                for (unsigned k = 0, first = FIELD(list, OF(EPOCHTECHS, 0xC), unsigned), cnt = FIELD(list, OF(EPOCHTECHS, 0x10), unsigned); k < cnt; k++) {
                    ee_unit *u = deque_at(list, first + k);
                    if (!citizen_free(u, pass == 0)) continue;
                    int dx = unit_get_x(u) - fx, dy = unit_get_y(u) - fy, d = dx * dx + dy * dy;
                    if (d < bd) { bd = d; best = u; }
                }
            }
        if (!best) break;
        void *mem = goal_build_alloc();
        if (!mem) break;
        ee_goal *goal = goal_build_init(mem, pl, todo[i], 1);
        unit_set_goal(best, goal, 0, 0);
        unit_say(best, 0x1F51);
        ee_unit_type *td = unit_get_type_data(best);
        (*(void (__thiscall **)(void *, void *))(type_get_vtable(td) + 0x74))(td, best);
        ordered++;
        wood -= 50;
    }
    uint32_t *target = rm_target_at(rm);
    // no more farmers than food's target: with the food pile's ceiling (late_targets bit 8) fields stand idle on
    // purpose - filled again from the mines here, the farmers the rebalance had moved came straight back
    int room = (int)target[2] - rm_count(rm, 2);
    for (int i = 0; i < nd && farmers < room; i++) {   // farmers for finished fields
        ee_unit *best = NULL;
        int fx = unit_get_x(done[i]), fy = unit_get_y(done[i]), bd = 0x7FFFFFFF;
        for (int pass = 0; pass < 2 && !best; pass++)
            for (int r = pass ? 4 : 0; r < 7; r++) {
                if (pass && rm_count(rm, r) <= (int)target[r]) continue;   // only a surplus
                uint8_t *list = rm_list_at(rm, r);
                for (unsigned k = 0, first = FIELD(list, OF(EPOCHTECHS, 0xC), unsigned), cnt = FIELD(list, OF(EPOCHTECHS, 0x10), unsigned); k < cnt; k++) {
                    ee_unit *u = deque_at(list, first + k);
                    if (!citizen_free(u, pass == 0)) continue;
                    int dx = unit_get_x(u) - fx, dy = unit_get_y(u) - fy, d = dx * dx + dy * dy;
                    if (d < bd) { bd = d; best = u; }
                }
            }
        if (!best) break;
        rm_assign(rm, best, done[i]);
        farmers++;
    }
    if (farmers && g_log)
        fprintf(g_log, "# fields: P%u at %.1f min: %d finished fields without a farmer, %d farmers sent\n", idx, now / 60000.0,
                nd, farmers);
    if (ordered && g_log)
        fprintf(g_log, "# fields: P%u at %.1f min: %d unfinished fields, %d builders sent\n", idx, now / 60000.0,
                nt, ordered);
}
internal void econ_rebalance(ee_resource_mgr *rm) {
    ee_player *pl = rm_player(rm);
    if (!g_rebalance || !pl) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    uint32_t *target = rm_target_at(rm);
    int moves = 0, dry = 0, most = (PV(idx, g_v_mine_fill, g_mine_fill) & 1) || (PV(idx, g_v_cit_plan, g_cit_plan) & 1) ? 4 : 2;   // dry: donor lists without a movable citizen; mine_fill, citizen_plan: 4
    // wood too with late_targets (the game's own wood search, 0x48C060(1, unit), safe_sites' danger check): 38 on gold
    // above their target stayed there for good while wood had 3
    int first = (PV(idx, g_v_late, g_late_targets) & 2) && current_epoch(pl) >= FARM_EPOCH ? 3 : 4;
    for (int r = first; r < 7 && moves < most; r++) {
        while (moves < most && (int)target[r] - rm_count(rm, r) >= 2) {
            int d = -1, best = 2;
            for (int t = 2; t < 7; t++) {
                int s = rm_count(rm, t) - (int)target[t];
                if (t != r && !(dry >> t & 1) && s > best) { best = s; d = t; }
            }
            if (d < 0) break;
            ee_unit *u = movable_gatherer(rm, d);
            if (!u) { dry |= 1 << d; continue; }
            ee_unit *site = r == 3 ? hook_find_wood(rm, NULL, 1, u)
                                : hook_find_mine(rm, NULL, r, u, 1);   // with mine_choice: nearest to a drop site
            if (!site) break;   // no free, safe mine of this resource
            int before = rm_count(rm, r);
            rm_assign(rm, u, site);
            if (rm_count(rm, r) == before) { dry |= 1 << d; continue; }
            g_moved[idx][r]++;
            moves++;
        }
    }
}

// INCOME RATES per AI and resource (every 30 s, smoothed over ~2 minutes): per minute in total and per worker-minute
// (only with 2+ workers on it; defaults from the suites until then). For the reserve plan and the worker plan.
internal void rates_update(ee_player *pl, unsigned idx, uint32_t now) {
    if (g_rate[idx].t && now >= g_rate[idx].t && now - g_rate[idx].t < 30000) return;
    if (!g_rate[idx].t || now < g_rate[idx].t) {   // first look / new game
        memset(&g_rate[idx], 0, sizeof g_rate[idx]);
        memcpy(g_rate[idx].per_worker, k_default_pw, sizeof k_default_pw);
        memcpy(g_rate[idx].last, g_bud[idx].income, sizeof g_rate[idx].last);
        g_rate[idx].t = now ? now : 1;
        return;
    }
    void *cm = player_get_civ_mgr(pl);
    ee_resource_mgr *rm = cm ? cm_get_resource_mgr(cm) : NULL;
    float min = (float)(now - g_rate[idx].t) / 60000.0f;
    for (int r = 2; r < 7; r++) {
        float inc = (float)(g_bud[idx].income[r] - g_rate[idx].last[r]);
        g_rate[idx].last[r] = g_bud[idx].income[r];
        g_rate[idx].rate[r] = g_rate[idx].rate[r] * 0.75f + 0.25f * inc / min;
        int n = rm ? rm_count(rm, r) : 0;
        if (n >= 2) g_rate[idx].per_worker[r] = g_rate[idx].per_worker[r] * 0.75f + 0.25f * inc / (min * (float)n);
    }
    g_rate[idx].t = now;
}

internal int g_reserve_window_ms = 5 * 60000;   // aimod.ini [budget] reserve_window_min
internal void budget_reserve(ee_player *pl, int reserve[7]) {
    memset(reserve, 0, 7 * sizeof *reserve);
    if (follow_held(pl)) return;   // epoch_pacing=follow: nothing held while it may not advance
    ee_tech_tree *tt = player_get_tech_tree(pl);
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    if (!tt || !cm) return;
    int next = FIELD(tt, TT_EPOCH, int) + 1;
    if (next > 13 || next > end_epoch()) return;   // end epoch of this game
    uint8_t *data = FIELD(FIELD_AT(tt, 8 + next * 0x54), OF(TTREC, 0x8), uint8_t *);
    if (!data) return;
    if (copper_mode(pl) && next <= FARM_EPOCH) {   // copper_plan=2: only the next epoch's
        for (int r = 2; r < 7; r++)
            for (int e = next; e <= (copper_mode(pl) == 2 ? next : FARM_EPOCH); e++) reserve[r] += epoch_cost(pl, e, r);
        return;
    }
    int remaining = (int)(cm_get_timer(cm, 0) - game_time());
    float f = remaining <= 0 ? 1.0f : remaining >= g_reserve_window_ms ? 0.0f
            : 1.0f - (float)remaining / (float)g_reserve_window_ms;
    for (int r = 2; r < 7; r++) {
        int c = FIELD(data, OF(TECHDATA, 0x24) + (r - 2) * 4, int);
        reserve[r] = c > 0 ? (int)(c * f) : 0;
    }
}

// RESERVE PLAN (aimod.ini [all] reserve_plan, default on) - what spending sees instead of budget_reserve: like a
// player waiting for the slowest resource of the next epoch, the AI holds back only what it can't gather again in
// time - from the Copper Age on, when farms make food renewable. Requirement N: the next epoch's cost within 10
// minutes of the planned attempt (cm+0x58; nothing before). Horizon H = the time until the attempt or until the
// slowest missing resource is in (shortfall / income, at most 10 min), whichever is later. Each resource keeps
// N - 80% of what its income brings in within H: food spent on citizens while gold and iron are still missing
// grows back by the time they are ready; the bottleneck stays reserved. Before farms the full copper-plan reserve
// stays: wild food is finite (suites: releasing it there delayed the Copper Age by 1.5-3 minutes).
internal void spend_reserve(ee_player *pl, unsigned idx, int reserve[7], float *horizon) {
    if (horizon) *horizon = -1;
    if (!g_reserve_plan) { budget_reserve(pl, reserve); return; }
    memset(reserve, 0, 7 * sizeof *reserve);
    if (follow_held(pl)) return;
    ee_tech_tree *tt = player_get_tech_tree(pl);
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    if (!tt || !cm) return;
    int ep = FIELD(tt, TT_EPOCH, int), next = ep + 1;
    if (next > 13 || next > end_epoch() || !epoch_tech(pl, next)) return;
    if (copper_mode(pl) && next <= FARM_EPOCH) { budget_reserve(pl, reserve); return; }
    int remaining = (int)(cm_get_timer(cm, 0) - game_time());
    if (remaining > 10 * 60000) return;
    float until = remaining > 0 ? (float)remaining / 60000.0f : 0;   // minutes until the attempt
    for (int r = 2; r < 7; r++) reserve[r] = epoch_cost(pl, next, r);
    int *stock = player_stock_at(pl);
    float t = 0;
    for (int r = 2; r < 7; r++) {
        int miss = reserve[r] - stock[r];
        if (miss <= 0) continue;
        float tr = g_rate[idx].rate[r] > 1.0f ? (float)miss / g_rate[idx].rate[r] : 10.0f;
        if (tr > t) t = tr;
    }
    if (t > 10) t = 10;
    float h = until > t ? until : t;
    if (horizon) *horizon = h;
    for (int r = 2; r < 7; r++) {
        float more = 0.8f * g_rate[idx].rate[r] * h;
        int keep = reserve[r] - (int)more;
        reserve[r] = keep < 0 ? 0 : keep;
    }
}

internal void trace_train(ee_player *pl, int *req, uintptr_t r, const int *before, const int *reserve);

internal int unit_cost(ee_player *pl, int type, int r);

// Military request statistics, logged every 5 minutes: how often the AI asks for soldiers, how many it gets
// and which resource was short when a request failed (unit cost = object data 0x9418A0[type] +0x24..).
internal struct { unsigned req, trained, short_of[7], subs, saving; uint32_t last_log; int ftype[4]; unsigned fcount[4]; } g_mil[16];
internal unsigned mil_short(unsigned idx, int r) { return g_mil[idx & 15].short_of[r]; }
internal void mil_note(ee_player *pl, unsigned idx, const int *req, const int *visible, int spent, uintptr_t site) {
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
internal int unit_cost(ee_player *pl, int type, int r) {
    ee_unit_type *obj = type_data(type);
    ee_tech_tree *tt = player_get_tech_tree(pl);
    if (!obj || !tt || r < 2 || r > 6) return 0;
    ee_tech *tech = tech_lookup(tt, type_get_tech(obj));
    uint8_t *data = tech ? FIELD(tech, OF(TECH, 0x8), uint8_t *) : NULL;
    if (!data) return 0;
    float c = (float)FIELD(data, OF(TECHDATA, 0x24) + (r - 2) * 4, int);
    if (player_get_cheat(pl)) {
        int d = player_get_difficulty(pl);
        float pct = d == 0 ? (float)player_get_cost_easy(pl) : d == 2 ? (float)player_get_cost_hard(pl) : player_get_cost_medium(pl);
        c = c * pct * 0.01f;
    }
    return (int)c;
}

// UNIT SUBSTITUTION (aimod.ini [all] unit_substitution, default on): the AI's unit choice ignores what it can pay
// for and what it can still train - in the Copper Age P3 asked for the Golden Hind (a hero ship, 300 wood), Sampson
// (a hero, 65 gold), Incan Warriors and Simple Bowmen (gold), Clubmen and Rock Throwers (upgraded away) - hundreds
// of failed requests every 5 minutes, 10-12 soldiers at minute 45 with 2900 food and 400-600 iron in the military
// account. A military request (no ship, siege, ram) the budget refuses or the game can't train (0x555710) trains
// the strongest land soldier (family 5, no siege/ram/citizen/priest) (hit points x attack) that the budget pays for and the game can
// train right now instead - a player builds what his resources allow. Candidates: researched land soldiers, per
// player, refreshed every 30 s. Counted in the 5-minute army line ("substituted"). Never one without an attack: the
// Domestic Wolf (615, a leftover of the game's data: attack 0, food only) was the last candidate and so the one
// left whenever iron ran short - in the World War I age an army of 45 wolves that did no damage.
// SAVING FIRST ([all] substitute_wait, seconds, default 90; 0 = at once): a unit the game can train but the budget
// can't pay yet is saved for - substituting at once spent what was being saved on something weaker, and the better
// unit never came. Only once the AI has saved for that long without training a requested soldier (no iron coming
// in at all), or with enemy soldiers in its base (it needs soldiers now), does it train the substitute. A unit the
// game can't train (not researched, no building) is substituted at once - waiting doesn't bring it.
internal int g_unit_sub = 1, g_sub_wait = 90;
internal uint32_t g_save_since[16];   // game time it started saving (a request refused by the budget), 0 = not saving
internal int base_threatened(unsigned idx);   // diplo.c: enemy soldiers in its base
internal struct { ee_player *pl; uint32_t t; int n; int type[48]; } g_cand[16];
internal int land_soldier(int type) {
    ee_unit_type *td = type_data(type);
    if (!td || type_get_id(td) != type || type_get_family(td) != 5) return 0;   // a real record of this type
    if (ut_role(type, UTR_SIEGE | UTR_RAM | UTR_CITIZEN) || type == UT(PRIEST) || type == UT(PROPHET)) return 0;
    return 1;
}
// Requests that may be replaced: every military unit but ships (navies on water maps), siege and rams (the siege
// controller's business) - heroes, priests and the like included. Ships are every water family (+0x4B4, docs/navy.md):
// 4 warships, 10 submarines, 18 transports, 21 carriers, 22 fishing boats. Only 4 was excluded before: a fishing boat
// the budget refused (wood short: 109 failed 72 times in the island baseline) was saved for 90 s and then trained as
// a land soldier, and so could a transport, a submarine or a carrier.
internal int substitutable(int type) {
    ee_unit_type *td = type_data(type);
    if (!td || type_get_id(td) != type) return 0;
    int fam = type_get_family(td);
    if (fam == 4 || fam == 10 || fam == 18 || fam == 21 || fam == 22) return 0;
    if (ut_role(type, UTR_SIEGE | UTR_RAM | UTR_CITIZEN)) return 0;
    return 1;
}
internal int soldier_score(int type) {
    ee_unit_type *td = type_data(type);
    return type_get_hp(td) * type_get_attack(td);
}
internal int g_sub_spare_food;   // train_budget's early_army substitution (below)
internal int g_sub_thrift;       // train_budget's workforce_plan THRIFT substitution: the most score per gold + iron
internal int wf_thrift(ee_player *pl, unsigned idx, int type, const int *vis);   // citizens.c
internal void wf_thrift_note(unsigned idx, int type, int n);
internal int mt_choose(ee_player *pl, unsigned idx, int asked, const int *vis, const int *stock, const int *reserve, int want,
                     int blocked, int trainable, int *got);   // metal.c metal_plan VALUE
internal int mt_nowait(ee_player *pl, unsigned idx, int type, const int *vis, int wait);   // NOWAIT
internal void mt_saving(unsigned idx);
internal int g_hg_call;          // set by the home guard (defence.c) around its order: early_soldiers in the first epoch
internal int substitute_soldier(ee_player *pl, unsigned idx, const int *vis, int want, int *got) {
    uint32_t now = game_time();
    ee_tech_tree *tt = player_get_tech_tree(pl);
    if (!tt) return 0;
    if (g_cand[idx].pl != pl || now < g_cand[idx].t || now - g_cand[idx].t > 30000) {
        g_cand[idx].pl = pl; g_cand[idx].t = now; g_cand[idx].n = 0;
        for (int t = 0; t < 724; t++) {   // type ids end at 723 (data/db/converteroutput.log); the table ends there
            if (!land_soldier(t) || soldier_score(t) <= 0) continue;   // no attack: never a soldier
            ee_tech *tech = tech_lookup(tt, type_get_tech(type_data(t)));
            if (!tech || !FIELD(tech, OF(TECH, 0x4), uint8_t)) continue;
            int n = g_cand[idx].n, k = n < 48 ? n : 47;   // insert sorted by score, strongest first
            while (k > 0 && soldier_score(g_cand[idx].type[k - 1]) < soldier_score(t)) {
                if (k < 48) g_cand[idx].type[k] = g_cand[idx].type[k - 1];
                k--;
            }
            if (k < 48) g_cand[idx].type[k] = t;
            if (n < 48) g_cand[idx].n++;
        }
    }
    int best = 0, bn = 0;
    float bs = 0;
    for (int i = 0; i < g_cand[idx].n; i++) {
        int t = g_cand[idx].type[i], n = want, paid = 0;
        for (int r = 2; r < 7; r++) {
            int c = unit_cost(pl, t, r);
            if (c > 0) { paid = 1; if (vis[r] / c < n) n = vis[r] / c; }
        }
        if (!paid || n <= 0 || !can_build(pl, t)) continue;
        if (g_sub_thrift) {   // THRIFT: score per gold + iron (at least 10), the pile pays the rest
            float s = (float)soldier_score(t) / (float)(unit_cost(pl, t, 5) + unit_cost(pl, t, 6) > 10 ? unit_cost(pl, t, 5) + unit_cost(pl, t, 6) : 10);
            if (s > bs) { bs = s; best = t; bn = n; }
            continue;
        }
        if (!g_sub_spare_food) { *got = n; return t; }
        // early_army: the food-free ranged soldiers first (Slinger, Simple Bowman, Javelin, Composite Bow, Long Bow ...)
        // - score x2 with a range, a quarter with a food price: the citizens keep the food while the economy grows
        float s = (float)soldier_score(t) * (type_get_range(type_data(t)) > 2.0f ? 2.0f : 1.0f) *
                  (unit_cost(pl, t, 2) > 0 ? 0.25f : 1.0f);
        if (s > bs) { bs = s; best = t; bn = n; }
    }
    if (best) *got = bn;
    return best;
}

// The budget decides before the game does: a request it doesn't allow is refused here and the original is not
// called. (Hiding stock during the call leaked: 0x49014F counts the unit as "in production" (unit manager
// +0x13C) even when the building queue 0x5E8908 then refuses it for lack of resources, and that counter is
// subtracted from the population cap - after a while the AI could train nothing at all.)
internal int g_priority_mil;   // set by the siege controller around its own requests
internal int g_landing_order;   // set by the landing around its transport order (its reserve is not held from it)
internal int g_expand_order;    // expand.c EARLY: the outpost's transport before every reserve (250 wood; the reserves held it 6 minutes)
internal void landing_reserve(ee_player *pl, int reserve[7]);   // landing.c
internal void expand_reserve(ee_player *pl, int reserve[7]);    // expand.c
internal int landing_room_reserve(ee_player *pl);   // landing.c: population room held for a bombardment's fleet
// ROOM: the game's own callers ask the unit manager for room first (cap - in production - population, the start check
// of the buildings 0x624DF8 the same); the mod's own orders (AA units, siege, home guard, transports) didn't. At the cap
// they were queued and paid for and could never come out: lowpop (cap 30) - P3's "in production" ran to 56 after it
// got resources, 63 units of AA tanks (146) and siege (105) ordered with no room. Now cut to the room (none: refused).
// [all] train_room: a mask of players (default all; tests: 8 = P3 only). lowpop, 20 seeds, P3 without vs with (P2
// without): P3 kills 23.4 vs 28.2, losses 32.2 vs 28.9 (P2's losses 22.3 vs 28.6). The AI's cap follows the human's
// population: at the cap is common in normal games too.
internal int g_train_room = 0xFFFF;
internal int g_am_noroom;   // army.c: the last request was cut to no room
internal void am_train_note(ee_player *pl, unsigned idx, const int *req, int blocked, int sub_type, unsigned n, const int *stock, const int *vis);
internal unsigned g_noroom[16][6];   // units ordered with no room (cap - in production - population <= 0), by call site
// the budget's part of a training request: what the AI may spend on it (visibles), how many it pays for (afford,
// blocked when none), a substitute soldier (sub_type) - reserve gets the stock held back
internal void train_budget(ee_player *pl, unsigned idx, const struct ai_cfg *cfg, const int *req, int military, int req_unit, int ship,
                         int *stock, int *reserve, int *visibles, int *blocked, int *afford, int *sub_type) {
    uint32_t now = game_time();
    if (now < g_bud[idx].last_t) g_bud[idx].init = 0;   // new game
    g_bud[idx].last_t = now;
    spend_reserve(pl, idx, reserve, NULL);
    reserve[3] += field_wood(idx);   // the next fields' wood (build_fields)
    landing_reserve(pl, reserve);   // transports a landing waits for
    expand_reserve(pl, reserve);    // an outpost's drop site and tower (expand.c)
    bunker_reserve(pl, reserve);    // a fortress's wood (bunker.c)
    if (!ship && !req_unit && !g_priority_mil && !g_landing_order) navy_hold(pl, idx, reserve);   // the navy account: only warships spend it (navy.c)
    if (!g_bud[idx].init) {
        memset(&g_bud[idx], 0, sizeof g_bud[idx]);
        g_bud[idx].init = 1;
        g_bud[idx].last_t = now;
        for (int r = 2; r < 7; r++)
            g_bud[idx].mil[r] = stock[r] > reserve[r] ? (stock[r] - reserve[r]) * cfg->mil_start_share : 0;
    }
    int no_work = 0;
    if (!military) {
        void *cm = player_get_civ_mgr(pl);
        ee_resource_mgr *rm = cm ? cm_get_resource_mgr(cm) : NULL;
        int total = 0;
        for (int t = 0; rm && t < 7; t++) total += FIELD(rm_list_at(rm, t), 0x10, int);
        int idle = idle_citizens(pl);
        // never starve a small population; the first assignment pass runs only every 10 s
        // only matters while food is scarce: idle citizens are cheap (and builders) when it is plentiful
        int scarce = stock[2] < 2 * reserve[2] + 500;
        no_work = scarce && total >= 10 && idle > g_citizen_slack && idle * 100 > total * 15;
    }
    int ep = current_epoch(pl), cmode = copper_mode(pl), cit = is_citizen_type(req[0]);
    // EARLY ARMY (early_army, docs/economy.md; the playtester: "I don't train military in the first epoch and rely on
    // towers and citizens, but I go heavy on bowmen/rock slingers from the second epoch on"): before farms the
    // military account is the army's own money - it spends the account whatever the epoch's reserve (that comes out
    // of the rest), and the citizens never spend the account's food (they emptied the stock first: the account held
    // 500-780 food, 0 soldiers until minute 22). No army in the first epoch (requirement units, siege and ships
    // aside; early_soldiers: the home guard's few).
    int early = EARLY(idx, early_army) && ep < FARM_EPOCH;
    int no_army = early && military && !ship && !req_unit && !g_priority_mil && ep == 0 &&
                  !(EARLY(idx, early_soldiers) && g_hg_call);
    for (int r = 2; r < 7; r++) {
        int visible = req_unit || (ship && navy_earmarked(idx)) ? stock[r] : stock[r] - reserve[r];   // a requirement unit: the epoch waits for it
        // citizens before farms: they pay only food, and only what the food budget (stock + reachable wild
        // food) doesn't need for the epochs; a tiny workforce always grows
        if (cit && cmode == 1 && ep < FARM_EPOCH) {
            int wf = workforce(pl);
            if (r != 2 || wf < 10) visible = stock[r];
            else if (wf < g_min_workforce && stock[2] + (int)(wild_food(pl) * g_wild_share) >= reserve[2] + 500)
                visible = stock[2];   // enough food on the map: grow the economy first
            // else: workforce built (or the map is poor) -> save: the full reserve applies
        }
        if (cit && cmode == 2 && ep < FARM_EPOCH) {   // never stopped by the reserve: all but the epoch account
            int acc = g_epoch_acc_ep[idx] == ep + 1 ? (int)g_epoch_acc[idx][r] : 0;
            visible = stock[r] - acc;
            // unless the map's food runs out: wild food is all there is before farms (a Continental start with every
            // berry bush and animal gone at minute 12, 26 citizens, 252 food in the account for 45 minutes) - then the
            // epoch's food first, as the plan always did
            if (r == 2 && (int)(wild_food(pl) * g_wild_share) < epoch_cost(pl, ep + 1, 2) - acc + 500)
                visible = stock[2] - epoch_cost(pl, ep + 1, 2);
        }
        if (cit && r == 2) visible = cz_visible_food(pl, idx, stock, visible);   // citizen_plan FIRST (citizens.c)
        if (cit && early && g_bud[idx].mil[r] > 0) visible -= (int)g_bud[idx].mil[r];   // not the army's food
        if (military && !g_priority_mil && !req_unit) {   // priority requests (siege) and requirement units spend
                                                           // the stock above the reserve
            int allowed = ship ? navy_account(idx, r) : (int)g_bud[idx].mil[r];   // a warship: the navy account
            int excess = stock[r] - reserve[r] - (r >= 4 ? PV(idx, g_v_hoard_mineral, g_hoard_cap_mineral) : g_hoard_cap);   // never hoard beyond the cap
            if (excess > allowed) allowed = excess;
            if (early && !ship) visible = stock[r];   // the account is the army's own
            if (visible > allowed) visible = allowed;
        }
        if (g_expand_order) visible = stock[r];
        if (no_army) visible = 0;
        if (no_work) visible = 0;
        if (visible < 0) visible = 0;
        if (visible > stock[r]) visible = stock[r];
        visibles[r] = visible;
    }
    if (no_work) g_bud[idx].cit_blocked++;
    // as many of the requested units as the budget pays for (citizens come in batches of 10)
    int want = req[1] > 0 ? req[1] : 1;
    *afford = want;
    for (int r = 2; r < 7; r++) {
        int c = unit_cost(pl, req[0], r);
        if (c > 0 && visibles[r] / c < *afford) *afford = visibles[r] / c;
    }
    // copper_plan=2: while the epoch's cost is still being saved, only citizen_queue citizens paid ahead (the game asks
    // for 10 at a time and pays them all at once) - what the Town Center can't train yet stays in the stock
    if (cit && cmode == 2 && g_cit_queue > 0 && ep < FARM_EPOCH && stock[2] < reserve[2]) {
        void *cm = player_get_civ_mgr(pl), *um = cm ? cm_get_unit_mgr(cm) : NULL;
        int room = g_cit_queue - (um ? um_get_in_production(um) : 0);
        if (*afford > room) *afford = room < 0 ? 0 : room;
    }
    *blocked = *afford <= 0;
    if (no_army) return;
    int trainable = military && g_unit_sub && !g_priority_mil && substitutable(req[0]) &&
                    can_build(pl, req[0]);
    int sub = military && g_unit_sub && !g_priority_mil && substitutable(req[0]) && (*blocked || !trainable);
    // early_army: from the second epoch, while the economy grows, a soldier with a food price is replaced by the best
    // food-free (ranged first) one the account pays for (substitute_soldier's weighting) - at once, no saving first
    g_sub_spare_food = EARLY(idx, early_army) && ep >= 1 && (ep < FARM_EPOCH || workforce(pl) < g_early_workers) &&
                       military && !ship && !req_unit && g_unit_sub && !g_priority_mil && substitutable(req[0]);
    if (g_sub_spare_food && unit_cost(pl, req[0], 2) > 0) sub = 2;
    // workforce_plan THRIFT (citizens.c): food and wood pile while the refused soldier waits for gold or iron - the
    // soldier that does most per gold and iron at once, no saving (the gold went to the warships and techs meanwhile)
    g_sub_thrift = sub == 1 && *blocked && !ship && !req_unit && wf_thrift(pl, idx, req[0], visibles);
    // metal_plan VALUE (metal.c): a land soldier the budget pays is trained as the one that does most against the
    // enemy it knows per gold + iron, when that one is 25% better
    int value = sub != 2 && !g_sub_thrift && military && !ship && !req_unit && g_unit_sub && !g_priority_mil && land_soldier(req[0]);
    if (value && !sub) {
        int got = 0, alt = mt_choose(pl, idx, req[0], visibles, stock, reserve, want, 0, trainable, &got);
        if (alt) { *sub_type = alt; *afford = got; g_mil[idx].subs++; g_sub_spare_food = 0; return; }
    }
    if (sub == 1 && trainable && !g_sub_thrift) {   // the budget refused a unit it can train: save for it first
        if (!g_save_since[idx] || now < g_save_since[idx]) g_save_since[idx] = now ? now : 1;
        int sw = PV(idx, g_v_sub_wait, g_sub_wait);
        // metal_plan NOWAIT: not for a unit whose gold or iron won't come within the wait
        sub = !sw || now - g_save_since[idx] >= (uint32_t)sw * 1000u || base_threatened(idx) || mt_nowait(pl, idx, req[0], visibles, sw);
        if (!sub) { g_mil[idx].saving++; mt_saving(idx); }
    }
    if (sub) {
        int got = 0, alt = value ? mt_choose(pl, idx, req[0], visibles, stock, reserve, want, 1, 0, &got) : 0;
        if (!alt) alt = substitute_soldier(pl, idx, visibles, want, &got);
        if (alt) { *sub_type = alt; *afford = got; *blocked = 0; g_mil[idx].subs++; if (g_sub_thrift) wf_thrift_note(idx, alt, got); }
    }
    g_sub_spare_food = g_sub_thrift = 0;
}
// the request as the budget allows it (blocked: none) through the game's own training, cut to the room under the
// population cap for the mod's own orders; ra = the call site (a game's or the mod's); the game's result
internal uintptr_t train_call(void *um, ee_player *pl, unsigned idx, int *req, uintptr_t b, int blocked, int afford, int sub_type,
                            uintptr_t ra, int military, int ship) {
    uintptr_t res = 0;
    g_am_noroom = 0;
    if (blocked) { if (b) *(unsigned *)b = 0; }
    else {
        int saved_count = req[1], saved_type = req[0];
        if (sub_type) req[0] = sub_type;
        if (afford > 0 && (afford < req[1] || sub_type)) req[1] = afford;   // the caller reads back how many via *b
        int room0 = pl ? unit_limit(pl) - um_get_in_production(um) -
                         population(pl, 1, 0) : 1;
        int game_site = train_site(ra) >= 0;
        unsigned got0 = 0;
        if (!b) b = (uintptr_t)&got0;
        if (!game_site && (g_train_room >> idx & 1) && room0 < (req[1] > 0 ? req[1] : 1)) {   // the mod's own order: no more than the room
            if (room0 <= 0) { *(unsigned *)b = 0; req[1] = saved_count; req[0] = saved_type; g_am_noroom = 1; return 0; }
            req[1] = room0;
        }
        int hold = military && !g_priority_mil && !g_landing_order && pl ? landing_room_reserve(pl) : 0;
        if (military && !ship && !g_priority_mil && !g_landing_order && pl) hold += navy_room_reserve(pl);   // the fleet's (navy.c)
        if (hold > 0 && room0 - hold < (req[1] > 0 ? req[1] : 1)) {   // the fleet's room: other soldiers only beyond it
            if (room0 - hold <= 0) { *(unsigned *)b = 0; req[1] = saved_count; req[0] = saved_type; g_am_noroom = 1; return 0; }
            req[1] = room0 - hold;
        }
        res = um_train(um, req, b);
        if (room0 <= 0 && !*(unsigned *)b) g_am_noroom = 1;   // the game's own call at the cap
        if (room0 <= 0 && *(unsigned *)b) {
            int site = train_site(ra) >= 0 ? train_site(ra) : 5;
            g_noroom[idx][site] += *(unsigned *)b;
            if (site == 5 && g_log) fprintf(g_log, "# noroom: P%u %u x type %d from the mod's call at %p (room %d)\n", idx, *(unsigned *)b, req[0], (void *)ra, room0);
        }
        req[1] = saved_count;
        req[0] = saved_type;
    }
    return res;
}
// after the request: the military account pays what was spent, the notes and the budget line (once a minute)
internal void train_account(ee_player *pl, unsigned idx, const struct ai_cfg *cfg, int *req, uintptr_t res, int military, int ship,
                          int sub_type, int *stock, int *before, int *reserve, int *visibles, uintptr_t ra) {
    if (stock) {
        int any = 0, spent[7] = { 0 };
        for (int r = 2; r < 7; r++) {
            spent[r] = before[r] - stock[r];
            any |= spent[r] > 0;
            if (cfg->budget && military && !ship && spent[r] > 0) { g_bud[idx].mil[r] -= spent[r]; g_bud[idx].mil_spent[r] += spent[r]; }
        }
        if (ship) navy_paid(idx, req[0], spent, any);
        if (military && any && !sub_type) g_save_since[idx] = 0;   // a requested soldier trained: saved enough
        if (military) mil_note(pl, idx, req, visibles, any, ra);
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
}
// noinline: it reads its own return address (the call site) - also when the mod calls it
__attribute__((noinline)) internal uintptr_t __fastcall hook_train_units(ee_unit_mgr *um, void *edx, int *req, uintptr_t b) {
    (void)edx;
    ee_player *pl = um_owner(um);
    if (g_vanilla_mask) {   // a vanilla player's unit manager (before its first update too: the game's player array)
        void *own = pl;
        for (unsigned i = 1, n = game_player_count(); !own && i < n && i < 16; i++) {
            void *p = is_vanilla_idx((int)i) ? game_players()[i] : NULL, *cm = p ? player_get_civ_mgr(p) : NULL;
            if (cm && cm_get_unit_mgr(cm) == um) own = p;
        }
        if (is_vanilla(own)) {
            unsigned vn = 0;
            uintptr_t vb = b ? b : (uintptr_t)&vn;
            int vreq0 = req[0], vreq1 = req[1];
            uintptr_t r = um_train(um, req, vb);
            if (own && !is_citizen_type(vreq0)) {   // army.c: the vanilla side's requests too (blocked: never)
                int q[2] = { vreq0, vreq1 };
                g_am_noroom = 0;
                am_train_note(own, (unsigned)player_get_index(own) & 15, q, 0, 0, *(unsigned *)vb, NULL, NULL);
            }
            return r;
        }
    }
    int *stock = pl ? player_stock_at(pl) : NULL;
    int before[7] = { 0 }, reserve[7] = { 0 }, visibles[7] = { 0 }, blocked = 0, afford = 0, sub_type = 0;
    if (stock) memcpy(before, stock, sizeof before);
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    uintptr_t ra = (uintptr_t)__builtin_return_address(0) - 5;   // the call site: read here, in the hook
    // the fleet mix (navy.c): a warship the game asks for at its own sites is trained as the type the mix wants; the
    // request entry (req points into the unit manager's list) gets its own type back afterwards
    int asked = req[0], pick = 0, game_site = train_site(ra) >= 0;
    if (pl && game_site && !g_priority_mil && !g_landing_order) pick = navy_pick(pl, idx, asked);
    if (pick) req[0] = pick;
    // fishing boats and the landing's transports: economy (navy.c); warships on a water map: the navy account
    int military = !is_citizen_type(req[0]) && !(pl && navy_economy(idx, req[0], g_landing_order));
    int ship = military && pl && navy_pays(idx, req[0]);
    int req_unit = military && pl && requirement_unit(pl, req[0]);   // counts for the next epoch
    const struct ai_cfg *cfg = pl ? cfg_of(pl) : NULL;
    if (stock && cfg->budget) train_budget(pl, idx, cfg, req, military, req_unit, ship, stock, reserve, visibles, &blocked, &afford, &sub_type);
    else if (stock) memcpy(visibles, stock, sizeof visibles);
    if (stock && !military && is_citizen_type(req[0])) cz_budget_note(pl, idx, req, stock, visibles, blocked, cfg->budget ? afford : req[1]);
    unsigned got_local = 0;
    if (!b) b = (uintptr_t)&got_local;   // train_call did the same; army.c reads the count
    int req0 = req[0], req1 = req[1];
    uintptr_t res = train_call(um, pl, idx, req, b, blocked, afford, sub_type, ra, military, ship);
    if (pl && military) { int q[2] = { req0, req1 }; am_train_note(pl, idx, q, blocked, sub_type, *(unsigned *)b, stock, visibles); }
    train_account(pl, idx, cfg, req, res, military, ship, sub_type, stock, before, reserve, visibles, ra);
    if (pick) { navy_pick_done(pl, idx, pick, asked, before, stock, blocked); req[0] = asked; }
    return res;
}

internal void econ_install(const char *aimod_ini, const char *autotest_ini) {
    settings_load(aimod_ini, autotest_ini);
    sites_install(aimod_ini, autotest_ini);
    // finer tuning, same for all difficulties ([all] section; autotest may override)
    g_citizen_slack = cfg_int(aimod_ini, autotest_ini, 1, "idle_citizen_slack", 9999);
    g_hoard_cap = cfg_int(aimod_ini, autotest_ini, 1, "hoard_cap", 3000);
    g_hoard_cap_mineral = cfg_int(aimod_ini, autotest_ini, 1, "hoard_cap_mineral", 200);
    g_min_workforce = cfg_int(aimod_ini, autotest_ini, 1, "min_workforce", 40);
    g_wild_share = cfg_share(aimod_ini, autotest_ini, 1, "wild_food_share", 0.6, 0, 1);
    g_reserve_window_ms = cfg_int(aimod_ini, autotest_ini, 1, "reserve_window_min", 5) * 60000;
    g_rebalance = cfg_int(aimod_ini, autotest_ini, 1, "rebalance_workers", 1);
    g_late_targets = cfg_int(aimod_ini, autotest_ini, 1, "late_targets", 7);
    g_reserve_plan = cfg_int(aimod_ini, autotest_ini, 1, "reserve_plan", 1);
    g_mineral_margin = cfg_int(aimod_ini, autotest_ini, 1, "mineral_worker_margin", 300);
    g_early_requirements = cfg_int(aimod_ini, autotest_ini, 1, "early_requirements", 1);
    g_build_fix = cfg_int(aimod_ini, autotest_ini, 1, "build_fix", 1);
    g_train_room = cfg_int(aimod_ini, autotest_ini, 1, "train_room", 0xFFFF);
    g_build_fields = cfg_int(aimod_ini, autotest_ini, 1, "build_fields", 1);
    g_paid_upgrades = cfg_int(aimod_ini, autotest_ini, 1, "paid_upgrades", 1);
    if (autotest_ini) g_tech_log = ini_int("autotest", "tech_log", 0, autotest_ini);
    if (g_tech_log && build_is("aoc")) {   // AoC's vf6 takes an argument: the hook is the 2017 update's only yet
        if (g_log) fprintf(g_log, "# tech_log: free research not logged in this build\n");
    } else if (g_tech_log) {
        patch_tech_grant(hook_tech_grant);
    }
    g_more_granaries = cfg_int(aimod_ini, autotest_ini, 1, "more_granaries", 1);
    g_unit_sub = cfg_int(aimod_ini, autotest_ini, 1, "unit_substitution", 1);
    g_sub_wait = cfg_int(aimod_ini, autotest_ini, 1, "substitute_wait", 90);
    g_gather_research = cfg_int(aimod_ini, autotest_ini, 1, "gather_research", 1);
    g_early_towers = cfg_int(aimod_ini, autotest_ini, 1, "early_towers", 1);
    g_tower_bait = cfg_int(aimod_ini, autotest_ini, 1, "tower_bait", 1);
    g_early_militia = cfg_int(aimod_ini, autotest_ini, 1, "early_militia", 1);
    g_early_army = cfg_int(aimod_ini, autotest_ini, 1, "early_army", 0);
    g_early_soldiers = cfg_int(aimod_ini, autotest_ini, 1, "early_soldiers", 0);
    g_early_food_share = cfg_share(aimod_ini, autotest_ini, 1, "early_army_food_share", 0.10, 0, 1);
    g_early_workers = cfg_int(aimod_ini, autotest_ini, 1, "early_army_workers", 60);
    g_cit_queue = cfg_int(aimod_ini, autotest_ini, 1, "citizen_queue", 0);
    g_epoch_share = cfg_share(aimod_ini, autotest_ini, 1, "epoch_share", 0.40, 0, 1);
    g_food_floor = cfg_int(aimod_ini, autotest_ini, 1, "food_floor", 18);
    if (autotest_ini) {
        char vp[32];
        GetPrivateProfileStringA("autotest", "variant_players", "", vp, sizeof vp, autotest_ini);
        for (char *c = vp; *c; c++) if (*c >= '0' && *c <= '9') g_var_mask |= 1u << (*c - '0');
        g_v_early_towers = ini_int("autotest", "v_early_towers", -1, autotest_ini);
        g_v_tower_bait = ini_int("autotest", "v_tower_bait", -1, autotest_ini);
        g_v_early_militia = ini_int("autotest", "v_early_militia", -1, autotest_ini);
        g_v_early_army = ini_int("autotest", "v_early_army", -1, autotest_ini);
        g_v_early_soldiers = ini_int("autotest", "v_early_soldiers", -1, autotest_ini);
        g_v_copper = ini_int("autotest", "v_copper_plan", -1, autotest_ini);
        g_v_late = ini_int("autotest", "v_late_targets", -1, autotest_ini);
        g_v_hoard_mineral = ini_int("autotest", "v_hoard_cap_mineral", -1, autotest_ini);
        g_v_sub_wait = ini_int("autotest", "v_substitute_wait", -1, autotest_ini);
        if (g_var_mask && g_log)
            fprintf(g_log, "# variant: players %s play B: late_targets %d hoard_cap_mineral %d substitute_wait %d "
                    "(A: %d %d %d)\n", vp, g_v_late >= 0 ? g_v_late : g_late_targets, g_v_hoard_mineral >= 0 ? g_v_hoard_mineral :
                    g_hoard_cap_mineral, g_v_sub_wait >= 0 ? g_v_sub_wait : g_sub_wait, g_late_targets, g_hoard_cap_mineral,
                    g_sub_wait);
    }
    patch_cp_pay(hook_pay);
    int ok = patch_cp_add_resource(hook_add_resource), of = 1;
    patch_place(hook_place);
    int up_ok = patch_epoch_upgrades(hook_epoch_upgrades, hook_epoch_upgrades_aoc);
    if (g_log) fprintf(g_log, "# paid upgrades: %d (hook %d)\n", g_paid_upgrades, up_ok);
    int catch_ok = patch_epoch_catch_up(hook_epoch_catch_up);
    if (g_log) fprintf(g_log, "# epoch catch-up hook: %d\n", catch_ok);
    int pop_ok = patch_pop_cap(hook_pop_cap, NULL);
    if (g_log) fprintf(g_log, "# population limit (easy/medium/hard): %s/%s/%s (hooks %d/3)\n", g_cfg[0].pop_follow ? "follows the humans" : "game limit",
                       g_cfg[1].pop_follow ? "follows the humans" : "game limit", g_cfg[2].pop_follow ? "follows the humans" : "game limit", pop_ok);
    ok += patch_train_units(hook_train_units, &of);
    if (g_log) fprintf(g_log, "# economy hooks: %d/%d\n", ok, of);
}

// a new game (the game start hook, before persist_load restores g_bud): the last game's counts, times and slots go -
// the build fix's type slots filled up over a session, g_last_attack_time made a fake "# attack:" (and the chat line)
// at every game start, the income baselines of rates_update and gather_log came out negative
internal void econ_new_game(void) {
    memset(g_bud, 0, sizeof g_bud);
    memset(g_epoch_acc, 0, sizeof g_epoch_acc);
    memset(g_epoch_acc_ep, 0, sizeof g_epoch_acc_ep);
    memset(g_rate, 0, sizeof g_rate);
    memset(g_spend, 0, sizeof g_spend);
    memset(g_place, 0, sizeof g_place);
    memset(g_retry, 0, sizeof g_retry);
    memset(g_researched, 0, sizeof g_researched);
    memset(g_att, 0, sizeof g_att);
    memset(g_last_attack_time, 0, sizeof g_last_attack_time);
    memset(g_deco_done, 0, sizeof g_deco_done);
    memset(g_gather_income, 0, sizeof g_gather_income);
    memset(g_wood_want, 0, sizeof g_wood_want);
    memset(g_food_cap, 0, sizeof g_food_cap);
    memset(g_food_cap_t, 0, sizeof g_food_cap_t);
    memset(g_pull, 0, sizeof g_pull);
    memset(g_pull_t, 0, sizeof g_pull_t);
    memset(g_pull_seen, 0, sizeof g_pull_seen);
    memset(g_moved, 0, sizeof g_moved);
    memset(g_fields_open, 0, sizeof g_fields_open);
    memset(g_field_at, 0, sizeof g_field_at);
    memset(g_field_n, 0, sizeof g_field_n);
    memset(g_mil, 0, sizeof g_mil);
    memset(g_save_since, 0, sizeof g_save_since);
    memset(g_cand, 0, sizeof g_cand);
    memset(g_noroom, 0, sizeof g_noroom);
}
