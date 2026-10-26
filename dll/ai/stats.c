// UNIT STATS: what a unit does to another, from the game's own data - so the AI's judgements follow the game (and
// units modders add). Included by eeaimod.c before defence.c (its grid_each and is_soldier declared here).
//
// [ai_base] fight_estimate (default 0): 1 = the AI's fight decisions compare sides with this estimate (st_ratio) instead
// of summed hit points x attack: muster, hold the town, citizen militia (defence.c). The thresholds
// keep their meaning (the ratio is hit points x damage per second on each side). A/B 2026-10-17, overrun and assault
// (Continental 101, 10 seeds each, the defender): overrun alive 4 vs 3 of 10 (old vs estimate), kills 49 vs 31;
// assault alive 8 vs 9, workers 31.5 vs 22.8, losses 53 vs 62.5 - no gain: those are fights of numbers (90 against
// 20) where both agree, and where they differ the estimate rates the defenders lower, so it musters more. Off.
//
// Damage per shot (0x61DE61, docs/engine-notes.md): attack (the attacker's per-player stats +0xD8)
//   x table 0x8D2050[attacker class + target class x 6] (class = type +0x4C0; filled at load)
//   x hit factor 0x4D5F2C (thiscall on 0x917DE0: weapon kind type +0x4C8, target type, both player indices;
//     the table 0x8D200C[target family, type +0x4C4][weapon kind], 0 = cannot hit)
//   - armour (the target's stats +0xE0 + attacker class x 4); rounded, at least 1; a height factor for land
//   attackers (ignored here). Area damage (stats +0xA0) is not counted.
// Time between shots: the unit's own +0x2A0, 0 = the class value (stats +0x138, per player; the game refills +0x2A0
// from it, 0x61E176); type +0x50C is the base. Stats record of a type for a player: player +0x874 vector,
// 0x4A670A size, 0x492969 at. Max hit points stats +0x144, range +0x9C.

#define ST_TABLE damage_table()
internal int g_fight_est = 0;   // [ai_base] fight_estimate

internal ee_unit_stats *st_rec(ee_player *pl, int type) {   // the player's stats record of a type
    if (!pl || type < 0) return NULL;
    if ((unsigned)type >= type_stats_count(pl)) return NULL;
    ee_unit_stats **p = type_stats(pl, type);
    return p ? *p : NULL;
}
internal void *st_player(int i) {   // any player (g_players has the AIs only)
    return i >= 0 && i < 16 && (unsigned)i < game_player_count() ? game_players()[i] : NULL;
}
internal float st_factor(ee_unit_type *atd, int api, ee_unit_type *ttd, int tpi) {   // the game's hit factor, 0 = cannot hit
    if (!atd || !ttd) return 0.0f;
    return attack_bonus(atd, ttd, api, tpi);
}
// damage of one shot of type atd (stats ast, player index api) at type ttd (stats tst, tpi); 0 = cannot hurt it
internal int st_shot_t(ee_unit_type *atd, ee_unit_stats *ast, int api, ee_unit_type *ttd, ee_unit_stats *tst, int tpi) {
    if (!atd || !ttd) return 0;
    float att = ast ? unit_stats_get_attack(ast) : (float)unit_type_get_attack(atd);
    int ac = unit_type_get_damage_class(atd), tc = unit_type_get_damage_class(ttd);
    if (att <= 0 || ac < 0 || ac > 5 || tc < 0 || tc > 5) return 0;
    float f = st_factor(atd, api, ttd, tpi) * ST_TABLE[ac + tc * 6];
    if (f <= 0) return 0;
    float d = f * att - (tst ? unit_stats_get_armor(tst, ac) : 0.0f);
    int r = (int)(d + 0.5f);
    return r < 1 ? 1 : r;
}
internal int st_pidx(ee_unit *u) { ee_player *o = unit_get_owner(u); return o ? player_get_index(o) : 0; }
internal int st_shot(ee_unit *a, ee_unit *t) {
    return st_shot_t(unit_get_type_data(a), unit_get_stats(a), st_pidx(a), unit_get_type_data(t),
                     unit_get_stats(t), st_pidx(t));
}
internal float st_reload_t(ee_unit_type *atd, ee_unit_stats *ast) {   // seconds between shots (class value)
    int ms = ast ? unit_stats_get_reload(ast) : 0;
    if (ms <= 0 && atd) ms = unit_type_get_reload(atd);
    return ms > 0 ? ms / 1000.0f : 1.0f;
}
// in a fight a shot takes longer than the reload: picking the next target, turning, stepping into range - duels of
// 2026-10-17 (scripts/zz_duel.sh): machine gunners (reload 1 s) fired every 3.3-5.1 s, Mk V tanks (4 s) every
// 4.6-7.1 s; about 2 s on every shot
#define ST_SHOT_OVERHEAD 2.0f
internal float st_range(ee_unit *u) { ee_unit_stats *st = unit_get_stats(u); return st ? unit_stats_get_range(st) : unit_type_get_range(unit_get_type_data(u)); }
internal float st_speed(ee_unit *u) { ee_unit_stats *st = unit_get_stats(u); return st ? unit_stats_get_speed(st) : 0.0f; }   // tiles/s, 0 = cannot move
internal int st_maxhp(ee_unit_type *td, ee_unit_stats *st) { return st && unit_stats_get_hp(st) > 0 ? unit_stats_get_hp(st) : unit_type_get_hp(td); }

// --- a fight between two sides ------------------------------------------------------------------------------------
// By unit type: units of one type and player share their stats, so the work grows with types x types (a late army
// has 10-20), not units x units (1000 x 1000 with a big population). Each unit only adds its type's count and hp, and
// its tile for area damage (a count per tile; the neighbours of a target within a radius are a few tiles' sums).

#define ST_GROUPS 64
#define ST_UNITS 4096
#define ST_RADII 4
// area damage: at most this many units beside the target count - slow shells, moving targets and a crowd that spreads
// (3 artillery against 8 machine gunners: about the target and two more; 200 v 200 with all neighbours counted, 20
// artillery made 60% of an army's estimate and the army lost)
#define ST_AREA_MAX 2.0f
// a group: its type, its owner's stats of it and the owner's index; rep a live unit of it (its own reload), NULL for
// a group of remembered units only (st_side_add_rec); part: its share of the last st_side_dps
struct st_group { ee_unit *rep; ee_unit_type *td; ee_unit_stats *st; int type, owner, count; float hp, nbr[ST_RADII], part; };
struct st_side {
    int ng, n;
    float hp;
    struct st_group g[ST_GROUPS];
    int16_t ux[ST_UNITS], uy[ST_UNITS];
    uint8_t ug[ST_UNITS];
    int x0, y0, w, h;                   // the tile counts: bounding box
    uint16_t *cnt;
    int nr; float radius[ST_RADII];     // radii whose neighbour counts are known
};
internal void st_side_clear(struct st_side *s) { free(s->cnt); memset(s, 0, sizeof *s); }
// one unit of type td, owner index owner (stats st), with hp at x, y; u the unit when it is live and seen (else NULL)
internal int st_side_put(struct st_side *s, ee_unit *u, ee_unit_type *td, ee_unit_stats *st, int owner, int hp, int x, int y) {   // 0 when full
    int type = unit_type_get_id(td), k = 0;
    while (k < s->ng && (s->g[k].type != type || s->g[k].owner != owner)) k++;
    if (k == s->ng) {
        if (k == ST_GROUPS || s->n == ST_UNITS) return 0;
        s->g[k].rep = u; s->g[k].td = td; s->g[k].st = st; s->g[k].type = type; s->g[k].owner = owner;
        s->ng++;
    }
    if (s->n == ST_UNITS) return 0;
    if (!s->g[k].rep && u) s->g[k].rep = u;
    s->g[k].count++;
    s->g[k].hp += (float)hp;
    s->hp += (float)hp;
    s->ux[s->n] = (int16_t)x; s->uy[s->n] = (int16_t)y; s->ug[s->n] = (uint8_t)k;
    s->n++;
    s->nr = 0;
    return 1;
}
internal int st_side_add(struct st_side *s, ee_unit *u) {
    return st_side_put(s, u, unit_get_type_data(u), unit_get_stats(u), st_pidx(u), unit_get_hp(u), unit_get_x(u), unit_get_y(u));
}
// a unit remembered (intel.c): its type and owner, hit points and the tile where it was seen
internal int st_side_add_rec(struct st_side *s, int type, int owner, int hp, int x, int y) {
    ee_unit_type *td = type_data(type);
    void *pl = st_player(owner);
    return td && pl ? st_side_put(s, NULL, td, st_rec(pl, type), owner, hp, x, y) : 0;
}
// a group's range, speed (tiles/s, 0 = cannot move), minimum range, seconds between shots (a live unit's own - scenario
// triggers change it -, else its class's) and damage per second at another group
internal float stg_range(struct st_group *g) { return g->st ? unit_stats_get_range(g->st) : unit_type_get_range(g->td); }
internal float stg_speed(struct st_group *g) { return g->st ? unit_stats_get_speed(g->st) : 0.0f; }
internal float stg_minrange(struct st_group *g) { return unit_type_get_min_range(g->td); }
internal float stg_reload(struct st_group *g) {
    int own = g->rep ? unit_get_reload(g->rep) : 0;
    return own > 0 ? own / 1000.0f : st_reload_t(g->td, g->st);
}
internal float stg_dps(struct st_group *a, struct st_group *t) {
    return st_shot_t(a->td, a->st, a->owner, t->td, t->st, t->owner) / (stg_reload(a) + ST_SHOT_OVERHEAD);
}
// RANGE (the user: "it can even overpower all the other stats since a unit can be eliminated from range before ever
// getting a chance to fire back"): a unit that cannot move never hits one that outranges it; a unit that is
// outranged walks the difference under fire first (st_close)
internal int stg_reaches(struct st_group *a, struct st_group *t) { return stg_speed(a) > 0 || stg_range(a) + 0.5f >= stg_range(t); }
// once a mobile target has closed to its own range, an attacker whose minimum range is longer cannot hit it (artillery)
internal int stg_inside_min(struct st_group *a, struct st_group *t) { return stg_speed(t) > 0 && stg_minrange(a) > 0 && stg_minrange(a) >= stg_range(t); }
// average units of the side within radius r of a unit of group k (itself not counted)
internal float st_neighbours(struct st_side *s, int k, float r) {
    int ri = 0;
    while (ri < s->nr && s->radius[ri] != r) ri++;
    if (ri < s->nr) return s->g[k].nbr[ri];
    if (!s->cnt) {   // the tile counts, once
        int x1 = -100000, y1 = -100000;
        s->x0 = s->y0 = 100000;
        for (int i = 0; i < s->n; i++) {
            if (s->ux[i] < s->x0) s->x0 = s->ux[i];
            if (s->uy[i] < s->y0) s->y0 = s->uy[i];
            if (s->ux[i] > x1) x1 = s->ux[i];
            if (s->uy[i] > y1) y1 = s->uy[i];
        }
        s->w = x1 - s->x0 + 1; s->h = y1 - s->y0 + 1;
        if (s->n <= 0 || s->w <= 0 || s->h <= 0 || s->w > 1024 || s->h > 1024 || !(s->cnt = calloc((size_t)s->w * s->h, 2))) return 0;
        for (int i = 0; i < s->n; i++) s->cnt[(s->uy[i] - s->y0) * s->w + s->ux[i] - s->x0]++;
    }
    if (s->nr == ST_RADII) s->nr = 0;
    ri = s->nr++;
    s->radius[ri] = r;
    int rr = (int)r, sum[ST_GROUPS] = { 0 };
    for (int i = 0; i < s->n; i++)
        for (int dy = -rr; dy <= rr; dy++)
            for (int dx = -rr; dx <= rr; dx++) {
                int x = s->ux[i] + dx - s->x0, y = s->uy[i] + dy - s->y0;
                if ((float)(dx * dx + dy * dy) <= r * r && x >= 0 && y >= 0 && x < s->w && y < s->h) sum[s->ug[i]] += s->cnt[y * s->w + x];
            }
    for (int g = 0; g < s->ng; g++) s->g[g].nbr[ri] = s->g[g].count ? (float)sum[g] / s->g[g].count - 1.0f : 0;
    return s->g[k].nbr[ri];
}
// side a's damage per second at side b: each unit spreads its fire over the units of b it can hurt (by their count);
// contact: b has closed in (nothing inside a's minimum range counts); area damage: every unit within the radius of
// the hit takes the same (0x61DAE6), so a target counts with its neighbours
internal float st_side_dps(struct st_side *a, struct st_side *b, int contact) {
    float s = 0;
    for (int i = 0; i < a->ng; i++) {
        struct st_group *u = &a->g[i];
        float area = u->st ? unit_stats_get_area(u->st) : 0, num = 0, den = 0;
        for (int j = 0; j < b->ng; j++) {
            struct st_group *t = &b->g[j];
            float d = stg_reaches(u, t) && !(contact && stg_inside_min(u, t)) ? stg_dps(u, t) : 0;
            if (d <= 0) continue;
            if (area > 0) { float nb = st_neighbours(b, j, area); d *= 1.0f + (nb < ST_AREA_MAX ? nb : ST_AREA_MAX); }
            num += b->g[j].count * d; den += b->g[j].count;
        }
        a->g[i].part = den > 0 ? a->g[i].count * num / den : 0;
        s += a->g[i].part;
    }
    return s;
}
// seconds side a walks under fire before it shoots: each mobile group's range short of side b's longest, over its
// speed (weighted by what it deals)
internal float st_close(struct st_side *a, struct st_side *b) {
    float rb = 0, t = 0, w = 0;
    for (int j = 0; j < b->ng; j++) if (stg_range(&b->g[j]) > rb) rb = stg_range(&b->g[j]);
    for (int i = 0; i < a->ng; i++) {
        struct st_group *u = &a->g[i];
        float d = 0, v = stg_speed(u), gap = rb - stg_range(u);
        for (int j = 0; j < b->ng && d <= 0; j++) d = stg_dps(u, &b->g[j]);
        if (d <= 0 || v <= 0) continue;
        t += a->g[i].count * d * (gap > 0 ? gap / v : 0); w += a->g[i].count * d;
    }
    return w > 0 ? t / w : 0;
}
// time to destroy hp: first own closing (no fire), then free fire while the enemy closes, then contact fire
internal float st_kill_time(float hp, float own_close, float their_close, float free_dps, float contact_dps) {
    if (free_dps > 0 && hp <= free_dps * their_close) return own_close + hp / free_dps;
    hp -= free_dps * their_close;
    return contact_dps > 0 ? own_close + their_close + hp / contact_dps : 1e9f;
}
struct st_result { float dps_free[2], dps_contact[2], close[2], time[2]; };
// the estimate: time[0] = how long a needs to destroy b, time[1] b to destroy a
internal void st_estimate(struct st_side *a, struct st_side *b, struct st_result *r) {
    r->dps_contact[0] = st_side_dps(a, b, 1); r->dps_contact[1] = st_side_dps(b, a, 1);
    r->dps_free[0] = st_side_dps(a, b, 0); r->dps_free[1] = st_side_dps(b, a, 0);
    r->close[0] = st_close(a, b); r->close[1] = st_close(b, a);
    r->time[0] = st_kill_time(b->hp, r->close[0], r->close[1], r->dps_free[0], r->dps_contact[0]);
    r->time[1] = st_kill_time(a->hp, r->close[1], r->close[0], r->dps_free[1], r->dps_contact[1]);
}

// how much stronger own is than foe, on the scale of summed hit points x attack compared before: the square root of
// the time foe needs to destroy own over the time own needs to destroy foe. That time ratio is own hit points x damage
// per second over foe's - both grow with the count, so twice the units of the same type make it 4 (the square law);
// summed hit points x attack makes it 2. Unrooted, the thresholds tuned on the old measure (muster below 0.5, militia
// from 3) moved: 0.5 meant 0.71 x the enemy's numbers, 3 meant 1.73 x. With range, minimum range and area damage.
// Nothing to fight: very large / 0.
internal float st_ratio(struct st_side *own, struct st_side *foe) {
    if (!foe->n || foe->hp <= 0) return 1e6f;
    if (!own->n || own->hp <= 0) return 0.0f;
    struct st_result r;
    st_estimate(own, foe, &r);
    if (r.time[0] >= 1e9f) return r.time[1] >= 1e9f ? 1.0f : 0.0f;
    return r.time[1] >= 1e9f ? 1e6f : sqrtf(r.time[1] / (r.time[0] > 0.1f ? r.time[0] : 0.1f));
}

// --- test command -------------------------------------------------------------------------------------------------

struct st_scan { int x, y, r2, ta, tb; struct st_side *side[16]; };
internal int st_counts(ee_unit *u, ee_unit_type *td) {   // soldiers, citizens and buildings that shoot
    if (is_citizen_type(unit_type_get_id(td))) return 1;
    if (is_soldier(td)) return 1;
    return (unit_type_get_class_id(td) == CLS_BUILDING || unit_type_get_class_id(td) == CLS_FORTRESS) && unit_type_get_attack(td) > 0 && u;
}
internal void st_scan_unit(ee_unit *u, void *ctx) {
    struct st_scan *f = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    int p = st_pidx(u);
    if (unit_get_hp(u) <= 0 || !td || p <= 0 || p > 15 || !f->side[p] || ivec2_dist2(unit_get_tile(u), iv2(f->x, f->y)) > f->r2) return;
    if (f->ta >= 0 && unit_type_get_id(td) != f->ta && unit_type_get_id(td) != f->tb) return;   // only these types
    if (st_counts(u, td)) st_side_add(f->side[p], u);
}
internal double st_ms(LARGE_INTEGER a, LARGE_INTEGER b) {
    LARGE_INTEGER fq;
    QueryPerformanceFrequency(&fq);
    return (double)(b.QuadPart - a.QuadPart) * 1000.0 / (double)fq.QuadPart;
}
internal struct st_side g_st_sides[16];

internal void cmd_stats(char *out, const char *args) {
    if (!strncmp(args, "table", 5)) {
        for (int r = 0; r < 6; r++) {
            REPLY("target class %d:", r);
            for (int c = 0; c < 6; c++) REPLY(" %.3g", ST_TABLE[c + r * 6]);
            REPLY("\n");
        }
        return;
    }
    if (!strncmp(args, "types", 5)) {   // types P [from to]: the player's stats of every type
        int p = 1, lo = 0, hi = 0x300;
        sscanf(args + 5, "%d %d %d", &p, &lo, &hi);
        ee_player *pl = st_player(p);
        if (!pl) { REPLY("error: stats types P [from to]\n"); return; }
        for (int t = lo; t < hi && t < 0x300; t++) {
            ee_unit_type *td = type_data(t); ee_unit_stats *st = st_rec(pl, t);
            if (!td || !st || unit_stats_get_attack(st) <= 0) continue;
            REPLY("%3d fam %2d class %d cat %2d weapon %2d: att %g hp %d reload %.2f s range %g (min %g) speed %g area %g armour", t,
                  unit_type_get_class_id(td), unit_type_get_damage_class(td), unit_type_get_family(td), unit_type_get_weapon(td),
                  unit_stats_get_attack(st), st_maxhp(td, st), st_reload_t(td, st), unit_stats_get_range(st), unit_type_get_min_range(td), unit_stats_get_speed(st), unit_stats_get_area(st));
            for (int c = 0; c < 6; c++) REPLY(" %g", unit_stats_get_armor(st, c));
            REPLY("\n");
        }
        return;
    }
    if (!strncmp(args, "vs", 2)) {   // vs P T Q U: one T of P against one U of Q, both ways
        int p, t, q, u;
        if (sscanf(args + 2, "%d %d %d %d", &p, &t, &q, &u) != 4 || !st_player(p) || !st_player(q) ||
            t < 0 || t >= 0x300 || u < 0 || u >= 0x300) { REPLY("error: stats vs P T Q U\n"); return; }
        ee_unit_type *atd = type_data(t); ee_unit_type *btd = type_data(u); ee_unit_stats *ast = st_rec(st_player(p), t), *bst = st_rec(st_player(q), u);
        if (!atd || !btd) { REPLY("error: no type\n"); return; }
        for (int side = 0; side < 2; side++) {
            ee_unit_type *xt = side ? btd : atd; ee_unit_stats *xs = side ? bst : ast; ee_unit_type *yt = side ? atd : btd; ee_unit_stats *ys = side ? ast : bst;
            int xp = side ? q : p, yp = side ? p : q, sh = st_shot_t(xt, xs, xp, yt, ys, yp), hp = st_maxhp(yt, ys);
            float rl = st_reload_t(xt, xs);
            int shots = sh ? (hp + sh - 1) / sh : 0;
            REPLY("%d of P%d -> %d of P%d: factor %.3g x table %.3g, %d a shot, every %.2f s: %.2f per s, kills (%d hp) in %d shots, %.1f s\n",
                  side ? u : t, xp, side ? t : u, yp, st_factor(xt, xp, yt, yp), ST_TABLE[unit_type_get_damage_class(xt) + unit_type_get_damage_class(yt) * 6],
                  sh, rl, sh / rl, hp, shots, shots ? (shots - 1) * rl : -1.0f);
        }
        return;
    }
    if (!strncmp(args, "fight", 5) || !strncmp(args, "bench", 5)) {
        // fight x y [r [T U]]: the sides' soldiers and shooting buildings within r of x,y (only types T and U)
        // bench N x y [r]: the same with every unit there repeated to N per side - the time it takes
        int bench = args[0] == 'b', want = 0, r = 12;
        static struct st_scan f;
        memset(&f, 0, sizeof f);
        f.ta = f.tb = -1;
        if (bench ? sscanf(args + 5, "%d %d %d %d", &want, &f.x, &f.y, &r) < 3
                  : sscanf(args + 5, "%d %d %d %d %d", &f.x, &f.y, &r, &f.ta, &f.tb) < 2) {   // ("detail" stops the types)
            REPLY("error: stats fight x y [r [T U]] | bench N x y [r]\n"); return;
        }
        f.r2 = r * r;
        for (int p = 1; p < 16; p++) { st_side_clear(&g_st_sides[p]); f.side[p] = &g_st_sides[p]; }
        LARGE_INTEGER t0, t1, t2;
        QueryPerformanceCounter(&t0);
        grid_each(st_scan_unit, &f);
        QueryPerformanceCounter(&t1);
        for (int p = 1; p < 16 && bench; p++) {   // repeat the units found to N
            struct st_side *s = &g_st_sides[p];
            int n0 = s->n;
            for (int i = 0; n0 && s->n < want && s->n < ST_UNITS; i = (i + 1) % n0) {
                struct st_group *g = &s->g[s->ug[i]];
                g->count++; g->hp += g->hp / (g->count - 1); s->hp += g->hp / g->count;
                s->ux[s->n] = (int16_t)(s->ux[i] + (s->n / n0) % 7); s->uy[s->n] = (int16_t)(s->uy[i] + (s->n / n0) / 7); s->ug[s->n] = s->ug[i];
                s->n++;
            }
        }
        for (int a = 1; a < 16; a++) {
            if (!g_st_sides[a].n) continue;
            for (int b = a + 1; b < 16; b++) {
                if (!g_st_sides[b].n || !st_player(a) || !is_enemy(st_player(a), st_player(b))) continue;
                struct st_result e;
                QueryPerformanceCounter(&t1);
                st_estimate(&g_st_sides[a], &g_st_sides[b], &e);
                QueryPerformanceCounter(&t2);
                float ta = e.time[0], tb = e.time[1];
                REPLY("P%d (%d units in %d types, %.0f hp, %.1f dmg/s, at contact %.1f, %.0f s closing) vs P%d (%d in %d, %.0f hp, %.1f/s, %.1f, %.0f s): "
                      "P%d needs %.0f s, P%d %.0f s -> P%d (%.2f); scan %.3f ms, estimate %.3f ms\n", a, g_st_sides[a].n, g_st_sides[a].ng,
                      g_st_sides[a].hp, e.dps_free[0], e.dps_contact[0], e.close[0], b, g_st_sides[b].n, g_st_sides[b].ng, g_st_sides[b].hp,
                      e.dps_free[1], e.dps_contact[1], e.close[1], a, ta, b, tb, ta <= tb ? a : b, ta <= tb ? tb / ta : ta / tb,
                      st_ms(t0, t1), st_ms(t1, t2));
                for (int side = 0; side < 2 && strstr(args, "detail"); side++) {   // fight x y r detail: each type's share (contact)
                    struct st_side *x = &g_st_sides[side ? b : a], *y = &g_st_sides[side ? a : b];
                    st_side_dps(x, y, 1);
                    REPLY("   P%d:", side ? b : a);
                    for (int k = 0; k < x->ng; k++) REPLY(" %d x%d %.0f/s;", x->g[k].type, x->g[k].count, x->g[k].part);
                    REPLY("\n");
                }
            }
        }
        for (int p = 1; p < 16; p++) st_side_clear(&g_st_sides[p]);
        return;
    }
    // engage P x y r: P's soldiers there attack the nearest enemy within 30 tiles; engage P x y r T tx ty: P's units of
    // type T there attack the enemy nearest to them within 15 tiles of tx,ty - any unit or building (aircraft from
    // their airport at a town)
    if (!strncmp(args, "engage", 6)) {
        int p, x, y, r, n = 0, nm = 0, nf = 0, tt = -1, tx = 0, ty = 0;
        int na = sscanf(args + 6, "%d %d %d %d %d %d %d", &p, &x, &y, &r, &tt, &tx, &ty);
        if ((na != 4 && na != 7) || !st_player(p)) { REPLY("error: stats engage P x y r [T tx ty]\n"); return; }
        if (na == 4) tt = -1;
        static ee_unit *mu[ST_UNITS], *fu[ST_UNITS];
        ee_map *map = game_map(); ee_grid *grid = map ? map_get_grid(map) : NULL;
        if (!grid) { REPLY("error: no map\n"); return; }
        for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
            ee_unit *u = it.u;
            ee_unit_type *td = it.td;
            int q = st_pidx(u);
            if (unit_get_hp(u) <= 0 || q <= 0 || q > 15) continue;
            ivec2 up = unit_get_tile(u);
            if (tt >= 0) {
                if (q == p && unit_type_get_id(td) == tt && ivec2_dist2(up, iv2(x, y)) <= r * r && nm < ST_UNITS) mu[nm++] = u;
                else if (q != p && ivec2_dist2(up, iv2(tx, ty)) <= 15 * 15 && is_enemy(st_player(p), st_player(q)) && nf < ST_UNITS) fu[nf++] = u;
                continue;
            }
            if (!st_counts(u, td)) continue;
            int d2 = ivec2_dist2(up, iv2(x, y));
            if (q == p && d2 <= r * r && nm < ST_UNITS) mu[nm++] = u;
            else if (q != p && d2 <= 40 * 40 && is_enemy(st_player(p), st_player(q)) && nf < ST_UNITS) fu[nf++] = u;
        }
        for (int i = 0; i < nm; i++) {
            ee_unit *u = mu[i], *best = NULL;
            float bd = tt >= 0 ? 1e9f : 30.0f * 30.0f;
            for (int k = 0; k < nf; k++) {
                float d2 = (float)ivec2_dist2(unit_get_tile(fu[k]), unit_get_tile(u));
                if (d2 < bd && st_shot(u, fu[k]) > 0) { bd = d2; best = fu[k]; }
            }
            if (best) n += unit_goal(u, goal_attack(u, best), 0, 0x1F40);
        }
        REPLY("ok: %d of P%d's %d units there attack\n", n, p, nm);
        return;
    }
    if (!strncmp(args, "typedump", 8)) {   // typedump T: type T's non-zero dwords from +0x400 (int / float)
        int t = 0, from = 0x400;
        sscanf(args + 8, "%d %x", &t, &from);
        ee_unit_type *td = type_data(t);
        if (!td || !mem_readable(td, 0x700)) { REPLY("error: no type %d\n", t); return; }
        int o = snprintf(out, 4096, "ok: type %d family %d:", t, unit_type_get_class_id(td));
        for (int off = from & ~3; off < from + 0x300 && off < 0x700 && o < 3900; off += 4) {
            int v = FIELD(td, off, int);
            if (v) o += snprintf(out + o, 4096 - o, " %x=%d/%g", off, v, FIELD(td, off, float));
        }
        snprintf(out + o, 4096 - o, "\n");
        return;
    }
    if (!strncmp(args, "garrison", 8)) {   // garrison: every type with a garrison capacity (type +0x550), its family
        int o = snprintf(out, 4096, "ok: types with a garrison capacity (type family capacity):");
        for (int t = 0; t < 724 && o < 3900; t++) {   // type ids end at 723
            ee_unit_type *td = type_data(t);
            if (!td || unit_type_get_id(td) != t || unit_type_get_garrison_capacity(td) <= 0) continue;
            o += snprintf(out + o, 4096 - o, " %d/%d/%d", t, unit_type_get_class_id(td), unit_type_get_garrison_capacity(td));
        }
        snprintf(out + o, 4096 - o, "\n");
        return;
    }
    if (!strncmp(args, "danger", 6)) {   // danger P: P's citizens within an enemy's reach + 3, by threat type
        int p;
        if (sscanf(args + 6, "%d", &p) != 1 || p < 1 || p > 15 || !st_player(p)) { REPLY("error: usage: stats danger P\n"); return; }
        static ee_unit *cit[4096], *foe[4096];
        int nc = 0, nf = 0, hit = 0, seen = 0, flagged = 0, ntype = 0, tt[32] = { 0 }, tn[32] = { 0 }, tf[32] = { 0 };
        float tr[32] = { 0 };
        ee_map *map = game_map(); ee_grid *grid = map ? map_get_grid(map) : NULL;
        if (!grid) { REPLY("error: no map\n"); return; }
        for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
            ee_unit *u = it.u;
            ee_unit_type *td = it.td;
            int q = st_pidx(u);
            if (unit_get_hp(u) <= 0 || q <= 0 || q > 15) continue;
            if (q == p && is_citizen_type(unit_type_get_id(td))) { if (nc < 4096) cit[nc++] = u; }
            else if (q != p && is_enemy(st_player(p), st_player(q)) && unit_type_get_attack(td) > 0 && nf < 4096) foe[nf++] = u;
        }
        for (int i = 0; i < nc; i++) {
            ee_unit *c = cit[i], *best = NULL;
            float bd = 1e9f;
            for (int k = 0; k < nf; k++) {
                float r = st_range(foe[k]) + 3.0f, d2 = (float)ivec2_dist2(unit_get_tile(foe[k]), unit_get_tile(c));
                if (d2 <= r * r && d2 < bd && st_shot(foe[k], c) > 0) { bd = d2; best = foe[k]; }
            }
            if (!best) continue;
            hit++;
            if (seen_by(best, st_player(p))) seen++;
            if (raiders_near(st_player(p), unit_get_tile(c), 6)) flagged++;
            int t = unit_type_get_id(unit_get_type_data(best)), j = 0;
            while (j < ntype && tt[j] != t) j++;
            if (j == ntype && ntype < 32) { tt[ntype] = t; tr[ntype] = st_range(best); tf[ntype++] = unit_type_get_class_id(unit_get_type_data(best)); }
            if (j < 32) tn[j]++;
        }
        int o = snprintf(out, 4096, "ok: P%d %d citizens, %d within an enemy's reach + 3 (%d of those enemies seen, %d flagged by raiders_near); nearest threat type(family) x n:",
                         p, nc, hit, seen, flagged);
        for (int j = 0; j < ntype && o < 3900; j++) o += snprintf(out + o, 4096 - o, " %d(%d) range %.0f x %d", tt[j], tf[j], tr[j], tn[j]);
        snprintf(out + o, 4096 - o, "\n");
        return;
    }
    REPLY("error: stats table | types P [from to] | vs P T Q U | fight x y [r [T U]] | bench N x y [r] | engage P x y r | danger P\n");
}
