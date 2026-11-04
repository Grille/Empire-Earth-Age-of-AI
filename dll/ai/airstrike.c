// THE AIR FORCE ([autotest] v_air_strike; AS_ON, on since 2026-11-04 - below). The player, 2026-11-04, on breaking through a
// held position: "if I have to go through I bombard it, airplanes are preferred if the AA is weak"; then: "we need take
// full control of aircraft training and deployment too, the ai trickles them in which is very inefficient ... a proper
// bombing run can take care of [AA], you notice I have 4 airfields side by side with bombers, so pairs of 4 arrive
// together, 4 bombers easily take out an AA gun and overwhelm defenses". The game's own AI flies its planes one by one
// in its air attack subgroups (EECPASGAir, a script state machine) at the commander's target. Every AS_EVERY per AI
// with the mod's force (armies.c), from the bombers' epochs:
// - HELD: every bomber (family bomber, class airplane; not the atomic ones) is the mod's from its first tick - held
//   the game's way (player hold_unit, as armies.c holds its soldiers; unheld, the game's air subgroup took 3-5 of 6
//   back to its patrols within 5 s, air_strike.eeai), none of the game's groups gets it.
// - TRAINED: while a position needs the wing (seen within AF_STRONG_MS), every AF_TRAIN_EVERY the wing wanted -
//   AF_WING (8), or what the last position needed if more, at most AF_WING_MAX - less the bombers it has and those
//   asked for and not out yet - is trained (a standing wing with every request for the whole shortfall again: one
//   AI asked for 33 Gothas in 34 minutes and had 9 - 6600 gold and iron in its airports' queues while its army was
//   refused for gold and iron hundreds of times a minute; head-to-head from WWI soldiers -31, razed -25), the newest bomber the player can (the request as the siege controller's, through the
//   budget), with an Airport queued while there are fewer than one per AF_PER_AIRPORT of the wing (up to
//   AF_AIRPORTS; AF_AIRPORT_TRIES queued without one more there, none until there is - queued every minute, the build
//   manager dropped them): the planes take off from all of them at once. The Airport at the force's home - its staging point
//   towards the front - when that is AF_FORWARD tiles or more from the base (the player: "a human player moves its
//   airfields forward to reach deeper into enemy territory, the home airfields become empty backup ... the ai's
//   airplanes are all stuck at home since the ai never extends their reach"), else where the build manager picks.
// - THE SHOOTERS first (the player, 2026-11-04, on his artillery that shelled the force from beyond its sight: "planes
//   are a great counter here"): where a land unit hit the AI's units from AF_SHOOT_D tiles or more (intel.c
//   reveal_attackers), spots within AF_SHOOT_JOIN joined, for AF_SHOOT_MS after its last hit - the one hit from most
//   often is the position, its long-range units (artillery, siege, AF_SHOOT_D+ range) known there and the first
//   targets after the anti-air; a spot with nothing left to hit is dropped.
// - THE WING: else the position - the breach's focus while the force stands off (g_ar_breach), else its push's target
//   while out, else for AF_STRONG_MS the target its last attack time found too strong (armies.c a->strong: "if I have
//   to go through I bombard it" - the bombs before the push; air_strike.eeai: the force waited at a position with two
//   AA guns, no push, no strike) -; the anti-air near it (AA buildings and land AA units seen or remembered within AS_AA_R) and what
//   holds it (defences with a ground attack, attack.c defence_stats, then artillery or siege seen within AS_R, the
//   nearest first). Needed: a flight of AF_FLIGHT per anti-air, one more for the defences. The bombers ready - idle
//   (no goal: landed and rearmed) with AF_READY_PCT of their hit points - launch together when they are that many:
//   a flight at each anti-air, the rest in flights at the defences, by the game's own air order (0x442AD8: a plane
//   in its airport takes it; the mod's unit_goal refuses a unit off the map). A bomber of the wing whose target still
//   stands is left alone while it has any goal and for AS_AGAIN after its order: a plane changes its goals by itself on
//   a run (the attack, moves to take off and back to its airport) - an order every 5 s cut the runs short; idle again
//   it goes again. A target down: its bombers back to the ready. A target not down AF_RUN_MS after the launch, or
//   off the map (garrisoned), is given up: the bomber back to the ready (one wing at a time - on the save "choke
//   wolf" the first wing of 4 went at 158 min and none after, 12 bombers held to 210: a target that never went down). What holds it, the soldiers seen there last (what the
//   force's judgement counts: with the bombs only on the buildings it judged the position too strong all the same,
//   air_strike.eeai). BOMB, THEN GO: a wing's targets all down, the force judges its push again at once (armies.c
//   g_ar_push_now), not at its next attack time.
// The planes come back to their airports by themselves (the air traffic control). "# air force:" lines; a summary a
// minute (autotest).
// First measured (2026-11-04, off then): tests/scenarios/air_strike.eeai (a WWI position of the idle human with two 88mm AA guns; P2 with
// three airports and twelve Gothas): the force found the position too strong, the wing of 12-13 went at 3.3-3.8 min,
// two flights of 4 took out both AA guns, the rest the towers; single runs of the scene go different ways at every
// step. Head-to-head from World War I (late.eeai, start_epoch 9, 75 min, 24 mirrored games, 4 AIs) with - without: a
// standing wing first (8 trained whatever the need, every request the whole shortfall again) alive -0.21 (se 0.08),
// soldiers -31.5 (7.3), kills -49.3 (17.4), razed -25.1 (6.7), workers -34.1 (12.3) - airports queued every minute,
// up to 33 bombers asked for and 9 out, the army refused for gold and iron; trained only for a position, the asked
// counted: soldiers -3.5 (9.7), kills -1.0 (19.8), losses +6.8 (9.9), razed -3.8 (7.2) - one AI of 48 flew a wing (2
// waves): the test AIs can't pay for bombers (200 gold, 200 iron), their metal is short already. Not measured yet: a
// rich late game; the plane's reach (no range or fuel known: the forward Airport only).
// THE SHOOTERS (2026-11-04), the save "choke wolf" 153 -> 210 min (3 timings, P3 the variant), with - without: razed
// 86, 108, 93 / 56, 24, 80 (the last run ended at 180), units killed 459, 592, 582 / 462, 295, 491, units lost in
// gold + iron the same (44-45k / 28-50k); 1-3 wings, every one at the shooters' spot; the bombers came one at a time
// (the gold stock ~200: the soldiers take the rest). Head-to-head from WWI (Continental, 4 AIs, 36 mirrored games) with -
// without: alive +0.00 (0.10), soldiers -4.5 (8.4), kills -5.1 (18.7), losses -10.5 (9.2), razed -2.9 (6.9) - 49 wings
// flown in the first 12 (the AIs shell each other too). ON (2026-11-04): the gain where the player's kind of position
// holds the way, nothing lost elsewhere, and the player's call ("we need take full control of aircraft").

#ifndef AS_ON
#define AS_ON 1
#endif
#define AS_EVERY 5000
#define AS_R 14
#define AS_AA_R 14
#define AS_TARGETS 32
#define AS_BOMBERS 128
#define AS_AGAIN 20000
#define AF_FLIGHT 4
#define AF_WING 8
#define AF_WING_MAX 24
#define AF_PER_AIRPORT 4
#define AF_AIRPORTS 6
#define AF_READY_PCT 60
#define AF_TRAIN_EVERY 20000
#define AF_STRONG_MS 300000
#define AF_AIRPORT_TRIES 3
#define AF_FORWARD 15
#define AF_SHOOT_D 7
#define AF_SHOOT_JOIN 5
#define AF_SHOOT_MS 180000
#define AF_SHOOTS 16
#define AF_RUN_MS 180000

internal int g_v_air_strike = -1;
internal struct as_st {
    uint32_t t, log_t, train_t, airport_t, wait_log;
    struct { int id, tgt; uint32_t t, t0; } held[AS_BOMBERS]; int nh;   // the bombers held, their targets (serials; 0 none), when ordered, when launched at it
    int needed; uint32_t need_t;                                    // the last position's wing, when
    int asked, ap_seen, ap_tries; uint32_t asked_t;                                   // bombers asked for and not out yet; airports (and building) at the last queue, queued since without more
    unsigned waves, sent, trained, airports, waits, gave_up;        // waves launched, bombers sent, trained, airports queued, launches waited for more, targets given up
    int wing_out;                                                   // a wing launched whose targets aren't all down yet
    struct { ivec2 at; uint32_t t; int hits; } sh[AF_SHOOTS];       // the shooters' spots (intel.c reveal_attackers): where, the last hit, hits seen
} g_as[16];
// the bombers to train, the newest first (epochs as the tech tree counts them: current_epoch + 1)
internal const struct { enum ut_name type; int epoch; } k_af_types[] = {
    { UT_A_BOMBER_13_B_122_WYVERN, 13 }, { UT_A_BOMBER_12_STEALTH_B_2, 12 }, { UT_A_BOMBER_11_B_17, 11 },
    { UT_A_BOMBER_11_HEINKEL_111, 11 }, { UT_A_BOMBER_10_GOTHA, 10 } };
internal short g_as_types[64]; internal int g_as_nt = -1;   // the bomber types in the data (once a game)

internal void as_types(void) {
    if (g_as_nt >= 0) return;
    g_as_nt = 0;
    for (int t = 0; t < G_TYPES_N && g_as_nt < 64; t++) {
        ee_unit_type *td = type_data(t);
        if (!td || unit_type_get_id(td) != t) continue;
        if (unit_type_get_family(td) == KIND_BOMBER && unit_type_get_class_id(td) == CLS_AIRPLANE) g_as_types[g_as_nt++] = (short)t;
    }
    if (g_log) {
        fprintf(g_log, "# air force: bomber types");
        for (int k = 0; k < g_as_nt; k++) fprintf(g_log, " %d", g_as_types[k]);
        fprintf(g_log, "\n");
    }
}
// the game's air order (attack.c's unit_goal refuses a plane in its airport): 1 given; the goal freed if not
internal int as_order(ee_unit *u, ee_goal *goal) {
    if (!goal) return 0;
    if (attack_subgroup_air_give_goal(NULL, u, goal, 0, 0x1F40)) return 1;   // its `this` is not read (0x442AD8)
    ((void (__thiscall *)(void *, int))(*(void ***)goal)[0])(goal, 1);
    return 0;
}
// a bomber of a player whose air force is on: the game's groups don't get it (armies.c armies_refuses_offer) nor
// order it (landing.c hook_asg_air_give_goal) - held, the game's air subgroup it was in still ordered it about
// (air_strike.eeai: none of 11 held bombers ever idle)
internal int af_takes(ee_unit *u) {
    ee_player *pl = u ? unit_get_owner(u) : NULL;
    ee_unit_type *td = pl ? unit_get_type_data(u) : NULL;
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    return td && unit_type_get_family(td) == KIND_BOMBER && unit_type_get_class_id(td) == CLS_AIRPLANE && PV(idx, g_v_air_strike, AS_ON) &&
           ar_on(pl, idx) && current_epoch(pl) + 1 >= 10;
}
internal int as_held_at(struct as_st *s, int id) {
    for (int k = 0; k < s->nh; k++) if (s->held[k].id == id) return k;
    return -1;
}
// the finished Airports and those still building
internal int af_airports(void *um, int *building) {
    uint8_t *q = type_list(um, UT(B_AIRPORT));
    int n = 0;
    *building = 0;
    for (unsigned j = 0, f = q ? deque_front(q) : 0, c = q ? deque_count(q) : 0; j < c && j < 64; j++) {
        ee_unit *b = deque_at(q, f + j);
        if (!b || unit_get_hp(b) <= 0) continue;
        ee_unit *bs = unit_get_physics(b);
        if (bs && unit_get_finished(bs)) n++; else (*building)++;
    }
    return n;
}
// TRAINED: the wing wanted, the airports for it
internal void af_train(ee_player *pl, unsigned idx, struct as_st *s, int have, uint32_t now) {
    if (s->train_t && now - s->train_t < AF_TRAIN_EVERY) return;
    s->train_t = now ? now : 1;
    if (!s->need_t || now - s->need_t >= AF_STRONG_MS) return;   // no position needs it
    if (s->asked && now - s->asked_t >= AF_STRONG_MS) s->asked = 0;   // asked long ago and none came: the queue is gone
    int want = s->needed > AF_WING ? s->needed : AF_WING;
    if (want > AF_WING_MAX) want = AF_WING_MAX;
    if (have + s->asked >= want) return;
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    void *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!um) return;
    int building, ap = af_airports(um, &building), ap_want = (want + AF_PER_AIRPORT - 1) / AF_PER_AIRPORT, e = current_epoch(pl);
    if (ap_want > AF_AIRPORTS) ap_want = AF_AIRPORTS;
    if (ap + building > s->ap_seen) s->ap_tries = 0;   // one more came of it
    s->ap_seen = ap + building;
    if (bm && ap + building < ap_want && ut_has(UT(B_AIRPORT)) && !bq_has(bm, UT(B_AIRPORT)) && e >= 0 && e < 14 &&
        s->ap_tries < AF_AIRPORT_TRIES && (!s->airport_t || now - s->airport_t >= 60000)) {
        s->airport_t = now ? now : 1;
        s->ap_tries++;
        ivec2 h = g_armies[idx].home, base = build_mgr_get_base(bm);
        int fwd = base.x != -10000 && (h.x || h.y) && ivec2_dist2(h, base) >= AF_FORWARD * AF_FORWARD;
        int entry[3] = { UT(B_AIRPORT), fwd ? h.x : 0, fwd ? h.y : 0 };
        build_mgr_queue_add(bm, e, entry);
        s->airports++;
        ar_log(idx, now, "air force: an Airport queued %s%d,%d (%d of %d for a wing of %d, %d building)", fwd ? "forward at " : "by the build manager, the base at ",
               fwd ? h.x : base.x, fwd ? h.y : base.y, ap, ap_want, want, building);
    }
    if (!ap) return;
    for (size_t k = 0; k < sizeof k_af_types / sizeof *k_af_types; k++) {
        if (k_af_types[k].epoch > e + 1 || !ut_has(g_ut[k_af_types[k].type])) continue;
        int n = want - have - s->asked < 4 ? want - have - s->asked : 4;
        int req[4] = { g_ut[k_af_types[k].type], n, 0, 0 };
        unsigned got = 0;
        g_priority_mil = 1;
        hook_train_units(um, req, (uintptr_t)&got);
        g_priority_mil = 0;
        if (got) {
            s->trained += got; s->asked += (int)got; s->asked_t = now ? now : 1;
            ar_log(idx, now, "air force: %u x type %d trained (has %d, a wing of %d, %d airports)", got, req[0], have, want, ap);
            return;
        }
    }
}
internal void airstrike_tick(ee_player *pl, uint32_t now) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct as_st *s = &g_as[idx];
    if (!PV(idx, g_v_air_strike, AS_ON) || !ar_on(pl, idx)) return;
    if (s->t && now >= s->t && now - s->t < AS_EVERY) return;
    if (now < s->t) memset(s, 0, sizeof *s);
    s->t = now ? now : 1;
    if (current_epoch(pl) + 1 < 10) return;   // no bombers before World War I
    if (g_log && g_autotest && (!s->log_t || now - s->log_t >= 60000)) {
        s->log_t = now ? now : 1;
        fprintf(g_log, "# air force: P%u at %.1f min: %d bombers held, %u trained, %u airports queued, %u waves (%u bombers sent), %u launches waited for more, %u targets given up\n",
                idx, now / 60000.0, s->nh, s->trained, s->airports, s->waves, s->sent, s->waits, s->gave_up);
    }
    as_types();
    // HELD: every bomber the mod's; the dead out of the list
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    void *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    static ee_unit *bomb[AS_BOMBERS];
    int nb = 0, m = 0;
    for (int k = 0; k < g_as_nt; k++) {
        uint8_t *q = type_list(um, g_as_types[k]);
        for (unsigned j = 0, f = q ? deque_front(q) : 0, c = q ? deque_count(q) : 0; j < c && nb < AS_BOMBERS; j++) {
            ee_unit *u = deque_at(q, f + j);
            if (u && unit_get_hp(u) > 0 && unit_get_owner(u) == pl) bomb[nb++] = u;
        }
    }
    for (int k = 0; k < s->nh; k++) {
        ee_unit *u = unit_by_id((unsigned)s->held[k].id);
        if (u && unit_get_serial(u) == s->held[k].id && unit_get_hp(u) > 0 && unit_get_owner(u) == pl) s->held[m++] = s->held[k];
    }
    s->nh = m;
    for (int i = 0; i < nb; i++) {
        int id = unit_get_serial(bomb[i]);
        if (as_held_at(s, id) >= 0 || s->nh >= AS_BOMBERS) continue;
        if (!unit_get_ai_held(bomb[i])) player_hold_unit(pl, bomb[i]);
        s->held[s->nh].id = id; s->held[s->nh].tgt = 0; s->held[s->nh].t = 0; s->nh++;
        if (s->asked > 0) s->asked--;   // one of those asked for is out
    }
    af_train(pl, idx, s, nb, now);
    // the shooters: what hit its units from afar at intel.c's last scan, joined into spots
    for (int k = 0; k < g_rev[idx].nsh; k++) {
        if (g_rev[idx].sh[k].bld || g_rev[idx].sh[k].d < AF_SHOOT_D) continue;
        ivec2 at = g_rev[idx].sh[k].at;
        int j, old = 0;
        for (j = 0; j < AF_SHOOTS; j++) {
            if (s->sh[j].t && now - s->sh[j].t < AF_SHOOT_MS && ivec2_dist2(s->sh[j].at, at) <= AF_SHOOT_JOIN * AF_SHOOT_JOIN) break;
            if (s->sh[j].t < s->sh[old].t) old = j;
        }
        if (j == AF_SHOOTS) { j = old; s->sh[j].at = at; s->sh[j].hits = 0; }
        s->sh[j].hits++; s->sh[j].t = now ? now : 1;
    }
    int shoot = -1;
    for (int j = 0; j < AF_SHOOTS; j++)
        if (s->sh[j].t && now - s->sh[j].t < AF_SHOOT_MS && (shoot < 0 || s->sh[j].hits > s->sh[shoot].hits)) shoot = j;
    // the position
    struct armies_st *a = &g_armies[idx];
    ivec2 pos;
    if (shoot >= 0) pos = s->sh[shoot].at;
    else if (g_ar_breach[idx].on) pos = g_ar_breach[idx].focus;
    else if (a->out == 1 && a->target) pos = a->dest;
    else if (a->strong_t && now - a->strong_t < AF_STRONG_MS) pos = a->strong;
    else return;
    // the anti-air near it and what holds it - the nearest first
    static struct { ee_unit *u; long d; } aa[AS_TARGETS], tg[AS_TARGETS];
    int na = 0, nt = 0;
    int r = AS_R > AS_AA_R ? AS_R : AS_AA_R;
    for (struct grid_it it = grid_it_box(pos.x, pos.y, r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || unit_get_hp(u) <= 0) continue;
        long d = ivec2_dist2(unit_get_tile(u), pos);
        int fam = unit_type_get_class_id(it.td), f = unit_type_get_family(it.td);
        ee_unit_stats *ust = unit_get_stats(u);
        float rg = ust ? unit_stats_get_range(ust) : unit_type_get_range(it.td), at;
        int lr = fam == CLS_LAND && (is_siege(unit_type_get_id(it.td)) || f == KIND_ARTILLERY || rg >= (float)AF_SHOOT_D);
        if (!seen_by(u, pl) && !(shoot >= 0 && lr && d <= AF_SHOOT_JOIN * AF_SHOOT_JOIN)) continue;   // the shooters' spot: known there
        if (f == KIND_AA_BUILDING || f == KIND_LAND_AA) {
            if (d > AS_AA_R * AS_AA_R || na == AS_TARGETS) continue;
            int k = na++;
            while (k > 0 && aa[k - 1].d > d) { aa[k] = aa[k - 1]; k--; }
            aa[k].u = u; aa[k].d = d;
            continue;
        }
        if (d > AS_R * AS_R) continue;
        if (shoot < 0 || !lr) {   // (at the shooters' spot they come first, by their distance)
            if (fam == CLS_BUILDING || fam == CLS_FORTRESS) { if (!defence_stats(u, &rg, &at) || at <= 0) continue; if (shoot >= 0) d += (long)AS_R * AS_R; }
            else if (fam == CLS_LAND && (is_siege(unit_type_get_id(it.td)) || f == KIND_ARTILLERY)) d += (long)AS_R * AS_R;   // after the defences
            else if (fam == CLS_LAND && is_soldier(it.td)) d += 2L * AS_R * AS_R;   // the soldiers last: what the force's judgement counts
            else continue;
        }
        if (nt == AS_TARGETS && tg[nt - 1].d <= d) continue;
        int k = nt < AS_TARGETS ? nt++ : AS_TARGETS - 1;
        while (k > 0 && tg[k - 1].d > d) { tg[k] = tg[k - 1]; k--; }
        tg[k].u = u; tg[k].d = d;
    }
    if (!na && !nt) { if (shoot >= 0) s->sh[shoot].t = 0; return; }   // nothing left at the shooters' spot: the next
    s->needed = AF_FLIGHT * (na + (nt ? 1 : 0)); s->need_t = now ? now : 1;
    // the wing out: a bomber whose target stands keeps at it; idle again, its next run
    static ee_unit *ready[AS_BOMBERS];
    int nr = 0, out = 0;
    for (int i = 0; i < nb; i++) {
        ee_unit *u = bomb[i];
        int e = as_held_at(s, unit_get_serial(u));
        if (e < 0) continue;
        ee_unit *t = s->held[e].tgt ? unit_by_id((unsigned)s->held[e].tgt) : NULL;
        int stands = t && unit_get_serial(t) == s->held[e].tgt && unit_get_hp(t) > 0;
        ee_goal *g = unit_get_goal(u);
        if (stands && (unit_get_x(t) < 0 || now - s->held[e].t0 >= AF_RUN_MS)) {   // given up: the bomber to the next wing
            ee_unit_type *ttd = unit_get_type_data(t);
            ar_log(idx, now, "air force: a bomber gives up type %d at %d,%d (%s; seen %d, hp %d, the bomber's goal %d)", ttd ? unit_type_get_id(ttd) : -1,
                   unit_get_tile(t).x, unit_get_tile(t).y, unit_get_x(t) < 0 ? "off the map" : "not down in time", seen_by(t, pl), unit_get_hp(t), g != NULL);
            s->gave_up++; stands = 0;
        }
        if (stands) {
            out++;
            if (g || now - s->held[e].t < AS_AGAIN) continue;
            if (as_order(u, goal_attack(u, t))) s->held[e].t = now;
            continue;
        }
        s->held[e].tgt = 0;
        if (!g && unit_get_hp(u) * 100 >= unit_type_get_hp(unit_get_type_data(u)) * AF_READY_PCT) ready[nr++] = u;
    }
    if (out) return;   // one wing at a time
    if (s->wing_out) {   // its targets down: the force judges its push again now, not at its next attack time
        s->wing_out = 0;
        g_ar_push_now |= 1u << idx;
        ar_price_clear(a, pos);
        ar_log(idx, now, "air force: the wing's targets at %d,%d are down - the force judges its push again", pos.x, pos.y);
    }
    if (nr < s->needed && nr < AF_WING_MAX) {
        s->waits++;
        if (!s->wait_log || now - s->wait_log >= 60000) {
            s->wait_log = now ? now : 1;
            ar_log(idx, now, "air force: the wing at %d,%d waits - %d bombers ready of the %d it needs (anti-air %d, defences %d; %d bombers in all)", pos.x, pos.y, nr, s->needed, na, nt, nb);
        }
        return;
    }
    // the launch: a flight at each anti-air, the rest in flights at the defences
    int sent = 0;
    for (int i = 0; i < nr; i++) {
        int f = i / AF_FLIGHT;
        ee_unit *t = f < na ? aa[f].u : nt ? tg[(f - na) % nt].u : aa[f % na].u;
        int e = as_held_at(s, unit_get_serial(ready[i]));
        if (e < 0 || !as_order(ready[i], goal_attack(ready[i], t))) continue;
        s->held[e].tgt = unit_get_serial(t); s->held[e].t = s->held[e].t0 = now ? now : 1;
        sent++;
    }
    if (!sent) return;
    s->waves++; s->sent += (unsigned)sent; s->wing_out = 1;
    ar_log(idx, now, "air force: a wing of %d at %d,%d%s - %d at %d anti-air, the rest at %d targets (%d bombers in all)", sent, pos.x, pos.y,
           shoot >= 0 ? " (the shooters)" : "", na * AF_FLIGHT < sent ? na * AF_FLIGHT : sent, na, nt, nb);
}
internal void airstrike_new_game(void) { memset(g_as, 0, sizeof g_as); g_as_nt = -1; }
internal void airstrike_install(const char *autotest_ini) {
    if (autotest_ini) g_v_air_strike = ini_int("autotest", "v_air_strike", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# air force: %d (variant %d)\n", AS_ON, g_v_air_strike);
}
