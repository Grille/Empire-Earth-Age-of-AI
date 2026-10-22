// THE GAME'S FUNCTIONS, TYPED: the game's functions as plain C functions over types.h's objects - explored_by(site, pl)
// instead of GF(FN_EXPLORED)(FIELD(site, OF(UNIT, 0x14), void *), pl) - and its data as accessors. raw/functions.h has
// these functions' addresses and raw signatures (raw/tables.c's tables); the code outside game/api/ calls these
// instead. Each wrapper is the call itself: inlined, the machine code is the same as GF's (scripts/code_same.py).

// THE GAME'S DATA (raw/data.h's G_ rows): its globals read through these, typed - NULL / 0 before they exist
internal inline uint8_t *game_map(void) { return *(uint8_t **)GA(G_MAP); }                   // +0x1816FC its size in tiles
internal inline uint8_t *game_physics(void) { return *(uint8_t **)GA(G_WORLD_CLOCK); }       // EEPhysics: +0xD0 the game time
internal inline void *game_world(void) { return *(void **)GA(G_WORLD); }                     // the action queue's owner, the UI's
internal inline void *game_server(void) { return *(void **)GA(G_SERVER); }                   // the lockstep server (EEServer, EEClient)
internal inline uint8_t *game_data(void) { return *(uint8_t **)GA(G_GAME_DATA); }            // the game data loaded (GAMEDATA)
internal inline void *game_state(void) { return (void *)GA(G_GAME_STATE); }                 // the game state object itself
internal inline ee_player **game_players(void) { return (ee_player **)GA(G_PLAYERS); }      // [index], game_player_count() of them
internal inline unsigned game_player_count(void) { return *(unsigned *)GA(G_PLAYERS_N); }
internal inline unsigned local_player_index(void) { return *(unsigned *)GA(G_LOCAL_PLAYER); }   // the player on this PC
internal inline uintptr_t local_player_index_at(void) { return GA(G_LOCAL_PLAYER); }       // its address (aim.c's thunk)
internal inline ee_unit_type **type_table(void) { return (ee_unit_type **)GA(G_TYPES); }              // [type] its data (world.c type_data)
// the tech data's tables: an epoch's tech lists (0x10 each), the tech effects (0x44 each)
internal inline uint8_t *epoch_tech_list(int e) { return (uint8_t *)GA(G_EPOCH_TECHS) + e * 0x10; }
internal inline uint8_t *tech_effect(int id) { return (uint8_t *)GA(G_EFFECTS) + id * 0x44; }
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
internal inline uint8_t *scenario_manager(void) { return *(uint8_t **)GA(G_SM_MGR); }
internal inline void *string_resources(void) { return *(void **)GA(G_RESOURCES); }
internal inline void *cheat_parser(void) { return *(void **)GA(G_CHEAT_PARSER); }
internal inline uint8_t *sound_manager(void) { return *(uint8_t **)GA(G_SOUND_MANAGER); }
internal inline uint8_t **color_table(uint8_t ***end) { *end = *(uint8_t ***)GA(G_COLORS_END); return *(uint8_t ***)GA(G_COLORS_BEGIN); }
// the display and the input (engine/): the camera (EECameraControl: +0x78 look-at, +0xE0 zoom), the scroll speed and
// its setting, the window (GEWindow), the rasterizer and the current screen's index, the UI manager, the mouse
// (RTSMouse), the resolution and fullscreen settings, the sound setting, the draw list's counts, the selection's default
// rectangle, the editor's file buttons, the player name (a UWideString)
internal inline uint8_t *game_camera(void) { return (uint8_t *)GA(G_CAMERA); }
internal inline float scroll_speed(void) { return *(float *)GA(G_SCROLL_SPEED); }
internal inline float scroll_setting(void) { return *(float *)GA(G_SCROLL_SETTING); }
internal inline uint8_t *game_window(void) { return *(uint8_t **)GA(G_GAME_WINDOW); }
internal inline uint8_t *rasterizer(void) { return *(uint8_t **)GA(G_RASTERIZER); }
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

// THE GAME
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
// the citizens working site (all players)
internal inline unsigned site_gatherers(ee_unit *site) { return GF(FN_SITE_GATHERERS)((void *)GA(G_GAME_STATE), site); }
// the map's area id at pt (kind 1 water, 2 land): one id per connected region
internal inline unsigned short map_area(int *pt, int kind) {
    return ((unsigned short (__thiscall *)(void *, int *, int))GA(FN_REGION))((void *)GA(G_MAP_AREAS), pt, kind);
}
// the damage bonus of an attacker type (data atd) against a target type (data ttd), the players' indices
internal inline float attack_bonus(ee_unit_type *atd, ee_unit_type *ttd, int api, int tpi) {
    return GF(FN_ATTACK_BONUS)((void *)GA(G_BONUS_TABLE), FIELD(atd, OF(TD, 0x4C8), int), ttd, api, tpi);
}
// the local player resigns (the menu's message sent), the exit dialog's confirmation (the originals of diplo.c's hooks)
internal inline void resign_send(void) { GF(FN_RESIGN_SEND)(); }
internal inline void exit_confirm(void) { GF(FN_EXIT_CONFIRM)(); }
// the network messages: executed (the originals of diplo.c's hooks), a chat line built in mem, one sent to everyone
internal inline void msg_resign(void *msg) { GF(FN_RESIGN)(msg); }
internal inline void msg_pay_tribute(void *msg) { GF(FN_PAY_TRIBUTE)(msg); }
internal inline void msg_ingame_chat(void *msg) { GF(FN_INGAME_CHAT)(msg); }
internal inline void *msg_chat_init(void *mem, void *wide_text) { return GF(FN_CHAT_MSG_CTOR)(mem, wide_text); }
internal inline void server_send_all(void *net, void *msg) {
    ((void (__thiscall *)(void *, void *, int))(*(void ***)net)[GA(SLOT_SERVER_SEND)])(net, msg, 0xFF);
}
// a statistics counter n of the stats object +d (the kill counter's original: diplo.c)
internal inline void stat_add(void *stats, int n, int d) { GF(FN_STAT_ADD)(stats, n, d); }
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

// THE UI AND THE PLAYERS' LINES (voice/)
// a chat or system line on ui: the sender's colour (rgb floats), name and the line (UWideStrings), shown ms
internal inline void ui_line(void *ui, const void *rgb, void *name, void *line, int ms) { GF(FN_UI_LINE)(ui, (void *)rgb, name, line, ms, 0); }
// a typed line read as a cheat by the parser: its code and argument (0: no cheat)
internal inline void cheat_parse(void *parser, void *wtext, int *code, int *arg) { GF(FN_CHEAT_PARSE)(parser, wtext, code, arg); }
// a cheat message built in mem (game_new(GA(SIZE_CHEAT_MSG)))
internal inline void *msg_cheat_init(void *mem, int code, int arg) { return GF(FN_CHEAT_MSG_CTOR)(mem, code, arg); }
// the colour table's rgb of a colour index (the original of persona.c's hook)
internal inline void *game_color_rgb(unsigned idx) { return GF(FN_COLOR_RGB)(idx); }
// one of the game's own AI lines (base: its string block, ms shown; the original of persona.c's hook)
internal inline char native_chat(uint8_t *pim, int base, int ms) { return GF(FN_NATIVE_CHAT)(pim, base, ms); }

// ACTIONS AND MESSAGES (test/, voice/, diplo.c): what the UI queues or sends - its memory (game_new of its size), then
// made with its _init
internal inline void *pause_action_alloc(void) { return game_new(GA(SIZE_PAUSE_ACTION)); }
internal inline void *save_action_alloc(void) { return game_new(GA(SIZE_SAVE_ACTION)); }
internal inline void *camera_action_alloc(void) { return game_new(GA(SIZE_CAMERA_ACTION)); }
internal inline void *new_unit_msg_alloc(void) { return game_new(GA(SIZE_NEW_UNIT_MSG)); }
internal inline void *msg_chat_alloc(void) { return game_new(GA(SIZE_MSG_CHAT)); }
internal inline void *msg_cheat_alloc(void) { return game_new(GA(SIZE_CHEAT_MSG)); }
// an action into the world's queue (QueueAction on [G_WORLD]), an action's reference given back
internal inline void world_queue_action(void *act) { ((void (__thiscall *)(void *, void *))GA(FN_QUEUE_ACTION))(*(void **)GA(G_WORLD), act); }
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
// the camera's move to a tile (EEACenterCamera): made, then set (it queues itself)
internal inline void *camera_action_init(void *mem) { return GF(FN_CAMACT_CTOR)(mem); }
internal inline void camera_action_set(void *act, int x, int y) { GF(FN_CAMACT_SET)(act, x, y); }
// a new unit of type for player at xy (EEMHackANewUnit, a network message)
internal inline void new_unit_msg_init(void *mem, int *type, float *xy, char player) { GF(FN_NEW_UNIT_MSG_CTOR)(mem, type, xy, player); }
// a scenario effect run now (its trigger's conditions and delay not asked)
internal inline void sm_run_effect(void *effect, int flag) { GF(SM_RUN_EFFECT)(effect, flag); }

// THE SIMULATION (test/control.c, trace.c): the originals of the simulation thread's patched calls
internal inline long ts_receive(void *thread, void *a, void *b, void *c) {
    return (*(long (__thiscall **)(void *, void *, void *, void *))GA(IAT_TS_RECEIVE))(thread, a, b, c);
}
internal inline long ts_wait(void *ev, unsigned long ms) { return (*(long (__thiscall **)(void *, unsigned long))GA(IAT_TS_WAIT))(ev, ms); }
// one simulation step (EEPhysics vf8), the game start message executed
internal inline void phys_step(void *phys) { GF(FN_PHYS_STEP)(phys); }
internal inline void start_exec(void *msg) { ((void (__thiscall *)(void *))GA(START_EXEC))(msg); }

// THE ENGINE (engine/: the fixes' originals and what they ask)
// the game's cos (UMCos, its own math DLL through its import: the same bits on every PC - the C runtime's may differ)
internal inline float game_cos(float a) { return (*(float (__cdecl **)(float))GA(IAT_COS))(a); }
// a shot's ballistic path from -> to for unit u (type: its live stats): reachable (pitch out); the aim at target
internal inline char ballistic(ee_unit *u, ee_unit_stats *type, float *from, float *to, float *pitch) {
    return ((char (__cdecl *)(ee_unit *, ee_unit_stats *, float *, float *, float *))GA(FN_BALLISTIC))(u, type, from, to, pitch);
}
internal inline void unit_aim(ee_unit *u, ee_unit **target, float *aim, void *zero) {
    ((void (__cdecl *)(ee_unit *, ee_unit **, float *, void *))GA(FN_AIM))(u, target, aim, zero);
}
// a projectile class's: the projectile down (1: on the ground or the target), a missile launched, a unit's shot fired
internal inline char proj_down(void *cls, void *projectile) { return GF(FN_PROJ_DOWN)(cls, projectile); }
internal inline int missile_launch(void *cls, ee_unit *m, float *aim) { return GF(FN_MISSILE_LAUNCH)(cls, m, aim); }
internal inline void unit_fire(void *cls, ee_unit *u) { GF(FN_FIRE)(cls, u); }
// a unit type's projectile class (type data +0x3FC) by its vtable: a plain projectile (arrows, shells), a missile
internal inline int is_projectile_class(const void *pc) { return *(const uintptr_t *)pc == GA(VT_PROJECTILE); }
internal inline int is_missile_class(const void *pc) { return *(const uintptr_t *)pc == GA(VT_MISSILE); }
// the scroll action executed (the original of camera.c's hook), the camera moved by dx, dy; a scroll action object
// made by hand (camera.c's test: vtable, then its fields)
internal inline void scroll_execute(void *self) { GF(FN_SCROLL_EXECUTE)(self); }
internal inline void camera_move(float dx, float dy) { GF(FN_CAMERA_MOVE)((void *)GA(G_CAMERA), dx, dy); }
internal inline void scroll_action_init(uint8_t *obj) { *(uint32_t *)obj = GA(VT_SCROLL); }
// the selection: its unit ids packed into a message (the original of select.c's hook), a double-click's message built,
// carried out on the player, the selected group's count (the group's vtable)
internal inline unsigned sel_pack(uint8_t *list, uint32_t player_byte, uint8_t *out) { return GF(FN_SEL_PACK)(list, player_byte, out); }
internal inline void select_msg_init(void *msg, float *rect, int type, int flags) { GF(FN_SELECT_MSG_INIT)(msg, rect, type, flags); }
internal inline void player_select(ee_player *pl, void *selection) { GF(FN_SELECT_UNITS)(pl, selection); }
internal inline unsigned group_count(void *grp) { return ((unsigned (__thiscall *)(void *))(*(void ***)grp)[GA(SLOT_GROUP_COUNT)])(grp); }
// the sound manager's channel count set (as the options screen does: the original of sound.c's hook through its import)
internal inline int snd_set_max_channels(void *sm, int n) { return (*(int (__thiscall **)(void *, int))GA(IAT_SND_SETMAXCH))(sm, n); }
// the Miles mixer plugin's channels (Miles Sound System Mixer.dll at base: a vector of 8-byte entries) and its maximum
internal inline uint8_t *mix_channels(uintptr_t base, uint8_t **end) {
    uint8_t *v = *(uint8_t **)(base + GA(RVA_MIX_CHANNELS));
    if (end) *end = v ? *(uint8_t **)(base + GA(RVA_MIX_CHANNELS) + 4) : NULL;
    return v;
}
internal inline int mix_max_channels(uintptr_t base) { return *(int *)(base + GA(RVA_MIX_MAX_CHANNELS)); }
// the input devices (Low-Level Engine's ISMouse, ISKeyboard through the exe's imports), NULL before they exist
internal inline uint8_t *input_mouse(void) { uint8_t **p = *(uint8_t ***)GA(IMP_IS_MOUSE); return p ? *p : NULL; }
internal inline uint8_t *input_keyboard(void) { uint8_t **p = *(uint8_t ***)GA(IMP_IS_KEYBOARD); return p ? *p : NULL; }
// the mouse's position kept in its range, its update (the original of display.c's hook)
internal inline void mouse_clamp(void *m) { ((void (__thiscall *)(void *))GA(FN_MOUSE_CLAMP))(m); }
internal inline void mouse_update(ee_mouse *m) { ((void (__thiscall *)(void *))GA(FN_MOUSE_UPDATE))(m); }
// a resolution switch (the original of display.c's editor hooks), the editor's main form built
internal inline void set_resolution(int w, int h, int force, int flag) { ((void (__stdcall *)(int, int, int, int))GA(FN_SET_RESOLUTION))(w, h, force, flag); }
internal inline void *editor_form(void *form, void *owner) { return GF(FN_EDITOR_FORM)(form, owner); }
// the UI manager's current form, an edit box's text, the settings saved to the registry (as the options screens do)
internal inline uint8_t *ui_current_form(void *uim) { return ((uint8_t *(__thiscall *)(void *))*(void **)GA(IAT_UIM_FORM))(uim); }
internal inline const char *edit_text(void *edit) { return ((const char *(__thiscall *)(void *, unsigned long))*(void **)GA(IAT_EDIT_GET_TEXT))(edit, 0); }
internal inline void settings_save(void) { GF(FN_SETTINGS_SAVE)((void *)GA(G_SETTINGS)); }

// THE GAME'S RUN (the originals of the wiring's hooks: eeaimod.c, persist.c, test/autotest.c)
// a game started from setup (EEGameState::Start, on the game state object)
internal inline char game_start(void *state, void *setup, void *p2) { return ((char (__thiscall *)(void *, void *, void *))GA(FN_GAME_START))(state, setup, p2); }
// the command line read (1: on to the main menu), a -loadgame save loaded
internal inline char parse_cmdline(const char *cmd) { return ((char (__cdecl *)(const char *))GA(FN_PARSE_CMDLINE))(cmd); }
internal inline int load_saved(void *name) { return ((int (__cdecl *)(void *))GA(FN_LOAD_SAVED))(name); }
// the game saved as file in folder (0: saved; the folder is passed by value and destroyed)
internal inline int save_game(void *file, int a, int b, struct ustring folder) { return GF(FN_SAVE_GAME)(file, a, b, folder); }

// PLAYERS
// the population: player +0xB10, with_list: and the units of its +0x18 list, with_aboard: and the units inside
// transports (type family 0x1D) - (1, 0) is what the AI's unit manager counts
internal inline int population(ee_player *pl, int with_list, int with_aboard) { return GF(FN_POPULATION)(pl, with_list, with_aboard); }
// the game's unit limit (EEPlayer::vf31(0) called directly: a computer player's own vf31 returns its cap, +0xCCC)
internal inline int game_unit_limit(ee_player *pl) { return GF(FN_GAME_LIMIT)(pl, 0); }
// can it build or train type now: available, affordable, a builder (0x20: as the AI's own checks ask)
internal inline char can_build(ee_player *pl, int type) { return GF(FN_CAN_BUILD)(pl, type, 0x20, 0); }
// a foundation of type laid out at x, y (the player pays): the new building, NULL: none
internal inline ee_unit *place_foundation(ee_player *pl, int type, int x, int y) { return GF(FN_PLACE_BUILDING)(pl, type, x, y); }
// is the map object explored by pl (a site's +0x14: its map object)
internal inline char explored_by(ee_unit *site, ee_player *pl) { return GF(FN_EXPLORED)(FIELD(site, OF(UNIT, 0x14), void *), pl); }
// the highest epoch of the human players, 0 without
internal inline int highest_human_epoch(void) { return GF(FN_HUMAN_EPOCH)(); }
// can a building of type stand at x, y (size x size tiles: the game's placement test, every tile)
internal inline char can_place_area(ee_player *pl, int type, int x, int y, int size) { return GF(FN_CAN_PLACE)(pl, type, x, y, size, size, 0); }
// the player's per-type stats records (player +0x874, a vector): their count, the record of type
internal inline unsigned type_stats_count(ee_player *pl) { return GF(FN_VECTOR_SIZE)(FIELD_AT(pl, OF(PL, 0x874))); }
internal inline ee_unit_stats **type_stats(ee_player *pl, int type) { return (ee_unit_stats **)GF(FN_TYPE_STATS)(FIELD_AT(pl, OF(PL, 0x874)), type); }
// the player resigns (how: 2 as the resign message does)
internal inline void player_resign(ee_player *pl, int how) { GF(FN_PLAYER_RESIGN)(pl, how); }
// player a's stance towards b (1 enemy, 0 ally) set as the game does (EEPlayer's diplomacy, both its lists)
internal inline void set_diplomacy(ee_player *a, ee_player *b, int stance, int flag) {
    ((void (__thiscall *)(void *, void *, int, int))GA(FN_SET_DIPLOMACY))(a, b, stance, flag);
}
// the player's attacks on target called off (the units back to the pool)
internal inline void call_off_attacks(ee_player *pl, ee_player *target, int flag) { GF(FN_CALL_OFF_ATTACK)(pl, target, flag); }
// the combat manager (player +0xB8C) answers an attack by target (the game's revenge attack)
internal inline void revenge(ee_combat_mgr *combat_mgr, ee_player *target) { GF(FN_REVENGE)(combat_mgr, target); }
// units of the player deleted to make room for req (the original of fleet.c's hook)
internal inline int delete_for_room(ee_player *pl, void *req, int flag) { return GF(FN_DELETE_FOR_ROOM)(pl, req, flag); }
// the game's unit limit for the player (EEPlayer::vf31(0) through the player's own vtable, as FN_GAME_LIMIT; it pops
// its argument)
internal inline int unit_limit(ee_player *pl) { return ((int (__thiscall *)(void *, int))(*(void ***)pl)[GA(SLOT_UNIT_LIMIT)])(pl, 0); }
// does the game count the player as human (EEHumanPlayer's answers 1; FN_HUMAN_EPOCH's test)
internal inline char player_is_human(ee_player *pl) { return ((char (__thiscall *)(void *))(*(void ***)pl)[GA(SLOT_PLAYER_HUMAN)])(pl); }

// COMPUTER PLAYERS (the originals of econ.c's hooks)
internal inline void cp_add_resource(ee_player *pl, int type, int amount) { GF(FN_CP_ADD_RESOURCE)(pl, type, amount); }
internal inline void cp_pay(ee_player *pl, void *what, int b, int c) { GF(FN_CP_PAY)(pl, what, b, c); }
// the computer player's update (EEComputerPlayer::Update - the original of engine/scenario_ai.c's hook)
internal inline void cp_update(ee_player *pl, uint32_t now) { ((void (__thiscall *)(ee_player *, uint32_t))GA(FN_CP_UPDATE))(pl, now); }
// the next attack's time drawn (player +0xED8), a price as the cheat makes it (AdjustCost, vf27)
internal inline void cp_schedule_attack(ee_player *pl, uint32_t now) { GF(FN_SCHEDULE_ATTACK)(pl, now); }
internal inline int cp_adjust_cost(ee_player *pl, int price) { return GF(FN_ADJUST_COST)(pl, price); }
// the next epoch attempt's time drawn (civilization manager +0x58: SetNextEpochAdvanceTime)
internal inline void cm_set_epoch_time(ee_civ_mgr *cm, uint32_t now, char flag) { GF(FN_SET_EPOCH_TIME)(cm, now, flag); }
// the population cap that follows the humans, set every minute (player +0xCCC)
internal inline void cp_update_pop_cap(ee_player *pl) { GF(FN_POP_CAP)(pl); }
// the game's catch-up: ai's next epoch attempt redrawn soon when human is ahead
internal inline void cp_epoch_catch_up(ee_player *ai, ee_player *human) { GF(FN_EPOCH_CATCH_UP)(ai, human); }

// UNITS
// the player a unit belongs to (player 0: nature - wild animals, resources)
// the unit says sound (an answer to an order: 0x1F51 "building")
internal inline void unit_say(ee_unit *u, int sound) { GF(FN_UNIT_SAY)(u, sound); }
// the unit's goal set (the unit takes it over); append: after its current one, prio: the order's priority
internal inline void unit_set_goal(ee_unit *u, ee_goal *goal, int append, int prio) {
    ((void (__thiscall *)(void *, void *, char, int))GA(FN_SET_GOAL))(u, goal, (char)append, prio);
}
// the goal given into one of the unit's goal slots (the original of landing.c's unload retry)
internal inline void unit_give_goal(ee_unit *u, ee_goal *goal, int slot) { GF(FN_GIVE_GOAL)(u, goal, slot); }
// the units aboard a transport or inside a building
internal inline int units_inside(ee_unit *u) { return GF(FN_UNITS_INSIDE)(u); }
// the distance between two units (tiles)
internal inline float unit_distance(ee_unit *a, ee_unit *b) { return GF(FN_UNIT_DISTANCE)(a, b); }
// the unit changed in place into the type of new_td, owner player index pi (defence.c: a line upgrade)
internal inline int unit_convert(ee_unit *u, int pi, ee_unit_type *new_td) { return GF(FN_UNIT_CONVERT)(u, pi, (uint32_t)new_td); }
// the unit taken out of the map's unit list, given back to the game's pool (the originals of attack.c's guards)
internal inline void unit_unlink(ee_unit *u) { GF(FN_UNIT_UNLINK)(u); }
internal inline void unit_release(ee_unit *u) { GF(FN_UNIT_RELEASE)(u); }
// the unit's type told that the unit got a goal (EEUnitClass's vtable, as the game's own goal assignment does)
internal inline void type_goal_assigned(ee_unit_type *td, ee_unit *u) { ((void (__thiscall *)(void *, void *))(*(void ***)td)[GA(SLOT_TYPE_ASSIGN)])(td, u); }
// a model's damage points (the original of defence.c's guard)
internal inline int damage_points(void *model, int index, unsigned state) { return GF(FN_DAMAGE_POINTS)(model, index, state); }

// GOALS: built in game_new(GA(SIZE_GOAL_..)) memory, then given with unit_set_goal
// build or repair: pl's builder at building
internal inline ee_goal *goal_build_init(void *mem, ee_player *pl, ee_unit *building, int flag) { return (ee_goal *)GF(FN_BUILD_GOAL)(mem, pl, building, flag); }
// attack target (Art of Conquest's constructor takes a byte more: 0)
internal inline ee_goal *goal_attack_init(void *mem, ee_unit *u, ee_unit *target) {
    if (build_is("aoc")) return ((ee_goal *(__thiscall *)(void *, void *, void *, int, int))GA(FN_GOAL_ATTACK_CTOR))(mem, u, target, 1, 0);
    return ((ee_goal *(__thiscall *)(void *, void *, void *, int))GA(FN_GOAL_ATTACK_CTOR))(mem, u, target, 1);
}
// the units of vec (a std::vector) into holder (a transport or a building)
internal inline ee_goal *goal_load_init(void *mem, ee_unit *holder, void *vec) { return (ee_goal *)GF(FN_GOAL_LOAD_CTOR)(mem, holder, vec, 1); }
// the holder's units out at x, y
internal inline ee_goal *goal_unload_init(void *mem, ee_unit *holder, int x, int y, int flag) { return (ee_goal *)GF(FN_GOAL_UNLOAD_CTOR)(mem, holder, x, y, flag); }
// a move to xy (float[2])
internal inline ee_goal *goal_move_init(void *mem, float *xy) { return (ee_goal *)GF(FN_GOAL_MOVE_CTOR)(mem, xy, 1, 1, 0, 0.0f, 0, 0); }
// a goal's kind, by its vtable: an attack (EEUGAttackUnit, its target +0x30), a move (EEUGLocation), build or repair
// (EEUGBuildRepair, the building +0x3C), a load (EEUGLoadTransport, the holder +0x50), an unload (EEUGUnloadTransport)
internal inline int goal_is_attack(const ee_goal *g) { return *(const uintptr_t *)g == GA(VT_GOAL_ATTACK); }
internal inline int goal_is_location(const ee_goal *g) { return *(const uintptr_t *)g == GA(VT_GOAL_LOCATION); }
internal inline int goal_is_build(const ee_goal *g) { return *(const uintptr_t *)g == GA(VT_GOAL_BUILD); }
internal inline int goal_is_load(const ee_goal *g) { return *(const uintptr_t *)g == GA(VT_GOAL_LOAD); }
internal inline int goal_is_unload(const ee_goal *g) { return *(const uintptr_t *)g == GA(VT_GOAL_UNLOAD); }
// the memory of a goal the game may free (game_new of its size), then made with its _init
internal inline void *goal_build_alloc(void) { return game_new(GA(SIZE_GOAL_BUILD)); }
internal inline void *goal_attack_alloc(void) { return game_new(GA(SIZE_GOAL_ATTACK)); }
internal inline void *goal_load_alloc(void) { return game_new(GA(SIZE_GOAL_LOAD)); }
internal inline void *goal_unload_alloc(void) { return game_new(GA(SIZE_GOAL_UNLOAD)); }
internal inline void *goal_move_alloc(void) { return game_new(GA(SIZE_GOAL_MOVE)); }
// a load goal's part the units walk into (the goal's vtable)
internal inline ee_goal *goal_part(ee_goal *goal) { return ((ee_goal *(__thiscall *)(void *))(*(void ***)goal)[GA(SLOT_GOAL_PART)])(goal); }
// may the goal be handed to the unit (the original of attack.c's hook_goal_cast)
internal inline char goal_fits(ee_goal *goal, void *unit) { return GF(FN_GOAL_FBA_CHECK)(goal, unit); }
// a goal set's answer for the unit (the original of landing.c's hook)
internal inline int goalset_ask(void *set, ee_unit *u, int *out) { return GF(FN_GOALSET_ASK)(set, u, out); }

// THE AI'S MANAGERS
// the build manager places type at x, y (0, 0: where it finds room) - the original of econ.c's hook_place
internal inline char bm_place(ee_build_mgr *bm, int type, int x, int y) { return GF(FN_PLACE)(bm, type, x, y); }
internal inline char bm_queued(ee_build_mgr *bm, int type) { return GF(FN_BM_QUEUED)(bm, type); }
// one building of type into the build manager's list (as the AI's own requests: (1, 0))
internal inline void bm_request(ee_build_mgr *bm, int type) { GF(FN_BM_REQUEST)(bm, type, 1, 0); }
// the resource manager's turn (the originals of the wiring's hooks, eeaimod.c): its gatherer targets computed (1: they
// changed), its citizens assigned, its update
internal inline char rm_compute_targets(ee_resource_mgr *rm) { return ((char (__thiscall *)(void *))GA(FN_COMPUTE_TARGETS))(rm); }
internal inline char rm_assign_citizens(ee_resource_mgr *rm) { return ((char (__thiscall *)(void *))GA(FN_ASSIGN_CITIZENS))(rm); }
internal inline void rm_update(ee_resource_mgr *rm, uint32_t now) { ((void (__thiscall *)(void *, uint32_t))GA(FN_RM_UPDATE))(rm, now); }
// the citizen u to work at site (a resource, a field, a building); rm_assign_as: with the game's third argument
internal inline void rm_assign(ee_resource_mgr *rm, ee_unit *u, ee_unit *site) { GF(FN_ASSIGN_TO_SITE)(rm, u, site, 0); }
internal inline void rm_assign_as(ee_resource_mgr *rm, ee_unit *u, ee_unit *site, int c) { GF(FN_ASSIGN_TO_SITE)(rm, u, site, c); }
// the unit manager's citizens trained (the original of trace.c's hook)
internal inline uintptr_t um_train_citizens(ee_unit_mgr *um, uintptr_t a) { return GF(FN_TRAIN_CITIZENS)(um, a); }
// the unit manager's units of type: a std::deque of ee_unit *, NULL: none
internal inline uint8_t *um_type_list(ee_unit_mgr *um, int type) { return GF(FN_UM_TYPE_LIST)(um, type); }
// the unit orders (req: type, count ...) - the original of econ.c's hook_train_units
internal inline uintptr_t um_train(ee_unit_mgr *um, int *req, uintptr_t b) { return GF(FN_TRAIN_UNITS)(um, req, b); }
// tech id to research (requests its building if missing, adds its cost to the demand)
internal inline void rs_schedule(ee_research_mgr *rs, int id) { GF(FN_RS_SCHEDULE)(rs, id); }
// count of the next epoch's missing requirements asked for (buildings, units, techs)
internal inline void rs_request_requirements(ee_research_mgr *rs, int count) { GF(FN_REQUEST_TECHS)(rs, count); }
// a zone's site of kind (2: a resource site)
internal inline ee_unit *zone_site(ee_zone *z, int kind) { return (ee_unit *)GF(FN_ZONE_SITE)(z, kind); }
// enemies near x, y as the build manager sees them (its threat test)
internal inline char bm_enemy_near(ee_build_mgr *bm, int x, int y) { return ((char (__thiscall *)(void *, int, int))GA(FN_ENEMY_NEAR))(bm, x, y); }
// entry (type, x, y) into the build manager's list of epoch e (the lists at +4, 0x14 each)
internal inline void bm_queue_add(ee_build_mgr *bm, int e, int *entry) { GF(FN_BQ_ADD)(FIELD_AT(bm, OF(BM, 0x4) + e * 0x14), entry); }
// a wall of type from a to b (int[2] each) by the wall manager
internal inline void build_wall(void *wall_mgr, int *a, int *b, int type) { GF(FN_BUILD_WALL)(wall_mgr, a, b, type, 0); }
// the unit (its state, unit +0x30) in none of the civilization manager's groups
internal inline char cm_unit_free(void *cm, void *unit_state) { return GF(FN_CM_UNIT_FREE)(cm, unit_state); }
// a unit of type ordered but not started yet, one in production (the unit manager's queues)
internal inline char um_pending(ee_unit_mgr *um, int type) { return ((char (__thiscall *)(void *, int))GA(FN_UM_PENDING))(um, type); }
internal inline char um_in_production(ee_unit_mgr *um, int type) { return ((char (__thiscall *)(void *, int))GA(FN_UM_IN_PRODUCTION))(um, type); }
// the unit manager's citizens, the citizens it wants (share x population cap: the original of citizens.c's hook)
internal inline int um_citizens(ee_unit_mgr *um) { return GF(FN_UM_CITIZENS)(um); }
internal inline int um_citizens_wanted(ee_unit_mgr *um) { return GF(FN_UM_CITIZENS_WANTED)(um); }
// the shore an idle transport unloads at (the original of landing.c's hook)
internal inline int shore_to_unload(void *mgr, ee_unit *transport) { return GF(FN_SHORE_TO_UNLOAD)(mgr, transport); }
// the fishing boat type of a player (the original of navy.c's hook)
internal inline int fish_type(ee_civ_mgr *cm) { return GF(FN_FISH_TYPE)(cm); }

// THE RESOURCE MANAGER'S SEARCHES (the originals of sites.c's hooks; arguments as the game passes them)
internal inline ee_unit *rm_find_mine(ee_resource_mgr *rm, uintptr_t type, ee_unit *unit, uintptr_t rev) {
    return ((void *(__thiscall *)(void *, uintptr_t, uintptr_t, uintptr_t))GA(FN_FIND_MINE))(rm, type, (uintptr_t)unit, rev);
}
internal inline ee_unit *rm_find_food(ee_resource_mgr *rm, uintptr_t need_drop, uintptr_t near_base, ee_unit *unit) {
    return ((void *(__thiscall *)(void *, uintptr_t, uintptr_t, uintptr_t))GA(FN_FIND_FOOD))(rm, need_drop, near_base, (uintptr_t)unit);
}
internal inline ee_unit *rm_find_wood(ee_resource_mgr *rm, uintptr_t flag, ee_unit *unit) {
    return ((void *(__thiscall *)(void *, uintptr_t, uintptr_t))GA(FN_FIND_WOOD))(rm, flag, (uintptr_t)unit);
}
// the lists of fields, unbuilt fields and animals to hunt (n wanted, into out)
internal inline void *rm_farms(ee_resource_mgr *rm, uintptr_t n, uintptr_t out) {
    return ((void *(__thiscall *)(void *, uintptr_t, uintptr_t))GA(FN_FARMS))(rm, n, out);
}
internal inline void *rm_unbuilt_farms(ee_resource_mgr *rm, uintptr_t n, uintptr_t out) {
    return ((void *(__thiscall *)(void *, uintptr_t, uintptr_t))GA(FN_UNBUILT_FARMS))(rm, n, out);
}
internal inline void *rm_hunt(ee_resource_mgr *rm, uintptr_t n, uintptr_t f, uintptr_t out) {
    return ((void *(__thiscall *)(void *, uintptr_t, uintptr_t, uintptr_t))GA(FN_HUNT))(rm, n, f, out);
}

// a land attack subgroup (EECPASGLand, by its vtable: the subgroups of the other kinds have none of its fields)
internal inline int asg_is_land(const ee_attack_subgroup *sg) { return *(const uintptr_t *)sg == GA(VT_ASG_LAND); }

// AN ATTACK GROUP OF THE GAME'S OWN (test/testcmd.c aigroup: as the scenario effect "Create AI Military Group"):
// a task (0x34) with its location (0x18, the point at +4) and target, a strategy (0x44), the unit ids (a vector<int>
// in the game's heap) - the combat manager asked, then its command (cm +0x48) makes the group
internal inline void task_init(void *task) { GF(FN_TASK_CTOR)(task); }
internal inline void task_free(void *task) { GF(FN_TASK_DTOR)(task); }
internal inline void task_set_target(void *task, ee_unit *t) { GF(FN_TASK_SET_TARGET)(task, t); }
internal inline void task_set_location(void *task_loc, void *loc) { GF(FN_TASK_SET_LOCATION)(task_loc, loc); }
internal inline void location_init(void *loc) { GF(FN_LOCATION_CTOR)(loc); }
internal inline void point_set(void *point, int *pt) { GF(FN_POINT_SET)(point, pt); }
internal inline void strategy_init(void *s) { GF(FN_STRATEGY_CTOR)(s); }
internal inline void strategy_reset(void *s) { GF(FN_STRATEGY_INIT)(s); }
internal inline void strategy_free(void *s) { GF(FN_STRATEGY_DTOR)(s); }
internal inline char attack_plan_ok(ee_combat_mgr *cm, void *task, void *strategy, void *ids) { return GF(FN_ATTACK_PLAN_OK)(cm, task, strategy, ids); }
internal inline ee_attack_group *attack_group_new(ee_attack_cmd *cmd, void *task, void *strategy, void *ids) { return (ee_attack_group *)GF(FN_ATTACK_GROUP_NEW)(cmd, task, strategy, ids); }
// a vector the game made (its memory freed with the game's delete)
internal inline void vector_free(void *v) { GF(FN_VECTOR_FREE)(v); }
// the map's area object at a tile (regions, continents)
internal inline uint8_t *area_at(int *pt) { return GF(FN_AREA_AT)((void *)GA(G_MAP_AREAS), pt); }

// ATTACK GROUPS AND SUBGROUPS (the originals of attack.c's and landing.c's hooks)
internal inline char unit_to_group(void *cmd, ee_unit *u) { return GF(FN_UNIT_TO_GROUP)(cmd, u); }
internal inline void group_stat(void *stats, ee_unit *u) { GF(FN_GROUP_STAT)(stats, u); }
// the units b..e sorted by distance to unit (the targets' range sort)
internal inline void range_sort(void **b, void **e, ee_unit *unit) { GF(FN_RANGE_SORT)(b, e, unit); }
internal inline void asg_update(ee_attack_subgroup *sg, int now) { GF(FN_ASG_UPDATE)(sg, now); }
internal inline void asg_sea_update(ee_attack_subgroup *sg, int now) { GF(FN_ASG_SEA_UPDATE)(sg, now); }
internal inline void asg_sea_transport_update(ee_attack_subgroup *sg, int now) { GF(FN_ASG_SEA_TRANSPORT_UPDATE)(sg, now); }
internal inline void asg_air_update(ee_attack_subgroup *sg, int now) { GF(FN_ASG_AIR_UPDATE)(sg, now); }
internal inline void asg_stats(ee_attack_subgroup *sg) { GF(FN_ASG_STATS)(sg); }
internal inline void asg_land_stats(ee_attack_subgroup *sg) { GF(FN_ASG_LAND_STATS)(sg); }
internal inline char asg_give_goal(ee_attack_subgroup *sg, ee_unit *u, ee_goal *goal, int a, int prio) { return GF(FN_ASG_GIVE_GOAL)(sg, u, goal, a, prio); }
internal inline char asg_air_give_goal(ee_attack_subgroup *sg, ee_unit *u, ee_goal *goal, int a, int prio) { return GF(FN_ASG_AIR_GIVE_GOAL)(sg, u, goal, a, prio); }
// a land subgroup's route from start to dest (int[2] each), its arrival (waypoints at +0x60)
internal inline void asg_route(ee_attack_subgroup *sg, int *start, int *dest) { ((void (__thiscall *)(ee_attack_subgroup *, int *, int *))GA(FN_ASG_ROUTE))(sg, start, dest); }
internal inline void asg_route_done(ee_attack_subgroup *sg) { ((void (__thiscall *)(ee_attack_subgroup *))GA(FN_ASG_ROUTE_DONE))(sg); }
// a subgroup member's score for an enemy (info: what it scores)
internal inline float asgu_score(ee_asg_unit *member, uint8_t *info) { return ((float (__thiscall *)(ee_asg_unit *, uint8_t *))GA(FN_ASGU_SCORE))(member, info); }
// the route planner's cost of an edge into node
internal inline int route_cost(void *planner, int *edge, uint8_t *node) {
    return ((int (__thiscall *)(void *, int *, uint8_t *))GA(FN_ROUTE_COST))(planner, edge, node);
}

// TECHS
internal inline ee_tech *tech_lookup(ee_tech_tree *tt, int id) { return (ee_tech *)GF(FN_TECH_LOOKUP)(tt, id); }
// can it be researched now (EETechTreeNode's vtable)
internal inline char tech_available(ee_tech *t) { return ((char (__thiscall *)(void *))(*(void ***)t)[GA(SLOT_TECH_AVAILABLE)])(t); }
// researched for nothing (EETechTreeNode::vf6) - the 2017 update's, AoC's takes an argument
internal inline int tech_grant(ee_tech *t) { return GF(FN_TECH_GRANT)(t); }
// the tech tree's next epoch (to the game's end epoch: it sets it and sends the epoch event)
internal inline void epoch_next(ee_tech_tree *tt) { GF(FN_EPOCH_NEXT)(tt); }
// the epoch's line upgrades granted (node: the epoch's tech, cat 7); Art of Conquest: only_new - only the ones with
// data +0x81 set (econ.c PAID UPGRADES)
internal inline void epoch_upgrades_grant(ee_tech *node, int cat, int only_new) {
    if (build_is("aoc")) ((void (__thiscall *)(void *, int, int))GA(FN_EPOCH_UPGRADES))(node, cat, only_new);
    else ((void (__thiscall *)(void *, int))GA(FN_EPOCH_UPGRADES))(node, cat);
}
