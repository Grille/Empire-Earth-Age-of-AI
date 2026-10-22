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
// army_mass ([all], bits; [autotest] v_army_mass for the variant players; land or mixed maps only - navy_land):
// 1 GATHER: a member of a land attack subgroup that is out (state past forming, its members' middle 30+ tiles from
//   the base) but stands at home (within 25 tiles: a replacement the attack commander gave it) is held - the
//   subgroup's orders to it refused (landing.c hook_asg_give_goal, vf57; the land hand-off keeps its goal) - and walks
//   to the rally point: by the own tower nearest the point 6 tiles from the base toward the subgroup, 2 tiles on
//   the base's side of it (else that point). Released together once army_mass_group (5) of that subgroup wait, 3
//   for 2 minutes, or any after 4 minutes (a held member can keep the subgroup "engaged").
// 2 WAIT: the attack time (player +0xED8) comes while the AI's land soldiers (hit points x attack) are below
//   army_mass_ratio (130%) of the strongest enemy's army it knows (intel.c: soldiers and siege seen within 10
//   minutes) - the launch moves on by 20 s, for army_mass_wait (10) minutes at most; meanwhile the army is at home.
// 4 RETREAT: attack_retreat (defence.c) for this AI - an attack down to half its peak, or facing 1.5 x its strength
//   twice, goes home and comes again within 5 minutes.
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
    struct { ee_unit *u; uint8_t *sg; uint32_t since; } hold[MS_HOLD]; int nh;
    struct { ee_unit *u; uint32_t t; } gone[MS_HOLD]; int ng;   // released: not held again for 3 minutes
    int rx, ry;
    uint32_t wait0, wait_until;                    // WAIT: the first postponement of this attack, the time it was due
    unsigned held, released, refused, waits, waited, goes;   // since the last line (refused: the subgroups' orders)
    // "# mass2:" (autotest): per subgroup state class (0 forming/other < 2000, 1 march 2000s, 2 fight 3000s, 3 other)
    // its members, stragglers at home, out ahead of / behind the middle (toward the base); fronts (out subgroups 20+
    // tiles apart), samples with 2+ out; the subgroups' orders (vf57) per class: moves, attacks
    unsigned dm[4], dsh[4], dsa[4], dsb[4], fronts, multi, omv[4], oat[4];
    // TETHER / REGROUP (16 / 32): the subgroups out and their members' middle (every 2 s); targets skipped, regroup orders
    struct { uint8_t *sg; int cx, cy; } tc[MS_SG]; int ntc;
    unsigned tethered, regrouped, joined;
} g_mss[16];
internal void mass_new_game(void) { memset(g_mss, 0, sizeof g_mss); }

internal int ms_land_soldier(ee_unit *u, ee_unit_type *td) {
    int fam = type_get_family(td);
    return unit_get_hp(u) > 0 && unit_get_x(u) >= 0 && is_soldier(td) && fam != 3 && fam != 4 && fam != 7 &&
           fam != 10 && fam != 17 && fam != 18 && fam != 21 && fam != 22;
}
internal int ms_base(ee_player *pl, int *bx, int *by) {   // an AI's (a human player's +0xB90 is no computer manager: garbage)
    int i = pl ? player_get_index(pl) & 15 : 0;
    if (!pl || !(g_players[i] || is_vanilla_idx(i))) return 0;
    ee_civ_mgr *cm = player_get_civ_mgr(pl); ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
    if (!bm || bm_get_base_x(bm) == -10000) return 0;
    *bx = bm_get_base_x(bm); *by = bm_get_base_y(bm);
    return 1;
}
internal int ms_d2(int x0, int y0, int x1, int y1) { return (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0); }
// own land soldiers (pl's) within r tiles of (x, y), not counting u; enemy soldiers of pl there (en)
internal int ms_near(ee_player *pl, ee_unit *u, int x, int y, int r, int *en) {
    int n = 0;
    if (en) *en = 0;
    for (struct grid_it it = grid_it_box(x, y, r, 1); grid_it_next(&it); ) {
        if (it.u == u || !ms_land_soldier(it.u, it.td) || ms_d2(unit_get_x(it.u), unit_get_y(it.u), x, y) > r * r) continue;
        ee_player *o = unit_get_owner(it.u);
        if (o == pl) n++;
        else if (en && o && player_get_index(o) > 0 && is_enemy(pl, o)) (*en)++;
    }
    return n;
}
// the land attack subgroups of pl (attack commander player +0xB8C +0x48, groups' subgroup vectors +8..+0xC)
internal struct ms_sg { uint8_t *sg; int n, na, cx, cy, state, out; } g_msg[MS_SG];
internal int ms_subgroups(ee_player *pl, int bx, int by) {
    int ns = 0;
    uint8_t *cbm = player_get_combat_mgr(pl), *cmd = cbm ? FIELD(cbm, OF(AM, 0x48), uint8_t *) : NULL;
    if (!cmd) return 0;
    uint8_t *dq = cmd + 4;
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        uint8_t *g = deque_at(dq, first + k);
        for (uint8_t **sp = g ? FIELD(g, OF(AG, 0x8), uint8_t **) : NULL; sp && sp < FIELD(g, OF(AG, 0xC), uint8_t **) && ns < MS_SG; sp++) {
            uint8_t *sg = *sp;
            if (!sg || !asg_is_land(sg)) continue;
            struct ms_sg *s = &g_msg[ns++];
            float x = 0, y = 0, ax = 0, ay = 0;
            s->sg = sg; s->n = s->na = 0; s->state = FIELD(sg, OF(ASG, 0x1D0), int);
            for (uint8_t **r = FIELD(sg, OF(ASG, 0x54), uint8_t **); r && r < FIELD(sg, OF(ASG, 0x58), uint8_t **); r++) {
                ee_unit *u = *r ? FIELD(*r, OF(ASGU, 0x44), ee_unit *) : NULL;
                if (!u || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
                int ux = unit_get_x(u), uy = unit_get_y(u);
                x += (float)ux; y += (float)uy; s->n++;
                if (ms_d2(ux, uy, bx, by) > 25 * 25) { ax += (float)ux; ay += (float)uy; s->na++; }
            }
            s->cx = s->n ? (int)(x / (float)s->n) : bx; s->cy = s->n ? (int)(y / (float)s->n) : by;
            // out: past forming (0 next goal, 1000 empty, 1001 starting, 1004 forming) with its members away 30+ tiles
            // from the base - or none away any more while it marches or fights (2000+, not a transport's 8000)
            int ox = s->na ? (int)(ax / (float)s->na) : s->cx, oy = s->na ? (int)(ay / (float)s->na) : s->cy;
            s->out = s->state != 0 && s->state != 1000 && s->state != 1001 && s->state != 1004 && s->state != 8000 &&
                     (s->na ? ms_d2(ox, oy, bx, by) > 30 * 30 : s->state >= 2000);
            if (s->na) { s->cx = ox; s->cy = oy; }
        }
    }
    return ns;
}

// ---- the measurement ----
internal int ms_cls(int st) { return st >= 2000 && st < 3000 ? 1 : st >= 3000 && st < 4000 ? 2 : st >= 4000 ? 3 : 0; }
internal void mass_sample(ee_player *pl, struct mass *m, int bx, int by) {
    m->samples++;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || !ms_land_soldier(u, it.td)) continue;
        int x = unit_get_x(u), y = unit_get_y(u);
        int iso = ms_near(pl, u, x, y, 6, NULL) <= 2, away = ms_d2(x, y, bx, by) > 25 * 25, eng = fl_attacking(u);
        m->sold++; m->iso += iso; m->away += away; m->isoaway += iso && away; m->eng += eng; m->engiso += eng && iso;
    }
    int ns = ms_subgroups(pl, bx, by);
    for (int k = 0; k < ns; k++) {
        struct ms_sg *s = &g_msg[k];
        m->sg++; m->out += s->out; m->asg += (unsigned)s->n;
        int cx = 0, cy = 0, n = 0;   // stragglers: from the middle of all its members
        for (uint8_t **r = FIELD(s->sg, OF(ASG, 0x54), uint8_t **); r && r < FIELD(s->sg, OF(ASG, 0x58), uint8_t **); r++) {
            ee_unit *u = *r ? FIELD(*r, OF(ASGU, 0x44), ee_unit *) : NULL;
            if (u && unit_get_hp(u) > 0 && unit_get_x(u) >= 0) { cx += unit_get_x(u); cy += unit_get_y(u); n++; }
        }
        if (n < 2) continue;
        cx /= n; cy /= n;
        int c = ms_cls(s->state);
        for (uint8_t **r = FIELD(s->sg, OF(ASG, 0x54), uint8_t **); r && r < FIELD(s->sg, OF(ASG, 0x58), uint8_t **); r++) {
            ee_unit *u = *r ? FIELD(*r, OF(ASGU, 0x44), ee_unit *) : NULL;
            if (!u || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
            int ux = unit_get_x(u), uy = unit_get_y(u), st = ms_d2(ux, uy, cx, cy) > 12 * 12;
            m->strag += st; m->dm[c]++;
            if (!st) continue;
            if (ms_d2(ux, uy, bx, by) <= 25 * 25) m->dsh[c]++;
            else if ((ux - cx) * (cx - bx) + (uy - cy) * (cy - by) > 0) m->dsa[c]++;
            else m->dsb[c]++;
        }
    }
    int no = 0, fr = 0;   // fronts: out subgroups, a new front when 20+ tiles from every earlier one
    for (int k = 0; k < ns; k++) {
        if (!g_msg[k].out) continue;
        int cl = 0;
        for (int j = 0; j < k && !cl; j++) cl = g_msg[j].out && ms_d2(g_msg[j].cx, g_msg[j].cy, g_msg[k].cx, g_msg[k].cy) <= 20 * 20;
        no++; fr += !cl;
    }
    m->fronts += (unsigned)fr; m->multi += no >= 2;
}
// a unit died (diplo.c hook_kill_stat, n 3; autotest): a land soldier - who stood by it
internal void mass_death(ee_unit *dead) {
    if (!g_autotest || !dead) return;
    ee_unit_type *td = unit_get_type_data(dead);
    ee_player *pl = unit_get_owner(dead);
    int bx, by, fam = td ? type_get_family(td) : 3;
    if (!pl || !td || unit_get_x(dead) < 0 || !is_soldier(td) || fam == 3 || fam == 4 || fam == 7 || fam == 10 ||
        fam == 17 || fam == 18 || fam == 21 || fam == 22 || !ms_base(pl, &bx, &by)) return;
    struct mass *m = &g_mss[(unsigned)player_get_index(pl) & 15];
    int x = unit_get_x(dead), y = unit_get_y(dead), en, own = ms_near(pl, dead, x, y, 6, NULL);
    ms_near(pl, dead, x, y, 8, &en);
    int iso = own <= 2, away = ms_d2(x, y, bx, by) > 25 * 25;
    m->die++; m->die_iso += iso; m->die_away += away; m->die_isoaway += iso && away; m->die_outn += en >= 2 * (own + 1);
    int ns = ms_subgroups(pl, bx, by), w = 4;
    for (int k = 0; k < ns && w == 4; k++) {
        struct ms_sg *s = &g_msg[k];
        if (!s->out) continue;
        for (uint8_t **r = FIELD(s->sg, OF(ASG, 0x54), uint8_t **); r && r < FIELD(s->sg, OF(ASG, 0x58), uint8_t **) && w == 4; r++)
            if (*r && FIELD(*r, OF(ASGU, 0x44), ee_unit *) == dead)
                w = ms_d2(x, y, s->cx, s->cy) <= 12 * 12 ? 0 : !away ? 3 : (x - s->cx) * (s->cx - bx) + (y - s->cy) * (s->cy - by) > 0 ? 1 : 2;
    }
    m->dsg[w]++;
}

// ---- army_mass ----
// GATHER: the rally (every 20 s): the own finished tower within 14 tiles of the base nearest the point 6 tiles out
// toward (tx, ty), 2 tiles on the base's side of it; else that point
internal void ms_rally(ee_player *pl, struct mass *m, int bx, int by, int tx, int ty) {
    float dx = (float)(tx - bx), dy = (float)(ty - by), l = sqrtf(dx * dx + dy * dy);
    if (l < 1) { dx = 1; dy = 0; l = 1; }
    int px = bx + (int)(6.0f * dx / l), py = by + (int)(6.0f * dy / l), bd = 0x7FFFFFFF;
    m->rx = px; m->ry = py;
    for (struct grid_it it = grid_it_box(bx, by, 14, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit *bs = unit_get_physics(u);
        float r, a;
        int x = unit_get_x(u), y = unit_get_y(u), d = ms_d2(x, y, px, py);
        if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || !bs || !unit_get_finished(bs) || !defence_stats(u, &r, &a) ||
            ms_d2(x, y, bx, by) > 14 * 14 || d >= bd) continue;
        float ex = (float)(bx - x), ey = (float)(by - y), el = sqrtf(ex * ex + ey * ey);
        bd = d;
        m->rx = x + (el >= 1 ? (int)(2.0f * ex / el) : 0); m->ry = y + (el >= 1 ? (int)(2.0f * ey / el) : 0);
    }
}
internal int ms_held(struct mass *m, ee_unit *u, uint8_t *sg) {
    for (int k = 0; k < m->nh; k++) if (m->hold[k].u == u && (!sg || m->hold[k].sg == sg)) return k + 1;
    return 0;
}
// landing.c hook_asg_give_goal: a land subgroup's order to a member held at the rally
internal int mass_refuses(uint8_t *sg, ee_unit *u, void *goal) {
    ee_player *pl = FIELD(sg, OF(ASG, 0x190), void *);
    if (!pl || !asg_is_land(sg)) return 0;
    struct mass *m = &g_mss[(unsigned)player_get_index(pl) & 15];
    if (g_autotest) {
        int c = ms_cls(FIELD(sg, OF(ASG, 0x1D0), int));
        if (goal && goal_is_attack(goal)) m->oat[c]++; else m->omv[c]++;
    }
    if (!m->nh || !ms_held(m, u, sg)) return 0;
    m->refused++;
    return 1;
}
internal void ms_gather(ee_player *pl, unsigned idx, struct mass *m, uint32_t now, int bx, int by) {
    int ns = ms_subgroups(pl, bx, by);
    static uint8_t keep[MS_HOLD];
    memset(keep, 0, sizeof keep);
    int tx = bx, ty = by, best = -1;
    for (int k = 0; k < ns; k++) {
        struct ms_sg *s = &g_msg[k];
        if (!s->out || ((ARMY_MASS(idx) & 8) && s->state != 3000 && s->state != 3003)) continue;   // 8: only while it fights
        if (best < 0 || ms_d2(s->cx, s->cy, bx, by) < ms_d2(tx, ty, bx, by)) { best = k; tx = s->cx; ty = s->cy; }
        for (uint8_t **r = FIELD(s->sg, OF(ASG, 0x54), uint8_t **); r && r < FIELD(s->sg, OF(ASG, 0x58), uint8_t **); r++) {
            ee_unit *u = *r ? FIELD(*r, OF(ASGU, 0x44), ee_unit *) : NULL;
            if (!u || !ms_land_soldier(u, unit_get_type_data(u)) || ms_d2(unit_get_x(u), unit_get_y(u), bx, by) > 25 * 25 ||
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
    if (!m->tr || now < m->tr || now - m->tr >= 20000) { m->tr = now ? now : 1; ms_rally(pl, m, bx, by, tx, ty); }
    for (int k = 0; k < m->nh; k++) {   // release a subgroup's held members together
        uint8_t *sg = m->hold[k].sg;
        int n = 0;
        uint32_t oldest = 0;
        for (int j = 0; j < m->nh; j++)
            if (m->hold[j].sg == sg) { n++; if (now - m->hold[j].since > oldest) oldest = now - m->hold[j].since; }
        if (n >= g_mass_group || (n >= 3 && oldest >= (uint32_t)g_mass_hold * 1000u) || oldest >= (uint32_t)g_mass_hold * 2000u) {
            if (g_log && g_autotest) fprintf(g_log, "# mass: P%u at %.1f min: %d reinforcements go together (held %.1f min)\n", idx, now / 60000.0, n, oldest / 60000.0);
            int jx = -1, jy = -1;   // 64 JOIN: they walk to the subgroup's pack (its members away from the base)
            for (int i = 0; i < ns && (ARMY_MASS(idx) & 64); i++)
                if (g_msg[i].sg == sg && g_msg[i].na >= 2) { jx = g_msg[i].cx; jy = g_msg[i].cy; }
            for (int j = m->nh - 1; j >= 0; j--) if (m->hold[j].sg == sg) {
                if (jx >= 0) m->joined += unit_goal(m->hold[j].u, goal_move((float)jx + 0.5f + (float)(j % 3) - 1.0f, (float)jy + 0.5f + (float)(j / 3 % 3) - 1.0f), 0, 0x1F40) != 0;
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
        int x = unit_get_x(u), y = unit_get_y(u);
        if (ms_d2(x, y, m->rx, m->ry) <= 9 || as_goal_near(u, (float)m->rx + 0.5f, (float)m->ry + 0.5f, 2.5f)) continue;
        if (t && fl_attacking(u) && ms_d2(unit_get_x(t), unit_get_y(t), m->rx, m->ry) <= 15 * 15) continue;
        unit_goal(u, goal_move((float)m->rx + 0.5f + (float)(k % 3) - 1.0f, (float)m->ry + 0.5f + (float)(k / 3 % 3) - 1.0f), 0, 0x1F40);
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
internal void ms_tether(ee_player *pl, struct mass *m, int plan, int bx, int by) {
    int ns = ms_subgroups(pl, bx, by), t = g_mass_tether + 4;
    m->ntc = 0;
    for (int k = 0; k < ns; k++) {
        struct ms_sg *s = &g_msg[k];
        if (!s->out || s->na < 2) continue;
        m->tc[m->ntc].sg = s->sg; m->tc[m->ntc].cx = s->cx; m->tc[m->ntc].cy = s->cy; m->ntc++;
        if (!(plan & 32) || s->state < 3000 || s->state >= 4000) continue;
        for (uint8_t **r = FIELD(s->sg, OF(ASG, 0x54), uint8_t **); r && r < FIELD(s->sg, OF(ASG, 0x58), uint8_t **); r++) {
            ee_unit *u = *r ? FIELD(*r, OF(ASGU, 0x44), ee_unit *) : NULL;
            if (!u || !ms_land_soldier(u, unit_get_type_data(u)) || fl_attacking(u)) continue;
            int x = unit_get_x(u), y = unit_get_y(u);
            if (ms_d2(x, y, bx, by) <= 25 * 25 || ms_d2(x, y, s->cx, s->cy) < t * t ||
                as_goal_near(u, (float)s->cx + 0.5f, (float)s->cy + 0.5f, 4.0f)) continue;
            m->regrouped += unit_goal(u, goal_move((float)s->cx + 0.5f, (float)s->cy + 0.5f), 0, 0x1F40) != 0;
        }
    }
}
// attack.c hook_asgu_score: a target too far from the pack of an out subgroup's member
internal int mass_tether(uint8_t *sg, ee_unit *me, ee_unit *enemy) {
    ee_player *pl = FIELD(sg, OF(ASG, 0x190), void *);
    if (!pl || !me || !enemy) return 0;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct mass *m = &g_mss[idx];
    int plan = ARMY_MASS(idx);
    if (!m->ntc || !(plan & 16)) return 0;
    ee_unit_type *etd = unit_get_type_data(enemy);   // 128: buildings only - enemy units are fought wherever they are
    if ((plan & 128) && (!etd || (type_get_family(etd) != 2 && type_get_family(etd) != 29))) return 0;
    int k = 0;
    while (k < m->ntc && m->tc[k].sg != sg) k++;
    if (k == m->ntc) return 0;
    int ex = unit_get_x(enemy), ey = unit_get_y(enemy);
    if (ms_d2(ex, ey, m->tc[k].cx, m->tc[k].cy) <= g_mass_tether * g_mass_tether) return 0;
    uint8_t *st = unit_get_stats(me); ee_unit_type *td = unit_get_type_data(me);
    float r = (st ? FIELD(st, OF(USTAT, 0x9C), float) : td ? type_get_range(td) : 1.0f) + 2.0f;
    int dx = ex - unit_get_x(me), dy = ey - unit_get_y(me);
    if ((float)(dx * dx + dy * dy) <= r * r) return 0;
    m->tethered++;
    return 1;
}
internal int mass_retreat(unsigned idx) { return (ARMY_MASS(idx) & 4) != 0; }   // defence.c attack_retreat
internal void mass_tick(ee_player *pl, uint32_t now) {
    if (!pl) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct mass *m = &g_mss[idx];
    int bx, by;
    if (now < m->ts || now < m->tf) { memset(m, 0, sizeof *m); }
    if (!ms_base(pl, &bx, &by)) return;
    int plan = is_vanilla(pl) ? 0 : ARMY_MASS(idx);
    if (plan && (!m->tf || now - m->tf >= 2000)) {
        m->tf = now ? now : 1;
        if (!navy_land(idx)) m->nh = m->ntc = 0;
        else {
            if (plan & 1) ms_gather(pl, idx, m, now, bx, by); else m->nh = 0;
            if (plan & 48) ms_tether(pl, m, plan, bx, by); else m->ntc = 0;
            if ((plan & 2) && (!m->tw || now - m->tw >= 5000)) { m->tw = now ? now : 1; ms_wait(pl, idx, m, now); }
        }
    }
    if (!g_autotest || !g_log) return;
    if (!m->ts || now - m->ts >= 5000) { m->ts = now ? now : 1; mass_sample(pl, m, bx, by); }
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
internal void mass_install(const char *aimod_ini, const char *autotest_ini) {
    g_army_mass = cfg_int(aimod_ini, autotest_ini, 1, "army_mass", 0);
    g_mass_group = cfg_int(aimod_ini, autotest_ini, 1, "army_mass_group", 5);
    g_mass_hold = cfg_int(aimod_ini, autotest_ini, 1, "army_mass_hold", 120);
    g_mass_ratio = cfg_pct(aimod_ini, autotest_ini, 1, "army_mass_ratio", 130, 1000);
    g_mass_wait = cfg_int(aimod_ini, autotest_ini, 1, "army_mass_wait", 10);
    g_mass_tether = cfg_int(aimod_ini, autotest_ini, 1, "army_mass_tether", 12);
    if (autotest_ini) g_v_army_mass = ini_int("autotest", "v_army_mass", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# army mass: %d (variant %d), group %d, hold %d s, ratio %d%%, wait %d min, tether %d\n", g_army_mass, g_v_army_mass,
                       g_mass_group, g_mass_hold, g_mass_ratio, g_mass_wait, g_mass_tether);
}
