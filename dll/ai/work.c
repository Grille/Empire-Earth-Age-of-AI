// THE CITIZENS' WORK BY TERRITORY - the first piece of the mod's own economy, played while the game's computer player
// is off ([ai_base] game_ai = 0, eeaimod.c cp_update_own: nobody else sends a citizen to work then).
// The player, 2026-11-14, asked where to begin on the clean slate: "should we handle citezens and buildings over
// territors too maybe? teretories tell the ai what resouces it offers and asign the terretory citezens, they work on
// the mines. farming and woodworks is reserved for save backside territories where possible".
// So the territory view (territories.c) is the ground of it:
//   WHERE  - a territory is worked when the AI has a building in it (its drop sites are there) and it is not being
//            lost (TV_UNSAFE: nobody is sent in, as the shelter has it); no site in the reach of an enemy tower it
//            has explored (wk_danger - remembered, not only those in sight now: attack.c's threat_at forgets a
//            tower nobody looks at, and on a small map the next woodcutter walked under it: 61 of one AI's
//            citizens dead to a neighbour's towers in 40 minutes), and a citizen whose site lies under one is
//            sent to another. Of those, BACKLAND first for wood and food
//            (a site's distance counts double in a front, three times in a territory it only stands in), mines
//            wherever they lie - a mine does not move. A territory next to one of its own is the last resort (a
//            long walk: the drop site there is the building step's to bring).
//   WHAT   - each mine a territory offers takes the game's places for one (table value 0x16, 6) of its citizens,
//            walking there counted; trees and bushes as many as come.
//   HOW MANY - of its free citizens (those not building, not held for a landing or an outpost) a share per resource
//            by what it needs (WHAT IT NEEDS, below: what the last 10 minutes brought in and spent) - in a game's
//            first minutes by a plain rule, WK_SHARE, pushed towards what the bank lacks; the mines' share is cut to
//            the places the worked territories offer and what is cut goes to food and wood.
//   WHO    - every WK_EVERY: an idle citizen (no goal, or a gather goal whose site is gone or empty) to the resource
//            furthest under its share, at the nearest site that fits on its land (resource_mgr_assign, the game's
//            executor); then up to WK_MOVES citizens from a resource 2 or more over its share to one 2 or more under.
// Food: its finished fields first, a farmer each (build.c lays them), then the bushes, then animals. What is built
// is build.c's. Nothing of the old economy's tuning is here (econ_targets, the food plan, the mine slots): those vet
// the game's numbers and are silent with it - what proves itself here is brought over piece by piece.
#define WK_EVERY 3000
#define WK_MOVES 2
#define WK_CITS 768
#define WK_MINES 512
#define WK_BUSHES 512
internal const float WK_SHARE[7] = { 0, 0, 0.36f, 0.30f, 0.06f, 0.14f, 0.14f };   // food, wood, stone, gold, iron
struct wk_cit { ee_unit *u; ee_unit *site; uint8_t res; };   // res 0: idle
struct wk_site { ee_unit *u; uint16_t terr; uint8_t res, n, cost, cap; };   // n: its citizens there or on their way; cost: its territory's (wk_cost); cap: the food sites' places (a bush 3, a field 1)
internal struct wk_st {
    uint32_t t, log_t;
    int have[7], want[7], places[7], idle, busy, n, food_short, by_need;   // food_short: citizens wanted on food that found no site at the last look
    unsigned assigned, moved, pulled, none[7];   // none: no site found for a citizen wanted on that resource; pulled: looks a citizen's site lay under an enemy tower
} g_wk[16];
// the enemy defences it has explored (buildings stay known once seen), every 5 s; wk_danger: a tile in the reach of one
#define WK_DEFS 384
internal struct { uint32_t t; int n; short x[WK_DEFS], y[WK_DEFS]; float r[WK_DEFS]; } g_wkd[16];
internal void wk_defences(ee_player *pl, unsigned idx, uint32_t now) {
    if (g_wkd[idx].t && now >= g_wkd[idx].t && now - g_wkd[idx].t < 5000) return;
    g_wkd[idx].t = now ? now : 1;
    int n = 0;
    for (struct grid_it it = grid_it_all(1); n < WK_DEFS && grid_it_next(&it); ) {
        float r, a;
        if (unit_get_hp(it.u) <= 0 || !is_enemy(pl, unit_get_owner(it.u)) || !defence_stats(it.u, &r, &a) || !explored_by(it.u, pl)) continue;
        g_wkd[idx].x[n] = (short)unit_get_x(it.u); g_wkd[idx].y[n] = (short)unit_get_y(it.u); g_wkd[idx].r[n++] = r + 2.0f;
    }
    g_wkd[idx].n = n;
}
internal int wk_danger(unsigned idx, ivec2 p) {
    for (int k = 0; k < g_wkd[idx & 15].n; k++) {
        float dx = (float)(p.x - g_wkd[idx & 15].x[k]), dy = (float)(p.y - g_wkd[idx & 15].y[k]), r = g_wkd[idx & 15].r[k];
        if (dx * dx + dy * dy <= r * r) return 1;
    }
    return 0;
}
// what a site's distance is multiplied by, by its territory: 1 backland, 2 a front it holds, 3 one it only has a
// building in, 6 one beside such a one that nobody else has a building in; 0: not to be worked (water, being lost,
// another's, far from everything of its)
internal int wk_cost(struct tv_st *s, unsigned r) {
    if (r >= (unsigned)s->n || g_ter.t[r].water || s->t[r].state == TV_UNSAFE) return 0;
    if (s->t[r].role == TVR_BACK) return 1;
    if (s->t[r].role == TVR_FRONT) return 2;
    if (s->t[r].bld >= 1) return 3;
    if (s->t[r].ebld || s->t[r].abld) return 0;   // somebody lives there: no ground to walk into or settle (an AI's Settlement and tower by a mine in its neighbour's home shot that neighbour's citizens at their work: 34 to 52 of an AI's dead in 40 minutes)
    for (int k = 0; k < g_ter.t[r].nbn; k++) {
        unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
        if (o < (unsigned)s->n && !g_ter.t[o].water && s->t[o].bld >= 1 && s->t[o].state != TV_UNSAFE) return 6;
    }
    return 0;
}
// WHAT IT NEEDS (the player 2026-11-14: "we probably also record resource stats for the ai, how much as gatherd in the
// last 10 minutes, how much spend, defrence etc, so where not stuck on hard numbers but what the ai actually needs").
// A minute at a time, the last WK_WIN of them: what came in of each resource (the income the budget counts), what went
// out (that less the bank's change) and how many of its citizens stood on it. From 3 minutes on the shares come of it:
//   a citizen brings  - in / citizen-minutes (WK_PER_CIT until 3 citizen-minutes are there),
//   needed a minute   - what was spent a minute, and what the priority list holds back and the bank has not got, over
//                       5 minutes (budget_reserve: the epoch, what is saved for); food at least what keeps its citizen
//                       producers training without a pause; half as much again while the bank has under 150 of it (it
//                       would have spent more: spending follows income); half of it with more than 5 minutes of it and
//                       1500 in the bank, a fifth with 15 minutes and 4000; and never under 4% of all that is needed
//                       (what it never had it never spent),
//   citizens          - needed / a citizen brings, scaled to the citizens there are.
// "# needs:" every 5 minutes.
#define WK_WIN 10
internal const float WK_PER_CIT[7] = { 0, 0, 20, 20, 15, 15, 15 };
internal struct wk_need {
    uint32_t t, log_t;
    int n, cur, samples;
    int in[WK_WIN][7], out[WK_WIN][7], inc0[7], stock0[7], acc[7], sum_in[7], sum_out[7];
    float cm[WK_WIN][7], per_cit[7], need[7];
} g_wn[16];
internal int wk_needs(ee_player *pl, unsigned idx, struct wk_st *w, uint32_t now, float cits[7]) {
    struct wk_need *q = &g_wn[idx];
    const int *stock = player_stock_at(pl);
    if (!q->t || now < q->t) {
        memset(q, 0, sizeof *q);
        q->t = now ? now : 1;
        for (int r = 2; r < 7; r++) { q->inc0[r] = g_bud[idx].income[r]; q->stock0[r] = stock[r]; }
    }
    for (int r = 2; r < 7; r++) q->acc[r] += w->have[r];
    q->samples++;
    if (now - q->t >= 60000) {   // the minute's bucket
        int k = q->cur;
        for (int r = 2; r < 7; r++) {
            int in = g_bud[idx].income[r] - q->inc0[r], out = in - (stock[r] - q->stock0[r]);
            q->in[k][r] = in > 0 ? in : 0; q->out[k][r] = out > 0 ? out : 0;
            q->cm[k][r] = q->samples ? (float)q->acc[r] / (float)q->samples : 0;
            q->inc0[r] = g_bud[idx].income[r]; q->stock0[r] = stock[r]; q->acc[r] = 0;
        }
        q->samples = 0; q->cur = (k + 1) % WK_WIN; q->t = now;
        if (q->n < WK_WIN) q->n++;
    }
    if (q->n < 3) return 0;
    int reserve[7];
    budget_reserve(pl, reserve);
    float total = 0;
    for (int r = 2; r < 7; r++) {
        float cm = 0;
        q->sum_in[r] = q->sum_out[r] = 0;
        for (int k = 0; k < q->n; k++) { q->sum_in[r] += q->in[k][r]; q->sum_out[r] += q->out[k][r]; cm += q->cm[k][r]; }
        float pc = cm >= 3.0f && q->sum_in[r] > 0 ? (float)q->sum_in[r] / cm : WK_PER_CIT[r];
        q->per_cit[r] = pc < 3.0f ? 3.0f : pc > 80.0f ? 80.0f : pc;
        float spend = (float)q->sum_out[r] / (float)q->n, need = spend;
        if (reserve[r] > stock[r]) need += (float)(reserve[r] - stock[r]) / 5.0f;
        if (r == 2 && need < cz_nonstop_food(pl)) need = cz_nonstop_food(pl);
        if (stock[r] < 150) need *= 1.5f;
        if ((float)stock[r] > 4000.0f + 15.0f * spend) need *= 0.2f;
        else if ((float)stock[r] > 1500.0f + 5.0f * spend) need *= 0.5f;
        q->need[r] = need; total += need;
    }
    for (int r = 2; r < 7; r++) {
        if (q->need[r] < 0.04f * total) q->need[r] = 0.04f * total;
        cits[r] = q->need[r] / q->per_cit[r];
    }
    if (g_log && (!q->log_t || now - q->log_t >= 300000)) {
        q->log_t = now ? now : 1;
        static const char *nm[7] = { "", "", "food", "wood", "stone", "gold", "iron" };
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# needs: P%u at %.1f min: the last %d minutes", idx, now / 60000.0, q->n);
        for (int r = 2; r < 7; r++)
            fprintf(g_log, " | %s in %d out %d bank %d, a citizen brings %.0f a minute, needed %.0f a minute (held back %d)", nm[r], q->sum_in[r], q->sum_out[r], stock[r],
                    (double)q->per_cit[r], (double)q->need[r], reserve[r]);
        fprintf(g_log, "\n");
        LeaveCriticalSection(&g_lock);
    }
    return total > 0;
}
// the map's site of resource res on a tile with something left (a tree, a mine, a bush): the tiles hold them, not the
// unit grid (territory.c ter_tiles reads them the same way)
internal ee_unit *wk_tile_site(ee_map *map, int size, int x, int y, int res) {
    if (x < 0 || y < 0 || x >= size || y >= size) return NULL;
    ee_tile *tile = map_get_tiles(map)[y * size + x];
    for (void *node = tile ? tile_get_objects(tile) : NULL; node; node = tilenode_get_next(node)) {
        ee_unit *o = tilenode_get_object(node);
        ee_unit_type *td = o ? unit_get_type_data(o) : NULL;
        if (!td || unit_type_get_resource_type(td) != res || unit_get_amount(o) <= 0) continue;
        if (res == 2 && unit_type_get_id(td) != UT(RES_FORAGE_BUSH)) continue;   // the animals: food_animal; the fish: the boats; the fields: the building step
        return o;
    }
    return NULL;
}
// the nearest tree for a citizen at p: ring by ring out to `reach` tiles, by distance x its territory's cost (a ring
// further out cannot beat what is found once its bare distance is past that)
internal ee_unit *wk_tree(ee_player *pl, struct tv_st *s, ivec2 p, int reach) {
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0;
    ee_unit *best = NULL;
    float bd = 0;
    for (int d = 1; d <= reach && size && (!best || (float)(d * d) < bd); d++)
        for (int k = -d; k <= d; k++)
            for (int side = 0; side < 4; side++) {
                if ((side & 1) && (k == -d || k == d)) continue;   // the corners once
                int x = p.x + (side == 0 ? k : side == 2 ? k : side == 1 ? -d : d), y = p.y + (side == 0 ? -d : side == 2 ? d : k);
                ee_unit *u = wk_tile_site(map, size, x, y, 3);
                if (!u) continue;
                int c = wk_cost(s, ter_near(x, y));
                if (!c || !explored_by(u, pl) || wk_danger((unsigned)player_get_index(pl) & 15, iv2(x, y))) continue;   // not under an enemy tower it has seen
                float dd = (float)((x - p.x) * (x - p.x) + (y - p.y) * (y - p.y)) * (float)(c * c);
                if (!best || dd < bd) { best = u; bd = dd; }
            }
    return best;
}
internal void work_tick(ee_player *pl, ee_resource_mgr *rm, uint32_t now) {
    static struct wk_cit cit[WK_CITS];
    static struct wk_site mine[WK_MINES], bush[WK_BUSHES];
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct wk_st *w = &g_wk[idx];
    struct tv_st *s = &g_tv[idx];
    if (now < w->t) memset(w, 0, sizeof *w);
    if (w->t && now - w->t < WK_EVERY) return;
    w->t = now ? now : 1;
    if (!s->n || s->version != g_ter.version) return;   // the view first
    wk_defences(pl, idx, now);
    int nc = 0, nm = 0, nb = 0;
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0;
    if (!size) return;
    memset(w->have, 0, sizeof w->have); memset(w->places, 0, sizeof w->places);
    w->idle = w->busy = 0;
    unsigned cap = table_value(0x16);
    if (cap < 1 || cap > 32) cap = 6;
    for (int k = 0; k < g_ter.nsite; k++) {   // what the worked territories offer: the mines and the bushes of the map's sites
        int r = g_ter.site[k].res, x = g_ter.site[k].x, y = g_ter.site[k].y;
        if (r != 2 && (r < 4 || r > 6)) continue;
        unsigned tr = ter_near(x, y);
        int c = wk_cost(s, tr);
        ee_unit *u = c ? wk_tile_site(map, size, x, y, r) : NULL;
        if (!u || !explored_by(u, pl) || wk_danger(idx, iv2(x, y))) continue;
        struct wk_site *e = r == 2 ? (nb < WK_BUSHES ? &bush[nb++] : NULL) : (nm < WK_MINES ? &mine[nm++] : NULL);
        if (!e) continue;
        e->u = u; e->terr = (uint16_t)tr; e->res = (uint8_t)r; e->n = 0; e->cost = (uint8_t)c; e->cap = 3;
        if (r >= 4) w->places[r] += (int)cap;
    }
    {   // its finished fields: a farmer each (build.c lays them; none under raiders, as the old field pass had it)
        ee_civ_mgr *cm = player_get_civ_mgr(pl);
        ee_unit_mgr *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
        uint8_t *dq = um && ut_has(UT(B_FARM)) ? unit_mgr_type_list(um, UT(B_FARM)) : NULL;
        for (unsigned k = 0, first = dq ? deque_front(dq) : 0, cnt = dq ? deque_count(dq) : 0; k < cnt && nb < WK_BUSHES; k++) {
            ee_unit *f = deque_at(dq, first + k), *st = f ? unit_get_physics(f) : NULL;
            if (!st || unit_get_hp(f) <= 0 || !unit_get_finished(st) || unit_get_x(f) < 0) continue;
            unsigned tr = ter_near(unit_get_x(f), unit_get_y(f));
            int c = wk_cost(s, tr);
            if (!c || raiders_near(pl, unit_get_tile(f), 6) || wk_danger(idx, unit_get_tile(f))) continue;
            bush[nb].u = f; bush[nb].terr = (uint16_t)tr; bush[nb].res = 2; bush[nb].n = 0; bush[nb].cost = 0; bush[nb++].cap = 1;   // cost 0: a field before any bush
        }
    }
    int count = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {   // its citizens: what each does
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !is_citizen_type(unit_type_get_id(td))) continue;
        if (unit_get_frozen(u) || unit_get_ai_held(u) || unit_get_busy(u)) { w->busy++; continue; }   // held for a landing or an outpost, building
        ee_goal *g = unit_get_goal(u);
        int gather = g && !strcmp(rtti_name(g), "EEUGGather@@");
        if (g && !gather) { w->busy++; continue; }   // building, repairing, walking on an order
        ee_unit *site = gather ? goal_get_site(g) : NULL;
        int field = site && unit_get_type_data(site) && unit_type_get_id(unit_get_type_data(site)) == UT(B_FARM) && unit_get_hp(site) > 0;
        int r = field ? 2 : site && unit_get_amount(site) > 0 ? unit_get_resource(site) : 0;
        if (r < 2 || r > 6) { r = 0; site = NULL; }
        if (site && wk_danger(idx, unit_get_tile(site))) { r = 0; site = NULL; w->pulled++; }   // its site under an enemy tower it has seen: to another
        if (nc >= WK_CITS) { w->busy++; continue; }
        cit[nc].u = u; cit[nc].site = site; cit[nc++].res = (uint8_t)r;
        if (r) w->have[r]++; else w->idle++;
        for (int k = 0; r >= 4 && k < nm; k++) if (mine[k].u == site) { if (mine[k].n < 255) mine[k].n++; break; }
        for (int k = 0; r == 2 && k < nb; k++) if (bush[k].u == site) { if (bush[k].n < 255) bush[k].n++; break; }
    }
    w->n = nc;
    w->food_short = 0;
    if (!nc) return;
    {   // the shares: by what it needs - at first the plain rule, pushed towards what the bank lacks -, the mines' cut to their places
        const int *stock = player_stock_at(pl);
        float wt[7] = { 0 }, sum = 0, cut = 0;
        w->by_need = wk_needs(pl, idx, w, now, wt);
        for (int r = 2; r < 7; r++) {
            float k = 600.0f / ((float)(stock[r] > 0 ? stock[r] : 0) + 300.0f);
            if (!w->by_need) wt[r] = WK_SHARE[r] * (k < 0.5f ? 0.5f : k);
            sum += wt[r];
        }
        int left = nc;
        for (int r = 4; r < 7; r++) {
            int n = (int)((float)nc * wt[r] / sum + 0.5f);
            if (n > w->places[r]) { cut += (float)(n - w->places[r]); n = w->places[r]; }
            w->want[r] = n; left -= n;
        }
        (void)cut;
        if (left < 0) left = 0;
        w->want[2] = (int)((float)left * wt[2] / (wt[2] + wt[3]) + 0.5f);
        w->want[3] = left - w->want[2];
    }
    int moves = 0;
    for (int pass = 0; pass < 2; pass++)   // the idle ones first, then those of a resource over its share
        for (int i = 0; i < nc; i++) {
            struct wk_cit *c = &cit[i];
            if (pass ? (!c->res || w->have[c->res] - w->want[c->res] < 2 || moves >= WK_MOVES) : c->res != 0) continue;
            ivec2 p = unit_get_tile(c->u);
            unsigned land = area_id(p, 2);
            int tried[7] = { 0 }, done = 0;
            for (int attempt = 0; attempt < 5 && !done; attempt++) {
                int r = 0, most = pass ? 1 : -0x7FFFFFFF;   // a move only to a resource 2 or more under its share
                for (int q = 2; q < 7; q++) if (!tried[q] && q != c->res && w->want[q] - w->have[q] > most) { most = w->want[q] - w->have[q]; r = q; }
                if (!r) break;
                tried[r] = 1;
                ee_unit *site = NULL;
                int mk = -1;
                if (r >= 4) {
                    float bd = 0;
                    for (int k = 0; k < nm; k++) {
                        if (mine[k].res != r || mine[k].n >= cap || site_gatherers(mine[k].u) >= cap) continue;
                        ivec2 q = unit_get_tile(mine[k].u);
                        if (land && area_id(q, 2) && area_id(q, 2) != land) continue;
                        float d = (float)ivec2_dist2(p, q) * (float)(mine[k].cost * mine[k].cost);
                        if (mk < 0 || d < bd) { mk = k; bd = d; }
                    }
                    if (mk >= 0) site = mine[mk].u;
                } else if (r == 3) site = wk_tree(pl, s, p, 60);
                else {
                    float bd = 0;
                    int bk = -1;
                    for (int k = 0; k < nb; k++) {   // a field (a farmer), a bush (3 of its citizens at most)
                        if (bush[k].n >= bush[k].cap || (bush[k].cap > 1 && unit_get_amount(bush[k].u) <= 0)) continue;
                        ivec2 q = unit_get_tile(bush[k].u);
                        if (land && area_id(q, 2) && area_id(q, 2) != land) continue;
                        float d = (float)ivec2_dist2(p, q) * (float)(bush[k].cost * bush[k].cost);
                        if (bk < 0 || d < bd) { bk = k; bd = d; }
                    }
                    if (bk >= 0) { site = bush[bk].u; bush[bk].n++; }
                    else site = food_animal(pl, rm, c->u);   // food.c: an animal to hunt, by its own rules
                }
                if (!site) { w->none[r]++; w->food_short += r == 2; continue; }
                resource_mgr_assign(rm, c->u, site);
                if (c->res) { w->have[c->res]--; w->moved++; moves++; } else { w->idle--; w->assigned++; }
                w->have[r]++;
                if (mk >= 0 && mine[mk].n < 255) mine[mk].n++;
                c->res = (uint8_t)r; c->site = site;
                done = 1;
            }
        }
    if (g_log && (!w->log_t || now - w->log_t >= 60000)) {
        w->log_t = now ? now : 1;
        int worked = 0;
        for (int r = 0; r < s->n; r++) worked += !g_ter.t[r].water && s->t[r].bld >= 1 && s->t[r].state != TV_UNSAFE;
        log_locked("# work: P%u at %.1f min: %d citizens free for work (%d idle, %d more building or held), shares %s | at food %d wood %d stone %d gold %d iron %d | wanted %d %d %d %d %d | "
                   "%d mines and %d bushes in reach of %d worked territories (places stone %d gold %d iron %d) | so far %u sent to work, %u moved, no site for food %u wood %u stone %u gold %u iron %u\n",
                   idx, now / 60000.0, nc, w->idle, w->busy, w->by_need ? "by need" : "by the plain rule", w->have[2], w->have[3], w->have[4], w->have[5], w->have[6], w->want[2], w->want[3], w->want[4], w->want[5], w->want[6],
                   nm, nb, worked, w->places[4], w->places[5], w->places[6], w->assigned, w->moved, w->none[2], w->none[3], w->none[4], w->none[5], w->none[6]);
    }
}
// the citizens it found without work at its last look, -1 while it does not run (the game's computer player on)
internal int work_idle(unsigned idx) {
    struct wk_st *w = &g_wk[idx & 15];
    uint32_t now = game_time();
    return w->t && now >= w->t && now - w->t <= 4 * WK_EVERY ? w->idle : -1;
}
internal void work_new_game(void) { memset(g_wk, 0, sizeof g_wk); memset(g_wn, 0, sizeof g_wn); memset(g_wkd, 0, sizeof g_wkd); }
