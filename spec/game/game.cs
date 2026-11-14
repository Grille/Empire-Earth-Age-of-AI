/// game's data of the game, places in its code and numbers
static unsafe class game
{
    /// a player's PC changed (its msg_id; cdecl(world player id, msg_id, player index, ?)): when it is this PC, this PC's
    /// player set, the minimap's player and rebuild, its paint_done, the view table refilled (view_sight_copy), the tech
    /// tree's ui_refresh(1), the window told (0x661B3F) - the steps godmode's switch takes (engine/godmode.c)
    [address((ee)0x0053153E, (aoc)0x0053C5AD)]
    static code local_player_switch;

    [address((ee)0x004F80A4, (aoc)0x0050232D)]
    static code call_damage_points;

    /// the unit behaviours (stances)
    [address((ee)0x008D2044, (aoc)0x008E8744)]
    static block behaviour_table;

    /// EECameraControl; +0x78/+0x7C/+0x80 look-at x/y/z (tiles), +0xE0 zoom
    [address((ee)0x00918CD0, (aoc)0x00931710)]
    static EECameraControl camera;

    /// byte: the camera's step (0x51EE75, AoC 0x529A34) makes a fixed view instead of the camera's transformation from
    /// its blend (45 degrees, 40 up) while it is set - not seen set in play
    [address((ee)0x00919461, (aoc)0x009308E2), guess]
    static block camera_fixed;

    /// float in .rdata: the eye's distance from the look-at at zoom 1 in the normal view, -20.5 (read in the camera's move
    /// 0x51DDD7, AoC 0x528996: the distance = (it - 0.1) x zoom + 0.1; view 3 and a cinematic take -42 instead)
    [address((ee)0x0082C700, (aoc)0x00841EA0)]
    static block camera_eye_far;

    /// float in .rdata: the far depth of the normal view, 35 (0x51CE31, AoC 0x527BC7, every camera step; view 3 and a
    /// cinematic take 58 instead)
    [address((ee)0x0082C704, (aoc)0x00841EA4)]
    static block camera_far;

    /// float: the field of view in radians, given to the map's viewport at every camera step (0x51EE75 calls
    /// GEViewport::SetFieldOfView with it, AoC 0x529A34), 0.5236
    [address((ee)0x0091328C, (aoc)0x00929A34)]
    static block camera_fov;

    /// float: the camera's pitch in radians at zoom 1, -0.6109 (the second of the two the camera's orientation is made from)
    [address((ee)0x009183C8, (aoc)0x00930E08)]
    static block camera_pitch;

    /// the address operand of a fld in 0x515611 (AoC's the same place): a fixed -30 degrees (camera_tilt_was); dreXmod's
    /// camera points it at camera_pitch
    [address((ee)0x0051568B, (aoc)0x0051FCDC)]
    static code site_camera_tilt;

    [value((ee)0x0082C5C4, (aoc)0x00841D60)]
    static uint32 camera_tilt_was;

    [address((ee)0x00918E9A, (aoc)0x009318DA)]
    static block cheats_on;

    /// byte: a cinematic runs - set at its start (0x5A1CDA, which keeps scroll_speed), cleared at its end (0x5A1C33)
    [address((ee)0x00918EA0, (aoc)0x009318E0), guess]
    static block cinematic;

    [address((ee)0x00919858, (aoc)0x009319D4)]
    static EECheatCodes* cheat_parser;

    /// the player colour table (vector begin, end)
    [address((ee)0x008CFA74, (aoc)0x008E610C)]
    static block colors_begin;

    [address((ee)0x008CFA78, (aoc)0x008E6110)]
    static block colors_end;

    /// [attacker class + target class x 6]
    [address((ee)0x008D2050, (aoc)0x008E8750)]
    static block damage_table;

    /// the AIs' difficulty (the setup's)
    [address((ee)0x00918C14, (aoc)0x00931654)]
    static block difficulty;

    /// draw list counts: objects, small objects
    [address((ee)0x00912538, (aoc)0x00928CD8)]
    static block draw_n;

    [address((ee)0x0091253C, (aoc)0x00928CDC)]
    static block draw_small_n;

    /// the scenario editor's file buttons: 6 of {id, x, y, u0, v0, u1, v1, tooltip}
    [address((ee)0x008AE0A0, (aoc)0x008C47B0)]
    static block editor_file_buttons;

    /// tech effects, 0x44 each
    [address((ee)0x00919E70, (aoc)0x00931FF8)]
    static block effects;

    /// the game's end epoch
    [address((ee)0x009187F4, (aoc)0x00931234)]
    static block end_epoch;

    /// per epoch 0x10: the tech lists
    [address((ee)0x0093B1B8, (aoc)0x0095CF80)]
    static block epoch_techs;

    /// a byte: the camera free of the fog - the scenario editor's (the AoC decompilation: gFreeCamera): the units'
    /// collection draws unexplored tiles' objects and no doppelgangers with it, and the editor's selection
    [address((ee)0x0091986C, (aoc)0x009319E8), guess]
    static block free_camera;

    /// the address operands of its reads in the camera's code (0x51CC45, 0x51CE31 twice, 0x51DDD7 three times, 0x51EC85):
    /// the editor's own camera - the near plane, the far depth, the eye's distance, the look-at (engine/editor_camera.c
    /// points them at a byte that stays 0)
    [address((ee)0x0051CE04, (aoc)0x005279C3)]
    [address((ee)0x0051CFCD, (aoc)0x00527B8C)]
    [address((ee)0x0051CFF8, (aoc)0x00527BB7)]
    [address((ee)0x0051DEBB, (aoc)0x00528A7A)]
    [address((ee)0x0051DFA7, (aoc)0x00528B66)]
    [address((ee)0x0051E150, (aoc)0x00528D0F)]
    [address((ee)0x0051ED71, (aoc)0x00529930)]
    static code free_camera_reads;

    /// the push of 2 in the editor's start (EEUIFScenarioEditor vf18 0x58F363), the view it gives the camera
    [address((ee)0x0058F383, (aoc)0x0059F7B4)]
    static code site_editor_view;

    [address((ee)0x0091945A, (aoc)0x009308DB)]
    static block fullscreen;

    /// game data loaded (unit limit +0x2EC, chat time +0x2CC)
    [address((ee)0x00917BB8, (aoc)0x0092FCA8)]
    static block game_data;

    /// the data files' search path (FSPath*, Low-Level Engine.dll's): every data file the game opens is found through
    /// it (the db tables 0x541734, models, textures, sounds). Set up once at the start (AoC 0x53FE97, the 2017 update
    /// 0x535121's function): AoC's has two roots - its own folder (the startup directory) and the 2017 update's install
    /// folder (the registry's) - a directory Data\ and an archive Data\data.ssa each, FindFile looking in every
    /// directory before any archive
    [address((ee)0x0091978C, (aoc)0x00931908)]
    static block file_path;

    [address((ee)0x00918808, (aoc)0x00931248)]
    static block game_mode;

    [address((ee)0x009193F8, (aoc)0x00930878)]
    static block game_res_h;

    [address((ee)0x009193FC, (aoc)0x0093087C)]
    static block game_res_w;

    /// float; set like EEMGameSpeed::Apply (0x4B6BD4)
    [address((ee)0x009183C0, (aoc)0x00930E00)]
    static block game_speed;

    /// the world (EEWorld, a static object); its vtable slot 11 called after a speed change
    [address((ee)0x00918300, (aoc)0x00930D40)]
    static EEWorld game_state;

    /// 0 random map, 1 scenario, 2 saved game, 3/4 other
    [address((ee)0x00918BF4, (aoc)0x00931634)]
    static block game_type;

    /// GEWindow* (+0 HWND)
    [address((ee)0x009197A4, (aoc)0x00931920)]
    static block game_window;

    /// [target family][weapon kind]: 0 = it cannot hit
    [address((ee)0x008D200C, (aoc)0x008E8708)]
    static block hit_table;

    /// the hit table's variants by game mode
    [address((ee)0x008D2008, (aoc)0x008E870C)]
    static block hit_table_a;

    [address((ee)0x008D2010, (aoc)0x008E8710)]
    static block hit_table_b;

    /// the user-input messages processed so far
    [address((ee)0x00919B68, (aoc)0x00931CF0)]
    static block input_msgs_done;

    /// int: index of the player on this PC (the human in single player)
    [address((ee)0x00918E84, (aoc)0x009318C4)]
    static block local_player;

    /// ptr; +0x1816FC = map size in tiles
    [address((ee)0x009183BC, (aoc)0x00930DFC)]
    static block map;

    /// GERectangle (bottom, left, right, top): the minimap's points in its 800 x 600 pixels - a click's point is kept in
    /// it (0x65EAFA, AoC 0x6760EC)
    [address((ee)0x009432D0, (aoc)0x00965318)]
    static block minimap_bounds;

    /// EEMiniMap*: the minimap of this PC's player (rebuilt when its fog changes: the scenario's Visibility-On / Off)
    [address((ee)0x009428A8, (aoc)0x00964888)]
    static block minimap;

    /// RTSMouse
    [address((ee)0x009197A8, (aoc)0x00931924)]
    static block mouse;

    /// byte, toggled by EEAUserPauseGame::Execute (0x4064F1)
    [address((ee)0x00918EA2, (aoc)0x009318E2)]
    static block paused;

    /// int: ceil([0x918DEC] / [0x8ABC80]), at least 1 (0x547AAD) - the push moves a unit by push x step x it
    /// (movement.slot_pm_push)
    [address((ee)0x008ABC84, (aoc)0x008C237C)]
    static block phys_substeps;

    [address((ee)0x00918374, (aoc)0x00930DB4)]
    static block players;

    /// the number of players
    [address((ee)0x009183CC, (aoc)0x00930E0C)]
    static block players_n;

    /// the player name (UWideString)
    [address((ee)0x00919308, (aoc)0x00930768)]
    static block player_name;

    /// the population limit game setting
    [address((ee)0x009183D4, (aoc)0x00930E14)]
    static block pop_limit;

    [address((ee)0x00919794, (aoc)0x00931910)]
    static block rasterizer;

    /// the string resources
    [address((ee)0x00919770, (aoc)0x009318EC)]
    static block resources;

    /// byte: game is a scenario/campaign (settings 0x918410 + 0x409)
    [address((ee)0x00918819, (aoc)0x00931259)]
    static block scenario_flag;

    /// the current screen index
    [address((ee)0x00919428, (aoc)0x009308A8)]
    static block screen;

    /// byte: set 1 once the display is set up (0x5C76A2, beside game_res_w / game_res_h), never cleared; the
    /// wheel's zoom and the keys' do nothing without it
    [address((ee)0x0091944D, (aoc)0x009308CD), guess]
    static block screen_ready;

    /// float, settings slider x 0.01
    [address((ee)0x009193A8, (aoc)0x00930828)]
    static block scroll_setting;

    /// float
    [address((ee)0x00918DB0, (aoc)0x009317F0)]
    static block scroll_speed;

    /// float: the middle button's drag speed - EEUIEGrabScroll moves the camera by the mouse's counts x (it + 0.5) x
    /// (scroll_speed + 0.5) x 0.07 (both builds), beside scroll_setting (+4) as the settings' next slider
    [address((ee)0x009193AC, (aoc)0x0093082C), guess]
    static block grab_setting;

    /// a default selection rectangle
    [address((ee)0x008D2778, (aoc)0x008E8F18)]
    static block select_rect;

    /// EEClient/EEServer (lockstep server, slot 6 takes a message); UI checks [+0x10] != 0 and vtable+0x10 before
    /// pausing
    [address((ee)0x0091984C, (aoc)0x009319C8)]
    static block server;

    /// the game settings object
    [address((ee)0x00918EA8, (aoc)0x00930308)]
    static block settings;

    /// the settings' Rasterizer Name (a UString at settings +0x49C: settings_save_game writes its text +0x4A0 to the
    /// registry): the display's start (0x5392E9, AoC 0x5444D0) walks GERasterizer's list, game.rasterizer each one in
    /// turn, and takes the first whose name (+0x54) is equal to it by UString's operator== - else the first of the list
    [address((ee)0x00919344, (aoc)0x009307A4)]
    static block settings_rasterizer_name;

    /// the scenario manager
    [address((ee)0x00918E24, (aoc)0x00931864)]
    static block sm_mgr;

    [address((ee)0x00919848, (aoc)0x009319C4)]
    static block sound_manager;

    [address((ee)0x009193B8, (aoc)0x00930838)]
    static block sound_percent;

    /// table_value_game's table (0x16: the population limit)
    [address((ee)0x00917BB0, (aoc)0x0092FCA0)]
    static EEWorldTable table_values;

    [address((ee)0x00918E9F, (aoc)0x009318DF)]
    static block teams_locked;

    [address((ee)0x009418A0, (aoc)0x009636AC)]
    static block types;

    [address((ee)0x009197A0, (aoc)0x0093191C)]
    static block ui_manager;

    /// UIManager::GetFormByID's import slot - thiscall([game.ui_manager], uint32 id): the screen of that id (0x1A the
    /// game setup screen, the lobby; 0xC the main menu)
    [import_slot("Low-Level Engine.dll", "?GetFormByID@UIManager@@QBEPAVUIForm@@K@Z")]
    [address((ee)0x00823B4C, (aoc)0x00837AE0)]
    static block iat_form_by_id;

    /// the user interface: the action queue's owner, the chat lines'
    [address((ee)0x0091977C, (aoc)0x009318F8)]
    static EEUserInterface* ui;

    /// cos (the game's)
    [import_slot("Low-Level Engine.dll", "?UMCos@@YAMM@Z")]
    [address((ee)0x00823304, (aoc)0x00837340)]
    static block iat_cos;

    /// the game's rand
    [import_slot("msvcrt.dll", "rand")]
    [address((ee)0x00824648, (aoc)0x0083863C)]
    static block iat_rand;

    /// imported refcount release, thiscall on the action
    [import_slot("Low-Level Engine.dll", "?Dereference@UReferenceCounter@@QAEXXZ")]
    [address((ee)0x008231AC, (aoc)0x008371A8)]
    static block iat_release;

    /// the game's string resource (a language entry)
    [import_slot("Low-Level Engine.dll", "?GetString@UResourceFile@@QAEJJAAVUWideString@@@Z")]
    [address((ee)0x0082319C, (aoc)0x00837198)]
    static block iat_resource_string;

    [import_slot("Low-Level Engine.dll", "?Receive@TSThread@@QAEJAAKAAPAX1@Z")]
    [address((ee)0x008236B0, (aoc)0x0083765C)]
    static block iat_ts_receive;

    [import_slot("Low-Level Engine.dll", "?Wait@TSEvent@@QAEJK@Z")]
    [address((ee)0x00823688, (aoc)0x00837634)]
    static block iat_ts_wait;

    /// UString::operator=(const char *)
    [import_slot("Low-Level Engine.dll", "??4UString@@QAEAAV0@PBD@Z")]
    [address((ee)0x00823228, (aoc)0x00837224)]
    static block iat_ustring_assign;

    /// UString::UString()
    [import_slot("Low-Level Engine.dll", "??0UString@@QAE@XZ")]
    [address((ee)0x008231E4, (aoc)0x008371E0)]
    static block iat_ustring_ctor;

    /// UString::~UString()
    [import_slot("Low-Level Engine.dll", "??1UString@@UAE@XZ")]
    [address((ee)0x008245BC, (aoc)0x008385B0)]
    static block iat_ustring_dtor;

    /// UWideString::operator=(const char *)
    [import_slot("Low-Level Engine.dll", "??4UWideString@@QAEAAV0@PBD@Z")]
    [address((ee)0x00823224, (aoc)0x00837220)]
    static block iat_wstring_assign;

    /// UWideString::UWideString()
    [import_slot("Low-Level Engine.dll", "??0UWideString@@QAE@XZ")]
    [address((ee)0x008231A4, (aoc)0x008371A0)]
    static block iat_wstring_ctor;

    /// UWideString::operator=(const UWideString &) (the slots' names: they share their text)
    [import_slot("Low-Level Engine.dll", "??4UWideString@@QAEAAV0@ABV0@@Z")]
    [address((ee)0x008231D0, (aoc)0x008371CC)]
    static block iat_wstring_copy;

    /// UWideString::UWideString(const char *)
    [import_slot("Low-Level Engine.dll", "??0UWideString@@QAE@PBD@Z")]
    [address((ee)0x008232C4, (aoc)0x008372C0)]
    static block iat_wstring_ctor_c;

    /// UWideString::~UWideString()
    [import_slot("Low-Level Engine.dll", "??1UWideString@@UAE@XZ")]
    [address((ee)0x00823AEC, (aoc)0x00837194)]
    static block iat_wstring_dtor;

    /// UWideString::Format(const char *, ...)
    [import_slot("Low-Level Engine.dll", "?sprintf@@YAJAAVUWideString@@PBDZZ")]
    [address((ee)0x008232C8, (aoc)0x008372C4)]
    static block iat_wstring_format;

    [import_slot("Low-Level Engine.dll", "??0UWideString@@QAE@K@Z")]
    [address((ee)0x008235D0, (aoc)0x00837574)]
    static block iat_wstring_reserve;

    /// import: &ISKeyboard* in Low-Level Engine.dll
    [import_slot("Low-Level Engine.dll", "?gISKeyboard@@3PAVISKeyboard@@A")]
    [address((ee)0x00823AF8, (aoc)0x00837AE8)]
    static uint8** imp_is_keyboard;

    /// import: &ISMouse* in Low-Level Engine.dll
    [import_slot("Low-Level Engine.dll", "?gISMouse@@3PAVISMouse@@A")]
    [address((ee)0x008235CC, (aoc)0x00837570)]
    static ISMouse** imp_is_mouse;

    /// new_unit_msg_init
    [value(32)]
    static uint32 size_new_unit_msg;

    /// EEAUserPauseGame: vtable, refcount, byte
    [value(12)]
    static uint32 size_pause_action;

    /// save_action_init
    [value(48)]
    static uint32 size_save_action;

    /// join_action_init
    [value(0x38)]
    static uint32 size_join_action;

    /// EEPhysics vtable slot 8: one simulation step
    [address((ee)0x0082CB7C, (aoc)0x00842324)]
    static block vtbl_phys_step;

    /// EEAUserPauseGame vtable (0xC bytes: vtable, refcount, byte)
    [address((ee)0x00830E94, (aoc)0x00846790)]
    static vftable vt_pause_action;
}

/// the game's functions game.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class game_functions
{
    [address((ee)0x005D0B4F, (aoc)0x005E145B), cdecl]
    static extern void exit_confirm();

    /// the highest epoch of the players player.slot_player_human says are human, 0 without
    [address((ee)0x0046D436, (aoc)0x0047005A), cdecl]
    static extern int32 highest_human_epoch();

    [address((ee)0x00521F94, (aoc)0x0052CB90), cdecl]
    // call LoadSavedGameFromCmdline(UString *) 0x521F94
    [call_site("load_saved", (ee)0x00522BF8, (aoc)0x0052D8F9)]
    static extern int32 load_saved(void* name);

    /// the game's operator new
    [address((ee)0x00683718, (aoc)0x0069D178), cdecl]
    static extern void* game_new(nuint n);

    [address((ee)0x00522A36, (aoc)0x0052D703), cdecl]
    // call ParseCommandLine(const char *) 0x522A36
    [call_site("parse_cmdline", (ee)0x00535EF2, (aoc)0x005410C8)]
    static extern char8 parse_cmdline(cstr cmd);

    [address((ee)0x005D05CC, (aoc)0x005E0E7F), cdecl]
    static extern void resign_send();

    [address((ee)0x00540ACA, (aoc)0x0054D066), cdecl]
    static extern int32 save_game(void* file, int32 a, int32 b, ustring folder);

    /// a scenario's or a save's contents written: the header (EEPSaveGameHeader, its table of lump types) into header,
    /// then the lumps, an FSChunk each, into file - the same file at both calls; 0 = written. Its two calls: the save
    /// to disk (0x540DA3, under save_game; the editor's Save too) and the editor's quicksave into memory (EEAQuickSaveLoad
    /// 0x5952D3: the scenario its test play starts from)
    [address((ee)0x0054086A, (aoc)0x0054CE06), cdecl]
    [call_site("save_write", (ee)0x00540E9D, (aoc)0x0054D445)]
    [call_site("save_write", (ee)0x00595491, (aoc)0x005A571A)]
    static extern int32 save_write(void* objects, uint32 time, FSFile* header, FSFile* file);

    /// a game loaded from a file - a scenario, a saved game, the editor's test play (EEGameState::start; 0x540F55, the
    /// quickload's): thiscall(?, the file) -> 1 loaded. It reads the whole file into an FSMemoryFile (iat_memfile_load),
    /// then the header and the lumps its table lists, and nothing after them (2026-10-22: a lump appended unlisted is
    /// never read, the game plays as from the file without it)
    [address((ee)0x0053DEE3, (aoc)0x0054A3EB), thiscall]
    [call_site("load_file", (ee)0x00525DA6, (aoc)0x00530B19)]
    [call_site("load_file", (ee)0x00540F69, (aoc)0x0054D511)]
    static extern uint8 load_file(void* self, FSFile* file);

    /// thiscall(memory file, FSFile *): the whole file into the memory file (its position then at the start) - the load's
    /// (load_file), the header's check before it (0x54139E), a single lump's read (0x53E117)
    [import_slot("Low-Level Engine.dll", "?Load@FSMemoryFile@@QAEJAAVFSFile@@@Z")]
    [address((ee)0x00823B6C, (aoc)0x00837B54)]
    static block iat_memfile_load;

    /// every setting into the registry: game.settings, a URegistry (an engine DLL's class, RTTI in a running game -
    /// the function the exe's own, not one of the class's)
    [address((ee)0x0052AA81, (aoc)0x00535943), thiscall]
    static extern void settings_save_game(void* p1);

    /// the game's error box, an EEUIFError (0x1BC bytes) made and shown - thiscall(the caller's ecx, a UWideString*:
    /// its text, a, b, on_ok: called on its button, c); 87 calls. A headless test logs each: it waits for a click
    [address((ee)0x0065C6A9, (aoc)0x00674DB5), thiscall]
    static extern void error_box(void* parent, void* text, int32 a, int32 b, void* on_ok, int32 c);

    /// a vector's dtor (the game's delete)
    [address((ee)0x0061ABEC, (aoc)0x006002C7), thiscall]
    static extern void vector_free(void* v);

}

/// Low-Level Engine.dll's file - FSFileSpec on disk, FSMemoryFile in memory ... -: its virtual functions, in the same
/// slots in both builds' engines (scripts/re/dll_vtable.py on FSFile and FSMemoryFile); each returns 0 when done
[c_type("ee_fs_file")]
unsafe struct FSFile
{
    /// Read(unsigned char *, K, K &): n bytes into buf, the count read into *got
    [slot(25)]
    extern int32 read(uint8* buf, uint32 n, uint32* got);

    /// Write(unsigned char const *, K, K &)
    [slot(39)]
    extern int32 write(uint8* buf, uint32 n, uint32* put);

    /// GetLength(K &): its bytes
    [slot(45)]
    extern int32 get_length(uint32* n);

    /// GetPosition(J &)
    [slot(48)]
    extern int32 get_position(int32* at);

    /// SetPosition(J, K, bool): origin 0 offset bytes from the start, 1 back from the end, 2 on from here
    /// (FSMemoryFile's 0x100237CD)
    [slot(52)]
    extern int32 set_position(int32 offset, uint32 origin, int8 p3);
}
