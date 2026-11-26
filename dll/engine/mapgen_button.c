// THE AGE OF AI MAP'S OPTIONS BUTTON on the game setup screen (the player 2026-11-25: "naming the option age of ai,
// with an options button appearing beside it when selected"; step 3 of the plan he agreed to). The setup screen (form
// 26, the lobby's too) is built from the UI database; its code reaches its controls by their index in the form's
// array of them (UIForm.controls, as long as the database makes it), and the form draws what is in a second array
// (controls_drawn) and finds the control under the mouse in its groups. So the button gets a slot of its own: both
// arrays one longer (ui_form_grow), the button at the new end - beyond every index the game's code knows -, with the
// map type list's id (the form finds a control's group by it) and in the list's group. Low-Level Engine.dll's
// UIButton without pictures (the Cancel button's draw white on a control the form didn't load itself): a black box,
// the word map_options_button in the Cancel button's font and the lists' yellow. Made the first time the screen is
// brought up to date with the list's event there (setup_screen_refresh: from then on the list has it; the form is
// kept, it has the button from then on). It takes the right part of the list's place: with the mod's type chosen the
// list - and its text box, which keeps its own width when the list's changes - gives up MB_WIDTH of its width (the
// form's coordinates) and the button shows there, else the list has its whole width back and the button is hidden -
// set when the screen is brought up to date, set up (its single-player, host and client activations) and when the
// map type or the game type changes. On a PC that joined a multiplayer game it stays hidden: the host's options are
// that game's (its Start sends them to every PC: mapgen.c hook_lobby_check), the host has the button. Its click opens
// the options' dialog (mapgen_dialog.c) for the setup's map size and players - with an example seed: the game draws
// its own when it starts, unless the dialog's "play this map" is ticked (mapgen.c g_mg_keep; in multiplayer it goes
// with the host's options).
#define MB_WIDTH 80.0f   // the button's width, in the form's coordinates (the list: 210)
#define MB_GAP 4.0f
internal struct {
    ee_ui_form *form;
    ee_ui_control *list, *button;
    float right;         // the list's own right edge
    int joined;          // the screen set up for a multiplayer game this PC joined
} g_mb;
internal void *g_mb_vt[UI_EVENT_SLOTS];

// the setup screen's map type: the mod's chosen in its list
internal int mb_ours(ee_ui_control *list) {
    unsigned i = ui_combo_box_get_current_selection((ui_combo_box *)list);
    return i < map_types_count() && mapgen_is_ours(map_type_name_text(map_types_at(i)));
}

// The list and the button kept are still the form's controls. The game builds its forms anew (another resolution: the
// editor's, its uiscale) and a new setup screen can come at the address the old one had: the list and the button kept
// were freed with it, and the click on Random Map's skirmish set the list's width through a freed text box (the
// player's game, 2026-11-27: "ee died after i clicked on the random map skirmish button", a jump to garbage under
// mb_update). Then they are forgotten - the screen's next refresh makes the button for the new form (hook_mb_refresh).
internal int mb_still(ee_ui_form *form) {
    if (!g_mb.button || setup_screen_map_list(form) != g_mb.list) return 0;
    for (unsigned i = 0, n = ui_form_control_count(form); i < n; i++)
        if (ui_form_control_at(form, i) == g_mb.button) return 1;
    return 0;
}
// the list's width and the button by what is chosen (the list hidden - a scenario's or a saved game's setup -, a
// multiplayer game this PC joined: the list whole, no button)
internal void mb_update(ee_ui_form *form) {
    if (!form || form != g_mb.form || !g_mb.list || !g_mb.button) return;
    if (!mb_still(form)) {
        if (g_log) log_locked("# mapgen: the setup screen made anew at %p - its options button made again at its refresh\n", (void *)form);
        g_mb.form = NULL, g_mb.list = g_mb.button = NULL;
        return;
    }
    ee_ui_control *list = g_mb.list;
    int on = !g_mb.joined && ui_control_get_mode_now(list) && mb_ours(list);
    float right = on ? g_mb.right - MB_WIDTH - MB_GAP : g_mb.right, d = right - ui_control_get_right(list);
    ee_ui_control *box = ui_combo_box_get_text_box((ui_combo_box *)list);
    if (d != 0 && box) {   // the box first: the list's SetLocation puts the box at its corner and its arrow after the box
        float r[4] = { ui_control_get_bottom(box), ui_control_get_left(box), ui_control_get_right(box) + d, ui_control_get_top(box) };
        ui_control_set_rectangle(box, r);
    }
    if (d != 0) {
        float r[4] = { ui_control_get_bottom(list), ui_control_get_left(list), right, ui_control_get_top(list) };
        ui_control_set_rectangle(list, r);
    }
    ui_control_set_display_mode(g_mb.button, on ? 1 : 0);
    if (on) ui_control_set_dirty(g_mb.button, 1);   // drawn (made shown: its display mode alone changed nothing)
}

// its click: the options for the setup's map (its size, its players: the slots of a human - a network one's too - or
// a computer)
internal uint32_t __thiscall mb_click(void *ev, ee_ui_form *form, uint32_t arg) {
    (void)ev; (void)form; (void)arg;
    ee_setup *s = setup_lobby_at();
    int np = 0;
    for (int k = 0; k < 16; k++) { int t = setup_slot_get_type(setup_slot_at(s, k)); np += t == 0 || t == 1 || t == 2; }
    uint32_t seed = GetTickCount() % 1000000u;   // an example: the game draws its own at the start
    md_open_setup(map_scale_tiles(setup_get_map_scale(s)), np > 0 ? np : 1, seed);
    return 0;
}

// the button made for the screen (once: the form keeps it), in the list's place
internal void mb_make(ee_ui_form *form, ee_ui_control *list) {
    ee_ui_control *cancel = setup_screen_cancel(form);
    ee_ui_button *c = (ee_ui_button *)cancel;
    float rect[4] = { ui_control_get_bottom(list), ui_control_get_right(list) - MB_WIDTH, ui_control_get_right(list), ui_control_get_top(list) };
    int group = ui_form_group_of(form, list);
    if (group < 0) { if (g_log) log_line("# mapgen: the setup screen's map type list in none of its form's groups - no options button\n"); return; }
    ee_ui_control *b = button_new(ui_control_get_id(list), NULL, NULL, NULL, rect, form, ui_form_control_count(form));
    void *ev = b ? ui_event_own(g_mb_vt, (void *)mb_click) : NULL;
    if (!b || !ev || ui_form_grow(form, b) < 0) { if (g_log) log_line("# mapgen: the setup screen's options button not made (no memory)\n"); return; }
    static const float black[4] = { 0, 0, 0, 1 };
    ui_control_set_bg_color(b, (void *)black);
    ui_control_set_bg_opacity(b, 1.0f);
    __attribute__((aligned(8))) uint8_t w[32];
    wstring_init_text(w, word("map_options_button"));
    ee_ui_label_text *t = c ? ui_button_get_text(c) : NULL;
    button_set_text(b, w, t ? ui_label_text_get_font(t) : ui_get_font_label());
    wstring_free(w);
    ee_ui_label_text *bt = ui_button_get_text((ee_ui_button *)b);
    if (bt) {   // the lists' yellow
        ui_label_text_set_r(bt, 0.99607843f); ui_label_text_set_g(bt, 0.96862745f); ui_label_text_set_b(bt, 0.1254902f);
        ui_label_text_set_colored(bt, 1);
    }
    ui_button_on_click(b, ev);
    action_release(ev);   // the button keeps its own reference
    ui_form_add_control(form, b, (unsigned long)group);
    g_mb.form = form; g_mb.list = list; g_mb.button = b; g_mb.right = ui_control_get_right(list);
    if (g_log) log_locked("# mapgen: the setup screen's options button made (form %p, the map type list %u in group %d, %.0f .. %.0f, %s)\n", (void *)form,
                          ui_control_get_index(list), group, ui_control_get_left(list), g_mb.right, c ? "the Cancel button's font" : "the editor's font: no Cancel button found");
}

// the screen brought up to date (setup_functions.setup_screen_refresh, after the game's own): the button made (the
// first time: the list has its event only from there on, on the PC that serves the game) and set
internal void __cdecl hook_mb_refresh(void) {
    setup_screen_refresh();
    ee_ui_form *form = ui_form(FORM_SETUP);
    ee_ui_control *list = form ? setup_screen_map_list(form) : NULL;
    if (!list) return;
    if (form != g_mb.form || list != g_mb.list || !mb_still(form)) mb_make(form, list);   // (a form made anew where the old was)
    mb_update(form);
}
// the screen set up for a single-player game, a multiplayer game's host, one who joined: which it is known before
// the game's own setting up (it refreshes the screen), the button set after it; an earlier lobby's options from its
// host forgotten
internal uint32_t __thiscall hook_mb_sp(eeuiele_activate_single_player *ev, ee_ui_form *form, uint32_t arg) {
    g_mb.joined = 0;
    mapgen_host_forget();
    uint32_t r = eeuiele_activate_single_player_execute(ev, form, arg);
    mb_update(form);
    return r;
}
internal uint32_t __thiscall hook_mb_host(eeuiele_activate_multi_player_server *ev, ee_ui_form *form, uint32_t arg) {
    g_mb.joined = 0;
    mapgen_host_forget();
    uint32_t r = eeuiele_activate_multi_player_server_execute(ev, form, arg);
    mb_update(form);
    return r;
}
internal uint32_t __thiscall hook_mb_client(eeuiele_activate_multi_player_client *ev, ee_ui_form *form, uint32_t arg) {
    g_mb.joined = 1;
    mapgen_host_forget();
    uint32_t r = eeuiele_activate_multi_player_client_execute(ev, form, arg);
    mb_update(form);
    return r;
}
internal uint32_t __thiscall hook_mb_type(eeuiele_game_map_type *ev, ee_ui_form *form, uint32_t arg) {
    uint32_t r = eeuiele_game_map_type_execute(ev, form, arg);
    mb_update(form);
    return r;
}
internal uint32_t __thiscall hook_mb_game_type(eeuiele_game_type *ev, ee_ui_form *form, uint32_t arg) {
    uint32_t r = eeuiele_game_type_execute(ev, form, arg);
    mb_update(form);
    return r;
}
internal void mapgen_button_install(void) {
    int of = 0, calls = patch_setup_screen_refresh(hook_mb_refresh, &of);
    int n = patch_le_activate_sp(hook_mb_sp) + patch_le_activate_host(hook_mb_host) + patch_le_activate_client(hook_mb_client)
            + patch_le_map_type(hook_mb_type) + patch_le_game_type(hook_mb_game_type);
    if (g_log) fprintf(g_log, "# mapgen: the setup screen's options button - the screen's refresh %d/%d calls, %d of 5 events hooked%s\n", calls, of, n,
                       n == 5 && calls == of && of ? "" : " - FAILED: the exe differs");
}

// mapoptions: the setup screen's options button clicked (its event fired as a click fires it) - a test's way to the
// dialog without the mouse
internal void cmd_mapoptions(char *out, const char *args) {
    (void)args;
    if (!g_mb.button) { REPLY("error: no options button (the setup screen not brought up yet)\n"); return; }
    if (!ui_control_get_mode_now(g_mb.button)) { REPLY("error: the options button is hidden (another map type, a multiplayer game joined)\n"); return; }
    ui_control_fire_click(g_mb.button);
    REPLY("ok: the options button's click fired\n");
}

// mapkeep <seed>: the next Age of AI random map from that seed, as the dialog's "play this map" with it as its preview's
// (mapgen.c g_mg_keep) - a multiplayer test host's lobby without the screen (it goes with the host's options)
internal void cmd_mapkeep(char *out, const char *args) {
    unsigned seed;
    if (sscanf(args, "%u", &seed) != 1) { REPLY("error: mapkeep <seed>\n"); return; }
    g_mg_keep.on = 1;
    g_mg_keep.seed = seed;
    REPLY("ok: the next Age of AI map from seed %u\n", seed);
}
