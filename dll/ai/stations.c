// THE FORCE BY TERRITORY - who stands where, played while the game's computer player is off ([ai_base] game_ai = 0);
// the one force of armies.c does not run then. The player, 2026-11-14: "the single main force isnt the way anymore",
// then the territories' shares (produce.c SHARES: "100% is the full military force and the terriotories distribute
// that under each other"), then the pools: "we should also split the force into subcategories that are requested and
// asigned semiindependently, 1. main-force: unit vs unit 2. siege: building killers, distributed betwen territoris
// planing attacks ... 3. anti air ... 4, explorers a few fast moving units that scout unexplored lland", explorers
// "a temporary role before they are disbaned into the main army share again", and of the attack: "make it push on
// its own, no more attack timers, either its a good idea to attack or its not".
// Every SN_EVERY per AI, on the territory view and produce.c's shares:
//   STATIONS  - every land soldier of its has a station: a territory (where it stands, when it is new). THE MAIN
//               FORCE (soldiers that are no siege, ram or anti-air): a territory's share of them in soldiers; a
//               share worth fewer than SN_MIN goes to the neighbour with the largest (single soldiers hold nothing)
//               unless it is under attack. SIEGE (siege weapons, rams): among the territories planning an attack
//               (beside enemy buildings it knows, by their number) that have a share of the main force - never
//               alone; with no attack planned they stay where they are.
//               A share holds until the new one is off by a quarter, 2 at least (STEADY).
//   GIVING    - a territory with fewer stationed than its share (by a quarter, 2 at least, or by any under attack - those first)
//               is given soldiers of the nearest territories with more than theirs, territory by territory over
//               land: their station changes, they march. SN_MOVES a look.
//   ORDERS    - a soldier away from its station's place goes there (attack-move: it fights what it meets): the
//               middle of the enemy soldiers seen in the territory when there are any (an attack: its share is
//               there to meet it), else a Town Center, Capitol, Settlement or other building of its there, else
//               the territory's middle. Asked again when it stands idle elsewhere.
//   A PUSH    - no attack time: a territory it holds with SN_PUSH_MIN of the main force stationed, three quarters
//               of them there, beside a territory with enemy buildings: its soldiers and siege against what it
//               knows of the enemy at the nearest of those buildings (stage.c stage_foe: soldiers seen, defences,
//               the remembered) - from SN_PUSH they go, attack-move at the building, on to the next when it is
//               down; under SN_BACK they come back to their station and SN_PUSH_WAIT passes before the next look.
//               No building left there: the push is over, the shares say where they stand next.
//   SCOUTS    - while it has land territories no unit of its ever stood in (on the land the scout stands on) up
//               to SN_SCOUTS of the main force, the fastest (of equally fast ones the one that sees furthest), go to the nearest of them one after the other (one
//               not reached in SN_SCOUT_GIVE_UP is given up); none left: they are the main force's again,
//               stationed where they stand, and none is taken for SN_SCOUT_REST.
//   A SCENARIO'S AI TOGGLES (the player 2026-11-16: "respecting the ai flags (aktiv, cheat, angreifen, etc)"; the
//               editor's player dialog and the trigger Set AI Toggle, player_ai_toggle): Attack off - no push, the
//               ones on are over; Explore off - no scouts, the ones out the main force's again. Active off: the game
//               calls no update of its at all (nor the mod's); Build/Repair and Gather: eeaimod.c rm_tick.
//   DEMOLITION - a territory it has taken (its own, no enemy building but walls left, no enemy soldier there, no push
//               from it) has the enemy's wall pieces and gates in it brought down, a low task: SN_DEMO of its idle
//               main force and siege stationed there at the piece nearest its place, one after the other (the player
//               2026-11-15: "after taking a territory the ai should "deconstruct" enemy walls in it as a low priorethy
//               task"). A fight there or a push from there takes them first.
// Not here yet: anti-air, aircraft and ships by role (the player: "do it by role"), siege on a shoreline.
// "# stations:" a minute.
#define SN_EVERY 5000
#define SN_UNITS 1024
#define SN_MIN 3
#define SN_BAND 1 / 4             // STEADY: a share moves, a territory is short, from this much of it
#define SN_BAND_MIN 2            // ... soldiers at least
#define SN_MOVES 16
#define SN_NEAR 10
#define SN_REORDER 30000
#define SN_PUSH_MIN 6
#define SN_GIVE_STEPS 3          // territories a soldier is sent at most
#define SN_GIVE_STEPS_ATTACK 5   // ... to one under attack
#define SN_GROUP 4               // AS A GROUP: soldiers that march together at least
#define SN_GROUP_WAIT 45000      // ... or the first of them has waited this long
#define SN_GROUPS 64
#define SN_DEMO 3                // DEMOLITION: soldiers a territory sets at the enemy's walls in it at most
#define SN_STAY 180000           // a soldier given a station keeps it this long, but for an attack
#define SN_GROUP_MAX 160
#define SN_AA_BLD 20
#define SN_AA_SEEN 30
#define SN_PUSH 1.5f
#define SN_BACK 0.8f
#define SN_PUSH_WAIT 120000
#define SN_CALL 240000           // THE CALL: how long the called gather before another call
#define SN_PUSHES 6
#define SN_SCOUTS 3
#define SN_SCOUT_GIVE_UP 180000
#define SN_SCOUT_REST 300000
#define SN_EB 512
enum { SNP_MAIN, SNP_SIEGE, SNP_AA, SNP_N };
struct sn_unit { ee_unit *u; int id; short ter; uint8_t pool, scout; uint32_t order_t, since, wait_t, moved_t, stash_t; ivec2 to; };   // stash_t: sent into a fortress then (THE RESERVE)   // wait_t: it waits for a group to march with since then (AS A GROUP)   // since: a scout's territory is its goal since then
struct sn_push { short from, into; int on; uint32_t end_t; ivec2 at; uint32_t way_t; int breach, round_, straight; ivec2 wall; uint32_t near_t, force_t, wait, hold_t; int near_d; ivec2 near_at; };   // hold_t: THE CLEARED GROUND is held since then   // STUCK: near_d the nearest its middle came to near_at (squared), at near_t; force_t: a breach THE WAY did not find, since then; wait: before the next push from there (0: SN_PUSH_WAIT)   // THE WAY: looked at way_t; breach: a wall to bring down at `wall`; the walk round and through, in cells
internal struct sn_st {
    uint32_t t, log_t, scout_off;   // scout_off: no scouts before then
    int n;
    struct sn_unit m[SN_UNITS];
    struct sn_push push[SN_PUSHES];
    short tc[PR_T];   // STEADY: the territories' shares in soldiers as of the last look
    uint32_t res_t, res_out_t; unsigned res_in, res_out; ee_unit *res_last; int res_last_n;   // THE RESERVE: its last look, the last fortress let out; soldiers in, out
    unsigned moved, orders, pushes, won, lost, scouted, breaches, groups, grouped, demos;   // demos: soldiers set at an enemy wall (DEMOLITION)   // groups: orders to several at once, grouped: the soldiers in them
    int need_on; ivec2 need_at;   // where the force meets the enemy next (sn_need_at)
    int call_r; uint32_t call_t; unsigned calls;   // THE CALL: the territory the others' soldiers gather in for a push, since then; calls so far
} g_sn[16];
internal uint8_t g_sn_seen[16][PR_T];
internal struct st_side g_sn_own;
// TRAINING BY NEED's place with the game's computer player off (armies.c ar_need_pick: what to train is what adds most
// against the enemy there): a push's building, else the middle of the enemy soldiers in the territory of its with the
// most of them, else the enemy building it knows nearest the territory with the largest share. 0: none known.
internal int sn_need_at(unsigned idx, ivec2 *at) { *at = g_sn[idx & 15].need_at; return g_sn[idx & 15].need_on; }
// THE WAY (the player 2026-11-15: "ai attacks dont cope well with walls yet. the attack should actual try to pathfind to
// the target. maybe 2 paths? on with walls and one ignoring them, if the path with wall is a larger detour the ai bring
// siege to destroy walls"): from a push's soldiers to its building two ways over the cells of armies.c's route search -
// one as if no enemy wall stood, one with every cell of a wall or gate it has seen shut. No wall on the first: nothing
// to do. The second no more than SN_ROUND times the first (and SN_ROUND_MORE cells): the walk round, which the game's
// own pathfinding takes by itself. Longer, or none: A BREACH - the first wall on the way through is what the push
// goes at: its siege (the soldiers too when it has none) attacks the piece, the rest stands by it; looked at again
// every SN_WAY_EVERY, so the next piece follows and the push goes on once the way is open.
// 1 a breach (*wall the piece's tile), 0 none; *round_, *straight the two ways in cells (0 none)
#define SN_ROUND 2
#define SN_ROUND_MORE 10
#define SN_WAY_EVERY 15000
#define SN_RES_EVERY 10000      // THE RESERVE: a look
#define SN_RES_ROOM 5           // ... soldiers go in with this many places left on the map
#define SN_RES_BATCH 20         // ... this many a look
#define SN_RES_FOE 4            // ... or with this many enemy soldiers in a territory of its
#define SN_RES_AGAIN 60000      // ... and nobody goes in for this long after
#define SN_HOLD 600000          // THE CLEARED GROUND: held this long at most
#define SN_STUCK 120000        // STUCK: no nearer to its building for this long
#define SN_STUCK_AT 14          // ... and further from it than this (tiles): at it, it fights
#define SN_STUCK_WALL 12        // the enemy wall to breach then: within this of its soldiers
#define SN_STUCK_WAIT 600000    // none: before that territory pushes again
internal int sn_way(ee_player *pl, unsigned idx, ivec2 from, ivec2 to, ivec2 *wall, int *round_, int *straight, uint32_t now) {
    static int path[65536];
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0, d = 0, w = 0;
    *round_ = *straight = 0; (void)pl;
    int n = size > 0 ? ar_route_cells(idx, size, (unsigned short)map_area(from, 2), &g_armies[idx], now) : 0;
    if (!n) return 0;
    g_arr_walls = 2;
    int len = ar_route_find(n, from, to, path, 65536, &d, &w), first = -1;
    for (int j = len - 1; j >= 0 && first < 0; j--) if (g_arr.wall[path[j]]) first = path[j];   // from the start: path runs from the building back
    *straight = len;
    if (len && first >= 0) { g_arr_walls = 1; *round_ = ar_route_find(n, from, to, path, 65536, &d, &w); }
    g_arr_walls = 0;
    if (g_log && g_autotest) {   // tests: every look
        int wc = 0;
        for (int c = 0; c < n * n; c++) wc += g_arr.wall[c];
        log_locked("# way: P%u at %.1f min: %d,%d -> %d,%d: the way through %d cells%s, the walk round %d; wall cells it knows %d\n", idx, now / 60000.0, from.x, from.y, to.x, to.y, len, first >= 0 ? " over a wall" : "", *round_, wc);
    }
    if (!len || first < 0 || (*round_ && *round_ <= len * SN_ROUND + SN_ROUND_MORE)) return 0;
    ivec2 c = iv2((first % n) * AR_CELL + AR_CELL / 2, (first / n) * AR_CELL + AR_CELL / 2);
    *wall = c;
    return 1;
}
// the enemy wall piece or gate it has seen nearest p, within 4 tiles
internal ee_unit *sn_wall_near(ee_player *pl, ivec2 p, int reach) {
    ee_unit *best = NULL;
    int bd = 0;
    for (struct grid_it it = grid_it_box(p.x, p.y, reach, 1); grid_it_next(&it); ) {
        ee_player *o = unit_get_owner(it.u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || !(is_wall(unit_type_get_id(it.td)) || unit_type_get_class_id(it.td) == CLS_GATE) || !explored_by(it.u, pl)) continue;
        int d = ivec2_dist2(unit_get_tile(it.u), p);
        if (!best || d < bd) { best = it.u; bd = d; }
    }
    return best;
}
internal ee_unit *sn_wall_at(ee_player *pl, ivec2 p) { return sn_wall_near(pl, p, 4); }
internal int sn_cmp(const void *a, const void *b) { const struct sn_unit *x = a, *y = b; return x->u < y->u ? -1 : x->u > y->u; }
internal int sn_order(struct sn_st *z, struct sn_unit *m, int k, ivec2 d, int fight, uint32_t now) {
    ee_goal *g = ar_goal_at(d, k, fight);
    if (!g || !unit_goal(m->u, g, 0, 0x1F40)) return 0;
    m->order_t = now ? now : 1; m->to = d;
    z->orders++;
    return 1;
}
// AS A GROUP (the player 2026-11-15: "also armies should properly form/muster in territories and move in a group in
// formation attack move"): the soldiers that are to go from one territory to the same place get one order - the game's
// own move on a selection of them, as a click of the player's would be (armies.c ar_group_move's recipe), together
// (the group's speed: the fast wait for the slow, the mod's formations lay them out) and as an attack-move unless the
// siege moves alone. Fewer than SN_GROUP of them wait where they stand for more, SN_GROUP_WAIT at most - a push and
// an attack on their territory wait for nobody.
internal int sn_group_move(ee_player *pl, uint32_t *ids, int n, ivec2 from, ivec2 d, int fight) {
    static __attribute__((aligned(8))) uint8_t mem[0x40];
    if (n <= 0) return 0;
    uint32_t *sel[3] = { NULL, ids, ids + n };
    player_select(pl, sel);
    memset(mem, 0, sizeof mem);
    float x = (float)d.x + 0.5f, y = (float)d.y + 0.5f;
    path_msg_make((ee_path_msg *)mem, msg_as_begin(pl), x, y, 1, 0);
    path_msg_set_forced((ee_path_msg *)mem, !fight);
    path_msg_set_facing((ee_path_msg *)mem, atan2f(-(x - (float)from.x), y - (float)from.y));
    msg_execute((ee_msg *)mem);
    msg_as_end(pl);
    return n;
}
// the stationed of a pool counted by territory into ac; then the territories short of tc are given those of the
// nearest with more: 1 where anything moved
internal int sn_give(struct sn_st *z, int pool, int n, const int *tc, int *ac, const struct tv_st *s, int *budget) {
    static uint8_t vis[PR_T], done[PR_T], dep[PR_T];
    static short q[PR_T];
    int any = 0;
    uint32_t tnow = game_time();
    memset(ac, 0, (size_t)n * sizeof *ac);
    for (int i = 0; i < z->n; i++) if (z->m[i].pool == pool && !z->m[i].scout && z->m[i].ter >= 0 && z->m[i].ter < n) ac[z->m[i].ter]++;
    memset(done, 0, (size_t)n);
    while (*budget > 0) {
        int r = -1, best = 0;
        for (int k = 0; k < n; k++) {
            int df = tc[k] - ac[k], hard = s->t[k].foe > 0 && s->t[k].role != TVR_NONE;
            if (done[k] || df <= 0 || (!hard && df < (tc[k] * SN_BAND > SN_BAND_MIN ? tc[k] * SN_BAND : SN_BAND_MIN))) continue;
            int pr = df + (hard ? 100000 : 0);
            if (pr > best) { best = pr; r = k; }
        }
        if (r < 0) break;
        done[r] = 1;
        memset(vis, 0, (size_t)n);
        int qh = 0, qt = 0;
        q[qt++] = (short)r; vis[r] = 1; dep[r] = 0;
        int hard_r = s->t[r].foe > 0 && s->t[r].role != TVR_NONE, far_ = hard_r ? SN_GIVE_STEPS_ATTACK : SN_GIVE_STEPS;
        while (qh < qt && tc[r] > ac[r] && *budget > 0) {
            int o = q[qh++];
            // BY STEPS (the player 2026-11-15: "units still get send over half the map, they should do a simple pathfind how
            // many territores they are away, with incresing cost, neasrt first, very far might not be worth the move at all
            // and its better to produce locally"): the nearest territory with more than its share first, as before; one
            // step further keeps one more of its surplus for each step beyond the first, and beyond SN_GIVE_STEPS
            // territories (SN_GIVE_STEPS_ATTACK for one under attack) nobody is sent - what stays missing there is
            // produce.c's claim: it builds its own production.
            int give = o != r ? ac[o] - tc[o] - (hard_r ? 0 : dep[o] - 1) : 0;
            for (int i = 0; i < z->n && give > 0 && tc[r] > ac[r] && *budget > 0; i++) {
                struct sn_unit *m = &z->m[i];
                if (m->pool != pool || m->scout || m->ter != o) continue;
                if (!hard_r && m->moved_t && tnow >= m->moved_t && tnow - m->moved_t < SN_STAY) continue;   // it has just been sent somewhere: it stays (the shares shift with every look - 400 to 680 changes of station a player in 75 minutes)
                int pushing = 0;
                for (int p = 0; p < SN_PUSHES; p++) pushing |= z->push[p].on && z->push[p].from == o;
                pushing |= z->call_t && z->call_r == o && tnow >= z->call_t && tnow - z->call_t < SN_CALL;   // THE CALL: gathering for one
                if (pushing) break;   // a territory that is pushing gives none
                m->ter = (short)r; m->order_t = 0; m->moved_t = tnow ? tnow : 1;
                ac[o]--; ac[r]++; give--; (*budget)--; z->moved++; any = 1;
            }
            for (int k = 0; k < g_ter.t[o].nbn; k++) {
                unsigned w = g_ter.nb[g_ter.t[o].nb0 + k].to;
                if (w < (unsigned)n && !vis[w] && !g_ter.t[w].water && qt < PR_T && dep[o] < far_) { vis[w] = 1; dep[w] = (uint8_t)(dep[o] + 1); q[qt++] = (short)w; }
            }
        }
    }
    return any;
}
// build.c's claim: a push of its holds the territory it cleared
internal int sn_holding(unsigned idx, int r, ivec2 *at) {   // *at: where its soldiers stand
    for (int k = 0; k < SN_PUSHES; k++) if (g_sn[idx & 15].push[k].on && g_sn[idx & 15].push[k].hold_t && g_sn[idx & 15].push[k].into == r) { *at = g_sn[idx & 15].push[k].at; return 1; }
    return 0;
}
internal void stations_tick(ee_player *pl, uint32_t now) {
    static struct sn_unit old[SN_UNITS];
    static int tc[PR_T], ac[PR_T], tcs[PR_T], acs[PR_T], fx[PR_T], fy[PR_T], fn[PR_T];
    static ivec2 rally[PR_T];
    static uint8_t anc[PR_T];
    static struct { ivec2 p; short ter; } eb[SN_EB];
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct sn_st *z = &g_sn[idx];
    struct tv_st *s = &g_tv[idx];
    if (now < z->t) { memset(z, 0, sizeof *z); memset(g_sn_seen[idx], 0, sizeof g_sn_seen[idx]); }
    if (z->t && now - z->t < SN_EVERY) return;
    z->t = now ? now : 1;
    int n = s->n < PR_T ? s->n : PR_T;
    if (!n || s->version != g_ter.version) return;
    // its units: the soldiers with their stations, a place to stand in each territory, the territories it has stood in
    int on = z->n, nn = 0, count = 0, neb = 0;
    memcpy(old, z->m, (size_t)on * sizeof *old);
    memset(anc, 0, (size_t)n);
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
        int t = unit_type_get_id(td), fam = unit_type_get_class_id(td);
        ivec2 p = unit_get_tile(u);
        unsigned r = ter_near(p.x, p.y);
        if (r >= (unsigned)n) continue;
        g_sn_seen[idx][r] = 1;
        if (fam == CLS_BUILDING || fam == CLS_FORTRESS) {
            ee_unit *st = unit_get_physics(u);
            int town = is_wall(t) ? 0 : t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL) ? 3 : t == UT(B_SETTLEMENT) ? 2 : 1;
            if (st && unit_get_finished(st) && town > anc[r]) { anc[r] = (uint8_t)town; rally[r] = p; }
            continue;
        }
        // the class Land only: a wall piece is of the army's families too (the first look: 252 "soldiers" with 131 there, the walls stationed with them)
        int pool = fam != CLS_LAND ? -1 : is_aa(u) ? SNP_AA : is_siege(t) || is_ram(t) ? SNP_SIEGE : land_soldier(t) ? SNP_MAIN : -1;
        if (pool < 0 || nn >= SN_UNITS) continue;
        struct sn_unit key = { .u = u }, *o = on ? bsearch(&key, old, (size_t)on, sizeof *old, sn_cmp) : NULL;
        struct sn_unit *m = &z->m[nn++];
        if (o && o->id == unit_get_id(u)) *m = *o;
        else { memset(m, 0, sizeof *m); m->u = u; m->id = unit_get_id(u); m->ter = (short)r; }
        m->pool = (uint8_t)pool;
        if (!m->scout && (m->ter < 0 || m->ter >= n || g_ter.t[m->ter].water)) m->ter = (short)r;
    }
    z->n = nn;
    qsort(z->m, (size_t)nn, sizeof *z->m, sn_cmp);
    for (int r = 0; r < n; r++) if (!anc[r]) rally[r] = g_ter.t[r].c;
    // the enemy it knows: the soldiers it sees by territory (their middle), the buildings to push at
    memset(fn, 0, (size_t)n * sizeof *fn); memset(fx, 0, (size_t)n * sizeof *fx); memset(fy, 0, (size_t)n * sizeof *fy);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_player *o = unit_get_owner(u);
        if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || !cv_alive(u) || !seen_by(u, pl)) continue;
        ivec2 p = unit_get_tile(u);
        unsigned r = ter_near(p.x, p.y);
        if (r >= (unsigned)n) continue;
        if (fr_building(it.td)) { if (!is_wall(unit_type_get_id(it.td)) && neb < SN_EB) { eb[neb].p = p; eb[neb++].ter = (short)r; } }
        else if (ms_land_soldier(u, it.td)) { fx[r] += p.x; fy[r] += p.y; fn[r]++; }
    }
    // STATIONS: the shares in soldiers
    int np[SNP_N] = { 0, 0, 0 }, scouts = 0, wsum = 0;
    for (int i = 0; i < nn; i++) { if (z->m[i].scout) scouts++; else np[z->m[i].pool]++; }
    // STEADY (the player 2026-11-15, on the soldiers reshuffled with every look: "yes, continue with steading the
    // shares"): a territory's share in soldiers stays what it was until the new one is off by SN_BAND of it (SN_BAND_MIN
    // soldiers at least) - a soldier trained or lost, a citizen more in a front's guard moved every share by one and a
    // soldier with it, there and back (420 to 860 changes of station a player in 75 minutes). Not one under attack, not
    // one that loses its share altogether: those at once.
    for (int r = 0; r < n; r++) {
        int v = (g_pr_share[idx][r] * np[SNP_MAIN] + 500) / 1000, was = z->tc[r], band = was * SN_BAND > SN_BAND_MIN ? was * SN_BAND : SN_BAND_MIN;
        if (v && was && abs(v - was) < band && !(s->t[r].foe > 0 && s->t[r].role != TVR_NONE)) v = was;
        tc[r] = v; z->tc[r] = (short)(v > 32767 ? 32767 : v);
    }
    for (int r = 0; r < n; r++) {
        if (tc[r] <= 0 || tc[r] >= SN_MIN || (s->t[r].foe > 0 && s->t[r].role != TVR_NONE)) continue;
        int b = -1;
        for (int k = 0; k < g_ter.t[r].nbn; k++) {
            unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
            if (o < (unsigned)n && !g_ter.t[o].water && tc[o] >= SN_MIN && (b < 0 || tc[o] > tc[b])) b = (int)o;
        }
        if (b >= 0) { tc[b] += tc[r]; tc[r] = 0; }
    }
    for (int r = 0; r < n; r++) {   // siege: the territories planning an attack that have a share of the main force
        int a = 0;
        tcs[r] = 0;
        if (g_ter.t[r].water || s->t[r].role == TVR_NONE || tc[r] <= 0) continue;
        for (int k = 0; k < g_ter.t[r].nbn; k++) {
            unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
            if (o < (unsigned)n && !g_ter.t[o].water && s->t[o].ebld && PR_ATTACK + s->t[o].ebld > a) a = PR_ATTACK + s->t[o].ebld;
        }
        tcs[r] = a; wsum += a;
    }
    int budget = SN_MOVES;
    sn_give(z, SNP_MAIN, n, tc, ac, s, &budget);
    if (wsum > 0) {
        for (int r = 0; r < n; r++) tcs[r] = (tcs[r] * np[SNP_SIEGE] + wsum / 2) / wsum;
        sn_give(z, SNP_SIEGE, n, tcs, acs, s, &budget);
    }
    // ANTI-AIR (the player 2026-11-14: "3. anti air, pure devensive role, might also get a small share in the
    // backlands"): its anti-air units among the territories it holds by what stands there to protect - its buildings
    // there (SN_AA_BLD at most; a backland's count half) and SN_AA_SEEN more where enemy aircraft were last seen near its
    // towns (defence.c g_aa_st, the last 5 minutes). They go to their station and stay: no push takes them along.
    {
        static int tca[PR_T], aca[PR_T];
        int asum = 0, at = -1;
        if (g_aa_st[idx].seen_t && now >= g_aa_st[idx].seen_t && now - g_aa_st[idx].seen_t < 300000) at = (int)ter_near(g_aa_st[idx].ax, g_aa_st[idx].ay);
        for (int r = 0; r < n; r++) {
            int w = 0;
            if (!g_ter.t[r].water && s->t[r].role != TVR_NONE && s->t[r].state != TV_UNSAFE) {
                w = s->t[r].bld < SN_AA_BLD ? s->t[r].bld : SN_AA_BLD;
                if (s->t[r].role == TVR_BACK) w = (w + 1) / 2;
                if (r == at) w += SN_AA_SEEN;
            }
            tca[r] = w; asum += w;
        }
        if (asum > 0 && np[SNP_AA]) {
            for (int r = 0; r < n; r++) tca[r] = (tca[r] * np[SNP_AA] + asum / 2) / asum;
            sn_give(z, SNP_AA, n, tca, aca, s, &budget);
        }
    }
    // A PUSH: those on, then a new one
    for (int k = 0; k < SN_PUSHES; k++) {
        struct sn_push *p = &z->push[k];
        if (!p->on) continue;
        long sx = 0, sy = 0;
        int cn = 0, bi = -1, bd = 0;
        st_side_clear(&g_sn_own);
        for (int i = 0; i < nn; i++) {
            if (z->m[i].ter != p->from || z->m[i].scout || z->m[i].pool == SNP_AA) continue;
            ivec2 q = unit_get_tile(z->m[i].u);
            sx += q.x; sy += q.y; cn++;
            st_side_add(&g_sn_own, z->m[i].u);
        }
        ivec2 mid = cn ? iv2((int)(sx / cn), (int)(sy / cn)) : p->at;
        ivec2 lead = mid;   // THE WAY's start: the soldier nearest their middle (the middle itself can lie in a wood or on water)
        for (int i = 0, ld = 0x7FFFFFFF; i < nn; i++) {
            if (z->m[i].ter != p->from || z->m[i].scout) continue;
            ivec2 q = unit_get_tile(z->m[i].u);
            if (ivec2_dist2(q, mid) < ld) { ld = ivec2_dist2(q, mid); lead = q; }
        }
        for (int e = 0; e < neb; e++) {
            if (eb[e].ter != p->into) continue;
            int d = ivec2_dist2(eb[e].p, mid);
            if (bi < 0 || d < bd) { bi = e; bd = d; }
        }
        const char *end = NULL;
        float ratio = 0;
        if (!cn) end = "nobody left";
        else if (bi < 0) {
            // THE CLEARED GROUND (the player 2026-11-15: "the claiming proccess needs some work, right now the army comes,
            // clears evrething and than leaves. leaving the citezens alone to build their tower"): with no building left
            // there the push is won, not over - its soldiers stand where its last building stood until a
            // building of its holds it (territories.c TV_HOLDS: build.c's claim lays the tower there first, at once),
            // SN_HOLD at most, or until they are losing against what comes.
            if (!p->hold_t) {
                p->hold_t = now ? now : 1; z->won++;
                p->breach = 0;   // where the last building stood: they are there (the territory's middle may lie in a wood, behind a wall - 2 of the first 3 holds ended "stuck" on the way to it)
                for (int i = 0; i < nn; i++) if (z->m[i].ter == p->from) z->m[i].order_t = 0;
                if (g_log) log_locked("# stations: P%u at %.1f min: the push from territory %d into %d has cleared it - %d soldiers hold it until a building of its stands there\n", idx, now / 60000.0, p->from, p->into, cn);
            }
            stage_foe(pl, p->at);
            ratio = st_ratio(&g_sn_own, &g_sgs_foe);
            if (TV_HOLDS(idx, &s->t[p->into])) end = "claimed: a building of its holds the territory";
            else if (now < p->hold_t || now - p->hold_t >= SN_HOLD) end = "cleared, and not claimed in time";
            else if (ratio < SN_BACK) end = "losing the ground it cleared";
        }
        else {
            stage_foe(pl, eb[bi].p);
            ratio = st_ratio(&g_sn_own, &g_sgs_foe);
            if (ratio < SN_BACK) { end = "losing"; z->lost++; }
            else { p->at = eb[bi].p; p->hold_t = 0; }   // TO LOOK: one seen while it held the ground - the push goes on
        }
        // STUCK (the player 2026-11-15, his game's P2 from minute 183 to 222: 137 soldiers packed against a wall, "red is
        // stuck on an attack that never ends" - the cells of the route search are coarser than a wall is thick, the way
        // was "open" by a walk round that did not exist): a push whose soldiers have come no nearer to its building
        // for SN_STUCK - by a tenth of the way, and not standing at it - goes at the enemy wall or gate it has seen
        // nearest them (SN_STUCK_WALL tiles), THE WAY's breach without asking the route; with none there the push is
        // over, and SN_STUCK_WAIT passes before that territory pushes again.
        if (!end && !p->hold_t) {
            int d = ivec2_dist2(mid, p->at);
            if (!p->near_t || now < p->near_t || p->near_at.x != p->at.x || p->near_at.y != p->at.y || d * 10 < p->near_d * 8) { p->near_t = now ? now : 1; p->near_d = d; p->near_at = p->at; }
            else if (now - p->near_t >= SN_STUCK && d > SN_STUCK_AT * SN_STUCK_AT) {
                ee_unit *piece = sn_wall_near(pl, lead, SN_STUCK_WALL);
                p->near_t = now ? now : 1;
                if (piece) {
                    p->breach = 1; p->wall = unit_get_tile(piece); p->force_t = now ? now : 1; p->way_t = now ? now : 1;
                    z->breaches++;
                    for (int i = 0; i < nn; i++) if (z->m[i].ter == p->from) z->m[i].order_t = 0;
                    if (g_log) log_locked("# stations: P%u at %.1f min: the push from territory %d into %d is stuck (%d tiles from its building for %d s) - a breach at the wall at %d,%d\n",
                                          idx, now / 60000.0, p->from, p->into, (int)sqrtf((float)d), SN_STUCK / 1000, p->wall.x, p->wall.y);
                } else { end = "stuck, no way there"; z->lost++; p->wait = SN_STUCK_WAIT; }
            }
        }
        if (!end && p->force_t && now >= p->force_t && now - p->force_t < SN_STUCK && sn_wall_at(pl, p->wall)) {   // the forced breach holds while a piece stands there
            ee_unit *piece = sn_wall_at(pl, p->wall);
            ivec2 w = unit_get_tile(piece);
            if (w.x != p->wall.x || w.y != p->wall.y) { p->wall = w; for (int i = 0; i < nn; i++) if (z->m[i].ter == p->from) z->m[i].order_t = 0; }
            p->way_t = now ? now : 1;
        }
        if (!end && (!p->way_t || now < p->way_t || now - p->way_t >= SN_WAY_EVERY)) {   // THE WAY
            int was = p->breach;
            ivec2 wall = p->wall;
            p->way_t = now ? now : 1;
            p->breach = sn_way(pl, idx, lead, p->at, &wall, &p->round_, &p->straight, now);
            ee_unit *piece = p->breach ? sn_wall_at(pl, wall) : NULL;
            if (!piece) p->breach = 0; else wall = unit_get_tile(piece);
            if (p->breach != was || (p->breach && (wall.x != p->wall.x || wall.y != p->wall.y))) {
                for (int i = 0; i < nn; i++) if (z->m[i].ter == p->from) z->m[i].order_t = 0;
                if (p->breach) z->breaches++;
                if (g_log) log_locked("# stations: P%u at %.1f min: the push from territory %d into %d - %s (the way through %d cells, the walk round %d)\n", idx, now / 60000.0, p->from, p->into,
                                      p->breach ? "a wall in the way: a breach" : "the way is open", p->straight, p->round_);
            }
            p->wall = wall;
        }
        if (!end && !player_ai_toggle(pl, SM_AIT_ATTACK)) end = "the scenario's AI toggle Attack is off";
        if (!end) continue;
        p->on = 0; p->end_t = now ? now : 1;
        for (int i = 0; i < nn; i++) if (z->m[i].ter == p->from) z->m[i].order_t = 0;   // back to their station
        if (g_log) log_locked("# stations: P%u at %.1f min: the push from territory %d into %d is over - %s (%d soldiers, judged %.2f)\n", idx, now / 60000.0, p->from, p->into, end, cn, (double)ratio);
    }
    for (int r = 0; r < n; r++) {
        struct tv_terr *t = &s->t[r];
        if (g_ter.t[r].water || t->role == TVR_NONE || t->state != TV_QUIET || ac[r] < SN_PUSH_MIN || !player_ai_toggle(pl, SM_AIT_ATTACK)) continue;
        int slot = -1, busy = 0, into = -1, there = 0, cn = 0, bi = -1, bd = 0;
        for (int k = 0; k < SN_PUSHES; k++) {
            struct sn_push *p = &z->push[k];
            if (p->on && p->from == r) busy = 1;
            if (!p->on && p->from == r && p->end_t && now >= p->end_t && now - p->end_t < (p->wait ? p->wait : SN_PUSH_WAIT)) busy = 1;
            if (!p->on && (slot < 0 || p->end_t < z->push[slot].end_t)) slot = k;   // the one that ended longest ago
        }
        if (busy || slot < 0) continue;
        for (int k = 0; k < g_ter.t[r].nbn; k++) {
            unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
            if (o < (unsigned)n && !g_ter.t[o].water && s->t[o].ebld && (into < 0 || s->t[o].ebld > s->t[into].ebld)) into = (int)o;
        }
        if (into < 0) continue;
        for (int e = 0; e < neb; e++) {
            if (eb[e].ter != into) continue;
            int d = ivec2_dist2(eb[e].p, rally[r]);
            if (bi < 0 || d < bd) { bi = e; bd = d; }
        }
        // TO LOOK (the player's game of 2026-11-16, "aistuck": yellow's push into red's territory 7 ended "cleared" at
        // minute 82 with a production building of red's still there - built out of its sight on ground it had explored,
        // which the view counts (territories.c ebld) and a push does not know; no push started again in 20 minutes):
        // an enemy territory with no building it sees or remembers is pushed into all the same, at its middle - there
        // its soldiers see what stands, and the push goes on at that, or holds the ground as cleared.
        ivec2 aim = bi >= 0 ? eb[bi].p : g_ter.t[into].c;
        st_side_clear(&g_sn_own);
        for (int i = 0; i < nn; i++) {
            if (z->m[i].ter != r || z->m[i].scout || z->m[i].pool == SNP_AA) continue;
            ivec2 q = unit_get_tile(z->m[i].u);
            cn++;
            there += ter_near(q.x, q.y) == (unsigned)r || ivec2_dist2(q, rally[r]) <= 15 * 15;
            st_side_add(&g_sn_own, z->m[i].u);
        }
        if (there * 4 < cn * 3) continue;   // still gathering
        stage_foe(pl, aim);
        float ratio = st_ratio(&g_sn_own, &g_sgs_foe);
        // THE CALL (the player 2026-11-16: "yellow just sits there, it has enough soilders to finish red or atleast take
        // the food source it desperratly needs, instead it seems completly dead" - 53 soldiers over 4 territories, 9 to 16
        // each, its enemy beside one of them with 2 soldiers and its buildings; a push weighs only the territory's own,
        // and none started for 25 minutes): too few there, the main force of its quiet territories - no enemy soldiers
        // there, none pushing, on the same land - is weighed with them, each keeping half its share. Enough together:
        // they are stationed in this territory (it gives none for SN_CALL meanwhile) and the push starts when three
        // quarters of them are there and it is still judged to win. One call at a time.
        if (ratio < SN_PUSH) {
            if (z->call_t && now >= z->call_t && now - z->call_t < SN_CALL) continue;
            static short left[PR_T];
            static int joiner[SN_UNITS];
            unsigned land = area_id(rally[r], 2);
            for (int o = 0; o < n; o++) {
                int po = 0;
                for (int k = 0; k < SN_PUSHES; k++) po |= z->push[k].on && z->push[k].from == o;
                left[o] = (short)(o == r || po || g_ter.t[o].water || s->t[o].foe > 0 || s->t[o].state != TV_QUIET ? 0 : ac[o] - (z->tc[o] + 1) / 2);
            }
            int nj = 0;
            for (int i = 0; i < nn; i++) {
                struct sn_unit *m = &z->m[i];
                if (m->pool != SNP_MAIN || m->scout || m->ter < 0 || m->ter >= n || left[m->ter] <= 0) continue;
                if (land && area_id(unit_get_tile(m->u), 2) != land) continue;
                left[m->ter]--; joiner[nj++] = i;
                st_side_add(&g_sn_own, m->u);
            }
            float all = nj ? st_ratio(&g_sn_own, &g_sgs_foe) : ratio;
            if (all < SN_PUSH) continue;
            for (int j = 0; j < nj; j++) { struct sn_unit *m = &z->m[joiner[j]]; ac[m->ter]--; m->ter = (short)r; m->order_t = 0; m->moved_t = now ? now : 1; ac[r]++; z->moved++; }
            z->call_r = r; z->call_t = now ? now : 1; z->calls++;
            if (g_log) log_locked("# stations: P%u at %.1f min: territory %d calls %d soldiers from its quiet territories to push into %d (the building at %d,%d) - %d of its own judged %.2f, together %.2f (enemy there: %d)\n",
                                  idx, now / 60000.0, r, nj, into, aim.x, aim.y, cn, (double)ratio, (double)all, g_sgs_foe.n);
            continue;
        }
        struct sn_push *p = &z->push[slot];
        p->on = 1; p->from = (short)r; p->into = (short)into; p->at = aim; p->end_t = 0; p->way_t = 0; p->breach = 0; p->near_t = p->force_t = p->wait = p->hold_t = 0;
        z->pushes++;
        if (g_log) log_locked("# stations: P%u at %.1f min: territory %d pushes into %d - %d soldiers and siege at the building at %d,%d, judged %.2f (enemy there: %d)\n",
                              idx, now / 60000.0, r, into, cn, p->at.x, p->at.y, (double)ratio, g_sgs_foe.n);
    }
    // SCOUTS
    int unseen = 0;
    for (int r = 0; r < n; r++) unseen += !g_ter.t[r].water && !g_sn_seen[idx][r];
    int want_sc = unseen && np[SNP_MAIN] + scouts >= 3 && now >= z->scout_off && player_ai_toggle(pl, SM_AIT_EXPLORE) ? 1 + unseen / 8 : 0;
    if (want_sc > SN_SCOUTS) want_sc = SN_SCOUTS;
    while (scouts < want_sc) {
        int b = -1; float bf = -1.0f;
        for (int i = 0; i < nn; i++) {
            struct sn_unit *m = &z->m[i];
            // the fastest, among those the one that sees furthest (the player 2026-11-15: "samson is probbably the worst
            // chioce for a scout possible? they are slow and have low view range" - the pick went by armies.c's raid
            // families, 0 for everything on foot, so the first soldier of the list was taken)
            ee_unit_type *mtd = unit_get_type_data(m->u);
            int pushing = 0;
            ee_unit_stats *mst = mtd ? unit_get_stats(m->u) : NULL;
            float f = mst ? unit_stats_get_speed(mst) * 100.0f + unit_stats_get_sight(mst) : 0.0f;
            for (int p = 0; p < SN_PUSHES; p++) pushing |= z->push[p].on && z->push[p].from == m->ter;
            if (m->pool == SNP_MAIN && !m->scout && !pushing && f > bf) { bf = f; b = i; }
        }
        if (b < 0) break;
        z->m[b].scout = 1; z->m[b].ter = -1; z->m[b].order_t = 0;
        scouts++;
    }
    for (int i = 0; i < nn; i++) {
        struct sn_unit *m = &z->m[i];
        if (!m->scout) continue;
        ivec2 p = unit_get_tile(m->u);
        if (m->ter >= 0 && m->ter < n && !g_sn_seen[idx][m->ter] && m->since && now - m->since > SN_SCOUT_GIVE_UP) g_sn_seen[idx][m->ter] = 2;   // not reached in that time: given up (walled off, held against it)
        if (m->ter < 0 || m->ter >= n || g_sn_seen[idx][m->ter]) {   // the next one: the nearest it has not stood in, on its land, no other scout's
            unsigned land = area_id(p, 2);
            int b = -1, bd = 0;
            for (int r = 0; r < n; r++) {
                if (g_ter.t[r].water || g_sn_seen[idx][r] || (land && area_id(g_ter.t[r].c, 2) != land)) continue;
                int taken = 0, d = ivec2_dist2(g_ter.t[r].c, p);
                for (int j = 0; j < nn; j++) taken |= j != i && z->m[j].scout && z->m[j].ter == r;
                if (!taken && (b < 0 || d < bd)) { b = r; bd = d; }
            }
            if (b < 0 || !player_ai_toggle(pl, SM_AIT_EXPLORE)) {   // none left (or the scenario's Explore off): the main force's again
                unsigned r = ter_near(p.x, p.y);
                m->scout = 0; m->ter = (short)(r < (unsigned)n ? (int)r : 0); m->order_t = 0;
                z->scouted++;
                z->scout_off = now + SN_SCOUT_REST;   // what is left lies on other land: no scout is taken for it again at once (372 taken and sent back in an hour for 2 territories across the water)
                continue;
            }
            m->ter = (short)b; m->order_t = 0; m->since = now ? now : 1;
        }
        if (!m->order_t || (now - m->order_t > SN_REORDER && ar_idle(m->u))) sn_order(z, m, 0, g_ter.t[m->ter].c, 0, now);
    }
    // THE RESERVE (the player 2026-11-15: "the ai should never stop training units if it can help it, if its at its pop
    // limit it should train straight into the stash"): units inside a Fortress do not count toward the population.
    // Every SN_RES_EVERY: with the map full (SN_RES_ROOM places left) and no territory of its under attack, up to
    // (and no citizens the shelf would take first: bunker.c shelf_first) SN_RES_BATCH soldiers of the main force and the siege - not of a territory that pushes - go into a Fortress of
    // the reserve, the nearest to it first; the training fills the map again. No Fortress with room: one more is
    // built (bunker.c bk_build, BK_MAX). Out again, a Fortress a look: the nearest to a territory of its with enemy
    // soldiers in it (SN_RES_FOE of them), or the fullest that fits into the room on the map (SN_RES_ROOM kept) - and nobody in for
    // SN_RES_AGAIN after that.
    if (!z->res_t || now < z->res_t || now - z->res_t >= SN_RES_EVERY) {
        z->res_t = now ? now : 1;
        int room = bk_room(pl), hard_r = -1, ob = -1, od = 0;
        for (int r = 0; r < n && hard_r < 0; r++) if (!g_ter.t[r].water && s->t[r].role != TVR_NONE && fn[r] >= SN_RES_FOE) hard_r = r;   // a scout or two let nobody out (the first game: 37 of 44 looks let a fortress out)
        g_bk_more[idx] = 0;
        if (hard_r >= 0 || room > SN_RES_ROOM)
            for (int k = 0; k < g_bk[idx].nf; k++) {
                ee_unit *fo = g_bk[idx].f[k];
                if (g_bk[idx].store[k] != 2 || bk_held(fo) <= 0) continue;
                if (hard_r < 0 && bk_held(fo) + SN_RES_ROOM > room) continue;   // ONLY FOR ROOM (the player 2026-11-15: "if the pop limit has space again the units should go out and do tasks"): whoever fits into the room on the map comes out
                int d = hard_r >= 0 ? ivec2_dist2(unit_get_tile(fo), g_ter.t[hard_r].c) : -bk_held(fo);
                if (ob < 0 || d < od) { ob = k; od = d; }
            }
        if (ob >= 0) {
            ee_unit *fo = g_bk[idx].f[ob];
            int held = bk_held(fo);
            if (bk_unload(fo)) {
                int again = z->res_last == fo && z->res_last_n == held;   // told again: the game took the first order and did nothing (bunker.c bk_unload tries the next side)
                z->res_last = fo; z->res_last_n = held;
                if (!again) z->res_out += (unsigned)held;
                z->res_out_t = now ? now : 1;
                if (g_log && !again) log_locked("# reserve: P%u at %.1f min: room %d%s - the fortress at %d,%d lets its %d soldiers out (%u in, %u out so far)\n", idx, now / 60000.0, room,
                                      hard_r >= 0 ? ", enemy soldiers in a territory of its" : "", unit_get_x(fo), unit_get_y(fo), held, z->res_in, z->res_out);
            }
        } else if (hard_r < 0 && room <= SN_RES_ROOM && !shelf_first(pl, idx) && (!z->res_out_t || now < z->res_out_t || now - z->res_out_t >= SN_RES_AGAIN)) {
            int best = -1, space = 0;
            for (int k = 0; k < g_bk[idx].nf; k++) {
                ee_unit *fo = g_bk[idx].f[k];
                int sp = bk_capacity(fo) - bk_held(fo);
                if (g_bk[idx].store[k] == 1 || g_bk[idx].shelter[k] || g_bk[idx].walk[k] > 0 || (!g_bk[idx].store[k] && bk_held(fo) > 0)) continue;   // one load order a fortress at a time
                if (sp > space) { space = sp; best = k; }
            }
            if (best < 0 || space < 5) g_bk_more[idx] = 1;
            else {
                ee_unit *fo = g_bk[idx].f[best], *pick[SN_RES_BATCH];
                int pi_[SN_RES_BATCH], npk = 0, take = space < SN_RES_BATCH ? space : SN_RES_BATCH;
                ivec2 fp = unit_get_tile(fo);
                while (npk < take) {
                    int ci = -1, cdist = 0;
                    for (int i = 0; i < nn; i++) {
                        struct sn_unit *m = &z->m[i];
                        if (m->scout || m->pool == SNP_AA || m->ter < 0 || m->ter >= n || fn[m->ter] > 0 || (m->stash_t && now >= m->stash_t && now - m->stash_t < SN_RES_AGAIN)) continue;
                        int busy = 0;
                        for (int k = 0; k < SN_PUSHES; k++) busy |= z->push[k].on && z->push[k].from == m->ter;
                        for (int j = 0; j < npk; j++) busy |= pi_[j] == i;
                        if (busy) continue;
                        int d = ivec2_dist2(unit_get_tile(m->u), fp);
                        if (ci < 0 || d < cdist) { ci = i; cdist = d; }
                    }
                    if (ci < 0) break;
                    pi_[npk] = ci; pick[npk++] = z->m[ci].u;
                }
                if (npk && bk_load(fo, pick, npk)) {
                    g_bk[idx].store[best] = 2; g_bk[idx].walk[best] += npk;
                    for (int j = 0; j < npk; j++) z->m[pi_[j]].stash_t = now ? now : 1;
                    z->res_in += (unsigned)npk;
                    if (g_log) log_locked("# reserve: P%u at %.1f min: room %d - %d soldiers into the fortress at %d,%d (holds %d; %u in, %u out so far)\n", idx, now / 60000.0, room, npk, fp.x, fp.y, bk_held(fo), z->res_in, z->res_out);
                }
            }
        }
    }
    // DEMOLITION: each territory's enemy wall piece or gate nearest its place, and who is at one already
    static ee_unit *demo[PR_T];
    static int dn[PR_T];
    for (int r = 0; r < n; r++) { demo[r] = NULL; dn[r] = 0; }
    {
        static int dd[PR_T];
        for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
            ee_player *o = unit_get_owner(it.u);
            if (!o || player_get_index(o) <= 0 || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || unit_get_x(it.u) < 0 ||
                !(is_wall(unit_type_get_id(it.td)) || unit_type_get_class_id(it.td) == CLS_GATE) || !explored_by(it.u, pl)) continue;
            ivec2 q = unit_get_tile(it.u);
            unsigned r = ter_near(q.x, q.y);
            if (r >= (unsigned)n || s->t[r].role == TVR_NONE || s->t[r].ebld || fn[r] > 0) continue;
            int d = ivec2_dist2(q, rally[r]);
            if (!demo[r] || d < dd[r]) { demo[r] = it.u; dd[r] = d; }
        }
        for (int i = 0; i < nn; i++) {   // at a piece already
            ee_goal *g = unit_get_goal(z->m[i].u);
            ee_unit *tg = g && goal_is_attack(g) ? goal_get_target(g) : NULL;
            ee_unit_type *td = tg ? unit_get_type_data(tg) : NULL;
            if (td && z->m[i].ter >= 0 && z->m[i].ter < n && (is_wall(unit_type_get_id(td)) || unit_type_get_class_id(td) == CLS_GATE)) dn[z->m[i].ter]++;
        }
    }
    // ORDERS
    static struct { short r, cur; uint8_t fight, urgent; ivec2 d; int n; long sx, sy; uint32_t oldest; uint32_t ids[SN_GROUP_MAX]; short mi[SN_GROUP_MAX]; } grp[SN_GROUPS];
    int ngrp = 0;
    for (int i = 0; i < nn; i++) {
        struct sn_unit *m = &z->m[i];
        if (m->scout || m->ter < 0 || m->ter >= n) continue;
        if (m->stash_t && now >= m->stash_t && now - m->stash_t < SN_RES_AGAIN) continue;   // THE RESERVE: on its way into a fortress
        int r = m->ter, reach = SN_NEAR, fight = 1;
        ivec2 d = rally[r], p = unit_get_tile(m->u);
        struct sn_push *pu = NULL;
        for (int k = 0; k < SN_PUSHES; k++) if (z->push[k].on && z->push[k].from == r) pu = &z->push[k];
        if (m->pool == SNP_AA) {   // ANTI-AIR: at its station's place, whatever goes on
            if (ivec2_dist2(p, d) > SN_NEAR * SN_NEAR && (!m->order_t || (now - m->order_t > SN_REORDER && ar_idle(m->u)))) sn_order(z, m, i, d, 0, now);
            continue;
        }
        if (!pu && demo[r] && (m->pool == SNP_MAIN || m->pool == SNP_SIEGE)) {   // DEMOLITION
            ee_goal *cg = unit_get_goal(m->u);
            ee_unit *tg = cg && goal_is_attack(cg) ? goal_get_target(cg) : NULL;
            ee_unit_type *td = tg ? unit_get_type_data(tg) : NULL;
            if (td && unit_get_hp(tg) > 0 && (is_wall(unit_type_get_id(td)) || unit_type_get_class_id(td) == CLS_GATE)) { m->wait_t = 0; continue; }   // at it
            if (dn[r] < SN_DEMO && ar_idle(m->u)) {
                ee_goal *g = goal_attack(m->u, demo[r]);
                if (g && unit_goal(m->u, g, 0, 0x1F40)) { m->order_t = now ? now : 1; m->to = unit_get_tile(demo[r]); m->wait_t = 0; dn[r]++; z->orders++; z->demos++; continue; }
            }
        }
        if (pu && pu->breach) {   // A BREACH: the siege (everybody when it has none there) at the piece, the rest by it
            ee_unit *piece = sn_wall_at(pl, pu->wall);
            if (piece && (m->pool == SNP_SIEGE || !acs[r])) {
                if (!m->order_t || (ar_idle(m->u) && now - m->order_t >= 3000)) {
                    ee_goal *g = goal_attack(m->u, piece);
                    if (g && unit_goal(m->u, g, 0, 0x1F40)) { m->order_t = now ? now : 1; m->to = pu->wall; z->orders++; }
                }
                continue;
            }
            d = pu->wall; reach = 6;
        } else
        if (pu) { d = pu->at; reach = 5; }
        else if (fn[r] > 0 && s->t[r].role != TVR_NONE) { d = iv2(fx[r] / fn[r], fy[r] / fn[r]); reach = 5; }
        else if (m->pool == SNP_SIEGE) fight = 0;
        if (ivec2_dist2(p, d) <= reach * reach) { m->wait_t = 0; continue; }
        int moved = m->order_t && ivec2_dist2(m->to, d) > 6 * 6;   // its place has moved: the enemy, the push's next building
        if (!(!m->order_t || (moved && now - m->order_t >= 10000) || (now - m->order_t > SN_REORDER && ar_idle(m->u)))) continue;
        // AS A GROUP: with the others that stand in its territory and go to the same place
        int cur = (int)ter_near(p.x, p.y), urgent = pu || (fn[r] > 0 && s->t[r].role != TVR_NONE), k = 0;
        while (k < ngrp && (grp[k].r != r || grp[k].cur != cur || grp[k].fight != fight)) k++;
        if (k == ngrp) {
            if (ngrp == SN_GROUPS) { sn_order(z, m, i, d, fight, now); m->wait_t = 0; continue; }
            grp[k].r = (short)r; grp[k].cur = (short)cur; grp[k].fight = (uint8_t)fight; grp[k].urgent = 0; grp[k].d = d; grp[k].n = 0; grp[k].sx = grp[k].sy = 0; grp[k].oldest = now; ngrp++;
        }
        if (grp[k].n == SN_GROUP_MAX) { sn_order(z, m, i, d, fight, now); m->wait_t = 0; continue; }
        if (!m->wait_t) m->wait_t = now ? now : 1;
        if (m->wait_t < grp[k].oldest) grp[k].oldest = m->wait_t;
        grp[k].urgent |= (uint8_t)urgent;
        grp[k].ids[grp[k].n] = (uint32_t)unit_get_serial(m->u); grp[k].mi[grp[k].n++] = (short)i;
        grp[k].sx += p.x; grp[k].sy += p.y;
    }
    for (int k = 0; k < ngrp; k++) {
        if (grp[k].n < SN_GROUP && !grp[k].urgent && now - grp[k].oldest < SN_GROUP_WAIT) continue;   // they wait for more
        ivec2 from = iv2((int)(grp[k].sx / grp[k].n), (int)(grp[k].sy / grp[k].n));
        if (grp[k].n == 1) { struct sn_unit *m = &z->m[grp[k].mi[0]]; sn_order(z, m, 0, grp[k].d, grp[k].fight, now); m->wait_t = 0; continue; }
        sn_group_move(pl, grp[k].ids, grp[k].n, from, grp[k].d, grp[k].fight);
        z->orders++; z->groups++; z->grouped += (unsigned)grp[k].n;
        for (int j = 0; j < grp[k].n; j++) { struct sn_unit *m = &z->m[grp[k].mi[j]]; m->order_t = now ? now : 1; m->to = grp[k].d; m->wait_t = 0; }
    }
    z->need_on = 0;   // THE NEED'S PLACE
    for (int k = 0; k < SN_PUSHES && !z->need_on; k++) if (z->push[k].on) { z->need_at = z->push[k].at; z->need_on = 1; }
    if (!z->need_on) {
        int b = -1, big = -1, bd = 0;
        for (int r = 0; r < n; r++) {
            if (fn[r] > 0 && s->t[r].role != TVR_NONE && (b < 0 || fn[r] > fn[b])) b = r;
            if (tc[r] > 0 && (big < 0 || tc[r] > tc[big])) big = r;
        }
        if (b >= 0) { z->need_at = iv2(fx[b] / fn[b], fy[b] / fn[b]); z->need_on = 1; }
        for (int e = 0; b < 0 && big >= 0 && e < neb; e++) {
            int d = ivec2_dist2(eb[e].p, rally[big]);
            if (!z->need_on || d < bd) { z->need_at = eb[e].p; z->need_on = 1; bd = d; }
        }
    }
    if (g_log && (!z->log_t || now - z->log_t >= 60000)) {
        z->log_t = now ? now : 1;
        char buf[400] = "";
        int o = 0, pushing = 0;
        for (int r = 0; r < n && o < 330; r++)
            if (tc[r] || ac[r]) o += snprintf(buf + o, sizeof buf - (size_t)o, " %d:%d/%d%s", r, ac[r], tc[r], fn[r] && s->t[r].role != TVR_NONE ? "!" : "");
        for (int k = 0; k < SN_PUSHES; k++) pushing += z->push[k].on;
        log_locked("# stations: P%u at %.1f min: the main force %d, siege %d, anti-air %d, scouts %d (territories never stood in: %d) | pushes on %d, so far %u started, %u won, %u lost | "
                   "so far %u changes of station, %u orders (%u to groups, %u soldiers in them), %u scouts back, %u set at enemy walls | territory:stationed/share (! enemy soldiers there):%s\n",
                   idx, now / 60000.0, np[SNP_MAIN], np[SNP_SIEGE], np[SNP_AA], scouts, unseen, pushing, z->pushes, z->won, z->lost, z->moved, z->orders, z->groups, z->grouped, z->scouted, z->demos, buf);
    }
}
internal void stations_new_game(void) { memset(g_sn, 0, sizeof g_sn); memset(g_sn_seen, 0, sizeof g_sn_seen); }
