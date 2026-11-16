// TOWER CREEP ([ai_base] tower_creep, off; [autotest] v_tower_creep) - towers laid at an enemy's buildings, while the
// game's computer player is off
//
// The player, 2026-11-16: "offensive towers and tower creep, espacialy in these prehistoric stealmates they shoudl come
// handy. if an enemy territory seems mostly unguard walk up and place a tower just in range of its buildings, greeping
// in, it can even be used to take down enemy towers, just bring enough citezens to out repair the other towers demage
// while building, than add 2 or 3 more towers to overpower the enemy one."
//
// One creep at a time per AI, every CR_EVERY a look:
//   THE TARGET  - a finished enemy building it knows (seen_by: seen, or remembered), not a wall's piece, within
//                 CR_REACH tiles of a finished building of its own (the creep starts from its ground and goes on from
//                 its towers), unguarded: no enemy soldier seen or remembered within CR_FOE_R of it, none seen in its
//                 territory for CR_QUIET (the first games: most creeps dropped for soldiers who came within a minute -
//                 the target alone was looked at, at the start), no more than CR_CITS of its citizens seen within
//                 CR_FOE_R (a crew of 4 among the owner's working citizens was beaten to death by them, 5 hp a blow
//                 from ten and more: 118 of 120 crew members lost in 8 games died, mostly so). The nearest to its own buildings first; one
//                 where a creep was dropped not again for CR_BAD_WAIT.
//   THE SPOT    - a tile just inside its best tower's range of the target (CR_IN tiles inside it), out of the reach of
//                 every enemy defence it knows or has explored (work.c wk_danger), on the crew's land, the one nearest its own buildings (the side it
//                 comes from). A tower as the target: outside its reach too where its own tower outranges it; else in
//                 its reach ("bring enough citezens to out repair") - never where the enemy's outranges its own.
//   THE CREW    - CR_CREW of its citizens gathering or idle, the nearest; CR_CREW_TOWER for a spot in a tower's reach.
//                 The tower shoots the builders, not the foundation: a citizen of 65 hp went in two shots, one every
//                 6 s or so, while six put up 80 of a Stone Age tower's 1450 hp in 4 s - a crew of 8 was gone with the
//                 tower half up, every time. Only with min_workforce citizens left at work besides them.
//   THE WALK    - the crew walks to the spot first; the first foundation is laid once half of it stands within
//                 CR_THERE tiles of it (laid at the start, a creep dropped on the way had paid for its tower - and the
//                 owner's soldiers came for a foundation in its land before the builders did).
//   THE TOWERS  - laid by the mod's own placement (place_foundation, as towns.c lays a town's), the crew sent to build
//                 it; every look a crew member not building or repairing one of the creep's towers is sent to the
//                 one going up, else to the most damaged one. Against a building CR_TOWERS_B towers at most (the
//                 second when the first stands and the target still does after CR_SECOND), against a tower
//                 CR_TOWERS_T (one after the other, each laid when the one before stands: "2 or 3 more").
//   THE END     - the target gone: the next look picks the next target, nearest its buildings - its new towers among
//                 them, so it creeps on. Dropped (the target noted for CR_BAD_WAIT): enemy soldiers seen within
//                 CR_FOE_R of the spot while none of its towers there stands finished (seen: one remembered there
//                 dropped creeps whose way was clear) or as many of the owner's citizens as it has crew, fewer than
//                 CR_CREW_MIN of the crew left, no tower laid within
//                 CR_LAY_WAIT, CR_RELAY foundations of its destroyed with none standing, CR_GIVEUP since it began;
//                 its towers going up are taken up again then (refunded).
// The crew is not held from the economy: between two towers a crew member may go to work again and is taken back by
// the next look; citizens_safety still sends them home from raiders (an army coming is the end of a creep anyway).
// Log: "# creep:".
// Off by default: 8 games of 4 AIs from the first epoch on Continental small, seeds 3-10, against the same without it -
// at minute 60 citizens 330 against 361, soldiers 55 against 63, citizens killed 67 against 53, buildings 202
// against 205, for 1.5 enemy buildings a game its towers took (10.8 creeps a game, 9.2 of them dropped: 43 of 74 drops
// for the owner's citizens at the spot, 20 for soldiers).
#define CR_EVERY 4000
#define CR_REACH 40
#define CR_FOE_R 12
#define CR_IN 1.5f          // the spot this far inside its tower's range of the target's edge
#define CR_MARGIN 1.0f      // ... and this far outside every other enemy defence's reach
#define CR_CREW 4
#define CR_CREW_TOWER 12
#define CR_QUIET 120000
#define CR_CITS 2
#define CR_RELAY 3
#define CR_CREW_MIN 2
#define CR_TOWERS_B 2
#define CR_TOWERS_T 4
#define CR_SECOND 90000
#define CR_LAY_WAIT 120000   // from the start: the walk, then the foundation
#define CR_THERE 5
#define CR_GIVEUP 900000
#define CR_BAD 16
#define CR_BAD_WAIT 600000
#define CR_AGAIN 20000      // after a creep ends or is dropped, the next look for a target
#define CR_TRIES 12         // targets tried for a spot a look
#define CR_OWN 1024
#define CR_CAND 512
internal struct cr_st {
    uint32_t t, start_t, end_t, laid_t, stood_t, log_t;
    ee_unit *target; int tid, ttype, tower, exposed, lays; ivec2 tp;   // tower: the target is a defence; exposed: its spot in the target's reach; lays: foundations laid
    ivec2 spot; int ntow; ee_unit *tow[CR_TOWERS_T]; int tow_id[CR_TOWERS_T];
    int ncrew; ee_unit *crew[CR_CREW_TOWER]; int crew_id[CR_CREW_TOWER];
    struct { int id; uint32_t t; } bad[CR_BAD]; int nbad;
    unsigned started, laid, down, dropped, sends;
} g_cr[16];
internal int g_tower_creep = 0, g_v_tower_creep = -1;
#define TOWER_CREEP(idx) PV(idx, g_v_tower_creep, g_tower_creep)
internal int bd_send(ee_player *pl, ee_unit *cit, ee_unit *building);   // build.c
internal int bd_pays(ee_player *pl, int type, const int *com);
internal int cr_alive(ee_player *pl, ee_unit *u, int id) { return u && unit_get_serial(u) == id && unit_get_owner(u) == pl && unit_get_hp(u) > 0 && unit_get_x(u) >= 0; }
internal int cr_done(ee_unit *b) { ee_unit *st = unit_get_physics(b); return st && unit_get_finished(st); }
internal int cr_hp_max(ee_unit *u) { ee_unit_stats *st = unit_get_stats(u); return st ? unit_stats_get_hp(st) : unit_type_get_hp(unit_get_type_data(u)); }
internal int cr_is_bad(struct cr_st *c, int id, uint32_t now) {
    for (int k = 0; k < c->nbad && k < CR_BAD; k++) if (c->bad[k].id == id && now >= c->bad[k].t && now - c->bad[k].t < CR_BAD_WAIT) return 1;
    return 0;
}
// the end of a creep: dropped (why) or done (why NULL: its target gone)
internal void cr_end(unsigned idx, struct cr_st *c, uint32_t now, const char *why) {
    if (why) { c->bad[c->nbad++ % CR_BAD].id = c->tid; c->bad[(c->nbad - 1) % CR_BAD].t = now ? now : 1; c->dropped++; }
    else c->down++;
    if (g_log) log_locked("# creep: P%u at %.1f min: the creep on %d at %d,%d %s after %.1f min - %d towers laid there (%u creeps, %u targets down, %u dropped so far)\n",
                          idx, now / 60000.0, c->ttype, c->tp.x, c->tp.y, why ? "dropped:" : "done: its target is gone", (now - c->start_t) / 60000.0, c->ntow,
                          c->started, c->down, c->dropped);
    if (why && g_log) log_locked("# creep: P%u   why: %s\n", idx, why);
    // dropped: its towers going up removed, refunded (vf24 (0, 0, 0), as towns.c removes a town's) - a foundation left
    // in another's land drew build.c's builders to it
    for (int k = 0; why && k < c->ntow; k++) if (!cr_done(c->tow[k])) unit_destroy(c->tow[k], 0, 0, 0);
    c->target = NULL; c->ntow = 0; c->ncrew = 0; c->end_t = now ? now : 1;
}
// enemy citizens (they fight) pl sees within r of p
internal int cr_cits(ee_player *pl, ivec2 p, int r) {
    int n = 0;
    for (struct grid_it it = grid_it_box(p.x, p.y, r, 1); grid_it_next(&it); ) {
        ee_player *o = unit_get_owner(it.u);
        if (o && o != pl && player_get_index(o) > 0 && is_citizen_type(unit_type_get_id(it.td)) && unit_get_hp(it.u) > 0 && is_enemy(pl, o) && seen_by(it.u, pl)) n++;
    }
    return n;
}
// the enemy defences it knows covering q (but skip, the target)
internal int cr_covered(unsigned idx, ivec2 q, ee_unit *skip) {
    for (int k = 0; k < g_threat[idx].n; k++) {
        struct defence *d = &g_threat[idx].d[k];
        if (d->u == skip) continue;
        float r = d->range + CR_MARGIN, dx = (float)(q.x - d->x), dy = (float)(q.y - d->y);
        if (dx * dx + dy * dy <= r * r) return 1;
    }
    return 0;
}
// a spot for a tower of ttype within its range of target, nearest to the own buildings own[0..nown), out of the reach
// of every enemy defence it knows but skip (NULL: every one); 0 none
internal int cr_spot(ee_player *pl, unsigned idx, int ttype, ee_unit *target, ee_unit *skip, unsigned land, const ivec2 *own, int nown, ivec2 *out) {
    ee_unit_type *td = type_data(ttype), *gd = unit_get_type_data(target);
    if (!td || !gd) return 0;
    float rng = unit_type_get_range(td), half = (float)unit_type_get_size(gd) * 0.5f;
    if (!(rng > 2.0f) || rng > 30.0f) return 0;
    float r1 = rng + half - CR_IN, r0 = r1 - 2.0f;   // a ring two tiles wide just inside its reach
    ivec2 t = unit_get_tile(target);
    int box = (int)r1 + 1, best = 0x7FFFFFFF;
    for (int ax = -box; ax <= box; ax++)
        for (int ay = -box; ay <= box; ay++) {
            float d2 = (float)(ax * ax + ay * ay);
            ivec2 q = iv2(t.x + ax, t.y + ay);
            if (d2 > r1 * r1 || d2 < r0 * r0 || (land && area_id(q, 2) != land) || cr_covered(idx, q, skip) || (!skip && wk_danger(idx, q)) || !can_place_open(pl, ttype, q)) continue;
            int sc = 0x7FFFFFFF;
            for (int k = 0; k < nown; k++) { int d = ivec2_dist2(own[k], q); if (d < sc) sc = d; }
            if (sc < best) { best = sc; *out = q; }
        }
    return best < 0x7FFFFFFF;
}
internal void creep_tick(ee_player *pl, uint32_t now) {
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    struct cr_st *c = &g_cr[idx];
    if (!pl || GAME_AI(idx) || !TOWER_CREEP(idx)) return;
    if (now < c->t) memset(c, 0, sizeof *c);
    if (c->t && now - c->t < CR_EVERY) return;
    c->t = now ? now : 1;
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    int ttype = tower_newest(pl);
    if (!bm) return;
    threat_refresh(pl, now);
    // the creep going on: its target, towers and crew as they are
    if (c->target) {
        int m = 0;
        for (int k = 0; k < c->ntow; k++) if (cr_alive(pl, c->tow[k], c->tow_id[k])) { c->tow[m] = c->tow[k]; c->tow_id[m++] = c->tow_id[k]; }
        c->ntow = m; m = 0;
        for (int k = 0; k < c->ncrew; k++) if (cr_alive(pl, c->crew[k], c->crew_id[k])) { c->crew[m] = c->crew[k]; c->crew_id[m++] = c->crew_id[k]; }
        c->ncrew = m;
        ee_player *to = unit_get_serial(c->target) == c->tid ? unit_get_owner(c->target) : NULL;
        int stood = 0, going = -1;
        for (int k = 0; k < c->ntow; k++) { if (cr_done(c->tow[k])) stood++; else going = k; }
        if (stood && !c->stood_t) c->stood_t = now ? now : 1;
        if (!to || unit_get_hp(c->target) <= 0 || !is_enemy(pl, to)) { cr_end(idx, c, now, NULL); return; }
        const char *why = c->ncrew < CR_CREW_MIN && !stood ? "its crew is gone" :
                          !stood && raiders_near(pl, c->spot, CR_FOE_R) ? "enemy soldiers came before a tower stood" :
                          !stood && cr_cits(pl, c->spot, CR_FOE_R) >= c->ncrew ? "the owner's citizens there outnumber its crew" :
                          !c->ntow && c->lays >= CR_RELAY ? "its foundations were destroyed" :
                          !c->ntow && c->laid_t && now - c->laid_t >= CR_LAY_WAIT ? "no tower laid" :
                          now - c->start_t >= CR_GIVEUP ? "it took too long" : NULL;
        if (why) { cr_end(idx, c, now, why); return; }
        // THE WALK: to the spot, before the first foundation
        if (!c->lays) {
            int there = 0;
            for (int k = 0; k < c->ncrew; k++) there += ivec2_dist2(unit_get_tile(c->crew[k]), c->spot) <= CR_THERE * CR_THERE;
            if (2 * there < c->ncrew) {
                float sx = (float)c->spot.x + 0.5f, sy = (float)c->spot.y + 0.5f;
                for (int k = 0; k < c->ncrew; k++) {
                    ee_unit *u = c->crew[k];
                    ee_goal *g = unit_get_goal(u);
                    if (ivec2_dist2(unit_get_tile(u), c->spot) <= CR_THERE * CR_THERE || unit_get_frozen(u) || unit_get_ai_held(u) || unit_get_busy(u) ||
                        (g && goal_is_location(g) && fabsf(goal_get_dest_x(g) - sx) < 2 && fabsf(goal_get_dest_y(g) - sy) < 2)) continue;
                    unit_goal(u, goal_move(sx, sy), 0, 0x1F40);
                }
                return;
            }
        }
        // the next tower: none going up, the last one stands, fewer than the most for its target
        int most = c->tower ? CR_TOWERS_T : CR_TOWERS_B;
        if (going < 0 && c->ntow < most && (!c->ntow || (stood == c->ntow && (c->tower || now - c->stood_t >= CR_SECOND))) &&
            ttype && can_build(pl, ttype) && !prio_refuses(pl, ttype) && bd_pays(pl, ttype, g_bd_com[idx])) {
            ivec2 at = c->spot;
            ivec2 own[CR_TOWERS_T + 1];
            int nown = 0;
            own[nown++] = c->spot;
            ee_unit *skip = c->exposed ? c->target : NULL;
            if (c->ntow) cr_spot(pl, idx, ttype, c->target, skip, area_id(c->spot, 2), own, nown, &at);   // the next beside the first
            ee_unit *b = can_place_open(pl, ttype, at) && !cr_covered(idx, at, skip) ? place_foundation(pl, ttype, at.x, at.y) : NULL;
            if (b) {
                c->tow[c->ntow] = b; c->tow_id[c->ntow++] = unit_get_serial(b); going = c->ntow - 1; c->laid++; c->lays++; c->laid_t = now ? now : 1;
                if (g_log) log_locked("# creep: P%u at %.1f min: tower %d laid at %d,%d against %d at %d,%d (%s, tower %d of the creep, %d of its crew)\n", idx, now / 60000.0,
                                      ttype, at.x, at.y, c->ttype, c->tp.x, c->tp.y, c->tower ? "a defence" : "a building", c->ntow, c->ncrew);
            }
        }
        // the crew: onto the tower going up, else the most damaged one
        ee_unit *job = going >= 0 ? c->tow[going] : NULL;
        if (!job) {
            float worst = 0.95f;
            for (int k = 0; k < c->ntow; k++) {
                float f = (float)unit_get_hp(c->tow[k]) / (float)(cr_hp_max(c->tow[k]) > 0 ? cr_hp_max(c->tow[k]) : 1);
                if (f < worst) { worst = f; job = c->tow[k]; }
            }
        }
        for (int k = 0; job && k < c->ncrew; k++) {
            ee_unit *u = c->crew[k];
            ee_goal *g = unit_get_goal(u);
            int on = 0;
            if (g && !strcmp(rtti_name(g), "EEUGBuildRepair@@")) for (int j = 0; j < c->ntow && !on; j++) on = goal_get_building(g) == c->tow[j];
            if (on || unit_get_frozen(u) || unit_get_ai_held(u) || unit_get_busy(u)) continue;
            if (bd_send(pl, u, job)) c->sends++;
        }
        return;
    }
    // a new creep
    if (!ttype || !can_build(pl, ttype) || (c->end_t && now >= c->end_t && now - c->end_t < CR_AGAIN)) return;
    if (prio_refuses(pl, ttype) || !bd_pays(pl, ttype, g_bd_com[idx]) || workforce(pl) < g_min_workforce + CR_CREW) return;
    static ivec2 own[CR_OWN];
    static struct { ee_unit *u; int d; } cand[CR_CAND];
    int nown = 0, ncand = 0, count = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384 && nown < CR_OWN; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
        int fam = unit_type_get_class_id(td);
        if ((fam == CLS_BUILDING || fam == CLS_FORTRESS) && !is_wall(unit_type_get_id(td)) && cr_done(u)) own[nown++] = unit_get_tile(u);
    }
    if (!nown) return;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it) && ncand < CR_CAND; ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        int fam = it.td ? unit_type_get_class_id(it.td) : -1;
        if ((fam != CLS_BUILDING && fam != CLS_FORTRESS) || !o || player_get_index(o) <= 0 || !is_enemy(pl, o) || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 ||
            is_wall(unit_type_get_id(it.td)) || !cr_done(u) || !seen_by(u, pl) || cr_is_bad(c, unit_get_serial(u), now)) continue;   // a foundation is no target (one went for another's tower going up: gone before its own was laid)
        ivec2 p = unit_get_tile(u);
        int d = 0x7FFFFFFF;
        for (int k = 0; k < nown; k++) { int e = ivec2_dist2(own[k], p); if (e < d) d = e; }
        if (d > CR_REACH * CR_REACH) continue;
        cand[ncand].u = u; cand[ncand++].d = d;
    }
    // the crew's citizens: the free ones (gathering or idle)
    static struct { ee_unit *u; ivec2 p; } free_[2048];
    int nfree = 0;
    count = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384 && nfree < 2048; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || !is_citizen_type(unit_type_get_id(td)) || !(citizen_free(u, 0) || citizen_free(u, 1))) continue;
        free_[nfree].u = u; free_[nfree++].p = unit_get_tile(u);
    }
    if (nfree < CR_CREW) return;
    for (int tries = 0; tries < CR_TRIES && ncand; tries++) {
        int bj = 0;
        for (int j = 1; j < ncand; j++) if (cand[j].d < cand[bj].d) bj = j;
        ee_unit *tg = cand[bj].u;
        int tdist = cand[bj].d;
        cand[bj] = cand[--ncand];
        ivec2 tp = unit_get_tile(tg), at;
        float r, a;
        int tower = defence_stats(tg, &r, &a), exposed = 0;
        unsigned tr = ter_near(tp.x, tp.y);
        struct tv_terr *tt = tr < (unsigned)g_tv[idx].n && g_tv[idx].version == g_ter.version ? &g_tv[idx].t[tr] : NULL;
        if (!tt || tt->foe || (tt->foe_t && now >= tt->foe_t && now - tt->foe_t < CR_QUIET) || raiders_known(pl, tp, CR_FOE_R) || cr_cits(pl, tp, CR_FOE_R) > CR_CITS) continue;   // guarded
        // its land: the nearest free citizen's
        int nk = 0;
        for (int k = 1; k < nfree; k++) if (ivec2_dist2(free_[k].p, tp) < ivec2_dist2(free_[nk].p, tp)) nk = k;
        unsigned land = area_id(free_[nk].p, 2);
        if (!cr_spot(pl, idx, ttype, tg, NULL, land, own, nown, &at)) {   // in the target tower's reach: only as far as its own reaches
            ee_unit_type *otd = type_data(ttype);
            if (!tower || !otd || unit_type_get_range(otd) + 0.5f < r || nfree < CR_CREW_TOWER || !cr_spot(pl, idx, ttype, tg, tg, land, own, nown, &at)) continue;
            exposed = 1;
        }
        int want = exposed ? CR_CREW_TOWER : CR_CREW;
        if (nfree < want || workforce(pl) < g_min_workforce + want || raiders_known(pl, at, CR_FOE_R) || cr_cits(pl, at, CR_FOE_R) > CR_CITS) continue;
        memset(c->tow, 0, sizeof c->tow);
        c->target = tg; c->tid = unit_get_serial(tg); c->ttype = unit_type_get_id(unit_get_type_data(tg)); c->tower = tower; c->exposed = exposed; c->lays = 0; c->tp = tp; c->spot = at;
        c->start_t = c->laid_t = now ? now : 1; c->stood_t = 0; c->ntow = 0; c->ncrew = 0;
        while (c->ncrew < want && nfree > 0) {   // the nearest to the spot, on its land
            int bk = -1, bd = 0x7FFFFFFF;
            for (int k = 0; k < nfree; k++) {
                int d = ivec2_dist2(free_[k].p, at);
                if (d < bd && (!land || area_id(free_[k].p, 2) == land)) { bd = d; bk = k; }
            }
            if (bk < 0) break;
            c->crew[c->ncrew] = free_[bk].u; c->crew_id[c->ncrew++] = unit_get_serial(free_[bk].u);
            free_[bk] = free_[--nfree];
        }
        c->started++;
        if (g_log) log_locked("# creep: P%u at %.1f min: a creep on %d at %d,%d (%s of P%d, %.0f tiles from its nearest building) from %d,%d with %d citizens, tower %d\n",
                              idx, now / 60000.0, c->ttype, tp.x, tp.y, !tower ? "a building" : exposed ? "a defence, in its reach" : "a defence, from beyond its reach", player_get_index(unit_get_owner(tg)), sqrtf((float)tdist),
                              at.x, at.y, c->ncrew, ttype);
        c->t = now > CR_EVERY ? now - CR_EVERY : 1;   // its first tower at the next look
        return;
    }
}
internal void creep_new_game(void) { memset(g_cr, 0, sizeof g_cr); }
internal void creep_install(const char *autotest_ini) {
    g_tower_creep = g_set_ai[1].tower_creep;
    if (autotest_ini) g_v_tower_creep = ini_int("autotest", "v_tower_creep", -1, autotest_ini);
}
