// ARMY SPENDING (docs/economy.md "Economy into army"). Included by eeaimod.c after citizens.c.
// The playtester: "later everything it can get hold of should be exploited and converted into better attack and
// defence capability". citizen_plan brought +15..+35 workers and +700..+2000 stone/gold/iron an hour, but no bigger
// army (Continental without the cheat 8 soldiers fewer).
// "# mil:" once a game minute (autotest; the mod's AIs and the vanilla players alike - nothing here acts): the stock
// F/W/S/G/I, the military account (g_bud, econ.c), the army (soldiers, siege, ships, aircraft: families as
// is_army_family, attack.c) and its value (the units' prices without the Cheat - the tech data's costs), the military
// production buildings finished by kind (Barracks, Archery Range, Stable, Siege Factory, Dock, Navy Yard, Airport,
// Tank Factory, Cyber Factory, Fortress) and the share of 5-second samples they were training, towers and wall pieces
// (finished), the techs researched (all / the line upgrades, category 7), the game's unit wishes (soldiers waiting in the
// unit manager's 14 request lists), population / cap / room, and this minute's military training requests through
// 0x49014F: calls, units asked, trained, and the refused calls by why - budget (the stock pays one, the budget's
// visible part doesn't), poor (the stock itself can't), room (no population room, or the fleet's / landing's room
// held), game (the game refused it: no building, not researched, queue full), substituted.
internal const enum ut_name k_am_bld[] = { UT_B_BARRACKS, UT_B_ARCHERY_RANGE, UT_B_STABLE, UT_B_SIEGE_FACTORY, UT_B_DOCK,
    UT_B_NAVY_YARD, UT_B_AIRPORT, UT_B_TANK_FACTORY, UT_B_CYBER_FACTORY, UT_B_FORTRESS };
#define AM_NK ((int)(sizeof k_am_bld / sizeof *k_am_bld))
struct am_min { unsigned calls, asked, trained, budget, poor, room, game, subs, short_[7]; };
internal struct am_st {
    uint32_t t, ts;            // last minute line, last busy sample
    unsigned min;
    struct am_min m;
    unsigned samples, busy, have;   // busy samples: building-samples training / all
    unsigned last_short[7];         // the last minute's budget shortages by resource (MIX)
} g_am[16];
internal void am_new_game(void) { memset(g_am, 0, sizeof g_am); }

// the price of a type without the Cheat (unit_cost's tech data, no AdjustCost)
internal int am_raw_cost(ee_player *pl, int type) {
    ee_unit_type *obj = type_data(type);
    ee_tech_tree *tt = player_get_tech_tree(pl);
    if (!obj || !tt) return 0;
    ee_tech *tech = tech_lookup(tt, type_get_tech(obj));
    ee_tech_data *data = tech ? tech_get_data(tech) : NULL;
    int c = 0;
    for (int r = 2; data && r < 7; r++) c += techdata_get_cost(data, r);
    return c;
}
// a military training request through hook_train_units (econ.c), after the call: n trained
internal void am_train_note(ee_player *pl, unsigned idx, const int *req, int blocked, int sub_type, unsigned n, const int *stock, const int *vis) {
    struct am_min *m = &g_am[idx & 15].m;
    m->calls++;
    m->asked += (unsigned)(req[1] > 0 ? req[1] : 1);
    m->trained += n;
    if (sub_type) m->subs++;
    if (n) return;
    if (blocked) {
        int poor = 0;
        for (int r = 2; stock && r < 7; r++) poor |= stock[r] < unit_cost(pl, req[0], r);
        if (poor) m->poor++; else m->budget++;
        for (int r = 2; vis && r < 7; r++) m->short_[r] += vis[r] < unit_cost(pl, req[0], r);   // the budget's short resources
    } else if (g_am_noroom) m->room++;
    else m->game++;
}
// the production buildings of kind k: finished ones, those training
internal void am_bld(void *um, int type, int *have, int *busy) {
    uint8_t *q = type_list(um, type);
    *have = *busy = 0;
    for (unsigned j = 0, f = q ? FIELD(q, 0xC, unsigned) : 0, c = q ? FIELD(q, 0x10, unsigned) : 0; j < c && j < 128; j++) {
        ee_unit *b = deque_at(q, f + j);
        ee_unit *bs = b ? unit_get_physics(b) : NULL;
        if (!b || unit_get_hp(b) <= 0 || !bs || !unit_get_finished(bs)) continue;
        (*have)++;
        int cur = unit_get_producing(b), *qb = unit_get_queue_begin(b), *qe = unit_get_queue_end(b);
        if ((cur > 0 && cur != 100000) || (qb && qe > qb)) (*busy)++;
    }
}
// soldiers the game wants: the unit manager's 14 request lists (um +0xC + i*0x14 deques of 16-byte {type, count, ..},
// 256 a block - 0x4910BD), every entry but citizens
internal int am_wishes(ee_unit_mgr *um) {
    int n = 0;
    for (int i = 0; i < 14; i++) {
        uint8_t *dq = um_request_at(um, i);
        unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned), mapsz = FIELD(dq, 8, unsigned);
        int **map = FIELD(dq, 4, int **);
        for (unsigned k = 0; map && k < cnt && k < 512; k++) {
            unsigned off = first + k, blk = off >> 8;
            if (mapsz && blk >= mapsz) blk -= mapsz;
            int *e = (int *)((uint8_t *)map[blk] + (off & 255) * 16);
            if (e && !is_citizen_type(e[0]) && e[1] > 0) n += e[1];
        }
    }
    return n;
}
// what a land soldier's request sees held back (econ.c train_budget): the epoch, the fields' wood, a landing's
// transports, a fortress's wood, the navy account
internal void am_reserve(ee_player *pl, unsigned idx, int reserve[7]) {
    spend_reserve(pl, idx, reserve, NULL);
    reserve[3] += field_wood(idx);
    landing_reserve(pl, reserve);
    bunker_reserve(pl, reserve);
    navy_hold(pl, idx, reserve);
}
// ---- army_plan ([all], bits; [autotest] v_army_plan for the variant players; docs/economy.md "Economy into army") ----
// From the Copper Age (farms: food no longer runs out), what the economy brings in and nobody spends becomes army:
// 1 FLOAT: every 10 s the stock above everything held back (am_reserve: the epoch, fields, landing, fortress, navy) is
//   watched for 2 minutes; what never dropped below that (money lying unused - the citizens, buildings and techs had
//   their chance) less a little for them (army_float_keep: 300 food/wood, army_float_keep_mineral: 100 stone/gold/iron)
//   tops up the military account, so the budget lets the army spend it.
// 2 FILL: the game trains soldiers only for its requests (the next attack's group, the home guard) - with the money and
//   the population room the army stays as it is. Every 10 s while the account pays a soldier: the room under the cap
//   less the citizens still missing (citizen_plan's goal), the fleet's and landing's room and 2 -> up to 5 more through
//   the train hook (the budget applies): the type the game itself asks for (its request lists) when it can train it,
//   else the strongest it can (substitution).
// 4 BUILD: every 30 s: the land production buildings (Barracks, Archery Range, Stable, Tank / Cyber Factory) trained
//   in 70%+ of the samples since the last look and the account holds 600+ -> one more of the kind that trains the
//   soldier FILL picks, up to 2 + workers / 15 of them (the build manager's own queue and placement, 0x46802D).
// 8 MIX, 16 UPGRADE: below. Only with an enemy on the AI's own continent (navy_detect: land or mixed maps).
// 32 LATE: FLOAT, FILL and BUILD only from army_plan_epoch (5, the Middle Ages). Why (docs/economy.md "Mines and army on
//   the small maps"): in a game from the Stone Age both sides hold their towns with towers and the Copper and Bronze Age
//   armies have nothing that breaks a tower line - against the base AI 19 of the mod's 28 soldiers lost died under
//   enemy towers (Continental Large 4 of 104, e9 2 of 24); 7 came in with the Copper Age (minute 22-37), added 2-5
//   soldiers and 6 losses, 5 of them under towers (kills - losses -5.5 (1.8)); head-to-head there +18 lost per AI, 13
//   under towers, -16.8 (4.4); the same with a population cap of 500 - not the map's size: Continental e9 gains.
//   Stopping the top-up while half of the last 4+ soldiers lost died under towers changed nothing (-16.6 (4.9)): the
//   trickle starts before the first attack shows the towers. With LATE Continental e0 plays as without, Continental
//   Large and e9 as 7.
// Measured first (# mil, the base AI at hard, 60 min, 3 maps x cheat x 20 seeds x both seats): the mod's stock is spent -
// the military account holds 2000-11000 that the stock never has, the army's orders are refused by the budget 2600-5000
// times from minute 20, short of wood, gold and iron (the stock above the reserve under one soldier's price; the reserves
// are small, 0-560), the production buildings train 12-34% of the time, no room refusals; food and wood pile up only in
// the last 15 minutes. So the economy, not the budget, buildings or room, caps the army.
// Paired against the build without (40 games a cell): 7 on Continental e0 soldiers at 60 min +4.5 (1.6) / +5.3 (2.2) (no
// cheat / cheat), 22 / 11 more trained, kills - losses -0.8 (3.3) / +7.0 (4.6), wins unchanged; on the islands the first
// version (FILL and BUILD everywhere) cost Small Islands e5 soldiers (-5 at 45 min) - now off there. Head-to-head
// (Continental e0, 4 AIs, 24 games): soldiers +4.2 (2.1), kills +1.1 (4.6), losses +14.4 (5.8); lowpop (20 games):
// kills +22.6 (11.1), losses -22.4 (10.2), razed +8.1 (3.0). MIX acted rarely (food seldom lies 500 above the reserve
// for 2 minutes), UPGRADE changed nothing (the game researches the deferred upgrades anyway, up7 unchanged).
// With fall_back (2026-10-22, docs/experiments.md): Continental Large 80 games a cell kills - losses -8.0 (8.2) / +8.1
// (7.2) but the mod's wins 35 -> 61 of 80 without the cheat, the base AI's 66 -> 60 with it; Continental e9 +3.6 (3.0) /
// +6.2 (4.1), wins 55 -> 65, base AI 55 -> 43; head-to-head Continental Large +46.0 (17.5). Default 39 (7 + LATE).
internal int g_army_plan = 39, g_v_army_plan = -1;
internal int g_army_keep[7] = { 0, 0, 300, 300, 100, 100, 100 };
internal int g_army_epoch = 5;   // LATE: army_plan_epoch
#define ARMY_PLAN(idx) PV(idx, g_v_army_plan, g_army_plan)
internal const enum ut_name k_ap_land[] = { UT_B_BARRACKS, UT_B_ARCHERY_RANGE, UT_B_STABLE, UT_B_TANK_FACTORY, UT_B_CYBER_FACTORY };
internal struct ap_st {
    uint32_t t, tb, look;
    int fl[12][7], nf, kf;          // the last 12 samples of stock - reserve
    unsigned busy, have;            // land production samples since the last BUILD look
    unsigned added[7], fill, bld, mixed;   // since the last # mil line
    int last_type;
} g_ap[16];
internal uint8_t g_apo[16][768];   // types owned or wished (ap_owned)
internal uint32_t g_apo_t[16];
internal struct { int id[48], n; } g_apu[16];   // line upgrades scheduled (UPGRADE)
internal void ap_new_game(void) { memset(g_ap, 0, sizeof g_ap); memset(g_apo_t, 0, sizeof g_apo_t); memset(g_apu, 0, sizeof g_apu); }
internal void ap_upgrades(ee_player *pl, unsigned idx);
internal int navy_land(unsigned idx);   // navy.c

// the land soldier type to train with what vis pays: the first the game asks for (its request lists) that it can train
// now and vis pays, else the strongest trainable that vis pays (substitute_soldier)
internal int ap_type(ee_player *pl, unsigned idx, ee_unit_mgr *um, const int *vis) {
    for (int i = 0; i < 14; i++) {
        uint8_t *dq = um_request_at(um, i);
        unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned), mapsz = FIELD(dq, 8, unsigned);
        int **map = FIELD(dq, 4, int **);
        for (unsigned k = 0; map && k < cnt && k < 512; k++) {
            unsigned off = first + k, blk = off >> 8;
            if (mapsz && blk >= mapsz) blk -= mapsz;
            int *e = (int *)((uint8_t *)map[blk] + (off & 255) * 16), ok = 1;
            if (!e || e[1] <= 0 || !land_soldier(e[0]) || soldier_score(e[0]) <= 0) continue;
            for (int r = 2; r < 7; r++) if (vis[r] < unit_cost(pl, e[0], r)) ok = 0;
            if (ok && can_build(pl, e[0])) return e[0];
        }
    }
    int got = 0;
    return substitute_soldier(pl, idx, vis, 1, &got);
}
// the production building of a unit type (its tech's building list, data +0x10..+0x14) among the land kinds
internal int ap_producer(ee_player *pl, int type) {
    ee_unit_type *td = type_data(type);
    ee_tech_tree *tt = player_get_tech_tree(pl);
    ee_tech *t = td && tt ? tech_lookup(tt, type_get_tech(td)) : NULL;
    ee_tech_data *d = t ? tech_get_data(t) : NULL;
    int *b = d ? techdata_get_makers_begin(d) : NULL, *e = d ? techdata_get_makers_end(d) : NULL;
    for (int *p = b; b && e > b && e - b < 16 && p < e; p++)
        for (size_t k = 0; k < sizeof k_ap_land / sizeof *k_ap_land; k++) if (*p == g_ut[k_ap_land[k]]) return *p;
    return 0;
}
internal void army_plan_tick(ee_player *pl, unsigned idx, uint32_t now) {
    int wfa = wf_army(pl, idx), plan = ARMY_PLAN(idx) | wfa;   // citizens.c workforce_plan ARMY: FILL + BUILD at a high cap
    struct ap_st *a = &g_ap[idx];
    if (now < a->t) memset(a, 0, sizeof *a);
    if (!plan || (a->t && now - a->t < 10000)) return;
    a->t = now ? now : 1;
    if ((plan & 16) && (now < a->tb || now - a->tb >= 30000)) { a->tb = now ? now : 1; ap_upgrades(pl, idx); }   // UPGRADE
    // only with an enemy on its own continent (navy.c navy_detect, 10 s in): on the islands (every enemy across the water)
    // the money is the navy's and the landings' - the first plan there (all of it) cost soldiers on Small Islands
    if (current_epoch(pl) < FARM_EPOCH || !cfg_of(pl)->budget || (!navy_land(idx) && !wfa)) return;
    if ((plan & 32) && current_epoch(pl) < g_army_epoch) return;   // LATE
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    ee_unit_mgr *um = cm ? cm_get_unit_mgr(cm) : NULL;
    ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
    if (!um || !bm) return;
    int *stock = player_stock_at(pl);
    if (plan & 9) {   // FLOAT (the 2-minute watch MIX reads too)
        int res[7] = { 0 };
        am_reserve(pl, idx, res);
        for (int r = 2; r < 7; r++) a->fl[a->kf][r] = stock[r] - res[r];
        a->kf = (a->kf + 1) % 12;
        if (a->nf < 12) a->nf++;
        if (a->nf == 12 && (plan & 1))
            for (int r = 2; r < 7; r++) {
                int mn = a->fl[0][r];
                for (int k = 1; k < 12; k++) if (a->fl[k][r] < mn) mn = a->fl[k][r];
                float top = (float)(mn - g_army_keep[r]);
                if (top > g_bud[idx].mil[r]) { a->added[r] += (unsigned)(top - g_bud[idx].mil[r]); g_bud[idx].mil[r] = top; }
            }
    }
    static const int big[7] = { 0, 0, 1 << 20, 1 << 20, 1 << 20, 1 << 20, 1 << 20 };
    int type = (plan & 4) ? ap_type(pl, idx, um, big) : 0;   // BUILD: what the game would train
    if (type) a->last_type = type;
    if (plan & 2) {   // FILL: what the account and the stock above the reserves pay
        int res[7] = { 0 }, vis[7] = { 0 };
        am_reserve(pl, idx, res);
        for (int r = 2; r < 7; r++) {
            vis[r] = stock[r] - res[r];
            if (vis[r] > (int)g_bud[idx].mil[r]) vis[r] = (int)g_bud[idx].mil[r];
        }
        int cap = unit_limit(pl);
        int room = cap - um_get_in_production(um) - population(pl, 1, 0);
        int goal = g_czp[idx].goal ? g_czp[idx].goal : um_citizens_wanted(um);
        int miss = goal - workforce(pl);
        int free = room - (miss > 0 ? miss : 0) - landing_room_reserve(pl) - navy_room_reserve(pl) - 2;
        int ft = free > 0 ? ap_type(pl, idx, um, vis) : 0;
        if (ft) {
            int req[4] = { ft, free < 5 ? free : 5, 0, 0 };
            unsigned n = 0;
            hook_train_units(um, req, (uintptr_t)&n);
            a->fill += n;
        }
    }
    if (plan & 4) {   // BUILD: the samples come with the 10-s tick, a look every 30 s
        int nb = 0;
        for (size_t k = 0; k < sizeof k_ap_land / sizeof *k_ap_land; k++) {
            int h, b;
            am_bld(um, g_ut[k_ap_land[k]], &h, &b);
            a->have += (unsigned)h; a->busy += (unsigned)b; nb += h;
        }
        if (now < a->look) a->look = 0;
        if (now - a->look >= 30000 && a->have >= 3) {
            a->look = now ? now : 1;
            float acc = 0;
            for (int r = 2; r < 7; r++) acc += g_bud[idx].mil[r] > 0 ? g_bud[idx].mil[r] : 0;
            int bt = a->last_type ? ap_producer(pl, a->last_type) : 0;
            int lim = 2 + workforce(pl) / 15;
            if (bt && a->busy * 10 >= a->have * 7 && acc >= 600 && nb < lim &&
                !bm_queued(bm, bt) &&
                can_build(pl, bt)) {
                bm_request(bm, bt);
                a->bld++;
                if (g_log) fprintf(g_log, "# army: P%u at %.1f min: production busy %u/%u, %d buildings, account %.0f - one more %d (for type %d)\n",
                                   idx, now / 60000.0, a->busy, a->have, nb, acc, bt, a->last_type);
            }
            a->busy = a->have = 0;
        }
    }
}
// MIX (8): after the worker targets (hook_compute_targets, after cz_targets): the army's orders are refused for wood,
// gold and iron (soldiers cost food and one of those: knights 60 food 60 gold, long bows 40 wood 30 gold) while food
// piles up. While food lies unused (FLOAT's 2-minute low of food above the reserve at least army_mix_food, 500) and
// the army's orders were refused for wood, gold or iron (10+ times in the last minute or so), up to 8 workers of the
// food target go to the most refused of them with room (wood always, a metal up to its mine slots near drop sites) -
// food keeps two thirds of its target and citizen_plan's FEED; econ_rebalance then moves the gatherers
internal int g_army_mix_food = 500;
internal void ap_targets(ee_resource_mgr *rm) {
    ee_player *pl = rm_player(rm);
    if (!pl) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct ap_st *a = &g_ap[idx];
    if (!(ARMY_PLAN(idx) & 8) || current_epoch(pl) < FARM_EPOCH || a->nf < 12) return;
    int mn = a->fl[0][2];
    for (int k = 1; k < 12; k++) if (a->fl[k][2] < mn) mn = a->fl[k][2];
    if (mn < g_army_mix_food) return;
    uint32_t *target = rm_target_at(rm);
    int floor_ = (int)target[2] * 2 / 3;
    if (g_czp[idx].feed > floor_) floor_ = g_czp[idx].feed;
    int moves = (int)target[2] - floor_;
    if (moves > 8) moves = 8;
    struct am_st *z = &g_am[idx];
    unsigned done = 0;   // resources tried
    while (moves > 0) {
        int best = 0;
        unsigned bs = 9;
        for (int r = 3; r < 7; r++) {
            unsigned sh = z->last_short[r] + z->m.short_[r];
            if (r != 4 && !(done >> r & 1) && sh > bs) { bs = sh; best = r; }
        }
        if (!best) return;
        done |= 1u << best;
        int lim = best == 3 ? 0x7FFFFFFF : (g_mf[idx].slots[best] > rm_count(rm, best) ? g_mf[idx].slots[best] : rm_count(rm, best));
        for (; moves > 0 && (int)target[best] < lim; moves--) { target[2]--; target[best]++; a->mixed++; }
    }
}
// UPGRADE (16): paid_upgrades (econ.c) schedules every line upgrade of an epoch (category 7) at the epoch - each paid at
// its building (Copper Age 3080: 40 food 40 iron), whether the AI has the line or not (the base AI gets them free). Here
// a line upgrade (an effect of kind 0x13: type +0x14 -> +0x18, 0x5BA59D) waits until the AI has a unit or building of
// the old or the new type, or the game asks for one of them (its request lists); every 30 s the waiting ones (available
// +6, not researched +4) whose line is in use are scheduled (0x485F57), each once
internal void ap_owned(ee_player *pl, unsigned idx) {
    uint32_t now = game_time();
    if (g_apo_t[idx] && now >= g_apo_t[idx] && now - g_apo_t[idx] < 5000) return;
    g_apo_t[idx] = now ? now : 1;
    memset(g_apo[idx], 0, sizeof g_apo[idx]);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        int t = type_get_id(it.td);
        if (unit_get_owner(it.u) == pl && unit_get_hp(it.u) > 0 && t >= 0 && t < 768) g_apo[idx][t] = 1;
    }
    ee_civ_mgr *cm = player_get_civ_mgr(pl); ee_unit_mgr *um = cm ? cm_get_unit_mgr(cm) : NULL;
    for (int i = 0; um && i < 14; i++) {
        uint8_t *dq = um_request_at(um, i);
        unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned), mapsz = FIELD(dq, 8, unsigned);
        int **map = FIELD(dq, 4, int **);
        for (unsigned k = 0; map && k < cnt && k < 512; k++) {
            unsigned off = first + k, blk = off >> 8;
            if (mapsz && blk >= mapsz) blk -= mapsz;
            int *e = (int *)((uint8_t *)map[blk] + (off & 255) * 16);
            if (e && e[0] >= 0 && e[0] < 768 && e[1] > 0) g_apo[idx][e[0]] = 1;
        }
    }
}
// a line upgrade's types (effects of kind 0x13): 0 = none, else 1 when one of them is in use
internal int ap_upg_line(ee_player *pl, unsigned idx, ee_tech_data *d, int *used) {
    int ei = techdata_get_effects(d), n = 0;
    *used = 0;
    if (ei < 0 || ei > 0x5DC) return 0;
    ee_epoch_techs *list = epoch_tech_list(ei);
    unsigned cnt = effects_get_count(list), *ids = effects_get_ids(list);
    for (unsigned k = 0; ids && k < cnt && k < 64; k++) {
        if (ids[k] >= 2000) continue;
        ee_effect *ef = tech_effect(ids[k]);
        if (effect_get_kind(ef) != 0x13) continue;
        if (!n++) ap_owned(pl, idx);
        int a = effect_get_type(ef), b = effect_get_new_type(ef);
        if ((a >= 0 && a < 768 && g_apo[idx][a]) || (b >= 0 && b < 768 && g_apo[idx][b])) *used = 1;
    }
    return n;
}
internal int ap_upgrade_later(ee_player *pl, ee_tech_data *d) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    int used;
    if (!(ARMY_PLAN(idx) & 16) || !ap_upg_line(pl, idx, d, &used)) return 0;
    if (used && g_apu[idx].n < 48) g_apu[idx].id[g_apu[idx].n++] = techdata_get_id(d);   // scheduled now (econ.c)
    return !used;
}
internal int ap_rs_has(ee_research_mgr *rs, int id) {   // in one of the research manager's epoch lists (rs +0x18 + e*0x10)
    for (int e = 0; e < 14; e++) {
        int *b = rs_get_epoch_begin(rs, e), *x = rs_get_epoch_end(rs, e);
        for (int *q = b; b && x > b && x - b < 512 && q < x; q++) if (*q == id) return 1;
    }
    return 0;
}
internal void ap_upgrades(ee_player *pl, unsigned idx) {
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    ee_research_mgr *rs = cm ? cm_get_research_mgr(cm) : NULL;
    ee_tech_tree *tt = player_get_tech_tree(pl);
    if (!rs || !tt) return;
    for (int id = 1000; id < 15000; id++) {
        ee_tech *t = tech_lookup(tt, id);
        ee_tech_data *d = t ? tech_get_data(t) : NULL;
        if (!d || tech_get_researched(t) || !tech_get_available(t) || techdata_get_category(d) != 7) continue;
        int used, k;
        if (!ap_upg_line(pl, idx, d, &used) || !used) continue;
        for (k = 0; k < g_apu[idx].n && g_apu[idx].id[k] != id; k++) ;
        if (k < g_apu[idx].n || g_apu[idx].n >= 48 || ap_rs_has(rs, id)) continue;   // scheduled once
        g_apu[idx].id[g_apu[idx].n++] = id;
        rs_schedule(rs, id);
        if (g_log) fprintf(g_log, "# upgrades: P%u at %.1f min: line upgrade %d scheduled - the line is in use now\n", idx, game_time() / 60000.0, id);
    }
}
internal void ap_install(const char *autotest_ini) {
    g_army_plan = g_set_ai[1].army_plan;
    g_army_keep[2] = g_army_keep[3] = g_set_ai[1].army_float_keep;
    g_army_keep[4] = g_army_keep[5] = g_army_keep[6] = g_set_ai[1].army_float_keep_mineral;
    g_army_mix_food = g_set_ai[1].army_mix_food;
    g_army_epoch = g_set_ai[1].army_plan_epoch;
    if (autotest_ini) g_v_army_plan = ini_int("autotest", "v_army_plan", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# army plan: %d (variant %d), keep %d/%d\n", g_army_plan, g_v_army_plan, g_army_keep[2], g_army_keep[4]);
}

// "# pool:" (autotest, minutes 30 and 50): the research manager's candidate techs (rs +8 vector) - id, researched /
// available, category (+0x48), price, and the first effects (kind +0x20 / unit type +0x14 / attribute +0x2C)
internal void am_pool_log(ee_player *pl, unsigned idx) {
    ee_civ_mgr *cm = player_get_civ_mgr(pl); ee_research_mgr *rs = cm ? cm_get_research_mgr(cm) : NULL;
    ee_tech_tree *tt = player_get_tech_tree(pl);
    if (!rs || !tt) return;
    int *b = rs_get_pool_begin(rs), *e = rs_get_pool_end(rs);
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# pool: P%u %d techs:", idx, b && e > b ? (int)(e - b) : 0);
    for (int *q = b; b && q < e && q - b < 300; q++) {
        ee_tech *t = tech_lookup(tt, *q);
        ee_tech_data *d = t ? tech_get_data(t) : NULL;
        if (!d) continue;
        int c = 0;
        for (int r = 2; r < 7; r++) c += techdata_get_cost(d, r);
        fprintf(g_log, " %d(%d%d c%d $%d", *q, tech_get_researched(t), tech_get_available(t), techdata_get_category(d), c);
        int ei = techdata_get_effects(d);
        if (ei >= 0 && ei <= 0x5DC) {
            ee_epoch_techs *list = epoch_tech_list(ei);
            unsigned n = effects_get_count(list), *ids = effects_get_ids(list);
            for (unsigned k = 0; ids && k < n && k < 3; k++) {
                if (ids[k] >= 2000) continue;
                ee_effect *ef = tech_effect(ids[k]);
                fprintf(g_log, " %d/%d/%d", effect_get_kind(ef), effect_get_type(ef), effect_get_attr(ef));
            }
        }
        fputc(')', g_log);
    }
    fputc('\n', g_log);
    LeaveCriticalSection(&g_lock);
}
internal void am_tick(ee_player *pl, uint32_t now) {
    if (!g_autotest || !g_log || !pl) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct am_st *a = &g_am[idx];
    if (now < a->ts) memset(a, 0, sizeof *a);
    ee_civ_mgr *cm = player_get_civ_mgr(pl); ee_unit_mgr *um = cm ? cm_get_unit_mgr(cm) : NULL;
    if (!um) return;
    if (!a->ts || now - a->ts >= 5000) {   // the busy share: a sample every 5 s
        a->ts = now ? now : 1;
        for (int k = 0; k < AM_NK; k++) {
            int h, b;
            am_bld(um, g_ut[k_am_bld[k]], &h, &b);
            a->have += (unsigned)h; a->busy += (unsigned)b;
        }
        a->samples++;
    }
    if (a->t && now / 60000 == a->min) return;
    if (!a->t) { a->t = now ? now : 1; a->min = now / 60000; return; }
    a->t = now ? now : 1;
    int nb[AM_NK], sold = 0, siege = 0, ships = 0, air = 0, towers = 0, walls = 0;
    long value = 0;
    for (int k = 0; k < AM_NK; k++) { int b; am_bld(um, g_ut[k_am_bld[k]], &nb[k], &b); }
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0) continue;
        int t = type_get_id(td), fam = type_get_family(td);
        ee_unit *bs = unit_get_physics(u);
        if (ut_role(t, UTR_TOWER)) { towers += bs && unit_get_finished(bs); continue; }
        if (ut_role(t, UTR_WALL) && !ut_role(t, UTR_GATE)) { walls += bs && unit_get_finished(bs); continue; }
        if (!is_army_family(fam) || is_citizen_type(t) || fam == 18) continue;   // transports aside
        if (fam == 4 || fam == 10 || fam == 21) ships++;
        else if (fam == 3 || fam == 7 || fam == 17) air++;
        else if (is_siege(t) || is_ram(t)) siege++;
        else sold++;
        value += am_raw_cost(pl, t);
    }
    int tech = 0, up7 = 0, up7c = 0;
    ee_tech_tree *tt = player_get_tech_tree(pl);
    for (int id = 1000; tt && id < 15000; id++) {
        ee_tech *t = tech_lookup(tt, id);
        if (!t || !tech_get_researched(t)) continue;
        ee_tech_data *d = tech_get_data(t);
        tech++;
        if (d && techdata_get_category(d) == 7) { up7++; for (int r = 2; r < 7; r++) up7c += techdata_get_cost(d, r); }
    }
    int *st = player_stock_at(pl);
    int cap = unit_limit(pl);
    int pop = population(pl, 1, 0), inprod = um_get_in_production(um);
    struct am_min *m = &a->m;
    int res[7] = { 0 };
    if (!is_vanilla(pl)) am_reserve(pl, idx, res);
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# mil: P%u at %u min: stock %d %d %d %d %d | acc %.0f %.0f %.0f %.0f %.0f | sold %d siege %d ships %d air %d value %ld | "
            "bld %d %d %d %d %d %d %d %d %d %d busy %u/%u | towers %d walls %d | tech %d up7 %d cost %d | wish %d pop %d cap %d room %d | "
            "calls %u asked %u trained %u budget %u poor %u room %u game %u subs %u | short %u %u %u %u %u | res %d %d %d %d %d | "
            "plan float %u %u %u %u %u fill %u bld %u mix %u\n",
            idx, a->min, st[2], st[3], st[4], st[5], st[6], g_bud[idx].mil[2], g_bud[idx].mil[3], g_bud[idx].mil[4], g_bud[idx].mil[5],
            g_bud[idx].mil[6], sold, siege, ships, air, value, nb[0], nb[1], nb[2], nb[3], nb[4], nb[5], nb[6], nb[7], nb[8], nb[9],
            a->busy, a->have, towers, walls, tech, up7, up7c, am_wishes(um), pop, cap, cap - inprod - pop,
            m->calls, m->asked, m->trained, m->budget, m->poor, m->room, m->game, m->subs, m->short_[2], m->short_[3], m->short_[4],
            m->short_[5], m->short_[6], res[2], res[3], res[4], res[5], res[6],
            g_ap[idx].added[2], g_ap[idx].added[3], g_ap[idx].added[4], g_ap[idx].added[5], g_ap[idx].added[6], g_ap[idx].fill, g_ap[idx].bld, g_ap[idx].mixed);
    LeaveCriticalSection(&g_lock);
    memset(g_ap[idx].added, 0, sizeof g_ap[idx].added);
    g_ap[idx].fill = g_ap[idx].bld = g_ap[idx].mixed = 0;
    memcpy(a->last_short, m->short_, sizeof a->last_short);
    memset(m, 0, sizeof *m);
    a->busy = a->have = a->samples = 0;
    if (a->min == 30 || a->min == 50) am_pool_log(pl, idx);
    a->min = now / 60000;
}
