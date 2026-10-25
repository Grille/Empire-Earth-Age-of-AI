/// an AI town or resource zone
unsafe struct ee_zone
{
    /// .. +0xC its sites (ee_unit *)
    [offset(0x8), found(s)]
    void** sites_begin;

    /// a drop site serves it
    [offset(0x14), found(s)]
    int32 drop;

    [offset(0x18), found(s)]
    uint8 active;
}

/// the game's functions zone.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class zone_functions
{
    /// thiscall(zone, type) -> first site of that type with amount > 0
    [address((ee)0x004886F2, (aoc)0x0048D065), thiscall]
    static extern EEUnit* zone_site(ee_zone* z, int32 kind);
}
