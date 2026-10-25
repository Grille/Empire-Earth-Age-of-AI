
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
    /// its owner
    [offset(0x18), found(c)]
    EEPlayer* owner;

    // VIRTUAL FUNCTIONS

    /// the object freed: the release 0x4FCAD3 calls it when the last reference goes (RTSUnit::vf13 0x628D98 jumps
    /// to the imported release, IAT_RELEASE)
    [slot(13), was("SLOT_UNIT_RELEASE")]
    extern void release();
}

/// an ambient object or resource site (a tree, a gold mine); the base of every unit
[@base(typeof(RTSUnit)), size(0x48), c_type("ee_unit")]   // new 0x48, then its constructor 0x6161F4 / AoC 0x628CD2
[vtable((ee)0x00831FD8, (aoc)0x00847A20)]
unsafe struct EEUnit
{
    // VIRTUAL FUNCTIONS (EEUnit's slots; EEComplexUnit has its own functions in them)

    /// may unit enter the tile this object stands on (the path search's tile test 0x41D36A asks every object there):
    /// EEUnit's by its type data (a flag +0x600, its family, +0x568 - AoC moved them all) and its hit points
    [slot(18), address((ee)0x006167C7, (aoc)0x006292C9), was("SLOT_UNIT_LETS_PASS")]
    extern int8 lets_pass(EEComplexUnit* unit);
    /// an attack's hit points (EEUnit::vf28 0x6172BC, the call 0x61E0B2), the attacker's player index, one 1; AoC put
    /// a stub (ret 4) at 28
    [slot((ee)28, (aoc)29), was("SLOT_UNIT_DAMAGE")]
    extern void damage(EEComplexUnit* attacker, int32 dmg, int32 player, int32 one);

    /// freed: its slot 12 (+0x30), its references 0, then deleted
    [address((ee)0x00616DBB, (aoc)0x006298E1), overrides(typeof(RTSUnit))]
    extern void release();
}

/// a unit, building, ship or projectile: everything on the map that moves or is built
[@base(typeof(EEUnit)), size((ee)0x348, (aoc)0x364), c_type("ee_unit")]   // new, then its constructor 0x5E8394 / AoC 0x5F5999
[vtable((ee)0x00831494, (aoc)0x00846DDC)]
unsafe struct EEComplexUnit
{
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
    /// line's length to the ships' (EEPMNaval::vf16) - FN_PM_COLLIDE in both builds
    [offset(0x244), found(c)]
    float32 size;

    // VIRTUAL FUNCTIONS (its own in EEUnit's and RTSUnit's slots)

    /// may unit enter the tile this object stands on: decided for a gate (family 0x19, with hit points) by its chain's
    /// frozen flag (+0x33B / AoC +0x347), its goal and the asker's owner and diplomacy; AoC asks one test more (0x5FA661)
    [address((ee)0x005EC317, (aoc)0x005F9E81), overrides(typeof(EEUnit))]
    extern int8 lets_pass(EEComplexUnit* unit);

    // FUNCTIONS (thiscall on the unit)

    /// its top speed (tiles/s; its group's with the group speed on) - every movement model asks it
    [address((ee)0x0046123F, (aoc)0x0045E407), was("FN_UNIT_MAX_SPEED")]
    extern float32 max_speed();
    /// its current waypoint (the list's front) into pt[3]: 0, else it has none (7)
    [address((ee)0x005EC69D, (aoc)0x005FA229), was("FN_UNIT_WAYPOINT")]
    extern int32 waypoint(float32* pt);
    /// one more waypoint at the end of its list; keep_edge 1 keeps a point on the map's edge lines
    [address((ee)0x005E8BAC, (aoc)0x005F625B), was("FN_ADD_WAYPOINT")]
    extern void add_waypoint(float32* pt, int32 keep_edge);
    /// the goal into one of its goal slots
    [address((ee)0x004412D4, (aoc)0x0043D416), was("FN_GIVE_GOAL")]
    [call_site("unload_retry_give", (ee)0x0048D03C, (aoc)0x00491B12)]   // the resource manager's unload retry
    extern void give_goal(EEUnitGoal* goal, int32 slot);
    /// the goal as its order (append: after the ones it has)
    [address((ee)0x005F05BD, (aoc)0x005FE863), was("FN_SET_GOAL")]
    [call_site("repair_goal", (ee)0x0046DA88, (aoc)0x004706DF)]         // the game's repair orders, handed over
    extern void set_goal(EEUnitGoal* goal, int8 append, int32 prio);

    /// freed: its slot 12 (+0x30), its references 0, then back to the game's pool (0x5F20FB / AoC 0x60096B)
    [address((ee)0x005ED71E, (aoc)0x005FB7FB), overrides(typeof(RTSUnit)), was("FN_UNIT_RELEASE")]
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
    /// its type's data (G_TYPES[type id])
    [offset(0x298), found(c)]
    EEUnitClass* type_data;
    /// set: the destructor doesn't tell 0x4CEEF4
    [offset(0x331), found(c)]
    uint8 untold;
}

/// the game's functions on units that belong to no class: the unit an ordinary argument, all on the stack
static unsafe class unit
{
    /// the aim of u's shot at target (0x633413: the shooter its first argument)
    [address((ee)0x00633413, (aoc)0x0064939D), cdecl, was("FN_AIM")]
    [call_site("aim", (ee)0x0061E1D7, (aoc)0x006313D6)]                 // a unit's shot
    [call_site("missile_aim", (ee)0x0054E850, (aoc)0x0055B1C3)]         // a missile's
    static extern void aim(EEComplexUnit* u, EEUnit** target, float32* aim, void* zero);
}
