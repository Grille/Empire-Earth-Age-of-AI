// FLEET ([all] navy_fleet, default 0; [autotest] v_navy_fleet for the variant players; docs/navy.md stage 3): the mod
// leads an AI's warships on a water map (navy_detect: enemies across the water) instead of the game's sea groups.
// Included by eeaimod.c after navy.c (navy_kind, nv_f, g_nv_map), landing.c (g_ld, ld_hits, ld_open_water, unit
// goals), intel.c (g_intel).
//
// The game's own AI has no fleet: sea attack subgroups (EECPASGSea) sail to a random unit or water area of a random
// enemy, the naval maintenance (0x4779FA) opens a Defend group of 2-3 ships - nothing guards the fishing boats, seeks
// the enemy fleet or keeps away from coastal towers and cannons (docs/navy.md 2). The playtester: on water maps losing
// sea control is a lost game; the whole fleet must keep out of the reach of coastal siege, cannons, artillery and
// towers (the x4-5 against battleships is an equalizer - frigates and galleys do badly against them too); fish is
// limited and the boats go further out over time; landings need the sea cleared and ships near.
// - Who: every warship and submarine of the AI (navy_kind NV_WAR / NV_SUB) afloat, not a landing's (landing_has_unit):
//   carriers, transports and fishing boats stay the game's. Led as landing.c leads its units: a sea subgroup holding
//   one of them is not updated (hook_asg_sea_update), the subgroups' orders to them are refused (hook_asg_give_goal,
//   vf57 - except the land subgroup's hand-off, which uses the goal after a refusal; never a fleet ship's caller).
//   The naval maintenance's unit deletion for room (0x430D40 from 0x477B17) is skipped while the fleet leads.
// - Danger (every 2 s): enemy towers and defence buildings, siege and land units that hit ships x3+ against
//   battleships or reach 7+ tiles - seen now, and with honest_vision those remembered (buildings until looked at
//   again, siege 5 minutes): their range + 2 tiles. No ship sits, sails or attacks inside; a ship inside sails out
//   (away from the nearest one), a target inside is dropped, a way that crosses one goes round (a waypoint off the
//   line) or not at all.
// - Strength: hit points x damage a second (attack x 1000 / (reload + 2000) - the time a shot takes in duels) x the
//   hit table's factor (nv_f) against the other side's ships, each side's damage spread over the other's hit points;
//   the ratio sqrt(own hp x own dps / (enemy hp x enemy dps)) - Lanchester's square law for the ships' fight.
// - Tasks, in this order:
//   escort: a landing is getting its transports (phase 1) and enemy warships are known on its sea route - the fleet
//     goes to the water off the beach (out of danger) and fights them, when it is not weaker (ratio >= 1); while the
//     landing sails or is ashore the ships it didn't take stay off its beach (out of danger) and fight ships there;
//   defend: enemy warships seen within 15 tiles of its buildings, fishing boats or guard point - attacked when the
//     ratio against them is >= 0.8 (at home: its towers and docks help), each ship the nearest it can hit;
//   hunt: the ratio against every enemy warship known (seen now, remembered 5 minutes) >= 1.3 and the fleet at 60%+
//     of its hit points - first gathered, then the enemy's warships, then its fishing boats and transports (seen now
//     or remembered 2 minutes), then the water off its nearest known dock (its fishing grounds); retreat (to the
//     guard point, 60 s at least) once the hit points fall below half of what it had when the hunt began, or the
//     enemy ships near the fleet are stronger (ratio < 0.8); its coast (seen within 4 tiles of the sea, 3 tiles
//     clear of danger) after the boats; test keys navy_fleet_raid (the ratio for boats, coast and fishing grounds,
//     130 - 100 was no better), navy_fleet_coast, navy_fleet_boats;
//   guard: the point between the fishing boats (or the docks) and the nearest known enemy building, on open water,
//     out of danger; ships within 6 tiles of it stay (they shoot at what comes in range).
//   Ships below 35% hit points go back to the dock (repair: logged as the hit points regained there).
// - Fishing boats: an enemy warship seen within 10 tiles of a boat and no fleet that wins there (ratio < 1 or the
//   fleet farther than 20 tiles) - the boat goes to its nearest dock (the drop site; the game gives it new work
//   there, as with the citizens sent home from danger).
// `# navy: P<n> fleet at N min:` every minute and on a change of task: task and why, where, ships and hit points,
// strength against the enemy ships known (and near), the danger zones, losses and kills, boats pulled back, orders.
static int g_navy_fleet = 0, g_v_navy_fleet = -1;
static int g_fl_boats = 1, g_fl_coast = 1, g_fl_raid = 130;   // navy_fleet_raid: the ratio (%) for raids on boats and the coast   // test: navy_fleet_boats (fishing boats pulled back), navy_fleet_coast (the hunt shells the coast)
#define FL_MAX 40
#define FL_DZ 384
#define FL_ES 160
enum { FT_GUARD, FT_DEFEND, FT_GATHER, FT_HUNT, FT_RETREAT, FT_ESCORT, FT_SUPPORT, FT_N };
static const char *const k_ft[FT_N] = { "guard", "defend", "gather", "hunt", "retreat", "escort", "support" };
static struct fleet {
    uint8_t *u[FL_MAX]; int n;
    uint32_t last, log_t, task_t, retreat_t;
    int task, last_task; const char *why;
    int gx, gy, dx, dy, ocean;           // the guard point, where the fleet goes now, its sea
    float hp0, ratio, near_ratio; int eknown, enear, ndz;
    uint8_t *htgt; int hx, hy;           // the hunt's target (or point)
    struct { int x, y; uint32_t until; } cleared[8]; int ci;
    int docked_hp[FL_MAX]; uint8_t *docked[FL_MAX]; int ndocked; unsigned repaired;   // ships sent to the dock: their hp then
    unsigned orders, flees, pulls, hunts, retreats, defends, escorts, refused, maint_skipped, sg_skipped;
} g_fl[16];
static unsigned g_fl_refused;

static int fleet_on(unsigned idx) {
    idx &= 15;
    return PV(idx, g_v_navy_fleet, g_navy_fleet) && g_nv_map[idx].done && g_nv_map[idx].strength > 0;
}
static int fleet_has(void *pl, uint8_t *u) {
    if (!pl || !u) return 0;
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    if (!fleet_on(idx)) return 0;
    struct fleet *f = &g_fl[idx];
    for (int k = 0; k < f->n; k++) if (f->u[k] == u) return 1;
    return 0;
}
// a sea subgroup holding one of the fleet's ships: its update skipped (landing.c hook_asg_sea_update)
static int fleet_leads(uint8_t *sg) {
    void *pl = FIELD(sg, 0x190, void *);
    if (!pl || !g_fl[(unsigned)FIELD(pl, PL_INDEX, int) & 15].n) return 0;
    for (uint8_t **r = FIELD(sg, 0x54, uint8_t **); r && r < FIELD(sg, 0x58, uint8_t **); r++)
        if (*r && fleet_has(pl, FIELD(*r, 0x44, uint8_t *))) { g_fl[(unsigned)FIELD(pl, PL_INDEX, int) & 15].sg_skipped++; return 1; }
    return 0;
}
// a subgroup's order (vf57) to one of the fleet's ships (landing.c hook_asg_give_goal refuses it)
static int fleet_refuses(uint8_t *sg, uint8_t *u) {
    void *pl = FIELD(sg, 0x190, void *);
    if (!fleet_has(pl, u)) return 0;
    g_fl_refused++; g_fl[(unsigned)FIELD(pl, PL_INDEX, int) & 15].refused++;
    return 1;
}
// the naval maintenance (0x4779FA) short of its "Ships to Maintain": it deletes own units for the room (0x430D40,
// thiscall(player, request, 0)) and opens a Defend group - the deletion is skipped while the fleet leads the ships
#define CALL_NV_MAINT_DELETE 0x00477B17
#define FN_DELETE_FOR_ROOM 0x00430D40
static int __fastcall hook_nv_maint_delete(void *pl, void *edx, void *req, int flag) {
    (void)edx;
    if (pl && fleet_on((unsigned)FIELD(pl, PL_INDEX, int))) { g_fl[(unsigned)FIELD(pl, PL_INDEX, int) & 15].maint_skipped++; return 0; }
    return ((int (__thiscall *)(void *, void *, int))FN_DELETE_FOR_ROOM)(pl, req, flag);
}

// THE SCAN (every 2 s per AI with the fleet): own warships, fishing boats, docks and buildings; enemy ships; danger
struct fl_ship { uint8_t *u, *td; int x, y, hp, pi, seen; float dps; };
static struct fl_scan {
    void *pl; unsigned idx;
    struct fl_ship es[FL_ES]; int ne;                 // enemy warships and submarines (seen now, then remembered)
    struct fl_ship ev[64]; int nev;                   // enemy fishing boats and transports (prey)
    struct fl_ship ec[96]; int nec;                   // enemy units and buildings on the coast of the fleet's sea (seen now)
    struct { int x, y; float r; } dz[FL_DZ];          // danger zones (radius incl. the margin)
    uint8_t *boat[96]; int nb;                        // own fishing boats
    int bx[256], by[256], nbl;                        // own buildings
    int dkx[16], dky[16], nd;                         // own docks
    int ebx, eby, eb; int edx[16], edy[16], ned;      // the nearest known enemy building (to the guard base), enemy docks
} g_fs;
static float fl_dps(uint8_t *u, uint8_t *td) {
    uint8_t *st = u ? FIELD(u, 0x34, uint8_t *) : NULL;
    float a = st ? FIELD(st, 0xD8, float) : (float)FIELD(td, 0x510, int);
    int rl = FIELD(td, 0x50C, int);
    return a > 0 ? a * 1000.0f / (float)((rl > 0 ? rl : 2000) + 2000) : 0.0f;
}
static float fl_range(uint8_t *u, uint8_t *td) {
    uint8_t *st = u ? FIELD(u, 0x34, uint8_t *) : NULL;
    float r = st ? FIELD(st, 0x9C, float) : FIELD(td, 0x420, float) + 1.0f;   // remembered: the base range + an upgrade
    return r > 0 && r < 40 ? r : 0;
}
// does a land unit or building of type td threaten ships: a defence with a ground attack (attack.c defence_stats), or
// a unit whose weapon hits battleships x3+ (catapults, cannons, artillery ...) or reaches 7+ tiles, and hits ships
static float fl_danger_r(uint8_t *u, uint8_t *td) {
    int fam = FIELD(td, 0x4B4, int), t = FIELD(td, 0x4B8, int), kind = FIELD(td, 0x4C8, int);
    if (fam == 4 || fam == 10 || fam == 18 || fam == 21 || fam == 22 || fam == 3 || fam == 7 || fam == 17) return 0;   // afloat, aircraft
    if (FIELD(td, 0x510, int) <= 0 || (nv_f(kind, NV_C_FRIG) <= 0 && nv_f(kind, NV_C_LINE) <= 0)) return 0;
    if (fam == 2 || fam == 29) {
        float r, a;
        if (u) return defence_stats(u, &r, &a) ? r : 0;
        return t == 145 || t == 147 || FIELD(td, 0x420, float) <= 1.0f ? 0 : FIELD(td, 0x420, float) + 1.0f;
    }
    float r = fl_range(u, td);
    return (nv_f(kind, NV_C_LINE) >= 3.0f || r >= 7.0f) && r >= 2.0f ? r : 0;
}
static void fl_dz_add(struct fl_scan *s, int x, int y, float r) {
    struct fleet *f = &g_fl[s->idx];   // the count: the fleet's ndz (its log)
    if (r <= 0 || x < 0) return;
    if (f->ndz >= FL_DZ) return;
    for (int k = 0; k < f->ndz; k++) if (s->dz[k].x == x && s->dz[k].y == y && s->dz[k].r >= r + 2.0f) return;
    s->dz[f->ndz].x = x; s->dz[f->ndz].y = y; s->dz[f->ndz++].r = r + 2.0f;
}
static int fl_in_danger(int x, int y) {
    struct fleet *f = &g_fl[g_fs.idx];
    for (int k = 0; k < f->ndz; k++) {
        float dx = (float)(x - g_fs.dz[k].x), dy = (float)(y - g_fs.dz[k].y);
        if (dx * dx + dy * dy <= g_fs.dz[k].r * g_fs.dz[k].r) return k + 1;
    }
    return 0;
}
static int fl_in_danger_m(int x, int y, float m) {   // within a zone + m tiles
    struct fleet *f = &g_fl[g_fs.idx];
    for (int k = 0; k < f->ndz; k++) {
        float dx = (float)(x - g_fs.dz[k].x), dy = (float)(y - g_fs.dz[k].y), r = g_fs.dz[k].r + m;
        if (dx * dx + dy * dy <= r * r) return 1;
    }
    return 0;
}
static int fl_way_safe(int x0, int y0, int x1, int y1) {   // the straight way, every 2 tiles (the start left out)
    float dx = (float)(x1 - x0), dy = (float)(y1 - y0), len = sqrtf(dx * dx + dy * dy);
    int steps = (int)(len / 2.0f) + 1;
    for (int i = 1; i <= steps; i++) {
        float t = (float)i / (float)steps;
        if (fl_in_danger(x0 + (int)(dx * t), y0 + (int)(dy * t))) return 0;
    }
    return 1;
}
static int fl_water(int x, int y, unsigned oc, int *ox, int *oy) {   // open water of the sea oc near (x, y), else any of it
    if (ld_open_water(x, y, oc, ox, oy)) return 1;
    for (int d = 0; d <= 8; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d || !d ? 1 : 2 * d))
                if (area_id(x + dx, y + dy, 1) == oc) { *ox = x + dx; *oy = y + dy; return 1; }
    return 0;
}
// a safe water point on the way from (fx, fy) to (tx, ty), as near to the target as it gets (2-tile steps back)
static int fl_safe_toward(int fx, int fy, int tx, int ty, unsigned oc, int *ox, int *oy) {
    float dx = (float)(fx - tx), dy = (float)(fy - ty), len = sqrtf(dx * dx + dy * dy);
    int steps = (int)(len / 2.0f) + 1;
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / (float)steps;
        int x = tx + (int)(dx * t), y = ty + (int)(dy * t), wx, wy;
        if (area_id(x, y, 1) != oc) continue;
        if (!fl_in_danger(x, y) && fl_water(x, y, oc, &wx, &wy) && !fl_in_danger(wx, wy)) { *ox = wx; *oy = wy; return 1; }
    }
    return 0;
}
static int fl_coast(int x, int y, int oc) {   // land with the sea oc within 4 tiles (12 looks)
    static const signed char d[12][2] = { { 2, 0 }, { -2, 0 }, { 0, 2 }, { 0, -2 }, { 4, 0 }, { -4, 0 }, { 0, 4 }, { 0, -4 }, { 3, 3 }, { -3, 3 }, { 3, -3 }, { -3, -3 } };
    if (area_id(x, y, 1)) return 0;
    for (int k = 0; k < 12; k++) if ((int)area_id(x + d[k][0], y + d[k][1], 1) == oc) return 1;
    return 0;
}
static void fl_scan(void *pl, unsigned idx) {
    struct fl_scan *s = &g_fs;
    struct fleet *f = &g_fl[idx];
    s->pl = pl; s->idx = idx;
    s->ne = s->nev = s->nec = s->nb = s->nbl = s->nd = s->ned = 0; s->eb = 0;
    f->n = 0; f->ndz = 0;
    void *cm = FIELD(pl, 0xB90, void *);
    uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
    int hx = bm ? FIELD(bm, 0x12C, int) : 0, hy = bm ? FIELD(bm, 0x130, int) : 0, bd = 0x7FFFFFFF;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        uint8_t *u = it.u, *td = it.td, *o = FIELD(u, 0x18, uint8_t *);
        int x = FIELD(u, 0x1C, int), y = FIELD(u, 0x20, int), fam = FIELD(td, 0x4B4, int);
        if (!o || FIELD(u, 0x3C, int) <= 0 || x < 0) continue;
        int k = navy_kind(td);
        if (o == pl) {
            if ((k == NV_WAR || k == NV_SUB) && f->n < FL_MAX && !landing_has_unit(pl, u)) f->u[f->n++] = u;
            else if (k == NV_FISH && s->nb < 96) s->boat[s->nb++] = u;
            else if (fam == 2 || fam == 29) {
                if (FIELD(td, 0x4B8, int) == 36 && s->nd < 16) { s->dkx[s->nd] = x; s->dky[s->nd++] = y; }
                if (s->nbl < 256) { s->bx[s->nbl] = x; s->by[s->nbl++] = y; }
            }
            continue;
        }
        if (FIELD(o, PL_INDEX, int) <= 0 || !is_enemy(pl, o) || !seen_by(u, pl)) continue;
        if (k == NV_WAR || k == NV_SUB) {
            if (s->ne < FL_ES) { struct fl_ship *e = &s->es[s->ne++]; e->u = u; e->td = td; e->x = x; e->y = y; e->hp = FIELD(u, 0x3C, int); e->pi = ld_pidx(u); e->seen = 1; e->dps = fl_dps(u, td); }
            continue;
        }
        if (k == NV_FISH || k == NV_TRANSPORT) {
            if (s->nev < 64) { struct fl_ship *e = &s->ev[s->nev++]; e->u = u; e->td = td; e->x = x; e->y = y; e->hp = FIELD(u, 0x3C, int); e->pi = ld_pidx(u); e->seen = 1; e->dps = 0; }
            continue;
        }
        if (f->ocean && s->nec < 96 && (fam == 2 || fam == 29 || is_soldier(td) || is_siege(FIELD(td, 0x4B8, int))) && !is_wall(FIELD(td, 0x4B8, int)) && fl_coast(x, y, f->ocean)) {
            struct fl_ship *e = &s->ec[s->nec++]; e->u = u; e->td = td; e->x = x; e->y = y; e->hp = FIELD(u, 0x3C, int); e->pi = ld_pidx(u); e->seen = 1; e->dps = 0;
        }
        if (fam == 2 || fam == 29) {
            int dx = x - hx, dy = y - hy;
            if (dx * dx + dy * dy < bd) { bd = dx * dx + dy * dy; s->ebx = x; s->eby = y; s->eb = 1; }
            if (FIELD(td, 0x4B8, int) == 36 && s->ned < 16) { s->edx[s->ned] = x; s->edy[s->ned++] = y; }
        }
        fl_dz_add(s, x, y, fl_danger_r(u, td));
    }
    if (!g_honest_vision) return;
    // remembered (intel.c): buildings until their spot is looked at again, siege and ships 5 minutes, prey 2
    struct intel *in = &g_intel[idx];
    for (int k = 0; k < in->n; k++) {
        struct intel_rec *e = &in->r[k];
        if (e->t == in->last || e->x < 0) continue;   // seen now: in the scan above
        uint8_t *td = type_data(e->type);
        if (!td) continue;
        uint32_t age = in->now - e->t;
        int nk = navy_kind(td);
        if (e->kind == 0) {
            int dx = e->x - hx, dy = e->y - hy;
            if (dx * dx + dy * dy < bd) { bd = dx * dx + dy * dy; s->ebx = e->x; s->eby = e->y; s->eb = 1; }
            if (e->type == 36 && s->ned < 16) { s->edx[s->ned] = e->x; s->edy[s->ned++] = e->y; }
            fl_dz_add(s, e->x, e->y, fl_danger_r(NULL, td));
        } else if (e->kind == 1 && age <= 300000) fl_dz_add(s, e->x, e->y, fl_danger_r(NULL, td));
        else if ((nk == NV_WAR || nk == NV_SUB) && age <= 300000 && s->ne < FL_ES) {
            struct fl_ship *q = &s->es[s->ne++];
            q->u = NULL; q->td = td; q->x = e->x; q->y = e->y; q->hp = e->hp; q->pi = e->owner; q->seen = 0; q->dps = fl_dps(NULL, td);
        } else if ((nk == NV_FISH || nk == NV_TRANSPORT) && age <= 120000 && s->nev < 64) {
            struct fl_ship *q = &s->ev[s->nev++];
            q->u = NULL; q->td = td; q->x = e->x; q->y = e->y; q->hp = e->hp; q->pi = e->owner; q->seen = 0; q->dps = 0;
        }
    }
}
// the strength ratio of own ships (all, or within r of (x, y)) against the enemy ships listed (sel: only those with
// mask bit set); 99 when the enemy has nothing that hurts them
static float fl_ratio(struct fleet *f, int x, int y, int r, const unsigned char *sel) {
    float ho = 0, he = 0, dofs = 0, de = 0;
    for (int j = 0; j < g_fs.ne; j++) if (!sel || sel[j]) he += (float)g_fs.es[j].hp;
    int any = 0;
    for (int i = 0; i < f->n; i++) {
        uint8_t *u = f->u[i];
        if (r > 0) { int dx = FIELD(u, 0x1C, int) - x, dy = FIELD(u, 0x20, int) - y; if (dx * dx + dy * dy > r * r) continue; }
        ho += (float)FIELD(u, 0x3C, int); any = 1;
    }
    if (he <= 0) return 99.0f;
    if (!any || ho <= 0) return 0.0f;
    for (int i = 0; i < f->n; i++) {
        uint8_t *u = f->u[i], *td = FIELD(u, 0x2C, uint8_t *);
        if (r > 0) { int dx = FIELD(u, 0x1C, int) - x, dy = FIELD(u, 0x20, int) - y; if (dx * dx + dy * dy > r * r) continue; }
        float fh = 0, hpu = (float)FIELD(u, 0x3C, int), dp = fl_dps(u, td);
        for (int j = 0; j < g_fs.ne; j++) {
            if (sel && !sel[j]) continue;
            struct fl_ship *e = &g_fs.es[j];
            fh += (float)e->hp * nv_f(FIELD(td, 0x4C8, int), FIELD(e->td, 0x4C4, int));
            de += e->dps * hpu * nv_f(FIELD(e->td, 0x4C8, int), FIELD(td, 0x4C4, int));
        }
        dofs += dp * fh / he;
    }
    de /= ho;
    if (de <= 0) return dofs > 0 ? 99.0f : 1.0f;
    if (dofs <= 0) return 0.0f;
    return sqrtf(ho * dofs / (he * de));
}
static void fl_center(struct fleet *f, int *cx, int *cy) {
    float x = 0, y = 0;
    for (int i = 0; i < f->n; i++) { x += FIELD(f->u[i], 0x1C, int); y += FIELD(f->u[i], 0x20, int); }
    *cx = f->n ? (int)(x / f->n) : f->gx; *cy = f->n ? (int)(y / f->n) : f->gy;
}
static float fl_hp(struct fleet *f) {
    float h = 0;
    for (int i = 0; i < f->n; i++) h += (float)FIELD(f->u[i], 0x3C, int);
    return h;
}
static float fl_hp_max(uint8_t *u) {
    uint8_t *st = FIELD(u, 0x34, uint8_t *);
    int m = st ? FIELD(st, 0x144, int) : 0;
    return (float)(m > 0 ? m : FIELD(FIELD(u, 0x2C, uint8_t *), 0x55C, int));
}
// the guard point: from the fishing boats' middle (the docks' with fewer than 2 boats) 5 tiles toward the nearest
// known enemy building, on open water of the fleet's sea, out of danger (else nearer to the boats / docks)
static void fl_guard_point(struct fleet *f) {
    float bx = 0, by = 0; int n = 0;
    for (int k = 0; k < g_fs.nb; k++) {
        uint8_t *b = g_fs.boat[k];
        if (area_id(FIELD(b, 0x1C, int), FIELD(b, 0x20, int), 1) == (unsigned)f->ocean) { bx += FIELD(b, 0x1C, int); by += FIELD(b, 0x20, int); n++; }
    }
    if (n < 2) {
        bx = by = 0; n = 0;
        for (int k = 0; k < g_fs.nd; k++) { unsigned oc = ocean_near(g_fs.dkx[k], g_fs.dky[k], 4, NULL, NULL); if ((int)oc == f->ocean) { bx += g_fs.dkx[k]; by += g_fs.dky[k]; n++; } }
    }
    if (!n) return;
    int px = (int)(bx / n), py = (int)(by / n), tx = px, ty = py, wx, wy;
    if (g_fs.eb) {
        float dx = (float)(g_fs.ebx - px), dy = (float)(g_fs.eby - py), len = sqrtf(dx * dx + dy * dy);
        if (len > 1) { tx = px + (int)(5.0f * dx / len); ty = py + (int)(5.0f * dy / len); }
    }
    if (fl_safe_toward(px, py, tx, ty, (unsigned)f->ocean, &wx, &wy)) { f->gx = wx; f->gy = wy; }
}
static int fl_ship_ocean(uint8_t *u) { return (int)area_id(FIELD(u, 0x1C, int), FIELD(u, 0x20, int), 1); }
static int fl_attacking(uint8_t *u) {   // its current attack target (alive), else NULL
    uint8_t *g = FIELD(u, 0x1EC, uint8_t *), *t = g && *(uintptr_t *)g == VT_GOAL_ATTACK ? FIELD(g, 0x30, uint8_t *) : NULL;
    return t && FIELD(t, 0x3C, int) > 0 && FIELD(t, 0x1C, int) >= 0;
}
static uint8_t *fl_target_of(uint8_t *u) {
    uint8_t *g = FIELD(u, 0x1EC, uint8_t *);
    return g && *(uintptr_t *)g == VT_GOAL_ATTACK ? FIELD(g, 0x30, uint8_t *) : NULL;
}
// to (x, y) - round a danger zone on the way through a waypoint off the line (8 or 16 tiles to a side), else not at all
static void fl_move(struct fleet *f, uint8_t *u, int x, int y, int k) {
    int ux = FIELD(u, 0x1C, int), uy = FIELD(u, 0x20, int), oc = fl_ship_ocean(u);
    if (!oc) oc = f->ocean;
    x += (k % 3) - 1; y += (k / 3 % 3) - 1;   // not all on one tile
    int dx = x - ux, dy = y - uy;
    if (dx * dx + dy * dy <= 9) return;
    if (!fl_way_safe(ux, uy, x, y)) {
        int ok = 0;
        float len = sqrtf((float)(dx * dx + dy * dy)), px = -(float)dy / len, py = (float)dx / len;
        static const int off[4] = { 8, -8, 16, -16 };
        for (int j = 0; j < 4 && !ok; j++) {
            int mx = (ux + x) / 2 + (int)(px * (float)off[j]), my = (uy + y) / 2 + (int)(py * (float)off[j]), wx, wy;
            if (!fl_water(mx, my, (unsigned)oc, &wx, &wy) || fl_in_danger(wx, wy) || !fl_way_safe(ux, uy, wx, wy)) continue;
            x = wx; y = wy; ok = 1;
        }
        if (!ok) return;
    }
    if (as_goal_near(u, (float)x + 0.5f, (float)y + 0.5f, 2.5f)) return;
    f->orders += unit_goal(u, goal_move((float)x + 0.5f, (float)y + 0.5f), 0, 0x1F40);
}
// out of a danger zone: away from its centre to its edge + 3 tiles (or 45/90 degrees off that), else the guard point
static int fl_flee(struct fleet *f, uint8_t *u) {
    int ux = FIELD(u, 0x1C, int), uy = FIELD(u, 0x20, int), z = fl_in_danger(ux, uy), oc = fl_ship_ocean(u);
    if (!z) return 0;
    z--;
    float dx = (float)(ux - g_fs.dz[z].x), dy = (float)(uy - g_fs.dz[z].y), len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f) { dx = (float)(f->gx - ux); dy = (float)(f->gy - uy); len = sqrtf(dx * dx + dy * dy); }
    if (len < 0.5f) { dx = 1; dy = 0; len = 1; }
    dx /= len; dy /= len;
    float go = g_fs.dz[z].r - len + 3.0f;
    if (go < 3) go = 3;
    static const float ang[5] = { 0, 0.785f, -0.785f, 1.571f, -1.571f };
    for (int j = 0; j < 5; j++) {
        float c = cosf(ang[j]), s = sinf(ang[j]), ex = dx * c - dy * s, ey = dx * s + dy * c;
        int tx = ux + (int)(ex * go), ty = uy + (int)(ey * go), wx, wy;
        if (!oc || !fl_water(tx, ty, (unsigned)oc, &wx, &wy) || fl_in_danger(wx, wy)) continue;
        if (!as_goal_near(u, (float)wx + 0.5f, (float)wy + 0.5f, 2.5f)) f->orders += unit_goal(u, goal_move((float)wx + 0.5f, (float)wy + 0.5f), 0, 0x1F40);
        f->flees++;
        return 1;
    }
    if (!as_goal_near(u, (float)f->gx + 0.5f, (float)f->gy + 0.5f, 2.5f)) f->orders += unit_goal(u, goal_move((float)f->gx + 0.5f, (float)f->gy + 0.5f), 0, 0x1F40);
    f->flees++;
    return 1;
}
// the nearest enemy ship seen now (in sel) that u can hit, out of danger, within r of (x, y)
static uint8_t *fl_pick(uint8_t *u, const unsigned char *sel, int x, int y, int r) {
    uint8_t *best = NULL;
    int ux = FIELD(u, 0x1C, int), uy = FIELD(u, 0x20, int), oc = fl_ship_ocean(u);
    float bd = 1e9f;
    for (int j = 0; j < g_fs.ne; j++) {
        struct fl_ship *e = &g_fs.es[j];
        if (!e->seen || (sel && !sel[j]) || fl_in_danger_m(e->x, e->y, 3.0f)) continue;
        int dx = e->x - x, dy = e->y - y;
        if (dx * dx + dy * dy > r * r || (oc && (int)area_id(e->x, e->y, 1) != oc) || !ld_hits(u, e->u)) continue;
        float d = (float)((e->x - ux) * (e->x - ux) + (e->y - uy) * (e->y - uy));
        if (d < bd) { bd = d; best = e->u; }
    }
    return best;
}
// each ship: out of danger first; a target inside a danger zone dropped; then the task's target, else to (x, y)
static void fl_orders(struct fleet *f, const unsigned char *sel, int x, int y, int r, int hunt_prey, int retreat) {
    for (int i = 0; i < f->n; i++) {
        uint8_t *u = f->u[i];
        if (fl_flee(f, u)) continue;
        uint8_t *cur = fl_target_of(u);
        if (retreat) { fl_move(f, u, x, y, i); continue; }   // off the fight
        if (fl_attacking(u) && !fl_in_danger(FIELD(cur, 0x1C, int), FIELD(cur, 0x20, int))) continue;   // keeps fighting
        if (FIELD(u, 0x3C, int) < 0.35f * fl_hp_max(u) && g_fs.nd) {   // to the dock (repair)
            int b = 0, bd = 0x7FFFFFFF;
            for (int k = 0; k < g_fs.nd; k++) { int dx = g_fs.dkx[k] - FIELD(u, 0x1C, int), dy = g_fs.dky[k] - FIELD(u, 0x20, int); if (dx * dx + dy * dy < bd) { bd = dx * dx + dy * dy; b = k; } }
            int j;
            for (j = 0; j < f->ndocked && f->docked[j] != u; j++) ;
            if (j == f->ndocked && j < FL_MAX) { f->docked[j] = u; f->docked_hp[j] = FIELD(u, 0x3C, int); f->ndocked++; }
            if (bd > 16) fl_move(f, u, g_fs.dkx[b], g_fs.dky[b], 0);
            continue;
        }
        uint8_t *t = fl_pick(u, sel, x, y, r);
        for (int pass = 0; !t && hunt_prey && pass < 2; pass++) {   // the hunt: prey seen now near the hunt point, then the coast
            float bd = 1e9f;
            struct fl_ship *list = pass ? g_fs.ec : g_fs.ev;
            for (int j = 0; j < (pass ? g_fs.nec : g_fs.nev); j++) {
                struct fl_ship *e = &list[j];
                if (!e->seen || fl_in_danger_m(e->x, e->y, 3.0f) || !ld_hits(u, e->u)) continue;
                float d = (float)((e->x - x) * (e->x - x) + (e->y - y) * (e->y - y));
                if (d <= (float)(r * r) && d < bd) { bd = d; t = e->u; }
            }
        }
        if (t) {
            if (cur != t) f->orders += unit_goal(u, goal_attack(u, t), 0, 0x1F40);
            continue;
        }
        if (fl_ship_ocean(u) == f->ocean || !fl_ship_ocean(u)) fl_move(f, u, x, y, i);
    }
}
// fishing boats near enemy warships the fleet doesn't win against: to the nearest dock
static void fl_boats(struct fleet *f, int cx, int cy) {
    static unsigned char sel[FL_ES];
    if (!g_fl_boats) return;
    for (int k = 0; k < g_fs.nb; k++) {
        uint8_t *b = g_fs.boat[k];
        int x = FIELD(b, 0x1C, int), y = FIELD(b, 0x20, int), nfoe = 0;
        memset(sel, 0, sizeof sel);
        for (int j = 0; j < g_fs.ne; j++) {
            struct fl_ship *e = &g_fs.es[j];
            if (!e->seen || (e->x - x) * (e->x - x) + (e->y - y) * (e->y - y) > 100 || !ld_hits(e->u, b)) continue;
            sel[j] = 1; nfoe++;
        }
        if (!nfoe) continue;
        int fd = (cx - x) * (cx - x) + (cy - y) * (cy - y);
        if (f->n && fd <= 400 && fl_ratio(f, 0, 0, 0, sel) >= 1.0f) continue;   // the fleet is there and wins
        int bd = 0x7FFFFFFF, bk = -1;
        for (int d = 0; d < g_fs.nd; d++) { int dx = g_fs.dkx[d] - x, dy = g_fs.dky[d] - y; if (dx * dx + dy * dy < bd) { bd = dx * dx + dy * dy; bk = d; } }
        if (bk < 0 || bd <= 16 || as_goal_near(b, (float)g_fs.dkx[bk] + 0.5f, (float)g_fs.dky[bk] + 0.5f, 3.0f)) continue;
        if (unit_goal(b, goal_move((float)g_fs.dkx[bk] + 0.5f, (float)g_fs.dky[bk] + 0.5f), 0, 0x1F40)) f->pulls++;
    }
}
static uint32_t fl_lost(unsigned idx, int sunk) {   // warships and submarines lost (sunk: of the enemy, by this AI)
    uint32_t n = 0;
    for (int t = 0; t < G_TYPES_N; t++) {
        unsigned v = sunk ? g_nv_sunk_type[idx][t] : g_nv_lost_type[idx][t];
        if (v && nv_role(type_data(t)) != NR_NONE) n += v;
    }
    return n;
}
static void fl_log(void *pl, unsigned idx, struct fleet *f, uint32_t now, int cx, int cy) {
    if (!g_log) return;
    float hp = fl_hp(f), hm = 0;
    for (int i = 0; i < f->n; i++) hm += fl_hp_max(f->u[i]);
    uint32_t blost = 0, bsunk = 0;
    uint8_t *cbm = FIELD(pl, 0xB8C, uint8_t *), *cmd = cbm ? FIELD(cbm, 0x48, uint8_t *) : NULL;
    unsigned ngroups = cmd ? FIELD(cmd + 4, 0x10, unsigned) : 0;   // the attack commander's groups (frozen ones pile up?)
    for (int j = 0; j < 16; j++) { blost += g_nv_boat_kills[j][idx]; bsunk += g_nv_boat_kills[idx][j]; }
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# navy: P%u fleet at %.1f min: %s (%s) | at %d,%d -> %d,%d guard %d,%d | ships %d hp %.0f (%.0f%%) | ratio %.2f vs %d known, near %.2f vs %d"
                   " | danger %d | lost %u sunk %u boats-lost %u boats-sunk %u | pulled %u flees %u hunts %u retreats %u defends %u escorts %u"
                   " | repaired %u | orders %u refused %u sea-updates-skipped %u maint-skipped %u | attack groups %u\n",
            idx, now / 60000.0, k_ft[f->task], f->why ? f->why : "-", cx, cy, f->dx, f->dy, f->gx, f->gy, f->n, hp, hm > 0 ? 100.0f * hp / hm : 0.0f,
            f->ratio > 98 ? 99.0f : f->ratio, f->eknown, f->near_ratio > 98 ? 99.0f : f->near_ratio, f->enear, f->ndz,
            fl_lost(idx, 0), fl_lost(idx, 1), blost, bsunk, f->pulls, f->flees, f->hunts, f->retreats, f->defends, f->escorts,
            f->repaired, f->orders, f->refused, f->sg_skipped, f->maint_skipped, ngroups);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}
static void fl_set(struct fleet *f, int task, const char *why, uint32_t now) {
    if (f->task != task) { f->task = task; f->task_t = now; }
    f->why = why;
}

static void fleet_tick(void *pl, uint32_t now) {
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    struct fleet *f = &g_fl[idx];
    if (now < f->last) memset(f, 0, sizeof *f);   // a new game (also fleet_new_game)
    if (!fleet_on(idx)) { f->n = 0; return; }
    if (f->last && now - f->last < 2000) return;
    f->last = now ? now : 1;
    fl_scan(pl, idx);
    // its sea: the docks' (the first one with a sea next to it), else the first ship's
    if (!f->ocean)
        for (int k = 0; k < g_fs.nd && !f->ocean; k++) f->ocean = (int)ocean_near(g_fs.dkx[k], g_fs.dky[k], 4, NULL, NULL);
    if (!f->ocean && f->n) f->ocean = fl_ship_ocean(f->u[0]);
    if (!f->ocean) return;
    fl_guard_point(f);
    if (!f->gx && !f->gy) return;
    // repaired at the dock: hit points regained by the ships sent there
    for (int j = 0; j < f->ndocked; j++) {
        uint8_t *u = f->docked[j];
        int k;
        for (k = 0; k < f->n && f->u[k] != u; k++) ;
        if (k == f->n) { f->docked[j] = f->docked[--f->ndocked]; f->docked_hp[j--] = f->docked_hp[f->ndocked]; continue; }
        int h = FIELD(u, 0x3C, int);
        if (h > f->docked_hp[j]) { f->repaired += (unsigned)(h - f->docked_hp[j]); f->docked_hp[j] = h; }
        if (h >= 0.8f * fl_hp_max(u)) { f->docked[j] = f->docked[--f->ndocked]; f->docked_hp[j--] = f->docked_hp[f->ndocked]; }
    }
    int cx, cy;
    fl_center(f, &cx, &cy);
    fl_boats(f, cx, cy);
    static unsigned char sel[FL_ES];
    f->eknown = g_fs.ne;
    f->ratio = fl_ratio(f, 0, 0, 0, NULL);
    memset(sel, 0, sizeof sel); f->enear = 0;
    for (int j = 0; j < g_fs.ne; j++) {
        struct fl_ship *e = &g_fs.es[j];
        if (e->seen && (e->x - cx) * (e->x - cx) + (e->y - cy) * (e->y - cy) <= 144) { sel[j] = 1; f->enear++; }
    }
    f->near_ratio = fl_ratio(f, cx, cy, 12, sel);
    float hp = fl_hp(f), hmax = 0;
    for (int i = 0; i < f->n; i++) hmax += fl_hp_max(f->u[i]);
    int task = FT_GUARD, prev = f->task;
    const char *why = "nothing to do";
    int tx = f->gx, ty = f->gy, r = 10, prey = 0;
    unsigned char *use = NULL;
    // threats at home: enemy warships seen within 15 tiles of its buildings, fishing boats or the guard point
    static unsigned char home[FL_ES];
    int nh = 0, hx = 0, hy = 0;
    memset(home, 0, sizeof home);
    for (int j = 0; j < g_fs.ne; j++) {
        struct fl_ship *e = &g_fs.es[j];
        if (!e->seen) continue;
        int in = (e->x - f->gx) * (e->x - f->gx) + (e->y - f->gy) * (e->y - f->gy) <= 225;
        for (int k = 0; k < g_fs.nb && !in; k++) { int dx = FIELD(g_fs.boat[k], 0x1C, int) - e->x, dy = FIELD(g_fs.boat[k], 0x20, int) - e->y; in = dx * dx + dy * dy <= 225; }
        for (int k = 0; k < g_fs.nbl && !in; k++) { int dx = g_fs.bx[k] - e->x, dy = g_fs.by[k] - e->y; in = dx * dx + dy * dy <= 225; }
        if (in) { home[j] = 1; nh++; hx += e->x; hy += e->y; }
    }
    struct landing *l = &g_ld[idx];
    if (!f->n) { task = FT_GUARD; why = "no warships"; }
    else if (g_landing && l->phase >= 2 && l->phase != 5 && l->ocean == f->ocean) {   // the landing's escorts are its own: the rest off its beach
        task = FT_SUPPORT; why = "landing under way";
        int wx, wy;
        if (fl_safe_toward(cx, cy, l->wx, l->wy, (unsigned)f->ocean, &wx, &wy)) { tx = wx; ty = wy; }
        r = 12;
    } else if (nh && fl_ratio(f, 0, 0, 0, home) >= 0.8f) {
        task = FT_DEFEND; why = "enemy ships at home"; use = home; tx = hx / nh; ty = hy / nh; r = 20;
        if (f->task != FT_DEFEND) f->defends++;
    } else if (nh) { task = FT_GUARD; why = "enemy ships at home - too strong"; }
    else if (g_landing && l->phase == 1 && l->ocean == f->ocean) {   // a landing getting its transports: its sea route cleared
        static unsigned char route[FL_ES];
        int nr = 0, mx = (f->gx + l->wx) / 2, my = (f->gy + l->wy) / 2;
        memset(route, 0, sizeof route);
        for (int j = 0; j < g_fs.ne; j++) {
            struct fl_ship *e = &g_fs.es[j];
            int a = (e->x - l->wx) * (e->x - l->wx) + (e->y - l->wy) * (e->y - l->wy), b = (e->x - mx) * (e->x - mx) + (e->y - my) * (e->y - my);
            if (a <= 225 || b <= 225) { route[j] = 1; nr++; }
        }
        if (nr && fl_ratio(f, 0, 0, 0, route) >= 1.0f) {
            int wx, wy;
            task = FT_ESCORT; why = "clearing the landing's sea"; use = route; r = 16;
            if (fl_safe_toward(f->gx, f->gy, l->wx, l->wy, (unsigned)f->ocean, &wx, &wy)) { tx = wx; ty = wy; }
            if (f->task != FT_ESCORT) f->escorts++;
        }
    }
    if (task == FT_GUARD && f->n && !nh) {
        // retreat: a hunt that went wrong - 60 s at the guard point at least
        if (f->task == FT_HUNT && (hp < 0.5f * f->hp0 || (f->enear && f->near_ratio < 0.8f))) {
            f->retreats++; f->retreat_t = now;
            fl_set(f, FT_RETREAT, hp < 0.5f * f->hp0 ? "half the hit points lost" : "outmatched near", now);
        }
        if (f->task == FT_RETREAT && now - f->retreat_t < 60000) { task = FT_RETREAT; why = f->why; }
        else if (f->ratio >= g_fl_raid / 100.0f && hp >= 0.6f * hmax) {
            // the hunt's target: the enemy's warships (nearest to the fleet; out of danger, reachable on its sea) when
            // clearly stronger (1.3), else - not weaker (navy_fleet_raid, 1.0) - its fishing boats and transports, its
            // coast, the water off its nearest known dock
            int best = -1, bd = 0x7FFFFFFF, prey_i = -1, pd = 0x7FFFFFFF;
            for (int j = 0; j < g_fs.ne && f->ratio >= 1.3f; j++) {
                struct fl_ship *e = &g_fs.es[j];
                int d = (e->x - cx) * (e->x - cx) + (e->y - cy) * (e->y - cy);
                if (d < bd && (int)area_id(e->x, e->y, 1) == f->ocean && !fl_in_danger_m(e->x, e->y, 3.0f)) { bd = d; best = j; }
            }
            for (int j = 0; j < g_fs.nev && best < 0; j++) {
                struct fl_ship *e = &g_fs.ev[j];
                int d = (e->x - cx) * (e->x - cx) + (e->y - cy) * (e->y - cy);
                if (d < pd && (int)area_id(e->x, e->y, 1) == f->ocean && !fl_in_danger_m(e->x, e->y, 3.0f)) { pd = d; prey_i = j; }
            }
            int hx2 = -1, hy2 = -1, coast_i = -1, cd = 0x7FFFFFFF;
            for (int j = 0; j < g_fs.nec && best < 0 && prey_i < 0 && g_fl_coast; j++) {   // the coast out of danger (+3 tiles)
                struct fl_ship *e = &g_fs.ec[j];
                int d = (e->x - cx) * (e->x - cx) + (e->y - cy) * (e->y - cy);
                if (d < cd && !fl_in_danger_m(e->x, e->y, 3.0f)) { cd = d; coast_i = j; }
            }
            if (best >= 0) { hx2 = g_fs.es[best].x; hy2 = g_fs.es[best].y; why = "the enemy fleet"; }
            else if (prey_i >= 0) { hx2 = g_fs.ev[prey_i].x; hy2 = g_fs.ev[prey_i].y; why = "fishing boats, transports"; }
            else if (coast_i >= 0 && fl_safe_toward(cx, cy, g_fs.ec[coast_i].x, g_fs.ec[coast_i].y, (unsigned)f->ocean, &hx2, &hy2)) why = "the enemy's coast";
            else {
                for (int k = 0; k < g_fs.ned; k++) {   // the enemy's fishing grounds: off its nearest dock, not cleared lately
                    int wx, wy, skip = 0, d = (g_fs.edx[k] - cx) * (g_fs.edx[k] - cx) + (g_fs.edy[k] - cy) * (g_fs.edy[k] - cy);
                    if (d >= bd) continue;
                    for (int c = 0; c < 8; c++) skip |= now < f->cleared[c].until && abs(f->cleared[c].x - g_fs.edx[k]) < 6 && abs(f->cleared[c].y - g_fs.edy[k]) < 6;
                    if (skip || !fl_safe_toward(cx, cy, g_fs.edx[k], g_fs.edy[k], (unsigned)f->ocean, &wx, &wy)) continue;
                    bd = d; hx2 = wx; hy2 = wy; f->hx = g_fs.edx[k]; f->hy = g_fs.edy[k];
                }
                if (hx2 >= 0) why = "the enemy's fishing grounds";
            }
            if (hx2 < 0 && (f->task == FT_HUNT || f->task == FT_GATHER) && now - f->task_t < 30000) { hx2 = f->dx; hy2 = f->dy; why = f->task == FT_HUNT ? f->why : "the last target"; }
            if (hx2 >= 0) {
                // gathered first: every ship within 8 tiles of the middle (60 s at most)
                int spread = 0;
                for (int i = 0; i < f->n; i++) { int dx = FIELD(f->u[i], 0x1C, int) - cx, dy = FIELD(f->u[i], 0x20, int) - cy; spread |= dx * dx + dy * dy > 64; }
                if (f->task != FT_HUNT && spread && !(f->task == FT_GATHER && now - f->task_t >= 60000)) { task = FT_GATHER; why = "before the hunt"; tx = cx; ty = cy; if (!fl_safe_toward(f->gx, f->gy, cx, cy, (unsigned)f->ocean, &tx, &ty)) { tx = f->gx; ty = f->gy; } }
                else {
                    task = FT_HUNT; tx = hx2; ty = hy2; r = 14; prey = 1;
                    if (f->task != FT_HUNT) { f->hunts++; f->hp0 = hp; }
                    if (!strcmp(why, "the enemy's fishing grounds") && abs(cx - hx2) < 5 && abs(cy - hy2) < 5) {   // there, nothing found
                        f->cleared[f->ci].x = f->hx; f->cleared[f->ci].y = f->hy; f->cleared[f->ci].until = now + 180000; f->ci = (f->ci + 1) & 7;
                    }
                }
            }
        }
    }
    if (task == FT_RETREAT) { tx = f->gx; ty = f->gy; }
    int changed = task != prev;
    fl_set(f, task, why, now);
    f->dx = tx; f->dy = ty;
    fl_orders(f, use, tx, ty, r, prey, task == FT_RETREAT);
    if (changed || now - f->log_t >= 60000) { f->log_t = now; fl_log(pl, idx, f, now, cx, cy); }
}

static void fleet_new_game(void) { memset(g_fl, 0, sizeof g_fl); }
static void fleet_install(const char *aimod_ini, const char *autotest_ini) {
    g_navy_fleet = cfg_int(aimod_ini, autotest_ini, 1, "navy_fleet", 0);
    if (autotest_ini) g_v_navy_fleet = GetPrivateProfileIntA("autotest", "v_navy_fleet", -1, autotest_ini);
    g_fl_boats = cfg_int(aimod_ini, autotest_ini, 1, "navy_fleet_boats", 1);
    g_fl_coast = cfg_int(aimod_ini, autotest_ini, 1, "navy_fleet_coast", 1);
    g_fl_raid = cfg_int(aimod_ini, autotest_ini, 1, "navy_fleet_raid", 130);
    int ok = 0;
    if (g_navy_fleet || g_v_navy_fleet > 0) {
        ok = asg_order_hooks() + patch_call(CALL_NV_MAINT_DELETE, FN_DELETE_FOR_ROOM, (void *)hook_nv_maint_delete);
    }
    if (g_log) fprintf(g_log, "# navy: fleet %s (v_navy_fleet %d; hooks %d)\n", g_navy_fleet ? "on" : "off", g_v_navy_fleet, ok);
}
