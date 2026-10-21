// GOLD AND IRON (docs/economy.md "Gold and iron"). Included by eeaimod.c after expand.c (navy.c's account, expand.c's
// sea_held, stats.c's damage, intel.c's memory, econ.c's budget).
// The playtester: "convert the economy into attack and defence". At a high population (unit limit 1500) the mod's army
// is limited by gold and iron, not citizens, food or wood (docs/economy.md "Workforce into army"): every soldier costs
// one of them, the stock stays at ~200, the army's orders are refused 10000-12000 times for them.
//
// "# metal:" every 5 game minutes (autotest; every player, the base AI too): the income since the start (F W S G I;
// EEComputerPlayer::AddResource), what was paid (EEComputerPlayer vf45, econ.c hook_pay - every payment of the game's
// and the mod's: the building queue 0x5E8A11 pays units and research, 0x62F076 buildings) by what it bought, from the
// tech it paid (tech +8 data; data +0x6C the type it makes, -1 for research; +0x48 the category, 7 = line upgrades):
// citizens, land soldiers by the building that trains them (inf Barracks, rng Archery Range, cav Stable, sie siege and
// rams, oth the rest - heroes, priests, tanks), warships (wsh: warships, submarines, carriers), boats (fishing boats,
// transports), air, the epochs (ep), line upgrades (up7), other research (tech), towers, walls, military buildings
// (mbl: Barracks .. Fortress, docks), other buildings (bld); each as n:F/W/S/G/I (payments, sums). Then the army alive
// by the same lines, the enemy soldiers it knows of (intel.c, seen within 10 minutes) by line, the budget's saving for
// a refused unit (econ.c substitute_wait: 5-s samples saving, the gold + iron in stock then, refusals while saving),
// the gold + iron in stock and in the navy account (5-s means), and what metal_plan did.
#define MT_NB 18
enum { MB_CIT, MB_INF, MB_RNG, MB_CAV, MB_SIE, MB_OTH, MB_WSH, MB_BOAT, MB_AIR, MB_EP, MB_UP7, MB_TECH, MB_TOWER, MB_WALL,
       MB_MBL, MB_BLD, MB_UNK, MB_NONE };
static const char *const k_mb[MT_NB] = { "cit", "inf", "rng", "cav", "sie", "oth", "wsh", "boat", "air", "ep", "up7", "tech",
                                         "tower", "wall", "mbl", "bld", "unk", "none" };
static struct mt_st {
    uint32_t t, ts;                 // last line, last sample
    unsigned min;
    int inc[7];                     // income since the start
    int paid[MT_NB][7]; unsigned np[MT_NB];
    unsigned samples, saving, save_refused; double save_gi, stock_gi, navy_gi;
    uint32_t save_last;             // the last refusal while saving
    unsigned picks, kept, nowait, navy_cut, waited;   // metal_plan, since the last line
    int navy_freed;
    uint32_t calm_since;            // NAVY: the sea calm since
} g_mt[16];
static void mt_new_game(void) { memset(g_mt, 0, sizeof g_mt); }

// what a type is to the army: its line (MB_*) - land soldiers by their producer (the tech's building list, data
// +0x10..+0x14), ships by family (docs/navy.md: 4 warships, 10 submarines, 21 carriers; 18 transports, 22 fishing boats)
static int mt_line(ee_player *pl, int t) {
    uint8_t *td = type_data(t);
    if (!td) return MB_UNK;
    int fam = FIELD(td, OF(TD, 0x4B4), int);
    if (is_citizen_type(t)) return MB_CIT;
    if (ut_role(t, UTR_TOWER)) return MB_TOWER;
    if (ut_role(t, UTR_WALL)) return MB_WALL;
    if (fam == 2 || fam == 29) {
        for (int k = 0; k < AM_NK; k++) if (t == g_ut[k_am_bld[k]]) return MB_MBL;
        return MB_BLD;
    }
    if (fam == 4 || fam == 10 || fam == 21) return MB_WSH;
    if (fam == 18 || fam == 22) return MB_BOAT;
    if (fam == 3 || fam == 7 || fam == 17) return MB_AIR;
    if (is_siege(t) || is_ram(t)) return MB_SIE;
    if (!is_army_family(fam)) return MB_UNK;
    ee_tech_tree *tt = FIELD(pl, PL_TECHTREE, ee_tech_tree *);
    ee_tech *tech = tt ? tech_lookup(tt, FIELD(td, OF(TD, 0x4E0), int)) : NULL;
    uint8_t *d = tech ? FIELD(tech, OF(TECH, 0x8), uint8_t *) : NULL;
    int *b = d ? FIELD(d, OF(TECHDATA, 0x10), int *) : NULL, *e = d ? FIELD(d, OF(TECHDATA, 0x14), int *) : NULL;
    for (int *p = b; b && e > b && e - b < 16 && p < e; p++) {
        if (*p == UT(B_BARRACKS)) return MB_INF;
        if (*p == UT(B_ARCHERY_RANGE)) return MB_RNG;
        if (*p == UT(B_STABLE)) return MB_CAV;
    }
    return MB_OTH;
}
// income (econ.c hook_add_resource, every player)
static void mt_income(ee_player *pl, int type, int amount) {
    if (amount > 0 && type >= 2 && type < 7) g_mt[(unsigned)FIELD(pl, PL_INDEX, int) & 15].inc[type] += amount;
}
// a payment (econ.c hook_pay, every player): what = the tech paid
static void mt_paid(ee_player *pl, uint8_t *what, const int *before, const int *stock) {
    struct mt_st *m = &g_mt[(unsigned)FIELD(pl, PL_INDEX, int) & 15];
    uint8_t *d = what ? FIELD(what, OF(TECH, 0x8), uint8_t *) : NULL;
    int b = MB_UNK;
    if (d) {
        int t = FIELD(d, OF(TECHDATA, 0x6C), int);
        if (t >= 0 && type_data(t)) b = mt_line(pl, t);
        else if (FIELD(d, OF(TECHDATA, 0x48), int) == 7) b = MB_UP7;
        else {
            b = MB_TECH;
            for (int e = 1; e <= 14; e++) if (d == epoch_tech(pl, e)) { b = MB_EP; break; }
        }
    }
    int any = 0;
    for (int r = 2; r < 7; r++) if (before[r] > stock[r]) { m->paid[b][r] += before[r] - stock[r]; any = 1; }
    if (any) m->np[b]++;
}
// econ.c train_budget: the budget saves for a refused unit
static void mt_saving(unsigned idx) { struct mt_st *m = &g_mt[idx & 15]; m->save_refused++; m->save_last = game_time(); if (!m->save_last) m->save_last = 1; }

// ---- metal_plan ([all], bits; [autotest] v_metal_plan for the variant players) ----
// 1 VALUE: a land soldier the game (or army_plan's FILL) asks for is trained as the one that does most against the
//   enemy army it knows per gold + iron, when that one is 25% better than the asked: the value of a type c is the enemy
//   soldiers one c kills in its life against the enemy's mix (intel.c: its soldiers seen within 10 minutes, by type
//   and owner; none known: its own trainable types as the mix) - c's damage per second at each (stats.c: attack x the
//   class table x the hit factor - the game's counters - less armour, the per-player upgrades, a shot every reload +
//   2 s) over that one's hit points, times c's hit points over the mix's damage per second at c (a ranged c behind
//   the line takes 60% of a melee mix's); a type that is already half the army counts a little less (1 / (1 + share)).
//   Its price: gold + iron + a quarter of food and wood + half the stone. The budget pays as before; a substitute
//   (unit_substitution) is picked the same way.
// 2 NOWAIT: the budget saves for a refused unit (substitute_wait, 90 s) only when its missing gold and iron come in
//   within that time at the account's income (income rate x the military share); else the substitute at once.
// 4 SPARE: VALUE's price weighs food, wood and stone by what lies unused: 1500+ beyond the reserve x0.05, under
//   twice the unit's price of it x1, else as above - so the piled food and wood buy the units that need them.
// 8 FLEET: the fleet's size target (navy.c stage 4: navy_fleet_pop 15% of the population cap - 76 warships at a cap of
//   505, never reached: the navy's share of the income stayed at its most, 50%, the whole game and the warships took
//   two thirds of the gold and iron on Small Islands Large) is at most what the sea needs: metal_fleet_pct (250%) of the
//   enemy warships it knows (intel.c: seen within 10 minutes) + 6, at least metal_fleet_min (20) x the map strength
//   (first measured with 150% and 12: more land soldiers, but warships -6 to -12 and Tournament Islands Huge with the
//   cheat kills - losses -49 (14)).
// 16 CALM: on a water map, the sea calm for 3 minutes (expand.c sea_held: no enemy warship seen in its waters for 5
//   minutes, its warships at least the enemy's it knows) - the navy account keeps at most 100 gold and 100 iron; the
//   rest is the army's (the account held up to 600 of each).
// Every bit acts only from a population cap of metal_cap (250): at the small maps' caps (~170) the metal is not what
// limits the army (Continental e0: the gold + iron stock lay at ~900 all game), and VALUE / FLEET lost there (VALUE:
// Continental e0 mod wins 11 -> 4 of 40; FLEET: Small Islands e5 kills - losses -11 to -26).
// Measured (docs/economy.md "Gold and iron"; paired against the same build without, 40 games a cell): on the large maps
// without the cheat FLEET (+ CALM) brought 1000-1700 more gold + iron by minute 60, 60-90 more land soldiers trained,
// 7-18 more soldiers at 75 and fewer losses; with the cheat the base AI won fewer games on Small / Large Islands, but
// kills - losses fell on Small Islands Large and Tournament Islands Huge. VALUE gave more soldiers and wins on Continental
// Large without the cheat, fewer kills everywhere. Default 0.
static int g_metal_plan = 0, g_v_metal_plan = -1, g_mt_fleet_pct = 250, g_mt_fleet_min = 20;
#define METAL_PLAN(idx) PV(idx, g_v_metal_plan, g_metal_plan)
static int g_mt_cap = 250;   // [all] metal_cap: the bits act from this population cap on (the metal limits the army there)
static int mt_plan(ee_player *pl, unsigned idx) {
    if (!METAL_PLAN(idx) || !pl) return 0;
    return unit_limit(pl) >= g_mt_cap ? METAL_PLAN(idx) : 0;
}
#define MT_CAND 48
static struct mt_val {
    ee_player *pl; uint32_t t;
    int n, type[MT_CAND]; float val[MT_CAND];   // trainable land soldiers, their value (per one, before the price)
    int foe;                                    // enemy soldiers in the mix (0: the mirror)
} g_mv[16];
static uint16_t g_mt_own[16][768];   // own land soldiers by type (VALUE's share), with the values
static int g_mt_army[16];

static float mt_dps(uint8_t *atd, uint8_t *ast, int api, uint8_t *ttd, uint8_t *tst, int tpi) {
    int s = st_shot_t(atd, ast, api, ttd, tst, tpi);
    return s > 0 ? (float)s / (st_reload_t(atd, ast) + ST_SHOT_OVERHEAD) : 0.0f;
}
static float mt_range_t(uint8_t *td, uint8_t *st) { return st ? FIELD(st, OF(USTAT, 0x9C), float) : FIELD(td, OF(TD, 0x420), float); }
static float mt_price(ee_player *pl, unsigned idx, int t, const int *stock, const int *reserve);
// the values of the trainable land soldiers against the known enemy mix, every 30 s
static void mt_values(ee_player *pl, unsigned idx) {
    struct mt_val *v = &g_mv[idx];
    uint32_t now = game_time();
    if (v->pl == pl && now >= v->t && now - v->t < 30000) return;
    v->pl = pl; v->t = now ? now : 1; v->n = 0; v->foe = 0;
    ee_tech_tree *tt = FIELD(pl, PL_TECHTREE, ee_tech_tree *);
    if (!tt) return;
    for (int t = 0; t < 724 && v->n < MT_CAND; t++) {
        if (!land_soldier(t) || soldier_score(t) <= 0) continue;
        ee_tech *tech = tech_lookup(tt, FIELD(type_data(t), OF(TD, 0x4E0), int));
        int c = 0;
        for (int r = 2; r < 7; r++) c += unit_cost(pl, t, r);
        if (!tech || !FIELD(tech, OF(TECH, 0x4), uint8_t) || c < 20) continue;   // nothing for free: the Court Jester (0), heroes of scenarios
        v->type[v->n++] = t;
    }
    // the enemy's mix: (type, owner) -> count
    struct { int type, owner, n; } mix[32];
    int nm = 0, tot = 0;
    struct intel *in = &g_intel[idx];
    for (int k = 0; k < in->n; k++) {
        struct intel_rec *e = &in->r[k];
        if (e->kind != 1 || in->now - e->t > 600000 || !type_data(e->type)) continue;
        int j;
        for (j = 0; j < nm && (mix[j].type != e->type || mix[j].owner != e->owner); j++) ;
        if (j == nm) { if (nm == 32) continue; mix[nm].type = e->type; mix[nm].owner = e->owner; mix[nm].n = 0; nm++; }
        mix[j].n++; tot++;
    }
    v->foe = tot;
    if (tot < 3) {   // nothing known: the mirror (its own trainable types, one each)
        nm = 0;
        for (int i = 0; i < v->n && nm < 32; i++) { mix[nm].type = v->type[i]; mix[nm].owner = (int)idx; mix[nm].n = 1; nm++; }
    }
    int pi = (int)idx;
    for (int i = 0; i < v->n; i++) {
        int c = v->type[i];
        uint8_t *ctd = type_data(c), *cst = st_rec(pl, c);
        float chp = (float)st_maxhp(ctd, cst), crange = mt_range_t(ctd, cst), kill = 0, inc = 0, w = 0;
        for (int j = 0; j < nm; j++) {
            void *opl = st_player(mix[j].owner);
            uint8_t *etd = type_data(mix[j].type), *est = st_rec(opl, mix[j].type);
            float ehp = (float)st_maxhp(etd, est), erange = mt_range_t(etd, est), n = (float)mix[j].n;
            if (ehp <= 0) continue;
            kill += n * mt_dps(ctd, cst, pi, etd, est, mix[j].owner) / ehp;
            float f = crange >= 3.0f && erange < 1.5f ? 0.6f : 1.0f;   // a ranged one behind the line, a melee mix
            inc += n * f * mt_dps(etd, est, mix[j].owner, ctd, cst, pi);
            w += n;
        }
        v->val[i] = w > 0 && kill > 0 ? (kill / w) * chp / (inc > 0.01f ? inc / w : 0.01f) : 0.0f;
    }
    static uint32_t logged[16];
    if (g_autotest && g_log && (!logged[idx] || now < logged[idx] || now / 300000 != logged[idx] / 300000)) {   // # value: every 5 minutes
        logged[idx] = now ? now : 1;
        EnterCriticalSection(&g_lock);
        fprintf(g_log, "# value: P%u at %.1f min: foe %d (%d types) |", idx, now / 60000.0, tot, nm);
        for (int i = 0; i < v->n; i++) fprintf(g_log, " %d:%.2f/%.0f", v->type[i], v->val[i], mt_price(pl, idx, v->type[i], NULL, NULL));
        fprintf(g_log, "\n");
        LeaveCriticalSection(&g_lock);
    }
}
// VALUE's price (SPARE: food, wood and stone by what lies unused)
static float mt_price(ee_player *pl, unsigned idx, int t, const int *stock, const int *reserve) {
    float p = 0;
    for (int r = 2; r < 7; r++) {
        int c = unit_cost(pl, t, r);
        if (c <= 0) continue;
        float w = r >= 5 ? 1.0f : r == 4 ? 0.5f : 0.25f;
        if (r < 5 && (METAL_PLAN(idx) & 4) && stock && reserve) {
            int spare = stock[r] - reserve[r];
            w = spare >= 1500 ? 0.05f : spare < 2 * c ? 1.0f : w;
        }
        p += w * (float)c;
    }
    return p > 1 ? p : 1;
}
static float mt_value_of(ee_player *pl, unsigned idx, int t, const int *stock, const int *reserve) {
    struct mt_val *v = &g_mv[idx];
    for (int i = 0; i < v->n; i++)
        if (v->type[i] == t) {
            float share = g_mt_army[idx] > 10 && t < 768 ? (float)g_mt_own[idx][t] / (float)g_mt_army[idx] : 0;
            return v->val[i] / (1.0f + share) / mt_price(pl, idx, t, stock, reserve);
        }
    return 0;
}
// VALUE (econ.c train_budget, a land soldier's request): the type to train instead of asked (0: as asked), *got of it
// that vis pays; trainable / blocked: the asked one's state
static int mt_choose(ee_player *pl, unsigned idx, int asked, const int *vis, const int *stock, const int *reserve, int want,
                     int blocked, int trainable, int *got) {
    if (!(mt_plan(pl, idx) & 1) || current_epoch(pl) < 1) return 0;
    mt_values(pl, idx);
    struct mt_val *v = &g_mv[idx];
    float va = !blocked && trainable ? mt_value_of(pl, idx, asked, stock, reserve) : 0, bv = 0;
    int best = 0, bn = 0;
    for (int i = 0; i < v->n; i++) {
        int t = v->type[i], n = want, paid = 0;
        for (int r = 2; r < 7; r++) {
            int c = unit_cost(pl, t, r);
            if (c > 0) { paid = 1; if (vis[r] / c < n) n = vis[r] / c; }
        }
        if (!paid || n <= 0) continue;
        float s = mt_value_of(pl, idx, t, stock, reserve);
        if (s > bv && can_build(pl, t)) { bv = s; best = t; bn = n; }
    }
    if (!best || best == asked || (va > 0 && bv < 1.25f * va)) { if (va > 0) g_mt[idx].kept++; return 0; }
    g_mt[idx].picks++;
    *got = bn;
    return best;
}
// NOWAIT (econ.c train_budget): saving for type makes no sense - its missing gold or iron won't come within wait s
static int mt_nowait(ee_player *pl, unsigned idx, int type, const int *vis, int wait) {
    if (!(mt_plan(pl, idx) & 2)) return 0;
    const struct ai_cfg *c = cfg_of(pl);
    float sh = current_epoch(pl) >= FARM_EPOCH ? c->mil_income_share_farms : c->mil_income_share;
    for (int r = 5; r < 7; r++) {
        int miss = unit_cost(pl, type, r) - vis[r];
        if (miss <= 0) continue;
        float per_s = g_rate[idx].rate[r] * sh / 60.0f;
        if (per_s <= 0.01f || (float)miss / per_s > (float)wait) { g_mt[idx].nowait++; return 1; }
    }
    g_mt[idx].waited++;
    return 0;
}
// CALM: the account's gold and iron while the sea is calm
static void mt_navy(ee_player *pl, unsigned idx, uint32_t now) {
    struct mt_st *m = &g_mt[idx];
    if (!(mt_plan(pl, idx) & 16) || navy_share(idx) <= 0) { m->calm_since = 0; return; }
    int mine = 0, theirs = 0, inw = 0;
    if (!sea_held(pl, idx, &mine, &theirs, &inw)) { m->calm_since = 0; return; }
    if (!m->calm_since) { m->calm_since = now ? now : 1; return; }
    if (now - m->calm_since < 180000) return;
    for (int r = 5; r < 7; r++)
        if (g_nv_acc[idx][r] > 100.0f) { m->navy_freed += (int)(g_nv_acc[idx][r] - 100.0f); g_nv_acc[idx][r] = 100.0f; m->navy_cut++; }
}
// FLEET (navy.c navy_tick, the stage-4 target t): at most what the sea needs
static int mt_fleet_target(ee_player *pl, unsigned idx, int t) {
    if (!(mt_plan(pl, idx) & 8) || t <= 0) return t;
    struct intel *in = &g_intel[idx & 15];
    int n = 0;
    for (int k = 0; k < in->n; k++) {
        struct intel_rec *e = &in->r[k];
        if (e->kind != 2 || in->now - e->t > 600000) continue;
        uint8_t *td = type_data(e->type);
        int nk = td ? navy_kind(td) : NV_NONE;
        n += nk == NV_WAR || nk == NV_SUB;
    }
    int need = (n * g_mt_fleet_pct + 99) / 100 + 6, floor_ = (int)((float)g_mt_fleet_min * g_nv_map[idx & 15].strength + 0.5f);
    if (need < floor_) need = floor_;
    return need < t ? need : t;
}

static void mt_log(ee_player *pl, unsigned idx, unsigned min) {
    struct mt_st *m = &g_mt[idx];
    int army[MT_NB] = { 0 }, foe[MT_NB] = { 0 };
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        if (unit_owner(it.u) != pl || FIELD(it.u, OF(UNIT, 0x3C), int) <= 0) continue;
        int t = FIELD(it.td, OF(TD, 0x4B8), int), b = mt_line(pl, t);
        if (b >= MB_INF && b <= MB_AIR) army[b]++;
    }
    if (!is_vanilla(pl)) {
        struct intel *in = &g_intel[idx];
        for (int k = 0; k < in->n; k++)
            if (in->r[k].kind == 1 && in->now - in->r[k].t <= 600000) {
                void *opl = st_player(in->r[k].owner);
                int b = opl ? mt_line(opl, in->r[k].type) : MB_UNK;
                if (b >= MB_INF && b <= MB_OTH) foe[b]++;
            }
    }
    int *st = (int *)((uint8_t *)pl + PL_STOCK);
    float sm = m->samples ? (float)m->samples : 1.0f;
    EnterCriticalSection(&g_lock);
    fprintf(g_log, "# metal: P%u at %u min: inc %d %d %d %d %d | stock %d %d %d %d %d |", idx, min, m->inc[2], m->inc[3], m->inc[4],
            m->inc[5], m->inc[6], st[2], st[3], st[4], st[5], st[6]);
    for (int b = 0; b < MT_NB; b++)
        if (m->np[b]) fprintf(g_log, " %s=%u:%d/%d/%d/%d/%d", k_mb[b], m->np[b], m->paid[b][2], m->paid[b][3], m->paid[b][4], m->paid[b][5], m->paid[b][6]);
    fprintf(g_log, " | army inf %d rng %d cav %d sie %d oth %d wsh %d boat %d air %d | foe inf %d rng %d cav %d sie %d oth %d | "
            "save %u/%u gi %.0f refused %u | gi %.0f navy %.0f | plan picks %u kept %u nowait %u waited %u navy %u freed %d\n",
            army[MB_INF], army[MB_RNG], army[MB_CAV], army[MB_SIE], army[MB_OTH], army[MB_WSH], army[MB_BOAT], army[MB_AIR],
            foe[MB_INF], foe[MB_RNG], foe[MB_CAV], foe[MB_SIE], foe[MB_OTH], m->saving, m->samples,
            m->saving ? m->save_gi / m->saving : 0.0, m->save_refused, m->stock_gi / sm, m->navy_gi / sm,
            m->picks, m->kept, m->nowait, m->waited, m->navy_cut, m->navy_freed);
    LeaveCriticalSection(&g_lock);
    m->samples = m->saving = m->save_refused = 0;
    m->save_gi = m->stock_gi = m->navy_gi = 0;
    m->picks = m->kept = m->nowait = m->waited = m->navy_cut = 0;
    m->navy_freed = 0;
}
// every player's update (eeaimod.c hook_rm_update)
static void mt_tick(ee_player *pl, uint32_t now) {
    unsigned idx = (unsigned)FIELD(pl, PL_INDEX, int) & 15;
    struct mt_st *m = &g_mt[idx];
    if (now < m->ts) { memset(m, 0, sizeof *m); }
    if (m->ts && now - m->ts < 5000) return;
    m->ts = now ? now : 1;
    int *st = (int *)((uint8_t *)pl + PL_STOCK);
    int van = is_vanilla(pl);
    if (!van && METAL_PLAN(idx)) {
        mt_navy(pl, idx, now);
        if (METAL_PLAN(idx) & 1) {   // the army's types (VALUE's share), every 5 s with the samples
            static uint32_t own_t[16];
            if (now < own_t[idx] || now - own_t[idx] >= 30000) {
                own_t[idx] = now;
                memset(g_mt_own[idx], 0, sizeof g_mt_own[idx]);
                g_mt_army[idx] = 0;
                for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
                    if (unit_owner(it.u) != pl || FIELD(it.u, OF(UNIT, 0x3C), int) <= 0) continue;
                    int t = FIELD(it.td, OF(TD, 0x4B8), int);
                    if (t >= 0 && t < 768 && land_soldier(t)) { g_mt_own[idx][t]++; g_mt_army[idx]++; }
                }
            }
        }
    }
    if (!g_autotest || !g_log) return;
    m->samples++;
    m->stock_gi += st[5] + st[6];
    if (!van) m->navy_gi += g_nv_acc[idx][5] + g_nv_acc[idx][6];
    if (!van && m->save_last && now >= m->save_last && now - m->save_last <= 15000 && g_save_since[idx]) { m->saving++; m->save_gi += st[5] + st[6]; }
    if (!m->t) { m->t = now ? now : 1; m->min = now / 300000; return; }
    if (now / 300000 == m->min) return;
    m->min = now / 300000;
    mt_log(pl, idx, m->min * 5);
}
static void mt_install(const char *aimod_ini, const char *autotest_ini) {
    g_metal_plan = cfg_int(aimod_ini, autotest_ini, 1, "metal_plan", 0);
    g_mt_fleet_pct = cfg_int(aimod_ini, autotest_ini, 1, "metal_fleet_pct", 250);
    g_mt_fleet_min = cfg_int(aimod_ini, autotest_ini, 1, "metal_fleet_min", 20);
    g_mt_cap = cfg_int(aimod_ini, autotest_ini, 1, "metal_cap", 250);
    if (autotest_ini) g_v_metal_plan = ini_int("autotest", "v_metal_plan", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# metal plan: %d (variant %d), fleet %d%% of the enemy's + 6, at least %d\n", g_metal_plan, g_v_metal_plan,
                       g_mt_fleet_pct, g_mt_fleet_min);
}
