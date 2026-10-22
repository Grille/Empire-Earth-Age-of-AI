// The player name fix (single-player menu). Included by eeaimod.c after display.c (player_name_install is called
// from display_install, declared there).

// PLAYER NAME (a bug of the game): the single-player menu's name field (screen controls +0xF8, entry 8, a UIEditBox)
// is filled from the settings when the screen opens (EEUIESPEOpen::vf1: SetText(0x919308) - settings 0x918EA8 +0x460,
// "Player Name" in the registry), but none of its buttons writes it back: back to the main menu, a random map, a
// campaign - the name typed was gone ("Unbekannt" again, also in the game: the AIs called the player so). The
// buttons' handlers (vtable slot 1, thiscall(handler, screen, ?), ret 8) now first copy the field into the settings
// and save them (0x52AA81 thiscall(settings): every setting into the registry, as the options screens do).
internal void *g_sp_orig[6];
internal void keep_player_name(uint8_t *screen) {
    uint8_t **ctl = screen ? FIELD(screen, OF(UISCREEN, 0xF8), uint8_t **) : NULL;
    void *edit = ctl ? ctl[8] : NULL;
    if (!edit) return;
    const char *t = ((const char *(__thiscall *)(void *, unsigned long))*(void **)GA(IAT_EDIT_GET_TEXT))(edit, 0);   // UIEditBox::GetText
    const char *cur = *(const char **)(GA(G_PLAYER_NAME) + 4);
    if (!t || !*t || (cur && !strcmp(cur, t))) return;
    char name[64];
    lstrcpynA(name, t, sizeof name);
    ((void *(__thiscall *)(void *, const char *))*(void **)GA(IAT_WSTRING_ASSIGN))((void *)GA(G_PLAYER_NAME), name);   // UWideString =
    GF(FN_SETTINGS_SAVE)((void *)GA(G_SETTINGS));
    if (g_log) log_locked("# player name: \"%s\" kept (settings saved)\n", name);
}
#define SP_HOOK(i) static long __fastcall hook_sp_##i(void *h, void *edx, uint8_t *screen, int a) { \
    (void)edx; keep_player_name(screen); return ((long (__thiscall *)(void *, void *, int))g_sp_orig[i])(h, screen, a); }
SP_HOOK(0) SP_HOOK(1) SP_HOOK(2) SP_HOOK(3) SP_HOOK(4) SP_HOOK(5)
internal void player_name_install(void) {
    static void *const hooks[6] = { (void *)hook_sp_0, (void *)hook_sp_1, (void *)hook_sp_2, (void *)hook_sp_3,
                                    (void *)hook_sp_4, (void *)hook_sp_5 };
    int ok = 0;
    for (int i = 0; i < 6; i++) {
        g_sp_orig[i] = (void *)GL(SP_FNS)[i];
        ok += patch_vtable(GL(SP_VTABLES)[i], GA(SLOT_SP_BUTTON), GL(SP_FNS)[i], hooks[i]);
    }
    if (g_log) fprintf(g_log, "# player name: single-player menu buttons hooked %d/6%s\n", ok, ok < 6 ? " - FAILED (exe differs)" : "");
}
