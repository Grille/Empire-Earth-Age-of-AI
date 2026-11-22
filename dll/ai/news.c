// THE NEWS: what the force and the territories give an AI to say - its personality's lines (voice/say.h). Included by
// eeaimod.c after stations.c and ferry.c, whose pushes it reads; every step of an AI of the mod's, after them.
// The situations (persona.c k_lines; the player 2026-11-22, of a look at the lines: the attack lines had gone silent
// with the game's computer player off, and none spoke of the ground the territories now fight over):
//   attack, attack_threat - a push sets out (stations.c A PUSH): the threat to the player whose territory it goes into,
//              the news to the AI's allies - the game's attack waves had raised them (persona_attack), the mod's
//              pushes never did;
//   landing  - a push across the water has NW_LANDED of its soldiers on the other land (ferry.c), to that player;
//   territory_taken - a land territory is its now that an enemy held before (or held within NW_TAKEN_MEMORY before it
//              stood empty): to that enemy;
//   territory_lost - one it held is no longer its: to the enemy who holds it now, else the one with the most there;
//   territory_held - one it holds was being lost (territories.c UNSAFE) and is safe again with no enemy soldier
//              left there: to the enemy that came.
// Who holds a territory: game/territory.c ter_facts - the player with the most finished buildings there that hold
// ground (a tower, a Town Center, a production building), looked at every NW_EVERY (here: the map's facts are
// otherwise worked out only while the overlay shows them). The facts are the game's, as the lines' other values (a
// player's strength, its kills) are; the AI's own view (territories.c) only for UNSAFE. The same on every PC, so the
// situations' handlers (on_territory_lost ...) run alike everywhere. A new game or a loaded one starts from what holds
// then: nothing said of what happened before.
// "# news:" a line each.
#define NW_EVERY 10000
#define NW_TAKEN_MEMORY 600000
#define NW_LANDED 3
#define NW_T PR_T
struct nw_st {
    int ready; unsigned version; int n; uint32_t facts_t, log_t;   // facts_t: the facts it last went through
    int8_t owner[NW_T];                       // each land territory's holder at the last look, 0 none
    int8_t was[NW_T]; uint32_t was_t[NW_T];   // THE TAKEN GROUND: the enemy that last held it, until then
    int8_t att[NW_T]; uint8_t unsafe[NW_T];   // THE HELD GROUND: the enemy there while it was being lost
    uint32_t push_t[SN_PUSHES]; int8_t push_to[SN_PUSHES]; uint8_t landed[SN_PUSHES];   // each push slot's start it told, at whom, the landing told
};
internal struct nw_st g_nw[16];
internal uint32_t g_nw_facts_t;
internal unsigned g_nw_said[16][6];   // the log's: attacks, landings, taken, lost, held, pushes at nobody

// the enemy of pl with the most units (units 1) or buildings (0) in territory r - else of the other kind; 0 none
internal int nw_enemy_in(ee_player *pl, unsigned r, int units) {
    const struct ter_terr *t = &g_ter.t[r];
    for (int pass = 0; pass < 2; pass++) {
        int best = 0, most = 0, by_units = units ^ pass;
        for (int p = 1; p < nplayers(); p++) {
            ee_player *o = player_at(p);
            int k = by_units ? t->units[p] : t->bld[p];
            if (o && k > most && is_enemy(pl, o) && !is_out(o)) { most = k; best = p; }
        }
        if (best) return best;
    }
    return 0;
}
internal int nw_foe(ee_player *pl, int p) {   // p (a player's number) is an enemy of pl still in
    ee_player *o = p > 0 ? player_at(p) : NULL;
    return o && is_enemy(pl, o) && !is_out(o);
}
// a push's target: the enemy that holds the territory it goes into, else the one with the most there
internal int nw_push_target(ee_player *pl, const struct sn_push *p) {
    if (p->into < 0 || p->into >= g_ter.n) return 0;
    int o = g_ter.t[p->into].owner;
    return nw_foe(pl, o) ? o : nw_enemy_in(pl, (unsigned)p->into, 0);
}
internal void nw_log(unsigned idx, uint32_t now, const char *what, int r, int them) {
    if (g_log) log_locked("# news: P%u at %.1f min: %s %d - to P%d\n", idx, now / 60000.0, what, r, them);
}

internal void news_tick(ee_player *pl, unsigned idx, uint32_t now) {
    struct nw_st *z = &g_nw[idx & 15];
    if (!ter_ready() || !g_ter.n) return;
    if (!g_nw_facts_t || now < g_nw_facts_t || now - g_nw_facts_t >= NW_EVERY) { ter_facts(); g_nw_facts_t = now ? now : 1; }
    int me = (int)(idx & 15), n = g_ter.n < NW_T ? g_ter.n : NW_T;
    struct sn_st *sz = &g_sn[idx & 15];
    if (!z->ready || z->version != g_ter.version || z->n != n) {   // a new game, a loaded one, the map split anew: what holds now
        memset(z, 0, sizeof *z);
        z->ready = 1; z->version = g_ter.version; z->n = n; z->facts_t = g_nw_facts_t;
        for (int r = 0; r < n; r++) z->owner[r] = g_ter.t[r].owner;
        for (int k = 0; k < SN_PUSHES; k++) { z->push_t[k] = sz->push[k].start_t; z->landed[k] = 1; }
        return;
    }
    // THE PUSHES: one set out (a slot's start not told yet), one across the water landed
    for (int k = 0; k < SN_PUSHES; k++) {
        const struct sn_push *p = &sz->push[k];
        if (!p->on) continue;
        if (p->start_t != z->push_t[k]) {
            z->push_t[k] = p->start_t; z->landed[k] = 0;
            int e = nw_push_target(pl, p);
            z->push_to[k] = (int8_t)e;
            if (!e) { g_nw_said[me][5]++; continue; }   // into ground nobody holds or stands on that it knows of
            g_nw_said[me][0]++;
            nw_log(idx, now, "a push sets out into territory", p->into, e);
            persona_attack_on(pl, player_at(e));
        }
        if (p->across && !z->landed[k] && p->over >= NW_LANDED) {
            z->landed[k] = 1;
            if (!nw_foe(pl, z->push_to[k])) continue;
            g_nw_said[me][1]++;
            nw_log(idx, now, "a push lands in territory", p->into, z->push_to[k]);
            persona_say(pl, L_LANDING, player_at(z->push_to[k]), NULL);
        }
    }
    // THE TERRITORIES: with each new look at the facts
    if (z->facts_t == g_nw_facts_t) return;
    z->facts_t = g_nw_facts_t;
    const struct tv_st *v = &g_tv[idx & 15];
    for (int r = 0; r < n; r++) {
        if (g_ter.t[r].water) continue;
        int o = g_ter.t[r].owner, was = z->owner[r];
        z->owner[r] = (int8_t)o;
        if (o != me && nw_foe(pl, o)) { z->was[r] = (int8_t)o; z->was_t[r] = now ? now : 1; }
        if (o == me && was != me) {   // THE TAKEN GROUND
            int e = nw_foe(pl, was) ? was : z->was[r] && now >= z->was_t[r] && now - z->was_t[r] < NW_TAKEN_MEMORY && nw_foe(pl, z->was[r]) ? z->was[r] : 0;
            z->was[r] = 0;
            if (e) { g_nw_said[me][2]++; nw_log(idx, now, "taken: territory", r, e); persona_say(pl, L_TERRITORY_TAKEN, player_at(e), NULL); }
        } else if (was == me && o != me) {   // lost
            int e = nw_foe(pl, o) ? o : nw_enemy_in(pl, (unsigned)r, 1);
            z->unsafe[r] = 0; z->att[r] = 0;
            if (e) { g_nw_said[me][3]++; nw_log(idx, now, "lost: territory", r, e); persona_say(pl, L_TERRITORY_LOST, player_at(e), NULL); }
        }
        // THE HELD GROUND: being lost (its own view), then safe again with nobody of the enemy's left there
        int unsafe = r < v->n && v->t && v->t[r].state == TV_UNSAFE;
        if (unsafe && o == me) {
            int e = nw_enemy_in(pl, (unsigned)r, 1);
            if (e) z->att[r] = (int8_t)e;
            z->unsafe[r] = 1;
        } else if (z->unsafe[r] && !unsafe) {
            int e = z->att[r];
            z->unsafe[r] = 0; z->att[r] = 0;
            if (o == me && r < v->n && v->t && !v->t[r].foe && nw_foe(pl, e)) {
                g_nw_said[me][4]++;
                nw_log(idx, now, "held: territory", r, e);
                persona_say(pl, L_TERRITORY_HELD, player_at(e), NULL);
            }
        }
    }
    if (g_log && g_autotest && (!z->log_t || now - z->log_t >= 600000)) {   // every 10 minutes: what it had to say so far
        z->log_t = now ? now : 1;
        log_locked("# news: P%u at %.1f min: so far %u attacks, %u landings, %u territories taken, %u lost, %u held, %u pushes at nobody\n", idx, now / 60000.0,
                   g_nw_said[me][0], g_nw_said[me][1], g_nw_said[me][2], g_nw_said[me][3], g_nw_said[me][4], g_nw_said[me][5]);
    }
}
internal void news_new_game(void) { memset(g_nw, 0, sizeof g_nw); memset(g_nw_said, 0, sizeof g_nw_said); g_nw_facts_t = 0; }
