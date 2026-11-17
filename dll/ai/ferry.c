// THE FERRY SERVICE ([ai_base] ferry_service; the player 2026-11-17, on citizens and soldiers kept from the other lands:
// "citezen balacning acroos territories dosnt work across water yet right? / or general reinforcment" - and yes to one
// service both use). The force's shares (stations.c) and the citizens' work (work.c) walk the land: sn_give over the
// territory graph without its seas, a citizen to sites on its own land only. So a territory across the water kept short
// of its share however many soldiers stood spare at home, and free gold across the water stayed free however many cut
// wood at home; only the outposts' ferry (expand.c, a fixed number of trips) and the landings crossed.
// Every FY_LOOK, with no trip on: by land (any two lands of its - islands, a continent and an island, a custom map's
// mix alike), soldiers from the land most over its territories' shares to the one most short (FY_MIN each at least),
// else citizens - idle ones first, then wood cutters - from a land with FY_MIN_CIT of them to one with as many free
// places at the mines of resources it wants more citizens on and a drop site of its there. One of its transports on
// a sea both lands touch (no landing's, no outpost ferry's, no rescue's; empty) takes them: the passengers walk to the
// shore nearest their middle meanwhile (the stations and work.c leave a passenger alone: expand_has - not the game's
// hold, which a save keeps while the trip is forgotten), board, and are put off at
// the shore nearest where they are wanted; there they are the others' again - the stations count a soldier where it
// stands, work.c gives a citizen the territory it stands in. No free transport: one is trained (the expansion's way
// of paying - the priority list's row holds its price meanwhile, expand_transport_hold), FY_TRAIN apart at most.
// NOT BACK AGAIN: a land is short or has spare only by a quarter of its share too (FY_PART), and no trip of a kind goes
// back the way the last one of that kind came within FY_BACK - the shares move by a few with every soldier trained or
// lost, and those on their way count for neither land: 148 soldiers ferried in 75 minutes between two lands that each
// stood within 3 of their share, 2 -> 9 and 9 -> 2 by turns (Small Islands seed 4).
// "# ferry:" lines; each minute what the lands want and offer.
#define FY_LOOK 5000
#define FY_PART 4         // a land's gap a quarter of its share at least
#define FY_BACK 600000    // no trip back the way the last of its kind came within this
#define FY_MIN 4          // soldiers a land is short of and another has spare, at least, for a trip
#define FY_MIN_CIT 3      // free mine places and spare citizens, at least
#define FY_SHORE 60       // shores searched within this many tiles
#define FY_BOARD 240000   // no one aboard this long: given up
#define FY_LOAD 90000     // some aboard this long: off with those
#define FY_SEA 360000     // not unloaded this long: put off where it is
#define FY_TRAIN 120000   // a transport ordered at most this often
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
internal void fy_end(ee_player *pl, unsigned idx, struct fy_st *f, uint32_t now, const char *why) {
    if (f->phase >= 2 && ex_alive(&f->ship, pl) && ex_cargo(f->ship.u) > 0) {   // someone aboard: off at the nearest land with room
        ivec2 at = unit_get_tile(f->ship.u), t;
        if (ex_shore(f->to, f->ocean, at, 40, &t, NULL) || ex_shore(f->from, f->ocean, at, 40, &t, NULL))
            unit_goal(f->ship.u, ld_unload(f->ship.u, t), 0, 0x1F40);
    }
    if (why) fy_say(idx, now, "%s", why);
    f->phase = 0; f->npass = 0; f->ship.u = NULL;
}
// a free transport: its own, a ship, on the map, empty, nobody else's
internal int fy_free_ship(ee_player *pl, ee_unit *u, ee_unit_type *td) {
    return unit_get_owner(u) == pl && unit_get_hp(u) > 0 && unit_get_x(u) >= 0 && is_transport(unit_type_get_id(td)) &&
           ex_cargo(u) <= 0 && !landing_has_unit(pl, u) && !expand_ship(u);
}
internal int sn_ferry_pick(ee_player *pl, unsigned land, ivec2 toward, ee_unit **out, int max);    // stations.c
internal int wk_ferry_pick(ee_player *pl, unsigned land, ivec2 toward, ee_unit **out, int max);    // work.c
// a trip of kind (1 soldiers, 2 citizens) of n from land a to land b, wanted at dest: 1 when it set out
internal int fy_start(ee_player *pl, unsigned idx, struct fy_st *f, int kind, unsigned a, unsigned b, int n, ivec2 dest, uint32_t now) {
    ee_unit *pass[16];
    if (n > 16) n = 16;
    int np = kind == 1 ? sn_ferry_pick(pl, a, dest, pass, n) : wk_ferry_pick(pl, a, dest, pass, n);
    if (np < (kind == 1 ? FY_MIN : FY_MIN_CIT)) return 0;
    long sx = 0, sy = 0;
    for (int k = 0; k < np; k++) { sx += unit_get_x(pass[k]); sy += unit_get_y(pass[k]); }
    ivec2 mid = iv2((int)(sx / np), (int)(sy / np));
    // the transport: the nearest free one on a sea both lands touch (each sea's shores looked for once)
    struct { int oc, ok; ivec2 pick, pick_w, drop; } seas[4];
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
            seas[s].ok = ex_shore(a, oc, mid, FY_SHORE, &seas[s].pick, &seas[s].pick_w) && ex_shore(b, oc, dest, FY_SHORE, &seas[s].drop, NULL);
            ns++;
        }
        if (!seas[s].ok) continue;
        int d = ivec2_dist2(unit_get_tile(it.u), seas[s].pick_w);
        if (d < bd) { bd = d; best = it.u; bs = s; }
    }
    if (!best) {
        f->want_t = now ? now : 1;
        if (frees) { f->no_shore++; return 0; }   // free ones, but on no sea between the two
        f->no_ship++;
        void *cm = player_get_civ_mgr(pl);
        ee_unit_mgr *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
        int tt = ld_transport_type(pl);
        if (um && tt && (!f->train_t || now - f->train_t >= FY_TRAIN)) {   // paid as the expansion's (expand.c ex_ferry_ship)
            int req[4] = { tt, 1, 0, 0 };
            unsigned got = 0;
            g_priority_mil = 1; g_landing_order = 1; g_expand_econ = 1;
            hook_train_units(um, req, (uintptr_t)&got);
            g_priority_mil = 0; g_landing_order = 0; g_expand_econ = 0;
            if (got) {
                f->train_t = now ? now : 1; f->trained++;
                fy_say(idx, now, "trains a transport (type %d) for %d %s from land %u to land %u", tt, np, kind == 1 ? "soldiers" : "citizens", a, b);
            }
        }
        return 0;
    }
    f->want_t = 0;
    int cap = ld_cap(unit_type_get_id(unit_get_type_data(best))), room = 0;
    f->npass = 0;
    for (int k = 0; k < np && f->npass < 16; k++) {   // as many as fit (a gun or a rider takes more than one place)
        int sz = tw_size(pass[k]);
        if (room + sz > cap) continue;
        room += sz;
        f->pass[f->npass].u = pass[k]; f->pass[f->npass++].id = unit_get_serial(pass[k]);
    }
    if (f->npass < (kind == 1 ? FY_MIN : FY_MIN_CIT)) { f->npass = 0; return 0; }
    f->kind = kind; f->from = a; f->to = b; f->ocean = seas[bs].oc;
    f->pick = seas[bs].pick; f->pick_w = seas[bs].pick_w; f->drop = seas[bs].drop; f->dest = dest;
    f->ship.u = best; f->ship.id = unit_get_serial(best);
    f->phase = 1; f->since = now ? now : 1; f->load_t = 0; f->reissue = 0; f->esc_t = 0; f->trips++;
    f->last[kind].from = a; f->last[kind].to = b; f->last[kind].t = now ? now : 1;
    unit_goal(best, goal_move((float)f->pick_w.x + 0.5f, (float)f->pick_w.y + 0.5f), 0, 0x1F40);
    for (int k = 0; k < f->npass; k++)   // to the shore meanwhile
        unit_goal(f->pass[k].u, goal_move((float)f->pick.x + 0.5f, (float)f->pick.y + 0.5f), 0, 0x1F40);
    fy_say(idx, now, "%d %s from land %u (by %d,%d) to land %u (wanted at %d,%d) - the transport at %d,%d, boarding at %d,%d, off at %d,%d",
           f->npass, kind == 1 ? "soldiers" : "citizens", a, mid.x, mid.y, b, dest.x, dest.y, unit_get_x(best), unit_get_y(best),
           f->pick.x, f->pick.y, f->drop.x, f->drop.y);
    return 1;
}
internal int fy_enough(int gap, int want, int min) { return gap >= min && gap * FY_PART >= want; }
internal int fy_back(const struct fy_st *f, int kind, unsigned a, unsigned b, uint32_t now) {   // a -> b would go back the last trip's way
    return f->last[kind].t && f->last[kind].from == b && f->last[kind].to == a && now >= f->last[kind].t && now - f->last[kind].t < FY_BACK;
}
internal void fy_look(ee_player *pl, unsigned idx, struct fy_st *f, uint32_t now) {
    typeof(&g_snl[0]) S = &g_snl[idx];
    typeof(&g_wkl[0]) W = &g_wkl[idx];
    int sfresh = S->t && now >= S->t && now - S->t <= FY_FRESH, wfresh = W->t && now >= W->t && now - W->t <= FY_FRESH;
    // soldiers: the pair that moves most - the short land, the one with spare
    int bs = -1, as = -1, bn = 0;
    for (int b = 0; sfresh && b < S->n; b++) {
        if (!fy_enough(S->l[b].gap, S->l[b].want, FY_MIN)) continue;
        for (int a = 0; a < S->n; a++) {
            if (a == b || !fy_enough(-S->l[a].gap, S->l[a].want, FY_MIN)) continue;
            if (fy_back(f, 1, S->l[a].land, S->l[b].land, now)) { f->back++; continue; }
            int n = S->l[b].gap < -S->l[a].gap ? S->l[b].gap : -S->l[a].gap;
            if (n > bn) { bn = n; bs = b; as = a; }
        }
    }
    if (bs >= 0 && fy_start(pl, idx, f, 1, S->l[as].land, S->l[bs].land, bn, S->l[bs].at, now)) return;
    // citizens: the land with most free places (a drop site of its there), from the one with most to spare
    drops_cached(pl);
    int bc = -1, ac = -1;
    for (int k = 0; wfresh && k < W->n; k++) if (W->l[k].free >= FY_MIN_CIT && rs_drop_land(idx, W->l[k].land) && (bc < 0 || W->l[k].free > W->l[bc].free)) bc = k;
    for (int k = 0; bc >= 0 && k < W->n; k++)
        if (k != bc && W->l[k].spare >= FY_MIN_CIT && (ac < 0 || W->l[k].spare > W->l[ac].spare)) {
            if (fy_back(f, 2, W->l[k].land, W->l[bc].land, now)) { f->back++; continue; }
            ac = k;
        }
    if (bc >= 0 && ac >= 0) {
        int n = W->l[bc].free < W->l[ac].spare ? W->l[bc].free : W->l[ac].spare;
        fy_start(pl, idx, f, 2, W->l[ac].land, W->l[bc].land, n, W->l[bc].at, now);
    }
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
    fy_say(idx, now, "%u trips (%u soldiers, %u citizens put off; %u given up, %u transports sunk, %u put off on a beach under fire), %u transports trained, looks without a free transport %u, with none between the lands %u, trips back refused %u; %s | soldiers land:stationed/share%s",
           f->trips, f->soldiers, f->citizens, f->failed, f->sunk, f->beached, f->trained, f->no_ship, f->no_shore, f->back,
           f->phase ? (f->phase == 1 ? "a transport on its way" : f->phase == 2 ? "boarding" : "crossing") : "nothing on", b);
}
internal void ferry_act(ee_player *pl, unsigned idx, uint32_t now) {
    struct fy_st *f = &g_fy[idx];
    fy_minute(idx, f, now);
    if (!f->phase) {
        if (f->look_t && now >= f->look_t && now - f->look_t < FY_LOOK) return;
        f->look_t = now ? now : 1;
        fy_look(pl, idx, f, now);
        return;
    }
    if (!ex_alive(&f->ship, pl)) {
        f->sunk++; f->failed++;
        fy_end(pl, idx, f, now, f->phase == 3 ? "the transport was sunk on the way" : "the transport is gone");
        return;
    }
    ee_unit *sh = f->ship.u;
    if (convoy_lifeboat(pl, sh)) return;   // LIFEBOAT (convoy.c): under fire, to the nearest beach - stands back meanwhile
    if (f->phase >= 2 && convoy_beached(idx, sh, f->since)) { f->beached++; fy_end(pl, idx, f, now, "the transport was under fire - its passengers put off on the nearest beach"); return; }
    int n = 0, aboard = 0;
    for (int k = 0; k < f->npass; k++) if (ex_alive(&f->pass[k], pl)) f->pass[n++] = f->pass[k];
    f->npass = n;
    for (int k = 0; k < f->npass; k++) aboard += unit_get_x(f->pass[k].u) < 0;
    if (f->phase == 1 || f->phase == 2) {
        if (!f->npass) { fy_end(pl, idx, f, now, "no passenger left"); return; }
        if (now - f->since >= FY_BOARD && !aboard) { f->failed++; fy_end(pl, idx, f, now, "boarding failed in 4 minutes"); return; }
        if (f->phase == 1) {
            int there = ivec2_dist2(unit_get_tile(sh), f->pick_w) <= 49;
            if (!there && now - f->since < 120000) {
                if (!goal_near(sh, (float)f->pick_w.x + 0.5f, (float)f->pick_w.y + 0.5f, 2.0f))
                    unit_goal(sh, goal_move((float)f->pick_w.x + 0.5f, (float)f->pick_w.y + 0.5f), 0, 0x1F40);
                return;
            }
            f->phase = 2; f->load_t = now ? now : 1;
        }
        int esc = f->phase == 2 && aboard ? convoy_escort(idx, sh, f->drop, now) : 0;   // ESCORT (convoy.c): asked for once someone is aboard
        if (aboard == f->npass || (aboard && now - f->load_t >= FY_LOAD)) {
            if (esc == 1 && (!f->esc_t || now - f->esc_t < CY_WAIT)) { if (!f->esc_t) f->esc_t = now ? now : 1; return; }   // its escort on the way
            int m = 0;   // those left behind are the others' again
            for (int k = 0; k < f->npass; k++) if (unit_get_x(f->pass[k].u) < 0) f->pass[m++] = f->pass[k];
            f->npass = m;
            unit_goal(sh, ld_unload(sh, f->drop), 0, 0x1F40);
            f->phase = 3; f->since = now ? now : 1;
            fy_say(idx, now, "%d aboard - to land %u, off at %d,%d", aboard, f->to, f->drop.x, f->drop.y);
            return;
        }
        if (f->reissue && now - f->reissue < 20000) return;   // the load goal (again every 20 s for those not aboard)
        f->reissue = now ? now : 1;
        ee_unit *list[16];
        int m = 0;
        for (int k = 0; k < f->npass; k++) if (unit_get_x(f->pass[k].u) >= 0) list[m++] = f->pass[k].u;
        if (!m) return;
        ee_goal *g = goal_load(sh, list, m);
        if (!g || unit_get_goal(sh) != g) return;   // a load goal the transport didn't keep is deleted - no parts from it
        for (int k = 0; k < m; k++) unit_goal(list[k], goal_load_part(g), 0, 0x1F40);
        return;
    }
    // phase 3: across
    if (ex_cargo(sh) <= 0) {
        int off = 0;
        for (int k = 0; k < f->npass; k++) off += unit_get_x(f->pass[k].u) >= 0;
        if (f->kind == 1) f->soldiers += (unsigned)off; else f->citizens += (unsigned)off;
        fy_say(idx, now, "%d %s put off on land %u at %d,%d", off, f->kind == 1 ? "soldiers" : "citizens", f->to, f->drop.x, f->drop.y);
        if (f->kind == 1)   // on to where they are wanted; the stations take them there
            for (int k = 0; k < f->npass; k++) unit_goal(f->pass[k].u, goal_move((float)f->dest.x + 0.5f, (float)f->dest.y + 0.5f), 0, 0x1F40);
        fy_end(pl, idx, f, now, NULL);
        return;
    }
    if (now - f->since >= FY_SEA) { f->failed++; fy_end(pl, idx, f, now, "not across in 6 minutes - put off where it is"); return; }
    convoy_escort(idx, sh, f->drop, now);   // ESCORT (convoy.c)
    ee_goal *cur = unit_get_goal(sh);
    if (!cur || !goal_is_unload(cur)) unit_goal(sh, ld_unload(sh, f->drop), 0, 0x1F40);
}
