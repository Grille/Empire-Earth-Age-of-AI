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
// THE AIR FORCE'S SIZE ([ai_base] air_force_share; [autotest] v_air_force_share, percent). The player 2026-11-22: "th ai
// order way to few aiplanes, espacialy bomber/fighters it should use more to scout and harass units. the ai barely gets
// more than 15 airplanes in an 1500 pop game" - the fighters above came only against enemy aircraft seen (4 an Airport),
// the bombers only for the wing a position needed (8, 24 at most). With the share on, every AG_EVERY: of its military
// (land soldiers, fighters, bombers) that share is aircraft - fighters as many as the enemy aircraft it knows of
// (seen near its towns, airstrike.c THE PICTURE's fighters known or assumed) and AG_SPARE more, else AFS_FIGHTERS of
// it; bombers the rest (airstrike.c af_train trains them: g_afs_bombers). An Airport for every one its type holds
// (the data's 15), asked of build.c; neither kind beyond the room its Airports have. The fighters come AFS_BATCH an
// Airport an AG_TRAIN_EVERY, as the army's priority orders (the stock above the reserve) - while below the share only,
// so the share is what they take of the army's growth, never more. Its fighters are the air guard's alone: held (the
// game's way), refused to the game's air subgroups (airstrike.c af_takes) and to counter-fire - unheld, the game's
// subgroups ordered them into the enemy's anti-air and every one of an AI's was lost within the hour.
// MEASURED 2026-11-22 (head-to-head mirrored, air_rich: Continental 101 map_size 1, 4 AIs from World War I, 60 min, 10
// games; B v_air_force_share=25): B - A alive -0.05 (se 0.16), soldiers -17.7 (13.6), kills -26.8 (38.8), losses +10.1
// (13.0), razed -7.6 (9.8) - no gain, all on the wrong side: the aircraft come out of the army's room and money (the
// test armies are 20-70 soldiers: 5-20 aircraft) and are still lost about as fast as trained - intercepts that chase
// into the enemy's fighters and guns over contested ground (THE LEASH calls back only past AG_REACH + AG_LEASH of its
// buildings). OFF. One of the 10 B games crashed in the game's sight removal (0x40E585 from 0x555F1B, not seen before).
// THE SWEEPS ([ai_base] fighter_sweeps, with the share; [autotest] v_fighter_sweeps): the fighters ready beyond the
// guard (the enemy aircraft near its towns and AG_SPARE stay at their Airports for INTERCEPT) go every AG_EVERY in
// flights of up to SW_FLIGHT from one Airport:
//   HUNT   - at enemy aircraft in the air it sees (INTERCEPT has sent fighters at those over its own ground first) that
//            a fighter can hurt (the game's numbers, stats.c; no land unit: a fighter hurts none), by SW_CELL cell: the most worth for the way, within its reach (SW_REACH of its flight time there and back),
//            where the anti-air of THE PICTURE (airstrike.c dct_scan: guns seen, remembered, assumed by towns not
//            looked at lately, the enemy's fighters' cover) can't bring the weakest of the flight down there, over it
//            for SW_OVER_S and home - the bombers' rule (the player 2026-11-21: "undefended ... just not enough to
//            bring the bomber down on its own"), its damage reckoned at the bombers' and scaled by the speeds. One
//            flight a cell. Once there the fighters pick their next targets themselves, as the player's do.
//   LOOK   - none of those for them: one fighter flies over the enemy's or dark ground (explore.c) of a land territory
//            not looked at in SW_LOOK_MS, the longest unseen for the way, by the same rule - what it sees there is what
//            the bombers' picture and the territories know next.
// A fighter on a sweep is claimed (claims.c CLW_FIGHTERS, CLP_TASK); back in its Airport, idle, or out SW_RUN_MS (then
// called home) it is free again. "# air force size:" and "# sweeps:" once a game minute in tests.
#define AFS_FIGHTERS 0.4f
#define AFS_BATCH 2
#define AFS_MAX 6
#define AFS_PORT_HOLDS 15     // an Airport's room when the data has none
#define SW_FLIGHT 4
#define SW_RUN_MS 150000
#define SW_CELL 8
#define SW_CELLS (600 / SW_CELL + 1)
#define SW_CANDS 1024
#define SW_TRIES 48
#define SW_OVER_S 12.0f
#define SW_REACH 0.4f
#define SW_LOOK_MS 300000
#define SW_N 256
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
#define AG_LEASH 6
#define AG_FIGHTERS 256
#define AG_FOES 64
internal struct ag_st {
    uint32_t t, build_t, train_t, log_t;
    struct { int id; uint32_t t; } sent[AG_FIGHTERS]; int nsent;   // fighters ordered, when
    unsigned laid, trained, sorties, orders, leashed;
    // THE AIR FORCE'S SIZE: the last look's - aircraft wanted, of them fighters, bombers; its land soldiers, fighters,
    // bombers, Airports; fighters trained for the share
    int sz_want, sz_f, sz_b, sz_land, sz_nf, sz_nb, sz_ports, sz_ap; unsigned sz_trained;
    // THE SWEEPS: the fighters out (serial, its target's serial and type, the Airport it came from, sent when, where to,
    // 1 harass 2 look); runs and fighters sent, looks flown, fighters lost on them, their targets down and worth, looks
    // with fighters free and nothing to go at, fighters called home
    struct { int id, tgt, home; uint32_t t; ivec2 at; uint8_t kind; short ttype; } sw[SW_N]; int nsw;
    unsigned sw_runs, sw_sent, sw_looks, sw_lost, sw_down, sw_worth, sw_none, sw_back;
    unsigned sw_cells, sw_seen, sw_hurt, sw_far, sw_risky, sw_lfar, sw_lrisky, sw_lnone;
    // its fighters at the last look, for the loss lines (tests): where, hit points, flight time left, goal
    struct { int id, hp, fl; ivec2 at; const char *goal; } fl[AG_FIGHTERS]; int nfl; unsigned fl_logged;   // why none went: cells of targets seen (at the last look), tried out of reach, too much anti-air; looks the same, no ground to look at
} g_ag[16];
internal int g_afs_pct = 0, g_v_afs_pct = -1, g_sweeps = 1, g_v_sweeps = -1;   // [ai_base] air_force_share (percent), fighter_sweeps
#define AFS_SHARE(idx) PV(idx, g_v_afs_pct, g_afs_pct)
#define SWEEPS(idx) (AFS_SHARE(idx) > 0 && PV(idx, g_v_sweeps, g_sweeps))
internal int afs_owns_fighters(unsigned idx) { return AFS_SHARE(idx & 15) > 0; }   // counterfire.c: none of its fighters are soldiers to send
internal void airguard_install(const char *autotest_ini) {
    g_afs_pct = setting_pct(g_set_ai[1].air_force_share);
    g_sweeps = g_set_ai[1].fighter_sweeps;
    if (autotest_ini) {
        g_v_afs_pct = ini_int("autotest", "v_air_force_share", -1, autotest_ini);
        g_v_sweeps = ini_int("autotest", "v_fighter_sweeps", -1, autotest_ini);
    }
    if (g_log) fprintf(g_log, "# air force size: share %d%% (variant %d), sweeps %d (variant %d)\n", g_afs_pct, g_v_afs_pct, g_sweeps, g_v_sweeps);
}
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
// THE SWEEPS: the fighters out looked at - lost, back, done; their targets down
internal int sw_at(struct ag_st *g, int id) { for (int k = 0; k < g->nsw; k++) if (g->sw[k].id == id) return k; return -1; }
internal void sw_watch(ee_player *pl, struct ag_st *g, uint32_t now) {
    int m = 0;
    for (int k = 0; k < g->nsw; k++) {
        __typeof__(g->sw[0]) *w = &g->sw[k];
        ee_unit *u = unit_by_id((unsigned)w->id);
        ee_unit *t = w->tgt ? unit_by_id((unsigned)w->tgt) : NULL;
        int alive = u && unit_get_serial(u) == w->id && unit_get_hp(u) > 0 && unit_get_owner(u) == pl;
        if (w->tgt && (!t || unit_get_serial(t) != w->tgt || unit_get_hp(t) <= 0)) {   // its target down: counted once
            int v = 0;
            for (int r = 2; r < 7; r++) v += unit_cost(pl, w->ttype, r);
            g->sw_down++; g->sw_worth += (unsigned)(v > 0 ? v : 0);
            for (int j = 0; j < g->nsw; j++) if (g->sw[j].tgt == w->tgt) g->sw[j].tgt = 0;
        }
        if (!alive) { g->sw_lost++; continue; }
        uint32_t out = now - w->t;
        int done = out >= 10000 && (unit_get_x(u) < 0 || !unit_get_goal(u));   // back in its Airport, or idle where it went
        if (!done && out >= SW_RUN_MS) {   // out too long: home
            if (unit_get_x(u) >= 0 && w->home) as_rebase(pl, w->id, (unsigned)w->home);
            g->sw_back++; done = 1;
        }
        if (done) { cl_end(u, CLW_FIGHTERS); continue; }
        cl_take(u, CLP_TASK, CLW_FIGHTERS, 3 * AG_EVERY);
        g->sw[m++] = *w;
    }
    g->nsw = m;
}
// THE SWEEPS: the ready fighters beyond the guard (rdy[0..nfree)) - flights at what they can hurt and come back from, else a look
internal void sw_launch(ee_player *pl, unsigned idx, struct ag_st *g, ee_unit **rdy, int nfree, uint32_t now) {
    static float cw[SW_CELLS][SW_CELLS], cb[SW_CELLS][SW_CELLS];
    static ee_unit *cu[SW_CELLS][SW_CELLS];
    static struct { ee_unit *u; ivec2 at; float v; short type; uint8_t owner; } tg[SW_CANDS];
    static uint8_t taken[SW_CANDS], used[AG_FIGHTERS];
    struct dct_st *d = &g_dct[idx];
    if (nfree <= 0 || d->ref < 0 || !d->nport || d->speed <= 0) return;
    // the fighters' type: the first's (one type a flight's reckoning: they are all of its epoch's)
    ee_unit_type *ftd = unit_get_type_data(rdy[0]);
    int ft = ftd ? unit_type_get_id(ftd) : -1;
    ee_unit_stats *fst = st_rec(pl, ft);
    float fsp = fst ? unit_stats_get_speed(fst) : 0.0f, fhp = ftd ? (float)st_maxhp(ftd, fst) : 0.0f;
    int fms = fst ? unit_stats_get_flight_ms(fst) : 0;
    if (!ftd || fsp < 0.2f || fhp <= 0) return;
    float reach = fms > 0 ? (float)fms / 1000.0f * fsp * SW_REACH : 60.0f, scale = d->speed / fsp;
    // what it can hurt: enemy aircraft in the air it sees, by cell - a fighter hurts no land unit (the game's numbers: of
    // 447 enemy land units an AI's fighters saw in a pilot, none; land units were the first targets here)
    memset(cw, 0, sizeof cw); memset(cb, 0, sizeof cb);
    for (int o = 1; o < nplayers(); o++) {
        ee_player *op = player_at(o);
        if (!op || !is_enemy(pl, op)) continue;
        int count = 0, oi = player_get_index(op) & 15;
        for (ee_unit *u = player_get_units(op); u && count < 16384; u = unit_get_next(u), count++) {
            ee_unit_type *td = unit_get_type_data(u);
            int fam = td ? unit_type_get_class_id(td) : -1;
            if ((fam != CLS_AIRPLANE && fam != CLS_HELICOPTER && fam != CLS_HELICOPTER_TRANSPORT) || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !seen_by(u, pl)) continue;
            g->sw_seen++;
            if (st_shot_t(ftd, fst, (int)idx, td, unit_get_stats(u), oi) <= 0) continue;
            g->sw_hurt++;
            ivec2 p = unit_get_tile(u);
            int x = p.x / SW_CELL, y = p.y / SW_CELL, t = unit_type_get_id(td);
            if (x < 0 || y < 0 || x >= SW_CELLS || y >= SW_CELLS) continue;
            int c = 0;
            for (int r = 2; r < 7; r++) c += unit_cost(pl, t, r);
            float v = (float)(c < 600 ? c : 600);
            cw[x][y] += v;
            if (v > cb[x][y]) { cb[x][y] = v; cu[x][y] = u; }
        }
    }
    int nt = 0;
    for (int x = 0; x < SW_CELLS; x++)
        for (int y = 0; y < SW_CELLS && nt < SW_CANDS; y++)
            if (cw[x][y] > 0) {
                ee_player *o = unit_get_owner(cu[x][y]);
                tg[nt].u = cu[x][y]; tg[nt].at = unit_get_tile(cu[x][y]); tg[nt].v = cw[x][y];
                tg[nt].type = (short)unit_type_get_id(unit_get_type_data(cu[x][y])); tg[nt].owner = (uint8_t)(o ? player_get_index(o) & 15 : 0); nt++;
            }
    g->sw_cells = (unsigned)nt;
    memset(taken, 0, (size_t)nt);
    for (int k = 0; k < nt; k++)   // one flight a cell: not one a sweep is on already
        for (int j = 0; j < g->nsw && !taken[k]; j++)
            taken[k] = g->sw[j].kind == 1 && g->sw[j].at.x / SW_CELL == tg[k].at.x / SW_CELL && g->sw[j].at.y / SW_CELL == tg[k].at.y / SW_CELL;
    memset(used, 0, (size_t)nfree);
    int runs = 0, looked = 0, none = 0;
    for (int i = 0; i < nfree; i++) {
        if (used[i]) continue;
        int pk = dct_port(d, rdy[i]);
        if (pk < 0) { used[i] = 1; continue; }
        ivec2 from = d->port[pk];
        static int gi[AG_FIGHTERS];
        int ng = 0;   // the flight: those rdy at the same Airport, the soundest first
        for (int j = i; j < nfree && ng < SW_FLIGHT * 4; j++) {
            if (used[j] || (j != i && dct_port(d, rdy[j]) != pk)) continue;
            int k = ng++;
            while (k > 0 && unit_get_hp(rdy[gi[k - 1]]) < unit_get_hp(rdy[j])) { gi[k] = gi[k - 1]; k--; }
            gi[k] = j;
        }
        int n = ng < SW_FLIGHT ? ng : SW_FLIGHT;
        float weakest = (float)unit_get_hp(rdy[gi[n - 1]]);
        // HARASS: the most worth for the way, tried best first
        int best = -1;
        for (int tries = 0; tries < SW_TRIES; tries++) {
            int c = -1; float cv = 0;
            for (int k = 0; k < nt; k++) {
                if (taken[k]) continue;
                float dist = sqrtf((float)ivec2_dist2(from, tg[k].at)), v = tg[k].v / (30.0f + dist);
                if (dist > reach) { if (!tries) g->sw_far++; taken[k] = 2; continue; }
                if (c < 0 || v > cv) { c = k; cv = v; }
            }
            if (c < 0) break;
            float dmg = dct_sortie(d, from, tg[c].at, from, SW_OVER_S) * scale;
            int o = tg[c].owner;
            if (d->has_aa[o] && !ah_looked(pl, tg[c].at)) dmg += d->aadps[o] * (2.0f * d->aar[o] / fsp + SW_OVER_S);
            if (dmg * AD_MARGIN + AD_RESERVE * fhp < weakest) { best = c; break; }
            g->sw_risky++;
            taken[c] = 3;   // not for this flight (nor the next's: the same picture)
        }
        for (int k = 0; k < nt; k++) if (taken[k] == 2) taken[k] = 0;   // out of this Airport's reach only
        if (best >= 0) {
            taken[best] = 1;
            int sent = 0;
            for (int m = 0; m < n; m++) {
                ee_unit *u = rdy[gi[m]];
                used[gi[m]] = 1;
                if (g->nsw >= SW_N || !cl_take(u, CLP_TASK, CLW_FIGHTERS, SW_RUN_MS)) continue;
                if (!as_order(u, goal_attack(u, tg[best].u))) { cl_end(u, CLW_FIGHTERS); continue; }
                __typeof__(g->sw[0]) *w = &g->sw[g->nsw++];
                w->id = unit_get_serial(u); w->tgt = unit_get_serial(tg[best].u); w->ttype = tg[best].type; w->home = unit_get_serial((ee_unit *)d->port_u[pk][0]);
                w->t = now ? now : 1; w->at = tg[best].at; w->kind = 1;
                sent++;
            }
            if (!sent) continue;
            runs++; g->sw_runs++; g->sw_sent += (unsigned)sent;
            if (g_log) log_locked("# sweeps: P%u at %.1f min: %d fighters at type %d at %d,%d (worth %.0f there, from %d,%d, %d tiles)\n", idx, now / 60000.0, sent, tg[best].type,
                                  tg[best].at.x, tg[best].at.y, tg[best].v, from.x, from.y, (int)sqrtf((float)ivec2_dist2(from, tg[best].at)));
            continue;
        }
        // LOOK: one a look - the land territory of the enemy's, or dark, unseen longest for the way
        for (int m = 0; m < ng; m++) used[gi[m]] = 1;
        none = 1;
        if (looked) continue;
        int br = -1, any = 0; float bv = 0;
        for (int r = 0; r < g_ter.n; r++) {
            if (g_ter.t[r].water) continue;
            int ow = g_ter.t[r].owner;
            ee_player *op = ow > 0 ? player_at(ow) : NULL;
            if (!(op && is_enemy(pl, op)) && !xp_ter_dark(idx, r)) continue;
            ivec2 c = g_ter.t[r].c;
            uint32_t ago = intel_looked_ago(pl, c.x, c.y);
            float dist = sqrtf((float)ivec2_dist2(from, c));
            if (ago < SW_LOOK_MS) continue;
            any = 1;
            if (dist > reach) { g->sw_lfar++; continue; }
            float v = (float)(ago < 1800000 ? ago : 1800000) / (30.0f + dist);
            if (br >= 0 && v <= bv) continue;
            float dmg = dct_sortie(d, from, c, from, SW_OVER_S) * scale;
            if (op && d->has_aa[ow & 15] && !ah_looked(pl, c)) dmg += d->aadps[ow & 15] * (2.0f * d->aar[ow & 15] / fsp + SW_OVER_S);
            if (dmg * AD_MARGIN + AD_RESERVE * fhp >= (float)unit_get_hp(rdy[gi[0]])) { g->sw_lrisky++; continue; }
            br = r; bv = v;
        }
        if (!any) g->sw_lnone++;
        if (br < 0) continue;
        ee_unit *u = rdy[gi[0]];
        ivec2 c = g_ter.t[br].c;
        if (g->nsw >= SW_N || !cl_take(u, CLP_TASK, CLW_FIGHTERS, SW_RUN_MS)) continue;
        if (!as_order(u, goal_move((float)c.x + 0.5f, (float)c.y + 0.5f))) { cl_end(u, CLW_FIGHTERS); continue; }
        __typeof__(g->sw[0]) *w = &g->sw[g->nsw++];
        w->id = unit_get_serial(u); w->tgt = 0; w->ttype = 0; w->home = unit_get_serial((ee_unit *)d->port_u[pk][0]);
        w->t = now ? now : 1; w->at = c; w->kind = 2;
        looked = 1; g->sw_looks++;
        if (g_log) log_locked("# sweeps: P%u at %.1f min: a fighter looks at territory %d at %d,%d (%s, not looked at in %.1f min; from %d,%d, %d tiles)\n", idx, now / 60000.0, br, c.x, c.y,
                              g_ter.t[br].owner > 0 ? "the enemy's" : "dark", intel_looked_ago(pl, c.x, c.y) / 60000.0, from.x, from.y, (int)sqrtf((float)ivec2_dist2(from, c)));
    }
    if (none && !runs && !looked) g->sw_none++;
}
internal void airguard_tick(ee_player *pl, uint32_t now) {
    static ee_unit *mine[AG_FIGHTERS], *foe[AG_FOES];
    static ivec2 bld[512];
    static ee_unit *ports[64]; int nports = 0;
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
    // what it has: its fighters, its anti-air buildings, its Airports, where its buildings stand; for THE AIR FORCE'S
    // SIZE its land soldiers and bombers, its Airports going up too
    int nm = 0, have_b = 0, airports = 0, ap_all = 0, nb = 0, count = 0, land = 0, nbomb = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0) continue;
        int t = unit_type_get_id(td), fam = unit_type_get_class_id(td);
        if (ag_fighter(t)) {
            // THE AIR FORCE'S SIZE: held the game's way, as the bombers (airstrike.c HELD) - unheld, the game's own groups
            // ordered them about (19 of its orders to fighters a game, "# who decides:") and they were lost by the
            // enemy's anti-air buildings on moves and attacks that weren't the mod's, all of them within the hour
            if (AFS_SHARE(idx) > 0 && !unit_get_ai_held(u)) player_hold_unit(pl, u);
            if (nm < AG_FIGHTERS) mine[nm++] = u;
            continue;
        }
        if (fam == CLS_AIRPLANE && unit_type_get_family(td) == KIND_BOMBER) { nbomb++; continue; }
        if (fam == CLS_LAND && land_soldier(t)) { land++; continue; }
        if (fam != CLS_BUILDING && fam != CLS_FORTRESS) continue;
        for (size_t k = 0; k < sizeof k_aa_bldgs / sizeof *k_aa_bldgs; k++) have_b += t == g_ut[k_aa_bldgs[k].type];
        ee_unit *st = unit_get_physics(u);
        if (t == UT(B_AIRPORT) && st && unit_get_finished(st)) { airports++; if (nports < 64 && unit_get_x(u) >= 0) ports[nports++] = u; }
        ap_all += t == UT(B_AIRPORT);
        if (!is_wall(t) && unit_get_x(u) >= 0 && nb < 512) bld[nb++] = unit_get_tile(u);
    }
    // FIGHTERS LOST (tests): one at the last look and not now - where it was, its hit points, flight time, goal, the
    // nearest enemy that can hit an aircraft
    if (g_log && g_autotest) {
        for (int k = 0; k < g->nfl; k++) {
            int i = 0;
            while (i < nm && unit_get_serial(mine[i]) != g->fl[k].id) i++;
            if (i < nm || g->fl_logged >= 60) continue;
            g->fl_logged++;
            int ad = -1, at = -1;
            for (struct grid_it it = grid_it_box(g->fl[k].at.x, g->fl[k].at.y, 14, 1); grid_it_next(&it); ) {
                ee_player *o = unit_get_owner(it.u);
                if (!o || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || !ah_aa(it.td)) continue;
                int dd = (int)ivec2_dist2(unit_get_tile(it.u), g->fl[k].at);
                if (ad < 0 || dd < ad) { ad = dd; at = unit_type_get_id(it.td); }
            }
            log_locked("# fighters lost: P%u at %.1f min: last seen at %d,%d (%d hit points, %d ms flight left, goal %s, on a sweep %d); the nearest enemy anti-air type %d at %d tiles\n",
                       idx, now / 60000.0, g->fl[k].at.x, g->fl[k].at.y, g->fl[k].hp, g->fl[k].fl, g->fl[k].goal ? g->fl[k].goal : "none", sw_at(g, g->fl[k].id) >= 0,
                       at, ad < 0 ? -1 : (int)sqrtf((float)ad));
        }
        g->nfl = 0;
        for (int i = 0; i < nm; i++) {
            ee_unit *u = mine[i];
            ee_goal *gl = unit_get_goal(u);
            g->fl[g->nfl].id = unit_get_serial(u); g->fl[g->nfl].hp = unit_get_hp(u); g->fl[g->nfl].fl = unit_get_flight_ms(u);
            g->fl[g->nfl].at = unit_get_tile(u); g->fl[g->nfl].goal = gl ? rtti_name(gl) : NULL; g->nfl++;
        }
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
    // THE AIR FORCE'S SIZE
    int pct = AFS_SHARE(idx), sz_f = 0, sz_b = 0, sz = 0, holds = AFS_PORT_HOLDS;
    if (pct > 0) {
        ee_unit_type *atd = type_data(UT(B_AIRPORT));
        if (atd && unit_type_get_transport_capacity(atd) > 0) holds = unit_type_get_transport_capacity(atd);
        sz = (int)((float)(land + nm + nbomb) * (float)pct / 100.0f + 0.5f);
        float known = 0;   // the enemy's fighters known or assumed (THE PICTURE), its aircraft near the towns
        for (int o = 0; o < 16; o++) known += g_dct[idx].nf[o];
        int foes = (int)(known + 0.5f) > air ? (int)(known + 0.5f) : air;
        sz_f = foes ? foes + AG_SPARE : 0;
        if (sz_f < (int)((float)sz * AFS_FIGHTERS + 0.5f)) sz_f = (int)((float)sz * AFS_FIGHTERS + 0.5f);
        if (sz_f > sz) sz_f = sz;
        sz_b = sz - sz_f;
        int want_p = (sz + holds - 1) / holds;
        if (want_p > ap_all) { g_bd_airport[idx] = now ? now : 1; if (g_bd_airport_n[idx] < want_p || g_bd_airport_n[idx] > want_p + 1) g_bd_airport_n[idx] = want_p; }
        g_afs_ports[idx] = want_p;
    } else g_afs_ports[idx] = 0;
    g_afs_bombers[idx] = sz_b;
    g_afs_room[idx] = airports * holds - (nm < sz_f ? sz_f : nm);   // the bombers' room: beside the fighters it keeps
    g->sz_want = sz; g->sz_f = sz_f; g->sz_b = sz_b; g->sz_land = land; g->sz_nf = nm; g->sz_nb = nbomb; g->sz_ports = g_afs_ports[idx]; g->sz_ap = ap_all;
    // FIGHTERS
    int want_f = air ? air + AG_SPARE : 0, guard = want_f;
    if (want_f > airports * AG_PER_AIRPORT) want_f = airports * AG_PER_AIRPORT;
    if (sz_f > want_f) {   // THE AIR FORCE'S SIZE: its share, as far as its Airports have room beside the bombers
        int room = airports * holds - nbomb;
        want_f = sz_f < room ? sz_f : room > want_f ? room : want_f;
    }
    if (nm < want_f && (!g->train_t || now - g->train_t >= AG_TRAIN_EVERY)) {
        g->train_t = now ? now : 1;
        int best = 0;
        for (int k = 0; k < g_ag_nt; k++) {
            int t = g_ag_types[k];
            if (ut_has(t) && can_build(pl, t) && (!best || soldier_score(t) > soldier_score(best))) best = t;
        }
        if (best) {
            int batch = sz_f > nm ? airports * AFS_BATCH : AG_BATCH;   // THE AIR FORCE'S SIZE: from all its Airports
            if (batch > AFS_MAX) batch = AFS_MAX;
            int req[4] = { best, want_f - nm < batch ? want_f - nm : batch, 0, 0 };
            unsigned got = 0;
            g_priority_mil = 1;
            hook_train_units(um, req, (uintptr_t)&got);
            g_priority_mil = 0;
            g->trained += got;
            if (sz_f > nm) g->sz_trained += got;
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
    // THE LEASH (with the share): a fighter on an intercept AG_LEASH tiles beyond AG_REACH of its buildings is called back
    // to its nearest Airport - sent one by one, they chased the enemy's aircraft home into its fighters and anti-air
    // buildings (in a pilot 30 of 40 fighters lost so, none on a sweep)
    for (int k = 0; AFS_SHARE(idx) > 0 && nports && k < g->nsent; k++) {
        if (now < g->sent[k].t || now - g->sent[k].t >= 3 * AG_AGAIN) continue;
        ee_unit *u = unit_by_id((unsigned)g->sent[k].id);
        if (!u || unit_get_serial(u) != g->sent[k].id || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || unit_get_flight_ms(u) <= 0) continue;
        ivec2 p = unit_get_tile(u);
        int near_b = 0, r2 = (AG_REACH + AG_LEASH) * (AG_REACH + AG_LEASH);
        for (int j = 0; j < nb && !near_b; j++) near_b = ivec2_dist2(bld[j], p) <= r2;
        if (near_b) continue;
        int bp = 0;
        for (int j = 1; j < nports; j++) if (ivec2_dist2(unit_get_tile(ports[j]), p) < ivec2_dist2(unit_get_tile(ports[bp]), p)) bp = j;
        as_rebase(pl, g->sent[k].id, (unsigned)unit_get_serial(ports[bp]));
        g->sent[k].t = 0; g->leashed++;
    }
    // THE SWEEPS: the ready beyond the guard
    sw_watch(pl, g, now);
    if (SWEEPS(idx)) {
        static ee_unit *rdy[AG_FIGHTERS];
        int nr = 0, keep = guard > AG_SPARE ? guard : AG_SPARE;
        for (int i = 0; i < nm; i++) {
            ee_unit *u = mine[i];
            ee_unit_type *td = unit_get_type_data(u);
            int id = unit_get_serial(u), k = 0;
            if (unit_get_goal(u) || unit_get_hp(u) * 100 < unit_type_get_hp(td) * AG_READY_PCT || sw_at(g, id) >= 0) continue;
            while (k < g->nsent && g->sent[k].id != id) k++;
            if (k < g->nsent && now >= g->sent[k].t && now - g->sent[k].t < AG_AGAIN) continue;
            if (keep > 0) { keep--; continue; }   // the guard stays
            rdy[nr++] = u;
        }
        sw_launch(pl, idx, g, rdy, nr, now);
    }
    if (g_log && g_autotest && (!g->log_t || now - g->log_t >= 60000)) {
        g->log_t = now ? now : 1;
        log_locked("# air guard: P%u at %.1f min: enemy aircraft near its towns %d, fighters %d of %d wanted (%d Airports), anti-air buildings %d of %d | so far %u buildings laid, %u fighters trained, %u sorties (%u orders)\n",
                   idx, now / 60000.0, air, nm, want_f, airports, have_b, want_b, g->laid, g->trained, g->sorties, g->orders);
        log_locked("# air force size: P%u at %.1f min: share %d%%, sweeps %d | land soldiers %d, fighters %d, bombers %d | wanted %d aircraft: %d fighters, %d bombers; Airports %d of %d wanted (%d finished) | so far fighters trained for it %u, bombers trained %u\n",
                   idx, now / 60000.0, pct, SWEEPS(idx), land, nm, nbomb, sz, sz_f, sz_b, ap_all, g_afs_ports[idx], airports, g->sz_trained, g_as[idx].trained);
        log_locked("# sweeps: P%u at %.1f min: out %d | runs %u (%u fighters), looks %u, fighters lost on them %u, called home %u, targets down %u (worth %u), looks with none to go at %u (target cells %u now, enemy aircraft seen so far %u, of them a fighter can hurt %u; tried out of reach %u, too much anti-air %u; ground to look at out of reach %u, too much anti-air %u, none %u) | fighters lost in all %d, intercepts called back %u\n",
                   idx, now / 60000.0, g->nsw, g->sw_runs, g->sw_sent, g->sw_looks, g->sw_lost, g->sw_back, g->sw_down, g->sw_worth, g->sw_none,
                   g->sw_cells, g->sw_seen, g->sw_hurt, g->sw_far, g->sw_risky, g->sw_lfar, g->sw_lrisky, g->sw_lnone, (int)g->trained - nm, g->leashed);
    }
}
internal void airguard_new_game(void) { memset(g_ag, 0, sizeof g_ag); memset(g_ab, 0, sizeof g_ab); g_ag_nt = -1; }
