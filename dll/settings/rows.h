// THE SETTINGS: the keys of aimod.ini (and the test ini's own), one row each - its kind, section, key, default and
// the kind's limits - in the shipped file's order. settings.c makes the rows g_set (g_set.lead_targets: the value in
// force), read once when the DLL loads; what a setting means is told in config/aimod.ini above its key (the file is the
// documentation, the rows are what the code knows). A new setting is a row here and its key in config/aimod.ini with
// the row's default (scripts/build/settings_check.py, run by the build, holds the two together).
//
//   FLAG   0 or 1
//   INT    a whole number lo .. hi
//   STR    a text, shorter than lo characters (the buffer's size)
//   STR_P  a text per player slot: the keys key0 .. key15 (personality_P2)
//
// Every row of aimod.ini can be overridden by the same key in the test ini's [autotest] (A/B tests); a row of section
// autotest exists only there. A row whose reader isn't on g_set yet (sight_counts: sight.c's ini_int still reads the
// file, with the same default) is here so the file's keys are all known.
#define SETTINGS(X) \
    X(FLAG,  general,  enabled,                  1,  0, 0) \
    X(FLAG,  general,  scenario_ai,              1,  0, 0) \
    X(FLAG,  general,  lead_targets,             1,  0, 0) \
    X(FLAG,  general,  auto_victory,             1,  0, 0) \
    X(FLAG,  general,  lead_missiles,            0,  0, 0) \
    X(FLAG,  general,  spread_missiles,          1,  0, 0) \
    X(FLAG,  general,  skip_intro,               0,  0, 0) \
    X(FLAG,  general,  big_selection,            1,  0, 0) \
    X(FLAG,  general,  transport_split,          1,  0, 0) \
    X(FLAG,  general,  transport_unload_spread,  1,  0, 0) \
    X(FLAG,  general,  ship_routes,              0,  0, 0) \
    X(FLAG,  general,  ship_turns,               0,  0, 0) \
    X(FLAG,  general,  turn_fix,                 0,  0, 0) \
    X(FLAG,  general,  sight_counts,             1,  0, 0) \
    X(INT,   general,  chat,                     0,  0, 2) \
    X(INT,   general,  chat_seconds,             15, 0, 3600) \
    X(FLAG,  general,  ai_talk,                  1,  0, 0) \
    X(FLAG,  general,  ai_names,                 1,  0, 0) \
    X(STR,   general,  ai_dir,                   "", MAX_PATH, 0) \
    X(STR,   general,  language,                 "", 16, 0) \
    X(FLAG,  general,  ai_style,                 0,  0, 0) \
    X(FLAG,  general,  strict_parsing,           0,  0, 0) \
    X(FLAG,  general,  save_state,               1,  0, 0) \
    X(STR_P, general,  personality_P,            "", 40, 0) \
    X(FLAG,  general,  control_pipe,             0,  0, 0) \
    X(FLAG,  general,  memory_dumps,             0,  0, 0) \
    X(FLAG,  autotest, aim_hits,                 1,  0, 0) \
    X(INT,   autotest, aim_log,                  0,  0, 1000000)

// The sections whose every key is a row: a key there that isn't one is told in the log (a misspelt key otherwise
// silently kept its default). The others follow as their readers move onto the rows.
internal const char *const k_settings_whole[] = { "general" };
