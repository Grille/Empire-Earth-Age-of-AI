/// the game setup
unsafe struct ee_setup
{
    /// the setup copy 0x524B8F and game_start
    [offset(0x3A8), found(c)]
    block map_name;

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

/// the game setup's player slots, 0x38 each
[c_prefix("slot")]
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
    block name;

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
