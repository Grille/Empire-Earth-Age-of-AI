// THE TECH TREE (ee_tech_tree), ITS TECHS (ee_tech) AND THEIR DATA (ee_tech_data), the tech effects: fields,
// functions, hooks.

// FIELDS
// A PLAYER'S TECH TREE (EETechTree, player_get_tech_tree(pl))
internal inline int tt_get_player_index(const ee_tech_tree *tt) { return FIELD(tt, TT_PLAYER_INDEX, int); }     // +0x4
internal inline int tt_get_epoch(const ee_tech_tree *tt) { return FIELD(tt, TT_EPOCH, int); }          // +0x4E4 the current epoch (0 = Prehistoric)
// +0x8 the epoch records, 0x54 apart (Art of Conquest's epoch field lies one record further: 0x538)
internal inline ee_tt_epoch *tt_epoch_rec_at(const ee_tech_tree *tt, int e) { return (ee_tt_epoch *)FIELD_AT(tt, TT_EPOCH_RECS + e * 0x54); }
// an epoch record: +0x8 the epoch's tech's data, the new things the epoch needs +0x40 and has +0x48 (0x5BD681)
internal inline ee_tech_data *ttrec_get_tech_data(const ee_tt_epoch *rec) { return FIELD(rec, TTREC_TECH_DATA, ee_tech_data *); }
internal inline int ttrec_get_needed(const ee_tt_epoch *rec) { return FIELD(rec, TTREC_NEEDED, int); }
internal inline int ttrec_get_done(const ee_tt_epoch *rec) { return FIELD(rec, TTREC_DONE, int); }

// A TECH (EETechTreeNode, tech_lookup(tt, id); an epoch's node, its children: game/tech.c)
internal inline uint8_t tech_get_researched(const ee_tech *t) { return FIELD(t, TECH_RESEARCHED, uint8_t); }   // +0x4 (vf6 sets it)
internal inline uint8_t *tech_researched_at(const ee_tech *t) { return (uint8_t *)FIELD_AT(t, TECH_RESEARCHED); }
internal inline uint8_t tech_get_available(const ee_tech *t) { return FIELD(t, TECH_AVAILABLE, uint8_t); }    // +0x6 (vf1, vf6 test it)
internal inline ee_tech_data *tech_get_data(const ee_tech *t) { return FIELD(t, TECH_DATA, ee_tech_data *); } // +0x8
internal inline ee_tech_tree *tech_get_tree(const ee_tech *t) { return FIELD(t, TECHNODE_TREE, ee_tech_tree *); } // +0xC its tree
internal inline int tech_get_epoch(const ee_tech *t) { return FIELD(t, TECH_EPOCH, int); }               // +0x14
// an epoch node's children (game/tech.c): the 2017 update's vector +0x24 .. +0x28, Art of Conquest's map head +0x28
internal inline uint8_t *tech_get_kids_begin(const ee_tech *t) { return FIELD(t, TECHNODE_KIDS, uint8_t *); }
internal inline uint8_t *tech_get_kids_end(const ee_tech *t) { return FIELD(t, TECHNODE_KIDS + 0x4, uint8_t *); }

// A TECH'S DATA (tech_get_data(t): the same for every player)
// +0x10 .. +0x14 the unit types that make it (its buildings)
internal inline int *techdata_get_makers_begin(const ee_tech_data *d) { return FIELD(d, TECHDATA_MAKERS, int *); }
internal inline int *techdata_get_makers_end(const ee_tech_data *d) { return FIELD(d, TECHDATA_MAKERS + 0x4, int *); }
// +0x24 its cost by resource r (2 food .. 6 iron)
internal inline int techdata_get_cost(const ee_tech_data *d, int r) { return FIELD(d, TECHDATA_COSTS + (r - 2) * 4, int); }
internal inline int techdata_get_category(const ee_tech_data *d) { return FIELD(d, TECHDATA_CATEGORY, int); } // +0x48 (7: a line upgrade)
internal inline int techdata_get_id(const ee_tech_data *d) { return FIELD(d, TECHDATA_ID, int); }      // +0x4C
internal inline int techdata_get_effects(const ee_tech_data *d) { return FIELD(d, TECHDATA_EFFECTS, int); } // +0x50 its effect list (effect_list(i))
internal inline int techdata_get_unit_type(const ee_tech_data *d) { return FIELD(d, TECHDATA_UNIT_TYPE, int); } // +0x6C the unit type it makes (< 0: a tech)
// +0x7D it counts as one of an epoch's new things (EETechTree::vf4)
internal inline uint8_t techdata_get_counts(const ee_tech_data *d) { return FIELD(d, TECHDATA_COUNTS, uint8_t); }
internal inline void techdata_set_counts(ee_tech_data *d, uint8_t v) { FIELD(d, TECHDATA_COUNTS, uint8_t) = v; }
// +0x81 Art of Conquest: granted by its epoch's line upgrades (only_new)
internal inline uint8_t techdata_get_aoc_new(const ee_tech_data *d) { return FIELD(d, TECHDATA_AOC_NEW, uint8_t); }

// AN EFFECT LIST (G_EPOCH_TECHS[i], a tech's techdata_get_effects): +0x4 the effect ids, +0x8 their count
internal inline unsigned *effects_get_ids(const ee_epoch_techs *list) { return FIELD(list, EPOCHTECHS_IDS, unsigned *); }
internal inline unsigned effects_get_count(const ee_epoch_techs *list) { return FIELD(list, EPOCHTECHS_COUNT, unsigned); }

// a tech effect (tech_effect(id)): +0x20 its kind (2 an attribute, 0x13 a line upgrade), +0x14 the unit type it is on
// (-1 all; a line upgrade's old type), +0x18 a line upgrade's new type, +0x2C the attribute
internal inline int effect_get_type(const ee_effect *ef) { return FIELD(ef, EFFECT_TYPE, int); }
internal inline int effect_get_new_type(const ee_effect *ef) { return FIELD(ef, EFFECT_NEW_TYPE, int); }
internal inline int effect_get_kind(const ee_effect *ef) { return FIELD(ef, EFFECT_KIND, int); }
internal inline int effect_get_attr(const ee_effect *ef) { return FIELD(ef, EFFECT_ATTR, int); }

// FUNCTIONS
internal inline ee_tech *tech_lookup(ee_tech_tree *tt, int id) { return (ee_tech *)GF(FN_TECH_LOOKUP)(tt, id); }
// can it be researched now (EETechTreeNode's vtable)
internal inline char tech_available(ee_tech *t) { return ((char (__thiscall *)(void *))(*(void ***)t)[GA(SLOT_TECH_AVAILABLE)])(t); }
// researched for nothing (EETechTreeNode::vf6) - the 2017 update's, AoC's takes an argument
internal inline int tech_grant(ee_tech *t) { return GF(FN_TECH_GRANT)(t); }
// the tech tree's next epoch (to the game's end epoch: it sets it and sends the epoch event)
internal inline void epoch_next(ee_tech_tree *tt) { GF(FN_EPOCH_NEXT)(tt); }
// the epoch's line upgrades granted (node: the epoch's tech, cat 7); Art of Conquest: only_new - only the ones with
// data +0x81 set (econ.c PAID UPGRADES)
internal inline void epoch_upgrades_grant(ee_tech *node, int cat, int only_new) {
    if (build_is("aoc")) ((void (__thiscall *)(void *, int, int))GA(FN_EPOCH_UPGRADES))(node, cat, only_new);
    else ((void (__thiscall *)(void *, int))GA(FN_EPOCH_UPGRADES))(node, cat);
}

// HOOKS
// the epoch's upgrades: the 2017 update's (node, category), Art of Conquest's (node, category, only new)
typedef void (__thiscall *epoch_upgrades_hook)(ee_tech *node, int cat);
typedef void (__thiscall *epoch_upgrades_aoc_hook)(ee_tech *node, int cat, int only_new);
internal inline int patch_epoch_upgrades(epoch_upgrades_hook h, epoch_upgrades_aoc_hook h_aoc) {
    return patch_call(GA(CALL_EPOCH_UPGRADES), GA(FN_EPOCH_UPGRADES), build_is("aoc") ? (void *)h_aoc : (void *)h);
}
// a tech researched for nothing (EETechTreeNode::vf6, its vtable slot VTS_TECH_RESEARCHED)
typedef int (__thiscall *tech_grant_hook)(ee_tech *t);
internal inline int patch_tech_grant(tech_grant_hook h) { return patch_vtable(GA(VTS_TECH_RESEARCHED), 0, GA(FN_TECH_GRANT), (void *)h); }
