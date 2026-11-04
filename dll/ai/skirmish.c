// THE SKIRMISHERS ([autotest] v_skirmish; SK_ON, off until measured). The player, 2026-11-04, on his save "choke
// wolf" (a narrow valley his artillery and a few units at its end cover; the AI's force burned itself in it): bombers
// or a way round first, and if neither: "be tactical, not commit the whole army into it, sharpshooters are a good way
// to get close and pick off targets before the enemy can react, or even force the enemy to break formation to hunt
// the sharpshooters, which could give an opening to other units ... the ai's tactical capability to do damage with a
// few targeted units is very limited right now". Every SK_EVERY per AI with the mod's force (armies.c) at home:
// - WHEN: a position holds the force's way - the shooters' spot the air force bombs (airstrike.c: where land units hit
//   the AI's from AF_SHOOT_D tiles or more, the spot hit from most often) or the target the last attack time found
//   too strong (a->strong) -, within SK_REACH of home, SK_REST after the last party came home.
// - WHO: up to SK_N of the force's members at home that shoot SK_RANGE tiles or further (sharpshooters, snipers,
//   mortars, artillery ...), the longest first, with SK_FIT of their hit points; SK_MIN at least, else nobody goes.
//   They leave the force's list for the sortie (none of its orders reach them) and stay held the game's way (the
//   commander never gets them); home again they join the force.
// - THE SORTIE: an attack-move to the stand SK_STAND tiles short of the position on the line from home; on the way
//   and there each member not attacking already goes for a long-range shooter (artillery, siege, AF_SHOOT_D+ range)
//   seen or revealed (intel.c: hitting its units) within SK_HUNT - close in on what outranges it -, else an enemy
//   soldier it sees within its range + SK_PICK (no buildings: what a rifle does to a wall is nothing); one idle short
//   of the stand walks on. Nothing to shoot at the stand: it creeps SK_CREEP closer every SK_CREEP_MS, to
//   SK_STAND_MIN of the position. (The first try picked only for members without any order: the walk to the stand
//   is one, 0 targets picked in 26 sorties while the unseen artillery shelled them on the way.)
// - THE PULL-BACK: enemy fighters seen within SK_CHASE_R of the party's middle that it doesn't beat by SK_HOLD_PCT,
//   its hit points under SK_BLEED_PCT of what it went out with, or SK_MAX_MS out: home, a plain move.
// - THE BAIT: the party on its way home with chasers seen within SK_CHASE_R of it, and they within SK_AMBUSH of home:
//   the force at home goes at them as at a raid (armies.c ar_go, kind 2: home again when they are cleared) when it
//   beats them by AR_GO_PCT - the enemy broke formation to hunt the party, the force fights it on its own ground.
// THE SPOTTER (SK_ON bit 2; the player, same day: "sharpshooters are great scouts, if set to passive it's completely
// invisible to the enemy if it doesn't get too close, as well as providing a huge view radius"): while there is such
// a position, one sharpshooter or sniper of the force - one trained when the force has none, every SP_TRAIN_MS - leaves
// it, takes the game's Scout behaviour (dbunitbehavior.dat's first: it never attacks) and walks to the point SP_BACK
// inside its sight short of the position on the line from home, and stands there: what it sees, the bombers bomb
// (the give-ups on the save were unseen mortars at full hit points), the party picks and the force's judgement counts.
// The position gone, it gets its behaviour back and joins the force. Counter-fire leaves it alone (an attack order
// would give it away).
// "# armies: ... skirmish" lines; a summary a minute (autotest).
// Measured 2026-11-04 on the save "choke wolf" 153 -> 210 min (P3 the variant, 3 timings each, the air force on):
// since the air force's give-up (airstrike.c AF_RUN_MS) P3 wins that game at about 200 min whatever else is on - 4
// of 6 without either. The party (bit 1): 9-11 sorties a run, 28-49 targets picked, 40-59 of 72-85 members lost (the
// unseen artillery shells them on the way in); razed at 190 min 43, 67, 37 (51, 63, 37 before the picking was fixed)
// against 25-72 (51 on average) without, gold + iron lost in units 37-44k / 34-59k, won 6 of 6 at 198-208 min. The
// spotter (bit 2): a sniper (sight 13) trained at 157-182 min stood 10 tiles from the shooters for 3 to 36 minutes;
// the bombers' targets given up 3-39 a run against 15-61 without, but fewer bombers (its gold) - razed 27, 56, 81
// alone, 24, 33, 107 with the party, won 4 of 6 (one at 190.7). Level on all counts: off. The save no longer tells
// them apart; a position that holds needs a test of its own.

#ifndef SK_ON
#define SK_ON 0
#endif
#define SK_EVERY 3000
#define SK_N 8
#define SK_MIN 3
#define SK_RANGE 7.0f
#define SK_FIT 80
#define SK_REACH 80
#define SK_REST 30000
#define SK_STAND 10
#define SK_STAND_MIN 5
#define SK_CREEP 3
#define SK_CREEP_MS 15000
#define SK_PICK 2
#define SK_HUNT 14
#define SK_CHASE_R 10
#define SK_HOLD_PCT 150
#define SK_BLEED_PCT 50
#define SK_MAX_MS 240000
#define SK_HOME_MS 120000
#define SK_AMBUSH 25
#define SP_BACK 3
#define SP_TRAIN_MS 60000
#define SP_ORDER_MS 20000

internal int g_v_skirmish = -1;
internal struct sk_st {
    uint32_t t, log_t, out_t, creep_t, end_t;
    int state;                                  // 0 none, 1 out (to the stand, shooting), 2 home bound
    struct { ee_unit *u; int id; } m[SK_N]; int n;
    ivec2 pos, stand; float out_hp;             // the position, the stand, the party's hit points as it went
    unsigned sorties, picks, chased, bled, timed, baits, members, lost;   // sorties; targets given; pull-backs by chasers, by hit points, by time; the force at the chasers; members sent, lost
    ee_unit *sp; int sp_id; void *sp_beh; ivec2 sp_at; uint32_t sp_t, sp_train_t;   // the spotter, its behaviour before, its point, its last order, the last training
    unsigned spotters, sp_lost, sp_trained;
} g_sk[16];
internal struct st_side g_sk_own, g_sk_foe;

internal int sk_alive(struct sk_st *s, int k) {
    ee_unit *u = s->m[k].u;
    return u && unit_get_serial(u) == s->m[k].id && cv_alive(u);
}
// the party's middle and hit points (the live members into g_sk_own)
internal int sk_side(struct sk_st *s, ivec2 *mid) {
    long sx = 0, sy = 0;
    int n = 0;
    st_side_clear(&g_sk_own);
    for (int k = 0; k < s->n; k++) {
        if (!sk_alive(s, k)) continue;
        ivec2 p = unit_get_tile(s->m[k].u);
        sx += p.x; sy += p.y; n++;
        st_side_add(&g_sk_own, s->m[k].u);
    }
    if (mid && n) *mid = iv2((int)(sx / n), (int)(sy / n));
    return n;
}
// enemy land fighters pl sees within r of d into g_sk_foe; their middle
internal int sk_chasers(ee_player *pl, ivec2 d, int r, ivec2 *mid) {
    long sx = 0, sy = 0;
    st_side_clear(&g_sk_foe);
    for (struct grid_it it = grid_it_box(d.x, d.y, r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(u) || !ms_land_soldier(u, it.td) || !seen_by(u, pl)) continue;
        ivec2 p = unit_get_tile(u);
        if (ivec2_dist2(p, d) > r * r) continue;
        sx += p.x; sy += p.y;
        st_side_add(&g_sk_foe, u);
    }
    if (mid && g_sk_foe.n) *mid = iv2((int)(sx / g_sk_foe.n), (int)(sy / g_sk_foe.n));
    return g_sk_foe.n;
}
// the members back into the force (as soldiers at home: its gather walks them there)
internal void sk_disband(struct armies_st *a, struct sk_st *s) {
    for (int k = 0; k < s->n; k++) {
        if (!sk_alive(s, k)) { s->lost++; continue; }
        if (a->n >= AR_HELD) continue;
        a->held[a->n].u = s->m[k].u; a->held[a->n].id = s->m[k].id; a->held[a->n].order_t = 0; a->held[a->n].out = 0; a->held[a->n].back = 0; a->held[a->n].pick = 0; a->held[a->n].hp = -1; a->n++;
    }
    s->n = 0; s->state = 0;
}
internal float sk_range(ee_unit *u) {
    ee_unit_stats *st = unit_get_stats(u);
    return st ? unit_stats_get_range(st) : unit_type_get_range(unit_get_type_data(u));
}
// the position the force can't take: the shooters' spot, else the target found too strong
internal int sk_position(unsigned idx, struct armies_st *a, uint32_t now, ivec2 *pos) {
    struct as_st *as = &g_as[idx];
    int shoot = -1;
    for (int j = 0; j < AF_SHOOTS; j++)
        if (as->sh[j].t && now - as->sh[j].t < AF_SHOOT_MS && (shoot < 0 || as->sh[j].hits > as->sh[shoot].hits)) shoot = j;
    if (shoot >= 0) { *pos = as->sh[shoot].at; return 1; }
    if (a->strong_t && now - a->strong_t < AF_STRONG_MS) { *pos = a->strong; return 2; }
    return 0;
}
internal void sk_go(unsigned idx, struct armies_st *a, struct sk_st *s, uint32_t now) {
    ivec2 pos;
    int why = sk_position(idx, a, now, &pos);
    if (!why || ivec2_dist2(pos, a->home) > (long)SK_REACH * SK_REACH) return;
    // the longest range first
    static struct { int k; float rg; } c[AR_HELD];
    int nc = 0;
    for (int k = 0; k < a->n; k++) {
        ee_unit *u = a->held[k].u;
        if (a->held[k].out || a->held[k].back || !cv_alive(u) || ivec2_dist2(unit_get_tile(u), a->home) > 4 * AR_HOME_R * AR_HOME_R) continue;
        if (unit_get_hp(u) * 100 < unit_type_get_hp(unit_get_type_data(u)) * SK_FIT) continue;
        float rg = sk_range(u);
        if (rg < SK_RANGE) continue;
        int i = nc++;
        while (i > 0 && c[i - 1].rg < rg) { c[i] = c[i - 1]; i--; }
        c[i].k = k; c[i].rg = rg;
    }
    if (nc < SK_MIN) return;
    if (nc > SK_N) nc = SK_N;
    // out of the force's list (the highest index first: the compaction keeps the lower ones in place)
    s->n = 0;
    for (int i = 0; i < nc; i++) { s->m[s->n].u = a->held[c[i].k].u; s->m[s->n].id = a->held[c[i].k].id; s->n++; }
    for (int i = 0; i < nc; i++) a->held[c[i].k].u = NULL;
    int m = 0;
    for (int k = 0; k < a->n; k++) if (a->held[k].u) a->held[m++] = a->held[k];
    a->n = m;
    // the stand: SK_STAND short of the position towards home
    float dx = (float)(a->home.x - pos.x), dy = (float)(a->home.y - pos.y), l = sqrtf(dx * dx + dy * dy);
    s->pos = pos;
    s->stand = l > SK_STAND ? iv2(pos.x + (int)(dx / l * SK_STAND), pos.y + (int)(dy / l * SK_STAND)) : a->home;
    s->state = 1; s->out_t = s->creep_t = now ? now : 1;
    sk_side(s, NULL);
    s->out_hp = g_sk_own.hp;
    s->sorties++; s->members += (unsigned)s->n;
    for (int k = 0; k < s->n; k++) unit_goal(s->m[k].u, ar_goal_at(s->stand, k, 1), 0, 0x1F40);
    ar_log(idx, now, "skirmish: a party of %d (range %.0f-%.0f, hp %.0f) at %s %d,%d - its stand %d,%d", s->n, c[nc - 1].rg, c[0].rg, s->out_hp,
           why == 1 ? "the shooters'" : "the strong target", pos.x, pos.y, s->stand.x, s->stand.y);
}
// a member's target: a long-range shooter seen or revealed within SK_HUNT, else a soldier seen within its range +
// SK_PICK; 1 ordered
internal int sk_pick(ee_player *pl, struct sk_st *s, int k) {
    ee_unit *u = s->m[k].u, *best = NULL;
    ivec2 p = unit_get_tile(u);
    int r = (int)sk_range(u) + SK_PICK, h = r > SK_HUNT ? r : SK_HUNT;
    long bd = 0;
    for (struct grid_it it = grid_it_box(p.x, p.y, h, 1); grid_it_next(&it); ) {
        ee_unit *e = it.u;
        ee_player *o = unit_get_owner(e);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(e) || unit_type_get_class_id(it.td) != CLS_LAND || !seen_by(e, pl)) continue;
        long d = ivec2_dist2(unit_get_tile(e), p);
        if (d > (long)h * h) continue;
        ee_unit_stats *est = unit_get_stats(e);
        float erg = est ? unit_stats_get_range(est) : unit_type_get_range(it.td);
        if (is_siege(unit_type_get_id(it.td)) || unit_type_get_family(it.td) == KIND_ARTILLERY || erg >= (float)AF_SHOOT_D) ;   // the shooters first
        else if (ms_land_soldier(e, it.td) && d <= (long)r * r) d += (long)h * h;
        else continue;
        if (!best || d < bd) { best = e; bd = d; }
    }
    if (!best || !unit_goal(u, goal_attack(u, best), 0, 0x1F40)) return 0;
    s->picks++;
    return 1;
}
internal int sk_is_spotter(ee_unit *u) {
    ee_player *pl = u ? unit_get_owner(u) : NULL;
    struct sk_st *s = pl ? &g_sk[(unsigned)player_get_index(pl) & 15] : NULL;
    return s && s->sp == u && s->sp_id == unit_get_serial(u);
}
internal int sp_type(ee_unit_type *td) { int t = td ? unit_type_get_id(td) : -1; return t >= 0 && (t == UT(INF08_SHARPSHOOTER) || t == UT(INF10_SNIPER)); }
internal void sk_spotter(ee_player *pl, unsigned idx, struct armies_st *a, struct sk_st *s, uint32_t now) {
    if (s->sp && (unit_get_serial(s->sp) != s->sp_id || !cv_alive(s->sp) || unit_get_owner(s->sp) != pl)) {
        ar_log(idx, now, "skirmish: the spotter at %d,%d is lost", s->sp_at.x, s->sp_at.y);
        s->sp = NULL; s->sp_lost++;
    }
    ivec2 pos;
    int why = sk_position(idx, a, now, &pos);
    if (why && ivec2_dist2(pos, a->home) > (long)SK_REACH * SK_REACH) why = 0;
    if (!why) {   // no position: back into the force
        if (s->sp) {
            if (s->sp_beh) unit_set_behaviour(s->sp, s->sp_beh);
            if (a->n < AR_HELD) { a->held[a->n].u = s->sp; a->held[a->n].id = s->sp_id; a->held[a->n].order_t = 0; a->held[a->n].out = 0; a->held[a->n].back = 0; a->held[a->n].pick = 0; a->held[a->n].hp = -1; a->n++; }
            ar_log(idx, now, "skirmish: the spotter back to the force");
            s->sp = NULL;
        }
        return;
    }
    if (!s->sp) {   // one of the force's, the nearest home
        int best = -1;
        long bd = 0;
        for (int k = 0; k < a->n; k++) {
            ee_unit *u = a->held[k].u;
            if (a->held[k].out || !cv_alive(u) || !sp_type(unit_get_type_data(u))) continue;
            long d = ivec2_dist2(unit_get_tile(u), a->home);
            if (best < 0 || d < bd) { best = k; bd = d; }
        }
        if (best < 0) {   // none: one trained
            if (s->sp_train_t && now - s->sp_train_t < SP_TRAIN_MS) return;
            s->sp_train_t = now ? now : 1;
            ee_civ_mgr *cm = player_get_civ_mgr(pl);
            void *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
            int types[2] = { UT(INF10_SNIPER), UT(INF08_SHARPSHOOTER) };
            for (int i = 0; um && i < 2; i++) {
                if (!ut_has(types[i])) continue;
                int req[4] = { types[i], 1, 0, 0 };
                unsigned got = 0;
                g_priority_mil = 1;
                hook_train_units(um, req, (uintptr_t)&got);
                g_priority_mil = 0;
                if (got) { s->sp_trained++; ar_log(idx, now, "skirmish: a spotter (type %d) trained", types[i]); break; }
            }
            return;
        }
        s->sp = a->held[best].u; s->sp_id = a->held[best].id;
        a->held[best] = a->held[--a->n];
        s->sp_beh = unit_get_behaviour(s->sp);
        set_behaviour(s->sp, 0);   // Scout: never attacks
        s->sp_t = 0; s->spotters++;
    }
    // its point: SP_BACK inside its sight short of the position
    ee_unit_stats *st = unit_get_stats(s->sp);
    float sight = st ? unit_stats_get_sight(st) : 8.0f, d = sight - SP_BACK > 4.0f ? sight - SP_BACK : 4.0f;
    float dx = (float)(a->home.x - pos.x), dy = (float)(a->home.y - pos.y), l = sqrtf(dx * dx + dy * dy);
    ivec2 at = l > d ? iv2(pos.x + (int)(dx / l * d), pos.y + (int)(dy / l * d)) : pos;
    if (ivec2_dist2(at, s->sp_at) > 4 || !s->sp_t) ar_log(idx, now, "skirmish: the spotter (type %d, sight %.0f) to %d,%d for %s %d,%d", unit_type_get_id(unit_get_type_data(s->sp)), sight, at.x, at.y,
                                                      why == 1 ? "the shooters'" : "the strong target", pos.x, pos.y);
    s->sp_at = at;
    if (ivec2_dist2(unit_get_tile(s->sp), at) <= 2 || (unit_get_goal(s->sp) && s->sp_t && now - s->sp_t < SP_ORDER_MS)) return;
    if (unit_goal(s->sp, goal_move((float)at.x + 0.5f, (float)at.y + 0.5f), 0, 0x1F40)) s->sp_t = now ? now : 1;
}
internal void skirmish_tick(ee_player *pl, uint32_t now) {
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    struct sk_st *s = &g_sk[idx];
    struct armies_st *a = &g_armies[idx];
    if (now < s->t) memset(s, 0, sizeof *s);
    int on = PV(idx, g_v_skirmish, SK_ON);
    if (!on || !ar_on(pl, idx) || (s->t && now - s->t < SK_EVERY)) return;
    s->t = now ? now : 1;
    if (g_log && g_autotest && (s->sorties || s->n || s->spotters || s->sp_trained) && (!s->log_t || now - s->log_t >= 60000)) {
        s->log_t = now ? now : 1;
        ar_log(idx, now, "skirmish: %u sorties (%u members, %u lost), %u targets picked, home by chasers %u, hit points %u, time %u, the force at the chasers %u%s; spotters %u (lost %u, trained %u)%s",
               s->sorties, s->members, s->lost, s->picks, s->chased, s->bled, s->timed, s->baits, s->state ? " - a party out" : "",
               s->spotters, s->sp_lost, s->sp_trained, s->sp ? " - one out" : "");
    }
    if (on & 2) sk_spotter(pl, idx, a, s, now);
    if (!(on & 1)) return;
    if (!s->state) {
        if (!a->out && (!s->end_t || now - s->end_t >= SK_REST)) sk_go(idx, a, s, now);
        return;
    }
    ivec2 mid = a->home;
    int alive = sk_side(s, &mid);
    if (alive < SK_MIN && s->state == 1) {
        ar_log(idx, now, "skirmish: the party is down to %d - home", alive);
        s->state = 2; s->bled++;
        for (int k = 0; k < s->n; k++) if (sk_alive(s, k)) unit_goal(s->m[k].u, ar_goal_at(a->home, k, 0), 0, 0x1F40);
    }
    ivec2 cmid = mid;
    int nch = sk_chasers(pl, mid, SK_CHASE_R, &cmid);
    float r = nch ? st_ratio(&g_sk_own, &g_sk_foe) : 99.0f;
    if (s->state == 1) {
        const char *why = NULL;
        if (nch && r * 100.0f < (float)SK_HOLD_PCT) { why = "chased"; s->chased++; }
        else if (g_sk_own.hp * 100.0f < s->out_hp * (float)SK_BLEED_PCT) { why = "bled"; s->bled++; }
        else if (now - s->out_t >= SK_MAX_MS) { why = "its time is up"; s->timed++; }
        if (why) {
            ar_log(idx, now, "skirmish: the party of %d at %d,%d %s - home (hp %.0f of %.0f; %d enemy fighters near, %.2f)", alive, mid.x, mid.y, why, g_sk_own.hp, s->out_hp, nch, nch ? r : 0.0f);
            s->state = 2; s->out_t = now ? now : 1;
            for (int k = 0; k < s->n; k++) if (sk_alive(s, k)) unit_goal(s->m[k].u, ar_goal_at(a->home, k, 0), 0, 0x1F40);
            return;
        }
        int at_stand = ivec2_dist2(mid, s->stand) <= 4 * AR_DONE * AR_DONE, picked = 0;
        for (int k = 0; k < s->n; k++) {
            if (!sk_alive(s, k)) continue;
            ee_goal *g = unit_get_goal(s->m[k].u);
            if (g && goal_is_attack(g)) continue;
            if (sk_pick(pl, s, k)) { picked++; continue; }
            if (!g && !at_stand) unit_goal(s->m[k].u, ar_goal_at(s->stand, k, 1), 0, 0x1F40);   // idle short of the stand: on
        }
        if (at_stand && !picked && !nch && now - s->creep_t >= SK_CREEP_MS && ivec2_dist2(s->stand, s->pos) > SK_STAND_MIN * SK_STAND_MIN) {   // nothing to shoot: closer
            s->creep_t = now ? now : 1;
            float dx = (float)(s->pos.x - s->stand.x), dy = (float)(s->pos.y - s->stand.y), l = sqrtf(dx * dx + dy * dy);
            s->stand = iv2(s->stand.x + (int)(dx / l * SK_CREEP), s->stand.y + (int)(dy / l * SK_CREEP));
            for (int k = 0; k < s->n; k++) if (sk_alive(s, k) && !unit_get_goal(s->m[k].u)) unit_goal(s->m[k].u, ar_goal_at(s->stand, k, 1), 0, 0x1F40);
        }
        return;
    }
    // home bound: the chasers near home - the force goes at them
    if (nch && !a->out && ivec2_dist2(cmid, a->home) <= (long)SK_AMBUSH * SK_AMBUSH) {
        float rf = ar_judge(pl, a, 0, cmid);
        if (rf * 100.0f >= (float)AR_GO_PCT) {
            s->baits++;
            ar_log(idx, now, "skirmish: %d chasers of the party at %d,%d - the force of %d goes at them (%.2f)", nch, cmid.x, cmid.y, ar_count(a, 0), rf);
            ar_go(pl, idx, a, 2, cmid, NULL, 0, now);
        }
    }
    if (ivec2_dist2(mid, a->home) <= 4 * AR_HOME_R * AR_HOME_R || !alive || now - s->out_t >= SK_HOME_MS) {
        ar_log(idx, now, "skirmish: the party home - %d of %d (hp %.0f of %.0f)", alive, s->n, g_sk_own.hp, s->out_hp);
        sk_disband(a, s);
        s->end_t = now ? now : 1;
    }
}
internal void skirmish_new_game(void) { memset(g_sk, 0, sizeof g_sk); }
internal void skirmish_install(const char *autotest_ini) {
    if (autotest_ini) g_v_skirmish = ini_int("autotest", "v_skirmish", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# skirmish: %d (variant %d)\n", SK_ON, g_v_skirmish);
}
