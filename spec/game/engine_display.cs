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

[c_type("ee_input_mouse")]
unsafe struct ISMouse
{
    [offset(0x4), found(c)]
    uint8 suspended;

    [offset(0x18), found(c)]
    block buttons;

    [offset(0x44), found(c)]
    void** device;
}

[c_type("ee_rasterizer")]
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

[c_type("ee_surface")]
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

/// engine_display's data of the game, places in its code and numbers
static unsafe class engine_display
{
    /// the graphics thread's Sleep(1) after every pass of its loop (the thread function RTSGraphics starts, 0x50FE16:
    /// a message, then a frame unless paused, then this): call [Sleep]
    [address((ee)0x0050FEE8, (aoc)0x0051A4A7)]
    static code call_frame_sleep;

    /// its call of ISMouse::update (one of that function's call sites): on the path that draws a frame only
    [address((ee)0x0050FEC8, (aoc)0x0051A487)]
    static code call_frame_mouse;

    /// the exe's import slot of Sleep
    [import_slot("kernel32.dll", "Sleep")]
    [address((ee)0x0082318C, (aoc)0x00837174)]
    static block iat_sleep;

    /// DX7HRTnLDisplay.dll: the caps of the vertex buffer each rasterizer state creates (D3DVERTEXBUFFERDESC.dwCaps, a
    /// mov [ebp-0xC], imm32: D3DVBCAPS_WRITEONLY - video memory; its other buffer and DX7HRDisplay.dll's add
    /// D3DVBCAPS_SYSTEMMEMORY)
    [value((ee)0xF475, (aoc)0x103CE)]
    static uint32 rva_tnl_vb_caps;
}
