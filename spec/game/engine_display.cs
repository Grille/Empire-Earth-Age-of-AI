[c_prefix("dstate")]
unsafe struct ee_draw_state
{
    [offset(0x1C), found(c)]
    void* device;
}

[c_type("ee_font")]
unsafe struct GEFont
{
    [offset(0x18), found(c)]
    block hfont;

    [offset(0x24), found(c)]
    void* bitmap;
}

[c_type("ee_input_mouse"), c_prefix("imouse")]
unsafe struct ISMouse
{
    [offset(0x4), found(c)]
    uint8 suspended;

    [offset(0x18), found(c)]
    block buttons;

    [offset(0x44), found(c)]
    void** device;
}

[c_type("ee_rasterizer"), c_prefix("ras")]
unsafe struct DX7Rasterizer
{
    [offset(0x20), found(c)]
    ee_screen_state** screen_states;

    [offset(0x6C), found(c)]
    ee_draw_state** dstates;
}

[c_type("ee_screen")]
unsafe struct DX7Screen
{
    [offset(0x10), found(c)]
    uint8* modes_begin;

    /// IDirectDraw7 *
    [offset(0x30), found(c)]
    void* ddraw;

    [offset(0x38), found(c)]
    uint32 mode;

    [offset(0x3C), found(c)]
    uint8 desktop;

    [offset(0x3D), found(c)]
    uint8 fullscreen;
}

[c_prefix("scrstate")]
unsafe struct ee_screen_state
{
    [offset(0x2A8), found(c)]
    DX7Screen* screen;

    [offset(0x2AC), found(c)]
    DX7Surface* front;

    [offset(0x2B0), found(c)]
    DX7Surface* back;

    [offset(0x2BC), found(c)]
    GEWindow* window;

    [offset(0x2C4), found(c)]
    block stats;
}

[c_type("ee_surface"), c_prefix("surf")]
unsafe struct DX7Surface
{
    /// IDirectDrawSurface7 *
    [offset(0x48), found(c)]
    void* dds;

    [offset(0x54), found(c)]
    block dc;

    [offset(0x5D), found(c)]
    uint8 dc_out;
}

[c_type("ee_text")]
unsafe struct GEText
{
    [offset(0x34), found(c)]
    char8* @string;

    [offset(0x44), found(c)]
    GEFont* font;
}

[c_type("ee_window")]
unsafe struct GEWindow
{
    [offset(0x0), found(c)]
    block hwnd;

    [offset(0x4), found(c)]
    float32 _4;

    [offset(0x8), found(c)]
    float32 _8;

    [offset(0xC), found(c)]
    int32 width;

    [offset(0x10), found(c)]
    int32 height;
}
