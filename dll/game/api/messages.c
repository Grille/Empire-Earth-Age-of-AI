// THE MESSAGES (the commands players send each other) AND THE LOCKSTEP SERVER: fields, functions, hooks.

// FIELDS
internal inline LONG server_get_waiting(const ee_server *srv) { return FIELD(srv, of_server_waiting, LONG); }     // +0x94 messages waiting
// +0x1B4 / +0x1B8 the steps a second at most / at least, +0x1FB its current pace

// MESSAGES (the commands players send each other through the server): the sender's message id at +0xA (every
// message's, the chat message's row)
internal inline int8_t msg_get_from(const ee_msg *msg) { return (int8_t)FIELD(msg, of_chat_msg_from, uint8_t); }
// a tribute (EEMPayTribute): food +0x10, gold +0x14, iron +0x18, stone +0x1C, wood +0x20; to player +0x24
// a chat line (EEMInGameChat): its text, a UString at +0x10 (the characters at its +4)
internal inline const char *chat_msg_get_text(const ee_msg *msg) { return FIELD(FIELD_AT(msg, of_chat_msg_text), 4, const char *); }
// a selection (EEMSelectUnits): the selection +0x10 (player_select), its unit ids +0x14 .. +0x18, +0x134 a flag
internal inline uint32_t *select_msg_get_ids_end(const ee_msg *msg) { return FIELD(msg, of_select_msg_ids_begin + 0x4, uint32_t *); }
// a load into a transport (EEMLoadTransport): the transport's id +0x10, a unit's +0x14 (-1: the player's selection)
// an unload (EEMUnloadTransport): the tile clicked +0x14, +0x18 (the selected transports' unload point)
// the two made by hand in zeroed memory, the fields their Executes read (transports.c's test: executed, never sent; from: the sender's player_get_msg_id)
internal inline void load_msg_make(ee_msg *msg, int from, unsigned transport, int unit) {
    FIELD(msg, of_chat_msg_from, uint8_t) = (uint8_t)from;
    FIELD(msg, of_load_msg_transport, unsigned) = transport;
    FIELD(msg, of_load_msg_unit, int) = unit;
}
internal inline void unload_msg_make(ee_msg *msg, int from, int x, int y) {
    FIELD(msg, of_chat_msg_from, uint8_t) = (uint8_t)from;
    FIELD(msg, of_unload_msg_x, int) = x;
    FIELD(msg, of_unload_msg_y, int) = y;
}
// a move (EEMPathTo, the right-click's; vtable 0x824D84, 0x20 bytes): the target +0x14, +0x18 (floats), +0x10 the
// facing of a drag, +0x1C / +0x1D the move goal's +0x30 / +0x31 (1, 1 for a plain click), +0x1E queued (shift),
// +0x1F together: the goal's +0x33, with it the group (EEActiveGroup +0xB6, set by its hand-out 0x5DD436) moves its
// members at the group's speed (+0x70: their average, at most the slowest one's x the settings table's 0x31) instead
// of each its type's - a plain click sends 0. The Execute splits the selection into a group per kind and hands each
// the move.
internal inline void path_msg_make(ee_msg *msg, int from, float x, float y, int together, int queued) {
    FIELD(msg, of_chat_msg_from, uint8_t) = (uint8_t)from;
    FIELD(msg, of_path_msg_x, float) = x;
    FIELD(msg, of_path_msg_y, float) = y;
    FIELD(msg, of_path_msg_goal_30, uint8_t) = 1;
    FIELD(msg, of_path_msg_goal_31, uint8_t) = 1;
    FIELD(msg, of_path_msg_queued, uint8_t) = (uint8_t)queued;
    FIELD(msg, of_path_msg_together, uint8_t) = (uint8_t)together;
}
// the game start (EEMStartLoading): +0x14 the game's seed

// FUNCTIONS
// the network messages: executed (the originals of diplo.c's hooks), a chat line built in mem, one sent to everyone
internal inline void msg_resign(ee_msg *msg) { GF(FN_RESIGN)(msg); }
internal inline void msg_pay_tribute(ee_msg *msg) { GF(FN_PAY_TRIBUTE)(msg); }
internal inline void msg_ingame_chat(ee_msg *msg) { GF(FN_INGAME_CHAT)(msg); }
internal inline void msg_load(ee_msg *msg) { GF(FN_MSG_LOAD)(msg); }
internal inline void msg_unload(ee_msg *msg) { GF(FN_MSG_UNLOAD)(msg); }
internal inline void msg_path(ee_msg *msg) { GF(FN_MSG_PATH)(msg); }
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
typedef void (__thiscall *server_send_fn)(ee_server *net, ee_msg *msg, int to);
internal inline server_send_fn patch_server_send(ee_server *net, server_send_fn h) {
    void **slot = &((void **)*(void ***)net)[GA(SLOT_SERVER_SEND)];
    DWORD old;
    if (!VirtualProtect(slot, 4, PAGE_READWRITE, &old)) return NULL;
    server_send_fn prev = (server_send_fn)*slot;
    *slot = (void *)h;
    VirtualProtect(slot, 4, old, &old);
    return prev;
}

// MESSAGES AND THE MENU (diplo.c)
typedef void (__thiscall *msg_hook)(ee_msg *msg);
internal inline int patch_msg_pay_tribute(msg_hook h) { return patch_vtable(GA(VT_PAY_TRIBUTE), GA(SLOT_MSG_EXECUTE), GA(FN_PAY_TRIBUTE), (void *)h); }
internal inline int patch_msg_ingame_chat(msg_hook h) { return patch_vtable(GA(VT_INGAME_CHAT), GA(SLOT_MSG_EXECUTE), GA(FN_INGAME_CHAT), (void *)h); }
internal inline int patch_msg_resign(msg_hook h) { return patch_vtable(GA(VT_RESIGN), GA(SLOT_MSG_EXECUTE), GA(FN_RESIGN), (void *)h); }
// the load and unload commands (transports.c)
internal inline int patch_msg_load(msg_hook h) { return patch_vtable(GA(VT_MSG_LOAD), GA(SLOT_MSG_EXECUTE), GA(FN_MSG_LOAD), (void *)h); }
internal inline int patch_msg_unload(msg_hook h) { return patch_vtable(GA(VT_MSG_UNLOAD), GA(SLOT_MSG_EXECUTE), GA(FN_MSG_UNLOAD), (void *)h); }
// the game start message's execution (its vtable slot 10: it seeds the simulation's generator)
typedef void (__thiscall *start_exec_hook)(ee_msg *msg);
internal inline int patch_start_exec(start_exec_hook h) { return patch_code32(GA(START_VTBL_EXEC), GA(START_EXEC), (uint32_t)(uintptr_t)h); }
