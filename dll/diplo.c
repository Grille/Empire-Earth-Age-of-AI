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
//     war with (opinion > -20): an AI decides like a peace offer (+15 common enemy); a human gets the offer as a
//     line and 2 minutes to answer by turning allied - only then does the AI turn allied - otherwise it is withdrawn.
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
static int g_victory_off, g_victory_claimed;   // auto_victory=0: the mod cleared the end byte / the player ended the game
static void victory_hint(void *pl, int ai);
static uint32_t g_capit_last[16];       // [ai]: last capitulation offer (one per 10 minutes)
static int g_lord[16];                  // [ai]: the player it capitulated to (0 none) - never betrayed
static uint32_t g_lord_since[16];       // [ai]: its capitulation was accepted at (reputation)
static uint8_t g_justified[16][16];     // [x][y]: x's coming war on y is its side in a block split (reputation)
static int g_own_prev[16][16];          // [ai][j]: the AI's own stance towards j at the last look

// world scan (every 10 s): power, soldiers, buildings per player; enemy soldiers near each AI base
static struct {
    uint32_t t;
    float power[16];
    int soldiers[16], buildings[16], peak[16], close[16], epoch[16], out[16], citizens[16], centers[16];
    int towers[16], walls[16];   // standing towers, wall pieces (the AI voice: "your walls")
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
    if (is_tower_type(type)) g_ws.towers[o]++;
    else if (is_wall(type)) g_ws.walls[o]++;
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
        g_ws.towers[i] = g_ws.walls[i] = 0;
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

// KILLS BY PAIR: the game keeps only totals per player (statistics, player +0x4A8: 3 units killed, 2 buildings
// destroyed). They are counted in EEUnit::vf28 (0x6172BC, damage) when a unit drops to 0 hit points: at 0x617780 call
// 0x55DBCF thiscall(attacker's player +0x4A8, 3 or 2, 1), the dead unit in ESI (owner +0x18) - not for nature's units
// or one's own. Here also by pair: g_kills[killer][victim][K_SOLDIERS / K_BUILDINGS / K_CITIZENS] (soldiers = every
// unit that is no citizen: also ships, priests, siege) for the whole game, and the same per
// game minute in a ring of KILLS_RECENT_MIN buckets (g_kills_min_t: the minute each holds) for the recent fighting
// (saved with the game; for now only told to the language model, llm.c).
#define CALL_KILL_STAT 0x00617780
#define FN_STAT_ADD    0x0055DBCF
#define KILLS_RECENT_MIN 20
enum { K_SOLDIERS, K_BUILDINGS, K_CITIZENS, K_N };
static uint32_t g_kills[16][16][K_N];
static uint16_t g_kills_min[KILLS_RECENT_MIN][16][16][K_N];
static uint32_t g_kills_min_t[KILLS_RECENT_MIN];   // game minute + 1 the bucket holds (0 empty)
static void kills_recent(int k, int v, uint32_t out[K_N]) {   // k's kills of v in the last KILLS_RECENT_MIN minutes
    uint32_t m = game_time() / 60000u + 1;
    for (int c = 0; c < K_N; c++) out[c] = 0;
    for (int b = 0; b < KILLS_RECENT_MIN; b++)
        if (g_kills_min_t[b] && g_kills_min_t[b] + KILLS_RECENT_MIN > m)
            for (int c = 0; c < K_N; c++) out[c] += g_kills_min[b][k][v][c];
}
__attribute__((used)) uint8_t *g_kill_unit;   // the dead unit: esi at the call site
__attribute__((used)) void __fastcall hook_kill_stat(uint8_t *stats, void *edx, int n, int d) {
    (void)edx;
    void *owner = g_kill_unit ? FIELD(g_kill_unit, 0x18, void *) : NULL;
    if (stats && owner && (n == 2 || n == 3)) {
        int k = pidx(stats - 0x4A8), v = pidx(owner);
        uint32_t m = game_time() / 60000u + 1, b = m % KILLS_RECENT_MIN;
        if (g_kills_min_t[b] != m) { memset(g_kills_min[b], 0, sizeof g_kills_min[b]); g_kills_min_t[b] = m; }
        uint8_t *td = FIELD(g_kill_unit, 0x2C, uint8_t *);
        int c = n == 2 ? K_BUILDINGS : td && is_citizen_type(FIELD(td, 0x4B8, int)) ? K_CITIZENS : K_SOLDIERS;
        g_kills[k][v][c] += (uint32_t)d;
        g_kills_min[b][k][v][c] += (uint16_t)d;
    }
    ((void (__thiscall *)(void *, int, int))FN_STAT_ADD)(stats, n, d);
}
__attribute__((naked, used)) void kill_stat_thunk(void) {
    __asm__("movl %esi, _g_kill_unit\n\t"
            "jmp @hook_kill_stat@16");
}
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

// log2 from arithmetic only (multiplayer: the C runtime's log2f comes with Windows and may differ between versions;
// every PC must decide the same). Exponent from the bits, the mantissa m in [1,2) by the atanh series
// ln m = 2 (t + t^3/3 + t^5/5 + t^7/7), t = (m-1)/(m+1) <= 1/3: error < 1e-5.
static float det_log2f(float x) {
    if (!(x > 0)) return -1000.0f;
    union { float f; uint32_t u; } b = { x };
    int e = (int)((b.u >> 23) & 0xFF) - 127;
    b.u = (b.u & 0x007FFFFF) | 0x3F800000;
    float t = (b.f - 1.0f) / (b.f + 1.0f), t2 = t * t;
    return (float)e + 2.0f * t * (1.0f + t2 * (1.0f / 3 + t2 * (1.0f / 5 + t2 * (1.0f / 7)))) * 1.44269504f;
}

// would AI pl make peace / an alliance with player j now? (score > 0)
static float peace_score(void *pl, int ai, int j, uint32_t now) {
    if (g_war_since[ai][j] && now - g_war_since[ai][j] < (uint32_t)trait(pl, T_PRIDE) * 3000u) return -1000;
    float r = power_of(j) / power_of(ai);
    int lead = strongest_enemy(pl, ai);
    void *pj = player_at(j);
    float pw = 25.0f * det_log2f(r);
    if (pw > 40) pw = 40; else if (pw < -40) pw = -40;   // a much weaker player can still buy peace
    float s = g_opinion[ai][j] + pw + 0.4f * (float)(50 - trait(pl, T_AGGRESSION)) - 0.3f * (float)(trait(pl, T_PRIDE) - 50);
    if (lead && lead != j && pj && stance_of(pj, lead)) s += 15;
    return s;
}

// STANCE LINES ([all] stance_lines): every AI stance change used to show the game's line "<AI> wechselt
// diplomatische Haltung zu Feindselig" - also between AIs, and without saying towards whom. 1 (default): only the
// changes towards you, naming the AI (on your PC); 2: the game's line for all, as before; 0: none (the
// personality lines still speak).
static int g_stance_lines = 1;
static void block_line(int to, const char *de, const char *en);   // below
static void stance_note(int ai, int j, int s) {
    void *pl = player_at(ai), *pj = player_at(j);
    if (!pl || !pj) return;
    if (g_stance_lines >= 2) { chat_stance_line(pl, s); return; }
    if (g_stance_lines != 1 || g_players[j]) return;
    char de[200], en[200];
    wsprintfA(de, s ? "%s ist jetzt Ihr Feind." : "%s ist jetzt mit Ihnen verb\xfc" "ndet.", player_name(pl));
    wsprintfA(en, s ? "%s is now your enemy." : "%s is now your ally.", player_name(pl));
    block_line(j, de, en);
    if (g_log) fprintf(g_log, "# diplomacy: line for P%d: %s\n", j, en);
}

static void make_peace(int ai, int j, uint32_t now) {
    void *pl = player_at(ai), *pj = player_at(j);
    if (!pl || !pj) return;
    if (stance_of(pl, j)) { set_stance(ai, j, 0); stance_note(ai, j, 0); }
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
    stance_note(ai, j, 1);
    g_ally_since[ai][j] = g_ally_since[j][ai] = 0;
    g_war_since[j][ai] = now ? now : 1;   // the other side: war was declared on j
    g_offer_t[ai][j] = 0;
}

// VASSALS have no diplomacy of their own while their lord is in the game and they are allied with him: no offers,
// no betrayal, no capitulation to anyone else; a peace offer gets "talk to my lord" (line ask_lord, {enemy} = the
// lord, and a system line for a human). They still answer war and take their lord's side in a block split; a lord
// who turns on his vassal sets it free.
static int is_vassal(int ai) {
    int l = g_lord[ai];
    void *pl = player_at(ai), *pk = l ? player_at(l) : NULL;
    return pk && pl && !is_out(pk) && !stance_of(pl, l);
}
static int block_veto(int ai, int j, uint32_t now, int *own);   // alliance blocks (below)
static void block_refuse(void *pl, int j, int who, int own);

// A pending peace offer of j (allied towards the AI while the AI is at war with him)
static void peace_offer(void *pl, int ai, int j, uint32_t now) {
    void *pj = player_at(j);
    if (is_vassal(ai)) {   // not its decision
        if (!g_peace_refused[ai][j]) {
            void *pk = player_at(g_lord[ai]);
            persona_say(pl, L_ASK_LORD, pj, pk);
            char de[300], en[300];
            wsprintfA(de, "%s ist Vasall von %s: verhandeln Sie mit %s.", player_name(pl), player_name(pk), player_name(pk));
            wsprintfA(en, "%s is a vassal of %s: negotiate with %s.", player_name(pl), player_name(pk), player_name(pk));
            if (!g_players[j]) block_line(j, de, en);
            if (g_log) fprintf(g_log, "# diplomacy: P%d at %.1f min: peace offer of P%d - a vassal, refers to its lord P%d\n",
                               ai, now / 60000.0, j, g_lord[ai]);
        }
        g_peace_refused[ai][j] = 1;
        g_peace_due[ai][j] = now + 120000;
        return;
    }
    float s = peace_score(pl, ai, j, now);
    if (g_log) fprintf(g_log, "# diplomacy: P%d at %.1f min: peace offer of P%d, score %.0f (opinion %.0f, power %.0f vs %.0f)\n",
                       ai, now / 60000.0, j, s, g_opinion[ai][j], g_ws.power[j], g_ws.power[ai]);
    int own = 0, veto = s > 0 && !TEAMS_LOCKED ? block_veto(ai, j, now, &own) : 0;
    if (s > 0 && !TEAMS_LOCKED && !veto) {
        make_peace(ai, j, now);
        persona_say(pl, L_PEACE_ACCEPT, pj, NULL);
        return;
    }
    if (veto) {
        if (!g_peace_refused[ai][j]) block_refuse(pl, j, veto, own);
        g_peace_refused[ai][j] = 1;
        g_peace_due[ai][j] = now + 120000;
        return;
    }
    if (!g_peace_refused[ai][j]) persona_say(pl, L_PEACE_REFUSE, pj, NULL);
    g_peace_refused[ai][j] = 1;
    g_peace_due[ai][j] = now + 120000;
}

// ALLIANCE BLOCKS ([all] alliance_blocks, default 1; with ai_diplomacy, not in scenarios or with locked teams).
// Stances are pairwise, so two AIs can both be allied with the player and at war with each other - and nothing in
// the game can make them stop. A block = the players linked by alliances (allied both ways, through any chain).
// Every 2 s of game time:
//  - a war inside a block splits it: when a member turns hostile towards another, every other AI of the block takes
//    a side - the one it capitulated to; else its direct ally if it is allied with only one of the two; else the
//    one it thinks better of (the attacked +10) - and declares war on the other;
//  - inside a block there is peace: two AIs of one block at war make peace (their common ally brokers it); an AI
//    at war with a human of its block offers him peace (allied at once, a system line says why, 2 minutes to
//    answer) - if he doesn't, it counts as his war on the AI and the block splits;
//  - a betrayal is war on the victim's whole block (only if that block has under half the traitor's power, never
//    the block of the one it capitulated to).
// Tests: the idle observer of a test random map (allied with every AI) is no member.
static int g_blocks = 1;
static int g_block_observer;            // autotest block_observer=1: the test observer counts (human cases)
static int g_block_seen;
static int g_block_of[16];              // block (its smallest member) at the last pass, 0 = none
static uint8_t g_mutual_prev[16][16];   // allied both ways at the last pass
static uint32_t g_block_last;

static int block_member(int i) {
    void *p = player_at(i);
    if (!p || is_out(p)) return 0;
    return !(g_rm.map[0] && !g_block_observer && i == *(int *)G_LOCAL_PLAYER);
}
static int mutual(int a, int b) {
    return block_member(a) && block_member(b) && !stance_of(player_at(a), b) && !stance_of(player_at(b), a);
}
static void blocks_compute(int *blk) {
    int n = nplayers();
    for (int i = 0; i < 16; i++) blk[i] = i < n && i > 0 && block_member(i) ? i : 0;
    for (int changed = 1; changed; ) {   // the smallest index names the block
        changed = 0;
        for (int i = 1; i < n; i++)
            for (int j = i + 1; j < n; j++)
                if (blk[i] && blk[j] && blk[i] != blk[j] && mutual(i, j)) {
                    int lo = blk[i] < blk[j] ? blk[i] : blk[j], hi = blk[i] ^ blk[j] ^ lo;
                    for (int k = 1; k < n; k++) if (blk[k] == hi) blk[k] = lo;
                    changed = 1;
                }
    }
}
static int blocks_on(void) { return g_blocks && g_ai_diplomacy && !TEAMS_LOCKED && !scenario_game(); }
static void block_line(int to, const char *de, const char *en) {   // a system line, only on that player's PC
    if (to != (int)*(unsigned *)0x00918E84) return;
    uint8_t *opt = *(uint8_t **)0x00917BB8;
    system_line(!lstrcmpiA(g_lang, "de") ? de : en, opt ? FIELD(opt, 0x2CC, int) * 2 : 20000);
}

// agg turned hostile towards vic, both of block `old`: every other AI of it takes a side
static void block_split(int agg, int vic, const int *blk, uint32_t now) {
    int old = blk[vic];
    for (int m = 1; m < nplayers(); m++) {
        void *pm = player_at(m);
        if (m == agg || m == vic || !g_players[m] || !block_member(m) || !old || blk[m] != old) continue;
        int side;
        if (g_lord[m] == agg || g_lord[m] == vic) side = g_lord[m];
        else if (mutual(m, vic) != mutual(m, agg)) side = mutual(m, vic) ? vic : agg;
        else side = g_opinion[m][vic] + 10 >= g_opinion[m][agg] ? vic : agg;
        int other = side == vic ? agg : vic;
        if (stance_of(pm, other)) continue;
        g_justified[m][other] = 1;
        declare_war(m, other, now);
        g_mutual_prev[m][other] = g_mutual_prev[other][m] = 0;
        persona_say(pm, L_WAR_DECLARED, player_at(other), NULL);
        if (g_log) fprintf(g_log, "# diplomacy: blocks: P%d turned on P%d - P%d sides with P%d, war on P%d\n",
                           agg, vic, m, side, other);
    }
}

static void vassal_grumble(int v, int other, uint32_t now);   // below, with the veto
static void blocks_pass(uint32_t now) {
    if (!blocks_on()) { g_block_seen = 0; return; }
    if (g_block_seen && now - g_block_last < 2000) return;
    g_block_last = now;
    int n = nplayers(), blk[16];
    if (g_block_seen)   // wars inside a block since the last pass
        for (int x = 1; x < n; x++)
            for (int y = x + 1; y < n; y++) {
                if (!g_mutual_prev[x][y] || mutual(x, y) || !block_member(x) || !block_member(y)) continue;
                int xh = stance_of(player_at(x), y), yh = stance_of(player_at(y), x);
                int agg = xh && !yh ? x : !xh && yh ? y : (g_players[x] ? y : x);   // both at once: the human
                if (g_log) fprintf(g_log, "# diplomacy: blocks: P%d turned hostile towards its block's P%d\n", agg, agg == x ? y : x);
                block_split(agg, agg == x ? y : x, g_block_of, now);
            }
    blocks_compute(blk);
    for (int x = 1; x < n; x++)   // peace inside a block
        for (int y = x + 1; y < n; y++) {
            if (!blk[x] || blk[x] != blk[y] || mutual(x, y)) continue;
            void *px = player_at(x), *py = player_at(y);
            if (g_players[x] && g_players[y]) {
                vassal_grumble(x, y, now);
                vassal_grumble(y, x, now);
                make_peace(x, y, now);
                make_peace(y, x, now);
                persona_say(px, L_PEACE_ACCEPT, py, NULL);
                if (g_log) fprintf(g_log, "# diplomacy: blocks: P%d and P%d make peace (one block)\n", x, y);
                for (int h = 1; h < n; h++)
                    if (!g_players[h] && blk[h] == blk[x]) {
                        char de[200], en[200];
                        wsprintfA(de, "%s und %s schlie\xdf" "en Frieden: sie stehen im selben B\xfcndnis.", player_name(px), player_name(py));
                        wsprintfA(en, "%s and %s make peace: they are in the same alliance.", player_name(px), player_name(py));
                        block_line(h, de, en);
                    }
            } else if (g_players[x] || g_players[y]) {
                int a = g_players[x] ? x : y, h = a == x ? y : x;
                void *pa = player_at(a), *ph = player_at(h);
                if (g_offer_t[a][h]) continue;   // an offer is pending
                if (stance_of(pa, h)) {   // the AI is at war with him: it offers peace (turns allied when he does)
                    vassal_grumble(a, h, now);
                    g_offer_t[a][h] = now ? now : 1;
                    g_offer_kind[a][h] = 2;
                    char de[300], en[300];
                    wsprintfA(de, "%s ist mit Ihren Verb\xfcndeten verb\xfcndet und bietet Frieden an: stellen Sie binnen 2 Minuten "
                                  "auf Verb\xfcndet, sonst zerbricht das B\xfcndnis.", player_name(pa));
                    wsprintfA(en, "%s is allied with your allies and offers peace: turn allied within 2 minutes, or the "
                                  "alliance breaks.", player_name(pa));
                    block_line(h, de, en);
                    if (g_log) fprintf(g_log, "# diplomacy: blocks: P%d offers P%d (human, same block) peace\n", a, h);
                } else {                  // the AI is allied, he isn't (and no offer): his war on it
                    (void)ph;
                    if (g_log) fprintf(g_log, "# diplomacy: blocks: P%d (human) stays hostile towards P%d of its block\n", h, a);
                    block_split(h, a, blk, now);
                }
            }
        }
    blocks_compute(g_block_of);
    for (int x = 0; x < 16; x++)
        for (int y = 0; y < 16; y++) g_mutual_prev[x][y] = (uint8_t)(x < n && y < n && x != y && mutual(x, y));
    g_block_seen = 1;
}

// BLOCK VETO: peace between ai and j joins their blocks, and inside a block there is peace - so every pair of the
// two blocks at war (other than ai and j themselves) would have to make peace too. An AI of such a pair with a real
// grudge against the other - a war declared on it within pride/20 minutes, or an opinion below -30 (a stranger at
// war is -10, a declared war takes 30-70 off) - won't, and the alliance is off. Returns the player who stands in the
// way, the one the refusal names (*own: an ally of ai's own side objects, else one of j's side), else 0. Humans
// decide their own stances. A vassal whose lord is part of the deal has no say (it grumbles when the peace comes).
static float block_grudge(int x, int y, uint32_t now) {
    void *px = player_at(x);
    if (g_war_since[x][y] && now - g_war_since[x][y] < (uint32_t)trait(px, T_PRIDE) * 3000u) return -1000;
    return g_opinion[x][y];
}
static int lord_in_deal(int k, int ai, int j, const int *blk) {
    int l = g_lord[k];
    return l && (l == ai || l == j || (blk[l] && (blk[l] == blk[ai] || blk[l] == blk[j])));
}
// a vassal made to make peace with someone it holds a grudge against tells its lord so
static void vassal_grumble(int v, int other, uint32_t now) {
    if (!g_players[v] || !g_lord[v] || block_grudge(v, other, now) >= -30) return;
    persona_say(player_at(v), L_VASSAL_GRUMBLE, player_at(g_lord[v]), player_at(other));
    if (g_log) fprintf(g_log, "# diplomacy: blocks: P%d (vassal of P%d) grumbles about peace with P%d (grudge %.0f)\n", v, g_lord[v],
                       other, block_grudge(v, other, now));
}
static int g_veto_quiet;   // peace_terms asks without a log line
static int block_veto(int ai, int j, uint32_t now, int *own) {
    if (!blocks_on()) return 0;
    int blk[16], n = nplayers(), who = 0, mine = 0;
    blocks_compute(blk);
    float worst = -30;
    for (int p = 1; p < n; p++) {
        if (p != ai && !(blk[ai] && blk[p] == blk[ai])) continue;
        for (int q = 1; q < n; q++) {
            if (q != j && !(blk[j] && blk[q] == blk[j])) continue;
            if (q == p || (p == ai && q == j) || mutual(p, q) || !block_member(p) || !block_member(q)) continue;
            if (blk[p] && blk[p] == blk[q]) continue;   // already one block
            float g = 1e9f;
            if (g_players[p] && !lord_in_deal(p, ai, j, blk)) g = block_grudge(p, q, now);
            if (g_players[q] && !lord_in_deal(q, ai, j, blk) && block_grudge(q, p, now) < g) g = block_grudge(q, p, now);
            if (g < worst) { worst = g; who = q == j ? p : q; mine = q == j; }
        }
    }
    if (own) *own = mine;
    if (who && g_log && !g_veto_quiet) fprintf(g_log, "# diplomacy: blocks: no peace between P%d and P%d - P%d (%s side) stands in the way (grudge %.0f)\n",
                              ai, j, who, mine ? "the first one's" : "the other", worst);
    return who;
}
static void block_refuse(void *pl, int j, int who, int own) {   // the refusal: a line, and a system line for him
    void *pj = player_at(j), *pw = player_at(who);
    persona_say(pl, own ? L_BLOCK_REFUSE_OWN : L_BLOCK_REFUSE, pj, pw);
    char de[300], en[300];
    if (own) {
        wsprintfA(de, "%s lehnt ab: der Verb\xfcndete %s will keinen Frieden mit %s.", player_name(pl), player_name(pw), player_name(pj));
        wsprintfA(en, "%s refuses: its ally %s won't make peace with %s.", player_name(pl), player_name(pw), player_name(pj));
    } else {
        wsprintfA(de, "%s lehnt ab: kein Frieden mit %s und dessen B\xfcndnis mit %s.", player_name(pl), player_name(pj), player_name(pw));
        wsprintfA(en, "%s refuses: no peace with %s while allied with %s.", player_name(pl), player_name(pj), player_name(pw));
    }
    if (!g_players[j]) block_line(j, de, en);
}

// PEACE TERMS (the AI voice, llm.c): what AI ai would take for peace with j right now, by the rules of peace_offer -
// the tribute that lifts its peace score above 0 through the opinion it buys (diplo_tribute: total of all resources /
// (20 + 0.8 x (100 - greed))), rounded up to 50, or why no price would do (fixed teams, a vassal, its pride's time
// after his declaration, a block veto, an opinion that can't rise far enough). 0 = not at war with him.
static int peace_terms(int ai, int j, char *out, int cap) {
    void *pl = player_at(ai), *pj = player_at(j);
    out[0] = 0;
    if (!pl || !pj || !stance_of(pl, j)) return 0;
    uint32_t now = game_time();
    if (TEAMS_LOCKED) { snprintf(out, cap, "The teams are fixed in this game: no peace is possible."); return 1; }
    if (is_vassal(ai)) {
        snprintf(out, cap, "You are the vassal of %s: they must ask %s for peace, not you.", player_name(player_at(g_lord[ai])),
                 player_name(player_at(g_lord[ai])));
        return 1;
    }
    uint32_t hold = (uint32_t)trait(pl, T_PRIDE) * 3000u;
    if (g_war_since[ai][j] && now - g_war_since[ai][j] < hold) {
        unsigned m = (hold - (now - g_war_since[ai][j]) + 59999u) / 60000u;
        snprintf(out, cap, "Your pride allows no peace with them for another %u minute%s, whatever they pay (they declared war on "
                 "you).", m, m == 1 ? "" : "s");
        return 1;
    }
    int own = 0;
    g_veto_quiet = 1;
    int veto = block_veto(ai, j, now, &own);
    g_veto_quiet = 0;
    if (veto) {
        snprintf(out, cap, own ? "Your ally %s won't make peace with them - no peace at any price while that holds."
                               : "%s, in their alliance, can't be at peace with your side - no peace at any price while that holds.",
                 player_name(player_at(veto)));
        return 1;
    }
    float s = peace_score(pl, ai, j, now);
    if (s > 0) {
        snprintf(out, cap, "You would accept peace now without tribute: they only have to set their stance towards you to allied.");
        return 1;
    }
    float need = 1.0f - s, room = 100.0f - g_opinion[ai][j];
    if (need > room) {
        snprintf(out, cap, "No tribute could buy peace from you right now: your grudge and the balance of power stand against it.");
        return 1;
    }
    int price = ((int)(need * (20.0f + 0.8f * (float)(100 - trait(pl, T_GREED)))) + 49) / 50 * 50;
    snprintf(out, cap, "Your price for peace: tribute worth about %d (food, wood, stone, gold and iron all count the same, sent "
             "through the game's tribute screen); then they set their stance towards you to allied and you accept. Less is "
             "not enough; you may ask for more.", price);
    return price;
}

// AI ai's own initiative (every 60 s)
static void diplo_initiative(void *pl, int ai, uint32_t now) {
    if (!g_ai_diplomacy || TEAMS_LOCKED || scenario_game() || now < 600000 || is_vassal(ai)) return;
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
            if (g_opinion[ai][j] <= bs || block_veto(ai, j, now, NULL)) continue;   // not with someone its side can't stand
            bs = g_opinion[ai][j]; best = j;
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
            } else {                 // a human: the offer as a line; the AI turns allied when he does (2 minutes)
                g_offer_t[ai][best] = now ? now : 1;
                g_offer_kind[ai][best] = 0;
                char de[300], en[300];
                wsprintfA(de, "%s bietet ein B\xfcndnis gegen %s an: stellen Sie %s binnen 2 Minuten auf Verb\xfcndet, um "
                              "anzunehmen.", player_name(pl), player_name(player_at(lead)), player_name(pl));
                wsprintfA(en, "%s offers an alliance against %s: set %s to allied within 2 minutes to accept.", player_name(pl),
                          player_name(player_at(lead)), player_name(pl));
                block_line(best, de, en);
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
        // with blocks: the victim's whole block (everyone the AI is linked with through j) - its power, its lord
        int blk[16], lord_in = 0;
        float bpow = power_of(j);
        if (blocks_on()) {
            blocks_compute(blk);
            bpow = 0;
            for (int k = 1; k < nplayers(); k++)
                if (k != ai && blk[k] && blk[k] == blk[j]) { bpow += power_of(k); if (g_lord[ai] == k) lord_in = 1; }
        }
        if (lord_in || bpow * 2 > power_of(ai) || g_opinion[ai][j] >= 30) continue;
        int chance = (100 - trait(pl, T_LOYALTY)) * trait(pl, T_AGGRESSION);   // per 100000
        if ((int)(rnd_game32() % 100000u) >= chance) continue;
        if (g_log) fprintf(g_log, "# diplomacy: P%d at %.1f min betrays P%d (opinion %.0f, power %.0f vs %.0f)\n",
                           ai, now / 60000.0, j, g_opinion[ai][j], g_ws.power[ai], bpow);
        persona_say(pl, L_BETRAY, pj, NULL);
        declare_war(ai, j, now);
        if (blocks_on())   // the whole block: no one left to take sides
            for (int k = 1; k < nplayers(); k++)
                if (k != ai && blk[k] && blk[k] == blk[j]) {
                    declare_war(ai, k, now);
                    g_mutual_prev[ai][k] = g_mutual_prev[k][ai] = 0;
                }
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
static int pay_promise(int ai, int j, int *paid) {   // what was offered, as far as the stock still has it
    void *pl = player_at(ai), *pj = player_at(j);
    int total = 0;
    if (paid) memset(paid, 0, 7 * sizeof *paid);
    promise_escrow(ai, 0);
    if (pl && pj) {
        int *from = (int *)((uint8_t *)pl + PL_STOCK), *to = (int *)((uint8_t *)pj + PL_STOCK);
        for (int r = 2; r < 7; r++) {
            int n = g_promise[ai][r] < from[r] ? g_promise[ai][r] : from[r];
            if (n <= 0) continue;
            from[r] -= n; to[r] += n; total += n;
            if (paid) paid[r] = n;
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
    memcpy(g_say_tribute, g_promise[ai], sizeof g_say_tribute);
    if (g_log) fprintf(g_log, "# diplomacy: P%d at %.1f min capitulates to P%d (%s): %d resources (%.0f%%), power %.0f vs %.0f, "
                       "%d of its soldiers at home, buildings %d of %d at the peak\n", ai, now / 60000.0, a,
                       g_players[a] ? "AI" : "human", total, share * 100, g_ws.power[ai], g_ws.power[a], g_ws.close_by[ai][a],
                       g_ws.buildings[ai], g_ws.peak[ai]);
    persona_say(pl, L_CAPITULATE, pa, NULL);
    if (g_players[a]) {   // an AI conqueror decides at once: peace score + what the tribute is worth to it
        float gain = (float)total / (20.0f + 0.8f * (float)(100 - trait(pa, T_GREED)));
        float s = peace_score(pa, a, ai, now) + gain;
        if (s > 0 && block_veto(ai, a, now, NULL)) s = -1;   // its side and the capitulating side can't be one block
        if (g_log) fprintf(g_log, "# diplomacy: P%d answers: score %.0f (tribute worth %.0f)\n", a, s, gain);
        if (s > 0) {
            make_peace(ai, a, now);
            make_peace(a, ai, now);
            pay_promise(ai, a, NULL);
            opinion_add(a, ai, gain);
            g_lord[ai] = a;
            g_lord_since[ai] = now ? now : 1;
            persona_say(pa, L_PEACE_ACCEPT, pl, NULL);
        } else {
            memset(g_promise[ai], 0, sizeof g_promise[ai]);
            persona_say(pa, L_PEACE_REFUSE, pl, NULL);
        }
        return;
    }
    set_stance(ai, a, 0);   // a human: its units stop fighting (a white flag); his answer (allied) within 2 minutes
    if (g_stance_lines >= 2) chat_stance_line(pl, 0);
    promise_escrow(ai, 1);
    g_offer_t[ai][a] = now ? now : 1;
    g_offer_kind[ai][a] = 1;
    char de[300], en[300];
    char what[96];
    tribute_text(g_promise[ai], what, sizeof what);
    wsprintfA(de, "%s kapituliert und bietet %s: stellen Sie %s binnen 2 Minuten auf Verb\xfcndet, um anzunehmen.",
              player_name(pl), what, player_name(pl));
    wsprintfA(en, "%s surrenders and offers %s: set %s to allied within 2 minutes to accept.", player_name(pl), what,
              player_name(pl));
    block_line(a, de, en);
}
static void check_capitulation(void *pl, int ai, uint32_t now) {
    if (!g_ai_diplomacy || TEAMS_LOCKED || scenario_game() || now < 600000 || is_vassal(ai)) return;
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
    persona_say(pl, L_SURRENDER, player_at(lead), NULL);
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

static void victory_off_pass(void);   // below, with the victory
static void diplo_safe_point(void) {
    victory_off_pass();
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

// REPUTATION: breaking an alliance is seen by everyone. When a player turns hostile towards someone it was allied
// with both ways, every other AI thinks worse of him, by its loyalty: -15 x loyalty/100 (x2 when the victim is its
// own ally); betraying one's own vassal x3, within 10 minutes of accepting its capitulation (taking the tribute,
// then attacking) x5 - and the vassal itself -40 on top of its own reaction (war declared on it: -40, betrayal -60,
// x pride). Not counted: answering a war (the pair wasn't allied any more), an AI withdrawing an unanswered offer
// (never allied both ways), taking a side in a block split (g_justified).
static uint8_t g_rep_prev[16][16];   // [x][y]: allied both ways at the last look
static int g_rep_seen;


static int allied_both(int x, int y) {
    void *px = player_at(x), *py = player_at(y);
    return px && py && !is_out(px) && !is_out(py) && !stance_of(px, y) && !stance_of(py, x);
}
static void reputation_pass(uint32_t now) {
    int n = nplayers();
    if (g_rep_seen)
        for (int x = 1; x < n; x++)
            for (int y = 1; y < n; y++) {
                void *px = player_at(x), *py = player_at(y);
                if (x == y || !g_rep_prev[x][y] || !px || !py || is_out(px) || is_out(py) || !stance_of(px, y)) continue;
                if (stance_of(py, x) && g_players[x] && !g_players[y]) continue;   // both at once: the human started it
                if (g_justified[x][y]) { g_justified[x][y] = 0; continue; }
                int vassal = g_lord[y] == x, fresh = vassal && g_lord_since[y] && now - g_lord_since[y] < 600000;
                for (int o = 1; o < n; o++) {
                    void *po = player_at(o);
                    if (o == x || o == y || !g_players[o] || !po || is_out(po)) continue;
                    float d = -15.0f * (float)trait(po, T_LOYALTY) / 100.0f;
                    if (allied_both(o, y)) d *= 2;
                    if (vassal) d *= fresh ? 5 : 3;
                    opinion_add(o, x, d);
                }
                if (vassal && g_players[y]) opinion_add(y, x, -40);
                if (g_log) fprintf(g_log, "# diplomacy: reputation: P%d broke its alliance with P%d%s - every AI thinks worse of it\n",
                                   x, y, fresh ? " (its vassal, right after accepting the capitulation)" : vassal ? " (its vassal)" : "");
            }
    for (int x = 0; x < 16; x++)
        for (int y = 0; y < 16; y++) g_rep_prev[x][y] = (uint8_t)(x > 0 && y > 0 && x < n && y < n && x != y && allied_both(x, y));
    g_rep_seen = 1;
}

static void diplo_tick(void *pl, unsigned ai, uint32_t now) {
    persona_assign(pl);
    reputation_pass(now);
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
        if (g_diplo_seen[ai] && s && !g_diplo_prev[ai][j] && g_lord[ai] == j) {   // its lord turned on it: free
            g_lord[ai] = 0;
            if (g_log) fprintf(g_log, "# diplomacy: P%u at %.1f min: its lord P%d turned on it - no longer a vassal\n", ai, now / 60000.0, j);
        }
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
            stance_note((int)ai, j, 1);
            persona_say(pl, g_war_betrayal[ai][j] ? L_BETRAYED : L_WAR_DECLARED, pj, NULL);
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
        if (g_offer_t[ai][j]) {   // the AI's alliance offer, capitulation or block peace to a human
            int capit = g_offer_kind[ai][j] == 1, unity = g_offer_kind[ai][j] == 2;
            if (!s) {
                g_offer_t[ai][j] = 0;
                g_ally_since[ai][j] = now ? now : 1;
                if (capit) {
                    int paid_total = pay_promise((int)ai, j, g_say_tribute);
                    g_lord[ai] = j;
                    g_lord_since[ai] = now ? now : 1;
                    g_war_since[ai][j] = g_war_since[j][ai] = 0;
                    opinion_add((int)ai, j, 20);
                    persona_say(pl, L_CAPITULATE_ACCEPTED, pj, NULL);
                    if (g_log) fprintf(g_log, "# diplomacy: P%u at %.1f min: P%d accepts the capitulation, %d resources paid\n",
                                       ai, now / 60000.0, j, paid_total);
                } else {   // his answer to the alliance / block peace: now the AI turns allied too
                    make_peace((int)ai, j, now);
                    persona_say(pl, L_OFFER_ACCEPTED, pj, NULL);
                    if (g_log) fprintf(g_log, "# diplomacy: P%u at %.1f min: P%d accepts the %s\n", ai, now / 60000.0, j,
                                       unity ? "block peace" : "alliance");
                }
                g_offer_ignored[ai][j] = 0;
            } else if (now - g_offer_t[ai][j] > 120000) {
                g_offer_t[ai][j] = 0;
                if (capit) { promise_escrow((int)ai, 0); memset(g_promise[ai], 0, sizeof g_promise[ai]); }
                else if (!unity) g_offer_ignored[ai][j]++;
                if (capit && g_log) fprintf(g_log, "# diplomacy: P%u at %.1f min: P%d ignored the capitulation\n", ai, now / 60000.0, j);
                persona_say(pl, capit ? L_CAPITULATE_REFUSED : L_OFFER_WITHDRAWN, pj, NULL);
                if (!stance_of(pl, j)) {   // the capitulation's cease-fire ends (an offer never changed its stance)
                    set_stance((int)ai, j, 1);
                    stance_note((int)ai, j, 1);
                }
                if (unity && blocks_on()) {   // he stays at war with a member of his block: it splits
                    int blk[16];
                    blocks_compute(blk);
                    if (g_log) fprintf(g_log, "# diplomacy: blocks: P%d (human) let the block peace with P%u run out\n", j, ai);
                    if (blk[j] && blk[j] == blk[ai]) block_split(j, (int)ai, blk, now);
                }
            }
        }
    }
    g_diplo_seen[ai] = 1;
    blocks_pass(now);
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
    memset(g_kills, 0, sizeof g_kills);
    memset(g_kills_min, 0, sizeof g_kills_min);
    memset(g_kills_min_t, 0, sizeof g_kills_min_t);
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
    g_victory_off = g_victory_claimed = 0;
    memset(g_capit_last, 0, sizeof g_capit_last);
    memset(g_lord, 0, sizeof g_lord);
    memset(g_own_prev, 0, sizeof g_own_prev);
    g_block_seen = 0; g_block_last = 0;
    g_rep_seen = 0;
    memset(g_justified, 0, sizeof g_justified); memset(g_lord_since, 0, sizeof g_lord_since);
    memset(g_block_of, 0, sizeof g_block_of); memset(g_mutual_prev, 0, sizeof g_mutual_prev);
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
// AUTO VICTORY ([general] auto_victory, default 1 = the game's own end). The standard victory is the game state's
// byte +0xB96 (from the game settings +0x408 at the game start, the editor's flag): 0x531913 ends the game every step
// once one alliance group is left, and takes out every player without units (+0xB14 = its unit count). With
// auto_victory=0 the mod clears the byte (every pass, so saved games too) and takes out unit-less players itself
// (next safe point); the game goes on until the player ends it - "sieg"/"victory" in the chat, Resign or Exit - once
// everyone still in is allied with him (vassals or allies) or nobody is left: his vassals resign, the byte comes
// back and the game's own check ends it at the next step, his side winning. A hint says when.
#define STATE_STD_VICTORY ((uint8_t *)GAME_STATE + 0xB96)
static int g_auto_victory = 1;

static int allied_with(int a, int b) {
    void *pa = player_at(a), *pb = player_at(b);
    return pa && pb && !stance_of(pa, b) && !stance_of(pb, a);
}
static int can_claim(int me) {   // everyone else still in is allied with me (vassals included), or nobody is left
    void *pm = player_at(me);
    if (!pm || me < 1 || me > 15 || is_out(pm)) return 0;
    for (int j = 1; j < nplayers(); j++) {
        void *pj = player_at(j);
        if (j != me && pj && !is_out(pj) && !allied_with(me, j)) return 0;
    }
    return 1;
}
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
    int ok = can_claim(me) && (g_victory_off || vassals_of(me)), n = 0;
    for (int j = 1; ok && j < nplayers(); j++) { void *pj = player_at(j); n += pj && j != me && !is_out(pj) && g_lord[j] == me; }
    if (g_log) fprintf(g_log, "# victory: P%d claims victory at %.1f min (%s): %d vassals%s\n", me, game_time() / 60000.0, how,
                       n, ok ? " resign" : ", but not everyone has submitted - refused");
    if (ok && g_victory_off) { g_victory_claimed = 1; *STATE_STD_VICTORY = 1; }   // the game's own check ends it
    for (int j = 1; ok && j < nplayers(); j++) {
        void *pj = player_at(j);
        if (j == me || !pj || is_out(pj) || g_lord[j] != me) continue;   // allies stay and win with him
        persona_say(pj, L_LORD_VICTORY, pm, NULL);
        g_sit.said_defeat[j] = 1;
        InterlockedOr(&g_resign_queue, 1 << j);
    }
    return ok;
}
static void victory_off_pass(void) {   // auto_victory=0, every pass between simulation steps
    if (g_auto_victory || g_victory_claimed || !*(void **)0x00919774) return;
    if (*STATE_STD_VICTORY) {
        *STATE_STD_VICTORY = 0;
        if (!g_victory_off && g_log) fprintf(g_log, "# victory: automatic end off - the game goes on until the player ends it\n");
        g_victory_off = 1;
    }
    if (!g_victory_off) return;
    for (int i = 1; i < nplayers(); i++) {   // the game's elimination: no units left
        void *p = player_at(i);
        if (p && !is_out(p) && FIELD(p, 0xB14, int) < 1 && !(g_resign_queue & (1 << i))) {
            if (g_log) fprintf(g_log, "# victory: P%d has no units left - out\n", i);
            InterlockedOr(&g_resign_queue, 1 << i);
        }
    }
    int me = (int)*(unsigned *)0x00918E84;
    if (g_victory_told || !can_claim(me)) return;
    g_victory_told = 1;
    for (int j = 1; j < nplayers(); j++)   // a vassal says it in character, if there is one
        if (g_players[j] && g_lord[j] == me && !is_out(player_at(j))) { persona_say(player_at(j), L_ALL_SUBMITTED, player_at(me), NULL); break; }
    uint8_t *opt = *(uint8_t **)0x00917BB8;
    system_line(!lstrcmpiA(g_lang, "de")
                ? "Kein Gegner steht mehr gegen Euch. Schreibt 'sieg' in den Chat, um das Spiel als Sieger zu beenden - "
                  "oder spielt weiter."
                : "No opponent stands against you any more. Type 'victory' in the chat to end the game as the winner - or "
                  "play on.", opt ? FIELD(opt, 0x2CC, int) * 3 : 30000);
    if (g_log) fprintf(g_log, "# victory: P%d at %.1f min: no opponent left - \"sieg\" ends the game\n", me, game_time() / 60000.0);
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
        if (p && FIELD(p, 0x464, int8_t) == from && !g_players[i] && !is_out(p) && (g_victory_off ? can_claim(i) : vassals_of(i))) {
            claim_victory(i, "resign");
            return;
        }
    }
    ((void (__thiscall *)(void *))FN_RESIGN)(msg);
}
static void __cdecl hook_exit_confirm(void) {
    int me = (int)*(unsigned *)0x00918E84;
    void *pm = player_at(me);
    if (pm && !is_out(pm) && (g_victory_off ? can_claim(me) : vassals_of(me))) {
        if (g_log) { fprintf(g_log, "# victory: P%d leaves a won game - resigns instead (victory)\n", me); fflush(g_log); }
        ((void (__cdecl *)(void))FN_RESIGN_SEND)();
        return;
    }
    ((void (__cdecl *)(void))FN_EXIT_CONFIRM)();
}

static void llm_player_chat(const char *text);   // llm.c: the AIs answer (language model)
static void __fastcall hook_ingame_chat(uint8_t *msg, void *edx) {
    (void)edx;
    int me = (int)*(unsigned *)0x00918E84;
    void *pm = player_at(me);
    const char *text = FIELD(msg + 0x10, 4, const char *);
    int refused = 0;
    if (pm && text && FIELD(pm, 0x464, int8_t) == (int8_t)msg[0xA] && victory_word(text))
        refused = !claim_victory(me, "the chat word");
    ((void (__thiscall *)(void *))FN_INGAME_CHAT)(msg);
    if (pm && text && FIELD(pm, 0x464, int8_t) == (int8_t)msg[0xA] && !refused) llm_player_chat(text);
    if (!refused) return;
    // not yet: a system line under the player's own, naming who still holds out
    int de = !lstrcmpiA(g_lang, "de");
    char names[200] = "", line[320];
    for (int j = 1; j < nplayers(); j++) {
        void *pj = player_at(j);
        if (j == me || !pj || is_out(pj)) continue;
        if (allied_with(me, j) && (g_victory_off || (g_players[j] && g_lord[j] == me))) continue;
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
    if (sscanf(args, "observer %d", &a) == 1) {   // diplo observer 0|1: the test observer counts in blocks (tests)
        g_block_observer = a;
        REPLY("block_observer=%d\n", a);
        return;
    }
    if (sscanf(args, "surrender %d", &a) == 1) {   // diplo surrender P: AI P resigns at the next safe point (tests)
        if (!g_players[a & 15] || !player_at(a)) { REPLY("error: P%d is no AI player\n", a); return; }
        InterlockedOr(&g_resign_queue, 1 << a);
        REPLY("P%d resigns; citizens %d, centers %d, no citizens since %.1f min\n", a, g_ws.citizens[a], g_ws.centers[a],
              g_no_citizens_since[a] / 60000.0);
        return;
    }
    if (sscanf(args, "terms %d %d", &a, &b) == 2) {   // diplo terms P H: AI P's terms for peace with H (the AI voice)
        char t[400];
        int r = peace_terms(a, b, t, sizeof t);
        REPLY("P%d -> P%d: %s (%d; score %.0f, opinion %.0f)\n", a, b, r ? t : "not at war", r, player_at(a) ?
              peace_score(player_at(a), a, b, game_time()) : 0.0f, a > 0 && a < 16 && b >= 0 && b < 16 ? g_opinion[a][b] : 0.0f);
        return;
    }
    if (!strcmp(args, "kills")) {   // diplo kills: units / buildings each player destroyed of each other one
        for (int k = 1; k < nplayers(); k++)
            for (int v = 0; v < nplayers(); v++)
                if (g_kills[k][v][K_SOLDIERS] || g_kills[k][v][K_CITIZENS] || g_kills[k][v][K_BUILDINGS]) {
                    uint32_t r[K_N], *a = g_kills[k][v];
                    kills_recent(k, v, r);
                    REPLY("P%d killed of P%d: %u soldiers, %u citizens, %u buildings (last %d min: %u, %u, %u)\n", k, v,
                          a[K_SOLDIERS], a[K_CITIZENS], a[K_BUILDINGS], KILLS_RECENT_MIN, r[K_SOLDIERS], r[K_CITIZENS],
                          r[K_BUILDINGS]);
                }
        REPLY("done\n");
        return;
    }
    if (sscanf(args, "offer %d %d", &a, &b) == 2) {   // diplo offer P H: AI P offers human H an alliance now (tests)
        if (!g_players[a & 15] || g_players[b & 15] || !player_at(a) || !player_at(b)) { REPLY("error: P%d no AI or P%d no human\n", a, b); return; }
        g_offer_t[a][b] = game_time() ? game_time() : 1;
        g_offer_kind[a][b] = 0;
        g_offer_last[a][b] = game_time();
        REPLY("P%d offers P%d an alliance; its stance towards P%d stays %d\n", a, b, b, stance_of(player_at(a), b));
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
    g_blocks = GetPrivateProfileIntA("all", "alliance_blocks", 1, ini);
    g_stance_lines = GetPrivateProfileIntA("all", "stance_lines", 1, ini);
    g_auto_victory = GetPrivateProfileIntA("general", "auto_victory", 1, ini);
    if (test_ini) {
        g_answer_war = GetPrivateProfileIntA("autotest", "answer_war", g_answer_war, test_ini);
        g_ai_diplomacy = GetPrivateProfileIntA("autotest", "ai_diplomacy", g_ai_diplomacy, test_ini);
        g_surrender = GetPrivateProfileIntA("autotest", "surrender", g_surrender, test_ini);
        g_blocks = GetPrivateProfileIntA("autotest", "alliance_blocks", g_blocks, test_ini);
        g_stance_lines = GetPrivateProfileIntA("autotest", "stance_lines", g_stance_lines, test_ini);
        g_auto_victory = GetPrivateProfileIntA("autotest", "auto_victory", g_auto_victory, test_ini);
        g_block_observer = GetPrivateProfileIntA("autotest", "block_observer", 0, test_ini);
    }
    int ok = patch_vtable(VT_PAY_TRIBUTE, 10, FN_PAY_TRIBUTE, (void *)hook_tribute);
    int chat = patch_vtable(VT_INGAME_CHAT, 10, FN_INGAME_CHAT, (void *)hook_ingame_chat);
    BOOL kills = patch_call(CALL_KILL_STAT, FN_STAT_ADD, kill_stat_thunk);
    if (g_log) fprintf(g_log, "# diplomacy: kills by pair %s\n", kills ? "ok" : "FAILED");
    int leave = patch_vtable(VT_RESIGN, 10, FN_RESIGN, (void *)hook_resign) +
                patch_code32(EXIT_CONFIRM_AT, FN_EXIT_CONFIRM, (uint32_t)(uintptr_t)hook_exit_confirm);
    if (g_log) fprintf(g_log, "# diplomacy: answer_war=%d ai_diplomacy=%d surrender=%d alliance_blocks=%d auto_victory=%d, tribute hook %d, chat hook %d, leave hooks %d/2\n",
                       g_answer_war, g_ai_diplomacy, g_surrender, g_blocks, g_auto_victory, ok, chat, leave);
}
