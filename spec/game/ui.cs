/// the user interface, [game.ui] (its RTTI's; the spec called it EEWorld until 2026-10-25 - the world is game.world):
/// the action queue, the chat lines, its thread
[c_type("ee_ui")]
unsafe struct EEUserInterface
{
    /// its thread (TSThread::Post)
    /// +0x1C and +0x5C the same in AoC
    [offset(0x40), found(b)]
    void* thread;

    /// the map's seed
    [offset(0x54), found(b)]
    uint32 seed;
}

/// the camera, game.camera (EECameraControl)
[c_type("ee_camera")]
unsafe struct EECameraControl
{
    [offset(0x78), found(s)]
    float32 x;

    [offset(0xE0), found(s)]
    float32 zoom;
}

/// a graphical effect (a command's circle), [ui.gfx_effects] by its handle (gfx_get)
unsafe struct ee_gfx
{
    /// 0x663138's writes (both builds; order_marks' the same): the player index that sees it
    [offset(0x20), found(c)]
    int32 owner;

    /// 0x663138's writes (both builds; order_marks' the same), 1
    [offset(0x2D), found(c)]
    uint8 shown;

    /// 0x663138's writes (both builds; order_marks' the same), 0
    [offset(0x2E), found(c)]
    uint8 _0x2e;

    /// EEE3DCommandConfirmation vf12 (port_fields 9 votes): the record's +0x1C, x the unit's TD_MARK_SIZE x 1.25
    [offset(0x74), found(c)]
    float32 size;

    /// EEE3DCommandConfirmation vf12 (port_fields 5 votes): the record's +0x18, with a unit its TD_MARK_SIZE x 1.25
    [offset(0x78), found(c)]
    float32 size_2;
}

/// the game cursor, [game.mouse] (RTSMouse) - not the DirectInput device (E_MOUSE)
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

    [address((ee)0x00510E5D, (aoc)0x0051B3CC)]
    // the calls of mouse_update
    [call_site("mouse_update", (ee)0x004B8ABB, (aoc)0x004C11FE)]
    [call_site("mouse_update", (ee)0x0050FEC8, (aoc)0x0051A487)]
    [call_site("mouse_update", (ee)0x005111CF, (aoc)0x0051B73E)]
    [call_site("mouse_update", (ee)0x006618B8, (aoc)0x00678F02)]
    extern void update();
}

/// the object of native_chat_say
unsafe struct ee_native_chat
{
    [offset(0x20), found(s)]
    EEPlayer* player;

    [address((ee)0x00484EB3, (aoc)0x0048966C)]
    // the calls of native_chat_say (the AI's own lines)
    [call_site("native_chat", (ee)0x00470040, (aoc)0x00472F4C)]
    [call_site("native_chat", (ee)0x004715F8, (aoc)0x00474682)]
    [call_site("native_chat", (ee)0x0047166E, (aoc)0x00474705)]
    [call_site("native_chat", (ee)0x0047280E, (aoc)0x004758D5)]
    [call_site("native_chat", (ee)0x00472B9D, (aoc)0x00475C65)]
    extern char8 say(int32 @base, int32 ms);
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
unsafe struct ee_ui_form
{
    /// votes 6/7
    [offset((ee)0x5D8, (aoc)0x5E8), found(h, "votes 6/7")]
    int32 top_bar;
}

/// a single-player menu screen (EEUIESP*)
unsafe struct ee_ui_screen
{
    [offset(0xF8), found(v)]
    uint8** controls;
}

/// ui's data of the game, places in its code and numbers
static unsafe class ui
{
    /// the graphical effects (a command's circles): handle -> effect
    [address((ee)0x00918E1C, (aoc)0x0093185C)]
    static block gfx_effects;

    /// UIEditBox::GetText
    [address((ee)0x00823328, (aoc)0x00837EE0)]
    static block iat_edit_get_text;

    /// UIForm::AddHotKey(key, 3 masks, 4 events, K, bool)
    [address((ee)0x00824048, (aoc)0x00837BE0)]
    static block iat_uiform_add_hotkey;

    /// the UI manager's current form
    [address((ee)0x00823D60, (aoc)0x00837B44)]
    static block iat_uim_form;

    /// operand of push 0x19 (timer period in ms)
    [address((ee)0x004050C2, (aoc)0x00404734)]
    static code site_scroll_timer;

    /// camera_action_init
    [value(28)]
    static uint32 size_camera_action;

    /// EEUIEKeyboardScroll: vtable, refcount, 0, direction
    [value(16)]
    static uint32 size_key_scroll;

    /// ui.vt_scroll: Execute
    [value(1)]
    static uint32 slot_scroll_execute;

    /// ui.sp_vtables: a button's handler
    [value(1)]
    static uint32 slot_sp_button;

    /// their slot 1
    [address((ee)0x005D80D5, (aoc)0x005E8A41)]
    [address((ee)0x005D812B, (aoc)0x005E8B10)]
    [address((ee)0x00628E89, (aoc)0x0067CF38)]
    [address((ee)0x005D8156, (aoc)0x005E8B44)]
    [address((ee)0x005D88C0, (aoc)0x005E8B68)]
    [address((ee)0x005D817A, (aoc)0x005E8B86)]
    static code sp_fns;

    /// the single player screens' vtables (playername.c)
    [address((ee)0x00835740, (aoc)0x0084B814)]
    [address((ee)0x00835754, (aoc)0x0084B828)]
    [address((ee)0x00835768, (aoc)0x0084B83C)]
    [address((ee)0x00835560, (aoc)0x0084B65C)]
    [address((ee)0x0083577C, (aoc)0x0084B850)]
    [address((ee)0x00835790, (aoc)0x0084B864)]
    static vftable sp_vtables;

    /// EEUIEKeyboardScroll (0x10 bytes: vtable, refcount, 0, direction); Execute 0x5C9070 moves game.camera
    [address((ee)0x008348FC, (aoc)0x0084A998)]
    static vftable vt_key_scroll;

    [address((ee)0x00825098, (aoc)0x0083907C)]
    static vftable vt_scroll;
}

/// the game's functions ui.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class ui_functions
{
    /// EEACenterCamera ctor (0x1C bytes)
    [address((ee)0x00560179, (aoc)0x0056FAD4), thiscall]
    static extern void* camera_action_init(void* mem);

    /// thiscall (tile x, tile y): set target + queue (only if tile valid)
    [address((ee)0x00401F3E, (aoc)0x00401EE0), thiscall]
    static extern void camera_action_set(void* act, int32 x, int32 y);

    /// thiscall on game.camera (float dx, float dy)
    [address((ee)0x0051E2E6, (aoc)0x00528EA5), thiscall]
    static extern void camera_move_game(void* p1, float32 p2, float32 p3);

    /// thiscall([game.cheat_parser], UWideString *, int *code, int *arg)
    [address((ee)0x00520A17, (aoc)0x0052B5D6), thiscall]
    static extern void cheat_parse(void* parser, void* wtext, int32* code, int32* arg);

    [address((ee)0x00526B85, (aoc)0x005318BD), cdecl]
    [call_site("player_color", (ee)0x005262E4, (aoc)0x00531057)]
    static extern void* game_color_rgb(uint32 idx);

    /// thiscall([ui.gfx_effects], unit or 0, float pos[3], kind, int *handle, 0): a graphical effect added (0: done)
    [address((ee)0x004FB6D4, (aoc)0x00506038), thiscall]
    // EEAMouseClick::vf3's gfx_add: the command's circle
    [call_site("click_mark", (ee)0x00404A26, (aoc)0x0040416B)]
    static extern int32 gfx_add(void* fx, void* on, float32* pos, void* kind, int32* handle, int32 arg);

    /// thiscall([ui.gfx_effects], handle): NULL gone
    [address((ee)0x004FBE3E, (aoc)0x00506852), thiscall]
    static extern ee_gfx* gfx_get(void* p1, int32 p2);

    /// a graphical effect's record by its id (enum gfx_kind)
    [address((ee)0x00404C83, (aoc)0x0040432F), cdecl]
    static extern void* gfx_kind_game(uint32 p1);

    /// thiscall([ui.gfx_effects], handle)
    [address((ee)0x004FBEB2, (aoc)0x005068C6), thiscall]
    static extern int32 gfx_remove_game(void* p1, int32 p2);

    [address((ee)0x00510DAD, (aoc)0x0051B31C), thiscall]
    static extern void mouse_clamp(void* m);

    /// cdecl(group, 0, owner (a player's +0x45C), how (0xBB: a GFX_ORDER_ON circle at each unit's goal), 0): a
    /// command's circles
    [address((ee)0x004B12A2, (aoc)0x004B86CD), cdecl]
    // the group's unload order (0x5DB641): order_marks
    [call_site("unload_marks", (ee)0x005DCDBD, (aoc)0x005ED592)]
    static extern void order_marks(void* grp, void* on, int32 owner, int32 how, uint32 goal);

    [address((ee)0x004050FC, (aoc)0x0040478E), thiscall]
    static extern void scroll_execute(void* self);

    /// thiscall(ui [G_UI], colour, name, line, ms, 0): a chat or system line
    [address((ee)0x00661E91, (aoc)0x006794C6), thiscall]
    static extern void ui_line_game(void* p1, void* p2, void* p3, void* p4, int32 p5, int32 p6);
}
