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
// 3000 cap gold piled up (1000 in stock) while every soldier failed for gold; aimod.ini hoard_cap_mineral. 200 (was
// 600): ~600 iron lay in stock for the whole late game while the army failed for iron hundreds of times every 5
// minutes (antiair save: Brann's iron failures 816 -> 64; late-start A/B, 3 maps x 6 seeds: alive 78% vs 75%)
static int g_hoard_cap_mineral = 200;
// HEAD-TO-HEAD TESTS (autotest only): [autotest] variant_players = the player numbers playing variant B (e.g. 24);
// they use v_late_targets, v_hoard_cap_mineral, v_substitute_wait where set (the others the settings). AI-vs-AI
// suites with one variant for all measure how decisive the wars are, not which variant is stronger.
static unsigned g_var_mask;
static int g_v_late = -1, g_v_hoard_mineral = -1, g_v_sub_wait = -1;
#define PV(idx, v, g) ((g_var_mask >> ((idx) & 15) & 1) && (v) >= 0 ? (v) : (g))
static struct { uint32_t last_t; int init; float mil[7]; int income[7], mil_spent[7], cit_blocked, mil_limited; } g_bud[16];
// income rates per AI (rates_update): per minute and per worker-minute
static struct { uint32_t t; int last[7]; float rate[7], per_worker[7]; } g_rate[16];
static const float k_default_pw[7] = { 0, 0, 10, 10, 5, 8, 6 };
static void rates_update(void *pl, unsigned idx, uint32_t now);
static int g_reserve_plan = 1;   // aimod.ini [all] reserve_plan (see spend_reserve)
static int g_mineral_margin = 300;   // aimod.ini [all] mineral_worker_margin (econ_targets; -1 = off)

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
// RETRY (with build_fix): a dropped entry was gone for good - the planner never queued it again. Both AIs failed
// their AA gun (145) in the first minutes (refused with the resources there, 225 wood and 200 gold at minute 0.4)
// and had none for the rest of the game, against a player with aircraft. A dropped type (not the 395 settlement, not
// a decoration) is tried again every 2 minutes with the build manager's own placement (0x46580A, spot 0,0: the
// game's search) while the AI has none of it and can pay for it; given up after 5 failed retries.
static int defence_spot(void *pl, int type, int *ox, int *oy);   // defence.c
static void defence_tick(void *pl, unsigned idx, uint32_t now);
static void anti_air(void *pl, unsigned idx, uint32_t now);   // defence.c
static void bunker_reserve(void *pl, int *reserve);   // bunker.c
static void hold_town(void *pl, unsigned idx, uint32_t now);   // defence.c
static void attack_retreat(void *pl, unsigned idx, uint32_t now);   // defence.c
static void counter_siege(void *pl, unsigned idx, uint32_t now);
static void home_guard(void *pl, unsigned idx, uint32_t now);
static void muster(void *pl, unsigned idx, uint32_t now);       // defence.c: gather, then strike
static void diplo_tick(void *pl, unsigned ai, uint32_t now);
static void persona_attack_launched(unsigned ai);   // persona.c
#define CALL_PLACE 0x00468629
#define FN_PLACE   0x0046580A
static int g_build_fix = 1;
static struct { int type; unsigned tries, fails, dropped, streak; } g_place[16][64];   // every type the build lists hold
static int field_place_ok(uint8_t *bm, int x, int y);   // below, with the farm fields
static int upgraded_type(void *pl, int type);   // defence.c: the type an upgrade put in place of this one
static struct { int type; uint32_t next; uint8_t tries; } g_retry[16][16];   // dropped types, tried again
static int is_decoration(int type) {   // the editor's "x ..." objects (name at object data +0x1C)
    uint8_t *od = type >= 0 && type < 0x300 ? ((uint8_t **)0x009418A0)[type] : NULL;
    const char *name = od ? FIELD(od, 0x1C, const char *) : NULL;
    return name && name[0] == 'x' && name[1] == ' ';
}
static void retry_note(unsigned idx, int type, uint32_t now) {
    if (type == 395 || type == 0x2A || is_decoration(type)) return;
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
static char __fastcall hook_place(uint8_t *bm, void *edx, int type, int x, int y) {
    (void)edx;
    if (type == 0x2A && !field_place_ok(bm, x, y)) return 1;   // a field without foundation would crash (see there)
    int place_as = g_build_fix && type == 395 ? 43 : type;
    place_as = upgraded_type(FIELD(bm, 0x138, void *), place_as);   // queued before its upgrade: the new type
    if (!x && !y) defence_spot(FIELD(bm, 0x138, void *), type, &x, &y);   // towers: where the defence plan wants them
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
        if (type != 395) retry_note(idx, place_as, game_time());   // the 395 settlement arrives as 43: not retried
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

// GATHERING RESEARCH (aimod.ini [all] gather_research, default on): the research manager (cm+0x40) researches what is
// scheduled in its per-epoch lists (+0x18 + e*0x10); new techs come from a pool (+8) through a random planner
// (0x4858D9) that prefers gathering upgrades - but only while its flag +0x11A is set, and the AI's "attention" pass
// (0x42F912) switches on just player+0xE20 + 1 activities per period, research rarely among them. In the Bronze
// Age the AIs still had Hafts, Hand Axe, Metal Working and Hardened Chisel (75-150 food, 50-100 wood, +15% each)
// unresearched. Every 15 s one available (tech +6), unresearched (+4) pool tech with a gathering effect is scheduled
// with the planner's 0x485F57 (requests the building if missing, adds the cost to the demand) and taken out of the
// pool. Gathering effect: type 2 (+0x20), attribute +0x2C 0x1A..0x21 (wood, gold, stone, iron, ...), applied to
// citizens (+0x14 = 32/480) or to all (-1) - the planner's own test misses the -1 ones, which most of these are.
#define FN_RS_SCHEDULE 0x00485F57
static int workforce(void *pl);
static int g_gather_research = 1;
static unsigned g_researched[16];
static int is_gather_tech(uint8_t *tech) {
    uint8_t *d = FIELD(tech, 8, uint8_t *);
    int ei = d ? FIELD(d, 0x50, int) : -1;
    if (ei < 0 || ei > 0x5DC) return 0;
    uint8_t *list = (uint8_t *)0x0093B1B8 + ei * 0x10;
    unsigned n = FIELD(list, 8, unsigned), *ids = FIELD(list, 4, unsigned *);
    for (unsigned k = 0; ids && k < n && k < 64; k++) {
        if (ids[k] >= 2000) continue;
        uint8_t *ef = (uint8_t *)0x00919E70 + ids[k] * 0x44;
        int a = FIELD(ef, 0x2C, int), on = FIELD(ef, 0x14, int);
        if (FIELD(ef, 0x20, int) == 2 && (on == 0x20 || on == 0x1E0 || on == -1) && a >= 0x1A && a <= 0x21 && a != 0x1E) return 1;
    }
    return 0;
}
static void gather_research(void *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_gather_research || (now >= last[idx] && now - last[idx] < 15000)) return;
    last[idx] = now;
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *rs = cm ? FIELD(cm, 0x40, uint8_t *) : NULL, *tt = FIELD(pl, PL_TECHTREE, uint8_t *);
    if (!rs || !tt || workforce(pl) < 10) return;
    int *b = FIELD(rs, 8, int *), *e = FIELD(rs, 0xC, int *);
    for (int *q = b; b && q < e && e - b < 500; q++) {
        uint8_t *t = ((uint8_t *(__thiscall *)(void *, int))0x004019A1)(tt, *q);   // FN_TECH_LOOKUP
        if (!t || t[4] || !t[6] || !is_gather_tech(t)) continue;
        int id = *q;
        memmove(q, q + 1, (size_t)(e - q - 1) * sizeof *q);   // out of the pool, as the planner does
        FIELD(rs, 0xC, int *) = e - 1;
        ((void (__thiscall *)(void *, int))FN_RS_SCHEDULE)(rs, id);
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
static int g_paid_upgrades = 1;
static void __fastcall hook_epoch_upgrades(uint8_t *node, void *edx, int cat) {
    (void)edx;
    uint8_t *tt = FIELD(node, 0xC, uint8_t *);
    int pi = tt ? FIELD(tt, 4, int) : -1;
    // the game's own player array, not g_players: back in the scenario editor after a test run the editor sets the
    // computer players' epochs again (here) while g_players still held the test run's deleted players - the game
    // crashed reading one (eeaimod.dll+5e188)
    unsigned np = *(unsigned *)0x009183CC;
    void *pl = pi > 0 && pi < 16 && (unsigned)pi < np ? ((void **)0x00918374)[pi] : NULL;
    if (pl && (FIELD(pl, PL_INDEX, int) & 15) != pi) pl = NULL;
    void *cm = pl ? FIELD(pl, 0xB90, void *) : NULL;
    uint8_t *rs = cm ? FIELD(cm, 0x40, uint8_t *) : NULL;
    if (!g_paid_upgrades || !rs || cat != 7) { ((void (__thiscall *)(void *, int))0x005BDDBD)(node, cat); return; }
    uint8_t *v = FIELD(node, 0x24, uint8_t *), *ve = FIELD(node, 0x28, uint8_t *);
    char ids[256] = "";
    int n = 0;
    for (uint8_t *c = v; v && c < ve; c += 0x20) {   // the children, 0x20-byte nodes (as 0x5BDDBD walks them)
        uint8_t *d = FIELD(c, 8, uint8_t *);
        if (!d || FIELD(d, 0x48, int) != 7 || c[4]) continue;
        int id = FIELD(d, 0x4C, int);
        ((void (__thiscall *)(void *, int))FN_RS_SCHEDULE)(rs, id);
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
static struct { unsigned ticks, on[9]; } g_att[16];
static void attention_note(void *pl, unsigned idx, uint32_t now) {
    uint8_t *cm = FIELD(pl, 0xB90, uint8_t *), *b8c = FIELD(pl, 0xB8C, uint8_t *), *b98 = FIELD(pl, 0xB98, uint8_t *);
    if (!cm || !b8c || !b98) return;
    uint8_t *rm = FIELD(cm, 0x38, uint8_t *), *rs = FIELD(cm, 0x40, uint8_t *), *um = FIELD(cm, 0x44, uint8_t *);
    uint8_t *b8c4c = FIELD(b8c, 0x4C, uint8_t *);
    uint8_t f[9] = { cm[0x7A], rm ? rm[0x188] : 0, rs ? rs[0x11A] : 0, um ? um[0x140] : 0, cm[0x7B], cm[0x7C],
                     b8c4c ? b8c4c[0x48] : 0, b98[0x2D], b8c[0x62] };
    g_att[idx].ticks++;
    for (int i = 0; i < 9; i++) g_att[idx].on[i] += f[i] != 0;
    static uint32_t last[16];
    if (!g_log || now / 600000 == last[idx] / 600000) return;
    last[idx] = now;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# attention: P%u at %u min, E20=%d A28=%d, on %%:", idx, now / 60000, FIELD(pl, 0xE20, int), FIELD(pl, 0xA28, int));
    for (int i = 0; i < 9; i++) fprintf(g_log, " %d:%u", i, g_att[idx].ticks ? g_att[idx].on[i] * 100 / g_att[idx].ticks : 0);
    fputc('\n', g_log);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
    memset(&g_att[idx], 0, sizeof g_att[idx]);
}

// tech P id...: the player's tech record (tech tree lookup 0x4019A1): first bytes and the data's cost
static void cmd_tech(char *out, const char *args) {
    int p = 0, n = 0, id;
    sscanf(args, "%d%n", &p, &n);
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    uint8_t *tt = pl ? FIELD(pl, PL_TECHTREE, uint8_t *) : NULL;
    if (!tt) { REPLY("error: no player %d\n", p); return; }
    args += n;
    while (sscanf(args, "%d%n", &id, &n) == 1) {
        args += n;
        uint8_t *t = ((uint8_t *(__thiscall *)(void *, int))0x004019A1)(tt, id);   // FN_TECH_LOOKUP
        if (!t) { REPLY("%d: none\n", id); continue; }
        REPLY("%d:", id);
        for (int i = 0; i < 0x20; i++) REPLY(" %02x", t[i]);
        uint8_t *d = FIELD(t, 8, uint8_t *);
        if (d) REPLY(" | cost %d,%d,%d,%d,%d", FIELD(d, 0x24, int), FIELD(d, 0x28, int), FIELD(d, 0x2C, int), FIELD(d, 0x30, int), FIELD(d, 0x34, int));
        int ei = d ? FIELD(d, 0x50, int) : -1;
        REPLY(" | effects %d:", ei);
        if (ei >= 0 && ei <= 0x5DC) {   // effect list 0x93B1B8 + ei*0x10 (+4 ids, +8 count), effect 0x919E70 + id*0x44
            uint8_t *list = (uint8_t *)0x0093B1B8 + ei * 0x10;
            unsigned n = FIELD(list, 8, unsigned), *ids = FIELD(list, 4, unsigned *);
            for (unsigned k = 0; ids && k < n && k < 16; k++) {
                if (ids[k] >= 2000) { REPLY(" %u?", ids[k]); continue; }
                uint8_t *ef = (uint8_t *)0x00919E70 + ids[k] * 0x44;
                REPLY(" %u(%x/%x/%x)", ids[k], FIELD(ef, 0x14, int), FIELD(ef, 0x20, int), FIELD(ef, 0x2C, int));
            }
        }
        REPLY("\n");
    }
}

// techs P: the techs player P has researched (tech record +4), and which are new since the last call - with their
// tech epoch (record +0x14) and cost; tests: what an AI gets at an epoch advance
static void cmd_techs(char *out, const char *args) {
    static uint8_t seen[16][16384];
    int p = atoi(args), total = 0;
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    uint8_t *tt = pl ? FIELD(pl, PL_TECHTREE, uint8_t *) : NULL;
    if (!tt) { REPLY("error: no player %d\n", p); return; }
    REPLY("P%d epoch %d, new:", p, FIELD(tt, TT_EPOCH, int));
    for (int id = 0; id < 16384; id++) {
        uint8_t *t = ((uint8_t *(__thiscall *)(void *, int))0x004019A1)(tt, id);
        int done = t && t[4];
        total += done;
        if (done && !seen[p][id] && strlen(out) < 15000) {
            uint8_t *d = FIELD(t, 8, uint8_t *);
            REPLY(" %d(e%d", id, FIELD(t, 0x14, int));
            if (d) REPLY(" %d/%d/%d/%d/%d", FIELD(d, 0x24, int), FIELD(d, 0x28, int), FIELD(d, 0x2C, int), FIELD(d, 0x30, int), FIELD(d, 0x34, int));
            REPLY(")");
        }
        seen[p][id] = (uint8_t)done;
    }
    REPLY(" - %d researched\n", total);
}

// watchtech P id: hardware write-watch on player P's tech record +4 (researched): who researches it. Tests.
static void watch_set(uintptr_t addr, int len);
static void cmd_watchtech(char *out, const char *args) {
    int p = 0, id = 0;
    sscanf(args, "%d %d", &p, &id);
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    uint8_t *tt = pl ? FIELD(pl, PL_TECHTREE, uint8_t *) : NULL;
    uint8_t *t = tt ? ((uint8_t *(__thiscall *)(void *, int))0x004019A1)(tt, id) : NULL;
    if (!t) { REPLY("error: no tech %d for P%d\n", id, p); return; }
    watch_set((uintptr_t)(t + 4), 1);
    REPLY("ok: watching tech %d of P%d at %p (researched %d)\n", id, p, t + 4, t[4]);
}

// Free research (diagnostic, with tech_log): EETechTreeNode::vf6 0x5BF335 (vtable slot 0x830874) marks a tech
// researched and applies it without any payment; logged with its caller.
#define FN_TECH_GRANT 0x005BF335
static int __fastcall hook_tech_grant(uint8_t *node, void *edx) {
    (void)edx;
    uint8_t before = node[4];
    int r = ((int (__thiscall *)(void *))FN_TECH_GRANT)(node);
    if (g_log && !before && node[4]) {
        uint8_t *d = FIELD(node, 8, uint8_t *), *owner = FIELD(node, 0xC, uint8_t *);
        uintptr_t *fp = (uintptr_t *)__builtin_frame_address(0);
        fprintf(g_log, "# techgrant: tech %d player %d at %.2f min, caller %08x, stack",
                d ? FIELD(d, 0x4C, int) : -1, owner ? FIELD(owner, 4, int) : -1, game_time() / 60000.0,
                (unsigned)(uintptr_t)__builtin_return_address(0));
        for (int k = 0; k < 64; k++) {   // return addresses into the exe on the stack
            uintptr_t v = fp[k];
            if (v > 0x401000 && v < 0x700000) fprintf(g_log, " %06x", (unsigned)v);
        }
        fputc(10, g_log);
    }
    return r;
}

// Tech log (autotest tech_log=1): once a second per AI, techs newly researched, with the stock before (last check)
// and now - whether research is paid shows in the stock.
static int g_tech_log;
static void tech_log_tick(void *pl, unsigned idx, uint32_t now) {
    static uint8_t seen[16][16384];
    static int last_stock[16][7];
    static uint32_t last[16];
    if (!g_tech_log || !g_log || (now >= last[idx] && now - last[idx] < 1000)) return;
    last[idx] = now;
    uint8_t *tt = FIELD(pl, PL_TECHTREE, uint8_t *);
    int *stock = (int *)((uint8_t *)pl + PL_STOCK);
    if (!tt) return;
    char buf[1024];
    int o = 0, n = 0, cost[7] = { 0 };
    for (int id = 0; id < 16384; id++) {
        uint8_t *t = ((uint8_t *(__thiscall *)(void *, int))0x004019A1)(tt, id);
        int done = t && t[4];
        if (done && !seen[idx][id]) {
            uint8_t *d = FIELD(t, 8, uint8_t *);
            for (int r = 2; r < 7 && d; r++) cost[r] += FIELD(d, 0x24 + (r - 2) * 4, int);
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

// rlist P: the research manager's per-epoch tech lists (cm+0x40: +0x18 + e*0x10 = vector begin/end of tech ids,
// +0x114 current epoch); * = researched (tech +4)
static void cmd_rlist(char *out, const char *args) {
    int p = atoi(args);
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    void *cm = pl ? FIELD(pl, 0xB90, void *) : NULL;
    uint8_t *rs = cm ? FIELD(cm, 0x40, uint8_t *) : NULL, *tt = pl ? FIELD(pl, PL_TECHTREE, uint8_t *) : NULL;
    if (!rs || !tt) { REPLY("error: no AI player %d\n", p); return; }
    int cur = FIELD(rs, 0x114, int);
    REPLY("P%d research epoch %d\n", p, cur);
    for (int e = -1; e <= 13; e++) {   // -1: the list at +8
        int *b = FIELD(rs, 0x18 + e * 0x10, int *), *en = FIELD(rs, 0x1C + e * 0x10, int *);
        if (!b || en <= b || en - b > 500) continue;
        REPLY("list %d:", e);
        for (int *q = b; q < en; q++) {
            uint8_t *t = ((uint8_t *(__thiscall *)(void *, int))0x004019A1)(tt, *q);   // FN_TECH_LOOKUP
            REPLY(" %d%s", *q, t && t[4] ? "*" : "");
        }
        REPLY("\n");
    }
}

// tinfo P type...: type data of unit types - family +0x4B4, hit points +0x55C, attack +0x510, range +0x420 - and
// whether player P could train/build one now (0x555710: available, affordable, a building/builder)
static void cmd_tinfo(char *out, const char *args) {
    int p = 0, n = 0, t;
    sscanf(args, "%d%n", &p, &n);
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    if (!pl) { REPLY("error: no player %d\n", p); return; }
    args += n;
    while (sscanf(args, "%d%n", &t, &n) == 1) {
        args += n;
        uint8_t *td = (t >= 0 && t < 2048) ? ((uint8_t **)0x009418A0)[t] : NULL;
        if (!td) { REPLY("%d: none\n", t); continue; }
        REPLY("%d: family %d hp %d attack %d range %.1f can %d\n", t, FIELD(td, 0x4B4, int), FIELD(td, 0x55C, int),
              FIELD(td, 0x510, int), FIELD(td, 0x420, float),
              ((char (__thiscall *)(void *, int, int, int))0x00555710)(pl, t, 0x20, 0));
    }
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
static int g_gather_income[16][7];   // the income at the last line (econ_new_game: income counts start over)
static void gather_log(void *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    int (*last_income)[7] = g_gather_income;
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
#define FARM_EPOCH 2   // farms and granaries come with epoch 2, the Copper Age
static int current_epoch(void *pl);
static void __fastcall hook_add_resource(void *pl, void *edx, int type, int amount) {
    (void)edx;
    ((void (__thiscall *)(void *, int, int))FN_CP_ADD_RESOURCE)(pl, type, amount);
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (amount > 0 && type >= 2 && type < 7 && g_bud[idx].init) {
        g_bud[idx].income[type] += amount;
        // from the Copper Age (farms: food no longer runs out) a player turns most of his income into army
        const struct ai_cfg *c = cfg_of(pl);
        g_bud[idx].mil[type] += amount * (current_epoch(pl) >= FARM_EPOCH ? c->mil_income_share_farms : c->mil_income_share);
    }
}

// POPULATION LIMIT (aimod.ini pop_follow_human per difficulty, default 0 = the game limit; the shipped aimod.ini: 1
// for [easy] - the playtester: the defaults should show the AI at its strongest without cheating). Every minute 0x42F7A5 sets the AI's
// cap (player +0xCCC, read through EEComputerPlayer vf31) to (2.4 x the largest human population + 0.6 x the game
// limit) / 3 (dbcpbehavior records 92/93), times the difficulty's factor (record 98: 70 / 90 / 140%) - so the AI grows
// with the human and doesn't flood him with units; Ignorieren (+0xF02) or game setting 0x9187FC = 4 give it the game
// limit. pop_follow_human=0: the game limit (EEPlayer vf31 0x556EBC, as for a player) for every AI.
#define FN_POP_CAP     0x0042F7A5   // thiscall(player), called at 0x4311E5, 0x43FA9B, 0x474AF0
#define FN_GAME_LIMIT  0x00556EBC   // EEPlayer::vf31 thiscall(player, 0): the game's unit limit for this player
static void __fastcall hook_pop_cap(void *pl, void *edx) {
    (void)edx;
    ((void (__thiscall *)(void *))FN_POP_CAP)(pl);
    if (!cfg_of(pl)->pop_follow) FIELD(pl, 0xCCC, int) = ((int (__thiscall *)(void *, int))FN_GAME_LIMIT)(pl, 0);
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
static int g_min_workforce = 40;   // economy first: below this, citizens ignore the reserve and no early advance

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
static int g_early_requirements = 1;
static int requirements_missing(void *pl) {
    uint8_t *tt = FIELD(pl, PL_TECHTREE, uint8_t *);
    if (!tt) return 0;
    int next = FIELD(tt, TT_EPOCH, int) + 1;
    if (next > 13 || next > *(int *)0x009187F4) return 0;
    uint8_t *rec = tt + 8 + next * 0x54;
    return FIELD(rec, 0x40, int) - FIELD(rec, 0x48, int);
}
// unit type whose first training counts for the next epoch (its tech: object data 0x9418A0[type] +0x4E0)
static int requirement_unit(void *pl, int type) {
    uint8_t *tt = FIELD(pl, PL_TECHTREE, uint8_t *), *od = type >= 0 && type < 2000 ? ((uint8_t **)0x009418A0)[type] : NULL;
    if (!g_early_requirements || !tt || !od || requirements_missing(pl) < 1) return 0;
    uint8_t *t = ((uint8_t *(__thiscall *)(void *, int))0x004019A1)(tt, FIELD(od, 0x4E0, int));
    uint8_t *d = t ? FIELD(t, 8, uint8_t *) : NULL;
    return d && !t[4] && d[0x7D];
}
// Decoration (the editor's "x ..." objects: statues, houses, flags, ruins - name at object data +0x1C) counts as
// an epoch requirement too. When nothing else can be made right now (0x485C8F), the request falls back to every
// candidate and picks a statue (types 400, 402 in the suites), which the AI can never place: the build fix drops it
// after 3 failures, the request stays pending, and the epoch waits until something else happens to count. Their
// techs lose the "counts" flag (data +0x7D) - with no candidate left the request credits the requirement itself
// (0x4860D6's own fallback). Once per AI and game.
static void *g_deco_done[16];   // the tech tree fixed (a new game's can reuse the address: econ_new_game)
static void decoration_fix(void *pl, unsigned ai) {
    void **done = g_deco_done;
    uint8_t *tt = FIELD(pl, PL_TECHTREE, uint8_t *);
    if (!g_early_requirements || !tt || done[ai] == tt) return;
    done[ai] = tt;
    int n = 0;
    for (int type = 0; type < 0x300; type++) {   // the table's size, as the game checks it (0x4B..: < 0x300)
        uint8_t *od = ((uint8_t **)0x009418A0)[type];
        const char *name = od ? FIELD(od, 0x1C, const char *) : NULL;
        if (!name || name[0] != 'x' || name[1] != ' ') continue;
        uint8_t *t = ((uint8_t *(__thiscall *)(void *, int))0x004019A1)(tt, FIELD(od, 0x4E0, int));
        uint8_t *d = t ? FIELD(t, 8, uint8_t *) : NULL;
        if (d && d[0x7D]) { d[0x7D] = 0; n++; }
    }
    if (g_log) fprintf(g_log, "# epoch plan: P%u: %d decoration objects no longer count as epoch requirements\n", ai, n);
}

// the epoch-requirement request out (cm+0x76) since: renewed by early_requirements (3 minutes without progress) or
// else re-armed by econ_tick after 5 minutes - one timer, so the two never renew the same request twice
static uint32_t g_epoch_req_t[16];
static void early_requirements(void *pl, unsigned ai, uint32_t now, int prefarm) {
    static uint32_t asked[16];
    decoration_fix(pl, ai);
    static int asked_missing[16];
    uint8_t *cm = FIELD(pl, 0xB90, uint8_t *), *tt = FIELD(pl, PL_TECHTREE, uint8_t *);
    if (!g_early_requirements || !cm || !tt || !FIELD(cm, 0x40, void *)) return;
    int missing = requirements_missing(pl);
    if (missing < 1) { asked[ai] = 0; return; }
    if (cm[0x76]) {   // a request is out: renew it after 3 minutes without progress
        if (asked[ai] && now >= asked[ai] && now - asked[ai] < 180000) return;
        if (asked[ai] && missing < asked_missing[ai]) { asked[ai] = g_epoch_req_t[ai] = now ? now : 1; asked_missing[ai] = missing; return; }
    }
    if (!prefarm && (int)(FIELD(cm, 0x58, uint32_t) - now) > 10 * 60000) return;
    ((void (__thiscall *)(void *, int))0x004860D6)(FIELD(cm, 0x40, void *), missing);
    cm[0x76] = 1;
    asked[ai] = g_epoch_req_t[ai] = now ? now : 1;
    asked_missing[ai] = missing;
    if (g_log) fprintf(g_log, "# epoch plan: P%u at %.1f min asks for %d new things for epoch %d\n", ai, now / 60000.0,
                       missing, FIELD(tt, TT_EPOCH, int) + 1);
}

// Per AI tick (from hook_rm_update): pull the epoch attempt forward in the pre-farm epochs once affordable.
static int unit_cost(void *pl, int type, int r);   // below
static void retry_dropped(void *pl, unsigned idx, uint32_t now) {
    if (!g_build_fix) return;
    if (now < 30000) { memset(g_retry[idx], 0, sizeof g_retry[idx]); return; }   // new game
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
    void *um = cm ? FIELD(cm, 0x44, void *) : NULL;
    int *stock = (int *)((uint8_t *)pl + PL_STOCK);
    if (!bm || !um) return;
    for (int k = 0; k < 16; k++) {
        int type = g_retry[idx][k].type;
        if (!type || now < g_retry[idx][k].next) continue;
        type = upgraded_type(pl, type);
        uint8_t *dq = ((uint8_t *(__thiscall *)(void *, int))0x0048F8AF)(um, type);
        if (dq && FIELD(dq, 0x10, int) > 0) { g_retry[idx][k].type = 0; continue; }   // it has one now
        int pay = 1;
        for (int r = 2; r < 7; r++) if (unit_cost(pl, type, r) > stock[r]) pay = 0;
        if (!pay) { g_retry[idx][k].next = now + 30000; continue; }
        char ok = ((char (__thiscall *)(void *, int, int, int))FN_PLACE)(bm, type, 0, 0);
        int give_up = !ok && ++g_retry[idx][k].tries >= 5;
        if (g_log) fprintf(g_log, "# place: P%u at %.1f min: dropped type %d tried again - %s\n", idx, now / 60000.0, type,
                           ok ? "placed" : give_up ? "failed, given up" : "failed");
        if (ok || give_up) g_retry[idx][k].type = 0;
        else g_retry[idx][k].next = now + 120000;
    }
}
static uint32_t g_last_attack_time[16];   // +0xED8 at the last look (defence.c attack_retreat moves it on, not a launch)
static void econ_tick(void *pl, uint32_t now) {
    const struct ai_cfg *c = cfg_of(pl);
    // attack log: the next-attack time (+0xED8) is redrawn whenever an attack starts
    unsigned ai = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
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
        void *cm = FIELD(pl, 0xB90, void *);
        uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
        int x = bm ? FIELD(bm, 0x12C, int) : -10000, y = bm ? FIELD(bm, 0x130, int) : -10000;
        if (bm && (x != bx[ai] || y != by[ai])) {
            if (g_log) fprintf(g_log, "# base: P%u at %.1f min: %d,%d -> %d,%d\n", ai, now / 60000.0, bx[ai], by[ai], x, y);
            bx[ai] = x;
            by[ai] = y;
        }
    }
    chat_tick(pl, ai, now);
    diplo_tick(pl, ai, now);
    rates_update(pl, ai, now);
    uint32_t t_attack = FIELD(pl, 0xED8, uint32_t);
    if (t_attack != g_last_attack_time[ai]) {
        if (g_last_attack_time[ai] && g_log)
            fprintf(g_log, "# attack: P%u at %.1f min, next at %.1f min\n", ai, now / 60000.0, t_attack / 60000.0);
        if (g_last_attack_time[ai]) persona_attack_launched(ai);
        if (g_last_attack_time[ai])
            ai_say(pl, CHAT_ATTACK, "[ai] attack launched, next one in %.0f min", ((float)t_attack - (float)now) / 60000.0f);
        g_last_attack_time[ai] = t_attack;
    }
    int ep = current_epoch(pl);
    early_requirements(pl, ai, now, c->copper_plan && ep < FARM_EPOCH);
    // pull the attempt forward: before farms with the copper plan, in every epoch with epoch_pacing=fast
    if (!(c->copper_plan && ep < FARM_EPOCH) && !c->pacing_fast) return;
    uint8_t *cm = FIELD(pl, 0xB90, uint8_t *);
    // AttemptEpochAdvance (0x46C966) asks the research manager once for the techs the next epoch requires
    // (record +0x40 needed, +0x48 done) and then waits while cm+0x76 is set; re-arm it after 5 minutes.
    uint32_t *pending_since = g_epoch_req_t;   // shared with early_requirements' renewal
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (cm && cm[0x76]) {
        if (!pending_since[pi] || now < pending_since[pi]) pending_since[pi] = now ? now : 1;
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
    // diagnostic: while it can pay but hasn't advanced, every 30 s - requirement techs needed/done (epoch record
    // +0x40/+0x48), requirement request pending (cm+0x76), epoch research scheduled (cm+0x78), attention cm+0x7A
    static uint32_t last_wait[16];
    if (g_log && (now < last_wait[pi] || now - last_wait[pi] >= 30000)) {
        last_wait[pi] = now;
        uint8_t *tt = FIELD(pl, PL_TECHTREE, uint8_t *), *rec = tt + 8 + (ep + 1) * 0x54;
        fprintf(g_log, "# epoch wait: P%u at %.1f min, epoch %d: requirements %d/%d, pending %d, scheduled %d, attention %d;",
                pi, now / 60000.0, ep + 1, FIELD(rec, 0x48, int), FIELD(rec, 0x40, int), cm[0x76], cm[0x78], cm[0x7A]);
        // the research manager's list of this epoch: tech id (* researched) building +0x6C, cost food..iron
        uint8_t *rs = FIELD(cm, 0x40, uint8_t *);
        int *b = rs ? FIELD(rs, 0x18 + ep * 0x10, int *) : NULL, *en = rs ? FIELD(rs, 0x1C + ep * 0x10, int *) : NULL;
        for (int *q = b; b && en > b && en - b < 100 && q < en; q++) {
            uint8_t *t = ((uint8_t *(__thiscall *)(void *, int))0x004019A1)(tt, *q), *d = t ? FIELD(t, 8, uint8_t *) : NULL;
            if (d) fprintf(g_log, " %d%s@%d(%d/%d/%d/%d/%d)", *q, t[4] ? "*" : "", FIELD(d, 0x6C, int), FIELD(d, 0x24, int),
                           FIELD(d, 0x28, int), FIELD(d, 0x2C, int), FIELD(d, 0x30, int), FIELD(d, 0x34, int));
        }
        fputc('\n', g_log);
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
static int *bq_entry(uint8_t *bm, int e, unsigned i);   // attack.c: the build queue of epoch e
static int rm_count(uint8_t *rm, int t);                 // below: citizens on resource t
static int unit_cost(void *pl, int type, int r);       // below
static int g_late_targets = 7;
static int g_wood_want[16];   // bit 1: the last wood target (steps of 2+)
static int g_food_cap[16];          // bit 8: the food target's ceiling now (0 = not set)
static uint32_t g_food_cap_t[16];   // its last step
// n more workers for stone, gold and iron: by their demand beyond stock (up to 3000 each, +100 so each gets some), the
// rounding rest to gold and iron
static void to_metals(uint32_t *target, const int *demand, const int *stock, int n) {
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
static int g_pull[16];
static uint32_t g_pull_t[16];
static unsigned g_pull_seen[16][7];
static unsigned mil_short(unsigned idx, int r);   // below: the army's orders refused for resource r (counter)
static void budget_reserve(void *pl, int reserve[7]);
static void econ_targets(uint8_t *rm) {
    void *pl = rm_player(rm);
    if (!pl || !cfg_of(pl)->copper_plan) return;
    int late = current_epoch(pl) >= FARM_EPOCH;
    int lt = PV(FIELD(pl, PL_INDEX, int), g_v_late, g_late_targets);
    if (late && !(lt & 1)) return;
    int *stock = (int *)((uint8_t *)pl + PL_STOCK);
    int *demand = (int *)(rm + RM_DEMAND);
    uint32_t *target = (uint32_t *)(rm + RM_TARGET);
    int reserve[7];
    budget_reserve(pl, reserve);
    int total = 0, need[7] = { 0 }, sum = 0;
    for (int r = 2; r < 7; r++) total += (int)target[r];
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (late) {
        if (total < 6) return;
        // the game's wood demand, or the wood of the next 3 queued buildings up to its epoch if more: a building it
        // can't pay for is no demand at all (the queue's "can build", 0x555710, tests the cost too) - the airport
        // (425 wood) and tank factory waited with 115 wood in stock and a demand of 55, so no AA tank either
        void *cm = FIELD(rm, RM_OWNER, void *);
        uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
        int queued = 0, nq = 0;
        for (int e = 0; bm && e <= current_epoch(pl) && e < 14 && nq < 3; e++)
            for (unsigned i = 0; nq < 3; i++) {
                int *ent = bq_entry(bm, e, i);
                if (!ent) break;
                int c = unit_cost(pl, ent[0], 3);
                if (c > 0) { queued += c; nq++; }
            }
        int need = (demand[3] > queued ? demand[3] : queued) - stock[3];   // filled within about 2 minutes
        need = need < 0 ? 0 : need > 2000 ? 2000 : need;
        float pw = g_rate[idx].per_worker[3];
        int want = (int)((float)need / (2.0f * (pw > 2.0f ? pw : 2.0f)) + 0.99f), most = total * 2 / 5;
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
        void *um = cm ? FIELD(cm, 0x44, void *) : NULL;
        uint8_t *fq = um ? ((uint8_t *(__thiscall *)(void *, int))0x0048F8AF)(um, 0x2A) : NULL;
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
            int most = (int)target[2] * 3 / 5;
            g_pull[idx] = g_pull[idx] < 0 ? 0 : g_pull[idx] > most ? most : g_pull[idx];
            if (g_pull[idx] > 0) {
                to_metals(target, demand, stock, g_pull[idx]);
                target[2] -= (uint32_t)g_pull[idx];
            }
        }
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
#define FN_NEW           0x00683718   // operator new (cdecl)
#define FN_BUILD_GOAL    0x0060BAD0   // EEUGBuildRepair constructor: thiscall(goal, player, building, 1)
#define FN_SET_GOAL      0x005F05BD   // thiscall(unit, goal, 0, 0)
#define FN_UNIT_SAY      0x005EFCF0   // thiscall(unit, 0x1F51) - as the assignment does after the order
static int g_build_fields = 1;
static int g_fields_open[16];   // unfinished fields per player (econ_fields), for the wood reserve
static int field_wood(unsigned idx) { return g_build_fields ? 50 * (g_fields_open[idx] < 2 ? g_fields_open[idx] : 2) : 0; }
static uint8_t *field_site(uint8_t *f) {   // what a BuildRepair goal holds (+0x3C) for this field (0x60BAD0)
    uint8_t *p = FIELD(f, 0x30, uint8_t *), *last = f;
    while (p) { last = p; p = FIELD(p, 0x1CC, uint8_t *); }
    return last;
}
static int citizen_free(uint8_t *u, int idle) {   // idle, or (idle=0) gathering
    if (!u || FIELD(u, 0x3C, int) <= 0 || FIELD(u, 0x33B, uint8_t) || FIELD(u, 0x341, uint8_t) || FIELD(u, 0x68, int) ||
        FIELD(u, 0x1C, int) == -10000)
        return 0;
    void *goal = FIELD(u, 0x1EC, void *);
    return idle ? !goal : goal && !strcmp(rtti_name(goal), "EEUGGather@@");
}
// plan P: the building plan's entries (cm+8: list head, node +0 next, +0xC type, +0x10 wanted count), as 0x46CD18
// walks them (0x2F = the epoch's tower)
static void cmd_plan(char *out, const char *args) {
    int p = atoi(args), t = 0;
    if (sscanf(args, "%d retry %d", &p, &t) == 2 && p > 0 && p < 16) {   // plan P retry T: T counts as dropped, tried now
        retry_note((unsigned)p, t, game_time());
        for (int k = 0; k < 16; k++) if (g_retry[p][k].type == t) g_retry[p][k].next = game_time();
        REPLY("P%d: type %d queued for a retry\n", p, t);
        return;
    }
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    uint8_t *cm = pl ? FIELD(pl, 0xB90, uint8_t *) : NULL, *head = cm ? FIELD(cm, 8, uint8_t *) : NULL;
    if (!head) { REPLY("error: no AI player %d\n", p); return; }
    REPLY("P%d plan (build flag %d, attention %d):", p, FIELD(pl, 0xEFC, uint8_t), FIELD(cm, 0x7B, uint8_t));
    int n = 0;
    for (uint8_t *nd = FIELD(head, 0, uint8_t *); nd && nd != head && n < 200; nd = FIELD(nd, 0, uint8_t *), n++)
        REPLY(" %d:%d", FIELD(nd, 0xC, int), FIELD(nd, 0x10, int));
    REPLY("\n");
}
static int field_worked(uint8_t *rm, uint8_t *f) {   // a citizen of this AI farms it or is on its way
    // (0x52E5F7 counts only those at the field - one carrying food to the granary left it "free")
    for (int r = 0; r < 7; r++) {
        uint8_t *list = rm + RM_LISTS + r * 0x14;
        for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), cnt = FIELD(list, 0x10, unsigned); k < cnt; k++) {
            uint8_t *u = deque_at(list, first + k), *goal = u ? FIELD(u, 0x1EC, uint8_t *) : NULL;
            if (goal && FIELD(goal, 0x38, uint8_t *) == f && !strcmp(rtti_name(goal), "EEUGGather@@")) return 1;
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
static struct { int16_t x, y; uint32_t queued; } g_field_at[16][32];
static int g_field_n[16];
static int can_place(void *pl, int type, int x, int y);   // defence.c
static int raiders_near(void *pl, int x, int y, int r);     // attack.c: enemy soldiers it can see
static void fields_rebuild(void *pl, unsigned idx, void *um, uint8_t *dq, uint32_t now) {
    if (now < 60000) g_field_n[idx] = 0;   // new game
    unsigned first = dq ? FIELD(dq, 0xC, unsigned) : 0, cnt = dq ? FIELD(dq, 0x10, unsigned) : 0;
    for (unsigned k = 0; k < cnt; k++) {   // remember every field's tile
        uint8_t *f = deque_at(dq, first + k);
        if (!f || FIELD(f, 0x3C, int) <= 0) continue;
        int x = FIELD(f, 0x1C, int), y = FIELD(f, 0x20, int), j = 0;
        while (j < g_field_n[idx] && (g_field_at[idx][j].x != x || g_field_at[idx][j].y != y)) j++;
        if (j == g_field_n[idx] && j < 32) { g_field_at[idx][j].x = (int16_t)x; g_field_at[idx][j].y = (int16_t)y;
                                            g_field_at[idx][j].queued = 0; g_field_n[idx]++; }
    }
    uint8_t *gq = ((uint8_t *(__thiscall *)(void *, int))0x0048F8AF)(um, 0x34);   // granaries
    int queued = 0;
    if (!gq) return;
    for (int j = 0; j < g_field_n[idx]; j++) {
        int x = g_field_at[idx][j].x, y = g_field_at[idx][j].y, there = 0, granary = 0;
        for (unsigned k = 0; k < cnt && !there; k++) {
            uint8_t *f = deque_at(dq, first + k);
            there = f && FIELD(f, 0x3C, int) > 0 && FIELD(f, 0x1C, int) == x && FIELD(f, 0x20, int) == y;
        }
        if (there || (g_field_at[idx][j].queued && now - g_field_at[idx][j].queued < 120000)) continue;
        for (unsigned k = 0, gf = FIELD(gq, 0xC, unsigned), gc = FIELD(gq, 0x10, unsigned); k < gc && !granary; k++) {
            uint8_t *g = deque_at(gq, gf + k);
            granary = g && FIELD(g, 0x3C, int) > 0 && abs(FIELD(g, 0x1C, int) - x) <= 4 && abs(FIELD(g, 0x20, int) - y) <= 4;
        }
        if (!granary) continue;
        if (raiders_near(pl, x, y, 8)) continue;   // raided: later (builders: 8 tiles, as citizens_safety)
        g_field_at[idx][j].queued = now;
        if (!can_place(pl, 0x2A, x, y)) continue;
        ((void *(__cdecl *)(void *, int, int, int))0x0062EF8A)(pl, 0x2A, x, y);
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
// for one (0x46802D(bm, 0x34, 1, 0), as that plan entry does); it comes with 8 fields, which the passes above build
// and farm.
// The game's spot can be one its builders never manage to lay out: P2's main Town Center hemmed in by walls, towers,
// a tank factory and both granaries' fields - every minute a request, "placed", the builders stood at 95,97 with no
// foundation until the next request moved them, idle; 16 fields for a food target of 107 from minute 14 to the end
// (88 on wood, food 10). A request without a new granary 50 s later: the next goes to our own spot - near a Town
// Center, Capitol or Settlement, 4-12 tiles out, placeable, no enemy soldiers within 12, room for 6+ of the 8 fields
// around it (2 tiles apart, as the game lays them out).
static int granary_spot(void *pl, void *um, int *ox, int *oy) {
    static const int centers[] = { 35, 41, 43 };
    for (int c = 0; c < 3; c++) {
        uint8_t *q = ((uint8_t *(__thiscall *)(void *, int))0x0048F8AF)(um, centers[c]);
        for (unsigned k = 0, f = q ? FIELD(q, 0xC, unsigned) : 0, n = q ? FIELD(q, 0x10, unsigned) : 0; k < n; k++) {
            uint8_t *b = deque_at(q, f + k);
            if (!b || FIELD(b, 0x3C, int) <= 0) continue;
            int cx = FIELD(b, 0x1C, int), cy = FIELD(b, 0x20, int);
            for (int r = 4; r <= 12; r++)
                for (int ax = -r; ax <= r; ax++)
                    for (int ay = -r; ay <= r; ay++) {
                        if (abs(ax) != r && abs(ay) != r) continue;
                        int x = cx + ax, y = cy + ay, room = 0;
                        if (!can_place(pl, 0x34, x, y) || raiders_near(pl, x, y, 12)) continue;
                        for (int fx = -2; fx <= 2; fx += 2)
                            for (int fy = -2; fy <= 2; fy += 2)
                                if ((fx || fy) && can_place(pl, 0x2A, x + fx, y + fy)) room++;
                        if (room < 6) continue;
                        *ox = x; *oy = y;
                        return 1;
                    }
        }
    }
    return 0;
}
static int g_more_granaries = 1;
static void more_granaries(void *pl, unsigned idx, uint8_t *rm, void *cm, void *um, int open_fields, uint32_t now) {
    static uint32_t next[16];
    if (!g_more_granaries || open_fields || current_epoch(pl) < FARM_EPOCH) return;
    if (now < next[idx] && next[idx] - now <= 60000) return;
    next[idx] = now + 60000;
    int want = (int)FIELD(rm, RM_TARGET + 2 * 4, uint32_t), have = rm_count(rm, 2);
    if (want < have + 6) return;
    uint8_t *bm = FIELD(cm, 0x3C, uint8_t *), *gq = ((uint8_t *(__thiscall *)(void *, int))0x0048F8AF)(um, 0x34);
    int granaries = 0;
    for (unsigned k = 0, f = gq ? FIELD(gq, 0xC, unsigned) : 0, c = gq ? FIELD(gq, 0x10, unsigned) : 0; k < c; k++) {
        uint8_t *g = deque_at(gq, f + k), *st = g ? FIELD(g, 0x30, uint8_t *) : NULL;
        if (!g || FIELD(g, 0x3C, int) <= 0) continue;
        if (!st || !FIELD(st, 0x340, uint8_t)) return;   // one is being built
        granaries++;
    }
    int cap = 1 + want / 8;
    if (!bm || granaries >= (cap < 8 ? cap : 8) || ((char (__thiscall *)(void *, int))0x00465640)(bm, 0x34)) return;
    static struct { uint32_t t; int n; } req[16];
    int x, y;
    if (now < req[idx].t) req[idx].t = 0;   // a new game
    if (req[idx].t && granaries <= req[idx].n && now - req[idx].t >= 50000 && granary_spot(pl, um, &x, &y)) {
        char ok = ((char (__thiscall *)(void *, int, int, int))FN_PLACE)(bm, 0x34, x, y);
        if (g_log) fprintf(g_log, "# fields: P%u at %.1f min: the last granary request brought none - our spot %d,%d: %s\n",
                           idx, now / 60000.0, x, y, ok ? "placed" : "refused");
        req[idx].t = now ? now : 1;
        req[idx].n = granaries;
        if (ok) return;
    }
    req[idx].t = now ? now : 1;
    req[idx].n = granaries;
    ((void (__thiscall *)(void *, int, int, int))0x0046802D)(bm, 0x34, 1, 0);
    if (g_log) fprintf(g_log, "# fields: P%u at %.1f min: food target %d, %d on food, %d granaries - one more requested\n",
                       idx, now / 60000.0, want, have, granaries);
}
// CRASH GUARD: the build list can hold a field (0x2A) too - its re-queue asks for a destroyed one again - and the
// placement's field case (0x46580A) only orders builders to the foundation on that tile: with none there it makes
// EEUGBuildRepair for a null building and crashes (0x60BB19, seen once fields got built and then lost in battle).
// hook_place lets a field through only with a foundation on its tile - laid out first if the tile is free - and
// reports the others as placed (dropped).
static int field_place_ok(uint8_t *bm, int x, int y) {
    void *pl = FIELD(bm, 0x138, void *), *cm = FIELD(bm, 0x134, void *);
    void *um = cm ? FIELD(cm, 0x44, void *) : NULL;
    if (!pl || !um || (!x && !y)) return 0;
    uint8_t *dq = ((uint8_t *(__thiscall *)(void *, int))0x0048F8AF)(um, 0x2A);
    for (unsigned k = 0, first = dq ? FIELD(dq, 0xC, unsigned) : 0, cnt = dq ? FIELD(dq, 0x10, unsigned) : 0; k < cnt; k++) {
        uint8_t *f = deque_at(dq, first + k);
        if (f && FIELD(f, 0x3C, int) > 0 && FIELD(f, 0x1C, int) == x && FIELD(f, 0x20, int) == y) return 1;
    }
    if (!can_place(pl, 0x2A, x, y)) return 0;
    ((void *(__cdecl *)(void *, int, int, int))0x0062EF8A)(pl, 0x2A, x, y);
    if (g_log) fprintf(g_log, "# fields: P%d: build list field at %d,%d - foundation laid out first\n", FIELD(pl, PL_INDEX, int), x, y);
    return 1;
}
static int site_danger(void *pl, uint8_t *site, int food);   // attack.c: known enemy towers on the way
static void econ_fields(uint8_t *rm) {   // before the citizen assignment
    void *pl = rm_player(rm);
    if (!g_build_fields || !pl) return;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    static uint32_t next[16];
    uint32_t now = game_time();
    if (now < next[idx] && next[idx] - now <= 5000) return;
    next[idx] = now + 5000;
    void *cm = FIELD(rm, RM_OWNER, void *);
    void *um = cm ? FIELD(cm, 0x44, void *) : NULL;   // unit manager: per-type lists (0x48F8AF)
    uint8_t *dq = um ? ((uint8_t *(__thiscall *)(void *, int))0x0048F8AF)(um, 0x2A) : NULL;
    if (um) fields_rebuild(pl, idx, um, dq, now);
    if (!dq) return;
    uint8_t *todo[16], *done[16];
    int nt = 0, nd = 0, ordered = 0, farmers = 0;
    for (unsigned k = 0, first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned); k < cnt && nt < 16; k++) {
        uint8_t *f = deque_at(dq, first + k), *st = f ? FIELD(f, 0x30, uint8_t *) : NULL;
        if (!st || FIELD(f, 0x3C, int) <= 0) continue;
        if (site_danger(pl, f, 1)) continue;   // under known enemy towers: no builders, no farmers (attack.c)
        // raided: a player doesn't send his citizens one by one into the raiders - the same distances as
        // citizens_safety (attack.c: 8 for builders, 6 for farmers): with 6 here a builder went every 10 s to a
        // field with bowmen 7 tiles away, was sent back 4 s later, and the next one went (the player's army shot
        // them on the way)
        if (raiders_near(pl, FIELD(f, 0x1C, int), FIELD(f, 0x20, int), FIELD(st, 0x340, uint8_t) ? 6 : 8))
            continue;
        if (!FIELD(st, 0x340, uint8_t)) todo[nt++] = f;
        else if (nd < 16 && !field_worked(rm, f)) done[nd++] = f;
    }
    g_fields_open[idx] = nt;
    more_granaries(pl, idx, rm, cm, um, nt + nd, now);
    int wood = FIELD(pl, PL_STOCK + 3 * 4, int);
    for (int i = 0; i < nt && wood >= 50; i++) {
        uint8_t *site = field_site(todo[i]), *best = NULL;
        int fx = FIELD(todo[i], 0x1C, int), fy = FIELD(todo[i], 0x20, int), bd = 0x7FFFFFFF, busy = 0;
        for (int r = 0; r < 7 && !busy; r++) {   // a builder already on it?
            uint8_t *list = rm + RM_LISTS + r * 0x14;
            for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), cnt = FIELD(list, 0x10, unsigned); k < cnt; k++) {
                uint8_t *u = deque_at(list, first + k), *goal = u ? FIELD(u, 0x1EC, uint8_t *) : NULL;
                if (goal && FIELD(goal, 0x3C, uint8_t *) == site && !strcmp(rtti_name(goal), "EEUGBuildRepair@@")) { busy = 1; break; }
            }
        }
        if (busy) continue;
        for (int pass = 0; pass < 3 && !best; pass++)   // idle, on food, on stone/gold/iron - never on wood
            for (int r = 0; r < 7; r++) {
                if (pass == 1 ? r != 2 : pass == 2 ? r < 4 : 0) continue;
                uint8_t *list = rm + RM_LISTS + r * 0x14;
                for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), cnt = FIELD(list, 0x10, unsigned); k < cnt; k++) {
                    uint8_t *u = deque_at(list, first + k);
                    if (!citizen_free(u, pass == 0)) continue;
                    int dx = FIELD(u, 0x1C, int) - fx, dy = FIELD(u, 0x20, int) - fy, d = dx * dx + dy * dy;
                    if (d < bd) { bd = d; best = u; }
                }
            }
        if (!best) break;
        void *goal = ((void *(__cdecl *)(unsigned))FN_NEW)(0x5C);
        if (!goal) break;
        ((void (__thiscall *)(void *, void *, void *, int))FN_BUILD_GOAL)(goal, pl, todo[i], 1);
        ((void (__thiscall *)(void *, void *, int, int))FN_SET_GOAL)(best, goal, 0, 0);
        ((void (__thiscall *)(void *, int))FN_UNIT_SAY)(best, 0x1F51);
        uint8_t *td = FIELD(best, 0x2C, uint8_t *);
        (*(void (__thiscall **)(void *, void *))(FIELD(td, 0, uint8_t *) + 0x74))(td, best);
        ordered++;
        wood -= 50;
    }
    uint32_t *target = (uint32_t *)(rm + RM_TARGET);
    // no more farmers than food's target: with the food pile's ceiling (late_targets bit 8) fields stand idle on
    // purpose - filled again from the mines here, the farmers the rebalance had moved came straight back
    int room = (int)target[2] - rm_count(rm, 2);
    for (int i = 0; i < nd && farmers < room; i++) {   // farmers for finished fields
        uint8_t *best = NULL;
        int fx = FIELD(done[i], 0x1C, int), fy = FIELD(done[i], 0x20, int), bd = 0x7FFFFFFF;
        for (int pass = 0; pass < 2 && !best; pass++)
            for (int r = pass ? 4 : 0; r < 7; r++) {
                if (pass && rm_count(rm, r) <= (int)target[r]) continue;   // only a surplus
                uint8_t *list = rm + RM_LISTS + r * 0x14;
                for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), cnt = FIELD(list, 0x10, unsigned); k < cnt; k++) {
                    uint8_t *u = deque_at(list, first + k);
                    if (!citizen_free(u, pass == 0)) continue;
                    int dx = FIELD(u, 0x1C, int) - fx, dy = FIELD(u, 0x20, int) - fy, d = dx * dx + dy * dy;
                    if (d < bd) { bd = d; best = u; }
                }
            }
        if (!best) break;
        ((void (__thiscall *)(void *, void *, void *, int))FN_GATHER)(rm, best, done[i], 0);
        farmers++;
    }
    if (farmers && g_log)
        fprintf(g_log, "# fields: P%u at %.1f min: %d finished fields without a farmer, %d farmers sent\n", idx, now / 60000.0,
                nd, farmers);
    if (ordered && g_log)
        fprintf(g_log, "# fields: P%u at %.1f min: %d unfinished fields, %d builders sent\n", idx, now / 60000.0,
                nt, ordered);
}
static void econ_rebalance(uint8_t *rm) {
    void *pl = rm_player(rm);
    if (!g_rebalance || !pl) return;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    uint32_t *target = (uint32_t *)(rm + RM_TARGET);
    int moves = 0, dry = 0;   // dry: donor lists without a movable citizen
    // wood too with late_targets (the game's own wood search, 0x48C060(1, unit), safe_sites' danger check): 38 on gold
    // above their target stayed there for good while wood had 3
    int first = (PV(idx, g_v_late, g_late_targets) & 2) && current_epoch(pl) >= FARM_EPOCH ? 3 : 4;
    for (int r = first; r < 7 && moves < 2; r++) {
        while (moves < 2 && (int)target[r] - rm_count(rm, r) >= 2) {
            int d = -1, best = 2;
            for (int t = 2; t < 7; t++) {
                int s = rm_count(rm, t) - (int)target[t];
                if (t != r && !(dry >> t & 1) && s > best) { best = s; d = t; }
            }
            if (d < 0) break;
            uint8_t *u = movable_gatherer(rm, d);
            if (!u) { dry |= 1 << d; continue; }
            void *site = r == 3 ? hook_find_wood(rm, NULL, 1, u)
                                : hook_find_mine(rm, NULL, r, u, 1);   // with mine_choice: nearest to a drop site
            if (!site) break;   // no free, safe mine of this resource
            int before = rm_count(rm, r);
            ((void (__thiscall *)(void *, void *, void *, int))FN_ASSIGN_TO_SITE)(rm, u, site, 0);
            if (rm_count(rm, r) == before) { dry |= 1 << d; continue; }
            g_moved[idx][r]++;
            moves++;
        }
    }
}

// INCOME RATES per AI and resource (every 30 s, smoothed over ~2 minutes): per minute in total and per worker-minute
// (only with 2+ workers on it; defaults from the suites until then). For the reserve plan and the worker plan.
static void rates_update(void *pl, unsigned idx, uint32_t now) {
    if (g_rate[idx].t && now >= g_rate[idx].t && now - g_rate[idx].t < 30000) return;
    if (!g_rate[idx].t || now < g_rate[idx].t) {   // first look / new game
        memset(&g_rate[idx], 0, sizeof g_rate[idx]);
        memcpy(g_rate[idx].per_worker, k_default_pw, sizeof k_default_pw);
        memcpy(g_rate[idx].last, g_bud[idx].income, sizeof g_rate[idx].last);
        g_rate[idx].t = now ? now : 1;
        return;
    }
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *rm = cm ? FIELD(cm, 0x38, uint8_t *) : NULL;
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

// RESERVE PLAN (aimod.ini [all] reserve_plan, default on) - what spending sees instead of budget_reserve: like a
// player waiting for the slowest resource of the next epoch, the AI holds back only what it can't gather again in
// time - from the Copper Age on, when farms make food renewable. Requirement N: the next epoch's cost within 10
// minutes of the planned attempt (cm+0x58; nothing before). Horizon H = the time until the attempt or until the
// slowest missing resource is in (shortfall / income, at most 10 min), whichever is later. Each resource keeps
// N - 80% of what its income brings in within H: food spent on citizens while gold and iron are still missing
// grows back by the time they are ready; the bottleneck stays reserved. Before farms the full copper-plan reserve
// stays: wild food is finite (suites: releasing it there delayed the Copper Age by 1.5-3 minutes).
static void spend_reserve(void *pl, unsigned idx, int reserve[7], float *horizon) {
    if (horizon) *horizon = -1;
    if (!g_reserve_plan) { budget_reserve(pl, reserve); return; }
    memset(reserve, 0, 7 * sizeof *reserve);
    uint8_t *tt = FIELD(pl, PL_TECHTREE, uint8_t *), *cm = FIELD(pl, 0xB90, uint8_t *);
    if (!tt || !cm) return;
    int ep = FIELD(tt, TT_EPOCH, int), next = ep + 1;
    if (next > 13 || next > *(int *)0x009187F4 || !epoch_tech(pl, next)) return;
    if (cfg_of(pl)->copper_plan && next <= FARM_EPOCH) { budget_reserve(pl, reserve); return; }
    int remaining = (int)(FIELD(cm, 0x58, uint32_t) - game_time());
    if (remaining > 10 * 60000) return;
    float until = remaining > 0 ? (float)remaining / 60000.0f : 0;   // minutes until the attempt
    for (int r = 2; r < 7; r++) reserve[r] = epoch_cost(pl, next, r);
    int *stock = (int *)((uint8_t *)pl + PL_STOCK);
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

static void trace_train(void *pl, int *req, uintptr_t r, const int *before, const int *reserve);

static int unit_cost(void *pl, int type, int r);

// Military request statistics, logged every 5 minutes: how often the AI asks for soldiers, how many it gets
// and which resource was short when a request failed (unit cost = object data 0x9418A0[type] +0x24..).
static struct { unsigned req, trained, short_of[7], subs, saving; uint32_t last_log; int ftype[4]; unsigned fcount[4]; } g_mil[16];
static unsigned mil_short(unsigned idx, int r) { return g_mil[idx & 15].short_of[r]; }
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
static int g_unit_sub = 1, g_sub_wait = 90;
static uint32_t g_save_since[16];   // game time it started saving (a request refused by the budget), 0 = not saving
static int base_threatened(unsigned idx);   // diplo.c: enemy soldiers in its base
static struct { void *pl; uint32_t t; int n; int type[48]; } g_cand[16];
static int land_soldier(int type) {
    uint8_t *td = (type >= 0 && type < 724) ? ((uint8_t **)0x009418A0)[type] : NULL;
    static const int no[] = { 104, 105, 106, 107, 141, 142, 602, 603, 102, 287, 158, 32, 480, 1, 216 };
    if (!td || FIELD(td, 0x4B8, int) != type || FIELD(td, 0x4B4, int) != 5) return 0;   // a real record of this type
    for (size_t i = 0; i < sizeof no / sizeof *no; i++) if (type == no[i]) return 0;
    return 1;
}
// Requests that may be replaced: every military unit but ships (family 4: navies on water maps), siege and rams
// (the siege controller's business) - heroes, priests and the like included.
static int substitutable(int type) {
    uint8_t *td = (type >= 0 && type < 724) ? ((uint8_t **)0x009418A0)[type] : NULL;
    static const int no[] = { 104, 105, 106, 107, 141, 142, 602, 603, 102, 287, 158, 32, 480 };
    if (!td || FIELD(td, 0x4B8, int) != type || FIELD(td, 0x4B4, int) == 4) return 0;
    for (size_t i = 0; i < sizeof no / sizeof *no; i++) if (type == no[i]) return 0;
    return 1;
}
static int soldier_score(int type) {
    uint8_t *td = ((uint8_t **)0x009418A0)[type];
    return FIELD(td, 0x55C, int) * FIELD(td, 0x510, int);
}
static int substitute_soldier(void *pl, unsigned idx, const int *vis, int want, int *got) {
    uint32_t now = game_time();
    uint8_t *tt = FIELD(pl, PL_TECHTREE, uint8_t *);
    if (!tt) return 0;
    if (g_cand[idx].pl != pl || now < g_cand[idx].t || now - g_cand[idx].t > 30000) {
        g_cand[idx].pl = pl; g_cand[idx].t = now; g_cand[idx].n = 0;
        for (int t = 0; t < 724; t++) {   // type ids end at 723 (data/db/converteroutput.log); the table ends there
            if (!land_soldier(t) || soldier_score(t) <= 0) continue;   // no attack: never a soldier
            uint8_t *tech = ((uint8_t *(__thiscall *)(void *, int))0x004019A1)(tt, FIELD(((uint8_t **)0x009418A0)[t], 0x4E0, int));
            if (!tech || !tech[4]) continue;
            int n = g_cand[idx].n, k = n < 48 ? n : 47;   // insert sorted by score, strongest first
            while (k > 0 && soldier_score(g_cand[idx].type[k - 1]) < soldier_score(t)) {
                if (k < 48) g_cand[idx].type[k] = g_cand[idx].type[k - 1];
                k--;
            }
            if (k < 48) g_cand[idx].type[k] = t;
            if (n < 48) g_cand[idx].n++;
        }
    }
    for (int i = 0; i < g_cand[idx].n; i++) {
        int t = g_cand[idx].type[i], n = want, paid = 0;
        for (int r = 2; r < 7; r++) {
            int c = unit_cost(pl, t, r);
            if (c > 0) { paid = 1; if (vis[r] / c < n) n = vis[r] / c; }
        }
        if (!paid || n <= 0 || !((char (__thiscall *)(void *, int, int, int))0x00555710)(pl, t, 0x20, 0)) continue;
        *got = n;
        return t;
    }
    return 0;
}

// The budget decides before the game does: a request it doesn't allow is refused here and the original is not
// called. (Hiding stock during the call leaked: 0x49014F counts the unit as "in production" (unit manager
// +0x13C) even when the building queue 0x5E8908 then refuses it for lack of resources, and that counter is
// subtracted from the population cap - after a while the AI could train nothing at all.)
static int g_priority_mil;   // set by the siege controller around its own requests
static int g_landing_order;   // set by the landing around its transport order (its reserve is not held from it)
static void landing_reserve(void *pl, int reserve[7]);   // landing.c
// ROOM: the game's own callers ask the unit manager for room first (cap - in production - population, the start check
// of the buildings 0x624DF8 the same); the mod's own orders (AA units, siege, home guard, transports) didn't. At the cap
// they were queued and paid for and could never come out: lowpop (cap 30) - P3's "in production" ran to 56 after it
// got resources, 63 units of AA tanks (146) and siege (105) ordered with no room. Now cut to the room (none: refused).
// [all] train_room: a mask of players (default all; tests: 8 = P3 only). lowpop, 20 seeds, P3 without vs with (P2
// without): P3 kills 23.4 vs 28.2, losses 32.2 vs 28.9 (P2's losses 22.3 vs 28.6). The AI's cap follows the human's
// population: at the cap is common in normal games too.
static int g_train_room = 0xFFFF;
static unsigned g_noroom[16][6];   // units ordered with no room (cap - in production - population <= 0), by call site
static uintptr_t __fastcall hook_train_units(void *um, void *edx, int *req, uintptr_t b) {
    (void)edx;
    void *pl = um_owner(um);
    int *stock = pl ? (int *)((uint8_t *)pl + PL_STOCK) : NULL;
    int before[7] = { 0 }, reserve[7] = { 0 }, visibles[7] = { 0 }, blocked = 0, afford = 0, sub_type = 0;
    if (stock) memcpy(before, stock, sizeof before);
    unsigned idx = pl ? (unsigned)FIELD(pl, PL_INDEX, int) & 15 : 0;
    int military = !is_citizen_type(req[0]);
    int req_unit = military && pl && requirement_unit(pl, req[0]);   // counts for the next epoch
    const struct ai_cfg *cfg = pl ? cfg_of(pl) : NULL;
    if (stock && cfg->budget) {
        uint32_t now = game_time();
        if (now < g_bud[idx].last_t) g_bud[idx].init = 0;   // new game
        g_bud[idx].last_t = now;
        spend_reserve(pl, idx, reserve, NULL);
        reserve[3] += field_wood(idx);   // the next fields' wood (build_fields)
        landing_reserve(pl, reserve);   // transports a landing waits for
        bunker_reserve(pl, reserve);    // a fortress's wood (bunker.c)
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
            int visible = req_unit ? stock[r] : stock[r] - reserve[r];   // a requirement unit: the epoch waits for it
            // citizens before farms: they pay only food, and only what the food budget (stock + reachable wild
            // food) doesn't need for the epochs; a tiny workforce always grows
            if (!military && cfg->copper_plan && current_epoch(pl) < FARM_EPOCH) {
                int wf = workforce(pl);
                if (r != 2 || wf < 10) visible = stock[r];
                else if (wf < g_min_workforce && stock[2] + (int)(wild_food(pl) * g_wild_share) >= reserve[2] + 500)
                    visible = stock[2];   // enough food on the map: grow the economy first
                // else: workforce built (or the map is poor) -> save: the full reserve applies
            }
            if (military && !g_priority_mil && !req_unit) {   // priority requests (siege) and requirement units spend
                                                               // the stock above the reserve
                int allowed = (int)g_bud[idx].mil[r];
                int excess = stock[r] - reserve[r] - (r >= 4 ? PV(idx, g_v_hoard_mineral, g_hoard_cap_mineral) : g_hoard_cap);   // never hoard beyond the cap
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
        int trainable = military && g_unit_sub && !g_priority_mil && substitutable(req[0]) &&
                        ((char (__thiscall *)(void *, int, int, int))0x00555710)(pl, req[0], 0x20, 0);
        int sub = military && g_unit_sub && !g_priority_mil && substitutable(req[0]) && (blocked || !trainable);
        if (sub && trainable) {   // the budget refused a unit it can train: save for it first
            if (!g_save_since[idx] || now < g_save_since[idx]) g_save_since[idx] = now ? now : 1;
            int sw = PV(idx, g_v_sub_wait, g_sub_wait);
            sub = !sw || now - g_save_since[idx] >= (uint32_t)sw * 1000u || base_threatened(idx);
            if (!sub) g_mil[idx].saving++;
        }
        if (sub) {
            int got = 0, alt = substitute_soldier(pl, idx, visibles, want, &got);
            if (alt) { sub_type = alt; afford = got; blocked = 0; g_mil[idx].subs++; }
        }
    } else if (stock) memcpy(visibles, stock, sizeof visibles);
    uintptr_t res = 0;
    if (blocked) { if (b) *(unsigned *)b = 0; }
    else {
        int saved_count = req[1], saved_type = req[0];
        if (sub_type) req[0] = sub_type;
        if (afford > 0 && (afford < req[1] || sub_type)) req[1] = afford;   // the caller reads back how many via *b
        int room0 = pl ? ((int (__thiscall *)(void *, int))(*(void ***)pl)[31])(pl, 0) - FIELD(um, 0x13C, int) -
                         ((int (__thiscall *)(void *, int, int))0x00556E11)(pl, 1, 0) : 1;
        uintptr_t ra0 = (uintptr_t)__builtin_return_address(0) - 5;
        int game_site = 0;
        for (int k = 0; k < 5; k++) game_site |= k_train_sites[k] == ra0;
        unsigned got0 = 0;
        if (!b) b = (uintptr_t)&got0;
        if (!game_site && (g_train_room >> idx & 1) && room0 < (req[1] > 0 ? req[1] : 1)) {   // the mod's own order: no more than the room
            if (room0 <= 0) { *(unsigned *)b = 0; req[1] = saved_count; req[0] = saved_type; goto trained; }
            req[1] = room0;
        }
        res = ((uintptr_t (__thiscall *)(void *, int *, uintptr_t))FN_TRAIN_UNITS)(um, req, b);
        if (room0 <= 0 && *(unsigned *)b) {
            uintptr_t ra = (uintptr_t)__builtin_return_address(0) - 5;
            int site = 5;
            for (int k = 0; k < 5; k++) if (k_train_sites[k] == ra) site = k;
            g_noroom[idx][site] += *(unsigned *)b;
            if (site == 5 && g_log) fprintf(g_log, "# noroom: P%u %u x type %d from the mod's call at %p (room %d)\n", idx, *(unsigned *)b, req[0], (void *)ra, room0);
        }
        req[1] = saved_count;
        req[0] = saved_type;
    }
trained:
    if (stock) {
        int any = 0;
        for (int r = 2; r < 7; r++) {
            int spent = before[r] - stock[r];
            any |= spent > 0;
            if (cfg->budget && military && spent > 0) { g_bud[idx].mil[r] -= spent; g_bud[idx].mil_spent[r] += spent; }
        }
        if (military && any && !sub_type) g_save_since[idx] = 0;   // a requested soldier trained: saved enough
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
    g_hoard_cap_mineral = cfg_int(aimod_ini, autotest_ini, 1, "hoard_cap_mineral", 200);
    g_min_workforce = cfg_int(aimod_ini, autotest_ini, 1, "min_workforce", 40);
    g_wild_share = cfg_float(aimod_ini, autotest_ini, 1, "wild_food_share", "0.6");
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
    if (autotest_ini) g_tech_log = GetPrivateProfileIntA("autotest", "tech_log", 0, autotest_ini);
    if (g_tech_log) {
        void **slot = (void **)0x00830874;
        DWORD old;
        if (*slot == (void *)FN_TECH_GRANT && VirtualProtect(slot, 4, PAGE_READWRITE, &old)) {
            *slot = (void *)hook_tech_grant;
            VirtualProtect(slot, 4, old, &old);
        }
    }
    g_more_granaries = cfg_int(aimod_ini, autotest_ini, 1, "more_granaries", 1);
    g_unit_sub = cfg_int(aimod_ini, autotest_ini, 1, "unit_substitution", 1);
    g_sub_wait = cfg_int(aimod_ini, autotest_ini, 1, "substitute_wait", 90);
    g_gather_research = cfg_int(aimod_ini, autotest_ini, 1, "gather_research", 1);
    if (autotest_ini) {
        char vp[32];
        GetPrivateProfileStringA("autotest", "variant_players", "", vp, sizeof vp, autotest_ini);
        for (char *c = vp; *c; c++) if (*c >= '0' && *c <= '9') g_var_mask |= 1u << (*c - '0');
        g_v_late = GetPrivateProfileIntA("autotest", "v_late_targets", -1, autotest_ini);
        g_v_hoard_mineral = GetPrivateProfileIntA("autotest", "v_hoard_cap_mineral", -1, autotest_ini);
        g_v_sub_wait = GetPrivateProfileIntA("autotest", "v_substitute_wait", -1, autotest_ini);
        if (g_var_mask && g_log)
            fprintf(g_log, "# variant: players %s play B: late_targets %d hoard_cap_mineral %d substitute_wait %d "
                    "(A: %d %d %d)\n", vp, g_v_late >= 0 ? g_v_late : g_late_targets, g_v_hoard_mineral >= 0 ? g_v_hoard_mineral :
                    g_hoard_cap_mineral, g_v_sub_wait >= 0 ? g_v_sub_wait : g_sub_wait, g_late_targets, g_hoard_cap_mineral,
                    g_sub_wait);
    }
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
    int up_ok = patch_call(0x0043D365, 0x005BDDBD, hook_epoch_upgrades);
    if (g_log) fprintf(g_log, "# paid upgrades: %d (hook %d)\n", g_paid_upgrades, up_ok);
    int pop_ok = patch_call(0x004311E5, FN_POP_CAP, hook_pop_cap) + patch_call(0x0043FA9B, FN_POP_CAP, hook_pop_cap) +
                 patch_call(0x00474AF0, FN_POP_CAP, hook_pop_cap);
    if (g_log) fprintf(g_log, "# population limit (easy/medium/hard): %s/%s/%s (hooks %d/3)\n", g_cfg[0].pop_follow ? "follows the humans" : "game limit",
                       g_cfg[1].pop_follow ? "follows the humans" : "game limit", g_cfg[2].pop_follow ? "follows the humans" : "game limit", pop_ok);
    for (size_t i = 0; i < sizeof k_train_sites / sizeof *k_train_sites; i++)
        ok += patch_call(k_train_sites[i], FN_TRAIN_UNITS, hook_train_units);
    if (g_log) fprintf(g_log, "# economy hooks: %d/%d\n", ok, (int)(sizeof k_train_sites / sizeof *k_train_sites) + 1);
}

// a new game (the game start hook, before persist_load restores g_bud): the last game's counts, times and slots go -
// the build fix's type slots filled up over a session, g_last_attack_time made a fake "# attack:" (and the chat line)
// at every game start, the income baselines of rates_update and gather_log came out negative
static void econ_new_game(void) {
    memset(g_bud, 0, sizeof g_bud);
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
