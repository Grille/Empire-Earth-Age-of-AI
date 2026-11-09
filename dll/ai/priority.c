// THE PRIORITY LIST (the player 2026-11-09: "we should really polish the priorethy system to handle such things"; 2026-11-08:
// "a lower tier only gets produced if the resources can pay for the higher tiers"). One row for each thing the AI saves
// for, the most important first. Something new to save for is a row here and nothing else: its function says what it
// still needs, skip / only say who has to leave that money alone. Every spender asks this list: a unit order through
// train_budget (econ.c), a building through prio_refuses, the army's plan (army.c am_reserve) for what lies unused.
// A refused building is the game's own failed placement: the round puts the entry at the back of its queue and ends.
#define S_MIL        1u     // a military unit
#define S_SHIP       2u     // a ship
#define S_REQ        4u     // a unit the next epoch requires
#define S_ORDER      8u     // a priority order: siege, a landing's fleet and transports, an outpost's transport
#define S_TR_ECON    16u    // transports_economy: the game's transport for its crossings
#define S_PLAN       32u    // army_plan's look at what lies unused
#define S_BUILD      64u    // a building
#define S_BUILD_ECON 128u   // ... that is the economy's own: a town, food, a house, a dock

internal int g_spend_tiers = 0;     // [ai_base] spend_tiers: buildings wait for an outpost too (expand_hold)
internal int g_farm_hold[16];       // the wood the next silo and its fields still need (econ.c farm_plan)

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
internal void prio_epoch(ee_player *pl, unsigned s, int r[7]) { (void)s; spend_reserve(pl, PRIO_IDX, r, NULL); }
// units: the next fields' wood, or the silo's whole sum while it has its turn; a building only yields to the silo
internal void prio_farms(ee_player *pl, unsigned s, int r[7]) { r[3] += s & S_BUILD ? g_farm_hold[PRIO_IDX] : field_wood(PRIO_IDX); }
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
    { "epoch",     prio_epoch,    S_BUILD,                                       0,              0 },
    { "farms",     prio_farms,    S_TR_ECON | S_BUILD_ECON,                      0,              0 },
    { "landing",   prio_landing,  S_TR_ECON | S_BUILD,                           0,              0 },
    { "outpost",   prio_outpost,  S_TR_ECON | S_PLAN,                            0,              0 },
    { "fortress",  prio_fortress, S_TR_ECON | S_BUILD,                           0,              0 },
    { "forest",    prio_forest,   S_TR_ECON | S_PLAN | S_BUILD,                  0,              0 },
    { "upgrades",  prio_upgrades, S_BUILD,                                       S_MIL | S_PLAN, 0 },
    { "navy",      prio_navy,     S_SHIP | S_REQ | S_ORDER | S_TR_ECON | S_BUILD, 0,             0 },
    { "special",   prio_special,  S_REQ | S_ORDER | S_BUILD,                     S_MIL,          1 },
};
#define PRIO_N ((int)(sizeof g_prio / sizeof g_prio[0]))

internal int prio_applies(const struct prio_row *w, unsigned s) { return !(w->skip & s) && (!w->only || (w->only & s)); }
// what the rows ask this spender to leave alone, added up in reserve (the rows marked apart in apart instead, if given)
internal void prio_hold(ee_player *pl, unsigned spender, int reserve[7], int apart[7]) {
    for (int n = 0; n < PRIO_N; n++)
        if (prio_applies(&g_prio[n], spender)) g_prio[n].hold(pl, spender, g_prio[n].apart && apart ? apart : reserve);
}
// BUILDINGS (spend_tiers for the outpost's row, default 0 until measured): the savings only ever held back units - a
// building is paid the moment the build manager's round (every 3 s) places it, so an outpost's transport (250 wood)
// waited 7-11 minutes for wood the buildings spent as it came in, with the whole stock open to it. A building that would
// dip into what a row above it needs waits. Per resource: a tower of stone goes up while wood is held.
internal int prio_refuses(ee_player *pl, int type) {
    static unsigned refused[16];
    static uint32_t log_t[16];
    unsigned idx = PRIO_IDX, s = S_BUILD;
    if (!g_spend_tiers && !g_farm_hold[idx]) return 0;
    if (UT_IN(type, UT_TOWNS, UT_B_GRANARY, UT_B_GRANARY_IMPROVED, UT_B_FARM, UT_B_HOUSE, UT_B_DOCK)) s |= S_BUILD_ECON;
    int hold[7] = { 0 }, *stock = player_stock_at(pl), held = -1, row = 0;
    for (; row < PRIO_N && held < 0; row++) {
        if (!prio_applies(&g_prio[row], s)) continue;
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
