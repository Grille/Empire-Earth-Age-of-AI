// Defences: how the AI fortifies its base. Included by eeaimod.c after attack.c (uses its defence_stats, is_wall).

// Callback over every unit of the unit grid (each unit once: only in the cell of its own position).
static void grid_each(void (*fn)(uint8_t *u, void *ctx), void *ctx) {
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return;
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    int cs = FIELD(grid, 8, int);
    unsigned ny = FIELD(grid, 0x14, unsigned), count = FIELD(grid, 0x10, unsigned) * ny;
    for (unsigned c = 0; cells && cs > 0 && c < count; c++)
        for (uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **); b && b < FIELD(cells + c * 16, 8, uint8_t **); b++) {
            uint8_t *u = *b;
            if (!u || !FIELD(u, 0x2C, uint8_t *)) continue;
            if (FIELD(u, 0x1C, int) / cs != (int)(c / ny) || FIELD(u, 0x20, int) / cs != (int)(c % ny)) continue;
            fn(u, ctx);
        }
}

// DEFENCE LAYOUT (aimod.ini [all] defence_layout, default on). The game's AI decides when and how many towers to
// build (its building plan, cm 0x46CD18) and puts each where its spot finder (0x466ED4) finds room along paths from
// the base - observed: 4 bronze towers scattered one by one on the side away from the enemy, none at the mines. A
// player masses towers where the attacks come from and guards his outlying mines. Here the AI's own tower requests
// (build queue entries without a position) are placed instead:
//   front: towards the nearest enemy building this AI has seen (up to 2 enemies; nothing seen yet -> the map centre),
//          defence_distance (10) tiles from the base centre, slots 3 tiles apart across that direction - close
//          enough that the towers cover each other (range 5+);
//   outposts: one slot on the enemy side of every own drop site 12+ tiles from the base (settlements at mines).
//   Order: 3 at the first front, 3 at the second, the outposts, then 2 more per front.
// Each slot is checked with the game's own placement test (0x62E940, every tile of the footprint) within 2 tiles.
// WALLS (defence_walls, default on): the game's wall manager (cm+0x48) works on a ring around the base one segment at
// a time and in practice leaves one stray piece with a gate. Walls belong in front of towers: once 2 towers stand at a
// front, an 11-tile wall arc goes up 3 tiles in front of them (the wall manager's own 0x497C3F(from, to, type): the
// citizens get the same build-wall order a player gives). The ends stay open - own units walk round, attackers on
// foot must break through or walk the length of it under tower fire.
#define FN_CAN_PLACE   0x0062E940   // cdecl char(player, type, x, y, w, h, 0)
#define FN_BUILD_WALL  0x00497C3F   // thiscall(wall manager, int from[2], int to[2], type, char)
static int g_def = 1, g_def_dist = 10, g_def_walls = 1, g_def_more = 1;
static uint8_t g_def_off[16];   // per player, pipe "defence P 0|1" (A/B scenarios)
static unsigned g_def_towers[16], g_def_walls_built[16];
struct front { int fx, fy, enemy; float dx, dy; int wall_tries; uint32_t wall_t; };
static struct def_plan {
    void *pl; uint32_t t; int bx, by, nf, nout;
    struct front f[2];
    int out[8][2];
    struct { int x, y; uint32_t t; } pend[8];   // slots given out recently (the construction may not show yet)
} g_plan[16];

static int is_tower_type(int t) { return t == 47 || t == 328 || (t >= 461 && t <= 466); }
static int can_place(void *pl, int type, int x, int y) {
    uint8_t *td = (type >= 0 && type < 2048) ? ((uint8_t **)0x009418A0)[type] : NULL;
    uint8_t *map = *(uint8_t **)G_MAP;
    int n = map ? (int)FIELD(map, 0x1816FC, uint32_t) : 0, sz = td ? FIELD(td, 0x58C, int) : 0;
    if (!td || x < 2 || y < 2 || x >= n - 2 || y >= n - 2) return 0;
    if (sz < 1) sz = 1;
    return ((char (__cdecl *)(void *, int, int, int, int, int, int))FN_CAN_PLACE)(pl, type, x, y, sz, sz, 0) != 0;
}

struct def_scan { void *pl; int bx, by; int d2[16], x[16], y[16]; int towers, tx[64], ty[64]; int walls, wx[256], wy[256]; };
static void def_scan_unit(uint8_t *u, void *ctx) {
    struct def_scan *s = ctx;
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    void *owner = FIELD(u, 0x18, void *);
    int fam = FIELD(td, 0x4B4, int), x = FIELD(u, 0x1C, int), y = FIELD(u, 0x20, int);
    if (fam != 2 && fam != 29) return;
    if (owner == s->pl) {
        if (is_tower_type(FIELD(td, 0x4B8, int)) && s->towers < 64) { s->tx[s->towers] = x; s->ty[s->towers++] = y; }
        else if (is_wall(FIELD(td, 0x4B8, int)) && s->walls < 256) { s->wx[s->walls] = x; s->wy[s->walls++] = y; }
        return;
    }
    if (!owner || !is_enemy(s->pl, owner) || !seen_by(u, s->pl)) return;
    unsigned pi = (unsigned)FIELD(owner, PL_INDEX, int) & 15;
    int d2 = (x - s->bx) * (x - s->bx) + (y - s->by) * (y - s->by);
    if (d2 < s->d2[pi]) { s->d2[pi] = d2; s->x[pi] = x; s->y[pi] = y; }
}

static struct def_scan g_scan;   // last scan (own towers), sim thread only
static void def_refresh(void *pl, unsigned idx, uint32_t now) {
    struct def_plan *p = &g_plan[idx];
    if (p->pl == pl && now >= p->t && now - p->t < 30000) return;
    if (p->pl != pl) memset(p, 0, sizeof *p);
    p->pl = pl; p->t = now;
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
    p->nf = p->nout = 0;
    if (!bm || FIELD(bm, 0x12C, int) == -10000) return;
    p->bx = FIELD(bm, 0x12C, int); p->by = FIELD(bm, 0x130, int);
    struct def_scan *s = &g_scan;
    memset(s, 0, sizeof *s);
    s->pl = pl; s->bx = p->bx; s->by = p->by;
    for (int i = 0; i < 16; i++) s->d2[i] = 0x7FFFFFFF;
    grid_each(def_scan_unit, s);
    struct front old[2];
    memcpy(old, p->f, sizeof old);
    for (int k = 0; k < 2; k++) {   // the two nearest enemies
        int best = -1;
        for (int i = 0; i < 16; i++)
            if (s->d2[i] != 0x7FFFFFFF && (best < 0 || s->d2[i] < s->d2[best]) && (k == 0 || i != p->f[0].enemy)) best = i;
        float dx, dy;
        if (best >= 0) { dx = (float)(s->x[best] - p->bx); dy = (float)(s->y[best] - p->by); }
        else if (k == 0) {   // nothing seen yet: towards the map centre
            uint8_t *map = *(uint8_t **)G_MAP;
            int n = map ? (int)FIELD(map, 0x1816FC, uint32_t) : 0;
            dx = (float)(n / 2 - p->bx); dy = (float)(n / 2 - p->by);
        } else break;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 4.0f) break;
        struct front *f = &p->f[p->nf];
        f->dx = dx / len; f->dy = dy / len; f->enemy = best;
        f->fx = p->bx + (int)lroundf(f->dx * (float)g_def_dist);
        f->fy = p->by + (int)lroundf(f->dy * (float)g_def_dist);
        // keep the wall state while the front stays where it was
        f->wall_tries = 0; f->wall_t = 0;
        for (int j = 0; j < 2; j++)
            if (abs(old[j].fx - f->fx) <= 2 && abs(old[j].fy - f->fy) <= 2) { f->wall_tries = old[j].wall_tries; f->wall_t = old[j].wall_t; }
        p->nf++;
    }
    static int d[64][3];
    int nd = drop_sites(pl, d, 64);
    for (int i = 0; i < nd && p->nout < 8; i++) {
        int dx = d[i][0] - p->bx, dy = d[i][1] - p->by;
        if (d[i][2] || dx * dx + dy * dy < 144 || !p->nf) continue;
        p->out[p->nout][0] = d[i][0] + (int)lroundf(p->f[0].dx * 3.0f);
        p->out[p->nout][1] = d[i][1] + (int)lroundf(p->f[0].dy * 3.0f);
        p->nout++;
    }
}

// Slot i of the plan in build order, 0 when there is none.
static int def_slot(struct def_plan *p, int i, int *x, int *y) {
    static const int first[3] = { 0, -1, 1 }, more[2] = { -2, 2 };
    for (int k = 0; k < p->nf; k++, i -= 3)
        if (i < 3) { *x = p->f[k].fx + (int)lroundf(-p->f[k].dy * 3.0f * first[i]); *y = p->f[k].fy + (int)lroundf(p->f[k].dx * 3.0f * first[i]); return 1; }
    if (i < p->nout) { *x = p->out[i][0]; *y = p->out[i][1]; return 1; }
    i -= p->nout;
    for (int k = 0; k < p->nf; k++, i -= 2)
        if (i < 2) { *x = p->f[k].fx + (int)lroundf(-p->f[k].dy * 3.0f * more[i]); *y = p->f[k].fy + (int)lroundf(p->f[k].dx * 3.0f * more[i]); return 1; }
    return 0;
}

// From the build manager's placement (econ.c hook_place): a position for a tower request without one.
static int defence_spot(void *pl, int type, int *ox, int *oy) {
    if (!g_def || !pl || !is_tower_type(type)) return 0;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (g_def_off[idx]) return 0;
    uint32_t now = game_time();
    def_refresh(pl, idx, now);
    struct def_plan *p = &g_plan[idx];
    struct def_scan *s = &g_scan;
    if (s->pl != pl) { p->t = now - 60000; def_refresh(pl, idx, now); }   // the scan belongs to another AI
    int x, y;
    for (int i = 0; def_slot(p, i, &x, &y); i++) {
        int taken = 0;
        for (int t = 0; t < s->towers && !taken; t++) taken = abs(s->tx[t] - x) <= 2 && abs(s->ty[t] - y) <= 2;
        for (int t = 0; t < 8 && !taken; t++)
            taken = p->pend[t].t && now - p->pend[t].t < 90000 && abs(p->pend[t].x - x) <= 2 && abs(p->pend[t].y - y) <= 2;
        if (taken) continue;
        for (int r = 0; r <= 2; r++)
            for (int ax = -r; ax <= r; ax++)
                for (int ay = -r; ay <= r; ay++) {
                    if ((abs(ax) != r && abs(ay) != r) || !can_place(pl, type, x + ax, y + ay)) continue;
                    *ox = x + ax; *oy = y + ay;
                    int o = 0;
                    for (int t = 1; t < 8; t++) if (p->pend[t].t < p->pend[o].t) o = t;
                    p->pend[o].x = *ox; p->pend[o].y = *oy; p->pend[o].t = now ? now : 1;
                    g_def_towers[idx]++;
                    if (g_log) fprintf(g_log, "# defence: P%u tower %d at %d,%d (slot %d; base %d,%d, front %d,%d)\n",
                                       idx, type, *ox, *oy, i, p->bx, p->by, p->nf ? p->f[0].fx : -1, p->nf ? p->f[0].fy : -1);
                    return 1;
                }
    }
    return 0;
}

// Tile (x, y) within 1 tile of the segment a-b (sampled at 11 points).
static int on_segment(int x, int y, const int *a, const int *b) {
    for (int j = 0; j <= 10; j++)
        if (abs(x - (a[0] + (b[0] - a[0]) * j / 10)) <= 1 && abs(y - (a[1] + (b[1] - a[1]) * j / 10)) <= 1) return 1;
    return 0;
}

// Walls: per AI every 20 s (econ_tick). Best wall type the player can build now (0x555710: available, affordable).
static void defence_tick(void *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_def || !g_def_walls || g_def_off[idx] || (now >= last[idx] && now - last[idx] < 20000)) return;
    last[idx] = now;
    def_refresh(pl, idx, now);
    struct def_plan *p = &g_plan[idx];
    struct def_scan *s = &g_scan;
    if (s->pl != pl) return;
    void *cm = FIELD(pl, 0xB90, void *), *wm = cm ? FIELD(cm, 0x48, void *) : NULL;
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
    if (!wm || !bm) return;
    int *stock = (int *)((uint8_t *)pl + PL_STOCK);
    // Towers of our own: the game's plan asks for few (P3: none in the first 15 minutes of a Bronze start, with 685
    // stone in stock - stone has hardly another use). While a slot of the plan is free and the stone allows (100 kept
    // back), one tower per minute goes into the build queue with its position; nothing while one is still queued.
    static uint32_t last_tower[16];
    if (g_def_more && (!last_tower[idx] || now < last_tower[idx] || now - last_tower[idx] >= 60000)) {
        int queued = 0;
        for (int t = 461; t <= 466 && !queued; t++) queued = bq_has(bm, t);
        if (!queued) queued = bq_has(bm, 47);
        int ttype = 0;
        for (int t = 466; t >= 461 && !ttype; t--)
            if (((char (__thiscall *)(void *, int, int, int))0x00555710)(pl, t, 0x20, 0)) ttype = t;
        if (!ttype && ((char (__thiscall *)(void *, int, int, int))0x00555710)(pl, 47, 0x20, 0)) ttype = 47;
        int x, y, e = current_epoch(pl);
        if (!queued && ttype && stock[4] >= unit_cost(pl, ttype, 4) + 100 && e >= 0 && e < 14 &&
            defence_spot(pl, ttype, &x, &y)) {
            int entry[3] = { ttype, x, y };
            ((void (__thiscall *)(void *, int *))0x00468C37)(bm + 4 + e * 0x14, entry);
            last_tower[idx] = now ? now : 1;
            if (g_log) fprintf(g_log, "# defence: P%u queues tower %d at %d,%d (stone %d)\n", idx, ttype, x, y, stock[4]);
        }
    }
    for (int k = 0; k < p->nf; k++) {
        struct front *f = &p->f[k];
        if (f->wall_tries >= 3 || (f->wall_t && now - f->wall_t < 180000)) continue;
        int ntow = 0;
        for (int t = 0; t < s->towers; t++)
            ntow += abs(s->tx[t] - f->fx) <= 7 && abs(s->ty[t] - f->fy) <= 7;
        if (ntow < 2) continue;
        int wtype = 0;
        for (int t = 472; t >= 467 && !wtype; t--)
            if (((char (__thiscall *)(void *, int, int, int))0x00555710)(pl, t, 0x20, 0)) wtype = t;
        if (!wtype) continue;
        int ok = 1;
        for (int r = 2; r < 7; r++) if (stock[r] < 11 * unit_cost(pl, wtype, r)) ok = 0;
        if (!ok) continue;
        int cx = f->fx + (int)lroundf(f->dx * 3.0f), cy = f->fy + (int)lroundf(f->dy * 3.0f);
        int a[2] = { cx + (int)lroundf(-f->dy * 5.0f), cy + (int)lroundf(f->dx * 5.0f) };
        int b[2] = { cx - (int)lroundf(-f->dy * 5.0f), cy - (int)lroundf(f->dx * 5.0f) };
        int have = 0;   // wall pieces standing on the arc (then a new order only repairs)
        for (int w = 0; w < s->walls; w++) have += on_segment(s->wx[w], s->wy[w], a, b);
        if (have >= 8) continue;
        ((void (__thiscall *)(void *, int *, int *, int, char))FN_BUILD_WALL)(wm, a, b, wtype, 0);
        f->wall_tries++;
        f->wall_t = now ? now : 1;
        g_def_walls_built[idx]++;
        if (g_log) fprintf(g_log, "# defence: P%u wall %d from %d,%d to %d,%d in front of %d towers (front %d,%d)\n",
                           idx, wtype, a[0], a[1], b[0], b[1], ntow, f->fx, f->fy);
        return;   // one order per tick
    }
}

// COUNTER-SIEGE (aimod.ini [all] counter_siege, default on): catapults outrange towers (bronze: 7 against 5) and the
// AI just watched its towers being shelled. A player sends a few soldiers out at them. Every 2 s per AI: enemy siege
// weapons (and rams) it can see within their range + 4 tiles of one of its buildings are threats; its soldiers within
// 20 tiles of one - idle, marching, or fighting something more than 4 tiles away - attack it (up to 5 each, nearest
// first, priority like the siege controller's orders). Only when they are at least 2x + 3 the enemy soldiers within
// 8 tiles of the siege weapon: a sally into the escorting army (siege_towers: 14 defenders against 36) lost the base
// that towers and walls would have held.
static int g_csiege = 1;
static unsigned g_csiege_orders[16], g_csiege_skipped[16];
static int is_soldier(uint8_t *td) {
    int fam = FIELD(td, 0x4B4, int), type = FIELD(td, 0x4B8, int);
    if (fam == 2 || fam == 6 || fam == 0x13 || fam == 0x16 || fam == 0x18 || fam == 0x19 || fam == 0x1D || fam == 0 || fam == 1)
        return 0;
    return !is_siege(type) && !is_ram(type) && type != 32 && type != 480;
}
struct cs_scan {
    void *pl;
    int nb, bx[256], by[256];           // own buildings
    int ns; uint8_t *sold[128];          // own soldiers
    int ne; uint8_t *esiege[16];         // enemy siege weapons seen
    int nx; int ex[256], ey[256];        // enemy soldiers (positions)
};
static void cs_unit(uint8_t *u, void *ctx) {
    struct cs_scan *s = ctx;
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    void *owner = FIELD(u, 0x18, void *);
    if (FIELD(u, 0x3C, int) <= 0 || !owner) return;
    int fam = FIELD(td, 0x4B4, int), type = FIELD(td, 0x4B8, int), x = FIELD(u, 0x1C, int), y = FIELD(u, 0x20, int);
    if (owner == s->pl) {
        if ((fam == 2 || fam == 29) && s->nb < 256) { s->bx[s->nb] = x; s->by[s->nb++] = y; }
        else if (is_soldier(td) && s->ns < 128) s->sold[s->ns++] = u;
    } else if (is_enemy(s->pl, owner)) {
        if ((is_siege(type) || is_ram(type)) && s->ne < 16 && seen_by(u, s->pl)) s->esiege[s->ne++] = u;
        else if (is_soldier(td) && s->nx < 256) { s->ex[s->nx] = x; s->ey[s->nx++] = y; }
    }
}
static void counter_siege(void *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_csiege || g_def_off[idx] || (now >= last[idx] && now - last[idx] < 2000)) return;
    last[idx] = now;
    static struct cs_scan s;
    memset(&s, 0, sizeof s);
    s.pl = pl;
    grid_each(cs_unit, &s);
    for (int i = 0; i < s.ne; i++) {
        uint8_t *e = s.esiege[i], *etd = FIELD(e, 0x2C, uint8_t *), *st = FIELD(e, 0x34, uint8_t *);
        int x = FIELD(e, 0x1C, int), y = FIELD(e, 0x20, int);
        float r = (st ? FIELD(st, 0x9C, float) : FIELD(etd, 0x420, float)) + 4.0f;
        int threat = 0;
        for (int b = 0; b < s.nb && !threat; b++)
            threat = (float)((s.bx[b] - x) * (s.bx[b] - x) + (s.by[b] - y) * (s.by[b] - y)) <= r * r;
        if (!threat) continue;
        int guards = 0;
        for (int k = 0; k < s.nx; k++) guards += abs(s.ex[k] - x) <= 8 && abs(s.ey[k] - y) <= 8;
        // responders: nearest first, up to 5
        int pick[5], np = 0, avail = 0, already = 0;
        for (int k = 0; k < s.ns; k++) {
            uint8_t *u = s.sold[k];
            if (!u) continue;   // already ordered this tick
            uint8_t *g = FIELD(u, 0x1EC, uint8_t *);
            int dx = FIELD(u, 0x1C, int) - x, dy = FIELD(u, 0x20, int) - y, d2 = dx * dx + dy * dy;
            if (d2 > 400) continue;
            if (g && *(uintptr_t *)g == VT_GOAL_ATTACK) {
                uint8_t *t = FIELD(g, 0x30, uint8_t *);
                if (t == e) { already++; s.sold[k] = NULL; continue; }
                if (t && abs(FIELD(t, 0x1C, int) - FIELD(u, 0x1C, int)) <= 4 && abs(FIELD(t, 0x20, int) - FIELD(u, 0x20, int)) <= 4)
                    continue;   // busy fighting close by
            }
            avail++;
            if (np < 5) pick[np++] = k;
            else {   // keep the 5 nearest
                int worst = 0, fd = -1;
                for (int j = 0; j < 5; j++) {
                    uint8_t *v = s.sold[pick[j]];
                    int vx = FIELD(v, 0x1C, int) - x, vy = FIELD(v, 0x20, int) - y;
                    if (vx * vx + vy * vy > fd) { fd = vx * vx + vy * vy; worst = j; }
                }
                if (d2 < fd) pick[worst] = k;
            }
        }
        if (already >= 5) continue;
        if (avail + already < 2 * guards + 3) { g_csiege_skipped[idx]++; continue; }
        int ordered = 0;
        for (int j = 0; j < np && already + ordered < 5; j++) {
            uint8_t *u = s.sold[pick[j]];
            if (unit_goal(u, goal_attack(u, e), 0, 0x1F40)) ordered++;
            s.sold[pick[j]] = NULL;   // one target per unit per tick
        }
        g_csiege_orders[idx] += (unsigned)ordered;
        if (ordered && g_log)
            fprintf(g_log, "# counter-siege: P%u at %.1f min: enemy type %d at %d,%d (%d guards) <- %d soldiers (%d already on it)\n",
                    idx, now / 60000.0, FIELD(etd, 0x4B8, int), x, y, guards, ordered, already);
    }
}

// HOME GUARD (aimod.ini [all] home_guard = soldiers per epoch, default 4; 0 = off): the unit manager (0x490AD6)
// only trains what is ordered into its request queues - mostly the next attack's group, ordered shortly before the
// attack. Between attacks nothing is trained: at minute 45 a Bronze Age AI had 10 soldiers, 25 requests in 5
// minutes and 1555 food + 1554 wood in its military account; a raid met 2-5 defenders. A player keeps a force at
// home. Every 10 s: fewer than max(6 + home_guard x epoch, home_guard_share x military room) own soldiers within 25
// tiles of the base (room = population cap x (1 - citizen share): 0.35 x 166 x 0.55 = 32) -> up to 5 more through
// the train hook (budget applies), type: the strongest trainable land soldier (substitution picks what the budget
// pays for).
static int g_home_guard = 4;
static float g_home_share = 0.35f;   // aimod.ini home_guard_share: of the military room (cap x (1 - citizen share))
static unsigned g_home_trained[16];
struct home_scan { void *pl; int bx, by, n; };
static void home_unit(uint8_t *u, void *ctx) {
    struct home_scan *h = ctx;
    if (FIELD(u, 0x18, void *) != h->pl || FIELD(u, 0x3C, int) <= 0 || !is_soldier(FIELD(u, 0x2C, uint8_t *))) return;
    int dx = FIELD(u, 0x1C, int) - h->bx, dy = FIELD(u, 0x20, int) - h->by;
    h->n += dx * dx + dy * dy <= 625;
}
static int home_soldiers(void *pl, int bx, int by) {
    struct home_scan h = { pl, bx, by, 0 };
    grid_each(home_unit, &h);
    return h.n;
}
static void home_guard(void *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_home_guard || g_def_off[idx] || (now >= last[idx] && now - last[idx] < 10000)) return;
    last[idx] = now;
    void *cm = FIELD(pl, 0xB90, void *), *um = cm ? FIELD(cm, 0x44, void *) : NULL;
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
    if (!um || !bm || FIELD(bm, 0x12C, int) == -10000) return;
    int e = current_epoch(pl), target = 6 + g_home_guard * e;
    // and a share of the military room: population cap (vf31 - follows the human's population unless Ignorieren) x
    // (1 - citizen share +0xBDC)
    int cap = ((int (__thiscall *)(void *, int))(*(void ***)pl)[31])(pl, 0);
    int room = (int)((float)cap * (1.0f - FIELD(pl, 0xBDC, float)) * g_home_share);
    if (room > target) target = room;
    int home = home_soldiers(pl, FIELD(bm, 0x12C, int), FIELD(bm, 0x130, int));
    if (home >= target) return;
    int big[7] = { 0, 0, 1 << 20, 1 << 20, 1 << 20, 1 << 20, 1 << 20 }, got = 0;
    int type = substitute_soldier(pl, idx, big, 1, &got);   // the strongest the game can train now
    if (!type) return;
    int req[4] = { type, target - home < 5 ? target - home : 5, 0, 0 };
    unsigned n = 0;
    hook_train_units(um, NULL, req, (uintptr_t)&n);
    g_home_trained[idx] += n;
    if (n && g_log) fprintf(g_log, "# home guard: P%u at %.1f min: %d of %d soldiers at home, %u more (type %d asked)\n",
                            idx, now / 60000.0, home, target, n, type);
}

// basemap P [r]: the map around AI player P's base (build manager +0x12C/+0x130), 2 characters per tile:
// T tower (own defence with a ground attack), W wall, G gate, C Town Center/Capitol/Settlement, b other own building,
// c citizen, s soldier, m mine (stone/gold/iron site), digits = other players' buildings/units (player index),
// o planned tower slot (defence layout).
struct basemap { void *pl; int x0, y0, r; char *g; };
static void basemap_unit(uint8_t *u, void *ctx) {
    struct basemap *m = ctx;
    int x = FIELD(u, 0x1C, int) - m->x0 + m->r, y = FIELD(u, 0x20, int) - m->y0 + m->r, w = 2 * m->r + 1;
    if (x < 0 || y < 0 || x >= w || y >= w) return;
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    void *owner = FIELD(u, 0x18, void *);
    int type = FIELD(td, 0x4B8, int), fam = FIELD(td, 0x4B4, int);
    char ch, *cell = &m->g[y * w + x];
    float r, a;
    if (owner != m->pl) {
        if (!owner) return;
        int pi = FIELD(owner, PL_INDEX, int);
        if (pi <= 0) return;   // nature / gaia
        ch = (char)('0' + pi % 10);
    } else if (is_wall(type)) ch = (type == 454 || type == 473 || type == 294 || type == 442 || type == 443 || type == 444) ? 'G' : 'W';
    else if (defence_stats(u, &r, &a)) ch = 'T';
    else if (type == 35 || type == 41 || type == 43 || type == 395) ch = 'C';
    else if (fam == 2 || fam == 29) ch = 'b';
    else if (type == 32 || type == 480) ch = 'c';
    else ch = 's';
    static const char prio[] = ".mcs0123456789bWGCT";   // what wins when several things share a tile
    if (strchr(prio, ch) > strchr(prio, *cell)) *cell = ch;
}
static void cmd_basemap(char *out, const char *args) {
    int p = 0, r = 20;
    sscanf(args, "%d %d", &p, &r);
    if (r < 5) r = 5;
    if (r > 40) r = 40;
    void *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    void *cm = pl ? FIELD(pl, 0xB90, void *) : NULL;
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
    if (!bm) { REPLY("error: no AI player %d\n", p); return; }
    static char g[81 * 81];
    int w = 2 * r + 1;
    memset(g, '.', (size_t)(w * w));
    struct basemap m = { pl, FIELD(bm, 0x12C, int), FIELD(bm, 0x130, int), r, g };
    void **zb = FIELD(pl, 0xB50, void **), **ze = FIELD(pl, 0xB54, void **);   // mines
    for (int z = 0; zb && zb + z < ze; z++)
        for (int t = 4; zb[z] && t < 7; t++) {
            uint8_t *s = ((uint8_t *(__thiscall *)(void *, int))FN_ZONE_SITE)(zb[z], t);
            if (!s) continue;
            int x = FIELD(s, 0x1C, int) - m.x0 + r, y = FIELD(s, 0x20, int) - m.y0 + r;
            if (x >= 0 && y >= 0 && x < w && y < w) g[y * w + x] = 'm';
        }
    grid_each(basemap_unit, &m);
    unsigned pi = (unsigned)p & 15;   // planned tower slots of the defence layout: o
    if (g_plan[pi].pl == pl) {
        int sx, sy;
        for (int i = 0; def_slot(&g_plan[pi], i, &sx, &sy); i++) {
            int gx = sx - m.x0 + r, gy = sy - m.y0 + r;
            if (gx >= 0 && gy >= 0 && gx < w && gy < w && g[gy * w + gx] == '.') g[gy * w + gx] = 'o';
        }
    }
    REPLY("P%d base %d,%d (x down, y right)\n", p, m.x0, m.y0);
    for (int x = 0; x < w; x++) {
        char line[2 * 81 + 2];
        for (int y = 0; y < w; y++) { line[2 * y] = g[y * w + x]; line[2 * y + 1] = ' '; }
        line[2 * w] = 0;
        REPLY("%s\n", line);
    }
}


// defence P [0|1]: switch the defence layout off/on for one player (scenario A/B tests)
static void cmd_defence(char *out, const char *args) {
    int p = -1, on = -1;
    sscanf(args, "%d %d", &p, &on);
    if (p < 0 || p > 15) { REPLY("error: defence P [0|1]\n"); return; }
    if (on >= 0) g_def_off[p] = !on;
    REPLY("defence P%d %s (towers placed %u, wall orders %u)\n", p, g_def_off[p] ? "off" : "on", g_def_towers[p], g_def_walls_built[p]);
}

// ENGINE GUARD: the damage effect (smoke/fire on damaged buildings, EEEDamagedEffect::vf19 0x4F8048) asks the
// object's model (unit vtable +0x54) for its damage points: 0x4D291F(index, state) looks up the model's map at +0x14C
// by state, then the map found there by the index. Twice (in ~60 runs, not reproducible, since the defence layout) the
// inner map had no head (+4 = 0) and the lookup crashed (0x4D3BD3, read 00000004). The call at 0x4F80A4 goes through
// a check: such a model gets "no damage points" (the effect then shows one), logged once per type with the unit.
#define CALL_DAMAGE_POINTS 0x004F80A4
#define FN_DAMAGE_POINTS   0x004D291F
__attribute__((used)) uint8_t *g_dmg_unit;   // the effect's unit: esi at the call site
static uint8_t *rb_find(uint8_t *map, unsigned key) {   // MSVC map: +4 head; node +0 left, +4 parent, +8 right, +0xC key, +0x15 nil
    uint8_t *head = FIELD(map, 4, uint8_t *), *res, *n;
    if (!head) return NULL;
    res = head; n = FIELD(head, 4, uint8_t *);
    while (n && !n[0x15]) {
        if (FIELD(n, 0xC, unsigned) < key) n = FIELD(n, 8, uint8_t *);
        else { res = n; n = FIELD(n, 0, uint8_t *); }
    }
    return (res == head || key < FIELD(res, 0xC, unsigned)) ? NULL : res;
}
__attribute__((used)) int __fastcall hook_damage_points(uint8_t *model, void *edx, int index, unsigned state) {
    (void)edx;
    uint8_t *n = model ? rb_find(model + 0x14C, state) : NULL, *sub = n ? FIELD(n, 0x10, uint8_t *) : NULL;
    if (n && (!sub || !FIELD(sub, 4, void *))) {
        static int logged;
        uint8_t *u = g_dmg_unit, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
        if (g_log && logged < 20) {
            logged++;
            fprintf(g_log, "# engine guard: damage points of a model without them - unit type %d owner P%d at %d,%d, state %u index %d\n",
                    td ? FIELD(td, 0x4B8, int) : -1, u && FIELD(u, 0x18, void *) ? FIELD(FIELD(u, 0x18, void *), PL_INDEX, int) : -1,
                    u ? FIELD(u, 0x1C, int) : -1, u ? FIELD(u, 0x20, int) : -1, state, index);
            fflush(g_log);
        }
        return 0;
    }
    return ((int (__thiscall *)(void *, int, unsigned))FN_DAMAGE_POINTS)(model, index, state);
}
__attribute__((naked, used)) void damage_points_thunk(void) {
    __asm__("movl %esi, _g_dmg_unit\n\t"
            "jmp @hook_damage_points@16");
}

static void defence_install(const char *aimod_ini, const char *autotest_ini) {
    g_def = cfg_int(aimod_ini, autotest_ini, 1, "defence_layout", 1);
    g_def_dist = cfg_int(aimod_ini, autotest_ini, 1, "defence_distance", 10);
    g_def_walls = cfg_int(aimod_ini, autotest_ini, 1, "defence_walls", 1);
    g_def_more = cfg_int(aimod_ini, autotest_ini, 1, "defence_towers", 1);
    g_csiege = cfg_int(aimod_ini, autotest_ini, 1, "counter_siege", 1);
    g_home_guard = cfg_int(aimod_ini, autotest_ini, 1, "home_guard", 4);
    g_home_share = cfg_float(aimod_ini, autotest_ini, 1, "home_guard_share", "0.35");
    BOOL guard = patch_call(CALL_DAMAGE_POINTS, FN_DAMAGE_POINTS, damage_points_thunk);
    if (g_log) fprintf(g_log, "# engine guard: damage points %s\n", guard ? "ok" : "FAILED");
    if (g_log) fprintf(g_log, "# defence: layout=%d distance=%d walls=%d own towers=%d counter-siege=%d home guard=%d/epoch, %.2f of the military room\n", g_def, g_def_dist, g_def_walls, g_def_more, g_csiege, g_home_guard, g_home_share);
}
