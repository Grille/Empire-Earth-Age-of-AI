// MULTIPLAYER BLOCKED ([general] multiplayer; the player 2026-11-19, before the first release: "i would block
// multyplayer, but it should show a dialog or something that if player want to try the setting must match and it will
// probably have issues anyways"). The mod changes the simulation - the AI, damage, movement, routes - and nothing yet
// checks that every PC plays the same version and settings (a PC without the mod, or other settings, parts ways with
// the others' game). The main menu's Multiplayer button (EEUIEMSEMultiPlayer.execute: the multiplayer menu, or a
// lobby's launch) queues the game's own error box instead (word multiplayer_blocked: why, and multiplayer=1 for those
// who try anyway) and the menu stays. The box is made by an action of the mod's own, run from the game's main loop as
// the game's own boxes come from its actions, not from inside the UI's event handling. The test runs' multiplayer (test/netgame.c) starts its games by the join action, not by the button.
internal void *g_mp_box_vt[UI_ACTION_SLOTS];
internal void __thiscall mp_box_exec(void *action) {
    (void)action;
    __attribute__((aligned(8))) uint8_t w[32];   // as the game's code gives it (a struct wstring's 16 bytes are too few:
    wstring_init_text(w, word("multiplayer_blocked"));   // its constructor wrote over this function's saved registers)
    error_box(ui_manager(), w, 1, 0, NULL, 1);   // its parent the UI manager, as the game's own calls give it
    wstring_free(w);
}
internal uint32_t __thiscall hook_mp_button(eeuiemse_multi_player *ev, ee_ui_form *form, uint32_t arg) {
    if (g_set.multiplayer) return eeuiemse_multi_player_execute(ev, form, arg);
    ui_action_queue_own(g_mp_box_vt, (void *)mp_box_exec);
    if (g_log) log_line("# multiplayer: the main menu's button - blocked ([general] multiplayer=0), the box queued\n");
    return 0;
}
internal void mp_block_install(void) {
    int ok = patch_main_multiplayer(hook_mp_button);
    if (g_log) fprintf(g_log, "# multiplayer: %s\n", g_set.multiplayer ? "allowed ([general] multiplayer=1)"
                       : ok ? "the main menu's button blocked with a box ([general] multiplayer=0)" : "NOT blocked: the button's slot differs");
}
