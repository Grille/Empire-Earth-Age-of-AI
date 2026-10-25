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
}

/// the object at a player's +0xB98
[c_prefix("b98")]
unsafe struct ee_player_b98
{
    /// read through a player's +0xB98
    [offset(0x2D), found(c)]
    uint8 att;
}

/// a player's statistics, player +0x4A8 (EEPlayerStatistics)
[c_type("ee_player_stats"), c_prefix("plstats")]
unsafe struct EEPlayerStatistics
{
    [offset(0x1C), found(b)]
    block lists;
}
