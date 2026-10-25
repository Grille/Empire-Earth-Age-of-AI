/// the draw list's records, DRAW_OLD (0x148 each)
[c_type("void")]
unsafe struct ee_draw_record
{
    /// the same in AoC: its draw list sites read DRAW_OLD + 0x58 and + 0xFC (0x50B2AD, 0x50EF07)
    [offset(0x58), found(h, "the same in AoC: its draw list sites read DRAW_OLD + 0x58 and + 0xFC (0x50B2AD, 0x50EF07)")]
    block _0x58;

    /// the same in AoC: its draw list sites read DRAW_OLD + 0x58 and + 0xFC (0x50B2AD, 0x50EF07)
    [offset(0xFC), found(h, "the same in AoC: its draw list sites read DRAW_OLD + 0x58 and + 0xFC (0x50B2AD, 0x50EF07)")]
    block _0xfc;
}
