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
//   WANTED    - a land territory it holds says how many soldiers it wants: DEFENCE, a front's guard (the view's
//               want: a third of its citizens, 3 to 12, 8 with enemy soldiers seen there in the last 5 minutes;
//               backland none) and one for each enemy soldier there now; EXPLORATION, 1 with a land neighbour nobody
//               is known to hold; ATTACK, beside a territory with enemy buildings it has seen PR_ATTACK and one for
//               each of them, PR_ATTACK_MAX at most.
//   GIVEN     - its soldiers standing in a territory beyond what that one wants are free: a neighbour short of
//               soldiers counts them as its own (the count only, here: who marches where is the armies').
//   SHORT     - what is still missing has to be produced. Its way to the nearest production building of its (a
//               finished one, territory by territory: 1 a land territory, PR_SEA onto water, PR_FAR with none
//               anywhere) beyond the first step, times the soldiers missing, is the territory's claim: from
//               PR_LOCAL it builds its own - a hub next door serves whatever is missing ("if there is a production
//               hub in a neigbor its fine to use it": the first look built a second hub beside the first for 16
//               missing), two territories off 12 missing build, three off 6, across a sea 4. Only a territory that is established (PR_BLD of its buildings) and secured
//               (no enemy soldier in it, not being lost) builds: the claim waits for the Settlement and the tower.
//   WHAT      - the first building of a hub: the first of PR_KINDS the game offers (a Barracks). A kind it has
//               nowhere yet follows, in the hub that wants the most - but past its first production building
//               nothing before it has 2 granaries (the player 2026-11-08: one production building until 2 farms).
//               And one more of a kind the army plan found busy (army.c BUILD, which asked the game's build manager
//               for it): in the hub that wants the most too. So a far hub starts as one Barracks and grows by what
//               is used (every hub with every kind at first: one AI at 10 production buildings in 4 territories
//               with 23 soldiers).
// "# produce:" a minute.
#define PR_EVERY 10000
#define PR_T 4096
#define PR_SEA 3
#define PR_FAR 8
#define PR_LOCAL 12
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
    int hubs, buildings, want, have, shortn, claims;   // the last look: territories with production, production buildings, soldiers wanted / there / still missing, territories whose claim reaches PR_LOCAL
} g_pr[16];
internal int pr_kind(int type) {
    for (int k = 0; k < PR_NK; k++) if (g_ut[PR_KINDS[k]] >= 0 && g_ut[PR_KINDS[k]] == type) return k;
    return -1;
}
internal void produce_look(ee_player *pl, unsigned idx, uint32_t now) {
    static short want[PR_T], have[PR_T], fre[PR_T], sh[PR_T];
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
    z->want = z->have = z->shortn = z->hubs = z->claims = 0; z->buildings = total;
    for (int r = 0; r < n; r++) {   // WANTED
        struct tv_terr *t = &s->t[r];
        int d = 0, e = 0, a = 0;
        want[r] = 0; have[r] = g_ter.t[r].water ? 0 : t->sold;
        if (!g_ter.t[r].water && t->role != TVR_NONE) {
            d = (t->role == TVR_FRONT || (t->foe_t && now >= t->foe_t && now - t->foe_t < TV_FOE_MEMORY) ? t->want : 0) + t->foe;
            for (int k = 0; k < g_ter.t[r].nbn; k++) {
                unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
                if (o >= (unsigned)n || g_ter.t[o].water) continue;
                if (!s->t[o].friendly && !s->t[o].ebld) e = 1;
                if (s->t[o].ebld) { int w = PR_ATTACK + s->t[o].ebld; if (w > PR_ATTACK_MAX) w = PR_ATTACK_MAX; if (w > a) a = w; }
            }
            want[r] = (short)(d + e + a);
        }
        fre[r] = (short)(have[r] > want[r] ? have[r] - want[r] : 0);
        sh[r] = (short)(want[r] > have[r] ? want[r] - have[r] : 0);
        z->want += want[r]; z->have += have[r];
        int any = 0;
        for (int k = 0; k < PR_NK; k++) any += np[r][k];
        z->hubs += any > 0;
    }
    for (int r = 0; r < n; r++)   // GIVEN
        for (int k = 0; sh[r] > 0 && k < g_ter.t[r].nbn; k++) {
            unsigned o = g_ter.nb[g_ter.t[r].nb0 + k].to;
            if (o >= (unsigned)n || g_ter.t[o].water || !fre[o]) continue;
            int g = fre[o] < sh[r] ? fre[o] : sh[r];
            fre[o] = (short)(fre[o] - g); sh[r] = (short)(sh[r] - g);
        }
    int best = -1, bscore = 0, hub = -1, hwant = -1, first = -1, fscore = -1;
    for (int r = 0; r < n; r++) {   // SHORT: the claims
        struct tv_terr *t = &s->t[r];
        z->shortn += sh[r];
        if (g_ter.t[r].water || t->role == TVR_NONE || !anc[r]) continue;
        int any = 0;
        for (int k = 0; k < PR_NK; k++) any += np[r][k];
        if (any && want[r] > hwant) { hwant = want[r]; hub = r; }
        if (t->bld < PR_BLD || t->state != TV_QUIET || any) continue;   // established and secured, no hub yet
        int fs = (t->role == TVR_FRONT ? 1000 : 0) + sh[r] * 10 + t->bld;   // with no production anywhere: a front first, the one missing the most
        if (fs > fscore) { fscore = fs; first = r; }
        int d = dist[r] > PR_FAR ? PR_FAR : dist[r], score = sh[r] * (d - 1);
        if (score < PR_LOCAL) continue;
        z->claims++;
        if (score > bscore) { bscore = score; best = r; }
    }
    int ter = -1, type = 0;
    const char *why = NULL;
    if (!total && first >= 0) { ter = first; why = "its first production"; }
    else if (best >= 0 && done) { ter = best; why = "a territory builds its own: too far from the next production for what it misses"; }   // none while its first is still going up
    if (ter >= 0)
        for (int k = 0; k < PR_NK && !type; k++) if (ut_has(g_ut[PR_KINDS[k]]) && prio_buildable(pl, g_ut[PR_KINDS[k]])) type = g_ut[PR_KINDS[k]];
    if (!type && total && granaries >= 2 && hub >= 0 && s->t[hub].state == TV_QUIET) {   // a kind it has nowhere: in the hub that wants the most
        for (int k = 0; k < PR_NK && !type; k++) {
            int all = 0;
            for (int r = 0; r < n; r++) all += np[r][k];
            if (!all && ut_has(g_ut[PR_KINDS[k]]) && prio_buildable(pl, g_ut[PR_KINDS[k]])) { ter = hub; type = g_ut[PR_KINDS[k]]; why = "a kind it has nowhere yet"; }
        }
    }
    if (!type && g_pr_more[idx] && hub >= 0 && granaries >= 2 && s->t[hub].state == TV_QUIET && prio_buildable(pl, g_pr_more[idx])) {
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
            if (want[r] || have[r]) o += snprintf(buf + o, sizeof buf - (size_t)o, " %d:%d/%d%s", r, have[r], want[r], dist[r] == 0 ? "*" : "");
        log_locked("# produce: P%u at %.1f min: soldiers wanted %d, there %d, to produce %d | production buildings %d (%d finished) in %d territories, granaries %d | claims %d | asks for %d in territory %d (%s) | territory:there/wanted (* production):%s\n",
                   idx, now / 60000.0, z->want, z->have, z->shortn, total, done, z->hubs, granaries, z->claims, z->type, z->type ? z->ter : -1, z->type ? z->why : "nothing", buf);
    }
}
internal void produce_new_game(void) { memset(g_pr, 0, sizeof g_pr); memset(g_pr_more, 0, sizeof g_pr_more); }
