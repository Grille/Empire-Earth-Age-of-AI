/// a formation's kind, the formation buttons in their order (the factory 0x4D84xx numbers them so; EEFShortLine,
/// EEFLine, EEFWedge, EEFCircle ...). The loose line and the staggered line are the loose ones (the player, 2026-11-03)
[c_prefix("FORMATION")]
enum formation_kind : int32 { short_line, long_line, loose_line, loose_staggered_line, deep_wedge, shallow_wedge, inward_circle, outward_circle }

/// a group's formation, its +0x34 (EEFormation, EEFLine, EEFWedge ...)
[c_type("void")]
unsafe struct EEFormation
{
    /// the formation's centre {x, y, z} - a group move's clicked point (both builds' EEFLine::vf2 read it)
    [offset(0x48), found(c)]
    vec3 center;

    /// its kind, the formation button (the factory 0x4D84xx; both builds' EEFormation ctor)
    [offset(0x58), found(c)]
    formation_kind kind;

    /// EEFormation::vf1 thiscall(formation, group, vector *slots), slot 1 of every formation's vtable (EEFLine,
    /// EEFShortLine, EEFWedge, EEFCircle ... 12): the group's units into a list, then the kind's vf2 (EEFLine
    /// 0x4D79D9) fills the slots - formation.size_formation_slot each {x, y, z, unit, facing} - from FORMATION_CENTER (the
    /// clicked point, set by the group's dispatch 0x5DB641 just before, 0x5DBC89), every row and column spaced by the
    /// group's largest type +0x470; a line's front row on the centre, the rows behind it. The dispatch then pairs
    /// units and slots (nearest, or by their order along the move) and gives each unit an EEUGLocation at its slot
    /// (0x5DC902)
    [address((ee)0x004D7E0B, (aoc)0x004DDDD5)]
    extern void slots(void* group, void* slots);
}

/// a formation's slot (formation_slots)
[c_type("void")]
unsafe struct ee_formation_slot
{
    /// a formation slot's point {x, y, z} (EEFLine::vf2 in both builds)
    [offset(0x0), found(c)]
    vec3 pos;

    /// the slot's unit (the same)
    [offset(0xC), found(c)]
    EEUnit* unit;

    /// the slot's facing (the dispatch passes it to the unit's EEUGLocation)
    [offset(0x10), found(c)]
    uint32 facing;
}

/// formation's data of the game, places in its code and numbers
static unsafe class formation
{
    /// formation_slots: a slot {x, y, z, unit, facing} (both builds' EEFLine::vf2 steps 0x14)
    [value(20)]
    static uint32 size_formation_slot;

    /// the formations' vtables: formation_slots
    [value(1)]
    static uint32 slot_formation_slots;
}

/// the game's functions formation.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class formation_functions
{
}
