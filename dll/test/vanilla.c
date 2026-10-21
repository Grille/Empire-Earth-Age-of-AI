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
