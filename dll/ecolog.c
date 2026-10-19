// ECONOMY AND CITIZEN-LOSS LOG (autotest only; scripts/eco_timeline.py, docs/testing.md "Against the base-game AI"):
// the same lines for the mod's AIs and the vanilla players, so their economies can be laid side by side. Nothing
// here acts on the game - it only reads.
// "# eco: P<i> at <m> min: k=v ..." every game minute: citizens on the map by what they do (their goal +0x1EC:
// gather - by the resource of the site's list -, build/repair, walk, attack, load a transport, other, none), soldiers,
// population (0x556E11 with production) and cap (vf31), units in production (unit manager +0x13C), citizens killed
// so far (cdead) - at the town (cd_town: within 6 tiles of a drop site), near it (cd_near: 7-15), far out (cd_far) -
// and by what (cd_army soldiers, cd_bld towers / buildings, cd_ship ships, cd_air aircraft, cd_unk unknown), the
// dangerous sites the searches refused so far (refused, sites.c; mod only).
// "# death: P<v> citizen ..." for every citizen killed (both sides): where (tiles from the nearest drop site), what it
// was doing, the killer (player, type, class), the victim's own soldiers within 10 tiles and the killer side's
// soldiers within 8.
// The killer itself is not known at the kill statistics call (0x617780: vf28's attacker argument and its register are
// reused by then) - the line names the nearest unit of the killer's player within 12 tiles (soldier, tower, ship ...).
static struct eco_dead { unsigned n, town, near_, far_, army, bld, ship, air, unk; } g_eco_dead[16];
static uint32_t g_eco_t[16];

static void ecolog_new_game(void) { memset(g_eco_dead, 0, sizeof g_eco_dead); memset(g_eco_t, 0, sizeof g_eco_t); }

static const char *eco_goal(uint8_t *u, int *res) {   // what a citizen does; res: the gather site's type id (or -1)
    uint8_t *g = FIELD(u, 0x1EC, uint8_t *);
    *res = -1;
    if (!g) return "none";
    if (*(uintptr_t *)g == VT_GOAL_LOCATION) return "walk";
    if (*(uintptr_t *)g == VT_GOAL_ATTACK) return "attack";
    const char *n = rtti_name(g);
    if (!strcmp(n, "EEUGGather@@")) {
        uint8_t *s = FIELD(g, 0x38, uint8_t *), *td = s ? FIELD(s, 0x2C, uint8_t *) : NULL;
        if (td) *res = FIELD(td, 0x4B8, int);
        return "gather";
    }
    if (!strcmp(n, "EEUGBuildRepair@@")) return "build";
    if (!strcmp(n, "EEUGLoadTransport@@")) return "load";
    return "other";
}

static void ecolog_tick(void *pl, uint32_t now) {
    if (!g_auto.enabled || !g_log || !pl) return;
    unsigned idx = (unsigned)pidx(pl);
    if (now >= g_eco_t[idx] && now / 60000 == g_eco_t[idx] / 60000 && g_eco_t[idx]) return;
    g_eco_t[idx] = now ? now : 1;
    void *cm = FIELD(pl, 0xB90, void *), *ump = cm ? FIELD(cm, 0x44, void *) : NULL;
    uint8_t *rm = cm ? FIELD(cm, 0x38, uint8_t *) : NULL;
    int cit = 0, gat[7] = { 0 }, build = 0, walk = 0, att = 0, load = 0, other = 0, none = 0, idle0 = 0;
    for (int r = 0; rm && r < 7; r++) {   // the resource lists (deques): who gathers for which resource, list 0 idle
        uint8_t *list = rm + RM_LISTS + r * 0x14;
        for (unsigned k = 0, first = FIELD(list, 0xC, unsigned), n = FIELD(list, 0x10, unsigned); k < n && k < 1024; k++) {
            uint8_t *u = deque_at(list, first + k);
            if (!u || FIELD(u, 0x3C, int) <= 0) continue;
            int res;
            const char *g = eco_goal(u, &res);
            if (!strcmp(g, "gather")) gat[r]++;
            else if (r == 0 && !strcmp(g, "none")) idle0++;
        }
    }
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {   // every citizen on the map
        uint8_t *u = it.u, *td = it.td;
        if (FIELD(u, 0x18, void *) != pl || FIELD(u, 0x3C, int) <= 0 || !is_citizen_type(FIELD(td, 0x4B8, int))) continue;
        cit++;
        int res;
        const char *g = eco_goal(u, &res);
        if (!strcmp(g, "build")) build++;
        else if (!strcmp(g, "walk")) walk++;
        else if (!strcmp(g, "attack")) att++;
        else if (!strcmp(g, "load")) load++;
        else if (!strcmp(g, "other")) other++;
        else if (!strcmp(g, "none")) none++;
    }
    struct eco_dead *d = &g_eco_dead[idx];
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# eco: P%u at %u min: cit=%d gF=%d gW=%d gS=%d gG=%d gI=%d g0=%d build=%d walk=%d attack=%d load=%d other=%d none=%d "
            "idle0=%d soldiers=%d pop=%d cap=%d inprod=%d cdead=%u cd_town=%u cd_near=%u cd_far=%u cd_army=%u cd_bld=%u "
            "cd_ship=%u cd_air=%u cd_unk=%u refused=%u\n",
            idx, now / 60000, cit, gat[2], gat[3], gat[4], gat[5], gat[6], gat[0], build, walk, att, load, other, none, idle0,
            army_size(pl), ((int (__thiscall *)(void *, int, int))0x00556E11)(pl, 1, 0),
            ((int (__thiscall *)(void *, int))(*(void ***)pl)[31])(pl, 0), ump ? FIELD(ump, 0x13C, int) : -1,
            d->n, d->town, d->near_, d->far_, d->army, d->bld, d->ship, d->air, d->unk, is_vanilla(pl) ? 0u : g_site_refused[idx]);
    LeaveCriticalSection(&g_lock);
}

// a unit of v killed by k (diplo.c hook_kill_stat, n 3 = a unit): citizens only
static void eco_death(uint8_t *dead, int k, int v) {
    if (!g_auto.enabled || !g_log || !dead) return;
    uint8_t *td = FIELD(dead, 0x2C, uint8_t *);
    void *pl = FIELD(dead, 0x18, void *);
    if (!td || !pl || !is_citizen_type(FIELD(td, 0x4B8, int))) return;
    int x = FIELD(dead, 0x1C, int), y = FIELD(dead, 0x20, int), dd[2], dist = -1;
    if (nearest_drop(pl, x, y, 1, dd)) dist = (int)sqrtf((float)((dd[0] - x) * (dd[0] - x) + (dd[1] - y) * (dd[1] - y)));
    int res;
    const char *goal = eco_goal(dead, &res);
    int own = 0, foe = 0, best = 0x7FFFFFFF;
    uint8_t *att = NULL;   // the killer's nearest unit
    void *kp = player_at(k);
    for (struct grid_it it = grid_it_box(x, y, 12, 1); grid_it_next(&it); ) {
        uint8_t *u = it.u, *t = it.td;
        if (!t || FIELD(u, 0x3C, int) <= 0) continue;
        void *o = FIELD(u, 0x18, void *);
        int ux = FIELD(u, 0x1C, int) - x, uy = FIELD(u, 0x20, int) - y, d2 = ux * ux + uy * uy;
        int fam = FIELD(t, 0x4B4, int);
        int sc = d2 + (is_army_family(fam) ? 0 : fam == 0x1D ? 1000 : 2000);   // soldiers, ships, aircraft first, then towers
        if (o == kp && sc < best && d2 <= 144 && fam != 6 && fam != 22 && fam != 0 && fam != 1 && !is_citizen_type(FIELD(t, 0x4B8, int))) { best = sc; att = u; }
        if (!is_army_family(fam)) continue;
        if (o == pl && d2 <= 100) own++;
        else if (o == kp && d2 <= 64) foe++;
    }
    int atype = att ? FIELD(FIELD(att, 0x2C, uint8_t *), 0x4B8, int) : -1, acls = att ? FIELD(FIELD(att, 0x2C, uint8_t *), 0x4B4, int) : -1;
    struct eco_dead *d = &g_eco_dead[v & 15];
    d->n++;
    if (dist >= 0 && dist <= 6) d->town++; else if (dist >= 0 && dist <= 15) d->near_++; else d->far_++;
    // the killer's class (+0x4B4): 2 / 19 / 24 / 25 buildings and towers (as the kill statistics count them), ships
    // 4 / 10 / 18 / 21, aircraft 3 / 7 / 17
    if (!att) d->unk++;
    else if (acls == 2 || acls == 19 || acls == 24 || acls == 25 || acls == 29) d->bld++;
    else if (acls == 4 || acls == 10 || acls == 18 || acls == 21) d->ship++;
    else if (acls == 3 || acls == 7 || acls == 17) d->air++;
    else d->army++;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# death: P%d citizen at %.1f min at %d,%d, %d tiles from a drop site, %s (site type %d), by P%d (nearest: type %d class %d at %d tiles); "
            "own soldiers within 10: %d, the killer's within 8: %d\n", v, game_time() / 60000.0, x, y, dist, goal,
            res, k, atype, acls, att ? (int)sqrtf((float)(best % 1000)) : -1, own, foe);
    LeaveCriticalSection(&g_lock);
}
