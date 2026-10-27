// THE SPOILS OF A RESIGN ([ai_base] resign_spoils; the player 2026-10-27: a resigned player's buildings and units are
// just left as is - P4 of the save "standing army" had left the game with 117 soldiers and its town, and the AIs
// spent their waves on it). Included by eeaimod.c after diplo.c (set_stance, is_out, player_at) and bunker.c
// (bk_capacity, bk_held, bk_unload); run at the safe point between simulation steps (test/control.c hook_sim_receive).
// When a player goes out of the game (resigned, or the mod's surrender):
// - it is at war with everyone (both ways, the game's own stance change), its fortresses and airports let their units
//   out, and its land fighters roam: in bands (units within SP_BAND tiles of each other), each band an attack-move to
//   the nearest unit of anyone - no building - on its land within SP_HUNT tiles, else to a point SP_WANDER tiles off;
//   again every SP_ROAM_MS while most of the band isn't fighting (the target moves, an old order is dropped). They
//   fight units only: the auto-target search of a unit whose player is out takes no building (hook_target_ok: the
//   candidate test 0x639993 says no) - they can't take on a town, and houses to hit would only bind them until
//   someone clears them. Citizens, priests, ships and aircraft stay as they are.
// - SP_HAND_MS later its buildings change hands (the units have left them by then): the damage it took in the last 10
//   minutes (player +0xA70, the game's own sum per attacker; the mod keeps a snapshot a minute) tells the conquerors -
//   every player still in the game with at least SP_SHARE % of it. Each gets its share of the buildings' value (their
//   cost), the buildings nearest its own first; walls go to the conqueror nearest them. The change is the game's own
//   (unit_give 0x638670: the scenario effect Owner's, a priest's conversion's), a gate or an airport by the one piece
//   whose handover brings its other pieces along (below). No conqueror (nobody did SP_SHARE %): the buildings stay as they are.
// Players out before the game was loaded are left to the target rule and the roaming alone (the same on every PC).
// Measured on the save: P4's 118 soldiers roamed out in bands and were gone in 11 minutes; tests/scenarios/resign.eeai
// (P3 out at minute 25, Continental, 4 AIs from epoch 5): seed 7 P2 56% / P4 43% of the damage - 13 buildings each,
// value 1425 / 1075. "# spoils:" lines.
internal int g_spoils = 1;
#define SP_SHARE 20
#define SP_HAND_MS 30000
#define SP_BAND 10
#define SP_HUNT 80
#define SP_WANDER 30
#define SP_ROAM_MS 10000
#define SP_IN_MS 30000   // a game loaded shows its out players in for a moment, then out: no resign that
internal struct {
    int dmg[11][17];   // a snapshot of player +0xA70 per game minute, ring by minute
    uint32_t min[11];  // the minute each holds (0 = none: minute + 1)
    uint8_t out;       // out at the last look
    uint32_t in_since; // in the game since (game time + 1; 0 out): a resign counts after SP_IN_MS in
    uint32_t hand_at;  // its buildings change hands then (0 none)
    int share[17];     // the conquerors' shares in percent
} g_sp[16];
internal uint32_t g_sp_roam_t;
internal int g_sp_seen;   // the players' out flags read once (a loaded game: no handover for those out already)
internal uint32_t g_sp_last;   // the last tick's game time: a jump back or a gap of 10+ s is a game loaded - the flags read afresh

internal int sp_building(int fam) {
    return fam == CLS_BUILDING || fam == CLS_FORTRESS || fam == CLS_GATE || fam == CLS_NAVAL_BUILDING || fam == CLS_BUILDING_PRIEST ||
           fam == CLS_SPACE_BUILDING;
}
internal int sp_roamer(ee_unit *u, ee_unit_type *td) {   // a land fighter of an out player, on the map
    int fam = unit_type_get_class_id(td);
    return (fam == CLS_LAND || fam == CLS_LAND_MILITARY_HERO) && !is_citizen_type(unit_type_get_id(td)) && unit_type_get_attack(td) > 0 &&
           unit_get_hp(u) > 0 && unit_get_x(u) >= 0;
}

// the candidate test of the auto-target search (unit_functions.target_ok): a unit of an out player takes no building
internal char __thiscall hook_target_ok(ee_target_search *s, ee_unit *u) {
    char ok = target_ok(s, u);
    if (!ok || !g_spoils) return ok;
    ee_player *pl = target_search_get_player(s);
    if (!pl || !player_get_out(pl)) return ok;
    ee_unit_type *td = unit_get_type_data(u);
    return td && sp_building(unit_type_get_class_id(td)) ? 0 : ok;
}

// THE HANDOVER
struct sp_scan { int pi, n; ee_unit *b[2048]; int na[17]; ivec2 a[17][256]; };
internal void sp_scan_unit(ee_unit *u, void *ctx) {
    struct sp_scan *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    ee_player *o = unit_get_owner(u);
    if (!td || !o || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !sp_building(unit_type_get_class_id(td))) return;
    int oi = player_get_index(o) & 15;
    // a chain of pieces (a gate's five, an airport and its runways: physics +0x1CC the next) is given by its last piece
    // alone: unit_give calls the type's handover (vf8) only for it, and that remakes and links the others (EEUCGate
    // vf8 0x601FC5) - each piece given apart was remade unlinked (a gate's open test read NULL+0x1D0 15 s later); the
    // line upgrade (0x5BA59D) skips the same pieces
    ee_unit *ph = unit_get_physics(u);
    if (oi == s->pi) { if (s->n < 2048 && !(ph && unit_get_chain(ph))) s->b[s->n++] = u; }
    else if (oi <= 16 && s->na[oi] < 256) s->a[oi][s->na[oi]++] = unit_get_tile(u);   // the others' buildings: where each one is
}
internal long sp_dist(struct sp_scan *s, int q, ivec2 p) {   // to conqueror q's nearest building
    long best = 0x7FFFFFFF;
    for (int k = 0; k < s->na[q]; k++) { long d = ivec2_dist2(s->a[q][k], p); if (d < best) best = d; }
    return best;
}
internal int sp_value(ee_player *pl, ee_unit *u) {
    int t = unit_type_get_id(unit_get_type_data(u)), v = 0;
    for (int r = 2; r < 7; r++) v += unit_cost(pl, t, r);
    return v > 0 ? v : 1;
}
internal void sp_hand_over(int pi) {
    static struct sp_scan s;   // 40 KB: not on the stack
    memset(&s, 0, sizeof s);
    s.pi = pi;
    grid_each(sp_scan_unit, &s);
    ee_player *pl = player_at(pi);
    int q[17], nq = 0;
    for (int j = 1; j < 17; j++) if (g_sp[pi].share[j]) q[nq++] = j;
    if (!pl || !nq || !s.n) return;
    long total = 0, quota[17] = { 0 }, got[17] = { 0 };
    int given[17] = { 0 }, walls[17] = { 0 }, failed = 0;
    static int val[2048];
    for (int k = 0; k < s.n; k++) {
        val[k] = is_wall(unit_type_get_id(unit_get_type_data(s.b[k]))) ? 0 : sp_value(pl, s.b[k]);
        total += val[k];
    }
    for (int i = 0; i < nq; i++) quota[q[i]] = total * g_sp[pi].share[q[i]] / 100;
    // the buildings by how clearly one conqueror is nearest (the margin to the second), each to the nearest one with
    // quota left - the shares stay whole pieces, not a patchwork
    static long margin[2048];
    static uint8_t near1[2048];
    for (int k = 0; k < s.n; k++) {
        long d1 = 0x7FFFFFFF, d2 = 0x7FFFFFFF;
        int b1 = q[0];
        for (int i = 0; i < nq; i++) {
            long d = sp_dist(&s, q[i], unit_get_tile(s.b[k]));
            if (d < d1) { d2 = d1; d1 = d; b1 = q[i]; } else if (d < d2) d2 = d;
        }
        near1[k] = (uint8_t)b1;
        margin[k] = nq > 1 ? d2 - d1 : 0;
    }
    for (int done = 0; done < s.n; done++) {
        int k = -1;
        for (int j = 0; j < s.n; j++) if (s.b[j] && (k < 0 || margin[j] > margin[k])) k = j;
        if (k < 0) break;
        ee_unit *u = s.b[k];
        s.b[k] = NULL;
        int to = near1[k];
        if (val[k] && got[to] + val[k] / 2 > quota[to]) {   // its quota is full: the nearest with room
            long bd = 0x7FFFFFFF;
            for (int i = 0; i < nq; i++) {
                long d = sp_dist(&s, q[i], unit_get_tile(u));
                if (got[q[i]] + val[k] / 2 <= quota[q[i]] && d < bd) { bd = d; to = q[i]; }
            }
        }
        if (unit_give(u, to, 0)) { got[to] += val[k]; if (val[k]) given[to]++; else walls[to]++; }   // the scenario effect Owner's change
        else failed++;
    }
    if (g_log) {
        fprintf(g_log, "# spoils: P%d's buildings change hands at %.1f min (value %ld, %d failed):", pi, game_time() / 60000.0, total, failed);
        for (int i = 0; i < nq; i++)
            fprintf(g_log, " P%d %d%% of the damage - %d buildings (value %ld) and %d walls", q[i], g_sp[pi].share[q[i]], given[q[i]], got[q[i]], walls[q[i]]);
        fputc('\n', g_log);
    }
}

// THE RESIGN: at war with everyone, the garrisons out, the conquerors noted
internal void sp_unload_one(ee_unit *u, void *ctx) {
    int pi = *(int *)ctx;
    ee_player *o = unit_get_owner(u);
    if (o && (player_get_index(o) & 15) == pi && unit_get_hp(u) > 0 && bk_capacity(u) > 0 && bk_held(u) > 0) {
        ee_unit_type *td = unit_get_type_data(u);
        if (td && sp_building(unit_type_get_class_id(td))) bk_unload(u);
    }
}
internal void sp_resigned(int pi, uint32_t now) {
    ee_player *pl = player_at(pi);
    if (!pl) return;
    for (int j = 1; j < nplayers(); j++)
        if (j != pi && player_at(j)) { set_stance(pi, j, 1); set_stance(j, pi, 1); }
    grid_each(sp_unload_one, &pi);
    // the damage of the last 10 minutes: now against the oldest snapshot at most 10 minutes old
    unsigned m = now / 60000;
    int oldest = -1;
    for (int k = 0; k < 11; k++) {
        unsigned sm = g_sp[pi].min[k] - 1;
        if (g_sp[pi].min[k] && sm + 10 >= m && (oldest < 0 || sm < g_sp[pi].min[oldest] - 1)) oldest = k;
    }
    long d[17] = { 0 }, total = 0;
    for (int j = 1; j < 17; j++) {
        ee_player *pj = player_at(j);
        if (j == pi || !pj || is_out(pj)) continue;
        d[j] = player_get_damage_by(pl, j) - (oldest >= 0 ? g_sp[pi].dmg[oldest][j] : 0);
        if (d[j] < 0) d[j] = 0;
        total += d[j];
    }
    memset(g_sp[pi].share, 0, sizeof g_sp[pi].share);
    int nq = 0, sum = 0;
    for (int j = 1; j < 17 && total > 0; j++) if (d[j] * 100 >= total * SP_SHARE) sum += (int)d[j], nq++;
    for (int j = 1; j < 17 && sum > 0; j++) if (d[j] * 100 >= total * SP_SHARE) g_sp[pi].share[j] = (int)(d[j] * 100 / sum);
    g_sp[pi].hand_at = nq ? (now + SP_HAND_MS ? now + SP_HAND_MS : 1) : 0;
    if (g_log) {
        fprintf(g_log, "# spoils: P%d out at %.1f min - at war with everyone, its land fighters roam; damage in the last %u minutes %ld:",
                pi, now / 60000.0, oldest >= 0 ? m - (g_sp[pi].min[oldest] - 1) : m, total);
        for (int j = 1; j < 17; j++) if (d[j]) fprintf(g_log, " P%d %ld", j, d[j]);
        fprintf(g_log, nq ? " - the buildings to %d conqueror(s) in %d s\n" : " - no conqueror (%d), the buildings stay\n", nq, SP_HAND_MS / 1000);
    }
}

// THE ROAMING: every SP_ROAM_MS, each band of idle roamers on its way
struct sp_roam { int nr; ee_unit *r[1024]; int ne; ivec2 e[4096]; ee_player *eo[4096]; };
internal void sp_roam_unit(ee_unit *u, void *ctx) {
    struct sp_roam *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    ee_player *o = unit_get_owner(u);
    if (!td || !o || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) return;
    if (player_get_out(o)) { if (sp_roamer(u, td) && s->nr < 1024) s->r[s->nr++] = u; }
    else if ((is_army_family(unit_type_get_class_id(td)) || is_citizen_type(unit_type_get_id(td))) && s->ne < 4096) {   // no building
        s->e[s->ne] = unit_get_tile(u); s->eo[s->ne++] = o;
    }
}
internal int sp_on_map(ivec2 p) {   // 2 tiles in from every edge
    ee_map *map = game_map();
    int size = map ? (int)map_get_size(map) : 0;
    return p.x >= 2 && p.y >= 2 && p.x < size - 2 && p.y < size - 2;
}
internal void sp_roam(uint32_t now) {
    static struct sp_roam s;
    memset(&s, 0, sizeof s);
    grid_each(sp_roam_unit, &s);
    static uint8_t used[1024];
    memset(used, 0, sizeof used);
    unsigned bands = 0, sent = 0, hunting = 0, nowhere = 0;
    for (int i = 0; i < s.nr; i++) {
        if (used[i]) continue;
        ee_unit *lead = s.r[i];
        ee_player *o = unit_get_owner(lead);
        ivec2 lp = unit_get_tile(lead);
        ee_unit *band[64];
        int nb = 0, idle = 0;
        long cx = 0, cy = 0;
        for (int j = i; j < s.nr && nb < 64; j++) {   // the band: the lead's own player's within SP_BAND of it
            if (used[j] || unit_get_owner(s.r[j]) != o || ivec2_dist2(unit_get_tile(s.r[j]), lp) > SP_BAND * SP_BAND) continue;
            used[j] = 1;
            band[nb++] = s.r[j];
            ee_goal *g = unit_get_goal(s.r[j]);
            idle += !g || !goal_is_attack(g);   // not fighting: its last order (an old guard post, our march) may change
            cx += unit_get_x(s.r[j]); cy += unit_get_y(s.r[j]);
        }
        bands++;
        if (idle * 2 < nb) continue;   // fighting
        ivec2 c = iv2((int)(cx / nb), (int)(cy / nb)), to = c;
        short area = (short)map_area(c, 2);
        // a tile off the map has area 0, and so has land without a continent id (the player's camp on the reference
        // save): a band there would take a point past the map's far edge for its own land (found looking at the
        // player's crash at 594.7 min, a unit put off the map - that one came from a holder's unload, not from here).
        // Goals only on the map and in a known area.
        if (!area) { nowhere++; continue; }
        long bd = (long)SP_HUNT * SP_HUNT + 1;
        int found = 0;
        for (int k = 0; k < s.ne; k++) {   // the nearest unit of anyone on its land
            if (s.eo[k] == o) continue;
            long d = ivec2_dist2(s.e[k], c);
            if (d < bd && sp_on_map(s.e[k]) && (short)map_area(s.e[k], 2) == area) { bd = d; to = s.e[k]; found = 1; }
        }
        if (!found) {   // a point SP_WANDER off on its land - the same on every PC (no random numbers)
            uint32_t h = (uint32_t)unit_get_serial(lead) * 2654435761u ^ (now / SP_ROAM_MS) * 40503u;
            for (int t = 0; t < 8; t++, h = h * 1103515245u + 12345u) {
                int a = (int)(h >> 8) % 360;
                ivec2 p = iv2(c.x + (int)(SP_WANDER * cosf(a * 0.0174533f)), c.y + (int)(SP_WANDER * sinf(a * 0.0174533f)));
                if (sp_on_map(p) && (short)map_area(p, 2) == area) { to = p; break; }
            }
        }
        if (to.x == c.x && to.y == c.y) continue;
        hunting += found;
        for (int j = 0; j < nb; j++) {
            float xy[2] = { to.x + 0.5f, to.y + 0.5f };
            void *mem = goal_move_alloc();
            ee_goal *g = mem ? goal_attack_move_init(mem, xy) : NULL;
            if (g && unit_goal(band[j], g, 0, 0x1F40)) sent++;   // the state the AI's own moves start with (0: it stood)
        }
    }
    static uint32_t logged;
    if (g_log && s.nr && now - logged >= 60000) {
        logged = now;
        fprintf(g_log, "# spoils: roaming at %.1f min: %d roamers in %u bands, %u sent on (%u bands to units, %u in no known area left)\n",
                now / 60000.0, s.nr, bands, sent, hunting, nowhere);
    }
}

// the safe point (test/control.c hook_sim_receive)
internal void spoils_tick(void) {
    if (!g_spoils || scenario_game()) return;
    uint32_t now = game_time();
    if (!now) return;
    if (now < g_sp_last || now - g_sp_last > 10000) g_sp_seen = 0;
    g_sp_last = now;
    unsigned m = now / 60000;
    int roamers = 0;
    for (int i = 1; i < nplayers(); i++) {
        ee_player *p = player_at(i);
        if (!p) continue;
        int k = (int)(m % 11);
        if (g_sp[i].min[k] != m + 1) {   // this minute's snapshot
            g_sp[i].min[k] = m + 1;
            for (int j = 0; j < 17; j++) g_sp[i].dmg[k][j] = player_get_damage_by(p, j);
        }
        int out = player_get_out(p) != 0;
        if (out && !g_sp[i].out && g_sp_seen && g_sp[i].in_since && now - (g_sp[i].in_since - 1) >= SP_IN_MS) sp_resigned(i, now);
        if (out) g_sp[i].in_since = 0;
        else if (!g_sp[i].in_since || !g_sp_seen) g_sp[i].in_since = now + 1;
        g_sp[i].out = (uint8_t)out;
        if (g_sp[i].hand_at && now >= g_sp[i].hand_at) { g_sp[i].hand_at = 0; sp_hand_over(i); }
        roamers += out;
    }
    g_sp_seen = 1;
    if (roamers && (!g_sp_roam_t || now - g_sp_roam_t >= SP_ROAM_MS)) { g_sp_roam_t = now; sp_roam(now); }
}
internal void spoils_new_game(void) { memset(g_sp, 0, sizeof g_sp); g_sp_roam_t = 0; g_sp_seen = 0; g_sp_last = 0; }
internal void spoils_install(void) {
    g_spoils = g_set_ai[1].resign_spoils;
    int of = 0, ok = patch_target_ok(hook_target_ok, &of);
    if (g_log) fprintf(g_log, "# spoils: %d (target hook %d/%d)\n", g_spoils, ok, of);
}
