/// a player (EEPlayer, EEComputerPlayer)
unsafe struct EEPlayer
{
    [offset(0x18), found(c)]
    uint8* list_begin;

    [offset(0x24), found(c)]
    block name;

    [offset(0x438), found(c)]
    block color;

    [offset(0x45C), found(c)]
    int32 index;

    /// its id in a chat message (msg +0xA)
    [offset(0x464), found(b)]
    int8 msg_id;

    /// no fog
    [offset(0x465), found(c)]
    uint8 explore_all;

    /// out of the game (resigned, defeated)
    [offset(0x467), found(c)]
    uint8 @out;

    [offset(0x488), found(b)]
    block civ_name;

    [offset(0x4A8), found(c)]
    EEPlayerStatistics stats;

    [offset((ee)0x874, (aoc)0x884), found(b)]
    block type_stats;

    [offset((ee)0x9BC, (aoc)0x9AC), found(c)]
    int32* selected_group;

    [offset((ee)0x9D4, (aoc)0x9CC), found(c)]
    EETechTree* tech_tree;

    [offset((ee)0x9E0, (aoc)0x9D8), found(c)]
    int32 chat_to;

    [offset((ee)0x9E4, (aoc)0x9DC), found(b)]
    block stance;

    /// orders the attention's activities
    [offset((ee)0xA28, (aoc)0xA20), found(c)]
    int32 strategy;

    /// 1 won, 2 lost
    [offset((ee)0xA30, (aoc)0xA28), found(c)]
    int32 result;

    /// int[7]: 2 food, 3 wood, 4 stone, 5 gold, 6 iron
    [offset((ee)0xAF0, (aoc)0xAF4), found(c)]
    fixed int32 stock[7];

    [offset((ee)0xB10, (aoc)0xB14), found(c)]
    int32 pop_b10;

    /// its objects (< 1: out)
    [offset((ee)0xB14, (aoc)0xB18), found(c)]
    int32 objects;

    [offset((ee)0xB1C, (aoc)0xB20), found(c)]
    int32 pop_b1c;

    [offset((ee)0xB20, (aoc)0xB24), found(c)]
    int32 pop_b20;

    [offset((ee)0xB2C, (aoc)0xB30), found(b)]
    block chat_to_player;

    /// (0: the civ dialog opens)
    [offset((ee)0xB46, (aoc)0xB4A), found(v)]
    uint8 civ_chosen;

    /// the resource zones (map-wide)
    [offset((ee)0xB50, (aoc)0xB84), found(c)]
    ee_zone** zones_begin;

    [offset((ee)0xB8C, (aoc)0xBC0), found(c)]
    EECPCombatManager* combat_mgr;

    /// a computer player's
    [offset((ee)0xB90, (aoc)0xBC4), found(c)]
    EECPCivilizationManager* civ_mgr;

    /// (PLB98: not known)
    [offset((ee)0xB98, (aoc)0xBCC), found(c)]
    ee_player_b98* b98;

    /// 0 easy, 1 medium, 2 hard
    [offset((ee)0xBA4, (aoc)0xBD8), found(c)]
    int32 difficulty;

    [offset((ee)0xBB4, (aoc)0xBEC), found(c)]
    float32 cost_medium;

    /// of the cap (0.45)
    [offset((ee)0xBDC, (aoc)0xC14), found(b)]
    float32 citizen_share;

    /// its own cap (vf31)
    [offset((ee)0xCCC, (aoc)0xD04), found(b)]
    int32 pop_cap;

    [offset((ee)0xCD8, (aoc)0xD10), found(b)]
    int32 attack_on_contact;

    [offset((ee)0xCDC, (aoc)0xD14), found(b)]
    int32 attack_for_resources;

    [offset((ee)0xCE0, (aoc)0xD18), found(b)]
    int32 attack_revenge;

    /// between attacks
    [offset((ee)0xCFC, (aoc)0xD34), found(c)]
    int32 attack_max;

    [offset((ee)0xD00, (aoc)0xD38), found(c)]
    int32 attack_min;

    [offset((ee)0xD28, (aoc)0xD60), found(b)]
    int32 first_attack_max;

    [offset((ee)0xD2C, (aoc)0xD64), found(b)]
    int32 first_attack_min;

    [offset((ee)0xD48, (aoc)0xD80), found(c)]
    int32 cost_easy;

    [offset((ee)0xD4C, (aoc)0xD84), found(c)]
    int32 cost_hard;

    /// the epoch timer's draw, random(+0xD5C, +0xD58) (0x46DE8C)
    [offset((ee)0xD58, (aoc)0xD90), found(c)]
    int32 epoch_max;

    /// the epoch timer's minutes
    [offset((ee)0xD5C, (aoc)0xD94), found(c)]
    int32 epoch_min;

    [offset((ee)0xE10, (aoc)0xE4C), found(c)]
    int32 dropoff_distance;

    /// activities a period (+1)
    [offset((ee)0xE20, (aoc)0xE5C), found(v)]
    int32 attention;

    /// game time
    [offset((ee)0xED8, (aoc)0xF28), found(c)]
    uint32 next_attack;

    [offset((ee)0xEF4, (aoc)0xF44), found(b)]
    block toggles;

    [offset((ee)0xEFA, (aoc)0xF4A), found(c)]
    uint8 boats_for_food;

    [offset((ee)0xEFB, (aoc)0xF4B), found(c)]
    uint8 cheat;

    [offset((ee)0xEFC, (aoc)0xF4C), found(b)]
    uint8 build;

    [offset((ee)0xF02, (aoc)0xF52), found(c)]
    uint8 ignore;

    /// set when 0x43DD2F draws the next attack from the interval (+0xEF5 is the first update's flag, 0x43F91A)
    [offset((ee)0xF05, (aoc)0xF55), found(c)]
    uint8 first_attack_done;

    /// thiscall(player, 2) as EEMResign
    [address((ee)0x0055BCEA, (aoc)0x0056AF5C)]
    extern void resign(int32 how);

    /// EEMSelectUnits carried out on the player
    [address((ee)0x0055B3FB, (aoc)0x0056A658)]
    extern void select(void* selection);

    /// thiscall(player): every tile's sight count (its low byte) into MAP_VIEW_ROWS - a load, the viewed player
    /// changed
    [address((ee)0x0055BC4E, (aoc)0x0056AEC0)]
    extern void view_sight_copy();

    /// thiscall(player, on player, 1): calls off its attack groups on that player
    [address((ee)0x0043033E, (aoc)0x0042DCA6)]
    extern void call_off_attacks(EEPlayer* target, int32 flag);

    [address((ee)0x00430D40, (aoc)0x0042E851)]
    [call_site("maint_delete", (ee)0x00477B17, (aoc)0x0047B635)]
    extern int32 delete_for_room(void* req, int32 flag);

    /// thiscall(player, 1, 0)
    [address((ee)0x00556E11, (aoc)0x005651E0)]
    extern int32 population(int32 with_list, int32 with_aboard);

    /// thiscall(player, type, 0x20, 0): can train or build one now
    [address((ee)0x00555710, (aoc)0x005639E8)]
    extern char8 can_build(int32 type, int32 flags, int32 zero);

    /// EEPlayer::vf31 thiscall(player, 0): the game's unit limit for this player
    [address((ee)0x00556EBC, (aoc)0x0056528B)]
    extern int32 game_limit(int32 zero);

    [address((ee)0x0055B8E1, (aoc)0x0056AB41)]
    extern void set_diplomacy(EEPlayer* other, int32 stance, int32 flag);
}

/// the object at a player's +0xB98
unsafe struct ee_player_b98
{
    /// read through a player's +0xB98
    [offset(0x2D), found(c)]
    uint8 att;
}

/// the computer's player (the AI's): an EEPlayer with the computer's managers - its own functions, the C's cp_
[@base(typeof(EEPlayer)), c_type("ee_player"), vtable((ee)0x008264C0, (aoc)0x0083A57C)]
unsafe struct EEComputerPlayer
{
    /// EEComputerPlayer::AdjustCost (vf27): a price with the cheat
    [address((ee)0x00430FBD, (aoc)0x0042EACE)]
    extern int32 adjust_cost(int32 price);

    /// EEComputerPlayer::vf18(type, amount)
    [address((ee)0x0042F783, (aoc)0x0042D1A5)]
    extern void add_resource(int32 type, int32 amount);

    /// EEComputerPlayer::vf45(cost object, ?, ?): pays a unit/building/tech
    [address((ee)0x0043F53D, (aoc)0x0043ED98)]
    extern void pay(void* what, int32 b, int32 c);

    [address((ee)0x0043F91A, (aoc)0x0043F175)]
    [call_site("cp_update", (ee)0x0042F3CD, (aoc)0x0042CDD1)]
    extern void update(uint32 now);

    /// thiscall(computer player, human): the attempt redrawn soon when the human is ahead
    [address((ee)0x00431533, (aoc)0x0042F064)]
    // its one call: EEPlayer::vf41, a human reached an epoch
    [call_site("epoch_catch_up", (ee)0x0055A5B5, (aoc)0x00569633)]
    extern void epoch_catch_up(EEPlayer* human);

    /// thiscall(player), called at 0x4311E5, 0x43FA9B, 0x474AF0
    [address((ee)0x0042F7A5, (aoc)0x0042D1C7)]
    // the callers of player_update_pop_cap
    [call_site("pop_cap", (ee)0x004311E5, (aoc)0x0042ECF6)]
    [call_site("pop_cap", (ee)0x0043FA9B, (aoc)0x0043F2F6)]
    [call_site("pop_cap", (ee)0x00474AF0, (aoc)0x004781C9)]
    extern void update_pop_cap();

    /// thiscall(player, now): the next attack time (+0xED8)
    [address((ee)0x0043DD2F, (aoc)0x0043D4CD)]
    extern void schedule_attack(uint32 now);
}

/// a player's statistics, player +0x4A8 (EEPlayerStatistics)
[c_type("ee_player_stats")]
unsafe struct EEPlayerStatistics
{
    [offset(0x1C), found(b)]
    block lists;

    [address((ee)0x0055DBCF, (aoc)0x0056D507)]
    extern void add(int32 n, int32 d);
}

/// player's data of the game, places in its code and numbers
static unsafe class player
{
    [address((ee)0x00617780, (aoc)0x0062A2DF)]
    static code call_kill_stat;

    /// EEComputerPlayer's vtable: player_add_resource (econ.c's hook; AoC as before)
    [value(18)]
    static uint32 slot_cp_add_resource;

    /// EEComputerPlayer's vtable: player_pay (econ.c's hook; AoC as before)
    [value(45)]
    static uint32 slot_cp_pay;

    /// EEPlayer: char thiscall(player) - human (EEHumanPlayer's returns 1; highest_human_epoch's test)
    [value(29)]
    static uint32 slot_player_human;

    /// EEPlayer: int thiscall(player, 0), the unit limit (game_limit)
    [value(31)]
    static uint32 slot_unit_limit;

}

/// the game's functions player.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class player_functions
{
    /// cdecl char(player, type, x, y, w, h, 0)
    [address((ee)0x0062E940, (aoc)0x00643CE6), cdecl]
    static extern char8 can_place_game(void* p1, int32 p2, int32 p3, int32 p4, int32 p5, int32 p6, int32 p7);

    /// cdecl(player, type, x, y): a building placed (a foundation)
    [address((ee)0x0062EF8A, (aoc)0x00644C83), cdecl]
    static extern EEUnit* place_foundation(EEPlayer* pl, int32 type, int32 x, int32 y);

    /// thiscall(player +0x874, type): the per-player stats record
    [address((ee)0x00492969, (aoc)0x0051E2A5), thiscall]
    static extern uint8** type_stats_game(void* p1, int32 p2);

    [address((ee)0x004A670A, (aoc)0x0047C428), thiscall]
    static extern uint32 vector_size(void* p1);
}
