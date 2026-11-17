// THE AIR GUARD - anti-air buildings and fighters while the game's computer player is off ([ai_base] game_ai = 0). The
// player, 2026-11-14, on the force's pools: "3. anti air, pure devensive role, might also get a small share in the
// backlands ... aircraft and ships should probably also be fittet into this" (by role, his call), and 2026-11-15: "do
// the fighters and anti air next". Included by eeaimod.c after stations.c and airstrike.c.
// What is there already: defence.c anti_air counts the enemy aircraft near its towns (g_aa_st: how many, where, when)
// and trains the anti-air units for them; its anti-air buildings go into the build manager's queue, which nobody works
// then. stations.c stations the anti-air units (its pool SNP_AA). Here, every AG_EVERY from the first aircraft's epoch:
//   AA BUILDINGS - while enemy aircraft were seen near its towns in the last AG_SEEN: as many as anti_air wants (1 and
//                  one for every 4 of the anti-air it wants, 4 at most), the newest it can build, by the Settlement,
//                  Town Center or Capitol nearest to where the aircraft were - laid at once (hook_place), one an
//                  AG_BUILD_EVERY.
//   FIGHTERS     - the same while: a fighter (the data's a_fighter types - not the fighter-bombers, the bombers'
//                  wing is airstrike.c's) for every enemy aircraft seen and AG_SPARE more, AG_PER_AIRPORT an Airport of
//                  its at most, the strongest it can train, AG_BATCH an AG_TRAIN_EVERY - with an Airport standing
//                  (airstrike.c and build.c's wood sink build them).
//   INTERCEPT    - enemy aircraft it sees within AG_REACH of one of its buildings: every fighter of its that is ready
//                  (no goal: landed and rearmed, AG_READY_PCT of its hit points) is sent at the nearest of them by
//                  the game's own air order (airstrike.c as_order); one on its way is left alone for AG_AGAIN.
// "# air guard:" at every order and once a game minute.
#define AG_EVERY 5000
#define AG_SEEN 600000
#define AG_BUILD_EVERY 60000
#define AG_TRAIN_EVERY 30000
#define AG_BATCH 2
#define AG_SPARE 2
#define AG_PER_AIRPORT 4
#define AG_REACH 18
#define AG_READY_PCT 60
#define AG_AGAIN 20000
#define AG_FIGHTERS 64
#define AG_FOES 64
internal struct ag_st {
    uint32_t t, build_t, train_t, log_t;
    struct { int id; uint32_t t; } sent[AG_FIGHTERS]; int nsent;   // fighters ordered, when
    unsigned laid, trained, sorties, orders;
} g_ag[16];
internal short g_ag_types[32]; internal int g_ag_nt = -1;   // the fighter types in the data (once a game)
internal void ag_types(void) {
    if (g_ag_nt >= 0) return;
    g_ag_nt = 0;
    for (int t = 0; t < G_TYPES_N && g_ag_nt < 32; t++) {
        ee_unit_type *td = type_data(t);
        const char *nm = td && unit_type_get_id(td) == t ? unit_ident(t) : NULL;
        if (nm && unit_type_get_class_id(td) == CLS_AIRPLANE && unit_type_get_attack(td) > 0 && !strncmp(nm, "a_fighter", 9)) g_ag_types[g_ag_nt++] = (short)t;
    }
}
internal int ag_fighter(int t) { for (int k = 0; k < g_ag_nt; k++) if (g_ag_types[k] == t) return 1; return 0; }
// THE AIR BASES - its aircraft move to the Airport nearest the fighting (the player 2026-11-15: "next the airplane base
// relocation"; 2026-10, on the wing: "airfields forward to reach deeper into enemy territory, the home airfields
// become empty backup ... the ai's airplanes are all stuck at home since the ai never extends their reach").
// How a player does it: aircraft in the air selected, a right-click on another Airport - the game's EEMReturnToBase
// (game/api/messages.c rebase_msg_make): each of the selection in the air gets that Airport as its home and turns
// for it. One standing in its Airport is not touched: it has to fly first.
// Every AB_EVERY, with a place where its force meets the enemy next (stations.c sn_need_at): the forward Airport is
// its finished one nearest that place, in no territory being lost and with no enemy soldiers near. An aircraft based
// (last seen inside) at an Airport AB_GAIN tiles further from the place than the forward one moves, AB_CAP based at
// the forward one at most:
//   in the air (a sortie of the wing's or the guard's)  - rebased as it flies, its orders kept: it lands forward;
//   on the ground, no order                            - sent to fly to the forward Airport (AB_LAUNCH a look), and
//                                                        rebased at the next look, when it is in the air.
// "# air bases:" at every move and once a game minute (tests).
#define AB_EVERY 10000
#define AB_GAIN 15
#define AB_CAP 12
#define AB_LAUNCH 2
#define AB_PLANES 256
#define AB_PORTS 64
internal struct ab_st {
    uint32_t t, log_t;
    struct { int id, home; uint32_t launch_t; } p[AB_PLANES]; int n;   // its aircraft: the Airport each was last seen in (its serial, 0 none), sent to fly when
    unsigned rebased, launched;
} g_ab[16];
internal void airbase_tick(ee_player *pl, unsigned idx, uint32_t now) {
    static ee_unit *ap[AB_PORTS], *air[AB_PLANES];
    static int based[AB_PORTS];
    static __attribute__((aligned(8))) uint8_t mem[0x40];
    struct ab_st *a = &g_ab[idx];
    if (now < a->t) memset(a, 0, sizeof *a);
    if (a->t && now - a->t < AB_EVERY) return;
    a->t = now ? now : 1;
    ivec2 at;
    if (!sn_need_at(idx, &at)) return;
    int nap = 0, nair = 0, count = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0) continue;
        int t = unit_type_get_id(td);
        ee_unit *st = unit_get_physics(u);
        if (t == UT(B_AIRPORT)) { if (unit_get_x(u) >= 0 && st && unit_get_finished(st) && nap < AB_PORTS) ap[nap++] = u; }
        else if (unit_type_get_class_id(td) == CLS_AIRPLANE && unit_type_get_refuel_at(td) == 1 && nair < AB_PLANES) air[nair++] = u;
    }
    if (nap < 2 || !nair) return;
    int fwd = -1, fd = 0;
    for (int k = 0; k < nap; k++) {
        ivec2 p = unit_get_tile(ap[k]);
        int d = ivec2_dist2(p, at);
        if (tv_unsafe(idx, p.x, p.y) || raiders_near(pl, p, 10)) continue;
        if (fwd < 0 || d < fd) { fwd = k; fd = d; }
    }
    if (fwd < 0) return;
    // its aircraft: where each is based
    static struct { int id, home; uint32_t launch_t; } np[AB_PLANES];
    memset(based, 0, sizeof based);
    for (int i = 0; i < nair; i++) {
        ee_unit *u = air[i];
        int id = unit_get_serial(u), k = 0, home = 0;
        uint32_t lt = 0;
        while (k < a->n && a->p[k].id != id) k++;
        if (k < a->n) { home = a->p[k].home; lt = a->p[k].launch_t; }
        // its home: the unit's own field (+0x1D4, what EEMReturnToBase sets), else the Airport it is inside or stands on
        // with no flight time left (the first run read the holder alone: 0 of 12 aircraft based anywhere)
        void *h = unit_get_air_traffic(u), *h2 = unit_get_x(u) < 0 ? unit_get_holder(u) : NULL;
        int seen = 0;
        for (int j = 0; j < nap && !seen; j++)
            if ((h && (h == (void *)ap[j] || h == (void *)unit_get_physics(ap[j]))) || (h2 && (h2 == (void *)ap[j] || h2 == (void *)unit_get_physics(ap[j])))) { home = unit_get_serial(ap[j]); seen = 1; }
        for (int j = 0; j < nap && !seen; j++)
            if (unit_get_x(u) >= 0 && unit_get_flight_ms(u) <= 0 && ivec2_dist2(unit_get_tile(u), unit_get_tile(ap[j])) <= 8 * 8) { home = unit_get_serial(ap[j]); seen = 1; }
        np[i].id = id; np[i].home = home; np[i].launch_t = lt;
        for (int j = 0; j < nap; j++) if (home && home == unit_get_serial(ap[j])) based[j]++;
    }
    int fser = unit_get_serial(ap[fwd]), launched = 0, moved = 0;
    float fr = sqrtf((float)fd) + (float)AB_GAIN;
    ivec2 fp = unit_get_tile(ap[fwd]);
    for (int i = 0; i < nair && based[fwd] < AB_CAP; i++) {
        ee_unit *u = air[i];
        if (np[i].home == fser) continue;
        int hk = -1;
        for (int j = 0; j < nap; j++) if (np[i].home && np[i].home == unit_get_serial(ap[j])) hk = j;
        if (hk >= 0 && sqrtf((float)ivec2_dist2(unit_get_tile(ap[hk]), at)) < fr) continue;   // its Airport is about as near
        int ours = np[i].launch_t && now >= np[i].launch_t && now - np[i].launch_t < 120000;
        if (unit_get_x(u) >= 0 && unit_get_flight_ms(u) > 0) {   // in the air: rebased
            if (hk < 0 && !ours) continue;   // never seen in an Airport of its: a new one on its way out, a carrier's
            uint32_t id1 = (uint32_t)unit_get_serial(u), *sel[3] = { NULL, &id1, &id1 + 1 };
            msg_as_select(pl, sel);
            memset(mem, 0, sizeof mem);
            rebase_msg_make((ee_rebase_msg *)mem, msg_as_begin(pl), (unsigned)fser);
            rebase_msg_set_keep((ee_rebase_msg *)mem, (uint8_t)(ours ? 0 : 1));   // a sortie flies on and lands forward; one sent up for the move turns for it at once
            msg_execute((ee_msg *)mem);
            msg_as_end(pl);
            if (hk >= 0) based[hk]--;
            np[i].home = fser; np[i].launch_t = 0; based[fwd]++;
            a->rebased++; moved++;
        } else if (unit_get_flight_ms(u) <= 0 && hk >= 0 && !ours && !unit_get_goal(u) && launched < AB_LAUNCH && as_order(u, goal_move((float)fp.x + 0.5f, (float)fp.y + 0.5f))) {
            np[i].launch_t = now ? now : 1;
            a->launched++; launched++;
        }
    }
    memcpy(a->p, np, (size_t)nair * sizeof *np); a->n = nair;
    if (g_log && (moved || launched || (g_autotest && (!a->log_t || now - a->log_t >= 60000)))) {
        a->log_t = now ? now : 1;
        log_locked("# air bases: P%u at %.1f min: the forward Airport at %d,%d (%d tiles from where the force meets the enemy, %d,%d), %d of its %d aircraft based there, %d Airports | now %d rebased in the air, %d sent up to move | so far %u rebased, %u sent up\n",
                   idx, now / 60000.0, fp.x, fp.y, (int)sqrtf((float)fd), at.x, at.y, based[fwd], nair, nap, moved, launched, a->rebased, a->launched);
    }
}
internal void airguard_tick(ee_player *pl, uint32_t now) {
    static ee_unit *mine[AG_FIGHTERS], *foe[AG_FOES];
    static ivec2 bld[512];
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    struct ag_st *g = &g_ag[idx];
    if (now < g->t) memset(g, 0, sizeof *g);   // a new game
    if (!pl || is_vanilla(pl) || GAME_AI(idx) || (g->t && now - g->t < AG_EVERY)) return;
    g->t = now ? now : 1;
    int epoch = current_epoch(pl) + 1;
    if (epoch < 10) return;
    ag_types();
    airbase_tick(pl, idx, now);   // THE AIR BASES
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    ee_unit_mgr *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!um || !bm) return;
    // what it has: its fighters, its anti-air buildings, its Airports, where its buildings stand
    int nm = 0, have_b = 0, airports = 0, nb = 0, count = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0) continue;
        int t = unit_type_get_id(td), fam = unit_type_get_class_id(td);
        if (ag_fighter(t)) { if (nm < AG_FIGHTERS) mine[nm++] = u; continue; }
        if (fam != CLS_BUILDING && fam != CLS_FORTRESS) continue;
        for (size_t k = 0; k < sizeof k_aa_bldgs / sizeof *k_aa_bldgs; k++) have_b += t == g_ut[k_aa_bldgs[k].type];
        ee_unit *st = unit_get_physics(u);
        if (t == UT(B_AIRPORT) && st && unit_get_finished(st)) airports++;
        if (!is_wall(t) && unit_get_x(u) >= 0 && nb < 512) bld[nb++] = unit_get_tile(u);
    }
    int seen = g_aa_st[idx].seen_t && now >= g_aa_st[idx].seen_t && now - g_aa_st[idx].seen_t < AG_SEEN, air = seen ? g_aa_st[idx].air : 0;
    const int *stock = player_stock_at(pl);
    // AA BUILDINGS
    int want = 2 + (air + 1) / 2, want_b = air ? 1 + (want > 16 ? 16 : want) / 4 : 0;
    if (want_b > 4) want_b = 4;
    ivec2 d;
    if (have_b < want_b && (!g->build_t || now - g->build_t >= AG_BUILD_EVERY) && nearest_drop(pl, iv2(g_aa_st[idx].ax, g_aa_st[idx].ay), 0, &d)) {
        int bt = 0, pays = 1;
        for (size_t k = 0; k < sizeof k_aa_bldgs / sizeof *k_aa_bldgs && !bt; k++)
            if (k_aa_bldgs[k].epoch <= epoch && ut_has(g_ut[k_aa_bldgs[k].type]) && can_build(pl, g_ut[k_aa_bldgs[k].type])) bt = g_ut[k_aa_bldgs[k].type];
        for (int r = 2; bt && r < 7; r++) if (stock[r] - g_bd_com[idx][r] < unit_cost(pl, bt, r)) pays = 0;
        g->build_t = now ? now : 1;
        for (int r = 3; bt && pays && r <= 7; r++)
            for (int ax = -r; ax <= r && bt; ax++)
                for (int ay = -r; ay <= r && bt; ay++) {
                    ivec2 q = iv2(d.x + ax, d.y + ay);
                    if ((abs(ax) != r && abs(ay) != r) || !can_place(pl, bt, q) || raiders_near(pl, q, 8) || !wk_workable(idx, q)) continue;
                    if (hook_place(bm, bt, q.x, q.y)) {
                        g->laid++;
                        if (g_log) log_locked("# air guard: P%u at %.1f min: an anti-air building %d laid at %d,%d (enemy aircraft %d near %d,%d; it has %d of %d)\n",
                                              idx, now / 60000.0, bt, q.x, q.y, air, g_aa_st[idx].ax, g_aa_st[idx].ay, have_b, want_b);
                    }
                    bt = 0;
                }
    }
    // FIGHTERS
    int want_f = air ? air + AG_SPARE : 0;
    if (want_f > airports * AG_PER_AIRPORT) want_f = airports * AG_PER_AIRPORT;
    if (nm < want_f && (!g->train_t || now - g->train_t >= AG_TRAIN_EVERY)) {
        g->train_t = now ? now : 1;
        int best = 0;
        for (int k = 0; k < g_ag_nt; k++) {
            int t = g_ag_types[k];
            if (ut_has(t) && can_build(pl, t) && (!best || soldier_score(t) > soldier_score(best))) best = t;
        }
        if (best) {
            int req[4] = { best, want_f - nm < AG_BATCH ? want_f - nm : AG_BATCH, 0, 0 };
            unsigned got = 0;
            g_priority_mil = 1;
            hook_train_units(um, req, (uintptr_t)&got);
            g_priority_mil = 0;
            g->trained += got;
            if (got && g_log) log_locked("# air guard: P%u at %.1f min: %u x fighter %d trained (it has %d of %d wanted against %d enemy aircraft, %d Airports)\n",
                                         idx, now / 60000.0, got, best, nm, want_f, air, airports);
        }
    }
    // INTERCEPT
    int nf = 0;
    for (struct grid_it it = grid_it_all(1); nm && grid_it_next(&it) && nf < AG_FOES; ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        int fam = unit_type_get_class_id(it.td);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || unit_get_hp(u) <= 0 || (fam != CLS_AIRPLANE && fam != CLS_HELICOPTER && fam != CLS_HELICOPTER_TRANSPORT) || !seen_by(u, pl)) continue;
        ivec2 p = unit_get_tile(u);
        int near_b = 0;
        for (int k = 0; k < nb && !near_b; k++) near_b = ivec2_dist2(bld[k], p) <= AG_REACH * AG_REACH;
        if (near_b) foe[nf++] = u;
    }
    int sent = 0;
    for (int i = 0; i < nm && nf; i++) {
        ee_unit *u = mine[i];
        int id = unit_get_serial(u), k = 0;
        while (k < g->nsent && g->sent[k].id != id) k++;
        if (k < g->nsent && now >= g->sent[k].t && now - g->sent[k].t < AG_AGAIN) continue;
        ee_unit_type *td = unit_get_type_data(u);
        if (unit_get_goal(u) || unit_get_hp(u) * 100 < unit_type_get_hp(td) * AG_READY_PCT) continue;
        ee_unit *tg = NULL;
        int bd = 0;
        ivec2 p = unit_get_tile(u);
        for (int j = 0; j < nf; j++) { int dd = ivec2_dist2(unit_get_tile(foe[j]), p); if (!tg || dd < bd) { tg = foe[j]; bd = dd; } }
        if (!tg || !as_order(u, goal_attack(u, tg))) continue;
        if (k == g->nsent) { if (g->nsent < AG_FIGHTERS) g->nsent++; else k = (int)(now % AG_FIGHTERS); }
        g->sent[k].id = id; g->sent[k].t = now ? now : 1;
        g->orders++; sent++;
    }
    if (sent) {
        g->sorties++;
        if (g_log) log_locked("# air guard: P%u at %.1f min: %d fighters sent at %d enemy aircraft over its ground\n", idx, now / 60000.0, sent, nf);
    }
    if (g_log && g_autotest && (!g->log_t || now - g->log_t >= 60000)) {
        g->log_t = now ? now : 1;
        log_locked("# air guard: P%u at %.1f min: enemy aircraft near its towns %d, fighters %d of %d wanted (%d Airports), anti-air buildings %d of %d | so far %u buildings laid, %u fighters trained, %u sorties (%u orders)\n",
                   idx, now / 60000.0, air, nm, want_f, airports, have_b, want_b, g->laid, g->trained, g->sorties, g->orders);
    }
}
internal void airguard_new_game(void) { memset(g_ag, 0, sizeof g_ag); memset(g_ab, 0, sizeof g_ab); g_ag_nt = -1; }
