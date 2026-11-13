// Defences: how the AI fortifies its base. Included by eeaimod.c after attack.c (uses its defence_stats, is_wall).

// DEFENCE LAYOUT (ageofai.ini [ai_base] defence_layout, default on). The game's AI decides when and how many towers to
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
// front, an 11-tile wall goes up 3 tiles in front of them (the wall manager's own 0x497C3F(from, to, type): the
// citizens get the same build-wall order a player gives). The ends stay open - own units walk round, attackers on
// foot must break through or walk the length of it under tower fire. Straight (2026-10-27, the player: the walls are
// useful but should be straightened out and use gates): across the front at the nearest of the 8 directions - along
// the tiles or exactly diagonal; at any other angle the pieces stood as a staircase. A gate in its middle once the 5
// pieces there are finished, so its own units pass through: as the player lays one - the gate button over a run of
// finished pieces, no builders, there at once - by the same call (build_gate, 0x6022E3: the pieces turned into the
// gate's type; the wall manager's own gates go that way too, 0x498017). A gate laid as a building never got built (into
// a gap, before the wall, by the wall manager's drag, or onto the finished wall: it stayed at 1 hit point).
internal int g_def = 1, g_def_dist = 10, g_def_walls = 1, g_def_more = 1;
internal int g_aa = 1, g_aa_ahead = 1, g_aa_ahead_b = 3;   // [ai_base] anti_air, aa_ahead, aa_ahead_buildings (below)
internal int g_aa_exp = 0, g_v_aa_exp = -1;                 // [ai_base] aa_expected, [autotest] v_aa_expected (ANTI-AIR, BY THE AIR FORCE IT EXPECTS)
internal int g_aa_strong = 0, g_v_aa_strong = -1;           // [ai_base] aa_strong, [autotest] v_aa_strong (ANTI-AIR, STRONG)
internal int g_hold = 0;   // [ai_base] hold_town (below)
internal uint8_t g_def_off[16];   // per player, pipe "defence P 0|1" (A/B scenarios)
internal unsigned g_def_towers[16], g_def_walls_built[16];
struct front { int fx, fy, enemy; float dx, dy; int wall_tries; uint32_t wall_t; int gx, gy, gdir, gtries; };   // its wall's gate: middle, direction (0 no wall), tries
internal struct def_plan {
    ee_player *pl; uint32_t t; int bx, by, nf, nout;
    struct front f[2];
    int out[8][2];
    struct { int x, y; uint32_t t; } pend[8];   // slots given out recently (the construction may not show yet)
} g_plan[16];

internal unsigned area_id(ivec2 p, int kind);   // forces.c: 1 the sea, 2 the land there (0 none)
internal const enum ut_name k_tower_line[] = { UT_TOWER_LINE };   // oldest first
internal int tower_newest(ee_player *pl) {   // the best tower the game lets this player build now (0: none)
    for (int i = 6; i >= 0; i--)
        if (ut_has(g_ut[k_tower_line[i]]) && can_build(pl, g_ut[k_tower_line[i]])) return g_ut[k_tower_line[i]];
    return 0;
}
internal int bq_has(ee_build_mgr *bm, int type);   // attack.c
internal int tower_queued(ee_build_mgr *bm) {   // a tower of the line in the build manager's queues
    for (int i = 0; i < 7; i++) if (bq_has(bm, g_ut[k_tower_line[i]])) return 1;
    return 0;
}
internal int can_place(ee_player *pl, int type, ivec2 p) {
    ee_unit_type *td = type_data(type);
    ee_map *map = game_map();
    int n = map ? (int)map_get_size(map) : 0, sz = td ? unit_type_get_size(td) : 0;
    if (!td || p.x < 2 || p.y < 2 || p.x >= n - 2 || p.y >= n - 2) return 0;
    if (sz < 1) sz = 1;
    return can_place_area(pl, type, p.x, p.y, sz) != 0;
}

struct def_scan { ee_player *pl; int bx, by; int d2[16], x[16], y[16]; int towers, tx[64], ty[64]; int walls, wx[256], wy[256]; };   // walls: gate pieces too
internal void def_scan_unit(ee_unit *u, void *ctx) {
    struct def_scan *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    ee_player *owner = unit_get_owner(u);
    int fam = unit_type_get_class_id(td), x = unit_get_x(u), y = unit_get_y(u);
    if (fam == CLS_GATE && owner == s->pl && s->walls < 256) { s->wx[s->walls] = x; s->wy[s->walls++] = y; return; }
    if (fam != CLS_BUILDING && fam != CLS_FORTRESS) return;
    if (owner == s->pl) {
        if (is_tower_type(unit_type_get_id(td)) && s->towers < 64) { s->tx[s->towers] = x; s->ty[s->towers++] = y; }
        else if (is_wall(unit_type_get_id(td)) && s->walls < 256) { s->wx[s->walls] = x; s->wy[s->walls++] = y; }
        return;
    }
    if (!owner || !is_enemy(s->pl, owner) || !seen_by(u, s->pl)) return;
    unsigned pi = (unsigned)player_get_index(owner) & 15;
    int d2 = ivec2_dist2(iv2(x, y), iv2(s->bx, s->by));
    if (d2 < s->d2[pi]) { s->d2[pi] = d2; s->x[pi] = x; s->y[pi] = y; }
}

internal struct def_scan g_scan;   // last scan (own towers), sim thread only
internal void def_refresh(ee_player *pl, unsigned idx, uint32_t now) {
    struct def_plan *p = &g_plan[idx];
    if (p->pl == pl && now >= p->t && now - p->t < 30000) return;
    if (p->pl != pl) memset(p, 0, sizeof *p);
    p->pl = pl; p->t = now;
    void *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    p->nf = p->nout = 0;
    if (!bm || build_mgr_get_base_x(bm) == -10000) return;
    p->bx = build_mgr_get_base_x(bm); p->by = build_mgr_get_base_y(bm);
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
            ee_map *map = game_map();
            int n = map ? (int)map_get_size(map) : 0;
            dx = (float)(n / 2 - p->bx); dy = (float)(n / 2 - p->by);
        } else break;
        float len = sqrtf(dx * dx + dy * dy);
        if (len < 4.0f) break;
        struct front *f = &p->f[p->nf];
        f->dx = dx / len; f->dy = dy / len; f->enemy = best;
        f->fx = p->bx + (int)lroundf(f->dx * (float)g_def_dist);
        f->fy = p->by + (int)lroundf(f->dy * (float)g_def_dist);
        // keep the wall state while the front stays where it was
        f->wall_tries = 0; f->wall_t = 0; f->gdir = 0; f->gtries = 0;
        for (int j = 0; j < 2; j++)
            if (abs(old[j].fx - f->fx) <= 2 && abs(old[j].fy - f->fy) <= 2) {
                f->wall_tries = old[j].wall_tries; f->wall_t = old[j].wall_t;
                f->gx = old[j].gx; f->gy = old[j].gy; f->gdir = old[j].gdir; f->gtries = old[j].gtries;
            }
        p->nf++;
    }
    static struct drop d[64];
    int nd = drop_sites(pl, d, 64);
    for (int i = 0; i < nd && p->nout < 8; i++) {
        if (d[i].food || ivec2_dist2(d[i].p, iv2(p->bx, p->by)) < 144 || !p->nf) continue;
        p->out[p->nout][0] = d[i].p.x + (int)lroundf(p->f[0].dx * 3.0f);
        p->out[p->nout][1] = d[i].p.y + (int)lroundf(p->f[0].dy * 3.0f);
        p->nout++;
    }
}

// ENCLOSE ([ai_base] enclose, default 0 until measured; the player 2026-11-06: "the tower line with a wall is silly
// anyways, even on land you just go around it, the ai should try to enclose itself fully" - and on an island the wall
// stands at the water's edge, which makes landings impossible instead of keeping the landed troops out; towers packed
// tightly along the coast keep the ships away). Against the cheating base AI on Small Islands (docs/testing.md
// 2026-11-06) 86% of the mod's dead citizens were killed by troops that had landed, the first at 17-40 min.
// A plan per AI every minute (encl_plan): its home continent flood-filled from the base point - at most ENCL_ISLAND
// tiles and no enemy building seen on it: an island, its ring the coast (its land tiles beside the ocean); else a ring
// around the town, an octagon (its sides along the tiles and exactly diagonal, as the game lays a straight wall) 3
// tiles beyond its farthest building within 30 tiles, at least defence_distance + 2 and never shrinking, its tiles in
// the water left out (the water closes it). Within 2 tiles of an own dock the ring stays open (3 opened 29 of a small
// island's 36 coast tiles): the dock's builders, and the mod's landings board at the nearest free shore (landing.c).
// The ring nearest the own buildings first. Walls (in defence_tick): up to two straight runs a tick (8 directions, at
// most 12 pieces) of free ring tiles the wall can take; a gate in the middle of each of the octagon's four straight
// sides once its pieces stand. Towers: the defence layout's slots become spots along the ring (on the coast 1 tile
// inland and 5 apart, in the octagon 2 tiles inside and 6 apart), the outposts' after them; the front walls are not
// built. Stone: the walls first - a whole coast of 77 tiles is 770 stone, 4-5 towers; with 6 stone gatherers the extra
// towers took it all (from minute 10 every tick found under the 110 a piece needs): the extra towers wait for the open
// ring's stone (up to 40 pieces), and the economy puts workers on stone for it (econ.c, encl_stone).
#define ENCL_RING 2048
#define ENCL_ISLAND 8000
#define ENCL_SAFE 9
internal int g_encl_safe = 0;
internal int g_encl = 0, g_encl_from = 40, g_encl_runs = 1, g_encl_flight = 0, g_encl_ships = 0, g_encl_docks = 0;
// DOCK POCKETS (enclose_dock_gates; the player 2026-11-07: "a small open section with a gate that has space for docks and
// boarding, or several on larger islands"): the coast within 2 tiles of an own dock stays open for its builders and the
// ships; with this the opening is ENCL_POCKET - 1 tiles each way - room beside the dock for the landings' boarding (the
// dock filled the 2 tiles: on a walled Small Islands coast 28 of 56 boardings found no meeting point, every free tile by
// the sea taken) - and the pocket gets a wall of its own on the land side - the land tiles ENCL_POCKET tiles round the
// dock (a square, joining the coast's ring at both ends) - and a gate in the middle of its side towards the town once
// that side stands: what lands in the pocket meets the gate.
#define ENCL_POCKET 5
// THE TOWN RING ([ai_base] base_ring, default 0 until measured; the player 2026-11-13: "right now its open lines and
// messy, is it possible to plan a propper ring with gates and towers around the base? extending it once the base grew
// enough with stuff worth protecting" - and his choices: the town and the mines worked near it inside, an outgrown side
// moved out with the old stretch left standing as an inner wall, the ring only once the economy stands). An octagon
// whose eight sides lie along the tiles and exactly diagonal (as the game lays a straight wall), each side RING_MARGIN
// tiles beyond the farthest thing it holds on that side: the town's buildings within RING_TOWN of the base point (not
// its towers - they stand on the ring's inside), and every mine its citizens work within RING_MINE of one of them. Its
// wall goes on its border tiles of land: the sea, cliffs, a forest or a mine on the line close it where they stand (a
// tree cut down leaves a free tile, walled at the next look). A side the town comes closer to than RING_MARGIN moves
// out RING_GROW further than needed (not tile by tile); the stretch it leaves stays an inner wall, a gate in it. Gates:
// a run of five wall pieces in the middle of each straight side along the tiles, and of a diagonal side of RING_DIAG_GATE
// tiles or more. Towers 2 tiles inside: beside the gates first, then the corners, then along the sides. The ring's tiles
// and those beside it are kept free of new buildings (the packing asks ring_blocks).
#define RING_TOWN 30
#define RING_MINE 8
#define RING_LINK 8
#define RING_MARGIN 4
#define RING_GROW 8
#define RING_PRESS 3
#define RING_MAX 48
#define RING_DIAG_GATE 12
#define RING_BOX 128
internal int g_ring = 0, g_v_ring = -1;
internal struct encl {
    ee_player *pl; uint32_t t, log_t; int island, tiles, walled;
    int nring; int16_t rx[ENCL_RING], ry[ENCL_RING];   // in build order
    int nslot; int16_t sx[96], sy[96];                 // tower spots
    int ngate; struct { int16_t x, y; uint8_t dir, tries; } gate[16];
    // the town ring (base_ring): its eight sides, kept from plan to plan - xmin, xmax, ymin, ymax, x+y min/max, x-y min/max
    int town, has_rb, rb[8], grows; uint32_t wall_t;
    int nopen; int16_t open_x[32], open_y[32];   // pieces taken down for a pocket: never walled again
    struct { int16_t x, y; uint8_t dir, ok; } sgate[8];   // each side's gate, as the last plan placed it
    int ninner; struct { int16_t x, y; uint8_t dir, tries; } inner[8];   // the gates of the stretches left inside
    int box_x, box_y; uint8_t box[RING_BOX * RING_BOX / 8];   // the ring and the tiles beside it: no building there
    struct { int16_t ax, ay, bx, by; uint32_t t; } pend[24];   // runs ordered (their builders may not have started)
    unsigned runs, pieces, gates;
    int stone;   // the open ring's walls (the wall it builds now)
    int ready;   // citizens enough for the walls (enclose_citizens), at the last tick
    int safe, mines, safe_mines, sheltered;   // safe space: land and mines beyond ENCL_SAFE of the sea; enough of it
    unsigned skip_pend, skip_fire, skip_raid, skip_stone, skip_econ, skip_shut;   // DIAG: why the scans since the last log ordered nothing
} g_encs[16];

struct encl_scan { ee_player *pl; unsigned hc; int nb, enemy; ivec2 b[2048]; uint8_t bs[2048], btower[2048], bw[2048]; int nd; ivec2 d[16]; int nfoe; ivec2 foe[8]; int neb; ivec2 eb[512]; };
internal void encl_scan_unit(ee_unit *u, void *ctx) {   // own buildings and docks, an enemy building on the continent
    struct encl_scan *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    ee_player *o = unit_get_owner(u);
    int fam = unit_type_get_class_id(td), t = unit_type_get_id(td);
    if (o && o != s->pl && fam == CLS_LAND && unit_get_hp(u) > 0 && unit_get_x(u) >= 0 && player_get_index(o) > 0 && is_enemy(s->pl, o)) {
        ivec2 p = unit_get_tile(u);   // DIAG: enemy soldiers on the continent
        if (area_id(p, 2) == s->hc) { if (s->nfoe < 8) s->foe[s->nfoe] = p; s->nfoe++; }
        return;
    }
    if (!o || (fam != CLS_BUILDING && fam != CLS_FORTRESS) || unit_get_hp(u) <= 0 || is_wall(t)) return;
    ivec2 p = unit_get_tile(u);
    if (o == s->pl) {
        if (t == UT(B_DOCK) && s->nd < 16) s->d[s->nd++] = p;
        if (s->nb < 2048) {   // its tile is its footprint's corner (packing.c), size its side
            int sz = unit_type_get_size(td);
            s->bs[s->nb] = (uint8_t)(sz < 1 ? 1 : sz > 16 ? 16 : sz);
            s->btower[s->nb] = (uint8_t)is_tower_type(t);
            s->bw[s->nb] = (uint8_t)(t == UT(B_HOUSE) || t == UT(B_FARM) ? 1 : 3);   // the town ring: what a side moves out for
            s->b[s->nb++] = p;
        }
    } else if (player_get_index(o) > 0 && is_enemy(s->pl, o) && seen_by(u, s->pl)) {
        if (area_id(p, 2) == s->hc) s->enemy = 1;
        if (s->neb < 512) s->eb[s->neb++] = p;   // the town ring keeps halfway to them
    }
}
internal int encl_walled(int x, int y);
internal int encl_land(ivec2 q, unsigned hc, int n) {   // land of the continent (a wall piece takes its tile out), off the map's edge
    return q.x >= 3 && q.y >= 3 && q.x < n - 3 && q.y < n - 3 && ((area_id(q, 2) == hc && !area_id(q, 1)) || encl_walled(q.x, q.y));
}
internal int encl_coast(ivec2 q) {
    for (int dx = -1; dx <= 1; dx++)
        for (int dy = -1; dy <= 1; dy++)
            if ((dx || dy) && area_id(iv2(q.x + dx, q.y + dy), 1)) return 1;
    return 0;
}
internal int encl_walled(int x, int y) {   // a wall or gate piece on the tile (anyone's, any state)
    ee_map *map = game_map();
    int n = map ? (int)map_get_size(map) : 0;
    if (x < 0 || y < 0 || x >= n || y >= n) return 0;
    ee_tile *tile = map_get_tiles(map)[y * n + x];
    for (void *node = tile ? tile_get_objects(tile) : NULL; node; node = tilenode_get_next(node)) {
        ee_unit *o = tilenode_get_object(node);
        ee_unit_type *td = o ? unit_get_type_data(o) : NULL;
        if (td && unit_type_get_is_wall(td)) return 1;
    }
    return 0;
}
struct encl_key { int16_t x, y; int key; };
internal int encl_key_cmp(const void *a, const void *b) {
    const struct encl_key *p = a, *q = b;
    return p->key != q->key ? (p->key < q->key ? -1 : 1) : p->x != q->x ? p->x - q->x : p->y - q->y;
}
internal int encl_on(unsigned idx) { return g_encl || PV(idx, g_v_ring, g_ring); }   // a ring of either kind
// the town ring's sides: 0 xmin, 1 xmax, 2 ymin, 3 ymax, 4/5 x+y, 6/7 x-y - odd ones bound from above
internal int ring_val(int k, int x, int y) { return k < 2 ? x : k < 4 ? y : k < 6 ? x + y : x - y; }
internal int ring_in(const int *rb, int x, int y) {
    return x >= rb[0] && x <= rb[1] && y >= rb[2] && y <= rb[3] && x + y >= rb[4] && x + y <= rb[5] && x - y >= rb[6] && x - y <= rb[7];
}
internal int ring_border(const int *rb, int x, int y) {   // inside, with a tile outside beside it (four ways: a closed line)
    return ring_in(rb, x, y) && (!ring_in(rb, x + 1, y) || !ring_in(rb, x - 1, y) || !ring_in(rb, x, y + 1) || !ring_in(rb, x, y - 1));
}
internal int ring_sides(const int *rb, int x, int y) {   // the sides the tile lies on: 2 or more at a corner
    int c = 0;
    for (int k = 0; k < 8; k++) c += ring_val(k, x, y) == rb[k];
    return c;
}
internal const uint8_t k_ring_dir[8] = { 2, 2, 1, 1, 3, 3, 4, 4 };   // gate_fits' dir along each side
internal const int8_t k_ring_step[8][2] = { { 0, 1 }, { 0, 1 }, { 1, 0 }, { 1, 0 }, { 1, -1 }, { 1, -1 }, { 1, 1 }, { 1, 1 } };   // along it, x rising
// a gate's five tiles centred at (x, y) on side k: all on the ring's land, none at a corner
internal int ring_gate_ok(const int *rb, int k, int x, int y, unsigned hc, int n) {
    for (int i = -2; i <= 2; i++) {
        int tx = x + k_ring_step[k][0] * i, ty = y + k_ring_step[k][1] * i;
        if (!ring_border(rb, tx, ty) || ring_val(k, tx, ty) != rb[k] || ring_sides(rb, tx, ty) != 1 || !encl_land(iv2(tx, ty), hc, n)) return 0;
    }
    return 1;
}
internal int ring_own_gate(ee_player *pl, int x, int y) {   // a gate piece of pl's on the tile
    ee_map *map = game_map();
    int n = map ? (int)map_get_size(map) : 0;
    if (x < 0 || y < 0 || x >= n || y >= n) return 0;
    ee_tile *tile = map_get_tiles(map)[y * n + x];
    for (void *node = tile ? tile_get_objects(tile) : NULL; node; node = tilenode_get_next(node)) {
        ee_unit *o = tilenode_get_object(node);
        ee_unit_type *td = o ? unit_get_type_data(o) : NULL;
        if (td && unit_get_owner(o) == pl && ut_role(unit_type_get_id(td), UTR_GATE)) return 1;
    }
    return 0;
}
// The town ring's plan (encl_plan, base_ring): its sides moved out where the town presses on them, its border tiles of
// land into k, its gates and the box of tiles kept free.
internal void ring_plan(ee_player *pl, unsigned idx, struct encl *e, struct encl_scan *s, ivec2 b, unsigned hc, int n,
                        struct encl_key *k, int *pnk, uint32_t now) {
    int need[8], mpress[8] = { 0 }, held = 0, mines = 0;
    for (int j = 0; j < 8; j++) need[j] = j & 1 ? -0x7FFFFFFF : 0x7FFFFFFF;
    // the town: the buildings (not towers) joined to the base point by gaps of at most RING_LINK tiles - an outlying
    // settlement or a camp by a far mine is not its part
    static uint8_t in[2048];
    static int16_t town_q[2048];
    int nq = 0;
    for (int i = 0; i < s->nb; i++) {
        int x0 = s->b[i].x, y0 = s->b[i].y, x1 = x0 + s->bs[i] - 1, y1 = y0 + s->bs[i] - 1;
        int gx = b.x < x0 ? x0 - b.x : b.x > x1 ? b.x - x1 : 0, gy = b.y < y0 ? y0 - b.y : b.y > y1 ? b.y - y1 : 0;
        in[i] = 0;
        if (s->btower[i] || abs(x0 - b.x) > RING_TOWN || abs(y0 - b.y) > RING_TOWN || gx > RING_LINK || gy > RING_LINK) continue;
        in[i] = 1; town_q[nq++] = (int16_t)i;
    }
    for (int h = 0; h < nq; h++) {
        int a = town_q[h], ax1 = s->b[a].x + s->bs[a] - 1, ay1 = s->b[a].y + s->bs[a] - 1;
        for (int i = 0; i < s->nb; i++) {
            if (in[i] || s->btower[i] || abs(s->b[i].x - b.x) > RING_TOWN || abs(s->b[i].y - b.y) > RING_TOWN) continue;
            int x0 = s->b[i].x, y0 = s->b[i].y, x1 = x0 + s->bs[i] - 1, y1 = y0 + s->bs[i] - 1;
            int gx = x0 > ax1 ? x0 - ax1 : s->b[a].x > x1 ? s->b[a].x - x1 : 0, gy = y0 > ay1 ? y0 - ay1 : s->b[a].y > y1 ? s->b[a].y - y1 : 0;
            if (gx > RING_LINK || gy > RING_LINK) continue;
            in[i] = 1; town_q[nq++] = (int16_t)i;
        }
    }
#define RING_HOLD(px, py) do { for (int j_ = 0; j_ < 8; j_++) { int v_ = ring_val(j_, (px), (py)); if (j_ & 1 ? v_ > need[j_] : v_ < need[j_]) need[j_] = v_; } } while (0)
    RING_HOLD(b.x, b.y);
    for (int i = 0; i < s->nb; i++) {   // the town's footprints
        if (!in[i]) continue;
        int x0 = s->b[i].x, y0 = s->b[i].y, x1 = x0 + s->bs[i] - 1, y1 = y0 + s->bs[i] - 1;
        RING_HOLD(x0, y0); RING_HOLD(x1, y0); RING_HOLD(x0, y1); RING_HOLD(x1, y1);
        held++;
    }
    void *cm = player_get_civ_mgr(pl);
    ee_resource_mgr *rm = cm ? civ_mgr_get_resource_mgr(cm) : NULL;
    for (int r = 4; rm && r < 7; r++) {   // the mines its citizens work near the town
        uint8_t *list = resource_mgr_list_at(rm, r);
        for (unsigned q = 0, first = deque_front(list), cnt = deque_count(list); q < cnt && q < 1024; q++) {
            ee_unit *u = deque_at(list, first + q);
            ee_goal *goal = u && unit_get_hp(u) > 0 ? unit_get_goal(u) : NULL;
            ee_unit *site = goal && !strcmp(rtti_name(goal), "EEUGGather@@") ? goal_get_site(goal) : NULL;
            if (!site) continue;
            ee_unit_type *td = unit_get_type_data(site);
            int sz = td ? unit_type_get_size(td) : 1, x0 = unit_get_x(site), y0 = unit_get_y(site), by_town = 0;
            if (sz < 1 || sz > 16) sz = 1;
            for (int i = 0; i < s->nb && !by_town; i++)
                by_town = in[i] && abs(s->b[i].x - x0) <= RING_MINE && abs(s->b[i].y - y0) <= RING_MINE;
            if (!by_town) continue;
            RING_HOLD(x0, y0); RING_HOLD(x0 + sz - 1, y0); RING_HOLD(x0, y0 + sz - 1); RING_HOLD(x0 + sz - 1, y0 + sz - 1);
            mines++;
            for (int j = 0; e->has_rb && j < 8; j++) {   // a worked mine by the wall or beyond: weight enough
                int out = j & 1 ? 1 : -1, mg = j >= 4 ? 3 : 2, v = ring_val(j, x0, y0);
                for (int c = 1; c < 4; c++) { int w = ring_val(j, x0 + (c & 1) * (sz - 1), y0 + (c >> 1) * (sz - 1)); if ((w - v) * out > 0) v = w; }
                if ((v - (e->rb[j] - out * mg)) * out > 0) mpress[j] = RING_PRESS;
            }
        }
    }
#undef RING_HOLD
    // a side moves out RING_GROW beyond what it holds (a diagonal side's numbers are x+y or x-y: half again as many for
    // the same distance) - while no wall stands whenever the town comes within RING_MARGIN of it; once walls stand only
    // when the ring is closed (or its first wall is 15 minutes old: a stretch moved out half built stays behind as a
    // fragment), a full RING_GROW further (a step cut short by the enemy's limit left walls 5 tiles apart, three in a
    // row), and for the town's weight there: RING_PRESS houses or fields, or a bigger building or a worked mine, within 2
    // tiles of the wall or beyond (not for every house the game puts outside); the stretch it leaves keeps its gate as
    // an inner wall's. Never beyond halfway from the town's edge to a seen enemy building in front of the side: two
    // towns' rings don't cross.
    if (e->walled && !e->wall_t) e->wall_t = now ? now : 1;
    int building = e->walled && e->walled < e->nring - e->nring / 20 && now - e->wall_t < 900000;
    for (int j = 0; j < 8 && !building; j++) {
        int out = j & 1 ? 1 : -1, diag = j >= 4, mg = diag ? (RING_MARGIN * 3 + 1) / 2 : RING_MARGIN;
        int want = need[j] + out * mg;
        int lim = ring_val(j, b.x, b.y) + out * (diag ? RING_MAX * 3 / 2 : RING_MAX);
        for (int i = 0; i < s->neb; i++) {   // the enemy in front of this side
            int ev = ring_val(j, s->eb[i].x, s->eb[i].y), in_front = (ev - need[j]) * out > 0;
            for (int q = 0; q < 8 && in_front; q++) {
                if (q == j) continue;
                int qm = (q >= 4 ? 3 : 2) * RING_MARGIN, qb = e->has_rb ? e->rb[q] : need[q] + (q & 1 ? qm : -qm);
                in_front = q & 1 ? ring_val(q, s->eb[i].x, s->eb[i].y) <= qb + qm : ring_val(q, s->eb[i].x, s->eb[i].y) >= qb - qm;
            }
            int half = need[j] + (ev - need[j]) / 2;
            if (in_front && (half - lim) * out < 0) lim = half;
        }
        if (e->has_rb && (want - e->rb[j]) * out <= 0) continue;   // the town is still inside its margin
        if (e->has_rb && e->walled) {   // walls stand: only for the town's weight on this side
            int press = 0, pz = diag ? 3 : 2;
            for (int i = 0; i < s->nb; i++) {
                if (!in[i]) continue;
                int x0 = s->b[i].x, y0 = s->b[i].y, x1 = x0 + s->bs[i] - 1, y1 = y0 + s->bs[i] - 1, v = ring_val(j, x0, y0);
                int v2 = ring_val(j, x1, y0), v3 = ring_val(j, x0, y1), v4 = ring_val(j, x1, y1);
                if (out > 0) { if (v2 > v) v = v2; if (v3 > v) v = v3; if (v4 > v) v = v4; }
                else { if (v2 < v) v = v2; if (v3 < v) v = v3; if (v4 < v) v = v4; }
                if ((v - (e->rb[j] - out * pz)) * out > 0) press += s->bw[i];
            }
            if (press + mpress[j] < RING_PRESS) continue;
        }
        int to = want + out * (diag ? (RING_GROW * 3 + 1) / 2 : RING_GROW);
        if ((to - lim) * out > 0) to = lim;
        if (e->has_rb && (to - e->rb[j]) * out < (e->walled ? (diag ? (RING_GROW * 3 + 1) / 2 : RING_GROW) : 1)) continue;   // at its limit, or not a full step
        if (e->has_rb && e->walled) {
            e->grows++;
            int walled = 0;   // the old stretch's gate, where its pieces stand
            for (int i = -2; e->sgate[j].ok && i <= 2; i++)
                walled += encl_walled(e->sgate[j].x + k_ring_step[j][0] * i, e->sgate[j].y + k_ring_step[j][1] * i);
            if (walled >= 3 && e->ninner < 8) {
                e->inner[e->ninner].x = e->sgate[j].x; e->inner[e->ninner].y = e->sgate[j].y;
                e->inner[e->ninner].dir = e->sgate[j].dir; e->inner[e->ninner++].tries = 0;
            }
            if (g_log) fprintf(g_log, "# ring: P%u at %.1f min: side %d out from %d to %d (the town at %d)%s\n", idx, now / 60000.0,
                               j, e->rb[j], to, need[j], walled >= 3 ? ", the old stretch stays with its gate" : "");
        }
        e->rb[j] = to;
    }
    e->has_rb = 1;
    const int *rb = e->rb;
    int nk = *pnk;
    e->box_x = rb[0] - 1; e->box_y = rb[2] - 1;
    memset(e->box, 0, sizeof e->box);
    for (int x = rb[0]; x <= rb[1]; x++)
        for (int y = rb[2]; y <= rb[3]; y++) {
            if (!ring_border(rb, x, y)) continue;
            for (int dx = -1; dx <= 1; dx++)
                for (int dy = -1; dy <= 1; dy++) {
                    int bx = x + dx - e->box_x, by = y + dy - e->box_y;
                    if (bx >= 0 && by >= 0 && bx < RING_BOX && by < RING_BOX) e->box[(by * RING_BOX + bx) >> 3] |= (uint8_t)(1 << (bx & 7));
                }
            if (nk < ENCL_RING && encl_land(iv2(x, y), hc, n)) { k[nk].x = (int16_t)x; k[nk++].y = (int16_t)y; }
        }
    *pnk = nk;
    // the gates: in the middle of each side along the tiles, and of a long diagonal one - the run of five nearest it
    for (int j = 0; j < 8; j++) {
        static int16_t tx[512], ty[512];
        int nt = 0;
        e->sgate[j].ok = 0;
        for (int x = rb[0]; x <= rb[1]; x++)
            for (int y = rb[2]; y <= rb[3] && nt < 512; y++)
                if (ring_val(j, x, y) == rb[j] && ring_border(rb, x, y)) { tx[nt] = (int16_t)x; ty[nt++] = (int16_t)y; }
        if (nt < 5 || (j >= 4 && nt < RING_DIAG_GATE)) continue;
        int have = -1;   // a gate of its own on the side already: that one (the side's middle moves when its ends do)
        for (int i = 0; i < nt && have < 0; i++) if (ring_own_gate(pl, tx[i], ty[i])) have = i;
        if (have >= 0) {
            int i = have + 2 < nt ? have + 2 : have;
            e->sgate[j].x = tx[i]; e->sgate[j].y = ty[i]; e->sgate[j].dir = k_ring_dir[j]; e->sgate[j].ok = 1;
            continue;
        }
        for (int off = 0; off <= nt / 2 && !e->sgate[j].ok; off++)
            for (int sg = -1; sg <= 1 && !e->sgate[j].ok; sg += 2) {
                int i = nt / 2 + sg * off;
                if (i < 0 || i >= nt || !ring_gate_ok(rb, j, tx[i], ty[i], hc, n)) continue;
                e->sgate[j].x = tx[i]; e->sgate[j].y = ty[i]; e->sgate[j].dir = k_ring_dir[j]; e->sgate[j].ok = 1;
            }
        if (!e->sgate[j].ok || e->ngate >= 16) continue;
        e->gate[e->ngate].x = e->sgate[j].x; e->gate[e->ngate].y = e->sgate[j].y;
        e->gate[e->ngate].dir = e->sgate[j].dir; e->gate[e->ngate++].tries = 0;
    }
    for (int i = 0; i < e->ninner && e->ngate < 16; i++) {
        e->gate[e->ngate].x = e->inner[i].x; e->gate[e->ngate].y = e->inner[i].y;
        e->gate[e->ngate].dir = e->inner[i].dir; e->gate[e->ngate++].tries = 0;
    }
    static uint32_t logged[16];
    if (g_log && (!logged[idx] || now - logged[idx] >= 300000)) {
        logged[idx] = now ? now : 1;
        fprintf(g_log, "# ring: P%u at %.1f min: x %d..%d, y %d..%d, x+y %d..%d, x-y %d..%d around %d buildings and %d mine workers, %d sides moved out, %d gates (%d inner)\n",
                idx, now / 60000.0, rb[0], rb[1], rb[2], rb[3], rb[4], rb[5], rb[6], rb[7], held, mines, e->grows, e->ngate, e->ninner);
    }
}
// the town ring's tower spots, 2 tiles inside: beside its gates, at its corners, then along it (after its tiles are
// in e->rx / e->ry)
internal void ring_slots(struct encl *e, ivec2 b, unsigned hc, int n) {
    static int16_t cx[ENCL_RING + 64], cy[ENCL_RING + 64];
    int nc = 0;
    for (int g = 0; g < e->ngate; g++) {
        int j = 0;
        while (j < 8 && k_ring_dir[j] != e->gate[g].dir) j++;
        for (int sg = -1; j < 8 && sg <= 1; sg += 2) { cx[nc] = (int16_t)(e->gate[g].x + sg * 4 * k_ring_step[j][0]); cy[nc++] = (int16_t)(e->gate[g].y + sg * 4 * k_ring_step[j][1]); }
    }
    for (int i = 0; i < e->nring; i++) if (ring_sides(e->rb, e->rx[i], e->ry[i]) >= 2) { cx[nc] = e->rx[i]; cy[nc++] = e->ry[i]; }
    for (int i = 0; i < e->nring && nc < ENCL_RING + 64; i++) { cx[nc] = e->rx[i]; cy[nc++] = e->ry[i]; }
    for (int i = 0; i < nc && e->nslot < 96; i++) {
        int sx = 0, sy = 0, ok = 0;
        for (int d = 2; d <= 3 && !ok; d++) {
            sx = cx[i] + d * (b.x > cx[i] ? 1 : b.x < cx[i] ? -1 : 0); sy = cy[i] + d * (b.y > cy[i] ? 1 : b.y < cy[i] ? -1 : 0);
            ok = ring_in(e->rb, sx, sy) && !ring_border(e->rb, sx, sy) && encl_land(iv2(sx, sy), hc, n) && !encl_walled(sx, sy);
        }
        for (int j = 0; j < e->nslot && ok; j++) ok = abs(e->sx[j] - sx) >= 6 || abs(e->sy[j] - sy) >= 6;
        if (ok) { e->sx[e->nslot] = (int16_t)sx; e->sy[e->nslot++] = (int16_t)sy; }
    }
}
// packing.c: footprint (x, y, s) on the town ring or a tile beside it
internal int ring_blocks(ee_player *pl, int x, int y, int s) {
    struct encl *e = pl ? &g_encs[(unsigned)player_get_index(pl) & 15] : NULL;
    if (!e || e->pl != pl || !e->town || !e->nring) return 0;
    for (int ty = y; ty < y + s; ty++)
        for (int tx = x; tx < x + s; tx++) {
            int bx = tx - e->box_x, by = ty - e->box_y;
            if (bx >= 0 && by >= 0 && bx < RING_BOX && by < RING_BOX && (e->box[(by * RING_BOX + bx) >> 3] >> (bx & 7) & 1)) return 1;
        }
    return 0;
}
internal void encl_plan(ee_player *pl, unsigned idx, struct def_plan *p, uint32_t now) {
    struct encl *e = &g_encs[idx];
    if (e->pl == pl && now >= e->t && now - e->t < 60000) return;
    if (e->pl != pl) memset(e, 0, sizeof *e);
    e->pl = pl; e->t = now ? now : 1;
    e->nring = e->nslot = e->ngate = 0;
    ee_map *map = game_map();
    int n = map ? (int)map_get_size(map) : 0;
    ivec2 b = iv2(p->bx, p->by);
    unsigned hc = n > 16 && p->pl == pl ? area_id(b, 2) : 0;
    if (!hc) return;
    static struct encl_scan s;
    memset(&s, 0, sizeof s);
    s.pl = pl; s.hc = hc;
    grid_each(encl_scan_unit, &s);
    static struct encl_key k[ENCL_RING];
    int nk = 0;
    e->island = 0; e->tiles = 0;
    if (!s.enemy) {   // the continent, flood-filled over its land
        static int16_t qx[ENCL_ISLAND], qy[ENCL_ISLAND];
        uint8_t *mark = calloc((size_t)n * (size_t)n, 1);
        int qn = 0, over = 0;
        if (mark) {
            mark[b.y * n + b.x] = 1; qx[qn] = (int16_t)b.x; qy[qn++] = (int16_t)b.y;
            static const int nb4[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
            for (int h = 0; h < qn && !over; h++)
                for (int d = 0; d < 4 && !over; d++) {
                    ivec2 q = iv2(qx[h] + nb4[d][0], qy[h] + nb4[d][1]);
                    if (q.x < 0 || q.y < 0 || q.x >= n || q.y >= n || mark[q.y * n + q.x]) continue;
                    mark[q.y * n + q.x] = 1;
                    // a wall piece takes its tile out of the continent: kept, or the walled coast left the ring
                    if ((area_id(q, 2) != hc || area_id(q, 1)) && !encl_walled(q.x, q.y)) continue;
                    if (qn >= ENCL_ISLAND) { over = 1; break; }
                    qx[qn] = (int16_t)q.x; qy[qn++] = (int16_t)q.y;
                }
            free(mark);
            e->tiles = qn;
            e->island = !over;
        }
        // SAFE SPACE (enclose_safe_share; the player 2026-11-07: walling a large island is too much, there is safe space
        // inland - "the question the AI has to ask is if it has enough safe space to build without walling in"): its land
        // tiles beyond ENCL_SAFE tiles of the sea (no ship reaches them), by a walk inward from the coast, and the mines
        // (stone, gold, iron) on them. With at least enclose_safe_tiles of them the coast isn't walled (2026-11-07: Small
        // Islands 40 and 152 of 472 / 437, Large Islands 297 and 378 of 983 / 900)
        e->safe = e->safe_mines = e->mines = 0;
        uint8_t *dist = e->island && qn > 0 ? calloc((size_t)n * (size_t)n, 1) : NULL;
        if (dist) {
            static int16_t wx[ENCL_ISLAND], wy[ENCL_ISLAND];
            int wn = 0;
            for (int h = 0; h < qn; h++) dist[qy[h] * n + qx[h]] = 0xFF;   // the land, not reached yet
            for (int h = 0; h < qn; h++)
                if (encl_coast(iv2(qx[h], qy[h]))) { dist[qy[h] * n + qx[h]] = 1; wx[wn] = qx[h]; wy[wn++] = qy[h]; }
            for (int h = 0; h < wn; h++)
                for (int dx = -1; dx <= 1; dx++)
                    for (int dy = -1; dy <= 1; dy++) {
                        int x = wx[h] + dx, y = wy[h] + dy;
                        if (x < 0 || y < 0 || x >= n || y >= n || dist[y * n + x] != 0xFF || wn >= ENCL_ISLAND) continue;
                        dist[y * n + x] = (uint8_t)(dist[wy[h] * n + wx[h]] + 1 < 0xFE ? dist[wy[h] * n + wx[h]] + 1 : 0xFE);
                        wx[wn] = (int16_t)x; wy[wn++] = (int16_t)y;
                    }
            for (int h = 0; h < qn; h++) e->safe += dist[qy[h] * n + qx[h]] > ENCL_SAFE;
            ee_zone **zb = player_get_zones_begin(pl), **ze = player_get_zones_end(pl);   // the mines it knows
            for (int z = 0; zb && zb + z < ze; z++)
                for (int t = 4; zb[z] && t < 7; t++) {
                    ee_unit *ms = zone_site(zb[z], t);
                    int x = ms ? unit_get_x(ms) : -1, y = ms ? unit_get_y(ms) : -1;
                    if (x < 0 || y < 0 || x >= n || y >= n || !dist[y * n + x]) continue;
                    e->mines++;
                    e->safe_mines += dist[y * n + x] > ENCL_SAFE;
                }
            free(dist);
        }
        e->sheltered = e->island && g_encl_safe > 0 && e->safe >= g_encl_safe;
        if (e->sheltered) e->island = 0, nk = 0;   // no coast ring: enough room out of the ships' reach
        if (!g_encl) e->island = 0;   // base_ring alone: its land is a town's like any other
        for (int h = 0; e->island && h < qn && nk < ENCL_RING; h++) {   // the coast
            ivec2 q = iv2(qx[h], qy[h]);
            if (encl_land(q, hc, n) && encl_coast(q)) { k[nk].x = (int16_t)q.x; k[nk++].y = (int16_t)q.y; }
        }
        for (int d = 0; e->island && g_encl_docks && d < s.nd && d < 4; d++) {   // the dock pockets' walls and gates
            ivec2 o = s.d[d];
            for (int dx = -ENCL_POCKET; dx <= ENCL_POCKET; dx++)
                for (int dy = -ENCL_POCKET; dy <= ENCL_POCKET && nk < ENCL_RING; dy++) {
                    if (abs(dx) != ENCL_POCKET && abs(dy) != ENCL_POCKET) continue;
                    ivec2 q = iv2(o.x + dx, o.y + dy);
                    int dup = 0;
                    for (int j = 0; j < nk && !dup; j++) dup = k[j].x == q.x && k[j].y == q.y;
                    if (!dup && encl_land(q, hc, n)) { k[nk].x = (int16_t)q.x; k[nk++].y = (int16_t)q.y; }
                }
            int gx = b.x - o.x, gy = b.y - o.y;   // the side towards the town; gate_fits' dir along it
            ivec2 g = abs(gx) >= abs(gy) ? iv2(o.x + (gx > 0 ? ENCL_POCKET : -ENCL_POCKET), o.y) : iv2(o.x, o.y + (gy > 0 ? ENCL_POCKET : -ENCL_POCKET));
            if (!encl_land(g, hc, n)) continue;
            e->gate[e->ngate].x = (int16_t)g.x; e->gate[e->ngate].y = (int16_t)g.y;
            e->gate[e->ngate].dir = (uint8_t)(abs(gx) >= abs(gy) ? 2 : 1); e->gate[e->ngate++].tries = 0;
        }
    }
    e->town = !e->island && PV(idx, g_v_ring, g_ring);   // the town ring (base_ring): where no coast ring is built
    if (e->town) ring_plan(pl, idx, e, &s, b, hc, n, k, &nk, now);
    int m = 0;   // open at the docks; the rest by distance to the nearest own building
    for (int i = 0; i < nk; i++) {
        int open = 0, best = 0x7FFFFFFF;
        int wide = e->island && g_encl_docks ? ENCL_POCKET - 1 : 2;
        for (int d = 0; d < s.nd && !open; d++) open = abs(s.d[d].x - k[i].x) <= wide && abs(s.d[d].y - k[i].y) <= wide;
        if (open) continue;
        for (int j = 0; j < s.nb; j++) {
            int d2 = ivec2_dist2(s.b[j], iv2(k[i].x, k[i].y));
            if (d2 < best) best = d2;
        }
        k[m] = k[i]; k[m++].key = best;
    }
    qsort(k, (size_t)m, sizeof k[0], encl_key_cmp);
    int walled = 0, gap = e->island ? 5 : 6, in = e->island ? 1 : 2;
    for (int i = 0; i < m; i++) {
        e->rx[i] = k[i].x; e->ry[i] = k[i].y;
        walled += encl_walled(k[i].x, k[i].y);
        if (e->nslot >= 96 || e->town) continue;   // the town ring's spots: ring_slots, below
        // a tower spot: inward towards the base point (on the coast 1 or 2 tiles, the first off the ring - asking for a
        // tile without water beside it left 1-2 spots on a small island's 40-tile coast)
        int sx = 0, sy = 0, ok = 0;
        for (int d = in; d <= in + 1 && !ok; d++) {
            sx = k[i].x + d * (b.x > k[i].x ? 1 : b.x < k[i].x ? -1 : 0); sy = k[i].y + d * (b.y > k[i].y ? 1 : b.y < k[i].y ? -1 : 0);
            ok = encl_land(iv2(sx, sy), hc, n) && !encl_walled(sx, sy);
            for (int j = 0; j < m && ok; j++) ok = k[j].x != sx || k[j].y != sy;
        }
        for (int j = 0; j < e->nslot && ok; j++) ok = abs(e->sx[j] - sx) >= gap || abs(e->sy[j] - sy) >= gap;
        if (ok) { e->sx[e->nslot] = (int16_t)sx; e->sy[e->nslot++] = (int16_t)sy; }
    }
    e->nring = m; e->walled = walled;
    if (e->town) ring_slots(e, b, hc, n);
    {
        static const enum ut_name wl[] = { UT_WALL_LINE };
        int wt = 0;
        for (int i = 5; i >= 0 && !wt; i--) if (ut_has(g_ut[wl[i]]) && can_build(pl, g_ut[wl[i]])) wt = g_ut[wl[i]];
        e->stone = wt ? (m - walled) * unit_cost(pl, wt, 4) : 0;
    }
    if (g_log && s.nfoe) {   // DIAG: enemy soldiers on the island, the open ring tiles
        char buf[512];
        int len = snprintf(buf, sizeof buf, "# enclose diag: P%u at %.1f min: %d enemy soldiers on the continent (", idx, now / 60000.0, s.nfoe);
        for (int i = 0; i < s.nfoe && i < 8 && len < 400; i++) len += snprintf(buf + len, sizeof buf - (size_t)len, "%s%d,%d", i ? " " : "", s.foe[i].x, s.foe[i].y);
        len += snprintf(buf + len, sizeof buf - (size_t)len, "), open ring:");
        for (int i = 0, shown = 0; i < m && shown < 16 && len < 480; i++)
            if (!encl_walled(k[i].x, k[i].y)) { len += snprintf(buf + len, sizeof buf - (size_t)len, " %d,%d", k[i].x, k[i].y); shown++; }
        fprintf(g_log, "%s\n", buf);
    }
    if (g_log && (!e->log_t || now - e->log_t >= 300000)) {
        e->log_t = now ? now : 1;
        // DIAG: what the open ring tiles hold, and what the island's land borders on
        static const enum ut_name wline[] = { UT_WALL_LINE };
        int wtype = 0, place = 0, obj = 0, bare = 0, nb_ocean = 0, nb_none = 0, nb_other = 0;
        for (int i = 5; i >= 0 && !wtype; i--) if (ut_has(g_ut[wline[i]]) && can_build(pl, g_ut[wline[i]])) wtype = g_ut[wline[i]];
        for (int i = 0; i < m && wtype; i++) {
            int x = e->rx[i], y = e->ry[i];
            if (encl_walled(x, y)) continue;
            if (can_place(pl, wtype, iv2(x, y))) { place++; continue; }
            ee_tile *tile = map_get_tiles(map)[y * n + x];
            int any = 0;
            for (void *node = tile ? tile_get_objects(tile) : NULL; node; node = tilenode_get_next(node)) {
                ee_unit *o = tilenode_get_object(node);
                ee_unit_type *td = o ? unit_get_type_data(o) : NULL;
                if (td && !any) {
                    any = 1;
                    if (obj < 6) fprintf(g_log, "# enclose diag: P%u ring %d,%d holds type %d class %d\n", idx, x, y, unit_type_get_id(td), unit_type_get_class_id(td));
                }
            }
            if (any) obj++; else bare++;
        }
        for (int dx = -40; e->island && dx <= 40; dx++)
            for (int dy = -40; dy <= 40; dy++) {
                ivec2 q = iv2(b.x + dx, b.y + dy);
                if (q.x < 1 || q.y < 1 || q.x >= n - 1 || q.y >= n - 1 || area_id(q, 2) != hc || area_id(q, 1)) continue;
                static const int nb4[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
                for (int d = 0; d < 4; d++) {
                    ivec2 r2 = iv2(q.x + nb4[d][0], q.y + nb4[d][1]);
                    unsigned c = area_id(r2, 2), o = area_id(r2, 1);
                    if (c == hc && !o) continue;
                    if (o) nb_ocean++; else if (!c) nb_none++; else nb_other++;
                }
            }
        fprintf(g_log, "# enclose diag: P%u open ring tiles: %d placeable, %d hold an object, %d bare but refused; the land's edges: %d ocean, %d no area, %d another land\n",
                idx, place, obj, bare, nb_ocean, nb_none, nb_other);
        fprintf(g_log, "# enclose: P%u at %.1f min: %s (continent %d tiles%s), ring %d tiles, %d walled, tower spots %d, gates %d, docks %d; runs %u, pieces %u, gates built %u; skipped: pending %u, tower fire %u, enemies near %u, no stone %u, few citizens %u, would shut in %u; safe space %d of %d tiles, mines %d of %d%s\n",
                idx, now / 60000.0, e->sheltered ? "island, safe room enough" : e->island ? "island" : "town ring", e->tiles,
                e->island || e->sheltered ? "" : s.enemy ? ", an enemy on it" : " or more",
                m, walled, e->nslot, e->ngate, s.nd, e->runs, e->pieces, e->gates, e->skip_pend, e->skip_fire, e->skip_raid, e->skip_stone, e->skip_econ, e->skip_shut,
                e->safe, e->tiles, e->safe_mines, e->mines, e->sheltered ? " - no ring" : "");
        e->skip_pend = e->skip_fire = e->skip_raid = e->skip_stone = e->skip_econ = e->skip_shut = 0;
    }
}

// Slot i of the plan in build order, 0 when there is none.
internal int def_slot(struct def_plan *p, int i, int *x, int *y) {
    static const int first[3] = { 0, -1, 1 }, more[2] = { -2, 2 };
    struct encl *e = &g_encs[(p - g_plan) & 15];
    if (encl_on((unsigned)(p - g_plan) & 15) && e->pl == p->pl && e->nslot && e->ready) {
        if (i < e->nslot) { *x = e->sx[i]; *y = e->sy[i]; return 1; }
        i -= e->nslot;
        if (i < p->nout) { *x = p->out[i][0]; *y = p->out[i][1]; return 1; }
        return 0;
    }
    for (int k = 0; k < p->nf; k++, i -= 3)
        if (i < 3) { *x = p->f[k].fx + (int)lroundf(-p->f[k].dy * 3.0f * first[i]); *y = p->f[k].fy + (int)lroundf(p->f[k].dx * 3.0f * first[i]); return 1; }
    if (i < p->nout) { *x = p->out[i][0]; *y = p->out[i][1]; return 1; }
    i -= p->nout;
    for (int k = 0; k < p->nf; k++, i -= 2)
        if (i < 2) { *x = p->f[k].fx + (int)lroundf(-p->f[k].dy * 3.0f * more[i]); *y = p->f[k].fy + (int)lroundf(p->f[k].dx * 3.0f * more[i]); return 1; }
    return 0;
}

// From the build manager's placement (econ.c hook_place): a position for a tower request without one.
// TOWERS BY THE COAST'S WORK ([ai_base] coast_towers, default 0 until measured; the player 2026-11-07: "before walling
// towers are the first priority, they can defend resources inland and sink ships on the coast"). On an island map
// (navy_islands) the towers go first where the citizens work within a ship's reach of the sea: the sites of the
// gatherers (resource lists 2-6) with ocean within CT_SEA tiles, in CT_CELL-tile cells; the cell with the most of them
// that no own tower (or a tower ordered in the last 90 s) covers within CT_COVER gets one, CT_IN tiles in from the shore
// on the way from the cell's centre to the nearest sea - it covers the workers and the water the ships come by. The
// defence layout's slots after those. Ships alone sent most of the citizens back on the islands (docs/testing.md).
#define CT_SEA 9
#define CT_CELL 6
#define CT_COVER 6
#define CT_IN 2
internal int g_coast_towers = 0;
internal int navy_islands(unsigned idx);   // navy.c: the map known, every enemy across the water
internal int coast_spot(ee_player *pl, unsigned idx, struct def_plan *p, struct def_scan *s, int type, uint32_t now, int *ox, int *oy) {
    void *cm = player_get_civ_mgr(pl);
    ee_resource_mgr *rm = cm ? civ_mgr_get_resource_mgr(cm) : NULL;
    if (!rm) return 0;
    static struct { int x, y, n; } c[64];
    int nc = 0;
    for (int r = 2; r < 7; r++) {
        uint8_t *list = resource_mgr_list_at(rm, r);
        for (unsigned k = 0, first = deque_front(list), cnt = deque_count(list); k < cnt && k < 1024; k++) {
            ee_unit *u = deque_at(list, first + k);
            ee_goal *goal = u && unit_get_hp(u) > 0 ? unit_get_goal(u) : NULL;
            ee_unit *site = goal && !strcmp(rtti_name(goal), "EEUGGather@@") ? goal_get_site(goal) : NULL;
            if (!site || unit_type_get_class_id(unit_get_type_data(u)) == CLS_NAVAL_CITIZEN) continue;   // not the fishing boats
            int sx = unit_get_x(site), sy = unit_get_y(site), j = 0;
            while (j < nc && (c[j].x / CT_CELL != sx / CT_CELL || c[j].y / CT_CELL != sy / CT_CELL)) j++;
            if (j == nc) { if (nc == 64) continue; c[nc].x = sx; c[nc].y = sy; c[nc++].n = 0; }
            c[j].n++;
        }
    }
    int n_cov = 0, n_in = 0, n_place = 0, workers = 0;
    for (int j = 0; j < nc; j++) workers += c[j].n;
    for (;;) {
        int best = -1;
        for (int j = 0; j < nc; j++) if (c[j].n > 0 && (best < 0 || c[j].n > c[best].n)) best = j;
        if (best < 0) {
            static uint32_t logged[16];
            if (g_log && (!logged[idx] || now - logged[idx] >= 120000)) {
                logged[idx] = now ? now : 1;
                fprintf(g_log, "# defence: P%u no coast tower spot: %d places of work (%d at work), %d covered, %d inland, %d without a free tile\n",
                        idx, nc, workers, n_cov, n_in, n_place);
            }
            return 0;
        }
        int cx = c[best].x, cy = c[best].y, at = c[best].n;
        c[best].n = 0;
        int covered = 0;   // towers by it, standing or ordered in the last 90 s: coast_towers of them
        for (int t = 0; t < s->towers; t++) covered += abs(s->tx[t] - cx) <= CT_COVER && abs(s->ty[t] - cy) <= CT_COVER;
        for (int t = 0; t < 8; t++)
            covered += p->pend[t].t && now - p->pend[t].t < 90000 && abs(p->pend[t].x - cx) <= CT_COVER && abs(p->pend[t].y - cy) <= CT_COVER;
        if (covered >= g_coast_towers) { n_cov++; continue; }
        int wx = 0, wy = 0, wd = 0x7FFFFFFF;   // the nearest sea
        for (int dx = -CT_SEA; dx <= CT_SEA; dx++)
            for (int dy = -CT_SEA; dy <= CT_SEA; dy++) {
                int d = dx * dx + dy * dy;
                if (d < wd && d <= CT_SEA * CT_SEA && map_area(iv2(cx + dx, cy + dy), 1)) { wd = d; wx = cx + dx; wy = cy + dy; }
            }
        if (wd == 0x7FFFFFFF) { n_in++; continue; }   // inland: no ship reaches it
        float l = sqrtf((float)wd), f = l > (float)CT_IN ? (l - (float)CT_IN) / l : 0.0f;
        int tx = cx + (int)lroundf((float)(wx - cx) * f), ty = cy + (int)lroundf((float)(wy - cy) * f);
        for (int r = 0; r <= 2; r++)
            for (int ax = -r; ax <= r; ax++)
                for (int ay = -r; ay <= r; ay++) {
                    if ((abs(ax) != r && abs(ay) != r) || raiders_near(pl, iv2(tx + ax, ty + ay), 10) ||
                        !can_place(pl, type, iv2(tx + ax, ty + ay))) continue;
                    *ox = tx + ax; *oy = ty + ay;
                    int o = 0;
                    for (int t = 1; t < 8; t++) if (p->pend[t].t < p->pend[o].t) o = t;
                    p->pend[o].x = *ox; p->pend[o].y = *oy; p->pend[o].t = now ? now : 1;
                    g_def_towers[idx]++;
                    if (g_log) fprintf(g_log, "# defence: P%u coast tower %d at %d,%d (%d at work by %d,%d, the sea %d,%d)\n",
                                       idx, type, *ox, *oy, at, cx, cy, wx, wy);
                    return 1;
                }
        n_place++;
    }
}
internal int defence_spot(ee_player *pl, int type, int *ox, int *oy) {
    if (!g_def || !pl || !is_tower_type(type)) return 0;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    if (g_def_off[idx]) return 0;
    if (early_core_spot(pl, type, ox, oy)) return 1;   // early.c: the town's first towers by the Town Center
    uint32_t now = game_time();
    def_refresh(pl, idx, now);
    struct def_plan *p = &g_plan[idx];
    struct def_scan *s = &g_scan;
    if (s->pl != pl) { p->t = now - 60000; def_refresh(pl, idx, now); }   // the scan belongs to another AI
    if (encl_on(idx)) encl_plan(pl, idx, p, now);
    int x, y;
    threat_refresh(pl, now);   // slots inside known enemy defences would cost the builders (attack.c)
    if (g_coast_towers && navy_islands(idx) && coast_spot(pl, idx, p, s, type, now, ox, oy)) return 1;
    for (int i = 0; def_slot(p, i, &x, &y); i++) {
        int taken = 0;
        for (int t = 0; t < s->towers && !taken; t++) taken = abs(s->tx[t] - x) <= 2 && abs(s->ty[t] - y) <= 2;
        for (int t = 0; t < 8 && !taken; t++)
            taken = p->pend[t].t && now - p->pend[t].t < 90000 && abs(p->pend[t].x - x) <= 2 && abs(p->pend[t].y - y) <= 2;
        if (taken) continue;
        for (int r = 0; r <= 2; r++)
            for (int ax = -r; ax <= r; ax++)
                for (int ay = -r; ay <= r; ay++) {
                    if ((abs(ax) != r && abs(ay) != r) || route_covered(pl, p->bx, p->by, x + ax, y + ay) ||
                        raiders_near(pl, iv2(x + ax, y + ay), 10) ||   // an enemy army there: its builders would walk into it
                        !can_place(pl, type, iv2(x + ax, y + ay))) continue;   // the builders' way too
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
internal int on_segment(int x, int y, const int *a, const int *b) {
    for (int j = 0; j <= 10; j++)
        if (abs(x - (a[0] + (b[0] - a[0]) * j / 10)) <= 1 && abs(y - (a[1] + (b[1] - a[1]) * j / 10)) <= 1) return 1;
    return 0;
}

// build_gate's five tiles at (x, y) along dir: each with exactly one object of the type flag it takes (+0x612), that
// one a finished wall piece of pl's - build_gate checks nothing, and reads past its list on fewer than five
internal int gate_fits(ee_player *pl, int x, int y, int dir) {
    static const int step[5][2] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { -1, 1 }, { 1, 1 } };
    ee_map *map = game_map();
    int n = map ? (int)map_get_size(map) : 0;
    if (dir < 1 || dir > 4) return 0;
    for (int i = -2; i <= 2; i++) {
        int tx = x + step[dir][0] * i, ty = y + step[dir][1] * i, cnt = 0, ok = 0;
        if (tx < 0 || ty < 0 || tx >= n || ty >= n) return 0;
        ee_tile *tile = map_get_tiles(map)[ty * n + tx];
        for (void *node = tile ? tile_get_objects(tile) : NULL; node; node = tilenode_get_next(node)) {
            ee_unit *o = tilenode_get_object(node);
            ee_unit_type *td = o ? unit_get_type_data(o) : NULL;
            if (!td || !unit_type_get_is_wall(td)) continue;
            ee_unit *bs = unit_get_physics(o);
            int t = unit_type_get_id(td);
            cnt++;
            ok = unit_get_owner(o) == pl && ut_role(t, UTR_WALL) && !ut_role(t, UTR_GATE) && unit_get_hp(o) > 0 && bs && unit_get_finished(bs);
        }
        if (cnt != 1 || !ok) return 0;
    }
    return 1;
}

// econ.c: the stone the ring still needs - its open walls, and once there is a ring 200 for repairs (0: enclose off).
// Without it nobody mined stone on a finished ring (the walls owed nothing, few towers asked) and the pieces the
// enemy broke from minute 35 stayed open: its soldiers, stranded on the beach since minute 20, walked in.
internal int sf_stone(ee_player *pl);   // site forts: the stone the fort being built still needs
internal int encl_stone(ee_player *pl) {   // with the site forts' (econ.c staffs stone for it)
    struct encl *e = pl ? &g_encs[(unsigned)player_get_index(pl) & 15] : NULL;
    return (e && encl_on((unsigned)player_get_index(pl) & 15) && e->pl == pl && e->nring && e->ready ? e->stone + 200 : 0) + sf_stone(pl);   // the ring not before its walls start
}
internal int encl_owed(ee_player *pl) {   // the open walls' stone alone: the extra towers wait for it
    struct encl *e = pl ? &g_encs[(unsigned)player_get_index(pl) & 15] : NULL;
    return e && encl_on((unsigned)player_get_index(pl) & 15) && e->pl == pl ? e->stone : 0;
}
internal int encl_ring_has(struct encl *e, int x, int y) {
    for (int i = 0; i < e->nring; i++) if (e->rx[i] == x && e->ry[i] == y) return 1;
    return 0;
}
internal int encl_pending(struct encl *e, int x, int y, uint32_t now) {   // on a run ordered in the last 4 minutes
    for (int i = 0; i < 24; i++) {
        if (!e->pend[i].t || now - e->pend[i].t >= 240000) continue;
        int ax = e->pend[i].ax, ay = e->pend[i].ay, dx = e->pend[i].bx - ax, dy = e->pend[i].by - ay;
        int len = abs(dx) > abs(dy) ? abs(dx) : abs(dy), sx = dx > 0 ? 1 : dx < 0 ? -1 : 0, sy = dy > 0 ? 1 : dy < 0 ? -1 : 0;
        for (int j = 0; j <= len; j++) if (ax + sx * j == x && ay + sy * j == y) return 1;
    }
    return 0;
}
internal int encl_built(int x, int y) {   // a finished wall or gate piece on the tile
    ee_map *map = game_map();
    int n = map ? (int)map_get_size(map) : 0;
    if (x < 0 || y < 0 || x >= n || y >= n) return 0;
    ee_tile *tile = map_get_tiles(map)[y * n + x];
    for (void *node = tile ? tile_get_objects(tile) : NULL; node; node = tilenode_get_next(node)) {
        ee_unit *o = tilenode_get_object(node);
        ee_unit_type *td = o ? unit_get_type_data(o) : NULL;
        ee_unit *bs = o ? unit_get_physics(o) : NULL;
        if (td && unit_type_get_is_wall(td) && unit_get_hp(o) > 0 && bs && unit_get_finished(bs)) return 1;
    }
    return 0;
}
// enclose_in_flight: the runs ordered in the last 4 minutes with a piece not standing finished yet
internal int encl_in_flight(struct encl *e, uint32_t now) {
    int n = 0;
    for (int i = 0; i < 24; i++) {
        if (!e->pend[i].t || now - e->pend[i].t >= 240000) continue;
        int ax = e->pend[i].ax, ay = e->pend[i].ay, dx = e->pend[i].bx - ax, dy = e->pend[i].by - ay;
        int len = abs(dx) > abs(dy) ? abs(dx) : abs(dy), sx = dx > 0 ? 1 : dx < 0 ? -1 : 0, sy = dy > 0 ? 1 : dy < 0 ? -1 : 0;
        for (int j = 0; j <= len; j++) if (!encl_built(ax + sx * j, ay + sy * j)) { n++; break; }
    }
    return n;
}
internal int raiders_scan(ee_player *pl, ivec2 p, int r0, int *kinds);   // attack.c
internal int encl_free(ee_player *pl, struct encl *e, int wtype, int x, int y, uint32_t now) {
    for (int i = 0; i < e->nopen; i++) if (e->open_x[i] == x && e->open_y[i] == y) return 0;
    return encl_ring_has(e, x, y) && !encl_walled(x, y) && !encl_pending(e, x, y, now) && can_place(pl, wtype, iv2(x, y));
}
// NOTHING SHUT IN (the player 2026-11-13, with base_ring on: "the ai now has a tendency to close sections off, box
// resources or its citizens in, it should check that gates can actually be placed in a way to keep everything
// accessible"). Each tick a walk from the base (four ways, as the reach map's corners allow; gates open, the runs
// ordered and not built yet shut) over the ring's box and RF_PAD round it: a run is ordered only if every tile that
// walk reached it still reaches, but for the run's own - else its first piece alone, else the next tile. A planned
// gate's five pieces go first (the gate is laid over them finished, so it stands before its side closes) and are
// exempt while a gate can be built. What is shut in already - a citizen or a building of its own the walk over what
// stands (the ordered runs open) doesn't reach, where its own walls are part of what closes it - gets a gate in a
// straight run of five of its finished pieces between it and the base's side; where none fits (a short stretch
// between trees, cliffs and a Town Center laid after it: a woodcutter shut in by its diagonal of three) the piece
// between them is taken down, and its tile is never walled again (built again, it shut the pocket again: 4 times a game).
#define RF_W 192
#define RF_PAD 16
internal struct rflood {
    int x0, y0, w, h, sx, sy, n0;
    uint8_t walk[RF_W * RF_W], base[RF_W * RF_W], seen[RF_W * RF_W];   // walk: the ordered runs shut too
    uint8_t real[RF_W * RF_W], base_real[RF_W * RF_W];   // what stands: the pockets' look
    uint8_t *on;   // the tiles rf_walk walks
    int16_t q[RF_W * RF_W][2];
} g_shut;
internal const int8_t k_gate_step[5][2] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { -1, 1 }, { 1, 1 } };   // gate_fits' dirs
internal int rf_in(int x, int y) { return x >= g_shut.x0 && y >= g_shut.y0 && x < g_shut.x0 + g_shut.w && y < g_shut.y0 + g_shut.h; }
internal int rf_at(int x, int y) { return (y - g_shut.y0) * RF_W + x - g_shut.x0; }
// the walk from (sx, sy) into seen with the run a-b shut (ax < 0: none); the tiles reached
internal int rf_walk(int sx, int sy, int ax, int ay, int bx, int by, uint8_t *seen) {
    uint8_t *walk = g_shut.on ? g_shut.on : g_shut.walk;
    int dx = bx - ax, dy = by - ay, len = abs(dx) > abs(dy) ? abs(dx) : abs(dy), ux = dx > 0 ? 1 : dx < 0 ? -1 : 0, uy = dy > 0 ? 1 : dy < 0 ? -1 : 0;
    static int16_t shut[16][2];
    int ns = 0;
    for (int j = 0; ax >= 0 && j <= len && ns < 16; j++) {
        int x = ax + ux * j, y = ay + uy * j;
        if (rf_in(x, y) && walk[rf_at(x, y)]) { walk[rf_at(x, y)] = 0; shut[ns][0] = (int16_t)x; shut[ns++][1] = (int16_t)y; }
    }
    memset(seen, 0, sizeof g_shut.seen);
    int head = 0, tail = 0, n = 0;
    if (rf_in(sx, sy) && walk[rf_at(sx, sy)]) { seen[rf_at(sx, sy)] = 1; g_shut.q[tail][0] = (int16_t)sx; g_shut.q[tail++][1] = (int16_t)sy; }
    static const int8_t nx4[4] = { 1, -1, 0, 0 }, ny4[4] = { 0, 0, 1, -1 };
    while (head < tail) {
        int cx = g_shut.q[head][0], cy = g_shut.q[head][1];
        head++; n++;
        for (int d = 0; d < 4; d++) {
            int x = cx + nx4[d], y = cy + ny4[d];
            if (!rf_in(x, y) || seen[rf_at(x, y)] || !walk[rf_at(x, y)]) continue;
            seen[rf_at(x, y)] = 1; g_shut.q[tail][0] = (int16_t)x; g_shut.q[tail++][1] = (int16_t)y;
        }
    }
    for (int j = 0; j < ns; j++) walk[rf_at(shut[j][0], shut[j][1])] = 1;
    return n;
}
// the box, its walkable tiles and the base's walk; 0: no room for it
internal int rf_setup(struct encl *e, ivec2 b, uint32_t now) {
    ee_map *map = game_map();
    int n = map ? (int)map_get_size(map) : 0, x0 = b.x, y0 = b.y, x1 = b.x, y1 = b.y;
    if (!n) return 0;
    for (int i = 0; i < e->nring; i++) {
        if (e->rx[i] < x0) x0 = e->rx[i];
        if (e->rx[i] > x1) x1 = e->rx[i];
        if (e->ry[i] < y0) y0 = e->ry[i];
        if (e->ry[i] > y1) y1 = e->ry[i];
    }
    x0 -= RF_PAD; y0 -= RF_PAD; x1 += RF_PAD; y1 += RF_PAD;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 >= n) x1 = n - 1;
    if (y1 >= n) y1 = n - 1;
    if (x1 - x0 + 1 > RF_W || y1 - y0 + 1 > RF_W) return 0;
    g_shut.x0 = x0; g_shut.y0 = y0; g_shut.w = x1 - x0 + 1; g_shut.h = y1 - y0 + 1;
    for (int y = y0; y <= y1; y++)
        for (int x = x0; x <= x1; x++) {
            g_shut.real[rf_at(x, y)] = (uint8_t)(rm_tile_ok(map, n, x, y) != 0);
            g_shut.walk[rf_at(x, y)] = (uint8_t)(g_shut.real[rf_at(x, y)] && !encl_pending(e, x, y, now));
        }
    g_shut.sx = -1;
    for (int r = 0; r <= 6 && g_shut.sx < 0; r++)   // the walkable tile nearest the base point
        for (int dx = -r; dx <= r && g_shut.sx < 0; dx++)
            for (int dy = -r; dy <= r; dy++)
                if ((abs(dx) == r || abs(dy) == r) && rf_in(b.x + dx, b.y + dy) && g_shut.walk[rf_at(b.x + dx, b.y + dy)]) { g_shut.sx = b.x + dx; g_shut.sy = b.y + dy; break; }
    if (g_shut.sx < 0) return 0;
    g_shut.on = NULL;
    g_shut.n0 = rf_walk(g_shut.sx, g_shut.sy, -1, 0, 0, 0, g_shut.base);
    g_shut.on = g_shut.real;
    rf_walk(g_shut.sx, g_shut.sy, -1, 0, 0, 0, g_shut.base_real);
    g_shut.on = NULL;
    return 1;
}
// the run a-b shuts nothing in: what the base's walk reached, it still reaches, but the run's own tiles
internal int rf_run_ok(int ax, int ay, int bx, int by) {
    int dx = bx - ax, dy = by - ay, len = abs(dx) > abs(dy) ? abs(dx) : abs(dy), ux = dx > 0 ? 1 : dx < 0 ? -1 : 0, uy = dy > 0 ? 1 : dy < 0 ? -1 : 0, own = 0;
    for (int j = 0; j <= len; j++) { int x = ax + ux * j, y = ay + uy * j; own += rf_in(x, y) && g_shut.base[rf_at(x, y)]; }
    return rf_walk(g_shut.sx, g_shut.sy, ax, ay, bx, by, g_shut.seen) >= g_shut.n0 - own;
}
internal void rf_ordered(int ax, int ay, int bx, int by) {   // a run ordered: shut from now on, the base's walk again
    int dx = bx - ax, dy = by - ay, len = abs(dx) > abs(dy) ? abs(dx) : abs(dy), ux = dx > 0 ? 1 : dx < 0 ? -1 : 0, uy = dy > 0 ? 1 : dy < 0 ? -1 : 0;
    for (int j = 0; j <= len; j++) { int x = ax + ux * j, y = ay + uy * j; if (rf_in(x, y)) g_shut.walk[rf_at(x, y)] = 0; }
    g_shut.n0 = rf_walk(g_shut.sx, g_shut.sy, -1, 0, 0, 0, g_shut.base);
}
internal int encl_gate_type(ee_player *pl) {   // the newest gate it can build, 0 none
    static const enum ut_name gline[] = { UT_GATE_LINE };   // oldest first
    for (int i = 5; i >= 0; i--) if (ut_has(g_ut[gline[i]]) && can_build(pl, g_ut[gline[i]])) return g_ut[gline[i]];
    return 0;
}
internal ee_unit *encl_own_piece(ee_player *pl, int x, int y) {   // a wall piece of pl's (not a gate) on the tile
    ee_map *map = game_map();
    int n = map ? (int)map_get_size(map) : 0;
    if (x < 0 || y < 0 || x >= n || y >= n) return NULL;
    ee_tile *tile = map_get_tiles(map)[y * n + x];
    for (void *node = tile ? tile_get_objects(tile) : NULL; node; node = tilenode_get_next(node)) {
        ee_unit *o = tilenode_get_object(node);
        ee_unit_type *td = o ? unit_get_type_data(o) : NULL;
        if (td && unit_get_owner(o) == pl && unit_type_get_is_wall(td) && !ut_role(unit_type_get_id(td), UTR_GATE) && unit_get_hp(o) > 0) return o;
    }
    return NULL;
}
// something of its own shut in already: a gate in a straight run of five of its finished pieces between that and the
// base's side - 1 when one was ordered
internal int rf_free_pocket(ee_player *pl, unsigned idx, struct encl *e, uint32_t now) {
    static uint32_t logged[16];
    int count = 0, px = -1, py = -1;
    for (ee_unit *u = player_get_units(pl); u && count < 8192 && px < 0; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0) continue;
        int t = unit_type_get_id(td), cls = unit_type_get_class_id(td), x = unit_get_x(u), y = unit_get_y(u);
        if (!rf_in(x, y) || is_wall(t)) continue;
        if (is_citizen_type(t)) {
            if (g_shut.real[rf_at(x, y)] && !g_shut.base_real[rf_at(x, y)]) { px = x; py = y; }
        } else if (cls == CLS_BUILDING || cls == CLS_FORTRESS) {   // a free tile round it, none the base reaches
            int s = unit_type_get_size(td), fx = -1, fy = -1, reached = 0;
            if (s < 1 || s > 8) continue;
            for (int ry = y - 1; ry <= y + s && !reached; ry++)
                for (int rx = x - 1; rx <= x + s && !reached; rx++) {
                    if ((rx >= x && rx < x + s && ry >= y && ry < y + s) || !rf_in(rx, ry) || !g_shut.real[rf_at(rx, ry)]) continue;
                    if (g_shut.base_real[rf_at(rx, ry)]) reached = 1; else if (fx < 0) { fx = rx; fy = ry; }
                }
            if (!reached && fx >= 0) { px = fx; py = fy; }
        }
    }
    if (px < 0) return 0;
    g_shut.on = g_shut.real;
    int size = rf_walk(px, py, -1, 0, 0, 0, g_shut.seen);   // the pocket
    g_shut.on = NULL;
    int gtype = encl_gate_type(pl), walls = 0;
    ee_unit *piece = NULL;   // else a piece of its own between the pocket and the base's side, taken down
    int qx = 0, qy = 0;
    static const int8_t nx4[4] = { 1, -1, 0, 0 }, ny4[4] = { 0, 0, 1, -1 };
    for (int y = g_shut.y0; y < g_shut.y0 + g_shut.h; y++)
        for (int x = g_shut.x0; x < g_shut.x0 + g_shut.w; x++) {
            int pocket = 0, base = 0;   // a wall tile with the pocket on one side and the base's side on the other
            for (int d = 0; d < 4; d++) {
                int ax = x + nx4[d], ay = y + ny4[d];
                if (!rf_in(ax, ay)) continue;
                pocket |= g_shut.seen[rf_at(ax, ay)];
                base |= g_shut.base_real[rf_at(ax, ay)];
            }
            if (!pocket || !encl_built(x, y)) continue;
            walls = 1;   // its own walls close it (not only buildings, cliffs or trees: not the ring's doing)
            if (!base) continue;
            if (!piece) piece = encl_own_piece(pl, x, y), qx = x, qy = y;
            if (!gtype) continue;
            for (int dir = 1; dir <= 4; dir++)
                for (int off = -2; off <= 2; off++) {
                    int cx = x + k_gate_step[dir][0] * off, cy = y + k_gate_step[dir][1] * off;
                    if (!gate_fits(pl, cx, cy, dir)) continue;
                    int tile[2] = { cx, cy };
                    build_gate(tile, player_get_index(pl), unit_type_get_tech(type_data(gtype)), dir);
                    if (g_log) fprintf(g_log, "# enclose: P%u gate %d at %d,%d frees a pocket of %d tiles (its unit at %d,%d)\n", idx, gtype, cx, cy, size, px, py);
                    return 1;
                }
        }
    if (piece) {   // no gate fits there (a corner, a short stretch between trees and buildings): the piece goes
        unit_destroy(piece, 0, 0, 0);
        if (e->nopen < 32) { e->open_x[e->nopen] = (int16_t)qx; e->open_y[e->nopen++] = (int16_t)qy; }   // built again it shut the pocket again (4 times a game)
        if (g_log) fprintf(g_log, "# enclose: P%u wall piece at %d,%d taken down: it shut in a pocket of %d tiles (its unit at %d,%d), no gate fits\n",
                           idx, qx, qy, size, px, py);
        return 1;
    }
    if (walls && g_log && (!logged[idx] || now - logged[idx] >= 300000)) {
        logged[idx] = now ? now : 1;
        fprintf(g_log, "# enclose: P%u a pocket of %d tiles at %d,%d shut in - none of its pieces borders the base's side\n", idx, size, px, py);
    }
    return 0;
}
// The ring's walls and gates (defence_tick): up to two runs a tick, the stone above a tower's 100 kept back.
internal void encl_walls(ee_player *pl, unsigned idx, struct def_plan *p, void *wm, const int *have_stock, uint32_t now) {
    struct encl *e = &g_encs[idx];
    int stock[7];   // what this tick's runs leave (the game takes it as they are built)
    memcpy(stock, have_stock, sizeof stock);
    encl_plan(pl, idx, p, now);
    if (e->pl != pl || !e->nring) { e->ready = 0; return; }
    // the economy first (2026-11-07, 40 long games against the cheating AI: the ring from the start had 10 of a Small
    // Islands start's 12 citizens building walls at 5 min, 19 gathering at 15 min against 33 - every game lost by
    // minute 39 on average): until enclose_citizens citizens the defence layout's towers and front walls as without
    // enclose (def_slot, defence_tick), then the ring - enclose_runs runs a tick
    void *cmr = player_get_civ_mgr(pl);
    ee_resource_mgr *rm = cmr ? civ_mgr_get_resource_mgr(cmr) : NULL;
    int cit = 0;
    for (int r = 0; rm && r < 7; r++) cit += (int)deque_count(resource_mgr_list_at(rm, r));
    e->ready = cit >= g_encl_from;
    if (!e->ready) { e->skip_econ++; return; }
    for (int g = 0; g < e->ngate; g++) {   // a gate, once the side's middle stands finished: the newest it can build
        if (e->gate[g].tries >= 20 || !gate_fits(pl, e->gate[g].x, e->gate[g].y, e->gate[g].dir)) continue;
        int gtype = encl_gate_type(pl);
        e->gate[g].tries++;
        if (!gtype) continue;
        int tile[2] = { e->gate[g].x, e->gate[g].y };
        build_gate(tile, player_get_index(pl), unit_type_get_tech(type_data(gtype)), e->gate[g].dir);
        e->gate[g].tries = 20;
        e->gates++;
        if (g_log) fprintf(g_log, "# enclose: P%u gate %d at %d,%d\n", idx, gtype, tile[0], tile[1]);
        return;
    }
    static const enum ut_name wline[] = { UT_WALL_LINE };   // oldest first
    int wtype = 0;
    for (int i = 5; i >= 0 && !wtype; i--)   // the newest it can build
        if (ut_has(g_ut[wline[i]]) && can_build(pl, g_ut[wline[i]])) wtype = g_ut[wline[i]];
    if (!wtype) return;
    threat_refresh(pl, now);
    // MP's way (the player 2026-11-07: "the wall is the meta in mp games, and the only way to have peace on an own island
    // from ships"): the coast ring never closed - at 30-50 min a third of a Small Islands ring still open, its runs
    // skipped for the ships off the coast. enclose_in_flight: a new run only while fewer than that many stand unfinished
    // (a few builders at a time, from the start); enclose_under_ships: on an island only enemy land units near stop a run
    // (a piece that stands stops the ships' fire)
    if (g_encl_flight > 0 && encl_in_flight(e, now) >= g_encl_flight) { e->skip_pend++; return; }
    if (!rf_setup(e, iv2(p->bx, p->by), now)) { e->skip_shut++; return; }   // nothing ordered that can't be checked
    if (rf_free_pocket(pl, idx, e, now)) return;
    int gtype = encl_gate_type(pl);
    for (int order = 0; order < g_encl_runs; order++) {
        int most = 12;   // the pieces the stock pays for
        for (int r = 2; r < 7; r++) {
            int c = unit_cost(pl, wtype, r), have = stock[r] - (r == 4 ? 100 : 0);
            if (c > 0 && have / c < most) most = have / c;
        }
        if (most < 1) { e->skip_stone++; return; }
        int ax = -1, ay = -1, bx = -1, by = -1;
        // the planned gates' pieces first (only those: a run along the gate's line within its five), then the ring
        for (int g = 0; g < e->ngate && ax < 0; g++) {
            int dir = e->gate[g].dir, ux = k_gate_step[dir][0], uy = k_gate_step[dir][1], f = -3, l = -3;
            if (dir < 1 || dir > 4 || !gtype) continue;
            for (int i = -2; i <= 2; i++) {
                int x = e->gate[g].x + ux * i, y = e->gate[g].y + uy * i;
                if (!encl_free(pl, e, wtype, x, y, now)) { if (f > -3) break; continue; }
                if (f == -3) f = i;
                l = i;
                if (l - f + 1 >= most) break;
            }
            if (f == -3 || route_covered(pl, p->bx, p->by, e->gate[g].x, e->gate[g].y) || raiders_near(pl, iv2(e->gate[g].x, e->gate[g].y), 10)) continue;
            ax = e->gate[g].x + ux * f; ay = e->gate[g].y + uy * f; bx = e->gate[g].x + ux * l; by = e->gate[g].y + uy * l;   // exempt: its gate opens it
        }
        static int16_t skip[64][2];
        int nskip = 0;
        for (int i = 0; i < e->nring && ax < 0; i++) {
            int x = e->rx[i], y = e->ry[i], skipped = 0;
            for (int k = 0; k < nskip && !skipped; k++) skipped = skip[k][0] == x && skip[k][1] == y;
            if (skipped) continue;
            if (!encl_free(pl, e, wtype, x, y, now)) { e->skip_pend += !encl_walled(x, y) && encl_pending(e, x, y, now); continue; }
            if (route_covered(pl, p->bx, p->by, x, y)) { e->skip_fire++; continue; }   // the builders would walk into enemy fire
            int foes = 0;
            if (g_encl_ships && e->island ? (raiders_scan(pl, iv2(x, y), 10, &foes), foes & 1) : raiders_near(pl, iv2(x, y), 10)) {
                e->skip_raid++;   // or an enemy army, ships off the coast (enclose_under_ships on an island: land units only)
                continue;
            }
            static const int axis[4][2] = { { 1, 0 }, { 0, 1 }, { 1, 1 }, { 1, -1 } };
            int best = 0;
            for (int a = 0; a < 4; a++) {
                int f = 0, bk = 0, dx = axis[a][0], dy = axis[a][1];
                while (f + 1 < most && encl_free(pl, e, wtype, x + dx * (f + 1), y + dy * (f + 1), now)) f++;
                while (f + bk + 1 < most && encl_free(pl, e, wtype, x - dx * (bk + 1), y - dy * (bk + 1), now)) bk++;
                if (f + bk + 1 > best) { best = f + bk + 1; ax = x - dx * bk; ay = y - dy * bk; bx = x + dx * f; by = y + dy * f; }
            }
            if (rf_run_ok(ax, ay, bx, by)) break;
            if (rf_run_ok(x, y, x, y)) { ax = bx = x; ay = by = y; break; }   // the run would shut something in: this piece alone
            e->skip_shut++;   // even that one would: the next tile
            if (nskip < 64) { skip[nskip][0] = (int16_t)x; skip[nskip++][1] = (int16_t)y; }
            ax = -1;
        }
        if (ax < 0) return;
        rf_ordered(ax, ay, bx, by);
        int a[2] = { ax, ay }, bb[2] = { bx, by }, len = abs(bx - ax) > abs(by - ay) ? abs(bx - ax) : abs(by - ay);
        build_wall(wm, a, bb, wtype);
        int o = 0;
        for (int i = 1; i < 24; i++) if (e->pend[i].t < e->pend[o].t) o = i;
        e->pend[o].ax = (int16_t)ax; e->pend[o].ay = (int16_t)ay; e->pend[o].bx = (int16_t)bx; e->pend[o].by = (int16_t)by;
        e->pend[o].t = now ? now : 1;
        e->runs++; e->pieces += (unsigned)len + 1;
        for (int r = 2; r < 7; r++) stock[r] -= (len + 1) * unit_cost(pl, wtype, r);
        if (g_log) fprintf(g_log, "# enclose: P%u wall %d from %d,%d to %d,%d (%d pieces; ring %d, %d walled)\n",
                           idx, wtype, ax, ay, bx, by, len + 1, e->nring, e->walled);
    }
}

// SITE FORTS ([ai_base] site_forts, forts per AI, default 0 until measured; the player 2026-11-07 with a picture: a mine
// in a small square, a tower at each corner, walls between them, one side closed by a building with a gap - "walling
// the whole island is hard ... could also be useful on land maps", decided by the place, not the map's kind). Every 20 s
// (defence_tick): while it has fewer forts than site_forts and none is being built, the mine (stone, gold, iron) the
// most of its citizens work - at least SF_MIN - gets one, sea within CT_SEA tiles (ships reach it) or citizens hurt by
// it in the last 2 minutes counting SF_EXPOSED more; none within 2 x SF_R of another fort. Its square SF_R tiles round
// the mine: the four corner towers first (one at a time through the build queue, as the own towers), then the walls of
// the three sides away from the base point, run by run where a piece can stand; the side towards the base stays open
// for the citizens, its corners' towers over it. Ships' fire stops at the walls (docs/engine-notes.md "Walls and
// fire"), raiders come in only by the open side.
#define SF_MAX 6
#define SF_R 4
#define SF_MIN 2
#define SF_EXPOSED 4
internal int g_site_forts = 0, g_shore_guns = 0, g_shore_walls = 0;   // [ai_base] site_forts, shore_guns, shore_walls (SHORE GUNS below)
internal struct sfort { int x, y, open, towers, tries; uint32_t t; } g_sf[16][SF_MAX];
internal int g_sf_n[16];
internal unsigned g_sf_towers[16], g_sf_runs[16];
internal void sf_corner(const struct sfort *f, int k, int *x, int *y) {   // 0 (-,-), 1 (+,-), 2 (+,+), 3 (-,+)
    *x = f->x + (k == 1 || k == 2 ? SF_R : -SF_R);
    *y = f->y + (k >= 2 ? SF_R : -SF_R);
}
internal int sf_pick(ee_player *pl, unsigned idx, uint32_t now) {   // a new fort's mine: 1 found
    void *cm = player_get_civ_mgr(pl);
    ee_resource_mgr *rm = cm ? civ_mgr_get_resource_mgr(cm) : NULL;
    void *bmv = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    ee_build_mgr *bm = bmv;
    if (!rm || !bm || build_mgr_get_base_x(bm) == -10000) return 0;
    static struct { ee_unit *s; int n; } c[64];
    int nc = 0;
    for (int r = 4; r < 7; r++) {
        uint8_t *list = resource_mgr_list_at(rm, r);
        for (unsigned k = 0, first = deque_front(list), cnt = deque_count(list); k < cnt && k < 1024; k++) {
            ee_unit *u = deque_at(list, first + k);
            ee_goal *goal = u && unit_get_hp(u) > 0 ? unit_get_goal(u) : NULL;
            ee_unit *site = goal && !strcmp(rtti_name(goal), "EEUGGather@@") ? goal_get_site(goal) : NULL;
            if (!site) continue;
            int j = 0;
            while (j < nc && c[j].s != site) j++;
            if (j == nc) { if (nc == 64) continue; c[nc].s = site; c[nc++].n = 0; }
            c[j].n++;
        }
    }
    int best = -1, bscore = 0, bx = 0, by = 0, bsea = 0, bhurt = 0;
    for (int j = 0; j < nc; j++) {
        if (c[j].n < SF_MIN) continue;
        ee_unit_type *td = unit_get_type_data(c[j].s);
        int sz = td ? unit_type_get_size(td) : 0, x = unit_get_x(c[j].s) + (sz > 1 && sz < 8 ? sz / 2 : 0), y = unit_get_y(c[j].s) + (sz > 1 && sz < 8 ? sz / 2 : 0);
        int taken = 0;
        for (int f = 0; f < g_sf_n[idx] && !taken; f++) taken = abs(g_sf[idx][f].x - x) < 2 * SF_R && abs(g_sf[idx][f].y - y) < 2 * SF_R;
        if (taken) continue;
        int sea = 0;
        for (int dx = -CT_SEA; dx <= CT_SEA && !sea; dx++)
            for (int dy = -CT_SEA; dy <= CT_SEA && !sea; dy++) sea = dx * dx + dy * dy <= CT_SEA * CT_SEA && map_area(iv2(x + dx, y + dy), 1);
        int hurt = hurt_near(idx, x, y) > 0, score = c[j].n + (sea || hurt ? SF_EXPOSED : 0);
        if (score > bscore) { bscore = score; best = j; bx = x; by = y; bsea = sea; bhurt = hurt; }
    }
    if (best < 0) return 0;
    struct sfort *f = &g_sf[idx][g_sf_n[idx]++];
    memset(f, 0, sizeof *f);
    f->x = bx; f->y = by; f->t = now ? now : 1;
    int gx = build_mgr_get_base_x(bm) - bx, gy = build_mgr_get_base_y(bm) - by;   // the side towards the base: 0 -x, 1 +x, 2 -y, 3 +y
    f->open = abs(gx) >= abs(gy) ? (gx < 0 ? 0 : 1) : (gy < 0 ? 2 : 3);
    if (g_log) fprintf(g_log, "# fort: P%u at %.1f min: fort %d round the mine (type %d) at %d,%d - %d at work%s%s, open towards the base (side %d)\n",
                       idx, now / 60000.0, g_sf_n[idx], unit_type_get_id(unit_get_type_data(c[best].s)), bx, by, c[best].n,
                       bsea ? ", the sea in a ship's reach" : "", bhurt ? ", citizens hurt by it" : "", f->open);
    return 1;
}
// the fort's towers and walls, one order a tick: 1 ordered or waiting
internal int sf_walls(ee_player *pl, unsigned idx, struct sfort *f, void *wm, const int *stock, uint32_t now);
internal int sf_build(ee_player *pl, unsigned idx, struct sfort *f, ee_build_mgr *bm, void *wm, const int *stock, uint32_t now) {
    int ttype = tower_newest(pl), e = current_epoch(pl);
    if (f->towers < 4) {   // the corner towers
        if (!ttype || tower_queued(bm) || stock[4] < unit_cost(pl, ttype, 4) + 100 || e < 0 || e >= 14) {
            static uint32_t logged[16];
            if (g_log && (!logged[idx] || now - logged[idx] >= 120000)) {
                logged[idx] = now ? now : 1;
                fprintf(g_log, "# fort: P%u waits for its corner tower %d: tower type %d, %s, stone %d of %d\n", idx, f->towers, ttype,
                        tower_queued(bm) ? "a tower queued" : "none queued", stock[4], ttype ? unit_cost(pl, ttype, 4) + 100 : 0);
            }
            return 1;
        }
        int cx, cy;
        sf_corner(f, f->towers, &cx, &cy);
        for (int r = 0; r <= 1; r++)
            for (int ax = -r; ax <= r; ax++)
                for (int ay = -r; ay <= r; ay++) {
                    if ((abs(ax) != r && abs(ay) != r) || !can_place(pl, ttype, iv2(cx + ax, cy + ay))) continue;
                    int entry[3] = { ttype, cx + ax, cy + ay };
                    build_mgr_queue_add(bm, e, entry);
                    g_sf_towers[idx]++;
                    if (g_log) fprintf(g_log, "# fort: P%u tower %d at %d,%d (corner %d of the fort at %d,%d)\n", idx, ttype, cx + ax, cy + ay, f->towers, f->x, f->y);
                    f->towers++;
                    return 1;
                }
        f->towers++;   // no room at that corner: the next
        return 1;
    }
    return sf_walls(pl, idx, f, wm, stock, now);
}
// the fort's walls, a run at a time (also an outpost's fort, expand.c): 1 ordered or waiting
internal int sf_walls(ee_player *pl, unsigned idx, struct sfort *f, void *wm, const int *stock, uint32_t now) {
    static const enum ut_name wline[] = { UT_WALL_LINE };
    int wtype = 0;
    for (int i = 5; i >= 0 && !wtype; i--) if (ut_has(g_ut[wline[i]]) && can_build(pl, g_ut[wline[i]])) wtype = g_ut[wline[i]];
    if (!wtype || f->tries >= 12) return 0;
    static const int sides[4][2] = { { 3, 0 }, { 1, 2 }, { 0, 1 }, { 2, 3 } };   // side 0 -x .. 3 +y: its corners
    for (int sd = 0; sd < 4; sd++) {
        if (sd == f->open) continue;
        int ax, ay, bx, by;
        sf_corner(f, sides[sd][0], &ax, &ay);
        sf_corner(f, sides[sd][1], &bx, &by);
        int dx = bx > ax ? 1 : bx < ax ? -1 : 0, dy = by > ay ? 1 : by < ay ? -1 : 0, len = abs(bx - ax) > abs(by - ay) ? abs(bx - ax) : abs(by - ay);
        int rs = -1, best_s = -1, best_l = 0;   // the longest run of free tiles between the corners' towers
        for (int j = 2; j <= len - 1; j++) {
            int x = ax + dx * j, y = ay + dy * j, free_ = !encl_walled(x, y) && can_place(pl, wtype, iv2(x, y)) && j < len - 1;
            if (free_ && rs < 0) rs = j;
            if (!free_ && rs >= 0) { if (j - rs > best_l) { best_l = j - rs; best_s = rs; } rs = -1; }
        }
        if (best_l < 1) continue;
        int most = 12;
        for (int r = 2; r < 7; r++) {
            int c = unit_cost(pl, wtype, r), have = stock[r] - (r == 4 ? 100 : 0);
            if (c > 0 && have / c < most) most = have / c;
        }
        if (most < 1) return 1;
        if (best_l > most) best_l = most;
        int a[2] = { ax + dx * best_s, ay + dy * best_s }, b[2] = { ax + dx * (best_s + best_l - 1), ay + dy * (best_s + best_l - 1) };
        build_wall(wm, a, b, wtype);
        f->tries++;
        g_sf_runs[idx]++;
        if (g_log) fprintf(g_log, "# fort: P%u wall %d from %d,%d to %d,%d (side %d of the fort at %d,%d)\n", idx, wtype, a[0], a[1], b[0], b[1], sd, f->x, f->y);
        return 1;
    }
    (void)now;
    return 0;   // its walls stand (or can't)
}
internal void site_forts_tick(ee_player *pl, unsigned idx, ee_build_mgr *bm, void *wm, const int *stock, uint32_t now) {
    if (!g_site_forts) return;
    int busy = 0;
    for (int f = 0; f < g_sf_n[idx] && !busy; f++)
        if (now - g_sf[idx][f].t < 1800000) busy = sf_build(pl, idx, &g_sf[idx][f], bm, wm, stock, now);   // 30 minutes a fort
    if (!busy && g_sf_n[idx] < g_site_forts && g_sf_n[idx] < SF_MAX) sf_pick(pl, idx, now);
}
internal void site_forts_new_game(void) { memset(g_sf_n, 0, sizeof g_sf_n); }
internal int sf_busy(unsigned idx, uint32_t now) {   // a fort being built (its towers or walls not all ordered)
    for (int f = 0; f < g_sf_n[idx & 15]; f++)
        if (now - g_sf[idx & 15][f].t < 1800000 && (g_sf[idx & 15][f].towers < 4 || g_sf[idx & 15][f].tries < 3)) return 1;
    return 0;
}
internal int sf_stone(ee_player *pl) {   // the towers still to order and three sides of walls, about
    if (!g_site_forts || !pl) return 0;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    uint32_t now = game_time();
    int ttype = tower_newest(pl), need = 0;
    for (int f = 0; f < g_sf_n[idx]; f++) {
        struct sfort *o = &g_sf[idx][f];
        if (now - o->t >= 1800000) continue;
        need += (4 - o->towers) * (ttype ? unit_cost(pl, ttype, 4) : 150);
        if (o->tries < 3) need += 3 * (2 * SF_R - 3) * 10;   // a piece of wall about 10 stone
    }
    return need;
}

// Walls: per AI every 20 s (econ_tick). Best wall type the player can build now (0x555710: available, affordable).
internal void defence_tick(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_def || (!g_def_walls && !g_encl && !g_ring && g_v_ring <= 0) || g_def_off[idx] || (now >= last[idx] && now - last[idx] < 20000)) return;
    last[idx] = now;
    def_refresh(pl, idx, now);
    struct def_plan *p = &g_plan[idx];
    struct def_scan *s = &g_scan;
    if (s->pl != pl) return;
    void *cm = player_get_civ_mgr(pl), *wm = cm ? civ_mgr_get_wall_mgr(cm) : NULL;
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!wm || !bm) return;
    int *stock = player_stock_at(pl);
    // Towers of our own: the game's plan asks for few (P3: none in the first 15 minutes of a Bronze start, with 685
    // stone in stock - stone has hardly another use). While a slot of the plan is free and the stone allows (100 kept
    // back), one tower per minute goes into the build queue with its position; nothing while one is still queued.
    static uint32_t last_tower[16];
    if (g_def_more && !(g_site_forts && sf_busy(idx, now)) && (!last_tower[idx] || now < last_tower[idx] || now - last_tower[idx] >= 60000)) {   // a fort's towers first
        int queued = tower_queued(bm), ttype = tower_newest(pl);
        int x, y, e = current_epoch(pl);
        int owed = encl_owed(pl);   // enclose: the open ring's walls first
        owed = owed > 400 ? 400 : owed;
        if (!queued && ttype && stock[4] >= unit_cost(pl, ttype, 4) + 100 + owed && e >= 0 && e < 14 &&
            defence_spot(pl, ttype, &x, &y)) {
            int entry[3] = { ttype, x, y };
            build_mgr_queue_add(bm, e, entry);
            last_tower[idx] = now ? now : 1;
            if (g_log) fprintf(g_log, "# defence: P%u queues tower %d at %d,%d (stone %d)\n", idx, ttype, x, y, stock[4]);
            ai_say(pl, CHAT_TOWER, "[ai] extra tower (type %d) at %d,%d, stone %d", ttype, x, y, stock[4]);
        }
    }
    site_forts_tick(pl, idx, bm, wm, stock, now);
    if (encl_on(idx)) encl_walls(pl, idx, p, wm, stock, now);
    if (encl_on(idx) && g_encs[idx].pl == pl && g_encs[idx].ready) return;   // the ring instead of the fronts' walls
    if (!g_def_walls || PV(idx, g_v_ring, g_ring)) return;   // the town ring: no open lines before it either
    for (int k = 0; k < p->nf; k++) {   // the gate, once the wall's middle stands finished: the newest it can build
        struct front *f = &p->f[k];
        if (!f->gdir || f->gtries >= 20 || !gate_fits(pl, f->gx, f->gy, f->gdir)) continue;
        static const enum ut_name gline[] = { UT_GATE_LINE };   // oldest first
        int gtype = 0;
        for (int i = 5; i >= 0 && !gtype; i--)
            if (ut_has(g_ut[gline[i]]) && can_build(pl, g_ut[gline[i]])) gtype = g_ut[gline[i]];
        f->gtries++;
        if (!gtype) continue;
        int tile[2] = { f->gx, f->gy };
        build_gate(tile, player_get_index(pl), unit_type_get_tech(type_data(gtype)), f->gdir);
        f->gdir = 0;
        if (g_log) fprintf(g_log, "# defence: P%u gate %d in its wall at %d,%d\n", idx, gtype, tile[0], tile[1]);
        return;   // one order per tick
    }
    for (int k = 0; k < p->nf; k++) {
        struct front *f = &p->f[k];
        if (f->wall_tries >= 3 || (f->wall_t && now - f->wall_t < 180000)) continue;
        int ntow = 0;
        for (int t = 0; t < s->towers; t++)
            ntow += abs(s->tx[t] - f->fx) <= 7 && abs(s->ty[t] - f->fy) <= 7;
        if (ntow < 2) continue;
        static const enum ut_name wline[] = { UT_WALL_LINE };   // oldest first
        int wtype = 0;
        for (int i = 5; i >= 0 && !wtype; i--)   // the newest it can build
            if (ut_has(g_ut[wline[i]]) && can_build(pl, g_ut[wline[i]])) wtype = g_ut[wline[i]];
        if (!wtype) continue;
        int ok = 1;
        for (int r = 2; r < 7; r++) if (stock[r] < 11 * unit_cost(pl, wtype, r)) ok = 0;
        if (!ok) continue;
        int cx = f->fx + (int)lroundf(f->dx * 3.0f), cy = f->fy + (int)lroundf(f->dy * 3.0f);
        float px = -f->dy, py = f->dx, ax = fabsf(px), ay = fabsf(py);   // across the front, snapped to 8 directions
        int sx = ax > 2.414f * ay ? (px > 0 ? 1 : -1) : ay > 2.414f * ax ? 0 : (px > 0 ? 1 : -1);
        int sy = ay > 2.414f * ax ? (py > 0 ? 1 : -1) : ax > 2.414f * ay ? 0 : (py > 0 ? 1 : -1);
        int h = sx && sy ? 4 : 5;   // 11 tiles along, 8 diagonal steps (11.3)
        int a[2] = { cx + sx * h, cy + sy * h }, b[2] = { cx - sx * h, cy - sy * h };
        threat_refresh(pl, now);
        if (route_covered(pl, p->bx, p->by, a[0], a[1]) || route_covered(pl, p->bx, p->by, b[0], b[1]) ||
            route_covered(pl, p->bx, p->by, cx, cy)) continue;   // the builders would walk into enemy fire
        if (raiders_near(pl, iv2(a[0], a[1]), 10) || raiders_near(pl, iv2(b[0], b[1]), 10) || raiders_near(pl, iv2(cx, cy), 10))
            continue;   // or into an enemy army standing there
        // the arc on the map and on land, ends and middle: on an island map an arc into the water (or off the map)
        // crashed the game's wall placement (0x497C3F -> 0x636E30, a null tile object), twice on Small Islands
        ee_map *wmap = game_map();
        int mn = wmap ? (int)map_get_size(wmap) : 0, land = mn > 0;
        for (int q = 0; q <= 2 && land; q++) {
            int qx = q == 0 ? a[0] : q == 1 ? b[0] : cx, qy = q == 0 ? a[1] : q == 1 ? b[1] : cy;
            land = qx >= 3 && qy >= 3 && qx < mn - 3 && qy < mn - 3 && area_id(iv2(qx, qy), 2) && !area_id(iv2(qx, qy), 1);
        }
        if (!land) { f->wall_tries++; f->wall_t = now ? now : 1; continue; }
        int have = 0;   // wall and gate pieces standing on the arc (then a new order only repairs)
        for (int w = 0; w < s->walls; w++) have += on_segment(s->wx[w], s->wy[w], a, b);
        if (have >= 8) continue;
        build_wall(wm, a, b, wtype);
        f->wall_tries++;
        f->wall_t = now ? now : 1;
        f->gx = cx; f->gy = cy; f->gdir = !sy ? 1 : !sx ? 2 : sx == sy ? 4 : 3; f->gtries = 0;
        g_def_walls_built[idx]++;
        if (g_log) fprintf(g_log, "# defence: P%u wall %d from %d,%d to %d,%d in front of %d towers (front %d,%d)\n",
                           idx, wtype, a[0], a[1], b[0], b[1], ntow, f->fx, f->fy);
        ai_say(pl, CHAT_WALL, "[ai] walling the front at %d,%d (%d towers there)", f->fx, f->fy, ntow);
        return;   // one order per tick
    }
}

// COUNTER-TOWER (same switch): a player walks citizens up to an AI's mine or berries and puts a tower beside it - the
// AI let it be built and the tower killed its gatherers. Enemy defences under construction ([u+0x30]+0x340 = 0) it
// can see within 12 tiles of its buildings or citizens get the same answer as siege: the nearest soldiers (or, short
// of soldiers, its citizens) attack the builders - the enemy citizens whose EEUGBuildRepair holds it - spread over
// them, and the foundation only when nobody builds it. Finished towers are the siege weapons' job (attack.c siege_tick).
// COUNTER-SIEGE (ageofai.ini [ai_base] counter_siege, default on): catapults outrange towers (bronze: 7 against 5) and the
// AI just watched its towers being shelled. A player sends a few soldiers out at them. Every 2 s per AI: enemy siege
// weapons (and rams) it can see within their range + 4 tiles of one of its buildings are threats; its soldiers within
// 20 tiles of one - idle, marching, or fighting something more than 4 tiles away - attack it (up to 5 each, nearest
// first, priority like the siege controller's orders). Only when they are at least 2x + 3 the enemy soldiers within
// 8 tiles of the siege weapon: a sally into the escorting army (siege_towers: 14 defenders against 36) lost the base
// that towers and walls would have held.
internal int g_csiege = 1;
internal unsigned g_csiege_orders[16], g_csiege_skipped[16];
struct cs_scan {
    ee_player *pl;
    int nb, bx[256], by[256];           // own buildings
    int ns; ee_unit *sold[128];          // own soldiers
    int ne; ee_unit *esiege[16];         // enemy siege weapons seen
    int nx; int ex[256], ey[256];        // enemy soldiers (positions)
    int nc; int cx[128], cy[128];        // own citizens (positions)
    ee_unit *civ[128];                   // own citizens
    int nu; ee_unit *eu[128];            // enemy soldiers and citizens (militia)
    int nt; ee_unit *etow[16];           // enemy defences under construction, seen
};
internal void cs_unit(ee_unit *u, void *ctx) {
    struct cs_scan *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    ee_player *owner = unit_get_owner(u);
    if (unit_get_hp(u) <= 0 || !owner) return;
    int fam = unit_type_get_class_id(td), type = unit_type_get_id(td), x = unit_get_x(u), y = unit_get_y(u);
    if (owner == s->pl) {
        if ((fam == CLS_BUILDING || fam == CLS_FORTRESS) && s->nb < 256) { s->bx[s->nb] = x; s->by[s->nb++] = y; }
        else if (is_soldier(td) && s->ns < 128) s->sold[s->ns++] = u;
        else if (is_citizen_type(type) && s->nc < 128) { s->civ[s->nc] = u; s->cx[s->nc] = x; s->cy[s->nc++] = y; }
    } else if (is_enemy(s->pl, owner)) {
        ee_unit *bs = unit_get_physics(u);
        float r, a;
        if ((is_soldier(td) || is_citizen_type(type)) && s->nu < 128 && seen_by(u, s->pl)) s->eu[s->nu++] = u;
        if ((is_siege(type) || is_ram(type)) && s->ne < 16 && seen_by(u, s->pl)) s->esiege[s->ne++] = u;
        else if ((fam == CLS_BUILDING || fam == CLS_FORTRESS) && bs && !unit_get_finished(bs) && s->nt < 16 && defence_stats(u, &r, &a) &&
                 seen_by(u, s->pl)) s->etow[s->nt++] = u;
        else if (is_soldier(td) && s->nx < 256) { s->ex[s->nx] = x; s->ey[s->nx++] = y; }
    }
}
// CITIZEN MILITIA (ageofai.ini [ai_base] citizen_militia, default on). A player answers citizens that walk up to his mine or
// berries and start a tower there with his own citizens, and in the early ages his citizens beat off a small rush.
// The AI's citizens never fought. Every 2 s (with the counter-siege scan):
// - enemy citizens building a defence (EEUGBuildRepair, target +0x3C unfinished, a ground attack) within 12 tiles of
//   the AI's buildings or citizens, any epoch;
// - up to the Bronze Age: enemy units attacking (EEUGAttackUnit, target +0x30) a unit of the AI within 10 tiles of
//   its citizens;
// are attacked by the AI's citizens within 10 tiles, at most 3 per enemy and against at most 6 - only when those
// citizens' strength (hit points x attack) is 3 times that of the enemies within 6 tiles: Bronze Age soldiers (119 hp)
// killed 10 citizens of a count-based militia while losing 49 hit points. Gathering or idle citizens only; afterwards the assignment puts them back to work.
// [ai_base] militia_estimate (default 0): 1 = the citizens decide with the stats estimate (dll/ai/stats.c) instead, in every
// epoch (the playtester: "a hard limit isn't right... tower builders without guards should always be engaged; troops
// in later ages should be avoided, but many units are faster than citizens - caught alone and running, they get
// chased down"): enemies without soldiers among them (builders, a foundation) when the citizens win (ratio >= 1);
// soldiers when the citizens win clearly (ratio >= 1.5), or when they are caught - the soldiers attack one of ours
// and are faster than citizens - and would still win (militia_caught_share 1: the share of the attackers destroyed;
// 0.33 had 3 citizens attack a Mk V tank 42 times - 0.2 of their damage gets through, a third of a tank is no use). A fallback ("when the
// citizens have a chance and no military is near to do the job"): not when own soldiers within 12 tiles win on their own.
internal int g_militia = 1, g_militia_est = 0;
internal float g_militia_share = 1.0f;   // [ai_base] militia_caught_share: the share of the attackers caught citizens must still destroy
internal unsigned g_militia_caught[16];
internal unsigned g_militia_orders[16];
internal float unit_power(ee_unit *u) {   // hit points x attack (live stats), a rough fighting strength
    ee_unit_stats *st = unit_get_stats(u);
    float a = st ? unit_stats_get_attack(st) : (float)unit_type_get_attack(unit_get_type_data(u));
    return (float)unit_get_hp(u) * (a > 0 ? a : 0);
}
internal struct st_side g_mil_own, g_mil_foe;   // fight_estimate (dll/ai/stats.c)
internal int militia_answer(struct cs_scan *s, ee_unit *t, int *nfree) {   // citizens -> t; returns the number ordered
    int x = unit_get_x(t), y = unit_get_y(t), enemies = 0, ordered = 0;
    float epow = 0, cpow = 0, fast = 1e9f, cspeed = 0;
    unsigned pi = (unsigned)player_get_index(s->pl) & 15;
    int est = g_fight_est || g_militia_est || EARLY(pi, early_militia), soldiers = 0;
    if (est) { st_side_clear(&g_mil_own); st_side_clear(&g_mil_foe); }
    for (int k = 0; k < s->nu; k++) {
        ee_unit *e = s->eu[k];
        if (unit_get_hp(e) <= 0 || abs(unit_get_x(e) - x) > 6 || abs(unit_get_y(e) - y) > 6) continue;
        enemies++;
        epow += unit_power(e);
        if (est) st_side_add(&g_mil_foe, e);
        if (is_soldier(unit_get_type_data(e))) { soldiers++; if (st_speed(e) < fast) fast = st_speed(e); }
    }
    if (enemies < 1) {   // a foundation: no fight, only its hit points
        enemies = 1; epow = unit_power(t);
        if (est) st_side_add(&g_mil_foe, t);
    }
    if (enemies > 6) return 0;
    for (int c = 0, n = 0; c < s->nc && n < 3 * enemies; c++)
        if (s->civ[c] && abs(s->cx[c] - x) <= 10 && abs(s->cy[c] - y) <= 10) {
            cpow += unit_power(s->civ[c]); n++;
            if (est) st_side_add(&g_mil_own, s->civ[c]);
            if (st_speed(s->civ[c]) > cspeed) cspeed = st_speed(s->civ[c]);
        }
    // EARLY MILITIA ([ai_base] early_militia; dll/ai/early.c): against soldiers the citizens fight only next to their own
    // soldiers or under their own towers, and only where the estimate has them win without a loss - in the first
    // epoch 1-6 Clubmen and Rock Throwers killed 39-48 citizens a game at the town, militia ordered 40-63 times a
    // game. Otherwise the citizens fall back under a tower (tower_bait) and the tower does the fighting.
    if (EARLY(pi, early_militia) && soldiers) {
        int cover = early_cover(s->pl, x, y);
        for (int k = 0; k < s->ns && !cover; k++)
            cover = s->sold[k] && abs(unit_get_x(s->sold[k]) - x) <= 12 && abs(unit_get_y(s->sold[k]) - y) <= 12;
        struct st_result r;
        if (cover && g_mil_own.n) st_estimate(&g_mil_own, &g_mil_foe, &r);
        // their damage while ours destroys them, in citizens: under one
        float lost = cover && g_mil_own.n && r.time[0] < 1e9f ? r.dps_contact[1] * r.time[0] * (float)g_mil_own.n / g_mil_own.hp : 99.0f;
        if (lost >= 1.0f) { g_early_mil_refused[pi]++; return 0; }
    }
    if (g_militia_est) {
        if (!g_mil_own.n) return 0;
        // a fallback (the playtester): soldiers near that win it on their own do the job instead
        static struct st_side mil_sold;
        st_side_clear(&mil_sold);
        for (int k = 0; k < s->ns; k++)
            if (s->sold[k] && abs(unit_get_x(s->sold[k]) - x) <= 12 && abs(unit_get_y(s->sold[k]) - y) <= 12) st_side_add(&mil_sold, s->sold[k]);
        if (mil_sold.n && st_ratio(&mil_sold, &g_mil_foe) >= 1.0f) return 0;
        float stand = st_ratio(&g_mil_own, &g_mil_foe);
        if (!soldiers) { if (stand < 1.0f) return 0; }   // builders, a foundation
        else if (stand < 1.5f) {
            struct st_result r;   // caught: running is no way out - fight if the citizens still do some good
            st_estimate(&g_mil_own, &g_mil_foe, &r);
            float share = r.time[0] < 1e9f ? r.time[1] / (r.time[0] > 0.1f ? r.time[0] : 0.1f) : 0;
            int attacking = unit_get_goal(t) && goal_is_attack(unit_get_goal(t));
            if (!attacking || fast <= cspeed * 1.1f || share < g_militia_share) return 0;
            g_militia_caught[(unsigned)player_get_index(s->pl) & 15]++;
        }
    } else {
        float stand = g_fight_est ? st_ratio(&g_mil_own, &g_mil_foe) : (epow > 0 ? cpow / epow : 1e6f);
        if (stand < 3.0f) return 0;   // citizens beat clubmen and builders, not real soldiers
    }
    for (int c = 0; c < s->nc && ordered < 3 * enemies; c++) {
        ee_unit *u = s->civ[c];
        if (!u || abs(s->cx[c] - x) > 10 || abs(s->cy[c] - y) > 10) continue;
        ee_goal *g = unit_get_goal(u);
        if (g && goal_is_attack(g)) { s->civ[c] = NULL; continue; }   // already fighting
        if (g && strcmp(rtti_name(g), "EEUGGather@@")) continue;   // building, walking: leave it
        if (unit_goal(u, goal_attack(u, t), 0, 0x1F40)) ordered++;
        s->civ[c] = NULL;   // one target per citizen per tick
    }
    *nfree -= ordered;
    return ordered;
}
// RAIDS THE TOWN HOLDS ([ai_base] raid_held, default 150% since 2026-11-07; the player 2026-11-07: citizens keep working,
// even under attack - "what i look at is if my towers and local soldiers can handle the raiders before they can kill
// my citizens"; six knights riding through a town kept the AI's gathering 8-15 down for 10 minutes after they had
// gone, tests/scenarios/riders.eeai): a site with raiders near is not dangerous while the AI's soldiers within 10
// tiles of it and its towers covering it beat the enemy soldiers it sees within 12 tiles on the fight estimate
// (st_ratio >= raid_held, 150% = clearly). Asked by the citizens' safety (attack.c citizens_safety, site_danger_res);
// a cell of 4 x 4 tiles keeps its answer for 4 s (one safety tick). Raiders only remembered, not seen: no answer here
// (the safety's own). Measured with hurt_seen_short (attack.c), 2026-11-07: riders.eeai, 30 seeds - in the games
// without riders too (P2's own raids) citizens killed 6.1 -> 2.6, at 40 min citizens 96 -> 107, soldiers 37 -> 48;
// against the cheating game AI, 40 long games a map (quit at 600 min) - Small Islands won 0 -> 2, lost 36 -> 29,
// kills - vanilla's +6 -> +98, workers -116 -> -62; Large Islands won 11 -> 16, lost 24 -> 22; Continental 26 -> 27.
internal float g_raid_held = 0;
struct rh_scan { ee_player *pl; ivec2 p; };
internal struct st_side g_rh_own, g_rh_foe;
internal void rh_unit(ee_unit *u, void *ctx) {
    struct rh_scan *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    ee_player *o = unit_get_owner(u);
    if (!td || !o || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) return;
    int d = abs(unit_get_x(u) - s->p.x) > abs(unit_get_y(u) - s->p.y) ? abs(unit_get_x(u) - s->p.x) : abs(unit_get_y(u) - s->p.y);
    int fam = unit_type_get_class_id(td);
    float rng, att;
    if (o == s->pl) {
        if ((fam == CLS_BUILDING || fam == CLS_FORTRESS) ? defence_stats(u, &rng, &att) && (float)d <= rng + 1.0f : is_soldier(td) && d <= 10)
            st_side_add(&g_rh_own, u);
    } else if (d <= 12 && (fam == CLS_LAND || fam == CLS_NAVAL) && unit_type_get_attack(td) > 0 && !is_citizen_type(unit_type_get_id(td)) &&
               player_get_index(o) > 0 && is_enemy(s->pl, o) && seen_by(u, s->pl))
        st_side_add(&g_rh_foe, u);
}
internal int raid_held(ee_player *pl, ivec2 p) {
    if (g_raid_held <= 0 || !pl) return 0;
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    static struct { int cx, cy; uint32_t t; uint8_t held; } cache[16][32];
    uint32_t now = game_time();
    int cx = p.x >> 2, cy = p.y >> 2;
    typeof(&cache[0][0]) c = &cache[pi][(unsigned)(cx * 31 + cy) & 31];
    if (c->t && c->cx == cx && c->cy == cy && now >= c->t && now - c->t < 4000) return c->held;
    st_side_clear(&g_rh_own); st_side_clear(&g_rh_foe);
    struct rh_scan s = { pl, iv2(cx * 4 + 2, cy * 4 + 2) };
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    for (struct grid_it it = grid_it_box(s.p.x, s.p.y, 14, 1); grid && grid_it_next(&it); ) rh_unit(it.u, &s);
    c->cx = cx; c->cy = cy; c->t = now ? now : 1;
    c->held = (uint8_t)(g_rh_foe.n > 0 && st_ratio(&g_rh_own, &g_rh_foe) >= g_raid_held);
    return c->held;
}

internal void militia(ee_player *pl, unsigned idx, struct cs_scan *s, uint32_t now) {
    if (!g_militia || !s->nc || !s->nu) return;
    int early = g_militia_est || current_epoch(pl) <= 3, nfree = s->nc, total = 0, targets = 0;
    char what[160] = "";
    for (int k = 0; k < s->nu && nfree > 2; k++) {
        ee_unit *e = s->eu[k];
        ee_goal *g = unit_get_goal(e);
        ee_unit *t;
        if (!g || unit_get_hp(e) <= 0) continue;
        int x = unit_get_x(e), y = unit_get_y(e), why = 0;
        if (!strcmp(rtti_name(g), "EEUGBuildRepair@@") && (t = goal_get_building(g)) && unit_get_owner(t) != pl) {
            ee_unit *bs = unit_get_physics(t);
            float r, a;
            if (bs && !unit_get_finished(bs) && defence_stats(t, &r, &a)) why = 1;   // a tower going up
        } else if (early && goal_is_attack(g) && (t = goal_get_target(g)) && unit_get_owner(t) == pl)
            why = 2;   // attacking one of ours
        if (!why) continue;
        int close = 0;
        for (int b = 0; why == 1 && b < s->nb && !close; b++) close = abs(s->bx[b] - x) <= 12 && abs(s->by[b] - y) <= 12;
        for (int c = 0; c < s->nc && !close; c++) close = abs(s->cx[c] - x) <= (why == 1 ? 12 : 10) && abs(s->cy[c] - y) <= (why == 1 ? 12 : 10);
        if (!close) continue;
        int n = militia_answer(s, e, &nfree);
        if (n) {
            total += n; targets++;
            size_t l = strlen(what);
            if (l < sizeof what - 32)
                snprintf(what + l, sizeof what - l, " %s P%d type %d at %d,%d", why == 1 ? "builder" : "attacker",
                         player_get_index(unit_get_owner(e)), unit_type_get_id(unit_get_type_data(e)), x, y);
        }
    }
    if (!total) return;
    g_militia_orders[idx] += (unsigned)total;
    if (g_log) fprintf(g_log, "# militia: P%u at %.1f min: %d citizens against %d enemies:%s (caught so far %u)\n", idx, now / 60000.0, total, targets, what, g_militia_caught[idx]);
}
internal void counter_fire(ee_player *pl, unsigned idx, uint32_t now);   // counterfire.c: shooters out of reach
internal void counter_siege(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    counter_fire(pl, idx, now);
    if (!g_csiege || g_def_off[idx] || (now >= last[idx] && now - last[idx] < 2000)) return;
    last[idx] = now;
    static struct cs_scan s;
    memset(&s, 0, sizeof s);
    s.pl = pl;
    grid_each(cs_unit, &s);
    militia(pl, idx, &s, now);
    for (int i = 0; i < s.ne + s.nt; i++) {
        int tower = i >= s.ne;   // an enemy defence going up near our buildings or citizens (counter-tower)
        ee_unit *e = tower ? s.etow[i - s.ne] : s.esiege[i];
        ee_unit_type *etd = unit_get_type_data(e); ee_unit_stats *st = unit_get_stats(e);
        int x = unit_get_x(e), y = unit_get_y(e);
        float r = tower ? 12.0f : (st ? unit_stats_get_range(st) : unit_type_get_range(etd)) + 4.0f;
        int threat = 0;
        for (int b = 0; b < s.nb && !threat; b++)
            threat = (float)ivec2_dist2(iv2(s.bx[b], s.by[b]), iv2(x, y)) <= r * r;
        for (int c = 0; tower && c < s.nc && !threat; c++)
            threat = (float)ivec2_dist2(iv2(s.cx[c], s.cy[c]), iv2(x, y)) <= r * r;
        if (!threat) continue;
        // a tower going up: its builders first (a player kills the citizens, the foundation is harmless without them)
        ee_unit *bl[8];
        ee_unit *site = tower ? field_site(e) : NULL;   // what their EEUGBuildRepair holds (+0x3C)
        int nbl = 0;
        for (int k = 0; tower && k < s.nu && nbl < 8; k++) {
            ee_goal *g = unit_get_goal(s.eu[k]);
            // the goal's kind before its building: a smaller goal ends before +0x3C (one at the end of the heap's
            // page faulted there, 2026-11-13)
            if (g && unit_get_hp(s.eu[k]) > 0 && !strcmp(rtti_name(g), "EEUGBuildRepair@@") &&
                (goal_get_building(g) == site || goal_get_building(g) == e)) bl[nbl++] = s.eu[k];
        }
        int guards = 0;
        for (int k = 0; k < s.nx; k++) guards += abs(s.ex[k] - x) <= 8 && abs(s.ey[k] - y) <= 8;
        // responders: nearest first, up to 5
        int pick[5], np = 0, avail = 0, already = 0;
        for (int k = 0; k < s.ns; k++) {
            ee_unit *u = s.sold[k];
            if (!u) continue;   // already ordered this tick
            ee_goal *g = unit_get_goal(u);
            int d2 = ivec2_dist2(unit_get_tile(u), iv2(x, y));
            if (d2 > 400) continue;
            if (g && goal_is_attack(g)) {
                ee_unit *t = goal_get_target(g);
                int mine = t == e;
                for (int b = 0; b < nbl && !mine; b++) mine = t == bl[b];
                if (mine) { already++; s.sold[k] = NULL; continue; }
                if (t && abs(unit_get_x(t) - unit_get_x(u)) <= 4 && abs(unit_get_y(t) - unit_get_y(u)) <= 4)
                    continue;   // busy fighting close by
            }
            avail++;
            if (np < 5) pick[np++] = k;
            else {   // keep the 5 nearest
                int worst = 0, fd = -1;
                for (int j = 0; j < 5; j++) {
                    ee_unit *v = s.sold[pick[j]];
                    int vx = unit_get_x(v) - x, vy = unit_get_y(v) - y;
                    if (vx * vx + vy * vy > fd) { fd = vx * vx + vy * vy; worst = j; }
                }
                if (d2 < fd) pick[worst] = k;
            }
        }
        if (already >= 5) continue;
        if (avail + already < 2 * guards + 3) {
            int nfree = s.nc, n = 0;   // a tower going up and too few soldiers: the citizens go at the builders
            for (int b = 0; tower && g_militia && b < nbl && nfree > 2; b++) n += militia_answer(&s, bl[b], &nfree);
            if (tower && g_militia && !nbl) n += militia_answer(&s, e, &nfree);   // nobody building: the foundation
            if (n) g_militia_orders[idx] += (unsigned)n;
            else g_csiege_skipped[idx]++;
            continue;
        }
        int ordered = 0;
        for (int j = 0; j < np && already + ordered < 5; j++) {
            ee_unit *u = s.sold[pick[j]];
            ee_unit *t = nbl ? bl[(already + ordered) % nbl] : e;   // spread over the builders
            if (unit_goal(u, goal_attack(u, t), 0, 0x1F40)) ordered++;
            s.sold[pick[j]] = NULL;   // one target per unit per tick
        }
        g_csiege_orders[idx] += (unsigned)ordered;
        if (ordered)
            ai_say(pl, CHAT_COUNTER, "[ai] enemy siege at %d,%d - %d soldiers sent against it (%d guards)", x, y,
                   ordered + already, guards);
        if (ordered && g_log)
            fprintf(g_log, "# counter-%s: P%u at %.1f min: enemy type %d at %d,%d (%d guards, %d builders) <- %d soldiers (%d already on it)\n",
                    tower ? "tower" : "siege", idx, now / 60000.0, unit_type_get_id(etd), x, y, guards, nbl, ordered, already);
    }
}

// HOME GUARD (ageofai.ini [ai_base] home_guard = soldiers per epoch, default 4; 0 = off): the unit manager (0x490AD6)
// only trains what is ordered into its request queues - mostly the next attack's group, ordered shortly before the
// attack. Between attacks nothing is trained: at minute 45 a Bronze Age AI had 10 soldiers, 25 requests in 5
// minutes and 1555 food + 1554 wood in its military account; a raid met 2-5 defenders. A player keeps a force at
// home. Every 10 s: fewer than max(6 + home_guard x epoch, home_guard_share x military room) own soldiers within 25
// tiles of the base (room = population cap x (1 - citizen share): 0.35 x 166 x 0.55 = 32) -> up to 5 more through
// the train hook (budget applies), type: the strongest trainable land soldier (substitution picks what the budget
// pays for).
internal int g_home_guard = 4;
internal float g_home_share = 0.35f;   // the medium AI's home_guard_share (the log; each AI uses its own: cfg_of)
internal unsigned g_home_trained[16];
struct home_scan { ee_player *pl; int bx, by, n; };
internal void home_unit(ee_unit *u, void *ctx) {
    struct home_scan *h = ctx;
    if (unit_get_owner(u) != h->pl || unit_get_hp(u) <= 0 || !is_soldier(unit_get_type_data(u))) return;
    h->n += ivec2_dist2(unit_get_tile(u), iv2(h->bx, h->by)) <= 625;
}
internal int home_soldiers(ee_player *pl, int bx, int by) {
    struct home_scan h = { pl, bx, by, 0 };
    grid_each(home_unit, &h);
    return h.n;
}
internal int home_soldiers_now(ee_player *pl) {
    void *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    return bm && build_mgr_get_base_x(bm) != -10000 ? home_soldiers(pl, build_mgr_get_base_x(bm), build_mgr_get_base_y(bm)) : -1;
}
internal void home_guard(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_home_guard || g_def_off[idx] || (now >= last[idx] && now - last[idx] < 10000)) return;
    last[idx] = now;
    void *cm = player_get_civ_mgr(pl), *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!um || !bm || build_mgr_get_base_x(bm) == -10000) return;
    int e = current_epoch(pl), target = 6 + g_home_guard * e;
    // and a share of the military room: population cap (vf31 - follows the human's population unless Ignorieren) x
    // (1 - citizen share +0xBDC)
    int cap = unit_limit(pl);
    float share = cfg_of(pl)->home_guard_share;   // the difficulty's (its section, else [ai_base]) or the personality's
    int room = (int)((float)cap * (1.0f - player_get_citizen_share(pl)) * share);
    if (room > target) target = room;
    // early_army (econ.c train_budget): no soldiers in the first epoch - towers and citizens; early_soldiers=N (the
    // alternative to the towers): N soldiers there from minute 2
    if (EARLY(idx, early_army) && e == 0) {
        target = now >= 120000 ? EARLY(idx, early_soldiers) : 0;
        if (!target) return;
    }
    int home = home_soldiers(pl, build_mgr_get_base_x(bm), build_mgr_get_base_y(bm));
    if (home >= target) return;
    int big[7] = { 0, 0, 1 << 20, 1 << 20, 1 << 20, 1 << 20, 1 << 20 }, got = 0;
    int type = substitute_soldier(pl, idx, big, 1, &got);   // the strongest the game can train now
    if (!type) return;
    int req[4] = { type, target - home < 5 ? target - home : 5, 0, 0 };
    unsigned n = 0;
    g_hg_call = 1;
    hook_train_units(um, req, (uintptr_t)&n);
    g_hg_call = 0;
    g_home_trained[idx] += n;
    if (n) ai_say(pl, CHAT_GUARD, "[ai] home guard %d of %d, training %u more", home, target, n);
    if (n && g_log) fprintf(g_log, "# home guard: P%u at %.1f min: %d of %d soldiers at home, %u more (type %d asked)\n",
                            idx, now / 60000.0, home, target, n, type);
}

// MUSTER ([ai_base] muster, default 1; with fight_estimate the sides are compared with dll/ai/stats.c's estimate): an enemy army in the base got the AI's defenders one by one - every soldier the
// home guard trained walked into it alone, and the next one, and the next. A player would pull back what is left,
// gather, and strike once there is a force that can win. Every 3 s per AI: enemy soldiers within 22 tiles of the
// base (strength = hit points x attack) against its own soldiers within 35 and its towers within 22. Only when the
// fight at home is hopeless - at least 8 enemies, twice the strength of soldiers and towers: muster - every own
// soldier there goes to a rally point 18 tiles beyond the base, on the side away from the enemy's centre; new ones
// join as they are trained. Strike once the soldiers at the rally point are 1.3 x the enemy in the base: each attacks
// the nearest enemy soldier. The muster ends without a strike when the enemy is gone (fewer than 3), when soldiers
// and towers can stand up to it again, or when the enemy follows to the rally point a third time (then it fights
// where it is) - and no new muster for 30 s / 60 s. Citizens near the enemy go to the rally point too
// (citizens_safety, attack.c) - their drop site is where the fight is - unless enemy soldiers stand on the way: then
// 12 tiles in the clear direction nearest the rally point (Hedda's citizens, working east of her base, crossed it to
// the rally point in the west - through the player's army camped there).
// First version (any superiority, 5 enemies, no way out): assault scenario, 4 seeds - the defender gave up a base it
// would have held (9 attackers against 4 soldiers, towers and militia: off 88% alive, on 62%), the attackers followed
// to the rally point, its fleeing citizens stopped the economy and it surrendered.
internal int g_muster = 1;
internal struct { int on, moves; float rx, ry; uint32_t since, cool, cool_len; unsigned episodes, orders, strikes; } g_mus[16];
struct mus_scan {
    ee_player *pl; int bx, by, en, no, ne;
    float epow, opow, tpow, ex, ey;
    ee_unit *own[256], *enemy[128];
    struct st_side *os, *es;   // fight_estimate: own soldiers and towers, the enemies (dll/ai/stats.c)
};
internal struct mus_scan g_ms;
internal struct st_side g_ms_own, g_ms_foe, g_ms_rally;
internal void mus_unit(ee_unit *u, void *ctx) {
    struct mus_scan *m = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    if (unit_get_hp(u) <= 0 || !td) return;
    ee_player *o = unit_get_owner(u);
    int x = unit_get_x(u), y = unit_get_y(u), d2 = ivec2_dist2(iv2(x, y), iv2(m->bx, m->by));
    float tr, ta;
    if (o == m->pl && d2 <= 22 * 22 && defence_stats(u, &tr, &ta)) {
        m->tpow += (float)unit_get_hp(u) * ta;
        if (m->os) st_side_add(m->os, u);
        return;
    }
    if (!is_soldier(td)) return;
    if (o == m->pl) {
        if (d2 > 35 * 35) return;
        if (m->no < 256) m->own[m->no++] = u;
        m->opow += unit_power(u);
        if (m->os) st_side_add(m->os, u);
    } else if (o && player_get_index(o) > 0 && is_enemy(m->pl, o) && d2 <= 22 * 22) {
        float p = unit_power(u);
        if (m->es) st_side_add(m->es, u);
        m->epow += p;
        m->ex += (float)x * p;
        m->ey += (float)y * p;
        m->en++;
        if (m->ne < 128) m->enemy[m->ne++] = u;
    }
}
internal void mus_rally(unsigned idx, struct mus_scan *s) {
    float cx = s->epow > 0 ? s->ex / s->epow : (float)s->bx + 1, cy = s->epow > 0 ? s->ey / s->epow : (float)s->by;
    float dx = (float)s->bx - cx, dy = (float)s->by - cy, l = sqrtf(dx * dx + dy * dy);
    if (l < 1.0f) { dx = 1.0f; dy = 0.0f; l = 1.0f; }
    float msz = (float)map_get_size(game_map());
    float rx = (float)s->bx + dx / l * 18.0f, ry = (float)s->by + dy / l * 18.0f;
    if (msz > 8) {
        rx = rx < 3 ? 3 : rx > msz - 4 ? msz - 4 : rx;
        ry = ry < 3 ? 3 : ry > msz - 4 ? msz - 4 : ry;
    }
    g_mus[idx].rx = rx;
    g_mus[idx].ry = ry;
}
internal int muster_rally(unsigned idx, float *x, float *y) {   // citizens_safety: the rally point while mustering
    if (idx >= 16 || !g_mus[idx].on) return 0;
    *x = g_mus[idx].rx;
    *y = g_mus[idx].ry;
    return 1;
}
internal ee_unit *mus_nearest_enemy(struct mus_scan *s, ee_unit *u) {
    ee_unit *best = NULL;
    int bd = 0x7FFFFFFF, x = unit_get_x(u), y = unit_get_y(u);
    for (int k = 0; k < s->ne; k++) {
        ee_unit *e = s->enemy[k];
        if (unit_get_hp(e) <= 0) continue;
        int d = ivec2_dist2(unit_get_tile(e), iv2(x, y));
        if (d < bd) { bd = d; best = e; }
    }
    return best;
}
internal void muster(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_muster || g_def_off[idx] || (now >= last[idx] && now - last[idx] < 3000)) return;
    last[idx] = now;
    void *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!bm || build_mgr_get_base_x(bm) == -10000) { g_mus[idx].on = 0; return; }
    struct mus_scan *s = &g_ms;
    memset(s, 0, sizeof *s);
    s->pl = pl;
    s->bx = build_mgr_get_base_x(bm);
    s->by = build_mgr_get_base_y(bm);
    if (g_fight_est) {
        st_side_clear(&g_ms_own); st_side_clear(&g_ms_foe);
        s->os = &g_ms_own; s->es = &g_ms_foe;
    }
    grid_each(mus_unit, s);
    // how the home side stands against the enemy in the base: summed hit points x attack, or the fight estimate
    float stand = g_fight_est ? st_ratio(&g_ms_own, &g_ms_foe) : (s->epow > 0 ? (s->opow + s->tpow) / s->epow : 1e6f);
    if (!g_mus[idx].on) {
        if (s->en < 8 || stand >= 0.5f ||
            (g_mus[idx].cool && now - g_mus[idx].cool < g_mus[idx].cool_len)) return;
        g_mus[idx].on = 1;
        g_mus[idx].moves = 0;
        g_mus[idx].since = now ? now : 1;
        g_mus[idx].episodes++;
        mus_rally(idx, s);
        if (g_log) fprintf(g_log, "# muster: P%u at %.1f min: %d enemy soldiers in the base (strength %.0f) against %d own "
                           "(%.0f) and towers (%.0f), %s %.2f - gathering at %.0f,%.0f\n", idx, now / 60000.0, s->en, s->epow, s->no, s->opow,
                           s->tpow, g_fight_est ? "estimate" : "ratio", stand, g_mus[idx].rx, g_mus[idx].ry);
        ai_say(pl, CHAT_GUARD, "[ai] base overrun (%d enemies), gathering at %.0f,%.0f", s->en, g_mus[idx].rx, g_mus[idx].ry);
    } else {
        if (s->en < 3 || stand >= 1.0f) {
            g_mus[idx].on = 0;
            g_mus[idx].cool = now ? now : 1;
            g_mus[idx].cool_len = 30000;
            if (g_log) fprintf(g_log, "# muster: P%u at %.1f min: %s - muster ends after %.1f min\n", idx, now / 60000.0,
                               s->en < 3 ? "the enemy is gone from the base" : "soldiers and towers can hold the base again",
                               (now - g_mus[idx].since) / 60000.0);
            return;
        }
        float rpow = 0;
        int rn = 0, near_rally = 0;
        if (g_fight_est) st_side_clear(&g_ms_rally);
        for (int k = 0; k < s->no; k++) {
            ee_unit *u = s->own[k];
            if (vec2_dist2(v2((float)unit_get_x(u), (float)unit_get_y(u)), v2(g_mus[idx].rx, g_mus[idx].ry)) <= 100) {
                rpow += unit_power(u); rn++;
                if (g_fight_est) st_side_add(&g_ms_rally, u);
            }
        }
        float gathered = g_fight_est ? st_ratio(&g_ms_rally, &g_ms_foe) : (s->epow > 0 ? rpow / s->epow : 1e6f);
        if (gathered >= 1.3f) {   // strike
            int ordered = 0;
            for (int k = 0; k < s->no; k++) {
                ee_unit *u = s->own[k];
                ee_unit *t = mus_nearest_enemy(s, u);
                if (t && unit_goal(u, goal_attack(u, t), 0, 0x1F40)) ordered++;
            }
            g_mus[idx].on = 0;
            g_mus[idx].cool = now ? now : 1;
            g_mus[idx].cool_len = 30000;
            g_mus[idx].strikes++;
            if (g_log) fprintf(g_log, "# muster: P%u at %.1f min: strike - %d gathered (strength %.0f) against %d enemies "
                               "(%.0f), %s %.2f, %d ordered\n", idx, now / 60000.0, rn, rpow, s->en, s->epow,
                               g_fight_est ? "estimate" : "ratio", gathered, ordered);
            ai_say(pl, CHAT_GUARD, "[ai] strike with %d against %d", rn, s->en);
            return;
        }
        for (int k = 0; k < s->ne && !near_rally; k++) {
            near_rally = vec2_dist2(v2((float)unit_get_x(s->enemy[k]), (float)unit_get_y(s->enemy[k])), v2(g_mus[idx].rx, g_mus[idx].ry)) <= 100;
        }
        if (near_rally && ++g_mus[idx].moves >= 3) {   // it follows: no getting away - fight where it is
            g_mus[idx].on = 0;
            g_mus[idx].cool = now ? now : 1;
            g_mus[idx].cool_len = 60000;
            int ordered = 0;
            for (int k = 0; k < s->no; k++) {
                ee_unit *u = s->own[k];
                ee_unit *t = mus_nearest_enemy(s, u);
                if (t && unit_goal(u, goal_attack(u, t), 0, 0x1F40)) ordered++;
            }
            if (g_log) fprintf(g_log, "# muster: P%u at %.1f min: the enemy follows to the rally point - fighting where it is "
                               "(%d ordered)\n", idx, now / 60000.0, ordered);
            return;
        }
        if (near_rally) {
            mus_rally(idx, s);
            if (g_log) fprintf(g_log, "# muster: P%u at %.1f min: enemies at the rally point - now %.0f,%.0f\n", idx,
                               now / 60000.0, g_mus[idx].rx, g_mus[idx].ry);
        }
    }
    int ordered = 0;   // everyone who isn't there or on the way
    for (int k = 0; k < s->no; k++) {
        ee_unit *u = s->own[k];
        ee_goal *goal = unit_get_goal(u);
        if (vec2_dist2(v2((float)unit_get_x(u), (float)unit_get_y(u)), v2(g_mus[idx].rx, g_mus[idx].ry)) <= 36) continue;
        if (goal && goal_is_location(goal) && fabsf(goal_get_dest_x(goal) - g_mus[idx].rx) < 2 &&
            fabsf(goal_get_dest_y(goal) - g_mus[idx].ry) < 2) continue;
        if (unit_goal(u, goal_move(g_mus[idx].rx, g_mus[idx].ry), 0, 0x1F40)) ordered++;
    }
    g_mus[idx].orders += ordered;
}

// HOLD THE TOWN ([ai_base] hold_town, default 0). Soldiers carry a behaviour (unit +0x208: an EEUnitBehavior of the
// table at [0x8D2044] - 0 Scout, 1 Stand Ground, 2 Guard, 3 Harass, 4 Cautious, 5 Aggressive; set by the player's
// command EEMSetUnitBehavior 0x4BFF1A for units whose type data +0x621 allows it). Every soldier is Aggressive: it
// chases what it sees. A player's army camped outside an AI's town (artillery, outranging the towers, with its
// guards) drew the defenders out one by one - user's game: P2 lost 320 units and killed 18 in 33 minutes, P3 285
// and 43, while the home guard replaced each. Every 3 s per AI (not while mustering): enemy soldiers (siege too) it
// sees within 35 tiles of its base against its home soldiers there, by hit points x attack (not the towers: a
// soldier that walks out leaves their range). Home: within 35 tiles, also members of an attack group still forming
// (the attack commander takes nearly all soldiers into its groups early), not those marching out. Weaker than 1.2 x
// the enemy: the home soldiers pull back to 6 tiles beyond the base, away from the enemy, and Guard there (held
// where they stood, the artillery shelled them: 15 of 15 lost in 1.5 minutes). 1.3 x the enemy or more: a sortie -
// all Aggressive, each attacks its nearest enemy, together (then 60 s before the next). No enemy near: Aggressive.
// tests/probes/camp.eeai (8 artillery, 16 machine gunners, 6 tanks camped 15-18 tiles from P2's Town Center, P2 with 12
// machine gunners of its own), 6 pairs: P2 lost 22 vs 31 units, killed 21 vs 26, the camp kept 11 vs 7 of 30 -
// fewer losses, but the camp (within the towers' support there) survives more often. A camp that plays like the
// user's (tests/probes/camp2.eeai: reinforced every 2 minutes, pushing in every 4 and pulling back): 5 pairs, P2 killed
// 27 vs 36, lost 51 vs 59, kept 4 vs 17 citizens and its Town Center 2 vs 5 times, the camp grew to 53 vs 46 - pulled
// back, they leave the town to the pushes. Off.
internal struct { int holding; uint32_t sortie_t; unsigned holds, sorties, set; } g_hold_st[16];
internal int landing_has_unit(ee_player *pl, ee_unit *u);   // landing.c
internal ee_unit *g_asg_units[512];
internal int g_asg_n;
internal void asg_units(ee_player *pl) {   // the units of AI pl's attack subgroups (records +0x44)
    g_asg_n = 0;
    ee_combat_mgr *cbm = player_get_combat_mgr(pl); ee_attack_cmd *cmd = cbm ? combat_mgr_get_commander(cbm) : NULL;
    if (!cmd) return;
    uint8_t *dq = attack_cmd_groups_at(cmd);
    unsigned first = deque_front(dq), cnt = deque_count(dq);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        ee_attack_group *g = deque_at(dq, first + k);
        for (ee_attack_subgroup **sp = g ? attack_group_get_subgroups_begin(g) : NULL; sp && sp < attack_group_get_subgroups_end(g); sp++) {
            ee_asg_unit **b = *sp ? attack_subgroup_get_units_begin(*sp) : NULL; ee_asg_unit **e = *sp ? attack_subgroup_get_units_end(*sp) : NULL;
            for (ee_asg_unit **r = b; b && r < e && g_asg_n < 512; r++)
                if (*r && asg_unit_get_unit(*r)) g_asg_units[g_asg_n++] = asg_unit_get_unit(*r);
        }
    }
}
internal void hold_town(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    if (!g_hold || g_def_off[idx] || (now >= last[idx] && now - last[idx] < 3000)) return;
    if (now < last[idx]) memset(&g_hold_st[idx], 0, sizeof g_hold_st[idx]);   // a new game
    last[idx] = now;
    if (g_mus[idx].on) return;   // the muster has them
    void *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    void **tab = behaviour_table();
    if (!bm || build_mgr_get_base_x(bm) == -10000 || !tab) return;
    struct mus_scan *s = &g_ms;
    memset(s, 0, sizeof *s);
    s->pl = pl;
    s->bx = build_mgr_get_base_x(bm);
    s->by = build_mgr_get_base_y(bm);
    grid_each(mus_unit, s);   // enemies within 22, own within 35: count the enemies within 35 here
    if (g_fight_est) { st_side_clear(&g_ms_own); st_side_clear(&g_ms_foe); }
    float epow = 0;
    int en = 0;
    ee_unit *enemy[128];
    {
        for (struct grid_it it = grid_it_box(s->bx, s->by, 35, 1); grid_it_next(&it); ) {
            ee_unit *u = it.u;
            ee_unit_type *td = it.td;
            if (unit_get_hp(u) <= 0) continue;
            int type = unit_type_get_id(td);
            ee_player *o = unit_get_owner(u);
            if (ivec2_dist2(unit_get_tile(u), iv2(s->bx, s->by)) > 35 * 35 || !o || o == pl || player_get_index(o) <= 0 || !is_enemy(pl, o)) continue;
            if ((!is_soldier(td) && !is_siege(type)) || !seen_by(u, pl)) continue;
            epow += unit_power(u);
            if (g_fight_est) st_side_add(&g_ms_foe, u);
            if (en < 128) enemy[en] = u;
            en++;
        }
    }
    asg_units(pl);
    ee_unit *home[256];
    int nh = 0;
    float hpow = 0;
    // home: every soldier within 35 tiles - also those of an attack group still forming there (the attack commander
    // takes nearly all soldiers into its groups long before they leave) - but not one marching out (a move order to a
    // point beyond 35 tiles); attack groups' members beyond 35 tiles: as the game has them (Aggressive)
    for (int j = 0; j < g_asg_n; j++) {
        ee_unit *u = g_asg_units[j];
        if (ivec2_dist2(unit_get_tile(u), iv2(s->bx, s->by)) > 35 * 35) unit_set_stance(u, 5);
    }
    for (int k = 0; k < s->no; k++) {
        ee_unit *u = s->own[k];
        ee_goal *g = unit_get_goal(u);
        if (landing_has_unit(pl, u)) continue;   // the landing leads them (it ordered loaded transports at their docks against raiders)
        if (g && goal_is_location(g)) {
            if (vec2_dist2(v2(goal_get_dest_x(g), goal_get_dest_y(g)), v2((float)s->bx, (float)s->by)) > 35.0f * 35.0f) { unit_set_stance(u, 5); continue; }   // marching out
        }
        home[nh++] = u;
        hpow += unit_power(u);
        if (g_fight_est) st_side_add(&g_ms_own, u);
    }
    // the home soldiers against the enemies near (towers left out: they don't follow the soldiers out of their range)
    float stand = g_fight_est ? st_ratio(&g_ms_own, &g_ms_foe) : (epow > 0 ? hpow / epow : 1e6f);
    if (en == 0) {   // nothing near: as the game has them
        for (int k = 0; k < nh; k++) unit_set_stance(home[k], 5);
        if (g_hold_st[idx].holding && g_log) fprintf(g_log, "# hold: P%u at %.1f min: no enemy near the town - soldiers free again\n", idx, now / 60000.0);
        g_hold_st[idx].holding = 0;
        return;
    }
    if (stand >= 1.3f && (!g_hold_st[idx].sortie_t || now - g_hold_st[idx].sortie_t >= 60000)) {
        int ordered = 0;
        for (int k = 0; k < nh; k++) {
            ee_unit *u = home[k];
            ee_unit *best = NULL;
            unit_set_stance(u, 5);
            int bd = 0x7FFFFFFF, x = unit_get_x(u), y = unit_get_y(u);
            for (int j = 0; j < en && j < 128; j++) {
                if (unit_get_hp(enemy[j]) <= 0) continue;
                int d = ivec2_dist2(unit_get_tile(enemy[j]), iv2(x, y));
                if (d < bd) { bd = d; best = enemy[j]; }
            }
            if (best && unit_goal(u, goal_attack(u, best), 0, 0x1F40)) ordered++;
        }
        g_hold_st[idx].sortie_t = now ? now : 1;
        g_hold_st[idx].holding = 0;
        g_hold_st[idx].sorties++;
        if (g_log) fprintf(g_log, "# hold: P%u at %.1f min: sortie - %d soldiers (strength %.0f) against %d enemies (%.0f), %d ordered\n",
                           idx, now / 60000.0, nh, hpow, en, epow, ordered);
        return;
    }
    if (stand < 1.2f) {   // towers don't count: they don't follow the soldiers out of their range
        // held where they stood, the artillery that outranges them shelled them there (camp test: 15 of 15 lost in 1.5
        // minutes): back to a point 6 tiles beyond the base, away from the enemy's centre, and Guard there
        float cx = 0, cy = 0, w = 0;
        for (int j = 0; j < en && j < 128; j++) {
            float p = unit_power(enemy[j]);
            cx += (float)unit_get_x(enemy[j]) * p; cy += (float)unit_get_y(enemy[j]) * p; w += p;
        }
        float dx = w > 0 ? (float)s->bx - cx / w : 1.0f, dy = w > 0 ? (float)s->by - cy / w : 0.0f, l = sqrtf(dx * dx + dy * dy);
        if (l < 1.0f) { dx = 1.0f; dy = 0.0f; l = 1.0f; }
        float hx = (float)s->bx + 0.5f + dx / l * 6.0f, hy = (float)s->by + 0.5f + dy / l * 6.0f;
        int changed = 0;
        for (int k = 0; k < nh; k++) {
            ee_unit *u = home[k];
            ee_goal *g = unit_get_goal(u);
            changed += unit_set_stance(u, 2);
            float ux = (float)unit_get_x(u) - hx, uy = (float)unit_get_y(u) - hy;
            if (ux * ux + uy * uy <= 16.0f) continue;
            if (g && goal_is_location(g) && fabsf(goal_get_dest_x(g) - hx) < 2 && fabsf(goal_get_dest_y(g) - hy) < 2) continue;
            unit_goal(u, goal_move(hx + (float)(k % 5) - 2.0f, hy + (float)(k / 5 % 5) - 2.0f), 0, 0x1F40);
        }
        g_hold_st[idx].set += (unsigned)changed;
        if (!g_hold_st[idx].holding) {
            g_hold_st[idx].holds++;
            if (g_log) fprintf(g_log, "# hold: P%u at %.1f min: %d enemies near the town (strength %.0f) against %d soldiers (%.0f) "
                               "and towers (%.0f) - holding (Guard)\n", idx, now / 60000.0, en, epow, nh, hpow, s->tpow);
        }
        g_hold_st[idx].holding = 1;
    }
}

// basemap P [r]: the map around AI player P's base (build manager +0x12C/+0x130), 2 characters per tile:
// T tower (own defence with a ground attack), W wall, G gate, C Town Center/Capitol/Settlement, b other own building,
// c citizen, s soldier, m mine (stone/gold/iron site), digits = other players' buildings/units (player index),
// o planned tower slot (defence layout).
struct basemap { ee_player *pl; int x0, y0, r; char *g; };
internal void basemap_unit(ee_unit *u, void *ctx) {
    struct basemap *m = ctx;
    int x = unit_get_x(u) - m->x0 + m->r, y = unit_get_y(u) - m->y0 + m->r, w = 2 * m->r + 1;
    if (x < 0 || y < 0 || x >= w || y >= w) return;
    ee_unit_type *td = unit_get_type_data(u);
    ee_player *owner = unit_get_owner(u);
    int type = unit_type_get_id(td), fam = unit_type_get_class_id(td);
    char ch, *cell = &m->g[y * w + x];
    float r, a;
    if (owner != m->pl) {
        if (!owner) return;
        int pi = player_get_index(owner);
        if (pi <= 0) return;   // nature / gaia
        ch = (char)('0' + pi % 10);
    } else if (is_wall(type)) ch = ut_role(type, UTR_GATE) ? 'G' : 'W';
    else if (defence_stats(u, &r, &a)) ch = 'T';
    else if (UT_IN(type, UT_TOWNS, UT_X_SINGLE_PLAYER_SETTLEMENT)) ch = 'C';
    else if (fam == CLS_BUILDING || fam == CLS_FORTRESS) ch = 'b';
    else if (is_citizen_type(type)) ch = 'c';
    else ch = 's';
    static const char prio[] = ".mcs0123456789bWGCT";   // what wins when several things share a tile
    if (strchr(prio, ch) > strchr(prio, *cell)) *cell = ch;
}
internal void cmd_basemap(char *out, const char *args) {
    int p = 0, r = 20;
    sscanf(args, "%d %d", &p, &r);
    if (r < 5) r = 5;
    if (r > 40) r = 40;
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    void *cm = pl ? player_get_civ_mgr(pl) : NULL;
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!bm) { REPLY("error: no AI player %d\n", p); return; }
    static char g[81 * 81];
    int w = 2 * r + 1;
    memset(g, '.', (size_t)(w * w));
    struct basemap m = { pl, build_mgr_get_base_x(bm), build_mgr_get_base_y(bm), r, g };
    ee_zone **zb = player_get_zones_begin(pl); ee_zone **ze = player_get_zones_end(pl);   // mines
    for (int z = 0; zb && zb + z < ze; z++)
        for (int t = 4; zb[z] && t < 7; t++) {
            ee_unit *s = zone_site(zb[z], t);
            if (!s) continue;
            int x = unit_get_x(s) - m.x0 + r, y = unit_get_y(s) - m.y0 + r;
            if (x >= 0 && y >= 0 && x < w && y < w) g[y * w + x] = 'm';
        }
    grid_each(basemap_unit, &m);
    unsigned pi = (unsigned)p & 15;   // planned tower slots of the defence layout: o; the enclosing ring: ,
    for (int i = 0; encl_on(pi) && g_encs[pi].pl == pl && i < g_encs[pi].nring; i++) {
        int gx = g_encs[pi].rx[i] - m.x0 + r, gy = g_encs[pi].ry[i] - m.y0 + r;
        if (gx >= 0 && gy >= 0 && gx < w && gy < w && g[gy * w + gx] == '.') g[gy * w + gx] = ',';
    }
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
// `ringmap P [r]`: the ground round AI player P's base point for a picture of its walls (scripts/results/ringmap.py),
// one character a tile, a row per y: . walkable, ~ water, ^ other ground no citizen walks, t tree, m mine or other
// resource, W its wall, w its wall going up, G its gate, T its tower, C its Town Center, B its other buildings, E another
// player's building or wall, x something else in the way, , its ring's tile still open, o a tower spot of its ring
internal void cmd_ringmap(char *out, const char *args) {
    int p = 0, r = 50;
    sscanf(args, "%d %d", &p, &r);
    if (r < 5) r = 5;
    if (r > 56) r = 56;
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    void *cm = pl ? player_get_civ_mgr(pl) : NULL;
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    ee_map *map = game_map();
    int n = map ? (int)map_get_size(map) : 0;
    if (!bm || !n || build_mgr_get_base_x(bm) == -10000) { REPLY("error: no AI player %d with a base\n", p); return; }
    int x0 = build_mgr_get_base_x(bm) - r, y0 = build_mgr_get_base_y(bm) - r, w = 2 * r + 1;
    struct encl *e = &g_encs[(unsigned)p & 15];
    int ring = encl_on((unsigned)p & 15) && e->pl == pl;
    int nx = 0, xt[8], xc[8];   // what the x tiles hold
    REPLY("P%d ringmap %d,%d %d (x right, y down)\n", p, x0, y0, w);
    for (int y = y0; y < y0 + w; y++) {
        char line[2 * 56 + 3];
        for (int x = x0; x < x0 + w; x++) {
            char c = '^';
            if (x >= 0 && y >= 0 && x < n && y < n) {
                ee_tile *tile = map_get_tiles(map)[y * n + x];
                c = rm_tile_ok(map, n, x, y) ? '.' : area_id(iv2(x, y), 1) ? '~' : '^';
                for (void *node = tile ? tile_get_objects(tile) : NULL; node; node = tilenode_get_next(node)) {
                    ee_unit *o = tilenode_get_object(node);
                    ee_unit_type *td = o ? unit_get_type_data(o) : NULL;
                    ee_unit_stats *st = o ? unit_get_stats(o) : NULL;
                    if (!td || (st && unit_stats_get_speed(st) > 0) || unit_get_hp(o) <= 0) continue;
                    int t = unit_type_get_id(td), cls = unit_type_get_class_id(td);
                    ee_player *ow = unit_get_owner(o);
                    ee_unit *ph = unit_get_physics(o);
                    float fr, fa;
                    char ch = cls == CLS_RESOURCE ? (unit_type_get_resource_type(td) == 3 ? 't' : 'm')
                            : ow != pl ? (ow && player_get_index(ow) > 0 && (cls == CLS_BUILDING || cls == CLS_FORTRESS || unit_type_get_is_wall(td)) ? 'E' : 0)
                            : unit_type_get_is_wall(td) ? (ut_role(t, UTR_GATE) ? 'G' : ph && unit_get_finished(ph) ? 'W' : 'w')
                            : defence_stats(o, &fr, &fa) ? 'T' : UT_IN(t, UT_TOWNS) ? 'C' : (cls == CLS_BUILDING || cls == CLS_FORTRESS) ? 'B' : 0;
                    if (ch) { c = ch; break; }
                    if (c == '^') {   // in the way, none of the above: x, the first few named after the map
                        c = 'x';
                        if (nx < 8) { xt[nx] = t; xc[nx++] = cls; }
                    }
                }
            }
            if (ring && c == '.' && encl_ring_has(e, x, y)) c = ',';
            line[x - x0] = c;
        }
        line[w] = 0;
        REPLY("%s\n", line);
    }
    for (int i = 0; ring && i < e->nslot; i++) REPLY("slot %d,%d\n", e->sx[i], e->sy[i]);
    for (int i = 0; ring && i < e->ngate; i++) REPLY("gate %d,%d dir %d\n", e->gate[i].x, e->gate[i].y, e->gate[i].dir);
    for (int i = 0; i < nx; i++) REPLY("x type %d class %d\n", xt[i], xc[i]);
}


// defence P [0|1]: switch the defence layout off/on for one player (scenario A/B tests)
internal void cmd_defence(char *out, const char *args) {
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
__attribute__((used)) ee_unit *g_dmg_unit;   // the effect's unit: esi at the call site
internal uint8_t *rb_find(uint8_t *map, unsigned key) {   // MSVC map: +4 head; node +0 left, +4 parent, +8 right, +0xC key, +0x15 nil
    uint8_t *head = FIELD(map, 4, uint8_t *), *res, *n;
    if (!head) return NULL;
    res = head; n = FIELD(head, 4, uint8_t *);
    while (n && !n[0x15]) {
        if (FIELD(n, 0xC, unsigned) < key) n = FIELD(n, 8, uint8_t *);
        else { res = n; n = FIELD(n, 0, uint8_t *); }
    }
    return (res == head || key < FIELD(res, 0xC, unsigned)) ? NULL : res;
}
__attribute__((used)) int __thiscall hook_damage_points(ee_dmg_model *model, int index, unsigned state) {
    uint8_t *n = model ? rb_find(dmg_model_map_at(model), state) : NULL, *sub = n ? FIELD(n, 0x10, uint8_t *) : NULL;
    if (n && (!sub || !FIELD(sub, 4, void *))) {
        static int logged;
        ee_unit *u = g_dmg_unit;
        ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
        if (g_log && logged < 20) {
            logged++;
            fprintf(g_log, "# engine guard: damage points of a model without them - unit type %d owner P%d at %d,%d, state %u index %d\n",
                    td ? unit_type_get_id(td) : -1, u && unit_get_owner(u) ? player_get_index(unit_get_owner(u)) : -1,
                    u ? unit_get_x(u) : -1, u ? unit_get_y(u) : -1, state, index);
            fflush(g_log);
        }
        return 0;
    }
    return dmg_model_damage_points(model, index, state);
}
__attribute__((naked, used)) void damage_points_thunk(void) {
    __asm__("movl %esi, _g_dmg_unit\n\t"
            "jmp _hook_damage_points");
}

// THE GAME'S WALLS ([ai_base] game_walls, default 0 since 2026-10-27; the player: the AI's own wall building is between
// useless and annoying, a little ugly too - and much of the deadly build orders, citizens sent one by one to wall
// segments at the front): the wall manager's update (EECPWallManager 0x499E20, called by the civilization manager's
// update at 0x46E3E1 - its ring around the base, one segment at a time, a stray piece with a gate in practice) is
// skipped for the mod's AIs. The walls defence_walls builds go through the wall manager's build_wall, not its update.
internal int g_game_walls = 0;
internal void __thiscall hook_wall_update(ee_wall_mgr *wm, uint32_t now) {
    for (int i = 1; !g_game_walls && i < 16 && (unsigned)i < game_player_count(); i++) {
        ee_player *pl = game_players()[i];
        ee_civ_mgr *cm = pl && g_players[i] && !is_vanilla(pl) ? player_get_civ_mgr(pl) : NULL;
        if (cm && civ_mgr_get_wall_mgr(cm) == wm) return;
    }
    wall_mgr_update(wm, now);
}
internal void defence_install(const char *autotest_ini) {
    g_game_walls = g_set_ai[1].game_walls;
    if (!patch_wall_update(hook_wall_update) && g_log) fprintf(g_log, "# defence: the wall manager's update not hooked - the game's walls stay\n");
    g_def = g_set_ai[1].defence_layout;
    g_def_dist = g_set_ai[1].defence_distance;
    g_def_walls = g_set_ai[1].defence_walls;
    g_def_more = g_set_ai[1].defence_towers;
    g_encl = g_set_ai[1].enclose;
    g_ring = g_set_ai[1].base_ring;
    if (autotest_ini) g_v_ring = ini_int("autotest", "v_base_ring", -1, autotest_ini);
    g_coast_towers = g_set_ai[1].coast_towers;
    g_site_forts = g_set_ai[1].site_forts;
    g_shore_guns = g_set_ai[1].shore_guns;
    g_shore_walls = g_set_ai[1].shore_walls;
    g_encl_from = g_set_ai[1].enclose_citizens;
    g_encl_runs = g_set_ai[1].enclose_runs;
    g_encl_flight = g_set_ai[1].enclose_in_flight;
    g_encl_ships = g_set_ai[1].enclose_under_ships;
    g_encl_docks = g_set_ai[1].enclose_dock_gates;
    g_encl_safe = g_set_ai[1].enclose_safe_tiles;
    g_csiege = g_set_ai[1].counter_siege;
    g_militia = g_set_ai[1].citizen_militia;
    g_militia_est = g_set_ai[1].militia_estimate;
    g_raid_held = g_set_ai[1].raid_held;
    g_militia_share = g_set_ai[1].militia_caught_share;
    g_home_guard = g_set_ai[1].home_guard;
    g_home_share = g_set_ai[1].home_guard_share;
    g_muster = g_set_ai[1].muster;
    g_fight_est = g_set_ai[1].fight_estimate;
    g_aa = g_set_ai[1].anti_air;
    g_aa_ahead = g_set_ai[1].aa_ahead;
    g_aa_exp = g_set_ai[1].aa_expected;
    g_aa_strong = g_set_ai[1].aa_strong;
    g_aa_ahead_b = g_set_ai[1].aa_ahead_buildings;
    g_hold = g_set_ai[1].hold_town;
    BOOL guard = patch_damage_points(damage_points_thunk);
    if (g_log) fprintf(g_log, "# engine guard: damage points %s\n", guard ? "ok" : "FAILED");
    if (g_log) fprintf(g_log, "# defence: layout=%d distance=%d walls=%d enclose=%d base_ring=%d/%d own towers=%d counter-siege=%d home guard=%d/epoch, %.2f of the military room, fight_estimate=%d hold_town=%d\n", g_def, g_def_dist, g_def_walls, g_encl, g_ring, g_v_ring, g_def_more, g_csiege, g_home_guard, g_home_share, g_fight_est, g_hold);
}

// ANTI-AIR ([ai_base] anti_air, default 1). The game's unit choice hardly asks for anti-air: the user's planes overran
// both AIs' towns, one asked for an AA tank once in 5 minutes among hundreds of orders, built 2 88mm guns and lost its
// few AA tanks for good. Every 5 s: enemy aircraft it can see within 25 tiles of its drop sites (the most seen in the
// last 5 minutes counts). While there were any: it wants 2 + half their number in anti-air (at most 16), a quarter of
// that (1..4) as AA buildings next to the drop site nearest to where the aircraft were - the rest AA units, the
// newest of its epoch, up to 3 at a time every 15 s; one building at a time in the build queue, a minute apart.
// Orders as the siege's (priority: the stock above the epoch reserve, not the army's account); losses are replaced.
// AHEAD ([ai_base] aa_ahead, default 1; the playtester: "they only build as reaction? then it's already too late, getting
// them up while the town is actively bombed is nearly impossible"): from World War I, once an enemy is there too, AA
// stands before any aircraft come - an AA building at each of the first aa_ahead_buildings (3) drop sites nearest the
// base point that has none within 7 tiles, and 2 AA units; enemy aircraft seen anywhere count (half of them, they fly
// fast), and the most seen is kept 10 minutes instead of 5. tests/scenarios/bombing.eeai (10 seeds, P3 reactive vs ahead): 25
// aircraft and 4 bombers - workers 36 vs 52, buildings lost 4.3 vs 1.2, losses 25 vs 19; 12 aircraft (they did little
// harm) - workers 65 vs 57: the guns cost a little where the air threat stays small.
internal const struct { enum ut_name type; int epoch; } k_aa_units[] = {
    { UT_AA13_TANK_SKYWATCHER, 13 }, { UT_AA10_STINGER_SOLDIER, 12 }, { UT_AA10_TANK, 10 } };
internal const struct { enum ut_name type; int epoch; } k_aa_bldgs[] = { { UT_B_AA13_MISSILE_TOWER, 13 }, { UT_B_AA10_GUN_88MM, 10 } };
// an aircraft's weight in the threat (STRONG below): in fighters' worth, 1 + attack / 100 (fighters 1.2, the bombers 2-2.8,
// the late fighter-bombers 2.9-3.9), at most 6; an atomic bomber 6; none without an attack (transports, balloons)
internal float aa_weight(ee_unit_type *td) {
    if (!td) return 0;
    if (unit_type_get_family(td) == KIND_ATOMIC_BOMBER) return 6.0f;
    int at = unit_type_get_attack(td);
    if (at <= 0) return 0;
    float w = 1.0f + (float)at / 100.0f;
    return w > 6.0f ? 6.0f : w;
}
internal int aa_named_unit(int t) { for (size_t k = 0; k < sizeof k_aa_units / sizeof *k_aa_units; k++) if (t == g_ut[k_aa_units[k].type]) return 1; return 0; }
internal int aa_named_bldg(int t) { for (size_t k = 0; k < sizeof k_aa_bldgs / sizeof *k_aa_bldgs; k++) if (t == g_ut[k_aa_bldgs[k].type]) return 1; return 0; }
struct aa_scan { ee_player *pl; int nair, have_u, have_b, nall, nbx, bxy[256][2]; float axs, ays, wair, wall; };
internal void aa_scan_unit(ee_unit *u, void *ctx) {   // anti_air: own AA, enemy aircraft
    struct aa_scan *a = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    if (unit_get_hp(u) <= 0) return;
    int fam = unit_type_get_class_id(td), t = unit_type_get_id(td);
    ee_player *o = unit_get_owner(u), *pl = a->pl;
    if (o == pl) {
        for (size_t k = 0; k < sizeof k_aa_units / sizeof *k_aa_units; k++) a->have_u += t == g_ut[k_aa_units[k].type];
        if (g_aa_strong || g_v_aa_strong > 0) {   // STRONG: every type of the families, a data mod's too
            int f = unit_type_get_family(td);
            if (f == KIND_LAND_AA && !aa_named_unit(t)) a->have_u++;
            if (f == KIND_AA_BUILDING && !aa_named_bldg(t)) {
                a->have_b++;
                if (a->nbx < 256) { a->bxy[a->nbx][0] = unit_get_x(u); a->bxy[a->nbx++][1] = unit_get_y(u); }
            }
        }
        for (size_t k = 0; k < sizeof k_aa_bldgs / sizeof *k_aa_bldgs; k++)
            if (t == g_ut[k_aa_bldgs[k].type]) {
                a->have_b++;
                if (a->nbx < 256) { a->bxy[a->nbx][0] = unit_get_x(u); a->bxy[a->nbx++][1] = unit_get_y(u); }
            }
        return;
    }
    if ((fam != CLS_AIRPLANE && fam != CLS_HELICOPTER && fam != CLS_HELICOPTER_TRANSPORT) || !o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !seen_by(u, pl)) return;
    a->nall++; a->wall += aa_weight(td);
    int x = unit_get_x(u), y = unit_get_y(u);
    ivec2 d;
    if (!nearest_drop(pl, iv2(x, y), 0, &d) || ivec2_dist2(d, iv2(x, y)) > 625) return;
    a->nair++; a->wair += aa_weight(td); a->axs += (float)x; a->ays += (float)y;
}

// STRONG ANTI-AIR ([ai_base] aa_strong, off until measured; [autotest] v_aa_strong; the player, 2026-11-11: "anti aa in
// general is still far too weak ... the game has nuclear bombers, and even normal ones can ruin an economy. right now
// building up air forces is an easy way to outplay the ai, it needs aa units and more static aa, i often place 2x2 aa
// units in a block or line, while the ai occasionally places a single one"). In place of the counts above, every 5 s:
// - THE THREAT, in fighters' worth (aa_weight): the most of the aircraft it saw near its towns and anywhere (kept
//   AAS_KEEP), the air force it expects of its enemies (army_view.c aa_threat_expected: what each fields, and the
//   aircraft among its counters to what it saw of the AI's), and before any is seen AAS_FLOOR - an enemy in World War I
//   or later - or AAS_FLOOR_PORT - an enemy airport it knows: aircraft can come any minute.
// - STATIC AA IN BLOCKS: its places - its Town Centers, Capitols and Settlements first, then its other drop sites, the
//   nearest its base first - each want a block of AA buildings within AAS_R: 2 while the threat is below AAS_BIG, else
//   4 - a 2x2 of the building's footprint where it fits, else a line -, laid out at once as foundations (paid, as the
//   build manager's placing does) with AAS_BUILDERS of its nearest gathering citizens each (bunker.c bk_builders), a
//   block every AAS_EVERY - through the build manager's queue they came too late: in the test 54 queued by minute 27, 1
//   standing, 158 queued to 14 by the end. Its foundations it keeps builders on, each counted for its place, until done
//   or AAS_FOUND_MS. One place per AAS_PER_SITE of the threat (one at least), as many as it has. Places within AAS_R of
//   one already chosen are that one's; each AA building counts for the place nearest it. Without a real threat (the row
//   "air", priority.c) a foundation waits for the rows as a building does (prio_refuses).
// - AA UNITS: 2 + the threat, no more than a quarter of its unit limit; the newest land AA type the game offers it now
//   (the game's data: family land_aa), up to AAS_TRAIN every 10 s; without a building that makes it (the type's makers
//   in the game's data) one is asked of the build manager first (the test's P3 in World War I wanted 19-22 and had
//   none: no factory for the Flak halftrack).
// The AA building type: the newest of family aa_building the game offers it. "# anti-air: P<n> strong ..." a minute.
// Measured (tests/scenarios/air_war.eeai, air_war_heavy.eeai: Small Islands, P2's growing air force at P3's town, 8 games
// each, off / on): World War I fighter-bombers - citizens killed from the air 7.8 / 8.2, buildings lost 3.9 / 2.6,
// aircraft shot down 30 / 35, workers 138 / 127, 28 AA buildings and 12.5 AA units standing; World War II B-17s, ME262s
// and B-29s - citizens killed from the air 15.8 / 10.2 (se 4.9 / 3.8), units lost 52 / 37, buildings lost 6.5 / 5.0,
// workers 111 / 102. The scripted air forces barely hurt even the AI without it (its citizens grew from 13 to 84 under
// the bombs): no measure yet of what the player's air force does - it picks the economy's targets, pulls planes back,
// masses them. Massed raids as the player flies them (tests/scenarios/air_raid.eeai: 30 B-17s, air_raid_nuclear.eeai: 30
// B-29s, the pipe's airraid - anti-air first, half lost: home and filled up again; 60 min, 8 games each, off / on): B-17s
// citizens killed from the air 67 / 27, buildings lost 36 / 20, but its citizens at the end 19 / 24 and the town dead as
// soon (3 of the 8 on cut short); B-29s 46 / 53, 1 / 3 citizens left - nothing. It trained no AA unit at all: its maker
// waited in the game's build list the whole game (now laid out by itself). With that (4 seeds each): 2 - 6 AA units up
// by the first run, the raid's whole runs at its anti-air, half of them shot down by minute 34 - and the refilled wave
// razed the 8 AA buildings by minute 44, the AA units never trained again: its citizens at minute 30 / 40 / 50 25 / 27
// / 1 against 34 / 29 / 24 off (B-17s), the B-29s' towns dead by minute 30 either way. Static AA and a few AA units
// don't hold a wave that comes back full; off.
#define AAS_KEEP 600000u
#define AAS_FLOOR 3.0f
#define AAS_FLOOR_PORT 6.0f
#define AAS_BIG 8.0f
#define AAS_PER_SITE 6.0f
#define AAS_R 8
#define AAS_R_MAKER 16   // the building that makes its AA units: this far round its base
#define AAS_EVERY 20000u
#define AAS_TRAIN 5
#define AAS_BUILDERS 2
#define AAS_FOUND_MS 300000u
#define AAS_FOUNDS 32
#define AA_STRONG(idx) PV(idx, g_v_aa_strong, g_aa_strong)
internal float aa_threat_expected(ee_player *pl, unsigned idx, uint32_t now, int *port);   // army_view.c
internal struct { float w_near, w_all; uint32_t near_t, all_t, block_t, train_t, log_t, prod_t, pf_t, pf_b_t; unsigned prods; ee_unit *pf_u; int pf_id; struct { ee_unit *u; int id; uint32_t t, b_t; } fnd[AAS_FOUNDS]; int nf; unsigned blocks, queued, trained, done, gave_up; } g_aas[16];
// the newest type of family fam the game offers pl now (the strongest of those: hit points x attack); 0 none
internal int aa_type_of(ee_player *pl, int fam, int land_unit) {
    int best = 0, bep = -1; float bs = 0;
    ee_tech_tree *tt = player_get_tech_tree(pl);
    for (int t = 0; tt && t < G_TYPES_N; t++) {
        ee_unit_type *td = type_data(t);
        if (!td || unit_type_get_id(td) != t || unit_type_get_family(td) != fam || unit_type_get_attack(td) <= 0) continue;
        if (land_unit && unit_type_get_class_id(td) != CLS_LAND) continue;
        if (!prio_buildable(pl, t)) continue;
        ee_tech *x = tech_tree_lookup(tt, unit_type_get_tech(td));
        int ep = x ? tech_get_epoch(x) : 0;
        float sc = (float)unit_type_get_hp(td) * (float)unit_type_get_attack(td);
        if (ep > bep || (ep == bep && sc > bs)) { best = t; bep = ep; bs = sc; }
    }
    return best;
}
// a block of n (2 or 4) of type bt at site p: a 2x2 (or 2 side by side) of its footprint, else a line, the nearest spot
// out from p (3 .. rmax tiles); the spots in out, how many (0: none fits)
internal int aa_block_at(ee_player *pl, int bt, ivec2 p, int n, ivec2 *out, int rmax) {
    ee_unit_type *td = type_data(bt);
    int sz = td ? unit_type_get_size(td) : 1;
    if (sz < 1) sz = 1;
    static const int sq[4][2] = { { 0, 0 }, { 1, 0 }, { 0, 1 }, { 1, 1 } };
    for (int shape = 0; shape < 3; shape++)   // 0: the square, 1: a line along x, 2: along y
        for (int r = 3; r <= rmax; r++)
            for (int ax = -r; ax <= r; ax++)
                for (int ay = -r; ay <= r; ay++) {
                    if (abs(ax) != r && abs(ay) != r) continue;
                    ivec2 o = iv2(p.x + ax, p.y + ay);
                    if (raiders_near(pl, o, 8)) continue;
                    int ok = 1;
                    for (int k = 0; k < n && ok; k++) {
                        ivec2 q = shape == 0 ? iv2(o.x + sq[k][0] * sz, o.y + sq[k][1] * sz) : shape == 1 ? iv2(o.x + k * sz, o.y) : iv2(o.x, o.y + k * sz);
                        ok = can_place(pl, bt, q);
                        out[k] = q;
                    }
                    if (ok) return n;
                }
    return 0;
}
internal void am_bld(void *um, int type, int *have, int *busy);   // army.c
internal int bk_builders(ee_player *pl, ee_unit *b, int n);            // bunker.c: n builders for b, the nearest gathering citizens
// the building that makes type t (the first building among its tech's makers in the game's data); 0 none
internal int aa_producer(ee_player *pl, int t) {
    ee_unit_type *td = type_data(t);
    ee_tech_tree *tt = player_get_tech_tree(pl);
    ee_tech *x = td && tt ? tech_tree_lookup(tt, unit_type_get_tech(td)) : NULL;
    ee_tech_data *d = x ? tech_get_data(x) : NULL;
    int *b = d ? tech_data_get_makers_begin(d) : NULL, *e = d ? tech_data_get_makers_end(d) : NULL;
    for (int *q = b; b && e > b && e - b < 16 && q < e; q++) {
        ee_unit_type *m = type_data(*q);
        int c = m ? unit_type_get_class_id(m) : -1;
        if (c == CLS_BUILDING || c == CLS_FORTRESS) return *q;
    }
    return 0;
}
internal int aa_is_town(ee_unit *u) {
    int t = u ? unit_type_get_id(unit_get_type_data(u)) : -1;
    return t >= 0 && (t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL) || t == UT(B_SETTLEMENT));
}
internal void aa_strong(ee_player *pl, unsigned idx, uint32_t now, const struct aa_scan *a, int epoch) {
    idx &= 15;
    __typeof__(g_aas[0]) *z = &g_aas[idx];
    if (now < z->log_t || now < z->block_t || now < z->train_t) memset(z, 0, sizeof *z);   // a new game
    if (a->wair >= z->w_near || now - z->near_t > AAS_KEEP) { z->w_near = a->wair; z->near_t = now ? now : 1; }
    if (a->wall >= z->w_all || now - z->all_t > AAS_KEEP) { z->w_all = a->wall; z->all_t = now ? now : 1; }
    int port = 0, ahead = 0;
    float ex = aa_threat_expected(pl, idx, now, &port);
    for (int q = 1; q < 16 && !ahead; q++) {
        void *o = st_player(q);
        ahead = o && o != pl && is_enemy(pl, o) && current_epoch(o) + 1 >= 10;
    }
    float threat = z->w_near > z->w_all ? z->w_near : z->w_all;
    if (ex > threat) threat = ex;
    float least = port ? AAS_FLOOR_PORT : ahead ? AAS_FLOOR : 0.0f;
    if (least > threat) threat = least;
    g_aa_urgent[idx] = 0; memset(g_aa_hold[idx], 0, sizeof g_aa_hold[idx]); g_aa_maker[idx] = 0;
    if (threat <= 0) return;
    // priority.c the row "air": aircraft over its towns, an enemy airport, or its first 2 AA buildings once enemy aircraft
    // are seen anywhere or expected (the test's town had none standing when the first wave came: the farms first; from
    // minute 0 on, before any of that, it cost the economy the army then broke)
    g_aa_urgent[idx] = z->w_near > 0 || least >= AAS_FLOOR_PORT || (a->have_b < 2 && (z->w_all > 0 || ex > 0));
    int block = threat < AAS_BIG ? 2 : 4;
    int sites = 1 + (int)(threat / AAS_PER_SITE);
    // the places, towns first then the nearest its base; covered: block AA buildings within AAS_R, or a block queued there
    // within the last 2 minutes
    unsigned pi = idx;
    void *cm0 = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm0 ? civ_mgr_get_build_mgr(cm0) : NULL;
    ivec2 base = bm ? build_mgr_get_base(bm) : iv2(0, 0);
    drops_cached(pl);
    int used[64] = { 0 }, covered = 0, want_site = -1, chosen[64], nch = 0, cnt[64] = { 0 };
    while (nch < sites && nch < 64) {
        int bi = -1, bd = 0x7FFFFFFF, btown = 0;
        for (int i = 0; i < g_drops[pi].n && i < 64; i++) {
            const struct drop *d = &g_drops[pi].d[i];
            if (used[i] || d->food) continue;
            int town = aa_is_town(d->u), dd = ivec2_dist2(d->p, base);
            if (bi < 0 || town > btown || (town == btown && dd < bd)) { bi = i; bd = dd; btown = town; }
        }
        if (bi < 0) break;
        used[bi] = 1;
        int near_one = 0;
        for (int c = 0; c < nch && !near_one; c++) near_one = ivec2_dist2(g_drops[pi].d[chosen[c]].p, g_drops[pi].d[bi].p) <= AAS_R * AAS_R;
        if (!near_one) chosen[nch++] = bi;
    }
    for (int k = 0; k < a->nbx; k++) {   // each AA building for the place nearest it
        int bc = -1, bd = (AAS_R + 3) * (AAS_R + 3) + 1;
        for (int c = 0; c < nch; c++) {
            int dd = ivec2_dist2(g_drops[pi].d[chosen[c]].p, iv2(a->bxy[k][0], a->bxy[k][1]));
            if (dd < bd) { bd = dd; bc = c; }
        }
        if (bc >= 0) cnt[bc]++;
    }
    // its foundations going up: builders kept on them, each counted for the place nearest it
    int nf = 0, going = 0;
    for (int k = 0; k < z->nf; k++) {
        ee_unit *f = z->fnd[k].u;
        int live = f && unit_get_serial(f) == z->fnd[k].id && unit_get_owner(f) == pl;
        ee_unit *bs = live ? unit_get_physics(f) : NULL;
        if (live && bs && unit_get_finished(bs)) { z->done++; continue; }   // standing: the scan counts it
        if (!live || now - z->fnd[k].t > AAS_FOUND_MS) { z->gave_up += live; continue; }
        if (!z->fnd[k].b_t || now - z->fnd[k].b_t >= 15000) { z->fnd[k].b_t = now ? now : 1; bk_builders(pl, f, AAS_BUILDERS); }
        int bc = -1, bd = (AAS_R + 3) * (AAS_R + 3) + 1;
        for (int c = 0; c < nch; c++) {
            int dd = ivec2_dist2(g_drops[pi].d[chosen[c]].p, unit_get_tile(f));
            if (dd < bd) { bd = dd; bc = c; }
        }
        if (bc >= 0) cnt[bc]++;
        going++;
        z->fnd[nf++] = z->fnd[k];
    }
    z->nf = nf;
    for (int c = 0; c < nch; c++) {
        int have = cnt[c];
        if (have >= block) covered++;
        else if (want_site < 0) want_site = chosen[c];
    }
    sites = nch;
    // AA units
    int lim = (int)game_unit_limit(pl) / 4, want_u = 2 + (int)(threat + 0.5f);
    if (lim < 4) lim = 4;
    if (want_u > lim) want_u = lim;
    void *um = cm0 ? civ_mgr_get_unit_mgr(cm0) : NULL;
    int ut = aa_type_of(pl, KIND_LAND_AA, 1), prod = ut ? aa_producer(pl, ut) : 0, prod_have = 0, prod_busy = 0;
    if (um && prod) am_bld(um, prod, &prod_have, &prod_busy);
    if (prod && !prod_have) g_aa_maker[idx] = prod;
    if (g_aa_urgent[idx]) {   // the row "air" holds a round of units and the block still missing
        int nu = want_u - a->have_u > AAS_TRAIN ? AAS_TRAIN : want_u - a->have_u, bt0 = aa_type_of(pl, KIND_AA_BUILDING, 0);
        int nb = want_site >= 0 ? block : 0;
        for (int r = 2; r < 7; r++)
            g_aa_hold[idx][r] = (ut && nu > 0 && prod_have ? nu * unit_cost(pl, ut, r) : 0) + (bt0 ? nb * unit_cost(pl, bt0, r) : 0) +
                                (prod && !prod_have ? unit_cost(pl, prod, r) : 0);
    }
    // nothing makes them: the building first, laid out by itself with builders kept on it (asked of the build manager it
    // never came - the massed raids' 8 games: "makers asked for 0", 0 AA units of up to 77 wanted: the game's list of
    // epoch 10 held the Tank Factory all game, so queued() said yes and the ask never went out; the game's request adds
    // nothing for a type already listed either. Its list works only its front entry each 3 s, one building a round, an
    // entry it can't build now to the back - in ordinary games its entries go up within 1 - 8 minutes)
    ee_unit *pf = z->pf_u && unit_get_serial(z->pf_u) == z->pf_id && unit_get_owner(z->pf_u) == pl ? z->pf_u : NULL;
    if (pf && unit_get_finished(unit_get_physics(pf))) pf = NULL, z->pf_u = NULL;   // standing: am_bld counts it
    if (pf && now - z->pf_t > AAS_FOUND_MS) pf = NULL, z->pf_u = NULL, z->gave_up++;
    if (pf && (!z->pf_b_t || now - z->pf_b_t >= 15000)) { z->pf_b_t = now ? now : 1; bk_builders(pl, pf, AAS_BUILDERS + 1); }
    if (!pf) z->pf_u = NULL;
    if (bm && prod && !prod_have && !pf && a->have_u < want_u && prio_buildable(pl, prod) && (!z->prod_t || now - z->prod_t >= AAS_EVERY)) {
        z->prod_t = now ? now : 1;
        const int *stock = player_stock_at(pl);
        int pay = 1;
        for (int r = 2; r < 7; r++) pay &= unit_cost(pl, prod, r) <= stock[r];
        ivec2 spot[1];
        if (pay && !prio_refuses(pl, prod) && aa_block_at(pl, prod, base, 1, spot, AAS_R_MAKER)) {
            ee_unit *f = place_foundation(pl, prod, spot[0].x, spot[0].y);
            if (f) {
                z->pf_u = f; z->pf_id = unit_get_serial(f); z->pf_t = now ? now : 1; z->pf_b_t = z->pf_t;
                bk_builders(pl, f, AAS_BUILDERS + 1);
                z->prods++;
                if (g_log) fprintf(g_log, "# anti-air: P%u at %.1f min strong: a type %d laid out at %d,%d to make its AA units (type %d)\n", idx, now / 60000.0, prod, spot[0].x, spot[0].y, ut);
            }
        }
    }
    if (um && ut && a->have_u < want_u && (!z->train_t || now - z->train_t >= 10000)) {
        z->train_t = now ? now : 1;
        int n = want_u - a->have_u > AAS_TRAIN ? AAS_TRAIN : want_u - a->have_u;
        special_want(idx, SP_AA, ut, want_u - a->have_u);   // econ.c SPECIAL ORDERS: saved up for
        int req[4] = { ut, n, 0, 0 };
        unsigned got = 0;
        g_priority_mil = 1;
        hook_train_units(um, req, (uintptr_t)&got);
        g_priority_mil = 0;
        z->trained += got;
    }
    // a block at the first place not covered
    int bt = aa_type_of(pl, KIND_AA_BUILDING, 0), e = current_epoch(pl);
    if (bm && bt && want_site >= 0 && e >= 0 && e < 14 && (!z->block_t || now - z->block_t >= AAS_EVERY)) {
        z->block_t = now ? now : 1;
        ivec2 p = g_drops[pi].d[want_site].p, spot[4];
        int n = aa_block_at(pl, bt, p, block, spot, AAS_R), laid = 0;
        if (!n && block == 4) n = aa_block_at(pl, bt, p, block = 2, spot, AAS_R);
        const int *stock = player_stock_at(pl);
        for (int k = 0; k < n && z->nf < AAS_FOUNDS; k++) {
            int pay = 1;
            for (int r = 2; r < 7; r++) pay &= unit_cost(pl, bt, r) <= stock[r];
            if (!pay || prio_refuses(pl, bt)) break;   // the row "air" lets it through under a real threat
            ee_unit *f = place_foundation(pl, bt, spot[k].x, spot[k].y);
            if (!f) continue;
            z->fnd[z->nf].u = f; z->fnd[z->nf].id = unit_get_serial(f); z->fnd[z->nf].t = now ? now : 1; z->fnd[z->nf].b_t = now ? now : 1;
            z->nf++; laid++;
            bk_builders(pl, f, AAS_BUILDERS);
        }
        n = laid;
        if (n) {
            z->blocks++; z->queued += (unsigned)n;
            if (g_log) fprintf(g_log, "# anti-air: P%u at %.1f min strong: a block of %d type %d laid out at %d,%d for the place at %d,%d (threat %.1f)\n",
                               idx, now / 60000.0, n, bt, spot[0].x, spot[0].y, p.x, p.y, threat);
        }
    }
    if (g_log && g_autotest && (!z->log_t || now - z->log_t >= 60000)) {
        z->log_t = now ? now : 1;
        fprintf(g_log, "# anti-air: P%u at %.1f min strong: threat %.1f (seen near its towns %.1f, anywhere %.1f, expected %.1f, floor %.0f%s), places covered %d of %d (blocks of %d), AA buildings %d, AA units %d of %d (type %d, made in %d: %d of them); blocks %u (%u foundations: done %u, going up %d, given up %u), units trained %u, makers asked for %u\n",
                idx, now / 60000.0, threat, z->w_near, z->w_all, ex, least, port ? ", an enemy airport known" : "", covered, sites, block, a->have_b, a->have_u, want_u, ut, prod, prod_have,
                z->blocks, z->queued, z->done, going, z->gave_up, z->trained, z->prods);
        // where the game's own build lists hold that building (one list per epoch; queued() looks through all 14)
        for (int e2 = 0; bm && prod && !prod_have && e2 < 14; e2++)
            for (unsigned i = 0, n2 = 0; ; i++) {
                int *ent = bq_entry(bm, e2, i);
                if (!ent) { if (n2) fprintf(g_log, "# anti-air: P%u at %.1f min strong: the game's list of epoch %d (its epoch now %d) holds the type %d, %u of %u entries\n", idx, now / 60000.0, e2 + 1, current_epoch(pl) + 1, prod, n2, i); break; }
                n2 += ent[0] == prod;
            }
    }
    (void)epoch;
}
internal struct { uint32_t last, seen_t, train_t, build_t, all_t, exp_log_t; int air, ax, ay, all; unsigned trained, queued; } g_aa_st[16];
// BY THE AIR FORCE IT EXPECTS ([ai_base] aa_expected, off until measured; [autotest] v_aa_expected; army view step 7):
// half the aircraft it expects of its enemies (army_view.c avx_air_expected: what each fields, and of its counters to what
// it saw of the AI's) count as aircraft seen - as aa_ahead's aircraft seen anywhere, half of them. Measured (Continental
// from WWI, 75 min, 10 games each) without a result: the AIs flew next to nothing (expected 0.1-1.3 aircraft at the end,
// the AA orders as without it); h2h B on - A razed +2.5 (se 9.3), against the game AI off / on razed 33.5 / 23.8 (7.2,
// 7.8) - noise. Off; to be measured where aircraft fly (tests/scenarios/bombing_late.eeai).
#define AA_EXP(idx) PV(idx, g_v_aa_exp, g_aa_exp)
internal float avx_air_expected(ee_player *pl, unsigned idx, uint32_t now);   // army_view.c
internal void anti_air(ee_player *pl, unsigned idx, uint32_t now) {
    if (!g_aa || !pl) return;
    if (now < g_aa_st[idx].last) memset(&g_aa_st[idx], 0, sizeof g_aa_st[idx]);   // a new game
    if (g_aa_st[idx].last && now - g_aa_st[idx].last < 5000) return;
    g_aa_st[idx].last = now ? now : 1;
    int epoch = current_epoch(pl) + 1;   // tech ids count from 1
    if (epoch < 10) return;
    struct aa_scan a = { 0 };   // each unit once (grid_each): counted by hand per grid cell, an AA building in 2-4 cells
    a.pl = pl;                   // counted 2-4 times - fewer AA units trained, the AA buildings stopped early
    grid_each(aa_scan_unit, &a);
    if (AA_STRONG(idx)) { aa_strong(pl, idx, now, &a, epoch); return; }
    int nair = a.nair, have_u = a.have_u, have_b = a.have_b, nall = a.nall, nbx = a.nbx;
    int (*bxy)[2] = a.bxy;
    float axs = a.axs, ays = a.ays;
    if (nair) {
        if (nair >= g_aa_st[idx].air || now - g_aa_st[idx].seen_t > 300000) g_aa_st[idx].air = nair;
        g_aa_st[idx].seen_t = now ? now : 1;
        g_aa_st[idx].ax = (int)(axs / (float)nair); g_aa_st[idx].ay = (int)(ays / (float)nair);
    } else if (g_aa_st[idx].seen_t && now - g_aa_st[idx].seen_t > (g_aa_ahead ? 600000u : 300000u)) g_aa_st[idx].air = 0;
    int ahead = 0, air = g_aa_st[idx].air;
    if (AA_EXP(idx) || (g_log && g_autotest && (!g_aa_st[idx].exp_log_t || now - g_aa_st[idx].exp_log_t >= 60000))) {
        float ex = avx_air_expected(pl, idx, now);
        if (g_log && g_autotest && (!g_aa_st[idx].exp_log_t || now - g_aa_st[idx].exp_log_t >= 60000)) {
            g_aa_st[idx].exp_log_t = now ? now : 1;
            fprintf(g_log, "# anti-air: P%u at %.1f min: enemy aircraft near its towns %d, anywhere %d, expected %.1f%s\n", idx, now / 60000.0, air, g_aa_st[idx].all, ex,
                    AA_EXP(idx) ? " - counted" : "");
        }
        if (AA_EXP(idx) && (int)(ex / 2.0f + 0.5f) > air) air = (int)(ex / 2.0f + 0.5f);
    }
    if (g_aa_ahead) {
        if (nall >= g_aa_st[idx].all || now - g_aa_st[idx].all_t > 600000) { g_aa_st[idx].all = nall; g_aa_st[idx].all_t = now ? now : 1; }
        if (g_aa_st[idx].all / 2 > air) air = g_aa_st[idx].all / 2;
        for (int q = 1; q < 16 && !ahead; q++) {
            void *o = st_player(q);   // every player (g_players: the AIs only)
            ahead = o && o != pl && is_enemy(pl, o) && current_epoch(o) + 1 >= 10;
        }
        if (!ahead && !air) return;
    } else if (!air) return;
    int want = 2 + (air + 1) / 2;
    if (want > 16) want = 16;
    int want_b = air ? 1 + want / 4 : 0;
    if (want_b > 4) want_b = 4;
    // ahead: the drop sites nearest the base point without an AA building within 7 tiles - the first of them gets the
    // next one, as many as aa_ahead_buildings
    ivec2 cover = iv2(0, 0);
    int uncovered = 0;
    if (ahead) {
        unsigned pi = (unsigned)player_get_index(pl) & 15;
        void *cm0 = player_get_civ_mgr(pl);
        ee_build_mgr *bm0 = cm0 ? civ_mgr_get_build_mgr(cm0) : NULL;
        ivec2 base = bm0 ? build_mgr_get_base(bm0) : iv2(0, 0);
        int used[64] = { 0 };
        drops_cached(pl);
        for (int pick = 0; pick < g_aa_ahead_b && pick < g_drops[pi].n; pick++) {
            int bi = -1, bd = 0x7FFFFFFF;
            for (int i = 0; i < g_drops[pi].n && i < 64; i++) {
                const struct drop *d = &g_drops[pi].d[i];
                int dd = ivec2_dist2(d->p, base);
                if (!used[i] && !d->food && dd < bd) { bd = dd; bi = i; }
            }
            if (bi < 0) break;
            used[bi] = 1;
            ivec2 d = g_drops[pi].d[bi].p;
            int covered = 0;
            for (int k = 0; k < nbx && !covered; k++) covered = abs(bxy[k][0] - d.x) <= 7 && abs(bxy[k][1] - d.y) <= 7;
            if (!covered && !uncovered++) cover = d;
        }
        if (have_b + uncovered > want_b) want_b = have_b + uncovered;
        if (want_b > 4 + g_aa_ahead_b) want_b = 4 + g_aa_ahead_b;
        if (want < 2 + want_b) want = 2 + want_b;
    }
    int want_u = want - (have_b < want_b ? have_b : want_b);
    void *cm = player_get_civ_mgr(pl), *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    // AA units
    if (um && have_u < want_u && (!g_aa_st[idx].train_t || now - g_aa_st[idx].train_t >= 15000)) {
        g_aa_st[idx].train_t = now ? now : 1;
        for (size_t k = 0; k < sizeof k_aa_units / sizeof *k_aa_units; k++) {
            if (k_aa_units[k].epoch > epoch || !ut_has(g_ut[k_aa_units[k].type])) continue;
            int n = want_u - have_u > 3 ? 3 : want_u - have_u;
            special_want(idx, SP_AA, g_ut[k_aa_units[k].type], want_u - have_u);   // econ.c SPECIAL ORDERS: saved up for
            int req[4] = { g_ut[k_aa_units[k].type], n, 0, 0 };
            unsigned got = 0;
            g_priority_mil = 1;
            hook_train_units(um, req, (uintptr_t)&got);
            g_priority_mil = 0;
            if (!got) continue;
            g_aa_st[idx].trained += got;
            if (g_log) fprintf(g_log, "# anti-air: P%u at %.1f min trains %u x type %d (enemy aircraft %d, AA units %d of %d, AA buildings %d of %d)\n",
                               idx, now / 60000.0, got, req[0], g_aa_st[idx].air, have_u, want_u, have_b, want_b);
            break;
        }
    }
    // AA buildings: next to the drop site nearest to where the aircraft were
    int queued = 0;
    for (size_t k = 0; bm && k < sizeof k_aa_bldgs / sizeof *k_aa_bldgs; k++) queued |= bq_has(bm, g_ut[k_aa_bldgs[k].type]);
    int e = current_epoch(pl);
    ivec2 d;
    if (uncovered && !g_aa_st[idx].air) d = cover;   // ahead: no aircraft over a drop site yet
    if (bm && !queued && have_b < want_b && e >= 0 && e < 14 && (!g_aa_st[idx].build_t || now - g_aa_st[idx].build_t >= 60000) &&
        ((uncovered && !g_aa_st[idx].air) || nearest_drop(pl, iv2(g_aa_st[idx].ax, g_aa_st[idx].ay), 0, &d))) {
        int bt = 0;
        for (size_t k = 0; k < sizeof k_aa_bldgs / sizeof *k_aa_bldgs && !bt; k++)
            if (k_aa_bldgs[k].epoch <= epoch && ut_has(g_ut[k_aa_bldgs[k].type]) && can_build(pl, g_ut[k_aa_bldgs[k].type]))
                bt = g_ut[k_aa_bldgs[k].type];
        for (int r = 3; bt && r <= 7; r++)
            for (int ax = -r; ax <= r; ax++)
                for (int ay = -r; ay <= r; ay++) {
                    if ((abs(ax) != r && abs(ay) != r) || !can_place(pl, bt, iv2(d.x + ax, d.y + ay)) ||
                        raiders_near(pl, iv2(d.x + ax, d.y + ay), 8)) continue;
                    int entry[3] = { bt, d.x + ax, d.y + ay };
                    build_mgr_queue_add(bm, e, entry);
                    g_aa_st[idx].build_t = now ? now : 1;
                    g_aa_st[idx].queued++;
                    if (g_log) fprintf(g_log, "# anti-air: P%u at %.1f min queues type %d at %d,%d (enemy aircraft %d near %d,%d, %d seen anywhere; AA buildings %d of %d%s)\n",
                                       idx, now / 60000.0, bt, d.x + ax, d.y + ay, g_aa_st[idx].air, g_aa_st[idx].ax, g_aa_st[idx].ay, g_aa_st[idx].all,
                                       have_b, want_b, uncovered && !g_aa_st[idx].air ? ", ahead" : "");
                    goto placed;
                }
        g_aa_st[idx].build_t = now ? now : 1;   // no room: try again in a minute
    }
placed:;
}

// SHORE GUNS ([ai_base] shore_guns, guns per threatened dock, default 0 until measured; the player 2026-11-07:
// "defences also mean trebuchets or cannons near the docks, they reach further than the towers and have a bonus against
// ships"). The fishing boats die at the docks (# boat death, 12 Small Islands games against the cheating game AI: 166
// of 249 within 3 tiles of their dock, sunk by frigates and galleys of range 7 - a tower reaches 5). Every SG_EVERY: a
// dock is threatened while an enemy warship it sees was within SG_NEAR of it in the last SG_KEEP; each such dock wants
// shore_guns siege weapons or artillery, the longest reach it can train (catapult 7, trebuchet 8, the cannons 8-10,
// artillery 12) - ordered as the siege's are (attack.c siege_train: a Siege Factory queued when there is none), held
// from the game's commander (player hold_unit) and posted on the shore by the dock, on the side the ships came from,
// where they shoot what comes in range. A gun stays at its post (the ships come back); one away from it by more than
// SG_STRAY (chasing) walks back; one whose dock is gone gets the next. "# shore guns:" at every order and once a game
// minute (autotest).
// THE BATTERY (the player: "i usually place a hospital by the docks and place the shore guns in its influence, i can
// forget about it and the ai never manages to destroy them"): docks within SG_GROUP tiles of each other are one
// battery (the first measurement wanted 2 guns at each of 4-5 docks and held 1.4); each battery gets a Hospital
// 3-6 tiles from its dock with the sea within 4 (it heals every own land unit within ~4 tiles of its centre, 3 hit
// points a second - docs/economy.md), queued one at a time; once it stands the guns' posts are within SG_HEAL of its
// centre with the sea within 3.
// MASSED (the player: "do you mass them? i put them on shore in groups of 5"): a battery wants all of shore_guns and
// the batteries fill one at a time - the next only once the one before has its guns (2 a dock at every dock held 1.1).
// THE WALL ([ai_base] shore_walls; the player: "maybe catapults and trebuchets work better behind a wall"): once
// the Hospital stands, wall pieces on the coast's tiles within SG_WALL of its centre, one a tick, the coast within 3 of
// a dock left open for its boats - a wall stops the ships' flat shots (frigates, galleys: line_of_fire_at_game) and a
// landing; the guns fire over it (catapult, trebuchet: high arc). The posts may stand beside it.
// AN OUTPOST'S (expand.c FORT'S GUNS): its landing shore counts as a dock; the guns of its battery wait for its boat
// (sg_ferry_guns, no post yet) and are posted once ashore.
#define SG_EVERY 5000
#define SG_NEAR 12
#define SG_KEEP 600000
#define SG_STRAY 4
#define SG_MAX 12
#define SG_DOCKS 8
#define SG_GROUP 10
#define SG_HEAL 3
#define SG_WALL 5
internal const enum ut_name k_sg_types[] = {
    UT_GUN_ARTILLERY13_COLOSSUS, UT_GUN_SIEGE13_PALADIN_CANNON, UT_GUN_ARTILLERY10_ARTILLERY, UT_GUN_SIEGE10_HOWITZER_CANNON,
    UT_GUN_SIEGE09_SERPENTINE_CANNON, UT_GUN_ARTILLERY08_BOMBARD, UT_GUN_SIEGE07_BASILISK_CANNON, UT_GUN_SIEGE06_TREBUCHET,
    UT_GUN_SIEGE04_CATAPULT };
internal int sg_gun_type(int t) {
    for (size_t k = 0; k < sizeof k_sg_types / sizeof *k_sg_types; k++) if (t == g_ut[k_sg_types[k]]) return 1;
    return 0;
}
struct sg_scan { ee_player *pl; int nd, ng, ns, nh, factory; ivec2 d[16]; ee_unit *g[48]; ivec2 s[64], h[16]; };
internal void sg_scan_unit(ee_unit *u, void *ctx) {   // own docks and guns, the enemy's warships it sees
    struct sg_scan *a = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) return;
    int t = unit_type_get_id(td);
    ee_player *o = unit_get_owner(u);
    if (o == a->pl) {
        if (t == UT(B_DOCK) && a->nd < 16) a->d[a->nd++] = unit_get_tile(u);
        else if (t == UT(B_SIEGE_FACTORY)) a->factory = 1;
        else if (t == UT(B_HOSPITAL) && a->nh < 16) a->h[a->nh++] = unit_get_tile(u);
        else if (sg_gun_type(t) && a->ng < 48) a->g[a->ng++] = u;
        return;
    }
    if (unit_type_get_class_id(td) != CLS_NAVAL || !o || player_get_index(o) <= 0 || !is_enemy(a->pl, o) || !seen_by(u, a->pl)) return;
    if (a->ns < 64) a->s[a->ns++] = unit_get_tile(u);
}
internal struct {
    uint32_t last, train_t, fac_t, log_t;
    int nd; struct { ivec2 p, from, hosp; uint32_t t; int has_h; unsigned isle; } d[SG_DOCKS];   // the batteries: their dock, the ships' side, when last, its Hospital, an outpost's island
    int ng; struct { int id; ivec2 post, bad; int dock, tries, healed; } g[SG_MAX];   // the guns held: id, post, its battery (-1 none yet), a post it didn't reach, posted by a Hospital
    uint32_t hq_t;   // a Hospital queued last
    unsigned trained, orders, adopted, failed, stuck, hosp_queued, walls, stances, no_post;
} g_sg[16];
internal int shore_gun(unsigned idx, ee_unit *u) {
    if (!g_shore_guns || !u) return 0;
    int id = unit_get_serial(u);
    for (int k = 0; k < g_sg[idx & 15].ng; k++) if (g_sg[idx & 15].g[k].id == id) return 1;
    return 0;
}
internal int goal_near(ee_unit *u, float x, float y, float r);   // forces.c
internal int ld_tile_free(ivec2 p);                             // landing.c
internal int ld_type_avail(ee_player *pl, int t);               // landing.c
internal int ld_tile_free_by_walls(ivec2 p);                    // landing.c
internal unsigned ocean_near(ivec2 p, int r, ivec2 *w);         // landing.c
// a post by the dock: open land 1-4 tiles from it with the sea within 2 - by its Hospital once it stands: within
// SG_HEAL of its centre, the sea within 3 - on the gun's land and walk (the reach map: a tile by a dock is often a spit
// of its own - in the first pilot 1000 orders and no gun at its post), nearest the ships' side, 2 tiles from the others
// a post's tile: nothing on it but own soldiers, no building (a wall piece aside) or resource beside it - the 3x3 clear
// of everything found no post for most guns on a crowded shore ("5 of 5, at their posts 0")
internal int sg_tile_ok(ee_player *pl, ivec2 p) {
    for (struct grid_it it = grid_it_box(p.x, p.y, 3, 1); grid_it_next(&it); ) {
        ee_unit_type *t = it.td;
        int dx = abs(unit_get_x(it.u) - p.x), dy = abs(unit_get_y(it.u) - p.y);
        if (!t || dx > 1 || dy > 1 || unit_get_hp(it.u) <= 0) continue;
        int fam = unit_type_get_class_id(t), on = !dx && !dy;
        if (fam == CLS_AMBIENT) continue;
        if (on && !(unit_get_owner(it.u) == pl && is_army_family(fam))) return 0;
        if (!on && (fam == CLS_RESOURCE || ((fam == CLS_BUILDING || fam == CLS_FORTRESS) && !unit_type_get_is_wall(t)))) return 0;
    }
    return 1;
}
internal int sg_post(ee_player *pl, unsigned idx, int dock, ee_unit *u, ivec2 bad, ivec2 *out) {
    // by the Hospital within its reach, else (no room there) by the dock as before its Hospital stood
    for (int pass = g_sg[idx].d[dock].has_h ? 0 : 1; pass < 2; pass++) {
    int heal = pass == 0, rr = heal ? SG_HEAL + 1 : 4;
    ivec2 d = heal ? g_sg[idx].d[dock].hosp : g_sg[idx].d[dock].p, f = g_sg[idx].d[dock].from, at = unit_get_tile(u);
    int owner = player_get_index(pl), cont = (int)area_id(at, 2), reg = rm_region_of(owner, at.x, at.y);
    int best = 0x7FFFFFFF;
    for (int dx = -rr; dx <= rr; dx++)
        for (int dy = -rr; dy <= rr; dy++) {
            ivec2 q = iv2(d.x + dx, d.y + dy);
            if ((abs(dx) < 2 && abs(dy) < 2) || (heal && dx * dx + dy * dy > (SG_HEAL + 1) * (SG_HEAL + 1)) || (int)area_id(q, 2) != cont ||
                area_id(q, 1) || (q.x == bad.x && q.y == bad.y) || !ocean_near(q, heal ? 4 : 2, NULL) || !sg_tile_ok(pl, q) || encl_walled(q.x, q.y)) continue;
            int r = reg >= 0 ? rm_region_of(owner, q.x, q.y) : -1;
            if (r >= 0 && r != reg) continue;
            int taken = 0;
            for (int k = 0; k < g_sg[idx].ng && !taken; k++) taken = g_sg[idx].g[k].dock >= 0 && ivec2_dist2(g_sg[idx].g[k].post, q) < 4;
            int sc = ivec2_dist2(q, f);
            if (!taken && sc < best) { best = sc; *out = q; }
        }
    if (best < 0x7FFFFFFF) return 1;
    }
    g_sg[idx].no_post++;
    return 0;
}
internal int ex_battery(unsigned idx, ivec2 *p, ivec2 *sea, unsigned *cont, int *ready);   // expand.c
// an outpost's guns still on the land the boat leaves from (from), up to max (into out, when given): their number
internal int sg_ferry_guns(unsigned idx, unsigned cont, unsigned from, ee_unit **out, int max) {
    int n = 0;
    for (int k = 0; k < g_sg[idx].ng && n < max; k++) {
        int dk = g_sg[idx].g[k].dock;
        ee_unit *u = dk >= 0 && dk < g_sg[idx].nd && g_sg[idx].d[dk].isle == cont ? unit_by_id((unsigned)g_sg[idx].g[k].id) : NULL;
        if (!u || unit_get_x(u) < 0 || (unsigned)area_id(unit_get_tile(u), 2) != from || g_sg[idx].g[k].tries >= 2) continue;
        if (out) out[n] = u;
        n++;
    }
    return n;
}
// a gun that didn't get aboard: after two boats it stays (one gun at a spot no boat reached was fetched 49 times in a game)
internal void sg_ferry_failed(unsigned idx, ee_unit *u) {
    for (int k = 0; k < g_sg[idx].ng; k++) if (g_sg[idx].g[k].id == unit_get_serial(u)) g_sg[idx].g[k].tries++;
}
internal void shore_guns(ee_player *pl, unsigned idx, uint32_t now) {
    if (!g_shore_guns || !pl) return;
    if (now < g_sg[idx].last) memset(&g_sg[idx], 0, sizeof g_sg[idx]);   // a new game
    if (g_sg[idx].last && now - g_sg[idx].last < SG_EVERY) return;
    g_sg[idx].last = now ? now : 1;
    struct sg_scan a = { 0 };
    a.pl = pl;
    grid_each(sg_scan_unit, &a);
    ivec2 isle_p = iv2(0, 0), isle_w = iv2(0, 0);
    unsigned isle = 0;
    int isle_ready = 0;
    if (ex_battery(idx, &isle_p, &isle_w, &isle, &isle_ready) && a.nd < 16) a.d[a.nd++] = isle_p; else isle = isle_ready = 0;   // an outpost's shore, one more dock
    // the threatened docks: kept by place while the dock stands, refreshed by the ships near it
    int m = 0;
    for (int k = 0; k < g_sg[idx].nd; k++) {
        int stands = 0;
        for (int j = 0; j < a.nd && !stands; j++) stands = ivec2_dist2(a.d[j], g_sg[idx].d[k].p) <= SG_GROUP * SG_GROUP;
        if (stands && now - g_sg[idx].d[k].t < SG_KEEP) g_sg[idx].d[m++] = g_sg[idx].d[k];
    }
    // one gone or quiet: the guns are posted anew (those without a threatened dock stay where they stand)
    if (m != g_sg[idx].nd) for (int q = 0; q < g_sg[idx].ng; q++) g_sg[idx].g[q].dock = -1;
    g_sg[idx].nd = m;
    for (int j = 0; j < a.nd; j++) {
        int close = -1, nd2 = SG_NEAR * SG_NEAR + 1;
        for (int s = 0; s < a.ns; s++) { int d2 = ivec2_dist2(a.s[s], a.d[j]); if (d2 < nd2) { nd2 = d2; close = s; } }
        int ahead = isle_ready && j == a.nd - 1;   // an outpost's, ready: without waiting for the ships
        if (close < 0 && !ahead) continue;
        int k;
        for (k = 0; k < g_sg[idx].nd && ivec2_dist2(g_sg[idx].d[k].p, a.d[j]) > SG_GROUP * SG_GROUP; k++) ;   // a battery near
        if (k == g_sg[idx].nd) {
            if (k >= SG_DOCKS) continue;
            g_sg[idx].nd++;
            g_sg[idx].d[k].p = a.d[j]; g_sg[idx].d[k].from = close >= 0 ? a.s[close] : isle_w; g_sg[idx].d[k].has_h = 0;
        }
        g_sg[idx].d[k].t = now ? now : 1;
    }
    for (int k = 0; k < g_sg[idx].nd; k++) g_sg[idx].d[k].isle = isle && ivec2_dist2(g_sg[idx].d[k].p, isle_p) <= SG_GROUP * SG_GROUP ? isle : 0;
    int full = 0;   // the batteries with all their guns: the next one's are wanted only then
    for (int d = 0; d < g_sg[idx].nd; d++) {
        int n = 0;
        for (int q = 0; q < g_sg[idx].ng; q++) n += g_sg[idx].g[q].dock == d;
        full += n >= g_shore_guns;
    }
    int want = g_shore_guns * (full + 1 < g_sg[idx].nd ? full + 1 : g_sg[idx].nd);
    if (want > SG_MAX) want = SG_MAX;
    void *cm = player_get_civ_mgr(pl), *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    int e = current_epoch(pl), nhosp = 0;
    // the batteries' Hospitals: an own one within 6 tiles of the dock, else one queued (one at a time, a minute apart)
    for (int k = 0; k < g_sg[idx].nd; k++) {
        int had = g_sg[idx].d[k].has_h, bh = -1, bd = 37;
        for (int h = 0; h < a.nh; h++) { int d2 = ivec2_dist2(a.h[h], g_sg[idx].d[k].p); if (d2 < bd) { bd = d2; bh = h; } }
        g_sg[idx].d[k].has_h = bh >= 0;
        if (bh >= 0) { g_sg[idx].d[k].hosp = a.h[bh]; nhosp++; }
        if (had && bh < 0) for (int q = 0; q < g_sg[idx].ng; q++) if (g_sg[idx].g[q].dock == k) g_sg[idx].g[q].dock = -1;   // gone: posted anew
        if (bh >= 0 || !bm || e < 0 || e >= 14 || !ut_has(UT(B_HOSPITAL)) || !can_build(pl, UT(B_HOSPITAL)) || bq_has(bm, UT(B_HOSPITAL)) ||
            (g_sg[idx].hq_t && now - g_sg[idx].hq_t < 60000)) continue;
        ivec2 d = g_sg[idx].d[k].p, best = iv2(-1, -1);
        int bs = 0x7FFFFFFF;
        for (int dx = -6; dx <= 6; dx++)
            for (int dy = -6; dy <= 6; dy++) {
                ivec2 q = iv2(d.x + dx, d.y + dy);
                int d2 = dx * dx + dy * dy;
                if (d2 < 9 || d2 > 36 || area_id(q, 1) || !ocean_near(q, 4, NULL) || (g_sg[idx].d[k].isle && (unsigned)area_id(q, 2) != g_sg[idx].d[k].isle) ||
                    !can_place(pl, UT(B_HOSPITAL), q) || raiders_near(pl, q, 8)) continue;
                if (d2 < bs) { bs = d2; best = q; }
            }
        g_sg[idx].hq_t = now ? now : 1;
        if (best.x < 0) continue;
        int entry[3] = { UT(B_HOSPITAL), best.x, best.y };
        build_mgr_queue_add(bm, e, entry);
        g_sg[idx].hosp_queued++;
        if (g_log) fprintf(g_log, "# shore guns: P%u at %.1f min queues a Hospital at %d,%d for the battery at the dock %d,%d\n", idx, now / 60000.0,
                           best.x, best.y, d.x, d.y);
    }
    // the wall in front of a battery with its Hospital: one piece a tick, the coast tile nearest the Hospital
    void *wm = cm ? civ_mgr_get_wall_mgr(cm) : NULL;
    int wtype = 0, *stock = player_stock_at(pl);
    static const enum ut_name wline[] = { UT_WALL_LINE };
    for (int i = 5; g_shore_walls && wm && i >= 0 && !wtype; i--) if (ut_has(g_ut[wline[i]]) && can_build(pl, g_ut[wline[i]])) wtype = g_ut[wline[i]];
    for (int k = 0; wtype && k < g_sg[idx].nd; k++) {
        if (!g_sg[idx].d[k].has_h) continue;
        int enough = 1;
        for (int r = 2; r < 7; r++) enough &= stock[r] >= unit_cost(pl, wtype, r) + (r == 4 ? 50 : 0);
        if (!enough) break;
        ivec2 h = g_sg[idx].d[k].hosp, best = iv2(-1, -1);
        int bs = 0x7FFFFFFF;
        for (int dx = -SG_WALL; dx <= SG_WALL; dx++)
            for (int dy = -SG_WALL; dy <= SG_WALL; dy++) {
                ivec2 q = iv2(h.x + dx, h.y + dy);
                int d2 = dx * dx + dy * dy, by_dock = 0;
                if (d2 > SG_WALL * SG_WALL || d2 >= bs || !area_id(q, 2) || area_id(q, 1) || !encl_coast(q) || encl_walled(q.x, q.y)) continue;
                for (int j = 0; j < a.nd && !by_dock; j++) by_dock = abs(a.d[j].x - q.x) <= 3 && abs(a.d[j].y - q.y) <= 3;
                if (by_dock || !can_place(pl, wtype, q)) continue;
                bs = d2; best = q;
            }
        if (best.x < 0) continue;
        int pa[2] = { best.x, best.y };
        build_wall(wm, pa, pa, wtype);
        g_sg[idx].walls++;
        if (g_log) fprintf(g_log, "# shore guns: P%u at %.1f min: wall %d at %d,%d in front of the battery at %d,%d\n", idx, now / 60000.0, wtype, best.x, best.y, h.x, h.y);
        break;
    }
    // the guns held: the dead out, then new ones (not held by another part, not in a landing) while short - after a
    // load the held guns by a dock again (the list is not saved)
    m = 0;
    for (int k = 0; k < g_sg[idx].ng; k++) {
        ee_unit *u = unit_by_id((unsigned)g_sg[idx].g[k].id);
        if (u && unit_get_serial(u) == g_sg[idx].g[k].id && unit_get_hp(u) > 0 && unit_get_owner(u) == pl) g_sg[idx].g[m++] = g_sg[idx].g[k];
    }
    g_sg[idx].ng = m;
    for (int i = 0; i < a.ng && g_sg[idx].ng < want; i++) {
        ee_unit *u = a.g[i];
        int id = unit_get_serial(u), have = 0;
        for (int k = 0; k < g_sg[idx].ng && !have; k++) have = g_sg[idx].g[k].id == id;
        if (have || landing_has_unit(pl, u)) continue;
        if (unit_get_ai_held(u)) {
            int by_dock = 0;
            for (int j = 0; j < a.nd && !by_dock; j++) by_dock = ivec2_dist2(unit_get_tile(u), a.d[j]) <= 36;
            if (!by_dock) continue;
        } else player_hold_unit(pl, u);
        g_sg[idx].g[g_sg[idx].ng].id = id; g_sg[idx].g[g_sg[idx].ng].dock = -1; g_sg[idx].g[g_sg[idx].ng].tries = 0;
        g_sg[idx].g[g_sg[idx].ng].bad = iv2(-1, -1); g_sg[idx].g[g_sg[idx].ng].healed = 0; g_sg[idx].g[g_sg[idx].ng++].post = iv2(-1, -1);
        g_sg[idx].adopted++;
    }
    // training: the longest reach it can, 2 a minute at most; no Siege Factory - one queued (5 minutes apart)
    if (g_sg[idx].ng < want && (!g_sg[idx].train_t || now - g_sg[idx].train_t >= 60000)) {
        g_sg[idx].train_t = now ? now : 1;
        if (!a.factory) {
            if (bm && e >= 0 && e < 14 && ut_has(UT(B_SIEGE_FACTORY)) && !bq_has(bm, UT(B_SIEGE_FACTORY)) &&
                (!g_sg[idx].fac_t || now - g_sg[idx].fac_t >= 300000)) {
                g_sg[idx].fac_t = now ? now : 1;
                int entry[3] = { UT(B_SIEGE_FACTORY), 0, 0 };
                build_mgr_queue_add(bm, e, entry);
                if (g_log) fprintf(g_log, "# shore guns: P%u at %.1f min queues a Siege Factory (guns %d of %d)\n", idx, now / 60000.0, g_sg[idx].ng, want);
            }
        } else if (um) {
            unsigned trained0 = g_sg[idx].trained;
            // by reach, the longest first - only what its tech record allows now (ld_type_avail): in a later epoch the game
            // took the obsolete Trebuchet's order and never built it (7 ordered, none came)
            int order[sizeof k_sg_types / sizeof *k_sg_types], no = 0;
            for (size_t k = 0; k < sizeof k_sg_types / sizeof *k_sg_types; k++) {
                int t = g_ut[k_sg_types[k]];
                ee_unit_type *td = ut_has(t) && ld_type_avail(pl, t) ? type_data(t) : NULL;
                if (!td) continue;
                int j = no++;
                for (; j > 0 && unit_type_get_range(type_data(order[j - 1])) < unit_type_get_range(td); j--) order[j] = order[j - 1];
                order[j] = t;
            }
            special_want(idx, SP_SHORE, no ? order[0] : 0, want - g_sg[idx].ng);   // econ.c SPECIAL ORDERS: saved up for
            for (int k = 0; k < no; k++) {
                int n = want - g_sg[idx].ng > 2 ? 2 : want - g_sg[idx].ng;
                int req[4] = { order[k], n, 0, 0 };
                unsigned got = 0;
                g_priority_mil = 1;
                hook_train_units(um, req, (uintptr_t)&got);
                g_priority_mil = 0;
                if (!got) continue;
                g_sg[idx].trained += got;
                if (g_log) fprintf(g_log, "# shore guns: P%u at %.1f min trains %u x type %d (range %.0f; guns %d of %d, docks threatened %d)\n",
                                   idx, now / 60000.0, got, order[k], unit_type_get_range(type_data(order[k])), g_sg[idx].ng, want, g_sg[idx].nd);
                break;
            }
            if (g_sg[idx].trained == trained0) {   // the factory busy, the stock short, no type open
                g_sg[idx].failed++;
                if (g_log && g_sg[idx].failed % 10 == 1) {   // DIAG: what it could pick
                    int *st = player_stock_at(pl);
                    fprintf(g_log, "# shore guns: P%u at %.1f min trains nothing (stock W%d G%d I%d) - open:", idx, now / 60000.0, st[3], st[5], st[6]);
                    for (size_t k = 0; k < sizeof k_sg_types / sizeof *k_sg_types; k++) {
                        int t = g_ut[k_sg_types[k]];
                        fprintf(g_log, " %d%s", t, !ut_has(t) ? "(none)" : ld_type_avail(pl, t) ? "(open)" : "(closed)");
                    }
                    fprintf(g_log, "\n");
                }
            }
        }
    }
    // posts: each gun without a dock to the threatened dock with the fewest; back to its post when away
    int posted = 0, close_by = 0;
    for (int k = 0; k < g_sg[idx].ng; k++) {
        ee_unit *u = unit_by_id((unsigned)g_sg[idx].g[k].id);
        if (!u || unit_get_x(u) < 0) continue;   // (aboard an outpost's boat)
        unsigned gc = (unsigned)area_id(unit_get_tile(u), 2);
        if (g_sg[idx].g[k].dock >= 0 && g_sg[idx].g[k].post.x < 0 && g_sg[idx].d[g_sg[idx].g[k].dock].isle == gc) g_sg[idx].g[k].dock = -1;   // ashore: posted now
        if (g_sg[idx].g[k].dock >= 0 && g_sg[idx].d[g_sg[idx].g[k].dock].has_h && !g_sg[idx].g[k].healed && g_sg[idx].g[k].post.x >= 0) g_sg[idx].g[k].dock = -1;
        if (g_sg[idx].g[k].dock < 0 && g_sg[idx].nd) {
            int bd = -1;   // the first battery short of its guns (massed: one filled before the next)
            for (int d = 0; d < g_sg[idx].nd && bd < 0; d++) {
                int n = 0;
                for (int q = 0; q < g_sg[idx].ng; q++) n += g_sg[idx].g[q].dock == d;
                if (n < g_shore_guns) bd = d;
            }
            ivec2 p;
            if (bd >= 0 && g_sg[idx].d[bd].isle && g_sg[idx].d[bd].isle != gc) { g_sg[idx].g[k].dock = bd; g_sg[idx].g[k].post = iv2(-1, -1); continue; }   // waits for the boat
            if (bd >= 0 && sg_post(pl, idx, bd, u, g_sg[idx].g[k].bad, &p)) {
                g_sg[idx].g[k].dock = bd; g_sg[idx].g[k].post = p; g_sg[idx].g[k].tries = 0; g_sg[idx].g[k].healed = g_sg[idx].d[bd].has_h;
            }
        }
        if (g_sg[idx].g[k].dock < 0 || g_sg[idx].g[k].post.x < 0) continue;
        ivec2 p = g_sg[idx].g[k].post;
        // HOLD POSITION (the player: "in a line beside a hospital in hold position mode, they should not try to chase or get
        // attack commands"): the game's stand-ground stance - it shoots what comes in range and doesn't move for it; no
        // other part's order reaches it (attack.c unit_goal, landing.c hook_asg_give_goal)
        g_sg[idx].stances += (unsigned)unit_set_stance(u, 1);
        if (ivec2_dist2(unit_get_tile(u), p) <= 4) { posted++; g_sg[idx].g[k].tries = 0; continue; }
        if (ivec2_dist2(unit_get_tile(u), p) <= SG_STRAY * SG_STRAY && unit_get_goal(u)) { close_by++; continue; }   // shooting near its post
        if (goal_near(u, (float)p.x + 0.5f, (float)p.y + 0.5f, 1.5f)) continue;
        if (++g_sg[idx].g[k].tries > 6) {   // ordered 6 times and not there: another post
            if (g_log) fprintf(g_log, "# shore guns: P%u at %.1f min: gun at %d,%d doesn't reach its post %d,%d - another\n", idx, now / 60000.0,
                               unit_get_x(u), unit_get_y(u), p.x, p.y);
            g_sg[idx].stuck++; g_sg[idx].g[k].bad = p; g_sg[idx].g[k].dock = -1; continue;
        }
        g_sg_order = 1;
        g_sg[idx].orders += (unsigned)unit_goal(u, goal_move((float)p.x + 0.5f, (float)p.y + 0.5f), 0, 0x1F40);
        g_sg_order = 0;
    }
    if (g_autotest && g_log && (!g_sg[idx].log_t || now - g_sg[idx].log_t >= 60000)) {
        g_sg[idx].log_t = now ? now : 1;
        ee_unit *g0 = g_sg[idx].ng ? unit_by_id((unsigned)g_sg[idx].g[0].id) : NULL;   // DIAG: a gun's stance
        for (int k = 0; k < g_sg[idx].ng; k++) {   // DIAG: where the guns away from their posts are
            ee_unit *u = unit_by_id((unsigned)g_sg[idx].g[k].id);
            if (!u || g_sg[idx].g[k].dock < 0 || ivec2_dist2(unit_get_tile(u), g_sg[idx].g[k].post) <= 4) continue;
            ee_goal *gg = unit_get_goal(u);
            fprintf(g_log, "# shore guns: P%u gun %d at %d,%d, its post %d,%d (%.0f tiles), goal %s, tries %d\n", idx, k, unit_get_x(u), unit_get_y(u),
                    g_sg[idx].g[k].post.x, g_sg[idx].g[k].post.y, sqrtf((float)ivec2_dist2(unit_get_tile(u), g_sg[idx].g[k].post)),
                    !gg ? "none" : goal_is_location(gg) ? "move" : goal_is_attack(gg) ? "attack" : "other", g_sg[idx].g[k].tries);
        }
        fprintf(g_log, "# shore guns: P%u at %.1f min: docks %d, batteries %d (a Hospital %d, %u queued), enemy warships seen %d | guns %d of %d, at their posts %d, near them %d | trained %u, adopted %u, orders %u, stuck %u, nothing trainable %u, no post %u, wall pieces %u, stances set %u (a gun's stance %d, its type takes %d)%s\n",
                idx, now / 60000.0, a.nd, g_sg[idx].nd, nhosp, g_sg[idx].hosp_queued, a.ns, g_sg[idx].ng, want, posted, close_by, g_sg[idx].trained, g_sg[idx].adopted,
                g_sg[idx].orders, g_sg[idx].stuck, g_sg[idx].failed, g_sg[idx].no_post, g_sg[idx].walls, g_sg[idx].stances, g0 ? unit_stance(g0) : -2, g0 && unit_get_type_data(g0) ? (int)unit_type_get_behaviours(unit_get_type_data(g0)) : -2, a.factory ? "" : ", no Siege Factory");
    }
}

// UPGRADE STRAGGLERS: a researched line upgrade (effect kind 0x13, 0x5BA59D: e.g. 3051 guard tower 47 -> 461)
// converts the player's finished objects of the old type on the spot (0x62F192 cdecl(unit, player index, type data
// 0x9418A0[new type]) - hit points in proportion), and from then on the old type can't be built. Nothing converts what
// is finished later: a tower still a foundation or in the AI's build queue at that moment (the queue names the type:
// 47) was finished as the old type and stayed it (the Khan: 4 Paleo towers beside 4 Copper ones, 3051 researched).
// Every 30 s of game time, at the safe point between simulation steps: per AI and line (guard towers, walls, gates)
// the newest type it can build (0x555710), and every finished object of an older type of that line that it can no
// longer build is converted (still buildable = its upgrade not researched: the Copper tower comes with its own tech,
// the conversion of the old ones is the paid upgrade - paid_upgrades).
internal const enum ut_name k_upg_names[3][7] = {
    { UT_TOWER_LINE },        // guard towers Paleo..Digital
    { UT_WALL_LINE, UT_N },   // walls Copper..Digital
    { UT_GATE_LINE, UT_N },   // gates Copper..Digital
};
#define UPG(l, i) (k_upg_names[l][i] == UT_N ? -1 : g_ut[k_upg_names[l][i]])   // the type, -1 past the line's end
internal struct { ee_player *pl; int best[3]; uint8_t avail[3][7]; ee_unit *u[64]; int to[64], n; } g_upg;
internal void upg_scan(ee_unit *u, void *ctx) {
    (void)ctx;
    if (g_upg.n >= 64 || unit_get_owner(u) != g_upg.pl || unit_get_hp(u) <= 0) return;
    ee_unit_type *td = unit_get_type_data(u); ee_unit *st = unit_get_physics(u);
    if (!td || !st || !unit_get_finished(st)) return;   // finished only
    if (((int)unit_get_list_node(u)) && ((int)unit_get_chain(st))) return;  // the effect's own condition (0x5BA645)
    int t = unit_type_get_id(td);
    for (int l = 0; l < 3; l++)
        for (int i = 0; i < 7 && UPG(l, i) >= 0; i++)
            if (UPG(l, i) == t && i < g_upg.best[l] && !g_upg.avail[l][i]) {
                g_upg.u[g_upg.n] = u;
                g_upg.to[g_upg.n++] = UPG(l, g_upg.best[l]);
                return;
            }
}
internal unsigned g_upg_done[16];
internal void upgrade_stragglers(void) {   // the safe point (control.c hook_sim_receive)
    static uint32_t last;
    uint32_t now = game_time();
    if (!now || (last && now >= last && now - last < 30000)) return;
    last = now;
    for (int p = 1; p < 16; p++) {
        ee_player *pl = g_players[p];
        if (!pl || player_get_out(pl) || is_vanilla(pl)) continue;
        int any = 0;
        for (int l = 0; l < 3; l++) {
            g_upg.best[l] = -1;
            for (int i = 0; i < 7; i++) {
                g_upg.avail[l][i] = UPG(l, i) >= 0 &&
                    can_build(pl, UPG(l, i));
                if (g_upg.avail[l][i]) g_upg.best[l] = i;
            }
            any |= g_upg.best[l] > 0;
        }
        if (!any) continue;
        g_upg.pl = pl;
        g_upg.n = 0;
        grid_each(upg_scan, NULL);   // collected first: the conversion replaces the object
        int pi = player_get_index(pl);
        for (int k = 0; k < g_upg.n; k++) {
            ee_unit_type *ntd = type_data(g_upg.to[k]);
            if (!ntd) continue;
            int from = unit_type_get_id(unit_get_type_data(g_upg.u[k])), x = unit_get_x(g_upg.u[k]), y = unit_get_y(g_upg.u[k]);
            unit_convert(g_upg.u[k], pi, ntd);
            if (g_unit_converted) g_unit_converted(g_upg.to[k], x, y);
            g_upg_done[p & 15]++;
            if (g_log) fprintf(g_log, "# upgrades: P%d at %.1f min: %d at %d,%d left behind by its upgrade - now %d\n", p,
                               now / 60000.0, from, x, y, g_upg.to[k]);
        }
    }
}

// The AI's build queue names types: an entry queued before its line's upgrade (the defence planner's "tower 47" in
// the Stone Age) was laid out afterwards as the old type, which the upgrade then never converts - a player's build
// menu only offers the new one. hook_place (econ.c) asks here: a tower, wall or gate type the AI can no longer
// build becomes the newest one of its line it can.
internal int upgraded_type(ee_player *pl, int type) {
    if (!pl) return type;
    for (int l = 0; l < 3; l++)
        for (int i = 0; i < 7 && UPG(l, i) >= 0; i++) {
            if (UPG(l, i) != type) continue;
            if (can_build(pl, type)) return type;
            for (int j = 6; j > i; j--)
                if (UPG(l, j) >= 0 && can_build(pl, UPG(l, j))) {
                    if (g_log) fprintf(g_log, "# upgrades: P%d places %d instead of %d (upgraded since it was queued)\n",
                                       player_get_index(pl), UPG(l, j), type);
                    return UPG(l, j);
                }
            return type;
        }
    return type;
}

// a new game (the game start hook): plans, scans and musters of the last game (its players, groups and
// times; a muster still on sent the citizens to the old rally point)
internal void defence_new_game(void) {
    memset(g_plan, 0, sizeof g_plan);
    memset(&g_scan, 0, sizeof g_scan);
    memset(g_def_towers, 0, sizeof g_def_towers);
    memset(g_def_walls_built, 0, sizeof g_def_walls_built);
    memset(g_csiege_orders, 0, sizeof g_csiege_orders);
    memset(g_csiege_skipped, 0, sizeof g_csiege_skipped);
    memset(g_militia_caught, 0, sizeof g_militia_caught);
    memset(g_militia_orders, 0, sizeof g_militia_orders);
    memset(g_home_trained, 0, sizeof g_home_trained);
    memset(g_mus, 0, sizeof g_mus);
    memset(g_hold_st, 0, sizeof g_hold_st);
    memset(g_aa_st, 0, sizeof g_aa_st);
    memset(g_upg_done, 0, sizeof g_upg_done);
}
