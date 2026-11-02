// THE MOD'S ARMIES ([ai_base] mod_armies; [autotest] v_mod_armies for the variant players; land maps - navy_land).
// Included by eeaimod.c after stage.c (mass.c ms_land_soldier, landing.c landing_has_unit, army.c ou_sortie,
// attack.c hook_unit_to_group, navy.c navy_land).
// The player (2026-11-01): the game's attack commander is "dumb as rocks" - it grabs every soldier the moment it is
// trained, splits a wave into squads of about ten and sends each on its own tour of enemy buildings with waits at every
// stop; the mod's mass, stage, commit and focus each patch one symptom of that through the same veto on its orders.
// So the commander is replaced for land armies, nothing below it: the game keeps training (army.c shapes the
// requests), unit fighting and movement, ships and aircraft.
// OWNERSHIP: every land soldier the commander would take (combat_mgr_unit_free: a type that fights, alive, moving, not
// chained) is held the game's own way (player hold_unit: the unit's ai_held set, taken out of the commander's pool and
// groups) within AR_TICK of being free, and the commander's offer of a freed unit to its groups (unit_to_group,
// attack.c hook_unit_to_group) is refused for these players. The commander's groups form from those offers - a new
// unit joins a group or founds one, long before the attack time; the launch (0x46F4FF) only sends the formed groups
// (one with no units formed when the offers were refused: 2026-11-02) - so with the offers refused its groups stay
// empty. A held unit stands where it is (nothing moves it yet).
// PARITY (the first step, measured against the commander): a tick before the attack time (player next_attack) the held
// soldiers are let go (ai_held cleared, register_unit: back in the commander's pool), and the game's own launch
// (0x46F4FF) takes them - its target choice, its squads, its tours - so only the holding between attacks differs; what
// the launch leaves free is held again once the attack time has moved on (the launch scheduled the next). The ground
// is then ready for the mod's own launch in the commander's place (the next step).
// Measured 2026-11-02 (head-to-head, Continental huge 1v1 late, 75 min, 24 mirrored games; ~2.6 launches and 183
// soldiers let go a game): soldiers +14.5 (se 6.2), kills +1.3 (6.9), losses -8.0 (8.5), razed -1.8 (1.6), workers
// -8.1 (10.5) - parity, the soldiers trained between two waves waiting at home instead of trickling after one out.
// Before that, with a launch of the mod's own instead - all held soldiers as one group (ag_launch) at the nearest known
// enemy building: soldiers +11.5
// (se 5.4), kills -6.8 (9.0), losses +2.8 (9.0), but razed 3.5 against the commander's 10.3 (-6.8, se 2.1), workers
// -15.6 (6.7): the wave went home once its one building fell. The game's attack-on-a-player form (a task with the
// target player only, strategy kind 0) is refused by attack_plan_ok (it wants a target unit or a location); the
// game's own launch doesn't ask it.
// "# armies:" once a game minute (autotest): held now, held in all, launches and their soldiers, offers refused.
#define AR_TICK 2000
#define AR_MIN_LAUNCH 8            // soldiers a launch needs at least (the outdated sortie's)
#define AR_HELD 2048
#define AR_RELEASE_MS 20000        // the game's launch comes within this of the attack time, or didn't
internal int g_mod_armies = 0, g_v_mod_armies = -1;
#define MOD_ARMIES(idx) PV(idx, g_v_mod_armies, g_mod_armies)
internal struct armies_st {
    uint32_t t, log_t, release_t, release_due;
    int n;
    struct { ee_unit *u; int id; } held[AR_HELD];
    unsigned held_all, launches, launched, refused, no_launch;
} g_armies[16];
internal int ar_on(ee_player *pl, unsigned idx) { return pl && !is_vanilla(pl) && MOD_ARMIES(idx) && navy_land(idx); }
internal int g_ar_offering;   // the mod lets soldiers go: the commander's offers of them pass (armies_refuses_offer)
internal int ar_ptr_cmp(const void *x, const void *y) { ee_unit *a = *(ee_unit *const *)x, *b = *(ee_unit *const *)y; return a < b ? -1 : a > b; }
internal int ou_members(ee_player *pl, ee_unit **out, int max);   // army.c: the units in pl's attack subgroups
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
    if (g_ar_offering || !td || !ar_on(pl, idx) || !ms_land_soldier(u, td)) return 0;
    g_armies[idx].refused++;
    return 1;
}
// the game's way of letting a held unit go (0x48A382): ai_held cleared, registered again - which offers it to the
// commander's groups at once (0x483C59 -> 0x482425 -> unit_to_group: it joins one or founds one), so the offer passes
internal void ar_release(ee_combat_mgr *cm, ee_unit *u) {
    unit_set_ai_held(u, 0);
    g_ar_offering = 1;
    combat_mgr_register_unit(cm, u);
    g_ar_offering = 0;
}
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
    // the attack time: the held let go a tick ahead of the game's launch (0x46F4FF), which takes them from the pool;
    // none held again until it has scheduled the next attack (or AR_RELEASE_MS passed: no launch came)
    uint32_t due = player_get_next_attack(pl);
    if (a->n >= AR_MIN_LAUNCH && now + 2 * AR_TICK >= due && !a->release_t) {
        a->release_t = now ? now : 1; a->release_due = due;
        for (int k = 0; k < a->n; k++) ar_release(cm, a->held[k].u);
        a->launches++; a->launched += (unsigned)a->n;
        if (g_log && g_autotest) {
            EnterCriticalSection(&g_lock);
            fprintf(g_log, "# armies: P%u at %.1f min: %d soldiers let go for the attack at %.1f min\n", idx, now / 60000.0, a->n, due / 60000.0);
            LeaveCriticalSection(&g_lock);
        }
        a->n = 0;
    }
    if (a->release_t && (player_get_next_attack(pl) != a->release_due || now - a->release_t >= AR_RELEASE_MS)) {
        if (player_get_next_attack(pl) == a->release_due) a->no_launch++;
        a->release_t = 0;
        if (g_log && g_autotest) {   // DEBUG: the groups the launch made
            ivec2 b;
            static ee_unit *dm[4096];
            int ns = ms_base(pl, &b) ? ms_subgroups(pl, b) : 0, free_n = 0, dn = ou_members(pl, dm, 4096);
            qsort(dm, (size_t)dn, sizeof *dm, ar_ptr_cmp);
            for (struct grid_it it = grid_it_all(1); grid_it_next(&it); )
                if (unit_get_owner(it.u) == pl && ms_land_soldier(it.u, it.td) && !bsearch(&it.u, dm, (size_t)dn, sizeof *dm, ar_ptr_cmp)) free_n++;
            EnterCriticalSection(&g_lock);
            fprintf(g_log, "# armies: P%u at %.1f min: after the launch %d land subgroups (", idx, now / 60000.0, ns);
            for (int k = 0; k < ns; k++) fprintf(g_log, "%s%d units state %d", k ? ", " : "", g_msg[k].n, g_msg[k].state);
            fprintf(g_log, "), %d soldiers in no subgroup\n", free_n);
            LeaveCriticalSection(&g_lock);
        }
    }
    if (a->release_t) return;
    // every free land soldier held - not a member of an attack subgroup (unit_free is no group test: a member passes
    // it, army.c; the first try held a launched wave back 0.7 s after its launch)
    static ee_unit *mem[4096];
    int nm = ou_members(pl, mem, 4096);
    qsort(mem, (size_t)nm, sizeof *mem, ar_ptr_cmp);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it) && a->n < AR_HELD; ) {
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || !ms_land_soldier(u, it.td) || ar_held(a, u) || unit_get_ai_held(u)) continue;
        if (bsearch(&u, mem, (size_t)nm, sizeof *mem, ar_ptr_cmp)) continue;
        if (!combat_mgr_unit_free(cm, unit_get_physics(u)) || landing_has_unit(pl, u) || ou_sortie(pl, u)) continue;
        player_hold_unit(pl, u);
        a->held[a->n].u = u; a->held[a->n].id = unit_get_id(u); a->n++;
        a->held_all++;
    }
    if (g_log && g_autotest && (!a->log_t || now < a->log_t || now - a->log_t >= 60000)) {
        a->log_t = now ? now : 1;
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# armies: P%u at %.1f min: held %d (in all %u), launches %u with %u soldiers let go (%u not taken up), offers refused %u, next attack at %.1f min\n", idx, now / 60000.0,
                a->n, a->held_all, a->launches, a->launched, a->no_launch, a->refused, player_get_next_attack(pl) / 60000.0);
        LeaveCriticalSection(&g_lock);
    }
}
internal void armies_new_game(void) { memset(g_armies, 0, sizeof g_armies); }
internal void armies_install(const char *autotest_ini) {
    g_mod_armies = g_set_ai[1].mod_armies;
    if (autotest_ini) g_v_mod_armies = ini_int("autotest", "v_mod_armies", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# mod armies: %d (variant %d)\n", g_mod_armies, g_v_mod_armies);
}
