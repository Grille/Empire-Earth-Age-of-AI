// THE DECISIONS' GENERATOR (rnd_game): what changes the game is drawn alike on every PC - the AI's diplomacy, the
// language's decisions (script/host.h rnd_decide).
// Two generators: rnd() for what is only shown (chat lines; ai_talk and the audience differ per PC), rnd_game() for
// decisions that change the game (the personality, a betrayal). Multiplayer is lockstep - every PC simulates every
// AI and must decide the same - so in a game with more than one human the game generator is seeded from the game's
// own rand() (msvcrt, import 0x824648) the first time the simulation needs it: every PC calls it at the same step.
// Single player: seeded like rnd() (the clock; tests a constant).
static uint32_t g_rnd_game = 0x2545F491u;
static int g_mp, g_mp_seeded;   // g_mp: more than one human in the setup (persona_new_game)
static uint32_t rnd_game32(void) {
    if (g_mp && !g_mp_seeded) {
        int (__cdecl *game_rand)(void) = *(int (__cdecl **)(void))GA(IAT_RAND);
        int a = game_rand(), b = game_rand();
        g_rnd_game = ((uint32_t)a << 15 ^ (uint32_t)b) * 2654435761u ^ 0x2545F491u;
        if (!g_rnd_game) g_rnd_game = 1;
        g_mp_seeded = 1;
        if (g_log) fprintf(g_log, "# persona: multiplayer, game generator seeded from the game's rand (%d %d)\n", a, b);
    }
    g_rnd_game ^= g_rnd_game << 13; g_rnd_game ^= g_rnd_game >> 17; g_rnd_game ^= g_rnd_game << 5;
    return g_rnd_game;
}
static int rnd_game(int n) { return n > 0 ? (int)(rnd_game32() % (unsigned)n) : 0; }
