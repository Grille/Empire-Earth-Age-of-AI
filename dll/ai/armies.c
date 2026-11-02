// THE MOD'S ARMIES ([ai_base] mod_armies; [autotest] v_mod_armies for the variant players; land maps - navy_land).
// Included by eeaimod.c after stage.c (mass.c ms_land_soldier, landing.c landing_has_unit, army.c ou_sortie,
// attack.c ag_launch / defence_stats / hook_unit_to_group, intel.c g_intel, navy.c navy_land).
// The player (2026-11-01): the game's attack commander is "dumb as rocks" - it grabs every soldier the moment it is
// trained, splits a wave into squads of about ten and sends each on its own tour of enemy buildings with waits at every
// stop; the mod's mass, stage, commit and focus each patch one symptom of that through the same veto on its orders.
// So the commander is replaced for land armies, nothing below it: the game keeps training (army.c shapes the
// requests), unit fighting and movement, ships and aircraft.
// OWNERSHIP: every land soldier the commander would take (combat_mgr_unit_free: a type that fights, alive, moving, not
// chained) is held the game's own way (player hold_unit: the unit's ai_held set, taken out of the commander's pool and
// groups) within AR_TICK of being free, and the commander's offer of a freed unit to its groups (unit_to_group,
// attack.c hook_unit_to_group) is refused for these players - the commander's groups stay empty, its launches find
// nothing. A held unit stands where it is (nothing moves it yet).
// PARITY (the first step, measured against the commander): at the attack time (player next_attack) the held soldiers
// are let go (ai_held cleared, register_unit) and launched as one attack group of the game's own (ag_launch) against
// the known enemy building nearest their middle (a tower or fortress only when no other is left; intel: honest), and
// the next attack scheduled as the game does (schedule_attack). The group is a real one: mass, stage, commit, focus and
// fall back work on it as before; its members, freed when it ends, are held again on the next tick.
// "# armies:" once a game minute (autotest): held now, held in all, launches and their soldiers, offers refused.
#define AR_TICK 2000
#define AR_MIN_LAUNCH 8            // soldiers a launch needs at least (the outdated sortie's)
#define AR_HELD 2048
internal int g_mod_armies = 0, g_v_mod_armies = -1;
#define MOD_ARMIES(idx) PV(idx, g_v_mod_armies, g_mod_armies)
internal struct armies_st {
    uint32_t t, log_t, launch_t;
    int n;
    struct { ee_unit *u; int id; } held[AR_HELD];
    unsigned held_all, launches, launched, refused, no_target;
} g_armies[16];
internal int ar_on(ee_player *pl, unsigned idx) { return pl && !is_vanilla(pl) && MOD_ARMIES(idx) && navy_land(idx); }
internal int ar_held(struct armies_st *a, ee_unit *u) {
    for (int k = 0; k < a->n; k++) if (a->held[k].u == u) return a->held[k].id == unit_get_id(u);
    return 0;
}
// attack.c hook_unit_to_group: the commander's offer of a freed unit to its groups - refused for a land soldier of a
// mod-army player (it stays free; the next tick holds it)
internal int armies_refuses_offer(ee_attack_cmd *cmd, ee_unit *u) {
    (void)cmd;
    ee_player *pl = u ? unit_get_owner(u) : NULL;
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
    if (!td || !ar_on(pl, idx) || !ms_land_soldier(u, td)) return 0;
    g_armies[idx].refused++;
    return 1;
}
// the known enemy building nearest p (intel: buildings seen, still standing): a tower or fortress only when no other
internal ee_unit *ar_target(ee_player *pl, unsigned idx, ivec2 p) {
    struct intel *in = &g_intel[idx];
    ee_unit *best = NULL, *best_def = NULL;
    long bd = 0, bdd = 0;
    for (int k = 0; k < in->n; k++) {
        struct intel_rec *r = &in->r[k];
        ee_unit *u = r->u;
        ee_player *o = r->owner > 0 && r->owner < 16 ? player_at(r->owner) : NULL;
        if (r->kind != 0 || !u || !o || !is_enemy(pl, o) || is_out(o) || unit_get_owner(u) != o || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
        ee_unit_type *td = unit_get_type_data(u);
        int fam = td ? unit_type_get_class_id(td) : -1;
        if (!td || (fam != CLS_BUILDING && fam != CLS_FORTRESS) || is_wall(unit_type_get_id(td))) continue;
        float rg, at;
        long d = ivec2_dist2(unit_get_tile(u), p);
        if (fam == CLS_FORTRESS || defence_stats(u, &rg, &at)) { if (!best_def || d < bdd) { best_def = u; bdd = d; } }
        else if (!best || d < bd) { best = u; bd = d; }
    }
    return best ? best : best_def;
}
internal void ar_release(ee_combat_mgr *cm, ee_unit *u) { unit_set_ai_held(u, 0); combat_mgr_register_unit(cm, u); }
internal void armies_tick(ee_player *pl, uint32_t now) {
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    struct armies_st *a = &g_armies[idx];
    if (now < a->t) memset(a, 0, sizeof *a);   // a new game
    if (!ar_on(pl, idx) || (a->t && now - a->t < AR_TICK)) return;
    a->t = now ? now : 1;
    ee_combat_mgr *cm = player_get_combat_mgr(pl);
    if (!cm) return;
    // the held: the dead and the lost dropped
    int m = 0;
    for (int k = 0; k < a->n; k++) {
        ee_unit *u = a->held[k].u;
        if (unit_get_id(u) == a->held[k].id && unit_get_owner(u) == pl && unit_get_hp(u) > 0) a->held[m++] = a->held[k];
    }
    a->n = m;
    // every free land soldier held
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it) && a->n < AR_HELD; ) {
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || !ms_land_soldier(u, it.td) || ar_held(a, u) || unit_get_ai_held(u)) continue;
        if (!combat_mgr_unit_free(cm, unit_get_physics(u)) || landing_has_unit(pl, u) || ou_sortie(pl, u)) continue;
        player_hold_unit(pl, u);
        a->held[a->n].u = u; a->held[a->n].id = unit_get_id(u); a->n++;
        a->held_all++;
    }
    // the attack time: the held launched as one group at the nearest known enemy building - a tick ahead of the game's
    // own launch (0x46F4FF), which otherwise finds nothing and moves the time on first (one game: 40 -> 63 min, no launch)
    uint32_t due = player_get_next_attack(pl);
    if (a->n >= AR_MIN_LAUNCH && now + AR_TICK >= due && (!a->launch_t || now - a->launch_t >= 60000)) {
        a->launch_t = now ? now : 1;
        long sx = 0, sy = 0;
        for (int k = 0; k < a->n; k++) { sx += unit_get_x(a->held[k].u); sy += unit_get_y(a->held[k].u); }
        ee_unit *target = ar_target(pl, idx, iv2((int)(sx / a->n), (int)(sy / a->n)));
        if (!target) { a->no_target++; player_schedule_attack(pl, now); }
        else {
            static int ids[AR_HELD];
            int n = a->n, ok = 0, nsg = 0;
            for (int k = 0; k < n; k++) { ar_release(cm, a->held[k].u); ids[k] = unit_get_serial(a->held[k].u); }
            ee_attack_group *g = ag_launch(pl, ids, n, target, &ok, &nsg);
            if (g) { a->launches++; a->launched += (unsigned)n; a->n = 0; player_schedule_attack(pl, now); }
            else for (int k = 0; k < n; k++) player_hold_unit(pl, a->held[k].u);   // refused: held on
            if (g_log && g_autotest) {
                EnterCriticalSection(&g_lock);
                fprintf(g_log, "# armies: P%u at %.1f min: %d soldiers launched at %d,%d (P%d type %d): %s (plan ok %d, %d subgroups), next attack at %.1f min\n", idx, now / 60000.0, n,
                        unit_get_x(target), unit_get_y(target), player_get_index(unit_get_owner(target)), unit_type_get_id(unit_get_type_data(target)), g ? "a group" : "refused", ok, nsg,
                        player_get_next_attack(pl) / 60000.0);
                LeaveCriticalSection(&g_lock);
            }
        }
    }
    if (g_log && g_autotest && (!a->log_t || now < a->log_t || now - a->log_t >= 60000)) {
        a->log_t = now ? now : 1;
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# armies: P%u at %.1f min: held %d (in all %u), launches %u with %u soldiers, no target %u, offers refused %u, next attack at %.1f min\n", idx, now / 60000.0,
                a->n, a->held_all, a->launches, a->launched, a->no_target, a->refused, player_get_next_attack(pl) / 60000.0);
        LeaveCriticalSection(&g_lock);
    }
}
internal void armies_new_game(void) { memset(g_armies, 0, sizeof g_armies); }
internal void armies_install(const char *autotest_ini) {
    g_mod_armies = g_set_ai[1].mod_armies;
    if (autotest_ini) g_v_mod_armies = ini_int("autotest", "v_mod_armies", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# mod armies: %d (variant %d)\n", g_mod_armies, g_v_mod_armies);
}
