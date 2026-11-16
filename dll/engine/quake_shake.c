// EARTHQUAKE SHAKE ([camera] quake_shake; the player 2026-11-16: "it shake fights with our camera very badly. i asume
// it messes the actual camera position and isnt just some offset like it should be"). The earthquake's shake (EE
// 0x4AA52E, AoC's at its call sites) takes the camera's look-at (camera.look_at_blend), pushes it a random way by the
// quake's strength x the step's time and centres the camera there (camera.center) - every step the quake shakes, never
// back: the camera walked off, and each push was a new move of the camera that the fine scrolling and the zoom's ride
// (camera.c, render_frame.c) took up as the player's. Now the push is kept as an offset instead and the camera stays:
// each frame's view (rf_camera_frame) moves by it, fading out over QS_FADE_MS until the next step's push - the view
// shakes around where the camera stands. Only while the mod's renderer draws (the game's draws from the camera alone).
#define QS_FADE_MS 120.0
internal float g_qs_base[3];            // the look-at the shake took (the simulation thread)
internal volatile float g_qs_off[2];    // the push of its last step, world x, y
internal volatile LONGLONG g_qs_at;     // rd_qpc of it
internal int g_qs_on;
internal void __thiscall hook_quake_look_at(ee_camera *cam, float t, float *out) {
    camera_look_at_blend(cam, t, out);
    memcpy(g_qs_base, out, sizeof g_qs_base);
}
internal void __thiscall hook_quake_center(ee_camera *cam, float *at, char a, char b) {
    if (!g_qs_on || !g_rd_on) { camera_center(cam, at, a, b); return; }   // the game's own draw: its own shake
    g_qs_off[0] = at[0] - g_qs_base[0];
    g_qs_off[1] = at[1] - g_qs_base[1];
    g_qs_at = rd_qpc();
}
// the frame's camera transformation (by rows, the eye in the fourth column) moved by the shake now
internal void quake_shake_frame(float *m) {
    LONGLONG at = g_qs_at;
    if (!at) return;
    double ms = (double)(rd_qpc() - at) / (double)rd_qpc_ms(1);
    if (ms >= QS_FADE_MS || ms < 0) { g_qs_at = 0; return; }
    float w = (float)(1.0 - ms / QS_FADE_MS);
    m[3] += g_qs_off[0] * w;
    m[7] += g_qs_off[1] * w;
}
internal void quake_shake_install(void) {
    g_qs_on = g_set.quake_shake;
    int a = patch_quake_look_at(hook_quake_look_at), b = patch_quake_center(hook_quake_center);
    if (g_log) fprintf(g_log, "# quake shake: %s (%s)\n", g_qs_on ? "the view shakes, the camera stays" : "the game's (the camera moved)",
                       a && b ? "hooked" : "NOT hooked - the game's");
}
