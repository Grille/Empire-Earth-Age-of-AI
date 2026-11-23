// A FOUNDATION WITHOUT ITS TECH: a citizen working on a foundation of its player's - a player with a computer player,
// the mod's AIs too - has the build manager look at it again (EECPBuildManager.foundation_check, from the build goal's
// vf19): its spot no longer good, the foundation is taken down and its price given back, read off its type's tech's node
// in the player's tree. That node is read without a check: a type whose tech the player's tree lacks crashes the game
// there (0x4682DD, a null read; 2026-11-22, a test game from the player's save at minute 219, an AI of epoch 13 - which
// type is not known: the notice names it). Fix: no node, no look - the foundation stays as it is (the look at a good
// spot, 0x5E8A3B, is skipped with it: a foundation the player's tree cannot price is rare enough).
internal void __thiscall hook_foundation_check(ee_build_mgr *bm, ee_unit *f) {
    ee_player *pl = bm ? build_mgr_get_player(bm) : NULL;
    ee_unit_type *td = f ? unit_get_type_data(f) : NULL, *ty = td ? type_data(unit_type_get_id(td)) : NULL;
    int tech = ty ? unit_type_get_tech(ty) : 100000;
    ee_tech_tree *tt = pl ? player_get_tech_tree(pl) : NULL;
    if (tt && tech_tree_lookup(tt, tech)) { build_mgr_foundation_check(bm, f); return; }
    notice("a foundation whose tech its player's tree lacks", "type %d of P%d, tech %d - not looked at again (it crashed the game at 0x4682DD; engine/foundation_check.c)",
           td ? unit_type_get_id(td) : -1, pl ? player_get_index(pl) : -1, tech);
}
internal void foundation_check_install(void) { patch_foundation_check(hook_foundation_check); }
