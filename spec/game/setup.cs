/// the game setup
unsafe struct ee_setup
{
    /// thiscall(setup, w, h): the new map's two sides from map_scale - 42, 58, 75, 105, 160, 220 (Tiny .. Gigantic;
    /// the map a diamond in a square of w + h tiles a side), 0 for any other scale. Its three calls: the game start of
    /// a random map (before the map is made), the editor's random map (EESEACreateRandomMap.run) and random_map_create
    [address((ee)0x00524E1F, (aoc)0x0052FB70)]
    [call_site("map_dims", (ee)0x005266BF, (aoc)0x00531432)]
    [call_site("map_dims", (ee)0x00596505, (aoc)0x005A67C1)]
    [call_site("map_dims", (ee)0x0066D41A, (aoc)0x00686572)]
    extern void map_dims(int32* w, int32* h);

    /// the campaign: game_start 0x525BBA builds "Campaigns\" + it; the setup copy 0x524B8F copies it
    [offset(0x380), found(c)]
    wstring campaign;

    /// copied with campaign (0x524B8F, game_start's campaign branch 0x525F6D); the second of four strings 0x14 apart
    /// (map_name +0x3A8)
    [offset(0x394), found(c), guess]
    wstring campaign_scenario;

    /// the setup copy 0x524B8F and game_start
    [offset(0x3A8), found(c)]
    ustring map_name;

    /// the setup copy 0x524B8F and game_start; the registry's "Game Speed" (Game Options, the options loader
    /// 0x52A19A at +4)
    [offset(0x3D4), found(c)]
    int32 speed;

    /// 0 random map, 1 scenario, 2 saved game, 3 editor test, 4 campaign
    /// the setup copy 0x524B8F and game_start
    [offset(0x3D8), found(c)]
    int32 mode;

    /// 1 Small ..
    /// the setup copy 0x524B8F and game_start; the registry's "Map Size" (Game Options, the options loader 0x52A19A
    /// at +4)
    [offset(0x3DC), found(c)]
    int32 map_scale;

    /// the setup copy 0x524B8F and game_start
    [offset(0x3E0), found(c)]
    int32 start_epoch;

    /// the setup copy 0x524B8F and game_start
    [offset(0x3E4), found(c)]
    int32 end_epoch;

    /// the game variant (0x8CF52C's list)
    /// the setup copy 0x524B8F and game_start; the registry's "Game Variant" (Game Options, the options loader
    /// 0x52A19A at +4)
    [offset(0x3E8), found(c)]
    int32 variant;

    /// the setup copy 0x524B8F and game_start; the registry's "Starting Resources" (Game Options, the options loader
    /// 0x52A19A at +4)
    [offset(0x3EC), found(c)]
    int32 resources;

    /// the setup copy 0x524B8F and game_start
    [offset(0x3F0), found(c)]
    int32 unit_limit;

    /// the setup copy 0x524B8F and game_start; the registry's "Wonders For Victory" (Game Options, the options
    /// loader 0x52A19A at +4)
    [offset(0x3F4), found(c)]
    int32 wonders;

    /// the setup copy 0x524B8F and game_start
    [offset(0x3F8), found(c)]
    int32 difficulty;

    /// the setup copy 0x524B8F and game_start
    [offset(0x3FC), found(c)]
    int32 _0x3fc;

    /// the setup copy 0x524B8F and game_start
    [offset(0x400), found(c)]
    block flags;
}

/// the setup the menus edit
static unsafe class setup
{
    /// the setup the menus and the multiplayer lobby edit (16 slots, then the settings: ee_setup's layout) - each PC
    /// of a multiplayer game holds one, kept alike by the lobby's messages; the Start button checks it
    /// (setup_functions.lobby_start) and every PC's game starts from its own when the start message comes
    [address((ee)0x0091881C, (aoc)0x0093125C)]
    static ee_setup lobby;

    /// the setup a lobby starts from: the host / join action (EEAJoinOrHostAGame) copies it into lobby (0x524B8F) before
    /// it connects
    [address((ee)0x00918EAC, (aoc)0x0093030C)]
    static ee_setup lobby_source;

    /// the map sizes the game knows (map_scale 0 .. 5); their names the language file's strings from map_scale_text on
    [value(6)]
    static uint32 map_scales;

    [value(0x7404)]
    static uint32 map_scale_text;

    /// the imm8 of the setup copy's (0x524B8F) cmp eax, 6 / jae: a map_scale of map_scales or more becomes 1 (Small)
    [address((ee)0x00524CBD, (aoc)0x0052FA0E)]
    static code site_scale_limit;

    /// the imm8 of push 6 in the scenario editor's map panel (EEUIFSEMap 0x59198C): its list of map sizes made that long
    [address((ee)0x00592BAB, (aoc)0x005A2E05)]
    static code site_editor_scales;

    /// the imm8 of cmp [ebp + 8], 6 / jb ending the loop after it, each a size's name (map_scale_text + i)
    [address((ee)0x00592BEA, (aoc)0x005A2E44)]
    static code site_editor_scales_end;

    /// the imm32 of cmp [ebp + 8], 0x7409 / jbe ending the setup screen's loop of the size list's names (0x5CE77D:
    /// map_scale_text up to the last size's)
    [address((ee)0x005CE87B, (aoc)0x005DF58A)]
    static code site_setup_scales_end;
}

[c_prefix("")]
static unsafe class setup_functions
{
    /// EEUIELEStart::vf1, the game setup screen's Start button - stdcall(UIForm* form, p2; ecx unused): checks the
    /// setup (two players at least, each human once - else a message box), closes the game to new players, sends the
    /// start message (kind 0x5D: rand() after srand(TMGetTime()), the humans' count) to every PC and saves the
    /// settings; form: the screen (game.iat_form_by_id 0x1A), its element 10 switched off
    [address((ee)0x005C9B91, (aoc)0x005DA6B3), stdcall]
    static extern int32 lobby_start(void* form, int32 p2);

    /// the game setup screen (form 26) brought up to date with setup.lobby: its lists and seats from it, and on the PC
    /// that serves the game (a single-player one too: the server's vf4) its lists' and buttons' events attached
    /// (0x5CB0A4 - the map type list's EEUIELEGameMapType among them; until then the screen has none). Its calls: the
    /// screen's own setting up (0x5CE2AA) and the lobby's messages (0x661727) - hooked after the game's own: the Age
    /// of AI map's options button (engine/mapgen_button.c)
    [address((ee)0x005CACB8, (aoc)0x005DB6EC), cdecl]
    [call_site("setup_screen_refresh", (ee)0x005CE2AA, (aoc)0x005DEFB9)]
    [call_site("setup_screen_refresh", (ee)0x00661727, (aoc)0x00678D71)]
    static extern void setup_screen_refresh();
}

/// the game setup's player slots, 0x38 each
unsafe struct ee_setup_slot
{
    /// the setup code
    [offset(0x0), found(c)]
    int32 type;

    /// engine-notes.md "Game start"; port_check: 162 reads in the paired setup code, AoC the same
    [offset(0x4), found(c)]
    int32 _0x4;

    /// the player's name; port_check: 159 reads in the paired setup code, AoC the same
    [offset(0x8), found(c)]
    ustring name;

    /// engine-notes.md "Game start"; port_check: 22 reads in the paired setup code, AoC the same
    [offset(0x1C), found(c)]
    int32 _0x1c;

    /// engine-notes.md "Game start"; port_check: 19 reads in the paired setup code, AoC the same
    [offset(0x20), found(c)]
    int32 index;

    /// the setup code
    [offset(0x24), found(c)]
    int32 color;

    /// engine-notes.md "Game start"; port_check: 15 reads in the paired setup code, AoC the same
    [offset(0x28), found(c)]
    int32 team;

    /// engine-notes.md "Game start"; port_check: 10 reads in the paired setup code, AoC the same
    [offset(0x2C), found(c)]
    int32 citizens;

    /// engine-notes.md "Game start"; port_check: 14 reads in the paired setup code, AoC the same
    [offset(0x30), found(c)]
    int32 _0x30;

    /// engine-notes.md "Game start"; port_check: 32 reads in the paired setup code, AoC the same
    [offset(0x34), found(c)]
    uint32 flags;

    /// a slot's size, the next slot
    /// port_check: 40 reads in the paired setup code, AoC the same
    [offset(0x38), found(c)]
    block size;
}
