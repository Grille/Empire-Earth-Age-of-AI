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

    /// this PC's bar on the multiplayer loading screen (form 0x17): the lobby slot flagged local (+0x34's low byte),
    /// set when the screen opens (EEUIESSEOpen::vf1 0x5D12AC; kept if no slot is) - the loading percentage this PC
    /// sends names it (EEMLoadPercent +0x14, 0x6620A1), every PC's screen indexes its bars with it (0x661793)
    [offset(0x58), found(c)]
    int32 loading_bar;

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

    /// port_check: 19 reads in the paired camera code, AoC the same
    [offset(0x78), found(c)]
    float32 x;

    /// the look-at point's y (x +0x78)
    [offset(0x7C), found(c)]
    float32 y;

    /// its transformation (a GETransformation, 4x4 floats by rows: the eye's place in the world at +0xC, +0x1C, +0x2C),
    /// made by make_transformation at blend in the camera's step (0x51EE75, AoC 0x529A34) alone, which also sets the
    /// device's view from it: the camera's move (move) only sets where it goes and snap. The step runs in the
    /// simulation thread's loop, between its steps (0x546CAE) - in a light game every few ms, so the move looked as if
    /// it set it at once; in the player's 8000-unit save it stood for up to six frames of 120 (the pipe's picture
    /// camtrace, 2026-11-05). AoC's offset by its other fields' (not looked at)
    [offset(0x84), found(c)]
    block transformation;

    /// 0x51EE75 advances it by dt x blend_rate up to 1.0 (snap: at once) and passes it as t to the lerps 0x51D46C /
    /// 0x51D56A and to make_transformation (the view matrix +0x84); 0x51E79C sets it 0 for a new move
    /// (the AoC decompilation: mFraction)
    [offset(0xD8), found(c), guess]
    float32 blend;

    /// blend's speed, per second of the step's dt (0x51E79C: from the distance to go; AoC 0x529A34 the same)
    [offset(0xDC), found(c)]
    float32 blend_rate;

    /// 0x51E79C sets it from its record (the move: 1 - not a ride along a path); the step then puts blend at 1.0 at once
    /// and clears it (AoC 0x529A34 the same)
    [offset(0xFC), found(c)]
    uint8 snap;

    /// thiscall (m, t): the camera's transformation at blend t into m - the eye and look-at lerped from their last
    /// places (+0x34, +0x40) to the new (+0x6C, +0x78), the turn slerped, the eye kept above the ground (0x51CC45, the
    /// map's heights); only the camera's fields and the map read, m written. AoC 0x527EB1 the same code
    [address((ee)0x0051D2F2, (aoc)0x00527EB1)]
    extern void make_transformation(void* m, float32 t);

    /// port_check: 10 reads in the paired camera code, AoC the same
    [offset(0xE0), found(c)]
    float32 zoom;

    /// thiscall on game.camera (float dx, float dy)
    [address((ee)0x0051E2E6, (aoc)0x00528EA5)]
    extern void move(float32 dx, float32 dy);

    /// the zoom (+0xE0) set and the camera's move to it started (0x51DDD7, AoC 0x528996): the step's blend (+0xD8)
    /// turns and moves the transformation to it a step at a time
    [address((ee)0x0051F2F3, (aoc)0x00529EB2)]
    extern void zoom_to(float32 zoom);
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

    /// clamp 0x510DAD / AoC 0x51B31C: y kept below +0x14 in both
    [offset(0x14), found(c)]
    float32 y_max;

    /// clamp 0x510DAD / AoC 0x51B31C: x kept from +0x18 in both
    [offset(0x18), found(c)]
    float32 x_min;

    /// clamp 0x510DAD / AoC 0x51B31C: x kept below +0x1C in both
    [offset(0x1C), found(c)]
    float32 x_max;

    /// clamp 0x510DAD / AoC 0x51B31C: y kept from +0x20 in both
    [offset(0x20), found(c)]
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

    /// say 0x484EB3 / AoC 0x48966C: [+0x20] + 0x438 and + 0x24, the same in both
    [offset(0x20), found(c)]
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

/// an arrow key's scrolling (EEUIEKeyboardScroll, 0x10 bytes: vtable, refcount, 0, direction): a hotkey's event of the
/// game's form (0x64D0AB), run by the form's input poll on the game thread while the key is down
[c_type("ee_key_scroll"), vtable((ee)0x008348FC, (aoc)0x0084A998)]
unsafe struct EEUIEKeyboardScroll
{
    /// 0 up, 1 down, 2 left, 3 right
    [offset(0xC), found(c)]
    int32 direction;

    /// moves game.camera by (scroll_speed + 0.5) * (scroll_setting + 0.5) * 0.05 in its direction (camera_move:
    /// up is +dy, left -dx). UIEvent's Execute takes two dwords (ret 8 in both builds) this one never reads - a hook
    /// without them crashed the game on the first arrow key (2026-10-28)
    [slot(1), address((ee)0x005C9070, (aoc)0x005D9B6F)]
    [vtable_site("key_scroll_execute")]
    extern uint32 execute(uint32 a, uint32 b);
}

// THE USER INTERFACE: Low-Level Engine.dll's UI classes (UIManager, UIForm, UIControl and its kinds, UIEvent) build
// every screen from the UI database (data.ssa's db/dbui*.dat: dbuiforms one row per form - its id the row, +0xCC -,
// dbuicontrols the controls with their form and rectangle, dbuiback the backgrounds, dbuifonts, dbuicontrolevents /
// dbuiformevents the events by id, dbuihotkey the keys). The exe adds its own: EEUIManager ([game.ui_manager]) makes
// form 11 an EEUIFMainGame and the controls of types 1000.. (the minimap, the status buttons, the unit bars ...), and
// its event creators turn an event id into the exe's event object (EEUIE*: a button's handler, its execute). The
// scenario editor builds its form in code (editor.cs). The classes here, the forms and controls of the exe
// (ui_forms.cs), the events (ui_events.cs) and the DLL's UI functions the exe imports (ui_imports.cs): their vtables,
// constructors and sizes found in both builds by scripts/re/ui_classes.py (--events: the creators' ids), the slots'
// names by the DLL's exported vtables (scripts/re/dll_vtable.py). A layout changes by the control's rectangle
// (set_rectangle, set_location; set_display_mode hides one) once its form is built, a form by a hook on its open or
// its constructor, a button by a hook on its event's execute.

/// the forms of the UI database by their id (dbuiforms.dat: the row, its name there; 0 and 1 empty rows)
[c_prefix("FORM")]
enum ui_form_id : int32
{
    /// "Test form"
    test = 2,
    /// "In Game Scenario & Save Game Screen"
    save_game,
    /// "In Game Scenario & Load Game Screen"
    load_game,
    /// "Game Settings Screen"
    game_settings,
    /// "Single Player Menu"
    single_player,
    /// "CivBuilder Main Form"
    civ_builder,
    /// "Play Campaign Form"
    play_campaign,
    /// "Game Tools Menu"
    game_tools,
    /// "Choose Campaign Screen"
    choose_campaign,
    /// "Scott Main Game Form": the game's screen (EEUIFMainGame)
    main_game,
    /// "Main Menu Screen"
    main_menu,
    /// "Single Player Load Game Screen"
    load_single,
    /// "Campaign Screen"
    campaign,
    /// "CivBuilder New Civilization Form"
    civ_new,
    /// "CivBuilder Save Civilization Form"
    civ_save,
    /// "CivBuilder Load Civilization Form"
    civ_load,
    /// "Chat In Game pop-up dialog"
    chat,
    /// "Scenario Intro Screen(Briefing Screen)"
    scenario_intro,
    /// "Scenario Load Screen"
    scenario_load,
    /// "WON.net Stat Screen"
    won_stats,
    /// "DS Debug Sound Screen"
    debug_sound,
    /// "Splash Screen": also the multiplayer loading screen (EEUserInterface.loading_bar)
    splash,
    /// "Campaign Editor Form"
    campaign_editor,
    /// "Post Game Summary Screen"
    post_game,
    /// "Multi/Single Setup Screen(Lobby)": the game setup screen
    setup,
    /// "Diplomacy Screen"
    diplomacy,
    /// "In-game Options form"
    options,
    /// "Chat Log Form"
    chat_log,
    /// "Multiplayer Menu Form"
    multiplayer,
    /// "Available Rooms (Sierra) Form"
    sierra_rooms,
    /// "Available Games (Sierra) Form"
    sierra_games,
    /// "Campaign Scenario Select Form"
    campaign_scenario,
    /// "Choose Custom Campaign Form"
    custom_campaign,
    /// "Predefined Civ Form"
    predefined_civ,
    /// "Create Sierra Account Form"
    sierra_create_account,
    /// "Update Sierra Account Form"
    sierra_update_account,
    /// "Select Server Form"
    select_server,
    /// "Demo Outro Form"
    demo_outro
}

/// a form (a screen or a dialog), Low-Level Engine.dll's UIForm: made from its row of dbuiforms.dat (UIManager's
/// create_form), the UI manager's current one (UIManager::GetCurrentForm, ui.iat_uim_form). Its vtable is the DLL's;
/// the exe's forms (EEUIFMainGame, EEUIFScenarioEditor ...) derive from it. Its slots: the DLL's export names
[c_type("ee_ui_form")]
unsafe struct UIForm
{
    /// its controls by their index in the form (the single-player menu's name field: entry 8, a UIEditBox); the event
    /// creators read entry 4 (0x65A46D)
    [offset(0xF8), found(v)]
    uint8** controls;

    /// the control that has the keyboard (an edit box clicked into), NULL none: GetKeyboardCaptured, set by
    /// SetKeyboardCaptured from UIControl::CaptureKeyboard / ReleaseCapturedKeyboard; the game runs its own F-key
    /// hotkeys only while it is NULL (EEUIManager::update_keyboard 0x65C99D), not the form's
    [offset(0x108), found(c)]
    UIControl* keyboard_captured;

    /// the mouse in the form's coordinates (those of its controls' rectangles; the scenario editor's the screen's
    /// pixels, y down): GetMouseLocation, set by UpdateMouseLocation every update, which then finds the control under
    /// it by GERectangle::Contains (UIControl +0x18). (Its result, GetMouseOverControl +0x118, waits for a click while
    /// a control has the keyboard)
    [offset(0xE4), found(c)]
    float32 mouse_x;

    [offset(0xE8), found(c)]
    float32 mouse_y;

    // VIRTUAL FUNCTIONS (slot 0 the destructor)

    [slot(1)]
    extern void activate();

    [slot(2)]
    extern void add_ime_result_string(void* text);

    [slot(3)]
    extern int32 change_ime_composition_string(void* text, void* attributes, uint32 cursor);

    [slot(4)]
    extern int32 close_ime_composition_string();

    [slot(5)]
    extern void deactivate();

    /// a GERectangle
    [slot(6)]
    extern int32 draw_background(void* rect);

    [slot(7)]
    extern int32 open_ime_composition_string();

    /// the form laid out anew in rect (a GERectangle: the screen)
    [slot(8)]
    extern void relayout_form(void* rect);

    [slot(9)]
    extern void set_help_text_popup_location();

    [slot(10)]
    extern void update_information();

    [slot(11)]
    extern int32 update_keyboard();

    [slot(12)]
    extern int32 update_mouse();

    /// a typed key (its text, its flags)
    [slot(13)]
    extern int32 update_translated_key(char8* key, int16 flags);

    [slot(14)]
    extern void close();

    [slot(15)]
    extern int32 copy_data();

    [slot(16)]
    extern int32 draw_polygons();

    [slot(17)]
    extern int32 draw_text();

    /// the form opened (shown): where a form of the exe places what it builds itself (EEUIFScenarioEditor's lays out
    /// its panels, dll/game/api/editor_layout.h)
    [slot(18)]
    extern void open();

    [slot(19)]
    extern void set_rectangle(void* rect);
}

/// a control of a form, Low-Level Engine.dll's UIControl (UIEntity's): made from its row of dbuicontrols.dat - the
/// DLL's own kinds (UIButton, UITextLabel, UIEditBox, UIComboBox, a list box ...) or, for types 1000.., EEUIManager's
/// create_control. Its vtable is the DLL's; its slots by the DLL's export names (17 and 18 one empty function there:
/// by their order). Coordinates are floats in the form's design resolution (dbuiforms +0xE8 / +0xEC: 800x600 or
/// 1024x768), rectangles GERectangles, points GE2DPoints
[c_type("ee_ui_control")]
unsafe struct UIControl
{
    /// its place in the form, the GERectangle it was made with: bottom, left, right, top (the editor's panels put a
    /// control's next one below its bottom and right of its right, 0x58A70E; the units list's made as {y + h - 1, x,
    /// x + w - 1, y}, 0x5898C7)
    [offset(0x18), found(c)]
    float32 bottom;

    [offset(0x1C), found(c)]
    float32 left;

    [offset(0x20), found(c)]
    float32 right;

    [offset(0x24), found(c)]
    float32 top;

    /// the form it is in (UIControl::CaptureKeyboard captures it there)
    [offset(0x68), found(c)]
    UIForm* form;

    // VIRTUAL FUNCTIONS (slot 0 the destructor)

    [slot(1)]
    extern void add_ime_result_string(void* text);

    [slot(2)]
    extern int32 change_ime_composition_string(void* text, void* attributes, uint32 cursor);

    /// a GEFont
    [slot(3)]
    extern void change_font(void* font);

    [slot(4)]
    extern int32 close_ime_composition_string();

    [slot(5)]
    extern int32 copy_data();

    [slot(6)]
    extern int32 draw_polygons();

    [slot(7)]
    extern int32 draw_text();

    /// its help text into a UWideString; 0: none
    [slot(8)]
    extern uint8 get_help_string(void* text);

    [slot(9)]
    extern uint8 get_ime_enabled();

    [slot(10)]
    extern uint8 get_tabable();

    [slot(11)]
    extern uint32 get_type();

    [slot(12)]
    extern void hide_control();

    [slot(13)]
    extern int32 mouse_l_button_down();

    [slot(14)]
    extern int32 mouse_l_button_up();

    [slot(15)]
    extern int32 mouse_l_button_double_pressed();

    /// a GE2DPoint
    [slot(16)]
    extern int32 mouse_move(void* at);

    [slot(17)]
    extern int32 mouse_r_button_down();

    [slot(18)]
    extern int32 mouse_r_button_up();

    [slot(19)]
    extern int32 mouse_wheel_turned(int32 amount);

    [slot(20)]
    extern int32 open_ime_composition_string();

    [slot(21)]
    extern void release_captured_keyboard();

    [slot(22)]
    extern void release_captured_mouse();

    /// a GEColor
    [slot(23)]
    extern void set_bg_color(void* color);

    [slot(24)]
    extern void set_bg_opacity(float32 opacity);

    /// to be drawn again
    [slot(25)]
    extern void set_dirty(uint8 dirty);

    /// shown, hidden ... (the DLL's display modes)
    [slot(26)]
    extern void set_display_mode(uint32 mode);

    /// its top left corner (a GE2DPoint)
    [slot(27)]
    extern void set_location(void* at);

    /// its place and size (a GERectangle)
    [slot(28)]
    extern void set_rectangle(void* rect);

    [slot(29)]
    extern void tabbed_to();

    [slot(30)]
    extern void tabbed_from();

    [slot(31)]
    extern int32 update_keyboard();

    [slot(32)]
    extern int32 update_translated_key(char8* key, int16 flags);

    [slot(33)]
    extern int32 draw_border(void* rect, uint32 flags);
}

/// a button, Low-Level Engine.dll's UIButton (its vtable the DLL's): the slots it adds
[@base(typeof(UIControl)), c_type("ee_ui_button")]
unsafe struct UIButton
{
    [slot(34)]
    extern void button_down();

    [slot(35)]
    extern void button_up();

    [slot(36)]
    extern void mouse_over();
}

/// a form's background, Low-Level Engine.dll's UIBackground (dbuiback.dat's rows; EEUIManager's create_background
/// makes the exe's of type 0x65)
[c_type("ee_ui_background")]
unsafe struct UIBackground
{
    [slot(1)]
    extern void close();

    [slot(2)]
    extern void open();

    [slot(3)]
    extern void relayout_background();
}

/// what a control or a form does, Low-Level Engine.dll's UIEvent (a UReferenceCounter: vtable, refcount, 0): the
/// DLL runs a control's events when it is used (a button pressed), a form's when it opens or closes; the exe's are
/// made by EEUIManager's event creators from their id (ui_events.cs)
[size(0xC), c_type("ee_ui_event")]
unsafe struct UIEvent
{
    /// thiscall(event, the form, ?), ret 8 in both builds: what the event does (the single-player menu's buttons
    /// read the form: dll/engine/playername.c); slot 0 the destructor
    [slot(1)]
    extern uint32 execute(UIForm* form, uint32 arg);

    [slot(2)]
    extern int32 initialize_event();

    [slot(3)]
    extern int32 uninitialize_event();
}

/// the UI's manager, Low-Level Engine.dll's UIManager: the forms (by id: UIManager::GetFormByID, the stack of open
/// ones: Push / Pop / SwapForm - ui_imports.cs), the UI database. Its slots by the DLL's export names
[c_type("ee_uim")]
unsafe struct UIManager
{
    [slot(1)]
    extern int32 change_ime_candidate_window(void* list);

    [slot(2)]
    extern int32 change_ime_composition_string(void* text, void* attributes, uint32 cursor);

    [slot(3)]
    extern int32 close_ime_candidate_window();

    [slot(4)]
    extern int32 close_ime_composition_string();

    /// a background of a type the DLL doesn't know (its row of dbuiback.dat); the DLL's: none
    [slot(5)]
    extern UIBackground* create_user_defined_background(void* row);

    /// a control of a type the DLL doesn't know (its row of dbuicontrols.dat); the DLL's: none
    [slot(6)]
    extern UIControl* create_user_defined_control(void* row);

    /// a form from its row of dbuiforms.dat (a UIDbDataForms)
    [slot(7)]
    extern UIForm* create_form(void* row);

    [slot(8)]
    extern int32 open_ime_candidate_window();

    [slot(9)]
    extern int32 open_ime_composition_string();

    /// a GE2DPoint: the mouse
    [slot(10)]
    extern int32 update_ui(void* mouse);

    [slot(11)]
    extern int32 update_keyboard();

    [slot(12)]
    extern int32 update_mouse(void* mouse);
}

/// the game's UI manager, [game.ui_manager] (an object there, not a pointer): the DLL's UIManager with the exe's
/// forms, controls and event creators
[@base(typeof(UIManager)), vtable((ee)0x00834DE4, (aoc)0x0084AE90)]
unsafe struct EEUIManager
{
    /// the event creators (ui.event_creators), a list of cdecl(form, id) -> UIEvent* or 0: the constructor pushes
    /// each (0x65DD92 onto +0x1C, both builds)
    [offset(0x1C), found(c)]
    vector event_creators;

    /// its constructor: the DLL's UIManager with the sound manager and the resources, then the event creators
    [address((ee)0x00659CDD, (aoc)0x00671319)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);

    // VIRTUAL FUNCTIONS

    /// type 0x65 (row +0x68): an EEUIBGenericBackground of the form +0x64
    [address((ee)0x0065BE8F, (aoc)0x00674564), overrides(typeof(UIManager))]
    extern UIBackground* create_user_defined_background(void* row);

    /// by the row's type (+0x110; its form +0x108): 1000 EEUIFMGStatusButton, 1001 EEUIMiniMap, 1003 EEUIFMGErrorLog,
    /// 1004 EEUIFMGStatusBar, 1005 EEUIFMGAttribute, 1007 EEUICSelectUnits, 1008 EEUICCommandUnits, 1009 EEUICPortrait,
    /// 1010 EEUICChatLine, 1011 EECBUIListBox, 1013 EEUICWonNetStatBox, 1014 EEUICUnitIconBar, 1015 EEUIBSavedGroup,
    /// 1016 EEUIPGSLineGraph, 1017 EEUICPing, 1018 EEUICWonNetRoomsBox, 1019 EEUICWonNetGamesBox
    [address((ee)0x0065BF72, (aoc)0x00674647), overrides(typeof(UIManager))]
    extern UIControl* create_user_defined_control(void* row);

    /// form 11 (row +0xCC) an EEUIFMainGame (0x64C8B0), every other the DLL's
    [address((ee)0x0065C5B3, (aoc)0x00674C94), overrides(typeof(UIManager))]
    extern UIForm* create_form(void* row);

    [address((ee)0x0065C99D, (aoc)0x006750A9), overrides(typeof(UIManager))]
    extern int32 update_keyboard();

    [address((ee)0x0065CA9F, (aoc)0x006751AB), overrides(typeof(UIManager))]
    extern int32 update_mouse(void* mouse);
}

/// the save-game action (EEASaveGame)
[c_type("ee_save_action"), vtable((ee)0x00825084, (aoc)0x00839060)]
unsafe struct EEASaveGame
{
    [address((ee)0x00404FA1, (aoc)0x00404613)]
    extern void* init(ustring* path, int32 flag, ustring dir);
}

/// the multiplayer menu's host or join action (EEAJoinOrHostAGame, 0x38 bytes): its Execute (slot 1, 0x402BB8) checks
/// the player's name (+0x10) and the game's (+0x24: no control characters, ' " \ @, no space first, last or twice -
/// else an error box), makes [game.server] - host (mode 0): an EEServer that publishes the game; join by address
/// (mode 2): an EEClient that connects to the address in +0x24 - and opens the game setup screen (0x1A). The
/// command line's -hostgame / -joinbyip queue one too (parse_cmdline 0x5227E4 / 0x52290D), but then replace the
/// server they made with another client: the game hangs in the menus
[c_type("ee_join_action"), vtable((ee)0x00824C34, (aoc)0x00838C08)]
unsafe struct EEAJoinOrHostAGame
{
    /// EEAJoinOrHostAGame(mode, player name, game name or address) - both UWideStrings, copied
    [address((ee)0x00402B1E, (aoc)0x00402086)]
    extern void init(int32 mode, wstring* player, wstring* game);

    /// 0 host, 1 join a detected game, 2 / 3 join by address, 4 host (another kind of server)
    [offset(0xC), found(c)]
    int32 mode;
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

    /// UIEditBox::UIEditBox(id, GEFont *, K, bool, const GERectangle &, UIForm *, the form's control index, K, bool,
    /// bool) - the trigger screen's (0x57ED81): (id, ui.font_edit, 5, 0, rect, form, index, 2, 1, 0) into 0x1E0 bytes,
    /// then the exe's vtable (UIEditBox's)
    [import_slot("Low-Level Engine.dll", "??0UIEditBox@@QAE@KPAVGEFont@@K_NABVGERectangle@@PAVUIForm@@KK11@Z")]
    [address((ee)0x00823D48, (aoc)0x00837D4C)]
    static block iat_edit_ctor;

    /// UIEditBox::UpdateKeyboard - the slot every edit box's vtable of the exe reaches (the vtables' slot 31 a jmp
    /// through it: the trigger screen's boxes, the chat line, the mod's): a form calls it for the box that has the
    /// keyboard
    [import_slot("Low-Level Engine.dll", "?UpdateKeyboard@UIEditBox@@UAEJXZ")]
    [address((ee)0x00823C50, (aoc)0x00837C4C)]
    static block iat_edit_keyboard;

    /// the font of the scenario editor's lists and edit boxes (the units list, the trigger screen's edit boxes)
    [address((ee)0x009197B4, (aoc)0x00931930)]
    static void* font_edit;

    /// UIEditBox's vtable (the exe's): its slots, 0..35
    [value(36)]
    static uint32 edit_box_slots;

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

    [address((ee)0x00825098, (aoc)0x0083907C)]
    static vftable vt_scroll;

    /// the UI manager's event creators (EEUIManager.event_creators), in its constructor's order (the fourth twice):
    /// cdecl(form, id) -> the UIEvent of a control event id they know (dbuicontrolevents.dat's rows, 0..251), else 0.
    /// Which makes which: each event's comment in ui_events.cs (scripts/re/ui_classes.py --events runs them for every
    /// id) - 0x65A1A5 the forms' open and close (1..10), 0x65A46D save and load (11..20), 0x65AF3A the game screen's
    /// buttons (22..27, 138..143, 241..249 ...), 0x65B496 the main menu (28..33), 0x65BCBD the single-player menu
    /// (35..40), 0x65B7DA the post-game summary (144..184) ...
    [address((ee)0x0065A9E6, (aoc)0x006726B3)]
    [address((ee)0x0065ABE0, (aoc)0x00672AB1)]
    [address((ee)0x0065AC0D, (aoc)0x00672ADE)]
    [address((ee)0x00659F31, (aoc)0x00671598)]
    [address((ee)0x0065A46D, (aoc)0x00671F5D)]
    [address((ee)0x0065AF3A, (aoc)0x00672DE6)]
    [address((ee)0x0065B496, (aoc)0x006734C5)]
    [address((ee)0x0065B684, (aoc)0x00673836)]
    [address((ee)0x0065B7DA, (aoc)0x00673A89)]
    [address((ee)0x0065BA24, (aoc)0x00673EAC)]
    [address((ee)0x0065BBE6, (aoc)0x0067406E)]
    [address((ee)0x0065BCBD, (aoc)0x00674200)]
    [address((ee)0x0065BDB8, (aoc)0x006743D2)]
    [address((ee)0x00659F31, (aoc)0x00671598)]
    [address((ee)0x0065C5EE, (aoc)0x00674CCF)]
    [address((ee)0x0065A81E, (aoc)0x006724EB)]
    [address((ee)0x0065A1A5, (aoc)0x00671A10)]
    static code event_creators;
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
