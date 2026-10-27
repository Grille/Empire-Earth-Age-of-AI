// THE MESSAGES (the commands players send each other) AND THE LOCKSTEP SERVER: fields, functions, hooks.

// FIELDS
internal inline LONG server_get_waiting(const ee_server *srv) { return FIELD(srv, of_server_waiting, LONG); }     // +0x94 messages waiting
// +0x1B4 / +0x1B8 the steps a second at most / at least, +0x1FB its current pace

// MESSAGES (the commands players send each other through the server): the sender's message id at +0xA (every
// message's: EEMessage's from, under each message's own name - load_msg_get_from)
// a tribute (EEMPayTribute): food +0x10, gold +0x14, iron +0x18, stone +0x1C, wood +0x20; to player +0x24
// a chat line (EEMInGameChat): its text, a UString at +0x10 (the characters at its +4)
internal inline const char *chat_msg_get_text(const ee_chat_msg *msg) { return FIELD(FIELD_AT(msg, of_chat_msg_text), 4, const char *); }
// a selection (EEMSelectUnits): the selection +0x10 (player_select), its unit ids +0x14 .. +0x18, +0x134 a flag
internal inline uint32_t *select_msg_get_ids_end(const ee_select_msg *msg) { return FIELD(msg, of_select_msg_ids_begin + 0x4, uint32_t *); }
// a load into a transport (EEMLoadTransport): the transport's id +0x10, a unit's +0x14 (-1: the player's selection)
// an unload (EEMUnloadTransport): the tile clicked +0x14, +0x18 (the selected transports' unload point)
// the two made by hand in zeroed memory, the fields their Executes read (transports.c's test: executed, never sent; from: the sender's player_get_msg_id)
internal inline void load_msg_make(ee_load_msg *msg, int from, unsigned transport, int unit) {
    load_msg_set_from(msg, (int8_t)from);
    FIELD(msg, of_load_msg_transport, unsigned) = transport;
    FIELD(msg, of_load_msg_unit, int) = unit;
}
internal inline void unload_msg_make(ee_unload_msg *msg, int from, int x, int y) {
    unload_msg_set_from(msg, (int8_t)from);
    FIELD(msg, of_unload_msg_x, int) = x;
    FIELD(msg, of_unload_msg_y, int) = y;
}
// a move (EEMPathTo, the right-click's; vtable 0x824D84, 0x20 bytes): the target +0x14, +0x18 (floats), +0x10 the
// facing of a drag, +0x1C / +0x1D the move goal's +0x30 / +0x31 (1, 1 for a plain click), +0x1E queued (shift),
// +0x1F together: the goal's +0x33, with it the group (EEActiveGroup +0xB6, set by its hand-out 0x5DD436) moves its
// members at the group's speed (+0x70: their average, at most the slowest one's x the settings table's 0x31) instead
// of each its type's - a plain click sends 0. The Execute splits the selection into a group per kind and hands each
// the move.
internal inline void path_msg_make(ee_path_msg *msg, int from, float x, float y, int together, int queued) {
    *(uintptr_t *)msg = vt_eem_path_to;   // carried out through it (msg_execute) as the game does: hooks of its slot see it
    path_msg_set_from(msg, (int8_t)from);
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
internal inline void server_send_all(ee_server *net, void *msg) {
    ((void (__thiscall *)(void *, void *, int))(*(void ***)net)[value_messages_slot_server_send])(net, msg, 0xFF);
}
// a cheat message built in mem (game_new(value_messages_size_cheat_msg))
internal inline void *msg_chat_alloc(void) { return game_new(value_messages_size_msg_chat); }
internal inline void *msg_cheat_alloc(void) { return game_new(value_messages_size_cheat_msg); }
// the selection: its unit ids packed into a message (the original of select.c's hook), a double-click's message built,
// carried out on the player, the selected group's count (the group's vtable)

// HOOKS
// the selection's packing into a message (select.c)
// the lockstep server's send (its vtable slot 6, on the live object: EEServer or EEClient) - the slot's previous
// function, NULL when not patched; two hooks chain (the second calls the first)
typedef void (__thiscall *server_send_fn)(ee_server *net, ee_msg *msg, int to);
internal inline server_send_fn patch_server_send(ee_server *net, server_send_fn h) {
    void **slot = &((void **)*(void ***)net)[value_messages_slot_server_send];
    DWORD old;
    if (!VirtualProtect(slot, 4, PAGE_READWRITE, &old)) return NULL;
    server_send_fn prev = (server_send_fn)*slot;
    *slot = (void *)h;
    VirtualProtect(slot, 4, old, &old);
    return prev;
}

// MESSAGES AND THE MENU (diplo.c)
internal inline int patch_msg_pay_tribute(tribute_msg_execute_fn h) { return patch_vtable(data_messages_vt_pay_tribute, value_messages_slot_msg_execute, (uintptr_t)fn_tribute_msg_execute, (void *)h); }
internal inline int patch_msg_ingame_chat(chat_msg_execute_fn h) { return patch_vtable(data_messages_vt_ingame_chat, value_messages_slot_msg_execute, (uintptr_t)fn_chat_msg_execute, (void *)h); }
internal inline int patch_msg_resign(resign_msg_execute_fn h) { return patch_vtable(data_messages_vt_resign, value_messages_slot_msg_execute, (uintptr_t)fn_resign_msg_execute, (void *)h); }
// the load and unload commands (transports.c)
internal inline int patch_msg_load(load_msg_execute_fn h) { return patch_vtable(data_messages_vt_msg_load, value_messages_slot_msg_execute, (uintptr_t)fn_load_msg_execute, (void *)h); }
internal inline int patch_msg_unload(unload_msg_execute_fn h) { return patch_vtable(data_messages_vt_msg_unload, value_messages_slot_msg_execute, (uintptr_t)fn_unload_msg_execute, (void *)h); }
// the game start message's execution (its vtable slot 10: it seeds the simulation's generator)
internal inline int patch_start_exec(start_msg_execute_fn h) { return patch_code32(data_messages_start_vtbl_exec, (uintptr_t)fn_start_msg_execute, (uint32_t)(uintptr_t)h); }

// THE MOD'S OWN MESSAGE (kind value_messages_kind_free_first, one the game's factory has none of): what one PC
// decides and every PC must carry out alike - the host's AI talk, the language model's decisions (in multiplayer the
// model runs on the host). A message of the game's in every respect but its class: EEMessage's construct, the game's
// operator new, the network's header (ExportHeader / ImportHeader), sent through the lockstep server like a player's
// command and executed on every PC, the sender's too, at the step it is scheduled for. Its vtable is the chat's (sent
// while paused, as talk is) with EEMessage's destructor (it owns nothing: its bytes are in it) and its own export,
// size, import, execute and name; its factory takes the kind's place in NEMessage::sConstructors (mod_msg_register:
// at every game start, after the game filled the table at its own). The payload: a subject (who handles it - each
// subject's handler, mod_msg_on), the bytes' count, the bytes. The sender is its originating slot (msg_get_from: the
// network sets it - a message can't name another; single player: 0xFE). About 512 bytes a turn leave one PC, the rest
// in later turns, in order (docs/engine-notes.md "Network messages").
#define MOD_MSG_MAX 480
enum { MOD_MSG_TEST };   // the subjects (test/testcmd.c `modmsg`: the message itself tested)
struct mod_msg { uint8_t base[0x10]; uint8_t subject, spare; uint16_t n; uint8_t data[MOD_MSG_MAX]; };
typedef void (*mod_msg_fn)(int from_slot, const uint8_t *data, int n);
internal mod_msg_fn g_mod_msg_on[16];
internal void *g_mod_msg_vt[13];   // [0] EEMessage's RTTI locator, then the 12 slots (the object points at [1])
internal unsigned g_mod_msg_bad;   // received with a subject nobody handles, or a count too large (corrupt)
internal inline uint32_t msg_export_header(void *m, uint8_t *buf) { return ((uint32_t (__thiscall *)(void *, uint8_t *))*(void **)messages_iat_export_header_at())(m, buf); }
internal inline uint32_t msg_import_header(void *m, uint8_t *mem) { return ((uint32_t (__thiscall *)(void *, uint8_t *))*(void **)messages_iat_import_header_at())(m, mem); }
internal int __thiscall mod_msg_size(struct mod_msg *m) { return 4 + 4 + m->n; }   // the header, subject, spare, count, bytes
internal void __thiscall mod_msg_export(struct mod_msg *m, uint8_t **buf, int *size) {   // as EEMCheatCode's 0x4B4E9E
    int n = mod_msg_size(m);
    uint8_t *b = game_new((uintptr_t)n);
    *buf = b;
    if (!b) return;
    uint32_t h = msg_export_header(m, b);
    b[h] = m->subject;
    b[h + 1] = 0;
    memcpy(b + h + 2, &m->n, 2);
    memcpy(b + h + 4, m->data, m->n);
    *size = n;
}
internal void __thiscall mod_msg_import(struct mod_msg *m, uint8_t *mem) {
    uint32_t h = msg_import_header(m, mem);
    uint16_t n;
    m->subject = mem[h];
    memcpy(&n, mem + h + 2, 2);
    if (n > MOD_MSG_MAX) { n = 0; m->subject = 0xFF; }   // (corrupt: not carried out)
    m->n = n;
    memcpy(m->data, mem + h + 4, n);
}
internal void __thiscall mod_msg_execute(struct mod_msg *m) {
    mod_msg_fn f = m->subject < 16 ? g_mod_msg_on[m->subject] : NULL;
    if (f) f((uint8_t)msg_get_from((ee_msg *)m), m->data, m->n);
    else g_mod_msg_bad++;
}
internal const char *__thiscall mod_msg_name(struct mod_msg *m) { (void)m; return "eeaimod"; }
internal ee_msg *mod_msg_build(uint8_t *mem) {   // the factory (cdecl, as the game's message_build)
    struct mod_msg *m = game_new(sizeof *m);
    if (!m) return NULL;
    msg_construct((ee_msg *)m, 0xFF, 0);
    *(void ***)m = &g_mod_msg_vt[1];
    mod_msg_import(m, mem);
    return (ee_msg *)m;
}
// its vtable made and its factory in the table: at every game start, on every PC (a message can come at any step)
internal void mod_msg_register(void) {
    void **chat = (void **)messages_vt_ingame_chat_at(), **base = (void **)(uintptr_t)vt_ee_message;
    void **table = *(void ***)messages_iat_constructors_at();
    g_mod_msg_vt[0] = base[-1];
    for (int i = 0; i < 12; i++) g_mod_msg_vt[1 + i] = chat[i];
    g_mod_msg_vt[1 + 0] = base[0];
    g_mod_msg_vt[1 + slot_msg_export] = (void *)mod_msg_export;
    g_mod_msg_vt[1 + slot_msg_size] = (void *)mod_msg_size;
    g_mod_msg_vt[1 + slot_msg_import] = (void *)mod_msg_import;
    g_mod_msg_vt[1 + slot_msg_execute] = (void *)mod_msg_execute;
    g_mod_msg_vt[1 + slot_msg_name] = (void *)mod_msg_name;
    if (table) table[value_messages_kind_free_first] = (void *)mod_msg_build;
}
// the subject's handler (once, at the start: who carries the message out on every PC)
internal inline void mod_msg_on(int subject, mod_msg_fn f) { if (subject >= 0 && subject < 16) g_mod_msg_on[subject] = f; }
// a message sent to every PC, the bytes (n at most MOD_MSG_MAX) for the subject's handler: 0 not sent
internal int mod_msg_send(int subject, const void *data, int n) {
    ee_server *net = game_server();
    if (!net || subject < 0 || subject >= 16 || n < 0 || n > MOD_MSG_MAX || !g_mod_msg_vt[1]) return 0;
    struct mod_msg *m = game_new(sizeof *m);
    if (!m) return 0;
    msg_construct((ee_msg *)m, (uint8_t)value_messages_kind_free_first, 1);   // (sent also by a player out of the game, as chat)
    *(void ***)m = &g_mod_msg_vt[1];
    m->subject = (uint8_t)subject;
    m->spare = 0;
    m->n = (uint16_t)n;
    memcpy(m->data, data, (size_t)n);
    server_send_all(net, m);
    return 1;
}
