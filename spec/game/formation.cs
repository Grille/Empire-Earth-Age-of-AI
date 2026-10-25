/// a group's formation, its +0x34 (EEFormation, EEFLine, EEFWedge ...)
[c_type("void")]
unsafe struct EEFormation
{
    /// the formation's centre {x, y, z} - a group move's clicked point (both builds' EEFLine::vf2 read it)
    [offset(0x48), found(c)]
    vec3 center;

    /// 0 ShortLine, 1 LongLine, 2 LooseLine, 3 StaggeredLine, 4 DeepWedge, 5 ShallowWedge, 6 InwardCircle, 7
    /// OutwardCircle (the factory 0x4D84xx; both builds' EEFormation ctor)
    [offset(0x58), found(c)]
    int32 kind;
}

/// a formation's slot (FN_FORMATION_SLOTS)
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
