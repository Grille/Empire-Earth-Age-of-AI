// THE PLAYERS: the computer players the mod drives (g_players, as their updates come by), the ones it leaves to the
// game (vanilla, tests), and what the game says about each one: index, stance, out of the game.
static ee_player *g_players[16];   // EEComputerPlayer seen by the AI hooks, by player index (ctl_note_player)
static void ctl_note_player(ee_player *pl) {
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
static int is_vanilla(const ee_player *pl) { return pl && g_vanilla_mask && is_vanilla_idx(FIELD(pl, PL_INDEX, int)); }

static ee_player *player_at(int i) {
    unsigned n = *(unsigned *)GA(G_PLAYERS_N);
    return i > 0 && (unsigned)i < n && i < 16 ? ((ee_player **)GA(G_PLAYERS))[i] : NULL;
}
// the game's unit limit for a player (EEPlayer::vf31(0), as FN_GAME_LIMIT; it pops its argument)
static int unit_limit(ee_player *pl) { return ((int (__thiscall *)(void *, int))(*(void ***)pl)[GA(SLOT_UNIT_LIMIT)])(pl, 0); }
static int nplayers(void) { unsigned n = *(unsigned *)GA(G_PLAYERS_N); return n > 16 ? 16 : (int)n; }
// the players the game counts as human (SLOT_PLAYER_HUMAN), as FN_HUMAN_EPOCH walks them: from index 1
static int human_count(void) {
    int n = 0;
    for (int i = 1; i < nplayers(); i++) {
        ee_player *p = player_at(i);
        if (p && ((char (__thiscall *)(void *))(*(void ***)p)[GA(SLOT_PLAYER_HUMAN)])(p)) n++;
    }
    return n;
}
static int stance_of(ee_player *a, int b) { return FIELD(a, OF(PL, 0x9E4) + b * 4, int); }
static int is_out(ee_player *p) { return !p || FIELD(p, OF(PL, 0x467), uint8_t); }

static int is_enemy(ee_player *pl, ee_player *other) {
    if (!other || other == pl) return 0;
    unsigned oi = (unsigned)FIELD(other, PL_INDEX, int);
    return oi < 16 && FIELD(pl, OF(PL, 0x9E4) + oi * 4, int) != 0;
}

// Player colours: index into the game's colour table (0x8CFA74: pointers to {?, float r, g, b}); entries 0-16 are
// player colours (17+ other engine colours). Names in English or German (cp1252 umlauts as \xFC).
static const char *const k_color_names[17][3] = {
    { "blue", "blau", "" }, { "red", "rot", "" }, { "yellow", "gelb", "" }, { "magenta", "lila", "" },
    { "orange", "orange", "" }, { "green", "gruen", "gr\xFCn" }, { "cyan", "tuerkis", "t\xFCrkis" },
    { "darkred", "dunkelrot", "" }, { "darkgrey", "dunkelgrau", "" }, { "purple", "violett", "" },
    { "grey", "grau", "" }, { "silver", "silber", "" }, { "lightyellow", "hellgelb", "" },
    { "lime", "hellgruen", "hellgr\xFCn" }, { "lightblue", "hellblau", "" }, { "pink", "rosa", "" },
    { "darkgrey2", "dunkelgrau2", "" },
};
static int color_index(const char *s) {   // by its name; -1: none
    for (int c = 0; c < 17; c++)
        for (int k = 0; k < 3; k++)
            if (k_color_names[c][k][0] && !lstrcmpiA(s, k_color_names[c][k])) return c;
    return -1;
}
