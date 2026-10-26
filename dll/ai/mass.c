// THE ARMY AS ONE (docs/economy.md "The army as one"). Included by eeaimod.c after fleet.c (fl_attacking), intel.c,
// landing.c, defence.c (is_soldier, unit_power, defence_stats).
// The playtester: against the base game's brain-dead nonstop waves a proper AI gathers its soldiers under cover
// (towers, Town Center, production), lets the waves break on the towers and the massed army, strikes only when the
// mass is strong enough against the enemy force it knows, and never feeds single soldiers into a fight -
// reinforcements gather first and join as groups. army_plan (army.c) gave 5 more soldiers on Continental, but in
// head-to-head games they died without killing more (losses +14): the trickle.
//
// "# mass:" once a game minute (autotest; the mod's AIs and the vanilla players alike), sampled every 5 s: own land
// soldiers (`sold`), alone (`iso`: 2 or fewer own land soldiers within 6 tiles), away from the base (`away`: more than
// 25 tiles), both, in a fight (`eng`: an attack order at a live target) and alone in it; the land attack subgroups
// (`sg`), those out (`out`: their members' middle 30+ tiles from the base), their members (`asg`) and stragglers
// (`strag`: 12+ tiles from their subgroup's middle); this minute's land soldiers killed (`died`) - alone, away, both,
// outnumbered (`outn`: enemy soldiers within 8 tiles at least twice the own + 1) - and what army_mass did.
//
// army_mass ([ai_base], bits; [autotest] v_army_mass for the variant players; land or mixed maps only - navy_land):
// 1 GATHER: a member of a land attack subgroup that is out (state past forming, its members' middle 30+ tiles from
//   the base) but stands at home (within 25 tiles: a replacement the attack commander gave it) is held - the
//   subgroup's orders to it refused (landing.c hook_asg_give_goal, vf57; the land hand-off keeps its goal) - and walks
//   to the rally point: by the own tower nearest the point 6 tiles from the base toward the subgroup, 2 tiles on
//   the base's side of it (else that point). Released together once army_mass_group (5) of that subgroup wait, 3
//   for 2 minutes, or any after 4 minutes (a held member can keep the subgroup "engaged").
// 2 WAIT: the attack time (player +0xED8) comes while the AI's land soldiers (hit points x attack) are below
//   army_mass_ratio (130%) of the strongest enemy's army it knows (intel.c: soldiers and siege seen within 10
//   minutes) - the launch moves on by 20 s, for army_mass_wait (10) minutes at most; meanwhile the army is at home.
// 4 (was RETREAT: attack_retreat for this AI - removed, docs/experiments.md; the bit does nothing now).
// 8 (with 1): GATHER only while that subgroup fights (state 3000/3003).
// 16 TETHER: a member of a land subgroup out picks no target more than army_mass_tether (12) tiles from the middle of
//   its members away from the base unless it is within its own range + 2 (attack.c hook_asgu_score); 128 (with 16):
//   only buildings - enemy units are fought wherever they are. 32 REGROUP: a member out, tether + 4 tiles from that
//   middle, attacking nothing, walks back to it. 64 JOIN (with 1): released reinforcements walk to their subgroup's
//   middle (else the subgroup's next orders take them).
// "# mass2:" once a game minute (autotest): the land subgroups' members by state - forming, marching (2000s),
// fighting (3000s), other - and their stragglers at home / ahead of / behind the middle; the fronts (out subgroups
// 20+ tiles from each other) and the share of samples with 2+ out; the subgroups' orders (vf57: moves / attacks);
// TETHER, REGROUP and JOIN counts; the land soldiers killed - in an out subgroup's pack (12 tiles), ahead of it,
// behind it, on the way from home, in none out. Measured against the base AI (hard, Continental e0, 40 games):
// 8 of a subgroup's ~10 members are in the fight state at a time, stragglers are fighters 12+ tiles ahead (15-18%) or
// behind (7-9%) and replacements from home (9-11%); marching subgroups stay together (their stragglers mostly the
// replacements at home); 61-67% of the soldiers killed die in the pack, 13-16% ahead of it, 7-8% behind, 1% on the
// way from home. With 2+ subgroups out (a third of the minutes) 4.9 of them stand on 2.1 fronts - but a land
// subgroup requests ~17 routes a game: the fronts come from the fights, not the marches (a goal switch at the route
// request never acted).
// Measured (base AI at hard, Continental e0, 40 games a cell): half the mod's land soldiers that die die alone, 70-84%
// of them away from the base; a quarter of an attack subgroup's members string out 12+ tiles. Paired, 1 against the
// build without: no cheat kills - losses +11.0 (3.4), losses -4.0 (2.7), mod wins 20 -> 14; with the cheat losses -7.3
// (3.5) but kills -17.0 (8.1), razed -2.3 (1.0), base AI wins 3 -> 4; head-to-head (24 games) kills +9.0 (3.6), losses
// +1.3 (5.8); lowpop (20) kills +9.6, losses -6.9. 9 milder, 2 hardly acts, 4 and 7 cost kills against the cheat
// (-20). 16 (no cheat / cheat): losses -7.4 (2.8) / -4.8 (4.0), kills -1.1 / -13.9 (5.6); 144: 80 games a cell kills
// - losses +2.6 (1.4) / -2.5 (3.5), head-to-head (48 games) kills +1.1 (3.2), losses +6.0 (4.0); 17, 65, 145 like
// GATHER (wins without the cheat 20 -> 12-19, kills with it -15 to -24). Default 0.
internal int g_army_mass = 0, g_v_army_mass = -1;
internal int g_mass_group = 5, g_mass_hold = 120, g_mass_ratio = 130, g_mass_wait = 10, g_mass_tether = 12;
#define ARMY_MASS(idx) PV(idx, g_v_army_mass, g_army_mass)
#define MS_HOLD 96
#define MS_SG 64
internal struct mass {
    uint32_t ts, tf, tr, tw; unsigned min; int started;
    unsigned samples, sold, iso, away, isoaway, eng, engiso, sg, out, asg, strag;   // sums over the samples
    unsigned die, die_iso, die_away, die_isoaway, die_outn;
    unsigned dsg[5];   // land soldiers killed: in an out subgroup's pack (within 12 tiles of its middle), ahead of it, behind it, behind at home, none
    struct { ee_unit *u; ee_attack_subgroup *sg; uint32_t since; } hold[MS_HOLD]; int nh;
    struct { ee_unit *u; uint32_t t; } gone[MS_HOLD]; int ng;   // released: not held again for 3 minutes
    ivec2 r;                                       // GATHER: the rally
    uint32_t wait0, wait_until;                    // WAIT: the first postponement of this attack, the time it was due
    unsigned held, released, refused, waits, waited, goes;   // since the last line (refused: the subgroups' orders)
    // "# mass2:" (autotest): per subgroup state class (0 forming/other < 2000, 1 march 2000s, 2 fight 3000s, 3 other)
    // its members, stragglers at home, out ahead of / behind the middle (toward the base); fronts (out subgroups 20+
    // tiles apart), samples with 2+ out; the subgroups' orders (vf57) per class: moves, attacks
    unsigned dm[4], dsh[4], dsa[4], dsb[4], fronts, multi, omv[4], oat[4];
    // TETHER / REGROUP (16 / 32): the subgroups out and their members' middle (every 2 s); targets skipped, regroup orders
    struct { ee_attack_subgroup *sg; ivec2 c; } tc[MS_SG]; int ntc;
    unsigned tethered, regrouped, joined;
} g_mss[16];
internal void mass_new_game(void) { memset(g_mss, 0, sizeof g_mss); }

internal int ms_land_soldier(ee_unit *u, ee_unit_type *td) {
    int fam = unit_type_get_class_id(td);
    return unit_get_hp(u) > 0 && unit_get_x(u) >= 0 && is_soldier(td) && fam != CLS_AIRPLANE && fam != CLS_NAVAL && fam != CLS_HELICOPTER &&
           fam != CLS_SUBMERGED_NAVAL && fam != CLS_HELICOPTER_TRANSPORT && fam != CLS_NAVAL_TRANSPORT && fam != CLS_NAVAL_BUILDING && fam != CLS_NAVAL_CITIZEN;
}
internal int ms_base(ee_player *pl, ivec2 *b) {   // an AI's (a human player's +0xB90 is no computer manager: garbage)
    int i = pl ? player_get_index(pl) & 15 : 0;
    if (!pl || !(g_players[i] || is_vanilla_idx(i))) return 0;
    ee_civ_mgr *cm = player_get_civ_mgr(pl); ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!bm || build_mgr_get_base_x(bm) == -10000) return 0;
    *b = build_mgr_get_base(bm);
    return 1;
}
// own land soldiers (pl's) within r tiles of p, not counting u; enemy soldiers of pl there (en)
internal int ms_near(ee_player *pl, ee_unit *u, ivec2 p, int r, int *en) {
    int n = 0;
    if (en) *en = 0;
    for (struct grid_it it = grid_it_box(p.x, p.y, r, 1); grid_it_next(&it); ) {
        if (it.u == u || !ms_land_soldier(it.u, it.td) || ivec2_dist2(unit_get_tile(it.u), p) > r * r) continue;
        ee_player *o = unit_get_owner(it.u);
        if (o == pl) n++;
        else if (en && o && player_get_index(o) > 0 && is_enemy(pl, o)) (*en)++;
    }
    return n;
}
// the land attack subgroups of pl (attack commander player +0xB8C +0x48, groups' subgroup vectors +8..+0xC)
internal struct ms_sg { ee_attack_subgroup *sg; int n, na, state, out; ivec2 c; } g_msg[MS_SG];   // c: its members' middle
internal int ms_subgroups(ee_player *pl, ivec2 b) {
    int ns = 0;
    ee_combat_mgr *cbm = player_get_combat_mgr(pl); ee_attack_cmd *cmd = cbm ? combat_mgr_get_commander(cbm) : NULL;
    if (!cmd) return 0;
    uint8_t *dq = attack_cmd_groups_at(cmd);
    unsigned first = deque_front(dq), cnt = deque_count(dq);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        ee_attack_group *g = deque_at(dq, first + k);
        for (ee_attack_subgroup **sp = g ? attack_group_get_subgroups_begin(g) : NULL; sp && sp < attack_group_get_subgroups_end(g) && ns < MS_SG; sp++) {
            ee_attack_subgroup *sg = *sp;
            if (!sg || !attack_subgroup_is_land(sg)) continue;
            struct ms_sg *s = &g_msg[ns++];
            float x = 0, y = 0, ax = 0, ay = 0;
            s->sg = sg; s->n = s->na = 0; s->state = attack_subgroup_get_state(sg);
            for (ee_asg_unit **r = attack_subgroup_get_units_begin(sg); r && r < attack_subgroup_get_units_end(sg); r++) {
                ee_unit *u = *r ? asg_unit_get_unit(*r) : NULL;
                if (!u || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
                ivec2 p = unit_get_tile(u);
                x += (float)p.x; y += (float)p.y; s->n++;
                if (ivec2_dist2(p, b) > 25 * 25) { ax += (float)p.x; ay += (float)p.y; s->na++; }
            }
            s->c = s->n ? iv2((int)(x / (float)s->n), (int)(y / (float)s->n)) : b;
            // out: past forming (0 next goal, 1000 empty, 1001 starting, 1004 forming) with its members away 30+ tiles
            // from the base - or none away any more while it marches or fights (2000+, not a transport's 8000)
            ivec2 o = s->na ? iv2((int)(ax / (float)s->na), (int)(ay / (float)s->na)) : s->c;
            s->out = s->state != 0 && s->state != 1000 && s->state != 1001 && s->state != 1004 && s->state != 8000 &&
                     (s->na ? ivec2_dist2(o, b) > 30 * 30 : s->state >= 2000);
            if (s->na) s->c = o;
        }
    }
    return ns;
}

// ---- the measurement ----
internal int ms_cls(int st) { return st >= 2000 && st < 3000 ? 1 : st >= 3000 && st < 4000 ? 2 : st >= 4000 ? 3 : 0; }
internal void mass_sample(ee_player *pl, struct mass *m, ivec2 b) {
    m->samples++;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || !ms_land_soldier(u, it.td)) continue;
        ivec2 p = unit_get_tile(u);
        int iso = ms_near(pl, u, p, 6, NULL) <= 2, away = ivec2_dist2(p, b) > 25 * 25, eng = fl_attacking(u);
        m->sold++; m->iso += iso; m->away += away; m->isoaway += iso && away; m->eng += eng; m->engiso += eng && iso;
    }
    int ns = ms_subgroups(pl, b);
    for (int k = 0; k < ns; k++) {
        struct ms_sg *s = &g_msg[k];
        m->sg++; m->out += s->out; m->asg += (unsigned)s->n;
        ivec2 c = iv2(0, 0);   // stragglers: from the middle of all its members
        int n = 0;
        for (ee_asg_unit **r = attack_subgroup_get_units_begin(s->sg); r && r < attack_subgroup_get_units_end(s->sg); r++) {
            ee_unit *u = *r ? asg_unit_get_unit(*r) : NULL;
            if (u && unit_get_hp(u) > 0 && unit_get_x(u) >= 0) { c = ivec2_add(c, unit_get_tile(u)); n++; }
        }
        if (n < 2) continue;
        c.x /= n; c.y /= n;
        int cl = ms_cls(s->state);
        for (ee_asg_unit **r = attack_subgroup_get_units_begin(s->sg); r && r < attack_subgroup_get_units_end(s->sg); r++) {
            ee_unit *u = *r ? asg_unit_get_unit(*r) : NULL;
            if (!u || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
            ivec2 p = unit_get_tile(u);
            int st = ivec2_dist2(p, c) > 12 * 12;
            m->strag += st; m->dm[cl]++;
            if (!st) continue;
            if (ivec2_dist2(p, b) <= 25 * 25) m->dsh[cl]++;
            else if (ivec2_dot(ivec2_sub(p, c), ivec2_sub(c, b)) > 0) m->dsa[cl]++;   // ahead of the middle
            else m->dsb[cl]++;
        }
    }
    int no = 0, fr = 0;   // fronts: out subgroups, a new front when 20+ tiles from every earlier one
    for (int k = 0; k < ns; k++) {
        if (!g_msg[k].out) continue;
        int cl = 0;
        for (int j = 0; j < k && !cl; j++) cl = g_msg[j].out && ivec2_dist2(g_msg[j].c, g_msg[k].c) <= 20 * 20;
        no++; fr += !cl;
    }
    m->fronts += (unsigned)fr; m->multi += no >= 2;
}
// a unit died (diplo.c hook_kill_stat, n 3; autotest): a land soldier - who stood by it
internal void mass_death(ee_unit *dead) {
    if (!g_autotest || !dead) return;
    ee_unit_type *td = unit_get_type_data(dead);
    ee_player *pl = unit_get_owner(dead);
    ivec2 b;
    int fam = td ? unit_type_get_class_id(td) : 3;
    if (!pl || !td || unit_get_x(dead) < 0 || !is_soldier(td) || fam == CLS_AIRPLANE || fam == CLS_NAVAL || fam == CLS_HELICOPTER || fam == CLS_SUBMERGED_NAVAL ||
        fam == CLS_HELICOPTER_TRANSPORT || fam == CLS_NAVAL_TRANSPORT || fam == CLS_NAVAL_BUILDING || fam == CLS_NAVAL_CITIZEN || !ms_base(pl, &b)) return;
    struct mass *m = &g_mss[(unsigned)player_get_index(pl) & 15];
    ivec2 p = unit_get_tile(dead);
    int en, own = ms_near(pl, dead, p, 6, NULL);
    ms_near(pl, dead, p, 8, &en);
    int iso = own <= 2, away = ivec2_dist2(p, b) > 25 * 25;
    m->die++; m->die_iso += iso; m->die_away += away; m->die_isoaway += iso && away; m->die_outn += en >= 2 * (own + 1);
    int ns = ms_subgroups(pl, b), w = 4;
    for (int k = 0; k < ns && w == 4; k++) {
        struct ms_sg *s = &g_msg[k];
        if (!s->out) continue;
        for (ee_asg_unit **r = attack_subgroup_get_units_begin(s->sg); r && r < attack_subgroup_get_units_end(s->sg) && w == 4; r++)
            if (*r && asg_unit_get_unit(*r) == dead)
                w = ivec2_dist2(p, s->c) <= 12 * 12 ? 0 : !away ? 3 : ivec2_dot(ivec2_sub(p, s->c), ivec2_sub(s->c, b)) > 0 ? 1 : 2;
    }
    m->dsg[w]++;
}

// ---- army_mass ----
// GATHER: the rally (every 20 s): the own finished tower within 14 tiles of the base nearest the point 6 tiles out
// toward t, 2 tiles on the base's side of it; else that point
internal void ms_rally(ee_player *pl, struct mass *m, ivec2 b, ivec2 t) {
    float dx = (float)(t.x - b.x), dy = (float)(t.y - b.y), l = sqrtf(dx * dx + dy * dy);
    if (l < 1) { dx = 1; dy = 0; l = 1; }
    ivec2 pt = iv2(b.x + (int)(6.0f * dx / l), b.y + (int)(6.0f * dy / l));
    int bd = 0x7FFFFFFF;
    m->r = pt;
    for (struct grid_it it = grid_it_box(b.x, b.y, 14, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit *bs = unit_get_physics(u);
        float r, a;
        ivec2 p = unit_get_tile(u);
        int d = ivec2_dist2(p, pt);
        if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || !bs || !unit_get_finished(bs) || !defence_stats(u, &r, &a) ||
            ivec2_dist2(p, b) > 14 * 14 || d >= bd) continue;
        float ex = (float)(b.x - p.x), ey = (float)(b.y - p.y), el = sqrtf(ex * ex + ey * ey);
        bd = d;
        m->r = iv2(p.x + (el >= 1 ? (int)(2.0f * ex / el) : 0), p.y + (el >= 1 ? (int)(2.0f * ey / el) : 0));
    }
}
internal int ms_held(struct mass *m, ee_unit *u, ee_attack_subgroup *sg) {
    for (int k = 0; k < m->nh; k++) if (m->hold[k].u == u && (!sg || m->hold[k].sg == sg)) return k + 1;
    return 0;
}
// landing.c hook_asg_give_goal: a land subgroup's order to a member held at the rally
internal int mass_refuses(ee_attack_subgroup *sg, ee_unit *u, ee_goal *goal) {
    ee_player *pl = attack_subgroup_get_player(sg);
    if (!pl || !attack_subgroup_is_land(sg)) return 0;
    struct mass *m = &g_mss[(unsigned)player_get_index(pl) & 15];
    if (g_autotest) {
        int c = ms_cls(attack_subgroup_get_state(sg));
        if (goal && goal_is_attack(goal)) m->oat[c]++; else m->omv[c]++;
    }
    if (!m->nh || !ms_held(m, u, sg)) return 0;
    m->refused++;
    return 1;
}
internal void ms_gather(ee_player *pl, unsigned idx, struct mass *m, uint32_t now, ivec2 b) {
    int ns = ms_subgroups(pl, b);
    static uint8_t keep[MS_HOLD];
    memset(keep, 0, sizeof keep);
    ivec2 front = b;   // the out subgroup nearest the base
    int best = -1;
    for (int k = 0; k < ns; k++) {
        struct ms_sg *s = &g_msg[k];
        if (!s->out || ((ARMY_MASS(idx) & 8) && s->state != 3000 && s->state != 3003)) continue;   // 8: only while it fights
        if (best < 0 || ivec2_dist2(s->c, b) < ivec2_dist2(front, b)) { best = k; front = s->c; }
        for (ee_asg_unit **r = attack_subgroup_get_units_begin(s->sg); r && r < attack_subgroup_get_units_end(s->sg); r++) {
            ee_unit *u = *r ? asg_unit_get_unit(*r) : NULL;
            if (!u || !ms_land_soldier(u, unit_get_type_data(u)) || ivec2_dist2(unit_get_tile(u), b) > 25 * 25 ||
                landing_has_unit(pl, u)) continue;
            int h = ms_held(m, u, s->sg);
            int g = 0;
            for (int j = 0; j < m->ng && !g; j++) g = m->gone[j].u == u && now - m->gone[j].t < 180000;
            if (!h && !g && m->nh < MS_HOLD) {
                m->hold[m->nh].u = u; m->hold[m->nh].sg = s->sg; m->hold[m->nh].since = now ? now : 1;
                h = ++m->nh; m->held++;
            }
            if (h) keep[h - 1] = 1;
        }
    }
    for (int k = m->nh - 1; k >= 0; k--)   // gone, out of the subgroup, the subgroup home or gone: let go
        if (!keep[k]) { m->hold[k] = m->hold[--m->nh]; keep[k] = keep[m->nh]; }
    if (!m->nh) return;
    if (!m->tr || now < m->tr || now - m->tr >= 20000) { m->tr = now ? now : 1; ms_rally(pl, m, b, front); }
    for (int k = 0; k < m->nh; k++) {   // release a subgroup's held members together
        ee_attack_subgroup *sg = m->hold[k].sg;
        int n = 0;
        uint32_t oldest = 0;
        for (int j = 0; j < m->nh; j++)
            if (m->hold[j].sg == sg) { n++; if (now - m->hold[j].since > oldest) oldest = now - m->hold[j].since; }
        if (n >= g_mass_group || (n >= 3 && oldest >= (uint32_t)g_mass_hold * 1000u) || oldest >= (uint32_t)g_mass_hold * 2000u) {
            if (g_log && g_autotest) fprintf(g_log, "# mass: P%u at %.1f min: %d reinforcements go together (held %.1f min)\n", idx, now / 60000.0, n, oldest / 60000.0);
            ivec2 join = iv2(-1, -1);   // 64 JOIN: they walk to the subgroup's pack (its members away from the base)
            for (int i = 0; i < ns && (ARMY_MASS(idx) & 64); i++)
                if (g_msg[i].sg == sg && g_msg[i].na >= 2) join = g_msg[i].c;
            for (int j = m->nh - 1; j >= 0; j--) if (m->hold[j].sg == sg) {
                if (join.x >= 0) m->joined += unit_goal(m->hold[j].u, goal_move((float)join.x + 0.5f + (float)(j % 3) - 1.0f, (float)join.y + 0.5f + (float)(j / 3 % 3) - 1.0f), 0, 0x1F40) != 0;
                int o = 0;   // a free slot of the released list, else its oldest
                if (m->ng < MS_HOLD) o = m->ng++;
                else for (int i = 1; i < MS_HOLD; i++) if (m->gone[i].t < m->gone[o].t) o = i;
                m->gone[o].u = m->hold[j].u; m->gone[o].t = now ? now : 1;
                m->hold[j] = m->hold[--m->nh]; m->released++;
            }
            k = -1;
        }
    }
    for (int k = 0; k < m->nh; k++) {   // to the rally - a member fighting near it keeps fighting
        ee_unit *u = m->hold[k].u;
        ee_unit *t = fl_target_of(u);
        if (ivec2_dist2(unit_get_tile(u), m->r) <= 9 || goal_near(u, (float)m->r.x + 0.5f, (float)m->r.y + 0.5f, 2.5f)) continue;
        if (t && fl_attacking(u) && ivec2_dist2(unit_get_tile(t), m->r) <= 15 * 15) continue;
        unit_goal(u, goal_move((float)m->r.x + 0.5f + (float)(k % 3) - 1.0f, (float)m->r.y + 0.5f + (float)(k / 3 % 3) - 1.0f), 0, 0x1F40);
    }
}
// WAIT: the land army against the strongest enemy army known; the attack time moved on while weaker
internal void ms_wait(ee_player *pl, unsigned idx, struct mass *m, uint32_t now) {
    uint32_t due = player_get_next_attack(pl);
    if (m->wait0 && due != m->wait_until) m->wait0 = 0;   // launched or rescheduled by the game: a new attack
    if (due > now + 15000 && !m->wait0) return;
    if (m->wait0 && due > now + 15000) return;
    float own = 0, foe[16] = { 0 }, top = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); )
        if (unit_get_owner(it.u) == pl && ms_land_soldier(it.u, it.td)) own += unit_power(it.u);
    struct intel *in = &g_intel[idx];
    for (int k = 0; k < in->n; k++)
        if (in->r[k].kind == 1 && in->now - in->r[k].t <= 600000 && in->r[k].owner > 0 && in->r[k].owner < 16)
            foe[in->r[k].owner] += (float)in->r[k].hp * in->r[k].att;
    for (int j = 1; j < 16; j++) {
        void *pj = st_player(j);
        if (pj && is_enemy(pl, pj) && foe[j] > top) top = foe[j];
    }
    if (!m->wait0) m->wait0 = now ? now : 1;
    if (top <= 0 || own * 100.0f >= top * (float)g_mass_ratio || now - m->wait0 >= (uint32_t)g_mass_wait * 60000u) {
        if (now - m->wait0 >= 20000) {
            m->goes++;
            if (g_log && g_autotest) fprintf(g_log, "# mass: P%u at %.1f min: the attack goes after %.1f min - army %.0f, the strongest enemy's known %.0f\n",
                                                 idx, now / 60000.0, (now - m->wait0) / 60000.0, own, top);
        }
        m->wait0 = 0;
        return;
    }
    if (now - m->wait0 < 1000) m->waits++;
    m->waited += 5;
    player_set_next_attack(pl, m->wait_until = now + 20000);
    g_last_attack_time[idx] = m->wait_until;   // no launch (econ.c's attack line)
}
// TETHER (16): the subgroups out (as GATHER's) and the middle of their members away from the base - the pack; a member
// of one picks no target more than army_mass_tether (12) tiles from that middle unless it is within its own range + 2
// of it (attack.c hook_asgu_score; measured: 8 of a subgroup's ~10 members are in combat state at a time, 15% of
// them 12+ tiles ahead of the middle, chasing - the march waits while any member is engaged, so the pack itself only
// moves on once nothing near it is left). REGROUP (32): a member out, in a fighting subgroup, tether + 4 tiles or more
// from the middle and attacking nothing walks back to it.
internal void ms_tether(ee_player *pl, struct mass *m, int plan, ivec2 b) {
    int ns = ms_subgroups(pl, b), t = g_mass_tether + 4;
    m->ntc = 0;
    for (int k = 0; k < ns; k++) {
        struct ms_sg *s = &g_msg[k];
        if (!s->out || s->na < 2) continue;
        m->tc[m->ntc].sg = s->sg; m->tc[m->ntc].c = s->c; m->ntc++;
        if (!(plan & 32) || s->state < 3000 || s->state >= 4000) continue;
        for (ee_asg_unit **r = attack_subgroup_get_units_begin(s->sg); r && r < attack_subgroup_get_units_end(s->sg); r++) {
            ee_unit *u = *r ? asg_unit_get_unit(*r) : NULL;
            if (!u || !ms_land_soldier(u, unit_get_type_data(u)) || fl_attacking(u)) continue;
            ivec2 p = unit_get_tile(u);
            if (ivec2_dist2(p, b) <= 25 * 25 || ivec2_dist2(p, s->c) < t * t ||
                goal_near(u, (float)s->c.x + 0.5f, (float)s->c.y + 0.5f, 4.0f)) continue;
            m->regrouped += unit_goal(u, goal_move((float)s->c.x + 0.5f, (float)s->c.y + 0.5f), 0, 0x1F40) != 0;
        }
    }
}
// attack.c hook_asgu_score: a target too far from the pack of an out subgroup's member
internal int mass_tether(ee_attack_subgroup *sg, ee_unit *me, ee_unit *enemy) {
    ee_player *pl = attack_subgroup_get_player(sg);
    if (!pl || !me || !enemy) return 0;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct mass *m = &g_mss[idx];
    int plan = ARMY_MASS(idx);
    if (!m->ntc || !(plan & 16)) return 0;
    ee_unit_type *etd = unit_get_type_data(enemy);   // 128: buildings only - enemy units are fought wherever they are
    if ((plan & 128) && (!etd || (unit_type_get_class_id(etd) != CLS_BUILDING && unit_type_get_class_id(etd) != CLS_FORTRESS))) return 0;
    int k = 0;
    while (k < m->ntc && m->tc[k].sg != sg) k++;
    if (k == m->ntc) return 0;
    ivec2 e = unit_get_tile(enemy);
    if (ivec2_dist2(e, m->tc[k].c) <= g_mass_tether * g_mass_tether) return 0;
    ee_unit_stats *st = unit_get_stats(me); ee_unit_type *td = unit_get_type_data(me);
    float r = (st ? unit_stats_get_range(st) : td ? unit_type_get_range(td) : 1.0f) + 2.0f;
    if ((float)ivec2_dist2(e, unit_get_tile(me)) <= r * r) return 0;
    m->tethered++;
    return 1;
}
internal void mass_tick(ee_player *pl, uint32_t now) {
    if (!pl) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct mass *m = &g_mss[idx];
    ivec2 b;
    if (now < m->ts || now < m->tf) { memset(m, 0, sizeof *m); }
    if (!ms_base(pl, &b)) return;
    int plan = is_vanilla(pl) ? 0 : ARMY_MASS(idx);
    if (plan && (!m->tf || now - m->tf >= 2000)) {
        m->tf = now ? now : 1;
        if (!navy_land(idx)) m->nh = m->ntc = 0;
        else {
            if (plan & 1) ms_gather(pl, idx, m, now, b); else m->nh = 0;
            if (plan & 48) ms_tether(pl, m, plan, b); else m->ntc = 0;
            if ((plan & 2) && (!m->tw || now - m->tw >= 5000)) { m->tw = now ? now : 1; ms_wait(pl, idx, m, now); }
        }
    }
    if (!g_autotest || !g_log) return;
    if (!m->ts || now - m->ts >= 5000) { m->ts = now ? now : 1; mass_sample(pl, m, b); }
    if (!m->started) { m->started = 1; m->min = now / 60000; return; }
    if (now / 60000 == m->min) return;
    float s = m->samples ? (float)m->samples : 1.0f;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# mass: P%u at %u min: sold %.1f iso %.1f away %.1f isoaway %.1f eng %.1f engiso %.1f | sg %.1f out %.1f asg %.1f strag %.1f | "
            "died %u iso %u away %u isoaway %u outn %u | held %u released %u refused %u holding %d | waits %u waited %u goes %u\n",
            idx, m->min, m->sold / s, m->iso / s, m->away / s, m->isoaway / s, m->eng / s, m->engiso / s, m->sg / s, m->out / s, m->asg / s,
            m->strag / s, m->die, m->die_iso, m->die_away, m->die_isoaway, m->die_outn, m->held, m->released, m->refused, m->nh,
            m->waits, m->waited, m->goes);
    fprintf(g_log, "# mass2: P%u at %u min: form %.1f %.1f %.1f %.1f | march %.1f %.1f %.1f %.1f | fight %.1f %.1f %.1f %.1f | "
            "other %.1f %.1f %.1f %.1f | fronts %.2f multi %.2f | orders %u/%u %u/%u %u/%u %u/%u | tethered %u regrouped %u joined %u | died pack %u ahead %u behind %u home %u none %u\n", idx, m->min,
            m->dm[0] / s, m->dsh[0] / s, m->dsa[0] / s, m->dsb[0] / s, m->dm[1] / s, m->dsh[1] / s, m->dsa[1] / s, m->dsb[1] / s,
            m->dm[2] / s, m->dsh[2] / s, m->dsa[2] / s, m->dsb[2] / s, m->dm[3] / s, m->dsh[3] / s, m->dsa[3] / s, m->dsb[3] / s,
            m->fronts / s, m->multi / s, m->omv[0], m->oat[0], m->omv[1], m->oat[1], m->omv[2], m->oat[2], m->omv[3], m->oat[3], m->tethered, m->regrouped, m->joined,
            m->dsg[0], m->dsg[1], m->dsg[2], m->dsg[3], m->dsg[4]);
    LeaveCriticalSection(&g_lock);
    m->tethered = m->regrouped = m->joined = 0;
    memset(m->dsg, 0, sizeof m->dsg);
    memset(m->dm, 0, sizeof m->dm); memset(m->dsh, 0, sizeof m->dsh); memset(m->dsa, 0, sizeof m->dsa); memset(m->dsb, 0, sizeof m->dsb);
    memset(m->omv, 0, sizeof m->omv); memset(m->oat, 0, sizeof m->oat); m->fronts = m->multi = 0;
    m->samples = m->sold = m->iso = m->away = m->isoaway = m->eng = m->engiso = m->sg = m->out = m->asg = m->strag = 0;
    m->die = m->die_iso = m->die_away = m->die_isoaway = m->die_outn = 0;
    m->held = m->released = m->refused = m->waits = m->waited = m->goes = 0;
    m->min = now / 60000;
}
internal void mass_install(const char *autotest_ini) {
    g_army_mass = g_set_ai[1].army_mass;
    g_mass_group = g_set_ai[1].army_mass_group;
    g_mass_hold = g_set_ai[1].army_mass_hold;
    g_mass_ratio = g_set_ai[1].army_mass_ratio;
    g_mass_wait = g_set_ai[1].army_mass_wait;
    g_mass_tether = g_set_ai[1].army_mass_tether;
    if (autotest_ini) g_v_army_mass = ini_int("autotest", "v_army_mass", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# army mass: %d (variant %d), group %d, hold %d s, ratio %d%%, wait %d min, tether %d\n", g_army_mass, g_v_army_mass,
                       g_mass_group, g_mass_hold, g_mass_ratio, g_mass_wait, g_mass_tether);
}
