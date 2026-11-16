// THE HOLY - priests and prophets while the game's computer player is off (the player 2026-11-16: "the ai needs to make
// use of religuess units, espacialy in the early ages they are very powerfull offensive units. they probably should get
// their own cattegory like siege. prophets are spezial strike units, they can acompany and push as siege (earthquake)
// or can strike on their own as hit and run, sneak in, spawn a desaster in unprotected territory and run away again.
// priest can be used to snatch away citecens away from the pack or lone soilders").
// Neither is one of stations.c's soldiers (a priest is of the class Land Priest, a prophet no land soldier: econ.c
// land_soldier): they are this file's alone. Every HL_EVERY for each AI:
//   TRAINING  - with a finished Temple: prophets up to HL_PROPHETS (one more every HL_PROPHET_EPOCHS epochs), priests up
//               to HL_PRIESTS, one at a time, when the bank holds the price (HL_BANK times it; nothing is saved for
//               them: the priority list knows them not).
//   PROPHETS  - a power cast where it hits most, the caster's power its cost or more (the calamities' own costs,
//               game/api goal.c calamity_cost: earthquake 50, volcano 60, fire 75, hurricane, plague, malaria 100):
//               enemy buildings (HL_BUILDINGS within HL_CLUSTER of one: earthquake, else fire, else volcano), enemy
//               soldiers (HL_SOLDIERS: plague, else hurricane), citizens (HL_CITIZENS: malaria, else plague) - what it
//               knows within HL_REACH of it; an enemy tower first (THE TOWERS), the earthquake a few tiles before it.
//               The first prophet goes with the force while it meets the enemy (stations.c sn_need_at): HL_BEHIND tiles
//               behind that place, as the siege - a push goes at the enemy's Temple first then (stations.c TEMPLE
//               FIRST). The others, power for an earthquake at least: A STRIKE - the nearest enemy building it knows
//               within HL_STRIKE with no enemy soldiers near (raiders_near), under no enemy tower (work.c wk_danger) or a
//               tower itself, is walked to with HL_ESCORT soldiers beside it (stations.c AN ESCORT), cast at, and it
//               walks home again; hurt, or the target guarded since, it turns home at once.
//   PRIESTS   - a conversion (the game's attack order: a priest's attack converts) of an enemy citizen or soldier
//               within HL_PRIEST_LOOK that stands alone (no other enemy soldier within HL_ALONE of it) and under no
//               tower, citizens first, the nearest; none near enemy soldiers that outnumber them. Idle: with the force
//               (HL_BEHIND behind where it meets the enemy), else at home. Hurt with enemies by: home.
// "# holy:" at every cast and strike, and once a minute in tests.
#define HL_EVERY 2000
#define HL_TRAIN_EVERY 20000
#define HL_PROPHETS 2           // one with the force, one on strikes (WITH THE FORCE, ON ITS OWN)
#define HL_PROPHET_EPOCHS 4
#define HL_PROPHETS_MAX 4
#define HL_PRIESTS 3
#define HL_BANK 1
#define HL_LOSSES 8
#define HL_LOSS_WINDOW 600000   // THE LOST: HL_LOSS_MAX of a kind lost within this - none of that kind trained meanwhile
#define HL_LOSS_MAX 3
#define HL_ESCORT 4          // AN ESCORT: soldiers beside a prophet on a strike (stations.c sn_escort)
#define HL_CAUGHT 4           // A PARTING GIFT: this many enemy soldiers within HL_CAUGHT_R
#define HL_CAUGHT_R 6
#define HL_HOME_R 15          // AT HOME: within this of a town of its
#define HL_DEFEND 25          // ... the enemy soldiers within this of it
#define HL_DEFEND_N 4         // ... this many in a group
#define HL_FINISH_R 10        // THE POWERS: its own soldiers and towers within this of a group ...
#define HL_FINISH_N 5         // ... this many - malaria (they finish them off)
#define HL_VOLCANO_EVERY 5    // ... buildings: a volcano first one look in this many
#define HL_SHIPS 2            // enemy ships in a group: a hurricane
#define HL_REACH 14
#define HL_TOWER_LOOK 16      // THE TOWERS: an enemy tower within this
#define HL_QUAKE_OFF 3        // ... gets the earthquake this many tiles before it
#define HL_CLUSTER 4
#define HL_BUILDINGS 2
#define HL_SOLDIERS 6
#define HL_CITIZENS 5
#define HL_BEHIND 8
#define HL_STRIKE 70
#define HL_PRIEST_LOOK 12
#define HL_ALONE 5
#define HL_HURT 0.6f
#define HL_UNITS 64
#define HL_SEEN 512
internal const int k_hl_tech[6] = { 1018, 1013, 1017, 1019, 1022, 1016 };   // dbtechtree: earthquake, fire, volcano, plague, hurricane, malaria
enum { HLP_QUAKE, HLP_FIRE, HLP_VOLCANO, HLP_PLAGUE, HLP_HURRICANE, HLP_MALARIA, HLP_N };
internal struct hl_st {
    uint32_t t, log_t, train_t;
    struct { int id; ivec2 strike; uint32_t strike_t; } p[HL_UNITS]; int np;   // prophets on a strike: its target, since
    struct { int id; uint32_t t; } ord[2 * HL_UNITS]; int nord;   // its last cast or conversion order, when (A GIVEN ORDER)
    unsigned casts[HLP_N], strikes, converts, trained_p, trained_c, towers, refused, gifts, defends;
    int last_pro, last_pri; uint32_t lost[2][HL_LOSSES]; unsigned nlost[2];   // THE LOST: the last count of each, when each was lost (a ring)
} g_hl[16];
// A GIVEN ORDER: a cast or a conversion walks first - the unit's goal is a move meanwhile (the cast goal's own walk into
// reach, EEUGLocation) - and is not given again for HL_ORDER_WAIT while the unit has a goal (the first count: 130 to 180
// "casts" an hour, every 2 s one)
#define HL_ORDER_WAIT 20000
internal int hl_ordered(struct hl_st *h, ee_unit *u, uint32_t now) {
    int id = unit_get_id(u);
    for (int k = 0; k < h->nord; k++) if (h->ord[k].id == id) return now >= h->ord[k].t && now - h->ord[k].t < HL_ORDER_WAIT && unit_get_goal(u);
    return 0;
}
internal void hl_order_note(struct hl_st *h, ee_unit *u, uint32_t now) {
    int id = unit_get_id(u), n = 0;
    for (int k = 0; k < h->nord; k++) if (h->ord[k].id != id && now >= h->ord[k].t && now - h->ord[k].t < HL_ORDER_WAIT) h->ord[n++] = h->ord[k];
    if (n < 2 * HL_UNITS) { h->ord[n].id = id; h->ord[n++].t = now ? now : 1; }
    h->nord = n;
}
internal void holy_new_game(void) { memset(g_hl, 0, sizeof g_hl); }
internal int holy_prophets(unsigned idx) { return g_hl[idx & 15].last_pro; }   // stations.c TEMPLE FIRST
internal int hl_lost(struct hl_st *h, int k, uint32_t now) {   // THE LOST: those of kind k (0 prophets, 1 priests) lost within HL_LOSS_WINDOW
    int n = 0;
    for (unsigned j = 0; j < HL_LOSSES && j < h->nlost[k]; j++) n += h->lost[k][j] && now >= h->lost[k][j] && now - h->lost[k][j] < HL_LOSS_WINDOW;
    return n;
}
// a power's cost for pl (-1: not its, or unknown)
internal int hl_cost(ee_player *pl, int k) {
    ee_tech_tree *tt = player_get_tech_tree(pl);
    ee_tech *t = tt ? tech_tree_lookup(tt, k_hl_tech[k]) : NULL;
    ee_tech_data *d = t ? tech_get_data(t) : NULL;
    return d ? calamity_cost(tech_data_get_calamity(d)) : -1;
}
internal int hl_cal(ee_player *pl, int k) {   // the power's calamity number, -1 none
    ee_tech_tree *tt = player_get_tech_tree(pl);
    ee_tech *t = tt ? tech_tree_lookup(tt, k_hl_tech[k]) : NULL;
    ee_tech_data *d = t ? tech_get_data(t) : NULL;
    return d ? tech_data_get_calamity(d) : -1;
}
// the power's own check first (goal.c calamity_check: the caster's power, the tile explored, no enemy Temple's
// protection there) - without it the first runs gave casts the game dropped at once, every 2 s one
// ON A UNIT (the player 2026-11-16: "the only desasters that need a unit target are plaque and fire, the others are
// cast on the ground"; the plague on the ground was refused by its check and crashed the game's cast code at
// 0x60E491): the plague on the enemy soldier or citizen nearest at within HL_CLUSTER, fire on the enemy building
// nearest it; the others on the ground
enum { HLK_BUILDINGS, HLK_SOLDIERS, HLK_CITIZENS, HLK_SHIPS, HLK_N };   // THE POWERS: the kinds of groups
internal int hl_kind(ee_unit_type *td);
internal ee_unit *hl_target(ee_player *pl, ivec2 at, int k) {
    if (k != HLP_PLAGUE && k != HLP_FIRE) return NULL;
    ee_unit *b = NULL;
    int bd = 0;
    for (struct grid_it it = grid_it_box(at.x, at.y, HL_CLUSTER, 1); grid_it_next(&it); ) {
        ee_player *o = unit_get_owner(it.u);
        int kd = o && is_enemy(pl, o) && unit_get_hp(it.u) > 0 && unit_get_x(it.u) >= 0 && seen_by(it.u, pl) ? hl_kind(it.td) : -1;
        if (k == HLP_FIRE ? kd != HLK_BUILDINGS : kd != HLK_SOLDIERS && kd != HLK_CITIZENS) continue;
        int d = ivec2_dist2(unit_get_tile(it.u), at);
        if (!b || d < bd) { b = it.u; bd = d; }
    }
    return b;
}
internal int hl_check(ee_player *pl, ee_unit *u, ivec2 at, int k) { return calamity_check(hl_cal(pl, k), u, hl_target(pl, at, k), at.x, at.y); }
internal int hl_cast(ee_player *pl, ee_unit *u, ivec2 at, int k) {
    ee_unit *tg = hl_target(pl, at, k);
    if (calamity_check(hl_cal(pl, k), u, tg, at.x, at.y) != 0) return 0;
    if (tg) at = unit_get_tile(tg);
    void *mem = goal_cast_alloc();
    ee_goal *g = mem ? goal_cast_init(mem, u, tg, at.x, at.y, k_hl_tech[k]) : NULL;
    return g && unit_goal(u, g, 0, 0x1F40);
}
internal int hl_walk(ee_unit *u, ivec2 to) {
    ivec2 p = unit_get_tile(u);
    if (ivec2_dist2(p, to) <= 3 * 3) return 0;
    ee_goal *g = unit_get_goal(u);
    if (g && goal_is_location(g) && fabsf(goal_get_dest_x(g) - (float)to.x) < 3.0f && fabsf(goal_get_dest_y(g) - (float)to.y) < 3.0f) return 0;   // on its way
    return as_order(u, goal_move((float)to.x + 0.5f, (float)to.y + 0.5f));
}
// HL_BEHIND tiles from front towards from
internal ivec2 hl_behind(ivec2 front, ivec2 from) {
    float dx = (float)(from.x - front.x), dy = (float)(from.y - front.y), d = sqrtf(dx * dx + dy * dy);
    if (d < 1.0f) return front;
    return iv2(front.x + (int)(dx / d * HL_BEHIND), front.y + (int)(dy / d * HL_BEHIND));
}
// THE POWERS (the player 2026-11-16: "dont just put malaria first, it would always win. malaria: dosnt kill but sets groups
// to 1 hp, good when own forces a nerby to finish them off. plaque: does spread and kill on its own, the better option
// when the prophet is alone (parting gift) or strikes citezens. huricane: demages enemy ships, good devensive option for
// habours. volcano: kinda useless but flashy, might be picked instead as an eartquaqe sometimes just because"): what a
// group gets, in order - soldiers malaria with HL_FINISH_N of its own soldiers and towers within HL_FINISH_R of them,
// else the plague; citizens the plague (malaria after it with its own near); ships a hurricane; buildings an earthquake,
// then fire, one look in HL_VOLCANO_EVERY a volcano first.
internal int hl_kind(ee_unit_type *td) {
    int f = unit_type_get_class_id(td), t = unit_type_get_id(td);
    if ((f == CLS_BUILDING || f == CLS_FORTRESS) && !is_wall(t)) return HLK_BUILDINGS;
    if (is_citizen_type(t)) return HLK_CITIZENS;
    if (f == CLS_NAVAL || f == CLS_SUBMERGED_NAVAL || f == CLS_NAVAL_TRANSPORT) return HLK_SHIPS;
    if (is_soldier(td) && f == CLS_LAND) return HLK_SOLDIERS;
    return -1;
}
internal int hl_own_near(ee_player *pl, ivec2 at, int r) {   // its soldiers and towers within r of at
    int n = 0;
    float rr, aa;
    for (struct grid_it it = grid_it_box(at.x, at.y, r, 1); grid_it_next(&it); ) {
        if (unit_get_owner(it.u) != pl || unit_get_hp(it.u) <= 0 || unit_get_x(it.u) < 0) continue;
        n += (is_soldier(it.td) && unit_type_get_class_id(it.td) == CLS_LAND) || defence_stats(it.u, &rr, &aa);
    }
    return n;
}
internal int hl_powers(ee_player *pl, int kind, ivec2 at, uint32_t now, int out[3]) {
    out[0] = out[1] = out[2] = -1;
    int own = kind == HLK_SOLDIERS || kind == HLK_CITIZENS ? hl_own_near(pl, at, HL_FINISH_R) >= HL_FINISH_N : 0;
    if (kind == HLK_BUILDINGS) {
        int v = (now / HL_EVERY) % HL_VOLCANO_EVERY == 0;
        out[0] = v ? HLP_VOLCANO : HLP_QUAKE; out[1] = v ? HLP_QUAKE : HLP_FIRE; out[2] = v ? HLP_FIRE : HLP_VOLCANO;
    } else if (kind == HLK_SOLDIERS) { out[0] = own ? HLP_MALARIA : HLP_PLAGUE; out[1] = own ? HLP_PLAGUE : -1; }
    else if (kind == HLK_CITIZENS) { out[0] = HLP_PLAGUE; out[1] = own ? HLP_MALARIA : -1; }
    else if (kind == HLK_SHIPS) out[0] = HLP_HURRICANE;
    return own;
}
internal const char *const k_hl_name[HLP_N] = { "an earthquake", "a fire", "a volcano", "the plague", "a hurricane", "malaria" };
internal void holy_tick(ee_player *pl, uint32_t now) {
    unsigned idx = pl ? (unsigned)player_get_index(pl) & 15 : 0;
    struct hl_st *h = &g_hl[idx];
    if (!pl || GAME_AI(idx)) return;
    if (now < h->t) memset(h, 0, sizeof *h);
    if (h->t && now - h->t < HL_EVERY) return;
    h->t = now ? now : 1;
    static ee_unit *pro[HL_UNITS], *pri[HL_UNITS];
    static ivec2 town[64];
    int npro = 0, npri = 0, ntown = 0, temple = 0, count = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
        int t = unit_type_get_id(td);
        if (t == UT(PROPHET) && npro < HL_UNITS) pro[npro++] = u;
        else if (t == UT(PRIEST) && npri < HL_UNITS) pri[npri++] = u;
        else if (t == UT(B_TEMPLE) || t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL) || t == UT(B_SETTLEMENT)) {
            ee_unit *st = unit_get_physics(u);
            if (!st || !unit_get_finished(st)) continue;
            if (t == UT(B_TEMPLE)) temple = 1;
            else if (ntown < 64) town[ntown++] = unit_get_tile(u);
        }
    }
    // THE LOST (the player 2026-11-16, agreeing to it: one AI trained 14 priests in an hour, their conversions walking
    // into its enemy): a fall in the count is a loss (killed or converted); HL_LOSS_MAX of a kind lost within
    // HL_LOSS_WINDOW - none of that kind trained until the oldest of them is out of it.
    for (int k = 0; k < 2; k++) {
        int now_n = k ? npri : npro, *last = k ? &h->last_pri : &h->last_pro;
        for (int j = now_n; j < *last; j++) h->lost[k][h->nlost[k]++ % HL_LOSSES] = now ? now : 1;
        *last = now_n;
    }
    // TRAINING
    if (temple && (!h->train_t || now < h->train_t || now - h->train_t >= HL_TRAIN_EVERY)) {
        h->train_t = now ? now : 1;
        int wp = HL_PROPHETS + current_epoch(pl) / HL_PROPHET_EPOCHS;
        if (wp > HL_PROPHETS_MAX) wp = HL_PROPHETS_MAX;
        int want_c = npri < HL_PRIESTS && hl_lost(h, 1, now) < HL_LOSS_MAX;
        int type = npro < wp && hl_lost(h, 0, now) < HL_LOSS_MAX ? UT(PROPHET) : want_c ? UT(PRIEST) : 0;
        if (type == UT(PROPHET) && hl_cost(pl, HLP_QUAKE) < 0) type = want_c ? UT(PRIEST) : 0;   // no powers for it
        const int *stock = player_stock_at(pl);
        for (int r = 2; type && r < 7; r++) if (stock[r] < HL_BANK * unit_cost(pl, type, r)) type = 0;
        ee_civ_mgr *cm = player_get_civ_mgr(pl);
        ee_unit_mgr *um = cm ? civ_mgr_get_unit_mgr(cm) : NULL;
        if (type && um && ut_has(type) && can_build(pl, type)) {
            int req[4] = { type, 1, 0, 0 };
            unsigned got = 0;
            g_priority_mil = 1;   // its own order: no substitute
            hook_train_units(um, req, (uintptr_t)&got);
            g_priority_mil = 0;
            if (type == UT(PROPHET)) h->trained_p += got; else h->trained_c += got;
        }
    }
    ivec2 front;
    int fight = sn_need_at(idx, &front);
    // PROPHETS
    struct { int id; ivec2 strike; uint32_t strike_t; } keep[HL_UNITS];
    int nk = 0;
    static ee_unit *seen[HL_SEEN];
    int escorted = 0;   // one prophet's strike escorted a look
    for (int i = 0; i < npro; i++) {
        ee_unit *u = pro[i];
        ivec2 p = unit_get_tile(u);
        int id = unit_get_id(u), k0 = 0;
        while (k0 < h->np && h->p[k0].id != id) k0++;
        ivec2 strike = k0 < h->np ? h->p[k0].strike : iv2(-1, -1);
        uint32_t strike_t = k0 < h->np ? h->p[k0].strike_t : 0;
        ee_goal *g = unit_get_goal(u);
        int casting = (g && goal_is_cast(g)) || hl_ordered(h, u, now);
        ivec2 home = p;
        for (int k = 0, bd = -1; k < ntown; k++) { int d = ivec2_dist2(town[k], p); if (bd < 0 || d < bd) { bd = d; home = town[k]; } }
        ee_unit_type *td = unit_get_type_data(u);
        int hurt = (float)unit_get_hp(u) < HL_HURT * (float)unit_type_get_hp(td);
        int pw = unit_get_power(u), cost[HLP_N];
        for (int k = 0; k < HLP_N; k++) cost[k] = hl_cost(pl, k);
        // A PARTING GIFT (the player 2026-11-16: "prophet revange: they often get cought by armies outside of temple range,
        // the prophet cant escape, but it could give the pest as a parting gift"): HL_CAUGHT enemy soldiers within
        // HL_CAUGHT_R of it, or hurt with any there - the plague on them, else a hurricane, else a volcano, at the middle of
        // them, whatever the clusters want; the power's own check as always.
        if (!casting) {
            int ns = 0; long sx = 0, sy = 0;
            for (struct grid_it it = grid_it_box(p.x, p.y, HL_CAUGHT_R, 1); grid_it_next(&it); ) {
                ee_player *o = unit_get_owner(it.u);
                if (!o || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || !is_soldier(it.td) || unit_type_get_class_id(it.td) != CLS_LAND || !seen_by(it.u, pl)) continue;
                ivec2 q = unit_get_tile(it.u);
                if (ivec2_dist2(q, p) > HL_CAUGHT_R * HL_CAUGHT_R) continue;
                ns++; sx += q.x; sy += q.y;
            }
            if (ns >= HL_CAUGHT || (hurt && ns > 0)) {
                ivec2 at = iv2((int)(sx / ns), (int)(sy / ns));
                int gift[3];
                hl_powers(pl, HLK_SOLDIERS, at, now, gift);   // THE POWERS: alone the plague, its own near malaria
                for (int j = 0; j < 3 && !casting; j++) {
                    int k = gift[j];
                    if (k < 0 || cost[k] <= 0 || pw < cost[k] || !hl_cast(pl, u, at, k)) continue;
                    hl_order_note(h, u, now);
                    h->casts[k]++; h->gifts++;
                    strike = iv2(-1, -1); strike_t = 0;
                    if (g_log) log_locked("# holy: P%u at %.1f min: a prophet caught at %d,%d by %d enemy soldiers (hp %d) casts %s on them at %d,%d (power %d)\n",
                                          idx, now / 60000.0, p.x, p.y, ns, unit_get_hp(u), k_hl_name[k], at.x, at.y, pw);
                    casting = 1;
                }
            }
        }
        // AT HOME (the player 2026-11-16: "also malaria is a goo devensive option, in range of own temples the prophet still
        // works, its a quick way to dispatch an attacking army, if an fully charged prophet happens to be in the base"): within
        // HL_HOME_R of a town of its, the enemy soldiers and ships it knows within HL_DEFEND - the biggest group, HL_DEFEND_N
        // soldiers or HL_SHIPS ships within HL_CLUSTER of one, soldiers first - get what THE POWERS give them: malaria
        // where its own stand by to finish them, else the plague; ships a hurricane (its own Temple's protection is
        // against the enemy's powers, not its own).
        if (!casting && ivec2_dist2(p, home) <= HL_HOME_R * HL_HOME_R) {
            static ivec2 es[HL_SEEN];
            static uint8_t ek[HL_SEEN];
            int ne = 0;
            for (struct grid_it it = grid_it_box(p.x, p.y, HL_DEFEND, 1); grid_it_next(&it) && ne < HL_SEEN; ) {
                ee_player *o = unit_get_owner(it.u);
                int kd = o && is_enemy(pl, o) && unit_get_hp(it.u) > 0 && seen_by(it.u, pl) ? hl_kind(it.td) : -1;
                if (kd != HLK_SOLDIERS && kd != HLK_SHIPS) continue;
                ivec2 q = unit_get_tile(it.u);
                if (ivec2_dist2(q, p) <= HL_DEFEND * HL_DEFEND) { ek[ne] = (uint8_t)kd; es[ne++] = q; }
            }
            int bn = 0, bk = -1; ivec2 bat = p;
            for (int a = 0; a < ne; a++) {
                int c = 0;
                for (int b2 = 0; b2 < ne; b2++) c += ek[b2] == ek[a] && ivec2_dist2(es[b2], es[a]) <= HL_CLUSTER * HL_CLUSTER;
                if (c < (ek[a] == HLK_SHIPS ? HL_SHIPS : HL_DEFEND_N)) continue;
                int sa = ek[a] == HLK_SOLDIERS, sb = bk == HLK_SOLDIERS;
                if (bk < 0 || sa > sb || (sa == sb && c > bn)) { bn = c; bat = es[a]; bk = ek[a]; }
            }
            int def[3] = { -1, -1, -1 };
            if (bk >= 0) hl_powers(pl, bk, bat, now, def);
            for (int j = 0; j < 3 && bk >= 0 && !casting; j++) {
                int k = def[j];
                if (k < 0 || cost[k] <= 0 || pw < cost[k] || !hl_cast(pl, u, bat, k)) continue;
                hl_order_note(h, u, now);
                h->casts[k]++; h->defends++;
                strike = iv2(-1, -1); strike_t = 0;
                if (g_log) log_locked("# holy: P%u at %.1f min: a prophet at home at %d,%d casts %s on %d enemy %s at %d,%d (power %d)\n",
                                      idx, now / 60000.0, p.x, p.y, k_hl_name[k], bn, bk == HLK_SHIPS ? "ships" : "soldiers", bat.x, bat.y, pw);
                casting = 1;
            }
        }
        // THE TOWERS (the player 2026-11-16: "on towers, that where prophets are espacialy usefull if the enemy forgot its
        // temple. think that the prophet dosnt has to walk up to the tower, casting the earthquake a few tiles before the
        // tower from savethy is enough to bring it down"): the nearest enemy tower it knows within HL_TOWER_LOOK (a
        // building with a ground attack, attack.c defence_stats) gets an earthquake HL_QUAKE_OFF tiles before it on the
        // prophet's side - the prophet casts from its reach of that spot, outside the tower's. One under an enemy Temple's
        // protection fails the power's check and is left.
        if (!casting && cost[HLP_QUAKE] > 0 && pw >= cost[HLP_QUAKE]) {
            ivec2 tp = p, at = p;
            int bd = -1;
            float rr, aa;
            for (struct grid_it it = grid_it_box(p.x, p.y, HL_TOWER_LOOK, 1); grid_it_next(&it); ) {
                ee_player *o = unit_get_owner(it.u);
                if (!o || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || !seen_by(it.u, pl) || !defence_stats(it.u, &rr, &aa)) continue;
                ivec2 q = unit_get_tile(it.u);
                int d = ivec2_dist2(q, p);
                if (d > HL_TOWER_LOOK * HL_TOWER_LOOK || (bd >= 0 && d >= bd)) continue;
                float dx = (float)(p.x - q.x), dy = (float)(p.y - q.y), dl = sqrtf(dx * dx + dy * dy);
                ivec2 sp = dl < 1.0f ? q : iv2(q.x + (int)(dx / dl * HL_QUAKE_OFF + 0.5f), q.y + (int)(dy / dl * HL_QUAKE_OFF + 0.5f));
                if (hl_check(pl, u, sp, HLP_QUAKE) != 0) { h->refused++; continue; }   // THE REFUSED: the next tower
                bd = d; tp = q; at = sp;
            }
            if (bd >= 0) {
                if (hl_cast(pl, u, at, HLP_QUAKE)) {
                    hl_order_note(h, u, now);
                    h->casts[HLP_QUAKE]++; h->towers++;
                    strike = iv2(-1, -1); strike_t = 0;
                    if (g_log) log_locked("# holy: P%u at %.1f min: a prophet at %d,%d casts an earthquake at %d,%d before the enemy tower at %d,%d (power %d, cost %d)\n",
                                          idx, now / 60000.0, p.x, p.y, at.x, at.y, tp.x, tp.y, pw, cost[HLP_QUAKE]);
                    casting = 1;
                }
            }
        }
        if (!casting) {
            // what it knows within HL_REACH: the best place for each kind of power
            int nseen = 0;
            for (struct grid_it it = grid_it_box(p.x, p.y, HL_REACH, 1); grid_it_next(&it) && nseen < HL_SEEN; ) {
                ee_player *o = unit_get_owner(it.u);
                if (!o || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || !seen_by(it.u, pl) || ivec2_dist2(unit_get_tile(it.u), p) > HL_REACH * HL_REACH) continue;
                seen[nseen++] = it.u;
            }
            int best_n[HLK_N] = { 0, 0, 0, 0 };   // buildings, soldiers, citizens, ships
            ivec2 best_at[HLK_N];
            for (int a = 0; a < nseen; a++) {
                ee_unit_type *ta = unit_get_type_data(seen[a]);
                int kind = hl_kind(ta);
                if (kind < 0) continue;
                ivec2 q = unit_get_tile(seen[a]);
                int c = 0;
                for (int b = 0; b < nseen; b++) {
                    c += hl_kind(unit_get_type_data(seen[b])) == kind && ivec2_dist2(unit_get_tile(seen[b]), q) <= HL_CLUSTER * HL_CLUSTER;
                }
                if (c > best_n[kind]) { best_n[kind] = c; best_at[kind] = q; }
            }
            int pick = -1;
            ivec2 at = p;
            static const int need[HLK_N] = { HL_BUILDINGS, HL_SOLDIERS, HL_CITIZENS, HL_SHIPS };
            static const int korder[HLK_N] = { HLK_SHIPS, HLK_SOLDIERS, HLK_CITIZENS, HLK_BUILDINGS };
            for (int ki = 0; ki < HLK_N && pick < 0; ki++) {   // ships, soldiers, citizens, buildings
                int kd = korder[ki], order[3];
                if (best_n[kd] < need[kd]) continue;
                hl_powers(pl, kd, best_at[kd], now, order);   // THE POWERS
                for (int j = 0; j < 3 && pick < 0; j++) {
                    int k = order[j];
                    if (k < 0 || cost[k] <= 0 || pw < cost[k]) continue;
                    if (hl_check(pl, u, best_at[kd], k) != 0) { h->refused++; continue; }   // THE REFUSED: a Temple's protection there - the next power or kind
                    pick = k; at = best_at[kd];
                }
            }
            if (pick >= 0 && hl_cast(pl, u, at, pick)) {
                hl_order_note(h, u, now);
                h->casts[pick]++;
                strike = iv2(-1, -1);   // done: home (below, the next look)
                strike_t = 0;
                if (g_log) log_locked("# holy: P%u at %.1f min: a prophet at %d,%d casts %s at %d,%d (power %d, cost %d; %d buildings, %d soldiers, %d citizens, %d ships of the enemy there)\n",
                                      idx, now / 60000.0, p.x, p.y, k_hl_name[pick],
                                      at.x, at.y, pw, cost[pick], best_n[0], best_n[1], best_n[2], best_n[3]);
                casting = 1;
            }
        }
        if (!casting) {
            if (strike.x >= 0 && (hurt || cost[HLP_QUAKE] <= 0 || pw < cost[HLP_QUAKE] || raiders_near(pl, strike, 10) || (strike_t && now - strike_t > 300000))) strike = iv2(-1, -1);   // A STRIKE called off
            // WITH THE FORCE, ON ITS OWN (the first test: 0 strikes in 3 games - the force meets the enemy somewhere
            // nearly all the time, and every prophet went with it): the first prophet goes with the force, the others strike
            int with = fight && i == 0;
            if (strike.x < 0 && !hurt && with) hl_walk(u, hl_behind(front, home));   // with the force, as the siege
            else if (strike.x < 0 && !hurt && cost[HLP_QUAKE] > 0 && pw >= cost[HLP_QUAKE]) {   // A STRIKE: the nearest enemy building unguarded
                int bd = -1;
                for (struct grid_it it = grid_it_box(p.x, p.y, HL_STRIKE, 1); grid_it_next(&it); ) {
                    ee_player *o = unit_get_owner(it.u);
                    int fa = unit_type_get_class_id(it.td);
                    if (!o || !is_enemy(pl, o) || (fa != CLS_BUILDING && fa != CLS_FORTRESS) || is_wall(unit_type_get_id(it.td)) || unit_get_hp(it.u) <= 0 || !seen_by(it.u, pl)) continue;
                    ivec2 q = unit_get_tile(it.u);
                    int d = ivec2_dist2(q, p);
                    float rr, aa;
                    int tower = defence_stats(it.u, &rr, &aa);   // THE TOWERS: a tower is struck from outside its reach
                    if (d > HL_STRIKE * HL_STRIKE || (bd >= 0 && d >= bd) || raiders_near(pl, q, 10) || (!tower && wk_danger(idx, q))) continue;
                    if (hl_check(pl, u, q, HLP_QUAKE) == 2) { h->refused++; continue; }   // THE REFUSED: under an enemy Temple's protection - another
                    bd = d; strike = q;
                    if (tower) {   // where it stands to cast: the tower's reach + 2 from it, on its side
                        float dx = (float)(p.x - q.x), dy = (float)(p.y - q.y), dl = sqrtf(dx * dx + dy * dy);
                        if (dl >= 1.0f) strike = iv2(q.x + (int)(dx / dl * (rr + 2.0f)), q.y + (int)(dy / dl * (rr + 2.0f)));
                    }
                }
                if (strike.x >= 0) {
                    strike_t = now ? now : 1; h->strikes++;
                    if (g_log) log_locked("# holy: P%u at %.1f min: a prophet at %d,%d strikes at the enemy building at %d,%d (power %d)\n", idx, now / 60000.0, p.x, p.y, strike.x, strike.y, pw);
                }
            }
            // AT ITS TARGET (the second test: 10 strikes, no cast - the clusters want 2 buildings, a strike goes for one):
            // within HL_REACH of the building struck, the earthquake on it (a tower's strike ends in THE TOWERS above)
            if (strike.x >= 0 && ivec2_dist2(p, strike) <= HL_REACH * HL_REACH && pw >= cost[HLP_QUAKE]) {
                int bld = 0;
                for (struct grid_it it = grid_it_box(strike.x, strike.y, 2, 1); grid_it_next(&it) && !bld; ) {
                    ee_player *o = unit_get_owner(it.u);
                    int fa = unit_type_get_class_id(it.td);
                    bld = o && is_enemy(pl, o) && (fa == CLS_BUILDING || fa == CLS_FORTRESS) && unit_get_hp(it.u) > 0 && seen_by(it.u, pl);
                }
                if (bld && hl_cast(pl, u, strike, HLP_QUAKE)) {
                    hl_order_note(h, u, now);
                    h->casts[HLP_QUAKE]++;
                    if (g_log) log_locked("# holy: P%u at %.1f min: a prophet at %d,%d casts an earthquake at the building it struck at %d,%d (power %d)\n", idx, now / 60000.0, p.x, p.y, strike.x, strike.y, pw);
                    strike = iv2(-1, -1); strike_t = 0;
                }
            }
            if (strike.x >= 0) { hl_walk(u, strike); if (!escorted) { sn_escort(idx, p, HL_ESCORT, now); escorted = 1; } }   // stations.c AN ESCORT: beside it on the way
            else if (!with || hurt) hl_walk(u, home);
        }
        if (strike.x >= 0 && nk < HL_UNITS) { keep[nk].id = id; keep[nk].strike = strike; keep[nk++].strike_t = strike_t; }
    }
    memcpy(h->p, keep, (size_t)nk * sizeof *keep); h->np = nk;
    // PRIESTS
    for (int i = 0; i < npri; i++) {
        ee_unit *u = pri[i];
        ivec2 p = unit_get_tile(u);
        ee_goal *g = unit_get_goal(u);
        ee_unit *tg = g && goal_is_attack(g) ? goal_get_target(g) : NULL;
        ivec2 home = p;
        for (int k = 0, bd = -1; k < ntown; k++) { int d = ivec2_dist2(town[k], p); if (bd < 0 || d < bd) { bd = d; home = town[k]; } }
        int hurt = (float)unit_get_hp(u) < HL_HURT * (float)unit_type_get_hp(unit_get_type_data(u));
        int foes = raiders_near(pl, p, HL_ALONE + 1);
        if (hurt && foes) { hl_walk(u, home); continue; }
        if (tg && unit_get_hp(tg) > 0 && unit_get_owner(tg) != pl && raiders_near(pl, unit_get_tile(tg), HL_ALONE) <= 1) continue;   // converting
        if (hl_ordered(h, u, now)) continue;   // A GIVEN ORDER: on its way
        ee_unit *best = NULL;
        int bs = 0;
        for (struct grid_it it = grid_it_box(p.x, p.y, HL_PRIEST_LOOK, 1); grid_it_next(&it); ) {
            ee_player *o = unit_get_owner(it.u);
            int t = unit_type_get_id(it.td), cit = is_citizen_type(t);
            if (!o || !is_enemy(pl, o) || unit_get_hp(it.u) <= 0 || !seen_by(it.u, pl) || unit_get_x(it.u) < 0) continue;
            if (!cit && !(is_soldier(it.td) && unit_type_get_class_id(it.td) == CLS_LAND)) continue;
            ivec2 q = unit_get_tile(it.u);
            int d = ivec2_dist2(q, p);
            if (d > HL_PRIEST_LOOK * HL_PRIEST_LOOK || raiders_near(pl, q, HL_ALONE) > (cit ? 0 : 1) || wk_danger(idx, q)) continue;
            int sc = 100000 - d + (cit ? 50000 : 0);
            if (!best || sc > bs) { best = it.u; bs = sc; }
        }
        if (best) {
            ee_goal *ga = goal_attack(u, best);
            if (ga && unit_goal(u, ga, 0, 0x1F40)) { h->converts++; hl_order_note(h, u, now); }
            continue;
        }
        if (tg) continue;
        hl_walk(u, fight ? hl_behind(front, home) : home);
    }
    if (g_log && g_autotest && (!h->log_t || now - h->log_t >= 60000)) {
        h->log_t = now ? now : 1;
        log_locked("# holy: P%u at %.1f min: %d prophets, %d priests, Temple %d | so far trained %u prophets and %u priests, casts quake %u fire %u volcano %u plague %u hurricane %u malaria %u, %u strikes, %u at towers, %u parting gifts, %u at home, %u refused by the power's check, %u conversions ordered, lost in 10 minutes %d prophets %d priests | earthquake costs %d\n",
                   idx, now / 60000.0, npro, npri, temple, h->trained_p, h->trained_c, h->casts[0], h->casts[1], h->casts[2], h->casts[3], h->casts[4], h->casts[5], h->strikes, h->towers, h->gifts, h->defends, h->refused, h->converts, hl_lost(h, 0, now), hl_lost(h, 1, now), hl_cost(pl, HLP_QUAKE));
    }
}
