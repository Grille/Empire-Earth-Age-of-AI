// THE CONVOYS ([ai_base] transport_lifeboat, transport_escort; the game's computer player off) - the player 2026-11-17:
// "the ferry check shouldt refuse because of enemy activety, this could easily lock the ai from reaching other islands,
// but it should escort its transport and scout ahed with a ship to assure the ships arive safely", and "ships shoul
// emergency land, when sunk all unit onboard are lost, once taking demage and seeing no chance of escape it should
// straigline to the nearst beach and unload to atleast save the units".
// LIFEBOAT (every CY_LOOK): a transport of its afloat with someone aboard - any manager's: the ferry, the outposts',
// the rescue, a landing's - whose hit points went down within CY_WIN (CY_HIT of them) and still go down: at that loss it
// sinks in hit points / rate seconds, and it is where it goes (its unload's point, else its move's) in distance / its
// speed. When it would not make it (CY_MARGIN), the nearest free beach of any land by its sea within CY_BEACH_R -
// nearer than where it goes: straight there and unload (landing.c ld_unload). Its manager stands back meanwhile
// (convoy_lifeboat; the unload is given again while another order took its place) until it is empty or CY_LIFE has
// passed; then the passengers are the others' where they landed (the stations count a soldier where it stands, work.c
// gives a citizen the territory it stands in) and the manager ends the trip (convoy_beached).
// ESCORT: a manager's transport - the ferry's, the outposts' ferry's and its scouting, the rescue's - asks each look
// from the start of its trip, on its way to the boarding shore, boarding and crossing (convoy_escort: the crossing's
// two ends, whether it is loaded at sea; renewed within CY_WISH); squadrons.c sends one (ESCORT there: the scouts sweep
// the way before, at sea ahead of it and THE SCREEN). The answer: 0 none to be had, 1 on its way, 2 with it. Nobody
// waits for it: a transport's best defence is getting there before it is sunk (the player 2026-11-18). The outposts'
// ferry called a trip off when the sea on the way was held (landing.c ld_sea_risk), and didn't scout an island then:
// no more with the escort on.
// "# convoy:" lines: each lifeboat, each minute the counts (sunk escorted: loaded and sunk with its escort with it).
// MEASURED 2026-11-17, head-to-head (Small Islands large from epoch 9, 4 AIs, 60 min, seeds 1-12 mirrored, 24 games;
// squadrons on for all): the LIFEBOAT alone - units lost aboard sunk transports 5.0 per AI against 13.3, loaded
// transports sunk without one 0.25 against 1.29 (1.35 lifeboats a game, 9.6 put off, 0.21 sunk on the way to the
// beach); B - A per game losses -7.8 (se 4.0), soldiers +5.7 (5.1), kills -3.8 (3.4), workers -3.5 (2.0). On. The
// ESCORT alone - lost aboard 11.7 against 12.3, kills -4.3 (2.8), losses -1.3 (4.5), soldiers +7.1 (5.1); the old
// refusals 17-28 in those games, a crossing waited for its escort once: no gain shown - off. Both (and epoch 11, 12
// games): lost aboard 8.0 against 13.9 and 6.8 against 10.8. That first escort came when the transport was loaded
// and its scouts ran from what they didn't beat; the escort now (asked for from the start, the sweep, nobody waiting,
// THE SCREEN - squadrons.c), 2026-11-18, the same games with the lifeboat on for all, from epoch 9 and from 5 (24 each):
// lost aboard 4.5 against 6.4 and 5.4 against 7.8, crossings as fast (at sea 73 s against 70 s, 71 against 63; loading
// 68 against 69, 65 against 63), about 2.5 screens an AI a game and no warship lost screening; B - A soldiers -5.1
// (se 3.8) and -3.2 (4.5), kills -3.0 (3.2) and -2.2 (4.3): the gain within noise - off.
#define CY_LOOK 1000
#define CY_WIN 6000        // the hit points' loss over this
#define CY_MARGIN 1.25f    // it makes it: sunk later than this x the time to where it goes (+ 2 s)
#define CY_HIT 0.08f       // a loss of this share of its hit points in the window at least (one stray shot turned a whole one)
#define CY_BEACH_R 40      // a beach searched within this many tiles
#define CY_LIFE 60000      // a lifeboat ends after this, ashore or not
#define CY_SHIPS 24
internal struct cy_st {
    struct cy_ship { ee_unit *u; int id, hp[8], ns, seen, life, aboard; uint32_t t[8], life_t; ivec2 beach; } s[CY_SHIPS]; int n;
    struct cy_wish w[CY_WISHES]; int nw;
    struct { ee_unit *u; int id; uint32_t t; } done[4]; int di;   // put off by a lifeboat lately (convoy_beached)
    uint32_t t, log_t;
    unsigned lifeboats, saved, sunk_life, sunk_loaded, lost_aboard, escorts, sunk_esc;
} g_cy[16];
internal PRINTF_LIKE(3, 4) void cy_say(unsigned idx, uint32_t now, const char *fmt, ...) {
    if (!g_log) return;
    char buf[400];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    log_locked("# convoy: P%u at %.1f min: %s\n", idx, now / 60000.0, buf);
}
internal int convoy_lifeboat(ee_player *pl, ee_unit *sh) {
    struct cy_st *c = &g_cy[(unsigned)player_get_index(pl) & 15];
    for (int k = 0; sh && k < c->n; k++) if (c->s[k].u == sh && c->s[k].life) return c->s[k].id == unit_get_serial(sh);
    return 0;
}
internal int convoy_beached(unsigned idx, ee_unit *sh, uint32_t since) {   // once, for a lifeboat since then (the trip's start)
    struct cy_st *c = &g_cy[idx & 15];
    uint32_t now = game_time();
    for (int k = 0; sh && k < 4; k++)
        if (c->done[k].u == sh && c->done[k].id == unit_get_serial(sh) && c->done[k].t >= since && now >= c->done[k].t && now - c->done[k].t < CY_LIFE) {
            c->done[k].u = NULL;
            return 1;
        }
    return 0;
}
internal int convoy_escort(unsigned idx, ee_unit *sh, ivec2 from, ivec2 dest, int sea, uint32_t now) {
    struct cy_st *c = &g_cy[idx & 15];
    if (!CONVOY_ESCORT(idx) || !sh) return 0;
    int id = unit_get_serial(sh), k = 0;
    while (k < c->nw && !(c->w[k].u == sh && c->w[k].id == id)) k++;
    if (k == c->nw) {   // a new one: a free place, else the stalest
        for (int j = 0; j < c->nw && k == c->nw; j++) if (now - c->w[j].t >= CY_WISH) k = j;
        if (k == c->nw) { if (c->nw == CY_WISHES) return 0; c->nw++; }
        c->w[k].u = sh; c->w[k].id = id; c->w[k].state = 0; c->escorts++;
    }
    c->w[k].from = from; c->w[k].dest = dest; c->w[k].sea = sea; c->w[k].t = now ? now : 1;
    return c->w[k].state;
}
// the nearest free land tile by the sea oc within CY_BEACH_R of p (rings outwards)
internal int cy_beach(ivec2 p, int oc, ivec2 *out) {
    static const signed char nb[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
    for (int d = 1; d <= CY_BEACH_R; d++)
        for (int dx = -d; dx <= d; dx++)
            for (int dy = -d; dy <= d; dy += (abs(dx) == d ? 1 : 2 * d)) {
                ivec2 q = iv2(p.x + dx, p.y + dy);
                if (!area_id(q, 2) || area_id(q, 1)) continue;
                int sea = 0;
                for (int k = 0; k < 4 && !sea; k++) sea = (int)area_id(iv2(q.x + nb[k][0], q.y + nb[k][1]), 1) == oc;
                if (sea && ld_tile_free(q)) { *out = q; return 1; }
            }
    return 0;
}
// LIFEBOAT: one look at a loaded transport - 1 when it turned for a beach
internal int cy_decide(unsigned idx, struct cy_st *c, struct cy_ship *e, int aboard, uint32_t now) {
    ee_unit *u = e->u;
    int hp = unit_get_hp(u), top = -1, recent = -1;
    uint32_t top_t = 0;
    for (int j = 0; j < 8 && j < e->ns; j++) {
        if (!e->t[j] || now - e->t[j] > CY_WIN) continue;
        if (e->hp[j] > top) { top = e->hp[j]; top_t = e->t[j]; }
        if (now - e->t[j] >= 2000 && now - e->t[j] <= 3500) recent = e->hp[j];
    }
    if (top <= hp || recent <= hp || now - top_t < 1500 || (float)(top - hp) < CY_HIT * fl_hp_max(u)) return 0;   // not losing hit points now, or a stray hit
    float rate = (float)(top - hp) * 1000.0f / (float)(now - top_t), die = (float)hp / rate, speed = st_speed(u);
    if (speed <= 0.1f) speed = 0.1f;
    ee_goal *g = unit_get_goal(u);
    ivec2 at = unit_get_tile(u), dest = at;
    int has = 0;
    if (g && goal_is_unload(g)) { dest = iv2(goal_get_unload_x(g), goal_get_unload_y(g)); has = 1; }
    else if (g && goal_is_location(g)) { dest = iv2((int)goal_get_dest_x(g), (int)goal_get_dest_y(g)); has = 1; }
    float to_dest = has ? sqrtf((float)ivec2_dist2(at, dest)) : 1e6f;
    if (die >= to_dest / speed * CY_MARGIN + 2.0f) return 0;   // it makes it
    ivec2 b;
    if (!cy_beach(at, fl_ship_ocean(u), &b)) return 0;
    float to_beach = sqrtf((float)ivec2_dist2(at, b));
    if (to_beach >= to_dest) return 0;   // where it goes is no farther
    e->life = 1; e->beach = b; e->life_t = now ? now : 1; e->aboard = aboard; c->lifeboats++;
    unit_goal(u, ld_unload(u, b), 0, 0x1F40);
    cy_say(idx, now, "transport at %d,%d with %d aboard under fire - %d of %d hit points, losing %.0f a second: sunk in %.0f s, %s - to the beach at %d,%d (%.0f s)",
           at.x, at.y, aboard, hp, (int)fl_hp_max(u), (double)rate, (double)die, has ? "where it goes too far" : "going nowhere", b.x, b.y, (double)(to_beach / speed));
    return 1;
}
internal void convoy_tick(ee_player *pl, unsigned idx, uint32_t now) {
    struct cy_st *c = &g_cy[idx];
    if (now < c->t) memset(c, 0, sizeof *c);   // a new game
    if (c->t && now - c->t < CY_LOOK) return;
    c->t = now ? now : 1;
    int life = LIFEBOAT(idx);
    for (int k = 0; k < c->n; k++) c->s[k].seen = 0;
    for (struct grid_it it = grid_it_all(1); grid_it_next(&it); ) {
        ee_unit *u = it.u;
        if (unit_get_owner(u) != pl || unit_get_hp(u) <= 0 || unit_get_x(u) < 0 || !is_transport(unit_type_get_id(it.td)) || ld_is_air_transport(it.td)) continue;
        int id = unit_get_serial(u), k = 0;
        while (k < c->n && !(c->s[k].u == u && c->s[k].id == id)) k++;
        if (k == c->n) {
            if (k == CY_SHIPS) continue;
            c->n++;
            memset(&c->s[k], 0, sizeof c->s[k]);
            c->s[k].u = u; c->s[k].id = id;
        }
        struct cy_ship *e = &c->s[k];
        e->seen = 1;
        e->hp[e->ns & 7] = unit_get_hp(u); e->t[e->ns & 7] = now ? now : 1; e->ns++;
        int aboard = unit_count_inside(u);
        if (e->life) {
            if (aboard <= 0) {
                c->saved += (unsigned)e->aboard; e->life = 0;
                c->done[c->di].u = u; c->done[c->di].id = id; c->done[c->di].t = now ? now : 1; c->di = (c->di + 1) & 3;
                cy_say(idx, now, "transport put %d off on the beach at %d,%d", e->aboard, e->beach.x, e->beach.y);
            } else if (now - e->life_t >= CY_LIFE) {
                e->life = 0;
                cy_say(idx, now, "transport not unloaded in %d s at the beach at %d,%d - given back with %d aboard", CY_LIFE / 1000, e->beach.x, e->beach.y, aboard);
            } else {
                ee_goal *g = unit_get_goal(u);
                if (!g || !goal_is_unload(g) || abs(goal_get_unload_x(g) - e->beach.x) > 3 || abs(goal_get_unload_y(g) - e->beach.y) > 3)
                    unit_goal(u, ld_unload(u, e->beach), 0, 0x1F40);
            }
            continue;
        }
        if (aboard > 0) e->aboard = aboard;
        else e->aboard = 0;
        if (life && aboard > 0) cy_decide(idx, c, e, aboard, now);
    }
    for (int k = 0; k < c->n; k++) {   // not seen: sunk
        if (c->s[k].seen) continue;
        if (c->s[k].aboard > 0) c->lost_aboard += (unsigned)c->s[k].aboard;
        if (c->s[k].life) { c->sunk_life++; cy_say(idx, now, "transport sunk on its way to the beach at %d,%d with %d aboard", c->s[k].beach.x, c->s[k].beach.y, c->s[k].aboard); }
        else if (c->s[k].aboard > 0) c->sunk_loaded++;
        for (int j = 0; c->s[k].aboard > 0 && j < c->nw; j++) if (c->w[j].u == c->s[k].u && c->w[j].id == c->s[k].id && c->w[j].state == 2 && now - c->w[j].t < CY_WISH) c->sunk_esc++;
        c->s[k--] = c->s[--c->n];
    }
    if (g_log && (!c->log_t || now - c->log_t >= 60000) && (c->lifeboats || c->escorts || c->sunk_loaded)) {
        c->log_t = now ? now : 1;
        cy_say(idx, now, "lifeboats %u (put off %u aboard, sunk on the way to the beach %u), loaded transports sunk otherwise %u, lost aboard %u, escorts asked for %u, sunk escorted %u",
               c->lifeboats, c->saved, c->sunk_life, c->sunk_loaded, c->lost_aboard, c->escorts, c->sunk_esc);
    }
}
internal void convoy_new_game(void) { memset(g_cy, 0, sizeof g_cy); }
