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
