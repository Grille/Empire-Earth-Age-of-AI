// LANDING ([all] landing, default 0 - experimental): a landing behind the enemy's lines. Included by eeaimod.c after
// assault.c (uses its area ids, as_home_threat, as_enemy_power, as_enemy_near, as_objective).
//
// The user (reference save): "my back is basically completely undefended - landing with ~3 full transports and a few
// citizens for a defended bridgehead could cause a lot of trouble; going over land means going through the heavily
// defended main front". The game's own transport handling hardly works (one transport per AI, loading for 8+
// minutes while its land subgroups wait elsewhere - docs/engine-notes.md). Here the mod does it itself:
// - Plan (every 30 s while idle, while an attack is on - also with an enemy army at its door): a landing site -
//   open land at the shore of the sea its dock lies on, on the continent of the target's buildings it has seen,
//   the weakest defended (the user: "the AI will rarely find a fully undefended spot - it just has to choose the
//   weakest one and bring an army that actually has a chance"), then the most buildings within 20 tiles; 15+
//   soldiers in land attack subgroups at home, as strong as 1.4 times what holds the beach (ld_strength: groups,
//   not units summed).
// - Ships: its transports (the newest type of its epoch), up to landing_transports (3), the missing ones trained
//   (priority orders); not there within 6 minutes: given up for 10 minutes.
// - Board: whole land subgroups at home (their updates skipped while the landing leads them, as the transports'
//   subgroups), up to 10 per transport; the game's own load goal (EEUGLoadTransport 0x61487F on the transport with
//   the passenger list, each passenger the goal's vf4) - again every 30 s for those not aboard; after 3 minutes it
//   sails with what is aboard (half at least, else given up).
// - Sail and unload: EEUGUnloadTransport (0x615A1F, transport, tile x, y) at the site. Committed: no turning back
//   while transports with soldiers aboard are afloat (towers in the back can sink them - land before). Up to 4
//   warships go along (their subgroups led too): ahead to the water in front of the site, attacking the defences and
//   warships there so the towers shoot at them, then shelling the shore. Towers covering that water count against
//   a site (the beach itself must be out of every known tower's reach).
// - Ashore: one group - fights what comes within 12 tiles of its centre, else marches on the nearest building of the
//   target it has seen (attacking it within 6 tiles). A quarter left: released to the game.
static int g_landing = 0, g_land_ships = 3;   // aimod.ini [all] landing, landing_transports
static int g_land_trace = 0;   // test: landing_trace=1 - each transport every tick while soldiers are aboard
static int g_land_knows = 0;   // test: landing_knows_map=1 - every building of the target counts, seen or not
static int g_land_flag = 0;   // the unload goal's last argument: the AI's code passes 1, the player's command 0 (test: landing_flag)
#define LD_PASS 64
// a group's strength: sqrt(hit points x attack, each summed over the group) - N soldiers wear a tower down N times
// faster than one, so a group's hit points x attack per unit summed (unit_power) rated a tower as 49 soldiers
struct ld_def { float soldiers, towers, water, all; int n, assumed; };   // strengths: soldiers, defences reaching the beach, over the water, beach together
static struct landing {
    int phase;                           // 0 idle, 1 ships, 2 board, 3 sail, 4 ashore
    uint8_t *target;
    int sx, sy, ocean, wx, wy;           // landing site, the sea, the water in front of it
    uint8_t *ships[4]; int nships;
    uint8_t *esc[12]; int nesc;          // warships (up to 4) and aircraft (up to 8) along
    uint8_t *pass[LD_PASS]; int npass, launch_n;
    uint8_t *frozen[48]; int nfrozen;    // subgroups led by the landing
    uint8_t *obj; int ox, oy;
    uint32_t since, last, next_try, reissue, trained_t, boarded_t;
    int switched;                        // the beach changed once at the sail check
    int tr_missing, tr_type, tr_got;     // transports still to come, their type, taken at the last order
    int ex, ey, ewx, ewy, gathered;      // where they board: the shore nearest the passengers, the water in front
    struct ld_def def;                   // the beach's defence when planned
    int all_ashore, tries_at[4], ux[4], uy[4];   // each transport's own unload point
    unsigned tries, landings, aborted, orders;
} g_ld[16];

static const struct { int epoch, type; } k_transports[] = { { 13, 114 }, { 10, 113 }, { 8, 112 }, { 4, 111 }, { 3, 110 }, { 2, 313 } };
static int is_transport(int t) {
    for (size_t k = 0; k < sizeof k_transports / sizeof *k_transports; k++) if (k_transports[k].type == t) return 1;
    return 0;
}

static int landing_leads(uint8_t *sg) {
    void *pl = FIELD(sg, 0x190, void *);
    unsigned pi = pl ? (unsigned)FIELD(pl, PL_INDEX, int) & 15 : 0;
    if (!g_landing || !pl || !g_ld[pi].phase) return 0;
    for (int k = 0; k < g_ld[pi].nfrozen; k++) if (g_ld[pi].frozen[k] == sg) return 1;
    return 0;
}
// the transports still missing: their cost is kept from every other unit order (econ.c's reserve) while the landing
// waits for them - P3's 3000 of each went on other units within 4 minutes, its transports never came
static void landing_reserve(void *pl, int reserve[7]) {
    struct landing *l = &g_ld[(unsigned)FIELD(pl, PL_INDEX, int) & 15];
    if (!g_landing || l->phase != 1 || g_landing_order || l->tr_missing <= 0 || !l->tr_type) return;
    for (int r = 2; r < 7; r++) reserve[r] += l->tr_missing * unit_cost(pl, l->tr_type, r);
}
static int landing_busy(void *pl) {   // boarding, at sea or ashore
    return g_landing && g_ld[(unsigned)FIELD(pl, PL_INDEX, int) & 15].phase >= 2;
}
static int landing_has_unit(void *pl, uint8_t *u) {   // a transport, warship or soldier of a landing under way
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    struct landing *l = &g_ld[pi];
    if (!g_landing || l->phase < 2) return 0;
    for (int k = 0; k < l->nships; k++) if (l->ships[k] == u) return 1;
    for (int k = 0; k < l->nesc; k++) if (l->esc[k] == u) return 1;
    for (int k = 0; k < l->npass; k++) if (l->pass[k] == u) return 1;
    return 0;
}
#define VT_ASG_SEA 0x00826F10
#define FN_ASG_SEA_UPDATE 0x0044ED11   // EECPASGSea slot 90: update(now)
static void __fastcall hook_asg_sea_update(uint8_t *sg, void *edx, int now) {
    (void)edx;
    if (landing_leads(sg)) return;
    ((void (__thiscall *)(void *, int))FN_ASG_SEA_UPDATE)(sg, now);
}
#define VT_ASG_AIR 0x00826610
#define FN_ASG_AIR_UPDATE 0x00442DBA   // EECPASGAir slot 90: update(now)
static void __fastcall hook_asg_air_update(uint8_t *sg, void *edx, int now) {
    (void)edx;
    if (landing_leads(sg)) return;
    ((void (__thiscall *)(void *, int))FN_ASG_AIR_UPDATE)(sg, now);
}
#define VT_ASG_SEA_TRANSPORT 0x008270F8
#define FN_ASG_SEA_TRANSPORT_UPDATE 0x0044FB92   // EECPASGSeaTransport slot 90: update(now)
static void __fastcall hook_asg_sea_transport_update(uint8_t *sg, void *edx, int now) {
    (void)edx;
    if (landing_leads(sg)) return;
    ((void (__thiscall *)(void *, int))FN_ASG_SEA_TRANSPORT_UPDATE)(sg, now);
}

// nothing (trees, buildings, units of any owner) on the 3x3 tiles around (x, y): room to unload - a landing site
// on a wooded coast: every unload there ended at once, the transports circling with the soldiers aboard
static int ld_tile_free(int x, int y) {
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid || x < 1 || y < 1) return 0;
    int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
    uint8_t *cells = FIELD(grid, 4, uint8_t *);
    for (int gx = (x - 3) / cs; cells && cs > 0 && gx <= (x + 3) / cs; gx++)
        for (int gy = (y - 3) / cs; gy <= (y + 3) / cs; gy++) {
            if (gx < 0 || gy < 0 || gx >= nx || gy >= ny) continue;
            uint8_t *cell = cells + (gx * ny + gy) * 16;
            for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **); b++) {
                uint8_t *u = *b;
                if (!u || !FIELD(u, 0x2C, uint8_t *)) continue;
                if (abs(FIELD(u, 0x1C, int) - x) <= 1 && abs(FIELD(u, 0x20, int) - y) <= 1) return 0;
            }
        }
    return 1;
}

static unsigned ocean_near(int x, int y, int r, int *wx, int *wy) {   // an ocean id within r tiles (and that tile)
    for (int d = 1; d <= r; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d ? 1 : 2 * d)) {
                unsigned o = area_id(x + dx, y + dy, 1);
                if (o) { if (wx) { *wx = x + dx; *wy = y + dy; } return o; }
            }
    return 0;
}

struct ld_scan { void *pl; uint8_t *target; int nb, bx[2000], by[2000]; int nd, dx[16], dy[16]; int nt; uint8_t *tr[32]; int nw; uint8_t *war[16]; int na; uint8_t *air[16]; };
static int is_fishing_boat(int t) { return t == 108 || t == 109 || t == 211 || t == 332 || t == 365; }
static void ld_unit(uint8_t *u, void *ctx) {
    struct ld_scan *s = ctx;
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    void *o = FIELD(u, 0x18, void *);
    if (FIELD(u, 0x3C, int) <= 0 || !o) return;
    int fam = FIELD(td, 0x4B4, int), t = FIELD(td, 0x4B8, int);
    if (o == s->target && (fam == 2 || fam == 29) && !is_wall(t) && (g_land_knows || seen_by(u, s->pl)) && s->nb < 2000) {
        s->bx[s->nb] = FIELD(u, 0x1C, int); s->by[s->nb++] = FIELD(u, 0x20, int);
    } else if (o == s->pl && t == 36 && s->nd < 16) {
        s->dx[s->nd] = FIELD(u, 0x1C, int); s->dy[s->nd++] = FIELD(u, 0x20, int);
    } else if (o == s->pl && is_transport(t) && FIELD(u, 0x1C, int) >= 0 && s->nt < 32) s->tr[s->nt++] = u;
    else if (o == s->pl && fam == 4 && !is_transport(t) && !is_fishing_boat(t) && FIELD(td, 0x510, int) > 0 && s->nw < 16)
        s->war[s->nw++] = u;
    else if (o == s->pl && (fam == 3 || fam == 7 || fam == 17) && FIELD(td, 0x510, int) > 0 && FIELD(u, 0x1C, int) >= 0 && !is_aa(u) && s->na < 16)
        s->air[s->na++] = u;
}

static float ld_strength(float hp, float att) { return sqrtf(hp * att); }
static float ld_attack(uint8_t *u) { return FIELD(u, 0x3C, int) > 0 ? unit_power(u) / (float)FIELD(u, 0x3C, int) : 0.0f; }
// what defends a beach (seen): soldiers within 15 tiles, defences reaching it, defences reaching the water in front
// (they shoot at the transports)
static void ld_defence(void *pl, int x, int y, int wx, int wy, struct ld_def *d) {
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    float sh = 0, sa = 0, th = 0, ta = 0, wh = 0, wa = 0;
    d->n = 0;
    if (grid) {
        int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int), r = 27;
        uint8_t *cells = FIELD(grid, 4, uint8_t *);
        for (int gx = (x - r) / cs; cells && cs > 0 && gx <= (x + r) / cs; gx++)
            for (int gy = (y - r) / cs; gy <= (y + r) / cs; gy++) {
                if (gx < 0 || gy < 0 || gx >= nx || gy >= ny) continue;
                uint8_t *cell = cells + (gx * ny + gy) * 16;
                for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **); b++) {
                    uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                    if (!td || FIELD(u, 0x3C, int) <= 0) continue;
                    void *o = FIELD(u, 0x18, void *);
                    if (!o || FIELD(o, PL_INDEX, int) <= 0 || !is_enemy(pl, o) || !seen_by(u, pl)) continue;
                    int ux = FIELD(u, 0x1C, int), uy = FIELD(u, 0x20, int);
                    float dx = (float)(ux - x), dy = (float)(uy - y), d2 = dx * dx + dy * dy, hp = (float)FIELD(u, 0x3C, int), range, att;
                    if (is_soldier(td) || is_siege(FIELD(td, 0x4B8, int))) {
                        if (d2 <= 225.0f) { sh += hp; sa += ld_attack(u); d->n++; }
                    } else if (defence_stats(u, &range, &att)) {
                        float rr = (range + 2.0f) * (range + 2.0f), wdx = (float)(ux - wx), wdy = (float)(uy - wy);
                        if (d2 <= rr) { th += hp; ta += att; }
                        if (wdx * wdx + wdy * wdy <= rr) { wh += hp; wa += att; }
                    }
                }
            }
    }
    // honest_vision (intel.c): the soldiers seen there lately, out of sight now; a beach not looked at for 3 minutes is
    // assumed held by a quarter of the army it knows of (the user: "assuming the enemy likely has towers and a defence
    // army before I ever saw his base")
    float rh = 0, ra = 0; int rn = 0;
    intel_soldiers(pl, x, y, 15, &rh, &ra, NULL, &rn);
    sh += rh; sa += ra; d->n += rn;
    d->assumed = 0;
    if (g_honest_vision && intel_looked_ago(pl, x, y) > 180000) {
        float ah = 0, aa = 0; int an = 0;
        intel_army(pl, &ah, &aa, NULL, &an);
        sh += 0.25f * ah; sa += 0.25f * aa; d->assumed = 1;
    }
    d->soldiers = ld_strength(sh, sa); d->towers = ld_strength(th, ta); d->water = ld_strength(wh, wa);
    d->all = ld_strength(sh + th, sa + ta);
}
// the strength a landing needs against a beach: 1.4 times what holds it - the user: "it just has to choose the weakest
// one and bring an army that actually has a chance". The defences over the water hit the transports, not the troops:
// the warships take them on first
static float ld_need(const struct ld_def *d) { return 1.4f * d->all; }

// a landing site: open land at the shore of ocean `ocean` next to the target's buildings - the weakest defended
// (rarely an undefended one), against the strength `power` the landing can bring
static struct { unsigned tried, land, shore, sea, safe, quiet, oc[8], ocn[8]; } g_ld_stat[16];   // why no site (landing P)
static int ld_site(void *pl, unsigned pi, struct ld_scan *s, unsigned ocean, float power, int *sx, int *sy, int *swx, int *swy, struct ld_def *bd) {
    memset(&g_ld_stat[pi], 0, sizeof g_ld_stat[pi]);
    float best = -1e9f;
    int bx = 0, by = 0, bwx = 0, bwy = 0, found = 0;
    if (power < 1.0f) power = 1.0f;
    int step = s->nb > 60 ? s->nb / 60 : 1;
    for (int b = 0; b < s->nb; b += step) {
        unsigned bc = area_id(s->bx[b], s->by[b], 2);
        for (int r = 6; r <= 30; r += 3)
            for (int a = 0; a < 16; a++) {
                float ang = (float)a * 0.3927f;
                int x = s->bx[b] + (int)(cosf(ang) * r), y = s->by[b] + (int)(sinf(ang) * r);
                g_ld_stat[pi].tried++;
                if (!bc || area_id(x, y, 2) != bc || area_id(x, y, 1) || !ld_tile_free(x, y)) continue;   // open land of the building's continent (an island in the bay was taken: the troops stuck there)
                g_ld_stat[pi].land++;
                int wx, wy;
                unsigned on = ocean_near(x, y, 2, &wx, &wy);
                if (on) {
                    g_ld_stat[pi].shore++;
                    int q = 0;
                    while (q < 8 && g_ld_stat[pi].oc[q] && g_ld_stat[pi].oc[q] != on) q++;
                    if (q < 8) { g_ld_stat[pi].oc[q] = on; g_ld_stat[pi].ocn[q]++; }
                }
                if (on != ocean) continue;
                g_ld_stat[pi].sea++;
                struct ld_def d;
                ld_defence(pl, x, y, wx, wy, &d);
                g_ld_stat[pi].safe += d.towers <= 0;
                g_ld_stat[pi].quiet += d.towers <= 0 && d.n <= 2;
                int close = 0;
                for (int k = 0; k < s->nb; k++) {
                    int ddx = s->bx[k] - x, ddy = s->by[k] - y;
                    close += ddx * ddx + ddy * ddy <= 400;
                }
                // the weakest beach first, then the most buildings near it (up to 12)
                float sc = (float)(close < 12 ? close : 12) - 20.0f * (ld_need(&d) + 0.5f * d.water) / power;
                if (close >= 1 && sc > best) { best = sc; bx = x; by = y; bwx = wx; bwy = wy; *bd = d; found = 1; }
            }
    }
    if (!found) return 0;
    *sx = bx; *sy = by; *swx = bwx; *swy = bwy;
    return 1;
}

static struct as_home g_ld_home;
static void ld_home_scan(void *pl) { g_ld_home.pl = pl; g_ld_home.nb = g_ld_home.ne = 0; grid_each(as_home_unit, &g_ld_home); }
static int ld_at_home(int x, int y) {
    if (x < 0) return 0;
    for (int b = 0; b < g_ld_home.nb; b++) {
        int dx = g_ld_home.bx[b] - x, dy = g_ld_home.by[b] - y;
        if (dx * dx + dy * dy <= 400) return 1;
    }
    return 0;
}

static void ld_release(unsigned pi, const char *why, uint32_t now) {
    struct landing *l = &g_ld[pi];
    if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: %s\n", pi, now / 60000.0, why);
    l->phase = 0; l->nfrozen = 0; l->nships = 0; l->npass = 0; l->obj = NULL;
}

static int ld_aboard(uint8_t *u) { return FIELD(u, 0x1C, int) < 0; }
static int ld_alive(uint8_t *u) { return u && FIELD(u, 0x3C, int) > 0 && FIELD(u, 0x2C, uint8_t *); }

// the nearest enemy defence or warship (seen) within r tiles - what shoots at the transports
static uint8_t *ld_naval_target(void *pl, int x, int y, int r, int soldiers) {   // soldiers: enemy soldiers and siege as well
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return NULL;
    int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
    uint8_t *cells = FIELD(grid, 4, uint8_t *), *best = NULL;
    float bd = 1e9f;
    for (int gx = (x - r) / cs; cells && cs > 0 && gx <= (x + r) / cs; gx++)
        for (int gy = (y - r) / cs; gy <= (y + r) / cs; gy++) {
            if (gx < 0 || gy < 0 || gx >= nx || gy >= ny) continue;
            uint8_t *cell = cells + (gx * ny + gy) * 16;
            for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **); b++) {
                uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                if (!td || FIELD(u, 0x3C, int) <= 0) continue;
                void *o = FIELD(u, 0x18, void *);
                if (!o || FIELD(o, PL_INDEX, int) <= 0 || !is_enemy(pl, o) || !seen_by(u, pl)) continue;
                float rr, a;
                int t = FIELD(td, 0x4B8, int);
                if (!defence_stats(u, &rr, &a) && !(FIELD(td, 0x4B4, int) == 4 && !is_transport(t) && !is_fishing_boat(t)) &&
                    !(soldiers && (is_soldier(td) || is_siege(t))))
                    continue;
                float dx = (float)(FIELD(u, 0x1C, int) - x), dy = (float)(FIELD(u, 0x20, int) - y), d2 = dx * dx + dy * dy;
                if (d2 <= (float)(r * r) && d2 < bd) { bd = d2; best = u; }
            }
        }
    return best;
}
// the warships along (from the sail on): they go ahead to the water in front of the site and take on the defences
// and warships there - the towers shoot at them instead of the transports (the user: "a player would send a few
// warships along to draw the towers' attention away from the transports") - then shell the shore
// From the plan on: they shell the beach free (its defences, the warships off it, then the soldiers on it) while
// the transports are built and boarded.
static int ld_is_air(uint8_t *u) {
    int fam = FIELD(FIELD(u, 0x2C, uint8_t *), 0x4B4, int);
    return fam == 3 || fam == 7 || fam == 17;
}
static void ld_escorts(void *pl, struct landing *l) {
    for (int k = 0; k < l->nesc; k++) {
        uint8_t *u = l->esc[k], *g = FIELD(u, 0x1EC, uint8_t *);
        if (g && *(uintptr_t *)g == VT_GOAL_ATTACK && ld_alive(FIELD(g, 0x30, uint8_t *))) continue;
        if (ld_is_air(u)) {   // aircraft: the beach's defences, its soldiers, else over the beach
            if (FIELD(u, 0x1C, int) < 0) continue;   // landed at its airport (refuelling)
            uint8_t *e = ld_naval_target(pl, l->sx, l->sy, 12, 0);
            if (!e) e = ld_naval_target(pl, l->wx, l->wy, 10, 0);
            if (!e) e = ld_naval_target(pl, l->sx, l->sy, 12, 1);
            if (e) l->orders += unit_goal(u, goal_attack(u, e), 0, 0x1F40);
            else if (!g || !as_goal_near(u, (float)l->sx + 0.5f, (float)l->sy + 0.5f, 3.0f))
                l->orders += unit_goal(u, goal_move((float)l->sx + 0.5f, (float)l->sy + 0.5f), 0, 0x1F40);
            continue;
        }
        // the beach's defences and the warships off it first, then what shoots at it where it is (nearest to itself
        // first: two warships shelled a tower 35 tiles from the beach for minutes), then the soldiers on the beach
        uint8_t *e = ld_naval_target(pl, l->wx, l->wy, 12, 0);
        if (!e) e = ld_naval_target(pl, l->sx, l->sy, 12, 0);
        if (!e) e = ld_naval_target(pl, FIELD(u, 0x1C, int), FIELD(u, 0x20, int), 8, 0);
        if (!e) e = ld_naval_target(pl, l->sx, l->sy, 10, 1);
        if (e) l->orders += unit_goal(u, goal_attack(u, e), 0, 0x1F40);
        else if (!as_goal_near(u, (float)l->wx + 0.5f, (float)l->wy + 0.5f, 3.0f))
            l->orders += unit_goal(u, goal_move((float)l->wx + 0.5f, (float)l->wy + 0.5f), 0, 0x1F40);
    }
}

// water of the sea `ocean` with no land within 2 tiles, nearest to (x, y) within 10: a loaded transport waits there -
// idle at the shore with soldiers aboard, the game's own unit code (0x483E10: cargo, no order, land next to it)
// unloads it on the spot: P3 lost half of those aboard while its transports waited for the warships
static int ld_open_water(int x, int y, unsigned ocean, int *ox, int *oy) {
    for (int d = 1; d <= 10; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d ? 1 : 2 * d)) {
                int tx = x + dx, ty = y + dy, ok = area_id(tx, ty, 1) == ocean;
                for (int ax = -2; ax <= 2 && ok; ax++)
                    for (int ay = -2; ay <= 2 && ok; ay++) ok = area_id(tx + ax, ty + ay, 1) == ocean;
                if (ok) { *ox = tx; *oy = ty; return 1; }
            }
    return 0;
}

// called off with soldiers aboard: each transport unloads on the land nearest to it (at home)
static void ld_unload_home(struct landing *l) {
    for (int k = 0; k < l->nships; k++) {
        uint8_t *sh = l->ships[k];
        int x0 = FIELD(sh, 0x1C, int), y0 = FIELD(sh, 0x20, int), ux = -1, uy = -1;
        for (int d = 1; d <= 10 && ux < 0; d++)
            for (int dx = -d; dx <= d && ux < 0; dx++)
                for (int dy = -d; dy <= d && ux < 0; dy += (abs(dx) == d ? 1 : 2 * d))
                    if (area_id(x0 + dx, y0 + dy, 2) && !area_id(x0 + dx, y0 + dy, 1) && ld_tile_free(x0 + dx, y0 + dy)) { ux = x0 + dx; uy = y0 + dy; }
        if (ux < 0) continue;
        void *g = ((void *(__cdecl *)(unsigned))0x00683718)(0x40);
        if (g) g = ((void *(__thiscall *)(void *, void *, int, int, int))0x00615A1F)(g, sh, ux, uy, g_land_flag);
        l->orders += unit_goal(sh, g, 0, 0x1F40);
    }
}

// every subgroup holding a transport, warship or soldier of the landing is led by it - checked every tick: the game
// moves units between subgroups, and a transport with soldiers aboard sailed off alone in its new subgroup
static void ld_refreeze(struct landing *l, uint8_t *dq, unsigned first, unsigned cnt) {
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        uint8_t *g = deque_at(dq, first + k);
        for (uint8_t **sp = g ? FIELD(g, 8, uint8_t **) : NULL; sp && sp < FIELD(g, 0xC, uint8_t **); sp++) {
            uint8_t *sg = *sp;
            int mine = 0, have = 0;
            for (int f = 0; f < l->nfrozen; f++) have |= l->frozen[f] == sg;
            if (!sg || have || l->nfrozen >= 48) continue;
            for (uint8_t **r = FIELD(sg, 0x54, uint8_t **); r && r < FIELD(sg, 0x58, uint8_t **) && !mine; r++) {
                uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL;
                for (int j = 0; j < l->nships && !mine; j++) mine = u == l->ships[j];
                for (int j = 0; j < l->nesc && !mine; j++) mine = u == l->esc[j];
                for (int j = 0; j < l->npass && !mine; j++) mine = u == l->pass[j];
            }
            if (mine) l->frozen[l->nfrozen++] = sg;
        }
    }
}

// a shore tile to unload on: land of the site's continent, room on the 3x3 tiles, the landing's sea within 2 tiles,
// within 15 of the site; nearest to (x, y), at least `gap` tiles from the other transports' points and from
// (ax, ay) - where the last unload failed
static int ld_shore_tile(struct landing *l, int k, int x, int y, int ax, int ay, int gap, int *ox, int *oy) {
    unsigned cont = area_id(l->sx, l->sy, 2);
    for (int d = 0; d <= 10; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d || !d ? 1 : 2 * d)) {
                int tx = x + dx, ty = y + dy, ok = 1;
                if ((tx - l->sx) * (tx - l->sx) + (ty - l->sy) * (ty - l->sy) > 225) continue;
                if (area_id(tx, ty, 2) != cont || area_id(tx, ty, 1) || (int)ocean_near(tx, ty, 2, NULL, NULL) != l->ocean) continue;
                if (ax >= 0 && abs(tx - ax) < 3 && abs(ty - ay) < 3) continue;
                for (int j = 0; j < l->nships && ok; j++)
                    if (j != k && abs(tx - l->ux[j]) < gap && abs(ty - l->uy[j]) < gap) ok = 0;
                if (!ok || !ld_tile_free(tx, ty)) continue;
                *ox = tx; *oy = ty;
                return 1;
            }
    return 0;
}
static void *ld_unload(uint8_t *sh, int x, int y) {
    void *g = ((void *(__cdecl *)(unsigned))0x00683718)(0x40);
    if (g) g = ((void *(__thiscall *)(void *, void *, int, int, int))0x00615A1F)(g, sh, x, y, g_land_flag);
    return g;
}

static void ld_why(unsigned pi, uint32_t now, const char *why) {   // why no landing is planned (once a minute)
    static uint32_t t[16];
    if (!g_log || (t[pi] && now - t[pi] < 60000 && now >= t[pi])) return;
    t[pi] = now;
    fprintf(g_log, "# landing: P%u at %.1f min: none planned - %s\n", pi, now / 60000.0, why);
}

static void landing_tick(void *pl, uint32_t now) {
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    struct landing *l = &g_ld[pi];
    if (!g_landing) return;
    if (now < l->last) memset(l, 0, sizeof *l);   // a new game
    if (now - l->last < (l->phase == 3 ? 500u : 2000u)) return;   // sailing: a transport stalled off the beach is sunk
    l->last = now;
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL, *um = cm ? FIELD(cm, 0x44, uint8_t *) : NULL;
    uint8_t *cbm = FIELD(pl, 0xB8C, uint8_t *), *cmd = cbm ? FIELD(cbm, 0x48, uint8_t *) : NULL;
    if (!cmd || !bm || !um || FIELD(bm, 0x12C, int) == -10000) { if (l->phase) ld_release(pi, "no base", now); return; }
    uint8_t *dq = cmd + 4;
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    {   // subgroups the game deleted meanwhile (never read a stored one without this)
        int nf = 0;
        for (int f = 0; f < l->nfrozen; f++) if (asg_live(pl, l->frozen[f])) l->frozen[nf++] = l->frozen[f];
        l->nfrozen = nf;
    }
    static struct ld_scan s;
    if (l->phase == 0) {
        if (now < l->next_try) return;
        l->next_try = now + 30000;
        // the attack's target: the group with the most land members
        uint8_t *tgt[16] = { 0 }; int tn[16] = { 0 }, nt = 0;
        for (unsigned k = 0; k < cnt && k < 64; k++) {
            uint8_t *g = deque_at(dq, first + k), *t = g ? FIELD(g, 0x48, uint8_t *) : NULL;
            if (!t || !is_enemy(pl, t)) continue;
            int j = 0;
            while (j < nt && tgt[j] != t) j++;
            if (j == nt) { if (nt == 16) continue; tgt[nt++] = t; }
            for (uint8_t **sp = FIELD(g, 8, uint8_t **); sp && sp < FIELD(g, 0xC, uint8_t **); sp++)
                if (*sp && *(uintptr_t *)*sp == VT_ASG_LAND) tn[j] += (int)(FIELD(*sp, 0x58, uint8_t **) - FIELD(*sp, 0x54, uint8_t **));
        }
        uint8_t *target = NULL;
        for (int j = 0, bn = 0; j < nt; j++) if (tn[j] > bn) { bn = tn[j]; target = tgt[j]; }
        if (!target) { ld_why(pi, now, "no attack on"); return; }
        memset(&s, 0, sizeof s); s.pl = pl; s.target = target;
        grid_each(ld_unit, &s);
        if (!s.nd || !s.nb) { ld_why(pi, now, !s.nd ? "no dock" : "no building of the target seen"); return; }
        unsigned ocean = 0;
        for (int d = 0; d < s.nd && !ocean; d++) ocean = ocean_near(s.dx[d], s.dy[d], 4, NULL, NULL);
        int sx, sy, swx, swy;
        if (!ocean) { ld_why(pi, now, "no sea at its docks"); return; }
        // soldiers at home in land subgroups (within 20 tiles of its buildings - the base point can lie far off the town)
        ld_home_scan(pl);
        int home = 0;
        float ahp = 0, aatt = 0;
        for (unsigned k = 0; k < cnt && k < 64; k++) {
            uint8_t *g = deque_at(dq, first + k);
            for (uint8_t **sp = g ? FIELD(g, 8, uint8_t **) : NULL; sp && sp < FIELD(g, 0xC, uint8_t **); sp++) {
                if (!*sp || *(uintptr_t *)*sp != VT_ASG_LAND || (g_assault && assault_leads(*sp))) continue;
                for (uint8_t **r = FIELD(*sp, 0x54, uint8_t **); r && r < FIELD(*sp, 0x58, uint8_t **); r++) {
                    uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL;
                    if (!ld_alive(u) || !is_soldier(FIELD(u, 0x2C, uint8_t *))) continue;
                    if (ld_at_home(FIELD(u, 0x1C, int), FIELD(u, 0x20, int))) { home++; ahp += (float)FIELD(u, 0x3C, int); aatt += ld_attack(u); }
                }
            }
        }
        if (home < 15) { ld_why(pi, now, "fewer than 15 soldiers at home"); return; }
        int cap = g_land_ships * 10;   // what the transports carry
        if (home > cap) { ahp = ahp * (float)cap / (float)home; aatt = aatt * (float)cap / (float)home; }
        float power = ld_strength(ahp, aatt);
        struct ld_def d;
        if (!ld_site(pl, pi, &s, ocean, power, &sx, &sy, &swx, &swy, &d)) {
            static char why[96];
            snprintf(why, sizeof why, "no landing site (its docks on sea %u, %u at a shore, %u at that sea)", ocean, g_ld_stat[pi].shore, g_ld_stat[pi].sea);
            ld_why(pi, now, why);
            return;
        }
        if (power < ld_need(&d)) {
            static char why[160];
            snprintf(why, sizeof why, "the weakest beach %d,%d needs %.0f, its army %.0f (soldiers there %.0f, defences %.0f, over the water %.0f)",
                     sx, sy, ld_need(&d), power, d.soldiers, d.towers, d.water);
            ld_why(pi, now, why);
            return;
        }
        l->target = target; l->sx = sx; l->sy = sy; l->ocean = (int)ocean; l->wx = swx; l->wy = swy; l->def = d;
        l->phase = 1; l->since = now; l->tries++; l->trained_t = 0; l->boarded_t = 0; l->switched = 0; l->tr_missing = 0; l->tr_got = 0;
        l->nesc = 0; l->nfrozen = 0;
        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: plans a landing on P%d at %d,%d (sea %u), %d soldiers at home (strength %.0f), "
                           "the beach needs %.0f (soldiers %.0f%s, defences %.0f, over the water %.0f), %d transports, %d warships\n",
                           pi, now / 60000.0, FIELD(target, PL_INDEX, int), sx, sy, ocean, home, power, ld_need(&d), d.soldiers, d.assumed ? " - not looked at, assumed" : "", d.towers,
                           d.water, s.nt, s.nw);
        return;
    }
    if (!l->target || !is_enemy(pl, l->target)) { ld_release(pi, "no enemy any more", now); return; }
    for (int k = 0; k < l->nesc; k++) if (!ld_alive(l->esc[k])) { l->esc[k--] = l->esc[--l->nesc]; }
    if (l->phase >= 2) { ld_refreeze(l, dq, first, cnt); ld_escorts(pl, l); }
    if (l->phase == 1) {   // ships
        memset(&s, 0, sizeof s); s.pl = pl; s.target = l->target;
        grid_each(ld_unit, &s);
        int want = g_land_ships;
        if (s.nt < want) {
            if (now - l->since >= 360000) { l->aborted++; l->next_try = now + 600000; ld_release(pi, "no transports - given up", now); return; }
            if (g_log && (now - l->since) % 60000 < 2000) {
                int *st = (int *)((uint8_t *)pl + PL_STOCK);
                fprintf(g_log, "# landing: P%u at %.1f min: waits for transports - has %d of %d, %d docks, stock %d,%d,%d,%d,%d\n", pi, now / 60000.0,
                        s.nt, want, s.nd, st[2], st[3], st[4], st[5], st[6]);
            }
            l->tr_missing = want - s.nt;
            // what was ordered takes time (ordered every 20 s: 7); nothing taken (not affordable): again in 20 s
            if (!l->trained_t || now - l->trained_t >= (l->tr_got ? 120000u : 20000u)) {
                l->trained_t = now;
                int epoch = current_epoch(pl) + 1;
                for (size_t k = 0; k < sizeof k_transports / sizeof *k_transports; k++) {
                    if (k_transports[k].epoch > epoch) continue;
                    int req[4] = { k_transports[k].type, want - s.nt, 0, 0 };
                    unsigned got = 0;
                    l->tr_type = k_transports[k].type;
                    g_priority_mil = 1; g_landing_order = 1;
                    hook_train_units(um, NULL, req, (uintptr_t)&got);
                    g_priority_mil = 0; g_landing_order = 0;
                    l->tr_got = (int)got;
                    if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: trains %u transports (type %d), has %d\n", pi, now / 60000.0, got, k_transports[k].type, s.nt);
                    break;
                }
            }
            return;
        }
        // the ships, their subgroups, the passengers' subgroups
        ld_home_scan(pl);
        l->nships = 0; l->nfrozen = 0; l->npass = 0;
        for (int k = 0; k < s.nt && l->nships < want; k++) l->ships[l->nships++] = s.tr[k];
        // the warships go ahead from now on: they shell the beach while the troops board (the user: "bombing the landing
        // area free with planes and warships first") - not before the transports are there: sent while those were
        // being built, P3's warships waited off the beach and 3 of 4 were sunk, the transports never came
        for (int k = 0; k < s.nw && l->nesc < 4; k++) l->esc[l->nesc++] = s.war[k];
        // its aircraft too: they bomb the beach's defences and soldiers, else fly over it - a look at the beach
        // (honest_vision) before the transports commit
        for (int k = 0; k < s.na && l->nesc < 12; k++) l->esc[l->nesc++] = s.air[k];
        for (unsigned k = 0; k < cnt && k < 64; k++) {
            uint8_t *g = deque_at(dq, first + k);
            for (uint8_t **sp = g ? FIELD(g, 8, uint8_t **) : NULL; sp && sp < FIELD(g, 0xC, uint8_t **); sp++) {
                uint8_t *sg = *sp;
                if (!sg || l->nfrozen >= 48) continue;
                int mine = 0, home = 0, n = 0;
                for (uint8_t **r = FIELD(sg, 0x54, uint8_t **); r && r < FIELD(sg, 0x58, uint8_t **); r++) {
                    uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL;
                    if (!ld_alive(u)) continue;
                    for (int j = 0; j < l->nships; j++) mine |= u == l->ships[j];
                    for (int j = 0; j < l->nesc; j++) mine |= u == l->esc[j];
                    n++; home += ld_at_home(FIELD(u, 0x1C, int), FIELD(u, 0x20, int));
                }
                if (mine) { l->frozen[l->nfrozen++] = sg; continue; }
                if (*(uintptr_t *)sg != VT_ASG_LAND || (g_assault && assault_leads(sg)) || !n || home * 2 < n ||
                    l->npass >= l->nships * 10)
                    continue;
                l->frozen[l->nfrozen++] = sg;
                for (uint8_t **r = FIELD(sg, 0x54, uint8_t **); r && r < FIELD(sg, 0x58, uint8_t **); r++) {
                    uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL;
                    if (ld_alive(u) && FIELD(u, 0x1C, int) >= 0 && is_soldier(FIELD(u, 0x2C, uint8_t *)) && l->npass < l->nships * 10 && l->npass < LD_PASS)
                        l->pass[l->npass++] = u;
                }
            }
        }
        if (l->npass < 10) { l->aborted++; l->next_try = now + 300000; ld_release(pi, "too few passengers", now); return; }
        l->phase = 2; l->since = now; l->reissue = 0; l->gathered = 0;
        {   // the meeting point: the shore of the landing's sea nearest the passengers (on their land) - boarding
            // straight away, soldiers 30-40 tiles off walked to transports strewn along the coast: after 3 minutes
            // P2 had fewer than half aboard
            float px = 0, py = 0; int pn = 0;
            for (int k = 0; k < l->npass; k++) { px += FIELD(l->pass[k], 0x1C, int); py += FIELD(l->pass[k], 0x20, int); pn++; }
            int x0 = (int)(px / pn), y0 = (int)(py / pn), found = 0;
            unsigned cont = area_id(FIELD(l->pass[0], 0x1C, int), FIELD(l->pass[0], 0x20, int), 2);
            for (int d = 0; d <= 40 && !found; d++)
                for (int dx = -d; dx <= d && !found; dx++)
                    for (int dy = -d; dy <= d && !found; dy += (abs(dx) == d || !d ? 1 : 2 * d)) {
                        int tx = x0 + dx, ty = y0 + dy, wx, wy;
                        if (area_id(tx, ty, 2) != cont || area_id(tx, ty, 1) || !ld_tile_free(tx, ty)) continue;
                        if ((int)ocean_near(tx, ty, 2, &wx, &wy) != l->ocean) continue;
                        l->ex = tx; l->ey = ty; l->ewx = wx; l->ewy = wy; found = 1;
                    }
            if (!found) l->gathered = 1;   // none: board where they are
            else if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: they meet at %d,%d (passengers around %d,%d)\n", pi, now / 60000.0, l->ex, l->ey, x0, y0);
        }
        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: boarding %d soldiers on %d transports, %d warships and aircraft along\n", pi, now / 60000.0, l->npass, l->nships, l->nesc);
    }
    // drop the dead
    for (int k = 0; k < l->nships; k++) if (!ld_alive(l->ships[k])) { l->ships[k--] = l->ships[--l->nships]; }
    for (int k = 0; k < l->npass; k++) if (!ld_alive(l->pass[k])) { l->pass[k--] = l->pass[--l->npass]; }
    if (!l->nships && l->phase < 4) { l->aborted++; l->next_try = now + 600000; ld_release(pi, "transports lost", now); return; }
    if (l->phase == 2 && !l->gathered) {   // to the meeting point first: the transports to the water in front, the soldiers to the shore
        int there = 0;
        for (int k = 0; k < l->nships; k++) {
            uint8_t *sh = l->ships[k];
            int dx = FIELD(sh, 0x1C, int) - l->ewx, dy = FIELD(sh, 0x20, int) - l->ewy;
            if (dx * dx + dy * dy <= 49) { there++; continue; }
            if (!as_goal_near(sh, (float)l->ewx + 0.5f, (float)l->ewy + 0.5f, 2.0f))
                l->orders += unit_goal(sh, goal_move((float)l->ewx + 0.5f, (float)l->ewy + 0.5f), 0, 0x1F40);
        }
        int close = 0;
        for (int k = 0; k < l->npass; k++) {
            uint8_t *u = l->pass[k];
            if (ld_aboard(u)) { close++; continue; }
            int dx = FIELD(u, 0x1C, int) - l->ex, dy = FIELD(u, 0x20, int) - l->ey;
            if (dx * dx + dy * dy <= 64) { close++; continue; }
            if (!as_goal_near(u, (float)l->ex + 0.5f, (float)l->ey + 0.5f, 3.0f))
                l->orders += unit_goal(u, goal_move((float)l->ex + 0.5f, (float)l->ey + 0.5f), 0, 0x1F40);
        }
        // the transports there and most soldiers near (or 2 minutes): board
        if ((there == l->nships && close * 4 >= l->npass * 3) || now - l->since >= 120000) {
            if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: at the meeting point - %d of %d transports, %d of %d soldiers\n", pi, now / 60000.0,
                               there, l->nships, close, l->npass);
            l->gathered = 1; l->since = now; l->reissue = 0;
        }
        return;
    }
    if (l->phase == 2)   // loaded and idle at the shore: the game unloads it (0x483E10) - out onto open water
        for (int k = 0; k < l->nships; k++) {
            uint8_t *sh = l->ships[k], *g = FIELD(sh, 0x1EC, uint8_t *);
            if ((g && FIELD(g, 5, uint8_t) != 1) || ((int (__thiscall *)(void *))0x0042E347)(sh) <= 0) continue;
            int wx, wy;
            if (ld_open_water(FIELD(sh, 0x1C, int), FIELD(sh, 0x20, int), (unsigned)l->ocean, &wx, &wy))
                l->orders += unit_goal(sh, goal_move((float)wx + 0.5f, (float)wy + 0.5f), 0, 0x1F40);
        }
    if (l->phase == 2) {   // board
        int aboard = 0;
        for (int k = 0; k < l->npass; k++) aboard += ld_aboard(l->pass[k]);
        if (aboard == l->npass || now - l->since >= 180000) {
            if (aboard * 2 < l->npass || aboard < 6) { l->aborted++; l->next_try = now + 600000; ld_release(pi, "boarding failed", now); return; }
            // the last word before the commit: the beach now (the warships have shelled it meanwhile) against what is
            // aboard; too strong: the warships get up to 3 more minutes, then it is called off (unloaded at home).
            // Strong enough: it sails at once - waiting at home for the warships to take on the defences cost P3 half
            // of those aboard (unloaded again, 20 -> 11) and 2 warships did not bring down the tower in 4 minutes
            if (!l->boarded_t) l->boarded_t = now;
            struct ld_def d;
            ld_defence(pl, l->sx, l->sy, l->wx, l->wy, &d);
            float phh = 0, pa = 0;
            for (int k = 0; k < l->npass; k++) if (ld_aboard(l->pass[k])) { phh += (float)FIELD(l->pass[k], 0x3C, int); pa += ld_attack(l->pass[k]); }
            float pw = ld_strength(phh, pa);
            int esc_busy = l->nesc && now - l->boarded_t < 180000;
            if (pw < ld_need(&d) && !esc_busy) {
                if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: the beach needs %.0f (soldiers %.0f, defences %.0f, over the water %.0f), aboard %.0f\n",
                                   pi, now / 60000.0, ld_need(&d), d.soldiers, d.towers, d.water, pw);
                ld_unload_home(l);
                l->aborted++; l->next_try = now + 600000; ld_release(pi, "too strong a beach - called off, unloaded at home", now);
                return;
            }
            // the warships found the beach held more strongly than thought (honest_vision: P3 planned on a beach it had
            // not looked at, its warships found 1967 of soldiers there): another beach, if one is weak enough now
            if (pw < ld_need(&d) && !d.assumed && !l->switched) {
                memset(&s, 0, sizeof s); s.pl = pl; s.target = l->target;
                grid_each(ld_unit, &s);
                int nx, ny, nwx, nwy;
                struct ld_def nd;
                if (ld_site(pl, pi, &s, (unsigned)l->ocean, pw, &nx, &ny, &nwx, &nwy, &nd) && ld_need(&nd) <= pw && (nx != l->sx || ny != l->sy)) {
                    if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: the beach %d,%d needs %.0f, aboard %.0f - switches to %d,%d (needs %.0f%s)\n",
                                       pi, now / 60000.0, l->sx, l->sy, ld_need(&d), pw, nx, ny, ld_need(&nd), nd.assumed ? ", not looked at" : "");
                    l->sx = nx; l->sy = ny; l->wx = nwx; l->wy = nwy; l->def = nd; l->switched = 1; l->boarded_t = now;
                    return;
                }
            }
            if (g_land_trace && g_log && (now - l->boarded_t) % 10000 < 2000)
                for (int k = 0; k < l->nships; k++)
                    fprintf(g_log, "# ldtrace: P%u %.2f waiting ship %d at %d,%d aboard %d goal %s\n", pi, now / 60000.0, k, FIELD(l->ships[k], 0x1C, int),
                            FIELD(l->ships[k], 0x20, int), ((int (__thiscall *)(void *))0x0042E347)(l->ships[k]), rtti_name(FIELD(l->ships[k], 0x1EC, void *)));
            // honest_vision: a beach nobody has looked at lately - the warships on their way look first
            if (pw < ld_need(&d) || (d.assumed && esc_busy)) {
                if (g_log && (now - l->boarded_t) % 30000 < 2000)
                    fprintf(g_log, "# landing: P%u at %.1f min: waits for the warships (%d) - the beach needs %.0f (soldiers %.0f%s, defences %.0f, over the water %.0f), aboard %.0f\n",
                            pi, now / 60000.0, l->nesc, ld_need(&d), d.soldiers, d.assumed ? " - not looked at, assumed" : "", d.towers, d.water, pw);
                return;
            }
            // those left ashore go back to the game: subgroups without a passenger, ship or warship of the landing
            int n = 0;
            for (int k = 0; k < l->npass; k++) if (ld_aboard(l->pass[k])) l->pass[n++] = l->pass[k];
            l->npass = n; l->launch_n = n;
            int nf = 0;
            for (int f = 0; f < l->nfrozen; f++) {
                uint8_t *sg = l->frozen[f];
                int keep = 0;
                for (uint8_t **r = FIELD(sg, 0x54, uint8_t **); r && r < FIELD(sg, 0x58, uint8_t **) && !keep; r++) {
                    uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL;
                    for (int j = 0; j < l->npass && !keep; j++) keep = u == l->pass[j];
                    for (int j = 0; j < l->nships && !keep; j++) keep = u == l->ships[j];
                    for (int j = 0; j < l->nesc && !keep; j++) keep = u == l->esc[j];
                }
                if (keep) l->frozen[nf++] = sg;
            }
            l->nfrozen = nf;
            // each transport its own point on the beach, 4+ tiles apart: with one point for all, the first put its
            // soldiers there and the unload of the next ended at once (no room), soldiers aboard, the transport idle
            // off the beach
            for (int k = 0; k < l->nships; k++) { l->ux[k] = -100; l->uy[k] = -100; }
            for (int k = 0; k < l->nships; k++)
                if (!ld_shore_tile(l, k, l->sx, l->sy, -1, -1, 4, &l->ux[k], &l->uy[k])) { l->ux[k] = l->sx; l->uy[k] = l->sy; }
            for (int k = 0; k < l->nships; k++) {
                void *g = ld_unload(l->ships[k], l->ux[k], l->uy[k]);
                if (g_log && g) fprintf(g_log, "# landing: P%u: transport at %d,%d sails, the goal's unload point %d,%d\n", pi,
                                        FIELD(l->ships[k], 0x1C, int), FIELD(l->ships[k], 0x20, int), FIELD(g, 0x34, int), FIELD(g, 0x38, int));
                l->orders += unit_goal(l->ships[k], g, 0, 0x1F40);
            }
            l->phase = 3; l->since = now; l->reissue = 0; l->all_ashore = 0; memset(l->tries_at, 0, sizeof l->tries_at);
            if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: sails with %d soldiers on %d transports to %d,%d\n", pi, now / 60000.0, n, l->nships, l->sx, l->sy);
            ai_say(pl, CHAT_ATTACK, "[ai] landing with %d units", n);
            return;
        }
        if (l->reissue && now - l->reissue < 30000) return;
        l->reissue = now;
        // per ship its share of those not aboard
        int per = (l->npass + l->nships - 1) / l->nships;
        for (int k = 0; k < l->nships; k++) {
            uint8_t *list[16]; int m = 0;
            for (int j = k * per; j < (k + 1) * per && j < l->npass; j++) if (!ld_aboard(l->pass[j])) list[m++] = l->pass[j];
            if (!m) continue;
            uint8_t *vec[4] = { 0, (uint8_t *)list, (uint8_t *)(list + m), (uint8_t *)(list + m) };
            void *g = ((void *(__cdecl *)(unsigned))0x00683718)(0x68);
            if (!g) continue;
            g = ((void *(__thiscall *)(void *, void *, void *, int))0x0061487F)(g, l->ships[k], vec, 1);
            l->orders += unit_goal(l->ships[k], g, 0, 0x1F40);
            for (int j = k * per; j < (k + 1) * per && j < l->npass; j++) {
                if (ld_aboard(l->pass[j])) continue;
                void *pg = ((void *(__thiscall *)(void *))(*(void ***)g)[4])(g);
                l->orders += unit_goal(l->pass[j], pg, 0, 0x1F40);
            }
        }
        return;
    }
    if (g_land_trace && g_log && l->phase >= 3)
        for (int k = 0; k < l->nships; k++) {
            uint8_t *sh = l->ships[k], *g = FIELD(sh, 0x1EC, uint8_t *);
            int n = ((int (__thiscall *)(void *))0x0042E347)(sh);
            if (!n) continue;
            fprintf(g_log, "# ldtrace: P%u %.2f ship %d at %d,%d hp %d aboard %d goal %s", pi, now / 60000.0, k, FIELD(sh, 0x1C, int), FIELD(sh, 0x20, int),
                    FIELD(sh, 0x3C, int), n, rtti_name(g));
            if (g && *(uintptr_t *)g == 0x00831F5C)
                fprintf(g_log, " @%p water %d,%d / %d,%d point %d,%d reach %.1f state %d", (void *)g, FIELD(g, 0x24, int), FIELD(g, 0x28, int), FIELD(g, 0x2C, int),
                        FIELD(g, 0x30, int), FIELD(g, 0x34, int), FIELD(g, 0x38, int), sqrtf(FIELD(g, 0x3C, float)), FIELD(g, 5, uint8_t));
            fputs("\n", g_log);
        }
    // sail and unload (3), ashore (4) - while some are still aboard, the ones ashore already fight
    int aboard = 0, ashore = 0;
    float cx = 0, cy = 0;
    for (int k = 0; k < l->npass; k++) {
        if (ld_aboard(l->pass[k])) { aboard++; continue; }
        ashore++; cx += FIELD(l->pass[k], 0x1C, int); cy += FIELD(l->pass[k], 0x20, int);
    }
    if (aboard) {
        // a transport with soldiers aboard and no unload order (it ended without unloading, or something else gave the
        // transport an order): the unload again at once - every 0.5 s while sailing. Committed: no turning back while
        // transports with soldiers aboard are afloat (a player lands before they are sunk)
        for (int k = 0; k < l->nships; k++) {
            uint8_t *sh = l->ships[k], *cur = FIELD(sh, 0x1EC, uint8_t *);
            if (cur && *(uintptr_t *)cur == 0x00831F5C) continue;
            if (((int (__thiscall *)(void *))0x0042E347)(sh) <= 0) continue;   // nothing aboard
            // off the beach (within 10 tiles of its point): the unload ended there - the nearest shore tile with room
            // to where it is, away from where it failed; farther off (another order took it): its point again
            int x0 = FIELD(sh, 0x1C, int), y0 = FIELD(sh, 0x20, int), ux = l->ux[k], uy = l->uy[k], ddx = x0 - ux, ddy = y0 - uy;
            l->tries_at[k]++;
            if (ddx * ddx + ddy * ddy <= 100 && ld_shore_tile(l, k, x0, y0, ux, uy, 2, &ux, &uy)) { l->ux[k] = ux; l->uy[k] = uy; }
            void *g = ld_unload(sh, ux, uy);
            l->orders += unit_goal(sh, g, 0, 0x1F40);
            if (g_log && l->tries_at[k] <= 8)   // the goal's own unload point (+0x34/+0x38, from the transport class vf57)
                fprintf(g_log, "# landing: P%u at %.1f min: transport at %d,%d (hp %d) still loaded - unloads at %d,%d, the goal's point %d,%d\n",
                        pi, now / 60000.0, x0, y0, FIELD(sh, 0x3C, int), ux, uy, g ? FIELD(g, 0x34, int) : -1, g ? FIELD(g, 0x38, int) : -1);
        }
    }
    if (l->phase == 3) {
        if (ashore) {
            l->phase = 4; l->obj = NULL; l->landings++;
            if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: ashore near %d,%d - %d landed, %d still aboard\n", pi, now / 60000.0,
                               l->sx, l->sy, ashore, aboard);
        } else if (now - l->since >= 600000) { l->aborted++; ld_release(pi, "no landing within 10 minutes", now); return; }
        else return;
    }
    if (!aboard && l->nships && g_log && l->phase == 4 && !l->all_ashore) {
        l->all_ashore = 1;
        fprintf(g_log, "# landing: P%u at %.1f min: all %d ashore\n", pi, now / 60000.0, ashore);
    }
    // ashore: one group
    int alive = ashore + aboard;
    if (alive * 4 < l->launch_n || !alive) { ld_release(pi, "bridgehead spent", now); return; }
    if (!ashore) return;
    cx /= ashore; cy /= ashore;
    uint8_t *foe = as_enemy_near(pl, l->target, (int)cx, (int)cy, 12, 1, NULL);
    if (!l->obj || !ld_alive(l->obj) || FIELD(l->obj, 0x18, uint8_t *) != l->target) {
        l->obj = as_objective(pl, pi, l->target, area_id((int)cx, (int)cy, 2), (int)cx, (int)cy);
        if (l->obj) { l->ox = FIELD(l->obj, 0x1C, int); l->oy = FIELD(l->obj, 0x20, int); }
    }
    for (int k = 0; k < l->npass; k++) {
        uint8_t *u = l->pass[k];
        if (ld_aboard(u) || as_fighting(u)) continue;
        if (foe) {
            uint8_t *e = as_enemy_near(pl, l->target, FIELD(u, 0x1C, int), FIELD(u, 0x20, int), 14, 1, NULL);
            l->orders += unit_goal(u, goal_attack(u, e ? e : foe), 0, 0x1F40);
        } else if (l->obj) {
            float dx = (float)(l->ox - FIELD(u, 0x1C, int)), dy = (float)(l->oy - FIELD(u, 0x20, int));
            if (dx * dx + dy * dy <= 36.0f) l->orders += unit_goal(u, goal_attack(u, l->obj), 0, 0x1F40);
            else if (!as_goal_near(u, (float)l->ox + 0.5f, (float)l->oy + 0.5f, 3.0f))
                l->orders += unit_goal(u, goal_move((float)l->ox + 0.5f, (float)l->oy + 0.5f), 0, 0x1F40);
        }
    }
    if (!foe && !l->obj) ld_release(pi, "nothing left to attack ashore", now);
}

// terrain x y [r] (test): the map around a tile, a character per tile: ~ ocean, . land (its continent; digit: another
// continent), ? neither id, then on top: T tree or other gaia object, B building, s soldier, c citizen (player's
// digit for buildings of others when P given: B for player P only... kept simple: B any building, u any unit)
static void cmd_terrain(char *out, const char *args) {
    int x0 = 0, y0 = 0, r = 20;
    sscanf(args, "%d %d %d", &x0, &y0, &r);
    if (r > 40) r = 40;
    if (r < 2) r = 2;
    static char g[81 * 82];
    int w = 2 * r + 1;
    unsigned c0 = area_id(x0, y0, 2);
    for (int y = 0; y < w; y++)
        for (int x = 0; x < w; x++) {
            int tx = x0 - r + x, ty = y0 - r + y;
            unsigned o = area_id(tx, ty, 1), c = area_id(tx, ty, 2);
            g[y * w + x] = o ? '~' : c == c0 && c ? '.' : c ? (char)('0' + c % 10) : '?';
        }
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (grid) {
        int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
        uint8_t *cells = FIELD(grid, 4, uint8_t *);
        for (int gx = (x0 - r) / cs; cells && cs > 0 && gx <= (x0 + r) / cs; gx++)
            for (int gy = (y0 - r) / cs; gy <= (y0 + r) / cs; gy++) {
                if (gx < 0 || gy < 0 || gx >= nx || gy >= ny) continue;
                uint8_t *cell = cells + (gx * ny + gy) * 16;
                for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **); b++) {
                    uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                    if (!td) continue;
                    int x = FIELD(u, 0x1C, int) - x0 + r, y = FIELD(u, 0x20, int) - y0 + r;
                    if (x < 0 || y < 0 || x >= w || y >= w) continue;
                    void *o = FIELD(u, 0x18, void *);
                    int fam = FIELD(td, 0x4B4, int), pi = o ? FIELD(o, PL_INDEX, int) : 0;
                    char ch = pi <= 0 ? 'T' : (fam == 2 || fam == 29) ? (char)('A' + pi) : (char)('a' + pi);
                    g[y * w + x] = ch;
                }
            }
    }
    REPLY("terrain around %d,%d (r %d; row = y from %d, column = x from %d): ~ ocean, . continent %u, digit other continent, ? none, T gaia object, B/C/D.. building of P1/P2/P3.., b/c/d.. unit\n",
          x0, y0, r, y0 - r, x0 - r, c0);
    for (int y = 0; y < w; y++) {
        REPLY("  %3d ", y0 - r + y);
        for (int x = 0; x < w; x++) REPLY("%c", g[y * w + x]);
        REPLY("\n");
    }
}

// landing P [now] (test): the state of AI P's landing; now: plan at once
static void cmd_landing(char *out, const char *args) {
    int p = atoi(args);
    if (p <= 0 || p >= 16) { REPLY("error: landing P [now]\n"); return; }
    struct landing *l = &g_ld[p];
    if (strstr(args, "now")) l->next_try = 0;
    int aboard = 0;
    for (int k = 0; k < l->npass; k++) aboard += ld_alive(l->pass[k]) && ld_aboard(l->pass[k]);
    REPLY("P%d landing %s: phase %d, target P%d, site %d,%d (sea %d, water %d,%d), %d ships, %d warships, %d passengers (%d aboard), %d subgroups led; "
          "tries %u, landings %u, aborted %u, orders %u\n", p, g_landing ? "on" : "off", l->phase,
          l->target ? FIELD(l->target, PL_INDEX, int) : -1, l->sx, l->sy, l->ocean, l->wx, l->wy, l->nships, l->nesc, l->npass, aboard, l->nfrozen,
          l->tries, l->landings, l->aborted, l->orders);
    REPLY("  last site search: %u points, %u land, %u at a shore, %u at its sea, %u out of tower reach, %u without enemy soldiers; seas at the shores:",
          g_ld_stat[p].tried, g_ld_stat[p].land, g_ld_stat[p].shore, g_ld_stat[p].sea, g_ld_stat[p].safe, g_ld_stat[p].quiet);
    for (int q = 0; q < 8 && g_ld_stat[p].oc[q]; q++) REPLY(" %u x%u", g_ld_stat[p].oc[q], g_ld_stat[p].ocn[q]);
    REPLY("\n");
    if (l->phase) {
        struct ld_def d;
        ld_defence(g_players[p], l->sx, l->sy, l->wx, l->wy, &d);
        REPLY("  the beach when planned: needs %.0f (soldiers %.0f, defences %.0f, over the water %.0f); now: needs %.0f (%.0f, %.0f, %.0f)\n",
              ld_need(&l->def), l->def.soldiers, l->def.towers, l->def.water, ld_need(&d), d.soldiers, d.towers, d.water);
    }
    for (int k = 0; k < l->nesc; k++)
        if (ld_alive(l->esc[k])) {
            uint8_t *g = FIELD(l->esc[k], 0x1EC, uint8_t *), *t = g && *(uintptr_t *)g == VT_GOAL_ATTACK ? FIELD(g, 0x30, uint8_t *) : NULL;
            REPLY("  %s type %d at %d,%d hp %d goal %s", ld_is_air(l->esc[k]) ? "aircraft" : "warship", FIELD(FIELD(l->esc[k], 0x2C, uint8_t *), 0x4B8, int), FIELD(l->esc[k], 0x1C, int),
                  FIELD(l->esc[k], 0x20, int), FIELD(l->esc[k], 0x3C, int), rtti_name(g));
            if (t) REPLY(" on type %d at %d,%d", FIELD(FIELD(t, 0x2C, uint8_t *), 0x4B8, int), FIELD(t, 0x1C, int), FIELD(t, 0x20, int));
            REPLY("\n");
        }
    if (l->obj && ld_alive(l->obj))
        REPLY("  objective type %d at %d,%d\n", FIELD(FIELD(l->obj, 0x2C, uint8_t *), 0x4B8, int), l->ox, l->oy);
    for (int k = 0; k < l->npass && k < 24; k++) {
        uint8_t *u = l->pass[k], *g;
        if (!ld_alive(u)) continue;
        g = FIELD(u, 0x1EC, uint8_t *);
        REPLY("  passenger type %d at %d,%d hp %d goal %s", FIELD(FIELD(u, 0x2C, uint8_t *), 0x4B8, int), FIELD(u, 0x1C, int),
              FIELD(u, 0x20, int), FIELD(u, 0x3C, int), rtti_name(g));
        if (g && *(uintptr_t *)g == VT_GOAL_LOCATION) REPLY(" to %.0f,%.0f", FIELD(g, 0x20, float), FIELD(g, 0x24, float));
        if (g && *(uintptr_t *)g == VT_GOAL_ATTACK && FIELD(g, 0x30, uint8_t *))
            REPLY(" on type %d at %d,%d", FIELD(FIELD(FIELD(g, 0x30, uint8_t *), 0x2C, uint8_t *), 0x4B8, int),
                  FIELD(FIELD(g, 0x30, uint8_t *), 0x1C, int), FIELD(FIELD(g, 0x30, uint8_t *), 0x20, int));
        REPLY("\n");
    }
    for (int k = 0; k < l->nships; k++)
        if (ld_alive(l->ships[k]))
            REPLY("  ship type %d at %d,%d goal %s\n", FIELD(FIELD(l->ships[k], 0x2C, uint8_t *), 0x4B8, int), FIELD(l->ships[k], 0x1C, int),
                  FIELD(l->ships[k], 0x20, int), rtti_name(FIELD(l->ships[k], 0x1EC, void *)));
}

// who else orders a landing transport to unload (the soldiers aboard while it waited at home were unloaded again):
// the game's calls of the unload goal's constructor go through here; for a transport of a landing, the caller is logged
static const uintptr_t k_unload_sites[] = { 0x445AAD, 0x483E4D, 0x48CC5D, 0x48D022, 0x4BF82A, 0x5B3E23, 0x5DC81B };
static unsigned g_foreign_unload[8];
static void *__fastcall hook_unload_ctor(void *g, void *edx, uint8_t *tr, int x, int y, int flag) {
    (void)edx;
    uintptr_t ret = (uintptr_t)__builtin_return_address(0) - 5;
    for (int p = 0; p < 16 && tr; p++)
        for (int k = 0; k < g_ld[p].nships && g_ld[p].phase; k++)
            if (g_ld[p].ships[k] == tr) {
                int site = 0;
                while (site < 7 && k_unload_sites[site] != ret) site++;
                g_foreign_unload[site]++;
                if (g_log) fprintf(g_log, "# landing: P%d's transport %d (phase %d) told to unload at %d,%d by the game (call at 0x%X)\n", p, k, g_ld[p].phase, x, y, (unsigned)ret);
            }
    return ((void *(__thiscall *)(void *, void *, int, int, int))0x00615A1F)(g, tr, x, y, flag);
}

// the game's idle-transport code (0x483E10: cargo aboard, no order, at a shore - 0x42C49A at 0x483E24 says where to
// land) unloads a transport of a landing waiting for its last passengers or for the warships: for those "nowhere"
static int __fastcall hook_idle_unload_shore(void *mgr, void *edx, uint8_t *tr) {
    (void)edx;
    for (int p = 0; p < 16 && tr; p++)
        for (int k = 0; k < g_ld[p].nships && g_ld[p].phase; k++)
            if (g_ld[p].ships[k] == tr) return 0;
    return ((int (__thiscall *)(void *, void *))0x0042C49A)(mgr, tr);
}

static void landing_install(const char *aimod_ini, const char *autotest_ini) {
    g_landing = cfg_int(aimod_ini, autotest_ini, 1, "landing", 0);
    g_land_ships = cfg_int(aimod_ini, autotest_ini, 1, "landing_transports", 3);
    g_land_flag = cfg_int(aimod_ini, autotest_ini, 1, "landing_flag", 0);
    g_land_knows = cfg_int(aimod_ini, autotest_ini, 1, "landing_knows_map", 0);
    g_land_trace = cfg_int(aimod_ini, autotest_ini, 1, "landing_trace", 0);
    if (g_land_ships < 1) g_land_ships = 1;
    if (g_land_ships > 4) g_land_ships = 4;
    if (g_landing) patch_call(0x00483E24, 0x0042C49A, (void *)hook_idle_unload_shore);
    if (g_landing)
        for (size_t k = 0; k < sizeof k_unload_sites / sizeof *k_unload_sites; k++) patch_call(k_unload_sites[k], 0x00615A1F, (void *)hook_unload_ctor);
    int h = patch_vtable(VT_ASG_SEA_TRANSPORT, 90, FN_ASG_SEA_TRANSPORT_UPDATE, hook_asg_sea_transport_update) &
            patch_vtable(VT_ASG_SEA, 90, FN_ASG_SEA_UPDATE, hook_asg_sea_update) &
            patch_vtable(VT_ASG_AIR, 90, FN_ASG_AIR_UPDATE, hook_asg_air_update);
    if (g_log) fprintf(g_log, "# landing: %s, %d transports (transport subgroup hook %s)\n", g_landing ? "on" : "off", g_land_ships, h ? "ok" : "FAILED");
}
