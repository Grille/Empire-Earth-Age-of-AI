// A PLAYER (ee_player: EEPlayer, EEComputerPlayer), the players' list and statistics: fields, functions, hooks.

// FIELDS (engine-notes.md "Players", the AI settings table's fields: ai/settings.c)
// +0x18 / +0x1C: a list of 4-byte entries - not known; the population check's log counts it
internal inline uint8_t *player_get_list_begin(const ee_player *pl) { return FIELD(pl, PL_LIST, uint8_t *); }
internal inline uint8_t *player_get_list_end(const ee_player *pl) { return FIELD(pl, PL_LIST + 0x4, uint8_t *); }
internal inline void *player_name_at(const ee_player *pl) { return (void *)FIELD_AT(pl, PL_NAME); }        // +0x24 its name (UWideString)
internal inline const char *player_get_name(const ee_player *pl) { return FIELD(pl, PL_NAME + 0x4, const char *); } // +0x28 the name's text
internal inline const void *player_color_at(const ee_player *pl) { return FIELD_AT(pl, PL_COLOR); }      // +0x438 its colour (RGB)
internal inline int player_get_index(const ee_player *pl) { return FIELD(pl, PL_INDEX, int); }          // +0x45C
internal inline int8_t player_get_msg_id(const ee_player *pl) { return FIELD(pl, PL_MSG_ID, int8_t); }  // +0x464 its id in a chat message (msg +0xA)
internal inline uint8_t player_get_explore_all(const ee_player *pl) { return FIELD(pl, PL_EXPLORE_ALL, uint8_t); } // +0x465 no fog
internal inline void player_set_explore_all(ee_player *pl, uint8_t v) { FIELD(pl, PL_EXPLORE_ALL, uint8_t) = v; }
internal inline uint8_t player_get_out(const ee_player *pl) { return FIELD(pl, PL_OUT, uint8_t); }    // +0x467 out of the game (resigned, defeated)
internal inline void *player_civ_name_at(const ee_player *pl) { return (void *)FIELD_AT(pl, PL_CIV_NAME); }  // +0x488 its civilization's name (UWideString)
internal inline uint8_t *player_stats_at(const ee_player *pl) { return (uint8_t *)FIELD_AT(pl, PL_STATS); } // +0x4A8 EEPlayerStatistics (PLSTATS)
internal inline int *player_get_selected_group(const ee_player *pl) { return FIELD(pl, PL_SELECTED_GROUP, int *); } // +0x9BC
internal inline ee_tech_tree *player_get_tech_tree(const ee_player *pl) { return FIELD(pl, PL_TECH_TREE, ee_tech_tree *); } // +0x9D4
// +0x9E0 whom its chat reaches: 0 everyone, 1 allies, 2 enemies, 3 the players marked at +0xB2C
internal inline int player_get_chat_to(const ee_player *pl) { return FIELD(pl, PL_CHAT_TO, int); }
internal inline void player_set_chat_to(ee_player *pl, int v) { FIELD(pl, PL_CHAT_TO, int) = v; }
internal inline int player_get_stance(const ee_player *pl, int i) { return FIELD(pl, PL_STANCE + i * 4, int); } // +0x9E4 [16] towards player i: 0 allied, 1 enemy
internal inline int player_get_strategy(const ee_player *pl) { return FIELD(pl, PL_STRATEGY, int); }        // +0xA28 orders the attention's activities
internal inline int player_get_result(const ee_player *pl) { return FIELD(pl, PL_RESULT, int); }         // +0xA30 1 won, 2 lost
internal inline int *player_stock_at(const ee_player *pl) { return (int *)FIELD_AT(pl, PL_STOCK); }     // +0xAF0 int[7]: 2 food, 3 wood, 4 stone, 5 gold, 6 iron
internal inline int player_get_stock(const ee_player *pl, int r) { return FIELD(pl, PL_STOCK + r * 4, int); }
// +0xB10 / +0xB1C / +0xB20: population counts - +0xB1C and +0xB20 in production (0x556E11), +0xB10 not known
internal inline int player_get_pop_b10(const ee_player *pl) { return FIELD(pl, PL_0xB10, int); }
internal inline int player_get_objects(const ee_player *pl) { return FIELD(pl, PL_OBJECTS, int); }        // +0xB14 its objects (< 1: out)
internal inline int player_get_pop_b1c(const ee_player *pl) { return FIELD(pl, PL_0xB1C, int); }
internal inline int player_get_pop_b20(const ee_player *pl) { return FIELD(pl, PL_0xB20, int); }
internal inline uint8_t player_get_chat_to_player(const ee_player *pl, int i) { return FIELD(pl, PL_CHAT_TO_PLAYER + i, uint8_t); } // +0xB2C [16]
internal inline void player_set_chat_to_player(ee_player *pl, int i, uint8_t v) { FIELD(pl, PL_CHAT_TO_PLAYER + i, uint8_t) = v; }
internal inline void player_set_civ_chosen(ee_player *pl, uint8_t v) { FIELD(pl, PL_CIV_CHOSEN, uint8_t) = v; } // +0xB46 (0: the civ dialog opens)
internal inline ee_zone **player_get_zones_begin(const ee_player *pl) { return FIELD(pl, PL_ZONES, ee_zone **); } // +0xB50 the resource zones (map-wide)
internal inline ee_zone **player_get_zones_end(const ee_player *pl) { return FIELD(pl, PL_ZONES + 0x4, ee_zone **); }
internal inline ee_combat_mgr *player_get_combat_mgr(const ee_player *pl) { return FIELD(pl, PL_COMBAT_MGR, ee_combat_mgr *); } // +0xB8C
internal inline ee_civ_mgr *player_get_civ_mgr(const ee_player *pl) { return FIELD(pl, PL_CIV_MGR, ee_civ_mgr *); } // +0xB90 a computer player's
internal inline uint8_t *player_get_b98(const ee_player *pl) { return FIELD(pl, PL_0xB98, uint8_t *); }    // +0xB98 (PLB98: not known)
internal inline int player_get_difficulty(const ee_player *pl) { return FIELD(pl, PL_DIFFICULTY, int); }      // +0xBA4 0 easy, 1 medium, 2 hard
internal inline void player_set_difficulty(ee_player *pl, int v) { FIELD(pl, PL_DIFFICULTY, int) = v; }
// the price percent with Cheat (AdjustCost 0x430FBD): easy +0xD48 (int), medium +0xBB4 (float), hard +0xD4C (int)
internal inline float player_get_cost_medium(const ee_player *pl) { return FIELD(pl, PL_COST_MEDIUM, float); }
internal inline void player_set_cost_medium(ee_player *pl, float v) { FIELD(pl, PL_COST_MEDIUM, float) = v; }
internal inline int player_get_cost_easy(const ee_player *pl) { return FIELD(pl, PL_COST_EASY, int); }
internal inline void player_set_cost_easy(ee_player *pl, int v) { FIELD(pl, PL_COST_EASY, int) = v; }
internal inline int player_get_cost_hard(const ee_player *pl) { return FIELD(pl, PL_COST_HARD, int); }
internal inline void player_set_cost_hard(ee_player *pl, int v) { FIELD(pl, PL_COST_HARD, int) = v; }
internal inline float player_get_citizen_share(const ee_player *pl) { return FIELD(pl, PL_CITIZEN_SHARE, float); } // +0xBDC of the cap (0.45)
internal inline void player_set_citizen_share(ee_player *pl, float v) { FIELD(pl, PL_CITIZEN_SHARE, float) = v; }
internal inline int player_get_pop_cap(const ee_player *pl) { return FIELD(pl, PL_POP_CAP, int); }         // +0xCCC its own cap (vf31)
internal inline void player_set_pop_cap(ee_player *pl, int v) { FIELD(pl, PL_POP_CAP, int) = v; }
// attacks (the AI settings table, 0x431697): chances in percent, minutes, the next attack's game time
internal inline void player_set_attack_on_contact(ee_player *pl, int v) { FIELD(pl, PL_ATTACK_ON_CONTACT, int) = v; }   // +0xCD8
internal inline void player_set_attack_for_resources(ee_player *pl, int v) { FIELD(pl, PL_ATTACK_FOR_RESOURCES, int) = v; } // +0xCDC
internal inline void player_set_attack_revenge(ee_player *pl, int v) { FIELD(pl, PL_ATTACK_REVENGE, int) = v; }      // +0xCE0
internal inline void player_set_attack_max(ee_player *pl, int v) { FIELD(pl, PL_ATTACK_MAX, int) = v; }          // +0xCFC between attacks
internal inline void player_set_attack_min(ee_player *pl, int v) { FIELD(pl, PL_ATTACK_MIN, int) = v; }          // +0xD00
internal inline void player_set_first_attack_max(ee_player *pl, int v) { FIELD(pl, PL_FIRST_ATTACK_MAX, int) = v; }    // +0xD28
internal inline void player_set_first_attack_min(ee_player *pl, int v) { FIELD(pl, PL_FIRST_ATTACK_MIN, int) = v; }    // +0xD2C
internal inline void player_set_epoch_min(ee_player *pl, int v) { FIELD(pl, PL_EPOCH_MIN, int) = v; }           // +0xD58 the epoch timer's minutes
internal inline void player_set_epoch_max(ee_player *pl, int v) { FIELD(pl, PL_EPOCH_MAX, int) = v; }           // +0xD5C
internal inline int player_get_dropoff_distance(const ee_player *pl) { return FIELD(pl, PL_DROPOFF_DISTANCE, int); }   // +0xE10
internal inline int player_get_attention(const ee_player *pl) { return FIELD(pl, PL_ATTENTION, int); }          // +0xE20 activities a period (+1)
internal inline uint32_t player_get_next_attack(const ee_player *pl) { return FIELD(pl, PL_NEXT_ATTACK, uint32_t); } // +0xED8 game time
internal inline void player_set_next_attack(ee_player *pl, uint32_t v) { FIELD(pl, PL_NEXT_ATTACK, uint32_t) = v; }
// +0xEF4.. the AI toggles: Angreifen EF4, Aktiv EF9, Cheat EFB, Bauen EFC, Erkunden EFD, Sammeln EFF, Verteidigen F00,
// Ignorieren F02 - and +0xEF5 first attack done, +0xEFA boats instead of fields (a water map with fish)
internal inline uint8_t player_get_toggle(const ee_player *pl, int i) { return FIELD(pl, PL_TOGGLES + i, uint8_t); }
internal inline uint8_t player_get_first_attack_done(const ee_player *pl) { return FIELD(pl, PL_FIRST_ATTACK_DONE, uint8_t); }
internal inline uint8_t player_get_boats_for_food(const ee_player *pl) { return FIELD(pl, PL_BOATS_FOR_FOOD, uint8_t); }
internal inline uint8_t player_get_cheat(const ee_player *pl) { return FIELD(pl, PL_CHEAT, uint8_t); }
internal inline void player_set_cheat(ee_player *pl, uint8_t v) { FIELD(pl, PL_CHEAT, uint8_t) = v; }
internal inline uint8_t player_get_build(const ee_player *pl) { return FIELD(pl, PL_BUILD, uint8_t); }
internal inline void player_set_ignore(ee_player *pl, uint8_t v) { FIELD(pl, PL_IGNORE, uint8_t) = v; }
// a player's statistics (player_stats_at): series k {?, begin, end, ..} of 8-byte samples
internal inline uint8_t *plstats_get_begin(const void *st, int k) { return FIELD(st, PLSTATS_LISTS + k * 0x18, uint8_t *); }
internal inline uint8_t *plstats_get_end(const void *st, int k) { return FIELD(st, PLSTATS_LISTS + 0x4 + k * 0x18, uint8_t *); }
// the object at a player's +0xB98 (not known): its +0x2D the attention's activity 7
internal inline uint8_t b98_get_att(const uint8_t *b98) { return b98[PLB98_ATT]; }

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
internal inline unsigned type_stats_count(ee_player *pl) { return GF(FN_VECTOR_SIZE)(FIELD_AT(pl, PL_TYPE_STATS)); }
internal inline ee_unit_stats **type_stats(ee_player *pl, int type) { return (ee_unit_stats **)GF(FN_TYPE_STATS)(FIELD_AT(pl, PL_TYPE_STATS), type); }
// the player resigns (how: 2 as the resign message does)
internal inline void player_resign(ee_player *pl, int how) { GF(FN_PLAYER_RESIGN)(pl, how); }
// player a's stance towards b (1 enemy, 0 ally) set as the game does (EEPlayer's diplomacy, both its lists)
internal inline void set_diplomacy(ee_player *a, ee_player *b, int stance, int flag) {
    ((void (__thiscall *)(void *, void *, int, int))GA(FN_SET_DIPLOMACY))(a, b, stance, flag);
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
internal inline void cp_update(ee_player *pl, uint32_t now) { ((void (__thiscall *)(ee_player *, uint32_t))GA(FN_CP_UPDATE))(pl, now); }
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
typedef void (__fastcall *cp_update_hook)(ee_player *pl, void *edx, uint32_t now);
internal inline int patch_cp_update(cp_update_hook h) { return patch_call(GA(CALL_CP_UPDATE), GA(FN_CP_UPDATE), (void *)h); }
typedef void (__fastcall *cp_pay_hook)(ee_player *pl, void *edx, void *what, int b, int c);
internal inline int patch_cp_pay(cp_pay_hook h) { return patch_vtable(GA(VT_COMPUTER_PLAYER), GA(SLOT_CP_PAY), GA(FN_CP_PAY), (void *)h); }
typedef void (__fastcall *cp_add_resource_hook)(ee_player *pl, void *edx, int type, int amount);
internal inline int patch_cp_add_resource(cp_add_resource_hook h) {
    return patch_vtable(GA(VT_COMPUTER_PLAYER), GA(SLOT_CP_ADD_RESOURCE), GA(FN_CP_ADD_RESOURCE), (void *)h);
}
typedef void (__fastcall *pop_cap_hook)(ee_player *pl, void *edx);
internal inline int patch_pop_cap(pop_cap_hook h, int *of) {
    const uint32_t sites[3] = { GA(CALL_POP_CAP_1), GA(CALL_POP_CAP_2), GA(CALL_POP_CAP_3) };
    return patch_calls(sites, 3, GA(FN_POP_CAP), (void *)h, of);
}
typedef void (__fastcall *epoch_catch_up_hook)(ee_player *ai, void *edx, ee_player *human);
internal inline int patch_epoch_catch_up(epoch_catch_up_hook h) { return patch_call(GA(CALL_EPOCH_CATCH_UP), GA(FN_EPOCH_CATCH_UP), (void *)h); }
// the maintenance's deletion of units for room (the navy keeps its ships: fleet.c)
typedef int (__fastcall *delete_for_room_hook)(ee_player *pl, void *edx, void *req, int flag);
internal inline int patch_maint_delete(delete_for_room_hook h) { return patch_call(GA(CALL_NV_MAINT_DELETE), GA(FN_DELETE_FOR_ROOM), (void *)h); }
