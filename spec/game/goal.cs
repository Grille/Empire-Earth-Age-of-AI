/// a unit's goal, its +0x1EC (EEUnitGoal, EEUG*)
[c_type("ee_goal")]
unsafe struct EEUnitGoal
{
    /// (aim.c: 2 while it attacks)
    [offset(0x5), found(b)]
    uint8 state;

    [offset(0x20), found(c)]
    float32 dest_x;

    /// a kind 6 goal's target
    /// a kind 6 goal's target - the contact test (EEPhysicsModel::vf14, AoC vf15) lets two units at work on kind 6 meet
    /// only when it is the same
    [offset(0x38), found(c)]
    void* _0x38;

    /// a kind 4 goal's target
    /// a kind 4 goal's target - the same for kind 4
    [offset(0x3C), found(c)]
    void* _0x3c;

    [offset(0x50), found(c)]
    EEUnit* holder;
}
