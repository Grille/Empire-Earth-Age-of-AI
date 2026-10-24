// THE SETTINGS: the keys of aimod.ini (and the test ini's own), one row each - its kind, section, key, default and
// the kind's limits - in the shipped file's order. settings.c makes the rows g_set (g_set.lead_targets: the value in
// force), read once when the DLL loads; what a setting means is told in config/aimod.ini above its key (the file is the
// documentation, the rows are what the code knows). A new setting is a row here and its key in config/aimod.ini with
// the row's default (scripts/build/settings_check.py, run by the build, holds the two together).
//
//   FLAG    0 or 1
//   INT     a whole number lo .. hi
//   PCT     a share kept as whole percents (temperature=80% -> 80), 0% .. lo%
//   CHOICE  one of the names in lo ("auto|d3d|gdi"), kept as its index (auto 0, d3d 1 ...); the default by name
//   STR     a text, shorter than lo characters (the buffer's size)
//   STR_P   a text per player slot: the keys key0 .. key15 (personality_P2)
//
// X: a row of [general] (or of the test ini's [autotest], which exists only there) - g_set.key. S: a row of another
// section, whose keys repeat across sections (enabled, smooth, log) - g_set.section_key. The test ini's [autotest]
// overrides any row by that same name (lead_targets=0, display_present=gdi, llm_enabled=1).
#define SETTINGS(X, S) \
    X(FLAG,   general,  enabled,                  1,  0, 0) \
    X(FLAG,   general,  scenario_ai,              1,  0, 0) \
    X(FLAG,   general,  lead_targets,             1,  0, 0) \
    X(FLAG,   general,  auto_victory,             1,  0, 0) \
    X(FLAG,   general,  lead_missiles,            0,  0, 0) \
    X(FLAG,   general,  spread_missiles,          1,  0, 0) \
    X(FLAG,   general,  skip_intro,               0,  0, 0) \
    X(FLAG,   general,  big_selection,            1,  0, 0) \
    X(FLAG,   general,  transport_split,          1,  0, 0) \
    X(FLAG,   general,  transport_unload_spread,  1,  0, 0) \
    X(FLAG,   general,  ship_routes,              0,  0, 0) \
    X(FLAG,   general,  ship_turns,               0,  0, 0) \
    X(FLAG,   general,  turn_fix,                 0,  0, 0) \
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
    X(FLAG,   autotest, aim_hits,                 1,  0, 0) \
    X(INT,    autotest, aim_log,                  0,  0, 1000000)

// The sections whose every key is a row: a key there that isn't one is told in the log (a misspelt key otherwise
// silently kept its default). The others follow as their readers move onto the rows.
internal const char *const k_settings_whole[] = { "general", "camera", "display", "editor", "sound", "llm" };
