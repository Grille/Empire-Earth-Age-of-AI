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
// The same in play ([camera] camera_free_turn; the player 2026-11-25: "yes, build it, on by default"): the game's form
// has the key's events too, but the turn asks for the editor's free camera byte - that one read points at g_ec_turn_ok
// (1), the press puts view 0 or 1 into 2 as in the editor (not view 3, a camera object's or a cinematic's; not in a
// cinematic). The way back is the game's perspective key (F2: 2 goes to 0); Home is the editor's only.
internal uint8_t g_ec_not_free;   // always 0
internal uint8_t g_ec_turn_ok = 1;

internal uint32_t __thiscall hook_ec_turn(eeuie_rotate_camera *self, ee_ui_form *form, uint32_t arg) {
    uint32_t r = eeuie_rotate_camera_execute(self, form, arg);
    ee_camera *cam = game_camera();
    if ((editor_is_on() || g_set.camera_free_turn) && cam && camera_get_view(cam) == 2) camera_set_snap(cam, 1);
    return r;
}

internal uint32_t __thiscall hook_ec_turn_key(eeuie_disable_mouse_move *self, ee_ui_form *form, uint32_t arg) {
    ee_camera *cam = game_camera();
    int v = cam ? camera_get_view(cam) : 2;
    if (v != 2 && (editor_is_on() || (g_set.camera_free_turn && v != 3 && !*game_cinematic_at() && game_map())))
        camera_view_to(cam, 2);
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

// The perspective key in the editor too (the player 2026-11-26: "yes add f2 to the editor"): the game's form has it
// (F2, 0x64C8B0), the editor's never had - the view went 0 to 2 by the turn's key and back by Home only. The game's own
// event, on the editor's form as it is built (display.c hook_editor_form).
#define DIK_F2 0x3C
internal void editor_camera_hotkeys(ee_editor_form *form) {
    void *ev = toggle_perspective_new();
    if (ev) ui_form_add_press_hotkey(form, DIK_F2, ev);
    if (g_log) log_locked("# editor: F2 the perspective %s\n", ev ? "on" : "FAILED");
}

internal void editor_camera_install(void) {
    int n = patch_editor_camera(&g_ec_not_free),
        t = patch_disable_mouse_move_execute(hook_ec_turn_key) + patch_home_rotation_execute(hook_ec_home)
          + patch_rotate_camera_execute(hook_ec_turn);
    int p = g_set.camera_free_turn ? patch_rotate_free(&g_ec_turn_ok) : 0;
    if (g_log) fprintf(g_log, "# editor: the game's camera, patches %d/%d, the free turn's keys %d/3, in play %s\n", n, editor_camera_patches(), t,
                       !g_set.camera_free_turn ? "off" : p ? "on" : "FAILED");
}
