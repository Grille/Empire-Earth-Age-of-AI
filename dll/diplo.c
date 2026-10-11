// AI DIPLOMACY. The game's AI has none: EECPDiplomacyManager (ctor 0x47AB80) is an empty class, so an AI never
// changes its stance. Stances are one-way (player +0x9E4 + i*4: 0 allied, 1 enemy towards player i): a player who
// declares war on an AI (EEMSetDiplomaticStance -> 0x52D5D7) is an enemy to the AI's units only in his own eyes -
// the AI stays allied, does not fight back, and its revenge reaction (0x46FFB7: attack + "Sie wollen ein Problem?")
// waits for an enemy stance that never comes. Included by eeaimod.c after defence.c.
//
// ANSWER WAR (aimod.ini [all] answer_war, default 1): a player who turns hostile towards an AI that is still allied
// with him gets war back 3-6 s later: 0x52D5D7 thiscall(game state 0x918300, from, to, stance 1) - the same call the
// stance message makes, with the team bookkeeping (0x530F24) - the game's stance line in the chat ("<AI> wechselt
// diplomatische Haltung zu Feindselig"), then the revenge reaction 0x46FFB7 thiscall(combat manager player +0xB8C,
// declaring player): it picks his most attractive unit or building (0x470FF5), plans an attack on it (0x472427) and
// taunts (12500 + 0..9). Only changes during the game count (the first look takes the stances as they are, e.g. a
// scenario's one-way stances).
#define GAME_STATE   ((void *)0x00918300)
#define FN_SET_STANCE 0x0052D5D7
#define FN_REVENGE    0x0046FFB7
static int g_answer_war = 1;
static int g_diplo_seen[16];
static int g_diplo_prev[16][16];        // [ai][j]: player j's stance towards the AI at the last look
static uint32_t g_diplo_due[16][16];    // [ai][j]: answer war at this time (0 = nothing pending)
static unsigned g_diplo_answers[16];

static void *player_at(int i) {
    unsigned n = *(unsigned *)0x009183CC;
    return i > 0 && (unsigned)i < n && i < 16 ? ((void **)0x00918374)[i] : NULL;
}
static int stance_of(void *a, int b) { return FIELD(a, 0x9E4 + b * 4, int); }

static void diplo_tick(void *pl, unsigned ai, uint32_t now) {
    if (!g_answer_war || FIELD(pl, 0x467, uint8_t)) return;
    unsigned n = *(unsigned *)0x009183CC;
    for (int j = 1; j < (int)n && j < 16; j++) {
        void *pj = player_at(j);
        if (!pj || j == (int)ai) continue;
        int s = stance_of(pj, (int)ai);
        if (g_diplo_seen[ai] && s != 0 && g_diplo_prev[ai][j] == 0 && !stance_of(pl, j)) {
            g_diplo_due[ai][j] = now + 3000 + (uint32_t)((ai * 7 + (unsigned)j * 3) % 4) * 1000;
            if (g_log) fprintf(g_log, "# diplomacy: P%u at %.1f min: P%d declared war on it\n", ai, now / 60000.0, j);
        }
        g_diplo_prev[ai][j] = s;
        if (g_diplo_due[ai][j] && now >= g_diplo_due[ai][j]) {
            g_diplo_due[ai][j] = 0;
            if (!s || stance_of(pl, j) || FIELD(pj, 0x467, uint8_t)) continue;   // peace again, or already at war
            ((void (__thiscall *)(void *, int, int, int))FN_SET_STANCE)(GAME_STATE, (int)ai, j, 1);
            chat_stance_line(pl, 1);
            void *cbm = FIELD(pl, 0xB8C, void *);
            if (cbm) ((void (__thiscall *)(void *, void *))FN_REVENGE)(cbm, pj);
            g_diplo_answers[ai]++;
            if (g_log) fprintf(g_log, "# diplomacy: P%u at %.1f min answers war: now enemy of P%d (stance %d)\n",
                               ai, now / 60000.0, j, stance_of(pl, j));
        }
    }
    g_diplo_seen[ai] = 1;
}

// diplo: the stance matrix; diplo A B S: player A takes stance S (0 allied, 1 enemy) towards B, as the stance
// message does (tests: what a human's declaration does)
static void cmd_diplo(char *out, const char *args) {
    int a = -1, b = -1, s = -1;
    sscanf(args, "%d %d %d", &a, &b, &s);
    if (player_at(a) && player_at(b) && a != b && (s == 0 || s == 1)) {
        ((void (__thiscall *)(void *, int, int, int))FN_SET_STANCE)(GAME_STATE, a, b, s);
        REPLY("P%d -> P%d: %d\n", a, b, stance_of(player_at(a), b));
        return;
    }
    unsigned n = *(unsigned *)0x009183CC;
    REPLY("stance of row towards column (0 allied, 1 enemy); answered wars per AI in []\n    ");
    for (unsigned j = 1; j < n && j < 16; j++) REPLY(" P%-2u", j);
    REPLY("\n");
    for (unsigned i = 1; i < n && i < 16; i++) {
        void *pi = player_at((int)i);
        if (!pi) continue;
        REPLY("P%-2u ", i);
        for (unsigned j = 1; j < n && j < 16; j++) REPLY("  %c ", i == j ? '-' : '0' + stance_of(pi, (int)j));
        REPLY(" [%u]%s\n", g_diplo_answers[i], FIELD(pi, 0x467, uint8_t) ? " (out)" : "");
    }
}

static void diplo_install(const char *ini, const char *test_ini) {
    g_answer_war = GetPrivateProfileIntA("all", "answer_war", 1, ini);
    if (test_ini) g_answer_war = GetPrivateProfileIntA("autotest", "answer_war", g_answer_war, test_ini);
    if (g_log) fprintf(g_log, "# diplomacy: answer_war=%d\n", g_answer_war);
}
