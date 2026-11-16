// A SCENARIO'S MILITARY TARGETS (the player 2026-11-16: "the most important parts would be the trigger system attack
// targets ... it dont has to follow the hole system, but it should roughly understand what the scenario want from
// it"): the trigger Modify AI Military Target puts a unit or an AI area on one of the computer player's two lists
// (EECPCombatManager.attack_targets_begin, defend_targets_begin) and takes it off again; the lists are the game's,
// saved with it, read here as they stand. stations.c uses them: an attack target's territory is where its pushes go
// first (at the target), a defend target's territory holds a share of its main force (SN_DEFEND).
struct scen_target { ivec2 at; ee_unit *u; int area; uint8_t defend; };

// its targets now: the live units' and the areas' (their tile from the area), up to max
internal int scen_targets(ee_player *pl, struct scen_target *out, int max) {
    ee_combat_mgr *cm = pl ? player_get_combat_mgr(pl) : NULL;
    if (!cm) return 0;
    int k = 0;
    for (int defend = 0; defend < 2; defend++) {
        int n = combat_mgr_target_count(cm, defend);
        for (int i = 0; i < n && k < max; i++) {
            ee_task *t = combat_mgr_target(cm, defend, i);
            ee_unit *u = task_get_target_unit(t);
            ee_location *l = task_location_at(t);
            int area = u ? 0 : location_get_unit_id(l);
            struct scen_target *o = &out[k];
            if (u) { if (unit_get_hp(u) <= 0) continue; o->at = unit_get_tile(u); }
            else if (area && location_resolve(l)) o->at = location_get_tile(l);
            else continue;
            o->u = u; o->area = area; o->defend = (uint8_t)defend;
            k++;
        }
    }
    return k;
}
// scentarget P [attack|defend X Y Q] (test): AI P's scenario targets, or player Q's unit nearest X,Y (within 12
// tiles, a building first) added to them as the trigger adds it (by its id: the serial the trigger passes)
internal void cmd_scentarget(char *out, const char *args) {
    int p = 0, x = 0, y = 0, q = 0;
    char kind[16] = "";
    int n = sscanf(args, "%d %15s %d %d %d", &p, kind, &x, &y, &q);
    ee_player *pl = n >= 1 && p > 0 && p < 16 ? player_at(p) : NULL;
    ee_combat_mgr *cm = pl ? player_get_combat_mgr(pl) : NULL;
    if (!cm || (n != 1 && n != 5) || (n == 5 && strcmp(kind, "attack") && strcmp(kind, "defend"))) { REPLY("error: scentarget P [attack|defend X Y Q]\n"); return; }
    if (n == 5) {
        ee_unit *best = NULL;
        int bd = 0, bb = 0;
        for (struct grid_it it = grid_it_box(x, y, 12, 1); grid_it_next(&it); ) {
            ee_player *o = unit_get_owner(it.u);
            if (!o || player_get_index(o) != q || unit_get_hp(it.u) <= 0) continue;
            int d = abs(unit_get_x(it.u) - x) + abs(unit_get_y(it.u) - y), b = fr_building(it.td);
            if (!best || b > bb || (b == bb && d < bd)) { best = it.u; bd = d; bb = b; }
        }
        if (!best) { REPLY("error: no unit of P%d within 12 tiles of %d,%d\n", q, x, y); return; }
        uint32_t id = (uint32_t)unit_get_serial(best);
        if (unit_by_id(id) != best) { REPLY("error: the unit's serial %u finds another unit\n", id); return; }
        if (kind[0] == 'a') combat_mgr_add_attack_target(cm, id, 0); else combat_mgr_add_defend_target(cm, id, 0);
        if (g_log) log_locked("# scenario targets: P%d at %.1f min: a target to %s added (test) - P%d's unit at %d,%d\n", p, game_time() / 60000.0, kind, q, unit_get_x(best), unit_get_y(best));
    }
    struct scen_target st[32];
    int k = scen_targets(pl, st, 32);
    REPLY("P%d: %d targets (on the lists: attack %d, defend %d)\n", p, k, combat_mgr_target_count(cm, 0), combat_mgr_target_count(cm, 1));
    for (int i = 0; i < k; i++) REPLY("  %s %s %d at %d,%d\n", st[i].defend ? "defend" : "attack", st[i].u ? "unit" : "area", st[i].u ? unit_get_serial(st[i].u) : st[i].area, st[i].at.x, st[i].at.y);
}
