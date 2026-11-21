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
// THE DOCTRINE ([ai_base] bomber_doctrine, on since 2026-11-21 - below; its parts bomber_survive, bomber_come_home, bomber_value_wing,
// bomber_claim, on under it; [autotest] v_bomber_doctrine, v_bomber_survive ...). The player, 2026-11-21, after the lone
// runs above: "undefended doesn't mean fully undefended here, just not enough to bring the bomber down on its own", "a
// damaged bomber isn't a loss at all as long as it can return", "the wings are meant to overwhelm in numbers so that
// even a few lost bombers still turn a net positive"; on counter-fire: "killing mortars and artillery behind lines is a
// good use for bombers, but it should be a deliberate task, not stolen from other running tasks".
// - THE PICTURE (every AS_EVERY, dct_scan): what of the enemy can hit its bombers by the game's own numbers (stats.c: a
//   shot of the unit's attack at the bomber type held most, by the hit table and armour, over its reload, x AD_HIT - the
//   share of it that lands, measured), with its reach (range, AD_PAD, a mover's AD_CHASE_S of its speed): the guns seen
//   (buildings remembered), with honest_vision the moving ones seen within AD_MEM_MOB where seen (their reach grown by
//   their drift, their fire fading), one of an enemy's strongest guns by each of its towns not looked at within
//   AH_LOOKED (and at a run's target), the strongest known where a bomber on a run was lost; an enemy's fighters (seen,
//   remembered, AD_FIGHTERS assumed with an Airport or carrier) as a cover AH_FIGHTER_R round its buildings - each a
//   fight of AD_FIGHT_S at most, its ammunition first. A way's damage: each gun's fire for the time the way is in its
//   reach at the bomber's speed, the fighters' for the time in their cover, all of it on the one bomber (as guns aim).
// - THE SMALL RUNS (bomber_survive; in place of the lone runs' no-anti-air rule): 1 to AD_FLIGHT from one Airport, as
//   many as the target's hit points take, the soundest first, at the target worth most for the way (buildings, working
//   citizens, counter-fire's shooters) whose damage there, over it and home x AD_MARGIN, with AD_RESERVE of a bomber's
//   hit points left, is below the weakest one's hit points now.
// - THE WAY HOME (bomber_come_home, dct_home, every second): above.
// - THE VALUE WING (bomber_value_wing; in place of the count): the positions - the force's, AD_PUSH_W more, and the
//   enemy's buildings by AD_CELL cell, the AD_CANDS worth most - weighed for each number of the bombers ready: the worth
//   they destroy (prices; an AA gun one bomber more; a target they can't destroy AD_PARTIAL of the damage) less the
//   bombers expected lost (each gun firing as long as any of them is in its reach, so numbers share its fire; x
//   AD_MARGIN, one lost a hit point total); launched at once where the gain is highest, no more than AD_LOSS_MAX of them
//   expected lost, and the rest fly the small runs. Trained for: the fewest that would gain at the force's position.
// - NO THEFT (bomber_claim): a bomber on any task of the air force is claimed (claims.c CLW_BOMBERS at CLP_TASK) and
//   counter-fire takes no bombers as soldiers; it asks the air force about its shooters instead (af_request): AD_CF_W
//   their price as targets of the small runs for AD_CF_MS.
// "# bombers:" a minute (every player, the doctrine on or not): the bombers lost by where (on the wing, a run, called
// home, otherwise in the air, in an Airport), the sorties (out of an Airport and back), back with damage, the damage
// taken and expected, repaired, the targets of its bombers destroyed (anti-air, buildings, units, worth), the guns'
// fire expected on bombers in the air each second against the hit points they lost (by what it knew), counter-fire's
// requests and the runs at them.
// MEASURED 2026-11-21, head-to-head, 4 AIs from World War I (start_epoch 9), 60 min, the air_rich money (3000 of each at
// 10, 25, 40 min), seeds 1-10 mirrored, 20 games a map, B with the doctrine (B - A, se):
//   Continental 101 map_size 1: alive +0.17 (0.10), soldiers +20.0 (7.7), kills +46.8 (30.1), losses -8.2 (13.8), razed
//   +11.8 (9.2), workers +22.1 (14.1); per AI B / A: bombers trained 16.9 / 16.6, lost 13.0 / 11.4 (+1.6, 1.7), back with
//   damage 3.3 / 1.2, anti-air destroyed by its bombers 0.90 / 0.05, buildings 1.68 / 0.10, worth 886 / 212 (+674, 159),
//   idle share of the bomber-minutes held 0.69 / 0.83 (-0.13, 0.05).
//   Large Islands 101 map_size 1: alive +0.00, soldiers +12.7 (4.8), kills +7.9 (8.0), losses +0.3 (8.5), razed +2.8
//   (3.0), workers -1.2 (2.1); bombers lost 10.0 / 11.7 (-1.7, 1.8), anti-air destroyed 0.68 / 0.00, buildings 1.70 /
//   0.75, worth 730 / 202 (+528, 94), idle share 0.62 / 0.79 (-0.18, 0.04); counter-fire's shooters destroyed 1.45 an AI.
//   One part out (Continental, seeds 1-5 mirrored, 10 games, A the whole): without the small runs the idle share +0.20
//   (0.06), the games within the noise; without the value wing anti-air destroyed -0.80 (0.36), buildings -0.9 (0.6);
//   without the claim and without the way home within the noise (the way home: above). ON: the player's doctrine, the
//   bombers busier and destroying more, the games no worse.
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
// THE DOCTRINE ([ai_base] bomber_doctrine, its parts bomber_survive, bomber_come_home, bomber_value_wing, bomber_claim;
// [autotest] v_bomber_doctrine, v_bomber_survive ... - below)
internal int g_bd_on = 0, g_v_bd_on = -1, g_bd_part[4] = { 1, 1, 1, 1 }, g_v_bd_part[4] = { -1, -1, -1, -1 };
enum { DCP_SURVIVE, DCP_HOME, DCP_WING, DCP_CLAIM };
internal int dct_part(unsigned idx, int p) { idx &= 15; return PV(idx, g_v_bd_on, g_bd_on) && PV(idx, g_v_bd_part[p], g_bd_part[p]); }
internal int dct_claims_bombers(unsigned idx) { return dct_part(idx, DCP_CLAIM); }   // counterfire.c: its bombers are no soldiers of its
// counterfire.c's requests: shooters that shell its units from beyond their reach - a strike asked of the air force (THE
// SMALL RUNS weigh them as any target, at AD_CF_W their worth: the player, 2026-11-21: "killing mortars and artillery
// behind lines is a good use for bombers, but it should be a deliberate task, not stolen from other running tasks")
#define AD_CF_MS 30000
#define AD_CF_W 2.0f
#define AS_RUNS(idx) (AS_HARASS(idx) || dct_part(idx, DCP_SURVIVE))   // lone runs: the old rule or the estimate's
internal struct as_st {
    uint32_t t, log_t, train_t, airport_t, wait_log;
    struct { int id, tgt; uint32_t t, t0; uint8_t har, back; ivec2 at, tat, from; int home;   // the bombers held, their targets (serials; 0 none), when ordered, when launched at it, on a lone run, where last seen flying, the lone run's target, start, Airport (serial) and whether it was called back
             short ttype; uint8_t out, home2, landed, nohome, cf; int hp0, hp_last, hp_sec; float pred, here_prev, here_k[5]; } held[AS_BOMBERS]; int nh;   // THE DOCTRINE's books: its target's type; out on a sortie (in the air since it last stood in an Airport), called home, in an Airport at the last look, told it can't make it home; its hit points when it went out and at the last look, the damage expected on the sortie
    unsigned lost_wing, lost_home, lost_air, lost_ground, sorties, dmg_back, called_home, doomed_on;   // bombers lost on the wing, called home, in the air otherwise, in an Airport; sorties flown, back with damage, called home, gone on (no way home)
    unsigned down_aa, down_bld, down_unit, down_val, ev_launch;
    unsigned cf_asked, cf_runs, cf_sent, cf_down, cf_val;           // counter-fire's requests (shooters asked about), the runs at them, bombers sent, shooters destroyed and their worth
    struct { ee_unit *u; int id; uint32_t t; } cf[16]; int cf_i;    // the shooters counter-fire asked the air force to strike, when     // targets of its bombers destroyed - anti-air, buildings, units - and their worth; wings launched by their worth
    float dmg_taken, dmg_pred, repaired, obs_taken, obs_pred, obs_k[5]; float best_ev; int down_ids[64], down_i;   // damage taken on sorties and expected, hit points repaired in the Airports; the best worth of a wing at the last look; targets counted down
    ivec2 wing_pos; int wing_push; uint32_t home_t, ev_log;           // the value wing's position, whether it is the force's; the come-home look, the wing's wait line
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
// THE DOCTRINE's books: a target of its bombers gone while they were on it - counted once (anti-air, buildings, units
// and their worth)
internal void dct_down(struct as_st *s, ee_player *pl, int id, int type) {
    for (int k = 0; k < 64; k++) if (s->down_ids[k] == id) return;
    s->down_ids[s->down_i] = id; s->down_i = (s->down_i + 1) & 63;
    ee_unit_type *td = (unsigned)type < G_TYPES_N ? type_data(type) : NULL;
    if (!td) return;
    int f = unit_type_get_family(td), c = unit_type_get_class_id(td), v = 0;
    if (f == KIND_AA_BUILDING || f == KIND_LAND_AA) s->down_aa++; else if (c == CLS_BUILDING || c == CLS_FORTRESS) s->down_bld++; else s->down_unit++;
    for (int r = 2; r < 7; r++) v += unit_cost(pl, type, r);
    s->down_val += (unsigned)(v > 0 ? v : 0);
}
// a held bomber free for a run: not on a lone run any more (one back on the ground ends it), no wing target standing, no
// goal - 1 with AF_READY_PCT of its hit points, 2 with less (THE DOCTRINE's runs weigh its hit points), 0 not free
internal int ah_free(struct as_st *s, int e, ee_unit *u, uint32_t now, int *har_out) {
    ee_goal *g = unit_get_goal(u);
    if (s->held[e].har) {
        if (g && now - s->held[e].t0 < AH_RUN_MS) { (*har_out)++; return 0; }
        ee_unit *t = s->held[e].tgt ? unit_by_id((unsigned)s->held[e].tgt) : NULL;
        if (!t || unit_get_serial(t) != s->held[e].tgt || unit_get_hp(t) <= 0) {
            s->har_down++; dct_down(s, unit_get_owner(u), s->held[e].tgt, s->held[e].ttype);
            if (s->held[e].cf) {   // a shooter counter-fire asked for: counted once (the others of its run no more)
                int v = 0;
                for (int r = 2; r < 7; r++) v += unit_cost(unit_get_owner(u), s->held[e].ttype, r);
                s->cf_down++; s->cf_val += (unsigned)(v > 0 ? v : 0);
                for (int j = 0; j < s->nh; j++) if (s->held[j].har && s->held[j].tgt == s->held[e].tgt) s->held[j].cf = 0;
            }
        }
        s->held[e].har = 0; s->held[e].tgt = 0; s->held[e].cf = 0;
        cl_end(u, CLW_BOMBERS);
    }
    if (s->held[e].tgt) {
        ee_unit *t = unit_by_id((unsigned)s->held[e].tgt);
        if (t && unit_get_serial(t) == s->held[e].tgt && unit_get_hp(t) > 0) return 0;   // the wing's, its target stands
        if (!t || unit_get_serial(t) != s->held[e].tgt || unit_get_hp(t) <= 0) dct_down(s, unit_get_owner(u), s->held[e].tgt, s->held[e].ttype);
        s->held[e].tgt = 0;
    }
    if (g) return 0;
    return unit_get_hp(u) * 100 >= unit_type_get_hp(unit_get_type_data(u)) * AF_READY_PCT ? 1 : 2;
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
            s->held[e].ttype = (short)unit_type_get_id(unit_get_type_data(tg[best].u));
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
internal void as_rebase(ee_player *pl, int id, unsigned home) {   // the bomber back to its Airport home, at once (EEMReturnToBase)
    static __attribute__((aligned(8))) uint8_t mem[0x40];
    uint32_t id1 = (uint32_t)id, *sel[3] = { NULL, &id1, &id1 + 1 };
    msg_as_select(pl, sel);
    memset(mem, 0, sizeof mem);
    rebase_msg_make((ee_rebase_msg *)mem, msg_as_begin(pl), home);
    rebase_msg_set_keep((ee_rebase_msg *)mem, 0);   // turns for it at once
    msg_execute((ee_msg *)mem);
    msg_as_end(pl);
}
internal void ah_watch(ee_player *pl, struct as_st *s) {
    for (int k = 0; k < s->nh; k++) {
        if (!s->held[k].har || s->held[k].back || !s->held[k].home) continue;
        ee_unit *u = unit_by_id((unsigned)s->held[k].id);
        if (!u || unit_get_serial(u) != s->held[k].id || unit_get_x(u) < 0 || unit_get_flight_ms(u) <= 0) continue;
        ee_unit *t = unit_by_id((unsigned)s->held[k].tgt), *at = fl_target_of(u);
        int down = !t || unit_get_serial(t) != s->held[k].tgt || unit_get_hp(t) <= 0;
        int other = at && at != t, stray = ah_seg_d2(s->held[k].from, s->held[k].tat, unit_get_tile(u)) > (long)AH_STRAY * AH_STRAY;
        if (!down && !other && !stray) continue;
        as_rebase(pl, s->held[k].id, (unsigned)s->held[k].home);
        s->held[k].back = 1; s->har_back++;
        ee_unit_type *atd = at ? unit_get_type_data(at) : NULL;
        ar_log((unsigned)player_get_index(pl) & 15, game_time(), "air force: a bomber on a lone run called back at %d,%d (%s; its target at %d,%d, its own attack type %d at %d,%d)", unit_get_tile(u).x, unit_get_tile(u).y,
               down ? "its target down" : other ? "it went after something else" : "off the way", s->held[k].tat.x, s->held[k].tat.y, atd ? unit_type_get_id(atd) : -1,
               at ? unit_get_tile(at).x : -1, at ? unit_get_tile(at).y : -1);
    }
}
// ---- THE DOCTRINE -----------------------------------------------------------------------------------------------------
#define AD_THREATS 384
#define AD_MARGIN 1.5f        // the damage expected x this: what isn't seen (guns out of sight, fighters, shots that turn)
#define AD_RESERVE 0.10f      // ... and this share of its hit points left when it lands
#define AD_OVER_S 8.0f        // seconds over the target: the run in, its bombs, the turn away
#define AD_PAD 0.5f           // tiles beyond a gun's range it still hits (the plane's size)
#define AD_HIT 0.35f          // of a gun's shot a reload what lands on a bomber in its range (measured: 2026-11-21, 4 air_rich games,
                              // guns seen with the bombers in their range 58.6k expected at the full rate, 20.0k taken in all)
#define AD_CHASE_S 3.0f       // a gun that moves (land AA, ships) closes in for this long: its speed x this more reach
#define AD_DRIFT 6.0f         // a moving gun remembered: its reach grows by its speed x the time since, at most this
#define AD_MEM_MOB 90000      // ... and its fire fades to none over this long (it has moved on: remembered guns counted in full
                              // expected 5 times the damage the bombers took)
#define AD_FIGHT_S 20.0f      // a fighter at a bomber at most this long (its ammunition first)
#define AD_FIGHTERS 2         // fighters assumed of an enemy with an Airport (or carrier) and none seen
#define AD_FLIGHT 4           // a small run: at most this many from one Airport
#define AD_PUSH_W 1.5f        // the force's position is worth this much more: it opens the push's way
#define AD_CANDS 6            // other positions weighed for the wing: the enemy's buildings by AD_CELL cell, the most worth
#define AD_CELL 16
#define AD_CELLS (600 / AD_CELL + 1)
#define AD_HOME_EVERY 1000
#define AD_MARGIN_HOME 1.2f   // the way home's margin: below the launch's, a bomber breaks off when it fares worse than reckoned
#define AD_VAL_MAX 1500
#define AD_PARTIAL 0.5f       // a target damaged, not destroyed: this share of the damage's worth
#define AD_LOSS_MAX 0.4f      // a wing expected to lose more than this share of its bombers doesn't go
#define AD_NOAMMO 4           // a bomber type without ammunition: its shots a sortie
internal struct dct_st {
    struct dct_threat { ee_unit *u; ivec2 at; float r, dps; short type; uint8_t owner, kind; } t[AD_THREATS]; int n;   // kind 0 seen, 1 remembered, 2 assumed by a town not looked at, 3 where a bomber on a run was lost
    uint16_t cover[AH_CELLS][AH_CELLS];                              // the fighters' cover: a bit an enemy
    float nf[16], fsortie[16], fdps[16], aadps[16], aar[16];        // by enemy: fighters known (or assumed), one fighter's damage at a bomber and a second, its gun assumed where unseen (damage a second, reach)
    uint8_t has_aa[16], airport[16];
    short ftype[16], aatype[16]; float ftype_v[16], aatype_v[16];    // the strongest fighter and gun seen of each enemy, kept over the game
    short ref; float speed, maxhp; int cost, ammo;                  // the bomber type it is for (the most held): speed (tiles a second), hit points, price, shots a sortie
    struct dct_tgt { ee_unit *u; ivec2 at; float v; short type; uint8_t owner, cf; } tg[AH_TARGETS]; int ntg;   // targets known: buildings (not walls), citizens at work
    ivec2 port[16]; void *port_u[16][2]; int nport;                  // its Airports
    uint8_t logged[G_TYPES_N / 8];
} g_dct[16];
struct dct_acc { float dmg, tin[16]; };   // a sortie's damage from the guns, its time in each enemy's fighters' cover
struct as_tg { ee_unit *u; long d; };
// a gun's damage a second at the bomber type btd and its reach - 0 when it cannot hit it (the game's own numbers: stats.c)
internal float dct_dps(ee_unit_type *td, int oi, ee_unit_type *btd, ee_unit_stats *bst, int bi, float *reach) {
    ee_player *op = st_player(oi);
    int t = unit_type_get_id(td);
    ee_unit_stats *ast = op ? st_rec(op, t) : NULL;
    if (reach) *reach = 0;
    if (unit_type_get_attack(td) <= 0) return 0;
    int shot = st_shot_t(td, ast, oi, btd, bst, bi);
    if (shot <= 0) return 0;
    float sp = ast ? unit_stats_get_speed(ast) : 0.0f;
    if (reach) *reach = (ast ? unit_stats_get_range(ast) : unit_type_get_range(td)) + AD_PAD + (unit_type_get_class_id(td) != CLS_AIRPLANE && sp > 0 ? sp * AD_CHASE_S : 0.0f);
    return AD_HIT * (float)shot / st_reload_t(td, ast);
}
internal float dct_fighter(ee_unit_type *td, int oi, ee_unit_type *btd, ee_unit_stats *bst, int bi, float *dps) {   // a fighter's damage at a bomber in one fight
    *dps = dct_dps(td, oi, btd, bst, bi, NULL);
    ee_player *op = st_player(oi);
    ee_unit_stats *ast = op ? st_rec(op, unit_type_get_id(td)) : NULL;
    float most = *dps * AD_FIGHT_S;
    if (unit_type_get_uses_ammo(td) && unit_type_get_ammo(td) > 0) {
        float a = (float)st_shot_t(td, ast, oi, btd, bst, bi) * (float)unit_type_get_ammo(td);
        if (a < most) most = a;
    }
    return most;
}
internal float dct_worth(ee_player *pl, int t) {
    int c = 0;
    for (int r = 2; r < 7; r++) c += unit_cost(pl, t, r);
    return (float)(c < AD_VAL_MAX ? c : AD_VAL_MAX);
}
internal float dct_bombs(struct dct_st *d, ee_player *pl, unsigned idx, ee_unit *u) {   // what one bomber's load does to u
    int shot = st_shot_t(type_data(d->ref), st_rec(pl, d->ref), (int)idx, unit_get_type_data(u), unit_get_stats(u), st_pidx(u));
    return (float)shot * (float)d->ammo;
}
internal void dct_add(struct dct_st *d, ee_unit *u, ivec2 at, float r, float dps, int t, int oi, int kind) {
    if (d->n >= AD_THREATS || dps <= 0 || r <= 0) return;
    struct dct_threat *e = &d->t[d->n++];
    e->u = u; e->at = at; e->r = r; e->dps = dps; e->type = (short)t; e->owner = (uint8_t)oi; e->kind = (uint8_t)kind;
}
// THE PICTURE: what of the enemy shoots at its bombers, as far as it knows - every AS_EVERY
internal void dct_scan(ee_player *pl, unsigned idx, struct as_st *s, ee_unit **bomb, int nb, uint32_t now) {
    struct dct_st *d = &g_dct[idx];
    static float dc[16][G_TYPES_N], dr[16][G_TYPES_N]; static uint32_t dg[16][G_TYPES_N], gen;
    static ee_unit *towns[64], *cit[AH_CIT]; static struct { ivec2 at; int8_t o; } bld[AH_BLD];
    int ntown = 0, ncit = 0, nbld = 0;
    gen++;
    d->n = d->ntg = d->nport = 0;
    // the bomber type: the most held, else the newest it can train
    int best = -1, bn = 0, e = current_epoch(pl);
    for (int i = 0; i < nb; i++) {
        int t = unit_type_get_id(unit_get_type_data(bomb[i])), n = 0;
        for (int j = 0; j < nb; j++) n += unit_type_get_id(unit_get_type_data(bomb[j])) == t;
        if (n > bn) { bn = n; best = t; }
    }
    for (size_t k = 0; best < 0 && k < sizeof k_af_types / sizeof *k_af_types; k++)
        if (k_af_types[k].epoch <= e + 1 && ut_has(g_ut[k_af_types[k].type])) best = g_ut[k_af_types[k].type];
    d->ref = (short)best;
    if (best < 0 || (unsigned)best >= G_TYPES_N) { d->ref = -1; return; }
    ee_unit_type *btd = type_data(best);
    ee_unit_stats *bst = st_rec(pl, best);
    d->speed = bst ? unit_stats_get_speed(bst) : 0.0f;
    if (d->speed < 0.2f) d->speed = 1.0f;
    d->maxhp = (float)st_maxhp(btd, bst);
    d->ammo = unit_type_get_uses_ammo(btd) ? (unit_type_get_ammo(btd) > 0 ? unit_type_get_ammo(btd) : 1) : AD_NOAMMO;
    d->cost = 0;
    for (int r = 2; r < 7; r++) d->cost += unit_cost(pl, best, r);
    memset(d->nf, 0, sizeof d->nf); memset(d->has_aa, 0, sizeof d->has_aa); memset(d->airport, 0, sizeof d->airport);
    // what it sees (buildings remembered), and with honest_vision what it saw within AH_MEM where it saw it
    for (int o = 1; o < nplayers(); o++) {
        ee_player *op = player_at(o);
        if (!op || !is_enemy(pl, op)) continue;
        int count = 0, oi = player_get_index(op) & 15;
        for (ee_unit *u = player_get_units(op); u && count < 16384; u = unit_get_next(u), count++) {
            ee_unit_type *td = unit_get_type_data(u);
            if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !seen_by(u, pl)) continue;
            int c = unit_type_get_class_id(td), t = unit_type_get_id(td);
            if ((unsigned)t >= G_TYPES_N) continue;
            if (dg[oi][t] != gen) { dg[oi][t] = gen; dc[oi][t] = dct_dps(td, oi, btd, bst, (int)idx, &dr[oi][t]); }
            if (dc[oi][t] > 0) {
                d->has_aa[oi] = 1;
                if (!(d->logged[t / 8] >> (t % 8) & 1) && g_log) {   // the model once a type: what the game's numbers make of it
                    d->logged[t / 8] |= (uint8_t)(1 << (t % 8));
                    ar_log(idx, now, "air force: the anti-air of type %d (P%d) - %.0f a second at type %d, reach %.1f (bomber hp %.0f, speed %.2f, %d shots a sortie, price %d)",
                           t, oi, dc[oi][t], best, dr[oi][t], d->maxhp, d->speed, d->ammo, d->cost);
                }
                if (c == CLS_AIRPLANE) {
                    float fd, fs = dct_fighter(td, oi, btd, bst, (int)idx, &fd);
                    d->nf[oi] += 1.0f;
                    if (fs > d->ftype_v[oi]) { d->ftype_v[oi] = fs; d->ftype[oi] = (short)t; }
                    continue;
                }
                if (dc[oi][t] * dr[oi][t] > d->aatype_v[oi]) { d->aatype_v[oi] = dc[oi][t] * dr[oi][t]; d->aatype[oi] = (short)t; }
                dct_add(d, u, unit_get_tile(u), dr[oi][t], dc[oi][t], t, oi, 0);
                continue;
            }
            if (t == UT(B_AIRPORT) || unit_type_get_family(td) == KIND_AIRCRAFT_CARRIER) d->airport[oi] = 1;   // it has or can have fighters: they rise at what comes near (airguard.c INTERCEPT)
            if ((t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL) || t == UT(B_SETTLEMENT)) && ntown < 64) towns[ntown++] = u;
            if (c == CLS_BUILDING || c == CLS_FORTRESS || unit_type_get_family(td) == KIND_AIRCRAFT_CARRIER) {
                if (nbld < AH_BLD) { bld[nbld].at = unit_get_tile(u); bld[nbld++].o = (int8_t)oi; }
                if (c == CLS_NAVAL || is_wall(t) || d->ntg >= AH_TARGETS) continue;
                struct dct_tgt *g = &d->tg[d->ntg++];
                g->u = u; g->at = unit_get_tile(u); g->v = dct_worth(pl, t) + 100.0f; g->type = (short)t; g->owner = (uint8_t)oi; g->cf = 0;
            } else if (is_citizen_type(t) && ncit < AH_CIT) cit[ncit++] = u;
        }
    }
    struct intel *in = &g_intel[idx];
    for (int k = 0; g_honest_vision && k < in->n; k++) {
        struct intel_rec *r = &in->r[k];
        ee_unit_type *td = r->kind ? type_data(r->type) : NULL;
        if (!td || in->now - r->t > AH_MEM || r->t == in->last || r->owner <= 0 || r->owner >= 16 || (unsigned)r->type >= G_TYPES_N) continue;   // (seen at the last look: above)
        int t = r->type, oi = r->owner;
        if (dg[oi][t] != gen) { dg[oi][t] = gen; dc[oi][t] = dct_dps(td, oi, btd, bst, (int)idx, &dr[oi][t]); }
        if (dc[oi][t] <= 0) continue;
        d->has_aa[oi] = 1;
        if (unit_type_get_class_id(td) == CLS_AIRPLANE) { d->nf[oi] += 1.0f; continue; }
        if (r->x < 0) continue;
        ee_player *op = st_player(oi);
        ee_unit_stats *ast = op ? st_rec(op, t) : NULL;
        float drift = (ast ? unit_stats_get_speed(ast) : 0.0f) * (float)(in->now - r->t) / 1000.0f;
        if (in->now - r->t < AD_MEM_MOB) dct_add(d, NULL, iv2(r->x, r->y), dr[oi][t] + (drift < AD_DRIFT ? drift : AD_DRIFT), dc[oi][t] * (1.0f - (float)(in->now - r->t) / (float)AD_MEM_MOB), t, oi, 1);
    }
    // each enemy's gun and fighter where it isn't seen: the strongest seen of it, else of anyone, else the first of them
    int gun = UT(B_AA10_GUN_88MM), fig = UT(A_FIGHTER10_SOPWITH_CAMEL_F_1), gun_v = 0, fig_v = 0;
    for (int o = 0; o < 16; o++) {
        if (d->aatype_v[o] > (float)gun_v && d->aatype[o] > 0) { gun_v = (int)d->aatype_v[o]; gun = d->aatype[o]; }
        if (d->ftype_v[o] > (float)fig_v && d->ftype[o] > 0) { fig_v = (int)d->ftype_v[o]; fig = d->ftype[o]; }
    }
    float best_aa = 0;
    for (int o = 0; o < 16; o++) {
        int gt = d->aatype[o] > 0 ? d->aatype[o] : gun, ft = d->ftype[o] > 0 ? d->ftype[o] : fig;
        ee_unit_type *gtd = gt >= 0 ? type_data(gt) : NULL, *ftd = ft >= 0 ? type_data(ft) : NULL;
        d->aadps[o] = gtd ? dct_dps(gtd, o, btd, bst, (int)idx, &d->aar[o]) : 0.0f;
        d->fsortie[o] = ftd ? dct_fighter(ftd, o, btd, bst, (int)idx, &d->fdps[o]) : 0.0f;
        if (d->has_aa[o] && d->aadps[o] > best_aa) best_aa = d->aadps[o];
        if (d->airport[o] && d->nf[o] < (float)AD_FIGHTERS) d->nf[o] = (float)AD_FIGHTERS;
    }
    for (int k = 0; k < ntown; k++) {   // an enemy that has anti-air has it by its towns: assumed there unless looked at lately
        ee_player *o = unit_get_owner(towns[k]);
        int oi = o ? player_get_index(o) & 15 : 0;
        ivec2 p = unit_get_tile(towns[k]);
        if (oi && d->has_aa[oi] && intel_looked_ago(pl, p.x, p.y) > AH_LOOKED) dct_add(d, NULL, p, d->aar[oi], d->aadps[oi], 0, oi, 2);
    }
    for (int k = 0; k < AH_FLAK; k++)   // where bombers on runs were lost: the strongest gun known there
        if (s->flak[k].t && now - s->flak[k].t < AH_MEM) dct_add(d, NULL, s->flak[k].at, (float)AH_FLAK_R, best_aa > 0 ? best_aa : d->aadps[0], 0, 0, 3);
    // the fighters' cover: within AH_FIGHTER_R of a building of an enemy with fighters (or an Airport)
    memset(d->cover, 0, sizeof d->cover);
    for (int k = 0; k < nbld; k++) {
        int o = bld[k].o & 15;
        if (d->nf[o] <= 0) continue;
        int r = (AH_FIGHTER_R + AH_CELL) / AH_CELL, cx = bld[k].at.x / AH_CELL, cy = bld[k].at.y / AH_CELL;
        for (int x = cx - r; x <= cx + r; x++)
            for (int y = cy - r; y <= cy + r; y++)
                if (x >= 0 && y >= 0 && x < AH_CELLS && y < AH_CELLS && ivec2_dist2(iv2(x * AH_CELL + AH_CELL / 2, y * AH_CELL + AH_CELL / 2), bld[k].at) <= AH_FIGHTER_R * AH_FIGHTER_R)
                    d->cover[x][y] |= (uint16_t)(1u << o);
    }
    // citizens at work: two or more of an owner within 4 tiles, the one with the most beside it
    for (int i = 0; i < ncit && d->ntg < AH_TARGETS; i++) {
        ivec2 p = unit_get_tile(cit[i]);
        int n = 0;
        for (int j = 0; j < ncit; j++) if (unit_get_owner(cit[j]) == unit_get_owner(cit[i]) && ivec2_dist2(unit_get_tile(cit[j]), p) <= 16) n++;
        if (n < 2) continue;
        struct dct_tgt *g = &d->tg[d->ntg++];
        ee_player *o = unit_get_owner(cit[i]);
        g->u = cit[i]; g->at = p; g->v = 120.0f * (float)n; g->type = (short)unit_type_get_id(unit_get_type_data(cit[i])); g->owner = (uint8_t)(o ? player_get_index(o) & 15 : 0); g->cf = 0;
    }
    // counter-fire's requests: the shooters it asked about within AD_CF_MS, still there and seen - worth AD_CF_W their price
    for (int k = 0; k < 16 && d->ntg < AH_TARGETS; k++) {
        ee_unit *u = s->cf[k].u;
        if (!u || !s->cf[k].t || now - s->cf[k].t >= AD_CF_MS || unit_get_serial(u) != s->cf[k].id || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !seen_by(u, pl)) continue;
        ee_player *o = unit_get_owner(u);
        struct dct_tgt *g = &d->tg[d->ntg++];
        g->u = u; g->at = unit_get_tile(u); g->type = (short)unit_type_get_id(unit_get_type_data(u)); g->owner = (uint8_t)(o ? player_get_index(o) & 15 : 0); g->cf = 1;
        g->v = (dct_worth(pl, g->type) + 100.0f) * AD_CF_W;
    }
    // its Airports: where a sortie starts and ends
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    void *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    uint8_t *q = um ? type_list(um, UT(B_AIRPORT)) : NULL;
    for (unsigned j = 0, f = q ? deque_front(q) : 0, c = q ? deque_count(q) : 0; j < c && d->nport < 16; j++) {
        ee_unit *b = deque_at(q, f + j);
        if (!b || unit_get_hp(b) <= 0 || unit_get_x(b) < 0) continue;
        d->port[d->nport] = unit_get_tile(b); d->port_u[d->nport][0] = b; d->port_u[d->nport][1] = unit_get_physics(b); d->nport++;
    }
}
// the damage on the way a -> b from the guns (gone: those knocked out, not counted), and the time in the fighters' cover
internal void dct_way(struct dct_st *d, struct dct_acc *acc, ivec2 a, ivec2 b, const uint8_t *gone) {
    float dx = (float)(b.x - a.x), dy = (float)(b.y - a.y), l = sqrtf(dx * dx + dy * dy);
    for (int k = 0; k < d->n; k++) {
        if (gone && gone[k]) continue;
        struct dct_threat *e = &d->t[k];
        float px = (float)(e->at.x - a.x), py = (float)(e->at.y - a.y), along = l > 0 ? (px * dx + py * dy) / l : 0.0f, h2 = px * px + py * py - along * along;
        if (h2 < 0) h2 = 0;
        if (h2 >= e->r * e->r) continue;
        float half = sqrtf(e->r * e->r - h2), lo = along - half, hi = along + half;
        if (lo < 0) lo = 0;
        if (hi > l) hi = l;
        if (hi > lo) acc->dmg += e->dps * (hi - lo) / d->speed;
    }
    float step = (float)AH_CELL / 2;
    for (float t = 0; t <= l; t += step) {
        int x = (a.x + (l > 0 ? (int)(dx * t / l) : 0)) / AH_CELL, y = (a.y + (l > 0 ? (int)(dy * t / l) : 0)) / AH_CELL;
        if (x < 0 || y < 0 || x >= AH_CELLS || y >= AH_CELLS || !d->cover[x][y]) continue;
        for (int o = 0; o < 16; o++) if (d->cover[x][y] >> o & 1) acc->tin[o] += (t + step <= l ? step : l - t) / d->speed;
    }
}
// ... over p for secs: the guns that reach it, the fighters' cover there
internal void dct_over(struct dct_st *d, struct dct_acc *acc, ivec2 p, float secs, const uint8_t *gone) {
    for (int k = 0; k < d->n; k++)
        if ((!gone || !gone[k]) && (float)ivec2_dist2(d->t[k].at, p) <= d->t[k].r * d->t[k].r) acc->dmg += d->t[k].dps * secs;
    int x = p.x / AH_CELL, y = p.y / AH_CELL;
    if (x >= 0 && y >= 0 && x < AH_CELLS && y < AH_CELLS)
        for (int o = 0; o < 16; o++) if (d->cover[x][y] >> o & 1) acc->tin[o] += secs;
}
// the same as times: each gun's seconds in reach on the way a -> b, each enemy's in its fighters' cover (tk, tf added to)
internal void dct_leg(struct dct_st *d, ivec2 a, ivec2 b, float *tk, float *tf) {
    float dx = (float)(b.x - a.x), dy = (float)(b.y - a.y), l = sqrtf(dx * dx + dy * dy);
    for (int k = 0; k < d->n; k++) {
        struct dct_threat *e = &d->t[k];
        float px = (float)(e->at.x - a.x), py = (float)(e->at.y - a.y), along = l > 0 ? (px * dx + py * dy) / l : 0.0f, h2 = px * px + py * py - along * along;
        if (h2 < 0) h2 = 0;
        if (h2 >= e->r * e->r) continue;
        float half = sqrtf(e->r * e->r - h2), lo = along - half, hi = along + half;
        if (lo < 0) lo = 0;
        if (hi > l) hi = l;
        if (hi > lo) tk[k] += (hi - lo) / d->speed;
    }
    float step = (float)AH_CELL / 2;
    for (float t = 0; t <= l; t += step) {
        int x = (a.x + (l > 0 ? (int)(dx * t / l) : 0)) / AH_CELL, y = (a.y + (l > 0 ? (int)(dy * t / l) : 0)) / AH_CELL;
        if (x < 0 || y < 0 || x >= AH_CELLS || y >= AH_CELLS || !d->cover[x][y]) continue;
        for (int o = 0; o < 16; o++) if (d->cover[x][y] >> o & 1) tf[o] += (t + step <= l ? step : l - t) / d->speed;
    }
}
internal void dct_stay(struct dct_st *d, ivec2 p, float secs, float *tk, float *tf) {
    for (int k = 0; k < d->n; k++) if ((float)ivec2_dist2(d->t[k].at, p) <= d->t[k].r * d->t[k].r) tk[k] += secs;
    int x = p.x / AH_CELL, y = p.y / AH_CELL;
    if (x >= 0 && y >= 0 && x < AH_CELLS && y < AH_CELLS)
        for (int o = 0; o < 16; o++) if (d->cover[x][y] >> o & 1) tf[o] += secs;
}
internal float dct_total(struct dct_st *d, const struct dct_acc *acc) {   // the guns' and the fighters' (each fighter one fight, the most it does)
    float t = acc->dmg;
    for (int o = 0; o < 16; o++) {
        if (acc->tin[o] <= 0 || d->nf[o] <= 0) continue;
        float f = d->fdps[o] * (acc->tin[o] < AD_FIGHT_S ? acc->tin[o] : AD_FIGHT_S);
        t += d->nf[o] * (f < d->fsortie[o] ? f : d->fsortie[o]);
    }
    return t;
}
// a sortie from -> at -> back: the damage a bomber takes (all of it on the one, as guns aim)
internal float dct_sortie(struct dct_st *d, ivec2 from, ivec2 at, ivec2 back, float over) {
    struct dct_acc acc;
    memset(&acc, 0, sizeof acc);
    dct_way(d, &acc, from, at, NULL);
    dct_over(d, &acc, at, over, NULL);
    dct_way(d, &acc, at, back, NULL);
    return dct_total(d, &acc);
}
internal int dct_port(struct dct_st *d, ee_unit *u) {   // its Airport (ah_port), else the nearest; -1 none
    int pk = ah_port(u, d->port_u, d->nport);
    if (pk >= 0 || !d->nport) return pk;
    ivec2 p = unit_get_tile(u);
    for (int k = 0; k < d->nport; k++) if (pk < 0 || ivec2_dist2(d->port[k], p) < ivec2_dist2(d->port[pk], p)) pk = k;
    return pk;
}
// THE SMALL RUNS: the bombers free go in flights of up to AD_FLIGHT from one Airport (as many as the target's hit points
// take, the soundest first) at the target known worth most for the way that they come back from alive - the damage
// expected there, over it and home, x AD_MARGIN and AD_RESERVE of a bomber's hit points left, below the hit points of the
// weakest of the flight (all of the fire on the one); the ground round a target of an enemy with anti-air not looked at
// lately has one of its guns assumed there. One flight a target at a time.
internal void dct_runs(ee_player *pl, unsigned idx, struct as_st *s, ee_unit **ready, int nr, uint32_t now) {
    struct dct_st *d = &g_dct[idx];
    static uint8_t taken[AH_TARGETS], used[AS_BOMBERS];
    static int gi[AS_BOMBERS];
    if (nr <= 0 || d->ref < 0 || !d->ntg || !d->nport) return;
    memset(taken, 0, (size_t)d->ntg);
    memset(used, 0, (size_t)nr);
    for (int k = 0; k < d->ntg; k++)
        for (int j = 0; j < s->nh && !taken[k]; j++) taken[k] = s->held[j].har && s->held[j].tgt == unit_get_serial(d->tg[k].u);
    int runs = 0, none = 0;
    for (int i = 0; i < nr; i++) {
        int e0 = as_held_at(s, unit_get_serial(ready[i]));
        if (used[i] || e0 < 0 || s->held[e0].tgt) { used[i] = 1; continue; }   // (the wing's: launched at this look)
        int pk = ah_port(ready[i], d->port_u, d->nport);
        ivec2 from = pk >= 0 ? d->port[pk] : unit_get_x(ready[i]) < 0 ? d->port[0] : unit_get_tile(ready[i]);
        int ng = 0;   // the flight: those free at the same Airport, the soundest first
        for (int j = i; j < nr && ng < AS_BOMBERS; j++) {
            int ej = as_held_at(s, unit_get_serial(ready[j]));
            if (used[j] || ej < 0 || s->held[ej].tgt || (j != i && (pk < 0 || ah_port(ready[j], d->port_u, d->nport) != pk))) continue;
            int k = ng++;
            while (k > 0 && unit_get_hp(ready[gi[k - 1]]) < unit_get_hp(ready[j])) { gi[k] = gi[k - 1]; k--; }
            gi[k] = j;
        }
        int best = -1, bn = 0;
        float bv = 0, bdmg = 0;
        for (int k = 0; k < d->ntg; k++) {
            if (taken[k]) continue;
            struct dct_tgt *g = &d->tg[k];
            float dist = sqrtf((float)ivec2_dist2(from, g->at)), bombs = dct_bombs(d, pl, idx, g->u), hp = (float)unit_get_hp(g->u);
            if (bombs <= 0 || hp <= 0) continue;
            int need = (int)ceilf(hp / bombs);
            if (need < 1) need = 1;
            if (need > AD_FLIGHT) need = AD_FLIGHT;
            int n = need < ng ? need : ng;
            float frac = (float)n * bombs / hp, v = g->v * (frac < 1 ? frac : 1.0f) / (30.0f + dist);
            if (best >= 0 && v <= bv) continue;
            float dmg = dct_sortie(d, from, g->at, from, AD_OVER_S);
            if (d->has_aa[g->owner] && !ah_looked(pl, g->at)) dmg += d->aadps[g->owner] * (2.0f * d->aar[g->owner] / d->speed + AD_OVER_S);
            if (dmg * AD_MARGIN + AD_RESERVE * d->maxhp >= (float)unit_get_hp(ready[gi[n - 1]])) continue;
            best = k; bv = v; bn = n; bdmg = dmg;
        }
        if (best < 0) { none = 1; for (int m = 0; m < ng; m++) used[gi[m]] = 1; continue; }   // the soundest of them find none: the rest neither
        taken[best] = 1;
        struct dct_tgt *g = &d->tg[best];
        int sent = 0, hpmin = 0;
        for (int m = 0; m < bn; m++) {
            ee_unit *u = ready[gi[m]];
            int e = as_held_at(s, unit_get_serial(u));
            used[gi[m]] = 1;
            if (e < 0 || !cl_take(u, CLP_TASK, CLW_BOMBERS, AH_RUN_MS)) continue;   // ORDERS BY PRIORITY (claims.c): nobody else's on the run
            if (!as_order(u, goal_attack(u, g->u))) { cl_end(u, CLW_BOMBERS); continue; }
            s->held[e].tgt = unit_get_serial(g->u); s->held[e].t = s->held[e].t0 = now ? now : 1; s->held[e].har = 1; s->held[e].tat = g->at;
            s->held[e].from = from; s->held[e].home = pk >= 0 ? unit_get_serial((ee_unit *)d->port_u[pk][0]) : 0; s->held[e].back = 0;
            s->held[e].ttype = g->type; s->held[e].pred = bdmg; s->held[e].cf = g->cf;
            hpmin = unit_get_hp(u);
            sent++;
        }
        if (!sent) continue;
        runs++; s->har_runs++; s->har_sent += (unsigned)sent;
        if (g->cf) { s->cf_runs++; s->cf_sent += (unsigned)sent; }
        ar_log(idx, now, "air force: a run of %d%s at type %d at %d,%d (from %d,%d, %d tiles) - damage expected %.0f a bomber, the weakest has %d (anti-air known %d, targets %d)", sent, g->cf ? " (counter-fire's)" : "", g->type,
               g->at.x, g->at.y, from.x, from.y, (int)sqrtf((float)ivec2_dist2(from, g->at)), bdmg, hpmin, d->n, d->ntg);
    }
    if (none && !runs) s->har_none++;
}
internal void as_runs(ee_player *pl, unsigned idx, struct as_st *s, ee_unit **ready, int nr, ee_unit **idle, int ni, uint32_t now) {
    if (dct_part(idx, DCP_SURVIVE)) dct_runs(pl, idx, s, idle, ni, now);
    else ah_runs(pl, idx, s, ready, nr, now);
}
// THE WAY HOME (every AD_HOME_EVERY): a bomber in the air whose hit points no longer take it through what is left of
// its sortie - a small run's: to its target, over it and home; the wing's: to its target and half the time over it, the
// fire shared by the wing's bombers (the wing reckoned with losses: its bombs first) - with AD_MARGIN_HOME and
// AD_RESERVE is called back to its Airport while they still take it home (the way home's damage as expected, no
// margin); one that can't make it home either way goes on with its bombs. In its Airport it is repaired (the game's)
// and flies again. AD_MARGIN_HOME is below the launch's AD_MARGIN: with the same margin a bomber launched was called
// back a second later at the next sighting (47 of 49 sorties in a pilot). Most of those called back are still sound
// (454 of 560 at 90% of their hit points or more, 20 games): new guns seen on the rest of the way. Calling back only
// the hit ones, and the sound ones when their hit points just take them home, measured worse (air_rich 20 games,
// the whole doctrine against none: losses +31, se 10, against -8, se 14; bombers lost after the call 8.0 an AI
// against 2.9 - called too late); without the way home at all within the noise (20 games: seeds 1-5 soldiers +32,
// se 13, seeds 6-10 -11, se 8).
internal void dct_home(ee_player *pl, unsigned idx, struct as_st *s, uint32_t now) {
    struct dct_st *d = &g_dct[idx];
    if (d->ref < 0 || !d->nport) return;
    int wing = 0;   // the wing's bombers in the air: they share the guns' fire
    for (int k = 0; k < s->nh; k++) if (s->held[k].tgt && !s->held[k].har) wing++;
    for (int k = 0; k < s->nh; k++) {
        __typeof__(s->held[0]) *h = &s->held[k];
        ee_unit *u = unit_by_id((unsigned)h->id);
        if (!u || unit_get_serial(u) != h->id || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) { h->hp_sec = 0; continue; }
        ivec2 p = unit_get_tile(u);
        float here = 0, share = (float)(h->tgt && !h->har && wing > 1 ? wing : 1);
        float hk[5] = { 0 };   // by the kind of what it knows (seen, remembered, assumed by a town, where a bomber was lost), and the guns seen within their own range
        for (int j = 0; j < d->n; j++) {
            float d2 = (float)ivec2_dist2(d->t[j].at, p);
            if (d2 > d->t[j].r * d->t[j].r) continue;
            here += d->t[j].dps; hk[d->t[j].kind & 3] += d->t[j].dps;
            if (d->t[j].u && unit_get_hp(d->t[j].u) > 0) { float rg = st_range(d->t[j].u) + 0.5f; if (d2 <= rg * rg) hk[4] += d->t[j].dps; }
        }
        // the guns' model against what they do: the fire expected on it since the last look, the hit points it lost
        if (h->hp_sec > 0 && h->hp_sec >= unit_get_hp(u)) {
            s->obs_taken += (float)(h->hp_sec - unit_get_hp(u)); s->obs_pred += h->here_prev;
            for (int j = 0; j < 5; j++) s->obs_k[j] += h->here_k[j];
        }
        h->hp_sec = unit_get_hp(u); h->here_prev = here / share * (float)AD_HOME_EVERY / 1000.0f;
        for (int j = 0; j < 5; j++) h->here_k[j] = hk[j] / share * (float)AD_HOME_EVERY / 1000.0f;
        if (h->home2 || !unit_get_goal(u)) continue;
        int pk = dct_port(d, u);
        if (pk < 0) continue;
        ivec2 home = d->port[pk];
        ee_unit_type *td = unit_get_type_data(u);
        float hp = (float)unit_get_hp(u), res = AD_RESERVE * (float)st_maxhp(td, unit_get_stats(u));
        struct dct_acc acc;
        memset(&acc, 0, sizeof acc);
        dct_way(d, &acc, p, home, NULL);
        float way_home = dct_total(d, &acc), need = way_home * AD_MARGIN_HOME + res;
        ee_unit *t = h->tgt ? unit_by_id((unsigned)h->tgt) : NULL;
        if (t && (unit_get_serial(t) != h->tgt || unit_get_hp(t) <= 0)) t = NULL;
        if (!t) t = fl_target_of(u);
        if (t && (!unit_type_get_uses_ammo(td) || unit_get_ammo(u) > 0)) {
            if (h->tgt && !h->har) {   // the wing's: its bombs first (the wing reckoned with losses) - home only when it can't get them there
                memset(&acc, 0, sizeof acc);
                dct_way(d, &acc, p, unit_get_tile(t), NULL);
                dct_over(d, &acc, unit_get_tile(t), AD_OVER_S / 2, NULL);
                need = dct_total(d, &acc) / share * AD_MARGIN_HOME;
            } else need = dct_sortie(d, p, unit_get_tile(t), home, AD_OVER_S) / share * AD_MARGIN_HOME + res;
        }
        if (hp >= need) continue;
        if ((float)unit_get_hp(u) < way_home) {   // it can't make it home either way: its bombs first
            if (!h->nohome) { h->nohome = 1; s->doomed_on++; }
            continue;
        }
        as_rebase(pl, h->id, (unsigned)unit_get_serial((ee_unit *)d->port_u[pk][0]));
        h->home2 = 1; h->tgt = 0; h->har = 0; s->called_home++;
        ar_log(idx, now, "air force: a bomber called home at %d,%d - %d hit points, the rest of its sortie takes %.0f (home %.0f, %d tiles)", p.x, p.y, unit_get_hp(u), need, way_home,
               (int)sqrtf((float)ivec2_dist2(p, home)));
    }
}
// the bombers at a look: those of the wing whose target stands keep at it (out); the free ones - ready with AF_READY_PCT
// of their hit points, idle with any - for the next wing or run
internal int as_wing_look(ee_player *pl, unsigned idx, struct as_st *s, ee_unit **bomb, int nb, uint32_t now, ee_unit **ready, int *nr, ee_unit **idle, int *ni, int *har_out) {
    int out = 0;
    for (int i = 0; i < nb; i++) {
        ee_unit *u = bomb[i];
        int e = as_held_at(s, unit_get_serial(u));
        if (e < 0) continue;
        if (s->held[e].har) {   // a lone run's: free once back (not the wing's out)
            int f = ah_free(s, e, u, now, har_out);
            if (!f) continue;
            if (f == 1) ready[(*nr)++] = u;
            idle[(*ni)++] = u;
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
        } else if (!stands && s->held[e].tgt) dct_down(s, pl, s->held[e].tgt, s->held[e].ttype);
        if (stands) {
            out++;
            if (g || now - s->held[e].t < AS_AGAIN) continue;
            if (as_order(u, goal_attack(u, t))) s->held[e].t = now;
            continue;
        }
        s->held[e].tgt = 0;
        if (g) continue;
        if (unit_get_hp(u) * 100 >= unit_type_get_hp(unit_get_type_data(u)) * AF_READY_PCT) ready[(*nr)++] = u;
        idle[(*ni)++] = u;
    }
    return out;
}
// no position for the wing: every free bomber to the lone runs
internal void ah_idle(ee_player *pl, unsigned idx, struct as_st *s, ee_unit **bomb, int nb, uint32_t now) {
    static ee_unit *ready[AS_BOMBERS], *idle[AS_BOMBERS];
    int nr = 0, ni = 0, har_out = 0;
    if (!AS_RUNS(idx)) return;
    for (int i = 0; i < nb; i++) {
        int e = as_held_at(s, unit_get_serial(bomb[i])), f = e >= 0 ? ah_free(s, e, bomb[i], now, &har_out) : 0;
        if (f == 1) ready[nr++] = bomb[i];
        if (f) idle[ni++] = bomb[i];
    }
    as_runs(pl, idx, s, ready, nr, idle, ni, now);
}
// the anti-air near pos and what holds it - the nearest first (bld, THE VALUE WING: every building but walls too, after
// the siege)
internal void as_targets(ee_player *pl, ivec2 pos, int shoot, int bld, struct as_tg *aa, int *pna, struct as_tg *tg, int *pnt) {
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
            if (fam == CLS_BUILDING || fam == CLS_FORTRESS) {
                if (!defence_stats(u, &rg, &at) || at <= 0) { if (!bld || is_wall(unit_type_get_id(it.td))) continue; d += 3L * AS_R * AS_R / 2; }
                else if (shoot >= 0) d += (long)AS_R * AS_R;
            }
            else if (fam == CLS_LAND && (is_siege(unit_type_get_id(it.td)) || f == KIND_ARTILLERY)) d += (long)AS_R * AS_R;   // after the defences
            else if (fam == CLS_LAND && is_soldier(it.td)) d += 2L * AS_R * AS_R;   // the soldiers last: what the force's judgement counts
            else continue;
        }
        if (nt == AS_TARGETS && tg[nt - 1].d <= d) continue;
        int k = nt < AS_TARGETS ? nt++ : AS_TARGETS - 1;
        while (k > 0 && tg[k - 1].d > d) { tg[k] = tg[k - 1]; k--; }
        tg[k].u = u; tg[k].d = d;
    }
    *pna = na; *pnt = nt;
}
// THE VALUE WING: what n bombers (hp each) from `from` do at pos - the worth they destroy (all the load a target's hit
// points take, the anti-air first and in the order of as_targets; an AA gun knocked out fires no more on the way back,
// and is worth a bomber more: what it would shoot down next) less the bombers expected lost: the damage of the guns
// and fighters on the way in, half the time over the target, the other half and the way out without the guns knocked
// out - the guns' fire is the same for 1 or 20 (numbers share it: the wing overwhelms), x AD_MARGIN, a bomber lost for
// every hp of it; half of those lost drop no bombs. alloc: how many at each target
#define AD_TGTS (2 * AS_TARGETS)
internal float g_dct_tin[AD_TGTS][AD_THREATS], g_dct_tout[AD_TGTS][AD_THREATS], g_dct_fin[AD_TGTS][16], g_dct_fout[AD_TGTS][16];   // dct_prep: each target's way in (and half the time over it), out (the other half, the way back)
internal float dct_ev(struct dct_st *d, int na, int nt, const float *bombs, const float *hps, const float *worth, const int *gk,
                     int n, float hp, float w, int *alloc, float *lost, float *dmg) {
    static uint8_t gone[AD_THREATS];
    static float tk[AD_THREATS];
    memset(gone, 0, (size_t)d->n);
    int left = n;
    float val = 0;
    for (int j = 0; j < na + nt; j++) {
        alloc[j] = 0;
        if (bombs[j] <= 0 || left <= 0) continue;
        int need = (int)ceilf(hps[j] / bombs[j]);
        if (need < 1) need = 1;
        if (need > left) {   // more than are left: the damage they do, at AD_PARTIAL its share of the worth (it is repaired)
            alloc[j] = left; val += worth[j] * (float)left * bombs[j] / hps[j] * AD_PARTIAL; left = 0;
            continue;
        }
        alloc[j] = need; left -= need;
        val += worth[j];
        if (j < na && gk[j] >= 0) gone[gk[j]] = 1;
    }
    if (val <= 0) return -1.0e9f;
    // each gun fires as long as any bomber of the wing is in its reach (the longest of their ways), each enemy's fighters
    // as long as any is in their cover
    struct dct_acc acc;
    memset(&acc, 0, sizeof acc);
    memset(tk, 0, (size_t)d->n * sizeof *tk);
    for (int j = 0; j < na + nt; j++) {
        if (!alloc[j]) continue;
        for (int k = 0; k < d->n; k++) { float t = g_dct_tin[j][k] + (gone[k] ? 0.0f : g_dct_tout[j][k]); if (t > tk[k]) tk[k] = t; }
        for (int o = 0; o < 16; o++) { float t = g_dct_fin[j][o] + g_dct_fout[j][o]; if (t > acc.tin[o]) acc.tin[o] = t; }
    }
    for (int k = 0; k < d->n; k++) acc.dmg += d->t[k].dps * tk[k];
    float D = dct_total(d, &acc), L = D * AD_MARGIN / (hp > 1 ? hp : 1.0f);
    if (L > (float)n) L = (float)n;
    *lost = L; *dmg = D;
    if (L > AD_LOSS_MAX * (float)n) return -1.0e9f;   // a wing overwhelms: a few lost, not most of it
    return val * w * (1.0f - 0.5f * L / (float)n) - L * (float)d->cost;
}
struct dct_cand { ivec2 at; float w; int push; };
// a position's targets, their load, hit points and worth
internal int dct_prep(ee_player *pl, unsigned idx, struct dct_cand *c, int shoot, ivec2 from, struct as_tg *aa, int *na, struct as_tg *tg, int *nt, float *bombs, float *hps, float *worth, int *gk) {
    struct dct_st *d = &g_dct[idx];
    as_targets(pl, c->at, c->push ? shoot : -1, 1, aa, na, tg, nt);
    for (int j = 0; j < *na + *nt; j++) {
        ee_unit *u = j < *na ? aa[j].u : tg[j - *na].u;
        ivec2 at = unit_get_tile(u);
        memset(g_dct_tin[j], 0, (size_t)d->n * sizeof **g_dct_tin); memset(g_dct_tout[j], 0, (size_t)d->n * sizeof **g_dct_tout);
        memset(g_dct_fin[j], 0, sizeof g_dct_fin[j]); memset(g_dct_fout[j], 0, sizeof g_dct_fout[j]);
        dct_leg(d, from, at, g_dct_tin[j], g_dct_fin[j]); dct_stay(d, at, AD_OVER_S / 2, g_dct_tin[j], g_dct_fin[j]);
        dct_stay(d, at, AD_OVER_S / 2, g_dct_tout[j], g_dct_fout[j]); dct_leg(d, at, from, g_dct_tout[j], g_dct_fout[j]);
        bombs[j] = dct_bombs(d, pl, idx, u); hps[j] = (float)unit_get_hp(u);
        worth[j] = dct_worth(pl, unit_type_get_id(unit_get_type_data(u))) + (j < *na ? (float)d->cost : 0.0f);
        gk[j] = -1;
        for (int k = 0; j < *na && k < d->n; k++) if (d->t[k].u == u) { gk[j] = k; break; }
    }
    return *na + *nt;
}
// THE VALUE WING's look (in place of the wing above): the positions - the force's (worth AD_PUSH_W more) and the enemy's
// buildings known, by AD_CELL cell the AD_CANDS worth most - each weighed for every number of the bombers ready (the
// soundest first); the wing launches at once where the worth is highest, if it is a gain, with as many as make it
// highest: each target as many as its hit points take, the rest spread over them (the anti-air first). Never a wait
// for a count: no gain anywhere, the small runs fly meanwhile, and those not on the wing fly them too. Trained for: the
// fewest at the force's position that would be a gain (sound), else AF_WING.
internal void dct_wing(ee_player *pl, unsigned idx, struct as_st *s, ee_unit **bomb, int nb, int shoot, uint32_t now) {
    struct dct_st *d = &g_dct[idx];
    struct armies_st *a = &g_armies[idx];
    static ee_unit *ready[AS_BOMBERS], *idle[AS_BOMBERS];
    static struct as_tg aa[AS_TARGETS], tg[AS_TARGETS];
    static float bombs[2 * AS_TARGETS], hps[2 * AS_TARGETS], worth[2 * AS_TARGETS];
    static int gk[2 * AS_TARGETS], alloc[2 * AS_TARGETS], best_alloc[2 * AS_TARGETS];
    static float cv[AD_CELLS][AD_CELLS]; static short ci[AD_CELLS][AD_CELLS];
    int nr = 0, ni = 0, har_out = 0, na = 0, nt = 0;
    int out = as_wing_look(pl, idx, s, bomb, nb, now, ready, &nr, idle, &ni, &har_out);
    if (out) { as_runs(pl, idx, s, ready, nr, idle, ni, now); return; }   // one wing at a time (the small runs meanwhile)
    if (s->wing_out) {
        s->wing_out = 0;
        if (s->wing_push) {   // its targets down: the force judges its push again now, not at its next attack time
            g_ar_push_now |= 1u << idx;
            ar_price_clear(a, s->wing_pos);
            ar_log(idx, now, "air force: the wing's targets at %d,%d are down - the force judges its push again", s->wing_pos.x, s->wing_pos.y);
        }
    }
    if (d->ref < 0) return;
    struct dct_cand cand[AD_CANDS + 1];
    int nc = 0;
    ivec2 fpos;
    if (shoot >= 0) fpos = s->sh[shoot].at;
    else if (g_ar_breach[idx].on) fpos = g_ar_breach[idx].focus;
    else if (a->out == 1 && a->target) fpos = a->dest;
    else if (a->strong_t && now - a->strong_t < AF_STRONG_MS) fpos = a->strong;
    else if (!GAME_AI(idx) && sn_need_at(idx, &fpos)) { }
    else fpos = iv2(-1000, -1000);
    if (fpos.x > -1000) { cand[0].at = fpos; cand[0].w = AD_PUSH_W; cand[0].push = 1; nc = 1; }
    memset(cv, 0, sizeof cv);
    for (int k = 0; k < d->ntg; k++) {
        ee_unit_type *td = unit_get_type_data(d->tg[k].u);
        int c = td ? unit_type_get_class_id(td) : -1, x = d->tg[k].at.x / AD_CELL, y = d->tg[k].at.y / AD_CELL;
        if ((c != CLS_BUILDING && c != CLS_FORTRESS) || x < 0 || y < 0 || x >= AD_CELLS || y >= AD_CELLS) continue;
        if (cv[x][y] <= 0 || d->tg[k].v > d->tg[ci[x][y]].v) ci[x][y] = (short)k;
        cv[x][y] += d->tg[k].v;
    }
    while (nc < AD_CANDS + 1) {
        int bx = -1, by = -1;
        for (int x = 0; x < AD_CELLS; x++)
            for (int y = 0; y < AD_CELLS; y++) if (cv[x][y] > 0 && (bx < 0 || cv[x][y] > cv[bx][by])) { bx = x; by = y; }
        if (bx < 0) break;
        cv[bx][by] = 0;
        ivec2 at = d->tg[ci[bx][by]].at;
        if (nc && cand[0].push && ivec2_dist2(at, cand[0].at) <= AS_R * AS_R) continue;   // the force's own
        cand[nc].at = at; cand[nc].w = 1.0f; cand[nc].push = 0; nc++;
    }
    // the bombers ready, the soundest first, and where they fly from
    for (int i = 1; i < nr; i++)
        for (int j = i; j > 0 && unit_get_hp(ready[j]) > unit_get_hp(ready[j - 1]); j--) { ee_unit *t = ready[j]; ready[j] = ready[j - 1]; ready[j - 1] = t; }
    ivec2 from = iv2(0, 0);
    for (int i = 0; i < nr; i++) { int pk = dct_port(d, ready[i]); ivec2 p = pk >= 0 ? d->port[pk] : unit_get_tile(ready[i]); from.x += p.x; from.y += p.y; }
    if (nr) { from.x /= nr; from.y /= nr; }
    else if (d->nport) from = d->port[0];
    int bc = -1, bn = 0, nmin = 0;
    float bev = 0, blost = 0, bdmg = 0, bval_ev = -1.0e9f;
    for (int c = 0; c < nc; c++) {
        int nj = dct_prep(pl, idx, &cand[c], shoot, from, aa, &na, tg, &nt, bombs, hps, worth, gk);
        if (!nj) { if (cand[c].push && shoot >= 0) s->sh[shoot].t = 0; continue; }   // nothing left at the shooters' spot: the next
        float lost, dmg, hsum = 0;
        if (cand[c].push)   // the training: the fewest sound ones that would be a gain here
            for (int n = 1; n <= AF_WING_MAX && !nmin; n++) if (dct_ev(d, na, nt, bombs, hps, worth, gk, n, d->maxhp, cand[c].w, alloc, &lost, &dmg) > 0) nmin = n;
        for (int n = 1; n <= nr; n++) {
            hsum += (float)unit_get_hp(ready[n - 1]);
            if (n > 6 && n < nr && n % (n > 16 ? 4 : 2)) continue;   // (every count up to 6, then every second, every fourth, and all of them)
            float ev = dct_ev(d, na, nt, bombs, hps, worth, gk, n, hsum / (float)n, cand[c].w, alloc, &lost, &dmg);
            if (ev > bval_ev) bval_ev = ev;
            if (ev <= bev) continue;
            bev = ev; bc = c; bn = n; blost = lost; bdmg = dmg;
        }
    }
    s->best_ev = bval_ev;
    if (nc && cand[0].push) { s->needed = nmin ? nmin : AF_WING; s->need_t = now ? now : 1; }
    if (bc < 0) {
        s->waits++;
        if (!s->ev_log || now - s->ev_log >= 60000) {
            s->ev_log = now ? now : 1;
            ar_log(idx, now, "air force: no wing - no position a gain for the %d bombers ready (best %.0f; %d positions, the force's %s; %d bombers in all, %d on runs, anti-air known %d)", nr, bval_ev, nc,
                   nc && cand[0].push ? "weighed" : "none", nb, har_out, d->n);
        }
        as_runs(pl, idx, s, ready, nr, idle, ni, now);
        return;
    }
    // the launch: each target its share, the rest spread over them, the anti-air first
    int nj = dct_prep(pl, idx, &cand[bc], shoot, from, aa, &na, tg, &nt, bombs, hps, worth, gk);
    float lost, dmg, hsum = 0;
    for (int i = 0; i < bn; i++) hsum += (float)unit_get_hp(ready[i]);
    dct_ev(d, na, nt, bombs, hps, worth, gk, bn, hsum / (float)bn, cand[bc].w, best_alloc, &lost, &dmg);
    int sent = 0, ntgt = 0, naa = 0, i = 0, given = 0;
    for (int j = 0; j < nj; j++) { given += best_alloc[j]; ntgt += best_alloc[j] > 0; naa += j < na && best_alloc[j] > 0; }
    for (int j = 0; given < bn && ntgt; j = (j + 1) % nj) if (best_alloc[j] > 0) { best_alloc[j]++; given++; }
    for (int j = 0; j < nj; j++)
        for (int m = 0; m < best_alloc[j] && i < bn; m++, i++) {
            ee_unit *u = ready[i], *t = j < na ? aa[j].u : tg[j - na].u;
            int e = as_held_at(s, unit_get_serial(u));
            if (e < 0) continue;
            if (dct_part(idx, DCP_CLAIM) && !cl_take(u, CLP_TASK, CLW_BOMBERS, AF_RUN_MS)) continue;
            if (!as_order(u, goal_attack(u, t))) { if (dct_part(idx, DCP_CLAIM)) cl_end(u, CLW_BOMBERS); continue; }
            s->held[e].tgt = unit_get_serial(t); s->held[e].t = s->held[e].t0 = now ? now : 1;
            s->held[e].ttype = (short)unit_type_get_id(unit_get_type_data(t)); s->held[e].pred = dmg / (float)bn;
            sent++;
        }
    if (!sent) return;
    s->waves++; s->sent += (unsigned)sent; s->wing_out = 1; s->ev_launch++;
    s->wing_pos = cand[bc].at; s->wing_push = cand[bc].push;
    ar_log(idx, now, "air force: a wing of %d at %d,%d by its worth (%s) - %.0f gain, %.1f bombers lost expected (damage %.0f in all, %.0f hp each) - %d targets, %d of them anti-air (%d bombers in all)",
           sent, cand[bc].at.x, cand[bc].at.y, cand[bc].push ? shoot >= 0 ? "the shooters" : "the force's position" : "enemy buildings", bev, blost, bdmg, hsum / (float)bn, ntgt, naa, nb);
    // the rest to the small runs
    int nr2 = 0, ni2 = 0;
    for (int k = 0; k < nr; k++) { int e = as_held_at(s, unit_get_serial(ready[k])); if (e >= 0 && !s->held[e].tgt) ready[nr2++] = ready[k]; }
    for (int k = 0; k < ni; k++) { int e = as_held_at(s, unit_get_serial(idle[k])); if (e >= 0 && !s->held[e].tgt) idle[ni2++] = idle[k]; }
    as_runs(pl, idx, s, ready, nr2, idle, ni2, now);
}
internal void airstrike_tick(ee_player *pl, uint32_t now) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct as_st *s = &g_as[idx];
    if (!ar_on(pl, idx) && (!pl || is_vanilla(pl) || GAME_AI(idx))) return;
    if (s->t && now >= s->t && now - s->t < AS_EVERY) {
        if (dct_part(idx, DCP_HOME) && PV(idx, g_v_air_strike, AS_ON) && (now < s->home_t || now - s->home_t >= AD_HOME_EVERY)) { s->home_t = now; dct_home(pl, idx, s, now); }   // THE WAY HOME
        return;
    }
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
        fprintf(g_log, "# bombers: P%u at %.1f min: doctrine %d (survive %d, home %d, wing %d, claim %d) | lost %u: wing %u, lone runs %u, called home %u, in the air otherwise %u, in an Airport %u | "
                "sorties %u, back with damage %u, damage taken %.0f (expected %.0f), repaired %.0f | called home %u, went on with no way home %u | targets down %u anti-air, %u buildings, %u units, worth %u | "
                "wings by worth %u, the best worth now %.0f | the guns' fire on bombers in the air: expected %.0f, taken %.0f (expected of guns seen %.0f - in their own range %.0f -, remembered %.0f, assumed %.0f, where lost %.0f) | counter-fire asked %u, runs %u (%u bombers), shooters down %u, worth %u\n",
                idx, now / 60000.0, PV(idx, g_v_bd_on, g_bd_on), dct_part(idx, DCP_SURVIVE), dct_part(idx, DCP_HOME), dct_part(idx, DCP_WING), dct_part(idx, DCP_CLAIM),
                s->lost, s->lost_wing, s->lost_har, s->lost_home, s->lost_air, s->lost_ground, s->sorties, s->dmg_back, s->dmg_taken, s->dmg_pred, s->repaired,
                s->called_home, s->doomed_on, s->down_aa, s->down_bld, s->down_unit, s->down_val, s->ev_launch, s->best_ev, s->obs_pred, s->obs_taken, s->obs_k[0], s->obs_k[4], s->obs_k[1], s->obs_k[2], s->obs_k[3], s->cf_asked, s->cf_runs, s->cf_sent, s->cf_down, s->cf_val);
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
            // THE DOCTRINE's books (every player): a sortie from the look it is out of its Airport to the look it is back in
            // one - the damage it took and was expected to, back with damage; the hit points repaired while it stands there
            __typeof__(s->held[0]) *h = &s->held[m - 1];
            int hp = unit_get_hp(u), in = unit_get_x(u) < 0, mx = st_maxhp(unit_get_type_data(u), unit_get_stats(u));
            if (!in && !h->out) { h->out = 1; h->hp0 = h->hp_last > 0 ? h->hp_last : hp; s->sorties++; }
            else if (in && h->out) {
                h->out = 0; h->nohome = 0;
                if (hp < h->hp0) { s->dmg_taken += (float)(h->hp0 - hp); if (hp * 20 < mx * 19) s->dmg_back++; }
                s->dmg_pred += h->pred; h->pred = 0;
            } else if (in && h->hp_last > 0 && hp > h->hp_last) s->repaired += (float)(hp - h->hp_last);
            if (in || !unit_get_goal(u)) h->home2 = 0;
            h->landed = (uint8_t)in; h->hp_last = hp;
            if (dct_part(idx, DCP_CLAIM)) {   // NO MORE THEFT: a bomber on any task of the air force's is its own (claims.c)
                if (h->tgt || h->har || h->home2 || (h->out && unit_get_goal(u))) cl_take(u, CLP_TASK, CLW_BOMBERS, 3 * AS_EVERY);
                else cl_end(u, CLW_BOMBERS);
            }
        } else {
            s->lost++; s->lost_har += s->held[k].har;
            if (s->held[k].landed) s->lost_ground++;
            else if (s->held[k].home2) s->lost_home++;
            else if (!s->held[k].har) { if (s->held[k].tgt) s->lost_wing++; else s->lost_air++; }
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
        memset(&s->held[s->nh], 0, sizeof s->held[0]);
        s->held[s->nh].id = id; s->held[s->nh].at = unit_get_tile(bomb[i]); s->held[s->nh].hp_last = unit_get_hp(bomb[i]); s->held[s->nh].landed = unit_get_x(bomb[i]) < 0; s->nh++;
        if (s->asked > 0) s->asked--;   // one of those asked for is out
    }
    if (PV(idx, g_v_bd_on, g_bd_on)) dct_scan(pl, idx, s, bomb, nb, now);   // THE DOCTRINE's picture of the anti-air
    af_train(pl, idx, s, nb, now);
    if (AS_RUNS(idx)) ah_watch(pl, s);
    if (dct_part(idx, DCP_WING)) { dct_wing(pl, idx, s, bomb, nb, shoot, now); return; }   // THE VALUE WING in place of the count below
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
    static struct as_tg aa[AS_TARGETS], tg[AS_TARGETS];
    int na = 0, nt = 0;
    as_targets(pl, pos, shoot, 0, aa, &na, tg, &nt);
    if (!na && !nt) { if (shoot >= 0) s->sh[shoot].t = 0; ah_idle(pl, idx, s, bomb, nb, now); return; }   // nothing left at the shooters' spot: the next
    s->needed = AF_FLIGHT * (na + (nt ? 1 : 0)); s->need_t = now ? now : 1;
    // the wing out: a bomber whose target stands keeps at it; idle again, its next run
    static ee_unit *ready[AS_BOMBERS], *idle[AS_BOMBERS];
    int nr = 0, ni = 0, har_out = 0;
    int out = as_wing_look(pl, idx, s, bomb, nb, now, ready, &nr, idle, &ni, &har_out);
    if (out) { as_runs(pl, idx, s, ready, nr, idle, ni, now); return; }   // one wing at a time (the lone runs meanwhile)
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
        if (nr + har_out < s->needed && nr + har_out < AF_WING_MAX) as_runs(pl, idx, s, ready, nr, idle, ni, now);   // else those out come back to make it
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
        s->held[e].ttype = (short)unit_type_get_id(unit_get_type_data(t));
        if (dct_part(idx, DCP_CLAIM)) cl_take(ready[i], CLP_TASK, CLW_BOMBERS, 3 * AS_EVERY);
        sent++;
    }
    if (!sent) return;
    s->waves++; s->sent += (unsigned)sent; s->wing_out = 1;
    ar_log(idx, now, "air force: a wing of %d at %d,%d%s - %d at %d anti-air, the rest at %d targets (%d bombers in all)", sent, pos.x, pos.y,
           shoot >= 0 ? " (the shooters)" : "", na * AF_FLIGHT < sent ? na * AF_FLIGHT : sent, na, nt, nb);
}
internal void af_request(ee_player *pl, ee_unit *u, uint32_t now) {   // counterfire.c: a shooter to strike, if the air force finds a run at it worth it
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct as_st *s = &g_as[idx];
    if (!u || !dct_part(idx, DCP_CLAIM) || !PV(idx, g_v_air_strike, AS_ON)) return;
    int id = unit_get_serial(u);
    for (int k = 0; k < 16; k++) if (s->cf[k].u == u && s->cf[k].id == id) { s->cf[k].t = now ? now : 1; return; }
    s->cf[s->cf_i].u = u; s->cf[s->cf_i].id = id; s->cf[s->cf_i].t = now ? now : 1; s->cf_i = (s->cf_i + 1) & 15;
    s->cf_asked++;
}
internal void airstrike_new_game(void) { memset(g_as, 0, sizeof g_as); memset(g_dct, 0, sizeof g_dct); g_as_nt = -1; }
internal void airstrike_install(const char *autotest_ini) {
    static const char *const k_parts[4] = { "v_bomber_survive", "v_bomber_come_home", "v_bomber_value_wing", "v_bomber_claim" };
    g_as_harass = g_set_ai[1].bomber_harass;
    g_bd_on = g_set_ai[1].bomber_doctrine;
    g_bd_part[DCP_SURVIVE] = g_set_ai[1].bomber_survive; g_bd_part[DCP_HOME] = g_set_ai[1].bomber_come_home;
    g_bd_part[DCP_WING] = g_set_ai[1].bomber_value_wing; g_bd_part[DCP_CLAIM] = g_set_ai[1].bomber_claim;
    if (autotest_ini) {
        g_v_air_strike = ini_int("autotest", "v_air_strike", -1, autotest_ini); g_v_as_harass = ini_int("autotest", "v_bomber_harass", -1, autotest_ini);
        g_v_bd_on = ini_int("autotest", "v_bomber_doctrine", -1, autotest_ini);
        for (int p = 0; p < 4; p++) g_v_bd_part[p] = ini_int("autotest", k_parts[p], -1, autotest_ini);
    }
    if (g_log) fprintf(g_log, "# air force: %d (variant %d), lone runs %d (variant %d), doctrine %d (variant %d; parts %d %d %d %d, variants %d %d %d %d)\n", AS_ON, g_v_air_strike, g_as_harass, g_v_as_harass,
                       g_bd_on, g_v_bd_on, g_bd_part[0], g_bd_part[1], g_bd_part[2], g_bd_part[3], g_v_bd_part[0], g_v_bd_part[1], g_v_bd_part[2], g_v_bd_part[3]);
}
