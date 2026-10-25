/// the scenario manager (EEScenarioManager; test/scen.c)
[c_type("ee_scen_mgr")]
unsafe struct EEScenarioManager
{
    /// a 4x4 matrix: the cinematic's start 0x5A1CDA saves the camera's rotation (quaternion camera +0x4C, 0x51D4CA)
    /// here, its end 0x5A18D0 restores it (0x51EA3E)
    [offset(0x8), found(c), guess]
    block cam_rot;

    /// the same for the camera's quaternion +0x5C (0x51D528 saves, 0x51EAF7 restores)
    [offset(0x48), found(c), guess]
    block cam_rot_2;

    /// the camera's eye saved at a cinematic's start (0x51D46C), restored at its end (0x51E988)
    [offset(0x88), found(c), guess]
    vec3 cam_eye;

    /// the camera's look-at point saved at a cinematic's start (0x51D56A), restored at its end (0x51EC85)
    [offset(0x94), found(c), guess]
    vec3 cam_look;

    /// its file's code (the scenario manager)
    [offset(0xC0), found(c)]
    block chat;

    /// {player index, type id} pairs of 8 bytes: vf7 0x5A26AC unloads each type's media for its player ("Scenario:
    /// Unloading Media for")
    [offset(0xD8), found(c), guess]
    vector media_loaded;

    /// its file's code (the scenario manager)
    [offset(0xE8), found(c)]
    vector triggers;

    /// its file's code (the scenario manager)
    [offset(0xF8), found(c)]
    vector conds;

    /// its file's code (the scenario manager)
    [offset(0x108), found(c)]
    vector effects;

    /// its file's code (the scenario manager)
    [offset(0x118), found(c)]
    vector objects;

    /// its file's code (the scenario manager)
    [offset(0x128), found(c)]
    vector areas;

    /// 64-byte records (a unit id +0x38, an int +0x2C, a cinematic byte +0x3C): 0x5A45CA takes one per call and points
    /// the camera at the unit (0x51DDD7); a cinematic's start drops those of +0x3C 0, its end those of 1
    [offset(0x148), found(c), guess]
    vector camera_queue;

    /// its file's code (the scenario manager)
    [offset(0x168), found(c)]
    vector queue;

    /// the scenario's hints text: the effect's run 0x5AE130 / 0x5AE14B clears it or appends the effect's text
    /// (text_kind 0)
    [offset(0x188), found(c)]
    wstring hints;

    /// the history text, the same for text_kind 2 (0x5AE06B / 0x5AE089)
    [offset(0x19C), found(c)]
    wstring history;

    /// the instructions text, the same for text_kind 1 (0x5AE0CF / 0x5AE0ED)
    [offset(0x1D8), found(c)]
    wstring instructions;

    /// game.scroll_speed at a cinematic's start (0x5A1CDA), given back at its end through 0x51F2F3 (0x5A18D0)
    [offset(0x204), found(c)]
    float32 saved_scroll_speed;

    /// ms
    /// its file's code (the scenario manager)
    [offset(0x210), found(c)]
    float32 time;

    /// an area by its id
    [address((ee)0x0056DFB7, (aoc)0x0057DA08)]
    extern EESMArea* area(int32 id);

    /// a condition by its id (index), NULL out of range
    [address((ee)0x0056D186, (aoc)0x0057CADF)]
    extern ee_scen_cond* cond(int32 id);

    /// an effect by its id
    [address((ee)0x00573183, (aoc)0x00582D82)]
    extern ee_scen_effect* effect(int32 id);

    /// an object by its id
    [address((ee)0x00579460, (aoc)0x00588EDD)]
    extern EESMObject* @object(int32 id);

    /// a trigger by its id
    [address((ee)0x0057ED54, (aoc)0x0058EF8A)]
    extern EESMTrigger* trigger(int32 id);

    /// thiscall(mgr, player spec, vector<int> *out or NULL, pick one): the spec's players (docs/triggers.md)
    [address((ee)0x005A23F6, (aoc)0x005B260A)]
    extern int32 players(int32 p2, void* p3, char8 p4);

    /// thiscall(mgr, effect id, time ms, cinematic): run at that scenario time
    [address((ee)0x0059FE52, (aoc)0x005B005B)]
    extern void queue_effect(int32 effect, int32 at, char8 cinematic);
}

/// a scenario condition
unsafe struct ee_scen_cond
{
    /// its file's code (the scenario manager)
    [offset(0x4), found(c)]
    wstring name;

    /// the editor's EESETSCondition::vf11 0x56F0D1 puts name (+0x4) and then this into its two edit boxes, as
    /// EESETSEffect::vf11 its name and description
    [offset(0x18), found(c), guess]
    wstring description;

    /// its file's code (the scenario manager)
    [offset(0x2C), found(c)]
    wstring text;

    /// its file's code (the scenario manager)
    [offset(0x40), found(c)]
    sm_cond_type type;

    /// its file's code (the scenario manager)
    [offset(0x44), found(c)]
    sm_compare op;

    /// game: the difficulty
    /// its file's code (the scenario manager)
    [offset(0x48), found(c)]
    sm_difficulty difficulty;

    /// diplomacy: 0 allied, 1 enemy
    /// its file's code (the scenario manager)
    [offset(0x4C), found(c)]
    sm_diplomacy stance;

    /// its file's code (the scenario manager)
    [offset(0x50), found(c)]
    sm_player_attr attr;

    /// attribute: the other player's
    /// its file's code (the scenario manager)
    [offset(0x54), found(c)]
    sm_player_attr attr2;

    /// its file's code (the scenario manager)
    [offset(0x58), found(c)]
    int32 tech;

    /// its file's code (the scenario manager)
    [offset(0x5C), found(c)]
    int32 value;

    /// its file's code (the scenario manager)
    [offset(0x60), found(c)]
    int32 @object;

    /// its file's code (the scenario manager)
    [offset(0x64), found(c)]
    sm_player_spec player;

    /// diplomacy, attribute: the other spec
    /// its file's code (the scenario manager)
    [offset(0x68), found(c)]
    sm_player_spec player2;

    /// its file's code (the scenario manager)
    [offset(0x6C), found(c)]
    int32 time_from;

    /// its file's code (the scenario manager)
    [offset(0x70), found(c)]
    int32 time_to;

    /// game: at or after that hour
    /// its file's code (the scenario manager)
    [offset(0x74), found(c)]
    int32 hour;

    /// its file's code (the scenario manager)
    [offset(0x78), found(c)]
    int32 trigger;

    /// its file's code (the scenario manager)
    [offset(0x80), found(c)]
    char8 trigger_on;

    /// its file's code (the scenario manager)
    [offset(0x81), found(c)]
    char8 trigger_fired;

    /// its file's code (the scenario manager)
    [offset(0x82), found(c)]
    char8 test_difficulty;

    /// its file's code (the scenario manager)
    [offset(0x83), found(c)]
    char8 timed;

    /// its file's code (the scenario manager)
    [offset(0x84), found(c)]
    char8 test_hour;

    /// attribute against attribute
    /// its file's code (the scenario manager)
    [offset(0x85), found(c)]
    char8 vs_player;

    /// the condition holds now (its own side effects: an object it tests may lock)
    [address((ee)0x005AA80B, (aoc)0x005BAABF)]
    extern char8 eval();

    /// thiscall(cond, player index, attribute): what a condition compares
    [address((ee)0x005AACC4, (aoc)0x005BAF78)]
    extern int32 player_attr(int32 player, int32 attr);
}

/// a scenario area (EESMArea)
[c_type("ee_scen_area")]
unsafe struct EESMArea
{
    /// its file's code (the scenario manager)
    [offset(0x4), found(c)]
    wstring name;

    /// its file's code (the scenario manager)
    [offset(0x2C), found(c)]
    block point;

    /// its file's code (the scenario manager)
    [offset(0x34), found(c)]
    block rect;

    /// its file's code (the scenario manager)
    [offset(0x5C), found(c)]
    sm_area_type type;

    /// kept: the class's other fields all kept, no read of this one in its methods
    [offset(0x64), found(h, "kept: the class's other fields all kept, no read of this one in its methods")]
    int32 ai_area;

    [address((ee)0x005A9E1D, (aoc)0x005BA0D1)]
    extern char8 contains(EEUnit* p2);
}

/// a scenario effect
unsafe struct ee_scen_effect
{
    /// its file's code (the scenario manager)
    [offset(0x8), found(c)]
    vector group;

    /// its name (the editor's list, Save Triggers Only's "Effect '%s'")
    [offset(0x14), found(c)]
    wstring name;

    /// its description
    [offset(0x28), found(c)]
    wstring description;

    /// its file's code (the scenario manager)
    [offset(0x50), found(c)]
    wstring text;

    /// its file's code (the scenario manager)
    [offset(0x8C), found(c)]
    block class_name;

    /// kept: the class's other fields all kept, no read of this one in its methods
    [offset(0xA4), found(h, "kept: the class's other fields all kept, no read of this one in its methods")]
    block sound;

    /// its file's code (the scenario manager)
    [offset(0xB4), found(c)]
    block line;

    /// set attribute
    /// its file's code (the scenario manager)
    [offset(0xC8), found(c)]
    sm_player_attr player_attr;

    /// diplomacy
    /// its file's code (the scenario manager)
    [offset(0xCC), found(c)]
    sm_diplomacy stance;

    /// dialogue, chat
    /// its file's code (the scenario manager)
    [offset(0xD4), found(c)]
    sm_player_spec msg_players;

    /// object owner
    /// its file's code (the scenario manager)
    [offset(0xD8), found(c)]
    sm_player_spec owner_players;

    /// player, AI effects
    /// its file's code (the scenario manager)
    [offset(0xDC), found(c)]
    sm_player_spec players;

    /// diplomacy
    /// its file's code (the scenario manager)
    [offset(0xE0), found(c)]
    sm_player_spec target_players;

    /// its file's code (the scenario manager)
    [offset(0xE4), found(c)]
    sm_ai_action ai_kind;

    /// set AI variable: the record
    /// its file's code (the scenario manager)
    [offset(0xE8), found(c)]
    int32 ai_var;

    /// its file's code (the scenario manager)
    [offset(0xEC), found(c)]
    sm_ai_toggle ai_toggle;

    /// its file's code (the scenario manager)
    [offset(0xF0), found(c)]
    sm_media_action media_kind;

    /// its file's code (the scenario manager)
    [offset(0xF8), found(c)]
    sm_player_action player_kind;

    /// its file's code (the scenario manager)
    [offset(0xFC), found(c)]
    sm_trigger_action trigger_kind;

    /// its file's code (the scenario manager)
    [offset(0x100), found(c)]
    sm_category category;

    /// its file's code (the scenario manager)
    [offset(0x104), found(c)]
    sm_object_action object_kind;

    /// unit / class attribute
    /// its file's code (the scenario manager)
    [offset(0x108), found(c)]
    sm_unit_op op;

    /// its file's code (the scenario manager)
    [offset(0x10C), found(c)]
    int32 tech;

    /// AI variable
    /// its file's code (the scenario manager)
    [offset(0x110), found(c)]
    float32 @float;

    /// zoom, AI variable, land favourite
    /// its file's code (the scenario manager)
    [offset(0x114), found(c)]
    int32 int1;

    /// scroll speed, AI variable, air favourite
    /// its file's code (the scenario manager)
    [offset(0x118), found(c)]
    int32 int2;

    /// AI target / group, camera
    /// its file's code (the scenario manager)
    [offset(0x120), found(c)]
    int32 area;

    /// its object: AI target / group, camera
    /// its file's code (the scenario manager)
    [offset(0x124), found(c)]
    int32 target;

    /// calamity, graphic effect
    /// its file's code (the scenario manager)
    [offset(0x128), found(c)]
    sm_calamity calamity;

    /// its file's code (the scenario manager)
    [offset(0x12C), found(c)]
    int32 cal_area;

    /// its file's code (the scenario manager)
    [offset(0x130), found(c)]
    int32 cal_object;

    /// daylight, cinematic speed
    /// its file's code (the scenario manager)
    [offset(0x134), found(c)]
    int32 hour;

    /// its file's code (the scenario manager)
    [offset(0x138), found(c)]
    int32 day_cycle;

    /// its file's code (the scenario manager)
    [offset(0x13C), found(c)]
    sm_game_action game_kind;

    /// s
    /// its file's code (the scenario manager)
    [offset(0x140), found(c)]
    int32 duration;

    /// 0 hints, 1 instructions, 2 history
    /// its file's code (the scenario manager)
    [offset(0x144), found(c)]
    sm_text_kind text_kind;

    /// < 20 unit, else class
    /// its file's code (the scenario manager)
    [offset(0x148), found(c)]
    sm_unit_attr unit_attr;

    /// attribute value, facing (degrees)
    /// its file's code (the scenario manager)
    [offset(0x14C), found(c)]
    int32 value;

    /// object effects
    /// its file's code (the scenario manager)
    [offset(0x158), found(c)]
    int32 @object;

    /// its file's code (the scenario manager)
    [offset(0x15C), found(c)]
    sm_stance obj_stance;

    /// 0 normal .. 5 garrison, 7+ a power
    /// its file's code (the scenario manager)
    [offset(0x160), found(c)]
    sm_task task;

    /// its file's code (the scenario manager)
    [offset(0x164), found(c)]
    int32 task_area;

    /// its file's code (the scenario manager)
    [offset(0x168), found(c)]
    int32 task_object;

    /// its file's code (the scenario manager)
    [offset(0x16C), found(c)]
    int32 civ_points;

    /// its file's code (the scenario manager)
    [offset(0x170), found(c)]
    sm_attr_mode attr_mode;

    /// its file's code (the scenario manager)
    [offset(0x174), found(c)]
    int32 amount;

    /// camera, flare
    /// its file's code (the scenario manager)
    [offset(0x178), found(c)]
    int32 cam_area;

    /// its file's code (the scenario manager)
    [offset(0x17C), found(c)]
    int32 cam_object;

    /// trigger effects
    /// its file's code (the scenario manager)
    [offset(0x180), found(c)]
    int32 trigger;

    /// AI toggle, weather
    /// its file's code (the scenario manager)
    [offset(0x188), found(c)]
    char8 on;

    /// AI target / group, camera
    /// its file's code (the scenario manager)
    [offset(0x189), found(c)]
    char8 use_area;

    /// AI: attack (1) / defend (0)
    /// its file's code (the scenario manager)
    [offset(0x18A), found(c)]
    char8 attack;

    /// kept: the class's other fields all kept, no read of this one in its methods
    [offset(0x18B), found(h, "kept: the class's other fields all kept, no read of this one in its methods")]
    char8 favorites;

    /// AI target: add (1) / remove (0)
    /// its file's code (the scenario manager)
    [offset(0x18C), found(c)]
    char8 add;

    /// change text: clear it
    /// its file's code (the scenario manager)
    [offset(0x18E), found(c)]
    char8 clear;

    /// flare, camera: the area
    /// its file's code (the scenario manager)
    [offset(0x18F), found(c)]
    char8 flare_area;

    /// attribute in %, face at once
    /// its file's code (the scenario manager)
    [offset(0x190), found(c)]
    char8 percent;

    /// tech tree: add (1) / remove (0)
    /// its file's code (the scenario manager)
    [offset(0x191), found(c)]
    char8 tech_add;

    /// its file's code (the scenario manager)
    [offset(0x192), found(c)]
    char8 cal_use_area;

    [address((ee)0x005AD29B, (aoc)0x005BD5EB)]
    extern void run(int32 flag);

    /// thiscall(effect, player index, attribute, mode 0 set / 1 add / 2 subtract, value)
    [address((ee)0x005AC7F5, (aoc)0x005BCB1F)]
    extern void apply_player_attr(int32 player, int32 attr, int32 mode, int32 v);
}

/// a scenario object (EESMObject): units picked by owner, class, area ...
[c_type("ee_scen_object")]
unsafe struct EESMObject
{
    /// its file's code (the scenario manager)
    [offset(0x4), found(c)]
    wstring name;

    /// its file's code (the scenario manager)
    [offset(0x1C), found(c)]
    vector units;

    /// its file's code (the scenario manager)
    [offset(0x28), found(c)]
    sm_player_spec players;

    /// 0 any, 1..14 epoch, 15..71 family, 72..112 RPS, 113+ group
    /// its file's code (the scenario manager)
    [offset(0x2C), found(c)]
    sm_class @class;

    /// 0: by class
    /// its file's code (the scenario manager)
    [offset(0x30), found(c)]
    int32 type;

    /// its file's code (the scenario manager)
    [offset(0x34), found(c)]
    int32 min;

    /// its file's code (the scenario manager)
    [offset(0x38), found(c)]
    int32 max;

    /// its file's code (the scenario manager)
    [offset(0x3C), found(c)]
    int32 area;

    /// 0 sight, 1 range, 2 near
    /// its file's code (the scenario manager)
    [offset(0x40), found(c)]
    sm_relation rel_kind;

    /// its file's code (the scenario manager)
    [offset(0x44), found(c)]
    int32 rel_object;

    /// its file's code (the scenario manager)
    [offset(0x48), found(c)]
    int32 parent;

    /// its file's code (the scenario manager)
    [offset(0x4C), found(c)]
    sm_unit_attr attr;

    /// its file's code (the scenario manager)
    [offset(0x50), found(c)]
    int32 attr_min;

    /// its file's code (the scenario manager)
    [offset(0x54), found(c)]
    int32 attr_max;

    /// its file's code (the scenario manager)
    [offset(0x5C), found(c)]
    int32 state;

    /// its file's code (the scenario manager)
    [offset(0x60), found(c)]
    sm_player_spec seen_players;

    /// its file's code (the scenario manager)
    [offset(0x68), found(c)]
    sm_test_mode area_mode;

    /// its file's code (the scenario manager)
    [offset(0x6C), found(c)]
    sm_test_mode rel_mode;

    /// its file's code (the scenario manager)
    [offset(0x70), found(c)]
    sm_test_mode attr_mode;

    /// its file's code (the scenario manager)
    [offset(0x74), found(c)]
    sm_test_mode seen_mode;

    /// its file's code (the scenario manager)
    [offset(0x78), found(c)]
    sm_test_mode state_mode;

    /// its file's code (the scenario manager)
    [offset(0x7C), found(c)]
    sm_test_mode selected_mode;

    /// its file's code (the scenario manager)
    [offset(0x80), found(c)]
    char8 locked;

    /// its file's code (the scenario manager)
    [offset(0x81), found(c)]
    char8 dynamic;

    /// its file's code (the scenario manager)
    [offset(0x82), found(c)]
    char8 attr_percent;

    /// thiscall(object, unit type): the type passes the object's class / type filter
    [address((ee)0x005B58A3, (aoc)0x005C6051)]
    extern char8 class_test(int32 p2);

    /// thiscall(object, dead too): collects / filters its units, true when any; locks a non-dynamic object on success
    [address((ee)0x005B5CAE, (aoc)0x005C6458)]
    extern char8 eval(char8 p2);
}

/// a trigger's condition reference
unsafe struct ee_trig_cond
{
    /// its file's code (the scenario manager)
    [offset(0x4), found(c)]
    int32 index;

    /// its file's code (the scenario manager)
    [offset(0x8), found(c)]
    int32 join;

    /// its file's code (the scenario manager)
    [offset(0xC), found(c)]
    char8 not;
}

/// a trigger's effect reference
unsafe struct ee_trig_effect
{
    /// its file's code (the scenario manager)
    [offset(0x4), found(c)]
    int32 kind;

    /// its file's code (the scenario manager)
    [offset(0x8), found(c)]
    int32 delay;

    /// its file's code (the scenario manager)
    [offset(0xC), found(c)]
    int32 index;
}

/// a scenario trigger (EESMTrigger)
[c_type("ee_trigger")]
unsafe struct EESMTrigger
{
    /// its file's code (the scenario manager)
    [offset(0x4), found(c)]
    wstring name;

    /// its file's code (the scenario manager)
    [offset(0x18), found(c)]
    wstring desc;

    /// a trigger's condition references, as its +0x40
    [offset(0x30), found(s)]
    vector conds;

    /// its file's code (the scenario manager)
    [offset(0x40), found(c)]
    vector effects;

    /// s the conditions held
    /// its file's code (the scenario manager)
    [offset(0x4C), found(c)]
    int32 true_for;

    /// its file's code (the scenario manager)
    [offset(0x50), found(c)]
    int32 since;

    /// loaded and saved, not its index
    /// its file's code (the scenario manager); an int its load and save stream - not its index (1 for trigger 0)
    [offset(0x54), found(c)]
    int32 _0x54;

    /// its file's code (the scenario manager)
    [offset(0x58), found(c)]
    char8 on;

    /// its file's code (the scenario manager)
    [offset(0x59), found(c)]
    char8 fired;

    /// its file's code (the scenario manager)
    [offset(0x5A), found(c)]
    char8 loop;

    /// checked during cinematics too
    /// its file's code (the scenario manager)
    [offset(0x5B), found(c)]
    char8 cinematic;

    /// thiscall(trigger, scenario time ms): its conditions, true-for, effects queued
    [address((ee)0x005B93B0, (aoc)0x005C9AB5)]
    extern void update(float32 now);
}

/// scenario's data of the game, places in its code and numbers
static unsafe class scenario
{
    /// float 500: the scenario manager checks its triggers every this many ms of game time
    [address((ee)0x008306DC, (aoc)0x00845FCC)]
    static block sm_step;
}

/// the game's functions scenario.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class scenario_functions
{
}
