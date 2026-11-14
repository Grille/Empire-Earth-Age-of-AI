// GODMODE (`godmode [on | off]`, the console's `>godmode`; the player 2026-11-14: "i see evrething, and i can control
// every players units aswell as its buildings, i see a buildings ques like my own and can manipulate them. the main
// focus is debuging and seeing the ais units tasks and queues like i see my own"). On, this PC sees the whole map, and a
// click on any player's unit or building makes this PC that player (playas, below) with the unit selected as its own:
// its buttons, its building's queue, its orders, the top bar its stock. A click on one of the PC's own units, or
// `godmode off`, and it is itself again.
// A click is a message (EEMSelectUnit) carried out for the sender's player, which selects only its own units - another's
// is just shown, and one in its fog not even that (EEPlayer.select_unit). Its call in the message's Execute is hooked:
// the switch, then the click sent again from the owner (below: the panel is filled from the sender's selection).
// The panel's buttons: it shows them only for a player a human plays (EEComputerPlayer.computer_is_human: the computer
// says 0) - asked from the panel, the switched PC's player says 1 (hook_cp_is_human); and it is built anew only when the
// unit it shows changes, so a switch marks it to be built again (ui_panel_again).
// Checked 2026-11-14 (Continental seed 5, the game's AI on, headless): a click on P2's barracks - this PC P2, its stock
// in the top bar, the barracks with its rally flag and two train buttons; a rock thrower trained from it (P2 paid 24
// food, 24 gold); the main view without fog as P2, the minimap P2's; a save made as P2 loads as P1's game (msg_ids
// 0xFE / 0xFF as before).
// The fog: the game's no-fog flag (player explore_all) is also what tells its sight code a player has seen and explored
// everything (ee_unit_vis::explored 0x404A8A - the mod's explored_by -, the seen test 0x44E46E, the transports'
// 0x60EDA9): on an AI it would change what the AI watched knows. So it is set only on the PC's own player, whose flag
// no one's decisions read, and the own renderer leaves the fog out by god_see_all(), wherever it does for the scenario
// editor's free camera (the units' collection, the ground). The minimap is the game's: it shows what the player played as
// has explored. The game's units' collection ([display] own collection off) keeps the fog of the player played as.
// The player played as goes on deciding beside the orders given: its AI may give the units other orders.
// Not in a multiplayer game (playas: the other PCs would go on with this one as its own player). A save made while
// switched is saved as this PC's own player (god_save_begin / end round hook_save_game, autosaves too).

// THE SWITCH (`playas [P | back]`, a debug tool of its own - the player 2026-11-11: "make ai units and building
// controlable like my own"): this PC plays as player P - its selection, orders, top bar and sight. A selection is one
// player's (EEMSelectUnits: the ids of the sender's own), so another's units can't join it: the switch is the whole
// player. Two places say who this PC is: the game's local player index (read where it is needed, no copies: the top
// bar, the sight, the buttons) and the player its messages are carried out for - EEWorld::player(msg +0xA, the PC's
// slot: 0xFE in single player) is the first player whose msg_id is that slot. So P takes the PC's msg_id and the player
// left behind one nobody sends (0xE0 + its index; the computer players all have 0xFF, and the first of those answers a
// message of 0xFF: the one left behind must not become it). Then the steps of the game's own change of this PC's player
// (game.local_player_switch 0x53153E): the minimap's player and its rebuild, the view table refilled, the tech tree's
// screen, the bottom panel built again. P's computer player goes on deciding beside the orders given; the one left
// behind gets none. back: the player this PC was, every msg_id as it was.
internal int g_playas_was = -1;   // the PC's own player while switched
internal int8_t g_playas_ids[16];
internal ee_player *g_playas_game;   // its player object: another game since, another load, forgets the switch
internal void __thiscall hook_view_sight_copy(ee_player *pl);   // sight.c (sight_counts)
internal int g_sight_on;
internal int playas_switched(void) {
    unsigned n = game_player_count();
    if (g_playas_was >= 0 && (n > 16 || (unsigned)g_playas_was >= n || game_players()[g_playas_was] != g_playas_game)) g_playas_was = -1;
    return g_playas_was >= 0;
}
// this PC's player p; NULL done, else what stands in the way
internal const char *playas_to(int p) {
    unsigned *me = (unsigned *)local_player_index_at(), n = game_player_count();
    if (g_mp) return "not in a multiplayer game";
    if (p < 1 || (unsigned)p >= n || n > 16 || !game_players()[p]) return "no such player";
    if (!playas_switched()) {
        if ((unsigned)p == *me) return NULL;
        g_playas_was = (int)*me;
        g_playas_game = game_players()[*me];
        for (unsigned i = 0; i < n; i++) g_playas_ids[i] = game_players()[i] ? player_get_msg_id(game_players()[i]) : 0;
    }
    for (unsigned i = 0; i < n; i++) if (game_players()[i]) player_set_msg_id(game_players()[i], g_playas_ids[i]);
    unsigned was = *me;
    *me = (unsigned)p;
    if (p == g_playas_was) g_playas_was = -1;
    else {
        player_set_msg_id(game_players()[g_playas_was], (int8_t)(0xE0 + g_playas_was));
        player_set_msg_id(game_players()[p], g_playas_ids[g_playas_was]);
    }
    ee_player *pl = game_players()[p];
    ee_minimap *mm = *(ee_minimap **)game_minimap_at();
    if (mm) {
        minimap_set_player(mm, pl);
        minimap_rebuild(mm);
        minimap_paint_done(mm);
    }
    if (g_sight_on) hook_view_sight_copy(pl);
    else player_view_sight_copy(pl);
    if (player_get_tech_tree(pl)) tech_tree_ui_refresh(player_get_tech_tree(pl), 1);
    ui_panel_again();   // a unit it showed as another's (the click's own selection, made at once): now with its buttons
    if (g_log) log_locked("# playas: this PC was P%u, plays as P%d\n", was, p);
    return NULL;
}
internal void cmd_playas(char *out, const char *args) {
    unsigned me = local_player_index();
    int p = -1;
    if (!strncmp(args, "back", 4)) p = playas_switched() ? g_playas_was : (int)me;
    else if (sscanf(args, "%d", &p) != 1) {
        REPLY("this PC plays as P%u%s (playas P | back)\n", me, playas_switched() ? " (switched)" : "");
        return;
    }
    const char *no = playas_to(p);
    if (no) { REPLY("error: playas: %s (1 .. %u)\n", no, game_player_count() ? game_player_count() - 1 : 0); return; }
    REPLY("this PC plays as P%d (was P%u)\n", p, me);
}

internal int g_god;              // godmode on
internal ee_player *g_god_home;  // this PC's own player when it was switched on: another game, another load ends it
internal int g_god_home_i;       // its index
internal uint8_t g_god_had_all;  // its explore_all before
internal int god_on(void) {   // (the render thread too: once an object drawn)
    if (g_god && ((unsigned)g_god_home_i >= game_player_count() || game_players()[g_god_home_i] != g_god_home)) g_god = 0;
    return g_god;
}
// the own renderer: no fog - the editor's free camera, or godmode
internal int god_see_all(void) { return *game_free_camera_at() || (g_god && god_on()); }

internal void god_minimap(void) {
    ee_minimap *mm = *(ee_minimap **)game_minimap_at();
    if (mm) { minimap_rebuild(mm); minimap_paint_done(mm); }
}

// a click on a unit, carried out for its sender's player: in godmode, another player's unit makes its owner this PC's
// player, and the click is sent again from it - the server's executor fills the selection panel (its buttons, the
// building's queue) from the sender's selection after the Execute (0x4B0000's executor: FUN_005F92FF(selected group),
// the UI's unit): a selection made here for the owner left the panel the old player's, empty
internal void __thiscall hook_select_unit(ee_player *pl, uint32_t id, uint8_t fog_check, uint8_t clear) {
    if (g_god && god_on() && !g_mp && pl && pl == game_players()[local_player_index()]) {
        ee_unit *u = object_by_id(id);
        ee_player *o = u ? unit_get_owner(u) : NULL;
        int p = o ? player_get_index(o) : 0;
        if (o && o != pl && p > 0 && (unsigned)p < game_player_count() && game_players()[p] == o && !playas_to(p) &&
            select_unit_msg_send(id)) {
            ui_panel_again();
            return;
        }
    }
    player_select_unit(pl, id, fog_check, clear);
}

// the command panel's question whether a human plays this PC's player (the computer's answer: 0): the buttons of the
// player played as (godmode's switch or playas) - its trains, its building's queue - come only with 1. Asked from the panel only (its return
// address): the simulation's own questions get the computer's 0, the AI watched decides as it would (the computer's
// function is a plain "return 0" in both builds)
internal unsigned g_god_asked, g_god_told;   // the panel's questions, those answered 1 (godmode's reply)
internal char __thiscall hook_cp_is_human(ee_player *pl) {
    if (__builtin_return_address(0) != (void *)player_panel_is_human_ret_at()) return 0;
    g_god_asked++;
    int yes = pl == game_players()[local_player_index()] && playas_switched();   // godmode's switch or playas
    g_god_told += (unsigned)yes;
    return (char)yes;
}

internal void cmd_godmode(char *out, const char *args) {
    char w[8] = "";
    sscanf(args, "%7s", w);
    int on = god_on();
    if (!w[0]) on = !on;
    else if (!strcmp(w, "on")) on = 1;
    else if (!strcmp(w, "off")) on = 0;
    else { REPLY("error: godmode [on | off]\n"); return; }
    if (g_mp) { REPLY("error: godmode: not in a multiplayer game\n"); return; }
    if (!game_map() || !game_player_count()) { REPLY("error: godmode: no game running\n"); return; }
    if (on && !g_god) {
        g_god_home_i = playas_switched() ? g_playas_was : (int)local_player_index();
        g_god_home = game_players()[g_god_home_i];
        if (!g_god_home) { REPLY("error: godmode: this PC has no player\n"); return; }
        g_god_had_all = player_get_explore_all(g_god_home);
        player_set_explore_all(g_god_home, 1);
        g_god = 1;
        god_minimap();
        if (g_log) log_locked("# godmode: on (this PC's player P%d)\n", g_god_home_i);
        REPLY("godmode on: the whole map; a click on any player's unit or building plays as its owner, one of yours "
              "plays as you again, godmode off ends it\n");
        return;
    }
    if (!on && g_god) {
        g_god = 0;
        if (playas_switched()) playas_to(g_god_home_i);
        if (g_god_home) player_set_explore_all(g_god_home, g_god_had_all);
        g_god_home = NULL;
        god_minimap();
        if (g_log) log_locked("# godmode: off\n");
        REPLY("godmode off: this PC plays as P%u\n", local_player_index());
        return;
    }
    REPLY("godmode %s (this PC plays as P%u; the panel asked %u times, told yes %u)\n", on ? "on" : "off",
          local_player_index(), g_god_asked, g_god_told);
}

// a save while switched: written as this PC's own player (the msg_ids and the local index as they were), then switched
// again - only those, the sight and the screens stay the switched player's
internal int g_god_save_p = -1;
internal int8_t g_god_save_ids[16];
internal void god_save_begin(void) {
    g_god_save_p = -1;
    if (!playas_switched()) return;
    unsigned n = game_player_count(), *me = (unsigned *)local_player_index_at();
    g_god_save_p = (int)*me;
    for (unsigned i = 0; i < n; i++) if (game_players()[i]) {
        g_god_save_ids[i] = player_get_msg_id(game_players()[i]);
        player_set_msg_id(game_players()[i], g_playas_ids[i]);
    }
    *me = (unsigned)g_playas_was;
}
internal void god_save_end(void) {
    if (g_god_save_p < 0) return;
    unsigned n = game_player_count();
    for (unsigned i = 0; i < n && i < 16; i++) if (game_players()[i]) player_set_msg_id(game_players()[i], g_god_save_ids[i]);
    *(unsigned *)local_player_index_at() = (unsigned)g_god_save_p;
    g_god_save_p = -1;
}

internal void godmode_install(void) {
    int sel = patch_select_unit(hook_select_unit), panel = patch_cp_is_human(hook_cp_is_human);
    if (g_log && (!sel || !panel))
        fprintf(g_log, "# godmode: hooks FAILED (the click's selection %d, the panel's question %d)\n", sel, panel);
}
