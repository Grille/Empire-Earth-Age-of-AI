/// a cluster of a group's units (FN_GROUP_SECTOR_ROUTE's units)
[c_type("void")]
unsafe struct ee_unit_list
{
    /// FN_GROUP_SECTOR_ROUTE's reads (both builds): a vector of ee_unit * (begin, end +4)
    [offset(0x4), found(c)]
    block units;
}
