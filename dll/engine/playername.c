// The player name fix (single-player menu). Included by eeaimod.c after display.c (player_name_install is called
// from display_install, declared there).

// PLAYER NAME (a bug of the game): the single-player menu's name field (screen controls +0xF8, entry 8, a UIEditBox)
// is filled from the settings when the screen opens (EEUIESPEOpen::vf1: SetText(0x919308) - settings 0x918EA8 +0x460,
// "Player Name" in the registry), but none of its buttons writes it back: back to the main menu, a random map, a
// campaign - the name typed was gone ("Unbekannt" again, also in the game: the AIs called the player so). The
// buttons' handlers (vtable slot 1, thiscall(handler, screen, ?), ret 8) now first copy the field into the settings
// and save them (0x52AA81 thiscall(settings): every setting into the registry, as the options screens do).
internal void *g_sp_orig[6];
internal void keep_player_name(ee_ui_screen *screen) {
    uint8_t **ctl = screen ? uiscreen_get_controls(screen) : NULL;
    void *edit = ctl ? ctl[8] : NULL;
    if (!edit) return;
    const char *t = edit_text(edit);   // UIEditBox::GetText
    const char *cur = *(const char **)((uint8_t *)player_name_wstring() + 4);
    if (!t || !*t || (cur && !strcmp(cur, t))) return;
    char name[64];
    lstrcpynA(name, t, sizeof name);
    wstring_set(player_name_wstring(), name);   // UWideString =
    settings_save();
    if (g_log) log_locked("# player name: \"%s\" kept (settings saved)\n", name);
}
#define SP_HOOK(i) static long __thiscall hook_sp_##i(void *h, ee_ui_screen *screen, int a) { \
    keep_player_name(screen); return ((long (__thiscall *)(void *, void *, int))g_sp_orig[i])(h, screen, a); }
SP_HOOK(0) SP_HOOK(1) SP_HOOK(2) SP_HOOK(3) SP_HOOK(4) SP_HOOK(5)
internal void player_name_install(void) {
    static void *const hooks[6] = { (void *)hook_sp_0, (void *)hook_sp_1, (void *)hook_sp_2, (void *)hook_sp_3,
                                    (void *)hook_sp_4, (void *)hook_sp_5 };
    int ok = patch_sp_buttons(hooks, g_sp_orig);
    if (g_log) fprintf(g_log, "# player name: single-player menu buttons hooked %d/6%s\n", ok, ok < 6 ? " - FAILED (exe differs)" : "");
}
