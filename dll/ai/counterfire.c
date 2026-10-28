// COUNTER-FIRE ([ai_base] counter_fire, default on): the AI's units shelled from out of their reach - mortars,
// artillery, snipers, a tower further off than they shoot - and nobody goes after the shooters. Seen in the player's
// game (2026-10-28, setup3): P6's tanks (range 6) at their wave's leg point beside the player's fort, shot to wrecks
// by heavy mortars and snipers they never answered. The unit level answer (engine/unitsfirst.c answer_fire: each
// unit goes for its own attacker) only baited them out one by one. A player does it differently (the player: "if
// something of mine gets hit with mortars or artillery I look what I have nearby and form an attack group to take it
// out"). So every CF_EVERY ms per AI: the enemy units it sees whose attack goal aims at one of its units or buildings
// from beyond that one's reach (further off than its range + 1, the shooter's own range at least CF_RANGE) are
// shooters; shooters within CF_CLUSTER tiles of each other are one target. Against it count the enemy's fighters
// (soldiers and shooting buildings, stats.c st_counts) it sees within CF_ESCORT tiles of the shooter. Its soldiers
// within CF_FROM tiles (on the shooter's land, able to hurt it, not inside anything, not fighting something within 4
// tiles of themselves, not sent already) join nearest first - those already attacking a shooter of it before them -
// until the stats estimate (st_ratio) says they win by CF_WIN, at most CF_MOST; short of that nobody goes. Each
// gets an attack goal on one of the shooters, spread over them, and is committed for CF_HOLD (stage.c
// stage_committed: no staging point, rally, falling back or leash holds it back). "# counter-fire:" lines.
// Measured on copies of the player's saves (exact runs), P6 killed / lost / buildings razed: setup3 674-700 min off
// 60 / 107 / 6, on 292 / 83 / 29 (42 sorties, 273 soldiers; with attack_commit 216 / 135 / 20 -> 292 / 76 / 25);
// stage_fill 638-672 min off 188 / 209 / 42, on 300 / 176 / 104 (with attack_commit 225 / 153 / 37 -> 328 / 141 / 67);
// P7 gained on both. On.

#define CF_EVERY 2000
#define CF_RANGE 5.0f
#define CF_CLUSTER 6
#define CF_ESCORT 10
#define CF_FROM 24
#define CF_WIN 1.5f
#define CF_MOST 60
#define CF_HOLD 45000
#define CF_N 512      // committed units per AI
#define CF_SHOOTERS 128
#define CF_OWN 2048

internal int g_cfire = 1;
internal unsigned g_cf_sent[16], g_cf_sorties[16], g_cf_short[16];
// the units sent, sorted by address (stage_committed asks for every unit the attack subgroups score)
internal struct cf_list { struct cf_ent { ee_unit *u; int id; uint32_t until; } e[CF_N]; int n; } g_cf[16];

internal int cf_cmp(const void *a, const void *b) {
    uintptr_t x = (uintptr_t)*(ee_unit *const *)a, y = (uintptr_t)*(ee_unit *const *)b;
    return x < y ? -1 : x > y;
}
internal int counter_fire_unit(ee_player *pl, ee_unit *u) {
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    struct cf_list *l = &g_cf[idx];
    if (!pl || !u || !l->n) return 0;
    struct cf_ent *f = bsearch(&u, l->e, (size_t)l->n, sizeof *l->e, cf_cmp);   // the key and an entry both start with the unit
    uint32_t now = game_time();
    return f && f->id == unit_get_id(u) && now < f->until && f->until - now <= CF_HOLD;
}

struct cf_scan {
    ee_player *pl;
    int ns; ee_unit *sh[CF_SHOOTERS];    // enemy shooters out of their victims' reach
    int no; ee_unit *own[CF_OWN];        // own soldiers
    int nf; ee_unit *foe[4096];          // enemy fighters seen
};
internal void cf_unit(ee_unit *u, void *ctx) {
    struct cf_scan *s = ctx;
    ee_unit_type *td = unit_get_type_data(u);
    ee_player *owner = unit_get_owner(u);
    if (!td || !owner || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) return;
    if (owner == s->pl) {
        if (is_soldier(td) && !unit_get_holder(u) && s->no < CF_OWN) s->own[s->no++] = u;
        return;
    }
    if (!is_enemy(s->pl, owner) || !st_counts(u, td) || !seen_by(u, s->pl)) return;
    if (s->nf < 4096) s->foe[s->nf++] = u;
    ee_goal *g = unit_get_goal(u);
    ee_unit *v = g && goal_is_attack(g) ? goal_get_target(g) : NULL;
    if (!v || unit_get_owner(v) != s->pl || unit_get_hp(v) <= 0 || s->ns >= CF_SHOOTERS) return;
    float r = st_range(u), vr = st_range(v) + 1.0f;
    if (r >= CF_RANGE && (float)ivec2_dist2(unit_get_tile(u), unit_get_tile(v)) > vr * vr) s->sh[s->ns++] = u;
}

internal struct st_side g_cf_own, g_cf_foe;
internal void counter_fire(ee_player *pl, unsigned idx, uint32_t now) {
    static uint32_t last[16], logged[16];
    if (!g_cfire || is_vanilla(pl) || (now >= last[idx] && now - last[idx] < CF_EVERY)) return;
    last[idx] = now;
    struct cf_list *l = &g_cf[idx];
    int keep = 0;   // the commitments still running
    for (int k = 0; k < l->n; k++)
        if (now < l->e[k].until && l->e[k].until - now <= CF_HOLD) l->e[keep++] = l->e[k];
    l->n = keep;
    static struct cf_scan s;
    s.pl = pl; s.ns = s.no = s.nf = 0;
    grid_each(cf_unit, &s);
    static uint8_t done[CF_SHOOTERS];
    memset(done, 0, sizeof done);
    int added = 0;
    for (int i = 0; i < s.ns; i++) {
        if (done[i]) continue;
        ee_unit *e = s.sh[i];
        ivec2 c = unit_get_tile(e);
        ee_unit *cl[16];
        int ncl = 0;
        for (int j = i; j < s.ns; j++)
            if (!done[j] && ivec2_dist2(unit_get_tile(s.sh[j]), c) <= CF_CLUSTER * CF_CLUSTER) { done[j] = 1; if (ncl < 16) cl[ncl++] = s.sh[j]; }
        st_side_clear(&g_cf_foe); st_side_clear(&g_cf_own);
        for (int k = 0; k < s.nf; k++)
            if (ivec2_dist2(unit_get_tile(s.foe[k]), c) <= CF_ESCORT * CF_ESCORT) st_side_add(&g_cf_foe, s.foe[k]);
        unsigned area = area_id(c, 2);
        ee_unit_type *etd = unit_get_type_data(e);
        int pi = player_get_index(pl), ei = st_pidx(e);
        // candidates nearest first; those already on a shooter of it count from the start
        static struct { ee_unit *u; int d2; } cand[CF_OWN];
        int nc = 0, already = 0;
        for (int k = 0; k < s.no; k++) {
            ee_unit *u = s.own[k];
            if (!u) continue;
            ee_goal *g = unit_get_goal(u);
            ee_unit *t = g && goal_is_attack(g) ? goal_get_target(g) : NULL;
            int on = 0;
            for (int j = 0; j < ncl && t && !on; j++) on = t == cl[j];
            if (on) { st_side_add(&g_cf_own, u); already++; s.own[k] = NULL; continue; }
            int d2 = ivec2_dist2(unit_get_tile(u), c);
            if (d2 > CF_FROM * CF_FROM || counter_fire_unit(pl, u)) continue;
            if (t && abs(unit_get_x(t) - unit_get_x(u)) <= 4 && abs(unit_get_y(t) - unit_get_y(u)) <= 4) continue;   // busy close by
            ee_unit_type *td = unit_get_type_data(u);
            if (attack_bonus(td, etd, pi, ei) <= 0.0f) continue;
            if (unit_type_get_class_id(td) != CLS_HELICOPTER && (!area || area_id(unit_get_tile(u), 2) != area)) continue;
            cand[nc].u = u; cand[nc].d2 = d2; nc++;
        }
        for (int a = 1; a < nc; a++)   // insertion sort by distance (a few dozen)
            for (int b = a; b > 0 && cand[b].d2 < cand[b - 1].d2; b--) { __typeof__(cand[0]) t = cand[b]; cand[b] = cand[b - 1]; cand[b - 1] = t; }
        float ratio = st_ratio(&g_cf_own, &g_cf_foe);
        int take = 0;
        while (ratio < CF_WIN && take < nc && already + take < CF_MOST) {
            int step = take + 4 < nc ? 4 : nc - take;   // judged every 4 units
            for (int k = 0; k < step; k++) st_side_add(&g_cf_own, cand[take + k].u);
            take += step;
            ratio = st_ratio(&g_cf_own, &g_cf_foe);
        }
        if (ratio < CF_WIN) { g_cf_short[idx]++; continue; }
        if (!take) continue;   // enough on it already
        int sent = 0;
        for (int k = 0; k < take; k++) {
            ee_unit *u = cand[k].u;
            if (!unit_goal(u, goal_attack(u, cl[k % ncl]), 0, 0x1F40)) continue;
            sent++;
            if (l->n + added < CF_N) { l->e[l->n + added] = (struct cf_ent){ u, unit_get_id(u), now + CF_HOLD }; added++; }
            for (int j = 0; j < s.no; j++) if (s.own[j] == u) { s.own[j] = NULL; break; }
        }
        g_cf_sent[idx] += (unsigned)sent;
        g_cf_sorties[idx]++;
        if (g_log && sent)
            fprintf(g_log, "# counter-fire: P%u at %.1f min: %d shooters (type %d at %d,%d), %d enemy fighters within %d <- %d soldiers (%d already on them), judged %.2f\n",
                    idx, now / 60000.0, ncl, unit_type_get_id(etd), c.x, c.y, g_cf_foe.n, CF_ESCORT, sent, already, ratio);
    }
    if (added) {
        l->n += added;
        qsort(l->e, (size_t)l->n, sizeof *l->e, cf_cmp);
    }
    if (g_log && (now < logged[idx] || now - logged[idx] >= 60000)) {
        logged[idx] = now;
        fprintf(g_log, "# counter-fire: P%u at %.1f min: %u sorties, %u soldiers sent, %u times too weak to go\n", idx,
                now / 60000.0, g_cf_sorties[idx], g_cf_sent[idx], g_cf_short[idx]);
    }
}

internal void counter_fire_new_game(void) {
    memset(g_cf, 0, sizeof g_cf);
    memset(g_cf_sent, 0, sizeof g_cf_sent); memset(g_cf_sorties, 0, sizeof g_cf_sorties); memset(g_cf_short, 0, sizeof g_cf_short);
}
internal void counter_fire_install(void) {
    g_cfire = g_set_ai[1].counter_fire;
    if (g_log) fprintf(g_log, "# counter-fire: %d\n", g_cfire);
}
