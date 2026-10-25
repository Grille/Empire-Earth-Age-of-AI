// THE SCENARIO EDITOR'S LISTS: the values the scenario objects' fields hold and the functions take, named as the
// editor shows them (the strings of Language.dll, number in brackets; docs/triggers.md has what each does). A
// list's value is its place in the editor's list unless a value is given.

/// an effect's category (29321) and each category's actions (scen_effect_get_kind)
[c_prefix("SM_CAT")]
enum sm_category : int32 { @object = 1, player, trigger, media, game, ai }

/// 29350
[c_prefix("SM_OBJ")]
enum sm_object_action : int32
{
    create, freeze, unfreeze, kill, remove, task_object, task_area, stance, unit_attribute, owner, class_name,
    face_object, class_attribute
}

/// 29452
[c_prefix("SM_PL")]
enum sm_player_action : int32
{
    victory, script_camera, defeat, diplomacy, set_attribute_level, modify_tech_tree, award_civ_points, fade_in,
    fade_out, visibility_on, visibility_off, minimap_flare
}

/// 29650
[c_prefix("SM_TRIG")]
enum sm_trigger_action : int32 { on, off, fired, not_fired }

/// 29525
[c_prefix("SM_MEDIA")]
enum sm_media_action : int32 { play_sound, change_text, send_dialogue, send_chat_message }

/// 29575
[c_prefix("SM_GAME")]
enum sm_game_action : int32
{
    daylight_hour, day_night_cycle_time, calamity, graphic_effect, start_cinematic_mode, end_cinematic_mode,
    ambient_weather
}

/// 29690; Maintain Object does nothing
[c_prefix("SM_AI")]
enum sm_ai_action : int32
{
    modify_ai_military_target, create_ai_military_group, build_object, maintain_object, set_ai_toggle, set_ai_variable,
    set_favorite_units
}

/// a condition's type (29300 from Player-Attribute; 0 always true, 1 an object), its compare (29310: unsigned)
[c_prefix("SM_COND")]
enum sm_cond_type : int32
{
    @true, @object, player_attribute, player_technology, trigger_attribute, player_diplomacy, game_attribute,
    chat_message_contains
}

[c_prefix("SM")]
enum sm_compare : int32 { less, less_equal, equal, greater_equal, greater }

/// a player spec (29050): a group, or one player - SM_PLAYER(n). In conditions "any" and "all" are swapped
[c_prefix("SM")]
enum sm_player_spec : int32
{
    world, any_human, any_computer, all_human, all_computer, any_player, any_exclude_world, player_0
}

/// a player attribute (29670; the editor offers up to Difficulty Level, the rest work too): the variables, the
/// tribute sent / received by resource, then statistics series n as SM_PA_STATS + n (AoC: only 1..30, then its mining
/// restriction - 29681, player +0xAE4, read and set)
[c_prefix("SM_PA")]
enum sm_player_attr : int32
{
    any_resource, all_resources, stone, gold, wood, food, iron, current_pop, pop_cap, unit_count, difficulty_level,
    variable0, food_sent = 21, food_received, wood_sent, wood_received, stone_sent, stone_received, gold_sent,
    gold_received, iron_sent, iron_received, stats = 30, mining_restriction = 61
}

/// 29500: Set-Attribute Level
[c_prefix("SM_TO")]
enum sm_attr_mode : int32 { exactly, increase_by, decrease_by }

/// a unit attribute (29100; Unit-Attribute sets them), from SM_UA_SHOCK_ARMOR the class's (Class-Attribute), from
/// SM_UA_TOTAL_SHOCK_ARMOR the totals only an object tests; the operation (29590; a class attribute only increases or
/// decreases)
[c_prefix("SM_UA")]
enum sm_unit_attr : int32
{
    hitpoints, any_resource, stone, gold, wood, food, iron, flight_time, cargo_count, garrison_level, ammunition,
    animation_is_paused, reload_time, graphic_scale, power, rotting_time, construction_complete, unit_variable1,
    unit_variable2, unit_variable3, shock_armor, arrow_armor, pierce_armor, gun_armor, laser_armor, missile_armor,
    reload_time_class, attack, work_rate, flight_time_class, leadership, max_objects_carried, repair, hitpoints_class,
    carry_amount, container_healing, range, line_of_sight, max_velocity, power_class, animal_ai_active, camera_focus,
    total_shock_armor, total_arrow_armor, total_pierce_armor, total_gun_armor, total_laser_armor, total_missile_armor,
    total_reload_time, total_attack, total_work_rate, total_flight_time, total_leadership, total_max_objects_carried,
    total_repair, total_hitpoints, total_carry_amount, total_healing, total_range, total_line_of_sight,
    total_max_velocity, total_power, total_placeholder2, total_placeholder3, total_transport_weight, total_weight,
    total_tile_size, total_cruise_altitude, total_minimum_range, total_can_enter_air_transports,
    total_can_enter_land_transports, total_can_enter_sea_transports, total_place_holder1, total_place_holder2,
    total_place_holder3
}

[c_prefix("SM")]
enum sm_unit_op : int32 { increase, decrease, exactly }

/// Task-Object / Task-Area (29375), then a power cast: SM_TASK_POWER + its calamity (sm_calamity); Stance (29475)
[c_prefix("SM_TASK")]
enum sm_task : int32 { normal, patrol, attack_move, load, unload, garrison, power }

[c_prefix("SM_STANCE")]
enum sm_stance : int32
{
    explore, idle, locked, unlocked, town_patrol, scout, stand_ground, guard, harass, cautious, aggressive
}

/// 29510: Diplomacy, Player-Diplomacy
[c_prefix("SM")]
enum sm_diplomacy : int32 { allied, enemy }

/// 29550: Change Text
[c_prefix("SM_TEXT")]
enum sm_text_kind : int32 { hints, instructions, history }

/// a calamity: the records of db\dbcalamity.dat - the editor offers the prophets' (29600, Any: one of them at random),
/// the powers after them work too (campaigns use Time Warp and Battlecry); AoC's file has 13 more, from Flaming Arrows
[c_prefix("SM_CAL")]
enum sm_calamity : int32
{
    any, global_warming, ice_age, earthquake, hurricane, slave_revolt, rebellion, plague, malaria, volcano, fire,
    asteroid, flood, shield_battery, ion_pulse_cannon, time_warp, teleport, resonator, anti_matter_storm, mech_virus,
    battlecry_7, battlecry_8, battlecry_9, flaming_arrows, fa_fire, pilum_attack, mind_control, conscription,
    logic_bomb, planted_explosives, capital_ship_special, cloaking, meteor_storm, line_of_sight, battlecry_centurion,
    special_location
}

/// 29625: Set AI Toggle
[c_prefix("SM_AIT")]
enum sm_ai_toggle : int32 { active, cheat, attack, defend, build_repair, explore, ignore, gather }

/// 29665: Game-Attribute, Difficulty Level
[c_prefix("SM")]
enum sm_difficulty : int32 { easy, medium, hard }

/// an object's tests: each one's mode, the relation's kind (29001); its class filter: any, an epoch (1..14, AoC 1..15),
/// then a family, an RPS class, a unit group - their ranges move with AoC's 15th epoch: sm_class_family(f) ...
[c_prefix("SM")]
enum sm_test_mode : int32 { ignore, must, must_not }

[c_prefix("SM_REL")]
enum sm_relation : int32 { los, range, near }

[c_prefix("SM_CLASS")]
enum sm_class : int32 { any }

/// an area's type: its rectangle, the land or sea its point is on, a player's shape round its point
[c_prefix("SM_AREA")]
enum sm_area_type : int32 { rectangle, region, player_shape }

/// scenario_lists's data of the game, places in its code and numbers
static unsafe class scenario_lists
{
    /// scen_object_class_test: a scenario object's class from here is a family (AoC: 15 epochs before)
    [value((ee)15, (aoc)16)]
    static uint32 sm_class_family;

    /// from here a unit group
    [value((ee)0x71, (aoc)0x75)]
    static uint32 sm_class_group;

    /// from here an RPS class
    [value((ee)72, (aoc)76)]
    static uint32 sm_class_rps;
}
