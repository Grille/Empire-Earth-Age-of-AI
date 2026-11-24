// THE CARRIERS ([ai_base] carriers; [autotest] v_carriers) - the fleets' moving air bases, on water and in space alike.
// The player, 2026-11-24: "continue with the space fighters and Carriers, also a good moment to introduce them for the
// sea - also be aware of planetary fighters (airport based who can travel space)". Included by eeaimod.c after
// airguard.c (as_order, as_rebase, the stats); called from fleet.c fleet_tick right after its scan (g_fs: the own
// carriers, docks, danger zones).
// What the game has (the data, AoC's tinfo): the Aircraft Carriers (class 21: Enterprise, Nexus) made at the Navy Yard,
// the Space Carrier (class 32) at the Space Dock; each makes its planes - the carriers' F-14, Corsair, Avenger, the
// Space Carrier's Space Fighter (class 34: the airplane's movement over land, water and space) - and they refuel
// aboard it (the type's refuel_at: 2 the carriers', 4 the Space Carrier's; an Airport's planes 1). A carrier with room
// left (unit room) takes a plane trained at it; aboard, a plane is off the map (x < 0, its holder the carrier).
// The navy's mix buys them (navy.c NR_CARR: one for every NV_CARRIER_PER warships of the other roles); here, every
// CA_EVERY:
//   BEHIND THE FLEET - each carrier follows the main group of its sea's warships (the ship with the most others within
//                      CA_GROUP_R, their middle), CA_BEHIND tiles back towards the nearest own dock of that sea, on
//                      water out of every danger zone (fleet.c fl_safe_toward); no warship on its sea: by that dock.
//                      Out of a danger zone first (fl_flee). Claimed as the fleet's (claims.c CLW_FLEET).
//   ITS PLANES       - trained at the carriers with room (the strongest plane of the carrier's it may train,
//                      navy.c nv_wing), CA_FLIGHT an order, CA_TRAIN_EVERY; held (the game's groups get none).
//   THE WINGS        - a ready plane (no goal, CA_READY_PCT of its hit points, aboard or idle) rises from its carrier
//                      at, first, an enemy aircraft within its reach of the carrier (the Space Fighters, the
//                      Planetary Fighters and helicopters too) it can hurt, the nearest; else an enemy ship or
//                      spaceship (warships, transports, carriers, fishing boats) it can hurt within its reach, the most
//                      worth for the way, where the enemy's anti-air within CA_AA_R of it (what can shoot the plane by
//                      the game's numbers: cruisers, corvettes, fighters, AA guns) can't bring the flight down over
//                      the target - CA_FLIGHT planes, their hit points against CA_MARGIN x that fire for CA_OVER_S and the
//                      way in. Its reach: CA_REACH of its flight time's way (the sweeps' rule), CA_REACH_MAX at most.
//   THE LEASH        - a plane sent CA_LEASH tiles beyond its reach from its carrier, or whose target is down, is called back
//                      (the planes pick their next targets themselves, into the anti-air by the shore)
//                      (EEMReturnToBase, as_rebase).
// "# carriers:" once a game minute in tests, and at each strike.
#define CA_EVERY 4000
#define CA_GROUP_R 8
#define CA_BEHIND 4
#define CA_TRAIN_EVERY 20000
#define CA_FLIGHT 4
#define CA_READY_PCT 60
#define CA_AGAIN 15000
#define CA_REACH 0.4f
#define CA_REACH_MAX 24.0f
#define CA_AA_R 10
#define CA_OVER_S 12.0f
#define CA_MARGIN 1.5f
#define CA_LEASH 8
#define CA_PLANES 128
#define CA_FOES 256
internal struct ca_st {
    uint32_t t, train_t, log_t;
    struct { int id, home, tgt; uint32_t t; } sent[CA_PLANES]; int nsent;
    unsigned moves, trained, queued, poor, refused, nowing, intercepts, strikes, risky, leashed; int last_wing;
    int planes, aboard, room;
    struct { int id, hp, fl; ivec2 at; const char *goal; } last[CA_PLANES]; int nlast; unsigned lost_logged;   // PLANES LOST (tests): the last look's
} g_ca[16];
internal void carriers_new_game(void) { memset(g_ca, 0, sizeof g_ca); }
internal int ca_plane(ee_unit_type *td) {   // a carrier's plane: refuels aboard a carrier (2) or a Space Carrier (4)
    int c = td ? unit_type_get_class_id(td) : -1, r = td ? unit_type_get_refuel_at(td) : 0;
    return (c == CLS_AIRPLANE || c == CLS_PLANET_SPACESHIP) && (r == 2 || r == 4) && unit_type_get_attack(td) > 0;
}
// the carrier a plane is based at: the one it is aboard, else the nearest (index into cv, -1 none)
internal int ca_base(ee_unit *u, ee_unit *const *cv, int n) {
    void *h = unit_get_x(u) < 0 ? unit_get_holder(u) : NULL, *a = unit_get_air_traffic(u);
    for (int k = 0; k < n; k++)
        if ((h && (h == (void *)cv[k] || h == (void *)unit_get_physics(cv[k]))) || (a && (a == (void *)cv[k] || a == (void *)unit_get_physics(cv[k])))) return k;
    if (unit_get_x(u) < 0) return -1;
    int b = -1, bd = 0;
    for (int k = 0; k < n; k++) {
        if (unit_get_x(cv[k]) < 0) continue;
        int d = ivec2_dist2(unit_get_tile(cv[k]), unit_get_tile(u));
        if (b < 0 || d < bd) { b = k; bd = d; }
    }
    return b;
}
// the enemy anti-air (airstrike.c ah_aa: AA guns and units, cruisers, corvettes, fighters) seen within its reach and
// CA_AA_PAD of p - the planes circle their carrier: lost to the AA tanks and fighters by an enemy planet's edge, 4 of
// 4 Space Fighters at a time (AoC Small Planets 2026-11-24)
#define CA_AA_PAD 6.0f
internal int ca_aa_near(ee_player *pl, ivec2 p) {
    for (struct grid_it it = grid_it_box(p.x, p.y, 20, 1); grid_it_next(&it); ) {
        ee_player *o = unit_get_owner(it.u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || unit_get_x(it.u) < 0 || !ah_aa(it.td) || !seen_by(it.u, pl)) continue;
        float r = ah_aa_reach(it.td) + CA_AA_PAD;
        if ((float)ivec2_dist2(unit_get_tile(it.u), p) <= r * r) return 1;
    }
    return 0;
}
// BEHIND THE FLEET: where carrier c goes - behind its sea's main group of warships, else by a dock of that sea; out of
// the enemy's anti-air reach (ca_aa_near): back along the way to the dock, else the dock, else nowhere
internal int ca_place(ee_player *pl, struct fleet *f, ee_unit *c, ivec2 *o) {
    int oc = fl_ship_ocean(c);
    if (!oc) return 0;
    ivec2 cp = unit_get_tile(c), dock = cp;
    int have_dock = 0, dd = 0;
    for (int k = 0; k < g_fs.nd; k++) {   // its sea's dock nearest the carrier
        ivec2 w;
        if ((int)ocean_near(g_fs.dk[k], 4, &w) != oc) continue;
        int d = ivec2_dist2(w, cp);
        if (!have_dock || d < dd) { dock = w; dd = d; have_dock = 1; }
    }
    int best = -1, bn = 0;
    for (int i = 0; i < f->n; i++) {   // the main group: the warship with the most others near
        if (fl_ship_ocean(f->u[i]) != oc) continue;
        int nn = 0;
        for (int j = 0; j < f->n; j++) nn += fl_ship_ocean(f->u[j]) == oc && ivec2_dist2(unit_get_tile(f->u[i]), unit_get_tile(f->u[j])) <= CA_GROUP_R * CA_GROUP_R;
        if (nn > bn) { bn = nn; best = i; }
    }
    ivec2 to;
    if (best >= 0) {
        ivec2 b = unit_get_tile(f->u[best]);
        float sx = 0, sy = 0; int m = 0;
        for (int j = 0; j < f->n; j++)
            if (fl_ship_ocean(f->u[j]) == oc && ivec2_dist2(b, unit_get_tile(f->u[j])) <= CA_GROUP_R * CA_GROUP_R) { sx += (float)unit_get_x(f->u[j]); sy += (float)unit_get_y(f->u[j]); m++; }
        float mx = sx / (float)m, my = sy / (float)m, dx = (float)dock.x - mx, dy = (float)dock.y - my, l = sqrtf(dx * dx + dy * dy);
        to = l > 0.5f ? iv2((int)(mx + dx / l * (float)(l < CA_BEHIND ? l : CA_BEHIND)), (int)(my + dy / l * (float)(l < CA_BEHIND ? l : CA_BEHIND))) : iv2((int)mx, (int)my);
    } else if (have_dock) to = dock;
    else return 0;
    ivec2 w;
    if (!fl_water(to, (unsigned)oc, &w)) return 0;
    if (fl_in_danger_m(w, 2.0f) && !fl_safe_toward(have_dock ? dock : cp, w, (unsigned)oc, &w)) return 0;
    if (ca_aa_near(pl, w)) {
        ivec2 from = have_dock ? dock : cp, q, safe = iv2(-1, -1);
        float dx = (float)(from.x - w.x), dy = (float)(from.y - w.y), len = sqrtf(dx * dx + dy * dy);
        int steps = (int)(len / 2.0f) + 1;
        for (int i = 1; i <= steps && safe.x < 0; i++) {
            float t = (float)i / (float)steps;
            if (fl_water(iv2(w.x + (int)(dx * t), w.y + (int)(dy * t)), (unsigned)oc, &q) && !ca_aa_near(pl, q) && !fl_in_danger_m(q, 2.0f)) safe = q;
        }
        if (safe.x < 0) return 0;
        w = safe;
    }
    *o = w;
    return 1;
}
// a plane free for an order: aboard or without a goal, or circling its carrier (the Space Fighters never go aboard:
// they keep a move goal round it) - no attack of its own on
internal int ca_ready(ee_unit *u, ee_unit *c) {
    ee_goal *g = unit_get_goal(u);
    if (!g || unit_get_x(u) < 0) return 1;
    return !goal_is_attack(g) && c && unit_get_x(c) >= 0 && ivec2_dist2(unit_get_tile(u), unit_get_tile(c)) <= 8 * 8;
}
// the risk of a flight of n planes of type ptd (stats pst, its owner pi) from `from` at target t: the fire of the enemy
// units that can hurt the plane and reach the way there (airstrike.c ah_seg_d2) or the target - those by the target for
// CA_OVER_S, the others for their reach's way across at speed sp, both ways; 1 when n of them come through. The way
// counts: of the water carriers' planes lost, 47 of 69 fell to the AA tanks and missile towers by the shore on a strike
// at a ship off it (Small Islands head-to-head 2026-11-24)
internal int ca_flight_ok(int pi, ee_unit_type *ptd, ee_unit_stats *pst, ivec2 from, ee_unit *t, int n, float sp, ee_unit *const *aa, int naa) {
    float dmg = 0, v = sp > 0.2f ? sp : 0.2f;
    ivec2 p = unit_get_tile(t);
    for (int k = 0; k < naa; k++) {
        ee_unit *e = aa[k];
        ee_unit_type *etd = unit_get_type_data(e);
        ee_player *o = unit_get_owner(e);
        if (unit_get_hp(e) <= 0 || unit_get_x(e) < 0 || !o) continue;
        float r = ah_aa_reach(etd), d2 = (float)ah_seg_d2(from, p, unit_get_tile(e));
        if (d2 > r * r) continue;
        int s = st_shot_t(etd, unit_get_stats(e), player_get_index(o), ptd, pst, pi);
        if (s <= 0) continue;
        float dps = (float)s / (st_reload_t(etd, unit_get_stats(e)) + ST_SHOT_OVERHEAD);
        int over = (float)ivec2_dist2(unit_get_tile(e), p) <= (r + 2.0f) * (r + 2.0f);
        dmg += dps * (over ? CA_OVER_S : 0.0f) + dps * 2.0f * 2.0f * sqrtf(r * r - d2) / v;   // over the target, and across its reach in and out
    }
    float hp = (float)n * (float)st_maxhp(ptd, pst) * (float)CA_READY_PCT / 100.0f;
    return dmg * CA_MARGIN < hp;
}
internal void carriers_tick(ee_player *pl, unsigned idx, struct fleet *f, uint32_t now) {
    static ee_unit *planes[CA_PLANES], *foe[CA_FOES], *aa[CA_FOES];
    static int base[CA_PLANES];
    struct ca_st *s = &g_ca[idx & 15];
    if (now < s->t) memset(s, 0, sizeof *s);
    if (!CARRIERS(idx) || !g_fs.ncv || (s->t && now - s->t < CA_EVERY)) return;
    s->t = now ? now : 1;
    // BEHIND THE FLEET
    for (int k = 0; k < g_fs.ncv; k++) {
        ee_unit *c = g_fs.cv[k];
        if (unit_get_hp(c) <= 0 || unit_get_x(c) < 0) continue;
        if (fl_flee(f, c)) continue;
        ivec2 to;
        if (!ca_place(pl, f, c, &to) || !cl_take(c, CLP_TASK, CLW_FLEET, 2 * CA_EVERY)) continue;
        unsigned o0 = f->orders;
        fl_move(f, c, to, 0);
        s->moves += f->orders != o0;
    }
    // ITS PLANES: on the map, aboard
    int np = 0, aboard = 0, room = 0, count = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384 && np < CA_PLANES; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0 || !ca_plane(td)) continue;
        if (!unit_get_ai_held(u)) player_hold_unit(pl, u);
        base[np] = ca_base(u, g_fs.cv, g_fs.ncv);
        aboard += unit_get_x(u) < 0;
        planes[np++] = u;
    }
    for (int k = 0; k < g_fs.ncv; k++) if (unit_get_hp(g_fs.cv[k]) > 0 && unit_get_room(g_fs.cv[k]) > 0) room += unit_get_room(g_fs.cv[k]);
    s->planes = np; s->aboard = aboard; s->room = room;
    if (g_log && g_autotest) {   // PLANES LOST (tests): one at the last look and not now - where, its hit points, flight left, goal, the nearest enemy that can hit it
        for (int k = 0; k < s->nlast && s->lost_logged < 60; k++) {
            int i = 0;
            while (i < np && unit_get_serial(planes[i]) != s->last[k].id) i++;
            if (i < np) continue;
            s->lost_logged++;
            int ad = -1, at = -1;
            for (struct grid_it it = grid_it_box(s->last[k].at.x, s->last[k].at.y, 14, 1); grid_it_next(&it); ) {
                ee_player *o = unit_get_owner(it.u);
                if (!o || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || !ah_aa(it.td)) continue;
                int dd = (int)ivec2_dist2(unit_get_tile(it.u), s->last[k].at);
                if (ad < 0 || dd < ad) { ad = dd; at = unit_type_get_id(it.td); }
            }
            log_locked("# carriers: P%u at %.1f min: a plane lost - last seen at %d,%d (%d hit points, %d ms flight left, goal %s); the nearest enemy anti-air type %d at %d tiles\n",
                       idx, now / 60000.0, s->last[k].at.x, s->last[k].at.y, s->last[k].hp, s->last[k].fl, s->last[k].goal ? s->last[k].goal : "none", at, ad < 0 ? -1 : (int)sqrtf((float)ad));
        }
        s->nlast = 0;
        for (int i = 0; i < np; i++) {
            ee_goal *gl = unit_get_goal(planes[i]);
            s->last[s->nlast].id = unit_get_serial(planes[i]); s->last[s->nlast].hp = unit_get_hp(planes[i]); s->last[s->nlast].fl = unit_get_flight_ms(planes[i]);
            s->last[s->nlast].at = unit_get_tile(planes[i]); s->last[s->nlast].goal = gl ? rtti_name(gl) : NULL; s->nlast++;
        }
    }
    if (room > 0 && (!s->train_t || now - s->train_t >= CA_TRAIN_EVERY)) {
        s->train_t = now ? now : 1;
        ee_civ_mgr *cm = player_get_civ_mgr(pl);
        ee_unit_mgr *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
        for (int k = 0; um && k < g_fs.ncv; k++) {   // the first carrier with room's plane: the game queues it at a carrier with room
            ee_unit *c = g_fs.cv[k];
            if (unit_get_hp(c) <= 0 || unit_get_room(c) <= 0) continue;
            int w = nv_wing(pl, unit_type_get_id(unit_get_type_data(c)));
            if (!w) { s->nowing++; continue; }
            s->last_wing = w;
            int req[4] = { w, room < CA_FLIGHT ? room : CA_FLIGHT, 0, 0 };
            unsigned got = 0;
            int paid0 = 0, paid1 = 0;
            for (int r = 2; r < 7; r++) paid0 += player_stock_at(pl)[r];
            hook_train_units(um, req, (uintptr_t)&got);   // the navy account pays it (navy.c navy_pays), as fleet.c fl_build's warships
            for (int r = 2; r < 7; r++) paid1 += player_stock_at(pl)[r];
            if (got && paid1 >= paid0) got = 0;   // fleet.c ONLY WHAT WAS PAID: the game's count says yes to an order nobody queued
            // the game's unit order finds no Space Carrier to make it at (0x494F42 takes the makers that pass their type's
            // vf 0x48: the Aircraft Carriers do, the Space Carrier not - 7 orders, none queued, AoC Small Planets): then
            // into each carrier's own queue with room, as a player's click (EEUnit::enqueue pays it), one a carrier - out
            // of the navy account, charged to it (navy_paid)
            const int *stock = player_stock_at(pl);
            ee_unit_type *wtd = type_data(w);
            for (int j = k; !got && wtd && j < g_fs.ncv; j++) {
                ee_unit *cj = g_fs.cv[j];
                if (unit_get_hp(cj) <= 0 || unit_get_room(cj) <= 0 || nv_wing(pl, unit_type_get_id(unit_get_type_data(cj))) != w) continue;
                int pays = 1, acc = 0, before[7], spent[7] = { 0 }, any = 0;
                for (int r = 2; r < 7; r++) { pays &= stock[r] - g_bd_com[idx][r] >= unit_cost(pl, w, r); before[r] = stock[r]; if (r != 2 && r != 4) acc += unit_cost(pl, w, r); }
                if (!pays || (navy_share(idx) > 0 && navy_account(idx, 3) < acc)) { s->poor++; break; }
                unit_enqueue(cj, unit_type_get_tech(wtd), 0);
                for (int r = 2; r < 7; r++) { spent[r] = before[r] - stock[r]; any |= spent[r] > 0; }
                if (any) { got++; s->queued++; if (navy_share(idx) > 0) navy_paid(idx, w, spent, 1); }
            }
            s->trained += got; s->refused += !got;
            if (got && g_log) log_locked("# carriers: P%u at %.1f min: %u x plane %d ordered for its carriers (%d planes, %d aboard, room %d)\n", idx, now / 60000.0, got, w, np, aboard, room);
            break;
        }
    }
    // THE WINGS: the enemy it sees - aircraft, ships and spaceships
    int nf = 0, naa = 0;   // and its anti-air (airstrike.c ah_aa), for the flights' way
    for (struct grid_it it = grid_it_all(1); np && grid_it_next(&it) && nf < CA_FOES; ) {
        ee_player *o = unit_get_owner(it.u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || unit_get_x(it.u) < 0) continue;
        if (naa < CA_FOES && ah_aa(it.td) && seen_by(it.u, pl)) aa[naa++] = it.u;
        if (!is_air_class(unit_type_get_class_id(it.td)) && navy_kind(it.td) == NV_NONE) continue;
        if (seen_by(it.u, pl)) foe[nf++] = it.u;
    }
    int pi = player_get_index(pl);
    static uint8_t used[CA_PLANES];
    memset(used, 0, (size_t)np);
    for (int i = 0; i < np && nf; i++) {
        ee_unit *u = planes[i];
        int b = base[i];
        if (used[i] || b < 0) continue;
        ee_unit_type *td = unit_get_type_data(u);
        ee_unit_stats *st = unit_get_stats(u);
        int id = unit_get_serial(u), k = 0;
        while (k < s->nsent && s->sent[k].id != id) k++;
        if (k < s->nsent && now >= s->sent[k].t && now - s->sent[k].t < CA_AGAIN) continue;
        if (!ca_ready(u, g_fs.cv[b]) || unit_get_hp(u) * 100 < st_maxhp(td, st) * CA_READY_PCT) continue;
        float sp = st ? unit_stats_get_speed(st) : 0.0f;
        int fms = st ? unit_stats_get_flight_ms(st) : 0;
        float reach = fms > 0 && sp > 0.2f ? (float)fms / 1000.0f * sp * CA_REACH : CA_REACH_MAX;
        if (reach > CA_REACH_MAX) reach = CA_REACH_MAX;
        ivec2 from = unit_get_tile(g_fs.cv[b]);
        // an enemy aircraft within reach it can hurt, the nearest; else a ship, the most worth for the way, where its flight comes through
        ee_unit *tg = NULL; int air = 0; float bv = 0;
        for (int j = 0; j < nf; j++) {
            ee_unit *e = foe[j];
            ee_unit_type *etd = unit_get_type_data(e);
            ee_player *eo = unit_get_owner(e);
            float d = sqrtf((float)ivec2_dist2(unit_get_tile(e), from));
            if (d > reach || st_shot_t(td, st, pi, etd, unit_get_stats(e), eo ? player_get_index(eo) : 0) <= 0) continue;
            int ea = is_air_class(unit_type_get_class_id(etd));
            if (ea) { if (!air || d < bv) { tg = e; bv = d; air = 1; } continue; }
            if (air) continue;
            int c = 0;
            for (int r = 2; r < 7; r++) c += unit_cost(pl, unit_type_get_id(etd), r);
            float v = (float)(c < 800 ? c : 800) / (30.0f + d);
            if (v > bv) { tg = e; bv = v; }
        }
        if (!tg) continue;
        // the flight: the ready planes of its carrier, up to CA_FLIGHT
        static int fl[CA_FLIGHT];
        int n = 0;
        for (int j = i; j < np && n < CA_FLIGHT; j++) {
            ee_unit *v = planes[j];
            int vid = unit_get_serial(v), q = 0;
            if (used[j] || base[j] != b || unit_type_get_id(unit_get_type_data(v)) != unit_type_get_id(td)) continue;
            while (q < s->nsent && s->sent[q].id != vid) q++;
            if ((q < s->nsent && now >= s->sent[q].t && now - s->sent[q].t < CA_AGAIN) || !ca_ready(v, g_fs.cv[base[j]]) || unit_get_hp(v) * 100 < st_maxhp(td, st) * CA_READY_PCT) continue;
            fl[n++] = j;
        }
        for (int j = 0; j < n; j++) used[fl[j]] = 1;
        if (!air && !ca_flight_ok(pi, td, st, from, tg, n, sp, aa, naa)) { s->risky++; continue; }
        int sent = 0;
        for (int j = 0; j < n; j++) {
            ee_unit *v = planes[fl[j]];
            if (!cl_take(v, CLP_TASK, CLW_FIGHTERS, 60000)) continue;
            if (!as_order(v, goal_attack(v, tg))) { cl_end(v, CLW_FIGHTERS); continue; }
            int vid = unit_get_serial(v), q = 0;
            while (q < s->nsent && s->sent[q].id != vid) q++;
            if (q == s->nsent) { if (s->nsent < CA_PLANES) s->nsent++; else q = (int)(now % CA_PLANES); }
            s->sent[q].id = vid; s->sent[q].home = unit_get_serial(g_fs.cv[b]); s->sent[q].tgt = unit_get_serial(tg); s->sent[q].t = now ? now : 1;
            sent++;
        }
        if (!sent) continue;
        if (air) s->intercepts++; else s->strikes++;
        if (g_log) log_locked("# carriers: P%u at %.1f min: %d planes (type %d) from the carrier at %d,%d at %s type %d at %d,%d (%d tiles)\n", idx, now / 60000.0, sent, unit_type_get_id(td),
                              from.x, from.y, air ? "the aircraft" : "the ship", unit_type_get_id(unit_get_type_data(tg)), unit_get_x(tg), unit_get_y(tg), (int)sqrtf((float)ivec2_dist2(unit_get_tile(tg), from)));
    }
    // THE LEASH
    for (int k = 0; k < s->nsent; k++) {
        if (!s->sent[k].t || now < s->sent[k].t || now - s->sent[k].t >= 4 * CA_AGAIN) continue;
        ee_unit *u = unit_by_id((unsigned)s->sent[k].id), *c = unit_by_id((unsigned)s->sent[k].home);
        if (!u || unit_get_serial(u) != s->sent[k].id || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || unit_get_flight_ms(u) <= 0) continue;
        if (!c || unit_get_serial(c) != s->sent[k].home || unit_get_hp(c) <= 0 || unit_get_x(c) < 0) continue;
        ee_unit_stats *st = unit_get_stats(u);
        float sp = st ? unit_stats_get_speed(st) : 0.0f;
        int fms = st ? unit_stats_get_flight_ms(st) : 0;
        float reach = fms > 0 && sp > 0.2f ? (float)fms / 1000.0f * sp * CA_REACH : CA_REACH_MAX;
        if (reach > CA_REACH_MAX) reach = CA_REACH_MAX;
        float r = reach + (float)CA_LEASH;
        ee_unit *t = s->sent[k].tgt ? unit_by_id((unsigned)s->sent[k].tgt) : NULL;
        int down = s->sent[k].tgt && (!t || unit_get_serial(t) != s->sent[k].tgt || unit_get_hp(t) <= 0);   // its target down: home, not on to the next it picks
        if (!down && (float)ivec2_dist2(unit_get_tile(u), unit_get_tile(c)) <= r * r) continue;
        as_rebase(pl, s->sent[k].id, (unsigned)s->sent[k].home);
        s->sent[k].t = 0; s->leashed++;
    }
    if (g_log && g_autotest && (!s->log_t || now - s->log_t >= 60000)) {
        s->log_t = now ? now : 1;
        log_locked("# carriers: P%u at %.1f min: %d carriers, %d planes (%d aboard), room %d | so far %u moves, %u planes ordered (%u of them into a carrier's own queue, %u not paid by the navy account, %u orders refused, plane %d; %u looks with no plane to make), %u intercepts, %u strikes, %u flights too risky, %u called back\n",
                   idx, now / 60000.0, g_fs.ncv, np, aboard, room, s->moves, s->trained, s->queued, s->poor, s->refused, s->last_wing, s->nowing, s->intercepts, s->strikes, s->risky, s->leashed);
    }
}
