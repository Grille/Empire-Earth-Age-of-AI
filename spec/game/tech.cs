/// G_EFFECTS' records
unsafe struct ee_effect
{
    /// the tech tree code's reads (unchanged)
    [offset(0x14), found(v)]
    int32 type;

    /// the tech tree code's reads (unchanged)
    [offset(0x18), found(v)]
    int32 new_type;

    /// the tech tree code's reads (unchanged)
    [offset(0x20), found(v)]
    int32 kind;

    /// the tech tree code's reads (unchanged)
    [offset(0x2C), found(v)]
    int32 attr;
}

/// G_EPOCH_TECHS' records
[c_prefix("effects")]
unsafe struct ee_epoch_techs
{
    /// the tech tree code's reads (unchanged)
    [offset(0x4), found(v)]
    uint32* ids;

    /// the tech tree code's reads (unchanged)
    [offset(0x8), found(v)]
    uint32 count;
}

/// a tech, FN_TECH_LOOKUP
[c_type("ee_tech")]
unsafe struct EETechTreeNode
{
    /// (vf6 sets it)
    /// researched - EETechTreeNode::vf6 sets it (0x5BF335 / 0x5CFA53)
    [offset(0x4), found(c)]
    uint8 researched;

    /// (vf1, vf6 test it)
    /// available - vf1 and vf6 test it
    [offset(0x6), found(c)]
    uint8 available;

    /// the tech tree code's reads (unchanged)
    [offset(0x8), found(v)]
    ee_tech_data* data;

    /// its tree
    [offset(0xC), found(c)]
    EETechTree* tree;

    /// the tech tree code's reads (unchanged)
    [offset(0x14), found(v)]
    int32 epoch;

    [offset(0x24), found(v)]
    uint8* kids_begin;
}

/// a tech's data, TECH +8 (costs, its building ...)
[c_prefix("techdata")]
unsafe struct ee_tech_data
{
    /// the tech tree code's reads (unchanged)
    [offset(0x10), found(v)]
    int32* makers_begin;

    /// the tech tree code's reads (unchanged)
    [offset(0x24), found(v)]
    block costs;

    /// (7: a line upgrade)
    /// the tech tree code's reads (unchanged)
    [offset(0x48), found(v)]
    int32 category;

    /// the tech tree code's reads (unchanged)
    [offset(0x4C), found(v)]
    int32 id;

    /// its effect list (effect_list(i))
    /// the tech tree code's reads (unchanged)
    [offset(0x50), found(v)]
    int32 effects;

    /// the unit type it makes (< 0: a tech)
    /// the tech tree code's reads (unchanged)
    [offset(0x6C), found(v)]
    int32 unit_type;

    /// the tech tree code's reads (unchanged)
    [offset(0x7D), found(v)]
    uint8 counts;

    /// AoC's epoch upgrades (standard only) grant only a tech with it set: dbtechtree byte 0xF7, clear on civ power and
    /// special unit upgrades (0 on all of the 2017 update's) - read in AoC only
    [offset(0x81), found(h, "AoC's epoch upgrades (standard only) grant only a tech with it set: dbtechtree byte 0xF7, clear on civ power and special unit upgrades (0 on all of the 2017 update's) - read in AoC only")]
    uint8 aoc_new;
}

/// a player's tech tree, player +0x9D4 (EETechTree)
[c_prefix("tt")]
unsafe struct EETechTree
{
    [offset(0x4), found(v)]
    int32 player_index;

    /// the epoch records, 0x54 apart (AoC: one more, its 0x4E4 one record on)
    [offset(0x8), found(s)]
    block epoch_recs;

    /// the current epoch (0 = Prehistoric)
    [offset((ee)0x4E4, (aoc)0x538), found(v)]
    int32 epoch;
}

/// the tech tree's epoch records, TT + 8 + epoch * 0x54
[c_prefix("ttrec")]
unsafe struct ee_tt_epoch
{
    /// the tech tree code's reads (unchanged)
    [offset(0x8), found(v)]
    ee_tech_data* tech_data;

    /// the tech tree code's reads (unchanged)
    [offset(0x40), found(v)]
    int32 needed;

    /// the tech tree code's reads (unchanged)
    [offset(0x48), found(v)]
    int32 done;
}

/// tech's data of the game, places in its code and numbers (raw/'s address rows)
static unsafe class tech
{
    /// the call of FN_EPOCH_UPGRADES
    [address((ee)0x0043D365, (aoc)0x0043C9FC)]
    static code call_epoch_upgrades;

    /// thiscall(node, category), AoC's (+ only new): econ.c
    [address((ee)0x005BDDBD, (aoc)0x005CE8C5)]
    static code fn_epoch_upgrades;

    /// EETechTreeNode: char thiscall(tech)
    [value(1)]
    static uint32 slot_tech_available;

    /// EETechTreeNode::vf6's vtable slot
    [address((ee)0x00830874, (aoc)0x00846168)]
    static block vts_tech_researched;
}

/// the game's functions tech.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class tech_functions
{
    /// thiscall(tech tree): the next epoch (to the game's end epoch) - 0x5BC893 sets it, sends the epoch event
    [address((ee)0x005BC15D, (aoc)0x005CCB34), thiscall]
    static extern void epoch_next(EETechTree* tt);

    /// AoC's takes an argument: called in the 2017 update only
    [address((ee)0x005BF335, (aoc)0x005CFA53), thiscall]
    static extern int32 tech_grant(EETechTreeNode* t);

    [address((ee)0x004019A1, (aoc)0x004018A4), thiscall]
    static extern EETechTreeNode* tech_lookup(EETechTree* tt, int32 id);
}
