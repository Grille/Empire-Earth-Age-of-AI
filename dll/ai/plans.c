// THE STRATEGY PLANS ([ai_base] strategy_plans, [autotest] v_strategy_plans): what an AI of the mod's puts its force
// into, picked at its start and changed on what the fighting shows. The player 2026-11-22: "the game is a lot of picking
// a strategie and adapting it based on the needs, the ai is fairly static in the way it aproches the game, every ai
// attempting the same style of play"; then "there isnt a 'strongest' default right now humans quickly lertn what the ai
// does and plan against it, giving the ai difrent strategy plans makes it less predictable and might be able to throw
// a human player off". Step 3 of the plans (step 1 returns.c, step 2 the air's worth, airguard.c).
// A plan is a factor on each lever (settings.c THE LEANINGS: air_force, navy, early_anti_air, pressure). The factor a
// lever is played by (plan_f) is the plan's times the personality's leaning - 0% its k_lean_span lo, 50% 1, 100% its
// hi - within the lever's k_lever_bound. The readers: airguard.c the air force's share; navy.c the navy's share, its most and the
// fleet's size; defence.c STRONG ANTI-AIR the share of the threat built before any aircraft is seen; econ.c and metal.c
// the army's share of the income (the start's and the running one).
// THE PICK: at its first update once navy.c knows its waters, drawn from the decisions' generator (game/random.c: every
// PC alike), each plan weighed by its k_plans weight, by the leanings - one above 50% raises the plans that put more
// into its lever and lowers those that put less (lean_pull: 0% a quarter, 100% 2.5 times) - and by the evidence. Sea
// power only where the water matters to it (navy.c: PL_SEA_MIN of its enemies across the water, or seas that want
// PL_SEA_MIN of a whole fleet - on Continental, where its seas wanted 5 warships, 8% of one, it gave up air and anti-air
// for nothing: workers -30 at minute 60 against the AIs without plans).
// THE EVIDENCE: what a lever's kinds of force brought back (returns.c rt_worth_of: killed for lost, the last 10
// minutes weighed most, a prior on both sides) - air_force its fighters, bombers and helicopters, navy its warships,
// early_anti_air its anti-air (units and buildings; not the towers, which kill raiders all game), pressure its land
// soldiers. A plan's fitness: the returns of the levers it stands on (k_plans' on). Every PL_EVERY from PL_FIRST,
// PL_DWELL after the last pick: a plan below PL_FAIL, or one that no longer fits (sea power where the water stopped
// mattering), is picked again among the others by weight x leanings x fitness; one another beats by PL_BETTER gives
// way to that one (a draw there took the reason's plan seldom: a plan the next 10 minutes for no reason of its own).
// "# plan:" at each pick, with the fitness of all.
// A game loaded picks again (the plan is not saved).
// MEASURED (head-to-head, B with the plans, 60 min, 10 games mirrored each, B - A per game, se): Continental World War
// I with money for bombers (air_rich) alive -0.15 (0.13), soldiers +4.9 (11.3), losses -13.5 (23.6), workers +2.1
// (20.8); Small Islands large epoch 5 alive 0, soldiers +8.6 (7.2), workers +3.4 (3.4); Continental from the start
// alive +0.10 (0.07), soldiers +7.2 (8.2), workers -4.9 (15.4) - level, no plan clearly worse than the others. Before
// PL_SEA_MIN sea power on Continental cost 50-65 workers; before the better plan was taken by itself the plans churned
// every 10 minutes, and before guarded stood on the anti-air alone (the towers counted too) the AIs most attacked
// drifted into it (workers -35). The test's draws are its game_seed's (g_pl_seed): each game its own plans.
#define PL_EVERY 60000u
#define PL_FIRST 900000u
#define PL_DWELL 600000u
#define PL_FAIL 0.6f
#define PL_BETTER 1.5f
#define PL_SEA_MIN 0.25f   // sea power: a quarter of its enemies across the water, or seas that want a quarter of a whole fleet
enum { PN_BALANCED, PN_AIR, PN_SEA, PN_GUARDED, PN_PRESSURE, PN_ECONOMY, PN_N };
internal const struct { const char *name; float weight, f[LN_N]; unsigned on; } k_plans[PN_N] = {
    //                           air    navy   anti-air army     stands on (a bit per lever)
    { "balanced",         0.5f, { 1.0f,  1.0f,  1.0f,  1.0f  }, 0 },
    { "air power",        1.0f, { 1.8f,  0.8f,  1.0f,  1.0f  }, 1u << LN_AIR },
    { "sea power",        1.0f, { 0.7f,  1.6f,  1.0f,  1.0f  }, 1u << LN_NAVY },
    { "guarded",          1.0f, { 0.9f,  1.0f,  1.8f,  0.9f  }, 1u << LN_ANTI_AIR },
    { "pressure",         1.0f, { 1.0f,  1.0f,  0.6f,  1.25f }, 1u << LN_PRESSURE },
    { "economy first",    1.0f, { 1.0f,  0.9f,  1.2f,  0.85f }, 0 },
};
internal const float k_lean_span[LN_N][2] = { { 0.4f, 1.8f }, { 0.6f, 1.5f }, { 0.2f, 2.0f }, { 0.85f, 1.15f } };
internal const float k_lever_bound[LN_N][2] = { { 0.2f, 2.5f }, { 0.5f, 2.0f }, { 0.0f, 2.0f }, { 0.75f, 1.3f } };
internal struct pl_st { int plan; uint32_t pick_t, check_t; unsigned picks; float f[LN_N]; } g_pl[16];
internal int g_plans = 0, g_v_plans = -1;
internal uint32_t g_pl_seed;   // a test's game_seed: the draws its own in each game (the decisions' generator starts alike in every test)
#define PLANS(idx) PV(idx, g_v_plans, g_plans)
internal float rt_worth_of(unsigned idx, unsigned mask);   // returns.c

internal float plan_f(unsigned idx, int lever) {
    idx &= 15;
    return PLANS(idx) && g_pl[idx].pick_t && lever >= 0 && lever < LN_N ? g_pl[idx].f[lever] : 1.0f;
}
// a leaning's own factor on its lever: 0% lo, 50% 1, 100% hi
internal float lean_factor(unsigned idx, int t) {
    float l = lean_of(idx, t), lo = k_lean_span[t][0], hi = k_lean_span[t][1];
    return l <= 0.5f ? lo + (1.0f - lo) * 2.0f * l : 1.0f + (hi - 1.0f) * 2.0f * (l - 0.5f);
}
// a leaning's pull on the plans that put more into its lever (and against those that put less)
internal float lean_pull(unsigned idx, int t) {
    float l = lean_of(idx, t);
    return l <= 0.5f ? 0.25f + 1.5f * l : 1.0f + 3.0f * (l - 0.5f);
}
internal float plan_evidence(unsigned idx, int lever) {
    switch (lever) {
    case LN_AIR: return rt_worth_of(idx, 1u << RK_FIGHTER | 1u << RK_BOMBER | 1u << RK_HELI);
    case LN_NAVY: return rt_worth_of(idx, 1u << RK_NAVY);
    case LN_ANTI_AIR: return rt_worth_of(idx, 1u << RK_LAND_AA);
    default: return rt_worth_of(idx, 1u << RK_LAND);
    }
}
internal int plan_fits(unsigned idx, int p) {   // sea power where the water matters to it
    return p != PN_SEA || (g_nv_map[idx & 15].done && (g_nv_map[idx & 15].strength >= PL_SEA_MIN || navy_strength(idx) >= PL_SEA_MIN));
}
internal float plan_fitness(unsigned idx, int p) {
    float f = 1.0f;
    for (int l = 0; l < LN_N; l++) if (k_plans[p].on >> l & 1) f *= plan_evidence(idx, l);
    return f;
}
internal float plan_weight(unsigned idx, int p) {
    float w = k_plans[p].weight;
    for (int l = 0; l < LN_N; l++) {
        float pull = lean_pull(idx, l);
        if (k_plans[p].f[l] > 1.0f) w *= pull;
        else if (k_plans[p].f[l] < 1.0f) w /= pull;
    }
    return w;
}
internal void plan_pick(unsigned idx, uint32_t now, int not_this, int this_one, const char *why) {   // this_one >= 0: no draw
    struct pl_st *s = &g_pl[idx];
    float w[PN_N], fit[PN_N], sum = 0;
    for (int p = 0; p < PN_N; p++) {
        fit[p] = plan_fitness(idx, p);
        w[p] = p == not_this || !plan_fits(idx, p) ? 0.0f : plan_weight(idx, p) * fit[p];
        sum += w[p];
    }
    int was = s->pick_t ? s->plan : -1, pick = PN_BALANCED;
    if (this_one >= 0) pick = this_one;
    else if (sum > 0) {   // a draw in 1/1000ths of the sum: every PC the same
        float r = (float)((rnd_game32() ^ g_pl_seed * 2654435761u) % 1000u) / 1000.0f * sum;
        for (pick = 0; pick < PN_N - 1 && (w[pick] <= 0 || r >= w[pick]); pick++) r -= w[pick];
        if (w[pick] <= 0) pick = PN_BALANCED;
    }
    s->plan = pick; s->pick_t = now ? now : 1; s->picks++;
    for (int l = 0; l < LN_N; l++) {
        float f = k_plans[pick].f[l] * lean_factor(idx, l);
        s->f[l] = f < k_lever_bound[l][0] ? k_lever_bound[l][0] : f > k_lever_bound[l][1] ? k_lever_bound[l][1] : f;
    }
    if (!g_log) return;
    char line[512];
    line[0] = 0;
    for (int p = 0; p < PN_N; p++) out_printf(line, sizeof line, "%s %s %.2f/%.2f", p ? "," : " | weight/fitness:", k_plans[p].name, w[p], fit[p]);
    log_locked("# plan: P%u at %.1f min picks %s (%s%s%s) - levers air_force %.2f navy %.2f early_anti_air %.2f pressure %.2f | leanings %.0f%% %.0f%% %.0f%% %.0f%%%s\n",
               idx, now / 60000.0, k_plans[pick].name, why, was >= 0 ? ", was " : "", was >= 0 ? k_plans[was].name : "",
               s->f[LN_AIR], s->f[LN_NAVY], s->f[LN_ANTI_AIR], s->f[LN_PRESSURE],
               100.0f * lean_of(idx, LN_AIR), 100.0f * lean_of(idx, LN_NAVY), 100.0f * lean_of(idx, LN_ANTI_AIR), 100.0f * lean_of(idx, LN_PRESSURE), line);
}
internal void plans_tick(ee_player *pl, uint32_t now) {
    unsigned idx = (unsigned)player_get_index(pl) & 15;
    struct pl_st *s = &g_pl[idx];
    if (now < s->pick_t || now < s->check_t) memset(s, 0, sizeof *s);   // a new game, or one loaded
    if (!PLANS(idx)) return;
    if (!s->pick_t) {   // the start's: once navy.c knows its waters (it looks from 10 s on)
        if (!g_nv_map[idx].done && now < 60000) return;
        plan_pick(idx, now, -1, -1, "the start");
        s->check_t = now ? now : 1;
        return;
    }
    if (now - s->check_t < PL_EVERY) return;
    s->check_t = now ? now : 1;
    if (now < PL_FIRST || now - s->pick_t < PL_DWELL) return;
    float cur = plan_fitness(idx, s->plan), best = 0;
    int bp = -1;
    for (int p = 0; p < PN_N; p++) {
        float f = p == s->plan || !plan_fits(idx, p) ? 0.0f : plan_fitness(idx, p);
        if (f > best) { best = f; bp = p; }
    }
    static char why[96];
    int to = -1;
    if (!plan_fits(idx, s->plan)) lstrcpyA(why, "it no longer fits");
    else if (cur < PL_FAIL) snprintf(why, sizeof why, "its returns %.2f", cur);
    else if (bp >= 0 && best > cur * PL_BETTER) { snprintf(why, sizeof why, "%s returns %.2f against its %.2f", k_plans[bp].name, best, cur); to = bp; }
    else return;
    plan_pick(idx, now, s->plan, to, why);
}
internal void plans_install(const char *autotest_ini) {
    g_plans = g_set_ai[1].strategy_plans;
    if (autotest_ini) g_v_plans = ini_int("autotest", "v_strategy_plans", -1, autotest_ini);
    g_pl_seed = autotest_ini ? (uint32_t)ini_int("autotest", "game_seed", 0, autotest_ini) : 0;
    if (g_log) fprintf(g_log, "# strategy plans: %d (variant %d)\n", g_plans, g_v_plans);
}
