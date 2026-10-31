// AoC'S RANDOM RELOAD (`aoc_reload`, Art of Conquest only): between two blows AoC waits reload x a random factor
// between the game data's +0x984 and +0x988 (unit_reload_wait) - 0.0 and 1.0, anything from no wait to the whole
// reload; the 2017 update waits the whole reload. The attack's tick (EEUGAttackUnit.tick, AttackEnemyUnit) sets the
// attack animation from its first frame each time and the blow lands when the animation reaches its strike frame
// (a citizen's: 8 of 0..15, about a second), so a shorter wait restarted the swing before it: on Continental (seed
// 101, game_seed 11) two AIs' food citizens never hit an ostrich - the strike function ran on no animal in AoC,
// 20 -> 15 -> 10 -> 5 -> 0 in the 2017 update - and stood by it for the whole game (21 citizens at 40 minutes, no
// army; 3 of 3 runs, and the player's game without the mod alike). With the factor at 0.99..1.0 the same games
// hunted (3 of 3).
// game (0): AoC's as it is. full (1): the 2017 update's - the whole reload, no factor. swing (2): AoC's random wait,
// but the tick doesn't restart a swing under way (the attack animation past its first frame, short of its strike
// frame): the frame moves on, the next tick finds it at the blow; a restart at the first frame loses nothing.
#define RL_GAME 0
#define RL_FULL 1
#define RL_SWING 2
internal int g_rl_mode;
internal unsigned g_rl_kept;             // ticks a swing under way was left to land (swing)

internal void __thiscall hook_reload_wait(ee_unit *u, int reload) {
    if (g_rl_mode == RL_FULL) unit_set_reload(u, reload);
    else unit_reload_wait(u, reload);
}

internal void __thiscall hook_attack_unit_tick(ee_attack_goal *goal, ee_unit *u) {
    ee_unit_stats *st = u ? unit_get_stats(u) : NULL;
    ee_dmg_model *gfx = st ? unit_stats_get_gfx(st) : NULL;
    if (gfx && unit_ai_state_id(u) == 2001) {   // AttackEnemyUnit
        int a = unit_attack_anim(u), f = unit_get_anim_frame(u);
        if (a >= 0 && a < 16 && unit_get_anim(u) == a && f > ((int *)dmg_model_anim_first_at(gfx))[a] &&
            f < ((int *)dmg_model_anim_strike_at(gfx))[a]) {
            g_rl_kept++;
            return;
        }
    }
    eeug_attack_unit_tick(goal, u);
}

internal void reload_install(void) {
    g_rl_mode = g_set.patches_aoc_reload;
    if (!build_is("aoc")) return;
    int of = 0, ok = g_rl_mode == RL_FULL ? patch_reload_wait(hook_reload_wait, &of) : g_rl_mode == RL_SWING ? patch_attack_unit_tick(hook_attack_unit_tick) : 0;
    if (g_log) fprintf(g_log, "# engine fix: aoc_reload=%s (%s)\n", g_rl_mode == RL_FULL ? "full" : g_rl_mode == RL_SWING ? "swing" : "game",
                       g_rl_mode == RL_FULL ? (ok == of && ok ? "reload calls hooked" : "FAILED") : g_rl_mode == RL_SWING ? (ok ? "attack tick hooked" : "FAILED") : "-");
}
