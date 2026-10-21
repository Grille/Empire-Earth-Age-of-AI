// ASSAULT ([all] assault, default 0 - EXPERIMENTAL, worse in every test so far): the mod leads an AI's land attack
// as one army. Included by eeaimod.c after defence.c (uses its grid_each, is_soldier, unit_power) and attack.c.
//
// Traced on the user's reference save (docs/engine-notes.md, Attacks): an attack on a player has no target at the
// group - every land subgroup of 10 walks its own legs (home, coast points, the player's outlying buildings) and
// waits 1-3 minutes at each; one went through the player's army camped outside the town, died there, and its
// replacements followed in 3s and 4s - the trickle the user saw feeding the camp. Every 2 s per AI:
// - The army: the members of every land subgroup (EECPASGLand) of the attack groups on the target player (the one
//   with the most of them), on the land of the base's continent, not aboard a transport. While the assault leads
//   them, those subgroups' updates (slot 90, 0x4491DD) are skipped - no legs, no target picking of their own.
// - Not while an enemy army is in its territory (3+ soldiers seen within 20 tiles of its buildings): the game leads.
// - Objective: an enemy army within 40 tiles of the base, else the nearest building of the target the AI has seen
//   on the continent (a known defence's threat counts as 3 tiles each). None: the game keeps the attack.
// - Gather at the base. Go when 80% are there (at least 6; after 4 minutes 6) and their strength (hit points x
//   attack) is 1.5 x what the AI has seen at the objective (soldiers, and defences reaching it). Anti-air stays home.
// - March as one: a waypoint 7 tiles ahead of the wave's centre towards the objective (straight - the route
//   planner's tower avoidance is not used), the next once the centre got there. Stragglers go to the centre.
// - Fight together: enemies within 12 tiles of the centre - every member without a fight attacks the nearest.
// - Newcomers wait at the base for the next wave. Half the wave lost (or, with assault_outmatched, facing 1.5 x
//   twice): called off, the next attack within 5 minutes.
// Measured: the reference save with a camp that plays (scripts/refcamp.eeai): P3 killed 6-10 of the player's
// soldiers and 9-16 aircraft (game's attack: 28-34 and 32-34), lost more citizens - the army left the town to the
// camp's pushes. pathtest (scripts/pathtest_as.eeai): P2's 84 went straight at P3's army among the lasers and
// lost half in 1.5 minutes; P3 soldiers killed in 20 minutes 20-33 (game's attack, with the mod's tower-avoiding
// routes: 50). What it lacks: the route planner's way around defences, and a reason to leave a town under attack.
static int g_assault = 0;   // aimod.ini [all] assault
static int g_as_outmatched = 0;   // [all] assault_outmatched: pull back when facing 1.5 x (pathtest: called off at 71 of 84)
#define AS_WAVE 160
static struct assault {
    uint8_t *target;                     // target player
    int phase;                           // 0 none, 1 gather, 2 march
    int cont, rx, ry;                    // home continent, rally point
    uint8_t *obj; int ox, oy;            // objective building
    int wx, wy; uint32_t wset;           // waypoint and when it was set
    uint32_t since, last, logged;
    int nwave, launch_n, bad, doorstep, threat; uint8_t *wave[AS_WAVE];
    uint8_t *taken[64]; int ntaken;      // subgroups led (their updates skipped)
    unsigned launches, spent, done, orders, objectives;
} g_as[16];

// The continent of a tile: EEAIContinentManager (global object 0x917BC8) 0x425F9F thiscall(mgr, int pt[2], kind) - the
// area at the tile (0x425F16), its +0x84 (kind 1: the ocean) or +0x86 (kind 2: the land continent). Reference save:
// both AIs' towns, the player's base and outposts on land continent 2, the water at P3's docks ocean 1; a few areas
// (the player's camp at 108,75) have neither - taken as passable.
static unsigned area_id(int x, int y, int kind) {
    int pt[2] = { x, y };
    if (x < 0 || y < 0) return 0;
    return ((unsigned short (__thiscall *)(void *, int *, int))GA(FN_REGION))((void *)GA(G_MAP_AREAS), pt, kind);
}
static unsigned cont_at(int x, int y) { return area_id(x, y, 2); }
static int cont_ok(int x, int y, unsigned hc) {   // land of continent hc (or an area without ids that is no ocean)
    unsigned c = area_id(x, y, 2);
    return c == hc || (c == 0 && area_id(x, y, 1) == 0 && x >= 0 && y >= 0);
}

static int assault_leads(uint8_t *sg) {
    void *pl = FIELD(sg, 0x190, void *);
    unsigned pi = pl ? (unsigned)FIELD(pl, PL_INDEX, int) & 15 : 0;
    if (!pl || !g_as[pi].phase) return 0;
    for (int k = 0; k < g_as[pi].ntaken; k++) if (g_as[pi].taken[k] == sg) return 1;
    return 0;
}
// still one of AI pl's attack subgroups? A stored subgroup pointer can be deleted by the game between two ticks (a
// group finished, an attack called off) - reading its members then crashed (landing test: a freed record list)
static int asg_live(void *pl, uint8_t *sg) {
    uint8_t *cbm = FIELD(pl, 0xB8C, uint8_t *), *cmd = cbm ? FIELD(cbm, 0x48, uint8_t *) : NULL;
    if (!cmd || !sg) return 0;
    uint8_t *dq = cmd + 4;
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        uint8_t *g = deque_at(dq, first + k);
        for (uint8_t **sp = g ? FIELD(g, 8, uint8_t **) : NULL; sp && sp < FIELD(g, 0xC, uint8_t **); sp++)
            if (*sp == sg) return 1;
    }
    return 0;
}
static int assault_has_unit(void *pl, uint8_t *u) {   // a member of the wave or the army gathering (hold_town leaves them)
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!g_assault || !g_as[pi].phase) return 0;
    for (int k = 0; k < g_as[pi].ntaken; k++) {
        uint8_t *sg = g_as[pi].taken[k];
        if (!asg_live(pl, sg)) continue;
        for (uint8_t **r = FIELD(sg, 0x54, uint8_t **); r && r < FIELD(sg, 0x58, uint8_t **); r++)
            if (*r && FIELD(*r, 0x44, uint8_t *) == u) return 1;
    }
    return 0;
}

static int landing_leads(uint8_t *sg);   // landing.c
static void __fastcall hook_asg_update(uint8_t *sg, void *edx, int now) {
    (void)edx;
    if (!is_vanilla(FIELD(sg, 0x190, void *)) && ((g_assault && assault_leads(sg)) || landing_leads(sg))) return;
    GF(FN_ASG_UPDATE)(sg, now);
}

struct as_obj { void *pl; uint8_t *target; unsigned pi, cont; int x, y; uint8_t *best; float score; };
static void as_obj_unit(uint8_t *u, void *ctx) {
    struct as_obj *s = ctx;
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    int fam = FIELD(td, 0x4B4, int);
    if (FIELD(u, 0x18, uint8_t *) != s->target || (fam != 2 && fam != 29) || FIELD(u, 0x3C, int) <= 0 ||
        is_wall(FIELD(td, 0x4B8, int)) || !seen_by(u, s->pl))
        return;
    int x = FIELD(u, 0x1C, int), y = FIELD(u, 0x20, int);
    float dx = (float)(x - s->x), dy = (float)(y - s->y), sc = sqrtf(dx * dx + dy * dy) + 3.0f * threat_at(s->pi, x, y);
    if (s->best && sc >= s->score) return;
    if (!cont_ok(x, y, s->cont)) return;
    s->best = u; s->score = sc;
}
static uint8_t *as_objective(void *pl, unsigned pi, uint8_t *target, unsigned cont, int x, int y) {
    struct as_obj s = { pl, target, pi, cont, x, y, NULL, 0 };
    grid_each(as_obj_unit, &s);
    return s.best;
}

// the nearest enemy within r tiles of (x, y): soldiers and siege first (r), then defences, then (buildings != 0) any
// building of the target within 8 tiles
static uint8_t *as_enemy_near(void *pl, uint8_t *target, int x, int y, int r, int buildings, int *soldiers) {
    uint8_t *map = *(uint8_t **)GA(G_MAP);
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    if (!grid) return NULL;
    uint8_t *best[3] = { 0 };
    float bd[3] = { 1e9f, 1e9f, 1e9f };
    if (soldiers) *soldiers = 0;
    for (struct grid_it it = grid_it_box(x, y, r, 1); grid_it_next(&it); ) {
        uint8_t *u = it.u, *td = it.td;
        if (!td || FIELD(u, 0x3C, int) <= 0) continue;
        void *o = FIELD(u, 0x18, void *);
        if (!o || FIELD(o, PL_INDEX, int) <= 0 || !is_enemy(pl, o)) continue;
        float dx = (float)(FIELD(u, 0x1C, int) - x), dy = (float)(FIELD(u, 0x20, int) - y), d2 = dx * dx + dy * dy;
        if (d2 > (float)(r * r)) continue;
        int fam = FIELD(td, 0x4B4, int), type = FIELD(td, 0x4B8, int), k;
        float rr, a;
        if (fam == 3 || fam == 7 || fam == 17 || fam == 4) continue;   // aircraft, ships: out of a land army's reach
        if (is_soldier(td) || is_siege(type)) { k = 0; if (soldiers) (*soldiers)++; }
        else if (defence_stats(u, &rr, &a)) k = 1;
        else if (buildings && o == target && (fam == 2 || fam == 29) && !is_wall(type) && d2 <= 64.0f) k = 2;
        else continue;
        if (d2 < bd[k]) { bd[k] = d2; best[k] = u; }
    }
    return best[0] ? best[0] : best[1] ? best[1] : best[2];
}

static int is_aa(uint8_t *u) {   // anti-air units (AA tank, Stinger, Skywatcher): they stay home
    int t = FIELD(FIELD(u, 0x2C, uint8_t *), 0x4B8, int);
    return t == UT(AA10_TANK) || t == UT(AA10_STINGER_SOLDIER) || t == UT(AA13_TANK_SKYWATCHER);
}
// what player pl has seen of its enemies within r tiles of (x, y): soldiers and siege (hit points x attack), with
// defences != 0 also defences whose range reaches the point; the soldiers' centre and number
static float as_enemy_power(void *pl, int x, int y, int r, float *cx, float *cy, int *n, int defences) {
    uint8_t *map = *(uint8_t **)GA(G_MAP);
    uint8_t *grid = map ? FIELD(map, G_UNIT_GRID_OFS, uint8_t *) : NULL;
    float pow = 0, sx = 0, sy = 0;
    *n = 0; *cx = (float)x; *cy = (float)y;
    if (!grid) return 0;
    int rr = r + 12;
    for (struct grid_it it = grid_it_box(x, y, rr, 1); grid_it_next(&it); ) {
        uint8_t *u = it.u, *td = it.td;
        if (!td || FIELD(u, 0x3C, int) <= 0) continue;
        void *o = FIELD(u, 0x18, void *);
        if (!o || FIELD(o, PL_INDEX, int) <= 0 || !is_enemy(pl, o) || !seen_by(u, pl)) continue;
        float dx = (float)(FIELD(u, 0x1C, int) - x), dy = (float)(FIELD(u, 0x20, int) - y), d2 = dx * dx + dy * dy;
        float range, att;
        if (is_soldier(td) || is_siege(FIELD(td, 0x4B8, int))) {
            if (d2 > (float)(r * r)) continue;
            pow += unit_power(u); sx += (float)FIELD(u, 0x1C, int); sy += (float)FIELD(u, 0x20, int); (*n)++;
        } else if (defences && defence_stats(u, &range, &att) && d2 <= (range + 2.0f) * (range + 2.0f))
            pow += (float)FIELD(u, 0x3C, int) * att;
    }
    if (*n) { *cx = sx / *n; *cy = sy / *n; }
    if (g_honest_vision) {   // soldiers seen there lately, out of sight now (intel.c; nothing with the game's view)
        float h = 0, a = 0; int m = 0;
        intel_soldiers(pl, x, y, r, &h, &a, &pow, &m);
        *n += m;
    }
    return pow;
}

// Stored unit pointers: a unit's memory is freed when it dies (and reused) - read only while it is still on the map
// (the objective, a building at its spot) or still a member (the wave, against this tick's members)
struct as_find { uint8_t *u; int found; };
static void as_find_fn(uint8_t *u, void *ctx) { struct as_find *f = ctx; if (u == f->u) f->found = 1; }
static int as_on_map(uint8_t *u, int x, int y) {
    struct as_find f = { u, 0 };
    if (u) grid_box(x, y, 2, as_find_fn, &f);
    return f.found;
}
static int as_member(uint8_t **mem, int n, uint8_t *u) {
    for (int j = 0; j < n; j++) if (mem[j] == u) return 1;
    return 0;
}
static int as_in_wave(struct assault *a, uint8_t *u) {
    for (int k = 0; k < a->nwave; k++) if (a->wave[k] == u) return 1;
    return 0;
}
static int as_goal_near(uint8_t *u, float x, float y, float r) {   // walking there already
    uint8_t *g = FIELD(u, 0x1EC, uint8_t *);
    return g && *(uintptr_t *)g == GA(VT_GOAL_LOCATION) && fabsf(FIELD(g, 0x20, float) - x) < r && fabsf(FIELD(g, 0x24, float) - y) < r;
}
static int as_fighting(uint8_t *u) {
    uint8_t *g = FIELD(u, 0x1EC, uint8_t *), *t = g && *(uintptr_t *)g == GA(VT_GOAL_ATTACK) ? FIELD(g, 0x30, uint8_t *) : NULL;
    return t && FIELD(t, 0x3C, int) > 0;
}

static void as_end(void *pl, unsigned pi, uint32_t now, const char *why, int n) {
    struct assault *a = &g_as[pi];
    int t = FIELD(a->target, PL_INDEX, int) & 15;
    g_cancel_why[pi][t] = why;
    cancel_citizen_attack((int)pi, t);
    uint32_t next = now + 300000;
    if (FIELD(pl, 0xED8, uint32_t) > next) { FIELD(pl, 0xED8, uint32_t) = next; g_last_attack_time[pi] = next; }
    if (g_log) fprintf(g_log, "# assault: P%u at %.1f min: %s on P%d - %d of %d units left; next attack at %.1f min\n",
                       pi, now / 60000.0, why, t, n, a->launch_n, FIELD(pl, 0xED8, uint32_t) / 60000.0);
    ai_say(pl, CHAT_ATTACK, "[ai] %s: %d units left", why, n);
    a->phase = 0; a->ntaken = 0; a->nwave = 0; a->obj = NULL;
}

struct as_home { void *pl; int nb, bx[512], by[512]; int ne, ex[512], ey[512]; };
static void as_home_unit(uint8_t *u, void *ctx) {
    struct as_home *h = ctx;
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    void *o = FIELD(u, 0x18, void *);
    if (!o || FIELD(u, 0x3C, int) <= 0) return;
    int fam = FIELD(td, 0x4B4, int);
    if (o == h->pl) {
        if ((fam == 2 || fam == 29) && !is_wall(FIELD(td, 0x4B8, int)) && h->nb < 512) { h->bx[h->nb] = FIELD(u, 0x1C, int); h->by[h->nb++] = FIELD(u, 0x20, int); }
    } else if (FIELD(o, PL_INDEX, int) > 0 && is_enemy(h->pl, o) && (is_soldier(td) || is_siege(FIELD(td, 0x4B8, int))) &&
               seen_by(u, h->pl) && h->ne < 512) { h->ex[h->ne] = FIELD(u, 0x1C, int); h->ey[h->ne++] = FIELD(u, 0x20, int); }
}
// enemy soldiers and siege the AI sees within 20 tiles of any of its buildings: an army in its territory
static int as_home_threat(void *pl) {
    static struct as_home h;
    h.pl = pl; h.nb = h.ne = 0;
    grid_each(as_home_unit, &h);
    int n = 0;
    for (int e = 0; e < h.ne; e++)
        for (int b = 0; b < h.nb; b++) {
            int dx = h.ex[e] - h.bx[b], dy = h.ey[e] - h.by[b];
            if (dx * dx + dy * dy <= 400) { n++; break; }
        }
    return n;
}

static void assault_tick(void *pl, uint32_t now) {
    unsigned pi = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    struct assault *a = &g_as[pi];
    if (!g_assault) return;
    if (now < a->last) memset(a, 0, sizeof *a);   // a new game
    if (now - a->last < 2000) return;
    a->last = now;
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
    uint8_t *cbm = FIELD(pl, 0xB8C, uint8_t *), *cmd = cbm ? FIELD(cbm, 0x48, uint8_t *) : NULL;
    if (!cmd || !bm || FIELD(bm, 0x12C, int) == -10000) { a->phase = 0; a->ntaken = 0; return; }
    int bx = FIELD(bm, 0x12C, int), by = FIELD(bm, 0x130, int);
    unsigned hc = cont_at(bx, by);
    // the army: land subgroups of the groups on one target (the one with the most members)
    uint8_t *dq = cmd + 4, *tgt[16] = { 0 };
    int tn[16] = { 0 }, nt = 0;
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        uint8_t *g = deque_at(dq, first + k), *t = g ? FIELD(g, 0x48, uint8_t *) : NULL;
        if (!t || !is_enemy(pl, t)) continue;
        int j = 0;
        while (j < nt && tgt[j] != t) j++;
        if (j == nt) { if (nt == 16) continue; tgt[nt++] = t; }
        for (uint8_t **sp = FIELD(g, 8, uint8_t **); sp && sp < FIELD(g, 0xC, uint8_t **); sp++)
            if (*sp && *(uintptr_t *)*sp == GA(VT_ASG_LAND)) tn[j] += (int)(FIELD(*sp, 0x58, uint8_t **) - FIELD(*sp, 0x54, uint8_t **));
    }
    uint8_t *target = NULL;
    for (int j = 0, bestn = 0; j < nt; j++) if (tn[j] > bestn) { bestn = tn[j]; target = tgt[j]; }
    if (!target || !hc || g_mus[pi].on) {   // no attack, or the muster has the soldiers at home
        if (a->phase && g_log) fprintf(g_log, "# assault: P%u at %.1f min: %s - the game leads again\n", pi, now / 60000.0,
                                       !target ? "no attack" : "muster");
        a->phase = 0; a->ntaken = 0; a->nwave = 0;
        return;
    }
    if (target != a->target) { a->phase = 0; a->nwave = 0; a->obj = NULL; a->target = target; }
    // an enemy army in its territory (3+ soldiers within 20 tiles of its buildings): the soldiers fight it where it
    // is, as the game has them (reference save, the player's camp pushing into P3's town: a wave gathered at the base
    // and marching out left the town to it - P3 killed 6 of the player's soldiers and 13 aircraft instead of 31 and
    // 33, lost 75 citizens instead of 60). A wave already out goes on.
    if (a->phase != 2) {
        int threat = as_home_threat(pl);
        if (threat >= 3) {
            if ((a->phase || !a->threat) && g_log)
                fprintf(g_log, "# assault: P%u at %.1f min: %d enemy soldiers in its territory - the game leads\n", pi, now / 60000.0, threat);
            a->threat = 1; a->phase = 0; a->ntaken = 0; a->nwave = 0;
            return;
        }
        a->threat = 0;
    }
    // members: on land, not aboard; subgroups with members aboard a transport stay with the game
    static uint8_t *mem[512];
    int n = 0;
    uint8_t *taken[64];
    int ntaken = 0;
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        uint8_t *g = deque_at(dq, first + k);
        if (!g || FIELD(g, 0x48, uint8_t *) != target) continue;
        for (uint8_t **sp = FIELD(g, 8, uint8_t **); sp && sp < FIELD(g, 0xC, uint8_t **); sp++) {
            uint8_t *sg = *sp;
            if (!sg || *(uintptr_t *)sg != GA(VT_ASG_LAND) || ntaken == 64) continue;
            int aboard = 0, m0 = n;
            for (uint8_t **r = FIELD(sg, 0x54, uint8_t **); r && r < FIELD(sg, 0x58, uint8_t **); r++) {
                uint8_t *u = *r ? FIELD(*r, 0x44, uint8_t *) : NULL, *td = u ? FIELD(u, 0x2C, uint8_t *) : NULL;
                if (!td || FIELD(u, 0x3C, int) <= 0) continue;
                if (FIELD(u, 0x1C, int) < 0) { aboard = 1; continue; }
                if (!cont_ok(FIELD(u, 0x1C, int), FIELD(u, 0x20, int), hc)) { aboard = 1; continue; }   // landed elsewhere
                if (is_citizen_type(FIELD(td, 0x4B8, int))) continue;
                if (n < 512) mem[n++] = u;
            }
            if (aboard) { n = m0; continue; }
            taken[ntaken++] = sg;
        }
    }
    // objective: an enemy army at the doorstep (3+ soldiers or siege seen within 40 tiles of the base) first - it
    // stood on the way (reference save: the player's camp on the only land route), and a wave marching past it left
    // the town to it; else the nearest building of the target on the continent
    float hx = 0, hy = 0, wave_pow = 0, epow = 0;
    int hn = 0;
    float hpow = as_enemy_power(pl, bx, by, 40, &hx, &hy, &hn, 0);
    int doorstep = hn >= 3;
    if (doorstep) { a->obj = NULL; a->ox = (int)hx; a->oy = (int)hy; }
    else if (!a->obj || a->doorstep || !as_on_map(a->obj, a->ox, a->oy) || FIELD(a->obj, 0x3C, int) <= 0 || FIELD(a->obj, 0x18, uint8_t *) != target) {
        int fx = bx, fy = by;
        if (a->phase == 2 && a->nwave) {   // from the wave's centre
            float sx = 0, sy = 0; int m = 0;
            for (int k = 0; k < a->nwave; k++) if (as_member(mem, n, a->wave[k])) { sx += FIELD(a->wave[k], 0x1C, int); sy += FIELD(a->wave[k], 0x20, int); m++; }
            if (m) { fx = (int)(sx / m); fy = (int)(sy / m); }
        }
        uint8_t *o = as_objective(pl, pi, target, hc, fx, fy);
        if ((a->obj || a->doorstep) && o) a->objectives++;
        a->obj = o;
        if (o) { a->ox = FIELD(o, 0x1C, int); a->oy = FIELD(o, 0x20, int); }
    }
    a->doorstep = doorstep;
    if (!a->obj && !doorstep) {
        if (a->phase == 2) { a->done++; as_end(pl, pi, now, "assault (nothing left to attack)", n); return; }
        if (a->phase && g_log) fprintf(g_log, "# assault: P%u at %.1f min: no objective of P%d on its continent - the game leads\n",
                                       pi, now / 60000.0, FIELD(target, PL_INDEX, int));
        a->phase = 0; a->ntaken = 0;
        return;
    }
    memcpy(a->taken, taken, sizeof *taken * ntaken);
    a->ntaken = ntaken;
    if (!a->phase) {   // a new assault: the army gathers at the base (where its towers help, and it guards the town)
        a->rx = bx; a->ry = by;
        a->phase = 1; a->since = now; a->nwave = 0; a->cont = (int)hc; a->logged = 0; a->bad = 0;
        if (g_log) fprintf(g_log, "# assault: P%u at %.1f min: attack on P%d, %d units - gathering at %d,%d; objective %s at %d,%d\n",
                           pi, now / 60000.0, FIELD(target, PL_INDEX, int), n, a->rx, a->ry,
                           doorstep ? "the enemy army at the doorstep" : "a building", a->ox, a->oy);
    }
    float rx = (float)a->rx + 0.5f, ry = (float)a->ry + 0.5f;
    // the reserve (not in the wave): to the rally point, wait there (fights near home go on)
    int at_rally = 0, reserve = 0;
    for (int k = 0; k < n; k++) {
        uint8_t *u = mem[k];
        if (a->phase == 2 && as_in_wave(a, u)) continue;
        reserve++;
        float dx = (float)FIELD(u, 0x1C, int) + 0.5f - rx, dy = (float)FIELD(u, 0x20, int) + 0.5f - ry;
        if (dx * dx + dy * dy <= 100.0f) { at_rally++; if (!is_aa(u)) wave_pow += unit_power(u); continue; }
        if (as_fighting(u) && dx * dx + dy * dy <= 900.0f) continue;   // defending near home
        if (!as_goal_near(u, rx, ry, 3.0f)) a->orders += unit_goal(u, goal_move(rx, ry), 0, 0x1F40);
    }
    if (a->phase == 1) {
        // strong enough: 1.5 x what it has seen there - the army at the doorstep, or soldiers and defences at the objective
        if (doorstep) epow = hpow;
        else { float ex, ey; int en; epow = as_enemy_power(pl, a->ox, a->oy, 15, &ex, &ey, &en, 1); }
        // honest_vision: an objective not looked at for 3 minutes - a quarter of the army it knows of may be there
        if (!doorstep && g_honest_vision && intel_looked_ago(pl, a->ox, a->oy) > 180000) {
            float h = 0, at = 0, pw = 0; int n2 = 0;
            intel_army(pl, &h, &at, &pw, &n2);
            epow += 0.25f * pw;
        }
        int need = reserve * 4 / 5;
        if (need < 6) need = 6;
        int strong = wave_pow >= 1.5f * epow;
        int go = strong && ((at_rally >= need && now - a->since >= 20000) || (at_rally >= 6 && now - a->since >= 240000));
        if (g_log && now - a->logged >= 60000) {
            a->logged = now;
            fprintf(g_log, "# assault: P%u at %.1f min: gathering %d of %d at %d,%d (need %d), strength %.0f against %.0f%s\n", pi,
                    now / 60000.0, at_rally, reserve, a->rx, a->ry, need, wave_pow, epow, doorstep ? " at the doorstep" : " at the objective");
        }
        if (!go) return;
        a->nwave = 0;
        for (int k = 0; k < n && a->nwave < AS_WAVE; k++) {   // anti-air stays home
            float dx = (float)FIELD(mem[k], 0x1C, int) + 0.5f - rx, dy = (float)FIELD(mem[k], 0x20, int) + 0.5f - ry;
            if (dx * dx + dy * dy <= 100.0f && !is_aa(mem[k])) a->wave[a->nwave++] = mem[k];
        }
        a->launch_n = a->nwave; a->phase = 2; a->wx = a->rx; a->wy = a->ry; a->wset = 0; a->launches++; a->logged = now;
        float dx = (float)(a->ox - a->rx), dy = (float)(a->oy - a->ry);
        if (g_log) fprintf(g_log, "# assault: P%u at %.1f min: goes with %d units (strength %.0f against %.0f) from %d,%d -> %s of P%d at %d,%d (%.0f tiles)\n",
                           pi, now / 60000.0, a->nwave, wave_pow, epow, a->rx, a->ry, doorstep ? "the army" : "a building",
                           FIELD(target, PL_INDEX, int), a->ox, a->oy, sqrtf(dx * dx + dy * dy));
        ai_say(pl, CHAT_ATTACK, "[ai] assault with %d units", a->nwave);
    }
    // the wave: alive members, its centre and strength
    int alive = 0;
    float cx = 0, cy = 0, pow = 0;
    for (int k = 0; k < a->nwave; k++) {
        uint8_t *u = a->wave[k];
        int still = 0;
        for (int j = 0; j < n && !still; j++) still = mem[j] == u;   // still a member (not dead, not handed back)
        if (!still) { a->wave[k--] = a->wave[--a->nwave]; continue; }
        alive++; cx += (float)FIELD(u, 0x1C, int); cy += (float)FIELD(u, 0x20, int); pow += unit_power(u);
    }
    if (alive * 2 < a->launch_n || !alive) { a->spent++; as_end(pl, pi, now, "assault (spent)", alive); return; }
    cx /= alive; cy /= alive;
    int icx = (int)cx, icy = (int)cy, soldiers = 0;
    float odx = (float)a->ox - cx, ody = (float)a->oy - cy, od = sqrtf(odx * odx + ody * ody);
    int at_obj = od <= 6.0f && a->obj;
    uint8_t *foe = as_enemy_near(pl, target, icx, icy, 12, at_obj, &soldiers);
    {   // outmatched where it stands (1.5 x its strength, twice in a row): back home
        float ex, ey; int en;
        float here = foe ? as_enemy_power(pl, icx, icy, 12, &ex, &ey, &en, 1) : 0;
        a->bad = here > 1.5f * pow ? a->bad + 1 : 0;
        if (g_as_outmatched && a->bad >= 2) { a->spent++; as_end(pl, pi, now, "assault (outmatched)", alive); return; }
    }
    if (g_log && now - a->logged >= 60000) {
        a->logged = now;
        fprintf(g_log, "# assault: P%u at %.1f min: wave %d of %d at %.0f,%.0f, %.0f tiles to the objective, %s; reserve %d (%d at the rally point)\n",
                pi, now / 60000.0, alive, a->launch_n, cx, cy, od, foe ? "fighting" : "marching", reserve, at_rally);
    }
    for (int k = 0; k < a->nwave; k++) {
        uint8_t *u = a->wave[k];
        float ux = (float)FIELD(u, 0x1C, int), uy = (float)FIELD(u, 0x20, int);
        float dx = ux - cx, dy = uy - cy;
        if (foe) {   // fight together: the nearest enemy to each one without a fight
            if (as_fighting(u)) continue;
            uint8_t *e = as_enemy_near(pl, target, (int)ux, (int)uy, 14, at_obj, NULL);
            if (!e) e = foe;
            a->orders += unit_goal(u, goal_attack(u, e), 0, 0x1F40);
            continue;
        }
        if (dx * dx + dy * dy >= 144.0f) {   // a straggler: to the centre
            if (!as_goal_near(u, cx + 0.5f, cy + 0.5f, 4.0f)) a->orders += unit_goal(u, goal_move(cx + 0.5f, cy + 0.5f), 0, 0x1F40);
            continue;
        }
    }
    if (foe) return;
    if (at_obj) {   // the objective's surroundings clear, the objective standing: attack it
        for (int k = 0; k < a->nwave; k++)
            if (!as_fighting(a->wave[k])) a->orders += unit_goal(a->wave[k], goal_attack(a->wave[k], a->obj), 0, 0x1F40);
        return;
    }
    // march: the next waypoint once the centre reached the last (or after 30 s)
    float wdx = (float)a->wx - cx, wdy = (float)a->wy - cy;
    if (!a->wset || wdx * wdx + wdy * wdy <= 16.0f || now - a->wset >= 30000) {
        int wx = a->ox, wy = a->oy;
        if (od > 7.0f) {
            wx = (int)(cx + odx / od * 7.0f); wy = (int)(cy + ody / od * 7.0f);
            for (int s = 0; s < 3 && !cont_ok(wx, wy, hc); s++) {   // water or a cliff: farther on, else the objective
                wx = (int)(cx + odx / od * (10.0f + 4.0f * s)); wy = (int)(cy + ody / od * (10.0f + 4.0f * s));
            }
            if (!cont_ok(wx, wy, hc)) { wx = a->ox; wy = a->oy; }
        }
        a->wx = wx; a->wy = wy; a->wset = now;
    }
    for (int k = 0; k < a->nwave; k++) {
        uint8_t *u = a->wave[k];
        float dx = (float)FIELD(u, 0x1C, int) - cx, dy = (float)FIELD(u, 0x20, int) - cy;
        if (dx * dx + dy * dy >= 144.0f || as_fighting(u)) continue;
        if (!as_goal_near(u, (float)a->wx + 0.5f, (float)a->wy + 0.5f, 2.0f))
            a->orders += unit_goal(u, goal_move((float)a->wx + 0.5f, (float)a->wy + 0.5f), 0, 0x1F40);
    }
}

// assault P (test): the state of AI P's assault
static void cmd_assault(char *out, const char *args) {
    int p = atoi(args);
    if (p <= 0 || p >= 16) { REPLY("error: assault P\n"); return; }
    struct assault *a = &g_as[p];
    REPLY("P%d assault %s: phase %d, target P%d, rally %d,%d, objective %d,%d, waypoint %d,%d, wave %d of %d, subgroups %d; "
          "launches %u, spent %u, done %u, objectives %u, orders %u\n", p, g_assault ? "on" : "off", a->phase,
          a->target ? FIELD(a->target, PL_INDEX, int) : -1, a->rx, a->ry, a->ox, a->oy, a->wx, a->wy, a->nwave, a->launch_n,
          a->ntaken, a->launches, a->spent, a->done, a->objectives, a->orders);
}

static void assault_install(const char *aimod_ini, const char *autotest_ini) {
    g_assault = cfg_int(aimod_ini, autotest_ini, 1, "assault", 0);
    g_as_outmatched = cfg_int(aimod_ini, autotest_ini, 1, "assault_outmatched", 0);
    int h = patch_vtable(GA(VT_ASG_LAND), GA(SLOT_ASG_UPDATE), GA(FN_ASG_UPDATE), hook_asg_update);
    if (g_log) fprintf(g_log, "# assault: %s (subgroup update hook %s)\n", g_assault ? "on" : "off", h ? "ok" : "FAILED");
}

// a new game (the game start hook): the waves and subgroups led are the last game's
static void assault_new_game(void) { memset(g_as, 0, sizeof g_as); }
