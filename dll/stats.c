// UNIT STATS: what a unit does to another, from the game's own data - so the AI's judgements follow the game (and
// units modders add). Included by eeaimod.c before defence.c (its grid_each and is_soldier declared here).
//
// [all] fight_estimate (default 0): 1 = the AI's fight decisions compare sides with this estimate (st_ratio) instead
// of summed hit points x attack: muster, hold the town, attack retreat, citizen militia (defence.c). The thresholds
// keep their meaning (the ratio is hit points x damage per second on each side). A/B 2026-10-17, overrun and assault
// (Continental 101, 10 seeds each, the defender): overrun alive 4 vs 3 of 10 (old vs estimate), kills 49 vs 31;
// assault alive 8 vs 9, workers 31.5 vs 22.8, losses 53 vs 62.5 - no gain: those are fights of numbers (90 against
// 20) where both agree, and where they differ the estimate rates the defenders lower, so it musters more. Off.
//
// Damage per shot (0x61DE61, docs/engine-notes.md): attack (the attacker's per-player stats +0xD8)
//   x table 0x8D2050[attacker class + target class x 6] (class = type +0x4C0; filled at load)
//   x hit factor 0x4D5F2C (thiscall on 0x917DE0: weapon kind type +0x4C8, target type, both player indices;
//     the table 0x8D200C[target category type +0x4C4][weapon kind], 0 = cannot hit)
//   - armour (the target's stats +0xE0 + attacker class x 4); rounded, at least 1; a height factor for land
//   attackers (ignored here). Area damage (stats +0xA0) is not counted.
// Time between shots: the unit's own +0x2A0, 0 = the class value (stats +0x138, per player; the game refills +0x2A0
// from it, 0x61E176); type +0x50C is the base. Stats record of a type for a player: player +0x874 vector,
// 0x4A670A size, 0x492969 at. Max hit points stats +0x144, range +0x9C.

#define ST_TABLE ((float *)0x008D2050)
static int g_fight_est = 0;   // [all] fight_estimate
static void grid_each(void (*fn)(uint8_t *u, void *ctx), void *ctx);   // defence.c
static int is_soldier(uint8_t *td);   // defence.c

static uint8_t *st_rec(void *pl, int type) {   // the player's stats record of a type
    if (!pl || type < 0) return NULL;
    uint8_t *vec = (uint8_t *)pl + 0x874;
    if ((unsigned)type >= ((unsigned (__thiscall *)(void *))0x004A670A)(vec)) return NULL;
    uint8_t **p = ((uint8_t **(__thiscall *)(void *, int))0x00492969)(vec, type);
    return p ? *p : NULL;
}
static void *st_player(int i) {   // any player (g_players has the AIs only)
    return i >= 0 && i < 16 && (unsigned)i < *(unsigned *)0x009183CC ? ((void **)0x00918374)[i] : NULL;
}
static float st_factor(uint8_t *atd, int api, uint8_t *ttd, int tpi) {   // the game's hit factor, 0 = cannot hit
    if (!atd || !ttd) return 0.0f;
    return ((float (__thiscall *)(void *, int, void *, int, int))0x004D5F2C)((void *)0x00917DE0, FIELD(atd, 0x4C8, int), ttd, api, tpi);
}
// damage of one shot of type atd (stats ast, player index api) at type ttd (stats tst, tpi); 0 = cannot hurt it
static int st_shot_t(uint8_t *atd, uint8_t *ast, int api, uint8_t *ttd, uint8_t *tst, int tpi) {
    if (!atd || !ttd) return 0;
    float att = ast ? FIELD(ast, 0xD8, float) : (float)FIELD(atd, 0x510, int);
    int ac = FIELD(atd, 0x4C0, int), tc = FIELD(ttd, 0x4C0, int);
    if (att <= 0 || ac < 0 || ac > 5 || tc < 0 || tc > 5) return 0;
    float f = st_factor(atd, api, ttd, tpi) * ST_TABLE[ac + tc * 6];
    if (f <= 0) return 0;
    float d = f * att - (tst ? FIELD(tst, 0xE0 + ac * 4, float) : 0.0f);
    int r = (int)(d + 0.5f);
    return r < 1 ? 1 : r;
}
static int st_pidx(uint8_t *u) { uint8_t *o = FIELD(u, 0x18, uint8_t *); return o ? FIELD(o, PL_INDEX, int) : 0; }
static int st_shot(uint8_t *a, uint8_t *t) {
    return st_shot_t(FIELD(a, 0x2C, uint8_t *), FIELD(a, 0x34, uint8_t *), st_pidx(a), FIELD(t, 0x2C, uint8_t *),
                     FIELD(t, 0x34, uint8_t *), st_pidx(t));
}
static float st_reload_t(uint8_t *atd, uint8_t *ast) {   // seconds between shots (class value)
    int ms = ast ? FIELD(ast, 0x138, int) : 0;
    if (ms <= 0 && atd) ms = FIELD(atd, 0x50C, int);
    return ms > 0 ? ms / 1000.0f : 1.0f;
}
static float st_reload(uint8_t *a) {   // the unit's own (scenario triggers change it), else its class's
    int own = FIELD(a, 0x2A0, int);
    return own > 0 ? own / 1000.0f : st_reload_t(FIELD(a, 0x2C, uint8_t *), FIELD(a, 0x34, uint8_t *));
}
// in a fight a shot takes longer than the reload: picking the next target, turning, stepping into range - duels of
// 2026-10-17 (scripts/zz_duel.sh): machine gunners (reload 1 s) fired every 3.3-5.1 s, Mk V tanks (4 s) every
// 4.6-7.1 s; about 2 s on every shot
#define ST_SHOT_OVERHEAD 2.0f
static float st_dps(uint8_t *a, uint8_t *t) { return st_shot(a, t) / (st_reload(a) + ST_SHOT_OVERHEAD); }
static float st_range(uint8_t *u) { uint8_t *st = FIELD(u, 0x34, uint8_t *); return st ? FIELD(st, 0x9C, float) : FIELD(FIELD(u, 0x2C, uint8_t *), 0x420, float); }
static float st_speed(uint8_t *u) { uint8_t *st = FIELD(u, 0x34, uint8_t *); return st ? FIELD(st, 0xAC, float) : 0.0f; }   // tiles/s, 0 = cannot move
// RANGE (the user: "it can even overpower all the other stats since a unit can be eliminated from range before ever
// getting a chance to fire back"): a unit that cannot move never hits one that outranges it; a unit that is
// outranged walks the difference under fire first (st_close)
static int st_reaches(uint8_t *a, uint8_t *t) { return st_speed(a) > 0 || st_range(a) + 0.5f >= st_range(t); }
static float st_minrange(uint8_t *u) { return FIELD(FIELD(u, 0x2C, uint8_t *), 0x41C, float); }
// once a mobile target has closed to its own range, an attacker whose minimum range is longer cannot hit it (artillery)
static int st_inside_min(uint8_t *a, uint8_t *t) { return st_speed(t) > 0 && st_minrange(a) > 0 && st_minrange(a) >= st_range(t); }
static int st_maxhp(uint8_t *td, uint8_t *st) { return st && FIELD(st, 0x144, int) > 0 ? FIELD(st, 0x144, int) : FIELD(td, 0x55C, int); }

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
struct st_group { uint8_t *rep; int type, owner, count; float hp, nbr[ST_RADII], part; };   // part: its share of the last st_side_dps
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
static void st_side_clear(struct st_side *s) { free(s->cnt); memset(s, 0, sizeof *s); }
static int st_side_add(struct st_side *s, uint8_t *u) {   // 0 when full
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    int type = FIELD(td, 0x4B8, int), owner = st_pidx(u), k = 0;
    while (k < s->ng && (s->g[k].type != type || s->g[k].owner != owner)) k++;
    if (k == s->ng) {
        if (k == ST_GROUPS || s->n == ST_UNITS) return 0;
        s->g[k].rep = u; s->g[k].type = type; s->g[k].owner = owner;
        s->ng++;
    }
    if (s->n == ST_UNITS) return 0;
    s->g[k].count++;
    s->g[k].hp += FIELD(u, 0x3C, int);
    s->hp += FIELD(u, 0x3C, int);
    s->ux[s->n] = (int16_t)FIELD(u, 0x1C, int); s->uy[s->n] = (int16_t)FIELD(u, 0x20, int); s->ug[s->n] = (uint8_t)k;
    s->n++;
    s->nr = 0;
    return 1;
}
// average units of the side within radius r of a unit of group k (itself not counted)
static float st_neighbours(struct st_side *s, int k, float r) {
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
static float st_side_dps(struct st_side *a, struct st_side *b, int contact) {
    float s = 0;
    for (int i = 0; i < a->ng; i++) {
        uint8_t *u = a->g[i].rep, *ast = FIELD(u, 0x34, uint8_t *);
        float area = ast ? FIELD(ast, 0xA0, float) : 0, num = 0, den = 0;
        for (int j = 0; j < b->ng; j++) {
            uint8_t *t = b->g[j].rep;
            float d = st_reaches(u, t) && !(contact && st_inside_min(u, t)) ? st_dps(u, t) : 0;
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
static float st_close(struct st_side *a, struct st_side *b) {
    float rb = 0, t = 0, w = 0;
    for (int j = 0; j < b->ng; j++) if (st_range(b->g[j].rep) > rb) rb = st_range(b->g[j].rep);
    for (int i = 0; i < a->ng; i++) {
        uint8_t *u = a->g[i].rep;
        float d = 0, v = st_speed(u), gap = rb - st_range(u);
        for (int j = 0; j < b->ng && d <= 0; j++) d = st_dps(u, b->g[j].rep);
        if (d <= 0 || v <= 0) continue;
        t += a->g[i].count * d * (gap > 0 ? gap / v : 0); w += a->g[i].count * d;
    }
    return w > 0 ? t / w : 0;
}
// time to destroy hp: first own closing (no fire), then free fire while the enemy closes, then contact fire
static float st_kill_time(float hp, float own_close, float their_close, float free_dps, float contact_dps) {
    if (free_dps > 0 && hp <= free_dps * their_close) return own_close + hp / free_dps;
    hp -= free_dps * their_close;
    return contact_dps > 0 ? own_close + their_close + hp / contact_dps : 1e9f;
}
struct st_result { float dps_free[2], dps_contact[2], close[2], time[2]; };
// the estimate: time[0] = how long a needs to destroy b, time[1] b to destroy a
static void st_estimate(struct st_side *a, struct st_side *b, struct st_result *r) {
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
static float st_ratio(struct st_side *own, struct st_side *foe) {
    if (!foe->n || foe->hp <= 0) return 1e6f;
    if (!own->n || own->hp <= 0) return 0.0f;
    struct st_result r;
    st_estimate(own, foe, &r);
    if (r.time[0] >= 1e9f) return r.time[1] >= 1e9f ? 1.0f : 0.0f;
    return r.time[1] >= 1e9f ? 1e6f : sqrtf(r.time[1] / (r.time[0] > 0.1f ? r.time[0] : 0.1f));
}

// --- test command -------------------------------------------------------------------------------------------------

struct st_scan { int x, y, r2, ta, tb; struct st_side *side[16]; };
static int st_counts(uint8_t *u, uint8_t *td) {   // soldiers, citizens and buildings that shoot
    if (FIELD(td, 0x4B8, int) == 32 || FIELD(td, 0x4B8, int) == 480) return 1;
    if (is_soldier(td)) return 1;
    return (FIELD(td, 0x4B4, int) == 2 || FIELD(td, 0x4B4, int) == 29) && FIELD(td, 0x510, int) > 0 && u;
}
static void st_scan_unit(uint8_t *u, void *ctx) {
    struct st_scan *f = ctx;
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    int p = st_pidx(u), dx = FIELD(u, 0x1C, int) - f->x, dy = FIELD(u, 0x20, int) - f->y;
    if (FIELD(u, 0x3C, int) <= 0 || !td || p <= 0 || p > 15 || !f->side[p] || dx * dx + dy * dy > f->r2) return;
    if (f->ta >= 0 && FIELD(td, 0x4B8, int) != f->ta && FIELD(td, 0x4B8, int) != f->tb) return;   // only these types
    if (st_counts(u, td)) st_side_add(f->side[p], u);
}
static double st_ms(LARGE_INTEGER a, LARGE_INTEGER b) {
    LARGE_INTEGER fq;
    QueryPerformanceFrequency(&fq);
    return (double)(b.QuadPart - a.QuadPart) * 1000.0 / (double)fq.QuadPart;
}
static struct st_side g_st_sides[16];

static void cmd_stats(char *out, const char *args) {
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
        void *pl = st_player(p);
        if (!pl) { REPLY("error: stats types P [from to]\n"); return; }
        for (int t = lo; t < hi && t < 0x300; t++) {
            uint8_t *td = ((uint8_t **)G_TYPES)[t], *st = st_rec(pl, t);
            if (!td || !st || FIELD(st, 0xD8, float) <= 0) continue;
            REPLY("%3d fam %2d class %d cat %2d weapon %2d: att %g hp %d reload %.2f s range %g (min %g) speed %g area %g armour", t,
                  FIELD(td, 0x4B4, int), FIELD(td, 0x4C0, int), FIELD(td, 0x4C4, int), FIELD(td, 0x4C8, int),
                  FIELD(st, 0xD8, float), st_maxhp(td, st), st_reload_t(td, st), FIELD(st, 0x9C, float), FIELD(td, 0x41C, float), FIELD(st, 0xAC, float), FIELD(st, 0xA0, float));
            for (int c = 0; c < 6; c++) REPLY(" %g", FIELD(st, 0xE0 + c * 4, float));
            REPLY("\n");
        }
        return;
    }
    if (!strncmp(args, "vs", 2)) {   // vs P T Q U: one T of P against one U of Q, both ways
        int p, t, q, u;
        if (sscanf(args + 2, "%d %d %d %d", &p, &t, &q, &u) != 4 || !st_player(p) || !st_player(q) ||
            t < 0 || t >= 0x300 || u < 0 || u >= 0x300) { REPLY("error: stats vs P T Q U\n"); return; }
        uint8_t *atd = ((uint8_t **)G_TYPES)[t], *btd = ((uint8_t **)G_TYPES)[u], *ast = st_rec(st_player(p), t), *bst = st_rec(st_player(q), u);
        if (!atd || !btd) { REPLY("error: no type\n"); return; }
        for (int side = 0; side < 2; side++) {
            uint8_t *xt = side ? btd : atd, *xs = side ? bst : ast, *yt = side ? atd : btd, *ys = side ? ast : bst;
            int xp = side ? q : p, yp = side ? p : q, sh = st_shot_t(xt, xs, xp, yt, ys, yp), hp = st_maxhp(yt, ys);
            float rl = st_reload_t(xt, xs);
            int shots = sh ? (hp + sh - 1) / sh : 0;
            REPLY("%d of P%d -> %d of P%d: factor %.3g x table %.3g, %d a shot, every %.2f s: %.2f per s, kills (%d hp) in %d shots, %.1f s\n",
                  side ? u : t, xp, side ? t : u, yp, st_factor(xt, xp, yt, yp), ST_TABLE[FIELD(xt, 0x4C0, int) + FIELD(yt, 0x4C0, int) * 6],
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
        static uint8_t *mu[ST_UNITS], *fu[ST_UNITS];
        uint8_t *map = *(uint8_t **)G_MAP, *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
        if (!grid) { REPLY("error: no map\n"); return; }
        int cells = FIELD(grid, 0x10, int) * FIELD(grid, 0x14, int);
        uint8_t *cl = FIELD(grid, 4, uint8_t *);
        for (int ci = 0; ci < cells; ci++)
            for (uint8_t **bp = FIELD(cl + ci * 16, 4, uint8_t **); bp && bp < FIELD(cl + ci * 16, 8, uint8_t **); bp++) {
                uint8_t *u = *bp, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                int q = u ? st_pidx(u) : 0;
                if (!td || FIELD(u, 0x3C, int) <= 0 || q <= 0 || q > 15) continue;
                if (tt >= 0) {
                    int dx = FIELD(u, 0x1C, int) - x, dy = FIELD(u, 0x20, int) - y, ex = FIELD(u, 0x1C, int) - tx, ey = FIELD(u, 0x20, int) - ty;
                    if (q == p && FIELD(td, 0x4B8, int) == tt && dx * dx + dy * dy <= r * r && nm < ST_UNITS) mu[nm++] = u;
                    else if (q != p && ex * ex + ey * ey <= 15 * 15 && is_enemy(st_player(p), st_player(q)) && nf < ST_UNITS) fu[nf++] = u;
                    continue;
                }
                if (!st_counts(u, td)) continue;
                int dx = FIELD(u, 0x1C, int) - x, dy = FIELD(u, 0x20, int) - y, d2 = dx * dx + dy * dy;
                if (q == p && d2 <= r * r && nm < ST_UNITS) mu[nm++] = u;
                else if (q != p && d2 <= 40 * 40 && is_enemy(st_player(p), st_player(q)) && nf < ST_UNITS) fu[nf++] = u;
            }
        for (int i = 0; i < nm; i++) {
            uint8_t *u = mu[i], *best = NULL;
            float bd = tt >= 0 ? 1e9f : 30.0f * 30.0f;
            for (int k = 0; k < nf; k++) {
                float dx = (float)(FIELD(fu[k], 0x1C, int) - FIELD(u, 0x1C, int)), dy = (float)(FIELD(fu[k], 0x20, int) - FIELD(u, 0x20, int));
                if (dx * dx + dy * dy < bd && st_shot(u, fu[k]) > 0) { bd = dx * dx + dy * dy; best = fu[k]; }
            }
            if (best) n += unit_goal(u, goal_attack(u, best), 0, 0x1F40);
        }
        REPLY("ok: %d of P%d's %d units there attack\n", n, p, nm);
        return;
    }
    if (!strncmp(args, "typedump", 8)) {   // typedump T: type T's non-zero dwords from +0x400 (int / float)
        int t = 0, from = 0x400;
        sscanf(args + 8, "%d %x", &t, &from);
        uint8_t *td = t >= 0 && t < 1024 ? ((uint8_t **)0x009418A0)[t] : NULL;
        if (!td || IsBadReadPtr(td, 0x700)) { REPLY("error: no type %d\n", t); return; }
        int o = snprintf(out, 4096, "ok: type %d family %d:", t, FIELD(td, 0x4B4, int));
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
            uint8_t *td = ((uint8_t **)0x009418A0)[t];
            if (!td || FIELD(td, 0x4B8, int) != t || FIELD(td, 0x550, int) <= 0) continue;
            o += snprintf(out + o, 4096 - o, " %d/%d/%d", t, FIELD(td, 0x4B4, int), FIELD(td, 0x550, int));
        }
        snprintf(out + o, 4096 - o, "\n");
        return;
    }
    if (!strncmp(args, "danger", 6)) {   // danger P: P's citizens within an enemy's reach + 3, by threat type
        int p;
        if (sscanf(args + 6, "%d", &p) != 1 || p < 1 || p > 15 || !st_player(p)) { REPLY("error: usage: stats danger P\n"); return; }
        static uint8_t *cit[4096], *foe[4096];
        int nc = 0, nf = 0, hit = 0, seen = 0, flagged = 0, ntype = 0, tt[32] = { 0 }, tn[32] = { 0 }, tf[32] = { 0 };
        float tr[32] = { 0 };
        uint8_t *map = *(uint8_t **)G_MAP, *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
        if (!grid) { REPLY("error: no map\n"); return; }
        int cells = FIELD(grid, 0x10, int) * FIELD(grid, 0x14, int);
        uint8_t *cl = FIELD(grid, 4, uint8_t *);
        for (int ci = 0; ci < cells; ci++)
            for (uint8_t **bp = FIELD(cl + ci * 16, 4, uint8_t **); bp && bp < FIELD(cl + ci * 16, 8, uint8_t **); bp++) {
                uint8_t *u = *bp, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                int q = u ? st_pidx(u) : 0;
                if (!td || FIELD(u, 0x3C, int) <= 0 || q <= 0 || q > 15) continue;
                if (q == p && (FIELD(td, 0x4B8, int) == 32 || FIELD(td, 0x4B8, int) == 480)) { if (nc < 4096) cit[nc++] = u; }
                else if (q != p && is_enemy(st_player(p), st_player(q)) && FIELD(td, 0x510, int) > 0 && nf < 4096) foe[nf++] = u;
            }
        for (int i = 0; i < nc; i++) {
            uint8_t *c = cit[i], *best = NULL;
            float bd = 1e9f;
            for (int k = 0; k < nf; k++) {
                float dx = (float)(FIELD(foe[k], 0x1C, int) - FIELD(c, 0x1C, int)), dy = (float)(FIELD(foe[k], 0x20, int) - FIELD(c, 0x20, int));
                float r = st_range(foe[k]) + 3.0f, d2 = dx * dx + dy * dy;
                if (d2 <= r * r && d2 < bd && st_shot(foe[k], c) > 0) { bd = d2; best = foe[k]; }
            }
            if (!best) continue;
            hit++;
            if (seen_by(best, st_player(p))) seen++;
            if (raiders_near(st_player(p), FIELD(c, 0x1C, int), FIELD(c, 0x20, int), 6)) flagged++;
            int t = FIELD(FIELD(best, 0x2C, uint8_t *), 0x4B8, int), j = 0;
            while (j < ntype && tt[j] != t) j++;
            if (j == ntype && ntype < 32) { tt[ntype] = t; tr[ntype] = st_range(best); tf[ntype++] = FIELD(FIELD(best, 0x2C, uint8_t *), 0x4B4, int); }
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
