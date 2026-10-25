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

/// the lockstep server or client, [G_SERVER] (EEServer, EEClient)
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
