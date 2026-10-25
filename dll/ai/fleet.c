// FLEET ([all] navy_fleet, default 2 (stage 4: navy_fleet=2, with massing: navy_fleet_mass); [autotest] v_navy_fleet for the variant players; docs/navy.md stage 3): the mod
// leads an AI's warships on a water map (navy_detect: enemies across the water) instead of the game's sea groups.
// Included by eeaimod.c after navy.c (navy_kind, nv_f, g_nv_map), landing.c (g_ld, ld_hits, ld_open_water, unit
// goals), intel.c (g_intel).
//
// The game's own AI has no fleet: sea attack subgroups (EECPASGSea) sail to a random unit or water area of a random
// enemy, the naval maintenance (0x4779FA) opens a Defend group of 2-3 ships - nothing guards the fishing boats, seeks
// the enemy fleet or keeps away from coastal towers and cannons (docs/navy.md 2). The playtester: on water maps losing
// sea control is a lost game; the whole fleet must keep out of the reach of coastal siege, cannons, artillery and
// towers (the x4-5 against battleships is an equalizer - frigates and galleys do badly against them too); fish is
// limited and the boats go further out over time; landings need the sea cleared and ships near.
// - Who: every warship and submarine of the AI (navy_kind NV_WAR / NV_SUB) afloat, not a landing's (landing_has_unit):
//   carriers, transports and fishing boats stay the game's. Led as landing.c leads its units: a sea subgroup holding
//   one of them is not updated (hook_asg_sea_update), the subgroups' orders to them are refused (hook_asg_give_goal,
//   vf57 - except the land subgroup's hand-off, which uses the goal after a refusal; never a fleet ship's caller).
//   The naval maintenance's unit deletion for room (0x430D40 from 0x477B17) is skipped while the fleet leads.
// - Danger (every 2 s): enemy towers and defence buildings, siege and land units that hit ships x3+ against
//   battleships or reach 7+ tiles - seen now, and with honest_vision those remembered (buildings until looked at
//   again, siege 5 minutes): their range + 2 tiles. No ship sits, sails or attacks inside; a ship inside sails out
//   (away from the nearest one), a target inside is dropped, a way that crosses one goes round (a waypoint off the
//   line) or not at all.
// - Strength: hit points x damage a second (attack x 1000 / (reload + 2000) - the time a shot takes in duels) x the
//   hit table's factor (nv_f) against the other side's ships, each side's damage spread over the other's hit points;
//   the ratio sqrt(own hp x own dps / (enemy hp x enemy dps)) - Lanchester's square law for the ships' fight.
// - Tasks, in this order:
//   escort: a landing is getting its transports (phase 1) and enemy warships are known on its sea route - the fleet
//     goes to the water off the beach (out of danger) and fights them, when it is not weaker (ratio >= 1); while the
//     landing sails or is ashore the ships it didn't take stay off its beach (out of danger) and fight ships there;
//   defend: enemy warships seen within 15 tiles of its buildings, fishing boats or guard point - attacked when the
//     ratio against them is >= 0.8 (at home: its towers and docks help), each ship the nearest it can hit;
//   hunt: the ratio against every enemy warship known (seen now, remembered 5 minutes) >= 1.3 and the fleet at 60%+
//     of its hit points - first gathered, then the enemy's warships, then its fishing boats and transports (seen now
//     or remembered 2 minutes), then the water off its nearest known dock (its fishing grounds); retreat (to the
//     guard point, 60 s at least) once the hit points fall below half of what it had when the hunt began, or the
//     enemy ships near the fleet are stronger (ratio < 0.8); its coast (seen within 4 tiles of the sea, 3 tiles
//     clear of danger) after the boats; test keys navy_fleet_raid (the ratio for boats, coast and fishing grounds,
//     130 - 100 was no better), navy_fleet_coast, navy_fleet_boats;
//   guard: the point between the fishing boats (or the docks) and the nearest known enemy building, on open water,
//     out of danger; ships within 6 tiles of it stay (they shoot at what comes in range).
//   Ships below 35% hit points go back to the dock (repair: logged as the hit points regained there).
// - Fishing boats: an enemy warship seen within 10 tiles of a boat and no fleet that wins there (ratio < 1 or the
//   fleet farther than 20 tiles) - the boat goes to its nearest dock (the drop site; the game gives it new work
//   there, as with the citizens sent home from danger).
// `# navy: P<n> fleet at N min:` every minute and on a change of task: task and why, where, ships and hit points,
// strength against the enemy ships known (and near), the danger zones, losses and kills, boats pulled back, orders.
internal int g_navy_fleet = 2, g_v_navy_fleet = -1;
internal int g_fl_boats = 1, g_fl_coast = 1, g_fl_raid = 130;   // navy_fleet_raid: the ratio (%) for raids on boats and the coast   // test: navy_fleet_boats (fishing boats pulled back), navy_fleet_coast (the hunt shells the coast)
#define FL_MAX 64
#define FL_DZ 384
#define FL_ES 160
enum { FT_GUARD, FT_DEFEND, FT_GATHER, FT_HUNT, FT_RETREAT, FT_ESCORT, FT_SUPPORT, FT_RAID, FT_PATROL, FT_SCOUT, FT_MASS, FT_N };
internal const char *const k_ft[FT_N] = { "guard", "defend", "gather", "hunt", "retreat", "escort", "support", "raid", "patrol", "scout", "mass" };
internal struct fleet {
    ee_unit *u[FL_MAX]; int n;
    uint32_t last, log_t, task_t, retreat_t;
    int task, last_task; const char *why;
    ivec2 g, go; int ocean;              // the guard point, where the fleet goes now, its sea
    ivec2 hv; int shelter, split;   // stage 4: the haven (water by an own tower or the dock farthest from the enemy), sheltering now, in task forces now
    ivec2 rally; int rcover, rcr, mass; unsigned masses, strikes, rally_towers; uint32_t rally_t, tower_t;   // stage 4 massing: the rally, towers covering it, its radius, massing now
    uint8_t lostm[10], sunkm[10]; uint32_t ring_m; unsigned shelters;   // warships lost / sunk per game minute (the last 10)
    float hp0, ratio, near_ratio; int eknown, enear, ndz;
    uint8_t *htgt; ivec2 hunt;           // the hunt's target (or point)
    struct { ivec2 p; uint32_t until; } cleared[8]; int ci;
    int docked_hp[FL_MAX]; ee_unit *docked[FL_MAX]; int ndocked; unsigned repaired;   // ships sent to the dock: their hp then
    unsigned orders, flees, pulls, hunts, retreats, defends, escorts, refused, maint_skipped, sg_skipped;
    // stage 4 (task forces): kills by its ships' targets (fleet_kill: warships, fishing boats, transports, citizens,
    // buildings, others), the targets now (pointer + unit id), the patrol's or the scouting's waypoint, the mod's orders
    unsigned kills[6], lost, lost_task[FT_N]; ee_unit *tgt[FL_MAX]; int tgt_id[FL_MAX], ntgt;   // lost: its ships sunk (lost_task: by its task then)
    int wp; uint32_t wp_t, build_t; unsigned built, build_refused;
    uint32_t bring_t[8]; int bring_i;   // the mod's warship orders of the last 90 s (not afloat yet)
} g_fl[16];
internal unsigned g_fl_refused;

internal int fleet_on(unsigned idx) {
    idx &= 15;
    return PV(idx, g_v_navy_fleet, g_navy_fleet) && g_nv_map[idx].done && g_nv_map[idx].strength > 0;
}
// stage 4 (navy_fleet=2): the size target and the mod's own warship orders (navy.c), task forces (below)
internal int fleet_builds(unsigned idx) { return fleet_on(idx) && PV(idx & 15, g_v_navy_fleet, g_navy_fleet) >= 2; }
internal int fleet_sheltering(unsigned idx) { return g_fl[idx & 15].shelter; }
internal int g_fl_mass = 1;   // navy_fleet_mass: stage 4 masses at the rally while weaker (fl_rally, below)
internal int g_fl_s4;   // set while a stage-4 fleet is led (fl_orders' preferences, fl_safe_toward's margin)
internal int fleet_has(ee_player *pl, ee_unit *u) {
    if (!pl || !u) return 0;
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    if (!fleet_on(idx)) return 0;
    struct fleet *f = &g_fl[idx];
    for (int k = 0; k < f->n; k++) if (f->u[k] == u) return 1;
    return 0;
}
// a sea subgroup holding one of the fleet's ships: its update skipped (landing.c hook_asg_sea_update)
internal int fleet_leads(ee_attack_subgroup *sg) {
    ee_player *pl = attack_subgroup_get_player(sg);
    if (!pl || !g_fl[(unsigned)player_get_index(pl) & 15].n) return 0;
    for (ee_asg_unit **r = attack_subgroup_get_units_begin(sg); r && r < attack_subgroup_get_units_end(sg); r++)
        if (*r && fleet_has(pl, asg_unit_get_unit(*r))) { g_fl[(unsigned)player_get_index(pl) & 15].sg_skipped++; return 1; }
    return 0;
}
// a subgroup's order (vf57) to one of the fleet's ships (landing.c hook_asg_give_goal refuses it)
internal int fleet_refuses(ee_attack_subgroup *sg, ee_unit *u) {
    ee_player *pl = attack_subgroup_get_player(sg);
    if (!fleet_has(pl, u)) return 0;
    g_fl_refused++; g_fl[(unsigned)player_get_index(pl) & 15].refused++;
    return 1;
}
// the naval maintenance (0x4779FA) short of its "Ships to Maintain": it deletes own units for the room (0x430D40,
// thiscall(player, request, 0)) and opens a Defend group - the deletion is skipped while the fleet leads the ships
internal int __thiscall hook_nv_maint_delete(ee_player *pl, void *req, int flag) {
    if (pl && !is_vanilla(pl) && fleet_on((unsigned)player_get_index(pl))) { g_fl[(unsigned)player_get_index(pl) & 15].maint_skipped++; return 0; }
    return delete_for_room(pl, req, flag);
}

// THE SCAN (every 2 s per AI with the fleet): own warships, fishing boats, docks and buildings; enemy ships; danger
struct fl_ship { ee_unit *u; ee_unit_type *td; ivec2 p; int hp, pi, seen; float dps; };
internal struct fl_scan {
    ee_player *pl; unsigned idx;
    struct fl_ship es[FL_ES]; int ne;                 // enemy warships and submarines (seen now, then remembered)
    struct fl_ship ev[64]; int nev;                   // enemy fishing boats and transports (prey)
    struct fl_ship ec[96]; int nec;                   // enemy units and buildings on the coast of the fleet's sea (seen now)
    struct { ivec2 p; float r; } dz[FL_DZ];           // danger zones (radius incl. the margin)
    ee_unit *boat[96]; int nb;                        // own fishing boats
    ivec2 b[256]; int nbl;                            // own buildings
    ivec2 dk[16]; int nd;                             // own docks
    ivec2 eb, ed[16]; int eb_known, ned;              // the nearest known enemy building (to the guard base), enemy docks
    ivec2 tw[32]; int ntw; float twr[32];             // own towers and defence buildings (their range)
} g_fs;
internal float fl_dps(ee_unit *u, ee_unit_type *td) {
    ee_unit_stats *st = u ? unit_get_stats(u) : NULL;
    float a = st ? unit_stats_get_attack(st) : (float)unit_type_get_attack(td);
    int rl = unit_type_get_reload(td);
    return a > 0 ? a * 1000.0f / (float)((rl > 0 ? rl : 2000) + 2000) : 0.0f;
}
internal float fl_range(ee_unit *u, ee_unit_type *td) {
    ee_unit_stats *st = u ? unit_get_stats(u) : NULL;
    float r = st ? unit_stats_get_range(st) : unit_type_get_range(td) + 1.0f;   // remembered: the base range + an upgrade
    return r > 0 && r < 40 ? r : 0;
}
// does a land unit or building of type td threaten ships: a defence with a ground attack (attack.c defence_stats), or
// a unit whose weapon hits battleships x3+ (catapults, cannons, artillery ...) or reaches 7+ tiles, and hits ships
internal float fl_danger_r(ee_unit *u, ee_unit_type *td) {
    int fam = unit_type_get_family(td), t = unit_type_get_id(td), kind = unit_type_get_weapon(td);
    if (fam == 4 || fam == 10 || fam == 18 || fam == 21 || fam == 22 || fam == 3 || fam == 7 || fam == 17) return 0;   // afloat, aircraft
    if (unit_type_get_attack(td) <= 0 || (nv_f(kind, NV_C_FRIG) <= 0 && nv_f(kind, NV_C_LINE) <= 0)) return 0;
    if (fam == 2 || fam == 29) {
        float r, a;
        if (u) return defence_stats(u, &r, &a) ? r : 0;
        return t == UT(B_AA10_GUN_88MM) || t == UT(B_AA13_MISSILE_TOWER) || unit_type_get_range(td) <= 1.0f ? 0 : unit_type_get_range(td) + 1.0f;
    }
    float r = fl_range(u, td);
    return (nv_f(kind, NV_C_LINE) >= 3.0f || r >= 7.0f) && r >= 2.0f ? r : 0;
}
internal void fl_dz_add(struct fl_scan *s, ivec2 p, float r) {
    struct fleet *f = &g_fl[s->idx];   // the count: the fleet's ndz (its log)
    if (r <= 0 || p.x < 0) return;
    if (f->ndz >= FL_DZ) return;
    for (int k = 0; k < f->ndz; k++) if (ivec2_eq(s->dz[k].p, p) && s->dz[k].r >= r + 2.0f) return;
    s->dz[f->ndz].p = p; s->dz[f->ndz++].r = r + 2.0f;
}
internal int fl_in_danger(ivec2 p) {
    struct fleet *f = &g_fl[g_fs.idx];
    for (int k = 0; k < f->ndz; k++)
        if ((float)ivec2_dist2(p, g_fs.dz[k].p) <= g_fs.dz[k].r * g_fs.dz[k].r) return k + 1;
    return 0;
}
internal int fl_in_danger_m(ivec2 p, float m) {   // within a zone + m tiles
    struct fleet *f = &g_fl[g_fs.idx];
    for (int k = 0; k < f->ndz; k++) {
        float r = g_fs.dz[k].r + m;
        if ((float)ivec2_dist2(p, g_fs.dz[k].p) <= r * r) return 1;
    }
    return 0;
}
internal int fl_way_safe(ivec2 a, ivec2 b) {   // the straight way, every 2 tiles (the start left out)
    float dx = (float)(b.x - a.x), dy = (float)(b.y - a.y), len = sqrtf(dx * dx + dy * dy);
    int steps = (int)(len / 2.0f) + 1;
    for (int i = 1; i <= steps; i++) {
        float t = (float)i / (float)steps;
        if (fl_in_danger(iv2(a.x + (int)(dx * t), a.y + (int)(dy * t)))) return 0;
    }
    return 1;
}
internal int fl_water(ivec2 p, unsigned oc, ivec2 *o) {   // open water of the sea oc near p, else any of it
    if (ld_open_water(p, oc, o)) return 1;
    for (int d = 0; d <= 8; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d || !d ? 1 : 2 * d))
                if (area_id(iv2(p.x + dx, p.y + dy), 1) == oc) { *o = iv2(p.x + dx, p.y + dy); return 1; }
    return 0;
}
// a safe water point on the way from `from` to `to`, as near to the target as it gets (2-tile steps back)
internal int fl_safe_toward(ivec2 from, ivec2 to, unsigned oc, ivec2 *o) {
    float dx = (float)(from.x - to.x), dy = (float)(from.y - to.y), len = sqrtf(dx * dx + dy * dy);
    int steps = (int)(len / 2.0f) + 1;
    for (int i = 0; i <= steps; i++) {
        float t = (float)i / (float)steps;
        ivec2 q = iv2(to.x + (int)(dx * t), to.y + (int)(dy * t)), w;
        if (area_id(q, 1) != oc) continue;
        if (g_fl_s4) {   // stage 4: 3 tiles clear of a zone - ships spread round the point, and at the edge they went in and out (1900 flees an hour)
            if (!fl_in_danger_m(q, 3.0f) && fl_water(q, oc, &w) && !fl_in_danger_m(w, 3.0f)) { *o = w; return 1; }
            continue;
        }
        if (!fl_in_danger(q) && fl_water(q, oc, &w) && !fl_in_danger(w)) { *o = w; return 1; }
    }
    return 0;
}
internal int fl_coast(ivec2 p, int oc) {   // land with the sea oc within 4 tiles (12 looks)
    static const signed char d[12][2] = { { 2, 0 }, { -2, 0 }, { 0, 2 }, { 0, -2 }, { 4, 0 }, { -4, 0 }, { 0, 4 }, { 0, -4 }, { 3, 3 }, { -3, 3 }, { 3, -3 }, { -3, -3 } };
    if (area_id(p, 1)) return 0;
    for (int k = 0; k < 12; k++) if ((int)area_id(iv2(p.x + d[k][0], p.y + d[k][1]), 1) == oc) return 1;
    return 0;
}
internal void fl_scan(ee_player *pl, unsigned idx) {
    struct fl_scan *s = &g_fs;
    struct fleet *f = &g_fl[idx];
    s->pl = pl; s->idx = idx;
    s->ne = s->nev = s->nec = s->nb = s->nbl = s->nd = s->ned = s->ntw = 0; s->eb_known = 0;
    f->n = 0; f->ndz = 0;
    void *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    ivec2 h = bm ? build_mgr_get_base(bm) : iv2(0, 0);
    int bd = 0x7FFFFFFF;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        ee_unit_type *td = it.td;
        ee_player *o = unit_get_owner(u);
        int x = unit_get_x(u), y = unit_get_y(u), fam = unit_type_get_family(td);
        if (!o || unit_get_hp(u) <= 0 || x < 0) continue;
        int k = navy_kind(td);
        if (o == pl) {
            if ((k == NV_WAR || k == NV_SUB) && f->n < FL_MAX && !landing_has_unit(pl, u)) f->u[f->n++] = u;
            else if (k == NV_FISH && s->nb < 96) s->boat[s->nb++] = u;
            else if (fam == 2 || fam == 29) {
                if (unit_type_get_id(td) == UT(B_DOCK) && s->nd < 16) s->dk[s->nd++] = iv2(x, y);
                float tr, ta;
                if (s->ntw < 32 && defence_stats(u, &tr, &ta)) { s->tw[s->ntw] = iv2(x, y); s->twr[s->ntw++] = tr; }
                if (s->nbl < 256) s->b[s->nbl++] = iv2(x, y);
            }
            continue;
        }
        if (player_get_index(o) <= 0 || !is_enemy(pl, o) || !seen_by(u, pl)) continue;
        if (k == NV_WAR || k == NV_SUB) {
            if (s->ne < FL_ES) { struct fl_ship *e = &s->es[s->ne++]; e->u = u; e->td = td; e->p = iv2(x, y); e->hp = unit_get_hp(u); e->pi = ld_pidx(u); e->seen = 1; e->dps = fl_dps(u, td); }
            continue;
        }
        if (k == NV_FISH || k == NV_TRANSPORT) {
            if (s->nev < 64) { struct fl_ship *e = &s->ev[s->nev++]; e->u = u; e->td = td; e->p = iv2(x, y); e->hp = unit_get_hp(u); e->pi = ld_pidx(u); e->seen = 1; e->dps = 0; }
            continue;
        }
        if (f->ocean && s->nec < 96 && (fam == 2 || fam == 29 || is_soldier(td) || is_siege(unit_type_get_id(td))) && !is_wall(unit_type_get_id(td)) && fl_coast(iv2(x, y), f->ocean)) {
            struct fl_ship *e = &s->ec[s->nec++]; e->u = u; e->td = td; e->p = iv2(x, y); e->hp = unit_get_hp(u); e->pi = ld_pidx(u); e->seen = 1; e->dps = 0;
        }
        if (fam == 2 || fam == 29) {
            int d = ivec2_dist2(iv2(x, y), h);
            if (d < bd) { bd = d; s->eb = iv2(x, y); s->eb_known = 1; }
            if (unit_type_get_id(td) == UT(B_DOCK) && s->ned < 16) s->ed[s->ned++] = iv2(x, y);
        }
        fl_dz_add(s, iv2(x, y), fl_danger_r(u, td));
    }
    if (!g_honest_vision) return;
    // remembered (intel.c): buildings until their spot is looked at again, siege and ships 5 minutes, prey 2
    struct intel *in = &g_intel[idx];
    for (int k = 0; k < in->n; k++) {
        struct intel_rec *e = &in->r[k];
        if (e->t == in->last || e->x < 0) continue;   // seen now: in the scan above
        ee_unit_type *td = type_data(e->type);
        if (!td) continue;
        uint32_t age = in->now - e->t;
        int nk = navy_kind(td);
        if (e->kind == 0) {
            int d = ivec2_dist2(iv2(e->x, e->y), h);
            if (d < bd) { bd = d; s->eb = iv2(e->x, e->y); s->eb_known = 1; }
            if (e->type == UT(B_DOCK) && s->ned < 16) s->ed[s->ned++] = iv2(e->x, e->y);
            fl_dz_add(s, iv2(e->x, e->y), fl_danger_r(NULL, td));
        } else if (e->kind == 1 && age <= 300000) fl_dz_add(s, iv2(e->x, e->y), fl_danger_r(NULL, td));
        else if ((nk == NV_WAR || nk == NV_SUB) && age <= 300000 && s->ne < FL_ES) {
            struct fl_ship *q = &s->es[s->ne++];
            q->u = NULL; q->td = td; q->p = iv2(e->x, e->y); q->hp = e->hp; q->pi = e->owner; q->seen = 0; q->dps = fl_dps(NULL, td);
        } else if ((nk == NV_FISH || nk == NV_TRANSPORT) && age <= 120000 && s->nev < 64) {
            struct fl_ship *q = &s->ev[s->nev++];
            q->u = NULL; q->td = td; q->p = iv2(e->x, e->y); q->hp = e->hp; q->pi = e->owner; q->seen = 0; q->dps = 0;
        }
    }
}
// the strength ratio of own ships (all, or within r of p) against the enemy ships listed (sel: only those with
// mask bit set); 99 when the enemy has nothing that hurts them
internal float fl_ratio(struct fleet *f, ivec2 p, int r, const unsigned char *sel) {
    float ho = 0, he = 0, dofs = 0, de = 0;
    for (int j = 0; j < g_fs.ne; j++) if (!sel || sel[j]) he += (float)g_fs.es[j].hp;
    int any = 0;
    for (int i = 0; i < f->n; i++) {
        ee_unit *u = f->u[i];
        if (r > 0 && ivec2_dist2(unit_get_tile(u), p) > r * r) continue;
        ho += (float)unit_get_hp(u); any = 1;
    }
    if (he <= 0) return 99.0f;
    if (!any || ho <= 0) return 0.0f;
    for (int i = 0; i < f->n; i++) {
        ee_unit *u = f->u[i];
        ee_unit_type *td = unit_get_type_data(u);
        if (r > 0 && ivec2_dist2(unit_get_tile(u), p) > r * r) continue;
        float fh = 0, hpu = (float)unit_get_hp(u), dp = fl_dps(u, td);
        for (int j = 0; j < g_fs.ne; j++) {
            if (sel && !sel[j]) continue;
            struct fl_ship *e = &g_fs.es[j];
            fh += (float)e->hp * nv_f(unit_type_get_weapon(td), unit_type_get_category(e->td));
            de += e->dps * hpu * nv_f(unit_type_get_weapon(e->td), unit_type_get_category(td));
        }
        dofs += dp * fh / he;
    }
    de /= ho;
    if (de <= 0) return dofs > 0 ? 99.0f : 1.0f;
    if (dofs <= 0) return 0.0f;
    return sqrtf(ho * dofs / (he * de));
}
internal ivec2 fl_center(struct fleet *f) {   // its ships' middle, else the guard point
    float x = 0, y = 0;
    for (int i = 0; i < f->n; i++) { x += unit_get_x(f->u[i]); y += unit_get_y(f->u[i]); }
    return f->n ? iv2((int)(x / f->n), (int)(y / f->n)) : f->g;
}
internal float fl_hp(struct fleet *f) {
    float h = 0;
    for (int i = 0; i < f->n; i++) h += (float)unit_get_hp(f->u[i]);
    return h;
}
internal float fl_hp_max(ee_unit *u) {
    ee_unit_stats *st = unit_get_stats(u);
    int m = st ? unit_stats_get_hp(st) : 0;
    return (float)(m > 0 ? m : unit_type_get_hp(unit_get_type_data(u)));
}
// the guard point: from the fishing boats' middle (the docks' with fewer than 2 boats) 5 tiles toward the nearest
// known enemy building, on open water of the fleet's sea, out of danger (else nearer to the boats / docks)
internal void fl_guard_point(struct fleet *f) {
    float bx = 0, by = 0; int n = 0;
    for (int k = 0; k < g_fs.nb; k++) {
        ee_unit *b = g_fs.boat[k];
        if (area_id(unit_get_tile(b), 1) == (unsigned)f->ocean) { bx += unit_get_x(b); by += unit_get_y(b); n++; }
    }
    if (n < 2) {
        bx = by = 0; n = 0;
        for (int k = 0; k < g_fs.nd; k++) { unsigned oc = ocean_near(g_fs.dk[k], 4, NULL); if ((int)oc == f->ocean) { bx += g_fs.dk[k].x; by += g_fs.dk[k].y; n++; } }
    }
    if (!n) return;
    ivec2 m = iv2((int)(bx / n), (int)(by / n)), t = m;
    if (g_fs.eb_known) {
        float dx = (float)(g_fs.eb.x - m.x), dy = (float)(g_fs.eb.y - m.y), len = sqrtf(dx * dx + dy * dy);
        if (len > 1) t = iv2(m.x + (int)(5.0f * dx / len), m.y + (int)(5.0f * dy / len));
    }
    fl_safe_toward(m, t, (unsigned)f->ocean, &f->g);
}
internal int fl_ship_ocean(ee_unit *u) { return (int)area_id(unit_get_tile(u), 1); }
internal int fl_attacking(ee_unit *u) {   // its current attack target (alive), else NULL
    ee_goal *g = unit_get_goal(u);
    ee_unit *t = g && goal_is_attack(g) ? goal_get_target(g) : NULL;
    return t && unit_get_hp(t) > 0 && unit_get_x(t) >= 0;
}
internal ee_unit *fl_target_of(ee_unit *u) {
    ee_goal *g = unit_get_goal(u);
    return g && goal_is_attack(g) ? goal_get_target(g) : NULL;
}
// to t - round a danger zone on the way through a waypoint off the line (8 or 16 tiles to a side), else not at all
internal void fl_move(struct fleet *f, ee_unit *u, ivec2 t, int k) {
    ivec2 up = unit_get_tile(u);
    int oc = fl_ship_ocean(u);
    if (!oc) oc = f->ocean;
    t = ivec2_add(t, iv2((k % 3) - 1, (k / 3 % 3) - 1));   // not all on one tile
    ivec2 d = ivec2_sub(t, up);
    if (ivec2_len2(d) <= 9) return;
    if (!fl_way_safe(up, t)) {
        int ok = 0;
        float len = sqrtf((float)ivec2_len2(d)), px = -(float)d.y / len, py = (float)d.x / len;
        static const int off[4] = { 8, -8, 16, -16 };
        for (int j = 0; j < 4 && !ok; j++) {
            ivec2 mid = iv2((up.x + t.x) / 2 + (int)(px * (float)off[j]), (up.y + t.y) / 2 + (int)(py * (float)off[j])), w;
            if (!fl_water(mid, (unsigned)oc, &w) || fl_in_danger(w) || !fl_way_safe(up, w)) continue;
            t = w; ok = 1;
        }
        if (!ok) return;
    }
    vec2 c = ivec2_centre(t);
    if (goal_near(u, c.x, c.y, 2.5f)) return;
    f->orders += unit_goal(u, goal_move(c.x, c.y), 0, 0x1F40);
}
// out of a danger zone: away from its centre to its edge + 3 tiles (or 45/90 degrees off that), else the guard point
internal int fl_flee(struct fleet *f, ee_unit *u) {
    int ux = unit_get_x(u), uy = unit_get_y(u), z = fl_in_danger(iv2(ux, uy)), oc = fl_ship_ocean(u);
    if (!z) return 0;
    z--;
    float dx = (float)(ux - g_fs.dz[z].p.x), dy = (float)(uy - g_fs.dz[z].p.y), len = sqrtf(dx * dx + dy * dy);
    if (len < 0.5f) { dx = (float)(f->g.x - ux); dy = (float)(f->g.y - uy); len = sqrtf(dx * dx + dy * dy); }
    if (len < 0.5f) { dx = 1; dy = 0; len = 1; }
    dx /= len; dy /= len;
    float go = g_fs.dz[z].r - len + 3.0f;
    if (go < 3) go = 3;
    static const float ang[5] = { 0, 0.785f, -0.785f, 1.571f, -1.571f };
    for (int j = 0; j < 5; j++) {
        float c = cosf(ang[j]), s = sinf(ang[j]), ex = dx * c - dy * s, ey = dx * s + dy * c;
        ivec2 w;
        if (!oc || !fl_water(iv2(ux + (int)(ex * go), uy + (int)(ey * go)), (unsigned)oc, &w) || fl_in_danger(w)) continue;
        vec2 wc = ivec2_centre(w);
        if (!goal_near(u, wc.x, wc.y, 2.5f)) f->orders += unit_goal(u, goal_move(wc.x, wc.y), 0, 0x1F40);
        f->flees++;
        return 1;
    }
    vec2 g = ivec2_centre(f->g);
    if (!goal_near(u, g.x, g.y, 2.5f)) f->orders += unit_goal(u, goal_move(g.x, g.y), 0, 0x1F40);
    f->flees++;
    return 1;
}
// the nearest enemy ship seen now (in sel) that u can hit, out of danger, within r of p
internal ee_unit *fl_pick(ee_unit *u, const unsigned char *sel, ivec2 p, int r) {
    ee_unit *best = NULL;
    ivec2 up = unit_get_tile(u);
    int oc = fl_ship_ocean(u);
    float bd = 1e9f;
    for (int j = 0; j < g_fs.ne; j++) {
        struct fl_ship *e = &g_fs.es[j];
        if (!e->seen || (sel && !sel[j]) || fl_in_danger_m(e->p, 3.0f)) continue;
        if (ivec2_dist2(e->p, p) > r * r || (oc && (int)area_id(e->p, 1) != oc) || !ld_hits(u, e->u)) continue;
        float d = (float)ivec2_dist2(e->p, up);
        if (g_fl_s4 && unit_type_get_family(e->td) == 10 && nv_role(unit_get_type_data(u)) == NR_FRIG) d *= 0.25f;   // frigates: the submarines first
        if (d < bd) { bd = d; best = e->u; }
    }
    return best;
}
// the leash while massing (fl_decide FLD_MASS): a ship keeps fighting only a target within g_fl_lr of the rally - it
// doesn't chase an enemy out of the towers' cover
internal ivec2 g_fl_leash; internal int g_fl_lr;
internal int cover_sea(unsigned idx);   // cover.c: fall_back 4 SEA
// SEA (fall_back 4): where a retreat and a damaged force go - under the towers (the rally while towers cover it), else
// the haven (by the dock farthest from the enemy), else the guard point
internal void fl_cover_point(unsigned idx, ivec2 *t) {
    struct fleet *m = &g_fl[idx & 15];
    if (m->rcover && (m->rally.x || m->rally.y)) *t = m->rally;
    else if (m->hv.x || m->hv.y) *t = m->hv;
}
// SEA: the dock a damaged ship goes to - the nearest out of danger, one with an own tower within 8 tiles counting
// as half the way; -1 none
internal int fl_repair_dock(ivec2 p) {
    int b = -1;
    float bd = 1e18f;
    for (int k = 0; k < g_fs.nd; k++) {
        if (fl_in_danger(g_fs.dk[k])) continue;
        int cov = 0;
        for (int j = 0; j < g_fs.ntw && !cov; j++) cov = ivec2_dist2(g_fs.tw[j], g_fs.dk[k]) <= 64;
        float d = sqrtf((float)ivec2_dist2(g_fs.dk[k], p)) * (cov ? 0.5f : 1.0f);
        if (d < bd) { bd = d; b = k; }
    }
    return b;
}
// each ship: out of danger first; a target inside a danger zone dropped; then the task's target, else to p
internal void fl_orders(struct fleet *f, const unsigned char *sel, ivec2 p, int r, int hunt_prey, int retreat) {
    for (int i = 0; i < f->n; i++) {
        ee_unit *u = f->u[i];
        if (fl_flee(f, u)) continue;
        ee_unit *cur = fl_target_of(u);
        // SEA: a damaged ship off to the dock before anything else (it kept fighting at 34% while it had a target) and
        // until 80% (it sailed again at 35%)
        int sea = cover_sea(g_fs.idx), dock = -1;
        if (sea && g_fs.nd) {
            struct fleet *m = &g_fl[g_fs.idx];
            int hp = unit_get_hp(u), j;
            for (j = 0; j < m->ndocked && m->docked[j] != u; j++) ;
            if ((hp < 0.5f * fl_hp_max(u) || j < m->ndocked) && (dock = fl_repair_dock(unit_get_tile(u))) >= 0) {
                if (j == m->ndocked && j < FL_MAX) { m->docked[j] = u; m->docked_hp[j] = hp; m->ndocked++; }
                ivec2 w;
                // to the water by the dock (the dock's own tile is land), once within 4 tiles of it no more orders
                if (ivec2_dist2(g_fs.dk[dock], unit_get_tile(u)) > 16 && ocean_near(g_fs.dk[dock], 4, &w)) fl_move(f, u, w, 0);
                continue;
            }
        }
        if (retreat) { fl_move(f, u, p, i); continue; }   // off the fight
        int leashed = 0;
        if (g_fl_lr && cur) leashed = ivec2_dist2(unit_get_tile(cur), g_fl_leash) > g_fl_lr * g_fl_lr;
        if (fl_attacking(u) && !leashed && !fl_in_danger(unit_get_tile(cur))) continue;   // keeps fighting
        if (unit_get_hp(u) < 0.35f * fl_hp_max(u) && g_fs.nd) {   // to the dock (repair)
            int b = 0, bd = 0x7FFFFFFF;
            for (int k = 0; k < g_fs.nd; k++) { int d = ivec2_dist2(g_fs.dk[k], unit_get_tile(u)); if (d < bd) { bd = d; b = k; } }
            int j;
            struct fleet *m = &g_fl[g_fs.idx];   // the repair is counted for the whole fleet (task forces: their ships)
            for (j = 0; j < m->ndocked && m->docked[j] != u; j++) ;
            if (j == m->ndocked && j < FL_MAX) { m->docked[j] = u; m->docked_hp[j] = unit_get_hp(u); m->ndocked++; }
            if (bd > 16) fl_move(f, u, g_fs.dk[b], 0);
            continue;
        }
        ee_unit *t = fl_pick(u, sel, p, r);
        for (int pass = 0; !t && hunt_prey && pass < 2; pass++) {   // the hunt: prey seen now near the hunt point, then the coast
            float bd = 1e9f;
            struct fl_ship *list = pass ? g_fs.ec : g_fs.ev;
            for (int j = 0; j < (pass ? g_fs.nec : g_fs.nev); j++) {
                struct fl_ship *e = &list[j];
                if (!e->seen || fl_in_danger_m(e->p, 3.0f) || !ld_hits(u, e->u)) continue;
                float d = (float)ivec2_dist2(e->p, p);
                if (g_fl_s4 && pass) d *= is_citizen_type(unit_type_get_id(e->td)) ? 0.25f : is_soldier(e->td) ? 2.0f : 1.0f;   // the economy first
                if (d <= (float)(r * r) && d < bd) { bd = d; t = e->u; }
            }
        }
        if (t) {
            if (cur != t) f->orders += unit_goal(u, goal_attack(u, t), 0, 0x1F40);
            continue;
        }
        if (fl_ship_ocean(u) == f->ocean || !fl_ship_ocean(u)) fl_move(f, u, p, i);
    }
}
// fishing boats near enemy warships the fleet doesn't win against: to the nearest dock
internal void fl_boats(struct fleet *f, ivec2 c) {
    static unsigned char sel[FL_ES];
    if (!g_fl_boats) return;
    for (int k = 0; k < g_fs.nb; k++) {
        ee_unit *b = g_fs.boat[k];
        ivec2 bp = unit_get_tile(b);
        int nfoe = 0;
        memset(sel, 0, sizeof sel);
        for (int j = 0; j < g_fs.ne; j++) {
            struct fl_ship *e = &g_fs.es[j];
            if (!e->seen || ivec2_dist2(e->p, bp) > 100 || !ld_hits(e->u, b)) continue;
            sel[j] = 1; nfoe++;
        }
        if (!nfoe) continue;
        if (f->n && ivec2_dist2(c, bp) <= 400 && fl_ratio(f, iv2(0, 0), 0, sel) >= 1.0f) continue;   // the fleet is there and wins
        int bd = 0x7FFFFFFF, bk = -1;
        for (int d = 0; d < g_fs.nd; d++) { int dd = ivec2_dist2(g_fs.dk[d], bp); if (dd < bd) { bd = dd; bk = d; } }
        if (bk < 0 || bd <= 16) continue;
        vec2 dc = ivec2_centre(g_fs.dk[bk]);
        if (goal_near(b, dc.x, dc.y, 3.0f)) continue;
        if (unit_goal(b, goal_move(dc.x, dc.y), 0, 0x1F40)) f->pulls++;
    }
}
internal uint32_t fl_lost(unsigned idx, int sunk) {   // warships and submarines lost (sunk: of the enemy, by this AI)
    uint32_t n = 0;
    for (int t = 0; t < G_TYPES_N; t++) {
        unsigned v = sunk ? g_nv_sunk_type[idx][t] : g_nv_lost_type[idx][t];
        if (v && nv_role(type_data(t)) != NR_NONE) n += v;
    }
    return n;
}
// TASK FORCES (stage 4, navy_fleet=2; docs/navy.md): the playtester - a fleet of 20+ warships split into smaller
// forces with jobs, none sitting idle ("any unit that just sits there is a huge opportunity cost"): a home guard for
// the fishing grounds and docks, hunters that strike the enemy's warships before they mass, raiders against its fishing
// boats and the citizens and docks on its coast (the economy), a patrol on the sea lanes against transports and
// expansion, the escort of a landing. Combined arms: each force gets the ships that fit its job (k_tf_fit) - frigates
// in every force that may meet submarines (they hunt them first: fl_pick), battleships and submarines with the hunters,
// frigates and galleys with the raiders and the patrol. A known submarine is a target for the frigates, not a reason
// to stop: the ratio is worked out against the enemy group a force goes for, not against everything known.
enum { TF_HOME, TF_HUNT, TF_RAID, TF_PATROL, TF_ESCORT, TF_N };
internal const char *const k_tf[TF_N] = { "home", "hunt", "raid", "patrol", "escort" };
internal struct fleet g_tf[16][TF_N];
internal struct { ee_unit *u; int id; signed char tf; } g_tfm[16][FL_MAX];   // the ship's force (pointer + unit id +0x60)
internal int g_ntfm[16], g_help;
// how well a role fits a force (NR_NONE, F, L, A, S, AA)
internal const signed char k_tf_fit[TF_N][NR_N] = {
    { 1, 1, 2, 2, 1, 3 },   // home: battleships and galleys by the towers; cruisers against aircraft
    { 1, 2, 3, 2, 3, 1 },   // hunt: the line - battleships, submarines; frigates against subs and galleys
    { 1, 3, 1, 3, 1, 0 },   // raid: frigates and galleys (galleys x2 against land units, x4 buildings)
    { 1, 3, 1, 1, 2, 0 },   // patrol: frigates (transports, subs), submarines
    { 1, 2, 2, 2, 1, 2 },   // escort
};

internal void fl_log(ee_player *pl, unsigned idx, struct fleet *f, uint32_t now, ivec2 c) {
    if (!g_log) return;
    float hp = fl_hp(f), hm = 0;
    for (int i = 0; i < f->n; i++) hm += fl_hp_max(f->u[i]);
    uint32_t blost = 0, bsunk = 0;
    ee_combat_mgr *cbm = player_get_combat_mgr(pl); ee_attack_cmd *cmd = cbm ? combat_mgr_get_commander(cbm) : NULL;
    unsigned ngroups = cmd ? (unsigned)attack_cmd_get_group_count(cmd) : 0;   // the attack commander's groups (frozen ones pile up?)
    for (int j = 0; j < 16; j++) { blost += g_nv_boat_kills[j][idx]; bsunk += g_nv_boat_kills[idx][j]; }
    struct fleet s = *f;   // stage 4: the counters of the task forces added
    int s4 = fleet_builds(idx);
    for (int t = 0; s4 && t < TF_N; t++) {
        struct fleet *q = &g_tf[idx][t];
        s.orders += q->orders; s.flees += q->flees; s.hunts += q->hunts; s.retreats += q->retreats; s.defends += q->defends; s.escorts += q->escorts;
    }
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# navy: P%u fleet at %.1f min: %s (%s) | at %d,%d -> %d,%d guard %d,%d | ships %d hp %.0f (%.0f%%) | ratio %.2f vs %d known, near %.2f vs %d"
                   " | danger %d | lost %u sunk %u boats-lost %u boats-sunk %u | pulled %u flees %u hunts %u retreats %u defends %u escorts %u"
                   " | repaired %u | orders %u refused %u sea-updates-skipped %u maint-skipped %u | attack groups %u",
            idx, now / 60000.0, k_ft[f->task], f->why ? f->why : "-", c.x, c.y, f->go.x, f->go.y, f->g.x, f->g.y, f->n, hp, hm > 0 ? 100.0f * hp / hm : 0.0f,
            f->ratio > 98 ? 99.0f : f->ratio, f->eknown, f->near_ratio > 98 ? 99.0f : f->near_ratio, f->enear, f->ndz,
            fl_lost(idx, 0), fl_lost(idx, 1), blost, bsunk, s.pulls, s.flees, s.hunts, s.retreats, s.defends, s.escorts,
            f->repaired, s.orders, f->refused, f->sg_skipped, f->maint_skipped, ngroups);
    if (s4) {
        fprintf(g_log, " | target %d built %u refused %u | haven %d,%d%s | forces", g_nv_target[idx], f->built, f->build_refused, f->hv.x, f->hv.y, f->shelter ? " sheltering" : f->split ? " split" : " one fleet");
        fprintf(g_log, " (%u times)", f->shelters);
        if (g_fl_mass) fprintf(g_log, " | rally %d,%d towers %d %s (massed %u, struck %u, towers asked %u)", f->rally.x, f->rally.y, f->rcover,
                               f->mass ? "massing" : "striking", f->masses, f->strikes, f->rally_towers);
        for (int t = 0; t < TF_N; t++) fprintf(g_log, " %s %d", k_tf[t], g_tf[idx][t].n);
        fprintf(g_log, " | lost by force");
        for (int t = 0; t < TF_N; t++) fprintf(g_log, " %s %u", k_tf[t], g_tf[idx][t].lost);
        fprintf(g_log, " | by task");
        for (int t = 0; t < FT_N; t++) fprintf(g_log, " %s %u", k_ft[t], f->lost_task[t]);
    }
    fprintf(g_log, " | kills W%u B%u T%u C%u Bd%u O%u\n", f->kills[0], f->kills[1], f->kills[2], f->kills[3], f->kills[4], f->kills[5]);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}
internal void fl_set(struct fleet *f, int task, const char *why, uint32_t now) {
    if (f->task != task) { f->task = task; f->task_t = now; }
    f->why = why;
}
// the targets its ships attack now (fleet_kill: whose kill it was) - pointers compared only, never read later
internal void fl_targets(struct fleet *f) {
    f->ntgt = 0;
    for (int i = 0; i < f->n; i++) {
        ee_unit *t = fl_target_of(f->u[i]);
        if (t && f->ntgt < FL_MAX) { f->tgt[f->ntgt] = t; f->tgt_id[f->ntgt++] = unit_get_id(t); }
    }
}
// the minute ring of warships lost and sunk (stage 4: losing at sea)
internal void fl_ring(struct fleet *m, int sunk) {
    uint32_t mi = game_time() / 60000u + 1;
    if (m->ring_m != mi) {
        for (uint32_t k = m->ring_m + 1; m->ring_m && k <= mi && k < m->ring_m + 11; k++) { m->lostm[k % 10] = 0; m->sunkm[k % 10] = 0; }
        if (!m->ring_m || mi < m->ring_m || mi >= m->ring_m + 11) { memset(m->lostm, 0, sizeof m->lostm); memset(m->sunkm, 0, sizeof m->sunkm); }
        m->ring_m = mi;
    }
    if (sunk > 0) m->sunkm[mi % 10]++;
    else if (sunk == 0) m->lostm[mi % 10]++;
}
// a unit of the enemy died (diplo.c hook_kill_stat, k the killer's index, n 2 a building, 3 a unit): the force (stage 4)
// or the fleet (stage 3) whose ship had it as its target gets the kill - warship, fishing boat, transport, citizen,
// building, other
internal void fleet_kill(int k, ee_unit *dead, int n) {
    if (k <= 0 || k >= 16 || !dead || !g_fl[k].n || !fleet_on((unsigned)k)) return;
    ee_unit_type *td = unit_get_type_data(dead);
    int id = unit_get_id(dead), c = 5;
    if (n == 2) c = 4;
    else if (td) {
        int nk = navy_kind(td);
        c = nk == NV_WAR || nk == NV_SUB || nk == NV_CARRIER ? 0 : nk == NV_FISH ? 1 : nk == NV_TRANSPORT ? 2 : kill_kind(td) == K_CIVIL ? 3 : 5;
    }
    int s4 = fleet_builds((unsigned)k), nf = s4 ? TF_N : 1;
    struct fleet *fs = s4 ? g_tf[k] : &g_fl[k];
    for (int t = 0; t < nf; t++)
        for (int i = 0; i < fs[t].ntgt; i++)
            if (fs[t].tgt[i] == dead && fs[t].tgt_id[i] == id) { fs[t].kills[c]++; if (s4) { g_fl[k].kills[c]++; if (!c) fl_ring(&g_fl[k], 1); } return; }
}

// one of the AI's own ships sunk (diplo.c hook_kill_stat, v the victim's index): counted for its force and the task
// that force had (stage 4) - where the fleet loses its ships
internal void fleet_lost(int v, ee_unit *dead) {
    if (v <= 0 || v >= 16 || !dead || !fleet_builds((unsigned)v)) return;
    int id = unit_get_id(dead);
    for (int j = 0; j < g_ntfm[v]; j++)
        if (g_tfm[v][j].u == dead && g_tfm[v][j].id == id) {
            struct fleet *f = &g_tf[v][(int)g_tfm[v][j].tf];
            f->lost++; g_fl[v].lost++; fl_ring(&g_fl[v], 0);
            if (f->task >= 0 && f->task < FT_N) { f->lost_task[f->task]++; g_fl[v].lost_task[f->task]++; }
            return;
        }
}

// THE HAVEN (stage 4): open water by an own tower near the sea - the one farthest from the nearest known enemy building -
// else by the dock farthest from it; out of danger. The enemy (and a human) keeps out of the towers' reach.
internal void fl_haven(struct fleet *m) {
    float bd = -1;
    for (int pass = 0; pass < 2 && bd < 0; pass++) {
        int n = pass ? g_fs.nd : g_fs.ntw;
        for (int k = 0; k < n; k++) {
            ivec2 b = pass ? g_fs.dk[k] : g_fs.tw[k], w, o;
            if ((int)ocean_near(b, pass ? 4 : 6, &w) != m->ocean || !fl_water(w, (unsigned)m->ocean, &o) || fl_in_danger(o)) continue;
            float d = g_fs.eb_known ? (float)ivec2_dist2(b, g_fs.eb) : 0.0f;
            if (d > bd) { bd = d; m->hv = o; }
        }
    }
    if (bd < 0) m->hv = m->g;
}

// MASSING (stage 4, [all] navy_fleet_mass, default 1; docs/navy.md stage 5). The playtester: "massing a fleet under the
// protection of towers and land units is an important step". Against the sea opponent the stage-4 ships came one at
// a time into a sea the enemy held (0.3-0.8 afloat on average, 8-20 sunk a game, most at the guard point). While the
// fleet is weaker than the enemy fleet known (ratio < 1.0) or below its critical mass (a quarter of the fleet's target,
// 4 at least; ratio and size with a margin: under 3/4 of it), every ship waits at the rally - and the mod keeps
// training: the rally is where it grows. It strikes as one force at ratio 1.25 with the critical mass; then the
// stage-4 decision (one fleet, task forces only when clearly superior).
// THE RALLY: the water tile of its sea inside the reach of the most own towers (range - 1, so the towers really shoot
// there), then by a dock, far from the nearest known enemy building, near its tower; out of danger. The sea
// opponent keeps its targets 2 tiles clear of a tower's range + 2, so ships there are not hunted; an enemy that comes
// in meets the towers and the fleet together - the fleet fights there (ratio 0.5 with the towers covering, 0.8
// without) and never chases out of the cover (the leash: radius + 3). No tower covers its sea: the haven (by the dock
// farthest from the enemy) - and a tower is asked for there (fl_rally_tower).
internal void fl_rally(struct fleet *m, uint32_t now) {
    if (m->rally_t && now >= m->rally_t && now - m->rally_t < 20000 && (m->rally.x || m->rally.y) && !fl_in_danger(m->rally)) return;
    m->rally_t = now ? now : 1;
    float best = -1e9f;
    int bc = 0, br = 0;
    for (int k = 0; k < g_fs.ntw; k++) {
        int R = (int)g_fs.twr[k] - 1;
        if (R < 2) continue;
        if (R > 10) R = 10;
        for (int dx = -R; dx <= R; dx++)
            for (int dy = -R; dy <= R; dy++) {
                ivec2 q = ivec2_add(g_fs.tw[k], iv2(dx, dy));
                int cov = 0, dock = 0, cr = 0;
                if (dx * dx + dy * dy > R * R || (int)area_id(q, 1) != m->ocean || fl_in_danger(q)) continue;
                for (int j = 0; j < g_fs.ntw; j++) {
                    float rr = g_fs.twr[j] - 1.0f;
                    if ((float)ivec2_dist2(q, g_fs.tw[j]) <= rr * rr) { cov++; if ((int)g_fs.twr[j] > cr) cr = (int)g_fs.twr[j]; }
                }
                for (int j = 0; j < g_fs.nd && !dock; j++) dock = ivec2_dist2(g_fs.dk[j], q) <= 64;
                float away = g_fs.eb_known ? sqrtf((float)ivec2_dist2(q, g_fs.eb)) : 0.0f;
                float sc = 1000.0f * (float)cov + 200.0f * (float)dock + 2.0f * away - 10.0f * sqrtf((float)(dx * dx + dy * dy));
                if (sc > best) { best = sc; m->rally = q; bc = cov; br = cr; }
            }
    }
    m->rcover = bc;
    m->rcr = bc ? br + 2 : 8;
    if (!bc) m->rally = m->hv;
}
// no tower covers the fleet's sea: one asked for by the dock farthest from the enemy (the haven's) - a land tile
// within 5 tiles of it with the sea within 3, placeable - through the build manager's queue with its position (as
// defence.c queues its towers: 0x468C37 on the epoch's queue), the best tower type it can build and pay now
// (0x555710); every 2 minutes while none is queued, 3 a game at most
internal void fl_rally_tower(ee_player *pl, unsigned idx, struct fleet *m, uint32_t now) {
    if (m->rcover || m->rally_towers >= 3 || (m->tower_t && now >= m->tower_t && now - m->tower_t < 120000) || !g_fs.nd) return;
    m->tower_t = now ? now : 1;
    void *cm = player_get_civ_mgr(pl);
    ee_build_mgr *bm = cm ? civ_mgr_get_build_mgr(cm) : NULL;
    if (!bm) return;
    if (tower_queued(bm)) return;
    int ttype = tower_newest(pl);
    int e = current_epoch(pl);
    if (!ttype || e < 0 || e >= 14) return;
    int bk = 0;
    float bd = -1;
    for (int k = 0; k < g_fs.nd; k++) {
        float d = g_fs.eb_known ? (float)ivec2_dist2(g_fs.dk[k], g_fs.eb) : 0.0f;
        if (d > bd) { bd = d; bk = k; }
    }
    ivec2 dk = g_fs.dk[bk];
    for (int r = 2; r <= 5; r++)
        for (int ax = -r; ax <= r; ax++)
            for (int ay = -r; ay <= r; ay++) {
                ivec2 q = ivec2_add(dk, iv2(ax, ay));
                if ((abs(ax) != r && abs(ay) != r) || !fl_coast(q, m->ocean) || raiders_near(pl, q, 10) || !can_place(pl, ttype, q)) continue;
                int entry[3] = { ttype, q.x, q.y };
                build_mgr_queue_add(bm, e, entry);
                m->rally_towers++;
                if (g_log) fprintf(g_log, "# navy: P%u at %.1f min: no tower covers the fleet's sea - tower %d queued at %d,%d by the dock %d,%d\n",
                                   idx, now / 60000.0, ttype, q.x, q.y, dk.x, dk.y);
                return;
            }
}

// THE HOME THREATS: enemy warships seen within 15 tiles of the guard point, its buildings or fishing boats
internal unsigned char g_home[FL_ES];
internal int g_nh; internal ivec2 g_hsum;   // their count, their positions' sum
internal void fl_home_threats(struct fleet *m) {
    g_nh = 0; g_hsum = iv2(0, 0);
    memset(g_home, 0, sizeof g_home);
    for (int j = 0; j < g_fs.ne; j++) {
        struct fl_ship *e = &g_fs.es[j];
        if (!e->seen) continue;
        int in = ivec2_dist2(e->p, m->g) <= 225;
        for (int k = 0; k < g_fs.nb && !in; k++) in = ivec2_dist2(unit_get_tile(g_fs.boat[k]), e->p) <= 225;
        for (int k = 0; k < g_fs.nbl && !in; k++) in = ivec2_dist2(g_fs.b[k], e->p) <= 225;
        if (in) { g_home[j] = 1; g_nh++; g_hsum = ivec2_add(g_hsum, e->p); }
    }
}
// the strength against every enemy ship known and against those seen within 12 tiles of c
internal void fl_strength(struct fleet *f, ivec2 c) {
    static unsigned char sel[FL_ES];
    f->eknown = g_fs.ne;
    f->ratio = fl_ratio(f, iv2(0, 0), 0, NULL);
    memset(sel, 0, sizeof sel); f->enear = 0;
    for (int j = 0; j < g_fs.ne; j++) {
        struct fl_ship *e = &g_fs.es[j];
        if (e->seen && ivec2_dist2(e->p, c) <= 144) { sel[j] = 1; f->enear++; }
    }
    f->near_ratio = fl_ratio(f, c, 12, sel);
}

// THE FLEET'S DECISION (stage 3: the whole fleet; stage 4: the home guard - with FLD_HOME no hunt and no landing,
// the hunters and the escort do that - or the whole fleet while it is 3 ships or fewer). Returns: the task changed.
#define FLD_HOME 1
#define FLD_SHELTER 2   // stage 4: the whole fleet at the haven (fleet_tick: much weaker than the enemy fleet known)
#define FLD_MASS 4      // stage 4, navy_fleet_mass: the whole fleet masses at the rally (fleet_tick)
internal int fl_decide(unsigned idx, struct fleet *f, uint32_t now, int flags) {
    ivec2 c = fl_center(f), t = f->g;
    fl_strength(f, c);
    float hp = fl_hp(f), hmax = 0;
    for (int i = 0; i < f->n; i++) hmax += fl_hp_max(f->u[i]);
    int task = FT_GUARD, prev = f->task;
    const char *why = "nothing to do";
    int r = 10, prey = 0;
    unsigned char *use = NULL;
    int nh = g_nh;
    struct landing *l = &g_ld[idx];
    if ((flags & FLD_MASS) && f->n) {   // massing at the rally: fight there only, with the towers (fleet_tick)
        static unsigned char cov[FL_ES];
        int R = f->rcr > 0 ? f->rcr : 8, nc = 0;
        memset(cov, 0, sizeof cov);
        for (int j = 0; j < g_fs.ne; j++) {
            struct fl_ship *e = &g_fs.es[j];
            if (e->seen && ivec2_dist2(e->p, f->rally) <= R * R) { cov[j] = 1; nc++; }
        }
        task = FT_MASS; t = f->rally; r = R;
        if (nc && fl_ratio(f, iv2(0, 0), 0, cov) >= (f->rcover ? 0.5f : 0.8f)) {
            task = FT_DEFEND; why = f->rcover ? "enemy ships at the rally - with the towers" : "enemy ships at the rally"; use = cov;
            if (f->task != FT_DEFEND) f->defends++;
        } else why = nc ? "massing - the enemy at the rally too strong" : f->rcover ? "massing under the towers" : "massing by the dock";
        int changed = task != prev;
        fl_set(f, task, why, now);
        f->go = t;
        g_fl_leash = t; g_fl_lr = R + 3;
        fl_orders(f, use, t, r, 0, 0);
        g_fl_lr = 0;
        return changed;
    }
    if (!f->n) { task = FT_GUARD; why = "no warships"; }
    else if (!(flags & FLD_HOME) && g_landing && l->phase >= 2 && l->phase != 5 && l->ocean == f->ocean) {   // the landing's escorts are its own: the rest off its beach
        task = FT_SUPPORT; why = "landing under way";
        fl_safe_toward(c, l->water, (unsigned)f->ocean, &t);
        r = 12;
    } else if (nh && fl_ratio(f, iv2(0, 0), 0, g_home) >= 0.8f) {
        task = FT_DEFEND; why = "enemy ships at home"; use = g_home; t = iv2(g_hsum.x / nh, g_hsum.y / nh); r = 20;
        if (f->task != FT_DEFEND) f->defends++;
    } else if (nh) {
        task = FT_GUARD; why = "enemy ships at home - too strong";
        if (g_fl_s4 && (f->hv.x || f->hv.y)) { t = f->hv; r = 8; why = "enemy ships at home - too strong: to the haven"; }   // stage 4: under its towers
    }
    if ((flags & FLD_SHELTER) && task == FT_GUARD) { t = f->hv; r = 8; if (!nh) why = "sheltering - the enemy fleet much stronger"; }
    else if (!(flags & FLD_HOME) && g_landing && l->phase == 1 && l->ocean == f->ocean) {   // a landing getting its transports: its sea route cleared
        static unsigned char route[FL_ES];
        ivec2 lw = l->water, mid = iv2((f->g.x + lw.x) / 2, (f->g.y + lw.y) / 2);   // the landing's water, half way there
        int nr = 0;
        memset(route, 0, sizeof route);
        for (int j = 0; j < g_fs.ne; j++) {
            struct fl_ship *e = &g_fs.es[j];
            if (ivec2_dist2(e->p, lw) <= 225 || ivec2_dist2(e->p, mid) <= 225) { route[j] = 1; nr++; }
        }
        if (nr && fl_ratio(f, iv2(0, 0), 0, route) >= 1.0f) {
            task = FT_ESCORT; why = "clearing the landing's sea"; use = route; r = 16;
            fl_safe_toward(f->g, lw, (unsigned)f->ocean, &t);
            if (f->task != FT_ESCORT) f->escorts++;
        }
    }
    if (task == FT_GUARD && f->n && !nh && !(flags & FLD_HOME)) {
        // retreat: a hunt that went wrong - 60 s at the guard point at least
        if (f->task == FT_HUNT && (hp < 0.5f * f->hp0 || (f->enear && f->near_ratio < 0.8f))) {
            f->retreats++; f->retreat_t = now;
            fl_set(f, FT_RETREAT, hp < 0.5f * f->hp0 ? "half the hit points lost" : "outmatched near", now);
        }
        if (f->task == FT_RETREAT && now - f->retreat_t < 60000) { task = FT_RETREAT; why = f->why; }
        else if (f->ratio >= g_fl_raid / 100.0f && hp >= 0.6f * hmax) {
            // the hunt's target: the enemy's warships (nearest to the fleet; out of danger, reachable on its sea) when
            // clearly stronger (1.3), else - not weaker (navy_fleet_raid, 1.0) - its fishing boats and transports, its
            // coast, the water off its nearest known dock
            int best = -1, bd = 0x7FFFFFFF, prey_i = -1, pd = 0x7FFFFFFF;
            for (int j = 0; j < g_fs.ne && f->ratio >= 1.3f; j++) {
                struct fl_ship *e = &g_fs.es[j];
                int d = ivec2_dist2(e->p, c);
                if (d < bd && (int)area_id(e->p, 1) == f->ocean && !fl_in_danger_m(e->p, 3.0f)) { bd = d; best = j; }
            }
            for (int j = 0; j < g_fs.nev && best < 0; j++) {
                struct fl_ship *e = &g_fs.ev[j];
                int d = ivec2_dist2(e->p, c);
                if (d < pd && (int)area_id(e->p, 1) == f->ocean && !fl_in_danger_m(e->p, 3.0f)) { pd = d; prey_i = j; }
            }
            int coast_i = -1, cd = 0x7FFFFFFF;
            ivec2 h = iv2(-1, -1);   // the hunt's point (x -1: none)
            for (int j = 0; j < g_fs.nec && best < 0 && prey_i < 0 && g_fl_coast; j++) {   // the coast out of danger (+3 tiles)
                struct fl_ship *e = &g_fs.ec[j];
                int d = ivec2_dist2(e->p, c);
                if (d < cd && !fl_in_danger_m(e->p, 3.0f)) { cd = d; coast_i = j; }
            }
            if (best >= 0) { h = g_fs.es[best].p; why = "the enemy fleet"; }
            else if (prey_i >= 0) { h = g_fs.ev[prey_i].p; why = "fishing boats, transports"; }
            else if (coast_i >= 0 && fl_safe_toward(c, g_fs.ec[coast_i].p, (unsigned)f->ocean, &h)) why = "the enemy's coast";
            else {
                for (int k = 0; k < g_fs.ned; k++) {   // the enemy's fishing grounds: off its nearest dock, not cleared lately
                    int skip = 0, d = ivec2_dist2(g_fs.ed[k], c);
                    ivec2 w;
                    if (d >= bd) continue;
                    for (int q = 0; q < 8; q++) skip |= now < f->cleared[q].until && abs(f->cleared[q].p.x - g_fs.ed[k].x) < 6 && abs(f->cleared[q].p.y - g_fs.ed[k].y) < 6;
                    if (skip || !fl_safe_toward(c, g_fs.ed[k], (unsigned)f->ocean, &w)) continue;
                    bd = d; h = w; f->hunt = g_fs.ed[k];
                }
                if (h.x >= 0) why = "the enemy's fishing grounds";
            }
            if (h.x < 0 && (f->task == FT_HUNT || f->task == FT_GATHER) && now - f->task_t < 30000) { h = f->go; why = f->task == FT_HUNT ? f->why : "the last target"; }
            if (h.x >= 0) {
                // gathered first: every ship within 8 tiles of the middle (60 s at most)
                int spread = 0;
                for (int i = 0; i < f->n; i++) spread |= ivec2_dist2(unit_get_tile(f->u[i]), c) > 64;
                if (f->task != FT_HUNT && spread && !(f->task == FT_GATHER && now - f->task_t >= 60000)) { task = FT_GATHER; why = "before the hunt"; t = c; if (!fl_safe_toward(f->g, c, (unsigned)f->ocean, &t)) t = f->g; }
                else {
                    task = FT_HUNT; t = h; r = 14; prey = 1;
                    if (f->task != FT_HUNT) { f->hunts++; f->hp0 = hp; }
                    if (!strcmp(why, "the enemy's fishing grounds") && abs(c.x - h.x) < 5 && abs(c.y - h.y) < 5) {   // there, nothing found
                        f->cleared[f->ci].p = f->hunt; f->cleared[f->ci].until = now + 180000; f->ci = (f->ci + 1) & 7;
                    }
                }
            }
        }
    }
    if (task == FT_RETREAT) { t = f->g; if (cover_sea(idx)) fl_cover_point(idx, &t); }
    int changed = task != prev;
    fl_set(f, task, why, now);
    f->go = t;
    fl_orders(f, use, t, r, prey, task == FT_RETREAT);
    return changed;
}

// the enemy ships known (seen or remembered) within r of (x, y): sel, their count
internal int fl_local(ivec2 p, int r, unsigned char *sel) {
    int n = 0;
    memset(sel, 0, FL_ES);
    for (int j = 0; j < g_fs.ne; j++) {
        struct fl_ship *e = &g_fs.es[j];
        if (ivec2_dist2(e->p, p) <= r * r) { sel[j] = 1; n++; }
    }
    return n;
}
// two forces together (the home guard and the hunters at home)
internal float fl_ratio2(struct fleet *a, struct fleet *b, const unsigned char *sel) {
    static struct fleet t;
    t.n = 0;
    for (int i = 0; i < a->n && t.n < FL_MAX; i++) t.u[t.n++] = a->u[i];
    for (int i = 0; i < b->n && t.n < FL_MAX; i++) t.u[t.n++] = b->u[i];
    return fl_ratio(&t, iv2(0, 0), 0, sel);
}
// the hunters' target: the enemy warship group (within 12 tiles of one) where they are strongest - 1.2 or more - near
// ones first ("strike before they mass"); -1: warships known, all too strong for this force; 0: none known
internal int tf_target_fleet(struct fleet *f, ivec2 c, ivec2 *t, const char **why) {
    static unsigned char loc[FL_ES], done[FL_ES];
    memset(done, 0, sizeof done);
    float bs = 0;
    int bj = -1, any = 0;
    for (int j = 0; j < g_fs.ne; j++) {
        struct fl_ship *e = &g_fs.es[j];
        if (done[j] || (int)area_id(e->p, 1) != f->ocean || fl_in_danger_m(e->p, 3.0f)) continue;
        any++;
        fl_local(e->p, 12, loc);
        for (int k = 0; k < g_fs.ne; k++) done[k] |= loc[k];
        float rr = fl_ratio(f, iv2(0, 0), 0, loc);
        if (rr < 1.2f) continue;
        float d = sqrtf((float)ivec2_dist2(e->p, c)), s = (rr > 3.0f ? 3.0f : rr) / (1.0f + d / 20.0f);
        if (s > bs) { bs = s; bj = j; }
    }
    if (bj >= 0) { *t = g_fs.es[bj].p; *why = "enemy warships"; return 1; }
    return any ? -1 : 0;
}
// the raiders' target: the enemy's economy on the water and the coast - transports 4, fishing boats 3, citizens 3,
// docks 2, other buildings 1, soldiers 0.5 - over (1 + distance / 25), out of danger (+3 tiles), where the enemy
// warships within 12 tiles are not stronger (ratio 1)
internal int tf_target_prey(struct fleet *f, ivec2 c, ivec2 *tp, const char **why) {
    static unsigned char loc[FL_ES];
    float bs = 0;
    int found = 0;
    for (int pass = 0; pass < 2; pass++) {
        struct fl_ship *list = pass ? g_fs.ec : g_fs.ev;
        int n = pass ? g_fs.nec : g_fs.nev;
        for (int j = 0; j < n; j++) {
            struct fl_ship *e = &list[j];
            int t = unit_type_get_id(e->td), fam = unit_type_get_family(e->td);
            float w;
            const char *wy;
            if (!pass) { int k = navy_kind(e->td); w = k == NV_TRANSPORT ? 4.0f : 3.0f; wy = k == NV_TRANSPORT ? "transports" : "fishing boats"; }
            else if (is_citizen_type(t)) { w = 3.0f; wy = "citizens on the coast"; }
            else if (t == UT(B_DOCK)) { w = 2.0f; wy = "docks"; }
            else if (fam == 2 || fam == 29) { w = 1.0f; wy = "buildings on the coast"; }
            else { w = 0.5f; wy = "the coast"; }
            if (pass && (!e->seen || !g_fl_coast)) continue;
            if (!pass && (int)area_id(e->p, 1) != f->ocean) continue;
            if (fl_in_danger_m(e->p, 3.0f)) continue;
            float d = sqrtf((float)ivec2_dist2(e->p, c)), s = w / (1.0f + d / 25.0f);
            if (s <= bs) continue;
            if (fl_local(e->p, 12, loc) && fl_ratio(f, iv2(0, 0), 0, loc) < 1.0f) continue;
            ivec2 at = e->p;
            if (pass && !fl_safe_toward(c, e->p, (unsigned)f->ocean, &at)) continue;
            bs = s; *tp = at; *why = wy; found = 1;
        }
    }
    return found;
}
// no target: the enemy's fishing grounds (off its known docks, not cleared in the last 3 minutes), else its nearest
// known building's shore, else a sweep round the guard point (45 degrees on every 90 s or once there)
internal int tf_scout(struct fleet *f, ivec2 c, uint32_t now, ivec2 *t, const char **why) {
    int bd = 0x7FFFFFFF, found = 0;
    for (int k = 0; k < g_fs.ned; k++) {
        int skip = 0, d = ivec2_dist2(g_fs.ed[k], c);
        ivec2 w;
        if (d >= bd) continue;
        for (int q = 0; q < 8; q++) skip |= now < f->cleared[q].until && abs(f->cleared[q].p.x - g_fs.ed[k].x) < 6 && abs(f->cleared[q].p.y - g_fs.ed[k].y) < 6;
        if (skip || !fl_safe_toward(c, g_fs.ed[k], (unsigned)f->ocean, &w)) continue;
        if (abs(c.x - w.x) < 5 && abs(c.y - w.y) < 5) {   // there, nothing found: cleared for 3 minutes
            f->cleared[f->ci].p = g_fs.ed[k]; f->cleared[f->ci].until = now + 180000; f->ci = (f->ci + 1) & 7;
            continue;
        }
        bd = d; *t = w; found = 1;
    }
    if (found) { *why = "the enemy's fishing grounds"; return 1; }
    if (now - f->wp_t >= 90000) { f->wp++; f->wp_t = now; }
    for (int tries = 0; tries < 8; tries++) {
        ivec2 q;
        if (g_fs.eb_known && !(f->wp & 1)) { q = g_fs.eb; *why = "the enemy's shore"; }
        else {
            float a = 0.785f * (float)(f->wp & 7);
            q = iv2(f->g.x + (int)(28.0f * cosf(a)), f->g.y + (int)(28.0f * sinf(a))); *why = "scouting";
        }
        if (fl_safe_toward(f->g, q, (unsigned)f->ocean, t) && (abs(c.x - t->x) >= 5 || abs(c.y - t->y) >= 5)) return 1;
        f->wp++; f->wp_t = now;
    }
    return 0;
}
// the patrol: contacts on the way first - transports seen within 16 tiles, then warships there when not weaker (1.0;
// -1: weaker, back off) - else the sea lane's waypoints between the guard point and the nearest known enemy building
// (35% and 65% of the way, and 10 tiles to each side half way), or round the guard point when none is known
internal int tf_target_patrol(struct fleet *f, ivec2 c, uint32_t now, ivec2 *t, const char **why, unsigned char **use) {
    static unsigned char loc[FL_ES];
    int bd = 257;
    for (int j = 0; j < g_fs.nev; j++) {
        struct fl_ship *e = &g_fs.ev[j];
        int d = ivec2_dist2(e->p, c);
        if (!e->seen || d >= bd || navy_kind(e->td) != NV_TRANSPORT || fl_in_danger_m(e->p, 3.0f)) continue;
        bd = d; *t = e->p; *why = "enemy transports";
    }
    if (bd < 257) return 1;
    int n = 0;
    memset(loc, 0, sizeof loc);
    for (int j = 0; j < g_fs.ne; j++) {
        struct fl_ship *e = &g_fs.es[j];
        if (e->seen && ivec2_dist2(e->p, c) <= 256 && !fl_in_danger_m(e->p, 3.0f)) { loc[j] = 1; n++; }
    }
    if (n) {
        if (fl_ratio(f, iv2(0, 0), 0, loc) < 1.0f) return -1;
        *use = loc; *t = c; *why = "enemy warships on the lane";
        return 1;
    }
    ivec2 q;
    if (now - f->wp_t >= 80000) { f->wp++; f->wp_t = now; }
    for (int tries = 0; tries < 4; tries++) {
        int k = f->wp & 3;
        if (g_fs.eb_known) {
            float dx = (float)(g_fs.eb.x - f->g.x), dy = (float)(g_fs.eb.y - f->g.y), len = sqrtf(dx * dx + dy * dy);
            if (len < 1) len = 1;
            float a = k == 0 ? 0.35f : k == 2 ? 0.65f : 0.5f, side = k == 1 ? 10.0f : k == 3 ? -10.0f : 0.0f;
            q = iv2(f->g.x + (int)(dx * a - dy / len * side), f->g.y + (int)(dy * a + dx / len * side));
        } else {
            float a = 1.571f * (float)k;
            q = iv2(f->g.x + (int)(15.0f * cosf(a)), f->g.y + (int)(15.0f * sinf(a)));
        }
        if (fl_safe_toward(f->g, q, (unsigned)f->ocean, t) && (abs(c.x - t->x) >= 4 || abs(c.y - t->y) >= 4)) { *why = "the sea lanes"; return 2; }
        f->wp++; f->wp_t = now;
    }
    return 0;
}
// a force's decision (stage 4; the home guard: fl_decide). Returns: the task changed.
internal int tf_decide(unsigned idx, int kind, struct fleet *f, struct fleet *home, uint32_t now) {
    ivec2 c = fl_center(f), t = f->g;
    fl_strength(f, c);
    float hp = fl_hp(f), hmax = 0;
    for (int i = 0; i < f->n; i++) hmax += fl_hp_max(f->u[i]);
    int task = FT_GUARD, prev = f->task, r = 12, prey = 0;
    int engaged = prev == FT_HUNT || prev == FT_RAID || prev == FT_PATROL || prev == FT_SCOUT;
    const char *why = "nothing to do";
    unsigned char *use = NULL;
    struct landing *l = &g_ld[idx];
    if (!f->n) { fl_set(f, FT_GUARD, "no ships", now); return 0; }
    if (kind == TF_ESCORT) {
        if (g_landing && l->phase >= 2 && l->phase != 5 && l->ocean == f->ocean) {   // the landing's own escorts sail with it: this force off its beach
            task = FT_SUPPORT; why = "landing under way";
            fl_safe_toward(c, l->water, (unsigned)f->ocean, &t);
        } else if (g_landing && l->phase == 1 && l->ocean == f->ocean) {   // getting its transports: the route cleared, else covered
            static unsigned char route[FL_ES];
            ivec2 lw = l->water, mid = iv2((f->g.x + lw.x) / 2, (f->g.y + lw.y) / 2);   // the landing's water, half way there
            int nr = 0;
            memset(route, 0, sizeof route);
            for (int j = 0; j < g_fs.ne; j++) {
                struct fl_ship *e = &g_fs.es[j];
                if (ivec2_dist2(e->p, lw) <= 225 || ivec2_dist2(e->p, mid) <= 225) { route[j] = 1; nr++; }
            }
            task = FT_ESCORT; r = 16;
            if (nr && fl_ratio(f, iv2(0, 0), 0, route) >= 1.0f) {
                why = "clearing the landing's sea"; use = route;
                fl_safe_toward(f->g, lw, (unsigned)f->ocean, &t);
            } else {
                why = nr ? "covering the landing's route - enemy too strong" : "covering the landing's route";
                fl_safe_toward(f->g, mid, (unsigned)f->ocean, &t);
            }
            if (prev != FT_ESCORT) f->escorts++;
        }
    } else if (kind != TF_RAID && g_help && fl_ratio2(home, f, g_home) >= 0.8f) {   // the home guard can't hold alone
        task = FT_DEFEND; why = "helping the home guard"; use = g_home; t = iv2(g_hsum.x / g_nh, g_hsum.y / g_nh); r = 20;
        if (prev != FT_DEFEND) f->defends++;
    } else if (engaged && (hp < 0.5f * f->hp0 || (f->enear && f->near_ratio < 0.8f))) {
        f->retreats++; f->retreat_t = now;
        task = FT_RETREAT; why = hp < 0.5f * f->hp0 ? "half the hit points lost" : "outmatched near";
    } else if (prev == FT_RETREAT && now - f->retreat_t < 45000) { task = FT_RETREAT; why = f->why; }
    else if (hp < 0.5f * hmax) { task = FT_GUARD; why = "damaged - repairs"; if (cover_sea(idx)) fl_cover_point(idx, &t); }
    else {
        int found = kind == TF_HUNT ? tf_target_fleet(f, c, &t, &why) : kind == TF_RAID ? tf_target_prey(f, c, &t, &why) :
                    tf_target_patrol(f, c, now, &t, &why, &use);
        if (found < 0) { task = FT_GUARD; why = kind == TF_HUNT ? "the enemy fleet stronger everywhere" : "outmatched on the lane"; t = f->g; }
        else {
            int eng = kind == TF_HUNT ? FT_HUNT : kind == TF_RAID ? FT_RAID : FT_PATROL;
            if (!found && kind != TF_PATROL) { found = tf_scout(f, c, now, &t, &why); eng = FT_SCOUT; }
            else if (found == 2) eng = FT_PATROL;
            else if (kind == TF_PATROL) eng = FT_HUNT;
            if (found) {
                int spread = 0;
                for (int i = 0; i < f->n; i++) spread |= ivec2_dist2(unit_get_tile(f->u[i]), c) > 64;
                if (!engaged && kind != TF_PATROL && spread && !(prev == FT_GATHER && now - f->task_t >= 45000)) {   // gathered first (45 s at most)
                    task = FT_GATHER; why = "gathering";
                    if (!fl_safe_toward(f->g, c, (unsigned)f->ocean, &t)) t = f->g;
                } else {
                    task = eng; r = 14; prey = 1;
                    if (!engaged) { f->hunts++; f->hp0 = hp; }
                }
            }
        }
    }
    if (task == FT_RETREAT) { t = f->g; if (cover_sea(idx)) fl_cover_point(idx, &t); }
    int changed = task != prev || (f->why && strcmp(f->why, why));
    fl_set(f, task, why, now);
    f->go = t;
    fl_orders(f, use, t, r, prey, task == FT_RETREAT);
    return changed;
}
// the forces' sizes by the fleet's: 3 or fewer stay one fleet (the home guard decides as stage 3 did); else an
// escort of a quarter while a landing gets ready or is under way, of the rest a sixth at home (1+), a fifth raiding
// (1+), a sixth on patrol (from 6 ships), the others hunt - 20 ships: home 3, raid 4, patrol 3, hunt 10
internal void tf_sizes(int n, int land, int want[TF_N]) {
    for (int t = 0; t < TF_N; t++) want[t] = 0;
    if (n <= 3) { want[TF_HOME] = n; return; }
    int e = land ? (n / 4 > 1 ? n / 4 : 1) : 0, rest = n - e;
    int h = rest / 6 > 1 ? rest / 6 : 1, rd = rest / 5 > 1 ? rest / 5 : 1, p = rest >= 6 ? (rest / 6 > 1 ? rest / 6 : 1) : 0;
    want[TF_ESCORT] = e; want[TF_HOME] = h; want[TF_RAID] = rd; want[TF_PATROL] = p; want[TF_HUNT] = rest - h - rd - p;
}
internal int tf_fit(int t, ee_unit *u) { return k_tf_fit[t][nv_role(unit_get_type_data(u))]; }
// each ship stays in its force; a force over its size (one more is kept, so no ship goes back and forth) lets the
// least fitting go, a force under it takes the best fitting free ship (else one from a force over its size); the rest
// hunt
internal void tf_assign(unsigned idx, struct fleet *m, const int want[TF_N]) {
    static signed char of[FL_MAX];
    static const int order[TF_N] = { TF_ESCORT, TF_HOME, TF_HUNT, TF_RAID, TF_PATROL };
    int cnt[TF_N] = { 0 };
    for (int i = 0; i < m->n; i++) {
        ee_unit *u = m->u[i];
        int id = unit_get_id(u);
        of[i] = -1;
        for (int j = 0; j < g_ntfm[idx]; j++) if (g_tfm[idx][j].u == u && g_tfm[idx][j].id == id) { of[i] = g_tfm[idx][j].tf; break; }
        if (of[i] >= 0) cnt[(int)of[i]]++;
    }
    for (int t = 0; t < TF_N; t++)
        while (cnt[t] > want[t] + (want[t] ? 1 : 0)) {
            int w = -1;
            for (int i = 0; i < m->n; i++) if (of[i] == t && (w < 0 || tf_fit(t, m->u[i]) < tf_fit(t, m->u[w]))) w = i;
            if (w < 0) break;
            of[w] = -1; cnt[t]--;
        }
    for (int o = 0; o < TF_N; o++) {
        int t = order[o];
        while (cnt[t] < want[t]) {
            int b = -1;
            for (int i = 0; i < m->n; i++) if (of[i] < 0 && (b < 0 || tf_fit(t, m->u[i]) > tf_fit(t, m->u[b]))) b = i;
            if (b < 0)
                for (int i = 0; i < m->n; i++)
                    if (of[i] >= 0 && of[i] != t && cnt[(int)of[i]] > want[(int)of[i]] && (b < 0 || tf_fit(t, m->u[i]) > tf_fit(t, m->u[b]))) b = i;
            if (b < 0) break;
            if (of[b] >= 0) cnt[(int)of[b]]--;
            of[b] = (signed char)t; cnt[t]++;
        }
    }
    for (int t = 0; t < TF_N; t++) g_tf[idx][t].n = 0;
    g_ntfm[idx] = 0;
    for (int i = 0; i < m->n; i++) {
        if (of[i] < 0) of[i] = want[TF_HUNT] ? TF_HUNT : TF_HOME;
        struct fleet *f = &g_tf[idx][(int)of[i]];
        f->u[f->n++] = m->u[i];
        g_tfm[idx][g_ntfm[idx]].u = m->u[i]; g_tfm[idx][g_ntfm[idx]].id = unit_get_id(m->u[i]); g_tfm[idx][g_ntfm[idx]++].tf = of[i];
    }
}
internal void tf_log(unsigned idx, int k, struct fleet *f, uint32_t now) {
    if (!g_log) return;
    int ro[NR_N] = { 0 };
    float hp = fl_hp(f), hm = 0;
    for (int i = 0; i < f->n; i++) { ro[nv_role(unit_get_type_data(f->u[i]))]++; hm += fl_hp_max(f->u[i]); }
    ivec2 c = fl_center(f);
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# navy: P%u tf %s at %.1f min: %s (%s) | ships %d F%d L%d A%d S%d AA%d hp %.0f%% | at %d,%d -> %d,%d | ratio %.2f vs %d known, near %.2f vs %d"
                   " | kills W%u B%u T%u C%u Bd%u O%u lost %u | hunts %u retreats %u defends %u orders %u flees %u\n",
            idx, k_tf[k], now / 60000.0, k_ft[f->task], f->why ? f->why : "-", f->n, ro[NR_FRIG], ro[NR_LINE], ro[NR_ANTI], ro[NR_SUB], ro[NR_AA],
            hm > 0 ? 100.0f * hp / hm : 0.0f, c.x, c.y, f->go.x, f->go.y, f->ratio > 98 ? 99.0f : f->ratio, f->eknown, f->near_ratio > 98 ? 99.0f : f->near_ratio,
            f->enear, f->kills[0], f->kills[1], f->kills[2], f->kills[3], f->kills[4], f->kills[5], f->lost, f->hunts, f->retreats, f->defends, f->orders, f->flees);
    fflush(g_log);
    LeaveCriticalSection(&g_lock);
}
// THE FLEET'S OWN ORDERS (stage 4): every 10 s, while the warships afloat and those ordered in the last 90 s are below
// the target (navy.c g_nv_target) and the population has room, one warship of the type the mix wants (navy.c
// navy_fleet_type: the role furthest below its share, payable now) through the game's training (econ.c
// hook_train_units: paid from the navy account, cut to the room). The game's own warship orders go on as well.
internal void fl_build(ee_player *pl, unsigned idx, struct fleet *m, uint32_t now) {
    if (now >= m->build_t && now - m->build_t < 10000) return;
    m->build_t = now ? now : 1;
    int t = g_nv_target[idx], recent = 0;
    for (int q = 0; q < 8; q++) recent += m->bring_t[q] && now >= m->bring_t[q] && now - m->bring_t[q] < 90000;
    if (t <= 0 || m->shelter || g_nv_own[idx].war + recent >= t || !g_nv_own[idx].docks_built || bk_room(pl) <= 0) return;
    void *cm = player_get_civ_mgr(pl);
    ee_unit_mgr *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
    int type = um ? navy_fleet_type(pl, idx) : 0;
    if (!type) return;
    int *stock = player_stock_at(pl), before[7];
    memcpy(before, stock, sizeof before);
    int req[4] = { type, 1, 0, 0 };
    unsigned got = 0;
    hook_train_units(um, req, (uintptr_t)&got);
    navy_pick_done(pl, idx, type, type, before, stock, !got);
    if (got) { m->built++; m->bring_t[m->bring_i] = now ? now : 1; m->bring_i = (m->bring_i + 1) & 7; }
    else m->build_refused++;
    if (got && g_log) fprintf(g_log, "# navy: P%u at %.1f min: the fleet trains a warship (type %d, role %s) - %d afloat, target %d\n", idx, now / 60000.0, type,
                              k_nr[nv_role(type_data(type))], g_nv_own[idx].war, t);
}
internal void tf_tick(ee_player *pl, unsigned idx, struct fleet *m, uint32_t now) {
    struct landing *l = &g_ld[idx];
    int land = g_landing && l->phase >= 1 && l->phase != 5 && l->ocean == m->ocean, want[TF_N], ch[TF_N] = { 0 };
    tf_sizes(m->n, land, want);
    // SPLIT only when clearly the stronger at sea (ratio 1.5 against every enemy warship known, 6+ ships; together again
    // under 1.1): split forces of 1-3 ships lost to a concentrated enemy - against the weak sea opponent (8-12
    // warships) the AI lost 2-16 warships more a game than without the fleet. Else one fleet (the stage-3 decision:
    // guard, defend, hunt at 1.3, the landing), or at the haven (shelter)
    if (!m->split && m->n >= 6 && m->ratio >= 1.5f) m->split = 1;
    else if (m->split && (m->n < 6 || m->ratio < 1.1f)) m->split = 0;
    if (m->mass) m->split = 0;
    if (m->shelter || m->mass || !m->split) { for (int t = 0; t < TF_N; t++) want[t] = 0; want[TF_HOME] = m->n; }   // one fleet
    tf_assign(idx, m, want);
    g_fl_s4 = 1;
    for (int t = 0; t < TF_N; t++) {
        struct fleet *f = &g_tf[idx][t];
        f->ocean = m->ocean; f->g = m->g; f->hv = m->hv;
        f->rally = m->rally; f->rcover = m->rcover; f->rcr = m->rcr;
    }
    struct fleet *home = &g_tf[idx][TF_HOME];
    ch[TF_HOME] = fl_decide(idx, home, now, m->mass ? FLD_MASS : m->shelter ? FLD_HOME | FLD_SHELTER : m->split ? FLD_HOME : 0);
    g_help = g_nh && fl_ratio(home, iv2(0, 0), 0, g_home) < 1.0f;
    for (int t = TF_HUNT; t < TF_N; t++) ch[t] = tf_decide(idx, t, &g_tf[idx][t], home, now);
    g_fl_s4 = 0;
    for (int t = 0; t < TF_N; t++) {
        struct fleet *f = &g_tf[idx][t];
        fl_targets(f);
        if (f->n && (ch[t] || now - f->log_t >= 60000)) { f->log_t = now; tf_log(idx, t, f, now); }
    }
    fl_build(pl, idx, m, now);
}

internal void fleet_tick(ee_player *pl, uint32_t now) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct fleet *f = &g_fl[idx];
    if (now < f->last) { memset(f, 0, sizeof *f); memset(g_tf[idx], 0, sizeof g_tf[idx]); g_ntfm[idx] = 0; }   // a new game (also fleet_new_game)
    if (!fleet_on(idx)) { f->n = 0; return; }
    if (f->last && now - f->last < 2000) return;
    f->last = now ? now : 1;
    fl_scan(pl, idx);
    // its sea: the docks' (the first one with a sea next to it), else the first ship's
    if (!f->ocean)
        for (int k = 0; k < g_fs.nd && !f->ocean; k++) f->ocean = (int)ocean_near(g_fs.dk[k], 4, NULL);
    if (!f->ocean && f->n) f->ocean = fl_ship_ocean(f->u[0]);
    if (!f->ocean) return;
    fl_guard_point(f);
    if (!f->g.x && !f->g.y) return;
    // repaired at the dock: hit points regained by the ships sent there
    for (int j = 0; j < f->ndocked; j++) {
        ee_unit *u = f->docked[j];
        int k;
        for (k = 0; k < f->n && f->u[k] != u; k++) ;
        if (k == f->n) { f->docked[j] = f->docked[--f->ndocked]; f->docked_hp[j--] = f->docked_hp[f->ndocked]; continue; }
        int h = unit_get_hp(u);
        if (h > f->docked_hp[j]) { f->repaired += (unsigned)(h - f->docked_hp[j]); f->docked_hp[j] = h; }
        if (h >= 0.8f * fl_hp_max(u)) { f->docked[j] = f->docked[--f->ndocked]; f->docked_hp[j--] = f->docked_hp[f->ndocked]; }
    }
    ivec2 c = fl_center(f);
    fl_boats(f, c);
    fl_home_threats(f);
    if (fleet_builds(idx)) {   // stage 4: task forces
        // SHELTER (fleet in being): against an enemy fleet known that is much stronger (ratio < 0.6, 3+ warships) the
        // ships went out one by one and were sunk at the guard point - against the sea opponent (16 warships) the
        // AI lost 6-18 more a game than without the fleet. Then all of them wait at the haven, under its towers, and
        // grow; out again at 0.9.
        fl_haven(f);
        float rr = fl_ratio(f, iv2(0, 0), 0, NULL);
        f->ratio = rr;
        // also while it is losing at sea: 4+ warships lost in the last 10 minutes and at least twice what it sank (the
        // enemy fleet is seen only near: 3 known at once was rare against the sea opponent's 16) - then the mod trains
        // none and the account's share is the base one (fl_build, navy.c navy_share): ships sent out one by one only fed
        // the enemy's kills
        fl_ring(f, -1);
        int l10 = 0, s10 = 0;
        for (int k = 0; k < 10; k++) { l10 += f->lostm[k]; s10 += f->sunkm[k]; }
        int losing = l10 - s10 >= 4 && l10 >= 2 * s10, weak = g_fs.ne >= 3 && rr < 0.6f;
        if (g_fl_mass) {   // massing replaces the shelter: at the rally, under the towers, and growing (above)
            fl_rally(f, now);
            fl_rally_tower(pl, idx, f, now);
            int crit = g_nv_target[idx] / 4 > 4 ? g_nv_target[idx] / 4 : 4;
            if (!f->mass && !(f->n >= crit * 3 / 4 && rr >= 1.0f)) { f->mass = 1; f->masses++; }
            else if (f->mass && f->n >= crit && rr >= 1.25f) { f->mass = 0; f->strikes++; }
            f->shelter = 0;
        } else if (!f->shelter && (weak || losing) && (f->hv.x || f->hv.y)) { f->shelter = 1; f->shelters++; }
        else if (f->shelter && !losing && (rr >= 0.9f || g_fs.ne < 3)) f->shelter = 0;
        tf_tick(pl, idx, f, now);
        fl_strength(f, c);
        f->task = FT_GUARD; f->why = "task forces"; f->go = c;
        if (now - f->log_t >= 60000) { f->log_t = now; fl_log(pl, idx, f, now, c); }
        return;
    }
    int changed = fl_decide(idx, f, now, 0);
    fl_targets(f);
    if (changed || now - f->log_t >= 60000) { f->log_t = now; fl_log(pl, idx, f, now, c); }
}

internal void fleet_new_game(void) { memset(g_fl, 0, sizeof g_fl); memset(g_tf, 0, sizeof g_tf); memset(g_ntfm, 0, sizeof g_ntfm); }
internal void fleet_install(const char *autotest_ini) {
    g_navy_fleet = g_set_ai[1].navy_fleet;
    if (autotest_ini) g_v_navy_fleet = ini_int("autotest", "v_navy_fleet", -1, autotest_ini);
    g_fl_boats = g_set.navy_fleet_boats;
    g_fl_coast = g_set.navy_fleet_coast;
    g_fl_raid = g_set.navy_fleet_raid;
    g_fl_mass = g_set_ai[1].navy_fleet_mass;
    int ok = 0;
    if (g_navy_fleet || g_v_navy_fleet > 0) {
        ok = asg_order_hooks() + patch_maint_delete(hook_nv_maint_delete);
    }
    if (g_log) fprintf(g_log, "# navy: fleet %s (v_navy_fleet %d; hooks %d)\n", g_navy_fleet ? "on" : "off", g_v_navy_fleet, ok);
}
