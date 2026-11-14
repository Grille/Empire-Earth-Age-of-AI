// THE CITIZENS' WORK BY TERRITORY - the first piece of the mod's own economy, played while the game's computer player
// is off ([ai_base] game_ai = 0, eeaimod.c cp_update_own: nobody else sends a citizen to work then).
// The player, 2026-11-14, asked where to begin on the clean slate: "should we handle citezens and buildings over
// territors too maybe? teretories tell the ai what resouces it offers and asign the terretory citezens, they work on
// the mines. farming and woodworks is reserved for save backside territories where possible".
// So the territory view (territories.c) is the ground of it:
//   WHERE  - a territory is worked when the AI has a building in it (its drop sites are there) and it is not being
//            lost (TV_UNSAFE: nobody is sent in, as the shelter has it). Of those, BACKLAND first for wood and food
//            (a site's distance counts double in a front, three times in a territory it only stands in), mines
//            wherever they lie - a mine does not move. A territory next to one of its own is the last resort (a
//            long walk: the drop site there is the building step's to bring).
//   WHAT   - each mine a territory offers takes the game's places for one (table value 0x16, 6) of its citizens,
//            walking there counted; trees and bushes as many as come.
//   HOW MANY - of its free citizens (those not building, not held for a landing or an outpost) a share per resource
//            by a plain rule, WK_SHARE, pushed towards what the bank lacks (a stock under 200 doubles a resource's
//            weight, one over 600 halves it at most); the mines' share is cut to the places the worked territories
//            offer and what is cut goes to food and wood.
//   WHO    - every WK_EVERY: an idle citizen (no goal, or a gather goal whose site is gone or empty) to the resource
//            furthest under its share, at the nearest site that fits on its land (resource_mgr_assign, the game's
//            executor); then up to WK_MOVES citizens from a resource 2 or more over its share to one 2 or more under.
// Not here yet (the building step): fields - food is bushes and animals until then, which run out -, drop sites by
// the mines, anything built at all. And nothing of the old economy's tuning (econ_targets, the food plan, the mine
// slots): those vet the game's numbers and are silent with it - what proves itself here is brought over piece by piece.
#define WK_EVERY 3000
#define WK_MOVES 2
#define WK_CITS 768
#define WK_MINES 512
#define WK_BUSHES 512
internal const float WK_SHARE[7] = { 0, 0, 0.36f, 0.30f, 0.06f, 0.14f, 0.14f };   // food, wood, stone, gold, iron
struct wk_cit { ee_unit *u; ee_unit *site; uint8_t res; };   // res 0: idle
struct wk_site { ee_unit *u; uint16_t terr; uint8_t res, n, cost; };   // n: its citizens there or on their way; cost: its territory's (wk_cost)
internal struct wk_st {
    uint32_t t, log_t;
    int have[7], want[7], places[7], idle, busy, n;
    unsigned assigned, moved, none[7];   // none: no site found for a citizen wanted on that resource
} g_wk[16];
// what a site's distance is multiplied by, by its territory: 1 backland, 2 a front it holds, 3 one it only has a
// building in, 6 one beside such a one; 0: not to be worked (water, being lost, far from everything of its)
internal int wk_cost(struct tv_st *s, unsigned r) {
    if (r >= (unsigned)s->n || g_ter.t[r].water || s->t[r].state == TV_UNSAFE) return 0;
    if (s->t[r].role == TVR_BACK) return 1;
    if (s->t[r].role == TVR_FRONT) return 2;
    if (s->t[r].bld >= 1) return 3;
    for (int k = 0; k < g_ter.t[r].nbn; k++) {
        unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
        if (o < (unsigned)s->n && !g_ter.t[o].water && s->t[o].bld >= 1 && s->t[o].state != TV_UNSAFE) return 6;
    }
    return 0;
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
                if (!c || !explored_by(u, pl)) continue;
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
        if (!u || !explored_by(u, pl)) continue;
        struct wk_site *e = r == 2 ? (nb < WK_BUSHES ? &bush[nb++] : NULL) : (nm < WK_MINES ? &mine[nm++] : NULL);
        if (!e) continue;
        e->u = u; e->terr = (uint16_t)tr; e->res = (uint8_t)r; e->n = 0; e->cost = (uint8_t)c;
        if (r >= 4) w->places[r] += (int)cap;
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
        int r = site && unit_get_amount(site) > 0 ? unit_get_resource(site) : 0;
        if (r < 2 || r > 6) { r = 0; site = NULL; }
        if (nc >= WK_CITS) { w->busy++; continue; }
        cit[nc].u = u; cit[nc].site = site; cit[nc++].res = (uint8_t)r;
        if (r) w->have[r]++; else w->idle++;
        for (int k = 0; r >= 4 && k < nm; k++) if (mine[k].u == site) { if (mine[k].n < 255) mine[k].n++; break; }
        for (int k = 0; r == 2 && k < nb; k++) if (bush[k].u == site) { if (bush[k].n < 255) bush[k].n++; break; }
    }
    w->n = nc;
    if (!nc) return;
    {   // the shares: the plain rule, pushed towards what the bank lacks, the mines' cut to their places
        const int *stock = player_stock_at(pl);
        float wt[7] = { 0 }, sum = 0, cut = 0;
        for (int r = 2; r < 7; r++) {
            float k = 600.0f / ((float)(stock[r] > 0 ? stock[r] : 0) + 300.0f);
            wt[r] = WK_SHARE[r] * (k < 0.5f ? 0.5f : k);
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
                    for (int k = 0; k < nb; k++) {   // a bush: 3 of its citizens at most
                        if (bush[k].n >= 3 || unit_get_amount(bush[k].u) <= 0) continue;
                        ivec2 q = unit_get_tile(bush[k].u);
                        if (land && area_id(q, 2) && area_id(q, 2) != land) continue;
                        float d = (float)ivec2_dist2(p, q) * (float)(bush[k].cost * bush[k].cost);
                        if (bk < 0 || d < bd) { bk = k; bd = d; }
                    }
                    if (bk >= 0) { site = bush[bk].u; bush[bk].n++; }
                    else site = food_animal(pl, rm, c->u);   // food.c: an animal to hunt, by its own rules
                }
                if (!site) { w->none[r]++; continue; }
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
        log_locked("# work: P%u at %.1f min: %d citizens free for work (%d idle, %d more building or held) | at food %d wood %d stone %d gold %d iron %d | wanted %d %d %d %d %d | "
                   "%d mines and %d bushes in reach of %d worked territories (places stone %d gold %d iron %d) | so far %u sent to work, %u moved, no site for food %u wood %u stone %u gold %u iron %u\n",
                   idx, now / 60000.0, nc, w->idle, w->busy, w->have[2], w->have[3], w->have[4], w->have[5], w->have[6], w->want[2], w->want[3], w->want[4], w->want[5], w->want[6],
                   nm, nb, worked, w->places[4], w->places[5], w->places[6], w->assigned, w->moved, w->none[2], w->none[3], w->none[4], w->none[5], w->none[6]);
    }
}
// the citizens it found without work at its last look, -1 while it does not run (the game's computer player on)
internal int work_idle(unsigned idx) {
    struct wk_st *w = &g_wk[idx & 15];
    uint32_t now = game_time();
    return w->t && now >= w->t && now - w->t <= 4 * WK_EVERY ? w->idle : -1;
}
internal void work_new_game(void) { memset(g_wk, 0, sizeof g_wk); }
