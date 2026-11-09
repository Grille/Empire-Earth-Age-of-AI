// THE EDITOR'S CAMERA IS THE GAME'S (the player, 2026-11-09: "can you make the editor use the actual game camera? in base
// game it has longer range for editing and is only isomentric, with our changes to camera its just worse as an independed
// cam"). The game gives the editor a camera of its own by the free camera byte (game.free_camera), read in the camera's
// code: 42 tiles back and a far depth of 58 instead of the normal view's, the look-at raised, the near plane fixed, the
// camera allowed under the ground's height - and the editor's start puts the view to 2, a fixed angle. Here those reads see
// a byte that stays 0, and the editor starts in view 0: [camera]'s distance, view distance and field of view, and the
// angle levelling out on the way in, are the editor's too. Everything else that reads the byte (no fog in the editor, its
// selection) goes on as it was. The wheel past the game's furthest: camera.c hook_wheel_zoom.
internal uint8_t g_ec_not_free;   // always 0

internal void editor_camera_install(void) {
    int n = patch_editor_camera(&g_ec_not_free);
    if (g_log) fprintf(g_log, "# editor: the game's camera, patches %d/%d\n", n, editor_camera_patches());
}
