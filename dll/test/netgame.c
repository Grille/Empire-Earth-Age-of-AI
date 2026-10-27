// MULTIPLAYER TEST GAMES (autotest ini mp=host|join; scripts/test/mp.sh): test slots that play one game over the
// network, every PC's mod deciding alike - does the game stay in sync? (sync_trace on every PC, the traces diffed.)
// run_test.ps1 starts them at the main menu (no command line); there the mod does what the multiplayer menu's buttons
// do - it queues an EEAJoinOrHostAGame (mode 0 host the game "eeaimod<slot>", 2 join 127.0.0.1) as "Host" /
// "Join<seat>" (mp_wait ms after the menu is up): it makes the server or the client and opens the game setup screen,
// the lobby. (The game's own -hostgame / -joinbyip queue the same action, but then replace the server it made with
// another client: the game hangs in the menus.) A joiner connects to the host slot's ports (mp_host=<slot>: TCP
// Protocol.dll's connect sites moved by that slot's offset, the others by its own - slot_ports); a joiner that asks
// before the host listens stays alone (mp.sh starts it once the host's lobby is open). The host presses Start once
// mp_humans humans are seated and have been for 3 s, by the Start button's code (lobby_start) with a stand-in for the
// screen. Both run on the UI thread: an action of the mod's own on the UI's queue every half second. Every PC's game
// then starts from its own copy of the lobby's setup, which randmap_rewrite makes the test's map on each alike - the
// humans in slots 0 .. mp_humans-1, this PC's own flagged local (mp_seat).
// Included by autotest.c.

internal struct {
    int mode, humans, host_slot;   // mode 1 host, 2 join (0: a single-player test); the host's slot
    int seat;                      // a joiner's seat: its name "Join<seat>" (the host's 0, "Host")
    int asked, wait;               // the host / join action queued
    DWORD menu_since;              // the UI's actions run since (GetTickCount)
    volatile LONG started;         // the host pressed Start
    DWORD full_since;              // the lobby full since (GetTickCount, 0: not full)
} g_net = { 0, 2, 0, 0, 0, 0, 0, 0, 0 };

internal void mp_load(const char *ini) {
    char v[16];
    GetPrivateProfileStringA("autotest", "mp", "", v, sizeof v, ini);
    g_net.mode = !lstrcmpiA(v, "host") ? 1 : !lstrcmpiA(v, "join") ? 2 : 0;
    g_net.humans = ini_int("autotest", "mp_humans", 2, ini);
    if (g_net.humans < 2) g_net.humans = 2;
    if (g_net.humans > 8) g_net.humans = 8;
    g_net.host_slot = ini_int("autotest", "mp_host", 0, ini);
    g_net.seat = g_net.mode == 2 ? ini_int("autotest", "mp_seat", 1, ini) : 0;
    g_net.wait = ini_int("autotest", "mp_wait", 2000, ini);   // ms at the menu before it hosts or joins
}

// the ports this game connects to: the host slot's when it joins another slot's game, else its own
internal int mp_connect_slot(void) { return g_net.mode == 2 && g_net.host_slot > 0 ? g_net.host_slot : g_slot; }

// A multiplayer setup (0x918C24 set: the host / join action sets it, a single-player start clears it) seats a player
// who connects only in a human slot of its name (the server's connection handler 0x4C26FF: the online lobby names
// them in advance) - so the host names the seats first, its own 0 "Host", a joiner's "Join<seat>" (mp_seat=<n>). The
// handler then sends the seat's record to every PC as slot <the record's +0x20>, so that is the seat's number too.
internal char *mp_seat_name(char *s, int seat) {
    if (seat) wsprintfA(s, "Join%d", seat);
    else lstrcpyA(s, "Host");
    return s;
}
// host or join, 2 s after the main menu runs the UI's actions (the menu set up), once
internal void mp_ask(DWORD now) {
    if (!g_net.menu_since) g_net.menu_since = now ? now : 1;
    if (now - g_net.menu_since < (DWORD)g_net.wait) return;
    g_net.asked = 1;
    *game_type_var() = 0;   // a random-map game (the last game's type, 2 a saved game, seats only its players by name)
    char player[32], game[32];
    mp_seat_name(player, g_net.seat);
    // the seats named (see mp_seat_name) in the setup the action makes the lobby's, every slot its own colour (the
    // Start button refuses players of one colour)
    if (g_net.mode == 1)
        for (int i = 0; i < 16; i++) {
            char n[32];
            ee_setup_slot *s = setup_slot_at(setup_lobby_source_at(), i);
            setup_slot_set_color(s, i);
            setup_slot_set_index(s, i);   // where a seat's player's record goes (the handler sends it to +0x20's slot)
            if (i >= g_net.humans) continue;
            setup_slot_set_type(s, 1);
            __attribute__((aligned(8))) uint8_t w[32];   // the game's own assignment: the slots' names share their text
            wstring_init_text(w, mp_seat_name(n, i));
            wstring_copy(setup_slot_name_at(s), w);
            wstring_free(w);
        }
    if (g_net.mode == 1) wsprintfA(game, "eeaimod%d", g_slot);
    else lstrcpyA(game, "127.0.0.1");
    ee_join_action *act = join_action_new(g_net.mode == 1 ? 0 : 2, player, game);
    if (!act) return;
    ui_queue_action(game_get_ui(), act);
    action_release(act);
    log_line("# mp: %s \"%s\" as \"%s\"\n", g_net.mode == 1 ? "hosting" : "joining", game, player);
}

// The Start button's form, a stand-in: the lobby screen was never opened (its elements aren't made), and the button's
// code reads only its element table (+0xF8) - element 10 (the button), switched off by its slot 26 thiscall(on)
internal void __thiscall mp_element_noop(void *e, int on) { (void)e; (void)on; }
internal void *g_mp_element_vt[27], *g_mp_element[1], *g_mp_elements[11];
internal uint8_t g_mp_form[0xFC];
internal void *mp_start_form(void) {
    for (int i = 0; i < 27; i++) g_mp_element_vt[i] = (void *)mp_element_noop;
    g_mp_element[0] = g_mp_element_vt;
    g_mp_elements[10] = g_mp_element;
    *(void ***)(g_mp_form + 0xF8) = g_mp_elements;
    return g_mp_form;
}

// the host's lobby: Start once it is full
internal void mp_lobby(DWORD now) {
    if (g_net.started || !net_hosting()) return;
    ee_setup *lobby = setup_lobby_at();
    int humans = (int)server_get_seated(game_server());   // seated (the seats are human from the start)
    if (humans < g_net.humans) { g_net.full_since = 0; return; }
    if (!g_net.full_since) {
        g_net.full_since = now ? now : 1;
        log_line("# mp: %d humans in the lobby, Start in 3 s\n", humans);
        return;
    }
    if (now - g_net.full_since < 3000) return;
    InterlockedExchange(&g_net.started, 1);
    // every slot its own colour for the Start button's check (the game starts from the setup randmap_rewrite makes on
    // every PC) and every human ready (+0x35, the flags' second byte: the lobby's Ready box a joiner ticks)
    for (int i = 0; i < 16; i++) {
        ee_setup_slot *s = setup_slot_at(lobby, i);
        setup_slot_set_color(s, i);
        if (setup_slot_get_type(s) == 1) setup_slot_set_flags(s, setup_slot_get_flags(s) | 0x100);
    }
    for (int i = 0; i < 16; i++) {
        ee_setup_slot *s = setup_slot_at(lobby, i);
        if (setup_slot_get_type(s) == 4 || i > 3) continue;
        const char *pn = wstring_text(setup_slot_name_at(s));
        log_line("# mp: lobby slot %d type %d colour %d team %d flags %08x name \"%s\"\n", i, setup_slot_get_type(s),
                 setup_slot_get_color(s), setup_slot_get_team(s), setup_slot_get_flags(s), pn ? pn : "");
    }
    int r = lobby_start(mp_start_form(), 0);
    log_line("# mp: Start pressed (%d)\n", r);
}

// the mod's action: the pause action's vtable (vtable, refcount, a byte - 12 bytes, its destructor frees them), its
// Execute the mod's - [0] the RTTI locator, the object points at [1]
internal void *g_net_vt[4];
// This PC's seat flagged local in its lobby (+0x34's low byte, the others not): the loading screen takes its bar from
// it (ui loading_bar) - neither the host's named seats nor the records the server sends say which is whose, and a
// bar left unset names a garbage player in this PC's loading percentage, which the other PCs' screens index with
internal void mp_local_seat(void) {
    for (int i = 0; i < 16; i++) {
        ee_setup_slot *s = setup_slot_at(setup_lobby_at(), i);
        setup_slot_set_flags(s, (setup_slot_get_flags(s) & ~0xFFu) | (i == g_net.seat));
    }
}
// at the game start (the loading screen may have opened before the lobby said so)
internal void mp_game_start(void) {
    if (!g_net.mode) return;
    mp_local_seat();
    ui_set_loading_bar(game_get_ui(), g_net.seat);
}

internal void __thiscall mp_step(void *act) {
    (void)act;
    DWORD now = GetTickCount();
    if (g_net.asked) mp_local_seat();
    if (!g_net.asked) mp_ask(now);
    else if (g_net.mode == 1) mp_lobby(now);
}
internal DWORD WINAPI mp_thread(LPVOID unused) {
    (void)unused;
    void **base = pause_action_vtable();
    g_net_vt[0] = base[-1];
    for (int i = 0; i < 3; i++) g_net_vt[1 + i] = base[i];
    g_net_vt[1 + 1] = (void *)mp_step;
    while (!game_time()) {
        Sleep(500);
        ee_ui *ui = game_get_ui();
        uint8_t *act = ui ? pause_action_alloc() : NULL;
        if (!act) continue;
        pause_action_init(act);
        *(void ***)act = &g_net_vt[1];
        ui_queue_action(ui, act);
        action_release(act);
    }
    return 0;
}

// The game's error boxes, logged (a headless test has nobody to click them: the game waits there) - with the caller,
// the box's text in the game's language
internal void __thiscall hook_error_box(void *parent, void *text, int a, int b, void *on_ok, int c) {
    const char *t = text ? wstring_text(text) : NULL;
    log_line("# error box (from %p): \"%s\"\n", __builtin_return_address(0), t ? t : "");
    error_box(parent, text, a, b, on_ok, c);
}

internal void mp_start(void) {
    if (!g_autotest) return;
    int boxes = patch_error_box(hook_error_box);
    log_line("# autotest: the game's error boxes logged (%d calls hooked)\n", boxes);
    if (!g_net.mode) return;
    log_line("# mp: %s (%d humans%s)\n", g_net.mode == 1 ? "host" : "join", g_net.humans,
             g_net.mode == 2 ? ", the host's ports" : "");
    CloseHandle(CreateThread(NULL, 0, mp_thread, NULL, 0, NULL));
}
