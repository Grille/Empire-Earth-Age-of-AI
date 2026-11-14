// RESEARCH BY RANK - the third piece of the mod's own economy, played while the game's computer player is off
// ([ai_base] game_ai = 0; work.c the citizens' work, build.c the buildings). The game's research manager, its random
// planner and its epoch attempt don't run then: nothing was researched at all, no epoch reached.
// The player, 2026-11-14: "research first: prioretyes: 1. all resarch and upgrades (exept hero/unit upgrades) doable in
// capitol/towncenter, universtithy, and hospitals. 2. all unit upgrades of units that actualy exist curently 3. epoch
// (skiped based on mode and conditions) 4. all other available unit upgrades".
// Every RS_EVERY per AI, over every tech of its tree that is available, not researched, of its epoch or an earlier
// one, with a price (the data's "not used" techs have none), no object (a unit or building is no research) and a
// finished building of its that researches it (without one there is nothing to buy or to save for: the first look
// held the price of two line upgrades back for the rest of the game, with no Barracks to research them):
//   RANK 1  - what a Town Center, Capitol, University, Hospital or granary researches that is no unit's upgrade: the
//             gathering techs (first: they pay back), the hospital's and the university's, the granary's (the same kind
//             of thing, left out of his list only by its building), and the walls' and towers' line upgrades (a Town
//             Center's too). The cheapest first. And the temple's range techs (an effect on the Temple itself; the
//             player the same day: "the temple range upgrade is quite important to prevent enemy prophets
//             disasters, thats goe sinto rank 1").
//   RANK 2  - a line upgrade (category 7, an effect of kind 0x13: type -> new type) of a line it fields: RS_FIELDED
//             units of the line's types or more, or one of them on order with none there yet (a new unit comes out
//             as the old type until the upgrade is in). The line with the most units first.
//   RANK 3  - the next epoch, when it is due - the game's own time for it (the civilization manager's, drawn anew
//             here when an epoch is reached: epoch_min .. epoch_max, the human players' epochs by epoch_pacing),
//             at once with rush, with follow once a human is ahead, before the farms with the copper plan; never
//             while follow holds it, never with fewer citizens than min_workforce (the economy first: nothing is
//             saved for it then either) - and when its "new things" are there (the 2 requirements an epoch asks for:
//             while they are missing the cheapest tech that counts is bought in its place).
//   RANK 4  - every other line upgrade (a line it doesn't field) and the temple's other techs (its priests' and
//             prophets': "other temple techs should go in rank 4 since at some point the ai should lern how to use
//             temple units aynways"), the cheapest first, with what no row of the priority list holds back.
// Never: a hero's upgrade (his list; the AI uses no heroes), the units' improvements (attack, hit points ... per
// type: not in the tech tree, not found yet).
// A rank buys only with what the ranks above it don't still wait for: the price of the next RS_HOLD1 of rank 1 and
// RS_HOLD2 of rank 2 it could not buy this look (no money, no free building) is held back - from its own lower
// ranks here, from everybody else by the row "research" of the priority list (priority.c; the epoch has its own
// row; work.c's needs count the due epoch's price: g_rs_epoch). Bought as a player buys: handed to a finished building that makes it (the building's own queue, 0x5E8908:
// the tech tree's checks, the price taken), the one with the least in its queue, none that holds a research already
// or more than RS_QUEUE entries (a Town Center trains citizens: the tech waits behind what is queued).
// A tech marked queued that no building holds at two looks running is set free again (econ.c LOST UPGRADES).
#define RS_EVERY 5000
#define RS_FIELDED 5
#define RS_HOLD1 2
#define RS_HOLD2 4
#define RS_QUEUE 2
#define RS_IDS 15000
#define RS_BLD 128
#define RS_CAND 256
enum { RS_ECON = 1, RS_LINE, RS_EPOCH, RS_REST };
internal struct rsr_st {
    uint32_t t, log_t;
    int ep;                  // its epoch at the last look, + 1 (0: no look yet)
    int left[5], next[5];    // by rank, the last look: techs still to buy; the first of them (-1 none)
    unsigned bought[5], refused, freed;
    int due, miss;           // the epoch: due now; its requirements still missing
} g_rsr[16];
internal uint8_t g_rs_lost[16][RS_IDS / 8 + 1];   // marked queued, in no building at the last look
struct rs_bld { ee_unit *u; int type, load, tech; };
struct rs_cand { int id, rank, score, cost[7], counts; ee_tech *t; };
// a line upgrade's line: 0 no line (no effect of kind 0x13), 1 soldiers or ships, 2 walls and towers, 3 a hero;
// *units: how many of its types it has, *ordered: one of them on order and none there
internal int rs_line(unsigned idx, ee_tech_data *d, const short *cnt, int *units, int *ordered) {
    int ei = tech_data_get_effects(d), kind = 0;
    *units = *ordered = 0;
    if (ei < 0 || ei > 0x5DC) return 0;
    ee_epoch_techs *list = epoch_tech_list(ei);
    unsigned n = epoch_techs_get_count(list), *ids = epoch_techs_get_ids(list);
    for (unsigned k = 0; ids && k < n && k < 64; k++) {
        if (ids[k] >= 2000) continue;
        ee_effect *ef = tech_effect(ids[k]);
        if (effect_get_kind(ef) != 0x13) continue;
        int ab[2] = { effect_get_type(ef), effect_get_new_type(ef) };
        for (int j = 0; j < 2; j++) {
            ee_unit_type *td = ab[j] >= 0 && ab[j] < 768 ? type_data(ab[j]) : NULL;
            if (!td) continue;
            int k2 = unit_type_get_family(td) == KIND_HERO ? 3 : is_wall(ab[j]) || is_tower_type(ab[j]) ? 2 : 1;
            if (k2 > kind) kind = k2;
            *units += cnt[ab[j]];
            if (!cnt[ab[j]] && g_apo[idx][ab[j]]) *ordered = 1;
        }
    }
    return kind;
}
// the building of its that would research d now: a finished one of the tech's makers, the least in its queue
internal struct rs_bld *rs_maker(ee_tech_data *d, struct rs_bld *bl, int nb) {
    int *mb = tech_data_get_makers_begin(d), *me = tech_data_get_makers_end(d);
    struct rs_bld *best = NULL;
    for (int *m = mb; mb && me > mb && me - mb < 64 && m < me; m++)
        for (int k = 0; k < nb; k++)
            if (bl[k].type == *m && !bl[k].tech && bl[k].load <= RS_QUEUE && (!best || bl[k].load < best->load)) best = &bl[k];
    return best;
}
internal int rs_has_maker(ee_tech_data *d, const struct rs_bld *bl, int nb) {
    int *mb = tech_data_get_makers_begin(d), *me = tech_data_get_makers_end(d);
    for (int *m = mb; mb && me > mb && me - mb < 64 && m < me; m++)
        for (int k = 0; k < nb; k++) if (bl[k].type == *m) return 1;
    return 0;
}
internal int rs_made_by(ee_tech_data *d, int type) {
    int *mb = tech_data_get_makers_begin(d), *me = tech_data_get_makers_end(d);
    for (int *m = mb; mb && me > mb && me - mb < 64 && m < me; m++) if (*m == type) return 1;
    return 0;
}
// one of the tech's effects is on that type itself (a temple's range tech: on the Temple)
internal int rs_effect_on(ee_tech_data *d, int type) {
    int ei = tech_data_get_effects(d);
    if (ei < 0 || ei > 0x5DC || type < 0) return 0;
    ee_epoch_techs *list = epoch_tech_list(ei);
    unsigned n = epoch_techs_get_count(list), *ids = epoch_techs_get_ids(list);
    for (unsigned k = 0; ids && k < n && k < 64; k++)
        if (ids[k] < 2000 && effect_get_type(tech_effect(ids[k])) == type) return 1;
    return 0;
}
internal int rs_pays(const int *stock, const int *hold, const int *cost) {
    for (int r = 2; r < 7; r++) if (cost[r] > 0 && stock[r] - hold[r] < cost[r]) return 0;
    return 1;
}
// handed to the building: 1 when it took it (the node is marked queued then)
internal int rs_buy(struct rsr_st *z, struct rs_bld *m, struct rs_cand *c, int rank) {
    unit_enqueue(m->u, c->id, (char)(rank == RS_EPOCH));   // an epoch through the user-input message, as the game's own research manager sends it
    if (tech_get_queueable(c->t)) { z->refused++; return 0; }
    m->tech = 1; m->load++;
    z->bought[rank]++;
    return 1;
}
internal int rs_cmp(const void *a, const void *b) {
    const struct rs_cand *x = a, *y = b;
    return x->rank != y->rank ? x->rank - y->rank : x->score != y->score ? (x->score < y->score ? -1 : 1) : x->id - y->id;
}
internal void research_tick(ee_player *pl, uint32_t now) {
    static struct rs_bld bl[RS_BLD];
    static struct rs_cand cand[RS_CAND];
    static short cnt[768];
    static uint8_t held[RS_IDS / 8 + 1];
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct rsr_st *z = &g_rsr[idx];
    if (now < z->t) { memset(z, 0, sizeof *z); memset(g_rs_lost[idx], 0, sizeof g_rs_lost[idx]); memset(g_rs_hold[idx], 0, sizeof g_rs_hold[idx]); }
    if (z->t && now - z->t < RS_EVERY) return;
    z->t = now ? now : 1;
    ee_tech_tree *tt = player_get_tech_tree(pl);
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    int *stock = player_stock_at(pl);
    if (!tt || !cm || !stock) return;
    int ep = tech_tree_get_epoch(tt), nb = 0, nc = 0, count = 0, ncit = 0;
    uint32_t *timer = civ_mgr_timers_at(cm);
    if (z->ep != ep + 1) {   // an epoch reached (or its first look): the next one's time, as the game draws it - its own draw is in its update
        if (z->ep && *timer <= now) civ_mgr_set_epoch_time(cm, now, 0);
        if (g_log) log_locked("# research: P%u at %.1f min: epoch %d, the next one due at %.1f min\n", idx, now / 60000.0, ep, *timer / 60000.0);
        z->ep = ep + 1;
    }
    memset(cnt, 0, sizeof cnt);
    memset(held, 0, sizeof held);
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {   // what it has: units by type, its finished buildings and their queues
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0) continue;
        int t = unit_type_get_id(td), fam = unit_type_get_class_id(td);
        if (t >= 0 && t < 768 && cnt[t] < 0x7FFF) cnt[t]++;
        ncit += is_citizen_type(t);
        if ((fam != CLS_BUILDING && fam != CLS_FORTRESS) || is_wall(t)) continue;
        ee_unit *st = unit_get_physics(u);
        if (!st || !unit_get_finished(st)) continue;
        int cur = unit_get_producing(u), *qb = unit_get_queue_begin(u), *qe = unit_get_queue_end(u), nq = qb && qe > qb && qe - qb < 4096 ? (int)(qe - qb) : 0;
        int load = cur > 0 && cur != 100000, tech = 0;
        for (int j = -1; j < nq; j++) {
            int id = j < 0 ? (load ? cur : -1) : qb[j];
            if (id < 0 || id >= RS_IDS) continue;
            held[id >> 3] |= (uint8_t)(1 << (id & 7));
            ee_tech *q = tech_tree_lookup(tt, id);
            ee_tech_data *qd = q ? tech_get_data(q) : NULL;
            if (qd && !tech_data_get_is_object(qd)) tech = 1;
        }
        if (nb < RS_BLD) { bl[nb].u = u; bl[nb].type = t; bl[nb].load = load + nq; bl[nb++].tech = tech; }
    }
    ap_owned(pl, idx);   // army.c: g_apo - the types it has or has on order
    g_rs_econ_first[idx] = ep >= FARM_EPOCH && ncit < g_min_workforce;   // econ.c follow_held: no epoch due or saved for yet (before the farms the copper plan has its own rule)
    int next = ep + 1, miss = requirements_missing(pl);
    ee_tech_data *ed = epoch_tech(pl, next);
    int pacing = cfg_of(pl)->pacing;
    int due = ed && !follow_held(pl) && (now >= *timer || pacing == PACE_RUSH || (pacing == PACE_FOLLOW && human_count()) || (copper_mode(pl) && ep < FARM_EPOCH));
    z->due = due; z->miss = miss;
    for (int id = 14; id < RS_IDS; id++) {   // the candidates (0 .. 13: the epochs)
        ee_tech *t = tech_tree_lookup(tt, id);
        ee_tech_data *d = t ? tech_get_data(t) : NULL;
        int inq = held[id >> 3] >> (id & 7) & 1, was = g_rs_lost[idx][id >> 3] >> (id & 7) & 1;
        g_rs_lost[idx][id >> 3] &= (uint8_t)~(1 << (id & 7));
        if (!d || tech_get_researched(t) || !tech_get_available(t) || tech_get_disabled(t) || tech_get_epoch(t) > ep || tech_data_get_is_object(d) || inq) continue;
        int cat = tech_data_get_category(d);
        if (cat != 0 && cat != 7) continue;
        if (cat == 7 && build_is("aoc") && !tech_data_get_aoc_new(d)) continue;   // the civilization powers' and special units' own: as econ.c's upgrades_retry
        if (!tech_get_queueable(t)) {   // marked queued, in no building
            if (!was) { g_rs_lost[idx][id >> 3] |= (uint8_t)(1 << (id & 7)); continue; }
            tech_set_queueable(t, 1); z->freed++;
        }
        struct rs_cand c = { .id = id, .t = t, .counts = tech_data_get_counts(d) };
        int sum = 0, units = 0, ordered = 0;
        for (int r = 2; r < 7; r++) { c.cost[r] = tech_data_get_cost(d, r) > 0 ? tech_data_get_cost(d, r) : 0; sum += c.cost[r]; }
        if (!sum || !rs_has_maker(d, bl, nb)) continue;   // no price: the data's "not used"; no building of its researches it: nothing to save for
        int home = rs_made_by(d, UT(B_TOWN_CENTER)) || rs_made_by(d, UT(B_CAPITOL)) || rs_made_by(d, UT(B_UNIVERSITY)) || rs_made_by(d, UT(B_HOSPITAL)) ||
                   rs_made_by(d, UT(B_GRANARY)) || rs_made_by(d, UT(B_GRANARY_IMPROVED));
        if (cat == 0) {
            if (home || rs_effect_on(d, UT(B_TEMPLE))) { c.rank = RS_ECON; c.score = (is_gather_tech(t) ? 0 : 100000) + sum; }
            else { c.rank = RS_REST; c.score = sum; }   // a temple's other techs
        } else {
            int line = rs_line(idx, d, cnt, &units, &ordered);
            if (line == 3) continue;   // a hero's
            if (line == 2) { c.rank = RS_ECON; c.score = 100000 + sum; }
            else if (line == 1 && (units >= RS_FIELDED || ordered)) { c.rank = RS_LINE; c.score = -units; }
            else { c.rank = RS_REST; c.score = sum; }
        }
        if (nc < RS_CAND) cand[nc++] = c;
    }
    qsort(cand, (size_t)nc, sizeof *cand, rs_cmp);
    int hold[7] = { 0 }, waits[5] = { 0 }, buys = 0;
    int bw = g_rs_want[idx] - 1;   // build.c: the University, Hospital or Temple it saves for comes first - the techs are researched there (not its first production building: research does not wait for that)
    if (bw >= 0 && pr_kind(bw) >= 0) bw = -1;
    for (int r = 2; r < 7; r++) hold[r] = g_bd_com[idx][r] + (bw >= 0 ? unit_cost(pl, bw, r) : 0);   // ... and what build.c has laid and not paid
    memset(z->left, 0, sizeof z->left);
    for (int k = 1; k < 5; k++) z->next[k] = -1;
    char told[200] = "";
    for (int k = 0; k < nc; k++) {   // ranks 1 and 2: what the ranks above don't wait for
        struct rs_cand *c = &cand[k];
        if (c->rank > RS_LINE) break;
        z->left[c->rank]++;
        if (z->next[c->rank] < 0) z->next[c->rank] = c->id;
        struct rs_bld *m = rs_pays(stock, hold, c->cost) ? rs_maker(tech_get_data(c->t), bl, nb) : NULL;
        if (m && buys < 4 && rs_buy(z, m, c, c->rank)) {
            size_t l = strlen(told);
            if (l < sizeof told - 20) snprintf(told + l, sizeof told - l, " %d(rank %d)", c->id, c->rank);
            buys++; c->rank = 0;
            continue;
        }
        if (waits[c->rank]++ < (c->rank == RS_ECON ? RS_HOLD1 : RS_HOLD2)) for (int r = 2; r < 7; r++) hold[r] += c->cost[r];
    }
    for (int r = 2; r < 7; r++) g_rs_hold[idx][r] = hold[r] - g_bd_com[idx][r] - (bw >= 0 ? unit_cost(pl, bw, r) : 0);   // the rows add those themselves
    g_rs_epoch[idx] = due && miss <= 0 ? next : 0;
    // rank 3: the epoch - or, while a requirement is missing, the cheapest tech that counts for it
    ee_tech *et = ed ? tech_tree_lookup(tt, tech_data_get_id(ed)) : NULL;
    int ein = ed && tech_data_get_id(ed) >= 0 && tech_data_get_id(ed) < RS_IDS && (held[tech_data_get_id(ed) >> 3] >> (tech_data_get_id(ed) & 7) & 1);
    if (due && et && !ein && !tech_get_researched(et)) {
        z->left[RS_EPOCH] = 1; z->next[RS_EPOCH] = tech_data_get_id(ed);
        if (miss > 0) {
            struct rs_cand *best = NULL;
            int bsum = 0x7FFFFFFF;
            for (int k = 0; k < nc; k++) {
                int sum = 0;
                for (int r = 2; r < 7; r++) sum += cand[k].cost[r];
                if (cand[k].rank && cand[k].counts && sum < bsum) { bsum = sum; best = &cand[k]; }
            }
            struct rs_bld *m = best && rs_pays(stock, hold, best->cost) ? rs_maker(tech_get_data(best->t), bl, nb) : NULL;
            if (m && rs_buy(z, m, best, RS_EPOCH)) {
                size_t l = strlen(told);
                if (l < sizeof told - 32) snprintf(told + l, sizeof told - l, " %d(for the epoch)", best->id);
                best->rank = 0;
            }
        } else {
            if (!tech_get_queueable(et)) tech_set_queueable(et, 1);   // nothing holds it (ein): lost on its way, as the others
            struct rs_cand c = { .id = tech_data_get_id(ed), .t = et };
            for (int r = 2; r < 7; r++) c.cost[r] = epoch_cost(pl, next, r);
            struct rs_bld *m = rs_pays(stock, hold, c.cost) ? rs_maker(ed, bl, nb) : NULL;
            if (m && rs_buy(z, m, &c, RS_EPOCH)) {
                size_t l = strlen(told);
                if (l < sizeof told - 24) snprintf(told + l, sizeof told - l, " epoch %d", next);
            }
        }
    }
    // rank 4: with what no row of the priority list holds back (the epoch's and this file's among them)
    int rows[7] = { 0 }, any4 = 0;
    for (int k = 0; k < nc; k++) {
        struct rs_cand *c = &cand[k];
        if (c->rank != RS_REST) continue;
        z->left[RS_REST]++;
        if (z->next[RS_REST] < 0) z->next[RS_REST] = c->id;
        if (buys >= 4) continue;
        if (!any4++) prio_hold(pl, S_PLAN, rows, NULL);
        struct rs_bld *m = rs_pays(stock, rows, c->cost) ? rs_maker(tech_get_data(c->t), bl, nb) : NULL;
        if (!m || !rs_buy(z, m, c, RS_REST)) continue;
        buys++;
        for (int r = 2; r < 7; r++) rows[r] += c->cost[r];   // the stock is read again at the next look
        size_t l = strlen(told);
        if (l < sizeof told - 20) snprintf(told + l, sizeof told - l, " %d(rank 4)", c->id);
    }
    if (g_log && told[0])
        log_locked("# research: P%u at %.1f min, epoch %d: handed to its buildings:%s\n", idx, now / 60000.0, ep, told);
    if (g_log && (!z->log_t || now - z->log_t >= 60000)) {
        z->log_t = now ? now : 1;
        log_locked("# research: P%u at %.1f min, epoch %d: to buy rank 1 %d (next %d), rank 2 %d (next %d), rank 4 %d (next %d) | epoch %d: %s, due at %.1f min, requirements missing %d | "
                   "held back F%d W%d S%d G%d I%d | bought so far %u / %u / %u epochs and for them / %u, %u not taken, %u set free\n",
                   idx, now / 60000.0, ep, z->left[1], z->next[1], z->left[2], z->next[2], z->left[4], z->next[4], next, !ed ? "none" : due ? "due" : "not due",
                   *timer / 60000.0, miss, hold[2], hold[3], hold[4], hold[5], hold[6], z->bought[1], z->bought[2], z->bought[3], z->bought[4], z->refused, z->freed);
    }
}
internal void research_new_game(void) { memset(g_rs_epoch, 0, sizeof g_rs_epoch); memset(g_rs_econ_first, 0, sizeof g_rs_econ_first); memset(g_rsr, 0, sizeof g_rsr); memset(g_rs_lost, 0, sizeof g_rs_lost); memset(g_rs_hold, 0, sizeof g_rs_hold); }
