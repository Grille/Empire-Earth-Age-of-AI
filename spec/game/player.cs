/// a player (EEPlayer, EEComputerPlayer)
unsafe struct EEPlayer
{
    [offset(0x18), found(c)]
    uint8* list_begin;

    [offset(0x24), found(c)]
    wstring name;

    [offset(0x438), found(c)]
    block color;

    /// the first of its units in its linked list, the next one at a unit's +0xC (set_diplomacy, delete_for_room walk
    /// it)
    [offset(0x450), found(c)]
    EEUnit* units;

    [offset(0x45C), found(c)]
    int32 index;

    /// its unit count (the scenario's player attribute unit_count; recounted on load)
    [offset(0x460), found(c)]
    int32 unit_count;

    /// its id in a chat message (msg +0xA)
    [offset(0x464), found(b)]
    int8 msg_id;

    /// no fog
    [offset(0x465), found(c)]
    uint8 explore_all;

    /// not the gaia player: the constructor stores index != 0 (0x55F8xx, AoC the same); set_diplomacy 0x55B8E1 acts
    /// only when both players have it
    [offset(0x466), found(c)]
    uint8 not_gaia;

    /// out of the game (resigned, defeated)
    [offset(0x467), found(c)]
    uint8 @out;

    [offset(0x488), found(b)]
    wstring civ_name;

    [offset(0x4A8), found(c)]
    EEPlayerStatistics stats;

    /// its civilization by the database's name ("Carthage"; civ_name is the game language's: "Karthago") - both
    /// builds' civilization lists read it
    [offset(0x7B0), found(c)]
    cstr civ_name_en;

    [offset((ee)0x874, (aoc)0x884), found(b)]
    block type_stats;

    /// int32 kinds: 0x5564FB adds a finished building's type +0x5B4 (dbobjects' "Wonder bonus") here; set_diplomacy
    /// looks for kind 5 and refreshes the shared sight (0x559472 / 0x5545E3), 0x52D4xx for kind 4
    [offset(0x8EC), found(c), guess]
    vector wonder_bonuses;

    /// itself as a computer player (the computer player's constructor's dynamic cast), NULL for a human
    [offset((ee)0x9B4, (aoc)0x9A4), found(c)]
    EEComputerPlayer* computer;

    [offset((ee)0x9BC, (aoc)0x9AC), found(c)]
    int32* selected_group;

    /// the UI's copy of selected_group (the same class, vtable 0x82D224): 0x55636C swaps it in for one command,
    /// 0x4B0000 copies selected_group back when the server's command returns; the mouse click and hover code read it
    [offset((ee)0x9C0, (aoc)0x9B0), found(c), guess]
    void* ui_group;

    /// the unit set of a wonder bonus 6 (0x40DD73 of its type's +0x5B8, "Effect apply to"), set when such a building is
    /// finished (0x556xxx) with +0xB42 = 1; the player's init zeroes it
    [offset((ee)0x9C8, (aoc)0x9B8), found(c), guess]
    void* wonder_unit_set;

    [offset((ee)0x9D4, (aoc)0x9CC), found(c)]
    EETechTree* tech_tree;

    /// a single selected unit beside selected_group (refcounted at +4): its setter 0x55BB9F clears selected_group,
    /// 0x559E80 clears it on a deselect, set_diplomacy drops it when its owner turns hostile
    [offset((ee)0x9D8, (aoc)0x9D0), found(c), guess]
    EEUnit* selected_unit;

    /// the UI's copy of selected_unit, as ui_group of selected_group: setter 0x55BB7C, 0x55636C and 0x4B0000 set it
    /// from selected_unit; the hover code compares it with the unit under the mouse
    [offset((ee)0x9DC, (aoc)0x9D4), found(c), guess]
    EEUnit* ui_unit;

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

    /// the scenario's ten trigger variables (player_attr 11..20)
    [offset((ee)0xA44, (aoc)0xA3C), found(c)]
    fixed int32 variables[10];

    /// its units' running attack goals: the first sets strategy 2, the last one ending strategy 1 (the attack goal's
    /// constructor and destructor)
    [offset((ee)0xAB4, (aoc)0xAAC), found(c)]
    int32 attacks_running;

    /// tribute sent by resource: food, wood, stone, gold, iron (player_attr 21, 23 ...)
    [offset((ee)0xAB8, (aoc)0xAB0), found(c)]
    fixed int32 tribute_sent[5];

    /// tribute received by resource (player_attr 22, 24 ...)
    [offset((ee)0xACC, (aoc)0xAC4), found(c)]
    fixed int32 tribute_received[5];

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

    /// the cheat 0xD toggles it (0x520F75; its string 30633 "Brainstorm"), build time and cost code read it (0x600xxx,
    /// 0x624DF8, 0x5592xx)
    [offset((ee)0xB41, (aoc)0xB45), found(c), guess]
    uint8 instant_build;

    /// (0: the civ dialog opens)
    [offset((ee)0xB46, (aoc)0xB4A), found(v)]
    uint8 civ_chosen;

    /// the resource zones (map-wide)
    [offset((ee)0xB50, (aoc)0xB84), found(c)]
    ee_zone** zones_begin;

    /// the end of its zones' vector (zones_begin)
    [offset((ee)0xB54, (aoc)0xB88), found(c)]
    ee_zone** zones_end;

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

    /// medium: the share of a unit's cost it pays (record 67 "Unit Cost Reduction - Medium")
    [offset((ee)0xBB4, (aoc)0xBEC), found(c)]
    float32 cost_medium;

    /// the share of its population to be citizens (record 3 "Citizen Percentage"; 0.45 of the cap)
    [offset((ee)0xBDC, (aoc)0xC14), found(c)]
    float32 citizen_share;

    /// its own cap (vf31)
    [offset((ee)0xCCC, (aoc)0xD04), found(b)]
    int32 pop_cap;

    /// percent: it attacks at once when an enemy is hit, not waiting for the attack interval (record 25 "Attack Enemy")
    [offset((ee)0xCD8, (aoc)0xD10), found(c)]
    int32 attack_on_contact;

    /// percent: it attacks at once when another player needs resources (record 26 "Attack to Claim Resource")
    [offset((ee)0xCDC, (aoc)0xD14), found(c)]
    int32 attack_for_resources;

    /// percent: it attacks at once when an enemy attacks it (record 27 "Attack for Revenge")
    [offset((ee)0xCE0, (aoc)0xD18), found(c)]
    int32 attack_revenge;

    /// minutes between attacks, the most (record 6 "Attack Frequency", second value)
    [offset((ee)0xCFC, (aoc)0xD34), found(c)]
    int32 attack_max;

    /// minutes between attacks, the least (record 6, first value)
    [offset((ee)0xD00, (aoc)0xD38), found(c)]
    int32 attack_min;

    /// minutes before its first attack, the most (record 1 "Initial Attack Delay", second value)
    [offset((ee)0xD28, (aoc)0xD60), found(c)]
    int32 first_attack_max;

    /// minutes before its first attack, the least (record 1, first value)
    [offset((ee)0xD2C, (aoc)0xD64), found(c)]
    int32 first_attack_min;

    /// easy: the share of a unit's cost it pays (record 96 "Unit Cost Reduction - Easy & Hard", first value)
    [offset((ee)0xD48, (aoc)0xD80), found(c)]
    int32 cost_easy;

    /// hard: the same (record 96, second value)
    [offset((ee)0xD4C, (aoc)0xD84), found(c)]
    int32 cost_hard;

    /// minutes before it starts a new epoch, the most (record 28 "Epoch Frequency", second value): the epoch timer
    /// draws random(epoch_min, epoch_max) (0x46DE8C)
    [offset((ee)0xD58, (aoc)0xD90), found(c)]
    int32 epoch_max;

    /// minutes before it starts a new epoch, the least (record 28, first value)
    [offset((ee)0xD5C, (aoc)0xD94), found(c)]
    int32 epoch_min;

    /// tiles its citizens walk between a resource and a drop-off before they build one nearer (record 14 "Resource
    /// Drop-off Distance": the loader keeps its square root, the record holds it squared - 36)
    [offset((ee)0xE10, (aoc)0xE4C), found(c)]
    int32 dropoff_distance;

    /// activities a period (+1) (record 66, no name in the game's strings; 4)
    [offset((ee)0xE20, (aoc)0xE5C), found(c)]
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
    /// units it picked to delete: update deletes each one (its vf +0x60 (1, 0, 0)); delete_for_room passes over them
    [offset((ee)0xB80, (aoc)0xBB4), found(c)]
    EEUnit** doomed_begin;

    /// the end of doomed_begin's vector
    [offset((ee)0xB84, (aoc)0xBB8), found(c)]
    EEUnit** doomed_end;

    /// its favourite land unit type (default 0x14; the scenario's Set Favorite Units)
    [offset((ee)0xBA8, (aoc)0xBDC), found(c)]
    int32 favorite_land;

    /// its favourite air unit type (default 0xF2)
    [offset((ee)0xBAC, (aoc)0xBE0), found(c)]
    int32 favorite_air;

    /// its favourite sea unit type (default 0x48; AoC has a fourth at +0xBE8)
    [offset((ee)0xBB0, (aoc)0xBE4), found(c)]
    int32 favorite_sea;

    /// the chance it attacks another computer player instead of a human (record 91 "Computer vs Computer Aggression")
    [offset((ee)0xBB8, (aoc)0xBF0), found(c)]
    float32 cvc_aggression;

    /// the chance a carrier is picked for an attack or defence group (record 52 "Carrier Usage", second value x 0.01)
    [offset((ee)0xBBC, (aoc)0xBF4), found(c)]
    float32 carrier_chance;

    /// the most of its population cap carriers may be (record 52, the float)
    [offset((ee)0xBC0, (aoc)0xBF8), found(c)]
    float32 carrier_pop_share;

    /// minutes added to the interval between attacks per epoch of the game's range (record 22)
    [offset((ee)0xBC4, (aoc)0xBFC), found(c)]
    float32 attack_interval_per_epoch;

    /// the share of its population cap to be specialized units, priests, prophets ... (record 45 "Specialized Unit
    /// Percentage")
    [offset((ee)0xBC8, (aoc)0xC00), found(c)]
    float32 specialized_pct;

    /// not on island maps: the share of an attack group picked to counter the enemy's units (record 53 "RPS Selection
    /// Non-Islands")
    [offset((ee)0xBCC, (aoc)0xC04), found(c)]
    float32 rps_pct;

    /// the same on island maps (record 110 "RPS Selection Islands")
    [offset((ee)0xBD0, (aoc)0xC08), found(c)]
    float32 rps_pct_islands;

    /// the chance a specialized unit takes a free place in an attack or defence group instead of an RPS unit (record 54
    /// "Specialized Unit Selection Chance")
    [offset((ee)0xBD4, (aoc)0xC0C), found(c)]
    float32 specialized_chance;

    /// the chance a military unit is assigned to guard a town (record 90 "Town Guard Percentage")
    [offset((ee)0xBD8, (aoc)0xC10), found(c)]
    float32 town_guard_pct;

    /// deathmatch: attack groups scaled by it until dm_group_until (record 105, x 0.01)
    [offset((ee)0xBE0, (aoc)0xC18), found(c)]
    float32 dm_group_scale;

    /// easy: the factor on catch_up_max (record 107, x 0.01)
    [offset((ee)0xBE4, (aoc)0xC1C), found(c)]
    float32 catch_up_easy;

    /// hard: the factor on catch_up_max
    [offset((ee)0xBE8, (aoc)0xC20), found(c)]
    float32 catch_up_hard;

    /// deathmatch: the factor on catch_up_max, and the epoch timer's
    [offset((ee)0xBEC, (aoc)0xC24), found(c)]
    float32 catch_up_dm;

    /// of two places to guard, the chance it picks the continent one (record 71 "Guard Continent Percentage")
    [offset((ee)0xBF0, (aoc)0xC28), found(c)]
    float32 guard_continent_pct;

    /// of two places to guard, the chance it picks the chokepoint air one (record 77 "Guard Chokepoint-Air Percentage")
    [offset((ee)0xBF4, (aoc)0xC2C), found(c)]
    float32 guard_chokepoint_air_pct;

    /// of two places to guard, the chance it picks the chokepoint one (record 70 "Guard Chokepoint Percentage")
    [offset((ee)0xBF8, (aoc)0xC30), found(c)]
    float32 guard_chokepoint_pct;

    /// of two places to guard, the chance it picks the high ground one (record 72 "Guard High Ground Percentage")
    [offset((ee)0xBFC, (aoc)0xC34), found(c)]
    float32 guard_high_ground_pct;

    /// record 79, no name in the game's strings; one of the guard percentages (records 70..85) by its place
    [offset((ee)0xC00, (aoc)0xC38), found(c), guess]
    float32 guard_pct_79;

    /// record 83, no name in the game's strings; one of the guard percentages (records 70..85) by its place
    [offset((ee)0xC04, (aoc)0xC3C), found(c), guess]
    float32 guard_pct_83;

    /// record 80, no name in the game's strings; one of the guard percentages (records 70..85) by its place
    [offset((ee)0xC08, (aoc)0xC40), found(c), guess]
    float32 guard_pct_80;

    /// record 74, no name in the game's strings; one of the guard percentages (records 70..85) by its place
    [offset((ee)0xC0C, (aoc)0xC44), found(c), guess]
    float32 guard_pct_74;

    /// record 84, no name in the game's strings; one of the guard percentages (records 70..85) by its place
    [offset((ee)0xC10, (aoc)0xC48), found(c), guess]
    float32 guard_pct_84;

    /// record 81, no name in the game's strings; one of the guard percentages (records 70..85) by its place
    [offset((ee)0xC14, (aoc)0xC4C), found(c), guess]
    float32 guard_pct_81;

    /// record 75, no name in the game's strings; one of the guard percentages (records 70..85) by its place
    [offset((ee)0xC18, (aoc)0xC50), found(c), guess]
    float32 guard_pct_75;

    /// record 85, no name in the game's strings; one of the guard percentages (records 70..85) by its place
    [offset((ee)0xC1C, (aoc)0xC54), found(c), guess]
    float32 guard_pct_85;

    /// record 76, no name in the game's strings; one of the guard percentages (records 70..85) by its place
    [offset((ee)0xC20, (aoc)0xC58), found(c), guess]
    float32 guard_pct_76;

    /// of two places to guard, the chance it picks the town one (record 78 "Guard Town Percentage")
    [offset((ee)0xC24, (aoc)0xC5C), found(c)]
    float32 guard_town_pct;

    /// record 73, no name in the game's strings; one of the guard percentages (records 70..85) by its place
    [offset((ee)0xC28, (aoc)0xC60), found(c), guess]
    float32 guard_pct_73;

    /// of two places to guard, the chance it picks the waterway one (record 82 "Guard Waterway Percentage")
    [offset((ee)0xC2C, (aoc)0xC64), found(c)]
    float32 guard_waterway_pct;

    /// the chance a hero is picked for an attack or defence group (record 51 "Hero Usage", second value x 0.01)
    [offset((ee)0xC30, (aoc)0xC68), found(c)]
    float32 hero_chance;

    /// the most of its population cap heroes may be (record 51, the float)
    [offset((ee)0xC34, (aoc)0xC6C), found(c)]
    float32 hero_pop_share;

    /// the most of its population defensive military units may be (record 20)
    [offset((ee)0xC38, (aoc)0xC70), found(c)]
    float32 defence_share;

    /// record 119 "Siege Weapon Percentage" (its float x 0.01)
    [offset((ee)0xC3C, (aoc)0xC74), found(c)]
    float32 siege_pct;

    /// tiles between an attacker and its victim within which the victim still reacts (record 48 "Maximum Attack
    /// Response Range")
    [offset((ee)0xC40, (aoc)0xC78), found(c)]
    float32 response_range;

    /// the share of its population cap to be military units (record 43 "Number of Military Units to Train")
    [offset((ee)0xC44, (aoc)0xC7C), found(c)]
    float32 military_pct;

    /// record 69, no name in the game's strings (0.1); seven functions of the combat and defence compare with it
    /// (0x4813AE ...)
    [offset((ee)0xC48, (aoc)0xC80), found(c)]
    float32 record_69;

    /// the most of its population cap priests may be (record 49 "Priest Usage", the float)
    [offset((ee)0xC4C, (aoc)0xC84), found(c)]
    float32 priest_pop_share;

    /// the chance a priest is picked for an attack or defence group (record 49, second value x 0.01)
    [offset((ee)0xC50, (aoc)0xC88), found(c)]
    float32 priest_chance;

    /// the most of its population cap prophets may be (record 50 "Prophet Usage", the float)
    [offset((ee)0xC54, (aoc)0xC8C), found(c)]
    float32 prophet_pop_share;

    /// the chance a prophet is picked for an attack or defence group (record 50, second value x 0.01)
    [offset((ee)0xC58, (aoc)0xC90), found(c)]
    float32 prophet_chance;

    /// seconds between the attention's redraws (record 68; 30)
    [offset((ee)0xC5C, (aoc)0xC94), found(c)]
    float32 attention_period;

    /// record 12, no name in the game's strings: its second value x 0.01 (the scenario's Set AI Variable writes it into
    /// record_12_a and leaves this one)
    [offset((ee)0xC60, (aoc)0xC98), found(c)]
    float32 record_12_b;

    /// record 12: its first value x 0.01
    [offset((ee)0xC64, (aoc)0xC9C), found(c)]
    float32 record_12_a;

    /// with fewer ships than the humans: the share of the difference it adds to the ships it builds (record 111 "Naval
    /// Unit Weight")
    [offset((ee)0xC68, (aoc)0xCA0), found(c)]
    float32 naval_weight;

    /// the same for aircraft (record 112 "Air Unit Weight")
    [offset((ee)0xC6C, (aoc)0xCA4), found(c)]
    float32 air_weight;

    /// record 95 "Desired Number of Towers", its float (0.99): by the record's text the least room between two towers
    /// in shares of a tower's range
    [offset((ee)0xC70, (aoc)0xCA8), found(c), guess]
    float32 tower_spacing;

    /// deathmatch: the factor on the attack interval by difficulty (record 122)
    [offset((ee)0xC74, (aoc)0xCAC), found(c)]
    fixed float32 dm_attack_factor[3];

    /// deathmatch: the factor on the first attack by difficulty (record 123)
    [offset((ee)0xC80, (aoc)0xCB8), found(c)]
    fixed float32 dm_first_attack_factor[3];

    /// its player index % 10 (the loader), given to the object at +0xB9C (0x484E25): which of ten updates is its turn,
    /// an update stagger
    [offset((ee)0xCA0, (aoc)0xCD8), found(c), guess]
    int32 update_slot;

    /// deathmatch: the least ships it keeps by difficulty (record 44 "Ships to Maintain (Deathmatch)"; the hard one the
    /// rounded float)
    [offset((ee)0xCA4, (aoc)0xCDC), found(c)]
    fixed int32 dm_ships[3];

    /// the least ships it keeps by difficulty (record 5 "Ships to Maintain"; the hard one the rounded float)
    [offset((ee)0xCB0, (aoc)0xCE8), found(c)]
    fixed int32 ships[3];

    /// minutes between the defence manager's naval checks (record 118)
    [offset((ee)0xCBC, (aoc)0xCF4), found(c)]
    int32 naval_check_min;

    /// record 4 "Troops to Assist Allies" by difficulty - the record's text says the troops it sends an ally, but the
    /// loader stores minutes x 60000: a time in ms
    [offset((ee)0xCC0, (aoc)0xCF8), found(c), guess]
    fixed int32 assist_allies_ms[3];

    /// the most carriers in an attack or defence group (record 52 "Carrier Usage", first value)
    [offset((ee)0xCD0, (aoc)0xD08), found(c)]
    int32 carrier_max;

    /// aircraft it sends exploring (record 10)
    [offset((ee)0xCD4, (aoc)0xD0C), found(c)]
    int32 air_explorers;

    /// an attack group's random size range, its top in percent (record 13)
    [offset((ee)0xCE4, (aoc)0xD1C), found(c)]
    int32 group_max_pct;

    /// the same range's bottom in percent
    [offset((ee)0xCE8, (aoc)0xD20), found(c)]
    int32 group_min_pct;

    /// easy: percent on the attack interval (record 7)
    [offset((ee)0xCEC, (aoc)0xD24), found(c)]
    int32 attack_pct_easy;

    /// hard: percent on the attack interval
    [offset((ee)0xCF0, (aoc)0xD28), found(c)]
    int32 attack_pct_hard;

    /// minutes the interval between attacks is clamped to, the most (record 21)
    [offset((ee)0xCF4, (aoc)0xD2C), found(c)]
    int32 attack_ceiling;

    /// the same clamp's least
    [offset((ee)0xCF8, (aoc)0xD30), found(c)]
    int32 attack_floor;

    /// minutes added to the first attack per epoch of the game's range (record 18)
    [offset((ee)0xD04, (aoc)0xD3C), found(c)]
    int32 first_attack_per_epoch;

    /// easy: percent on the first attack's delay (record 2)
    [offset((ee)0xD08, (aoc)0xD40), found(c)]
    int32 first_attack_pct_easy;

    /// hard: percent on the first attack's delay
    [offset((ee)0xD0C, (aoc)0xD44), found(c)]
    int32 first_attack_pct_hard;

    /// minutes the first attack's delay is clamped to, the most (record 19)
    [offset((ee)0xD20, (aoc)0xD58), found(c)]
    int32 first_attack_ceiling;

    /// the same clamp's least
    [offset((ee)0xD24, (aoc)0xD5C), found(c)]
    int32 first_attack_floor;

    /// easy: an attack group's size in percent (record 11; the mod's attack_group_percent)
    [offset((ee)0xD30, (aoc)0xD68), found(c)]
    int32 group_pct_easy;

    /// medium: an attack group's size in percent
    [offset((ee)0xD34, (aoc)0xD6C), found(c)]
    int32 group_pct_medium;

    /// hard: an attack group's size in percent
    [offset((ee)0xD38, (aoc)0xD70), found(c)]
    int32 group_pct_hard;

    /// the fewest of its buildings left before it starts a last all-out attack (record 120 "Bonzai Attack Level"; 9)
    [offset((ee)0xD3C, (aoc)0xD74), found(c)]
    int32 bonzai_level;

    /// record 17, no name in the game's strings (60)
    [offset((ee)0xD40, (aoc)0xD78), found(c)]
    int32 record_17;

    /// record 16, no name in the game's strings (25)
    [offset((ee)0xD44, (aoc)0xD7C), found(c)]
    int32 record_16;

    /// a defence group's random size range, its top in percent (record 87)
    [offset((ee)0xD50, (aoc)0xD88), found(c)]
    int32 defence_group_max_pct;

    /// the same range's bottom in percent
    [offset((ee)0xD54, (aoc)0xD8C), found(c)]
    int32 defence_group_min_pct;

    /// easy: percent on the epoch timer (record 29)
    [offset((ee)0xD60, (aoc)0xD98), found(c)]
    int32 epoch_pct_easy;

    /// hard: percent on the epoch timer
    [offset((ee)0xD64, (aoc)0xD9C), found(c)]
    int32 epoch_pct_hard;

    /// minutes, the least, for the redrawn epoch attempt when a human reaches a later epoch (record 102)
    [offset((ee)0xD68, (aoc)0xDA0), found(c)]
    int32 catch_up_min;

    /// the same, the most
    [offset((ee)0xD6C, (aoc)0xDA4), found(c)]
    int32 catch_up_max;

    /// easy: seconds between expansions (record 97)
    [offset((ee)0xD70, (aoc)0xDA8), found(c)]
    int32 expansion_secs_easy;

    /// medium: seconds between expansions
    [offset((ee)0xD74, (aoc)0xDAC), found(c)]
    int32 expansion_secs_medium;

    /// hard: seconds between expansions
    [offset((ee)0xD78, (aoc)0xDB0), found(c)]
    int32 expansion_secs_hard;

    /// the most heroes in an attack or defence group (record 51 "Hero Usage", first value)
    [offset((ee)0xD7C, (aoc)0xDB4), found(c)]
    int32 hero_max;

    /// attributes it picks to improve per unit type before an attack (record 47 "Unit Attributes Selected for
    /// Improvements")
    [offset((ee)0xD80, (aoc)0xDB8), found(c)]
    int32 improve_attributes;

    /// unit types it picks to improve before an attack (record 46 "Unit Selected for Improvements")
    [offset((ee)0xD84, (aoc)0xDBC), found(c)]
    int32 improve_units;

    /// minutes before a defence group is refilled, the most (record 89)
    [offset((ee)0xD88, (aoc)0xDC0), found(c)]
    int32 refill_delay_max;

    /// the same, the least
    [offset((ee)0xD8C, (aoc)0xDC4), found(c)]
    int32 refill_delay_min;

    /// land units it sends exploring (record 8)
    [offset((ee)0xD90, (aoc)0xDC8), found(c)]
    int32 land_explorers;

    /// record 86, no name in the game's strings (3); a cap in 0x4769F7
    [offset((ee)0xD94, (aoc)0xDCC), found(c)]
    int32 record_86;

    /// the most it builds (record 109, the rounded float)
    [offset((ee)0xD98, (aoc)0xDD0), found(c)]
    int32 anti_air_max;

    /// the most it builds (record 36, the rounded float)
    [offset((ee)0xD9C, (aoc)0xDD4), found(c)]
    int32 airports_max;

    /// the most it builds (record 31, the rounded float)
    [offset((ee)0xDA0, (aoc)0xDD8), found(c)]
    int32 archery_ranges_max;

    /// the most it builds (record 30, the rounded float)
    [offset((ee)0xDA4, (aoc)0xDDC), found(c)]
    int32 barracks_max;

    /// the most it builds (record 33, the rounded float)
    [offset((ee)0xDA8, (aoc)0xDE0), found(c)]
    int32 docks_max;

    /// the most it builds (record 41, the rounded float)
    [offset((ee)0xDAC, (aoc)0xDE4), found(c)]
    int32 granaries_max;

    /// the most it builds (record 40, the rounded float)
    [offset((ee)0xDB0, (aoc)0xDE8), found(c)]
    int32 hospitals_max;

    /// the most it builds (record 37, the rounded float)
    [offset((ee)0xDB4, (aoc)0xDEC), found(c)]
    int32 cyber_factories_max;

    /// the most it builds (record 42, the rounded float)
    [offset((ee)0xDB8, (aoc)0xDF0), found(c)]
    int32 cyber_labs_max;

    /// the most it builds (record 35, the rounded float)
    [offset((ee)0xDBC, (aoc)0xDF4), found(c)]
    int32 naval_yards_max;

    /// the most it builds (record 34, the rounded float)
    [offset((ee)0xDC0, (aoc)0xDF8), found(c)]
    int32 siege_factories_max;

    /// the most it builds (record 32, the rounded float)
    [offset((ee)0xDC4, (aoc)0xE00), found(c)]
    int32 stables_max;

    /// the most it builds (record 108, the rounded float)
    [offset((ee)0xDC8, (aoc)0xE04), found(c)]
    int32 tank_factories_max;

    /// the most it builds (record 39, the rounded float)
    [offset((ee)0xDCC, (aoc)0xE08), found(c)]
    int32 temples_max;

    /// record 95 "Desired Number of Towers", second value (16) - the most, as the other buildings' rounded float
    [offset((ee)0xDD0, (aoc)0xE0C), found(c), guess]
    int32 towers_max;

    /// the most it builds (record 38, the rounded float)
    [offset((ee)0xDD4, (aoc)0xE10), found(c)]
    int32 universities_max;

    /// easy: the least military buildings of each kind (record 103)
    [offset((ee)0xDD8, (aoc)0xE14), found(c)]
    int32 min_mil_buildings_easy;

    /// the same in deathmatch (record 104)
    [offset((ee)0xDDC, (aoc)0xE18), found(c)]
    int32 min_mil_buildings_dm_easy;

    /// medium: the least military buildings of each kind (record 106)
    [offset((ee)0xDE0, (aoc)0xE1C), found(c)]
    int32 min_mil_buildings_medium;

    /// the same in deathmatch
    [offset((ee)0xDE4, (aoc)0xE20), found(c)]
    int32 min_mil_buildings_dm_medium;

    /// hard: the least military buildings of each kind
    [offset((ee)0xDE8, (aoc)0xE24), found(c)]
    int32 min_mil_buildings_hard;

    /// the same in deathmatch
    [offset((ee)0xDEC, (aoc)0xE28), found(c)]
    int32 min_mil_buildings_dm_hard;

    /// easy: percent on its population cap (record 98)
    [offset((ee)0xDF0, (aoc)0xE2C), found(c)]
    int32 pop_pct_easy;

    /// medium: percent on its population cap
    [offset((ee)0xDF4, (aoc)0xE30), found(c)]
    int32 pop_pct_medium;

    /// hard: percent on its population cap
    [offset((ee)0xDF8, (aoc)0xE34), found(c)]
    int32 pop_pct_hard;

    /// the most priests in an attack or defence group (record 49 "Priest Usage", first value)
    [offset((ee)0xDFC, (aoc)0xE38), found(c)]
    int32 priest_max;

    /// the most prophets in an attack or defence group (record 50 "Prophet Usage", first value)
    [offset((ee)0xE00, (aoc)0xE3C), found(c)]
    int32 prophet_max;

    /// seconds between random researches (record 15), times research_pct
    [offset((ee)0xE04, (aoc)0xE40), found(c)]
    int32 research_interval;

    /// easy: percent on research_interval (record 23)
    [offset((ee)0xE08, (aoc)0xE44), found(c)]
    int32 research_pct_easy;

    /// hard: percent on research_interval
    [offset((ee)0xE0C, (aoc)0xE48), found(c)]
    int32 research_pct_hard;

    /// added to its gold and iron at its first update (record 101 "Extra resources")
    [offset((ee)0xE14, (aoc)0xE50), found(c)]
    int32 extra_gold_iron;

    /// ships it sends exploring (record 9)
    [offset((ee)0xE18, (aoc)0xE54), found(c)]
    int32 sea_explorers;

    /// minutes before it upgrades a town center to a capitol (record 99)
    [offset((ee)0xE1C, (aoc)0xE58), found(c)]
    int32 capitol_timer;

    /// seconds: the attention's activity 3 (and 0) only while the next attack is further off (record 65)
    [offset((ee)0xE24, (aoc)0xE60), found(c)]
    int32 attack_lead;

    /// seconds: activity 0 (the epoch) only when the civilization manager's epoch time is within it (record 64)
    [offset((ee)0xE28, (aoc)0xE64), found(c)]
    int32 epoch_lead;

    /// the least stock of a resource the attention needs before an activity (0x43F66F; records 55..63): [0] activity
    /// 3, [1] 1, [2] 0, [3] 7, [4] 6, [5] 4 and 9, [6] 5, [7] 2, [8] 8
    [offset((ee)0xE2C, (aoc)0xE68), found(c)]
    fixed int32 min_stock[9];

    /// the units it has per building before it builds another (record 109 "Desired Number of Anti-Air Buildings", first
    /// value)
    [offset((ee)0xE50, (aoc)0xE8C), found(c)]
    int32 anti_air_units_each;

    /// the units it has per building before it builds another (record 36 "Desired Number of Airports", first value)
    [offset((ee)0xE54, (aoc)0xE90), found(c)]
    int32 airports_units_each;

    /// the units it has per building before it builds another (record 31 "Desired Number of Archery Ranges", first
    /// value)
    [offset((ee)0xE58, (aoc)0xE94), found(c)]
    int32 archery_ranges_units_each;

    /// the units it has per building before it builds another (record 30 "Desired Number of Barracks", first value)
    [offset((ee)0xE5C, (aoc)0xE98), found(c)]
    int32 barracks_units_each;

    /// the units it has per building before it builds another (record 33 "Desired Number of Docks", first value)
    [offset((ee)0xE60, (aoc)0xE9C), found(c)]
    int32 docks_units_each;

    /// the units it has per building before it builds another (record 41 "Desired Number of Granaries", first value)
    [offset((ee)0xE64, (aoc)0xEA0), found(c)]
    int32 granaries_units_each;

    /// the units it has per building before it builds another (record 40 "Desired Number of Hospitals", first value)
    [offset((ee)0xE68, (aoc)0xEA4), found(c)]
    int32 hospitals_units_each;

    /// the units it has per building before it builds another (record 37 "Desired Number of Cyber Factories", first
    /// value)
    [offset((ee)0xE6C, (aoc)0xEA8), found(c)]
    int32 cyber_factories_units_each;

    /// the units it has per building before it builds another (record 42 "Desired Number of Cyber Laboratories", first
    /// value)
    [offset((ee)0xE70, (aoc)0xEAC), found(c)]
    int32 cyber_labs_units_each;

    /// the units it has per building before it builds another (record 35 "Desired Number of Naval Yards", first value)
    [offset((ee)0xE74, (aoc)0xEB0), found(c)]
    int32 naval_yards_units_each;

    /// the units it has per building before it builds another (record 34 "Desired Number of Siege Factories", first
    /// value)
    [offset((ee)0xE78, (aoc)0xEB4), found(c)]
    int32 siege_factories_units_each;

    /// the units it has per building before it builds another (record 32 "Desired Number of Stables", first value)
    [offset((ee)0xE7C, (aoc)0xEBC), found(c)]
    int32 stables_units_each;

    /// the units it has per building before it builds another (record 108 "Desired Number of Tank Factories", first
    /// value)
    [offset((ee)0xE80, (aoc)0xEC0), found(c)]
    int32 tank_factories_units_each;

    /// the units it has per building before it builds another (record 39 "Desired Number of Temples", first value)
    [offset((ee)0xE84, (aoc)0xEC4), found(c)]
    int32 temples_units_each;

    /// walls and towers to other buildings (record 95 "Desired Number of Towers", first value)
    [offset((ee)0xE88, (aoc)0xEC8), found(c)]
    int32 towers_each;

    /// the units it has per building before it builds another (record 38 "Desired Number of Universities", first value)
    [offset((ee)0xE8C, (aoc)0xECC), found(c)]
    int32 universities_units_each;

    /// minutes before it upgrades a settlement to a town center (record 100)
    [offset((ee)0xE90, (aoc)0xED0), found(c)]
    int32 tc_timer;

    /// the most flame throwers it has (record 113 "Desired Number of Flame Throwers")
    [offset((ee)0xE94, (aoc)0xED4), found(c)]
    int32 flame_throwers_max;

    /// ms before it checks whether lost town guards are replaced, by difficulty (record 114 "Town Guard Update",
    /// minutes x 60000)
    [offset((ee)0xE98, (aoc)0xED8), found(c)]
    fixed int32 town_guard_update_ms[3];

    /// the same in deathmatch (record 115 "Town Guard Update - Deathmatch")
    [offset((ee)0xEA4, (aoc)0xEE4), found(c)]
    fixed int32 dm_town_guard_update_ms[3];

    /// ms before it checks whether lost defensive units are replaced, by difficulty (record 88 "Defensive Unit Update",
    /// minutes x 60000)
    [offset((ee)0xEB0, (aoc)0xEF0), found(c)]
    fixed int32 defence_update_ms[3];

    /// the same in deathmatch (record 116 "Defensive Unit Update - Deathmatch")
    [offset((ee)0xEBC, (aoc)0xEFC), found(c)]
    fixed int32 dm_defence_update_ms[3];

    /// minutes between overseas expansions by difficulty (record 121)
    [offset((ee)0xEC8, (aoc)0xF08), found(c)]
    fixed int32 island_expansion_min[3];

    /// game time of the attention's next redraw (0x42F912): now + attention_period
    [offset((ee)0xEDC, (aoc)0xF2C), found(c)]
    uint32 next_attention;

    /// game time it was last hit (its vf33); 60 s later, with no attack running, update ends the attack strategy
    [offset((ee)0xEE0, (aoc)0xF30), found(c)]
    uint32 last_hit;

    /// when update next recomputes the population cap (every 60 s)
    [offset((ee)0xEE4, (aoc)0xF34), found(c)]
    uint32 next_pop_cap;

    /// when update next sends citizens to an unclaimed resource zone (0x430B98)
    [offset((ee)0xEE8, (aoc)0xF38), found(c)]
    uint32 next_expansion;

    /// deathmatch: game time until which dm_group_scale applies (record 105)
    [offset((ee)0xEEC, (aoc)0xF3C), found(c)]
    uint32 dm_group_until;

    /// on a water map: game time from which an overseas expansion is due (0x43123A)
    [offset((ee)0xEF0, (aoc)0xF40), found(c)]
    uint32 next_island_expansion;

    /// 0 until update's one-time setup ran (the attention, the first attack, the expansions scheduled)
    [offset((ee)0xEF5, (aoc)0xF45), found(c)]
    uint8 first_update_done;

    /// next_attention is set; update reschedules while 0 (cleared before saving)
    [offset((ee)0xEF6, (aoc)0xF46), found(c)]
    uint8 attention_scheduled;

    /// its vf33 found a citizen (class 6 or 22) of its hit: the attention's activity 6 (defence) until 60 s after the
    /// last hit
    [offset((ee)0xEF7, (aoc)0xF47), found(c)]
    uint8 citizens_hit;

    /// its vf33 found a hit its unit's type counts: the attention's activity 6 the same way
    [offset((ee)0xEF8, (aoc)0xF48), found(c)]
    uint8 hit;

    /// the scenario's "Active" AI toggle (0); vf31 recomputes the cap only while it is clear
    [offset((ee)0xEF9, (aoc)0xF49), found(c)]
    uint8 active;

    /// set at the end of a load (vf8); the next update runs 0x43FB96 once and clears it
    [offset((ee)0xF04, (aoc)0xF54), found(c)]
    uint8 reload_pending;

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
