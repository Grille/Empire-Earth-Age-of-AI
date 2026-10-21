// FALL BACK TO COVER (docs/economy.md "Fall back to cover", ships docs/navy.md). Included by eeaimod.c after metal.c
// (mt_line), mass.c (ms_base), fleet.c (fl_attacking), landing.c (landing_has_unit), stats.c (the fight estimate),
// attack.c (threat_at, defence_stats, unit_goal, goal_move, seen_by) and assault.c (area_id, as_goal_near).
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
// fall_back ([all], bits; [autotest] v_fall_back for the variant players; per unit, never an attack called back):
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
// Measured (base AI at hard, 6 maps x both cheats, 40 games a cell; head-to-head): docs/economy.md "Fall back to
// cover" - 7 on by default.
static int g_fall_back = 7, g_v_fall_back = -1;
static int g_fb_ratio = 50, g_fb_hp = 40, g_fb_value = 100, g_fb_dist = 60;
#define FALL_BACK(idx) PV(idx, g_v_fall_back, g_fall_back)
#define CV_N 8192
#define CV_TOMB ((uint8_t *)1)
#define CV_HOLD 128
#define CV_MIL 2048
#define CV_CELL 8
#define CV_CELLS 64
struct cv_rec { uint8_t *u; int id, hp; uint32_t seen, low_t, eng_t, att_t; int16_t eng; uint8_t quiet, etower; };
static struct cv_rec g_cvr[CV_N], g_cvr_tmp[CV_N];
static unsigned g_cvr_n;
static uint32_t g_cvr_purge;
struct cv_hold { uint8_t *u; int id, kind, tx, ty; uint32_t since, at; };
static struct cv_pl {
    uint32_t ts, tb, tscan; unsigned min; int started;
    unsigned samples, mil, dmg, dmgq;
    float heal_h, heal_d, heal_o;
    int nhs, ndk, ncv, bx, by, base;
    int hx[16], hy[16], dkx[16], dky[16], cvx[64], cvy[64];
    struct cv_hold hold[CV_HOLD]; int nh;
    unsigned heal_sent, heal_done, broke, back, refused, judged;   // since the last line
} g_cv[16];
static uint8_t g_cv_foe[CV_CELLS][CV_CELLS], g_cv_seen[CV_CELLS][CV_CELLS];   // enemy fighters per 8-tile cell (all / seen)
static struct st_side g_cvs_own, g_cvs_foe;
static void cover_new_game(void) { memset(g_cvr, 0, sizeof g_cvr); g_cvr_n = 0; g_cvr_purge = 0; memset(g_cv, 0, sizeof g_cv); }

static unsigned cv_hash(uint8_t *u) { return ((uint32_t)(uintptr_t)u >> 3) * 2654435761u >> 19; }
static struct cv_rec *cv_find(uint8_t *u, int add) {
    unsigned h = cv_hash(u) & (CV_N - 1);
    int id = FIELD(u, OF(UNIT, 0x60), int);
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
static void cv_purge(uint32_t now) {   // once a minute: the records seen in the last 30 s, no tombstones
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
static int cv_mil(uint8_t *td) {
    int fam = FIELD(td, OF(TD, 0x4B4), int), t = FIELD(td, OF(TD, 0x4B8), int);
    if (!is_army_family(fam) || is_citizen_type(t) || FIELD(td, OF(TD, 0x510), int) <= 0) return 0;
    if (fam == 3 || fam == 7 || fam == 17 || fam == 18 || fam == 21 || fam == 22) return 0;
    if (fam == 4 || fam == 10) return 2;
    return 1;
}
static int cv_alive(uint8_t *u) { return FIELD(u, OF(UNIT, 0x3C), int) > 0 && FIELD(u, OF(UNIT, 0x1C), int) >= 0; }
static int cv_built(uint8_t *u) { uint8_t *bs = FIELD(u, OF(UNIT, 0x30), uint8_t *); return bs && bs[OF(UNIT, 0x340)]; }
static int cv_value(void *pl, int type) {   // the base price, F+W+S+G+I (no cheat)
    uint8_t *obj = type_data(type), *tt = pl ? FIELD(pl, PL_TECHTREE, uint8_t *) : NULL;
    uint8_t *tech = obj && tt ? GF(FN_TECH_LOOKUP)(tt, FIELD(obj, OF(TD, 0x4E0), int)) : NULL;
    uint8_t *data = tech ? FIELD(tech, OF(TECH, 0x8), uint8_t *) : NULL;
    int v = 0;
    for (int r = 2; data && r < 7; r++) v += FIELD(data, OF(TECHDATA, 0x24) + (r - 2) * 4, int);
    return v;
}
static int cv_hpmax(uint8_t *u) { return st_maxhp(FIELD(u, OF(UNIT, 0x2C), uint8_t *), FIELD(u, OF(UNIT, 0x34), uint8_t *)); }
static int cv_d2(int x0, int y0, int x1, int y1) { return (x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0); }
// an enemy fighter in (x, y)'s 8-tile cell or one around it (seen: only those pl sees)
static int cv_foe_near(int x, int y, int seen) {
    int cx = x / CV_CELL, cy = y / CV_CELL;
    for (int a = cx - 1; a <= cx + 1; a++)
        for (int b = cy - 1; b <= cy + 1; b++)
            if (a >= 0 && b >= 0 && a < CV_CELLS && b < CV_CELLS && (seen ? g_cv_seen[a][b] : g_cv_foe[a][b])) return 1;
    return 0;
}
// THE LOCAL FIGHT at (x, y) for pl: own and allied fighters within r against the enemy's within r, defences on both
// sides whose reach (+1) covers the spot; honest: only the enemies pl sees. st_ratio: above 1 own stronger (the square
// root of the time ratio), 1e6 nothing to fight
static float cv_local(void *pl, int x, int y, int r, int honest) {
    st_side_clear(&g_cvs_own); st_side_clear(&g_cvs_foe);
    for (struct grid_it it = grid_it_box(x, y, r + 10, 1); grid_it_next(&it); ) {
        uint8_t *u = it.u, *td = it.td;
        void *o = FIELD(u, OF(UNIT, 0x18), void *);
        if (!o || FIELD(o, PL_INDEX, int) <= 0 || !cv_alive(u)) continue;
        int fam = FIELD(td, OF(TD, 0x4B4), int), d2 = cv_d2(FIELD(u, OF(UNIT, 0x1C), int), FIELD(u, OF(UNIT, 0x20), int), x, y);
        float rg, at;
        if (fam == 2 || fam == 29) {
            if (!defence_stats(u, &rg, &at) || at <= 0 || !cv_built(u) || (float)d2 > (rg + 1.0f) * (rg + 1.0f)) continue;
        } else if (!cv_mil(td) || d2 > r * r) continue;
        int foe = is_enemy(pl, o);
        if (foe && honest && !seen_by(u, pl)) continue;
        st_side_add(foe ? &g_cvs_foe : &g_cvs_own, u);
    }
    return st_ratio(&g_cvs_own, &g_cvs_foe);
}
// an enemy defence covers (x, y) (its range + 1; all, not only the known ones)
static int cv_enemy_def(void *pl, int x, int y) {
    for (struct grid_it it = grid_it_box(x, y, 12, 1); grid_it_next(&it); ) {
        uint8_t *u = it.u, *td = it.td;
        int fam = FIELD(td, OF(TD, 0x4B4), int);
        float rg, at;
        if ((fam != 2 && fam != 29) || !cv_alive(u) || !is_enemy(pl, FIELD(u, OF(UNIT, 0x18), void *)) || !defence_stats(u, &rg, &at) || at <= 0 || !cv_built(u)) continue;
        if ((float)cv_d2(FIELD(u, OF(UNIT, 0x1C), int), FIELD(u, OF(UNIT, 0x20), int), x, y) <= (rg + 1.0f) * (rg + 1.0f)) return 1;
    }
    return 0;
}
static int cv_nearest(const int *xs, const int *ys, int n, int x, int y) {   // tiles to the nearest, -1 none
    int b = -1;
    for (int k = 0; k < n; k++) { int d = cv_d2(xs[k], ys[k], x, y); if (b < 0 || d < b) b = d; }
    return b < 0 ? -1 : (int)sqrtf((float)b);
}
static int cv_held(struct cv_pl *c, uint8_t *u) {
    for (int k = 0; k < c->nh; k++) if (c->hold[k].u == u) return k + 1;
    return 0;
}

// THE SCAN: own Hospitals, Docks and defences (finished), the enemy fighters by cell, and the own military units
static uint8_t *g_cv_units[CV_MIL];
static int g_cv_nu;
static void cv_scan(void *pl, struct cv_pl *c) {
    c->nhs = c->ndk = c->ncv = 0; g_cv_nu = 0;
    memset(g_cv_foe, 0, sizeof g_cv_foe); memset(g_cv_seen, 0, sizeof g_cv_seen);
    c->base = ms_base(pl, &c->bx, &c->by);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        uint8_t *u = it.u, *td = it.td;
        void *o = FIELD(u, OF(UNIT, 0x18), void *);
        if (!o || FIELD(o, PL_INDEX, int) <= 0 || !cv_alive(u)) continue;
        int fam = FIELD(td, OF(TD, 0x4B4), int), t = FIELD(td, OF(TD, 0x4B8), int), x = FIELD(u, OF(UNIT, 0x1C), int), y = FIELD(u, OF(UNIT, 0x20), int);
        if (o == pl) {
            if (fam == 2 || fam == 29) {
                float rg, at;
                if (!cv_built(u)) continue;
                if (t == UT(B_HOSPITAL) && c->nhs < 16) { c->hx[c->nhs] = x; c->hy[c->nhs++] = y; }
                else if (t == UT(B_DOCK) && c->ndk < 16) { c->dkx[c->ndk] = x; c->dky[c->ndk++] = y; }
                else if (defence_stats(u, &rg, &at) && at > 0 && c->ncv < 64) { c->cvx[c->ncv] = x; c->cvy[c->ncv++] = y; }
            } else if (cv_mil(td) && g_cv_nu < CV_MIL) g_cv_units[g_cv_nu++] = u;
            continue;
        }
        if (!is_enemy(pl, o)) continue;
        float rg, at;
        if (!cv_mil(td) && !((fam == 2 || fam == 29) && defence_stats(u, &rg, &at) && at > 0)) continue;
        int cx = x / CV_CELL, cy = y / CV_CELL;
        if (cx < 0 || cy < 0 || cx >= CV_CELLS || cy >= CV_CELLS) continue;
        g_cv_foe[cx][cy] = 1;
        if (seen_by(u, pl)) g_cv_seen[cx][cy] = 1;
    }
}

// the measurement's sample (every 5 s, autotest)
static void cv_sample(void *pl, struct cv_pl *c, uint32_t now) {
    static struct { int cx, cy; float r; } cache[256];
    int nc = 0;
    c->samples++;
    for (int i = 0; i < g_cv_nu; i++) {
        uint8_t *u = g_cv_units[i];
        struct cv_rec *r = cv_find(u, 1);
        if (!r) continue;
        int hp = FIELD(u, OF(UNIT, 0x3C), int), mx = cv_hpmax(u), x = FIELD(u, OF(UNIT, 0x1C), int), y = FIELD(u, OF(UNIT, 0x20), int);
        int land = cv_mil(FIELD(u, OF(UNIT, 0x2C), uint8_t *)) == 1;
        if (r->seen && hp > r->hp) {   // regained: by an own Hospital / Dock within 5 tiles, else elsewhere
            int dh = cv_nearest(c->hx, c->hy, c->nhs, x, y), dd = cv_nearest(c->dkx, c->dky, c->ndk, x, y);
            if (land && dh >= 0 && dh <= 5) c->heal_h += (float)(hp - r->hp);
            else if (!land && dd >= 0 && dd <= 5) c->heal_d += (float)(hp - r->hp);
            else c->heal_o += (float)(hp - r->hp);
        }
        r->hp = hp; r->seen = now ? now : 1;
        c->mil++;
        if (mx > 0 && hp * 2 < mx) {
            if (!r->low_t) r->low_t = now ? now : 1;
            c->dmg++;
            if (!cv_foe_near(x, y, 0)) { r->quiet = 1; c->dmgq++; }
        } else { r->low_t = 0; r->quiet = 0; }
        if (fl_attacking(u)) {
            if (!r->att_t || now - r->att_t > 30000) {   // a new fight: its estimate (cached per 4-tile cell this sample)
                int k = 0;
                while (k < nc && (cache[k].cx != x / 4 || cache[k].cy != y / 4)) k++;
                if (k == nc && nc < 256) { cache[nc].cx = x / 4; cache[nc].cy = y / 4; cache[nc].r = cv_local(pl, x, y, 10, 0); nc++; }
                float e = k < 256 ? cache[k].r : 1.0f;
                r->eng = (int16_t)(e > 99.0f ? 9900 : (int)(e * 100.0f + 0.5f));
                r->eng_t = now ? now : 1;
                uint8_t *t = fl_target_of(u);
                r->etower = t ? (uint8_t)cv_enemy_def(pl, FIELD(t, OF(UNIT, 0x1C), int), FIELD(t, OF(UNIT, 0x20), int)) : 0;
            }
            r->att_t = now ? now : 1;
        }
    }
}

// a military unit killed (diplo.c hook_kill_stat, n 3; k the killer's index; autotest)
static void cover_death(uint8_t *dead, int k) {
    if (!g_autotest || !g_log || !dead) return;
    uint8_t *td = FIELD(dead, OF(UNIT, 0x2C), uint8_t *);
    void *pl = FIELD(dead, OF(UNIT, 0x18), void *);
    if (!td || !pl || FIELD(dead, OF(UNIT, 0x1C), int) < 0) return;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    int kind = cv_mil(td);
    if (!kind || !(g_players[idx] || is_vanilla_idx((int)idx))) return;
    struct cv_pl *c = &g_cv[idx];
    struct cv_rec *r = cv_find(dead, 0);
    uint32_t now = game_time();
    int t = FIELD(td, OF(TD, 0x4B8), int), x = FIELD(dead, OF(UNIT, 0x1C), int), y = FIELD(dead, OF(UNIT, 0x20), int), nb = 0;
    char line = 'o';
    if (kind == 2) line = 'w';
    else if (FIELD(td, OF(TD, 0x4C4), int) == KIND_HERO) line = 'h';
    else switch (mt_line(pl, t)) { case MB_INF: line = 'i'; break; case MB_RNG: line = 'r'; break; case MB_CAV: line = 'c'; break; case MB_SIE: line = 's'; break; default: break; }
    for (struct grid_it it = grid_it_box(x, y, 6, 1); grid_it_next(&it); ) {
        void *o = FIELD(it.u, OF(UNIT, 0x18), void *);
        if (it.u != dead && o && FIELD(o, PL_INDEX, int) > 0 && !is_enemy(pl, o) && cv_alive(it.u) && cv_mil(it.td) &&
            cv_d2(FIELD(it.u, OF(UNIT, 0x1C), int), FIELD(it.u, OF(UNIT, 0x20), int), x, y) <= 36) nb++;
    }
    float loc = cv_local(pl, x, y, 10, 0);
    int eng = r && r->eng_t && now >= r->eng_t && now - r->eng_t < 120000 ? r->eng : -1;
    int h = cv_held(c, dead);
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# cvd: P%u at %.1f min: %c%d val %d hp %d | eng %d %.0f | loc %d | tower %d near %d | low %.0f quiet %d | hosp %d dock %d | held %d | by P%d\n",
            idx, now / 60000.0, line, t, cv_value(pl, t), cv_hpmax(dead), eng, eng >= 0 ? (now - r->eng_t) / 1000.0 : 0.0,
            loc > 99.0f ? 9900 : (int)(loc * 100.0f + 0.5f), cv_enemy_def(pl, x, y), nb,
            r && r->low_t && now >= r->low_t ? (now - r->low_t) / 1000.0 : 0.0, r ? r->quiet : 0,
            cv_nearest(c->hx, c->hy, c->nhs, x, y), cv_nearest(c->dkx, c->dky, c->ndk, x, y), h ? c->hold[h - 1].kind : 0, k);
    LeaveCriticalSection(&g_lock);
    if (r) r->u = CV_TOMB;
}

// ---- fall_back ----
// landing.c hook_asg_give_goal: an attack subgroup's order to a unit held here
static int cover_refuses(uint8_t *sg, uint8_t *u) {
    void *pl = FIELD(sg, OF(ASG, 0x190), void *);
    if (!pl) return 0;
    struct cv_pl *c = &g_cv[(unsigned)FIELD(pl, PL_INDEX, int) & 15];
    if (!c->nh || !cv_held(c, u)) return 0;
    c->refused++;
    return 1;
}
static int cover_sea(unsigned idx) { return (FALL_BACK(idx & 15) & 4) != 0; }   // fleet.c
// the cover for a unit at (x, y) on land area a: the nearest own defence no known enemy defence covers (2 tiles on the
// base's side of it), else a Hospital, else the base; 0 none within reach
static int cv_cover(void *pl, struct cv_pl *c, int x, int y, unsigned a, int *tx, int *ty) {
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    int best = -1, bd = g_fb_dist * g_fb_dist + 1;
    threat_refresh(pl, game_time());
    for (int k = 0; k < c->ncv; k++) {
        int d = cv_d2(c->cvx[k], c->cvy[k], x, y);
        if (d < bd && area_id(c->cvx[k], c->cvy[k], 2) == a && !(threat_at(pi, c->cvx[k], c->cvy[k]) > 0)) { bd = d; best = k; }
    }
    if (best >= 0) {
        int cx = c->cvx[best], cy = c->cvy[best];
        float dx = (float)(c->bx - cx), dy = (float)(c->by - cy), l = sqrtf(dx * dx + dy * dy);
        *tx = cx + (c->base && l >= 1 ? (int)lroundf(2.0f * dx / l) : 0); *ty = cy + (c->base && l >= 1 ? (int)lroundf(2.0f * dy / l) : 0);
        return 1;
    }
    for (int k = 0; k < c->nhs; k++) {
        int d = cv_d2(c->hx[k], c->hy[k], x, y);
        if (d < bd && area_id(c->hx[k], c->hy[k], 2) == a) { bd = d; *tx = c->hx[k]; *ty = c->hy[k] + 2; best = k; }
    }
    if (best >= 0) return 1;
    if (c->base && area_id(c->bx, c->by, 2) == a && cv_d2(c->bx, c->by, x, y) <= g_fb_dist * g_fb_dist) { *tx = c->bx; *ty = c->by; return 1; }
    return 0;
}
static void cv_take(struct cv_pl *c, uint8_t *u, int kind, int tx, int ty, uint32_t now) {
    if (c->nh >= CV_HOLD) return;
    struct cv_hold *h = &c->hold[c->nh++];
    h->u = u; h->id = FIELD(u, OF(UNIT, 0x60), int); h->kind = kind; h->tx = tx; h->ty = ty; h->since = now ? now : 1; h->at = 0;
}
static void cv_behave(void *pl, unsigned idx, struct cv_pl *c, uint32_t now, int plan) {
    // the held units: gone, healed, arrived and rested - let go
    for (int k = c->nh - 1; k >= 0; k--) {
        struct cv_hold *h = &c->hold[k];
        uint8_t *u = h->u;
        int go = 0;
        if (!cv_alive(u) || FIELD(u, OF(UNIT, 0x60), int) != h->id || FIELD(u, OF(UNIT, 0x18), void *) != pl) go = 1;
        else {
            int d2 = cv_d2(FIELD(u, OF(UNIT, 0x1C), int), FIELD(u, OF(UNIT, 0x20), int), h->tx, h->ty);
            if (d2 <= 9 && !h->at) h->at = now ? now : 1;
            if (h->kind == 1) {
                int hp = FIELD(u, OF(UNIT, 0x3C), int), mx = cv_hpmax(u), nb = 0;
                for (int j = 0; j < c->nhs; j++) nb |= cv_d2(c->hx[j], c->hy[j], h->tx, h->ty) <= 9;
                if (hp * 10 >= mx * 9 || !nb || now - h->since > 240000) { go = 1; c->heal_done += hp * 10 >= mx * 9; }
            } else if ((h->at && now - h->at >= 20000) || now - h->since > 90000) go = 1;
        }
        if (go) { if (h->kind == 2) c->back++; c->hold[k] = c->hold[--c->nh]; continue; }
        int x = FIELD(u, OF(UNIT, 0x1C), int), y = FIELD(u, OF(UNIT, 0x20), int);
        if (cv_d2(x, y, h->tx, h->ty) <= (h->at ? 25 : 4) || as_goal_near(u, (float)h->tx + 0.5f, (float)h->ty + 0.5f, 2.5f)) continue;
        if (h->at && fl_attacking(u)) continue;   // at the cover: it fights what comes in reach
        unit_goal(u, goal_move((float)h->tx + 0.5f + (float)(k % 3) - 1.0f, (float)h->ty + 0.5f + (float)(k / 3 % 3) - 1.0f), 0, 0x1F40);
    }
    static struct { int cx, cy; float r; } cache[256];
    int nc = 0;
    for (int i = 0; i < g_cv_nu; i++) {
        uint8_t *u = g_cv_units[i], *td = FIELD(u, OF(UNIT, 0x2C), uint8_t *);
        if (cv_mil(td) != 1 || cv_held(c, u) || landing_has_unit(pl, u)) continue;
        int x = FIELD(u, OF(UNIT, 0x1C), int), y = FIELD(u, OF(UNIT, 0x20), int), hp = FIELD(u, OF(UNIT, 0x3C), int), mx = cv_hpmax(u);
        unsigned a = area_id(x, y, 2);
        if (!a) continue;
        if ((plan & 1) && c->nhs && mx > 0 && hp * 100 < mx * g_fb_hp && cv_value(pl, FIELD(td, OF(TD, 0x4B8), int)) >= g_fb_value) {
            int best = -1, bd = g_fb_dist * g_fb_dist + 1;
            for (int k = 0; k < c->nhs; k++) {
                int d = cv_d2(c->hx[k], c->hy[k], x, y);
                if (d < bd && area_id(c->hx[k], c->hy[k], 2) == a && !cv_foe_near(c->hx[k], c->hy[k], 1)) { bd = d; best = k; }
            }
            if (best >= 0) {
                float dx = (float)(x - c->hx[best]), dy = (float)(y - c->hy[best]), l = sqrtf(dx * dx + dy * dy);
                int tx = c->hx[best] + (l >= 1 ? (int)lroundf(2.0f * dx / l) : 2), ty = c->hy[best] + (l >= 1 ? (int)lroundf(2.0f * dy / l) : 0);
                cv_take(c, u, 1, tx, ty, now);
                c->heal_sent++;
                if (g_log && g_autotest) fprintf(g_log, "# cover: P%u at %.1f min: type %d at %d,%d (hp %d/%d) to the Hospital %d,%d\n", idx, now / 60000.0,
                                                 FIELD(td, OF(TD, 0x4B8), int), x, y, hp, mx, c->hx[best], c->hy[best]);
                continue;
            }
        }
        if ((plan & 2) && cv_foe_near(x, y, 1)) {
            int k = 0;
            while (k < nc && (cache[k].cx != x / 4 || cache[k].cy != y / 4)) k++;
            if (k == nc && nc < 256) { cache[nc].cx = x / 4; cache[nc].cy = y / 4; cache[nc].r = cv_local(pl, x, y, 10, 1); nc++; c->judged++; }
            float e = k < 256 ? cache[k].r : 1e6f;
            int tx, ty;
            if (e * 100.0f >= (float)g_fb_ratio || !cv_cover(pl, c, x, y, a, &tx, &ty) || cv_d2(x, y, tx, ty) <= 16) continue;
            cv_take(c, u, 2, tx, ty, now);
            c->broke++;
            if (g_log && g_autotest) fprintf(g_log, "# cover: P%u at %.1f min: type %d at %d,%d breaks off (fight %.2f) to %d,%d\n", idx, now / 60000.0,
                                             FIELD(td, OF(TD, 0x4B8), int), x, y, e, tx, ty);
        }
    }
}

static void cover_tick(void *pl, uint32_t now) {
    if (!pl) return;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    struct cv_pl *c = &g_cv[idx];
    if (now < c->ts || now < c->tb) { memset(c, 0, sizeof *c); }
    int plan = is_vanilla(pl) ? 0 : FALL_BACK(idx) & 3, meas = g_autotest && g_log;
    int act = plan && (!c->tb || now - c->tb >= 2000), smp = meas && (!c->ts || now - c->ts >= 5000);
    if (!act && !smp) return;
    cv_scan(pl, c);
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
            "fb heal %u healed %u broke %u back %u held %d refused %u judged %u\n", idx, c->min, c->mil / s, c->dmg / s, c->dmgq / s,
            c->heal_h, c->heal_d, c->heal_o, c->nhs, c->ndk, c->ncv, c->heal_sent, c->heal_done, c->broke, c->back, c->nh, c->refused, c->judged);
    LeaveCriticalSection(&g_lock);
    c->samples = c->mil = c->dmg = c->dmgq = 0; c->heal_h = c->heal_d = c->heal_o = 0;
    c->heal_sent = c->heal_done = c->broke = c->back = c->refused = c->judged = 0;
    c->min = now / 60000;
}
static void cover_install(const char *aimod_ini, const char *autotest_ini) {
    g_fall_back = cfg_int(aimod_ini, autotest_ini, 1, "fall_back", 7);
    g_fb_ratio = cfg_pct(aimod_ini, autotest_ini, 1, "fall_back_ratio", 50, 1000);
    g_fb_hp = cfg_pct(aimod_ini, autotest_ini, 1, "fall_back_hp", 40, 100);
    g_fb_value = cfg_int(aimod_ini, autotest_ini, 1, "fall_back_value", 100);
    g_fb_dist = cfg_int(aimod_ini, autotest_ini, 1, "fall_back_dist", 60);
    if (autotest_ini) g_v_fall_back = ini_int("autotest", "v_fall_back", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# fall back: %d (variant %d), ratio %d%%, hp %d%%, value %d, dist %d\n", g_fall_back, g_v_fall_back,
                       g_fb_ratio, g_fb_hp, g_fb_value, g_fb_dist);
}
