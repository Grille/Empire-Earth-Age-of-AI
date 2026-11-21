// THE FORCE BY TERRITORY - who stands where, played while the game's computer player is off ([ai_base] game_ai = 0);
// the one force of armies.c does not run then. The player, 2026-11-14: "the single main force isnt the way anymore",
// then the territories' shares (produce.c SHARES: "100% is the full military force and the terriotories distribute
// that under each other"), then the pools: "we should also split the force into subcategories that are requested and
// asigned semiindependently, 1. main-force: unit vs unit 2. siege: building killers, distributed betwen territoris
// planing attacks ... 3. anti air ... 4, explorers a few fast moving units that scout unexplored lland", explorers
// "a temporary role before they are disbaned into the main army share again", and of the attack: "make it push on
// its own, no more attack timers, either its a good idea to attack or its not".
// Every SN_EVERY per AI, on the territory view and produce.c's shares:
//   STATIONS  - every land soldier of its has a station: a territory (where it stands, when it is new). THE MAIN
//               FORCE (soldiers that are no siege, ram or anti-air): a territory's share of them in soldiers; a
//               share worth fewer than SN_MIN goes to the neighbour with the largest (single soldiers hold nothing)
//               unless it is under attack. SIEGE (siege weapons, rams): among the territories planning an attack
//               (beside enemy buildings it knows, by their number) that have a share of the main force - never
//               alone; with no attack planned they stay where they are.
//               A share holds until the new one is off by a quarter, 2 at least (STEADY).
//   GIVING    - a territory with fewer stationed than its share (by a quarter, 2 at least, or by any under attack - those first)
//               is given soldiers of the nearest territories with more than theirs, territory by territory over
//               land: their station changes, they march. SN_MOVES a look.
//   ORDERS    - a soldier away from its station's place goes there (attack-move: it fights what it meets): the
//               middle of the enemy soldiers seen in the territory when there are any (an attack: its share is
//               there to meet it), else a Town Center, Capitol, Settlement or other building of its there, else
//               the territory's middle. Asked again when it stands idle elsewhere.
//   A PUSH    - no attack time: a territory it holds with SN_PUSH_MIN of the main force stationed, three quarters
//               of them there, beside a territory with enemy buildings: its soldiers and siege against what it
//               knows of the enemy at the nearest of those buildings (stage.c stage_foe: soldiers seen, defences,
//               the remembered) - from SN_PUSH they go, attack-move at the building, on to the next when it is
//               down; under SN_BACK they come back to their station and SN_PUSH_WAIT passes before the next look.
//               No building left there: the push is over, the shares say where they stand next.
//   SCOUTS    - while it has land territories no unit of its ever stood in (on the land the scout stands on) up
//               to SN_SCOUTS of the main force, the fastest (of equally fast ones the one that sees furthest), go to the nearest of them one after the other (one
//               not reached in SN_SCOUT_GIVE_UP is given up); none left: they are the main force's again,
//               stationed where they stand, and none is taken for SN_SCOUT_REST.
//               A LOOK AGAIN (the player 2026-11-16: "the ai does it only at the start to uncover the map, but it should
//               after regularly scout neutral and enemy territory for changes"): a land territory it doesn't hold that
//               no unit of its has stood in for SN_RESCOUT is due again, as one never stood in (those first); one
//               given up is tried again after SN_RESCOUT too. What a scout sees there is what the AI knows of it: the
//               enemy buildings by territory are those in sight or remembered (attack.c seen_by), no longer every one
//               on ground it ever explored - a computer player has the whole map explored from the start.
//   PATROLS   - single soldiers of the main force at the patches of its own ground it has long had out of sight
//               (watch.c), patch after patch; the main force's again when none is left (below, [ai_base] fog_patrols).
//   A SCENARIO'S AI TOGGLES (the player 2026-11-16: "respecting the ai flags (aktiv, cheat, angreifen, etc)"; the
//               editor's player dialog and the trigger Set AI Toggle, player_ai_toggle): Attack off - no push, the
//               ones on are over; Explore off - no scouts, the ones out the main force's again. Active off: the game
//               calls no update of its at all (nor the mod's); Build/Repair and Gather: eeaimod.c rm_tick.
//   DEMOLITION - a territory it has taken (its own, no enemy building but walls left, no enemy soldier there, no push
//               from it) has the enemy's wall pieces and gates in it brought down, a low task: SN_DEMO of its idle
//               main force and siege stationed there at the piece nearest its place, one after the other (the player
//               2026-11-15: "after taking a territory the ai should "deconstruct" enemy walls in it as a low priorethy
//               task"). A fight there or a push from there takes them first.
//   COAST BATTERIES - its siege at rest at posts along the coasts of its front seas (below, [ai_base] coast_siege).
// Not here yet: anti-air, aircraft and ships by role (the player: "do it by role").
// "# stations:" a minute.
#define SN_EVERY 5000
#define SN_UNITS 1024
#define SN_MIN 3
#define SN_BAND 1 / 4             // STEADY: a share moves, a territory is short, from this much of it
#define SN_BAND_MIN 2            // ... soldiers at least
#define SN_MOVES 16
#define SN_NEAR 10
#define SN_REORDER 30000
#define SN_STILL 180000          // A STUCK ORDER: this long within SN_STILL_D tiles since its order
#define SN_STILL_D 4
#define SN_PUSH_MIN 6
#define SN_GIVE_STEPS 3          // territories a soldier is sent at most
#define SN_GIVE_STEPS_ATTACK 5   // ... to one under attack
#define SN_GROUP 4               // AS A GROUP: soldiers that march together at least
#define SN_GROUP_WAIT 45000      // ... or the first of them has waited this long
#define SN_GROUPS 64
#define SN_DEMO 3                // DEMOLITION: soldiers a territory sets at the enemy's walls in it at most
#define SN_STAY 180000           // a soldier given a station keeps it this long, but for an attack
#define SN_GROUP_MAX 160
#define SN_AA_BLD 20
#define SN_AA_SEEN 30
#define SN_PUSH 1.5f
#define SN_BACK 0.8f
#define SN_PUSH_WAIT 120000
#define SN_CALL 240000           // THE CALL: how long the called gather before another call
#define SN_PUSHES 6
#define SN_SCOUTS 3
#define SN_SCOUT_GIVE_UP 180000
#define SN_SCOUT_REST 300000
#define SN_RESCOUT 600000       // A LOOK AGAIN: a territory it doesn't hold, this long after a unit of its last stood in it
#define SN_EB 512
#define SN_FORM_IN 16            // A STANDING FORMATION: a soldier within this of its patch is the formation's (farther: the march there)
#define SN_FORM_FAR 18           // ... the patch searched within this of the station's point
#define SN_FORM_LOOK 60000       // ... looked for again after this
#define SN_FORM_AGAIN 20000      // ... laid out again at most this often
#define SN_FORM_MAX 512          // ... soldiers at rest a look
#define SN_FC 256                // ... territories a player remembers a patch for
enum { SNP_MAIN, SNP_SIEGE, SNP_AA, SNP_N };
struct sn_unit { ee_unit *u; int id; short ter; uint8_t pool, scout, post; int8_t stance0; uint32_t order_t, since, wait_t, moved_t, stash_t, still_t; ivec2 to, still, look; };   // look: a patrol's patch (PATROLS; scout 3), since: taken then   // still: where it stood since still_t (A STUCK ORDER)   // stash_t: sent into a fortress then (THE RESERVE)   // wait_t: it waits for a group to march with since then (AS A GROUP)   // since: a scout's territory is its goal since then
#define SN_CITS 6                // THE BRIDGEHEAD: the citizens a push across takes along at most
#define SN_BH 8                  // ... the buildings it lays by the beach (A BASE OVER THERE)
#define SN_BH_JOIN 12            // ... a soldier new this near a production building of it joins the push
#define SN_BH_EVERY 10000        // ... one laid this often at most
#define SN_BH_PAY 180000         // ... one it can't build or pay this long: dropped
#define SN_LAST 1.0f             // THE LAST WORD: what is aboard, its losses planned for, judged at least this against the beach and the target
#define SN_LAST_WEAK 20000       // ... judged less this long: called off
#define SN_CALLED_OFF 600000     // ... then no push from that territory this long
struct sn_bh { int kind, type, state, bid; ee_unit *b; ivec2 p; uint32_t t; };   // kind 0 a tower, 1 a Hospital, 2 a Temple, 3 a production building, 4 a Town Center (else a Settlement); state 0 to lay (t: since), 1 going up, 2 standing, 3 dropped or destroyed
struct sn_push { short from, into; int on; uint32_t end_t; ivec2 at, mmid; int mn;   // mmid, mn: the middle of its soldiers (no siege), how many (THE SIEGE'S ESCORT)
                 int across, left, over, waves; unsigned from_land, to_land; ivec2 beach; uint32_t start_t, wave_t;   // waves: set out so far, wave_t: the next one waits for transports since then (ferry.c ONE WAVE AT A TIME); ACROSS: over the water - its lands, the beach; soldiers of it still on the land it left, and over on the other
                 uint32_t way_t; int breach, round_, straight; ivec2 wall; uint32_t near_t, force_t, wait, hold_t; int near_d; ivec2 near_at;   // hold_t: THE CLEARED GROUND is held since then   // STUCK: near_d the nearest its middle came to near_at (squared), at near_t; force_t: a breach THE WAY did not find, since then; wait: before the next push from there (0: SN_PUSH_WAIT)   // THE WAY: looked at way_t; breach: a wall to bring down at `wall`; the walk round and through, in cells
                 struct ex_unit cit[SN_CITS]; int ncit, cits_want, nbh, sup_oc, recruits; struct sn_bh bh[SN_BH]; uint32_t bh_t, weak_t, judge_t; float judge_r; ivec2 sup_at; };   // THE BRIDGEHEAD: its citizens ashore, how many it takes along, its buildings, the last laid; THE LAST WORD: judged too weak since weak_t, last at judge_t (judge_r); THE SQUADRON AHEAD: the water off its beach, that sea
// ACROSS (push_across, OFF): a push into a territory across the water - the ferry (ferry.c) carries its soldiers.
// MEASURED 2026-11-18 (head-to-head, mirrored, 12 seeds, both sides expand_across without the outposts): soldiers -21.6
// (se 4.2), kills -17.5 (7.8) - 159 pushes, 109 not carried over in 8 minutes; every one judged against nothing (an
// enemy home island it had never looked at: its towers and army unknown), 6 to 24 soldiers, the transports few and
// trained one by one (trips of 1 to 3). Then (the player: "the territory system has scouts, they should be able to use
// ferrys too"; "landings into hostile situations should be coordinated to arrive in parallel"): only into a territory a
// unit of its stood in lately (SCOUTS ACROSS bring them over), its transports gathered first and meeting off the beach
// (ferry.c GATHERED FIRST, TOGETHER) - soldiers -14.3 (3.8), kills +0.1 (9.2), losses +9.3 (6.9), razed +0.7 (0.65);
// 83 pushes: 35 not carried over in time, 23 losing, 5 claimed. Still more cost than gain: OFF. Why worse than
// landing.c's landings (the player: "look why its still worse than the old landings"): the push's soldiers went over one
// by one as they were trained (504 trips for 53 pushes, 196 with one soldier, 5.5 minutes from the first to the last),
// each late transport alone at the meeting point, which waited for trips still loading at home - and landing.c took the
// same soldiers and transports meanwhile. Now as a landing (ferry.c ONE WAVE AT A TIME, BOARD TOGETHER, the meeting
// point by crossing.c cx_meet), measured in the landings' place (v_landing=0 v_push_across=1 against the defaults, same
// test): trips of 8-12 mostly, but still soldiers -10.1 (4.2), kills -1.4 (7.3), losses +9.5 (7.4), razed -0.3 (0.4);
// 67 pushes, 39 not carried over in time, 2 claimed; of 248 trips 106 put off, 57 put on a beach under fire, 74 sunk.
// The landings themselves, against none (v_landing=0): soldiers +7.0 (4.4), kills +4.5 (6.4), razed -0.6 (0.3), and
// more held across the water without them (citizens off the home island 17.2 -> 24.6, buildings there 5.6 -> 8.0).
// Then what landing.c did and the pushes didn't (the player: "yes do that"): PLANNED FOR LOSSES, THE LAST WORD, THE
// SQUADRON AHEAD (squadrons.c), THE BRIDGEHEAD and A BASE OVER THERE (its citizens), and in ferry.c THE WHOLE PUSH FIRST
// (in places: IN PLACES) and THE TRANSPORTS HELD - the same test, 24 games: soldiers +8.9 (se 5.1), kills +9.7 (7.9),
// losses -7.6 (8.0), razed +0.0 (0.4); held at the end, the pushes against the landings: islands 1.33 against 1.15,
// citizens off the home island 25.2 against 16.2, miners there 8.8 against 6.4, drop sites 4.8 against 3.5, buildings 8.9
// against 6.1; territories cleared and claimed 11 against 4. 45 pushes, 24 still called off by THE LAST WORD (the first
// waves 17.4 of 24.7 soldiers on 2.1 transports, none on one), 11 not carried over in time, 9 losing; the bridgehead
// laid 12 towers, 8 production buildings, 4 Hospitals, 2 Temples, a town building - 16 of them standing at the end, 7
// destroyed. Better than landing.c in its place - the switch (landings off, pushes across on, the game's computer
// player off) the player's call; OFF until then. Aboard a
// unit is on no tile and leaves the look's list; put off it is new to it - so the ferry hands its passengers back here
// (sn_ferried) and a new one of them takes the push's territory as its station again
#define PUSH_ACROSS(idx) PV(idx, g_v_sn_across, g_sn_across)
#define SN_JOIN 128
internal struct { int id; short ter; uint8_t scout; uint32_t t; } g_sn_join[16][SN_JOIN];
internal void sn_ferried(unsigned idx, ee_unit *u, int ter, int scout) {
    int k = 0;
    uint32_t now = game_time();
    for (int j = 1; j < SN_JOIN; j++) if (g_sn_join[idx & 15][j].t < g_sn_join[idx & 15][k].t) k = j;
    g_sn_join[idx & 15][k].id = unit_get_serial(u); g_sn_join[idx & 15][k].ter = (short)ter; g_sn_join[idx & 15][k].scout = (uint8_t)scout; g_sn_join[idx & 15][k].t = now ? now : 1;
}
// SCOUTS ACROSS (expand_across): a scout whose next territory lies across the water asks the ferry (trip kind 4) - one
// transport for it, a crossing that is no important one (ferry_safety: it waits for a safe sea)
#define SN_SX 4
internal struct { ee_unit *u; int id; unsigned from, to; ivec2 at; int ter; uint32_t t; } g_sn_sx[16][SN_SX];
struct sn_unit;
internal void sn_rejoin(unsigned idx, struct sn_unit *m);
internal int fy_push_transit(unsigned idx, int k);   // ferry.c: push k's soldiers the ferry has taken (to the shore, aboard)
internal struct sn_st {
    uint32_t t, log_t, scout_off, step_t;   // scout_off: no scouts before then; step_t: THE NEXT GOAL AT ONCE
    int n;
    struct sn_unit m[SN_UNITS];
    struct sn_push push[SN_PUSHES];
    short tc[PR_T];   // STEADY: the territories' shares in soldiers as of the last look
    uint32_t calm_t[PR_T];   // no push from that territory before then (its last push's end + wait) - its own: kept in the push's slot, the wait was lost when a later push took the slot over
    uint32_t res_t, res_out_t, res_in_t; unsigned res_in, res_out; ee_unit *res_last; int res_last_n, res_room, res_claim;   // THE RESERVE: its last look, the last fortress let out, the last load; soldiers in, out; the room spare and the training's claim at the last look
    unsigned moved, orders, pushes, won, lost, scouted, looks, breaches, groups, grouped, demos, stale, formed, gates; int due;   // looks: scouts' goals inside their territory (THROUGH THE TERRITORY)   // gates: siege set at an enemy gate (SIEGE AT THE GATES)   // formed: formations laid out at rest (A STANDING FORMATION)   // due: territories a scout goes to (A LOOK AGAIN)   // stale: orders taken for dead (A STUCK ORDER)   // demos: soldiers set at an enemy wall (DEMOLITION)   // groups: orders to several at once, grouped: the soldiers in them
    int need_on; ivec2 need_at;   // where the force meets the enemy next (sn_need_at)
    ivec2 esc_at; uint32_t esc_t; int esc_n, esc_fight; unsigned escorts, quarantined, cured;   // quarantined, cured: QUARANTINE's so far   // AN ESCORT: whom it follows (where), asked last then, how many; soldiers given so far
    int call_r; uint32_t call_t; unsigned calls;   // THE CALL: the territory the others' soldiers gather in for a push, since then; calls so far
} g_sn[16];
internal uint8_t g_sn_seen[16][PR_T];
internal uint32_t g_sn_look[16][PR_T];   // A LOOK AGAIN: when a unit of its last stood in each territory (or a scout gave it up)
internal uint32_t g_sn_dark_t[16][PR_T];   // A LOOK INSIDE: when a scout in it last found no ground there to explore
// a territory a scout goes to: never stood in, or with ground it hasn't explored (A LOOK INSIDE: explore.c - unless a
// scout in it found none to reach in the last SN_RESCOUT) (2: those first), or one it doesn't hold not stood in for
// SN_RESCOUT (1)
internal int sn_due(unsigned idx, int r, const struct tv_st *s, uint32_t now) {
    if (g_ter.t[r].water) return 0;
    if (!g_sn_seen[idx][r]) return 2;
    if (xp_ter_dark(idx, r) && (!g_sn_dark_t[idx][r] || now < g_sn_dark_t[idx][r] || now - g_sn_dark_t[idx][r] >= SN_RESCOUT)) return 2;
    return s->t[r].role == TVR_NONE && (now < g_sn_look[idx][r] || now - g_sn_look[idx][r] >= SN_RESCOUT);
}
internal struct st_side g_sn_own;
// TRAINING BY NEED's place with the game's computer player off (armies.c ar_need_pick: what to train is what adds most
// against the enemy there): a push's building, else the middle of the enemy soldiers in the territory of its with the
// most of them, else the enemy building it knows nearest the territory with the largest share. 0: none known.
internal int sn_need_at(unsigned idx, ivec2 *at) { *at = g_sn[idx & 15].need_at; return g_sn[idx & 15].need_on; }
// AN ESCORT (the player 2026-11-16, agreeing to it: a prophet's strike, holy.c, with soldiers beside it): asked again
// every look while it is wanted - n soldiers of the main force follow at (the prophet where it stands now), the
// nearest of territories not pushing and with no enemy soldiers in them, marked as scouts (scout 2: no share, no push
// takes them); SN_ESC_KEEP after the last ask they are the main force's again, stationed where they stand. They walk
// (a plain move) and stand their ground (stance 1) - the player 2026-11-16: "the escort also shoudlnt attack, except to
// defend the prophet": fight set (enemy soldiers by the prophet) they attack-move to it; let go, their stance is
// what it was. sn_escort_near: how many it has, how many of them within r of at (holy.c: the prophet waits for them)
#define SN_ESC_KEEP 10000
#define SN_HOSP_SIDE 2       // QUARANTINE: its place this far from a Hospital's middle (its size 3: 0.5 from its edge, in the reach of 2)
#define SN_TEMPLE_R 9        // ... a Temple's reach from its middle (8 from its edge, without upgrades)
internal void sn_escort(unsigned idx, ivec2 at, int n, int fight, uint32_t now) { struct sn_st *z = &g_sn[idx & 15]; z->esc_at = at; z->esc_n = n; z->esc_fight = fight; z->esc_t = now ? now : 1; }
internal int sn_escort_near(unsigned idx, ivec2 at, int r, int *given) {
    struct sn_st *z = &g_sn[idx & 15];
    int nr = 0, n = 0;
    for (int i = 0; i < z->n; i++) {
        if (z->m[i].scout != 2 || !z->m[i].u) continue;
        if (unit_get_hp(z->m[i].u) <= 0) continue;
        n++; nr += ivec2_dist2(unit_get_tile(z->m[i].u), at) <= r * r;
    }
    *given = n;
    return nr;
}
internal int sn_prophet_escorted(unsigned idx) { int n = 0; sn_escort_near(idx, iv2(0, 0), 0, &n); return n > 0; }
// PROPHETS ACROSS (ferry.c): the escort on land (area_id's), up to max - aboard the prophet's transport with it; and the
// escort kept while the prophet is aboard (holy.c asks nothing of it meanwhile)
internal int sn_escort_units(unsigned idx, unsigned land, ee_unit **out, int max) {
    struct sn_st *z = &g_sn[idx & 15];
    int n = 0;
    for (int i = 0; i < z->n && n < max; i++)
        if (z->m[i].scout == 2 && z->m[i].u && unit_get_hp(z->m[i].u) > 0 && unit_get_x(z->m[i].u) >= 0 && area_id(unit_get_tile(z->m[i].u), 2) == land) out[n++] = z->m[i].u;
    return n;
}
internal void sn_escort_keep(unsigned idx, uint32_t now) { struct sn_st *z = &g_sn[idx & 15]; if (z->esc_t) z->esc_t = now ? now : 1; }
internal int holy_prophets(unsigned idx);   // holy.c: its prophets (TEMPLE FIRST)
// THE WAY (the player 2026-11-15: "ai attacks dont cope well with walls yet. the attack should actual try to pathfind to
// the target. maybe 2 paths? on with walls and one ignoring them, if the path with wall is a larger detour the ai bring
// siege to destroy walls"): from a push's soldiers to its building two ways over the cells of armies.c's route search -
// one as if no enemy wall stood, one with every cell of a wall or gate it has seen shut. No wall on the first: nothing
// to do. The second no more than SN_ROUND times the first (and SN_ROUND_MORE cells): the walk round, which the game's
// own pathfinding takes by itself. Longer, or none: A BREACH - the first wall on the way through is what the push
// goes at: its siege (the soldiers too when it has none) attacks the piece, the rest stands by it; looked at again
// every SN_WAY_EVERY, so the next piece follows and the push goes on once the way is open.
// 1 a breach (*wall the piece's tile), 0 none; *round_, *straight the two ways in cells (0 none)
#define SN_ROUND 2
#define SN_ROUND_MORE 10
#define SN_WAY_EVERY 15000
#define SN_RES_EVERY 10000      // THE RESERVE: a look
#define SN_RES_ROOM 5           // ... soldiers go in with this many places left on the map
#define SN_RES_BATCH 20         // ... this many a look
#define SN_RES_FOE 4            // ... or with this many enemy soldiers in a territory of its
// THE SIEGE'S ESCORT (the player 2026-11-16: "sometimes siege attacks completly alone, huge waste of resources, for
// pushes it should only funtionas attachment to a force"): in a push the siege goes to the middle of the push's
// soldiers (the main force), never to the building itself; at a breach it attacks the wall piece only with them within
// SN_SIEGE_NEAR of it; a push with fewer than SN_SIEGE_ESCORT soldiers left is over (siege counted as somebody before:
// the soldiers dead, the catapults went on alone); enemies in a territory of its: the siege goes at them only with
// SN_SIEGE_ESCORT soldiers stationed there, else it stands at the station's place
#define SN_SIEGE_ESCORT 3
#define SN_SIEGE_NEAR 12
// SIEGE AT THE GATES (the player 2026-11-17: gates as the force's targets did nothing - "gates arent attacked by units
// automaticly (good, it would bind them for to long) ... ignored like other walls, but siege shoulkd take intrest in them
// and attack them directly"): a siege weapon in a push, or in a territory of its with enemies, attacks the nearest enemy
// gate it knows (seen or remembered, attack.c seen_by) within SN_GATE_R of it - escorted as THE SIEGE'S ESCORT has it:
// the push's soldiers within SN_SIEGE_NEAR, SN_SIEGE_ESCORT stationed in the territory. The soldiers leave gates alone.
#define SN_GATE_R 12
// RAMS AT THE TOWERS ([autotest] v_ram_towers 1, 0 none; the player 2026-11-19, his game: "the ai isnt using the samsons
// effectivly, clearing towers is one of its best use. both player have some samsons on p2 homeisland, but both stand
// just around"): outside a push the siege had nothing to do but gates and walls, and stood in its formation however near
// an enemy tower stood. The rams (UTR_RAM: the Sampson too) of a territory not pushing - SN_RAM_MIN of them at least -
// go together at the enemy building nearest their middle within SN_RAM_R on their land that it knows (seen_by): a
// defence first, else any but a wall, where no enemy soldier it knows stands within SN_RAM_SAFE (a ram can't hit one:
// the soldiers' business); they stay on it until it is down, then the next. Measured with armies.c's SIEGE WITH A
// PURPOSE (its note): no cost shown, never 3 rams together in those test games; on.
// RAMS WITH THE SOLDIERS (v_ram_towers 2; the player 2026-11-19, his game again: "the ai is still to conservative with
// its rams ... they stay mostly in the back when they should clear towers and building with the other soilders, they
// also look somewhat undecided"): at 109 min 15 of P2's rams stood idle or walking 5-9 tiles from a P3 tower its
// soldiers were hitting, and at one building they were sent again 8 times in 40 s (15, 6, 2, 3, 2, 3, 1, 1). Three
// causes: a building was left alone with any enemy soldier known near it - the citizens' raiders_known, one seen or
// remembered from the last 3 minutes whose reach takes the building in, the enemy's own rams too (P3 had 80) - which in
// a fight is always; a pushing territory's rams walked behind its soldiers (THE SIEGE'S ESCORT) and never hit one; a
// look that found no building let the formation or the enemy soldiers' middle take back the rams already on one. Now
// a building is the rams' with SN_SIEGE_ESCORT soldiers of the player within SN_RAM_ESC of it - they fight what stands
// there, the rams are their attachment - pushing or not, a single ram too; without them, as before, only SN_RAM_MIN
// rams together and with no enemy that can hurt a ram (a ram can't) in sight within SN_RAM_SAFE, and not in a push. A
// ram at an enemy building stays on it whatever the look finds; a defence first, then the one rams of its are on.
// Siege Towers and Trojan Horses (rams without an attack) are no rams here. MEASURED 2026-11-19 (head-to-head,
// Continental, 4 AIs from epoch 2 - the Sampson's -, 120 min, seeds 2-14 mirrored, 24 games; the island test from
// epoch 5 gives no ram an order; B as before, v_ram_towers=1): even - B - A alive -0.10 then +0.00 (se 0.19, 0.17),
// soldiers -9.1 then +10.8 (18.0, 12.1), razed -7.8 then +18.3 (30.4, 21.0) over 10 and 14 games; the games where A's
// rams were sent 5 times or more: A 6 won, 1 lost, 4 neither (a side ahead pushes, and its rams with it - no proof).
// The old way's rams got an order in 3 of the 24 games. On: the player's call ("to conservative").
#define SN_RAM_MIN 3
#define SN_RAM_R 30
#define SN_RAM_SAFE 8
#define SN_RAM_ESC 10
internal int g_sn_rams = 2, g_v_sn_rams = -1;
#define RAM_TOWERS(idx) PV(idx, g_v_sn_rams, g_sn_rams)
internal int sn_is_ram(ee_unit *u) { ee_unit_type *td = unit_get_type_data(u); return td && ut_role(unit_type_get_id(td), UTR_RAM) && unit_type_get_attack(td) > 0; }   // not a Siege Tower, a Trojan Horse (no attack)
// an enemy of pl that can hurt a ram in sight within r of p (soldiers, ships by the shore; no rams, no citizens)
internal int sn_ram_foes(ee_player *pl, ivec2 p, int r) {
    for (struct grid_it it = grid_it_box(p.x, p.y, r, 1); grid_it_next(&it); ) {
        int fam = unit_type_get_class_id(it.td), t = unit_type_get_id(it.td);
        if ((fam != CLS_LAND && fam != CLS_NAVAL) || is_citizen_type(t) || ut_role(t, UTR_RAM) || unit_type_get_attack(it.td) <= 0 || unit_get_hp(it.u) <= 0) continue;
        ee_player *o = unit_get_owner(it.u);
        if (o && player_get_index(o) > 0 && is_enemy(pl, o) && seen_by(it.u, pl)) return 1;
    }
    return 0;
}
// the enemy building u is a ram's target already
internal int sn_ram_at(ee_player *pl, ee_unit *u) {
    ee_goal *cg = unit_get_goal(u);
    ee_unit *tg = cg && goal_is_attack(cg) ? goal_get_target(cg) : NULL;
    if (!tg || unit_get_hp(tg) <= 0 || !unit_get_owner(tg) || !is_enemy(pl, unit_get_owner(tg))) return 0;
    ee_unit_type *td = unit_get_type_data(tg);
    int f = td ? unit_type_get_class_id(td) : 0;
    return f == CLS_BUILDING || f == CLS_FORTRESS;
}
// the rams of territory r at the building nearest their middle: how many sent; done[] marks those it orders and those
// at a building already (RAMS WITH THE SOLDIERS: the stations leave them there). pushing: r's push is on.
internal int sn_rams(ee_player *pl, unsigned idx, struct sn_st *z, int nn, int r, int pushing, uint8_t *done, uint32_t now) {
    static short ri[SN_UNITS];
    static ivec2 sp[SN_UNITS];   // RAMS WITH THE SOLDIERS: the soldiers' tiles
    static ee_unit *on[SN_UNITS];
    int nr = 0, ns = 0, non = 0, with = RAM_TOWERS(idx) == 2;
    long sx = 0, sy = 0;
    for (int i = 0; i < nn; i++) {
        struct sn_unit *m = &z->m[i];
        if (with && m->pool == SNP_MAIN && !m->scout && unit_get_x(m->u) >= 0 && unit_get_hp(m->u) > 0) sp[ns++] = unit_get_tile(m->u);
        if (m->ter != r || m->scout || m->pool != SNP_SIEGE || !sn_is_ram(m->u) || unit_get_x(m->u) < 0 || unit_get_hp(m->u) <= 0 || !cl_may(m->u, CLP_ROUTINE, CLW_STATION)) continue;
        if (with && sn_ram_at(pl, m->u)) {   // at a building: on it till it is down
            done[i] = 1;
            ee_goal *cg = unit_get_goal(m->u);
            if (non < SN_UNITS) on[non++] = goal_get_target(cg);
        }
        ri[nr++] = (short)i; sx += unit_get_x(m->u); sy += unit_get_y(m->u);
    }
    if (nr < 1 || (!with && (nr < SN_RAM_MIN || pushing))) return 0;
    ivec2 c = iv2((int)(sx / nr), (int)(sy / nr));
    ee_unit *best = NULL;
    int bd = 0x7FFFFFFF, besc = 0;
    for (struct grid_it it = grid_it_box(c.x, c.y, SN_RAM_R, 1); grid_it_next(&it); ) {
        ee_player *o = unit_get_owner(it.u);
        int fam = unit_type_get_class_id(it.td), t = unit_type_get_id(it.td);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || (fam != CLS_BUILDING && fam != CLS_FORTRESS) || is_wall(t) || unit_get_hp(it.u) <= 0 || !seen_by(it.u, pl)) continue;
        ivec2 q = unit_get_tile(it.u);
        if (ivec2_dist2(q, c) > SN_RAM_R * SN_RAM_R) continue;
        unsigned land = area_id(q, 2);
        int nl = 0;   // its rams on the building's land
        for (int k = 0; k < nr; k++) nl += area_id(unit_get_tile(z->m[ri[k]].u), 2) == land;
        if (!nl) continue;
        float rg, at;
        int dfc = defence_stats(it.u, &rg, &at), d = ivec2_dist2(q, c) - (dfc ? 1 << 22 : 0), esc = 0;   // a defence first
        for (int k = 0; k < non; k++) if (on[k] == it.u) { d -= 1 << 20; break; }   // ... then the one rams are on
        if (d >= bd) continue;
        if (with) for (int k = 0; k < ns && esc < SN_SIEGE_ESCORT; k++) esc += ivec2_dist2(sp[k], q) <= SN_RAM_ESC * SN_RAM_ESC;
        if (esc < SN_SIEGE_ESCORT) {   // alone: as RAMS AT THE TOWERS
            if (pushing || nl < SN_RAM_MIN) continue;
            if (with ? sn_ram_foes(pl, q, SN_RAM_SAFE) : raiders_known(pl, q, SN_RAM_SAFE)) continue;
        }
        bd = d; best = it.u; besc = esc >= SN_SIEGE_ESCORT;
    }
    if (!best) return 0;
    unsigned land = area_id(unit_get_tile(best), 2);
    int sent = 0;
    for (int k = 0; k < nr; k++) {
        struct sn_unit *m = &z->m[ri[k]];
        if (area_id(unit_get_tile(m->u), 2) != land) continue;
        done[ri[k]] = 1;
        if (sn_ram_at(pl, m->u)) continue;   // at a building already
        ee_goal *g = goal_attack(m->u, best);
        if (g && unit_goal(m->u, g, 0, 0x1F40)) { m->order_t = now ? now : 1; m->to = unit_get_tile(best); m->wait_t = 0; z->orders++; sent++; }
    }
    if (sent && g_log) log_locked("# stations: P%u at %.1f min: %d rams of territory %d at the enemy's %s (type %d) at %d,%d%s%s\n", idx, now / 60000.0, sent, r,
                                  bd < -(1 << 21) ? "defence" : "building", unit_type_get_id(unit_get_type_data(best)), unit_get_x(best), unit_get_y(best),
                                  besc ? ", with its soldiers" : "", pushing ? " (pushing)" : "");
    return sent;
}
#define SN_RES_ODDS 3           // ... unless its soldiers stationed there are this many times as many
#define SN_RES_LOOK 120         // ... at most this many a look (several Fortresses)
#define SN_RES_AHEAD 2          // ... Fortress room wanted ahead: this many units per production building
#define SN_RES_AGAIN 60000      // ... and nobody goes in for this long after
#define SN_HOLD 600000          // THE CLEARED GROUND: held this long at most
#define SN_STUCK 120000        // STUCK: no nearer to its building for this long
#define SN_STUCK_AT 14          // ... and further from it than this (tiles): at it, it fights
#define SN_STUCK_WALL 12        // the enemy wall to breach then: within this of its soldiers
#define SN_STUCK_WAIT 600000    // none: before that territory pushes again
// A SCENARIO'S TARGETS (scen_targets.c: the trigger Modify AI Military Target's lists): an attack target's territory is where
// a push goes first, at the target (a unit's tile, an area's point; an area reached - SN_TARGET_AT - is pushed as any
// other: its buildings, or the ground held), and of the neighbours of a territory that may push the one nearest an
// attack target is taken when none holds one - the way there; a defend target's territory has at least SN_DEFEND
// percent of the main force (shared by the defend targets), from the territories with the largest shares
#define SN_TARGETS 32
#define SN_TARGET_AT 8
#define SN_DEFEND 40
// the first attack target in territory r (-1 none)
internal int sn_target_in(const struct scen_target *st, int nst, int r) {
    for (int i = 0; i < nst; i++) if (!st[i].defend && ter_near(st[i].at.x, st[i].at.y) == (unsigned)r) return i;
    return -1;
}
internal int sn_way(ee_player *pl, unsigned idx, ivec2 from, ivec2 to, ivec2 *wall, int *round_, int *straight, uint32_t now) {
    static int path[65536];
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0, d = 0, w = 0;
    *round_ = *straight = 0; (void)pl;
    int n = size > 0 ? ar_route_cells(idx, size, (unsigned short)map_area(from, 2), &g_armies[idx], now) : 0;
    if (!n) return 0;
    g_arr_walls = 2;
    int len = ar_route_find(n, from, to, path, 65536, &d, &w), first = -1;
    for (int j = len - 1; j >= 0 && first < 0; j--) if (g_arr.wall[path[j]]) first = path[j];   // from the start: path runs from the building back
    *straight = len;
    if (len && first >= 0) { g_arr_walls = 1; *round_ = ar_route_find(n, from, to, path, 65536, &d, &w); }
    g_arr_walls = 0;
    if (g_log && g_autotest) {   // tests: every look
        int wc = 0;
        for (int c = 0; c < n * n; c++) wc += g_arr.wall[c];
        log_locked("# way: P%u at %.1f min: %d,%d -> %d,%d: the way through %d cells%s, the walk round %d; wall cells it knows %d\n", idx, now / 60000.0, from.x, from.y, to.x, to.y, len, first >= 0 ? " over a wall" : "", *round_, wc);
    }
    if (!len || first < 0 || (*round_ && *round_ <= len * SN_ROUND + SN_ROUND_MORE)) return 0;
    ivec2 c = iv2((first % n) * AR_CELL + AR_CELL / 2, (first / n) * AR_CELL + AR_CELL / 2);
    *wall = c;
    return 1;
}
// the enemy wall piece or gate it has seen nearest p, within 4 tiles
internal ee_unit *sn_wall_near(ee_player *pl, ivec2 p, int reach) {
    ee_unit *best = NULL;
    int bd = 0;
    for (struct grid_it it = grid_it_box(p.x, p.y, reach, 1); grid_it_next(&it); ) {
        ee_player *o = unit_get_owner(it.u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || !(is_wall(unit_type_get_id(it.td)) || unit_type_get_class_id(it.td) == CLS_GATE) || !seen_by(it.u, pl)) continue;
        int d = ivec2_dist2(unit_get_tile(it.u), p);
        if (!best || d < bd) { best = it.u; bd = d; }
    }
    return best;
}
internal ee_unit *sn_wall_at(ee_player *pl, ivec2 p) { return sn_wall_near(pl, p, 4); }
internal int sn_cmp(const void *a, const void *b) { const struct sn_unit *x = a, *y = b; return x->u < y->u ? -1 : x->u > y->u; }
// a soldier the look no longer has (aboard a ferry - PROPHETS ACROSS takes the escort along -, inside a fortress): the
// stance AN ESCORT or a coast post gave it goes back to what it was - it comes back as a new entry, its stance0 lost
// with the old one, and stood its ground for good
internal void sn_stance_back(const struct sn_unit *old, int on, ee_unit *u) {
    struct sn_unit key = { .u = u }, *o = on ? bsearch(&key, old, (size_t)on, sizeof *old, sn_cmp) : NULL;
    if (o && o->id == unit_get_serial(u) && (o->scout == 2 || o->post) && o->stance0 >= 0) unit_set_stance(u, o->stance0);
}
// A STUCK ORDER (the player 2026-11-16, of yellow in "aistuck": "the yellow ai seemed genuinly dead, no reinforcments
// or moves, as if something got stuck completly" - from minute 84 to 101 not one order, 34 of its 53 soldiers on an
// attack and 19 on a move the game never ended; a soldier is ordered again only when it stands idle, and a load, which
// forgets the orders given, set it going at once): a soldier that has not come SN_STILL_D tiles from where it stood
// for SN_STILL since its last order counts as idle - its order is taken for dead.
internal int sn_idle(struct sn_st *z, struct sn_unit *m, uint32_t now) {
    if (ar_idle(m->u)) return 1;
    if (unit_get_x(m->u) < 0) return 0;   // inside
    ivec2 p = unit_get_tile(m->u);
    if (!m->still_t || now < m->still_t || ivec2_dist2(p, m->still) >= SN_STILL_D * SN_STILL_D) { m->still = p; m->still_t = now ? now : 1; return 0; }
    if (now - m->still_t < SN_STILL || !m->order_t || now < m->order_t || now - m->order_t < SN_STILL) return 0;
    m->still_t = now ? now : 1; z->stale++;
    return 1;
}
internal int sn_order(struct sn_st *z, struct sn_unit *m, int k, ivec2 d, int fight, uint32_t now) {
    if (!cl_may(m->u, CLP_ROUTINE, CLW_STATION)) return 0;   // ORDERS BY PRIORITY (claims.c): healing, under cover, in a strike
    ee_goal *g = ar_goal_at(d, k, fight);
    if (!g || !unit_goal(m->u, g, 0, 0x1F40)) return 0;
    m->order_t = now ? now : 1; m->to = d;
    z->orders++;
    return 1;
}
// COAST BATTERIES ([ai_base] coast_siege, off until measured; [autotest] v_coast_siege; the player 2026-11-20: "front
// sea giving territoeries bordering it an coast modifer? (slightly incresed demand for siege, and siege gets placed on
// the coats if idle, idealy behind a wall if it can shot over one)", and "Frigates are still vulnurble, a group can take
// out a single siege, but 5 haubitze for example will cost any fleet greatly to clear"). A siege weapon of the AI's
// epoch outreaches the battleship of it by a tile or two (Catapult 7 / Bronze 6, Trebuchet 8 / Middle Ages 7, Howitzer
// 9 / Dreadnought 8, Paladin 10 / Leviathan 8) - from a few tiles inland it hits what shells the shore first.
//   THE COASTS - the land territories it holds with SN_SIEGE_ESCORT of the main force's share at least along a sea that
//     is a front of its (territories.c THE SEAS) or an open one: a coast's worth 1, 2 more with enemy warships seen in
//     that sea in the last SN_COAST_SEEN, 1 more for a border sea; the SN_COAST_MAX most worth (their buildings decide
//     between equals) are its batteries.
//   THE POST - on the line from the sea's middle to the territory's, the first tile of its land and SN_COAST_IN tiles
//     further inland (behind a wall along the water's edge: siege shoots over one), else nearer the shore. A battery's
//     coast counts SN_COAST_KEEP more while it still is one (STEADY: they changed every minute or two).
//   THE SIEGE - with no attack planned (the pushes' siege comes first), its siege goes to the batteries, SN_COAST_N
//     each, the most worth first, the rest to that one; at its post a siege weapon stands its ground (stance 1: it
//     shoots what comes within its reach and doesn't walk to the water after it), its old stance back when it leaves.
//     Enemies on the land of its territory take it as before (with soldiers of its there).
//   THE DEMAND - the batteries with enemy warships seen or on a border sea want SN_COAST_N siege each: short of them
//     (the queued counted) every SN_COAST_EVERY up to 5 of the siege type it trains now that threatens ships (fleet.c
//     fl_danger_r) and reaches furthest - an order of its own beside the army's (armies.c SIEGE WITH SOLDIERS caps the
//     need's siege at one for every 3 soldiers; this one at the batteries' want).
//     Nothing that makes it (the sea opponent's games: no Siege Factory, no siege all game): the building laid out by
//     itself by its base, 3 builders kept on it, 5 minutes at most - as the strong anti-air's maker (defence.c).
// "# stations: ... coast" in the minute's line; "# stations: P<n> ... battery" lines.
internal int g_sn_coast = 0, g_v_sn_coast = -1;
#define SN_COAST(idx) PV(idx, g_v_sn_coast, g_sn_coast)
#define SN_COAST_N 5
#define SN_COAST_MAX 2
#define SN_COAST_IN 3
#define SN_COAST_SEEN 300000u
#define SN_COAST_EVERY 20000u
#define SN_COAST_KEEP 1.5f   // STEADY: a battery's coast this much more worth while it still is one
struct sn_battery { short ter, sea; uint8_t hot; ivec2 post; };
internal struct { int n, n0; struct sn_battery b[SN_COAST_MAX], b0[SN_COAST_MAX]; uint32_t train_t, log_t, fnd_t, fnd_b_t; unsigned trained, posted, makers; ee_unit *fnd; int fnd_id; } g_snc[16];   // b0: the last look's; fnd: the Siege Factory it laid
internal int sn_coast_at(unsigned idx, int r, ivec2 *post) {   // territory r is a battery: its post
    for (int k = 0; k < g_snc[idx & 15].n; k++) if (g_snc[idx & 15].b[k].ter == r) { *post = g_snc[idx & 15].b[k].post; return 1; }
    return 0;
}
internal unsigned sn_land_of(int r, ivec2 rally);   // below
internal ivec2 sn_coast_post(int r, int w, unsigned land) {
    ivec2 a = g_ter.t[w].c, b = g_ter.t[r].c;
    float dx = (float)(b.x - a.x), dy = (float)(b.y - a.y), len = sqrtf(dx * dx + dy * dy);
    if (len < 1.0f || !land) return iv2(-1, -1);
    dx /= len; dy /= len;
    for (float k = 0; k <= len; k += 0.5f) {
        ivec2 shore = iv2(a.x + (int)(dx * k), a.y + (int)(dy * k));
        if (area_id(shore, 2) != land) continue;
        for (int in = SN_COAST_IN; in >= 0; in--) {
            ivec2 p = iv2(shore.x + (int)(dx * (float)in + (dx < 0 ? -0.5f : 0.5f)), shore.y + (int)(dy * (float)in + (dy < 0 ? -0.5f : 0.5f)));
            if (area_id(p, 2) == land && (int)ter_near(p.x, p.y) == r) return p;
        }
        return shore;
    }
    return iv2(-1, -1);
}
internal void sn_coast_plan(unsigned idx, const struct tv_st *s, int n, const int *tc, const ivec2 *rally, uint32_t now) {
    typeof(&g_snc[0]) C = &g_snc[idx];
    float sc[SN_COAST_MAX];
    int sea[SN_COAST_MAX];
    C->n0 = C->n; memcpy(C->b0, C->b, sizeof C->b);
    C->n = 0;
    for (int r = 0; r < n; r++) {
        if (g_ter.t[r].water || s->t[r].role == TVR_NONE || tc[r] < SN_SIEGE_ESCORT) continue;
        int bw = -1, hot = 0;
        float bs = 0;
        for (int k = 0; k < g_ter.t[r].nbn; k++) {
            unsigned w = g_ter.nb[g_ter.t[r].nb0 + k].to;
            if (w >= (unsigned)n || !g_ter.t[w].water || (s->t[w].role != TVR_FRONT && !s->t[w].open_sea)) continue;
            int seen = s->t[w].eship_t && now >= s->t[w].eship_t && now - s->t[w].eship_t < SN_COAST_SEEN;
            float v = 1.0f + (seen ? 2.0f : 0.0f) + (s->t[w].border ? 1.0f : 0.0f);
            for (int j = 0; j < C->n0; j++) v += C->b0[j].ter == r && C->b0[j].sea == (short)w ? SN_COAST_KEEP : 0.0f;   // STEADY: a battery keeps its coast
            if (v > bs) { bs = v; bw = (int)w; hot = seen || s->t[w].border; }
        }
        if (bw < 0) continue;
        bs += (float)s->t[r].bld * 0.001f;
        int k = C->n;   // into the SN_COAST_MAX most worth, in order
        if (k == SN_COAST_MAX) { if (sc[k - 1] >= bs) continue; k--; } else C->n++;
        while (k > 0 && sc[k - 1] < bs) { sc[k] = sc[k - 1]; sea[k] = sea[k - 1]; C->b[k] = C->b[k - 1]; k--; }
        sc[k] = bs; sea[k] = bw; C->b[k].ter = (short)r; C->b[k].sea = (short)bw; C->b[k].hot = (uint8_t)hot;
    }
    int m = 0;
    for (int k = 0; k < C->n; k++) {
        ivec2 p = sn_coast_post(C->b[k].ter, sea[k], sn_land_of(C->b[k].ter, rally[C->b[k].ter]));
        if (p.x < 0) continue;
        C->b[m] = C->b[k]; C->b[m++].post = p;
    }
    C->n = m;
    if (g_log && (!C->log_t || now < C->log_t || now - C->log_t >= 60000)) {
        C->log_t = now ? now : 1;
        char b[200] = "";
        for (int k = 0; k < C->n; k++) { size_t l = strlen(b); snprintf(b + l, sizeof b - l, " | territory %d, post %d,%d (sea %d%s)", C->b[k].ter, C->b[k].post.x, C->b[k].post.y, C->b[k].sea, C->b[k].hot ? ", threatened" : ""); }
        log_locked("# stations: P%u at %.1f min: coast batteries %d%s; so far siege ordered for them %u, posted %u, makers laid %u\n", idx, now / 60000.0, C->n, b, C->trained, C->posted, C->makers);
    }
}
internal void sn_coast_train(ee_player *pl, unsigned idx, int have, uint32_t now) {
    typeof(&g_snc[0]) C = &g_snc[idx];
    int want = 0;
    for (int k = 0; k < C->n; k++) want += C->b[k].hot ? SN_COAST_N : 0;
    if (!want || (C->train_t && now >= C->train_t && now - C->train_t < SN_COAST_EVERY)) return;
    have += ar_queued(pl, UTR_SIEGE);
    if (have >= want) return;
    C->train_t = now ? now : 1;
    int best = 0, any = 0;   // any: the one it could train with a building that makes it
    float bs = 0, ba = 0;
    for (int t = 0; t < G_TYPES_N; t++) {
        ee_unit_type *td = type_data(t);
        if (!td || unit_type_get_id(td) != t || unit_type_get_class_id(td) != CLS_LAND || !ut_role(t, UTR_SIEGE) || ut_role(t, UTR_RAM)) continue;
        float r = fl_danger_r(NULL, td);
        if (r <= 0 || !can_build(pl, t)) continue;
        float v = r * 1000.0f + (float)unit_type_get_attack(td);
        if (v > ba) { ba = v; any = t; }
        if (ap_trains_now(pl, t) && v > bs) { bs = v; best = t; }
    }
    void *cm = player_get_civ_mgr(pl), *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    // nothing makes it (the sea opponent's games: no Siege Factory in any, no siege all game): the building laid by
    // itself, builders kept on it, as the strong anti-air's maker (defence.c)
    ee_unit *f = C->fnd && unit_get_serial(C->fnd) == C->fnd_id && unit_get_owner(C->fnd) == pl ? C->fnd : NULL;
    if (f && (unit_get_finished(unit_get_physics(f)) || now - C->fnd_t > 300000)) f = NULL;
    if (f && now - C->fnd_b_t >= 15000) { C->fnd_b_t = now ? now : 1; bk_builders(pl, f, 3); }
    C->fnd = f;
    if (!best && any && !f && bm && um) {
        int prod = aa_producer(pl, any), have_b = 0, busy = 0;
        if (prod) am_bld(um, prod, &have_b, &busy);
        const int *stock = player_stock_at(pl);
        int pay = prod && !have_b && prio_buildable(pl, prod) && !prio_refuses(pl, prod);
        for (int r = 2; pay && r < 7; r++) pay &= unit_cost(pl, prod, r) <= stock[r];
        ivec2 spot[1];
        if (pay && aa_block_at(pl, prod, build_mgr_get_base(bm), 1, spot, 16)) {
            ee_unit *nf = lay_foundation(pl, prod, spot[0].x, spot[0].y);
            if (nf) {
                C->fnd = nf; C->fnd_id = unit_get_serial(nf); C->fnd_t = C->fnd_b_t = now ? now : 1; C->makers++;
                bk_builders(pl, nf, 3);
                if (g_log) log_locked("# stations: P%u at %.1f min: battery siege - a type %d laid out at %d,%d to make type %d\n", idx, now / 60000.0, prod, spot[0].x, spot[0].y, any);
            }
        }
    }
    if (!best || !um) return;
    int req[4] = { best, want - have < 5 ? want - have : 5, 0, 0 };
    unsigned got = 0;
    g_priority_mil = 1;
    hook_train_units(um, req, (uintptr_t)&got);
    g_priority_mil = 0;
    C->trained += got;
    if (g_log && got) log_locked("# stations: P%u at %.1f min: battery siege - %u of type %d ordered (%d of %d at the batteries' coasts and queued)\n", idx, now / 60000.0, got, best, have, want);
}
// AS A GROUP (the player 2026-11-15: "also armies should properly form/muster in territories and move in a group in
// formation attack move"): the soldiers that are to go from one territory to the same place get one order - the game's
// own move on a selection of them, as a click of the player's would be (armies.c ar_group_move's recipe), together
// (the group's speed: the fast wait for the slow, the mod's formations lay them out) and as an attack-move unless the
// siege moves alone. Fewer than SN_GROUP of them wait where they stand for more, SN_GROUP_WAIT at most - a push and
// an attack on their territory wait for nobody.
internal int sn_group_move(ee_player *pl, uint32_t *ids, int n, ivec2 from, ivec2 d, int fight) {
    static __attribute__((aligned(8))) uint8_t mem[0x40];
    if (n <= 0) return 0;
    uint32_t *sel[3] = { NULL, ids, ids + n };
    msg_as_select(pl, sel);
    memset(mem, 0, sizeof mem);
    float x = (float)d.x + 0.5f, y = (float)d.y + 0.5f;
    path_msg_make((ee_path_msg *)mem, msg_as_begin(pl), x, y, 1, 0);
    path_msg_set_forced((ee_path_msg *)mem, !fight);
    path_msg_set_facing((ee_path_msg *)mem, atan2f(-(x - (float)from.x), y - (float)from.y));
    msg_execute((ee_msg *)mem);
    msg_as_end(pl);
    return n;
}
// A STANDING FORMATION (the player 2026-11-17: "the unit holding a territory should stand in formation at the center,
// its not just looks, being spread in the formation is saver and allows the builders to reach their foundation. right
// now they all deathball to the same spot, blocking builders and make an easy target for area demage"; and, of a grid
// of spots tried first: "i ment that the units in the territory should reform with the alredy existing formations
// (the ones alredy used for moving by players) onto a somewhat free space, not spread tile for tile"): a station at
// rest (no push from there, no enemy soldiers in its territory) stands on a patch near its point - the first, from the
// point outwards, of a box its number needs (2 + its root, 3 to 12 tiles each way) on its land with no building, field,
// wall, gate, tree or mine in it; where the point is the territory's empty middle (a claim lays its tower there) the
// box leaves it free. The march there is the group's as before (AS A GROUP); once there, whenever soldiers have joined
// or one stands idle well off the block (back from a fight), all its idle soldiers there get one group move to the
// patch - the game's own move on a selection, so the mod's formations lay them out as a player's click would, the
// ranged behind -, facing where the force meets the enemy next (THE NEED'S PLACE). Not more often than SN_FORM_AGAIN.
// Before: one point for all, each soldier sent alone within 5 x 5 tiles of it - a clump on the claim's foundation.
struct sn_fc { short r; ivec2 spot, at; uint32_t spot_t, form_t; int form_n, box; };
internal struct sn_fc g_snfc[16][SN_FC];
internal struct sn_fc *sn_fc_at(unsigned idx, int r) {   // territory r's entry (a new one takes the oldest place)
    struct sn_fc *c = g_snfc[idx & 15], *old = &c[0];
    for (int k = 0; k < SN_FC; k++) {
        if (c[k].spot_t && c[k].r == r) return &c[k];
        if (!c[k].spot_t || (old->spot_t && c[k].spot_t < old->spot_t)) old = &c[k];
    }
    memset(old, 0, sizeof *old); old->r = (short)r;
    return old;
}
internal int sn_box_free(ivec2 q, int rr, unsigned land) {   // a box of rr each way round q: open ground of land
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0;
    if (q.x - rr < 2 || q.y - rr < 2 || q.x + rr >= size - 2 || q.y + rr >= size - 2) return 0;
    for (int dy = -rr; dy <= rr; dy += rr > 4 ? 2 : 1)
        for (int dx = -rr; dx <= rr; dx += rr > 4 ? 2 : 1) {
            ivec2 t = iv2(q.x + dx, q.y + dy);
            if (area_id(t, 2) != land || area_id(t, 1)) return 0;
        }
    for (struct grid_it it = grid_it_box(q.x, q.y, rr + 1, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        if (abs(unit_get_x(u) - q.x) > rr || abs(unit_get_y(u) - q.y) > rr || unit_get_hp(u) <= 0) continue;
        int fam = unit_type_get_class_id(it.td);
        ee_player *o = unit_get_owner(u);
        if (fam == CLS_BUILDING || fam == CLS_FORTRESS || fam == CLS_GATE || (o && player_get_index(o) == 0 && unit_type_get_attack(it.td) <= 0)) return 0;   // nature's trees and mines, not its animals
    }
    return 1;
}
// territory r's patch round its point at, for n soldiers (open: the point is its empty middle - kept free); none
// found: the point itself
internal void sn_fc_spot(struct sn_fc *c, ivec2 at, int open, int n, uint32_t now) {
    int rr = 2 + (int)sqrtf((float)(n > 0 ? n : 1));
    if (rr > 12) rr = 12;
    if (c->spot_t && now >= c->spot_t && now - c->spot_t < SN_FORM_LOOK && ivec2_eq(c->at, at) && rr <= c->box + 1) return;
    c->spot_t = now ? now : 1; c->at = at; c->box = rr; c->spot = at;
    unsigned land = area_id(at, 2);
    for (int d = 0; land && d <= SN_FORM_FAR; d += 2)
        for (int dx = -d; dx <= d; dx += 2)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d || !d ? 2 : 2 * d)) {
                ivec2 q = iv2(at.x + dx, at.y + dy);
                if (open && abs(q.x - at.x) <= rr + 2 && abs(q.y - at.y) <= rr + 2) continue;   // the claim's foundation left free
                if (!sn_box_free(q, rr, land)) continue;
                c->spot = q;
                return;
            }
}
internal void sn_rejoin(unsigned idx, struct sn_unit *m) {
    uint32_t now = game_time();
    for (int j = 0; j < SN_JOIN; j++)
        if (g_sn_join[idx & 15][j].t && g_sn_join[idx & 15][j].id == m->id && now - g_sn_join[idx & 15][j].t < 120000) {
            m->ter = g_sn_join[idx & 15][j].ter; g_sn_join[idx & 15][j].t = 0;
            if (g_sn_join[idx & 15][j].scout) { m->scout = 1; m->since = now ? now : 1; }   // SCOUTS ACROSS: on with its look
            return;
        }
}
// ACROSS: the beach for a push into land `land` aimed at `aim` - its tile nearest the aim with a sea within 2 tiles and
// no enemy defence it has seen in reach (work.c wk_danger), within SN_BEACH_R; 0: none
#define SN_BEACH_R 40
#define SN_ISLE_GUARD 5          // ISLAND GUARD: the least of the main force a territory of its across the water holds
#define SN_CROSS_T 480000        // ACROSS: a push with none of it over this long is given up
internal int sn_beach(unsigned idx, unsigned land, ivec2 aim, ivec2 *out) {
    for (int r = 0; r <= SN_BEACH_R; r++)
        for (int dx = -r; dx <= r; dx++)
            for (int dy = -r; dy <= r; dy += (abs(dx) == r || !r ? 1 : 2 * r)) {
                ivec2 q = iv2(aim.x + dx, aim.y + dy);
                if (area_id(q, 2) != land || !ocean_near(q, 2, NULL) || wk_danger(idx, q)) continue;
                *out = q;
                return 1;
            }
    return 0;
}
// PLANNED FOR LOSSES (the player 2026-11-18, of the sea round an enemy's land: "a landing will always be dangerus, it also
// should plan it push strength with the asumption that 1-2 transport be lost"): a push over the water is judged without
// the soldiers of its lost transports - one, two from 4 transports on, full loads of its transport type - and against
// the enemy at its beach as well as at its target (the weaker of the two). All of it lost: judged 0, whatever stands
// there (a push across has a second transport's worth at least). No wait for a safe sea (landing.c waited 3 minutes for
// one, then called it off): round an enemy's land there is none - the losses are planned for instead.
internal int sn_loss(ee_player *pl, int n) {
    int tt = ld_transport_type(pl), cap = tt ? ld_cap(tt) : 10;
    if (cap < 1) cap = 10;
    int ships = (n + cap - 1) / cap, lost = (ships > 3 ? 2 : 1) * cap;
    return lost < n ? lost : n;
}
// the soldiers u (n) against the enemy at aim - over the water (across) their losses planned for, and the beach too
internal float sn_judge(ee_player *pl, ee_unit *const *u, int n, int across, ivec2 aim, ivec2 beach) {
    int skip = across ? sn_loss(pl, n) : 0;
    if (across && skip >= n) return 0.0f;
    st_side_clear(&g_sn_own);
    for (int k = skip; k < n; k++) st_side_add(&g_sn_own, u[k]);
    stage_foe(pl, aim);
    float r = st_ratio(&g_sn_own, &g_sgs_foe);
    if (across && ivec2_dist2(aim, beach) > SG_FOE * SG_FOE) {
        stage_foe(pl, beach);
        float rb = st_ratio(&g_sn_own, &g_sgs_foe);
        if (rb < r) r = rb;
    }
    return r;
}
// THE BRIDGEHEAD (the player 2026-11-18: "part of a push should be defensive infrastructure like towers, hospitals and
// temples"): the citizens a push across takes along (its first wave, 2 a transport, SN_CITS at most - ferry.c) build by
// its beach on the land it pushes into, in this order: a tower, a Hospital (the wounded heal there), a second tower, a
// Temple (the enemy's prophets' disasters, the plague's spread kept off) - the newest tower it can build, each when it
// can pay it, at the spot nearest the citizens' middle out of every enemy defence's reach it knows (work.c wk_danger),
// 3 tiles from the others; one laid every SN_BH_EVERY, all of them on the one going up; one it can't build or pay for
// SN_BH_PAY is dropped. A finished tower holds the ground (territories.c TV_HOLDS). All done, or the push over: the
// citizens are work.c's again where they stand (build.c's claim, the mines over there). Before, a push took no citizens:
// what it cleared got its tower once build.c's claim found a builder - one the ferry brought when none stood there.
// A BASE OVER THERE (the player, the same day: "the push citezens should start building up a full base when they have
// nothing better to do, production buildings are extremly valuable on enemy land since they can supply the fight directly
// and cut out the dangerus crossing"): after the first tower a production building, after the Temple a second one of
// another kind, a Town Center (a Settlement when it can't pay one: a drop site for those who work there after), a third
// tower - SN_BH in all. The production building: the first of produce.c's kinds the game offers that the bridgehead
// hasn't got. A soldier new within SN_BH_JOIN of a standing one (the game trains at the building it picks) is the push's
// (sn_bh_recruit): over there already, no crossing.
internal int sn_bh_has(ee_player *pl, ee_unit *u) {   // a citizen of a push's bridgehead (expand_has: work.c leaves it alone)
    struct sn_st *z = &g_sn[(unsigned)player_get_index(pl) & 15];
    for (int k = 0; k < SN_PUSHES; k++)
        for (int j = 0; z->push[k].on && j < z->push[k].ncit; j++) if (z->push[k].cit[j].u == u) return 1;
    return 0;
}
internal int sn_push_cits(unsigned idx, int k) {   // ferry.c: the citizens push k still wants taken along (its first wave)
    if (k < 0 || k >= SN_PUSHES) return 0;
    struct sn_push *p = &g_sn[idx & 15].push[k];
    return p->on && p->across && !p->waves ? p->cits_want - p->ncit : 0;
}
internal void sn_bh_land(unsigned idx, int k, ee_unit *u) {   // ferry.c: a citizen of push k put off by its beach
    if (k < 0 || k >= SN_PUSHES) return;
    struct sn_push *p = &g_sn[idx & 15].push[k];
    if (!p->on || p->ncit >= SN_CITS) return;
    p->cit[p->ncit].u = u; p->cit[p->ncit++].id = unit_get_serial(u);
}
internal int sn_bh_type(ee_player *pl, const struct sn_push *p, int kind) {
    static const int none[7];
    int t = kind == 0 ? tower_newest(pl) : kind == 1 ? UT(B_HOSPITAL) : kind == 2 ? UT(B_TEMPLE) : 0;
    if (kind == 3)   // A BASE OVER THERE: the first production kind it can build and hasn't got here
        for (int k = 0; k < PR_NK && !t; k++) {
            int c = g_ut[PR_KINDS[k]], had = 0;
            for (int j = 0; j < p->nbh; j++) had |= (p->bh[j].state == 1 || p->bh[j].state == 2) && p->bh[j].type == c;
            if (!had && ld_can_build(pl, c)) t = c;
        }
    if (kind == 4) t = ld_can_build(pl, UT(B_TOWN_CENTER)) && bd_pays(pl, UT(B_TOWN_CENTER), none) ? UT(B_TOWN_CENTER) : UT(B_SETTLEMENT);
    return t && ld_can_build(pl, t) ? t : 0;
}
// A BASE OVER THERE: a soldier new to the stations (m: its territory where it stands) by a production building of a
// push's bridgehead joins that push
internal void sn_bh_recruit(unsigned idx, struct sn_unit *m, uint32_t now) {
    ivec2 q = unit_get_tile(m->u);
    for (int k = 0; k < SN_PUSHES; k++) {
        struct sn_push *p = &g_sn[idx & 15].push[k];
        if (!p->on || !p->across || area_id(q, 2) != p->to_land) continue;
        for (int j = 0; j < p->nbh; j++) {
            if (p->bh[j].state != 2 || p->bh[j].kind != 3 || ivec2_dist2(q, p->bh[j].p) > SN_BH_JOIN * SN_BH_JOIN) continue;
            m->ter = p->from; p->recruits++;
            if (g_log) log_locked("# stations: P%u at %.1f min: the push into %d - a soldier trained at its bridgehead joins it (%d so far)\n", idx, now / 60000.0, p->into, p->recruits);
            return;
        }
    }
}
internal int sn_bh_spot(ee_player *pl, unsigned idx, struct sn_push *p, int type, ivec2 c, ivec2 *out) {
    ee_unit_type *td = type_data(type);
    int sz = td ? unit_type_get_size(td) : 1;
    if (sz < 1) sz = 1;
    for (int d = 2; d <= 12; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d ? 1 : 2 * d)) {
                ivec2 q = iv2(c.x + dx, c.y + dy);
                int ok = area_id(q, 2) == p->to_land && !area_id(q, 1);
                for (int j = 0; j < p->nbh && ok; j++)   // room between them: a way through
                    if ((p->bh[j].state == 1 || p->bh[j].state == 2) && abs(p->bh[j].p.x - q.x) < sz + 3 && abs(p->bh[j].p.y - q.y) < sz + 3) ok = 0;
                if (!ok || !can_place_open(pl, type, q) || wk_danger(idx, q)) continue;
                *out = q;
                return 1;
            }
    return 0;
}
internal void sn_bridgehead(ee_player *pl, unsigned idx, struct sn_push *p, uint32_t now) {
    static const char *const nm[5] = { "tower", "Hospital", "Temple", "production building", "town building" };
    long sx = 0, sy = 0;
    int n = 0;
    for (int k = 0; k < p->ncit; k++) {
        if (!ex_alive(&p->cit[k], pl)) { p->cit[k--] = p->cit[--p->ncit]; continue; }
        if (unit_get_x(p->cit[k].u) >= 0 && area_id(unit_get_tile(p->cit[k].u), 2) == p->to_land) { sx += unit_get_x(p->cit[k].u); sy += unit_get_y(p->cit[k].u); n++; }
    }
    if (!n) return;
    if (!p->nbh) {
        static const int plan[SN_BH] = { 0, 3, 1, 0, 2, 3, 4, 0 };
        for (int j = 0; j < SN_BH; j++) { memset(&p->bh[j], 0, sizeof p->bh[j]); p->bh[j].kind = plan[j]; }
        p->nbh = SN_BH;
    }
    ivec2 c = iv2((int)(sx / n), (int)(sy / n));
    int going = -1, left = 0;
    for (int j = 0; j < p->nbh; j++) {   // going up: finished or destroyed
        struct sn_bh *h = &p->bh[j];
        if (h->state != 1) continue;
        if (!h->b || unit_get_serial(h->b) != h->bid || unit_get_hp(h->b) <= 0) { h->state = 3; h->b = NULL; }
        else if (unit_get_physics(h->b) && unit_get_finished(unit_get_physics(h->b))) h->state = 2;
        else { going = j; continue; }
        if (g_log) log_locked("# stations: P%u at %.1f min: the push into %d - the bridgehead's %s at %d,%d %s\n", idx, now / 60000.0, p->into, nm[h->kind], h->p.x, h->p.y, h->state == 2 ? "stands" : "is destroyed");
    }
    for (int j = 0; j < p->nbh && going < 0 && (!p->bh_t || now - p->bh_t >= SN_BH_EVERY); j++) {   // the next, in order
        struct sn_bh *h = &p->bh[j];
        if (h->state) continue;
        if (!h->t) h->t = now ? now : 1;
        static const int none[7];
        int t = sn_bh_type(pl, p, h->kind);
        if (!t || !bd_pays(pl, t, none)) {
            if (now - h->t < SN_BH_PAY) break;
            h->state = 3;
            if (g_log) log_locked("# stations: P%u at %.1f min: the push into %d - no %s for the bridgehead it can build and pay in %d s, dropped\n", idx, now / 60000.0, p->into, nm[h->kind], SN_BH_PAY / 1000);
            continue;
        }
        ivec2 at;
        ee_unit *b = sn_bh_spot(pl, idx, p, t, c, &at) ? lay_foundation(pl, t, at.x, at.y) : NULL;
        p->bh_t = now ? now : 1;
        if (g_log) log_locked("# stations: P%u at %.1f min: the push into %d - the bridgehead's %s (type %d) %s, %d citizens ashore\n", idx, now / 60000.0, p->into, nm[h->kind], t,
                              b ? "laid" : "has no room", n);
        if (!b) { h->state = 3; continue; }
        h->b = b; h->bid = unit_get_serial(b); h->type = t; h->p = at; h->state = 1; going = j;
    }
    for (int j = 0; j < p->nbh; j++) left += p->bh[j].state < 2;
    if (!left) {
        if (g_log) log_locked("# stations: P%u at %.1f min: the push into %d - the bridgehead done, %d citizens back to work\n", idx, now / 60000.0, p->into, p->ncit);
        p->ncit = 0;
        return;
    }
    if (going < 0) return;
    ee_unit *b = p->bh[going].b;
    for (int k = 0; k < p->ncit; k++) {   // all of them on the one going up
        ee_unit *u = p->cit[k].u;
        ee_goal *g = unit_get_goal(u);
        if (unit_get_x(u) < 0 || (g && !strcmp(rtti_name(g), "EEUGBuildRepair@@") && goal_get_building(g) == b)) continue;
        bd_send(pl, u, b);
    }
}
// PRODUCTION WHERE THE FIGHT IS ([ai_base] fight_production; army.c FILL, produce.c; the player 2026-11-19, of the
// soldiers a push's base over the water was to train: "its alredy decided by territory needs right? prioretising
// production buildings in fough in territories (invading or devending) would be a logical extension"). Where production
// stands followed the territories' needs (produce.c) - in quiet territories only - but not where a soldier was trained:
// the game hands an order to the producers of one region (ee_unit_mgr::train), the one the request's fourth value
// names, else the one with the most of them - home, whatever the order was for, and the bridgehead's Barracks trained
// none. So the order names (pass 0) a push's base over the water with a standing building that trains the type, else
// (and pass 1) the region of a territory of its under attack, holding (not TV_UNSAFE) and shortest of its share, with
// one in it; 0: none, the game's pick (and when nothing trains it there now). *kind: 0 the push's base, 1 defended
// MEASURED 2026-11-19 (head-to-head, Small Islands large, 4 AIs from epoch 5, 60 min, seeds 1-12 mirrored, 24 games, both
// sides with push_across): soldiers -0.5 (se 3.7), kills +6.8 (11.0), losses -0.7 (8.5), razed +0.8 (0.5), workers +1.2
// (0.7); held across the water about the same (citizens off the home island 21.4 against 19.7, buildings 8.1 against
// 7.2). Trained so: 644 soldiers for defended territories' regions, 23 at a push's base (few stand while their push is
// on: 12 production buildings laid by the B seats' bridgeheads in 82 pushes). No clear gain - ON all the same by the
// player's call (2026-11-19: "the gain sems small but there, and could make a bigger diffrence in a situation like we
// have here" - his game's island both AIs settled, where P2's soldiers took down P3's tower going up).
// A SHORT QUEUE (the player 2026-11-20: "why does p2 barely fill its pop limit even tough it has the resources for
// units?" - his P2 at minute 225, population 1043 of 1520 with 472 queued: one Tank Factory at the front 153 deep, a
// Siege Factory 29, five Barracks 37 each, 11 Barracks and every other building idle - queued units count against the
// cap, and every order went to the fight's few buildings): only a building whose queue holds fewer than SN_FP_QUEUE
// (the one in the making counted) is named; all of them busy, the game's pick.
#define SN_FP_QUEUE 3
internal int sn_fp_short(ee_unit *b) {
    int *qb = unit_get_queue_begin(b), *qe = unit_get_queue_end(b), cur = unit_get_producing(b);
    int nq = qb && qe > qb ? (int)(qe - qb) : 0;
    return nq + (cur > 0 && cur != 100000) < SN_FP_QUEUE;
}
internal unsigned sn_train_region(ee_player *pl, unsigned idx, int type, int pass, int *kind) {
    int pt = ap_producer(pl, type);
    if (!pt || pass > 1) return 0;
    struct sn_st *z = &g_sn[idx & 15];
    for (int k = 0; !pass && k < SN_PUSHES; k++) {
        struct sn_push *p = &z->push[k];
        if (!p->on || !p->across || !p->over) continue;
        for (int j = 0; j < p->nbh; j++) {
            struct sn_bh *h = &p->bh[j];
            if (h->state != 2 || h->type != pt || !h->b || unit_get_serial(h->b) != h->bid || unit_get_hp(h->b) <= 0 || !sn_fp_short(h->b)) continue;   // A SHORT QUEUE
            unsigned a = unit_region(h->b);
            if (a) { *kind = 0; return a; }
        }
    }
    struct tv_st *s = &g_tv[idx & 15];
    int n = s->n < PR_T ? s->n : PR_T, best = 0, count = 0;
    unsigned area = 0;
    if (!n || s->version != g_ter.version) return 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_type_get_id(td) != pt || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !cz_finished(u) || !sn_fp_short(u)) continue;   // A SHORT QUEUE
        unsigned r = ter_near(unit_get_x(u), unit_get_y(u));
        if (r >= (unsigned)n || s->t[r].state == TV_UNSAFE || !g_pr_foe[idx & 15][r]) continue;
        int miss = (int)g_pr_share[idx & 15][r] - (int)g_pr_has[idx & 15][r];
        unsigned a = miss > best ? unit_region(u) : 0;
        if (a) { best = miss; area = a; }
    }
    if (area) *kind = 1;
    return area;
}
// THE LAST WORD (ferry.c, as landing.c's before it sails): push k's first wave aboard, soldiers u (n) - judged at once
// against the beach and the target, its losses planned for; under SN_LAST for SN_LAST_WEAK it is called off (unloaded at
// home): 1 go, 0 wait, -1 called off
internal int sn_last_word(ee_player *pl, unsigned idx, int k, ee_unit *const *u, int n, uint32_t now) {
    if (k < 0 || k >= SN_PUSHES) return 1;
    struct sn_push *p = &g_sn[idx & 15].push[k];
    if (!p->on || p->over || p->waves > 1) return 1;   // a later wave goes to those over
    if (!p->judge_t || now - p->judge_t >= 2000) { p->judge_r = sn_judge(pl, u, n, 1, p->at, p->beach); p->judge_t = now ? now : 1; }
    if (p->judge_r >= SN_LAST) { p->weak_t = 0; return 1; }
    if (!p->weak_t) {
        p->weak_t = now ? now : 1;
        if (g_log) log_locked("# stations: P%u at %.1f min: the push into %d - %d soldiers aboard judged %.2f at the beach and the target, their losses planned for (enemy there: %d)\n",
                              idx, now / 60000.0, p->into, n, (double)p->judge_r, g_sgs_foe.n);
    }
    if (now - p->weak_t < SN_LAST_WEAK) return 0;
    return -1;
}
internal void sn_push_off(unsigned idx, int k, const char *why, uint32_t now) {   // ferry.c: push k called off
    struct sn_st *z = &g_sn[idx & 15];
    struct sn_push *p = &z->push[k];
    p->on = 0; p->end_t = now ? now : 1; p->wait = SN_CALLED_OFF; p->ncit = 0;
    for (int i = 0; i < z->n; i++) if (z->m[i].ter == p->from) z->m[i].order_t = 0;   // back to their station
    if (p->from >= 0 && p->from < PR_T) z->calm_t[p->from] = p->end_t + p->wait;
    if (g_log) log_locked("# stations: P%u at %.1f min: the push from territory %d into %d is over - %s (judged %.2f)\n", idx, now / 60000.0, p->from, p->into, why, (double)p->judge_r);
}
// ACROSS: the soldiers of push k still on the land it left (siege too), nearest `toward` first, into out (max): how many
internal int expand_has(ee_player *pl, ee_unit *u);
internal int sn_push_pick(ee_player *pl, int k, ivec2 toward, ee_unit **out, int max) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct sn_st *z = &g_sn[idx];
    static int d[SN_UNITS];
    int got = 0;
    if (k < 0 || k >= SN_PUSHES) return 0;
    struct sn_push *p = &z->push[k];
    if (!p->on || !p->across) return 0;
    for (int i = 0; i < z->n; i++) {
        struct sn_unit *m = &z->m[i];
        d[i] = -1;
        if (m->ter != p->from || m->scout || m->pool == SNP_AA || unit_get_hp(m->u) <= 0 || unit_get_x(m->u) < 0) continue;
        if (area_id(unit_get_tile(m->u), 2) != p->from_land || expand_has(pl, m->u)) continue;
        d[i] = ivec2_dist2(unit_get_tile(m->u), toward);
    }
    while (got < max) {
        int b = -1;
        for (int i = 0; i < z->n; i++) if (d[i] >= 0 && (b < 0 || d[i] < d[b])) b = i;
        if (b < 0) break;
        out[got++] = z->m[b].u; d[b] = -1;
    }
    return got;
}
// THE FERRY SERVICE (expand.c): the main force's share and those stationed, by land - sn_give walks the land only, so a
// territory across the water keeps short of its share, however many stand spare at home. at: the place of the land's
// territory shortest of its share; gap: what its territories are short of as GIVING counts it, less what they have
// beyond their shares (> 0 short, < 0 spare). Each look; sn_ferry_pick takes the spare ones of a land for the transport.
#define SN_LANDS 16
struct sn_land { unsigned land; int want, have, gap, hot; ivec2 at; };   // hot: a territory of its under attack (the crossing: an important delivery)
internal short g_sn_ptc[16][SNP_N][PR_T];   // each pool's shares by territory, the last look (sn_ferry_pick)
internal struct sn_lands { uint32_t t; int n; struct sn_land l[SN_LANDS]; } g_snl[16][SNP_N];   // by pool (main force, siege, anti-air)
// EVERY POOL BY THE FERRY (the player 2026-11-20, of the anti-air - "anti air never gets redistributed via ferrys right?,
// there a 100 or so mobile aa in one place" - then "why do we need to add this as a special case for evrething? there is
// no reason why anething should be disalowed from using ferries when it would distribute over land", and "siege also
// seems stuck on one island"): the lands table and the ferry's pick were the main force's alone; now every pool's
// shares are counted by land the same way (sn_lands) and the ferry serves each (ferry.c, trip kind 1 with its pool).
internal void sn_lands(unsigned idx, int pool, int n, const int *tc, const struct tv_st *s, const ivec2 *rally, uint32_t now) {
    struct sn_lands *L = &g_snl[idx & 15][pool];
    struct sn_st *z = &g_sn[idx & 15];
    static int ac[PR_T];
    int worst[SN_LANDS];
    memset(ac, 0, (size_t)n * sizeof *ac);
    for (int i = 0; i < z->n; i++) if (z->m[i].pool == pool && !z->m[i].scout && z->m[i].ter >= 0 && z->m[i].ter < n) ac[z->m[i].ter]++;
    L->n = 0; L->t = now ? now : 1;
    for (int r = 0; r < n; r++) {
        g_sn_ptc[idx & 15][pool][r] = (short)tc[r];
        if (g_ter.t[r].water || (!tc[r] && !ac[r])) continue;
        unsigned a = sn_land_of(r, rally[r]);
        if (!a) continue;
        int k = 0;
        while (k < L->n && L->l[k].land != a) k++;
        if (k == L->n) { if (k == SN_LANDS) continue; L->l[k].land = a; L->l[k].want = L->l[k].have = 0; L->l[k].gap = L->l[k].hot = 0; L->l[k].at = rally[r]; worst[k] = -0x7FFFFFFF; L->n++; }
        L->l[k].want += tc[r]; L->l[k].have += ac[r];
        // short as GIVING has it over land (sn_give: by SN_BAND of its share, SN_BAND_MIN at least, or by any under
        // attack) - a land short by a soldier or two at every change of the shares sent 2.5 trips an AI back the
        // way one had come within 10 minutes (Small Islands large, 10 seeds); spare: any beyond the share
        int df = tc[r] - ac[r], hard = s->t[r].foe > 0 && s->t[r].role != TVR_NONE;
        L->l[k].hot |= hard;
        if (df < 0) L->l[k].gap += df;
        else if (hard || df >= (tc[r] * SN_BAND > SN_BAND_MIN ? tc[r] * SN_BAND : SN_BAND_MIN)) L->l[k].gap += df;
        if (df > worst[k]) { worst[k] = df; L->l[k].at = rally[r]; }
    }
}
internal unsigned sn_land_of(int r, ivec2 rally) {
    unsigned a = area_id(rally, 2);
    return a ? a : area_id(g_ter.t[r].c, 2);
}
// the units of pool `pool` on land `land` beyond their territories' shares (not a push's, not where the enemy is),
// nearest to `toward` first, into out (at most max): how many
internal int expand_has(ee_player *pl, ee_unit *u);   // expand.c: a passenger of one of its ferries
internal int sn_ferry_pick(ee_player *pl, int pool, unsigned land, ivec2 toward, ee_unit **out, int max) {   // EVERY POOL BY THE FERRY: pool's
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct sn_st *z = &g_sn[idx];
    struct tv_st *s = &g_tv[idx];
    static short cnt[PR_T];
    static int dist[SN_UNITS];
    int n = s->n < PR_T ? s->n : PR_T, got = 0;
    memset(cnt, 0, (size_t)n * sizeof *cnt);
    for (int i = 0; i < z->n; i++) if (z->m[i].pool == pool && !z->m[i].scout && z->m[i].ter >= 0 && z->m[i].ter < n) cnt[z->m[i].ter]++;
    for (int i = 0; i < z->n; i++) {
        struct sn_unit *m = &z->m[i];
        dist[i] = -1;
        if (m->pool != pool || m->scout || m->ter < 0 || m->ter >= n || cnt[m->ter] <= g_sn_ptc[idx][pool][m->ter] || s->t[m->ter].foe > 0) continue;
        if (unit_get_hp(m->u) <= 0 || unit_get_x(m->u) < 0 || area_id(unit_get_tile(m->u), 2) != land || expand_has(pl, m->u)) continue;
        int pushing = 0;
        for (int k = 0; k < SN_PUSHES; k++) pushing |= z->push[k].on && z->push[k].from == m->ter;
        if (!pushing) dist[i] = ivec2_dist2(unit_get_tile(m->u), toward);
    }
    while (got < max) {
        int b = -1;
        for (int i = 0; i < z->n; i++) if (dist[i] >= 0 && cnt[z->m[i].ter] > g_sn_ptc[idx][pool][z->m[i].ter] && (b < 0 || dist[i] < dist[b])) b = i;
        if (b < 0) break;
        out[got++] = z->m[b].u; cnt[z->m[b].ter]--; dist[b] = -1;
    }
    return got;
}
// SIEGE AT THE GATES: unit id attacks target, by the player's attack message on a selection of it (engine/editor_orders.c's way)
internal void sn_attack_msg(ee_player *pl, uint32_t id, ee_unit *target) {
    static __attribute__((aligned(8))) uint8_t mem[0x40];
    uint32_t ids[1] = { id };
    uint32_t *sel[3] = { NULL, ids, ids + 1 };
    msg_as_select(pl, sel);
    memset(mem, 0, sizeof mem);
    attack_msg_make((ee_attack_msg *)mem, msg_as_begin(pl), (unsigned)unit_get_serial(target), 0);
    msg_execute((ee_msg *)mem);
    msg_as_end(pl);
}
// DEMOLITION, A BREACH: u attacks an enemy wall piece or gate - a gate by the player's attack message as SIEGE AT THE
// GATES does (an attack goal made on a gate crashed the game), a piece by the goal: 1 ordered
internal int sn_attack_piece(ee_player *pl, ee_unit *u, ee_unit *target) {
    ee_unit_type *td = unit_get_type_data(target);
    if (td && unit_type_get_class_id(td) == CLS_GATE) { sn_attack_msg(pl, (uint32_t)unit_get_serial(u), target); return 1; }
    ee_goal *g = goal_attack(u, target);
    return g && unit_goal(u, g, 0, 0x1F40);
}
// SIEGE AT THE GATES: the nearest enemy gate pl knows within r of p
internal ee_unit *sn_gate_near(ee_player *pl, ivec2 p, int r) {
    ee_unit *best = NULL;
    int bd = 0x7FFFFFFF;
    for (struct grid_it it = grid_it_box(p.x, p.y, r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (unit_type_get_class_id(it.td) != CLS_GATE || unit_get_hp(u) <= 0 || !o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !seen_by(u, pl)) continue;
        int d = ivec2_dist2(unit_get_tile(u), p);
        if (d <= r * r && d < bd) { bd = d; best = u; }
    }
    return best;
}
internal void sn_count(const struct sn_st *z, int pool, int n, int *ac) {   // the stationed of a pool counted by territory into ac
    memset(ac, 0, (size_t)n * sizeof *ac);
    for (int i = 0; i < z->n; i++) if (z->m[i].pool == pool && !z->m[i].scout && z->m[i].ter >= 0 && z->m[i].ter < n) ac[z->m[i].ter]++;
}
// the stationed of a pool counted by territory into ac; then the territories short of tc are given those of the
// nearest with more: 1 where anything moved
internal int sn_give(struct sn_st *z, int pool, int n, const int *tc, int *ac, const struct tv_st *s, int *budget) {
    static uint8_t vis[PR_T], done[PR_T], dep[PR_T];
    static short q[PR_T];
    int any = 0;
    uint32_t tnow = game_time();
    sn_count(z, pool, n, ac);
    memset(done, 0, (size_t)n);
    while (*budget > 0) {
        int r = -1, best = 0;
        for (int k = 0; k < n; k++) {
            int df = tc[k] - ac[k], hard = s->t[k].foe > 0 && s->t[k].role != TVR_NONE;
            if (done[k] || df <= 0 || (!hard && df < (tc[k] * SN_BAND > SN_BAND_MIN ? tc[k] * SN_BAND : SN_BAND_MIN))) continue;
            int pr = df + (hard ? 100000 : 0);
            if (pr > best) { best = pr; r = k; }
        }
        if (r < 0) break;
        done[r] = 1;
        memset(vis, 0, (size_t)n);
        int qh = 0, qt = 0;
        q[qt++] = (short)r; vis[r] = 1; dep[r] = 0;
        int hard_r = s->t[r].foe > 0 && s->t[r].role != TVR_NONE, far_ = hard_r ? SN_GIVE_STEPS_ATTACK : SN_GIVE_STEPS;
        while (qh < qt && tc[r] > ac[r] && *budget > 0) {
            int o = q[qh++];
            // BY STEPS (the player 2026-11-15: "units still get send over half the map, they should do a simple pathfind how
            // many territores they are away, with incresing cost, neasrt first, very far might not be worth the move at all
            // and its better to produce locally"): the nearest territory with more than its share first, as before; one
            // step further keeps one more of its surplus for each step beyond the first, and beyond SN_GIVE_STEPS
            // territories (SN_GIVE_STEPS_ATTACK for one under attack) nobody is sent - what stays missing there is
            // produce.c's claim: it builds its own production.
            int give = o != r ? ac[o] - tc[o] - (hard_r ? 0 : dep[o] - 1) : 0;
            for (int i = 0; i < z->n && give > 0 && tc[r] > ac[r] && *budget > 0; i++) {
                struct sn_unit *m = &z->m[i];
                if (m->pool != pool || m->scout || m->ter != o) continue;
                if (!hard_r && m->moved_t && tnow >= m->moved_t && tnow - m->moved_t < SN_STAY) continue;   // it has just been sent somewhere: it stays (the shares shift with every look - 400 to 680 changes of station a player in 75 minutes)
                int pushing = 0;
                for (int p = 0; p < SN_PUSHES; p++) pushing |= z->push[p].on && z->push[p].from == o;
                pushing |= z->call_t && z->call_r == o && tnow >= z->call_t && tnow - z->call_t < SN_CALL;   // THE CALL: gathering for one
                if (pushing) break;   // a territory that is pushing gives none
                m->ter = (short)r; m->order_t = 0; m->moved_t = tnow ? tnow : 1;
                ac[o]--; ac[r]++; give--; (*budget)--; z->moved++; any = 1;
            }
            for (int k = 0; k < g_ter.t[o].nbn; k++) {
                unsigned w = g_ter.nb[g_ter.t[o].nb0 + k].to;
                if (w < (unsigned)n && !vis[w] && !g_ter.t[w].water && qt < PR_T && dep[o] < far_) { vis[w] = 1; dep[w] = (uint8_t)(dep[o] + 1); q[qt++] = (short)w; }
            }
        }
    }
    return any;
}
// build.c's claim: a push of its holds the territory it cleared
internal int sn_holding(unsigned idx, int r, ivec2 *at) {   // *at: where its soldiers stand
    for (int k = 0; k < SN_PUSHES; k++) if (g_sn[idx & 15].push[k].on && g_sn[idx & 15].push[k].hold_t && g_sn[idx & 15].push[k].into == r) { *at = g_sn[idx & 15].push[k].at; return 1; }
    return 0;
}
// THE NEXT GOAL AT ONCE (explore.c; the player 2026-11-20: "exploring is very slow, with much idle time, scouts should
// get a new order the moment they reached their goal"): between the looks, every XP_STEP, a scout walking through its
// territory whose goal is done (uncovered, looked at) or that stands idle gets the next one there; none left: the
// next look moves it on
internal void sn_scouts_step(ee_player *pl, struct sn_st *z, unsigned idx, uint32_t now) {
    if (!g_set_ai[1].explore || !g_honest_map || (z->step_t && now >= z->step_t && now - z->step_t < XP_STEP)) return;
    z->step_t = now ? now : 1;
    for (int i = 0; i < z->n; i++) {
        struct sn_unit *m = &z->m[i];
        if (m->scout != 1 || m->look.x < 0 || m->ter < 0 || m->ter >= PR_T || (unsigned)m->ter >= (unsigned)g_ter.n) continue;
        if (!m->u || (int)unit_get_serial(m->u) != m->id || unit_get_hp(m->u) <= 0 || unit_get_x(m->u) < 0) continue;
        if (!xp_done(idx, pl, m->look) && !ar_idle(m->u)) continue;   // on its way
        ivec2 p = unit_get_tile(m->u), lk;
        if (!xp_goal(idx, pl, p, g_ter.t[m->ter].c, 0, m->ter, NULL, &lk) && !xp_stale(idx, pl, p, m->ter, &lk)) continue;
        if (lk.x == m->look.x && lk.y == m->look.y) continue;
        m->look = lk; m->since = now ? now : 1;
        if (sn_order(z, m, 0, lk, 0, now)) z->looks++;
    }
}
// the quick step at a simulation step's end (eeaimod.c ai_step_end), for a player whose look ran in this game lately
internal void stations_step(ee_player *pl, uint32_t now) {
    if (!pl || is_vanilla(pl)) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct sn_st *z = &g_sn[idx];
    if (!z->n || !z->t || now < z->t || now - z->t > 3 * SN_EVERY) return;
    sn_scouts_step(pl, z, idx, now);
}
internal void stations_tick(ee_player *pl, uint32_t now) {
    static struct sn_unit old[SN_UNITS];
    static int tc[PR_T], ac[PR_T], tcs[PR_T], acs[PR_T], fx[PR_T], fy[PR_T], fn[PR_T];
    static ivec2 rally[PR_T];
    static uint8_t anc[PR_T];
    static struct { ivec2 p; short ter; uint8_t temple; } eb[SN_EB];   // temple: TEMPLE FIRST
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct sn_st *z = &g_sn[idx];
    struct tv_st *s = &g_tv[idx];
    if (now < z->t) { memset(z, 0, sizeof *z); memset(g_sn_seen[idx], 0, sizeof g_sn_seen[idx]); memset(g_sn_look[idx], 0, sizeof g_sn_look[idx]); memset(g_sn_dark_t[idx], 0, sizeof g_sn_dark_t[idx]); }
    if (z->t && now - z->t < SN_EVERY) { sn_scouts_step(pl, z, idx, now); return; }
    z->t = now ? now : 1;
    int n = s->n < PR_T ? s->n : PR_T;
    if (!n || s->version != g_ter.version) return;
    static struct scen_target stg[SN_TARGETS];
    int nst = scen_targets(pl, stg, SN_TARGETS), nat = 0;
    for (int i = 0; i < nst; i++) nat += !stg[i].defend;
    // its units: the soldiers with their stations, a place to stand in each territory, the territories it has stood in
    int on = z->n, nn = 0, count = 0, neb = 0;
    memcpy(old, z->m, (size_t)on * sizeof *old);
    memset(anc, 0, (size_t)n);
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0) continue;
        if (unit_get_x(u) < 0) { sn_stance_back(old, on, u); continue; }
        int t = unit_type_get_id(td), fam = unit_type_get_class_id(td);
        ivec2 p = unit_get_tile(u);
        unsigned r = ter_near(p.x, p.y);
        if (r >= (unsigned)n) continue;
        g_sn_seen[idx][r] = 1;
        g_sn_look[idx][r] = now ? now : 1;   // A LOOK AGAIN: a unit of its stands there
        if (fam == CLS_BUILDING || fam == CLS_FORTRESS) {
            ee_unit *st = unit_get_physics(u);
            int town = is_wall(t) ? 0 : t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL) ? 3 : t == UT(B_SETTLEMENT) ? 2 : 1;
            if (st && unit_get_finished(st) && town > anc[r]) { anc[r] = (uint8_t)town; rally[r] = p; }
            continue;
        }
        // the class Land only: a wall piece is of the army's families too (the first look: 252 "soldiers" with 131 there, the walls stationed with them)
        if (expand_has(pl, u)) { sn_stance_back(old, on, u); continue; }   // a ferry's passenger (expand.c): the ferry's until it is put off
        int pool = fam != CLS_LAND ? -1 : is_aa(u) ? SNP_AA : is_siege(t) || is_ram(t) ? SNP_SIEGE : land_soldier(t) ? SNP_MAIN : -1;
        if (pool < 0 || nn >= SN_UNITS) continue;
        struct sn_unit key = { .u = u }, *o = on ? bsearch(&key, old, (size_t)on, sizeof *old, sn_cmp) : NULL;
        struct sn_unit *m = &z->m[nn++];
        if (o && o->id == unit_get_serial(u)) *m = *o;
        else { memset(m, 0, sizeof *m); m->u = u; m->id = unit_get_serial(u); m->ter = (short)r; sn_rejoin(idx, m); if (m->ter == (short)r) sn_bh_recruit(idx, m, now); }
        m->pool = (uint8_t)pool;
        if (!m->scout && (m->ter < 0 || m->ter >= n || g_ter.t[m->ter].water)) m->ter = (short)r;
    }
    z->n = nn;
    qsort(z->m, (size_t)nn, sizeof *z->m, sn_cmp);
    for (int r = 0; r < n; r++) if (!anc[r]) rally[r] = g_ter.t[r].c;
    // the enemy it knows: the soldiers it sees by territory (their middle), the buildings to push at
    memset(fn, 0, (size_t)n * sizeof *fn); memset(fx, 0, (size_t)n * sizeof *fx); memset(fy, 0, (size_t)n * sizeof *fy);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(u) || !seen_by(u, pl)) continue;
        ivec2 p = unit_get_tile(u);
        unsigned r = ter_near(p.x, p.y);
        if (r >= (unsigned)n) continue;
        if (fr_building(it.td)) { if (fr_target(it.td) && neb < SN_EB) { eb[neb].p = p; eb[neb].temple = unit_type_get_id(it.td) == UT(B_TEMPLE); eb[neb++].ter = (short)r; } }   // fronts.c fr_target: no wall piece, no gate
        else if (ms_land_soldier(u, it.td)) { fx[r] += p.x; fy[r] += p.y; fn[r]++; }
    }
    // STATIONS: the shares in soldiers
    int np[SNP_N] = { 0, 0, 0 }, scouts = 0, wsum = 0;
    for (int i = 0; i < nn; i++) { if (z->m[i].scout) scouts += z->m[i].scout == 1; else np[z->m[i].pool]++; }   // escorts (scout 2) are neither
    // TEMPLE FIRST (the player 2026-11-16, agreeing to it: an enemy Temple's protection keeps its prophets' powers off
    // everything in its reach): with prophets of its own a push goes at the Temple in the territory first
    int pro = holy_prophets(idx) > 0;
    // STEADY (the player 2026-11-15, on the soldiers reshuffled with every look: "yes, continue with steading the
    // shares"): a territory's share in soldiers stays what it was until the new one is off by SN_BAND of it (SN_BAND_MIN
    // soldiers at least) - a soldier trained or lost, a citizen more in a front's guard moved every share by one and a
    // soldier with it, there and back (420 to 860 changes of station a player in 75 minutes). Not one under attack, not
    // one that loses its share altogether: those at once.
    for (int r = 0; r < n; r++) {
        int v = (g_pr_share[idx][r] * np[SNP_MAIN] + 500) / 1000, was = z->tc[r], band = was * SN_BAND > SN_BAND_MIN ? was * SN_BAND : SN_BAND_MIN;
        if (v && was && abs(v - was) < band && !(s->t[r].foe > 0 && s->t[r].role != TVR_NONE)) v = was;
        tc[r] = v; z->tc[r] = (short)(v > 32767 ? 32767 : v);
    }
    if (EXPAND_ACROSS(idx)) {   // ISLAND GUARD (work.c ACROSS THE WATER): a territory off its home land with a building of its there
        // (a foundation too) holds SN_ISLE_GUARD of the main force at least, taken from the home territory with the most -
        // without, its first settlements across the water stood alone: 33 citizens of it killed a game against the
        // outposts' 10 (their guard and towers), 17 of them by enemy soldiers, 13 by ships (head-to-head, 12 seeds)
        unsigned home = (unsigned)ex_home(pl);
        for (int r = 0; r < n && home; r++) {
            // THE ESCORT (the player 2026-11-18: "both on land and now across water a small escort for assumed unclaimed land
            // expansion is reasonable"): one it settles over land without holding it yet too
            if (g_ter.t[r].water || s->t[r].bld < 1 || tc[r] >= SN_ISLE_GUARD || (area_id(g_ter.t[r].c, 2) == home && s->t[r].role != TVR_NONE) || s->t[r].state == TV_UNSAFE) continue;
            int h = -1;
            for (int o = 0; o < n; o++) if (!g_ter.t[o].water && area_id(g_ter.t[o].c, 2) == home && (h < 0 || tc[o] > tc[h])) h = o;
            int give = h >= 0 ? SN_ISLE_GUARD - tc[r] : 0;
            if (h >= 0 && tc[h] - give < SN_MIN) give = tc[h] - SN_MIN;
            if (give <= 0) continue;
            tc[h] -= give; tc[r] += give; z->tc[h] = (short)tc[h]; z->tc[r] = (short)tc[r];
        }
    }
    if (nst > nat) {   // A SCENARIO'S TARGETS: a defend target's share
        static uint8_t dfl[PR_T];
        memset(dfl, 0, (size_t)n);
        for (int i = 0; i < nst; i++) { unsigned dr = ter_near(stg[i].at.x, stg[i].at.y); if (stg[i].defend && dr < (unsigned)n) dfl[dr] = 1; }
        for (int i = 0; i < nst; i++) {
            unsigned dr = ter_near(stg[i].at.x, stg[i].at.y);
            if (!stg[i].defend || dr >= (unsigned)n || g_ter.t[dr].water) continue;
            int want = np[SNP_MAIN] * SN_DEFEND / 100 / (nst - nat), guard = 0;
            while (tc[dr] < want && guard++ < 4096) {
                int b = -1;
                for (int r = 0; r < n; r++) if (!dfl[r] && tc[r] > 0 && (b < 0 || tc[r] > tc[b])) b = r;
                if (b < 0) break;
                tc[b]--; tc[dr]++;
            }
        }
    }
    for (int r = 0; r < n; r++) {
        if (tc[r] <= 0 || tc[r] >= SN_MIN || (s->t[r].foe > 0 && s->t[r].role != TVR_NONE)) continue;
        int b = -1;
        for (int k = 0; k < g_ter.t[r].nbn; k++) {
            unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
            if (o < (unsigned)n && !g_ter.t[o].water && tc[o] >= SN_MIN && (b < 0 || tc[o] > tc[b])) b = (int)o;
        }
        if (b >= 0) { tc[b] += tc[r]; tc[r] = 0; }
    }
    for (int r = 0; r < n; r++) {   // siege: the territories planning an attack that have a share of the main force
        int a = 0;
        tcs[r] = 0;
        if (g_ter.t[r].water || s->t[r].role == TVR_NONE || tc[r] <= 0) continue;
        for (int k = 0; k < g_ter.t[r].nbn; k++) {
            unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
            if (o < (unsigned)n && !g_ter.t[o].water && s->t[o].ebld && PR_ATTACK + s->t[o].ebld > a) a = PR_ATTACK + s->t[o].ebld;
        }
        // ... and across the water where pushes go across (ACROSS; the player 2026-11-20: "siege also seems stuck on one
        // island" - its share went beside enemy buildings over land only, so a push across had none stationed to take)
        if (PUSH_ACROSS(idx)) {
            wk_across_build();
            for (int q = 0; r < g_wkx.n && q < g_wkx.no[r]; q++) {
                unsigned o = g_wkx.o[r][q];
                if (o < (unsigned)n && s->t[o].ebld && PR_ATTACK + s->t[o].ebld > a) a = PR_ATTACK + s->t[o].ebld;
            }
        }
        tcs[r] = a; wsum += a;
    }
    int budget = SN_MOVES;
    sn_give(z, SNP_MAIN, n, tc, ac, s, &budget);
    sn_lands(idx, SNP_MAIN, n, tc, s, rally, now);   // THE FERRY SERVICE: by land
    if (SN_COAST(idx)) sn_coast_plan(idx, s, n, tc, rally, now); else g_snc[idx].n = 0;   // COAST BATTERIES
    if (wsum > 0) {
        for (int r = 0; r < n; r++) tcs[r] = (tcs[r] * np[SNP_SIEGE] + wsum / 2) / wsum;
        sn_give(z, SNP_SIEGE, n, tcs, acs, s, &budget);
    } else if (g_snc[idx].n) {   // COAST BATTERIES: no attack planned - the siege to the batteries, SN_COAST_N each, the rest to the first
        int left = np[SNP_SIEGE];
        for (int r = 0; r < n; r++) tcs[r] = 0;
        for (int k = 0; k < g_snc[idx].n; k++) { int take = left < SN_COAST_N ? left : SN_COAST_N; tcs[g_snc[idx].b[k].ter] += take; left -= take; }
        tcs[g_snc[idx].b[0].ter] += left;
        sn_give(z, SNP_SIEGE, n, tcs, acs, s, &budget);
    }
    if (wsum <= 0 && !g_snc[idx].n) {   // no attack planned, no battery: no siege wanted anywhere
        memset(tcs, 0, (size_t)n * sizeof *tcs);
        sn_count(z, SNP_SIEGE, n, acs);   // its siege still counted: acs is shared by all AIs and held another's (the formations' box, a breach)
    }
    sn_lands(idx, SNP_SIEGE, n, tcs, s, rally, now);   // THE FERRY SERVICE: its siege by land too
    if (g_snc[idx].n) {   // COAST BATTERIES: THE DEMAND (its siege that is no ram)
        int have = 0;
        for (int i = 0; i < z->n; i++) have += z->m[i].pool == SNP_SIEGE && !sn_is_ram(z->m[i].u);
        sn_coast_train(pl, idx, have, now);
    }
    // ANTI-AIR (the player 2026-11-14: "3. anti air, pure devensive role, might also get a small share in the
    // backlands"): its anti-air units among the territories it holds by what stands there to protect - its buildings
    // there (SN_AA_BLD at most; a backland's count half) and SN_AA_SEEN more where enemy aircraft were last seen near its
    // towns (defence.c g_aa_st, the last 5 minutes). They go to their station and stay: no push takes them along.
    {
        static int tca[PR_T], aca[PR_T];
        int asum = 0, at = -1;
        if (g_aa_st[idx].seen_t && now >= g_aa_st[idx].seen_t && now - g_aa_st[idx].seen_t < 300000) at = (int)ter_near(g_aa_st[idx].ax, g_aa_st[idx].ay);
        for (int r = 0; r < n; r++) {
            int w = 0;
            if (!g_ter.t[r].water && s->t[r].role != TVR_NONE && s->t[r].state != TV_UNSAFE) {
                w = s->t[r].bld < SN_AA_BLD ? s->t[r].bld : SN_AA_BLD;
                if (s->t[r].role == TVR_BACK) w = (w + 1) / 2;
                if (r == at) w += SN_AA_SEEN;
            }
            tca[r] = w; asum += w;
        }
        if (asum > 0 && np[SNP_AA]) {
            for (int r = 0; r < n; r++) tca[r] = (tca[r] * np[SNP_AA] + asum / 2) / asum;
            sn_give(z, SNP_AA, n, tca, aca, s, &budget);
        } else memset(tca, 0, (size_t)n * sizeof *tca);
        sn_lands(idx, SNP_AA, n, tca, s, rally, now);   // THE FERRY SERVICE: its anti-air by land too
    }
    // A PUSH: those on, then a new one
    for (int k = 0; k < SN_PUSHES; k++) {
        struct sn_push *p = &z->push[k];
        if (!p->on) continue;
        if (p->across && p->ncit) sn_bridgehead(pl, idx, p, now);   // THE BRIDGEHEAD
        long sx = 0, sy = 0, msx = 0, msy = 0;
        int cn = 0, bi = -1, bd = 0;
        st_side_clear(&g_sn_own);
        p->mn = 0;
        // those the ferry has taken (walking to its shore, aboard) are left out of the look (expand_has) but still of
        // the push: counted as still to cross - without, a push whose every soldier was with the ferry was "nobody
        // left", 130 of 190 pushes across, and those put off came back to a push that was over
        p->left = p->across ? fy_push_transit(idx, k) : 0; p->over = 0;
        for (int i = 0; i < nn; i++) {
            if (z->m[i].ter != p->from || z->m[i].scout || z->m[i].pool == SNP_AA) continue;
            ivec2 q = unit_get_tile(z->m[i].u);
            if (p->across) { unsigned ql = area_id(q, 2); if (ql == p->to_land) p->over++; else { p->left++; continue; } }   // ACROSS: the middle and the strength of those over
            sx += q.x; sy += q.y; cn++;
            if (z->m[i].pool == SNP_MAIN) { msx += q.x; msy += q.y; p->mn++; }
            st_side_add(&g_sn_own, z->m[i].u);
        }
        p->mmid = p->mn ? iv2((int)(msx / p->mn), (int)(msy / p->mn)) : p->at;   // THE SIEGE'S ESCORT: where its soldiers are
        ivec2 mid = cn ? iv2((int)(sx / cn), (int)(sy / cn)) : p->at;
        ivec2 lead = mid;   // THE WAY's start: the soldier nearest their middle (the middle itself can lie in a wood or on water)
        for (int i = 0, ld = 0x7FFFFFFF; i < nn; i++) {
            if (z->m[i].ter != p->from || z->m[i].scout) continue;
            ivec2 q = unit_get_tile(z->m[i].u);
            if (ivec2_dist2(q, mid) < ld) { ld = ivec2_dist2(q, mid); lead = q; }
        }
        for (int e = 0; e < neb; e++) {
            if (eb[e].ter != p->into) continue;
            int d = ivec2_dist2(eb[e].p, mid) - (eb[e].temple && pro ? 1 << 24 : 0);   // TEMPLE FIRST
            if (bi < 0 || d < bd) { bi = e; bd = d; }
        }
        int ti = sn_target_in(stg, nst, p->into);   // A SCENARIO'S TARGET there: what it goes at first
        if (ti >= 0 && !stg[ti].u && ivec2_dist2(mid, stg[ti].at) <= SN_TARGET_AT * SN_TARGET_AT) ti = -1;   // an area reached
        const char *end = NULL;
        float ratio = 0;
        if (p->across && p->left && (!p->over || p->left * 4 > p->left + p->over)) {   // ACROSS: the ferry still at it - the beach held meanwhile
            if (now - p->start_t < SN_CROSS_T) continue;
            if (!p->over) end = "not carried over in time";
        }
        if (end) { }
        else if (!cn && !p->left) end = "nobody left";
        else if (p->mn < SN_SIEGE_ESCORT) end = "too few soldiers left for its siege";   // siege alone is no push (the player 2026-11-16: "sometimes siege attacks completly alone")
        else if (ti >= 0) {
            stage_foe(pl, stg[ti].at);
            ratio = st_ratio(&g_sn_own, &g_sgs_foe);
            if (ratio < SN_BACK) { end = "losing (at the scenario's target)"; z->lost++; }
            else { p->at = stg[ti].at; p->hold_t = 0; }
        }
        else if (bi < 0) {
            // THE CLEARED GROUND (the player 2026-11-15: "the claiming proccess needs some work, right now the army comes,
            // clears evrething and than leaves. leaving the citezens alone to build their tower"): with no building left
            // there the push is won, not over - its soldiers stand where its last building stood until a
            // building of its holds it (territories.c TV_HOLDS: build.c's claim lays the tower there first, at once),
            // SN_HOLD at most, or until they are losing against what comes.
            if (!p->hold_t) {
                p->hold_t = now ? now : 1; z->won++;
                p->breach = 0;   // where the last building stood: they are there (the territory's middle may lie in a wood, behind a wall - 2 of the first 3 holds ended "stuck" on the way to it)
                for (int i = 0; i < nn; i++) if (z->m[i].ter == p->from) z->m[i].order_t = 0;
                if (g_log) log_locked("# stations: P%u at %.1f min: the push from territory %d into %d has cleared it - %d soldiers hold it until a building of its stands there\n", idx, now / 60000.0, p->from, p->into, cn);
            }
            stage_foe(pl, p->at);
            ratio = st_ratio(&g_sn_own, &g_sgs_foe);
            if (TV_HOLDS(idx, &s->t[p->into])) end = "claimed: a building of its holds the territory";
            else if (now < p->hold_t || now - p->hold_t >= SN_HOLD) end = "cleared, and not claimed in time";
            else if (ratio < SN_BACK) end = "losing the ground it cleared";
        }
        else {
            stage_foe(pl, eb[bi].p);
            ratio = st_ratio(&g_sn_own, &g_sgs_foe);
            if (ratio < SN_BACK) { end = "losing"; z->lost++; }
            else { p->at = eb[bi].p; p->hold_t = 0; }   // TO LOOK: one seen while it held the ground - the push goes on
        }
        // STUCK (the player 2026-11-15, his game's P2 from minute 183 to 222: 137 soldiers packed against a wall, "red is
        // stuck on an attack that never ends" - the cells of the route search are coarser than a wall is thick, the way
        // was "open" by a walk round that did not exist): a push whose soldiers have come no nearer to its building
        // for SN_STUCK - by a tenth of the way, and not standing at it - goes at the enemy wall or gate it has seen
        // nearest them (SN_STUCK_WALL tiles), THE WAY's breach without asking the route; with none there the push is
        // over, and SN_STUCK_WAIT passes before that territory pushes again.
        if (!end && !p->hold_t) {
            int d = ivec2_dist2(mid, p->at);
            if (!p->near_t || now < p->near_t || p->near_at.x != p->at.x || p->near_at.y != p->at.y || d * 10 < p->near_d * 8) { p->near_t = now ? now : 1; p->near_d = d; p->near_at = p->at; }
            else if (now - p->near_t >= SN_STUCK && d > SN_STUCK_AT * SN_STUCK_AT) {
                ee_unit *piece = sn_wall_near(pl, lead, SN_STUCK_WALL);
                p->near_t = now ? now : 1;
                if (piece) {
                    p->breach = 1; p->wall = unit_get_tile(piece); p->force_t = now ? now : 1; p->way_t = now ? now : 1;
                    z->breaches++;
                    for (int i = 0; i < nn; i++) if (z->m[i].ter == p->from) z->m[i].order_t = 0;
                    if (g_log) log_locked("# stations: P%u at %.1f min: the push from territory %d into %d is stuck (%d tiles from its building for %d s) - a breach at the wall at %d,%d\n",
                                          idx, now / 60000.0, p->from, p->into, (int)sqrtf((float)d), SN_STUCK / 1000, p->wall.x, p->wall.y);
                } else { end = "stuck, no way there"; z->lost++; p->wait = SN_STUCK_WAIT; }
            }
        }
        if (!end && p->force_t && now >= p->force_t && now - p->force_t < SN_STUCK && sn_wall_at(pl, p->wall)) {   // the forced breach holds while a piece stands there
            ee_unit *piece = sn_wall_at(pl, p->wall);
            ivec2 w = unit_get_tile(piece);
            if (w.x != p->wall.x || w.y != p->wall.y) { p->wall = w; for (int i = 0; i < nn; i++) if (z->m[i].ter == p->from) z->m[i].order_t = 0; }
            p->way_t = now ? now : 1;
        }
        if (!end && (!p->way_t || now < p->way_t || now - p->way_t >= SN_WAY_EVERY)) {   // THE WAY
            int was = p->breach;
            ivec2 wall = p->wall;
            p->way_t = now ? now : 1;
            p->breach = sn_way(pl, idx, lead, p->at, &wall, &p->round_, &p->straight, now);
            ee_unit *piece = p->breach ? sn_wall_at(pl, wall) : NULL;
            if (!piece) p->breach = 0; else wall = unit_get_tile(piece);
            if (p->breach != was || (p->breach && (wall.x != p->wall.x || wall.y != p->wall.y))) {
                for (int i = 0; i < nn; i++) if (z->m[i].ter == p->from) z->m[i].order_t = 0;
                if (p->breach) z->breaches++;
                if (g_log) log_locked("# stations: P%u at %.1f min: the push from territory %d into %d - %s (the way through %d cells, the walk round %d)\n", idx, now / 60000.0, p->from, p->into,
                                      p->breach ? "a wall in the way: a breach" : "the way is open", p->straight, p->round_);
            }
            p->wall = wall;
        }
        if (!end && !player_ai_toggle(pl, SM_AIT_ATTACK)) end = "the scenario's AI toggle Attack is off";
        if (!end) continue;
        p->on = 0; p->end_t = now ? now : 1; p->ncit = 0;   // THE BRIDGEHEAD's citizens work.c's again
        if (p->from >= 0 && p->from < PR_T) z->calm_t[p->from] = p->end_t + (p->wait ? p->wait : SN_PUSH_WAIT);
        for (int i = 0; i < nn; i++) if (z->m[i].ter == p->from) z->m[i].order_t = 0;   // back to their station
        if (g_log) log_locked("# stations: P%u at %.1f min: the push from territory %d into %d is over - %s (%d soldiers, judged %.2f)\n", idx, now / 60000.0, p->from, p->into, end, cn, (double)ratio);
    }
    for (int r = 0; r < n; r++) {
        struct tv_terr *t = &s->t[r];
        if (g_ter.t[r].water || t->role == TVR_NONE || t->state != TV_QUIET || ac[r] < SN_PUSH_MIN || !player_ai_toggle(pl, SM_AIT_ATTACK)) continue;
        int slot = -1, busy = 0, into = -1, there = 0, cn = 0, bi = -1, bd = 0;
        for (int k = 0; k < SN_PUSHES; k++) {
            struct sn_push *p = &z->push[k];
            if (p->on && p->from == r) busy = 1;
            if (!p->on && (slot < 0 || p->end_t < z->push[slot].end_t)) slot = k;   // the one that ended longest ago
        }
        if (now < z->calm_t[r]) busy = 1;   // its last push's wait
        if (busy || slot < 0) continue;
        for (int k = 0; k < g_ter.t[r].nbn; k++) {
            unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
            if (o < (unsigned)n && !g_ter.t[o].water && s->t[o].ebld && (into < 0 || s->t[o].ebld > s->t[into].ebld)) into = (int)o;
        }
        int across = 0;
        if (into < 0 && PUSH_ACROSS(idx)) {   // ACROSS: no enemy beside it over land - one across the water
            wk_across_build();
            for (int q = 0; r < g_wkx.n && q < g_wkx.no[r]; q++) {
                unsigned o = g_wkx.o[r][q];
                // LOOKED AT FIRST (the player 2026-11-18: "the territory system has scouts, they should be able to use
                // ferrys too for these jobs"): only one a unit of its stood in lately - what it knows there is then
                // what holds it, and the judging below sizes the push by it (blind, every push went at an enemy home
                // island never looked at: soldiers -21.6 a game)
                if (!g_sn_seen[idx][o] || now < g_sn_look[idx][o] || now - g_sn_look[idx][o] >= SN_RESCOUT) continue;
                if (o < (unsigned)n && s->t[o].ebld && (into < 0 || s->t[o].ebld > s->t[into].ebld)) { into = (int)o; across = 1; }
            }
        }
        int ti = -1;   // A SCENARIO'S TARGETS: a neighbour with an attack target first, else the enemy's nearest one
        for (int k = 0; k < g_ter.t[r].nbn && nat && ti < 0; k++) {
            unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
            if (o < (unsigned)n && !g_ter.t[o].water && (ti = sn_target_in(stg, nst, (int)o)) >= 0) into = (int)o;
        }
        if (nat && ti < 0 && into >= 0) {
            long best = -1;
            for (int k = 0; k < g_ter.t[r].nbn; k++) {
                unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
                if (o >= (unsigned)n || g_ter.t[o].water || !s->t[o].ebld) continue;
                for (int i = 0; i < nst; i++) {
                    long d = stg[i].defend ? -1 : ivec2_dist2(g_ter.t[o].c, stg[i].at);
                    if (d >= 0 && (best < 0 || d < best)) { best = d; into = (int)o; }
                }
            }
        }
        if (into < 0) continue;
        for (int e = 0; e < neb; e++) {
            if (eb[e].ter != into) continue;
            int d = ivec2_dist2(eb[e].p, rally[r]) - (eb[e].temple && pro ? 1 << 24 : 0);   // TEMPLE FIRST
            if (bi < 0 || d < bd) { bi = e; bd = d; }
        }
        // TO LOOK (the player's game of 2026-11-16, "aistuck": yellow's push into red's territory 7 ended "cleared" at
        // minute 82 with a production building of red's still there - built out of its sight on ground it had explored,
        // which the view counts (territories.c ebld) and a push does not know; no push started again in 20 minutes):
        // an enemy territory with no building it sees or remembers is pushed into all the same, at its middle - there
        // its soldiers see what stands, and the push goes on at that, or holds the ground as cleared.
        ivec2 aim = ti >= 0 ? stg[ti].at : bi >= 0 ? eb[bi].p : g_ter.t[into].c, beach = aim;
        if (across && !sn_beach(idx, area_id(g_ter.t[into].c, 2), aim, &beach)) continue;   // no beach out of its towers' reach
        static ee_unit *cand[SN_UNITS];   // its soldiers, then THE CALL's: judged together (PLANNED FOR LOSSES)
        for (int i = 0; i < nn; i++) {
            if (z->m[i].ter != r || z->m[i].scout || z->m[i].pool == SNP_AA) continue;
            ivec2 q = unit_get_tile(z->m[i].u);
            cand[cn++] = z->m[i].u;
            there += ter_near(q.x, q.y) == (unsigned)r || ivec2_dist2(q, rally[r]) <= 15 * 15;
        }
        if (there * 4 < cn * 3) continue;   // still gathering
        float ratio = sn_judge(pl, cand, cn, across, aim, beach);
        // THE CALL (the player 2026-11-16: "yellow just sits there, it has enough soilders to finish red or atleast take
        // the food source it desperratly needs, instead it seems completly dead" - 53 soldiers over 4 territories, 9 to 16
        // each, its enemy beside one of them with 2 soldiers and its buildings; a push weighs only the territory's own,
        // and none started for 25 minutes): too few there, the main force of its quiet territories - no enemy soldiers
        // there, none pushing, on the same land - is weighed with them, each keeping half its share. Enough together:
        // they are stationed in this territory (it gives none for SN_CALL meanwhile) and the push starts when three
        // quarters of them are there and it is still judged to win. One call at a time.
        if (ratio < SN_PUSH) {
            if (z->call_t && now >= z->call_t && now - z->call_t < SN_CALL) continue;
            static short left[PR_T];
            static int joiner[SN_UNITS];
            unsigned land = area_id(rally[r], 2);
            for (int o = 0; o < n; o++) {
                int po = 0;
                for (int k = 0; k < SN_PUSHES; k++) po |= z->push[k].on && z->push[k].from == o;
                left[o] = (short)(o == r || po || g_ter.t[o].water || s->t[o].foe > 0 || s->t[o].state != TV_QUIET ? 0 : ac[o] - (z->tc[o] + 1) / 2);
            }
            int nj = 0;
            for (int i = 0; i < nn; i++) {
                struct sn_unit *m = &z->m[i];
                if (m->pool != SNP_MAIN || m->scout || m->ter < 0 || m->ter >= n || left[m->ter] <= 0) continue;
                if (land && area_id(unit_get_tile(m->u), 2) != land) continue;
                left[m->ter]--; joiner[nj] = i; cand[cn + nj++] = m->u;
            }
            float all = nj ? sn_judge(pl, cand, cn + nj, across, aim, beach) : ratio;
            if (all < SN_PUSH) continue;
            for (int j = 0; j < nj; j++) { struct sn_unit *m = &z->m[joiner[j]]; ac[m->ter]--; m->ter = (short)r; m->order_t = 0; m->moved_t = now ? now : 1; ac[r]++; z->moved++; }
            z->call_r = r; z->call_t = now ? now : 1; z->calls++;
            if (g_log) log_locked("# stations: P%u at %.1f min: territory %d calls %d soldiers from its quiet territories to push into %d (the building at %d,%d) - %d of its own judged %.2f, together %.2f (enemy there: %d)\n",
                                  idx, now / 60000.0, r, nj, into, aim.x, aim.y, cn, (double)ratio, (double)all, g_sgs_foe.n);
            continue;
        }
        struct sn_push *p = &z->push[slot];
        p->on = 1; p->from = (short)r; p->into = (short)into; p->at = aim; p->end_t = 0; p->way_t = 0; p->breach = 0; p->near_t = p->force_t = p->wait = p->hold_t = 0;
        p->across = across; p->from_land = area_id(rally[r], 2); p->to_land = area_id(g_ter.t[into].c, 2); p->beach = beach; p->left = cn; p->over = 0; p->start_t = now ? now : 1; p->waves = 0; p->wave_t = 0;
        p->ncit = p->cits_want = p->nbh = p->sup_oc = p->recruits = 0; p->bh_t = p->weak_t = p->judge_t = 0; p->judge_r = ratio;
        z->pushes++;
        if (g_log) log_locked("# stations: P%u at %.1f min: territory %d pushes into %d%s - %d soldiers and siege at the %s at %d,%d, judged %.2f (enemy there: %d)%s\n",
                              idx, now / 60000.0, r, into, across ? " across the water" : "", cn, ti >= 0 ? "scenario's target" : "building", p->at.x, p->at.y, (double)ratio, g_sgs_foe.n,
                              across ? " - the ferry carries them to the beach" : "");
    }
    // SCOUTS
    int unseen = 0, due = 0;
    for (int r = 0; r < n; r++) { unseen += !g_ter.t[r].water && !g_sn_seen[idx][r]; due += sn_due(idx, r, s, now) != 0; }   // A LOOK AGAIN
    z->due = due;
    int want_sc = due && np[SNP_MAIN] + scouts >= 3 && now >= z->scout_off && player_ai_toggle(pl, SM_AIT_EXPLORE) ? 1 + due / 8 : 0;
    if (want_sc > SN_SCOUTS) want_sc = SN_SCOUTS;
    while (scouts < want_sc) {
        int b = -1; float bf = -1.0f;
        for (int i = 0; i < nn; i++) {
            struct sn_unit *m = &z->m[i];
            // the fastest, among those the one that sees furthest (the player 2026-11-15: "samson is probbably the worst
            // chioce for a scout possible? they are slow and have low view range" - the pick went by armies.c's raid
            // families, 0 for everything on foot, so the first soldier of the list was taken)
            ee_unit_type *mtd = unit_get_type_data(m->u);
            int pushing = 0;
            ee_unit_stats *mst = mtd ? unit_get_stats(m->u) : NULL;
            float f = mst ? unit_stats_get_speed(mst) * 100.0f + unit_stats_get_sight(mst) : 0.0f;
            for (int p = 0; p < SN_PUSHES; p++) pushing |= z->push[p].on && z->push[p].from == m->ter;
            if (m->pool == SNP_MAIN && !m->scout && !pushing && f > bf) { bf = f; b = i; }
        }
        if (b < 0) break;
        z->m[b].scout = 1; z->m[b].ter = -1; z->m[b].order_t = 0; z->m[b].since = 0; z->m[b].look = iv2(-1, -1);
        scouts++;
    }
    // QUARANTINE (the player 2026-11-16: "i take it the a ai never heard of quarantine yet? xD to plaque spreads
    // extremly good, because the ai mixes infected soilders with uninfected ones witout care, it loses whole armies to
    // that. infected forced need to go to a hospital survive undr a temples influece to prevent spreading"): a soldier
    // with the plague (unit_plagued) leaves whatever it does - its station, a push, a scout's or an escort's walk -
    // marked scout 4 (no share, no push takes it), and walks (no attack-move, nothing on the way) to its cure place,
    // and stands there; the plague gone, it is the main force's again where it stands. THE CURE PLACE (the player:
    // "the hospital keeps them alive until the plaque times out, if they not in a hospitals influence tehy die. the
    // temple dosnt heal, but it prevents new infections so they are no danger to othe runits anymore. thats why the
    // combination is important"): the game's data (dbareaeffect.dat) gives a Hospital SN_HOSP_R tiles from its edge
    // ("Hospital Rate 2": 2 to 9 with its upgrades) and a Temple SN_TEMPLE_R ("Temple Range 8" to 12) - beside a finished
    // Hospital with a finished Temple of its within the Temple's reach, on the Temple's side, first; else beside a
    // Hospital; else by a Temple; none: it stands where it is, away from the others' moves. (A test: clubmen 3 tiles
    // from a Hospital without upgrades died as fast as ones 21 tiles off.)
    {
        int nq = 0;
        for (int i = 0; i < nn; i++) nq += z->m[i].scout == 4 || unit_plagued(z->m[i].u);
        static ivec2 hosp[64], tmpl[64], cure[64];
        static int cure_rank[64];
        int nh = 0, nt = 0, ncure = 0, cnt = 0;
        for (ee_unit *u = nq ? player_get_units(pl) : NULL; u && cnt < 16384; u = unit_get_next(u), cnt++) {
            ee_unit_type *td = unit_get_type_data(u);
            int t = td ? unit_type_get_id(td) : -1;
            if ((t != UT(B_HOSPITAL) && t != UT(B_TEMPLE)) || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !home_built(u)) continue;
            if (t == UT(B_HOSPITAL) && nh < 64) hosp[nh++] = unit_get_tile(u);
            else if (t == UT(B_TEMPLE) && nt < 64) tmpl[nt++] = unit_get_tile(u);
        }
        for (int k = 0; k < nh; k++) {   // beside each Hospital: toward its nearest Temple when that one reaches there (rank 2), else on its own (1)
            int bt = -1, btd = 0;
            for (int j = 0; j < nt; j++) { int d = ivec2_dist2(tmpl[j], hosp[k]); if (bt < 0 || d < btd) { bt = j; btd = d; } }
            ivec2 at = iv2(hosp[k].x, hosp[k].y + SN_HOSP_SIDE);
            int rank = 1;
            if (bt >= 0) {
                float dx = (float)(tmpl[bt].x - hosp[k].x), dy = (float)(tmpl[bt].y - hosp[k].y), dl = sqrtf(dx * dx + dy * dy);
                if (dl >= 1.0f) at = iv2(hosp[k].x + (int)lroundf(dx / dl * SN_HOSP_SIDE), hosp[k].y + (int)lroundf(dy / dl * SN_HOSP_SIDE));
                rank = ivec2_dist2(at, tmpl[bt]) <= SN_TEMPLE_R * SN_TEMPLE_R ? 2 : 1;
            }
            cure[ncure] = at; cure_rank[ncure++] = rank;
        }
        for (int j = 0; j < nt && ncure < 64; j++) { cure[ncure] = tmpl[j]; cure_rank[ncure++] = 0; }
        for (int i = 0; i < nn && nq; i++) {
            struct sn_unit *m = &z->m[i];
            ivec2 q = unit_get_tile(m->u);
            int sick = unit_plagued(m->u);
            if (m->scout == 4 && !sick) {   // cured
                unsigned r = ter_near(q.x, q.y); m->scout = 0; m->ter = (short)(r < (unsigned)n ? (int)r : 0); m->order_t = 0; z->cured++;
                if (g_log) log_locked("# stations: P%u at %.1f min: a soldier at %d,%d cured - the force's again (%u quarantined, %u cured so far)\n", idx, now / 60000.0, q.x, q.y, z->quarantined, z->cured);
                continue;
            }
            if (!sick) continue;
            if (m->scout != 4) {
                if (m->scout == 2 && m->stance0 >= 0) unit_set_stance(m->u, m->stance0);
                m->scout = 4; m->ter = -1; m->order_t = 0; z->quarantined++;
                if (g_log) log_locked("# stations: P%u at %.1f min: a soldier at %d,%d has the plague - quarantined (%d Hospitals and Temples to go to; %u so far)\n", idx, now / 60000.0, q.x, q.y, ncure, z->quarantined);
            }
            ivec2 to = q;
            int bd = -1;
            int br = -1;
            for (int k = 0; k < ncure; k++) { int d = ivec2_dist2(cure[k], q); if (cure_rank[k] > br || (cure_rank[k] == br && d < bd)) { bd = d; br = cure_rank[k]; to = cure[k]; } }
            if (!m->order_t || now - m->order_t >= 5000) {
                if (bd >= 0 && bd > 1) sn_order(z, m, 0, to, 0, now);
                else if (bd < 0) { ee_goal *g = unit_get_goal(m->u); if (!m->order_t && g) sn_order(z, m, 0, q, 0, now); else m->order_t = now ? now : 1; }   // nowhere to go: it stops where it is
            }
        }
    }
    // AN ESCORT: given, followed, let go
    {
        int esc_on = z->esc_t && now >= z->esc_t && now - z->esc_t < SN_ESC_KEEP, ne = 0;
        for (int i = 0; i < nn; i++) {
            struct sn_unit *m = &z->m[i];
            if (m->scout != 2) continue;
            ivec2 q = unit_get_tile(m->u);
            if (!esc_on || ne >= z->esc_n) {
                unsigned r = ter_near(q.x, q.y); m->scout = 0; m->ter = (short)(r < (unsigned)n ? (int)r : 0); m->order_t = 0;
                if (m->stance0 >= 0) unit_set_stance(m->u, m->stance0);
                continue;
            }
            ne++;
            if ((ivec2_dist2(q, z->esc_at) > 3 * 3 || z->esc_fight) && (!m->order_t || now - m->order_t >= 3000)) sn_order(z, m, 0, z->esc_at, z->esc_fight, now);
        }
        unsigned esc_land = area_id(z->esc_at, 2);   // its land's soldiers only (another land's walked to the shore and stood there)
        while (esc_on && ne < z->esc_n && esc_land) {
            int b = -1, bd = 0;
            for (int i = 0; i < nn; i++) {
                struct sn_unit *m = &z->m[i];
                if (m->pool != SNP_MAIN || m->scout || m->ter < 0 || m->ter >= n || s->t[m->ter].foe > 0) continue;
                int pushing = 0;
                for (int k = 0; k < SN_PUSHES; k++) pushing |= z->push[k].on && z->push[k].from == m->ter;
                int d = ivec2_dist2(unit_get_tile(m->u), z->esc_at);
                if (area_id(unit_get_tile(m->u), 2) != esc_land) continue;
                if (!pushing && (b < 0 || d < bd)) { b = i; bd = d; }
            }
            if (b < 0) break;
            z->m[b].scout = 2; z->m[b].ter = -1; z->m[b].order_t = 0; z->escorts++; ne++;
            z->m[b].stance0 = (int8_t)unit_stance(z->m[b].u);
            unit_set_stance(z->m[b].u, 1);
            sn_order(z, &z->m[b], 0, z->esc_at, z->esc_fight, now);
            if (g_log && ne == z->esc_n) log_locked("# stations: P%u at %.1f min: an escort of %d for the prophet at %d,%d (so far %u soldiers given)\n", idx, now / 60000.0, ne, z->esc_at.x, z->esc_at.y, z->escorts);
        }
    }
    for (int i = 0; i < nn; i++) {
        struct sn_unit *m = &z->m[i];
        if (m->scout != 1) continue;
        ivec2 p = unit_get_tile(m->u);
        if (m->ter >= 0 && m->ter < n && sn_due(idx, m->ter, s, now) && m->since && now - m->since > SN_SCOUT_GIVE_UP) {   // not reached in that time: given up (walled off, held against it) - till SN_RESCOUT
            if (!g_sn_seen[idx][m->ter]) g_sn_seen[idx][m->ter] = 2;
            g_sn_look[idx][m->ter] = now ? now : 1;
        }
        // THROUGH THE TERRITORY (explore.c; the player 2026-11-20: "the scouts just walk toward the center of each
        // territory, missing evrething around it, they also should be more systematic"): while its territory has
        // unexplored ground at its edge on the scout's land, or a part looked at long ago, the scout stays and walks
        // there, goal after goal (a goal not reached in SN_SCOUT_GIVE_UP gives the territory up, as before). With
        // honest_map only: with the map handed over the scouts go as they did (their looks again unmeasured otherwise)
        ivec2 lk = iv2(-1, -1);
        int inside = m->ter >= 0 && m->ter < n && g_set_ai[1].explore && g_honest_map && area_id(g_ter.t[m->ter].c, 2) == area_id(p, 2) &&
                     (!m->since || now - m->since <= SN_SCOUT_GIVE_UP) &&
                     (xp_goal(idx, pl, p, g_ter.t[m->ter].c, 0, m->ter, NULL, &lk) || xp_stale(idx, pl, p, m->ter, &lk));
        if (inside) {
            if (m->look.x < 0 || !m->order_t || xp_done(idx, pl, m->look) || sn_idle(z, m, now)) {   // THE NEXT GOAL AT ONCE
                if (lk.x != m->look.x || lk.y != m->look.y) m->since = now ? now : 1;
                m->look = lk;
                if (sn_order(z, m, 0, lk, 0, now)) z->looks++;
            }
            continue;
        }
        m->look = iv2(-1, -1);
        if (m->ter >= 0 && m->ter < n && ter_near(p.x, p.y) == (unsigned)m->ter) g_sn_dark_t[idx][m->ter] = now ? now : 1;   // in it, nothing there to explore it can reach: not due for that a while
        if (m->ter < 0 || m->ter >= n || !sn_due(idx, m->ter, s, now)) {   // the next one: the nearest due (never stood in first), on its land, no other scout's
            unsigned land = area_id(p, 2);
            int b = -1, bd = 0;
            for (int pass = 0; pass < 2 && b < 0; pass++)   // SCOUTS ACROSS: none left on its land - one across the water (the ferry takes it)
                for (int r = 0; r < n; r++) {
                    int du = sn_due(idx, r, s, now);
                    if (!du || (land && (area_id(g_ter.t[r].c, 2) == land) != !pass) || (pass && !EXPAND_ACROSS(idx))) continue;
                    int taken = 0, d = ivec2_dist2(g_ter.t[r].c, p) / (du == 2 ? 4 : 1);
                    for (int j = 0; j < nn; j++) taken |= j != i && z->m[j].scout == 1 && z->m[j].ter == r;
                    if (!taken && (b < 0 || d < bd)) { b = r; bd = d; }
                }
            if (b < 0 || !player_ai_toggle(pl, SM_AIT_EXPLORE)) {   // none left (or the scenario's Explore off): the main force's again
                unsigned r = ter_near(p.x, p.y);
                m->scout = 0; m->ter = (short)(r < (unsigned)n ? (int)r : 0); m->order_t = 0;
                z->scouted++;
                z->scout_off = now + SN_SCOUT_REST;   // what is left lies on other land: no scout is taken for it again at once (372 taken and sent back in an hour for 2 territories across the water)
                continue;
            }
            m->ter = (short)b; m->order_t = 0; m->since = now ? now : 1;
        }
        unsigned tl = area_id(g_ter.t[m->ter].c, 2), pl_ = area_id(p, 2);
        if (tl && pl_ && tl != pl_) {   // SCOUTS ACROSS: the ferry's (asked each look; its give-up time from when it is over)
            int k = 0, f = -1;
            while (k < SN_SX && !(g_sn_sx[idx][k].u == m->u && g_sn_sx[idx][k].id == m->id)) { if (f < 0 && (!g_sn_sx[idx][k].t || now - g_sn_sx[idx][k].t > 15000)) f = k; k++; }
            if (k == SN_SX) k = f;
            if (k >= 0) { g_sn_sx[idx][k].u = m->u; g_sn_sx[idx][k].id = m->id; g_sn_sx[idx][k].from = pl_; g_sn_sx[idx][k].to = tl; g_sn_sx[idx][k].at = g_ter.t[m->ter].c; g_sn_sx[idx][k].ter = m->ter; g_sn_sx[idx][k].t = now ? now : 1; }
            m->since = now ? now : 1;
            continue;
        }
        if (!m->order_t || (now - m->order_t > SN_REORDER && sn_idle(z, m, now))) sn_order(z, m, 0, g_ter.t[m->ter].c, 0, now);
    }
    // PATROLS (watch.c: the player 2026-11-16, "it should regulary patrol these alrger unknows patches with single
    // units"): as many of the main force as watch.c wants (by the army's size, no more than there are dark patches; the
    // scenario's Explore off: none), each the fastest (of those the one that sees furthest) of a territory with no
    // enemy soldiers in it and not pushing - marked scout 3: no share, no push takes it. Each walks (no attack-move:
    // what it finds the view and the pushes answer) to the nearest dark patch no other patrol has, the next one as
    // soon as that patch's middle is in sight; one not seen WT_GIVE_UP after it was taken is given up (watch_skip).
    // None left for it, or fewer wanted: the main force's again, stationed where it stands.
    {
        watch_tick(pl, idx, now);
        int want_pt = player_ai_toggle(pl, SM_AIT_EXPLORE) ? watch_patrols_wanted(idx, np[SNP_MAIN]) : 0, npt = 0;
        for (int i = 0; i < nn; i++) if (z->m[i].scout == 3) npt++;
        while (npt < want_pt) {
            int b = -1; float bf = -1.0f;
            for (int i = 0; i < nn; i++) {
                struct sn_unit *m = &z->m[i];
                if (m->pool != SNP_MAIN || m->scout || m->ter < 0 || m->ter >= n || s->t[m->ter].foe > 0) continue;
                int pushing = 0;
                for (int p = 0; p < SN_PUSHES; p++) pushing |= z->push[p].on && z->push[p].from == m->ter;
                ee_unit_stats *mst = unit_get_type_data(m->u) ? unit_get_stats(m->u) : NULL;
                float f = mst ? unit_stats_get_speed(mst) * 100.0f + unit_stats_get_sight(mst) : 0.0f;
                if (!pushing && f > bf) { bf = f; b = i; }
            }
            if (b < 0) break;
            struct sn_unit *m = &z->m[b];
            m->scout = 3; m->ter = -1; m->order_t = 0; m->since = 0; m->look = iv2(-1, -1);
            npt++; watch_count(idx, 0);
        }
        for (int i = 0; i < nn; i++) {
            struct sn_unit *m = &z->m[i];
            if (m->scout != 3) continue;
            ivec2 p = unit_get_tile(m->u);
            if (!want_pt) m->look = iv2(-1, -1);   // switched off, the scenario's Explore off, too few soldiers: all back
            if (m->look.x >= 0 && wt_in_sight(pl, m->look)) { m->look = iv2(-1, -1); watch_count(idx, 1); }   // seen: the next one
            else if (m->look.x >= 0 && m->since && now - m->since > WT_GIVE_UP) { watch_skip(idx, m->look, now); m->look = iv2(-1, -1); watch_count(idx, 2); }
            if (m->look.x < 0 && npt <= want_pt) {
                ivec2 others[2 * WT_PATROLS];
                int no = 0;
                for (int j = 0; j < nn && no < 2 * WT_PATROLS; j++) if (j != i && z->m[j].scout == 3 && z->m[j].look.x >= 0) others[no++] = z->m[j].look;
                if (watch_patrol_target(idx, p, area_id(p, 2), others, no, &m->look)) { m->since = now ? now : 1; m->order_t = 0; }
            }
            if (m->look.x < 0) {   // nothing left for it (or one too many): the main force's again
                unsigned r = ter_near(p.x, p.y);
                m->scout = 0; m->ter = (short)(r < (unsigned)n ? (int)r : 0); m->order_t = 0;
                npt--;
                continue;
            }
            if (!m->order_t || (now - m->order_t > SN_REORDER && sn_idle(z, m, now))) sn_order(z, m, 0, m->look, 0, now);
        }
    }
    // THE RESERVE (the player 2026-11-15: "the ai should never stop training units if it can help it, if its at its pop
    // limit it should train straight into the stash"): units inside a Fortress do not count toward the population.
    // Every SN_RES_EVERY: with no room left for the training (SN_RES_ROOM places: the population and the units its
    // buildings have queued - bunker.c bk_room_free) and no territory of its under attack (SN_RES_ODDS), up to
    // (and no citizens the shelf would take first: bunker.c shelf_first) SN_RES_BATCH soldiers of the main force and the siege - not of a territory that pushes - go into a Fortress of
    // the reserve, the nearest to it first; the training fills the map again. While the bank pays the training
    // claims a place for each production building (FULL STEAM), a look loads several Fortresses, and more are built
    // ahead (FORTRESSES AHEAD; bunker.c bk_build, BK_MAX). Out again, a Fortress a look: the nearest to a territory of its with enemy
    // soldiers in it (SN_RES_FOE of them), or the fullest that fits into the room on the map (SN_RES_ROOM kept) - and nobody in for
    // SN_RES_AGAIN after that.
    if (!z->res_t || now < z->res_t || now - z->res_t >= SN_RES_EVERY) {
        z->res_t = now ? now : 1;
        // FULL STEAM (the player 2026-11-16: "idealy the ai keeps always training on full steam just filling up
        // reserves"): while the bank pays for training (army.c ap_training_pays) the room the training claims is a place
        // for each of its production buildings (produce.c), else what they have queued (bunker.c bk_room_free) - with 5
        // places kept, 28-43 buildings had 20-30 units in production between them, 3-6 soldiers a minute where they
        // make about 50. room: what is spare beyond that claim
        int nb = ap_training_pays(pl, idx) ? g_pr[idx].buildings : 0, q = units_queued(pl), claim = q > nb ? q : nb;
        int room = bk_room(pl) - claim, hard_r = -1, ob = -1, od = 0;
        z->res_room = room; z->res_claim = claim; g_res_claim[idx] = nb;
        // a scout or two let nobody out (the first game: 37 of 44 looks let a fortress out); nor a few raiders where its
        // soldiers stand SN_RES_ODDS to one (the player's game of 2026-11-16: P2's 1000 soldiers at the cap, 4+ enemies
        // in one of its 7 territories most minutes - 140 of its own there - and nobody stashed)
        for (int r = 0; r < n && hard_r < 0; r++) if (!g_ter.t[r].water && s->t[r].role != TVR_NONE && fn[r] >= SN_RES_FOE && fn[r] * SN_RES_ODDS > ac[r]) hard_r = r;
        g_bk_more[idx] = 0;
        // out for room only with SN_RES_BATCH to spare beyond the Fortress's soldiers and not within SN_RES_AGAIN of a
        // load: the room a load makes is the training's until its units come out (they were let out again before)
        int quiet_in = !z->res_in_t || now < z->res_in_t || now - z->res_in_t >= SN_RES_AGAIN;
        if (hard_r >= 0 || (room > SN_RES_ROOM && quiet_in))
            for (int k = 0; k < g_bk[idx].nf; k++) {
                ee_unit *fo = g_bk[idx].f[k];
                if (g_bk[idx].store[k] != 2 || bk_held(fo) <= 0) continue;
                if (hard_r < 0 && bk_held(fo) + SN_RES_ROOM + SN_RES_BATCH > room) continue;   // ONLY FOR ROOM (the player 2026-11-15: "if the pop limit has space again the units should go out and do tasks"): whoever fits into the room on the map comes out
                int d = hard_r >= 0 ? ivec2_dist2(unit_get_tile(fo), g_ter.t[hard_r].c) : -bk_held(fo);
                if (ob < 0 || d < od) { ob = k; od = d; }
            }
        if (LET_FEW(idx) && (hard_r >= 0 || (room > SN_RES_ROOM + SN_RES_BATCH && quiet_in))) {
            // LET OUT FEW (bunker.c): as many as are wanted, one at a time - for a territory under attack what its soldiers
            // lack to SN_RES_ODDS to one (SN_RES_BATCH at least), the nearest Fortress first; for room what the map
            // holds beyond SN_RES_ROOM + SN_RES_BATCH, the fullest first
            int want = hard_r >= 0 ? fn[hard_r] * SN_RES_ODDS - ac[hard_r] : room - SN_RES_ROOM - SN_RES_BATCH;
            if (hard_r >= 0 && want < SN_RES_BATCH) want = SN_RES_BATCH;
            if (want > SN_RES_LOOK) want = SN_RES_LOOK;
            ee_unit *fo = NULL;
            int got = bk_let_out_of(pl, idx, 2, want, 0, hard_r >= 0 ? &g_ter.t[hard_r].c : NULL, &fo);
            if (got > 0) {
                z->res_out += (unsigned)got;
                z->res_out_t = now ? now : 1;
                if (g_log) log_locked("# reserve: P%u at %.1f min: room %d%s - %d soldiers out, one at a time (%d wanted; the last from the fortress at %d,%d; %u in, %u out so far)\n", idx, now / 60000.0, room,
                                      hard_r >= 0 ? ", enemy soldiers in a territory of its" : "", got, want, unit_get_x(fo), unit_get_y(fo), z->res_in, z->res_out);
            }
        } else if (ob >= 0) {
            ee_unit *fo = g_bk[idx].f[ob];
            int held = bk_held(fo);
            if (bk_unload(fo)) {
                int again = z->res_last == fo && z->res_last_n == held;   // told again: the game took the first order and did nothing (bunker.c bk_unload tries the next side)
                z->res_last = fo; z->res_last_n = held;
                if (!again) z->res_out += (unsigned)held;
                z->res_out_t = now ? now : 1;
                if (g_log && !again) log_locked("# reserve: P%u at %.1f min: room %d%s - the fortress at %d,%d lets its %d soldiers out (%u in, %u out so far)\n", idx, now / 60000.0, room,
                                      hard_r >= 0 ? ", enemy soldiers in a territory of its" : "", unit_get_x(fo), unit_get_y(fo), held, z->res_in, z->res_out);
            }
        } else if (hard_r < 0 && room <= SN_RES_ROOM && (nb > 0 || !shelf_first(pl, idx)) &&   /* FULL STEAM: beside the shelf - alone it kept up 20 a look (cap 500, 61 buildings, 32-42 in production) */ (!z->res_out_t || now < z->res_out_t || now - z->res_out_t >= SN_RES_AGAIN)) {
            // a look loads SN_RES_BATCH and what the map holds beyond the claim, into as many Fortresses as that takes
            // (one load order a Fortress at a time: a new one replaces the one before)
            int want = SN_RES_BATCH + (room < 0 ? -room : 0), cap1 = 40, have = 0;
            if (want > SN_RES_LOOK) want = SN_RES_LOOK;
            static uint8_t used[BK_MAX];
            memset(used, 0, sizeof used);
            for (int k = 0; k < g_bk[idx].nall; k++) {   // FORTRESSES AHEAD: unfinished ones count as empty
                ee_unit *bs = unit_get_physics(g_bk[idx].all[k]);
                if (!bs || !unit_get_finished(bs)) have += cap1;
            }
            for (int k = 0; k < g_bk[idx].nf; k++) {
                ee_unit *fo = g_bk[idx].f[k];
                if (bk_capacity(fo) > 0) cap1 = bk_capacity(fo);
                if (g_bk[idx].store[k] == 1 || g_bk[idx].shelter[k] || (!g_bk[idx].store[k] && bk_held(fo) > 0)) continue;
                have += bk_space(fo) > 0 ? bk_space(fo) : 0;
            }
            for (int k = 0; k < g_bk[idx].nf && want > 0; k++) {   // the Fortresses in no particular order: the soldiers nearest each go
                int best = -1, space = 0;
                for (int j = 0; j < g_bk[idx].nf; j++) {
                    ee_unit *fo = g_bk[idx].f[j];
                    int sp = bk_space(fo);   // by the soldiers' sizes (bunker.c)
                    if (used[j] || g_bk[idx].store[j] == 1 || g_bk[idx].shelter[j] || g_bk[idx].walk[j] > 0 || (!g_bk[idx].store[j] && bk_held(fo) > 0)) continue;
                    if (sp > space) { space = sp; best = j; }
                }
                if (best < 0 || space < 5) break;
                used[best] = 1;
                ee_unit *fo = g_bk[idx].f[best], *pick[64];
                int pi_[64], npk = 0, left = space;
                ivec2 fp = unit_get_tile(fo);
                while (npk < 64 && npk < want && left > 0) {
                    int ci = -1, cdist = 0;
                    for (int i = 0; i < nn; i++) {
                        struct sn_unit *m = &z->m[i];
                        if (m->scout || m->pool == SNP_AA || m->ter < 0 || m->ter >= n || fn[m->ter] > 0 || (m->stash_t && now >= m->stash_t && now - m->stash_t < SN_RES_AGAIN)) continue;
                        if (bk_size(m->u) > left) continue;   // what still fits
                        int busy = 0;
                        for (int h = 0; h < SN_PUSHES; h++) busy |= z->push[h].on && z->push[h].from == m->ter;
                        for (int j = 0; j < npk; j++) busy |= pi_[j] == i;
                        if (busy) continue;
                        int d = ivec2_dist2(unit_get_tile(m->u), fp);
                        if (ci < 0 || d < cdist) { ci = i; cdist = d; }
                    }
                    if (ci < 0) break;
                    pi_[npk] = ci; pick[npk++] = z->m[ci].u;
                    left -= bk_size(z->m[ci].u);
                }
                if (!npk) break;   // nobody left to take
                if (bk_load(fo, pick, npk)) {
                    g_bk[idx].store[best] = 2; g_bk[idx].walk[best] += npk;
                    for (int j = 0; j < npk; j++) z->m[pi_[j]].stash_t = now ? now : 1;
                    z->res_in += (unsigned)npk;
                    z->res_in_t = now ? now : 1;
                    want -= npk;
                    if (g_log) log_locked("# reserve: P%u at %.1f min: room %d (claim %d) - %d soldiers into the fortress at %d,%d (holds %d; %u in, %u out so far)\n", idx, now / 60000.0, room, claim, npk, fp.x, fp.y, bk_held(fo), z->res_in, z->res_out);
                }
            }
            // FORTRESSES AHEAD: room in them for SN_RES_AHEAD units a production building (a minute of full steam,
            // a unit about every 30 s each), SN_RES_BATCH at least - else as many more as that takes (bunker.c bk_build)
            int ahead = SN_RES_AHEAD * g_pr[idx].buildings;
            if (ahead < SN_RES_BATCH) ahead = SN_RES_BATCH;
            if (have < ahead) g_bk_more[idx] = (ahead - have + cap1 - 1) / cap1;
        }
    }
    // DEMOLITION: each territory's enemy wall piece or gate nearest its place, and who is at one already
    static ee_unit *demo[PR_T];
    static int dn[PR_T];
    for (int r = 0; r < n; r++) { demo[r] = NULL; dn[r] = 0; }
    {
        static int dd[PR_T];
        for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
            ee_player *o = unit_get_owner(it.u);
            if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || unit_get_x(it.u) < 0 ||
                !(is_wall(unit_type_get_id(it.td)) || unit_type_get_class_id(it.td) == CLS_GATE) || !seen_by(it.u, pl)) continue;
            ivec2 q = unit_get_tile(it.u);
            unsigned r = ter_near(q.x, q.y);
            if (r >= (unsigned)n || s->t[r].role == TVR_NONE || s->t[r].ebld || fn[r] > 0) continue;
            int d = ivec2_dist2(q, rally[r]);
            if (!demo[r] || d < dd[r]) { demo[r] = it.u; dd[r] = d; }
        }
        for (int i = 0; i < nn; i++) {   // at a piece already
            ee_goal *g = unit_get_goal(z->m[i].u);
            ee_unit *tg = g && goal_is_attack(g) ? goal_get_target(g) : NULL;
            ee_unit_type *td = tg ? unit_get_type_data(tg) : NULL;
            if (td && z->m[i].ter >= 0 && z->m[i].ter < n && (is_wall(unit_type_get_id(td)) || unit_type_get_class_id(td) == CLS_GATE)) dn[z->m[i].ter]++;
        }
    }
    // ORDERS
    static uint8_t rammed[SN_UNITS];   // RAMS AT THE TOWERS: the rams it gave a building
    memset(rammed, 0, (size_t)nn);
    static short nram[PR_T];
    memset(nram, 0, (size_t)n * sizeof *nram);
    for (int i = 0; RAM_TOWERS(idx) && i < nn; i++) if (z->m[i].pool == SNP_SIEGE && z->m[i].ter >= 0 && z->m[i].ter < n && sn_is_ram(z->m[i].u)) nram[z->m[i].ter]++;
    for (int r = 0; RAM_TOWERS(idx) && r < n && player_ai_toggle(pl, SM_AIT_ATTACK); r++) {
        if (nram[r] < 1 || g_ter.t[r].water) continue;
        int pushing = 0;
        for (int k = 0; k < SN_PUSHES; k++) pushing |= z->push[k].on && z->push[k].from == r;
        sn_rams(pl, idx, z, nn, r, pushing, rammed, now);
    }
    static struct { short r, i; } fm[SN_FORM_MAX];   // A STANDING FORMATION: the idle at rest on their patch
    int nf = 0;
    static struct { short r, cur; uint8_t fight, urgent; ivec2 d; int n; long sx, sy; uint32_t oldest; uint32_t ids[SN_GROUP_MAX]; short mi[SN_GROUP_MAX]; } grp[SN_GROUPS];
    int ngrp = 0;
    for (int i = 0; i < nn; i++) {
        struct sn_unit *m = &z->m[i];
        if (m->scout || m->ter < 0 || m->ter >= n || rammed[i]) continue;
        if (!cl_may(m->u, CLP_ROUTINE, CLW_STATION)) { m->wait_t = 0; continue; }   // ORDERS BY PRIORITY (claims.c): another's for now
        // THE RESERVE: on its way into a fortress - as long as its load order lasts, not only SN_RES_AGAIN (a walk
        // longer than that was called back to its station on the way)
        if ((m->stash_t && now >= m->stash_t && now - m->stash_t < SN_RES_AGAIN) || (m->stash_t && bk_walking_into(m->u))) continue;
        int r = m->ter, reach = SN_NEAR, fight = 1;
        ivec2 d = rally[r], p = unit_get_tile(m->u);
        struct sn_push *pu = NULL;
        for (int k = 0; k < SN_PUSHES; k++) if (z->push[k].on && z->push[k].from == r) pu = &z->push[k];
        if (m->pool == SNP_AA) {   // ANTI-AIR: at its station's place, whatever goes on
            if (ivec2_dist2(p, d) > SN_NEAR * SN_NEAR && (!m->order_t || (now - m->order_t > SN_REORDER && sn_idle(z, m, now)))) sn_order(z, m, i, d, 0, now);
            continue;
        }
        if (!pu && demo[r] && (m->pool == SNP_MAIN || (m->pool == SNP_SIEGE && ac[r] >= SN_SIEGE_ESCORT))) {   // DEMOLITION (the siege with soldiers of its there: THE SIEGE'S ESCORT)
            ee_goal *cg = unit_get_goal(m->u);
            ee_unit *tg = cg && goal_is_attack(cg) ? goal_get_target(cg) : NULL;
            ee_unit_type *td = tg ? unit_get_type_data(tg) : NULL;
            if (td && unit_get_hp(tg) > 0 && (is_wall(unit_type_get_id(td)) || unit_type_get_class_id(td) == CLS_GATE)) { m->wait_t = 0; continue; }   // at it
            if (dn[r] < SN_DEMO && ar_idle(m->u)) {
                if (sn_attack_piece(pl, m->u, demo[r])) { m->order_t = now ? now : 1; m->to = unit_get_tile(demo[r]); m->wait_t = 0; dn[r]++; z->orders++; z->demos++; continue; }
            }
        }
        {   // COAST BATTERIES: THE POST - its siege at rest stands its ground there; leaving it, its stance back
            ivec2 cp;
            if (m->pool == SNP_SIEGE && !m->scout && !pu && !(fn[r] > 0 && s->t[r].role != TVR_NONE) && sn_coast_at(idx, r, &cp)) {
                if (ivec2_dist2(p, cp) <= 3 * 3) {
                    m->wait_t = 0;
                    if (!m->post) { m->post = 1; m->stance0 = (int8_t)unit_stance(m->u); unit_set_stance(m->u, 1); g_snc[idx].posted++; }
                    continue;
                }
                if (!m->order_t || ivec2_dist2(m->to, cp) > 3 * 3 || (now - m->order_t > SN_REORDER && sn_idle(z, m, now))) sn_order(z, m, i, cp, 0, now);
                m->wait_t = 0;
                continue;
            }
            if (m->post) { m->post = 0; if (m->stance0 >= 0) unit_set_stance(m->u, m->stance0); m->stance0 = -1; }
        }
        if (!pu && !(fn[r] > 0 && s->t[r].role != TVR_NONE) && (m->pool == SNP_MAIN || m->pool == SNP_SIEGE)) {   // A STANDING FORMATION: at rest
            struct sn_fc *fc = sn_fc_at(idx, r);
            sn_fc_spot(fc, d, !anc[r], ac[r] + acs[r], now);
            d = fc->spot;
            if (ivec2_dist2(p, d) <= SN_FORM_IN * SN_FORM_IN) {   // there: the formation's (below)
                m->wait_t = 0;
                if (ar_idle(m->u) && nf < SN_FORM_MAX) { fm[nf].r = (short)r; fm[nf++].i = (short)i; }
                continue;
            }
        }
        if (m->pool == SNP_SIEGE && (pu ? ivec2_dist2(pu->mmid, p) <= SN_SIEGE_NEAR * SN_SIEGE_NEAR && pu->mn >= SN_SIEGE_ESCORT :
                                     fn[r] > 0 && s->t[r].role != TVR_NONE && ac[r] >= SN_SIEGE_ESCORT)) {   // SIEGE AT THE GATES
            ee_goal *cg = unit_get_goal(m->u);
            ee_unit *tg = cg && goal_is_attack(cg) ? goal_get_target(cg) : NULL;
            if (tg && unit_get_hp(tg) > 0 && unit_type_get_class_id(unit_get_type_data(tg)) == CLS_GATE) { m->wait_t = 0; continue; }   // at one
            ee_unit *gate = sn_gate_near(pl, p, SN_GATE_R);
            if (gate && (!m->order_t || now - m->order_t >= 3000)) {   // as a player's right-click: an attack goal made on a gate here crashed the game (its walls-family piece lookup, 0x6311A8, has none for a gate)
                sn_attack_msg(pl, (uint32_t)unit_get_serial(m->u), gate);
                m->order_t = now ? now : 1; m->to = unit_get_tile(gate); m->wait_t = 0; z->orders++; z->gates++;
                continue;
            }
        }
        if (pu && pu->across && area_id(p, 2) != pu->to_land) {   // ACROSS: not over yet - at its station till the ferry takes it
            if (ivec2_dist2(p, d) <= SN_NEAR * SN_NEAR || (m->order_t && ivec2_dist2(m->to, d) <= 36 && now - m->order_t < SN_REORDER)) { m->wait_t = 0; continue; }
            sn_order(z, m, i, d, 1, now); m->wait_t = 0; continue;
        }
        if (pu && pu->across && pu->left * 4 > pu->left + pu->over) {   // ACROSS: over, three quarters of them not yet - the beach held
            d = pu->beach;
            if (ivec2_dist2(p, d) <= 6 * 6 || (m->order_t && ivec2_dist2(m->to, d) <= 36 && now - m->order_t < SN_REORDER)) { m->wait_t = 0; continue; }
            sn_order(z, m, i, d, 1, now); m->wait_t = 0; continue;
        }
        if (pu && pu->breach) {   // A BREACH: the siege (everybody when it has none there) at the piece, the rest by it
            ee_unit *piece = sn_wall_at(pl, pu->wall);
            int escorted = ivec2_dist2(pu->mmid, pu->wall) <= SN_SIEGE_NEAR * SN_SIEGE_NEAR;   // THE SIEGE'S ESCORT: its soldiers by the wall
            if (piece && ((m->pool == SNP_SIEGE && escorted) || !acs[r])) {
                if ((!m->order_t || (now - m->order_t >= 3000 && sn_idle(z, m, now))) && sn_attack_piece(pl, m->u, piece)) { m->order_t = now ? now : 1; m->to = pu->wall; z->orders++; }
                continue;
            }
            d = pu->wall; reach = 6;
            if (m->pool == SNP_SIEGE) { d = pu->mmid; reach = 4; }   // ... else with them
        } else
        if (pu) { d = m->pool == SNP_SIEGE ? pu->mmid : pu->at; reach = m->pool == SNP_SIEGE ? 4 : 5; }   // THE SIEGE'S ESCORT: in a push the siege goes where its soldiers are, never ahead to the building
        else if (fn[r] > 0 && s->t[r].role != TVR_NONE && (m->pool != SNP_SIEGE || ac[r] >= SN_SIEGE_ESCORT)) { d = iv2(fx[r] / fn[r], fy[r] / fn[r]); reach = 5; }   // enemies there: the siege only with soldiers of its there
        else if (m->pool == SNP_SIEGE && !(fn[r] > 0 && s->t[r].role != TVR_NONE)) fight = 0;
        if (ivec2_dist2(p, d) <= reach * reach) { m->wait_t = 0; continue; }
        int moved = m->order_t && ivec2_dist2(m->to, d) > 6 * 6;   // its place has moved: the enemy, the push's next building
        if (!(!m->order_t || (moved && now - m->order_t >= 10000) || (now - m->order_t > SN_REORDER && sn_idle(z, m, now)))) continue;
        // AS A GROUP: with the others that stand in its territory and go to the same place
        int cur = (int)ter_near(p.x, p.y), urgent = pu || (fn[r] > 0 && s->t[r].role != TVR_NONE), k = 0;
        while (k < ngrp && (grp[k].r != r || grp[k].cur != cur || grp[k].fight != fight)) k++;
        if (k == ngrp) {
            if (ngrp == SN_GROUPS) { sn_order(z, m, i, d, fight, now); m->wait_t = 0; continue; }
            grp[k].r = (short)r; grp[k].cur = (short)cur; grp[k].fight = (uint8_t)fight; grp[k].urgent = 0; grp[k].d = d; grp[k].n = 0; grp[k].sx = grp[k].sy = 0; grp[k].oldest = now; ngrp++;
        }
        if (grp[k].n == SN_GROUP_MAX) { sn_order(z, m, i, d, fight, now); m->wait_t = 0; continue; }
        if (!m->wait_t) m->wait_t = now ? now : 1;
        if (m->wait_t < grp[k].oldest) grp[k].oldest = m->wait_t;
        grp[k].urgent |= (uint8_t)urgent;
        grp[k].ids[grp[k].n] = (uint32_t)unit_get_serial(m->u); grp[k].mi[grp[k].n++] = (short)i;
        grp[k].sx += p.x; grp[k].sy += p.y;
    }
    for (int k = 0; k < ngrp; k++) {
        if (grp[k].n < SN_GROUP && !grp[k].urgent && now - grp[k].oldest < SN_GROUP_WAIT) continue;   // they wait for more
        ivec2 from = iv2((int)(grp[k].sx / grp[k].n), (int)(grp[k].sy / grp[k].n));
        if (grp[k].n == 1) { struct sn_unit *m = &z->m[grp[k].mi[0]]; sn_order(z, m, 0, grp[k].d, grp[k].fight, now); m->wait_t = 0; continue; }
        sn_group_move(pl, grp[k].ids, grp[k].n, from, grp[k].d, grp[k].fight);
        z->orders++; z->groups++; z->grouped += (unsigned)grp[k].n;
        for (int j = 0; j < grp[k].n; j++) { struct sn_unit *m = &z->m[grp[k].mi[j]]; m->order_t = now ? now : 1; m->to = grp[k].d; m->wait_t = 0; }
    }
    for (int a = 0; a < nf; a++) {   // A STANDING FORMATION: each territory's idle at rest, laid out again when due
        if (fm[a].r < 0) continue;
        int r = fm[a].r, cnt = 0, off = 0;
        struct sn_fc *fc = sn_fc_at(idx, r);
        static uint32_t ids[SN_FORM_MAX];
        static short mi[SN_FORM_MAX];
        int tight = 3 + 2 * fc->box;   // off the block: farther than this from the patch
        for (int b = a; b < nf; b++) {
            if (fm[b].r != r) continue;
            struct sn_unit *m = &z->m[fm[b].i];
            ids[cnt] = (uint32_t)unit_get_serial(m->u); mi[cnt++] = fm[b].i;
            off |= ivec2_dist2(unit_get_tile(m->u), fc->spot) > tight * tight;
            fm[b].r = -1;
        }
        if (cnt < fc->form_n) fc->form_n = cnt;   // some left or fell: the rest stands
        if (!(cnt > fc->form_n || off) || (fc->form_t && now >= fc->form_t && now - fc->form_t < SN_FORM_AGAIN)) continue;
        ivec2 from = rally[r];   // facing: the enemy's way (THE NEED'S PLACE, the last look's), else out from the point
        if (z->need_on) from = iv2(2 * fc->spot.x - z->need_at.x, 2 * fc->spot.y - z->need_at.y);
        if (cnt == 1) sn_order(z, &z->m[mi[0]], 0, fc->spot, 1, now);
        else {
            sn_group_move(pl, ids, cnt, from, fc->spot, 1);
            z->orders++;
            for (int k = 0; k < cnt; k++) { struct sn_unit *m = &z->m[mi[k]]; m->order_t = now ? now : 1; m->to = fc->spot; }
        }
        fc->form_t = now ? now : 1; fc->form_n = cnt; z->formed++;
    }
    z->need_on = 0;   // THE NEED'S PLACE
    for (int k = 0; k < SN_PUSHES && !z->need_on; k++) if (z->push[k].on) { z->need_at = z->push[k].at; z->need_on = 1; }
    if (!z->need_on) {
        int b = -1, big = -1, bd = 0;
        for (int r = 0; r < n; r++) {
            if (fn[r] > 0 && s->t[r].role != TVR_NONE && (b < 0 || fn[r] > fn[b])) b = r;
            if (tc[r] > 0 && (big < 0 || tc[r] > tc[big])) big = r;
        }
        if (b >= 0) { z->need_at = iv2(fx[b] / fn[b], fy[b] / fn[b]); z->need_on = 1; }
        for (int e = 0; b < 0 && big >= 0 && e < neb; e++) {
            int d = ivec2_dist2(eb[e].p, rally[big]);
            if (!z->need_on || d < bd) { z->need_at = eb[e].p; z->need_on = 1; bd = d; }
        }
    }
    if (g_log && (!z->log_t || now - z->log_t >= 60000)) {
        z->log_t = now ? now : 1;
        char buf[400] = "";
        int o = 0, pushing = 0;
        for (int r = 0; r < n && o < 330; r++)
            if (tc[r] || ac[r]) o += snprintf(buf + o, sizeof buf - (size_t)o, " %d:%d/%d%s", r, ac[r], tc[r], fn[r] && s->t[r].role != TVR_NONE ? "!" : "");
        for (int k = 0; k < SN_PUSHES; k++) pushing += z->push[k].on;
        int rh = 0, rf = 0;   // THE RESERVE: soldiers inside its Fortresses
        for (int k = 0; k < g_bk[idx].nf; k++) if (g_bk[idx].store[k] == 2) { rh += bk_held(g_bk[idx].f[k]); rf++; }
        log_locked("# stations: P%u at %.1f min: the main force %d, siege %d, anti-air %d, scouts %d (territories never stood in: %d, due a look again: %d) | pushes on %d, so far %u started, %u won, %u lost | "
                   "the reserve %d soldiers in %d fortresses (so far %u in, %u out), room spare %d beyond the training's %d, %d more fortresses wanted | "
                   "so far %u changes of station, %u orders (%u to groups, %u soldiers in them, %u formations laid at rest), %u scouts back, %u scouts' goals inside their territory, %u set at enemy walls, %u siege at enemy gates, %u orders taken for dead | territory:stationed/share (! enemy soldiers there):%s\n",
                   idx, now / 60000.0, np[SNP_MAIN], np[SNP_SIEGE], np[SNP_AA], scouts, unseen, z->due, pushing, z->pushes, z->won, z->lost, rh, rf, z->res_in, z->res_out, z->res_room, z->res_claim, g_bk_more[idx],
                   z->moved, z->orders, z->groups, z->grouped, z->formed, z->scouted, z->looks, z->demos, z->gates, z->stale, buf);
    }
}
internal void stations_install(const char *autotest_ini) {
    if (autotest_ini) g_v_sn_rams = ini_int("autotest", "v_ram_towers", -1, autotest_ini);   // RAMS AT THE TOWERS
    if (autotest_ini) g_v_cl = ini_int("autotest", "v_claims", -1, autotest_ini);   // claims.c ORDERS BY PRIORITY
    g_sn_coast = g_set_ai[1].coast_siege;   // COAST BATTERIES
    if (autotest_ini) g_v_sn_coast = ini_int("autotest", "v_coast_siege", -1, autotest_ini);
}
internal void stations_new_game(void) { memset(g_snc, 0, sizeof g_snc); memset(g_sn, 0, sizeof g_sn); memset(g_sn_seen, 0, sizeof g_sn_seen); memset(g_sn_look, 0, sizeof g_sn_look); memset(g_sn_dark_t, 0, sizeof g_sn_dark_t); }
