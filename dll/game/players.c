// THE PLAYERS: the computer players the mod drives (g_players, as their updates come by), the ones it leaves to the
// game (vanilla, tests), and what the game says about each one: index, stance, out of the game.
static void *g_players[16];   // EEComputerPlayer seen by the AI hooks, by player index (ctl_note_player)
static void ctl_note_player(void *pl) {
    g_players[(unsigned)FIELD(pl, PL_INDEX, int) & 15] = pl;
}

// VANILLA PLAYERS (autotest only): [autotest] vanilla_players = the player numbers (e.g. 3, or 23) that play the
// game's own AI, unmodified - the benchmark "the mod's AI against the base game's (cheating) AI in one game". Every
// hook that acts for or on an AI player passes straight through to the game's original for them, every per-player
// tick of the mod skips them (only its log lines stay: battle statistics, army, gather, spending, attack times), the
// mod's diplomacy treats them as it treats a human (it never decides for them), and no setting, personality or cheat
// of the mod's touches them; they get the game's own difficulty and Cheat toggle instead (vanilla_difficulty,
// vanilla_cheat - randmap.c). What stays on for everyone: the engine guards (pure crash fixes: the unit list and
// double release, subgroup members without their unit, damage points, goal sets after a dropped goal, the field
// without foundation, no unit offered to groups during the mod's own call-off) and the test plumbing.
static unsigned g_vanilla_mask;
static int is_vanilla_idx(int idx) { return (g_vanilla_mask >> (idx & 15)) & 1; }
static int is_vanilla(const void *pl) { return pl && g_vanilla_mask && is_vanilla_idx(FIELD(pl, PL_INDEX, int)); }

static void *player_at(int i) {
    unsigned n = *(unsigned *)GA(G_PLAYERS_N);
    return i > 0 && (unsigned)i < n && i < 16 ? ((void **)GA(G_PLAYERS))[i] : NULL;
}
static int nplayers(void) { unsigned n = *(unsigned *)GA(G_PLAYERS_N); return n > 16 ? 16 : (int)n; }
static int stance_of(void *a, int b) { return FIELD(a, 0x9E4 + b * 4, int); }
static int is_out(void *p) { return !p || FIELD(p, 0x467, uint8_t); }

static int is_enemy(void *pl, void *other) {
    if (!other || other == pl) return 0;
    unsigned oi = (unsigned)FIELD(other, PL_INDEX, int);
    return oi < 16 && FIELD(pl, 0x9E4 + oi * 4, int) != 0;
}
