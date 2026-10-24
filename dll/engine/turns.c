// TURN FIX (`turn_fix`; docs/engine-notes.md "The rotation"): Art of Conquest's fix of the turn, brought back. Every
// movement model turns a unit by a turn rate it keeps (unit +0x17C) and asks each tick whether to turn faster or
// slower (the shared EEPhysicsModel decision, FN_TURN_DECIDE): the 2017 update's takes rate and angle left as sizes
// and decides from the rate now, so it brakes a step late and the unit swings past its heading, then back (the angle
// step is cut at the heading, the rate isn't), and a unit turning away it lets turn on. AoC rewrote it
// (EEPhysicsModel::vf12): turning away, brake; else turn faster only when, a step on at the higher rate, the braking
// from there still ends within the angle left. Here the same, with the braking counted step by step from the turn
// acceleration (AoC takes the steps from the type's +0xC8). At rest a unit may always start: the step is cut at the
// heading. Every model (ships, land units, aircraft, animals ...); AoC keeps its own.

internal int g_tf_on;

internal int __thiscall hook_turn_decide(void *model, ee_unit *u, float rate, float left) {
    ee_physics *ph = game_physics();
    float dt = ph ? phys_get_step_s(ph) : 0, a = u ? dt * unit_turn_accel(u) : 0;
    if (dt <= 0 || a <= 0) return model_turn_decide(model, u, rate, left);
    if ((rate < 0 && left > 0) || (rate > 0 && left < 0)) return 4;
    if (left == 0) return rate == 0 ? 2 : 4;
    if (rate == 0) return 3;
    float w = fabsf(rate) + a, e = fabsf(left) - dt * w;
    if (e < 0) return 4;
    int n = (int)(w / a);   // the braking's steps at w - a, w - 2a ... above 0
    return dt * ((float)n * w - a * (float)(n * (n + 1)) * 0.5f) <= e ? 3 : 4;
}

internal void turns_install(const char *aimod_ini, const char *autotest_ini) {
    g_tf_on = ini_int("general", "turn_fix", 0, aimod_ini);
    if (autotest_ini) g_tf_on = ini_int("autotest", "turn_fix", g_tf_on, autotest_ini);
    int n = g_tf_on ? patch_turn_decide(hook_turn_decide) : 0;
    if (g_log) fprintf(g_log, "# turnfix: turn_fix=%d (%s)\n", g_tf_on, !g_tf_on ? "-" : build_is("aoc") ? "AoC's own" : n ? "hooked" : "FAILED");
    if (g_log && g_tf_on && n) fprintf(g_log, "# turnfix: %d vtable slots\n", n);
}
