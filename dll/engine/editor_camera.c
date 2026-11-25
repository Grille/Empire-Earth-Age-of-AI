// THE EDITOR'S CAMERA IS THE GAME'S (the player, 2026-11-09: "can you make the editor use the actual game camera? in base
// game it has longer range for editing and is only isomentric, with our changes to camera its just worse as an independed
// cam"). The game gives the editor a camera of its own by the free camera byte (game.free_camera), read in the camera's
// code: 42 tiles back and a far depth of 58 instead of the normal view's, the look-at raised, the near plane fixed, the
// camera allowed under the ground's height - and the editor's start puts the view to 2, a fixed angle. Here those reads see
// a byte that stays 0, and the editor starts in view 0: [camera]'s distance, view distance and field of view, and the
// angle levelling out on the way in, are the editor's too. Everything else that reads the byte (no fog in the editor, its
// selection) goes on as it was. The wheel past the game's furthest: camera.c hook_wheel_zoom.
// The editor's free turn (the player 2026-11-25: "the scenario editor had a "free" cam feature, with our own camera thats
// completly lost"): the ` key held turns the camera by the mouse (EEUIERotateCamera), but only in view 2, whose zoom and
// scroll keep the turn - in view 0 they put the zoom's pitch back. So the key's press puts the editor's camera in view 2
// as it stands (hook_ec_turn_key: the turn of view 0 kept, the game's mouse frozen by the game's own event then), and
// Home, the game's way back to the editor's fixed start, goes back to view 0 instead (hook_ec_home): the turn, the
// pitch, the angle levelling out on the way in as before.
// Each poll's turn is a move of its own: the render thread draws the camera's ride ahead of the camera's step
// (render_frame.c rf_camera_frame), and the next turn's move starts from where the step had got to - the picture went
// back a little at every poll while the mouse turned it (the player 2026-11-25: "it stutters and jump back while moving,
// only a visual bug, in the end it moves as it should"). So a turn is there at once, as a notch of the wheel is
// (hook_ec_turn: snap; its steps as fine as the mouse's counts).
internal uint8_t g_ec_not_free;   // always 0

internal uint32_t __thiscall hook_ec_turn(eeuie_rotate_camera *self, ee_ui_form *form, uint32_t arg) {
    uint32_t r = eeuie_rotate_camera_execute(self, form, arg);
    ee_camera *cam = game_camera();
    if (editor_is_on() && cam && camera_get_view(cam) == 2) camera_set_snap(cam, 1);
    return r;
}

internal uint32_t __thiscall hook_ec_turn_key(eeuie_disable_mouse_move *self, ee_ui_form *form, uint32_t arg) {
    ee_camera *cam = game_camera();
    if (editor_is_on() && cam && camera_get_view(cam) != 2) camera_view_to(cam, 2);
    return eeuie_disable_mouse_move_execute(self, form, arg);
}
internal uint32_t __thiscall hook_ec_home(eeuie_return_to_home_rotation *self, ee_ui_form *form, uint32_t arg) {
    ee_camera *cam = game_camera();
    if (editor_is_on() && cam) {
        camera_view_to(cam, 0);
        return 0;
    }
    return eeuie_return_to_home_rotation_execute(self, form, arg);
}

internal void editor_camera_install(void) {
    int n = patch_editor_camera(&g_ec_not_free),
        t = patch_disable_mouse_move_execute(hook_ec_turn_key) + patch_home_rotation_execute(hook_ec_home)
          + patch_rotate_camera_execute(hook_ec_turn);
    if (g_log) fprintf(g_log, "# editor: the game's camera, patches %d/%d, the free turn's keys %d/3\n", n, editor_camera_patches(), t);
}
