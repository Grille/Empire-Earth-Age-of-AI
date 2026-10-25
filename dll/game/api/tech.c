// THE TECH TREE (ee_tech_tree), ITS TECHS (ee_tech) AND THEIR DATA (ee_tech_data), the tech effects: fields,
// functions, hooks.

// FIELDS
// A PLAYER'S TECH TREE (EETechTree, player_get_tech_tree(pl))
// +0x8 the epoch records, 0x54 apart (Art of Conquest's epoch field lies one record further: 0x538)
internal inline ee_tt_epoch *tech_tree_epoch_rec_at(const ee_tech_tree *tt, int e) { return (ee_tt_epoch *)FIELD_AT(tt, of_tech_tree_epoch_recs + e * 0x54); }
// an epoch record: +0x8 the epoch's tech's data, the new things the epoch needs +0x40 and has +0x48 (0x5BD681)

// A TECH (EETechTreeNode, tech_tree_lookup(tt, id); an epoch's node, its children: game/tech.c)
// an epoch node's children (game/tech.c): the 2017 update's vector +0x24 .. +0x28, Art of Conquest's map head +0x28
internal inline uint8_t *tech_get_kids_end(const ee_tech *t) { return FIELD(t, of_tech_kids_begin + 0x4, uint8_t *); }

// A TECH'S DATA (tech_get_data(t): the same for every player)
// +0x10 .. +0x14 the unit types that make it (its buildings)
internal inline int *tech_data_get_makers_end(const ee_tech_data *d) { return FIELD(d, of_tech_data_makers_begin + 0x4, int *); }
// +0x24 its cost by resource r (2 food .. 6 iron)
internal inline int tech_data_get_cost(const ee_tech_data *d, int r) { return FIELD(d, of_tech_data_costs + (r - 2) * 4, int); }
// +0x7D it counts as one of an epoch's new things (EETechTree::vf4)
// +0x81 Art of Conquest: granted by its epoch's line upgrades (only_new)

// AN EFFECT LIST (data_game_epoch_techs[i], a tech's tech_data_get_effects): +0x4 the effect ids, +0x8 their count

// a tech effect (tech_effect(id)): +0x20 its kind (2 an attribute, 0x13 a line upgrade), +0x14 the unit type it is on
// (-1 all; a line upgrade's old type), +0x18 a line upgrade's new type, +0x2C the attribute

// FUNCTIONS
// can it be researched now (EETechTreeNode's vtable)
internal inline char tech_available(ee_tech *t) { return ((char (__thiscall *)(void *))(*(void ***)t)[value_tech_slot_tech_available])(t); }
// researched for nothing (EETechTreeNode::vf6) - the 2017 update's, AoC's takes an argument
// the tech tree's next epoch (to the game's end epoch: it sets it and sends the epoch event)
// the epoch's line upgrades granted (node: the epoch's tech, cat 7); Art of Conquest: only_new - only the ones with
// data +0x81 set (econ.c PAID UPGRADES)
internal inline void epoch_upgrades_grant(ee_tech *node, int cat, int only_new) {
    if (build_is("aoc")) ((void (__thiscall *)(void *, int, int))data_tech_fn_epoch_upgrades)(node, cat, only_new);
    else ((void (__thiscall *)(void *, int))data_tech_fn_epoch_upgrades)(node, cat);
}

// HOOKS
// the epoch's upgrades: the 2017 update's (node, category), Art of Conquest's (node, category, only new)
typedef void (__thiscall *epoch_upgrades_hook)(ee_tech *node, int cat);
typedef void (__thiscall *epoch_upgrades_aoc_hook)(ee_tech *node, int cat, int only_new);
internal inline int patch_epoch_upgrades(epoch_upgrades_hook h, epoch_upgrades_aoc_hook h_aoc) {
    return patch_call(data_tech_call_epoch_upgrades, data_tech_fn_epoch_upgrades, build_is("aoc") ? (void *)h_aoc : (void *)h);
}
// a tech researched for nothing (EETechTreeNode::vf6, its vtable slot data_tech_vts_tech_researched)
typedef int (__thiscall *tech_grant_hook)(ee_tech *t);
internal inline int patch_tech_grant(tech_grant_hook h) { return patch_vtable(data_tech_vts_tech_researched, 0, (uintptr_t)fn_tech_grant, (void *)h); }
