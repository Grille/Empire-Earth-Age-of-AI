/// the camera, G_CAMERA (EECameraControl)
[c_type("ee_camera")]
unsafe struct EECameraControl
{
    [offset(0x78), found(s)]
    float32 x;

    [offset(0xE0), found(s)]
    float32 zoom;
}

/// a graphical effect (a command's circle), [G_GFX_EFFECTS] by its handle (FN_GFX_GET)
[c_type("void")]
unsafe struct ee_gfx
{
    /// 0x663138's writes (both builds; FN_ORDER_MARKS' the same): the player index that sees it
    [offset(0x20), found(c)]
    int32 owner;

    /// 0x663138's writes (both builds; FN_ORDER_MARKS' the same), 1
    [offset(0x2D), found(c)]
    uint8 shown;

    /// 0x663138's writes (both builds; FN_ORDER_MARKS' the same), 0
    [offset(0x2E), found(c)]
    uint8 _0x2e;

    /// EEE3DCommandConfirmation vf12 (port_fields 9 votes): the record's +0x1C, x the unit's TD_MARK_SIZE x 1.25
    [offset(0x74), found(c)]
    float32 size;

    /// EEE3DCommandConfirmation vf12 (port_fields 5 votes): the record's +0x18, with a unit its TD_MARK_SIZE x 1.25
    [offset(0x78), found(c)]
    float32 size_2;
}

/// the game cursor, [G_MOUSE] (RTSMouse) - not the DirectInput device (E_MOUSE)
[c_type("ee_mouse")]
unsafe struct RTSMouse
{
    [offset(0x4), found(c)]
    float32 x;

    [offset(0x8), found(c)]
    float32 y;

    /// as the 2017 update (were rows of RM, which this is not: not checked in AoC)
    [offset(0x14), found(s)]
    float32 y_max;

    /// as the 2017 update (were rows of RM, which this is not: not checked in AoC)
    [offset(0x18), found(s)]
    float32 x_min;

    /// as the 2017 update (were rows of RM, which this is not: not checked in AoC)
    [offset(0x1C), found(s)]
    float32 x_max;

    /// as the 2017 update (were rows of RM, which this is not: not checked in AoC)
    [offset(0x20), found(s)]
    float32 y_min;

    [offset(0xA4), found(c)]
    uint8 frozen;
}

/// the object of FN_NATIVE_CHAT
[c_prefix("nativechat")]
unsafe struct ee_native_chat
{
    [offset(0x20), found(s)]
    EEPlayer* player;
}

/// the scroll action (EEAScroll)
[c_type("ee_scroll")]
unsafe struct EEAScroll
{
    /// elapsed (10 ms)
    [offset(0x40), found(v)]
    uint32 ticks;

    [offset(0x44), found(v)]
    block keys;
}

/// the UI manager's current form
[c_prefix("uiform")]
unsafe struct ee_ui_form
{
    /// votes 6/7
    [offset((ee)0x5D8, (aoc)0x5E8), found(h, "votes 6/7")]
    int32 top_bar;
}

/// a single-player menu screen (EEUIESP*)
[c_prefix("uiscreen")]
unsafe struct ee_ui_screen
{
    [offset(0xF8), found(v)]
    uint8** controls;
}
