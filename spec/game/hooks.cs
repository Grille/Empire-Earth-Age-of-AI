/// the draw list's records, hooks.draw_old (0x148 each)
[c_type("void")]
unsafe struct ee_draw_record
{
    /// the same in AoC: its draw list sites read hooks.draw_old + 0x58 and + 0xFC (0x50B2AD, 0x50EF07)
    [offset(0x58), found(h, "the same in AoC: its draw list sites read hooks.draw_old + 0x58 and + 0xFC (0x50B2AD, 0x50EF07)")]
    block _0x58;

    /// the same in AoC: its draw list sites read hooks.draw_old + 0x58 and + 0xFC (0x50B2AD, 0x50EF07)
    [offset(0xFC), found(h, "the same in AoC: its draw list sites read hooks.draw_old + 0x58 and + 0xFC (0x50B2AD, 0x50EF07)")]
    block _0xfc;

    /// a draw list record
    [address((ee)0x00506442, (aoc)0x005118E9)]
    extern void init();
}

/// hooks's data of the game, places in its code and numbers
static unsafe class hooks
{
    [address((ee)0x006177CF, (aoc)0x0062A366)]
    static code call_attack_alert;

    /// the loading splash's start (jne)
    [address((ee)0x00534400, (aoc)0x0053F2FE)]
    static code code_splash_start_jne;

    /// the loading splash's stop (jne)
    [address((ee)0x0053443D, (aoc)0x0053F33B)]
    static code code_splash_stop_jne;

    [address((ee)0x008D2D30, (aoc)0x008E94D0)]
    static block draw_old;

    /// the draw list's operands to move (drawlist.c, in its table's order)
    [address((ee)0x0050099C, (aoc)0x0050B497)]
    [address((ee)0x00504158, (aoc)0x0050F093)]
    [address((ee)0x005007B2, (aoc)0x0050B2AD)]
    [address((ee)0x005007CC, (aoc)0x0050B2C7)]
    [address((ee)0x00503C69, (aoc)0x0050EBA4)]
    [address((ee)0x00503ED0, (aoc)0x0050EE0B)]
    [address((ee)0x00503FCC, (aoc)0x0050EF07)]
    [address((ee)0x005043B5, (aoc)0x0050F2F0)]
    [address((ee)0x0050DC5A, (aoc)0x0051854F)]
    static code draw_sites;

    /// the small objects' draw list: 0x600 records of 0x3C
    [address((ee)0x008FBD30, (aoc)0x009124D0)]
    static block draw_small_old;

    /// the small objects' draw list's operands to move (drawlist.c, in its table's order)
    [address((ee)0x00500BBC, (aoc)0x0050B6B7)]
    [address((ee)0x0050419D, (aoc)0x0050F0D8)]
    [address((ee)0x00503C5A, (aoc)0x0050EB95)]
    [address((ee)0x00504016, (aoc)0x0050EF51)]
    [address((ee)0x00504041, (aoc)0x0050EF7C)]
    [address((ee)0x005041AD, (aoc)0x0050F0E8)]
    [address((ee)0x005043A6, (aoc)0x0050F2E1)]
    static code draw_small_sites;

    [address((ee)0x005D0B2C, (aoc)0x005E1438)]
    static code exit_confirm_at;

    /// __thiscall, ret 4
    [address((ee)0x00441432, (aoc)0x00440E57)]
    static code fn_add_movie;

    [address((ee)0x00519BC1, (aoc)0x00524497)]
    static code fn_attack_alert;

    /// the float 1/600 (the minimap's y scale operand)
    [address((ee)0x0082CBCC, (aoc)0x00842374)]
    static block float_1_600;

    /// the float 1/800 (the minimap's x scale operand)
    [address((ee)0x008358A8, (aoc)0x0084B940)]
    static block float_1_800;

    [address((ee)0x00824644, (aoc)0x00838638)]
    static block iat_srand;

    /// call AddMovie(path) in PlayIntro 0x538867
    [address((ee)0x00538909, (aoc)0x00543A57)]
    [address((ee)0x0053892A, (aoc)0x00543A78)]
    [address((ee)0x00538945, (aoc)0x00543A93)]
    static code intro_calls;

    /// the minimap's operands of hooks.float_1_800
    [address((ee)0x0065E01D, (aoc)0x00675618)]
    [address((ee)0x0065E8A3, (aoc)0x00675E95)]
    [address((ee)0x0065EB3E, (aoc)0x00676130)]
    static code minimap_fx_ops;

    /// the minimap's operands of hooks.float_1_600
    [address((ee)0x0065E032, (aoc)0x0067562D)]
    [address((ee)0x0065E8CE, (aoc)0x00675EC0)]
    [address((ee)0x0065EB68, (aoc)0x0067615A)]
    static code minimap_fy_ops;

    /// GEFontBitmap::Draw (both overloads): movsx of a character
    [value((ee)0x27A46, (aoc)0x15C01)]
    static uint32 rva_lle_font_movsx_1;

    [value((ee)0x27C9A, (aoc)0x15E55)]
    static uint32 rva_lle_font_movsx_2;

    /// its import slot
    [value((ee)0x940EC, (aoc)0x970EC)]
    static uint32 rva_lle_iat_wfmo;

    /// ISInitialize's, setting up the device
    [value((ee)0x4A76F, (aoc)0x39607)]
    static uint32 rva_lle_input_coop;

    /// Low-Level Engine, ISMouse::Resume: the 5 of push 5 (DISCL_ flags)
    [value((ee)0x4C838, (aoc)0x3BA05)]
    static uint32 rva_lle_mouse_coop;

    /// NESynchronousServer::Send: call [WaitForMultipleObjects] (turbo)
    [value((ee)0x55DF5, (aoc)0x49DDA)]
    static uint32 rva_lle_send_wait;

    /// je over the CloseWindow call on WM_ACTIVATEAPP(0)
    [address((ee)0x00662666, (aoc)0x00679C57)]
    static code site_close_window_je;

    /// the graphical goal manager (0x4FCDF9): cmp [selection + count], 10 - with 10 or more units selected their own
    /// goal markers are left out; the 10 its last byte (+6)
    [address((ee)0x004FCE70, (aoc)0x00507940)]
    static code site_goal_markers_max;

    [address((ee)0x006601F1, (aoc)0x006777E5)]
    static code site_editor_menu;

    [address((ee)0x0065FD6D, (aoc)0x00677361)]
    static code site_editor_res;

    [address((ee)0x00553348, (aoc)0x00561459)]
    static code site_sim_receive;

    [address((ee)0x005533C5, (aoc)0x005614D6)]
    static code site_sim_wait;
}

/// the game's functions hooks.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class hooks_functions
{
    [address((ee)0x0065F314, (aoc)0x006768FE), stdcall]
    // the other calls of set_resolution (display.c)
    [call_site("other_resolution", (ee)0x0065F30A, (aoc)0x006768F4)]
    [call_site("other_resolution", (ee)0x0065F61B, (aoc)0x00676C0A)]
    [call_site("other_resolution", (ee)0x0065F945, (aoc)0x00676F39)]
    [call_site("other_resolution", (ee)0x0065FAD4, (aoc)0x006770C8)]
    [call_site("other_resolution", (ee)0x00660CD3, (aoc)0x0067831D)]
    [call_site("other_resolution", (ee)0x00661329, (aoc)0x00678973)]
    [call_site("other_resolution", (ee)0x00661363, (aoc)0x006789AD)]
    static extern void set_resolution(int32 w, int32 h, int32 force, int32 flag);
}

/// TCP Protocol.dll's code (each build ships its own): its places relative to the DLL's base
[dll("TCP Protocol.dll")]
static unsafe class tcp_protocol
{
    /// the network ports' immediates (UDP 33334, TCP 33335), moved for a test slot
    [address((ee)0x000017E2, (aoc)0x00001684)]
    [address((ee)0x0000196F, (aoc)0x0000185C)]
    [address((ee)0x000019EB, (aoc)0x000018D8)]
    [address((ee)0x00001EB8, (aoc)0x00001DA5)]
    [address((ee)0x00001FBD, (aoc)0x00001EAA)]
    [address((ee)0x00002148, (aoc)0x00002035)]
    [address((ee)0x0000279C, (aoc)0x00002689)]
    [address((ee)0x00002F2C, (aoc)0x00002E1C)]
    static code ports;
}
