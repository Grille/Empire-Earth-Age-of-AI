// A FOUNDATION WITHOUT ITS TECH: a citizen working on a foundation of its player's - a player with a computer player,
// the mod's AIs too - has the build manager look at it again (EECPBuildManager.foundation_check, from the build goal's
// vf19): its spot no longer good, the foundation is taken down and its price given back, read off its type's tech's node
// in the player's tree. That node is read without a check: a type whose tech the player's tree lacks crashes the game
// there (0x4682DD, a null read; 2026-11-22, a test game from the player's save at minute 219, an AI of epoch 13 - which
// type is not known: this logs it). Fix: no node, no look - the foundation stays as it is (the look at a good spot,
// 0x5E8A3B, is skipped with it: a foundation the player's tree cannot price is rare enough).
internal unsigned g_fc_skipped;
internal void __thiscall hook_foundation_check(ee_build_mgr *bm, ee_unit *f) {
    ee_player *pl = bm ? build_mgr_get_player(bm) : NULL;
    ee_unit_type *td = f ? unit_get_type_data(f) : NULL, *ty = td ? type_data(unit_type_get_id(td)) : NULL;
    int tech = ty ? unit_type_get_tech(ty) : 100000;
    ee_tech_tree *tt = pl ? player_get_tech_tree(pl) : NULL;
    if (tt && tech_tree_lookup(tt, tech)) { build_mgr_foundation_check(bm, f); return; }
    if (g_log && g_fc_skipped++ < 5)
        fprintf(g_log, "# engine fix: a foundation (type %d) of P%d whose tech %d its tree lacks - not looked at again (it crashed the game) at %.1f min\n",
                td ? unit_type_get_id(td) : -1, pl ? player_get_index(pl) : -1, tech, game_time() / 60000.0);
}
internal void foundation_check_install(void) { patch_foundation_check(hook_foundation_check); }
