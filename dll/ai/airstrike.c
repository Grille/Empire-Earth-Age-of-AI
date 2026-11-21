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
//   targets after the anti-air; a spot with nothing left to hit is dropped. A field gun (intel.c sh_gun: the
//   ballista is the Middle Ages' group killer at range 6, the player) counts from AF_GUN_D. The spots are kept in every age and with
//   the air force off (the skirmishers, skirmish.c, go at them too - the player: "also consider the ages before
//   airplanes, most of the units exist in an earlier version": trebuchets and cannons hold a valley the same way).
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
// - THE LONE RUNS ([ai_base] bomber_harass, off; [autotest] v_bomber_harass): the wing waits with no time limit for the
//   bombers it needs, and they sit at their Airports meanwhile (a 90-minute island game logged "the wing ... waits" for
//   most of it; in air_rich below 83% of the bomber-minutes held were idle). The player, 2026-11-21: "the bombers
//   shouldnt sit idle, the wings are an way to overwelm devenses, lone bombers could still harees undefended targets".
//   While the wing gathers (or flies, or no position needs it), the bombers ready go in flights of up to AH_FLIGHT from
//   one Airport at what the AI knows of an enemy - buildings (seen or remembered; not walls) and citizens at work (two or
//   more within 4 tiles) - that is undefended as far as it knows:
//   - an enemy with an Airport or fighters known: none of its targets, and no way within AH_FIGHTER_R of a building of
//     its (the fighters rise at what comes near its buildings - airguard.c INTERCEPT);
//   - the ground within AH_LOOK_R of the target looked at within AH_LOOKED, and of an enemy known to have anti-air the
//     way there too (a point every AH_WAY_STEP): the anti-air it doesn't see is not there;
//   - no anti-air within its reach and AH_MARGIN (AH_AA_MIN at least) of the target or the straight way: AA buildings
//     and units, cruisers, carriers seen now or, with honest_vision, within AH_MEM where seen; by the Town Centers,
//     Capitols and Settlements of an enemy known to have anti-air unless looked at within AH_LOOKED (aa_strong puts its
//     blocks there); within AH_MEM where a bomber on a lone run was lost, and its target.
//   The most worth (its price, at most 800; 120 a citizen) for the way, one flight a target at a time. A bomber on a run
//   is claimed (claims.c CLW_BOMBERS, CLP_TASK - counter-fire took them as soldiers and sent them at the mortars);
//   called back to its Airport (the game's EEMReturnToBase, as airguard.c THE AIR BASES) when it goes after something
//   else, its target is down, or it is AH_STRAY off the way; back on the ground (or AH_RUN_MS on) it is free again.
//   None go while the bombers ready and those on a run make the wing: those out come back and it launches.
//   MEASURED 2026-11-21 (tests/scenarios/air_rich.eeai: Continental 101 map_size 1, 4 AIs, WWI, 60 min, 3000 of each
//   for every AI at 10, 25 and 40 min; head-to-head, seeds 1-10 mirrored, 20 games, B with the lone runs): B - A alive
//   -0.03 (se 0.11), soldiers -5.9 (6.0), kills -27 (28), losses -16 (13), razed -7.8 (7.7), workers -17 (12) - within
//   the noise, all on the wrong side; the idle share of the bomber-minutes held 0.83 both: 2.5 bombers an AI flew lone
//   runs in an hour, 0.8 of them lost - the AIs' anti-air (aa_strong's blocks by every town and drop site, the AA
//   half-tracks with the soldiers, fighters wherever an Airport stands) leaves next to nothing undefended. Looser
//   rules before (anti-air only where seen; no fighters' cover; no looked-at ground) flew 2-12 bombers an AI and lost
//   45-65% of them (by the 88mm guns and the half-tracks, most of them seen). Off: it is for an enemy without
//   anti-air, not for these.
// The planes come back to their airports by themselves (the air traffic control). "# air force:" lines; a summary a
// minute (autotest) - with the bomber-minutes held and idle (no goal at the look), the bombers lost, the lone runs.
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
#define AF_GUN_D 5
#define AF_SHOOT_JOIN 5
#define AF_SHOOT_MS 180000
#define AF_SHOOTS 16
#define AF_RUN_MS 180000

#define AH_FLIGHT 2
#define AH_MARGIN 6
#define AH_AA_MIN 10
#define AH_FIGHTER_R 20
#define AH_MEM 300000
#define AH_RUN_MS 150000
#define AH_AA 256
#define AH_CIT 256
#define AH_TARGETS 512
#define AH_FLAK 16
#define AH_FLAK_R 12
#define AH_LOOKED 60000
#define AH_LOOK_R 12
#define AH_WAY_STEP 8
#define AH_STRAY 6
#define AH_BLD 2048
#define AH_CELL 8
#define AH_CELLS (600 / AH_CELL + 1)

internal int g_v_air_strike = -1;
internal int g_as_harass = 0, g_v_as_harass = -1;   // [ai_base] bomber_harass, [autotest] v_bomber_harass (THE LONE RUNS, below)
#define AS_HARASS(idx) PV(idx, g_v_as_harass, g_as_harass)
internal struct as_st {
    uint32_t t, log_t, train_t, airport_t, wait_log;
    struct { int id, tgt; uint32_t t, t0; uint8_t har, back; ivec2 at, tat, from; int home; } held[AS_BOMBERS]; int nh;   // the bombers held, their targets (serials; 0 none), when ordered, when launched at it, on a lone run, where last seen flying, the lone run's target, start, Airport (serial) and whether it was called back
    struct { ivec2 at; uint32_t t; } flak[AH_FLAK]; int flak_i;     // where bombers on lone runs were lost (and their targets), when
    unsigned held_looks, idle_looks, lost, lost_har;                // bombers held at each look and those of them with no goal; bombers lost, on a lone run
    unsigned har_runs, har_sent, har_down, har_none, har_back;                // lone runs flown, bombers sent on them, their targets down when they came back, looks with bombers free and nothing undefended, bombers called back
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
           (ar_on(pl, idx) || !GAME_AI(idx)) && current_epoch(pl) + 1 >= 10;   // the game's computer player off: the territories' pushes are its positions (below)
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
    // the game's computer player off (the player 2026-11-15: "the navel port and airports arent build yet"): nobody works
    // the build manager's queue - build.c lays it (g_bd_airport: asked when), no count of tries
    if (!GAME_AI(idx)) { if (ap + building < ap_want && ut_has(UT(B_AIRPORT))) { g_bd_airport[idx] = now ? now : 1; g_bd_airport_n[idx] = ap_want; } }
    else
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
// the shooters: what hit its units from afar at intel.c's last scan, joined into spots; the one hit from most often
// (-1 none)
internal int as_shooters(unsigned idx, struct as_st *s, uint32_t now) {
    for (int k = 0; k < g_rev[idx].nsh; k++) {
        if (g_rev[idx].sh[k].bld || g_rev[idx].sh[k].d < (g_rev[idx].sh[k].gun ? AF_GUN_D : AF_SHOOT_D)) continue;
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
    return shoot;
}
// THE LONE RUNS: what shoots at aircraft - AA buildings and units, cruisers (navy.c NR_AA), carriers, fighters
internal int ah_aa(ee_unit_type *td) {
    int f = unit_type_get_family(td), c = unit_type_get_class_id(td);
    return f == KIND_AA_BUILDING || f == KIND_LAND_AA || f == KIND_AIRCRAFT_CARRIER || (c == CLS_AIRPLANE && f == KIND_AIRCRAFT && unit_type_get_attack(td) > 0) ||
           ((c == CLS_NAVAL || c == CLS_SUBMERGED_NAVAL) && nv_role(td) == NR_AA);
}
internal float ah_aa_reach(ee_unit_type *td) {   // its reach over the way: its range and AH_MARGIN; a fighter's AH_FIGHTER_R
    if (unit_type_get_class_id(td) == CLS_AIRPLANE) return (float)AH_FIGHTER_R;
    float r = unit_type_get_range(td) + (float)AH_MARGIN;
    return r < (float)AH_AA_MIN ? (float)AH_AA_MIN : r;
}
internal long ah_seg_d2(ivec2 a, ivec2 b, ivec2 p) {   // the squared distance of p to the way a - b
    float dx = (float)(b.x - a.x), dy = (float)(b.y - a.y), l = dx * dx + dy * dy, t = 0;
    if (l > 0) { t = ((float)(p.x - a.x) * dx + (float)(p.y - a.y) * dy) / l; t = t < 0 ? 0 : t > 1 ? 1 : t; }
    float ex = (float)a.x + t * dx - (float)p.x, ey = (float)a.y + t * dy - (float)p.y;
    return (long)(ex * ex + ey * ey);
}
// the ground within AH_LOOK_R of at looked at within AH_LOOKED (its middle and 8 points round it): what stands there is
// known; always with honest_vision off (the game's view)
internal int ah_looked(ee_player *pl, ivec2 at) {
    static const signed char k_ring[9][2] = { { 0, 0 }, { 10, 0 }, { 7, 7 }, { 0, 10 }, { -7, 7 }, { -10, 0 }, { -7, -7 }, { 0, -10 }, { 7, -7 } };
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0;
    for (int k = 0; k < 9; k++) {
        int x = at.x + k_ring[k][0] * AH_LOOK_R / 10, y = at.y + k_ring[k][1] * AH_LOOK_R / 10;
        if (x < 0 || y < 0 || x >= size || y >= size) continue;   // off the map: nothing stands there
        if (intel_looked_ago(pl, x, y) > AH_LOOKED) return 0;
    }
    return 1;
}
// ... and the way there, a point every AH_WAY_STEP tiles
internal int ah_way_looked(ee_player *pl, ivec2 a, ivec2 b) {
    float d = sqrtf((float)ivec2_dist2(a, b));
    for (float t = (float)AH_WAY_STEP; t < d; t += (float)AH_WAY_STEP)
        if (intel_looked_ago(pl, a.x + (int)((float)(b.x - a.x) * t / d), a.y + (int)((float)(b.y - a.y) * t / d)) > AH_LOOKED) return 0;
    return 1;
}
// a held bomber free for a run: not on a lone run any more (one back on the ground ends it), no wing target standing, no
// goal, AF_READY_PCT of its hit points
internal int ah_free(struct as_st *s, int e, ee_unit *u, uint32_t now, int *har_out) {
    ee_goal *g = unit_get_goal(u);
    if (s->held[e].har) {
        if (g && now - s->held[e].t0 < AH_RUN_MS) { (*har_out)++; return 0; }
        ee_unit *t = s->held[e].tgt ? unit_by_id((unsigned)s->held[e].tgt) : NULL;
        if (!t || unit_get_serial(t) != s->held[e].tgt || unit_get_hp(t) <= 0) s->har_down++;
        s->held[e].har = 0; s->held[e].tgt = 0;
        cl_end(u, CLW_BOMBERS);
    }
    if (s->held[e].tgt) {
        ee_unit *t = unit_by_id((unsigned)s->held[e].tgt);
        if (t && unit_get_serial(t) == s->held[e].tgt && unit_get_hp(t) > 0) return 0;   // the wing's, its target stands
        s->held[e].tgt = 0;
    }
    return !g && unit_get_hp(u) * 100 >= unit_type_get_hp(unit_get_type_data(u)) * AF_READY_PCT;
}
// the Airport a bomber is based at or stands in (as airguard.c THE AIR BASES reads it): its index in ports, -1 none
internal int ah_port(ee_unit *u, void *ports[][2], int nport) {
    void *h = unit_get_air_traffic(u), *h2 = unit_get_x(u) < 0 ? unit_get_holder(u) : NULL;
    for (int k = 0; k < nport; k++)
        if ((h && (h == ports[k][0] || h == ports[k][1])) || (h2 && (h2 == ports[k][0] || h2 == ports[k][1]))) return k;
    return -1;
}
// the lone runs: the bombers free while the wing gathers (or flies, or has nothing to do) go in flights of AH_FLIGHT, by
// their Airport, at what the AI knows of its enemies (buildings, citizens at work) with no anti-air it knows of near it
// or near the way there - the most worth for the way; one flight a target at a time. Back home they are free again
// (the wing takes them when it is ready to go: the caller sends none while the free and those out make the wing)
internal void ah_runs(ee_player *pl, unsigned idx, struct as_st *s, ee_unit **ready, int nr, uint32_t now) {
    static struct { ivec2 at; long r2; } aa[AH_AA];
    static struct { ee_unit *u; ivec2 at; float v; } tg[AH_TARGETS];
    static ee_unit *cit[AH_CIT];
    static ivec2 port[16]; static void *port_u[16][2];
    if (nr <= 0 || !AS_HARASS(idx)) return;
    int naa = 0, ntg = 0, ncit = 0, nport = 0, fighters[16] = { 0 }, has_aa[16] = { 0 };
    static ee_unit *towns[64]; int ntown = 0, nbld = 0;
    static struct { ivec2 at; int8_t o; } bld[AH_BLD];
    static uint8_t cover[AH_CELLS][AH_CELLS];   // THE FIGHTERS' COVER: cells within AH_FIGHTER_R of a building of an enemy with an Airport or fighters
    // what it knows: enemy units seen (buildings remembered), and with honest_vision the aircraft, ships and soldiers it
    // saw within AH_MEM where it saw them
    for (int o = 1; o < nplayers(); o++) {
        ee_player *op = player_at(o);
        if (!op || !is_enemy(pl, op)) continue;
        int count = 0;
        for (ee_unit *u = player_get_units(op); u && count < 16384; u = unit_get_next(u), count++) {
            ee_unit_type *td = unit_get_type_data(u);
            if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !seen_by(u, pl)) continue;
            int c = unit_type_get_class_id(td), t = unit_type_get_id(td);
            if (ah_aa(td)) {
                has_aa[o & 15] = 1;
                if (c == CLS_AIRPLANE) fighters[o] = 1;
                if (naa < AH_AA) { float r = ah_aa_reach(td); aa[naa].at = unit_get_tile(u); aa[naa++].r2 = (long)(r * r); }
                continue;
            }
            if (t == UT(B_AIRPORT)) fighters[o & 15] = 1;   // it has or can have fighters: they rise at what comes near its buildings (airguard.c INTERCEPT)
            if ((t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL) || t == UT(B_SETTLEMENT)) && ntown < 64) towns[ntown++] = u;
            if (c == CLS_BUILDING || c == CLS_FORTRESS) {
                if (nbld < AH_BLD) { bld[nbld].at = unit_get_tile(u); bld[nbld++].o = (int8_t)o; }
                if (is_wall(t) || ntg >= AH_TARGETS) continue;
                int cost = 0;
                for (int r = 2; r < 7; r++) cost += unit_cost(pl, t, r);
                tg[ntg].u = u; tg[ntg].at = unit_get_tile(u); tg[ntg++].v = (float)(cost < 800 ? cost : 800) + 100.0f;
            } else if (is_citizen_type(t) && ncit < AH_CIT) cit[ncit++] = u;
        }
    }
    struct intel *in = &g_intel[idx];
    for (int k = 0; g_honest_vision && k < in->n && naa < AH_AA; k++) {
        struct intel_rec *e = &in->r[k];
        ee_unit_type *td = e->kind ? type_data(e->type) : NULL;
        if (!td || in->now - e->t > AH_MEM || !ah_aa(td)) continue;
        if (e->owner > 0 && e->owner < 16) { has_aa[e->owner] = 1; if (unit_type_get_class_id(td) == CLS_AIRPLANE) fighters[e->owner] = 1; }
        if (e->x < 0) continue;
        float r = ah_aa_reach(td);
        aa[naa].at = iv2(e->x, e->y); aa[naa++].r2 = (long)(r * r);
    }
    for (int k = 0; k < ntown && naa < AH_AA; k++) {   // an enemy that has anti-air has it by its towns: assumed there unless looked at lately
        ee_player *o = unit_get_owner(towns[k]);
        ivec2 p = unit_get_tile(towns[k]);
        if (o && has_aa[player_get_index(o) & 15] && intel_looked_ago(pl, p.x, p.y) > AH_LOOKED) { aa[naa].at = p; aa[naa++].r2 = (long)AH_FLAK_R * AH_FLAK_R; }
    }
    for (int k = 0; k < AH_FLAK && naa < AH_AA; k++)   // where bombers on lone runs were lost
        if (s->flak[k].t && now - s->flak[k].t < AH_MEM) { aa[naa].at = s->flak[k].at; aa[naa++].r2 = (long)AH_FLAK_R * AH_FLAK_R; }
    memset(cover, 0, sizeof cover);
    for (int k = 0; k < nbld; k++) {
        if (!fighters[bld[k].o & 15]) continue;
        int r = (AH_FIGHTER_R + AH_CELL) / AH_CELL, cx = bld[k].at.x / AH_CELL, cy = bld[k].at.y / AH_CELL;
        for (int x = cx - r; x <= cx + r; x++)
            for (int y = cy - r; y <= cy + r; y++)
                if (x >= 0 && y >= 0 && x < AH_CELLS && y < AH_CELLS && ivec2_dist2(iv2(x * AH_CELL + AH_CELL / 2, y * AH_CELL + AH_CELL / 2), bld[k].at) <= (AH_FIGHTER_R + AH_CELL) * (AH_FIGHTER_R + AH_CELL)) cover[x][y] = 1;
    }
    // citizens at work: two or more of an owner within 4 tiles, the one with the most beside it
    for (int i = 0; i < ncit && ntg < AH_TARGETS; i++) {
        ivec2 p = unit_get_tile(cit[i]);
        int n = 0;
        for (int j = 0; j < ncit; j++) if (unit_get_owner(cit[j]) == unit_get_owner(cit[i]) && ivec2_dist2(unit_get_tile(cit[j]), p) <= 16) n++;
        if (n >= 2) { tg[ntg].u = cit[i]; tg[ntg].at = p; tg[ntg++].v = 120.0f * (float)n; }
    }
    // its Airports: where a flight starts
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    void *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    uint8_t *q = um ? type_list(um, UT(B_AIRPORT)) : NULL;
    for (unsigned j = 0, f = q ? deque_front(q) : 0, c = q ? deque_count(q) : 0; j < c && nport < 16; j++) {
        ee_unit *b = deque_at(q, f + j);
        if (!b || unit_get_hp(b) <= 0 || unit_get_x(b) < 0) continue;
        port[nport] = unit_get_tile(b); port_u[nport][0] = b; port_u[nport][1] = unit_get_physics(b); nport++;
    }
    static uint8_t taken[AH_TARGETS];
    memset(taken, 0, (size_t)ntg);
    for (int k = 0; k < ntg; k++)   // one flight a target: not one a run is on already
        for (int j = 0; j < s->nh && !taken[k]; j++) taken[k] = s->held[j].har && s->held[j].tgt == unit_get_serial(tg[k].u);
    int runs = 0, none = 0;
    for (int i = 0; i < nr; ) {
        ee_unit *u = ready[i];
        ivec2 from = unit_get_tile(u);   // its Airport: the one it is based at or stands in, else where it is
        int pk = ah_port(u, port_u, nport);
        if (pk >= 0) from = port[pk];
        else if (unit_get_x(u) < 0) { if (!nport) { i++; continue; } from = port[0]; }
        int best = -1; float bv = 0;
        for (int k = 0; k < ntg; k++) {
            if (taken[k]) continue;
            float d = sqrtf((float)ivec2_dist2(from, tg[k].at)), v = tg[k].v / (30.0f + d);
            if (best >= 0 && v <= bv) continue;
            ee_player *to = unit_get_owner(tg[k].u);
            int oi = to ? player_get_index(to) & 15 : 0;
            int open = to && !fighters[oi] && ah_looked(pl, tg[k].at) && (!has_aa[oi] || ah_way_looked(pl, from, tg[k].at));
            for (float t = 0, dd = d > 1 ? d : 1; open && t <= dd; t += (float)AH_CELL / 2) {   // the way clear of the fighters' cover
                int x = (from.x + (int)((float)(tg[k].at.x - from.x) * t / dd)) / AH_CELL, y = (from.y + (int)((float)(tg[k].at.y - from.y) * t / dd)) / AH_CELL;
                if (x >= 0 && y >= 0 && x < AH_CELLS && y < AH_CELLS && cover[x][y]) open = 0;
            }
            for (int a = 0; a < naa && open; a++) if (ah_seg_d2(from, tg[k].at, aa[a].at) <= aa[a].r2) open = 0;
            if (open) { best = k; bv = v; }
        }
        if (best < 0) { none = 1; break; }
        taken[best] = 1;
        int sent = 0;
        for (int n = 0; n < AH_FLIGHT && i < nr; n++, i++) {
            if (n && ah_port(ready[i], port_u, nport) != pk) break;   // a flight from one Airport
            int e = as_held_at(s, unit_get_serial(ready[i]));
            if (e < 0 || !cl_take(ready[i], CLP_TASK, CLW_BOMBERS, AH_RUN_MS)) continue;   // ORDERS BY PRIORITY (claims.c): nobody else's on the run
            if (!as_order(ready[i], goal_attack(ready[i], tg[best].u))) { cl_end(ready[i], CLW_BOMBERS); continue; }
            s->held[e].tgt = unit_get_serial(tg[best].u); s->held[e].t = s->held[e].t0 = now ? now : 1; s->held[e].har = 1; s->held[e].tat = tg[best].at;
            s->held[e].from = from; s->held[e].home = pk >= 0 ? unit_get_serial((ee_unit *)port_u[pk][0]) : 0; s->held[e].back = 0;
            sent++;
        }
        if (!sent) continue;
        runs++; s->har_runs++; s->har_sent += (unsigned)sent;
        ee_unit_type *ttd = unit_get_type_data(tg[best].u);
        ar_log(idx, now, "air force: a lone run of %d at type %d at %d,%d (from %d,%d%s, %d tiles; anti-air known %d, undefended targets %d)", sent, ttd ? unit_type_get_id(ttd) : -1,
               tg[best].at.x, tg[best].at.y, from.x, from.y, pk >= 0 ? "" : unit_get_x(u) < 0 ? " - not in its Airport's list" : " - in the air", (int)sqrtf((float)ivec2_dist2(from, tg[best].at)), naa, ntg);
    }
    if (none && !runs) s->har_none++;
}
// a bomber on a lone run that went after something else (the plane picks its next target by itself - they went on
// past theirs into the anti-air: of 30 lost, most 10-30 tiles beyond their target or off the way, by anti-air it had
// seen), whose target is down, or AH_STRAY off the way: called back to its Airport, as a player does (the game's
// EEMReturnToBase, airguard.c THE AIR BASES), once
internal void ah_watch(ee_player *pl, struct as_st *s) {
    static __attribute__((aligned(8))) uint8_t mem[0x40];
    for (int k = 0; k < s->nh; k++) {
        if (!s->held[k].har || s->held[k].back || !s->held[k].home) continue;
        ee_unit *u = unit_by_id((unsigned)s->held[k].id);
        if (!u || unit_get_serial(u) != s->held[k].id || unit_get_x(u) < 0 || unit_get_flight_ms(u) <= 0) continue;
        ee_unit *t = unit_by_id((unsigned)s->held[k].tgt), *at = fl_target_of(u);
        int down = !t || unit_get_serial(t) != s->held[k].tgt || unit_get_hp(t) <= 0;
        int other = at && at != t, stray = ah_seg_d2(s->held[k].from, s->held[k].tat, unit_get_tile(u)) > (long)AH_STRAY * AH_STRAY;
        if (!down && !other && !stray) continue;
        uint32_t id1 = (uint32_t)s->held[k].id, *sel[3] = { NULL, &id1, &id1 + 1 };
        msg_as_select(pl, sel);
        memset(mem, 0, sizeof mem);
        rebase_msg_make((ee_rebase_msg *)mem, msg_as_begin(pl), (unsigned)s->held[k].home);
        rebase_msg_set_keep((ee_rebase_msg *)mem, 0);   // turns for it at once
        msg_execute((ee_msg *)mem);
        msg_as_end(pl);
        s->held[k].back = 1; s->har_back++;
        ee_unit_type *atd = at ? unit_get_type_data(at) : NULL;
        ar_log((unsigned)player_get_index(pl) & 15, game_time(), "air force: a bomber on a lone run called back at %d,%d (%s; its target at %d,%d, its own attack type %d at %d,%d)", unit_get_tile(u).x, unit_get_tile(u).y,
               down ? "its target down" : other ? "it went after something else" : "off the way", s->held[k].tat.x, s->held[k].tat.y, atd ? unit_type_get_id(atd) : -1,
               at ? unit_get_tile(at).x : -1, at ? unit_get_tile(at).y : -1);
    }
}
// no position for the wing: every free bomber to the lone runs
internal void ah_idle(ee_player *pl, unsigned idx, struct as_st *s, ee_unit **bomb, int nb, uint32_t now) {
    static ee_unit *ready[AS_BOMBERS];
    int nr = 0, har_out = 0;
    if (!AS_HARASS(idx)) return;
    for (int i = 0; i < nb; i++) {
        int e = as_held_at(s, unit_get_serial(bomb[i]));
        if (e >= 0 && ah_free(s, e, bomb[i], now, &har_out)) ready[nr++] = bomb[i];
    }
    ah_runs(pl, idx, s, ready, nr, now);
}
internal void airstrike_tick(ee_player *pl, uint32_t now) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct as_st *s = &g_as[idx];
    if (!ar_on(pl, idx) && (!pl || is_vanilla(pl) || GAME_AI(idx))) return;
    if (s->t && now >= s->t && now - s->t < AS_EVERY) return;
    if (now < s->t) memset(s, 0, sizeof *s);
    s->t = now ? now : 1;
    int shoot = as_shooters(idx, s, now);
    if (!PV(idx, g_v_air_strike, AS_ON) || current_epoch(pl) + 1 < 10) return;   // no bombers before World War I
    if (g_log && g_autotest && (!s->log_t || now - s->log_t >= 60000)) {
        s->log_t = now ? now : 1;
        fprintf(g_log, "# air force: P%u at %.1f min: %d bombers held, %u trained, %u airports queued, %u waves (%u bombers sent), %u launches waited for more, %u targets given up | "
                "idle %.1f of %.1f bomber-minutes held, %u lost (%u on lone runs), lone runs %u (%u bombers, %u targets down), %u looks with none undefended, %u called back\n",
                idx, now / 60000.0, s->nh, s->trained, s->airports, s->waves, s->sent, s->waits, s->gave_up,
                s->idle_looks * (AS_EVERY / 60000.0), s->held_looks * (AS_EVERY / 60000.0), s->lost, s->lost_har, s->har_runs, s->har_sent, s->har_down, s->har_none, s->har_back);
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
        if (u && unit_get_serial(u) == s->held[k].id && unit_get_hp(u) > 0 && unit_get_owner(u) == pl) {
            if (unit_get_x(u) >= 0) s->held[k].at = unit_get_tile(u);
            s->held[m++] = s->held[k];
            s->held_looks++; s->idle_looks += !unit_get_goal(u);   // idle: no goal at the look (landed - rearmed, repaired or waiting)
        } else {
            s->lost++; s->lost_har += s->held[k].har;
            if (s->held[k].har) {   // and the nearest anti-air there (what it knew of it: seen)
                int ad = -1, at = -1, as = 0;
                for (struct grid_it it = grid_it_box(s->held[k].at.x, s->held[k].at.y, 16, 1); grid_it_next(&it); ) {
                    ee_player *o = unit_get_owner(it.u);
                    if (!o || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || !ah_aa(it.td)) continue;
                    int d = (int)ivec2_dist2(unit_get_tile(it.u), s->held[k].at);
                    if (ad < 0 || d < ad) { ad = d; at = unit_type_get_id(it.td); as = seen_by(it.u, pl); }
                }
                ar_log(idx, now, "air force: a bomber on a lone run lost - last seen at %d,%d, its target at %d,%d, %.1f min after it went; the nearest anti-air there type %d at %d tiles (seen %d)",
                       s->held[k].at.x, s->held[k].at.y, s->held[k].tat.x, s->held[k].tat.y, (now - s->held[k].t0) / 60000.0, at, ad < 0 ? -1 : (int)sqrtf((float)ad), as);
            }
            for (int j = 0; s->held[k].har && j < 2; j++) {   // THE LONE RUNS: shot down by something it didn't know of - kept clear of for AH_MEM
                s->flak[s->flak_i].at = j ? s->held[k].tat : s->held[k].at; s->flak[s->flak_i].t = now ? now : 1;
                s->flak_i = (s->flak_i + 1) % AH_FLAK;
            }
        }
    }
    s->nh = m;
    for (int i = 0; i < nb; i++) {
        int id = unit_get_serial(bomb[i]);
        if (as_held_at(s, id) >= 0 || s->nh >= AS_BOMBERS) continue;
        if (!unit_get_ai_held(bomb[i])) player_hold_unit(pl, bomb[i]);
        s->held[s->nh].id = id; s->held[s->nh].tgt = 0; s->held[s->nh].t = 0; s->held[s->nh].har = 0; s->held[s->nh].at = unit_get_tile(bomb[i]); s->held[s->nh].tat = iv2(0, 0); s->nh++;
        if (s->asked > 0) s->asked--;   // one of those asked for is out
    }
    af_train(pl, idx, s, nb, now);
    if (AS_HARASS(idx)) ah_watch(pl, s);
    // the position
    struct armies_st *a = &g_armies[idx];
    ivec2 pos;
    if (shoot >= 0) pos = s->sh[shoot].at;
    else if (g_ar_breach[idx].on) pos = g_ar_breach[idx].focus;
    else if (a->out == 1 && a->target) pos = a->dest;
    else if (a->strong_t && now - a->strong_t < AF_STRONG_MS) pos = a->strong;
    else if (!GAME_AI(idx) && sn_need_at(idx, &pos)) { }   // stations.c: a push's building, the enemy in a territory of its, the nearest enemy building
    else { ah_idle(pl, idx, s, bomb, nb, now); return; }
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
        int lr = fam == CLS_LAND && (sh_gun(it.td) || rg >= (float)AF_SHOOT_D);
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
    if (!na && !nt) { if (shoot >= 0) s->sh[shoot].t = 0; ah_idle(pl, idx, s, bomb, nb, now); return; }   // nothing left at the shooters' spot: the next
    s->needed = AF_FLIGHT * (na + (nt ? 1 : 0)); s->need_t = now ? now : 1;
    // the wing out: a bomber whose target stands keeps at it; idle again, its next run
    static ee_unit *ready[AS_BOMBERS];
    int nr = 0, out = 0, har_out = 0;
    for (int i = 0; i < nb; i++) {
        ee_unit *u = bomb[i];
        int e = as_held_at(s, unit_get_serial(u));
        if (e < 0) continue;
        if (s->held[e].har) {   // a lone run's: free once back (not the wing's out)
            if (!ah_free(s, e, u, now, &har_out)) continue;
            ready[nr++] = u;
            continue;
        }
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
    if (out) { ah_runs(pl, idx, s, ready, nr, now); return; }   // one wing at a time (the lone runs meanwhile)
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
            ar_log(idx, now, "air force: the wing at %d,%d waits - %d bombers ready of the %d it needs (anti-air %d, defences %d; %d bombers in all, %d on lone runs)", pos.x, pos.y, nr, s->needed, na, nt, nb, har_out);
        }
        if (nr + har_out < s->needed && nr + har_out < AF_WING_MAX) ah_runs(pl, idx, s, ready, nr, now);   // else those out come back to make it
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
    g_as_harass = g_set_ai[1].bomber_harass;
    if (autotest_ini) { g_v_air_strike = ini_int("autotest", "v_air_strike", -1, autotest_ini); g_v_as_harass = ini_int("autotest", "v_bomber_harass", -1, autotest_ini); }
    if (g_log) fprintf(g_log, "# air force: %d (variant %d), lone runs %d (variant %d)\n", AS_ON, g_v_air_strike, g_as_harass, g_v_as_harass);
}
