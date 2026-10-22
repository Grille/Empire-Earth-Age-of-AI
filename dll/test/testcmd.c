// TEST COMMANDS: control-pipe commands that only tests use - spawning, giving, hurting, moving units, forcing an
// attack or a call-off, scripted armies (wave, charge), and looks into the game's data (techs, type data, unit
// info). Included by eeaimod.c last (it uses the statics of every feature file); dispatched by control.c's execute
// (its forward declarations). The commands that show a feature's own state (attacks, intel, landing, defence ...)
// stay with their feature.

// ---- from econ.c ----
// epochup P: player P - any, the tests' idle human too - gets its next epoch at once, unpaid (FN_EPOCH_NEXT: the tech
// tree's epoch, its event and what becomes available; the epoch tech itself stays unresearched - FN_TECH_GRANT refuses
// epoch techs, category 3). For epoch_pacing=follow's test: the human ahead at a set minute.
internal void cmd_epochup(char *out, const char *args) {
    int p = atoi(args);
    ee_player *pl = player_at(p);
    ee_tech_tree *tt = pl ? player_get_tech_tree(pl) : NULL;
    if (!tt) { REPLY("error: no player %d\n", p); return; }
    int e = tt_get_epoch(tt);
    epoch_next(tt);
    REPLY("P%d epoch %d -> %d\n", p, e, tt_get_epoch(tt));
}
// tech P id...: the player's tech record (tech tree lookup 0x4019A1): first bytes and the data's cost
internal void cmd_tech(char *out, const char *args) {
    int p = 0, n = 0, id;
    sscanf(args, "%d%n", &p, &n);
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    ee_tech_tree *tt = pl ? player_get_tech_tree(pl) : NULL;
    if (!tt) { REPLY("error: no player %d\n", p); return; }
    args += n;
    while (sscanf(args, "%d%n", &id, &n) == 1) {
        args += n;
        ee_tech *t = tech_lookup(tt, id);
        if (!t) { REPLY("%d: none\n", id); continue; }
        REPLY("%d:", id);
        for (int i = 0; i < 0x20; i++) REPLY(" %02x", ((const uint8_t *)t)[i]);
        ee_tech_data *d = tech_get_data(t);
        if (d) REPLY(" | cost %d,%d,%d,%d,%d", techdata_get_cost(d, 2), techdata_get_cost(d, 3), techdata_get_cost(d, 4), techdata_get_cost(d, 5), techdata_get_cost(d, 6));
        int ei = d ? techdata_get_effects(d) : -1;
        REPLY(" | effects %d:", ei);
        if (ei >= 0 && ei <= 0x5DC) {   // effect list 0x93B1B8 + ei*0x10 (+4 ids, +8 count), effect 0x919E70 + id*0x44
            ee_epoch_techs *list = epoch_tech_list(ei);
            unsigned count = effects_get_count(list), *ids = effects_get_ids(list);
            for (unsigned k = 0; ids && k < count && k < 16; k++) {
                if (ids[k] >= 2000) { REPLY(" %u?", ids[k]); continue; }
                ee_effect *ef = tech_effect(ids[k]);
                REPLY(" %u(%x/%x/%x)", ids[k], effect_get_type(ef), effect_get_kind(ef), effect_get_attr(ef));
            }
        }
        REPLY("\n");
    }
}

// techs P: the techs player P has researched (tech record +4), and which are new since the last call - with their
// tech epoch (record +0x14) and cost; tests: what an AI gets at an epoch advance
internal void cmd_techs(char *out, const char *args) {
    static uint8_t seen[16][16384];
    int p = atoi(args), total = 0;
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    ee_tech_tree *tt = pl ? player_get_tech_tree(pl) : NULL;
    if (!tt) { REPLY("error: no player %d\n", p); return; }
    REPLY("P%d epoch %d, new:", p, tt_get_epoch(tt));
    for (int id = 0; id < 16384; id++) {
        ee_tech *t = tech_lookup(tt, id);
        int done = t && tech_get_researched(t);
        total += done;
        if (done && !seen[p][id] && strlen(out) < 15000) {
            ee_tech_data *d = tech_get_data(t);
            REPLY(" %d(e%d", id, tech_get_epoch(t));
            if (d) REPLY(" %d/%d/%d/%d/%d", techdata_get_cost(d, 2), techdata_get_cost(d, 3), techdata_get_cost(d, 4), techdata_get_cost(d, 5), techdata_get_cost(d, 6));
            REPLY(")");
        }
        seen[p][id] = (uint8_t)done;
    }
    REPLY(" - %d researched\n", total);
}

// watchtech P id: hardware write-watch on player P's tech record +4 (researched): who researches it. Tests.
internal void watch_set(uintptr_t addr, int len);
internal void cmd_watchtech(char *out, const char *args) {
    int p = 0, id = 0;
    sscanf(args, "%d %d", &p, &id);
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    ee_tech_tree *tt = pl ? player_get_tech_tree(pl) : NULL;
    ee_tech *t = tt ? tech_lookup(tt, id) : NULL;
    if (!t) { REPLY("error: no tech %d for P%d\n", id, p); return; }
    watch_set((uintptr_t)tech_researched_at(t), 1);
    REPLY("ok: watching tech %d of P%d at %p (researched %d)\n", id, p, (void *)tech_researched_at(t), tech_get_researched(t));
}

// rlist P: the research manager's per-epoch tech lists (cm+0x40: +0x18 + e*0x10 = vector begin/end of tech ids,
// +0x114 current epoch); * = researched (tech +4)
internal void cmd_rlist(char *out, const char *args) {
    int p = atoi(args);
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    void *cm = pl ? player_get_civ_mgr(pl) : NULL;
    ee_research_mgr *rs = cm ? cm_get_research_mgr(cm) : NULL; ee_tech_tree *tt = pl ? player_get_tech_tree(pl) : NULL;
    if (!rs || !tt) { REPLY("error: no AI player %d\n", p); return; }
    int cur = rs_get_current(rs);
    REPLY("P%d research epoch %d\n", p, cur);
    for (int e = -1; e <= 13; e++) {   // -1: the list at +8
        int *b = rs_get_epoch_begin(rs, e), *en = rs_get_epoch_end(rs, e);
        if (!b || en <= b || en - b > 500) continue;
        REPLY("list %d:", e);
        for (int *q = b; q < en; q++) {
            ee_tech *t = tech_lookup(tt, *q);
            REPLY(" %d%s", *q, t && tech_get_researched(t) ? "*" : "");
        }
        REPLY("\n");
    }
}

// tinfo P type...: type data of unit types - family +0x4B4, hit points +0x55C, attack +0x510, range +0x420 - and
// whether player P could train/build one now (0x555710: available, affordable, a building/builder)
internal void cmd_tinfo(char *out, const char *args) {
    int p = 0, n = 0, t;
    sscanf(args, "%d%n", &p, &n);
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    if (!pl) { REPLY("error: no player %d\n", p); return; }
    args += n;
    while (sscanf(args, "%d%n", &t, &n) == 1) {
        args += n;
        ee_unit_type *td = type_data(t);
        if (!td) { REPLY("%d: none\n", t); continue; }
        REPLY("%d: family %d hp %d attack %d range %.1f can %d\n", t, type_get_family(td), type_get_hp(td),
              type_get_attack(td), type_get_range(td),
              can_build(pl, t));
    }
}

// drops P: the player's drop sites (x,y; g = granary, food only)
internal void cmd_drops(char *out, const char *args) {
    int p = atoi(args);
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    if (!pl) { REPLY("error: no AI player %d\n", p); return; }
    static int d[64][3];
    int nd = drop_sites(pl, d, 64);
    REPLY("P%d drop sites:", p);
    for (int i = 0; i < nd; i++) REPLY(" %d,%d%s", d[i][0], d[i][1], d[i][2] ? "g" : "");
    REPLY("\n");
}

// ---- from attack.c ----
// attacknow P (test): AI P's next attack is due now (player +0xED8 = the game time) - the game launches it at its next
// look, if it has a target
internal void cmd_attacknow(char *out, const char *args) {
    int p = atoi(args);
    ee_player *pl = (p > 0 && p < 16) ? g_players[p] : NULL;
    if (!pl) { REPLY("error: no AI player %d\n", p); return; }
    uint32_t was = player_get_next_attack(pl);
    player_set_next_attack(pl, game_time());
    REPLY("P%d: next attack %.1f -> %.1f min\n", p, was / 60000.0, game_time() / 60000.0);
}

// nullmember P (test): the first member record (of an attack subgroup of AI P with 2+ members) loses its unit (+0x44
// = 0) - the state the engine guards (hook_group_stat, asg_purge) are for
internal void cmd_nullmember(char *out, const char *args) {
    int p = atoi(args);
    ee_player *pl = (p > 0 && p < 16) ? g_players[p] : NULL;
    ee_combat_mgr *cbm = pl ? player_get_combat_mgr(pl) : NULL; ee_attack_cmd *cmd = cbm ? am_get_commander(cbm) : NULL;
    if (!cmd) { REPLY("error: no AI player %d with an attack commander\n", p); return; }
    uint8_t *dq = ac_groups_at(cmd);
    unsigned first = FIELD(dq, 0xC, unsigned), cnt = FIELD(dq, 0x10, unsigned);
    for (unsigned k = 0; k < cnt && k < 64; k++) {
        ee_attack_group *g = deque_at(dq, first + k);
        for (ee_attack_subgroup **sp = g ? ag_get_subgroups_begin(g) : NULL; sp && sp < ag_get_subgroups_end(g); sp++) {
            int n = 0, cit = 0;
            if (!*sp) continue;
            subgroup_count(*sp, &n, &cit);
            ee_asg_unit **b = asg_get_units_begin(*sp);
            if (n < 2 || !b || !*b) continue;
            ee_unit *u = asgu_get_unit(*b);
            asgu_set_unit(*b, NULL);
            REPLY("P%d subgroup %p (vt %06x, %d members): record %p lost its unit %p\n", p, (void *)*sp,
                  *(unsigned *)*sp, n, (void *)*b, (void *)u);
            ee_player *target = ag_get_target(g);
            if (strstr(args, "off") && target) {   // and the attack called off at once, as diplo_safe_point does
                breach_forget(pl);
                asg_purge_all(pl);
                call_off_attacks(pl, target, 1);
                REPLY("P%d's attack on P%d called off\n", p, player_get_index(target));
            }
            return;
        }
    }
    REPLY("P%d: no attack subgroup with 2+ members (groups %u)\n", p, cnt);
}

// Enemy buildings of AI player P (all, seen or not): owner, type, tile, seen by P, known threat on them.
internal void cmd_targets(char *out, const char *args) {
    int p = atoi(args);
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    if (!pl) { REPLY("error: no AI player %d\n", p); return; }
    threat_refresh(pl, 0);
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid) { REPLY("error: no map\n"); return; }
    unsigned n = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (!td || (type_get_family(td) != 2 && type_get_family(td) != 29) || !is_enemy(pl, unit_get_owner(u)))
            continue;
        int x = unit_get_x(u), y = unit_get_y(u);
        if (strlen(out) > 15000) { REPLY("...\n"); return; }
        REPLY("  type %3d of P%d at %d,%d %s threat %.1f\n", type_get_id(td),
              player_get_index(unit_get_owner(u)), x, y, seen_by(u, pl) ? "seen  " : "unseen", threat_at(p, x, y));
        n++;
    }
    REPLY("P%d: %u enemy buildings\n", p, n);
}

// spawn <player> <type> <x> <y> [count]: create units in a running game the way the game's own debug spawner does
// (0x64C561): an EEMHackANewUnit message (operator new 0x683718, 0x20 bytes; ctor 0x650B3C thiscall(int *type,
// float xy[2], char player)) handed to the lockstep server ([0x91984C] slot 6: (msg, 0xFF)). The simulation
// executes it in a coming tick (EEMHackANewUnit::vf10 0x4B7044: ground height, buildings checked against the
// map) - like any player command. Several units go on a grid of 2 tiles around x, y.
// give P amount R (test): player P's stock of resource R grows by amount - R 1 food .. 5 iron, 0 each of them (a
// script: resource.gold, resource.all)
internal void cmd_give(char *out, const char *args) {
    int p = 0, amount = 0, r = -1;
    ee_player *pl = sscanf(args, "%d %d %d", &p, &amount, &r) == 3 && p >= 0 && p < 16 ? g_players[p] : NULL;
    if (!pl || r < 0 || r > 5) { REPLY("error: give P amount R (R: 0 all, 1 food .. 5 iron - resource.all, resource.gold ...)\n"); return; }
    int *stock = player_stock_at(pl);
    for (int k = 2; k < 7; k++) if (!r || k == r + 1) stock[k] += amount;
    REPLY("P%d: stock %d/%d/%d/%d/%d\n", p, stock[2], stock[3], stock[4], stock[5], stock[6]);
}

// train P type [count] (test): AI P orders count (default 1) units of a type through its own unit manager, as a
// priority order (the stock above the epoch reserve) - unlike spawn, the units come from its factories and join its pool
internal void cmd_train(char *out, const char *args) {
    int p = 0, type = 0, count = 1;
    sscanf(args, "%d %d %d", &p, &type, &count);
    ee_player *pl = (p > 0 && p < 16) ? g_players[p] : NULL;
    void *cm = pl ? player_get_civ_mgr(pl) : NULL, *um = cm ? cm_get_unit_mgr(cm) : NULL;
    if (!um || type <= 0) { REPLY("error: train P type [count] (AI player)\n"); return; }
    int req[4] = { type, count, 0, 0 };
    unsigned got = 0;
    g_priority_mil = 1;
    hook_train_units(um, req, (uintptr_t)&got);
    g_priority_mil = 0;
    REPLY("P%d: %u of %d x type %d ordered\n", p, got, count, type);
}

internal void cmd_spawn(char *out, const char *args) {
    int p, type, x, y, n = 1;
    if (sscanf(args, "%d %d %d %d %d", &p, &type, &x, &y, &n) < 4 || p < 0 || p > 15 || type < 0 || type >= 0x300 || n < 1 || n > 100) {
        REPLY("error: usage: spawn <player> <type> <x> <y> [count]\n");
        return;
    }
    void *srv = game_server();
    if (!type_data(type) || !srv) { REPLY("error: no type %d or no game\n", type); return; }
    {   // off the map the game crashes (150,140 on a 128-tile map)
        ee_map *map = game_map(); ee_grid *grid = map ? map_get_grid(map) : NULL;
        int w = grid ? grid_get_cell_size(grid) * grid_get_nx(grid) : 0, h = grid ? grid_get_cell_size(grid) * grid_get_ny(grid) : 0;
        if (x < 1 || y < 1 || x >= w - 2 || y >= h - 2) { REPLY("error: %d,%d is off the map (%d x %d)\n", x, y, w, h); return; }
    }
    int side = 1;
    while (side * side < n) side++;
    for (int i = 0; i < n; i++) {
        float xy[2] = { (float)(x + (i % side) * 2) + 0.5f, (float)(y + (i / side) * 2) + 0.5f };
        void *msg = new_unit_msg_alloc();
        if (!msg) { REPLY("error: out of memory\n"); return; }
        new_unit_msg_init(msg, &type, xy, (char)p);
        server_send_all(srv, msg);
    }
    REPLY("ok: %d x type %d (%s) for P%d at %d,%d queued\n", n, type, unit_ident(type), p, x, y);
}

// spawnline <player> <type> <x0> <y0> <x1> <y1> [step]: one unit every <step> tiles (default 1) along the line,
// e.g. a wall. Same spawner as spawn.
internal void cmd_spawnline(char *out, const char *args) {
    int p, type, x0, y0, x1, y1, step = 1;
    if (sscanf(args, "%d %d %d %d %d %d %d", &p, &type, &x0, &y0, &x1, &y1, &step) < 6 || p < 0 || p > 15 || type < 0 ||
        type >= 0x300 || step < 1) {
        REPLY("error: usage: spawnline <player> <type> <x0> <y0> <x1> <y1> [step]\n");
        return;
    }
    void *srv = game_server();
    if (!type_data(type) || !srv) { REPLY("error: no type %d or no game\n", type); return; }
    int dx = x1 - x0, dy = y1 - y0, len = abs(dx) > abs(dy) ? abs(dx) : abs(dy), n = 0;
    for (int i = 0; i <= len; i += step) {
        float xy[2] = { (float)x0 + (len ? (float)(dx * i) / (float)len : 0) + 0.5f,
                        (float)y0 + (len ? (float)(dy * i) / (float)len : 0) + 0.5f };
        void *msg = new_unit_msg_alloc();
        if (!msg) break;
        new_unit_msg_init(msg, &type, xy, (char)p);
        server_send_all(srv, msg);
        n++;
    }
    REPLY("ok: %d x type %d (%s) for P%d from %d,%d to %d,%d queued\n", n, type, unit_ident(type), p, x0, y0, x1, y1);
}

// bqueue P: the build manager's per-epoch queues (bq_entry); $ = can be built now (0x555710, the build manager's
// check: available in this epoch, affordable, a builder)
internal void cmd_bqueue(char *out, const char *args) {
    int p = atoi(args);
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    void *cm = pl ? player_get_civ_mgr(pl) : NULL;
    ee_build_mgr *bm = cm ? cm_get_build_mgr(cm) : NULL;
    if (!bm) { REPLY("error: no AI player %d\n", p); return; }
    for (int e = 0; e < 14; e++) {
        int *ent = bq_entry(bm, e, 0);
        if (!ent) continue;
        REPLY("epoch %d:", e);
        for (unsigned i = 1; ent && i <= 60; ent = bq_entry(bm, e, i++))
            REPLY(" %d%s", ent[0], can_build(pl, ent[0]) ? "$" : "");
        REPLY("\n");
    }
}

// canbuild P: every type the game lets player P build or train now (0x555710: available in this epoch, affordable,
// a builder) - which types of a family or class the data really offers (the Missile Base, campaign units)
internal void cmd_canbuild(char *out, const char *args) {
    int p = atoi(args);
    ee_player *pl = (p >= 0 && p < 16) ? g_players[p] : NULL;
    if (!pl) { REPLY("error: no player %d\n", p); return; }
    REPLY("P%d can build:", p);
    for (int t = 0; t < G_TYPES_N && strlen(out) < 15000; t++)
        if (type_data(t) && can_build(pl, t))
            REPLY(" %d %s,", t, unit_ident(t));
    REPLY("\n");
}

// count P: all units and buildings of player P by type (type: count)
internal void cmd_count(char *out, const char *args) {
    int p = atoi(args);
    static int n[0x300];
    memset(n, 0, sizeof n);
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid) { REPLY("error: no map\n"); return; }
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        ee_player *owner = unit_get_owner(u);
        if (!td || !owner || player_get_index(owner) != p) continue;
        int t = type_get_id(td);
        if (t >= 0 && t < 0x300) n[t]++;
    }
    REPLY("P%d:", p);
    for (int t = 0; t < 0x300; t++) if (n[t]) REPLY(" %d:%d", t, n[t]);
    REPLY("\n");
}

// gridcheck: how the unit grid holds the units - per family: units, those in more than one cell (and the most cells),
// those not in the cell of their own position (a loop visiting each unit only there would miss them)
internal void cmd_gridcheck(char *out, const char *args) {
    (void)args;
    enum { HN = 16384 };
    static struct { ee_unit *u; int cells, own, fam; } h[HN];
    memset(h, 0, sizeof h);
    if (!game_map()) { REPLY("error: no map\n"); return; }
    int full = 0;
    for (struct grid_it it = grid_it_all(0); grid_it_next(&it); ) {
        unsigned k = ((uintptr_t)it.u >> 4) % HN, n = 0;
        while (h[k].u && h[k].u != it.u && n++ < HN) k = (k + 1) % HN;
        if (n >= HN) { full = 1; break; }
        if (!h[k].u) { h[k].u = it.u; h[k].fam = type_get_family(it.td); }
        h[k].cells++;
        if (unit_get_x(it.u) / it.cs == it.cx && unit_get_y(it.u) / it.cs == it.cy) h[k].own = 1;
    }
    static int nu[64], nm[64], mx[64], no[64];
    memset(nu, 0, sizeof nu); memset(nm, 0, sizeof nm); memset(mx, 0, sizeof mx); memset(no, 0, sizeof no);
    for (int k = 0; k < HN; k++) {
        if (!h[k].u) continue;
        int f = h[k].fam & 63;
        nu[f]++;
        if (h[k].cells > 1) nm[f]++;
        if (h[k].cells > mx[f]) mx[f] = h[k].cells;
        if (!h[k].own) no[f]++;
    }
    REPLY("gridcheck%s (family: units, in 2+ cells, most cells, not in own cell):", full ? " (table full)" : "");
    for (int f = 0; f < 64; f++) if (nu[f]) REPLY(" f%d:%d/%d/%d/%d", f, nu[f], nm[f], mx[f], no[f]);
    REPLY("\n");
}

// objs P T: every unit of type T of player P - tile, hit points / the type's, goal (tests: are buildings finished?)
internal void cmd_objs(char *out, const char *args) {
    int p = 0, t = -1;
    sscanf(args, "%d %d", &p, &t);
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid) { REPLY("error: no map\n"); return; }
    REPLY("P%d type %d:", p, t);
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        ee_player *owner = unit_get_owner(u);
        if (!td || !owner || player_get_index(owner) != p || (t >= 0 && type_get_id(td) != t) || strlen(out) > 15000) continue;
        if (t < 0) REPLY(" T%d", type_get_id(td));   // all types: each with its type
        REPLY(" [%d,%d hp %d/%d %s]", unit_get_x(u), unit_get_y(u), unit_get_hp(u),
              type_get_hp(td), rtti_name(unit_get_goal(u)));
    }
    REPLY("\n");
}

// found P T x y: lay out a building of type T for player P at x, y, unfinished (1 hp) - as the build manager does
// (0x62EF8A cdecl(player, type, x, y), which also pays its cost, except for fields). Tests: an enemy tower going up.
internal void cmd_found(char *out, const char *args) {
    int p, t, x, y;
    if (sscanf(args, "%d %d %d %d", &p, &t, &x, &y) < 4) { REPLY("error: usage: found P T x y\n"); return; }
    ee_player *pl = player_at(p);   // diplo.c: any player, human or AI
    if (!pl || t < 0 || t >= 724 || !type_data(t)) { REPLY("error: no player %d or type %d\n", p, t); return; }
    void *u = place_foundation(pl, t, x, y);
    REPLY("ok: type %d for P%d at %d,%d: %p\n", t, p, x, y, u);
}

// build P bx by x y r: player P's citizens within r tiles of x, y build (or repair) P's building at bx, by - the
// game's order (EEUGBuildRepair: new 0x5C, 0x60BAD0(goal, player, building, 1)). Tests: enemy builders at a tower.
internal void cmd_build(char *out, const char *args) {
    int p, bx, by, x, y, r, n = 0;
    if (sscanf(args, "%d %d %d %d %d %d", &p, &bx, &by, &x, &y, &r) < 6) { REPLY("error: usage: build P bx by x y r\n"); return; }
    ee_player *pl = player_at(p);
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!pl || !grid) { REPLY("error: no player %d or map\n", p); return; }
    ee_unit *bld = NULL;
    ee_unit *civ[32];
    int nc = 0;
    for (struct grid_it it = grid_it_box(x, y, r + 4, 1); grid_it_next(&it); ) {   // wide enough for the building too
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0) continue;
        int t = type_get_id(td), ux = unit_get_x(u), uy = unit_get_y(u);
        if (ux == bx && uy == by && (type_get_family(td) == 2 || type_get_family(td) == 29)) bld = u;
        else if (is_citizen_type(t) && abs(ux - x) <= r && abs(uy - y) <= r && nc < 32) {
            int dup = 0;
            for (int k = 0; k < nc; k++) dup |= civ[k] == u;
            if (!dup) civ[nc++] = u;
        }
    }
    if (!bld) { REPLY("error: no building of P%d at %d,%d\n", p, bx, by); return; }
    for (int k = 0; k < nc; k++) {
        void *mem = goal_build_alloc();
        ee_goal *g = mem ? goal_build_init(mem, pl, bld, 1) : NULL;
        n += unit_goal(civ[k], g, 0, 0x1F51);
    }
    REPLY("ok: %d citizens of P%d build at %d,%d\n", n, p, bx, by);
}

// watchtype P T [off]: hardware write-watch on the first unit of type T of player P, at offset off (default 0x2C,
// the type data: who changes a unit's type). Tests.
internal void watch_set(uintptr_t addr, int len);
internal void cmd_watchtype(char *out, const char *args) {
    int p = 0, t = 0;
    unsigned off = 0x2C;
    sscanf(args, "%d %d %x", &p, &t, &off);
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid) { REPLY("error: no map\n"); return; }
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        ee_player *owner = unit_get_owner(u);
        if (!td || !owner || player_get_index(owner) != p || type_get_id(td) != t) continue;
        watch_set((unsigned)(uintptr_t)FIELD_AT(u, off), 4);
        REPLY("ok: watching unit %p +%x (type %d of P%d at %d,%d)\n", u, off, t, p, unit_get_x(u), unit_get_y(u));
        return;
    }
    REPLY("error: no unit of type %d for P%d\n", t, p);
}

// hurt P T x y r dmg: damage every object of type T of player P within r tiles of x, y the way an attack does
// (0x61E0B2: target vf 0x70 thiscall(attacker, damage, attacker's player index, 1)); attacker = the object itself
// (tests: destroy farm fields, buildings)
internal void cmd_hurt(char *out, const char *args) {
    int p, t, x, y, r, dmg, n = 0;
    if (sscanf(args, "%d %d %d %d %d %d", &p, &t, &x, &y, &r, &dmg) < 6) { REPLY("error: usage: hurt P T x y r dmg\n"); return; }
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid) { REPLY("error: no map\n"); return; }
    ee_unit *hit[64];
    for (struct grid_it it = grid_it_box(x, y, r, 1); n < 64 && grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        ee_player *owner = u ? unit_get_owner(u) : NULL;
        if (!td || !owner || player_get_index(owner) != p || type_get_id(td) != t || unit_get_hp(u) <= 0) continue;
        if (abs(unit_get_x(u) - x) > r || abs(unit_get_y(u) - y) > r) continue;
        int dup = 0;
        for (int k = 0; k < n; k++) dup |= hit[k] == u;
        if (!dup) hit[n++] = u;
    }
    for (int k = 0; k < n; k++)   // collected first: a dying object may leave its cell
        (*(void (__thiscall **)(void *, void *, int, int, int))(FIELD(hit[k], 0, uint8_t *) + 0x70))(hit[k], hit[k], dmg, p, 1);
    REPLY("ok: %d objects hit for %d\n", n, dmg);
}

// Soldiers of player P (any player, AI or not) grouped by current goal (unit+0x1EC): count, centre, and the
// goal's first dwords of the first unit (to find what it is after).
// STRESS WAVES (tests): a grown AI against ever stronger attacks. "wave A D pct [x y]" gives player A (e.g. the idle
// P1) pct% of player D's army, type by type - the most of each type D has had at a wave so far (a mirror: the right
// epoch, a fair mix, and waves that keep growing while D's army shrinks; ships and aircraft left out), at x,y
// (default: beside A's buildings); A and D are made enemies (the test maps ally the idle human with every AI).
// "charge A D" (then every 10 s, at most 80 orders a pass): each soldier of A attacks the nearest soldier of D within 15 tiles, else the nearest citizen within
// 20, else D's nearest building (one already fighting a unit of D keeps it). The first runs went for the nearest
// building only: 500 archers at the walls razed 16 buildings in 45 minutes. Setup lines, e.g. "@60 wave 1 2 50",
// "@60.5 charge 1 2" ... - the minute D falls (game over) is the score.
struct w_scan { int n; ee_unit *u[1024]; };
internal void units_of(int p, int want, struct w_scan *o) {   // want: 0 soldiers (cmd_army's filter), 1 the rest
                                                             // (buildings, walls, towers), 2 citizens
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    o->n = 0;
    if (!grid) return;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        ee_player *owner = unit_get_owner(u);
        if (!td || !owner || player_get_index(owner) != p || unit_get_hp(u) <= 0) continue;
        int fam = type_get_family(td), ty = type_get_id(td);
        int civ = is_citizen_type(ty), sold = !civ && is_army_family(fam);
        if (want == 2 ? !civ : want == 1 ? civ || sold : !sold) continue;   // 1: the rest - buildings, walls, towers
        if (o->n < 1024) o->u[o->n++] = u;
    }
}
internal void cmd_wave(char *out, const char *args) {
    int a, d, pct, x = -1, y = -1;
    if (sscanf(args, "%d %d %d %d %d", &a, &d, &pct, &x, &y) < 3 || a < 0 || a > 15 || d < 0 || d > 15 || pct < 1) {
        REPLY("error: usage: wave A D pct [x y]\n");
        return;
    }
    static struct w_scan sc;
    ee_player **pls = game_players();
    if (!pls[a] || !pls[d]) { REPLY("error: no player %d or %d\n", a, d); return; }
    if (player_get_stance(pls[a], d) != 1) set_diplomacy(pls[a], pls[d], 1, 0);   // 1 = enemy
    if (player_get_stance(pls[d], a) != 1) set_diplomacy(pls[d], pls[a], 1, 0);
    // the spot: A's Town Center (or Capitol, or any building) at the first wave, kept for the later ones (D razes A's
    // base once they are at war). Beside A's buildings on a 2-tile grid, part of each wave stood in a pocket for good.
    static struct { uint32_t t; int x, y; } spot[16];
    if (game_time() < spot[a].t) spot[a].t = 0, spot[a].x = -1;   // a new game
    if (x < 0 && spot[a].t && spot[a].x >= 0) x = spot[a].x, y = spot[a].y;
    if (x < 0) {
        units_of(a, 1, &sc);
        for (int i = 0; i < sc.n; i++) {
            int t = type_get_id(unit_get_type_data(sc.u[i]));
            if (x < 0 || t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL)) x = unit_get_x(sc.u[i]), y = unit_get_y(sc.u[i]);
            if (t == UT(B_TOWN_CENTER)) break;
        }
        if (x < 0) { REPLY("error: P%d has no buildings, give x y\n", a); return; }
    }
    spot[a].t = game_time() ? game_time() : 1; spot[a].x = x; spot[a].y = y;
    units_of(d, 0, &sc);
    // the mix: per type the most D has had at a wave (new game: game time went back)
    static struct { uint32_t t; int nt, type[64], cnt[64], fam[64]; } mix[16];
    uint32_t now = game_time();
    if (now < mix[d].t) mix[d].nt = 0;
    mix[d].t = now;
    int ct[64] = { 0 }, total = 0, sent = 0, left = 0;
    for (int i = 0; i < sc.n; i++) {
        ee_unit_type *td = unit_get_type_data(sc.u[i]);
        int t = type_get_id(td), j = 0;
        if (is_ship_or_air(td)) { left++; continue; }
        while (j < mix[d].nt && mix[d].type[j] != t) j++;
        if (j == mix[d].nt) {
            if (j == 64) continue;
            mix[d].type[j] = t; mix[d].cnt[j] = 0; mix[d].fam[j] = type_get_family(td); mix[d].nt++;
        }
        ct[j]++; total++;
    }
    int base = 0;
    for (int j = 0; j < mix[d].nt; j++) {
        if (ct[j] > mix[d].cnt[j]) mix[d].cnt[j] = ct[j];
        base += mix[d].cnt[j];
    }
    int nt = mix[d].nt, *type = mix[d].type, *cnt = mix[d].cnt, *fam = mix[d].fam;
    REPLY("wave: P%d gets %d%% of P%d's army (%d soldiers now, mix of %d) at %d,%d:", a, pct, d, total, base, x, y);
    char tmp[256], args2[64];
    static const int ring[8][2] = { { 9, 0 }, { 0, 9 }, { -9, 0 }, { 0, -9 }, { 6, 6 }, { -6, 6 }, { -6, -6 }, { 6, -6 } };
    int chunk = 0;
    for (int j = 0; j < nt; j++) {
        int n = (cnt[j] * pct + 50) / 100;
        while (n > 0) {   // groups of 16 (a 4x4 grid, 2 tiles apart) around the spot
            int k = n > 16 ? 16 : n, r = chunk++ % 8;
            sprintf(args2, "%d %d %d %d %d", a, type[j], x + ring[r][0] - 3, y + ring[r][1] - 3, k);
            tmp[0] = 0;   // cmd_spawn appends its reply
            cmd_spawn(tmp, args2);
            n -= k; sent += k;
        }
        REPLY(" %dx%d(f%d)", (cnt[j] * pct + 50) / 100, type[j], fam[j]);
    }
    REPLY(" - %d units (%d ships/aircraft left out)\n", sent, left);
}
internal struct { int a, d; uint32_t t, logged; unsigned rot; } g_charge = { -1, -1, 0, 0, 0 };
internal void charge_pass(char *out, int a, int d) {
    int n = 0, kept = 0, idle = 0, marching = 0;
    static struct w_scan mine, foe, bld, civ;
    units_of(a, 0, &mine);
    units_of(d, 0, &foe);
    units_of(d, 1, &bld);
    units_of(d, 2, &civ);
    for (int i0 = 0; i0 < mine.n; i0++) {
        int i = (int)((i0 + g_charge.rot) % (unsigned)mine.n);
        ee_unit *u = mine.u[i];
        ee_goal *goal = unit_get_goal(u);
        if (goal && !strcmp(rtti_name(goal), "EEUGAttackUnit@@")) {
            ee_unit *t = goal_get_target(goal);
            void *to = t ? unit_get_owner(t) : NULL;
            if (to && player_get_index(to) == d && unit_get_hp(t) > 0) { kept++; continue; }
        }
        if (goal && !strcmp(rtti_name(goal), "EEUGLocation@@")) { marching++; continue; }
        if (++idle > 80) continue;   // 80 orders a pass: 500 at once lost their goal within 10 s (the path search)
        float ux = (float)unit_get_x(u), uy = (float)unit_get_y(u), best = 15.0f * 15.0f;
        ee_unit *tgt = NULL;
        for (int k = 0; k < foe.n; k++) {
            float dx = (float)unit_get_x(foe.u[k]) - ux, dy = (float)unit_get_y(foe.u[k]) - uy;
            if (dx * dx + dy * dy < best) { best = dx * dx + dy * dy; tgt = foe.u[k]; }
        }
        if (!tgt) {
            best = 20.0f * 20.0f;
            for (int k = 0; k < civ.n; k++) {
                float dx = (float)unit_get_x(civ.u[k]) - ux, dy = (float)unit_get_y(civ.u[k]) - uy;
                if (dx * dx + dy * dy < best) { best = dx * dx + dy * dy; tgt = civ.u[k]; }
            }
        }
        if (!tgt) {
            best = 1e12f;
            for (int k = 0; k < bld.n; k++) {
                float dx = (float)unit_get_x(bld.u[k]) - ux, dy = (float)unit_get_y(bld.u[k]) - uy;
                if (dx * dx + dy * dy < best) { best = dx * dx + dy * dy; tgt = bld.u[k]; }
            }
        }
        if (!tgt) continue;
        float tx = (float)unit_get_x(tgt), ty = (float)unit_get_y(tgt);
        if ((tx - ux) * (tx - ux) + (ty - uy) * (ty - uy) > 20.0f * 20.0f)   // far: walk there first
            n += unit_goal(u, goal_move(tx + (ux - tx) * 0.1f, ty + (uy - ty) * 0.1f), 0, 0x1F40);
        else n += unit_goal(u, goal_attack(u, tgt), 0, 0x1F40);
    }
    g_charge.rot += 80;
    if (out) sprintf(out + strlen(out), "charge: P%d %d soldiers, %d without a target, %d sent, %d marching, %d fighting; P%d %d "
                     "soldiers, %d citizens, %d buildings\n", a, mine.n, idle, n, marching, kept, d, foe.n, civ.n, bld.n);
}
// charge A D: from now on every 10 s (attack_tick), logged every minute; "charge off" stops it
internal void cmd_charge(char *out, const char *args) {
    int a, d;
    if (!strncmp(args, "off", 3)) { g_charge.a = g_charge.d = -1; REPLY("charge: off\n"); return; }
    if (sscanf(args, "%d %d", &a, &d) < 2 || a < 0 || a > 15 || d < 0 || d > 15) { REPLY("error: usage: charge A D | off\n"); return; }
    g_charge.a = a; g_charge.d = d; g_charge.t = g_charge.logged = game_time();
    charge_pass(out, a, d);
}
internal void seaop_tick(uint32_t now);   // testopp.c
internal void charge_tick(uint32_t now) {
    seaop_tick(now);
    if (g_charge.d < 0) return;
    if (now < g_charge.t) { g_charge.a = g_charge.d = -1; return; }   // a new game
    if (now - g_charge.t < 10000) return;
    g_charge.t = now;
    static char line[256];
    line[0] = 0;
    charge_pass(now - g_charge.logged >= 60000 ? line : NULL, g_charge.a, g_charge.d);
    if (line[0] && g_log) {
        g_charge.logged = now;
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# %.1f min %s", now / 60000.0, line);
        fflush(g_log);
        LeaveCriticalSection(&g_lock);
    }
}
internal void cmd_army(char *out, const char *args) {
    int p = 0, only = -1;   // army P [type]: only that unit type
    sscanf(args, "%d %d", &p, &only);
    ee_map *map = game_map();
    ee_grid *grid = map ? map_get_grid(map) : NULL;
    if (!grid) { REPLY("error: no map\n"); return; }
    struct { const char *goal; int n; long sx, sy; ee_unit *first; } g[32];
    int ng = 0, total = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        ee_player *owner = unit_get_owner(u);
        if (!td || !owner || player_get_index(owner) != p) continue;
        if (!is_army_family(type_get_family(td))) continue;
        if (only >= 0 && type_get_id(td) != only) continue;
        const char *gn = rtti_name(unit_get_goal(u));
        int j = 0;
        while (j < ng && g[j].goal != gn) j++;
        if (j == ng) { if (ng == 32) continue; g[ng].goal = gn; g[ng].n = 0; g[ng].sx = g[ng].sy = 0; g[ng].first = u; ng++; }
        g[j].n++; g[j].sx += unit_get_x(u); g[j].sy += unit_get_y(u);
        total++;
    }
    REPLY("P%d: %d soldiers\n", p, total);
    for (int j = 0; j < ng; j++) {
        REPLY("  %3d x %-28s centre %ld,%ld", g[j].n, g[j].goal, g[j].sx / g[j].n, g[j].sy / g[j].n);
        uint32_t *gd = ((uint32_t *)unit_get_goal(g[j].first));
        if (gd) for (int k = 0; k < 24; k++) REPLY(" %08x", gd[k]);
        REPLY("\n");
    }
}

// ---- from forces.c ----
// continent x y ... (test): per tile its land continent and ocean ids, the area object (0x425F16) and its fields +0x40..
internal void cmd_continent(char *out, const char *args) {
    int v[16], n = sscanf(args, "%d %d %d %d %d %d %d %d %d %d %d %d %d %d %d %d", &v[0], &v[1], &v[2], &v[3], &v[4],
                          &v[5], &v[6], &v[7], &v[8], &v[9], &v[10], &v[11], &v[12], &v[13], &v[14], &v[15]);
    for (int k = 0; k + 1 < n; k += 2) {
        int pt[2] = { v[k], v[k + 1] };
        unsigned w = area_id(v[k], v[k + 1], 1);
        uint8_t *o = area_at(pt);
        REPLY("%d,%d: land %u ocean %u, area %p (vt %06x)", v[k], v[k + 1], cont_at(v[k], v[k + 1]), w, (void *)o,
              o ? *(unsigned *)o : 0);
        if (o) for (int q = 0x40; q < 0x90; q += 4) REPLY(" %x", FIELD(o, q, unsigned));
        REPLY("\n");
    }
}

// ---- from forces.c ----
// aigroup P [x y] (test): AI P forms an attack group as the scenario effect "Create AI Military Group" does (0x5AB8BE,
// docs/triggers.md) - a group of the game's own (EECPAttackGroup) with a target, where the AI's own attacks on a player
// (mission 1) have none and walk legs. Target: the enemy building nearest x,y (default: the centre of P's soldiers),
// mission 7 (attack the goal unit); units: P's land soldiers the combat manager calls free (0x4707D8: in no group).
// Task (0x34, ctor 0x441FB3): +4 location (0x47C8B7 = location 0x18, ctor 0x47C4E8, point at +4), target unit via
// 0x442246, +0x20 target player, +0x28 mission, +0x2C 2, +0x30 1. Strategy (0x44, ctor 0x473BC4, reset 0x473CA7):
// +0x3C 4 attack / 5 defend. Then cm 0x47067A(task, strategy, vector<int> ids) and [cm+0x48] 0x45106C(same).
struct ag_scan { ee_player *pl; ee_combat_mgr *cm; int n, ids[256]; long sx, sy; int x, y, nodef; ee_unit *best; long bd; };
internal void ag_unit(ee_unit *u, void *ctx) {
    struct ag_scan *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    ee_player *owner = unit_get_owner(u);
    if (unit_get_hp(u) <= 0 || !owner || !td) return;
    int fam = type_get_family(td), x = unit_get_x(u), y = unit_get_y(u);
    if (owner == s->pl && s->x < 0) {
        if (is_soldier(td) && !is_ship_or_air(td) && s->n < 256 &&
            cm_unit_free(s->cm, unit_get_physics(u))) {
            s->ids[s->n++] = unit_get_serial(u); s->sx += x; s->sy += y;
        }
    } else if (s->x >= 0 && is_enemy(s->pl, owner) && (fam == 2 || fam == 29)) {
        float r, a;
        if (s->nodef && defence_stats(u, &r, &a)) return;
        long d = (long)(x - s->x) * (x - s->x) + (long)(y - s->y) * (y - s->y);
        if (!s->best || d < s->bd) { s->best = u; s->bd = d; }
    }
}
internal void cmd_aigroup(char *out, const char *args) {
    int p = 0, x = -1, y = -1;
    if (sscanf(args, "%d %d %d", &p, &x, &y) < 3) x = y = -1;
    int nodef = strstr(args, "nodef") != NULL;   // aigroup P [x y] [nodef]: not a defence
    ee_player *pl = p > 0 && p < 16 ? g_players[p] : NULL;
    ee_combat_mgr *cm = pl ? player_get_combat_mgr(pl) : NULL; ee_attack_cmd *cmd = cm ? am_get_commander(cm) : NULL;
    if (!cmd) { REPLY("error: aigroup P [x y] - no AI player %d\n", p); return; }
    static struct ag_scan s;
    memset(&s, 0, sizeof s);
    s.pl = pl; s.cm = cm; s.x = -1; s.nodef = nodef;
    grid_each(ag_unit, &s);
    if (!s.n) { REPLY("error: P%d has no free land soldiers\n", p); return; }
    s.x = x >= 0 ? x : (int)(s.sx / s.n); s.y = y >= 0 ? y : (int)(s.sy / s.n);
    grid_each(ag_unit, &s);
    if (!s.best) { REPLY("error: no enemy building\n"); return; }
    ee_unit *t = s.best;
        ee_player *tpl = unit_get_owner(t);
    __attribute__((aligned(8))) uint8_t task_mem[0x40], loc[0x20], strat_mem[0x50];
    ee_task *task = (ee_task *)task_mem; ee_strategy *strat = (ee_strategy *)strat_mem;
    task_init(task);
    location_init(loc);
    strategy_init(strat);
    task_set_target(task, t);
    int pt[2] = { unit_get_x(t), unit_get_y(t) };
    point_set(loc + 4, pt);
    task_set_location(task_mem + 4, loc);
    task_set_target_player(task, game_players()[player_get_index(tpl) & 15]);
    task_set_unknown_28(task, 7); task_set_unknown_2c(task, 2); task_set_unknown_30(task, 1);
    strategy_reset(strat);
    strategy_set_unknown_3c(strat, 4); strategy_set_unknown_40(strat, 0);
    // vector<int> {allocator, first, last, end} with the game's heap (its dtor 0x61ABEC frees with the game's delete)
    __attribute__((aligned(8))) uint8_t vec[16] = { 0 };
    int *ids = game_new((unsigned)s.n * 4);
    memcpy(ids, s.ids, (size_t)s.n * 4);
    FIELD(vec, 4, int *) = ids; FIELD(vec, 8, int *) = ids + s.n; FIELD(vec, 0xC, int *) = ids + s.n;
    char ok = attack_plan_ok(cm, task, strat, vec);
    ee_attack_group *g = ok ? attack_group_new(cmd, task, strat, vec) : NULL;
    vector_free(vec);
    task_free(task);
    strategy_free(strat);
    int nsg = g && ag_get_subgroups_begin(g) ? (int)(ag_get_subgroups_end(g) - ag_get_subgroups_begin(g)) : 0;
    REPLY("P%d: %d free land soldiers (centre %ld,%ld) -> target type %d of P%d at %d,%d: %s, group %p with %d subgroups\n",
          p, s.n, s.sx / s.n, s.sy / s.n, type_get_id(unit_get_type_data(t)), player_get_index(tpl), pt[0], pt[1],
          ok ? "accepted" : "refused (0x47067A)", g, nsg);
}

// ---- from intel.c ----
// calloff P T (test): AI P calls off its attack on T (the game's call-off at the next safe point, as fall_back's END)
internal void cmd_calloff(char *out, const char *args) {
    int p = 0, t = 0;
    if (sscanf(args, "%d %d", &p, &t) != 2 || p <= 0 || p >= 16 || t <= 0 || t >= 16) { REPLY("error: calloff P T\n"); return; }
    g_cancel_why[p][t] = "test";
    cancel_citizen_attack(p, t);
    REPLY("P%d calls off its attack on P%d at the next safe point\n", p, t);
}

// ---- from aim.c ----
// uinfo x y [r]: units within r tiles: position, speed, facing, hit points, goal
internal void cmd_uinfo(char *out, const char *args) {
    int x = 0, y = 0, r = 3;
    sscanf(args, "%d %d %d", &x, &y, &r);
    if (!game_map()) { REPLY("error: no map\n"); return; }
    REPLY("t %u\n", game_time());
    for (struct grid_it it = grid_it_box(x, y, r, 1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        int ux = unit_get_x(u), uy = unit_get_y(u);
        if (abs(ux - x) > r || abs(uy - y) > r || strlen(out) > 15000) continue;
        ee_player *owner = unit_get_owner(u);
        REPLY("%p type %d P%d hp %d pos %.3f,%.3f,%.2f spd %.3f/%.3f fwd %.2f,%.2f,%.2f goal %s\n",
              u, type_get_id(td), owner ? player_get_index(owner) : -1, unit_get_hp(u),
              unit_get_pos_x(u), unit_get_pos_y(u), unit_get_pos_z(u), unit_get_speed(u),
              unit_get_speed_min(u), unit_get_facing_x(u), unit_get_facing_y(u), unit_get_facing_z(u),
              rtti_name(unit_get_goal(u)));
    }
}

// move P x y r tx ty: units of player P within r tiles of x, y walk to tx, ty keeping their spread (tests)
internal void cmd_move(char *out, const char *args) {
    int p, x, y, r, tx, ty, n = 0;
    if (sscanf(args, "%d %d %d %d %d %d", &p, &x, &y, &r, &tx, &ty) < 6) { REPLY("error: usage: move P x y r tx ty\n"); return; }
    if (!game_map()) { REPLY("error: no map\n"); return; }
    ee_unit *found[200];
    int nf = 0;
    for (struct grid_it it = grid_it_box(x, y, r, 1); nf < 200 && grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        ee_player *owner = unit_get_owner(u);
        if (!owner || player_get_index(owner) != p || type_get_family(td) == 2) continue;
        if (abs(unit_get_x(u) - x) > r || abs(unit_get_y(u) - y) > r) continue;
        found[nf++] = u;
    }
    for (int i = 0; i < nf; i++)
        n += unit_goal(found[i], goal_move((float)tx + unit_get_pos_x(found[i]) - (float)x,
                                           (float)ty + unit_get_pos_y(found[i]) - (float)y), 0, 0x1F40);
    REPLY("ok: %d of %d units of P%d moving\n", n, nf, p);
}
