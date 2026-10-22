// THE PLAYERS: the computer players the mod drives (g_players, as their updates come by), the ones it leaves to the
// game (vanilla, tests), and what the game says about each one: index, stance, out of the game.
internal ee_player *g_players[16];   // EEComputerPlayer seen by the AI hooks, by player index (ctl_note_player)
internal void ctl_note_player(ee_player *pl) {
    g_players[(unsigned)player_get_index(pl) & 15] = pl;
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
internal unsigned g_vanilla_mask;
internal int is_vanilla_idx(int idx) { return (g_vanilla_mask >> (idx & 15)) & 1; }
internal int is_vanilla(const ee_player *pl) { return pl && g_vanilla_mask && is_vanilla_idx(player_get_index(pl)); }

internal ee_player *player_at(int i) {
    unsigned n = game_player_count();
    return i > 0 && (unsigned)i < n && i < 16 ? game_players()[i] : NULL;
}
internal int nplayers(void) { unsigned n = game_player_count(); return n > 16 ? 16 : (int)n; }
// the players the game counts as human (SLOT_PLAYER_HUMAN), as FN_HUMAN_EPOCH walks them: from index 1
internal int human_count(void) {
    int n = 0;
    for (int i = 1; i < nplayers(); i++) {
        ee_player *p = player_at(i);
        if (p && player_is_human(p)) n++;
    }
    return n;
}
internal int stance_of(ee_player *a, int b) { return player_get_stance(a, b); }
internal int is_out(ee_player *p) { return !p || player_get_out(p); }

internal int is_enemy(ee_player *pl, ee_player *other) {
    if (!other || other == pl) return 0;
    unsigned oi = (unsigned)player_get_index(other);
    return oi < 16 && player_get_stance(pl, oi) != 0;
}

// Player colours: index into the game's colour table (0x8CFA74: pointers to {?, float r, g, b}); entries 0-16 are
// player colours, 17-31 unused white slots, then the UI's, terrain and resource colours (docs/engine-notes.md).
// Names: English, German, the German with its umlaut where it has one (cp1252 \xFC; written as such by color_name).
internal const char *const k_color_names[17][3] = {
    { "blue", "blau", "" }, { "red", "rot", "" }, { "yellow", "gelb", "" }, { "magenta", "lila", "" },
    { "orange", "orange", "" }, { "green", "gruen", "gr\xFCn" }, { "cyan", "tuerkis", "t\xFCrkis" },
    { "darkred", "dunkelrot", "" }, { "darkgrey", "dunkelgrau", "" }, { "purple", "violett", "" },
    { "grey", "grau", "" }, { "silver", "silber", "" }, { "lightyellow", "hellgelb", "" },
    { "lime", "hellgruen", "hellgr\xFCn" }, { "lightblue", "hellblau", "" }, { "pink", "rosa", "" },
    { "darkgrey2", "dunkelgrau2", "" },
};
internal int color_index(const char *s) {   // by its name; -1: none
    for (int c = 0; c < 17; c++)
        for (int k = 0; k < 3; k++)
            if (k_color_names[c][k][0] && !lstrcmpiA(s, k_color_names[c][k])) return c;
    return -1;
}
