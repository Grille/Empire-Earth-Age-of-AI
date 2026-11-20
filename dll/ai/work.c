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
//   ITS TERRITORY - every citizen belongs to a territory (the player 2026-11-15, after a far drop site had drawn 250 of
//            an AI's citizens into an enemy's towers: "citezens shouldnt be able to be pulled in like that in the first
//            place, every one should be locked to its asigned territory, and moving to another territory is an
//            deliberate ai decison. not an unfinished settlement that pulls the whole population into death"): the
//            one it stands in when it is first seen. It is sent to sites of that territory only. A CHANGE of
//            territory is a step of its own: a citizen that finds no site of the resource it is wanted on at home
//            may be given the nearest territory by the territory graph - over territories its citizens may work
//            in only (wk_cost: one it holds or one beside it, quiet, no enemy building) - that has one, WK_XFER
//            citizens a look at most; the rest stays where it is, on another resource or idle. A territory that
//            may no longer be worked gives its citizens to the nearest it holds (a retreat, not counted).
// Food: its finished fields first, a farmer each (build.c lays them), then the bushes, then animals. What is built
// is build.c's. Nothing of the old economy's tuning is here (econ_targets, the food plan, the mine slots): those vet
// the game's numbers and are silent with it - what proves itself here is brought over piece by piece.
#define WK_EVERY 3000
#define WK_MOVES 2
#define WK_XFER 2     // citizens given another territory a look
#define WK_XFER_T 12  // territories looked at for one
#define WK_T 4096     // territories at most
#define WK_CITS 768
#define WK_MINES 512
#define WK_BUSHES 512
internal const float WK_SHARE[7] = { 0, 0, 0.36f, 0.30f, 0.06f, 0.14f, 0.14f };   // food, wood, stone, gold, iron
#define WK_PATCH 6   // citizens at a food patch (a bush)
#define WK_PATCH_BANK 600
struct wk_cit { ee_unit *u; ee_unit *site; uint8_t res; short home, hk; };   // res 0: idle; home: its territory, hk: its entry in wk_st.home
struct wk_home { ee_unit *u; int id; short ter; };
struct wk_site { ee_unit *u; uint16_t terr; uint8_t res, n, cost, cap; };   // n: its citizens there or on their way; cost: its territory's (wk_cost); cap: the food sites' places (a bush 3, a field 1)
internal struct wk_st {
    uint32_t t, log_t;
    int need_c[7];   // THE POOL: the citizens each resource needs (wk_needs' count and a margin, a mine's no more than its places)
    int have[7], want[7], places[7], idle, busy, n, food_short, by_need, crowd;   // food_short: citizens wanted on food that found no site at the last look; crowd: of the idle, those beyond an animal's WK_HUNTERS
    unsigned assigned, moved, pulled, none[7];   // none: no site found for a citizen wanted on that resource; pulled: looks a citizen's site lay under an enemy tower
    unsigned xfers, retreats;   // ITS TERRITORY: citizens given another territory for a site, given one it holds because theirs may not be worked
    unsigned delivering, load_lost;   // ITS LOAD FIRST: moves held while the citizen carries a load; loads lost (idle, none of its resource)
    int nh; struct wk_home home[WK_CITS];
} g_wk[16];
internal struct { struct wk_site mine[WK_MINES], bush[WK_BUSHES]; int nm, nb; unsigned cap; } g_wks;   // a look's sites
// the enemy defences it has explored (buildings stay known once seen), every 5 s; wk_danger: a tile in the reach of one
#define WK_DEFS 384
internal struct { uint32_t t; int n; short x[WK_DEFS], y[WK_DEFS]; float r[WK_DEFS]; } g_wkd[16];
internal void wk_defences(ee_player *pl, unsigned idx, uint32_t now) {
    if (g_wkd[idx].t && now >= g_wkd[idx].t && now - g_wkd[idx].t < 5000) return;
    g_wkd[idx].t = now ? now : 1;
    int n = 0;
    for (struct grid_it it = grid_it_all(1); n < WK_DEFS && grid_it_next(&it); ) {
        float r, a;
        if (unit_get_hp(it.u) <= 0 || !is_enemy(pl, unit_get_owner(it.u)) || !defence_stats(it.u, &r, &a) || !seen_by(it.u, pl)) continue;   // the enemy defences it has seen (in sight or remembered)
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
// ACROSS THE WATER ([ai_base] expand_across; the player 2026-11-18: expansion and pushes across water "should make
// expansion and pushes across water possible by drawing from the ferry pool, not be a separate system"): a land
// territory is beside another land's territory that touches the same sea territory, or one of two seas side by side -
// as a territory over land is beside its neighbour. The ferry (ferry.c) carries what goes over. With it build.c lays a
// Settlement and a tower by the mines there (TOWER FIRST; on a land with none of its citizens the foundation laid as its
// own - the game's placement makes none without a builder), the ferry brings builders to it (bd_waits) and miners to the
// free places, the stations an ISLAND GUARD.
// MEASURED 2026-11-18 against the outposts (expand.c; head-to-head, mirrored, 12 seeds, Small Islands large, 4 AIs, epoch
// 5, 60 min; with expand_islands=0 for this side), per AI outposts -> this: islands held besides home 0.40 -> 0.96, drop
// sites there 2.8 -> 4.6, citizens there 14 -> 25, miners 8 -> 10, citizens ferried 20 -> 53; soldiers +2.7 (se 3.5),
// kills +1.8 (6.2), workers +3.5 (2.3), losses +18 (5.7): citizens killed 11 -> 27 (enemy soldiers 6.5 -> 17, ships 3.5 ->
// 8). Before the guard and the tower first 33 were killed; before the foundation laid as its own nothing settled at all.
// More ground held and worked for more citizens lost - the player: "dosnt matter if the islands can be hold in the end",
// and a small escort for every settling of unclaimed ground, over land too (stations.c THE ESCORT). With it, against the
// outposts again: islands held at the end 0.44 -> 0.96, drop sites there 3.4 -> 4.3, workers +6.8 (se 1.6), razed +0.5
// (0.3), soldiers -3.5 (3.5), kills -5.8 (7.8), losses +24.8 (7.9), citizens killed 9 -> 29. ON, the outposts off with it
// for the game's computer player off (expand.c EXPAND).
#define WK_ACROSS 16
internal struct { unsigned version; int n; uint16_t (*o)[WK_ACROSS]; uint8_t *no; } g_wkx;
internal void wk_across_build(void) {
    if (g_wkx.o && g_wkx.version == g_ter.version && g_wkx.n == g_ter.n) return;
    free(g_wkx.o); free(g_wkx.no);
    g_wkx.n = g_ter.n; g_wkx.version = g_ter.version;
    g_wkx.o = calloc((size_t)g_ter.n + 1, sizeof *g_wkx.o); g_wkx.no = calloc((size_t)g_ter.n + 1, 1);
    if (!g_wkx.o || !g_wkx.no) { g_wkx.n = 0; return; }
    for (int r = 0; r < g_ter.n; r++) {
        if (g_ter.t[r].water) continue;
        unsigned land = area_id(g_ter.t[r].c, 2);
        for (int i = 0; i < g_ter.t[r].nbn; i++) {
            unsigned w1 = g_ter.nb[g_ter.t[r].nb0 + i].to;
            if (w1 >= (unsigned)g_ter.n || !g_ter.t[w1].water) continue;
            for (int j = -1; j < g_ter.t[w1].nbn; j++) {   // the sea itself (-1), then the seas beside it
                unsigned w = j < 0 ? w1 : g_ter.nb[g_ter.t[w1].nb0 + j].to;
                if (w >= (unsigned)g_ter.n || !g_ter.t[w].water) continue;
                for (int k = 0; k < g_ter.t[w].nbn; k++) {
                    unsigned o = g_ter.nb[g_ter.t[w].nb0 + k].to, q;
                    if (o >= (unsigned)g_ter.n || g_ter.t[o].water || area_id(g_ter.t[o].c, 2) == land) continue;
                    for (q = 0; q < g_wkx.no[r] && g_wkx.o[r][q] != o; q++) ;
                    if (q == g_wkx.no[r] && g_wkx.no[r] < WK_ACROSS) g_wkx.o[r][g_wkx.no[r]++] = (uint16_t)o;
                }
            }
        }
    }
}
// is land territory r across the water from one it holds (safe)
internal int wk_across_held(struct tv_st *s, unsigned r) {
    wk_across_build();
    if (r >= (unsigned)g_wkx.n) return 0;
    for (int q = 0; q < g_wkx.no[r]; q++) {
        unsigned o = g_wkx.o[r][q];
        if (o < (unsigned)s->n && s->t[o].role != TVR_NONE && s->t[o].state != TV_UNSAFE) return 1;
    }
    return 0;
}
// what a site's distance is multiplied by, by its territory: 1 backland, 2 a front it holds; beside one it holds
// (WITHIN REASON, below) 3 one it has a building in, 6 one nobody has a building in; across the water from one it holds
// (ACROSS THE WATER) 5 and 10, beside such a one over its land (ACROSS, INLAND) 10 and 20; 0: not to be worked (water, being lost, another's, far from everything of its)
internal int wk_cost(struct tv_st *s, unsigned r) {
    if (r >= (unsigned)s->n || g_ter.t[r].water || s->t[r].state == TV_UNSAFE) return 0;
    if (s->t[r].role == TVR_BACK) return 1;
    if (s->t[r].role == TVR_FRONT) return 2;
    // WITHIN REASON (the player 2026-11-15: "controlling the whole map is the endgoal, so mines further out fit into that,
    // but within reason, right now yellow(p3) is on a giant death march into territory 46, a territory thats far beyond
    // its border, its soilders disnt secure, with build order under an enemy towers" - a drop site laid there had made it
    // a worked territory, and the wood cutters with no tree left at home went: 296 citizens down to 38): a territory it
    // does not hold is worked only beside one it holds, with no enemy soldier seen in it and no building of another's -
    // a building of its own there changes the price, not that.
    if (s->t[r].ebld || s->t[r].state != TV_QUIET) return 0;
    int held = 0, across = 0;
    for (int k = 0; k < g_ter.t[r].nbn && !held; k++) {
        unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
        held = o < (unsigned)s->n && !g_ter.t[o].water && s->t[o].role != TVR_NONE && s->t[o].state != TV_UNSAFE;
    }
    if (!held && EXPAND_ACROSS((unsigned)(s - g_tv) & 15) && wk_across_held(s, r)) held = across = 1;   // ACROSS THE WATER
    // ACROSS, INLAND ([ai_base] expand_across_inland, off until measured; the player 2026-11-20, his P3 on Large Islands:
    // "p3 has a single transport and gets nothing done" - at minute 146 with 280 soldiers on its home island and 2 idle
    // transports: the islands across its seas had no mine on their coasts (territories 10, 18, 25), the free iron and gold
    // lay one territory further in (8 behind 18, 29 behind 25), and a claim over land wants a neighbour it holds - nothing
    // was ever asked of the ferry after its scouts): a territory beside one across the water from one it holds, over that
    // land, is worked as across too, at twice the price
    if (!held && EXPAND_ACROSS((unsigned)(s - g_tv) & 15) && EXPAND_INLAND((unsigned)(s - g_tv) & 15))
        for (int k = 0; k < g_ter.t[r].nbn && !held; k++) {
            unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
            if (o < (unsigned)s->n && !g_ter.t[o].water && !s->t[o].ebld && s->t[o].state == TV_QUIET && wk_across_held(s, o)) held = across = 2;
        }
    if (!held) {   // it holds nothing yet (the start: a Capitol is one building, TV_HELD make a territory its own): where it has a building, and beside
        for (int o = 0; o < s->n; o++) if (!g_ter.t[o].water && s->t[o].role != TVR_NONE) return 0;
        held = s->t[r].bld >= 1;
        for (int k = 0; k < g_ter.t[r].nbn && !held; k++) {
            unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
            held = o < (unsigned)s->n && !g_ter.t[o].water && s->t[o].bld >= 1 && s->t[o].state != TV_UNSAFE;
        }
        if (!held) return 0;
    }
    if (s->t[r].bld >= 1) return across ? 5 * across : 3;
    if (s->t[r].ebld || s->t[r].abld) return 0;   // somebody lives there: no ground to walk into or settle (an AI's Settlement and tower by a mine in its neighbour's home shot that neighbour's citizens at their work: 34 to 52 of an AI's dead in 40 minutes)
    return across ? 10 * across : 6;
}
internal int wk_workable(unsigned idx, ivec2 p) { return wk_cost(&g_tv[idx & 15], ter_near(p.x, p.y)) > 0; }   // garrison.c: a place its citizens may go to
internal int wk_beside(ivec2 a, ivec2 b) {   // garrison.c: the same territory, or two side by side
    unsigned ra = ter_near(a.x, a.y), rb = ter_near(b.x, b.y);
    if (ra == rb) return ra != TER_NONE;
    if (ra >= (unsigned)g_ter.n || rb >= (unsigned)g_ter.n) return 0;
    for (int k = 0; k < g_ter.t[ra].nbn; k++) if (g_ter.nb[g_ter.t[ra].nb0 + k].to == rb) return 1;
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
#define WK_ARMY_MS 30000   // a soldier's training, about
#define WK_ARMY_BANK 400    // above what is held back
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
    if (g_rs_epoch[idx])   // research.c: the epoch it would buy now - budget_reserve knows it only by the game's time for it (epoch_pacing = rush: Bronze 26 minutes after it was due, 3 citizens on gold and iron)
        for (int r = 2; r < 7; r++) if (epoch_cost(pl, g_rs_epoch[idx], r) > reserve[r]) reserve[r] = epoch_cost(pl, g_rs_epoch[idx], r);
    prio_research(pl, 0, reserve);   // priority.c: what the research's ranks 1 and 2 wait for, the research building it saves for
    // THE ARMY'S APPETITE (the player 2026-11-15: "the ai just barely order units right now, even with full bank it just
    // sits on it" - his game at minute 70: 5 to 13 of 120-300 citizens each on gold and iron, the epoch's saving held
    // all of it, 34 soldiers): what was spent says nothing of what the army would spend - a soldier costs gold or
    // iron, none came in, none was spent, none was needed. What its land production buildings use training without
    // a pause (a soldier of the type it trains now every WK_ARMY_MS in each; of gold and iron each at least half its
    // metal - the next type may cost the other) is needed whatever was spent, of a resource with less than
    // WK_ARMY_BANK above what is held back.
    float army[7] = { 0 };
    {
        ee_civ_mgr *cm = player_get_civ_mgr(pl); ee_unit_mgr *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
        int nb = 0, ty = g_ap[idx].last_type;
        for (size_t k = 0; um && k < sizeof k_ap_land / sizeof *k_ap_land; k++) { int h = 0, b = 0; am_bld(um, g_ut[k_ap_land[k]], &h, &b); nb += h; }
        if (ty && nb && workforce(pl) >= g_min_workforce) {   // the economy first: before that its citizens went to the mines for it and the growth stalled (27 citizens for 40 at minute 20 from the Stone Age)
            float per = (float)nb * 60000.0f / (float)WK_ARMY_MS, metal = (float)(unit_cost(pl, ty, 5) + unit_cost(pl, ty, 6)) * 0.5f;
            for (int r = 2; r < 7; r++) army[r] = (float)unit_cost(pl, ty, r) * per;
            for (int r = 5; r < 7; r++) if (army[r] < metal * per) army[r] = metal * per;
        }
    }
    float total = 0;
    for (int r = 2; r < 7; r++) {
        float cm = 0;
        q->sum_in[r] = q->sum_out[r] = 0;
        for (int k = 0; k < q->n; k++) { q->sum_in[r] += q->in[k][r]; q->sum_out[r] += q->out[k][r]; cm += q->cm[k][r]; }
        float pc = cm >= 3.0f && q->sum_in[r] > 0 ? (float)q->sum_in[r] / cm : WK_PER_CIT[r];
        q->per_cit[r] = pc < 3.0f ? 3.0f : pc > 80.0f ? 80.0f : pc;
        float spend = (float)q->sum_out[r] / (float)q->n, need = spend;
        if (reserve[r] > stock[r]) need += (float)(reserve[r] - stock[r]) / 5.0f;
        if (r == 2 && stock[r] - reserve[r] > WK_ARMY_BANK) army[r] = 0;
        if (r == 2 && need < cz_nonstop_food(pl) + army[2]) need = cz_nonstop_food(pl) + army[2];
        if (stock[r] - reserve[r] > WK_ARMY_BANK) army[r] = 0;   // enough of it lies there: another resource is what its soldiers wait for
        if (r > 2 && need < army[r]) need = army[r];
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
internal ee_unit *wk_tree(ee_player *pl, struct tv_st *s, ivec2 p, int reach, int ter) {
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
                if (ter >= 0 && (int)ter_near(x, y) != ter) continue;   // ITS TERRITORY
                int c = wk_cost(s, ter_near(x, y));
                if (!c || !explored_by(u, pl) || wk_danger((unsigned)player_get_index(pl) & 15, iv2(x, y))) continue;   // not under an enemy tower it has seen
                float dd = (float)((x - p.x) * (x - p.x) + (y - p.y) * (y - p.y)) * (float)(c * c);
                if (!best || dd < bd) { best = u; bd = dd; }
            }
    return best;
}
// THE ANIMALS BY TERRITORY, once a look (the player 2026-11-15, on food_animal being half of the simulation's work in
// his game at 94 min: "shouldnt that value be cached per territory? a flag that says wether it has animals or berries
// at all"). food_animal gives the one animal nearest to any of the player's drop sites - scanning 31 x 31 tiles round
// every one of them, asked again by each citizen for each territory it tried, and its answer of no use but in the
// territory it lay in: a citizen asking for B got nothing while B had animals, the nearest being in A. Now the scan
// runs once, the first time a look asks: every animal food_animal would take (its rules: within food_hunt_range of a
// drop site that takes hunters, food left, not fierce, explored, no enemy near, no known towers on the way) listed by
// its territory, nearest to its drop site first. A territory's list empty: no animals there. The hunters are counted
// at the ask (site_gatherers, under 3), the citizen's land too.
#define WK_ANIMALS 2048
struct wk_animal { ee_unit *u; int d2; uint16_t ter; short area; };
internal struct { int n, built; struct wk_animal e[WK_ANIMALS]; } g_wka;
// HERDS KEPT ([ai_base] herd_keep; the player 2026-11-16: "right now the ai hunts animals to extinction, but in the
// first two epochs it shoudl always leave two animals per herd alive so it can reproduce, that way its always guranted
// food, after the first farm it dosnt matter anymore"; 2026-11-18: one left is enough - "if i than kill one animal an
// small one imediatly spawn from one of the remaining" - and "leave atlest one alive and only hunt grown up animals"):
// before farms (FARM_EPOCH, or never in a game that ends before them) a living animal is listed only when it is grown
// (its scale 1: a herd's young grow up to it) and its herd (spec EEHerd, the animal's +0x200; the world player's list)
// keeps herd_keep living grown ones besides those being hunted - those stay listed, more may join them (its young
// don't count: they breed only once grown, and a herd of young was left to die out). Dead ones (their meat) and the
// animals of no herd (they don't breed) always.
#define WK_HERDS 512
internal struct { ee_herd *h; int allow; } g_wkh[WK_HERDS];
internal int g_wkh_n;
internal unsigned g_wkh_kept, g_wkh_young;   // left out of a look's list: for their herd, as young (the log)
internal int wk_hunt_ok(ee_unit *a);   // THE HUNTER'S OWN NEXT ANIMAL
internal int wk_herd_takes(ee_unit *a) {
    if (unit_get_hp(a) <= 0) return 1;
    ee_herd *h = unit_get_herd(a);
    if (!h) return 1;
    if (unit_get_scale(a) < 1.0f) { g_wkh_young++; return 0; }
    int k = 0;
    while (k < g_wkh_n && g_wkh[k].h != h) k++;
    if (k == g_wkh_n) {
        if (k == WK_HERDS) return 0;
        int alive = 0, hunted = 0;
        ee_unit **b = herd_get_members_begin(h), **e = herd_get_members_end(h);
        for (ee_unit **m = b; b && m < e && m - b < 256; m++)
            if (*m && unit_get_hp(*m) > 0 && unit_get_x(*m) >= 0 && unit_get_scale(*m) >= 1.0f) { alive++; hunted += site_gatherers(*m) > 0; }
        g_wkh[k].h = h; g_wkh[k].allow = alive - g_herd_keep - hunted; g_wkh_n++;
        if (alive <= g_herd_keep) g_wkh[k].allow = -1000;   // none of it: those already on one finish it
    }
    if (g_wkh[k].allow <= -1000) { g_wkh_kept++; return 0; }
    if (site_gatherers(a) > 0) return wk_hunt_ok(a);   // being hunted: more may join it - while the check of its hunters lets it be (else handed it again each turn: 4495 times a game)
    if (g_wkh[k].allow <= 0) { g_wkh_kept++; return 0; }
    g_wkh[k].allow--;
    return wk_hunt_ok(a);   // listed only as the check of its hunters will let it be: else handed back and forth
}
internal void wk_herds_keep(ee_player *pl) {
    if (!g_herd_keep || (current_epoch(pl) >= FARM_EPOCH && end_epoch() >= FARM_EPOCH)) return;
    g_wkh_n = 0;
    int m = 0;
    for (int i = 0; i < g_wka.n; i++) {   // in the look's order (nearest first): the nearest of a herd are the ones taken
        int dup = 0;
        for (int j = 0; j < m && !dup; j++) dup = g_wka.e[j].u == g_wka.e[i].u;   // in reach of two drop sites: once
        if (!dup && wk_herd_takes(g_wka.e[i].u)) g_wka.e[m++] = g_wka.e[i];
    }
    g_wka.n = m;
}
// THE HUNTER'S OWN NEXT ANIMAL (the player 2026-11-18: "i just watched them hunt dear to extinction, they attacked
// young animals aswell as the last dear. are you alredy handeling that the hunters switch to the next animal
// automaticly when the current one is depleted? they dont need an explict order"): a hunter whose carcass is gone
// takes the nearest animal on its own, past the look's list - so each look asks of every citizen on a living animal
// (its gather or its attack) whether HERDS KEPT allows it: grown, and of its herd those hunted in the herd's order
// leave herd_keep unhunted. One that isn't is given another site at once (wk_hunters_check, below); the look's list
// asks the same of the animals being hunted. Not an animal fighting the citizen (the militia's, AN ANIMAL'S FIGHT),
// not after the farms
internal unsigned g_wkh_stopped;   // hunters given another site off an animal HERDS KEPT leaves (the log)
internal int wk_hunt_ok(ee_unit *a) {
    ee_herd *h = unit_get_herd(a);
    if (!h || unit_get_hp(a) <= 0) return 1;
    if (unit_get_scale(a) < 1.0f) return 0;
    ee_unit **b = herd_get_members_begin(h), **e = herd_get_members_end(h);
    int alive = 0;
    for (ee_unit **m = b; b && m < e && m - b < 256; m++) alive += *m && unit_get_hp(*m) > 0 && unit_get_x(*m) >= 0 && unit_get_scale(*m) >= 1.0f;
    for (ee_unit **m = b; b && m < e && m - b < 256; m++) {   // the hunted grown ones in the herd's order, each taking one off
        if (!*m || unit_get_hp(*m) <= 0 || unit_get_x(*m) < 0 || unit_get_scale(*m) < 1.0f || (*m != a && !site_gatherers(*m))) continue;
        if (alive - 1 < g_herd_keep) return 0;
        if (*m == a) return 1;
        alive--;
    }
    return alive - 1 >= g_herd_keep;
}
internal ee_unit *wk_hunted(ee_unit *u, ee_goal *g) {   // the living wild animal u hunts by its goal, else NULL
    if (!g) return NULL;
    const char *gn = rtti_name(g);
    ee_unit *a = !strcmp(gn, "EEUGGather@@") ? goal_get_site(g) : goal_is_attack(g) ? goal_get_target(g) : NULL;
    ee_unit_type *td = a ? unit_get_type_data(a) : NULL;
    ee_player *o = a ? unit_get_owner(a) : NULL;
    if (!td || !o || player_get_index(o) != 0 || unit_type_get_family(td) != KIND_ANIMAL || unit_get_hp(a) <= 0) return NULL;
    ee_goal *ag = unit_get_goal(a);
    if (ag && goal_is_attack(ag) && goal_get_target(ag) == u) return NULL;   // it fights this citizen: its fight
    return a;
}
// THE HUNTER'S OWN NEXT ANIMAL, at its source (the player 2026-11-18, a Stone Age game: "all animals killed by minute
// 24"): the turn's check takes a hunter off an animal only after its first blows - a herd's last grown giraffe stood at
// 5 of 15 hit points beside its young, and the next round killed it. So the game's own choice of a citizen's next
// resource (unit_functions.citizen_next_resource: a hunter whose carcass is eaten) gives a mod's player no animal
// HERDS KEPT leaves: none instead - the citizen stands idle with its load, and the next look gives it work, its load's
// resource first (ITS LOAD FIRST)
internal unsigned g_wkh_source;   // next animals refused at the game's choice (the log)
// A CROWD ON ONE ANIMAL (the player 2026-11-18, his Stone Age game at 95 min: "100 citezens walking up to a single dear
// isnt usefull" - the game's next animal is the nearest, however many already went for it; 208 of the other AI's
// citizens on food, the island's last animals among them; "a single animal should maximal get 4 citezens"): an animal
// takes WK_HUNTERS of its citizens at most - the game's choice of one that has them is none (the citizen waits for the
// look, which gives it work), and the hunters' check below gives those beyond them another site. Those it finds none
// for count as idle at the look ("the citezens also drop out of idel to hunt, fasly making them look like they have
// something to do"): other work for them, the ferry's spare. Every epoch: a crowd is as useless after the farms.
#define WK_HUNTERS 4
#define WK_PREY 256
internal struct { ee_unit *a; int n; } g_wkp[WK_PREY];   // the look's animals and the hunters counted on each
internal int g_wkp_n;
internal int wk_animal_site(ee_unit *site) {   // a wild animal, living or its meat
    ee_unit_type *td = site ? unit_get_type_data(site) : NULL;
    ee_player *o = site ? unit_get_owner(site) : NULL;
    return td && o && player_get_index(o) == 0 && unit_type_get_family(td) == KIND_ANIMAL;
}
// one more citizen of the look on site: is it beyond WK_HUNTERS (an animal only)
internal int wk_crowd(ee_unit *site) {
    if (!wk_animal_site(site)) return 0;
    int k = 0;
    while (k < g_wkp_n && g_wkp[k].a != site) k++;
    if (k == g_wkp_n) { if (k == WK_PREY) return 0; g_wkp[k].a = site; g_wkp[k].n = 0; g_wkp_n++; }
    return ++g_wkp[k].n > WK_HUNTERS;
}
// the hunters on animal a: the game's count knows those gathering only - the look's count (wk_crowd) and those it sent
// meanwhile (wk_prey_add) also the ones still on their way: by the game's alone the look sent a dozen to one animal
// and the check took all but four off again, 13091 orders in an hour for one AI
internal int wk_prey_n(ee_unit *a) {
    int k = 0, n = (int)site_gatherers(a);
    while (k < g_wkp_n && g_wkp[k].a != a) k++;
    return k < g_wkp_n && g_wkp[k].n > n ? g_wkp[k].n : n;
}
internal void wk_prey_add(ee_unit *a) {
    if (!wk_animal_site(a)) return;
    int k = 0;
    while (k < g_wkp_n && g_wkp[k].a != a) k++;
    if (k == g_wkp_n) { if (k == WK_PREY) return; g_wkp[k].a = a; g_wkp[k].n = (int)site_gatherers(a); g_wkp_n++; }
    g_wkp[k].n++;
}
internal unsigned g_wkh_crowd[2];   // next animals refused for their crowd, hunters taken off a crowd (the log)
internal ee_unit *__thiscall hook_citizen_next_resource(ee_unit_type *td, ee_unit *u, ee_unit *res, int kind, void *excluded) {
    ee_unit *n = citizen_next_resource(td, u, res, kind, excluded);
    if (!n || unit_get_hp(n) <= 0) return n;
    ee_player *pl = unit_get_owner(u);
    int pi = pl ? player_get_index(pl) : 0;
    if (pi <= 0 || pi >= 16 || !g_players[pi] || is_vanilla_idx(pi)) return n;   // a mod's player only
    ee_unit_type *nt = unit_get_type_data(n);
    ee_player *o = unit_get_owner(n);
    if (!nt || !o || player_get_index(o) != 0 || unit_type_get_family(nt) != KIND_ANIMAL) return n;
    if ((int)site_gatherers(n) >= WK_HUNTERS) { g_wkh_crowd[0]++; return NULL; }
    if (!g_herd_keep || (current_epoch(pl) >= FARM_EPOCH && end_epoch() >= FARM_EPOCH) || wk_hunt_ok(n)) return n;
    g_wkh_source++;
    return NULL;
}
internal void work_install(void) {
    int of = 0, ok = patch_citizen_next_resource(hook_citizen_next_resource, &of);
    if (g_log) fprintf(g_log, "# work: the hunters' next animal hook %d/%d\n", ok, of);
}
// every turn of the player (a young deer dies to 3 hunters within a look's 3 s): its hunters on an animal not to be
// had are given another site by the game's own hand-out (resource_mgr_assign, as the look does) - food first, so the
// meat it carries stays (ITS LOAD FIRST): an animal of the look's list in its territory (built anew), a free bush;
// none, a tree. A halt (game/api unit_halt: the slot cleared, its goals dropped) inside the player's turn crashed the
// game in the movement's code (0x54DBED, read 0) within 10 minutes; an attack order alone left its gathering under it,
// which put the attack back (8 citizens on one young giraffe, again about once a second each)
internal void wk_animals(ee_player *pl, ee_resource_mgr *rm);
internal ee_unit *wk_animal(ee_player *pl, ee_resource_mgr *rm, ee_unit *cu, int ter);
internal unsigned g_wkh_to[3];   // stopped hunters sent to an animal, a bush, a tree (the log)
internal void wk_hunters_check(ee_player *pl, ee_resource_mgr *rm) {
    int herds = g_herd_keep && !(current_epoch(pl) >= FARM_EPOCH && end_epoch() >= FARM_EPOCH);
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    int count = 0, fresh = 0;
    // A CROWD ON ONE ANIMAL: its hunters counted first, by gather and by attack (site_gatherers knows the gathering only)
    static struct { ee_unit *a; int n; } prey_n[WK_PREY];
    int np = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !is_citizen_type(unit_type_get_id(td))) continue;
        ee_unit *prey = wk_hunted(u, unit_get_goal(u));
        if (!prey) continue;
        int k = 0;
        while (k < np && prey_n[k].a != prey) k++;
        if (k == np) { if (np == WK_PREY) continue; prey_n[np].a = prey; prey_n[np++].n = 0; }
        prey_n[k].n++;
    }
    count = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384 && np; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !is_citizen_type(unit_type_get_id(td))) continue;
        ee_unit *prey = wk_hunted(u, unit_get_goal(u));
        if (!prey || !cl_may(u, CLP_ROUTINE, CLW_WORK)) continue;
        int pk = 0;
        while (pk < np && prey_n[pk].a != prey) pk++;
        int crowd = pk < np && prey_n[pk].n > WK_HUNTERS;
        if (!crowd && (!herds || wk_hunt_ok(prey))) continue;
        if (!fresh) { wk_animals(pl, rm); fresh = 1; }   // the list of this moment: the last look's may hold the dead
        ivec2 p = unit_get_tile(u);
        int ter = (int)ter_near(p.x, p.y), k = 0;
        ee_unit *to = ter >= 0 ? wk_animal(pl, rm, u, ter) : NULL;
        if (to == prey) to = NULL;
        if (!to && (to = food_bush(pl, rm, u, 60 * 60))) k = 1;
        if (!to && ter >= 0 && g_tv[idx].n && (to = wk_tree(pl, &g_tv[idx], p, 40, ter))) k = 2;
        if (!to) continue;   // nothing to give it: it goes on
        resource_mgr_assign(rm, u, to);
        wk_prey_add(to);
        if (crowd) { prey_n[pk].n--; g_wkh_crowd[1]++; }   // those beyond the places only: the others stay on it
        else { g_wkh_stopped++; g_wkh_to[k]++; }
    }
}
internal int wk_animal_order(const void *a, const void *b) {
    const struct wk_animal *x = a, *y = b;
    return x->ter != y->ter ? (int)x->ter - (int)y->ter : x->d2 != y->d2 ? x->d2 - y->d2 : (x->u > y->u) - (x->u < y->u);
}
internal void wk_animals(ee_player *pl, ee_resource_mgr *rm) {
    g_wka.built = 1; g_wka.n = 0;
    ee_civ_mgr *cm = resource_mgr_get_owner(rm);
    void *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!bm) return;
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    drops_cached(pl);
    int reach2 = g_hunt_range * g_hunt_range;
    for (int i = 0; i < g_drops[pi].n; i++) {
        const struct drop *d = &g_drops[pi].d[i];
        if (d->food) continue;
        for (struct grid_it it = grid_it_box(d->p.x, d->p.y, g_hunt_range, 1); grid_it_next(&it) && g_wka.n < WK_ANIMALS; ) {
            ee_unit *a = it.u;
            ee_unit_type *td = it.td;
            if (!td || unit_get_resource(a) != 2 || unit_get_amount(a) <= 0 || unit_type_get_family(td) != KIND_ANIMAL) continue;
            if (food_fierce(unit_type_get_id(td)) || !explored_by(a, pl)) continue;   // not on ground it hasn't explored (honest_map: it knew every herd of the map)
            ivec2 pos = unit_get_tile(a);
            int d2 = ivec2_dist2(pos, d->p);
            if (d2 > reach2) continue;
            if (build_mgr_enemy_near(bm, pos.x, pos.y) || site_danger(pl, a, 1)) continue;
            struct wk_animal *e = &g_wka.e[g_wka.n++];
            e->u = a; e->d2 = d2; e->ter = (uint16_t)ter_near(pos.x, pos.y); e->area = food_area(pos);   // food.c AN ANIMAL'S LAND
        }
    }
    qsort(g_wka.e, (size_t)g_wka.n, sizeof *g_wka.e, wk_animal_order);
    wk_herds_keep(pl);   // HERDS KEPT
}
// an animal in territory ter for the citizen cu: the nearest to its drop site with under 3 hunters, on the citizen's land
internal ee_unit *wk_animal(ee_player *pl, ee_resource_mgr *rm, ee_unit *cu, int ter) {
    if (!g_wka.built) wk_animals(pl, rm);
    int lo = 0, hi = g_wka.n;
    while (lo < hi) { int m = (lo + hi) / 2; if ((int)g_wka.e[m].ter < ter) lo = m + 1; else hi = m; }
    short ur = (short)map_area(unit_get_tile(cu), 2);
    for (int k = lo; k < g_wka.n && (int)g_wka.e[k].ter == ter; k++)
        if (g_wka.e[k].area == ur && wk_prey_n(g_wka.e[k].u) < 3) return g_wka.e[k].u;
    return NULL;
}
// a site of resource r in territory ter for the citizen cu at p (land: its land): the nearest mine with a place, a
// field or bush with one, a tree, an animal; *mk, *bk: the mine's, the bush's entry
internal ee_unit *wk_find(ee_player *pl, ee_resource_mgr *rm, struct tv_st *s, ee_unit *cu, ivec2 p, unsigned land, int r, int ter, int home, int *mk, int *bk) {
    *mk = *bk = -1;
    float bd = 0;
    if (r >= 4) {
        for (int k = 0; k < g_wks.nm; k++) {
            struct wk_site *m = &g_wks.mine[k];
            if (m->res != r || m->terr != ter || m->n >= g_wks.cap || site_gatherers(m->u) >= g_wks.cap) continue;
            ivec2 q = unit_get_tile(m->u);
            if (land && area_id(q, 2) && area_id(q, 2) != land) continue;
            float d = (float)ivec2_dist2(p, q);
            if (*mk < 0 || d < bd) { *mk = k; bd = d; }
        }
        return *mk >= 0 ? g_wks.mine[*mk].u : NULL;
    }
    if (r == 3) return ter == home ? wk_tree(pl, s, p, 60, ter) : wk_tree(pl, s, g_ter.t[ter].c, 40, ter);
    for (int k = 0; k < g_wks.nb; k++) {   // a field (a farmer; cost 0: before any bush), a bush (WK_PATCH of its citizens at most)
        struct wk_site *b = &g_wks.bush[k];
        if (b->terr != ter || b->n >= b->cap || (b->cap > 1 && unit_get_amount(b->u) <= 0)) continue;
        ivec2 q = unit_get_tile(b->u);
        if (land && area_id(q, 2) && area_id(q, 2) != land) continue;
        float d = (float)ivec2_dist2(p, q) * (float)(b->cost * b->cost);
        if (*bk < 0 || d < bd) { *bk = k; bd = d; }
    }
    if (*bk >= 0) return g_wks.bush[*bk].u;
    // an animal to hunt, by food_animal's rules - in the territory asked for (from the Stone Age the hunt is most of the
    // food: with the animals of its own territory only an AI had 43 citizens for 70 at minute 30)
    return wk_cost(s, (unsigned)ter) ? wk_animal(pl, rm, cu, ter) : NULL;
}
// the territories a citizen of territory home may be given, nearest first by the graph, over land its citizens may work in
internal int wk_reach(struct tv_st *s, int home, short *out, int max) {
    static uint8_t seen[WK_T];
    int n = 0, head = 0, tn = s->n < WK_T ? s->n : WK_T;
    if (home < 0 || home >= tn) return 0;
    memset(seen, 0, (size_t)tn);
    static short queue[WK_T];
    int qn = 0;
    queue[qn++] = (short)home; seen[home] = 1;
    while (head < qn && n < max) {
        int r = queue[head++];
        if (r != home) out[n++] = (short)r;
        for (int k = 0; k < g_ter.t[r].nbn; k++) {
            unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
            if (o >= (unsigned)tn || seen[o] || g_ter.t[o].water || !wk_cost(s, o)) continue;
            seen[o] = 1; queue[qn++] = (short)o;
        }
    }
    return n;
}
// THE FERRY SERVICE (expand.c), by land: the free places at the mines of the resources it wants more citizens on
// (want over have) and the citizens there on wood or idle - work.c sends a citizen to sites on its own land only, so
// free mines across the water stay free however many cut wood at home. at: the mine with the most free places.
// TO THE TREES ACROSS ([ai_base] forest_across; the player 2026-11-19, after IDLE ABROAD below had left about 50
// citizens an AI idle at minute 100 of a Stone Age game, its island's food and wood gone: yes to "free islands with
// trees, or with no drop site yet" pulling them over): while citizens stand idle after the look, a land with none of
// its idle on it offers wood too - WK_TREE_PLACES a territory its citizens may work with a tree in it (wk_tree from its
// middle, once a minute) less its wood cutters on that land - and its bushes' free places; the ferry takes idle
// citizens only to those (wood), to a land with a drop site of its or a foundation waiting for builders (build.c lays
// a Settlement by such trees on a land with no drop site of its). idle: its idle citizens on the land.
// WOOD ACROSS (the player 2026-11-20: "p3 is starved for wood and has none left on its home island, but it also dosnt
// bring its citezens to the island it controls with wood reserves" - his P3 at minute 349: 42 wood cutters wanted, 0
// at work, 111 on food against 57 wanted, nobody idle: those who found no tree were set to food, so neither the trees
// across, the land's spare nor the ferry's pick saw them): while it is short of wood cutters the trees across count as
// with idle citizens, and those beyond their resource's share are spare and taken as the idle are.
#define WK_LANDS 16
#define WK_TREE_PLACES 8
#define WK_TREE_EVERY 60000
struct wk_land { unsigned land; int free, spare, wood, idle; ivec2 at; };
internal struct { uint32_t t; int n; struct { unsigned land; int ter; } e[64]; } g_wkt[16];   // the territories with a tree, at the last minute's look
internal struct { uint32_t t; int n; struct wk_land l[WK_LANDS]; } g_wkl[16];
internal struct wk_land *wk_land_at(unsigned idx, unsigned land) {
    typeof(&g_wkl[0]) L = &g_wkl[idx & 15];
    int k = 0;
    while (k < L->n && L->l[k].land != land) k++;
    if (k == L->n) { if (k == WK_LANDS) return NULL; memset(&L->l[k], 0, sizeof L->l[k]); L->l[k].land = land; L->n++; }
    return &L->l[k];
}
// citizens of land `land` free to cross - idle first, then those on wood - nearest to `toward`, into out (at most max)
internal int expand_has(ee_player *pl, ee_unit *u);   // expand.c: a passenger of one of its ferries
internal int wk_ferry_pick(ee_player *pl, unsigned land, ivec2 toward, ee_unit **out, int max) {
    static ee_unit *c[WK_CITS];
    static int d[WK_CITS];
    int nc = 0, count = 0, got = 0, over[7];
    struct wk_st *w = &g_wk[player_get_index(pl) & 15];
    for (int r = 0; r < 7; r++) over[r] = FOREST_ACROSS(player_get_index(pl) & 15) && w->want[3] > w->have[3] ? w->have[r] - w->want[r] : 0;   // WOOD ACROSS
    for (ee_unit *u = player_get_units(pl); u && count < 16384 && nc < WK_CITS; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !is_citizen_type(unit_type_get_id(td))) continue;
        if (unit_get_frozen(u) || unit_get_ai_held(u) || unit_get_busy(u) || expand_has(pl, u) || area_id(unit_get_tile(u), 2) != land) continue;
        ee_goal *g = unit_get_goal(u);
        ee_unit *site = g && !strcmp(rtti_name(g), "EEUGGather@@") ? goal_get_site(g) : NULL;
        int crowd = site && wk_animal_site(site) && (int)site_gatherers(site) > WK_HUNTERS;   // A CROWD ON ONE ANIMAL: as idle
        int rs = site ? unit_get_resource(site) : -1, extra = rs >= 2 && rs < 7 && rs != 3 && over[rs] > 0;   // WOOD ACROSS: beyond its resource's share
        if (g && !crowd && !extra && !(site && unit_get_amount(site) > 0 && rs == 3)) continue;   // idle, or on wood
        if (extra) over[rs]--;
        c[nc] = u; d[nc++] = ivec2_dist2(unit_get_tile(u), toward) / 4 + (g ? 0x10000000 : 0);
    }
    while (got < max) {
        int b = -1;
        for (int k = 0; k < nc; k++) if (c[k] && (b < 0 || d[k] < d[b])) b = k;
        if (b < 0) break;
        out[got++] = c[b]; c[b] = NULL;
    }
    return got;
}
// ITS LOAD FIRST, the drop-off: a citizen to be moved to another resource while it carries a load is sent to unload
// first, as the player's right-click on a drop site does (game/api unit_drop_off: it keeps its gathering), and given
// its new site by the first turn of its player that finds it empty - on its way back to its old resource. The game's
// own assignment (0x48C81C) sets a load to 0 when the new site is of another resource. Given up after WK_DROP_WAIT
// (still loaded: the look tries again) or when it takes another order meanwhile
#define WK_DROPS 128
#define WK_DROP_WAIT 60000
internal struct { int n; struct { int id, site; uint32_t t; uint8_t r; } e[WK_DROPS]; } g_wkdrop[16];
internal unsigned g_wkd_sent[16], g_wkd_moved[16], g_wkd_lapsed[16];   // the log
internal int wk_drop_at(unsigned idx, int id) {
    for (int k = 0; k < g_wkdrop[idx].n; k++) if (g_wkdrop[idx].e[k].id == id) return k;
    return -1;
}
internal int wk_drop(unsigned idx, ee_unit *u, ee_unit *site, int r, uint32_t now) {
    typeof(&g_wkdrop[0]) D = &g_wkdrop[idx];
    int id = unit_get_serial(u);
    if (D->n >= WK_DROPS || wk_drop_at(idx, id) >= 0 || !unit_drop_off(u)) return 0;
    D->e[D->n].id = id; D->e[D->n].site = unit_get_serial(site); D->e[D->n].t = now ? now : 1; D->e[D->n].r = (uint8_t)r; D->n++;
    g_wkd_sent[idx]++;
    return 1;
}
internal void wk_drops_check(ee_player *pl, ee_resource_mgr *rm, unsigned idx, uint32_t now) {
    typeof(&g_wkdrop[0]) D = &g_wkdrop[idx];
    int m = 0;
    for (int k = 0; k < D->n; k++) {
        ee_unit *u = unit_by_id((unsigned)D->e[k].id);
        if (!u || unit_get_serial(u) != D->e[k].id || unit_get_hp(u) <= 0 || unit_get_owner(u) != pl) continue;
        ee_goal *g = unit_get_goal(u);
        if (!g || !goal_is_gather(g) || !cl_may(u, CLP_ROUTINE, CLW_WORK)) continue;   // another order meanwhile: that one stands
        if (unit_get_amount(u) > 0) {
            if (now - D->e[k].t < WK_DROP_WAIT) D->e[m++] = D->e[k]; else g_wkd_lapsed[idx]++;
            continue;
        }
        ee_unit *site = object_by_id((unsigned)D->e[k].site);
        if (!site) continue;   // gone meanwhile: the next look finds it work
        resource_mgr_assign(rm, u, site);
        g_wkd_moved[idx]++;
    }
    D->n = m;
}
internal void work_tick(ee_player *pl, ee_resource_mgr *rm, uint32_t now) {
    static struct wk_cit cit[WK_CITS];
    struct wk_site *mine = g_wks.mine, *bush = g_wks.bush;
    static struct wk_home nhome[WK_CITS];
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct wk_st *w = &g_wk[idx];
    struct tv_st *s = &g_tv[idx];
    if (now < w->t) { memset(w, 0, sizeof *w); g_wkdrop[idx].n = 0; g_wkd_sent[idx] = g_wkd_moved[idx] = g_wkd_lapsed[idx] = 0; }
    wk_drops_check(pl, rm, idx, now);   // ITS LOAD FIRST, the drop-off
    wk_hunters_check(pl, rm);   // THE HUNTER'S OWN NEXT ANIMAL
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
    g_wks.cap = cap;
    for (int k = 0; k < g_ter.nsite; k++) {   // what the worked territories offer: the mines and the bushes of the map's sites
        int r = g_ter.site[k].res, x = g_ter.site[k].x, y = g_ter.site[k].y;
        if (r != 2 && (r < 4 || r > 6)) continue;
        unsigned tr = ter_near(x, y);
        int c = wk_cost(s, tr);
        ee_unit *u = c ? wk_tile_site(map, size, x, y, r) : NULL;
        // nor one citizens_safety would send them back from (raiders seen or remembered there, its citizens hurt there,
        // enemy towers on the way from its drop site): sent there, sent back, sent there again
        if (!u || !explored_by(u, pl) || wk_danger(idx, iv2(x, y)) || site_danger_res(pl, u, r == 2, r)) continue;
        struct wk_site *e = r == 2 ? (nb < WK_BUSHES ? &bush[nb++] : NULL) : (nm < WK_MINES ? &mine[nm++] : NULL);
        if (!e) continue;
        e->u = u; e->terr = (uint16_t)tr; e->res = (uint8_t)r; e->n = 0; e->cost = (uint8_t)c; e->cap = WK_PATCH;
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
    int count = 0, nhn = 0;
    g_wkp_n = 0; w->crowd = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {   // its citizens: what each does
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !is_citizen_type(unit_type_get_id(td))) continue;
        // ITS TERRITORY: the one it had, else where it stands; one that may not be worked: the nearest it holds
        int home = -1, hk = -1;
        if (nhn < WK_CITS) {
            int o = nhn < w->nh && w->home[nhn].u == u ? nhn : -1;
            for (int j = 0; o < 0 && j < w->nh; j++) if (w->home[j].u == u) o = j;
            ivec2 hp = unit_get_tile(u);
            home = o >= 0 && w->home[o].id == (int)unit_get_serial(u) ? w->home[o].ter : (int)ter_near(hp.x, hp.y);
            if (home >= 0 && home < s->n) {   // across the water (a ferry put it off there): the territory it stands in
                unsigned hl = area_id(g_ter.t[home].c, 2), ul = area_id(hp, 2);
                if (hl && ul && hl != ul) home = (int)ter_near(hp.x, hp.y);
            }
            if (home < 0 || home >= s->n || !wk_cost(s, (unsigned)home)) {
                int b = -1, bd = 0;
                for (int r = 0; r < s->n; r++) {
                    if (g_ter.t[r].water || s->t[r].role == TVR_NONE || !wk_cost(s, (unsigned)r)) continue;
                    int d = ivec2_dist2(g_ter.t[r].c, hp);
                    if (b < 0 || d < bd) { b = r; bd = d; }
                }
                if (b >= 0) { if (o >= 0) w->retreats++; home = b; }
            }
            hk = nhn; nhome[nhn].u = u; nhome[nhn].id = (int)unit_get_serial(u); nhome[nhn++].ter = (short)home;
        }
        if (unit_get_frozen(u) || unit_get_ai_held(u) || unit_get_busy(u) || expand_has(pl, u) || !cl_may(u, CLP_ROUTINE, CLW_WORK)) { w->busy++; continue; }   // ... or another's claim (claims.c: its safety sent it off, the militia's, HOLDING THE GROUND's)   // held for a landing or an outpost, building, a ferry's passenger (expand.c)
        int dk = wk_drop_at(idx, (int)unit_get_serial(u));
        if (dk >= 0) { w->have[g_wkdrop[idx].e[dk].r]++; continue; }   // on its way to unload: counted at its new resource, not moved again
        ee_goal *g = unit_get_goal(u);
        int gather = g && !strcmp(rtti_name(g), "EEUGGather@@");
        if (g && !gather) { w->busy++; continue; }   // building, repairing, walking on an order
        ee_unit *site = gather ? goal_get_site(g) : NULL;
        int field = site && unit_get_type_data(site) && unit_type_get_id(unit_get_type_data(site)) == UT(B_FARM) && unit_get_hp(site) > 0;
        int r = field ? 2 : site && unit_get_amount(site) > 0 ? unit_get_resource(site) : 0;
        if (r < 2 || r > 6) { r = 0; site = NULL; }
        if (r == 2 && wk_crowd(site)) { r = 0; site = NULL; w->crowd++; }   // A CROWD ON ONE ANIMAL: one beyond its places is idle
        if (site && wk_danger(idx, unit_get_tile(site))) { r = 0; site = NULL; w->pulled++; }   // its site under an enemy tower it has seen: to another
        if (nc >= WK_CITS) { w->busy++; continue; }
        cit[nc].u = u; cit[nc].site = site; cit[nc].home = (short)home; cit[nc].hk = (short)hk; cit[nc++].res = (uint8_t)r;
        if (r) w->have[r]++; else w->idle++;
        for (int k = 0; r >= 4 && k < nm; k++) if (mine[k].u == site) { if (mine[k].n < 255) mine[k].n++; break; }
        for (int k = 0; r == 2 && k < nb; k++) if (bush[k].u == site) { if (bush[k].n < 255) bush[k].n++; break; }
    }
    memcpy(w->home, nhome, (size_t)nhn * sizeof *nhome); w->nh = nhn;
    g_wks.nm = nm; g_wks.nb = nb;
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
        // THE POOL (the player 2026-11-15: "i think we need a pool of unneeded citezens ... they do building -> populate
        // mines -> work exxess food and wood -> stash if in the way"): what each resource needs in citizens is
        // wk_needs' count - what is spent and saved for over what a citizen brings - and a quarter more; the citizens
        // beyond the sum of it are the pool. They work all the same: builders are taken from the free ones, the
        // mines' places and their Settlements are filled first (the shares below, garrison.c), the rest cuts wood and
        // farms beyond the need - and when the map is full they are the ones the shelf takes (bunker.c: wk_pool).
        for (int r = 2; r < 7; r++) {
            int c = w->by_need ? (int)(wt[r] * 1.25f) + 2 : nc;
            w->need_c[r] = r >= 4 && c > w->places[r] ? w->places[r] : c;
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
        // THE PATCHES FIRST (the player 2026-11-14: "the ai has a very slow start right now - the ai should focus a bit more on
        // the natural food resource patches early on, trying to fill them up with 6 citezens all time before anething
        // else"): food wants at least the places of its fields and of the patches with something left, WK_PATCH each;
        // what that takes comes off wood first, then the mines - but wood keeps a quarter of the citizens (3 at least:
        // with all 15 on food at minute 5 the wood came in at 9 a minute, 935 food lay in the bank and the first Barracks
        // stood at minute 18 to 28), and not while food piles up (WK_PATCH_BANK in the bank)
        int fplaces = 0;
        for (int k = 0; k < nb; k++) fplaces += bush[k].cap > 1 ? (unit_get_amount(bush[k].u) > 0 ? bush[k].cap : 0) : 1;
        if (fplaces > nc) fplaces = nc;
        if (stock[2] >= WK_PATCH_BANK) fplaces = 0;
        int wood_keep = nc / 4 > 3 ? nc / 4 : 3;
        for (int r = 3; r < 7 && w->want[2] < fplaces; r++) {
            int spare = r == 3 ? w->want[r] - wood_keep : w->want[r];
            int take = fplaces - w->want[2] < spare ? fplaces - w->want[2] : spare;
            if (take <= 0) continue;
            w->want[r] -= take; w->want[2] += take;
        }
    }
    int moves = 0, xfers = 0;
    g_wka.built = 0;   // the look's animals anew
    for (int pass = 0; pass < 2; pass++)   // the idle ones first, then those of a resource over its share
        for (int i = 0; i < nc; i++) {
            struct wk_cit *c = &cit[i];
            if (pass ? (!c->res || w->have[c->res] - w->want[c->res] < 2 || moves >= WK_MOVES) : c->res != 0) continue;
            ivec2 p = unit_get_tile(c->u);
            unsigned land = area_id(p, 2);
            int tried[7] = { 0 }, done = 0;
            // ITS LOAD FIRST (the player 2026-11-18: "if the work is another resource what it currently holds is lost, i
            // noticed that in other areas alredy. if its asigned a new task on a difrent resource it should deliver what
            // it holds first to avoid waste"): a citizen carries what it gathered (unit +0x40, of the resource +0x38) until
            // its drop-off, and a gather of another resource starts it at 0. An idle one with a load asks for its load's
            // resource first; one to be moved off its resource while it carries some of it is sent to unload first and
            // moved once empty (wk_drop, the drop-off above; its gather goal's gathering flag set to 0 didn't send it
            // there: it went on gathering) - with no drop site to take its load it stays until a look finds it empty
            int load = unit_get_amount(c->u), lr = unit_get_resource(c->u);
            if (load <= 0 || lr < 2 || lr > 6) load = 0;
            for (int attempt = 0; attempt < 5 && !done; attempt++) {
                int r = 0, most = pass ? 1 : -0x7FFFFFFF;   // a move only to a resource 2 or more under its share
                for (int q = 2; q < 7; q++) if (!tried[q] && q != c->res && w->want[q] - w->have[q] > most) { most = w->want[q] - w->have[q]; r = q; }
                if (!pass && !attempt && load && w->want[lr] > 0) r = lr;   // ITS LOAD FIRST
                if (!r) break;
                tried[r] = 1;
                // ITS TERRITORY: a site at home; none there: another territory, while the look has changes left
                int mk = -1, bk = -1, home = c->home;
                ee_unit *site = home >= 0 && home < s->n ? wk_find(pl, rm, s, c->u, p, land, r, home, home, &mk, &bk) : NULL;
                if (!site && xfers < WK_XFER && home >= 0 && home < s->n) {
                    short to[WK_XFER_T];
                    int tn = wk_reach(s, home, to, WK_XFER_T);
                    for (int k = 0; k < tn && !site; k++) {
                        site = wk_find(pl, rm, s, c->u, p, land, r, to[k], home, &mk, &bk);
                        if (!site) continue;
                        c->home = to[k]; xfers++; w->xfers++;
                        if (c->hk >= 0) w->home[c->hk].ter = to[k];
                    }
                }
                if (!site) { w->none[r]++; w->food_short += r == 2; continue; }
                ee_goal *cg = unit_get_goal(c->u);
                int drop = load && lr != r && cg && goal_is_gather(cg);   // ITS LOAD FIRST: sent to unload, moved once empty
                if (drop && !wk_drop(idx, c->u, site, r, now)) { w->delivering++; break; }   // no drop site for it: held until a look finds it empty
                if (load && lr != r && !drop) w->load_lost += (unsigned)load;   // idle with a load, none of its resource to be had
                if (bk >= 0) bush[bk].n++;
                if (!drop) { resource_mgr_assign(rm, c->u, site); wk_prey_add(site); }
                if (c->res) { w->have[c->res]--; w->moved++; moves++; } else { w->idle--; w->assigned++; }
                w->have[r]++;
                if (mk >= 0 && mine[mk].n < 255) mine[mk].n++;
                c->res = (uint8_t)r; c->site = site;
                done = 1;
            }
        }
    {   // THE FERRY SERVICE: by land
        typeof(&g_wkl[0]) L = &g_wkl[idx];
        int most[WK_LANDS] = { 0 };
        L->n = 0; L->t = now ? now : 1;
        for (int k = 0; k < nm; k++) {
            int fr = (int)g_wks.cap - mine[k].n;
            // IDLE ABROAD (the player 2026-11-18: his AI's island bare of food and wood, 232 citizens idle while the mines
            // across the water had 22 places free - "they should be agressivly shiped off to other territoes instead"):
            // while citizens stand idle after the look, every free place counts, not only those of a resource under its share
            if (fr <= 0 || (w->want[mine[k].res] <= w->have[mine[k].res] && w->idle <= 0)) continue;
            struct wk_land *l = wk_land_at(idx, area_id(unit_get_tile(mine[k].u), 2));
            if (!l || !l->land) continue;
            l->free += fr;
            if (fr > most[l - L->l]) { most[l - L->l] = fr; l->at = unit_get_tile(mine[k].u); }
        }
        int wood_on[WK_LANDS] = { 0 }, over[7];
        for (int r = 0; r < 7; r++) over[r] = w->have[r] - w->want[r];
        for (int i = 0; i < nc; i++) {
            // WOOD ACROSS: those beyond their resource's share are spare too, while it is short of wood cutters
            int extra = FOREST_ACROSS(idx) && w->want[3] > w->have[3] && cit[i].res >= 2 && cit[i].res < 7 && cit[i].res != 3 && over[cit[i].res] > 0;
            if (cit[i].res != 0 && cit[i].res != 3 && !extra) continue;
            if (extra) over[cit[i].res]--;
            struct wk_land *l = wk_land_at(idx, area_id(unit_get_tile(cit[i].u), 2));
            if (!l || !l->land) continue;
            l->spare++;
            if (cit[i].res == 0 || extra) l->idle++; else wood_on[l - L->l]++;   // WOOD ACROSS: the extra ones as idle (the ferry fills the trees' places from those)
        }
        if (FOREST_ACROSS(idx) && (w->idle > 0 || w->want[3] > w->have[3])) {   // TO THE TREES ACROSS (WOOD ACROSS: short of wood cutters too)
            typeof(&g_wkt[0]) T = &g_wkt[idx];
            if (!T->t || now < T->t || now - T->t >= WK_TREE_EVERY) {
                T->t = now ? now : 1; T->n = 0;
                for (int r = 0; r < s->n && T->n < 64; r++) {
                    if (g_ter.t[r].water || !wk_cost(s, (unsigned)r)) continue;
                    ee_unit *tree = wk_tree(pl, s, g_ter.t[r].c, 24, r);
                    if (!tree) continue;
                    T->e[T->n].land = area_id(unit_get_tile(tree), 2); T->e[T->n++].ter = r;
                }
            }
            int trees[WK_LANDS] = { 0 };
            for (int k = 0; k < T->n; k++) {
                int j = 0;
                while (j < L->n && L->l[j].land != T->e[k].land) j++;
                if (j < L->n && L->l[j].idle) continue;   // its own idle there: no wood for them
                struct wk_land *l = j < L->n ? &L->l[j] : wk_land_at(idx, T->e[k].land);
                if (!l || !l->land) continue;
                trees[l - L->l] += WK_TREE_PLACES;
                if (!l->at.x && !l->at.y) l->at = g_ter.t[T->e[k].ter].c;
            }
            for (int k = 0; k < nb; k++) {
                int fr = bush[k].cap - bush[k].n;
                if (fr <= 0) continue;
                struct wk_land *l = wk_land_at(idx, area_id(unit_get_tile(bush[k].u), 2));
                if (!l || !l->land || l->idle) continue;
                l->wood += fr;
                if (!l->at.x && !l->at.y) l->at = unit_get_tile(bush[k].u);
            }
            for (int k = 0; k < L->n; k++) { int tw = trees[k] - wood_on[k]; if (tw > 0) L->l[k].wood += tw; }
        }
    }
    if (g_log && (!w->log_t || now - w->log_t >= 60000)) {
        w->log_t = now ? now : 1;
        int worked = 0;
        for (int r = 0; r < s->n; r++) worked += !g_ter.t[r].water && s->t[r].bld >= 1 && s->t[r].state != TV_UNSAFE;
        log_locked("# work: P%u at %.1f min: %d citizens free for work (%d idle, %d more building or held), shares %s | at food %d wood %d stone %d gold %d iron %d | wanted %d %d %d %d %d | "
                   "%d mines and %d bushes in reach of %d worked territories (places stone %d gold %d iron %d) | so far %u sent to work, %u moved, %u given another territory, %u back from one not to be worked, no site for food %u wood %u stone %u gold %u iron %u | its load first: %u sent to unload before a move (%u moved once empty, %u still loaded after a minute), %u moves held for a load, %u lost | a crowd on one animal: %d of the idle, so far %u next animals refused, %u hunters taken off\n",
                   idx, now / 60000.0, nc, w->idle, w->busy, w->by_need ? "by need" : "by the plain rule", w->have[2], w->have[3], w->have[4], w->have[5], w->have[6], w->want[2], w->want[3], w->want[4], w->want[5], w->want[6],
                   nm, nb, worked, w->places[4], w->places[5], w->places[6], w->assigned, w->moved, w->xfers, w->retreats, w->none[2], w->none[3], w->none[4], w->none[5], w->none[6], g_wkd_sent[idx], g_wkd_moved[idx], g_wkd_lapsed[idx], w->delivering, w->load_lost, w->crowd, g_wkh_crowd[0], g_wkh_crowd[1]);
        static uint32_t wild_t;
        if (g_autotest && (!wild_t || now < wild_t || now - wild_t >= 60000)) {   // HERDS KEPT's measure: the map's living food animals once a minute
            wild_t = now ? now : 1;
            int alive = 0, young = 0, inh = 0, carc = 0;
            for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
                if (unit_type_get_family(it.td) != KIND_ANIMAL || unit_get_resource(it.u) != 2) continue;
                if (unit_get_hp(it.u) <= 0) { carc += unit_get_amount(it.u) > 0; continue; }
                alive++; young += unit_get_scale(it.u) < 1.0f; inh += unit_get_herd(it.u) != NULL;
            }
            log_locked("# wild: at %.1f min: %d food animals alive (%d young, %d in a herd), %d carcasses with food | herd_keep %d: left out %u times for their herd, %u as young; %u hunters given another site on their own next animal (an animal %u, a bush %u, a tree %u), %u next animals refused at the game's choice\n",
                       now / 60000.0, alive, young, inh, carc, g_herd_keep, g_wkh_kept, g_wkh_young, g_wkh_stopped, g_wkh_to[0], g_wkh_to[1], g_wkh_to[2], g_wkh_source);
        }
    }
}
// the citizens it found without work at its last look, -1 while it does not run (the game's computer player on)
internal int work_idle(unsigned idx) {
    struct wk_st *w = &g_wk[idx & 15];
    uint32_t now = game_time();
    return w->t && now >= w->t && now - w->t <= 4 * WK_EVERY ? w->idle : -1;
}
internal void work_new_game(void) { memset(g_wk, 0, sizeof g_wk); memset(g_wn, 0, sizeof g_wn); memset(g_wkd, 0, sizeof g_wkd); }
// THE POOL for bunker.c's shelf: the citizens beyond what its resources need - sp[2], sp[3] those of them on food and
// on wood (the ones a fortress may take), the return their sum; under 0: that many citizens short of the need (the
// stashed are wanted back). 0 while the shares do not go by need yet or work.c has not looked for 30 s.
internal int wk_pool(unsigned idx, int sp[7]) {
    struct wk_st *w = &g_wk[idx & 15];
    uint32_t now = game_time();
    for (int r = 0; r < 7; r++) sp[r] = 0;
    if (!w->by_need || !w->t || now < w->t || now - w->t > 30000) return 0;
    int need = 0;
    for (int r = 2; r < 7; r++) need += w->need_c[r];
    int tot = w->n - need;
    if (tot <= 0) return tot;
    for (int r = 2; r < 4; r++) { int v = w->have[r] - w->need_c[r]; sp[r] = v < 0 ? 0 : v > tot ? tot : v; }
    if (sp[2] + sp[3] > tot) sp[3] = tot - sp[2];
    return sp[2] + sp[3];
}
