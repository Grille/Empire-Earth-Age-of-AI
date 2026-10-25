// THE SETTINGS: the keys of aimod.ini (and the test ini's own), one row each - its kind, section, key, default and
// the kind's limits - in the shipped file's order. settings.c makes the rows g_set (g_set.lead_targets: the value in
// force), read once when the DLL loads; what a setting means is told in config/aimod.ini above its key (the file is the
// documentation, the rows are what the code knows). A new setting is a row here and its key in config/aimod.ini with
// the row's default (scripts/build/settings_check.py, run by the build, holds the two together).
//
//   FLAG    0 or 1
//   INT     a whole number lo .. hi
//   NUM     any whole number (a level, a bit mask, a count: what the code makes of it is its own)
//   PCT     a share kept as whole percents (temperature=80% -> 80), 0% .. lo%
//   SHARE   a share kept as it is (home_guard_share=35% -> 0.35, a float), lo% .. hi%; the default in percent
//   CHOICE  one of the names in lo ("auto|d3d|gdi"), kept as its index (auto 0, d3d 1 ...); the default by name
//   STR     a text, shorter than lo characters (the buffer's size)
//   STR_P   a text per player slot: the keys key0 .. key15 (personality_P2)
//   CHILD   0 or 1 under a master, the row lo (a CHOICE default|enabled|disabled before it, in its section): not set,
//           it follows the master - default: the child's own default, enabled: 1, disabled: 0; set, its own value
//           wins (improved_unit_movement over ship_routes, land_collide ...). The code reads the child as a FLAG.
//
// X: a row of [general] (or of the test ini's [autotest], which exists only there) - g_set.key. S: a row of another
// section, whose keys repeat across sections (enabled, smooth, log) - g_set.section_key. D: a row of [all], one value
// per AI difficulty - g_set_ai[d].key (0 easy, 1 medium, 2 hard), each from [easy] / [medium] / [hard], else [all];
// a part that doesn't tell the difficulties apart reads [1], the medium one. The test ini's [autotest] overrides any
// row by that same name (lead_targets=0, display_present=gdi, llm_enabled=1, avoid_towers=0 - every difficulty).
#define SETTINGS(X, S, D) \
    X(FLAG,   general,  enabled,                  1,  0, 0) \
    X(FLAG,   general,  scenario_ai,              1,  0, 0) \
    X(FLAG,   general,  lead_targets,             1,  0, 0) \
    X(FLAG,   general,  auto_victory,             1,  0, 0) \
    X(FLAG,   general,  lead_missiles,            0,  0, 0) \
    X(FLAG,   general,  spread_missiles,          1,  0, 0) \
    X(FLAG,   general,  skip_intro,               0,  0, 0) \
    X(FLAG,   general,  big_selection,            1,  0, 0) \
    X(FLAG,   general,  goal_markers,             1,  0, 0) \
    X(FLAG,   general,  transport_split,          1,  0, 0) \
    X(FLAG,   general,  transport_unload_spread,  1,  0, 0) \
    X(CHOICE, general,  improved_unit_movement,   "default", "default|enabled|disabled", 0) \
    X(CHILD,  general,  ship_routes,              0,  improved_unit_movement, 0) \
    X(CHILD,  general,  land_routes,              0,  improved_unit_movement, 0) \
    X(CHILD,  general,  land_widen,               0,  improved_unit_movement, 0) \
    X(CHILD,  general,  ship_turns,               0,  improved_unit_movement, 0) \
    X(CHILD,  general,  land_turns,               0,  improved_unit_movement, 0) \
    X(CHILD,  general,  turn_fix,                 1,  improved_unit_movement, 0) \
    X(CHILD,  general,  ship_collide,             1,  improved_unit_movement, 0) \
    X(CHILD,  general,  land_collide,             0,  improved_unit_movement, 0) \
    X(CHILD,  general,  formations,               0,  improved_unit_movement, 0) \
    X(CHILD,  general,  formation_march,          0,  improved_unit_movement, 0) \
    X(CHILD,  general,  formation_follow,         0,  improved_unit_movement, 0) \
    X(FLAG,   general,  sight_counts,             1,  0, 0) \
    X(INT,    general,  chat,                     0,  0, 2) \
    X(INT,    general,  chat_seconds,             15, 0, 3600) \
    X(FLAG,   general,  ai_talk,                  1,  0, 0) \
    X(FLAG,   general,  ai_names,                 1,  0, 0) \
    X(STR,    general,  ai_dir,                   "", MAX_PATH, 0) \
    X(STR,    general,  language,                 "", 16, 0) \
    X(FLAG,   general,  ai_style,                 0,  0, 0) \
    X(FLAG,   general,  strict_parsing,           0,  0, 0) \
    X(FLAG,   general,  save_state,               1,  0, 0) \
    X(STR_P,  general,  personality_P,            "", 40, 0) \
    X(FLAG,   general,  control_pipe,             0,  0, 0) \
    X(FLAG,   general,  memory_dumps,             0,  0, 0) \
    S(CHOICE, camera,   scroll,                   "game", "game|ee|aoc", 0) \
    S(FLAG,   camera,   smooth,                   1,  0, 0) \
    S(CHOICE, display,  mode,                     "native", "native|windowed|borderless", 0) \
    S(CHOICE, display,  lock_mouse,               "auto", "0|1|auto", 0) \
    S(INT,    display,  draw_limit,               4096, 0, 1000000) \
    S(FLAG,   display,  smooth,                   0,  0, 0) \
    S(CHOICE, display,  present,                  "auto", "auto|d3d|gdi|primary", 0) \
    S(FLAG,   display,  vsync,                    0,  0, 0) \
    S(INT,    display,  frame_watch,              0,  0, 1000) \
    S(FLAG,   display,  text_dc,                  1,  0, 0) \
    S(FLAG,   display,  text_bitmap,              1,  0, 0) \
    S(FLAG,   display,  diag,                     0,  0, 0) \
    S(FLAG,   display,  dpi_aware,                1,  0, 0) \
    S(STR,    editor,   resolution,               "ee", 16, 0) \
    S(INT,    sound,    log,                      0,  0, 2) \
    S(FLAG,   sound,    clamp_channels,           1,  0, 0) \
    S(INT,    sound,    channels,                 32, 0, 1000) \
    S(FLAG,   llm,      enabled,                  0,  0, 0) \
    S(STR,    llm,      url,                      "http://localhost:1234/v1/chat/completions", 256, 0) \
    S(STR,    llm,      model,                    "", 96, 0) \
    S(FLAG,   llm,      lines,                    1,  0, 0) \
    S(FLAG,   llm,      chat,                     1,  0, 0) \
    S(FLAG,   llm,      decide,                   0,  0, 0) \
    S(FLAG,   llm,      ai_talk,                  0,  0, 0) \
    S(FLAG,   llm,      ai_talk_show,             1,  0, 0) \
    S(INT,    llm,      ai_talk_gap,              60, 0, 1000000) \
    S(INT,    llm,      ai_talk_turns,            4,  0, 1000000) \
    S(FLAG,   llm,      ai_decide,                0,  0, 0) \
    S(FLAG,   llm,      constrained,              1,  0, 0) \
    S(INT,    llm,      memory,                   8,  0, 100000) \
    S(CHOICE, llm,      memory_mode,              "pair", "pair|player", 0) \
    S(FLAG,   llm,      equal,                    0,  0, 0) \
    S(INT,    llm,      timeout,                  20, 0, 1000000) \
    S(CHOICE, llm,      fallback,                 "rules", "rules|pause", 0) \
    S(INT,    llm,      max_tokens,               300, 0, 1000000) \
    S(PCT,    llm,      temperature,              80, 200, 0) \
    S(FLAG,   llm,      think,                    0,  0, 0) \
    S(STR,    llm,      extra,                    "", 256, 0) \
    S(STR,    llm,      api_key,                  "", 128, 0) \
    S(FLAG,   llm,      log,                      1,  0, 0) \
    D(PCT,   cost_percent,                 100, 1000, 0) \
    D(NUM,   copper_plan,                  1, 0, 0) \
    D(SHARE, epoch_share,                  40, 0, 100) \
    D(NUM,   food_floor,                   18, 0, 0) \
    D(NUM,   budget,                       1, 0, 0) \
    D(SHARE, military_start_share,         30, 0, 100) \
    D(SHARE, military_income_share,        40, 0, 100) \
    D(SHARE, military_income_share_farms,  65, 0, 100) \
    D(CHOICE, epoch_pacing,                "independent", "classic|timer|rush|follow|independent", 0) \
    D(NUM,   epoch_min,                    50, 0, 0) \
    D(NUM,   epoch_max,                    70, 0, 0) \
    D(NUM,   first_attack_min,             0, 0, 0) \
    D(NUM,   first_attack_max,             0, 0, 0) \
    D(NUM,   attack_interval_min,          20, 0, 0) \
    D(NUM,   attack_interval_max,          25, 0, 0) \
    D(PCT,   attack_group_percent,         150, 1000, 0) \
    D(PCT,   attack_on_contact,            0, 100, 0) \
    D(PCT,   attack_for_resources,         0, 100, 0) \
    D(PCT,   attack_revenge,               -1, 100, 0) \
    D(SHARE, citizen_share,                0, 0, 100) \
    D(FLAG,  avoid_towers,                 1, 0, 0) \
    D(FLAG,  attack_weak_spots,            1, 0, 0) \
    D(FLAG,  attack_leash,                 1, 0, 0) \
    D(FLAG,  attack_breach,                1, 0, 0) \
    D(FLAG,  attack_ignore_air,            1, 0, 0) \
    D(FLAG,  honest_vision,                1, 0, 0) \
    D(FLAG,  avoid_towers_citizens,        1, 0, 0) \
    D(FLAG,  citizen_militia,              1, 0, 0) \
    D(FLAG,  militia_estimate,             0, 0, 0) \
    D(SHARE, militia_caught_share,         100, 0, 100) \
    D(FLAG,  early_towers,                 1, 0, 0) \
    D(FLAG,  tower_bait,                   1, 0, 0) \
    D(FLAG,  early_militia,                1, 0, 0) \
    D(FLAG,  early_army,                   0, 0, 0) \
    D(SHARE, early_army_food_share,        10, 0, 100) \
    D(NUM,   early_army_workers,           60, 0, 0) \
    D(FLAG,  early_soldiers,               0, 0, 0) \
    D(FLAG,  citizen_threats,              1, 0, 0) \
    D(FLAG,  bunkers,                      1, 0, 0) \
    D(NUM,   bunker_reserve,               65535, 0, 0) \
    D(FLAG,  attack_siege,                 1, 0, 0) \
    D(NUM,   avoid_towers_weight,          2000, 0, 0) \
    D(NUM,   avoid_towers_margin,          6, 0, 0) \
    D(NUM,   attack_weak_spot_radius,      60, 0, 0) \
    D(FLAG,  build_fix,                    1, 0, 0) \
    D(FLAG,  build_fields,                 1, 0, 0) \
    D(FLAG,  paid_upgrades,                1, 0, 0) \
    D(FLAG,  more_granaries,               1, 0, 0) \
    D(FLAG,  fishing_boat,                 1, 0, 0) \
    D(FLAG,  navy,                         1, 0, 0) \
    D(PCT,   navy_share,                   25, 100, 0) \
    D(NUM,   navy_fleet,                   2, 0, 0) \
    D(PCT,   navy_fleet_pop,               15, 100, 0) \
    D(PCT,   navy_share_max,               50, 100, 0) \
    D(FLAG,  navy_fleet_mass,              1, 0, 0) \
    D(FLAG,  navy_mines,                   1, 0, 0) \
    D(FLAG,  mine_choice,                  1, 0, 0) \
    D(FLAG,  overseas_mines,               1, 0, 0) \
    D(FLAG,  rebalance_workers,            1, 0, 0) \
    D(NUM,   food_plan,                    3, 0, 0) \
    D(NUM,   food_hunt_range,              15, 0, 0) \
    D(NUM,   mine_fill,                    0, 0, 0) \
    D(NUM,   citizen_plan,                 3, 0, 0) \
    D(SHARE, citizen_army_share,           40, 0, 90) \
    D(NUM,   workforce_plan,               0, 0, 0) \
    D(NUM,   workforce_army_cap,           250, 0, 0) \
    D(NUM,   army_plan,                    39, 0, 0) \
    D(NUM,   army_plan_epoch,              5, 0, 0) \
    D(NUM,   army_float_keep,              300, 0, 0) \
    D(NUM,   army_float_keep_mineral,      100, 0, 0) \
    D(NUM,   army_mix_food,                500, 0, 0) \
    D(NUM,   metal_plan,                   0, 0, 0) \
    D(NUM,   metal_cap,                    250, 0, 0) \
    D(NUM,   metal_fleet_pct,              250, 0, 0) \
    D(NUM,   metal_fleet_min,              20, 0, 0) \
    D(NUM,   army_mass,                    0, 0, 0) \
    D(NUM,   army_mass_group,              5, 0, 0) \
    D(NUM,   army_mass_hold,               120, 0, 0) \
    D(PCT,   army_mass_ratio,              130, 1000, 0) \
    D(NUM,   army_mass_wait,               10, 0, 0) \
    D(NUM,   army_mass_tether,             12, 0, 0) \
    D(NUM,   fall_back,                    7, 0, 0) \
    D(PCT,   fall_back_ratio,              50, 1000, 0) \
    D(PCT,   fall_back_hp,                 40, 100, 0) \
    D(NUM,   fall_back_value,              100, 0, 0) \
    D(NUM,   fall_back_dist,               60, 0, 0) \
    D(NUM,   fall_back_breaks,             2, 0, 0) \
    D(PCT,   fall_back_end,                50, 100, 0) \
    D(NUM,   expand_islands,               0, 0, 0) \
    D(NUM,   expand_citizens,              8, 0, 0) \
    D(NUM,   expand_guard,                 3, 0, 0) \
    D(NUM,   expand_minute,                8, 0, 0) \
    D(NUM,   expand_trips,                 4, 0, 0) \
    D(NUM,   expand_max,                   3, 0, 0) \
    D(FLAG,  gather_research,              1, 0, 0) \
    D(FLAG,  defence_layout,               1, 0, 0) \
    D(FLAG,  defence_towers,               1, 0, 0) \
    D(FLAG,  defence_walls,                1, 0, 0) \
    D(NUM,   defence_distance,             10, 0, 0) \
    D(FLAG,  counter_siege,                1, 0, 0) \
    D(FLAG,  anti_air,                     1, 0, 0) \
    D(FLAG,  aa_ahead,                     1, 0, 0) \
    D(NUM,   aa_ahead_buildings,           3, 0, 0) \
    D(FLAG,  hold_town,                    0, 0, 0) \
    D(FLAG,  unit_substitution,            1, 0, 0) \
    D(NUM,   substitute_wait,              90, 0, 0) \
    D(NUM,   train_room,                   65535, 0, 0) \
    D(NUM,   late_targets,                 7, 0, 0) \
    D(NUM,   idle_citizen_slack,           9999, 0, 0) \
    D(NUM,   home_guard,                   4, 0, 0) \
    D(SHARE, home_guard_share,             35, 0, 100) \
    D(FLAG,  muster,                       1, 0, 0) \
    D(FLAG,  citizen_attacks,              0, 0, 0) \
    D(FLAG,  answer_war,                   1, 0, 0) \
    D(NUM,   stance_lines,                 1, 0, 0) \
    D(FLAG,  ai_diplomacy,                 1, 0, 0) \
    D(NUM,   offer_minutes,                2, 0, 0) \
    D(NUM,   alliance_blocks,              1, 0, 0) \
    D(FLAG,  surrender,                    1, 0, 0) \
    D(FLAG,  landing,                      1, 0, 0) \
    D(NUM,   landing_transports,           8, 0, 0) \
    D(NUM,   landing_citizens,             10, 0, 0) \
    D(NUM,   landing_waves,                3, 0, 0) \
    D(FLAG,  landing_bombard,              0, 0, 0) \
    D(FLAG,  landing_keep_transports,      1, 0, 0) \
    D(FLAG,  transport_watchdog,           0, 0, 0) \
    D(FLAG,  fight_estimate,               0, 0, 0) \
    D(NUM,   min_workforce,                40, 0, 0) \
    D(FLAG,  early_requirements,           1, 0, 0) \
    D(FLAG,  reserve_plan,                 1, 0, 0) \
    D(NUM,   mineral_worker_margin,        300, 0, 0) \
    D(SHARE, wild_food_share,              60, 0, 100) \
    D(NUM,   hoard_cap,                    3000, 0, 0) \
    D(NUM,   hoard_cap_mineral,            200, 0, 0) \
    D(FLAG,  pop_follow_human,             0, 0, 0) \
    D(NUM,   reserve_window_min,           5, 0, 0) \
    D(NUM,   site_max_distance,            20, 0, 0) \
    X(FLAG,   autotest, aim_hits,                 1,  0, 0) \
    X(INT,    autotest, aim_log,                  0,  0, 1000000) \
    X(INT,    autotest, formation_kind,           -1, -1, 7) \
    X(NUM,   autotest, citizen_train_ms,             28000, 0, 0) \
    X(NUM,   autotest, citizen_queue,                0, 0, 0) \
    X(FLAG,  autotest, navy_fleet_boats,             1, 0, 0) \
    X(FLAG,  autotest, navy_fleet_coast,             1, 0, 0) \
    X(NUM,   autotest, navy_fleet_raid,              130, 0, 0) \
    X(FLAG,  autotest, landing_flag,                 0, 0, 0) \
    X(FLAG,  autotest, landing_knows_map,            0, 0, 0) \
    X(FLAG,  autotest, landing_trace,                0, 0, 0) \
    X(NUM,   autotest, transport_log,                0, 0, 0)

// THE SECTIONS: every section a row names, one row each - its name, its place (aimod_config.exe's pages sort by it),
// its kind and its title (the page's). Every key of a section of aimod.ini is a row: one that isn't is told in the log
// (a misspelt key otherwise silently kept its default) - the test ini's [autotest] has knobs of its own besides the rows.
//
//   PLAIN   a section of its own rows
//   ALL     [all]: the rows of D, for every AI difficulty
//   DIFF    a difficulty's own section ([easy] / [medium] / [hard]: settings.c k_set_diff), the rows of [all] again
//   TEST    the test ini's own: not in aimod.ini, no page
#define SETTING_SECTIONS(T) \
    T(general,   10, PLAIN, "General") \
    T(camera,    20, PLAIN, "Camera") \
    T(display,   30, PLAIN, "Display") \
    T(editor,    40, PLAIN, "Scenario editor") \
    T(sound,     50, PLAIN, "Sound") \
    T(llm,       60, PLAIN, "Language model") \
    T(all,       70, ALL,   "AI: every difficulty") \
    T(easy,      80, DIFF,  "AI: easy only") \
    T(medium,    90, DIFF,  "AI: medium only") \
    T(hard,     100, DIFF,  "AI: hard only") \
    T(autotest,   0, TEST,  "Tests")

// What the easy AI gets where neither [easy] nor [all] says otherwise (instead of the row's default): it eases off -
// keeps to the human's epoch and population. The shipped [easy] says the same (settings_check.py).
#define SETTINGS_EASY(E) \
    E(epoch_pacing,     "classic") \
    E(pop_follow_human, "1")
