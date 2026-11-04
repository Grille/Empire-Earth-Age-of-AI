// AIR STRIKE ([autotest] v_air_strike; AS_ON, off until measured). The player, 2026-11-04, on breaking through a held
// position: "if I have to go through I bombard it, airplanes are preferred if the AA is weak. if strong AA or earlier
// epoch own range units and artillery". The game's own AI flies its planes in its air attack subgroups (EECPASGAir,
// a script state machine) at the commander's target, not at what holds the way of the mod's force. Every AS_EVERY
// per AI with the mod's force (armies.c): the position - the breach's focus while the force stands off (g_ar_breach),
// else the target of its push while it is out -; what holds it: the enemy defences (attack.c defence_stats: a ground
// attack) and artillery or siege seen within AS_R of it, the nearest first; the enemy's anti-air near the first of
// them (within AS_AA_R: an AA building AS_AA_BLD, a land AA unit AS_AA_UNIT, a fighter 1 - seen, the buildings also
// remembered). The anti-air weak - its score x AS_AA_PER within the bombers' count -: every bomber (family bomber or
// atomic bomber) not on one of the targets already gets an attack on one of the first AS_SPREAD of them, spread over
// them, by the game's own air order (0x442AD8: a plane in its airport takes it too - the mod's unit_goal refuses a
// unit off the map). The planes come back to their airports by themselves (the air traffic control). A bomber sent is
// held the game's way (player hold_unit, as armies.c holds its soldiers): unheld, the game's air subgroup took 3-5 of 6
// back to its patrols within 5 s (air_strike.eeai). The strike over - no position, nothing to hit there, the anti-air
// too strong - its bombers are let go as the game does (ai_held cleared, the combat manager's register_unit, 0x48A382).
// "# air strike:" lines; a summary a minute (autotest). OFF (2026-11-04): tests/scenarios/air_strike.eeai (a WWI
// position of the idle human, six Gotha bombers) - the strike's bombers at it from 5.9 min, the first tower down at
// 8.6, but the position falls without them too. The player's save "choke wolf" (pushes every 4 minutes from 30, 45
// and 90 s): never flown - his anti-air near the targets 6-18 against P3's 2 bombers, held back 125-238 times a run.
// Head-to-head from World War I (late.eeai, start_epoch 9, 75 min, 24 mirrored games, 4 AIs) with - without: razed
// +2.0 (se 9.1), kills -0.8 (23.2), losses +1.1 (13.0), soldiers +2.9 (11.2) - the AIs had 0-6 aircraft, a strike
// in a few games (3-14 an AI, nothing to hit 19-132 times). It waits for an AI that trains bombers.

#ifndef AS_ON
#define AS_ON 0
#endif
#define AS_EVERY 5000
#define AS_R 14
#define AS_AA_R 14
#define AS_AA_BLD 3
#define AS_AA_UNIT 2
#define AS_AA_PER 2
#define AS_SPREAD 3
#define AS_TARGETS 32
#define AS_BOMBERS 256
#define AS_HELD 64
#define AS_AGAIN 20000

internal int g_v_air_strike = -1;
internal struct as_st {
    uint32_t t, log_t;
    unsigned strikes, sent, aa_strong, none;   // strikes ordered, bombers sent, held back by the anti-air, positions with bombers but nothing to hit
    struct { int id, tgt; uint32_t t; } held[AS_HELD]; int nh;   // the bombers held for the strike, their targets (serials), when ordered
} g_as[16];
internal short g_as_types[64]; internal int g_as_nt = -1;   // the bomber types (once a game)

internal void as_types(void) {
    if (g_as_nt >= 0) return;
    g_as_nt = 0;
    for (int t = 0; t < G_TYPES_N && g_as_nt < 64; t++) {
        ee_unit_type *td = type_data(t);
        if (!td || unit_type_get_id(td) != t) continue;
        int f = unit_type_get_family(td);
        if ((f == KIND_BOMBER || f == KIND_ATOMIC_BOMBER) && unit_type_get_class_id(td) == CLS_AIRPLANE) g_as_types[g_as_nt++] = (short)t;
    }
    if (g_log) {
        fprintf(g_log, "# air strike: bomber types");
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
// the anti-air pl knows within AS_AA_R of p: AA buildings x AS_AA_BLD, land AA units x AS_AA_UNIT, fighters x 1
internal int as_aa(ee_player *pl, ivec2 p) {
    int s = 0;
    for (struct grid_it it = grid_it_box(p.x, p.y, AS_AA_R, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || unit_get_hp(u) <= 0 || !seen_by(u, pl)) continue;
        if (ivec2_dist2(unit_get_tile(u), p) > AS_AA_R * AS_AA_R) continue;
        int f = unit_type_get_family(it.td);
        s += f == KIND_AA_BUILDING ? AS_AA_BLD : f == KIND_LAND_AA ? AS_AA_UNIT : f == KIND_AIRCRAFT ? 1 : 0;
    }
    return s;
}
// the strike's bombers let go
internal void as_release(ee_player *pl, struct as_st *s) {
    ee_combat_mgr *cmgr = player_get_combat_mgr(pl);
    for (int k = 0; k < s->nh; k++) {
        ee_unit *u = unit_by_id((unsigned)s->held[k].id);
        if (!u || unit_get_serial(u) != s->held[k].id || unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || !unit_get_ai_held(u)) continue;
        unit_set_ai_held(u, 0);
        if (cmgr) combat_mgr_register_unit(cmgr, u);
    }
    s->nh = 0;
}
internal void airstrike_tick(ee_player *pl, uint32_t now) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct as_st *s = &g_as[idx];
    if (!PV(idx, g_v_air_strike, AS_ON) || !ar_on(pl, idx)) return;
    if (s->t && now >= s->t && now - s->t < AS_EVERY) return;
    if (now < s->t) memset(s, 0, sizeof *s);
    s->t = now ? now : 1;
    if (g_log && g_autotest && (!s->log_t || now - s->log_t >= 60000)) {
        s->log_t = now ? now : 1;
        fprintf(g_log, "# air strike: P%u at %.1f min: %u strikes, %u bombers sent, %u times the anti-air too strong, %u times nothing to hit\n",
                idx, now / 60000.0, s->strikes, s->sent, s->aa_strong, s->none);
    }
    struct armies_st *a = &g_armies[idx];
    ivec2 pos;
    if (g_ar_breach[idx].on) pos = g_ar_breach[idx].focus;
    else if (a->out == 1 && a->target) pos = a->dest;
    else { as_release(pl, s); return; }
    as_types();
    // the bombers
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    void *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    static ee_unit *bomb[AS_BOMBERS];
    int nb = 0;
    for (int k = 0; k < g_as_nt; k++) {
        uint8_t *q = type_list(um, g_as_types[k]);
        for (unsigned j = 0, f = q ? deque_front(q) : 0, c = q ? deque_count(q) : 0; j < c && nb < AS_BOMBERS; j++) {
            ee_unit *u = deque_at(q, f + j);
            if (u && unit_get_hp(u) > 0 && unit_get_owner(u) == pl) bomb[nb++] = u;
        }
    }
    if (!nb) return;
    // what holds the position: defences, then artillery and siege - the nearest first
    static struct { ee_unit *u; long d; } tg[AS_TARGETS];
    int nt = 0;
    for (struct grid_it it = grid_it_box(pos.x, pos.y, AS_R, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || unit_get_hp(u) <= 0 || !seen_by(u, pl)) continue;
        long d = ivec2_dist2(unit_get_tile(u), pos);
        if (d > AS_R * AS_R) continue;
        int fam = unit_type_get_class_id(it.td), f = unit_type_get_family(it.td);
        float rg, at;
        if (fam == CLS_BUILDING || fam == CLS_FORTRESS) { if (f == KIND_AA_BUILDING || !defence_stats(u, &rg, &at) || at <= 0) continue; }
        else if (fam == CLS_LAND && (is_siege(unit_type_get_id(it.td)) || f == KIND_ARTILLERY)) d += (long)AS_R * AS_R;   // after the defences
        else continue;
        if (nt == AS_TARGETS && tg[nt - 1].d <= d) continue;
        int k = nt < AS_TARGETS ? nt++ : AS_TARGETS - 1;
        while (k > 0 && tg[k - 1].d > d) { tg[k] = tg[k - 1]; k--; }
        tg[k].u = u; tg[k].d = d;
    }
    if (!nt) { s->none++; as_release(pl, s); return; }
    int aa = as_aa(pl, unit_get_tile(tg[0].u));
    if (aa * AS_AA_PER > nb) {
        if (!s->aa_strong++ || s->aa_strong % 12 == 0)
            ar_log(idx, now, "air strike at %d,%d held back: anti-air %d near it against %d bombers", pos.x, pos.y, aa, nb);
        as_release(pl, s);
        return;
    }
    int spread = nt < AS_SPREAD ? nt : AS_SPREAD, sent = 0;
    for (int i = 0; i < nb; i++) {
        // a bomber of the strike whose target still stands is left alone while it has any goal and for AS_AGAIN after
        // its order: a plane changes its goals by itself on a run - the attack, moves (EEUGLocation) to take off and
        // back to its airport -, and an order every 5 s cut the runs short (air_strike.eeai: 5 of 6 sent again each
        // time); idle (landed, rearmed) it is sent again
        ee_unit *u = bomb[i];
        int id = unit_get_serial(u), e = -1, busy = 0;
        for (int k = 0; k < s->nh && e < 0; k++) if (s->held[k].id == id) e = k;
        ee_goal *g = unit_get_goal(u);
        for (int k = 0; e >= 0 && (g || now - s->held[e].t < AS_AGAIN) && k < nt && !busy; k++) busy = unit_get_serial(tg[k].u) == s->held[e].tgt;
        if (busy) continue;
        ee_unit *t = tg[(sent + i) % spread].u;
        if (e < 0 && s->nh < AS_HELD) {   // first: out of the game's groups
            if (!unit_get_ai_held(u)) player_hold_unit(pl, u);
            e = s->nh++; s->held[e].id = id;
        }
        if (!as_order(u, goal_attack(u, t))) continue;
        sent++;
        if (e >= 0) { s->held[e].tgt = unit_get_serial(t); s->held[e].t = now; }
    }
    if (!sent) return;
    s->strikes++; s->sent += (unsigned)sent;
    ee_unit *t0 = tg[0].u;
    ar_log(idx, now, "air strike at %d,%d: %d of %d bombers at %d targets (the first type %d at %d,%d; anti-air %d)", pos.x, pos.y, sent, nb, spread,
           unit_type_get_id(unit_get_type_data(t0)), unit_get_x(t0), unit_get_y(t0), aa);
}
internal void airstrike_new_game(void) { memset(g_as, 0, sizeof g_as); g_as_nt = -1; }
internal void airstrike_install(const char *autotest_ini) {
    if (autotest_ini) g_v_air_strike = ini_int("autotest", "v_air_strike", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# air strike: %d (variant %d)\n", AS_ON, g_v_air_strike);
}
