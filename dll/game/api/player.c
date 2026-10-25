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
internal inline void stat_add(void *stats, int n, int d) { GF(FN_STAT_ADD)(stats, n, d); }
internal inline void player_select(ee_player *pl, void *selection) { GF(FN_SELECT_UNITS)(pl, selection); }

// the population: player +0xB10, with_list: and the units of its +0x18 list, with_aboard: and the units inside
// transports (type family 0x1D) - (1, 0) is what the AI's unit manager counts
internal inline int population(ee_player *pl, int with_list, int with_aboard) { return GF(FN_POPULATION)(pl, with_list, with_aboard); }
// the game's unit limit (EEPlayer::vf31(0) called directly: a computer player's own vf31 returns its cap, +0xCCC)
internal inline int game_unit_limit(ee_player *pl) { return GF(FN_GAME_LIMIT)(pl, 0); }
// can it build or train type now: available, affordable, a builder (0x20: as the AI's own checks ask)
internal inline char can_build(ee_player *pl, int type) { return GF(FN_CAN_BUILD)(pl, type, 0x20, 0); }
// a foundation of type laid out at x, y (the player pays): the new building, NULL: none
internal inline ee_unit *place_foundation(ee_player *pl, int type, int x, int y) { return GF(FN_PLACE_BUILDING)(pl, type, x, y); }
// can a building of type stand at x, y (size x size tiles: the game's placement test, every tile)
internal inline char can_place_area(ee_player *pl, int type, int x, int y, int size) { return GF(FN_CAN_PLACE)(pl, type, x, y, size, size, 0); }
// the player's per-type stats records (player +0x874, a vector): their count, the record of type
internal inline unsigned type_stats_count(ee_player *pl) { return GF(FN_VECTOR_SIZE)(FIELD_AT(pl, of_player_type_stats)); }
internal inline ee_unit_stats **type_stats(ee_player *pl, int type) { return (ee_unit_stats **)GF(FN_TYPE_STATS)(FIELD_AT(pl, of_player_type_stats), type); }
// the player resigns (how: 2 as the resign message does)
internal inline void player_resign(ee_player *pl, int how) { GF(FN_PLAYER_RESIGN)(pl, how); }
// player a's stance towards b (1 enemy, 0 ally) set as the game does (EEPlayer's diplomacy, both its lists)
internal inline void set_diplomacy(ee_player *a, ee_player *b, int stance, int flag) {
    GF(FN_SET_DIPLOMACY)(a, b, stance, flag);
}
// the player's attacks on target called off (the units back to the pool)
internal inline void call_off_attacks(ee_player *pl, ee_player *target, int flag) { GF(FN_CALL_OFF_ATTACK)(pl, target, flag); }
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
internal inline void cp_update(ee_player *pl, uint32_t now) { GF(FN_CP_UPDATE)(pl, now); }
// the next attack's time drawn (player +0xED8), a price as the cheat makes it (AdjustCost, vf27)
internal inline void cp_schedule_attack(ee_player *pl, uint32_t now) { GF(FN_SCHEDULE_ATTACK)(pl, now); }
internal inline int cp_adjust_cost(ee_player *pl, int price) { return GF(FN_ADJUST_COST)(pl, price); }
// the population cap that follows the humans, set every minute (player +0xCCC)
internal inline void cp_update_pop_cap(ee_player *pl) { GF(FN_POP_CAP)(pl); }
// the game's catch-up: ai's next epoch attempt redrawn soon when human is ahead
internal inline void cp_epoch_catch_up(ee_player *ai, ee_player *human) { GF(FN_EPOCH_CATCH_UP)(ai, human); }

// HOOKS
// COMPUTER PLAYERS (econ.c)
// the computer player's update (engine/scenario_ai.c: the scenario flag cleared meanwhile)
typedef void (__thiscall *cp_update_hook)(ee_player *pl, uint32_t now);
internal inline int patch_cp_update(cp_update_hook h) { return patch_call(GA(CALL_CP_UPDATE), GA(FN_CP_UPDATE), (void *)h); }
typedef void (__thiscall *cp_pay_hook)(ee_player *pl, void *what, int b, int c);
internal inline int patch_cp_pay(cp_pay_hook h) { return patch_vtable(GA(VT_COMPUTER_PLAYER), GA(SLOT_CP_PAY), GA(FN_CP_PAY), (void *)h); }
typedef void (__thiscall *cp_add_resource_hook)(ee_player *pl, int type, int amount);
internal inline int patch_cp_add_resource(cp_add_resource_hook h) {
    return patch_vtable(GA(VT_COMPUTER_PLAYER), GA(SLOT_CP_ADD_RESOURCE), GA(FN_CP_ADD_RESOURCE), (void *)h);
}
typedef void (__thiscall *pop_cap_hook)(ee_player *pl);
internal inline int patch_pop_cap(pop_cap_hook h, int *of) {
    const uint32_t sites[3] = { GA(CALL_POP_CAP_1), GA(CALL_POP_CAP_2), GA(CALL_POP_CAP_3) };
    return patch_calls(sites, 3, GA(FN_POP_CAP), (void *)h, of);
}
typedef void (__thiscall *epoch_catch_up_hook)(ee_player *ai, ee_player *human);
internal inline int patch_epoch_catch_up(epoch_catch_up_hook h) { return patch_call(GA(CALL_EPOCH_CATCH_UP), GA(FN_EPOCH_CATCH_UP), (void *)h); }
// the maintenance's deletion of units for room (the navy keeps its ships: fleet.c)
typedef int (__thiscall *delete_for_room_hook)(ee_player *pl, void *req, int flag);
internal inline int patch_maint_delete(delete_for_room_hook h) { return patch_call(GA(CALL_NV_MAINT_DELETE), GA(FN_DELETE_FOR_ROOM), (void *)h); }
// a kill counted in the statistics (diplo.c - a thunk)
internal inline int patch_kill_stat(thunk_fn thunk) { return patch_call(GA(CALL_KILL_STAT), GA(FN_STAT_ADD), (void *)thunk); }
