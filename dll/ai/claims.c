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
// fleet's and the squadrons' orders. MEASURED 2026-11-19 (head-to-head, Small Islands large, 4 AIs from epoch 5, 60 min,
// seeds 1-12 mirrored, 24 games; B without claims, v_claims=0): B - A soldiers -8.8 (se 5.2), kills -2.8 (7.9), losses
// +10.0 (7.5), razed -0.9 (0.5); warships lost 24.1 against 24.2 per AI, sunk 23.5 against 24.9. On. Next: transports
// (ferry, convoys, landings), then citizens, then soldiers.
enum { CLP_SURVIVE = 1, CLP_REPAIR, CLP_CARRY, CLP_TASK, CLP_ROUTINE };
enum { CLW_NONE, CLW_STORM, CLW_FLEE, CLW_REPAIR, CLW_SQUADRON, CLW_FLEET, CLW_N };
internal const char *const k_clw[CLW_N] = { "-", "storm", "danger zone", "repair", "squadron", "fleet" };
#define CL_N 4096
#define CL_SWEEP 5000
#define CL_REPAIR_T 120000   // a repair's claim, renewed each look while it goes
#define CL_SURVIVE_T 15000
struct cl_entry { ee_unit *u; int id; uint8_t prio, who; int8_t stance0; uint32_t until; };
internal struct cl_entry g_cl[CL_N];
internal struct { unsigned taken, over, refused[CLW_N][CLW_N]; uint32_t log_t; } g_cls[16];
internal uint32_t g_cl_sweep;
internal int g_cl_on = 1, g_v_cl = -1;   // [autotest] v_claims=0: none for the variant's players (every order as before)
#define CL_ON(idx) PV(idx, g_v_cl, g_cl_on)
internal int cl_live(const struct cl_entry *e, uint32_t now) {
    return e->u && unit_get_serial(e->u) == e->id && unit_get_hp(e->u) > 0 && now < e->until;
}
internal unsigned cl_owner(ee_unit *u) { ee_player *o = unit_get_owner(u); return o ? (unsigned)player_get_index(o) & 15 : 0; }
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
    if (!CL_ON(idx)) return 1;
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
    struct cl_entry *e = u && CL_ON(cl_owner(u)) ? cl_find(u, NULL) : NULL;
    if (!e || !cl_live(e, now) || e->who == who || prio < e->prio) return 1;
    g_cls[cl_owner(u)].refused[who][e->who]++;
    return 0;
}
internal void cl_stance(ee_unit *u, int stance) {   // the claimed unit's stance while the claim lasts
    struct cl_entry *e = u && CL_ON(cl_owner(u)) ? cl_find(u, NULL) : NULL;
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
    for (int k = 0; k < CL_N; k++) g_cl[k].stance0 = -1;
}
