/// the in-game chat message (EEMInGameChat)
[c_type("ee_msg"), c_prefix("chat_msg")]
unsafe struct EEMInGameChat
{
    [offset(0xA), found(b)]
    uint8 from;

    [offset(0x10), found(c)]
    block text;
}

/// the select message (EEMSelectUnits)
[c_type("ee_msg"), c_prefix("select_msg")]
unsafe struct EEMSelectUnits
{
    [offset(0x10), found(b)]
    block selection;

    [offset(0x14), found(b)]
    uint32* ids_begin;

    [offset(0x134), found(c)]
    uint8 flag;
}

/// the load-into-a-transport message (EEMLoadTransport)
[c_type("ee_msg"), c_prefix("load_msg")]
unsafe struct EEMLoadTransport
{
    /// its Execute's reads (both builds)
    [offset(0x10), found(c)]
    uint32 transport;

    /// its Execute's reads (both builds)
    [offset(0x14), found(c)]
    int32 unit;
}

/// the move message (EEMPathTo)
[c_type("ee_msg"), c_prefix("path_msg")]
unsafe struct EEMPathTo
{
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
}

/// the game start message (EEMStartLoading)
[c_type("ee_msg"), c_prefix("start_msg")]
unsafe struct EEMStartLoading
{
    [offset(0x14), found(c)]
    uint32 seed;
}

/// the tribute message (EEMPayTribute)
[c_type("ee_msg"), c_prefix("tribute")]
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
}

/// the unload message (EEMUnloadTransport)
[c_type("ee_msg"), c_prefix("unload_msg")]
unsafe struct EEMUnloadTransport
{
    /// its Execute's reads (both builds)
    [offset(0x14), found(c)]
    int32 x;

    /// its Execute's reads (both builds)
    [offset(0x18), found(c)]
    int32 y;
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

/// messages's data of the game, places in its code and numbers
static unsafe class messages
{
    [address((ee)0x004BD133, (aoc)0x004C5E40)]
    static code call_sel_pack;

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
}

/// the game's functions messages.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class messages_functions
{
    /// new 0x24; (UWideString *)
    [address((ee)0x005C2A2F, (aoc)0x005D3196), thiscall]
    static extern void* msg_chat_init(void* mem, void* wide_text);

    /// new 0x18; (code, arg)
    [address((ee)0x005C29FB, (aoc)0x005D3162), thiscall]
    static extern void* msg_cheat_init(void* mem, int32 code, int32 arg);

    [address((ee)0x004B78C7, (aoc)0x004BFDD2), thiscall]
    static extern void msg_ingame_chat(EEMInGameChat* msg);

    /// EEMLoadTransport's Execute
    [address((ee)0x004B9446, (aoc)0x004C1B79), thiscall]
    static extern void msg_load(EEMInGameChat* msg);

    /// EEMPathTo's Execute
    [address((ee)0x004B9D7B, (aoc)0x004C281C), thiscall]
    static extern void msg_path(EEMInGameChat* msg);

    /// EEMUnloadTransport's Execute
    [address((ee)0x004BF756, (aoc)0x004C8615), thiscall]
    static extern void msg_unload(EEMInGameChat* msg);

    [address((ee)0x004BA6BE, (aoc)0x004C315B), thiscall]
    static extern void msg_pay_tribute(EEMInGameChat* msg);

    [address((ee)0x004BB906, (aoc)0x004C44BB), thiscall]
    static extern void msg_resign(EEMInGameChat* msg);

    /// EEMSelectUnits: thiscall(msg, rect, type, 0x4E)
    [address((ee)0x004BD072, (aoc)0x004C5D7F), thiscall]
    static extern void select_msg_init(void* msg, float32* rect, int32 type, int32 flags);

    [address((ee)0x004BD15A, (aoc)0x004C5E67), cdecl]
    static extern uint32 sel_pack(uint8* list, uint32 player_byte, uint8* @out);
}
