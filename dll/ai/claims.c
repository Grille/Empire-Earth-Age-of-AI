// ORDERS BY PRIORITY - who may give a unit an order, while the game's computer player is off. The player 2026-11-19:
// "generaly the ai units seem undecisive, ships send to a dock for repair drive off again just to be called back again
// in a loop / i think we need locking order with priorethy, a unit with a task cant get another order unless its of a
// heigher priorety (get away from the storm would override stay to heal for example)", and "some orders should maybe
// work with the unit stances, repairing units could get stand your ground" - his suspicion: enemies by the dock, the
// ship's own AI pulled it at them, the manager called it back each time.
// A unit's CLAIM: who holds it, at which priority (CLP_: survival before repair before carrying before a task before
// routine), until when. A manager asks before it orders: cl_take claims the unit for it (free, its own already, or held
// at a lower priority by another: taken over) or refuses (held at its priority or a higher one by another); cl_may only
// asks. A claim may set a stance (cl_stance; the game's: 0 scout, 1 stand ground, 2 guard, 3 harass, 4 cautious, 5
// aggressive) - the stance it had before comes back when the claim ends (cl_end) or expires.
// "# claims:" each minute: claims taken, taken over, and the orders refused by who asked and who held the unit.
// Ships first: the repair (fleet.c, squadrons.c), out of a storm (storms.c) or a danger zone (fleet.c fl_flee), the
// fleet's and the squadrons' orders. Citizens (the player, the same day: "next do citezens"): sent off by its safety -
// home from a raided site, away when the base is overrun, to a refuge, a stray home (attack.c citizens_safety) - and
// into a fight as militia or a muster's strike (defence.c) at CLP_SURVIVE, so its safety doesn't pull the militia out
// of the fight it was sent to; the safety's moves at CLP_TASK, HOLDING THE GROUND's builders at CLP_HOLD above it
// (build.c: the ground under fire is its business - with the safety's claim above it, the safety sent them home and
// HOLDING THE GROUND could not send them back for 30 s: islands held 1.10 against 1.48 without claims, 24 games);
// work.c's hand-outs and build.c's builders, routine, ask first - a citizen sent home from danger was not given the
// same site again at the next look.
// Soldiers and siege (the player, the same night: "continue with soilders and siege"): an order trace of 4 test games
// (who ordered each unit, who over whom within 20 s, A-B-A within a minute) found the ships' loop on land - FALL BACK
// TO COVER (cover.c) sent a hurt soldier to the Hospital or a beaten one under its towers, the stations sent it back
// out at their next look, cover pulled it back again: 1000 times over each other, 700 of them to and fro; counter-
// fire's strikes (counterfire.c) and the soldiers sent at enemy tower builders (defence.c, counter-siege) were called
// back the same way. Now cover's hold is a claim - healing at CLP_REPAIR and standing ground while it heals, a break
// from a lost fight at CLP_SURVIVE, renewed while it holds -, the strikes claim theirs at CLP_TASK for their time,
// and the stations' orders (stations.c sn_order, the ORDERS look, the rams) are routine and ask first. MEASURED
// 2026-11-19 (the same test, seeds 1-24 mirrored, 48 games; B with the ships' and citizens' claims only, v_claims=3):
// B - A soldiers -1.9 (se 4.2), kills -1.4 (6.8), losses -0.8 (6.3), razed +0.3 (0.4); held per AI at the end: islands
// 1.32 against 1.21, citizens off the home island 24.6 against 19.4, miners there 9.0 against 6.8, buildings 10.3
// against 8.8 - the fights even, more ground held. On. MEASURED 2026-11-19 (the same test, B without any claims): B - A soldiers +2.1
// (se 4.5), kills -3.3 (10.8), losses -1.3 (10.9), razed -0.5 (0.6), citizens -2.0 (1.0); islands held 1.29 both,
// buildings off the home island 9.1 against 9.3. On.
// The ships' MEASURED 2026-11-19 (head-to-head, Small Islands large, 4 AIs from epoch 5, 60 min,
// seeds 1-12 mirrored, 24 games; B without claims, v_claims=0): B - A soldiers -8.8 (se 5.2), kills -2.8 (7.9), losses
// +10.0 (7.5), razed -0.9 (0.5); warships lost 24.1 against 24.2 per AI, sunk 23.5 against 24.9. On. Next: transports
// (ferry, convoys, landings), then citizens, then soldiers.
// The muster's soldiers (defence.c MUSTER, ITS SOLDIERS CLAIMED; 2026-11-21): it ordered them without asking, so the
// stations and it sent the same soldiers to and fro while it lasted - now its gathering and its strikes claim them at
// CLP_TASK (v_claims part 16).
// The bombers' lone runs (airstrike.c THE LONE RUNS, bomber_harass): a bomber sent on one is claimed at CLP_TASK for
// the run - counter-fire took them as soldiers (roles.c is_soldier counts aircraft) and sent them from their
// target at the mortars and artillery that shelled its soldiers, into their anti-air.
// Above every priority: the player's own orders (godmode's switch - game/api/messages.c THE PLAYER'S ORDERS FIRST): a
// unit the player ordered is refused to every claim, whatever its priority, the refusals counted as held by "the player".
enum { CLP_SURVIVE = 1, CLP_REPAIR, CLP_CARRY, CLP_HOLD, CLP_TASK, CLP_ROUTINE };
enum { CLW_NONE, CLW_STORM, CLW_FLEE, CLW_REPAIR, CLW_SQUADRON, CLW_FLEET, CLW_SAFETY, CLW_MILITIA, CLW_HOLD, CLW_WORK, CLW_BUILD,
       CLW_STATION, CLW_HEAL, CLW_COVER, CLW_COUNTER, CLW_CSIEGE, CLW_FERRY, CLW_EXPLORE, CLW_MUSTER, CLW_BOMBERS, CLW_PLAYER, CLW_N };
internal const char *const k_clw[CLW_N] = { "-", "storm", "danger zone", "repair", "squadron", "fleet", "citizens' safety", "militia", "holding the ground", "work", "builders",
                                            "stations", "healing", "cover", "counter-fire", "counter-siege", "ferry", "exploring", "muster", "lone bomber runs", "the player" };
#define CL_N 4096
#define CL_SWEEP 5000
#define CL_REPAIR_T 120000   // a repair's claim, renewed each look while it goes
#define CL_SURVIVE_T 15000
#define CL_CIT_T 30000       // a citizen sent off by its safety (or into a fight as militia, to build HOLDING THE GROUND): nobody else's this long
struct cl_entry { ee_unit *u; int id; uint8_t prio, who; int8_t stance0; uint32_t until; };
internal struct cl_entry g_cl[CL_N];
internal struct { unsigned taken, over, refused[CLW_N][CLW_N]; uint32_t log_t; } g_cls[16];
internal uint32_t g_cl_sweep;
// [autotest] v_claims: the units the variant's players claim - 1 ships, 2 citizens, 4 the rest (soldiers, siege) - and 8
// the ferry's claims (ferry.c: its trips' transports and passengers, of whichever kind), 16 the muster's (defence.c); 0
// none (every order as before)
internal int g_cl_on = 31, g_v_cl = -1;
internal int is_ship_or_air(ee_unit_type *td);   // attack.c
internal int cl_bit(ee_unit *u) {
    ee_unit_type *td = unit_get_type_data(u);
    return !td ? 4 : is_citizen_type(unit_type_get_id(td)) ? 2 : is_ship_or_air(td) ? 1 : 4;
}
internal int cl_live(const struct cl_entry *e, uint32_t now) {
    return e->u && unit_get_serial(e->u) == e->id && unit_get_hp(e->u) > 0 && now < e->until;
}
internal unsigned cl_owner(ee_unit *u) { ee_player *o = unit_get_owner(u); return o ? (unsigned)player_get_index(o) & 15 : 0; }
internal int cl_on(ee_unit *u) { return (PV(cl_owner(u), g_v_cl, g_cl_on) & cl_bit(u)) != 0; }
internal void cl_free(struct cl_entry *e) {
    if (e->u && e->stance0 >= 0 && unit_get_serial(e->u) == e->id && unit_get_hp(e->u) > 0) unit_set_stance(e->u, e->stance0);
    memset(e, 0, sizeof *e);
    e->stance0 = -1;
}
// u's entry (NULL: none); *slot a free one for it
internal struct cl_entry *cl_find(ee_unit *u, struct cl_entry **slot) {
    int id = unit_get_serial(u);
    unsigned h = ((unsigned)id * 2654435761u) & (CL_N - 1);
    if (slot) *slot = NULL;
    for (int k = 0; k < 64; k++) {
        struct cl_entry *e = &g_cl[(h + (unsigned)k) & (CL_N - 1)];
        if (e->u == u && e->id == id) return e;
        if (!e->u && slot && !*slot) *slot = e;
    }
    return NULL;
}
internal void cl_sweep(uint32_t now) {   // expired ones: their stance back, the place free
    if (g_cl_sweep && now >= g_cl_sweep && now - g_cl_sweep < CL_SWEEP) return;
    g_cl_sweep = now ? now : 1;
    for (int k = 0; k < CL_N; k++) if (g_cl[k].u && !cl_live(&g_cl[k], now)) cl_free(&g_cl[k]);
}
// 1: who may order u at prio now (and holds it until now + dur); 0: another holds it at prio or higher
internal int cl_take(ee_unit *u, int prio, int who, uint32_t dur) {
    uint32_t now = game_time();
    cl_sweep(now);
    struct cl_entry *slot, *e = u ? cl_find(u, &slot) : NULL;
    unsigned idx = u ? cl_owner(u) : 0;
    if (!u) return 0;
    if (pc_held(u)) { g_cls[idx].refused[who][CLW_PLAYER]++; return 0; }
    if (!cl_on(u)) return 1;
    if (e && !cl_live(e, now)) { cl_free(e); e = NULL; slot = NULL; cl_find(u, &slot); }
    if (e && e->who != who) {
        if (prio >= e->prio) { g_cls[idx].refused[who][e->who]++; return 0; }
        g_cls[idx].over++;
    } else if (!e) {
        if (!slot) return 1;   // the table full here: as before, unclaimed
        e = slot; memset(e, 0, sizeof *e); e->stance0 = -1; e->u = u; e->id = unit_get_serial(u);
        g_cls[idx].taken++;
    }
    e->prio = (uint8_t)prio; e->who = (uint8_t)who; e->until = now + dur;
    return 1;
}
internal int cl_may(ee_unit *u, int prio, int who) {   // asks only
    uint32_t now = game_time();
    if (u && pc_held(u)) { g_cls[cl_owner(u)].refused[who][CLW_PLAYER]++; return 0; }
    struct cl_entry *e = u && cl_on(u) ? cl_find(u, NULL) : NULL;
    if (!e || !cl_live(e, now) || e->who == who || prio < e->prio) return 1;
    g_cls[cl_owner(u)].refused[who][e->who]++;
    return 0;
}
internal void cl_stance(ee_unit *u, int stance) {   // the claimed unit's stance while the claim lasts
    struct cl_entry *e = u && cl_on(u) ? cl_find(u, NULL) : NULL;
    if (!e) return;
    int cur = unit_stance(u);
    if (cur < 0 || cur == stance) return;
    if (e->stance0 < 0) e->stance0 = (int8_t)cur;
    unit_set_stance(u, stance);
}
internal void cl_end(ee_unit *u, int who) {
    struct cl_entry *e = u ? cl_find(u, NULL) : NULL;
    if (e && e->who == who) cl_free(e);
}
internal void cl_log(unsigned idx, uint32_t now) {
    if (!g_log || (g_cls[idx].log_t && now >= g_cls[idx].log_t && now - g_cls[idx].log_t < 60000)) return;
    g_cls[idx].log_t = now ? now : 1;
    char b[400] = "";
    for (int a = 0; a < CLW_N; a++)
        for (int h = 0; h < CLW_N; h++) {
            size_t l = strlen(b);
            if (g_cls[idx].refused[a][h] && l < sizeof b - 60) snprintf(b + l, sizeof b - l, " %s by %s %u,", k_clw[a], k_clw[h], g_cls[idx].refused[a][h]);
        }
    log_locked("# claims: P%u at %.1f min: %u taken, %u taken over; orders refused (who asked by who held it):%s\n", idx, now / 60000.0, g_cls[idx].taken, g_cls[idx].over, b[0] ? b : " none");
}
internal void claims_new_game(void) {
    memset(g_cl, 0, sizeof g_cl); memset(g_cls, 0, sizeof g_cls); g_cl_sweep = 0;
    pc_reset();   // the player's orders of the game before
    for (int k = 0; k < CL_N; k++) g_cl[k].stance0 = -1;
}
