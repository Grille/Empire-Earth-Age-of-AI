/// the draw list's records, hooks.draw_old (0x148 each)
[c_type("void")]
unsafe struct ee_draw_record
{
    /// its owner's colour (player +0x438 / +0x43C / +0x440, filled at 0x502xxx); init 0x506442 sets 1.0
    [offset(0x40), found(c)]
    vec3 color;

    /// a second colour: 0x502xxx sets it by the unit's serial (0.2 steps, serial % 3) or from the unit's own values
    /// when record +0x146 is set; init 1.0
    [offset(0x4C), found(c), guess]
    vec3 tint;

    /// its opacity (EEROUnits.opacity writes it before the fill; the same in AoC: its draw list sites read
    /// hooks.draw_old + 0x58 and + 0xFC, 0x50B2AD, 0x50EF07)
    [offset(0xFC), found(c)]
    float32 opacity;

    /// a draw list record
    [address((ee)0x00506442, (aoc)0x005118E9)]
    extern void init();

    // THE FIELDS ITS FILL WRITES (EEROUnits' FillUnit, EE 0x502622, AoC 0x50D3AD; collect.c's port uc_fill) - the
    // same offsets in both builds

    /// where it is drawn: a GETransformation (4 rows; the complex unit's +0x130 copied, else made at its tile)
    [offset(0x0), found(c)]
    fixed float32 transform[16];

    /// vectors (MSVC: the first element at +4, the end at +8; the list's init loop starts at the first, hooks.draw_sites):
    /// the health bars - [0] its own hit points over its
    /// type's, then each unit inside's (for the owner and allies); the complex unit's graphics' per-part values
    /// (+0x204) and their parts' models (+0x4C of each); the two lists of the complex unit's +0x1B8 and +0x1BC (each
    /// element's +0xC and +4) [guess: what they are drawn as]
    [offset(0x58), found(c)]
    vector hp_bars;

    [offset(0x68), found(c), guess]
    /// the complex unit's part_values, one for each part of its graphics
    vector part_values;

    [offset(0x78), found(c), guess]
    /// each part's model (the graphics' +4 vector, its elements' +0x4C), beside part_values
    vector part_models;

    [offset(0x88), found(c), guess]
    /// the complex unit's list_a: each element's +0xC (list_a_4 its +4)
    vector list_a_c;

    [offset(0x98), found(c), guess]
    /// the complex unit's list_a: each element's +4
    vector list_a_4;

    [offset(0xA8), found(c), guess]
    /// the complex unit's list_b: each element's +0xC (list_b_4 its +4)
    vector list_b_c;

    [offset(0xB8), found(c), guess]
    /// the complex unit's list_b: each element's +4
    vector list_b_4;

    /// the name shown under the unit under the mouse (UWideString: its stats' name; its characters at +4)
    [offset(0xC8), found(c)]
    wstring label;

    [offset(0xDC), found(c)]
    EEUnitClass* type;

    /// its graphics (EEUnit.graphics) and a second one (its type's built_type's stats' gfx; 0: none)
    [offset(0xE0), found(c)]
    EEGraphicObject* gfx;

    [offset(0xE4), found(c)]
    EEGraphicObject* gfx_built;

    /// its scale (the physical model's, or its stats' height)
    [offset(0xE8), found(c)]
    float32 scale;

    /// the selection circle's size: its type's mark_size, or twice its sight or range (by engine_display.circle_mode)
    [offset(0xEC), found(c)]
    float32 circle;

    /// its type's two floats +0x468 / +0x46C (AoC +0x16C / +0x170) while it has hit points, else 0 [guess: the health
    /// bar's place]
    [offset(0xF0), found(c), guess]
    float32 bar_x;

    [offset(0xF4), found(c), guess]
    /// the type's +0x46C (AoC +0x170) while it has hit points, beside bar_x
    float32 bar_y;

    /// the ground's height under it (0 on a tile with its light byte set while it is above 0)
    [offset(0xF8), found(c)]
    float32 ground_z;

    /// a power bar's fill (the complex unit's power over its stats'; 1 when its type has none)
    [offset(0x100), found(c)]
    float32 power_bar;

    /// how far between frame and next_frame (the complex unit's +0x23C)
    [offset(0x104), found(c)]
    float32 fraction;

    /// a gate's opening (the complex unit's +0x228), with gated set
    [offset(0x108), found(c)]
    float32 gate_open;

    /// with tinted: the tint's fourth value (engine_display.tint_alpha)
    [offset(0x10C), found(c)]
    float32 tint_alpha;

    [offset(0x110), found(c)]
    int32 class_id;

    [offset(0x114), found(c)]
    int32 type_id;

    [offset(0x118), found(c)]
    int32 family;

    /// its script state's id (0: none)
    [offset(0x11C), found(c)]
    uint32 state;

    /// what it produces (EEComplexUnit.producing while produce_left > 0, else 100000)
    [offset(0x120), found(c)]
    int32 producing;

    [offset(0x124), found(c)]
    int32 anim;

    [offset(0x128), found(c)]
    int32 size;

    [offset(0x12C), found(c)]
    int32 frame;

    [offset(0x130), found(c)]
    int32 next_frame;

    [offset(0x138), found(c)]
    int32 serial;

    [offset(0x13C), found(c)]
    int32 player;

    /// its first goal of kind 5, 6 or 2 asked for its vf6 (while it has hit points) [guess]
    [offset(0x140), found(c), guess]
    uint32 goal_value;

    /// a gaia animal or bush with food the player hasn't in sight
    [offset(0x145), found(c)]
    uint8 fogged;

    /// the complex unit's tint on (its +0x33D: the tint its +0x1AC)
    [offset(0x146), found(c)]
    uint8 tinted;

    [offset(0x147), found(c)]
    uint8 gated;
}

/// the small objects' draw list's records, hooks.draw_small_old (0x3C each): a tree's, as its fill (EEROUnits' FillObject,
/// EE 0x501DFF, AoC 0x50CBA3; collect.c's port uc_fill_tree) makes it - the same offsets in both builds
[c_type("void")]
unsafe struct ee_draw_small_record
{
    /// where it is drawn: its tile's middle shifted by its id, the ground's height there
    [offset(0x0), found(c)]
    vec3 pos;

    [offset(0xC), found(c)]
    EEGraphicObject* gfx;

    /// its stats' height times 1 + (its id % 11) x engine_display.tree_scale_step
    [offset(0x10), found(c)]
    float32 scale;

    /// the selection circle's size (its type's mark_size) while it is the player's selected unit
    [offset(0x14), found(c)]
    float32 circle;

    /// its type's bar_x / bar_y when a diagonal neighbour tile holds no tree, else 0 [guess: what they are for a tree]
    [offset(0x18), found(c), guess]
    float32 bar_x;

    /// the type's bar_y the same way (beside bar_x)
    [offset(0x1C), found(c), guess]
    float32 bar_y;

    /// its hit points over its stats'
    [offset(0x20), found(c)]
    float32 hp;

    /// its opacity (EEROUnits.opacity writes it before the fill)
    [offset(0x24), found(c)]
    float32 opacity;

    /// its sway in the wind: (1 + cos(tilt_offset - ((sway_clock + id x 8) % 360 - 180) x sway_step)) / 2, clamped
    /// to 0 .. 1; 0 for a model of one frame
    [offset(0x28), found(c)]
    float32 sway;

    [offset(0x2C), found(c)]
    int32 anim;

    /// 1 for a model of more than one frame, else 0; next_frame 0
    [offset(0x30), found(c)]
    int32 frame;

    [offset(0x34), found(c)]
    int32 next_frame;

    [offset(0x38), found(c)]
    uint8 flag;
}

/// hooks's data of the game, places in its code and numbers
static unsafe class hooks
{
    [address((ee)0x006177CF, (aoc)0x0062A366)]
    static code call_attack_alert;

    /// the 2017 update sets a unit's reload inline where AoC calls unit.reload_wait (its random wait): the attack ticks'
    /// cmp [esi+0x2A0], eax before their store of the type's reload (EEUGAttackGround 0x60ABC0, EEUGAttackUnit
    /// 0x60B34C; a je over the store follows) ...
    [address((ee)0x0060ABC0, (aoc)none)]
    [address((ee)0x0060B34C, (aoc)none)]
    static code reload_tick_cmp;

    /// ... and the fire's stores mov [esi+0x2A0], eax (0x61E18E the type's reload while the unit's is 0, 0x61E1C7 1.5 x
    /// it with alt_attack) - the unit in esi, the reload in eax at all four
    [address((ee)0x0061E18E, (aoc)none)]
    [address((ee)0x0061E1C7, (aoc)none)]
    static code reload_fire_store;

    /// the loading splash's start (jne)
    [address((ee)0x00534400, (aoc)0x0053F2FE)]
    static code code_splash_start_jne;

    /// the loading splash's stop (jne)
    [address((ee)0x0053443D, (aoc)0x0053F33B)]
    static code code_splash_stop_jne;

    [address((ee)0x008D2D30, (aoc)0x008E94D0)]
    static block draw_old;

    /// the draw list's operands to move (drawlist.c, in its table's order)
    [address((ee)0x0050099C, (aoc)0x0050B497)]
    [address((ee)0x00504158, (aoc)0x0050F093)]
    [address((ee)0x005007B2, (aoc)0x0050B2AD)]
    [address((ee)0x005007CC, (aoc)0x0050B2C7)]
    [address((ee)0x00503C69, (aoc)0x0050EBA4)]
    [address((ee)0x00503ED0, (aoc)0x0050EE0B)]
    [address((ee)0x00503FCC, (aoc)0x0050EF07)]
    [address((ee)0x005043B5, (aoc)0x0050F2F0)]
    [address((ee)0x0050DC5A, (aoc)0x0051854F)]
    static code draw_sites;

    /// the small objects' draw list: 0x600 records of 0x3C
    [address((ee)0x008FBD30, (aoc)0x009124D0)]
    static block draw_small_old;

    /// the small objects' draw list's operands to move (drawlist.c, in its table's order)
    [address((ee)0x00500BBC, (aoc)0x0050B6B7)]
    [address((ee)0x0050419D, (aoc)0x0050F0D8)]
    [address((ee)0x00503C5A, (aoc)0x0050EB95)]
    [address((ee)0x00504016, (aoc)0x0050EF51)]
    [address((ee)0x00504041, (aoc)0x0050EF7C)]
    [address((ee)0x005041AD, (aoc)0x0050F0E8)]
    [address((ee)0x005043A6, (aoc)0x0050F2E1)]
    static code draw_small_sites;

    [address((ee)0x005D0B2C, (aoc)0x005E1438)]
    static code exit_confirm_at;

    /// __thiscall, ret 4
    [address((ee)0x00441432, (aoc)0x00440E57)]
    static code fn_add_movie;

    [address((ee)0x00519BC1, (aoc)0x00524497)]
    static code fn_attack_alert;

    /// the float 1/600 (the minimap's y scale operand)
    [address((ee)0x0082CBCC, (aoc)0x00842374)]
    static block float_1_600;

    /// the float 1/800 (the minimap's x scale operand)
    [address((ee)0x008358A8, (aoc)0x0084B940)]
    static block float_1_800;

    [import_slot("msvcrt.dll", "srand")]
    [address((ee)0x00824644, (aoc)0x00838638)]
    static block iat_srand;

    /// call AddMovie(path) in PlayIntro 0x538867
    [address((ee)0x00538909, (aoc)0x00543A57)]
    [address((ee)0x0053892A, (aoc)0x00543A78)]
    [address((ee)0x00538945, (aoc)0x00543A93)]
    static code intro_calls;

    /// the minimap's operands of hooks.float_1_800
    [address((ee)0x0065E01D, (aoc)0x00675618)]
    [address((ee)0x0065E8A3, (aoc)0x00675E95)]
    [address((ee)0x0065EB3E, (aoc)0x00676130)]
    static code minimap_fx_ops;

    /// the minimap's operands of hooks.float_1_600
    [address((ee)0x0065E032, (aoc)0x0067562D)]
    [address((ee)0x0065E8CE, (aoc)0x00675EC0)]
    [address((ee)0x0065EB68, (aoc)0x0067615A)]
    static code minimap_fy_ops;

    /// GEFontBitmap::Draw (both overloads): movsx of a character
    [value((ee)0x27A46, (aoc)0x15C01)]
    static uint32 rva_lle_font_movsx_1;

    [value((ee)0x27C9A, (aoc)0x15E55)]
    static uint32 rva_lle_font_movsx_2;

    /// its import slot
    [value((ee)0x940EC, (aoc)0x970EC)]
    static uint32 rva_lle_iat_wfmo;

    /// ISInitialize's, setting up the device
    [value((ee)0x4A76F, (aoc)0x39607)]
    static uint32 rva_lle_input_coop;

    /// Low-Level Engine, ISMouse::Resume: the 5 of push 5 (DISCL_ flags)
    [value((ee)0x4C838, (aoc)0x3BA05)]
    static uint32 rva_lle_mouse_coop;

    /// NESynchronousServer::Send: call [WaitForMultipleObjects] (turbo)
    [value((ee)0x55DF5, (aoc)0x49DDA)]
    static uint32 rva_lle_send_wait;

    /// je over the CloseWindow call on WM_ACTIVATEAPP(0)
    [address((ee)0x00662666, (aoc)0x00679C57)]
    static code site_close_window_je;

    /// order_marks (0x4B12A2): jne past the first of its filters - a unit of the group gets its circle only while its
    /// +0x298 (AoC +0x29C: its number in the group) is under 15 (the formation's kind 0) or 20 (1 .. 5), or (6, 7) every
    /// n-th; made a jmp to where the circle is drawn (EB 35 in both builds)
    [address((ee)0x004B139C, (aoc)0x004B87C7)]
    static code site_order_marks_filter;

    /// order_marks: cmp [ebp-0x18], 10 after a circle is drawn (how 0xBB) - the group's circles end at the 10th; the
    /// 10 its last byte (+3)
    [address((ee)0x004B1681, (aoc)0x004B8AB3)]
    static code site_order_marks_max;

    [address((ee)0x006601F1, (aoc)0x006777E5)]
    static code site_editor_menu;

    [address((ee)0x0065FD6D, (aoc)0x00677361)]
    static code site_editor_res;

    [address((ee)0x00553348, (aoc)0x00561459)]
    static code site_sim_receive;

    [address((ee)0x005533C5, (aoc)0x005614D6)]
    static code site_sim_wait;
}

/// the game's functions hooks.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class hooks_functions
{
    [address((ee)0x0065F314, (aoc)0x006768FE), stdcall]
    // the other calls of set_resolution (display.c)
    [call_site("other_resolution", (ee)0x0065F30A, (aoc)0x006768F4)]
    [call_site("other_resolution", (ee)0x0065F61B, (aoc)0x00676C0A)]
    [call_site("other_resolution", (ee)0x0065F945, (aoc)0x00676F39)]
    [call_site("other_resolution", (ee)0x0065FAD4, (aoc)0x006770C8)]
    [call_site("other_resolution", (ee)0x00660CD3, (aoc)0x0067831D)]
    [call_site("other_resolution", (ee)0x00661329, (aoc)0x00678973)]
    [call_site("other_resolution", (ee)0x00661363, (aoc)0x006789AD)]
    static extern void set_resolution(int32 w, int32 h, int32 force, int32 flag);

    /// thiscall(the collection's, unit, its draw list record (ee_draw_record, 0x148), p3, p4): the units' collection
    /// (EEROUnits::vf0 0x503B83) fills a unit's record - its type, colours ... and its matrix (the record's first 3 x 4
    /// floats, as a unit's): a unit's from its physics part, an object without one (an ambient object, a resource
    /// site: the plain EEUnit, no matrix of its own) turned by its heading only (type +0x5FA, else 0) at the ground's
    /// height at its middle - level on any slope. Types with the flag +0x619 get the small record (0x501DFF) instead
    [address((ee)0x00502622, (aoc)0x0050D3AD), thiscall]
    [call_site("draw_record_fill", (ee)0x00503FFD, (aoc)0x0050EF38)]
    static extern void draw_record_fill(void* collection, EEUnit* u, float32* record, void* p3, void* p4);

    /// thiscall(the click's list - the units' collection's list of what it drew, at 0x9125D8: 0x50-byte entries from
    /// +0x2A0, their count +0x2A8, the object +0x4C -, object): whether the object has an entry already. The collection
    /// asks it for every object it is about to draw (one on several tiles is met again): a linear search, on the
    /// 8000-unit save 1000 a frame over 1175 entries
    [address((ee)0x0050E57F, (aoc)0x00518E74), thiscall]
    [call_site("pick_listed", (ee)0x00503F92, (aoc)0x0050EECD)]
    static extern uint8 pick_listed(void* list, EEUnit* u);
}

/// TCP Protocol.dll's code (each build ships its own): its places relative to the DLL's base
[dll("TCP Protocol.dll")]
static unsafe class tcp_protocol
{
    /// the ports a joining game connects to - NPIMConnectForXfer's 33335, NPIMConnect's 33334 (a detected game's
    /// address, a name or an IP): a test slot that joins another slot's game moves them by that slot's offset
    [address((ee)0x000017E2, (aoc)0x00001684)]
    [address((ee)0x0000196F, (aoc)0x0000185C)]
    [address((ee)0x000019EB, (aoc)0x000018D8)]
    static code connect_ports;

    /// the network ports' immediates a game listens on and broadcasts to (UDP 33334, TCP 33335: NPIMPublish's ...),
    /// moved for a test slot
    [address((ee)0x00001EB8, (aoc)0x00001DA5)]
    [address((ee)0x00001FBD, (aoc)0x00001EAA)]
    [address((ee)0x00002148, (aoc)0x00002035)]
    [address((ee)0x0000279C, (aoc)0x00002689)]
    [address((ee)0x00002F2C, (aoc)0x00002E1C)]
    static code ports;
}
