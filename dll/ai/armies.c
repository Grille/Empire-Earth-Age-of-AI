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
// On top of it (the same test, 24 games each): the held standing at the front station between waves (front_army,
// ~720 station walks a game) - soldiers +8.5 (6.1), kills -2.7 (11.8), losses +4.3 (12.9), razed -0.8 (1.8): nothing;
// the release only when the army beats the strongest enemy army known by army_mass_ratio (army_mass WAIT) - it never
// waited (the enemy army is rarely known before the attack: intel's soldiers seen within 10 min), parity with noise
// (razed -4.6 (1.4), workers -21 (8.5)). The push judgement needs an estimate of its own. The retreat below (the wave
// called off under 70%): soldiers +10 (4.3), kills -5.8 (9.1), losses +3.7 (10.0), razed -4.5 (1.7), workers -17 (7.3)
// - the judge misreads a spread wave (a wave of 95 against one unit and towers called off). The defence below on top:
// kills -34 (15.6), losses +29 (16.3), razed -3.6 (1.2): 195 raids a game answered with 1353 single orders - the
// soldiers thrown at the enemy's wave one at a time. Buildings lost A / B: parity 4.5 / 6.2, station 5.8 / 6.7,
// retreat 3.0 / 7.5, defence 2.6 / 6.2 - holding the soldiers idle between waves costs about 1.7 buildings each way:
// the commander keeps 3-4 squads out all the time (continuous pressure), and its soldiers at home stand in groups
// that go for what comes near them together. What the mod's army lacks is the group: orders to a force as one
// (attack-move as a mass, a move back as one), its strength judged as a whole - the next step.
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
#define AR_DEFEND_EVERY 3000
#define AR_DEFEND_R 14             // a raid: an enemy fighter seen within this of an own building
#define AR_DEFEND_REACH 30         // the held soldiers within this of the raid go for it
#define AR_DEFEND_RAIDS 8
#define AR_RETREAT_PCT 70          // the wave called off once its fight at its middle falls under this (twice running)
#define AR_RETREAT_EVERY 5000
#define AR_RETREAT_HOME 60000      // after the call-off: the freed walk home as they are held, this long
#define AR_RELEASE_MS 20000        // the game's launch comes within this of the attack time, or didn't
internal int g_mod_armies = 0, g_v_mod_armies = -1;
#define MOD_ARMIES(idx) PV(idx, g_v_mod_armies, g_mod_armies)
internal struct armies_st {
    uint32_t t, log_t, release_t, release_due;
    int n;
    struct { ee_unit *u; int id; } held[AR_HELD];
    uint32_t defend_t;
    unsigned defends, defended;            // raids answered, soldiers sent at them
    uint32_t judge_t, retreat_t;           // the wave's last judgement; the last call-off (its freed sent home)
    int weak;                              // judgements running under AR_RETREAT_PCT
    ivec2 home;                            // where the retreat goes
    unsigned held_all, launches, launched, refused, no_launch, retreats, sent_home;
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
// DEFEND: the commander's groups at home go for enemies that come near them (a member's target scoring), a held
// soldier only fights back - held, the army lost 7.5 buildings a game against the commander's 3.0 (24 games). So
// every AR_DEFEND_EVERY: an enemy land fighter it sees within AR_DEFEND_R of one of its buildings is a raid (up to
// AR_DEFEND_RAIDS), and every held soldier within AR_DEFEND_REACH of a raid that isn't attacking goes for the raider
// nearest it (the game's attack goal: it fights on from there, and is idle - held - again after).
internal int fl_attacking(ee_unit *u);   // fleet.c: an attack order at a live target
internal void ar_defend(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {
    if (a->defend_t && now >= a->defend_t && now - a->defend_t < AR_DEFEND_EVERY) return;
    a->defend_t = now ? now : 1;
    if (!a->n) return;
    static ee_unit *raider[AR_DEFEND_RAIDS];
    int nr = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it) && nr < AR_DEFEND_RAIDS; ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !ms_land_soldier(u, it.td) || !seen_by(u, pl)) continue;
        ivec2 p = unit_get_tile(u);
        int close = 0, same = 0;
        for (int k = 0; k < nr && !same; k++) same = ivec2_dist2(unit_get_tile(raider[k]), p) <= AR_DEFEND_R * AR_DEFEND_R;   // one raid, one raider
        if (same) continue;
        for (struct grid_it jt = grid_it_box(p.x, p.y, AR_DEFEND_R, 1); grid_it_next(&jt) && !close; )
            close = unit_get_owner(jt.u) == pl && unit_get_hp(jt.u) > 0 && fr_building(jt.td) && ivec2_dist2(unit_get_tile(jt.u), p) <= AR_DEFEND_R * AR_DEFEND_R;
        if (close) raider[nr++] = u;
    }
    if (!nr) return;
    int sent = 0;
    for (int k = 0; k < a->n; k++) {
        ee_unit *u = a->held[k].u;
        if (unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || fl_attacking(u)) continue;
        ivec2 p = unit_get_tile(u);
        ee_unit *t = NULL;
        long bd = (long)AR_DEFEND_REACH * AR_DEFEND_REACH + 1;
        for (int j = 0; j < nr; j++) { long d = ivec2_dist2(unit_get_tile(raider[j]), p); if (d < bd) { bd = d; t = raider[j]; } }
        if (t && unit_goal(u, goal_attack(u, t), 0, 0x1F40)) sent++;
    }
    if (sent) { a->defends++; a->defended += (unsigned)sent; }
    (void)idx;
}
// RETREAT (the mod's own, the player: "retreating into fortified positions when they are losing"): every
// AR_RETREAT_EVERY while a wave is out (members in subgroups past forming, away from the base), the fight at the
// members' middle is judged as the stage judges a march's end (stage.c: the enemy fighters seen and remembered within
// SG_FOE, the defences covering it, against the own fighters within SG_NEAR) - under AR_RETREAT_PCT twice running, the
// attack on that player is called off (the game's call_off_attacks, as diplo.c does it: no unit offered to the groups
// being freed) and the next attack scheduled anew; the freed are held on the next ticks and each sent home - the
// staging point when it stands, else the base - so they walk back together.
internal void ar_retreat(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {
    if (a->judge_t && now >= a->judge_t && now - a->judge_t < AR_RETREAT_EVERY) return;
    a->judge_t = now ? now : 1;
    ivec2 b;
    if (!ms_base(pl, &b)) return;
    int ns = ms_subgroups(pl, b), n = 0;
    long sx = 0, sy = 0;
    ee_player *target = NULL;
    for (int k = 0; k < ns; k++) {
        if (!g_msg[k].out || g_msg[k].n < 2) continue;
        n += g_msg[k].n; sx += (long)g_msg[k].c.x * g_msg[k].n; sy += (long)g_msg[k].c.y * g_msg[k].n;
        if (!target) target = attack_subgroup_get_target(g_msg[k].sg);
    }
    if (n < AR_MIN_LAUNCH || !target) { a->weak = 0; return; }
    ivec2 mid = iv2((int)(sx / n), (int)(sy / n));
    stage_foe(pl, mid);
    st_side_clear(&g_sgs_own);
    stage_own(pl, mid, SG_NEAR, mid, 0);
    float ratio = st_ratio(&g_sgs_own, &g_sgs_foe);
    if (ratio * 100.0f >= (float)AR_RETREAT_PCT) { a->weak = 0; return; }
    if (++a->weak < 2) return;
    a->weak = 0;
    g_in_calloff = 1;
    player_call_off_attacks(pl, target, 1);
    g_in_calloff = 0;
    player_schedule_attack(pl, now);
    struct stage *st = &g_stg[idx];
    a->home = st->on && !stage_danger(idx, st->s) ? st->s : b;
    a->retreat_t = now ? now : 1;
    a->retreats++;
    if (g_log && g_autotest) {
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# armies: P%u at %.1f min: the wave of %d at %d,%d called off - its fight %.2f (own %d, enemy %d) - home to %d,%d, next attack at %.1f min\n",
                idx, now / 60000.0, n, mid.x, mid.y, ratio, g_sgs_own.n, g_sgs_foe.n, a->home.x, a->home.y, player_get_next_attack(pl) / 60000.0);
        LeaveCriticalSection(&g_lock);
    }
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
        if (a->retreat_t && now - a->retreat_t < AR_RETREAT_HOME && ivec2_dist2(unit_get_tile(u), a->home) > 8 * 8) {   // the retreat: home
            int k = unit_get_id(u);
            if (unit_goal(u, goal_move((float)a->home.x + 0.5f + (float)(k % 5) - 2.0f, (float)a->home.y + 0.5f + (float)(k / 5 % 5) - 2.0f), 0, 0x1F40)) a->sent_home++;
        }
    }
    ar_defend(pl, idx, a, now);
    ar_retreat(pl, idx, a, now);
    if (g_log && g_autotest && (!a->log_t || now < a->log_t || now - a->log_t >= 60000)) {
        a->log_t = now ? now : 1;
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# armies: P%u at %.1f min: held %d (in all %u), launches %u with %u soldiers let go (%u not taken up), retreats %u (%u sent home), raids answered %u (%u soldiers), offers refused %u, next attack at %.1f min\n", idx, now / 60000.0,
                a->n, a->held_all, a->launches, a->launched, a->no_launch, a->retreats, a->sent_home, a->defends, a->defended, a->refused, player_get_next_attack(pl) / 60000.0);
        LeaveCriticalSection(&g_lock);
    }
}
internal void armies_new_game(void) { memset(g_armies, 0, sizeof g_armies); }
internal void armies_install(const char *autotest_ini) {
    g_mod_armies = g_set_ai[1].mod_armies;
    if (autotest_ini) g_v_mod_armies = ini_int("autotest", "v_mod_armies", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# mod armies: %d (variant %d)\n", g_mod_armies, g_v_mod_armies);
}
