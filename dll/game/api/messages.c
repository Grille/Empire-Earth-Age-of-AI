// THE MESSAGES (the commands players send each other) AND THE LOCKSTEP SERVER: fields, functions, hooks.

// FIELDS
internal inline unsigned server_get_sent(const ee_server *srv) { return FIELD(srv, SERVER_SENT, unsigned); }  // +0x10 / +0x190 sent / done
internal inline unsigned server_get_done(const ee_server *srv) { return FIELD(srv, SERVER_DONE, unsigned); }
internal inline LONG server_get_waiting(const ee_server *srv) { return FIELD(srv, SERVER_WAITING, LONG); }     // +0x94 messages waiting
// +0x1B4 / +0x1B8 the steps a second at most / at least, +0x1FB its current pace
internal inline unsigned server_get_rate_max(const ee_server *srv) { return FIELD(srv, SERVER_RATE_MAX, unsigned); }
internal inline void server_set_rate_max(ee_server *srv, unsigned v) { FIELD(srv, SERVER_RATE_MAX, unsigned) = v; }
internal inline unsigned server_get_rate_min(const ee_server *srv) { return FIELD(srv, SERVER_RATE_MIN, unsigned); }
internal inline void server_set_rate_min(ee_server *srv, unsigned v) { FIELD(srv, SERVER_RATE_MIN, unsigned) = v; }
internal inline uint8_t server_get_rate(const ee_server *srv) { return FIELD(srv, SERVER_RATE, uint8_t); }

// MESSAGES (the commands players send each other through the server): the sender's message id at +0xA (every
// message's, the chat message's row)
internal inline int8_t msg_get_from(const ee_msg *msg) { return (int8_t)FIELD(msg, MSG_CHAT_FROM, uint8_t); }
// a tribute (EEMPayTribute): food +0x10, gold +0x14, iron +0x18, stone +0x1C, wood +0x20; to player +0x24
internal inline int tribute_get_food(const ee_msg *msg) { return FIELD(msg, MSG_TRIBUTE_FOOD, int); }
internal inline int tribute_get_gold(const ee_msg *msg) { return FIELD(msg, MSG_TRIBUTE_GOLD, int); }
internal inline int tribute_get_iron(const ee_msg *msg) { return FIELD(msg, MSG_TRIBUTE_IRON, int); }
internal inline int tribute_get_stone(const ee_msg *msg) { return FIELD(msg, MSG_TRIBUTE_STONE, int); }
internal inline int tribute_get_wood(const ee_msg *msg) { return FIELD(msg, MSG_TRIBUTE_WOOD, int); }
internal inline uint8_t tribute_get_to(const ee_msg *msg) { return FIELD(msg, MSG_TRIBUTE_TO, uint8_t); }
// a chat line (EEMInGameChat): its text, a UString at +0x10 (the characters at its +4)
internal inline const char *chat_msg_get_text(const ee_msg *msg) { return FIELD(FIELD_AT(msg, MSG_CHAT_TEXT), 4, const char *); }
// a selection (EEMSelectUnits): the selection +0x10 (player_select), its unit ids +0x14 .. +0x18, +0x134 a flag
internal inline uint8_t *select_msg_selection_at(const ee_msg *msg) { return (uint8_t *)FIELD_AT(msg, MSG_SELECT_SELECTION); }
internal inline uint32_t *select_msg_get_ids_begin(const ee_msg *msg) { return FIELD(msg, MSG_SELECT_IDS, uint32_t *); }
internal inline uint32_t *select_msg_get_ids_end(const ee_msg *msg) { return FIELD(msg, MSG_SELECT_IDS + 0x4, uint32_t *); }
internal inline uint8_t select_msg_get_flag(const ee_msg *msg) { return FIELD(msg, MSG_SELECT_FLAG, uint8_t); }
// the game start (EEMStartLoading): +0x14 the game's seed
internal inline unsigned start_msg_get_seed(const ee_msg *msg) { return FIELD(msg, MSG_START_SEED, unsigned); }
internal inline void start_msg_set_seed(ee_msg *msg, unsigned v) { FIELD(msg, MSG_START_SEED, unsigned) = v; }

// FUNCTIONS
// the network messages: executed (the originals of diplo.c's hooks), a chat line built in mem, one sent to everyone
internal inline void msg_resign(void *msg) { GF(FN_RESIGN)(msg); }
internal inline void msg_pay_tribute(void *msg) { GF(FN_PAY_TRIBUTE)(msg); }
internal inline void msg_ingame_chat(void *msg) { GF(FN_INGAME_CHAT)(msg); }
internal inline void *msg_chat_init(void *mem, void *wide_text) { return GF(FN_CHAT_MSG_CTOR)(mem, wide_text); }
internal inline void server_send_all(ee_server *net, void *msg) {
    ((void (__thiscall *)(void *, void *, int))(*(void ***)net)[GA(SLOT_SERVER_SEND)])(net, msg, 0xFF);
}
// a cheat message built in mem (game_new(GA(SIZE_CHEAT_MSG)))
internal inline void *msg_cheat_init(void *mem, int code, int arg) { return GF(FN_CHEAT_MSG_CTOR)(mem, code, arg); }
internal inline void *msg_chat_alloc(void) { return game_new(GA(SIZE_MSG_CHAT)); }
internal inline void *msg_cheat_alloc(void) { return game_new(GA(SIZE_CHEAT_MSG)); }
// the selection: its unit ids packed into a message (the original of select.c's hook), a double-click's message built,
// carried out on the player, the selected group's count (the group's vtable)
internal inline unsigned sel_pack(uint8_t *list, uint32_t player_byte, uint8_t *out) { return GF(FN_SEL_PACK)(list, player_byte, out); }
internal inline void select_msg_init(void *msg, float *rect, int type, int flags) { GF(FN_SELECT_MSG_INIT)(msg, rect, type, flags); }

// HOOKS
// the selection's packing into a message (select.c)
typedef unsigned (__cdecl *sel_pack_hook)(uint8_t *list, uint32_t player_byte, uint8_t *out);
internal inline int patch_sel_pack(sel_pack_hook h) { return patch_call(GA(CALL_SEL_PACK), GA(FN_SEL_PACK), (void *)h); }
// the lockstep server's send (its vtable slot 6, on the live object: EEServer or EEClient) - the slot's previous
// function, NULL when not patched; two hooks chain (the second calls the first)
typedef void (__thiscall *server_send_fn)(void *net, void *msg, int to);

// MESSAGES AND THE MENU (diplo.c)
typedef void (__thiscall *msg_hook)(void *msg);
internal inline int patch_msg_pay_tribute(msg_hook h) { return patch_vtable(GA(VT_PAY_TRIBUTE), GA(SLOT_MSG_EXECUTE), GA(FN_PAY_TRIBUTE), (void *)h); }
internal inline int patch_msg_ingame_chat(msg_hook h) { return patch_vtable(GA(VT_INGAME_CHAT), GA(SLOT_MSG_EXECUTE), GA(FN_INGAME_CHAT), (void *)h); }
internal inline int patch_msg_resign(msg_hook h) { return patch_vtable(GA(VT_RESIGN), GA(SLOT_MSG_EXECUTE), GA(FN_RESIGN), (void *)h); }
