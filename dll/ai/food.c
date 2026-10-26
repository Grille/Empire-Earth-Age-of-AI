// WILD FOOD ([ai_base] food_plan, bits; [autotest] v_food_plan for the variant players). Included by eeaimod.c after early.c.
// The playtester: "hunting is an important starting step. Animals reproduce - a sustainable source if not overhunted,
// though usually collecting them as quickly as possible is better. Ditch hunting for farming as soon as possible, but
// even in late-epoch starts farms are a huge investment that can wait a bit while other sources (herds, forage,
// fish) are still available." And: early food is for citizens.
// What the game's AI does (docs/economy.md "Food"): the food passes of the assignment (0x48A554) go forage near the
// base (0x48BBA8, the FIRST free bush in zone order within 2 x player +0xE10 tiles, not the nearest), finished fields,
// hunting (0x48BA3B: one hunter an animal and pass, from the hunt list rm +0x88 - built once at the AI's start from
// the world's food units of class 0x11 and only ever pruned, so animals born or come later are never hunted), forage
// farther out, hunting farther out, then field foundations or fishing boats. They fill only the food target, and the
// assignment never takes a gatherer back: citizens sent off food (to a mine as leftovers, after a raid sent them
// home) stayed there - 21 food gatherers down to 1 at minute 20 with 20 on gold (1400 gold unused), target 30.
// Bits:
// 1 PLAN: before farms the worker plan (econ_targets) counts the citizens' food - the Town Center trains them
//   before the epoch is paid: 50 food a citizen for the next 10 while the citizens are short of what the AI wants -
//   and the wood demand only as far as real buildings need it (the queued ones, as LATE WOOD; the game's demand of
//   refused requests ran to 4500 wood in epoch 0 and took half the workers).
// 2 PULL: every assignment (10 s) up to 4 gatherers move to food from the list farthest above its target while food
//   is 2+ under its target: to an animal (bit 8) or the nearest free bush.
// 4 NEAR: the forage search takes the free bush nearest to a food drop site (Town Center, Capitol, Settlement,
//   Granary) within the pass's reach, with the game's checks (explored, gatherer cap, the unit's continent, no enemy
//   near, known towers on the way) - as mine_choice does for mines.
// 8 HUNT: before the game's passes, idle citizens go hunting - every animal (the hunt list or not) within
//   food_hunt_range (15) tiles of a drop site that takes hunters (Town Center, Capitol, Settlement), up to 3 hunters an
//   animal, the nearest to a drop site first; the fierce ones (tiger, wolf) are left alone. Before farms only.
// Measured against the base AI (hard, 60 min, 20 seeds x both seats, paired with the shipped build; docs/economy.md
// "Food"): PLAN + PULL (3, the default) on Continental e0 - the Stone Age 7.7 / 8.7 minutes earlier (no cheat / the
// cheat), workers +9.1 / +6.5 at minute 30, food gatherers 15-19 instead of 9-11 (the base AI: 10-21), the base AI's
// wins 0 -> 1 / 4 -> 4, the mod's 13 -> 22 / 1 -> 2; on the islands (e5, only PULL acts) the same within the noise.
// HUNT alone cost workers (-3 / -4 at 30 min): 23 of 28 citizens chased 15 animals (2300 food) across the map, and the
// food gathered by minute 20 rose 5% with twice the food gatherers - the walk decides wild food's rate. NEAR and CAMP
// (below) gave more workers but no better fights. Off.
internal int g_food_plan = 3, g_v_food_plan = -1, g_hunt_range = 15;   // [ai_base] food_hunt_range: tiles from a drop site
#define FOOD_PLAN(idx) PV(idx, g_v_food_plan, g_food_plan)
internal struct { unsigned pulled, hunt_sent, near_pick; } g_fd[16];

internal void food_new_game(void);

internal int food_fierce(int t) { return t == UT(RES_TIGER) || t == UT(RES_WOLF); }
// a free animal to hunt for unit u (NULL: the best for any): within food_hunt_range tiles of a non-granary drop site, food left,
// not fierce, under 3 hunters, explored, the unit's continent, no enemy near, no known towers on the way
internal ee_unit *food_animal(ee_player *pl, ee_resource_mgr *rm, ee_unit *u) {
    ee_civ_mgr *cm = resource_mgr_get_owner(rm);
    void *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!bm) return NULL;
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    drops_cached(pl);
    short ur = u ? (short)map_area(unit_get_tile(u), 2) : 0;
    ee_unit *best = NULL;
    int bd = g_hunt_range * g_hunt_range + 1;
    for (int i = 0; i < g_drops[pi].n; i++) {
        const struct drop *d = &g_drops[pi].d[i];
        if (d->food) continue;
        for (struct grid_it it = grid_it_box(d->p.x, d->p.y, g_hunt_range, 1); grid_it_next(&it); ) {
            ee_unit *a = it.u;
            ee_unit_type *td = it.td;
            if (!td || unit_get_resource(a) != 2 || unit_get_amount(a) <= 0 || unit_type_get_family(td) != KIND_ANIMAL) continue;
            if (food_fierce(unit_type_get_id(td))) continue;
            ivec2 pos = unit_get_tile(a);
            int d2 = ivec2_dist2(pos, d->p);
            if (d2 >= bd) continue;
            if (site_gatherers(a) >= 3) continue;
            if (u && (short)map_area(pos, 2) != ur) continue;
            if (build_mgr_enemy_near(bm, pos.x, pos.y) || site_danger(pl, a, 1)) continue;
            best = a; bd = d2;
        }
    }
    return best;
}
// the free bush nearest to a food drop site (NEAR), within max_d2 of the base; NULL: none
internal ee_unit *food_bush(ee_player *pl, ee_resource_mgr *rm, ee_unit *u, int max_d2) {
    ee_civ_mgr *cm = resource_mgr_get_owner(rm);
    void *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!bm) return NULL;
    ivec2 base = build_mgr_get_base(bm);
    if (base.x == -10000 || base.y == -10000) return NULL;
    unsigned pi = (unsigned)player_get_index(pl) & 15;
    drops_cached(pl);
    unsigned cap = table_value(0x16);
    short ur = u ? (short)map_area(unit_get_tile(u), 2) : 0;
    ee_unit *best = NULL;
    int bd = 0x7FFFFFFF;
    ee_zone **zb = player_get_zones_begin(pl); ee_zone **ze = player_get_zones_end(pl);
    for (int z = 0; zb && zb + z < ze && z < 256; z++) {
        ee_zone *zone = zb[z];
        void **sb = zone ? zone_get_sites_begin(zone) : NULL, **se = zone ? zone_get_sites_end(zone) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096; k++) {
            ee_unit *s = sb[k];
            ee_unit_type *td = s ? unit_get_type_data(s) : NULL;
            if (!td || unit_get_resource(s) != 2 || unit_type_get_id(td) != UT(RES_FORAGE_BUSH) || unit_get_amount(s) <= 0) continue;
            if (unit_vis_get_flag(unit_get_vis(s)) == 1) continue;
            ivec2 pos = unit_get_tile(s);
            if (ivec2_dist2(pos, base) >= max_d2) continue;
            int dd = drop_nearest(pi, pos, 1, NULL);   // to the nearest drop site that takes food
            if (dd >= bd) continue;
            if (!explored_by(s, pl)) continue;
            if (site_gatherers(s) >= cap) continue;
            if (u && (short)map_area(pos, 2) != ur) continue;
            if (build_mgr_enemy_near(bm, pos.x, pos.y) || site_danger_res(pl, s, 1, 2)) continue;
            best = s; bd = dd;
        }
    }
    return best;
}
// hook_find_food (sites.c) with NEAR: the nearest-to-a-drop bush within the pass's reach (near: 2 x +0xE10, else 6 x)
internal ee_unit *food_near_pick(ee_resource_mgr *rm, int near_base, ee_unit *unit) {
    ee_player *pl = resource_mgr_player(rm);
    if (!pl) return NULL;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    if (!(FOOD_PLAN(idx) & 4)) return NULL;
    int r6 = player_get_dropoff_distance(pl);
    ee_unit *s = food_bush(pl, rm, unit, near_base ? r6 * r6 * 4 : r6 * r6 * 36);
    if (s) g_fd[idx].near_pick++;
    return s;
}
// before the assignment (hook_assign_citizens): idle citizens to animals (HUNT, before farms)
internal void food_hunt(ee_resource_mgr *rm) {
    ee_player *pl = resource_mgr_player(rm);
    if (!pl) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    if (!(FOOD_PLAN(idx) & 8) || current_epoch(pl) >= FARM_EPOCH) return;
    uint32_t *target = resource_mgr_target_at(rm);
    int room = (int)target[2] - rm_count(rm, 2);
    uint8_t *dq = resource_mgr_list_at(rm, 0);   // list 0: idle
    for (unsigned k = deque_count(dq), first = deque_front(dq); k-- > 0 && room > 0; ) {
        ee_unit *u = deque_at(dq, first + k);
        if (!u || unit_get_hp(u) <= 0 || unit_get_frozen(u) || unit_get_ai_held(u) || unit_get_busy(u)) continue;
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || !is_citizen_type(unit_type_get_id(td)) || unit_get_goal(u)) continue;
        ee_unit *a = food_animal(pl, rm, u);
        if (!a) break;
        int before = rm_count(rm, 2);
        resource_mgr_assign(rm, u, a);
        if (rm_count(rm, 2) == before) break;
        g_fd[idx].hunt_sent++;
        room--;
    }
}
// after the assignment (hook_assign_citizens, after econ_rebalance): PULL
internal void food_pull(ee_resource_mgr *rm) {
    ee_player *pl = resource_mgr_player(rm);
    if (!pl) return;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    if (!(FOOD_PLAN(idx) & 2)) return;
    uint32_t *target = resource_mgr_target_at(rm);
    int pre = current_epoch(pl) < FARM_EPOCH, moves = 0, dry = 0, r6 = player_get_dropoff_distance(pl);
    while (moves < 4 && (int)target[2] - rm_count(rm, 2) >= 2) {
        int d = -1, best = 1;
        for (int t = 3; t < 7; t++) {
            int s = rm_count(rm, t) - (int)target[t];
            if (!(dry >> t & 1) && s > best) { best = s; d = t; }
        }
        if (d < 0) break;
        ee_unit *u = movable_gatherer(rm, d);
        if (!u) { dry |= 1 << d; continue; }
        ee_unit *site = pre && (FOOD_PLAN(idx) & 8) ? food_animal(pl, rm, u) : NULL;
        if (!site) site = food_bush(pl, rm, u, r6 * r6 * 36);
        if (!site) break;   // no free, safe food site in reach (fields: the fields pass)
        int before = rm_count(rm, 2);
        resource_mgr_assign(rm, u, site);
        if (rm_count(rm, 2) == before) { dry |= 1 << d; continue; }
        g_fd[idx].pulled++;
        moves++;
    }
}
// PLAN (econ_targets, before farms): the citizens' food and the real wood need
internal void food_plan_needs(ee_player *pl, ee_resource_mgr *rm, int *need, const int *stock, const int *reserve) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    if (!(FOOD_PLAN(idx) & 1)) return;
    int wf = workforce(pl), want = (int)(player_get_citizen_share(pl) * (float)player_get_pop_cap(pl));
    if (copper_mode(pl) == 1 && want > g_min_workforce) want = g_min_workforce;   // the plan saves from there on
    int more = want - wf;
    if (more > 10) more = 10;
    if (more > 0) need[2] += 50 * more;
    void *cm = resource_mgr_get_owner(rm);
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    int queued = 0, nq = 0;
    for (int e = 0; bm && e <= current_epoch(pl) && e < 14 && nq < 3; e++)
        for (unsigned i = 0; nq < 3; i++) {
            int *ent = bq_entry(bm, e, i);
            if (!ent) break;
            int c = unit_cost(pl, ent[0], 3);
            if (c > 0) { queued += c; nq++; }
        }
    int w = reserve[3] > queued ? reserve[3] : queued;
    w += field_wood(idx) + 100;
    if (need[3] > w - stock[3]) need[3] = w - stock[3] > 0 ? w - stock[3] : 0;
}
// 32 CAMP (before farms): a Settlement (a drop site for everything) by the richest wild food the town's drop sites are
//   far from - every 2 minutes while none is queued or being built and the AI can pay for one (0x555710), 2 a game: the
//   bushes and animals within 30 tiles of the base, each counted with the food within 6 tiles of it, those 10+ tiles
//   from every drop site; 1500+ food there -> a placeable spot 2-4 tiles from it without enemy soldiers within 12,
//   queued on the epoch's build queue with its position. The walk decides wild food's rate: with twice the food
//   gatherers (HUNT, PULL) the food gathered by minute 20 rose 5%.
internal struct { uint32_t t; int n; } g_camp[16];
internal void food_camp(ee_player *pl, unsigned idx, uint32_t now) {
    if (!(FOOD_PLAN(idx) & 32) || current_epoch(pl) >= FARM_EPOCH) return;
    if (now < g_camp[idx].t) memset(&g_camp[idx], 0, sizeof g_camp[idx]);
    if (g_camp[idx].n >= 2 || (g_camp[idx].t && now - g_camp[idx].t < 120000)) return;
    g_camp[idx].t = now ? now : 1;
    ee_civ_mgr *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    void *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    if (!bm || !um || !ut_has(UT(B_SETTLEMENT)) || build_mgr_get_base_x(bm) == -10000 || bq_has(bm, UT(B_SETTLEMENT)) || bq_has(bm, UT(X_SINGLE_PLAYER_SETTLEMENT))) return;
    uint8_t *q = type_list(um, UT(B_SETTLEMENT));
    for (unsigned k = 0, f = q ? deque_front(q) : 0, n = q ? deque_count(q) : 0; k < n; k++) {
        ee_unit *b = deque_at(q, f + k), *st = b ? unit_get_physics(b) : NULL;
        if (b && unit_get_hp(b) > 0 && (!st || !unit_get_finished(st))) return;
    }
    if (!can_build(pl, UT(B_SETTLEMENT))) return;
    ivec2 base = build_mgr_get_base(bm);
    static ivec2 fp[512];   // the food's tiles, ff its amounts
    static int ff[512];
    int n = 0;
    for (struct grid_it it = grid_it_box(base.x, base.y, 30, 1); grid_it_next(&it) && n < 512; ) {   // animals
        ee_unit *a = it.u;
        ee_unit_type *td = it.td;
        if (!td || unit_get_resource(a) != 2 || unit_get_amount(a) <= 0 || unit_type_get_family(td) != KIND_ANIMAL || food_fierce(unit_type_get_id(td))) continue;
        fp[n] = unit_get_tile(a); ff[n++] = unit_get_amount(a);
    }
    ee_zone **zb = player_get_zones_begin(pl); ee_zone **ze = player_get_zones_end(pl);   // bushes
    for (int z = 0; zb && zb + z < ze && z < 256; z++) {
        void **sb = zb[z] ? zone_get_sites_begin(zb[z]) : NULL, **se = zb[z] ? zone_get_sites_end(zb[z]) : NULL;
        for (int k = 0; sb && sb + k < se && k < 4096 && n < 512; k++) {
            ee_unit *st = sb[k];
            ee_unit_type *td = st ? unit_get_type_data(st) : NULL;
            if (!td || unit_get_resource(st) != 2 || unit_type_get_id(td) != UT(RES_FORAGE_BUSH) || unit_get_amount(st) <= 0) continue;
            ivec2 p = unit_get_tile(st);
            if (ivec2_dist2(p, base) > 900) continue;
            if (!explored_by(st, pl)) continue;
            fp[n] = p; ff[n++] = unit_get_amount(st);
        }
    }
    unsigned pi = idx & 15;
    drops_cached(pl);
    int best = -1, bf = 1499;
    for (int i = 0; i < n; i++) {
        int dd = drop_nearest(pi, fp[i], 1, NULL);
        if (dd < 100 || build_mgr_enemy_near(bm, fp[i].x, fp[i].y)) continue;
        int sum = 0;
        for (int j = 0; j < n; j++) if (ivec2_dist2(fp[j], fp[i]) <= 36) sum += ff[j];
        if (sum > bf) { bf = sum; best = i; }
    }
    if (best < 0) return;
    int e = current_epoch(pl);
    for (int r = 2; r <= 4; r++)
        for (int ax = -r; ax <= r; ax++)
            for (int ay = -r; ay <= r; ay++) {
                if (abs(ax) != r && abs(ay) != r) continue;
                int x = fp[best].x + ax, y = fp[best].y + ay;
                if (!can_place(pl, UT(B_SETTLEMENT), iv2(x, y)) || raiders_near(pl, iv2(x, y), 12)) continue;
                int entry[3] = { UT(B_SETTLEMENT), x, y };
                build_mgr_queue_add(bm, e, entry);
                g_camp[idx].n++;
                if (g_log) {
                    EnterCriticalSection(&g_lock);
                    fprintf(g_log, "# food: P%u at %.1f min: a Settlement queued at %d,%d by %d wild food (cost F%d W%d S%d G%d I%d)\n", idx,
                            now / 60000.0, x, y, bf, unit_cost(pl, UT(B_SETTLEMENT), 2), unit_cost(pl, UT(B_SETTLEMENT), 3), unit_cost(pl, UT(B_SETTLEMENT), 4),
                            unit_cost(pl, UT(B_SETTLEMENT), 5), unit_cost(pl, UT(B_SETTLEMENT), 6));
                    LeaveCriticalSection(&g_lock);
                }
                return;
            }
}
internal void food_log(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16];
    food_camp(pl, idx, now);
    if (!g_log || !g_autotest || (!g_food_plan && g_v_food_plan < 0)) return;
    if (now / 300000 == last[idx] / 300000 && last[idx]) return;
    last[idx] = now ? now : 1;
    (void)pl;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# food: P%u at %u min: plan %d, pulled %u, hunters sent %u, nearest bushes %u\n", idx, now / 60000,
            FOOD_PLAN(idx), g_fd[idx].pulled, g_fd[idx].hunt_sent, g_fd[idx].near_pick);
    LeaveCriticalSection(&g_lock);
}
internal void food_install(const char *autotest_ini) {
    g_food_plan = g_set_ai[1].food_plan;
    g_hunt_range = g_set_ai[1].food_hunt_range;
    if (autotest_ini) g_v_food_plan = ini_int("autotest", "v_food_plan", -1, autotest_ini);
}
// 16 WILD (econ_fields, from the Copper Age on): no builders for field foundations while wild food is free in reach -
//   a free bush or animal (as the game's own food passes take them before fields), or on a water map with fish
//   (player +0xEFA) 3000+ fish food known while the fishing boats are under 8 or keep up (food 4+ under its target with
//   8 boats: fields). The game itself builds fields only for the shortfall left after wild food; the mod's field pass
//   built every foundation at once (islands e5: 8 fields by minute 10 and no fishing boat until minute 20, the base AI
//   6 boats and no field). Measured (islands e5, food_plan 19 against 3): the fields waited until minute 25-30, but the
//   game's fishing boats came no sooner (0-1 until minute 20) - workers -7 to -11 at minute 30, the base AI's wins on
//   Small Islands 2 -> 5 of 40. Fish comes too slowly to replace the fields; off.
internal int food_fields_wait(ee_player *pl, ee_resource_mgr *rm) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    if (!(FOOD_PLAN(idx) & 16) || current_epoch(pl) < FARM_EPOCH) return 0;
    int r6 = player_get_dropoff_distance(pl);
    if (food_bush(pl, rm, NULL, r6 * r6 * 36) || food_animal(pl, rm, NULL)) return 1;
    if (!player_get_boats_for_food(pl)) return 0;
    int fish = 0, boats = 0;
    void **qb = resource_mgr_get_fish_begin(rm), **qe = resource_mgr_get_fish_end(rm);
    for (void **h = qb; qb && h < qe && h - qb < 4096; h++)
        if (*h && unit_get_amount(*h) > 0) fish += unit_get_amount(*h);
    if (fish < 3000) return 0;
    uint8_t *list = resource_mgr_list_at(rm, 2);
    for (unsigned k = 0, first = deque_front(list), n = deque_count(list); k < n && k < 1024; k++) {
        ee_unit *u = deque_at(list, first + k); ee_unit_type *td = u ? unit_get_type_data(u) : NULL;
        if (td && unit_get_hp(u) > 0 && unit_type_get_class_id(td) == CLS_NAVAL_CITIZEN) boats++;
    }
    int short_ = (int)resource_mgr_get_target(rm, 2) - rm_count(rm, 2);
    return !(boats >= 8 && short_ >= 4);
}
internal void food_new_game(void) { memset(g_fd, 0, sizeof g_fd); memset(g_camp, 0, sizeof g_camp); }
