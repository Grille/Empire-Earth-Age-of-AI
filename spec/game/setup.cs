/// the game setup
unsafe struct ee_setup
{
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
}

/// the game setup's player slots, 0x38 each
unsafe struct ee_setup_slot
{
    /// the setup code
    [offset(0x0), found(c)]
    int32 type;

    /// as the slot's others (engine-notes.md "Game start")
    [offset(0x4), found(s)]
    int32 _0x4;

    /// the player's name, as the slot's others
    [offset(0x8), found(s)]
    ustring name;

    /// as the slot's others (engine-notes.md "Game start")
    [offset(0x1C), found(s)]
    int32 _0x1c;

    /// as the slot's others (engine-notes.md "Game start")
    [offset(0x20), found(s)]
    int32 index;

    /// the setup code
    [offset(0x24), found(c)]
    int32 color;

    /// as the slot's others (engine-notes.md "Game start")
    [offset(0x28), found(s)]
    int32 team;

    /// as the slot's others (engine-notes.md "Game start")
    [offset(0x2C), found(s)]
    int32 citizens;

    /// as the slot's others (engine-notes.md "Game start")
    [offset(0x30), found(s)]
    int32 _0x30;

    /// as the slot's others (engine-notes.md "Game start")
    [offset(0x34), found(s)]
    uint32 flags;

    /// a slot's size, the next slot
    [offset(0x38), found(s)]
    block size;
}
