// THE FERRY SERVICE ([ai_base] ferry_service; the player 2026-11-17, on citizens and soldiers kept from the other lands:
// "citezen balacning acroos territories dosnt work across water yet right? / or general reinforcment" - and yes to one
// service both use). The force's shares (stations.c) and the citizens' work (work.c) walk the land: sn_give over the
// territory graph without its seas, a citizen to sites on its own land only. So a territory across the water kept short
// of its share however many soldiers stood spare at home, and free gold across the water stayed free however many cut
// wood at home; only the outposts' ferry (expand.c, a fixed number of trips) and the landings crossed.
// AS ON LAND (the player 2026-11-18: "remove the limiters, the reinforcment/balancing should work like on land, and if an
// ferry is bussy, aditonal should be build"): every FY_LOOK, by land (any two lands of its - islands, a continent and
// an island, a custom map's mix alike), soldiers from a land over its territories' shares to one short of them, and
// citizens - idle ones first, then wood cutters - from a land with any to spare to one with free places at the mines
// of resources it wants more citizens on and a drop site of its there: as many trips at once as it has transports
// for (FY_TRIPS), each one transport, however few go along. Those aboard or on their way to the shore count for
// the land they go to (the stations and work.c leave a passenger alone - expand_has - so they count for neither
// land otherwise: with one trip at a time and no such count 148 soldiers went to and fro between two lands that each
// stood within 3 of their share, Small Islands seed 4). TURNS: the kind that did not start the last trip goes first -
// one trip at a time and soldiers always first, the player's game of 2026-11-18 ran 30 trips in a row for a
// territory with enemy soldiers that kept 0 of its share of 363 (254 soldiers and 12 citizens put off) while 254
// citizens stood idle at home and the other lands had 12-18 free places at their mines each.
// A trip: the nearest free transport on a sea both lands touch (no landing's, no outpost ferry's, no rescue's, no
// other trip's; empty) goes to the shore nearest the passengers' middle, they walk there meanwhile, board, and are put
// off at the shore nearest where they are wanted; there they are the others' again - the stations count a soldier
// where it stands, work.c gives a citizen the territory it stands in. MORE TRANSPORTS: a trip that finds no free one
// has one ordered (the expansion's way of paying - the priority list's row holds its price meanwhile,
// expand_transport_hold), as many as trips found none beyond those ordered within FY_ORDER.
// MEASURED 2026-11-18 (Small Islands large, 4 AIs, from epoch 5, 60 min, 20 seeds, every AI the same DLL; one trip at a
// time with its limits -> as on land, per AI): trips 3.2 -> 10.8, transports trained 1.0 -> 4.2, citizens put off 9.4 ->
// 23.3 (se 2.5), citizens on other islands 10.6 -> 16.0 (2.1), miners there 6.6 -> 9.2 (1.4), soldiers put off 4.2 ->
// 7.0, soldiers at the end +2.2 (3.2); trips back the way one came within 10 minutes 1.7 (2.5 before GIVING's band on
// the short side): a land under attack is short by any, as on land.
// "# ferry:" lines; each minute what the lands want and offer.
#define FY_LOOK 5000
#define FY_SHORE 60       // shores searched within this many tiles
#define FY_BOARD 240000   // no one aboard this long: given up
#define FY_LOAD 90000     // some aboard this long: off with those
#define FY_SEA 360000     // not unloaded this long: put off where it is
#define FY_ORDER 90000    // a transport ordered counts as coming this long
#define FY_FRESH 15000    // the lands' tables older than this: no trip
internal PRINTF_LIKE(3, 4) void fy_say(unsigned idx, uint32_t now, const char *fmt, ...) {
    if (!g_log) return;
    char buf[400];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    log_locked("# ferry: P%u at %.1f min: %s\n", idx, now / 60000.0, buf);
}
internal void fy_end(ee_player *pl, unsigned idx, struct fy_trip *t, uint32_t now, const char *why) {
    if (t->phase >= 2 && ex_alive(&t->ship, pl) && ex_cargo(t->ship.u) > 0) {   // someone aboard: off at the nearest land with room
        ivec2 at = unit_get_tile(t->ship.u), d;
        if (ex_shore(t->to, t->ocean, at, 40, &d, NULL) || ex_shore(t->from, t->ocean, at, 40, &d, NULL))
            unit_goal(t->ship.u, ld_unload(t->ship.u, d), 0, 0x1F40);
    }
    if (why) fy_say(idx, now, "trip %u (risk %.2f): %s", t->no, (double)t->risk, why);
    t->phase = 0; t->npass = 0; t->ship.u = NULL;
}
// a free transport: its own, a ship, on the map, empty, nobody else's (expand_ship: no trip's either)
internal int fy_free_ship(ee_player *pl, ee_unit *u, ee_unit_type *td) {
    return unit_get_owner(u) == pl && unit_get_hp(u) > 0 && unit_get_x(u) >= 0 && is_transport(unit_type_get_id(td)) &&
           ex_cargo(u) <= 0 && !landing_has_unit(pl, u) && !expand_ship(u);
}
internal int sn_ferry_pick(ee_player *pl, unsigned land, ivec2 toward, ee_unit **out, int max);    // stations.c
internal int wk_ferry_pick(ee_player *pl, unsigned land, ivec2 toward, ee_unit **out, int max);    // work.c
// a trip of kind (1 soldiers, 2 citizens) of n from land a to land b, wanted at dest, as trip t: 1 when it set out, 0
// nobody to take, -1 no free transport on a sea between the two, -2 not now for the risk (THE CROSSING)
// THE CROSSING (crossing.c): the risk a trip of kind may take now - an important one (imp) arriving before it is sunk,
// any other coming through, more the longer none of its kind went for the risk
internal float fy_tolerance(const struct fy_st *f, int kind, int imp, uint32_t now) {
    if (imp) return CX_GO_HIGH;
    float tol = CX_GO_LOW;
    if (f->risky_t[kind] && now >= f->risky_t[kind]) tol += (float)(now - f->risky_t[kind]) / 60000.0f * CX_GO_STEP;
    return tol < CX_GO_WAIT ? tol : CX_GO_WAIT;
}
internal int fy_start(ee_player *pl, unsigned idx, struct fy_st *f, struct fy_trip *t, int kind, int imp, unsigned a, unsigned b, int n, ivec2 dest, uint32_t now) {
    ee_unit *pass[16];
    if (n > 16) n = 16;
    int np = kind == 1 ? sn_ferry_pick(pl, a, dest, pass, n) : wk_ferry_pick(pl, a, dest, pass, n);
    if (np < 1) return 0;
    long sx = 0, sy = 0;
    for (int k = 0; k < np; k++) { sx += unit_get_x(pass[k]); sy += unit_get_y(pass[k]); }
    ivec2 mid = iv2((int)(sx / np), (int)(sy / np));
    // the transport: the nearest free one on a sea both lands touch (each sea's shores looked for once)
    struct { int oc, ok; ivec2 pick, pick_w, drop, drop_w; } seas[4];
    int ns = 0, frees = 0;
    ee_unit *best = NULL;
    int bd = 0x7FFFFFFF, bs = -1;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        if (!fy_free_ship(pl, it.u, it.td)) continue;
        frees++;
        int oc = (int)ocean_near(unit_get_tile(it.u), 2, NULL), s = 0;
        if (!oc) continue;
        while (s < ns && seas[s].oc != oc) s++;
        if (s == ns) {
            if (ns == 4) continue;
            seas[s].oc = oc;
            seas[s].ok = ex_shore(a, oc, mid, FY_SHORE, &seas[s].pick, &seas[s].pick_w) && ex_shore(b, oc, dest, FY_SHORE, &seas[s].drop, &seas[s].drop_w);
            ns++;
        }
        if (!seas[s].ok) continue;
        int d = ivec2_dist2(unit_get_tile(it.u), seas[s].pick_w);
        if (d < bd) { bd = d; best = it.u; bs = s; }
    }
    if (!best) {
        if (frees) f->no_shore++; else f->no_ship++;   // free ones, but on no sea between the two - or none
        return -1;
    }
    int cap = ld_cap(unit_type_get_id(unit_get_type_data(best))), room = 0;
    t->npass = 0;
    for (int k = 0; k < np && t->npass < 16; k++) {   // as many as fit (a gun or a rider takes more than one place)
        int sz = tw_size(pass[k]);
        if (room + sz > cap) continue;
        room += sz;
        t->pass[t->npass].u = pass[k]; t->pass[t->npass++].id = unit_get_serial(pass[k]);
    }
    if (!t->npass) return 0;
    int hv = 0;
    ivec2 via = iv2(0, 0);
    float risk = 0;
    {   // rated with the check off too: the log's risk against what came of the trip
        struct cx_st *c = &g_cx_st[idx & 15];
        risk = crossing_rate(pl, idx, best, seas[bs].pick_w, seas[bs].drop_w, (unsigned)seas[bs].oc, &via, &hv);
        if (CROSSING_SAFETY(idx) && risk > fy_tolerance(f, kind, imp, now)) {
            if (!f->risky_t[kind]) f->risky_t[kind] = now ? now : 1;
            if (!(c->held++ % 10)) {   // every tenth: what holds it
                char w[160];
                cx_top_text(idx, w, sizeof w);
                fy_say(idx, now, "the crossing: %d %s from land %u to land %u held - risk %.2f over %.2f%s; the worst: %s", t->npass, kind == 1 ? "soldiers" : "citizens", a, b,
                       (double)risk, (double)fy_tolerance(f, kind, imp, now), imp ? " (important)" : "", w);
            }
            t->npass = 0;
            return -2;
        }
        if (CROSSING_SAFETY(idx) && !imp && risk > CX_FULL && room * 2 < cap) { c->held_load++; t->npass = 0; return -2; }   // into danger with a full load only
        if (CROSSING_SAFETY(idx) && imp && risk > CX_GO_LOW) c->gone_high++;
        if (!CROSSING_SAFETY(idx)) hv = 0;   // the way straight, as without the check
    }
    f->risky_t[kind] = 0;
    t->imp = imp; t->risk = risk; t->has_via = hv; t->via = via; t->via_done = 0; t->drop_w = seas[bs].drop_w; t->rate_t = now ? now : 1;
    t->kind = kind; t->from = a; t->to = b; t->ocean = seas[bs].oc;
    t->pick = seas[bs].pick; t->pick_w = seas[bs].pick_w; t->drop = seas[bs].drop; t->dest = dest;
    t->ship.u = best; t->ship.id = unit_get_serial(best);
    t->phase = 1; t->since = now ? now : 1; t->load_t = 0; t->reissue = 0;
    f->trips++; f->last_kind = kind; t->no = f->trips;
    unit_goal(best, goal_move((float)t->pick_w.x + 0.5f, (float)t->pick_w.y + 0.5f), 0, 0x1F40);
    for (int k = 0; k < t->npass; k++)   // to the shore meanwhile
        unit_goal(t->pass[k].u, goal_move((float)t->pick.x + 0.5f, (float)t->pick.y + 0.5f), 0, 0x1F40);
    fy_say(idx, now, "trip %u: %d %s from land %u (by %d,%d) to land %u (wanted at %d,%d) - the transport at %d,%d, boarding at %d,%d, off at %d,%d%s; risk %.2f%s",
           t->no, t->npass, kind == 1 ? "soldiers" : "citizens", a, mid.x, mid.y, b, dest.x, dest.y, unit_get_x(best), unit_get_y(best),
           t->pick.x, t->pick.y, t->drop.x, t->drop.y, hv ? " by a way off the line" : "", (double)risk, imp ? " (important)" : "");
    return 1;
}
internal void fy_order(ee_player *pl, unsigned idx, struct fy_st *f, int wanted, uint32_t now) {   // MORE TRANSPORTS
    int coming = 0, slot = -1;
    for (int k = 0; k < 4; k++) {
        if (f->order_t[k] && now >= f->order_t[k] && now - f->order_t[k] < FY_ORDER) coming++;
        else if (slot < 0) slot = k;
    }
    if (wanted <= coming || slot < 0) return;
    void *cm = player_get_civ_mgr(pl);
    ee_unit_mgr *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    int tt = ld_transport_type(pl);
    if (!um || !tt) return;
    int req[4] = { tt, 1, 0, 0 };
    unsigned got = 0;
    g_priority_mil = 1; g_landing_order = 1; g_expand_econ = 1;   // paid as the expansion's (expand.c ex_ferry_ship)
    hook_train_units(um, req, (uintptr_t)&got);
    g_priority_mil = 0; g_landing_order = 0; g_expand_econ = 0;
    if (!got) return;
    f->order_t[slot] = now ? now : 1; f->trained++;
    fy_say(idx, now, "trains a transport (type %d): %d trips found none free, %d ordered before still to come", tt, wanted, coming);
}
internal void fy_look(ee_player *pl, unsigned idx, struct fy_st *f, uint32_t now) {
    typeof(&g_snl[0]) S = &g_snl[idx];
    typeof(&g_wkl[0]) W = &g_wkl[idx];
    int sfresh = S->t && now >= S->t && now - S->t <= FY_FRESH, wfresh = W->t && now >= W->t && now - W->t <= FY_FRESH;
    // the lands' gaps, those on their way counted for the land they go to
    int sgap[SN_LANDS], cfree[WK_LANDS], cspare[WK_LANDS];
    for (int k = 0; k < S->n; k++) sgap[k] = S->l[k].gap;
    for (int k = 0; k < W->n; k++) { cfree[k] = W->l[k].free; cspare[k] = W->l[k].spare; }
    for (int i = 0; i < FY_TRIPS; i++) {
        struct fy_trip *t = &f->t[i];
        if (!t->phase) continue;
        if (t->kind == 1) { for (int k = 0; k < S->n; k++) if (S->l[k].land == t->to) sgap[k] -= t->npass; }
        else for (int k = 0; k < W->n; k++) if (W->l[k].land == t->to) cfree[k] -= t->npass;
    }
    drops_cached(pl);
    int blocked[3] = { 0, !sfresh, !wfresh }, unserved = 0;
    for (int tries = 0; tries < FY_TRIPS; tries++) {
        struct fy_trip *t = NULL;
        for (int i = 0; i < FY_TRIPS && !t; i++) if (!f->t[i].phase) t = &f->t[i];
        if (!t) break;
        // soldiers: the pair that moves most - the short land, the one with spare
        int bs = -1, as = -1, bn = 0;
        for (int b = 0; !blocked[1] && b < S->n; b++) {
            if (sgap[b] < 1) continue;
            for (int a = 0; a < S->n; a++) {
                if (a == b || sgap[a] > -1) continue;
                int n = sgap[b] < -sgap[a] ? sgap[b] : -sgap[a];
                if (n > bn) { bn = n; bs = b; as = a; }
            }
        }
        // citizens: the land with most free places (a drop site of its there), from the one with most to spare
        int bc = -1, ac = -1;
        for (int k = 0; !blocked[2] && k < W->n; k++) if (cfree[k] >= 1 && rs_drop_land(idx, W->l[k].land) && (bc < 0 || cfree[k] > cfree[bc])) bc = k;
        for (int k = 0; bc >= 0 && k < W->n; k++) if (k != bc && cspare[k] >= 1 && (ac < 0 || cspare[k] > cspare[ac])) ac = k;
        int can[3] = { 0, bs >= 0, bc >= 0 && ac >= 0 };
        if (!can[1] && !can[2]) break;
        int first = can[2] && (!can[1] || f->last_kind == 1) ? 2 : 1;   // TURNS
        for (int j = 0; j < 2; j++) {
            int kind = j ? 3 - first : first;
            if (!can[kind]) continue;
            int r = kind == 1 ? fy_start(pl, idx, f, t, 1, S->l[bs].hot, S->l[as].land, S->l[bs].land, bn, S->l[bs].at, now)
                              : fy_start(pl, idx, f, t, 2, 0, W->l[ac].land, W->l[bc].land, cfree[bc] < cspare[ac] ? cfree[bc] : cspare[ac], W->l[bc].at, now);
            if (r > 0) {
                if (kind == 1) { sgap[bs] -= t->npass; sgap[as] += t->npass; }
                else { cfree[bc] -= t->npass; cspare[ac] -= t->npass; }
                break;
            }
            blocked[kind] = 1;
            if (r == -1) unserved++;
        }
    }
    if (unserved) { f->want_t = now ? now : 1; fy_order(pl, idx, f, unserved, now); }
    else f->want_t = 0;
}
internal void fy_minute(unsigned idx, struct fy_st *f, uint32_t now) {
    if (!g_log || (f->log_t && now >= f->log_t && now - f->log_t < 60000)) return;
    f->log_t = now ? now : 1;
    char b[300] = "";
    typeof(&g_snl[0]) S = &g_snl[idx];
    typeof(&g_wkl[0]) W = &g_wkl[idx];
    for (int k = 0; k < S->n; k++) { size_t l = strlen(b); if (l < sizeof b - 40) snprintf(b + l, sizeof b - l, " %u:%d/%d", S->l[k].land, S->l[k].have, S->l[k].want); }
    size_t l = strlen(b);
    snprintf(b + l, sizeof b - l, " | citizens land:spare/free places");
    for (int k = 0; k < W->n; k++) { l = strlen(b); if (l < sizeof b - 40) snprintf(b + l, sizeof b - l, " %u:%d/%d", W->l[k].land, W->l[k].spare, W->l[k].free); }
    int on[4] = { 0 };
    for (int i = 0; i < FY_TRIPS; i++) on[f->t[i].phase & 3]++;
    fy_say(idx, now, "%u trips (%u soldiers, %u citizens put off; %u given up, %u transports sunk, %u put off on a beach under fire), %u transports trained, looks without a free transport %u, with none between the lands %u; on now: %d to the pick-up, %d boarding, %d crossing | soldiers land:stationed/share%s",
           f->trips, f->soldiers, f->citizens, f->failed, f->sunk, f->beached, f->trained, f->no_ship, f->no_shore, on[1], on[2], on[3], b);
    if (CROSSING_SAFETY(idx)) {
        struct cx_st *c = &g_cx_st[idx & 15];
        fy_say(idx, now, "the crossing: %u ways rated (%u off the line), trips held for the risk %u, for a load too small for it %u, important ones that went into danger %u; threats known %d",
               c->rated, c->detours, c->held, c->held_load, c->gone_high, c->n);
    }
}
// one trip's step
internal void fy_trip_act(ee_player *pl, unsigned idx, struct fy_st *f, struct fy_trip *t, uint32_t now) {
    if (!ex_alive(&t->ship, pl)) {
        f->sunk++; f->failed++;
        if (t->phase == 3) cx_sunk(idx, t->at, now);   // THE CROSSING: as if what sank it were there still
        fy_end(pl, idx, t, now, t->phase == 3 ? "the transport was sunk on the way" : "the transport is gone");
        return;
    }
    ee_unit *sh = t->ship.u;
    t->at = unit_get_tile(sh);
    if (convoy_lifeboat(pl, sh)) return;   // LIFEBOAT (convoy.c): under fire, to the nearest beach - stands back meanwhile
    if (t->phase >= 2 && convoy_beached(idx, sh, t->since)) { f->beached++; fy_end(pl, idx, t, now, "the transport was under fire - its passengers put off on the nearest beach"); return; }
    int n = 0, aboard = 0;
    for (int k = 0; k < t->npass; k++) if (ex_alive(&t->pass[k], pl)) t->pass[n++] = t->pass[k];
    t->npass = n;
    for (int k = 0; k < t->npass; k++) aboard += unit_get_x(t->pass[k].u) < 0;
    if (t->phase == 1 || t->phase == 2) {
        if (!t->npass) { fy_end(pl, idx, t, now, "no passenger left"); return; }
        if (now - t->since >= FY_BOARD && !aboard) { f->failed++; fy_end(pl, idx, t, now, "boarding failed in 4 minutes"); return; }
        if (t->phase == 1) {
            int there = ivec2_dist2(unit_get_tile(sh), t->pick_w) <= 49;
            if (!there && now - t->since < 120000) {
                convoy_escort(idx, sh, t->pick_w, t->drop, 0, now);
                if (!goal_near(sh, (float)t->pick_w.x + 0.5f, (float)t->pick_w.y + 0.5f, 2.0f))
                    unit_goal(sh, goal_move((float)t->pick_w.x + 0.5f, (float)t->pick_w.y + 0.5f), 0, 0x1F40);
                return;
            }
            if (CROSSING_SAFETY(idx) && !aboard) {   // THE CROSSING: rated again before anyone boards - too risky, it waits off the shore, empty
                if (now - t->rate_t >= CX_LOOK) {
                    t->rate_t = now ? now : 1;
                    t->risk = crossing_rate(pl, idx, sh, t->pick_w, t->drop_w, (unsigned)t->ocean, &t->via, &t->has_via);
                    if (t->risk > fy_tolerance(f, t->kind, t->imp, now)) { if (!f->risky_t[t->kind]) f->risky_t[t->kind] = now ? now : 1; t->load_t = 0; }
                    else t->load_t = 1;
                }
                if (!t->load_t) { convoy_escort(idx, sh, t->pick_w, t->drop, 0, now); return; }
            }
            t->phase = 2; t->load_t = now ? now : 1;
        }
        convoy_escort(idx, sh, t->pick_w, t->drop, 0, now);   // ESCORT (convoy.c): asked for from the start - the scouts sweep the way while it comes and boards
        if (aboard == t->npass || (aboard && now - t->load_t >= FY_LOAD)) {
            int m = 0;   // those left behind are the others' again
            for (int k = 0; k < t->npass; k++) if (unit_get_x(t->pass[k].u) < 0) t->pass[m++] = t->pass[k];
            t->npass = m;
            unit_goal(sh, t->has_via ? goal_move((float)t->via.x + 0.5f, (float)t->via.y + 0.5f) : ld_unload(sh, t->drop), 0, 0x1F40);
            t->phase = 3; t->since = now ? now : 1;
            fy_say(idx, now, "trip %u: %d aboard - to land %u, off at %d,%d", t->no, aboard, t->to, t->drop.x, t->drop.y);
            return;
        }
        if (t->reissue && now - t->reissue < 20000) return;   // the load goal (again every 20 s for those not aboard)
        t->reissue = now ? now : 1;
        ee_unit *list[16];
        int m = 0;
        for (int k = 0; k < t->npass; k++) if (unit_get_x(t->pass[k].u) >= 0) list[m++] = t->pass[k].u;
        if (!m) return;
        ee_goal *g = goal_load(sh, list, m);
        if (!g || unit_get_goal(sh) != g) return;   // a load goal the transport didn't keep is deleted - no parts from it
        for (int k = 0; k < m; k++) unit_goal(list[k], goal_load_part(g), 0, 0x1F40);
        return;
    }
    // phase 3: across
    if (ex_cargo(sh) <= 0) {
        int off = 0;
        for (int k = 0; k < t->npass; k++) off += unit_get_x(t->pass[k].u) >= 0;
        if (t->kind == 1) f->soldiers += (unsigned)off; else f->citizens += (unsigned)off;
        fy_say(idx, now, "trip %u (risk %.2f): %d %s put off on land %u at %d,%d", t->no, (double)t->risk, off, t->kind == 1 ? "soldiers" : "citizens", t->to, t->drop.x, t->drop.y);
        if (t->kind == 1)   // on to where they are wanted; the stations take them there
            for (int k = 0; k < t->npass; k++) unit_goal(t->pass[k].u, goal_move((float)t->dest.x + 0.5f, (float)t->dest.y + 0.5f), 0, 0x1F40);
        fy_end(pl, idx, t, now, NULL);
        return;
    }
    if (now - t->since >= FY_SEA) { f->failed++; fy_end(pl, idx, t, now, "not across in 6 minutes - put off where it is"); return; }
    convoy_escort(idx, sh, t->pick_w, t->drop, 1, now);   // ESCORT (convoy.c)
    if (t->has_via && !t->via_done) {   // THE CROSSING: the way off the line - its point first
        if (ivec2_dist2(unit_get_tile(sh), t->via) <= 16) t->via_done = 1;
        else {
            if (!goal_near(sh, (float)t->via.x + 0.5f, (float)t->via.y + 0.5f, 2.0f)) unit_goal(sh, goal_move((float)t->via.x + 0.5f, (float)t->via.y + 0.5f), 0, 0x1F40);
            return;
        }
    }
    ee_goal *cur = unit_get_goal(sh);
    if (!cur || !goal_is_unload(cur)) unit_goal(sh, ld_unload(sh, t->drop), 0, 0x1F40);
}
internal void ferry_act(ee_player *pl, unsigned idx, uint32_t now) {
    struct fy_st *f = &g_fy[idx];
    fy_minute(idx, f, now);
    for (int i = 0; i < FY_TRIPS; i++) if (f->t[i].phase) fy_trip_act(pl, idx, f, &f->t[i], now);
    if (f->look_t && now >= f->look_t && now - f->look_t < FY_LOOK) return;
    f->look_t = now ? now : 1;
    fy_look(pl, idx, f, now);
}
