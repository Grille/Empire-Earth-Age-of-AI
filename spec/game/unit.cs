// THE MAP'S OBJECTS: three classes, each the one before it and more. RTSUnit, the engine's (the unit list's node, tile,
// owner, id); EEUnit, its game's (type data, hit points, a resource's amount) - an ambient object or resource site, a
// tree, a gold mine; EEComplexUnit everything that moves or is built: units, buildings, ships, projectiles, the attack
// routes' PathPoint markers (type 223). The type data's factory (EEUnitClass::vf14 0x61E7BA) makes an EEUnit where
// the type's +0x4B0 is 0 and it isn't a PathPoint, else an EEComplexUnit; a save holds only these two (engine-notes
// "save format"). The C side holds all three as ee_unit for now: a field of EEComplexUnit read on a tree reads past
// its end - the caller has to know.
// Not one of them though named so: EEGhostUnit (below).

/// the engine's map object: the unit list's node, tile, owner and id (RTSDummyUnit, the list's head at 0x918314, is
/// only this)
[size(0x2C), c_type("ee_unit")]   // EEUnit's own fields from +0x2C (its constructor 0x616BDD clears +0x2C .. +0x44)
unsafe struct RTSUnit
{
    // FIELDS

    /// references (release 0x4FCAD3)
    [offset(0x4), found(c)]
    int32 refs;

    /// its node in the unit list {next, prev}
    [offset(0x8), found(c)]
    void* list_node;

    /// visibility: +0xC a uint16 per player, seen
    [offset(0x14), found(c)]
    ee_unit_vis* vis;

    /// tile x (-10000: off the map)
    [offset(0x1C), found(c)]
    int32 x;

    /// tile y
    [offset(0x20), found(c)]
    int32 y;

    [offset(0x24), found(c)]
    int32 serial;

    /// back in the pool (vf12 sets it)
    [offset(0x2A), found(c)]
    uint8 released;

    /// mIsValid
    [offset(0x2B), found(c)]
    uint8 valid;

    /// its owner
    [offset(0x18), found(c)]
    EEPlayer* owner;

    // VIRTUAL FUNCTIONS

    /// the object freed: the release 0x4FCAD3 calls it when the last reference goes (RTSUnit::vf13 0x628D98 jumps
    /// to the imported release, game.iat_release)
    [slot(13)]
    extern void release();
}

/// an ambient object or resource site (a tree, a gold mine); the base of every unit
[@base(typeof(RTSUnit)), size(0x48), c_type("ee_unit")]   // new 0x48, then its constructor 0x6161F4 / AoC 0x628CD2
[vtable((ee)0x00831FD8, (aoc)0x00847A20)]
unsafe struct EEUnit
{
    // FIELDS

    /// its type's data (TD)
    [offset(0x2C), found(c)]
    EEUnitClass* type_data;

    /// (the game's dump: physics) - a unit too
    [offset(0x30), found(c)]
    EEUnit* physics;

    /// its player's stats of the type (USTAT)
    [offset(0x34), found(c)]
    ee_unit_stats* stats;

    /// a resource site's resource
    [offset(0x38), found(c)]
    int32 resource;

    /// hit points (> 0: alive)
    [offset(0x3C), found(c)]
    int32 hp;

    /// a resource site's amount left
    [offset(0x40), found(c)]
    int32 amount;

    // VIRTUAL FUNCTIONS (EEUnit's slots; EEComplexUnit has its own functions in them)

    /// may unit enter the tile this object stands on (the path search's tile test 0x41D36A asks every object there):
    /// EEUnit's by its type data (a flag +0x600, its family, +0x568 - AoC moved them all) and its hit points
    [slot(18), address((ee)0x006167C7, (aoc)0x006292C9)]
    extern int8 lets_pass(EEComplexUnit* unit);
    /// an attack's hit points (EEUnit::vf28 0x6172BC, the call 0x61E0B2), the attacker's player index, one 1; AoC put
    /// a stub (ret 4) at 28
    [slot((ee)28, (aoc)29)]
    extern void damage(EEComplexUnit* attacker, int32 dmg, int32 player, int32 one);

    /// freed: its slot 12 (+0x30), its references 0, then deleted
    [address((ee)0x00616DBB, (aoc)0x006298E1), overrides(typeof(RTSUnit))]
    extern void release();

    /// thiscall(unit): drops its current waypoint (the deque's front; GetNextMoveWaypoint); 0, or 7 with none
    [address((ee)0x005EDCF4, (aoc)0x005FBDD1)]
    extern int32 pop_waypoint();

    /// thiscall(unit): its acceleration (stats +0xA8; its group's with the group speed on)
    [address((ee)0x0054A4CA, (aoc)0x00554C95)]
    extern float32 accel();

    /// thiscall(unit): its top turn rate (radians/s, stats +0xB4; its group's with the group speed on)
    [address((ee)0x0054A4E7, (aoc)0x0055463E)]
    extern float32 max_turn();

    /// thiscall(unit, 0x1F51) - as the assignment does after the order
    [address((ee)0x005EFCF0, (aoc)0x005FDFA5)]
    extern void set_state(int32 state);

    /// thiscall(unit): its turn acceleration (radians/s per s, stats +0xB0; its group's with the group speed on)
    [address((ee)0x0054A504, (aoc)0x0055465B)]
    extern float32 turn_accel();

    [address((ee)0x00628E29, (aoc)0x0063DBDA)]
    extern void unlink();

    /// units aboard a transport or inside a building
    [address((ee)0x0042E347, (aoc)0x0040D5C5)]
    extern int32 count_inside();
}

/// a unit, building, ship or projectile: everything on the map that moves or is built
[@base(typeof(EEUnit)), size((ee)0x348, (aoc)0x364), c_type("ee_unit")]   // new, then its constructor 0x5E8394 / AoC 0x5F5999
[vtable((ee)0x00831494, (aoc)0x00846DDC)]
unsafe struct EEComplexUnit
{
    // FIELDS

    /// a building's queue
    [offset(0x4C), found(c)]
    int32* queue_begin;

    /// (lookup 0x4111F6)
    [offset(0x60), found(c)]
    int32 id;

    [offset(0x68), found(c)]
    int32 busy;

    /// radians/s, signed
    /// the turn rate now (radians/s, signed) - model_collide in both builds, model_turn_decide's caller
    [offset(0x17C), found(c)]
    float32 turn_rate;

    /// a projectile's: the attack's type data
    [offset(0x1C0), found(c)]
    EEUnitClass* attacker_type;

    [offset(0x1CC), found(c)]
    EEUnit* chain;

    /// set while inside a transport or building (EEUCTransport::vf63: none may board)
    [offset(0x1D8), found(c)]
    void* holder;

    /// its script state (AISTATE)
    [offset(0x1E8), found(c)]
    ee_ai_state* ai_state;

    /// its goal (GOAL)
    [offset(0x1EC), found(c)]
    EEUnitGoal* goal;

    /// the goal object (0x5ED24B casts it)
    [offset(0x1F0), found(c)]
    EEUnitGoal* goal_object;

    /// its behaviour (behaviour_table())
    [offset(0x208), found(c)]
    void* behaviour;

    /// the push velocity (tiles/s)
    /// the push velocity {x, y} (tiles/s; the collision test adds, movement.slot_pm_push moves the unit by it x a step x
    /// game.phys_substeps and zeroes it) - model_collide in both builds
    [offset(0x230), found(c)]
    vec2 push;

    /// what a building makes now
    [offset((ee)0x25C, (aoc)0x260), found(c)]
    int32 producing;

    /// the movement mode
    /// the movement mode (0x549F82 picks the model's step by it; 2 no collisions) - the push (movement.slot_pm_push) sets an
    /// idle unit's pushed walk (+0x260 13) only while it is neither 0 nor 5, in both builds
    [offset((ee)0x264, (aoc)0x268), found(c)]
    int32 move_mode;

    /// a missile's climb left (ms)
    [offset((ee)0x28C, (aoc)0x290), found(c)]
    int32 flight_ms;

    /// its own reload (ms, 0: its class's)
    [offset((ee)0x2A0, (aoc)0x2A4), found(c)]
    int32 reload;

    /// a transport's room left (EEUGLoadTransport::vf34; full below 1)
    /// 2 votes, UNIT_RELOAD's shift
    [offset((ee)0x2A4, (aoc)0x2A8), found(c)]
    int32 room;

    /// set while it moves
    /// set while it moves (model_collide's first test in both builds)
    [offset((ee)0x2D8, (aoc)0x2DC), found(c)]
    int32 moving;

    /// EEPMNaval::vf21's step test (0x550300 / AoC 0x55xxxx's read of +0x2E4) against TILE_SEA_CLASS
    [offset((ee)0x2E0, (aoc)0x2E4), found(c)]
    int32 sea_mode;

    /// frozen (the scenario effect)
    [offset((ee)0x33B, (aoc)0x347), found(c)]
    uint8 frozen;

    /// on a building's +0x30: built
    [offset((ee)0x340, (aoc)0x34C), found(c)]
    uint8 finished;

    [offset((ee)0x341, (aoc)0x34D), found(c)]
    uint8 ai_held;

    /// at work (the contact test passes it on a kind 6 / 4 goal)
    /// set while at work (byte): the contact test passes a unit with it on a kind 6 or 4 goal, but for one at the same
    /// work - in both builds' test
    [offset((ee)0x347, (aoc)0x353), found(c)]
    uint8 at_work;

    // FIELDS

    /// its waypoints (20-byte points, 0xCC a block): the front the current one, the last the destination
    [offset(0x7C), found(c)]
    deque waypoints;
    /// its waypoints left (the deque's size)
    [offset(0x8C), found(c)]
    int32 waypoints_left;
    /// its matrix (GETransformation, 3 x 4 floats by row): [1] [5] [9] the local y axis (facing), [3] [7] [11] the
    /// position in tiles
    [offset(0x130), found(b)]
    fixed float32 matrix[12];
    /// the speed, at least the type's minimum
    [offset(0x21C), found(c)]
    float32 speed_min;
    /// tiles/s (0 standing)
    [offset(0x238), found(c)]
    float32 speed;
    /// the type's size (+0x474) x the unit's scale, in tiles - a diameter to the circle test (EEPhysicsModel::vf15), the
    /// line's length to the ships' (EEPMNaval::vf16) - model_collide in both builds
    [offset(0x244), found(c)]
    float32 size;

    // VIRTUAL FUNCTIONS (its own in EEUnit's and RTSUnit's slots)

    /// may unit enter the tile this object stands on: decided for a gate (family 0x19, with hit points) by its chain's
    /// frozen flag (+0x33B / AoC +0x347), its goal and the asker's owner and diplomacy; AoC asks one test more (0x5FA661)
    [address((ee)0x005EC317, (aoc)0x005F9E81), overrides(typeof(EEUnit))]
    extern int8 lets_pass(EEComplexUnit* unit);

    // FUNCTIONS (thiscall on the unit)

    /// its top speed (tiles/s; its group's with the group speed on) - every movement model asks it
    [address((ee)0x0046123F, (aoc)0x0045E407)]
    extern float32 max_speed();
    /// its current waypoint (the list's front) into pt[3]: 0, else it has none (7)
    [address((ee)0x005EC69D, (aoc)0x005FA229)]
    extern int32 waypoint(float32* pt);
    /// one more waypoint at the end of its list; keep_edge 1 keeps a point on the map's edge lines
    [address((ee)0x005E8BAC, (aoc)0x005F625B)]
    extern void add_waypoint(float32* pt, int32 keep_edge);
    /// the goal into one of its goal slots
    [address((ee)0x004412D4, (aoc)0x0043D416)]
    [call_site("unload_retry_give", (ee)0x0048D03C, (aoc)0x00491B12)]   // the resource manager's unload retry
    extern void give_goal(EEUnitGoal* goal, int32 slot);
    /// the goal as its order (append: after the ones it has)
    [address((ee)0x005F05BD, (aoc)0x005FE863)]
    [call_site("repair_goal", (ee)0x0046DA88, (aoc)0x004706DF)]         // the game's repair orders, handed over
    extern void assign_goal(EEUnitGoal* goal, int8 append, int32 prio);

    /// freed: its slot 12 (+0x30), its references 0, then back to the game's pool (0x5F20FB / AoC 0x60096B)
    [address((ee)0x005ED71E, (aoc)0x005FB7FB), overrides(typeof(RTSUnit))]
    [vtable_site("unit_release")]
    extern void release();
}

/// an image of a unit type for a player, no map object (its constructor calls neither RTSUnit's nor EEUnit's): made by
/// 0x5F4608 (new 0x334, its constructor 0x5F3D4C(type id, player, flag); type 66 three more) into a list at its
/// caller's +0xE4 - callers 0x562D07, 0x64F72A (a tech's unit type) and the scenario's Move Unit effect
/// (EESEAMoveUnit::vf1) - what for (a placement preview?) not established. Its destructor 0x5F3DE5 tells 0x4CEEF4
/// (the player's index, the type id) unless +0x331 is set.
[size(0x334)]
[vtable((ee)0x00831588, (aoc)0x00846EF4)]
unsafe struct EEGhostUnit
{
    /// the player it is for (its +0x45C, the index, read by the destructor)
    [offset(0x294), found(c)]
    EEPlayer* player;
    /// its type's data (game.types[type id])
    [offset(0x298), found(c)]
    EEUnitClass* type_data;
    /// set: the destructor doesn't tell 0x4CEEF4
    [offset(0x331), found(c)]
    uint8 untold;
}

/// the game's functions on units that belong to no class: the unit an ordinary argument, all on the stack
static unsafe class unit
{
    [address((ee)0x00628BE7, (aoc)0x0063D998)]
    static code call_node_del_1;

    [address((ee)0x00628B7F, (aoc)0x0063D930)]
    static code call_node_del_2;

    [address((ee)0x006836F0, (aoc)0x0069D150)]
    static code fn_op_delete;

    /// the aim of u's shot at target (0x633413: the shooter its first argument)
    [address((ee)0x00633413, (aoc)0x0064939D), cdecl]
    [call_site("aim", (ee)0x0061E1D7, (aoc)0x006313D6)]                 // a unit's shot
    [call_site("missile_aim", (ee)0x0054E850, (aoc)0x0055B1C3)]         // a missile's
    static extern void aim(EEComplexUnit* u, EEUnit** target, float32* aim, void* zero);
}

/// a unit's script state, its +0x1E8 (a state of its .tai, engine-notes.md "Unit AI scripts")
unsafe struct ee_ai_state
{
    /// the state's id, read by every condition through a unit's +0x1E8
    [offset(0x24), found(c)]
    uint32 id;
}

/// the game's functions unit.c wraps, by the names the C side gives them
[c_prefix("")]
static unsafe class unit_functions
{
    [address((ee)0x0062CBB3, (aoc)0x00641A6A), cdecl]
    static extern char8 ballistic_game(EEUnit* p1, ee_unit_stats* p2, float32* p3, float32* p4, float32* p5);

    [address((ee)0x0052E5F7, (aoc)0x00539532), thiscall]
    static extern uint32 site_gatherers_game(void* p1, void* p2);

    /// cdecl(unit, player index, new type data)
    [address((ee)0x0062F192, (aoc)0x00644E95), cdecl]
    static extern int32 unit_convert(EEUnit* u, int32 pi, EEUnitClass* new_td);

    /// cdecl(unit, other)
    [address((ee)0x0062D5A4, (aoc)0x00642489), cdecl]
    static extern float32 unit_distance(EEUnit* a, EEUnit* b);

    /// thiscall(unit, slot): the slot's goal queue (unit +0x94 a vector of 0x28-byte slots; +4 a deque of goal
    /// pointers: +4 block map, +8 its size, +0xC the front's index, +0x10 the count; 0x400 a block) - slot 0 the
    /// orders, the front the current, shift appends
    [address((ee)0x005F3473, (aoc)0x005FA1CD), thiscall]
    static extern void* unit_goal_queue(void* p1, int32 p2);

}
