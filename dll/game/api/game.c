// THE GAME: its global state and what reads it (game_map(), game_players(), game_mouse() - the objects then have their
// files), settings and tables, its memory, strings, actions, the simulation and the game's run.

// THE SIMULATION ([data_api_world_clock], game_physics(): EEPhysics), THE WORLD ([data_game_ui])
// the screenshot keys' request, posted to the UI thread: size -1 the screen, else the whole map size x size/2, the game
// paused while it renders (run_on 0); the file is EE###.bmp in the game folder
internal inline void ui_screenshot(ee_ui *w, int size, int run_on) { fn_screenshot_game(w, size, run_on, 0); }

// THE GAME DATA ([data_game_game_data], game_data()): a chat line's time on screen (ms), the unit limits (default, most)
// a damage model's map by state (damage_points' object)

// THE GAME'S DATA (spec/game/game.cs's): its globals read through these, typed - NULL / 0 before they exist
internal inline ee_map *game_map(void) { return *(ee_map **)data_game_map; }                   // +0x1816FC its size in tiles
internal inline ee_physics *game_physics(void) { return *(ee_physics **)data_api_world_clock; }       // EEPhysics: +0xD0 the game time
internal inline int phys_substeps(void) { return *(int *)data_game_phys_substeps; }                    // the push's factor on a step (at least 1)
internal inline ee_server *game_server(void) { return *(ee_server **)data_game_server; }                   // the lockstep server (EEServer, EEClient)
internal inline ee_game_data *game_data(void) { return *(ee_game_data **)data_game_game_data; }            // the game data loaded (GAMEDATA)
internal inline void *game_state(void) { return (void *)data_game_game_state; }                 // the game state object itself
internal inline ee_player **game_players(void) { return (ee_player **)data_game_players; }      // [index], game_player_count() of them
internal inline unsigned game_player_count(void) { return *(unsigned *)data_game_players_n; }
internal inline unsigned local_player_index(void) { return *(unsigned *)data_game_local_player; }   // the player on this PC
internal inline uintptr_t local_player_index_at(void) { return data_game_local_player; }       // its address (aim.c's thunk)
internal inline ee_unit_type **type_table(void) { return (ee_unit_type **)data_game_types; }              // [type] its data (world.c type_data)
// the tech data's tables: an epoch's tech lists (0x10 each), the tech effects (0x44 each)
internal inline ee_epoch_techs *epoch_tech_list(int e) { return (ee_epoch_techs *)((uint8_t *)data_game_epoch_techs + e * 0x10); }
internal inline ee_effect *tech_effect(int id) { return (ee_effect *)((uint8_t *)data_game_effects + id * 0x44); }
// the combat tables: the hit table (by game mode: the table and its two variants), [attacker class + target class x 6]
// the damage factors, the unit behaviours (stances)
internal inline uint8_t *hit_table(void) { return *(uint8_t **)data_game_hit_table; }
internal inline void *hit_table_a(void) { return *(void **)data_game_hit_table_a; }
internal inline void *hit_table_b(void) { return *(void **)data_game_hit_table_b; }
internal inline float *damage_table(void) { return (float *)data_game_damage_table; }
internal inline void **behaviour_table(void) { return *(void ***)data_game_behaviour_table; }
// the game's settings and state
internal inline int game_mode(void) { return *(int *)data_game_game_mode; }
internal inline int end_epoch(void) { return *(int *)data_game_end_epoch; }                     // the game's last epoch
internal inline int pop_limit_setting(void) { return *(int *)data_game_pop_limit; }             // the population limit setting
internal inline int *game_type_var(void) { return (int *)data_game_game_type; }               // 0 random map, 1 scenario, 2 saved ...
internal inline int *difficulty_var(void) { return (int *)data_game_difficulty; }             // the AIs' difficulty (the setup's)
internal inline float *game_speed_var(void) { return (float *)data_game_game_speed; }
internal inline uint8_t game_paused(void) { return *(uint8_t *)data_game_paused; }
internal inline uint8_t *teams_locked_var(void) { return (uint8_t *)data_game_teams_locked; }
internal inline uint8_t *scenario_flag_var(void) { return (uint8_t *)data_game_scenario_flag; }   // a scenario or campaign
internal inline char cheats_on(void) { return *(char *)data_game_cheats_on; }
internal inline unsigned input_msgs_done(void) { return *(unsigned *)data_game_input_msgs_done; }   // user inputs processed so far
// the game's objects: the scenario manager, the string resources, the cheat parser, the sound manager, the colour
// table (its rgb records, begin and end)
internal inline ee_scen_mgr *scenario_manager(void) { return *(ee_scen_mgr **)data_game_sm_mgr; }
internal inline void *string_resources(void) { return *(void **)data_game_resources; }
internal inline void *cheat_parser(void) { return *(void **)data_game_cheat_parser; }
internal inline ee_sound_mgr *sound_manager(void) { return *(ee_sound_mgr **)data_game_sound_manager; }
internal inline uint8_t **color_table(uint8_t ***end) { *end = *(uint8_t ***)data_game_colors_end; return *(uint8_t ***)data_game_colors_begin; }
// the display and the input (engine/): the camera (EECameraControl: +0x78 look-at, +0xE0 zoom), the scroll speed and
// its setting, the window (GEWindow), the rasterizer and the current screen's index, the UI manager, the mouse
// (RTSMouse), the resolution and fullscreen settings, the sound setting, the draw list's counts, the selection's default
// rectangle, the editor's file buttons, the player name (a UWideString)
internal inline ee_camera *game_camera(void) { return (ee_camera *)data_game_camera; }
internal inline float scroll_speed(void) { return *(float *)data_game_scroll_speed; }
internal inline float scroll_setting(void) { return *(float *)data_game_scroll_setting; }
internal inline ee_window *game_window(void) { return *(ee_window **)data_game_game_window; }
internal inline ee_rasterizer *rasterizer(void) { return *(ee_rasterizer **)data_game_rasterizer; }
internal inline int screen_index(void) { return *(int *)data_game_screen; }
internal inline void *ui_manager(void) { return *(void **)data_game_ui_manager; }
internal inline ee_mouse *game_mouse(void) { return *(ee_mouse **)data_game_mouse; }
internal inline int game_res_w(void) { return *(int *)data_game_game_res_w; }
internal inline int game_res_h(void) { return *(int *)data_game_game_res_h; }
internal inline uint8_t *fullscreen_var(void) { return (uint8_t *)data_game_fullscreen; }
internal inline float sound_percent(void) { return *(float *)data_game_sound_percent; }
internal inline uint32_t draw_count(void) { return *(uint32_t *)data_game_draw_n; }
internal inline uint32_t draw_small_count(void) { return *(uint32_t *)data_game_draw_small_n; }
internal inline const float *select_rect(void) { return (const float *)data_game_select_rect; }
internal inline int32_t *editor_file_buttons(void) { return (int32_t *)data_game_editor_file_buttons; }
internal inline void *player_name_wstring(void) { return (void *)data_game_player_name; }

// the game's operator new: memory the game may free
// a value of the game's table of settings (0x16: the gatherers a site takes)
internal inline unsigned table_value(int key) { return fn_table_value_game((void *)data_game_table_values, key); }
// the player object of a player index (the game state's, humans too)
internal inline ee_player *game_player(int index) { return fn_game_player_game((void *)data_game_game_state, index); }
// the unit of an id (a message's: the player in bits 24..31) - its handle's +0x30, as EEMLoadTransport finds its
// transport; NULL unknown
internal inline ee_unit *unit_by_id(unsigned id) {
    ee_unit *h = fn_unit_by_id_game((void *)data_game_game_state, id);
    return h ? FIELD(h, of_unit_physics, ee_unit *) : NULL;
}
// the stance of player index from towards to (the game's own change, with its message)
internal inline void game_set_stance(int from, int to, int stance) { fn_set_stance_game((void *)data_game_game_state, from, to, stance); }
// player index out of the game (the resign's first step)
internal inline void game_player_out(int flag, int index) { fn_game_player_out_game((void *)data_game_game_state, flag, index); }
// the local player resigns (the menu's message sent), the exit dialog's confirmation (the originals of diplo.c's hooks)
// the game's rand() (msvcrt's, through its import): the generator every PC of a game shares
internal inline int game_rand(void) { return (*(int (__cdecl **)(void))data_game_iat_rand)(); }

// THE GAME'S STRINGS: its runtime's UWideString and UString (16 bytes, the text at +4; the code gives them 32 bytes,
// aligned 8) through the game's imports - made, set, freed as the game does
internal inline void wstring_init(void *s) { ((void (__thiscall *)(void *))*(void **)data_game_iat_wstring_ctor)(s); }
internal inline void wstring_init_text(void *s, const char *text) { ((void (__thiscall *)(void *, const char *))*(void **)data_game_iat_wstring_ctor_c)(s, text); }
internal inline void *wstring_set(void *s, const char *text) { return ((void *(__thiscall *)(void *, const char *))*(void **)data_game_iat_wstring_assign)(s, text); }
internal inline void wstring_reserve(void *s, unsigned long n) { ((void (__thiscall *)(void *, unsigned long))*(void **)data_game_iat_wstring_reserve)(s, n); }
#define wstring_format(s, ...) ((int (__cdecl *)(void *, const char *, ...))*(void **)data_game_iat_wstring_format)((s), __VA_ARGS__)
internal inline void wstring_free(void *s) { ((void (__thiscall *)(void *))*(void **)data_game_iat_wstring_dtor)(s); }
internal inline void ustring_init(void *s) { ((void (__thiscall *)(void *))*(void **)data_game_iat_ustring_ctor)(s); }
internal inline void *ustring_set(void *s, const char *text) { return ((void *(__thiscall *)(void *, const char *))*(void **)data_game_iat_ustring_assign)(s, text); }
internal inline void ustring_free(void *s) { ((void (__thiscall *)(void *))*(void **)data_game_iat_ustring_dtor)(s); }
// the language file's string id into s (a UWideString), res: the game's string resources
internal inline int resource_string(void *res, int id, void *s) { return ((int (__thiscall *)(void *, int, void *))*(void **)data_game_iat_resource_string)(res, id, s); }

// ACTIONS AND MESSAGES (test/, voice/, diplo.c): what the UI queues or sends - its memory (game_new of its size), then
// made with its _init
internal inline void *pause_action_alloc(void) { return game_new(value_game_size_pause_action); }
internal inline void *save_action_alloc(void) { return game_new(value_game_size_save_action); }
internal inline void *new_unit_msg_alloc(void) { return game_new(value_game_size_new_unit_msg); }
// an action into the world's queue (QueueAction on [data_game_ui]), an action's reference given back
internal inline void ui_queue_action(void *act) { fn_queue_action_game(*(void **)data_game_ui, act); }
internal inline void action_release(void *act) { (*(void (__thiscall **)(void *))data_game_iat_release)(act); }
// the pause button's action (EEAUserPauseGame: vtable, refcount 1, a byte - it toggles the pause)
internal inline uint8_t *pause_action_init(uint8_t *mem) {
    *(uint32_t *)mem = data_game_vt_pause_action;
    *(uint32_t *)(mem + 4) = 1;
    mem[8] = 0;
    return mem;
}
// the save action: path a UString, the folder by value (the constructor destroys its copy)
// a new unit of type for player at xy (EEMHackANewUnit, a network message)

// THE SIMULATION (test/control.c, trace.c): the originals of the simulation thread's patched calls
internal inline long ts_receive(void *thread, void *a, void *b, void *c) {
    return (*(long (__thiscall **)(void *, void *, void *, void *))data_game_iat_ts_receive)(thread, a, b, c);
}
internal inline long ts_wait(void *ev, unsigned long ms) { return (*(long (__thiscall **)(void *, unsigned long))data_game_iat_ts_wait)(ev, ms); }
// one simulation step (EEPhysics vf8), the game start message executed

// THE ENGINE (engine/: the fixes' originals and what they ask)
// the game's cos (UMCos, its own math DLL through its import: the same bits on every PC - the C runtime's may differ)
internal inline float game_cos(float a) { return (*(float (__cdecl **)(float))data_game_iat_cos)(a); }
// the input devices (Low-Level Engine's ISMouse, ISKeyboard through the exe's imports), NULL before they exist
internal inline ee_input_mouse *input_mouse(void) { ee_input_mouse **p = *(ee_input_mouse ***)data_game_imp_is_mouse; return p ? *p : NULL; }
internal inline uint8_t *input_keyboard(void) { uint8_t **p = *(uint8_t ***)data_game_imp_is_keyboard; return p ? *p : NULL; }
// a resolution switch (the original of display.c's editor hooks), the editor's main form built
internal inline void settings_save(void) { fn_settings_save_game((void *)data_game_settings); }

// THE GAME'S RUN (the originals of the wiring's hooks: eeaimod.c, persist.c, test/autotest.c)
// a game started from setup (EEGameState::Start, on the game state object)
// the command line read (1: on to the main menu), a -loadgame save loaded
// the game saved as file in folder (0: saved; the folder is passed by value and destroyed)
// the highest epoch of the human players, 0 without
// a model's damage points (the original of defence.c's guard)
// a wall of type from a to b (int[2] each) by the wall manager
internal inline void build_wall(void *wall_mgr, int *a, int *b, int type) { fn_build_wall_game(wall_mgr, a, b, type, 0); }
// the shore an idle transport unloads at (the original of landing.c's hook)
// a vector the game made (its memory freed with the game's delete)

// HOOKS
// one simulation step (EEPhysics' vtable slot 8): the hook, and whether it is in place
typedef void (__thiscall *phys_step_hook)(ee_physics *phys);
internal inline int patch_phys_step(phys_step_hook h) { return patch_code32(data_game_vtbl_phys_step, (uintptr_t)fn_physics_step, (uint32_t)(uintptr_t)h); }
internal inline int phys_step_hooked_by(phys_step_hook h) { return *(uint32_t *)data_game_vtbl_phys_step == (uint32_t)(uintptr_t)h; }
// a model's damage points (the guard: defence.c - a thunk)
internal inline int patch_damage_points(thunk_fn thunk) { return patch_call(data_game_call_damage_points, (uintptr_t)fn_damage_points, (void *)thunk); }
