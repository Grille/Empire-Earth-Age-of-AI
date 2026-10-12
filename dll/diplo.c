// AI DIPLOMACY. The game's AI has none: EECPDiplomacyManager (ctor 0x47AB80) is an empty class, so an AI never
// changes its stance. Stances are one-way (player +0x9E4 + i*4: 0 allied, 1 enemy towards player i): a player who
// declares war on an AI (EEMSetDiplomaticStance -> 0x52D5D7) is an enemy to the AI's units only in his own eyes -
// the AI stays allied, does not fight back, and its revenge reaction (0x46FFB7: attack + "Sie wollen ein Problem?")
// waits for an enemy stance that never comes. Included by eeaimod.c after persona.c.
//
// Stance changes: 0x52D5D7 thiscall(game state 0x918300, from, to, stance) - the call the stance message makes,
// with the team bookkeeping (0x530F24 / 0x52D27F). Locked teams: [0x918E9F] (the diplomacy screen then sends none).
//
// ANSWER WAR ([all] answer_war, default 1): whoever turns hostile towards an AI that is still allied with him gets
// war back 3-6 s later: the game's stance line ("<AI> wechselt diplomatische Haltung zu Feindselig"), the
// personality's line (war_declared, or betrayed after 2+ minutes of alliance), then the revenge reaction 0x46FFB7
// thiscall(combat manager player +0xB8C, declaring player): attack on his most attractive object (0x470FF5 ->
// 0x472427). Only changes during the game count (the first look takes the stances as they are).
//
// OPINION of every AI about every player (-100..100): starts at +20 for allies, -10 for enemies; war declared on
// it -40 (betrayal -60, x pride); tribute + value / (20 + 0.8 x (100 - greed)); per minute allied +loyalty/100,
// per minute at war -pride/200, a common enemy (both at war with the strongest player) +0.5; a grudge (below 0)
// fades with forgiveness.
//
// PEACE: a player who turns allied towards an AI at war with him offers peace (in this game peace = alliance).
// 4-8 s later the AI decides (and then every 2 minutes while the offer stands, silently unless it accepts):
//   no peace within pride/20 minutes of his declaration; otherwise
//   score = opinion + 25 x log2(his power / own power) (at most +-40) + 0.4 x (50 - aggression) - 0.3 x (pride - 50)
//           + 15 if he fights the AI's strongest enemy        accept if score > 0.
// Power = sum of soldier hit points + 30 per building (scan every 10 s).
//
// AI INITIATIVE ([all] ai_diplomacy, default 1; not in scenarios/campaigns, not with locked teams), per AI every
// 60 s from minute 10:
//   alliance: at war with a leader (power > 1.5 x own) - offer an alliance to another enemy of the leader it is at
//     war with (opinion > -20): an AI decides like a peace offer (+15 common enemy); a human gets the AI's allied
//     stance at once (its units stop attacking) and 2 minutes to answer, otherwise the offer is withdrawn.
//   betrayal: allied for 10+ minutes with a player of under half its power, opinion < 30: chance per minute
//     (100 - loyalty) x aggression / 100000 - then war (betray line). Never the player it capitulated to.
//   Threat: per AI and player, that player's soldiers within 20 tiles of the AI's base, summed over the 10-s scans
//   (x0.8 each scan: ~5 x the soldiers there now). An alliance is never offered to a player whose army is in the
//   AI's base (threat 10+) - the strongest player isn't always the one doing the damage.
//
// CAPITULATION (with ai_diplomacy, from minute 10, at most every 10 minutes per AI): when the writing is on the wall -
// the attacker (highest threat) has 5+ soldiers in its base and threat 15+, the AI has lost over 10% of its peak
// buildings, and the attacker's power is at least 1.5 + pride/20 - caution/50 times its own (Khan 5.0x, a cautious
// AI 2-3x) - it offers the attacker peace and tribute: 20% + caution/4 % + (100 - greed)/4 % of each resource
// (line capitulate, {tribute} = the total). A human accepts by turning allied within 2 minutes (the AI turns allied
// at once, so its units stop fighting): the AI pays (as far as its stock still has it), says capitulate_accepted
// and never betrays him; otherwise capitulate_refused and war again. An AI conqueror decides at once: peace score
// + the tribute's worth to it (as tribute) > 0.
//
// PEACE ENDS ATTACKS: an attack keeps its target player from the launch. Whenever an AI's own stance towards a
// player turns allied (peace, alliance, capitulation) - or an attack group still marches on a non-enemy (attack.c
// route hook) - its attacks on that player are called off at the next safe point (diplo_safe_point).
//
// SITUATIONS (personality lines, persona.c): greeting, under attack (5+ enemy soldiers within 20 tiles of the
// base, when it starts), building lost, new age, an enemy a whole age ahead, winning / losing (power x2 / x0.5 against the
// strongest enemy), desperate (buildings under 40% of their peak), a player defeated (+0x467), tribute thanks.
//
// SURRENDER ([all] surrender, default 1; not in scenarios/campaigns): an AI without citizens can't rebuild - instead
// of leaving its last buildings for the player to hunt down, it resigns: no citizens for 60 s and no way to train one
// (no Town Center/Capitol, or not enough to pay a citizen), or no citizens for 3 minutes at all - unless its army
// still has half the power of its strongest enemy. The game's own resign (EEMResign::vf10 0x4BB906):
// 0x52E2B7 thiscall(game state, 0, index) - out (+0x467), its units removed (0x530C2F) - and 0x55BCEA
// thiscall(player, 2) - lost, "<name> wurde besiegt" for the others. Removing units inside the AI's own update
// would pull them from under its caller, so the resign waits for the next safe point between simulation steps
// (control.c hook_sim_receive).
#define GAME_STATE    ((void *)0x00918300)
#define FN_SET_STANCE 0x0052D5D7
#define FN_REVENGE    0x0046FFB7
#define TEAMS_LOCKED  (*(uint8_t *)0x00918E9F)
static int g_answer_war = 1, g_ai_diplomacy = 1;
static int g_diplo_seen[16];
static int g_diplo_prev[16][16];        // [ai][j]: player j's stance towards the AI at the last look
static uint32_t g_diplo_due[16][16];    // [ai][j]: answer war at this time (0 = nothing pending)
static uint32_t g_war_since[16][16];    // [ai][j]: when j declared war on the AI
static uint32_t g_ally_since[16][16];   // [ai][j]: allied (both ways) since
static uint32_t g_peace_due[16][16];    // [ai][j]: decide j's peace offer at this time
static int g_peace_refused[16][16];
static uint32_t g_offer_t[16][16];      // [ai][j]: AI offered j an alliance at (0 none), withdrawn after 2 min
static uint32_t g_offer_last[16][16];
static int g_offer_ignored[16][16];   // [ai][j]: alliance offers j let run out in a row (next offer 10/20/40 min later)
static float g_opinion[16][16];
static uint8_t g_war_betrayal[16][16];  // [ai][j]: the pending answer is to a betrayal
static unsigned g_diplo_answers[16];
static float g_pressure[16][16];        // [ai][j]: j's soldiers near the AI's base, summed over scans (x0.8 per 10 s)
static uint8_t g_offer_kind[16][16];    // [ai][j]: the pending offer to a human is 0 an alliance, 1 a capitulation
static int g_promise[16][7];            // [ai]: resources offered with a capitulation (food..iron = 2..6)
static uint8_t g_escrow[16];            // [ai]: the promise is taken out of its stock while a human decides
static int g_victory_told;               // victory over vassals (below the tribute hook)
static void victory_hint(void *pl, int ai);
static uint32_t g_capit_last[16];       // [ai]: last capitulation offer (one per 10 minutes)
static int g_lord[16];                  // [ai]: the player it capitulated to (0 none) - never betrayed
static int g_own_prev[16][16];          // [ai][j]: the AI's own stance towards j at the last look

// world scan (every 10 s): power, soldiers, buildings per player; enemy soldiers near each AI base
static struct {
    uint32_t t;
    float power[16];
    int soldiers[16], buildings[16], peak[16], close[16], epoch[16], out[16], citizens[16], centers[16];
    int close_by[16][16];   // [ai][j]: j's soldiers within 20 tiles of the AI's base
    int bx[16], by[16];
} g_ws;

static void *player_at(int i) {
    unsigned n = *(unsigned *)0x009183CC;
    return i > 0 && (unsigned)i < n && i < 16 ? ((void **)0x00918374)[i] : NULL;
}
static int nplayers(void) { unsigned n = *(unsigned *)0x009183CC; return n > 16 ? 16 : (int)n; }
static int stance_of(void *a, int b) { return FIELD(a, 0x9E4 + b * 4, int); }
static int is_out(void *p) { return !p || FIELD(p, 0x467, uint8_t); }
static int pidx(void *p) { return (int)((unsigned)FIELD(p, PL_INDEX, int) & 15); }
static int scenario_game(void) { return g_is_scenario != 0; }

static void set_stance(int from, int to, int s) {
    ((void (__thiscall *)(void *, int, int, int))FN_SET_STANCE)(GAME_STATE, from, to, s);
}

static void ws_unit(uint8_t *u, void *ctx) {
    (void)ctx;
    void *owner = FIELD(u, 0x18, void *);
    if (!owner || FIELD(u, 0x3C, int) <= 0) return;
    int o = pidx(owner);
    if (o == 0) return;   // nature (wild animals): hostile to everyone, not an attack
    uint8_t *td = FIELD(u, 0x2C, uint8_t *);
    int fam = FIELD(td, 0x4B4, int), type = FIELD(td, 0x4B8, int);
    if (is_citizen_type(type)) { g_ws.citizens[o]++; return; }
    if (type == 35 || type == 41) g_ws.centers[o]++;   // Town Center, Capitol: where citizens are trained
    if (fam == 2 || fam == 29) { g_ws.buildings[o]++; g_ws.power[o] += 30; return; }
    if (!is_soldier(td)) return;
    g_ws.soldiers[o]++;
    g_ws.power[o] += (float)FIELD(u, 0x3C, int);
    int x = FIELD(u, 0x1C, int), y = FIELD(u, 0x20, int);
    for (int a = 1; a < 16; a++) {
        void *pa = g_players[a];
        if (!pa || a == o || g_ws.bx[a] == -10000 || !stance_of(pa, o)) continue;
        int dx = x - g_ws.bx[a], dy = y - g_ws.by[a];
        if (dx * dx + dy * dy <= 400) { g_ws.close[a]++; g_ws.close_by[a][o]++; }
    }
}
static void world_scan(uint32_t now) {
    if (g_ws.t && now - g_ws.t < 10000) return;
    g_ws.t = now ? now : 1;
    for (int i = 0; i < 16; i++) {
        g_ws.power[i] = 0; g_ws.soldiers[i] = g_ws.buildings[i] = g_ws.close[i] = g_ws.citizens[i] = g_ws.centers[i] = 0;
        g_ws.bx[i] = -10000;
        void *cm = g_players[i] ? FIELD(g_players[i], 0xB90, void *) : NULL;
        uint8_t *bm = cm ? FIELD(cm, 0x3C, uint8_t *) : NULL;
        if (bm && FIELD(bm, 0x12C, int) != -10000) { g_ws.bx[i] = FIELD(bm, 0x12C, int); g_ws.by[i] = FIELD(bm, 0x130, int); }
    }
    memset(g_ws.close_by, 0, sizeof g_ws.close_by);
    grid_each(ws_unit, NULL);
    for (int a = 0; a < 16; a++)
        for (int j = 0; j < 16; j++) g_pressure[a][j] = g_pressure[a][j] * 0.8f + (float)g_ws.close_by[a][j];
    for (int i = 1; i < nplayers(); i++) {
        void *p = player_at(i);
        if (!p) continue;
        if (g_ws.buildings[i] > g_ws.peak[i]) g_ws.peak[i] = g_ws.buildings[i];
        g_ws.epoch[i] = current_epoch(p);
    }
}
static float power_of(int i) { return g_ws.power[i] > 1.0f ? g_ws.power[i] : 1.0f; }
// the strongest player the AI is at war with (or 0)
static int strongest_enemy(void *pl, int ai) {
    int best = 0;
    for (int j = 1; j < nplayers(); j++) {
        void *pj = player_at(j);
        if (!pj || j == ai || is_out(pj) || !stance_of(pl, j)) continue;
        if (!best || g_ws.power[j] > g_ws.power[best]) best = j;
    }
    return best;
}

static void opinion_add(int ai, int j, float d) {
    float v = g_opinion[ai][j] + d;
    g_opinion[ai][j] = v < -100 ? -100 : v > 100 ? 100 : v;
}

// would AI pl make peace / an alliance with player j now? (score > 0)
static float peace_score(void *pl, int ai, int j, uint32_t now) {
    if (g_war_since[ai][j] && now - g_war_since[ai][j] < (uint32_t)trait(pl, T_PRIDE) * 3000u) return -1000;
    float r = power_of(j) / power_of(ai);
    int lead = strongest_enemy(pl, ai);
    void *pj = player_at(j);
    float pw = 25.0f * log2f(r);
    if (pw > 40) pw = 40; else if (pw < -40) pw = -40;   // a much weaker player can still buy peace
    float s = g_opinion[ai][j] + pw + 0.4f * (float)(50 - trait(pl, T_AGGRESSION)) - 0.3f * (float)(trait(pl, T_PRIDE) - 50);
    if (lead && lead != j && pj && stance_of(pj, lead)) s += 15;
    return s;
}

static void make_peace(int ai, int j, uint32_t now) {
    void *pl = player_at(ai), *pj = player_at(j);
    if (!pl || !pj) return;
    if (stance_of(pl, j)) set_stance(ai, j, 0);
    chat_stance_line(pl, 0);
    g_ally_since[ai][j] = g_ally_since[j][ai] = now ? now : 1;
    g_war_since[ai][j] = g_war_since[j][ai] = 0;
    g_peace_due[ai][j] = 0;
    g_peace_refused[ai][j] = 0;
    opinion_add(ai, j, 10);
}

static void declare_war(int ai, int j, uint32_t now) {
    void *pl = player_at(ai);
    if (!pl || stance_of(pl, j)) return;
    set_stance(ai, j, 1);
    chat_stance_line(pl, 1);
    g_ally_since[ai][j] = g_ally_since[j][ai] = 0;
    g_war_since[j][ai] = now ? now : 1;   // the other side: war was declared on j
    g_offer_t[ai][j] = 0;
}

// A pending peace offer of j (allied towards the AI while the AI is at war with him)
static void peace_offer(void *pl, int ai, int j, uint32_t now) {
    void *pj = player_at(j);
    float s = peace_score(pl, ai, j, now);
    if (g_log) fprintf(g_log, "# diplomacy: P%d at %.1f min: peace offer of P%d, score %.0f (opinion %.0f, power %.0f vs %.0f)\n",
                       ai, now / 60000.0, j, s, g_opinion[ai][j], g_ws.power[j], g_ws.power[ai]);
    if (s > 0 && !TEAMS_LOCKED) {
        make_peace(ai, j, now);
        persona_say(pl, L_PEACE_ACCEPT, pj, NULL);
        return;
    }
    if (!g_peace_refused[ai][j]) persona_say(pl, L_PEACE_REFUSE, pj, NULL);
    g_peace_refused[ai][j] = 1;
    g_peace_due[ai][j] = now + 120000;
}

// AI ai's own initiative (every 60 s)
static void diplo_initiative(void *pl, int ai, uint32_t now) {
    if (!g_ai_diplomacy || TEAMS_LOCKED || scenario_game() || now < 600000) return;
    int lead = strongest_enemy(pl, ai);
    // alliance against the leader
    if (lead && g_ws.power[lead] > 1.5f * power_of(ai)) {
        int best = 0;
        float bs = -20;
        for (int j = 1; j < nplayers(); j++) {
            void *pj = player_at(j);
            if (!pj || j == ai || j == lead || is_out(pj) || !stance_of(pl, j) || !stance_of(pj, lead)) continue;
            if (g_pressure[ai][j] >= 10) continue;   // not the one whose army is in its base right now
            if (g_offer_t[ai][j] || (g_offer_last[ai][j] &&
                                     now - g_offer_last[ai][j] < (600000u << (g_offer_ignored[ai][j] < 2 ? g_offer_ignored[ai][j] : 2))))
                continue;
            if (g_opinion[ai][j] > bs) { bs = g_opinion[ai][j]; best = j; }
        }
        if (best) {
            void *pj = player_at(best);
            g_offer_last[ai][best] = now;
            persona_say(pl, L_ALLIANCE_OFFER, pj, player_at(lead));
            if (g_players[best]) {   // an AI decides at once (its turn of peace_offer)
                float s = peace_score(pj, best, ai, now) + 15;
                if (g_log) fprintf(g_log, "# diplomacy: P%d offers P%d (AI) an alliance against P%d: score %.0f\n", ai, best, lead, s);
                if (s > 0) {
                    make_peace(ai, best, now);
                    make_peace(best, ai, now);
                    persona_say(pj, L_OFFER_ACCEPTED, pl, player_at(lead));
                } else persona_say(pj, L_PEACE_REFUSE, pl, NULL);
            } else {                 // a human: the AI's stance now, his answer within 2 minutes
                set_stance(ai, best, 0);
                chat_stance_line(pl, 0);
                g_offer_t[ai][best] = now ? now : 1;
                g_offer_kind[ai][best] = 0;
                if (g_log) fprintf(g_log, "# diplomacy: P%d offers P%d (human) an alliance against P%d\n", ai, best, lead);
            }
            return;
        }
    }
    // betrayal of a weak ally
    for (int j = 1; j < nplayers(); j++) {
        void *pj = player_at(j);
        if (!pj || j == ai || is_out(pj) || stance_of(pl, j) || !g_ally_since[ai][j] || now - g_ally_since[ai][j] < 600000) continue;
        if (g_lord[ai] == j) continue;   // capitulated to j: keeps its word
        if (power_of(j) * 2 > power_of(ai) || g_opinion[ai][j] >= 30) continue;
        int chance = (100 - trait(pl, T_LOYALTY)) * trait(pl, T_AGGRESSION);   // per 100000
        if ((int)(rnd32() % 100000u) >= chance) continue;
        if (g_log) fprintf(g_log, "# diplomacy: P%d at %.1f min betrays P%d (opinion %.0f, power %.0f vs %.0f)\n",
                           ai, now / 60000.0, j, g_opinion[ai][j], g_ws.power[ai], g_ws.power[j]);
        persona_say(pl, L_BETRAY, pj, NULL);
        declare_war(ai, j, now);
        return;
    }
}

// situations worth a line (every 10 s, after the scan)
static struct {   // per AI: what it has said / seen (reset by diplo_new_game)
    int last_epoch[16], last_buildings[16], said_desperate[16], said_greeting[16], said_defeat[16], was_out[16];
    int attacked[16], was_ahead[16];
    int mood[16];   // -1 losing, 0, 1 winning (lines when it changes)
    uint32_t last_min[16], last_sit[16];
} g_sit;
static void diplo_situations(void *pl, int ai, uint32_t now) {
    if (!g_sit.said_greeting[ai] && now >= 20000 + (uint32_t)ai * 4000u) {   // at the start only, not after a load
        g_sit.said_greeting[ai] = 1;
        if (now < 90000) persona_say(pl, L_GREETING, NULL, NULL);
    }
    int e = current_epoch(pl);
    if (g_sit.last_epoch[ai] >= 0 && e > g_sit.last_epoch[ai]) persona_say(pl, L_EPOCH_UP, NULL, NULL);
    g_sit.last_epoch[ai] = e;
    if (g_ws.close[ai] >= 5 && !g_sit.attacked[ai]) persona_say(pl, L_UNDER_ATTACK, NULL, NULL);   // when it starts
    g_sit.attacked[ai] = g_ws.close[ai] >= 3;
    if (g_sit.last_buildings[ai] && g_ws.buildings[ai] < g_sit.last_buildings[ai]) persona_say(pl, L_BUILDING_LOST, NULL, NULL);
    g_sit.last_buildings[ai] = g_ws.buildings[ai];
    if (!g_sit.said_desperate[ai] && g_ws.peak[ai] >= 10 && g_ws.buildings[ai] * 10 < g_ws.peak[ai] * 4) {
        g_sit.said_desperate[ai] = 1;
        persona_say(pl, L_DESPERATE, NULL, NULL);
    }
    int lead = strongest_enemy(pl, ai);
    if (lead) {
        float r = power_of(ai) / power_of(lead);
        int m = r > 2.0f ? 1 : r < 0.5f ? -1 : (g_sit.mood[ai] == 1 && r > 1.5f) ? 1 : (g_sit.mood[ai] == -1 && r < 0.67f) ? -1 : 0;
        if (m != g_sit.mood[ai] && m) persona_say(pl, m > 0 ? L_WINNING : L_LOSING, player_at(lead), NULL);
        g_sit.mood[ai] = m;
        int ahead = g_ws.epoch[lead] > e;   // when the strongest enemy gets ahead
        if (ahead && !g_sit.was_ahead[ai]) persona_say(pl, L_ENEMY_AHEAD, player_at(lead), NULL);
        g_sit.was_ahead[ai] = ahead;
    }
    for (int j = 1; j < nplayers(); j++) {
        void *pj = player_at(j);
        int out = is_out(pj);
        if (out && !(g_sit.was_out[ai] & (1 << j)) && pj && j != ai)
            persona_say(pl, stance_of(pl, j) ? L_ENEMY_DEFEATED : L_ALLY_DEFEATED, pj, NULL);
        if (out) g_sit.was_out[ai] |= 1 << j;
    }
}

// capitulation (every 10 s, after the scan; see the header)
// a capitulation offered to a human: the tribute leaves the stock at once, so it can't be spent while he decides
// (in a game: 1454 promised, 1211 left 40 s later); back into the stock before paying or when he ignores it
static void promise_escrow(int ai, int take) {
    void *pl = player_at(ai);
    if (!pl || g_escrow[ai] == take) return;
    int *stock = (int *)((uint8_t *)pl + PL_STOCK), total = 0;
    for (int r = 2; r < 7; r++) {
        int n = g_promise[ai][r];
        if (take && n > stock[r]) n = g_promise[ai][r] = stock[r] > 0 ? stock[r] : 0;
        stock[r] += take ? -n : n;
        total += n;
    }
    g_escrow[ai] = (uint8_t)take;
    if (g_log) fprintf(g_log, "# diplomacy: P%d tribute of %d %s\n", ai, total, take ? "held back from its stock" : "back in its stock");
}
static int pay_promise(int ai, int j) {   // what was offered, as far as the stock still has it
    void *pl = player_at(ai), *pj = player_at(j);
    int total = 0;
    promise_escrow(ai, 0);
    if (pl && pj) {
        int *from = (int *)((uint8_t *)pl + PL_STOCK), *to = (int *)((uint8_t *)pj + PL_STOCK);
        for (int r = 2; r < 7; r++) {
            int n = g_promise[ai][r] < from[r] ? g_promise[ai][r] : from[r];
            if (n <= 0) continue;
            from[r] -= n; to[r] += n; total += n;
        }
    }
    memset(g_promise[ai], 0, sizeof g_promise[ai]);
    return total;
}
static void capitulate(void *pl, int ai, int a, uint32_t now) {
    void *pa = player_at(a);
    int *stock = (int *)((uint8_t *)pl + PL_STOCK), total = 0;
    float share = 0.2f + (float)trait(pl, T_CAUTION) / 400.0f + (float)(100 - trait(pl, T_GREED)) / 400.0f;
    for (int r = 2; r < 7; r++) total += g_promise[ai][r] = stock[r] > 0 ? (int)((float)stock[r] * share) : 0;
    g_capit_last[ai] = now ? now : 1;
    g_say_tribute = total;
    if (g_log) fprintf(g_log, "# diplomacy: P%d at %.1f min capitulates to P%d (%s): %d resources (%.0f%%), power %.0f vs %.0f, "
                       "%d of its soldiers at home, buildings %d of %d at the peak\n", ai, now / 60000.0, a,
                       g_players[a] ? "AI" : "human", total, share * 100, g_ws.power[ai], g_ws.power[a], g_ws.close_by[ai][a],
                       g_ws.buildings[ai], g_ws.peak[ai]);
    persona_say(pl, L_CAPITULATE, pa, NULL);
    if (g_players[a]) {   // an AI conqueror decides at once: peace score + what the tribute is worth to it
        float gain = (float)total / (20.0f + 0.8f * (float)(100 - trait(pa, T_GREED)));
        float s = peace_score(pa, a, ai, now) + gain;
        if (g_log) fprintf(g_log, "# diplomacy: P%d answers: score %.0f (tribute worth %.0f)\n", a, s, gain);
        if (s > 0) {
            make_peace(ai, a, now);
            make_peace(a, ai, now);
            pay_promise(ai, a);
            opinion_add(a, ai, gain);
            g_lord[ai] = a;
            persona_say(pa, L_PEACE_ACCEPT, pl, NULL);
        } else {
            memset(g_promise[ai], 0, sizeof g_promise[ai]);
            persona_say(pa, L_PEACE_REFUSE, pl, NULL);
        }
        return;
    }
    set_stance(ai, a, 0);   // a human: its units stop fighting; his answer (allied) within 2 minutes
    chat_stance_line(pl, 0);
    promise_escrow(ai, 1);
    g_offer_t[ai][a] = now ? now : 1;
    g_offer_kind[ai][a] = 1;
}
static void check_capitulation(void *pl, int ai, uint32_t now) {
    if (!g_ai_diplomacy || TEAMS_LOCKED || scenario_game() || now < 600000) return;
    if (g_capit_last[ai] && now - g_capit_last[ai] < 600000) return;
    int a = 0;   // the attacker: whose soldiers have been in its base the most lately
    for (int j = 1; j < nplayers(); j++) {
        void *pj = player_at(j);
        if (!pj || j == ai || is_out(pj) || !stance_of(pl, j) || !stance_of(pj, ai) || g_offer_t[ai][j]) continue;
        if (!a || g_pressure[ai][j] > g_pressure[ai][a]) a = j;
    }
    if (!a || g_ws.close_by[ai][a] < 5 || g_pressure[ai][a] < 15) return;      // its army is in the base, and stays
    if (g_ws.peak[ai] < 5 || g_ws.buildings[ai] * 10 >= g_ws.peak[ai] * 9) return;   // and it is losing buildings
    float need = 1.5f + (float)trait(pl, T_PRIDE) / 20.0f - (float)trait(pl, T_CAUTION) / 50.0f;   // pride holds out
    if (need < 1.5f) need = 1.5f;
    if (power_of(a) < need * power_of(ai)) return;
    capitulate(pl, ai, a, now);
}

// surrender (every 10 s, after the scan): queued here, carried out at the next safe point (diplo_safe_point)
static int g_surrender = 1;
static uint32_t g_no_citizens_since[16];
static volatile LONG g_resign_queue;   // bit per player
static void check_surrender(void *pl, int ai, uint32_t now) {
    if (!g_surrender || scenario_game() || (g_resign_queue & (1 << ai))) return;
    if (g_ws.citizens[ai] > 0) { g_no_citizens_since[ai] = 0; return; }
    if (!g_no_citizens_since[ai]) { g_no_citizens_since[ai] = now ? now : 1; return; }
    int *stock = (int *)((uint8_t *)pl + PL_STOCK), pay = 1;
    for (int r = 2; r < 7; r++) {
        int c = unit_cost(pl, 32, r);
        if (c > 0 && stock[r] < c) pay = 0;
    }
    int can_train = g_ws.centers[ai] > 0 && pay;
    if (now - g_no_citizens_since[ai] < (can_train ? 180000u : 60000u)) return;
    int lead = strongest_enemy(pl, ai);
    if (!lead || power_of(ai) * 2.0f >= power_of(lead)) return;   // nobody to give up to / its army can still win
    if (!persona_say(pl, L_SURRENDER, player_at(lead), NULL)) persona_say(pl, L_DEFEATED, player_at(lead), NULL);
    g_sit.said_defeat[ai] = 1;
    InterlockedOr(&g_resign_queue, 1 << ai);
    if (g_log) fprintf(g_log, "# surrender: P%d at %.1f min: no citizens for %.1f min, %s, power %.0f vs P%d %.0f\n", ai,
                       now / 60000.0, (now - g_no_citizens_since[ai]) / 60000.0,
                       can_train ? "could still train" : g_ws.centers[ai] ? "can't pay a citizen" : "no Town Center",
                       g_ws.power[ai], lead, g_ws.power[lead]);
}
// Attacks on a player who is no enemy any more: EEComputerPlayer::vf22 (0x43033E) thiscall(player, target player,
// 1) -> 0x46FD01 (combat manager) -> 0x4510F2 (attack commander cbm+0x48): every attack group whose target (+0x48) is
// that player sends its units home, hands them back and is deleted. (The game itself calls it only with 0 - freeze -
// from the scenario trigger that sets a stance.) Queued, because it deletes subgroups: at the next safe point.
static volatile LONG g_cancel_queue[16];   // [ai]: bit per target player
static void diplo_cancel_attacks(int ai, int j) {
    if (ai > 0 && ai < 16 && j > 0 && j < 16 && g_players[ai]) InterlockedOr(&g_cancel_queue[ai], 1 << j);
}
static int attack_groups(void *pl) {   // the attack commander's deque count
    uint8_t *cbm = FIELD(pl, 0xB8C, uint8_t *), *cmd = cbm ? FIELD(cbm, 0x48, uint8_t *) : NULL;
    return cmd ? FIELD(cmd, 0x14, int) : -1;
}

static void diplo_safe_point(void) {
    for (int i = 1; i < 16; i++) {
        if (!g_cancel_queue[i]) continue;
        LONG q = InterlockedExchange(&g_cancel_queue[i], 0);
        void *p = player_at(i);
        if (!p || !g_players[i] || is_out(p)) continue;
        for (int j = 1; j < 16; j++) {
            void *pj = player_at(j);
            if (!(q & (1 << j)) || !pj || stance_of(p, j)) continue;   // enemies again: the attack stays
            int before = attack_groups(p);
            breach_forget(p);
            ((void (__thiscall *)(void *, void *, int))0x0043033E)(p, pj, 1);
            if (g_log) { fprintf(g_log, "# diplomacy: P%d calls off its attacks on P%d (attack groups %d -> %d)\n", i, j, before,
                                 attack_groups(p)); fflush(g_log); }
        }
    }
    if (!g_resign_queue) return;
    LONG q = InterlockedExchange(&g_resign_queue, 0);
    for (int i = 1; i < 16; i++) {
        void *p = player_at(i);
        if (!(q & (1 << i)) || !p || is_out(p)) continue;
        ((void (__thiscall *)(void *, int, int))0x0052E2B7)(GAME_STATE, 0, i);
        ((void (__thiscall *)(void *, int))0x0055BCEA)(p, 2);
        if (g_log) { fprintf(g_log, "# surrender: P%d resigned (out %d)\n", i, FIELD(p, 0x467, uint8_t)); fflush(g_log); }
    }
}

static void diplo_tick(void *pl, unsigned ai, uint32_t now) {
    persona_assign(pl);
    if (FIELD(pl, 0x467, uint8_t)) {   // defeated: last words once
        if (!g_sit.said_defeat[ai]) { g_sit.said_defeat[ai] = 1; persona_say(pl, L_DEFEATED, NULL, NULL); }
        return;
    }
    int n = nplayers();
    for (int j = 1; j < n; j++) {
        void *pj = player_at(j);
        if (!pj || j == (int)ai) continue;
        int s = stance_of(pj, (int)ai), mine = stance_of(pl, j);
        if (!g_diplo_seen[ai]) {
            g_opinion[ai][j] = mine ? -10.0f : 20.0f;
            if (!mine && !s) g_ally_since[ai][j] = now ? now : 1;
        }
        if (g_diplo_seen[ai] && !mine && g_own_prev[ai][j]) diplo_cancel_attacks((int)ai, j);   // the AI made peace
        g_own_prev[ai][j] = mine;
        if (g_diplo_seen[ai] && s && !g_diplo_prev[ai][j]) {   // j turned hostile
            int betrayal = !mine && g_ally_since[ai][j] && now - g_ally_since[ai][j] > 120000;
            opinion_add(ai, j, (betrayal ? -60.0f : -40.0f) * (0.5f + trait(pl, T_PRIDE) / 100.0f));
            g_war_since[ai][j] = now ? now : 1;
            g_ally_since[ai][j] = 0;
            g_peace_due[ai][j] = 0;
            g_offer_t[ai][j] = 0;
            if (g_answer_war && !mine) {
                g_diplo_due[ai][j] = now + 3000 + (uint32_t)((ai * 7 + (unsigned)j * 3) % 4) * 1000;
                g_war_betrayal[ai][j] = (uint8_t)betrayal;
            }
            if (g_log) fprintf(g_log, "# diplomacy: P%u at %.1f min: P%d declared war on it%s\n", ai, now / 60000.0, j,
                               betrayal ? " (betrayal)" : "");
        }
        if (g_diplo_seen[ai] && !s && g_diplo_prev[ai][j] && mine) {   // j offers peace
            g_peace_due[ai][j] = now + 4000 + (uint32_t)((ai * 5 + (unsigned)j) % 5) * 1000;
            g_peace_refused[ai][j] = 0;
        }
        if (!s && !mine && !g_ally_since[ai][j]) g_ally_since[ai][j] = now ? now : 1;
        g_diplo_prev[ai][j] = s;
        if (g_diplo_due[ai][j] && now >= g_diplo_due[ai][j]) {
            g_diplo_due[ai][j] = 0;
            if (!s || stance_of(pl, j) || is_out(pj)) continue;   // peace again, or already at war
            set_stance((int)ai, j, 1);
            chat_stance_line(pl, 1);
            if (!persona_say(pl, g_war_betrayal[ai][j] ? L_BETRAYED : L_WAR_DECLARED, pj, NULL) && g_war_betrayal[ai][j])
                persona_say(pl, L_WAR_DECLARED, pj, NULL);
            void *cbm = FIELD(pl, 0xB8C, void *);
            if (cbm) ((void (__thiscall *)(void *, void *))FN_REVENGE)(cbm, pj);
            g_diplo_answers[ai]++;
            if (g_log) fprintf(g_log, "# diplomacy: P%u at %.1f min answers war: now enemy of P%d (stance %d)\n",
                               ai, now / 60000.0, j, stance_of(pl, j));
        }
        if (g_peace_due[ai][j] && now >= g_peace_due[ai][j]) {
            g_peace_due[ai][j] = 0;
            if (!s && stance_of(pl, j) && !is_out(pj)) peace_offer(pl, (int)ai, j, now);
        }
        if (g_offer_t[ai][j]) {   // the AI's alliance offer or capitulation to a human
            int capit = g_offer_kind[ai][j];
            if (!s) {
                g_offer_t[ai][j] = 0;
                g_ally_since[ai][j] = now ? now : 1;
                if (capit) {
                    g_say_tribute = pay_promise((int)ai, j);
                    g_lord[ai] = j;
                    g_war_since[ai][j] = g_war_since[j][ai] = 0;
                    opinion_add((int)ai, j, 20);
                    persona_say(pl, L_CAPITULATE_ACCEPTED, pj, NULL);
                    if (g_log) fprintf(g_log, "# diplomacy: P%u at %.1f min: P%d accepts the capitulation, %d resources paid\n",
                                       ai, now / 60000.0, j, g_say_tribute);
                } else persona_say(pl, L_OFFER_ACCEPTED, pj, NULL);
                g_offer_ignored[ai][j] = 0;
            } else if (now - g_offer_t[ai][j] > 120000) {
                g_offer_t[ai][j] = 0;
                if (capit) { promise_escrow((int)ai, 0); memset(g_promise[ai], 0, sizeof g_promise[ai]); }
                else g_offer_ignored[ai][j]++;
                if (capit && g_log) fprintf(g_log, "# diplomacy: P%u at %.1f min: P%d ignored the capitulation\n", ai, now / 60000.0, j);
                persona_say(pl, capit ? L_CAPITULATE_REFUSED : L_OFFER_WITHDRAWN, pj, NULL);
                set_stance((int)ai, j, 1);
                chat_stance_line(pl, 1);
            }
        }
    }
    g_diplo_seen[ai] = 1;
    // once a minute: opinions drift; the AI's own moves
    if (!g_sit.last_min[ai] || now - g_sit.last_min[ai] >= 60000) {
        int first = !g_sit.last_min[ai];
        g_sit.last_min[ai] = now ? now : 1;
        world_scan(now);
        int lead = strongest_enemy(pl, (int)ai);
        for (int j = 1; j < n && !first; j++) {
            void *pj = player_at(j);
            if (!pj || j == (int)ai || is_out(pj)) continue;
            float d = 0;
            if (!stance_of(pl, j) && !stance_of(pj, (int)ai)) d += trait(pl, T_LOYALTY) / 100.0f;
            if (stance_of(pl, j)) d -= trait(pl, T_PRIDE) / 200.0f;
            if (lead && lead != j && stance_of(pj, lead) && stance_of(pl, lead)) d += 0.5f;
            if (g_opinion[ai][j] < 0) d -= g_opinion[ai][j] * trait(pl, T_FORGIVENESS) / 2000.0f;
            opinion_add((int)ai, j, d);
        }
        if (!first) diplo_initiative(pl, (int)ai, now);
    }
    if (!g_sit.last_sit[ai] || now - g_sit.last_sit[ai] >= 10000) {
        g_sit.last_sit[ai] = now ? now : 1;
        world_scan(now);
        diplo_situations(pl, (int)ai, now);
        check_capitulation(pl, (int)ai, now);
        check_surrender(pl, (int)ai, now);
        victory_hint(pl, (int)ai);
    }
}

// a new game (the game start hook): forget everything about the last one
static void diplo_new_game(void) {
    memset(g_diplo_seen, 0, sizeof g_diplo_seen);
    memset(g_diplo_prev, 0, sizeof g_diplo_prev);
    memset(g_diplo_due, 0, sizeof g_diplo_due);
    memset(g_war_since, 0, sizeof g_war_since);
    memset(g_ally_since, 0, sizeof g_ally_since);
    memset(g_peace_due, 0, sizeof g_peace_due);
    memset(g_peace_refused, 0, sizeof g_peace_refused);
    memset(g_offer_t, 0, sizeof g_offer_t);
    memset(g_offer_last, 0, sizeof g_offer_last);
    memset(g_offer_ignored, 0, sizeof g_offer_ignored);
    memset(g_opinion, 0, sizeof g_opinion);
    memset(g_war_betrayal, 0, sizeof g_war_betrayal);
    memset(g_diplo_answers, 0, sizeof g_diplo_answers);
    memset(&g_ws, 0, sizeof g_ws);
    memset(&g_sit, 0, sizeof g_sit);
    memset(g_no_citizens_since, 0, sizeof g_no_citizens_since);
    memset(g_pressure, 0, sizeof g_pressure);
    memset(g_offer_kind, 0, sizeof g_offer_kind);
    memset(g_promise, 0, sizeof g_promise);
    memset(g_escrow, 0, sizeof g_escrow);
    g_victory_told = 0;
    memset(g_capit_last, 0, sizeof g_capit_last);
    memset(g_lord, 0, sizeof g_lord);
    memset(g_own_prev, 0, sizeof g_own_prev);
    for (int i = 0; i < 16; i++) g_cancel_queue[i] = 0;
    g_resign_queue = 0;
    for (int i = 0; i < 16; i++) g_sit.last_epoch[i] = -1;
}

// Tribute (EEMPayTribute::vf10 0x4BA6BE, vtable 0x8289C0 slot 10): sender id +0xA (0x52E6EF), receiver index
// +0x24, amounts +0x10 food, +0x14 gold, +0x18 iron, +0x1C stone, +0x20 wood.
#define VT_PAY_TRIBUTE 0x008289C0
#define FN_PAY_TRIBUTE 0x004BA6BE
static void diplo_tribute(void *to, void *from, int total) {
    if (!to || !from || to == from || !g_players[pidx(to)] || total <= 0) return;
    int ai = pidx(to), j = pidx(from);
    float gain = (float)total / (20.0f + 0.8f * (float)(100 - trait(to, T_GREED)));
    opinion_add(ai, j, gain);
    if (g_log) fprintf(g_log, "# diplomacy: P%d receives %d tribute from P%d: opinion +%.0f -> %.0f\n", ai, total, j, gain, g_opinion[ai][j]);
    persona_say(to, gain >= 5 ? L_TRIBUTE_THANKS : L_TRIBUTE_SMALL, from, NULL);
    if (!stance_of(from, ai) && stance_of(to, j)) g_peace_due[ai][j] = game_time() + 3000;   // ask again now
}
static void __fastcall hook_tribute(uint8_t *msg, void *edx) {
    (void)edx;
    int total = FIELD(msg, 0x10, int) + FIELD(msg, 0x14, int) + FIELD(msg, 0x18, int) + FIELD(msg, 0x1C, int) + FIELD(msg, 0x20, int);
    void *to = player_at(msg[0x24]);
    void *from = ((void *(__thiscall *)(void *, int))0x0052E6EF)(GAME_STATE, (int)(int8_t)msg[0xA]);
    ((void (__thiscall *)(void *))FN_PAY_TRIBUTE)(msg);
    diplo_tribute(to, from, total);
}

// VICTORY OVER VASSALS: the game ends only when everyone still in is allied with everyone else (0x531913 every step:
// one alliance group left -> its members win; the end is final, the simulation stops). Vassals at war with each
// other keep it running for ever. Once every other player still in has capitulated to the local player (and both
// sides are allied), one of them tells him; he can play on and end it whenever he types "sieg" or "victory" in the
// chat: the vassals resign (the surrender path, next safe point) and he wins alone.
// Chat: EEMInGameChat::vf10 (0x4B78C7, vtable 0x82936C slot 10), sender = the player whose +0x464 is msg+0xA,
// text = UWideString msg+0x10 (char * at +4, as every UWideString here); sent by EEUIECESendMessage::vf1 (0x5C28D6, after the cheat codes):
// 0x5C2A2F thiscall(new(0x24), UWideString *) and [0x91984C] vtable +0x18 (message, 0xFF).
#define VT_INGAME_CHAT 0x0082936C
#define FN_INGAME_CHAT 0x004B78C7
// g_victory_told: the hint was given (once a game, again after loading)
static int vassals_of(int me) {   // how many vassals, if every other player still in is one of me; else 0
    void *pm = player_at(me);
    int n = 0;
    if (!pm || me < 1 || me > 15) return 0;
    for (int j = 1; j < nplayers(); j++) {
        void *pj = player_at(j);
        if (j == me || !pj || is_out(pj)) continue;
        if (!g_players[j] || g_lord[j] != me || stance_of(pj, me) || stance_of(pm, j)) return 0;
        n++;
    }
    return n;
}
static void victory_hint(void *pl, int ai) {   // every 10 s per AI, after the scan
    int me = (int)*(unsigned *)0x00918E84;
    if (g_victory_told || g_lord[ai] != me || !vassals_of(me)) return;
    g_victory_told = 1;
    persona_say(pl, L_ALL_SUBMITTED, player_at(me), NULL);   // in character; the how in a system line after it
    uint8_t *opt = *(uint8_t **)0x00917BB8;
    system_line(!lstrcmpiA(g_lang, "de")
                ? "Alle verbliebenen Gegner haben sich Euch unterworfen. Schreibt 'sieg' in den Chat, um das Spiel als "
                  "Sieger zu beenden - oder spielt weiter."
                : "All remaining opponents have submitted to you. Type 'victory' in the chat to end the game as the "
                  "winner - or play on.", opt ? FIELD(opt, 0x2CC, int) * 3 : 30000);
    if (g_log) fprintf(g_log, "# victory: P%d at %.1f min: every other player is a vassal of P%d - \"sieg\" ends the game\n",
                       ai, game_time() / 60000.0, me);
}
static int victory_word(const char *t) {
    char w[16];
    int n = 0;
    while (*t == ' ') t++;
    while (*t && n < 15) w[n++] = *t++;
    if (*t) return 0;
    while (n && (w[n - 1] == ' ' || w[n - 1] == '!' || w[n - 1] == '.')) n--;
    w[n] = 0;
    return !lstrcmpiA(w, "sieg") || !lstrcmpiA(w, "victory");
}
// the vassals of `me` resign (next safe point) and `me` wins alone - if every other player still in is one
static int claim_victory(int me, const char *how) {
    void *pm = player_at(me);
    int n = vassals_of(me);
    if (g_log) fprintf(g_log, "# victory: P%d claims victory at %.1f min (%s): %d vassals%s\n", me, game_time() / 60000.0, how,
                       n, n ? " resign" : ", but not everyone has submitted - refused");
    for (int j = 1; n && j < nplayers(); j++) {
        void *pj = player_at(j);
        if (j == me || !pj || is_out(pj)) continue;
        persona_say(pj, L_LORD_VICTORY, pm, NULL);
        g_sit.said_defeat[j] = 1;
        InterlockedOr(&g_resign_queue, 1 << j);
    }
    return n;
}
// Leaving a won game counts too. The menu's Resign (EEUIEMGOEResign::vf1 0x5D0578, confirmed: 0x5D05CC) sends
// EEMResign (vtable 0x828820 slot 10, 0x4BB906: the sender (+0xA as for chat) resigns and loses); a human whose
// every opponent has submitted gets the victory instead. Exit Game (EEUIEMGOEExitGame::vf1 0x5D0AFB, confirmed:
// 0x5D0B4F, pushed at 0x5D0B2B) leaves without any result - in that situation it resigns instead, which ends the
// game with the victory screen, the statistics and then the menu as usual.
#define VT_RESIGN       0x00828820
#define FN_RESIGN       0x004BB906
#define EXIT_CONFIRM_AT 0x005D0B2C
#define FN_EXIT_CONFIRM 0x005D0B4F
#define FN_RESIGN_SEND  0x005D05CC
static int patch_code32(uintptr_t at, uint32_t expected, uint32_t val) {   // an operand in the code
    uint32_t *p = (uint32_t *)at;
    DWORD old;
    if (*p != expected || !VirtualProtect(p, 4, PAGE_EXECUTE_READWRITE, &old)) return 0;
    *p = val;
    VirtualProtect(p, 4, old, &old);
    FlushInstructionCache(GetCurrentProcess(), p, 4);
    return 1;
}
static void __fastcall hook_resign(uint8_t *msg, void *edx) {
    (void)edx;
    int8_t from = (int8_t)msg[0xA];
    for (int i = 1; i < nplayers(); i++) {
        void *p = player_at(i);
        if (p && FIELD(p, 0x464, int8_t) == from && !g_players[i] && !is_out(p) && vassals_of(i)) {
            claim_victory(i, "resign");
            return;
        }
    }
    ((void (__thiscall *)(void *))FN_RESIGN)(msg);
}
static void __cdecl hook_exit_confirm(void) {
    int me = (int)*(unsigned *)0x00918E84;
    void *pm = player_at(me);
    if (pm && !is_out(pm) && vassals_of(me)) {
        if (g_log) { fprintf(g_log, "# victory: P%d leaves a won game - resigns instead (victory)\n", me); fflush(g_log); }
        ((void (__cdecl *)(void))FN_RESIGN_SEND)();
        return;
    }
    ((void (__cdecl *)(void))FN_EXIT_CONFIRM)();
}

static void __fastcall hook_ingame_chat(uint8_t *msg, void *edx) {
    (void)edx;
    int me = (int)*(unsigned *)0x00918E84;
    void *pm = player_at(me);
    const char *text = FIELD(msg + 0x10, 4, const char *);
    int refused = 0;
    if (pm && text && FIELD(pm, 0x464, int8_t) == (int8_t)msg[0xA] && victory_word(text))
        refused = !claim_victory(me, "the chat word");
    ((void (__thiscall *)(void *))FN_INGAME_CHAT)(msg);
    if (!refused) return;
    // not yet: a system line under the player's own, naming who still holds out
    int de = !lstrcmpiA(g_lang, "de");
    char names[200] = "", line[320];
    for (int j = 1; j < nplayers(); j++) {
        void *pj = player_at(j);
        if (j == me || !pj || is_out(pj)) continue;
        if (g_players[j] && g_lord[j] == me && !stance_of(pj, me) && !stance_of(pm, j)) continue;
        if (names[0] && lstrlenA(names) < 150) lstrcatA(names, ", ");
        if (lstrlenA(names) < 150) lstrcatA(names, player_name(pj));
    }
    wsprintfA(line, de ? "Noch kein Sieg - nicht unterworfen: %s." : "No victory yet - not submitted: %s.", names);
    uint8_t *opt = *(uint8_t **)0x00917BB8;
    system_line(line, opt ? FIELD(opt, 0x2CC, int) * 2 : 20000);
    if (g_log) fprintf(g_log, "# victory: refusal shown: %s\n", line);
}
static void chat_send(const char *text) {   // a chat line from the local player, as the chat box sends it (tests)
    void *net = *(void **)0x0091984C;
    if (!net) return;
    __attribute__((aligned(8))) uint8_t u[32];
    ((void (__thiscall *)(void *))*(void **)0x008231A4)(u);
    ((void *(__thiscall *)(void *, const char *))*(void **)0x00823224)(u, text);
    void *msg = ((void *(__cdecl *)(size_t))0x00683718)(0x24);
    if (msg) {
        ((void (__thiscall *)(void *, void *))0x005C2A2F)(msg, u);
        ((void (__thiscall *)(void *, void *, int))(*(void ***)net)[6])(net, msg, 0xFF);
    }
    ((void (__thiscall *)(void *))*(void **)0x00823AEC)(u);
}

// diplo: the stance matrix; diplo A B S: player A takes stance S (0 allied, 1 enemy) towards B, as the stance
// message does (tests: what a human's declaration does)
static void cmd_diplo(char *out, const char *args) {
    int a = -1, b = -1, s = -1;
    if (!strcmp(args, "exit") || !strcmp(args, "resign")) {   // diplo exit / resign: the menu's confirmed action (tests)
        if (args[0] == 'e') hook_exit_confirm(); else ((void (__cdecl *)(void))FN_RESIGN_SEND)();
        REPLY("done\n");
        return;
    }
    if (!strncmp(args, "chat ", 5)) {   // diplo chat TEXT: the local player sends TEXT to everyone (tests)
        chat_send(args + 5);
        REPLY("sent\n");
        return;
    }
    if (sscanf(args, "surrender %d", &a) == 1) {   // diplo surrender P: AI P resigns at the next safe point (tests)
        if (!g_players[a & 15] || !player_at(a)) { REPLY("error: P%d is no AI player\n", a); return; }
        InterlockedOr(&g_resign_queue, 1 << a);
        REPLY("P%d resigns; citizens %d, centers %d, no citizens since %.1f min\n", a, g_ws.citizens[a], g_ws.centers[a],
              g_no_citizens_since[a] / 60000.0);
        return;
    }
    if (sscanf(args, "capitulate %d %d", &a, &b) == 2) {   // diplo capitulate P A: AI P capitulates to A now (tests)
        if (!g_players[a & 15] || !player_at(a) || !player_at(b) || a == b) { REPLY("error: P%d is no AI player or bad P%d\n", a, b); return; }
        capitulate(player_at(a), a, b, game_time());
        REPLY("P%d capitulates to P%d: %d %d %d %d %d offered%s\n", a, b, g_promise[a][2], g_promise[a][3], g_promise[a][4],
              g_promise[a][5], g_promise[a][6], g_lord[a] == b ? " - accepted" : "");
        return;
    }
    if (sscanf(args, "tribute %d %d %d", &a, &b, &s) == 3) {   // diplo tribute A B N: A's tribute of N reaches AI B
        diplo_tribute(player_at(b), player_at(a), s);
        REPLY("P%d -> P%d: tribute %d, opinion %.0f\n", a, b, s, b > 0 && b < 16 && a > 0 && a < 16 ? g_opinion[b][a] : 0.0f);
        return;
    }
    sscanf(args, "%d %d %d", &a, &b, &s);
    if (player_at(a) && player_at(b) && a != b && (s == 0 || s == 1)) {
        set_stance(a, b, s);
        REPLY("P%d -> P%d: %d\n", a, b, stance_of(player_at(a), b));
        return;
    }
    int n = nplayers();
    REPLY("stance of row towards column (0 allied, 1 enemy) / opinion; teams locked %d, scenario %d\n    ", TEAMS_LOCKED, scenario_game());
    for (int j = 1; j < n; j++) REPLY("      P%-2d", j);
    REPLY("  power  wars answered\n");
    for (int i = 1; i < n; i++) {
        void *pi = player_at(i);
        if (!pi) continue;
        REPLY("P%-2d ", i);
        for (int j = 1; j < n; j++) {
            if (i == j) REPLY("        -");
            else if (g_players[i]) REPLY("  %d/%5.0f", stance_of(pi, j), g_opinion[i][j]);
            else REPLY("  %d      ", stance_of(pi, j));
        }
        REPLY("  %6.0f  %u%s", g_ws.power[i], g_diplo_answers[i], is_out(pi) ? " (out)" : "");
        if (g_lord[i]) REPLY("  capitulated to P%d", g_lord[i]);
        for (int j = 1; j < n; j++) if (g_pressure[i][j] >= 1) REPLY("  threat P%d %.0f", j, g_pressure[i][j]);
        REPLY("\n");
    }
}

static void diplo_install(const char *ini, const char *test_ini) {
    g_answer_war = GetPrivateProfileIntA("all", "answer_war", 1, ini);
    g_ai_diplomacy = GetPrivateProfileIntA("all", "ai_diplomacy", 1, ini);
    g_surrender = GetPrivateProfileIntA("all", "surrender", 1, ini);
    if (test_ini) {
        g_answer_war = GetPrivateProfileIntA("autotest", "answer_war", g_answer_war, test_ini);
        g_ai_diplomacy = GetPrivateProfileIntA("autotest", "ai_diplomacy", g_ai_diplomacy, test_ini);
        g_surrender = GetPrivateProfileIntA("autotest", "surrender", g_surrender, test_ini);
    }
    int ok = patch_vtable(VT_PAY_TRIBUTE, 10, FN_PAY_TRIBUTE, (void *)hook_tribute);
    int chat = patch_vtable(VT_INGAME_CHAT, 10, FN_INGAME_CHAT, (void *)hook_ingame_chat);
    int leave = patch_vtable(VT_RESIGN, 10, FN_RESIGN, (void *)hook_resign) +
                patch_code32(EXIT_CONFIRM_AT, FN_EXIT_CONFIRM, (uint32_t)(uintptr_t)hook_exit_confirm);
    if (g_log) fprintf(g_log, "# diplomacy: answer_war=%d ai_diplomacy=%d surrender=%d, tribute hook %d, chat hook %d, leave hooks %d/2\n",
                       g_answer_war, g_ai_diplomacy, g_surrender, ok, chat, leave);
}
