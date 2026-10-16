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
//   weakest one and bring an army that actually has a chance"), then the most buildings within 20 tiles; the
//   soldiers at home it can spare (land attack subgroups, one kept per enemy soldier at its door) 1.2 times as
//   strong as 1.4 times what holds the beach (ld_strength: groups, not units summed).
// - Size (the user: "early a single transport might do damage, later a large scale landing with 10 ships might be
//   needed"): 1.5 times what holds the beach and the soldiers it knows within 40 tiles behind it, as far as those it
//   can spare and the transports it has or can pay (stock now, one to save for, up to landing_transports, 8) go;
//   the transports full (their type's capacity +0x54C: 10-14), room for the citizens.
// - Ships: its transports (the newest type of its epoch), the missing ones trained (priority orders, their cost
//   reserved); not all there after 3 minutes: those it has if they carry enough; none within 6: given up for 10.
// - Board: whole land subgroups at home (their updates skipped while the landing leads them, as the transports'
//   subgroups), at a meeting point; the game's own load goal (EEUGLoadTransport 0x61487F on the transport with
//   the passenger list, each passenger the goal's vf4) - again every 30 s for those not aboard; all aboard, 9 in 10
//   after a minute, or 3 minutes: it sails with what is aboard (half at least, else given up). 3+ transports
//   gather at sea 10-18 tiles off the beach first (75 s at most) and run in together.
// - Sail and unload: EEUGUnloadTransport (0x615A1F, transport, tile x, y) at the site. Committed: no turning back
//   while transports with soldiers aboard are afloat (towers in the back can sink them - land before). Up to 4
//   warships go along (their subgroups led too): ahead to the water in front of the site, attacking the defences and
//   warships there so the towers shoot at them, then shelling the shore. Towers covering that water count against
//   a site (the beach itself must be out of every known tower's reach).
// - Ashore: one group - raids (ld_raid_target), else marches on the nearest building of the target it has seen that is
//   no defence. Citizens along build a forward base (ld_bridgehead: towers, barracks, town center, hospital, factory -
//   by the size; soldiers trained there join, ld_capture). Waves: the transports bring the soldiers
//   at home over while it holds (ld_wave, landing_waves 3). A quarter left: released to the game.
static int g_landing = 0, g_land_ships = 8;   // aimod.ini [all] landing, landing_transports (the most; the size follows the beach)
static int g_land_waves = 3;   // [all] landing_waves: how often the transports bring more soldiers over (0-10)
static int g_land_cits = 10;   // [all] landing_citizens: the most citizens along to build the forward base (0-10)
static int g_land_trace = 0;   // test: landing_trace=1 - each transport every tick while soldiers are aboard
static int g_land_knows = 0;   // test: landing_knows_map=1 - every building of the target counts, seen or not
static int g_land_flag = 0;   // the unload goal's last argument: the AI's code passes 1, the player's command 0 (test: landing_flag)
#define LD_PASS 128
#define LD_SHIPS 10
// a group's strength: sqrt(hit points x attack, each summed over the group) - N soldiers wear a tower down N times
// faster than one, so a group's hit points x attack per unit summed (unit_power) rated a tower as 49 soldiers
// strengths: soldiers, defences reaching the beach, over the water, the beach together, with the soldiers within 40 tiles
struct ld_def { float soldiers, towers, water, all, behind; int n, assumed; };
// the forward base: what the citizens along build, in this order (several at once while citizens are free)
enum { LDB_TOWER, LDB_BARRACKS, LDB_CENTER, LDB_HOSPITAL, LDB_FACTORY };
struct ld_proj { uint8_t *b; int kind, type, cits, state, tries, x, y; };   // state 0 to come, 1 going up, 2 standing, 3 dropped
static struct landing {
    int phase;                           // 0 idle, 1 ships, 2 board, 3 sail, 4 ashore
    uint8_t *target;
    int sx, sy, ocean, wx, wy;           // landing site, the sea, the water in front of it
    uint8_t *ships[LD_SHIPS]; int nships;
    int want_ships, want_n; float unit_pw, need;   // the size planned: transports, soldiers; a soldier's strength, what the beach needs
    uint8_t *esc[14]; int nesc;          // warships (up to 6) and aircraft (up to 8) along
    uint8_t *pass[LD_PASS]; int npass, launch_n;
    uint8_t *frozen[96]; int nfrozen;    // subgroups led by the landing
    uint8_t *obj; int ox, oy;
    uint32_t since, last, next_try, reissue, trained_t, boarded_t;
    int switched;                        // the beach changed once at the sail check
    int tr_missing, tr_type, tr_got;     // transports still to come, their type, taken at the last order
    uint8_t *cit[10]; int ncit, want_cits; signed char cjob[10];   // citizens along for the forward base, each one's project (-1 none)
    struct ld_proj proj[6]; int nproj; uint32_t base_t;   // the forward base; the last project started
    int ex, ey, ewx, ewy, gathered;      // where they board: the shore nearest the passengers, the water in front
    struct ld_def def;                   // the beach's defence when planned
    int all_ashore, tries_at[LD_SHIPS], ux[LD_SHIPS], uy[LD_SHIPS];   // each transport's own unload point
    int stx, sty, staged; uint32_t stage_t;   // where the transports gather at sea before the run in
    int dkx, dky;                        // its dock nearest the beach (the way over, while the meeting point is not known)
    uint32_t sea_t;                      // since when the sea on the way is held (the sail check)
    int meet;                            // the meeting point found (ex, ey)
    int wst, nw, waves; uint32_t wt, wre;   // the next wave: 0 none, 1 to the meeting point, 2 boarding, 3 at sea; its soldiers
    uint8_t *wpass[LD_PASS];
    unsigned tries, landings, aborted, orders;
} g_ld[16];

static const struct { int epoch, type; } k_transports[] = { { 13, 114 }, { 10, 113 }, { 8, 112 }, { 4, 111 }, { 3, 110 }, { 2, 313 } };
static int is_transport(int t) {
    for (size_t k = 0; k < sizeof k_transports / sizeof *k_transports; k++) if (k_transports[k].type == t) return 1;
    return 0;
}
static int ld_cap(int t) {   // soldiers a transport holds: its type's +0x54C (313/110: 10, 111: 12, 112-114: 14; 110 held 10 in the board test)
    uint8_t *td = t > 0 && t < 0x300 ? ((uint8_t **)G_TYPES)[t] : NULL;
    int c = td ? FIELD(td, 0x54C, int) : 0;
    return c >= 4 && c <= 30 ? c : 10;
}
// citizens along by the landing's size (the user: "gone completely for one ship landings ... 10 for larger landings - so
// five can build a town center, and the others barracks and factories beside the defences, a hospital"): one
// transport none, two 3 (towers), three 6 (a barracks too), four and more 10 (a forward base); landing_citizens at most
static int ld_cits_for(int ships, int cap) {
    int c = ships <= 1 ? 0 : ships == 2 ? 3 : ships == 3 ? 6 : 10;
    if (c > g_land_cits) c = g_land_cits;
    if (c > cap) c = cap;
    return c;
}
static int ld_transport_type(void *pl) {   // the newest transport of its epoch
    int epoch = current_epoch(pl) + 1;
    for (size_t k = 0; k < sizeof k_transports / sizeof *k_transports; k++)
        if (k_transports[k].epoch <= epoch) return k_transports[k].type;
    return 0;
}
static int ld_affordable(void *pl, int t) {   // how many of type t its stock pays now
    int *st = (int *)((uint8_t *)pl + PL_STOCK), n = 20;
    for (int r = 2; r < 7; r++) {
        int c = unit_cost(pl, t, r);
        if (c > 0 && st[r] / c < n) n = st[r] / c;
    }
    return n;
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
    for (int k = 0; k < l->ncit; k++) if (l->cit[k] == u) return 1;
    for (int k = 0; k < l->nw; k++) if (l->wpass[k] == u) return 1;
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

// A subgroup gives one of its units an order through vf57 (thiscall(sg, unit, goal, 0, priority), 0x45CBEA, air
// 0x442AD8; 0 refused - the caller deletes the goal). Its update is skipped while the landing leads it, but its event
// handlers still give orders: the transport subgroup's (0x445A0F, through its second vtable) unloaded the waiting
// landing transports 5 times in one run (the unload at 0x445AAD). Refused while the landing leads the subgroup.
#define FN_ASG_GIVE_GOAL     0x0045CBEA
#define FN_ASG_AIR_GIVE_GOAL 0x00442AD8
static unsigned g_ld_refused;
static char __fastcall hook_asg_give_goal(uint8_t *sg, void *edx, uint8_t *u, void *goal, int a, int prio) {
    (void)edx;
    if (landing_leads(sg)) { g_ld_refused++; return 0; }
    return ((char (__thiscall *)(void *, void *, void *, int, int))FN_ASG_GIVE_GOAL)(sg, u, goal, a, prio);
}
static char __fastcall hook_asg_air_give_goal(uint8_t *sg, void *edx, uint8_t *u, void *goal, int a, int prio) {
    (void)edx;
    if (landing_leads(sg)) { g_ld_refused++; return 0; }
    return ((char (__thiscall *)(void *, void *, void *, int, int))FN_ASG_AIR_GIVE_GOAL)(sg, u, goal, a, prio);
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
    float sh = 0, sa = 0, th = 0, ta = 0, wh = 0, wa = 0, bh = 0, ba = 0;
    d->n = 0;
    if (grid) {
        int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int), r = 40;
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
                        else if (d2 <= 1600.0f) { bh += hp; ba += ld_attack(u); }
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
    rh = ra = 0;
    intel_soldiers(pl, x, y, 40, &rh, &ra, NULL, &rn);   // within 40, the 15 counted above again: a little more caution
    bh += rh; ba += ra;
    d->assumed = 0;
    if (g_honest_vision && intel_looked_ago(pl, x, y) > 180000) {
        float ah = 0, aa = 0; int an = 0;
        intel_army(pl, &ah, &aa, NULL, &an);
        sh += 0.25f * ah; sa += 0.25f * aa; d->assumed = 1;
    }
    d->soldiers = ld_strength(sh, sa); d->towers = ld_strength(th, ta); d->water = ld_strength(wh, wa);
    d->all = ld_strength(sh + th, sa + ta);
    d->behind = ld_strength(sh + th + bh, sa + ta + ba);
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
// its buildings on the continent of its base point: a forward base across the sea is no home (the troops at the beach
// were counted at home and boarded again)
static void ld_home_scan(void *pl) {
    g_ld_home.pl = pl; g_ld_home.nb = g_ld_home.ne = 0;
    grid_each(as_home_unit, &g_ld_home);
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
    unsigned hc = bm ? area_id(FIELD(bm, 0x12C, int), FIELD(bm, 0x130, int), 2) : 0;
    if (!hc) return;
    int n = 0;
    for (int b = 0; b < g_ld_home.nb; b++)
        if (area_id(g_ld_home.bx[b], g_ld_home.by[b], 2) == hc) { g_ld_home.bx[n] = g_ld_home.bx[b]; g_ld_home.by[n++] = g_ld_home.by[b]; }
    g_ld_home.nb = n;
}
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
    l->phase = 0; l->nfrozen = 0; l->nships = 0; l->npass = 0; l->obj = NULL; l->ncit = 0; l->nproj = 0;
    l->wst = 0; l->nw = 0; l->waves = 0;
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
// THE SEA ON THE WAY (the user: "I certainly place subs or planes before my coasts on patrol now"; "subs are always
// visible, but can only be attacked by frigates and towers"; "best to take the game's own table as source of truth"):
// who can hit whom is the game's own factor 0x4D5F2C (thiscall on 0x917DE0: the attacker's weapon kind - type +0x4C8,
// the target's type, both players' indices) - the table 0x8D200C [target category, type +0x4C4][weapon kind], 0: it
// cannot (the game's attack subgroups test it the same way, 0x4504FD). A threat: an enemy unit afloat or in the air
// whose weapon hits the transport type; an escort counts against the threats of a category when its weapon hits them.
enum { LDS_SHIP, LDS_SUB, LDS_AIR };   // for the log only: afloat, submarines (family 10), aircraft
struct ld_sea { float h[64], a[64]; uint8_t *rep[64]; int rpi[64], n[3]; };
static float ld_factor(uint8_t *atd, int api, uint8_t *ttd, int tpi) {
    if (!atd || !ttd || FIELD(atd, 0x510, int) <= 0) return 0.0f;
    return ((float (__thiscall *)(void *, int, void *, int, int))0x004D5F2C)((void *)0x00917DE0, FIELD(atd, 0x4C8, int), ttd, api, tpi);
}
static int ld_pidx(uint8_t *u) { uint8_t *o = FIELD(u, 0x18, uint8_t *); return o ? FIELD(o, 0x45C, int) : -1; }
static int ld_hits(uint8_t *a, uint8_t *t) {   // can a hit t (the game's table)
    return a && t && ld_factor(FIELD(a, 0x2C, uint8_t *), ld_pidx(a), FIELD(t, 0x2C, uint8_t *), ld_pidx(t)) > 0.0f;
}
static int ld_flies(uint8_t *u) { int fam = FIELD(FIELD(u, 0x2C, uint8_t *), 0x4B4, int); return fam == 3 || fam == 7 || fam == 17; }
// an enemy (seen) afloat or in the air that hits the transport type trtd (of player index tpi)
static int ld_sea_foe(void *pl, uint8_t *u, uint8_t *trtd, int tpi) {
    void *o = FIELD(u, 0x18, void *);
    int x = FIELD(u, 0x1C, int), y = FIELD(u, 0x20, int);
    if (!o || FIELD(o, PL_INDEX, int) <= 0 || FIELD(u, 0x3C, int) <= 0 || x < 0 || !is_enemy(pl, o)) return 0;
    if (!ld_flies(u) && !area_id(x, y, 1)) return 0;   // on land: the beach's defence counts it
    return ld_factor(FIELD(u, 0x2C, uint8_t *), ld_pidx(u), trtd, tpi) > 0.0f && seen_by(u, pl);
}
// the threats on the way over: within 10 tiles of the line from (x0, y0) to the water in front of the beach, every
// 5 tiles; aircraft within 25 of the beach too (a patrol)
struct ld_sea_scan { void *pl; uint8_t *trtd; int tpi, npt, px[48], py[48], sx, sy; struct ld_sea *t; };
static void ld_sea_unit(uint8_t *u, void *ctx) {
    struct ld_sea_scan *c = ctx;
    if (!ld_sea_foe(c->pl, u, c->trtd, c->tpi)) return;
    int x = FIELD(u, 0x1C, int), y = FIELD(u, 0x20, int), on = 0, air = ld_flies(u);
    if (air) on = (x - c->sx) * (x - c->sx) + (y - c->sy) * (y - c->sy) <= 625;
    for (int q = 0; q < c->npt && !on; q++) on = (x - c->px[q]) * (x - c->px[q]) + (y - c->py[q]) * (y - c->py[q]) <= 100;
    if (!on) return;
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    int k = FIELD(td, 0x4C4, int) & 63;
    c->t->h[k] += (float)FIELD(u, 0x3C, int); c->t->a[k] += ld_attack(u);
    if (!c->t->rep[k]) { c->t->rep[k] = td; c->t->rpi[k] = ld_pidx(u); }
    c->t->n[air ? LDS_AIR : FIELD(td, 0x4B4, int) == 10 ? LDS_SUB : LDS_SHIP]++;
}
static void ld_sea_threat(void *pl, uint8_t *trtd, int x0, int y0, int wx, int wy, int sx, int sy, struct ld_sea *t) {
    static struct ld_sea_scan c;
    memset(&c, 0, sizeof c); memset(t, 0, sizeof *t);
    c.pl = pl; c.trtd = trtd; c.tpi = FIELD(pl, 0x45C, int); c.sx = sx; c.sy = sy; c.t = t;
    if (!trtd) return;
    float dx = (float)(wx - x0), dy = (float)(wy - y0), len = sqrtf(dx * dx + dy * dy);
    int steps = (int)(len / 5.0f);
    if (steps > 46) steps = 46;
    for (int i = 0; i <= steps; i++) {
        float f = steps ? (float)i / (float)steps : 1.0f;
        c.px[c.npt] = x0 + (int)(dx * f); c.py[c.npt++] = y0 + (int)(dy * f);
    }
    grid_each(ld_sea_unit, &c);
}
static int ld_sea_useful(const struct ld_sea *e, uint8_t *u) {   // its weapon hits some threat on the way
    for (int k = 0; k < 64; k++)
        if (e->rep[k] && ld_factor(FIELD(u, 0x2C, uint8_t *), ld_pidx(u), e->rep[k], e->rpi[k]) > 0.0f) return 1;
    return 0;
}
static void ld_sea_add(struct ld_sea *o, const struct ld_sea *e, uint8_t *u) {   // u (own) against each category it hits
    if (!ld_alive(u)) return;
    for (int k = 0; k < 64; k++)
        if (e->rep[k] && ld_factor(FIELD(u, 0x2C, uint8_t *), ld_pidx(u), e->rep[k], e->rpi[k]) > 0.0f) {
            o->h[k] += (float)FIELD(u, 0x3C, int); o->a[k] += ld_attack(u);
        }
}
// what gets through to the transports, against their hit points: each category's attack for a minute in reach (an
// attack every 2 s), the escorts hitting it stopping at most half (strength against strength): with 2 cruisers
// "matching" 5 strafing aircraft over the beach P2 sailed, the aircraft sank a transport with 14 aboard while the
// cruisers chased them - the escorts must thin a patrol out before the transports go
static float ld_sea_risk(const struct ld_sea *e, const struct ld_sea *o, float tr_hp) {
    float dmg = 0;
    for (int k = 0; k < 64; k++) {
        float es = ld_strength(e->h[k], e->a[k]), os = ld_strength(o->h[k], o->a[k]);
        if (es > 0) dmg += e->a[k] * (os >= es ? 0.5f : 1.0f - 0.5f * os / es) * 30.0f;
    }
    return tr_hp > 0 ? dmg / tr_hp : 0.0f;
}
// the nearest threat to the transports (type trtd) within r tiles that esc can hit (esc NULL: any)
static uint8_t *ld_hunt(void *pl, int x, int y, int r, uint8_t *esc, uint8_t *trtd) {
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid || !trtd) return NULL;
    int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int), tpi = FIELD(pl, 0x45C, int);
    uint8_t *cells = FIELD(grid, 4, uint8_t *), *best = NULL;
    float bd = 1e9f;
    for (int gx = (x - r) / cs; cells && cs > 0 && gx <= (x + r) / cs; gx++)
        for (int gy = (y - r) / cs; gy <= (y + r) / cs; gy++) {
            if (gx < 0 || gy < 0 || gx >= nx || gy >= ny) continue;
            uint8_t *cell = cells + (gx * ny + gy) * 16;
            for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **); b++) {
                uint8_t *u = *b;
                if (!u || !FIELD(u, 0x2C, uint8_t *)) continue;
                float dx = (float)(FIELD(u, 0x1C, int) - x), dy = (float)(FIELD(u, 0x20, int) - y), d2 = dx * dx + dy * dy;
                if (d2 > (float)(r * r) || d2 >= bd || !ld_sea_foe(pl, u, trtd, tpi) || (esc && !ld_hits(esc, u))) continue;
                bd = d2; best = u;
            }
        }
    return best;
}
static float ld_tr_hp(struct landing *l) {   // the transports' hit points
    float h = 0;
    for (int k = 0; k < l->nships; k++) h += ld_alive(l->ships[k]) ? (float)FIELD(l->ships[k], 0x3C, int) : 0.0f;
    return h;
}
static uint8_t *ld_tr_td(struct landing *l) { return l->nships ? FIELD(l->ships[0], 0x2C, uint8_t *) : NULL; }
// the sail check: the threats on the way from (x0, y0) against its escorts
static float ld_sea_now(void *pl, struct landing *l, int x0, int y0, struct ld_sea *e) {
    static struct ld_sea o;
    memset(&o, 0, sizeof o);
    ld_sea_threat(pl, ld_tr_td(l), x0, y0, l->wx, l->wy, l->sx, l->sy, e);
    for (int k = 0; k < l->nesc; k++) ld_sea_add(&o, e, l->esc[k]);
    return ld_sea_risk(e, &o, ld_tr_hp(l));
}

static void ld_escorts(void *pl, struct landing *l) {
    for (int k = 0; k < l->nesc; k++) {
        uint8_t *u = l->esc[k], *g = FIELD(u, 0x1EC, uint8_t *);
        if (g && *(uintptr_t *)g == VT_GOAL_ATTACK && ld_alive(FIELD(g, 0x30, uint8_t *))) continue;
        if (FIELD(u, 0x1C, int) < 0) continue;   // landed at its airport (refuelling)
        int ux = FIELD(u, 0x1C, int), uy = FIELD(u, 0x20, int);
        // the threats to the transports it can hit first (the game's table: frigates the submarines, cruisers and
        // fighters the aircraft ...): off the beach, on the way over, near itself
        uint8_t *trtd = ld_tr_td(l);
        uint8_t *h = ld_hunt(pl, l->wx, l->wy, 16, u, trtd);
        if (!h && l->meet) h = ld_hunt(pl, (l->wx + l->ewx) / 2, (l->wy + l->ewy) / 2, 14, u, trtd);
        if (!h && ld_flies(u)) h = ld_hunt(pl, l->sx, l->sy, 16, u, trtd);
        if (!h) h = ld_hunt(pl, ux, uy, 12, u, trtd);
        if (h) { l->orders += unit_goal(u, goal_attack(u, h), 0, 0x1F40); continue; }
        if (ld_is_air(u)) {   // aircraft: the beach's defences, its soldiers, else over the beach
            uint8_t *e = ld_naval_target(pl, l->sx, l->sy, 12, 0);
            if (!ld_hits(u, e)) e = ld_naval_target(pl, l->wx, l->wy, 10, 0);
            if (!ld_hits(u, e)) e = ld_naval_target(pl, l->sx, l->sy, 12, 1);
            if (!ld_hits(u, e)) e = NULL;   // a fighter: nothing on the ground - over the beach
            if (e) l->orders += unit_goal(u, goal_attack(u, e), 0, 0x1F40);
            else if (!g || !as_goal_near(u, (float)l->sx + 0.5f, (float)l->sy + 0.5f, 3.0f))
                l->orders += unit_goal(u, goal_move((float)l->sx + 0.5f, (float)l->sy + 0.5f), 0, 0x1F40);
            continue;
        }
        // the beach's defences and the warships off it first, then what shoots at it where it is (nearest to itself
        // first: two warships shelled a tower 35 tiles from the beach for minutes), then the soldiers on the beach
        uint8_t *e = ld_naval_target(pl, l->wx, l->wy, 12, 0);
        if (!ld_hits(u, e)) e = ld_naval_target(pl, l->sx, l->sy, 12, 0);
        if (!ld_hits(u, e)) e = ld_naval_target(pl, FIELD(u, 0x1C, int), FIELD(u, 0x20, int), 8, 0);
        if (!ld_hits(u, e)) e = ld_naval_target(pl, l->sx, l->sy, 10, 1);
        if (!ld_hits(u, e)) e = NULL;
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
        if (((int (__thiscall *)(void *))0x0042E347)(sh) <= 0) continue;   // nothing aboard
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
            if (!sg || have || l->nfrozen >= 96) continue;
            for (uint8_t **r = FIELD(sg, 0x54, uint8_t **); r && r < FIELD(sg, 0x58, uint8_t **) && !mine; r++) {
                uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL;
                for (int j = 0; j < l->nships && !mine; j++) mine = u == l->ships[j];
                for (int j = 0; j < l->nesc && !mine; j++) mine = u == l->esc[j];
                for (int j = 0; j < l->npass && !mine; j++) mine = u == l->pass[j];
                for (int j = 0; j < l->nw && !mine; j++) mine = u == l->wpass[j];
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
    int rr = (15 + l->nships) * (15 + l->nships);   // 10 transports 4 tiles apart need more beach
    for (int d = 0; d <= 10 + l->nships; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d || !d ? 1 : 2 * d)) {
                int tx = x + dx, ty = y + dy, ok = 1;
                if ((tx - l->sx) * (tx - l->sx) + (ty - l->sy) * (ty - l->sy) > rr) continue;
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
// the run in: each transport its own point on the beach, 4+ tiles apart (3 when the beach is short of room): with
// one point for all, the first put its soldiers there and the unload of the next ended at once (no room), soldiers
// aboard, the transport idle off the beach
static void ld_run_in(struct landing *l, unsigned pi) {
    for (int k = 0; k < l->nships; k++) { l->ux[k] = -100; l->uy[k] = -100; }
    for (int k = 0; k < l->nships; k++)
        if (((int (__thiscall *)(void *))0x0042E347)(l->ships[k]) <= 0) continue;   // empty (a wave on fewer transports): stays off the beach
        else if (!ld_shore_tile(l, k, l->sx, l->sy, -1, -1, 4, &l->ux[k], &l->uy[k]) && !ld_shore_tile(l, k, l->sx, l->sy, -1, -1, 3, &l->ux[k], &l->uy[k])) {
            l->ux[k] = l->sx; l->uy[k] = l->sy;
        }
    for (int k = 0; k < l->nships; k++) {
        if (l->ux[k] == -100) continue;
        void *g = ld_unload(l->ships[k], l->ux[k], l->uy[k]);
        if (g_log && g) fprintf(g_log, "# landing: P%u: transport at %d,%d runs in, the goal's unload point %d,%d\n", pi,
                                FIELD(l->ships[k], 0x1C, int), FIELD(l->ships[k], 0x20, int), FIELD(g, 0x34, int), FIELD(g, 0x38, int));
        l->orders += unit_goal(l->ships[k], g, 0, 0x1F40);
    }
    l->staged = 1;
}
// transport k with soldiers aboard and no unload order: off the beach (within 10 tiles of its point) the unload ended
// there - the nearest shore tile with room to where it is, away from where it failed; farther off (another order took
// it): its point again
static void ld_reunload(struct landing *l, unsigned pi, int k, uint32_t now) {
    uint8_t *sh = l->ships[k];
    int x0 = FIELD(sh, 0x1C, int), y0 = FIELD(sh, 0x20, int), ux = l->ux[k], uy = l->uy[k], ddx = x0 - ux, ddy = y0 - uy;
    l->tries_at[k]++;
    if (ddx * ddx + ddy * ddy <= 100 && ld_shore_tile(l, k, x0, y0, ux, uy, 2, &ux, &uy)) { l->ux[k] = ux; l->uy[k] = uy; }
    void *g = ld_unload(sh, ux, uy);
    l->orders += unit_goal(sh, g, 0, 0x1F40);
    if (g_log && l->tries_at[k] <= 8)   // the goal's own unload point (+0x34/+0x38, from the transport class vf57)
        fprintf(g_log, "# landing: P%u at %.1f min: transport at %d,%d (hp %d) still loaded - unloads at %d,%d, the goal's point %d,%d\n",
                pi, now / 60000.0, x0, y0, FIELD(sh, 0x3C, int), ux, uy, g ? FIELD(g, 0x34, int) : -1, g ? FIELD(g, 0x38, int) : -1);
}
// where 3+ transports gather at sea before the run in, so they reach the beach together (one after another, the
// first ashore fought alone and each transport arriving late was the towers' and warships' only target): open water
// of the landing's sea 10-18 tiles off the water in front of the beach, towards where they come from, the least
// known threat
static int ld_stage(struct landing *l, unsigned pi, int *ox, int *oy) {
    float dx = (float)(l->ewx - l->wx), dy = (float)(l->ewy - l->wy), len = sqrtf(dx * dx + dy * dy), best = 1e9f;
    if (len < 20.0f) return 0;   // the meeting point is near: they come together anyway
    int found = 0;
    for (int r = 10; r <= 18; r += 4) {
        int x, y;
        if (!ld_open_water(l->wx + (int)(dx * (float)r / len), l->wy + (int)(dy * (float)r / len), (unsigned)l->ocean, &x, &y)) continue;
        int bx = x - l->wx, by = y - l->wy;
        if (bx * bx + by * by < 64) continue;
        float t = threat_at(pi, x, y) + 0.01f * (float)r;
        if (ld_hunt(g_players[pi], x, y, 10, NULL, ld_tr_td(l)) || ld_naval_target(g_players[pi], x, y, 10, 0)) t += 5.0f;   // threats or defences near
        if (t < best) { best = t; *ox = x; *oy = y; found = 1; }
    }
    return found;
}

struct ld_cits { void *pl; int x, y, n; uint8_t *u[32]; int d[32]; };
static void ld_cit_unit(uint8_t *u, void *ctx) {
    struct ld_cits *c = ctx;
    int t = FIELD(FIELD(u, 0x2C, uint8_t *), 0x4B8, int);
    if (FIELD(u, 0x18, void *) != c->pl || (t != 32 && t != 480) || FIELD(u, 0x3C, int) <= 0 || FIELD(u, 0x1C, int) < 0 || c->n >= 32) return;
    int dx = FIELD(u, 0x1C, int) - c->x, dy = FIELD(u, 0x20, int) - c->y, d = dx * dx + dy * dy;
    if (d > 625) return;
    c->u[c->n] = u; c->d[c->n++] = d;
}

// The forward base (the user: "being able to produce units on site is very valuable"): the citizens ashore build, by
// the landing's size (ld_base_plan) - towers, a barracks, a town center (5 citizens), a hospital, a factory - near the
// beach on land of the landed continent: the newest type of each it can build and pay (0x555710), the spot by the
// game's own placement test (can_place), out of known tower reach if it can, 3+ tiles from its other buildings; the
// foundation by the game's own creation (0x62EF8A cdecl(player, type, x, y), the cost paid), the builders with
// EEUGBuildRepair (new 0x5C, 0x60BAD0(goal, player, building, 0)) as the game orders its builders. One started every
// 10 s while citizens are free; a project it cannot pay for a minute is dropped. Free citizens help the others; all
// done: the citizens go back to the game.
static void ld_base_plan(struct landing *l) {
    static const int plan3[] = { LDB_TOWER, LDB_TOWER }, plan6[] = { LDB_TOWER, LDB_BARRACKS, LDB_TOWER },
                     plan10[] = { LDB_TOWER, LDB_BARRACKS, LDB_CENTER, LDB_HOSPITAL, LDB_FACTORY, LDB_TOWER };
    const int *pl = l->ncit >= 8 ? plan10 : l->ncit >= 5 ? plan6 : plan3;
    int n = l->ncit >= 8 ? 6 : l->ncit >= 5 ? 3 : l->ncit ? 2 : 0;
    static const int cits[] = { 3, 2, 5, 2, 3 };
    memset(l->proj, 0, sizeof l->proj);
    for (int k = 0; k < n; k++) { l->proj[k].kind = pl[k]; l->proj[k].cits = cits[pl[k]]; }
    l->nproj = n; l->base_t = 0;
}
static int ld_can_build(void *pl, int t) { return ((char (__thiscall *)(void *, int, int, int))0x00555710)(pl, t, 0x20, 0) != 0; }
static int ld_kind_type(void *pl, int kind) {   // the type of a kind it can build and pay now (0: none)
    static const int towers[] = { 466, 465, 464, 463, 462, 461, 47 }, centers[] = { 35, 43 }, factories[] = { 69, 68, 64, 54 };
    const int *t = NULL; int n = 0;
    switch (kind) {
    case LDB_TOWER: t = towers; n = 7; break;
    case LDB_CENTER: t = centers; n = 2; break;
    case LDB_FACTORY: t = factories; n = 4; break;
    case LDB_BARRACKS: return ld_can_build(pl, 34) ? 34 : 0;
    case LDB_HOSPITAL: return ld_can_build(pl, 62) ? 62 : 0;
    }
    for (int k = 0; k < n; k++) if (ld_can_build(pl, t[k])) return t[k];
    return 0;
}
static const char *ld_kind_name(int kind) {
    static const char *nm[] = { "tower", "barracks", "town center", "hospital", "factory" };
    return kind >= 0 && kind < 5 ? nm[kind] : "?";
}
static int ld_base_spot(void *pl, unsigned pi, struct landing *l, int type, int cx, int cy, int *ox, int *oy) {
    unsigned cont = area_id(cx, cy, 2);
    uint8_t *td = ((uint8_t **)G_TYPES)[type];
    int sz = td ? FIELD(td, 0x58C, int) : 1, found = 0;
    float best = 1e9f;
    if (sz < 1) sz = 1;
    for (int d = 2; d <= 12; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d ? 1 : 2 * d)) {
                int x = cx + dx, y = cy + dy, ok = area_id(x, y, 2) == cont && !area_id(x, y, 1);
                for (int k = 0; k < l->nproj && ok; k++)   // room between its buildings: a way through
                    if (l->proj[k].state && abs(l->proj[k].x - x) < sz + 3 && abs(l->proj[k].y - y) < sz + 3) ok = 0;
                if (!ok || !can_place(pl, type, x, y)) continue;
                float sc = (float)d + 10.0f * threat_at(pi, x, y);
                if (sc < best) { best = sc; *ox = x; *oy = y; found = 1; }
            }
    return found;
}
static void ld_bridgehead(void *pl, unsigned pi, struct landing *l, uint32_t now) {
    int n = 0, cx = 0, cy = 0;
    for (int k = 0; k < l->ncit; k++)
        if (!ld_alive(l->cit[k])) { l->ncit--; l->cit[k] = l->cit[l->ncit]; l->cjob[k] = l->cjob[l->ncit]; k--; }
    for (int k = 0; k < l->ncit; k++) if (!ld_aboard(l->cit[k])) { n++; cx += FIELD(l->cit[k], 0x1C, int); cy += FIELD(l->cit[k], 0x20, int); }
    if (!n) return;
    cx /= n; cy /= n;
    // going up: finished or destroyed - its builders free
    for (int j = 0; j < l->nproj; j++) {
        struct ld_proj *p = &l->proj[j];
        if (p->state != 1) continue;
        uint8_t *bs = ld_alive(p->b) ? FIELD(p->b, 0x30, uint8_t *) : NULL;
        if (ld_alive(p->b) && !(bs && FIELD(bs, 0x340, uint8_t))) continue;
        p->state = ld_alive(p->b) ? 2 : 3;
        if (p->state == 3) p->b = NULL;
        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: forward base - %s %d at %d,%d %s\n", pi, now / 60000.0, ld_kind_name(p->kind), p->type,
                           p->x, p->y, p->state == 2 ? "finished" : "destroyed");
        for (int k = 0; k < l->ncit; k++) if (l->cjob[k] == j) l->cjob[k] = -1;
    }
    // the next project: citizens free, 10 s after the last
    int free = 0, busy = 0;
    for (int k = 0; k < l->ncit; k++) free += !ld_aboard(l->cit[k]) && l->cjob[k] < 0;
    for (int j = 0; j < l->nproj; j++) busy += l->proj[j].state == 1;
    for (int j = 0; j < l->nproj && free && (!l->base_t || now - l->base_t >= 10000); j++) {
        struct ld_proj *p = &l->proj[j];
        if (p->state) continue;
        int need = p->cits < n ? p->cits : n;
        if (busy && free < need) break;   // in order: wait for builders
        int t = ld_kind_type(pl, p->kind), x = 0, y = 0;
        if (!t) {   // it cannot build or pay it: again for a minute (every 10 s), then dropped - the next
            l->base_t = now;
            if (++p->tries >= 6) {
                p->state = 3;
                if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: forward base - no %s it can build and pay, dropped\n", pi, now / 60000.0, ld_kind_name(p->kind));
                continue;
            }
            break;
        }
        uint8_t *b = ld_base_spot(pl, pi, l, t, cx, cy, &x, &y) ? ((uint8_t *(__cdecl *)(void *, int, int, int))0x0062EF8A)(pl, t, x, y) : NULL;
        l->base_t = now;
        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: forward base - %s %d at %d,%d (%s), %d citizens ashore, %d free\n", pi, now / 60000.0,
                           ld_kind_name(p->kind), t, x, y, b ? "foundation laid" : "no room", n, free);
        if (!b) { p->state = 3; continue; }
        p->b = b; p->type = t; p->x = x; p->y = y; p->state = 1;
        for (int m = 0; m < need; m++) {   // the nearest free builders
            int bk = -1, bd = 0;
            for (int k = 0; k < l->ncit; k++) {
                if (ld_aboard(l->cit[k]) || l->cjob[k] >= 0) continue;
                int dx = FIELD(l->cit[k], 0x1C, int) - x, dy = FIELD(l->cit[k], 0x20, int) - y;
                if (bk < 0 || dx * dx + dy * dy < bd) { bk = k; bd = dx * dx + dy * dy; }
            }
            if (bk < 0) break;
            l->cjob[bk] = (signed char)j; free--;
        }
        break;
    }
    // orders: each on its building; the free ones help the first going up
    int helping = -1, left = 0;
    for (int j = 0; j < l->nproj; j++) {
        if (l->proj[j].state == 1 && helping < 0) helping = j;
        left += l->proj[j].state < 2;
    }
    if (!left) {
        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: forward base done - %d citizens back to the game\n", pi, now / 60000.0, l->ncit);
        l->ncit = 0;
        return;
    }
    for (int k = 0; k < l->ncit; k++) {
        int j = l->cjob[k] >= 0 ? l->cjob[k] : helping;
        uint8_t *u = l->cit[k], *g = FIELD(u, 0x1EC, uint8_t *), *b = j >= 0 ? l->proj[j].b : NULL;
        if (!b || ld_aboard(u) || (g && !strcmp(rtti_name(g), "EEUGBuildRepair@@") && FIELD(g, 0x3C, uint8_t *) == b)) continue;
        void *bg = ((void *(__cdecl *)(unsigned))0x00683718)(0x5C);
        if (!bg) continue;
        bg = ((void *(__thiscall *)(void *, void *, void *, int))0x0060BAD0)(bg, pl, b, 0);
        l->orders += unit_goal(u, bg, 0, 0x1F51);
    }
}
// soldiers trained at the forward base (its barracks, town center or factory standing): within 8 tiles of it and not
// the landing's - they join the troops ashore instead of the game's groups at home
struct ld_cap_scan { void *pl; struct landing *l; int got; };
static void ld_capture_unit(uint8_t *u, void *ctx) {
    struct ld_cap_scan *c = ctx;
    struct landing *l = c->l;
    if (FIELD(u, 0x18, void *) != c->pl || !ld_alive(u) || ld_aboard(u) || !is_soldier(FIELD(u, 0x2C, uint8_t *)) || l->npass >= LD_PASS) return;
    int x = FIELD(u, 0x1C, int), y = FIELD(u, 0x20, int), near_base = 0;
    for (int j = 0; j < l->nproj && !near_base; j++) {
        struct ld_proj *p = &l->proj[j];
        if (p->state == 2 && p->kind != LDB_TOWER && p->kind != LDB_HOSPITAL && ld_alive(p->b))
            near_base = (x - p->x) * (x - p->x) + (y - p->y) * (y - p->y) <= 64;
    }
    if (!near_base) return;
    for (int k = 0; k < l->npass; k++) if (l->pass[k] == u) return;
    l->pass[l->npass++] = u; l->launch_n++; c->got++;
}
static void ld_capture(void *pl, unsigned pi, struct landing *l, uint32_t now) {
    int prod = 0;
    for (int j = 0; j < l->nproj; j++) prod |= l->proj[j].state == 2 && l->proj[j].kind != LDB_TOWER && l->proj[j].kind != LDB_HOSPITAL;
    if (!prod) return;
    struct ld_cap_scan c = { pl, l, 0 };
    grid_each(ld_capture_unit, &c);
    if (c.got && g_log) fprintf(g_log, "# landing: P%u at %.1f min: forward base - %d soldiers trained there join the troops\n", pi, now / 60000.0, c.got);
}

static void ld_why(unsigned pi, uint32_t now, const char *why) {   // why no landing is planned (once a minute)
    static uint32_t t[16];
    if (!g_log || (t[pi] && now - t[pi] < 60000 && now >= t[pi])) return;
    t[pi] = now;
    fprintf(g_log, "# landing: P%u at %.1f min: none planned - %s\n", pi, now / 60000.0, why);
}

// subgroups not led by the landing any more: those without a transport, warship, passenger or wave soldier of it
static void ld_prune_frozen(struct landing *l) {
    int nf = 0;
    for (int f = 0; f < l->nfrozen; f++) {
        uint8_t *sg = l->frozen[f];
        int keep = 0;
        for (uint8_t **r = FIELD(sg, 0x54, uint8_t **); r && r < FIELD(sg, 0x58, uint8_t **) && !keep; r++) {
            uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL;
            for (int j = 0; j < l->npass && !keep; j++) keep = u == l->pass[j];
            for (int j = 0; j < l->nw && !keep; j++) keep = u == l->wpass[j];
            for (int j = 0; j < l->nships && !keep; j++) keep = u == l->ships[j];
            for (int j = 0; j < l->nesc && !keep; j++) keep = u == l->esc[j];
        }
        if (keep) l->frozen[nf++] = sg;
    }
    l->nfrozen = nf;
}

// The waves (the user: "early a single transport might do damage, later a large scale landing with 10 ships might be
// needed"): everyone ashore, the transports go back to the meeting point and bring the soldiers at home it can spare
// over to the bridgehead while it holds - up to landing_waves times. A player keeps a landing alive that way, and a
// few transports carry a big landing. 1: to the meeting point, 2: boarding, 3: at sea - ashore they join the others.
static void ld_wave(void *pl, unsigned pi, struct landing *l, uint8_t *dq, unsigned first, unsigned cnt, uint32_t now) {
    for (int k = 0; k < l->nw; k++) if (!ld_alive(l->wpass[k])) { l->wpass[k--] = l->wpass[--l->nw]; }
    int cap = ld_cap(FIELD(FIELD(l->ships[0], 0x2C, uint8_t *), 0x4B8, int));
    if (l->wst == 0) {
        // empty and idle off the enemy beach they were sunk (P2 kept 1 of 6): back to the meeting point meanwhile
        for (int k = 0; k < l->nships && l->all_ashore && l->meet; k++) {
            uint8_t *sh = l->ships[k], *g = FIELD(sh, 0x1EC, uint8_t *);
            int dx = FIELD(sh, 0x1C, int) - l->ewx, dy = FIELD(sh, 0x20, int) - l->ewy;
            if (dx * dx + dy * dy > 49 && (!g || FIELD(g, 5, uint8_t) == 1) && ((int (__thiscall *)(void *))0x0042E347)(sh) <= 0)
                l->orders += unit_goal(sh, goal_move((float)l->ewx + 0.5f, (float)l->ewy + 0.5f), 0, 0x1F40);
        }
        if (!l->all_ashore || l->waves >= g_land_waves || !l->meet || now - l->wt < 20000) return;
        l->wt = now;
        // soldiers at home in land subgroups the game leads (whole subgroups, as the first boarding); an enemy army
        // at its door: one for each of its soldiers stays
        ld_home_scan(pl);
        int home = 0;
        for (unsigned k = 0; k < cnt && k < 64; k++) {
            uint8_t *g = deque_at(dq, first + k);
            for (uint8_t **sp = g ? FIELD(g, 8, uint8_t **) : NULL; sp && sp < FIELD(g, 0xC, uint8_t **); sp++) {
                if (!*sp || *(uintptr_t *)*sp != VT_ASG_LAND || landing_leads(*sp) || (g_assault && assault_leads(*sp))) continue;
                for (uint8_t **r = FIELD(*sp, 0x54, uint8_t **); r && r < FIELD(*sp, 0x58, uint8_t **); r++) {
                    uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL;
                    home += ld_alive(u) && is_soldier(FIELD(u, 0x2C, uint8_t *)) && ld_at_home(FIELD(u, 0x1C, int), FIELD(u, 0x20, int));
                }
            }
        }
        int want = home - as_home_threat(pl);
        if (want > l->nships * cap) want = l->nships * cap;
        if (want < 4) return;
        l->nw = 0;
        for (unsigned k = 0; k < cnt && k < 64 && l->nw < want; k++) {
            uint8_t *g = deque_at(dq, first + k);
            for (uint8_t **sp = g ? FIELD(g, 8, uint8_t **) : NULL; sp && sp < FIELD(g, 0xC, uint8_t **) && l->nw < want; sp++) {
                uint8_t *sg = *sp;
                if (!sg || l->nfrozen >= 96 || *(uintptr_t *)sg != VT_ASG_LAND || landing_leads(sg) || (g_assault && assault_leads(sg))) continue;
                int n = 0, at = 0;
                for (uint8_t **r = FIELD(sg, 0x54, uint8_t **); r && r < FIELD(sg, 0x58, uint8_t **); r++) {
                    uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL;
                    if (!ld_alive(u)) continue;
                    n++; at += ld_at_home(FIELD(u, 0x1C, int), FIELD(u, 0x20, int));
                }
                if (!n || at * 2 < n) continue;
                l->frozen[l->nfrozen++] = sg;
                for (uint8_t **r = FIELD(sg, 0x54, uint8_t **); r && r < FIELD(sg, 0x58, uint8_t **); r++) {
                    uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL;
                    if (ld_alive(u) && !ld_aboard(u) && is_soldier(FIELD(u, 0x2C, uint8_t *)) && l->nw < want && l->nw < LD_PASS) l->wpass[l->nw++] = u;
                }
            }
        }
        if (l->nw < 4) { l->nw = 0; ld_prune_frozen(l); return; }
        for (int a = 0; a < l->nships; a++)   // the nearest the meeting point first: a wave takes only those it needs
            for (int b = a + 1; b < l->nships; b++) {
                int da = (FIELD(l->ships[a], 0x1C, int) - l->ewx) * (FIELD(l->ships[a], 0x1C, int) - l->ewx) + (FIELD(l->ships[a], 0x20, int) - l->ewy) * (FIELD(l->ships[a], 0x20, int) - l->ewy);
                int db = (FIELD(l->ships[b], 0x1C, int) - l->ewx) * (FIELD(l->ships[b], 0x1C, int) - l->ewx) + (FIELD(l->ships[b], 0x20, int) - l->ewy) * (FIELD(l->ships[b], 0x20, int) - l->ewy);
                if (db < da) { uint8_t *t = l->ships[a]; l->ships[a] = l->ships[b]; l->ships[b] = t; }
            }
        l->wst = 1; l->wt = now; l->wre = 0;
        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: wave %d - %d transports back for %d soldiers at home\n", pi, now / 60000.0, l->waves + 1, l->nships, l->nw);
        return;
    }
    if (l->wst == 1) {   // the transports to the water in front of the meeting point, the soldiers to the shore
        int there = 0, close = 0;
        for (int k = 0; k < l->nships; k++) {
            uint8_t *sh = l->ships[k];
            int dx = FIELD(sh, 0x1C, int) - l->ewx, dy = FIELD(sh, 0x20, int) - l->ewy;
            if (dx * dx + dy * dy <= 49) { there++; continue; }
            if (!as_goal_near(sh, (float)l->ewx + 0.5f, (float)l->ewy + 0.5f, 2.0f))
                l->orders += unit_goal(sh, goal_move((float)l->ewx + 0.5f, (float)l->ewy + 0.5f), 0, 0x1F40);
        }
        for (int k = 0; k < l->nw; k++) {
            uint8_t *u = l->wpass[k];
            int dx = FIELD(u, 0x1C, int) - l->ex, dy = FIELD(u, 0x20, int) - l->ey;
            if (dx * dx + dy * dy <= 64) { close++; continue; }
            if (!as_goal_near(u, (float)l->ex + 0.5f, (float)l->ey + 0.5f, 3.0f))
                l->orders += unit_goal(u, goal_move((float)l->ex + 0.5f, (float)l->ey + 0.5f), 0, 0x1F40);
        }
        int need = (l->nw + cap - 1) / cap;
        if (need > l->nships) need = l->nships;
        there = 0;
        for (int k = 0; k < need; k++) {
            int dx = FIELD(l->ships[k], 0x1C, int) - l->ewx, dy = FIELD(l->ships[k], 0x20, int) - l->ewy;
            there += dx * dx + dy * dy <= 49;
        }
        if ((there == need && close * 4 >= l->nw * 3) || now - l->wt >= 180000) { l->wst = 2; l->wt = now; l->wre = 0; }
        return;
    }
    if (l->wst == 2) {   // boarding: loaded and idle at the shore out onto open water (the game unloads it there)
        int aboard = 0;
        for (int k = 0; k < l->nw; k++) aboard += ld_aboard(l->wpass[k]);
        for (int k = 0; k < l->nships; k++) {
            uint8_t *sh = l->ships[k], *g = FIELD(sh, 0x1EC, uint8_t *);
            int wx, wy;
            if ((g && FIELD(g, 5, uint8_t) != 1) || ((int (__thiscall *)(void *))0x0042E347)(sh) <= 0) continue;
            if (ld_open_water(FIELD(sh, 0x1C, int), FIELD(sh, 0x20, int), (unsigned)l->ocean, &wx, &wy))
                l->orders += unit_goal(sh, goal_move((float)wx + 0.5f, (float)wy + 0.5f), 0, 0x1F40);
        }
        if (aboard == l->nw || now - l->wt >= 150000) {
            if (aboard < 4) {
                if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: wave %d - boarding failed (%d of %d aboard)\n", pi, now / 60000.0, l->waves + 1, aboard, l->nw);
                ld_unload_home(l);
                l->nw = 0; l->wst = 0; l->waves++; l->wt = now;
                ld_prune_frozen(l);
                return;
            }
            {   // the sea on the way held: the escorts get 2 more minutes, then the wave is unloaded at home
                struct ld_sea e;
                float risk = ld_sea_now(pl, l, l->ewx, l->ewy, &e);
                if (risk > 0.4f && now - l->wt < 270000) {
                    if (g_log && (now - l->wt) % 30000 < 2000)
                        fprintf(g_log, "# landing: P%u at %.1f min: wave %d waits - the sea holds %d warships, %d submarines, %d aircraft (risk %.2f)\n",
                                pi, now / 60000.0, l->waves + 1, e.n[LDS_SHIP], e.n[LDS_SUB], e.n[LDS_AIR], risk);
                    return;
                }
                if (risk > 0.4f) {
                    if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: wave %d - the sea held, unloaded at home\n", pi, now / 60000.0, l->waves + 1);
                    ld_unload_home(l);
                    l->nw = 0; l->wst = 0; l->waves = g_land_waves; l->wt = now;
                    ld_prune_frozen(l);
                    return;
                }
            }
            int n = 0;
            for (int k = 0; k < l->nw; k++) if (ld_aboard(l->wpass[k])) l->wpass[n++] = l->wpass[k];
            l->nw = n;
            ld_prune_frozen(l);
            memset(l->tries_at, 0, sizeof l->tries_at);
            ld_run_in(l, pi);
            l->wst = 3; l->wt = now;
            if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: wave %d sails with %d soldiers\n", pi, now / 60000.0, l->waves + 1, n);
            return;
        }
        if (l->wre && now - l->wre < 30000) return;
        l->wre = now;
        int per = cap;   // full transports, the nearest first
        for (int k = 0; k < l->nships; k++) {
            uint8_t *list[LD_PASS]; int m = 0;
            for (int j = k * per; j < (k + 1) * per && j < l->nw; j++) if (!ld_aboard(l->wpass[j])) list[m++] = l->wpass[j];
            if (!m) continue;
            uint8_t *vec[4] = { 0, (uint8_t *)list, (uint8_t *)(list + m), (uint8_t *)(list + m) };
            void *g = ((void *(__cdecl *)(unsigned))0x00683718)(0x68);
            if (!g) continue;
            g = ((void *(__thiscall *)(void *, void *, void *, int))0x0061487F)(g, l->ships[k], vec, 1);
            l->orders += unit_goal(l->ships[k], g, 0, 0x1F40);
            for (int j = k * per; j < (k + 1) * per && j < l->nw; j++) {
                if (ld_aboard(l->wpass[j])) continue;
                void *pg = ((void *(__thiscall *)(void *))(*(void ***)g)[4])(g);
                l->orders += unit_goal(l->wpass[j], pg, 0, 0x1F40);
            }
        }
        return;
    }
    // at sea: unloaded again at once when the unload ended with soldiers aboard; ashore they join the others
    for (int k = 0; k < l->nships; k++) {
        uint8_t *sh = l->ships[k], *cur = FIELD(sh, 0x1EC, uint8_t *);
        if ((cur && *(uintptr_t *)cur == 0x00831F5C) || ((int (__thiscall *)(void *))0x0042E347)(sh) <= 0) continue;
        ld_reunload(l, pi, k, now);
    }
    int landed = 0;
    for (int k = 0; k < l->nw; k++) {
        uint8_t *u = l->wpass[k];
        if (ld_aboard(u)) continue;
        if (l->npass < LD_PASS) { l->pass[l->npass++] = u; l->launch_n++; }
        l->wpass[k--] = l->wpass[--l->nw]; landed++;
    }
    if (landed && g_log) fprintf(g_log, "# landing: P%u at %.1f min: wave %d - %d ashore, %d still aboard\n", pi, now / 60000.0, l->waves + 1, landed, l->nw);
    if (!l->nw || now - l->wt >= 600000) {
        l->waves++; l->wst = 0; l->wt = now; l->nw = 0;
        ld_prune_frozen(l);
    }
}

static uint8_t *ld_raid_target(void *pl, uint8_t *target, int x, int y, int r);   // below
static uint8_t *ld_objective(void *pl, unsigned pi, uint8_t *target, unsigned cont, int x, int y);
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
        // the size (the user: "early a single transport might do damage, later a large scale landing with 10 ships
        // might be needed"): as many soldiers as the beach needs, 1.25 times for what lies behind it - from those at
        // home it can spare (an enemy army at its door: one for each of its soldiers stays), on the transports it has or
        // can pay (its stock now, and one to save for), up to landing_transports. Enough for what holds the beach and
        // the soldiers it knows within 40 tiles (they come running), 1.5 times: 9 sent against the beach's tower alone
        // (just enough) lost one boarding and were called off
        int keep = as_home_threat(pl), avail = home - keep;
        if (avail < 6) { ld_why(pi, now, keep ? "an enemy army at its door - no soldiers to spare" : "fewer than 6 soldiers at home"); return; }
        int ttype = ld_transport_type(pl), cap = ld_cap(ttype);
        if (!ttype) { ld_why(pi, now, "no transport in its epoch"); return; }
        int afford = s.nt + ld_affordable(pl, ttype) + 1;
        if (afford > g_land_ships) afford = g_land_ships;
        if (avail > afford * cap) avail = afford * cap;
        float unit_pw = ld_strength(ahp / (float)home, aatt / (float)home), power = unit_pw * (float)avail;   // n alike soldiers: n times one
        struct ld_def d;
        if (!ld_site(pl, pi, &s, ocean, power, &sx, &sy, &swx, &swy, &d)) {
            static char why[96];
            snprintf(why, sizeof why, "no landing site (its docks on sea %u, %u at a shore, %u at that sea)", ocean, g_ld_stat[pi].shore, g_ld_stat[pi].sea);
            ld_why(pi, now, why);
            return;
        }
        if (power < 1.2f * ld_need(&d)) {
            static char why[160];
            snprintf(why, sizeof why, "the weakest beach %d,%d needs %.0f (1.2 x %.0f), its army %.0f (soldiers there %.0f, defences %.0f, over the water %.0f)",
                     sx, sy, 1.2f * ld_need(&d), ld_need(&d), power, d.soldiers, d.towers, d.water);
            ld_why(pi, now, why);
            return;
        }
        float big = 1.4f * d.behind;   // as ld_need, with those behind
        int n = (int)ceilf(1.5f * (big > ld_need(&d) ? big : ld_need(&d)) / unit_pw);
        if (n < 6) n = 6;
        if (n > avail) n = avail;
        // the citizens along take room too; fewer of them when the soldiers would not fit
        int ships = (n + cap - 1) / cap, cits = ld_cits_for(ships, cap);
        {   // the sea on the way over (from its dock nearest the beach) against what it could take along
            int bd = -1;
            for (int k = 0; k < s.nd; k++) {
                int dd = (s.dx[k] - swx) * (s.dx[k] - swx) + (s.dy[k] - swy) * (s.dy[k] - swy);
                if (bd < 0 || dd < bd) { bd = dd; l->dkx = s.dx[k]; l->dky = s.dy[k]; }
            }
            static struct ld_sea e, o;
            uint8_t *ttd = ((uint8_t **)G_TYPES)[ttype];
            memset(&o, 0, sizeof o);
            ld_sea_threat(pl, ttd, l->dkx, l->dky, swx, swy, sx, sy, &e);
            for (int k = 0, m = 0; k < s.nw && m < 6; k++) if (ld_sea_useful(&e, s.war[k])) { ld_sea_add(&o, &e, s.war[k]); m++; }
            for (int k = 0, m = 0; k < s.na && m < 8; k++) if (ld_sea_useful(&e, s.air[k])) { ld_sea_add(&o, &e, s.air[k]); m++; }
            float risk = ld_sea_risk(&e, &o, (float)((ships + 1) * (ttd ? FIELD(ttd, 0x55C, int) : 1000)));
            if (risk > 0.4f) {
                static char why[200];
                snprintf(why, sizeof why, "the sea on the way to %d,%d is held - %d afloat, %d submarines, %d aircraft that hit transports (risk %.2f)",
                         sx, sy, e.n[LDS_SHIP], e.n[LDS_SUB], e.n[LDS_AIR], risk);
                ld_why(pi, now, why);
                return;
            }
        }
        ships = (n + cits + cap - 1) / cap;
        if (ships > afford) ships = afford;
        cits = ld_cits_for(ships, cap);
        if (ships * cap - cits < n) cits = ships * cap - n > 0 ? ships * cap - n : 0;
        n = ships * cap - cits;   // the transports full
        if (n > avail) n = avail;
        l->want_cits = cits;
        l->want_ships = ships; l->want_n = n; l->unit_pw = unit_pw; l->need = ld_need(&d);
        l->target = target; l->sx = sx; l->sy = sy; l->ocean = (int)ocean; l->wx = swx; l->wy = swy; l->def = d;
        l->phase = 1; l->since = now; l->tries++; l->trained_t = 0; l->boarded_t = 0; l->switched = 0; l->tr_missing = 0; l->tr_got = 0; l->sea_t = 0;
        l->nesc = 0; l->nfrozen = 0;
        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: plans a landing on P%d at %d,%d (sea %u) with %d soldiers and %d citizens on %d transports (type %d, %d each) - "
                           "%d at home, %d kept, %d can go (strength %.0f, %d transports it has or can pay); "
                           "the beach needs %.0f, with those behind %.0f (soldiers %.0f%s, defences %.0f, over the water %.0f), %d transports, %d warships\n",
                           pi, now / 60000.0, FIELD(target, PL_INDEX, int), sx, sy, ocean, n, cits, ships, ttype, cap, home, keep, avail, power, afford,
                           ld_need(&d), big, d.soldiers, d.assumed ? " - not looked at, assumed" : "", d.towers, d.water, s.nt, s.nw);
        return;
    }
    if (!l->target || !is_enemy(pl, l->target)) { ld_release(pi, "no enemy any more", now); return; }
    for (int k = 0; k < l->nesc; k++) if (!ld_alive(l->esc[k])) { l->esc[k--] = l->esc[--l->nesc]; }
    if (l->phase >= 2) { ld_refreeze(l, dq, first, cnt); ld_escorts(pl, l); }
    if (l->phase == 1) {   // ships
        memset(&s, 0, sizeof s); s.pl = pl; s.target = l->target;
        grid_each(ld_unit, &s);
        int want = l->want_ships;
        if (s.nt < want && now - l->since >= 180000 && s.nt) {
            // not all there after 3 minutes: those it has, if they carry enough for the beach (a size it cannot reach in
            // time - no waiting for it)
            int cap = ld_cap(ld_transport_type(pl)), cits = ld_cits_for(s.nt, cap), n = s.nt * cap - cits < l->want_n ? s.nt * cap - cits : l->want_n;
            if (l->unit_pw * (float)n >= l->need) {
                if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: goes with %d of %d transports (%d soldiers, strength %.0f, the beach needs %.0f)\n",
                                   pi, now / 60000.0, s.nt, want, n, l->unit_pw * (float)n, l->need);
                l->want_ships = want = s.nt; l->want_n = n; l->want_cits = cits;
            }
        }
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
        // up to 6 warships - the frigates first when submarines are on the way, the cruisers when aircraft are; and
        // its fighters only against aircraft
        static struct ld_sea e;
        ld_sea_threat(pl, ld_tr_td(l), l->dkx, l->dky, l->wx, l->wy, l->sx, l->sy, &e);
        // up to 6 warships, first those whose weapons hit the threats on the way (the game's table)
        for (int pass = 0; pass < 2; pass++)
            for (int k = 0; k < s.nw && l->nesc < 6; k++) {
                uint8_t *u = s.war[k];
                if (u && (pass || ld_sea_useful(&e, u))) { l->esc[l->nesc++] = u; s.war[k] = NULL; }
            }
        // its aircraft too: they bomb the beach's defences and soldiers, else fly over it - a look at the beach
        // (honest_vision) before the transports commit
        // aircraft: those that hit a threat on the way or the beach's tower (a fighter that hits neither stays home)
        uint8_t *tw = ((uint8_t **)G_TYPES)[465];
        for (int k = 0; k < s.na && l->nesc < 14; k++)
            if (ld_sea_useful(&e, s.air[k]) || ld_factor(FIELD(s.air[k], 0x2C, uint8_t *), ld_pidx(s.air[k]), tw, FIELD(l->target, 0x45C, int)) > 0.0f)
                l->esc[l->nesc++] = s.air[k];
        for (unsigned k = 0; k < cnt && k < 64; k++) {
            uint8_t *g = deque_at(dq, first + k);
            for (uint8_t **sp = g ? FIELD(g, 8, uint8_t **) : NULL; sp && sp < FIELD(g, 0xC, uint8_t **); sp++) {
                uint8_t *sg = *sp;
                if (!sg || l->nfrozen >= 96) continue;
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
                    l->npass >= l->want_n)
                    continue;
                l->frozen[l->nfrozen++] = sg;
                for (uint8_t **r = FIELD(sg, 0x54, uint8_t **); r && r < FIELD(sg, 0x58, uint8_t **); r++) {
                    uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL;
                    if (ld_alive(u) && FIELD(u, 0x1C, int) >= 0 && is_soldier(FIELD(u, 0x2C, uint8_t *)) && l->npass < l->want_n && l->npass < LD_PASS)
                        l->pass[l->npass++] = u;
                }
            }
        }
        if (l->npass < 6) { l->aborted++; l->next_try = now + 300000; ld_release(pi, "too few passengers", now); return; }
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
            l->meet = found;
            if (!found) l->gathered = 1;   // none: board where they are
            // citizens for the bridgehead (the user: "a few citizens for a defended bridgehead"): the nearest to the
            // meeting point, within 25 tiles
            struct ld_cits c = { pl, found ? l->ex : x0, found ? l->ey : y0, 0, { 0 }, { 0 } };
            l->ncit = 0;
            if (l->want_cits) grid_each(ld_cit_unit, &c);
            for (int k = 0; k < c.n && l->ncit < l->want_cits; k++) {
                int b = -1;
                for (int j = 0; j < c.n; j++) if (c.u[j] && (b < 0 || c.d[j] < c.d[b])) b = j;
                if (b < 0) break;
                l->cjob[l->ncit] = -1; l->cit[l->ncit++] = c.u[b]; c.u[b] = NULL;
            }
            ld_base_plan(l);
            if (found && g_log) fprintf(g_log, "# landing: P%u at %.1f min: they meet at %d,%d (passengers around %d,%d)\n", pi, now / 60000.0, l->ex, l->ey, x0, y0);
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
        for (int k = 0; k < l->ncit; k++) {
            uint8_t *u = l->cit[k];
            if (!ld_alive(u) || ld_aboard(u) || as_goal_near(u, (float)l->ex + 0.5f, (float)l->ey + 0.5f, 3.0f)) continue;
            int dx = FIELD(u, 0x1C, int) - l->ex, dy = FIELD(u, 0x20, int) - l->ey;
            if (dx * dx + dy * dy > 16) l->orders += unit_goal(u, goal_move((float)l->ex + 0.5f, (float)l->ey + 0.5f), 0, 0x1F40);
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
        // all aboard, 9 in 10 after a minute (P2 waited 3 minutes for its last 3 of 36), or 3 minutes
        if (aboard == l->npass || (now - l->since >= 60000 && aboard * 10 >= l->npass * 9) || now - l->since >= 180000) {
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
            {   // the sea on the way (the user: subs and planes on patrol before the coast): held - the escorts get 3 minutes
                // to clear it, then it is called off
                struct ld_sea e;
                float risk = ld_sea_now(pl, l, l->meet ? l->ewx : FIELD(l->ships[0], 0x1C, int), l->meet ? l->ewy : FIELD(l->ships[0], 0x20, int), &e);
                if (risk > 0.4f) {
                    if (!l->sea_t) l->sea_t = now;
                    if (now - l->sea_t >= 180000) {
                        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: the sea on the way is held - %d warships, %d submarines, %d aircraft (risk %.2f)\n",
                                           pi, now / 60000.0, e.n[LDS_SHIP], e.n[LDS_SUB], e.n[LDS_AIR], risk);
                        ld_unload_home(l);
                        l->aborted++; l->next_try = now + 600000; ld_release(pi, "the sea held - called off, unloaded at home", now);
                        return;
                    }
                    if (g_log && (now - l->sea_t) % 30000 < 2000)
                        fprintf(g_log, "# landing: P%u at %.1f min: waits - on the way %d afloat, %d submarines, %d aircraft that hit transports (risk %.2f), %d escorts\n",
                                pi, now / 60000.0, e.n[LDS_SHIP], e.n[LDS_SUB], e.n[LDS_AIR], risk, l->nesc);
                    return;
                }
                l->sea_t = 0;
            }
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
            int nc = 0;
            for (int k = 0; k < l->ncit; k++) if (ld_alive(l->cit[k]) && ld_aboard(l->cit[k])) { l->cjob[nc] = -1; l->cit[nc++] = l->cit[k]; }
            l->ncit = nc;
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
            l->staged = 0; l->stage_t = now;
            if (l->nships >= 3 && ld_stage(l, pi, &l->stx, &l->sty)) {
                for (int k = 0; k < l->nships; k++) l->orders += unit_goal(l->ships[k], goal_move((float)l->stx + 0.5f, (float)l->sty + 0.5f), 0, 0x1F40);
                if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: the transports gather at sea at %d,%d\n", pi, now / 60000.0, l->stx, l->sty);
            } else ld_run_in(l, pi);
            l->phase = 3; l->since = now; l->reissue = 0; l->all_ashore = 0; memset(l->tries_at, 0, sizeof l->tries_at);
            if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: sails with %d soldiers on %d transports to %d,%d\n", pi, now / 60000.0, n, l->nships, l->sx, l->sy);
            ai_say(pl, CHAT_ATTACK, "[ai] landing with %d units", n);
            return;
        }
        if (l->reissue && now - l->reissue < 30000) return;
        l->reissue = now;
        // per ship its share of those not aboard
        // per ship its share of those not aboard, the citizens with the first (in its share: full transports had no
        // room for them)
        int per = (l->npass + l->ncit + l->nships - 1) / l->nships;
        for (int k = 0; k < l->nships; k++) {
            uint8_t *list[LD_PASS + 4]; int m = 0;
            int lo = k ? k * per - l->ncit : 0, hi = (k + 1) * per - l->ncit;
            if (lo < 0) lo = 0;
            if (k == l->nships - 1 || hi > l->npass) hi = l->npass;
            for (int j = lo; j < hi; j++) if (!ld_aboard(l->pass[j])) list[m++] = l->pass[j];
            for (int j = 0; j < l->ncit && !k; j++) if (ld_alive(l->cit[j]) && !ld_aboard(l->cit[j])) list[m++] = l->cit[j];   // the citizens with the first
            if (!m) continue;
            uint8_t *vec[4] = { 0, (uint8_t *)list, (uint8_t *)(list + m), (uint8_t *)(list + m) };
            void *g = ((void *(__cdecl *)(unsigned))0x00683718)(0x68);
            if (!g) continue;
            g = ((void *(__thiscall *)(void *, void *, void *, int))0x0061487F)(g, l->ships[k], vec, 1);
            l->orders += unit_goal(l->ships[k], g, 0, 0x1F40);
            for (int j = 0; j < l->ncit && !k; j++) {
                if (!ld_alive(l->cit[j]) || ld_aboard(l->cit[j])) continue;
                void *pg = ((void *(__thiscall *)(void *))(*(void ***)g)[4])(g);
                l->orders += unit_goal(l->cit[j], pg, 0, 0x1F40);
            }
            for (int j = lo; j < hi; j++) {
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
    if (l->phase == 3 && !l->staged) {   // gathering at sea: all there (within 6 tiles) or 75 s - the run in
        int there = 0;
        for (int k = 0; k < l->nships; k++) {
            uint8_t *sh = l->ships[k];
            int dx = FIELD(sh, 0x1C, int) - l->stx, dy = FIELD(sh, 0x20, int) - l->sty;
            if (dx * dx + dy * dy <= 36) { there++; continue; }
            if (!as_goal_near(sh, (float)l->stx + 0.5f, (float)l->sty + 0.5f, 2.0f))
                l->orders += unit_goal(sh, goal_move((float)l->stx + 0.5f, (float)l->sty + 0.5f), 0, 0x1F40);
        }
        if (there < l->nships && now - l->stage_t < 75000) return;
        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: %d of %d transports gathered after %.0f s - the run in\n", pi, now / 60000.0,
                           there, l->nships, (now - l->stage_t) / 1000.0);
        ld_run_in(l, pi);
        l->since = now;
        return;
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
            ld_reunload(l, pi, k, now);
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
    if (!aboard && l->nships && l->phase == 4 && !l->all_ashore) {
        l->all_ashore = 1; l->wt = now;
        if (g_log) fprintf(g_log, "# landing: P%u at %.1f min: all %d ashore\n", pi, now / 60000.0, ashore);
    }
    if (l->phase == 4 && l->nships) ld_wave(pl, pi, l, dq, first, cnt, now);
    // ashore: one group (a wave at sea goes on: it lands and holds the beach)
    int alive = ashore + aboard;
    if ((alive * 4 < l->launch_n || !alive) && l->wst != 3) {
        if (l->wst) ld_unload_home(l);
        ld_release(pi, "bridgehead spent", now);
        return;
    }
    if (!ashore) return;
    if (l->ncit) ld_bridgehead(pl, pi, l, now);
    ld_capture(pl, pi, l, now);
    cx /= ashore; cy /= ashore;
    uint8_t *foe = ld_raid_target(pl, l->target, (int)cx, (int)cy, 12);
    if (!l->obj || !ld_alive(l->obj) || FIELD(l->obj, 0x18, uint8_t *) != l->target) {
        unsigned cont = area_id(l->sx, l->sy, 2);   // the beach's land: the centre of a big landing spread along the coast lay in the water
        l->obj = ld_objective(pl, pi, l->target, cont, (int)cx, (int)cy);
        if (!l->obj) l->obj = as_objective(pl, pi, l->target, cont, (int)cx, (int)cy);   // only defences left
        if (l->obj) { l->ox = FIELD(l->obj, 0x1C, int); l->oy = FIELD(l->obj, 0x20, int); }
    }
    for (int k = 0; k < l->npass; k++) {
        uint8_t *u = l->pass[k];
        if (ld_aboard(u) || as_fighting(u)) continue;
        if (foe) {
            uint8_t *e = ld_raid_target(pl, l->target, FIELD(u, 0x1C, int), FIELD(u, 0x20, int), 14);
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

// Ashore the landing raids (the user: "my back is basically undefended ... could cause a lot of trouble"): what it
// sees within r tiles, by kind - soldiers and siege within 8 (they fight back), citizens, buildings of the target
// within 8 that are no defence, defences only when they reach it; nearest first within a kind. (Before: soldiers,
// then any defence within 12, then buildings - the troops walked into towers past the citizens.)
static uint8_t *ld_raid_target(void *pl, uint8_t *target, int x, int y, int r) {
    uint8_t *map = *(uint8_t **)G_MAP;
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return NULL;
    int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
    uint8_t *cells = FIELD(grid, 4, uint8_t *), *best[4] = { 0 };
    float bd[4] = { 1e9f, 1e9f, 1e9f, 1e9f };
    for (int gx = (x - r) / cs; cells && cs > 0 && gx <= (x + r) / cs; gx++)
        for (int gy = (y - r) / cs; gy <= (y + r) / cs; gy++) {
            if (gx < 0 || gy < 0 || gx >= nx || gy >= ny) continue;
            uint8_t *cell = cells + (gx * ny + gy) * 16;
            for (uint8_t **b = FIELD(cell, 4, uint8_t **); b && b < FIELD(cell, 8, uint8_t **); b++) {
                uint8_t *u = *b, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                if (!td || FIELD(u, 0x3C, int) <= 0) continue;
                void *o = FIELD(u, 0x18, void *);
                if (!o || FIELD(o, PL_INDEX, int) <= 0 || !is_enemy(pl, o) || !seen_by(u, pl)) continue;
                float dx = (float)(FIELD(u, 0x1C, int) - x), dy = (float)(FIELD(u, 0x20, int) - y), d2 = dx * dx + dy * dy, rr, a;
                if (d2 > (float)(r * r)) continue;
                int fam = FIELD(td, 0x4B4, int), type = FIELD(td, 0x4B8, int), k;
                if (fam == 3 || fam == 7 || fam == 17 || fam == 4) continue;   // aircraft, ships
                if (is_soldier(td) || is_siege(type)) { if (d2 > 64.0f) continue; k = 0; }
                else if (type == 32 || type == 480) k = 1;   // citizens
                else if (defence_stats(u, &rr, &a)) { if (d2 > (rr + 1.0f) * (rr + 1.0f)) continue; k = 3; }
                else if (o == target && (fam == 2 || fam == 29) && !is_wall(type) && d2 <= 64.0f) k = 2;
                else continue;
                if (d2 < bd[k]) { bd[k] = d2; best[k] = u; }
            }
        }
    for (int k = 0; k < 4; k++) if (best[k]) return best[k];
    return NULL;
}
// the next objective ashore: the nearest building of the target it knows that is no defence (its distance plus 3 per
// tower covering it, on the landed continent)
struct ld_obj { void *pl; uint8_t *target; unsigned pi, cont; int x, y; uint8_t *best; float score; };
static void ld_obj_unit(uint8_t *u, void *ctx) {
    struct ld_obj *s = ctx;
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    int fam = FIELD(td, 0x4B4, int);
    float rr, a;
    if (FIELD(u, 0x18, uint8_t *) != s->target || (fam != 2 && fam != 29) || FIELD(u, 0x3C, int) <= 0 ||
        is_wall(FIELD(td, 0x4B8, int)) || defence_stats(u, &rr, &a) || !seen_by(u, s->pl))
        return;
    int x = FIELD(u, 0x1C, int), y = FIELD(u, 0x20, int);
    float dx = (float)(x - s->x), dy = (float)(y - s->y), sc = sqrtf(dx * dx + dy * dy) + 3.0f * threat_at(s->pi, x, y);
    if ((s->best && sc >= s->score) || !cont_ok(x, y, s->cont)) return;
    s->best = u; s->score = sc;
}
static uint8_t *ld_objective(void *pl, unsigned pi, uint8_t *target, unsigned cont, int x, int y) {
    struct ld_obj s = { pl, target, pi, cont, x, y, NULL, 0 };
    grid_each(ld_obj_unit, &s);
    return s.best;
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
          "tries %u, landings %u, aborted %u, orders %u, subgroup orders refused %u\n", p, g_landing ? "on" : "off", l->phase,
          l->target ? FIELD(l->target, PL_INDEX, int) : -1, l->sx, l->sy, l->ocean, l->wx, l->wy, l->nships, l->nesc, l->npass, aboard, l->nfrozen,
          l->tries, l->landings, l->aborted, l->orders, g_ld_refused);
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

// board P x y [r] (test): each transport of P within r tiles (10) loads every soldier and siege unit of P within r
// (the game's load goal with the whole list); again later: how many each holds - a transport's capacity
struct ld_bd { void *pl; int x, y, r, nt, ns; uint8_t *t[8], *s[64]; };
static void ld_bd_unit(uint8_t *u, void *ctx) {
    struct ld_bd *b = ctx;
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    int dx = FIELD(u, 0x1C, int) - b->x, dy = FIELD(u, 0x20, int) - b->y, t = FIELD(td, 0x4B8, int);
    if (FIELD(u, 0x18, void *) != b->pl || FIELD(u, 0x3C, int) <= 0 || dx * dx + dy * dy > b->r * b->r) return;
    if (is_transport(t) && b->nt < 8) b->t[b->nt++] = u;
    else if ((is_soldier(td) || is_siege(t)) && b->ns < 64) b->s[b->ns++] = u;
}
static void cmd_board(char *out, const char *args) {
    struct ld_bd b = { 0 };
    int p = -1;
    if (!strncmp(args, "stats", 5)) {   // a unit of P1 per type: its stats record (unit +0x34) floats +0x80..+0x13C
        static const int tt[] = { 121, 204, 152, 143, 602, 91, 388, 465 };
        struct ld_bd f = { 0 };
        for (size_t j = 0; j < sizeof tt / sizeof *tt; j++) {
            uint8_t *map = *(uint8_t **)G_MAP, *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL, *hit = NULL;
            if (!grid) break;
            int cs = FIELD(grid, 8, int), nx = FIELD(grid, 0x10, int), ny = FIELD(grid, 0x14, int);
            uint8_t *cells = FIELD(grid, 4, uint8_t *);
            for (int c = 0; c < nx * ny && !hit; c++)
                for (uint8_t **b = FIELD(cells + c * 16, 4, uint8_t **); b && b < FIELD(cells + c * 16, 8, uint8_t **) && !hit; b++)
                    if (*b && FIELD(*b, 0x2C, uint8_t *) && FIELD(FIELD(*b, 0x2C, uint8_t *), 0x4B8, int) == tt[j] && FIELD(*b, 0x18, uint8_t *) && FIELD(FIELD(*b, 0x18, uint8_t *), PL_INDEX, int) == 1 && FIELD(*b, 0x3C, int) > 0) hit = *b;
            uint8_t *st = hit ? FIELD(hit, 0x34, uint8_t *) : NULL;
            if (!st) { REPLY("type %d: none\n", tt[j]); continue; }
            REPLY("type %d:", tt[j]);
            for (int off = 0x80; off < 0x140; off += 4) REPLY(" %X=%g", off, FIELD(st, off, float));
            REPLY("\n");
        }
        for (size_t j = 0; j < sizeof tt / sizeof *tt; j++) {
            uint8_t *td = ((uint8_t **)G_TYPES)[tt[j]];
            REPLY("type data %d:", tt[j]);
            for (int off = 0x410; off < 0x440; off += 4) REPLY(" %X=%g", off, FIELD(td, off, float));
            REPLY(" | 510=%d 4C0=%d 4C4=%d 4C8=%d", FIELD(td, 0x510, int), FIELD(td, 0x4C0, int), FIELD(td, 0x4C4, int), FIELD(td, 0x4C8, int));
            for (int off = 0x514; off < 0x530; off += 4) REPLY(" %X=%d", off, FIELD(td, off, int));
            REPLY("\n");
        }
        (void)f;
        return;
    }
    if (!strncmp(args, "hits", 4)) {   // the game's hit factor (0x4D5F2C): attackers (P2) by row, targets (P1) by column
        static const int at[] = { 193, 163, 399, 250, 71, 72, 379, 305, 465, 145, 152, 146 }, tt[] = { 113, 250, 72, 399, 152, 465 };
        REPLY("attacker:      " );
        for (size_t j = 0; j < sizeof tt / sizeof *tt; j++) REPLY(" %5d", tt[j]);
        REPLY("\n");
        for (size_t k = 0; k < sizeof at / sizeof *at; k++) {
            REPLY("  %3d (kind %2d):", at[k], FIELD(((uint8_t **)G_TYPES)[at[k]], 0x4C8, int));
            for (size_t j = 0; j < sizeof tt / sizeof *tt; j++) REPLY(" %5.2f", ld_factor(((uint8_t **)G_TYPES)[at[k]], 2, ((uint8_t **)G_TYPES)[tt[j]], 1));
            REPLY("\n");
        }
        return;
    }
    if (!strncmp(args, "types", 5)) {   // the type fields (ints) small for every transport, 0 for a soldier (20): capacity?
        static const int tt[] = { 313, 110, 111, 112, 113, 114, 20 };
        for (int off = 0; off < 0x800; off += 4) {
            int ok = 1, v[7];
            for (int k = 0; k < 7; k++) {
                uint8_t *td = ((uint8_t **)G_TYPES)[tt[k]];
                v[k] = td ? FIELD(td, off, int) : -1;
                if (k < 6) ok &= v[k] >= 2 && v[k] <= 60;
            }
            if (ok && v[6] != v[0]) REPLY("+0x%X: %d %d %d %d %d %d, soldier %d\n", off, v[0], v[1], v[2], v[3], v[4], v[5], v[6]);
        }
        return;
    }
    b.r = 10;
    if (sscanf(args, "%d %d %d %d", &p, &b.x, &b.y, &b.r) < 3 || p <= 0 || p >= 16 || !g_players[p]) { REPLY("error: board P x y [r]\n"); return; }
    b.pl = g_players[p];
    grid_each(ld_bd_unit, &b);
    for (int k = 0; k < b.nt; k++) {
        uint8_t *sh = b.t[k];
        int n = ((int (__thiscall *)(void *))0x0042E347)(sh);
        REPLY("transport type %d at %d,%d holds %d", FIELD(FIELD(sh, 0x2C, uint8_t *), 0x4B8, int), FIELD(sh, 0x1C, int), FIELD(sh, 0x20, int), n);
        if (!n && b.ns) {
            uint8_t *vec[4] = { 0, (uint8_t *)b.s, (uint8_t *)(b.s + b.ns), (uint8_t *)(b.s + b.ns) };
            void *g = ((void *(__cdecl *)(unsigned))0x00683718)(0x68);
            if (g) g = ((void *(__thiscall *)(void *, void *, void *, int))0x0061487F)(g, sh, vec, 1);
            if (g) {
                unit_goal(sh, g, 0, 0x1F40);
                for (int j = 0; j < b.ns; j++) {
                    if (ld_aboard(b.s[j])) continue;
                    void *pg = ((void *(__thiscall *)(void *))(*(void ***)g)[4])(g);
                    unit_goal(b.s[j], pg, 0, 0x1F40);
                }
                REPLY(" - loads %d", b.ns);
            }
            b.ns = 0;   // the next transport: the rest later
        }
        REPLY("\n");
    }
    if (!b.nt) REPLY("no transport of P%d within %d of %d,%d\n", p, b.r, b.x, b.y);
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
    g_land_ships = cfg_int(aimod_ini, autotest_ini, 1, "landing_transports", 8);
    g_land_flag = cfg_int(aimod_ini, autotest_ini, 1, "landing_flag", 0);
    g_land_knows = cfg_int(aimod_ini, autotest_ini, 1, "landing_knows_map", 0);
    g_land_trace = cfg_int(aimod_ini, autotest_ini, 1, "landing_trace", 0);
    g_land_cits = cfg_int(aimod_ini, autotest_ini, 1, "landing_citizens", 10);
    g_land_waves = cfg_int(aimod_ini, autotest_ini, 1, "landing_waves", 3);
    if (g_land_waves < 0) g_land_waves = 0;
    if (g_land_waves > 10) g_land_waves = 10;
    if (g_land_cits < 0) g_land_cits = 0;
    if (g_land_cits > 10) g_land_cits = 10;
    if (g_land_ships < 1) g_land_ships = 1;
    if (g_land_ships > LD_SHIPS) g_land_ships = LD_SHIPS;
    if (g_landing) patch_call(0x00483E24, 0x0042C49A, (void *)hook_idle_unload_shore);
    if (g_landing)
        for (size_t k = 0; k < sizeof k_unload_sites / sizeof *k_unload_sites; k++) patch_call(k_unload_sites[k], 0x00615A1F, (void *)hook_unload_ctor);
    int h = patch_vtable(VT_ASG_SEA_TRANSPORT, 90, FN_ASG_SEA_TRANSPORT_UPDATE, hook_asg_sea_transport_update) &
            patch_vtable(VT_ASG_SEA, 90, FN_ASG_SEA_UPDATE, hook_asg_sea_update) &
            patch_vtable(VT_ASG_AIR, 90, FN_ASG_AIR_UPDATE, hook_asg_air_update);
    int gg = 0;
    if (g_landing)   // the subgroups' own orders refused while the landing leads them: transport, sea, land, air
        gg = patch_vtable(VT_ASG_SEA_TRANSPORT, 57, FN_ASG_GIVE_GOAL, hook_asg_give_goal) + patch_vtable(VT_ASG_SEA, 57, FN_ASG_GIVE_GOAL, hook_asg_give_goal) +
             patch_vtable(VT_ASG_LAND, 57, FN_ASG_GIVE_GOAL, hook_asg_give_goal) + patch_vtable(VT_ASG_AIR, 57, FN_ASG_AIR_GIVE_GOAL, hook_asg_air_give_goal);
    if (g_log) fprintf(g_log, "# landing: %s, %d transports (transport subgroup hook %s, order hooks %d/4)\n", g_landing ? "on" : "off", g_land_ships, h ? "ok" : "FAILED", gg);
}
