// THE FITTED RING ([ai_base] ring_fitted, with base_ring; [autotest] v_ring_fitted for the variant players. The player
// 2026-11-27: "the current wall planbing of the ai is primitive, how about it actualy uses pathfidning to plan an
// enclosure for its base with needed gates etc? being able to fit it around existing building and mines"). THE TOWN
// RING (defence.c) draws eight straight sides round the town whatever lies there; this plans the ring's line over the
// ground as it is: the cheapest set of tiles that, walled, cuts every walk from the town to the land around.
// The window: FIT_W tiles round the base point. Outside: the ground further than FIT_FAR tiles from every building of
// the town, and the ground nearer an enemy building it has seen than the town (two towns' rings never cross).
// Inside for certain: the town (ring_town: its buildings joined to the base point, not its towers) and the mines its
// citizens work by it, the way a citizen walks from the base point to each of them, and the open ground within
// FIT_MARGIN steps of those (room to walk and to build: no piece is planned nearer). A tile's price: a wall piece
// FIT_WALL where one can stand, and a quarter more for every FIT_WALL tiles from the town (a neck far out only when
// it is that much shorter); a tree FIT_TREE percent of a piece - no piece can stand there, it closes the line until
// it is cut down (the piece laid then), but not to everyone: the player 2026-11-27, of the first version's half a
// piece, "there are units that can traverse forests, while clsoing them of or eclosing them is often unpractical,
// the ai should cosider it if its relativly cheap" (the data's "Can walk through trees": partisans, barbarians). So
// the line leans on a wood only where walling past it would take over half as many pieces again as the wood's tiles
// on the line - along a wood's straight edge it is walled, through the middle of a broad one it is not; a
// mine's rock or a building of its own FIT_OBJ; its own walls, standing, going up or ordered, nothing. Water and
// cliffs are no tiles at all - the line runs to them and stops; shallows a citizen wades through are open ground
// like any (the player 2026-11-27: "shallows are threated as open land right? walls can also be build on them" - the
// tiles are asked as the game's routes ask them, the citizen's table of ground kinds, and a piece is planned only
// where the wall's own table lets it stand: the log counts both). Another player's building or wall costs a piece like
// open ground (not the town's to lean on: the first try's lines ran along the neighbours' walls). The cheapest such line is a smallest cut between the town and
// the outside (the tiles are the nodes, a walk goes four ways: a chain of pieces side by side or corner to corner
// closes it, as the game's diagonal walls do); of the cuts of one price the one nearest the town. Dinic's flow over
// the window's 16000 tiles: 1-9 ms a plan.
// The plan is kept while it holds: every look (encl_plan's, a minute) asks only whether the town still cannot walk
// out past its line (2: a building of the town beyond it) and whether something of the town has come within
// FIT_PRESS steps of a piece not laid yet (3). Planned anew then, the walls that stand free: the line moves out round
// what was added and the stretch behind stays an inner wall (the player's choice of 2026-11-13). Once a piece of the
// plan stands it is kept until it is all laid, FIT_LOCK at most (the first try planned anew each time a building
// went up outside: 15 plans in 38 minutes, each leaving its fragments). A piece the enemy broke is the plan's still -
// its own walls on the line are its tiles too - and is laid again without a new plan.
// THE GATES: where its own ways cross the line. The ways: from the base point to each of its buildings, to each
// site its citizens work, and - through the window's edge - to its buildings further off, the land territories
// beside its own and the nearest enemy buildings it has seen. With the line shut but for the gates so far, the
// destination that got the longest way round (cut off, or FIT_DETOUR steps and a quarter more than before) gets a
// gate: of the straight runs of five of the line's pieces (to lay, or standing) on level ground with open ground on
// both sides (what the game's gate needs, gate_fits) the one nearest where that destination's way crossed the line,
// within FIT_GATE_REACH; FIT_GATES at most. The gates' pieces are laid first and nothing is laid that would shut
// something in (defence.c encl_lay, NOTHING SHUT IN). Towers: 2 tiles inside the line, beside the gates first, then
// FIT_TOWER_GAP apart along it.
// A GATE'S PLACE: the cheapest line has no such run where it is short - across a neck of three tiles between a wood
// and a cliff, along a line that turns every other tile (the first games: of the ways that wanted a gate up to 10 a
// plan found none, 104 looks of one AI's - and what cannot get a gate is never closed, NOTHING SHUT IN). Then the
// gate's place is chosen first and the line planned through it: five open tiles in a row on level ground, open
// ground along both sides, within FIT_PIN_REACH of where the way crossed the line and not on ground that is inside
// for certain. Its side towards the base point is inside for certain from then on, its other side outside, its five
// tiles free: the smallest cut then runs along exactly those five and on from their ends to whatever closes it
// there. Planned anew at once for it (the walls that stand free, as ever), FIT_GATES such places a town.
#define FIT_W 125
#define FIT_H (FIT_W / 2)
#define FIT_R 60               // further from the base point (the greater of |dx|, |dy|): outside whatever else
#define FIT_N (FIT_W * FIT_W)
#define FIT_FAR 28
#define FIT_MARGIN 10
#define FIT_PRESS 2
#define FIT_LOCK 900000
#define FIT_WALL 8
#define FIT_TREE 150
#define FIT_OBJ 2
#define FIT_INF 0x3FFFFFFF
#define FIT_GATES 6
#define FIT_DETOUR 16
#define FIT_GATE_REACH 16
#define FIT_TOWER_GAP 8
#define FIT_PIN_REACH 10
// a tile: no ground a citizen walks (water, cliffs, off the map), open ground, a tree, something else in the way, a
// wall's piece of its own (standing, going up, ordered; a gate)
enum { FC_NONE, FC_FREE, FC_TREE, FC_OBJ, FC_OWN };
enum { FF_GATE = 1, FF_GPLAN = 2, FF_PROT = 4, FF_STREET = 8, FF_SRC = 16, FF_SNK = 32, FF_CUT = 64, FF_SHUT = 128, FF_IN = 256, FF_NOWALL = 512, FF_OTHER = 1024, FF_PIN = 2048 };
internal struct fit_plan {
    ee_player *pl;
    int n; int16_t x[ENCL_RING], y[ENCL_RING];   // the line's tiles
    int ngate; struct { int16_t x, y; uint8_t dir; } gate[FIT_GATES];
    int npin; struct { int16_t x, y; uint8_t dir; int8_t in; } pin[FIT_GATES];   // A GATE'S PLACE: its middle, its way, the side that is the town's (fit_pin_side's)
    unsigned pins, pins_lost, pins_done;   // such places made; given up (their ground taken, the town grown onto them); their gate stands
    unsigned looks, shut;   // SHUT OFF: looks since its walls began; those at which no walk led from the town to the outside
    int cost, nogate, open, trees, objs, stand;   // the last look's: the cut's price, destinations no gate could be found for, pieces to lay, the line's tiles under trees, under something else, walled
    unsigned plans, fails, held; uint32_t us, us_max, log_t, laid_t; uint8_t why;   // held: looks that kept a plan that no longer holds (FIT_LOCK); laid_t: since a piece of it stands
    unsigned ng_line, ng_level, ng_sides;   // THE GATES, why the runs of five nearest a crossing were none: not five of the line in a row, not level, no open ground on both sides
} g_fit[16];
internal struct {
    int x0, y0, start, wade, wade_wall;   // wade: its open tiles under shallow water; those a wall may stand on
    uint8_t cls[FIT_N], dt[FIT_N], de[FIT_N]; uint16_t f[FIT_N]; float hgt[FIT_N];   // dt, de: tiles to the town's nearest building, to the nearest enemy building seen (255: none)
    int d0[FIT_N], d1[FIT_N], q[2 * FIT_N + 2];
    int head[2 * FIT_N + 2], level[2 * FIT_N + 2], it[2 * FIT_N + 2], st[2 * FIT_N + 2];   // the flow's: a tile's two nodes 2i (in) and 2i + 1 (out)
    struct { int to, cap, next; } ed[FIT_N * 14 + 16];
    int ne;
} g_fitw;   // one look's work, the sim thread's
internal int fit_on_line(unsigned idx, int x, int y) {   // a tile of the kept plan's line
    for (int j = 0; j < g_fit[idx].n; j++) if (g_fit[idx].x[j] == x && g_fit[idx].y[j] == y) return 1;
    return 0;
}
internal const int8_t k_fit_dx[4] = { 1, -1, 0, 0 }, k_fit_dy[4] = { 0, 0, 1, -1 };
internal int fit_has(int x, int y) { return x >= g_fitw.x0 && y >= g_fitw.y0 && x < g_fitw.x0 + FIT_W && y < g_fitw.y0 + FIT_W; }
internal int fit_i(int x, int y) { return (y - g_fitw.y0) * FIT_W + x - g_fitw.x0; }
internal int fit_cheb(int i) { int dx = abs(i % FIT_W - FIT_H), dy = abs(i / FIT_W - FIT_H); return dx > dy ? dx : dy; }   // from the base point
#define FIT_OPEN(j) (g_fitw.cls[j] == FC_FREE || (g_fitw.f[j] & (FF_GATE | FF_GPLAN)))   // its own units walk there (a planned gate's tiles too)
// the window round b as it stands now: each tile's kind, its own walls and open gates, the runs ordered and not there yet
internal void fit_grid(ee_player *pl, struct encl *e, ivec2 b, ee_map *map, int n, uint32_t now) {
    static ee_unit_type *walker;
    static const enum ut_name wline[] = { UT_WALL_LINE };   // oldest first
    ee_unit_type *wtd = NULL;   // the wall it lays (encl_lay's choice): where its ground table lets it stand
    for (int i = (int)(sizeof wline / sizeof *wline) - 1; i >= 0 && !wtd; i--) if (ut_has(g_ut[wline[i]]) && can_build(pl, g_ut[wline[i]])) wtd = type_data(g_ut[wline[i]]);
    if (!walker) walker = type_data(UT(CITIZEN));
    g_fitw.wade = g_fitw.wade_wall = 0;
    int es = map_get_edge_sum(map), es2 = map_get_edge_sum_2(map), ed = map_get_edge_diff(map);
    g_fitw.x0 = b.x - FIT_H; g_fitw.y0 = b.y - FIT_H;
    for (int i = 0; i < FIT_N; i++) {
        int x = g_fitw.x0 + i % FIT_W, y = g_fitw.y0 + i / FIT_W, c = FC_NONE, f = 0;
        ee_tile *tile = x >= 0 && y >= 0 && x < n && y < n && x + y > es && x + y < es2 && y - x < ed && x - y < ed ? map_get_tiles(map)[y * n + x] : NULL;
        g_fitw.hgt[i] = tile ? tile_get_height(tile) : 0;
        if (tile && walker && unit_type_terrain_ok(walker, tile_get_kind(tile))) {
            int ok = rm_tile_ok(map, n, x, y), own = 0, other = 0, tree = 0;
            for (void *node = tile_get_objects(tile); node; node = tilenode_get_next(node)) {
                ee_unit *o = tilenode_get_object(node);
                ee_unit_type *td = o ? unit_get_type_data(o) : NULL;
                ee_unit_stats *st = o ? unit_get_stats(o) : NULL;
                ee_player *ow = td ? unit_get_owner(o) : NULL;
                if (!td || unit_get_hp(o) <= 0 || (st && unit_stats_get_speed(st) > 0)) continue;
                int cl = unit_type_get_class_id(td);
                if (unit_type_get_is_wall(td) && ow == pl) own = 1;
                else if (cl == CLS_RESOURCE) tree |= unit_type_get_resource_type(td) == 3;
                else if (ow && ow != pl && player_get_index(ow) > 0 && (unit_type_get_is_wall(td) || cl == CLS_BUILDING || cl == CLS_FORTRESS)) other = 1;
            }
            if (own) {   // its wall, standing or going up: shut - but a gate of its that opens to it
                c = FC_OWN;
                if (ok == 2) { ee_unit *g = rm_gate_on(tile); if (g && gate_opens_for(g, pl)) f |= FF_GATE; }
            } else if (other) { c = FC_OBJ; f |= FF_OTHER; }   // another player's: a piece's price, laid when it is gone
            else if (ok == 1) {
                c = FC_FREE;
                if (wtd && !unit_type_terrain_ok(wtd, tile_get_kind(tile))) f |= FF_NOWALL;   // ground it walks and no wall stands on
                if (tile_is_shallows(tile)) { g_fitw.wade++; g_fitw.wade_wall += !(f & FF_NOWALL); }
            } else c = tree ? FC_TREE : FC_OBJ;
        }
        g_fitw.cls[i] = (uint8_t)c; g_fitw.f[i] = (uint16_t)f;
    }
    for (int k = 0; k < 24; k++) {   // a run ordered: as good as standing
        if (!e->pend[k].t || now - e->pend[k].t >= 240000) continue;
        int ax = e->pend[k].ax, ay = e->pend[k].ay, dx = e->pend[k].bx - ax, dy = e->pend[k].by - ay;
        int len = abs(dx) > abs(dy) ? abs(dx) : abs(dy), sx = dx > 0 ? 1 : dx < 0 ? -1 : 0, sy = dy > 0 ? 1 : dy < 0 ? -1 : 0;
        for (int j = 0; j <= len; j++) {
            int x = ax + sx * j, y = ay + sy * j;
            if (fit_has(x, y) && g_fitw.cls[fit_i(x, y)] == FC_FREE) g_fitw.cls[fit_i(x, y)] = FC_OWN;
        }
    }
    for (int k = 0; k < e->nopen; k++)   // a piece taken down for a pocket: never walled again
        if (fit_has(e->open_x[k], e->open_y[k])) g_fitw.f[fit_i(e->open_x[k], e->open_y[k])] |= FF_NOWALL;
    g_fitw.start = -1;
    for (int r = 0; r <= 6 && g_fitw.start < 0; r++)   // the open tile nearest the base point
        for (int dy = -r; dy <= r && g_fitw.start < 0; dy++)
            for (int dx = -r; dx <= r; dx++)
                if ((abs(dx) == r || abs(dy) == r) && FIT_OPEN(fit_i(b.x + dx, b.y + dy))) { g_fitw.start = fit_i(b.x + dx, b.y + dy); break; }
}
// its own units' walk from tile s (four ways) over open ground and its open gates, not over the tiles flagged `shut`
internal void fit_walk(int s, int shut, int *d) {
    int h = 0, t = 0;
    for (int i = 0; i < FIT_N; i++) d[i] = -1;
    d[s] = 0; g_fitw.q[t++] = s;
    while (h < t) {
        int i = g_fitw.q[h++], x = i % FIT_W, y = i / FIT_W;
        for (int k = 0; k < 4; k++) {
            int nx = x + k_fit_dx[k], ny = y + k_fit_dy[k], j = ny * FIT_W + nx;
            if (nx < 0 || ny < 0 || nx >= FIT_W || ny >= FIT_W || d[j] >= 0 || !FIT_OPEN(j) || (g_fitw.f[j] & shut)) continue;
            d[j] = d[i] + 1; g_fitw.q[t++] = j;
        }
    }
}
// tiles (eight ways, whatever stands between) from the tiles flagged `from`, or from the points p, into d; 255: none or further
internal void fit_spread(int from, const ivec2 *p, int np, uint8_t *d) {
    int h = 0, t = 0;
    memset(d, 255, FIT_N);
    for (int i = 0; from && i < FIT_N; i++) if (g_fitw.f[i] & from) { d[i] = 0; g_fitw.q[t++] = i; }
    for (int k = 0; k < np; k++) if (fit_has(p[k].x, p[k].y) && d[fit_i(p[k].x, p[k].y)]) { d[fit_i(p[k].x, p[k].y)] = 0; g_fitw.q[t++] = fit_i(p[k].x, p[k].y); }
    while (h < t) {
        int i = g_fitw.q[h++], x = i % FIT_W, y = i / FIT_W;
        if (d[i] >= 254) continue;
        for (int m = 0; m < 9; m++) {
            int nx = x + m % 3 - 1, ny = y + m / 3 - 1, j = ny * FIT_W + nx;
            if (nx < 0 || ny < 0 || nx >= FIT_W || ny >= FIT_W || d[j] != 255) continue;
            d[j] = (uint8_t)(d[i] + 1); g_fitw.q[t++] = j;
        }
    }
}
// the footprint (x, y, sz) is the town's: its tiles, and the way there from the base point - inside the kept plan's
// line where there is one (d0: the walk with it shut), else the shortest (d1: a way that left through a stretch not
// laid yet and came back would press on the line every look)
internal void fit_protect(int x, int y, int sz) {
    int best = -1;
    const int *d = g_fitw.d0;
    for (int pass = 0; pass < 2 && best < 0; pass++, d = g_fitw.d1)
        for (int ty = y - 1; ty <= y + sz; ty++)
            for (int tx = x - 1; tx <= x + sz; tx++) {
                if (!fit_has(tx, ty)) continue;
                int j = fit_i(tx, ty);
                if (tx >= x && tx < x + sz && ty >= y && ty < y + sz) g_fitw.f[j] |= FF_PROT;
                if (d[j] >= 0 && (best < 0 || d[j] < d[best])) best = j;
            }
    d = best >= 0 && g_fitw.d0[best] >= 0 ? g_fitw.d0 : g_fitw.d1;
    for (int j = best; j >= 0 && !(g_fitw.f[j] & FF_STREET); ) {
        g_fitw.f[j] |= FF_STREET;
        int nx = -1;
        for (int k = 0; k < 4 && nx < 0; k++) {
            int tx = j % FIT_W + k_fit_dx[k], ty = j / FIT_W + k_fit_dy[k];
            if (tx >= 0 && ty >= 0 && tx < FIT_W && ty < FIT_W && d[ty * FIT_W + tx] == d[j] - 1) nx = ty * FIT_W + tx;
        }
        j = nx;
    }
}
// what is inside for certain: the town's tiles and ways, and the open ground within `margin` steps of them - never
// a tile that is outside or beside one (the flow would have no cut)
internal void fit_sources(int margin) {
    int h = 0, t = 0;
    for (int i = 0; i < FIT_N; i++) { g_fitw.f[i] &= (uint16_t)~FF_SRC; g_fitw.d1[i] = -1; }
    for (int i = 0; i < FIT_N; i++) {
        if (!(g_fitw.f[i] & (FF_PROT | FF_STREET)) || g_fitw.cls[i] == FC_NONE || g_fitw.cls[i] == FC_OWN || fit_cheb(i) >= FIT_R) continue;
        g_fitw.f[i] = (uint16_t)((g_fitw.f[i] | FF_SRC) & ~FF_SNK);
        for (int k = 0; k < 4; k++) g_fitw.f[i + k_fit_dy[k] * FIT_W + k_fit_dx[k]] &= (uint16_t)~FF_SNK;   // fit_cheb < FIT_R: in the window
        g_fitw.d1[i] = 0; g_fitw.q[t++] = i;
    }
    while (h < t) {
        int i = g_fitw.q[h++];
        if (g_fitw.d1[i] >= margin) continue;
        for (int k = 0; k < 4; k++) {
            int j = i + k_fit_dy[k] * FIT_W + k_fit_dx[k], by_out = 0;
            if (g_fitw.d1[j] >= 0 || g_fitw.cls[j] != FC_FREE || (g_fitw.f[j] & (FF_SNK | FF_NOWALL)) || fit_cheb(j) >= FIT_R) continue;
            for (int m = 0; m < 4; m++) by_out |= g_fitw.f[j + k_fit_dy[m] * FIT_W + k_fit_dx[m]] & FF_SNK;
            if (by_out) continue;
            g_fitw.f[j] |= FF_SRC; g_fitw.d1[j] = g_fitw.d1[i] + 1; g_fitw.q[t++] = j;
        }
    }
}
internal void fit_edge(int a, int b, int cap) {
    g_fitw.ed[g_fitw.ne].to = b; g_fitw.ed[g_fitw.ne].cap = cap; g_fitw.ed[g_fitw.ne].next = g_fitw.head[a]; g_fitw.head[a] = g_fitw.ne++;
    g_fitw.ed[g_fitw.ne].to = a; g_fitw.ed[g_fitw.ne].cap = 0; g_fitw.ed[g_fitw.ne].next = g_fitw.head[b]; g_fitw.head[b] = g_fitw.ne++;
}
internal int fit_price(int i) {   // what closing the tile costs
    int f = g_fitw.f[i], c = g_fitw.cls[i], wall = FIT_WALL + (g_fitw.dt[i] < 255 ? g_fitw.dt[i] : 255) / 4;
    return f & (FF_SRC | FF_NOWALL) ? FIT_INF : c == FC_OWN || (f & FF_PIN) ? 0 : c == FC_FREE || (f & FF_OTHER) ? wall : c == FC_TREE ? wall * FIT_TREE / 100 : FIT_OBJ;
}
// the smallest cut between what is inside for certain and the outside: its tiles flagged FF_CUT, its price - -1: none
// (the town reaches the outside over tiles no wall can stand on)
internal int fit_flow(void) {
    int S = 2 * FIT_N, T = S + 1, nn = T + 1, flow = 0;
    g_fitw.ne = 0;
    for (int i = 0; i < nn; i++) g_fitw.head[i] = -1;
    for (int i = 0; i < FIT_N; i++) {
        if (g_fitw.cls[i] == FC_NONE) continue;
        int f = g_fitw.f[i], x = i % FIT_W, y = i / FIT_W;
        fit_edge(2 * i, 2 * i + 1, fit_price(i));
        for (int k = 0; k < 4; k++) {
            int nx = x + k_fit_dx[k], ny = y + k_fit_dy[k];
            if (nx >= 0 && ny >= 0 && nx < FIT_W && ny < FIT_W && g_fitw.cls[ny * FIT_W + nx] != FC_NONE) fit_edge(2 * i + 1, 2 * (ny * FIT_W + nx), FIT_INF);
        }
        if (f & FF_SRC) fit_edge(S, 2 * i, FIT_INF);
        if (f & FF_SNK) fit_edge(2 * i + 1, T, FIT_INF);
    }
    for (;;) {   // Dinic: the levels by a walk from S, then every way along them filled
        int h = 0, t = 0, sp = 0, u = S;
        for (int i = 0; i < nn; i++) g_fitw.level[i] = -1;
        g_fitw.level[S] = 0; g_fitw.q[t++] = S;
        while (h < t && g_fitw.level[T] < 0) {
            int a = g_fitw.q[h++];
            for (int ei = g_fitw.head[a]; ei >= 0; ei = g_fitw.ed[ei].next)
                if (g_fitw.ed[ei].cap > 0 && g_fitw.level[g_fitw.ed[ei].to] < 0) { g_fitw.level[g_fitw.ed[ei].to] = g_fitw.level[a] + 1; g_fitw.q[t++] = g_fitw.ed[ei].to; }
        }
        if (g_fitw.level[T] < 0) break;
        memcpy(g_fitw.it, g_fitw.head, (size_t)nn * sizeof g_fitw.it[0]);
        for (;;) {
            if (u == T) {
                int f = FIT_INF, i = 0;
                for (int k = 0; k < sp; k++) if (g_fitw.ed[g_fitw.st[k]].cap < f) f = g_fitw.ed[g_fitw.st[k]].cap;
                if (f >= FIT_INF) return -1;
                for (int k = 0; k < sp; k++) { g_fitw.ed[g_fitw.st[k]].cap -= f; g_fitw.ed[g_fitw.st[k] ^ 1].cap += f; }
                flow += f;
                while (i < sp && g_fitw.ed[g_fitw.st[i]].cap) i++;   // back to the first edge it filled
                sp = i; u = sp ? g_fitw.ed[g_fitw.st[sp - 1]].to : S;
                continue;
            }
            int ei = g_fitw.it[u];
            while (ei >= 0 && !(g_fitw.ed[ei].cap > 0 && g_fitw.level[g_fitw.ed[ei].to] == g_fitw.level[u] + 1)) ei = g_fitw.ed[ei].next;
            g_fitw.it[u] = ei;
            if (ei >= 0) { g_fitw.st[sp++] = ei; u = g_fitw.ed[ei].to; continue; }
            if (!sp) break;
            g_fitw.level[u] = -1;   // a dead end: not entered again at this level
            sp--; u = sp ? g_fitw.ed[g_fitw.st[sp - 1]].to : S;
            g_fitw.it[u] = g_fitw.ed[g_fitw.st[sp]].next;
        }
    }
    {   // the cut nearest the town: what S still reaches; its tiles those it reaches into and not through, with a
        // tile beyond it does not reach (a wall of its own inside the line has the town on both sides: not the line's)
        int h = 0, t = 0;
        for (int i = 0; i < nn; i++) g_fitw.level[i] = 0;
        g_fitw.level[S] = 1; g_fitw.q[t++] = S;
        while (h < t) {
            int a = g_fitw.q[h++];
            for (int ei = g_fitw.head[a]; ei >= 0; ei = g_fitw.ed[ei].next)
                if (g_fitw.ed[ei].cap > 0 && !g_fitw.level[g_fitw.ed[ei].to]) { g_fitw.level[g_fitw.ed[ei].to] = 1; g_fitw.q[t++] = g_fitw.ed[ei].to; }
        }
        for (int i = 0; i < FIT_N; i++) {
            int beyond = 0, x = i % FIT_W, y = i / FIT_W;
            g_fitw.f[i] &= (uint16_t)~FF_CUT;
            if (g_fitw.cls[i] == FC_NONE || !g_fitw.level[2 * i] || g_fitw.level[2 * i + 1]) continue;
            for (int k = 0; k < 4 && !beyond; k++) {
                int nx = x + k_fit_dx[k], ny = y + k_fit_dy[k];
                beyond = nx >= 0 && ny >= 0 && nx < FIT_W && ny < FIT_W && g_fitw.cls[ny * FIT_W + nx] != FC_NONE && !g_fitw.level[2 * (ny * FIT_W + nx)];
            }
            if (beyond || (g_fitw.f[i] & FF_SNK)) g_fitw.f[i] |= FF_CUT;
        }
    }
    return flow;
}
// what the line (FF_CUT) holds in, from the tiles inside for certain (FF_SRC) - trees, buildings and its own walls
// off the line may go, so the walk passes them: FF_IN. 1: it gets outside
internal int fit_inside(void) {
    int h = 0, t = 0, out = 0;
    for (int i = 0; i < FIT_N; i++) {
        g_fitw.f[i] &= (uint16_t)~FF_IN;
        if ((g_fitw.f[i] & FF_SRC) && !(g_fitw.f[i] & FF_CUT)) { g_fitw.f[i] |= FF_IN; g_fitw.q[t++] = i; }
    }
    while (h < t) {
        int i = g_fitw.q[h++], x = i % FIT_W, y = i / FIT_W;
        out |= g_fitw.f[i] & FF_SNK;
        for (int k = 0; k < 4; k++) {
            int nx = x + k_fit_dx[k], ny = y + k_fit_dy[k], j = ny * FIT_W + nx;
            if (nx < 0 || ny < 0 || nx >= FIT_W || ny >= FIT_W || g_fitw.cls[j] == FC_NONE || (g_fitw.f[j] & (FF_IN | FF_CUT))) continue;
            g_fitw.f[j] |= FF_IN; g_fitw.q[t++] = j;
        }
    }
    return out != 0;
}
// does the kept plan still hold: 0 yes, 2 the town walks out past it (a building of the town beyond it), 3 something
// of the town within FIT_PRESS steps of a piece not laid yet. Leaves its tiles flagged FF_CUT and what it holds FF_IN.
internal int fit_valid(struct fit_plan *fp) {
    int pressed = 0;
    fit_sources(FIT_PRESS);
    for (int i = 0; i < FIT_N; i++) g_fitw.f[i] &= (uint16_t)~FF_CUT;
    for (int k = 0; k < fp->n; k++) {
        if (!fit_has(fp->x[k], fp->y[k])) continue;
        int i = fit_i(fp->x[k], fp->y[k]);
        pressed |= g_fitw.f[i] & FF_SRC;
        g_fitw.f[i] |= FF_CUT;
    }
    return fit_inside() ? 2 : pressed ? 3 : 0;
}
// A GATE'S PLACE: the tiles along one side of the five (sg 1 or -1) into out - 5 along the tiles, 10 (6 of them
// twice) on a diagonal; -1: one of them beyond the window
internal int fit_pin_side(int cx, int cy, int dir, int sg, int *out) {
    int ux = k_gate_step[dir][0], uy = k_gate_step[dir][1], n = 0;
    for (int i = -2; i <= 2; i++)
        for (int k = 0; k < 4; k++) {
            int x = cx + ux * i + k_fit_dx[k], y = cy + uy * i + k_fit_dy[k];
            if ((k_fit_dx[k] * -uy + k_fit_dy[k] * ux) * sg <= 0) continue;
            if (!fit_has(x, y)) return -1;
            out[n++] = fit_i(x, y);
        }
    return n;
}
internal int fit_by(int j, int flag) {   // a tile beside j (four ways) has the flag
    int x = j % FIT_W, y = j / FIT_W, r = 0;
    for (int k = 0; k < 4; k++) {
        int nx = x + k_fit_dx[k], ny = y + k_fit_dy[k];
        if (nx >= 0 && ny >= 0 && nx < FIT_W && ny < FIT_W) r |= g_fitw.f[ny * FIT_W + nx] & flag;
    }
    return r != 0;
}
// can five tiles centred at (cx, cy) along dir be a gate's place, with side `in` the town's: the five open or its own
// pieces, level, not inside for certain; the town's side not outside nor beside it, the other side not inside for
// certain nor beside it, open ground all of both (after fit_sources(FIT_MARGIN): FF_SRC is the margin's)
internal int fit_pin_ok(ee_map *map, int cx, int cy, int dir, int in) {
    int ux = k_gate_step[dir][0], uy = k_gate_step[dir][1], side[10], n;
    float lo = 0, hi = 0;
    for (int i = -2; i <= 2; i++) {
        int x = cx + ux * i, y = cy + uy * i;
        if (!fit_has(x, y)) return 0;
        int j = fit_i(x, y);
        if ((g_fitw.cls[j] != FC_FREE && g_fitw.cls[j] != FC_OWN) || (g_fitw.f[j] & (FF_SRC | FF_SNK | FF_PROT | FF_STREET | FF_NOWALL | FF_GATE))) return 0;
        if (i == -2 || g_fitw.hgt[j] < lo) lo = g_fitw.hgt[j];
        if (i == -2 || g_fitw.hgt[j] > hi) hi = g_fitw.hgt[j];
    }
    if (hi - lo > map_get_height_step(map) * 0.5f + 0.001f) return 0;
    n = fit_pin_side(cx, cy, dir, in, side);
    for (int k = 0; k < n; k++) if (g_fitw.cls[side[k]] != FC_FREE || (g_fitw.f[side[k]] & (FF_SNK | FF_NOWALL | FF_PIN)) || fit_by(side[k], FF_SNK) || fit_cheb(side[k]) >= FIT_R - 1) return 0;
    if (n <= 0) return 0;
    n = fit_pin_side(cx, cy, dir, -in, side);
    for (int k = 0; k < n; k++) if (g_fitw.cls[side[k]] != FC_FREE || (g_fitw.f[side[k]] & (FF_SRC | FF_PROT | FF_STREET | FF_NOWALL | FF_PIN)) || fit_by(side[k], FF_SRC)) return 0;
    return n > 0;
}
// the kept places into the window before the flow (after fit_sources(FIT_MARGIN)): the five free, the town's side
// inside for certain, the other outside; one that can no longer be is given up
internal void fit_pins(struct fit_plan *fp, ee_map *map) {
    for (int p = 0; p < fp->npin; p++) {
        int side[10], n, ux = k_gate_step[fp->pin[p].dir][0], uy = k_gate_step[fp->pin[p].dir][1];
        if (!fit_pin_ok(map, fp->pin[p].x, fp->pin[p].y, fp->pin[p].dir, fp->pin[p].in)) {
            if (g_fitw.f[fit_i(fp->pin[p].x, fp->pin[p].y)] & FF_GATE) fp->pins_done++; else fp->pins_lost++;   // its gate stands: the line runs through it for nothing now
            fp->pin[p] = fp->pin[--fp->npin]; p--;
            continue;
        }
        for (int i = -2; i <= 2; i++) g_fitw.f[fit_i(fp->pin[p].x + ux * i, fp->pin[p].y + uy * i)] |= FF_PIN;
        n = fit_pin_side(fp->pin[p].x, fp->pin[p].y, fp->pin[p].dir, fp->pin[p].in, side);
        for (int k = 0; k < n; k++) g_fitw.f[side[k]] |= FF_SRC;
        n = fit_pin_side(fp->pin[p].x, fp->pin[p].y, fp->pin[p].dir, -fp->pin[p].in, side);
        for (int k = 0; k < n; k++) g_fitw.f[side[k]] |= FF_SNK;
    }
}
// a gate's five tiles centred at (cx, cy) along dir (gate_fits' dirs): pieces of the line - to lay, or its own standing
// -, on level ground, open ground on both sides of its middle. 0 it can be; else why not (1 the line, 2 level, 3 sides)
internal int fit_gate_no(ee_map *map, int cx, int cy, int dir) {
    int ux = k_gate_step[dir][0], uy = k_gate_step[dir][1], a = 0, b = 0;
    float lo = 0, hi = 0;
    for (int i = -2; i <= 2; i++) {
        int x = cx + ux * i, y = cy + uy * i;
        if (!fit_has(x, y)) return 1;
        int j = fit_i(x, y);
        if (!(g_fitw.f[j] & FF_SHUT) && !(g_fitw.cls[j] == FC_OWN && (g_fitw.f[j] & FF_CUT) && !(g_fitw.f[j] & (FF_GATE | FF_GPLAN)))) return 1;
        if (i == -2 || g_fitw.hgt[j] < lo) lo = g_fitw.hgt[j];
        if (i == -2 || g_fitw.hgt[j] > hi) hi = g_fitw.hgt[j];
    }
    if (hi - lo > map_get_height_step(map) * 0.5f + 0.001f) return 2;
    for (int k = 0; k < 4; k++) {
        int x = cx + k_fit_dx[k], y = cy + k_fit_dy[k], side = k_fit_dx[k] * -uy + k_fit_dy[k] * ux;
        if (!side || !fit_has(x, y) || !FIT_OPEN(fit_i(x, y)) || (g_fitw.f[fit_i(x, y)] & FF_CUT)) continue;
        if (side > 0) a = 1; else b = 1;
    }
    return a && b ? 0 : 3;
}
struct fit_dest { int16_t x, y; uint8_t sz, live; int c0, t0; };   // a footprint its units go to; its way without the line: length, the tile it ends on
// the steps to a destination by the walk d (the tile it ends on into *at): in the window to a tile of it or beside it;
// beyond, to the window's edge and straight on from there. -1: no way
internal int fit_cost(const struct fit_dest *D, const int *d, int *at) {
    int best = -1;
    *at = -1;
    if (fit_has(D->x - 1, D->y - 1) && fit_has(D->x + D->sz, D->y + D->sz)) {
        for (int ty = D->y - 1; ty <= D->y + D->sz; ty++)
            for (int tx = D->x - 1; tx <= D->x + D->sz; tx++) {
                int j = fit_i(tx, ty);
                if (d[j] >= 0 && (best < 0 || d[j] < best)) { best = d[j]; *at = j; }
            }
        return best;
    }
    for (int k = 0; k < 4 * (FIT_W - 1); k++) {
        int s = k / (FIT_W - 1), o = k % (FIT_W - 1);
        int j = s == 0 ? o : s == 1 ? (FIT_W - 1) * FIT_W + o + 1 : s == 2 ? (o + 1) * FIT_W : o * FIT_W + FIT_W - 1;
        if (d[j] < 0) continue;
        float dx = (float)(g_fitw.x0 + j % FIT_W - D->x), dy = (float)(g_fitw.y0 + j / FIT_W - D->y);
        int c = d[j] + (int)(1.25f * sqrtf(dx * dx + dy * dy));
        if (best < 0 || c < best) { best = c; *at = j; }
    }
    return best;
}
internal void fit_dest_add(struct fit_dest *dst, int *nd, int max, int x, int y, int sz) {
    if (*nd >= max) return;
    if (!fit_has(x - 1, y - 1) || !fit_has(x + sz, y + sz)) {   // beyond the window: one for each 16 x 16 tiles
        sz = 1;
        for (int k = 0; k < *nd; k++) if (dst[k].sz == 1 && dst[k].x / 16 == x / 16 && dst[k].y / 16 == y / 16 && !fit_has(dst[k].x - 1, dst[k].y - 1)) return;
    }
    dst[*nd].x = (int16_t)x; dst[*nd].y = (int16_t)y; dst[*nd].sz = (uint8_t)sz; dst[*nd].live = 1; (*nd)++;
}
// the line's tiles to lay flagged FF_SHUT, the kept gates' five FF_GPLAN and not shut (a gate no longer on the line is dropped)
internal void fit_shut(struct fit_plan *fp) {
    for (int i = 0; i < FIT_N; i++) {
        g_fitw.f[i] &= (uint16_t)~(FF_SHUT | FF_GPLAN);
        if ((g_fitw.f[i] & FF_CUT) && g_fitw.cls[i] == FC_FREE) g_fitw.f[i] |= FF_SHUT;
    }
    for (int g = 0; g < fp->ngate; g++) {
        int ux = k_gate_step[fp->gate[g].dir][0], uy = k_gate_step[fp->gate[g].dir][1], on = 1;
        for (int i = -2; i <= 2 && on; i++) {
            int x = fp->gate[g].x + ux * i, y = fp->gate[g].y + uy * i;
            on = fit_has(x, y) && (g_fitw.f[fit_i(x, y)] & FF_CUT) && (g_fitw.cls[fit_i(x, y)] == FC_FREE || g_fitw.cls[fit_i(x, y)] == FC_OWN);
        }
        if (!on) { fp->gate[g] = fp->gate[--fp->ngate]; g--; continue; }
        for (int i = -2; i <= 2; i++) { int j = fit_i(fp->gate[g].x + ux * i, fp->gate[g].y + uy * i); g_fitw.f[j] = (uint16_t)((g_fitw.f[j] | FF_GPLAN) & ~FF_SHUT); }
    }
}
// THE GATES: the kept ones still on the line (fit_shut), then one more for the destination with the longest way
// round while there is such
// - where none can be, A GATE'S PLACE for it (`pins`: may make them); the places made (the line is planned again then)
internal int fit_gates(ee_player *pl, struct fit_plan *fp, struct encl_scan *s, ivec2 b, ee_map *map, int pins) {
    static struct fit_dest dst[768];
    int nd = 0, made = 0, margin = 0;
    fit_shut(fp);
    for (int p = 0; p < fp->npin && fp->ngate < FIT_GATES; p++) {   // a gate's place on the line: its gate
        int have = 0, ux = k_gate_step[fp->pin[p].dir][0], uy = k_gate_step[fp->pin[p].dir][1];
        for (int g = 0; g < fp->ngate && !have; g++) have = abs(fp->gate[g].x - fp->pin[p].x) <= 4 && abs(fp->gate[g].y - fp->pin[p].y) <= 4;
        if (have || fit_gate_no(map, fp->pin[p].x, fp->pin[p].y, fp->pin[p].dir)) continue;
        fp->gate[fp->ngate].x = fp->pin[p].x; fp->gate[fp->ngate].y = fp->pin[p].y; fp->gate[fp->ngate++].dir = fp->pin[p].dir;
        for (int i = -2; i <= 2; i++) { int j = fit_i(fp->pin[p].x + ux * i, fp->pin[p].y + uy * i); g_fitw.f[j] = (uint16_t)((g_fitw.f[j] | FF_GPLAN) & ~FF_SHUT); }
    }
    for (int i = 0; i < s->nb; i++) fit_dest_add(dst, &nd, 768, s->b[i].x, s->b[i].y, s->bs[i]);   // its buildings
    void *cm = player_get_civ_mgr(pl);
    ee_resource_mgr *rm = cm ? civ_mgr_get_resource_mgr(cm) : NULL;
    for (int r = 0; rm && r < 7; r++) {   // the sites its citizens work
        uint8_t *list = resource_mgr_list_at(rm, r);
        for (unsigned q = 0, first = deque_front(list), cnt = deque_count(list); q < cnt && q < 1024; q++) {
            ee_unit *u = deque_at(list, first + q);
            ee_goal *goal = u && unit_get_hp(u) > 0 ? unit_get_goal(u) : NULL;
            ee_unit *site = goal && !strcmp(rtti_name(goal), "EEUGGather@@") ? goal_get_site(goal) : NULL;
            if (!site || unit_get_x(site) < 0) continue;
            int dup = 0;
            for (int k = 0; k < nd && !dup; k++) dup = dst[k].x == unit_get_x(site) && dst[k].y == unit_get_y(site);
            if (!dup) fit_dest_add(dst, &nd, 768, unit_get_x(site), unit_get_y(site), 1);
        }
    }
    unsigned home = ter_near(b.x, b.y);   // the land territories beside its own
    for (int k = 0; home < (unsigned)g_ter.n && k < g_ter.t[home].nbn; k++) {
        unsigned to = g_ter.nb[g_ter.t[home].nb0 + k].to;
        if (to < (unsigned)g_ter.n && !g_ter.t[to].water) fit_dest_add(dst, &nd, 768, g_ter.t[to].c.x, g_ter.t[to].c.y, 1);
    }
    for (int k = 0, first = -1; k < 2; k++) {   // the two nearest enemy buildings it has seen, 20 tiles apart
        int best = -1;
        for (int i = 0; i < s->neb; i++)
            if ((best < 0 || ivec2_dist2(s->eb[i], b) < ivec2_dist2(s->eb[best], b)) && (first < 0 || ivec2_dist2(s->eb[i], s->eb[first]) > 400)) best = i;
        if (best < 0) break;
        fit_dest_add(dst, &nd, 768, s->eb[best].x, s->eb[best].y, 1);
        first = best;
    }
    fit_walk(g_fitw.start, 0, g_fitw.d0);
    for (int k = 0; k < nd; k++) { dst[k].c0 = fit_cost(&dst[k], g_fitw.d0, &dst[k].t0); if (dst[k].c0 < 0) dst[k].live = 0; }
    fp->nogate = 0;
    for (int last = -1;;) {
        int worst = -1, most = 0, at;
        fit_walk(g_fitw.start, FF_SHUT, g_fitw.d1);
        for (int k = 0; k < nd; k++) {
            if (!dst[k].live) continue;
            int c1 = fit_cost(&dst[k], g_fitw.d1, &at), more = c1 < 0 ? 100000 : c1 - dst[k].c0;
            if (more > FIT_DETOUR + dst[k].c0 / 4 && more > most) { most = more; worst = k; }
        }
        if (worst < 0) break;
        if (worst == last) { dst[worst].live = 0; fp->nogate++; continue; }   // its gate did not shorten its way: no second one beside it
        last = worst;
        int X = -1, bc = -1, bdir = 0, bd = FIT_GATE_REACH * FIT_GATE_REACH + 1, why_d = 0x7FFFFFFF, why = 1;
        for (int j = dst[worst].t0; j >= 0 && g_fitw.d0[j] > 0; ) {   // its way without the line, back to the base point: where it crosses the line
            if (g_fitw.f[j] & FF_SHUT) X = j;
            int nx = -1;
            for (int k = 0; k < 4 && nx < 0; k++) {
                int tx = j % FIT_W + k_fit_dx[k], ty = j / FIT_W + k_fit_dy[k];
                if (tx >= 0 && ty >= 0 && tx < FIT_W && ty < FIT_W && g_fitw.d0[ty * FIT_W + tx] == g_fitw.d0[j] - 1) nx = ty * FIT_W + tx;
            }
            j = nx;
        }
        for (int i = 0; X >= 0 && fp->ngate < FIT_GATES && i < FIT_N; i++) {
            if (!(g_fitw.f[i] & FF_CUT) || (g_fitw.cls[i] != FC_FREE && g_fitw.cls[i] != FC_OWN)) continue;
            int dx = i % FIT_W - X % FIT_W, dy = i / FIT_W - X / FIT_W, d2 = dx * dx + dy * dy;
            if (d2 >= bd) continue;
            for (int dir = 1; dir <= 4; dir++) {
                int no = fit_gate_no(map, g_fitw.x0 + i % FIT_W, g_fitw.y0 + i / FIT_W, dir);
                if (!no) { bd = d2; bc = i; bdir = dir; break; }
                if (d2 < why_d || (d2 == why_d && no > why)) { why_d = d2; why = no; }   // the nearest run that was none: by what it came nearest to being one
            }
        }
        if (bc < 0) {   // no gate can be there: A GATE'S PLACE near the crossing, else its way round stays
            int pc = -1, pdir = 0, pin = 0, pd = FIT_PIN_REACH * FIT_PIN_REACH + 1;
            dst[worst].live = 0;
            if (pins && X >= 0 && fp->ngate + made < FIT_GATES && fp->npin < FIT_GATES) {
                if (!margin) { fit_sources(FIT_MARGIN); margin = 1; }   // the walk d1 is its scratch: walked again at the loop's top
                for (int i = 0; i < FIT_N; i++) {
                    int dx = i % FIT_W - X % FIT_W, dy = i / FIT_W - X / FIT_W, d2 = dx * dx + dy * dy, taken = 0;
                    if (d2 >= pd || g_fitw.d0[i] < 0) continue;
                    for (int p = 0; p < fp->npin && !taken; p++) taken = abs(fp->pin[p].x - g_fitw.x0 - i % FIT_W) < 8 && abs(fp->pin[p].y - g_fitw.y0 - i / FIT_W) < 8;
                    if (taken) continue;
                    for (int dir = 1; dir <= 4; dir++) {
                        int sa[10], sb[10], na = fit_pin_side(g_fitw.x0 + i % FIT_W, g_fitw.y0 + i / FIT_W, dir, 1, sa), nb = fit_pin_side(g_fitw.x0 + i % FIT_W, g_fitw.y0 + i / FIT_W, dir, -1, sb), da = 0, db = 0;
                        if (na <= 0 || nb <= 0) continue;
                        for (int k = 0; k < na; k++) { if (g_fitw.d0[sa[k]] < 0) da = -1000000; da += g_fitw.d0[sa[k]]; }
                        for (int k = 0; k < nb; k++) { if (g_fitw.d0[sb[k]] < 0) db = -1000000; db += g_fitw.d0[sb[k]]; }
                        if (da < 0 || db < 0 || da == db) continue;   // both sides walked to: the nearer the base point is the town's
                        if (fit_pin_ok(map, g_fitw.x0 + i % FIT_W, g_fitw.y0 + i / FIT_W, dir, da < db ? 1 : -1)) { pd = d2; pc = i; pdir = dir; pin = da < db ? 1 : -1; break; }
                    }
                }
            }
            if (pc >= 0) {
                fp->pin[fp->npin].x = (int16_t)(g_fitw.x0 + pc % FIT_W); fp->pin[fp->npin].y = (int16_t)(g_fitw.y0 + pc / FIT_W); fp->pin[fp->npin].dir = (uint8_t)pdir; fp->pin[fp->npin++].in = (int8_t)pin;
                for (int i = -2; i <= 2; i++) g_fitw.f[pc + (k_gate_step[pdir][1] * FIT_W + k_gate_step[pdir][0]) * i] |= FF_PIN;
                fp->pins++; made++;
                continue;
            }
            fp->nogate++;
            if (X >= 0 && fp->ngate < FIT_GATES) { if (why == 3) fp->ng_sides++; else if (why == 2) fp->ng_level++; else fp->ng_line++; }
            continue;
        }
        fp->gate[fp->ngate].x = (int16_t)(g_fitw.x0 + bc % FIT_W); fp->gate[fp->ngate].y = (int16_t)(g_fitw.y0 + bc / FIT_W); fp->gate[fp->ngate++].dir = (uint8_t)bdir;
        for (int i = -2; i <= 2; i++) { int j = bc + (k_gate_step[bdir][1] * FIT_W + k_gate_step[bdir][0]) * i; g_fitw.f[j] = (uint16_t)((g_fitw.f[j] | FF_GPLAN) & ~FF_SHUT); }
    }
    return made;
}
// the window round b with the town in it: what stands, the town's tiles and ways (the kept plan's line shut for
// them), what is outside. 0: no open ground by the base point
internal int fit_window(ee_player *pl, struct fit_plan *fp, struct encl *e, struct encl_scan *s, ivec2 b, ee_map *map, int n, uint32_t now, int *pheld, int *pmines) {
    fit_grid(pl, e, b, map, n, now);
    if (g_fitw.start < 0) return 0;
    for (int j = 0; j < fp->n; j++)   // the kept plan's line shut, but for its gates: the town's ways inside it (fit_protect)
        if (fit_has(fp->x[j], fp->y[j])) g_fitw.f[fit_i(fp->x[j], fp->y[j])] |= FF_CUT;
    fit_shut(fp);
    fit_walk(g_fitw.start, FF_SHUT, g_fitw.d0);
    fit_walk(g_fitw.start, 0, g_fitw.d1);
    static uint8_t in[2048];
    int held = 0, mines = 0;
    ring_town(s, b, in);
    g_fitw.f[g_fitw.start] |= FF_STREET;
    for (int i = 0; i < s->nb; i++) if (in[i]) { fit_protect(s->b[i].x, s->b[i].y, s->bs[i]); held++; }
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
            for (int i = 0; i < s->nb && !by_town; i++) by_town = in[i] && abs(s->b[i].x - x0) <= RING_MINE && abs(s->b[i].y - y0) <= RING_MINE;
            if (!by_town || !fit_has(x0, y0) || (g_fitw.f[fit_i(x0, y0)] & FF_PROT)) continue;
            fit_protect(x0, y0, sz); mines++;
        }
    }
    fit_spread(FF_PROT, NULL, 0, g_fitw.dt);
    fit_spread(0, s->eb, s->neb, g_fitw.de);
    for (int i = 0; i < FIT_N; i++)   // outside: far from the town, nearer an enemy building than it, by the window's edge
        if (g_fitw.cls[i] != FC_NONE && (g_fitw.dt[i] > FIT_FAR || g_fitw.de[i] < g_fitw.dt[i] || fit_cheb(i) > FIT_R)) g_fitw.f[i] |= FF_SNK;
    *pheld = held; *pmines = mines;
    return 1;
}
// SHUT OFF (the measure of a ring, the fitted one's and the eight sides' alike): does a walk over open ground - not
// through a wall, a gate, a tree or a building as they stand now - lead from the town (FF_SRC) to the outside? Counted
// a look once its walls have begun (e->ready)
internal void fit_shut_off(struct fit_plan *fp, struct encl *e) {
    int h = 0, t = 0, out = 0;
    if (!e->ready) return;
    for (int i = 0; i < FIT_N; i++) { g_fitw.d1[i] = 0; if ((g_fitw.f[i] & FF_SRC) && g_fitw.cls[i] == FC_FREE) { g_fitw.d1[i] = 1; g_fitw.q[t++] = i; } }
    while (h < t && !out) {
        int i = g_fitw.q[h++], x = i % FIT_W, y = i / FIT_W;
        out = g_fitw.f[i] & FF_SNK;
        for (int k = 0; k < 4; k++) {
            int nx = x + k_fit_dx[k], ny = y + k_fit_dy[k], j = ny * FIT_W + nx;
            if (nx < 0 || ny < 0 || nx >= FIT_W || ny >= FIT_W || g_fitw.d1[j] || g_fitw.cls[j] != FC_FREE) continue;
            g_fitw.d1[j] = 1; g_fitw.q[t++] = j;
        }
    }
    fp->looks++; fp->shut += !out && t;
}
// the eight sides' (defence.c ring_plan, ring_fitted off): only SHUT OFF's count, for the comparison
internal void fit_measure(ee_player *pl, unsigned idx, struct encl *e, struct encl_scan *s, ivec2 b, int n, uint32_t now) {
    struct fit_plan *fp = &g_fit[idx];
    ee_map *map = game_map();
    int held, mines;
    if (fp->pl != pl) { memset(fp, 0, sizeof *fp); fp->pl = pl; }
    if (!map || b.x < 0 || b.y < 0 || b.x >= n || b.y >= n || !fit_window(pl, fp, e, s, b, map, n, now, &held, &mines)) return;
    fit_sources(FIT_PRESS);
    fit_shut_off(fp, e);
    if (g_log && (!fp->log_t || now - fp->log_t >= 300000)) {
        fp->log_t = now ? now : 1;
        fprintf(g_log, "# ring: P%u at %.1f min: eight sides round %d buildings and %d mines by them - the town shut off from the land around at %u of %u looks since its walls began\n", idx, now / 60000.0, held, mines, fp->shut, fp->looks);
    }
}
// encl_plan's (base_ring with ring_fitted): the ring's tiles to lay into k, its gates, the box of tiles kept free
internal void fit_plan(ee_player *pl, unsigned idx, struct encl *e, struct encl_scan *s, ivec2 b, int n, struct encl_key *k, int *pnk, uint32_t now) {
    struct fit_plan *fp = &g_fit[idx];
    ee_map *map = game_map();
    LARGE_INTEGER hz, t0, t1;
    int held = 0, mines = 0;
    QueryPerformanceFrequency(&hz); QueryPerformanceCounter(&t0);
    if (fp->pl != pl) { memset(fp, 0, sizeof *fp); fp->pl = pl; }
    if (!map || b.x < 0 || b.y < 0 || b.x >= n || b.y >= n || !fit_window(pl, fp, e, s, b, map, n, now, &held, &mines)) return;
    int why = !fp->plans ? 1 : fit_valid(fp);
    if (why && fp->laid_t && fp->open && now - fp->laid_t < FIT_LOCK) { fp->held++; why = 0; }   // a piece of it stands: laid to its end first
    for (int round = 0, planned = 0; round < 3; round++) {   // planned again for A GATE'S PLACE, twice at most a look
        if (why) {
            for (int i = 0; i < FIT_N; i++) g_fitw.f[i] &= (uint16_t)~FF_PIN;
            fit_sources(FIT_MARGIN);
            fit_pins(fp, map);
            int flow = fit_flow();
            fp->n = 0; fp->cost = flow; fp->why = (uint8_t)why; fp->laid_t = 0;
            if (!planned++) fp->plans++;
            if (flow < 0) { fp->fails++; for (int i = 0; i < FIT_N; i++) g_fitw.f[i] &= (uint16_t)~FF_CUT; }
            for (int i = 0; i < FIT_N && fp->n < ENCL_RING; i++)
                if (g_fitw.f[i] & FF_CUT) { fp->x[fp->n] = (int16_t)(g_fitw.x0 + i % FIT_W); fp->y[fp->n++] = (int16_t)(g_fitw.y0 + i / FIT_W); }
            if (fit_inside() && flow >= 0) { fp->fails++; fp->n = 0; for (int i = 0; i < FIT_N; i++) g_fitw.f[i] &= (uint16_t)~FF_CUT; }   // never: a cut that leaks is no plan
        }
        if (!fit_gates(pl, fp, s, b, map, round < 2)) break;
        if (!why) why = 4;
    }
    int nk = *pnk;
    fp->open = fp->trees = fp->objs = fp->stand = 0;
    e->box_x = g_fitw.x0 - 1; e->box_y = g_fitw.y0 - 1;
    memset(e->box, 0, sizeof e->box);
    for (int j = 0; j < fp->n; j++) {
        int x = fp->x[j], y = fp->y[j];
        if (!fit_has(x, y)) continue;
        int i = fit_i(x, y), c = g_fitw.cls[i];
        for (int dy = -1; dy <= 1; dy++)
            for (int dx = -1; dx <= 1; dx++) {
                int bx = x + dx - e->box_x, by = y + dy - e->box_y;
                if (bx >= 0 && by >= 0 && bx < RING_BOX && by < RING_BOX) e->box[(by * RING_BOX + bx) >> 3] |= (uint8_t)(1 << (bx & 7));
            }
        if (c == FC_TREE) fp->trees++; else if (c == FC_OBJ) fp->objs++; else if (c == FC_OWN) fp->stand++; else fp->open++;
        if ((c == FC_FREE || c == FC_OWN) && nk < ENCL_RING) { k[nk].x = (int16_t)x; k[nk++].y = (int16_t)y; }   // to lay, or laid: a tree's tile once it is cut
    }
    *pnk = nk;
    if (fp->stand && !fp->laid_t) fp->laid_t = now ? now : 1;
    fit_shut_off(fp, e);
    for (int g = 0; g < fp->ngate && e->ngate < 16; g++) {
        if (ring_own_gate(pl, fp->gate[g].x, fp->gate[g].y)) continue;   // it stands
        e->gate[e->ngate].x = fp->gate[g].x; e->gate[e->ngate].y = fp->gate[g].y; e->gate[e->ngate].dir = fp->gate[g].dir; e->gate[e->ngate++].tries = 0;
    }
    QueryPerformanceCounter(&t1);
    fp->us = (uint32_t)((t1.QuadPart - t0.QuadPart) * 1000000 / hz.QuadPart);
    if (fp->us > fp->us_max) fp->us_max = fp->us;
    if (g_log && (why || !fp->log_t || now - fp->log_t >= 300000)) {
        static const char *const whys[5] = { "kept", "its first", "the town walks out past the last", "the town pressed on the last", "a gate's place" };
        int pin_gates = 0;
        for (int p = 0; p < fp->npin; p++) for (int g = 0; g < fp->ngate; g++) pin_gates += fp->gate[g].x == fp->pin[p].x && fp->gate[g].y == fp->pin[p].y;
        fp->log_t = now ? now : 1;
        fprintf(g_log, "# ring: P%u at %.1f min: fitted (%s; plan %u, %u without a cut, %u looks a plan that no longer held was kept) round %d buildings and %d mines by them - %d tiles: %d to wall, %d walled, %d under trees, %d under other things; "
                "price %d; %d gates, %d of them at the %d places chosen for one (%u such made, %u with their gate standing, %u given up), %d ways left without one (so far no run of five on the line %u, not level %u, no open ground on both sides %u); the town shut off from the land around at %u of %u looks since its walls began; %d open tiles of the window under shallow water, a wall can stand on %d; %u us (the longest %u)\n",
                idx, now / 60000.0, whys[why < 5 ? why : 0], fp->plans, fp->fails, fp->held, held, mines, fp->n, fp->open, fp->stand, fp->trees, fp->objs, fp->cost, fp->ngate, pin_gates, fp->npin, fp->pins, fp->pins_done, fp->pins_lost, fp->nogate,
                fp->ng_line, fp->ng_level, fp->ng_sides, fp->shut, fp->looks, g_fitw.wade, g_fitw.wade_wall, fp->us, fp->us_max);
    }
}
// the fitted ring's tower spots, 2 tiles inside the line: beside its gates first, then along it (right after fit_plan:
// the window's flags are its)
internal void fit_slots(struct encl *e, unsigned idx) {
    struct fit_plan *fp = &g_fit[idx];
    for (int c = 0; c < 2 * fp->ngate + fp->n && e->nslot < 96; c++) {
        int cx, cy, found = 0, sx = 0, sy = 0;
        if (c < 2 * fp->ngate) {
            int g = c / 2, sg = c & 1 ? 4 : -4;
            cx = fp->gate[g].x + sg * k_gate_step[fp->gate[g].dir][0]; cy = fp->gate[g].y + sg * k_gate_step[fp->gate[g].dir][1];
        } else { cx = fp->x[c - 2 * fp->ngate]; cy = fp->y[c - 2 * fp->ngate]; }
        for (int dy = -2; dy <= 2 && !found; dy++)
            for (int dx = -2; dx <= 2 && !found; dx++) {
                if ((abs(dx) != 2 && abs(dy) != 2) || !fit_has(cx + dx - 1, cy + dy - 1) || !fit_has(cx + dx + 1, cy + dy + 1)) continue;
                int j = fit_i(cx + dx, cy + dy), by_line = 0;
                if (g_fitw.cls[j] != FC_FREE || !(g_fitw.f[j] & FF_IN) || (g_fitw.f[j] & (FF_CUT | FF_PROT))) continue;
                for (int m = 0; m < 9; m++) by_line |= g_fitw.f[j + (m / 3 - 1) * FIT_W + m % 3 - 1] & FF_CUT;
                if (by_line) continue;
                sx = cx + dx; sy = cy + dy; found = 1;
            }
        for (int j = 0; j < e->nslot && found; j++) found = abs(e->sx[j] - sx) >= FIT_TOWER_GAP || abs(e->sy[j] - sy) >= FIT_TOWER_GAP;
        if (found) { e->sx[e->nslot] = (int16_t)sx; e->sy[e->nslot++] = (int16_t)sy; }
    }
}
