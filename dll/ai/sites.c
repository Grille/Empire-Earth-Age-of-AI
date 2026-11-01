// SITES: the resource manager's site searches - mines (0x48C204), food (0x48BBA8), wood - and its field and hunt
// lists, hooked. The originals take the FIRST acceptable site in zone order, anywhere on the map, and only check for
// enemies at the site itself - the game's pathfinding then walks citizens straight through enemy defences
// (observed: red mining iron under yellow's towers 25 tiles away, losing its workforce). Here a site behind known
// enemy towers is refused (avoid_towers_citizens, attack.c site_danger), a mine is the free one nearest to a drop
// site (mine_choice), a bush the one nearest to a drop site (food_plan, food.c), an overseas mine search gets only
// a mine overseas (overseas_mines). (safe_sites - the nearest site to the base within site_max_distance, the
// leftover pass only to mines short of workers - removed: docs/experiments.md.) Included by eeaimod.c before econ.c.

internal int g_site_max_dist = 20;   // tiles from the base, ageofai.ini [ai_base] site_max_distance (econ.c: the wild food counted)

internal int site_danger(ee_player *pl, ee_unit *site, int food);   // attack.c: known enemy towers on the way
internal int site_danger_res(ee_player *pl, ee_unit *site, int food, int res);   // attack.c: res 2 food .. 6 iron
internal ee_unit *food_near_pick(ee_resource_mgr *rm, int near_base, ee_unit *unit);   // food.c (food_plan NEAR)
internal unsigned g_site_refused[16];   // dangerous sites the searches refused (logged by attack.c)

internal int route_safe(void *bm, ivec2 base, ivec2 to) {
    ivec2 d = ivec2_sub(to, base);
    int len = (int)sqrtf((float)ivec2_len2(d));
    for (int t = 3; t < len; t += 2)   // skip the first tiles: our own base
        if (build_mgr_enemy_near(bm, base.x + d.x * t / len, base.y + d.y * t / len)) return 0;
    return 1;
}

// Drop sites of a player from the unit grid: Town Center, Capitol, Settlement (also the single player one) take
// everything, Granary (also the improved one) only food. Up to `max` entries.
struct drop { ivec2 p; int food; ee_unit *u; };   // a drop site: its tile, a granary (food only), the building
internal int drop_sites(ee_player *pl, struct drop *out, int max) {
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid) return 0;
    int n = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (!td || unit_get_owner(u) != pl || n >= max) continue;
        int t = unit_type_get_id(td), food = t == UT(B_GRANARY) || t == UT(B_GRANARY_IMPROVED);
        if (!food && !UT_IN(t, UT_TOWNS, UT_X_SINGLE_PLAYER_SETTLEMENT)) continue;
        out[n].p = unit_get_tile(u); out[n].food = food; out[n].u = u;
        n++;
    }
    return n;
}
internal int drop_dist(const struct drop *d, int nd, ivec2 p, int food) {   // whole tiles to the nearest
    int best = 999;
    for (int i = 0; i < nd; i++) {
        if (d[i].food && !food) continue;
        int r = (int)sqrtf((float)ivec2_dist2(d[i].p, p));
        if (r < best) best = r;
    }
    return best;
}

// MINE CHOICE (ageofai.ini [ai_base] mine_choice, default on): the original mine search takes the first acceptable mine in
// reverse zone order - with mines next to its town centers free, P2 sent 20 citizens to iron 35 tiles from any drop
// site (3-5 iron per worker-minute against 10-12 for wood next door). Mines are practically endless (300000), so the
// walk decides the income: take the free mine nearest to one of the AI's drop sites (Town Center, Capitol,
// Settlement) instead, with the original's checks (active zone, explored, gatherer cap, no enemy near, the unit's
// continent) and no known enemy on the straight way from that drop site. Nothing found -> the original search.
internal int g_mine_choice = 1;
// MINE CROWDS ([ai_base] mine_crowds, default on; the player 2026-10-27 on the save "standing army": very many
// villagers overassigned to mines): the game counts a mine's gatherers (0x52E772, from the unit's +0x1F8 the site it
// works, set when it starts) only while they work - at most the cap (6); the citizens walking there or waiting beside
// it for a place are not counted, so every search (the game's and select_mine_near_drop) saw the mine free and sent
// more: P6 at minute 595 had 86 citizens on one gold mine, 33 and 30 on two others, while dozens of its mines had
// nobody. Here a mine counts as full when its gatherer count or the AI's own citizens aiming at it (their gather goal's
// site, counted every 2 s, a pick added at once) reach the cap, and every 10 s the citizens beyond the cap at a mine
// (the farthest from it first, 20 at most) go to the nearest free mine of the same kind.
internal int g_mine_crowds = 1;
internal struct aim_st { uint32_t t; int n; ee_unit *s[512]; uint16_t c[512]; } g_aim[16];
internal void aim_count(ee_player *pl, unsigned pi) {
    struct aim_st *a = &g_aim[pi & 15];
    uint32_t now = game_time();
    if (a->t && now >= a->t && now - a->t < 2000) return;
    a->t = now ? now : 1;
    a->n = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || !is_citizen_type(unit_type_get_id(it.td))) continue;
        ee_goal *g = unit_get_goal(u);
        ee_unit *s = g && !strcmp(rtti_name(g), "EEUGGather@@") ? goal_get_site(g) : NULL;
        int r = s ? unit_get_resource(s) : 0;
        if (r < 4 || r > 6) continue;
        int k;
        for (k = 0; k < a->n && a->s[k] != s; k++) ;
        if (k == a->n) { if (a->n == 512) continue; a->s[k] = s; a->c[k] = 0; a->n++; }
        a->c[k]++;
    }
}
internal unsigned aim_of(unsigned pi, ee_unit *s) {
    struct aim_st *a = &g_aim[pi & 15];
    for (int k = 0; k < a->n; k++) if (a->s[k] == s) return a->c[k];
    return 0;
}
internal void aim_add(unsigned pi, ee_unit *s) {
    struct aim_st *a = &g_aim[pi & 15];
    int k;
    for (k = 0; k < a->n && a->s[k] != s; k++) ;
    if (k == a->n) { if (a->n == 512) return; a->s[k] = s; a->c[k] = 0; a->n++; }
    a->c[k]++;
}
internal struct { ee_player *pl; uint32_t t; int n; struct drop d[64]; } g_drops[16];
internal void drops_cached(ee_player *pl) {
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    uint32_t now = game_time();
    if (g_drops[pi].pl != pl || now < g_drops[pi].t || now - g_drops[pi].t > 5000) {   // cached 5 s
        g_drops[pi].pl = pl; g_drops[pi].t = now;
        g_drops[pi].n = drop_sites(pl, g_drops[pi].d, 64);
    }
}
// the nearest of player pi's cached drop sites to p (food: granaries count too): its squared distance, 0x7FFFFFFF
// none; *k its index (k may be NULL) - the first of equally near ones
internal int drop_nearest(unsigned pi, ivec2 p, int food, int *k) {
    int best = 0x7FFFFFFF;
    for (int i = 0; i < g_drops[pi].n; i++) {
        const struct drop *d = &g_drops[pi].d[i];
        int d2 = ivec2_dist2(d->p, p);
        if ((d->food && !food) || d2 >= best) continue;
        best = d2;
        if (k) *k = i;
    }
    return best;
}
internal int nearest_drop(ee_player *pl, ivec2 p, int food, ivec2 *out) {   // its tile into *out; 0: none
    drops_cached(pl);
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    int k = -1;
    if (drop_nearest(pi, p, food, &k) == 0x7FFFFFFF) return 0;
    *out = g_drops[pi].d[k].p;
    return 1;
}
internal void *refuse_danger(ee_resource_mgr *rm, void *site, int food, int res) {
    ee_player *pl = resource_mgr_player(rm);
    if (!site || !site_danger_res(pl, site, food, res)) return site;
    g_site_refused[(unsigned)player_get_index(pl) & 15]++;
    return NULL;
}
// minecheck P type: every mine of that resource in P's zones and the first check of select_mine_near_drop that
// refuses it (tests)
internal void cmd_minecheck(char *out, const char *args) {
    int p = 0, type = 5;
    sscanf(args, "%d %d", &p, &type);
    ee_player *pl = p > 0 && p < 16 ? g_players[p] : NULL;
    ee_civ_mgr *cm = pl ? player_get_civ_mgr(pl) : NULL;
    void *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!bm) { REPLY("error: no AI P%d\n", p); return; }
    drops_cached(pl);
    int nd = g_drops[p].n;
    const struct drop *d = g_drops[p].d;
    unsigned cap = table_value(0x16);
    ee_zone **zb = player_get_zones_begin(pl); ee_zone **ze = player_get_zones_end(pl);
    REPLY("P%d type %d, %d drop sites:", p, type, nd);
    for (int i = 0; i < nd; i++) REPLY(" %d,%d%s", d[i].p.x, d[i].p.y, d[i].food ? "g" : "");
    REPLY("\n");
    for (int z = 0; zb && zb + z < ze; z++) {
        ee_zone *zone = zb[z];
        ee_unit *s = zone ? zone_site(zone, type) : NULL;
        if (!s) continue;
        ivec2 pos = unit_get_tile(s);
        int k = -1, dd = drop_nearest((unsigned)p, pos, 0, &k);
        unsigned g = site_gatherers(s);
        const char *why = !zone_get_active(zone) ? "zone inactive" : unit_get_amount(s) <= 0 ? "empty" :
            unit_vis_get_flag(unit_get_vis(s)) == 1 ? "site flag" : k < 0 ? "no drop site" :
            !explored_by(s, pl) ? "unexplored" :
            g >= cap ? "full" : build_mgr_enemy_near(bm, pos.x, pos.y) ? "enemy near" : NULL;
        if (!why) why = !route_safe(bm, d[k].p, pos) ? "route not safe" : site_danger(pl, s, 0) ? "site danger" : "ok";
        REPLY("  %d,%d zone %d: %u gatherers, drop %d,%d at %.0f tiles - %s\n", pos.x, pos.y, z, g, k >= 0 ? d[k].p.x : 0,
              k >= 0 ? d[k].p.y : 0, k >= 0 ? sqrtf((float)dd) : 0.0f, why);
    }
}
// LOCAL MINES ([ai_base] local_mines; the player 2026-10-28: "my citizens very rarely get far from their birthplace,
// citizen travel between settlements is zero"): the mine nearest to any drop site is often another town's - the
// rebalance, the crowds and the game's assignment sent citizens 200 tiles across the map and back (the player's
// autosave at minute 750: 656 citizens on gather trips of 60+ tiles in 4 minutes, 74% of the citizens' route search
// work, some sent on again before they got there). With a citizen given, a mine whose drop site lies within
// LOCAL_MINE_TILES of it - its own town - goes first. Preferring it changed nothing (1209 vs 1207 long trips): of 448
// picks 357 found its own town's mines full. So another town's mine - its drop site beyond LOCAL_MINE_FAR - is
// refused outright, the game's search's too: the citizen gets local work from the assignment's other passes, and the
// crowds' extras wait at their own town's mine (docs/economy.md "citizens stay in their town").
#define LOCAL_MINE_TILES 40
#define LOCAL_MINE_FAR 60
internal int g_local_mines;
internal int g_town_work, g_v_town_work = -1, g_town_hooked;   // TOWN WORK below: local only too; v_: h2h variant B
internal unsigned g_var_mask;   // econ.c: the players of variant B
internal ivec2 *g_town_home;   // TOWN WORK: a mine search measuring from a citizen's home, not where it stands
internal int town_on(unsigned pi) { return (g_var_mask >> (pi & 15) & 1) && g_v_town_work >= 0 ? g_v_town_work : g_town_work; }
// not_region: a continent (region class 2) whose mines don't count, -1 none (overseas_mines)
internal ee_unit *select_mine_near_drop(ee_resource_mgr *rm, int type, ee_unit *unit, int not_region) {
    ee_player *pl = resource_mgr_player(rm);
    ee_civ_mgr *cm = resource_mgr_get_owner(rm);
    void *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!pl || !bm) return NULL;
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    drops_cached(pl);
    if (g_mine_crowds) aim_count(pl, pi);
    const struct drop *d = g_drops[pi].d;
    unsigned cap = table_value(0x16);
    short ref_region = unit ? (short)map_area(unit_get_tile(unit), 2) : 0;
    ee_unit *best = NULL;
    int best_d = 0x7FFFFFFF, best_local = 0;
    int local = (g_local_mines || town_on(pi)) && unit && !is_vanilla(pl);
    ivec2 at = g_town_home ? *g_town_home : unit ? unit_get_tile(unit) : (ivec2){ 0, 0 };
    ee_zone **zb = player_get_zones_begin(pl); ee_zone **ze = player_get_zones_end(pl);
    for (int z = 0; zb && zb + z < ze; z++) {
        ee_zone *zone = zb[z];
        if (!zone || !zone_get_active(zone)) continue;
        ee_unit *s = zone_site(zone, type);
        if (!s || unit_get_amount(s) <= 0 || unit_vis_get_flag(unit_get_vis(s)) == 1) continue;
        ivec2 pos = unit_get_tile(s);
        int k = -1, dd = drop_nearest(pi, pos, 0, &k);   // nearest drop site for all resources (squared distance)
        if (k < 0) continue;
        if (local && ivec2_dist2(d[k].p, at) > LOCAL_MINE_FAR * LOCAL_MINE_FAR) continue;   // another town's
        int is_local = local && ivec2_dist2(d[k].p, at) <= LOCAL_MINE_TILES * LOCAL_MINE_TILES;
        if (is_local < best_local || (is_local == best_local && dd >= best_d)) continue;   // its own town's first
        if (!explored_by(s, pl)) continue;
        if (site_gatherers(s) >= cap) continue;
        if (g_mine_crowds && aim_of(pi, s) >= cap) continue;   // the citizens on their way or waiting there count
        if (unit && (short)map_area(pos, 2) != ref_region) continue;
        if (not_region >= 0 && (short)map_area(pos, 2) == not_region) continue;
        if (build_mgr_enemy_near(bm, pos.x, pos.y)) continue;
        if (!route_safe(bm, d[k].p, pos) || site_danger_res(pl, s, 0, type)) continue;
        best = s;
        best_d = dd;
        best_local = is_local;
    }
    if (best && g_mine_crowds) aim_add(pi, best);
    if (g_set.order_log && g_log && best) {   // tests: each pick - from where, local or not, how far
        ivec2 bp = unit_get_tile(best);
        log_locked("# mine pick: P%u res %d %s %d,%d -> %d,%d%s, caller %06x\n", pi, type, unit ? "citizen at" : "no citizen,", at.x, at.y,
                   bp.x, bp.y, best_local ? " (its town's)" : "", (unsigned)(CALLER() & 0xFFFFFF));
    }
    return best;
}
// MINE CROWDS: the citizens beyond the cap at a mine to the nearest free one of the same kind (every 10 s)
internal unsigned g_crowd_moved[16];
internal void mine_crowds(ee_resource_mgr *rm) {
    ee_player *pl = resource_mgr_player(rm);
    if (!pl || is_vanilla(pl)) return;
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    static uint32_t last[16], logged[16];
    uint32_t now = game_time();
    if (g_autotest && g_log && (!logged[pi] || now < logged[pi] || now - logged[pi] >= 60000)) {   // the crowds once a minute (also with it off)
        logged[pi] = now ? now : 1;
        g_aim[pi].t = 0;
        aim_count(pl, pi);
        unsigned cap = table_value(0x16);
        int over = 0, sites = 0, most = 0;
        for (int k = 0; k < g_aim[pi].n; k++)
            if (g_aim[pi].c[k] > cap) { over += (int)(g_aim[pi].c[k] - cap); sites++; if ((int)g_aim[pi].c[k] > most) most = g_aim[pi].c[k]; }
        fprintf(g_log, "# mine crowds: P%u at %.1f min: %d citizens beyond the cap at %d mines (at most %d at one)\n", pi, now / 60000.0, over, sites, most);
    }
    if (!g_mine_crowds || !g_mine_choice) return;
    if (last[pi] && now >= last[pi] && now - last[pi] < 10000) return;
    last[pi] = now ? now : 1;
    g_aim[pi].t = 0;
    aim_count(pl, pi);
    unsigned cap = table_value(0x16);
    int moved = 0;
    for (int k = 0; k < g_aim[pi].n && moved < 20; k++) {
        if (g_aim[pi].c[k] <= cap) continue;
        ee_unit *s = g_aim[pi].s[k];
        int type = unit_get_resource(s), over = (int)g_aim[pi].c[k] - (int)cap;
        ivec2 sp = unit_get_tile(s);
        static struct { ee_unit *u; int d; } v[256];
        int nv = 0;
        for (struct grid_it it = grid_it_all(1); grid_it_next(&it) && nv < 256; ) {
            ee_unit *u = it.u;
            if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || !is_citizen_type(unit_type_get_id(it.td))) continue;
            ee_goal *g = unit_get_goal(u);
            if (!g || strcmp(rtti_name(g), "EEUGGather@@") || goal_get_site(g) != s) continue;
            v[nv].u = u; v[nv].d = ivec2_dist2(unit_get_tile(u), sp); nv++;
        }
        while (over > 0 && nv > 0 && moved < 20) {   // the farthest from the mine first: those walking, then those waiting
            int bj = 0;
            for (int j = 1; j < nv; j++) if (v[j].d > v[bj].d) bj = j;
            ee_unit *u = v[bj].u;
            v[bj] = v[--nv];
            ee_unit *to = select_mine_near_drop(rm, type, u, -1);
            if (!to || to == s) break;   // no free mine of the kind: they wait
            resource_mgr_assign(rm, u, to);
            over--; moved++;
        }
    }
    g_crowd_moved[pi] += (unsigned)moved;
    if (moved && g_log) fprintf(g_log, "# mine crowds: P%u at %.1f min: %d citizens beyond the cap moved to free mines (%u in all)\n", pi, now / 60000.0, moved, g_crowd_moved[pi]);
}

// OVERSEAS MINES ([patches] overseas_mines, default on - Art of Conquest's fix): on a water map the citizen assignment
// (0x48A554) asks at its last call (data_ai_managers_mine_sites[5]) for any mine, no unit given, whenever a mine type is still short or
// the overseas timer (0x43123A) is due - with nobody short too - and puts every idle citizen of the home island on the
// transport manager's crossing to it (0x48A382). Any free mine does, the home island's too (60-75% of the searches;
// with mine_choice the nearest to a drop site): the boat mostly never comes, and the citizens taken off that pass miss
// the leftover pass after it (run only while idle ones are left) - idle until the next. Art of Conquest (0x48F3F5)
// ships only citizens connected to home and not to the mine. Here that search gets only a mine on another continent
// (region class 2, as the searches compare) than the base, none if there is none. Counted for every player (the
// game's own AI too): "# overseas mines:" with the islands log. Measured (tests/scenarios/islands.eeai, 60 min, 36 seeds x 2
// AIs, on - off): Small Islands e5 citizens +4.2 (se 2.5), home miners +3.1 (1.7), income +659 (1017); Large Islands e9
// citizens +2.3 (1.1), home miners +1.7 (1.3), income +25 (536); kills, losses, epochs within noise - a small gain
// within 2 standard errors, on since it costs nothing (2026-10-21, the user).
internal int g_overseas_mines = 1;
internal unsigned g_oversea[16][4];   // per player: searches, home mines found, overseas mines found, home ones replaced
internal void *find_mine_pick(ee_resource_mgr *rm, int type, ee_unit *unit, uintptr_t rev);
internal ee_unit *__thiscall hook_find_mine(ee_resource_mgr *rm, int type, ee_unit *unit, uintptr_t rev) {
    uintptr_t caller = CALLER();
    if (g_set.order_log && g_log) { char at[96]; crash_addr(at, sizeof at, caller); log_locked("# mine ask: res %d from %s%s\n", type, at, unit ? "" : ", no citizen"); }
    ee_unit *r = find_mine_pick(rm, type, unit, rev);
    ee_player *pl = resource_mgr_player(rm);
    ee_civ_mgr *cm = resource_mgr_get_owner(rm); ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (mine_site(caller) != 5 || !pl || !bm) return r;
    ivec2 base = build_mgr_get_base(bm);
    if (base.x == -10000) return r;   // no base point: the game ships everyone
    int home = (short)map_area(base, 2);
    unsigned *n = g_oversea[(unsigned)player_get_index(pl) & 15];
    n[0]++;
    if (!r) return r;
    if ((short)map_area(unit_get_tile(r), 2) != home) { n[2]++; return r; }
    n[1]++;
    if (!g_overseas_mines || is_vanilla(pl)) return r;
    r = g_mine_choice && type >= 4 && type < 7 ? select_mine_near_drop(rm, type, NULL, home) : NULL;
    if (r) n[3]++;
    return r;
}
internal void *find_mine_pick(ee_resource_mgr *rm, int type, ee_unit *unit, uintptr_t rev) {
    if (is_vanilla(resource_mgr_player(rm))) return resource_mgr_find_mine(rm, (uintptr_t)type, unit, rev);   // the game's search
    ee_unit *r = g_mine_choice && type >= 4 && type < 7 ? select_mine_near_drop(rm, type, unit, -1) : NULL;
    if (r) return r;
    r = refuse_danger(rm, resource_mgr_find_mine(rm, (uintptr_t)type, unit, rev), type == 2, type);
    if (r && unit && type >= 4 && type < 7 && (g_local_mines || town_on((unsigned)player_get_index(resource_mgr_player(rm))))) {   // local_mines: the game's pick another town's - refused
        ee_player *pl = resource_mgr_player(rm);
        unsigned pi = (unsigned)player_get_index(pl) & 15;
        drops_cached(pl);
        int k = -1;
        if (drop_nearest(pi, unit_get_tile(r), 0, &k) == 0x7FFFFFFF ||
            ivec2_dist2(g_drops[pi].d[k].p, unit_get_tile(unit)) > LOCAL_MINE_FAR * LOCAL_MINE_FAR) return NULL;
    }
    if (r && g_mine_crowds && type >= 4 && type < 7) {   // the game's search knows only the working gatherers: a full mine refused
        ee_player *pl = resource_mgr_player(rm);
        unsigned pi = (unsigned)player_get_index(pl) & 15;
        aim_count(pl, pi);
        if (aim_of(pi, r) >= table_value(0x16)) return NULL;
        aim_add(pi, r);
    }
    return r;
}

internal ee_unit *__thiscall hook_find_food(ee_resource_mgr *rm, uintptr_t need_drop, uintptr_t near_base, ee_unit *unit) {
    if (is_vanilla(resource_mgr_player(rm))) return resource_mgr_find_food(rm, need_drop, near_base, unit);
    ee_unit *s = food_near_pick(rm, (int)(near_base & 0xFF), unit);   // food_plan NEAR: the bush nearest to a drop site
    return s ? s : refuse_danger(rm, resource_mgr_find_food(rm, need_drop, near_base, unit), 1, 2);
}

// The assignment's lists (std::vector out: +4 begin, +8 end): finished fields without farmer (0x48BFD0, called at
// 0x48AC54), unfinished fields (0x48BF4F at 0x48B0FD), game to hunt (0x48BA3B at 0x48ACE4, 0x48AFC4) - the
// dangerous entries are taken out, as the searches refuse dangerous sites.
internal void list_filter(ee_resource_mgr *rm, uint8_t *v, int food) {
    ee_player *pl = resource_mgr_player(rm);
    void **b = v ? FIELD(v, 4, void **) : NULL, **e = v ? FIELD(v, 8, void **) : NULL, **w = b;
    if (!pl || !b || is_vanilla(pl)) return;   // a vanilla player: the game's list
    for (void **i = b; i < e; i++) {
        if (site_danger(pl, *i, food)) { g_site_refused[(unsigned)player_get_index(pl) & 15]++; continue; }
        *w++ = *i;
    }
    FIELD(v, 8, void **) = w;
}
internal void *__thiscall hook_farms(ee_resource_mgr *rm, uintptr_t n, uintptr_t out) {
    void *r = resource_mgr_farms(rm, n, out);
    list_filter(rm, (uint8_t *)out, 1);
    return r;
}
internal void *__thiscall hook_unbuilt_farms(ee_resource_mgr *rm, uintptr_t n, uintptr_t out) {
    void *r = resource_mgr_unbuilt_farms(rm, n, out);
    list_filter(rm, (uint8_t *)out, 1);
    return r;
}
internal void *__thiscall hook_hunt(ee_resource_mgr *rm, uintptr_t n, uintptr_t f, uintptr_t out) {
    void *r = resource_mgr_hunt(rm, n, f, out);
    list_filter(rm, (uint8_t *)out, 1);
    return r;
}
internal ee_unit *__thiscall hook_find_wood(ee_resource_mgr *rm, uintptr_t flag, ee_unit *unit) {
    if (is_vanilla(resource_mgr_player(rm))) return resource_mgr_find_wood(rm, flag, unit);
    return refuse_danger(rm, resource_mgr_find_wood(rm, flag, unit), 0, 3);
}

// TOWN WORK ([ai_base] town_work, off; docs/economy.md "citizens stay in their town"): the game's assignment
// (0x48A554) fills each resource's empire-wide shortfall with whichever citizen and site it finds - citizens walked
// 200 tiles to another town's mine and back. Refusing those sites alone (local_mines) left the citizens idle: the
// assignment only fills shortfalls, asks again and gets refused again. Here the mod decides the site at every one
// of the assignment's sends (calls_gather): a site whose nearest drop site lies beyond TOWN_FAR of the citizen's
// home is swapped for work there - the same resource, else the shortest one, else wood, else food; another town's
// only when its home has no work at all. After the assignment the idle citizens get local work the same way
// (town_idle), and every mine search is local only (select_mine_near_drop, find_mine_pick).
// HOMES (the player: "if I need new citizens in a town fast I train from several neighbouring town centers - a
// longer way once, but faster than waiting on the town's own"): the citizens' order (train_citizens 0x48EE84) shares
// its citizens out evenly over all town centers, wherever the work is. Here each one goes to the town whose mines
// have the most free places (TOWN_MINE_TILES round its drop site, the citizens promised to it counted) - queued at
// the town center that gets it there first: its queue (TOWN_QUEUE_TILES of walking each) plus the walk. The center
// keeps the town as the home of its next citizen out; the first job given to a citizen within TOWN_BORN_TILES of
// it takes that home (no hook sees a unit trained). A citizen without a home is at home where it stands.
#define TOWN_FAR LOCAL_MINE_FAR
#define TOWN_MINE_TILES 16
#define TOWN_QUEUE_TILES 25
#define TOWN_BORN_TILES 8
internal int rm_count(ee_resource_mgr *rm, int t);   // econ.c: citizens on resource t
internal int citizen_free(ee_unit *u, int idle);     // econ.c
internal resource_mgr_assign_to_site_fn g_gather_next;   // NULL the original, else trace.c's logging hook before it
internal struct { unsigned swapped, away, idle, sent, homed; uint32_t logged; } g_town[16];
// homes: per player an open table by unit (its id against a reused address), the home's tile
#define HOMES 2048
internal struct town_home { ee_unit *u; int id; ivec2 at; } g_town_homes[16][HOMES];
// promised homes: per player and town center, the homes of its next citizens out (oldest first)
#define PROMISE_TC 32
#define PROMISE_N 16
internal struct promise { ee_unit *tc; int n; ivec2 at[PROMISE_N]; } g_tc_promise[16][PROMISE_TC];
internal struct town_home *home_slot(unsigned pi, ee_unit *u) {
    unsigned h = ((uintptr_t)u >> 4) * 2654435761u % HOMES;
    for (int k = 0; k < 16; k++, h = (h + 1) % HOMES)
        if (g_town_homes[pi][h].u == u || !g_town_homes[pi][h].u) return &g_town_homes[pi][h];
    return &g_town_homes[pi][((uintptr_t)u >> 4) * 2654435761u % HOMES];   // full run: the first slot taken over
}
// u's home: its own, or a promise of a town center beside it (taken now), else where it stands
internal ivec2 home_of(unsigned pi, ee_unit *u) {
    struct town_home *h = home_slot(pi, u);
    if (h->u == u && h->id == unit_get_id(u)) return h->at;
    ivec2 at = unit_get_tile(u);
    for (int i = 0; i < PROMISE_TC; i++) {
        struct promise *p = &g_tc_promise[pi][i];
        if (!p->tc || !p->n || ivec2_dist2(unit_get_tile(p->tc), at) > TOWN_BORN_TILES * TOWN_BORN_TILES) continue;
        h->u = u; h->id = unit_get_id(u); h->at = p->at[0];
        memmove(p->at, p->at + 1, (size_t)--p->n * sizeof *p->at);
        g_town[pi].homed++;
        return h->at;
    }
    return at;
}
// site's nearest drop site (food: granaries too) within TOWN_FAR of the home
internal int town_local(unsigned pi, ivec2 home, ee_unit *site, int food) {
    int k = -1;
    if (drop_nearest(pi, unit_get_tile(site), food, &k) == 0x7FFFFFFF || k < 0) return 0;
    return ivec2_dist2(g_drops[pi].d[k].p, home) <= TOWN_FAR * TOWN_FAR;
}
// a site of resource r (2 food .. 6 iron) for citizen u at its home, NULL none
internal ee_unit *town_site(ee_resource_mgr *rm, int r, ee_unit *u, ivec2 home) {
    ee_player *pl = resource_mgr_player(rm);
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    drops_cached(pl);
    g_town_home = &home;   // the mine search measures from the home
    ee_unit *s = r >= 4 ? select_mine_near_drop(rm, r, u, -1)
               : r == 3 ? hook_find_wood(rm, 1, u) : hook_find_food(rm, 1, 1, u);
    g_town_home = NULL;
    return s && town_local(pi, home, s, r == 2) ? s : NULL;
}
// work at its home for u: resource first (-1 none), then the resources by shortfall, then wood, then food
internal ee_unit *town_work_for(ee_resource_mgr *rm, ee_unit *u, ivec2 home, int first) {
    ee_unit *s = first >= 2 && first < 7 ? town_site(rm, first, u, home) : NULL;
    if (s) return s;
    uint32_t *target = resource_mgr_target_at(rm);
    int tried = first >= 2 && first < 7 ? 1 << first : 0;
    for (;;) {
        int r = -1, most = 0;
        for (int t = 2; t < 7; t++) {
            int short_by = (int)target[t] - rm_count(rm, t);
            if (!(tried >> t & 1) && short_by > most) { most = short_by; r = t; }
        }
        if (r < 0) break;
        tried |= 1 << r;
        if ((s = town_site(rm, r, u, home))) return s;
    }
    if (!(tried & 8) && (s = town_site(rm, 3, u, home))) return s;
    if (!(tried & 4) && (s = town_site(rm, 2, u, home))) return s;
    return NULL;
}
internal void __thiscall hook_town_gather(ee_resource_mgr *rm, ee_unit *u, ee_unit *site, int c) {
    ee_player *pl = resource_mgr_player(rm);
    unsigned pi = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    if (pl && town_on(pi) && !is_vanilla(pl) && u && site && is_citizen_type(unit_type_get_id(unit_get_type_data(u)))) {
        int r = unit_get_resource(site);
        drops_cached(pl);
        ivec2 home = home_of(pi, u);
        if (!town_local(pi, home, site, r == 2)) {
            ee_unit *s = town_work_for(rm, u, home, r);
            if (s) { site = s; g_town[pi].swapped++; }
            else g_town[pi].away++;   // its home has no work: it goes
        }
    }
    if (g_gather_next) g_gather_next(rm, u, site, c); else resource_mgr_assign_as(rm, u, site, c);
}
// after the assignment: the idle citizens to work at their homes (40 at most a pass)
internal void town_idle(ee_resource_mgr *rm) {
    ee_player *pl = resource_mgr_player(rm);
    if (!pl || is_vanilla(pl)) return;
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    if (!town_on(pi)) return;
    int given = 0;
    drops_cached(pl);
    for (int r = 0; r < 7 && given < 40; r++) {
        uint8_t *list = resource_mgr_list_at(rm, r);
        for (unsigned k = 0, first = deque_front(list), cnt = deque_count(list); k < cnt && given < 40; k++) {
            ee_unit *u = deque_at(list, first + k);
            if (!citizen_free(u, 1)) continue;
            ee_unit *s = town_work_for(rm, u, home_of(pi, u), -1);
            if (!s) continue;
            resource_mgr_assign(rm, u, s);
            given++;
        }
    }
    g_town[pi].idle += (unsigned)given;
    uint32_t now = game_time();
    if (g_log && (!g_town[pi].logged || now < g_town[pi].logged || now - g_town[pi].logged >= 60000)) {
        g_town[pi].logged = now ? now : 1;
        fprintf(g_log, "# town work: P%u at %.1f min: %u far sites swapped for work at home, %u sent far (no work at home), %u idle "
                "citizens given work, %u citizens trained for a town with free mines, %u took that home\n",
                pi, now / 60000.0, g_town[pi].swapped, g_town[pi].away, g_town[pi].idle, g_town[pi].sent, g_town[pi].homed);
    }
}
// the town (index into g_drops) whose mines have the most free places beyond the citizens promised to it, -1 none
internal int town_needing(ee_player *pl, unsigned pi) {
    unsigned cap = table_value(0x16);
    int free_at[64] = { 0 };
    if (g_mine_crowds) aim_count(pl, pi);
    ee_zone **zb = player_get_zones_begin(pl); ee_zone **ze = player_get_zones_end(pl);
    for (int z = 0; zb && zb + z < ze; z++) {
        ee_zone *zone = zb[z];
        if (!zone || !zone_get_active(zone)) continue;
        for (int r = 4; r < 7; r++) {
            ee_unit *s = zone_site(zone, r);
            if (!s || unit_get_amount(s) <= 0 || !explored_by(s, pl)) continue;
            int k = -1;
            if (drop_nearest(pi, unit_get_tile(s), 0, &k) > TOWN_MINE_TILES * TOWN_MINE_TILES || k < 0 || k >= 64) continue;
            unsigned n = site_gatherers(s), a = g_mine_crowds ? aim_of(pi, s) : 0;
            if (a > n) n = a;
            if (n < cap) free_at[k] += (int)(cap - n);
        }
    }
    for (int i = 0; i < PROMISE_TC; i++)   // the citizens already on their way
        for (int j = 0; j < g_tc_promise[pi][i].n; j++)
            for (int k = 0; k < g_drops[pi].n && k < 64; k++)
                if (ivec2_eq(g_drops[pi].d[k].p, g_tc_promise[pi][i].at[j])) free_at[k]--;
    int best = -1, most = 0;
    for (int k = 0; k < g_drops[pi].n && k < 64; k++)
        if (free_at[k] > most) { most = free_at[k]; best = k; }
    return best;
}
// the citizens' order: each citizen queued for the town needing it most, at the town center that gets it there first
internal void __thiscall hook_train_citizen_at(ee_unit *tc, int tech, char now) {
    ee_player *pl = tc ? unit_get_owner(tc) : NULL;
    unsigned pi = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    if (!pl || is_vanilla(pl) || !town_on(pi)) { unit_enqueue(tc, tech, now); return; }
    drops_cached(pl);
    int k = town_needing(pl, pi);
    if (k < 0) { unit_enqueue(tc, tech, now); return; }   // no town short of miners: the game's even share
    ivec2 to = g_drops[pi].d[k].p;
    int tct = unit_type_get_id(unit_get_type_data(tc));
    ee_unit *best = tc;
    int best_cost = 0x7FFFFFFF;
    for (int i = 0; i < g_drops[pi].n; i++) {   // the town centers that train what tc trains
        ee_unit *c = g_drops[pi].d[i].u;
        if (!c || g_drops[pi].d[i].food || unit_get_hp(c) <= 0 || unit_type_get_id(unit_get_type_data(c)) != tct) continue;
        int *qb = unit_get_queue_begin(c), *qe = unit_get_queue_end(c);
        int queued = qb && qe > qb ? (int)(qe - qb) : 0;
        int cost = queued * TOWN_QUEUE_TILES + (int)sqrtf((float)ivec2_dist2(g_drops[pi].d[i].p, to));
        if (cost < best_cost) { best_cost = cost; best = c; }
    }
    unit_enqueue(best, tech, now);
    struct promise *p = NULL;
    for (int i = 0; i < PROMISE_TC && !p; i++) if (g_tc_promise[pi][i].tc == best) p = &g_tc_promise[pi][i];
    for (int i = 0; i < PROMISE_TC && !p; i++) if (!g_tc_promise[pi][i].tc || !g_tc_promise[pi][i].n) p = &g_tc_promise[pi][i];
    if (!p) return;
    if (p->tc != best) { p->tc = best; p->n = 0; }
    if (p->n == PROMISE_N) memmove(p->at, p->at + 1, (size_t)--p->n * sizeof *p->at);   // the oldest dropped
    p->at[p->n++] = to;
    g_town[pi].sent++;
}

internal void sites_install(void) {
    g_site_max_dist = g_set_ai[1].site_max_distance;
    g_mine_choice = g_set_ai[1].mine_choice;
    g_mine_crowds = g_set_ai[1].mine_crowds;
    g_local_mines = g_set_ai[1].local_mines;
    g_town_work = g_set_ai[1].town_work;
    if (g_autotest) g_v_town_work = ini_int("autotest", "v_town_work", -1, g_ini);
    g_overseas_mines = g_set.overseas_mines;
    int ok = 0, total = 0;
    // always: the searches refuse sites behind known enemy towers (avoid_towers_citizens, attack.c)
    ok += patch_find_mine(hook_find_mine, &total) + patch_find_food(hook_find_food, &total) + patch_find_wood(hook_find_wood, &total) +
          patch_farms(hook_farms, hook_unbuilt_farms, &total) + patch_hunt(hook_hunt, &total);
    if (g_town_work || g_v_town_work > 0) { ok += patch_gather(hook_town_gather, &total) + patch_train_citizen_at(hook_train_citizen_at, &total); g_town_hooked = 1; }
    if (g_log) fprintf(g_log, "# site hooks: %d/%d, max distance %d; mine_choice=%d, overseas_mines=%d\n", ok, total, g_site_max_dist, g_mine_choice, g_overseas_mines);
}

// a new game (the game start hook)
internal void sites_new_game(void) {
    memset(g_drops, 0, sizeof g_drops);
    memset(g_site_refused, 0, sizeof g_site_refused);
    memset(g_oversea, 0, sizeof g_oversea);
    memset(g_town, 0, sizeof g_town);
    memset(g_town_homes, 0, sizeof g_town_homes);
    memset(g_tc_promise, 0, sizeof g_tc_promise);
}
