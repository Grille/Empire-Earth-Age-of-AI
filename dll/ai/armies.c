// THE MOD'S ARMIES ([ai_base] mod_armies; [autotest] v_mod_armies for the variant players; land maps - navy_land).
// Included by eeaimod.c after stage.c (mass.c ms_land_soldier, landing.c landing_has_unit, army.c ou_sortie,
// attack.c hook_unit_to_group, navy.c navy_land, stage.c stage_foe, fronts.c fronts_station).
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
// empty, its launch finds nothing and schedules the next attack time: the commander's own timing stays.
// THE FORCE: the held soldiers are one force, ordered as one (the player's own way: the whole force of the region
// goes, holds, and comes back together). Its home: the front station (fronts.c: a bit behind the tower nearest the
// heaviest front), the staging point when there is none, else the base - a member idle away from it walks there. At
// the attack time (player next_attack) the force takes the enemy building nearest its home that it knows (seen_by:
// seen or remembered) when the whole force beats the enemy there (stage_foe: the fighters seen and remembered within
// SG_FOE, the defences covering it) by AR_GO_PCT: every member attack-moves to it (the game's attack-move: it fights
// what it meets and walks on). Under that the force waits at home and grows - the ratio grows with its numbers. Out,
// every AR_JUDGE_EVERY the force is judged as a whole - all its members out against the enemy at its middle and at
// its destination, the lesser - and under AR_BACK_PCT twice running it goes home as one (a plain move: no fighting on
// the way). The building down (none seen within AR_DONE of the destination with a member there), the next: the known
// enemy building nearest the force's middle, judged the same - under AR_GO_PCT: home. A raid (AR_RAID_BAND enemy land
// fighters seen within AR_RAID_R of an own building, or one attacking something of ours, within AR_RAID_REACH of
// home, seen there twice running) while the force is home: the whole force attack-moves to it - one order to the
// force, not one per soldier - and goes home once no enemy fighter is seen there. Soldiers trained while the force
// is out gather at home (not sent after it one by one) and join the next push. A member idle away from where the
// force was sent is ordered again, at most every AR_REORDER.
// Measured 2026-11-02 (head-to-head, Continental huge 1v1 late, 75 min, 24 mirrored games; a game: 1.6 pushes, 3.5
// further legs, 12.3 raids answered, 1.7 retreats as losing, 0.25 next targets refused, no wait at the attack time -
// the enemy there is unknown then -, 3260 orders): soldiers +9.3 (se 7.7), kills +9.3 (12.6), losses -15.8 (13.9),
// razed +2.4 (4.4), workers +14.6 (11.3); buildings lost 8.7 against the commander's 11.2 - the first owned army
// that loses fewer than the commander (the parity release: 6.2 against 4.5). A second 24 (seeds 13-24) brought the
// 48 to soldiers +10.6 (5.1), kills +1.2 (9.3), losses -5.6 (9.9), razed +1.4 (2.4), buildings lost 8.2 / 9.6: level.
// The judge lines of a push showed why: the force of 31 walked into the enemy base judged 1.3-3 all the way while its
// hit points went 10700 -> 2900 in 3 minutes - the enemy's soldiers arrive one by one at the middle, the ratio stays
// above 1 and the force is ground down; it was called losing with 19 of 71 left on average (84 of 96 pushes). So
// home when the force is no longer winning: AR_BACK_PCT 70 -> 100 (48 games): soldiers +12.9 (se 5.5), kills +30.4
// (7.9), losses -31.9 (7.9), razed +3.3 (1.6), buildings lost 7.2 against 10.6. The same with 70 and a bleed rule
// instead (home once the force has lost half the hit points it went out with, AR_BLEED_PCT 50; 48 games): kills
// +31.9 (6.9), losses -33.0 (7.0), razed +1.8 (1.3), soldiers +6.9 (4.1), buildings lost 5.6 / 7.4 - as good on the
// fight, less on the buildings; 100 kept, the bleed rule left in at 0. Confirmed with 100 on other setups (24
// mirrored games each): 4 AIs on Continental huge late - soldiers +18.4 (6.0), kills -22.0 (9.0), losses -33.1
// (10.7), razed +7.3 (3.0), buildings lost 11.7 / 20.2; Mediterranean huge 1v1 late - kills -6.8 (13.0), losses
// +5.8 (13.4), razed +3.3 (1.5), workers -10.2 (5.7), buildings lost 6.8 / 10.1; from epoch 1 on Continental large,
// 100 min - kills +19.3 (13.1), losses -3.7 (18.5), razed +11.3 (5.4), buildings lost 15.5 / 28.7 (23 raids a game
// there). On by default since 2026-11-02 ([ai_base] mod_armies). The raid's band (AR_RAID_BAND 3, before: any
// enemy fighter seen near a building; 48 games): raids 12.4 -> 9.8 a game, orders 2876 -> 2567, every score level
// (kills +1.5 se 7.4, losses -1.1, razed -0.4, buildings lost 6.5 / 6.9) - nearly every raid answered is one enemy
// actually attacking something of ours; kept as the sanity bound it is. PRESSING ON (AR_PRESS 1: after a push, the
// next as soon as the force beats by AR_GO_PCT the enemy that sent it home, 1.9 presses a game; 48 games): kills
// -35.3 (se 9.6), losses +34.2 (9.5), razed -0.1, buildings lost 5.7 / 5.6 - the judge at the target from home sees
// nothing of a base it hasn't scouted, so the force walks back into what beat it and bleeds again before the retreat
// rule bites. Only after a push that razed and wasn't driven off (AR_PRESS 2): 0.2 presses a game, level (kills -6.9
// se 8.1, razed +0.7 se 1.5). Off: the game's attack times stay the push times. THE FORTIFIED LINE (AR_FORT: home at
// the own defence near the station covered by the most defences, found on 2026 of 2250 home checks in a smoke game;
// 48 games): level - soldiers -7.9 (se 5.2), kills +1.7 (7.2), losses -3.1 (7.1), razed -0.3, buildings lost 6.6 /
// 6.3. Off. The Mediterranean workers -10.2 of the confirmation was noise: a second 24 there gave +0.4 (6.5), razed
// +2.7 (2.0).
// BEFORE THE FORCE (2026-11-02, head-to-head, Continental huge 1v1 late, 75 min, 24 mirrored games each): PARITY - the
// held let go to the game's own launch a tick ahead of the attack time (ai_held cleared, register_unit): soldiers
// +14.5 (se 6.2), kills +1.3 (6.9), losses -8.0 (8.5), razed -1.8 (1.6), workers -8.1 (10.5) - the soldiers trained
// between two waves waiting at home instead of trickling after one out. On top of it: the held standing at the front
// station between waves (front_army, ~720 station walks a game) - soldiers +8.5 (6.1), kills -2.7 (11.8), losses +4.3
// (12.9), razed -0.8 (1.8): nothing; the release only when the army beats the strongest enemy army known by
// army_mass_ratio (army_mass WAIT) - it never waited (the enemy army is rarely known before the attack: intel's
// soldiers seen within 10 min), parity with noise. A retreat of the game's wave (called off under 70%, judged at the
// members' middle with the own fighters within SG_NEAR of it): soldiers +10 (4.3), kills -5.8 (9.1), losses +3.7
// (10.0), razed -4.5 (1.7), workers -17 (7.3) - the judge misread a spread wave (a wave of 95 against one unit and
// towers called off). A defence on top - every held soldier within 30 of a raid sent at the raider nearest it: kills
// -34 (15.6), losses +29 (16.3), razed -3.6 (1.2): 195 raids a game answered with 1353 single orders, the soldiers
// thrown at the enemy's wave one at a time. Buildings lost A / B: parity 4.5 / 6.2, station 5.8 / 6.7, retreat 3.0 /
// 7.5, defence 2.6 / 6.2 - holding the soldiers idle between waves costs about 1.7 buildings each way: the commander
// keeps 3-4 squads out all the time (continuous pressure), and its soldiers at home stand in groups that go for what
// comes near them together. Every add-on that ordered the held one by one lost or changed nothing: the trickle. A
// launch of the mod's own as one of the game's groups (ag_launch) at the nearest known enemy building: razed 3.5
// against the commander's 10.3 (-6.8, se 2.1) - the wave went home once its one building fell. The game's
// attack-on-a-player form (a task with the target player only, strategy kind 0) is refused by attack_plan_ok (it
// wants a target unit or a location); the game's own launch doesn't ask it.
// "# armies:" once a game minute (autotest) and at every order to the force.
#define AR_TICK 2000
#define AR_MIN_LAUNCH 8            // soldiers a push needs at least (the outdated sortie's)
#define AR_MIN_RAID 3
#define AR_HELD 2048
#define AR_HOME_R 10               // at home: within this of it
#define AR_GO_PCT 130              // a push: the force beats the enemy at the target by this (army_stage_go's 130)
#ifndef AR_BACK_PCT
#define AR_BACK_PCT 100            // out: the force's fight under this twice running - home (70 bled: below)
#endif
#ifndef AR_BLEED_PCT
#define AR_BLEED_PCT 0             // out: the force's hit points down by this much of what it went out with - home (0: never)
#endif
#define AR_JUDGE_EVERY 4000
#define AR_DONE 8                  // the destination reached: a member within this of it
#define AR_RAID_EVERY 4000
#define AR_RAID_R 14               // a raid: an enemy land fighter seen within this of an own building
#define AR_RAID_REACH 60           // raids the force answers: within this of home
#define AR_REORDER 20000           // a member ordered again at most every this
#define AR_JIT 5                   // the orders' spread: a 5 x 5 square of tiles
#define AR_HOME_MOVE 12            // home moves when the station is this far from it for AR_HOME_STAY (at once when home is in danger)
#define AR_HOME_STAY 60000
#define AR_RAID_SAME 10            // a raid: a raider seen twice running (AR_RAID_EVERY apart) within this of each other
#ifndef AR_PRESS
#define AR_PRESS 0                 // pressing on: 1 after any push, 2 after one that razed and wasn't driven off - the next as soon as the force beats what stopped it (0: the attack time only; 1 lost: below)
#endif
#define AR_PRESS_EVERY 10000
#define AR_PRESS_HOME_PCT 75       // pressing on: this much of the force gathered at home
#ifndef AR_FORT
#define AR_FORT 0                  // home at the own fortified line: the own defence within AR_FORT_R of the station covered by the most own defences (level: above)
#endif
#define AR_FORT_R 25
#ifndef AR_RAID_BAND
#define AR_RAID_BAND 3             // a raid: this many enemy fighters seen within AR_RAID_R of the raider, or one of them attacking ours
#endif
internal int g_mod_armies = 0, g_v_mod_armies = -1, g_v_ar_raid_band = -1, g_v_ar_press = -1, g_v_ar_fort = -1;   // [autotest] v_ar_raid_band, v_ar_press, v_ar_fort: the variant players' AR_RAID_BAND, AR_PRESS, AR_FORT
#define MOD_ARMIES(idx) PV(idx, g_v_mod_armies, g_mod_armies)
internal struct armies_st {
    uint32_t t, log_t;
    int n;
    struct { ee_unit *u; int id; uint32_t order_t; uint8_t out; } held[AR_HELD];   // out: went with the force
    int out;                               // 0 home, 1 a push, 2 a raid
    ivec2 home, dest, cand, raid_c;         // home; the force's destination; the next home seen (since cand_t); the raider seen last
    ee_unit *target; int target_id;        // the push's building (NULL: a raid's point)
    uint32_t judge_t, raid_t, out_t, wait_due, home_t, cand_t, raid_c_t, press_t;
    int weak, clear, stopped;              // judgements running under AR_BACK_PCT; raid checks running with no enemy seen; a push has ended (2: with its target down, not driven off)
    float ratio, out_hp, stop_hp;          // the last judgement; the force's hit points as it went out; the enemy's that ended the last push
    unsigned held_all, pushes, presses, legs, raids, backs, waits, orders, refused, forted;   // forted: home checks that found a defence to stand at
} g_armies[16];
internal struct st_side g_ar_own;
internal int g_ar_debug = 0, g_ar_foe_mid_n; internal float g_ar_foe_mid_hp;   // DEBUG
internal int ar_on(ee_player *pl, unsigned idx) { return pl && !is_vanilla(pl) && MOD_ARMIES(idx) && navy_land(idx); }
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
    if (!td || !ar_on(pl, idx) || !ms_land_soldier(u, td)) return 0;
    g_armies[idx].refused++;
    return 1;
}
__attribute__((format(printf, 3, 4))) internal void ar_log(unsigned idx, uint32_t now, const char *fmt, ...) {
    if (!g_log || !g_autotest) return;
    va_list ap;
    va_start(ap, fmt);
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# armies: P%u at %.1f min: ", idx, now / 60000.0);
    vfprintf(g_log, fmt, ap);
    fputc('\n', g_log);
    LeaveCriticalSection(&g_lock);
    va_end(ap);
}
// the force's home: the front station when it is safe, else the staging point, else the base (0: none yet). The
// station moves with the fronts' weights (every 10 s), the staging point with the marches: home follows only a
// place that stays AR_HOME_MOVE away for AR_HOME_STAY (the smoke game: home jumped 20-60 tiles every few minutes and
// the whole force walked after it, 1900 orders in 45 min), at once when home is in danger.
// THE FORTIFIED LINE (AR_FORT, the player: "retreat into fortified positions"): home is not the bare station but the
// own built defence within AR_FORT_R of it that the most own defences cover (the nearest on ties), when there is one
// and it isn't in danger - the force stands under its towers and walls and the enemy that follows it home meets them.
internal int defence_stats(ee_unit *u, float *range, float *attack);   // attack.c
internal ivec2 ar_fort(ee_player *pl, unsigned idx, ivec2 at) {
    static ee_unit *def[256]; static float rg[256];
    int nd = 0;
    for (struct grid_it it = grid_it_box(at.x, at.y, AR_FORT_R, 1); grid_it_next(&it) && nd < 256; ) {
        ee_unit *u = it.u;
        int fam = unit_type_get_class_id(it.td);
        float r, atk;
        if (unit_get_owner(u) != pl || (fam != CLS_BUILDING && fam != CLS_FORTRESS) || !cv_alive(u) || !cv_built(u)) continue;
        if (!defence_stats(u, &r, &atk) || atk <= 0 || ivec2_dist2(unit_get_tile(u), at) > AR_FORT_R * AR_FORT_R) continue;
        def[nd] = u; rg[nd] = r; nd++;
    }
    int best = -1, bc = 0;
    long bd = 0;
    for (int k = 0; k < nd; k++) {
        ivec2 p = unit_get_tile(def[k]);
        if (stage_danger(idx, p)) continue;
        int c = 0;
        for (int j = 0; j < nd; j++) c += (float)ivec2_dist2(unit_get_tile(def[j]), p) <= (rg[j] + 1.0f) * (rg[j] + 1.0f);
        long d = ivec2_dist2(p, at);
        if (best < 0 || c > bc || (c == bc && d < bd)) { best = k; bc = c; bd = d; }
    }
    return best >= 0 ? unit_get_tile(def[best]) : at;
}
internal int ar_home(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {
    ivec2 at;
    struct stage *st = &g_stg[idx];
    if (!(fronts_station(idx, 0, &at) && !stage_danger(idx, at)) && !(st->on && !stage_danger(idx, (at = st->s))) && !ms_base(pl, &at)) return 0;
    if (PV(idx, g_v_ar_fort, AR_FORT)) { ivec2 f = ar_fort(pl, idx, at); a->forted += f.x != at.x || f.y != at.y; at = f; }
    if (!a->home_t) { a->home = at; a->home_t = now ? now : 1; return 1; }
    if (ivec2_dist2(at, a->home) <= AR_HOME_MOVE * AR_HOME_MOVE) { a->cand_t = 0; return 1; }
    if (stage_danger(idx, a->home)) { a->home = at; a->cand_t = 0; return 1; }
    if (!a->cand_t || ivec2_dist2(at, a->cand) > AR_HOME_MOVE * AR_HOME_MOVE) { a->cand = at; a->cand_t = now ? now : 1; return 1; }
    if (now - a->cand_t >= AR_HOME_STAY) { a->home = at; a->cand_t = 0; }
    return 1;
}
// the force (out: the members out with it, else all) as a side, its middle and count
internal int ar_side(struct armies_st *a, int out, ivec2 *mid) {
    long sx = 0, sy = 0;
    int n = 0;
    st_side_clear(&g_ar_own);
    for (int k = 0; k < a->n; k++) {
        ee_unit *u = a->held[k].u;
        if ((out && !a->held[k].out) || !cv_alive(u)) continue;
        ivec2 p = unit_get_tile(u);
        sx += p.x; sy += p.y; n++;
        st_side_add(&g_ar_own, u);
    }
    if (mid) *mid = n ? iv2((int)(sx / n), (int)(sy / n)) : a->home;
    return n;
}
// the force against the enemy at d as pl knows it (stage_foe)
internal float ar_judge(ee_player *pl, struct armies_st *a, int out, ivec2 d) {
    stage_foe(pl, d);
    ar_side(a, out, NULL);
    return st_ratio(&g_ar_own, &g_sgs_foe);
}
// the enemy building nearest from that pl knows of (seen or remembered; no walls: seen_by's intel_remembers covers
// buildings and fortresses, walls are neither)
internal ee_unit *ar_target(ee_player *pl, ivec2 from) {
    ee_unit *best = NULL;
    long bd = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(u) || !fr_building(it.td) || !seen_by(u, pl)) continue;
        long d = ivec2_dist2(unit_get_tile(u), from);
        if (!best || d < bd) { best = u; bd = d; }
    }
    return best;
}
internal ee_unit *fl_target_of(ee_unit *u);   // fleet.c: its attack order's target
internal int ar_enemy_near(ee_player *pl, ivec2 d, int r, int buildings) {   // enemy land fighters (or buildings) pl sees within r of d
    for (struct grid_it it = grid_it_box(d.x, d.y, r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(u) || ivec2_dist2(unit_get_tile(u), d) > r * r) continue;
        if ((buildings ? fr_building(it.td) : ms_land_soldier(u, it.td)) && seen_by(u, pl)) return 1;
    }
    return 0;
}
internal void ar_order(struct armies_st *a, int k, ee_goal *g, uint32_t now) {
    if (!unit_goal(a->held[k].u, g, 0, 0x1F40)) return;
    a->held[k].order_t = now ? now : 1;
    a->orders++;
}
internal ee_goal *ar_goal_at(ivec2 d, int k, int fight) {   // the force's order at d, spread over AR_JIT x AR_JIT tiles
    float xy[2] = { (float)d.x + 0.5f + (float)(k % AR_JIT) - (float)(AR_JIT / 2), (float)d.y + 0.5f + (float)(k / AR_JIT % AR_JIT) - (float)(AR_JIT / 2) };
    if (!fight) return goal_move(xy[0], xy[1]);
    void *mem = goal_move_alloc();
    return mem ? goal_attack_move_init(mem, xy) : NULL;
}
// the force goes (kind 1 a push at target, 2 a raid at d): all its members, or only those out already (a next leg)
internal void ar_go(struct armies_st *a, int kind, ivec2 d, ee_unit *target, int out_only, uint32_t now) {
    a->out = kind; a->dest = d; a->target = target; a->target_id = target ? unit_get_id(target) : 0;
    a->out_t = now ? now : 1; a->judge_t = 0; a->weak = 0; a->clear = 0;
    for (int k = 0; k < a->n; k++) {
        if (out_only && !a->held[k].out) continue;
        a->held[k].out = 1;
        ar_order(a, k, ar_goal_at(d, k, 1), now);
    }
    if (!out_only) { ar_side(a, 1, NULL); a->out_hp = g_ar_own.hp; }
}
internal void ar_stopped(struct armies_st *a, float foe_hp, int won) { a->stopped = won ? 2 : 1; a->stop_hp = foe_hp; }   // a push ends: what stopped it
internal void ar_home_all(struct armies_st *a, uint32_t now) {   // the force out comes home as one
    a->out = 0; a->target = NULL; a->backs++;
    for (int k = 0; k < a->n; k++) {
        if (!a->held[k].out) continue;
        a->held[k].out = 0;
        ar_order(a, k, ar_goal_at(a->home, k, 0), now);
    }
}
internal int ar_idle(ee_unit *u) { return unit_ai_state_id(u) == 5000 && !unit_get_goal(u); }
internal int ar_count(struct armies_st *a, int out) {
    int n = 0;
    for (int k = 0; k < a->n; k++) n += (!out || a->held[k].out) && cv_alive(a->held[k].u);
    return n;
}
// the force out: its judgement, its next leg or its way home
internal void ar_out(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {
    ivec2 mid;
    int n = ar_side(a, 1, &mid);
    if (n < (a->out == 2 ? 1 : AR_MIN_LAUNCH / 2)) {
        ar_log(idx, now, "the force out is down to %d - home to %d,%d", n, a->home.x, a->home.y);
        if (a->out == 1) ar_stopped(a, g_sgs_foe.hp, 0);
        ar_home_all(a, now);
        return;
    }
    if (!a->judge_t || now - a->judge_t >= AR_JUDGE_EVERY) {
        a->judge_t = now ? now : 1;
        float r = ar_judge(pl, a, 1, mid);
        g_ar_foe_mid_n = g_sgs_foe.n; g_ar_foe_mid_hp = g_sgs_foe.hp;
        float rd = ar_judge(pl, a, 1, a->dest);
        int own = g_ar_own.n, foe = g_sgs_foe.n;
        a->ratio = rd < r ? rd : r;
        if (g_ar_debug) ar_log(idx, now, "  judge: %d out (hp %.0f) at %d,%d - at the middle %.2f (enemy %d, hp %.0f), at %d,%d %.2f (enemy %d, hp %.0f)", n, g_ar_own.hp, mid.x, mid.y, r, g_ar_foe_mid_n, g_ar_foe_mid_hp, a->dest.x, a->dest.y, rd, foe, g_sgs_foe.hp);
        if (AR_BLEED_PCT && g_ar_own.hp < a->out_hp * (float)(100 - AR_BLEED_PCT) / 100.0f) {
            ar_log(idx, now, "the force of %d at %d,%d has bled (hp %.0f of %.0f) - home to %d,%d", n, mid.x, mid.y, g_ar_own.hp, a->out_hp, a->home.x, a->home.y);
            if (a->out == 1) ar_stopped(a, g_sgs_foe.hp, 0);
            ar_home_all(a, now);
            return;
        }
        if (a->ratio * 100.0f < (float)AR_BACK_PCT) {
            if (++a->weak >= 2) {
                ar_log(idx, now, "the force of %d at %d,%d is losing (%.2f: own %d, enemy %d at %d,%d) - home to %d,%d", n, mid.x, mid.y, a->ratio, own, foe, a->dest.x, a->dest.y, a->home.x, a->home.y);
                if (a->out == 1) ar_stopped(a, rd < r ? g_sgs_foe.hp : g_ar_foe_mid_hp, 0);
                ar_home_all(a, now);
                return;
            }
        } else a->weak = 0;
    }
    // the leg done: the building down (or gone from the tile with a member there), the raid cleared
    int there = 0;
    for (int k = 0; k < a->n && !there; k++) there = a->held[k].out && cv_alive(a->held[k].u) && ivec2_dist2(unit_get_tile(a->held[k].u), a->dest) <= AR_DONE * AR_DONE;
    int done;
    if (a->out == 2) { a->clear = ar_enemy_near(pl, a->dest, SG_FOE, 0) ? 0 : a->clear + 1; done = a->clear >= 2; }
    else done = (a->target && (unit_get_id(a->target) != a->target_id || !cv_alive(a->target))) || (there && !ar_enemy_near(pl, a->dest, 3, 1));
    if (done) {
        if (a->out == 2) { ar_log(idx, now, "the raid at %d,%d is over - the force of %d home to %d,%d", a->dest.x, a->dest.y, n, a->home.x, a->home.y); ar_home_all(a, now); return; }
        ee_unit *t = ar_target(pl, mid);
        ivec2 d = t ? unit_get_tile(t) : mid;
        float r = t ? ar_judge(pl, a, 1, d) : 0.0f;
        if (!t || r * 100.0f < (float)AR_GO_PCT) {
            ar_log(idx, now, "%d,%d is down - %s - the force of %d home to %d,%d", a->dest.x, a->dest.y, t ? "the next is too strong" : "no enemy building known", n, a->home.x, a->home.y);
            ar_stopped(a, t ? g_sgs_foe.hp : 0.0f, 1);
            ar_home_all(a, now);
            return;
        }
        a->legs++;
        ar_log(idx, now, "%d,%d is down - the force of %d on to %d,%d (%.2f: own %d, enemy %d)", a->dest.x, a->dest.y, n, d.x, d.y, r, g_ar_own.n, g_sgs_foe.n);
        ar_go(a, 1, d, t, 1, now);
        return;
    }
    // a member idle away from the destination: ordered again
    for (int k = 0; k < a->n; k++) {
        ee_unit *u = a->held[k].u;
        if (!a->held[k].out || !cv_alive(u) || !ar_idle(u) || now - a->held[k].order_t < AR_REORDER) continue;
        if (ivec2_dist2(unit_get_tile(u), a->dest) <= AR_DONE * AR_DONE) continue;
        ar_order(a, k, ar_goal_at(a->dest, k, 1), now);
    }
}
// the force at home: its members gather there; a raid near answered; the attack time's push
internal void ar_at_home(ee_player *pl, unsigned idx, struct armies_st *a, uint32_t now) {
    for (int k = 0; k < a->n; k++) {
        ee_unit *u = a->held[k].u;
        if (a->held[k].out || !cv_alive(u) || !ar_idle(u) || now - a->held[k].order_t < AR_REORDER) continue;
        if (ivec2_dist2(unit_get_tile(u), a->home) <= AR_HOME_R * AR_HOME_R) continue;
        ar_order(a, k, ar_goal_at(a->home, k, 0), now);
    }
    int n = ar_count(a, 0);
    if (a->out) return;
    if (n >= AR_MIN_RAID && (!a->raid_t || now - a->raid_t >= AR_RAID_EVERY)) {   // the raid nearest home
        a->raid_t = now ? now : 1;
        ee_unit *raider = NULL;
        long bd = 0;
        int band = 0;
        for (struct grid_it it = grid_it_box(a->home.x, a->home.y, AR_RAID_REACH, 1); grid_it_next(&it); ) {
            ee_unit *u = it.u;
            ee_player *o = unit_get_owner(u);
            if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !ms_land_soldier(u, it.td) || !seen_by(u, pl)) continue;
            ivec2 p = unit_get_tile(u);
            long d = ivec2_dist2(p, a->home);
            if (d > (long)AR_RAID_REACH * AR_RAID_REACH || (raider && d >= bd)) continue;
            // a raid, not a scout passing: the raider's band (enemy fighters seen within AR_RAID_R of it) is AR_RAID_BAND
            // strong, or one of them is attacking something of ours - the whole force went for every single scout before
            // (12-23 raids a game, 3000 orders, the force away when the real attack came)
            int close = 0, nb = 0, hitting = 0;
            for (struct grid_it jt = grid_it_box(p.x, p.y, AR_RAID_R, 1); grid_it_next(&jt) && !close; )
                close = unit_get_owner(jt.u) == pl && cv_alive(jt.u) && fr_building(jt.td) && ivec2_dist2(unit_get_tile(jt.u), p) <= AR_RAID_R * AR_RAID_R;
            if (!close) continue;
            for (struct grid_it jt = grid_it_box(p.x, p.y, AR_RAID_R, 1); grid_it_next(&jt) && !hitting; ) {
                ee_player *jo = unit_get_owner(jt.u);
                if (!jo || !is_enemy(pl, jo) || !ms_land_soldier(jt.u, jt.td) || !seen_by(jt.u, pl) || ivec2_dist2(unit_get_tile(jt.u), p) > AR_RAID_R * AR_RAID_R) continue;
                nb++;
                ee_unit *t = fl_target_of(jt.u);
                hitting = t && unit_get_owner(t) == pl;
            }
            if (nb < PV(idx, g_v_ar_raid_band, AR_RAID_BAND) && !hitting) continue;
            raider = u; bd = d; band = nb;
        }
        ivec2 d = raider ? unit_get_tile(raider) : iv2(-1000, -1000);
        int again = raider && a->raid_c_t && now - a->raid_c_t <= 2 * AR_RAID_EVERY && ivec2_dist2(d, a->raid_c) <= AR_RAID_SAME * AR_RAID_SAME;   // seen there twice running
        a->raid_c = d; a->raid_c_t = raider ? (now ? now : 1) : 0;
        if (again) {
            float r = ar_judge(pl, a, 0, d);
            if (r * 100.0f >= (float)AR_BACK_PCT) {
                a->raids++;
                ar_log(idx, now, "a raid of %d at %d,%d - the force of %d goes (%.2f: own %d, enemy %d)", band, d.x, d.y, n, r, g_ar_own.n, g_sgs_foe.n);
                ar_go(a, 2, d, NULL, 0, now);
                return;
            }
        }
    }
    // PRESSING ON: the game's attack times are minutes apart; after a push the force goes again as soon as it beats
    // what stopped it (the enemy's hit points at the judgement that sent it home: the judge at the target from home
    // sees nothing of an enemy base it hasn't scouted) by AR_GO_PCT, AR_PRESS_HOME_PCT of it gathered, and the target
    // judged as at the attack time. The first try waited for the force's own hit points as it went out: a push of 123
    // that razed three buildings and came home with 50 never pressed again.
    uint32_t due = player_get_next_attack(pl);
    int press = 0;
    int press_mode = PV(idx, g_v_ar_press, AR_PRESS);
    if (press_mode && a->stopped && (press_mode < 2 || a->stopped == 2) && n >= AR_MIN_LAUNCH && (!a->press_t || now - a->press_t >= AR_PRESS_EVERY) && !(now + 2 * AR_TICK >= due && a->wait_due != due)) {
        a->press_t = now ? now : 1;
        int at_home = 0;
        for (int k = 0; k < a->n; k++) at_home += cv_alive(a->held[k].u) && ivec2_dist2(unit_get_tile(a->held[k].u), a->home) <= AR_HOME_R * AR_HOME_R;
        ar_side(a, 0, NULL);
        press = at_home * 100 >= n * AR_PRESS_HOME_PCT && g_ar_own.hp * 100.0f >= a->stop_hp * (float)AR_GO_PCT;
    }
    if (press || (n >= AR_MIN_LAUNCH && now + 2 * AR_TICK >= due && a->wait_due != due)) {   // the attack time: once per attack time
        if (!press) a->wait_due = due;
        ee_unit *t = ar_target(pl, a->home);
        ivec2 d = t ? unit_get_tile(t) : a->home;
        float r = t ? ar_judge(pl, a, 0, d) : 0.0f;
        if (!t || r * 100.0f < (float)AR_GO_PCT) {
            if (press) return;   // told at the attack time
            a->waits++;
            if (t) ar_log(idx, now, "the attack time: the force of %d waits at %d,%d - %d,%d is too strong (%.2f: own %d, enemy %d)", n, a->home.x, a->home.y, d.x, d.y, r, g_ar_own.n, g_sgs_foe.n);
            else ar_log(idx, now, "the attack time: the force of %d waits at %d,%d - no enemy building known", n, a->home.x, a->home.y);
            return;
        }
        if (press) a->presses++; else a->pushes++;
        ar_log(idx, now, "%s: the force of %d from %d,%d at %d,%d (%.2f: own %d, enemy %d; stopped by hp %.0f)", press ? "pressing on" : "the attack time", n, a->home.x, a->home.y, d.x, d.y, r, g_ar_own.n, g_sgs_foe.n, a->stop_hp);
        ar_go(a, 1, d, t, 0, now);
    }
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
    // every free land soldier held - not a member of an attack subgroup (unit_free is no group test: a member passes
    // it, army.c)
    static ee_unit *mem[4096];
    int nm = ou_members(pl, mem, 4096);
    qsort(mem, (size_t)nm, sizeof *mem, ar_ptr_cmp);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it) && a->n < AR_HELD; ) {
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || !ms_land_soldier(u, it.td) || ar_held(a, u) || unit_get_ai_held(u)) continue;
        if (bsearch(&u, mem, (size_t)nm, sizeof *mem, ar_ptr_cmp)) continue;
        if (!combat_mgr_unit_free(cm, unit_get_physics(u)) || landing_has_unit(pl, u) || ou_sortie(pl, u)) continue;
        player_hold_unit(pl, u);
        a->held[a->n].u = u; a->held[a->n].id = unit_get_id(u); a->held[a->n].order_t = 0; a->held[a->n].out = 0; a->n++;
        a->held_all++;
    }
    if (!ar_home(pl, idx, a, now)) return;
    if (a->out) ar_out(pl, idx, a, now);
    ar_at_home(pl, idx, a, now);
    if (g_log && g_autotest && (!a->log_t || now < a->log_t || now - a->log_t >= 60000)) {
        a->log_t = now ? now : 1;
        ar_log(idx, now, "held %d (in all %u), %s%d,%d (judged %.2f), pushes %u (pressing on %u) legs %u raids %u backs %u waits %u, orders %u, offers refused %u, forted %u, next attack at %.1f min",
               a->n, a->held_all, a->out == 1 ? "out at " : a->out == 2 ? "at a raid at " : "home at ", a->out ? a->dest.x : a->home.x, a->out ? a->dest.y : a->home.y,
               a->ratio > 99.0f ? 99.0f : a->ratio, a->pushes, a->presses, a->legs, a->raids, a->backs, a->waits, a->orders, a->refused, a->forted, player_get_next_attack(pl) / 60000.0);
    }
}
internal void armies_new_game(void) { memset(g_armies, 0, sizeof g_armies); }
internal void armies_install(const char *autotest_ini) {
    g_mod_armies = g_set_ai[1].mod_armies;
    if (autotest_ini) {
        g_v_mod_armies = ini_int("autotest", "v_mod_armies", -1, autotest_ini);
        g_v_ar_raid_band = ini_int("autotest", "v_ar_raid_band", -1, autotest_ini);
        g_v_ar_press = ini_int("autotest", "v_ar_press", -1, autotest_ini);
        g_v_ar_fort = ini_int("autotest", "v_ar_fort", -1, autotest_ini);
    }
    if (g_log) fprintf(g_log, "# mod armies: %d (variant %d)\n", g_mod_armies, g_v_mod_armies);
}
