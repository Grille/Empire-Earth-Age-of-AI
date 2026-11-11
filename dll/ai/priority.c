// THE PRIORITY LIST (the player 2026-11-09: "we should really polish the priorethy system to handle such things"; 2026-11-08:
// "a lower tier only gets produced if the resources can pay for the higher tiers"). One row for each thing the AI saves
// for, the most important first. Something new to save for is a row here and nothing else: its function says what it
// still needs, skip / only say who has to leave that money alone. Every spender asks this list: a unit order through
// train_budget (econ.c), a building through prio_refuses, the army's plan (army.c am_reserve) for what lies unused.
// A refused building is the game's own failed placement: the round puts the entry at the back of its queue and ends.
// A PERSONALITY'S LIST (priorities = { ... }, voice/persona.c, with ai_style=1; the player 2026-11-09: customizable, by
// personality): a row's name with a condition switches that row on or off for now; a building's identifier with how many
// it wants is an entry of the row "wants", in the file's order, one at a time: its cost held back like the silos', the
// game asked to build it. Both worked out every 10 s. A file without the record: the list as it stands here.
#define S_MIL        1u     // a military unit
#define S_SHIP       2u     // a ship
#define S_REQ        4u     // a unit the next epoch requires
#define S_ORDER      8u     // a priority order: siege, a landing's fleet and transports, an outpost's transport
#define S_TR_ECON    16u    // transports_economy: the game's transport for its crossings
#define S_PLAN       32u    // army_plan's look at what lies unused
#define S_BUILD      64u    // a building
#define S_BUILD_ECON 128u   // ... that is the economy's own: a town, food, a house, a dock
#define S_WANTED     256u   // ... that is the wanted entry itself
#define S_AIR        512u   // anti-air while its air threat is real: an AA building or unit, the building that makes its AA units

// THE ROW "air" (defence.c STRONG ANTI-AIR, aa_strong; the player 2026-11-11: AA far too weak - in the test the AA
// buildings waited for the farms while the town was bombed, "27 buildings held back ... type 145 for the farms"): while
// its air threat is real (aircraft seen over its towns, or an enemy airport it knows) the AA it still wants is the first
// row - its cost held from every other spender -, and its AA (S_AIR) waits for no row.
internal int g_aa_urgent[16];      // the threat is real now
internal int g_aa_hold[16][7];     // what its AA still wants (a block's buildings, a round of units)
internal int g_aa_maker[16];       // the building it asks for to make its AA units (0 none)
internal int prio_air_type(unsigned idx, int type) {
    idx &= 15;
    if (!g_aa_urgent[idx] || type <= 0) return 0;
    ee_unit_type *td = type_data(type);
    int f = td ? unit_type_get_family(td) : -1;
    return f == KIND_LAND_AA || f == KIND_AA_BUILDING || type == g_aa_maker[idx];
}

internal int g_spend_tiers = 0;     // [ai_base] spend_tiers: buildings wait for an outpost too (expand_hold)
internal int g_farm_hold[16];       // the wood the next silo and its fields still need (econ.c farm_plan)
#define PRIO_WANTS 12
#define PRIO_WAIT 360000           // an entry without progress for this long (no room, no builders): the next one's turn
internal struct { int type, n; } g_prio_want[16][PRIO_WANTS];
internal int g_prio_nwant[16];
internal int g_prio_cur[16];        // the entry whose turn it is: its type + 1, 0 none
internal unsigned g_prio_off[16];   // the rows this AI's personality has off for now
internal void (*g_prio_refresh)(ee_player *pl, unsigned idx);   // voice/persona.c: the file's values, now

internal void spend_reserve(ee_player *pl, unsigned idx, int reserve[7], float *horizon);   // econ.c, its parts below
internal int field_wood(unsigned idx);
internal void upgrade_reserve(ee_player *pl, int reserve[7]);
internal void special_reserve(ee_player *pl, int reserve[7]);
internal int unit_cost(ee_player *pl, int type, int r);
internal void landing_reserve(ee_player *pl, int reserve[7]);   // landing.c
internal void expand_reserve(ee_player *pl, int reserve[7]);    // expand.c
internal void expand_hold(ee_player *pl, int hold[7]);
internal void bunker_reserve(ee_player *pl, int *reserve);      // bunker.c
internal void wood_reserve(ee_player *pl, int reserve[7]);      // mines.c
internal void navy_hold(ee_player *pl, unsigned idx, int reserve[7]);   // navy.c

#define PRIO_IDX ((unsigned)player_get_index(pl) & 15)
internal void prio_air(ee_player *pl, unsigned s, int r[7]) { (void)s; if (g_aa_urgent[PRIO_IDX]) for (int k = 2; k < 7; k++) r[k] += g_aa_hold[PRIO_IDX][k]; }
internal void prio_epoch(ee_player *pl, unsigned s, int r[7]) { (void)s; spend_reserve(pl, PRIO_IDX, r, NULL); }
// units: the next fields' wood, or the silo's whole sum while it has its turn; a building only yields to the silo
internal void prio_farms(ee_player *pl, unsigned s, int r[7]) { r[3] += s & S_BUILD ? g_farm_hold[PRIO_IDX] : field_wood(PRIO_IDX); }
internal void prio_wants(ee_player *pl, unsigned s, int r[7]) {
    (void)s;
    int t = g_prio_cur[PRIO_IDX] - 1;
    for (int k = 2; k < 7 && t >= 0; k++) r[k] += unit_cost(pl, t, k);
}
internal void prio_landing(ee_player *pl, unsigned s, int r[7]) { (void)s; landing_reserve(pl, r); }
internal void prio_outpost(ee_player *pl, unsigned s, int r[7]) {
    if (!(s & S_BUILD)) expand_reserve(pl, r);
    else if (g_spend_tiers) expand_hold(pl, r);
}
internal void prio_fortress(ee_player *pl, unsigned s, int r[7]) { (void)s; bunker_reserve(pl, r); }
internal void prio_forest(ee_player *pl, unsigned s, int r[7]) { (void)s; wood_reserve(pl, r); }
internal void prio_upgrades(ee_player *pl, unsigned s, int r[7]) { (void)s; upgrade_reserve(pl, r); }
internal void prio_navy(ee_player *pl, unsigned s, int r[7]) { (void)s; navy_hold(pl, PRIO_IDX, r); }
internal void prio_special(ee_player *pl, unsigned s, int r[7]) { (void)s; special_reserve(pl, r); }

internal const struct prio_row {
    const char *name;
    void (*hold)(ee_player *pl, unsigned spender, int r[7]);
    unsigned skip, only;   // skip: these spenders may use it; only: if set, no others have to leave it alone
    int apart;             // a warship with a target sees the stock but this (train_budget)
} g_prio[] = {
    { "air",       prio_air,      S_AIR,                                         0,              0 },
    { "epoch",     prio_epoch,    S_BUILD,                                       0,              0 },
    { "farms",     prio_farms,    S_TR_ECON | S_BUILD_ECON,                      0,              0 },
    { "wants",     prio_wants,    S_TR_ECON | S_BUILD_ECON | S_WANTED,           0,              0 },
    { "landing",   prio_landing,  S_TR_ECON | S_BUILD,                           0,              0 },
    { "outpost",   prio_outpost,  S_TR_ECON | S_PLAN,                            0,              0 },
    { "fortress",  prio_fortress, S_TR_ECON | S_BUILD,                           0,              0 },
    { "forest",    prio_forest,   S_TR_ECON | S_PLAN | S_BUILD,                  0,              0 },
    { "upgrades",  prio_upgrades, S_BUILD,                                       S_MIL | S_PLAN, 0 },
    { "navy",      prio_navy,     S_SHIP | S_REQ | S_ORDER | S_TR_ECON | S_BUILD, 0,             0 },
    { "special",   prio_special,  S_REQ | S_ORDER | S_BUILD,                     S_MIL,          1 },
};
#define PRIO_N ((int)(sizeof g_prio / sizeof g_prio[0]))

internal int prio_applies(unsigned idx, int row, unsigned s) {
    const struct prio_row *w = &g_prio[row];
    if (s & S_AIR) return 0;   // anti-air under a real threat waits for nothing
    return !(g_prio_off[idx] >> row & 1) && !(w->skip & s) && (!w->only || (w->only & s));
}
// what the rows ask this spender to leave alone, added up in reserve (the rows marked apart in apart instead, if given)
internal void prio_hold(ee_player *pl, unsigned spender, int reserve[7], int apart[7]) {
    for (int n = 0; n < PRIO_N; n++)
        if (prio_applies(PRIO_IDX, n, spender)) g_prio[n].hold(pl, spender, g_prio[n].apart && apart ? apart : reserve);
}
// BUILDINGS (spend_tiers for the outpost's row, default 0 until measured): the savings only ever held back units - a
// building is paid the moment the build manager's round (every 3 s) places it, so an outpost's transport (250 wood)
// waited 7-11 minutes for wood the buildings spent as it came in, with the whole stock open to it. A building that would
// dip into what a row above it needs waits. Per resource: a tower of stone goes up while wood is held.
internal int prio_refuses(ee_player *pl, int type) {
    static unsigned refused[16];
    static uint32_t log_t[16];
    unsigned idx = PRIO_IDX, s = S_BUILD;
    if (!g_spend_tiers && !g_farm_hold[idx] && !g_prio_cur[idx] && !g_aa_urgent[idx]) return 0;
    if (type == g_prio_cur[idx] - 1) s |= S_WANTED;
    if (prio_air_type(idx, type)) s |= S_AIR;
    if (UT_IN(type, UT_TOWNS, UT_B_GRANARY, UT_B_GRANARY_IMPROVED, UT_B_FARM, UT_B_HOUSE, UT_B_DOCK)) s |= S_BUILD_ECON;
    int hold[7] = { 0 }, *stock = player_stock_at(pl), held = -1, row = 0;
    for (; row < PRIO_N && held < 0; row++) {
        if (!prio_applies(idx, row, s)) continue;
        g_prio[row].hold(pl, s, hold);
        for (int r = 2; r < 7 && held < 0; r++) {
            int c = unit_cost(pl, type, r);
            if (c > 0 && hold[r] > 0 && stock[r] - c < hold[r]) held = r;
        }
    }
    if (held < 0) return 0;
    uint32_t now = game_time();
    if (now < log_t[idx]) { log_t[idx] = 0; refused[idx] = 0; }   // a new game
    refused[idx]++;
    if (g_log && now - log_t[idx] >= 60000) {
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# priority: P%u at %.1f min: %u buildings held back so far, now type %d for the %s - resource %d: stock %d, held %d\n", idx, now / 60000.0,
                refused[idx], type, g_prio[row - 1].name, held, stock[held], hold[held]);
        LeaveCriticalSection(&g_lock);
        log_t[idx] = now ? now : 1;
    }
    return 1;
}
// the file's names, for voice/persona.c: a row's number by its name (-1: none)
internal int prio_row(const char *name) {
    for (int n = 0; n < PRIO_N; n++) if (!lstrcmpiA(name, g_prio[n].name)) return n;
    return -1;
}
internal void prio_row_set(unsigned idx, int row, int on) { if (on) g_prio_off[idx] &= ~(1u << row); else g_prio_off[idx] |= 1u << row; }
internal void prio_want_add(unsigned idx, int type, int n) {
    if (type >= 0 && n > 0 && g_prio_nwant[idx] < PRIO_WANTS) { g_prio_want[idx][g_prio_nwant[idx]].type = type; g_prio_want[idx][g_prio_nwant[idx]++].n = n; }
}
internal int current_epoch(ee_player *pl);   // econ.c
internal int prio_buildable(ee_player *pl, int t) {   // the game offers it to this player now
    ee_unit_type *td = type_data(t);
    ee_tech_tree *tt = player_get_tech_tree(pl);
    ee_tech *x = td && tt ? tech_tree_lookup(tt, unit_type_get_tech(td)) : NULL;
    return x && tech_available(x) && tech_get_epoch(x) <= current_epoch(pl);   // not can_build: it tests the cost too, nothing would be saved for
}
internal void prio_wants_tick(ee_player *pl, unsigned idx, uint32_t now) {
    static struct { uint32_t next, asked; int cur; struct { int have; uint32_t since; } w[PRIO_WANTS]; } st[16];
    if (now < st[idx].next && st[idx].next - now <= 10000) return;
    if (now + 10000 < st[idx].next) memset(&st[idx], 0, sizeof st[idx]);   // a new game
    st[idx].next = now + 10000;
    g_prio_off[idx] = 0;
    g_prio_nwant[idx] = 0;
    if (g_prio_refresh) g_prio_refresh(pl, idx);
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    void *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    int cur = -1, cur_have = 0, cur_n = 0;
    for (int k = 0; um && k < g_prio_nwant[idx]; k++) {
        int t = g_prio_want[idx][k].type, have = 0;
        uint8_t *q = type_list(um, t);
        for (unsigned j = 0, f = q ? deque_front(q) : 0, c = q ? deque_count(q) : 0; j < c; j++) {
            ee_unit *u = deque_at(q, f + j);
            have += u && unit_get_hp(u) > 0;
        }
        // the wait runs only while it is this entry's turn: one further down waits for those above it first
        if (cur >= 0 || have != st[idx].w[k].have || !st[idx].w[k].since || now < st[idx].w[k].since) { st[idx].w[k].have = have; st[idx].w[k].since = now ? now : 1; }
        if (cur >= 0 || have >= g_prio_want[idx][k].n || now - st[idx].w[k].since > PRIO_WAIT || !prio_buildable(pl, t)) continue;
        cur = t; cur_have = have; cur_n = g_prio_want[idx][k].n;
    }
    g_prio_cur[idx] = cur + 1;
    if (cur != st[idx].cur - 1 && g_log) log_locked("# priority: P%u at %.1f min: its list wants %s (%d of %d)\n", idx, now / 60000.0, cur >= 0 ? unit_ident(cur) : "nothing now", cur_have, cur_n);
    st[idx].cur = cur + 1;
    if (cur >= 0 && bm && !build_mgr_queued(bm, cur) && (!st[idx].asked || now < st[idx].asked || now - st[idx].asked >= 30000)) {
        st[idx].asked = now ? now : 1;
        build_mgr_request_one(bm, cur);
    }
}
internal void prio_new_game(void) {
    memset(g_prio_cur, 0, sizeof g_prio_cur);
    memset(g_prio_off, 0, sizeof g_prio_off);
    memset(g_prio_nwant, 0, sizeof g_prio_nwant);
}
