// PRODUCTION BY TERRITORY - where the production buildings go, played while the game's computer player is off
// ([ai_base] game_ai = 0): which building, in which territory; build.c lays it (PRODUCTION there).
// The player, 2026-11-14: "let it place buildings, wit production buildings it should actually prefer frontier
// territories (that are somehwat established and secured) so units are close to where they are needed. probably bets
// if territories request buildings based on demand. they say how much units they want (devence, exploration, attack)
// and neighbouring territoirs give thes units if they a free. if no units a free and must be produced the territory
// should build the production buildings itself. scaled with distance to existing production, if there is a production
// hub in a neigbor its fine to use it, if the next prodcution is many land and even sea territories away: lets build
// up localy".
// Every PR_EVERY per AI, on the territory view (territories.c):
//   SHARES    - the territories deal in shares of the whole force, not in soldiers (the player, the same day: "now
//               that i think about it, territories probably shouldnt deal with real unit numbers but relations
//               instead. 100% is the full military force and the terriotories distribute that under each other, with
//               conversions into the system like how much % of all the force do i need to defend against the current
//               attack, do i have that share alredy or could my friendly neighboars contribute"). The force: its land
//               soldiers, wherever they stand. Of its 1000 thousandths
//                 AN ATTACK comes first: a territory it holds with enemy soldiers in it needs PR_MARGIN times their
//                 number - converted: that many of the force as a share of it (more than the whole force in all:
//                 each its part of the whole, and the look says by how much the force is too small);
//                 THE REST is divided by weight among the land territories it holds: DEFENCE, a front's guard (the
//                 view's: a third of its citizens, 3 to 12, 8 with enemy soldiers seen there in the last 5 minutes;
//                 backland none), EXPLORATION, 1 with a land neighbour nobody is known to hold, ATTACK, beside a
//                 territory with enemy buildings it has seen PR_ATTACK and one for each of them, PR_ATTACK_MAX at most.
//   HAS       - the share of the force standing in the territory. Beyond its own share it is free: a neighbour short
//               of its share counts it as its own (the count only, here: who marches where is the armies').
//   SHORT     - the share still missing is what production has to bring there. Times the way to its nearest
//               production building (a finished one, territory by territory: 1 a land territory, PR_SEA onto water,
//               PR_FAR with none anywhere) beyond the first step it is the territory's claim: from PR_LOCAL it builds
//               its own - a hub next door serves whatever is missing ("if there is a production hub in a neigbor its
//               fine to use it"), two territories off a quarter of the force missing builds, three off an eighth,
//               across a sea a twelfth. Only a territory that is established (PR_BLD of its buildings) and secured
//               (no enemy soldier in it, not being lost) builds, and none before it is fed (below).
//   WHAT      - the first building of a hub: the first of PR_KINDS the game offers (a Barracks). A kind it has
//               nowhere yet follows, in the hub that wants the most - but past its first production building
//               nothing before it is FED (the player 2026-11-08: one production building until 2 farms).   
//               And one more of a kind the army plan found busy (army.c BUILD, which asked the game's build manager
//               for it): in the hub that wants the most too. So a far hub starts as one Barracks and grows by what
//               is used (every hub with every kind at first: one AI at 10 production buildings in 4 territories
//               with 23 soldiers).
// "# produce:" a minute.
#define PR_EVERY 10000
#define PR_T 4096
#define PR_SEA 3
#define PR_FAR 8
#define PR_LOCAL 250      // thousandths of the force missing x steps beyond the first
#define PR_HOLD 60000     // an attack's size holds this long
#define PR_MARGIN 5 / 4   // against an attack: this many times the enemy soldiers there
#define PR_BLD 3
#define PR_ATTACK 10
#define PR_ATTACK_MAX 30
internal const enum ut_name PR_KINDS[] = { UT_B_BARRACKS, UT_B_ARCHERY_RANGE, UT_B_STABLE, UT_B_SIEGE_FACTORY, UT_B_TANK_FACTORY, UT_B_CYBER_FACTORY };
#define PR_NK ((int)(sizeof PR_KINDS / sizeof *PR_KINDS))
internal struct pr_st {
    uint32_t t, log_t, more_t;
    int type, ter, first;    // what it asks build.c for: the building (0: nothing now), the territory; first: it has no production building yet
    ivec2 at;                // ... by this building of its there
    const char *why;
    int hubs, buildings, force, attack, over, shortn, claims;   // the last look: territories with production, production buildings; its land soldiers; the share an attack takes, what the attacks ask beyond the whole force, the share missing where no neighbour gives (thousandths); territories whose claim reaches PR_LOCAL
} g_pr[16];
// the shares of the last look, by territory (thousandths of the force): what it should have, what stands there
internal uint16_t g_pr_share[16][PR_T], g_pr_has[16][PR_T];
internal uint16_t g_pr_foe[16][PR_T]; internal uint32_t g_pr_foe_t[16][PR_T];   // the attack's size that holds: enemy soldiers, seen when
internal int pr_kind(int type) {
    for (int k = 0; k < PR_NK; k++) if (g_ut[PR_KINDS[k]] >= 0 && g_ut[PR_KINDS[k]] == type) return k;
    return -1;
}
internal void produce_look(ee_player *pl, unsigned idx, uint32_t now) {
    static int want[PR_T], have[PR_T], fre[PR_T], sh[PR_T], hard[PR_T], wgt[PR_T];
    static uint8_t dist[PR_T], np[PR_T][PR_NK], anc[PR_T];
    static ivec2 anchor[PR_T];
    struct pr_st *z = &g_pr[idx];
    struct tv_st *s = &g_tv[idx];
    if (now < z->t) memset(z, 0, sizeof *z);
    if (z->t && now - z->t < PR_EVERY) return;
    z->t = now ? now : 1;
    z->type = 0;
    int n = s->n < PR_T ? s->n : PR_T;
    if (!n || s->version != g_ter.version) return;
    memset(np, 0, sizeof np); memset(anc, 0, (size_t)n); memset(dist, 255, (size_t)n);
    int count = 0, granaries = 0, total = 0, done = 0;
    for (ee_unit *u = player_get_units(pl); u && count < 16384; u = unit_get_next(u), count++) {   // its production buildings and a building to build by, territory by territory
        ee_unit_type *td = unit_get_type_data(u);
        if (!td || unit_get_hp(u) <= 0 || unit_get_x(u) < 0) continue;
        int t = unit_type_get_id(td), fam = unit_type_get_class_id(td);
        if ((fam != CLS_BUILDING && fam != CLS_FORTRESS) || is_wall(t)) continue;
        ivec2 p = unit_get_tile(u);
        unsigned r = ter_near(p.x, p.y);
        if (r >= (unsigned)n) continue;
        ee_unit *st = unit_get_physics(u);
        int fin = st && unit_get_finished(st), k = pr_kind(t);
        int town = t == UT(B_TOWN_CENTER) || t == UT(B_CAPITOL) ? 3 : t == UT(B_SETTLEMENT) ? 2 : 1;
        if (fin && town > anc[r]) { anc[r] = (uint8_t)town; anchor[r] = p; }
        granaries += fin && (t == UT(B_GRANARY) || t == UT(B_GRANARY_IMPROVED));
        if (k < 0) continue;
        if (np[r][k] < 255) np[r][k]++;
        total++;
        if (fin) { dist[r] = 0; done++; }
    }
    for (int pass = 0, ch = 1; ch && pass < n; pass++) {   // the way to its nearest production, territory by territory
        ch = 0;
        for (int r = 0; r < n; r++) {
            if (dist[r] >= PR_FAR) continue;
            for (int k = 0; k < g_ter.t[r].nbn; k++) {
                unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
                if (o >= (unsigned)n) continue;
                int c = dist[r] + (g_ter.t[o].water ? PR_SEA : 1);
                if (c < dist[o] && c <= PR_FAR) { dist[o] = (uint8_t)c; ch = 1; }
            }
        }
    }
    z->force = z->attack = z->over = z->shortn = z->hubs = z->claims = 0; z->buildings = total;
    int force = 0, hsum = 0, wsum = 0;
    for (int r = 0; r < n; r++) if (!g_ter.t[r].water) force += s->t[r].sold;
    for (int r = 0; r < n; r++) {   // SHARES: an attack's as a share of the force, the weights of the rest
        struct tv_terr *t = &s->t[r];
        int d = 0, e = 0, a = 0;
        hard[r] = wgt[r] = 0;
        if (!g_ter.t[r].water && t->role != TVR_NONE) {
            // the attack's size holds: the most enemy soldiers seen there in the last PR_HOLD (they step in and out of
            // sight between two looks, and with every step the attacked territory's share halved and doubled and every
            // other's with it - the whole force marching there and back)
            int foe = t->foe;
            if (foe >= g_pr_foe[idx][r] || now < g_pr_foe_t[idx][r] || now - g_pr_foe_t[idx][r] >= PR_HOLD) { g_pr_foe[idx][r] = (uint16_t)(foe > 65535 ? 65535 : foe); g_pr_foe_t[idx][r] = now; }
            else foe = g_pr_foe[idx][r];
            if (foe > 0) { int need = foe * PR_MARGIN + 1; hard[r] = force > 0 ? need * 1000 / force : 1000; }
            d = t->role == TVR_FRONT || (t->foe_t && now >= t->foe_t && now - t->foe_t < TV_FOE_MEMORY) ? t->want : 0;
            for (int k = 0; k < g_ter.t[r].nbn; k++) {
                unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
                if (o >= (unsigned)n || g_ter.t[o].water) continue;
                if (!s->t[o].friendly && !s->t[o].ebld) e = 1;
                if (s->t[o].ebld) { int w = PR_ATTACK + s->t[o].ebld; if (w > PR_ATTACK_MAX) w = PR_ATTACK_MAX; if (w > a) a = w; }
            }
            wgt[r] = d + e + a;
        }
        hsum += hard[r]; wsum += wgt[r];
        int any = 0;
        for (int k = 0; k < PR_NK; k++) any += np[r][k];
        z->hubs += any > 0;
    }
    z->force = force; z->over = hsum > 1000 ? hsum - 1000 : 0; z->attack = hsum > 1000 ? 1000 : hsum;
    for (int r = 0; r < n; r++) {   // ... the share each should have, the share that stands there
        int h = hsum > 1000 ? hard[r] * 1000 / hsum : hard[r];
        want[r] = h + (wsum > 0 ? (1000 - z->attack) * wgt[r] / wsum : 0);
        have[r] = force > 0 && !g_ter.t[r].water ? s->t[r].sold * 1000 / force : 0;
        fre[r] = have[r] > want[r] ? have[r] - want[r] : 0;
        sh[r] = want[r] > have[r] ? want[r] - have[r] : 0;
        g_pr_share[idx][r] = (uint16_t)want[r]; g_pr_has[idx][r] = (uint16_t)have[r];
    }
    for (int r = 0; r < n; r++)   // HAS: a neighbour's free share counts
        for (int k = 0; sh[r] > 0 && k < g_ter.t[r].nbn; k++) {
            unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
            if (o >= (unsigned)n || g_ter.t[o].water || !fre[o]) continue;
            int g = fre[o] < sh[r] ? fre[o] : sh[r];
            fre[o] -= g; sh[r] -= g;
        }
    int best = -1, bscore = 0, hub = -1, hwant = -1, first = -1, fscore = -1;
    for (int r = 0; r < n; r++) {   // SHORT: the claims
        struct tv_terr *t = &s->t[r];
        z->shortn += sh[r];
        if (g_ter.t[r].water || t->role == TVR_NONE || !anc[r]) continue;
        int any = 0;
        for (int k = 0; k < PR_NK; k++) any += np[r][k];
        if (any && t->state == TV_QUIET && want[r] > hwant) { hwant = want[r]; hub = r; }   // a quiet one: the hub that wants the most is the one under attack, and nothing was built while it was
        // PRODUCTION WHERE THE FIGHT IS (stations.c, fight_production): a territory under attack that holds (TV_FOES: the
        // enemy's soldiers there beaten) and is short of its share claims its own at once, before the quiet ones
        int fought = FIGHT_PROD(idx) && t->state == TV_FOES && hard[r] > 0 && sh[r] > 0;
        if (t->bld < PR_BLD || (t->state != TV_QUIET && !fought) || any) continue;   // established and secured (or fought and holding), no hub yet
        int fs = (t->role == TVR_FRONT ? 100000 : 0) + sh[r] * 10 + t->bld;   // with no production anywhere: a front first, the one missing the most
        if (fs > fscore) { fscore = fs; first = r; }
        int d = dist[r] > PR_FAR ? PR_FAR : dist[r], score = sh[r] * (d - 1) + (fought ? PR_LOCAL + (1 << 20) : 0);
        if (score < PR_LOCAL) continue;
        z->claims++;
        if (score > bscore) { bscore = score; best = r; }
    }
    // FED: 2 granaries, or the food coming in is what its citizen producers use without a pause (the patches feed it:
    // with them filled first the second granary came at minute 30, and one Barracks trained all its soldiers)
    int fed = granaries >= 2 || g_rate[idx].rate[2] >= cz_nonstop_food(pl);
    int ter = -1, type = 0;
    const char *why = NULL;
    if (!total && first >= 0) { ter = first; why = "its first production"; }
    else if (best >= 0 && done && fed) { ter = best; why = "a territory builds its own: too far from the next production for what it misses"; }   // none while its first is still going up
    if (ter >= 0)
        for (int k = 0; k < PR_NK && !type; k++) if (ut_has(g_ut[PR_KINDS[k]]) && prio_buildable(pl, g_ut[PR_KINDS[k]])) type = g_ut[PR_KINDS[k]];
    if (!type && total && fed && hub >= 0 && s->t[hub].state == TV_QUIET) {   // a kind it has nowhere: in the hub that wants the most
        for (int k = 0; k < PR_NK && !type; k++) {
            int all = 0;
            for (int r = 0; r < n; r++) all += np[r][k];
            if (!all && ut_has(g_ut[PR_KINDS[k]]) && prio_buildable(pl, g_ut[PR_KINDS[k]])) { ter = hub; type = g_ut[PR_KINDS[k]]; why = "a kind it has nowhere yet"; }
        }
    }
    if (!type && g_pr_more[idx] && hub >= 0 && fed && s->t[hub].state == TV_QUIET && prio_buildable(pl, g_pr_more[idx])) {
        ter = hub; type = g_pr_more[idx]; why = "one more: the army plan found that kind busy";
    }
    if (g_pr_more[idx] && (!z->more_t || now < z->more_t)) z->more_t = now ? now : 1;
    if (g_pr_more[idx] && now - z->more_t > 120000) { g_pr_more[idx] = 0; z->more_t = 0; }   // asked 2 minutes ago and not built: the army plan asks again when it still holds
    if (type && ter >= 0) { z->type = type; z->ter = ter; z->at = anchor[ter]; z->why = why; z->first = !total; }
    if (g_log && (!z->log_t || now - z->log_t >= 60000)) {
        z->log_t = now ? now : 1;
        char buf[400] = "";
        int o = 0;
        for (int r = 0; r < n && o < 330; r++)
            if (want[r] || have[r]) o += snprintf(buf + o, sizeof buf - (size_t)o, " %d:%d/%d%s%s", r, (have[r] + 5) / 10, (want[r] + 5) / 10, hard[r] ? "!" : "", dist[r] == 0 ? "*" : "");
        log_locked("# produce: P%u at %.1f min: the force %d soldiers | an attack takes %d%% of it%s, missing where no neighbour gives %d%% | production buildings %d (%d finished) in %d territories, granaries %d | claims %d | asks for %d in territory %d (%s) | territory:has%%/share%% (! under attack, * production):%s\n",
                   idx, now / 60000.0, z->force, (z->attack + 5) / 10, z->over ? " - the force is too small for it" : "", (z->shortn + 5) / 10, total, done, z->hubs, granaries, z->claims, z->type, z->type ? z->ter : -1, z->type ? z->why : "nothing", buf);
    }
}
internal void produce_new_game(void) { memset(g_pr, 0, sizeof g_pr); memset(g_fp_n, 0, sizeof g_fp_n); memset(g_pr_more, 0, sizeof g_pr_more); memset(g_pr_share, 0, sizeof g_pr_share); memset(g_pr_has, 0, sizeof g_pr_has); memset(g_pr_foe, 0, sizeof g_pr_foe); memset(g_pr_foe_t, 0, sizeof g_pr_foe_t); }
