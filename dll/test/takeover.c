// THE TAKEOVER (the player 2026-11-16: "can you add a command to hand the human player to an ai? even change its colors
// and select a personalithy? its for quick skirmish ai tests witout having to setup a playerless scenario every
// time"; the hard way by his choice: a real computer player made in the running game). `handover [personality]
// [colour]`, from the pipe or the chat (`> handover fuchs 3`): a computer player is made in the first free slot as
// the game start makes one (EEComputerPlayer.construct, set_color, the world's set_player and count; the parts sized
// by the players' count made anew: the map's sight counts, the influence table, the AI's turns), takes the human's
// epoch, techs, improvements, stock and stances, then every unit and building of the human's (unit_give, the units
// that walk first: the AI's build manager takes its base from its first one), and its AI starts (ai_start). The human
// stays in the game as an observer - no fog, godmode on, allied to the new player - and is kept in: the victory
// check would put out a player without objects, and end the game with no human left (victory_check: its objects
// raised to 1 for the check). The new player is one of the mod's AIs from its first turn (persona_assign: the
// personality named here, else one as at a game start). Single player only; a save after it is not tested.
internal int g_ho_human = -1;   // the player handed over (kept in), -1 none
internal int g_ho_ai = -1;      // the computer player it was handed to
internal world_victory_check_fn g_ho_victory;
internal int __thiscall hook_victory_check(ee_world *w) {
    ee_player *h = g_ho_human > 0 && (unsigned)g_ho_human < game_player_count() ? game_players()[g_ho_human] : NULL;
    if (!h) return world_victory_check(w);
    int was = player_get_objects(h);
    if (was < 1) player_set_objects(h, 1);
    int r = world_victory_check(w);
    if (was < 1) player_set_objects(h, was);
    return r;
}
internal void takeover_install(void) {
    int ok = patch_victory_check(hook_victory_check);
    if (g_log) fprintf(g_log, "# takeover: the victory check hooked %s (the human handed over stays in)\n", ok ? "ok" : "FAILED");
}
internal void takeover_new_game(void) { g_ho_human = g_ho_ai = -1; }
// WHAT FOLLOWS THE HUMAN FOLLOWS ITS STAND-IN (the player 2026-11-16, of epoch_pacing follow and pop_follow_human: "make
// it follow the (human) ai"): the game and the mod take the humans' epoch and population from the players the game
// counts as human (highest_human_epoch: their tech trees' epoch; the AI's population cap 0x42F7A5: their population),
// and the observer is one of them still. At each of the stand-in's turns the observer's tech tree has its epoch (the
// game's catch-up of the AIs too when it rises: tech_researched of a human's epoch, as a human's own advance) and its
// population is the stand-in's (pop_b10: the observer has no units or lists, nothing else counts it).
internal void takeover_tick(ee_player *pl) {
    if (g_ho_ai < 0 || player_get_index(pl) != g_ho_ai || (unsigned)g_ho_human >= game_player_count()) return;
    ee_player *h = game_players()[g_ho_human];
    ee_tech_tree *ht = h ? player_get_tech_tree(h) : NULL, *tt = player_get_tech_tree(pl);
    if (!ht || !tt) return;
    int e = tech_tree_get_epoch(tt), was = tech_tree_get_epoch(ht);
    if (e != was) {
        tech_tree_set_epoch(ht, e);
        if (e > was) player_tech_researched(h, e);
        if (g_log) log_locked("# takeover: at %.1f min the observer P%d is at epoch %d with its stand-in P%d (was %d)\n", game_time() / 60000.0, g_ho_human, e, g_ho_ai, was);
    }
    player_set_pop_b10(h, player_population(pl, 1, 1));
    // the stand-in itself paces as the human it plays for: following the humans it would follow itself (follow: held
    // at its own epoch for good, and every AI that follows with it) - independent pacing then, and the game limit
    const struct ai_cfg *c = cfg_of(pl);
    if (c->pacing == PACE_FOLLOW || c->pop_follow) {
        unsigned i = (unsigned)g_ho_ai & 15;
        struct ai_cfg k = *c;
        if (k.pacing == PACE_FOLLOW) k.pacing = PACE_INDEPENDENT;
        k.pop_follow = 0;
        g_cfg_pl[i] = k; g_cfg_pl_on[i] = 1;
        if (g_log) log_locked("# takeover: P%d paces as the human it plays for - %s pacing, the game's population limit\n", g_ho_ai, k.pacing == PACE_INDEPENDENT ? "independent" : "its own");
    }
}

internal int persona_find(const char *file);
internal void persona_apply(ee_player *pl, unsigned idx, int pick);
internal void cmd_godmode(char *out, const char *args);
internal void cmd_handover(char *out, const char *args) {
    char who[64] = "";
    int colour = -1;
    sscanf(args, "%63s %d", who, &colour);
    if (who[0] >= '0' && who[0] <= '9' && colour < 0) { colour = atoi(who); who[0] = 0; }   // handover 3: a colour alone
    if (g_mp) { REPLY("error: not in a multiplayer game\n"); return; }
    unsigned hi = local_player_index(), n = game_player_count();
    ee_player *h = hi < n ? game_players()[hi] : NULL;
    ee_map *map = game_map();
    if (!h || !map || player_get_out(h) || g_ho_human >= 0) { REPLY("error: no human player to hand over (or done already)\n"); return; }
    if (n >= 16) { REPLY("error: no free player slot\n"); return; }
    int pick = who[0] ? persona_find(who) : -1;
    if (who[0] && pick < 0) { REPLY("error: no personality '%s'\n", who); return; }
    // the colour: the one asked for, else the first no player has
    float rgb[3];
    if (colour < 0 || colour > 16) {
        for (int c = 0; c <= 16 && colour < 0; c++) {
            const float *k = (const float *)game_color_rgb((uint32_t)c);
            int taken = 0;
            for (unsigned i = 1; i < n; i++) { const float *q = game_players()[i] ? (const float *)player_color_at(game_players()[i]) : NULL; taken |= q && q[0] == k[0] && q[1] == k[1] && q[2] == k[2]; }
            if (!taken) colour = c;
        }
        if (colour < 0) colour = 0;
    }
    memcpy(rgb, game_color_rgb((uint32_t)colour), sizeof rgb);
    ee_world *w = game_state();
    unsigned in = 0;
    for (unsigned i = 1; i < n; i++) in += game_players()[i] && !player_get_out(game_players()[i]);
    uint32_t each = world_get_pop_each(w);
    // MADE as the game start makes a computer player
    _Alignas(8) uint8_t ws[32];
    wstring_init_text(ws, player_get_name(h) ? player_get_name(h) : "AI");
    ee_player *p = computer_player_alloc();
    if (!p) { wstring_free(ws); REPLY("error: no memory\n"); return; }
    player_construct(p, ws, (int)n);
    wstring_free(ws);
    player_set_color(p, rgb);
    world_set_player(w, (int)n, p);
    world_set_player_count(w, n + 1);   // before any unit is its: unit_by_id bounds the ids by the count
    world_change_pop_total(w, each * (in + 1));   // every player's share as it was
    world_recount(w, 0);
    map_sight_players(map, n + 1);
    world_new_influence(w, map);
    ee_influence *inf = world_get_influence(w);
    for (struct grid_it it = grid_it_all(1); inf && grid_it_next(&it); )
        if (it.td && unit_type_get_influence(it.td)) influence_add(inf, it.u, unit_get_x(it.u), unit_get_y(it.u));
    // its tech tree as the start sets it up, then the human's epoch, techs and improvements
    ee_tech_tree *tt = player_get_tech_tree(p), *ht = player_get_tech_tree(h);
    int techs = 0, imps = 0;
    if (tt && ht) {
        tech_tree_assign_player(tt, p);
        tech_tree_set_epochs(tt, tech_tree_get_start_epoch(ht), tech_tree_get_end_epoch(ht));
        tech_tree_setup(tt, *game_tech_setup_flag_at(), 1);
        if (tech_tree_get_epoch(ht) > tech_tree_get_epoch(tt)) tech_tree_reach_epoch(tt, tech_tree_get_epoch(ht));
        for (int pass = 0, more = 1; pass < 8 && more; pass++) {   // a tech can need another first: again until none is left
            more = 0;
            for (int id = 0; id < 15000; id++) {
                ee_tech *a = tech_tree_lookup(ht, id), *b = a && tech_get_researched(a) ? tech_tree_lookup(tt, id) : NULL;
                if (!b || tech_get_researched(b)) continue;
                tech_apply(b, 1, 1); tech_set_researched(b, 1); techs++; more = 1;
            }
        }
    }
    for (int t = 0; t < 768; t++)
        for (int a = 0; a < 16; a++) {
            int want = player_improvement_level(h, t, a), guard = 0;
            while (want > 1 && player_improvement_level(p, t, a) > 0 && player_improvement_level(p, t, a) < want && guard++ < 16) { player_apply_improvement(p, t, a, 0); imps++; }
        }
    int *hs = player_stock_at(h), *ps = player_stock_at(p);
    for (int r = 0; r < 7; r++) ps[r] = hs[r];
    player_set_extra_gold_iron(p, 0);   // the first update's gift: the stock is the human's
    player_set_civ_chosen(p, 1);
    player_set_cheat(p, 0);
    // ITS STANCES: allied with a player the human is allied with both ways - unless that one is at war with another of
    // the human's allies: each set_stance joins the two players' alliance groups, and allied with two at war with each
    // other its group took them both in - one alliance left, the standard victory ended the game at the next step (the
    // first test: the test runner's human is allied with every AI). Else at war, both ways
    for (unsigned j = 1; j < n; j++) {
        ee_player *pj = j == hi ? NULL : game_players()[j];
        if (!pj) continue;
        int ally = !player_get_stance(h, (int)j) && !player_get_stance(pj, (int)hi);
        for (unsigned k = 1; k < n && ally; k++) {
            ee_player *pk = k == hi || k == j ? NULL : game_players()[k];
            if (pk && !player_get_stance(h, (int)k) && !player_get_stance(pk, (int)hi) && (player_get_stance(pj, (int)k) || player_get_stance(pk, (int)j))) ally = 0;
        }
        game_set_stance((int)n, (int)j, ally ? 0 : 1);
        game_set_stance((int)j, (int)n, ally ? 0 : 1);
    }
    game_set_stance((int)n, (int)hi, 0);
    game_set_stance((int)hi, (int)n, 0);
    // ITS UNITS: the human's, the ones that walk first
    static uint32_t ids[16384];
    int ni = 0, given = 0;
    for (int pass = 0; pass < 2; pass++) {
        int count = 0;
        for (ee_unit *u = player_get_units(h); u && count < 16384 && ni < 16384; u = unit_get_next(u), count++) {
            ee_unit_type *td = unit_get_type_data(u);
            if (!td || unit_get_hp(u) <= 0 || fr_building(td) != pass) continue;
            ids[ni++] = (uint32_t)unit_get_serial(u);
        }
    }
    for (int i = 0; i < ni; i++) {
        ee_unit *u = unit_by_id(ids[i]);
        if (u && unit_get_owner(u) == h && unit_give(u, (int)n, 0)) given++;
    }
    player_adopt(p, 0, 1);
    for (int r = 0; r < 7; r++) hs[r] = 0;
    player_ai_start(p);
    ai_turns_refresh();
    // THE HUMAN: an observer, kept in
    g_ho_human = (int)hi; g_ho_ai = (int)n;
    player_set_explore_all(h, 1);
    if (pick >= 0) { persona_assign(p); persona_apply(p, n, pick); }   // assigned first (marked), then the one asked for
    char gm[REPLY_CAP];
    gm[0] = 0;
    cmd_godmode(gm, "on");
    if (g_log) log_locked("# takeover: at %.1f min P%u handed over to the new computer player P%u (colour %d%s%s): %d units and buildings, %d techs, %d improvements, epoch %d\n",
                          game_time() / 60000.0, hi, n, colour, pick >= 0 ? ", personality " : "", pick >= 0 ? who : "", given, techs, imps, tt ? tech_tree_get_epoch(tt) : -1);
    REPLY("P%u handed over to the new computer player P%u (colour %d): %d units and buildings, %d techs, %d improvements; you watch with godmode\n", hi, n, colour, given, techs, imps);
}
