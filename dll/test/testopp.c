// SEA OPPONENT (tests only; docs/testing.md "Sea opponent", docs/navy.md stage 3 measurement): the idle human slot P1
// plays a navy the way the playtester does on an island map, so an AI's navy can be measured against what a human
// does - not only against another AI's (AIs are weak at sea; a 1-3 ship AI fleet holds nothing). Started by the setup
// line "seaop start <ai> [level]" (scenarios/seaop.eeai), off otherwise; nothing here runs in normal play.
// - War: P1 and the AI are made enemies at once (the test maps ally the idle human with every AI) - at 2 s, before the
//   AI's navy_detect (10 s) looks for enemies across the water.
// - Build-up (spawned - P1 has no economy): warships at its docks on a schedule like a human's, a few early, most by
//   minute 20, losses replaced at the docks' pace up to an hour's budget; level ([autotest] seaop_level 1 weak, 2
//   medium, 3 strong): first ships at minute 8 / 5 / 3, 2 / 3 / 4 of them, 8 / 16 / 22 by minute 20, at most 12 / 22 /
//   30, 24 / 44 / 60 ships an hour; seaop_start=<min> moves the first ships. Types: the newest of each role P1 or the
//   AI could build now (the tech record's own test, landing.c ld_type_avail - the opponent keeps the AI's epoch):
//   frigates 40%, battleships 30%, galleys / catapult ships 30% (attack subs once there are no galleys).
// - Three groups (a new ship joins by its role: battleships and subs the main fleet, galleys the raiders, frigates the
//   group furthest below its share 35/40/25%); every 2 s:
//   * raid: the AI's fishing boats, then its transports, then citizens, docks and buildings within 3 tiles of the
//     sea, nearest first within 16 tiles; nothing seen: the fishing grounds where its boats were seen (5 min, then 2
//     min skipped once looked at), its docks' water, then a ring of 8 points off its coast;
//   * main: defends (AI warships seen within 15 tiles of P1's buildings or within 10 of the other groups, ratio 0.8),
//     escorts a landing to its beach, hunts the AI's warships when the ratio against every one known is 1.2+ and it
//     has 60% of its hit points (the nearest known, seen now or remembered 3 min), else shells the AI's docks when
//     no AI warship is known, else holds the middle of the sea lane;
//   * patrol: the sea lane (3 points between P1's docks and the AI's coast): the AI's transports seen within 25
//     tiles (the landings), fishing boats within 12, warships when the ratio is 1+;
//   * every group retreats home for 45 s when the AI's warships seen within 14 tiles are stronger (ratio < 0.8, the
//     patrol 1.0) or it is down to 45% of the hit points it set out with.
//   Strength as fleet.c's: the ratio sqrt(own hp x own dps / (their hp x their dps)), dps = attack x 1000 / (reload
//   + 2 s) x the game's hit factor (ld_factor) spread over the other side's hit points.
// - Danger zones: the AI's defence buildings seen (remembered until their spot is seen empty) and siege seen now:
//   range + 2 tiles; no target and no station inside one.
// - Honest information: P1's eyes are its own units and buildings, 9 tiles - an AI unit is known only when seen,
//   warships remembered 3 minutes, buildings until their spot is looked at again (the test's P1 itself sees the whole
//   map). Shortcut: the AI's home (its first Capitol / Town Center) is known from the start, as a human scouts it in
//   the first minutes.
// - Landings ([autotest] seaop_landings, default 1): waves at minute 35 (weak), 25 + 45 (medium), 20 + 32 + 44
//   (strong) with 2 / 3 / 4 transports full of soldiers of the AI's own two most common land types (spawned at P1's
//   dock, loaded with the game's load goal), to a beach 10-25 tiles from the AI's home, out of danger, the main fleet
//   escorting; unloaded with the game's unload goal, then they attack (nearest soldier within 12, citizen within 15,
//   else the nearest building). A wave is ashore when soldiers stood on the AI's land, stopped when its transports
//   sank first.
// - Home: P1's island keeps a guard of the AI's soldier types (6 / 10 / 14, 2 more every 30 s) and 2 towers of the
//   AI's tower type, which attack the AI's land units on P1's island - a human defends, and an idle base would end
//   the test at the first landing.
// - Log, every minute: "# sea: P1 vs P<ai> at <m> min (<level>): key=value ..." (scripts/seacontrol.py), and lines
//   for the waves. Counts from diplo.c's kills by pair (g_kills, g_nv_boat_kills, g_nv_lost_type: with one AI every
//   loss of the AI's at sea is P1's doing); the AI's citizens killed while P1 has no soldier on the AI's land and no
//   AI land unit is on P1's island count as killed by ships.
// Included by eeaimod.c after testcmd.c (cmd_spawn); ticked from charge_tick (every AI update).

#define SO_N 128        // P1's warships tracked
#define SO_MEM 96       // the AI's warships and buildings remembered
#define SO_SIGHT 9      // tiles P1's units see
#define SO_FG 8         // fishing grounds remembered
#define SO_NZ 128       // danger zones
#define SO_MARGIN 2.0f  // a target this far clear of a danger zone (the ship shooting it stands nearer to the zone)
enum { SO_RAID, SO_MAIN, SO_PATROL, SO_G };
enum { SR_F, SR_L, SR_A, SR_S, SR_N };   // frigate, battleship (the line), galley / catapult ship, attack sub
internal const char *const k_so_lvl[4] = { "?", "weak", "medium", "strong" };

struct so_ship { ee_unit *u; int id, grp, role, type, x, y; };
struct so_mem { int id, type, x, y, kind; float hp, dps, r; uint32_t t; };   // kind 0 warship, 1 defence, 2 dock, 3 other building
struct so_grp { const char *why; uint32_t until; float hp0; int wp, n, cx, cy; float hp, hpmax; };
struct so_wave { int st, ntr, tr_id[6], w0, w1, aboard, landed, bx, by, wx, wy, min, hot; ee_unit *tr[6]; uint32_t t, load_t; };   // w0..w1: its soldiers in wsol_id
struct so_eu { ee_unit *u; ee_unit_type *td; int x, y, cls, seen; };   // the AI's units this tick; cls below
enum { SC_WAR, SC_TR, SC_BOAT, SC_CIT, SC_DOCK, SC_BLD, SC_DEF, SC_SOLD, SC_SIEGE, SC_OTHER };

internal struct {
    int on, ai, level, land, trace;
    uint32_t start_ms, last, log_t, spawn_t[2], guard_t, types_t, held_t;
    int hx, hy, ex, ey, towers;
    unsigned hcont, econt, ocean;
    int nd, dx[2], dy[2], dwx[2], dwy[2];   // P1's docks: a land tile at the shore, open water beside it
    int kx, ky, kwx, kwy;                   // where the waves embark: a free beach 6+ tiles from the docks (a dock blocked it)
    int awx, awy, lane[3][2], nring, ring[8][2];
    int types[SR_N], sold[2], tower, trtype;
    struct so_ship s[SO_N]; int ns;
    struct so_mem m[SO_MEM]; int nm;
    int fg[SO_FG][2]; uint32_t fg_t[SO_FG], fg_look[SO_FG];
    struct so_grp g[SO_G];
    struct so_wave w[3]; int nw, wave_min[3];
    int wsol_id[256], nwsol, boarding;     // soldiers of the waves (by unit id): not the guard; a wave loading
    // counters
    int spawned, spawned_r[SR_N], lost, lost_r[SR_N], tr_spawned, tr_lost, sol_spawned, guard_spawned;
    int sailed, ashore, stopped, landed;
    int ai_war_most, civ_ships;
    uint32_t civ_last, ai_held_ms, opp_held_ms, total_ms;
    unsigned orders, flees, detours, retreats;
} g_so;

// this tick's scan
internal struct {
    int ne, ex[1024], ey[1024];                   // P1's eyes
    int nps; ee_unit *ps[SO_N]; int psid[SO_N];   // P1's warships
    int npt; ee_unit *pt[32];                     // P1's transports
    int npl; ee_unit *pl[400];                    // P1's land soldiers
    int npb, pbx[256], pby[256];                  // P1's buildings
    int nu; struct so_eu u[2048];                 // the AI's units
    int nz, zx[SO_NZ], zy[SO_NZ]; float zr[SO_NZ];   // danger zones (radius incl. the margin)
    int ai_war, ai_tr, ai_boats, ai_isl, ai_towns, ai_on_opp, opp_off_home;
} g_sos;

internal int so_is_water(int x, int y, unsigned ocean) {
    unsigned o = area_id(x, y, 1);
    return o && (!ocean || o == ocean);
}
internal int so_open(int x, int y, unsigned ocean, int rad) {   // water all around within rad tiles
    for (int ax = -rad; ax <= rad; ax++)
        for (int ay = -rad; ay <= rad; ay++)
            if (!so_is_water(x + ax, y + ay, ocean)) return 0;
    return 1;
}
internal int so_near_water(int x, int y, int r) {
    for (int ax = -r; ax <= r; ax++)
        for (int ay = -r; ay <= r; ay++)
            if (area_id(x + ax, y + ay, 1)) return 1;
    return 0;
}
internal int so_find_open(int x, int y, int maxd, unsigned ocean, int rad, int *ox, int *oy) {   // nearest such water
    for (int d = 0; d <= maxd; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d || !d ? 1 : 2 * d)) {
                if (so_open(x + dx, y + dy, ocean, rad)) { *ox = x + dx; *oy = y + dy; return 1; }
                if (!d) break;
            }
    return 0;
}
internal float so_d2(int x0, int y0, int x1, int y1) { return (float)((x1 - x0) * (x1 - x0) + (y1 - y0) * (y1 - y0)); }
internal int so_in_danger(int x, int y, float margin) {
    for (int k = 0; k < g_sos.nz; k++)
        if (so_d2(x, y, g_sos.zx[k], g_sos.zy[k]) < (g_sos.zr[k] + margin) * (g_sos.zr[k] + margin)) return 1;
    return 0;
}
internal int so_family(ee_unit_type *td) { return td ? type_get_family(td) : -1; }
internal int so_warship(ee_unit_type *td) {   // warships and submarines with a weapon (no transports, fishing boats, carriers)
    int fam = so_family(td), t = td ? type_get_id(td) : 0;
    return (fam == 4 || fam == 10) && type_get_attack(td) > 0 && !is_transport(t) && !is_fishing_boat(t);
}
internal int so_role(ee_unit_type *td) {
    int k = type_get_weapon(td);
    return k == 1 ? SR_F : k == 34 ? SR_L : (k == 3 || k == 6) ? SR_A : k == 18 ? SR_S : -1;
}
internal float so_dps(ee_unit *u, ee_unit_type *td) {
    uint8_t *st = u ? unit_get_stats(u) : NULL;
    float a = st ? FIELD(st, OF(USTAT, 0xD8), float) : (float)type_get_attack(td);
    int rl = type_get_reload(td);
    return a > 0 ? a * 1000.0f / (float)((rl > 0 ? rl : 2000) + 2000) : 0.0f;
}
internal float so_hpmax(ee_unit *u) {
    uint8_t *st = unit_get_stats(u);
    int m = st ? FIELD(st, OF(USTAT, 0x144), int) : 0;
    return (float)(m > 0 ? m : type_get_hp(unit_get_type_data(u)));
}
// the strength ratio of side a against side b (types, hit points, dps; player indices): sqrt(hpA dA / (hpB dB))
struct so_side { int n; ee_unit_type *td[SO_MEM + SO_N]; float hp[SO_MEM + SO_N], dps[SO_MEM + SO_N]; int pi; };
internal float so_eff(struct so_side *a, struct so_side *b) {   // a's damage a second at b, by the hit table
    float hb = 0, d = 0;
    for (int j = 0; j < b->n; j++) hb += b->hp[j];
    if (hb <= 0) return 0;
    for (int i = 0; i < a->n; i++) {
        float f = 0;
        for (int j = 0; j < b->n; j++) f += b->hp[j] * ld_factor(a->td[i], a->pi, b->td[j], b->pi);
        d += a->dps[i] * f / hb;
    }
    return d;
}
internal float so_ratio(struct so_side *a, struct so_side *b) {
    if (!b->n) return 99.0f;
    if (!a->n) return 0.0f;
    float ha = 0, hb = 0;
    for (int i = 0; i < a->n; i++) ha += a->hp[i];
    for (int j = 0; j < b->n; j++) hb += b->hp[j];
    float da = so_eff(a, b), db = so_eff(b, a);
    if (db <= 0) return 99.0f;
    float r = sqrtf(ha * da / (hb * db));
    return r > 99.0f ? 99.0f : r;
}
internal void so_side_add(struct so_side *s, ee_unit_type *td, float hp, float dps) {
    if (s->n >= SO_MEM + SO_N || !td) return;
    s->td[s->n] = td; s->hp[s->n] = hp; s->dps[s->n] = dps; s->n++;
}

internal PRINTF_LIKE(1, 2) void so_log(const char *fmt, ...) {
    if (!g_log) return;
    va_list ap;
    va_start(ap, fmt);
    EnterCriticalSection(&g_lock);
    vfprintf(g_log, fmt, ap);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
    va_end(ap);
}
internal void so_spawn(int type, int x, int y, int n) {
    char tmp[512], a[64];
    sprintf(a, "1 %d %d %d %d", type, x, y, n);
    tmp[0] = 0;
    cmd_spawn(tmp, a);
}
internal int so_home_of(int p, int *x, int *y) {   // a player's Capitol, else Town Center, else any building
    int got = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_player *o = unit_get_owner(it.u);
        if (!o || player_get_index(o) != p || unit_get_hp(it.u) <= 0 || so_family(it.td) != 2) continue;
        int t = type_get_id(it.td), pr = t == UT(B_CAPITOL) ? 3 : t == UT(B_TOWN_CENTER) ? 2 : 1;
        if (pr > got) { got = pr; *x = unit_get_x(it.u); *y = unit_get_y(it.u); }
    }
    return got;
}
// the newest type of each role P1 or the AI could build now; the transport; the AI's two most common land soldiers
// and its tower (seen anywhere - P1 builds the same epoch's)
internal void so_types(void) {
    void *p1 = game_players()[1], *ai = game_players()[g_so.ai];
    int best[SR_N] = { 0 };
    for (int r = 0; r < SR_N; r++) g_so.types[r] = 0;
    for (int t = 1; t < 724; t++) {
        ee_unit_type *td = type_data(t);
        if (!td || type_get_id(td) != t || !so_warship(td) || t == UT(X09_AGINCOURT_BATTLESHIP) || t == UT(X07_GOLDEN_HIND)) continue;
        int r = so_role(td), tech = type_get_tech(td);
        if (r < 0 || tech < 2000 || tech <= best[r]) continue;
        if (!(p1 && ld_type_avail(p1, t)) && !(ai && ld_type_avail(ai, t))) continue;
        best[r] = tech; g_so.types[r] = t;
    }
    g_so.trtype = 0;
    for (int t = 0, bt = 0; t < G_TYPES_N; t++)   // the newest transport (as ld_transport_type)
        if (is_transport(t) && ((p1 && ld_type_avail(p1, t)) || (ai && ld_type_avail(ai, t))) && type_get_tech(type_data(t)) > bt) {
            bt = type_get_tech(type_data(t));
            g_so.trtype = t;
        }
    static int cnt[G_TYPES_N];
    memset(cnt, 0, sizeof cnt);
    int tower = 0;
    float tr = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_player *o = unit_get_owner(it.u);
        if (!o || player_get_index(o) != g_so.ai || unit_get_hp(it.u) <= 0) continue;
        int t = type_get_id(it.td), fam = so_family(it.td);
        float r, a;
        if (is_soldier(it.td) && fam != 4 && fam != 10 && fam != 18 && fam != 21 && fam != 22 && fam != 3 && fam != 7 && fam != 17 && t > 0 && t < G_TYPES_N) cnt[t]++;
        else if (defence_stats(it.u, &r, &a) && !UT_IN(t, UT_TOWNS, UT_B_DOCK, UT_B_NAVY_YARD, UT_B_FORTRESS) && r * a > tr) { tr = r * a; tower = t; }
    }
    int s0 = 0, s1 = 0;
    for (int t = 1; t < G_TYPES_N; t++) {
        if (cnt[t] > cnt[s0]) { s1 = s0; s0 = t; }
        else if (cnt[t] > cnt[s1]) s1 = t;
    }
    if (s0 && cnt[s0]) { g_so.sold[0] = s0; g_so.sold[1] = s1 && cnt[s1] ? s1 : s0; }
    if (tower) g_so.tower = tower;
}

internal void so_scan(uint32_t now) {
    static struct grid_it it;
    memset(&g_sos, 0, sizeof g_sos);
    for (it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        ee_player *o = unit_get_owner(u);
        if (!o || unit_get_hp(u) <= 0) continue;
        int p = player_get_index(o), x = unit_get_x(u), y = unit_get_y(u), fam = so_family(td), t = type_get_id(td);
        if (x < 0) continue;   // aboard
        if (p == 1) {
            if (g_sos.ne < 1024) { g_sos.ex[g_sos.ne] = x; g_sos.ey[g_sos.ne++] = y; }
            if (so_warship(td)) { if (g_sos.nps < SO_N) { g_sos.psid[g_sos.nps] = unit_get_serial(u); g_sos.ps[g_sos.nps++] = u; } }
            else if (fam == 18 || is_transport(t)) { if (g_sos.npt < 32) g_sos.pt[g_sos.npt++] = u; }
            else if (fam == 2 || fam == 29) { if (g_sos.npb < 256) { g_sos.pbx[g_sos.npb] = x; g_sos.pby[g_sos.npb++] = y; } }
            else if (is_soldier(td) && fam != 3 && fam != 7 && fam != 17 && g_sos.npl < 400) {
                g_sos.pl[g_sos.npl++] = u;
                if (cont_at(x, y) != g_so.hcont) g_sos.opp_off_home++;
            }
        } else if (p == g_so.ai && g_sos.nu < 2048) {
            struct so_eu *e = &g_sos.u[g_sos.nu++];
            float r, a;
            e->u = u; e->td = td; e->x = x; e->y = y; e->seen = 0;
            e->cls = so_warship(td) ? SC_WAR : (fam == 18 || is_transport(t)) ? SC_TR : (fam == 22 || is_fishing_boat(t)) ? SC_BOAT :
                     is_citizen_type(t) ? SC_CIT : (t == UT(B_DOCK) || t == UT(B_NAVY_YARD)) ? SC_DOCK : defence_stats(u, &r, &a) ? SC_DEF :
                     (fam == 2 || fam == 29) ? SC_BLD : is_siege(t) ? SC_SIEGE : is_soldier(td) ? SC_SOLD : SC_OTHER;
            if (e->cls == SC_WAR) g_sos.ai_war++;
            else if (e->cls == SC_TR) g_sos.ai_tr++;
            else if (e->cls == SC_BOAT) g_sos.ai_boats++;
            if (fam == 2 || fam == 29) {
                unsigned c = cont_at(x, y);
                if (c && c != g_so.econt && t != UT(B_DOCK) && t != UT(B_NAVY_YARD)) {
                    g_sos.ai_isl++;
                    if (UT_IN(t, UT_TOWNS, UT_X_SINGLE_PLAYER_SETTLEMENT)) g_sos.ai_towns++;
                }
            }
            if (fam != 4 && fam != 10 && fam != 18 && fam != 21 && fam != 22 && cont_at(x, y) == g_so.hcont) g_sos.ai_on_opp++;
        }
    }
    // what P1 sees: within SO_SIGHT of any of its units or buildings
    for (int i = 0; i < g_sos.nu; i++) {
        struct so_eu *e = &g_sos.u[i];
        for (int k = 0; k < g_sos.ne && !e->seen; k++)
            if (abs(g_sos.ex[k] - e->x) <= SO_SIGHT && abs(g_sos.ey[k] - e->y) <= SO_SIGHT &&
                so_d2(e->x, e->y, g_sos.ex[k], g_sos.ey[k]) <= SO_SIGHT * SO_SIGHT) e->seen = 1;
    }
    // memory: buildings until their spot is looked at and they are gone, warships 3 minutes at the last place seen
    for (int k = 0; k < g_so.nm; k++) {
        struct so_mem *m = &g_so.m[k];
        int alive = 0, looked = 0;
        for (int i = 0; i < g_sos.nu && !alive; i++)
            if (g_sos.u[i].seen && unit_get_serial(g_sos.u[i].u) == m->id) alive = 1;
        if (!alive)
            for (int e = 0; e < g_sos.ne && !looked; e++) looked = so_d2(m->x, m->y, g_sos.ex[e], g_sos.ey[e]) <= (SO_SIGHT - 2) * (SO_SIGHT - 2);
        if (!alive && (looked || (m->kind == 0 && now - m->t > 180000))) { g_so.m[k--] = g_so.m[--g_so.nm]; }
    }
    for (int i = 0; i < g_sos.nu; i++) {
        struct so_eu *e = &g_sos.u[i];
        if (!e->seen) continue;
        int kind = e->cls == SC_WAR ? 0 : e->cls == SC_DEF ? 1 : e->cls == SC_DOCK ? 2 : -1;
        if (kind < 0) continue;
        int id = unit_get_serial(e->u), k = 0;
        while (k < g_so.nm && g_so.m[k].id != id) k++;
        if (k == g_so.nm) { if (g_so.nm >= SO_MEM) continue; g_so.nm++; }
        struct so_mem *m = &g_so.m[k];
        float r = 0, a;
        if (kind == 1) defence_stats(e->u, &r, &a);
        m->id = id; m->type = type_get_id(e->td); m->x = e->x; m->y = e->y; m->kind = kind; m->t = now;
        m->hp = (float)unit_get_hp(e->u); m->dps = so_dps(e->u, e->td); m->r = r;
    }
    // fishing grounds: where the AI's boats were seen
    for (int i = 0; i < g_sos.nu; i++) {
        struct so_eu *e = &g_sos.u[i];
        if (!e->seen || e->cls != SC_BOAT) continue;
        int k = 0, old = 0;
        while (k < SO_FG && !(g_so.fg_t[k] && so_d2(e->x, e->y, g_so.fg[k][0], g_so.fg[k][1]) < 36.0f)) k++;
        if (k == SO_FG) {
            for (k = 0; k < SO_FG && g_so.fg_t[k]; k++) if (g_so.fg_t[k] < g_so.fg_t[old]) old = k;
            if (k == SO_FG) k = old;
            g_so.fg[k][0] = e->x; g_so.fg[k][1] = e->y; g_so.fg_look[k] = 0;
        }
        g_so.fg_t[k] = now;
    }
    // danger zones: remembered defences (range + 2), siege seen now that reaches 6+ tiles (range + 2), ranged
    // soldiers seen now that hit a frigate and reach 4+ tiles (range + 1: the archers on the shore sank most of the
    // first version's raiders); one within 2 tiles of a zone at least as big is left out
    for (int k = 0; k < g_so.nm && g_sos.nz < SO_NZ; k++)
        if (g_so.m[k].kind == 1) { g_sos.zx[g_sos.nz] = g_so.m[k].x; g_sos.zy[g_sos.nz] = g_so.m[k].y; g_sos.zr[g_sos.nz++] = g_so.m[k].r + 2.0f; }
    ee_unit_type *ftd = type_data(g_so.types[SR_F]);
    for (int i = 0; i < g_sos.nu && g_sos.nz < SO_NZ; i++) {
        struct so_eu *e = &g_sos.u[i];
        if (!e->seen || (e->cls != SC_SIEGE && e->cls != SC_SOLD)) continue;
        float r = ld_range(e->u), zr = e->cls == SC_SIEGE ? r + 2.0f : r + 1.0f;
        if (r < (e->cls == SC_SIEGE ? 6.0f : 4.0f) || r > 30.0f) continue;
        if (e->cls == SC_SOLD && (!ftd || ld_factor(e->td, g_so.ai, ftd, 1) <= 0.0f)) continue;
        int dup = 0;
        for (int k = 0; k < g_sos.nz && !dup; k++) dup = g_sos.zr[k] >= zr && so_d2(e->x, e->y, g_sos.zx[k], g_sos.zy[k]) <= 4.0f;
        if (!dup) { g_sos.zx[g_sos.nz] = e->x; g_sos.zy[g_sos.nz] = e->y; g_sos.zr[g_sos.nz++] = zr; }
    }
}

// P1's warships: new ones join a group, gone ones are lost
internal void so_ships(void) {
    for (int k = 0; k < g_so.ns; k++) {
        int alive = 0;
        struct so_ship *s = &g_so.s[k];
        for (int i = 0; i < g_sos.nps && !alive; i++) alive = g_sos.ps[i] == s->u && g_sos.psid[i] == s->id;
        if (alive) { s->x = unit_get_x(s->u); s->y = unit_get_y(s->u); continue; }
        if (g_so.trace) {   // where it sank and what of the AI's was seen near
            char line[512];
            int n = sprintf(line, "# sea: lost T%d (%s) at %d,%d at %.2f min%s, near:", s->type, s->grp == SO_RAID ? "raid" : s->grp == SO_MAIN ? "main" : "patrol",
                            s->x, s->y, (float)g_so.last / 60000.0f, so_in_danger(s->x, s->y, 0.0f) ? " in danger" : "");
            for (int i = 0; i < g_sos.nu && n < 440; i++) {
                struct so_eu *e = &g_sos.u[i];
                if (so_d2(s->x, s->y, e->x, e->y) <= 144.0f && e->cls != SC_CIT && e->cls != SC_OTHER)
                    n += sprintf(line + n, " T%d/c%d%s@%d,%d", type_get_id(e->td), e->cls, e->seen ? "" : "?", e->x, e->y);
            }
            so_log("%s\n", line);
        }
        g_so.lost++; g_so.lost_r[s->role]++; g_so.s[k--] = g_so.s[--g_so.ns];
    }
    for (int i = 0; i < g_sos.nps; i++) {
        int k = 0;
        while (k < g_so.ns && !(g_so.s[k].u == g_sos.ps[i] && g_so.s[k].id == g_sos.psid[i])) k++;
        if (k < g_so.ns || g_so.ns >= SO_N) continue;
        int role = so_role(unit_get_type_data(g_sos.ps[i])), n[SO_G] = { 0 }, grp;
        if (role < 0) role = SR_F;
        for (int j = 0; j < g_so.ns; j++) n[g_so.s[j].grp]++;
        static const float share[SO_G] = { 0.35f, 0.40f, 0.25f };
        if (role == SR_L || role == SR_S) grp = SO_MAIN;
        else if (role == SR_A) grp = SO_RAID;
        else {
            float bd = -1e9f;
            grp = SO_RAID;
            for (int gq = 0; gq < SO_G; gq++) {
                float d = share[gq] * (float)(g_so.ns + 1) - (float)n[gq];
                if (d > bd) { bd = d; grp = gq; }
            }
        }
        g_so.s[g_so.ns].u = g_sos.ps[i]; g_so.s[g_so.ns].id = g_sos.psid[i]; g_so.s[g_so.ns].grp = grp; g_so.s[g_so.ns].role = role;
        g_so.s[g_so.ns].type = type_get_id(unit_get_type_data(g_sos.ps[i])); g_so.s[g_so.ns].x = unit_get_x(g_sos.ps[i]); g_so.s[g_so.ns].y = unit_get_y(g_sos.ps[i]);
        g_so.ns++;
    }
}

// the schedule: warships wanted at this minute
internal int so_want(float min) {
    static const int st[4] = { 0, 8, 5, 3 }, n0[4] = { 0, 2, 3, 4 }, n20[4] = { 0, 8, 16, 22 }, cap[4] = { 0, 12, 22, 30 };
    int l = g_so.level;
    float s = g_so.start_ms ? (float)g_so.start_ms / 60000.0f : (float)st[l];
    if (min < s) return 0;
    float w = min <= 20.0f ? (float)n0[l] + (float)(n20[l] - n0[l]) * (min - s) / (20.0f - s > 1.0f ? 20.0f - s : 1.0f)
                           : (float)n20[l] + (float)(cap[l] - n20[l]) * (min - 20.0f) / 20.0f;
    if (w > (float)cap[l]) w = (float)cap[l];
    return (int)w;
}
internal void so_spawn_tick(uint32_t now) {
    static const int budget[4] = { 0, 24, 44, 60 };
    int want = so_want((float)now / 60000.0f);
    for (int d = 0; d < g_so.nd; d++) {
        if (g_so.ns >= want || g_so.spawned >= budget[g_so.level] * (int)(1 + now / 3600000u)) return;
        if (now - g_so.spawn_t[d] < 30000 && g_so.spawn_t[d]) continue;   // a dock trains a ship in 30 s
        // the role furthest below its share (frigates 40%, battleships 30%, galleys or subs 30%)
        int have[SR_N] = { 0 }, role = -1;
        float bd = -1e9f;
        static const float share[SR_N] = { 0.4f, 0.3f, 0.3f, 0.3f };
        for (int k = 0; k < g_so.ns; k++) have[g_so.s[k].role]++;
        for (int r = 0; r < SR_N; r++) {
            if (!g_so.types[r] || (r == SR_S && g_so.types[SR_A])) continue;
            float def = share[r] * (float)(g_so.ns + 1) - (float)have[r];
            if (def > bd) { bd = def; role = r; }
        }
        if (role < 0) return;
        so_spawn(g_so.types[role], g_so.dwx[d], g_so.dwy[d], 1);
        g_so.spawn_t[d] = now ? now : 1;
        g_so.spawned++; g_so.spawned_r[role]++;
    }
}

internal int so_cur_target(ee_unit *u, ee_unit **t) {   // its attack goal's target (alive)
    ee_goal *g = unit_get_goal(u);
    *t = g && goal_is_attack(g) ? goal_get_target(g) : NULL;
    return *t && unit_get_hp(*t) > 0;
}
internal void so_attack(ee_unit *u, ee_unit *t) {
    ee_unit *cur;
    if (so_cur_target(u, &cur) && cur == t) return;
    g_so.orders += (unsigned)unit_goal(u, goal_attack(u, t), 0, 0x1F40);
}
// the first danger zone the way from (x0, y0) to (x1, y1) passes through (-1: none); one the start lies in doesn't count
internal int so_seg_zone(int x0, int y0, int x1, int y1) {
    float dx = (float)(x1 - x0), dy = (float)(y1 - y0), l2 = dx * dx + dy * dy;
    for (int k = 0; k < g_sos.nz; k++) {
        float r = g_sos.zr[k], px = (float)(g_sos.zx[k] - x0), py = (float)(g_sos.zy[k] - y0);
        if (px * px + py * py < r * r) continue;
        float t = l2 > 0 ? (px * dx + py * dy) / l2 : 0;
        t = t < 0 ? 0 : t > 1 ? 1 : t;
        float qx = px - t * dx, qy = py - t * dy;
        if (qx * qx + qy * qy < r * r) return k;
    }
    return -1;
}
internal void so_go(ee_unit *u, int tx, int ty) {
    if (so_d2(unit_get_x(u), unit_get_y(u), tx, ty) <= 4.0f || as_goal_near(u, (float)tx + 0.5f, (float)ty + 0.5f, 2.5f)) return;
    g_so.orders += (unsigned)unit_goal(u, goal_move((float)tx + 0.5f, (float)ty + 0.5f), 0, 0x1F40);
}
// a ship inside a danger zone sails out, away from its centre (to r + 3 tiles, on water; else 45 degrees off)
internal int so_flee(ee_unit *u) {
    int x = unit_get_x(u), y = unit_get_y(u);
    for (int k = 0; k < g_sos.nz; k++) {
        float r = g_sos.zr[k], dx = (float)(x - g_sos.zx[k]), dy = (float)(y - g_sos.zy[k]), d = sqrtf(dx * dx + dy * dy);
        if (d >= r + 0.5f) continue;
        if (d < 0.5f) dx = 1, dy = 0, d = 1;
        for (int a = 0; a < 5; a++) {   // straight away, then 45 / -45 / 90 / -90 degrees off
            static const float ang[5] = { 0.0f, 0.785f, -0.785f, 1.571f, -1.571f };
            float c = cosf(ang[a]), s = sinf(ang[a]), ux = (dx * c - dy * s) / d, uy = (dx * s + dy * c) / d;
            int fx = g_sos.zx[k] + (int)(ux * (r + 3.0f)), fy = g_sos.zy[k] + (int)(uy * (r + 3.0f));
            if (so_is_water(fx, fy, 0)) { so_go(u, fx, fy); g_so.flees++; return 1; }
        }
        return 0;
    }
    return 0;
}
internal void so_move(ee_unit *u, int x, int y, int i) {   // spread around x, y on water, round the danger zones
    static const int off[9][2] = { { 0, 0 }, { 2, 0 }, { 0, 2 }, { -2, 0 }, { 0, -2 }, { 2, 2 }, { -2, 2 }, { -2, -2 }, { 2, -2 } };
    int tx = x + off[i % 9][0], ty = y + off[i % 9][1], ux = unit_get_x(u), uy = unit_get_y(u);
    if (!so_is_water(tx, ty, 0)) tx = x, ty = y;
    if (so_in_danger(tx, ty, 0.0f)) return;
    int k = so_seg_zone(ux, uy, tx, ty);
    if (k >= 0) {   // round it: a waypoint off the zone's centre, square to the way, on the ship's side first
        float dx = (float)(tx - ux), dy = (float)(ty - uy), l = sqrtf(dx * dx + dy * dy), r = g_sos.zr[k] + 4.0f;
        if (l < 1) return;
        float nx = -dy / l, ny = dx / l, side = (float)(ux - g_sos.zx[k]) * nx + (float)(uy - g_sos.zy[k]) * ny;
        if (side < 0) nx = -nx, ny = -ny;
        for (int pass = 0; pass < 2; pass++, nx = -nx, ny = -ny) {
            int wx = g_sos.zx[k] + (int)(nx * r), wy = g_sos.zy[k] + (int)(ny * r);
            if (so_is_water(wx, wy, 0) && !so_in_danger(wx, wy, 0.0f) && so_seg_zone(ux, uy, wx, wy) < 0) { so_go(u, wx, wy); g_so.detours++; return; }
        }
        return;   // no way round: stay
    }
    so_go(u, tx, ty);
}
// the best target for ship u among the AI's units seen: by class weight + distance, within r of (cx, cy), out of
// danger, one it can hit; mask: the classes wanted (bit per SC_*), w: extra tiles per class
internal ee_unit *so_pick(ee_unit *u, unsigned mask, const int *w, int cx, int cy, int r) {
    ee_unit *best = NULL;
    float bs = 1e9f;
    int x = unit_get_x(u), y = unit_get_y(u);
    for (int i = 0; i < g_sos.nu; i++) {
        struct so_eu *e = &g_sos.u[i];
        if (!e->seen || !(mask & (1u << e->cls)) || so_d2(e->x, e->y, cx, cy) > (float)(r * r)) continue;
        if ((e->cls == SC_CIT || e->cls == SC_DOCK || e->cls == SC_BLD) && !so_near_water(e->x, e->y, 3)) continue;
        if (so_in_danger(e->x, e->y, SO_MARGIN) || so_seg_zone(x, y, e->x, e->y) >= 0 || !ld_hits(u, e->u)) continue;
        float s = sqrtf(so_d2(x, y, e->x, e->y)) + (w ? (float)w[e->cls] : 0.0f);
        if (s < bs) { bs = s; best = e->u; }
    }
    return best;
}
internal void so_side_ships(struct so_side *s, int grp) {   // P1's ships of a group (-1: all)
    memset(s, 0, sizeof *s);
    s->pi = 1;
    for (int k = 0; k < g_so.ns; k++)
        if (grp < 0 || g_so.s[k].grp == grp) {
            ee_unit *u = g_so.s[k].u;
            ee_unit_type *td = unit_get_type_data(u);
            so_side_add(s, td, (float)unit_get_hp(u), so_dps(u, td));
        }
}
internal void so_side_seen(struct so_side *s, int cx, int cy, int r) {   // the AI's warships seen within r (r < 0: all known)
    memset(s, 0, sizeof *s);
    s->pi = g_so.ai;
    if (r >= 0) {
        for (int i = 0; i < g_sos.nu; i++) {
            struct so_eu *e = &g_sos.u[i];
            if (e->seen && e->cls == SC_WAR && so_d2(e->x, e->y, cx, cy) <= (float)(r * r))
                so_side_add(s, e->td, (float)unit_get_hp(e->u), so_dps(e->u, e->td));
        }
        return;
    }
    for (int k = 0; k < g_so.nm; k++)
        if (g_so.m[k].kind == 0) so_side_add(s, type_data(g_so.m[k].type), g_so.m[k].hp, g_so.m[k].dps);
}

internal void so_retreat(struct so_grp *g, const char *why, uint32_t now) { g->why = why; g->until = now + 45000; g_so.retreats++; }
internal void so_group(int gi, uint32_t now) {
    static struct so_side own, foe;
    struct so_grp *g = &g_so.g[gi];
    ee_unit *mem[SO_N];
    int n = 0;
    long sx = 0, sy = 0;
    g->hp = g->hpmax = 0;
    for (int k = 0; k < g_so.ns; k++) {
        if (g_so.s[k].grp != gi) continue;
        ee_unit *u = g_so.s[k].u;
        mem[n++] = u; sx += unit_get_x(u); sy += unit_get_y(u);
        g->hp += (float)unit_get_hp(u); g->hpmax += so_hpmax(u);
    }
    g->n = n;
    if (!n) { g->why = "empty"; return; }
    int cx = (int)(sx / n), cy = (int)(sy / n), home_x = g_so.dwx[0], home_y = g_so.dwy[0];
    g->cx = cx; g->cy = cy;
    if (now < g->until) {   // retreating
        for (int i = 0; i < n; i++) so_move(mem[i], home_x, home_y, i);
        return;
    }
    so_side_ships(&own, gi);
    so_side_seen(&foe, cx, cy, 14);
    float local = so_ratio(&own, &foe);
    if (g->hp > g->hp0 || so_d2(cx, cy, home_x, home_y) < 64.0f) g->hp0 = g->hp;
    if (foe.n && local < (gi == SO_PATROL ? 1.0f : 0.8f)) { so_retreat(g, "outmatched", now); return; }
    if (g->hp < 0.45f * g->hp0) { so_retreat(g, "damaged", now); g->hp0 = 0; return; }
    unsigned m = 0;
    int r = 16, tx = -1, ty = -1;
    static const int wr[SC_OTHER + 1] = { 0, 0, 0, 4, 6, 10, 99, 99, 99, 99 };   // raid: boats, transports, then the coast
    const int *w = NULL;
    if (gi == SO_RAID) {
        m = (1u << SC_WAR) * (foe.n > 0) | 1u << SC_BOAT | 1u << SC_TR | 1u << SC_CIT | 1u << SC_DOCK | 1u << SC_BLD;
        w = wr; g->why = "raid";
        // nothing near: a fishing ground (seen within 5 min, not looked at in the last 2), its docks, the ring
        float bd = 1e9f;
        for (int k = 0; k < SO_FG; k++) {
            if (!g_so.fg_t[k] || now - g_so.fg_t[k] > 300000 || (g_so.fg_look[k] && now - g_so.fg_look[k] < 120000)) continue;
            if (so_d2(cx, cy, g_so.fg[k][0], g_so.fg[k][1]) < 25.0f) { g_so.fg_look[k] = now; continue; }
            float d = so_d2(cx, cy, g_so.fg[k][0], g_so.fg[k][1]);
            if (d < bd && !so_in_danger(g_so.fg[k][0], g_so.fg[k][1], 1.0f)) { bd = d; tx = g_so.fg[k][0]; ty = g_so.fg[k][1]; g->why = "fishing grounds"; }
        }
        for (int k = 0; k < g_so.nm && tx < 0; k++) {
            int wx, wy;
            if (g_so.m[k].kind == 2 && so_find_open(g_so.m[k].x, g_so.m[k].y, 6, g_so.ocean, 1, &wx, &wy) && !so_in_danger(wx, wy, 1.0f)) {
                tx = wx; ty = wy; g->why = "docks";
            }
        }
        if (tx < 0 && g_so.nring) {
            int k = g->wp % g_so.nring;
            if (so_d2(cx, cy, g_so.ring[k][0], g_so.ring[k][1]) < 25.0f || so_in_danger(g_so.ring[k][0], g_so.ring[k][1], 1.0f)) g->wp++;
            k = g->wp % g_so.nring;
            tx = g_so.ring[k][0]; ty = g_so.ring[k][1]; g->why = "coast";
        }
    } else if (gi == SO_MAIN) {
        // defend: AI warships seen near P1's buildings or the other groups
        static struct so_side nearby;
        memset(&nearby, 0, sizeof nearby); nearby.pi = g_so.ai;
        int nx = -1, ny = -1;
        for (int i = 0; i < g_sos.nu; i++) {
            struct so_eu *e = &g_sos.u[i];
            if (!e->seen || e->cls != SC_WAR) continue;
            int close = 0;
            for (int b = 0; b < g_sos.npb && !close; b++) close = so_d2(e->x, e->y, g_sos.pbx[b], g_sos.pby[b]) <= 225.0f;
            for (int q = 0; q < SO_G && !close; q++) close = q != SO_MAIN && g_so.g[q].n && so_d2(e->x, e->y, g_so.g[q].cx, g_so.g[q].cy) <= 100.0f;
            if (close) { so_side_add(&nearby, e->td, (float)unit_get_hp(e->u), so_dps(e->u, e->td)); nx = e->x; ny = e->y; }
        }
        so_side_seen(&foe, 0, 0, -1);
        float all = so_ratio(&own, &foe);
        int wave = -1;
        for (int k = 0; k < g_so.nw; k++) if (g_so.w[k].st == 2 || g_so.w[k].st == 3) wave = k;
        m = 1u << SC_WAR | 1u << SC_TR;
        if (nearby.n && so_ratio(&own, &nearby) >= 0.8f) { tx = nx; ty = ny; g->why = "defend"; r = 18; }
        else if (wave >= 0) { tx = g_so.w[wave].wx; ty = g_so.w[wave].wy; g->why = "escort"; m |= 1u << SC_DEF; }
        else if (foe.n && all >= 1.2f && g->hp >= 0.6f * g->hpmax) {
            float bd = 1e9f;
            for (int k = 0; k < g_so.nm; k++) {
                float d = so_d2(cx, cy, g_so.m[k].x, g_so.m[k].y) + (now - g_so.m[k].t > 4000 ? 400.0f : 0.0f);
                if (g_so.m[k].kind == 0 && d < bd && !so_in_danger(g_so.m[k].x, g_so.m[k].y, 0.0f)) { bd = d; tx = g_so.m[k].x; ty = g_so.m[k].y; }
            }
            g->why = tx >= 0 ? "hunt" : "hunt (all in danger)";
        } else if (!foe.n && g->hp >= 0.6f * g->hpmax && n >= 3) {
            for (int k = 0; k < g_so.nm && tx < 0; k++) {
                int wx, wy;
                if (g_so.m[k].kind == 2 && !so_in_danger(g_so.m[k].x, g_so.m[k].y, 0.0f) &&
                    so_find_open(g_so.m[k].x, g_so.m[k].y, 6, g_so.ocean, 1, &wx, &wy)) { tx = wx; ty = wy; }
            }
            m |= 1u << SC_DOCK | 1u << SC_BOAT;
            g->why = tx >= 0 ? "shell docks" : "guard";
        } else g->why = "guard";
        if (tx < 0) { tx = g_so.lane[foe.n && all < 1.0f ? 0 : 1][0]; ty = g_so.lane[foe.n && all < 1.0f ? 0 : 1][1]; }
    } else {   // patrol: the sea lane; transports within 25, boats within 12
        m = 1u << SC_WAR | 1u << SC_TR | 1u << SC_BOAT;
        int k = g->wp % 3;
        if (so_d2(cx, cy, g_so.lane[k][0], g_so.lane[k][1]) < 25.0f) g->wp++;
        k = g->wp % 3;
        tx = g_so.lane[k][0]; ty = g_so.lane[k][1]; g->why = "lane";
        r = 25;
    }
    for (int i = 0; i < n; i++) {
        ee_unit *u = mem[i];
        ee_unit *t = NULL;
        ee_unit *cur;
        if (so_flee(u)) continue;
        if ((float)unit_get_hp(u) < 0.35f * so_hpmax(u)) { so_move(u, home_x, home_y, i); continue; }   // hurt: out of the fight
        if (gi == SO_PATROL) {
            static const int wp[SC_OTHER + 1] = { 4, 0, 6, 99, 99, 99, 99, 99, 99, 99 };
            t = so_pick(u, 1u << SC_TR, NULL, unit_get_x(u), unit_get_y(u), 25);
            if (!t) t = so_pick(u, 1u << SC_BOAT | (foe.n ? 1u << SC_WAR : 0), wp, unit_get_x(u), unit_get_y(u), 12);
            if (t) g->why = "intercept";
        } else t = so_pick(u, m, w, cx, cy, r);
        if (t) { so_attack(u, t); continue; }
        if (so_cur_target(u, &cur) && unit_get_owner(cur) == game_players()[g_so.ai]) {
            int x = unit_get_x(cur), y = unit_get_y(cur);
            if (x >= 0 && !so_in_danger(x, y, SO_MARGIN) && so_seg_zone(unit_get_x(u), unit_get_y(u), x, y) < 0) continue;   // its own choice
            if (tx >= 0) { g_so.orders += (unsigned)unit_goal(u, goal_move((float)tx + 0.5f, (float)ty + 0.5f), 0, 0x1F40); continue; }   // not that one
        }
        if (tx >= 0) so_move(u, tx, ty, i);
    }
}

// a beach for a wave: land of the AI's continent at the sea, 8-40 tiles from its home, out of danger (2 tiles clear),
// open water of P1's sea within 4 tiles; the nearest to the AI's home, a tile for every 3 of the way from the embarking
// point (Large Islands: no beach within 25 tiles of the AI's home). None out of danger (Small Islands e9: towers and
// riflemen along the whole coast): the one under the fewest zones, 10 tiles a zone - a human lands anyway.
internal int so_beach(struct so_wave *w) {
    float bd = 1e9f;
    int found = 0;
    for (int pass = 0; pass < 2 && !found; pass++)
        for (int x = g_so.ex - 40; x <= g_so.ex + 40; x++)
            for (int y = g_so.ey - 40; y <= g_so.ey + 40; y++) {
                float de = so_d2(x, y, g_so.ex, g_so.ey);
                if (de < 64.0f || de > 1600.0f || cont_at(x, y) != g_so.econt || area_id(x, y, 1)) continue;
                if (!so_near_water(x, y, 2) || (!pass && so_in_danger(x, y, 2.0f))) continue;
                int wx, wy, nz = 0;
                if (!so_find_open(x, y, 4, g_so.ocean, 1, &wx, &wy) || (!pass && so_in_danger(wx, wy, 1.0f))) continue;
                for (int k = 0; pass && k < g_sos.nz; k++) nz += so_d2(x, y, g_sos.zx[k], g_sos.zy[k]) < g_sos.zr[k] * g_sos.zr[k];
                float sc = sqrtf(de) + sqrtf(so_d2(wx, wy, g_so.kwx, g_so.kwy)) / 3.0f + 10.0f * (float)nz;
                if (sc < bd) { bd = sc; w->bx = x; w->by = y; w->wx = wx; w->wy = wy; found = 1 + pass; }
            }
    return found;
}
internal int so_is_wsol_in(ee_unit *u, int a, int b) {
    int id = unit_get_serial(u);
    for (int k = a; k < b; k++) if (g_so.wsol_id[k] == id) return 1;
    return 0;
}
internal int so_is_wsol(ee_unit *u) { return so_is_wsol_in(u, 0, g_so.nwsol); }
internal int so_tr_alive(ee_unit *u, int id) {
    for (int i = 0; i < g_sos.npt; i++) if (g_sos.pt[i] == u && unit_get_serial(u) == id) return 1;
    return 0;
}
internal void so_waves(uint32_t now) {
    float min = (float)now / 60000.0f;
    g_so.boarding = 0;
    for (int k = 0; k < g_so.nw; k++) g_so.boarding |= g_so.w[k].st <= 1;
    if (g_so.land && g_so.nw < 3 && g_so.wave_min[g_so.nw] && min >= (float)g_so.wave_min[g_so.nw] && g_so.sold[0] && g_so.trtype && g_so.nd) {
        struct so_wave *w = &g_so.w[g_so.nw++];
        memset(w, 0, sizeof *w);
        w->min = g_so.wave_min[g_so.nw - 1];
        w->ntr = g_so.level + 1;
        int cap = ld_cap(g_so.trtype), want = w->ntr * cap, put = 0;
        for (int k = 0; k < w->ntr; k++) so_spawn(g_so.trtype, g_so.kwx + (k % 2), g_so.kwy + (k / 2) % 2, 1);
        // the soldiers on land tiles of P1's island near the first dock, one a tile
        for (int d = 1; d <= 14 && put < want; d++)
            for (int dx = -d; dx <= d && put < want; dx++)
                for (int dy = -d; dy <= d && put < want; dy += (abs(dx) == d ? 1 : 2 * d)) {
                    int x = g_so.kx + dx, y = g_so.ky + dy;
                    if (cont_at(x, y) != g_so.hcont || area_id(x, y, 1) || ((x + y) & 1)) continue;
                    so_spawn(g_so.sold[put & 1], x, y, 1); put++;
                }
        g_so.tr_spawned += w->ntr; g_so.sol_spawned += put;
        w->st = 0; w->t = now;
        so_log("# sea: wave %d at %.1f min: %d transports (type %d) and %d soldiers (types %d, %d) at %d,%d\n", g_so.nw, min,
               w->ntr, g_so.trtype, put, g_so.sold[0], g_so.sold[1], g_so.kx, g_so.ky);
    }
    for (int k = 0; k < g_so.nw; k++) {
        struct so_wave *w = &g_so.w[k];
        int alive = 0, aboard = 0;
        for (int j = 0; j < w->ntr; j++)
            if (w->tr[j] && so_tr_alive(w->tr[j], w->tr_id[j])) { alive++; aboard += units_inside(w->tr[j]); }
            else w->tr[j] = NULL;
        if (w->st == 0 && now - w->t >= 4000) {   // the spawned units are there: load them
            int nt = 0;
            for (int i = 0; i < g_sos.npt && nt < w->ntr; i++) {
                int used = 0;
                for (int q = 0; q < g_so.nw; q++) for (int j = 0; j < g_so.w[q].ntr; j++) used |= g_so.w[q].tr[j] == g_sos.pt[i];
                if (!used && so_d2(unit_get_x(g_sos.pt[i]), unit_get_y(g_sos.pt[i]), g_so.kwx, g_so.kwy) < 100.0f) {
                    w->tr[nt] = g_sos.pt[i]; w->tr_id[nt++] = unit_get_serial(g_sos.pt[i]);
                }
            }
            w->ntr = nt;
            int ns = 0;
            w->w0 = g_so.nwsol;
            for (int i = 0; i < g_sos.npl && g_so.nwsol < 256; i++) {
                ee_unit *u = g_sos.pl[i];
                if (!so_is_wsol(u) && so_d2(unit_get_x(u), unit_get_y(u), g_so.kx, g_so.ky) <= 256.0f) {
                    g_so.wsol_id[g_so.nwsol++] = unit_get_serial(u); ns++;
                }
            }
            w->w1 = g_so.nwsol; w->st = 1; w->t = now; w->load_t = 0;
            if (!nt || !ns) { w->st = 5; so_log("# sea: wave %d: %d transports, %d soldiers found - called off\n", k + 1, nt, ns); }
            continue;
        }
        if (w->st == 1) {   // loading: all aboard or 150 s; the load goals again every 20 s (as landing.c every 30)
            static ee_unit *left[96];
            int nl = 0;
            for (int i = 0; i < g_sos.npl && nl < 96; i++)
                if (so_is_wsol_in(g_sos.pl[i], w->w0, w->w1) && cont_at(unit_get_x(g_sos.pl[i]), unit_get_y(g_sos.pl[i])) == g_so.hcont)
                    left[nl++] = g_sos.pl[i];
            if (!alive) { w->st = 5; continue; }
            if (nl && now - w->t < 150000) {
                if (w->load_t && now - w->load_t < 20000) continue;
                w->load_t = now;
                int a = 0, cap = ld_cap(g_so.trtype);
                for (int j = 0; j < w->ntr && a < nl; j++) {   // each transport its room's worth of those still ashore
                    if (!w->tr[j]) continue;
                    int room = cap - units_inside(w->tr[j]), b = a + room > nl ? nl : a + room;
                    if (b <= a) continue;
                    ee_goal *g = goal_load(w->tr[j], left + a, b - a);
                    for (int q = a; q < b && g; q++) g_so.orders += (unsigned)unit_goal(left[q], goal_load_part(g), 0, 0x1F40);
                    a = b;
                }
                continue;
            }
            if (!aboard || !(w->hot = so_beach(w))) { w->st = 5; so_log("# sea: wave %d: no beach or nobody aboard (%d) - called off\n", k + 1, aboard); continue; }
            w->hot = w->hot == 2;
            w->aboard = aboard; w->st = 2; w->t = now; w->load_t = 0; g_so.sailed++;
            so_log("# sea: wave %d sails at %.1f min: %d transports, %d aboard, beach %d,%d (water %d,%d)%s\n", k + 1, min, alive, aboard, w->bx, w->by,
                   w->wx, w->wy, w->hot ? " - no beach out of danger" : "");
        }
        if (w->st == 2 || w->st == 3) {
            if (!alive || !aboard) {   // all sunk, or everyone ashore
                int on = 0;
                for (int i = 0; i < g_sos.npl; i++) on += so_is_wsol_in(g_sos.pl[i], w->w0, w->w1) && cont_at(unit_get_x(g_sos.pl[i]), unit_get_y(g_sos.pl[i])) == g_so.econt;
                w->landed = on;
                if (on) { g_so.ashore++; g_so.landed += on; } else g_so.stopped++;
                so_log("# sea: wave %d %s at %.1f min: %d of %d soldiers ashore, %d of %d transports afloat\n", k + 1, on ? "ashore" : "stopped",
                       min, on, w->aboard, alive, w->ntr);
                w->st = 4;
                for (int j = 0; j < w->ntr; j++) if (w->tr[j]) so_go(w->tr[j], g_so.kwx, g_so.kwy);
                continue;
            }
            // sailing: off the beach (or 150 s on the way) the unload goal - the game takes the transport to the shore and
            // unloads; again every 20 s while soldiers are aboard; 4 minutes of that: given up
            if (w->st == 2 && now - w->t > 150000) { w->st = 3; w->t = now; }
            int again = !w->load_t || now - w->load_t >= 20000;
            for (int j = 0; j < w->ntr; j++) {
                ee_unit *sh = w->tr[j];
                if (!sh || units_inside(sh) <= 0) continue;
                float d = so_d2(unit_get_x(sh), unit_get_y(sh), w->wx, w->wy);
                if (d <= 16.0f || w->st == 3) {
                    if (w->st == 2) { w->st = 3; w->t = now; again = 1; }
                    if (again) { g_so.orders += (unsigned)unit_goal(sh, goal_unload(sh, w->bx, w->by, 1), 0, 0x1F40); w->load_t = now; }
                } else so_go(sh, w->wx, w->wy);   // the plain way: a transport doesn't wait for a zone to clear
            }
            if (w->st == 3 && now - w->t > 240000) {
                w->st = 4; g_so.stopped++;
                so_log("# sea: wave %d: 4 minutes off the beach, %d still aboard - given up (stopped)\n", k + 1, aboard);
                for (int j = 0; j < w->ntr; j++) if (w->tr[j]) so_go(w->tr[j], g_so.kwx, g_so.kwy);
            }
        }
    }
}
// P1's soldiers: on the AI's land they attack; at home the guard takes on the AI's land units on P1's island
internal void so_soldiers(uint32_t now) {
    static const int guard_n[4] = { 0, 6, 10, 14 };
    int guard = 0, orders = 0;
    for (int i = 0; i < g_sos.npl; i++) {
        ee_unit *u = g_sos.pl[i];
        ee_unit *cur;
        int x = unit_get_x(u), y = unit_get_y(u), home = cont_at(x, y) == g_so.hcont;
        if (home && !(g_so.boarding && so_is_wsol(u))) guard++;
        if (home && g_so.boarding && so_is_wsol(u)) continue;   // waiting to board
        if (so_cur_target(u, &cur) && unit_get_owner(cur) == game_players()[g_so.ai]) continue;
        if (orders >= 40) continue;
        ee_unit *best = NULL;
        float bd = 1e12f;
        for (int k = 0; k < g_sos.nu; k++) {
            struct so_eu *e = &g_sos.u[k];
            if (!e->seen || e->cls == SC_WAR || e->cls == SC_TR || e->cls == SC_BOAT || area_id(e->x, e->y, 1)) continue;
            float d = so_d2(x, y, e->x, e->y);
            if (home) {
                if (cont_at(e->x, e->y) != g_so.hcont || so_d2(e->x, e->y, g_so.hx, g_so.hy) > 625.0f) continue;
            } else {
                d += e->cls == SC_SOLD || e->cls == SC_SIEGE ? (d <= 144.0f ? 0 : 1e6f) : e->cls == SC_CIT ? (d <= 225.0f ? 2e6f : 4e6f) : 5e6f;
                if (cont_at(e->x, e->y) != cont_at(x, y)) continue;
            }
            if (d < bd) { bd = d; best = e->u; }
        }
        if (best) { g_so.orders += (unsigned)unit_goal(u, goal_attack(u, best), 0, 0x1F40); orders++; }
        else if (home && so_d2(x, y, g_so.hx, g_so.hy) > 100.0f && !as_goal_near(u, (float)g_so.hx + 2.5f, (float)g_so.hy + 2.5f, 3.0f)) {
            g_so.orders += (unsigned)unit_goal(u, goal_move((float)g_so.hx + 2.5f, (float)g_so.hy + 2.5f), 0, 0x1F40); orders++;
        }
    }
    if (g_so.sold[0] && guard < guard_n[g_so.level] && now - g_so.guard_t >= 30000) {
        for (int k = 0; k < 2; k++) {
            int x = g_so.hx + 3 + k * 2, y = g_so.hy + 3;
            if (cont_at(x, y) == g_so.hcont && !area_id(x, y, 1)) { so_spawn(g_so.sold[k], x, y, 1); g_so.guard_spawned++; }
        }
        g_so.guard_t = now ? now : 1;
    }
    if (!g_so.towers && g_so.tower) {
        static const int at[2][2] = { { 6, -2 }, { -2, 6 } };
        for (int k = 0; k < 2; k++) {
            int x = g_so.hx + at[k][0], y = g_so.hy + at[k][1];
            if (cont_at(x, y) == g_so.hcont && !area_id(x, y, 1)) so_spawn(g_so.tower, x, y, 1);
        }
        g_so.towers = 1;
    }
}

// held: no enemy warship within 12 tiles of a side's buildings
internal void so_held(uint32_t now) {
    uint32_t dt = g_so.held_t && now > g_so.held_t ? now - g_so.held_t : 0;
    g_so.held_t = now;
    if (dt > 10000) dt = 10000;
    int ai_cont = 0, opp_cont = 0;
    for (int k = 0; k < g_so.ns && !ai_cont; k++) {
        int x = unit_get_x(g_so.s[k].u), y = unit_get_y(g_so.s[k].u);
        for (int i = 0; i < g_sos.nu && !ai_cont; i++)
            ai_cont = (g_sos.u[i].cls == SC_BLD || g_sos.u[i].cls == SC_DOCK || g_sos.u[i].cls == SC_DEF) && so_d2(x, y, g_sos.u[i].x, g_sos.u[i].y) <= 144.0f;
    }
    for (int i = 0; i < g_sos.nu && !opp_cont; i++) {
        if (g_sos.u[i].cls != SC_WAR) continue;
        for (int b = 0; b < g_sos.npb && !opp_cont; b++) opp_cont = so_d2(g_sos.u[i].x, g_sos.u[i].y, g_sos.pbx[b], g_sos.pby[b]) <= 144.0f;
    }
    g_so.total_ms += dt;
    if (!ai_cont) g_so.ai_held_ms += dt;
    if (!opp_cont) g_so.opp_held_ms += dt;
    // the AI's citizens killed now: by ships while no P1 soldier is on other land and no AI land unit on P1's island
    uint32_t civ = g_kills[1][g_so.ai][K_CIVIL] - g_nv_boat_kills[1][g_so.ai];
    if (civ > g_so.civ_last && !g_sos.opp_off_home && !g_sos.ai_on_opp) g_so.civ_ships += (int)(civ - g_so.civ_last);
    g_so.civ_last = civ;
}

internal void so_report(uint32_t now) {
    int nr[SR_N] = { 0 }, ng[SO_G] = { 0 };
    float hp = 0, hpm = 0;
    for (int k = 0; k < g_so.ns; k++) {
        nr[g_so.s[k].role]++; ng[g_so.s[k].grp]++;
        hp += (float)unit_get_hp(g_so.s[k].u); hpm += so_hpmax(g_so.s[k].u);
    }
    int ai = g_so.ai, seen = 0, mem = 0;
    unsigned trl = 0;
    for (int t = 0; t < G_TYPES_N; t++) if (is_transport(t)) trl += g_nv_lost_type[ai][t];
    for (int i = 0; i < g_sos.nu; i++) seen += g_sos.u[i].seen && g_sos.u[i].cls == SC_WAR;
    for (int k = 0; k < g_so.nm; k++) mem += g_so.m[k].kind == 0;
    unsigned opp_trl = 0;
    for (int t = 0; t < G_TYPES_N; t++) if (is_transport(t)) opp_trl += g_nv_lost_type[1][t];
    so_log("# sea: P1 vs P%d at %u min (%s): opp_war=%d raid=%d main=%d patrol=%d F=%d L=%d A=%d S=%d opp_hp=%.0f spawned=%d "
           "opp_lost=%d opp_lost_by_ai=%u ai_war=%d ai_war_most=%d ai_war_lost=%u ai_tr=%d ai_tr_lost=%u ai_boats=%d "
           "ai_boats_lost=%u ai_fish=%d ai_cit_killed=%u ai_cit_by_ships=%d waves=%d sailed=%d ashore=%d stopped=%d "
           "landed=%d opp_tr_lost=%u ai_held_s=%u opp_held_s=%u total_s=%u ai_isl_bld=%d ai_isl_towns=%d ai_on_opp=%d "
           "seen_war=%d mem_war=%d dz=%d guard=%d retreats=%u flees=%u detours=%u orders=%u | raid %s, main %s, patrol %s\n",
           ai, now / 60000u, k_so_lvl[g_so.level], g_so.ns, ng[0], ng[1], ng[2], nr[0], nr[1], nr[2], nr[3], hpm > 0 ? 100.0f * hp / hpm : 0.0f,
           g_so.spawned, g_so.lost, g_kills[ai][1][K_NAVY] - opp_trl, g_sos.ai_war, g_so.ai_war_most,
           g_kills[1][ai][K_NAVY] > trl ? g_kills[1][ai][K_NAVY] - trl : 0, g_sos.ai_tr, trl, g_sos.ai_boats, g_nv_boat_kills[1][ai],
           g_nv_fish_food[ai], g_kills[1][ai][K_CIVIL] - g_nv_boat_kills[1][ai], g_so.civ_ships, g_so.nw, g_so.sailed, g_so.ashore,
           g_so.stopped, g_so.landed, opp_trl, g_so.ai_held_ms / 1000u, g_so.opp_held_ms / 1000u, g_so.total_ms / 1000u,
           g_sos.ai_isl, g_sos.ai_towns, g_sos.ai_on_opp, seen, mem, g_sos.nz, g_so.guard_spawned,
           g_so.retreats, g_so.flees, g_so.detours, g_so.orders, g_so.g[0].why ? g_so.g[0].why : "-", g_so.g[1].why ? g_so.g[1].why : "-", g_so.g[2].why ? g_so.g[2].why : "-");
}

internal void seaop_tick(uint32_t now) {
    if (!g_so.on) return;
    if (now < g_so.last) { memset(&g_so, 0, sizeof g_so); return; }   // a new game
    if (now - g_so.last < 2000) return;
    g_so.last = now;
    if (!game_players()[1] || !game_players()[g_so.ai] || !game_map()) return;
    if (!g_so.types_t || now - g_so.types_t >= 300000) { so_types(); g_so.types_t = now ? now : 1; }
    so_scan(now);
    so_ships();
    if (g_sos.ai_war > g_so.ai_war_most) g_so.ai_war_most = g_sos.ai_war;
    so_spawn_tick(now);
    for (int gi = 0; gi < SO_G; gi++) so_group(gi, now);
    so_waves(now);
    so_soldiers(now);
    so_held(now);
    if (now - g_so.log_t >= 60000) { g_so.log_t = now; so_report(now); }
}

// seaop start <ai> [level] | off | (status): the sea opponent (above)
internal void cmd_seaop(char *out, const char *args) {
    int ai = 0, level = 0;
    if (!strncmp(args, "off", 3)) { g_so.on = 0; REPLY("seaop: off\n"); return; }
    if (!strncmp(args, "ships", 5)) {   // each warship: group, type, tile, hit points, goal; the AI's units within 10 tiles
        for (int k = 0; k < g_so.ns; k++) {
            ee_unit *u = g_so.s[k].u;
            ee_unit *cur = NULL;
            int x = unit_get_x(u), y = unit_get_y(u);
            so_cur_target(u, &cur);
            REPLY("  %s T%d at %d,%d hp %d/%.0f %s", g_so.s[k].grp == SO_RAID ? "raid" : g_so.s[k].grp == SO_MAIN ? "main" : "patrol",
                  type_get_id(unit_get_type_data(u)), x, y, unit_get_hp(u), so_hpmax(u), rtti_name(unit_get_goal(u)));
            if (cur) REPLY(" -> T%d at %d,%d", type_get_id(unit_get_type_data(cur)), unit_get_x(cur), unit_get_y(cur));
            REPLY(" | near:");
            for (int i = 0; i < g_sos.nu; i++) {
                struct so_eu *e = &g_sos.u[i];
                if (so_d2(x, y, e->x, e->y) <= 100.0f && e->cls != SC_CIT && e->cls != SC_OTHER)
                    REPLY(" T%d/c%d%s@%d,%d", type_get_id(e->td), e->cls, e->seen ? "" : "?", e->x, e->y);
            }
            REPLY("%s\n", so_in_danger(x, y, 0.0f) ? " IN DANGER" : "");
        }
        for (int k = 0; k < g_sos.nz; k++) REPLY("  zone %d,%d r %.1f\n", g_sos.zx[k], g_sos.zy[k], g_sos.zr[k]);
        return;
    }
    if (strncmp(args, "start", 5)) {
        REPLY("seaop: %s vs P%d level %d: %d warships (raid %d, main %d, patrol %d), %d remembered, %d danger zones, types F%d L%d A%d S%d "
              "transport %d soldiers %d/%d tower %d, waves %d\n", g_so.on ? "on" : "off", g_so.ai, g_so.level, g_so.ns, g_so.g[0].n, g_so.g[1].n,
              g_so.g[2].n, g_so.nm, g_sos.nz, g_so.types[0], g_so.types[1], g_so.types[2], g_so.types[3], g_so.trtype, g_so.sold[0], g_so.sold[1],
              g_so.tower, g_so.nw);
        return;
    }
    sscanf(args + 5, "%d %d", &ai, &level);
    ee_player **pls = game_players();
    if (ai < 2 || ai > 15 || !pls[1] || !pls[ai] || !game_map()) { REPLY("error: seaop start <ai> [level] - no player %d or no game\n", ai); return; }
    memset(&g_so, 0, sizeof g_so);
    g_so.ai = ai;
    g_so.level = level >= 1 && level <= 3 ? level : ini_int("autotest", "seaop_level", 2, g_ini);
    if (g_so.level < 1 || g_so.level > 3) g_so.level = 2;
    g_so.land = ini_int("autotest", "seaop_landings", 1, g_ini);
    g_so.trace = ini_int("autotest", "seaop_trace", 0, g_ini);
    int st = ini_int("autotest", "seaop_start", -1, g_ini);
    if (st >= 0) g_so.start_ms = (uint32_t)st * 60000u + 1;
    static const int waves[4][3] = { { 0, 0, 0 }, { 35, 0, 0 }, { 25, 45, 0 }, { 20, 32, 44 } };
    for (int k = 0; k < 3; k++) g_so.wave_min[k] = waves[g_so.level][k];
    if (!so_home_of(1, &g_so.hx, &g_so.hy) || !so_home_of(ai, &g_so.ex, &g_so.ey)) { REPLY("error: P1 or P%d has no buildings\n", ai); return; }
    g_so.hcont = cont_at(g_so.hx, g_so.hy); g_so.econt = cont_at(g_so.ex, g_so.ey);
    // war, both ways (FN_SET_DIPLOMACY: 1 = enemy)
    if (player_get_stance(pls[1], ai) != 1) set_diplomacy(pls[1], pls[ai], 1, 0);
    if (player_get_stance(pls[ai], 1) != 1) set_diplomacy(pls[ai], pls[1], 1, 0);
    // P1's docks: land of its island at the sea (open water within 4), toward the AI first, the second 10+ tiles away
    for (int k = 0; k < 2; k++) {
        float best = 1e9f;
        for (int x = g_so.hx - 20; x <= g_so.hx + 20; x++)
            for (int y = g_so.hy - 20; y <= g_so.hy + 20; y++) {
                int wx, wy;
                if (cont_at(x, y) != g_so.hcont || area_id(x, y, 1) || !so_near_water(x, y, 1)) continue;
                if (k && so_d2(x, y, g_so.dx[0], g_so.dy[0]) < 100.0f) continue;
                float d = so_d2(x, y, g_so.ex, g_so.ey);
                if (d >= best || !so_find_open(x, y, 4, g_so.ocean, 1, &wx, &wy)) continue;
                best = d; g_so.dx[k] = x; g_so.dy[k] = y; g_so.dwx[k] = wx; g_so.dwy[k] = wy;
            }
        if (best >= 1e9f) break;
        g_so.nd++;
        if (!k) g_so.ocean = area_id(g_so.dwx[0], g_so.dwy[0], 1);
    }
    if (!g_so.nd) { REPLY("error: no shore on P1's island\n"); return; }
    {   // the embarking beach: land at the sea with room (12+ land tiles within 3), 6+ tiles from the docks, nearest dock 0
        float best = 1e9f;
        g_so.kx = g_so.dx[0]; g_so.ky = g_so.dy[0]; g_so.kwx = g_so.dwx[0]; g_so.kwy = g_so.dwy[0];
        for (int x = g_so.hx - 20; x <= g_so.hx + 20; x++)
            for (int y = g_so.hy - 20; y <= g_so.hy + 20; y++) {
                int wx, wy, land = 0, clear = 1;
                for (int k = 0; k < g_so.nd; k++) clear &= so_d2(x, y, g_so.dx[k], g_so.dy[k]) >= 36.0f;
                if (!clear || cont_at(x, y) != g_so.hcont || area_id(x, y, 1) || !so_near_water(x, y, 1)) continue;
                float d = so_d2(x, y, g_so.dx[0], g_so.dy[0]);
                if (d >= best) continue;
                for (int ax = -3; ax <= 3; ax++)
                    for (int ay = -3; ay <= 3; ay++) land += cont_at(x + ax, y + ay) == g_so.hcont && !area_id(x + ax, y + ay, 1);
                if (land < 12 || !so_find_open(x, y, 3, g_so.ocean, 1, &wx, &wy)) continue;
                best = d; g_so.kx = x; g_so.ky = y; g_so.kwx = wx; g_so.kwy = wy;
            }
    }
    for (int k = 0; k < g_so.nd && ut_has(UT(B_DOCK)); k++) so_spawn(UT(B_DOCK), g_so.dx[k], g_so.dy[k], 1);
    // the AI's coast: open water nearest its home; the lane between; a ring of 8 points off its coast
    if (!so_find_open(g_so.ex, g_so.ey, 40, g_so.ocean, 1, &g_so.awx, &g_so.awy)) { g_so.awx = g_so.dwx[0]; g_so.awy = g_so.dwy[0]; }
    static const float lf[3] = { 0.3f, 0.55f, 0.8f };
    for (int k = 0; k < 3; k++) {
        int x = g_so.dwx[0] + (int)((float)(g_so.awx - g_so.dwx[0]) * lf[k]), y = g_so.dwy[0] + (int)((float)(g_so.awy - g_so.dwy[0]) * lf[k]);
        if (!so_find_open(x, y, 12, g_so.ocean, 1, &g_so.lane[k][0], &g_so.lane[k][1])) { g_so.lane[k][0] = g_so.dwx[0]; g_so.lane[k][1] = g_so.dwy[0]; }
    }
    for (int a = 0; a < 8; a++) {
        static const int dir[8][2] = { { 1, 0 }, { 1, 1 }, { 0, 1 }, { -1, 1 }, { -1, 0 }, { -1, -1 }, { 0, -1 }, { 1, -1 } };
        for (int d = 4; d <= 40; d++) {
            int x = g_so.ex + dir[a][0] * d, y = g_so.ey + dir[a][1] * d, wx, wy;
            if (!so_is_water(x, y, g_so.ocean)) continue;
            x += dir[a][0] * 3; y += dir[a][1] * 3;
            if (so_find_open(x, y, 5, g_so.ocean, 1, &wx, &wy)) { g_so.ring[g_so.nring][0] = wx; g_so.ring[g_so.nring++][1] = wy; }
            break;
        }
    }
    so_types();
    g_so.types_t = game_time() ? game_time() : 1;
    g_so.last = game_time();
    g_so.on = 1;
    REPLY("seaop: P1 vs P%d, %s: home %d,%d (land %u) vs %d,%d (land %u), sea %u; docks %d,%d (water %d,%d) and %d,%d (water %d,%d); "
          "embark %d,%d (water %d,%d); lane %d,%d %d,%d %d,%d; %d coast points; types F%d L%d A%d S%d transport %d; waves at %d/%d/%d min%s\n",
          ai, k_so_lvl[g_so.level], g_so.hx, g_so.hy, g_so.hcont, g_so.ex, g_so.ey, g_so.econt, g_so.ocean, g_so.dx[0], g_so.dy[0],
          g_so.dwx[0], g_so.dwy[0], g_so.dx[1], g_so.dy[1], g_so.dwx[1], g_so.dwy[1], g_so.kx, g_so.ky, g_so.kwx, g_so.kwy, g_so.lane[0][0], g_so.lane[0][1], g_so.lane[1][0],
          g_so.lane[1][1], g_so.lane[2][0], g_so.lane[2][1], g_so.nring, g_so.types[0], g_so.types[1], g_so.types[2], g_so.types[3],
          g_so.trtype, g_so.wave_min[0], g_so.wave_min[1], g_so.wave_min[2], g_so.land ? "" : " (off)");
}
