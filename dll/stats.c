// UNIT STATS: what a unit does to another, from the game's own data - so the AI's judgements follow the game (and
// units modders add). Included by eeaimod.c after defence.c.
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

// --- test command -------------------------------------------------------------------------------------------------

struct st_fight { int x, y, r2, ta, tb, n[16]; uint8_t *u[16][128]; };
static void st_fight_unit(uint8_t *u, void *ctx) {
    struct st_fight *f = ctx;
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    int p = st_pidx(u), dx = FIELD(u, 0x1C, int) - f->x, dy = FIELD(u, 0x20, int) - f->y;
    if (FIELD(u, 0x3C, int) <= 0 || !td || p <= 0 || p > 15 || dx * dx + dy * dy > f->r2 || f->n[p] >= 128) return;
    if (f->ta >= 0 && FIELD(td, 0x4B8, int) != f->ta && FIELD(td, 0x4B8, int) != f->tb) return;   // only these types
    if (!is_soldier(td) && FIELD(td, 0x4B4, int) != 2 && FIELD(td, 0x4B4, int) != 29) return;   // soldiers and buildings
    if (FIELD(td, 0x510, int) <= 0 && !is_soldier(td)) return;   // buildings that shoot
    f->u[p][f->n[p]++] = u;
}
// side a's damage per second at side b: each unit spreads its fire over the units of b it can hurt; contact: b has
// closed in (no unit inside a's minimum range counts); area damage counts the units of b next to each target
// (tile positions now)
static float st_side_dps(uint8_t **a, int na, uint8_t **b, int nb, int contact) {
    float s = 0;
    for (int i = 0; i < na; i++) {
        float sum = 0; int k = 0;
        uint8_t *ast = FIELD(a[i], 0x34, uint8_t *);
        float area = ast ? FIELD(ast, 0xA0, float) : 0;
        for (int j = 0; j < nb; j++) {
            float d = st_reaches(a[i], b[j]) && !(contact && st_inside_min(a[i], b[j])) ? st_dps(a[i], b[j]) : 0;
            if (d > 0 && area > 0)   // area damage: every unit within the radius of the hit takes the same (0x61DAE6)
                for (int m = 0; m < nb; m++) {
                    float dx = (float)(FIELD(b[m], 0x1C, int) - FIELD(b[j], 0x1C, int)), dy = (float)(FIELD(b[m], 0x20, int) - FIELD(b[j], 0x20, int));
                    if (m != j && dx * dx + dy * dy <= area * area) d += st_dps(a[i], b[m]);
                }
            if (d > 0) { sum += d; k++; }
        }
        if (k) s += sum / k;
    }
    return s;
}
// seconds side a walks under fire before it shoots: each mobile unit's range short of side b's longest, over its speed
// (weighted by what the unit deals)
static float st_close(uint8_t **a, int na, uint8_t **b, int nb) {
    float rb = 0, t = 0, w = 0;
    for (int j = 0; j < nb; j++) if (st_range(b[j]) > rb) rb = st_range(b[j]);
    for (int i = 0; i < na; i++) {
        float d = 0, v = st_speed(a[i]), gap = rb - st_range(a[i]);
        for (int j = 0; j < nb && d <= 0; j++) d = st_dps(a[i], b[j]);
        if (d <= 0 || v <= 0) continue;
        t += d * (gap > 0 ? gap / v : 0); w += d;
    }
    return w > 0 ? t / w : 0;
}
// time to destroy hp: first own closing (no fire), then free fire while the enemy closes, then contact fire
static float st_kill_time(float hp, float own_close, float their_close, float free_dps, float contact_dps) {
    if (free_dps > 0 && hp <= free_dps * their_close) return own_close + hp / free_dps;
    hp -= free_dps * their_close;
    return contact_dps > 0 ? own_close + their_close + hp / contact_dps : 1e9f;
}
static float st_side_hp(uint8_t **a, int na) { float h = 0; for (int i = 0; i < na; i++) h += FIELD(a[i], 0x3C, int); return h; }

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
    if (!strncmp(args, "fight", 5)) {   // fight x y r: the sides' soldiers and shooting buildings there
        static struct st_fight f;
        memset(&f, 0, sizeof f);
        int r = 12;
        f.ta = f.tb = -1;   // fight x y r T U: only units of types T and U
        if (sscanf(args + 5, "%d %d %d %d %d", &f.x, &f.y, &r, &f.ta, &f.tb) < 2) { REPLY("error: stats fight x y [r [T U]]\n"); return; }
        f.r2 = r * r;
        grid_each(st_fight_unit, &f);
        for (int a = 1; a < 16; a++) {
            if (!f.n[a]) continue;
            for (int b = 1; b < 16; b++) {
                if (!f.n[b] || b <= a || !st_player(a) || !is_enemy(st_player(a), st_player(b))) continue;
                float da = st_side_dps(f.u[a], f.n[a], f.u[b], f.n[b], 1), db = st_side_dps(f.u[b], f.n[b], f.u[a], f.n[a], 1);
                float fa = st_side_dps(f.u[a], f.n[a], f.u[b], f.n[b], 0), fb = st_side_dps(f.u[b], f.n[b], f.u[a], f.n[a], 0);
                float ha = st_side_hp(f.u[a], f.n[a]), hb = st_side_hp(f.u[b], f.n[b]);
                float ca = st_close(f.u[a], f.n[a], f.u[b], f.n[b]), cb = st_close(f.u[b], f.n[b], f.u[a], f.n[a]);
                float ta = st_kill_time(hb, ca, cb, fa, da), tb = st_kill_time(ha, cb, ca, fb, db);
                REPLY("P%d (%d units, %.0f hp, %.1f dmg/s, at contact %.1f, %.0f s closing) vs P%d (%d, %.0f hp, %.1f/s, %.1f, %.0f s): "
                      "P%d needs %.0f s, P%d %.0f s -> P%d (%.2f)\n", a, f.n[a], ha, fa, da, ca, b, f.n[b], hb, fb, db, cb,
                      a, ta, b, tb, ta <= tb ? a : b, ta <= tb ? tb / ta : ta / tb);
            }
        }
        return;
    }
    if (!strncmp(args, "engage", 6)) {   // engage P x y r: P's soldiers there attack the nearest enemy within 30 tiles
        static struct st_fight f, e;
        int p, n = 0;
        memset(&f, 0, sizeof f); memset(&e, 0, sizeof e);
        f.ta = f.tb = e.ta = e.tb = -1;
        if (sscanf(args + 6, "%d %d %d %d", &p, &f.x, &f.y, &f.r2) != 4 || !st_player(p)) { REPLY("error: stats engage P x y r\n"); return; }
        f.r2 *= f.r2; e.x = f.x; e.y = f.y; e.r2 = 40 * 40;
        grid_each(st_fight_unit, &f);
        grid_each(st_fight_unit, &e);
        for (int i = 0; i < f.n[p & 15]; i++) {
            uint8_t *u = f.u[p & 15][i], *best = NULL;
            float bd = 30.0f * 30.0f;
            for (int q = 1; q < 16; q++)
                for (int j = 0; j < e.n[q] && is_enemy(st_player(p), st_player(q)); j++) {
                    float dx = (float)(FIELD(e.u[q][j], 0x1C, int) - FIELD(u, 0x1C, int)), dy = (float)(FIELD(e.u[q][j], 0x20, int) - FIELD(u, 0x20, int));
                    if (dx * dx + dy * dy < bd && st_shot(u, e.u[q][j]) > 0) { bd = dx * dx + dy * dy; best = e.u[q][j]; }
                }
            if (best) n += unit_goal(u, goal_attack(u, best), 0, 0x1F40);
        }
        REPLY("ok: %d of P%d's %d units there attack\n", n, p, f.n[p & 15]);
        return;
    }
    REPLY("error: stats table | types P [from to] | vs P T Q U | fight x y [r [T U]] | engage P x y r\n");
}
