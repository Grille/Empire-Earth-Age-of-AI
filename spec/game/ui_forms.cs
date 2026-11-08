// the exe's forms and controls (ui.cs: the UI, the DLL's classes they derive from). Each class's vtable, constructor
// and size found in both builds by scripts/re/ui_classes.py (a constructor's arguments by its ret: not typed yet); a
// slot of the DLL's classes that holds the exe's own function (not a jmp to the DLL's) is the class's [overrides].
// The game screen (form 11, EEUIFMainGame) and its parts (EEUIFMG*: the top bar, the status buttons, the groups of
// the bottom panel - EEUIFMGG* the chat, diplomacy, options, statistics and civilization builder dialogs), the
// controls EEUIManager makes for the UI database's types 1000.. (the minimap, the unit bars, the portrait ...),
// the menus' few own classes (EEUISI* the scenario intro's buttons, EEUIBColor the setup screen's colours ...) and
// the runtime library's templates the exe instantiates (UITextLabel ...).

[size((ee)0x3C, (aoc)0x3C), vtable((ee)0x008298C4, (aoc)0x0083E4F4)]
unsafe struct EEButtonObject
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x004CFBD5, (aoc)0x004D5498)]
    extern void* init();
}

/// control type 1011 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(UILBString)), size((ee)0x15C, (aoc)0x15C), vtable((ee)0x00835E00, (aoc)0x0084BED0)]
unsafe struct EECBUIListBox
{
    /// its constructor (9 arguments, not typed yet)
    [address((ee)0x00665930, (aoc)0x0067D187)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9);

    [address((ee)0x00665E57, (aoc)0x0067D6AE), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x00666186, (aoc)0x0067DAC8), overrides(typeof(UIControl))]
    extern int32 draw_text();

    [address((ee)0x00666D5D, (aoc)0x0067EF57), overrides(typeof(UIControl))]
    extern uint8 get_tabable();

    [address((ee)0x00666A30, (aoc)0x0067E37E), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x00666BD1, (aoc)0x0067E55A), overrides(typeof(UIControl))]
    extern void tabbed_to();
}

[size((ee)0x110, (aoc)0x110), vtable((ee)0x00835F78, (aoc)0x0084C060)]
unsafe struct EECampaignFormManager
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00675AD0, (aoc)0x0068F0CB)]
    extern void* init();
}

[size((ee)0x350, (aoc)0x350), vtable((ee)0x008310C4, (aoc)0x008469C0)]
unsafe struct EEPGSFormManager
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x005D3B95, (aoc)0x005E44DA)]
    extern void* init();
}

[@base(typeof(UIButton)), size((ee)0x108, (aoc)0x108), vtable((ee)0x00833B48, (aoc)0x00849BEC)]
unsafe struct EEUIBColor
{
    /// its constructor (5 arguments, not typed yet)
    [address((ee)0x0063E157, (aoc)0x006547E0)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5);

    [address((ee)0x0063E1E6, (aoc)0x00654892), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x0063E206, (aoc)0x006548B2), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();
}

[@base(typeof(UIBackground)), size((ee)0x98, (aoc)0x98), vtable((ee)0x00833BE0, (aoc)0x00849C84)]
unsafe struct EEUIBGenericBackground
{
    /// its constructor (12 arguments, not typed yet)
    [address((ee)0x0063E37C, (aoc)0x00654A05)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10, uint32 p11, uint32 p12);

    [address((ee)0x0063EFEC, (aoc)0x00655675), overrides(typeof(UIBackground))]
    extern void open();
}

/// made in 0x5C11B7, 0x5D342A, 0x5D73F9
[@base(typeof(UIBackground)), size((ee)0x84, (aoc)0x84), vtable((ee)0x00830914, (aoc)0x00846208)]
unsafe struct EEUIBMapBackground
{
}

/// control type 1015 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(UIButton)), size((ee)0x118, (aoc)0x118), vtable((ee)0x00833C08, (aoc)0x00849CB0)]
unsafe struct EEUIBSavedGroup
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x0063F5A1, (aoc)0x00655C2A)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);

    [address((ee)0x0063F74C, (aoc)0x00655DD5), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x0063F782, (aoc)0x00655E0B), overrides(typeof(UIControl))]
    extern int32 draw_polygons();

    [address((ee)0x0063F85E, (aoc)0x00655EE7), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x0063F7DD, (aoc)0x00655E66), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_double_pressed();

    [address((ee)0x0063F945, (aoc)0x00655FCE), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();
}

[@base(typeof(UIButton)), vtable((ee)0x00833CA4, (aoc)0x00849D54)]
unsafe struct EEUIBUpgrade
{
    /// its constructor (8 arguments, not typed yet)
    [address((ee)0x006401BC, (aoc)0x00656845)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8);

    [address((ee)0x006402BA, (aoc)0x00656980), overrides(typeof(UIButton))]
    extern void button_down();

    [address((ee)0x006402DC, (aoc)0x006569A2), overrides(typeof(UIButton))]
    extern void button_up();

    [address((ee)0x0064097A, (aoc)0x00657040), overrides(typeof(UIButton))]
    extern void mouse_over();

    [address((ee)0x0064041C, (aoc)0x00656AE2), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x00640458, (aoc)0x00656B1E), overrides(typeof(UIControl))]
    extern int32 draw_text();

    [address((ee)0x006405BD, (aoc)0x00656C83), overrides(typeof(UIControl))]
    extern uint8 get_help_string(void* text);

    [address((ee)0x00640894, (aoc)0x00656F5A), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x006409A4, (aoc)0x0065706A), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x006409BB, (aoc)0x00657081), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();

    [address((ee)0x00640A78, (aoc)0x0065713E), overrides(typeof(UIControl))]
    extern void set_dirty(uint8 dirty);
}

[vtable((ee)0x0082F65C, (aoc)0x00844F5C)]
unsafe struct EEUIBuildGate
{
    /// its constructor (2 arguments, not typed yet)
    [address((ee)0x0064C8B0, (aoc)0x00662E73)]
    extern void* init(uint32 p1, uint32 p2);
}

/// made in 0x58C831
[vtable((ee)0x0082F640, (aoc)0x00844F40)]
unsafe struct EEUIBuildWall
{
}

/// control type 1010 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(UIControl)), size((ee)0xD0, (aoc)0xD0), vtable((ee)0x00833D58, (aoc)0x00849DF8)]
unsafe struct EEUICChatLine
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x00640C0B, (aoc)0x00657223)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);

    [address((ee)0x00640EF1, (aoc)0x0065750E), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x006411FE, (aoc)0x0065781B), overrides(typeof(UIControl))]
    extern int32 draw_polygons();

    [address((ee)0x0064120F, (aoc)0x0065782C), overrides(typeof(UIControl))]
    extern int32 draw_text();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x00641F6C, (aoc)0x0065744C), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_to();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_from();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

/// control type 1008 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(UIControl)), size((ee)0xD0, (aoc)0xD0), vtable((ee)0x00833DE8, (aoc)0x00849E8C)]
unsafe struct EEUICCommandUnits
{
    /// its constructor (4 arguments, not typed yet)
    [address((ee)0x00641264, (aoc)0x00657881)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4);

    [address((ee)0x006413C8, (aoc)0x006579E5), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x00641630, (aoc)0x00657C4D), overrides(typeof(UIControl))]
    extern int32 draw_polygons();

    [address((ee)0x004B129F, (aoc)0x004AC9F3), overrides(typeof(UIControl))]
    extern uint8 get_tabable();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_double_pressed();

    [address((ee)0x0064198C, (aoc)0x00657FA9), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x00641B53, (aoc)0x00658170), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_to();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_from();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

/// control type 1017 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(UIControl)), size((ee)0x11C, (aoc)0x11C), vtable((ee)0x00833E78, (aoc)0x00849F1C)]
unsafe struct EEUICPing
{
    /// its constructor (4 arguments, not typed yet)
    [address((ee)0x00641DFE, (aoc)0x00658464)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4);

    [address((ee)0x00641EF6, (aoc)0x006585A6), overrides(typeof(UIControl))]
    extern int32 draw_polygons();

    [address((ee)0x004B129F, (aoc)0x004AC9F3), overrides(typeof(UIControl))]
    extern uint8 get_tabable();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x00641F6C, (aoc)0x0065744C), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_to();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_from();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

/// control type 1009 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(UIControl)), size((ee)0xC4, (aoc)0xC4), vtable((ee)0x00833F04, (aoc)0x00849FAC)]
unsafe struct EEUICPortrait
{
    /// its constructor (5 arguments, not typed yet)
    [address((ee)0x00641FBB, (aoc)0x0065861C)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5);

    [address((ee)0x00642326, (aoc)0x00658987), overrides(typeof(UIControl))]
    extern int32 draw_polygons();

    [address((ee)0x0064246F, (aoc)0x00658AD0), overrides(typeof(UIControl))]
    extern int32 draw_text();

    [address((ee)0x00642496, (aoc)0x00658AF7), overrides(typeof(UIControl))]
    extern uint8 get_help_string(void* text);

    [address((ee)0x00642657, (aoc)0x00658D02), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();

    [address((ee)0x004F43A5, (aoc)0x004FCA98), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x006428B2, (aoc)0x00658F5D), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x006428D2, (aoc)0x00658F7D), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x004F43A5, (aoc)0x004FCA98), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_to();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_from();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

/// control type 1007 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(UIControl)), size((ee)0xD4, (aoc)0xD4), vtable((ee)0x00833F90, (aoc)0x0084A038)]
unsafe struct EEUICSelectUnits
{
    /// its constructor (5 arguments, not typed yet)
    [address((ee)0x00642F2E, (aoc)0x0065958F)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5);

    [address((ee)0x00643114, (aoc)0x00659775), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x00643274, (aoc)0x006597E2), overrides(typeof(UIControl))]
    extern int32 draw_polygons();

    [address((ee)0x004B129F, (aoc)0x004AC9F3), overrides(typeof(UIControl))]
    extern uint8 get_tabable();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();

    [address((ee)0x0064366C, (aoc)0x00659BDA), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x00643583, (aoc)0x00659AF1), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_double_pressed();

    [address((ee)0x0064378E, (aoc)0x00659CFC), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_to();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_from();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

/// control type 1014 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(UIControl)), size((ee)0x144, (aoc)0x144), vtable((ee)0x0083401C, (aoc)0x0084A0C8)]
unsafe struct EEUICUnitIconBar
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x006438CF, (aoc)0x00659F7F)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);

    [address((ee)0x00643BA8, (aoc)0x0065A258), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x00643C7F, (aoc)0x0065A32F), overrides(typeof(UIControl))]
    extern int32 draw_polygons();

    [address((ee)0x0065E84A, (aoc)0x0065AE3B), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();

    [address((ee)0x006443EB, (aoc)0x0065AA9B), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x006447D9, (aoc)0x0065AFA5), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x0065E84A, (aoc)0x0065AE3B), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x006446E0, (aoc)0x0065AE44), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_to();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_from();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

/// control type 1019 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(UIControl)), size((ee)0x19C, (aoc)0x19C), vtable((ee)0x008340A8, (aoc)0x0084A154)]
unsafe struct EEUICWonNetGamesBox
{
    /// its constructor (8 arguments, not typed yet)
    [address((ee)0x00644C13, (aoc)0x0065B2CE)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8);

    [address((ee)0x00644FD3, (aoc)0x0065B68E), overrides(typeof(UIControl))]
    extern int32 copy_data();

    /// a jmp to 0x646E90 (AoC 0x65C28D) in both builds, the slot holds the jmp (the rooms and stats boxes the function)
    [address((ee)0x00644FF5, (aoc)0x0065B6B0), overrides(typeof(UIControl))]
    extern int32 draw_polygons();

    [address((ee)0x00644FFA, (aoc)0x0065B6B5), overrides(typeof(UIControl))]
    extern int32 draw_text();

    [address((ee)0x0059D985, (aoc)0x005AD441), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();

    [address((ee)0x0059DA20, (aoc)0x0065C4C4), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x0059D17E, (aoc)0x005AD4DC), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_double_pressed();

    [address((ee)0x00649012, (aoc)0x005AD5A8), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();

    [address((ee)0x006470AE, (aoc)0x005ADDD5), overrides(typeof(UIControl))]
    extern int32 mouse_wheel_turned(int32 amount);

    [address((ee)0x00648EC1, (aoc)0x005AD1F2), overrides(typeof(UIControl))]
    extern void set_location(void* at);

    [address((ee)0x006490E8, (aoc)0x005ADE79), overrides(typeof(UIControl))]
    extern void tabbed_to();

    [address((ee)0x0059D24A, (aoc)0x0065C61F), overrides(typeof(UIControl))]
    extern void tabbed_from();

    [address((ee)0x006470CC, (aoc)0x005AD327), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

/// control type 1018 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(UIControl)), size((ee)0x13C, (aoc)0x13C), vtable((ee)0x00834230, (aoc)0x0084A2DC)]
unsafe struct EEUICWonNetRoomsBox
{
    /// its constructor (8 arguments, not typed yet)
    [address((ee)0x006463C6, (aoc)0x0065CEAD)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8);

    [address((ee)0x0064653C, (aoc)0x0065D023), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x00646E90, (aoc)0x0065C28D), overrides(typeof(UIControl))]
    extern int32 draw_polygons();

    [address((ee)0x0064655E, (aoc)0x0065D045), overrides(typeof(UIControl))]
    extern int32 draw_text();

    [address((ee)0x0059D985, (aoc)0x005AD441), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();

    [address((ee)0x0059DA20, (aoc)0x0065C4C4), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x0059D17E, (aoc)0x005AD4DC), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_double_pressed();

    [address((ee)0x00649012, (aoc)0x005AD5A8), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();

    [address((ee)0x006470AE, (aoc)0x005ADDD5), overrides(typeof(UIControl))]
    extern int32 mouse_wheel_turned(int32 amount);

    [address((ee)0x00648EC1, (aoc)0x005AD1F2), overrides(typeof(UIControl))]
    extern void set_location(void* at);

    [address((ee)0x006490E8, (aoc)0x005ADE79), overrides(typeof(UIControl))]
    extern void tabbed_to();

    [address((ee)0x0059D24A, (aoc)0x0065C61F), overrides(typeof(UIControl))]
    extern void tabbed_from();

    [address((ee)0x006470CC, (aoc)0x005AD327), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

/// control type 1013 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(UIControl)), size((ee)0x14C, (aoc)0x14C), vtable((ee)0x008343C4, (aoc)0x0084A470)]
unsafe struct EEUICWonNetStatBox
{
    /// its constructor (8 arguments, not typed yet)
    [address((ee)0x00647C16, (aoc)0x0065E2F8)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8);

    [address((ee)0x00647D17, (aoc)0x0065E3F9), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x00646E90, (aoc)0x0065C28D), overrides(typeof(UIControl))]
    extern int32 draw_polygons();

    [address((ee)0x00647D31, (aoc)0x0065E413), overrides(typeof(UIControl))]
    extern int32 draw_text();

    [address((ee)0x0059D985, (aoc)0x005AD441), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();

    [address((ee)0x0059DA20, (aoc)0x0065C4C4), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x0059D17E, (aoc)0x005AD4DC), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_double_pressed();

    [address((ee)0x00649012, (aoc)0x005AD5A8), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();

    [address((ee)0x006470AE, (aoc)0x005ADDD5), overrides(typeof(UIControl))]
    extern int32 mouse_wheel_turned(int32 amount);

    [address((ee)0x00648EC1, (aoc)0x005AD1F2), overrides(typeof(UIControl))]
    extern void set_location(void* at);

    [address((ee)0x006490E8, (aoc)0x005ADE79), overrides(typeof(UIControl))]
    extern void tabbed_to();

    [address((ee)0x0059D24A, (aoc)0x0065C61F), overrides(typeof(UIControl))]
    extern void tabbed_from();

    [address((ee)0x006470CC, (aoc)0x005AD327), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

[size((ee)0x18, (aoc)0x18), vtable((ee)0x0083099C, (aoc)0x00846290)]
unsafe struct EEUIECLEFormManager
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x005C2B1E, (aoc)0x005D3269)]
    extern void* init();
}

[@base(typeof(UIButton)), size((ee)0x114, (aoc)0x114), vtable((ee)0x0083462C, (aoc)0x0084A6D8)]
unsafe struct EEUIFEBCancel
{
    /// its constructor (2 arguments, not typed yet)
    [address((ee)0x0064A975, (aoc)0x00660EB9)]
    extern void* init(uint32 p1, uint32 p2);

    [address((ee)0x0064AA94, (aoc)0x00660FD8), overrides(typeof(UIButton))]
    extern void button_up();

    [address((ee)0x0064A9F7, (aoc)0x00660F3B), overrides(typeof(UIControl))]
    extern int32 draw_polygons();
}

[@base(typeof(UIButton)), size((ee)0x110, (aoc)0x110), vtable((ee)0x00834594, (aoc)0x0084A640)]
unsafe struct EEUIFEBOK
{
    /// its constructor (2 arguments, not typed yet)
    [address((ee)0x0064A841, (aoc)0x00660D85)]
    extern void* init(uint32 p1, uint32 p2);

    [address((ee)0x0064A95A, (aoc)0x00660E9E), overrides(typeof(UIButton))]
    extern void button_up();

    [address((ee)0x0064A8BD, (aoc)0x00660E01), overrides(typeof(UIControl))]
    extern int32 draw_polygons();
}

[@base(typeof(UIForm)), size((ee)0x1BC, (aoc)0x1BC), vtable((ee)0x008346C4, (aoc)0x0084A770)]
unsafe struct EEUIFError
{
    /// its constructor (1 argument, not typed yet)
    [address((ee)0x0064AAB7, (aoc)0x00660FFB)]
    extern void* init(uint32 p1);

    [address((ee)0x0064B66B, (aoc)0x00661BAF), overrides(typeof(UIForm))]
    extern int32 draw_background(void* rect);

    [address((ee)0x0064B708, (aoc)0x00661C4C), overrides(typeof(UIForm))]
    extern void close();

    [address((ee)0x0064B678, (aoc)0x00661BBC), overrides(typeof(UIForm))]
    extern void open();
}

/// control type 1005 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(EEUIBUpgrade)), size((ee)0x138, (aoc)0x138), vtable((ee)0x008349DC, (aoc)0x0084AA78)]
unsafe struct EEUIFMGAttribute
{
    /// its constructor (8 arguments, not typed yet)
    [address((ee)0x00650FEA, (aoc)0x0066783D)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8);

    [address((ee)0x0065111F, (aoc)0x00667972), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x00651190, (aoc)0x006679E3), overrides(typeof(UIControl))]
    extern int32 draw_text();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_to();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_from();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

/// control type 1003 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(UILBString)), size((ee)0x13C, (aoc)0x13C), vtable((ee)0x00834A7C, (aoc)0x0084AB18)]
unsafe struct EEUIFMGErrorLog
{
    /// its constructor (6 arguments, not typed yet)
    [address((ee)0x00651286, (aoc)0x00667AD9)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6);

    [address((ee)0x006512E0, (aoc)0x00667B56), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();
}

[@base(typeof(EEUIFMGGroup)), size((ee)0x8, (aoc)0x8), vtable((ee)0x00834B3C, (aoc)0x0084ABD8)]
unsafe struct EEUIFMGGChat
{
    /// its constructor (1 argument, not typed yet)
    [address((ee)0x00651324, (aoc)0x00667B77)]
    extern void* init(uint32 p1);
}

[@base(typeof(EEUIFMGGroup)), size((ee)0x8, (aoc)0x8), vtable((ee)0x00834B58, (aoc)0x0084ABF4)]
unsafe struct EEUIFMGGCivBuilder
{
    /// its constructor (1 argument, not typed yet)
    [address((ee)0x00651363, (aoc)0x00667BB6)]
    extern void* init(uint32 p1);
}

[@base(typeof(EEUIFMGGroup)), size((ee)0x8, (aoc)0x8), vtable((ee)0x00834B74, (aoc)0x0084AC10)]
unsafe struct EEUIFMGGDiplomacy
{
    /// its constructor (1 argument, not typed yet)
    [address((ee)0x006513A2, (aoc)0x00667BF5)]
    extern void* init(uint32 p1);
}

[@base(typeof(EEUIFMGGroup)), size((ee)0x8, (aoc)0x8), vtable((ee)0x00834B90, (aoc)0x0084AC2C)]
unsafe struct EEUIFMGGOptions
{
    /// its constructor (1 argument, not typed yet)
    [address((ee)0x006513E1, (aoc)0x00667C34)]
    extern void* init(uint32 p1);
}

[@base(typeof(EEUIFMGGroup)), size((ee)0x8, (aoc)0x8), vtable((ee)0x00834BC8, (aoc)0x0084AC64)]
unsafe struct EEUIFMGGStatistics
{
    /// its constructor (1 argument, not typed yet)
    [address((ee)0x00651458, (aoc)0x00667CA8)]
    extern void* init(uint32 p1);
}

[vtable((ee)0x00834BAC, (aoc)0x0084AC48)]
unsafe struct EEUIFMGGroup
{
    /// its constructor (1 argument, not typed yet)
    [address((ee)0x00651423, (aoc)0x00667C73)]
    extern void* init(uint32 p1);
}

[@base(typeof(EEUIFMGGroup)), size((ee)0x24, (aoc)0x28), vtable((ee)0x00834BE4, (aoc)0x0084AC84)]
unsafe struct EEUIFMGMainGroup
{
    /// its constructor (1 argument, not typed yet)
    [address((ee)0x00651678, (aoc)0x00667EC8)]
    extern void* init(uint32 p1);
}

/// control type 1004 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(EEUIBUpgrade)), vtable((ee)0x00834C24, (aoc)0x0084ACC4)]
unsafe struct EEUIFMGStatusBar
{
    [address((ee)0x00652DA4, (aoc)0x00669896), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x00652E4D, (aoc)0x0066993F), overrides(typeof(UIControl))]
    extern int32 draw_polygons();

    [address((ee)0x00652E8D, (aoc)0x0066997F), overrides(typeof(UIControl))]
    extern int32 draw_text();

    [address((ee)0x00652F4E, (aoc)0x00669A40), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();

    [address((ee)0x00652FAA, (aoc)0x00669A9C), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x00653008, (aoc)0x00669AFA), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    [address((ee)0x00653099, (aoc)0x00669B8B), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x006530F4, (aoc)0x00669BE6), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();
}

/// control type 1000 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(UIButton)), size((ee)0x14C, (aoc)0x14C), vtable((ee)0x00834CF0, (aoc)0x0084AD98)]
unsafe struct EEUIFMGStatusButton
{
    /// its constructor (3 arguments, not typed yet)
    [address((ee)0x00656FF9, (aoc)0x0066DE97)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3);

    [address((ee)0x006573D3, (aoc)0x0066E274), overrides(typeof(UIButton))]
    extern void button_up();

    [address((ee)0x00657893, (aoc)0x0066E734), overrides(typeof(UIControl))]
    extern int32 copy_data();

    [address((ee)0x00657AB9, (aoc)0x0066EAF4), overrides(typeof(UIControl))]
    extern int32 draw_polygons();

    [address((ee)0x00657B7A, (aoc)0x0066EBB5), overrides(typeof(UIControl))]
    extern int32 draw_text();

    [address((ee)0x00657BA4, (aoc)0x0066EBDF), overrides(typeof(UIControl))]
    extern uint8 get_help_string(void* text);

    [address((ee)0x0065804C, (aoc)0x0066F186), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x0065806A, (aoc)0x0066F1A4), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();

    [address((ee)0x0065861E, (aoc)0x0066FB19), overrides(typeof(UIControl))]
    extern void set_location(void* at);

    [address((ee)0x0065867E, (aoc)0x0066FB79), overrides(typeof(UIControl))]
    extern void tabbed_from();
}

/// form 11 "Scott Main Game Form": the game's screen (EEUIManager.create_form makes it with push 0x5E4 / AoC 0x5F4;
/// every other form the DLL's UIForm)
[@base(typeof(UIForm)), size((ee)0x5E4, (aoc)0x5F4), c_type("ee_main_form")]
[vtable((ee)0x00834974, (aoc)0x0084AA10)]
unsafe struct EEUIFMainGame
{
    /// the top bar's text: 0 the age, 1 the game clock, 2 "FPS: n" (votes 6/7)
    [offset((ee)0x5D8, (aoc)0x5E8), found(h, "votes 6/7")]
    int32 top_bar;

    /// its constructor (2 arguments, not typed yet: the form's row and the manager)
    [address((ee)0x0064C8B0, (aoc)0x00662E73)]
    extern void* init(uint32 p1, uint32 p2);

    [address((ee)0x0064DCBF, (aoc)0x006648D4), overrides(typeof(UIForm))]
    extern void activate();

    [address((ee)0x0064E113, (aoc)0x00664D40), overrides(typeof(UIForm))]
    extern void deactivate();

    [address((ee)0x005248D4, (aoc)0x004E123D), overrides(typeof(UIForm))]
    extern void set_help_text_popup_location();

    [address((ee)0x00659B6F, (aoc)0x00671290), overrides(typeof(UIForm))]
    extern int32 update_keyboard();

    [address((ee)0x00659B97, (aoc)0x006712B8), overrides(typeof(UIForm))]
    extern int32 update_mouse();

    [address((ee)0x0064E0C4, (aoc)0x00664CF1), overrides(typeof(UIForm))]
    extern void close();

    [address((ee)0x0064E197, (aoc)0x00664DC4), overrides(typeof(UIForm))]
    extern int32 copy_data();

    [address((ee)0x0064E8DF, (aoc)0x0066550C), overrides(typeof(UIForm))]
    extern int32 draw_polygons();

    [address((ee)0x0064F409, (aoc)0x00666038), overrides(typeof(UIForm))]
    extern void open();
}

[@base(typeof(UIComboBox)), size((ee)0xF4, (aoc)0xF4), vtable((ee)0x00834718, (aoc)0x0084A7C4)]
unsafe struct EEUIFileComboBox
{
    /// its constructor (17 arguments, not typed yet)
    [address((ee)0x0064BF62, (aoc)0x006624A6)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5, uint32 p6, uint32 p7, uint32 p8, uint32 p9, uint32 p10, uint32 p11, uint32 p12, uint32 p13, uint32 p14, uint32 p15, uint32 p16, uint32 p17);
}

/// control type 1001 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(UIControl)), size((ee)0xE4, (aoc)0xE4), vtable((ee)0x0083581C, (aoc)0x0084B8B4)]
unsafe struct EEUIMiniMap
{
    /// the minimap's picture's top left in the form's pixels: a point on it is (the mouse - this) / (the window's size x
    /// 1/800, 1/600) (0x65EAFA, AoC 0x6760EC; the scale's operands hooks.minimap_fx_ops / fy_ops)
    [offset(0xA0), found(c)]
    float32 origin_x;

    [offset(0xA4), found(c)]
    float32 origin_y;

    /// its action, queued again for every click and drag
    [offset(0xD8), found(c)]
    EEAMiniMap* action;

    /// its constructor (5 arguments, not typed yet)
    [address((ee)0x0065DE66, (aoc)0x00675459)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5);

    /// run every frame by the render objects' update (UIManager::CopyData, the render thread)
    [address((ee)0x0065DF8A, (aoc)0x00675585), overrides(typeof(UIControl))]
    [vtable_site("minimap_copy_data")]
    extern int32 copy_data();

    [address((ee)0x0065E5A6, (aoc)0x00675BA1), overrides(typeof(UIControl))]
    extern int32 draw_polygons();

    [address((ee)0x0065E84A, (aoc)0x0065AE3B), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_down();

    [address((ee)0x0065E980, (aoc)0x00675F72), overrides(typeof(UIControl))]
    extern int32 mouse_l_button_up();

    [address((ee)0x0065EABD, (aoc)0x006760AF), overrides(typeof(UIControl))]
    extern int32 mouse_move(void* at);

    /// its action (+0xD8) set to the mouse's point (kind 0: the camera centred there) and queued (EEUserInterface's
    /// queue_action) - by the left button's up and by every mouse move while it is held
    [address((ee)0x0065EAFA, (aoc)0x006760EC)]
    [call_site("minimap_queue", (ee)0x0065E9BF, (aoc)0x00675FB1)]
    [call_site("minimap_queue", (ee)0x0065EAE6, (aoc)0x006760D8)]
    extern void queue_point();

    [address((ee)0x0065E853, (aoc)0x00675E45), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_down();

    [address((ee)0x0065EE50, (aoc)0x00675523), overrides(typeof(UIControl))]
    extern int32 mouse_r_button_up();

    [address((ee)0x0065EC04, (aoc)0x006761F6), overrides(typeof(UIControl))]
    extern void set_location(void* at);

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_to();

    [address((ee)0x0056278E, (aoc)0x005720C6), overrides(typeof(UIControl))]
    extern void tabbed_from();

    [address((ee)0x004D84A2, (aoc)0x005AD67E), overrides(typeof(UIControl))]
    extern int32 update_keyboard();
}

/// control type 1016 (dbuicontrols.dat +0x110): EEUIManager.create_control makes it
[@base(typeof(UIControl)), size((ee)0xD58, (aoc)0xD58), vtable((ee)0x008358C4, (aoc)0x0084B95C)]
unsafe struct EEUIPGSLineGraph
{
    /// its constructor (5 arguments, not typed yet)
    [address((ee)0x0065EE58, (aoc)0x00676442)]
    extern void* init(uint32 p1, uint32 p2, uint32 p3, uint32 p4, uint32 p5);
}

/// made in 0x5D6904
[size((ee)0xA8, (aoc)0xA8), vtable((ee)0x00831138, (aoc)0x00846A34)]
unsafe struct EEUISCIFormManager
{
}

[size((ee)0x8, (aoc)0x8), vtable((ee)0x008359E8, (aoc)0x0084BA84)]
unsafe struct EEWonNetAvailableGameFormManager
{
    /// its constructor (0 arguments, not typed yet)
    [address((ee)0x00662E72, (aoc)0x0067A37E)]
    extern void* init();
}

/// made in 0x59198C
[@base(typeof(UIButton)), size((ee)0x108, (aoc)0x108), vtable((ee)0x0082FA98, (aoc)0x00845368)]
unsafe struct UIBMap
{
    [address((ee)0x00591697, (aoc)0x005A18F1), overrides(typeof(UIButton))]
    extern void button_up();
}

/// made in 0x59B710, 0x5D23E7
[@base(typeof(UIControl)), size((ee)0xB0, (aoc)0xB0), vtable((ee)0x008300EC, (aoc)0x008459F0)]
unsafe struct UIBitmap
{
}

/// a check box's label (UICheckBox.label)
[c_type("ee_ui_label")]
unsafe struct ee_ui_label
{
    [offset(0x108), found(c)]
    ee_ui_label_text* text;
}

[c_type("ee_ui_label_text")]
unsafe struct ee_ui_label_text
{
    [offset(0x14), found(c)]
    float32 r;
    [offset(0x18), found(c)]
    float32 g;
    [offset(0x1C), found(c)]
    float32 b;
    /// 1: the colour is used
    [offset(0x4C), found(c)]
    uint8 colored;
}

/// made in 0x56575B, 0x5672AC, 0x569046, 0x56E14F, 0x571B93, 0x57ED81, 0x583705, 0x58D3E1
[@base(typeof(UIButton)), size((ee)0x10C, (aoc)0x10C), vtable((ee)0x0082D948, (aoc)0x008431F8)]
unsafe struct UICheckBox
{
    /// its label (the editor's own boxes: yellow, 0x56575B)
    [offset(0x104), found(c)]
    ee_ui_label* label;

    /// GetIsChecked
    [offset(0x108), found(c)]
    uint8 checked_state;
}

/// made in 0x569046, 0x56D39A, 0x56EA99, 0x571B93, 0x572A2A, 0x57ED81, 0x583705, 0x58D3E1, 0x59198C
[@base(typeof(UIControl)), size((ee)0xE0, (aoc)0xE0), vtable((ee)0x0082DB74, (aoc)0x00843424)]
unsafe struct UIComboBox
{
    /// the box its text is shown in (SetTextBoxBgOpacity: its vf24)
    [offset(0xC8), found(c)]
    UIControl* text_box;

    /// the line selected, 0xFFFFFFFF none (GetCurrentSelection)
    [offset(0xD0), found(c)]
    uint32 current_selection;

    /// the map panel sets it to 1 on its lists of fixed lines (0x59198C)
    [offset(0xDB), found(c), guess]
    uint8 fixed_lines;
}

/// made in 0x560EC7, 0x569046, 0x56B75E, 0x56E14F, 0x571B93, 0x583705, 0x59198C, 0x64AAB7
[@base(typeof(UIControl)), size((ee)0x1E0, (aoc)0x1E0), vtable((ee)0x0082D3E8, (aoc)0x00842C98)]
unsafe struct UIEditBox
{
}

/// made in 0x58E400
[size((ee)0x2C, (aoc)0x2C), vtable((ee)0x0082F808, (aoc)0x008450EC)]
unsafe struct UIFullBackground
{
}

/// made in 0x56B75E, 0x571B93
[@base(typeof(UIControl)), size((ee)0x13C, (aoc)0x13C), vtable((ee)0x0082DE2C, (aoc)0x008436DC)]
unsafe struct UILBString
{
}

/// made in 0x59198C
[@base(typeof(UIRadioButton)), size((ee)0x114, (aoc)0x114), vtable((ee)0x0082FB34, (aoc)0x00845404)]
unsafe struct UIRBMap
{
}

/// made in 0x56B75E, 0x56E14F, 0x571B93, 0x57ED81, 0x58A70E, 0x590734
[@base(typeof(UIButton)), size((ee)0x114, (aoc)0x114), vtable((ee)0x0082DD74, (aoc)0x00843624)]
unsafe struct UIRadioButton
{
}

/// made in 0x59DE62
[@base(typeof(UIControl)), size((ee)0xB0, (aoc)0xB0), vtable((ee)0x00830608, (aoc)0x00845F0C)]
unsafe struct UIScrollBar
{
}

/// made in 0x5672AC, 0x569046, 0x56E14F, 0x571B93, 0x590734
[@base(typeof(UIControl)), size((ee)0xF8, (aoc)0xF8), vtable((ee)0x0082DABC, (aoc)0x0084336C)]
unsafe struct UISlider
{
}

/// made in 0x5D23E7
[@base(typeof(UIControl)), size((ee)0x114, (aoc)0x114), vtable((ee)0x00831038, (aoc)0x00846934)]
unsafe struct UIStatusBar
{
}

/// made in 0x560EC7, 0x563EB8, 0x56575B, 0x5672AC, 0x5685A3, 0x569046, 0x56B75E, 0x56D39A, 0x56E14F, 0x56EA99, 0x571B93, 0x572A2A, 0x57ED81, 0x583705, 0x58A70E, 0x58D3E1, 0x5900BB, 0x590734, 0x59198C, 0x594C3F, 0x59B710, 0x64AAB7, 0x6526E7
[@base(typeof(UIControl)), size((ee)0x124, (aoc)0x124), vtable((ee)0x0082D484, (aoc)0x00842D34)]
unsafe struct UITextLabel
{
}

/// made in 0x57ED81
[@base(typeof(UIButton)), size((ee)0x110, (aoc)0x110), vtable((ee)0x0082EB38, (aoc)0x008443F8)]
unsafe struct UITriStateCheckBox
{
}
