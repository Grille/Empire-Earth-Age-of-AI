// A PLAYER (ee_player: EEPlayer, EEComputerPlayer), the players' list and statistics: fields, functions, hooks.

// FIELDS (engine-notes.md "Players", the AI settings table's fields: ai/settings.c)
// +0x18 / +0x1C: a list of 4-byte entries - not known; the population check's log counts it
internal inline uint8_t *player_get_list_end(const ee_player *pl) { return FIELD(pl, of_player_list_begin + 0x4, uint8_t *); }
internal inline const char *player_get_name(const ee_player *pl) { return FIELD(pl, of_player_name + 0x4, const char *); } // +0x28 the name's text
// +0x9E0 whom its chat reaches: 0 everyone, 1 allies, 2 enemies, 3 the players marked at +0xB2C
internal inline int player_get_stance(const ee_player *pl, int i) { return FIELD(pl, of_player_stance + i * 4, int); } // +0x9E4 [16] towards player i: 0 allied, 1 enemy
// +0xB10 / +0xB1C / +0xB20: population counts - +0xB1C and +0xB20 in production (0x556E11), +0xB10 not known
internal inline uint8_t player_get_chat_to_player(const ee_player *pl, int i) { return FIELD(pl, of_player_chat_to_player + i, uint8_t); } // +0xB2C [16]
internal inline void player_set_chat_to_player(ee_player *pl, int i, uint8_t v) { FIELD(pl, of_player_chat_to_player + i, uint8_t) = v; }
internal inline ee_zone **player_get_zones_end(const ee_player *pl) { return FIELD(pl, of_player_zones_begin + 0x4, ee_zone **); }
// the price percent with Cheat (AdjustCost 0x430FBD): easy +0xD48 (int), medium +0xBB4 (float), hard +0xD4C (int)
// attacks (the AI settings table, 0x431697): chances in percent, minutes, the next attack's game time
// +0xEF4.. the AI toggles: Angreifen EF4, Aktiv EF9, Cheat EFB, Bauen EFC, Erkunden EFD, Sammeln EFF, Verteidigen F00,
// Ignorieren F02 - and +0xEF5 the first update done (0x43F91A), +0xF05 the first attack done, +0xEFA boats instead of fields (a water map with fish)
internal inline uint8_t player_get_toggle(const ee_player *pl, int i) { return FIELD(pl, of_player_toggles + i, uint8_t); }
// a player's statistics (player_stats_at): series k {?, begin, end, ..} of 8-byte samples
internal inline uint8_t *plstats_get_begin(const ee_player_stats *st, int k) { return FIELD(st, of_plstats_lists + k * 0x18, uint8_t *); }
internal inline uint8_t *plstats_get_end(const ee_player_stats *st, int k) { return FIELD(st, of_plstats_lists + 0x4 + k * 0x18, uint8_t *); }
// the object at a player's +0xB98 (not known): its +0x2D the attention's activity 7

// FUNCTIONS
// a statistics counter n of the stats object +d (the kill counter's original: diplo.c)

// the population: player +0xB10, with_list: and the units of its +0x18 list, with_aboard: and the units inside
// transports (type family 0x1D) - (1, 0) is what the AI's unit manager counts
// the game's unit limit (EEPlayer::vf31(0) called directly: a computer player's own vf31 returns its cap, +0xCCC)
internal inline int game_unit_limit(ee_player *pl) { return fn_game_limit(pl, 0); }
// can it build or train type now: available, affordable, a builder (0x20: as the AI's own checks ask)
internal inline char can_build(ee_player *pl, int type) { return fn_can_build_game(pl, type, 0x20, 0); }
// a foundation of type laid out at x, y (the player pays): the new building, NULL: none
// can a building of type stand at x, y (size x size tiles: the game's placement test, every tile)
internal inline char can_place_area(ee_player *pl, int type, int x, int y, int size) { return fn_can_place_game(pl, type, x, y, size, size, 0); }
// the player's per-type stats records (player +0x874, a vector): their count, the record of type
internal inline unsigned type_stats_count(ee_player *pl) { return fn_vector_size(FIELD_AT(pl, of_player_type_stats)); }
internal inline ee_unit_stats **type_stats(ee_player *pl, int type) { return (ee_unit_stats **)fn_type_stats_game(FIELD_AT(pl, of_player_type_stats), type); }
// the player resigns (how: 2 as the resign message does)
// player a's stance towards b (1 enemy, 0 ally) set as the game does (EEPlayer's diplomacy, both its lists)
internal inline void set_diplomacy(ee_player *a, ee_player *b, int stance, int flag) {
    fn_set_diplomacy_game(a, b, stance, flag);
}
// the player's attacks on target called off (the units back to the pool)
// units of the player deleted to make room for req (the original of fleet.c's hook)
// the game's unit limit for the player (EEPlayer::vf31(0) through the player's own vtable, as game_limit; it pops
// its argument)
internal inline int unit_limit(ee_player *pl) { return ((int (__thiscall *)(void *, int))(*(void ***)pl)[value_player_slot_unit_limit])(pl, 0); }
// does the game count the player as human (EEHumanPlayer's answers 1; highest_human_epoch's test)
internal inline char player_is_human(ee_player *pl) { return ((char (__thiscall *)(void *))(*(void ***)pl)[value_player_slot_player_human])(pl); }

// COMPUTER PLAYERS (the originals of econ.c's hooks)
// the computer player's update (EEComputerPlayer::Update - the original of engine/scenario_ai.c's hook)
// the next attack's time drawn (player +0xED8), a price as the cheat makes it (AdjustCost, vf27)
// the population cap that follows the humans, set every minute (player +0xCCC)
// the game's catch-up: ai's next epoch attempt redrawn soon when human is ahead

// HOOKS
// COMPUTER PLAYERS (econ.c)
// the computer player's update (engine/scenario_ai.c: the scenario flag cleared meanwhile)
typedef void (__thiscall *cp_update_hook)(ee_player *pl, uint32_t now);
internal inline int patch_cp_update(cp_update_hook h) { return patch_call(data_player_call_cp_update, (uintptr_t)fn_cp_update, (void *)h); }
typedef void (__thiscall *cp_pay_hook)(ee_player *pl, void *what, int b, int c);
internal inline int patch_cp_pay(cp_pay_hook h) { return patch_vtable(data_player_vt_computer_player, value_player_slot_cp_pay, (uintptr_t)fn_cp_pay, (void *)h); }
typedef void (__thiscall *cp_add_resource_hook)(ee_player *pl, int type, int amount);
internal inline int patch_cp_add_resource(cp_add_resource_hook h) {
    return patch_vtable(data_player_vt_computer_player, value_player_slot_cp_add_resource, (uintptr_t)fn_cp_add_resource, (void *)h);
}
typedef void (__thiscall *pop_cap_hook)(ee_player *pl);
internal inline int patch_pop_cap(pop_cap_hook h, int *of) {
    const uint32_t sites[3] = { data_player_call_pop_cap_1, data_player_call_pop_cap_2, data_player_call_pop_cap_3 };
    return patch_calls(sites, 3, (uintptr_t)fn_cp_update_pop_cap, (void *)h, of);
}
typedef void (__thiscall *epoch_catch_up_hook)(ee_player *ai, ee_player *human);
internal inline int patch_epoch_catch_up(epoch_catch_up_hook h) { return patch_call(data_player_call_epoch_catch_up, (uintptr_t)fn_cp_epoch_catch_up, (void *)h); }
// the maintenance's deletion of units for room (the navy keeps its ships: fleet.c)
typedef int (__thiscall *delete_for_room_hook)(ee_player *pl, void *req, int flag);
internal inline int patch_maint_delete(delete_for_room_hook h) { return patch_call(data_player_call_nv_maint_delete, (uintptr_t)fn_delete_for_room, (void *)h); }
// a kill counted in the statistics (diplo.c - a thunk)
internal inline int patch_kill_stat(thunk_fn thunk) { return patch_call(data_player_call_kill_stat, (uintptr_t)fn_stat_add, (void *)thunk); }
