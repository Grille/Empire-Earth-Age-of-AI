// SITES: the resource manager's site searches - mines (0x48C204), food (0x48BBA8), wood - and its field and hunt
// lists, hooked. The originals take the FIRST acceptable site in zone order, anywhere on the map, and only check for
// enemies at the site itself - the game's pathfinding then walks citizens straight through enemy defences
// (observed: red mining iron under yellow's towers 25 tiles away, losing its workforce). Here a site behind known
// enemy towers is refused (avoid_towers_citizens, attack.c site_danger), a mine is the free one nearest to a drop
// site (mine_choice), a bush the one nearest to a drop site (food_plan, food.c), an overseas mine search gets only
// a mine overseas (overseas_mines). (safe_sites - the nearest site to the base within site_max_distance, the
// leftover pass only to mines short of workers - removed: docs/experiments.md.) Included by eeaimod.c before econ.c.

internal int g_site_max_dist = 20;   // tiles from the base, aimod.ini [all] site_max_distance (econ.c: the wild food counted)

internal int site_danger(ee_player *pl, ee_unit *site, int food);   // attack.c: known enemy towers on the way
internal int site_danger_res(ee_player *pl, ee_unit *site, int food, int res);   // attack.c: res 2 food .. 6 iron
internal ee_unit *food_near_pick(ee_resource_mgr *rm, int near_base, ee_unit *unit);   // food.c (food_plan NEAR)
internal unsigned g_site_refused[16];   // dangerous sites the searches refused (logged by attack.c)

internal int route_safe(void *bm, const int base[2], const int to[2]) {
    int dx = to[0] - base[0], dy = to[1] - base[1];
    int len = (int)sqrtf((float)(dx * dx + dy * dy));
    for (int t = 3; t < len; t += 2)   // skip the first tiles: our own base
        if (bm_enemy_near(bm, base[0] + dx * t / len, base[1] + dy * t / len)) return 0;
    return 1;
}

// Drop sites of a player from the unit grid: Town Center, Capitol, Settlement (also the single player one) take
// everything, Granary (also the improved one) only food. Up to `max` entries {x, y, food_only}.
internal int drop_sites(ee_player *pl, int (*out)[3], int max) {
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid) return 0;
    int n = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (!td || unit_get_owner(u) != pl || n >= max) continue;
        int t = type_get_id(td), food = t == UT(B_GRANARY) || t == UT(B_GRANARY_IMPROVED);
        if (!food && !UT_IN(t, UT_TOWNS, UT_X_SINGLE_PLAYER_SETTLEMENT)) continue;
        out[n][0] = unit_get_x(u); out[n][1] = unit_get_y(u); out[n][2] = food;
        n++;
    }
    return n;
}
internal int drop_dist(int (*d)[3], int nd, int x, int y, int food) {
    int best = 999;
    for (int i = 0; i < nd; i++) {
        if (d[i][2] && !food) continue;
        int dx = d[i][0] - x, dy = d[i][1] - y, r = (int)sqrtf((float)(dx * dx + dy * dy));
        if (r < best) best = r;
    }
    return best;
}

// MINE CHOICE (aimod.ini [all] mine_choice, default on): the original mine search takes the first acceptable mine in
// reverse zone order - with mines next to its town centers free, P2 sent 20 citizens to iron 35 tiles from any drop
// site (3-5 iron per worker-minute against 10-12 for wood next door). Mines are practically endless (300000), so the
// walk decides the income: take the free mine nearest to one of the AI's drop sites (Town Center, Capitol,
// Settlement) instead, with the original's checks (active zone, explored, gatherer cap, no enemy near, the unit's
// continent) and no known enemy on the straight way from that drop site. Nothing found -> the original search.
internal int g_mine_choice = 1;
internal struct { ee_player *pl; uint32_t t; int n; int d[64][3]; } g_drops[16];
internal void drops_cached(ee_player *pl) {
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    uint32_t now = game_time();
    if (g_drops[pi].pl != pl || now < g_drops[pi].t || now - g_drops[pi].t > 5000) {   // cached 5 s
        g_drops[pi].pl = pl; g_drops[pi].t = now;
        g_drops[pi].n = drop_sites(pl, g_drops[pi].d, 64);
    }
}
internal int nearest_drop(ee_player *pl, int x, int y, int food, int out[2]) {   // food: granaries count too
    drops_cached(pl);
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    int best = 0x7FFFFFFF;
    for (int i = 0; i < g_drops[pi].n; i++) {
        int *d = g_drops[pi].d[i], dx = d[0] - x, dy = d[1] - y;
        if ((d[2] && !food) || dx * dx + dy * dy >= best) continue;
        best = dx * dx + dy * dy; out[0] = d[0]; out[1] = d[1];
    }
    return best != 0x7FFFFFFF;
}
internal void *refuse_danger(ee_resource_mgr *rm, void *site, int food, int res) {
    ee_player *pl = rm_player(rm);
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
    void *bm = cm ? cm_get_build_mgr(cm) : NULL;
    if (!bm) { REPLY("error: no AI P%d\n", p); return; }
    drops_cached(pl);
    int nd = g_drops[p].n, (*d)[3] = g_drops[p].d;
    unsigned cap = table_value(0x16);
    ee_zone **zb = player_get_zones_begin(pl); ee_zone **ze = player_get_zones_end(pl);
    REPLY("P%d type %d, %d drop sites:", p, type, nd);
    for (int i = 0; i < nd; i++) REPLY(" %d,%d%s", d[i][0], d[i][1], d[i][2] ? "g" : "");
    REPLY("\n");
    for (int z = 0; zb && zb + z < ze; z++) {
        ee_zone *zone = zb[z];
        ee_unit *s = zone ? zone_site(zone, type) : NULL;
        if (!s) continue;
        int pos[2] = { unit_get_x(s), unit_get_y(s) }, k = -1, dd = 0x7FFFFFFF;
        for (int i = 0; i < nd; i++) {
            int dx = d[i][0] - pos[0], dy = d[i][1] - pos[1];
            if (!d[i][2] && dx * dx + dy * dy < dd) { dd = dx * dx + dy * dy; k = i; }
        }
        unsigned g = site_gatherers(s);
        const char *why = !zone_get_active(zone) ? "zone inactive" : unit_get_amount(s) <= 0 ? "empty" :
            siteown_get_flag(unit_get_vis(s)) == 1 ? "site flag" : k < 0 ? "no drop site" :
            !explored_by(s, pl) ? "unexplored" :
            g >= cap ? "full" : bm_enemy_near(bm, pos[0], pos[1]) ? "enemy near" : NULL;
        if (!why) {
            int from[2] = { d[k][0], d[k][1] };
            why = !route_safe(bm, from, pos) ? "route not safe" : site_danger(pl, s, 0) ? "site danger" : "ok";
        }
        REPLY("  %d,%d zone %d: %u gatherers, drop %d,%d at %.0f tiles - %s\n", pos[0], pos[1], z, g, k >= 0 ? d[k][0] : 0,
              k >= 0 ? d[k][1] : 0, k >= 0 ? sqrtf((float)dd) : 0.0f, why);
    }
}
// not_region: a continent (region class 2) whose mines don't count, -1 none (overseas_mines)
internal ee_unit *select_mine_near_drop(ee_resource_mgr *rm, int type, ee_unit *unit, int not_region) {
    ee_player *pl = rm_player(rm);
    ee_civ_mgr *cm = rm_get_owner(rm);
    void *bm = cm ? cm_get_build_mgr(cm) : NULL;
    if (!pl || !bm) return NULL;
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    drops_cached(pl);
    int nd = g_drops[pi].n, (*d)[3] = g_drops[pi].d;
    unsigned cap = table_value(0x16);
    short ref_region = 0;
    if (unit) { int up[2] = { unit_get_x(unit), unit_get_y(unit) }; ref_region = (short)map_area(up, 2); }
    ee_unit *best = NULL;
    int best_d = 0x7FFFFFFF;
    ee_zone **zb = player_get_zones_begin(pl); ee_zone **ze = player_get_zones_end(pl);
    for (int z = 0; zb && zb + z < ze; z++) {
        ee_zone *zone = zb[z];
        if (!zone || !zone_get_active(zone)) continue;
        ee_unit *s = zone_site(zone, type);
        if (!s || unit_get_amount(s) <= 0 || siteown_get_flag(unit_get_vis(s)) == 1) continue;
        int pos[2] = { unit_get_x(s), unit_get_y(s) };
        int k = -1, dd = 0x7FFFFFFF;   // nearest drop site for all resources (squared distance)
        for (int i = 0; i < nd; i++) {
            int dx = d[i][0] - pos[0], dy = d[i][1] - pos[1];
            if (!d[i][2] && dx * dx + dy * dy < dd) { dd = dx * dx + dy * dy; k = i; }
        }
        if (k < 0 || dd >= best_d) continue;
        if (!explored_by(s, pl)) continue;
        if (site_gatherers(s) >= cap) continue;
        if (unit && (short)map_area(pos, 2) != ref_region) continue;
        if (not_region >= 0 && (short)map_area(pos, 2) == not_region) continue;
        if (bm_enemy_near(bm, pos[0], pos[1])) continue;
        int from[2] = { d[k][0], d[k][1] };
        if (!route_safe(bm, from, pos) || site_danger_res(pl, s, 0, type)) continue;
        best = s;
        best_d = dd;
    }
    return best;
}

// OVERSEAS MINES ([all] overseas_mines, default on - Art of Conquest's fix): on a water map the citizen assignment
// (0x48A554) asks at its last call (MINE_SITES[5]) for any mine, no unit given, whenever a mine type is still short or
// the overseas timer (0x43123A) is due - with nobody short too - and puts every idle citizen of the home island on the
// transport manager's crossing to it (0x48A382). Any free mine does, the home island's too (60-75% of the searches;
// with mine_choice the nearest to a drop site): the boat mostly never comes, and the citizens taken off that pass miss
// the leftover pass after it (run only while idle ones are left) - idle until the next. Art of Conquest (0x48F3F5)
// ships only citizens connected to home and not to the mine. Here that search gets only a mine on another continent
// (region class 2, as the searches compare) than the base, none if there is none. Counted for every player (the
// game's own AI too): "# overseas mines:" with the islands log. Measured (scenarios/islands.eeai, 60 min, 36 seeds x 2
// AIs, on - off): Small Islands e5 citizens +4.2 (se 2.5), home miners +3.1 (1.7), income +659 (1017); Large Islands e9
// citizens +2.3 (1.1), home miners +1.7 (1.3), income +25 (536); kills, losses, epochs within noise - a small gain
// within 2 standard errors, on since it costs nothing (2026-10-21, the user).
internal int g_overseas_mines = 1;
internal unsigned g_oversea[16][4];   // per player: searches, home mines found, overseas mines found, home ones replaced
internal void *find_mine_pick(ee_resource_mgr *rm, int type, ee_unit *unit, uintptr_t rev);
internal void *__thiscall hook_find_mine(ee_resource_mgr *rm, int type, ee_unit *unit, uintptr_t rev) {
    uintptr_t caller = CALLER();
    ee_unit *r = find_mine_pick(rm, type, unit, rev);
    ee_player *pl = rm_player(rm);
    ee_civ_mgr *cm = rm_get_owner(rm); ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
    if (mine_site(caller) != 5 || !pl || !bm) return r;
    int base[2] = { bm_get_base_x(bm), bm_get_base_y(bm) };
    if (base[0] == -10000) return r;   // no base point: the game ships everyone
    int home = (short)map_area(base, 2);
    unsigned *n = g_oversea[(unsigned)player_get_index(pl) & 15];
    n[0]++;
    if (!r) return r;
    int pos[2] = { unit_get_x(r), unit_get_y(r) };
    if ((short)map_area(pos, 2) != home) { n[2]++; return r; }
    n[1]++;
    if (!g_overseas_mines || is_vanilla(pl)) return r;
    r = g_mine_choice && type >= 4 && type < 7 ? select_mine_near_drop(rm, type, NULL, home) : NULL;
    if (r) n[3]++;
    return r;
}
internal void *find_mine_pick(ee_resource_mgr *rm, int type, ee_unit *unit, uintptr_t rev) {
    if (is_vanilla(rm_player(rm))) return rm_find_mine(rm, (uintptr_t)type, unit, rev);   // the game's search
    ee_unit *r = g_mine_choice && type >= 4 && type < 7 ? select_mine_near_drop(rm, type, unit, -1) : NULL;
    return r ? r : refuse_danger(rm, rm_find_mine(rm, (uintptr_t)type, unit, rev), type == 2, type);
}

internal void *__thiscall hook_find_food(ee_resource_mgr *rm, uintptr_t need_drop, uintptr_t near_base, ee_unit *unit) {
    if (is_vanilla(rm_player(rm))) return rm_find_food(rm, need_drop, near_base, unit);
    ee_unit *s = food_near_pick(rm, (int)(near_base & 0xFF), unit);   // food_plan NEAR: the bush nearest to a drop site
    return s ? s : refuse_danger(rm, rm_find_food(rm, need_drop, near_base, unit), 1, 2);
}

// The assignment's lists (std::vector out: +4 begin, +8 end): finished fields without farmer (0x48BFD0, called at
// 0x48AC54), unfinished fields (0x48BF4F at 0x48B0FD), game to hunt (0x48BA3B at 0x48ACE4, 0x48AFC4) - the
// dangerous entries are taken out, as the searches refuse dangerous sites.
internal void list_filter(ee_resource_mgr *rm, uint8_t *v, int food) {
    ee_player *pl = rm_player(rm);
    void **b = v ? FIELD(v, 4, void **) : NULL, **e = v ? FIELD(v, 8, void **) : NULL, **w = b;
    if (!pl || !b || is_vanilla(pl)) return;   // a vanilla player: the game's list
    for (void **i = b; i < e; i++) {
        if (site_danger(pl, *i, food)) { g_site_refused[(unsigned)player_get_index(pl) & 15]++; continue; }
        *w++ = *i;
    }
    FIELD(v, 8, void **) = w;
}
internal void *__thiscall hook_farms(ee_resource_mgr *rm, uintptr_t n, uintptr_t out) {
    void *r = rm_farms(rm, n, out);
    list_filter(rm, (uint8_t *)out, 1);
    return r;
}
internal void *__thiscall hook_unbuilt_farms(ee_resource_mgr *rm, uintptr_t n, uintptr_t out) {
    void *r = rm_unbuilt_farms(rm, n, out);
    list_filter(rm, (uint8_t *)out, 1);
    return r;
}
internal void *__thiscall hook_hunt(ee_resource_mgr *rm, uintptr_t n, uintptr_t f, uintptr_t out) {
    void *r = rm_hunt(rm, n, f, out);
    list_filter(rm, (uint8_t *)out, 1);
    return r;
}
internal void *__thiscall hook_find_wood(ee_resource_mgr *rm, uintptr_t flag, ee_unit *unit) {
    if (is_vanilla(rm_player(rm))) return rm_find_wood(rm, flag, unit);
    return refuse_danger(rm, rm_find_wood(rm, flag, unit), 0, 3);
}

internal void sites_install(const char *aimod_ini, const char *autotest_ini) {
    g_site_max_dist = cfg_int(aimod_ini, autotest_ini, 1, "site_max_distance", 20);
    g_mine_choice = cfg_int(aimod_ini, autotest_ini, 1, "mine_choice", 1);
    g_overseas_mines = cfg_int(aimod_ini, autotest_ini, 1, "overseas_mines", 1);
    int ok = 0, total = 0;
    // always: the searches refuse sites behind known enemy towers (avoid_towers_citizens, attack.c)
    ok += patch_find_mine(hook_find_mine, &total) + patch_find_food(hook_find_food, &total) + patch_find_wood(hook_find_wood, &total) +
          patch_farms(hook_farms, hook_unbuilt_farms, &total) + patch_hunt(hook_hunt, &total);
    if (g_log) fprintf(g_log, "# site hooks: %d/%d, max distance %d; mine_choice=%d, overseas_mines=%d\n", ok, total, g_site_max_dist, g_mine_choice, g_overseas_mines);
}

// a new game (the game start hook)
internal void sites_new_game(void) {
    memset(g_drops, 0, sizeof g_drops);
    memset(g_site_refused, 0, sizeof g_site_refused);
    memset(g_oversea, 0, sizeof g_oversea);
}
