// THE GAME: its global state and what reads it (game_map(), game_players(), game_mouse() - the objects then have their
// files), settings and tables, its memory, strings, actions, the simulation and the game's run.

// THE SIMULATION ([G_WORLD_CLOCK], game_physics(): EEPhysics), THE WORLD ([G_WORLD])
internal inline unsigned phys_get_seed(const ee_physics *ph) { return FIELD(ph, PHYS_SEED, unsigned); }       // +0x18 the game generator
internal inline unsigned phys_get_step(const ee_physics *ph) { return FIELD(ph, PHYS_STEP, unsigned); }       // +0x80 steps done
internal inline float phys_get_step_s(const ee_physics *ph) { return FIELD(ph, PHYS_STEP_S, float); }          // +0x8C a step's length (s)
internal inline unsigned phys_get_steps_granted(const ee_physics *ph) { return FIELD(ph, PHYS_STEPS_GRANTED, unsigned); } // +0xB8 steps it may still run
internal inline uint32_t phys_get_time(const ee_physics *ph) { return FIELD(ph, PHYS_TIME, uint32_t); }      // +0xD0 game time (ms)
internal inline unsigned world_get_seed(const ee_world *w) { return FIELD(w, WORLD_SEED, unsigned); }        // +0x54 the map's seed
internal inline void world_set_seed(ee_world *w, unsigned v) { FIELD(w, WORLD_SEED, unsigned) = v; }

// THE GAME DATA ([G_GAME_DATA], game_data()): a chat line's time on screen (ms), the unit limits (default, most)
internal inline int gamedata_get_line_ms(const ee_game_data *gd) { return FIELD(gd, GAMEDATA_LINE_MS, int); }
internal inline int gamedata_get_unit_limit(const ee_game_data *gd) { return FIELD(gd, GAMEDATA_UNIT_LIMIT, int); }
internal inline int gamedata_get_unit_limit_max(const ee_game_data *gd) { return FIELD(gd, GAMEDATA_UNIT_LIMIT_MAX, int); }
// a damage model's map by state (FN_DAMAGE_POINTS' object)
internal inline uint8_t *dmgmodel_map_at(const ee_dmg_model *model) { return (uint8_t *)FIELD_AT(model, DMGMODEL_MAP); }

// THE GAME'S DATA (raw/data.h's G_ rows): its globals read through these, typed - NULL / 0 before they exist
internal inline ee_map *game_map(void) { return *(ee_map **)GA(G_MAP); }                   // +0x1816FC its size in tiles
internal inline ee_physics *game_physics(void) { return *(ee_physics **)GA(G_WORLD_CLOCK); }       // EEPhysics: +0xD0 the game time
internal inline ee_world *game_world(void) { return *(ee_world **)GA(G_WORLD); }                     // the action queue's owner, the UI's
internal inline ee_server *game_server(void) { return *(ee_server **)GA(G_SERVER); }                   // the lockstep server (EEServer, EEClient)
internal inline ee_game_data *game_data(void) { return *(ee_game_data **)GA(G_GAME_DATA); }            // the game data loaded (GAMEDATA)
internal inline void *game_state(void) { return (void *)GA(G_GAME_STATE); }                 // the game state object itself
internal inline ee_player **game_players(void) { return (ee_player **)GA(G_PLAYERS); }      // [index], game_player_count() of them
internal inline unsigned game_player_count(void) { return *(unsigned *)GA(G_PLAYERS_N); }
internal inline unsigned local_player_index(void) { return *(unsigned *)GA(G_LOCAL_PLAYER); }   // the player on this PC
internal inline uintptr_t local_player_index_at(void) { return GA(G_LOCAL_PLAYER); }       // its address (aim.c's thunk)
internal inline ee_unit_type **type_table(void) { return (ee_unit_type **)GA(G_TYPES); }              // [type] its data (world.c type_data)
// the tech data's tables: an epoch's tech lists (0x10 each), the tech effects (0x44 each)
internal inline ee_epoch_techs *epoch_tech_list(int e) { return (ee_epoch_techs *)((uint8_t *)GA(G_EPOCH_TECHS) + e * 0x10); }
internal inline ee_effect *tech_effect(int id) { return (ee_effect *)((uint8_t *)GA(G_EFFECTS) + id * 0x44); }
// the combat tables: the hit table (by game mode: the table and its two variants), [attacker class + target class x 6]
// the damage factors, the unit behaviours (stances)
internal inline uint8_t *hit_table(void) { return *(uint8_t **)GA(G_HIT_TABLE); }
internal inline void *hit_table_a(void) { return *(void **)GA(G_HIT_TABLE_A); }
internal inline void *hit_table_b(void) { return *(void **)GA(G_HIT_TABLE_B); }
internal inline float *damage_table(void) { return (float *)GA(G_DAMAGE_TABLE); }
internal inline void **behaviour_table(void) { return *(void ***)GA(G_BEHAVIOUR_TABLE); }
// the game's settings and state
internal inline int game_mode(void) { return *(int *)GA(G_GAME_MODE); }
internal inline int end_epoch(void) { return *(int *)GA(G_END_EPOCH); }                     // the game's last epoch
internal inline int pop_limit_setting(void) { return *(int *)GA(G_POP_LIMIT); }             // the population limit setting
internal inline int *game_type_var(void) { return (int *)GA(G_GAME_TYPE); }               // 0 random map, 1 scenario, 2 saved ...
internal inline int *difficulty_var(void) { return (int *)GA(G_DIFFICULTY); }             // the AIs' difficulty (the setup's)
internal inline float *game_speed_var(void) { return (float *)GA(G_GAME_SPEED); }
internal inline uint8_t game_paused(void) { return *(uint8_t *)GA(G_PAUSED); }
internal inline uint8_t *teams_locked_var(void) { return (uint8_t *)GA(G_TEAMS_LOCKED); }
internal inline uint8_t *scenario_flag_var(void) { return (uint8_t *)GA(G_SCENARIO_FLAG); }   // a scenario or campaign
internal inline char cheats_on(void) { return *(char *)GA(G_CHEATS_ON); }
internal inline unsigned input_msgs_done(void) { return *(unsigned *)GA(G_INPUT_MSGS_DONE); }   // user inputs processed so far
// the game's objects: the scenario manager, the string resources, the cheat parser, the sound manager, the colour
// table (its rgb records, begin and end)
internal inline ee_scen_mgr *scenario_manager(void) { return *(ee_scen_mgr **)GA(G_SM_MGR); }
internal inline void *string_resources(void) { return *(void **)GA(G_RESOURCES); }
internal inline void *cheat_parser(void) { return *(void **)GA(G_CHEAT_PARSER); }
internal inline ee_sound_mgr *sound_manager(void) { return *(ee_sound_mgr **)GA(G_SOUND_MANAGER); }
internal inline uint8_t **color_table(uint8_t ***end) { *end = *(uint8_t ***)GA(G_COLORS_END); return *(uint8_t ***)GA(G_COLORS_BEGIN); }
// the display and the input (engine/): the camera (EECameraControl: +0x78 look-at, +0xE0 zoom), the scroll speed and
// its setting, the window (GEWindow), the rasterizer and the current screen's index, the UI manager, the mouse
// (RTSMouse), the resolution and fullscreen settings, the sound setting, the draw list's counts, the selection's default
// rectangle, the editor's file buttons, the player name (a UWideString)
internal inline ee_camera *game_camera(void) { return (ee_camera *)GA(G_CAMERA); }
internal inline float scroll_speed(void) { return *(float *)GA(G_SCROLL_SPEED); }
internal inline float scroll_setting(void) { return *(float *)GA(G_SCROLL_SETTING); }
internal inline ee_window *game_window(void) { return *(ee_window **)GA(G_GAME_WINDOW); }
internal inline ee_rasterizer *rasterizer(void) { return *(ee_rasterizer **)GA(G_RASTERIZER); }
internal inline int screen_index(void) { return *(int *)GA(G_SCREEN); }
internal inline void *ui_manager(void) { return *(void **)GA(G_UI_MANAGER); }
internal inline ee_mouse *game_mouse(void) { return *(ee_mouse **)GA(G_MOUSE); }
internal inline int game_res_w(void) { return *(int *)GA(G_GAME_RES_W); }
internal inline int game_res_h(void) { return *(int *)GA(G_GAME_RES_H); }
internal inline uint8_t *fullscreen_var(void) { return (uint8_t *)GA(G_FULLSCREEN); }
internal inline float sound_percent(void) { return *(float *)GA(G_SOUND_PERCENT); }
internal inline uint32_t draw_count(void) { return *(uint32_t *)GA(G_DRAW_N); }
internal inline uint32_t draw_small_count(void) { return *(uint32_t *)GA(G_DRAW_SMALL_N); }
internal inline const float *select_rect(void) { return (const float *)GA(G_SELECT_RECT); }
internal inline int32_t *editor_file_buttons(void) { return (int32_t *)GA(G_EDITOR_FILE_BUTTONS); }
internal inline void *player_name_wstring(void) { return (void *)GA(G_PLAYER_NAME); }

// the game's operator new: memory the game may free
internal inline void *game_new(size_t n) { return GF(FN_NEW)(n); }
// a value of the game's table of settings (0x16: the gatherers a site takes)
internal inline unsigned table_value(int key) { return GF(FN_TABLE_VALUE)((void *)GA(G_TABLE_VALUES), key); }
// the player object of a player index (the game state's, humans too)
internal inline ee_player *game_player(int index) { return GF(FN_GAME_PLAYER)((void *)GA(G_GAME_STATE), index); }
// the stance of player index from towards to (the game's own change, with its message)
internal inline void game_set_stance(int from, int to, int stance) { GF(FN_SET_STANCE)((void *)GA(G_GAME_STATE), from, to, stance); }
// player index out of the game (the resign's first step)
internal inline void game_player_out(int flag, int index) { GF(FN_GAME_PLAYER_OUT)((void *)GA(G_GAME_STATE), flag, index); }
// the local player resigns (the menu's message sent), the exit dialog's confirmation (the originals of diplo.c's hooks)
internal inline void resign_send(void) { GF(FN_RESIGN_SEND)(); }
internal inline void exit_confirm(void) { GF(FN_EXIT_CONFIRM)(); }
// the game's rand() (msvcrt's, through its import): the generator every PC of a game shares
internal inline int game_rand(void) { return (*(int (__cdecl **)(void))GA(IAT_RAND))(); }

// THE GAME'S STRINGS: its runtime's UWideString and UString (16 bytes, the text at +4; the code gives them 32 bytes,
// aligned 8) through the game's imports - made, set, freed as the game does
internal inline void wstring_init(void *s) { ((void (__thiscall *)(void *))*(void **)GA(IAT_WSTRING_CTOR))(s); }
internal inline void wstring_init_text(void *s, const char *text) { ((void (__thiscall *)(void *, const char *))*(void **)GA(IAT_WSTRING_CTOR_C))(s, text); }
internal inline void *wstring_set(void *s, const char *text) { return ((void *(__thiscall *)(void *, const char *))*(void **)GA(IAT_WSTRING_ASSIGN))(s, text); }
internal inline void wstring_reserve(void *s, unsigned long n) { ((void (__thiscall *)(void *, unsigned long))*(void **)GA(IAT_WSTRING_RESERVE))(s, n); }
#define wstring_format(s, ...) ((int (__cdecl *)(void *, const char *, ...))*(void **)GA(IAT_WSTRING_FORMAT))((s), __VA_ARGS__)
internal inline void wstring_free(void *s) { ((void (__thiscall *)(void *))*(void **)GA(IAT_WSTRING_DTOR))(s); }
internal inline void ustring_init(void *s) { ((void (__thiscall *)(void *))*(void **)GA(IAT_USTRING_CTOR))(s); }
internal inline void *ustring_set(void *s, const char *text) { return ((void *(__thiscall *)(void *, const char *))*(void **)GA(IAT_USTRING_ASSIGN))(s, text); }
internal inline void ustring_free(void *s) { ((void (__thiscall *)(void *))*(void **)GA(IAT_USTRING_DTOR))(s); }
// the language file's string id into s (a UWideString), res: the game's string resources
internal inline int resource_string(void *res, int id, void *s) { return ((int (__thiscall *)(void *, int, void *))*(void **)GA(IAT_RESOURCE_STRING))(res, id, s); }

// ACTIONS AND MESSAGES (test/, voice/, diplo.c): what the UI queues or sends - its memory (game_new of its size), then
// made with its _init
internal inline void *pause_action_alloc(void) { return game_new(GA(SIZE_PAUSE_ACTION)); }
internal inline void *save_action_alloc(void) { return game_new(GA(SIZE_SAVE_ACTION)); }
internal inline void *new_unit_msg_alloc(void) { return game_new(GA(SIZE_NEW_UNIT_MSG)); }
// an action into the world's queue (QueueAction on [G_WORLD]), an action's reference given back
internal inline void world_queue_action(void *act) { GF(FN_QUEUE_ACTION)(*(void **)GA(G_WORLD), act); }
internal inline void action_release(void *act) { (*(void (__thiscall **)(void *))GA(IAT_RELEASE))(act); }
// the pause button's action (EEAUserPauseGame: vtable, refcount 1, a byte - it toggles the pause)
internal inline uint8_t *pause_action_init(uint8_t *mem) {
    *(uint32_t *)mem = GA(VT_PAUSE_ACTION);
    *(uint32_t *)(mem + 4) = 1;
    mem[8] = 0;
    return mem;
}
// the save action: path a UString, the folder by value (the constructor destroys its copy)
internal inline void save_action_init(void *mem, struct ustring *path, int flag, struct ustring dir) { GF(FN_SAVE_ACTION_CTOR)(mem, path, flag, dir); }
// a new unit of type for player at xy (EEMHackANewUnit, a network message)
internal inline void new_unit_msg_init(void *mem, int *type, float *xy, char player) { GF(FN_NEW_UNIT_MSG_CTOR)(mem, type, xy, player); }

// THE SIMULATION (test/control.c, trace.c): the originals of the simulation thread's patched calls
internal inline long ts_receive(void *thread, void *a, void *b, void *c) {
    return (*(long (__thiscall **)(void *, void *, void *, void *))GA(IAT_TS_RECEIVE))(thread, a, b, c);
}
internal inline long ts_wait(void *ev, unsigned long ms) { return (*(long (__thiscall **)(void *, unsigned long))GA(IAT_TS_WAIT))(ev, ms); }
// one simulation step (EEPhysics vf8), the game start message executed
internal inline void phys_step(ee_physics *phys) { GF(FN_PHYS_STEP)(phys); }
internal inline void start_exec(ee_msg *msg) { GF(START_EXEC)(msg); }

// THE ENGINE (engine/: the fixes' originals and what they ask)
// the game's cos (UMCos, its own math DLL through its import: the same bits on every PC - the C runtime's may differ)
internal inline float game_cos(float a) { return (*(float (__cdecl **)(float))GA(IAT_COS))(a); }
// the input devices (Low-Level Engine's ISMouse, ISKeyboard through the exe's imports), NULL before they exist
internal inline ee_input_mouse *input_mouse(void) { ee_input_mouse **p = *(ee_input_mouse ***)GA(IMP_IS_MOUSE); return p ? *p : NULL; }
internal inline uint8_t *input_keyboard(void) { uint8_t **p = *(uint8_t ***)GA(IMP_IS_KEYBOARD); return p ? *p : NULL; }
// a resolution switch (the original of display.c's editor hooks), the editor's main form built
internal inline void set_resolution(int w, int h, int force, int flag) { GF(FN_SET_RESOLUTION)(w, h, force, flag); }
internal inline void *editor_form(void *form, void *owner) { return GF(FN_EDITOR_FORM)(form, owner); }
internal inline void settings_save(void) { GF(FN_SETTINGS_SAVE)((void *)GA(G_SETTINGS)); }

// THE GAME'S RUN (the originals of the wiring's hooks: eeaimod.c, persist.c, test/autotest.c)
// a game started from setup (EEGameState::Start, on the game state object)
internal inline char game_start(void *state, ee_setup *setup, void *p2) { return GF(FN_GAME_START)(state, setup, p2); }
// the command line read (1: on to the main menu), a -loadgame save loaded
internal inline char parse_cmdline(const char *cmd) { return GF(FN_PARSE_CMDLINE)(cmd); }
internal inline int load_saved(void *name) { return GF(FN_LOAD_SAVED)(name); }
// the game saved as file in folder (0: saved; the folder is passed by value and destroyed)
internal inline int save_game(void *file, int a, int b, struct ustring folder) { return GF(FN_SAVE_GAME)(file, a, b, folder); }
// the highest epoch of the human players, 0 without
internal inline int highest_human_epoch(void) { return GF(FN_HUMAN_EPOCH)(); }
// a model's damage points (the original of defence.c's guard)
internal inline int damage_points(void *model, int index, unsigned state) { return GF(FN_DAMAGE_POINTS)(model, index, state); }
// a wall of type from a to b (int[2] each) by the wall manager
internal inline void build_wall(void *wall_mgr, int *a, int *b, int type) { GF(FN_BUILD_WALL)(wall_mgr, a, b, type, 0); }
// the shore an idle transport unloads at (the original of landing.c's hook)
internal inline int shore_to_unload(void *mgr, ee_unit *transport) { return GF(FN_SHORE_TO_UNLOAD)(mgr, transport); }
// a vector the game made (its memory freed with the game's delete)
internal inline void vector_free(void *v) { GF(FN_VECTOR_FREE)(v); }

// HOOKS
// one simulation step (EEPhysics' vtable slot 8): the hook, and whether it is in place
typedef void (__thiscall *phys_step_hook)(ee_physics *phys);
internal inline int patch_phys_step(phys_step_hook h) { return patch_code32(GA(VTBL_PHYS_STEP), GA(FN_PHYS_STEP), (uint32_t)(uintptr_t)h); }
internal inline int phys_step_hooked_by(phys_step_hook h) { return *(uint32_t *)GA(VTBL_PHYS_STEP) == (uint32_t)(uintptr_t)h; }
// a model's damage points (the guard: defence.c - a thunk)
internal inline int patch_damage_points(thunk_fn thunk) { return patch_call(GA(CALL_DAMAGE_POINTS), GA(FN_DAMAGE_POINTS), (void *)thunk); }
