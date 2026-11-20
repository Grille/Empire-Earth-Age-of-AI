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
#define FY_GATHER 180000  // GATHERED FIRST: a push waits this long at most for transports enough
#define FY_MEET 75000     // TOGETHER: at the meeting point this long at most for the push's other transports
#define FY_MEET_SAIL 150000   // ... a transport at sea this long counts as there (one that stopped short of the point waited till it was sunk)
#define FY_TOGETHER 90000 // BOARD TOGETHER: a push's loaded transport waits this long at most for the wave's others
// ORDERS BY PRIORITY (claims.c; the player 2026-11-19, the transports after the ships, citizens and soldiers): a trip's
// transport and passengers are its at CLP_CARRY from its start to its end, renewed each act. Out of a storm or a danger
// zone (CLP_SURVIVE) comes first - its orders wait meanwhile: the trip gave the unload again each act and sailed back
// into the storm the storm's escape took it out of, 47 and 43 times over each other in 4 test games, to and fro -;
// a passenger another took (healing, under cover, the militia) is let go - the load order every 20 s called it back
// from the Hospital or a lost fight -; counter-fire, the stations, the citizens' safety leave those walking to the
// shore and boarding alone. Off with v_claims without 8. MEASURED 2026-11-19 (head-to-head, Small Islands large, 4 AIs
// from epoch 5, 60 min, seeds 1-12 mirrored, 24 games; B every other claim, v_claims=7): B - A soldiers -1.2 (se 5.7),
// kills -1.6 (11.7), losses +3.4 (9.6), razed -0.8 (0.7); held per AI at the end: islands 1.25 against 1.21, citizens
// off the home island 21.0 against 18.4, miners there 8.1 against 6.0, buildings 9.7 against 8.0 (fewer ferried, 47
// against 54, more of them stayed). On.
#define FY_CLAIM_T 20000
internal int fy_take(unsigned idx, ee_unit *u) { return !(PV(idx, g_v_cl, g_cl_on) & 8) || cl_take(u, CLP_CARRY, CLW_FERRY, FY_CLAIM_T); }
internal int fy_may(unsigned idx, ee_unit *u) { return !(PV(idx, g_v_cl, g_cl_on) & 8) || cl_may(u, CLP_CARRY, CLW_FERRY); }
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
    if (ex_alive(&t->ship, pl)) cl_end(t->ship.u, CLW_FERRY);   // ORDERS BY PRIORITY: the others' again
    for (int k = 0; k < t->npass; k++) if (ex_alive(&t->pass[k], pl)) cl_end(t->pass[k].u, CLW_FERRY);
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
internal int sn_push_pick(ee_player *pl, int k, ivec2 toward, ee_unit **out, int max);   // stations.c ACROSS
internal void squadrons_support(unsigned idx, int k, ivec2 at, int oc, uint32_t now);   // squadrons.c THE SQUADRON AHEAD
internal int squadrons_support_state(unsigned idx, int k);
#define FY_BH_TRIP 2   // THE BRIDGEHEAD (stations.c): a push's citizens a transport of its first wave takes at most
internal int fy_is_cit(const struct ex_unit *e) {   // a passenger that is a citizen (a push's bridgehead)
    ee_unit_type *td = e->u && unit_get_serial(e->u) == e->id ? unit_get_type_data(e->u) : NULL;
    return td && is_citizen_type(unit_type_get_id(td));
}
internal int fy_trip_cits(const struct fy_trip *t) {
    int n = 0;
    for (int k = 0; k < t->npass; k++) n += fy_is_cit(&t->pass[k]);
    return n;
}
internal int fy_push_cits(unsigned idx, int k) {   // push k's citizens with a trip
    int n = 0;
    for (int i = 0; i < FY_TRIPS; i++) { const struct fy_trip *t = &g_fy[idx & 15].t[i]; if (t->phase && t->kind == 3 && t->push == k) n += fy_trip_cits(t); }
    return n;
}
// PROPHETS ACROSS ([ai_base] prophets_across; the player 2026-11-19: "prophet strike groups dont cross water yet right?"
// - a strike at a building across the water walked to the shore and stood there 5 minutes, 2879 strikes for 68 casts in
// six games on Small Islands): a prophet whose way - a strike, home, behind the force - lies on other land asks the
// ferry each look (holy.c hl_go; trip kind 5): one transport for it and its escort (stations.c AN ESCORT: the soldiers
// given it on its land, as many as fit), a crossing that is no important one (it waits for a safe sea), put off at the
// shore nearest where it goes. holy.c leaves it to the ferry meanwhile (fy_carries) and keeps its strike.
#define FY_HX 4
internal struct { ee_unit *u; int id; unsigned from, to; ivec2 at; uint32_t t; } g_fy_hx[16][FY_HX];
internal void fy_holy_ask(unsigned idx, ee_unit *u, unsigned from, unsigned to, ivec2 at, uint32_t now) {
    int id = unit_get_serial(u), k = 0, f = -1;
    while (k < FY_HX && !(g_fy_hx[idx & 15][k].u == u && g_fy_hx[idx & 15][k].id == id)) { if (f < 0 && (!g_fy_hx[idx & 15][k].t || now - g_fy_hx[idx & 15][k].t > 15000)) f = k; k++; }
    if (k == FY_HX) k = f;
    if (k < 0) return;
    g_fy_hx[idx & 15][k].u = u; g_fy_hx[idx & 15][k].id = id; g_fy_hx[idx & 15][k].from = from; g_fy_hx[idx & 15][k].to = to;
    g_fy_hx[idx & 15][k].at = at; g_fy_hx[idx & 15][k].t = now ? now : 1;
}
internal int fy_carries_id(unsigned idx, int id) {   // a trip of the ferry has the unit of this serial (to the pick-up, aboard)
    for (int i = 0; i < FY_TRIPS; i++) {
        const struct fy_trip *t = &g_fy[idx & 15].t[i];
        for (int k = 0; t->phase && k < t->npass; k++) if (t->pass[k].id == id) return 1;
    }
    return 0;
}
internal const char *fy_kind_name(int kind) { return kind == 5 ? "prophets" : kind == 4 ? "scouts" : kind != 2 ? "soldiers" : "citizens"; }
struct fy_want { unsigned a, b; ivec2 from, to; };   // THRIFT: a trip that found no free transport - its lands and points
internal struct fy_want g_fy_miss;   // fy_start's last
internal int fy_start(ee_player *pl, unsigned idx, struct fy_st *f, struct fy_trip *t, int kind, int imp, unsigned a, unsigned b, int n, ivec2 dest, int push, uint32_t now) {
    ee_unit *pass[16];
    if (n > 16) n = 16;
    int np = kind == 1 ? sn_ferry_pick(pl, a, dest, pass, n) : kind == 3 ? sn_push_pick(pl, push, dest, pass, n) : kind >= 4 ? 0 : wk_ferry_pick(pl, a, dest, pass, n);
    if (kind == 4 && push >= 0 && push < SN_SX && g_sn_sx[idx & 15][push].u && !expand_has(pl, g_sn_sx[idx & 15][push].u) && unit_get_hp(g_sn_sx[idx & 15][push].u) > 0) { pass[0] = g_sn_sx[idx & 15][push].u; np = 1; }   // SCOUTS ACROSS
    if (kind == 5 && push >= 0 && push < FY_HX && g_fy_hx[idx & 15][push].u && unit_get_serial(g_fy_hx[idx & 15][push].u) == g_fy_hx[idx & 15][push].id && unit_get_hp(g_fy_hx[idx & 15][push].u) > 0) {   // PROPHETS ACROSS: it and its escort
        pass[0] = g_fy_hx[idx & 15][push].u;
        np = 1 + sn_escort_units(idx, a, pass + 1, n - 1);
        int m = 1;
        for (int k = 1; k < np; k++) if (!fy_carries_id(idx, unit_get_serial(pass[k]))) pass[m++] = pass[k];   // none another prophet's trip has (two took the same escort)
        np = m;
    }
    if (kind == 3 && np > 0) {   // THE BRIDGEHEAD: its first wave's citizens first, FY_BH_TRIP a transport
        int wc = sn_push_cits(idx, push) - fy_push_cits(idx, push);
        ee_unit *cits[FY_BH_TRIP];
        int nc = wc > 0 ? wk_ferry_pick(pl, a, dest, cits, wc < FY_BH_TRIP ? wc : FY_BH_TRIP) : 0;
        if (nc) {
            if (np + nc > 16) np = 16 - nc;
            memmove(pass + nc, pass, (size_t)np * sizeof *pass);
            memcpy(pass, cits, (size_t)nc * sizeof *pass);
            np += nc;
        }
    }
    { int m = 0; for (int k = 0; k < np; k++) if (fy_may(idx, pass[k])) pass[m++] = pass[k]; np = m; }   // ORDERS BY PRIORITY: none another holds (healing, under cover, the militia's)
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
        if (!fy_free_ship(pl, it.u, it.td) || !fy_may(idx, it.u)) continue;   // ... nor one out of a storm
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
        g_fy_miss.a = a; g_fy_miss.b = b; g_fy_miss.from = mid; g_fy_miss.to = dest;
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
                fy_say(idx, now, "the crossing: %d %s from land %u to land %u held - risk %.2f over %.2f%s; the worst: %s", t->npass, fy_kind_name(kind), a, b,
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
    t->push = push; t->push_ter = kind == 3 && push >= 0 && push < SN_PUSHES ? g_sn[idx & 15].push[push].from : kind == 4 && push >= 0 && push < SN_SX ? g_sn_sx[idx & 15][push].ter : -1; t->meet_t = t->ready_t = 0;
    if (kind == 3) {   // TOGETHER: the wave's meeting point off the beach (crossing.c cx_meet) - one for all its transports
        ivec2 w;
        int got = 0;
        for (int i = 0; i < FY_TRIPS && !got; i++) {
            const struct fy_trip *o = &f->t[i];
            if (o != t && o->phase && o->phase < 3 && o->kind == 3 && o->push == push && o->has_via) { w = o->via; got = 1; }
        }
        if (got || cx_meet(pl, idx, best, seas[bs].pick_w, seas[bs].drop_w, (unsigned)seas[bs].oc, &w)) { t->via = w; t->has_via = 1; t->via_done = 0; }
        else t->has_via = 0;
    }
    t->kind = kind; t->from = a; t->to = b; t->ocean = seas[bs].oc;
    if (kind == 3 && push >= 0 && push < SN_PUSHES) { g_sn[idx & 15].push[push].sup_at = t->drop_w; g_sn[idx & 15].push[push].sup_oc = t->ocean; }   // THE SQUADRON AHEAD
    t->pick = seas[bs].pick; t->pick_w = seas[bs].pick_w; t->drop = seas[bs].drop; t->dest = dest;
    t->ship.u = best; t->ship.id = unit_get_serial(best);
    t->phase = 1; t->since = now ? now : 1; t->load_t = 0; t->reissue = 0;
    f->trips++; f->last_kind = kind; t->no = f->trips;
    fy_take(idx, best);   // ORDERS BY PRIORITY: the trip's till it ends
    unit_goal(best, goal_move((float)t->pick_w.x + 0.5f, (float)t->pick_w.y + 0.5f), 0, 0x1F40);
    for (int k = 0; k < t->npass; k++) {   // to the shore meanwhile
        fy_take(idx, t->pass[k].u);
        unit_goal(t->pass[k].u, goal_move((float)t->pick.x + 0.5f, (float)t->pick.y + 0.5f), 0, 0x1F40);
    }
    fy_say(idx, now, "trip %u: %d %s from land %u (by %d,%d) to land %u (wanted at %d,%d) - the transport at %d,%d, boarding at %d,%d, off at %d,%d%s; risk %.2f%s",
           t->no, t->npass, fy_kind_name(kind), a, mid.x, mid.y, b, dest.x, dest.y, unit_get_x(best), unit_get_y(best),
           t->pick.x, t->pick.y, t->drop.x, t->drop.y, hv ? " by a way off the line" : "", (double)risk, imp ? " (important)" : "");
    return 1;
}
internal int fy_push_transit(unsigned idx, int k) {   // stations.c ACROSS: push k's soldiers with a trip (its citizens not)
    int n = 0;
    for (int i = 0; i < FY_TRIPS; i++) { const struct fy_trip *t = &g_fy[idx & 15].t[i]; if (t->phase && t->kind == 3 && t->push == k) n += t->npass - fy_trip_cits(t); }
    return n;
}
internal int fy_push_trips(unsigned idx, int k) {
    int n = 0;
    for (int i = 0; i < FY_TRIPS; i++) n += g_fy[idx & 15].t[i].phase && g_fy[idx & 15].t[i].kind == 3 && g_fy[idx & 15].t[i].push == k;
    return n;
}
internal int fl_sea_docks(ee_unit_mgr *um, unsigned ocean, unsigned *area, unsigned *seas, int *nseas);   // fleet.c THE DOCKS OF A SEA
internal int fy_coming(const struct fy_st *f, uint32_t now) {   // transports ordered within FY_ORDER: still to come
    int n = 0;
    for (int k = 0; k < 4; k++) n += f->order_t[k] && now >= f->order_t[k] && now - f->order_t[k] < FY_ORDER;
    return n;
}
// A WAVE'S ORDERS ([ai_base] push_transport_orders; the player 2026-11-19, of his P2's 28 transports: "it looks like it
// just never stops producing them"): a push gathering room for its wave had a transport trained every look it lacked
// room, up to 4 at a time - P4 (Small Islands, 190 min) ordered 7 in 20 s for a wave of 101 that waited 184 s, the
// transports idle after it. With a limit: no more than that many trained for one wave; once they are there (none still
// to come) the wave sets out with the room it has, the rest of the push in later waves. g_fy_pord: trained for push k's
// wave so far
internal int g_fy_pord[16][SN_PUSHES];
internal void fy_order(ee_player *pl, unsigned idx, struct fy_st *f, int wanted, const struct fy_want *wt, int nwt, uint32_t now) {   // MORE TRANSPORTS
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
    unsigned sea = 0;
    if (FERRY_THRIFT(idx)) {   // THRIFT: at a dock on a sea both lands of a wanted trip touch - none there, none ordered
        unsigned seas[8], area = 0;
        int ns = 0;
        fl_sea_docks(um, 0, NULL, seas, &ns);
        ivec2 o, w;
        for (int i = 0; i < nwt && !sea; i++)
            for (int k = 0; k < ns && !sea; k++)
                if (ex_shore(wt[i].a, (int)seas[k], wt[i].from, FY_SHORE, &o, &w) && ex_shore(wt[i].b, (int)seas[k], wt[i].to, FY_SHORE, &o, &w) &&
                    fl_sea_docks(um, seas[k], &area, NULL, NULL) && area) { sea = seas[k]; req[3] = (int)area; }
        if (!sea) { f->no_dock++; return; }
    }
    unsigned got = 0;
    g_priority_mil = 1; g_landing_order = 1; g_expand_econ = 1;   // paid as the expansion's (expand.c ex_ferry_ship)
    hook_train_units(um, req, (uintptr_t)&got);
    g_priority_mil = 0; g_landing_order = 0; g_expand_econ = 0;
    if (!got) return;
    f->order_t[slot] = now ? now : 1; f->trained++;
    fy_say(idx, now, "trains a transport (type %d): %d trips found none free, %d ordered before still to come%s", tt, wanted, coming, sea ? " - at a dock on the sea of the trip" : "");
}
// HOME TO A DOCK ([ai_base] transport_park; the player 2026-11-19: "send transports to the nearest habour once they
// dropped off, that should save quite a lot of ships for the ai"): a transport put off its passengers stayed where it
// put them off - off an outpost's beach, a push's, the land a landing took - until the next trip or the enemy found it.
// Every FY_LOOK a transport of its with no order, empty and nobody's (fy_free_ship: no landing's, no outpost ferry's,
// no rescue's, no trip's) goes to the water by its nearest finished dock on its sea that is out of danger (no enemy
// tower it has seen in reach, no territory being lost), FY_PARK tiles off it at most; it stays free for the next trip.
#define FY_PARK 8
// THRIFT ([ai_base] ferry_thrift; the player 2026-11-19, his P2 at 187 minutes with 28 transports, 14 of them idle at
// one dock: "does p2 in my game really need all these transports? it looks like it just never stops producing them"):
// it had trained 65 - 18 of them from minute 120 to 154 while 7 to 13 stood free, for trips that found the free ones on
// a sea not between the two lands (a new one comes from a dock the game picks, as likely on that same sea) - and 1463
// citizens were put off in 658 trips, a land with 18 free places at its own mines sending its citizens to another
// with 6 while a third sent 9 to it, one or two a transport. With it: a transport is ordered only for a trip that found
// none free on a sea both its lands touch, and trained at a dock on such a sea (fleet.c fl_sea_docks: the game hands an
// order to the producers of one region, the biggest group unless it is named) - with no dock of its on one, none (the
// seas may not join: one free elsewhere is no help, and one from the biggest group of docks sailed there); a land's
// citizens fill its own free places first (its spare less them); a citizens' trip takes FY_CIT_MIN at least, fewer only
// when the land they go to wants no more. And the parked transports SPREAD over a sea's docks (the player: "idle
// transports should maybe spread betwen habours ti increase availabilithy"): a sea's free ones in even shares, a dock
// over its share sends one to the nearest under it.
#define FY_CIT_MIN 4
internal void fy_park(ee_player *pl, unsigned idx, struct fy_st *f) {
    void *cm = player_get_civ_mgr(pl);
    ee_unit_mgr *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    uint8_t *dq = um && ut_has(UT(B_DOCK)) ? unit_mgr_type_list(um, UT(B_DOCK)) : NULL;
    struct { ivec2 w; unsigned oc; int n; } dk[32];   // n: free ones parked there
    int nd = 0;
    for (unsigned k = 0, first = dq ? deque_front(dq) : 0, cnt = dq ? deque_count(dq) : 0; k < cnt && nd < 32; k++) {
        ee_unit *d = deque_at(dq, first + k), *st = d ? unit_get_physics(d) : NULL;
        if (!st || unit_get_hp(d) <= 0 || !unit_get_finished(st) || unit_get_x(d) < 0) continue;
        ivec2 p = unit_get_tile(d), w;
        if (tv_unsafe(idx, p.x, p.y) || wk_danger(idx, p)) continue;
        unsigned oc = ocean_near(p, 5, &w);
        if (oc) { dk[nd].w = w; dk[nd].n = 0; dk[nd++].oc = oc; }
    }
    if (!nd) return;
    // its free transports with no order: the dock they lie by (-1 none) - and those on their way to one, counted there and
    // left be (SPREAD: uncounted, the shares came out too small and ships went to and fro, 728 sent a game against 53)
    static struct { ee_unit *u; ivec2 at; unsigned oc; int dock, going; } sh[64];
    int nsh = 0, spread = FERRY_THRIFT(idx);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it) && nsh < 64; ) {
        if (!fy_free_ship(pl, it.u, it.td)) continue;
        ee_goal *g = unit_get_goal(it.u);
        ivec2 at = unit_get_tile(it.u);
        sh[nsh].u = it.u; sh[nsh].at = at; sh[nsh].oc = ocean_near(at, 2, NULL); sh[nsh].dock = -1; sh[nsh].going = g != NULL;
        int bd = 0;   // the nearest dock (two within FY_PARK: counted at the first, one sent to the second went again)
        for (int k = 0; k < nd; k++) {
            if (dk[k].oc != sh[nsh].oc) continue;
            if (g) {
                if (goal_is_location(g) && fabsf(goal_get_dest_x(g) - ((float)dk[k].w.x + 0.5f)) < 2 && fabsf(goal_get_dest_y(g) - ((float)dk[k].w.y + 0.5f)) < 2) { sh[nsh].dock = k; break; }
                continue;
            }
            int d = ivec2_dist2(at, dk[k].w);
            if (d <= FY_PARK * FY_PARK && (sh[nsh].dock < 0 || d < bd)) { sh[nsh].dock = k; bd = d; }
        }
        if (sh[nsh].dock >= 0) dk[sh[nsh].dock].n++;
        if (!g || sh[nsh].dock >= 0) nsh++;   // one on another errand isn't the parking's
    }
    for (int i = 0; i < nsh; i++) {
        if (sh[i].going) continue;
        int b = -1, bd = 0, docks = 0, ships = 0;
        for (int k = 0; k < nd; k++) if (dk[k].oc == sh[i].oc) { docks++; ships += dk[k].n; }
        for (int j = 0; j < nsh; j++) ships += sh[j].dock < 0 && sh[j].oc == sh[i].oc;
        int share = docks ? (ships + docks - 1) / docks : 0;   // SPREAD: even shares, rounded up
        if (sh[i].dock >= 0 && (!spread || dk[sh[i].dock].n <= share)) continue;   // parked, and not one too many there
        for (int k = 0; k < nd; k++) {
            if (dk[k].oc != sh[i].oc || k == sh[i].dock || (spread && sh[i].dock >= 0 && dk[k].n + 1 > share)) continue;
            int d = ivec2_dist2(sh[i].at, dk[k].w) + (spread ? dk[k].n * 400 : 0);   // SPREAD: a ship there counts as 20 tiles further
            if (b < 0 || d < bd) { b = k; bd = d; }
        }
        if (b < 0) continue;
        if (sh[i].dock >= 0) dk[sh[i].dock].n--;
        dk[b].n++; sh[i].dock = b;
        unit_goal(sh[i].u, goal_move((float)dk[b].w.x + 0.5f, (float)dk[b].w.y + 0.5f), 0, 0x1F40);
        f->parked++;
    }
}
internal void fy_look(ee_player *pl, unsigned idx, struct fy_st *f, uint32_t now) {
    typeof(&g_snl[0]) S = &g_snl[idx];
    typeof(&g_wkl[0]) W = &g_wkl[idx];
    int sfresh = S->t && now >= S->t && now - S->t <= FY_FRESH, wfresh = W->t && now >= W->t && now - W->t <= FY_FRESH;
    // the lands' gaps, those on their way counted for the land they go to
    // cwood, cidle: work.c TO THE TREES ACROSS - the places at the trees and bushes (idle citizens only), the idle
    int sgap[SN_LANDS], cfree[WK_LANDS], cspare[WK_LANDS], cwood[WK_LANDS], cidle[WK_LANDS];
    for (int k = 0; k < S->n; k++) sgap[k] = S->l[k].gap;
    for (int k = 0; k < W->n; k++) { cfree[k] = W->l[k].free; cspare[k] = W->l[k].spare; cwood[k] = W->l[k].wood; cidle[k] = W->l[k].idle; }
    for (int i = 0; i < FY_TRIPS; i++) {
        struct fy_trip *t = &f->t[i];
        if (!t->phase) continue;
        if (t->kind == 1) { for (int k = 0; k < S->n; k++) if (S->l[k].land == t->to) sgap[k] -= t->npass; }
        else for (int k = 0; k < W->n; k++) if (W->l[k].land == t->to) {
            int m = t->npass < cfree[k] ? t->npass : cfree[k] > 0 ? cfree[k] : 0;
            cfree[k] -= t->npass; cwood[k] -= t->npass - m;
        }
    }
    drops_cached(pl);
    int blocked[3] = { 0, !sfresh, !wfresh }, unserved = 0, held = 0, nwt = 0, pw[SN_PUSHES] = { 0 };   // pw: pushes that asked for transports this look
    struct fy_want wt[8];   // THRIFT: the trips that found no free transport
    #define FY_MISSED() do { unserved++; if (nwt < 8) wt[nwt++] = g_fy_miss; } while (0)
    // ACROSS (stations.c push_across): a push's soldiers first - every free transport at once, important ones (the
    // transports may be lost once they are over); those taken leave the push's count of soldiers still to cross
    // ONE WAVE AT A TIME (the player 2026-11-18, of the pushes across against landing.c's landings: "unifing the systems
    // dosnt mean throwing out what worked"): a push's soldiers set out as one wave, as a landing's - none while a wave of
    // it is on its way, its free transports taking three quarters of it first (GATHERED FIRST: "landings into hostile
    // situations should be coordinated to arrive in parallel"), a later wave half a transport at least. Without: every
    // soldier trained into the push's territory set out on a transport of its own - 504 trips for 53 pushes, 196 of them
    // with one soldier, the first to the last 5.5 minutes apart, the first put off after 5.3; a landing's 87% ashore at once
    for (int k = 0; k < SN_PUSHES && PUSH_ACROSS(idx); k++) {
        struct sn_push *p = &g_sn[idx].push[k];
        int left0 = p->on && p->across ? p->left - fy_push_transit(idx, k) : 0;
        if (left0 <= 0 || fy_push_transit(idx, k)) { p->wave_t = 0; g_fy_pord[idx & 15][k] = 0; continue; }
        int room = 0, tt = ld_transport_type(pl), cap = tt ? ld_cap(tt) : 10;
        if (p->waves && left0 < (cap + 1) / 2) { p->wave_t = 0; g_fy_pord[idx & 15][k] = 0; continue; }
        if (!p->wave_t) p->wave_t = now ? now : 1;
        if (!p->waves && !p->cits_want) {   // THE BRIDGEHEAD: 2 citizens a transport of the first wave, SN_CITS at most
            int cw = 2 * ((left0 + cap - 1) / (cap > 0 ? cap : 10));
            p->cits_want = cw < SN_CITS ? cw : SN_CITS;
        }
        // IN PLACES: a rider or a gun takes more than one of a transport's (tw_size) - counted by soldiers, the first waves
        // with room for all of them took 18.8 of 31.1 on average, and THE LAST WORD called off 36 of 51
        static ee_unit *lp[128];
        int nl = sn_push_pick(pl, k, p->beach, lp, 128), places = 0;
        for (int j = 0; j < nl; j++) places += tw_size(lp[j]);
        int need = (places > left0 ? places : left0) + (p->waves ? 0 : p->cits_want - p->ncit);
        for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) if (fy_free_ship(pl, it.u, it.td)) room += ld_cap(unit_type_get_id(it.td));
        // THE WHOLE PUSH FIRST: its first wave waits for room for all of it (a later one for three quarters) - the push
        // was judged with all its soldiers, its losses planned for; with three quarters the first waves took 19.6 of 32.9
        // soldiers on average, and THE LAST WORD found 42 of 51 too weak and called them off
        int share = p->waves ? 3 : 4;
        // THE TRANSPORTS HELD: while a push gathers them nobody else takes a free one - the soldiers' and citizens' trips
        // took each new one as it came (one AI trained 30, its pushes waited 3 minutes and sailed with one: 9 of 57
        // soldiers, called off by THE LAST WORD; 23 of 54 first waves so)
        int lim = PUSH_ORDERS(idx), capped = lim > 0 && g_fy_pord[idx & 15][k] >= lim;   // A WAVE'S ORDERS
        if (room * 4 < need * share && now - p->wave_t < FY_GATHER && !(capped && !fy_coming(f, now))) {
            int want = (need * share / 4 - room + cap - 1) / (cap > 0 ? cap : 10);
            if (!capped) { unserved += want > 0 ? want : 1; pw[k] = 1; }
            held = 1;
            if (nwt < 8 && p->from >= 0 && p->from < g_ter.n) { wt[nwt].a = p->from_land; wt[nwt].b = p->to_land; wt[nwt].from = g_ter.t[p->from].c; wt[nwt++].to = p->beach; }
            continue;
        }
        int ships = 0, set = 0;
        for (int left = left0; left > 0; ) {
            struct fy_trip *t = NULL;
            for (int i = 0; i < FY_TRIPS && !t; i++) if (!f->t[i].phase) t = &f->t[i];
            if (!t) break;
            int r = fy_start(pl, idx, f, t, 3, 1, p->from_land, p->to_land, left, p->beach, k, now);
            if (r == -1) FY_MISSED();
            if (r <= 0) break;
            left -= t->npass - fy_trip_cits(t); set += t->npass - fy_trip_cits(t); ships++;
        }
        if (ships) {
            p->waves++;
            fy_say(idx, now, "a push's wave %d: %d of %d soldiers set out on %d transports (room %d, waited %.0f s for transports, %d trained for it)", p->waves, set, left0, ships, room,
                   (double)(now - p->wave_t) / 1000.0, g_fy_pord[idx & 15][k]);
            p->wave_t = 0; g_fy_pord[idx & 15][k] = 0;
        }
    }
    for (int tries = 0; tries < FY_TRIPS && !held; tries++) {
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
        // (TO THE TREES ACROSS: the trees' and bushes' places count from a land with idle ones only)
        int bc = -1, ac = -1, cn = 0;
        #define FY_CPLACES(k) ((cfree[k] > 0 ? cfree[k] : 0) + (cwood[k] > 0 ? cwood[k] : 0))
        for (int k = 0; !blocked[2] && k < W->n; k++) if (FY_CPLACES(k) >= 1 && (rs_drop_land(idx, W->l[k].land) || bd_waits(idx, W->l[k].land, NULL)) && (bc < 0 || FY_CPLACES(k) > FY_CPLACES(bc))) bc = k;   // ACROSS THE WATER: a foundation waiting for builders there (build.c)
        int thrift = FERRY_THRIFT(idx);
        for (int k = 0; bc >= 0 && k < W->n; k++) {
            int sp = cspare[k] - (thrift && cfree[k] > 0 ? cfree[k] : 0), si = thrift && cidle[k] > sp ? sp : cidle[k];   // THRIFT: its own free places first
            if (k == bc || sp < 1) continue;
            int mines = cfree[bc] > 0 ? (cfree[bc] < sp ? cfree[bc] : sp) : 0, idl = si - mines, wood = cwood[bc] > 0 && idl > 0 ? (cwood[bc] < idl ? cwood[bc] : idl) : 0;
            if (mines + wood > cn || (ac >= 0 && mines + wood == cn && cspare[k] > cspare[ac])) { cn = mines + wood; ac = k; }   // as many as it can, then the most to spare
        }
        if (thrift && ac >= 0 && cn < FY_CIT_MIN && cn < FY_CPLACES(bc)) { ac = -1; f->small++; }   // THRIFT: a transport for a few only where no more are wanted
        #undef FY_CPLACES
        int can[3] = { 0, bs >= 0, bc >= 0 && ac >= 0 };
        if (!can[1] && !can[2]) break;
        int first = can[2] && (!can[1] || f->last_kind == 1) ? 2 : 1;   // TURNS
        for (int j = 0; j < 2; j++) {
            int kind = j ? 3 - first : first;
            if (!can[kind]) continue;
            int r = kind == 1 ? fy_start(pl, idx, f, t, 1, S->l[bs].hot, S->l[as].land, S->l[bs].land, bn, S->l[bs].at, -1, now)
                              : fy_start(pl, idx, f, t, 2, 0, W->l[ac].land, W->l[bc].land, cn, W->l[bc].at, -1, now);
            if (r > 0) {
                if (kind == 1) { sgap[bs] -= t->npass; sgap[as] += t->npass; }
                else {
                    int m = cfree[bc] > 0 ? (t->npass < cfree[bc] ? t->npass : cfree[bc]) : 0;
                    cfree[bc] -= m; cwood[bc] -= t->npass - m; cspare[ac] -= t->npass; cidle[ac] -= t->npass < cidle[ac] ? t->npass : cidle[ac];
                }
                break;
            }
            blocked[kind] = 1;
            if (r == -1) FY_MISSED();
        }
    }
    for (int k = 0; k < SN_SX && EXPAND_ACROSS(idx) && !held; k++) {   // SCOUTS ACROSS (stations.c): one transport a scout
        if (!g_sn_sx[idx][k].t || now - g_sn_sx[idx][k].t > 10000 || !g_sn_sx[idx][k].u || expand_has(pl, g_sn_sx[idx][k].u)) continue;
        struct fy_trip *t = NULL;
        for (int i = 0; i < FY_TRIPS && !t; i++) if (!f->t[i].phase) t = &f->t[i];
        if (!t) break;
        if (fy_start(pl, idx, f, t, 4, 0, g_sn_sx[idx][k].from, g_sn_sx[idx][k].to, 1, g_sn_sx[idx][k].at, k, now) == -1) FY_MISSED();
    }
    for (int k = 0; k < FY_HX && PROPHETS_ACROSS(idx) && !held; k++) {   // PROPHETS ACROSS (holy.c): one transport a prophet and its escort
        ee_unit *u = g_fy_hx[idx][k].u;
        if (!g_fy_hx[idx][k].t || now - g_fy_hx[idx][k].t > 10000 || !u || unit_get_serial(u) != g_fy_hx[idx][k].id || unit_get_hp(u) <= 0) continue;
        if (fy_carries_id(idx, g_fy_hx[idx][k].id) || unit_get_x(u) < 0 || bk_land_of(unit_get_tile(u)) != g_fy_hx[idx][k].from) continue;   // taken, or no longer where it asked from
        struct fy_trip *t = NULL;
        for (int i = 0; i < FY_TRIPS && !t; i++) if (!f->t[i].phase) t = &f->t[i];
        if (!t) break;
        int r = fy_start(pl, idx, f, t, 5, 0, g_fy_hx[idx][k].from, g_fy_hx[idx][k].to, 16, g_fy_hx[idx][k].at, k, now);
        if (r == -1) FY_MISSED();
        if (r > 0) g_fy_hx[idx][k].t = 0;
    }
    #undef FY_MISSED
    if (unserved) {
        unsigned tr0 = f->trained;
        f->want_t = now ? now : 1; fy_order(pl, idx, f, unserved, wt, nwt, now);
        for (int k = 0; k < SN_PUSHES && f->trained > tr0; k++) g_fy_pord[idx & 15][k] += pw[k];   // A WAVE'S ORDERS: counted for the pushes that asked
    }
    else f->want_t = 0;
}
internal void fy_minute(ee_player *pl, unsigned idx, struct fy_st *f, uint32_t now) {
    if (!g_log || (f->log_t && now >= f->log_t && now - f->log_t < 60000)) return;
    f->log_t = now ? now : 1;
    char b[300] = "";
    typeof(&g_snl[0]) S = &g_snl[idx];
    typeof(&g_wkl[0]) W = &g_wkl[idx];
    for (int k = 0; k < S->n; k++) { size_t l = strlen(b); if (l < sizeof b - 40) snprintf(b + l, sizeof b - l, " %u:%d/%d", S->l[k].land, S->l[k].have, S->l[k].want); }
    size_t l = strlen(b);
    snprintf(b + l, sizeof b - l, " | citizens land:spare/free places");
    for (int k = 0; k < W->n; k++) {   // +: TO THE TREES ACROSS, the trees' and bushes' places
        l = strlen(b);
        if (l < sizeof b - 40) snprintf(b + l, sizeof b - l, " %u:%d/%d", W->l[k].land, W->l[k].spare, W->l[k].free);
        l = strlen(b);
        if (W->l[k].wood > 0 && l < sizeof b - 40) snprintf(b + l, sizeof b - l, "+%d", W->l[k].wood);
    }
    int on[4] = { 0 }, ships = 0, free_ships = 0;
    for (int i = 0; i < FY_TRIPS; i++) on[f->t[i].phase & 3]++;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {   // its transports now (HOME TO A DOCK's measure: those kept)
        if (unit_get_owner(it.u) != pl || unit_get_hp(it.u) <= 0 || unit_get_x(it.u) < 0 || !is_transport(unit_type_get_id(it.td))) continue;
        ships++; free_ships += fy_free_ship(pl, it.u, it.td);
    }
    if (g_log) log_locked("# ferry: P%u at %.1f min: transports %d (%d free)\n", idx, now / 60000.0, ships, free_ships);
    fy_say(idx, now, "%u trips (%u soldiers, %u citizens put off; %u given up, %u transports sunk, %u put off on a beach under fire), %u transports trained, looks without a free transport %u, with none between the lands %u; on now: %d to the pick-up, %d boarding, %d crossing; %u sent home to a dock, %u citizens' trips not started for too few, %u transports not ordered for want of a dock on the sea | soldiers land:stationed/share%s",
           f->trips, f->soldiers, f->citizens, f->failed, f->sunk, f->beached, f->trained, f->no_ship, f->no_shore, on[1], on[2], on[3], f->parked, f->small, f->no_dock, b);
    if (CROSSING_SAFETY(idx)) {
        struct cx_st *c = &g_cx_st[idx & 15];
        fy_say(idx, now, "the crossing: %u ways rated (%u off the line), trips held for the risk %u, for a load too small for it %u, important ones that went into danger %u; threats known %d",
               c->rated, c->detours, c->held, c->held_load, c->gone_high, c->n);
    }
}
// THE LAST WORD (stations.c sn_last_word): push k's first wave aboard - its soldiers against the beach and the target,
// their losses planned for; called off: every transport of it unloads where it boarded. 1 go, 0 wait, -1 called off
internal int fy_last_word(ee_player *pl, unsigned idx, struct fy_st *f, struct fy_trip *t, uint32_t now) {
    static ee_unit *u[FY_TRIPS * 16];
    int n = 0;
    for (int i = 0; i < FY_TRIPS; i++) {
        struct fy_trip *o = &f->t[i];
        if (o->phase != 2 || o->kind != 3 || o->push != t->push) continue;
        for (int k = 0; k < o->npass; k++) if (unit_get_x(o->pass[k].u) < 0 && !fy_is_cit(&o->pass[k])) u[n++] = o->pass[k].u;
    }
    int r = sn_last_word(pl, idx, t->push, u, n, now), k = t->push;
    if (r >= 0) return r;
    for (int i = 0; i < FY_TRIPS; i++) {
        struct fy_trip *o = &f->t[i];
        if (!o->phase || o->phase == 3 || o->kind != 3 || o->push != k) continue;
        if (ex_alive(&o->ship, pl) && ex_cargo(o->ship.u) > 0) unit_goal(o->ship.u, ld_unload(o->ship.u, o->pick), 0, 0x1F40);
        fy_say(idx, now, "trip %u (risk %.2f): called off - too strong a beach, unloaded at home", o->no, (double)o->risk);
        o->phase = 0; o->npass = 0; o->ship.u = NULL;
    }
    sn_push_off(idx, k, "too strong a beach - called off, unloaded at home", now);
    return -1;
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
    for (int k = 0; k < t->npass; k++)   // ... and still its (ORDERS BY PRIORITY: one another took is let go)
        if (ex_alive(&t->pass[k], pl) && (unit_get_x(t->pass[k].u) < 0 || fy_take(idx, t->pass[k].u))) t->pass[n++] = t->pass[k];
    t->npass = n;
    for (int k = 0; k < t->npass; k++) aboard += unit_get_x(t->pass[k].u) < 0;
    if (!fy_take(idx, sh)) return;   // out of a storm or a danger zone first: the trip's orders wait
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
                    ivec2 rv = t->via;
                    int rh = t->has_via;
                    t->risk = crossing_rate(pl, idx, sh, t->pick_w, t->drop_w, (unsigned)t->ocean, &rv, &rh);
                    if (t->kind != 3) { t->via = rv; t->has_via = rh; }   // a push's way is its meeting point (TOGETHER)
                    if (t->risk > fy_tolerance(f, t->kind, t->imp, now)) { if (!f->risky_t[t->kind]) f->risky_t[t->kind] = now ? now : 1; t->load_t = 0; }
                    else t->load_t = 1;
                }
                if (!t->load_t) { convoy_escort(idx, sh, t->pick_w, t->drop, 0, now); return; }
            }
            t->phase = 2; t->load_t = now ? now : 1;
        }
        convoy_escort(idx, sh, t->pick_w, t->drop, 0, now);   // ESCORT (convoy.c): asked for from the start - the scouts sweep the way while it comes and boards
        int ready = aboard == t->npass || (aboard && now - t->load_t >= FY_LOAD);
        if (ready && t->kind == 3) {   // BOARD TOGETHER (as a landing's): a push's wave sails when all of it is aboard, FY_TOGETHER at most
            if (!t->ready_t) t->ready_t = now ? now : 1;
            int wait = 0;
            uint32_t first = t->ready_t;
            for (int i = 0; i < FY_TRIPS; i++) {
                const struct fy_trip *o = &f->t[i];
                if (o == t || !o->phase || o->phase == 3 || o->kind != 3 || o->push != t->push) continue;
                if (!o->ready_t) wait = 1; else if (o->ready_t < first) first = o->ready_t;
            }
            if (wait && now - first < FY_TOGETHER) ready = 0;
            if (ready) {   // THE LAST WORD
                int lw = fy_last_word(pl, idx, f, t, now);
                if (lw < 0) return;
                if (!lw) ready = 0;
            }
        }
        if (ready) {
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
        int off = 0, offc = 0;
        for (int k = 0; k < t->npass; k++) if (unit_get_x(t->pass[k].u) >= 0) { off++; offc += t->kind == 3 && fy_is_cit(&t->pass[k]); }
        if (t->kind != 2) { f->soldiers += (unsigned)(off - offc); f->citizens += (unsigned)offc; } else f->citizens += (unsigned)off;
        fy_say(idx, now, "trip %u (risk %.2f): %d %s put off on land %u at %d,%d%s", t->no, (double)t->risk, off, fy_kind_name(t->kind), t->to, t->drop.x, t->drop.y,
               offc ? " - citizens for the bridgehead among them" : "");
        if (t->kind != 2)   // on to where they are wanted; the stations take them there - a push's back to its push (stations.c ACROSS)
            for (int k = 0; k < t->npass; k++) {
                if (t->kind == 3 && fy_is_cit(&t->pass[k])) { sn_bh_land(idx, t->push, t->pass[k].u); continue; }   // THE BRIDGEHEAD
                unit_goal(t->pass[k].u, goal_move((float)t->dest.x + 0.5f, (float)t->dest.y + 0.5f), 0, 0x1F40);
                if ((t->kind == 3 || t->kind == 4) && t->push_ter >= 0) sn_ferried(idx, t->pass[k].u, t->push_ter, t->kind == 4);
            }
        fy_end(pl, idx, t, now, NULL);
        return;
    }
    if (now - t->since >= FY_SEA) { f->failed++; fy_end(pl, idx, t, now, "not across in 6 minutes - put off where it is"); return; }
    convoy_escort(idx, sh, t->pick_w, t->drop, 1, now);   // ESCORT (convoy.c)
    if (t->kind == 3 && t->has_via && !t->via_done && (ivec2_dist2(unit_get_tile(sh), t->via) <= 36 || now - t->since >= FY_MEET_SAIL)) {   // TOGETHER: the push's transports meet off the beach, then run in at once
        if (!t->meet_t) t->meet_t = now ? now : 1;
        int all = 1;
        uint32_t first = t->meet_t;
        for (int i = 0; i < FY_TRIPS; i++) {   // those at sea: one still at home is no reason to wait off the enemy's beach
            struct fy_trip *o = &f->t[i];
            if (o->phase != 3 || o->kind != 3 || o->push != t->push) continue;
            if (!o->meet_t) all = 0; else if (o->meet_t < first) first = o->meet_t;
        }
        // THE SQUADRON AHEAD (squadrons.c) on its way to the water off the beach: the wave waits for it here too
        if ((all && squadrons_support_state(idx, t->push) != 1) || now - first >= FY_MEET)
            for (int i = 0; i < FY_TRIPS; i++) if (f->t[i].phase == 3 && f->t[i].kind == 3 && f->t[i].push == t->push && f->t[i].meet_t) f->t[i].via_done = 1;
        if (!t->via_done) return;
    }
    if (t->has_via && !t->via_done) {   // THE CROSSING: the way off the line - its point first
        if (t->kind != 3 && ivec2_dist2(unit_get_tile(sh), t->via) <= 16) t->via_done = 1;
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
    fy_minute(pl, idx, f, now);
    for (int k = 0; k < SN_PUSHES; k++) {   // THE SQUADRON AHEAD (squadrons.c): from a push's first transport on until it is over
        struct sn_push *p = &g_sn[idx].push[k];
        if (p->on && p->across && p->sup_oc && (p->over || fy_push_trips(idx, k))) squadrons_support(idx, k, p->sup_at, p->sup_oc, now);
    }
    for (int i = 0; i < FY_TRIPS; i++) if (f->t[i].phase) fy_trip_act(pl, idx, f, &f->t[i], now);
    if (f->look_t && now >= f->look_t && now - f->look_t < FY_LOOK) return;
    f->look_t = now ? now : 1;
    fy_look(pl, idx, f, now);
    if (TRANSPORT_PARK(idx)) fy_park(pl, idx, f);   // after the trips took theirs
}
