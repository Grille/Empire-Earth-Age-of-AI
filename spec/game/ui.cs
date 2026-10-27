/// the user interface, [game.ui] (its RTTI's; the spec called it EEWorld until 2026-10-25 - the world is game.world):
/// the action queue, the chat lines, its thread
[c_type("ee_ui"), vtable((ee)0x008359A4, (aoc)0x0084BA40)]
unsafe struct EEUserInterface
{
    /// the queued UI actions: queue_action 0x66192F pushes onto it (0x402314) under actions_lock; the constructor
    /// builds three deques (+0x4, +0x18, +0x2C), the destructor 0x65F156 drains each under its lock
    [offset(0x4), found(c)]
    deque actions;

    /// its thread (TSThread::Post)
    /// +0x1C and +0x5C the same in AoC
    [offset(0x40), found(b)]
    void* thread;

    /// its critical section (TSScheduler::CreateCriticalSection in the constructor; +0x48 and +0x4C the other deques'),
    /// entered by queue_action 0x66192F
    [offset(0x44), found(c)]
    void* actions_lock;

    /// the map's seed
    [offset(0x54), found(b)]
    uint32 seed;

    [address((ee)0x0066192F, (aoc)0x00678F79)]
    extern void queue_action(void* action);

    [address((ee)0x00661B04, (aoc)0x00679139)]
    extern void screenshot(int32 size, int32 run_on, int32 zero);

    /// thiscall(ui [G_UI], colour, name, line, ms, 0): a chat or system line
    [address((ee)0x00661E91, (aoc)0x006794C6)]
    extern void line(void* rgb, void* name, void* line, int32 ms, int32 zero);
}

/// the camera, game.camera (EECameraControl)
[c_type("ee_camera")]
unsafe struct EECameraControl
{
    /// the look-at point after the last move, x
    [offset(0x40), found(c)]
    float32 last_x;

    /// the same, y
    [offset(0x44), found(c)]
    float32 last_y;

    /// the camera's position, +0x34 its last one: 0x51D46C lerps last to it by t; AoC's setter writes it with the
    /// look-at +0x78 and snaps +0x34 when its length is at most 0.001
    [offset(0x6C), found(c), guess]
    vec3 eye;

    [offset(0x78), found(s)]
    float32 x;

    /// the look-at point's y (x +0x78)
    [offset(0x7C), found(c)]
    float32 y;

    /// 0x51EE75 advances it by dt x +0xDC up to 1.0 and passes it as t to the lerps 0x51D46C / 0x51D56A and to 0x51D2F2
    /// (the view matrix +0x84)
    [offset(0xD8), found(c), guess]
    float32 blend;

    [offset(0xE0), found(s)]
    float32 zoom;

    /// thiscall on game.camera (float dx, float dy)
    [address((ee)0x0051E2E6, (aoc)0x00528EA5)]
    extern void move(float32 dx, float32 dy);
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

    /// the cursor's speed factor
    [offset(0x88), found(c)]
    float32 speed;

    /// the screen's height in pixels
    [offset(0x8C), found(c)]
    int32 screen_h;

    /// the screen's width in pixels
    [offset(0x90), found(c)]
    int32 screen_w;

    /// the wheel's value this frame (0 without a wheel)
    [offset(0x94), found(c)]
    int32 wheel;

    [offset(0xA4), found(c)]
    uint8 frozen;

    [address((ee)0x00510E5D, (aoc)0x0051B3CC)]
    // the calls of mouse_update
    [call_site("mouse_update", (ee)0x004B8ABB, (aoc)0x004C11FE)]
    [call_site("mouse_update", (ee)0x0050FEC8, (aoc)0x0051A487)]
    [call_site("mouse_update", (ee)0x005111CF, (aoc)0x0051B73E)]
    [call_site("mouse_update", (ee)0x006618B8, (aoc)0x00678F02)]
    extern void update();

    [address((ee)0x00510DAD, (aoc)0x0051B31C)]
    extern void clamp();
}

/// the object of native_chat_say
unsafe struct ee_native_chat
{
    /// the head of its map line -> the time a line stays muted (the map at +0x4, the constructor 0x484E25): say
    /// 0x484EB3 finds the line's base string id (0x4850B2) against it and skips it while now <= the node's +0x14;
    /// 0x484FD8 inserts now + 5000
    [offset(0x8), found(c), guess]
    void* said_until_head;

    /// uint32 per line group 0..5 (the constructor sizes it to 8 of -1, 0x4D0945): the variant (0..9) said last, so say
    /// picks another
    [offset(0x14), found(c), guess]
    vector last_variant;

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

    /// its player's update_slot (the constructor copies player +0xCA0): say adds it to the time before % 10 to pick a
    /// variant
    [offset(0x24), found(c)]
    int32 update_slot;
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

    [address((ee)0x004050FC, (aoc)0x0040478E)]
    extern void execute();
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

/// the scenario editor's form (EEUIFScenarioEditor)
[c_type("ee_editor_form"), vtable((ee)0x0082F7B4, (aoc)0x00845084)]
unsafe struct EEUIFScenarioEditor
{
    [address((ee)0x0058D3E1, (aoc)0x0059D615)]
    [call_site("editor_form", (ee)0x0065BA13, (aoc)0x00673E9B)]
    extern void* init(void* owner);
}

/// the save-game action (EEASaveGame)
[c_type("ee_save_action"), vtable((ee)0x00825084, (aoc)0x00839060)]
unsafe struct EEASaveGame
{
    [address((ee)0x00404FA1, (aoc)0x00404613)]
    extern void* init(ustring* path, int32 flag, ustring dir);
}

/// the centre-the-camera action (EEACenterCamera)
[c_type("ee_center_camera"), vtable((ee)0x0082D2F0, (aoc)0x00842BA0)]
unsafe struct EEACenterCamera
{
    /// EEACenterCamera ctor (0x1C bytes)
    [address((ee)0x00560179, (aoc)0x0056FAD4)]
    extern void* init();

    /// thiscall (tile x, tile y): set target + queue (only if tile valid)
    [address((ee)0x00401F3E, (aoc)0x00401EE0)]
    extern void set(int32 x, int32 y);
}

/// the cheat codes' parser, [game.cheat_parser] (EECheatCodes)
[c_type("ee_cheat_codes"), vtable((ee)0x0082C744, (aoc)0x00841EE4)]
unsafe struct EECheatCodes
{
    /// thiscall([game.cheat_parser], UWideString *, int *code, int *arg)
    [address((ee)0x00520A17, (aoc)0x0052B5D6)]
    extern void parse(void* wtext, int32* code, int32* arg);
}

/// the graphical effects (a command's circles), [ui.gfx_effects]: handle -> effect (EEEffectManager)
[c_type("ee_effect_mgr"), vtable((ee)0x0082C164, (aoc)0x008411C0)]
unsafe struct EEEffectManager
{
    /// thiscall([ui.gfx_effects], unit or 0, float pos[3], kind, int *handle, 0): a graphical effect added (0: done)
    [address((ee)0x004FB6D4, (aoc)0x00506038)]
    // EEAMouseClick::vf3's gfx_add: the command's circle
    [call_site("click_mark", (ee)0x00404A26, (aoc)0x0040416B)]
    extern int32 add(void* on, float32* pos, void* kind, int32* handle, int32 arg);

    /// thiscall([ui.gfx_effects], handle): NULL gone
    [address((ee)0x004FBE3E, (aoc)0x00506852)]
    extern ee_gfx* get(int32 handle);

    /// thiscall([ui.gfx_effects], handle)
    [address((ee)0x004FBEB2, (aoc)0x005068C6)]
    extern int32 remove(int32 handle);
}

/// ui's data of the game, places in its code and numbers
static unsafe class ui
{
    /// the graphical effects (a command's circles): handle -> effect
    [address((ee)0x00918E1C, (aoc)0x0093185C)]
    static EEEffectManager* gfx_effects;

    /// UIEditBox::GetText
    [import_slot("Low-Level Engine.dll", "?GetText@UIEditBox@@QBEPBDK@Z")]
    [address((ee)0x00823328, (aoc)0x00837EE0)]
    static block iat_edit_get_text;

    /// UIForm::AddHotKey(key, 3 masks, 4 events, K, bool)
    [import_slot("Low-Level Engine.dll", "?AddHotKey@UIForm@@QAEXKKKKPAVUIEvent@@000K_N@Z")]
    [address((ee)0x00824048, (aoc)0x00837BE0)]
    static block iat_uiform_add_hotkey;

    /// the UI manager's current form
    [import_slot("Low-Level Engine.dll", "?GetCurrentForm@UIManager@@QBEPAVUIForm@@XZ")]
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
    [address((ee)0x00526B85, (aoc)0x005318BD), cdecl]
    [call_site("player_color", (ee)0x005262E4, (aoc)0x00531057)]
    static extern void* game_color_rgb(uint32 idx);

    /// a graphical effect's record by its id (enum gfx_kind)
    [address((ee)0x00404C83, (aoc)0x0040432F), cdecl]
    static extern void* gfx_kind_game(uint32 p1);

    /// cdecl(group, 0, owner (a player's +0x45C), how (0xBB: a GFX_ORDER_ON circle at each unit's goal), 0): a
    /// command's circles
    [address((ee)0x004B12A2, (aoc)0x004B86CD), cdecl]
    // the group's unload order (0x5DB641): order_marks
    [call_site("unload_marks", (ee)0x005DCDBD, (aoc)0x005ED592)]
    static extern void order_marks(void* grp, void* on, int32 owner, int32 how, uint32 goal);

}
