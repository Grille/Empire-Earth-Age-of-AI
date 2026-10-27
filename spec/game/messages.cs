/// a message players send each other through the server (EEMessage, the base of every message: EEMPayTribute,
/// EEMInGameChat, EEMSelectUnits ...). Sent (EEServer slot 6) it is exported - the 4-byte header, then its payload -
/// and on every PC, the sender's too, made again from those bytes: NEMessage::ConvertMemoryToMessage (Low-Level
/// Engine.dll) calls the factory its kind has in NEMessage::sConstructors (messages.iat_constructors; the game's
/// message_build for kinds 0x41 .. 0xFD), which makes the object and imports the bytes; executed at the step it is
/// scheduled for. A kind of the mod's own: a factory in a free kind's place (messages.kind_free_first ..
/// kind_free_last) and a vtable of its own (game/api/messages.c).
[c_type("ee_msg"), vtable((ee)0x00824B0C, (aoc)0x00838AE8)]
unsafe struct EEMessage
{
    /// NEMessage's (Low-Level Engine.dll, the base of EEMessage): every constructor sets 1 (the DLL's 0x1000848C, EE's
    /// copy 0x404B58), holding it again adds one (NESynchronousServer::SendToAllAndWait, EEServer::vf27 0x4C3839)
    [offset(0x4), found(c)]
    uint32 ref_count;

    /// the message's type id (0x4E select, 0x67 load ...)
    [offset(0x8), found(c)]
    uint8 kind;

    /// NEMessage's: the slot it is for, 0xFF all (the DLL's GetForSpecificSlot / SetForSpecificSlot 0x100084C3 /
    /// 0x100084CB; EEServer::vf27 also tests 0xFE); every EE message is built with 0xFF
    [offset(0x9), found(c)]
    uint8 for_slot;

    /// NEMessage's originating slot (GetOriginatingSlot / SetOriginatingSlot): the sender's - every constructor sets
    /// 0xFF, the network the sending PC's slot (a message can't name another sender: the chat shows the player of it)
    [offset(0xA), found(c)]
    int8 from;

    /// vf0: the scalar deleting destructor - thiscall(msg, flags 1: freed); EEMessage's (0x4C04FA) for every message
    /// that owns nothing, the chat's its own (its text)
    [slot(0)]
    extern void* destroy(uint8 flags);

    /// vf1 (NEMessage's GetIsAvailableToSend; EEMessage's 0x401A2E): sent while the local player is in the game, or
    /// when sendable_when_out
    [slot(1)]
    extern char8 available_to_send();

    /// vf2: NEMessage's GetCanSendWhilePaused (32 of the game's messages say 1: 0x4B129C)
    [slot(2)]
    extern char8 can_send_while_paused();

    /// vf3: NEMessage's GetCountsTowardsPlayerInputThrottle
    [slot(3)]
    extern char8 counts_toward_throttle();

    /// vf4 / vf5: NEMessage's Get / SetSendToAllAndWaitMessageID
    [slot(4)]
    extern char8 get_wait_id(uint32* id);

    [slot(5)]
    extern void set_wait_id(uint32 id);

    /// vf6: its export - thiscall(msg, uint8** buf, int32* size): a new buffer of size() bytes (the game's operator
    /// new 0x683640), the header (messages.iat_export_header: 4 bytes), its payload (EEMCheatCode's 0x4B4E9E)
    [slot(6)]
    extern void export(uint8** buf, int32* size);

    /// vf7: its exported size, the header's 4 bytes and its payload's (EEMCheatCode: 0xD)
    [slot(7)]
    extern int32 size();

    /// vf8: its import - thiscall(msg, uint8* mem): the header (messages.iat_import_header) and its payload from the
    /// bytes (EEMCheatCode's 0x4B4F35)
    [slot(8)]
    extern void import(uint8* mem);

    /// vf9: 1 when other is the same request - only EEMLoadOrSaveGame's says so (0x4B8754: kind 0x9A, the same id at
    /// +0x10), every other message's 0 (0x4B6F21)
    [slot(9), guess]
    extern char8 same_as(EEMessage* other);

    /// vf10: the message carried out (EEMPathTo's and EEMLoadTransport's both builds)
    [slot(10)]
    extern void execute();

    /// vf11: its name for the debug output ("Cheat Code": EEMCheatCode's 0x4B4F2F)
    [slot(11)]
    extern char8* name();

    /// EEMessage(kind, sendable_when_out): ref_count 1, for_slot and from 0xFF, header_flags 0, EEMessage's vtable -
    /// every message's constructor and the factory call it first (message_build 0x4B1B31: (0xFF, 0), then the class's
    /// vtable and its import)
    [address((ee)0x00404B58, (aoc)0x004032A6)]
    extern void construct(uint8 kind, char8 when_out);

    /// NEMessage's constructor's fourth byte, the first of the 4-byte wire header (ExportHeader 0x1004D025: +0xB, +8,
    /// +9, +0xA); every EE message passes 0 and nothing reads it
    [offset(0xB), found(c), guess]
    uint8 header_flags;

    /// the first payload byte after the header: every message's export and import move it (EEMCheatCode 0x4B4E9E /
    /// 0x4B4F35, EEMSelectUnits 0x4BD386 ...) but no constructor sets it and no execute reads it - an uninitialized
    /// byte on the wire
    [offset(0xC), found(c), guess]
    uint8 spare;

    /// EEMessage(type, flag)'s flag (0x404B58), not serialized: EEMessage::vf1 0x401A2E (NEMessage's
    /// GetIsAvailableToSend) sends it while the local player is in the game or this is set (chat, selections: 1)
    [offset(0xD), found(c)]
    uint8 sendable_when_out;
}

/// the resignation (EEMResign)
[@base(typeof(EEMessage)), c_type("ee_resign_msg")]
unsafe struct EEMResign
{
    [address((ee)0x004BB906, (aoc)0x004C44BB)]
    extern void execute();
}

/// the in-game chat message (EEMInGameChat)
[@base(typeof(EEMessage)), c_type("ee_chat_msg")]
unsafe struct EEMInGameChat
{
    [offset(0x10), found(c)]
    block text;

    [address((ee)0x004B78C7, (aoc)0x004BFDD2)]
    extern void execute();

    /// new 0x24; (UWideString *)
    [address((ee)0x005C2A2F, (aoc)0x005D3196)]
    extern void* init(void* wide_text);
}

/// the select message (EEMSelectUnits)
[@base(typeof(EEMessage)), c_type("ee_select_msg")]
unsafe struct EEMSelectUnits
{
    [offset(0x10), found(b)]
    block selection;

    [offset(0x14), found(b)]
    uint32* ids_begin;

    /// the selection's rectangle
    [offset(0x20), found(c)]
    fixed float32 rect[4];

    /// the selection's kind (init's type)
    [offset(0x30), found(c)]
    int32 select_kind;

    /// the packed selection (sel_pack's output, its length the flag)
    [offset(0x34), found(c)]
    block packed;

    [offset(0x134), found(c)]
    uint8 flag;

    /// EEMSelectUnits: thiscall(msg, rect, type, 0x4E)
    [address((ee)0x004BD072, (aoc)0x004C5D7F)]
    extern void init(float32* rect, int32 type, int32 flags);
}

/// the load-into-a-transport message (EEMLoadTransport)
[@base(typeof(EEMessage)), c_type("ee_load_msg")]
unsafe struct EEMLoadTransport
{
    /// its Execute's reads (both builds)
    [offset(0x10), found(c)]
    uint32 transport;

    /// its Execute's reads (both builds)
    [offset(0x14), found(c)]
    int32 unit;

    /// EEMLoadTransport's Execute
    [address((ee)0x004B9446, (aoc)0x004C1B79)]
    extern void execute();
}

/// the move message (EEMPathTo)
[@base(typeof(EEMessage)), c_type("ee_path_msg"), vtable((ee)0x00824D84, (aoc)0x00838D5C)]
unsafe struct EEMPathTo
{
    /// the right-button drag's facing (the drag handler after 0x403542: atan2(-dx, dy) of the drag, 0 for a click),
    /// the move goal's +0x2C - the group's formation turned to it
    [offset(0x10), found(c)]
    float32 facing;

    /// its Execute's reads (both builds)
    [offset(0x14), found(c)]
    float32 x;

    /// its Execute's reads (both builds)
    [offset(0x18), found(c)]
    float32 y;

    /// its Execute's reads (both builds) - the move goal's +0x30
    [offset(0x1C), found(c)]
    uint8 goal_30;

    /// its Execute's reads (both builds) - the move goal's +0x31
    [offset(0x1D), found(c)]
    uint8 goal_31;

    /// its Execute's reads (both builds: the hand-out 0x5DD436 / AoC 0x5EDC93) - shift held: the goal queued after the
    /// unit's others
    [offset(0x1E), found(c)]
    uint8 queued;

    /// its Execute's reads (both builds) - the move goal's +0x33
    [offset(0x1F), found(c)]
    uint8 together;

    /// EEMPathTo's Execute: splits the selection into a group per kind and hands each the move (the formation's slots
    /// laid out within it)
    [address((ee)0x004B9D7B, (aoc)0x004C281C), overrides(typeof(EEMessage))]
    [vtable_site("path_msg_execute")]
    extern void execute();
}

/// the game start message (EEMStartLoading)
[@base(typeof(EEMessage)), c_type("ee_start_msg")]
unsafe struct EEMStartLoading
{
    /// the sender (0x5C7538's area) counts the start slots of kinds 1 and 2 into it; execute 0x4BEE57 copies it to
    /// server +0x2C and sets server +0x2E (multiplayer) when it is above 1; 1 byte on the wire
    [offset(0x10), found(c), guess]
    uint8 humans;

    [offset(0x14), found(c)]
    uint32 seed;

    [address((ee)0x004BEE57, (aoc)0x004C7CDC)]
    extern void execute();
}

/// the tribute message (EEMPayTribute)
[@base(typeof(EEMessage)), c_type("ee_tribute_msg")]
unsafe struct EEMPayTribute
{
    [offset(0x10), found(c)]
    int32 food;

    [offset(0x14), found(c)]
    int32 gold;

    [offset(0x18), found(c)]
    int32 iron;

    [offset(0x1C), found(c)]
    int32 stone;

    [offset(0x20), found(c)]
    int32 wood;

    [offset(0x24), found(v)]
    uint8 to;

    [address((ee)0x004BA6BE, (aoc)0x004C315B)]
    extern void execute();
}

/// the unload message (EEMUnloadTransport)
[@base(typeof(EEMessage)), c_type("ee_unload_msg")]
unsafe struct EEMUnloadTransport
{
    /// its Execute's reads (both builds)
    [offset(0x14), found(c)]
    int32 x;

    /// its Execute's reads (both builds)
    [offset(0x18), found(c)]
    int32 y;

    /// EEMUnloadTransport's Execute
    [address((ee)0x004BF756, (aoc)0x004C8615)]
    extern void execute();
}

/// the lockstep server or client, [game.server] (EEServer, EEClient)
unsafe struct EEServer
{
    /// / +0x190 sent / done
    [offset(0x10), found(b)]
    uint32 sent;

    /// messages waiting
    /// the next tick's messages, NESynchronousServer (port_dll.py)
    [offset(0x94), found(c)]
    block waiting;

    [offset(0x190), found(b)]
    uint32 done;

    /// NESynchronousServer::TuneForSlowCPU (port_dll.py)
    [offset(0x1B4), found(c)]
    uint32 rate_max;

    /// NESynchronousServer::TuneForSlowCPU (port_dll.py)
    [offset(0x1B8), found(c)]
    uint32 rate_min;

    /// NESynchronousServer::TuneForSlowCPU (port_dll.py)
    [offset(0x1FB), found(c)]
    uint8 rate;
}

/// the new-unit message, the cheat that makes a unit (EEMHackANewUnit)
[@base(typeof(EEMessage)), c_type("ee_new_unit_msg"), vtable((ee)0x00829338, (aoc)0x0083E088)]
unsafe struct EEMHackANewUnit
{
    /// EEMHackANewUnit: thiscall(int *type, float xy[2], char player)
    [address((ee)0x00650B3C, (aoc)0x00662C24)]
    extern void init(int32* type, float32* xy, char8 player);

    /// init 0x650B3C stores *type, export 0x4B6F2A moves 4 bytes
    [offset(0x10), found(c)]
    int32 unit_type;

    /// init stores xy[0] / xy[1] here, export moves 8 bytes, execute 0x4B7044 reads them as floats
    [offset(0x14), found(c)]
    fixed float32 pos[2];

    /// init's player byte, export / import 0x4B6F2A / 0x4B6FCC move 1 byte
    [offset(0x1C), found(c)]
    uint8 player;
}

/// a cheat code's message (EEMCheatCode)
[@base(typeof(EEMessage)), c_type("ee_cheat_msg"), vtable((ee)0x0082907C, (aoc)0x0083DDE0)]
unsafe struct EEMCheatCode
{
    /// new 0x18; (code, arg)
    [address((ee)0x005C29FB, (aoc)0x005D3162)]
    extern void* init(int32 code, int32 arg);

    /// init 0x5C29FB stores the cheat code, export / import 0x4B4E9E / 0x4B4F35 move it, execute 0x4B4E6A passes it to
    /// the cheat dispatcher 0x520339
    [offset(0x10), found(c)]
    int32 code;

    /// init's second argument, the dispatcher's third (execute 0x4B4E6A)
    [offset(0x14), found(c)]
    int32 arg;
}

/// messages's data of the game, places in its code and numbers
static unsafe class messages
{
    /// msg_cheat_init
    [value(24)]
    static uint32 size_cheat_msg;

    /// msg_chat_init
    [value(36)]
    static uint32 size_msg_chat;

    /// messages.vt_pay_tribute, messages.vt_ingame_chat, messages.vt_resign, messages.vt_msg_load, messages.vt_msg_unload: the message's Execute
    [value(10)]
    static uint32 slot_msg_execute;

    /// [game.server] (EEServer, EEClient): thiscall(net, message, 0xFF)
    [value(6)]
    static uint32 slot_server_send;

    [address((ee)0x008285D8, (aoc)0x0083D2A0)]
    static void* start_vtbl_exec;

    [address((ee)0x0082936C, (aoc)0x0083E0BC)]
    static vftable vt_ingame_chat;

    /// EEMLoadTransport
    [address((ee)0x00824E00, (aoc)0x00838DDC)]
    static vftable vt_msg_load;

    /// EEMUnloadTransport
    [address((ee)0x00828AC4, (aoc)0x0083D78C)]
    static vftable vt_msg_unload;

    [address((ee)0x008289C0, (aoc)0x0083D688)]
    static vftable vt_pay_tribute;

    [address((ee)0x00828820, (aoc)0x0083D4E8)]
    static vftable vt_resign;

    /// NEMessage::sConstructors's import slot (Low-Level Engine.dll): it holds the table's address - 256 factories,
    /// NEMessage *(*)(uint8 *mem) by kind (the header's second byte); the DLL's own kinds below 0x41, the game's
    /// message_build from 0x41 to 0xFD (messages_register)
    [address((ee)0x0082357C, (aoc)0x00837520)]
    static block iat_constructors;

    /// NEMessage::ExportHeader - thiscall(msg, uint8* buf): the 4-byte header (+0xB, kind, for_slot, from); its size
    [address((ee)0x00823580, (aoc)0x00837524)]
    static block iat_export_header;

    /// NEMessage::ImportHeader - thiscall(msg, uint8* mem): the header back; its size
    [address((ee)0x00823584, (aoc)0x00837528)]
    static block iat_import_header;

    /// the kinds no message of the game's has: its factory makes kinds 0x41 .. 0xAA (AoC .. 0xAE: message_build's
    /// cases) and says "Undefined message type" for the rest up to 0xFD
    [value(0xAF)]
    static uint32 kind_free_first;

    [value(0xFD)]
    static uint32 kind_free_last;
}

/// the game's functions messages.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class messages_functions
{
    /// cdecl(uint8* mem): the game's factory - a new message of the kind mem[1] (its class's size, EEMessage.construct
    /// (0xFF, 0), its vtable, its import of mem); NULL and an assertion for a kind it has none of
    [address((ee)0x004B1B31, (aoc)0x004B8F8E), cdecl]
    static extern EEMessage* message_build(uint8* mem);

    /// cdecl(): message_build into sConstructors for kinds 0x41 .. 0xFD, once at the start (0x530xxx's set-up)
    [address((ee)0x004B18A8, (aoc)0x004B8CDA), cdecl]
    static extern void messages_register();

    [address((ee)0x004BD15A, (aoc)0x004C5E67), cdecl]
    [call_site("sel_pack", (ee)0x004BD133, (aoc)0x004C5E40)]
    static extern uint32 sel_pack(uint8* list, uint32 player_byte, uint8* @out);
}
