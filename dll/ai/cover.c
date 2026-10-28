// FALL BACK TO COVER (docs/economy.md "Fall back to cover", ships docs/navy.md). Included by eeaimod.c after metal.c
// (mt_line), mass.c (ms_base), fleet.c (fl_attacking), landing.c (landing_has_unit), stats.c (the fight estimate),
// attack.c (threat_at, defence_stats, unit_goal, goal_move, seen_by) and forces.c (area_id, goal_near).
// The playtester: the AI loses expensive units in fights it cannot win, for nothing - a player pulls a lost fight back
// under his towers, sends damaged units to the hospital and damaged ships to the docks, and comes back healed.
// What the game does (measured 2026-10-21, scripts/rt_heal: Middle Ages, the idle P1's units hurt by the pipe): a
// Hospital heals every own land unit within ~4 tiles of its centre at once, 3 hit points a second (a Long Sword
// 100 -> 400 in 100 s; 5 tiles away nothing); a Dock repairs the ships within ~3 tiles, 2 a second each; nothing
// else regains hit points (no regeneration).
//
// Measurement (autotest; the mod's AIs and the vanilla players alike) - every 5 s per player its military units
// (land: an army family, no citizen, no aircraft; sea: warships and submarines; an attack value), each unit's record:
// since when below half its hit points, whether it then had a quiet moment (no enemy fighter in its 8-tile cell or
// the cells around: time to heal), its fight - when it took an attack target after 30 s without one, the estimate of
// that fight (stats.c st_ratio, own and allied fighters within 10 tiles against the enemy's within 10 and the enemy
// defences whose reach covers the spot; all units, not only those seen) and whether an enemy defence covered its
// target; the hit points regained (by an own Hospital / Dock within 5 tiles, elsewhere).
// "# cvd: P<v> at <m> min: <line><type> val <cost> hp <max> | eng <ratio x 100, -1 none in the last 2 min> <s ago> |
// loc <ratio x 100 at the death> | tower <an enemy defence covers the spot> near <own and allied fighters within 6
// tiles> | low <s below half> quiet <0/1> | hosp <tiles to the nearest own Hospital, -1> dock <..Dock> | held <fall_back
// kind> | by P<k>" for every military unit killed (line: i infantry, r ranged, c cavalry, s siege, h hero, o other, w
// warship; val: the type's base price F+W+S+G+I, no cheat). "# cover: P<n> at <m> min:" once a game minute: means over
// the samples of the military units, the damaged (below half), damaged in a quiet moment; the hit points regained by
// the Hospitals, the Docks, elsewhere; Hospitals and Docks standing; what fall_back did.
//
// fall_back ([ai_base], bits; [autotest] v_fall_back for the variant players; per unit, never an attack called back):
// 1 HEAL: a land unit worth fall_back_value (100) or more below fall_back_hp (40%) of its hit points walks to the
//   nearest own Hospital on its land within fall_back_dist (60) tiles (none seen near it) and stays until 90% (4
//   minutes at most) - its attack subgroup's orders refused meanwhile (landing.c hook_asg_give_goal) - then rejoins.
// 2 BREAK: a land unit with enemies seen near (its cell or the cells around) whose fight is clearly lost - the
//   estimate with only what the AI sees (enemy defences known: their reach) below fall_back_ratio (50%) - falls back to
//   cover: the nearest own defence building (towers, fortress) on its land within fall_back_dist tiles that no known
//   enemy defence covers, 2 tiles on the base's side of it; else a Hospital; else the base. Held 20 s after it arrived
//   (90 s at most), then the subgroup has it again. Not while within 4 tiles of the cover already (it fights there).
// 4 SEA (fleet.c): a fleet's retreat and a damaged force go under its towers (the rally, else the haven by a dock)
//   instead of the open-water guard point; a warship below half its hit points (not 35%) breaks off at once (it kept
//   fighting while it had a target), sails to the water by the dock - one an own tower covers counts half the way -
//   and stays until 80% (it sailed again at 35%). (Ordered to the dock's own tile, a land tile, the ships of one
//   Tournament Islands Huge game drove the engine into its broken unit list - 0x628D6F, reproducible with that seed.)
// 8 END (off): a unit that broke off from the same attack group fall_back_breaks (2) times gives it up (cv_end below),
//   one with no own cover near falls back toward home; an attack most of whose units gave up is called off.
// Measured (base AI at hard, 6 maps x both cheats, 40 games a cell; head-to-head): docs/economy.md "Fall back to
// cover" - 7 on by default; 8 no clear gain (docs/economy.md "8 END"), off.
internal int g_fall_back = 7, g_v_fall_back = -1;
internal int g_fb_ratio = 50, g_fb_hp = 40, g_fb_value = 100, g_fb_dist = 60, g_fb_breaks = 2, g_fb_end = 50;
#define FALL_BACK(idx) PV(idx, g_v_fall_back, g_fall_back)
internal int g_v_avoid_lost = -1;   // head-to-head: [autotest] v_avoid_lost_fights for the variant players
#define CV_N 8192
#define CV_TOMB ((ee_unit *)1)
#define CV_HOLD 128
#define CV_MIL 2048
#define CV_CELL 8
#define CV_CELLS 64
struct cv_rec { ee_unit *u; int id, hp; uint32_t seen, low_t, eng_t, att_t; int16_t eng; uint8_t quiet, etower; };
internal struct cv_rec g_cvr[CV_N], g_cvr_tmp[CV_N];
internal unsigned g_cvr_n;
internal uint32_t g_cvr_purge;
// kind 1 at the Hospital, 2 broke off to cover, 3 gave its attack up (END), 4 broke off toward home, no cover near (END);
// g its attack group then (END), home: no cover - the next step home when it arrived
struct cv_hold { ee_unit *u; ee_attack_group *g; int id, kind, home; ivec2 t; uint32_t since, at; };   // t: its cover
struct cv_brk { ee_unit *u; ee_attack_group *g; int id, n; uint32_t t; };   // END: how often a unit broke off from its attack group
internal struct cv_pl {
    uint32_t ts, tb, tscan, tz; unsigned min; int started;
    unsigned samples, mil, dmg, dmgq;
    float heal_h, heal_d, heal_o;
    int nhs, ndk, ncv, base;
    ivec2 b, h[16], dk[16], cv[64];   // the base; own Hospitals, Docks, defences
    struct cv_hold hold[CV_HOLD]; int nh;
    unsigned heal_sent, heal_done, broke, back, refused, judged;   // since the last line
    struct cv_brk brk[256];
    uint32_t end_t;
    unsigned gaveup, home, ended;   // since the last line (END)
} g_cv[16];
internal uint8_t g_cv_foe[CV_CELLS][CV_CELLS], g_cv_seen[CV_CELLS][CV_CELLS];   // enemy fighters per 8-tile cell (all / seen)
internal struct st_side g_cvs_own, g_cvs_foe;
internal void cover_new_game(void) { memset(g_cvr, 0, sizeof g_cvr); g_cvr_n = 0; g_cvr_purge = 0; memset(g_cv, 0, sizeof g_cv); }

internal unsigned cv_hash(ee_unit *u) { return ((uint32_t)(uintptr_t)u >> 3) * 2654435761u >> 19; }
internal struct cv_rec *cv_find(ee_unit *u, int add) {
    unsigned h = cv_hash(u) & (CV_N - 1);
    int id = unit_get_id(u);
    struct cv_rec *free_r = NULL;
    for (unsigned k = 0; k < CV_N; k++, h = (h + 1) & (CV_N - 1)) {
        struct cv_rec *r = &g_cvr[h];
        if (r->u == u) {
            if (r->id == id) return r;
            if (!add) return NULL;
            memset(r, 0, sizeof *r); r->u = u; r->id = id; r->eng = -1;   // a new unit at a dead one's address
            return r;
        }
        if (r->u == CV_TOMB) { if (!free_r) free_r = r; continue; }
        if (!r->u) {
            if (!add) return NULL;
            if (!free_r) { if (g_cvr_n >= CV_N * 3 / 4) return NULL; g_cvr_n++; free_r = r; }
            memset(free_r, 0, sizeof *free_r); free_r->u = u; free_r->id = id; free_r->eng = -1;
            return free_r;
        }
    }
    return NULL;
}
internal void cv_purge(uint32_t now) {   // once a minute: the records seen in the last 30 s, no tombstones
    if (g_cvr_purge && now >= g_cvr_purge && now - g_cvr_purge < 60000) return;
    g_cvr_purge = now ? now : 1;
    int n = 0;
    for (int k = 0; k < CV_N; k++)
        if (g_cvr[k].u && g_cvr[k].u != CV_TOMB && now >= g_cvr[k].seen && now - g_cvr[k].seen < 30000) g_cvr_tmp[n++] = g_cvr[k];
    memset(g_cvr, 0, sizeof g_cvr); g_cvr_n = 0;
    for (int i = 0; i < n; i++) {
        unsigned h = cv_hash(g_cvr_tmp[i].u) & (CV_N - 1);
        while (g_cvr[h].u) h = (h + 1) & (CV_N - 1);
        g_cvr[h] = g_cvr_tmp[i]; g_cvr_n++;
    }
}

// 1 a land fighter, 2 a warship / submarine, 0 else (citizens, aircraft, transports, priests, buildings)
internal int cv_mil(ee_unit_type *td) {
    int fam = unit_type_get_class_id(td), t = unit_type_get_id(td);
    if (!is_army_family(fam) || is_citizen_type(t) || unit_type_get_attack(td) <= 0) return 0;
    if (fam == CLS_AIRPLANE || fam == CLS_HELICOPTER || fam == CLS_HELICOPTER_TRANSPORT || fam == CLS_NAVAL_TRANSPORT || fam == CLS_NAVAL_BUILDING || fam == CLS_NAVAL_CITIZEN) return 0;
    if (fam == CLS_NAVAL || fam == CLS_SUBMERGED_NAVAL) return 2;
    return 1;
}
internal int cv_alive(ee_unit *u) { return unit_get_hp(u) > 0 && unit_get_x(u) >= 0; }
internal int cv_built(ee_unit *u) { ee_unit *bs = unit_get_physics(u); return bs && unit_get_finished(bs); }
internal int cv_value(ee_player *pl, int type) {   // the base price, F+W+S+G+I (no cheat)
    ee_unit_type *obj = type_data(type);
    ee_tech_tree *tt = pl ? player_get_tech_tree(pl) : NULL;
    ee_tech *tech = obj && tt ? tech_tree_lookup(tt, unit_type_get_tech(obj)) : NULL;
    ee_tech_data *data = tech ? tech_get_data(tech) : NULL;
    int v = 0;
    for (int r = 2; data && r < 7; r++) v += tech_data_get_cost(data, r);
    return v;
}
internal int cv_hpmax(ee_unit *u) { return st_maxhp(unit_get_type_data(u), unit_get_stats(u)); }
// an enemy fighter in p's 8-tile cell or one around it (seen: only those pl sees)
internal int cv_foe_near(ivec2 p, int seen) {
    int cx = p.x / CV_CELL, cy = p.y / CV_CELL;
    for (int a = cx - 1; a <= cx + 1; a++)
        for (int b = cy - 1; b <= cy + 1; b++)
            if (a >= 0 && b >= 0 && a < CV_CELLS && b < CV_CELLS && (seen ? g_cv_seen[a][b] : g_cv_foe[a][b])) return 1;
    return 0;
}
// THE ENEMY pl REMEMBERS near p (honest_vision; intel.c): its soldiers and siege seen within the last 3 minutes and
// not at the last look, at the tile where they were seen, within r tiles of p - or, with reach, whose range + 3 covers
// p (20 tiles at most). A player knows the army he saw a minute ago is still there; before 2026-10-26 the fight
// estimates counted only the enemies in sight at that moment: on the player's save "standing army" the AI went at his
// army judged 4:1 for it with 1 of the 81 enemies within 20 tiles in sight (64 remembered), saw the artillery once in
// it and broke off from inside its reach. Their longest range into *rg (may be NULL).
// [ai_base] fight_memory (default 1 since 2026-10-26; [autotest] v_fight_memory for the variant players). Measured
// (docs/testing.md fight_memory): on the player's save P6's and P7's units lost to his army 133 -> 79 a run (6 runs
// each); head-to-head (24 games) no cost - losses -6.0 (se 7.0), workers +7.3 (5.9), the rest within 1.
#define CV_MEMORY 180000
internal int g_fight_mem = 1, g_v_fight_mem = -1;
#define FIGHT_MEM(idx) PV(idx, g_v_fight_mem, g_fight_mem)
internal void cv_foe_mem(ee_player *pl, struct st_side *foe, ivec2 p, int r, int reach, float *rg) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct intel *in = &g_intel[idx];
    if (!g_honest_vision || !FIGHT_MEM(idx)) return;
    for (int k = 0; k < in->n; k++) {
        struct intel_rec *e = &in->r[k];
        if (e->kind != 1 || e->x < 0 || e->t == in->last || in->now - e->t > CV_MEMORY || e->owner <= 0) continue;
        void *o = st_player(e->owner);
        ee_unit_type *td = type_data(e->type);
        if (!o || !td || !is_enemy(pl, o) || !cv_mil(td)) continue;
        ee_unit_stats *st = st_rec(o, e->type);
        float range = st ? unit_stats_get_range(st) : unit_type_get_range(td), rr = (float)r;
        if (range >= 30) range = 0;
        if (reach && range + 3.0f > rr) rr = range + 3.0f > 20.0f ? 20.0f : range + 3.0f;
        if ((float)ivec2_dist2(iv2(e->x, e->y), p) > rr * rr) continue;
        if (st_side_add_rec(foe, e->type, e->owner, e->hp, e->x, e->y) && rg && range > *rg) *rg = range;
    }
}
// [ai_base] threat_memory (default 1 since 2026-10-27, docs/testing.md; [autotest] v_threat_memory for the variant players; with
// honest_vision): the citizens' safety asks for the enemy soldiers it remembers too - attack.c raiders_known (the
// builders' and gatherers' sites, the builders' orders, the way) and the danger map's threats (danger_refresh: the
// citizens' routes). On the player's save
// "standing army" P7's citizens, fetched home, were sent to build 26 tiles out and killed by Marines 8-9 tiles off that
// the safety, asking only for the enemies in sight at that moment, never counted. A remembered soldier counts at the
// tile where it was seen, for TM_MEMORY (a minute) - until an eye of the AI looks there and it is gone (intel.c moves
// it to nowhere). On the save the citizens killed 883 -> 593 (6 runs); head-to-head mirrored neutral (48 games).
#define TM_MEMORY 60000   // a remembered soldier counts for the citizens a minute
internal int g_threat_mem = 1, g_v_threat_mem = -1;
#define THREAT_MEM(idx) PV(idx, g_v_threat_mem, g_threat_mem)
// a remembered enemy soldier whose range + RAID_MARGIN (at least r0, at most rmax) covers p
internal int cv_mem_raider(ee_player *pl, ivec2 p, int r0, int rmax) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct intel *in = &g_intel[idx];
    if (!g_honest_vision || !THREAT_MEM(idx)) return 0;
    for (int k = 0; k < in->n; k++) {
        struct intel_rec *e = &in->r[k];
        if (e->kind != 1 || e->x < 0 || e->t == in->last || in->now - e->t > TM_MEMORY || e->owner <= 0) continue;
        if (abs(e->x - p.x) > rmax || abs(e->y - p.y) > rmax) continue;
        void *o = st_player(e->owner);
        ee_unit_type *td = type_data(e->type);
        if (!o || !td || !is_enemy(pl, o) || unit_type_get_attack(td) <= 0) continue;
        ee_unit_stats *st = st_rec(o, e->type);
        float rng = st ? unit_stats_get_range(st) : unit_type_get_range(td);
        int reach = rng > 0.0f && rng < 30.0f ? (int)(rng + 0.5f) + RAID_MARGIN : 0;
        if (reach > rmax) reach = rmax;
        if (reach < r0) reach = r0;
        if (abs(e->x - p.x) <= reach && abs(e->y - p.y) <= reach) return 1;
    }
    return 0;
}
// the remembered enemy soldiers into the danger map's threats (attack.c danger_refresh), at their reach as the seen ones
internal void cv_mem_threats(ee_player *pl, unsigned pi) {
    struct intel *in = &g_intel[pi & 15];
    if (!g_honest_vision || !THREAT_MEM(pi)) return;
    for (int k = 0; k < in->n; k++) {
        struct intel_rec *e = &in->r[k];
        if (e->kind != 1 || e->x < 0 || e->t == in->last || in->now - e->t > TM_MEMORY || e->owner <= 0) continue;
        void *o = st_player(e->owner);
        ee_unit_type *td = type_data(e->type);
        if (!o || !td || !is_enemy(pl, o) || unit_type_get_attack(td) <= 0) continue;
        ee_unit_stats *st = st_rec(o, e->type);
        float rng = st ? unit_stats_get_range(st) : unit_type_get_range(td);
        float reach = rng > 0.0f && rng < 30.0f ? rng + RAID_MARGIN : RAID_MARGIN;
        if (reach > RAID_FAR) reach = RAID_FAR;
        danger_disc(pi, DANGER_THREATS, (float)e->x + 0.5f, (float)e->y + 0.5f, reach);
    }
}
// the cells with enemy soldiers pl remembers (as cv_foe_mem) into g_cv_seen - judged as those seen
internal void cv_mark_mem(ee_player *pl) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct intel *in = &g_intel[idx];
    if (!g_honest_vision || !FIGHT_MEM(idx)) return;
    for (int k = 0; k < in->n; k++) {
        struct intel_rec *e = &in->r[k];
        if (e->kind != 1 || e->x < 0 || e->t == in->last || in->now - e->t > CV_MEMORY || e->owner <= 0) continue;
        void *o = st_player(e->owner);
        int cx = e->x / CV_CELL, cy = e->y / CV_CELL;
        if (o && is_enemy(pl, o) && cx < CV_CELLS && cy < CV_CELLS) g_cv_seen[cx][cy] = 1;
    }
}

// THE LOCAL FIGHT at p for pl: own and allied fighters within r against the enemy's within r, defences on both
// sides whose reach (+1) covers the spot; honest: only the enemies pl sees. st_ratio: above 1 own stronger (the square
// root of the time ratio), 1e6 nothing to fight. Honest: the enemies it remembers there too (cv_foe_mem)
internal float cv_local(ee_player *pl, ivec2 p, int r, int honest) {
    st_side_clear(&g_cvs_own); st_side_clear(&g_cvs_foe);
    for (struct grid_it it = grid_it_box(p.x, p.y, r + 10, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !cv_alive(u)) continue;
        int fam = unit_type_get_class_id(td), d2 = ivec2_dist2(unit_get_tile(u), p);
        float rg, at;
        if (fam == CLS_BUILDING || fam == CLS_FORTRESS) {
            if (!defence_stats(u, &rg, &at) || at <= 0 || !cv_built(u) || (float)d2 > (rg + 1.0f) * (rg + 1.0f)) continue;
        } else if (!cv_mil(td) || d2 > r * r) continue;
        int foe = is_enemy(pl, o);
        if (foe && honest && !seen_by(u, pl)) continue;
        st_side_add(foe ? &g_cvs_foe : &g_cvs_own, u);
    }
    if (honest) cv_foe_mem(pl, &g_cvs_foe, p, r, 0, NULL);
    return st_ratio(&g_cvs_own, &g_cvs_foe);
}
// LOST FIGHTS ([ai_base] avoid_lost_fights, with danger_routes; game/danger.c's DANGER_LOST - the player: an AI fed its soldiers in packs of 2-4 into
// his standing army of artillery and ranged units, 150 lost and none killed in 11 minutes on his save "standing army",
// on their way to its attack subgroups' marching points): every 4 s each 8-tile cell with enemy fighters pl sees is
// judged - the enemy within CV_ZONE_FOE of its middle (seen; their defences too) against pl's own and allied fighters
// out to the zone's edge + 4, so those waiting at the edge count - and where pl's side is below 1 (not winning: the
// estimate is optimistic for units spread out), the tiles within CV_ZONE_FOE + the enemy's longest reach there (at
// most 15) are a lost fight. Units gathered at the edge open it once they would win: they go in together.
// Measured (docs/testing.md avoid_lost_fights): on the player's save 22% fewer units lost to his army, head-to-head
// razed +7.0 (se 3.0) and kills +17.9 a game.
#define CV_ZONE_FOE 10
#define CV_ZONE_REACH 15
internal float cv_zone(ee_player *pl, ivec2 p, float *radius) {
    st_side_clear(&g_cvs_own); st_side_clear(&g_cvs_foe);
    float reach = 0;
    for (struct grid_it it = grid_it_box(p.x, p.y, CV_ZONE_FOE + 14, 1); grid_it_next(&it); ) {   // the enemy (defences: their reach)
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !cv_alive(u) || !is_enemy(pl, o) || !seen_by(u, pl)) continue;
        int fam = unit_type_get_class_id(it.td), d2 = ivec2_dist2(unit_get_tile(u), p);
        float rg = 0, at;
        if (fam == CLS_BUILDING || fam == CLS_FORTRESS) {
            if (!defence_stats(u, &rg, &at) || at <= 0 || !cv_built(u) || (float)d2 > (rg + 1.0f) * (rg + 1.0f)) continue;
        } else {
            if (!cv_mil(it.td) || d2 > CV_ZONE_FOE * CV_ZONE_FOE) continue;
            ee_unit_stats *st = unit_get_stats(u);
            rg = st ? unit_stats_get_range(st) : unit_type_get_range(it.td);
        }
        if (rg > reach && rg < 30) reach = rg;
        st_side_add(&g_cvs_foe, u);
    }
    cv_foe_mem(pl, &g_cvs_foe, p, CV_ZONE_FOE, 0, &reach);
    if (reach > CV_ZONE_REACH) reach = CV_ZONE_REACH;
    *radius = CV_ZONE_FOE + reach;
    int ro = (int)*radius + 4;
    for (struct grid_it it = grid_it_box(p.x, p.y, ro, 1); grid_it_next(&it); ) {   // own and allied, to the edge + 4
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !cv_alive(u) || is_enemy(pl, o)) continue;
        int fam = unit_type_get_class_id(it.td), d2 = ivec2_dist2(unit_get_tile(u), p);
        float rg, at;
        if (fam == CLS_BUILDING || fam == CLS_FORTRESS) {
            if (!defence_stats(u, &rg, &at) || at <= 0 || !cv_built(u) || (float)d2 > (rg + 1.0f) * (rg + 1.0f)) continue;
        } else if (!cv_mil(it.td) || d2 > ro * ro) continue;
        st_side_add(&g_cvs_own, u);
    }
    return st_ratio(&g_cvs_own, &g_cvs_foe);
}
internal void cv_zones(ee_player *pl, unsigned idx, uint32_t now) {
    ee_map *map = game_map();
    if (!map || !danger_begin(idx, DANGER_LOST, (int)map_get_size(map), now)) return;
    for (int cx = 0; cx < CV_CELLS; cx++)
        for (int cy = 0; cy < CV_CELLS; cy++) {
            if (!g_cv_seen[cx][cy]) continue;
            float r;
            ivec2 p = iv2(cx * CV_CELL + CV_CELL / 2, cy * CV_CELL + CV_CELL / 2);
            if (cv_zone(pl, p, &r) < 1.0f) danger_disc(idx, DANGER_LOST, (float)p.x, (float)p.y, r);
        }
}

// landing.c hook_asg_give_goal: an attack subgroup's march to a point in a lost fight ends at the last point outside on
// the straight way there (1 tile short); a unit inside it already walks out instead, away from that point (the first
// point outside within 30 tiles, else as it was). The unit's route goes round the lost fights
internal void cover_hold_leg(ee_attack_subgroup *sg, ee_unit *u, ee_goal *goal) {
    ee_player *pl = attack_subgroup_get_player(sg);
    ee_map *map = game_map();
    if (!g_danger_routes || !goal || !goal_is_location(goal) || !pl || !map || !u || ou_sortie(pl, u)) return;   // army.c: outdated units go on
    int size = (int)map_get_size(map);
    const uint8_t *lz = danger_map((unsigned)player_get_index(pl), DANGER_LOST, size);
    if (!lz) return;
    float tx = goal_get_dest_x(goal), ty = goal_get_dest_y(goal);
    vec3 q = unit_get_pos(u);
#define CV_LOST(x, y) ((int)(x) >= 0 && (int)(y) >= 0 && (int)(x) < size && (int)(y) < size && lz[(int)(y) * size + (int)(x)])
    if (!CV_LOST(tx, ty)) return;
    float dx = tx - q.x, dy = ty - q.y, l = sqrtf(dx * dx + dy * dy), out = -1;
    if (l < 1) return;
    if (CV_LOST(q.x, q.y)) {   // inside already: out, away from the point
        for (float s = 0.5f; s <= 30; s += 0.5f)
            if (!CV_LOST(q.x - dx / l * s, q.y - dy / l * s)) { out = -(s + 1); break; }
    } else {
        for (float s = 0; s <= l; s += 0.5f) {
            if (CV_LOST(q.x + dx / l * s, q.y + dy / l * s)) break;
            out = s;
        }
        if (out >= 0) out = out > 1 ? out - 1 : 0;
    }
#undef CV_LOST
    if (out == -1) return;
    goal_set_dest_x(goal, q.x + dx / l * out);
    goal_set_dest_y(goal, q.y + dy / l * out);
    g_dr_legs_held++;
}

// player pi's danger layer k at p (0: none, or the layer not fresh)
internal int cv_danger(unsigned pi, int k, ivec2 p) {
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0;
    const uint8_t *lv = size ? danger_map(pi, k, size) : NULL;
    return lv && p.x >= 0 && p.y >= 0 && p.x < size && p.y < size ? lv[p.y * size + p.x] : 0;
}

// an enemy defence covers p (its range + 1; all, not only the known ones)
internal int cv_enemy_def(ee_player *pl, ivec2 p) {
    for (struct grid_it it = grid_it_box(p.x, p.y, 12, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        int fam = unit_type_get_class_id(td);
        float rg, at;
        if ((fam != CLS_BUILDING && fam != CLS_FORTRESS) || !cv_alive(u) || !is_enemy(pl, unit_get_owner(u)) || !defence_stats(u, &rg, &at) || at <= 0 || !cv_built(u)) continue;
        if ((float)ivec2_dist2(unit_get_tile(u), p) <= (rg + 1.0f) * (rg + 1.0f)) return 1;
    }
    return 0;
}
internal int cv_nearest(const ivec2 *ps, int n, ivec2 p) {   // tiles to the nearest, -1 none
    int b = -1;
    for (int k = 0; k < n; k++) { int d = ivec2_dist2(ps[k], p); if (b < 0 || d < b) b = d; }
    return b < 0 ? -1 : (int)sqrtf((float)b);
}
internal int cv_held(struct cv_pl *c, ee_unit *u) {
    for (int k = 0; k < c->nh; k++) if (c->hold[k].u == u) return k + 1;
    return 0;
}

// THE SCAN: own Hospitals, Docks and defences (finished), the enemy fighters by cell, and the own military units
internal ee_unit *g_cv_units[CV_MIL];
internal int g_cv_nu;
internal void cv_scan(ee_player *pl, struct cv_pl *c) {
    c->nhs = c->ndk = c->ncv = 0; g_cv_nu = 0;
    memset(g_cv_foe, 0, sizeof g_cv_foe); memset(g_cv_seen, 0, sizeof g_cv_seen);
    c->base = ms_base(pl, &c->b);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !cv_alive(u)) continue;
        int fam = unit_type_get_class_id(td), t = unit_type_get_id(td);
        ivec2 p = unit_get_tile(u);
        if (o == pl) {
            if (fam == CLS_BUILDING || fam == CLS_FORTRESS) {
                float rg, at;
                if (!cv_built(u)) continue;
                if (t == UT(B_HOSPITAL) && c->nhs < 16) c->h[c->nhs++] = p;
                else if (t == UT(B_DOCK) && c->ndk < 16) c->dk[c->ndk++] = p;
                else if (defence_stats(u, &rg, &at) && at > 0 && c->ncv < 64) c->cv[c->ncv++] = p;
            } else if (cv_mil(td) && g_cv_nu < CV_MIL) g_cv_units[g_cv_nu++] = u;
            continue;
        }
        if (!is_enemy(pl, o)) continue;
        float rg, at;
        if (!cv_mil(td) && !((fam == CLS_BUILDING || fam == CLS_FORTRESS) && defence_stats(u, &rg, &at) && at > 0)) continue;
        int cx = p.x / CV_CELL, cy = p.y / CV_CELL;
        if (cx < 0 || cy < 0 || cx >= CV_CELLS || cy >= CV_CELLS) continue;
        g_cv_foe[cx][cy] = 1;
        if (seen_by(u, pl)) g_cv_seen[cx][cy] = 1;
    }
    cv_mark_mem(pl);
}

// the measurement's sample (every 5 s, autotest)
internal void cv_sample(ee_player *pl, struct cv_pl *c, uint32_t now) {
    static struct { int cx, cy; float r; } cache[256];
    int nc = 0;
    c->samples++;
    for (int i = 0; i < g_cv_nu; i++) {
        ee_unit *u = g_cv_units[i];
        struct cv_rec *r = cv_find(u, 1);
        if (!r) continue;
        int hp = unit_get_hp(u), mx = cv_hpmax(u);
        ivec2 p = unit_get_tile(u);
        int land = cv_mil(unit_get_type_data(u)) == 1;
        if (r->seen && hp > r->hp) {   // regained: by an own Hospital / Dock within 5 tiles, else elsewhere
            int dh = cv_nearest(c->h, c->nhs, p), dd = cv_nearest(c->dk, c->ndk, p);
            if (land && dh >= 0 && dh <= 5) c->heal_h += (float)(hp - r->hp);
            else if (!land && dd >= 0 && dd <= 5) c->heal_d += (float)(hp - r->hp);
            else c->heal_o += (float)(hp - r->hp);
        }
        r->hp = hp; r->seen = now ? now : 1;
        c->mil++;
        if (mx > 0 && hp * 2 < mx) {
            if (!r->low_t) r->low_t = now ? now : 1;
            c->dmg++;
            if (!cv_foe_near(p, 0)) { r->quiet = 1; c->dmgq++; }
        } else { r->low_t = 0; r->quiet = 0; }
        if (fl_attacking(u)) {
            if (!r->att_t || now - r->att_t > 30000) {   // a new fight: its estimate (cached per 4-tile cell this sample)
                int k = 0;
                while (k < nc && (cache[k].cx != p.x / 4 || cache[k].cy != p.y / 4)) k++;
                if (k == nc && nc < 256) { cache[nc].cx = p.x / 4; cache[nc].cy = p.y / 4; cache[nc].r = cv_local(pl, p, 10, 0); nc++; }
                float e = k < 256 ? cache[k].r : 1.0f;
                r->eng = (int16_t)(e > 99.0f ? 9900 : (int)(e * 100.0f + 0.5f));
                r->eng_t = now ? now : 1;
                ee_unit *t = fl_target_of(u);
                r->etower = t ? (uint8_t)cv_enemy_def(pl, unit_get_tile(t)) : 0;
            }
            r->att_t = now ? now : 1;
        }
    }
}

// a military unit killed (diplo.c hook_kill_stat, n 3; k the killer's index; autotest)
internal void cover_death(ee_unit *dead, int k) {
    if (!g_autotest || !g_log || !dead) return;
    ee_unit_type *td = unit_get_type_data(dead);
    ee_player *pl = unit_get_owner(dead);
    if (!td || !pl || unit_get_x(dead) < 0) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    int kind = cv_mil(td);
    if (!kind || !(g_players[idx] || is_vanilla_idx((int)idx))) return;
    struct cv_pl *c = &g_cv[idx];
    struct cv_rec *r = cv_find(dead, 0);
    uint32_t now = game_time();
    int t = unit_type_get_id(td), nb = 0;
    ivec2 p = unit_get_tile(dead);
    char line = 'o';
    if (kind == 2) line = 'w';
    else if (unit_type_get_family(td) == KIND_HERO) line = 'h';
    else switch (mt_line(pl, t)) { case MB_INF: line = 'i'; break; case MB_RNG: line = 'r'; break; case MB_CAV: line = 'c'; break; case MB_SIE: line = 's'; break; default: break; }
    for (struct grid_it it = grid_it_box(p.x, p.y, 6, 1); grid_it_next(&it); ) {
        ee_player *o = unit_get_owner(it.u);
        if (it.u != dead && o && player_get_index(o) > 0 && !is_enemy(pl, o) && cv_alive(it.u) && cv_mil(it.td) &&
            ivec2_dist2(unit_get_tile(it.u), p) <= 36) nb++;
    }
    float loc = cv_local(pl, p, 10, 0);
    int eng = r && r->eng_t && now >= r->eng_t && now - r->eng_t < 120000 ? r->eng : -1;
    int h = cv_held(c, dead);
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# cvd: P%u at %.1f min: %c%d val %d hp %d | eng %d %.0f | loc %d | tower %d near %d | low %.0f quiet %d | hosp %d dock %d | held %d | by P%d\n",
            idx, now / 60000.0, line, t, cv_value(pl, t), cv_hpmax(dead), eng, eng >= 0 ? (now - r->eng_t) / 1000.0 : 0.0,
            loc > 99.0f ? 9900 : (int)(loc * 100.0f + 0.5f), cv_enemy_def(pl, p), nb,
            r && r->low_t && now >= r->low_t ? (now - r->low_t) / 1000.0 : 0.0, r ? r->quiet : 0,
            cv_nearest(c->h, c->nhs, p), cv_nearest(c->dk, c->ndk, p), h ? c->hold[h - 1].kind : 0, k);
    LeaveCriticalSection(&g_lock);
    if (r) r->u = CV_TOMB;
}

// ---- fall_back ----
// landing.c hook_asg_give_goal: an attack subgroup's order to a unit held here
internal int cover_refuses(ee_attack_subgroup *sg, ee_unit *u) {
    ee_player *pl = attack_subgroup_get_player(sg);
    if (!pl) return 0;
    struct cv_pl *c = &g_cv[(unsigned)player_get_index(pl) & 15];
    if (!c->nh || !cv_held(c, u)) return 0;
    c->refused++;
    return 1;
}
internal int cover_sea(unsigned idx) { return (FALL_BACK(idx & 15) & 4) != 0; }   // fleet.c
// the cover *t for a unit at p on land area a: the nearest own defence no known enemy defence covers (2 tiles on the
// base's side of it), else a Hospital, else the base; 0 none within reach
internal int cv_cover(ee_player *pl, struct cv_pl *c, ivec2 p, unsigned a, ivec2 *t) {
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    int best = -1, bd = g_fb_dist * g_fb_dist + 1;
    threat_refresh(pl, game_time());
    for (int k = 0; k < c->ncv; k++) {
        ivec2 q = c->cv[k];
        int d = ivec2_dist2(q, p);
        if (d < bd && area_id(q, 2) == a && !(threat_at(pi, q) > 0)) { bd = d; best = k; }
    }
    if (best >= 0) {
        ivec2 q = c->cv[best];
        float dx = (float)(c->b.x - q.x), dy = (float)(c->b.y - q.y), l = sqrtf(dx * dx + dy * dy);
        *t = iv2(q.x + (c->base && l >= 1 ? (int)lroundf(2.0f * dx / l) : 0), q.y + (c->base && l >= 1 ? (int)lroundf(2.0f * dy / l) : 0));
        return 1;
    }
    for (int k = 0; k < c->nhs; k++) {
        int d = ivec2_dist2(c->h[k], p);
        if (d < bd && area_id(c->h[k], 2) == a) { bd = d; *t = iv2(c->h[k].x, c->h[k].y + 2); best = k; }
    }
    if (best >= 0) return 1;
    if (c->base && area_id(c->b, 2) == a && ivec2_dist2(c->b, p) <= g_fb_dist * g_fb_dist) { *t = c->b; return 1; }
    return 0;
}
internal void cv_take(struct cv_pl *c, ee_unit *u, int kind, ivec2 t, uint32_t now, ee_attack_group *g, int home) {
    if (c->nh >= CV_HOLD) return;
    struct cv_hold *h = &c->hold[c->nh++];
    h->u = u; h->g = g; h->id = unit_get_id(u); h->kind = kind; h->t = t; h->home = home;
    h->since = now ? now : 1; h->at = 0;
}

// ---- 8 END: an attack the units keep breaking off from ----
// The gap BREAK left: an attack subgroup sends a unit back into the same lost fight 20 s after it reached the cover,
// again and again; a unit far from any own cover (deep in enemy land, on an enemy island) fights on. A player gives a
// hopeless attack up: those who broke off from it fall_back_breaks (2) times stay back, and when fall_back_end (50%)
// of an attack's units have given up and its fighters still out are not winning where they stand, the attack is called
// off (the game's call-off at the safe point, diplo.c - the engine guard of attack.c with it) and
// the next one comes within 5 minutes, formed fresh. The attack groups: attack commander player +0xB8C +0x48, a deque
// of groups at +4 (count +0x14), a group's target player +0x48, its subgroups +8..+0xC, members +0x54..+0x58, record
// +0x44 = unit (as defence.c asg_units reads them).
#define CV_GRP 8
#define CV_GH 4096
internal struct cv_grp { ee_attack_group *g; ee_player *target; int n, gu, nf; float fx, fy, fpow; } g_cvg[CV_GRP];
internal int g_cvg_n;
internal struct { ee_unit *u; int k; } g_cvgh[CV_GH];
internal int cv_group_of(ee_unit *u) {   // the attack group (index into g_cvg) a unit is a member of, -1 none
    unsigned h = cv_hash(u) & (CV_GH - 1);
    for (int n = 0; n < CV_GH && g_cvgh[h].u; n++, h = (h + 1) & (CV_GH - 1)) if (g_cvgh[h].u == u) return g_cvgh[h].k;
    return -1;
}
internal void cv_groups(ee_player *pl) {
    memset(g_cvgh, 0, sizeof g_cvgh); g_cvg_n = 0;
    ee_combat_mgr *cbm = player_get_combat_mgr(pl); ee_attack_cmd *cmd = cbm ? combat_mgr_get_commander(cbm) : NULL;
    if (!cmd) return;
    uint8_t *dq = attack_cmd_groups_at(cmd);
    unsigned first = deque_front(dq), cnt = deque_count(dq), used = 0;
    for (unsigned k = 0; k < cnt && g_cvg_n < CV_GRP; k++) {
        ee_attack_group *g = deque_at(dq, first + k);
        if (!g) continue;
        struct cv_grp *q = &g_cvg[g_cvg_n];
        memset(q, 0, sizeof *q); q->g = g; q->target = attack_group_get_target(g);
        for (ee_attack_subgroup **sp = attack_group_get_subgroups_begin(g); sp && sp < attack_group_get_subgroups_end(g); sp++) {
            ee_asg_unit **b = *sp ? attack_subgroup_get_units_begin(*sp) : NULL; ee_asg_unit **e = *sp ? attack_subgroup_get_units_end(*sp) : NULL;
            for (ee_asg_unit **r = b; b && r < e && used < CV_GH / 2; r++) {
                ee_unit *u = *r ? asg_unit_get_unit(*r) : NULL;
                if (!u || !cv_alive(u) || cv_mil(unit_get_type_data(u)) != 1) continue;
                unsigned h = cv_hash(u) & (CV_GH - 1);
                while (g_cvgh[h].u && g_cvgh[h].u != u) h = (h + 1) & (CV_GH - 1);
                if (g_cvgh[h].u) continue;
                g_cvgh[h].u = u; g_cvgh[h].k = g_cvg_n; used++;
                q->n++;
            }
        }
        g_cvg_n++;
    }
}
// how often u broke off from its attack group g (this one counted); a record older than 10 minutes starts again
internal int cv_tally(struct cv_pl *c, ee_unit *u, ee_attack_group *g, uint32_t now) {
    int id = unit_get_id(u), old = 0;
    for (int k = 0; k < 256; k++) {
        struct cv_brk *b = &c->brk[k];
        if (b->u == u && b->id == id) {
            if (b->g != g || now - b->t > 600000) { b->g = g; b->n = 0; }
            b->t = now ? now : 1;
            return ++b->n;
        }
        if (!b->u) { if (c->brk[old].u) old = k; }   // a free record first, else the oldest
        else if (c->brk[old].u && b->t < c->brk[old].t) old = k;
    }
    struct cv_brk *b = &c->brk[old];
    b->u = u; b->g = g; b->id = id; b->n = 1; b->t = now ? now : 1;
    return 1;
}
// no own cover within reach: toward the base - the farthest point up to 20 tiles on the line to it that is still the
// unit's land (on an enemy island: the shore on the home side); 0 none 8+ tiles away
internal int cv_home_pt(struct cv_pl *c, ivec2 p, unsigned a, ivec2 *t) {
    if (!c->base) return 0;
    float dx = (float)(c->b.x - p.x), dy = (float)(c->b.y - p.y), l = sqrtf(dx * dx + dy * dy);
    int best = 0;
    for (int s = 2; s <= 20 && (float)s < l; s += 2) {
        ivec2 q = iv2(p.x + (int)lroundf(dx * (float)s / l), p.y + (int)lroundf(dy * (float)s / l));
        if (area_id(q, 2) != a) break;
        *t = q; best = s;
    }
    return best >= 8;
}
// the call-off: most of an attack's units gave up, and the rest is not winning where it stands
internal void cv_end(ee_player *pl, unsigned idx, struct cv_pl *c, uint32_t now) {
    if (c->end_t && now - c->end_t < 60000) return;
    for (int k = 0; k < c->nh; k++) {
        struct cv_hold *h = &c->hold[k];
        for (int j = 0; j < g_cvg_n; j++) if (h->kind == 3 && h->g == g_cvg[j].g) g_cvg[j].gu++;
    }
    for (int i = 0; i < g_cv_nu; i++) {   // the fighters still out: members not held
        ee_unit *u = g_cv_units[i];
        int j = cv_group_of(u);
        if (j < 0 || cv_held(c, u)) continue;
        struct cv_grp *q = &g_cvg[j];
        float p = unit_power(u);
        q->nf++; q->fpow += p; q->fx += (float)unit_get_x(u) * p; q->fy += (float)unit_get_y(u) * p;
    }
    if (landing_busy(pl)) return;   // its soldiers in the groups, aboard: at tile -10000 they look lost
    for (int j = 0; j < g_cvg_n; j++) {
        struct cv_grp *q = &g_cvg[j];
        int all = q->n > q->gu ? q->n : q->gu;
        if (!q->target || q->gu < 3 || q->gu * 100 < all * g_fb_end) continue;   // 3+ gave up: a real attack
        float e = 0;
        if (q->nf && q->fpow > 0) {   // never an attack winning where its fighters stand
            e = cv_local(pl, iv2((int)(q->fx / q->fpow), (int)(q->fy / q->fpow)), 10, 1);
            if (e >= 1.0f) continue;
        }
        int stronger = 0;   // the call-off ends all attacks on that player: not while another one on it fights on stronger
        for (int i = 0; i < g_cvg_n; i++) stronger |= i != j && g_cvg[i].target == q->target && g_cvg[i].fpow > q->fpow && g_cvg[i].gu * 100 < g_cvg[i].n * g_fb_end;
        if (stronger) continue;
        int t = player_get_index(q->target) & 15;
        g_cancel_why[idx][t] = "attack (fall back: given up)";
        cancel_citizen_attack((int)idx, t);
        c->end_t = now ? now : 1; c->ended++;
        uint32_t next = now + 300000;
        if (player_get_next_attack(pl) > next) { player_set_next_attack(pl, next); g_last_attack_time[idx] = next; }
        if (g_log && g_autotest) fprintf(g_log, "# cover: P%u at %.1f min: its attack on P%d ends - %d of %d units gave up, %d still out (fight %.2f); next attack at %.1f min\n",
                                         idx, now / 60000.0, t, q->gu, all, q->nf, q->nf ? e : 0.0f, player_get_next_attack(pl) / 60000.0);
        return;
    }
}
internal void cv_behave(ee_player *pl, unsigned idx, struct cv_pl *c, uint32_t now, int plan) {
    if (plan & 8) cv_groups(pl); else g_cvg_n = 0;
    // the held units: gone, healed, arrived and rested - let go
    for (int k = c->nh - 1; k >= 0; k--) {
        struct cv_hold *h = &c->hold[k];
        ee_unit *u = h->u;
        int go = 0;
        if (!cv_alive(u) || unit_get_id(u) != h->id || unit_get_owner(u) != pl) go = 1;
        else {
            int d2 = ivec2_dist2(unit_get_tile(u), h->t);
            if (d2 <= 9 && !h->at) h->at = now ? now : 1;
            if (h->kind == 1) {
                int hp = unit_get_hp(u), mx = cv_hpmax(u), nb = 0;
                for (int j = 0; j < c->nhs; j++) nb |= ivec2_dist2(c->h[j], h->t) <= 9;
                if (hp * 10 >= mx * 9 || !nb || now - h->since > 240000) { go = 1; c->heal_done += hp * 10 >= mx * 9; }
            } else if (h->kind == 3) {   // gave up: until its attack is over (the group gone, or it no longer in it), 5 minutes at most
                int j = cv_group_of(u);
                if (j < 0 || g_cvg[j].g != h->g || now - h->since > 300000) go = 1;
            } else if ((h->at && now - h->at >= 20000) || now - h->since > 90000) go = 1;
        }
        if (go) { if (h->kind == 2 || h->kind == 4) c->back++; c->hold[k] = c->hold[--c->nh]; continue; }
        ivec2 p = unit_get_tile(u);
        if (h->kind == 3 && h->home && h->at) {   // no cover: on toward home from where it got to
            ivec2 t;
            if (cv_home_pt(c, p, area_id(p, 2), &t)) { h->t = t; h->at = 0; }
        }
        if (ivec2_dist2(p, h->t) <= (h->at ? 25 : 4) || goal_near(u, (float)h->t.x + 0.5f, (float)h->t.y + 0.5f, 2.5f)) continue;
        if (h->at && fl_attacking(u)) continue;   // at the cover: it fights what comes in reach
        unit_goal(u, goal_move((float)h->t.x + 0.5f + (float)(k % 3) - 1.0f, (float)h->t.y + 0.5f + (float)(k / 3 % 3) - 1.0f), 0, 0x1F40);
    }
    static struct { int cx, cy; float r; } cache[256];
    int nc = 0;
    for (int i = 0; i < g_cv_nu; i++) {
        ee_unit *u = g_cv_units[i];
        ee_unit_type *td = unit_get_type_data(u);
        if (cv_mil(td) != 1 || cv_held(c, u) || landing_has_unit(pl, u) || ou_sortie(pl, u)) continue;   // outdated units fight to the end (army.c)
        ivec2 p = unit_get_tile(u);
        int hp = unit_get_hp(u), mx = cv_hpmax(u);
        unsigned a = area_id(p, 2);
        if (!a) continue;
        if ((plan & 1) && c->nhs && mx > 0 && hp * 100 < mx * g_fb_hp && cv_value(pl, unit_type_get_id(td)) >= g_fb_value) {
            int best = -1, bd = g_fb_dist * g_fb_dist + 1;
            for (int k = 0; k < c->nhs; k++) {
                int d = ivec2_dist2(c->h[k], p);
                if (d < bd && area_id(c->h[k], 2) == a && !cv_foe_near(c->h[k], 1)) { bd = d; best = k; }
            }
            if (best >= 0) {
                ivec2 hs = c->h[best];
                float dx = (float)(p.x - hs.x), dy = (float)(p.y - hs.y), l = sqrtf(dx * dx + dy * dy);
                cv_take(c, u, 1, iv2(hs.x + (l >= 1 ? (int)lroundf(2.0f * dx / l) : 2), hs.y + (l >= 1 ? (int)lroundf(2.0f * dy / l) : 0)), now, NULL, 0);
                c->heal_sent++;
                if (g_log && g_autotest) fprintf(g_log, "# cover: P%u at %.1f min: type %d at %d,%d (hp %d/%d) to the Hospital %d,%d\n", idx, now / 60000.0,
                                                 unit_type_get_id(td), p.x, p.y, hp, mx, hs.x, hs.y);
                continue;
            }
        }
        if ((plan & 2) && cv_foe_near(p, 1)) {
            int k = 0;
            while (k < nc && (cache[k].cx != p.x / 4 || cache[k].cy != p.y / 4)) k++;
            if (k == nc && nc < 256) { cache[nc].cx = p.x / 4; cache[nc].cy = p.y / 4; cache[nc].r = cv_local(pl, p, 10, 1); nc++; c->judged++; }
            float e = k < 256 ? cache[k].r : 1e6f;
            ivec2 t;
            int home = 0;
            if (e * 100.0f >= (float)g_fb_ratio) continue;
            if (!cv_cover(pl, c, p, a, &t)) {   // END: no own cover near - toward home rather than fight on
                if (!(plan & 8) || !cv_home_pt(c, p, a, &t)) continue;
                home = 1;
            } else if (ivec2_dist2(p, t) <= 16) continue;
            int j = (plan & 8) ? cv_group_of(u) : -1, n = j >= 0 ? cv_tally(c, u, g_cvg[j].g, now) : 0;
            int kind = j >= 0 && n >= g_fb_breaks ? 3 : home ? 4 : 2;
            cv_take(c, u, kind, t, now, j >= 0 ? g_cvg[j].g : NULL, home);
            c->broke++; c->gaveup += kind == 3; c->home += home;
            if (g_log && g_autotest) fprintf(g_log, "# cover: P%u at %.1f min: type %d at %d,%d breaks off (fight %.2f) to %d,%d%s\n", idx, now / 60000.0,
                                             unit_type_get_id(td), p.x, p.y, e, t.x, t.y,
                                             kind == 3 ? (home ? " - gives its attack up, home" : " - gives its attack up") : home ? " - home, no cover near" : "");
        }
    }
    if (plan & 8) cv_end(pl, idx, c, now);
}

internal void cover_tick(ee_player *pl, uint32_t now) {
    if (!pl) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct cv_pl *c = &g_cv[idx];
    if (now < c->ts || now < c->tb) { memset(c, 0, sizeof *c); }
    int plan = is_vanilla(pl) ? 0 : FALL_BACK(idx) & 11, meas = g_autotest && g_log;
    int act = plan && (!c->tb || now - c->tb >= 2000), smp = meas && (!c->ts || now - c->ts >= 5000);
    int zones = g_danger_routes && PV(idx, g_v_avoid_lost, g_avoid_lost) && !is_vanilla(pl) && (!c->tz || now - c->tz >= 4000);
    if (!act && !smp && !zones) return;
    cv_scan(pl, c);
    if (zones) { c->tz = now ? now : 1; cv_zones(pl, idx, now); }
    if (act) { c->tb = now ? now : 1; cv_behave(pl, idx, c, now, plan); }
    if (!smp) return;
    c->ts = now ? now : 1;
    cv_purge(now);
    cv_sample(pl, c, now);
    if (!c->started) { c->started = 1; c->min = now / 60000; return; }
    if (now / 60000 == c->min) return;
    float s = c->samples ? (float)c->samples : 1.0f;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# cover: P%u at %u min: mil %.1f dmg %.1f quiet %.1f | heal hosp %.0f dock %.0f other %.0f | hospitals %d docks %d defences %d | "
            "fb heal %u healed %u broke %u back %u held %d refused %u judged %u gave %u home %u ended %u\n", idx, c->min, c->mil / s, c->dmg / s,
            c->dmgq / s, c->heal_h, c->heal_d, c->heal_o, c->nhs, c->ndk, c->ncv, c->heal_sent, c->heal_done, c->broke, c->back, c->nh, c->refused,
            c->judged, c->gaveup, c->home, c->ended);
    LeaveCriticalSection(&g_lock);
    c->samples = c->mil = c->dmg = c->dmgq = 0; c->heal_h = c->heal_d = c->heal_o = 0;
    c->heal_sent = c->heal_done = c->broke = c->back = c->refused = c->judged = c->gaveup = c->home = c->ended = 0;
    c->min = now / 60000;
}
internal void cover_install(const char *autotest_ini) {
    g_fall_back = g_set_ai[1].fall_back;
    g_fb_ratio = g_set_ai[1].fall_back_ratio;
    g_fb_hp = g_set_ai[1].fall_back_hp;
    g_fb_value = g_set_ai[1].fall_back_value;
    g_fb_dist = g_set_ai[1].fall_back_dist;
    g_fb_breaks = g_set_ai[1].fall_back_breaks;
    g_fb_end = g_set_ai[1].fall_back_end;
    g_fight_mem = g_set_ai[1].fight_memory;
    g_threat_mem = g_set_ai[1].threat_memory;
    if (autotest_ini) g_v_threat_mem = ini_int("autotest", "v_threat_memory", -1, autotest_ini);
    if (autotest_ini) g_v_fight_mem = ini_int("autotest", "v_fight_memory", -1, autotest_ini);
    if (autotest_ini) g_v_fall_back = ini_int("autotest", "v_fall_back", -1, autotest_ini);
    if (autotest_ini) g_v_avoid_lost = ini_int("autotest", "v_avoid_lost_fights", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# fall back: %d (variant %d), ratio %d%%, hp %d%%, value %d, dist %d, breaks %d, end %d%%\n", g_fall_back, g_v_fall_back,
                       g_fb_ratio, g_fb_hp, g_fb_value, g_fb_dist, g_fb_breaks, g_fb_end);
}
