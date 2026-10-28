// THE STAGING POINT. Included by eeaimod.c after cover.c (cv_danger, cv_mil, cv_alive, cv_built, cv_held), mass.c
// (ms_base, ms_subgroups, ms_land_soldier, ms_held), stats.c (st_ratio), attack.c (seen_by, defence_stats, unit_goal,
// goal_move) and forces.c (area_id).
// The player: an AI must keep building its army and mass it outside direct danger until it has a real chance of
// pushing back - not feed it into the enemy's army. What the game does instead (measured on the player's save
// "standing army", 2026-10-26): an attack on a player sends each land subgroup on a tour of its own - its legs the
// player's buildings, a wait of 1-3 minutes at each - so P6's 136 soldiers stood on 10-14 fronts at once (subgroups 20+
// tiles apart), 3-15 of them fighting, and P7's 68 were in 45 subgroups; they reached the player's army a few at a time,
// broke off from deep inside its reach (30-40 tiles from their cover) and half of their losses died on the way back.
// fall_back, army_mass and avoid_lost_fights act on single units and none of them gathers the army.
//
// army_stage ([ai_base], default 1 since 2026-10-26; [autotest] v_army_stage for the variant players; with danger_routes): a land
// subgroup's march (vf57, landing.c hook_asg_give_goal) from safety to a point in danger - in reach of the enemies the
// AI knows (game/danger.c DANGER_THREATS) or in a lost fight (DANGER_LOST) - goes to the AI's one staging point
// instead. The front is the 8-tile cell its marches were held at most in the last 2 minutes; the stage the first tile
// out of every danger on the base's land on the way from there back to the base, 3 tiles further (placed again every
// 10 s, and at once when danger reaches it; none when danger reaches the base - the defence's). The army gathers
// there, its members picking no target beyond their own range + 2 (attack.c hook_asgu_score). A march goes on when
// its subgroup with the fighters near its destination already beats the enemy there by army_stage_go (130%; stats.c
// st_ratio: the enemy fighters within 20 tiles - seen, and remembered with fight_memory -, the defences covering it).
// Every 4 s the army at the stage is judged against the front the same way; once it wins, every member at the stage
// goes to the front together (the push; for 90 s the marches near it pass). Marches to points out of danger (an
// undefended building) and those of units in danger already (a fight being won) pass as before; lost fights are left
// to cover_hold_leg.
// Measured (docs/testing.md army_stage; with fight_memory): head-to-head (24 games) razed +5.1 (se 2.7), soldiers +4.9
// (2.9), kills +10.5 (9.6), losses -0.3, workers +8.7 (7.6); on the player's save as memory alone (77 vs 79 lost).
// "# stage:" once a game minute (the mod's AIs with it): the stage and its front, marches held / passed, pushes and
// the members they sent, targets skipped at the stage, members at the stage, the last judgement.
internal int g_army_stage = 1, g_v_army_stage = -1, g_stage_go = 130;
#define ARMY_STAGE(idx) PV(idx, g_v_army_stage, g_army_stage)
#define SG_FRONT 16
#define SG_R 14       // the stage's radius: who counts there, who waits
#define SG_FOE 20     // the enemy counted around a destination (a lost fight reaches 10 + 15 from its cell's middle)
#define SG_NEAR 12    // own fighters there already
#define SG_PUSH 90000
internal struct stage {
    int on, started; unsigned min;
    ivec2 s, front;                                         // the staging point, the front it was placed for
    uint32_t ts, tt, tj;                                    // placed; the last tick; the last judgement of the front
    struct { ivec2 c, d; uint32_t t; unsigned n; } f[SG_FRONT];   // held marches by 8-tile cell: the last destination, when, how many
    ivec2 push; uint32_t push_t;                            // the push: its destination, since (0 none)
    ee_attack_subgroup *jsg; ivec2 jc; uint32_t jt; int jgo; // the last judgement of a march: its subgroup, its destination's cell, when, go
    float ratio;
    int at;                                                 // its soldiers at the stage (the last tick)
    unsigned held, passed, pushes, sent, skips;             // since the last line
} g_stg[16];
internal struct st_side g_sgs_own, g_sgs_foe;
internal void wave_focus_new_game(void);
internal void stage_new_game(void) { memset(g_stg, 0, sizeof g_stg); wave_focus_new_game(); }

internal int stage_danger(unsigned idx, ivec2 p) { return cv_danger(idx, DANGER_THREATS, p) || cv_danger(idx, DANGER_LOST, p); }
internal int stage_on(ee_player *pl, unsigned idx) { return g_danger_routes && pl && !is_vanilla(pl) && ARMY_STAGE(idx) && navy_land(idx); }
internal int stage_pushing(struct stage *st, uint32_t now) { return st->push_t && now >= st->push_t && now - st->push_t < SG_PUSH; }

// a march held toward d: counted for the front
internal void stage_note(struct stage *st, ivec2 d, uint32_t now) {
    ivec2 c = iv2(d.x / 8, d.y / 8);
    int k = 0, old = 0;
    while (k < SG_FRONT && !(st->f[k].n && st->f[k].c.x == c.x && st->f[k].c.y == c.y)) k++;
    if (k == SG_FRONT) {   // a free record, else the oldest
        for (k = 0; k < SG_FRONT && st->f[k].n; k++) if (st->f[k].t < st->f[old].t) old = k;
        if (k == SG_FRONT) k = old;
        st->f[k].c = c; st->f[k].n = 0;
    }
    if (st->f[k].n && now - st->f[k].t > 120000) st->f[k].n = 0;
    st->f[k].d = d; st->f[k].t = now ? now : 1; st->f[k].n++;
}
// the stage: from the front back toward the base, the first tile on the base's land out of every danger, 3 further
internal void stage_place(ee_player *pl, unsigned idx, struct stage *st, uint32_t now) {
    int best = -1;
    ivec2 b;
    st->ts = now ? now : 1;
    for (int k = 0; k < SG_FRONT; k++)
        if (st->f[k].n && now - st->f[k].t <= 120000 && (best < 0 || st->f[k].n > st->f[best].n)) best = k;
    unsigned a = 0;
    if (best < 0 || !ms_base(pl, &b) || !(a = area_id(b, 2))) { st->on = 0; return; }
    ivec2 l = st->f[best].d;
    float dx = (float)(b.x - l.x), dy = (float)(b.y - l.y), len = sqrtf(dx * dx + dy * dy);
    st->on = 0;
    for (float s = 0; s <= len; s += 1.0f) {
        ivec2 q = iv2(l.x + (int)lroundf(dx * s / len), l.y + (int)lroundf(dy * s / len));
        if (area_id(q, 2) != a || stage_danger(idx, q)) continue;
        st->s = q; st->front = l; st->on = 1;
        for (float e = 1; e <= 3 && s + e <= len; e += 1.0f) {
            ivec2 r = iv2(l.x + (int)lroundf(dx * (s + e) / len), l.y + (int)lroundf(dy * (s + e) / len));
            if (area_id(r, 2) != a || stage_danger(idx, r)) break;
            st->s = r;
        }
        return;
    }
}
// the enemy at d as pl knows it: the fighters it sees within SG_FOE tiles, those it remembers there (cv_foe_mem), the
// defences covering d it has seen (seen_by: remembered ones too)
internal void stage_foe(ee_player *pl, ivec2 d) {
    st_side_clear(&g_sgs_foe);
    for (struct grid_it it = grid_it_box(d.x, d.y, SG_FOE, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !cv_alive(u) || !is_enemy(pl, o) || !seen_by(u, pl)) continue;
        int fam = unit_type_get_class_id(it.td), d2 = ivec2_dist2(unit_get_tile(u), d);
        float rg, at;
        if (fam == CLS_BUILDING || fam == CLS_FORTRESS) {
            if (!defence_stats(u, &rg, &at) || at <= 0 || !cv_built(u) || (float)d2 > (rg + 1.0f) * (rg + 1.0f)) continue;
        } else if (!cv_mil(it.td) || d2 > SG_FOE * SG_FOE) continue;
        st_side_add(&g_sgs_foe, u);
    }
    cv_foe_mem(pl, &g_sgs_foe, d, SG_FOE, 0, NULL);
}
// the AI's and its allies' land fighters within r of c into the own side, not those within xr of x (xr 0: none left out)
internal void stage_own(ee_player *pl, ivec2 c, int r, ivec2 x, int xr) {
    for (struct grid_it it = grid_it_box(c.x, c.y, r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        ivec2 p = unit_get_tile(u);
        if (!o || player_get_index(o) <= 0 || !cv_alive(u) || is_enemy(pl, o) || cv_mil(it.td) != 1 || ivec2_dist2(p, c) > r * r ||
            (xr && ivec2_dist2(p, x) <= xr * xr)) continue;
        st_side_add(&g_sgs_own, u);
    }
}
// a march's own judgement: the subgroup's members and the fighters near d already against the enemy at d
internal float stage_judge_leg(ee_player *pl, ee_attack_subgroup *sg, ivec2 d) {
    stage_foe(pl, d);
    st_side_clear(&g_sgs_own);
    stage_own(pl, d, SG_NEAR, d, 0);
    for (ee_asg_unit **r = attack_subgroup_get_units_begin(sg); r && r < attack_subgroup_get_units_end(sg); r++) {
        ee_unit *u = *r ? asg_unit_get_unit(*r) : NULL;
        if (u && cv_alive(u) && cv_mil(unit_get_type_data(u)) == 1 && ivec2_dist2(unit_get_tile(u), d) > SG_NEAR * SG_NEAR) st_side_add(&g_sgs_own, u);
    }
    return st_ratio(&g_sgs_own, &g_sgs_foe);
}
// the army's judgement: the fighters at the stage and those near the front already against the enemy there
internal float stage_judge_front(ee_player *pl, struct stage *st) {
    stage_foe(pl, st->front);
    st_side_clear(&g_sgs_own);
    stage_own(pl, st->s, SG_R, st->s, 0);
    stage_own(pl, st->front, SG_NEAR, st->s, SG_R);
    return st_ratio(&g_sgs_own, &g_sgs_foe);
}
// the push: every member of a land subgroup at the stage (not held by army_mass or fall_back) goes to the front
internal void stage_push(ee_player *pl, unsigned idx, struct stage *st, uint32_t now) {
    ivec2 b, d = st->front;
    st->push = d; st->push_t = now ? now : 1; st->pushes++;
    if (!ms_base(pl, &b)) return;
    int ns = ms_subgroups(pl, b), n = 0;
    for (int k = 0; k < ns; k++)
        for (ee_asg_unit **r = attack_subgroup_get_units_begin(g_msg[k].sg); r && r < attack_subgroup_get_units_end(g_msg[k].sg); r++) {
            ee_unit *u = *r ? asg_unit_get_unit(*r) : NULL;
            if (!u || !ms_land_soldier(u, unit_get_type_data(u)) || ivec2_dist2(unit_get_tile(u), st->s) > SG_R * SG_R ||
                cv_held(&g_cv[idx], u) || ms_held(&g_mss[idx], u, NULL)) continue;
            st->sent += unit_goal(u, goal_move((float)d.x + 0.5f + (float)(n % 3) - 1.0f, (float)d.y + 0.5f + (float)(n / 3 % 3) - 1.0f), 0, 0x1F40) != 0;
            n++;
        }
    if (g_log && g_autotest) fprintf(g_log, "# stage: P%u at %.1f min: the army goes from %d,%d to %d,%d (%.2f) - %d members\n", idx, now / 60000.0,
                                     st->s.x, st->s.y, d.x, d.y, st->ratio, n);
}
// landing.c hook_asg_give_goal: a land subgroup's march from safety into danger - on when it wins there by itself (or
// with the army's push), else to the stage; 1 when it decided (cover_hold_leg not asked)
internal int stage_leg(ee_attack_subgroup *sg, ee_unit *u, ee_goal *goal) {
    ee_player *pl = attack_subgroup_get_player(sg);
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    if (!u || !goal || !goal_is_location(goal) || !attack_subgroup_is_land(sg) || !stage_on(pl, idx) || ou_sortie_sg(sg)) return 0;   // army.c: outdated units go straight in
    struct stage *st = &g_stg[idx];
    uint32_t now = game_time();
    ivec2 d = iv2((int)goal_get_dest_x(goal), (int)goal_get_dest_y(goal)), p = unit_get_tile(u);
    if (!stage_danger(idx, d) || stage_danger(idx, p) || landing_has_unit(pl, u)) return 0;
    if (stage_pushing(st, now) && ivec2_dist2(d, st->push) <= 20 * 20) { st->passed++; return 1; }
    if (st->jsg != sg || !st->jt || now - st->jt >= 2000 || st->jc.x != d.x / 8 || st->jc.y != d.y / 8) {   // once per subgroup and cell (2 s)
        float r = stage_judge_leg(pl, sg, d);
        st->jsg = sg; st->jc = iv2(d.x / 8, d.y / 8); st->jt = now ? now : 1; st->jgo = r * 100.0f >= (float)g_stage_go;
    }
    if (st->jgo) { st->passed++; return 1; }
    stage_note(st, d, now);
    if (!st->on || now - st->ts >= 10000 || stage_danger(idx, st->s)) stage_place(pl, idx, st, now);
    if (!st->on) return 0;
    int k = unit_get_id(u);
    goal_set_dest_x(goal, (float)st->s.x + 0.5f + (float)(k % 3) - 1.0f);
    goal_set_dest_y(goal, (float)st->s.y + 0.5f + (float)(k / 3 % 3) - 1.0f);
    st->held++;
    return 1;
}
// attack.c hook_asgu_score: a member at the stage waits - no target beyond its own range + 2 (no push on)
internal int stage_skip(ee_attack_subgroup *sg, ee_unit *me, ee_unit *enemy) {
    ee_player *pl = attack_subgroup_get_player(sg);
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    if (!me || !enemy || !stage_on(pl, idx)) return 0;
    struct stage *st = &g_stg[idx];
    ivec2 p = unit_get_tile(me);
    if (!st->on || stage_pushing(st, game_time()) || ivec2_dist2(p, st->s) > SG_R * SG_R) return 0;
    ee_unit_stats *stt = unit_get_stats(me); ee_unit_type *td = unit_get_type_data(me);
    float r = (stt ? unit_stats_get_range(stt) : td ? unit_type_get_range(td) : 1.0f) + 2.0f;
    if ((float)ivec2_dist2(unit_get_tile(enemy), p) <= r * r) return 0;
    st->skips++;
    return 1;
}
internal void stage_tick(ee_player *pl, uint32_t now) {
    if (!pl) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct stage *st = &g_stg[idx];
    if (now < st->tt) memset(st, 0, sizeof *st);   // a new game
    if (!stage_on(pl, idx) || (st->tt && now - st->tt < 2000)) return;
    st->tt = now ? now : 1;
    int any = 0;
    for (int k = 0; k < SG_FRONT; k++) any |= st->f[k].n && now - st->f[k].t <= 120000;
    if (!any) st->on = 0;
    else if (now - st->ts >= 10000 || (st->on && stage_danger(idx, st->s))) stage_place(pl, idx, st, now);
    int at = 0;
    ivec2 b;
    if (st->on && ms_base(pl, &b)) {
        int ns = ms_subgroups(pl, b);
        for (int k = 0; k < ns; k++)
            for (ee_asg_unit **r = attack_subgroup_get_units_begin(g_msg[k].sg); r && r < attack_subgroup_get_units_end(g_msg[k].sg); r++) {
                ee_unit *u = *r ? asg_unit_get_unit(*r) : NULL;
                at += u && ms_land_soldier(u, unit_get_type_data(u)) && ivec2_dist2(unit_get_tile(u), st->s) <= SG_R * SG_R;
            }
        st->at = at;
        if (at && !stage_pushing(st, now) && (!st->tj || now - st->tj >= 4000)) {   // the front itself, for those waiting without orders
            st->tj = now ? now : 1;
            st->ratio = stage_judge_front(pl, st);
            if (st->ratio * 100.0f >= (float)g_stage_go) stage_push(pl, idx, st, now);
        }
    }
    if (!g_log) return;
    if (!st->started) { st->started = 1; st->min = now / 60000; return; }
    if (now / 60000 == st->min) return;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# stage: P%u at %u min: %s %d,%d front %d,%d | held %u passed %u pushes %u sent %u skips %u | at the stage %d | judged %.2f\n",
            idx, st->min, st->on ? "at" : "none", st->s.x, st->s.y, st->front.x, st->front.y, st->held, st->passed, st->pushes, st->sent,
            st->skips, at, st->ratio > 99.0f ? 99.0f : st->ratio);
    LeaveCriticalSection(&g_lock);
    st->held = st->passed = st->pushes = st->sent = st->skips = 0;
    st->min = now / 60000;
}
// towns.c FORWARD BASE: the soldiers at the staging point, the point and its front (0: no stage)
internal int stage_army(unsigned idx, ivec2 *at, ivec2 *front) {
    struct stage *st = &g_stg[idx & 15];
    if (!st->on) return 0;
    *at = st->s; *front = st->front;
    return st->at;
}

// ONE TARGET A WAVE ([ai_base] wave_focus; [autotest] v_wave_focus; the player 2026-10-27 on the save "standing
// army": the AI looks very disorganized, every unit ordered around individually - attack groups are meant to make it
// better): an attack on a player (mission 1) has no target at its group, so the game splits the wave into land
// subgroups of about 10 and each walks legs of its own - home, coast points, the player's outlying buildings - and
// waits at each (docs/engine-notes.md). P6's wave of 250 on P4 at minute 652 was 34 subgroups with 34 goals from
// 25,218 to 415,200, 95% of them points with no enemy within 8 tiles, and the staging point (above) gathered only the
// marches that led into known danger. Now the wave has one goal: a building of the player it attacks (walls aside)
// that the AI has seen - the nearest to the subgroup asking first, after it falls the nearest to the last one, so the
// wave works through the town - and every route a land subgroup of the wave plans goes there (on the same land; not
// the routes home, mission 8). With none known the game's own legs stay (the search). The weak spot and the breach
// (attack.c hook_asg_route) still apply after it; a weak spot found for the wave's goal becomes the wave's. A wave's
// record lapses 3 minutes after its last route. "# wave focus:" once a game minute.
internal int g_wave_focus = 0, g_v_wave_focus = -1;
#define WAVE_FOCUS(idx) PV(idx, g_v_wave_focus, g_wave_focus)
#define WV_N 24
internal struct wv { ee_attack_group *g; ivec2 at; int id; uint32_t t; } g_wv[16][WV_N];
internal struct { unsigned calls, set, moved, kept, none; uint32_t logged; } g_wvs[16];
internal int wv_standing(ee_player *pl, ivec2 at, int id) {   // the wave's goal: the enemy building of that id on its tile
    for (struct grid_it it = grid_it_box(at.x, at.y, 1, 1); grid_it_next(&it); )
        if (it.td && unit_get_id(it.u) == id && unit_get_hp(it.u) > 0 && is_enemy(pl, unit_get_owner(it.u))) return 1;
    return 0;
}
struct wv_pick { ee_player *pl, *target; ivec2 from; short area; ee_unit *best; long bd; };
internal void wv_pick_unit(ee_unit *u, void *ctx) {
    struct wv_pick *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    if (!td || unit_get_owner(u) != s->target || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) return;
    int fam = unit_type_get_class_id(td);
    if ((fam != CLS_BUILDING && fam != CLS_FORTRESS) || is_wall(unit_type_get_id(td)) || !explored_by(u, s->pl)) return;
    if ((short)map_area(unit_get_tile(u), 2) != s->area) return;
    long d = ivec2_dist2(unit_get_tile(u), s->from);
    if (!s->best || d < s->bd) { s->best = u; s->bd = d; }
}
// attack.c hook_asg_route, before the weak spot: a land subgroup's route to dest - the wave's record (NULL: none)
internal struct wv *wave_focus(ee_player *pl, unsigned pi, ee_attack_subgroup *sg, int *start, int *dest) {
    if (!WAVE_FOCUS(pi) || !attack_subgroup_is_land(sg) || attack_subgroup_get_mission(sg) == 8) return NULL;
    ee_attack_group *g = attack_subgroup_get_group(sg);
    ee_player *target = attack_subgroup_get_target(sg);
    if (!g || !target || !is_enemy(pl, target)) return NULL;
    g_wvs[pi].calls++;
    uint32_t now = game_time();
    struct wv *w = NULL, *old = &g_wv[pi][0];
    for (int k = 0; k < WV_N && !w; k++) {
        struct wv *e = &g_wv[pi][k];
        if (e->g && now - e->t > 180000) e->g = NULL;   // lapsed (a group freed, its address used again)
        if (e->g == g) w = e;
        else if (!e->g || (old->g && e->t < old->t)) old = e;
    }
    short area = (short)map_area(iv2(dest[0], dest[1]), 2);
    if (!w || !wv_standing(pl, w->at, w->id) || (short)map_area(w->at, 2) != area) {   // a new goal
        struct wv_pick s = { pl, target, w ? w->at : iv2(start[0], start[1]), area, NULL, 0 };
        grid_each(wv_pick_unit, &s);
        if (!s.best) { g_wvs[pi].none++; return NULL; }
        if (!w) w = old;
        w->g = g; w->at = unit_get_tile(s.best); w->id = unit_get_id(s.best);
        g_wvs[pi].set++;
        if (g_log && now - g_wvs[pi].logged >= 60000) {
            g_wvs[pi].logged = now;
            fprintf(g_log, "# wave focus: P%u at %.1f min: group %p on P%d goes to type %d at %d,%d | land routes %u: moved %u, already "
                    "there %u, none known %u; goals %u\n", pi, now / 60000.0, (void *)g, player_get_index(target),
                    unit_type_get_id(unit_get_type_data(s.best)), w->at.x, w->at.y, g_wvs[pi].calls, g_wvs[pi].moved, g_wvs[pi].kept,
                    g_wvs[pi].none, g_wvs[pi].set);
        }
    }
    w->t = now;
    if (w->at.x == dest[0] && w->at.y == dest[1]) { g_wvs[pi].kept++; return w; }
    dest[0] = w->at.x; dest[1] = w->at.y;
    location_set_unit(attack_subgroup_target_rec_at(sg), dest[0], dest[1], w->id);
    g_wvs[pi].moved++;
    return w;
}
internal void wave_focus_weak(struct wv *w, ivec2 at, int id) { w->at = at; w->id = id; }
internal void wave_focus_new_game(void) { memset(g_wv, 0, sizeof g_wv); memset(g_wvs, 0, sizeof g_wvs); }

internal void stage_install(const char *autotest_ini) {
    g_army_stage = g_set_ai[1].army_stage;
    g_stage_go = g_set_ai[1].army_stage_go;
    g_wave_focus = g_set_ai[1].wave_focus;
    if (autotest_ini) g_v_army_stage = ini_int("autotest", "v_army_stage", -1, autotest_ini);
    if (autotest_ini) g_v_wave_focus = ini_int("autotest", "v_wave_focus", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# army stage: %d (variant %d), go at %d%%; wave focus %d (variant %d)\n", g_army_stage, g_v_army_stage,
                       g_stage_go, g_wave_focus, g_v_wave_focus);
}
