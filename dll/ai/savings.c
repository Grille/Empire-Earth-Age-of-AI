// THE SAVINGS ([ai_base] savings, off until measured; [autotest] v_savings; the player 2026-11-20, after missile subs
// - 600 gold + 600 iron - were trained by 1 AI in 8: "a bit more expensive order shouldn't need to be priority to get
// through, that points at a potential larger problem", then "build it as a general system from the beginning"). The
// spenders gave up on whatever costs more than they hold at once: the navy's mix bought the next role it could pay now
// out of the same money (and skipped a type refused for 2 minutes), the army's SAVING FIRST substituted after 90 s
// whatever the money did - frigates and battleships spent what a missile sub was being saved for, a soldier needing
// more than 90 s of income came only when the stock happened to hold it.
// A SAVING: one per pool - SV_ARMY the soldiers' orders (their military account), SV_NAVY the warships' (the navy
// account) -, the unit its spender wants that the budget refuses and that it can train. While it lasts its whole cost -
// per resource it needs, the others stay free - is held from that pool's other orders: in the stock as a row of the
// priority list ("savings", priority.c; apart: the warships that see the stock past the others see it too) and in the
// pool's account (econ.c train_budget, navy.c's mix); the saved type itself sees all. The stock is one: a saving holds
// it from every military order, the other pool's too (held from the warships only, a missile sub's saving stalled at
// 95-98% while the soldiers - an account of 19 000 - spent the gold and iron: stock G336 I204). It ends:
//   - bought (sv_bought: an order of it got through)
//   - no longer asked within SV_ASKED (its spender wants another, or nothing)
//   - stalled: what it has of its cost (the spender's measure, 0..1) has not grown in SV_STALL (a resource with no income) -
//     or not at all in SV_OBSERVE since it began (the first test: the army's savings begun at 0% held its money 2 minutes)
//   - out of reach: at the rate it has grown since it began, not there within SV_HORIZON (looked at from SV_OBSERVE on)
//   - urgent: its spender needs something at once (the army: enemy soldiers in its base)
// A type given up is not saved for again for SV_AGAIN: its spender's own way meanwhile (the next payable, a substitute).
#define SV_ASKED 30000
#define SV_STALL 120000
#define SV_HORIZON 600000   // 5 minutes gave up on a missile sub growing 11% a minute at 29% (a player saves that long for one)
#define SV_OBSERVE 90000
#define SV_AGAIN 180000
internal const char *const k_svg[SV_N] = { "army", "navy" };
struct sv_entry { int type1; uint32_t since, asked_t, best_t; float have0, best; };   // type1: the type + 1, 0 none
internal struct sv_st {
    struct sv_entry e[SV_N];
    int gave1[SV_N]; uint32_t gave_t[SV_N];
    unsigned started[SV_N], bought[SV_N], stalled[SV_N], reach[SV_N], urgent[SV_N], dropped[SV_N];
    uint32_t log_t;
} g_sv[16];
internal int g_savings = 0, g_v_savings = -1;
#define SAVINGS(idx) PV(idx, g_v_savings, g_savings)
internal int sv_on(unsigned idx) { return SAVINGS(idx & 15) != 0; }

internal int sv_live(const struct sv_entry *e, uint32_t now) { return e->type1 && now >= e->asked_t && now - e->asked_t < SV_ASKED; }
internal void sv_end(unsigned idx, int g, const char *why, uint32_t now) {
    struct sv_entry *e = &g_sv[idx & 15].e[g];
    if (g_log && e->type1)
        log_locked("# savings: P%u at %.1f min: the %s's saving for type %d ends - %s after %.1f min, %.0f%% of its cost there\n",
                   idx & 15, now / 60000.0, k_svg[g], e->type1 - 1, why, (now - e->since) / 60000.0, 100.0 * e->best);
    e->type1 = 0;
}
// the spender of pool g wants type and has `have` of its cost (0..1, its own measure); urgent: it needs something now.
// 1: saving for it - wait (the pool's other orders leave its cost alone); 0: not - the spender's own way
internal int sv_want(unsigned idx, int g, int type, float have, int urgent) {
    idx &= 15;
    if (!SAVINGS(idx) || g < 0 || g >= SV_N || type < 0) return 0;
    struct sv_st *s = &g_sv[idx];
    struct sv_entry *e = &s->e[g];
    uint32_t now = game_time();
    if (e->type1 && (now < e->since || now < e->asked_t)) memset(e, 0, sizeof *e);   // a game before
    if (e->type1 && e->type1 != type + 1) {
        if (sv_live(e, now)) return 0;   // another type saved for
        s->dropped[g]++; sv_end(idx, g, "no longer asked", now);
    }
    if (!e->type1) {
        if ((s->gave1[g] == type + 1 && now >= s->gave_t[g] && now - s->gave_t[g] < SV_AGAIN) || urgent) return 0;
        memset(e, 0, sizeof *e);
        e->type1 = type + 1; e->since = e->best_t = now ? now : 1; e->have0 = e->best = have;
        s->started[g]++;
        if (g_log) log_locked("# savings: P%u at %.1f min: the %s saves for type %d - %.0f%% of its cost there\n", idx, now / 60000.0, k_svg[g], type, 100.0 * have);
    }
    e->asked_t = now ? now : 1;
    if (have > e->best + 0.01f) { e->best = have; e->best_t = now; }
    const char *why = NULL;
    if (urgent) { why = "needed something at once"; s->urgent[g]++; }
    else if (now - e->best_t >= SV_STALL || (now - e->since >= SV_OBSERVE && e->have0 < 0.05f && e->best <= e->have0 + 0.01f)) { why = "stalled"; s->stalled[g]++; }   // or, begun at next to nothing, nothing at all since
    else if (now - e->since >= SV_OBSERVE && e->best > e->have0 && e->best < 1.0f &&
             (1.0f - e->best) * (float)(now - e->since) / (e->best - e->have0) > (float)SV_HORIZON) { why = "out of reach"; s->reach[g]++; }
    if (why) {
        s->gave1[g] = type + 1; s->gave_t[g] = now ? now : 1;
        sv_end(idx, g, why, now);
        return 0;
    }
    return 1;
}
// an order of type got through: its saving is done
internal void sv_bought(unsigned idx, int type) {
    idx &= 15;
    for (int g = 0; g < SV_N; g++)
        if (g_sv[idx].e[g].type1 == type + 1) { g_sv[idx].bought[g]++; sv_end(idx, g, "bought", game_time()); }
}
// the type pool g saves for now (-1 none) - its spender asks for it while it lasts (navy.c's mix: its first choice shifted
// as ships came and went, and the missile subs' savings ended "no longer asked" after half a minute)
internal int sv_type(unsigned idx, int g) {
    const struct sv_entry *e = &g_sv[idx & 15].e[g];
    return g >= 0 && g < SV_N && sv_on(idx) && sv_live(e, game_time()) ? e->type1 - 1 : -1;
}
// a type of pool g given up lately (navy.c: the mix's old skip for it)
internal int sv_gave(unsigned idx, int g, int type) {
    uint32_t now = game_time();
    const struct sv_st *s = &g_sv[idx & 15];
    return g >= 0 && g < SV_N && s->gave1[g] == type + 1 && now >= s->gave_t[g] && now - s->gave_t[g] < SV_AGAIN;
}
// what pool g's saving holds from an order of type (its own: nothing), added to held
internal void sv_held(ee_player *pl, unsigned idx, int g, int type, int held[7]) {
    idx &= 15;
    const struct sv_entry *e = &g_sv[idx].e[g];
    if (!SAVINGS(idx) || !sv_live(e, game_time()) || e->type1 == type + 1) return;
    for (int r = 2; r < 7; r++) held[r] += unit_cost(pl, e->type1 - 1, r);
}
// the priority list's row "savings": the stock a soldier's (warship's) order leaves to its pool's saving
internal void sv_hold(ee_player *pl, unsigned spender, int r[7]) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    if (!(spender & (S_MIL | S_SHIP))) return;
    for (int g = 0; g < SV_N; g++) sv_held(pl, idx, g, g_sv_order, r);   // every pool's: the stock is one (the soldiers spent the gold and iron a missile sub stood at 97% of)
}
internal void sv_log(unsigned idx, uint32_t now) {
    idx &= 15;
    struct sv_st *s = &g_sv[idx];
    if (!g_log || !SAVINGS(idx) || (s->log_t && now >= s->log_t && now - s->log_t < 60000)) return;
    s->log_t = now ? now : 1;
    for (int g = 0; g < SV_N; g++) {
        const struct sv_entry *e = &s->e[g];
        log_locked("# savings: P%u at %.1f min: %s - %s%d (%.0f%%) | so far started %u, bought %u, stalled %u, out of reach %u, urgent %u, no longer asked %u\n",
                   idx, now / 60000.0, k_svg[g], sv_live(e, now) ? "saving for type " : "none ", sv_live(e, now) ? e->type1 - 1 : 0,
                   100.0 * e->best, s->started[g], s->bought[g], s->stalled[g], s->reach[g], s->urgent[g], s->dropped[g]);
    }
}
internal void savings_install(const char *autotest_ini) {
    g_savings = g_set_ai[1].savings;
    if (autotest_ini) g_v_savings = ini_int("autotest", "v_savings", -1, autotest_ini);
    if (g_log) fprintf(g_log, "# savings: %d (variant %d)\n", g_savings, g_v_savings);
}
internal void savings_new_game(void) { memset(g_sv, 0, sizeof g_sv); }
